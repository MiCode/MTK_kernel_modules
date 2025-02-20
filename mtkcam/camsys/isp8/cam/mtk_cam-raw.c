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
#include <linux/sched/clock.h>
#include <linux/rtc.h>

#include <mtk_printk_ctrl.h>

#include <soc/mediatek/smi.h>
#include <soc/mediatek/mmdvfs_v3.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>

#include "mtk_cam.h"
#include "mtk_cam-debug.h"
#include "mtk_cam-dvfs_qos.h"
#include "mtk_cam-raw.h"
#include "mtk_cam-qof.h"
#include "mtk_cam-raw_debug.h"
#include "mtk_cam-dmadbg.h"
#include "mtk_cam-raw_regs.h"
#include "mtk_cam-reg_utils.h"
//#include "mtk_cam-hsf.h"
#include "mtk_cam-trace.h"
#include "iommu_debug.h"
#include "mmqos-mtk.h"
#include "mtk-smi-dbg.h"
#include "mtk-mmdvfs-debug.h"

//static int debug_dump_fbc;
//module_param(debug_dump_fbc, int, 0644);
//MODULE_PARM_DESC(debug_dump_fbc, "debug: dump fbc");

static int debug_ddren_sw_mode;
module_param(debug_ddren_sw_mode, int, 0644);
MODULE_PARM_DESC(debug_ddren_sw_mode, "debug: 1 : active sw mode");

#define MTK_RAW_STOP_HW_TIMEOUT			(33)

#define KERNEL_LOG_MAX	                400

#define RAW_DEBUG 0
#define AEO_SW_WORKAROUND 1

#define FIFO_THRESHOLD(FIFO_SIZE, HEIGHT_RATIO, LOW_RATIO) \
	(((FIFO_SIZE * HEIGHT_RATIO) & 0xFFF) << 16 | \
	((FIFO_SIZE * LOW_RATIO) & 0xFFF))

#define raw_readl(raw, base, off) \
({\
		raw->io_ops->readl(raw, base, off); \
})

#define raw_readl_relaxed(raw, base, off) \
({\
		raw->io_ops->readl_relaxed(raw, base, off); \
})

#define raw_writel(val, raw, base, off) \
({\
	raw->io_ops->writel(raw, val, base, off); \
})

#define raw_writel_relaxed(val, raw, base, off) \
({\
	raw->io_ops->writel_relaxed(raw, val, base, off); \
})

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

static struct mtk_rms_device *get_rms_dev(struct mtk_raw_device *raw_dev)
{
	struct device *dev;
	struct mtk_cam_device *cam = raw_dev->cam;

	dev = cam->engines.rms_devs[raw_dev->id];

	return dev_get_drvdata(dev);
}

static struct mtk_raw_device *get_raw_dev(struct mtk_yuv_device *yuv_dev)
{
	struct device *dev;
	struct mtk_cam_device *cam = yuv_dev->cam;

	dev = cam->engines.raw_devs[yuv_dev->id];

	return dev_get_drvdata(dev);
}

static void init_raw_ddren(struct mtk_raw_device *dev, int is_srt, int frm_time_us)
{
	int val = 0;

	if (debug_ddren_sw_mode) {
		SET_FIELD(&val, CAMCTL_DDREN_SW_SET, 1);
		raw_writel(val, dev, dev->base, REG_CAMCTL_DDREN_CTL);
	} else {
		SET_FIELD(&val, CAMCTL_DDREN_HW_EN, 1);
		raw_writel(val, dev, dev->base, REG_CAMCTL_DDREN_CTL);

		/* hrt ddren timer for master */
		if (!dev->is_slave && !is_srt)
			qof_ddren_setting(dev, frm_time_us);
	}
	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev, "ddren_sw_mode:%d\n", debug_ddren_sw_mode);
}

void init_camsys_settings(struct mtk_raw_device *dev, bool is_srt, int frm_time_us)
{
	struct mtk_cam_device *cam_dev = dev->cam;
	struct mtk_yuv_device *yuv_dev = get_yuv_dev(dev);
	unsigned int reg_raw_urgent, reg_yuv_urgent;
	unsigned int raw_urgent, yuv_urgent;

	init_raw_ddren(dev, is_srt, frm_time_us);

	//Set rdy/req snapshot
	// TODO: QOF io ops?
	set_topdebug_rdyreq(dev, is_srt ? ALL_THE_TIME : TG_OVERRUN);

	//Set CQI sram size
	// TODO: QOF io ops?
	set_fifo_threshold(dev->dmatop_base + REG_CQI_R1_BASE, 64);
	set_fifo_threshold(dev->dmatop_base + REG_CQI_R2_BASE, 64);
	set_fifo_threshold(dev->dmatop_base + REG_CQI_R3_BASE, 64);
	set_fifo_threshold(dev->dmatop_base + REG_CQI_R4_BASE, 64);
	set_fifo_threshold(dev->dmatop_base + REG_CQI_R5_BASE, 64);
	set_fifo_threshold(dev->dmatop_base + REG_CQI_R6_BASE, 64);
	set_fifo_threshold(dev->dmatop_base + REG_CQI_R7_BASE, 64);
	set_fifo_threshold(dev->dmatop_base + REG_CQI_R8_BASE, 64);

	// TODO: move HALT1,2,3,4,13 to camsv/mraw
	writel_relaxed(HALT1_EN, cam_dev->base + REG_HALT1_EN);
	writel_relaxed(HALT2_EN, cam_dev->base + REG_HALT2_EN);
	writel_relaxed(HALT3_EN, cam_dev->base + REG_HALT3_EN);
	writel_relaxed(HALT4_EN, cam_dev->base + REG_HALT4_EN);
	writel_relaxed(HALT13_EN, cam_dev->base + REG_HALT13_EN);

	//Disable low latency
	/*
	raw_writel_relaxed(0xffff,
		dev, dev->dmatop_base, REG_CAMRAWDMATOP_LOW_LATENCY_LINE_CNT_IMGO_R1);
	raw_writel_relaxed(0xffff,
		dev, yuv_dev->dmatop_base, REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_YUVO_R1);
	raw_writel_relaxed(0xffff,
		dev, yuv_dev->dmatop_base, REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_YUVO_R3);
	raw_writel_relaxed(0xffff,
		dev, yuv_dev->dmatop_base, REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_DRZS4NO_R1);
	*/
#ifdef DEBUG_DMA_ENABLE_CRC_EN
	/* for debug: crc_en */
	raw_writel(BIT(24), dev, dev->base_dmatop, REG_CAMRAWDMATOP_DMA_DBG_SEL);
	raw_writel(BIT(24), dev, dev->base_dmatop, REG_CAMYUVDMATOP_DMA_DBG_SEL);
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
		if (dev->larb_vcsel)
			writel_relaxed(0x0, dev->larb_vcsel);
		if (yuv_dev->larb_vcsel)
			writel_relaxed(0x0, yuv_dev->larb_vcsel);
	} else {
		writel_relaxed(raw_urgent, cam_dev->base + reg_raw_urgent);
		writel_relaxed(yuv_urgent, cam_dev->base + reg_yuv_urgent);
		if (dev->larb_vcsel)
			writel_relaxed(0x7ffff, dev->larb_vcsel);
		if (yuv_dev->larb_vcsel)
			writel_relaxed(0x7f, yuv_dev->larb_vcsel);
	}

	wmb(); /* TBC */

	dev_info_ratelimited(dev->dev, "%s: is srt:%d halt1~10,13:0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, is_srt,
		readl(cam_dev->base + REG_HALT1_EN), readl(cam_dev->base + REG_HALT2_EN),
		readl(cam_dev->base + REG_HALT3_EN), readl(cam_dev->base + REG_HALT4_EN),
		readl(cam_dev->base + REG_HALT5_EN), readl(cam_dev->base + REG_HALT6_EN),
		readl(cam_dev->base + REG_HALT7_EN), readl(cam_dev->base + REG_HALT8_EN),
		readl(cam_dev->base + REG_HALT9_EN), readl(cam_dev->base + REG_HALT10_EN),
		readl(cam_dev->base + REG_HALT13_EN));
}

#define BPC_R2_PCRP				0x41EC
#define CBM_R1_PCRP				0x1018

void diable_rms_pcrp(struct mtk_raw_device *raw)
{
	struct mtk_rms_device *rms = get_rms_dev(raw);

	basic_writel(raw, 0x0, rms->base, BPC_R2_PCRP);
	basic_writel(raw, 0x0, rms->base, CBM_R1_PCRP);
}

void diable_rms_module(struct mtk_raw_device *raw)
{
	struct mtk_rms_device *rms = get_rms_dev(raw);

	basic_writel(raw, 0x0, rms->base, REG_CAMCTL3_MOD_EN);
	basic_writel(raw, 0x0, rms->base, REG_CAMCTL3_MOD2_EN);
	basic_writel(raw, 0x0, rms->base, REG_CAMCTL3_MOD3_EN);
	basic_writel(raw, 0x0, rms->base, REG_CAMCTL3_MOD4_EN);
	basic_writel(raw, 0x0, rms->base, REG_CAMCTL3_MOD5_EN);
	basic_writel(raw, 0x0, rms->base, REG_CAMCTL3_MOD6_EN);
}
static void dump_dmai_reg(struct mtk_raw_device *dev)
{
	u32 caci_base, caci_base_m, caci_oft, caci_oft_m, caci_xsize, caci_ysize, caci_stride;
	u32 rawi5_base, rawi5_base_m, rawi5_oft, rawi5_oft_m, rawi5_xsize, rawi5_ysize, rawi5_stride;

	/* caci r1 */
	caci_base = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0bc0);
	caci_base_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0bc4);
	caci_oft = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0bc8);
	caci_oft_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0bcc);
	caci_xsize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0bd0);
	caci_ysize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0bd4);
	caci_stride = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0bd8);

	dev_info(dev->dev,
		"[%s] raw%d - caci [in] 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, caci_base, caci_base_m, caci_oft,
		caci_oft_m, caci_xsize, caci_ysize, caci_stride);
	/* bpci r3 */
	caci_base = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0980);
	caci_base_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0984);
	caci_oft = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0988);
	caci_oft_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x098c);
	caci_xsize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0990);
	caci_ysize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0994);
	caci_stride = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0998);

	dev_info(dev->dev,
		"[%s] raw%d - bpci3 [in] 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, caci_base, caci_base_m, caci_oft,
		caci_oft_m, caci_xsize, caci_ysize, caci_stride);
	/* bpci r4 */
	caci_base = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x09c0);
	caci_base_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x09c4);
	caci_oft = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x09c8);
	caci_oft_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x09cc);
	caci_xsize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x09d0);
	caci_ysize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x09d4);
	caci_stride = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x09d8);

	dev_info(dev->dev,
		"[%s] raw%d - bpci4 [in] 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, caci_base, caci_base_m, caci_oft,
		caci_oft_m, caci_xsize, caci_ysize, caci_stride);
	/* pdi r1 */
	caci_base = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0ac0);
	caci_base_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0ac4);
	caci_oft = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0ac8);
	caci_oft_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0acc);
	caci_xsize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0ad0);
	caci_ysize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0ad4);
	caci_stride = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0ad8);

	dev_info(dev->dev,
		"[%s] raw%d - pdi [in] 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, caci_base, caci_base_m, caci_oft,
		caci_oft_m, caci_xsize, caci_ysize, caci_stride);
	/* rawi r5 */
	rawi5_base = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0820);
	rawi5_base_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0824);
	rawi5_oft = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0828);
	rawi5_oft_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x082c);
	rawi5_xsize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0830);
	rawi5_ysize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0834);
	rawi5_stride = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0838);

	dev_info(dev->dev,
		"[%s] raw%d - rawi5 [in] 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, rawi5_base, rawi5_base_m, rawi5_oft,
		rawi5_oft_m, rawi5_xsize, rawi5_ysize, rawi5_stride);
	/* ufdi r5 */
	rawi5_base = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0890);
	rawi5_base_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0894);
	rawi5_oft = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x0898);
	rawi5_oft_m = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x089c);
	rawi5_xsize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x08a0);
	rawi5_ysize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x08a4);
	rawi5_stride = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x08a8);

	dev_info(dev->dev,
		"[%s] raw%d - ufdi5 [in] 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, rawi5_base, rawi5_base_m, rawi5_oft,
		rawi5_oft_m, rawi5_xsize, rawi5_ysize, rawi5_stride);

}

static void dump_rms_reg(struct mtk_raw_device *dev)
{
	struct mtk_rms_device *rms = get_rms_dev(dev);
	u32 rms_en, rms_en2, rms_en3, rms_en4, rms_en5, rms_en6;
	u32 bpc_r2_pcrop, cbm_r1_pcrop;

	rms_en = raw_readl_relaxed(dev, rms->base_inner, REG_CAMCTL3_MOD_EN);
	rms_en2 = raw_readl_relaxed(dev, rms->base_inner, REG_CAMCTL3_MOD2_EN);
	rms_en3 = raw_readl_relaxed(dev, rms->base_inner, REG_CAMCTL3_MOD3_EN);
	rms_en4 = raw_readl_relaxed(dev, rms->base_inner, REG_CAMCTL3_MOD4_EN);
	rms_en5 = raw_readl_relaxed(dev, rms->base_inner, REG_CAMCTL3_MOD5_EN);
	rms_en6 = raw_readl_relaxed(dev, rms->base_inner, REG_CAMCTL3_MOD6_EN);
	bpc_r2_pcrop = raw_readl_relaxed(dev, rms->base_inner, BPC_R2_PCRP);
	cbm_r1_pcrop = raw_readl_relaxed(dev, rms->base_inner, CBM_R1_PCRP);

	dev_info(dev->dev,
		"[%s] raw%d - [in] rms_en/2/3/4/5/6:0x%x/0x%x/0x%x/0x%x/0x%x/0x%x, 0x%x/0x%x\n",
		__func__, dev->id, rms_en, rms_en2, rms_en3,
		rms_en4, rms_en5, rms_en6,
		bpc_r2_pcrop, cbm_r1_pcrop);
}
static void init_ADLWR_settings(struct mtk_cam_device *cam)
{
	if (IS_ERR_OR_NULL(cam->adlwr_base)) {
		if (CAM_DEBUG_ENABLED(JOB))
			dev_info(cam->dev, "%s: skipped\n", __func__);
		return;
	}
	/* CAMADLWR_CAMADLWR_ADL_CTRL_FIELD_ID_GROUP_2 */
	writel_relaxed(0x440, cam->adlwr_base + 0x350);
}
static void dump_ae_reg(struct mtk_raw_device *dev, bool force)
{
	u32 ae_stat_en, ae_win_org, ae_win_size, ae_win_pit, ae_win_num;
	u32 qbn_r1_ctl, qbn_r1_pcrp_ctl, pcrp0_xpos, pcrp0_ypos, pcrp1_xpos, pcrp1_ypos;

	ae_stat_en = raw_readl_relaxed(dev, dev->base_inner, 0x5840);
	ae_win_org = raw_readl_relaxed(dev, dev->base_inner, 0x5848);
	ae_win_size = raw_readl_relaxed(dev, dev->base_inner, 0x584c);
	ae_win_pit = raw_readl_relaxed(dev, dev->base_inner, 0x5850);
	ae_win_num = raw_readl_relaxed(dev, dev->base_inner, 0x5854);
	qbn_r1_ctl = raw_readl_relaxed(dev, dev->base_inner, 0x5800);
	qbn_r1_pcrp_ctl = raw_readl_relaxed(dev, dev->base_inner, 0x5804);
	pcrp0_xpos = raw_readl_relaxed(dev, dev->base_inner, 0x5808);
	pcrp0_ypos = raw_readl_relaxed(dev, dev->base_inner, 0x580c);
	pcrp1_xpos = raw_readl_relaxed(dev, dev->base_inner, 0x5810);
	pcrp1_ypos = raw_readl_relaxed(dev, dev->base_inner, 0x5814);
	if (CAM_DEBUG_ENABLED(RAW_INT) || force)
		dev_info(dev->dev,
		"[%s] raw%d - [in] ae_stat_en/ae_win_org/ae_win_size/ae_win_pit/ae_win_num:0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, ae_stat_en, ae_win_org,
		ae_win_size, ae_win_pit, ae_win_num);
	if (CAM_DEBUG_ENABLED(RAW_INT) || force)
		dev_info(dev->dev,
		"[%s] raw%d - [in] qbn_r1_ctl/qbn_r1_pcrp_ctl/pcrp0_xpos/pcrp0_ypos/pcrp1_xpos/pcrp1_ypos:0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, qbn_r1_ctl, qbn_r1_pcrp_ctl,
		pcrp0_xpos, pcrp0_ypos, pcrp1_xpos, pcrp1_ypos);
	qbn_r1_ctl = raw_readl_relaxed(dev, dev->base_inner, 0x5c80);
	qbn_r1_pcrp_ctl = raw_readl_relaxed(dev, dev->base_inner, 0x5c84);
	pcrp0_xpos = raw_readl_relaxed(dev, dev->base_inner, 0x5c88);
	pcrp0_ypos = raw_readl_relaxed(dev, dev->base_inner, 0x5c8c);
	pcrp1_xpos = raw_readl_relaxed(dev, dev->base_inner, 0x5c90);
	pcrp1_ypos = raw_readl_relaxed(dev, dev->base_inner, 0x5c94);
	if (CAM_DEBUG_ENABLED(RAW_INT) || force)
		dev_info(dev->dev,
		"[%s] raw%d - [in] qbn_r9_ctl/qbn_r9_pcrp_ctl/pcrp0_xpos/pcrp0_ypos/pcrp1_xpos/pcrp1_ypos:0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, qbn_r1_ctl, qbn_r1_pcrp_ctl,
		pcrp0_xpos, pcrp0_ypos, pcrp1_xpos, pcrp1_ypos);
}

static void dump_awb_reg(struct mtk_raw_device *dev, bool force)
{
	u32 awb_stat_en, awb_win_org, awb_win_size, awb_win_pit, awb_win_num;
	u32 qbn_r8_ctl, qbn_r8_pcrp_ctl, pcrp0_xpos, pcrp0_ypos, pcrp1_xpos, pcrp1_ypos;
	u32 awbo_x, awbo_y, awbo_s, awbo_basic, con0, con1, con2, con3, con4;

	awb_stat_en = raw_readl_relaxed(dev, dev->base_inner, 0x5a80);
	awb_win_org = raw_readl_relaxed(dev, dev->base_inner, 0x5a88);
	awb_win_size = raw_readl_relaxed(dev, dev->base_inner, 0x5a8c);
	awb_win_pit = raw_readl_relaxed(dev, dev->base_inner, 0x5a90);
	awb_win_num = raw_readl_relaxed(dev, dev->base_inner, 0x5a94);
	qbn_r8_ctl = raw_readl_relaxed(dev, dev->base_inner, 0x5a40);
	qbn_r8_pcrp_ctl = raw_readl_relaxed(dev, dev->base_inner, 0x5a44);
	pcrp0_xpos = raw_readl_relaxed(dev, dev->base_inner, 0x5a48);
	pcrp0_ypos = raw_readl_relaxed(dev, dev->base_inner, 0x5a4c);
	pcrp1_xpos = raw_readl_relaxed(dev, dev->base_inner, 0x5a50);
	pcrp1_ypos = raw_readl_relaxed(dev, dev->base_inner, 0x5a54);
	awbo_x = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x1560);
	awbo_y = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x1564);
	awbo_s = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x1568);
	awbo_basic = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x156c);
	con0 = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x1570);
	con1 = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x1574);
	con2 = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x1578);
	con3 = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x157c);
	con4 = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x1580);

	if (CAM_DEBUG_ENABLED(RAW_INT) || force)
		dev_info(dev->dev,
		"[%s] raw%d - [in] awb_stat_en/awb_win_org/awb_win_size/awb_win_pit/awb_win_num:0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, awb_stat_en, awb_win_org,
		awb_win_size, awb_win_pit, awb_win_num);
	if (CAM_DEBUG_ENABLED(RAW_INT) || force)
		dev_info(dev->dev,
		"[%s] raw%d - [in] qbn_r8_ctl/qbn_r8_pcrp_ctl/pcrp0_xpos/pcrp0_ypos/pcrp1_xpos/pcrp1_ypos:0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, qbn_r8_ctl, qbn_r8_pcrp_ctl,
		pcrp0_xpos, pcrp0_ypos, pcrp1_xpos, pcrp1_ypos);
	if (CAM_DEBUG_ENABLED(RAW_INT) || force)
		dev_info(dev->dev,
		"[%s] raw%d - [in] awbo_x/y/stride/basic/con0/1/2/3/4:0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, awbo_x, awbo_y, awbo_s, awbo_basic,
		con0, con1, con2, con3, con4);
}
static void dump_af_reg(struct mtk_raw_device *dev, bool force)
{
	u32 af_size, af_vld, af_blk_prot, af_blk_0, af_blk_1;
	u32 afo_xsize, afo_ysize, afo_stride;
	u32 qbn_ctl, qbn_pcrp_ctl, pcrp0_xpos, pcrp0_ypos, pcrp1_xpos, pcrp1_ypos;

	af_size = raw_readl_relaxed(dev, dev->base_inner, 0x5710);
	af_vld = raw_readl_relaxed(dev, dev->base_inner, 0x5714);
	af_blk_prot = raw_readl_relaxed(dev, dev->base_inner, 0x5718);
	af_blk_0 = raw_readl_relaxed(dev, dev->base_inner, 0x571c);
	af_blk_1 = raw_readl_relaxed(dev, dev->base_inner, 0x5720);
	afo_xsize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x1600);
	afo_ysize = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x1604);
	afo_stride = raw_readl_relaxed(dev, dev->dmatop_base_inner, 0x1608);
	qbn_ctl = raw_readl_relaxed(dev, dev->base_inner, 0x56c0);
	qbn_pcrp_ctl = raw_readl_relaxed(dev, dev->base_inner, 0x56c4);
	pcrp0_xpos = raw_readl_relaxed(dev, dev->base_inner, 0x56c8);
	pcrp0_ypos = raw_readl_relaxed(dev, dev->base_inner, 0x56d0);
	pcrp1_xpos = raw_readl_relaxed(dev, dev->base_inner, 0x56d4);
	pcrp1_ypos = raw_readl_relaxed(dev, dev->base_inner, 0x56d8);

	if (CAM_DEBUG_ENABLED(RAW_INT) || force)
		dev_info(dev->dev,
			 "[%s] raw%d - [in] af_size/af_vld/af_blk_prot/af_blk_0/af_blk_1:0x%x/0x%x/0x%x/0x%x/0x%x\n",
			 __func__, dev->id, af_size, af_vld, af_blk_prot, af_blk_0, af_blk_1);
	if (CAM_DEBUG_ENABLED(RAW_INT) || force)
		dev_info(dev->dev,
			 "[%s] raw%d - [in] afo_xsize/afo_ysize/afo_stride:0x%x/0x%x/0x%x\n",
			 __func__, dev->id, afo_xsize, afo_ysize, afo_stride);
	if (CAM_DEBUG_ENABLED(RAW_INT) || force)
		dev_info(dev->dev,
		"[%s] raw%d - [in] qbn_r6_ctl/qbn_r6_pcrp_ctl/pcrp0_xpos/pcrp0_ypos/pcrp1_xpos/pcrp1_ypos:0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__, dev->id, qbn_ctl, qbn_pcrp_ctl,
		pcrp0_xpos, pcrp0_ypos, pcrp1_xpos, pcrp1_ypos);
}

static void dump_dc_setting(struct mtk_raw_device *dev)
{
	dev_info_ratelimited(dev->dev, "[outer] CAMCTL_SCENARIO_CTL/MODE 0x%08x/0x%08x DCIF_CTL/2:0x%08x/0x%08x, CHASING_SRC_SEL:0x%08x, TG_DCIF_CTL:0x%08x, LOCK_DONE:0x%08x\n",
		 raw_readl(dev, dev->base, REG_CAMCTL_SCENARIO_CTL),
		 raw_readl(dev, dev->base, REG_CAMCTL_SCENARIO_MODE),
		 raw_readl(dev, dev->base, REG_CAMCTL_DCIF_CTL),
		 raw_readl(dev, dev->base, REG_CAMCTL_DCIF2_CTL),
		 raw_readl(dev, dev->base, REG_CAMCTL_DCIF_CHASING_SRC_SEL),
		 raw_readl(dev, dev->base, REG_TG_DCIF_CTL),
		 raw_readl(dev, dev->base, REG_CAMCTL_LOCK_DONE_SEL));
	dev_info_ratelimited(dev->dev, "[inner] CAMCTL_SCENARIO_CTL/MODE 0x%08x/0x%08x DCIF_CTL/2:0x%08x/0x%08x, CHASING_SRC_SEL:0x%08x, TG_DCIF_CTL:0x%08x, LOCK_DONE:0x%08x\n",
		 raw_readl(dev, dev->base_inner, REG_CAMCTL_SCENARIO_CTL),
		 raw_readl(dev, dev->base_inner, REG_CAMCTL_SCENARIO_MODE),
		 raw_readl(dev, dev->base_inner, REG_CAMCTL_DCIF_CTL),
		 raw_readl(dev, dev->base_inner, REG_CAMCTL_DCIF2_CTL),
		 raw_readl(dev, dev->base_inner, REG_CAMCTL_DCIF_CHASING_SRC_SEL),
		 raw_readl(dev, dev->base_inner, REG_TG_DCIF_CTL),
		 raw_readl(dev, dev->base_inner, REG_CAMCTL_LOCK_DONE_SEL));
}


static void dump_cq_setting(struct mtk_raw_device *dev)
{
	dev_info(dev->dev, "CQ_EN 0x%08x THR_CTL 0x%08x 0x%08x, 0x%08x\n",
		 raw_readl(dev, dev->base, REG_CAMCQ_CQ_EN),
		 raw_readl(dev, dev->base, REG_CAMCQ_CQ_THR0_CTL),
		 raw_readl(dev, dev->base, REG_CAMCQ_CQ_SUB_THR0_CTL),
		 raw_readl(dev, dev->base, REG_CAMCQ_SCQ_START_PERIOD));
}

static void dump_interrupt(struct mtk_raw_device *dev)
{
	dev_info_ratelimited(dev->dev, "CAMCTL INT17_EN 0x%08x\n",
		 raw_readl_relaxed(dev, dev->base, REG_CAMCTL_INT17_EN));
	dev_info_ratelimited(dev->dev, "CAMCTL INT18_EN 0x%08x\n",
		 raw_readl_relaxed(dev, dev->base, REG_CAMCTL_INT18_EN));
	dev_info_ratelimited(dev->dev, "CAMCTL INT20_EN 0x%08x\n",
		 raw_readl_relaxed(dev, dev->base, REG_CAMCTL_INT20_EN));
	dev_info_ratelimited(dev->dev, "CAMCTL INT21_EN 0x%08x\n",
		 raw_readl_relaxed(dev, dev->base, REG_CAMCTL_INT21_EN));
}

static void dump_tg_setting(struct mtk_raw_device *dev, const char *msg)
{
	dev_info(dev->dev,
		 "%s [outer] TG SENMODE/VFCON/PATHCFG/VSEOL_SUB: %x/%x/%x/%x FRMSIZE/R GRABPXL/LIN: %x/%x %x/%x\n",
		 msg,
		 raw_readl_relaxed(dev, dev->base, REG_TG_SEN_MODE),
		 raw_readl_relaxed(dev, dev->base, REG_TG_VF_CON),
		 raw_readl_relaxed(dev, dev->base, REG_TG_PATH_CFG),
		 raw_readl_relaxed(dev, dev->base, REG_TG_VSEOL_SUB_CTL),
		 raw_readl_relaxed(dev, dev->base, REG_TG_FRMSIZE_ST),
		 raw_readl_relaxed(dev, dev->base, REG_TG_FRMSIZE_ST_R),
		 raw_readl_relaxed(dev, dev->base, REG_TG_SEN_GRAB_PXL),
		 raw_readl_relaxed(dev, dev->base, REG_TG_SEN_GRAB_LIN));

	dev_info_ratelimited(dev->dev,
		 "%s [inner] TG SENMODE/VFCON/PATHCFG/VSEOL_SUB: %x/%x/%x/%x GRABPXL/LIN: %x/%x\n",
		 msg,
		 raw_readl_relaxed(dev, dev->base_inner, REG_TG_SEN_MODE),
		 raw_readl_relaxed(dev, dev->base_inner, REG_TG_VF_CON),
		 raw_readl_relaxed(dev, dev->base_inner, REG_TG_PATH_CFG),
		 raw_readl_relaxed(dev, dev->base_inner, REG_TG_VSEOL_SUB_CTL),
		 raw_readl_relaxed(dev, dev->base_inner, REG_TG_SEN_GRAB_PXL),
		 raw_readl_relaxed(dev, dev->base_inner, REG_TG_SEN_GRAB_LIN));
}

static void dump_seqence(struct mtk_raw_device *dev)
{
	dev_info(dev->dev, "in 0x%08x out 0x%08x (mod5_en:0x%x)\n",
		 basic_readl_relaxed(dev, dev->base_inner, REG_FRAME_IDX),
		 basic_readl_relaxed(dev, dev->base, REG_FRAME_IDX),
		 raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_MOD5_EN));
}

static void reset_error_handling(struct mtk_raw_device *dev)
{
	dev->tg_grab_err_handle_cnt = 0;
	dev->dma_err_handle_cnt = 0;
	dev->tg_overrun_handle_cnt = 0;
}

#define CAMCQ_CQ_EN_DEFAULT	0x14
void initialize(struct mtk_raw_device *dev, struct engine_callback *cb,
			    int is_slave, int is_srt, int frm_time_us)
{
	u32 val;

	val = CAMCQ_CQ_EN_DEFAULT;
	SET_FIELD(&val, CAMCQ_CQ_DROP_FRAME_EN, 1);
	raw_writel_relaxed(val, dev, dev->base, REG_CAMCQ_CQ_EN);

	raw_writel_relaxed(0xffffffff, dev, dev->base, REG_CAMCQ_SCQ_START_PERIOD);
	qof_set_cq_start_max(dev, -1);
	val = FBIT(CAMCQ_CQ_THR0_EN);
	SET_FIELD(&val, CAMCQ_CQ_THR0_MODE, 1);

	raw_writel_relaxed(val, dev, dev->base, REG_CAMCQ_CQ_THR0_CTL);
	raw_writel_relaxed(val, dev, dev->base, REG_CAMCQ_CQ_SUB_THR0_CTL);

	/* enable interrupt */
	val = FBIT(CAMCTL_CQ_THR0_DONE_EN) | FBIT(CAMCTL_CQ_THRSUB_DONE_EN) |
		FBIT(CAMCTL_CQ_ALL_THR_DONE_EN)| FBIT(CAMCTL_CQ_MAIN_VS_ERR_EN) |
		FBIT(CAMCTL_CQ_SUB_VS_ERR_EN)| FBIT(CAMCTL_CQ_SUB_CODE_ERR_EN) |
		FBIT(CAMCTL_CQ_DB_LOAD_ERR_EN);
	raw_writel_relaxed(val, dev, dev->base, REG_CAMCTL_INT21_EN);

#ifdef RAW_DEBUG_INIT
	dump_interrupt(dev);
	dump_cq_setting(dev);
#endif
	dev->log_en = false;
	dev->is_slave = is_slave;
	dev->sof_count = 0;
	dev->tg_count = 0;
	dev->vsync_count = 0;
	dev->sub_sensor_ctrl_en = false;
	dev->lock_done_ctrl = false;
	atomic_set(&dev->vf_en, 0);
	mtk_cam_raw_reset_msgfifo(dev);

	init_camsys_settings(dev, is_srt, frm_time_us);
	init_ADLWR_settings(dev->cam);
#ifdef RAW_DEBUG_INIT
	dump_topdebug_rdyreq_status(dev);
#endif
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
	raw_writel_relaxed(0xFFFE0000,
		       dev, dev->base, REG_FLKO_R1_BASE + DMA_OFFSET_ERR_STAT);
	raw_writel_relaxed(0xFFFE0000,
		       dev, dev->base, REG_UFEO_R1_BASE + DMA_OFFSET_ERR_STAT);
	raw_writel_relaxed(0xFFFE0000,
		       dev, dev->base, REG_PDO_R1_BASE + DMA_OFFSET_ERR_STAT);
	/* Workaround: disable AAO/AAHO error: double sof error for smvr
	 *  HW would send double sof to aao/aaho in subsample mode
	 *  disable it to bypass
	 */
	raw_writel_relaxed(0xFFFE0000,
		       dev, dev->base, REG_AEO_R1_BASE + DMA_OFFSET_ERR_STAT);
	raw_writel_relaxed(0xFFFE0000,
		       dev, dev->base, REG_AEHO_R1_BASE + DMA_OFFSET_ERR_STAT);
}
static void subsample_set_sensor_time(struct mtk_raw_device *dev,
	u32 subsample_ratio)
{
	dev->sub_sensor_ctrl_en = true;
	dev->set_sensor_idx = subsample_ratio - 2;
	dev->cur_vsync_idx = -1;
}
void clear_reg(struct mtk_raw_device *dev)
{
	raw_writel(0, dev, dev->base_inner, REG_CAMCTL_MOD10_EN);
	raw_writel(0, dev, dev->base, REG_CAMCTL_MOD10_EN);

	raw_writel(0, dev, dev->base_inner, REG_CAMCTL_CTRL_SIG_SEL);
	raw_writel(0, dev, dev->base, REG_CAMCTL_CTRL_SIG_SEL);

	raw_writel(0, dev, dev->base_inner, REG_CAMCTL_DCIF_CTL);
	raw_writel(0, dev, dev->base, REG_CAMCTL_DCIF_CTL);

	raw_writel(0, dev, dev->base_inner, REG_CAMCTL_DCIF2_CTL);
	raw_writel(0, dev, dev->base, REG_CAMCTL_DCIF2_CTL);

	diable_rms_pcrp(dev);
	diable_rms_module(dev);
}
static void reset_reg(struct mtk_raw_device *dev)
{
	u32 cq_en, sw_done, sw_sub_ctl;

	cq_en = raw_readl_relaxed(dev, dev->base_inner, REG_CAMCQ_CQ_EN);
	sw_done = raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_SW_PASS1_DONE);
	sw_sub_ctl = raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_SW_SUB_CTL);

	SET_FIELD(&cq_en, CAMCQ_SCQ_SUBSAMPLE_EN, 0);
	SET_FIELD(&cq_en, CAMCQ_SCQ_STAGGER_MODE, 0);
	raw_writel(cq_en, dev, dev->base_inner, REG_CAMCQ_CQ_EN);
	raw_writel(cq_en, dev, dev->base, REG_CAMCQ_CQ_EN);

	SET_FIELD(&sw_done, CAMCTL_DOWN_SAMPLE_EN, 0);
	raw_writel(sw_done, dev, dev->base_inner, REG_CAMCTL_SW_PASS1_DONE);
	raw_writel(sw_done, dev, dev->base, REG_CAMCTL_SW_PASS1_DONE);

	raw_writel(0, dev, dev->base_inner, REG_CAMCTL_SW_SUB_CTL);
	raw_writel(0, dev, dev->base, REG_CAMCTL_SW_SUB_CTL);

	raw_writel(0, dev, dev->base_inner, REG_CAMCTL_INT17_EN);
	raw_writel(0, dev, dev->base, REG_CAMCTL_INT17_EN);

	raw_writel(0, dev, dev->base_inner, REG_CAMCTL_INT18_EN);
	raw_writel(0, dev, dev->base, REG_CAMCTL_INT18_EN);

	raw_writel(0, dev, dev->base_inner, REG_CAMCTL_INT19_EN);
	raw_writel(0, dev, dev->base, REG_CAMCTL_INT19_EN);

	raw_writel(0, dev, dev->base_inner, REG_CAMCTL_INT20_EN);
	raw_writel(0, dev, dev->base, REG_CAMCTL_INT20_EN);

	raw_writel(0, dev, dev->base_inner, REG_CAMCTL_INT21_EN);
	raw_writel(0, dev, dev->base, REG_CAMCTL_INT21_EN);

	wmb(); /* make sure committed */
	reset_error_handling(dev);
	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "[%s] CQ_EN/SW_SUB_CTL/SW_DONE/DDREN_ST/SIG_SEL/MOD10 [in] 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x [out] 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
			 __func__,
			 raw_readl_relaxed(dev, dev->base_inner, REG_CAMCQ_CQ_EN),
			 raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_SW_SUB_CTL),
			 raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_SW_PASS1_DONE),
			 raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_DDREN_ST),
			 raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_CTRL_SIG_SEL),
			 raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_MOD10_EN),
			 raw_readl_relaxed(dev, dev->base, REG_CAMCQ_CQ_EN),
			 raw_readl_relaxed(dev, dev->base, REG_CAMCTL_SW_SUB_CTL),
			 raw_readl_relaxed(dev, dev->base, REG_CAMCTL_SW_PASS1_DONE),
			 raw_readl_relaxed(dev, dev->base, REG_CAMCTL_DDREN_ST),
			 raw_readl_relaxed(dev, dev->base, REG_CAMCTL_CTRL_SIG_SEL),
			 raw_readl_relaxed(dev, dev->base, REG_CAMCTL_MOD10_EN));
}

void subsample_enable(struct mtk_raw_device *dev, int subsample_ratio)
{
	u32 val;
	u32 sub_ratio = subsample_ratio - 1;

	subsample_set_sensor_time(dev, subsample_ratio);

	val = raw_readl_relaxed(dev, dev->base, REG_CAMCQ_CQ_EN);
	SET_FIELD(&val, CAMCQ_SCQ_SUBSAMPLE_EN, 1);
	SET_FIELD(&val, CAMCQ_CQ_DROP_FRAME_EN, 0);
	raw_writel_relaxed(val, dev, dev->base, REG_CAMCQ_CQ_EN);

	val = FBIT(CAMCTL_DOWN_SAMPLE_EN);
	SET_FIELD(&val, CAMCTL_DOWN_SAMPLE_PERIOD, sub_ratio);
	raw_writel_relaxed(val, dev, dev->base, REG_CAMCTL_SW_PASS1_DONE);
	raw_writel_relaxed(val, dev, dev->base_inner, REG_CAMCTL_SW_PASS1_DONE);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "[%s] raw%d - CQ_EN:0x%x ratio %d\n",
			 __func__, dev->id,
			 raw_readl_relaxed(dev, dev->base, REG_CAMCQ_CQ_EN),
			 subsample_ratio);
}

/* TODO: cq_set_stagger_mode(dev, 0/1) */
void stagger_enable(struct mtk_raw_device *dev)
{
	u32 val;

	val = raw_readl_relaxed(dev, dev->base, REG_CAMCQ_CQ_EN);
	SET_FIELD(&val, CAMCQ_SCQ_STAGGER_MODE, 1);
	SET_FIELD(&val, CAMCQ_SCQ_INVLD_CLR_CHK, 1);
	raw_writel_relaxed(val, dev, dev->base, REG_CAMCQ_CQ_EN);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "[%s] raw%d - CQ_EN:0x%x\n",
			 __func__, dev->id, raw_readl_relaxed(dev, dev->base, REG_CAMCQ_CQ_EN));
}

void ae_disable(struct mtk_raw_device *dev)
{
	u32 val_mod5, val_mod10, val_mod11;

	val_mod5 = raw_readl_relaxed(dev, dev->base, REG_CAMCTL_MOD5_EN);
	val_mod10 = raw_readl_relaxed(dev, dev->base, REG_CAMCTL_MOD10_EN);
	val_mod11 = raw_readl_relaxed(dev, dev->base, REG_CAMCTL_MOD11_EN);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "[%s++] raw%d - [out] MOD5/MOD10/MOD11:0x%x/0x%x/0x%x [in]:0x%x/0x%x/0x%x\n",
			 __func__, dev->id, val_mod5, val_mod10, val_mod11,
			 readl_relaxed(dev->base_inner + REG_CAMCTL_MOD5_EN),
			 readl_relaxed(dev->base_inner + REG_CAMCTL_MOD10_EN),
			 readl_relaxed(dev->base_inner + REG_CAMCTL_MOD11_EN));

	/* disable CAMCTL_AESTAT_R1_EN*/
	SET_FIELD(&val_mod5, CAMCTL_AESTAT_R1_EN, 0);
	raw_writel_relaxed(val_mod5, dev, dev->base, REG_CAMCTL_MOD5_EN);

	wmb(); /* wmb */
	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "[%s--] raw%d - [out] MOD5/MOD10/MOD11:0x%x/0x%x/0x%x [in]:0x%x/0x%x/0x%x\n",
			 __func__, dev->id, val_mod5, val_mod10, val_mod11,
			 readl_relaxed(dev->base_inner + REG_CAMCTL_MOD5_EN),
			 readl_relaxed(dev->base_inner + REG_CAMCTL_MOD10_EN),
			 readl_relaxed(dev->base_inner + REG_CAMCTL_MOD11_EN));
}

void stagger_disable(struct mtk_raw_device *dev)
{
	u32 val;

	val = raw_readl_relaxed(dev, dev->base, REG_CAMCQ_CQ_EN);
	SET_FIELD(&val, CAMCQ_SCQ_STAGGER_MODE, 0);
	SET_FIELD(&val, CAMCQ_SCQ_INVLD_CLR_CHK, 0);
	raw_writel_relaxed(val, dev, dev->base, REG_CAMCQ_CQ_EN);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "[%s] raw%d - CQ_EN:0x%x\n",
			 __func__, dev->id, raw_readl_relaxed(dev, dev->base, REG_CAMCQ_CQ_EN));
}

void lock_done_ctrl_enable(struct mtk_raw_device *dev, int on)
{
	raw_writel(on, dev, dev->base, REG_CAMCTL_LOCK_DONE_SEL);

	dev->lock_done_ctrl = on;

	dev_info(dev->dev, "[%s] raw%d - on:%d\n", __func__, dev->id, on);
}

void apply_cq(struct mtk_raw_device *dev,
	      dma_addr_t cq_addr,
	      unsigned int cq_size, unsigned int cq_offset,
	      unsigned int sub_cq_size, unsigned int sub_cq_offset)
{
	dma_addr_t main, sub;

	qof_dump_trigger_cnt(dev);
	qof_dump_voter(dev);
	qof_dump_power_state(dev);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev,
			 "apply raw%d cq - addr:0x%llx, size:%d/%d, offset:%d\n",
			 dev->id, cq_addr, cq_size, sub_cq_size, sub_cq_offset);


	/* note: apply cq with size = 0, will cause cq hang */
	if (WARN_ON(!cq_size || !sub_cq_size))
		return;

	main = cq_addr + cq_offset;
	sub = cq_addr + sub_cq_offset;

	raw_writel_relaxed(dmaaddr_lsb(main),
		       dev, dev->base, REG_CAMCQ_CQ_THR0_BASEADDR);
	raw_writel_relaxed(dmaaddr_msb(main),
		       dev, dev->base, REG_CAMCQ_CQ_THR0_BASEADDR_MSB);
	raw_writel_relaxed(cq_size,
		       dev, dev->base, REG_CAMCQ_CQ_THR0_DESC_SIZE);

	raw_writel_relaxed(dmaaddr_lsb(sub),
		       dev, dev->base, REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2);
	raw_writel_relaxed(dmaaddr_msb(sub),
		       dev, dev->base, REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2_MSB);
	raw_writel_relaxed(sub_cq_size,
		       dev, dev->base, REG_CAMCQ_CQ_SUB_THR0_DESC_SIZE_2);

	raw_writel(FBIT(CAMCTL_CQ_THR0_START), dev, dev->base, REG_CAMCTL_START);
	dev->apply_ts = ktime_get_boottime_ns();
}

void dbload_force(struct mtk_raw_device *dev)
{
	u32 val;

	val = raw_readl_relaxed(dev, dev->base, REG_CAMCTL_DB_LOAD_CTL2);
	SET_FIELD(&val, CAMCTL_DB_LOAD_FORCE, 1);
	raw_writel_relaxed(val, dev, dev->base, REG_CAMCTL_DB_LOAD_CTL2);
	raw_writel_relaxed(val, dev, dev->base_inner, REG_CAMCTL_DB_LOAD_CTL2);
	wmb(); /* TBC */
	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev, "%s: 0x%x\n", __func__, val);
}


void toggle_db(struct mtk_raw_device *dev)
{
	u32 val;

	val = raw_readl(dev, dev->base, REG_CAMCTL_DB_LOAD_CTL1);
	raw_writel(val & ~FBIT(CAMCTL_DB_EN), dev, dev->base, REG_CAMCTL_DB_LOAD_CTL1);

	/* read back to make sure committed */
	val = raw_readl(dev, dev->base, REG_CAMCTL_DB_LOAD_CTL1);
	raw_writel(val | FBIT(CAMCTL_DB_EN), dev, dev->base, REG_CAMCTL_DB_LOAD_CTL1);

	dev_info(dev->dev, "%s: 0x%x seq:0x%x/0x%x\n", __func__,
		raw_readl(dev, dev->base, REG_CAMCTL_DB_LOAD_CTL1),
		raw_readl_relaxed(dev, dev->base, REG_FHG_FHG_SPARE_1),
		raw_readl_relaxed(dev, dev->base_inner, REG_FHG_FHG_SPARE_1));
}



void enable_tg_db(struct mtk_raw_device *dev, int en)
{
	u32 val;

	val = raw_readl(dev, dev->base, REG_TG_PATH_CFG);
	SET_FIELD(&val, TG_DB_LOAD_DIS, !en);
	raw_writel(val, dev, dev->base, REG_TG_PATH_CFG);
}

static void set_tg_vfdata_en(struct mtk_raw_device *dev, int on)
{
	u32 val;

	atomic_set(&dev->vf_en, !!on);

	val = raw_readl(dev, dev->base, REG_TG_VF_CON);
	SET_FIELD(&val, TG_VFDATA_EN, on);
	raw_writel(val, dev, dev->base, REG_TG_VF_CON);
	if (on)
		dev_info(dev->dev, "%s: 0x%08x, seq:0x%x/0x%x\n",
		 __func__, raw_readl(dev, dev->base, REG_TG_VF_CON),
		raw_readl_relaxed(dev, dev->base, REG_FHG_FHG_SPARE_1),
		raw_readl_relaxed(dev, dev->base_inner, REG_FHG_FHG_SPARE_1));
}

static u32 scq_cnt_rate_khz(u32 time_stamp_cnt)
{
	return SCQ_DEFAULT_CLK_RATE * 1000 / ((time_stamp_cnt + 1) * 2);
}

void update_scq_start_period(struct mtk_raw_device *dev, int scq_ms, int frame_ms)
{
	u32 val, start_period, max_p1_delay;

	val = raw_readl_relaxed(dev, dev->base, REG_TG_TIME_STAMP_CNT);
	start_period = (scq_ms == -1) ? 0xFFFFFFFF : scq_ms * scq_cnt_rate_khz(val);
	max_p1_delay = (scq_ms == -1) ? frame_ms * scq_cnt_rate_khz(val) : start_period - 1;

	raw_writel_relaxed(start_period,
		       dev, dev->base, REG_CAMCQ_SCQ_START_PERIOD);

	raw_writel_relaxed(max_p1_delay,
		       dev, dev->base, REG_CAMCTL_DC_STAG_CTL);

	qof_set_cq_start_max(dev, scq_ms);
	dev_info(dev->dev, "[%s] STAMP_CNT:0x%08x, SCQ_START_PERIOD:0x%08x (%dms) P1_DELAY:0x%08x (%dms)\n",
		 __func__, val, raw_readl(dev, dev->base, REG_CAMCQ_SCQ_START_PERIOD), scq_ms,
		 raw_readl(dev, dev->base, REG_CAMCTL_DC_STAG_CTL), frame_ms);
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

	wfbc_en_raw = raw_readl_relaxed(dev, dev->base, REG_CAMCTL_WFBC_EN);
	wfbc_en_yuv = raw_readl_relaxed(dev, dev->yuv_base, REG_CAMCTL_WFBC_EN);
	raw_writel(wfbc_en_raw, dev, dev->base, REG_CAMCTL_WFBC_INC);
	raw_writel(wfbc_en_yuv, dev, dev->yuv_base, REG_CAMCTL_WFBC_INC);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev, "[%s] WFBC_INC camctl/camctl2:0x%x/0x%x seq:0x%x/0x%x\n",
			 __func__, wfbc_en_raw, wfbc_en_yuv,
			raw_readl_relaxed(dev, dev->base, REG_FHG_FHG_SPARE_1),
			raw_readl_relaxed(dev, dev->base_inner, REG_FHG_FHG_SPARE_1));
	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev, "[%s] (CTRL_SIG_SEL, SEL, SEL2, SEL3) out/in:(0x%x/0x%x,0x%x/0x%x,0x%x/0x%x,0x%x/0x%x)\n",
		__func__,
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_CTRL_SIG_SEL),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_CTRL_SIG_SEL),
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_SEL),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_SEL),
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_SEL2),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_SEL2),
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_SEL3),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_SEL3));
	if (CAM_DEBUG_ENABLED(RAW_INT))
		dump_dc_setting(dev);
}
void set_sig_sel_master(struct mtk_raw_device *dev)
{
	unsigned int camctl_sel3 = raw_readl_relaxed(dev, dev->base, REG_CAMCTL_SEL3);

	raw_writel(0x0, dev, dev->base, REG_CAMCTL_CTRL_SIG_SEL);
	/* sig_sel -> cq -> toggle db -> failed */
	raw_writel(0x0, dev, dev->base_inner, REG_CAMCTL_CTRL_SIG_SEL);
	if (dev->id == RAW_C) {
		raw_writel(camctl_sel3 | 0x0, dev, dev->base, REG_CAMCTL_SEL3);
		raw_writel(camctl_sel3 | 0x0, dev, dev->base_inner, REG_CAMCTL_SEL3);
	}
	dev_info(dev->dev, "[%s] (CTRL_SIG_SEL, SEL3) out/in:(0x%x/0x%x, 0x%x->0x%x/0x%x)\n",
		__func__,
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_CTRL_SIG_SEL),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_CTRL_SIG_SEL),
		camctl_sel3,
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_SEL3),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_SEL3));
}

void set_dcif_en_slave(struct mtk_raw_device *dev)
{
	raw_writel(0x1, dev, dev->base, REG_CAMCTL_DCIF2_CTL);
	raw_writel(0x1, dev, dev->base_inner, REG_CAMCTL_DCIF2_CTL);

	dev_info(dev->dev, "[%s] (DCIF_CTL, DCIF_CTL2) out/in:(0x%x/0x%x, 0x%x/0x%x)\n",
		__func__,
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_DCIF_CTL),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_DCIF_CTL),
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_DCIF2_CTL),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_DCIF2_CTL));
}

void set_sig_sel_slave(struct mtk_raw_device *dev)
{
	unsigned int camctl_sel3 = raw_readl_relaxed(dev, dev->base, REG_CAMCTL_SEL3);

	raw_writel(0x1, dev, dev->base, REG_CAMCTL_CTRL_SIG_SEL);
	raw_writel(0x1, dev, dev->base_inner, REG_CAMCTL_CTRL_SIG_SEL);

	if (dev->id == RAW_C) {
		raw_writel(camctl_sel3 | 0x1, dev, dev->base, REG_CAMCTL_SEL3);
		raw_writel(camctl_sel3 | 0x1, dev, dev->base_inner, REG_CAMCTL_SEL3);
	}
	dev_info(dev->dev, "[%s] (CTRL_SIG_SEL, SEL3, SEL) out/in:(0x%x/0x%x, 0x%x/0x%x, 0x%x/0x%x)\n",
		__func__,
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_CTRL_SIG_SEL),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_CTRL_SIG_SEL),
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_SEL3),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_SEL3),
		raw_readl_relaxed(dev, dev->base, REG_CAMCTL_SEL),
		raw_readl_relaxed(dev, dev->base_inner, REG_CAMCTL_SEL));
}

void check_master_raw_vf_en(struct mtk_raw_device *dev)
{
	unsigned int vf_con = raw_readl_relaxed(dev, dev->base, REG_TG_VF_CON);
	unsigned int vf_con_inner = raw_readl_relaxed(dev, dev->base_inner, REG_TG_VF_CON);

	dev_info(dev->dev, "%s: 0x%08x/0x%08x, seq:0x%x/0x%x\n",
			__func__, vf_con, vf_con_inner,
			raw_readl_relaxed(dev, dev->base, REG_FHG_FHG_SPARE_1),
			raw_readl_relaxed(dev, dev->base_inner, REG_FHG_FHG_SPARE_1));
	if ((vf_con & BIT(0)) == 0 || (vf_con_inner & BIT(0)) == 0) {
		set_tg_vfdata_en(dev, 1);
		dev_info(dev->dev, "%s:fix: 0x%08x/0x%08x, seq:0x%x/0x%x\n",
			__func__, raw_readl(dev, dev->base, REG_TG_VF_CON),
			raw_readl(dev, dev->base_inner, REG_TG_VF_CON),
			raw_readl_relaxed(dev, dev->base, REG_FHG_FHG_SPARE_1),
			raw_readl_relaxed(dev, dev->base_inner, REG_FHG_FHG_SPARE_1));
	}
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
	cookie = raw_readl(dev, dev->base_inner, REG_FRAME_IDX);
	engine_fsm_sof(&dev->fsm, cookie, cookie, 0, NULL);

	engine_handle_sof(&dev->cq_ref,
			  bit_map_bit(MAP_HW_RAW, dev->id),
			  cookie);
}

static inline void trigger_rawi(struct mtk_raw_device *dev, u32 val)
{
	//dev_info(dev->dev, "%s: 0x%x\n", __func__, val);
	raw_writel(val, dev, dev->base, REG_CAMCTL_RAW_TRIG);
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

	raw_writel(adlrd_ctrl, dev, dev->cam->base, CAM_MAIN_ADLRD_CTRL);
	trigger_rawi(dev, FBIT(CAMCTL_APU_TRIG) | FBIT(CAMCTL_RAW_TRIG));
}

static void write_pkt_apu_raw(struct mtk_raw_device *dev,
			      struct cmdq_pkt *pkt,
			      bool is_apu_dc)
{
	int raw_id = dev->id;
	int adlrd_ctrl;
	int trig;
	struct adl_cmdq_worker_param *param = NULL;

	CALL_PLAT_HW(query_adl_cmdq_worker_param, &param);
	if (WARN_ON(!param))
		return;

	adlrd_ctrl =
		(raw_id << 1) | /* ADLRD_MUX_SEL */
		0x1; /* ADLRD_EN */

	trig = is_apu_dc ?
		FBIT(CAMCTL_APU_TRIG) :
		(FBIT(CAMCTL_APU_TRIG) | FBIT(CAMCTL_RAW_TRIG));
	if (param) {
		cmdq_pkt_write(
				pkt, NULL, param->apu_dc_larb_base,
				is_apu_dc ? 0xf0000 : 0x00001, 0xffffffff);

		/* CAM_MAIN_ADLRD_CTRL */
		cmdq_pkt_write(
				pkt, NULL,
				param->cam_main_adlrd_ctrl_base, adlrd_ctrl, 0xffffffff);
	}
	/* CAMCTL_RAWI_TRIG: CAMCTL_APU_TRIG */
	cmdq_pkt_write(pkt, NULL, dev->base_reg_addr + REG_CAMCTL_RAW_TRIG, trig,
		       0xffffffff);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev->dev, "dc %d adlrd_ctrl 0x%x RAWI_TRIG 0x%x\n",
			 is_apu_dc, adlrd_ctrl, trig);
}

void write_pkt_trigger_apu_dc(struct mtk_raw_device *dev,
			      struct cmdq_pkt *pkt)
{
#define APU_SW_EVENT (675)

	struct adl_cmdq_worker_param *param = NULL;

	CALL_PLAT_HW(query_adl_cmdq_worker_param, &param);
	if (WARN_ON(!param))
		return;

	/* wait APU ready */
	cmdq_pkt_wfe(pkt, APU_SW_EVENT);

	write_pkt_apu_raw(dev, pkt, true /* is_apu_dc */);

	/* trigger APU */
	if (param)
		cmdq_pkt_write(pkt, NULL, param->apu_mbox_dc_mode_base, 0x1, 0xffffffff);
}

void write_pkt_trigger_apu_frame_mode(struct mtk_raw_device *dev,
				      struct cmdq_pkt *pkt)
{
	write_pkt_apu_raw(dev, pkt, false /* is_apu_dc */);
}

bool is_rawi_ufdi_rdone_zero(struct mtk_raw_device *dev)
{
	u32 rawi_r2_dbg, ufdi_r2_dbg, rawi_r5_dbg, ufdi_r5_dbg;

	writel(DBG_SEL_RAWI_R2_SMI_DBG_DATA, dev->dmatop_base + REG_CAMRAWDMATOP_DMA_DBG_SEL);
	rawi_r2_dbg = readl(dev->dmatop_base + REG_CAMRAWDMATOP_DMA_DBG_PORT);
	writel(DBG_SEL_UFDI_R2_SMI_DBG_DATA, dev->dmatop_base + REG_CAMRAWDMATOP_DMA_DBG_SEL);
	ufdi_r2_dbg = readl(dev->dmatop_base + REG_CAMRAWDMATOP_DMA_DBG_PORT);
	writel(DBG_SEL_RAWI_R5_SMI_DBG_DATA, dev->dmatop_base + REG_CAMRAWDMATOP_DMA_DBG_SEL);
	rawi_r5_dbg = readl(dev->dmatop_base + REG_CAMRAWDMATOP_DMA_DBG_PORT);
	writel(DBG_SEL_UFDI_R5_SMI_DBG_DATA, dev->dmatop_base + REG_CAMRAWDMATOP_DMA_DBG_SEL);
	ufdi_r5_dbg = readl(dev->dmatop_base + REG_CAMRAWDMATOP_DMA_DBG_PORT);

	if (rawi_r2_dbg & BIT(19) && ufdi_r2_dbg & BIT(19) &&
	    rawi_r5_dbg & BIT(19) && ufdi_r5_dbg & BIT(19))
		return true;

	return false;
}

/* check again for rawi dcif case */
bool is_all_dma_idle(struct mtk_raw_device *dev)
{
	struct mtk_yuv_device *yuv = get_yuv_dev(dev);

	u32 chasing_stat = raw_readl(dev, dev->dmatop_base, REG_CAMRAWDMATOP_DC_DBG_CHASING_STATUS);
	u32 chasing_stat2 = raw_readl(dev, dev->dmatop_base, REG_CAMRAWDMATOP_DC_DBG_CHASING_STATUS2);
	u32 raw_rst_stat = raw_readl(dev, dev->dmatop_base, REG_CAMRAWDMATOP_DMA_SOFT_RST_STAT);
	u32 raw_rst_stat2 = raw_readl(dev, dev->dmatop_base, REG_CAMRAWDMATOP_DMA_SOFT_RST_STAT2);
	u32 yuv_rst_stat = raw_readl(dev, yuv->dmatop_base, REG_CAMYUVDMATOP_DMA_SOFT_RST_STAT);

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
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS_RAWI_R4) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat2, CAMRAWDMATOP_RAWI_R4_SOFT_RST_STAT, 1);

	if (READ_FIELD(raw_rst_stat2,
			CAMRAWDMATOP_UFDI_R4_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat2,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS_UFDI_R4) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat2, CAMRAWDMATOP_UFDI_R4_SOFT_RST_STAT, 1);

	if (READ_FIELD(raw_rst_stat,
			CAMRAWDMATOP_RAWI_R5_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat2,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS_RAWI_R5) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat, CAMRAWDMATOP_RAWI_R5_SOFT_RST_STAT, 1);

	if (READ_FIELD(raw_rst_stat,
			CAMRAWDMATOP_UFDI_R5_SOFT_RST_STAT) == 0 &&
		(READ_FIELD(chasing_stat2,
			CAMRAWDMATOP_DC_DBG_CHASING_STATUS_UFDI_R5) & BIT(0)) == 0)
		SET_FIELD(&raw_rst_stat, CAMRAWDMATOP_UFDI_R5_SOFT_RST_STAT, 1);

	if (raw_rst_stat == REG_CAMRAWDMATOP_DMA_SOFT_RST_STAT_MASK &&
		raw_rst_stat2 == REG_CAMRAWDMATOP_DMA_SOFT_RST2_STAT_MASK &&
		yuv_rst_stat == REG_CAMYUVDMATOP_DMA_SOFT_RST_STAT_MASK)
		return is_rawi_ufdi_rdone_zero(dev);

	return false;
}

void dump_dma_soft_rst_stat(struct mtk_raw_device *dev)
{
	struct mtk_yuv_device *yuv = get_yuv_dev(dev);

	int raw_rst_stat = raw_readl(dev, dev->dmatop_base, REG_CAMRAWDMATOP_DMA_SOFT_RST_STAT);
	int raw_rst_stat2 = raw_readl(dev, dev->dmatop_base, REG_CAMRAWDMATOP_DMA_SOFT_RST_STAT2);
	int yuv_rst_stat = raw_readl(dev, yuv->dmatop_base, REG_CAMYUVDMATOP_DMA_SOFT_RST_STAT);

	dev_info(dev->dev, "%s: rst_stat: 0x%08x 0x%08x 0x%08x\n",
		 __func__, raw_rst_stat, raw_rst_stat2, yuv_rst_stat);
}

void reset(struct mtk_raw_device *dev)
{
	int sw_ctl;
	u32 mod10_en, val;
	int ret;

	dev_info(dev->dev, "%s\n", __func__);

	/* Disable all DMA DCM before reset */
	raw_writel(0xffffffff, dev, dev->base, REG_CAMCTL_MOD10_DCM_DIS);
	raw_writel(0xffffffff, dev, dev->base, REG_CAMCTL_MOD11_DCM_DIS);
	raw_writel(0xffffffff, dev, dev->yuv_base, REG_CAMCTL2_MOD11_DCM_DIS);

	/* enable CQI_R1 ~ R4 before reset and make sure loaded to inner */
	mod10_en = raw_readl(dev, dev->base, REG_CAMCTL_MOD10_EN);
	val = mod10_en | FBIT(CAMCTL_CQI_R1_EN) |
		FBIT(CAMCTL_CQI_R2_EN) |
		FBIT(CAMCTL_CQI_R3_EN) |
		FBIT(CAMCTL_CQI_R4_EN);
	raw_writel(val, dev, dev->base, REG_CAMCTL_MOD10_EN);
	raw_writel(val, dev, dev->base_inner, REG_CAMCTL_MOD10_EN);
	if (CAM_DEBUG_ENABLED(RAW_CG))
		dev_info(dev->dev, "%s mod10_en/val:0x%x/0x%x\n", __func__, mod10_en, val);
	raw_writel(0, dev, dev->base, REG_CAMCTL_SW_CTL);
	raw_writel(FBIT(CAMCTL_SW_RST_TRIG), dev, dev->base, REG_CAMCTL_SW_CTL);
	wmb(); /* make sure committed */

	ret = readx_poll_timeout(readl, dev->base + REG_CAMCTL_SW_CTL, sw_ctl,
				sw_ctl & FBIT(CAMCTL_SW_RST_ST) || is_all_dma_idle(dev),
				50 /* delay, us */,
				5000 /* timeout, us */);
	if (ret < 0) {
		dev_info(dev->dev, "%s: error: timeout!\n", __func__);
		dump_dma_soft_rst_stat(dev);
		mtk_smi_dbg_hang_detect("camsys-raw");
		goto RESET_FAILURE;
	}
	reset_error_handling(dev);
	/* do hw rst */
	raw_writel(FBIT(CAMCTL_HW_RST), dev, dev->base, REG_CAMCTL_SW_CTL);
	raw_writel(FBIT(CAMCTL_GLOBAL_HW_RST), dev, dev->base, REG_CAMCTL_GLOBAL_HW_RST_CTL);
	raw_writel(0, dev, dev->base, REG_CAMCTL_SW_CTL);
	raw_writel(0, dev, dev->base, REG_CAMCTL_GLOBAL_HW_RST_CTL);

RESET_FAILURE:

	raw_writel(mod10_en, dev, dev->base, REG_CAMCTL_MOD10_EN);
	raw_writel(mod10_en, dev, dev->base_inner, REG_CAMCTL_MOD10_EN);

	/* Enable all DMA DCM back */
	raw_writel(0x0, dev, dev->base, REG_CAMCTL_MOD10_DCM_DIS);
	raw_writel(0x0, dev, dev->base, REG_CAMCTL_MOD11_DCM_DIS);
	raw_writel(0x0, dev, dev->yuv_base, REG_CAMCTL2_MOD11_DCM_DIS);

	wmb(); /* make sure committed */
}

#define ADLRD_RESET  0x0800
#define ADLRD_CTRL_1 0x0804
#define ADLRD_CTRL_2 0x0808
void adlrd_reset(struct mtk_cam_device *cam_dev)
{
	int sw_ctl;
	int ret;

	if (IS_ERR_OR_NULL(cam_dev->adlrd_base)) {
		dev_info(cam_dev->dev, "%s: skipped\n", __func__);
		return;
	}

	writel(0x0, cam_dev->adlrd_base + ADLRD_RESET);
	writel(0x1, cam_dev->adlrd_base + ADLRD_RESET);
	wmb(); /* make sure committed */
	ret = readx_poll_timeout(readl, cam_dev->adlrd_base + ADLRD_RESET,
				 sw_ctl,
				 sw_ctl & BIT(2),
				 1 /* delay, us */,
				 5000 /* timeout, us */);
	if (ret < 0) {
		dev_info(cam_dev->dev, "%s: error: timeout!\n", __func__);
		return;
	}

	/* do hw rst */
	writel(BIT(4), cam_dev->adlrd_base + ADLRD_RESET);
	writel(0, cam_dev->adlrd_base + ADLRD_RESET);

	wmb(); /* make sure committed */

	dev_info(cam_dev->dev, "%s done\n", __func__);
}

int mtk_cam_raw_reset_msgfifo(struct mtk_raw_device *dev)
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
#ifdef UFD_DEBUG
	mtk_cam_dump_ufd_debug(dev,
			       "UFD_R2",
			       dbg_UFD_R2, ARRAY_SIZE(dbg_UFD_R2));
	mtk_cam_dump_ufd_debug(dev,
			       "UFD_R5",
			       dbg_UFD_R5, ARRAY_SIZE(dbg_UFD_R5));
#endif
	mtk_cam_dump_dma_debug(dev,
			       dev->dmatop_base, /* DMATOP_BASE */
			       "RAWI_R2",
			       dbg_RAWI_R2, ARRAY_SIZE(dbg_RAWI_R2));
	mtk_cam_dump_dma_debug(dev,
			       dev->dmatop_base, /* DMATOP_BASE */
			       "RAWI_R2_UFD",
			       dbg_RAWI_R2_UFD, ARRAY_SIZE(dbg_RAWI_R2_UFD));
	mtk_cam_dump_dma_debug(dev,
			       dev->dmatop_base, /* DMATOP_BASE */
			       "RAWI_R5",
			       dbg_RAWI_R5, ARRAY_SIZE(dbg_RAWI_R5));
	mtk_cam_dump_dma_debug(dev,
			       dev->dmatop_base, /* DMATOP_BASE */
			       "RAWI_R5_UFD",
			       dbg_RAWI_R5_UFD, ARRAY_SIZE(dbg_RAWI_R5_UFD));

}

static void raw_dump_debug_cqi_status(struct mtk_raw_device *dev)
{
	mtk_cam_dump_dma_debug(dev,
			       dev->dmatop_base, /* DMATOP_BASE */
			       "CQI_R1",
			       dbg_CQI_R1, ARRAY_SIZE(dbg_CQI_R1));
	mtk_cam_dump_dma_debug(dev,
			       dev->dmatop_base, /* DMATOP_BASE */
			       "CQI_R2",
			       dbg_CQI_R2, ARRAY_SIZE(dbg_CQI_R2));
	mtk_cam_dump_dma_debug(dev,
			       dev->dmatop_base, /* DMATOP_BASE */
			       "CQI_R3",
			       dbg_CQI_R3, ARRAY_SIZE(dbg_CQI_R3));
	mtk_cam_dump_dma_debug(dev,
			       dev->dmatop_base, /* DMATOP_BASE */
			       "CQI_R4",
			       dbg_CQI_R4, ARRAY_SIZE(dbg_CQI_R4));
}

static void dump_halt_setting(struct mtk_raw_device *dev)
{
	struct mtk_cam_device *cam_dev = dev->cam;

	dev_info_ratelimited(dev->dev, "%s: halt1~10,13:0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		__func__,
		readl(cam_dev->base + REG_HALT1_EN), readl(cam_dev->base + REG_HALT2_EN),
		readl(cam_dev->base + REG_HALT3_EN), readl(cam_dev->base + REG_HALT4_EN),
		readl(cam_dev->base + REG_HALT5_EN), readl(cam_dev->base + REG_HALT6_EN),
		readl(cam_dev->base + REG_HALT7_EN), readl(cam_dev->base + REG_HALT8_EN),
		readl(cam_dev->base + REG_HALT9_EN), readl(cam_dev->base + REG_HALT10_EN),
		readl(cam_dev->base + REG_HALT13_EN));
}

static void raw_handle_skip_frame(struct mtk_raw_device *raw_dev,
			     struct mtk_camsys_irq_info *data)
{
	int err_status = data->e.err_status;
	unsigned int fh_cookie = data->frame_idx_inner;

	dev_info(raw_dev->dev, "%s: dcif_status:0x%x, fh_cookie:0x%x\n",
			__func__, err_status, fh_cookie);

	if (err_status & FBIT(CAMCTL_P1_SKIP_FRAME_DC_STAG_INT_ST)) {
		dump_halt_setting(raw_dev);
		mtk_cam_bwr_dbg_dump(raw_dev->cam->bwr);
		mmdvfs_debug_status_dump(NULL);
#if KERNEL_VERSION(6, 7, 0) >= LINUX_VERSION_CODE
		mmqos_hrt_dump();
#endif

		if (DISABLE_RECOVER_FLOW || raw_dev->lock_done_ctrl)
			do_engine_callback(raw_dev->engine_cb, dump_request,
				raw_dev->cam, CAMSYS_ENGINE_RAW, raw_dev->id,
				fh_cookie, MSG_DC_SKIP_FRAME);
	}
}

static void raw_handle_ringbuffer_ofl(struct mtk_raw_device *raw,
				      struct mtk_camsys_irq_info *data)
{
	unsigned int fh_cookie = data->frame_idx_inner;

	dev_info(raw->dev, "%s: fh_cookie: 0x%x, chasing_status 0x%x, 0x%x line_cnt:0x%x 0x%x 0x%x 0x%x\n",
		 __func__, fh_cookie,
		 raw_readl_relaxed(raw, raw->dmatop_base, REG_CAMRAWDMATOP_DC_DBG_CHASING_STATUS),
		 raw_readl_relaxed(raw, raw->dmatop_base, REG_CAMRAWDMATOP_DC_DBG_CHASING_STATUS2),
		 raw_readl_relaxed(raw, raw->dmatop_base, REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_RAWI_R2),
		 raw_readl_relaxed(raw, raw->dmatop_base, REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_UFDI_R2),
		 raw_readl_relaxed(raw, raw->dmatop_base, REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_RAWI_R5),
		 raw_readl_relaxed(raw, raw->dmatop_base, REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_UFDI_R5));

	WRAP_AEE_EXCEPTION(MSG_RINGBUFFER_OFL, dev_name(raw->dev));
}

static bool is_sub_sample_sensor_timing(struct mtk_raw_device *dev)
{
	return dev->cur_vsync_idx >= dev->set_sensor_idx;
}

/* Frame IRQ Error Mask */
#define INT_ST_MASK_CAM_ERR_FRAME					\
	(FBIT(CAMCTL_DMA_ERR_ST)				|	\
	 FBIT(CAMCTL_CFG_SW_INCOMP_INT_ST))


/* CQ IRQ Error Mask */
#define INT_ST_MASK_CAM_ERR_CQ					\
	(FBIT(CAMCTL_CQ_MAX_START_DLY_SMALL_INT_ST)	|	\
	 FBIT(CAMCTL_CQ_MAX_START_DLY_ERR_INT_ST)	|	\
	 FBIT(CAMCTL_CQ_MAIN_CODE_ERR_ST)		|	\
	 FBIT(CAMCTL_CQ_DB_LOAD_ERR_ST)		|	\
	 FBIT(CAMCTL_CQ_MAIN_VS_ERR_ST)			|	\
	 FBIT(CAMCTL_CQ_MAIN_VS_ERR_ST)			|	\
	 FBIT(CAMCTL_CQ_TRIG_DLY_INT_ST)		|	\
	 FBIT(CAMCTL_CQ_SUB_CODE_ERR_ST)		|	\
	 FBIT(CAMCTL_CQ_SUB_VS_ERR_ST))

/* TG IRQ Error Mask */
#define INT_ST_MASK_CAM_ERR_TG2					\
	(FBIT(CAMCTL_TG2_OVRUN_ST)			|	\
	 FBIT(CAMCTL_TG2_GRABERR_ST)			|	\
	 FBIT(CAMCTL_TG2_SOF_DROP_ST))


/* TG IRQ Error Mask */
#define INT_ST_MASK_CAM_ERR_TG1					\
	(FBIT(CAMCTL_TG_OVRUN_ST)			|	\
	 FBIT(CAMCTL_TG_GRABERR_ST)			|	\
	 FBIT(CAMCTL_TG_SOF_DROP_ST))


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
static void mtk_raw_log_tg_timestamp(struct mtk_raw_device *raw)
{
	u32 ts_ctl;
	u64 ts;

	dev_info(raw->dev, "ctl 0x%x before lock, tg timestamp msb 0x%08x lsb 0x%08x\n",
		 raw_readl_relaxed(raw, raw->base, REG_TG_TIME_STAMP_CTL),
		 raw_readl_relaxed(raw, raw->base, REG_TG_TIME_STAMP_MSB),
		 raw_readl_relaxed(raw, raw->base, REG_TG_TIME_STAMP));

	ts_ctl = raw_readl_relaxed(raw, raw->base, REG_TG_TIME_STAMP_CTL) | BIT(4);
	raw_writel(ts_ctl, raw, raw->base, REG_TG_TIME_STAMP_CTL);

	ts = (u64)raw_readl(raw, raw->base, REG_TG_TIME_STAMP_MSB) << 32;
	ts += (u64)raw_readl(raw, raw->base, REG_TG_TIME_STAMP);

	dev_info(raw->dev, "ctl 0x%x after lock, tg timestamp %llu msb 0x%08x lsb 0x%08x\n",
		 raw_readl_relaxed(raw, raw->base, REG_TG_TIME_STAMP_CTL),
		 ts,
		 raw_readl_relaxed(raw, raw->base, REG_TG_TIME_STAMP_MSB),
		 raw_readl_relaxed(raw, raw->base, REG_TG_TIME_STAMP));
}
#endif

static irqreturn_t mtk_irq_raw_yuv(int irq, void *data)
{
	struct mtk_raw_device *raw = (struct mtk_raw_device *)data;
	struct mtk_yuv_device *yuv = get_yuv_dev(raw);
	struct device *dev = raw->dev;
	struct mtk_camsys_irq_info irq_info;
	/* raw part */
	unsigned int frame_idx, frame_idx_inner;
	unsigned int frame_status, tg1_status, tg2_status, cq_status, dcif_status, lock_done_sel;
	unsigned int frame_e_status, tg1_e_status, tg2_e_status, cq_e_status;
	unsigned int dma_ofl_status, dmao_done_status, dmai_done_status;
	unsigned int dma_ufl_status, dma_ring_ufl_status, tfm_mismatch_status;
	unsigned int tg_cnt, err_status;
	bool wake_thread = 0;
	/* yuv part */
	unsigned int frame_status_y, err_status_y, wdma_done_status_y;
	unsigned int dma_ofl_status_y, tfm_mismatch_status_y;

	/* yuv part */
	frame_status_y =
		raw_readl_relaxed(raw, yuv->base, REG_CAMCTL2_INT17_STATUS);
	wdma_done_status_y =
		raw_readl_relaxed(raw, yuv->base, REG_CAMCTL2_INT2_STATUS);
	dma_ofl_status_y =
		raw_readl_relaxed(raw, yuv->base, REG_CAMCTL2_INT5_STATUS);
	tfm_mismatch_status_y =
		raw_readl_relaxed(raw, yuv->base, REG_CAMCTL2_INT8_STATUS);
	err_status_y = frame_status_y & 0x4; // bit2: DMA_ERR

	//if (unlikely(debug_raw))
	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(yuv->dev, "YUV-INT: 17/2/5/8:0x%x(err:0x%x)/0x%x/0x%x/0x%x\n",
			frame_status_y, err_status_y,
			wdma_done_status_y, dma_ofl_status_y, tfm_mismatch_status_y);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		if (err_status_y)
			dump_yuv_dma_err_st(yuv);

	/* trace */
	trace_yuv_irq(yuv->dev, frame_status_y, wdma_done_status_y,
				tfm_mismatch_status_y);
	trace_raw_dma_status(yuv->dev, frame_status_y,
				dma_ofl_status_y, tfm_mismatch_status_y);
	/* raw part */
	dmao_done_status = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT2_STATUS);
	dmai_done_status = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT3_STATUS);
	dma_ofl_status	 = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT5_STATUS);
	dma_ufl_status	 = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT6_STATUS);
	dma_ring_ufl_status	 = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT7_STATUS);
	tfm_mismatch_status	 = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT8_STATUS);
	frame_status	 = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT17_STATUS);
	tg1_status		 = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT18_STATUS);
	tg2_status		 = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT19_STATUS);
	dcif_status		 = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT20_STATUS);
	cq_status		 = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_INT21_STATUS);

	frame_idx	= raw_readl_relaxed(raw, raw->base, REG_FRAME_IDX);
	frame_idx_inner	= raw_readl_relaxed(raw, raw->base_inner, REG_FRAME_IDX);
	lock_done_sel = raw_readl_relaxed(raw, raw->base, REG_CAMCTL_LOCK_DONE_SEL);
	tg_cnt = raw_readl_relaxed(raw, raw->base, REG_TG_INTER_ST);
	tg_cnt = (raw->tg_count & 0xffffff00) + ((tg_cnt & 0xff000000) >> 24);

	frame_e_status = frame_status & INT_ST_MASK_CAM_ERR_FRAME;
	tg1_e_status = tg1_status & INT_ST_MASK_CAM_ERR_TG1;
	tg2_e_status = tg2_status & INT_ST_MASK_CAM_ERR_TG2;
	cq_e_status = cq_status & INT_ST_MASK_CAM_ERR_CQ;
	err_status = frame_e_status | tg1_e_status | cq_e_status;

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(dev,
			"RAW-INT: 17/18/19/20/21/2/3/8 0x%x(err:0x%x)/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x lock:0x%x, in:0x%x\n",
			frame_status, err_status, tg1_status, tg2_status, dcif_status, cq_status,
			dmao_done_status, dmai_done_status, tfm_mismatch_status, lock_done_sel,
			frame_idx_inner);

	irq_info.irq_type = 0;
	irq_info.frame_idx = frame_idx;
	irq_info.frame_idx_inner = frame_idx_inner;
	irq_info.fbc_empty = 0;
	irq_info.ts_ns = ktime_get_boottime_ns();
	irq_info.debug_en = raw_readl_relaxed(raw, raw->base_inner, REG_CAMCTL_MOD5_EN);
	/* CQ done */
	if (cq_status & FBIT(CAMCTL_CQ_THR0_DONE_ST)) {
		if (raw->is_timeshared) {
			irq_info.irq_type |= 1 << CAMSYS_IRQ_SETTING_DONE;
		} else if (raw->cq_ref != NULL) {
			long mask = bit_map_bit(MAP_HW_RAW, raw->id);

			if (engine_handle_cq_done(&raw->cq_ref, mask)) {
				irq_info.irq_type |= 1 << CAMSYS_IRQ_SETTING_DONE;
				qof_dump_voter(raw);
				qof_dump_power_state(raw);
			}
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
#ifdef DEBUG_WDMA
	if (wdma_done_status_y & FBIT(CAMCTL2_TCYSO_R1_DONE_ST))
		irq_info.irq_type |= 1 << CAMSYS_IRQ_DEBUG_1;
#endif
	/* Frame done */
	if (frame_status & FBIT(CAMCTL_SW_PASS1_DONE_ST) ||
		frame_status_y & FBIT(CAMCTL2_YUV_PASS1_DONE_ST)) {
		irq_info.irq_type |= 1 << CAMSYS_IRQ_FRAME_DONE;
		qof_dump_trigger_cnt(raw);
		qof_dump_voter(raw);
		qof_dump_power_state(raw);
	}

	/* ois compensation */
	if (raw->lock_done_ctrl && dmao_done_status & FBIT(CAMCTL_FHO_R1_DONE_ST))
		raw_writel(1, raw, raw->base, REG_CAMCTL_LOCK_DONE_SEL);

	/* Frame start */
	if (tg1_status & FBIT(CAMCTL_TG_SOF_INT_ST) ||
		(!raw->sub_sensor_ctrl_en &&
		dmao_done_status & FBIT(CAMCTL_FHO_R1_DONE_ST))||
		dcif_status & FBIT(CAMCTL_DCIF_LAST_CQ_START_INT_ST)) {
		irq_info.irq_type |= 1 << CAMSYS_IRQ_FRAME_START;
		raw->sof_count++;
		raw->cur_vsync_idx = 0;

		if (frame_status & FBIT(CAMCTL_SW_ENQUE_ERR_ST) ||
			dcif_status & FBIT(CAMCTL_DCIF_LAST_CQ_START_INT_ST))
			irq_info.fbc_empty = 1;
		else
			irq_info.fbc_empty = 0;

		if (tg_cnt < raw->tg_count)
			raw->tg_count = tg_cnt + BIT(8);
		else
			raw->tg_count = tg_cnt;
		irq_info.tg_cnt = raw->tg_count;
		if (CAM_DEBUG_ENABLED(EXTISP_SW_CNT))
			irq_info.tg_cnt = raw->sof_count - 2;
		/* make sure no cq applied by hw postpone */
		if (tg1_status & FBIT(CAMCTL_TG_SOF_INT_ST) && frame_idx <= frame_idx_inner &&
			!raw->sub_sensor_ctrl_en)
			do_engine_callback(raw->engine_cb, do_workaround_at_sof,
				   raw->cam, CAMSYS_ENGINE_RAW, raw->id, frame_idx);
		engine_handle_sof(&raw->cq_ref,
				  bit_map_bit(MAP_HW_RAW, raw->id),
				  irq_info.frame_idx_inner);

		qof_dump_trigger_cnt(raw);
		qof_dump_voter(raw);
		qof_dump_power_state(raw);
	}

	/* DCIF main sof */
	if (dcif_status & FBIT(CAMCTL_DCIF_FIRST_SOF_INT_ST))
		irq_info.irq_type |= 1 << CAMSYS_IRQ_FRAME_START_DCIF_MAIN;

	/* Vsync interrupt */
	if (tg1_status & FBIT(CAMCTL_TG_VS_INT_ST))
		raw->vsync_count++;

	if (raw->sub_sensor_ctrl_en
	    && tg1_status & FBIT(CAMCTL_TG_VS_INT_ORG_ST)
	    && raw->cur_vsync_idx >= 0) {
		if (is_sub_sample_sensor_timing(raw)) {
			raw->cur_vsync_idx = -1;
			irq_info.irq_type |= 1 << CAMSYS_IRQ_TRY_SENSOR_SET;
		}
		++raw->cur_vsync_idx;
	}

	if (irq_info.irq_type && !raw->is_slave) {
		if (push_msgfifo(raw, &irq_info) == 0)
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

		if (push_msgfifo(raw, &err_info) == 0)
			wake_thread = 1;
	}

	/* enable to debug fbc related */
	//if (debug_raw && debug_dump_fbc && (irq_status & SOF_INT_ST))
	//	mtk_cam_raw_dump_fbc(raw_dev->dev, raw_dev->base, raw_dev->yuv_base);

	/* trace */
	trace_raw_irq(dev, frame_idx_inner,
		      frame_status, tg1_status, cq_status,
		      dmao_done_status, dmai_done_status, dcif_status);
	trace_raw_dma_status(dev, frame_status, dma_ofl_status, dma_ufl_status);

#ifdef NOT_READY
	if (MTK_CAM_TRACE_ENABLED(FBC) && (tg1_status & TG_VS_INT_ORG_ST)) {
#ifdef DUMP_FBC_SEL_OUTER
		// TODO: QOF: need RTC to read register
		MTK_CAM_TRACE(FBC, "frame %d FBC_SEL 0x% 8x/0x% 8x (outer)",
			irq_info.frame_idx_inner,
			raw_readl_relaxed(raw, raw->base, REG_CAMCTL_FBC_SEL),
			raw_readl_relaxed(raw, raw->yuv_base, REG_CAMCTL_FBC_SEL));
#endif
		// TODO: QOF: need RTC to read register
		mtk_cam_raw_dump_fbc(dev, raw->base, raw->yuv_base);
	}
#endif

	return wake_thread ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static int raw_process_fsm(struct mtk_raw_device *raw_dev,
			   struct mtk_camsys_irq_info *irq_info,
			   int *recovered_done)
{
	struct engine_fsm *fsm = &raw_dev->fsm;
	int done_type, sof_type;
	int cookie_done;
	int ret;
	int recovered = 0, postponed = 0;

	sof_type = irq_info->irq_type &
		(BIT(CAMSYS_IRQ_FRAME_START) |
		BIT(CAMSYS_IRQ_FRAME_START_DCIF_MAIN));
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
		else if (sof_type && (irq_info->fbc_empty == 0))
			postponed = 1;
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
					   irq_info->frame_idx,
					   irq_info->fbc_empty,
					   recovered_done);
	if (postponed) {
		irq_info->cookie_done = engine_update_for_done(fsm);
		dev_info(raw_dev->dev, "postponed sof in/out: 0x%x 0x%x\n",
			 irq_info->frame_idx_inner,
			 irq_info->frame_idx);
	}

	if (recovered)
		dev_info(raw_dev->dev, "recovered done 0x%x in/out: 0x%x 0x%x\n",
			 *recovered_done,
			 irq_info->frame_idx_inner,
			 irq_info->frame_idx);

	return recovered;
}
#define LOG_THREADED_IRQ (100 * 1000000)
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

		if (irq_info.irq_type & BIT(CAMSYS_IRQ_FRAME_START) ||
			irq_info.irq_type & BIT(CAMSYS_IRQ_DEBUG_1) ||
			irq_info.irq_type & BIT(CAMSYS_IRQ_ERROR) ||
			irq_info.irq_type & BIT(CAMSYS_IRQ_FRAME_DONE)) {
			char *str_buf;
			size_t str_buf_size;

			str_buf = raw_dev->str_debug_irq_data;
			str_buf_size = sizeof(raw_dev->str_debug_irq_data);
			memset(str_buf, 0, str_buf_size);

			if ((irq_info.ts_ns - raw_dev->apply_ts) >= LOG_THREADED_IRQ ||
				raw_dev->log_en ||
				CAM_DEBUG_ENABLED(CTRL))
				dev_info(raw_dev->dev,
					"ts=%llu irq %d, req:0x%x/0x%x mod_5:0x%x td:%llu (0x%x/0x%x/0x%x) qof:0x%x\n",
					irq_info.ts_ns / 1000,
					irq_info.irq_type,
					irq_info.frame_idx_inner,
					irq_info.frame_idx,
					irq_info.debug_en,
					ktime_get_boottime_ns() - irq_info.ts_ns,
					raw_readl_relaxed(raw_dev, raw_dev->base_inner, REG_FHG_FHG_SPARE_1),
					raw_readl_relaxed(raw_dev, raw_dev->base, REG_FHG_FHG_SPARE_1),
					raw_readl_relaxed(raw_dev, raw_dev->base, REG_CAMCTL_MOD5_EN),
					qof_on_off_cnt(raw_dev));
			else
				scnprintf(str_buf, str_buf_size,
				"[%llu] ts=%llu irq %d, req:0x%x/0x%x en:0x%x td:%llu (0x%x/0x%x/0x%x) qof:0x%x",
				local_clock(),
				irq_info.ts_ns / 1000,
				irq_info.irq_type,
				irq_info.frame_idx_inner,
				irq_info.frame_idx,
				irq_info.debug_en,
				ktime_get_boottime_ns() - irq_info.ts_ns,
				raw_readl_relaxed(raw_dev, raw_dev->base_inner, REG_FHG_FHG_SPARE_1),
				raw_readl_relaxed(raw_dev, raw_dev->base, REG_FHG_FHG_SPARE_1),
				raw_readl_relaxed(raw_dev, raw_dev->base, REG_CAMCTL_MOD5_EN),
				qof_on_off_cnt(raw_dev));
		}

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
	if (cnt <= (3 + raw_dev->sub_sensor_ctrl_en * 10)) {
		struct mtk_yuv_device *yuv_dev = get_yuv_dev(raw_dev);

		qof_mtcmos_raw_voter(raw_dev, true);
		dump_topdebug_rdyreq_status(raw_dev);
		dump_raw_dma_err_st(raw_dev);
		dump_yuv_dma_err_st(yuv_dev);
		qof_mtcmos_raw_voter(raw_dev, false);
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

	qof_mtcmos_raw_voter(raw_dev, true);

	if (cnt < (OVERRUN_DUMP_CNT + raw_dev->sub_sensor_ctrl_en * 10))
		dump_topdebug_rdyreq_status(raw_dev);

	else if (cnt == (OVERRUN_DUMP_CNT + raw_dev->sub_sensor_ctrl_en * 10)) {
		dump_halt_setting(raw_dev);
		mtk_cam_bwr_dbg_dump(raw_dev->cam->bwr);
		mmdvfs_debug_status_dump(NULL);
#if KERNEL_VERSION(6, 7, 0) >= LINUX_VERSION_CODE
		mmqos_hrt_dump();
#endif
		do_engine_callback(raw_dev->engine_cb, reset_sensor,
				   raw_dev->cam, CAMSYS_ENGINE_RAW, raw_dev->id,
				   fh_cookie);
		do_engine_callback(raw_dev->engine_cb, dump_request,
				   raw_dev->cam, CAMSYS_ENGINE_RAW, raw_dev->id,
				   fh_cookie, MSG_TG_OVERRUN);
	}
	qof_mtcmos_raw_voter(raw_dev, false);
}

u32 basic_readl(struct mtk_raw_device *raw, void __iomem *base, u32 offset)
{
	(void)raw;

	return readl(base + offset);
}

u32 basic_readl_relaxed(struct mtk_raw_device *raw, void __iomem *base, u32 offset)
{
	(void)raw;

	return readl_relaxed(base + offset);
}

void basic_writel(struct mtk_raw_device *raw, u32 val, void __iomem *base, u32 offset)
{
	(void)raw;

	writel(val, base + offset);
}

void basic_writel_relaxed(struct mtk_raw_device *raw, u32 val, void __iomem *base, u32 offset)
{
	(void)raw;

	writel_relaxed(val, base + offset);
}

struct raw_io_ops basic_io_ops = {
	.readl = basic_readl,
	.readl_relaxed = basic_readl_relaxed,
	.writel = basic_writel,
	.writel_relaxed = basic_writel_relaxed,
};

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
	val = raw_readl(dev, dev->base, REG_TG_VF_CON);
	raw_writel(val & (~TG_VFDATA_EN), dev, dev->base, REG_TG_VF_CON);
	ret = readl_poll_timeout_atomic(dev->base + REG_TG_INTER_ST, val,
					(val & TG_CAM_CS_MASK) == TG_IDLE_ST,
					USEC_PER_MSEC, MTK_RAW_STOP_HW_TIMEOUT);
	if (ret)
		dev_dbg(dev->dev, "can't stop HW:%d:0x%x\n", ret, val);

	/* Disable CMOS */
	val = raw_readl(dev, dev->base, REG_TG_SEN_MODE);
	raw_writel(val & (~TG_SEN_MODE_CMOS_EN), dev, dev->base, REG_TG_SEN_MODE);
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
	val = raw_readl(dev, dev->base, REG_TG_SEN_MODE);
	raw_writel(val | TG_SEN_MODE_CMOS_EN, dev, dev->base, REG_TG_SEN_MODE);

	/* Enable VF */
	val = raw_readl(dev->base + REG_TG_VF_CON);
	raw_writel(val | TG_VFDATA_EN, dev, dev->base, REG_TG_VF_CON);
#endif

	return 0;
}

int cg_dump_and_test(struct device *dev, int type, bool test)
{
	struct mtk_raw_device *raw;
	struct mtk_raw_device *yuv;
	struct mtk_rms_device *rms;
	void __iomem *dev_iomem;

	if (type == CG_RAW) {
		raw = dev_get_drvdata(dev);
		if (raw->id == 0)
			dev_iomem = raw->cam->rawa_cg_con;
		else if (raw->id == 1)
			dev_iomem = raw->cam->rawb_cg_con;
		else if (raw->id == 2)
			dev_iomem = raw->cam->rawc_cg_con;
		else
			return -1;
	} else if (type == CG_YUV) {
		yuv = dev_get_drvdata(dev);
		if (yuv->id == 0)
			dev_iomem = yuv->cam->yuva_cg_con;
		else if (yuv->id == 1)
			dev_iomem = yuv->cam->yuvb_cg_con;
		else if (yuv->id == 2)
			dev_iomem = yuv->cam->yuvc_cg_con;
		else
			return -1;
	} else if (type == CG_RMS) {
		rms = dev_get_drvdata(dev);
		if (rms->id == 0)
			dev_iomem = rms->cam->rmsa_cg_con;
		else if (rms->id == 1)
			dev_iomem = rms->cam->rmsb_cg_con;
		else if (rms->id == 2)
			dev_iomem = rms->cam->rmsc_cg_con;
		else
			return -1;
	} else {
		pr_info("wrong type!");
		return -1;
	}
	if (test) {
		dev_info(dev, "%s-status:cg:0x%x",
			__func__, readl(dev_iomem));
		writel(0x1, dev_iomem + 0x4);
		wmb(); /* make sure committed */
		dev_info(dev, "%s-status(after write cg set):cg:0x%x",
			__func__, readl(dev_iomem));
		writel(0x1, dev_iomem + 0x8);
		wmb(); /* make sure committed */
		dev_info(dev, "%s-status(after write cg clr):cg:0x%x",
			__func__, readl(dev_iomem));
	} else {
		dev_info(dev, "%s-status:cg:0x%x",
			__func__, readl(dev_iomem));
	}

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
	raw->base_reg_addr = res->start;

	/* base inner register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}
	raw->base_inner_reg_addr = res->start;

	raw->base_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(raw->base_inner)) {
		dev_dbg(dev, "failed to map register inner base\n");
		return PTR_ERR(raw->base_inner);
	}
	/* dmatop base register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dmatop_base");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	raw->dmatop_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(raw->dmatop_base)) {
		dev_dbg(dev, "failed to map register dmatop_base\n");
		return PTR_ERR(raw->dmatop_base);
	}
	/* dmatop base inner register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_dmatop_base");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	raw->dmatop_base_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(raw->dmatop_base_inner)) {
		dev_dbg(dev, "failed to map register dmatop_base_inner\n");
		return PTR_ERR(raw->dmatop_base_inner);
	}
	/* qof base register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "qof_base");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	raw->qof_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(raw->qof_base)) {
		dev_dbg(dev, "failed to map register qof_base\n");
		return PTR_ERR(raw->qof_base);
	}

	if (GET_PLAT_HW(snoc_support)) {
		raw->larb_vcsel = ioremap(REG_CAM_RAW_LARB_VCSEL +
				(phys_addr_t) raw->id * LARB_VCSEL_OFFSET, 0x4);
		if (IS_ERR(raw->larb_vcsel)) {
			dev_err(dev, "%s: failed to map larb_vcsel\n", __func__);
			raw->larb_vcsel = NULL;
		}
	} else
		raw->larb_vcsel = NULL;

	/* will be assigned later */
	raw->yuv_base = NULL;

	raw->irq = platform_get_irq(pdev, 0);
	if (raw->irq < 0) {
		dev_dbg(dev, "failed to get irq\n");
		return -ENODEV;
	}

	ret = devm_request_threaded_irq(dev, raw->irq,
					mtk_irq_raw_yuv,
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

static int mtk_raw_smi_pwr_get(void *data)
{
	struct mtk_raw_device *raw = data;

	if (raw)
		qof_mtcmos_raw_voter(raw, true);

	return 0;
}

static int mtk_raw_smi_pwr_get_if_in_use(void *data)
{
	struct mtk_raw_device *raw = data;

	if (raw && pm_runtime_active(raw->dev)) {
		qof_mtcmos_raw_voter(raw, true);
		return 1;
	}

	return 0;
}

static int mtk_raw_smi_pwr_put(void *data)
{
	struct mtk_raw_device *raw = data;

	if (raw && pm_runtime_active(raw->dev)) {
		qof_mtcmos_raw_voter(raw, false);
		return 1;
	}

	return 0;
}

static struct smi_user_pwr_ctrl smi_raw_a_pwr_cb = {
	 .name = "cam_rawa_cb",
	 .data = NULL,
	 .smi_user_id =  MTK_SMI_CAM_RAWA,
	 .smi_user_get = mtk_raw_smi_pwr_get,
	 .smi_user_get_if_in_use = mtk_raw_smi_pwr_get_if_in_use,
	 .smi_user_put = mtk_raw_smi_pwr_put,
};

static struct smi_user_pwr_ctrl smi_raw_b_pwr_cb = {
	 .name = "cam_rawb_cb",
	 .data = NULL,
	 .smi_user_id =  MTK_SMI_CAM_RAWB,
	 .smi_user_get = mtk_raw_smi_pwr_get,
	 .smi_user_get_if_in_use = mtk_raw_smi_pwr_get_if_in_use,
	 .smi_user_put = mtk_raw_smi_pwr_put,
};

static struct smi_user_pwr_ctrl smi_raw_c_pwr_cb = {
	 .name = "cam_rawc_cb",
	 .data = NULL,
	 .smi_user_id =  MTK_SMI_CAM_RAWC,
	 .smi_user_get = mtk_raw_smi_pwr_get,
	 .smi_user_get_if_in_use = mtk_raw_smi_pwr_get_if_in_use,
	 .smi_user_put = mtk_raw_smi_pwr_put,
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

	raw_dev->io_ops = &basic_io_ops;

	raw_dev->fifo_size =
		roundup_pow_of_two(8 * sizeof(struct mtk_camsys_irq_info));

	raw_dev->msg_buffer = devm_kzalloc(dev, raw_dev->fifo_size, GFP_KERNEL);
	if (!raw_dev->msg_buffer) {
		ret = -ENOMEM;
		goto UNREGISTER_PM_NOTIFIER;
	}

	raw_dev->default_printk_cnt = get_detect_count();

	raw_dev->apmcu_voter_cnt = 0;
	spin_lock_init(&raw_dev->apmcu_voter_lock);
	spin_lock_init(&raw_dev->qof_ctrl_lock);

	pm_runtime_enable(dev);

	ret = component_add(dev, &mtk_raw_component_ops);
	if (ret)
		goto UNREGISTER_PM_NOTIFIER;

	switch (raw_dev->id) {
	case RAW_A:
		smi_raw_a_pwr_cb.data = raw_dev;
		mtk_smi_dbg_register_pwr_ctrl_cb(&smi_raw_a_pwr_cb);
		break;
	case RAW_B:
		smi_raw_b_pwr_cb.data = raw_dev;
		mtk_smi_dbg_register_pwr_ctrl_cb(&smi_raw_b_pwr_cb);
		break;
	case RAW_C:
		smi_raw_c_pwr_cb.data = raw_dev;
		mtk_smi_dbg_register_pwr_ctrl_cb(&smi_raw_c_pwr_cb);
		break;
	default:
		break;
	}

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

	switch (raw_dev->id) {
	case RAW_A:
		smi_raw_a_pwr_cb.data = NULL;
		mtk_smi_dbg_unregister_pwr_ctrl_cb(&smi_raw_a_pwr_cb);
		break;
	case RAW_B:
		smi_raw_b_pwr_cb.data = NULL;
		mtk_smi_dbg_unregister_pwr_ctrl_cb(&smi_raw_b_pwr_cb);
		break;
	case RAW_C:
		smi_raw_c_pwr_cb.data = NULL;
		mtk_smi_dbg_unregister_pwr_ctrl_cb(&smi_raw_c_pwr_cb);
		break;
	default:
		break;
	}

	pm_runtime_disable(dev);
	mtk_cam_qos_remove(&raw_dev->qos);
	component_del(dev, &mtk_raw_component_ops);

	for (i = 0; i < raw_dev->num_clks; i++)
		clk_put(raw_dev->clks[i]);

	return 0;
}

int mtk_raw_runtime_suspend(struct device *dev)
{
	struct mtk_raw_device *drvdata = dev_get_drvdata(dev);
	int i;
	unsigned int pr_detect_count;

	dev_info(dev, "%s:disable clock\n", __func__);

	pr_detect_count = get_detect_count();
	if (pr_detect_count > drvdata->default_printk_cnt)
		set_detect_count(drvdata->default_printk_cnt);

	mtk_cam_reset_qos(dev, &drvdata->qos);
	mtk_cam_bwr_clr_bw(drvdata->cam->bwr,
		get_bwr_engine(drvdata->id), get_axi_port(drvdata->id, true));

	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_RAW, 0);
	for (i = drvdata->num_clks - 1; i >= 0; i--)
		clk_disable_unprepare(drvdata->clks[i]);
	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_RAW, 0);
	mtk_mmdvfs_enable_vcp(false, VCP_PWR_USR_CAM);

	return 0;
}

int mtk_raw_runtime_resume(struct device *dev)
{
	struct mtk_raw_device *drvdata = dev_get_drvdata(dev);
	int i, ret;
	unsigned int pr_detect_count;

	/* reset_msgfifo before enable_irq */
	ret = mtk_cam_raw_reset_msgfifo(drvdata);
	if (ret)
		return ret;

	pr_detect_count = get_detect_count();
	if (pr_detect_count < KERNEL_LOG_MAX)
		set_detect_count(KERNEL_LOG_MAX);
	dev_info(dev, "%s:enable clock\n", __func__);
	mtk_mmdvfs_enable_vcp(true, VCP_PWR_USR_CAM);
	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_RAW, 1);
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
	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_RAW, 0);

	reset(drvdata);
	enable_irq(drvdata->irq);

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
	/* dmatop base outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dmatop_base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	drvdata->dmatop_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(drvdata->dmatop_base)) {
		dev_dbg(dev, "failed to map register dmatop_base\n");
		return PTR_ERR(drvdata->dmatop_base);
	}

	/* base inner register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_dmatop_base");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	drvdata->dmatop_base_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(drvdata->dmatop_base_inner)) {
		dev_dbg(dev, "failed to map register dmatop_base_inner\n");
		return PTR_ERR(drvdata->dmatop_base_inner);
	}

	if (GET_PLAT_HW(snoc_support)) {
		drvdata->larb_vcsel = ioremap(REG_CAM_YUV_LARB_VCSEL +
					(phys_addr_t) drvdata->id * LARB_VCSEL_OFFSET, 0x4);
		if (IS_ERR(drvdata->larb_vcsel)) {
			dev_err(dev, "%s: failed to map larb_vcsel\n", __func__);
			drvdata->larb_vcsel = NULL;
		}
	} else
		drvdata->larb_vcsel = NULL;

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
int mtk_yuv_runtime_suspend(struct device *dev)
{
	struct mtk_yuv_device *drvdata = dev_get_drvdata(dev);
	int i;

	if (CAM_DEBUG_ENABLED(RAW_CG))
		dev_dbg(dev, "%s:disable clock\n", __func__);
	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_YUV, 0);

	mtk_cam_reset_qos(dev, &drvdata->qos);
	mtk_cam_bwr_clr_bw(drvdata->cam->bwr,
		get_bwr_engine(drvdata->id), get_axi_port(drvdata->id, false));

	for (i = drvdata->num_clks - 1; i >= 0; i--)
		clk_disable_unprepare(drvdata->clks[i]);
	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_YUV, 0);
	return 0;
}

int mtk_yuv_runtime_resume(struct device *dev)
{
	struct mtk_yuv_device *drvdata = dev_get_drvdata(dev);
	int i, ret;

	if (CAM_DEBUG_ENABLED(RAW_CG))
		dev_dbg(dev, "%s:enable clock\n", __func__);
	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_YUV, 1);
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
	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_YUV, 0);

	return 0;
}

void print_cq_settings(struct mtk_raw_device *raw, void __iomem *base)
{
	unsigned int inner_addr, inner_addr_msb, size;

	inner_addr = raw_readl_relaxed(raw, base, REG_CAMCQ_CQ_THR0_BASEADDR);
	inner_addr_msb = raw_readl_relaxed(raw, base, REG_CAMCQ_CQ_THR0_BASEADDR_MSB);
	size = raw_readl_relaxed(raw, base, REG_CAMCQ_CQ_THR0_DESC_SIZE);

	pr_info("CQ_THR0_inner_addr_msb:0x%x, CQ_THR0_inner_addr:%08x, size:0x%x\n",
		inner_addr_msb, inner_addr, size);

	inner_addr = raw_readl_relaxed(raw, base, REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2);
	inner_addr_msb = raw_readl_relaxed(raw, base, REG_CAMCQ_CQ_SUB_THR0_BASEADDR_2_MSB);
	size = raw_readl_relaxed(raw, base, REG_CAMCQ_CQ_SUB_THR0_DESC_SIZE_2);

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

	// TODO: QOF readl ops?
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

int mtk_raw_translation_fault_cb(int port, dma_addr_t mva, void *data)
{
	struct mtk_raw_device *raw = (struct mtk_raw_device *)data;
	unsigned int fh_cookie =
			raw_readl_relaxed(raw, raw->base_inner, REG_FRAME_IDX);
	unsigned int m4u_port = MTK_M4U_TO_PORT(port);
	struct dma_group group;
	int i;

	if (m4u_port == 0 || m4u_port == 1) { /* cq info */
		print_cq_settings(raw, raw->dmatop_base_inner);
	}

	if (CALL_PLAT_HW(query_raw_dma_group, m4u_port, &group))
		return 0;

	for (i = 0; i < ARRAY_SIZE(group.dma); i++) {
		if (group.dma[i] == 0x0)
			continue;

		print_dma_settings(raw->dmatop_base_inner,  group.dma[i]);
	}

	do_engine_callback(raw->engine_cb, dump_request,
		   raw->cam, CAMSYS_ENGINE_RAW, raw->id,
		   fh_cookie, MSG_M4U_TF);

	return 0;
}

int mtk_yuv_translation_fault_cb(int port, dma_addr_t mva, void *data)
{
	struct mtk_yuv_device *yuv = (struct mtk_yuv_device *)data;
	struct mtk_raw_device *raw = get_raw_dev(yuv);
	unsigned int fh_cookie =
			raw_readl_relaxed(raw, raw->base_inner, REG_FRAME_IDX);
	unsigned int m4u_port = MTK_M4U_TO_PORT(port);
	struct dma_group group;
	int i;

	if (CALL_PLAT_HW(query_yuv_dma_group, m4u_port, &group))
		return 0;

	for (i = 0; i < ARRAY_SIZE(group.dma); i++) {
		if (group.dma[i] == 0x0)
			continue;

		print_dma_settings(yuv->dmatop_base_inner, group.dma[i]);
	}

	do_engine_callback(raw->engine_cb, dump_request,
		   raw->cam, CAMSYS_ENGINE_RAW, raw->id,
		   fh_cookie, MSG_M4U_TF);

	return 0;
}

void fill_aa_info(struct mtk_raw_device *raw,
				  struct mtk_ae_debug_data *ae_info)
{
	struct mtk_rms_device *rms = get_rms_dev(raw);

	ae_info->OBC_R1_Sum[0] +=
		((u64)raw_readl(raw, raw->base, OFFSET_OBC_R1_R_SUM_H) << 32) |
		raw_readl(raw, raw->base, OFFSET_OBC_R1_R_SUM_L);
	ae_info->OBC_R2_Sum[0] +=
		((u64)raw_readl(raw, rms->base, OFFSET_OBC_R2_R_SUM_H) << 32) |
		raw_readl(raw, rms->base, OFFSET_OBC_R2_R_SUM_L);
	ae_info->OBC_R3_Sum[0] +=
		((u64)raw_readl(raw, rms->base, OFFSET_OBC_R3_R_SUM_H) << 32) |
		raw_readl(raw, rms->base, OFFSET_OBC_R3_R_SUM_L);
	ae_info->LTM_Sum[0] +=
		((u64)raw_readl(raw, raw->base, REG_LTM_AE_DEBUG_R_MSB) << 32) |
		raw_readl(raw, raw->base, REG_LTM_AE_DEBUG_R_LSB);


	ae_info->OBC_R1_Sum[1] +=
		((u64)raw_readl(raw, raw->base, OFFSET_OBC_R1_B_SUM_H) << 32) |
		raw_readl(raw, raw->base, OFFSET_OBC_R1_B_SUM_L);
	ae_info->OBC_R2_Sum[1] +=
		((u64)raw_readl(raw, rms->base, OFFSET_OBC_R2_B_SUM_H) << 32) |
		raw_readl(raw, rms->base, OFFSET_OBC_R2_B_SUM_L);
	ae_info->OBC_R3_Sum[1] +=
		((u64)raw_readl(raw, rms->base, OFFSET_OBC_R3_B_SUM_H) << 32) |
		raw_readl(raw, rms->base, OFFSET_OBC_R3_B_SUM_L);
	ae_info->LTM_Sum[1] +=
		((u64)raw_readl(raw, raw->base, REG_LTM_AE_DEBUG_B_MSB) << 32) |
		raw_readl(raw, raw->base, REG_LTM_AE_DEBUG_B_LSB);


	ae_info->OBC_R1_Sum[2] +=
		((u64)raw_readl(raw, raw->base, OFFSET_OBC_R1_GR_SUM_H) << 32) |
		raw_readl(raw, raw->base, OFFSET_OBC_R1_GR_SUM_L);
	ae_info->OBC_R2_Sum[2] +=
		((u64)raw_readl(raw, rms->base, OFFSET_OBC_R2_GR_SUM_H) << 32) |
		raw_readl(raw, rms->base, OFFSET_OBC_R2_GR_SUM_L);
	ae_info->OBC_R3_Sum[2] +=
		((u64)raw_readl(raw, rms->base, OFFSET_OBC_R3_GR_SUM_H) << 32) |
		raw_readl(raw, rms->base, OFFSET_OBC_R3_GR_SUM_L);
	ae_info->LTM_Sum[2] +=
		((u64)raw_readl(raw, raw->base, REG_LTM_AE_DEBUG_GR_MSB) << 32) |
		raw_readl(raw, raw->base, REG_LTM_AE_DEBUG_GR_LSB);


	ae_info->OBC_R1_Sum[3] +=
		((u64)raw_readl(raw, raw->base, OFFSET_OBC_R1_GB_SUM_H) << 32) |
		raw_readl(raw, raw->base, OFFSET_OBC_R1_GB_SUM_L);
	ae_info->OBC_R2_Sum[3] +=
		((u64)raw_readl(raw, rms->base, OFFSET_OBC_R2_GB_SUM_H) << 32) |
		raw_readl(raw, rms->base, OFFSET_OBC_R2_GB_SUM_L);
	ae_info->OBC_R3_Sum[3] +=
		((u64)raw_readl(raw, rms->base, OFFSET_OBC_R3_GB_SUM_H) << 32) |
		raw_readl(raw, rms->base, OFFSET_OBC_R3_GB_SUM_L);
	ae_info->LTM_Sum[3] +=
		((u64)raw_readl(raw, raw->base, REG_LTM_AE_DEBUG_GB_MSB) << 32) |
		raw_readl(raw, raw->base, REG_LTM_AE_DEBUG_GB_LSB);
	/* [0]~[3] is non clipped data; [4]~[7] is clipped data */
	ae_info->AESTAT_Sum[0] +=
		((u64)raw_readl(raw, raw->base, REG_AA_R_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_AA_R_SUM_L);
	ae_info->AESTAT_Sum[1] +=
		((u64)raw_readl(raw, raw->base, REG_AA_B_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_AA_B_SUM_L);
	ae_info->AESTAT_Sum[2] +=
		((u64)raw_readl(raw, raw->base, REG_AA_GR_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_AA_GR_SUM_L);
	ae_info->AESTAT_Sum[3] +=
		((u64)raw_readl(raw, raw->base, REG_AA_GB_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_AA_GB_SUM_L);

	ae_info->AESTAT_Sum[4] +=
		((u64)raw_readl(raw, raw->base, REG_AA_R_CLIP_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_AA_R_CLIP_SUM_L);
	ae_info->AESTAT_Sum[5] +=
		((u64)raw_readl(raw, raw->base, REG_AA_B_CLIP_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_AA_B_CLIP_SUM_L);
	ae_info->AESTAT_Sum[6] +=
		((u64)raw_readl(raw, raw->base, REG_AA_GR_CLIP_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_AA_GR_CLIP_SUM_L);
	ae_info->AESTAT_Sum[7] +=
		((u64)raw_readl(raw, raw->base, REG_AA_GB_CLIP_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_AA_GB_CLIP_SUM_L);

	/* [0]~[3] is non clipped data; [4]~[7] is clipped data */
	ae_info->DGN_Sum[0] +=
		((u64)raw_readl(raw, raw->base, REG_DGN_R_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_DGN_R_SUM_L);
	ae_info->DGN_Sum[1] +=
		((u64)raw_readl(raw, raw->base, REG_DGN_B_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_DGN_B_SUM_L);
	ae_info->DGN_Sum[2] +=
		((u64)raw_readl(raw, raw->base, REG_DGN_GR_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_DGN_GR_SUM_L);
	ae_info->DGN_Sum[3] +=
		((u64)raw_readl(raw, raw->base, REG_DGN_GB_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_DGN_GB_SUM_L);

	ae_info->DGN_Sum[4] +=
		((u64)raw_readl(raw, raw->base, REG_DGN_R_CLIP_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_DGN_R_CLIP_SUM_L);
	ae_info->DGN_Sum[5] +=
		((u64)raw_readl(raw, raw->base, REG_DGN_B_CLIP_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_DGN_B_CLIP_SUM_L);
	ae_info->DGN_Sum[6] +=
		((u64)raw_readl(raw, raw->base, REG_DGN_GR_CLIP_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_DGN_GR_CLIP_SUM_L);
	ae_info->DGN_Sum[7] +=
		((u64)raw_readl(raw, raw->base, REG_DGN_GB_CLIP_SUM_H) << 32) |
		raw_readl(raw, raw->base, REG_DGN_GB_CLIP_SUM_L);

	ae_info->CCM_Sum[0] +=
		((u64)raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_R_LSB) << 32) |
		raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_R_MSB);
	ae_info->CCM_Sum[1] +=
		((u64)raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_B_LSB) << 32) |
		raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_B_MSB);
	ae_info->CCM_Sum[2] +=
		((u64)raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_G_LSB) << 32) |
		raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_G_MSB);
	ae_info->CCM_Sum[3] = 0x0;
	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(raw->dev, "[%s] 0x%x/0x%x, 0x%x/0x%x, 0x%x/0x%x\n", __func__,
		raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_R_LSB),
		raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_R_MSB),
		raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_B_LSB),
		raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_B_MSB),
		raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_G_LSB),
		raw_readl(raw, raw->yuv_base, REG_CCM_AE_DEBUG_G_MSB));
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
	drvdata->cam = cam_dev;
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

int mtk_rms_runtime_suspend(struct device *dev)
{
	struct mtk_rms_device *drvdata = dev_get_drvdata(dev);
	int i;

	if (CAM_DEBUG_ENABLED(RAW_CG))
		dev_dbg(dev, "%s:disable clock\n", __func__);
	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_RMS, 0);
	for (i = drvdata->num_clks - 1; i >= 0; i--)
		clk_disable_unprepare(drvdata->clks[i]);
	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_RMS, 0);
	return 0;
}

int mtk_rms_runtime_resume(struct device *dev)
{
	struct mtk_rms_device *drvdata = dev_get_drvdata(dev);
	int i, ret;

	if (CAM_DEBUG_ENABLED(RAW_CG))
		dev_info(dev, "%s:enable clock\n", __func__);

	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_RMS, 1);

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
	if (CAM_DEBUG_ENABLED(RAW_CG))
		cg_dump_and_test(dev, CG_RMS, 0);

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

	return raw_id + cammux_id_raw_start;
}

//#define DEBUG_RAWI_R5
int raw_dump_debug_status(struct mtk_raw_device *dev, int dma_debug_dump)
{
	int need_smi_dump;

	qof_mtcmos_raw_voter(dev, true);

	dump_seqence(dev);
	dump_dc_setting(dev);
	dump_cq_setting(dev);
	dump_tg_setting(dev, "debug");
	dump_dmatop_dc_st(dev);
	dump_interrupt(dev);
	dump_rms_reg(dev);
	dump_dmai_reg(dev);
	dump_ae_reg(dev, 1);
	dump_awb_reg(dev, 1);
	dump_af_reg(dev, 1);
	qof_force_dump_all(dev);

	if (dma_debug_dump) {
		dump_topdebug_rdyreq_status(dev);

		if (dma_debug_dump & DD_DUMP_SRT)
			raw_dump_debug_ufbc_status(dev);

		if (dma_debug_dump & DD_DUMP_CQ)
			raw_dump_debug_cqi_status(dev);
	}

#ifdef DEBUG_RAWI_R5
	mtk_cam_dump_dma_debug(dev,
			       dev->base + 0x4000, /* DMATOP_BASE */
			       "RAWI_R5",
			       dbg_RAWI_R5, ARRAY_SIZE(dbg_RAWI_R5));
#endif

	/* avoid rdma TF */
	clear_reg(dev);
	qof_mtcmos_raw_voter(dev, false);

	need_smi_dump = dev->tg_overrun_handle_cnt > 0 ? 1 : 0;

	return need_smi_dump;
}
