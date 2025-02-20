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
#include <linux/bitops.h>

#include <soc/mediatek/smi.h>

#include "mtk_cam.h"
#include "mtk_cam-sv-regs.h"
#include "mtk_cam-sv.h"
#include "mtk_cam-fmt_utils.h"

#ifdef CAMSYS_TF_DUMP_7S
#include <dt-bindings/memory/mt6985-larb-port.h>
#include "iommu_debug.h"
#endif

#define MTK_CAMSV_STOP_HW_TIMEOUT			(33 * USEC_PER_MSEC)
#define CAMSV_DEBUG 0
static unsigned int debug_sv_fbc;
module_param(debug_sv_fbc, uint, 0644);
MODULE_PARM_DESC(debug_sv_fbc, "debug: sv fbc");

static int debug_cam_sv;
module_param(debug_cam_sv, int, 0644);

#undef dev_dbg
#define dev_dbg(dev, fmt, arg...)		\
	do {					\
		if (debug_cam_sv >= 1)	\
			dev_info(dev, fmt,	\
				## arg);	\
	} while (0)

static const struct of_device_id mtk_camsv_of_ids[] = {
	{.compatible = "mediatek,camsv",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_camsv_of_ids);

static const struct mtk_camsv_tag_param sv_tag_param_normal[2] = {
	{
		.tag_idx = SVTAG_2,
		.seninf_padidx = PAD_SRC_RAW0,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = false,
	},
	{
		.tag_idx = SVTAG_3,
		.seninf_padidx = PAD_SRC_RAW_W0,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = true,
	},
};

static const struct mtk_camsv_tag_param sv_tag_param_2exp_stagger[4] = {
	{
		.tag_idx = SVTAG_0,
		.seninf_padidx = PAD_SRC_RAW0,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = false,
	},
	{
		.tag_idx = SVTAG_2,
		.seninf_padidx = PAD_SRC_RAW1,
		.tag_order = MTKCAM_IPI_ORDER_LAST_TAG,
		.is_w = false,
	},
	{
		.tag_idx = SVTAG_1,
		.seninf_padidx = PAD_SRC_RAW_W0,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = true,
	},
	{
		.tag_idx = SVTAG_3,
		.seninf_padidx = PAD_SRC_RAW_W1,
		.tag_order = MTKCAM_IPI_ORDER_LAST_TAG,
		.is_w = true,
	},
};

static const struct mtk_camsv_tag_param sv_tag_param_3exp_stagger[3] = {
	{
		.tag_idx = SVTAG_0,
		.seninf_padidx = PAD_SRC_RAW0,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = false,
	},
	{
		.tag_idx = SVTAG_1,
		.seninf_padidx = PAD_SRC_RAW1,
		.tag_order = MTKCAM_IPI_ORDER_NORMAL_TAG,
		.is_w = false,
	},
	{
		.tag_idx = SVTAG_2,
		.seninf_padidx = PAD_SRC_RAW2,
		.tag_order = MTKCAM_IPI_ORDER_LAST_TAG,
		.is_w = false,
	},
};

static const struct mtk_camsv_tag_param sv_tag_param_display_ic[3] = {
	{
		.tag_idx = SVTAG_0,
		.seninf_padidx = PAD_SRC_RAW0,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = false,
	},
	{
		.tag_idx = SVTAG_1,
		.seninf_padidx = PAD_SRC_RAW1,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = false,
	},
	{
		.tag_idx = SVTAG_2,
		.seninf_padidx = PAD_SRC_GENERAL0,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = false,
	},
};

static unsigned int get_last_done_tag(unsigned int group_tag)
{
	unsigned int rst = 0, i;

	for (i = 0; i < MAX_SV_HW_TAGS; i++) {
		if (group_tag & (1 << i)) {
			rst = (1 << i);
			break;
		}
	}

	return rst;
}

static int sv_process_fsm(struct mtk_camsv_device *sv_dev,
			  struct mtk_camsys_irq_info *irq_info,
			  int *recovered_done)
{
	struct engine_fsm *fsm = &sv_dev->fsm;
	unsigned int done_type, sof_type, i;
	int recovered = 0;

	done_type = irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_DONE);
	if (done_type) {
		int cookie_done;
		int ret;

		for (i = 0; i < MAX_SV_HW_GROUPS; i++) {
			if (irq_info->done_tags & sv_dev->active_group_info[i])
				sv_dev->group_handled |= (1 << i) & sv_dev->used_group;
		}

		if (irq_info->done_tags & sv_dev->last_done_tag) {
			if (sv_dev->group_handled == sv_dev->used_group) {
				ret = engine_fsm_hw_done(fsm, &cookie_done);
				if (ret > 0) {
					irq_info->cookie_done = cookie_done;
					sv_dev->group_handled = 0;
				} else {
					/* handle for fake p1 done */
					dev_info_ratelimited(sv_dev->dev, "warn: fake done in/out: 0x%x 0x%x\n",
								 irq_info->frame_idx_inner,
								 irq_info->frame_idx);
					irq_info->irq_type &= ~done_type;
					irq_info->cookie_done = 0;
					sv_dev->group_handled = 0;
				}
			} else {
				dev_info_ratelimited(sv_dev->dev, "%s: warn: last group done comes without all group done received: in/out: 0x%x 0x%x group used/handled: 0x%x 0x%x\n",
					__func__,
					irq_info->frame_idx,
					irq_info->frame_idx_inner,
					sv_dev->used_group,
					sv_dev->group_handled);
				irq_info->irq_type &= ~done_type;
				irq_info->cookie_done = 0;
				sv_dev->group_handled = 0;
			}
		} else {
			irq_info->irq_type &= ~done_type;
			irq_info->cookie_done = 0;
		}
	}

	sof_type = irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_START);
	if (sof_type) {
		/* when first tag comes: 1. update used group 2. update last done tag */
		if (irq_info->sof_tags & sv_dev->first_tag) {
			for (i = 0; i < MAX_SV_HW_GROUPS; i++) {
				if (sv_dev->enque_tags & sv_dev->active_group_info[i]) {
					sv_dev->used_group |= (1 << i);
					sv_dev->last_done_tag =
						get_last_done_tag(sv_dev->active_group_info[i]);
				}
			}
		} else
			irq_info->irq_type &= ~sof_type;

		/* update fsm */
		if (irq_info->sof_tags & sv_dev->last_done_tag) {
			for (i = 0; i < MAX_SV_HW_GROUPS; i++) {
				if (sv_dev->last_done_tag & sv_dev->active_group_info[i]) {

					recovered =
						engine_fsm_sof(&sv_dev->fsm,
							       irq_info->frame_idx_inner,
							       irq_info->fbc_empty,
							       recovered_done);
					break;
				}
			}
		}
	}

	if (recovered)
		dev_info(sv_dev->dev, "recovered done 0x%x in/out: 0x%x 0x%x\n",
			 *recovered_done,
			 irq_info->frame_idx_inner,
			 irq_info->frame_idx);

	return recovered;
}

static void mtk_camsv_register_iommu_tf_callback(struct mtk_camsv_device *sv_dev)
{
#ifdef CAMSYS_TF_DUMP_7S
	dev_dbg(sv_dev->dev, "%s : sv->id:%d\n", __func__, sv_dev->id);

	switch (sv_dev->id) {
	case CAMSV_0:
		mtk_iommu_register_fault_callback(M4U_PORT_L14_CAMSV_0_WDMA,
			mtk_camsv_translation_fault_callback, (void *)sv_dev, false);
		break;
	case CAMSV_1:
		mtk_iommu_register_fault_callback(M4U_PORT_L13_CAMSV_1_WDMA,
			mtk_camsv_translation_fault_callback, (void *)sv_dev, false);
		break;
	case CAMSV_2:
		mtk_iommu_register_fault_callback(M4U_PORT_L29_CAMSV_2_WDMA,
			mtk_camsv_translation_fault_callback, (void *)sv_dev, false);
		break;
	case CAMSV_3:
		mtk_iommu_register_fault_callback(M4U_PORT_L29_CAMSV_3_WDMA,
			mtk_camsv_translation_fault_callback, (void *)sv_dev, false);
		break;
	case CAMSV_4:
		mtk_iommu_register_fault_callback(M4U_PORT_L29_CAMSV_4_WDMA,
			mtk_camsv_translation_fault_callback, (void *)sv_dev, false);
		break;
	case CAMSV_5:
		mtk_iommu_register_fault_callback(M4U_PORT_L29_CAMSV_5_WDMA,
			mtk_camsv_translation_fault_callback, (void *)sv_dev, false);
		break;
	}
#endif
};

#ifdef CAMSYS_TF_DUMP_7S
int mtk_camsv_translation_fault_callback(int port, dma_addr_t mva, void *data)
{
	int index;
	struct mtk_camsv_device *sv_dev = (struct mtk_camsv_device *)data;

	dev_info(sv_dev->dev, "tg_sen_mode:0x%x tg_vf_con:0x%x tg_path_cfg:0x%x",
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_SEN_MODE),
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_VF_CON),
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_PATH_CFG));

	for (index = 0; index < MAX_SV_HW_TAGS; index++) {
		dev_info(sv_dev->dev, "tag:%d tg_grab_pxl:0x%x tg_grab_lin:0x%x fmt:0x%x imgo_fbc0: 0x%x imgo_fbc1: 0x%x",
		index,
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_GRAB_PXL_TAG1 +
			index * CAMSVCENTRAL_GRAB_PXL_TAG_SHIFT),
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_GRAB_LIN_TAG1 +
			index * CAMSVCENTRAL_GRAB_LIN_TAG_SHIFT),
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FORMAT_TAG1 +
			index * CAMSVCENTRAL_FORMAT_TAG_SHIFT),
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FBC0_TAG1 +
			index * CAMSVCENTRAL_FBC0_TAG_SHIFT),
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FBC1_TAG1 +
			index * CAMSVCENTRAL_FBC1_TAG_SHIFT));
	}


	for (index = 0; index < MAX_SV_HW_TAGS; index++) {
		dev_info(sv_dev->dev, "tag:%d imgo_stride_img:0x%x imgo_addr_img:0x%x_%x",
			index,
			readl_relaxed(sv_dev->base_inner_dma +
				REG_CAMSVDMATOP_WDMA_BASIC_IMG1 +
				index * CAMSVDMATOP_WDMA_BASIC_IMG_SHIFT),
			readl_relaxed(sv_dev->base_inner_dma +
				REG_CAMSVDMATOP_WDMA_BASE_ADDR_IMG1 +
				index * CAMSVDMATOP_WDMA_BASE_ADDR_IMG_SHIFT),
			readl_relaxed(sv_dev->base_inner_dma +
				REG_CAMSVDMATOP_WDMA_BASE_ADDR_MSB_IMG1 +
				index * CAMSVDMATOP_WDMA_BASE_ADDR_MSB_IMG_SHIFT));
	}
	return 0;
}
#endif

static int reset_msgfifo(struct mtk_camsv_device *sv_dev)
{
	atomic_set(&sv_dev->is_fifo_overflow, 0);
	return kfifo_init(&sv_dev->msg_fifo, sv_dev->msg_buffer, sv_dev->fifo_size);
}

static int push_msgfifo(struct mtk_camsv_device *sv_dev,
			struct mtk_camsys_irq_info *info)
{
	int len;

	if (unlikely(kfifo_avail(&sv_dev->msg_fifo) < sizeof(*info))) {
		atomic_set(&sv_dev->is_fifo_overflow, 1);
		return -1;
	}

	len = kfifo_in(&sv_dev->msg_fifo, info, sizeof(*info));
	WARN_ON(len != sizeof(*info));

	return 0;
}

void sv_reset_by_camsys_top(struct mtk_camsv_device *sv_dev)
{
	int cq_dma_sw_ctl;
	int ret;

	dev_info(sv_dev->dev, "%s camsv_id:%d\n", __func__, sv_dev->id);

	writel(0, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL);
	writel(1, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL);
	wmb(); /* make sure committed */

	ret = readx_poll_timeout(readl, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL,
				cq_dma_sw_ctl,
				cq_dma_sw_ctl & 0x2,
				1 /* delay, us */,
				100000 /* timeout, us */);
	if (ret < 0) {
		dev_info(sv_dev->dev, "%s: timeout\n", __func__);

		dev_info(sv_dev->dev,
			 "tg_sen_mode: 0x%x, cq_dma_sw_ctl:0x%x, frame_no:0x%x\n",
			 readl(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE),
			 readl(sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL),
			 readl(sv_dev->base + REG_CAMSVCENTRAL_FRAME_SEQ_NO)
			);
		mtk_smi_dbg_hang_detect("camsys-camsv");
		goto RESET_FAILURE;
	}
	writel(0, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL);
	writel(0, sv_dev->cam->base + REG_CAM_MAIN_SW_RST_1);
	writel(3 << ((sv_dev->id) * 2), sv_dev->cam->base + REG_CAM_MAIN_SW_RST_1);
	writel(0, sv_dev->cam->base + REG_CAM_MAIN_SW_RST_1);
	wmb(); /* make sure committed */

RESET_FAILURE:
	return;
}

void sv_reset(struct mtk_camsv_device *sv_dev)
{
	int dma_sw_ctl, cq_dma_sw_ctl;
	int ret;

	dev_dbg(sv_dev->dev, "%s camsv_id:%d\n", __func__, sv_dev->id);

	writel(0, sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL);
	writel(1, sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL);
	wmb(); /* make sure committed */

	ret = readx_poll_timeout(readl, sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL,
				 dma_sw_ctl,
				 dma_sw_ctl & 0x2,
				 1 /* delay, us */,
				 100000 /* timeout, us */);
	if (ret < 0) {
		dev_info(sv_dev->dev, "%s: timeout\n", __func__);

		dev_info(sv_dev->dev,
			 "tg_sen_mode: 0x%x, dma_sw_ctl:0x%x, frame_no:0x%x\n",
			 readl(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE),
			 readl(sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL),
			 readl(sv_dev->base + REG_CAMSVCENTRAL_FRAME_SEQ_NO)
			);
		mtk_smi_dbg_hang_detect("camsys-camsv");
		goto RESET_FAILURE;
	}

	/* enable dma dcm after dma is idle */
	writel(0, sv_dev->base + REG_CAMSVCENTRAL_DCM_DIS);

	writel(0, sv_dev->base + REG_CAMSVCENTRAL_SW_CTL);
	writel(1, sv_dev->base + REG_CAMSVCENTRAL_SW_CTL);
	writel(0, sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL);
	writel(0, sv_dev->base + REG_CAMSVCENTRAL_SW_CTL);
	wmb(); /* make sure committed */

	/* reset cq dma */
	writel(0, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL);
	writel(1, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL);
	wmb(); /* make sure committed */

	ret = readx_poll_timeout(readl, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL,
				cq_dma_sw_ctl,
				cq_dma_sw_ctl & 0x2,
				1 /* delay, us */,
				100000 /* timeout, us */);
	if (ret < 0) {
		dev_info(sv_dev->dev, "%s: timeout\n", __func__);

		dev_info(sv_dev->dev,
			 "tg_sen_mode: 0x%x, cq_dma_sw_ctl:0x%x, frame_no:0x%x\n",
			 readl(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE),
			 readl(sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL),
			 readl(sv_dev->base + REG_CAMSVCENTRAL_FRAME_SEQ_NO)
			);
		mtk_smi_dbg_hang_detect("camsys-camsv");
		goto RESET_FAILURE;
	}
	writel(0, sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL);
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


RESET_FAILURE:
	return;
}

int mtk_cam_sv_dmao_common_config(struct mtk_camsv_device *sv_dev)
{
	int ret = 0;

	switch (sv_dev->id) {
	case CAMSV_0:
		/* imgo */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x880406AE);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x15580402);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x12AC0156);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x81560000);
		break;
	case CAMSV_1:
		/* imgo */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x856A0483);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x139C02B5);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x11CE00E7);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x80E70000);
		break;
	case CAMSV_2:
		/* imgo */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x84CE0401);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x13340267);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x119A00CD);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x80CD0000);
		break;
	case CAMSV_3:
		/* imgo */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x83000280);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x12000180);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x11000080);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x80800000);
		break;
	case CAMSV_4:
		/* imgo */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x80D800B4);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x1090006C);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x10480024);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x80240000);
		break;
	case CAMSV_5:
		/* imgo */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG, 0x80D800B4);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG, 0x1090006C);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG, 0x10480024);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG, 0x80240000);
		break;
	}

	/* cqi */
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSV_M1_CQI_ORIRDMA_CON0, 0x10000040);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSV_M1_CQI_ORIRDMA_CON1, 0x000D0007);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSV_M1_CQI_ORIRDMA_CON2, 0x001A0014);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSV_M1_CQI_ORIRDMA_CON3, 0x00270020);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSV_M1_CQI_ORIRDMA_CON4, 0x00070000);

	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSV_M2_CQI_ORIRDMA_CON0, 0x10000040);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSV_M2_CQI_ORIRDMA_CON1, 0x000D0007);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSV_M2_CQI_ORIRDMA_CON2, 0x001A0014);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSV_M2_CQI_ORIRDMA_CON3, 0x00270020);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSV_M2_CQI_ORIRDMA_CON4, 0x00070000);

	return ret;
}

int mtk_cam_sv_toggle_tg_db(struct mtk_camsv_device *sv_dev)
{
	int val, val2;

	val = CAMSV_READ_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_PATH_CFG);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_PATH_CFG,
		CAMSVCENTRAL_PATH_CFG, SYNC_VF_EN_DB_LOAD_DIS, 1);

	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_PATH_CFG,
		CAMSVCENTRAL_PATH_CFG, SYNC_VF_EN_DB_LOAD_DIS, 0);
	val2 = CAMSV_READ_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_PATH_CFG);
	dev_info(sv_dev->dev, "%s 0x%x->0x%x\n", __func__, val, val2);
	return 0;
}

int mtk_cam_sv_toggle_db(struct mtk_camsv_device *sv_dev)
{
	int val, val2;

	val = CAMSV_READ_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_MODULE_DB);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_MODULE_DB,
		CAMSVCENTRAL_MODULE_DB, CAM_DB_EN, 0);

	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_MODULE_DB,
		CAMSVCENTRAL_MODULE_DB, CAM_DB_EN, 1);
	val2 = CAMSV_READ_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_MODULE_DB);
	dev_info(sv_dev->dev, "%s 0x%x->0x%x\n", __func__, val, val2);

	return 0;
}

int mtk_cam_sv_central_common_enable(struct mtk_camsv_device *sv_dev)
{
	int ret = 0;

	mtk_cam_sv_toggle_db(sv_dev);
	mtk_cam_sv_toggle_tg_db(sv_dev);

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

int mtk_cam_sv_print_fbc_status(struct mtk_camsv_device *sv_dev)
{
	int ret = 0, i;

	if (debug_sv_fbc) {
		for (i = SVTAG_START; i < SVTAG_END; i++) {
			dev_info(sv_dev->dev, "%s tag_idx(%d) FBC_IMGO_CTRL1/2:0x%x/0x%x\n",
				__func__, i,
				CAMSV_READ_REG(sv_dev->base + REG_CAMSVCENTRAL_FBC0_TAG1 +
					CAMSVCENTRAL_FBC0_TAG_SHIFT * i),
				CAMSV_READ_REG(sv_dev->base + REG_CAMSVCENTRAL_FBC1_TAG1 +
					CAMSVCENTRAL_FBC0_TAG_SHIFT * i));
		}
	}

	return ret;
}

int mtk_cam_sv_central_common_disable(struct mtk_camsv_device *sv_dev)
{
	int ret = 0;

	/* disable dma dcm before do dma reset */
	writel(1, sv_dev->base + REG_CAMSVCENTRAL_DCM_DIS);

	/* bypass tg_mode function before vf off */
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, TG_MODE_OFF, 1);

	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_VF_CON,
		CAMSVCENTRAL_VF_CON, VFDATA_EN, 0);

	mtk_cam_sv_toggle_tg_db(sv_dev);

	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DONE_STATUS_EN, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_ERR_STATUS_EN, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_SOF_STATUS_EN, 0);

	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, CMOS_EN, 0);

	sv_reset(sv_dev);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DMA_EN_IMG, 0);
	mtk_cam_sv_toggle_db(sv_dev);

	return ret;
}

int mtk_cam_sv_fbc_disable(struct mtk_camsv_device *sv_dev, unsigned int tag_idx)
{
	int ret = 0;

	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_FBC0_TAG1 +
		CAMSVCENTRAL_FBC0_TAG_SHIFT * tag_idx, 0);

	return ret;
}

int mtk_cam_sv_dev_pertag_write_rcnt(struct mtk_camsv_device *sv_dev,
	unsigned int tag_idx)
{
	int ret = 0;

	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_FBC0_TAG1 +
		CAMSVCENTRAL_FBC0_TAG_SHIFT * tag_idx,
		CAMSVCENTRAL_FBC0_TAG1, RCNT_INC_TAG1, 1);

	return ret;
}

void mtk_cam_sv_vf_reset(struct mtk_camsv_device *sv_dev)
{
	if (CAMSV_READ_BITS(sv_dev->base + REG_CAMSVCENTRAL_VF_CON,
			CAMSVCENTRAL_VF_CON, VFDATA_EN)) {
		CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_VF_CON,
			CAMSVCENTRAL_VF_CON, VFDATA_EN, 0);
		mtk_cam_sv_toggle_tg_db(sv_dev);
		dev_info(sv_dev->dev, "preisp sv_vf_reset vf_en off");
		CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_VF_CON,
			CAMSVCENTRAL_VF_CON, VFDATA_EN, 1);
		mtk_cam_sv_toggle_tg_db(sv_dev);
		dev_info(sv_dev->dev, "preisp sv_vf_reset vf_en on");
	}
	dev_info(sv_dev->dev, "preisp sv_vf_reset");
}

int mtk_cam_sv_is_zero_fbc_cnt(struct mtk_camsv_device *sv_dev,
	unsigned int tag_idx)
{
	if (CAMSV_READ_BITS(sv_dev->base_inner +
			REG_CAMSVCENTRAL_FBC1_TAG1 + CAMSVCENTRAL_FBC1_TAG_SHIFT * tag_idx,
			CAMSVCENTRAL_FBC1_TAG1, FBC_CNT_TAG1) == 0)
		return 1;

	return 0;
}

void mtk_cam_sv_check_fbc_cnt(struct mtk_camsv_device *sv_dev,
	unsigned int tag_idx)
{
	unsigned int fbc_cnt = 0;

	fbc_cnt = CAMSV_READ_BITS(sv_dev->base +
		REG_CAMSVCENTRAL_FBC1_TAG1 + CAMSVCENTRAL_FBC1_TAG_SHIFT * tag_idx,
		CAMSVCENTRAL_FBC1_TAG1, FBC_CNT_TAG1);

	while (fbc_cnt < 2) {
		mtk_cam_sv_dev_pertag_write_rcnt(sv_dev, tag_idx);
		fbc_cnt++;
	}
}

int mtk_cam_sv_frame_no_inner(struct mtk_camsv_device *sv_dev)
{
	return readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FRAME_SEQ_NO);
}

void apply_camsv_cq(struct mtk_camsv_device *sv_dev,
	struct apply_cq_ref *ref,
	dma_addr_t cq_addr, unsigned int cq_size,
	unsigned int cq_offset, int initial)
{
#define CQ_VADDR_MASK 0xffffffff
	u32 cq_addr_lsb = (cq_addr + cq_offset) & CQ_VADDR_MASK;
	u32 cq_addr_msb = ((cq_addr + cq_offset) >> 32);

	if (cq_size == 0)
		return;

	if (WARN_ON(assign_apply_cq_ref(&sv_dev->cq_ref, ref)))
		return;

	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_CQ_SUB_THR0_DESC_SIZE_2,
		cq_size);
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_CQ_SUB_THR0_BASEADDR_2_MSB,
		cq_addr_msb);
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_CQ_SUB_THR0_BASEADDR_2,
		cq_addr_lsb);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_THR_START,
		CAMSVCQTOP_THR_START, CAMSVCQTOP_CSR_CQ_THR0_START, 1);

	if (initial)
		dev_info(sv_dev->dev, "apply 1st camsv scq: addr_msb:0x%x addr_lsb:0x%x size:%d cq_en(0x%x))",
			cq_addr_msb, cq_addr_lsb, cq_size,
			readl_relaxed(sv_dev->base_scq + REG_CAMSVCQTOP_THR_START));
	else
		dev_dbg(sv_dev->dev, "apply camsv scq: addr_msb:0x%x addr_lsb:0x%x size:%d",
			cq_addr_msb, cq_addr_lsb, cq_size);
}

bool mtk_cam_is_display_ic(struct mtk_cam_ctx *ctx)
{
	struct mtk_camsv_pipeline *sv_pipe;

	if (!ctx->num_sv_subdevs)
		return false;

	sv_pipe = &ctx->cam->pipelines.camsv[ctx->sv_subdev_idx[0]];

	return (sv_pipe->feature_pending & DISPLAY_IC) ? true : false;
}

unsigned int mtk_cam_get_sv_tag_index(struct mtk_camsv_device *sv_dev,
		unsigned int pipe_id)
{
	int i;

	for (i = SVTAG_START; i < SVTAG_END; i++) {
		if (sv_dev->enabled_tags & (1 << i)) {
			struct mtk_camsv_tag_info *tag_info =
				&sv_dev->tag_info[i];
			if (tag_info->sv_pipe && (tag_info->sv_pipe->id == pipe_id))
				return i;
		}
	}

	dev_info(sv_dev->dev, "[%s] tag is not found by pipe_id(%d)",
		__func__, pipe_id);
	return 0;
}

int mtk_cam_sv_dev_config(struct mtk_camsv_device *sv_dev)
{
	engine_fsm_reset(&sv_dev->fsm, sv_dev->dev);
	sv_dev->cq_ref = NULL;

	sv_dev->enque_tags = 0;
	sv_dev->first_tag = 0;
	sv_dev->last_tag = 0;
	sv_dev->last_done_tag = 0;

	sv_dev->group_handled = 0;
	sv_dev->used_group = 0;

	sv_dev->streaming_tag_cnt = 0;
	sv_dev->sof_count = 0;
	sv_dev->tg_cnt = 0;
	mtk_camsv_register_iommu_tf_callback(sv_dev);

	mtk_cam_sv_dmao_common_config(sv_dev);
	mtk_cam_sv_cq_config(sv_dev);
	mtk_cam_sv_cq_enable(sv_dev);
	dev_info(sv_dev->dev, "[%s] sv_dev->id:%d",
		__func__, sv_dev->id);

	return 0;
}

void mtk_cam_sv_fill_tag_info(struct mtk_camsv_tag_info *arr_tag,
	struct mtk_camsv_tag_param *tag_param, unsigned int hw_scen,
	unsigned int pixelmode, unsigned int sub_ratio,
	unsigned int mbus_width, unsigned int mbus_height,
	unsigned int mbus_code,	struct mtk_camsv_pipeline *pipeline)
{
	struct mtk_camsv_tag_info *tag_info = &arr_tag[tag_param->tag_idx];

	tag_info->sv_pipe = pipeline;
	tag_info->seninf_padidx = tag_param->seninf_padidx;
	tag_info->hw_scen = hw_scen;
	tag_info->tag_order = tag_param->tag_order;

	tag_info->cfg_in_param.pixel_mode = pixelmode;
	tag_info->cfg_in_param.data_pattern = 0x0;
	tag_info->cfg_in_param.in_crop.p.x = 0x0;
	tag_info->cfg_in_param.in_crop.p.y = 0x0;
	tag_info->cfg_in_param.in_crop.s.w = mbus_width;
	tag_info->cfg_in_param.in_crop.s.h = mbus_height;
	tag_info->cfg_in_param.fmt = sensor_mbus_to_ipi_fmt(mbus_code);
	tag_info->cfg_in_param.raw_pixel_id = sensor_mbus_to_ipi_pixel_id(mbus_code);
	tag_info->cfg_in_param.subsample = sub_ratio - 1; /* TODO(AY): remove -1 */
}

void mtk_cam_sv_reset_tag_info(struct mtk_camsv_device *sv_dev)
{
	struct mtk_camsv_tag_info *tag_info;
	int i;

	sv_dev->used_tag_cnt = 0;
	sv_dev->enabled_tags = 0;
	for (i = SVTAG_START; i < SVTAG_END; i++) {
		tag_info = &sv_dev->tag_info[i];
		tag_info->sv_pipe = NULL;
		tag_info->seninf_padidx = 0;
		tag_info->hw_scen = 0;
		tag_info->tag_order = MTKCAM_IPI_ORDER_FIRST_TAG;
	}
}

int mtk_cam_sv_get_tag_param(struct mtk_camsv_tag_param *arr_tag_param,
	unsigned int hw_scen, unsigned int exp_no, unsigned int req_amount)
{
	int ret = 0;

	if (hw_scen == (1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_STAGGER)) ||
		hw_scen == (1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_DC_STAGGER))) {
		if (exp_no == 2)
			memcpy(arr_tag_param, sv_tag_param_2exp_stagger,
				sizeof(struct mtk_camsv_tag_param) * req_amount);
		else if (exp_no == 3)
			memcpy(arr_tag_param, sv_tag_param_3exp_stagger,
				sizeof(struct mtk_camsv_tag_param) * req_amount);
		else
			memcpy(arr_tag_param, sv_tag_param_normal,
				sizeof(struct mtk_camsv_tag_param) * req_amount);
	} else if (hw_scen == (1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_ON_THE_FLY)) ||
		hw_scen == (1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_DC))) {
		memcpy(arr_tag_param, sv_tag_param_normal,
			sizeof(struct mtk_camsv_tag_param) * req_amount);
	} else if (hw_scen == (1 << MTKCAM_SV_SPECIAL_SCENARIO_DISPLAY_IC)) {
		memcpy(arr_tag_param, sv_tag_param_display_ic,
			sizeof(struct mtk_camsv_tag_param) * req_amount);
	} else {
		pr_info("failed to get tag param(hw_scen:0x%x/exp_no:%d)",
			hw_scen, exp_no);
		ret = 1;
	}

	return ret;
}

int mtk_cam_sv_cq_config(struct mtk_camsv_device *sv_dev)
{
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_CQ_DB_EN, 0);
	/* always enable stagger mode for multiple vsync(s) */
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_STAGGER_MODE, 1);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_MODE, 1);
	/* camsv todo: start period need to be calculated */
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_SCQ_START_PERIOD,
		0xFFFFFFFF);

	return 0;
}

int mtk_cam_sv_cq_enable(struct mtk_camsv_device *sv_dev)
{
	int i, subsample = 0;

	for (i = SVTAG_START; i < SVTAG_END; i++) {
		if (sv_dev->enabled_tags & (1 << i)) {
			subsample = sv_dev->tag_info[i].cfg_in_param.subsample;
			break;
		}
	}

	if (subsample)
		CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
			CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_SUBSAMPLE_EN, 1);

	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_EN, 1);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_INT_0_EN,
		CAMSVCQTOP_INT_0_EN, CAMSVCQTOP_CSR_SCQ_SUB_THR_DONE_INT_EN, 1);

	return 0;
}

int mtk_cam_sv_cq_disable(struct mtk_camsv_device *sv_dev)
{
	int i, subsample = 0;

	for (i = SVTAG_START; i < SVTAG_END; i++) {
		if (sv_dev->enabled_tags & (1 << i)) {
			subsample = sv_dev->tag_info[i].cfg_in_param.subsample;
			break;
		}
	}

	if (subsample) {
		CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
			CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_SUBSAMPLE_EN, 0);
	}

	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_EN, 0);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_STAGGER_MODE, 0);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_MODE, 0);
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_SCQ_START_PERIOD, 0);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_INT_0_EN,
		CAMSVCQTOP_INT_0_EN, CAMSVCQTOP_CSR_SCQ_SUB_THR_DONE_INT_EN, 0);

	return 0;
}

int mtk_cam_get_sv_cammux_id(struct mtk_camsv_device *sv_dev, int tag_idx)
{
	int cammux_id = 0;

	switch (sv_dev->id) {
	case CAMSV_0:
	case CAMSV_1:
	case CAMSV_2:
	case CAMSV_3:
		cammux_id = sv_dev->cammux_id + tag_idx;
		break;
	case CAMSV_4:
	case CAMSV_5:
		cammux_id = sv_dev->cammux_id;
		break;
	}

	return cammux_id;
}

int mtk_cam_sv_dev_pertag_stream_on(
	struct mtk_camsv_device *sv_dev,
	unsigned int tag_idx,
	bool on)
{
#ifdef HS_TODO
	struct mtk_camsv_pipeline *sv_pipe;
#endif
	int ret = 0;

	if (on) {
		sv_dev->streaming_tag_cnt++;
		if (sv_dev->streaming_tag_cnt == sv_dev->used_tag_cnt)
			ret |= mtk_cam_sv_central_common_enable(sv_dev);
	} else {
		if (sv_dev->streaming_tag_cnt == 0)
			goto EXIT;
		if (sv_dev->streaming_tag_cnt == sv_dev->used_tag_cnt) {
			ret |= mtk_cam_sv_cq_disable(sv_dev);
			ret |= mtk_cam_sv_central_common_disable(sv_dev);
		}
#ifdef HS_TODO
		if (watchdog_scenario(ctx) &&
			(tag_idx >= SVTAG_META_START &&
			tag_idx < SVTAG_META_END)) {
			sv_pipe = sv_dev->tag_info[tag_idx].sv_pipe;
			if (sv_pipe)
				mtk_ctx_watchdog_stop(ctx, sv_pipe->id);
		}
#endif

		ret |= mtk_cam_sv_fbc_disable(sv_dev, tag_idx);
		sv_dev->streaming_tag_cnt--;
	}

EXIT:
	dev_info(sv_dev->dev, "camsv %d %s en(%d) streaming_tag_cnt:%d\n",
		sv_dev->id, __func__, (on) ? 1 : 0, sv_dev->streaming_tag_cnt);
	return ret;
}

int mtk_cam_sv_dev_stream_on(struct mtk_camsv_device *sv_dev, bool on)
{
	int ret = 0, i;

	if (sv_dev) {
		for (i = SVTAG_START; i < SVTAG_END; i++) {
			if (sv_dev->enabled_tags & (1 << i))
				mtk_cam_sv_dev_pertag_stream_on(sv_dev, i, on);
		}
	}

	return ret;
}

void camsv_dump_dma_debug_data(struct mtk_camsv_device *sv_dev)
{
	u32 smi_crc_address, smi_crc_data, tag1_tag2_crc, len1_len2_crc, smi_cnt;
	u32 debug_img1, debug_len1, cmd_cnt_img1, cmd_cnt_len1;

	writel_relaxed(0x00010001, sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_SEL);
	smi_crc_address = readl_relaxed(sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x00010003, sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_SEL);
	smi_crc_data = readl_relaxed(sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x00010005, sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_SEL);
	tag1_tag2_crc = readl_relaxed(sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x00010009, sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_SEL);
	len1_len2_crc = readl_relaxed(sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x0001000F, sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_SEL);
	smi_cnt = readl_relaxed(sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x0001010B, sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_SEL);
	debug_img1 = readl_relaxed(sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x0001010C, sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_SEL);
	debug_len1 = readl_relaxed(sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x0001010E, sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_SEL);
	cmd_cnt_img1 = readl_relaxed(sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x0001090E, sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_SEL);
	cmd_cnt_len1 = readl_relaxed(sv_dev->base_dma + REG_CAMSV_DMATOP_DMA_DEBUG_PORT);

	dev_info_ratelimited(sv_dev->dev,
		"dma_top_debug:0x%x_0x%x_0x%x_0x%x_0x%x_0x%x_0x%x_0x%x_0x%x\n",
		smi_crc_address, smi_crc_data, tag1_tag2_crc, len1_len2_crc,
		smi_cnt, debug_img1, debug_len1, cmd_cnt_img1,
		cmd_cnt_len1);
}

void camsv_irq_handle_err(
	struct mtk_camsv_device *sv_dev,
	unsigned int dequeued_frame_seq_no, unsigned int tag_idx)
{
#ifdef NOT_READY
	struct mtk_cam_request_stream_data *s_data;
	struct mtk_cam_device *cam = sv_dev->cam;
	struct mtk_cam_ctx *ctx;
	struct mtk_camsv_tag_info tag_info = sv_dev->tag_info[tag_idx];
	unsigned int stream_id;

	dev_info_ratelimited(sv_dev->dev,
		"TAG_IDX:%d TG PATHCFG/SENMODE FRMSIZE/R RGRABPXL/LIN:0x%x/%x 0x%x/%x 0x%x/%x\n",
		tag_idx,
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_PATH_CFG),
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE),
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_FRMSIZE_ST),
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_FRMSIZE_ST_R),
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_GRAB_PXL_TAG1 +
			CAMSVCENTRAL_GRAB_PXL_TAG_SHIFT * tag_idx),
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_GRAB_LIN_TAG1 +
			CAMSVCENTRAL_GRAB_LIN_TAG_SHIFT * tag_idx));

	if (sv_dev->tag_info[tag_idx].hw_scen &
		MTK_CAMSV_SUPPORTED_SPECIAL_HW_SCENARIO) {
		if (sv_dev->ctx_stream_id >= MTKCAM_SUBDEV_CAMSV_END) {
			dev_info(sv_dev->dev, "stream id out of range : %d",
					sv_dev->ctx_stream_id);
			return;
		}
		ctx = &cam->ctxs[sv_dev->ctx_stream_id];
		if (!ctx) {
			dev_info(sv_dev->dev, "%s: cannot find ctx\n", __func__);
			return;
		}
		stream_id = sv_dev->ctx_stream_id;
	} else {
		if (sv_dev->ctx_stream_id >= MTKCAM_SUBDEV_CAMSV_END) {
			dev_info(sv_dev->dev, "stream id out of range : %d",
					sv_dev->ctx_stream_id);
			return;
		}
		ctx = &cam->ctxs[sv_dev->ctx_stream_id];
		if (!ctx) {
			dev_info(sv_dev->dev, "%s: cannot find ctx\n", __func__);
			return;
		}
		if (!tag_info.sv_pipe) {
			dev_info(sv_dev->dev, "%s: tag_idx:%d is not controlled by user\n",
				__func__, tag_idx);
			return;
		}
		stream_id = tag_info.sv_pipe->id;
	}

	s_data = mtk_cam_get_req_s_data(ctx, stream_id, dequeued_frame_seq_no);
	if (s_data) {
		mtk_cam_debug_seninf_dump(s_data);
	} else {
		dev_info(sv_dev->dev,
			 "%s: req(%d) can't be found for seninf dump\n",
			 __func__, dequeued_frame_seq_no);
	}
#endif
}

void camsv_handle_err(
	struct mtk_camsv_device *sv_dev,
	struct mtk_camsys_irq_info *data)
{
	int err_status = data->e.err_status;
	int frame_idx_inner = data->frame_idx_inner;
	int tag_idx;
	unsigned int fbc_imgo_status, imgo_addr, imgo_addr_msb, dcif_set, tg_vf_con;
	unsigned int first_tag, last_tag, group_info, i;
	/* camsv todo: show more error detail */
	for (tag_idx = 0; tag_idx < MAX_SV_HW_TAGS; tag_idx++) {
		if (!(data->err_tags & (1 << tag_idx)))
			continue;
		fbc_imgo_status =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_FBC1_TAG1 + tag_idx *
			CAMSVCENTRAL_FBC1_TAG_SHIFT);
		imgo_addr =
			readl_relaxed(sv_dev->base_dma + REG_CAMSVDMATOP_WDMA_BASE_ADDR_IMG1
			+ tag_idx * CAMSVDMATOP_WDMA_BASE_ADDR_IMG_SHIFT);
		imgo_addr_msb =
			readl_relaxed(sv_dev->base_dma +
			REG_CAMSVDMATOP_WDMA_BASE_ADDR_MSB_IMG1 +  tag_idx *
			CAMSVDMATOP_WDMA_BASE_ADDR_MSB_IMG_SHIFT);
		dcif_set =
			readl_relaxed(sv_dev->base_inner + REG_E_CAMSVCENTRAL_DCIF_SET);
		tg_vf_con =
			readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_VF_CON);
		dev_info_ratelimited(sv_dev->dev,
			"camsv_id:%d tag_idx:%d error_status:0x%x fbc_status:0x%x imgo_addr:0x%x%08x dcif_set:0x%x tg_vf:0x%x\n",
			sv_dev->id, tag_idx, err_status, fbc_imgo_status, imgo_addr_msb,
			imgo_addr, dcif_set, tg_vf_con);
		camsv_irq_handle_err(sv_dev, frame_idx_inner, tag_idx);
	}
	first_tag = readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_FIRST_TAG);
	last_tag = readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_LAST_TAG);
	for (i = 0; i < MAX_SV_HW_GROUPS; i++) {
		group_info = readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_GROUP_TAG0 +
			REG_CAMSVCENTRAL_GROUP_TAG_SHIFT * i);
		dev_info_ratelimited(sv_dev->dev, "group%d: group_info:%x", i, group_info);
	}
	dev_info_ratelimited(sv_dev->dev, "first_tag:0x%x last_tag:0x%x",
		first_tag, last_tag);
	if (!(data->err_tags) && (err_status & CAMSVCENTRAL_DMA_SRAM_FULL_ST))
		dev_info_ratelimited(sv_dev->dev, "camsv_id:%d camsv dma full error_status:0x%x",
			sv_dev->id, err_status);
	/* dump dma debug data */
	camsv_dump_dma_debug_data(sv_dev);
}

static irqreturn_t mtk_irq_camsv_done(int irq, void *data)
{
	struct mtk_camsv_device *sv_dev = (struct mtk_camsv_device *)data;
	struct mtk_camsys_irq_info irq_info;
	unsigned int dequeued_imgo_seq_no, dequeued_imgo_seq_no_inner;
	unsigned int irq_done_status;
	unsigned int irq_flag = 0;
	bool wake_thread = 0;

	memset(&irq_info, 0, sizeof(irq_info));
	dequeued_imgo_seq_no =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_FRAME_SEQ_NO);
	dequeued_imgo_seq_no_inner =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FRAME_SEQ_NO);
	irq_done_status	=
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_DONE_STATUS);

	dev_dbg(sv_dev->dev, "camsv-%d: done status:0x%x seq_no:%d_%d",
		sv_dev->id, irq_done_status,
		dequeued_imgo_seq_no_inner, dequeued_imgo_seq_no);

	irq_info.irq_type = 0;
	irq_info.ts_ns = ktime_get_boottime_ns();
	irq_info.frame_idx = dequeued_imgo_seq_no;
	irq_info.frame_idx_inner = dequeued_imgo_seq_no_inner;

	if (irq_done_status & CAMSVCENTRAL_SW_GP_PASS1_DONE_0_ST)
		irq_info.done_tags |= sv_dev->active_group_info[0];
	if (irq_done_status & CAMSVCENTRAL_SW_GP_PASS1_DONE_1_ST)
		irq_info.done_tags |= sv_dev->active_group_info[1];
	if (irq_done_status & CAMSVCENTRAL_SW_GP_PASS1_DONE_2_ST)
		irq_info.done_tags |= sv_dev->active_group_info[2];
	if (irq_done_status & CAMSVCENTRAL_SW_GP_PASS1_DONE_3_ST)
		irq_info.done_tags |= sv_dev->active_group_info[3];
	irq_info.irq_type |= (1 << CAMSYS_IRQ_FRAME_DONE);
	irq_flag = irq_info.irq_type;
	if (irq_flag && push_msgfifo(sv_dev, &irq_info) == 0)
		wake_thread = 1;

	return wake_thread ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static irqreturn_t mtk_irq_camsv_sof(int irq, void *data)
{
	struct mtk_camsv_device *sv_dev = (struct mtk_camsv_device *)data;
	struct mtk_camsys_irq_info irq_info;
	unsigned int dequeued_imgo_seq_no, dequeued_imgo_seq_no_inner;
	unsigned int tg_cnt;
	unsigned int irq_sof_status;
	unsigned int irq_flag = 0;
	bool wake_thread = 0;
	unsigned int i;

	memset(&irq_info, 0, sizeof(irq_info));
	dequeued_imgo_seq_no =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_FRAME_SEQ_NO);
	dequeued_imgo_seq_no_inner =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FRAME_SEQ_NO);
	irq_sof_status	=
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_SOF_STATUS);
	for (i = 0; i < MAX_SV_HW_GROUPS; i++) {
		sv_dev->active_group_info[i] =
			readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_GROUP_TAG0 +
				REG_CAMSVCENTRAL_GROUP_TAG_SHIFT * i);
	}
	sv_dev->enque_tags =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_ENQUE_TAGS);
	sv_dev->first_tag =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FIRST_TAG);
	sv_dev->last_tag =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_LAST_TAG);
	tg_cnt =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_VF_ST_TAG1 +
				CAMSVCENTRAL_VF_ST_TAG_SHIFT * 3);
	tg_cnt = (sv_dev->tg_cnt & 0xffffff00) + ((tg_cnt & 0xff000000) >> 24);
	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(sv_dev->dev, "camsv-%d: sof status:0x%x seq_no:%d_%d group_tags:0x%x_%x_%x_%x enque_tags:0x%x first_tag:0x%x last_tag:0x%x VF_ST_TAG4:%d",
		sv_dev->id, irq_sof_status,
		dequeued_imgo_seq_no_inner, dequeued_imgo_seq_no,
		sv_dev->active_group_info[0],
		sv_dev->active_group_info[1],
		sv_dev->active_group_info[2],
		sv_dev->active_group_info[3],
		sv_dev->enque_tags,
		sv_dev->first_tag,
		sv_dev->last_tag,
		tg_cnt);

	irq_info.irq_type = 0;
	irq_info.ts_ns = ktime_get_boottime_ns();
	irq_info.frame_idx = dequeued_imgo_seq_no;
	irq_info.frame_idx_inner = dequeued_imgo_seq_no_inner;
	irq_info.fbc_empty = ffs(sv_dev->last_tag) ?
		mtk_cam_sv_is_zero_fbc_cnt(sv_dev, ffs(sv_dev->last_tag) - 1) : 0;

	if (irq_sof_status & CAMSVCENTRAL_SOF_ST_TAG1)
		irq_info.sof_tags |= (1 << 0);
	if (irq_sof_status & CAMSVCENTRAL_SOF_ST_TAG2)
		irq_info.sof_tags |= (1 << 1);
	if (irq_sof_status & CAMSVCENTRAL_SOF_ST_TAG3)
		irq_info.sof_tags |= (1 << 2);
	if (irq_sof_status & CAMSVCENTRAL_SOF_ST_TAG4)
		irq_info.sof_tags |= (1 << 3);
	if (irq_sof_status & CAMSVCENTRAL_SOF_ST_TAG5)
		irq_info.sof_tags |= (1 << 4);
	if (irq_sof_status & CAMSVCENTRAL_SOF_ST_TAG6)
		irq_info.sof_tags |= (1 << 5);
	if (irq_sof_status & CAMSVCENTRAL_SOF_ST_TAG7)
		irq_info.sof_tags |= (1 << 6);
	if (irq_sof_status & CAMSVCENTRAL_SOF_ST_TAG8)
		irq_info.sof_tags |= (1 << 7);
	irq_info.irq_type |= (1 << CAMSYS_IRQ_FRAME_START);
	sv_dev->sof_count++;
	if (tg_cnt < sv_dev->tg_cnt)
		sv_dev->tg_cnt = tg_cnt + BIT(8);
	else
		sv_dev->tg_cnt = tg_cnt;
	sv_dev->sof_timestamp = ktime_get_boottime_ns();
	irq_flag = irq_info.irq_type;

	if (irq_info.sof_tags & sv_dev->first_tag)
		engine_handle_sof(&sv_dev->cq_ref, irq_info.frame_idx_inner);

	if (irq_flag && push_msgfifo(sv_dev, &irq_info) == 0)
		wake_thread = 1;

	return wake_thread ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static irqreturn_t mtk_irq_camsv_err(int irq, void *data)
{
	struct mtk_camsv_device *sv_dev = (struct mtk_camsv_device *)data;
	struct mtk_camsys_irq_info irq_info;
	unsigned int dequeued_imgo_seq_no, dequeued_imgo_seq_no_inner;
	unsigned int err_status;
	bool wake_thread = 0;

	dequeued_imgo_seq_no =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_FRAME_SEQ_NO);
	dequeued_imgo_seq_no_inner =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FRAME_SEQ_NO);
	err_status =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_ERR_STATUS);

	dev_dbg(sv_dev->dev, "camsv-%d: error status:0x%x seq_no:%d_%d",
		sv_dev->id, err_status,
		dequeued_imgo_seq_no_inner, dequeued_imgo_seq_no);

	irq_info.irq_type = 0;
	irq_info.ts_ns = ktime_get_boottime_ns();
	irq_info.frame_idx = dequeued_imgo_seq_no;
	irq_info.frame_idx_inner = dequeued_imgo_seq_no_inner;

	if (err_status) {
		struct mtk_camsys_irq_info err_info;
		memset(&err_info, 0, sizeof(err_info));
		if (err_status & ERR_ST_MASK_TAG1_ERR)
			err_info.err_tags |= (1 << 0);
		if (err_status & ERR_ST_MASK_TAG2_ERR)
			err_info.err_tags |= (1 << 1);
		if (err_status & ERR_ST_MASK_TAG3_ERR)
			err_info.err_tags |= (1 << 2);
		if (err_status & ERR_ST_MASK_TAG4_ERR)
			err_info.err_tags |= (1 << 3);
		if (err_status & ERR_ST_MASK_TAG5_ERR)
			err_info.err_tags |= (1 << 4);
		if (err_status & ERR_ST_MASK_TAG6_ERR)
			err_info.err_tags |= (1 << 5);
		if (err_status & ERR_ST_MASK_TAG7_ERR)
			err_info.err_tags |= (1 << 6);
		if (err_status & ERR_ST_MASK_TAG8_ERR)
			err_info.err_tags |= (1 << 7);
		err_info.irq_type = 1 << CAMSYS_IRQ_ERROR;
		err_info.ts_ns = irq_info.ts_ns;
		err_info.frame_idx = irq_info.frame_idx;
		err_info.frame_idx_inner = irq_info.frame_idx_inner;
		err_info.e.err_status = err_status;

		if (push_msgfifo(sv_dev, &err_info) == 0)
			wake_thread = 1;
	}

	return wake_thread ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static irqreturn_t mtk_irq_camsv_cq_done(int irq, void *data)
{
	struct mtk_camsv_device *sv_dev = (struct mtk_camsv_device *)data;
	struct mtk_camsys_irq_info irq_info;
	unsigned int dequeued_imgo_seq_no, dequeued_imgo_seq_no_inner;
	unsigned int cq_done_status;
	unsigned int irq_flag = 0;
	bool wake_thread = 0;

	dequeued_imgo_seq_no =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_FRAME_SEQ_NO);
	dequeued_imgo_seq_no_inner =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FRAME_SEQ_NO);
	cq_done_status =
		readl_relaxed(sv_dev->base_scq + REG_CAMSVCQTOP_INT_0_STATUS);

	dev_dbg(sv_dev->dev, "camsv-%d: cq done status:0x%x seq_no:%d_%d",
		sv_dev->id, cq_done_status,
		dequeued_imgo_seq_no_inner, dequeued_imgo_seq_no);

	irq_info.irq_type = 0;
	irq_info.ts_ns = ktime_get_boottime_ns();
	irq_info.frame_idx = dequeued_imgo_seq_no;
	irq_info.frame_idx_inner = dequeued_imgo_seq_no_inner;

	if (cq_done_status & CAMSVCQTOP_SCQ_SUB_THR_DONE) {

		if (engine_handle_cq_done(&sv_dev->cq_ref))
			irq_info.irq_type |= 1 << CAMSYS_IRQ_SETTING_DONE;
	}

	irq_flag = irq_info.irq_type;
	if (irq_flag && push_msgfifo(sv_dev, &irq_info) == 0)
		wake_thread = 1;

	return wake_thread ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static irqreturn_t mtk_thread_irq_camsv(int irq, void *data)
{
	struct mtk_camsv_device *sv_dev = (struct mtk_camsv_device *)data;
	struct mtk_camsys_irq_info irq_info;
	int recovered_done;
	int do_recover;

	if (unlikely(atomic_cmpxchg(&sv_dev->is_fifo_overflow, 1, 0)))
		dev_info(sv_dev->dev, "msg fifo overflow\n");

	while (kfifo_len(&sv_dev->msg_fifo) >= sizeof(irq_info)) {
		int len = kfifo_out(&sv_dev->msg_fifo, &irq_info, sizeof(irq_info));

		WARN_ON(len != sizeof(irq_info));
#ifdef CAMSV_DEBUG
		dev_info(sv_dev->dev, "ts=%llu irq_type %d, req:0x%x/0x%x\n",
			irq_info.ts_ns / 1000,
			irq_info.irq_type,
			irq_info.frame_idx_inner,
			irq_info.frame_idx);
#endif
		/* error case */
		if (unlikely(irq_info.irq_type == (1 << CAMSYS_IRQ_ERROR))) {
			camsv_handle_err(sv_dev, &irq_info);
			continue;
		}

		/* normal case */
		do_recover = sv_process_fsm(sv_dev, &irq_info,
					    &recovered_done);

		/* inform interrupt information to camsys controller */
		mtk_cam_ctrl_isr_event(sv_dev->cam,
				       CAMSYS_ENGINE_CAMSV, sv_dev->id,
				       &irq_info);

		if (do_recover) {
			irq_info.irq_type = BIT(CAMSYS_IRQ_FRAME_DONE);
			irq_info.cookie_done = recovered_done;

			mtk_cam_ctrl_isr_event(sv_dev->cam,
					       CAMSYS_ENGINE_CAMSV, sv_dev->id,
					       &irq_info);
		}
	}

	return IRQ_HANDLED;
}

static int mtk_camsv_pm_suspend(struct device *dev)
{
	struct mtk_camsv_device *sv_dev = dev_get_drvdata(dev);
	u32 val;
	int ret;

	dev_dbg(dev, "- %s\n", __func__);

	if (pm_runtime_suspended(dev))
		return 0;

	/* Disable ISP's view finder and wait for TG idle */
	dev_info(dev, "camsv suspend, disable VF\n");
	val = readl(sv_dev->base + REG_CAMSVCENTRAL_VF_CON);
	writel(val & (~CAMSVCENTRAL_VF_CON_VFDATA_EN),
		sv_dev->base + REG_CAMSVCENTRAL_VF_CON);
#ifdef CAMSV_TODO
	// camsv todo: implement this usage
	ret = readl_poll_timeout_atomic(
					sv_dev->base + REG_CAMSV_TG_INTER_ST, val,
					(val & CAMSV_TG_CS_MASK) == CAMSV_TG_IDLE_ST,
					USEC_PER_MSEC, MTK_CAMSV_STOP_HW_TIMEOUT);
	if (ret)
		dev_info(dev, "can't stop HW:%d:0x%x\n", ret, val);
#endif

	/* Disable CMOS */
	val = readl(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE);
	writel(val & (~CAMSVCENTRAL_SEN_MODE_CMOS_EN),
		sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE);

	/* Force ISP HW to idle */
	ret = pm_runtime_put_sync(dev);
	return ret;
}

static int mtk_camsv_pm_resume(struct device *dev)
{
	struct mtk_camsv_device *sv_dev = dev_get_drvdata(dev);
	u32 val;
	int ret;

	dev_dbg(dev, "- %s\n", __func__);

	if (pm_runtime_suspended(dev))
		return 0;

	/* Force ISP HW to resume */
	ret = pm_runtime_get_sync(dev);
	if (ret)
		return ret;

	/* Enable CMOS */
	dev_info(dev, "camsv resume, enable CMOS/VF\n");
	val = readl(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE);
	writel(val | CAMSVCENTRAL_SEN_MODE_CMOS_EN,
		sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE);

	/* Enable VF */
	val = readl(sv_dev->base + REG_CAMSVCENTRAL_VF_CON);
	writel(val | CAMSVCENTRAL_VF_CON_VFDATA_EN,
		sv_dev->base + REG_CAMSVCENTRAL_VF_CON);

	return 0;
}

static int mtk_camsv_suspend_pm_event(struct notifier_block *notifier,
			unsigned long pm_event, void *unused)
{
	struct mtk_camsv_device *sv_dev =
		container_of(notifier, struct mtk_camsv_device, notifier_blk);
	struct device *dev = sv_dev->dev;

	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:
		return NOTIFY_DONE;
	case PM_RESTORE_PREPARE:
		return NOTIFY_DONE;
	case PM_POST_HIBERNATION:
		return NOTIFY_DONE;
	case PM_SUSPEND_PREPARE: /* before enter suspend */
		mtk_camsv_pm_suspend(dev);
		return NOTIFY_DONE;
	case PM_POST_SUSPEND: /* after resume */
		mtk_camsv_pm_resume(dev);
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static int mtk_camsv_of_probe(struct platform_device *pdev,
			    struct mtk_camsv_device *sv_dev)
{
	struct device *dev = &pdev->dev;
	struct platform_device *larb_pdev;
	struct device_node *larb_node;
	struct device_link *link;
	struct resource *res;
	unsigned int i;
	int clks, larbs, ret;

	ret = of_property_read_u32(dev->of_node, "mediatek,camsv-id",
						       &sv_dev->id);
	if (ret) {
		dev_dbg(dev, "missing camid property\n");
		return ret;
	}

	ret = of_property_read_u32(dev->of_node, "mediatek,cammux-id",
						       &sv_dev->cammux_id);
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

	sv_dev->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(sv_dev->base)) {
		dev_dbg(dev, "failed to map register base\n");
		return PTR_ERR(sv_dev->base);
	}
	dev_dbg(dev, "camsv, map_addr=0x%pK\n", sv_dev->base);

	/* base dma outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base_DMA");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	sv_dev->base_dma = devm_ioremap_resource(dev, res);
	if (IS_ERR(sv_dev->base_dma)) {
		dev_dbg(dev, "failed to map register base dma\n");
		return PTR_ERR(sv_dev->base_dma);
	}
	dev_dbg(dev, "camsv, map_dma_addr=0x%pK\n", sv_dev->base_dma);

	/* base scq outer register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base_SCQ");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	sv_dev->base_scq = devm_ioremap_resource(dev, res);
	if (IS_ERR(sv_dev->base_scq)) {
		dev_dbg(dev, "failed to map register base scq\n");
		return PTR_ERR(sv_dev->base_scq);
	}
	dev_dbg(dev, "camsv, map_scq_addr=0x%pK\n", sv_dev->base_scq);

	/* base inner register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	sv_dev->base_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(sv_dev->base_inner)) {
		dev_dbg(dev, "failed to map register inner base\n");
		return PTR_ERR(sv_dev->base_inner);
	}

	dev_dbg(dev, "camsv, map_addr=0x%pK\n", sv_dev->base_inner);

	/* base inner dma register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base_DMA");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	sv_dev->base_inner_dma = devm_ioremap_resource(dev, res);
	if (IS_ERR(sv_dev->base_inner_dma)) {
		dev_dbg(dev, "failed to map register inner base dma\n");
		return PTR_ERR(sv_dev->base_inner_dma);
	}
	dev_dbg(dev, "camsv, map_addr(inner dma)=0x%pK\n", sv_dev->base_inner_dma);

	/* base inner scq register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base_SCQ");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	sv_dev->base_inner_scq = devm_ioremap_resource(dev, res);
	if (IS_ERR(sv_dev->base_inner_scq)) {
		dev_dbg(dev, "failed to map register inner base\n");
		return PTR_ERR(sv_dev->base_inner_scq);
	}
	dev_dbg(dev, "camsv, map_addr(inner scq)=0x%pK\n", sv_dev->base_inner_scq);

	for (i = 0; i < CAMSV_IRQ_NUM; i++) {
		sv_dev->irq[i] = platform_get_irq(pdev, i);
		if (!sv_dev->irq[i]) {
			dev_dbg(dev, "failed to get irq\n");
			return -ENODEV;
		}
	}

	for (i = 0; i < CAMSV_IRQ_NUM; i++) {
		sv_dev->irq[i] = platform_get_irq(pdev, i);
		if (!sv_dev->irq[i]) {
			dev_dbg(dev, "failed to get irq\n");
			return -ENODEV;
		}
	}

	for (i = 0; i < CAMSV_IRQ_NUM; i++) {
		if (i == 0)
			ret = devm_request_threaded_irq(dev, sv_dev->irq[i],
						mtk_irq_camsv_done,
						mtk_thread_irq_camsv,
						0, dev_name(dev), sv_dev);
		else if (i == 1)
			ret = devm_request_threaded_irq(dev, sv_dev->irq[i],
						mtk_irq_camsv_err,
						mtk_thread_irq_camsv,
						0, dev_name(dev), sv_dev);
		else if (i == 2)
			ret = devm_request_threaded_irq(dev, sv_dev->irq[i],
						mtk_irq_camsv_sof,
						mtk_thread_irq_camsv,
						0, dev_name(dev), sv_dev);
		else
			ret = devm_request_threaded_irq(dev, sv_dev->irq[i],
						mtk_irq_camsv_cq_done,
						mtk_thread_irq_camsv,
						0, dev_name(dev), sv_dev);
		if (ret) {
			dev_dbg(dev, "failed to request irq=%d\n", sv_dev->irq[i]);
			return ret;
		}

		dev_info(dev, "registered irq=%d\n", sv_dev->irq[i]);

		disable_irq(sv_dev->irq[i]);

		dev_info(dev, "%s:disable irq %d\n", __func__, sv_dev->irq[i]);
	}

	clks  = of_count_phandle_with_args(pdev->dev.of_node, "clocks",
			"#clock-cells");

	sv_dev->num_clks = (clks == -ENOENT) ? 0:clks;
	dev_info(dev, "clk_num:%d\n", sv_dev->num_clks);

	if (sv_dev->num_clks) {
		sv_dev->clks = devm_kcalloc(dev, sv_dev->num_clks, sizeof(*sv_dev->clks),
					 GFP_KERNEL);
		if (!sv_dev->clks)
			return -ENOMEM;
	}

	for (i = 0; i < sv_dev->num_clks; i++) {
		sv_dev->clks[i] = of_clk_get(pdev->dev.of_node, i);
		if (IS_ERR(sv_dev->clks[i])) {
			dev_info(dev, "failed to get clk %d\n", i);
			return -ENODEV;
		}
	}

	larbs = of_count_phandle_with_args(
					pdev->dev.of_node, "mediatek,larbs", NULL);
	larbs = (larbs == -ENOENT) ? 0:larbs;
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

	sv_dev->notifier_blk.notifier_call = mtk_camsv_suspend_pm_event;
	sv_dev->notifier_blk.priority = 0;
	ret = register_pm_notifier(&sv_dev->notifier_blk);
	if (ret) {
		dev_info(dev, "Failed to register PM notifier");
		return -ENODEV;
	}

	return 0;
}

static int mtk_camsv_component_bind(
	struct device *dev,
	struct device *master,
	void *data)
{
	struct mtk_camsv_device *sv_dev = dev_get_drvdata(dev);
	struct mtk_cam_device *cam_dev = data;

	sv_dev->cam = cam_dev;
	return mtk_cam_set_dev_sv(cam_dev->dev, sv_dev->id, dev);
}

static void mtk_camsv_component_unbind(
	struct device *dev,
	struct device *master,
	void *data)
{
}

static const struct component_ops mtk_camsv_component_ops = {
	.bind = mtk_camsv_component_bind,
	.unbind = mtk_camsv_component_unbind,
};

static int mtk_camsv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_camsv_device *sv_dev;
	int ret;

	sv_dev = devm_kzalloc(dev, sizeof(*sv_dev), GFP_KERNEL);
	if (!sv_dev)
		return -ENOMEM;

	sv_dev->dev = dev;
	dev_set_drvdata(dev, sv_dev);

	ret = mtk_camsv_of_probe(pdev, sv_dev);
	if (ret)
		return ret;

	sv_dev->fifo_size =
		roundup_pow_of_two(8 * sizeof(struct mtk_camsys_irq_info));
	sv_dev->msg_buffer = devm_kzalloc(dev, sv_dev->fifo_size,
					     GFP_KERNEL);
	if (!sv_dev->msg_buffer)
		return -ENOMEM;

	pm_runtime_enable(dev);

	return component_add(dev, &mtk_camsv_component_ops);
}

static int mtk_camsv_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pm_runtime_disable(dev);

	component_del(dev, &mtk_camsv_component_ops);
	return 0;
}

static int mtk_camsv_runtime_suspend(struct device *dev)
{
	struct mtk_camsv_device *sv_dev = dev_get_drvdata(dev);
	int i;

	dev_dbg(dev, "%s:disable clock\n", __func__);

	for (i = 0; i < sv_dev->num_clks; i++)
		clk_disable_unprepare(sv_dev->clks[i]);

	return 0;
}

static int mtk_camsv_runtime_resume(struct device *dev)
{
	struct mtk_camsv_device *sv_dev = dev_get_drvdata(dev);
	int i, ret;

	/* reset_msgfifo before enable_irq */
	ret = reset_msgfifo(sv_dev);
	if (ret)
		return ret;

	dev_dbg(dev, "%s:enable clock\n", __func__);
	for (i = 0; i < sv_dev->num_clks; i++) {
		ret = clk_prepare_enable(sv_dev->clks[i]);
		if (ret) {
			dev_info(dev, "enable failed at clk #%d, ret = %d\n",
				 i, ret);
			i--;
			while (i >= 0)
				clk_disable_unprepare(sv_dev->clks[i--]);

			return ret;
		}
	}
	sv_reset_by_camsys_top(sv_dev);

	for (i = 0; i < CAMSV_IRQ_NUM; i++) {
		enable_irq(sv_dev->irq[i]);
		dev_dbg(dev, "%s:enable irq %d\n", __func__, sv_dev->irq[i]);
	}


	dev_info(dev, "%s:enable irq\n", __func__);

	return 0;
}

static const struct dev_pm_ops mtk_camsv_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_camsv_runtime_suspend, mtk_camsv_runtime_resume,
			   NULL)
};

struct platform_driver mtk_cam_sv_driver = {
	.probe   = mtk_camsv_probe,
	.remove  = mtk_camsv_remove,
	.driver  = {
		.name  = "mtk-cam camsv",
		.of_match_table = of_match_ptr(mtk_camsv_of_ids),
		.pm     = &mtk_camsv_pm_ops,
	}
};

