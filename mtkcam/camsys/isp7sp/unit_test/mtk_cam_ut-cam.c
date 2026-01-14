// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/timekeeping.h>

#include "mtk_cam_ut.h"
#include "mtk_cam_ut-engines.h"

#include "mtk_cam_regs.h"

#define DUMP_FBC_AT_SOF		BIT(0) /* at SOF */
#define DUMP_FBC_AT_HW_DONE	BIT(1) /* at HW_PASS1_DONE */
#define DUMP_STX		BIT(2) /* at SW_PASS1_DONE */

#ifdef dev_dbg
#undef dev_dbg
#define dev_dbg dev_info
#endif

const struct mtk_cam_ut_data *cur_platform;

static unsigned int dump_cmd;
module_param(dump_cmd, uint, 0644);
MODULE_PARM_DESC(dump_cmd, "dump cmd");

struct ut_raw_status {
	/* raw INT1~7 */
	u32 irq;
	u32 wdma;
	u32 rdma;
	u32 drop;
	u32 ofl;
	u32 cq_done;
	u32 dcif;
};

struct ut_debug_cmd {
	union {
		struct {
			u8	dump_stx : 1;
			u8	dump_fbc : 1;
			u8	dump_dma_err : 1;
			u8	dump_tg_err : 1;
			u8	dump_tg_overrun : 1;
			u8	rsv_1 : 1;
			u8	rsv_2 : 1;
			u8	rsv_3 : 1;
		};
		u64	any_debug;
	};
};

struct ut_raw_msg {
	u64 ts_ns;
	struct ut_event		event;
	struct ut_debug_cmd	cmd;
};

#define FIFO_THRESHOLD(FIFO_SIZE, HEIGHT_RATIO, LOW_RATIO) \
	(((FIFO_SIZE * HEIGHT_RATIO) & 0xFFF) << 16 | \
	((FIFO_SIZE * LOW_RATIO) & 0xFFF))

void raw_disable_tg_vseol_sub_ctl(struct device *dev)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	u32 val;

	val = 0x1;
	writel(val, CAM_REG_TG_VSEOL_SUB_CTL(raw->base));
	dev_info(dev, "%s: [workaround for ut] set VSEOL_SUB_CTL to 0x%08x\n",
		 __func__, val);
}

static int ut_raw_reset(struct device *dev)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	void __iomem *base = raw->base;
	void __iomem *yuv_base = raw->yuv_base;
	u32 ctl;

	writel(0x00003fff, base + CAM_REG_CTL_RAW_MOD5_DCM_DIS);
	writel(0x001fffff, base + CAM_REG_CTL_RAW_MOD6_DCM_DIS);
	writel(0x007fffff, yuv_base + CAM_REG_CTL_RAW_MOD5_DCM_DIS);

	/* Disable all DMA DCM before reset */
	wmb(); /* TBC */
	writel(0, base + REG_CTL_SW_CTL);
	writel(1, base + REG_CTL_SW_CTL);

	if (!readl_poll_timeout_atomic(base + REG_CTL_SW_CTL, ctl,
				       ctl & 0x2, 1, 1000)) {
		writel(4, base + REG_CTL_SW_CTL);
		writel(0, base + REG_CTL_SW_CTL);
		dev_info(dev, "reset success\n");
	} else {
		dev_info(dev, "Reset hw timeout: SW_CTL 0x%x\n",
			 readl_relaxed(base + REG_CTL_SW_CTL));
	}

	writel(0x0, base + CAM_REG_CTL_RAW_MOD5_DCM_DIS);
	writel(0x0, base + CAM_REG_CTL_RAW_MOD6_DCM_DIS);
	writel(0x0, yuv_base + CAM_REG_CTL_RAW_MOD5_DCM_DIS);
/*
	writel_relaxed(0x0, base + REG_CTL_SW_PASS1_DONE);
	writel_relaxed(0x0, raw->base_inner + REG_CTL_SW_PASS1_DONE);
*/
	/* make sure reset take effect */
	wmb();

	return -1;
}

static void set_steamon_handle(struct device *dev, int type);
static void raw_set_topdebug_rdyreq(struct mtk_ut_raw_device *raw, u32 event);
static void raw_set_camctl_toggle_db(struct mtk_ut_raw_device *raw);

static int ut_raw_initialize(struct device *dev, void *ext_params)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = raw->ut;
	struct mtk_ut_raw_initial_params *p = ext_params;
	void __iomem *base = raw->base;
	void __iomem *yuv_base = raw->yuv_base;
	unsigned int reg_raw_urgent, reg_yuv_urgent;
	unsigned int raw_urgent, yuv_urgent;
	u32 val;

	if (!p)
		return -1;

	/* initialize for CQ */
	if (p->subsample) {
		val = readl_relaxed(base + REG_CQ_EN);
		writel_relaxed(val | SCQ_EN | SCQ_SUBSAMPLE_EN, base + REG_CQ_EN);

		writel_relaxed(0x100 | p->subsample, base + REG_CTL_SW_PASS1_DONE);
		writel_relaxed(0x100 | p->subsample, raw->base_inner + REG_CTL_SW_PASS1_DONE);
	} else {
		val = readl_relaxed(base + REG_CQ_EN);
		writel_relaxed(val | SCQ_EN, base + REG_CQ_EN);
	}

	writel_relaxed(CQ_THR0_MODE_IMMEDIATE | CQ_THR0_EN,
		       base + REG_CQ_THR0_CTL);
	writel_relaxed(CQ_THR0_MODE_IMMEDIATE | CQ_THR0_EN,
		       base + REG_CQ_SUB_THR0_CTL);

	writel_relaxed(0xffffffff, base + REG_SCQ_START_PERIOD);
	writel_relaxed(0xffffffff, raw->base_inner + REG_SCQ_START_PERIOD);

	writel_relaxed((0x10 << 24) | 64, base + REG_CQI_R1A_CON0);
	writel_relaxed((0x1 << 28) | FIFO_THRESHOLD(64, 2/10, 1/10),
			base + REG_CQI_R1A_CON1);
	writel_relaxed((0x1 << 28) | FIFO_THRESHOLD(64, 4/10, 3/10),
			base + REG_CQI_R1A_CON2);
	writel_relaxed((0x1 << 31) | FIFO_THRESHOLD(64, 6/10, 5/10),
			base + REG_CQI_R1A_CON3);
	writel_relaxed((0x1 << 31) | FIFO_THRESHOLD(64, 1/10, 0),
			base + REG_CQI_R1A_CON4);

	writel_relaxed(HALT1_EN, ut->base + REG_HALT1_EN);
	writel_relaxed(HALT2_EN, ut->base + REG_HALT2_EN);
	writel_relaxed(HALT13_EN, ut->base + REG_HALT13_EN);

	//Disable low latency
	writel_relaxed(0xffff,
		base + REG_CAMRAWDMATOP_LOW_LATENCY_LINE_CNT_IMGO_R1);
	writel_relaxed(0xffff,
		yuv_base + REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_YUVO_R1);
	writel_relaxed(0xffff,
		yuv_base + REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_YUVO_R3);
	writel_relaxed(0xffff,
		yuv_base + REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_DRZS4NO_R1);

	switch (raw->id) {
	case RAW_B:
		reg_raw_urgent = REG_HALT7_EN;
		reg_yuv_urgent = REG_HALT8_EN;
		raw_urgent = HALT7_EN;
		yuv_urgent = HALT8_EN;
		break;
	case RAW_C:
		reg_raw_urgent = REG_HALT9_EN;
		reg_yuv_urgent = REG_HALT10_EN;
		raw_urgent = HALT9_EN;
		yuv_urgent = HALT10_EN;
		break;
	case RAW_A:
		fallthrough;
	default:
		reg_raw_urgent = REG_HALT5_EN;
		reg_yuv_urgent = REG_HALT6_EN;
		raw_urgent = HALT5_EN;
		yuv_urgent = HALT6_EN;
		dev_info(dev, "%s: raw id %d\n", __func__, raw->id);
		break;
	}

	if (is_raw_sel_from_rawi(p->hardware_scenario)) {
		writel_relaxed(0x0, ut->base + reg_raw_urgent);
		writel_relaxed(0x0, ut->base + reg_yuv_urgent);

		dev_info(dev, "%s: is srt, raw 0x%x.\n",
			__func__, readl_relaxed(ut->base + reg_raw_urgent));
	} else {
		writel_relaxed(raw_urgent, ut->base + reg_raw_urgent);
		writel_relaxed(yuv_urgent, ut->base + reg_yuv_urgent);

		dev_info(dev, "%s: is hrt, raw 0x%x.\n",
			__func__, readl_relaxed(ut->base + reg_raw_urgent));
	}
	wmb();

	set_steamon_handle(dev, p->streamon_type);

	raw->is_subsample = p->subsample;
	raw->hardware_scenario = p->hardware_scenario;
	raw->is_initial_cq = 1;
	raw->cq_done_mask = 0;

	/* Workaround: disable AAO/AAHO error: double sof error for smvr
	 *  HW would send double sof to aao/aaho in subsample mode
	 *  disable it to bypass
	 */
	writel_relaxed(0xFFFE0000,
		       base + REG_AAO_R1_ERR_STAT);
	writel_relaxed(0xFFFE0000,
		       base + REG_AAHO_R1_ERR_STAT);

	return 0;
}

static bool not_support_rwfbc(void)
{
	if (strncmp("mt6897", cur_platform->platform, sizeof("mt6897")) == 0)
		return true;
	else
		return false;
}

static void rwfbc_inc_setup(struct device *dev)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	void __iomem *base = raw->base;
	void __iomem *yuv_base = raw->yuv_base;
	u32 wfbc_en_raw, wfbc_en_yuv;

	if (not_support_rwfbc()) {
		dev_info(dev, "[%s] platform:%s bypass using RWFBC\n",
			__func__, cur_platform->platform);
	} else {
		wfbc_en_raw = readl_relaxed(base + CAM_REG_CTL_WFBC_EN);
		writel_relaxed(wfbc_en_raw, base + CAM_REG_CTL_WFBC_INC);
		wfbc_en_yuv = readl_relaxed(yuv_base + CAM_REG_CTL_WFBC_EN);
		writel_relaxed(wfbc_en_yuv, yuv_base + CAM_REG_CTL_WFBC_INC);
		dev_info(dev, "[%s] platform:%s WFBC_INC, camctl/camctl2:0x%x/0x%x\n",
			__func__, cur_platform->platform, wfbc_en_raw, wfbc_en_yuv);
	}
}

static int ut_raw_s_stream(struct device *dev, enum streaming_enum on)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	void __iomem *base = raw->base;
	u32 val;

	dev_info(dev, "%s: %d\n", __func__, on);

	if (on == streaming_active) {
		(void)base;
		(void)val;
		raw_set_camctl_toggle_db(raw); /* toggle db first. */
		raw_set_topdebug_rdyreq(raw, ALL_THE_TIME);
		rwfbc_inc_setup(dev);

	} else if (on == streaming_vf) {
		raw_set_topdebug_rdyreq(raw, TG_OVERRUN);

		val = readl_relaxed(base + REG_TG_VF_CON);
		val |= TG_VFDATA_EN;
		writel_relaxed(val, base + REG_TG_VF_CON);
		/* make sure VF enable setting take effect */
		wmb();
	} else {
		val = readl_relaxed(base + REG_TG_VF_CON);
		val &= ~TG_VFDATA_EN;
		writel_relaxed(val, base + REG_TG_VF_CON);
	}

	return 0;
}

static int ut_raw_trigger_rawi_r2(struct device *dev, enum streaming_enum on)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	void __iomem *base = raw->base;

	if (on == streaming_vf) {
		raw_set_camctl_toggle_db(raw);
		raw_set_topdebug_rdyreq(raw, ALL_THE_TIME);
		rwfbc_inc_setup(dev);

		writel(0x1, base + REG_CTL_RAWI_TRIG);
		wmb(); /* TBC */
	} else if (on == streaming_active) {
		raw_set_camctl_toggle_db(raw);
		raw_set_topdebug_rdyreq(raw, ALL_THE_TIME);
		rwfbc_inc_setup(dev);
	}

	dev_info(raw->dev, "%s: on %d\n", __func__, on);
	return 0;
}

static int ut_raw_trigger_rawi_r3(struct device *dev, enum streaming_enum on)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	void __iomem *base = raw->base;

	dev_info(dev, "[%s] ut_raw_trigger_rawi\n", __func__);

	if (on == streaming_vf) {
		raw_set_camctl_toggle_db(raw);
		raw_set_topdebug_rdyreq(raw, ALL_THE_TIME);
		rwfbc_inc_setup(dev);

		writel_relaxed(0x2, base + REG_CTL_RAWI_TRIG);
		wmb(); /* TBC */
	} else if (on == streaming_active) {
		raw_set_camctl_toggle_db(raw);
		raw_set_topdebug_rdyreq(raw, ALL_THE_TIME);
		rwfbc_inc_setup(dev);
	}
	return 0;
}

static int ut_raw_trigger_rawi_r5(struct device *dev, enum streaming_enum on)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	void __iomem *base = raw->base;

	dev_info(dev, "[%s] ut_raw_trigger_rawi\n", __func__);

	if (on == streaming_vf) {
		raw_set_camctl_toggle_db(raw);
		raw_set_topdebug_rdyreq(raw, ALL_THE_TIME);
		rwfbc_inc_setup(dev);

		writel_relaxed(0x8, base + REG_CTL_RAWI_TRIG);
		wmb(); /* TBC */
	} else if (on == streaming_active) {
		raw_set_camctl_toggle_db(raw);
		raw_set_topdebug_rdyreq(raw, ALL_THE_TIME);
		rwfbc_inc_setup(dev);
	}

	return 0;
}

static int ut_raw_trigger_adlrd(struct device *dev, enum streaming_enum on)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	void __iomem *base = raw->base;

	if (on == streaming_vf) {
		raw_set_camctl_toggle_db(raw);
		raw_set_topdebug_rdyreq(raw, ALL_THE_TIME);
		rwfbc_inc_setup(dev);

		writel_relaxed(0x1100, base + REG_CTL_RAWI_TRIG);
		wmb(); /* TBC */
	} else if (on == streaming_active) {
		raw_set_camctl_toggle_db(raw);
		raw_set_topdebug_rdyreq(raw, ALL_THE_TIME);
		rwfbc_inc_setup(dev);
	}

	return 0;
}

static void set_steamon_handle(struct device *dev, int type)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);

	dev_info(raw->dev, "streamon type %d\n", type);

	if (type == STREAM_FROM_TG)
		raw->ops.s_stream = ut_raw_s_stream;
	else if (type == STREAM_FROM_RAWI_R2)
		raw->ops.s_stream = ut_raw_trigger_rawi_r2;
	else if (type == STREAM_FROM_RAWI_R3)
		raw->ops.s_stream = ut_raw_trigger_rawi_r3;
	else if (type == STREAM_FROM_RAWI_R5)
		raw->ops.s_stream = ut_raw_trigger_rawi_r5;
	else if (type == STREAM_FROM_ADLRD)
		raw->ops.s_stream = ut_raw_trigger_adlrd;
	else
		dev_info(raw->dev, "type %d not supported yet\n", type);
}

static int ut_raw_apply_cq(struct device *dev,
			    dma_addr_t cq_addr, unsigned int cq_size, unsigned int cq_offset,
			    unsigned int sub_cq_size, unsigned int sub_cq_offset)
{
#define CQ_VADDR_MASK 0xffffffff
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	void __iomem *base = raw->base;
	u32 cq_addr_lsb = (cq_addr + cq_offset) & CQ_VADDR_MASK;
	u32 cq_addr_msb = ((cq_addr + cq_offset) >> 32);

	dev_info(dev, "[%s] cq baseaddr = 0x%llx, cq size = 0x%x, cq offset = 0x%x, sub_cq size = 0x%x, , sub_cq_offset = 0x%x\n",
		__func__, cq_addr, cq_size, cq_offset, sub_cq_size, sub_cq_offset);
	dev_info(dev, "cq_addr_lsb: 0x%x cq_addr_msb: 0x%x\n", cq_addr_lsb, cq_addr_msb);


	writel_relaxed(cq_addr_lsb, base + REG_CQ_THR0_BASEADDR);
	writel_relaxed(cq_addr_msb, base + REG_CQ_THR0_BASEADDR_MSB);
	writel_relaxed(cq_size, base + REG_CQ_THR0_DESC_SIZE);

	cq_addr_lsb = (cq_addr + sub_cq_offset) & CQ_VADDR_MASK;
	cq_addr_msb = ((cq_addr + sub_cq_offset) >> 32);
	dev_info(dev, "sub cq_addr_lsb: 0x%x cq_addr_msb: 0x%x\n", cq_addr_lsb, cq_addr_msb);

	writel_relaxed(cq_addr_lsb,
		       base + REG_CQ_SUB_THR0_BASEADDR_2);
	writel_relaxed(cq_addr_msb,
		       base + REG_CQ_SUB_THR0_BASEADDR_MSB_2);
	writel_relaxed(sub_cq_size,
		       base + REG_CQ_SUB_THR0_DESC_SIZE_2);

	writel(CTL_CQ_THR0_START, base + REG_CTL_START);
	/* make sure reset take effect */

	wmb();/*TBC*/

	return 0;
}

static void ut_raw_set_ops(struct device *dev)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);

	raw->ops.reset = ut_raw_reset;
	raw->ops.s_stream = ut_raw_s_stream;
	raw->ops.apply_cq = ut_raw_apply_cq;
	raw->ops.initialize = ut_raw_initialize;
}

static int mtk_ut_raw_component_bind(struct device *dev,
				     struct device *master, void *data)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = data;
	struct ut_event evt;

	if (!data) {
		dev_info(dev, "no master data\n");
		return -1;
	}

	if (!ut->raw) {
		dev_info(dev, "no raw arr, num of raw %d\n", ut->num_raw);
		return -1;
	}
	ut->raw[raw->id] = dev;
	raw->ut = ut;

	evt.mask = EVENT_SOF | EVENT_CQ_DONE | EVENT_SW_P1_DONE | EVENT_CQ_MAIN_TRIG_DLY;
	if (add_listener(&raw->event_src, &ut->listener, evt)) {
		dev_info(dev, "add listener fail!\n");
		return -1;
	}
	return 0;
}

static void mtk_ut_raw_component_unbind(struct device *dev,
					struct device *master, void *data)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = data;

	//dev_dbg(dev, "%s\n", __func__);
	ut->raw[raw->id] = NULL;
	if (remove_listener(&raw->event_src, &ut->listener))
		dev_dbg(dev, "%s remove listener fail!\n", __func__);
}

static const struct component_ops mtk_ut_raw_component_ops = {
	.bind = mtk_ut_raw_component_bind,
	.unbind = mtk_ut_raw_component_unbind,
};

static void raw_handle_tg_grab_err(struct mtk_ut_raw_device *raw)
{
	void __iomem *base = raw->base;
	int val;

	val = readl_relaxed(CAM_REG_TG_PATH_CFG(base));
	val = val | TG_TG_FULL_SEL;
	writel_relaxed(val, CAM_REG_TG_PATH_CFG(base));
	/* make sure all the TG setting take effect */
	wmb();
	val = readl_relaxed(CAM_REG_TG_SEN_MODE(base));
	val = val | TG_CMOS_RDY_SEL;
	writel_relaxed(val, CAM_REG_TG_SEN_MODE(base));
	/* make sure all the TG setting take effect */
	wmb();
	dev_dbg(raw->dev,
		"TG_PATH_CFG:0x%x, TG_SEN_MODE:0x%x TG_FRMSIZE_ST:0x%8x, TG_FRMSIZE_ST_R:0x%8x TG_SEN_GRAB_PXL:0x%8x, TG_SEN_GRAB_LIN:0x%8x\n",
		readl_relaxed(CAM_REG_TG_PATH_CFG(base)),
		readl_relaxed(CAM_REG_TG_SEN_MODE(base)),
		readl_relaxed(CAM_REG_TG_FRMSIZE_ST(base)),
		readl_relaxed(CAM_REG_TG_FRMSIZE_ST_R(base)),
		readl_relaxed(CAM_REG_TG_SEN_GRAB_PXL(base)),
		readl_relaxed(CAM_REG_TG_SEN_GRAB_LIN(base)));
}


static struct dma_debug_item dbg_TSFSO_R1[] = {
	{0x0000001F, "32(hex) 0000"},
	{0x0000011F, "state_checksum"},
	{0x0000021F, "line_pix_cnt_tmp"},
	{0x0000031F, "line_pix_cnt"},
	{0x0000041F, "important_status"},
	{0x0000051F, "cmd_data_cnt"},
	{0x0000061F, "cmd_cnt_for_bvalid_phase"},
	{0x0000071F, "input_h_cnt"},
	{0x0000081F, "input_v_cnt"},
	{0x0000091F, "xfer_y_cnt"},
};

static struct dma_debug_item dbg_data_3[] = {
	{0x00003000, "FUS_R1"},
	{0x00003001, "BPS_R1"},
	{0x00003002, "CAC_R1"},
	{0x00003003, "DGN_R1"},
	{0x00003004, "LSC_R1"},
	{0x00003005, "WB_R1"},
	{0x00003006, "HLR_R1"},
	{0x00003007, "LTM_R1"},
	{0x00003008, "QBN_R4"},
	{0x00003009, "QBN_R5"},
	{0x0000300a, "MRG_R4"},
	{0x0000300b, "LTMS_R1"},
	{0x0000300c, "CRP_R1"},
	{0x0000300d, "QBN_R6"},
	{0x0000300e, "AF_R1"},
	{0x0000300f, "CRP_R6"},
	{0x00003010, "QBN_R1"},
	{0x00003011, "AA_R1"},
	{0x00003012, "CRP_R7"},
	{0x00003013, "QBN_R2"},
	{0x00003014, "TSFS_R1"},
};

void mtk_ut_dump_dma_debug(struct device *dev,
		void __iomem *top_base,
		const char *dma_name,
		struct dma_debug_item *items, int n)
{
#define MAX_DEBUG_SIZE (32)
	void __iomem *dbg_sel = top_base  + REG_DMA_DBG_SEL;
	void __iomem *dbg_port = top_base + REG_DMA_DBG_PORT;
	int i = 0;
	unsigned int vals[MAX_DEBUG_SIZE];

	if (n >= MAX_DEBUG_SIZE) {
		dev_info(dev, "%s: should enlarge array size for n(%d)\n",
			__func__, n);
		return;
	}
	for (i = 0; i < n; i++) {
		writel(items[i].debug_sel, dbg_sel);
		if (readl(dbg_sel) != items[i].debug_sel)
			dev_info(dev, "failed to write dbg_sel %08x\n",
				items[i].debug_sel);
		vals[i] = readl(dbg_port);
	};

	dev_info(dev, "%s: %s\n", __func__, dma_name);
	for (i = 0; i < n; i++) {
		dev_info(dev, "%08x: %08x [%s]\n",
			items[i].debug_sel, vals[i], items[i].msg);
	}
}

void mtk_ut_dump_module_dbg_data(struct device *dev,
		void __iomem *top_base, const char *module_name,
		struct dma_debug_item item)
{
	void __iomem *dbg_set = top_base + REG_CTL_DBG_SET;
	void __iomem *dbg_port = top_base + REG_CTL_DBG_PORT;
	unsigned int val;

	writel(item.debug_sel, dbg_set);
	if (readl(dbg_set) != item.debug_sel) {
		dev_info(dev, "failed to write dbg_sel %08x\n",
			item.debug_sel);
	}
	val = readl(dbg_port);
	dev_info(dev, "%s: %s\n", __func__, module_name);
	dev_info(dev, "%08x: %08x [%s]\n",
		item.debug_sel, val, item.msg);
}
static void raw_handle_dma_err(struct mtk_ut_raw_device *raw)
{
	void __iomem *base = raw->base;
	void __iomem *yuv_base = raw->yuv_base;

	dev_info_ratelimited(raw->dev,
			    "IMGO:%x,YUVO_R1/R2/R3/R4/R5:%x/%x/%x/%x/%x\n",
			    readl_relaxed(base + REG_IMGO_R1_ERR_STAT),
			    readl_relaxed(yuv_base + REG_YUVO_R1_ERR_STAT),
			    readl_relaxed(yuv_base + REG_YUVO_R2_ERR_STAT),
			    readl_relaxed(yuv_base + REG_YUVO_R3_ERR_STAT),
			    readl_relaxed(yuv_base + REG_YUVO_R4_ERR_STAT),
			    readl_relaxed(yuv_base + REG_YUVO_R5_ERR_STAT)
			   );

	dev_info_ratelimited(raw->dev,
			    "RZH1N2TO_R1/R2/R3:%x/%x/%x,DRZS4NO_R1/R2/R3:%x/%x/%x\n",
			    readl_relaxed(yuv_base + REG_RZH1N2TO_R1_ERR_STAT),
			    readl_relaxed(yuv_base + REG_RZH1N2TO_R2_ERR_STAT),
			    readl_relaxed(yuv_base + REG_RZH1N2TO_R3_ERR_STAT),
			    readl_relaxed(yuv_base + REG_DRZS4NO_R1_ERR_STAT),
			    readl_relaxed(yuv_base + REG_DRZS4NO_R2_ERR_STAT),
			    readl_relaxed(yuv_base + REG_DRZS4NO_R3_ERR_STAT)
			   );

	dev_info_ratelimited(raw->dev,
			    "AAO/AAHO/LTMSO/FLKO/AFO:%x/%x/%x/%x/%x,TSFSO_R1/R2:%x/%x\n",
			    readl_relaxed(base + REG_AAO_R1_ERR_STAT),
			    readl_relaxed(base + REG_AAHO_R1_ERR_STAT),
			    readl_relaxed(base + REG_LTMSO_R1_ERR_STAT),
			    readl_relaxed(base + REG_FLKO_R1_ERR_STAT),
			    readl_relaxed(base + REG_AFO_R1_ERR_STAT),
			    readl_relaxed(base + REG_TSFSO_R1_ERR_STAT),
			    readl_relaxed(base + REG_TSFSO_R2_ERR_STAT)
			   );

	dev_info_ratelimited(raw->dev,
			    "RAWI_R2/R3:%x/%x,LSCI:%x,BPCI_R1/R2/R3:%x/%x/%x\n",
			    readl_relaxed(base + REG_RAWI_R2_ERR_STAT),
			    readl_relaxed(base + REG_RAWI_R3_ERR_STAT),
			    readl_relaxed(base + REG_LSCI_R1_ERR_STAT),
			    readl_relaxed(base + REG_BPCI_R1_ERR_STAT),
			    readl_relaxed(base + REG_BPCI_R2_ERR_STAT),
			    readl_relaxed(base + REG_BPCI_R3_ERR_STAT)
			   );
}

static void raw_set_topdebug_rdyreq(struct mtk_ut_raw_device *raw, u32 event)
{
	void __iomem *base = raw->base;
	void __iomem *yuv_base = raw->yuv_base;

	u32 val = 0xa << 12;

	writel(val, base + REG_CTL_DBG_SET);
	writel(event, base + REG_CTL_DBG_SET2);
	writel(val, yuv_base + REG_CTL_DBG_SET);
	dev_info(raw->dev, "set CAMCTL_DBG_SET2/CAMCTL_DBG_SET (RAW/YUV) 0x%08x/0x%08x\n",
		event, val);
}

static void raw_set_camctl_toggle_db(struct mtk_ut_raw_device *raw)
{
	void __iomem *base = raw->base;
	unsigned int misc = readl_relaxed(base + CAM_REG_CTL_MISC);
	unsigned int misc_db_off = misc & ~0x10;

	misc |= 0x10;

	writel(misc_db_off, base + CAM_REG_CTL_MISC);
	writel(misc, base + CAM_REG_CTL_MISC);
	wmb();
}

void raw_handle_tg_overrun(struct mtk_ut_raw_device *raw)
{
	static const u32 debug_sel[] = {
		/* req group 1~7 */
		0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6,
		/* rdy group 1~7 */
		0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE,
	};
	void __iomem *dbg_set, *dbg_set2;
	void __iomem *dbg_port;
	void __iomem *base = raw->base;
	void __iomem *yuv_base = raw->yuv_base;
	u32 set;
	int i;

	/* CAMCTL_DBG_SET
	 *   CAMCTL_DEBUG_TOP_SEL [15:12]
	 *   CAMCTL_DEBUG_SEL     [11: 8]
	 *   CAMCTL_DEBUG_MOD_SEL [ 7: 0]
	 */

	/* CAMCTL_DBG_SET_2
	 *   0x1: latch all the time
	 *   0x2: latch when TG_OVERRUN
	 *   0x4: latch when CQ_MAIN_VS_ERR
	 *   0x8: latch when CQ_SUB_VS_ERR
	 *   0x10: latch when RAW dma_err
	 *   0x20: latch when YUV dma_err
	 */

	dbg_set = base + REG_CTL_DBG_SET;
	dbg_set2 = base + REG_CTL_DBG_SET2;
	dbg_port = base + REG_CTL_DBG_PORT;

	set = readl(dbg_set) & 0xfff000;
	for (i = 0; i < ARRAY_SIZE(debug_sel); i++) {
		writel(set | debug_sel[i] << 8, dbg_set);
		writel(0x2, dbg_set2);
		dev_info(raw->dev, "RAW debug_set 0x%08x port 0x%08x\n",
			 readl(dbg_set), readl(dbg_port));
	}

	dbg_set = yuv_base + REG_CTL_DBG_SET;
	dbg_set2 = yuv_base + REG_CTL_DBG_SET2;
	dbg_port = yuv_base + REG_CTL_DBG_PORT;

	set = readl(dbg_set) & 0xfff000;
	for (i = 0; i < ARRAY_SIZE(debug_sel); i++) {
		writel(set | debug_sel[i] << 8, dbg_set);
		writel(0x2, dbg_set2);
		dev_info(raw->dev, "YUV debug_set 0x%08x port 0x%08x\n",
			 readl(dbg_set), readl(dbg_port));
	}
}

static void raw_dump_stx(struct mtk_ut_raw_device *raw)
{
	void __iomem *base = raw->base;
	struct ut_raw_status statusx;

	/* raw */
	statusx.irq = readl_relaxed(CAM_REG_CTL_RAW_INT_STATUSX(base));
	statusx.wdma = readl_relaxed(CAM_REG_CTL_RAW_INT2_STATUSX(base));
	statusx.rdma = readl_relaxed(CAM_REG_CTL_RAW_INT3_STATUSX(base));
	statusx.drop = readl_relaxed(CAM_REG_CTL_RAW_INT4_STATUSX(base));
	statusx.ofl = readl_relaxed(CAM_REG_CTL_RAW_INT5_STATUSX(base));
	statusx.cq_done = readl_relaxed(CAM_REG_CTL_RAW_INT6_STATUSX(base));
	statusx.dcif = readl_relaxed(CAM_REG_CTL_RAW_INT7_STATUSX(base));

	dev_info(raw->dev,
		 "STATUSX INT1-7 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		 statusx.irq, statusx.wdma, statusx.rdma, statusx.drop,
		 statusx.ofl, statusx.cq_done, statusx.dcif);
}

static void raw_dump_fbc(struct mtk_ut_raw_device *raw)
{
	void __iomem *base = raw->base;
	void __iomem *yuvbase = raw->yuv_base;

	int fbc_ctl = 0x2c00;

	while (fbc_ctl <= 0x2c50) {
		dev_info(raw->dev, "RAW FBC(0x%08x) 0x%08x 0x%08x\n",
			 fbc_ctl,
			 readl_relaxed(base + fbc_ctl),
			 readl_relaxed(base + fbc_ctl + 4));
		fbc_ctl += 8;
	}

	fbc_ctl = 0x3780;
	while (fbc_ctl <= 0x3854) {
		dev_info(raw->dev, "YUV FBC(0x%08x) 0x%08x 0x%08x\n",
			 fbc_ctl,
			 readl_relaxed(yuvbase + fbc_ctl),
			 readl_relaxed(yuvbase + fbc_ctl + 4));
		fbc_ctl += 8;
	}
}

#define RAW_DEBUG 0
static irqreturn_t mtk_ut_raw_irq(int irq, void *data)
{

	struct mtk_ut_raw_device *raw = data;
	void __iomem *base = raw->base;
	struct ut_raw_status status;
	struct ut_raw_msg msg;
	struct ut_event *event = &msg.event;
	struct ut_debug_cmd *cmd = &msg.cmd;
	int wake_thread = 0;

	msg.ts_ns = ktime_get_boottime_ns();

	/* raw */
	status.irq = readl_relaxed(CAM_REG_CTL_RAW_INT_STATUS(base));
	status.wdma = readl_relaxed(CAM_REG_CTL_RAW_INT2_STATUS(base));
	status.rdma = readl_relaxed(CAM_REG_CTL_RAW_INT3_STATUS(base));
	status.drop = readl_relaxed(CAM_REG_CTL_RAW_INT4_STATUS(base));
	status.ofl = readl_relaxed(CAM_REG_CTL_RAW_INT5_STATUS(base));
	status.cq_done = readl_relaxed(CAM_REG_CTL_RAW_INT6_STATUS(base));
	status.dcif = readl_relaxed(CAM_REG_CTL_RAW_INT7_STATUS(base));

	event->mask = 0;
	cmd->any_debug = 0;

	if ((status.irq & SOF_INT_ST) ||
		(status.dcif & DCIF_LAST_SOF_INT_ST) ||
		(status.dcif & DCIF_LAST_CQ_START_INT_ST))
		event->mask |= EVENT_SOF;

	if (status.irq & SW_PASS1_DON_ST) {
		event->mask |= EVENT_SW_P1_DONE;

		if (dump_cmd & DUMP_STX)
			cmd->dump_stx = 1;
	}

	cmd->dump_fbc =
		((dump_cmd & DUMP_FBC_AT_SOF) && (status.irq & SOF_INT_ST)) ||
		((dump_cmd & DUMP_FBC_AT_HW_DONE) && (status.irq & HW_PASS1_DON_ST));

	if (status.cq_done & CTL_CQ_THR0_DONE_ST)
		raw->cq_done_mask |= 0x1;

	if (status.cq_done & CTL_CQ_THRSUB_DONE_ST)
		raw->cq_done_mask |= 0x2;

	if (raw->is_subsample && !raw->is_initial_cq
	    && (raw->cq_done_mask & 0x1)) {
		raw->cq_done_mask = 0;
		event->mask |= EVENT_CQ_DONE;
	} else if (raw->cq_done_mask == 0x3) {
		raw->is_initial_cq = 0;
		raw->cq_done_mask = 0;
		event->mask |= EVENT_CQ_DONE;
	}

	if (status.irq & CQ_MAIN_TRIG_DLY_ST)
		event->mask |= EVENT_CQ_MAIN_TRIG_DLY;

	if (status.irq & INT_ST_MASK_CAM_ERR) {
		dev_info(raw->dev, "int_err: 0x%lx\n",
			 status.irq & INT_ST_MASK_CAM_ERR);

		if (status.irq & DMA_ERR_ST)
			cmd->dump_dma_err = 1;

		if (status.irq & TG_GBERR_ST)
			cmd->dump_tg_err = 1;

		if (status.irq & TG_OVRUN_ST)
			cmd->dump_tg_overrun = 1;
	}

	if (raw->id != 0)
		event->mask = 0;

	if (event->mask || cmd->any_debug) {
		int len;

		if (unlikely(kfifo_avail(&raw->msgfifo) < sizeof(msg))) {
			dev_info(raw->dev, "msg fifo is full\n");
			goto nomem;
		}

		len = kfifo_in(&raw->msgfifo, &msg, sizeof(msg));
		WARN_ON(len != sizeof(msg));

		wake_thread = 1;
		dev_dbg(raw->dev, "time %lld: event %x, debug %llx\n",
			msg.ts_ns,
			event->mask, cmd->any_debug);
	}

nomem:

	dev_info(raw->dev, "INT1-7 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		 status.irq, status.wdma, status.rdma, status.drop,
		 status.ofl, status.cq_done, status.dcif);

	return wake_thread ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static irqreturn_t mtk_ut_raw_thread_irq(int irq, void *data)
{
	struct mtk_ut_raw_device *raw = data;
	struct ut_raw_msg msg;
	struct ut_event *event = &msg.event;
	struct ut_debug_cmd *cmd = &msg.cmd;

	while (kfifo_len(&raw->msgfifo) >= sizeof(msg)) {
		int len = kfifo_out(&raw->msgfifo, &msg, sizeof(msg));

		WARN_ON(len != sizeof(msg));

		dev_info(raw->dev, "time %lld\n", msg.ts_ns);

		if (cmd->dump_fbc)
			raw_dump_fbc(raw);

		if (cmd->dump_stx)
			raw_dump_stx(raw);

		if (cmd->dump_tg_err)
			raw_handle_tg_grab_err(raw);

		if (cmd->dump_dma_err) {
			raw_handle_dma_err(raw);
			mtk_ut_dump_dma_debug(raw->dev, raw->base,
				"TSFSO_R1", dbg_TSFSO_R1, 10);
			mtk_ut_dump_module_dbg_data(raw->dev, raw->base,
				"TSFS_R1", dbg_data_3[20]);
		}
		if (cmd->dump_tg_overrun)
			raw_handle_tg_overrun(raw);

		if (event->mask) {
			dev_dbg(raw->dev, "send event 0x%x\n", event->mask);
			if (send_event(&raw->event_src, *event))
				dev_info(raw->dev, "send event fail!");
		}
	}

	return IRQ_HANDLED;
}

static int mtk_ut_raw_of_probe(struct platform_device *pdev,
			    struct mtk_ut_raw_device *raw)
{
	struct device *dev = &pdev->dev;
	struct platform_device *larb_pdev;
	struct device_node *larb_node;
	struct device_link *link;
	struct resource *res;
	int irq, clks, larbs, ret;
	int i;

	ret = of_property_read_u32(dev->of_node, "mediatek,cam-id",
				   &raw->id);
	dev_info(dev, "id = %d\n", raw->id);
	if (ret) {
		dev_info(dev, "missing camid property\n");
		return ret;
	}

	/* base outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	raw->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(raw->base)) {
		dev_info(dev, "failed to map register base\n");
		return PTR_ERR(raw->base);
	}
	dev_dbg(dev, "raw, map_addr=0x%lx\n", (unsigned long)raw->base);
	/* base inner register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	raw->base_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(raw->base_inner)) {
		dev_info(dev, "failed to map register inner base\n");
		return PTR_ERR(raw->base_inner);
	}

	/* will be assigned later */
	raw->yuv_base = NULL;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_info(dev, "failed to get irq\n");
		return -ENODEV;
	}

	ret = devm_request_threaded_irq(dev, irq,
					mtk_ut_raw_irq,
					mtk_ut_raw_thread_irq,
					0, dev_name(dev), raw);
	if (ret) {
		dev_info(dev, "failed to request irq=%d\n", irq);
		return ret;
	}
	dev_dbg(dev, "registered irq=%d\n", irq);

	clks = of_count_phandle_with_args(pdev->dev.of_node,
				"clocks", "#clock-cells");

	raw->num_clks = (clks == -ENOENT) ? 0:clks;
	dev_info(dev, "clk_num:%d\n", raw->num_clks);

	if (raw->num_clks) {
		raw->clks = devm_kcalloc(dev, raw->num_clks, sizeof(*raw->clks),
					 GFP_KERNEL);
		if (!raw->clks)
			return -ENOMEM;
	}

	for (i = 0; i < raw->num_clks; i++) {
		raw->clks[i] = of_clk_get(pdev->dev.of_node, i);
		if (IS_ERR(raw->clks[i])) {
			dev_info(dev, "failed to get clk %d\n", i);
			return -ENODEV;
		}
	}

	larbs = of_count_phandle_with_args(
					pdev->dev.of_node, "mediatek,larbs", NULL);
	dev_info(dev, "larb_num:%d\n", larbs);

	for (i = 0; i < larbs; i++) {
		larb_node = of_parse_phandle(
					pdev->dev.of_node, "mediatek,larbs", i);
		if (!larb_node) {
			dev_info(dev, "failed to get larb id\n");
			continue;
		}

		larb_pdev = of_find_device_by_node(larb_node);
		if (WARN_ON(!larb_pdev)) {
			of_node_put(larb_node);
			dev_info(dev, "failed to get larb pdev\n");
			continue;
		}
		of_node_put(larb_node);

		link = device_link_add(&pdev->dev, &larb_pdev->dev,
						DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
		if (!link)
			dev_info(dev, "unable to link smi larb%d\n", i);
	}

	raw->fifo_size = roundup_pow_of_two(10 * sizeof(struct ut_raw_msg));
	if (kfifo_alloc(&raw->msgfifo, raw->fifo_size, GFP_KERNEL))
		return -ENOMEM;

	return 0;
}

static int mtk_ut_raw_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_raw_device *drvdata;
	int ret;

	//dev_info(dev, "%s\n", __func__);

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->dev = dev;
	dev_set_drvdata(dev, drvdata);

	ret = mtk_ut_raw_of_probe(pdev, drvdata);
	if (ret)
		return ret;

	ret = init_event_source(&drvdata->event_src);
	if (ret)
		return ret;

	ut_raw_set_ops(dev);
#if WITH_POWER_DRIVER
	pm_runtime_enable(dev);
#endif
	ret = component_add(dev, &mtk_ut_raw_component_ops);
	if (ret)
		return ret;

	dev_info(dev, "%s: success\n", __func__);
	return 0;
}

static int mtk_ut_raw_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_raw_device *drvdata = dev_get_drvdata(dev);
	int i;

	//dev_info(dev, "%s\n", __func__);
#if WITH_POWER_DRIVER
	pm_runtime_disable(dev);
#endif
	component_del(dev, &mtk_ut_raw_component_ops);

	for (i = 0; i < drvdata->num_clks; i++)
		clk_put(drvdata->clks[i]);

	kfifo_free(&drvdata->msgfifo);

	return 0;
}

static int mtk_ut_raw_pm_suspend(struct device *dev)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	u32 val;
	int ret;

	dev_dbg(dev, "- %s\n", __func__);

#if WITH_POWER_DRIVER
	if (pm_runtime_suspended(dev))
		return 0;
#endif
	/* Disable ISP's view finder and wait for TG idle */
	dev_dbg(dev, "cam suspend, disable VF\n");
	val = readl(raw->base + REG_TG_VF_CON);
	writel(val & (~TG_VFDATA_EN), raw->base + REG_TG_VF_CON);
	ret = readl_poll_timeout_atomic(raw->base + REG_TG_INTER_ST, val,
					(val & TG_CAM_CS_MASK) == TG_IDLE_ST,
					USEC_PER_MSEC, 33);
	if (ret)
		dev_dbg(dev, "can't stop HW:%d:0x%x\n", ret, val);

	/* Disable CMOS */
	val = readl(raw->base + REG_TG_SEN_MODE);
	writel(val & (~TG_SEN_MODE_CMOS_EN), raw->base + REG_TG_SEN_MODE);

#if WITH_POWER_DRIVER
	/* Force ISP HW to idle */
	ret = pm_runtime_force_suspend(dev);
#endif
	return ret;
}

static int mtk_ut_raw_pm_resume(struct device *dev)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	u32 val;
	int ret = 0;

	dev_dbg(dev, "- %s\n", __func__);
#if WITH_POWER_DRIVER
	if (pm_runtime_suspended(dev))
		return 0;
	/* Force ISP HW to resume */
	ret = pm_runtime_force_resume(dev);
	if (ret)
		return ret;
#endif
	/* Enable CMOS */
	dev_dbg(dev, "cam resume, enable CMOS/VF\n");
	val = readl(raw->base + REG_TG_SEN_MODE);
	writel(val | TG_SEN_MODE_CMOS_EN, raw->base + REG_TG_SEN_MODE);

	/* Enable VF */
	val = readl(raw->base + REG_TG_VF_CON);
	writel(val | TG_VFDATA_EN, raw->base + REG_TG_VF_CON);

	return ret;
}

static int mtk_ut_raw_runtime_suspend(struct device *dev)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	int i;

	ut_raw_reset(dev);

	for (i = 0; i < raw->num_clks; i++)
		clk_disable_unprepare(raw->clks[i]);

	return 0;
}

static int mtk_ut_raw_runtime_resume(struct device *dev)
{
	struct mtk_ut_raw_device *raw = dev_get_drvdata(dev);
	int i, ret;

	for (i = 0; i < raw->num_clks; i++) {
		ret = clk_prepare_enable(raw->clks[i]);
		if (ret) {
			dev_info(dev, "enable failed at clk #%d, ret = %d\n",
				 i, ret);
			i--;
			while (i >= 0)
				clk_disable_unprepare(raw->clks[i--]);

			return ret;
		}
	}

	ut_raw_reset(dev);
	return 0;
}

static const struct dev_pm_ops mtk_ut_raw_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_ut_raw_pm_suspend, mtk_ut_raw_pm_resume)
	SET_RUNTIME_PM_OPS(mtk_ut_raw_runtime_suspend, mtk_ut_raw_runtime_resume,
			   NULL)
};

static const struct of_device_id mtk_ut_raw_of_ids[] = {
	{.compatible = "mediatek,cam-raw",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_ut_raw_of_ids);

struct platform_driver mtk_ut_raw_driver = {
	.probe   = mtk_ut_raw_probe,
	.remove  = mtk_ut_raw_remove,
	.driver  = {
		.name  = "mtk-cam ut-raw",
		.of_match_table = of_match_ptr(mtk_ut_raw_of_ids),
		.pm     = &mtk_ut_raw_pm_ops,
	}
};

/* YUV part driver */
static void ut_yuv_set_ops(struct device *dev)
{
	struct mtk_ut_yuv_device *drvdata = dev_get_drvdata(dev);

	drvdata->ops.reset = NULL;
	drvdata->ops.s_stream = NULL;
	drvdata->ops.apply_cq = NULL;
	drvdata->ops.initialize = NULL;
}

static int mtk_ut_yuv_component_bind(struct device *dev,
				     struct device *master, void *data)
{
	struct mtk_ut_yuv_device *drvdata = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = data;

	//dev_info(dev, "%s\n", __func__);

	if (!data) {
		dev_info(dev, "no master data\n");
		return -1;
	}

	if (!ut->yuv) {
		dev_info(dev, "no yuv arr, num of yuv %d\n", ut->num_yuv);
		return -1;
	}
	ut->yuv[drvdata->id] = dev;

	return 0;
}

static void mtk_ut_yuv_component_unbind(struct device *dev,
					struct device *master, void *data)
{
	struct mtk_ut_yuv_device *drvdata = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = data;

	//dev_dbg(dev, "%s\n", __func__);
	ut->yuv[drvdata->id] = NULL;
}

static const struct component_ops mtk_ut_yuv_component_ops = {
	.bind = mtk_ut_yuv_component_bind,
	.unbind = mtk_ut_yuv_component_unbind,
};

static void yuv_handle_dma_err(struct mtk_ut_yuv_device *raw)
{
	//void __iomem *base = raw->base;
	void __iomem *yuv_base = raw->base;

	dev_info_ratelimited(raw->dev,
			    "YUVO_R1/R2/R3/R4/R5:%x/%x/%x/%x/%x\n",
			    readl_relaxed(yuv_base + REG_YUVO_R1_ERR_STAT),
			    readl_relaxed(yuv_base + REG_YUVO_R2_ERR_STAT),
			    readl_relaxed(yuv_base + REG_YUVO_R3_ERR_STAT),
			    readl_relaxed(yuv_base + REG_YUVO_R4_ERR_STAT),
			    readl_relaxed(yuv_base + REG_YUVO_R5_ERR_STAT)
			   );

	dev_info_ratelimited(raw->dev,
			    "RZH1N2TO_R1/R2/R3:%x/%x/%x,DRZS4NO_R1/R2/R3:%x/%x/%x\n",
			    readl_relaxed(yuv_base + REG_RZH1N2TO_R1_ERR_STAT),
			    readl_relaxed(yuv_base + REG_RZH1N2TO_R2_ERR_STAT),
			    readl_relaxed(yuv_base + REG_RZH1N2TO_R3_ERR_STAT),
			    readl_relaxed(yuv_base + REG_DRZS4NO_R1_ERR_STAT),
			    readl_relaxed(yuv_base + REG_DRZS4NO_R2_ERR_STAT),
			    readl_relaxed(yuv_base + REG_DRZS4NO_R3_ERR_STAT)
			   );
}

static irqreturn_t mtk_ut_yuv_irq(int irq, void *data)
{
	struct mtk_ut_yuv_device *drvdata = data;
	void __iomem *base = drvdata->base;
	struct ut_yuv_status status;

	/* yuv */
	status.irq = readl_relaxed(CAM_REG_CTL2_RAW_INT_STATUS(base));
	status.wdma = readl_relaxed(CAM_REG_CTL2_RAW_INT2_STATUS(base));
	status.drop = readl_relaxed(CAM_REG_CTL2_RAW_INT4_STATUS(base));
	status.ofl = readl_relaxed(CAM_REG_CTL2_RAW_INT5_STATUS(base));

	if (status.irq & YUV_DMA_ERR_ST)
		yuv_handle_dma_err(data);

	if ((dump_cmd & DUMP_STX) && (status.irq & YUV_PASS1_DON_ST)) {
		struct ut_yuv_status statusx;

		statusx.irq = readl_relaxed(CAM_REG_CTL2_RAW_INT_STATUSX(base));
		statusx.wdma = readl_relaxed(CAM_REG_CTL2_RAW_INT2_STATUSX(base));
		statusx.drop = readl_relaxed(CAM_REG_CTL2_RAW_INT4_STATUSX(base));
		statusx.ofl = readl_relaxed(CAM_REG_CTL2_RAW_INT5_STATUSX(base));

		dev_info(drvdata->dev, "STATUSX INT-DONE 1245 0x%x/0x%x/0x%x/0x%x\n",
			 statusx.irq, statusx.wdma, statusx.drop, statusx.ofl);
	}

	/* overflow interrupts may be annoying */
	dev_info_ratelimited(drvdata->dev, "INT5 overflow 0x%x\n",
			     status.ofl);

	if (status.irq || status.wdma || status.drop)
		dev_info(drvdata->dev, "INT 1245 0x%x/0x%x/0x%x/0x%x\n",
			 status.irq, status.wdma, status.drop, status.ofl);

	return IRQ_HANDLED;
}

static int mtk_ut_yuv_of_probe(struct platform_device *pdev,
			    struct mtk_ut_yuv_device *drvdata)
{
	struct device *dev = &pdev->dev;
	struct platform_device *larb_pdev;
	struct device_node *larb_node;
	struct device_link *link;
	struct resource *res;
	int irq, clks, larbs, ret;
	int i;

	ret = of_property_read_u32(dev->of_node, "mediatek,cam-id",
				   &drvdata->id);
	dev_info(dev, "id = %d\n", drvdata->id);
	if (ret) {
		dev_info(dev, "missing camid property\n");
		return ret;
	}

	/* base outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	drvdata->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(drvdata->base)) {
		dev_info(dev, "failed to map register base\n");
		return PTR_ERR(drvdata->base);
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_info(dev, "failed to get irq\n");
		return -ENODEV;
	}

	ret = devm_request_threaded_irq(dev, irq,
					NULL, mtk_ut_yuv_irq,
					IRQF_ONESHOT, dev_name(dev), drvdata);
	if (ret) {
		dev_info(dev, "failed to request irq=%d\n", irq);
		return ret;
	}
	dev_dbg(dev, "registered irq=%d\n", irq);

	clks = of_count_phandle_with_args(pdev->dev.of_node,
				"clocks", "#clock-cells");

	drvdata->num_clks = (clks == -ENOENT) ? 0:clks;
	dev_info(dev, "clk_num:%d\n", drvdata->num_clks);

	if (drvdata->num_clks) {
		drvdata->clks = devm_kcalloc(dev, drvdata->num_clks,
					     sizeof(*drvdata->clks),
					     GFP_KERNEL);
		if (!drvdata->clks)
			return -ENODEV;
	}

	for (i = 0; i < drvdata->num_clks; i++) {
		drvdata->clks[i] = of_clk_get(pdev->dev.of_node, i);
		if (IS_ERR(drvdata->clks[i])) {
			dev_info(dev, "failed to get clk %d\n", i);
			return -ENODEV;
		}
	}

	larbs = of_count_phandle_with_args(
					pdev->dev.of_node, "mediatek,larbs", NULL);
	dev_info(dev, "larb_num:%d\n", larbs);

	for (i = 0; i < larbs; i++) {
		larb_node = of_parse_phandle(
					pdev->dev.of_node, "mediatek,larbs", i);
		if (!larb_node) {
			dev_info(dev, "failed to get larb id\n");
			continue;
		}

		larb_pdev = of_find_device_by_node(larb_node);
		if (WARN_ON(!larb_pdev)) {
			of_node_put(larb_node);
			dev_info(dev, "failed to get larb pdev\n");
			continue;
		}
		of_node_put(larb_node);

		link = device_link_add(&pdev->dev, &larb_pdev->dev,
						DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
		if (!link)
			dev_info(dev, "unable to link smi larb%d\n", i);
	}

	return 0;
}


static int mtk_ut_yuv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_yuv_device *drvdata;
	int ret;

	dev_info(dev, "%s\n", __func__);

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->dev = dev;
	dev_set_drvdata(dev, drvdata);

	ret = mtk_ut_yuv_of_probe(pdev, drvdata);
	if (ret)
		return ret;

	ut_yuv_set_ops(dev);
#if WITH_POWER_DRIVER
	pm_runtime_enable(dev);
#endif
	ret = component_add(dev, &mtk_ut_yuv_component_ops);
	if (ret)
		return ret;

	dev_info(dev, "%s: success\n", __func__);
	return 0;
}

static int mtk_ut_yuv_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_yuv_device *drvdata = dev_get_drvdata(dev);
	int i;

	//dev_info(dev, "%s\n", __func__);
#if WITH_POWER_DRIVER
	pm_runtime_disable(dev);
#endif
	component_del(dev, &mtk_ut_yuv_component_ops);

	for (i = 0; i < drvdata->num_clks; i++)
		clk_put(drvdata->clks[i]);

	return 0;
}


static int mtk_ut_yuv_pm_suspend(struct device *dev)
{
	int ret = 0;

	dev_dbg(dev, "- %s\n", __func__);
#if WITH_POWER_DRIVER
	if (pm_runtime_suspended(dev))
		return 0;

	ret = pm_runtime_force_suspend(dev);
#endif
	return ret;
}

static int mtk_ut_yuv_pm_resume(struct device *dev)
{
	int ret = 0;

	dev_dbg(dev, "- %s\n", __func__);
#if WITH_POWER_DRIVER
	if (pm_runtime_suspended(dev))
		return 0;

	/* Force ISP HW to resume */
	ret = pm_runtime_force_resume(dev);
	if (ret)
		return ret;
#endif
	return ret;
}

static int mtk_ut_yuv_runtime_suspend(struct device *dev)
{
	struct mtk_ut_yuv_device *raw = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < raw->num_clks; i++)
		clk_disable_unprepare(raw->clks[i]);
	return 0;
}

static int mtk_ut_yuv_runtime_resume(struct device *dev)
{
	struct mtk_ut_yuv_device *raw = dev_get_drvdata(dev);
	int i, ret;

	for (i = 0; i < raw->num_clks; i++) {
		ret = clk_prepare_enable(raw->clks[i]);
		if (ret) {
			dev_info(dev, "enable failed at clk #%d, ret = %d\n",
				 i, ret);
			i--;
			while (i >= 0)
				clk_disable_unprepare(raw->clks[i--]);

			return ret;
		}
	}
	return 0;
}


static const struct dev_pm_ops mtk_ut_yuv_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_ut_yuv_pm_suspend, mtk_ut_yuv_pm_resume)
	SET_RUNTIME_PM_OPS(mtk_ut_yuv_runtime_suspend, mtk_ut_yuv_runtime_resume,
			   NULL)
};

static const struct of_device_id mtk_ut_yuv_of_ids[] = {
	{.compatible = "mediatek,cam-yuv",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_ut_yuv_of_ids);

struct platform_driver mtk_ut_yuv_driver = {
	.probe   = mtk_ut_yuv_probe,
	.remove  = mtk_ut_yuv_remove,
	.driver  = {
		.name  = "mtk-cam ut-yuv",
		.of_match_table = of_match_ptr(mtk_ut_yuv_of_ids),
		.pm     = &mtk_ut_yuv_pm_ops,
	}
};

/* RMS part driver */
static void ut_rms_set_ops(struct device *dev)
{
	struct mtk_ut_rms_device *drvdata = dev_get_drvdata(dev);

	drvdata->ops.reset = NULL;
	drvdata->ops.s_stream = NULL;
	drvdata->ops.apply_cq = NULL;
	drvdata->ops.initialize = NULL;
}

static int mtk_ut_rms_component_bind(struct device *dev,
				     struct device *master, void *data)
{
	struct mtk_ut_rms_device *drvdata = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = data;

	//dev_info(dev, "%s\n", __func__);

	if (!data) {
		dev_info(dev, "no master data\n");
		return -1;
	}

	if (!ut->rms) {
		dev_info(dev, "no rms arr, num of rms %d\n", ut->num_rms);
		return -1;
	}
	ut->rms[drvdata->id] = dev;

	return 0;
}

static void mtk_ut_rms_component_unbind(struct device *dev,
					struct device *master, void *data)
{
	struct mtk_ut_rms_device *drvdata = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = data;

	//dev_dbg(dev, "%s\n", __func__);
	ut->rms[drvdata->id] = NULL;
}

static const struct component_ops mtk_ut_rms_component_ops = {
	.bind = mtk_ut_rms_component_bind,
	.unbind = mtk_ut_rms_component_unbind,
};


static int mtk_ut_rms_of_probe(struct platform_device *pdev,
			    struct mtk_ut_rms_device *drvdata)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int clks, i, ret;

	ret = of_property_read_u32(dev->of_node, "mediatek,cam-id",
				   &drvdata->id);
	dev_info(dev, "id = %d\n", drvdata->id);
	if (ret) {
		dev_info(dev, "missing camid property\n");
		return ret;
	}

	/* base outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	drvdata->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(drvdata->base)) {
		dev_info(dev, "failed to map register base\n");
		return PTR_ERR(drvdata->base);
	}

	clks = of_count_phandle_with_args(pdev->dev.of_node,
				"clocks", "#clock-cells");

	drvdata->num_clks = (clks == -ENOENT) ? 0:clks;
	dev_info(dev, "clk_num:%d\n", drvdata->num_clks);

	if (drvdata->num_clks) {
		drvdata->clks = devm_kcalloc(dev, drvdata->num_clks,
						sizeof(*drvdata->clks),
						GFP_KERNEL);
		if (!drvdata->clks)
			return -ENODEV;
	}

	for (i = 0; i < drvdata->num_clks; i++) {
		drvdata->clks[i] = of_clk_get(pdev->dev.of_node, i);
		if (IS_ERR(drvdata->clks[i])) {
			dev_info(dev, "failed to get clk %d\n", i);
			return -ENODEV;
		}
	}

	return 0;
}


static int mtk_ut_rms_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_rms_device *drvdata;
	int ret;

	dev_info(dev, "%s\n", __func__);

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->dev = dev;
	dev_set_drvdata(dev, drvdata);

	ret = mtk_ut_rms_of_probe(pdev, drvdata);
	if (ret)
		return ret;

	ut_rms_set_ops(dev);
#if WITH_POWER_DRIVER
	pm_runtime_enable(dev);
#endif
	ret = component_add(dev, &mtk_ut_rms_component_ops);
	if (ret)
		return ret;

	dev_info(dev, "%s: success\n", __func__);
	return 0;
}

static int mtk_ut_rms_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_rms_device *drvdata = dev_get_drvdata(dev);
	int i;

	//dev_info(dev, "%s\n", __func__);
#if WITH_POWER_DRIVER
	pm_runtime_disable(dev);
#endif
	component_del(dev, &mtk_ut_rms_component_ops);

	for (i = 0; i < drvdata->num_clks; i++)
		clk_put(drvdata->clks[i]);

	return 0;
}

static int mtk_ut_rms_pm_suspend(struct device *dev)
{
	int ret = 0;

	dev_dbg(dev, "- %s\n", __func__);
#if WITH_POWER_DRIVER
	if (pm_runtime_suspended(dev))
		return 0;

	ret = pm_runtime_force_suspend(dev);
#endif
	return ret;
}

static int mtk_ut_rms_pm_resume(struct device *dev)
{
	int ret = 0;

	dev_dbg(dev, "- %s\n", __func__);
#if WITH_POWER_DRIVER
	if (pm_runtime_suspended(dev))
		return 0;

	/* Force ISP HW to resume */
	ret = pm_runtime_force_resume(dev);
	if (ret)
		return ret;
#endif
	return ret;
}

static int mtk_ut_rms_runtime_suspend(struct device *dev)
{
	struct mtk_ut_rms_device *raw = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < raw->num_clks; i++)
		clk_disable_unprepare(raw->clks[i]);
	return 0;
}

static int mtk_ut_rms_runtime_resume(struct device *dev)
{
	struct mtk_ut_rms_device *raw = dev_get_drvdata(dev);
	int i, ret;

	for (i = 0; i < raw->num_clks; i++) {
		ret = clk_prepare_enable(raw->clks[i]);
		if (ret) {
			dev_info(dev, "enable failed at clk #%d, ret = %d\n",
				 i, ret);
			i--;
			while (i >= 0)
				clk_disable_unprepare(raw->clks[i--]);

			return ret;
		}
	}
	return 0;
}


static const struct dev_pm_ops mtk_ut_rms_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_ut_rms_pm_suspend, mtk_ut_rms_pm_resume)
	SET_RUNTIME_PM_OPS(mtk_ut_rms_runtime_suspend, mtk_ut_rms_runtime_resume,
			   NULL)
};

static const struct of_device_id mtk_ut_rms_of_ids[] = {
	{.compatible = "mediatek,cam-rms",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_ut_rms_of_ids);

struct platform_driver mtk_ut_rms_driver = {
	.probe   = mtk_ut_rms_probe,
	.remove  = mtk_ut_rms_remove,
	.driver  = {
		.name  = "mtk-cam ut-rms",
		.of_match_table = of_match_ptr(mtk_ut_rms_of_ids),
		.pm     = &mtk_ut_rms_pm_ops,
	}
};

#if WITH_LARB_DRIVER
static int mtk_ut_larb_component_bind(struct device *dev,
				      struct device *master,
				      void *data)
{
	struct device_link *link;

	//dev_dbg(dev, "%s\n", __func__);

	link = device_link_add(master, dev, DL_FLAG_AUTOREMOVE_CONSUMER |
			       DL_FLAG_PM_RUNTIME);
	if (!link) {
		dev_info(dev, "Unable to create link between %s and %s\n",
			 dev_name(master), dev_name(dev));
		return -ENODEV;
	}
	return 0;
}

static void mtk_ut_larb_component_unbind(struct device *dev,
					 struct device *master,
					 void *data)
{
	dev_dbg(dev, "Start unbind larb component , return with no execution\n");
}

static const struct component_ops mtk_ut_larb_component_ops = {
	.bind = mtk_ut_larb_component_bind,
	.unbind = mtk_ut_larb_component_unbind,
};

static int mtk_ut_larb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;

	//dev_info(dev, "%s\n", __func__);

#ifdef CONFIG_MTK_IOMMU_PGTABLE_EXT
#if (CONFIG_MTK_IOMMU_PGTABLE_EXT > 32)
	if (dma_set_mask_and_coherent(dev, DMA_BIT_MASK(34)))
		dev_dbg(dev, "%s: No suitable DMA available\n", __func__);
#endif
#endif

	if (!dev->dma_parms) {
		dev->dma_parms =
			devm_kzalloc(dev, sizeof(*dev->dma_parms), GFP_KERNEL);
		if (!dev->dma_parms)
			return -ENOMEM;
	}

	if (dev->dma_parms) {
		ret = dma_set_max_seg_size(dev, UINT_MAX);
		if (ret)
			dev_info(dev, "Failed to set DMA segment size\n");
	}
#if WITH_POWER_DRIVER
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
#endif
	ret = component_add(dev, &mtk_ut_larb_component_ops);
	if (ret)
		return ret;

	return 0;
}

static int mtk_ut_larb_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_info(dev, "%s disable larb\n", __func__);
#if WITH_POWER_DRIVER
	pm_runtime_put(dev);
	pm_runtime_disable(dev);
#endif
	return 0;
}

static const struct of_device_id mtk_ut_larb_of_ids[] = {
	{.compatible = "mediatek,camisp-larb",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_ut_larb_of_ids);

struct platform_driver mtk_ut_larb_driver = {
	.probe   = mtk_ut_larb_probe,
	.remove  = mtk_ut_larb_remove,
	.driver  = {
		.name  = "mtk-cam larb-ut",
		.of_match_table = of_match_ptr(mtk_ut_larb_of_ids),
	}
};
#endif
