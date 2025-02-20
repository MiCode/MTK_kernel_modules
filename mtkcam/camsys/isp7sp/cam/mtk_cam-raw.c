// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/vmalloc.h>
#include <linux/suspend.h>
#include <linux/rtc.h>
#include <mtk_printk_ctrl.h>

#include <soc/mediatek/smi.h>
#include <soc/mediatek/mmdvfs_v3.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>

#include "mtk_cam.h"
#include "mtk_cam-debug.h"
#include "mtk_cam-dvfs_qos.h"
#include "mtk_cam-raw.h"
#include "mtk_cam-raw_debug.h"
#include "mtk_cam-dmadbg.h"
#include "mtk_cam-raw_regs.h"
//#include "mtk_cam-hsf.h"
#include "mtk_cam-trace.h"
#include "iommu_debug.h"

//static int debug_dump_fbc;
//module_param(debug_dump_fbc, int, 0644);
//MODULE_PARM_DESC(debug_dump_fbc, "debug: dump fbc");

#define MTK_RAW_STOP_HW_TIMEOUT			(33)

#define KERNEL_LOG_MAX	                400

#define DMA_OFFSET_ERR_STAT	0x34

#define RAW_DEBUG 0

/* use spare register FHG/FH_SPARE_7 */
#define REG_FRAME_SEQ_NUM					0x2c40

static int reset_msgfifo(struct mtk_raw_device *dev);

#define FIFO_THRESHOLD(FIFO_SIZE, HEIGHT_RATIO, LOW_RATIO) \
	(((FIFO_SIZE * HEIGHT_RATIO) & 0xFFF) << 16 | \
	((FIFO_SIZE * LOW_RATIO) & 0xFFF))

static void set_fifo_threshold(void __iomem *dma_base, unsigned int fifo_size)
{
	writel_relaxed((0x10 << 24) | fifo_size,
			dma_base + DMA_OFFSET_CON0);
	writel_relaxed((0x1 << 28) | FIFO_THRESHOLD(fifo_size, 2/10, 1/10),
			dma_base + DMA_OFFSET_CON1);
	writel_relaxed((0x1 << 28) | FIFO_THRESHOLD(fifo_size, 4/10, 3/10),
			dma_base + DMA_OFFSET_CON2);
	writel_relaxed((0x1 << 31) | FIFO_THRESHOLD(fifo_size, 6/10, 5/10),
			dma_base + DMA_OFFSET_CON3);
	writel_relaxed((0x1 << 31) | FIFO_THRESHOLD(fifo_size, 1/10, 0),
			dma_base + DMA_OFFSET_CON4);
}

static struct mtk_yuv_device *get_yuv_dev(struct mtk_raw_device *raw_dev)
{
	struct device *dev;
	struct mtk_cam_device *cam = raw_dev->cam;

	dev = cam->engines.yuv_devs[raw_dev->id];

	return dev_get_drvdata(dev);
}

static struct mtk_raw_device *get_raw_dev(struct mtk_yuv_device *yuv_dev)
{
	struct device *dev;
	struct mtk_cam_device *cam = yuv_dev->cam;

	dev = cam->engines.raw_devs[yuv_dev->id];

	return dev_get_drvdata(dev);
}

static void init_camsys_settings(struct mtk_raw_device *dev, bool is_srt, bool is_slb)
{
	struct mtk_cam_device *cam_dev = dev->cam;
	struct mtk_yuv_device *yuv_dev = get_yuv_dev(dev);
	unsigned int reg_raw_urgent, reg_yuv_urgent;
	unsigned int raw_urgent, yuv_urgent;

	//Set rdy/req snapshot
	set_topdebug_rdyreq(dev, is_srt ? ALL_THE_TIME : TG_OVERRUN);

	//Set CQI sram size
	set_fifo_threshold(dev->base + REG_CQI_R1_BASE, 64);
	set_fifo_threshold(dev->base + REG_CQI_R2_BASE, 64);
	set_fifo_threshold(dev->base + REG_CQI_R3_BASE, 64);
	set_fifo_threshold(dev->base + REG_CQI_R4_BASE, 64);

	writel_relaxed(HALT1_EN, cam_dev->base + REG_HALT1_EN);
	writel_relaxed(HALT2_EN, cam_dev->base + REG_HALT2_EN);
	writel_relaxed(HALT3_EN, cam_dev->base + REG_HALT3_EN);
	writel_relaxed(HALT4_EN, cam_dev->base + REG_HALT4_EN);
	writel_relaxed(HALT13_EN, cam_dev->base + REG_HALT13_EN);

	//Disable low latency
	writel_relaxed(0xffff,
		dev->base + REG_CAMRAWDMATOP_LOW_LATENCY_LINE_CNT_IMGO_R1);
	writel_relaxed(0xffff,
		yuv_dev->base + REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_YUVO_R1);
	writel_relaxed(0xffff,
		yuv_dev->base + REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_YUVO_R3);
	writel_relaxed(0xffff,
		yuv_dev->base + REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_DRZS4NO_R1);

#ifdef DEBUG_DMA_ENABLE_CRC_EN
	/* for debug: crc_en */
	writel(BIT(24), dev->base + REG_CAMRAWDMATOP_DMA_DBG_SEL);
	writel(BIT(24), dev->base + REG_CAMYUVDMATOP_DMA_DBG_SEL);
#endif

	switch (dev->id) {
	case RAW_A:
		reg_raw_urgent = REG_HALT5_EN;
		reg_yuv_urgent = REG_HALT6_EN;
		raw_urgent = HALT5_EN;
		yuv_urgent = HALT6_EN;
		break;
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
	default:
		dev_info(dev->dev, "%s: unknown raw id %d\n", __func__, dev->id);
		return;
	}

	if (is_srt) {
		writel_relaxed(0x0, cam_dev->base + reg_raw_urgent);
		writel_relaxed(0x0, cam_dev->base + reg_yuv_urgent);
		if (dev->num_larbs && dev->larbs[0])
			mtk_smi_larb_ultra_dis(&dev->larbs[0]->dev, !is_slb);

		if (yuv_dev->num_larbs && yuv_dev->larbs[0])
			mtk_smi_larb_ultra_dis(&yuv_dev->larbs[0]->dev, true);
	} else {
		writel_relaxed(raw_urgent, cam_dev->base + reg_raw_urgent);
		writel_relaxed(yuv_urgent, cam_dev->base + reg_yuv_urgent);
		if (dev->num_larbs && dev->larbs[0])
			mtk_smi_larb_ultra_dis(&dev->larbs[0]->dev, false);

		if (yuv_dev->num_larbs && yuv_dev->larbs[0])
			mtk_smi_larb_ultra_dis(&yuv_dev->larbs[0]->dev, false);
	}

	wmb(); /* TBC */
	dev_info(dev->dev, "%s: is srt:%d halt1~10,13:0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, is_srt,
		readl(cam_dev->base + REG_HALT1_EN), readl(cam_dev->base + REG_HALT2_EN),
		readl(cam_dev->base + REG_HALT3_EN), readl(cam_dev->base + REG_HALT4_EN),
		readl(cam_dev->base + REG_HALT5_EN), readl(cam_dev->base + REG_HALT6_EN),
		readl(cam_dev->base + REG_HALT7_EN), readl(cam_dev->base + REG_HALT8_EN),
		readl(cam_dev->base + REG_HALT9_EN), readl(cam_dev->base + REG_HALT10_EN),
		readl(cam_dev->base + REG_HALT13_EN));
}

static void init_ADLWR_settings(struct mtk_cam_device *cam)
{
	if (IS_ERR_OR_NULL(cam->adl_base)) {
		if (CAM_DEBUG_ENABLED(JOB))
			dev_info(cam->dev, "%s: skipped\n", __func__);
		return;
	}

	/* CAMADLWR_CAMADLWR_ADL_CTRL_FIELD_ID_GROUP_2 */
	writel_relaxed(0x440, cam->adl_base + 0x850);
}

static void dump_cq_setting(struct mtk_raw_device *dev)
{
	dev_info(dev->dev, "CQ_EN 0x%08x THR_CTL 0x%08x 0x%08x, 0x%08x\n",
		 readl(dev->base + REG_CAMCQ_CQ_EN),
		 readl(dev->base + REG_CAMCQ_CQ_THR0_CTL),
		 readl(dev->base + REG_CAMCQ_CQ_SUB_THR0_CTL),
		 readl(dev->base + REG_CAMCQ_SCQ_START_PERIOD));
}

static void dump_interrupt(struct mtk_raw_device *dev)
{
	dev_info(dev->dev, "CAMCTL INT_EN 0x%08x\n",
		 readl_relaxed(dev->base + REG_CAMCTL_INT_EN));
}

static void dump_tg_setting(struct mtk_raw_device *dev, const char *msg)
{
	dev_info(dev->dev,
		 "%s [outer] TG SENMODE/VFCON/PATHCFG/VSEOL_SUB: %x/%x/%x/%x FRMSIZE/R GRABPXL/LIN: %x/%x %x/%x\n",
		 msg,
		 readl_relaxed(dev->base + REG_TG_SEN_MODE),
		 readl_relaxed(dev->base + REG_TG_VF_CON),
		 readl_relaxed(dev->base + REG_TG_PATH_CFG),
		 readl_relaxed(dev->base + REG_TG_VSEOL_SUB_CTL),
		 readl_relaxed(dev->base + REG_TG_FRMSIZE_ST),
		 readl_relaxed(dev->base + REG_TG_FRMSIZE_ST_R),
		 readl_relaxed(dev->base + REG_TG_SEN_GRAB_PXL),
		 readl_relaxed(dev->base + REG_TG_SEN_GRAB_LIN));

	dev_info(dev->dev,
		 "%s [inner] TG SENMODE/VFCON/PATHCFG/VSEOL_SUB: %x/%x/%x/%x GRABPXL/LIN: %x/%x\n",
		 msg,
		 readl_relaxed(dev->base_inner + REG_TG_SEN_MODE),
		 readl_relaxed(dev->base_inner + REG_TG_VF_CON),
		 readl_relaxed(dev->base_inner + REG_TG_PATH_CFG),
		 readl_relaxed(dev->base_inner + REG_TG_VSEOL_SUB_CTL),
		 readl_relaxed(dev->base_inner + REG_TG_SEN_GRAB_PXL),
		 readl_relaxed(dev->base_inner + REG_TG_SEN_GRAB_LIN));
}

static void dump_seqence(struct mtk_raw_device *dev)
{
	dev_info(dev->dev, "in 0x%08x out 0x%08x\n",
		 readl_relaxed(dev->base_inner + REG_FRAME_SEQ_NUM),
		 readl_relaxed(dev->base + REG_FRAME_SEQ_NUM));
}

static void reset_error_handling(struct mtk_raw_device *dev)
{
	dev->tg_grab_err_handle_cnt = 0;
	dev->dma_err_handle_cnt = 0;
	dev->tg_overrun_handle_cnt = 0;
}

#define CAMCQ_CQ_EN_DEFAULT	0x14
void initialize(struct mtk_raw_device *dev, int is_slave, int is_srt, int is_slb,
		struct engine_callback *cb)
{
	u32 val;

	val = CAMCQ_CQ_EN_DEFAULT;
	SET_FIELD(&val, CAMCQ_CQ_DROP_FRAME_EN, 1);
	writel_relaxed(val, dev->base + REG_CAMCQ_CQ_EN);

	writel_relaxed(0xffffffff, dev->base + REG_CAMCQ_SCQ_START_PERIOD);
	val = FBIT(CAMCQ_CQ_THR0_EN);
	SET_FIELD(&val, CAMCQ_CQ_THR0_MODE, 1);

	writel_relaxed(val, dev->base + REG_CAMCQ_CQ_THR0_CTL);
	writel_relaxed(val, dev->base + REG_CAMCQ_CQ_SUB_THR0_CTL);

	/* enable interrupt */
	val = FBIT(CAMCTL_CQ_THR0_DONE_EN) | FBIT(CAMCTL_CQ_THRSUB_DONE_EN);
	writel_relaxed(val, dev->base + REG_CAMCTL_INT6_EN);

#if RAW_DEBUG
	dump_cq_setting(dev);
#endif

	dev->is_slave = is_slave;
	dev->sof_count = 0;
	dev->tg_count = 0;
	dev->vsync_count = 0;
	dev->sub_sensor_ctrl_en = false;
	dev->time_shared_busy = 0;
	atomic_set(&dev->vf_en, 0);
	reset_msgfifo(dev);

	init_camsys_settings(dev, is_srt, is_slb);
	init_ADLWR_settings(dev->cam);

	dev->engine_cb = cb;
	engine_fsm_reset(&dev->fsm, dev->dev);
	dev->cq_ref = NULL;

	reset_error_handling(dev);

	/* Workaround: disable FLKO error_sof: double sof error
	 *   HW will send FLKO dma error when
	 *      FLKO rcnt = 0 (not going to output this frame)
	 *      However, HW_PASS1_DONE still comes as expected
	 */
	/* Workaround: in ISP7SP and before, the DMA ports
	 * in the same merge group will mistakenly emit
	 * double SOF DMA error if they write out interleavingly,
	 * ex. mstream w/ UFO
	 */
	writel_relaxed(0xFFFE0000,
		       dev->base + REG_FLKO_R1_BASE + DMA_OFFSET_ERR_STAT);
	writel_relaxed(0xFFFE0000,
		       dev->base + REG_UFEO_R1_BASE + DMA_OFFSET_ERR_STAT);
	writel_relaxed(0xFFFE0000,
		       dev->base + REG_PDO_R1_BASE + DMA_OFFSET_ERR_STAT);

	/* Workaround: disable AAO/AAHO error: double sof error for smvr
	 *  HW would send double sof to aao/aaho in subsample mode
	 *  disable it to bypass
	 */
	writel_relaxed(0xFFFE0000,
		       dev->base + REG_AAO_R1_BASE + DMA_OFFSET_ERR_STAT);
	writel_relaxed(0xFFFE0000,
		       dev->base + REG_AAHO_R1_BASE + DMA_OFFSET_ERR_STAT);
}
static void subsample_set_sensor_time(struct mtk_raw_device *dev,
	u32 subsample_ratio)
{
	dev->sub_sensor_ctrl_en = true;
	dev->set_sensor_idx = subsample_ratio - 2;
	dev->cur_vsync_idx = -1;
}

static void reset_reg(struct mtk_raw_device *dev)
{
	u32 cq_en, sw_done, sw_sub_ctl;

	cq_en = readl_relaxed(dev->base_inner + REG_CAMCQ_CQ_EN);
	sw_done = readl_relaxed(dev->base_inner + REG_CAMCTL_SW_PASS1_DONE);
	sw_sub_ctl = readl_relaxed(dev->base_inner + REG_CAMCTL_SW_SUB_CTL);

	SET_FIELD(&cq_en, CAMCQ_SCQ_SUBSAMPLE_EN, 0);
	SET_FIELD(&cq_en, CAMCQ_SCQ_STAGGER_MODE, 0);
	writel(cq_en, dev->base_inner + REG_CAMCQ_CQ_EN);
	writel(cq_en, dev->base + REG_CAMCQ_CQ_EN);

	SET_FIELD(&sw_done, CAMCTL_DOWN_SAMPLE_EN, 0);
	writel(sw_done, dev->base_inner + REG_CAMCTL_SW_PASS1_DONE);
	writel(sw_done, dev->base + REG_CAMCTL_SW_PASS1_DONE);

	writel(0, dev->base_inner + REG_CAMCTL_SW_SUB_CTL);
	writel(0, dev->base + REG_CAMCTL_SW_SUB_CTL);

	wmb(); /* make sure committed */

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "[%s] CQ_EN/SW_SUB_CTL/SW_DONE [in] 0x%x/0x%x/0x%x [out] 0x%x/0x%x/0x%x\n",
			 __func__,
			 readl_relaxed(dev->base_inner + REG_CAMCQ_CQ_EN),
			 readl_relaxed(dev->base_inner + REG_CAMCTL_SW_SUB_CTL),
			 readl_relaxed(dev->base_inner + REG_CAMCTL_SW_PASS1_DONE),
			 readl_relaxed(dev->base + REG_CAMCQ_CQ_EN),
			 readl_relaxed(dev->base + REG_CAMCTL_SW_SUB_CTL),
			 readl_relaxed(dev->base + REG_CAMCTL_SW_PASS1_DONE));
}

void subsample_enable(struct mtk_raw_device *dev, int subsample_ratio)
{
	u32 val;
	u32 sub_ratio = subsample_ratio - 1;

	subsample_set_sensor_time(dev, subsample_ratio);

	val = readl_relaxed(dev->base + REG_CAMCQ_CQ_EN);
	SET_FIELD(&val, CAMCQ_SCQ_SUBSAMPLE_EN, 1);
	writel_relaxed(val, dev->base + REG_CAMCQ_CQ_EN);

	val = FBIT(CAMCTL_DOWN_SAMPLE_EN);
	SET_FIELD(&val, CAMCTL_DOWN_SAMPLE_PERIOD, sub_ratio);
	writel_relaxed(val, dev->base + REG_CAMCTL_SW_PASS1_DONE);
	writel_relaxed(val, dev->base_inner + REG_CAMCTL_SW_PASS1_DONE);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "[%s] raw%d - CQ_EN:0x%x ratio %d\n",
			 __func__, dev->id,
			 readl_relaxed(dev->base + REG_CAMCQ_CQ_EN),
			 subsample_ratio);
}

/* TODO: cq_set_stagger_mode(dev, 0/1) */
void stagger_enable(struct mtk_raw_device *dev, bool is_dc)
{
	u32 val;

	val = readl_relaxed(dev->base + REG_CAMCQ_CQ_EN);
	SET_FIELD(&val, CAMCQ_SCQ_STAGGER_MODE, 1);
	SET_FIELD(&val, CAMCQ_SCQ_INVLD_CLR_CHK, is_dc ? 1 : 0);
	writel_relaxed(val, dev->base + REG_CAMCQ_CQ_EN);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "[%s] raw%d - CQ_EN:0x%x\n",
			 __func__, dev->id, readl_relaxed(dev->base + REG_CAMCQ_CQ_EN));
}

void stagger_disable(struct mtk_raw_device *dev)
{
	u32 val;

	val = readl_relaxed(dev->base + REG_CAMCQ_CQ_EN);
	SET_FIELD(&val, CAMCQ_SCQ_STAGGER_MODE, 0);
	SET_FIELD(&val, CAMCQ_SCQ_INVLD_CLR_CHK, 0);
	writel_relaxed(val, dev->base + REG_CAMCQ_CQ_EN);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "[%s] raw%d - CQ_EN:0x%x\n",
			 __func__, dev->id, readl_relaxed(dev->base + REG_CAMCQ_CQ_EN));
}

void apply_cq(struct mtk_raw_device *dev,
	      dma_addr_t cq_addr,
	      unsigned int cq_size, unsigned int cq_offset,
	      unsigned int sub_cq_size, unsigned int sub_cq_offset)
{
	dma_addr_t main, sub;

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "apply raw%d cq - addr:0x%llx, size:%d/%d, offset:%d\n",
			 dev->id, cq_addr, cq_size, sub_cq_size, sub_cq_offset);

	/* note: apply cq with size = 0, will cause cq hang */
	if (WARN_ON(!cq_size || !sub_cq_size))
		return;

	main = cq_addr + cq_offset;
	sub = cq_addr + sub_cq_offset;

	writel_relaxed(dmaaddr_lsb(main),
		       dev->base + REG_CAMCQ_CQ_THR0_BASEADDR);
	writel_relaxed(dmaaddr_msb(main),
		       dev->base + REG_CAMCQ_CQ_THR0_BASEADDR_MSB);
	writel_relaxed(cq_size,
		       dev->base + REG_CAMCQ_CQ_THR0_DESC_SIZE);

	writel_relaxed(dmaaddr_lsb(sub),
		       dev->base + REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2);
	writel_relaxed(dmaaddr_msb(sub),
		       dev->base + REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2_MSB);
	writel_relaxed(sub_cq_size,
		       dev->base + REG_CAMCQ_CQ_SUB_THR0_DESC_SIZE_2);

	writel(FBIT(CAMCTL_CQ_THR0_START), dev->base + REG_CAMCTL_START);
}

void dbload_force(struct mtk_raw_device *dev)
{
	u32 val;

	val = readl_relaxed(dev->base + REG_CAMCTL_MISC);
	SET_FIELD(&val, CAMCTL_DB_LOAD_FORCE, 1);
	writel_relaxed(val, dev->base + REG_CAMCTL_MISC);
	writel_relaxed(val, dev->base_inner + REG_CAMCTL_MISC);
	wmb(); /* TBC */
	dev_info(dev->dev, "%s: 0x%x\n", __func__, val);
}

void toggle_db(struct mtk_raw_device *dev)
{
	u32 val;

	val = readl(dev->base + REG_CAMCTL_MISC);
	writel(val & ~FBIT(CAMCTL_DB_EN), dev->base + REG_CAMCTL_MISC);

	/* read back to make sure committed */
	val = readl(dev->base + REG_CAMCTL_MISC);
	writel(val | FBIT(CAMCTL_DB_EN), dev->base + REG_CAMCTL_MISC);
}

void enable_tg_db(struct mtk_raw_device *dev, int en)
{
	u32 val;

	val = readl(dev->base + REG_TG_PATH_CFG);
	SET_FIELD(&val, TG_DB_LOAD_DIS, !en);
	writel(val, dev->base + REG_TG_PATH_CFG);
}

static void set_tg_vfdata_en(struct mtk_raw_device *dev, int on)
{
	u32 val;

	atomic_set(&dev->vf_en, !!on);

	val = readl(dev->base + REG_TG_VF_CON);
	SET_FIELD(&val, TG_VFDATA_EN, on);
	writel(val, dev->base + REG_TG_VF_CON);

	dev_info(dev->dev, "%s: 0x%08x\n",
		 __func__, readl(dev->base + REG_TG_VF_CON));
}

static u32 scq_cnt_rate_khz(u32 time_stamp_cnt)
{
	return SCQ_DEFAULT_CLK_RATE * 1000 / ((time_stamp_cnt + 1) * 2);
}

void update_scq_start_period(struct mtk_raw_device *dev, int scq_ms)
{
	u32 val, start_period;

	val = readl_relaxed(dev->base + REG_TG_TIME_STAMP_CNT);
	start_period = (scq_ms == -1) ? 0xFFFFFFFF :
		scq_ms * scq_cnt_rate_khz(val);

	writel_relaxed(start_period,
		       dev->base + REG_CAMCQ_SCQ_START_PERIOD);
	dev_info(dev->dev, "[%s] REG_CAMCQ_SCQ_START_PERIOD:0x%08x (%dms)\n",
		 __func__, readl(dev->base + REG_CAMCQ_SCQ_START_PERIOD), scq_ms);
}

#define MAX_P1_DELAY_RATIO 80
void update_done_tolerance(struct mtk_raw_device *dev, int delay_ms)
{
	u32 val;
	int tolerance_ms;

	val = readl_relaxed(dev->base + REG_TG_TIME_STAMP_CNT);
	tolerance_ms = delay_ms * MAX_P1_DELAY_RATIO / 100;
	writel_relaxed(tolerance_ms * scq_cnt_rate_khz(val),
			dev->base + REG_CAMCTL_DC_STAG_CTL);

	dev_info(dev->dev, "[%s] REG_CAMCTL_DC_STAG_CTL:0x%08x (%dms)\n",
		 __func__, readl(dev->base + REG_CAMCTL_DC_STAG_CTL), tolerance_ms);
}

static bool not_support_rwfbc(struct mtk_raw_device *dev)
{
	return cur_platform->hw->platform_id == 6897;
}

void rwfbc_inc_setup(struct mtk_raw_device *dev)
{
	u32 wfbc_en_raw, wfbc_en_yuv;

	if (not_support_rwfbc(dev))
		return;

	wfbc_en_raw = readl_relaxed(dev->base + REG_CAMCTL_WFBC_EN);
	wfbc_en_yuv = readl_relaxed(dev->yuv_base + REG_CAMCTL_WFBC_EN);
	writel(wfbc_en_raw, dev->base + REG_CAMCTL_WFBC_INC);
	writel(wfbc_en_yuv, dev->yuv_base + REG_CAMCTL_WFBC_INC);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev, "[%s] WFBC_INC camctl/camctl2:0x%x/0x%x\n",
			 __func__, wfbc_en_raw, wfbc_en_yuv);
}

void stream_on(struct mtk_raw_device *dev, int on, bool reset_at_off)
{
	if (on) {
		/* toggle db before stream-on */
		enable_tg_db(dev, 0);
		enable_tg_db(dev, 1);
		set_tg_vfdata_en(dev, 1);
	} else {
		set_tg_vfdata_en(dev, 0);
		enable_tg_db(dev, 0);
		enable_tg_db(dev, 1);

		/* TODO: move reset_reg into initialize? */
		if (reset_at_off)
			reset_reg(dev);
	}

	//dev_info(dev->dev, "%s: %d\n", __func__, on);
}

void immediate_stream_off(struct mtk_raw_device *dev)
{
	if (WARN_ON_ONCE(1))
		dev_info(dev->dev, "not ready\n");
}

void m2m_update_sof_state(struct mtk_raw_device *dev)
{
	int cookie;

	/* update fsm's cookie_inner since m2m flow has no sof to update it */
	cookie = readl(dev->base_inner + REG_FRAME_SEQ_NUM);
	engine_fsm_sof(&dev->fsm, cookie, 0, NULL);

	engine_handle_sof(&dev->cq_ref,
			  bit_map_bit(MAP_HW_RAW, dev->id),
			  cookie);
}

static inline void trigger_rawi(struct mtk_raw_device *dev, u32 val)
{
	//dev_info(dev->dev, "%s: 0x%x\n", __func__, val);
	writel(val, dev->base + REG_CAMCTL_RAWI_TRIG);
}

void trigger_rawi_r2(struct mtk_raw_device *dev)
{
	trigger_rawi(dev, FBIT(CAMCTL_RAWI_R2_TRIG));
}

void trigger_rawi_r5(struct mtk_raw_device *dev)
{
	trigger_rawi(dev, FBIT(CAMCTL_RAWI_R5_TRIG));
}

#define CAM_MAIN_ADLRD_CTRL 0x32c
void trigger_adl(struct mtk_raw_device *dev)
{
	int adlrd_ctrl;

	adlrd_ctrl =
		(dev->id << 1) | /* ADLRD_MUX_SEL */
		0x1; /* ADLRD_EN */

	writel(adlrd_ctrl, dev->cam->base + CAM_MAIN_ADLRD_CTRL);
	trigger_rawi(dev, FBIT(CAMCTL_APU_TRIG) | FBIT(CAMCTL_RAW_TRIG));
}

static void write_pkt_apu_raw(struct mtk_raw_device *dev,
			      struct cmdq_pkt *pkt,
			      bool is_apu_dc)
{
	int raw_id = dev->id;
	int raw_base;
	int adlrd_ctrl;
	int trig;

	raw_base = (raw_id == 0) ? 0x1a030000 :
		   (raw_id == 1) ? 0x1a070000 : 0x1a0b0000;

	adlrd_ctrl =
		(raw_id << 1) | /* ADLRD_MUX_SEL */
		0x1; /* ADLRD_EN */

	trig = is_apu_dc ?
		FBIT(CAMCTL_APU_TRIG) :
		(FBIT(CAMCTL_APU_TRIG) | FBIT(CAMCTL_RAW_TRIG));

	if (is_apu_dc)
		cmdq_pkt_write(pkt, NULL, 0x1a003380, 0xf0000, 0xffffffff);
	else
		cmdq_pkt_write(pkt, NULL, 0x1a003380, 0x00001, 0xffffffff);

	/* CAM_MAIN_ADLRD_CTRL */
	cmdq_pkt_write(pkt, NULL, 0x1a00032c, adlrd_ctrl, 0xffffffff);

	/* CAMCTL_RAWI_TRIG: CAMCTL_APU_TRIG */
	cmdq_pkt_write(pkt, NULL, raw_base + REG_CAMCTL_RAWI_TRIG, trig,
		       0xffffffff);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev, "dc %d adlrd_ctrl 0x%x RAWI_TRIG 0x%x\n",
			 is_apu_dc, adlrd_ctrl, trig);
}

void write_pkt_trigger_apu_dc(struct mtk_raw_device *dev,
			      struct cmdq_pkt *pkt)
{
#define APU_SW_EVENT (675)
	/* wait APU ready */
	cmdq_pkt_wfe(pkt, APU_SW_EVENT);

	write_pkt_apu_raw(dev, pkt, true /* is_apu_dc */);

	/* trigger APU */
	cmdq_pkt_write(pkt, NULL, 0x190E1600, 0x1, 0xffffffff);
}

void write_pkt_trigger_apu_frame_mode(struct mtk_raw_device *dev,
				      struct cmdq_pkt *pkt)
{
	write_pkt_apu_raw(dev, pkt, false /* is_apu_dc */);
}

#define REG_DMA_SOFT_RST_STAT               0x4068
#define REG_DMA_SOFT_RST_STAT2              0x406C
#define REG_DMA_DBG_CHASING_STATUS          0x4098
#define REG_DMA_DBG_CHASING_STATUS2         0x409c
#define RAW_RST_STAT_CHECK		0x37ffffff
#define RAW_RST_STAT2_CHECK		0x1ff
#define YUV_RST_STAT_CHECK		0x1efffff
/* check again for rawi dcif case */
bool is_all_dma_idle(struct mtk_raw_device *dev)
{
	u32 chasing_stat = readl(dev->base + REG_DMA_DBG_CHASING_STATUS);
	u32 chasing_stat2 = readl(dev->base + REG_DMA_DBG_CHASING_STATUS2);
	u32 raw_rst_stat = readl(dev->base + REG_DMA_SOFT_RST_STAT);
	u32 raw_rst_stat2 = readl(dev->base + REG_DMA_SOFT_RST_STAT2);
	u32 yuv_rst_stat = readl(dev->yuv_base + REG_DMA_SOFT_RST_STAT);

	if (READ_FIELD(raw_rst_stat,
			CAMRAWDMATOP_RAWI_R2_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS_RAWI_R2) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat, CAMRAWDMATOP_RAWI_R2_SOFT_RST_STAT, 1);

	if (READ_FIELD(raw_rst_stat,
			CAMRAWDMATOP_UFDI_R2_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS_UFDI_R2) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat, CAMRAWDMATOP_UFDI_R2_SOFT_RST_STAT, 1);

	if (READ_FIELD(raw_rst_stat,
			CAMRAWDMATOP_RAWI_R3_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS_RAWI_R3) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat, CAMRAWDMATOP_RAWI_R3_SOFT_RST_STAT, 1);

	if (READ_FIELD(raw_rst_stat,
			CAMRAWDMATOP_UFDI_R3_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS_UFDI_R3) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat, CAMRAWDMATOP_UFDI_R3_SOFT_RST_STAT, 1);

	if (READ_FIELD(raw_rst_stat2,
			CAMRAWDMATOP_RAWI_R4_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat2,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS2_RAWI_R4) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat2, CAMRAWDMATOP_RAWI_R4_SOFT_RST_STAT, 1);

	if (READ_FIELD(raw_rst_stat2,
			CAMRAWDMATOP_UFDI_R4_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat2,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS2_UFDI_R4) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat2, CAMRAWDMATOP_UFDI_R4_SOFT_RST_STAT, 1);

	if (READ_FIELD(raw_rst_stat,
			CAMRAWDMATOP_RAWI_R5_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat2,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS2_RAWI_R5) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat, CAMRAWDMATOP_RAWI_R5_SOFT_RST_STAT, 1);

	if (READ_FIELD(raw_rst_stat,
			CAMRAWDMATOP_UFDI_R5_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat2,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS2_UFDI_R5) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat, CAMRAWDMATOP_UFDI_R5_SOFT_RST_STAT, 1);

	if (raw_rst_stat == RAW_RST_STAT_CHECK &&
		raw_rst_stat2 == RAW_RST_STAT2_CHECK &&
		yuv_rst_stat == YUV_RST_STAT_CHECK)
		return true;

	return false;
}

void dump_dma_soft_rst_stat(struct mtk_raw_device *dev)
{
	int raw_rst_stat = readl(dev->base + REG_DMA_SOFT_RST_STAT);
	int raw_rst_stat2 = readl(dev->base + REG_DMA_SOFT_RST_STAT2);
	int yuv_rst_stat = readl(dev->yuv_base + REG_DMA_SOFT_RST_STAT);

	dev_info(dev->dev, "%s: rst_stat: 0x%08x 0x%08x 0x%08x\n",
		 __func__, raw_rst_stat, raw_rst_stat2, yuv_rst_stat);
}

void reset(struct mtk_raw_device *dev)
{
	int sw_ctl;
	u32 mod6_en, val;
	int ret;

	dev_info(dev->dev, "%s\n", __func__);

	/* Disable all DMA DCM before reset */
	writel(0xffffffff, dev->base + REG_CAMCTL_MOD5_DCM_DIS);
	writel(0xffffffff, dev->base + REG_CAMCTL_MOD6_DCM_DIS);
	writel(0xffffffff, dev->yuv_base + REG_CAMCTL2_MOD5_DCM_DIS);
	writel(0xffffffff, dev->yuv_base + REG_CAMCTL2_MOD6_DCM_DIS);

	/* enable CQI_R1 ~ R4 before reset and make sure loaded to inner */
	mod6_en = readl(dev->base + REG_CAMCTL_MOD6_EN);
	val = mod6_en
		| FBIT(CAMCTL_CQI_R1_EN)
		| FBIT(CAMCTL_CQI_R2_EN)
		| FBIT(CAMCTL_CQI_R3_EN)
		| FBIT(CAMCTL_CQI_R4_EN);
	writel(val, dev->base + REG_CAMCTL_MOD6_EN);
	writel(val, dev->base_inner + REG_CAMCTL_MOD6_EN);

	writel(0, dev->base + REG_CAMCTL_SW_CTL);
	writel(FBIT(CAMCTL_SW_RST_TRIG), dev->base + REG_CAMCTL_SW_CTL);
	wmb(); /* make sure committed */

	ret = readx_poll_timeout(readl, dev->base + REG_CAMCTL_SW_CTL, sw_ctl,
				 sw_ctl & FBIT(CAMCTL_SW_RST_ST) || is_all_dma_idle(dev),
				 50 /* delay, us */,
				 5000 /* timeout, us */);
	if (ret < 0) {
		dev_info(dev->dev, "%s: error: reset timeout!\n",
			 __func__);
		dump_dma_soft_rst_stat(dev);
		mtk_smi_dbg_hang_detect("camsys-raw");
		goto RESET_FAILURE;
	}

	/* do hw rst */
	writel(FBIT(CAMCTL_HW_RST), dev->base + REG_CAMCTL_SW_CTL);
	writel(0, dev->base + REG_CAMCTL_SW_CTL);

RESET_FAILURE:

	writel(mod6_en, dev->base + REG_CAMCTL_MOD6_EN);
	writel(mod6_en, dev->base_inner + REG_CAMCTL_MOD6_EN);

	/* Enable all DMA DCM back */
	writel(0x0, dev->base + REG_CAMCTL_MOD5_DCM_DIS);
	writel(0x0, dev->base + REG_CAMCTL_MOD6_DCM_DIS);
	writel(0x0, dev->yuv_base + REG_CAMCTL2_MOD5_DCM_DIS);
	writel(0x0, dev->yuv_base + REG_CAMCTL2_MOD6_DCM_DIS);

	wmb(); /* make sure committed */
}

static int reset_msgfifo(struct mtk_raw_device *dev)
{
	atomic_set(&dev->is_fifo_overflow, 0);
	return kfifo_init(&dev->msg_fifo, dev->msg_buffer, dev->fifo_size);
}

static int push_msgfifo(struct mtk_raw_device *dev,
			struct mtk_camsys_irq_info *info)
{
	int len;

	if (unlikely(kfifo_avail(&dev->msg_fifo) < sizeof(*info))) {
		atomic_set(&dev->is_fifo_overflow, 1);
		return -1;
	}

	len = kfifo_in(&dev->msg_fifo, info, sizeof(*info));
	WARN_ON(len != sizeof(*info));

	return 0;
}

static void raw_handle_tg_grab_err(struct mtk_raw_device *raw_dev,
				   unsigned int fh_cookie);
static void raw_handle_dma_err(struct mtk_raw_device *raw_dev,
			       unsigned int fh_cookie);
static void raw_handle_tg_overrun_err(struct mtk_raw_device *raw_dev,
				      unsigned int fh_cookie);

static void raw_handle_error(struct mtk_raw_device *raw_dev,
			     struct mtk_camsys_irq_info *data)
{
	int err_status = data->e.err_status;
	unsigned int fh_cookie = data->frame_idx_inner;

	if (err_status & FBIT(CAMCTL_DMA_ERR_ST))
		raw_handle_dma_err(raw_dev, fh_cookie);

	if (err_status & FBIT(CAMCTL_TG_GRABERR_ST))
		raw_handle_tg_grab_err(raw_dev, fh_cookie);

	if (err_status & FBIT(CAMCTL_TG_OVRUN_ST))
		raw_handle_tg_overrun_err(raw_dev, fh_cookie);

	dev_info(raw_dev->dev, "%s: err_status:0x%x, fh_cookie:0x%x\n",
			__func__, err_status, fh_cookie);
}

static void raw_dump_debug_ufbc_status(struct mtk_raw_device *dev)
{
	mtk_cam_dump_ufd_debug(dev,
			       "UFD_R2",
			       dbg_UFD_R2, ARRAY_SIZE(dbg_UFD_R2));
	mtk_cam_dump_ufd_debug(dev,
			       "UFD_R5",
			       dbg_UFD_R5, ARRAY_SIZE(dbg_UFD_R5));
	mtk_cam_dump_dma_debug(dev,
			       dev->base + 0x4000, /* DMATOP_BASE */
			       "RAWI_R2",
			       dbg_RAWI_R2, ARRAY_SIZE(dbg_RAWI_R2));
	mtk_cam_dump_dma_debug(dev,
			       dev->base + 0x4000, /* DMATOP_BASE */
			       "RAWI_R2_UFD",
			       dbg_RAWI_R2_UFD, ARRAY_SIZE(dbg_RAWI_R2_UFD));
	mtk_cam_dump_dma_debug(dev,
			       dev->base + 0x4000, /* DMATOP_BASE */
			       "RAWI_R5",
			       dbg_RAWI_R5, ARRAY_SIZE(dbg_RAWI_R5));
	mtk_cam_dump_dma_debug(dev,
			       dev->base + 0x4000, /* DMATOP_BASE */
			       "RAWI_R5_UFD",
			       dbg_RAWI_R5_UFD, ARRAY_SIZE(dbg_RAWI_R5_UFD));

}

static void raw_handle_skip_frame(struct mtk_raw_device *raw_dev,
			     struct mtk_camsys_irq_info *data)
{
	int err_status = data->e.err_status;
	unsigned int fh_cookie = data->frame_idx_inner;

	dev_info(raw_dev->dev, "%s: dcif_status:0x%x, fh_cookie:0x%x\n",
			__func__, err_status, fh_cookie);

	if (err_status & FBIT(CAMCTL_P1_SKIP_FRAME_DC_STAG_INT_ST)) {
		raw_dump_debug_ufbc_status(raw_dev);

		mtk_smi_dbg_hang_detect("camsys-raw");

		do_engine_callback(raw_dev->engine_cb, dump_request,
				raw_dev->cam, CAMSYS_ENGINE_RAW, raw_dev->id,
				fh_cookie, MSG_DC_SKIP_FRAME);
	}
}

static void raw_handle_ringbuffer_ofl(struct mtk_raw_device *raw_dev,
				      struct mtk_camsys_irq_info *data)
{
	unsigned int fh_cookie = data->frame_idx_inner;

	dev_info(raw_dev->dev, "%s: fh_cookie: 0x%x, chasing_status 0x%x, 0x%x line_cnt:0x%x 0x%x 0x%x 0x%x\n",
		 __func__, fh_cookie,
		 readl_relaxed(raw_dev->base + REG_CAMRAWDMATOP_DC_DBG_CHASING_STATUS),
		 readl_relaxed(raw_dev->base + REG_CAMRAWDMATOP_DC_DBG_CHASING_STATUS2),
		 readl_relaxed(raw_dev->base + REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_RAWI_R2),
		 readl_relaxed(raw_dev->base + REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_UFDI_R2),
		 readl_relaxed(raw_dev->base + REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_RAWI_R5),
		 readl_relaxed(raw_dev->base + REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_UFDI_R5));

	WRAP_AEE_EXCEPTION(MSG_RINGBUFFER_OFL, dev_name(raw_dev->dev));
}

static bool is_sub_sample_sensor_timing(struct mtk_raw_device *dev)
{
	return dev->cur_vsync_idx >= dev->set_sensor_idx;
}

/* IRQ Error Mask */
#define INT_ST_MASK_CAM_ERR					\
	(FBIT(CAMCTL_TG_OVRUN_ST)			|	\
	 FBIT(CAMCTL_TG_GRABERR_ST)			|	\
	 FBIT(CAMCTL_TG_SOF_DROP_ST)			|	\
	 FBIT(CAMCTL_CQ_MAX_START_DLY_SMALL_INT_ST)	|	\
	 FBIT(CAMCTL_CQ_MAX_START_DLY_ERR_INT_ST)	|	\
	 FBIT(CAMCTL_CQ_MAIN_CODE_ERR_ST)		|	\
	 FBIT(CAMCTL_CQ_MAIN_VS_ERR_ST)			|	\
	 FBIT(CAMCTL_CQ_MAIN_VS_ERR_ST)			|	\
	 FBIT(CAMCTL_CQ_TRIG_DLY_INT_ST)		|	\
	 FBIT(CAMCTL_CQ_SUB_CODE_ERR_ST)		|	\
	 FBIT(CAMCTL_CQ_SUB_VS_ERR_ST)			|	\
	 FBIT(CAMCTL_DMA_ERR_ST))

#define DCIF_SKIP_MASK \
	(FBIT(CAMCTL_P1_SKIP_FRAME_ADL_INT_ST)			|	\
	 FBIT(CAMCTL_P1_SKIP_FRAME_2ND_PASS_TWO_SENSOR_INT_ST)	|	\
	 FBIT(CAMCTL_P1_SKIP_FRAME_1ST_PASS_TWO_SENSOR_INT_ST)	|	\
	 FBIT(CAMCTL_P1_SKIP_FRAME_2ND_PASS_RGBW_VHDR_INT_ST)	|	\
	 FBIT(CAMCTL_P1_SKIP_FRAME_1ST_PASS_RGBW_VHDR_INT_ST)	|	\
	 FBIT(CAMCTL_P1_SKIP_FRAME_DC_STAG_INT_ST))

#define RING_BUFFER_OFL_MASK \
	(FBIT(CAMCTL_UFDI_R5_RING_BUFFER_OVERFLOW_ST)		|	\
	 FBIT(CAMCTL_RAWI_R5_RING_BUFFER_OVERFLOW_ST)		|	\
	 FBIT(CAMCTL_UFDI_R4_RING_BUFFER_OVERFLOW_ST)		|	\
	 FBIT(CAMCTL_RAWI_R4_RING_BUFFER_OVERFLOW_ST)		|	\
	 FBIT(CAMCTL_UFDI_R3_RING_BUFFER_OVERFLOW_ST)		|	\
	 FBIT(CAMCTL_RAWI_R3_RING_BUFFER_OVERFLOW_ST)		|	\
	 FBIT(CAMCTL_UFDI_R2_RING_BUFFER_OVERFLOW_ST)		|	\
	 FBIT(CAMCTL_RAWI_R2_RING_BUFFER_OVERFLOW_ST))

/* TMP for FBC, to be removed */
#define WCNT_BIT_MASK				0xFF00
#define CNT_BIT_MASK				0xFF0000

#ifdef READ_TG_TIMESTAMP
static void mtk_raw_log_tg_timestamp(struct mtk_raw_device *raw_dev)
{
	u32 ts_ctl;
	u64 ts;

	dev_info(raw_dev->dev, "ctl 0x%x before lock, tg timestamp msb 0x%08x lsb 0x%08x\n",
		 readl_relaxed(raw_dev->base + REG_TG_TIME_STAMP_CTL),
		 readl_relaxed(raw_dev->base + REG_TG_TIME_STAMP_MSB),
		 readl_relaxed(raw_dev->base + REG_TG_TIME_STAMP));

	ts_ctl = readl_relaxed(raw_dev->base + REG_TG_TIME_STAMP_CTL) | BIT(4);
	writel(ts_ctl, raw_dev->base + REG_TG_TIME_STAMP_CTL);

	ts = (u64)readl(raw_dev->base + REG_TG_TIME_STAMP_MSB) << 32;
	ts += (u64)readl(raw_dev->base + REG_TG_TIME_STAMP);

	dev_info(raw_dev->dev, "ctl 0x%x after lock, tg timestamp %llu msb 0x%08x lsb 0x%08x\n",
		 readl_relaxed(raw_dev->base + REG_TG_TIME_STAMP_CTL),
		 ts,
		 readl_relaxed(raw_dev->base + REG_TG_TIME_STAMP_MSB),
		 readl_relaxed(raw_dev->base + REG_TG_TIME_STAMP));
}
#endif

static inline int is_fbc_empty(struct mtk_raw_device *raw_dev)
{
	int ctl2 = readl_relaxed(raw_dev->base + REG_FBC_FHO_R1_CTL2);

	return !((ctl2 >> 16) & 0xFF);
}

static irqreturn_t mtk_irq_raw(int irq, void *data)
{
	struct mtk_raw_device *raw_dev = (struct mtk_raw_device *)data;
	struct device *dev = raw_dev->dev;
	struct mtk_camsys_irq_info irq_info;
	unsigned int frame_idx, frame_idx_inner;
	unsigned int irq_status, err_status, dmao_done_status, dmai_done_status;
	unsigned int drop_status, dma_ofl_status, cq_done_status, dcif_status;
	unsigned int dma_ufl_status;
	unsigned int tg_cnt;
	bool wake_thread = 0;

	irq_status	 = readl_relaxed(raw_dev->base + REG_CAMCTL_INT_STATUS);
	dmao_done_status = readl_relaxed(raw_dev->base + REG_CAMCTL_INT2_STATUS);
	dmai_done_status = readl_relaxed(raw_dev->base + REG_CAMCTL_INT3_STATUS);
	drop_status	 = readl_relaxed(raw_dev->base + REG_CAMCTL_INT4_STATUS);
	dma_ofl_status	 = readl_relaxed(raw_dev->base + REG_CAMCTL_INT5_STATUS);
	cq_done_status	 = readl_relaxed(raw_dev->base + REG_CAMCTL_INT6_STATUS);
	dcif_status	 = readl_relaxed(raw_dev->base + REG_CAMCTL_INT7_STATUS);
	dma_ufl_status	 = readl_relaxed(raw_dev->base + REG_CAMCTL_INT8_STATUS);

	frame_idx	= readl_relaxed(raw_dev->base + REG_FRAME_SEQ_NUM);
	frame_idx_inner	= readl_relaxed(raw_dev->base_inner + REG_FRAME_SEQ_NUM);
	tg_cnt = readl_relaxed(raw_dev->base + REG_TG_INTER_ST);
	tg_cnt = (raw_dev->tg_count & 0xffffff00) + ((tg_cnt & 0xff000000) >> 24);
	err_status = irq_status & INT_ST_MASK_CAM_ERR;

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev,
			 "INT:0x%x(err:0x%x) 2~7 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x (in:0x%x) tg_cnt:%d/%lld\n",
			 irq_status, err_status,
			 dmao_done_status, dmai_done_status, drop_status,
			 dma_ofl_status, cq_done_status, dcif_status,
			 frame_idx_inner, tg_cnt, raw_dev->sof_count);

	irq_info.irq_type = 0;
	irq_info.ts_ns = ktime_get_boottime_ns();
	irq_info.frame_idx = frame_idx;
	irq_info.frame_idx_inner = frame_idx_inner;
	irq_info.fbc_empty = 0;

	/* CQ done */
	if (cq_done_status & FBIT(CAMCTL_CQ_THR0_DONE_ST)) {
		if (raw_dev->cq_ref != NULL) {
			long mask = bit_map_bit(MAP_HW_RAW, raw_dev->id);

			if (engine_handle_cq_done(&raw_dev->cq_ref, mask))
				irq_info.irq_type |= 1 << CAMSYS_IRQ_SETTING_DONE;
		}
	}

	/* DMAO done, only for AFO */
	if (dmao_done_status & FBIT(CAMCTL_AFO_R1_DONE_ST)) {
		irq_info.irq_type |= 1 << CAMSYS_IRQ_AFO_DONE;
		/* enable AFO_DONE_EN at backend manually */
	}

	/* Frame skipped */
	if (dcif_status & DCIF_SKIP_MASK) {
		irq_info.irq_type |= 1 << CAMSYS_IRQ_FRAME_SKIPPED;
		irq_info.e.err_status = dcif_status;
	}

	if (dma_ufl_status & RING_BUFFER_OFL_MASK)
		irq_info.irq_type |= 1 << CAMSYS_IRQ_RINGBUFFER_OVERFLOW;

	/* Frame done */
	if (irq_status & FBIT(CAMCTL_SW_PASS1_DONE_ST))
		irq_info.irq_type |= 1 << CAMSYS_IRQ_FRAME_DONE;

	/* Frame start */
	if (irq_status & FBIT(CAMCTL_TG_SOF_INT_ST) ||
		/* dc mode */
		dmao_done_status & FBIT(CAMCTL_FHO_R1_DONE_ST) ||
		dcif_status & FBIT(CAMCTL_DCIF_LAST_CQ_START_INT_ST)) {
		irq_info.irq_type |= 1 << CAMSYS_IRQ_FRAME_START;
		raw_dev->sof_count++;
		raw_dev->cur_vsync_idx = 0;

		if (not_support_rwfbc(raw_dev)) {
			irq_info.fbc_empty = is_fbc_empty(raw_dev);
		} else {
			if (irq_status & FBIT(CAMCTL_SW_ENQUE_ERR_ST) ||
				dcif_status & FBIT(CAMCTL_DCIF_LAST_CQ_START_INT_ST))
				irq_info.fbc_empty = 1;
			else
				irq_info.fbc_empty = 0;
		}

		if (tg_cnt < raw_dev->tg_count)
			raw_dev->tg_count = tg_cnt + BIT(8);
		else
			raw_dev->tg_count = tg_cnt;
		irq_info.tg_cnt = raw_dev->tg_count;
		if (CAM_DEBUG_ENABLED(EXTISP_SW_CNT))
			irq_info.tg_cnt = raw_dev->sof_count - 2;

		engine_handle_sof(&raw_dev->cq_ref,
				  bit_map_bit(MAP_HW_RAW, raw_dev->id),
				  irq_info.frame_idx_inner);
	}

	/* DCIF main sof */
	if (dcif_status & FBIT(CAMCTL_DCIF_FIRST_SOF_INT_ST))
		irq_info.irq_type |= 1 << CAMSYS_IRQ_FRAME_START_DCIF_MAIN;

	/* Vsync interrupt */
	if (irq_status & FBIT(CAMCTL_TG_VS_INT_ST))
		raw_dev->vsync_count++;

	if (raw_dev->sub_sensor_ctrl_en
	    && irq_status & FBIT(CAMCTL_TG_VS_INT_ORG_ST)
	    && raw_dev->cur_vsync_idx >= 0) {
		if (is_sub_sample_sensor_timing(raw_dev)) {
			raw_dev->cur_vsync_idx = -1;
			irq_info.irq_type |= 1 << CAMSYS_IRQ_TRY_SENSOR_SET;
		}
		++raw_dev->cur_vsync_idx;
	}

	if (irq_info.irq_type && !raw_dev->is_slave) {
		if (push_msgfifo(raw_dev, &irq_info) == 0)
			wake_thread = 1;
	}

	/* Check ISP error status */
	if (unlikely(err_status)) {
		struct mtk_camsys_irq_info err_info;

		err_info.irq_type = 1 << CAMSYS_IRQ_ERROR;
		err_info.ts_ns = irq_info.ts_ns;
		err_info.frame_idx = irq_info.frame_idx;
		err_info.frame_idx_inner = irq_info.frame_idx_inner;
		err_info.e.err_status = err_status;

		if (push_msgfifo(raw_dev, &err_info) == 0)
			wake_thread = 1;
	}

	/* enable to debug fbc related */
	//if (debug_raw && debug_dump_fbc && (irq_status & SOF_INT_ST))
	//	mtk_cam_raw_dump_fbc(raw_dev->dev, raw_dev->base, raw_dev->yuv_base);

	/* trace */
	trace_raw_irq(dev, frame_idx_inner,
		      irq_status, dmao_done_status, dmai_done_status,
		      cq_done_status, dcif_status);
	trace_raw_dma_status(dev, drop_status, dma_ofl_status, dma_ufl_status);

#ifdef NOT_READY
	if (MTK_CAM_TRACE_ENABLED(FBC) && (irq_status & TG_VS_INT_ORG_ST)) {
#ifdef DUMP_FBC_SEL_OUTER
		MTK_CAM_TRACE(FBC, "frame %d FBC_SEL 0x% 8x/0x% 8x (outer)",
			irq_info.frame_idx_inner,
			readl_relaxed(raw_dev->base + REG_CAMCTL_FBC_SEL),
			readl_relaxed(raw_dev->yuv_base + REG_CAMCTL_FBC_SEL));
#endif
		mtk_cam_raw_dump_fbc(dev, raw_dev->base, raw_dev->yuv_base);
	}
#endif

	return wake_thread ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static int raw_process_fsm(struct mtk_raw_device *raw_dev,
			   struct mtk_camsys_irq_info *irq_info,
			   int *recovered_done)
{
	struct engine_fsm *fsm = &raw_dev->fsm;
	int done_type;
	int cookie_done;
	int ret;
	int recovered = 0;

	done_type = irq_info->irq_type &
	    (BIT(CAMSYS_IRQ_AFO_DONE) | BIT(CAMSYS_IRQ_FRAME_DONE));

	if (done_type == BIT(CAMSYS_IRQ_AFO_DONE)) {
		/* special case: should not update state */
		ret = engine_fsm_partial_done(fsm, &cookie_done);
		if (!ret)
			irq_info->cookie_done = cookie_done;
	} else if (done_type) {

		ret = engine_fsm_hw_done(fsm, &cookie_done);
		if (ret > 0)
			irq_info->cookie_done = cookie_done;
		else {
			/* handle for fake p1 done */
			dev_info_ratelimited(raw_dev->dev, "warn: fake done in/out: 0x%x 0x%x\n",
					     irq_info->frame_idx_inner,
					     irq_info->frame_idx);
			irq_info->irq_type &= ~done_type;
			irq_info->cookie_done = 0;
		}
	}

	if (irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_START))
		recovered = engine_fsm_sof(fsm, irq_info->frame_idx_inner,
					   irq_info->fbc_empty,
					   recovered_done);

	if (recovered)
		dev_info(raw_dev->dev, "recovered done 0x%x in/out: 0x%x 0x%x\n",
			 *recovered_done,
			 irq_info->frame_idx_inner,
			 irq_info->frame_idx);

	return recovered;
}

static irqreturn_t mtk_thread_irq_raw(int irq, void *data)
{
	struct mtk_raw_device *raw_dev = (struct mtk_raw_device *)data;
	struct mtk_camsys_irq_info irq_info;
	int recovered_done;
	int do_recover;

	if (unlikely(atomic_cmpxchg(&raw_dev->is_fifo_overflow, 1, 0)))
		dev_info(raw_dev->dev, "msg fifo overflow\n");

	while (kfifo_len(&raw_dev->msg_fifo) >= sizeof(irq_info)) {
		int len = kfifo_out(&raw_dev->msg_fifo, &irq_info, sizeof(irq_info));

		WARN_ON(len != sizeof(irq_info));

#if RAW_DEBUG
		dev_info(raw_dev->dev, "ts=%lu irq_type %d, req:0x%x/0x%x tg_cnt:%d\n",
			irq_info.ts_ns / 1000,
			irq_info.irq_type,
			irq_info.frame_idx_inner,
			irq_info.frame_idx,
			irq_info.tg_cnt);
#endif

		/* error case */
		if (unlikely(irq_info.irq_type == (1 << CAMSYS_IRQ_ERROR))) {
			raw_handle_error(raw_dev, &irq_info);
			continue;
		}

		/* skip frame case */
		if (unlikely(irq_info.irq_type & BIT(CAMSYS_IRQ_FRAME_SKIPPED)))
			raw_handle_skip_frame(raw_dev, &irq_info);

		/* ringbuffer overflow case */
		if (unlikely(irq_info.irq_type & BIT(CAMSYS_IRQ_RINGBUFFER_OVERFLOW)))
			raw_handle_ringbuffer_ofl(raw_dev, &irq_info);

		if (irq_info.irq_type & (1 << CAMSYS_IRQ_FRAME_DONE))
			reset_error_handling(raw_dev);

		/* normal case */
		do_recover = raw_process_fsm(raw_dev, &irq_info,
					     &recovered_done);

		do_engine_callback(raw_dev->engine_cb, isr_event,
				   raw_dev->cam,
				   CAMSYS_ENGINE_RAW, raw_dev->id, &irq_info);

		if (do_recover) {
			irq_info.irq_type = BIT(CAMSYS_IRQ_FRAME_DONE);
			irq_info.cookie_done = recovered_done;

			do_engine_callback(raw_dev->engine_cb, isr_event,
					   raw_dev->cam,
					   CAMSYS_ENGINE_RAW, raw_dev->id,
					   &irq_info);
		}
	}

	return IRQ_HANDLED;
}

#define MAX_RETRY_SENSOR_CNT	2
static void raw_handle_tg_grab_err(struct mtk_raw_device *raw_dev,
				   unsigned int fh_cookie)
{
	int cnt;

	if (!atomic_read(&raw_dev->vf_en)) {
		dev_info(raw_dev->dev, "%s: skipped since vf is off\n", __func__);
		return;
	}

	cnt = raw_dev->tg_grab_err_handle_cnt++;

	dev_info_ratelimited(raw_dev->dev, "%s: cnt=%d, seq 0x%x\n",
			     __func__, cnt, fh_cookie);

	if (cnt <= MAX_RETRY_SENSOR_CNT)
		dump_tg_setting(raw_dev, "GRAB_ERR");

	if (cnt < MAX_RETRY_SENSOR_CNT)
		do_engine_callback(raw_dev->engine_cb, reset_sensor,
				   raw_dev->cam, CAMSYS_ENGINE_RAW, raw_dev->id,
				   fh_cookie);
	else if (cnt == MAX_RETRY_SENSOR_CNT)
		do_engine_callback(raw_dev->engine_cb, dump_request,
				   raw_dev->cam, CAMSYS_ENGINE_RAW, raw_dev->id,
				   fh_cookie, MSG_TG_GRAB_ERROR);
}

static void raw_handle_dma_err(struct mtk_raw_device *raw_dev,
			       unsigned int fh_cookie)
{
	int cnt;

	cnt = raw_dev->dma_err_handle_cnt++;

	if (cnt <= 3) {
		struct mtk_yuv_device *yuv_dev = get_yuv_dev(raw_dev);

		dump_raw_dma_err_st(raw_dev);
		dump_yuv_dma_err_st(yuv_dev);
	}
}

#define OVERRUN_DUMP_CNT 3
static void raw_handle_tg_overrun_err(struct mtk_raw_device *raw_dev,
				      unsigned int fh_cookie)
{
	int cnt;

	cnt = raw_dev->tg_overrun_handle_cnt++;

	dev_info_ratelimited(raw_dev->dev, "%s: cnt=%d, seq 0x%x\n",
			     __func__, cnt, fh_cookie);

	if (cnt < OVERRUN_DUMP_CNT)
		dump_topdebug_rdyreq(raw_dev);

	if (cnt < MAX_RETRY_SENSOR_CNT)
		do_engine_callback(raw_dev->engine_cb, reset_sensor,
				   raw_dev->cam, CAMSYS_ENGINE_RAW, raw_dev->id,
				   fh_cookie);
	else if (cnt == MAX_RETRY_SENSOR_CNT)
		do_engine_callback(raw_dev->engine_cb, dump_request,
				   raw_dev->cam, CAMSYS_ENGINE_RAW, raw_dev->id,
				   fh_cookie, MSG_TG_OVERRUN);
}

static int mtk_raw_pm_suspend_prepare(struct mtk_raw_device *dev)
{
//	u32 val;
	int ret;

	dev_dbg(dev->dev, "- %s\n", __func__);

	if (pm_runtime_suspended(dev->dev))
		return 0;

	/* Disable ISP's view finder and wait for TG idle */
	dev_dbg(dev->dev, "cam suspend, disable VF\n");
#ifdef NOT_READY
	val = readl(dev->base + REG_TG_VF_CON);
	writel(val & (~TG_VFDATA_EN), dev->base + REG_TG_VF_CON);
	ret = readl_poll_timeout_atomic(dev->base + REG_TG_INTER_ST, val,
					(val & TG_CAM_CS_MASK) == TG_IDLE_ST,
					USEC_PER_MSEC, MTK_RAW_STOP_HW_TIMEOUT);
	if (ret)
		dev_dbg(dev->dev, "can't stop HW:%d:0x%x\n", ret, val);

	/* Disable CMOS */
	val = readl(dev->base + REG_TG_SEN_MODE);
	writel(val & (~TG_SEN_MODE_CMOS_EN), dev->base + REG_TG_SEN_MODE);
#endif

	/* Force ISP HW to idle */
	ret = pm_runtime_force_suspend(dev->dev);
	return ret;
}

static int mtk_raw_pm_post_suspend(struct mtk_raw_device *dev)
{
//	u32 val;
	int ret;

	dev_dbg(dev->dev, "- %s\n", __func__);

	if (pm_runtime_suspended(dev->dev))
		return 0;

	/* Force ISP HW to resume */
	ret = pm_runtime_force_resume(dev->dev);
	if (ret)
		return ret;

#ifdef NOT_READY
	/* Enable CMOS */
	dev_dbg(dev->dev, "cam resume, enable CMOS/VF\n");
	val = readl(dev->base + REG_TG_SEN_MODE);
	writel(val | TG_SEN_MODE_CMOS_EN, dev->base + REG_TG_SEN_MODE);

	/* Enable VF */
	val = readl(dev->base + REG_TG_VF_CON);
	writel(val | TG_VFDATA_EN, dev->base + REG_TG_VF_CON);
#endif

	return 0;
}

static int raw_pm_notifier(struct notifier_block *nb,
							unsigned long action, void *data)
{
	struct mtk_raw_device *raw_dev =
			container_of(nb, struct mtk_raw_device, pm_notifier);

	switch (action) {
	case PM_SUSPEND_PREPARE:
		mtk_raw_pm_suspend_prepare(raw_dev);
		break;
	case PM_POST_SUSPEND:
		mtk_raw_pm_post_suspend(raw_dev);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int mtk_raw_of_probe(struct platform_device *pdev,
			    struct mtk_raw_device *raw)
{
	struct device *dev = &pdev->dev;
	struct platform_device *larb_pdev;
	struct device_node *larb_node;
	struct device_link *link;
	struct of_phandle_args args;
	struct resource *res;
	unsigned int i;
	int clks, larbs, iommus, smmus, ret;

	ret = of_property_read_u32(dev->of_node, "mediatek,cam-id",
				   &raw->id);
	if (ret) {
		dev_dbg(dev, "missing camid property\n");
		return ret;
	}

	if (dma_set_mask_and_coherent(dev, DMA_BIT_MASK(34)))
		dev_info(dev, "%s: No suitable DMA available\n", __func__);

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

	/* base outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	raw->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(raw->base)) {
		dev_dbg(dev, "failed to map register base\n");
		return PTR_ERR(raw->base);
	}

	/* base inner register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	raw->base_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(raw->base_inner)) {
		dev_dbg(dev, "failed to map register inner base\n");
		return PTR_ERR(raw->base_inner);
	}

	/* will be assigned later */
	raw->yuv_base = NULL;

	raw->irq = platform_get_irq(pdev, 0);
	if (raw->irq < 0) {
		dev_dbg(dev, "failed to get irq\n");
		return -ENODEV;
	}

	ret = devm_request_threaded_irq(dev, raw->irq,
					mtk_irq_raw,
					mtk_thread_irq_raw,
					IRQF_NO_AUTOEN, dev_name(dev), raw);
	if (ret) {
		dev_dbg(dev, "failed to request irq=%d\n", raw->irq);
		return ret;
	}
	dev_dbg(dev, "registered irq=%d\n", raw->irq);

	clks = of_count_phandle_with_args(pdev->dev.of_node, "clocks",
			"#clock-cells");

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
	raw->num_larbs = (larbs < 0) ? 0 : larbs;
	dev_info(dev, "larb_num:%d\n", larbs);

	if (raw->num_larbs) {
		raw->larbs = devm_kcalloc(dev,
					     raw->num_larbs, sizeof(*raw->larbs),
					     GFP_KERNEL);
		if (!raw->larbs)
			return -ENOMEM;
	}

	for (i = 0; i < raw->num_larbs; i++) {
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

		link = device_link_add(dev, &larb_pdev->dev,
						DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
		if (!link)
			dev_info(dev, "unable to link smi larb%d\n", i);
		else
			raw->larbs[i] = larb_pdev;
	}

#ifdef CONFIG_PM_SLEEP
	raw->pm_notifier.notifier_call = raw_pm_notifier;
	ret = register_pm_notifier(&raw->pm_notifier);
	if (ret) {
		dev_info(dev, "failed to register notifier block.\n");
		return ret;
	}
#endif

	iommus = of_count_phandle_with_args(
		pdev->dev.of_node, "iommus", "#iommu-cells");
	iommus = (iommus > 0) ? iommus : 0;
	for (i = 0; i < iommus; i++) {
		if (!of_parse_phandle_with_args(pdev->dev.of_node,
			"iommus", "#iommu-cells", i, &args)) {
			mtk_iommu_register_fault_callback(
				args.args[0], mtk_raw_translation_fault_cb,
				(void *)raw, false);
		}
	}

	smmus = of_property_count_u32_elems(
		pdev->dev.of_node, "mediatek,smmu-dma-axid");
	smmus = (smmus > 0) ? smmus : 0;
	for (i = 0; i < smmus; i++) {
		u32 axid;

		if (!of_property_read_u32_index(
			pdev->dev.of_node, "mediatek,smmu-dma-axid", i, &axid)) {
			mtk_iommu_register_fault_callback(
				axid, mtk_raw_translation_fault_cb,
				(void *)raw, false);
		}
	}

	return 0;
}

static int mtk_raw_component_bind(struct device *dev, struct device *master,
				  void *data)
{
	struct mtk_raw_device *raw_dev = dev_get_drvdata(dev);
	struct mtk_cam_device *cam_dev = data;

	raw_dev->cam = cam_dev;
	return mtk_cam_set_dev_raw(cam_dev->dev, raw_dev->id, dev, NULL, NULL);
}

static void mtk_raw_component_unbind(struct device *dev, struct device *master,
				     void *data)
{
}

static const struct component_ops mtk_raw_component_ops = {
	.bind = mtk_raw_component_bind,
	.unbind = mtk_raw_component_unbind,
};

static int mtk_raw_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_raw_device *raw_dev;
	int ret;

	raw_dev = devm_kzalloc(dev, sizeof(*raw_dev), GFP_KERNEL);
	if (!raw_dev)
		return -ENOMEM;

	raw_dev->dev = dev;
	dev_set_drvdata(dev, raw_dev);

	ret = mtk_raw_of_probe(pdev, raw_dev);
	if (ret)
		return ret;

	ret = mtk_cam_qos_probe(dev, &raw_dev->qos,
				GET_PLAT_HW(raw_icc_path_num));
	if (ret)
		goto UNREGISTER_PM_NOTIFIER;

	raw_dev->fifo_size =
		roundup_pow_of_two(8 * sizeof(struct mtk_camsys_irq_info));

	raw_dev->msg_buffer = devm_kzalloc(dev, raw_dev->fifo_size, GFP_KERNEL);
	if (!raw_dev->msg_buffer) {
		ret = -ENOMEM;
		goto UNREGISTER_PM_NOTIFIER;
	}

	raw_dev->default_printk_cnt = get_detect_count();

	pm_runtime_enable(dev);

	ret = component_add(dev, &mtk_raw_component_ops);
	if (ret)
		goto UNREGISTER_PM_NOTIFIER;

	return ret;

UNREGISTER_PM_NOTIFIER:
	unregister_pm_notifier(&raw_dev->pm_notifier);
	return ret;
}

static int mtk_raw_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_raw_device *raw_dev = dev_get_drvdata(dev);
	int i;

	unregister_pm_notifier(&raw_dev->pm_notifier);

	pm_runtime_disable(dev);
	mtk_cam_qos_remove(&raw_dev->qos);
	component_del(dev, &mtk_raw_component_ops);

	for (i = 0; i < raw_dev->num_clks; i++)
		clk_put(raw_dev->clks[i]);

	return 0;
}

static int mtk_raw_runtime_suspend(struct device *dev)
{
	struct mtk_raw_device *drvdata = dev_get_drvdata(dev);
	int i;
	unsigned int pr_detect_count;

	dev_dbg(dev, "%s:disable clock\n", __func__);

	pr_detect_count = get_detect_count();
	if (pr_detect_count > drvdata->default_printk_cnt)
		set_detect_count(drvdata->default_printk_cnt);

	// reset(drvdata);
	mtk_cam_reset_qos(dev, &drvdata->qos);

	for (i = 0; i < drvdata->num_clks; i++)
		clk_disable_unprepare(drvdata->clks[i]);

	mtk_mmdvfs_enable_vcp(false, VCP_PWR_USR_CAM);

	return 0;
}

static int mtk_raw_runtime_resume(struct device *dev)
{
	struct mtk_raw_device *drvdata = dev_get_drvdata(dev);
	int i, ret;
	unsigned int pr_detect_count;

	/* reset_msgfifo before enable_irq */
	ret = reset_msgfifo(drvdata);
	if (ret)
		return ret;

	enable_irq(drvdata->irq);

	pr_detect_count = get_detect_count();
	if (pr_detect_count < KERNEL_LOG_MAX)
		set_detect_count(KERNEL_LOG_MAX);

	dev_dbg(dev, "%s:enable clock\n", __func__);

	mtk_mmdvfs_enable_vcp(true, VCP_PWR_USR_CAM);
	for (i = 0; i < drvdata->num_clks; i++) {
		ret = clk_prepare_enable(drvdata->clks[i]);
		if (ret) {
			dev_info(dev, "enable failed at clk #%d, ret = %d\n",
				 i, ret);
			i--;
			while (i >= 0)
				clk_disable_unprepare(drvdata->clks[i--]);

			return ret;
		}
	}

	reset(drvdata);

	return 0;
}

static const struct dev_pm_ops mtk_raw_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_raw_runtime_suspend, mtk_raw_runtime_resume,
			   NULL)
};

static const struct of_device_id mtk_raw_of_ids[] = {
	{.compatible = "mediatek,cam-raw",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_raw_of_ids);

struct platform_driver mtk_cam_raw_driver = {
	.probe   = mtk_raw_probe,
	.remove  = mtk_raw_remove,
	.driver  = {
		.name  = "mtk-cam raw",
		.of_match_table = of_match_ptr(mtk_raw_of_ids),
		.pm     = &mtk_raw_pm_ops,
	}
};

static int mtk_yuv_component_bind(struct device *dev, struct device *master,
				  void *data)
{
	struct mtk_yuv_device *drvdata = dev_get_drvdata(dev);
	struct mtk_cam_device *cam_dev = data;

	dev_info(dev, "%s: id=%d\n", __func__, drvdata->id);
	drvdata->cam = cam_dev;
	return mtk_cam_set_dev_raw(cam_dev->dev, drvdata->id, NULL, dev, NULL);
}

static void mtk_yuv_component_unbind(struct device *dev, struct device *master,
				     void *data)
{
}

static const struct component_ops mtk_yuv_component_ops = {
	.bind = mtk_yuv_component_bind,
	.unbind = mtk_yuv_component_unbind,
};

static irqreturn_t mtk_irq_yuv(int irq, void *data)
{
	struct mtk_yuv_device *yuv = (struct mtk_yuv_device *)data;
	//struct device *dev = drvdata->dev;

	unsigned int irq_status, err_status, wdma_done_status, rdma_done_status;
	unsigned int drop_status, dma_ofl_status, dma_ufl_status;

	irq_status =
		readl_relaxed(yuv->base + REG_CAMCTL2_INT_STATUS);
	wdma_done_status =
		readl_relaxed(yuv->base + REG_CAMCTL2_INT2_STATUS);
	rdma_done_status =
		readl_relaxed(yuv->base + REG_CAMCTL2_INT3_STATUS);
	drop_status =
		readl_relaxed(yuv->base + REG_CAMCTL2_INT4_STATUS);
	dma_ofl_status =
		readl_relaxed(yuv->base + REG_CAMCTL2_INT5_STATUS);
	dma_ufl_status =
		readl_relaxed(yuv->base + REG_CAMCTL2_INT8_STATUS);

	err_status = irq_status & 0x4; // bit2: DMA_ERR

	//if (unlikely(debug_raw))
	if (CAM_DEBUG_ENABLED(RAW_INT))
		if (irq_status || err_status)
			dev_info(yuv->dev, "YUV-INT:0x%x(err:0x%x) INT2/4/5 0x%x/0x%x/0x%x\n",
			irq_status, err_status,
			wdma_done_status, drop_status, dma_ofl_status);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		if (err_status)
			dump_yuv_dma_err_st(yuv);

	/* trace */
	trace_yuv_irq(yuv->dev, irq_status, wdma_done_status, rdma_done_status);
	trace_raw_dma_status(yuv->dev, drop_status,
			     dma_ofl_status, dma_ufl_status);

	return IRQ_HANDLED;
}

static int mtk_yuv_pm_suspend_prepare(struct mtk_yuv_device *dev)
{
	int ret;

	if (pm_runtime_suspended(dev->dev))
		return 0;

	/* Force ISP HW to idle */
	ret = pm_runtime_force_suspend(dev->dev);
	return ret;
}

static int mtk_yuv_pm_post_suspend(struct mtk_yuv_device *dev)
{
	int ret;

	if (pm_runtime_suspended(dev->dev))
		return 0;

	/* Force ISP HW to resume */
	ret = pm_runtime_force_resume(dev->dev);
	if (ret)
		return ret;

	return 0;
}

static int yuv_pm_notifier(struct notifier_block *nb,
			   unsigned long action, void *data)
{
	struct mtk_yuv_device *yuv_dev =
			container_of(nb, struct mtk_yuv_device, pm_notifier);

	switch (action) {
	case PM_SUSPEND_PREPARE:
		mtk_yuv_pm_suspend_prepare(yuv_dev);
		break;
	case PM_POST_SUSPEND:
		mtk_yuv_pm_post_suspend(yuv_dev);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int mtk_yuv_of_probe(struct platform_device *pdev,
			    struct mtk_yuv_device *drvdata)
{
	struct device *dev = &pdev->dev;
	struct platform_device *larb_pdev;
	struct device_node *larb_node;
	struct device_link *link;
	struct of_phandle_args args;
	struct resource *res;
	unsigned int i;
	int clks, larbs, iommus, smmus, ret;

	ret = of_property_read_u32(dev->of_node, "mediatek,cam-id",
				   &drvdata->id);
	if (ret) {
		dev_dbg(dev, "missing camid property\n");
		return ret;
	}

	if (dma_set_mask_and_coherent(dev, DMA_BIT_MASK(34)))
		dev_info(dev, "%s: No suitable DMA available\n", __func__);

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

	/* base outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	drvdata->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(drvdata->base)) {
		dev_dbg(dev, "failed to map register base\n");
		return PTR_ERR(drvdata->base);
	}

	/* base inner register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	drvdata->base_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(drvdata->base_inner)) {
		dev_dbg(dev, "failed to map register inner base\n");
		return PTR_ERR(drvdata->base_inner);
	}

	drvdata->irq = platform_get_irq(pdev, 0);
	if (drvdata->irq < 0) {
		dev_dbg(dev, "failed to get irq\n");
		return -ENODEV;
	}

	ret = devm_request_irq(dev, drvdata->irq, mtk_irq_yuv,
			IRQF_NO_AUTOEN, dev_name(dev), drvdata);
	if (ret) {
		dev_dbg(dev, "failed to request irq=%d\n", drvdata->irq);
		return ret;
	}
	dev_dbg(dev, "registered irq=%d\n", drvdata->irq);

	clks = of_count_phandle_with_args(pdev->dev.of_node, "clocks",
			"#clock-cells");

	drvdata->num_clks  = (clks == -ENOENT) ? 0:clks;
	dev_info(dev, "clk_num:%d\n", drvdata->num_clks);

	if (drvdata->num_clks) {
		drvdata->clks = devm_kcalloc(dev,
					     drvdata->num_clks, sizeof(*drvdata->clks),
					     GFP_KERNEL);
		if (!drvdata->clks)
			return -ENOMEM;
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
	drvdata->num_larbs = (larbs < 0) ? 0 : larbs;
	dev_info(dev, "larb_num:%d\n", larbs);

	if (drvdata->num_larbs) {
		drvdata->larbs = devm_kcalloc(dev,
					     drvdata->num_larbs, sizeof(*drvdata->larbs),
					     GFP_KERNEL);
		if (!drvdata->larbs)
			return -ENOMEM;
	}

	for (i = 0; i < drvdata->num_larbs; i++) {
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

		link = device_link_add(dev, &larb_pdev->dev,
						DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
		if (!link)
			dev_info(dev, "unable to link smi larb%d\n", i);
		else
			drvdata->larbs[i] = larb_pdev;
	}

#ifdef CONFIG_PM_SLEEP
	drvdata->pm_notifier.notifier_call = yuv_pm_notifier;
	ret = register_pm_notifier(&drvdata->pm_notifier);
	if (ret) {
		dev_info(dev, "failed to register notifier block.\n");
		return ret;
	}
#endif

	iommus = of_count_phandle_with_args(
		pdev->dev.of_node, "iommus", "#iommu-cells");
	iommus = (iommus > 0) ? iommus : 0;
	for (i = 0; i < iommus; i++) {
		if (!of_parse_phandle_with_args(pdev->dev.of_node,
			"iommus", "#iommu-cells", i, &args)) {
			mtk_iommu_register_fault_callback(
				args.args[0], mtk_yuv_translation_fault_cb,
				(void *)drvdata, false);
		}
	}

	smmus = of_property_count_u32_elems(
		pdev->dev.of_node, "mediatek,smmu-dma-axid");
	smmus = (smmus > 0) ? smmus : 0;
	for (i = 0; i < smmus; i++) {
		u32 axid;

		if (!of_property_read_u32_index(
			pdev->dev.of_node, "mediatek,smmu-dma-axid", i, &axid)) {
			mtk_iommu_register_fault_callback(
				axid, mtk_yuv_translation_fault_cb,
				(void *)drvdata, false);
		}
	}

	return 0;
}

static int mtk_yuv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_yuv_device *drvdata;
	int ret;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->dev = dev;
	dev_set_drvdata(dev, drvdata);

	ret = mtk_yuv_of_probe(pdev, drvdata);
	if (ret) {
		dev_info(dev, "mtk_yuv_of_probe failed\n");
		return ret;
	}

	ret = mtk_cam_qos_probe(dev, &drvdata->qos,
				GET_PLAT_HW(yuv_icc_path_num));
	if (ret)
		goto UNREGISTER_PM_NOTIFIER;

	pm_runtime_enable(dev);

	ret = component_add(dev, &mtk_yuv_component_ops);
	if (ret)
		goto UNREGISTER_PM_NOTIFIER;

	return ret;

UNREGISTER_PM_NOTIFIER:
	unregister_pm_notifier(&drvdata->pm_notifier);
	return ret;
}

static int mtk_yuv_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_yuv_device *drvdata = dev_get_drvdata(dev);
	int i;

	unregister_pm_notifier(&drvdata->pm_notifier);

	pm_runtime_disable(dev);
	mtk_cam_qos_remove(&drvdata->qos);
	component_del(dev, &mtk_yuv_component_ops);

	for (i = 0; i < drvdata->num_clks; i++)
		clk_put(drvdata->clks[i]);

	return 0;
}

/* driver for yuv part */
static int mtk_yuv_runtime_suspend(struct device *dev)
{
	struct mtk_yuv_device *drvdata = dev_get_drvdata(dev);
	int i;

	dev_dbg(dev, "%s:disable clock\n", __func__);

	mtk_cam_reset_qos(dev, &drvdata->qos);

	for (i = 0; i < drvdata->num_clks; i++)
		clk_disable_unprepare(drvdata->clks[i]);

	return 0;
}

static int mtk_yuv_runtime_resume(struct device *dev)
{
	struct mtk_yuv_device *drvdata = dev_get_drvdata(dev);
	int i, ret;

	dev_dbg(dev, "%s:enable clock\n", __func__);

	enable_irq(drvdata->irq);

	for (i = 0; i < drvdata->num_clks; i++) {
		ret = clk_prepare_enable(drvdata->clks[i]);
		if (ret) {
			dev_info(dev, "enable failed at clk #%d, ret = %d\n",
				 i, ret);
			i--;
			while (i >= 0)
				clk_disable_unprepare(drvdata->clks[i--]);

			return ret;
		}
	}

	return 0;
}

void print_cq_settings(void __iomem *base)
{
	unsigned int inner_addr, inner_addr_msb, size;

	inner_addr = readl_relaxed(base + REG_CAMCQ_CQ_THR0_BASEADDR);
	inner_addr_msb = readl_relaxed(base + REG_CAMCQ_CQ_THR0_BASEADDR_MSB);
	size = readl_relaxed(base + REG_CAMCQ_CQ_THR0_DESC_SIZE);

	pr_info("CQ_THR0_inner_addr_msb:0x%x, CQ_THR0_inner_addr:%08x, size:0x%x\n",
		inner_addr_msb, inner_addr, size);

	inner_addr = readl_relaxed(base + REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2);
	inner_addr_msb = readl_relaxed(base + REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2_MSB);
	size = readl_relaxed(base + REG_CAMCQ_CQ_SUB_THR0_DESC_SIZE_2);

	pr_info("CQ_SUB_THR0_2: inner_addr_msb:0x%x, inner_addr:%08x, size:0x%x\n",
		inner_addr_msb, inner_addr, size);
}

void print_dma_settings(void __iomem *base, u32 dmao_base)
{
#define REG_MSB          0x4
#define REG_OFFSET       0x8
#define REG_OFFSET_MSB   0xC
#define REG_XSIZE        0x10
#define REG_YSIZE        0x14
#define REG_STRIDE       0x18

	pr_info("0x%x: addr:0x%08x, addr_msb:%08x, addr_offset:0x%08x, addr_msb_offset:%08x, xsize:0x%08x, ysize:0x%08x, stride:0x%08x\n",
		dmao_base,
		readl_relaxed(base + dmao_base),
		readl_relaxed(base + dmao_base + REG_MSB),
		readl_relaxed(base + dmao_base + REG_OFFSET),
		readl_relaxed(base + dmao_base + REG_OFFSET_MSB),
		readl_relaxed(base + dmao_base + REG_XSIZE),
		readl_relaxed(base + dmao_base + REG_YSIZE),
		readl_relaxed(base + dmao_base + REG_STRIDE));
}

#define CAMSYS_DMA_GROUP_SIZE 4
int mtk_raw_translation_fault_cb(int port, dma_addr_t mva, void *data)
{
	struct mtk_raw_device *raw_dev = (struct mtk_raw_device *)data;
	struct mtk_yuv_device *yuv_dev = get_yuv_dev(raw_dev);
	unsigned int fh_cookie =
			readl_relaxed(raw_dev->base_inner + REG_FRAME_SEQ_NUM);
	unsigned int m4u_port = MTK_M4U_TO_PORT(port);
	u32 group[CAMSYS_DMA_GROUP_SIZE];
	int i;

	if (m4u_port == 0) { /* cq info */
		print_cq_settings(raw_dev->base_inner);
	}

	if (CALL_PLAT_HW(query_raw_dma_group, m4u_port, group))
		return 0;

	for (i = 0; i < CAMSYS_DMA_GROUP_SIZE; i++) {
		if (group[i] == 0x0)
			continue;

		print_dma_settings(raw_dev->base_inner, group[i]);
	}
	dump_raw_dma_fbc(raw_dev);
	dump_yuv_dma_fbc(yuv_dev);

	do_engine_callback(raw_dev->engine_cb, dump_request,
		   raw_dev->cam, CAMSYS_ENGINE_RAW, raw_dev->id,
		   fh_cookie, MSG_M4U_TF);

	return 0;
}

int mtk_yuv_translation_fault_cb(int port, dma_addr_t mva, void *data)
{
	struct mtk_yuv_device *yuv_dev = (struct mtk_yuv_device *)data;
	struct mtk_raw_device *raw_dev = get_raw_dev(yuv_dev);
	unsigned int fh_cookie =
			readl_relaxed(raw_dev->base_inner + REG_FRAME_SEQ_NUM);
	unsigned int m4u_port = MTK_M4U_TO_PORT(port);
	u32 group[CAMSYS_DMA_GROUP_SIZE];
	int i;

	if (CALL_PLAT_HW(query_yuv_dma_group, m4u_port, group))
		return 0;

	for (i = 0; i < CAMSYS_DMA_GROUP_SIZE; i++) {
		if (group[i] == 0x0)
			continue;

		print_dma_settings(yuv_dev->base_inner, group[i]);
	}
	dump_raw_dma_fbc(raw_dev);
	dump_yuv_dma_fbc(yuv_dev);

	do_engine_callback(raw_dev->engine_cb, dump_request,
		   raw_dev->cam, CAMSYS_ENGINE_RAW, raw_dev->id,
		   fh_cookie, MSG_M4U_TF);

	return 0;
}

void fill_aa_info(struct mtk_raw_device *raw_dev,
				  struct mtk_ae_debug_data *ae_info)
{
	ae_info->OBC_R1_Sum[0] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R1_R_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R1_R_SUM_L);
	ae_info->OBC_R2_Sum[0] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R2_R_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R2_R_SUM_L);
	ae_info->OBC_R3_Sum[0] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R3_R_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R3_R_SUM_L);
	ae_info->LTM_Sum[0] +=
		((u64)readl(raw_dev->base + REG_LTM_AE_DEBUG_R_MSB) << 32) |
		readl(raw_dev->base + REG_LTM_AE_DEBUG_R_LSB);
	ae_info->AA_Sum[0] +=
		((u64)readl(raw_dev->base + REG_AA_R_SUM_H) << 32) |
		readl(raw_dev->base + REG_AA_R_SUM_L);

	ae_info->OBC_R1_Sum[1] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R1_B_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R1_B_SUM_L);
	ae_info->OBC_R2_Sum[1] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R2_B_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R2_B_SUM_L);
	ae_info->OBC_R3_Sum[1] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R3_B_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R3_B_SUM_L);
	ae_info->LTM_Sum[1] +=
		((u64)readl(raw_dev->base + REG_LTM_AE_DEBUG_B_MSB) << 32) |
		readl(raw_dev->base + REG_LTM_AE_DEBUG_B_LSB);
	ae_info->AA_Sum[1] +=
		((u64)readl(raw_dev->base + REG_AA_B_SUM_H) << 32) |
		readl(raw_dev->base + REG_AA_B_SUM_L);

	ae_info->OBC_R1_Sum[2] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R1_GR_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R1_GR_SUM_L);
	ae_info->OBC_R2_Sum[2] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R2_GR_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R2_GR_SUM_L);
	ae_info->OBC_R3_Sum[2] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R3_GR_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R3_GR_SUM_L);
	ae_info->LTM_Sum[2] +=
		((u64)readl(raw_dev->base + REG_LTM_AE_DEBUG_GR_MSB) << 32) |
		readl(raw_dev->base + REG_LTM_AE_DEBUG_GR_LSB);
	ae_info->AA_Sum[2] +=
		((u64)readl(raw_dev->base + REG_AA_GR_SUM_H) << 32) |
		readl(raw_dev->base + REG_AA_GR_SUM_L);

	ae_info->OBC_R1_Sum[3] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R1_GB_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R1_GB_SUM_L);
	ae_info->OBC_R2_Sum[3] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R2_GB_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R2_GB_SUM_L);
	ae_info->OBC_R3_Sum[3] +=
		((u64)readl(raw_dev->base + OFFSET_OBC_R3_GB_SUM_H) << 32) |
		readl(raw_dev->base + OFFSET_OBC_R3_GB_SUM_L);
	ae_info->LTM_Sum[3] +=
		((u64)readl(raw_dev->base + REG_LTM_AE_DEBUG_GB_MSB) << 32) |
		readl(raw_dev->base + REG_LTM_AE_DEBUG_GB_LSB);
	ae_info->AA_Sum[3] +=
		((u64)readl(raw_dev->base + REG_AA_GB_SUM_H) << 32) |
		readl(raw_dev->base + REG_AA_GB_SUM_L);
}

static const struct dev_pm_ops mtk_yuv_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_yuv_runtime_suspend, mtk_yuv_runtime_resume,
			   NULL)
};

static const struct of_device_id mtk_yuv_of_ids[] = {
	{.compatible = "mediatek,cam-yuv",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_yuv_of_ids);

struct platform_driver mtk_cam_yuv_driver = {
	.probe   = mtk_yuv_probe,
	.remove  = mtk_yuv_remove,
	.driver  = {
		.name  = "mtk-cam yuv",
		.of_match_table = of_match_ptr(mtk_yuv_of_ids),
		.pm     = &mtk_yuv_pm_ops,
	}
};

/* rms partition */

static int mtk_rms_component_bind(struct device *dev, struct device *master,
				  void *data)
{
	struct mtk_rms_device *drvdata = dev_get_drvdata(dev);
	struct mtk_cam_device *cam_dev = data;

	dev_info(dev, "%s: id=%d\n", __func__, drvdata->id);
	return mtk_cam_set_dev_raw(cam_dev->dev, drvdata->id, NULL, NULL, dev);
}

static void mtk_rms_component_unbind(struct device *dev, struct device *master,
				     void *data)
{
}

static const struct component_ops mtk_rms_component_ops = {
	.bind = mtk_rms_component_bind,
	.unbind = mtk_rms_component_unbind,
};

static int mtk_rms_pm_suspend_prepare(struct mtk_rms_device *dev)
{
	int ret;

	if (pm_runtime_suspended(dev->dev))
		return 0;

	/* Force ISP HW to idle */
	ret = pm_runtime_force_suspend(dev->dev);
	return ret;
}

static int mtk_rms_pm_post_suspend(struct mtk_rms_device *dev)
{
	int ret;

	if (pm_runtime_suspended(dev->dev))
		return 0;

	/* Force ISP HW to resume */
	ret = pm_runtime_force_resume(dev->dev);
	if (ret)
		return ret;

	return 0;
}

static int rms_pm_notifier(struct notifier_block *nb,
			   unsigned long action, void *data)
{
	struct mtk_rms_device *rms_dev =
			container_of(nb, struct mtk_rms_device, pm_notifier);

	switch (action) {
	case PM_SUSPEND_PREPARE:
		mtk_rms_pm_suspend_prepare(rms_dev);
		break;
	case PM_POST_SUSPEND:
		mtk_rms_pm_post_suspend(rms_dev);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int mtk_rms_of_probe(struct platform_device *pdev,
			    struct mtk_rms_device *drvdata)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	unsigned int i;
	int clks, ret;

	ret = of_property_read_u32(dev->of_node, "mediatek,cam-id",
				   &drvdata->id);
	if (ret) {
		dev_dbg(dev, "missing camid property\n");
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
		dev_dbg(dev, "failed to map register base\n");
		return PTR_ERR(drvdata->base);
	}

	/* base inner register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	drvdata->base_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(drvdata->base_inner)) {
		dev_dbg(dev, "failed to map register inner base\n");
		return PTR_ERR(drvdata->base_inner);
	}

	clks = of_count_phandle_with_args(pdev->dev.of_node, "clocks",
			"#clock-cells");

	drvdata->num_clks = (clks == -ENOENT) ? 0 : clks;
	dev_info(dev, "clk_num:%d\n", drvdata->num_clks);

	if (drvdata->num_clks) {
		drvdata->clks = devm_kcalloc(dev,
					     drvdata->num_clks, sizeof(*drvdata->clks),
					     GFP_KERNEL);
		if (!drvdata->clks)
			return -ENOMEM;
	}

	for (i = 0; i < drvdata->num_clks; i++) {
		drvdata->clks[i] = of_clk_get(pdev->dev.of_node, i);
		if (IS_ERR(drvdata->clks[i])) {
			dev_info(dev, "failed to get clk %d\n", i);
			return -ENODEV;
		}
	}

#ifdef CONFIG_PM_SLEEP
	drvdata->pm_notifier.notifier_call = rms_pm_notifier;
	ret = register_pm_notifier(&drvdata->pm_notifier);
	if (ret) {
		dev_info(dev, "failed to register notifier block.\n");
		return ret;
	}
#endif

	return 0;
}

static int mtk_rms_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_rms_device *drvdata;
	int ret;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->dev = dev;
	dev_set_drvdata(dev, drvdata);

	ret = mtk_rms_of_probe(pdev, drvdata);
	if (ret) {
		dev_info(dev, "mtk_rms_of_probe failed\n");
		return ret;
	}

	pm_runtime_enable(dev);

	ret = component_add(dev, &mtk_rms_component_ops);
	if (ret)
		goto UNREGISTER_PM_NOTIFIER;

	return ret;

UNREGISTER_PM_NOTIFIER:
	unregister_pm_notifier(&drvdata->pm_notifier);
	return ret;
}

static int mtk_rms_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_rms_device *drvdata = dev_get_drvdata(dev);
	int i;

	unregister_pm_notifier(&drvdata->pm_notifier);

	pm_runtime_disable(dev);

	component_del(dev, &mtk_rms_component_ops);

	for (i = 0; i < drvdata->num_clks; i++)
		clk_put(drvdata->clks[i]);

	return 0;
}

static int mtk_rms_runtime_suspend(struct device *dev)
{
	struct mtk_rms_device *drvdata = dev_get_drvdata(dev);
	int i;

	dev_dbg(dev, "%s:disable clock\n", __func__);

	for (i = 0; i < drvdata->num_clks; i++)
		clk_disable_unprepare(drvdata->clks[i]);

	return 0;
}

static int mtk_rms_runtime_resume(struct device *dev)
{
	struct mtk_rms_device *drvdata = dev_get_drvdata(dev);
	int i, ret;

	dev_dbg(dev, "%s:enable clock\n", __func__);

	for (i = 0; i < drvdata->num_clks; i++) {
		ret = clk_prepare_enable(drvdata->clks[i]);
		if (ret) {
			dev_info(dev, "enable failed at clk #%d, ret = %d\n",
				 i, ret);
			i--;
			while (i >= 0)
				clk_disable_unprepare(drvdata->clks[i--]);

			return ret;
		}
	}

	return 0;
}
static const struct dev_pm_ops mtk_rms_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_rms_runtime_suspend, mtk_rms_runtime_resume,
			   NULL)
};

static const struct of_device_id mtk_rms_of_ids[] = {
	{.compatible = "mediatek,cam-rms",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_rms_of_ids);

struct platform_driver mtk_cam_rms_driver = {
	.probe   = mtk_rms_probe,
	.remove  = mtk_rms_remove,
	.driver  = {
		.name  = "mtk-cam rms",
		.of_match_table = of_match_ptr(mtk_rms_of_ids),
		.pm     = &mtk_rms_pm_ops,
	}
};


int raw_to_tg_idx(int raw_id)
{
	int cammux_id_raw_start = GET_PLAT_HW(cammux_id_raw_start);

	return raw_id * 2 + cammux_id_raw_start;
}

//#define DEBUG_RAWI_R5
void raw_dump_debug_status(struct mtk_raw_device *dev, bool is_srt)
{
	dump_seqence(dev);
	dump_cq_setting(dev);
	dump_tg_setting(dev, "debug");
	dump_dmatop_dc_st(dev);
	dump_interrupt(dev);

	if (is_srt)
		dump_topdebug_rdyreq(dev);

#ifdef DEBUG_RAWI_R5
	mtk_cam_dump_dma_debug(dev,
			       dev->base + 0x4000, /* DMATOP_BASE */
			       "RAWI_R5",
			       dbg_RAWI_R5, ARRAY_SIZE(dbg_RAWI_R5));
#endif
}
