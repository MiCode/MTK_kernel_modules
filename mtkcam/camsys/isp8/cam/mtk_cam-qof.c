// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2024 MediaTek Inc.

#include <linux/iopoll.h>

#include "mtk_cam.h"
#include "mtk_cam-debug.h"
#include "mtk_cam-raw.h"
#include "mtk_cam-qof.h"
#include "mtk_cam-qof_regs.h"
#include "mtk_cam-raw_regs.h"
#include "mtk_cam-reg_utils.h"
#include "mtk_cam-bit_mapping.h"
#include "mtk_cam-hsf.h"

/* QOF timer freq = (TM_FREQ / 2 / (QOF_TIMER_FREQ_DIV+1)) */
#define TM_FREQ_KHZ						208000
#define QOF_TIMER_FREQ_DIV				3

/* TODO: tune this threshold */
#define HW_TIMER_MARGIN_US					1000
/* NOTE: reverse 500ns in HW_TIMER_MARGIN for PWR_ISO_0_DEF = 0xD */
#define PWR_ISO_0_DEF		0xD

/* NOTE: PWR_OFF_MAX_THRESHOLD_US should be large enough to avoid */
/* overlay of on_proc and off_proc(generally 10us) */
#define PWR_OFF_MAX_THRESHOLD_US			100

#define PWR_STATE_POLLING_TIMEOUT_SHORT_US		50
#define PWR_STATE_POLLING_TIMEOUT_LONG_US		600

#define PWR_ON_OFF_TIMEOUT		5000

static u32 force_dump_raw_map;

#define FORCE_DUMP(raw_id) (force_dump_raw_map & BIT(raw_id))

//#define WORKAROUND_VOTER_SET_OUTSIDE_OFF_PROC

static u32 itc_readl(struct mtk_raw_device *raw,
					 void __iomem *base, u32 offset);
static u32 itc_readl_relaxed(struct mtk_raw_device *raw,
							 void __iomem *base, u32 offset);

static u32 qof_readl(struct mtk_raw_device *raw,
					 void __iomem *base, u32 offset);
static u32 qof_readl_relaxed(struct mtk_raw_device *raw,
							 void __iomem *base, u32 offset);
static void qof_writel(struct mtk_raw_device *raw, u32 val,
					   void __iomem *base, u32 offset);
static void qof_writel_relaxed(struct mtk_raw_device *raw, u32 val,
							   void __iomem *base, u32 offset);

struct raw_io_ops qof_io_ops = {
	.readl = qof_readl,
	.readl_relaxed = qof_readl_relaxed,
	.writel = qof_writel,
	.writel_relaxed = qof_writel_relaxed,
};

struct raw_io_ops itc_only_io_ops = {
	.readl = itc_readl,
	.readl_relaxed = itc_readl_relaxed,
	.writel = basic_writel,
	.writel_relaxed = basic_writel_relaxed,
};

enum QOF_POWER_STATE {
	PS_REST			 = 1 << 0,
	PS_ON			 = 1 << 1,
	PS_GCE_SAVE		 = 1 << 2,
	PS_OFF_PROC		 = 1 << 3,
	PS_OFF			 = 1 << 4,
	PS_ON_PROC		 = 1 << 5,
	PS_GCE_RESTORE	 = 1 << 6,
	PS_RESERVED		 = 1 << 7,
};

static inline u32 wait_itc_done(struct mtk_raw_device *raw)
{
	u32 ret, val;

	ret = readx_poll_timeout_atomic(readl, raw->qof_base + REG_QOF_CAM_A_QOF_DONE_STATUS_1,
								 val, (val & 0x2), 30 /*us*/, 600);

	return ret;
}

static inline int avoid_power_state(struct mtk_raw_device *raw, u32 state)
{
	int ret;
	u32 val;

	if (state & (PS_GCE_SAVE | PS_ON_PROC | PS_GCE_RESTORE)) {
		// NOTE: PS_ON_PROC could take as long as 300us to finish
		// GCE SAVE/RESTORE depends on GCE operation
		ret = readx_poll_timeout(readl, raw->qof_base + REG_QOF_CAM_A_QOF_STATE_DBG_1,
								 val, !(val & state),
								 50 /*us*/, PWR_STATE_POLLING_TIMEOUT_LONG_US);
	} else {
		ret = readx_poll_timeout_atomic(readl, raw->qof_base + REG_QOF_CAM_A_QOF_STATE_DBG_1,
								 val, !(val & state),
								 15 /*us*/, PWR_STATE_POLLING_TIMEOUT_SHORT_US);
	}

	if (ret < 0)
		dev_info(raw->dev, "%s: error: timeout! (val: %u, state: %u)\n",
				 __func__, val, state);

	return ret;
}

u32 qof_get_mtcmos_margin(void)
{
	return HW_TIMER_MARGIN_US;
}

int qof_reset(struct mtk_raw_device *raw)
{
	struct mtk_cam_device *cam = raw->cam;
	u32 rst_val, rst_val_off;

	rst_val = readl(cam->qoftop_base + REG_QOF_CAM_TOP_QOF_SW_RST);
	rst_val_off = rst_val;
	switch (raw->id) {
	case RAW_A:
		SET_FIELD(&rst_val, QOF_CAM_TOP_QOF_SW_RST_RAWA_SW_RST, 1);
		SET_FIELD(&rst_val_off, QOF_CAM_TOP_QOF_SW_RST_RAWA_SW_RST, 0);
		break;
	case RAW_B:
		SET_FIELD(&rst_val, QOF_CAM_TOP_QOF_SW_RST_RAWB_SW_RST, 1);
		SET_FIELD(&rst_val_off, QOF_CAM_TOP_QOF_SW_RST_RAWB_SW_RST, 0);
		break;
	case RAW_C:
		SET_FIELD(&rst_val, QOF_CAM_TOP_QOF_SW_RST_RAWC_SW_RST, 1);
		SET_FIELD(&rst_val_off, QOF_CAM_TOP_QOF_SW_RST_RAWC_SW_RST, 0);
		break;
	default:
		dev_info(raw->dev, "%s: raw id %d not found", __func__, raw->id);
		return -1;
	}

	// NOTE: QOF_TIME_STAMP_1 need to be reset to 0 before QOF_SW_RST
	//       otherwise, CQ trigger by QOF may fail to work
	writel(0, raw->qof_base + REG_QOF_CAM_A_QOF_TIME_STAMP_1);
	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id))
		dev_info(raw->dev, "%s: rst_val 0x%08x", __func__, rst_val);

	// NOTE: QOF_SW_RST should be TOGGLED
	writel(rst_val, cam->qoftop_base + REG_QOF_CAM_TOP_QOF_SW_RST);
	writel(rst_val_off, cam->qoftop_base + REG_QOF_CAM_TOP_QOF_SW_RST);

	qof_init_timer_freq(raw);

	return 0;
}

static inline void qof_setup_pwr_th(struct mtk_raw_device *raw)
{
	u32 val = 0;

	SET_FIELD(&val, QOF_CAM_A_PWR_ISO_0_TH_1, PWR_ISO_0_DEF);
	writel(val, raw->qof_base + REG_QOF_CAM_A_QOF_POWER_ISO_CYCLE_1);

	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id))
		dev_info(raw->dev, "qof: %s: POWER_ISO_CYCLE 0x%08x", __func__,
				 readl(raw->qof_base + REG_QOF_CAM_A_QOF_POWER_ISO_CYCLE_1));
}

void qof_setup_ctrl(struct mtk_raw_device *raw, int on)
{
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&raw->qof_ctrl_lock, flags);

	val = readl(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);
	SET_FIELD(&val, QOF_CAM_A_QOF_CQ_EN_1, on);
	SET_FIELD(&val, QOF_CAM_A_HW_SEQ_EN_1, on);

	//NOTE: 0: RMS mtcmos independent; 1: RMS/RAW controlled by QOF
	SET_FIELD(&val, QOF_CAM_A_OPT_MTC_ACT_1, 1);

	SET_FIELD(&val, QOF_CAM_A_CQ_HW_CLR_EN_1, on);
	SET_FIELD(&val, QOF_CAM_A_CQ_HW_SET_EN_1, on);
	SET_FIELD(&val, QOF_CAM_A_RAW_HW_CLR_EN_1, on);
	SET_FIELD(&val, QOF_CAM_A_RAW_HW_SET_EN_1, on);

	SET_FIELD(&val, QOF_CAM_A_RTC_EN_1, on);

	writel(val, raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);
	raw->trigger_cq_by_qof = !!on;

	spin_unlock_irqrestore(&raw->qof_ctrl_lock, flags);

	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id))
		dev_info(raw->dev, "qof: %s: QOF_CTL 0x%08x", __func__,
				 readl(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1));
}

void qof_sof_src_sel(struct mtk_raw_device *raw, int exp_num,
					 bool with_tg, int sv_last_tag)
{
	struct mtk_cam_device *cam = raw->cam;
	u32 otf_dc_mode = 0;
	u32 exp_sof_sel = 0;
	u32 val;

	if (with_tg) {
		if (exp_num >= 2)
			otf_dc_mode = 1;
	} else {
		otf_dc_mode = 2;
		exp_sof_sel = sv_last_tag;
	}

	val = readl_relaxed(cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL);

	switch (raw->id) {
	case RAW_A:
		SET_FIELD(&val, QOF_CAM_TOP_OTF_DC_MODE_1, otf_dc_mode);
		SET_FIELD(&val, QOF_CAM_TOP_DCIF_EXP_SOF_SEL_1, exp_sof_sel);
		break;
	case RAW_B:
		SET_FIELD(&val, QOF_CAM_TOP_OTF_DC_MODE_2, otf_dc_mode);
		SET_FIELD(&val, QOF_CAM_TOP_DCIF_EXP_SOF_SEL_2, exp_sof_sel);
		break;
	case RAW_C:
		SET_FIELD(&val, QOF_CAM_TOP_OTF_DC_MODE_3, otf_dc_mode);
		SET_FIELD(&val, QOF_CAM_TOP_DCIF_EXP_SOF_SEL_3, exp_sof_sel);
		break;
	default:
		dev_info(raw->dev, "%s: raw id %d not found", __func__, raw->id);
		return;
	}

	writel(val, cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL);

	dev_info(raw->dev, "qof: %s: TOP_CTL 0x%08x exp:%d/tg:%d", __func__,
				 readl(cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL),
				 exp_num, with_tg);
}

void mtk_cam_enable_itc(struct mtk_raw_device *raw)
{
	// NOTE: for isp8 ITC need to be enable all the time,
	// setup ITC before setup int_en (first CQ)
	struct mtk_cam_device *cam = raw->cam;
	u32 val;
	bool qof_enabled = false;

	val = readl(cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL);

	switch (raw->id) {
	case RAW_A:
		SET_FIELD(&val, QOF_CAM_TOP_ITC_SRC_SEL_1, 1);
		qof_enabled = val & FBIT(QOF_CAM_TOP_QOF_SUBA_EN);
		break;
	case RAW_B:
		SET_FIELD(&val, QOF_CAM_TOP_ITC_SRC_SEL_2, 1);
		qof_enabled = val & FBIT(QOF_CAM_TOP_QOF_SUBB_EN);
		break;
	case RAW_C:
		SET_FIELD(&val, QOF_CAM_TOP_ITC_SRC_SEL_3, 1);
		qof_enabled = val & FBIT(QOF_CAM_TOP_QOF_SUBC_EN);
		break;
	default:
		return;
	}

	dev_info(raw->dev, "qof: %s: misc1/2/3 0x%x 0x%x 0x%x (0x%x 0x%x 0x%x)", __func__,
			 readl(raw->base + REG_CAMCTL_MISC),
			 readl(raw->yuv_base + REG_CAMCTL2_MISC),
			 readl(raw->rms_base + REG_CAMCTL3_MISC),
			 readl(raw->base_inner + REG_CAMCTL_MISC),
			 readl(raw->yuv_base_inner + REG_CAMCTL2_MISC),
			 readl(raw->rms_base_inner + REG_CAMCTL3_MISC));

	writel(val, cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL);

	raw->io_ops = (qof_enabled) ? &qof_io_ops : &itc_only_io_ops;

	dev_info(raw->dev, "qof: %s: top_ctrl 0x%x itc_status 0x%x", __func__,
			 readl(raw->cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL),
			 readl(raw->cam->qoftop_base + REG_QOF_CAM_TOP_ITC_STATUS));
}

#define ITC_RESET		(BIT(9)|BIT(8)|BIT(7)|BIT(6))
void mtk_cam_reset_itc(struct mtk_cam_device *cam)
{
	int i;
	u32 val;

	writel(ITC_RESET, cam->qoftop_base + REG_QOF_CAM_TOP_QOF_SW_RST);
	writel(0, cam->qoftop_base + REG_QOF_CAM_TOP_QOF_SW_RST);

	val = readl(cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL);
	SET_FIELD(&val, QOF_CAM_TOP_ITC_SRC_SEL_1, 1);
	SET_FIELD(&val, QOF_CAM_TOP_ITC_SRC_SEL_2, 1);
	SET_FIELD(&val, QOF_CAM_TOP_ITC_SRC_SEL_3, 1);
	writel(val, cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL);

	for (i = 0; i < cam->engines.num_raw_devices; i++) {
		struct mtk_raw_device *raw_dev;

		raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);
		raw_dev->io_ops = &itc_only_io_ops;
	}

	dev_info(cam->dev, "qof: %s: top_ctrl 0x%x itc_status 0x%x", __func__,
			 readl(cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL),
			 readl(cam->qoftop_base + REG_QOF_CAM_TOP_ITC_STATUS));
}

void qof_init_timer_freq(struct mtk_raw_device *raw)
{
	writel_relaxed(QOF_TIMER_FREQ_DIV,
			   raw->qof_base + REG_QOF_CAM_A_QOF_TIME_STAMP_1);
}

void qof_setup_hw_timer(struct mtk_raw_device *raw, u32 interval_us)
{
	u32 timer_freq_khz =
		(TM_FREQ_KHZ / 2 / (QOF_TIMER_FREQ_DIV + 1));
	u32 mtcmos_cycle;
	u32 pwr_off_max;

	if (!interval_us) {
		mtcmos_cycle = 0;
		pwr_off_max = 0;
	} else {
		mtcmos_cycle = (interval_us - qof_get_mtcmos_margin()) * timer_freq_khz / 1000;
		pwr_off_max = (interval_us - qof_get_mtcmos_margin() - PWR_OFF_MAX_THRESHOLD_US)
		* timer_freq_khz / 1000;
	}

	writel_relaxed(mtcmos_cycle,
				   raw->qof_base + REG_QOF_CAM_A_QOF_MTC_CYC_MAX_1);
	writel_relaxed(pwr_off_max,
				   raw->qof_base + REG_QOF_CAM_A_QOF_PWR_OFF_MAX_1);
	writel_relaxed(PWR_ON_OFF_TIMEOUT,
				   raw->qof_base + REG_QOF_CAM_A_QOF_TIMEOUT_MAX_1);

	qof_dump_hw_timer(raw);

	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id)) {
		dev_info(raw->dev, "qof: %s: freq(khz)/frm_tm(us)/mtcmos/max_pwr_off: %u/%u/%u/%u",
				__func__,
				timer_freq_khz, interval_us,
				mtcmos_cycle, pwr_off_max);
	}
}

int qof_enable(struct mtk_raw_device *raw, bool enable)
{
	int en = enable ? 1 : 0;

	struct mtk_cam_device *cam = raw->cam;
	u32 val;

	if (enable == qof_is_enabled(raw))
		return 0;

	val = readl(cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL);

	switch (raw->id) {
	case RAW_A:
		SET_FIELD(&val, QOF_CAM_TOP_QOF_SUBA_EN, en);
		break;
	case RAW_B:
		SET_FIELD(&val, QOF_CAM_TOP_QOF_SUBB_EN, en);
		break;
	case RAW_C:
		SET_FIELD(&val, QOF_CAM_TOP_QOF_SUBC_EN, en);
		break;
	default:
		return -1;
	}

	if (!en) {
		qof_set_force_dump(raw, true);
		qof_dump_trigger_cnt(raw);
		qof_set_force_dump(raw, false);
	}

	qof_reset(raw);
	qof_setup_ctrl(raw, en);
	if (en)
		qof_setup_pwr_th(raw);

	SET_FIELD(&val, QOF_CAM_TOP_SEQUENCE_MODE, QOF_SEQ_MODE_RTC_THEN_ITC);

	writel(val, cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL);

	if (en)
		writel(0xfff, cam->qoftop_base + REG_QOF_CAM_TOP_QOF_INT_EN);

	raw->io_ops = (enable) ? &qof_io_ops : &itc_only_io_ops;

	dev_info(raw->dev, "qof: %s: %s TOP_CTL 0x%08x",
			 __func__, (enable) ? "enable" : "disable",
			 readl(cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL));
	dev_info(raw->dev, "qof: %s: QOF_CAM_TOP_ITC_STATUS 0x%08x", __func__,
			 readl(cam->qoftop_base + REG_QOF_CAM_TOP_ITC_STATUS));

	return 0;
}

int qof_enable_cq_trigger_by_qof(struct mtk_raw_device *raw, bool enable)
{
	u32 val;
	u32 on = (enable) ? 1 : 0;
	unsigned long flags;

	if (!qof_is_enabled(raw)) {
		if (CAM_DEBUG_ENABLED(QOF))
			dev_info(raw->dev, "%s: raw id %d not enabled", __func__, raw->id);
		return 0;
	}

	spin_lock_irqsave(&raw->qof_ctrl_lock, flags);

	val = readl(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);
	SET_FIELD(&val, QOF_CAM_A_QOF_CQ_EN_1, on);
	writel(val, raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);
	raw->trigger_cq_by_qof = enable;

	spin_unlock_irqrestore(&raw->qof_ctrl_lock, flags);

	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id))
		dev_info(raw->dev, "qof: %s: QOF_CTL 0x%08x", __func__,
				 readl(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1));

	return 0;
}

// TODO: optimize
bool qof_is_enabled(struct mtk_raw_device *dev)
{
	bool enabled = false;
	struct mtk_cam_device *cam = dev->cam;
	u32 val;

	if (!cam)
		goto OUT;

	val = readl_relaxed(cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL);

	switch (dev->id) {
	case RAW_A:
		enabled = !!(val & FBIT(QOF_CAM_TOP_QOF_SUBA_EN));
		break;
	case RAW_B:
		enabled = !!(val & FBIT(QOF_CAM_TOP_QOF_SUBB_EN));
		break;
	case RAW_C:
		enabled = !!(val & FBIT(QOF_CAM_TOP_QOF_SUBC_EN));
		break;
	default:
		break;
	}

OUT:
	return enabled;
}

int qof_setup_twin(struct mtk_raw_device *raw, bool is_master, bool next_raw)
{
	int ret = 0;
	u32 val = 0;
	u32 voter_sel = (is_master) ? 0 : 1; //0: from raw; 1: from master
	unsigned long flags;

	if (!GET_PLAT_HW(qof_support))
		return ret;

	// NOTE: raw A emits signal to raw B, raw B to raw C
	ret = mtk_cam_hsf_qof_config(raw, is_master, is_master, !next_raw);

	if (ret) {
		dev_info(raw->dev, "ERROR: fail to setup QOF lock");
		return ret;
	}

	spin_lock_irqsave(&raw->qof_ctrl_lock, flags);

	val = readl(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);
	SET_FIELD(&val, QOF_CAM_A_ON_SEL_1, voter_sel);
	SET_FIELD(&val, QOF_CAM_A_OFF_SEL_1, voter_sel);
	writel(val, raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);

	spin_unlock_irqrestore(&raw->qof_ctrl_lock, flags);

	dev_info(raw->dev, "qof: %s: qof_ctrl val 0x%x top_ctrl 0x%x", __func__,
			 readl(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1),
			 readl(raw->cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL));

	return ret;
}

void qof_set_cq_start_max(struct mtk_raw_device *dev, int scq_ms)
{
	u32 start_period;

	start_period = (scq_ms == -1) ? 0xFFFFFFFF :
		scq_ms * (TM_FREQ_KHZ / ((QOF_TIMER_FREQ_DIV + 1) * 2));

	writel(start_period, dev->qof_base + REG_QOF_CAM_A_QOF_CQ_START_MAX_1);
	if (CAM_DEBUG_ENABLED(QOF))
		dev_info(dev->dev, "[%s] REG_QOF_CAM_A_QOF_CQ_START_MAX_1:0x%08x (%dms)\n",
				 __func__, readl(dev->qof_base + REG_QOF_CAM_A_QOF_CQ_START_MAX_1), scq_ms);
}

int __qof_mtcmos_raw_voter(struct mtk_raw_device *raw, bool enable, const char *caller)
{
	u32 qof_ctrl_write = 0;
	u32 pwr_state_wait = 0;
	u32 state_dbg = 0;
	u32 val = 0;
	int ret = 0;
	unsigned long flags;
	unsigned long flags_qof_ctrl;

	if (!qof_is_enabled(raw)) {
		if (CAM_DEBUG_ENABLED(QOF))
			dev_info(raw->dev, "[%s] qof not enabled", __func__);
		return 0;
	}

	spin_lock_irqsave(&raw->apmcu_voter_lock, flags);

	if (enable)
		raw->apmcu_voter_cnt++;
	else
		raw->apmcu_voter_cnt--;

	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id))
		dev_info(raw->dev, "[%s] %s: voter[%d]", __func__, caller, raw->apmcu_voter_cnt);

	if (raw->apmcu_voter_cnt == 1) {
		qof_ctrl_write = FBIT(QOF_CAM_A_APMCU_SET_1);
		SET_FIELD(&pwr_state_wait, QOF_CAM_A_POWER_STATE_1, PS_ON);
	} else if (raw->apmcu_voter_cnt == 0) {
		qof_ctrl_write = FBIT(QOF_CAM_A_APMCU_CLR_1);
	} else if (raw->apmcu_voter_cnt < 0) {
		dev_info(raw->dev, "[%s] ERROR: voter cnt negative %d", __func__,
				raw->apmcu_voter_cnt);
		goto UNLOCK;
	} else
		goto UNLOCK;

	spin_lock_irqsave(&raw->qof_ctrl_lock, flags_qof_ctrl);
	val = readl(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);

#ifdef WORKAROUND_VOTER_SET_OUTSIDE_OFF_PROC
	if (qof_ctrl_write | FBIT(QOF_CAM_A_APMCU_SET_1)) {
		// NOTE: workaround for HW issue
		avoid_power_state(raw, PS_OFF_PROC);
	}
#endif

	writel(val | qof_ctrl_write, raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);
	spin_unlock_irqrestore(&raw->qof_ctrl_lock, flags_qof_ctrl);

	if (pwr_state_wait) {
		// NOTE: could be in isr context
		ret = readx_poll_timeout_atomic(readl, raw->qof_base + REG_QOF_CAM_A_QOF_STATE_DBG_1,
					 state_dbg, state_dbg & pwr_state_wait,
					 50 /* delay, us */, PWR_STATE_POLLING_TIMEOUT_LONG_US);

		if (ret < 0)
			dev_info(raw->dev, "[%s] ERROR: pwr_state polling timeout", __func__);
	}

UNLOCK:
	spin_unlock_irqrestore(&raw->apmcu_voter_lock, flags);
	return ret;
}

void __qof_mtcmos_voter_handle(struct mtk_cam_engines *eng,
	unsigned int used_raw, struct qof_voter_handle *handle,
	const char *caller)
{
	int temp = 0;

	used_raw = bit_map_subset_of(MAP_HW_RAW, used_raw);
	if (handle->used_raw != used_raw) {
		temp = handle->used_raw;
		__qof_mtcmos_voter(eng, used_raw, true, caller);
		handle->used_raw = used_raw;
	}

	if (temp)
		__qof_mtcmos_voter(eng, temp, false, caller);
}

// TODO: synchronization
// will be run on threaded irq/done workqueue/flow workqueue
int __qof_mtcmos_voter(struct mtk_cam_engines *eng,
	unsigned int used_engine, bool enable,
	const char *caller)
{
	int raw_num = eng->num_raw_devices;
	unsigned long submask;
	struct mtk_raw_device *raw;
	int i;

	submask = bit_map_subset_of(MAP_HW_RAW, used_engine);
	for (i = 0; i < raw_num && submask; i++, submask >>= 1) {
		if (!(submask & 0x1))
			continue;
		raw = dev_get_drvdata(eng->raw_devs[i]);

		__qof_mtcmos_raw_voter(raw, enable, caller);
	}

	return 0;
}

int qof_reset_mtcmos_raw_voter(struct mtk_raw_device *raw)
{
	int ret = 0;
	u32 val;
	unsigned long flags;
	unsigned long flags_qof_ctrl;

	spin_lock_irqsave(&raw->apmcu_voter_lock, flags);

	if (raw->apmcu_voter_cnt > 1) {
		dev_info(raw->dev, "WARNING: QOF apmcu voter is %d (>1)",
				raw->apmcu_voter_cnt);
		/* WRAP_AEE_EXCEPTION("QOF", "apmuc voter cnt error"); */
	}

	spin_lock_irqsave(&raw->qof_ctrl_lock, flags_qof_ctrl);
	val = readl(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);
	writel(val | FBIT(QOF_CAM_A_APMCU_CLR_1),
		   raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);
	spin_unlock_irqrestore(&raw->qof_ctrl_lock, flags_qof_ctrl);

	raw->apmcu_voter_cnt = 0;

	spin_unlock_irqrestore(&raw->apmcu_voter_lock, flags);
	return ret;
}

int qof_reset_mtcmos_voter(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_cam_engines *eng = &cam->engines;
	int raw_num = eng->num_raw_devices;
	unsigned long submask;
	struct mtk_raw_device *raw;
	int i;

	submask = bit_map_subset_of(MAP_HW_RAW, ctx->used_engine);
	for (i = 0; i < raw_num && submask; i++, submask >>= 1) {
		if (!(submask & 0x1))
			continue;

		raw = dev_get_drvdata(eng->raw_devs[i]);
		qof_reset_mtcmos_raw_voter(raw);
	}

	return 0;
}

static inline int read_replace_qof_cq_addr(const struct mtk_raw_device *raw,
										   void __iomem **base, u32 *offset)
{
	u32 _off = *offset;

	void *_base;
	u32 _offset;

	// TODO: use define instead of debug opt?
	if (CAM_DEBUG_ENABLED(QOF_ADDR)) {
		_base = *base;
		_offset = *offset;
	}

#define CHANGE_TO(b) {*offset = b; break;}
	if (*base == raw->base) {
		switch (_off) {
		//CQ addr
		case REG_CAMCQ_CQ_THR0_BASEADDR:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ1_DATA_1)
		case REG_CAMCQ_CQ_THR0_BASEADDR_MSB:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ2_DATA_1)
		case REG_CAMCQ_CQ_THR0_DESC_SIZE:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ3_DATA_1)
		case REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ4_DATA_1)
		case REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2_MSB:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ5_DATA_1)
		case REG_CAMCQ_CQ_SUB_THR0_DESC_SIZE_2:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ6_DATA_1)
		default:
			return 0;
		}
	} else
		return 0;
#undef CHANGE_TO

	*base = raw->qof_base;

	if (CAM_DEBUG_ENABLED(QOF_ADDR))
		dev_info(raw->dev, "%s: {%p, 0x%08x} -> {%p, 0x%08x}",
				__func__,
				_base, _offset,
				*base, *offset);

	return 1;
}

static inline int read_replace_itc_addr(const struct mtk_raw_device *raw,
										void __iomem **base, u32 *offset)
{
	u32 _off = *offset;

	void *_base;
	u32 _offset;

	// TODO: use define instead of debug opt?
	if (CAM_DEBUG_ENABLED(QOF_ADDR)) {
		_base = *base;
		_offset = *offset;
	}

#define CHANGE_TO(b) {*offset = b; break;}
	if (*base == raw->base) {
		switch (_off) {
		// interrupt
		case REG_CAMCTL_INT_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT1_STATUS_1)
		case REG_CAMCTL_INT2_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT2_STATUS_1)
		case REG_CAMCTL_INT3_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT3_STATUS_1)
		//case REG_CAMCTL_INT4_STATUS:
			//CHANGE_TO(REG_QOF_CAM_A_INT4_STATUS_1)
		case REG_CAMCTL_INT5_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT5_STATUS_1)
		case REG_CAMCTL_INT6_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT6_STATUS_1)
		case REG_CAMCTL_INT7_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT7_STATUS_1)
		case REG_CAMCTL_INT8_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT8_STATUS_1)
		case REG_CAMCTL_INT17_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT9_STATUS_1)
		case REG_CAMCTL_INT18_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT10_STATUS_1)
		case REG_CAMCTL_INT19_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT11_STATUS_1)
		case REG_CAMCTL_INT20_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT12_STATUS_1)
		case REG_CAMCTL_INT21_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT13_STATUS_1)
		case REG_CAMCTL_INT25_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT14_STATUS_1)
		case REG_CAMCTL_INT26_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT15_STATUS_1)
		case REG_CAMCTL_TFMR_INT_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT16_STATUS_1)
		case REG_CAMCTL_TFMR_INT2_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT17_STATUS_1)
		case REG_CAMCTL_TFMR_INT3_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT18_STATUS_1)
		//case REG_CAMCTL_TFMR_INT4_STATUS:
			//CHANGE_TO(REG_QOF_CAM_A_INT19_STATUS_1)
		case REG_CAMCTL_TFMR_INT5_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT20_STATUS_1)
		case REG_CAMCTL_TFMR_INT6_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT21_STATUS_1)
		case REG_CAMCTL_TFMR_INT7_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT22_STATUS_1)
		case REG_CAMCTL_TFMR_INT8_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT23_STATUS_1)
		default:
			return 0;
		}
	} else if (*base == raw->yuv_base) {
		switch (_off) {
		case REG_CAMCTL2_INT_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT24_STATUS_1)
		case REG_CAMCTL2_INT2_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT25_STATUS_1)
		//case REG_CAMCTL2_INT3_STATUS:
			//CHANGE_TO(REG_QOF_CAM_A_INT26_STATUS_1)
		case REG_CAMCTL2_INT5_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT27_STATUS_1)
		case REG_CAMCTL2_INT8_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT28_STATUS_1)
		case REG_CAMCTL2_INT17_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT29_STATUS_1)
		case REG_CAMCTL2_INT25_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT30_STATUS_1)
		case REG_CAMCTL2_TFMR_INT_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT31_STATUS_1)
		case REG_CAMCTL2_TFMR_INT2_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT32_STATUS_1)
		//case REG_CAMCTL2_INT18_STATUS:
			//CHANGE_TO(REG_QOF_CAM_A_INT33_STATUS_1)
		case REG_CAMCTL2_TFMR_INT5_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT34_STATUS_1)
		case REG_CAMCTL2_TFMR_INT8_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT35_STATUS_1)
		default:
			return 0;
		}
	} else if (*base == raw->rms_base) {
		switch (_off) {
		case REG_CAMCTL3_INT_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT36_STATUS_1)
		case REG_CAMCTL3_INT8_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT37_STATUS_1)
		case REG_CAMCTL3_TFMR_INT_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT38_STATUS_1)
		case REG_CAMCTL3_TFMR_INT8_STATUS:
			CHANGE_TO(REG_QOF_CAM_A_INT39_STATUS_1)
		default:
			return 0;
		}
	} else
		return 0;
#undef CHANGE_TO

	*base = raw->qof_base;

	if (CAM_DEBUG_ENABLED(QOF_ADDR))
		dev_info(raw->dev, "%s: {%p, 0x%08x} -> {%p, 0x%08x}",
				__func__,
				_base, _offset,
				*base, *offset);

	return 1;
}

static inline int read_replace_rtc_addr(const struct mtk_raw_device *raw,
										void __iomem **base, u32 *offset)
{
	u32 _off = *offset;

	void *_base;
	u32 _offset;

	if (CAM_DEBUG_ENABLED(QOF_ADDR)) {
		_base = *base;
		_offset = *offset;
	}

#define CHANGE_TO(to) {*offset = to; break;}
	if (*base == raw->base) {
		// TODO: check REG_TG_INTER_ST update behavior
		switch (_off) {
		case REG_FRAME_IDX:
			CHANGE_TO(REG_QOF_CAM_A_TRANS1_DATA_1)
		case REG_TG_INTER_ST:
			CHANGE_TO(REG_QOF_CAM_A_TRANS3_DATA_1)
		default:
			return 0;
		}
	} else if (*base == raw->base_inner) {
		switch (_off) {
		case REG_FRAME_IDX:
			CHANGE_TO(REG_QOF_CAM_A_TRANS2_DATA_1)
		case REG_CAMCTL_MOD5_EN:
			CHANGE_TO(REG_QOF_CAM_A_TRANS4_DATA_1)
		default:
			return 0;
		}
	} else
		return 0;
#undef CHANGE_TO

	*base = raw->qof_base;

	if (CAM_DEBUG_ENABLED(QOF_ADDR))
		dev_info(raw->dev, "%s: {%p, 0x%08x} -> {%p, 0x%08x}",
				__func__,
				_base, _offset,
				*base, *offset);

	return 1;
}

void qof_setup_rtc(struct mtk_raw_device *dev)
{
	struct mtk_cam_device *cam = dev->cam;

	u32 base = (u32)((u64)dev->base_reg_addr - (u64)cam->base_reg_addr);
	u32 base_inner = (u32)((u64)dev->base_inner_reg_addr - (u64)cam->base_reg_addr);

	if (CAM_DEBUG_ENABLED(QOF))
		dev_info(dev->dev,
				 "qof: %s: base_reg_addr 0x%x base_inner_reg_addr 0x%x",
				 __func__, base, base_inner);

	writel_relaxed(base + REG_FRAME_IDX,
				   dev->qof_base + REG_QOF_CAM_A_TRANS1_ADDR_1);
	writel_relaxed(base_inner + REG_FRAME_IDX,
				   dev->qof_base + REG_QOF_CAM_A_TRANS2_ADDR_1);
	writel_relaxed(base + REG_TG_INTER_ST,
				   dev->qof_base + REG_QOF_CAM_A_TRANS3_ADDR_1);
	writel_relaxed(base_inner + REG_CAMCTL_MOD5_EN,
				   dev->qof_base + REG_QOF_CAM_A_TRANS4_ADDR_1);
}

static inline int write_replace_cq_baseaddr(const struct mtk_raw_device *raw,
										void __iomem **base, u32 *offset)
{
	u32 _off = *offset;

	void *_base;
	u32 _offset;

	if (CAM_DEBUG_ENABLED(QOF_ADDR)) {
		_base = *base;
		_offset = *offset;
	}

#define CHANGE_TO(to) {*offset = to; break;}

	if (*base == raw->base) {
		switch (_off) {
		case REG_CAMCQ_CQ_THR0_BASEADDR:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ1_DATA_1)
		case REG_CAMCQ_CQ_THR0_BASEADDR_MSB:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ2_DATA_1)
		case REG_CAMCQ_CQ_THR0_DESC_SIZE:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ3_DATA_1)
		case REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ4_DATA_1)
		case REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2_MSB:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ5_DATA_1)
		case REG_CAMCQ_CQ_SUB_THR0_DESC_SIZE_2:
			CHANGE_TO(REG_QOF_CAM_A_QOF_CQ6_DATA_1)
		default:
			return 0;
		}
	} else
		return 0;

#undef CHANGE_TO

	*base = raw->qof_base;

	if (CAM_DEBUG_ENABLED(QOF_ADDR))
		dev_info(raw->dev, "%s: {%p, 0x%08x} -> {%p, 0x%08x}",
				__func__,
				_base, _offset,
				*base, *offset);

	return 1;
}

static inline int write_replace_trigger_cq(const struct mtk_raw_device *raw,
										   void __iomem **base, u32 *offset,
										   u32 *val)
{
	if (*base == raw->base && *offset == REG_CAMCTL_START) {
		void *_base;
		u32 _offset;

		if (CAM_DEBUG_ENABLED(QOF_ADDR)) {
			_base = *base;
			_offset = *offset;
		}

		// TODO: "QOF_CTRL" reg synchonization
		*base = raw->qof_base;
		*offset = REG_QOF_CAM_A_QOF_CTL_1;

		*val = readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);
		SET_FIELD(val, QOF_CAM_A_CQ_START_1, 1);

		if (CAM_DEBUG_ENABLED(QOF_ADDR))
			dev_info(raw->dev, "%s: {%p, 0x%08x} -> {%p, 0x%08x} val 0x%x",
					__func__,
					_base, _offset,
					*base, *offset,
					 *val);

		return 1;
	}

	return 0;
}

static u32 qof_readl(struct mtk_raw_device *raw,
					 void __iomem *base, u32 offset)
{
	int ret =
		read_replace_itc_addr(raw, &base, &offset) ||
		read_replace_qof_cq_addr(raw, &base, &offset) ||
		read_replace_rtc_addr(raw, &base, &offset);

	(void)ret;

	if (CAM_DEBUG_ENABLED(QOF_ADDR))
		dev_info(raw->dev, "%s: {%p, 0x%08x}", __func__, base, offset);

	return readl(base + offset);
}

static u32 qof_readl_relaxed(struct mtk_raw_device *raw,
							 void __iomem *base, u32 offset)
{
	int ret =
		read_replace_itc_addr(raw, &base, &offset) ||
		read_replace_qof_cq_addr(raw, &base, &offset) ||
		read_replace_rtc_addr(raw, &base, &offset);

	(void)ret;

	if (CAM_DEBUG_ENABLED(QOF_ADDR))
		dev_info(raw->dev, "%s: {%p, 0x%08x}", __func__, base, offset);

	return readl_relaxed(base + offset);
}

static void qof_writel(struct mtk_raw_device *raw, u32 val,
					   void __iomem *base, u32 offset)
{
	int ret = 0;
	int ret_trigger_cq = 0;

	if (!raw->trigger_cq_by_qof)
		goto OUT;

	ret = write_replace_cq_baseaddr(raw, &base, &offset);

	if (!ret)
		ret_trigger_cq = write_replace_trigger_cq(raw, &base, &offset, &val);

	if (ret_trigger_cq) {
		if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id))
			dev_info(raw->dev, "%s: trigger CQ by QOF", __func__);
#ifdef WORKAROUND_VOTER_SET_OUTSIDE_OFF_PROC
		// NOTE: workaround for HW issue
		avoid_power_state(raw, PS_OFF_PROC);
#endif
	}

OUT:
	writel(val, base + offset);
}

static void qof_writel_relaxed(struct mtk_raw_device *raw, u32 val,
							   void __iomem *base, u32 offset)
{
	int ret = 0;
	int ret_trigger_cq = 0;

	if (!raw->trigger_cq_by_qof)
		goto OUT;

	ret = write_replace_cq_baseaddr(raw, &base, &offset);

	if (!ret)
		ret_trigger_cq = write_replace_trigger_cq(raw, &base, &offset, &val);

	if (ret_trigger_cq) {
		if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id))
			dev_info(raw->dev, "%s: trigger CQ by QOF", __func__);
#ifdef WORKAROUND_VOTER_SET_OUTSIDE_OFF_PROC
		// NOTE: workaround for HW issue
		avoid_power_state(raw, PS_OFF_PROC);
#endif
	}

OUT:
	writel_relaxed(val, base + offset);
}

static u32 itc_readl(struct mtk_raw_device *raw,
					 void __iomem *base, u32 offset)
{
	read_replace_itc_addr(raw, &base, &offset);

	if (CAM_DEBUG_ENABLED(QOF_ADDR))
		dev_info(raw->dev, "%s: {%p, 0x%08x}", __func__, base, offset);

	return readl(base + offset);
}

static u32 itc_readl_relaxed(struct mtk_raw_device *raw,
					 void __iomem *base, u32 offset)
{
	read_replace_itc_addr(raw, &base, &offset);

	if (CAM_DEBUG_ENABLED(QOF_ADDR))
		dev_info(raw->dev, "%s: {%p, 0x%08x}", __func__, base, offset);

	return readl_relaxed(base + offset);
}

void qof_dump_trigger_cnt(struct mtk_raw_device *raw)
{
	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id)) {
		unsigned int qof_trig_cnt =
			readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_TRIG_CNT_1);

		dev_info(raw->dev, "qof: on_cnf: %d off_cnt: %d",
				qof_trig_cnt & 0xFF, (qof_trig_cnt >> 16) & 0xFF);
	}
}

u32 qof_on_off_cnt(struct mtk_raw_device *raw)
{
	return readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_TRIG_CNT_1);
}

void qof_dump_voter(struct mtk_raw_device *raw)
{
	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id)) {
		unsigned int voter_dbg =
			readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_VOTER_DBG_1);
		unsigned int voter_history =
			READ_FIELD(voter_dbg, QOF_CAM_A_VOTE_CHECK_1);

		dev_info(raw->dev, "qof: voter overall: 0x%x (on=0x2, off=0x1)",
			READ_FIELD(voter_dbg, QOF_CAM_A_TRG_STATUS_1));
		dev_info(raw->dev, "qof: cur status: 0x%x (CQ/RAW/SCP/APMCU=0x8/0x4/0x2/0x1)",
			READ_FIELD(voter_dbg, QOF_CAM_A_VOTE_1));
		dev_info(raw->dev, "qof: history: 0x%x 0x%x 0x%x 0x%x",
			voter_history & 0xf,
			voter_history >> 4 & 0xf,
			voter_history >> 8 & 0xf,
			voter_history >> 12 & 0xf);
	}
}

void qof_dump_power_state(struct mtk_raw_device *raw)
{
	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id)) {
		unsigned int state_dbg =
			readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_STATE_DBG_1);
		unsigned int mtcmos_state_raw_lsb =
			readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_MTC_ST_RAW_LSB_1);
		unsigned int mtcmos_state_raw_msb =
			readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_MTC_ST_RAW_MSB2_1);

		dev_info(raw->dev, "qof: state dbg: 0x%08x", state_dbg);
		dev_info(raw->dev, "qof: mtcmos status msb/lsb: 0x%08x %08x",
			mtcmos_state_raw_msb, mtcmos_state_raw_lsb);
	}
}

void qof_dump_hw_timer(struct mtk_raw_device *raw)
{
	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id)) {
		dev_info(raw->dev, "qof: %s: TIME_STAMP_1 %u MTC_CYC_MAX %u PWR_OFF_MAX %u",
				__func__,
				readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_TIME_STAMP_1),
				readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_MTC_CYC_MAX_1),
				readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_PWR_OFF_MAX_1));
	}
}

void qof_dump_cq_addr(struct mtk_raw_device *raw)
{
	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id)) {
		dev_info(raw->dev, "qof: %s: QOF_CQ1/2/3/4/5/6_ADDR_1 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
				__func__,
				readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ1_ADDR_1),
				 readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ2_ADDR_1),
				 readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ3_ADDR_1),
				 readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ4_ADDR_1),
				 readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ5_ADDR_1),
				 readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ6_ADDR_1));
		dev_info(raw->dev, "qof: %s: QOF_CQ1/2/3/4/5/6_DATA_1 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
				__func__,
				readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ1_DATA_1),
				 readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ2_DATA_1),
				 readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ3_DATA_1),
				 readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ4_DATA_1),
				 readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ5_DATA_1),
				 readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CQ6_DATA_1));
		dev_info(raw->dev, "qof: %s: RAW (out)CQ1/2/3/4/5/6_DATA_1 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
				__func__,
				readl_relaxed(raw->base + REG_CAMCQ_CQ_THR0_BASEADDR),
				 readl_relaxed(raw->base + REG_CAMCQ_CQ_THR0_BASEADDR_MSB),
				 readl_relaxed(raw->base + REG_CAMCQ_CQ_THR0_DESC_SIZE),
				 readl_relaxed(raw->base + REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2),
				 readl_relaxed(raw->base + REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2_MSB),
				 readl_relaxed(raw->base + REG_CAMCQ_CQ_SUB_THR0_DESC_SIZE_2));
		dev_info(raw->dev, "qof: %s: RAW (in)CQ1/2/3/4/5/6_DATA_1 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
				__func__,
				readl_relaxed(raw->base_inner + REG_CAMCQ_CQ_THR0_BASEADDR),
				 readl_relaxed(raw->base_inner + REG_CAMCQ_CQ_THR0_BASEADDR_MSB),
				 readl_relaxed(raw->base_inner + REG_CAMCQ_CQ_THR0_DESC_SIZE),
				 readl_relaxed(raw->base_inner + REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2),
				 readl_relaxed(raw->base_inner + REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2_MSB),
				 readl_relaxed(raw->base_inner + REG_CAMCQ_CQ_SUB_THR0_DESC_SIZE_2));
	}
}

void qof_dump_ctrl(struct mtk_raw_device *raw)
{
	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id)) {
		dev_info(raw->dev, "qof: %s: TOP_CTL 0x%08x", __func__,
				 readl(raw->cam->qoftop_base + REG_QOF_CAM_TOP_QOF_TOP_CTL));
		dev_info(raw->dev, "qof: %s: 0x%08x",
				__func__, readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1));
	}
}

void qof_dump_qoftop_status(struct mtk_raw_device *raw)
{
	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id)) {
		dev_info(raw->dev, "qof: %s: QOF_CAM_TOP_ITC_STATUS 0x%08x", __func__,
			 readl(raw->cam->qoftop_base + REG_QOF_CAM_TOP_ITC_STATUS));
		dev_info(raw->dev, "qof: %s: QOF_CAM_A_QOF_DONE_STATUS_1 0x%08x", __func__,
			 readl(raw->cam->qoftop_base + REG_QOF_CAM_A_QOF_DONE_STATUS_1));
		dev_info(raw->dev, "qof: %s: QOF_CAM_TOP_QOF_INT_STATUS 0x%08x", __func__,
			 readl(raw->cam->qoftop_base + REG_QOF_CAM_TOP_QOF_INT_STATUS));
	}
}

void qof_set_force_dump(struct mtk_raw_device *raw, bool en)
{
	if (en)
		force_dump_raw_map |= BIT(raw->id);
	else
		force_dump_raw_map &= ~BIT(raw->id);
}

void qof_force_dump_all(struct mtk_raw_device *raw)
{
	qof_set_force_dump(raw, true);

	qof_dump_qoftop_status(raw);
	qof_dump_ctrl(raw);
	qof_dump_cq_addr(raw);
	qof_dump_hw_timer(raw);
	qof_dump_power_state(raw);
	qof_dump_voter(raw);
	qof_dump_trigger_cnt(raw);

	dev_info(raw->dev, "[%s] QOF CQ_START_MAX_1:0x%08x\n",
				 __func__, readl(raw->qof_base + REG_QOF_CAM_A_QOF_CQ_START_MAX_1));

	qof_set_force_dump(raw, false);

}

#define DDR_GEN_BEFORE_US     10
#define QOS_GEN_BEFORE_US     100
void qof_ddren_setting(struct mtk_raw_device *raw, int frm_time_us)
{
	int ddr_gen_pulse, qos_gen_pulse;
	int val;
	unsigned long flags;

	ddr_gen_pulse =
		(frm_time_us - DDR_GEN_BEFORE_US) * SCQ_DEFAULT_CLK_RATE /
		(2 * (QOF_TIMER_FREQ_DIV + 1)) - 1;
	qos_gen_pulse =
		(frm_time_us - QOS_GEN_BEFORE_US) * SCQ_DEFAULT_CLK_RATE /
		(2 * (QOF_TIMER_FREQ_DIV + 1)) - 1;

	spin_lock_irqsave(&raw->qof_ctrl_lock, flags);

	val = readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);
	writel_relaxed(
		val | FBIT(QOF_CAM_A_DDREN_HW_EN_1) | FBIT(QOF_CAM_A_BW_QOS_HW_EN_1),
		raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1);

	spin_unlock_irqrestore(&raw->qof_ctrl_lock, flags);

	writel_relaxed(ddr_gen_pulse, raw->qof_base + REG_QOF_CAM_A_QOF_DDREN_CYC_MAX_1);
	writel_relaxed(qos_gen_pulse, raw->qof_base + REG_QOF_CAM_A_QOF_BWQOS_CYC_MAX_1);

	if (CAM_DEBUG_ENABLED(QOF) || FORCE_DUMP(raw->id))
		pr_info("qof: %s: frm_time_us:%d, qof_ctrl:0x%x time_stamp:0x%x ddren_cyc_max:0x%x qos_cyc_max:0x%x\n",
			__func__, frm_time_us,
			readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_CTL_1),
			readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_TIME_STAMP_1),
			readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_DDREN_CYC_MAX_1),
			readl_relaxed(raw->qof_base + REG_QOF_CAM_A_QOF_BWQOS_CYC_MAX_1));
}
