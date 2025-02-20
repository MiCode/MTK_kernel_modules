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
#include "mtk_cam_ut.h"
#include "mtk_cam_ut-engines.h"
#include "../cam/mtk_cam-sv-regs.h"
#define CAMSV_WRITE_BITS(RegAddr, RegName, FieldName, FieldValue) do {\
	union RegName reg;\
	\
	reg.Raw = readl_relaxed(RegAddr);\
	reg.Bits.FieldName = FieldValue;\
	writel_relaxed(reg.Raw, RegAddr);\
} while (0)
#define CAMSV_WRITE_REG(RegAddr, RegValue) ({\
	writel_relaxed(RegValue, RegAddr);\
})
#define CAMSV_READ_BITS(RegAddr, RegName, FieldName) ({\
	union RegName reg;\
	\
	reg.Raw = readl_relaxed(RegAddr);\
	reg.Bits.FieldName;\
})
#define CAMSV_READ_REG(RegAddr) ({\
	unsigned int var;\
	\
	var = readl_relaxed(RegAddr);\
	var;\
})
#define CAMSV_IRQ_NUM 4
enum camsv_db_load_src {
	SV_DB_SRC_SUB_P1_DONE = 0,
	SV_DB_SRC_SOF         = 1,
	SV_DB_SRC_SUB_SOF     = 2,
};
enum camsv_module_id {
	CAMSV_START = 0,
	CAMSV_0 = CAMSV_START,
	CAMSV_1 = 1,
	CAMSV_2 = 2,
	CAMSV_3 = 3,
	CAMSV_4 = 4,
	CAMSV_5 = 5,
	CAMSV_6 = 6,
	CAMSV_7 = 7,
	CAMSV_8 = 8,
	CAMSV_9 = 9,
	CAMSV_10 = 10,
	CAMSV_11 = 11,
	CAMSV_12 = 12,
	CAMSV_13 = 13,
	CAMSV_14 = 14,
	CAMSV_15 = 15,
	CAMSV_END
};
enum camsv_int_en {
	SV_INT_EN_VS1_INT_EN               = (1L<<0),
	SV_INT_EN_TG_INT1_EN               = (1L<<1),
	SV_INT_EN_TG_INT2_EN               = (1L<<2),
	SV_INT_EN_EXPDON1_INT_EN           = (1L<<3),
	SV_INT_EN_TG_ERR_INT_EN            = (1L<<4),
	SV_INT_EN_TG_GBERR_INT_EN          = (1L<<5),
	SV_INT_EN_TG_SOF_INT_EN            = (1L<<6),
	SV_INT_EN_TG_WAIT_INT_EN           = (1L<<7),
	SV_INT_EN_TG_DROP_INT_EN           = (1L<<8),
	SV_INT_EN_VS_INT_ORG_EN            = (1L<<9),
	SV_INT_EN_DB_LOAD_ERR_EN           = (1L<<10),
	SV_INT_EN_PASS1_DON_INT_EN         = (1L<<11),
	SV_INT_EN_SW_PASS1_DON_INT_EN      = (1L<<12),
	SV_INT_EN_SUB_PASS1_DON_INT_EN     = (1L<<13),
	SV_INT_EN_UFEO_OVERR_INT_EN        = (1L<<15),
	SV_INT_EN_DMA_ERR_INT_EN           = (1L<<16),
	SV_INT_EN_IMGO_OVERR_INT_EN        = (1L<<17),
	SV_INT_EN_UFEO_DROP_INT_EN         = (1L<<18),
	SV_INT_EN_IMGO_DROP_INT_EN         = (1L<<19),
	SV_INT_EN_IMGO_DONE_INT_EN         = (1L<<20),
	SV_INT_EN_UFEO_DONE_INT_EN         = (1L<<21),
	SV_INT_EN_TG_INT3_EN               = (1L<<22),
	SV_INT_EN_TG_INT4_EN               = (1L<<23),
	SV_INT_EN_SW_ENQUE_ERR             = (1L<<24),
	SV_INT_EN_INT_WCLR_EN              = (1L<<31),
};
enum camsv_tg_fmt {
	SV_TG_FMT_RAW8      = 0,
	SV_TG_FMT_RAW10     = 1,
	SV_TG_FMT_RAW12     = 2,
	SV_TG_FMT_YUV422    = 3,
	SV_TG_FMT_RAW14     = 4,
	SV_TG_FMT_RSV1      = 5,
	SV_TG_FMT_RSV2      = 6,
	SV_TG_FMT_JPG       = 7,
};
enum camsv_tg_swap {
	TG_SW_UYVY = 0,
	TG_SW_YUYV = 1,
	TG_SW_VYUY = 2,
	TG_SW_YVYU = 3,
};
enum camsv_tag_idx {
	SVTAG_START = 0,
	SVTAG_IMG_START = SVTAG_START,
	SVTAG_0 = SVTAG_IMG_START,
	SVTAG_1,
	SVTAG_2,
	SVTAG_3,
	SVTAG_IMG_END,
	SVTAG_META_START = SVTAG_IMG_END,
	SVTAG_4 = SVTAG_META_START,
	SVTAG_5,
	SVTAG_6,
	SVTAG_7,
	SVTAG_META_END,
	SVTAG_END = SVTAG_META_END,
};
int ut_mtk_cam_sv_toggle_db(
	struct device *dev)
{
	int ret = 0;
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	int val, val2;

	val = CAMSV_READ_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_MODULE_DB);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_MODULE_DB,
		CAMSVCENTRAL_MODULE_DB, CAM_DB_EN, 0);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_MODULE_DB,
		CAMSVCENTRAL_MODULE_DB, CAM_DB_EN, 1);
	val2 = CAMSV_READ_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_MODULE_DB);
	dev_info(sv_dev->dev, "%s 0x%x->0x%x\n", __func__, val, val2);
	return ret;
}
int ut_sv_reset(struct device *dev)
{
	unsigned long end = jiffies + msecs_to_jiffies(100);
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	int i;

	dev_info(dev, "%s: sw rst\n", __func__);
	writel_relaxed(0, sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL);
	writel_relaxed(1, sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL);
	wmb(); /* TBC */
	while (time_before(jiffies, end)) {
		if (readl(sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL) & 0x2)
			break;
		usleep_range(10, 20);
	}
	/* enable dma dcm after dma is idle */
	writel_relaxed(0, sv_dev->base + REG_CAMSVCENTRAL_DCM_DIS);
	writel_relaxed(0, sv_dev->base + REG_CAMSVCENTRAL_SW_CTL);
	writel_relaxed(1, sv_dev->base + REG_CAMSVCENTRAL_SW_CTL);
	writel_relaxed(0, sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL);
	writel_relaxed(0, sv_dev->base + REG_CAMSVCENTRAL_SW_CTL);
	wmb(); /* make sure committed */
	/* reset cq dma */
	writel_relaxed(0, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL);
	writel_relaxed(1, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL);
	wmb(); /* make sure committed */
	while (time_before(jiffies, end)) {
		if (readl(sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL) & 0x2)
			break;
		usleep_range(10, 20);
	}
	writel_relaxed(0, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL);
	wmb(); /* make sure committed */
	/* reset cq */
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_CQ_RESET, 1);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_EN,
		CAMSVCQ_CQ_SUB_EN, CAMSVCQ_CQ_SUB_RESET, 1);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_EN,
		CAMSVCQ_CQ_SUB_EN, CAMSVCQ_CQ_SUB_RESET, 0);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_CQ_RESET, 0);
	/* avoid camsv tag data leakage */
	for (i = SVTAG_START; i < SVTAG_END; i++) {
		writel(0, sv_dev->base + REG_CAMSVCENTRAL_GRAB_PXL_TAG1 +
			CAMSVCENTRAL_GRAB_PXL_TAG_SHIFT * i);
		writel(0, sv_dev->base + REG_CAMSVCENTRAL_GRAB_LIN_TAG1 +
			CAMSVCENTRAL_GRAB_LIN_TAG_SHIFT * i);
	}
	wmb();/* make sure committed */
	dev_info(dev, "reset hw done\n");
	return 0;
}
int ut_mtk_cam_sv_toggle_tg_db(
	struct device *dev)
{
	int ret = 0;
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	int val, val2;

	val = CAMSV_READ_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_PATH_CFG);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_PATH_CFG,
		CAMSVCENTRAL_PATH_CFG, SYNC_VF_EN_DB_LOAD_DIS, 1);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_PATH_CFG,
		CAMSVCENTRAL_PATH_CFG, SYNC_VF_EN_DB_LOAD_DIS, 0);
	val2 = CAMSV_READ_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_PATH_CFG);
	dev_info(sv_dev->dev, "%s 0x%x->0x%x\n", __func__, val, val2);
	return ret;
}
int ut_mtk_cam_sv_fbc_disable(
	struct device *dev)
{
	int ret = 0, tag_idx;
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	/* camsv todo: close all fbc? */
	for (tag_idx = SVTAG_START; tag_idx < SVTAG_END; tag_idx++)
		CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_FBC0_TAG1 +
			CAMSVCENTRAL_FBC0_TAG_SHIFT * tag_idx, 0);
	return ret;
}
int ut_mtk_camsv_central_common_disable(struct device *dev)
{
	int ret = 0;
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	/* disable dma dcm before do dma reset */
	writel(1, sv_dev->base + REG_CAMSVCENTRAL_DCM_DIS);
	/* bypass tg_mode function before vf off */
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, TG_MODE_OFF, 1);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_VF_CON,
		CAMSVCENTRAL_VF_CON, VFDATA_EN, 0);
	ut_mtk_cam_sv_toggle_tg_db(dev);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DONE_STATUS_EN, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_ERR_STATUS_EN, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_SOF_STATUS_EN, 0);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, CMOS_EN, 0);
	ut_sv_reset(dev);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DMA_EN_IMG, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_E_CAMSVCENTRAL_DCIF_SET, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_E_CAMSVCENTRAL_DCIF_SEL, 0);
	ut_mtk_cam_sv_toggle_db(dev);
	return ret;
}
int ut_mtk_camsv_cq_disable(
	struct device *dev)
{
	int ret = 0;
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	/* camsv todo : camsv support subsample
	 *int i, subsample = 0;
	 *
	 *for (i = SVTAG_START; i < SVTAG_END; i++) {
	 *	if (camsv_dev->enabled_tags & (1 << i)) {
	 *		subsample = camsv_dev->tag_info[i].cfg_in_param.subsample;
	 *		break;
	 *	}
	 *}
	 *
	 *if (subsample) {
	 *	CAMSV_WRITE_BITS(camsv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
	 *		CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_SUBSAMPLE_EN, 0);
	 *}
	 */
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_EN, 0);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_STAGGER_MODE, 0);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_MODE, 0);
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_SCQ_START_PERIOD, 0);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_INT_0_EN,
		CAMSVCQTOP_INT_0_EN, CAMSVCQTOP_CSR_SCQ_SUB_THR_DONE_INT_EN, 0);
	return ret;
}
int ut_mtk_camsv_central_common_enable(
	struct device *dev)
{
	int ret = 0;
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	ut_mtk_cam_sv_toggle_db(dev);
	ut_mtk_cam_sv_toggle_tg_db(dev);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, CMOS_EN, 1);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_VF_CON,
		CAMSVCENTRAL_VF_CON, VFDATA_EN, 1);
	dev_info(sv_dev->dev, "%s sen_mode:0x%x vf_con:0x%x\n",
		__func__,
		CAMSV_READ_REG(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE),
		CAMSV_READ_REG(sv_dev->base + REG_CAMSVCENTRAL_VF_CON));
	return ret;
}
int ut_mtk_cam_sv_cq_enable(struct device *dev)
{
	int ret = 0;
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	/* camsv todo: support subsample
	 *int i, subsample = 0;
	 *
	 *for (i = SVTAG_START; i < SVTAG_END; i++) {
	 *	if (camsv_dev->enabled_tags & (1 << i)) {
	 *		subsample = camsv_dev->tag_info[i].cfg_in_param.subsample;
	 *		break;
	 *	}
	 *}
	 *
	 *if (subsample)
	 *	CAMSV_WRITE_BITS(camsv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
	 *		CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_SUBSAMPLE_EN, 1);
	 */
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_EN, 1);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_INT_0_EN,
		CAMSVCQTOP_INT_0_EN, CAMSVCQTOP_CSR_SCQ_SUB_THR_DONE_INT_EN, 1);
	return ret;
}
int ut_mtk_cam_sv_cq_config(
	struct device *dev)
{
	struct mtk_ut_camsv_device *camsv_dev = dev_get_drvdata(dev);
	/* camsv todo: db en */
	CAMSV_WRITE_BITS(camsv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_CQ_DB_EN, 0);
	/* reset stagger mode */
	CAMSV_WRITE_BITS(camsv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_STAGGER_MODE, 1);
	CAMSV_WRITE_BITS(camsv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_MODE, 1);
	/* camsv todo: start period need to be calculated */
	CAMSV_WRITE_REG(camsv_dev->base_scq  + REG_CAMSVCQ_SCQ_START_PERIOD,
		0xFFFFFFFF);
	return 0;
}
int ut_mtk_cam_sv_dmao_common_config(
	struct device *dev)
{
	int ret = 0;
	struct mtk_ut_camsv_device *camsv_dev = dev_get_drvdata(dev);

	switch (camsv_dev->id) {
	case CAMSV_0:
		/* wdma 1 */
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x86660555);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x14440333);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x12220111);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x81110000);
		/* wdma 2 */
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG2, 0x84000355);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG2, 0x12AB0200);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG2, 0x115600AB);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG2, 0x80AB0000);
		break;
	case CAMSV_1:
		/* wdma 1 */
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x86660555);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x14440333);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x12220111);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x81110000);
		/* wdma 2 */
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG2, 0x84000355);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG2, 0x12AB0200);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG2, 0x115600AB);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG2, 0x80AB0000);
		break;
	case CAMSV_2:
		/* wdma1 */
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x84CE0401);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x13340267);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x119A00CD);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x80CD0000);
		break;
	case CAMSV_3:
		/* imgo */
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x83000280);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x12000180);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x11000080);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x80800000);
		break;
	case CAMSV_4:
		/* imgo */
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x80D800B4);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x1090006C);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x10480024);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x80240000);
		break;
	case CAMSV_5:
		/* imgo */
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x80D800B4);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x1090006C);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x10480024);
		CAMSV_WRITE_REG(camsv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x80240000);
		break;
	}
	/* cqi */
	CAMSV_WRITE_REG(camsv_dev->base_scq + REG_CAMSV_M1_CQI_ORIRDMA_CON0, 0x10000040);
	CAMSV_WRITE_REG(camsv_dev->base_scq + REG_CAMSV_M1_CQI_ORIRDMA_CON1, 0x000D0007);
	CAMSV_WRITE_REG(camsv_dev->base_scq + REG_CAMSV_M1_CQI_ORIRDMA_CON2, 0x001A0014);
	CAMSV_WRITE_REG(camsv_dev->base_scq + REG_CAMSV_M1_CQI_ORIRDMA_CON3, 0x00270020);
	CAMSV_WRITE_REG(camsv_dev->base_scq + REG_CAMSV_M1_CQI_ORIRDMA_CON4, 0x00070000);
	CAMSV_WRITE_REG(camsv_dev->base_scq + REG_CAMSV_M2_CQI_ORIRDMA_CON0, 0x10000040);
	CAMSV_WRITE_REG(camsv_dev->base_scq + REG_CAMSV_M2_CQI_ORIRDMA_CON1, 0x000D0007);
	CAMSV_WRITE_REG(camsv_dev->base_scq + REG_CAMSV_M2_CQI_ORIRDMA_CON2, 0x001A0014);
	CAMSV_WRITE_REG(camsv_dev->base_scq + REG_CAMSV_M2_CQI_ORIRDMA_CON3, 0x00270020);
	CAMSV_WRITE_REG(camsv_dev->base_scq + REG_CAMSV_M2_CQI_ORIRDMA_CON4, 0x00070000);
	return ret;
}
static int ut_camsv_apply_cq(struct device *dev,
			    dma_addr_t cq_addr, unsigned int cq_size, unsigned int cq_offset,
			    unsigned int sub_cq_size, unsigned int sub_cq_offset)
{
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	#define CQ_VADDR_MASK 0xffffffff
	u32 cq_addr_lsb = (cq_addr + cq_offset) & CQ_VADDR_MASK;
	u32 cq_addr_msb = ((cq_addr + cq_offset) >> 32);

	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_CQ_SUB_THR0_DESC_SIZE_2,
		cq_size);
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_CQ_SUB_THR0_BASEADDR_2_MSB,
		cq_addr_msb);
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_CQ_SUB_THR0_BASEADDR_2,
		cq_addr_lsb);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_THR_START,
		CAMSVCQTOP_THR_START, CAMSVCQTOP_CSR_CQ_THR0_START, 1);
	dev_info(sv_dev->dev, "apply camsv scq: addr_msb:0x%x addr_lsb:0x%x size:%d",
			cq_addr_msb, cq_addr_lsb, cq_size);
	return 0;
}
static int ut_camsv_initialize(struct device *dev, void *ext_params)
{
	ut_mtk_cam_sv_dmao_common_config(dev);
	ut_mtk_cam_sv_cq_config(dev);
	ut_mtk_cam_sv_cq_enable(dev);
	return 0;
}
static int ut_camsv_s_stream(struct device *dev, enum streaming_enum on)
{
	int ret = 0;
	dev_info(dev, "%s: %d\n", __func__, on);
	if (on)
		ret = ut_mtk_camsv_central_common_enable(dev);
	else {
		ret = ut_mtk_camsv_cq_disable(dev) ||
			ut_mtk_camsv_central_common_disable(dev) ||
			ut_mtk_cam_sv_fbc_disable(dev);
	}
	return ret;
}
static int ut_camsv_dev_config(struct device *dev,
	struct mtkcam_ipi_input_param *cfg_in_param)
{
	int ret = 0;
	return ret;
}
static void ut_camsv_set_ops(struct device *dev)
{
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	sv_dev->ops.initialize = ut_camsv_initialize;
	sv_dev->ops.reset = ut_sv_reset;
	sv_dev->ops.s_stream = ut_camsv_s_stream;
	sv_dev->ops.dev_config = ut_camsv_dev_config;
	sv_dev->ops.apply_cq = ut_camsv_apply_cq;
}
static int mtk_ut_camsv_component_bind(struct device *dev,
				     struct device *master, void *data)
{
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = data;
	struct ut_event evt;

	if (!data) {
		dev_info(dev, "no master data\n");
		return -1;
	}
	if (!ut->camsv) {
		dev_info(dev, "no camsv arr, num of camsv %d\n", ut->num_camsv);
		return -1;
	}
	ut->camsv[sv_dev->id] = dev;
	evt.mask = EVENT_SV_SOF | EVENT_CQ_DONE | EVENT_SW_P1_DONE | EVENT_CQ_MAIN_TRIG_DLY;
	add_listener(&sv_dev->event_src, &ut->listener, evt);
	return 0;
}
static void mtk_ut_camsv_component_unbind(struct device *dev,
					struct device *master, void *data)
{
	struct mtk_ut_camsv_device *sv_dev = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = data;
	ut->camsv[sv_dev->id] = NULL;
	remove_listener(&sv_dev->event_src, &ut->listener);
}
static const struct component_ops mtk_ut_camsv_component_ops = {
	.bind = mtk_ut_camsv_component_bind,
	.unbind = mtk_ut_camsv_component_unbind,
};
static irqreturn_t ut_mtk_irq_camsv_sof(int irq, void *data)
{
	struct mtk_ut_camsv_device *camsv = data;
	struct ut_event event;
	unsigned int irq_sof_status;

	irq_sof_status = readl_relaxed(camsv->base + REG_CAMSVCENTRAL_SOF_STATUS);
	event.mask = 0;
	if (irq_sof_status)
		event.mask |= EVENT_SV_SOF;
	if (event.mask) {
		dev_dbg(camsv->dev, "send event 0x%x\n", event.mask);
		send_event(&camsv->event_src, event);
	}
	dev_info(camsv->dev, "irq_sof_status:0x%x", irq_sof_status);
	return IRQ_HANDLED;
}
static irqreturn_t ut_mtk_irq_camsv_done(int irq, void *data)
{
	struct mtk_ut_camsv_device *camsv = data;
	struct ut_event event;
	unsigned int irq_done_status;

	irq_done_status = readl_relaxed(camsv->base + REG_CAMSVCENTRAL_DONE_STATUS);
	event.mask = 0;
	if (irq_done_status && !camsv->is_dc_mode)
		event.mask |= EVENT_SW_P1_DONE;
	if (event.mask) {
		dev_dbg(camsv->dev, "send event 0x%x\n", event.mask);
		send_event(&camsv->event_src, event);
	}
	dev_info(camsv->dev, "irq_done_status:0x%x", irq_done_status);
	return IRQ_HANDLED;
}
static irqreturn_t ut_mtk_irq_camsv_cq_done(int irq, void *data)
{
	struct mtk_ut_camsv_device *camsv = data;
	struct ut_event event;
	unsigned int cq_done_status;

	cq_done_status = readl_relaxed(camsv->base_scq + REG_CAMSVCQTOP_INT_0_STATUS);
	event.mask = 0;
	if (cq_done_status & CAMSVCQTOP_SCQ_SUB_THR_DONE)
		event.mask |= EVENT_CQ_DONE;
	if (event.mask) {
		dev_dbg(camsv->dev, "send event 0x%x\n", event.mask);
		send_event(&camsv->event_src, event);
	}
	dev_info(camsv->dev, "cq_done_status:0x%x", cq_done_status);
	return IRQ_HANDLED;
}
static irqreturn_t ut_mtk_irq_camsv_err(int irq, void *data)
{
	struct mtk_ut_camsv_device *camsv = data;
	unsigned int err_status;

	err_status = readl_relaxed(camsv->base_inner + REG_CAMSVCENTRAL_ERR_STATUS);
	dev_info(camsv->dev, "err_status:0x%x", err_status);
	return IRQ_HANDLED;
}
static int mtk_ut_camsv_of_probe(struct platform_device *pdev,
			    struct mtk_ut_camsv_device *camsv)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int irq[CAMSV_IRQ_NUM], clks, ret;
	int i;

	ret = of_property_read_u32(dev->of_node, "mediatek,camsv-id",
				   &camsv->id);
	dev_info(dev, "id = %d\n", camsv->id);
	if (ret) {
		dev_info(dev, "missing camsvid property\n");
		return ret;
	}
	ret = of_property_read_u32(dev->of_node, "mediatek,cammux-id",
						       &camsv->cammux_id);
	if (ret) {
		dev_info(dev, "missing cammux id property\n");
		return ret;
	}
	/* base outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}
	camsv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(camsv->base)) {
		dev_info(dev, "failed to map register base\n");
		return PTR_ERR(camsv->base);
	}
	dev_dbg(dev, "camsv, map_addr=0x%pK\n", camsv->base);
	/* base dma outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base_DMA");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}
	camsv->base_dma = devm_ioremap_resource(dev, res);
	if (IS_ERR(camsv->base_dma)) {
		dev_info(dev, "failed to map register base dma\n");
		return PTR_ERR(camsv->base_dma);
	}
	dev_dbg(dev, "camsv, map_addr=0x%pK\n", camsv->base_dma);
	/* base scq outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base_SCQ");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}
	camsv->base_scq = devm_ioremap_resource(dev, res);
	if (IS_ERR(camsv->base_scq)) {
		dev_info(dev, "failed to map register base scq\n");
		return PTR_ERR(camsv->base_scq);
	}
	dev_dbg(dev, "camsv, map_addr=0x%pK\n", camsv->base_scq);
	/* base inner register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}
	camsv->base_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(camsv->base_inner)) {
		dev_info(dev, "failed to map register inner base\n");
		return PTR_ERR(camsv->base_inner);
	}
	dev_dbg(dev, "camsv, map_addr(inner)=0x%pK\n", camsv->base_inner);
	/* base inner dma register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base_DMA");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}
	camsv->base_inner_dma = devm_ioremap_resource(dev, res);
	if (IS_ERR(camsv->base_inner_dma)) {
		dev_info(dev, "failed to map register inner base dma\n");
		return PTR_ERR(camsv->base_inner_dma);
	}
	dev_dbg(dev, "camsv, map_addr(inner dma)=0x%pK\n", camsv->base_inner_dma);
	/* base inner scq register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base_SCQ");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}
	camsv->base_inner_scq = devm_ioremap_resource(dev, res);
	if (IS_ERR(camsv->base_inner_scq)) {
		dev_info(dev, "failed to map register inner base scq\n");
		return PTR_ERR(camsv->base_inner_scq);
	}
	dev_dbg(dev, "camsv, map_addr(inner scq)=0x%pK\n", camsv->base_inner_scq);
	for (i = 0; i < CAMSV_IRQ_NUM; i++) {
		irq[i] = platform_get_irq(pdev, i);
		if (!irq[i]) {
			dev_info(dev, "failed to get irq %d\n", i);
			return -ENODEV;
		}
	}
	for (i = 0; i < CAMSV_IRQ_NUM; i++) {
		if (i == 0)
			ret = devm_request_irq(dev, irq[i],
						ut_mtk_irq_camsv_done,
						0, dev_name(dev), camsv);
		else if (i == 1)
			ret = devm_request_irq(dev, irq[i],
						ut_mtk_irq_camsv_err,
						0, dev_name(dev), camsv);
		else if (i == 2)
			ret = devm_request_irq(dev, irq[i],
						ut_mtk_irq_camsv_sof,
						0, dev_name(dev), camsv);
		else
			ret = devm_request_irq(dev, irq[i],
						ut_mtk_irq_camsv_cq_done,
						0, dev_name(dev), camsv);
		if (ret) {
			dev_info(dev, "failed to request irq=%d\n", irq[i]);
			return ret;
		}
		dev_info(dev, "registered irq=%d\n", irq[i]);
	}
	clks = of_count_phandle_with_args(pdev->dev.of_node,
				"clocks", "#clock-cells");
	camsv->num_clks = (clks == -ENOENT) ? 0:clks;
	dev_info(dev, "clk_num:%d\n", camsv->num_clks);
	if (camsv->num_clks) {
		camsv->clks = devm_kcalloc(dev, camsv->num_clks,
						sizeof(*camsv->clks), GFP_KERNEL);
		if (!camsv->clks)
			return -ENOMEM;
	}
	for (i = 0; i < camsv->num_clks; i++) {
		camsv->clks[i] = of_clk_get(pdev->dev.of_node, i);
		if (IS_ERR(camsv->clks[i])) {
			dev_info(dev, "failed to get clk %d\n", i);
			return -ENODEV;
		}
	}
	return 0;
}
static int mtk_ut_camsv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_camsv_device *camsv;
	int ret;

	camsv = devm_kzalloc(dev, sizeof(*camsv), GFP_KERNEL);
	if (!camsv)
		return -ENOMEM;
	camsv->dev = dev;
	dev_set_drvdata(dev, camsv);
	ret = mtk_ut_camsv_of_probe(pdev, camsv);
	if (ret)
		return ret;
	init_event_source(&camsv->event_src);
	ut_camsv_set_ops(dev);
	pm_runtime_enable(dev);
	ret = component_add(dev, &mtk_ut_camsv_component_ops);
	if (ret)
		return ret;
	dev_info(dev, "%s: success\n", __func__);
	return 0;
}
static int mtk_ut_camsv_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_camsv_device *camsv = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < camsv->num_clks; i++) {
		if (camsv->clks[i])
			clk_put(camsv->clks[i]);
	}
	pm_runtime_disable(dev);
	component_del(dev, &mtk_ut_camsv_component_ops);
	return 0;
}
static int mtk_ut_camsv_pm_suspend(struct device *dev)
{
	dev_dbg(dev, "- %s\n", __func__);
	return 0;
}
static int mtk_ut_camsv_pm_resume(struct device *dev)
{
	dev_dbg(dev, "- %s\n", __func__);
	return 0;
}
static int ut_mtk_camsv_runtime_suspend(struct device *dev)
{
	struct mtk_ut_camsv_device *camsv = dev_get_drvdata(dev);
	int i;
	dev_info(dev, "%s:disable clock\n", __func__);
	for (i = 0; i < camsv->num_clks; i++)
		clk_disable_unprepare(camsv->clks[i]);
	return 0;
}
static int ut_mtk_camsv_runtime_resume(struct device *dev)
{
	struct mtk_ut_camsv_device *camsv = dev_get_drvdata(dev);
	int i, ret;

	for (i = 0; i < camsv->num_clks; i++) {
		ret = clk_prepare_enable(camsv->clks[i]);
		if (ret) {
			dev_info(dev, "enable failed at clk #%d, ret = %d\n",
				 i, ret);
			i--;
			while (i >= 0)
				clk_disable_unprepare(camsv->clks[i--]);
			return ret;
		}
	}
	ut_sv_reset(dev);
	return 0;
}
static const struct dev_pm_ops mtk_ut_camsv_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_ut_camsv_pm_suspend,
							mtk_ut_camsv_pm_resume)
	SET_RUNTIME_PM_OPS(ut_mtk_camsv_runtime_suspend,
						ut_mtk_camsv_runtime_resume,
						NULL)
};
static const struct of_device_id mtk_ut_camsv_of_ids[] = {
	{.compatible = "mediatek,camsv",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_ut_camsv_of_ids);
struct platform_driver mtk_ut_camsv_driver = {
	.probe   = mtk_ut_camsv_probe,
	.remove  = mtk_ut_camsv_remove,
	.driver  = {
		.name  = "mtk-cam camsv-ut",
		.of_match_table = of_match_ptr(mtk_ut_camsv_of_ids),
		.pm     = &mtk_ut_camsv_pm_ops,
	}
};
