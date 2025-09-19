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
#include "../cam/mtk_cam-mraw-regs.h"
#define SCQ_DEADLINE_MS  15 // ~1/2 frame length
#define SCQ_DEFAULT_CLK_RATE 208 // default 208MHz
#define MRAW_WRITE_BITS(RegAddr, RegName, FieldName, FieldValue) do {\
	union RegName reg;\
	\
	reg.Raw = readl_relaxed(RegAddr);\
	reg.Bits.FieldName = FieldValue;\
	writel_relaxed(reg.Raw, RegAddr);\
} while (0)
#define MRAW_WRITE_REG(RegAddr, RegValue) ({\
	writel_relaxed(RegValue, RegAddr);\
})
#define MRAW_READ_BITS(RegAddr, RegName, FieldName) ({\
	union RegName reg;\
	\
	reg.Raw = readl_relaxed(RegAddr);\
	reg.Bits.FieldName;\
})
#define MRAW_READ_REG(RegAddr) ({\
	unsigned int var;\
	\
	var = readl_relaxed(RegAddr);\
	var;\
})
enum mraw_db_load_src {
	MRAW_DB_SRC_SUB_P1_DONE = 0,
	MRAW_DB_SRC_SOF         = 1,
};
enum mraw_int_en1 {
	MRAW_INT_EN1_VS_INT_EN                   = (1L<<0),
	MRAW_INT_EN1_TG_INT1_EN                  = (1L<<1),
	MRAW_INT_EN1_TG_INT2_EN                  = (1L<<2),
	MRAW_INT_EN1_TG_INT3_EN                  = (1L<<3),
	MRAW_INT_EN1_TG_INT4_EN                  = (1L<<4),
	MRAW_INT_EN1_EXPDON_EN                   = (1L<<5),
	MRAW_INT_EN1_TG_ERR_EN                   = (1L<<6),
	MRAW_INT_EN1_TG_GBERR_EN                 = (1L<<7),
	MRAW_INT_EN1_TG_SOF_INT_EN               = (1L<<8),
	MRAW_INT_EN1_TG_SOF_WAIT_EN              = (1L<<9),
	MRAW_INT_EN1_TG_SOF_DROP_EN              = (1L<<10),
	MRAW_INT_EN1_VS_INT_ORG_EN               = (1L<<11),
	MRAW_INT_EN1_CQ_DB_LOAD_ERR_EN           = (1L<<12),
	MRAW_INT_EN1_CQ_MAX_START_DLY_ERR_INT_EN = (1L<<13),
	MRAW_INT_EN1_CQ_CODE_ERR_EN              = (1L<<14),
	MRAW_INT_EN1_CQ_VS_ERR_EN                = (1L<<15),
	MRAW_INT_EN1_CQ_TRIG_DLY_INT_EN          = (1L<<16),
	MRAW_INT_EN1_CQ_SUB_CODE_ERR_EN          = (1L<<17),
	MRAW_INT_EN1_CQ_SUB_VS_ERR_EN            = (1L<<18),
	MRAW_INT_EN1_PASS1_DONE_EN               = (1L<<19),
	MRAW_INT_EN1_SW_PASS1_DONE_EN            = (1L<<20),
	MRAW_INT_EN1_SUB_PASS1_DONE_EN           = (1L<<21),
	MRAW_INT_EN1_DMA_ERR_EN                  = (1L<<22),
	MRAW_INT_EN1_SW_ENQUE_ERR_EN             = (1L<<23),
	MRAW_INT_EN1_INT_WCLR_EN                 = (1L<<24),
};
enum mraw_int_en5 {
	MRAW_INT_EN5_IMGO_M1_ERR_EN              = (1L<<0),
	MRAW_INT_EN5_IMGBO_M1_ERR_EN             = (1L<<1),
	MRAW_INT_EN5_CPIO_M1_ERR_EN              = (1L<<2),
};
enum mraw_tg_fmt {
	SV_TG_FMT_RAW8      = 0,
	SV_TG_FMT_RAW10     = 1,
	SV_TG_FMT_RAW12     = 2,
	SV_TG_FMT_YUV422    = 3,
	SV_TG_FMT_RAW14     = 4,
	SV_TG_FMT_RSV1      = 5,
	SV_TG_FMT_RSV2      = 6,
	SV_TG_FMT_JPG       = 7,
};
enum mraw_tg_swap {
	TG_SW_UYVY = 0,
	TG_SW_YUYV = 1,
	TG_SW_VYUY = 2,
	TG_SW_YVYU = 3,
};
enum mraw_wfbc {
	MRAW_WFBC_IMGO                           = (1L<<0),
	MRAW_WFBC_IMGBO                          = (1L<<1),
	MRAW_WFBC_CPIO                           = (1L<<2),
};
int ut_mtk_cam_mraw_toggle_tg_db(struct device *dev)
{
	int ret = 0;
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_TG_PATH_CFG,
		MRAW_TG_PATH_CFG, TG_M1_DB_LOAD_DIS, 1);
	MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_TG_PATH_CFG,
		MRAW_TG_PATH_CFG, TG_M1_DB_LOAD_DIS, 0);
	return ret;
}
int ut_mtk_cam_mraw_toggle_db(struct device *dev)
{
	int ret = 0;
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_CTL_MISC,
		MRAW_CTL_MISC, MRAWCTL_DB_EN, 0);
	MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_CTL_MISC,
		MRAW_CTL_MISC, MRAWCTL_DB_EN, 1);
	return ret;
}
unsigned int ut_mtk_cam_mraw_cq_enable(
	struct device *dev)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);
	int ret = 0;
	u32 val;

	val = readl_relaxed(mraw_dev->base + REG_MRAW_TG_TIME_STAMP_CNT);
	//[todo]: implement/check the start period
	writel_relaxed(SCQ_DEADLINE_MS * 1000 * SCQ_DEFAULT_CLK_RATE
		, mraw_dev->base + REG_MRAW_SCQ_START_PERIOD);
	return ret;
}
unsigned int ut_mtk_cam_mraw_cq_config(
	struct device *dev, unsigned int subsample)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);
	int ret = 0;
	u32 val;

	val = readl_relaxed(mraw_dev->base + REG_MRAW_CQ_EN);
	val = val | MRAWCQ_DB_EN;

	if (subsample) {
		val = val | MRAWSCQ_SUBSAMPLE_EN;
		val = val | MRAWCQ_SOF_SEL;
		dev_info(mraw_dev->dev, "%s - mraw subsample(%d)\n", __func__, subsample);
	} else {
		val = val & ~MRAWSCQ_SUBSAMPLE_EN;
		val = val & ~MRAWCQ_SOF_SEL;
	}

	writel_relaxed(val, mraw_dev->base + REG_MRAW_CQ_EN);

	val = readl_relaxed(mraw_dev->base + REG_MRAW_CQ_SUB_EN);
	val = val | MRAWCQ_SUB_DB_EN;
	writel_relaxed(val, mraw_dev->base + REG_MRAW_CQ_SUB_EN);

	writel_relaxed(0xFFFFFFFF, mraw_dev->base + REG_MRAW_SCQ_START_PERIOD);
	wmb(); /* TBC */
	writel_relaxed(MRAWCQ_SUB_THR0_MODE_IMMEDIATE | MRAWCQ_SUB_THR0_EN,
		       mraw_dev->base + REG_MRAW_CQ_SUB_THR0_CTL);

	writel_relaxed(MRAWCTL_CQ_SUB_THR0_DONE_EN,
		       mraw_dev->base + REG_MRAW_CTL_INT6_EN);
	wmb(); /* TBC */

	dev_info(mraw_dev->dev, "%s - REG_CQ_EN:0x%x_%x ,REG_CQ_THR0_CTL:0x%8x\n",
		__func__,
		readl_relaxed(mraw_dev->base + REG_MRAW_CQ_EN),
		readl_relaxed(mraw_dev->base + REG_MRAW_CQ_SUB_EN),
		readl_relaxed(mraw_dev->base + REG_MRAW_CQ_SUB_THR0_CTL));

	return ret;
}
void mraw_reset_by_mraw_top(struct device *dev)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	dev_info(dev, "%s: top rst start\n", __func__);
	writel(0, mraw_dev->base + REG_CAMSYS_MRAW_SW_RST);
	writel(3 << ((mraw_dev->id) * 2 + 4), mraw_dev->mraw_base + REG_CAMSYS_MRAW_SW_RST);
	writel(0, mraw_dev->base + REG_CAMSYS_MRAW_SW_RST);
	wmb(); /* make sure committed */
}
void mraw_reset(struct device *dev)
{
	unsigned long end = jiffies + msecs_to_jiffies(100);
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	dev_info(dev, "%s: sw rst start\n", __func__);
	writel_relaxed(0, mraw_dev->base + REG_MRAW_CTL_SW_CTL);
	writel_relaxed(1, mraw_dev->base + REG_MRAW_CTL_SW_CTL);
	wmb(); /* TBC */
	while (time_before(jiffies, end)) {
		if (readl(mraw_dev->base + REG_MRAW_CTL_SW_CTL) & 0x2) {
			// do hw rst
			writel_relaxed(4, mraw_dev->base + REG_MRAW_CTL_SW_CTL);
			writel_relaxed(0, mraw_dev->base + REG_MRAW_CTL_SW_CTL);
			wmb(); /* TBC */
			return;
		}
		dev_info(dev,
			"tg_sen_mode: 0x%x, ctl_en: 0x%x, ctl_sw_ctl:0x%x, frame_no:0x%x\n",
			readl(mraw_dev->base + REG_MRAW_TG_SEN_MODE),
			readl(mraw_dev->base + REG_MRAW_CTL_MOD_EN),
			readl(mraw_dev->base + REG_MRAW_CTL_SW_CTL),
			readl(mraw_dev->base + REG_MRAW_FRAME_SEQ_NUM)
			);
		usleep_range(10, 20);
	}
	dev_info(dev, "reset hw timeout\n");
}

int ut_mtk_cam_mraw_cq_disable(struct device *dev)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);
	int ret = 0;

	writel_relaxed(0, mraw_dev->base + REG_MRAW_CQ_SUB_THR0_CTL);
	wmb(); /* TBC */
	return ret;
}

int ut_mtk_cam_mraw_trigger_wfbc_inc(struct device *dev)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_WFBC_INC,
		(MRAW_WFBC_IMGO | MRAW_WFBC_IMGBO | MRAW_WFBC_CPIO));

	return 0;
}

int ut_mtk_cam_mraw_top_enable(struct device *dev)
{
	int ret = 0;
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	/* toggle db */
	ut_mtk_cam_mraw_toggle_db(dev);

	ut_mtk_cam_mraw_trigger_wfbc_inc(dev);

	/* toggle tg db */
	ut_mtk_cam_mraw_toggle_tg_db(dev);

	/* enable cmos */
	dev_info(mraw_dev->dev, "%s: enable CMOS and VF\n", __func__);
	MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_TG_SEN_MODE,
		MRAW_TG_SEN_MODE, TG_CMOS_EN, 1);

	/* enable vf */
	if (MRAW_READ_BITS(mraw_dev->base + REG_MRAW_TG_SEN_MODE, MRAW_TG_SEN_MODE, TG_CMOS_EN))
		MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_TG_VF_CON,
			MRAW_TG_VF_CON, TG_M1_VFDATA_EN, 1);

	return ret;
}

int ut_mtk_cam_mraw_top_config(struct device *dev)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	unsigned int int_en1 = (MRAW_INT_EN1_TG_ERR_EN |
							MRAW_INT_EN1_TG_GBERR_EN |
							MRAW_INT_EN1_TG_SOF_INT_EN |
							MRAW_INT_EN1_CQ_SUB_CODE_ERR_EN |
							MRAW_INT_EN1_CQ_DB_LOAD_ERR_EN |
							MRAW_INT_EN1_CQ_SUB_VS_ERR_EN |
							MRAW_INT_EN1_CQ_TRIG_DLY_INT_EN |
							MRAW_INT_EN1_SW_PASS1_DONE_EN |
							MRAW_INT_EN1_DMA_ERR_EN |
							MRAW_INT_EN1_SW_ENQUE_ERR_EN
							);

	unsigned int int_en5 = (MRAW_INT_EN5_IMGO_M1_ERR_EN |
							MRAW_INT_EN5_IMGBO_M1_ERR_EN |
							MRAW_INT_EN5_CPIO_M1_ERR_EN
							);

	/* int en */
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_INT_EN, int_en1);
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_INT5_EN, int_en5);

	/* db load src */
	MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_CTL_MISC,
		MRAW_CTL_MISC, MRAWCTL_DB_LOAD_SRC, MRAW_DB_SRC_SOF);

	return 0;
}

int ut_mtk_cam_mraw_dmao_common_config(
	struct device *dev)
{
	int ret = 0;
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);
	/* imgo con */
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_IMGO_ORIWDMA_CON0,
		0x10000188);  // BURST_LEN and FIFO_SIZE
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_IMGO_ORIWDMA_CON1,
		0x004F0028);  // Threshold for pre-ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_IMGO_ORIWDMA_CON2,
		0x009D0076);  // Threshold for ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_IMGO_ORIWDMA_CON3,
		0x00EC00C4);  // Threshold for urgent
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_IMGO_ORIWDMA_CON4,
		0x00280000);  // Threshold for DVFS
	/* imgbo con */
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_IMGBO_ORIWDMA_CON0,
		0x10000140);  // BURST_LEN and FIFO_SIZE
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_IMGBO_ORIWDMA_CON1,
		0x00400020);  // Threshold for pre-ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_IMGBO_ORIWDMA_CON2,
		0x00800060);  // Threshold for ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_IMGBO_ORIWDMA_CON3,
		0x00C000A0);  // Threshold for urgent
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_IMGBO_ORIWDMA_CON4,
		0x00200000);  // Threshold for DVFS
	/* cpio con */
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CPIO_ORIWDMA_CON0,
		0x10000040);  // BURST_LEN and FIFO_SIZE
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CPIO_ORIWDMA_CON1,
		0x000D0007);  // Threshold for pre-ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CPIO_ORIWDMA_CON2,
		0x001A0014);  // Threshold for ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CPIO_ORIWDMA_CON3,
		0x00270020);  // Threshold for urgent
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CPIO_ORIWDMA_CON4,
		0x00070000);  // Threshold for DVFS

	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_FHO_ORIWDMA_CON0,
		0x10000040);  // BURST_LEN and FIFO_SIZE
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_FHO_ORIWDMA_CON1,
		0x000D0007);  // Threshold for pre-ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_FHO_ORIWDMA_CON2,
		0x001A0014);  // Threshold for ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_FHO_ORIWDMA_CON3,
		0x00270020);  // Threshold for urgent
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_FHO_ORIWDMA_CON4,
		0x00070000);  // Threshold for DVFS

	/* cqi con */
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_M1_CQI_ORIRDMA_CON0,
		0x10000040);  // BURST_LEN and FIFO_SIZE
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_M1_CQI_ORIRDMA_CON1,
		0x000D0007);  // Threshold for pre-ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_M1_CQI_ORIRDMA_CON2,
		0x001A0014);  // Threshold for ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_M1_CQI_ORIRDMA_CON3,
		0x00270020);  // Threshold for urgent
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_M1_CQI_ORIRDMA_CON4,
		0x00070000);  // Threshold for DVFS
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_M2_CQI_ORIRDMA_CON0,
		0x10000040);  // BURST_LEN and FIFO_SIZE
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_M2_CQI_ORIRDMA_CON1,
		0x000D0007);  // Threshold for pre-ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_M2_CQI_ORIRDMA_CON2,
		0x001A0014);  // Threshold for ultra
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_M2_CQI_ORIRDMA_CON3,
		0x00270020);  // Threshold for urgent
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_M2_CQI_ORIRDMA_CON4,
		0x00070000);  // Threshold for DVFS
	return ret;
}
int ut_mtk_cam_mraw_top_disable(struct device *dev)
{
	int ret = 0;
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	if (MRAW_READ_BITS(mraw_dev->base + REG_MRAW_TG_VF_CON,
			MRAW_TG_VF_CON, TG_M1_VFDATA_EN)) {
		MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_TG_VF_CON,
			MRAW_TG_VF_CON, TG_M1_VFDATA_EN, 0);
		ut_mtk_cam_mraw_toggle_tg_db(dev);
	}

	mraw_reset(dev);

	MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_CTL_MISC,
		MRAW_CTL_MISC, MRAWCTL_DB_EN, 0);
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_MISC, 0);
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_FMT_SEL, 0);
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_INT_EN, 0);
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_INT5_EN, 0);

	MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_CTL_MISC,
		MRAW_CTL_MISC, MRAWCTL_DB_EN, 1);
	return ret;
}

int ut_mtk_cam_mraw_tg_disable(struct device *dev)
{
	int ret = 0;
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);
	u32 val;

	dev_dbg(mraw_dev->dev, "stream off, disable CMOS\n");
	val = readl(mraw_dev->base + REG_MRAW_TG_SEN_MODE);
	writel(val & (~MRAWTG_CMOS_EN),
		mraw_dev->base + REG_MRAW_TG_SEN_MODE);

	return ret;
}

int ut_mtk_cam_mraw_dma_disable(struct device *dev)
{
	int ret = 0;
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_MOD3_EN, 0);

	return ret;
}

int ut_mtk_cam_mraw_fbc_disable(struct device *dev)
{
	int ret = 0;
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_WFBC_EN, 0);
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_FBC_GROUP, 0);

	return ret;
}

int ut_mtk_cam_mraw_bw_qos_config(struct device *dev)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	/* sw bw_qos en */
	MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_CTL_BW_QOS_CTL,
		MRAW_CTL_BW_QOS_CTL, MRAWCTL_BW_QOS_SW_SET, 1);

	return 0;
}

int ut_mtk_cam_mraw_ddren_config(struct device *dev)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	/* sw ddr en */
	MRAW_WRITE_BITS(mraw_dev->base + REG_MRAW_CTL_DDREN_CTL,
		MRAW_CTL_DDREN_CTL, MRAWCTL_DDREN_SW_SET, 1);

	return 0;
}

int ut_mtk_cam_mraw_fbc_config(struct device *dev)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_WFBC_EN, 0);
	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_FBC_GROUP,
		(MRAW_WFBC_IMGO | MRAW_WFBC_IMGBO | MRAW_WFBC_CPIO));

	return 0;
}

int ut_mtk_cam_mraw_fbc_enable(struct device *dev)
{
	int ret = 0;

	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	if (MRAW_READ_BITS(mraw_dev->base + REG_MRAW_TG_VF_CON,
			MRAW_TG_VF_CON, TG_M1_VFDATA_EN) == 1) {
		ret = -1;
		dev_dbg(mraw_dev->dev, "cannot enable fbc when streaming");
		goto EXIT;
	}

	MRAW_WRITE_REG(mraw_dev->base + REG_MRAW_CTL_WFBC_EN,
		(MRAW_WFBC_IMGO | MRAW_WFBC_IMGBO | MRAW_WFBC_CPIO));

EXIT:
	return ret;
}

static int ut_mraw_initialize(struct device *dev, void *ext_params)
{
	struct mtk_ut_mraw_initial_params *p = ext_params;

	ut_mtk_cam_mraw_top_config(dev);
	ut_mtk_cam_mraw_dmao_common_config(dev);
	ut_mtk_cam_mraw_ddren_config(dev);
	ut_mtk_cam_mraw_bw_qos_config(dev);
	ut_mtk_cam_mraw_fbc_config(dev);
	ut_mtk_cam_mraw_fbc_enable(dev);
	ut_mtk_cam_mraw_cq_config(dev, p->subsample);
	return 0;
}
static int ut_mraw_dev_config(struct device *dev, struct mtkcam_ipi_input_param *cfg_in_param)
{
	int ret = 0;
	return ret;
}
static int ut_mraw_reset(struct device *dev)
{
	mraw_reset(dev);
	return 0;
}
static int ut_mraw_s_stream(struct device *dev, enum streaming_enum on)
{
	int ret = 0;

	dev_info(dev, "%s: %d\n", __func__, on);
	if (on) {
		ret = ut_mtk_cam_mraw_cq_enable(dev) ||
			ut_mtk_cam_mraw_top_enable(dev);
	} else {
		ret = ut_mtk_cam_mraw_top_disable(dev) ||
			ut_mtk_cam_mraw_cq_disable(dev) ||
			ut_mtk_cam_mraw_fbc_disable(dev) ||
			ut_mtk_cam_mraw_dma_disable(dev) ||
			ut_mtk_cam_mraw_tg_disable(dev);
	}
	return ret;
}

static int ut_mraw_apply_cq(struct device *dev,
			    dma_addr_t cq_addr, unsigned int cq_size, unsigned int cq_offset,
			    unsigned int sub_cq_size, unsigned int sub_cq_offset)
{

	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);
#define CQ_VADDR_MASK 0xffffffff
	u32 cq_addr_lsb = (cq_addr + cq_offset) & CQ_VADDR_MASK;
	u32 cq_addr_msb = ((cq_addr + cq_offset) >> 32);

	dev_info(mraw_dev->dev,
		"apply mraw%d cq - addr:0x%llx ,size:%d,offset:%d",
		mraw_dev->id, cq_addr, cq_size, cq_offset);

	writel_relaxed(cq_addr_lsb, mraw_dev->base + REG_MRAW_CQ_SUB_THR0_BASEADDR_2);
	writel_relaxed(cq_addr_msb, mraw_dev->base + REG_MRAW_CQ_SUB_THR0_BASEADDR_2_MSB);
	writel_relaxed(cq_size, mraw_dev->base + REG_MRAW_CQ_SUB_THR0_DESC_SIZE_2);
	writel_relaxed(1, mraw_dev->base + REG_MRAW_CTL_START);
	wmb(); /* TBC */
	return 0;
}
static void ut_mraw_set_ops(struct device *dev)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);

	mraw_dev->ops.initialize = ut_mraw_initialize;
	mraw_dev->ops.reset = ut_mraw_reset;
	mraw_dev->ops.s_stream = ut_mraw_s_stream;
	mraw_dev->ops.dev_config = ut_mraw_dev_config;
	mraw_dev->ops.apply_cq = ut_mraw_apply_cq;
}
static int mtk_ut_mraw_component_bind(struct device *dev,
				     struct device *master, void *data)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = data;
	struct ut_event evt;

	if (!data) {
		dev_info(dev, "no master data\n");
		return -1;
	}
	if (!ut->mraw) {
		dev_info(dev, "no mraw arr, num of mraw %d\n", ut->num_mraw);
		return -1;
	}
	ut->mraw[mraw_dev->id] = dev;
	evt.mask = EVENT_MRAW_SOF | EVENT_CQ_DONE | EVENT_SW_P1_DONE | EVENT_CQ_MAIN_TRIG_DLY;
	add_listener(&mraw_dev->event_src, &ut->listener, evt);
	return 0;
}
static void mtk_ut_mraw_component_unbind(struct device *dev,
					struct device *master, void *data)
{
	struct mtk_ut_mraw_device *mraw_dev = dev_get_drvdata(dev);
	struct mtk_cam_ut *ut = data;

	ut->mraw[mraw_dev->id] = NULL;
	remove_listener(&mraw_dev->event_src, &ut->listener);
}
static const struct component_ops mtk_ut_mraw_component_ops = {
	.bind = mtk_ut_mraw_component_bind,
	.unbind = mtk_ut_mraw_component_unbind,
};

void ut_mtk_cam_mraw_debug_dump(struct mtk_ut_mraw_device *mraw_dev)
{
	dev_info_ratelimited(mraw_dev->dev,
		"seq_no:%d_%d tg_sen_mode:0x%x tg_vf_con:0x%x tg_path_cfg:0x%x frm_size:0x%x frm_size_r:0x%x grab_pix:0x%x grab_lin:0x%x\n",
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_FRAME_SEQ_NUM),
		readl_relaxed(mraw_dev->base + REG_MRAW_FRAME_SEQ_NUM),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_TG_SEN_MODE),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_TG_VF_CON),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_TG_PATH_CFG),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_TG_FRMSIZE_ST),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_TG_FRMSIZE_ST_R),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_TG_SEN_GRAB_PXL),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_TG_SEN_GRAB_LIN));

	dev_info_ratelimited(mraw_dev->dev, "mod_en:0x%x mod2_en:0x%x mod3_en:0x%x mod4_en:0x%x mod_ctl:0x%x sel:0x%x fmt_sel:0x%x done_sel:0x%x\n",
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CTL_MOD_EN),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CTL_MOD2_EN),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CTL_MOD3_EN),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CTL_MOD4_EN),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CTL_MODE_CTL),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CTL_SEL),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CTL_FMT_SEL),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CTL_DONE_SEL));

	dev_info_ratelimited(mraw_dev->dev, "imgo_xsize:0x%x imgo_ysize:0x%x imgo_stride:0x%x imgo_addr:0x%x_%x imgo_ofst_addr:0x%x_%x\n",
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGO_XSIZE),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGO_YSIZE),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGO_STRIDE),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGO_BASE_ADDR_MSB),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGO_BASE_ADDR),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGO_OFST_ADDR_MSB),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGO_OFST_ADDR));

	dev_info_ratelimited(mraw_dev->dev, "imgbo_xsize:0x%x imgbo_ysize:0x%x imgbo_stride:0x%x imgbo_addr:0x%x_%x imgbo_ofst_addr:0x%x_%x\n",
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGBO_XSIZE),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGBO_YSIZE),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGBO_STRIDE),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGBO_BASE_ADDR_MSB),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGBO_BASE_ADDR),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGBO_OFST_ADDR_MSB),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGBO_OFST_ADDR));

	dev_info_ratelimited(mraw_dev->dev, "cpio_xsize:0x%x cpio_ysize:0x%x cpio_stride:0x%x cpio_addr:0x%x_%x cpio_ofst_addr:0x%x_%x\n",
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CPIO_XSIZE),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CPIO_YSIZE),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CPIO_STRIDE),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CPIO_BASE_ADDR_MSB),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CPIO_BASE_ADDR),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CPIO_OFST_ADDR_MSB),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CPIO_OFST_ADDR));

	dev_info_ratelimited(mraw_dev->dev,
		"imgo_error_status:0x%x imgbo_error_status:0x%x cpio_error_status:0x%x\n",
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGO_ERR_STAT),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_IMGBO_ERR_STAT),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CPIO_ERR_STAT));

	dev_info_ratelimited(mraw_dev->dev, "sep_ctl:0x%x sep_crop:0x%x sep_vsize:0x%x\n",
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_SEP_CTL),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_SEP_CROP),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_SEP_VSIZE));

	dev_info_ratelimited(mraw_dev->dev, "mbn_cfg_0:0x%x mbn_cfg_1:0x%x mbn_cfg_2:0x%x\n",
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_MBN_CFG_0),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_MBN_CFG_1),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_MBN_CFG_2));

	dev_info_ratelimited(mraw_dev->dev, "cpi_cfg_0:0x%x cpi_cfg_1:0x%x\n",
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CPI_CFG_0),
		readl_relaxed(mraw_dev->base_inner + REG_MRAW_CPI_CFG_1));
}

static irqreturn_t ut_mtk_mraw_irq(int irq, void *data)
{
	struct mtk_ut_mraw_device *mraw = data;
	struct ut_event event;
	unsigned int irq_status, irq_status2, irq_status3, irq_status5, irq_status6;

	/*
	 * [ISP 7.0/7.1/7s/7sp] HW Bug Workaround: read MRAWCTL_INT2_STATUS every irq
	 * Because MRAWCTL_INT2_EN is attach to OTF_OVER_FLOW ENABLE incorrectly
	 */
	irq_status = readl_relaxed(mraw->base + REG_MRAW_CTL_INT_STATUS);
	irq_status2	= readl_relaxed(mraw->base + REG_MRAW_CTL_INT2_STATUS);
	irq_status3	= readl_relaxed(mraw->base + REG_MRAW_CTL_INT3_STATUS);
	irq_status5 = readl_relaxed(mraw->base + REG_MRAW_CTL_INT5_STATUS);
	irq_status6	= readl_relaxed(mraw->base + REG_MRAW_CTL_INT6_STATUS);
	event.mask = 0;

	/* sw p1 done */
	if (irq_status & MRAWCTL_SW_PASS1_DONE_ST)
		event.mask |= EVENT_SW_P1_DONE;

	/* frame start */
	if (irq_status & MRAWCTL_SOF_INT_ST)
		event.mask |= EVENT_MRAW_SOF;

	/* CQ done */
	if (irq_status6 & MRAWCTL_CQ_SUB_THR0_DONE_ST)
		event.mask |= EVENT_CQ_DONE;

	/* check mraw error*/
	if ((irq_status & INT_ST_MASK_MRAW_ERR) ||
		(irq_status & MRAWCTL_DMA_ERR_ST)) {
		dev_info(mraw->dev, "irq_status:0x%x", irq_status);
		ut_mtk_cam_mraw_debug_dump(mraw);
		return IRQ_HANDLED;
	}

	if (event.mask) {
		dev_dbg(mraw->dev, "send event 0x%x\n", event.mask);
		send_event(&mraw->event_src, event);
	}
	dev_info(mraw->dev, "irq_status:0x%x irq_status5:0x%x irq_status6:0x%x", irq_status,
		irq_status5, irq_status6);
	return IRQ_HANDLED;
}
static int mtk_ut_mraw_of_probe(struct platform_device *pdev,
			    struct mtk_ut_mraw_device *mraw)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int irq, clks, ret;
	int i;

	ret = of_property_read_u32(dev->of_node, "mediatek,mraw-id",
				   &mraw->id);
	dev_info(dev, "id = %d\n", mraw->id);
	if (ret) {
		dev_info(dev, "missing mrawid property\n");
		return ret;
	}
	ret = of_property_read_u32(dev->of_node, "mediatek,cammux-id",
						       &mraw->cammux_id);
	if (ret) {
		dev_dbg(dev, "missing cammux id property\n");
		return ret;
	}
	/* base outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}
	mraw->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(mraw->base)) {
		dev_info(dev, "failed to map register base\n");
		return PTR_ERR(mraw->base);
	}
	dev_dbg(dev, "mraw, map_addr=0x%pK\n", mraw->base);
	/* base inner register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}
	mraw->base_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(mraw->base_inner)) {
		dev_info(dev, "failed to map register inner base\n");
		return PTR_ERR(mraw->base_inner);
	}
	mraw->mraw_base = ioremap(0x1a170000, 0x1000);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_info(dev, "failed to get irq\n");
		return -ENODEV;
	}
	ret = devm_request_irq(dev, irq, ut_mtk_mraw_irq, 0,
			       dev_name(dev), mraw);
	if (ret) {
		dev_info(dev, "failed to request irq=%d\n", irq);
		return ret;
	}
	dev_dbg(dev, "registered irq=%d\n", irq);
	clks = of_count_phandle_with_args(pdev->dev.of_node,
				"clocks", "#clock-cells");
	mraw->num_clks = (clks == -ENOENT) ? 0:clks;
	dev_info(dev, "clk_num:%d\n", mraw->num_clks);
	if (mraw->num_clks) {
		mraw->clks = devm_kcalloc(dev, mraw->num_clks,
						sizeof(*mraw->clks), GFP_KERNEL);
		if (!mraw->clks)
			return -ENOMEM;
	}
	for (i = 0; i < mraw->num_clks; i++) {
		mraw->clks[i] = of_clk_get(pdev->dev.of_node, i);
		if (IS_ERR(mraw->clks[i])) {
			dev_info(dev, "failed to get clk %d\n", i);
			return -ENODEV;
		}
	}
	return 0;
}
static int mtk_ut_mraw_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_mraw_device *mraw;
	int ret;

	mraw = devm_kzalloc(dev, sizeof(*mraw), GFP_KERNEL);
	if (!mraw)
		return -ENOMEM;
	mraw->dev = dev;
	dev_set_drvdata(dev, mraw);
	ret = mtk_ut_mraw_of_probe(pdev, mraw);
	if (ret)
		return ret;
	init_event_source(&mraw->event_src);
	ut_mraw_set_ops(dev);
	pm_runtime_enable(dev);
	ret = component_add(dev, &mtk_ut_mraw_component_ops);
	if (ret)
		return ret;
	dev_info(dev, "%s: success\n", __func__);
	return 0;
}
static int mtk_ut_mraw_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_mraw_device *mraw = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < mraw->num_clks; i++) {
		if (mraw->clks[i])
			clk_put(mraw->clks[i]);
	}
	pm_runtime_disable(dev);
	component_del(dev, &mtk_ut_mraw_component_ops);
	return 0;
}
static int mtk_ut_mraw_pm_suspend(struct device *dev)
{
	dev_dbg(dev, "- %s\n", __func__);
	return 0;
}
static int mtk_ut_mraw_pm_resume(struct device *dev)
{
	dev_dbg(dev, "- %s\n", __func__);
	return 0;
}
static int mtk_ut_mraw_runtime_suspend(struct device *dev)
{
	struct mtk_ut_mraw_device *mraw = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < mraw->num_clks; i++)
		clk_disable_unprepare(mraw->clks[i]);
	return 0;
}
static int mtk_ut_mraw_runtime_resume(struct device *dev)
{
	struct mtk_ut_mraw_device *mraw = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < mraw->num_clks; i++)
		clk_prepare_enable(mraw->clks[i]);

	mraw_reset_by_mraw_top(dev);
	return 0;
}
static const struct dev_pm_ops mtk_ut_mraw_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_ut_mraw_pm_suspend,
							mtk_ut_mraw_pm_resume)
	SET_RUNTIME_PM_OPS(mtk_ut_mraw_runtime_suspend,
						mtk_ut_mraw_runtime_resume,
						NULL)
};
static const struct of_device_id mtk_ut_mraw_of_ids[] = {
	{.compatible = "mediatek,mraw",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_ut_mraw_of_ids);
struct platform_driver mtk_ut_mraw_driver = {
	.probe   = mtk_ut_mraw_probe,
	.remove  = mtk_ut_mraw_remove,
	.driver  = {
		.name  = "mtk-cam mraw-ut",
		.of_match_table = of_match_ptr(mtk_ut_mraw_of_ids),
		.pm     = &mtk_ut_mraw_pm_ops,
	}
};
