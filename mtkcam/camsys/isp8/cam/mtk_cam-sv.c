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

#include <soc/mediatek/smi.h>
#include <soc/mediatek/mmdvfs_v3.h>
#include <soc/mediatek/emi.h>

#include "mtk_cam.h"
#include "mtk_cam-sv-regs.h"
#include "mtk_cam-sv.h"
#include "mtk_cam-fmt_utils.h"
#include "mtk_cam-trace.h"
#include "mtk_cam-hsf.h"

#include "mmqos-mtk.h"
#include "iommu_debug.h"
#include "mtk-mmdvfs-debug.h"

#define MTK_CAMSV_STOP_HW_TIMEOUT			(33 * USEC_PER_MSEC)
#define CAMSV_DEBUG 0

static int debug_cam_sv;
module_param(debug_cam_sv, int, 0644);

static int urgent_high;
module_param(urgent_high, int, 0644);
static int urgent_low;
module_param(urgent_low, int, 0644);
static int ultra_high;
module_param(ultra_high, int, 0644);
static int ultra_low;
module_param(ultra_low, int, 0644);
static int pultra_high;
module_param(pultra_high, int, 0644);
static int pultra_low;
module_param(pultra_low, int, 0644);

static int camsv_fifo_detect;
module_param(camsv_fifo_detect, int, 0644);

static int debug_ddren_camsv_hw_mode;
module_param(debug_ddren_camsv_hw_mode, int, 0644);
MODULE_PARM_DESC(debug_ddren_camsv_hw_mode, "debug: 1 : active camsv hw mode");

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
		.tag_idx = SVTAG_1,
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
		.tag_idx = SVTAG_1,
		.seninf_padidx = PAD_SRC_RAW1,
		.tag_order = MTKCAM_IPI_ORDER_LAST_TAG,
		.is_w = false,
	},
	{
		.tag_idx = SVTAG_2,
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
		.tag_idx = SVTAG_2,
		.seninf_padidx = PAD_SRC_RAW1,
		.tag_order = MTKCAM_IPI_ORDER_NORMAL_TAG,
		.is_w = false,
	},
	{
		.tag_idx = SVTAG_1,
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

static const struct mtk_camsv_tag_param sv_tag_param_non_comb_ic[4] = {
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
		.seninf_padidx = PAD_SRC_RAW2,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = false,
	},
	{
		.tag_idx = SVTAG_3,
		.seninf_padidx = PAD_SRC_RAW3,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = false,
	},
};
static const struct mtk_camsv_tag_param sv_tag_param_extisp[1] = {
	{
		.tag_idx = SVTAG_3,
		.seninf_padidx = PAD_SRC_GENERAL0,
		.tag_order = MTKCAM_IPI_ORDER_FIRST_TAG,
		.is_w = false,
	},
};

static int sv_process_fsm(struct mtk_camsv_device *sv_dev,
			  struct mtk_camsys_irq_info *irq_info,
			  int *recovered_done)
{
	struct engine_fsm *fsm = &sv_dev->fsm;
	unsigned int i, done_type, sof_type, drop_type, done_tags = 0;
	int recovered = 0;

	sof_type = irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_START);
	if (sof_type) {
		/* when first tag comes: 1. reset drop tags 2. update used_tags */
		if (irq_info->sof_tags & sv_dev->first_tag) {
			sv_dev->drop_tags = 0;
			sv_dev->used_tags = 0;
			for (i = 0; i < MAX_SV_HW_GROUPS; i++)
				sv_dev->used_tags |= sv_dev->active_group_info[i];
		}
	}

	drop_type = irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_DROP);
	if (drop_type) {
		irq_info->irq_type &= ~drop_type;
		sv_dev->drop_tags |= irq_info->done_tags;
	}

	done_type = irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_DONE);
	if (done_type) {
		irq_info->irq_type &= ~done_type;
		done_tags = irq_info->done_tags;
	}

	if (drop_type || done_type) {
		int cookie_done;
		int ret;

		sv_dev->handled_tags |= done_tags;
		sv_dev->handled_tags |= sv_dev->drop_tags;
		sv_dev->handled_tags &= sv_dev->used_tags;

		if (sv_dev->handled_tags == sv_dev->used_tags) {
			ret = engine_fsm_hw_done(fsm, &cookie_done);
			if (ret > 0) {
				irq_info->irq_type |= BIT(CAMSYS_IRQ_FRAME_DONE);
				irq_info->cookie_done = cookie_done;
				sv_dev->handled_tags = 0;
			} else {
				/* handle for fake p1 done */
				dev_dbg(sv_dev->dev, "warn: fake done in/out: 0x%x 0x%x\n",
							 irq_info->frame_idx_inner,
							 irq_info->frame_idx);
				irq_info->cookie_done = 0;
				sv_dev->handled_tags = 0;
			}
		} else {
			dev_dbg(sv_dev->dev, "%s: not all group done yet: in/out: 0x%x 0x%x tag used/handled: 0x%x 0x%x\n",
				__func__,
				irq_info->frame_idx_inner,
				irq_info->frame_idx,
				sv_dev->used_tags,
				sv_dev->handled_tags);
			irq_info->cookie_done = 0;
		}
	}

	sof_type = irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_START);
	if (sof_type) {
		irq_info->irq_type &= ~sof_type;

		/* when first tag comes: update irq_type */
		if (irq_info->sof_tags & sv_dev->first_tag)
			irq_info->irq_type |= BIT(CAMSYS_IRQ_FRAME_START_DCIF_MAIN);

		/* when last tag comes: 1. update irq_type 2. update fsm */
		if (irq_info->sof_tags & sv_dev->last_tag) {
			irq_info->irq_type |= BIT(CAMSYS_IRQ_FRAME_START);
			for (i = 0; i < MAX_SV_HW_GROUPS; i++) {
				if (sv_dev->last_tag & sv_dev->active_group_info[i]) {

					recovered =
						engine_fsm_sof(&sv_dev->fsm,
							       irq_info->frame_idx_inner,
							       irq_info->frame_idx,
							       irq_info->fbc_empty,
							       recovered_done);
					break;
				}
			}
		}
	}

	if (recovered) {
		dev_info(sv_dev->dev, "recovered done 0x%x in/out: 0x%x 0x%x\n",
			 *recovered_done,
			 irq_info->frame_idx_inner,
			 irq_info->frame_idx);
		writel(0xDEADBEEF, sv_dev->base + REG_CAMSVCENTRAL_CAMSV_SPARE0);
		dev_info(sv_dev->dev, "camsv spare reigister WR test write 0xDEADBEEF, read 0x%x",
			readl(sv_dev->base + REG_CAMSVCENTRAL_CAMSV_SPARE0));
		if (sv_dev->debug_use_mraw_out_base && sv_dev->debug_use_mraw_in_base) {
			dev_info(sv_dev->dev, "mraw frame_no in/out 0x%x_0x%x\n",
				readl(sv_dev->debug_use_mraw_in_base + REG_MRAW_FHG_SPARE3),
				readl(sv_dev->debug_use_mraw_out_base + REG_MRAW_FHG_SPARE3));
		}
	}
	return recovered;
}

int mtk_camsv_translation_fault_callback(int port, dma_addr_t mva, void *data)
{
	int index;
	struct mtk_camsv_device *sv_dev = (struct mtk_camsv_device *)data;
	unsigned int first_tag, tag_idx;
	unsigned int frame_idx_inner;

	first_tag = readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FIRST_TAG);

	if (first_tag == 0) {
		dev_info(sv_dev->dev, "abnormal tf callback first_tag=0\n");
		return 0;
	}

	tag_idx = ffs(first_tag) - 1;
	frame_idx_inner = readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FH_SPARE_TAG_1 +
			tag_idx * CAMSVCENTRAL_FH_SPARE_SHIFT);
	dev_info_ratelimited(sv_dev->dev, "tg_sen_mode:0x%x tg_vf_con:0x%x tg_path_cfg:0x%x",
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_SEN_MODE),
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_VF_CON),
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_PATH_CFG));

	for (index = 0; index < CAMSV_MAX_TAGS; index++) {
		dev_info_ratelimited(sv_dev->dev, "tag:%d seq_no:%d_%d tg_grab_pxl:0x%x tg_grab_lin:0x%x fmt:0x%x imgo_fbc0: 0x%x imgo_fbc1: 0x%x",
		index,
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FH_SPARE_TAG_1 +
			index * CAMSVCENTRAL_FH_SPARE_SHIFT),
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_FH_SPARE_TAG_1 +
			index * CAMSVCENTRAL_FH_SPARE_SHIFT),
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

	for (index = 0; index < CAMSV_MAX_TAGS; index++) {
		dev_info_ratelimited(sv_dev->dev, "tag:%d imgo_stride_img:0x%x imgo_addr_img:0x%x_%x",
			index,
			readl_relaxed(sv_dev->base_dma_inner +
				REG_CAMSVDMATOP_WDMA_BASIC_IMG1 +
				index * CAMSVDMATOP_WDMA_BASIC_IMG_SHIFT),
			readl_relaxed(sv_dev->base_dma_inner +
				REG_CAMSVDMATOP_WDMA_BASE_ADDR_IMG1 +
				index * CAMSVDMATOP_WDMA_BASE_ADDR_IMG_SHIFT),
			readl_relaxed(sv_dev->base_dma_inner +
				REG_CAMSVDMATOP_WDMA_BASE_ADDR_MSB_IMG1 +
				index * CAMSVDMATOP_WDMA_BASE_ADDR_MSB_IMG_SHIFT));
	}
	mtk_cam_ctrl_dump_request(sv_dev->cam, CAMSYS_ENGINE_CAMSV, sv_dev->id,
		frame_idx_inner, MSG_M4U_TF);
	return 0;
}

void mtk_cam_sv_set_queue_mode(struct mtk_camsv_device *sv_dev, bool enable)
{
	if (enable)
		atomic_set(&sv_dev->is_queue_mode, 1);
	else
		atomic_set(&sv_dev->is_queue_mode, 0);
}

void mtk_cam_sv_backup(struct mtk_camsv_device *sv_dev)
{
	struct mtk_camsv_backup_setting *s = &sv_dev->backup_setting;
	int i;

	s->done_status_en = CAMSV_READ_REG(sv_dev->base +
					   REG_CAMSVCENTRAL_DONE_STATUS_EN);
	s->err_status_en = CAMSV_READ_REG(sv_dev->base +
					  REG_CAMSVCENTRAL_ERR_STATUS_EN);
	s->sof_status_en = CAMSV_READ_REG(sv_dev->base +
					  REG_CAMSVCENTRAL_SOF_STATUS_EN);
	s->channel_status_en = CAMSV_READ_REG(sv_dev->base +
					  REG_CAMSVCENTRAL_CHANNEL_STATUS_EN);
	s->common_status_en = CAMSV_READ_REG(sv_dev->base +
					  REG_CAMSVCENTRAL_COMMON_STATUS_EN);

	for (i = SVTAG_START; i < SVTAG_END; i++) {
		s->grab_pxl[i] = CAMSV_READ_REG(sv_dev->base_inner +
				       REG_CAMSVCENTRAL_GRAB_PXL_TAG1 +
				       CAMSVCENTRAL_GRAB_PXL_TAG_SHIFT * i);
		s->grab_lin[i] = CAMSV_READ_REG(sv_dev->base_inner +
				       REG_CAMSVCENTRAL_GRAB_LIN_TAG1 +
				       CAMSVCENTRAL_GRAB_LIN_TAG_SHIFT * i);
		s->fbc0[i] = CAMSV_READ_REG(sv_dev->base +
				       REG_CAMSVCENTRAL_FBC0_TAG1 +
				       CAMSVCENTRAL_FBC0_TAG_SHIFT * i);
		s->exp0[i] = CAMSV_READ_REG(sv_dev->base +
				       REG_CAMSVCENTRAL_INT_EXP0_TAG1 +
				       CAMSVCENTRAL_INT_EXP0_OFFSET * i);
		s->exp1[i] = CAMSV_READ_REG(sv_dev->base +
				       REG_CAMSVCENTRAL_INT_EXP1_TAG1 +
				       CAMSVCENTRAL_INT_EXP1_OFFSET * i);
	}

	s->dma_en_img = CAMSV_READ_REG(sv_dev->base +
				       REG_CAMSVCENTRAL_DMA_EN_IMG);
	s->dcif_set = CAMSV_READ_REG(sv_dev->base +
				     REG_CAMSVCENTRAL_DCIF_SET);
	s->dcif_sel = CAMSV_READ_REG(sv_dev->base +
				     REG_CAMSVCENTRAL_DCIF_SEL);
}

void mtk_cam_sv_restore(struct mtk_camsv_device *sv_dev)
{
	struct mtk_camsv_backup_setting *s = &sv_dev->backup_setting;
	int i;

	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DONE_STATUS_EN,
			s->done_status_en);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_ERR_STATUS_EN,
			s->err_status_en);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_SOF_STATUS_EN,
			s->sof_status_en);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_CHANNEL_STATUS_EN,
			s->channel_status_en);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_COMMON_STATUS_EN,
			s->common_status_en);

	for (i = SVTAG_START; i < SVTAG_END; i++) {
		CAMSV_WRITE_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_GRAB_PXL_TAG1 +
			CAMSVCENTRAL_GRAB_PXL_TAG_SHIFT * i, s->grab_pxl[i]);
		CAMSV_WRITE_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_GRAB_LIN_TAG1 +
			CAMSVCENTRAL_GRAB_LIN_TAG_SHIFT * i, s->grab_lin[i]);
		CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_FBC0_TAG1 +
			CAMSVCENTRAL_FBC0_TAG_SHIFT * i, s->fbc0[i]);
		CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_INT_EXP0_TAG1 +
			CAMSVCENTRAL_INT_EXP0_OFFSET * i, s->exp0[i]);
		CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_INT_EXP1_TAG1 +
			CAMSVCENTRAL_INT_EXP1_OFFSET * i, s->exp1[i]);
	}

	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DMA_EN_IMG, s->dma_en_img);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DCIF_SET, s->dcif_set);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DCIF_SEL, s->dcif_sel);
}


void mtk_cam_sv_exp_setup(struct mtk_camsv_device *sv_dev, int exp0, int exp1)
{
	unsigned int first_tag, tag_idx, grab_pxl, grab_lin;
	unsigned int x, y, w, h, value;

	/* skip unlock raw done sel for first frame */
	sv_dev->is_skip_raw_unlock_done = true;

	first_tag =
		CAMSV_READ_REG(sv_dev->base + REG_CAMSVCENTRAL_FIRST_TAG);
	if (first_tag)
		tag_idx = ffs(first_tag) - 1;
	else {
		dev_info(sv_dev->dev, "%s camsv_id:%d - first_tag shall not be zero\n",
			__func__, sv_dev->id);
		goto EXIT;
	}

	grab_pxl =
		CAMSV_READ_REG(sv_dev->base +
			REG_CAMSVCENTRAL_GRAB_PXL_TAG1 +
			(tag_idx * CAMSVCENTRAL_GRAB_PXL_TAG_SHIFT));
	grab_lin =
		CAMSV_READ_REG(sv_dev->base +
			REG_CAMSVCENTRAL_GRAB_LIN_TAG1 +
			(tag_idx * CAMSVCENTRAL_GRAB_LIN_TAG_SHIFT));
	x = grab_pxl & 0xFFFF;
	y = grab_lin & 0xFFFF;
	w = (grab_pxl >> 16) - x;
	h = (grab_lin >> 16) - y;

	if (w + x <= 16) {
		dev_info(sv_dev->dev, "%s camsv_id:%d - width too small:%d_%d\n",
			__func__, sv_dev->id, x, w);
		goto EXIT;
	}

	/* setup exp0 */
	value = (ALIGN(w + x - 16, 16) << 16) | (exp0 + y);
	CAMSV_WRITE_REG(sv_dev->base +
		REG_CAMSVCENTRAL_INT_EXP0_TAG1 +
		(CAMSVCENTRAL_INT_EXP0_OFFSET * tag_idx),
		value);
	CAMSV_WRITE_REG(sv_dev->base_inner +
		REG_CAMSVCENTRAL_INT_EXP0_TAG1 +
		(CAMSVCENTRAL_INT_EXP0_OFFSET * tag_idx),
		value);


	/* setup exp1 */
	value = (ALIGN(w + x - 16, 16) << 16) | (exp1 + y);
	CAMSV_WRITE_REG(sv_dev->base +
		REG_CAMSVCENTRAL_INT_EXP1_TAG1 +
		(CAMSVCENTRAL_INT_EXP1_OFFSET * tag_idx),
		value);
	CAMSV_WRITE_REG(sv_dev->base_inner +
		REG_CAMSVCENTRAL_INT_EXP1_TAG1 +
		(CAMSVCENTRAL_INT_EXP1_OFFSET * tag_idx),
		value);

	/* enable common interrupt */
	value = 3 << (CAMSVCENTRAL_DBG_INT_BIT_START +
		CAMSVCENTRAL_DBG_INT_BIT_OFFSET * tag_idx);
	CAMSV_WRITE_REG(sv_dev->base +
		REG_CAMSVCENTRAL_COMMON_STATUS_EN,
		value);
	CAMSV_WRITE_REG(sv_dev->base_inner +
		REG_CAMSVCENTRAL_COMMON_STATUS_EN,
		value);

	dev_info(sv_dev->dev, "%s camsv_id:%d - tag_idx:%d exp0:0x%x exp1:0x%x en:0x%x\n",
		__func__, sv_dev->id, tag_idx,
		CAMSV_READ_REG(sv_dev->base_inner +
			REG_CAMSVCENTRAL_INT_EXP0_TAG1 +
			(CAMSVCENTRAL_INT_EXP0_OFFSET * tag_idx)),
		CAMSV_READ_REG(sv_dev->base_inner +
			REG_CAMSVCENTRAL_INT_EXP1_TAG1 +
			(CAMSVCENTRAL_INT_EXP1_OFFSET * tag_idx)),
		CAMSV_READ_REG(sv_dev->base_inner +
			REG_CAMSVCENTRAL_COMMON_STATUS_EN));
EXIT:
	return;
}

int mtk_cam_sv_reset_msgfifo(struct mtk_camsv_device *sv_dev)
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
			 "tg_sen_mode: 0x%x, cq_dma_sw_ctl:0x%x\n",
			 readl(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE),
			 readl(sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL));
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

	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, CAM_SUB_EN, 0);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, SOF_SUB_EN, 0);
	CAMSV_WRITE_BITS(sv_dev->base_inner + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, CAM_SUB_EN, 0);
	CAMSV_WRITE_BITS(sv_dev->base_inner + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, SOF_SUB_EN, 0);

	CAMSV_WRITE_BITS(sv_dev->base_scq_inner + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_SUBSAMPLE_EN, 0);

	writel(0, sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL);
	writel(1, sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL);
	wmb(); /* make sure committed */

	ret = readx_poll_timeout(readl, sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL,
				 dma_sw_ctl,
				 dma_sw_ctl & 0x2,
				 1 /* delay, us */,
				 100000 /* timeout, us */);
	if (ret < 0) {

		dev_info(sv_dev->dev,
			 "%s: camsv dma timeout tg_sen_mode: 0x%x, dma_sw_ctl:0x%x camsv_dcm_status:0x%x cam_main_gals_dbg_status 0x%x, cam_main_ppc_prot_rdy_0 0x%x, cam_main_ppc_prot_rdy_1 0x%x\n",
			 __func__,
			 readl(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE),
			 readl(sv_dev->base_dma + REG_CAMSVDMATOP_SW_RST_CTL),
			 readl(sv_dev->base + REG_CAMSVCENTRAL_DCM_DIS_STATUS),
			 readl(sv_dev->cam->base + 0x414),
			 readl(sv_dev->cam->base + 0x588),
			 readl(sv_dev->cam->base + 0x58c));
		writel(0xf, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
		dev_info(sv_dev->dev, "camsv dma port0x%x",
			readl(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT));
		writel(0x1f, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
		dev_info(sv_dev->dev, "camsv dma port0x%x",
			readl(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT));
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
		dev_info(sv_dev->dev,
			 "%s: cq dma timeout tg_sen_mode: 0x%x, cq_dma_sw_ctl:0x%x cam_main_gals_dbg_status 0x%x, cam_main_ppc_prot_rdy_0 0x%x, cam_main_ppc_prot_rdy_1 0x%x\n",
			 __func__,
			 readl(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE),
			 readl(sv_dev->base_scq + REG_CAMSVCQTOP_SW_RST_CTL),
			 readl(sv_dev->cam->base + 0x414),
			 readl(sv_dev->cam->base + 0x588),
			 readl(sv_dev->cam->base + 0x58c));

		CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG_SET,
			CAMSVCQTOP_DEBUG_SET, CAMSVCQTOP_DEBUG_TOP_SEL, 0);
		CAMSV_WRITE_BITS(sv_dev->base_scq_inner + REG_CAMSVCQ_CQ_EN,
			CAMSVCQ_CQ_EN, CAMSVCQ_CQ_DBG_MAIN_SUB_SEL, 1);
		CAMSV_WRITE_BITS(sv_dev->base_scq_inner + REG_CAMSVCQ_CQ_EN,
			CAMSVCQ_CQ_EN, CAMSVCQ_CQ_DBG_SEL, 1);
		CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG_SET,
			CAMSVCQTOP_DEBUG_SET, CAMSVCQTOP_DEBUG_SEL, 0x0);
		dev_info(sv_dev->dev, "cqd0 checksum 0x%x",
			readl(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG));
		CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG_SET,
			CAMSVCQTOP_DEBUG_SET, CAMSVCQTOP_DEBUG_SEL, 0x4);
		dev_info(sv_dev->dev, "cqd1 checksum 0x%x",
			readl(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG));
		CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG_SET,
			CAMSVCQTOP_DEBUG_SET, CAMSVCQTOP_DEBUG_SEL, 0x8);
		dev_info(sv_dev->dev, "cqa0 checksum 0x%x",
			readl(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG));
		CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG_SET,
			CAMSVCQTOP_DEBUG_SET, CAMSVCQTOP_DEBUG_SEL, 0xc);
		dev_info(sv_dev->dev, "cqa1 checksum 0x%x",
			readl(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG));
		CAMSV_WRITE_BITS(sv_dev->base_scq_inner + REG_CAMSVCQ_CQ_EN,
			CAMSVCQ_CQ_EN, CAMSVCQ_CQ_DBG_SEL, 0);
		CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG_SET,
			CAMSVCQTOP_DEBUG_SET, CAMSVCQTOP_DEBUG_SEL, 0x1);
		dev_info(sv_dev->dev, "thr_state 0x%x",
			readl(sv_dev->base_scq + REG_CAMSVCQTOP_DEBUG));
		writel(0xDEADBEEF, sv_dev->base + REG_CAMSVCENTRAL_CAMSV_SPARE0);
		dev_info(sv_dev->dev, "camsv spare reigister WR test write 0xDEADBEEF, read 0x%x",
			readl(sv_dev->base + REG_CAMSVCENTRAL_CAMSV_SPARE0));
		writel(0x3000100, sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_SEL);
		writel(1, sv_dev->base_scq + REG_CAMSVCQ_SCQ_MISC);
		writel(1, sv_dev->base_scq + REG_CAMSVCQ_SCQ_SUB_MISC);

		dev_info(sv_dev->dev, "cqi_e1 state checksum 0x%x dbg_sel0x%x",
			readl(sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_PORT),
			readl(sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_SEL));
		writel(0x3000500, sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_SEL);
		dev_info(sv_dev->dev, "cqi_e1 smi debug data 0x%x dbg_sel0x%x",
			readl(sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_PORT),
			readl(sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_SEL));
		writel(0x3000101, sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_SEL);
		dev_info(sv_dev->dev, "cqi_e2 state checksum 0x%x dbg_sel0x%x",
			readl(sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_PORT),
			readl(sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_SEL));
		writel(0x3000501, sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_SEL);
		dev_info(sv_dev->dev, "cqi_e2 smi debug data 0x%x dbg_sel0x%x",
			readl(sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_PORT),
			readl(sv_dev->base_scq + REG_CAMSVCQDMATOP_DMA_DBG_SEL));
		writel(0, sv_dev->base_scq + REG_CAMSVCQ_SCQ_MISC);
		writel(0, sv_dev->base_scq + REG_CAMSVCQ_SCQ_SUB_MISC);
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

	/* disable cq dcm dis*/
	writel(0x0, sv_dev->base_scq + REG_CAMSVCQTOP_DCM_DIS);
	wmb(); /* make sure committed */

RESET_FAILURE:
	return;
}

int mtk_cam_sv_dmao_common_config(struct mtk_camsv_device *sv_dev,
	unsigned int fifo_img_p1, unsigned int fifo_img_p2,
	unsigned int fifo_len_p1, unsigned int fifo_len_p2,
	unsigned int leading_line_cnt, bool enable_stash_eco_fun)
{
	int ret = 0;
	struct sv_dma_th_setting th_setting;
	struct sv_dma_bw_setting bw_setting;

	memset(&th_setting, 0, sizeof(struct sv_dma_th_setting));
	memset(&bw_setting, 0, sizeof(struct sv_dma_bw_setting));

	bw_setting.urgent_high = urgent_high;
	bw_setting.urgent_low = urgent_low;
	bw_setting.ultra_high = ultra_high;
	bw_setting.ultra_low = ultra_low;
	bw_setting.pultra_high =  pultra_high;
	bw_setting.pultra_low = pultra_low;

	CALL_PLAT_V4L2(
		get_sv_dma_th_setting, sv_dev->id, fifo_img_p1, fifo_img_p2,
		fifo_len_p1, fifo_len_p2, &th_setting, &bw_setting);

	switch (sv_dev->id) {
	case CAMSV_0:
		/* wdma 1 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG,
			th_setting.urgent_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG,
			th_setting.ultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG,
			th_setting.pultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG,
			th_setting.dvfs_th);

		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_LEN,
			th_setting.urgent_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_LEN,
			th_setting.ultra_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_LEN,
			th_setting.pultra_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_LEN,
			th_setting.dvfs_len1_th);

		/* wdma 2 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG2,
			th_setting.urgent_th2);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG2,
			th_setting.ultra_th2);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG2,
			th_setting.pultra_th2);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG2,
			th_setting.dvfs_th2);

		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_LEN2,
			th_setting.urgent_len2_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_LEN2,
			th_setting.ultra_len2_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_LEN2,
			th_setting.pultra_len2_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_LEN2,
			th_setting.dvfs_len2_th);

		/* stg wdma 1 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_NONE_SAME_PG_SEND_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_LEADING_CNT_SRC,
			leading_line_cnt);
		/* ECO item */
		if (enable_stash_eco_fun)
			CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_SW_CTRL,
				0x8000);

		/* stg wdma 2 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG2_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG2_NONE_SAME_PG_SEND_EN_CTRL,
			0xFFF);
		if (enable_stash_eco_fun)
			CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG2_SW_CTRL,
				0x8000);
		break;
	case CAMSV_1:
		/* wdma 1 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG,
			th_setting.urgent_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG,
			th_setting.ultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG,
			th_setting.pultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG,
			th_setting.dvfs_th);

		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_LEN,
			th_setting.urgent_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_LEN,
			th_setting.ultra_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_LEN,
			th_setting.pultra_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_LEN,
			th_setting.dvfs_len1_th);

		/* wdma 2 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG2,
			th_setting.urgent_th2);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG2,
			th_setting.ultra_th2);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG2,
			th_setting.pultra_th2);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG2,
			th_setting.dvfs_th2);

		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_LEN2,
			th_setting.urgent_len2_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_LEN2,
			th_setting.ultra_len2_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_LEN2,
			th_setting.pultra_len2_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_LEN2,
			th_setting.dvfs_len2_th);

		/* stg wdma 1 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_NONE_SAME_PG_SEND_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_LEADING_CNT_SRC,
			leading_line_cnt);
		/* ECO item */
		if (enable_stash_eco_fun)
			CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_SW_CTRL,
				0x8000);

		/* stg wdma 2 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG2_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG2_NONE_SAME_PG_SEND_EN_CTRL,
			0xFFF);
		if (enable_stash_eco_fun)
			CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG2_SW_CTRL,
				0x8000);
		break;
	case CAMSV_2:
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG,
			th_setting.urgent_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG,
			th_setting.ultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG,
			th_setting.pultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG,
			th_setting.dvfs_th);

		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_LEN,
			th_setting.urgent_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_LEN,
			th_setting.ultra_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_LEN,
			th_setting.pultra_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_LEN,
			th_setting.dvfs_len1_th);

		/* stg wdma 1 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_NONE_SAME_PG_SEND_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_LEADING_CNT_SRC,
			leading_line_cnt);
		/* ECO item */
		if (enable_stash_eco_fun)
			CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_SW_CTRL,
				0x8000);
		break;
	case CAMSV_3:
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG,
			th_setting.urgent_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG,
			th_setting.ultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG,
			th_setting.pultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG,
			th_setting.dvfs_th);

		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_LEN,
			th_setting.urgent_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_LEN,
			th_setting.ultra_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_LEN,
			th_setting.pultra_len1_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_LEN,
			th_setting.dvfs_len1_th);

		/* stg wdma 1 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_NONE_SAME_PG_SEND_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_LEADING_CNT_SRC,
			leading_line_cnt);
		/* ECO item */
		if (enable_stash_eco_fun)
			CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_SW_CTRL,
				0x8000);
		break;
	case CAMSV_4:
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG,
			th_setting.urgent_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG,
			th_setting.ultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG,
			th_setting.pultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG,
			th_setting.dvfs_th);

		/* stg wdma 1 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_NONE_SAME_PG_SEND_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_LEADING_CNT_SRC,
			leading_line_cnt);
		/* ECO item */
		if (enable_stash_eco_fun)
			CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_SW_CTRL,
				0x8000);
		break;
	case CAMSV_5:
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG,
			th_setting.urgent_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG,
			th_setting.ultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG,
			th_setting.pultra_th);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG,
			th_setting.dvfs_th);

		/* stg wdma 1 */
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_NONE_SAME_PG_SEND_EN_CTRL,
			0xFFF);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_LEADING_CNT_SRC,
			leading_line_cnt);
		/* ECO item */
		if (enable_stash_eco_fun)
			CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVSTG1_SW_CTRL,
				0x8000);
		break;
	}

	/* cqi */
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQI_E1_ORIRDMA_CON0,
		th_setting.cq1_fifo_size);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQI_E1_ORIRDMA_CON1,
		th_setting.cq1_pultra_th);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQI_E1_ORIRDMA_CON2,
		th_setting.cq1_ultra_th);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQI_E1_ORIRDMA_CON3,
		th_setting.cq1_urgent_th);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQI_E1_ORIRDMA_CON4,
		th_setting.cq1_dvfs_th);

	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQI_E2_ORIRDMA_CON0,
		th_setting.cq2_fifo_size);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQI_E2_ORIRDMA_CON1,
		th_setting.cq2_pultra_th);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQI_E2_ORIRDMA_CON2,
		th_setting.cq2_ultra_th);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQI_E2_ORIRDMA_CON3,
		th_setting.cq2_urgent_th);
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQI_E2_ORIRDMA_CON4,
		th_setting.cq2_dvfs_th);

	sv_dev->enable_stash_eco_fun = 0;

	dev_dbg(sv_dev->dev, "stash eco status:%d img:0x%x_0x%x_0x%x_0x%x img2:0x%x_0x%x_0x%x_0x%x len:0x%x_0x%x_0x%x_0x%x len2:0x%x_0x%x_0x%x_0x%x\n",
		enable_stash_eco_fun,
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_IMG2),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_IMG2),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_IMG2),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_IMG2),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_LEN),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_LEN),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_LEN),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_LEN),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON3_LEN2),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON2_LEN2),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON1_LEN2),
		CAMSV_READ_REG(sv_dev->base_dma + REG_CAMSVDMATOP_CON4_LEN2));

	return ret;
}

int mtk_cam_sv_smi_path_sel(struct mtk_camsv_device *sv_dev, bool is_two_smi_comm)
{
	int ret = 0;

	switch (sv_dev->id) {
	case CAMSV_0:
	case CAMSV_1:
		smi_sysram_enable(&sv_dev->larb_pdev->dev,
			sv_dev->larb_master_id[SMI_PORT0_SV_CQI], false, "camsys-camsv");
		smi_sysram_enable(&sv_dev->larb_pdev->dev,
			sv_dev->larb_master_id[SMI_PORT1_SV_WDMA], false, "camsys-camsv");
		smi_sysram_enable(&sv_dev->larb_pdev->dev,
			sv_dev->larb_master_id[SMI_PORT2_SV_WDMA], false, "camsys-camsv");
		break;
	case CAMSV_2:
	case CAMSV_3:
	case CAMSV_4:
	case CAMSV_5:
		WARN_ON(is_two_smi_comm);
		/* default disp */
		smi_sysram_enable(&sv_dev->larb_pdev->dev,
			sv_dev->larb_master_id[SMI_PORT0_SV_CQI], false, "camsys-camsv");
		smi_sysram_enable(&sv_dev->larb_pdev->dev,
			sv_dev->larb_master_id[SMI_PORT1_SV_WDMA], false, "camsys-camsv");
		break;
	}

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

	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, CMOS_EN, 1);
	if (atomic_read(&sv_dev->is_sub_en))
		CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
			CAMSVCENTRAL_SEN_MODE, CAM_SUB_EN, 1);
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_VF_CON,
		CAMSVCENTRAL_VF_CON, VFDATA_EN, 1);

	dev_info(sv_dev->dev, "%s sen_mode:0x%x vf_con:0x%x\n",
		__func__,
		CAMSV_READ_REG(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE),
		CAMSV_READ_REG(sv_dev->base + REG_CAMSVCENTRAL_VF_CON));

	return ret;
}

int mtk_cam_sv_central_common_disable(struct mtk_camsv_device *sv_dev)
{
	int ret = 0, i;

	/* disable dma dcm before do dma reset */
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DCM_DIS, 1);

	/* turn off interrupt */
	/* channel status must be disabled before sof status disabled */
	/* due to channel status cleared in sof irq handler */
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_CHANNEL_STATUS_EN, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_COMMON_STATUS_EN, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DONE_STATUS_EN, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_ERR_STATUS_EN, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_SOF_STATUS_EN, 0);

	/* avoid camsv tag data leakage */
	for (i = SVTAG_START; i < SVTAG_END; i++) {
		CAMSV_WRITE_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_GRAB_PXL_TAG1 +
			CAMSVCENTRAL_GRAB_PXL_TAG_SHIFT * i, 0);
		CAMSV_WRITE_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_GRAB_LIN_TAG1 +
			CAMSVCENTRAL_GRAB_LIN_TAG_SHIFT * i, 0);

		CAMSV_WRITE_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_INT_EXP0_TAG1 +
			(CAMSVCENTRAL_INT_EXP0_OFFSET * i), 0);
		CAMSV_WRITE_REG(sv_dev->base_inner + REG_CAMSVCENTRAL_INT_EXP1_TAG1 +
			(CAMSVCENTRAL_INT_EXP1_OFFSET * i), 0);
	}

	/* bypass tg_mode function before vf off */
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, TG_MODE_OFF, 1);

	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_VF_CON,
		CAMSVCENTRAL_VF_CON, VFDATA_EN, 0);

	mtk_cam_sv_toggle_tg_db(sv_dev);

	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_SEN_MODE,
		CAMSVCENTRAL_SEN_MODE, CMOS_EN, 0);

	sv_reset(sv_dev);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DMA_EN_IMG, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DCIF_SET, 0);
	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DCIF_SEL, 0);

	CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_AXSLC_CMD, 0);
	for (i = SVTAG_START; i < SVTAG_END; i++) {
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_WDMA_SPECIAL_EN_IMG1 +
			CAMSVDMATOP_WDMA_SPECIAL_IMG_SHIFT * i, 0);
		CAMSV_WRITE_REG(sv_dev->base_dma + REG_CAMSVDMATOP_WDMA_AXSLC_SIZE_IMG1 +
			CAMSVDMATOP_WDMA__AXSLC_IMG_SHIFT * i, 0);
	}

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
	unsigned int enable_tags)
{
	int i;

	for (i = SVTAG_START; i < SVTAG_END; i++) {
		if (!(enable_tags & (1 << i)))
			continue;
		if (CAMSV_READ_BITS(sv_dev->base_inner +
				REG_CAMSVCENTRAL_FBC1_TAG1 + CAMSVCENTRAL_FBC1_TAG_SHIFT * i,
				CAMSVCENTRAL_FBC1_TAG1, FBC_CNT_TAG1))
			return 0;
	}

	return 1;
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

void apply_camsv_cq(struct mtk_camsv_device *sv_dev,
	dma_addr_t cq_addr, unsigned int cq_size,
	unsigned int cq_offset, int initial)
{
#define CQ_VADDR_MASK 0xFFFFFFFF
	u32 cq_addr_lsb = (cq_addr + cq_offset) & CQ_VADDR_MASK;
	u32 cq_addr_msb = ((cq_addr + cq_offset) >> 32);

	if (cq_size == 0)
		return;

	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_CQ_SUB_THR0_DESC_SIZE_2,
		cq_size);
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_CQ_SUB_THR0_BASEADDR_2_MSB,
		cq_addr_msb);
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_CQ_SUB_THR0_BASEADDR_2,
		cq_addr_lsb);
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQTOP_THR_START, 1);
	wmb(); /* TBC */

	if (initial) {
		/* enable stagger mode for multiple vsync(s) */
		CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
			CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_STAGGER_MODE, 1);
		dev_info(sv_dev->dev, "apply 1st camsv scq: addr_msb:0x%x addr_lsb:0x%x size:%d cq_en:0x%x\n",
			cq_addr_msb, cq_addr_lsb, cq_size,
			CAMSV_READ_REG(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN));
	} else
		dev_dbg(sv_dev->dev, "apply camsv scq: addr_msb:0x%x addr_lsb:0x%x size:%d\n",
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

bool mtk_cam_is_non_comb_ic(struct mtk_cam_ctx *ctx)
{
	if (!ctx->num_sv_subdevs)
		return false;

	if (ctx->seninf)
		return mtk_cam_seninf_is_non_comb_ic(ctx->seninf) != 0;
	return false;
}

void mtk_cam_update_sensor_resource(struct mtk_cam_ctx *ctx)
{
	struct mtk_camsv_device *sv_dev;
	struct v4l2_subdev_frame_interval fi;
	struct v4l2_ctrl *ctrl;
	struct v4l2_subdev_format sd_fmt;
	unsigned long vblank = 0;

	if (ctx->hw_sv == NULL)
		return;
	sv_dev = dev_get_drvdata(ctx->hw_sv);

	memset(&sv_dev->sensor_res, 0, sizeof(struct mtk_cam_resource_sensor_v2));
	memset(&fi, 0, sizeof(fi));
	memset(&sd_fmt, 0, sizeof(sd_fmt));

	if (ctx->sensor) {
		fi.pad = 0;
		fi.reserved[0] = V4L2_SUBDEV_FORMAT_ACTIVE;
#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
		v4l2_subdev_call_state_active(ctx->sensor, pad, get_frame_interval, &fi);
#else
		v4l2_subdev_call(ctx->sensor, video, g_frame_interval, &fi);
#endif

		ctrl = v4l2_ctrl_find(ctx->sensor->ctrl_handler, V4L2_CID_VBLANK);
		if (!ctrl)
			dev_info(ctx->cam->dev, "[%s] ctrl is NULL\n", __func__);
		else
			vblank = v4l2_ctrl_g_ctrl(ctrl);

		sd_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
		sd_fmt.pad = PAD_SRC_RAW0;
		v4l2_subdev_call(ctx->seninf, pad, get_fmt, NULL, &sd_fmt);

		/* update sensor resource */
		sv_dev->sensor_res.width = sd_fmt.format.width;
		sv_dev->sensor_res.height = sd_fmt.format.height;
		sv_dev->sensor_res.code = sd_fmt.format.code;
		sv_dev->sensor_res.interval.numerator = fi.interval.numerator;
		sv_dev->sensor_res.interval.denominator = fi.interval.denominator;
		sv_dev->sensor_res.vblank = vblank;
	}
}

struct mtk_cam_seninf_sentest_param *
	mtk_cam_get_sentest_param(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_cam_v4l2_pipelines *ppls = &cam->pipelines;
	struct mtk_camsv_pipeline *sv_pipe = NULL;
	struct mtk_camsv_pad_config *pad = NULL;
	unsigned long submask;
	int i;

	submask = bit_map_subset_of(MAP_SUBDEV_CAMSV, ctx->used_pipe);
	for (i = 0; i < ppls->num_camsv && submask; i++, submask >>= 1) {
		if (!(submask & 0x1))
			continue;

		sv_pipe = &ppls->camsv[i];
		pad = &sv_pipe->pad_cfg[MTK_CAMSV_SINK];

		break;
	}

	if (sv_pipe && pad) {
		if (atomic_read(&sv_pipe->is_sentest_param_updated))
			return &sv_pipe->sentest_param;

		memset(&sv_pipe->sentest_param, 0,
			sizeof(struct mtk_cam_seninf_sentest_param));

		mtk_cam_seninf_get_sentest_param(ctx->seninf,
			pad->mbus_fmt.code,
			&sv_pipe->sentest_param);
		atomic_set(&sv_pipe->is_sentest_param_updated, 1);

		pr_info("%s: mbus_code:0x%x is_lbmf:%d\n",
			__func__,
			pad->mbus_fmt.code,
			sv_pipe->sentest_param.is_lbmf);

		return &sv_pipe->sentest_param;
	} else {
		pr_info("%s: sv pipe not found(used_pipe:0x%x)\n",
			__func__, ctx->used_pipe);

		return NULL;
	}
}

int mtk_cam_sv_golden_set(struct mtk_camsv_device *sv_dev, bool is_golden_set)
{
	int ret = 0;
#ifdef MMINFRA_PLUS_ONE
	mtk_mmdvfs_camsv_dc_enable(sv_dev->id, is_golden_set);
	dev_info(sv_dev->dev, "%s: is_golden_set:%d",
		__func__, (is_golden_set) ? 1 : 0);
#endif
	return ret;
}

unsigned int mtk_cam_get_sv_tag_index(struct mtk_camsv_tag_info *arr_tag,
	unsigned int pipe_id)
{
	int i;

	for (i = SVTAG_START; i < SVTAG_END; i++) {
		struct mtk_camsv_tag_info *tag_info = &arr_tag[i];

		if (tag_info->sv_pipe && (tag_info->sv_pipe->id == pipe_id))
			return i;
	}

	pr_info("[%s] tag is not found by pipe_id(%d)", __func__, pipe_id);
	return 0;
}

unsigned int mtk_cam_get_seninf_pad_index(struct mtk_camsv_tag_info *arr_tag,
	unsigned int pipe_id)
{
	int i;

	for (i = SVTAG_START; i < SVTAG_END; i++) {
		struct mtk_camsv_tag_info *tag_info = &arr_tag[i];

		if (tag_info->sv_pipe && (tag_info->sv_pipe->id == pipe_id))
			return tag_info->sv_pipe->seninf_padidx;
	}

	pr_info("[%s] seninf pad is not found by pipe_id(%d)", __func__, pipe_id);
	return 0;
}

int mtk_cam_sv_dev_config(struct mtk_camsv_device *sv_dev,
	unsigned int sub_ratio, int frm_time_us)
{
	engine_fsm_reset(&sv_dev->fsm, sv_dev->dev);
	sv_dev->cq_ref = NULL;

	sv_dev->first_tag = 0;
	sv_dev->last_tag = 0;

	sv_dev->handled_tags = 0;
	sv_dev->used_tags = 0;
	sv_dev->drop_tags = 0;

	sv_dev->streaming_tag_cnt = 0;
	sv_dev->sof_count = 0;
	sv_dev->tg_cnt = 0;
	sv_dev->ois_updated_seq = 0;

	atomic_set(&sv_dev->is_otf, 0);
	atomic_set(&sv_dev->is_seamless, 0);
	atomic_set(&sv_dev->is_sw_clr, 0);
	atomic_set(&sv_dev->is_fifo_full, 0);
	atomic_set(&sv_dev->is_sub_en, 0);

	mtk_cam_sv_dmao_common_config(sv_dev, 0, 0, 0, 0, 0, 0);
	mtk_cam_sv_cq_config(sv_dev, sub_ratio);
	mtk_cam_sv_ddren_qos_config(sv_dev, frm_time_us);

	dev_info(sv_dev->dev, "[%s] sub_ratio:%d set seamless check\n", __func__, sub_ratio);

	return 0;
}

void mtk_cam_sv_fill_tag_info(struct mtk_camsv_tag_info *arr_tag,
	struct mtkcam_ipi_config_param *ipi_config,
	struct mtk_camsv_tag_param *tag_param, unsigned int hw_scen,
	unsigned int pixelmode, unsigned int sub_ratio,
	unsigned int mbus_width, unsigned int mbus_height,
	unsigned int mbus_code,	struct mtk_camsv_pipeline *pipeline)
{
	struct mtk_camsv_tag_info *tag_info = &arr_tag[tag_param->tag_idx];
	struct mtkcam_ipi_input_param *cfg_in_param =
		&ipi_config->sv_input[0][tag_param->tag_idx].input;

	tag_info->sv_pipe = pipeline;
	tag_info->seninf_padidx = tag_param->seninf_padidx;
	tag_info->hw_scen = hw_scen;
	tag_info->tag_order = tag_param->tag_order;
	tag_info->pixel_mode = pixelmode;

	cfg_in_param->pixel_mode = pixelmode;
	cfg_in_param->data_pattern = 0x0;
	cfg_in_param->in_crop.p.x = 0x0;
	cfg_in_param->in_crop.p.y = 0x0;
	cfg_in_param->in_crop.s.w = mbus_width;
	cfg_in_param->in_crop.s.h = mbus_height;
	cfg_in_param->fmt = sensor_mbus_to_ipi_fmt(mbus_code);
	cfg_in_param->raw_pixel_id = sensor_mbus_to_ipi_pixel_id(mbus_code);
	cfg_in_param->subsample = sub_ratio;
	// handle yuv special case
	if (cfg_in_param->fmt == MTKCAM_IPI_IMG_FMT_YUYV||
		cfg_in_param->fmt == MTKCAM_IPI_IMG_FMT_UYVY||
		cfg_in_param->fmt == MTKCAM_IPI_IMG_FMT_VYUY||
		cfg_in_param->fmt == MTKCAM_IPI_IMG_FMT_YVYU) {
		cfg_in_param->in_crop.s.w = mbus_width * 2;
	}
	// handle rgb888 special case
	if (cfg_in_param->fmt == MTKCAM_IPI_IMG_FMT_RGB888) {
		cfg_in_param->in_crop.s.w = mbus_width * 3;
	}
}

int mtk_cam_sv_get_tag_param(struct mtk_camsv_tag_param *arr_tag_param,
	unsigned int hw_scen, unsigned int exp_no, unsigned int req_amount)
{
	int ret = 0;

	if (hw_scen == (1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_STAGGER)) ||
		hw_scen == (1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_DC_STAGGER)) ||
		hw_scen == (1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_OFFLINE_STAGGER))) {
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
	} else if (hw_scen == (1 << MTKCAM_SV_SPECIAL_SCENARIO_NON_COMB_IC)) {
		memcpy(arr_tag_param, sv_tag_param_non_comb_ic,
			sizeof(struct mtk_camsv_tag_param) * req_amount);
	} else if (hw_scen == (1 << MTKCAM_SV_SPECIAL_SCENARIO_EXT_ISP)) {
		memcpy(arr_tag_param, sv_tag_param_extisp,
			sizeof(struct mtk_camsv_tag_param) * req_amount);
	} else {
		pr_info("failed to get tag param(hw_scen:0x%x/exp_no:%d)",
			hw_scen, exp_no);
		ret = 1;
	}

	return ret;
}

int mtk_cam_sv_cq_config(struct mtk_camsv_device *sv_dev, unsigned int sub_ratio)
{
	/* cq en */
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_CQ_DB_EN, 1);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_STAGGER_MODE, 0);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN,
		CAMSVCQ_CQ_EN, CAMSVCQ_SCQ_SUBSAMPLE_EN, (sub_ratio) ? 1 : 0);

	/* cq sub en */
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_EN,
		CAMSVCQ_CQ_SUB_EN, CAMSVCQ_CQ_SUB_DB_EN, 1);

	/* cq dma dcm dis enable */
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQTOP_DCM_DIS, 0x7);

	/* scq start period */
	CAMSV_WRITE_REG(sv_dev->base_scq  + REG_CAMSVCQ_SCQ_START_PERIOD,
		0xFFFFFFFF);

	/* cq sub thr0 ctl */
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_MODE, 1);
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_EN, 1);

	/* cq int en */
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQTOP_INT_0_EN,
		ERR_ST_MASK_CQ_ERR | CAMSVCQTOP_SCQ_SUB_THR_DONE);

	wmb(); /* TBC */

	dev_dbg(sv_dev->dev, "[%s] cq_en:0x%x_%x start_period:0x%x cq_sub_thr0_ctl:0x%x cq_int_en:0x%x cq_dcm0x%x\n",
		__func__,
		CAMSV_READ_REG(sv_dev->base_scq + REG_CAMSVCQ_CQ_EN),
		CAMSV_READ_REG(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_EN),
		CAMSV_READ_REG(sv_dev->base_scq + REG_CAMSVCQ_SCQ_START_PERIOD),
		CAMSV_READ_REG(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL),
		CAMSV_READ_REG(sv_dev->base_scq + REG_CAMSVCQTOP_INT_0_EN),
		CAMSV_READ_REG(sv_dev->base_scq + REG_CAMSVCQTOP_DCM_DIS));

	return 0;
}

#define HW_TIMER_INC_PERIOD   0x2
#define DDR_GEN_BEFORE_US     3
#define QOS_GEN_BEFORE_US     100
#define MARGIN                300
int mtk_cam_sv_ddren_qos_config(struct mtk_camsv_device *sv_dev, int frm_time_us)
{
	int ddr_gen_pulse, qos_gen_pulse;

	if (CAM_DEBUG_ENABLED(MMQOS))
		pr_info("%s frm_time_us %d\n", __func__, frm_time_us);

	if (frm_time_us == -1)
		return 0;

	ddr_gen_pulse = (frm_time_us - DDR_GEN_BEFORE_US - MARGIN) * SCQ_DEFAULT_CLK_RATE /
		(2 * (HW_TIMER_INC_PERIOD + 1)) - 1;

	qos_gen_pulse = (frm_time_us - QOS_GEN_BEFORE_US - MARGIN) * SCQ_DEFAULT_CLK_RATE /
		(2 * (HW_TIMER_INC_PERIOD + 1)) - 1;

	/* sw ddr en */
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_DDR_CFG,
		CAMSVCENTRAL_DDR_CFG, DDR_SET, 1);

	/* cq en */
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_DDR_CFG,
		CAMSVCENTRAL_DDR_CFG, DDR_OR_CQ_EN, 1);

	/* sw bw_qos en */
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_BW_QOS_CFG,
		CAMSVCENTRAL_BW_QOS_CFG, BW_QOS_SET, 1);

	/* cq en */
	CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_BW_QOS_CFG,
		CAMSVCENTRAL_BW_QOS_CFG, BW_QOS_OR_CQ_EN, 1);

	if (ddr_gen_pulse < 0 || qos_gen_pulse < 0) {
		pr_info("%s: framelength too small, so use sw mode\n", __func__);
		atomic_set(&sv_dev->is_sw_clr, 3);
		return 0;
	}

	if (debug_ddren_camsv_hw_mode) {
		CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_DDR_CFG,
			CAMSVCENTRAL_DDR_CFG, DDR_MODE_SEL, 0);
		CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_DDR_CFG,
			CAMSVCENTRAL_DDR_CFG, DDR_TIMER_EN, 1);
		CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_DDR_THRESHOLD,
			ddr_gen_pulse);

		CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_BW_QOS_CFG,
			CAMSVCENTRAL_BW_QOS_CFG, BW_QOS_MODE_SEL, 0);
		CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_BW_QOS_CFG,
			CAMSVCENTRAL_BW_QOS_CFG, BW_QOS_TIMER_EN, 1);
		CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_BW_QOS_THRESHOLD,
			qos_gen_pulse);
	}
	return 0;
}

#define CAMSV_TS_CNT 0x2
void mtk_cam_sv_update_start_period(
	struct mtk_camsv_device *sv_dev, int scq_ms)
{
	u32 scq_cnt_rate, start_period;

	CAMSV_WRITE_REG(sv_dev->base + REG_CAMSVCENTRAL_TIME_STAMP_CTL, 0x101);
	CAMSV_WRITE_REG(sv_dev->base +
		REG_CAMSVCENTRAL_TIME_STAMP_INC_CNT, CAMSV_TS_CNT);

	/* scq count rate(khz) */
	scq_cnt_rate = SCQ_DEFAULT_CLK_RATE * 1000 / ((CAMSV_TS_CNT + 1) * 2);
	start_period = (scq_ms == -1) ? 0xFFFFFFFF : scq_ms * scq_cnt_rate;

	/* scq start period */
	CAMSV_WRITE_REG(sv_dev->base_scq + REG_CAMSVCQ_SCQ_START_PERIOD,
		start_period);

	dev_info(sv_dev->dev, "[%s] start_period:0x%x ts_cnt:%d, scq_ms:%d\n",
		__func__,
		CAMSV_READ_REG(sv_dev->base_scq + REG_CAMSVCQ_SCQ_START_PERIOD),
		CAMSV_TS_CNT, scq_ms);
}

int mtk_cam_sv_cq_disable(struct mtk_camsv_device *sv_dev)
{
	CAMSV_WRITE_BITS(sv_dev->base_scq + REG_CAMSVCQ_CQ_SUB_THR0_CTL,
		CAMSVCQ_CQ_SUB_THR0_CTL, CAMSVCQ_CQ_SUB_THR0_EN, 0);

	return 0;
}

int mtk_cam_sv_dev_pertag_stream_on(
	struct mtk_camsv_device *sv_dev,
	unsigned int tag_idx,
	bool on)
{
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

		ret |= mtk_cam_sv_fbc_disable(sv_dev, tag_idx);
		sv_dev->streaming_tag_cnt--;
	}

EXIT:
	dev_info(sv_dev->dev, "camsv %d %s en(%d) streaming_tag_cnt:%d\n",
		sv_dev->id, __func__, (on) ? 1 : 0, sv_dev->streaming_tag_cnt);
	return ret;
}

int mtk_cam_sv_dev_stream_on(struct mtk_camsv_device *sv_dev, bool on,
	unsigned int enabled_tags, unsigned int used_tag_cnt)
{
	int ret = 0, i;

	if (on) {
		/* keep enabled tag info. for stream off use */
		sv_dev->enabled_tags = enabled_tags;
		sv_dev->used_tag_cnt = used_tag_cnt;
		/* enable camsv smi snoc config */
		if (sv_dev->id == 0)
			CAMSV_WRITE_REG(sv_dev->cam->base + REG_CAM_MAIN_LARB14_VC_SEL, 0x37);
		else if (sv_dev->id == 1)
			CAMSV_WRITE_REG(sv_dev->cam->base + REG_CAM_MAIN_LARB13_VC_SEL, 0x67);
		else
			CAMSV_WRITE_REG(sv_dev->cam->base + REG_CAM_MAIN_LARB29_VC_SEL, 0x3CFF);
	}

	for (i = SVTAG_START; i < SVTAG_END; i++) {
		if (sv_dev->enabled_tags & (1 << i))
			mtk_cam_sv_dev_pertag_stream_on(sv_dev, i, on);
	}

	return ret;
}

void camsv_dump_dma_debug_data(struct mtk_camsv_device *sv_dev)
{
	u32 smi_crc_address, smi_crc_data, tag1_tag2_crc, len1_len2_crc, smi_cnt;
	u32 debug_img1, debug_len1, cmd_cnt_img1, cmd_cnt_len1;

	writel_relaxed(0x00010001, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
	smi_crc_address = readl_relaxed(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x00010003, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
	smi_crc_data = readl_relaxed(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x00010005, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
	tag1_tag2_crc = readl_relaxed(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x00010009, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
	len1_len2_crc = readl_relaxed(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x0001000F, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
	smi_cnt = readl_relaxed(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x0001010B, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
	debug_img1 = readl_relaxed(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x0001010C, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
	debug_len1 = readl_relaxed(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x0001010E, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
	cmd_cnt_img1 = readl_relaxed(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT);
	writel_relaxed(0x0001090E, sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_SEL);
	cmd_cnt_len1 = readl_relaxed(sv_dev->base_dma + REG_CAMSVDMATOP_DMA_DEBUG_PORT);

	dev_info_ratelimited(sv_dev->dev,
		"dma_top_debug:0x%x_0x%x_0x%x_0x%x_0x%x_0x%x_0x%x_0x%x_0x%x\n",
		smi_crc_address, smi_crc_data, tag1_tag2_crc, len1_len2_crc,
		smi_cnt, debug_img1, debug_len1, cmd_cnt_img1,
		cmd_cnt_len1);
}

int mtk_cam_sv_debug_dump(struct mtk_camsv_device *sv_dev, unsigned int dump_tags)
{
	unsigned int i;
	unsigned int tg_sen_mode, tg_vf_con, tg_path_cfg;
	unsigned int seq_no_inner, seq_no_outer;
	unsigned int tag_fmt, tag_cfg;
	unsigned int tag_fbc_status, tag_addr, tag_addr_msb;
	unsigned int frm_size, frm_size_r, grab_pix, grab_lin;
	unsigned int dcif_set, dcif_sel;
	unsigned int first_tag, last_tag, group_info;
	int need_smi_dump = false;

	dump_tags = (dump_tags) ? dump_tags : BIT(CAMSV_MAX_TAGS) - 1;

	/* check tg setting */
	tg_sen_mode = readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_SEN_MODE);
	tg_vf_con = readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_VF_CON);
	tg_path_cfg = readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_PATH_CFG);
	dev_info(sv_dev->dev,
		"tg_sen_mode:0x%x tg_vf_con:0x%x tg_path_cfg:0x%x\n",
		tg_sen_mode, tg_vf_con, tg_path_cfg);

	/* check frame setting and status for each tag */
	for (i = 0; i < CAMSV_MAX_TAGS; i++) {
		if (!(dump_tags & (1 << i)))
			continue;

		writel_relaxed((i << 22) + (i << 27),
			sv_dev->base_inner + REG_CAMSVCENTRAL_TAG_R_SEL);

		seq_no_inner =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FH_SPARE_TAG_1 +
			CAMSVCENTRAL_FH_SPARE_SHIFT * i);
		seq_no_outer =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_FH_SPARE_TAG_1 +
			CAMSVCENTRAL_FH_SPARE_SHIFT * i);
		tag_fmt =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FORMAT_TAG1 +
			CAMSVCENTRAL_FORMAT_TAG_SHIFT * i);
		tag_cfg =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_CONFIG_TAG1 +
			CAMSVCENTRAL_CONFIG_TAG_SHIFT * i);
		tag_fbc_status =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FBC1_TAG1 +
			CAMSVCENTRAL_FBC1_TAG_SHIFT * i);
		tag_addr_msb =
			readl_relaxed(sv_dev->base_dma_inner +
			REG_CAMSVDMATOP_WDMA_BASE_ADDR_MSB_IMG1 +
			CAMSVDMATOP_WDMA_BASE_ADDR_MSB_IMG_SHIFT * i);
		tag_addr =
			readl_relaxed(sv_dev->base_dma_inner + REG_CAMSVDMATOP_WDMA_BASE_ADDR_IMG1
			+ i * CAMSVDMATOP_WDMA_BASE_ADDR_IMG_SHIFT);
		frm_size =
			readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FRMSIZE_ST);
		frm_size_r =
			readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FRMSIZE_ST_R);
		grab_pix =
			readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_GRAB_PXL_TAG1 +
			CAMSVCENTRAL_GRAB_PXL_TAG_SHIFT * i);
		grab_lin =
			readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_GRAB_LIN_TAG1 +
			CAMSVCENTRAL_GRAB_LIN_TAG_SHIFT * i);

		dev_info_ratelimited(sv_dev->dev,
			"tag_idx:%d seq_no:%d_%d fmt:0x%x cfg:0x%x fbc_status:0x%x addr:0x%x_%x frm_size:0x%x frm_size_r:0x%x grab_pix:0x%x grab_lin:0x%x\n",
			i, seq_no_inner, seq_no_outer, tag_fmt, tag_cfg,
			tag_fbc_status, tag_addr_msb, tag_addr,
			frm_size, frm_size_r, grab_pix, grab_lin);
	}

	/* check dcif setting */
	dcif_set = readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_DCIF_SET);
	dcif_sel = readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_DCIF_SEL);
	dev_info(sv_dev->dev, "dcif_set:0x%x dcif_sel:0x%x\n",
		dcif_set, dcif_sel);

	/* check tag/group setting */
	first_tag = readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FIRST_TAG);
	last_tag = readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_LAST_TAG);
	for (i = 0; i < MAX_SV_HW_GROUPS; i++) {
		group_info = readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_GROUP_TAG0 +
			CAMSVCENTRAL_GROUP_TAG_SHIFT * i);
		dev_info_ratelimited(sv_dev->dev, "group[%d]:0x%x\n", i, group_info);
	}
	dev_info_ratelimited(sv_dev->dev, "first_tag:0x%x last_tag:0x%x\n",
		first_tag, last_tag);

	/* dump dma debug data */
	camsv_dump_dma_debug_data(sv_dev);

	if (atomic_read(&sv_dev->is_fifo_full)) {
		need_smi_dump = true;
		atomic_set(&sv_dev->is_fifo_full, 0);
	}

	return need_smi_dump;
}

void camsv_handle_cq_err(
	struct mtk_camsv_device *sv_dev,
	struct mtk_camsys_irq_info *data)
{
	struct mtk_cam_ctx *ctx;
	unsigned int ctx_id;
	int err_status = data->e.err_status2;
	int frame_idx_inner = data->frame_idx_inner;

	ctx_id = ctx_from_fh_cookie(frame_idx_inner);
	ctx = &sv_dev->cam->ctxs[ctx_id];

	/* dump error status */
	dev_info(sv_dev->dev, "cq error_status:0x%x\n", err_status);

	/* dump seninf debug data */
	if (ctx && ctx->seninf)
		mtk_cam_seninf_dump_current_status(ctx->seninf, false);

	/* dump camsv debug data */
	mtk_cam_sv_debug_dump(sv_dev, 0);

	mtk_smi_dbg_hang_detect("camsys-camsv");
}
void camsv_handle_err(
	struct mtk_camsv_device *sv_dev,
	struct mtk_camsys_irq_info *data)
{
	struct mtk_cam_ctx *ctx;
	unsigned int ctx_id;
	int err_status = data->e.err_status;
	int frame_idx_inner = data->frame_idx_inner;

	ctx_id = ctx_from_fh_cookie(frame_idx_inner);
	ctx = &sv_dev->cam->ctxs[ctx_id];

	/* dump error status */
	dev_info(sv_dev->dev, "error_status:0x%x\n", err_status);

	/* dump seninf debug data */
	if (ctx && ctx->seninf) {
		ctx->is_sv_mraw_error = true;
		sv_dev->camsv_error_count += 1;
		if (sv_dev->camsv_error_count >= 2 && !ctx->is_seninf_error_trigger)
			ctx->is_seninf_error_trigger = mtk_cam_seninf_dump_current_status(ctx->seninf,
				true);
		else
			ctx->is_seninf_error_trigger = mtk_cam_seninf_dump_current_status(ctx->seninf,
				false);

	}

	/* dump camsv debug data */
	mtk_cam_sv_debug_dump(sv_dev, data->err_tags);

	/* check dma fifo status */
	if (!(data->err_tags) && (err_status & CAMSVCENTRAL_DMA_SRAM_FULL_ST)) {
		if (camsv_fifo_detect)
			mtk_cam_sv_execute_fifo_dump(sv_dev, data->ts_ns);

		dev_info(sv_dev->dev, "camsv dma fifo full\n");

#if !IS_ENABLED(CONFIG_MTK_EMI_LEGACY)
		mtk_emiisu_record_off();
#endif
		atomic_set(&sv_dev->is_fifo_full, 1);

		if (DISABLE_RECOVER_FLOW) {
			mtk_cam_bwr_dbg_dump(sv_dev->cam->bwr);
			mmdvfs_debug_status_dump(NULL);
#if KERNEL_VERSION(6, 7, 0) >= LINUX_VERSION_CODE
			mmqos_hrt_dump();
#endif
			if (atomic_read(&sv_dev->is_seamless))
				mtk_cam_ctrl_dump_request(sv_dev->cam, CAMSYS_ENGINE_CAMSV, sv_dev->id,
					frame_idx_inner, MSG_CAMSV_SEAMLESS_ERROR);
			else
				mtk_cam_ctrl_dump_request(sv_dev->cam, CAMSYS_ENGINE_CAMSV, sv_dev->id,
					frame_idx_inner, MSG_CAMSV_ERROR);
		}


		mtk_cam_ctrl_notify_hw_hang(sv_dev->cam,
			CAMSYS_ENGINE_CAMSV, sv_dev->id, frame_idx_inner);
	}
}

bool is_all_tag_setting_to_inner(struct mtk_camsv_device *sv_dev,
	unsigned int frm_seq_no_inner)
{
	unsigned int i, j, grp_seq_no_inner;

	for (i = 0; i < MAX_SV_HW_GROUPS; i++) {
		if (!sv_dev->active_group_info[i])
			continue;
		for (j = 0; j < CAMSV_MAX_TAGS; j++) {
			if (sv_dev->active_group_info[i] & BIT(j)) {
				grp_seq_no_inner =
					readl_relaxed(sv_dev->base_inner +
						REG_CAMSVCENTRAL_FH_SPARE_TAG_1 +
						CAMSVCENTRAL_FH_SPARE_SHIFT * j);
				if (grp_seq_no_inner != frm_seq_no_inner)
					return false;
				break;
			}
		}
	}

	return true;
}

static irqreturn_t mtk_irq_camsv_hybrid(int irq, void *data)
{
	struct mtk_camsv_device *sv_dev = (struct mtk_camsv_device *)data;
	struct mtk_camsys_irq_info irq_info;
	unsigned int frm_seq_no, frm_seq_no_inner;
	unsigned int i, err_status, done_status, cq_status;
	unsigned int first_tag, addr_frm_seq_no = REG_CAMSVCENTRAL_FH_SPARE_TAG_1;
	bool wake_thread = false;

	memset(&irq_info, 0, sizeof(irq_info));

	first_tag =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FIRST_TAG);
	for (i = 0; i < CAMSV_MAX_TAGS; i++) {
		if (first_tag & (1 << i)) {
			addr_frm_seq_no = REG_CAMSVCENTRAL_FH_SPARE_TAG_1 +
				CAMSVCENTRAL_FH_SPARE_SHIFT * i;
			break;
		}
	}

	frm_seq_no =
		readl_relaxed(sv_dev->base + addr_frm_seq_no);
	frm_seq_no_inner =
		readl_relaxed(sv_dev->base_inner + addr_frm_seq_no);
	err_status =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_ERR_STATUS);
	done_status	=
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_DONE_STATUS);
	cq_status =
		readl_relaxed(sv_dev->base_scq + REG_CAMSVCQTOP_INT_0_STATUS);

	irq_info.ts_ns = ktime_get_boottime_ns();
	irq_info.frame_idx = frm_seq_no;
	irq_info.frame_idx_inner = frm_seq_no_inner;

	if (err_status) {
		dev_dbg(sv_dev->dev, "camsv-%d: error status:0x%x seq_no:%d_%d",
			sv_dev->id, err_status,
			frm_seq_no_inner, frm_seq_no);

		irq_info.irq_type |= (1 << CAMSYS_IRQ_ERROR);
		irq_info.e.err_status = err_status;

		if (err_status & ERR_ST_MASK_TAG1_ERR)
			irq_info.err_tags |= (1 << SVTAG_0);
		if (err_status & ERR_ST_MASK_TAG2_ERR)
			irq_info.err_tags |= (1 << SVTAG_1);
		if (err_status & ERR_ST_MASK_TAG3_ERR)
			irq_info.err_tags |= (1 << SVTAG_2);
		if (err_status & ERR_ST_MASK_TAG4_ERR)
			irq_info.err_tags |= (1 << SVTAG_3);
		if (err_status & ERR_ST_MASK_TAG5_ERR)
			irq_info.err_tags |= (1 << SVTAG_4);
		if (err_status & ERR_ST_MASK_TAG6_ERR)
			irq_info.err_tags |= (1 << SVTAG_5);
		if (err_status & ERR_ST_MASK_TAG7_ERR)
			irq_info.err_tags |= (1 << SVTAG_6);
		if (err_status & ERR_ST_MASK_TAG8_ERR)
			irq_info.err_tags |= (1 << SVTAG_7);

		trace_camsv_irq_err(sv_dev->dev, frm_seq_no_inner, frm_seq_no,
					err_status);
	}

	if (done_status) {
		dev_dbg(sv_dev->dev, "camsv-%d: done status:0x%x seq_no:0x%x_0x%x",
			sv_dev->id, done_status, frm_seq_no_inner, frm_seq_no);
		sv_dev->camsv_error_count = 0;
		irq_info.irq_type |= (1 << CAMSYS_IRQ_FRAME_DONE);
		if (done_status & CAMSVCENTRAL_SW_GP_PASS1_DONE_0_ST)
			irq_info.done_tags |= sv_dev->active_group_info[0];
		if (done_status & CAMSVCENTRAL_SW_GP_PASS1_DONE_1_ST)
			irq_info.done_tags |= sv_dev->active_group_info[1];
		if (done_status & CAMSVCENTRAL_SW_GP_PASS1_DONE_2_ST)
			irq_info.done_tags |= sv_dev->active_group_info[2];
		if (done_status & CAMSVCENTRAL_SW_GP_PASS1_DONE_3_ST)
			irq_info.done_tags |= sv_dev->active_group_info[3];
		trace_camsv_irq_done(sv_dev->dev, frm_seq_no_inner, frm_seq_no,
					 done_status);
	}

	if (cq_status) {
		if (cq_status & ERR_ST_MASK_CQ_ERR) {
			dev_dbg(sv_dev->dev, "camsv-%d: cq error status:0x%x seq_no:%d_%d",
				sv_dev->id, cq_status,
				frm_seq_no_inner, frm_seq_no);

			irq_info.irq_type |= (1 << CAMSYS_IRQ_ERROR);
			irq_info.e.err_status2 = cq_status & ERR_ST_MASK_CQ_ERR;
		}

		if (cq_status & CAMSVCQTOP_SCQ_SUB_THR_DONE) {
			dev_dbg(sv_dev->dev, "camsv-%d: cq done status:0x%x seq_no:%d_%d",
				sv_dev->id, cq_status,
			frm_seq_no_inner, frm_seq_no);
			if (sv_dev->cq_ref != NULL) {
				long mask = bit_map_bit(MAP_HW_CAMSV, sv_dev->id);

				if (engine_handle_cq_done(&sv_dev->cq_ref, mask))
					irq_info.irq_type |= (1 << CAMSYS_IRQ_SETTING_DONE);
			}
		}

		trace_camsv_irq_cq_done(sv_dev->dev, frm_seq_no_inner, frm_seq_no,
					cq_status);
	}

	if (irq_info.irq_type && push_msgfifo(sv_dev, &irq_info) == 0)
		wake_thread = true;

	return wake_thread ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static irqreturn_t mtk_irq_camsv_sof(int irq, void *data)
{
	struct mtk_camsv_device *sv_dev = (struct mtk_camsv_device *)data;
	struct mtk_camsys_irq_info irq_info;
	unsigned int frm_seq_no, frm_seq_no_inner;
	unsigned int tg_cnt, enable_tags = 0;
	unsigned int irq_sof_status, irq_channel_status, i, m;
	unsigned int addr_frm_seq_no = REG_CAMSVCENTRAL_FH_SPARE_TAG_1;
	bool wake_thread = false;

	memset(&irq_info, 0, sizeof(irq_info));

	sv_dev->first_tag =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FIRST_TAG);
	sv_dev->last_tag =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_LAST_TAG);
	for (i = 0; i < CAMSV_MAX_TAGS; i++) {
		if (sv_dev->first_tag & (1 << i)) {
			addr_frm_seq_no = REG_CAMSVCENTRAL_FH_SPARE_TAG_1 +
				CAMSVCENTRAL_FH_SPARE_SHIFT * i;
			break;
		}
	}

	frm_seq_no =
		readl_relaxed(sv_dev->base + addr_frm_seq_no);
	frm_seq_no_inner =
		readl_relaxed(sv_dev->base_inner + addr_frm_seq_no);
	irq_sof_status =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_SOF_STATUS);
	irq_channel_status =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_CHANNEL_STATUS);
	for (i = 0; i < MAX_SV_HW_GROUPS; i++) {
		sv_dev->active_group_info[i] =
			readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_GROUP_TAG0 +
				CAMSVCENTRAL_GROUP_TAG_SHIFT * i);
		enable_tags |= sv_dev->active_group_info[i];
	}

	tg_cnt =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_VF_ST_TAG1 +
				CAMSVCENTRAL_VF_ST_TAG_SHIFT * 3);
	tg_cnt = (sv_dev->tg_cnt & 0xffffff00) + ((tg_cnt & 0xff000000) >> 24);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(sv_dev->dev, "camsv-%d: sof status:0x%x channel status:0x%x seq_no:0x%x_0x%x group_tags:0x%x_%x_%x_%x first_tag:0x%x last_tag:0x%x tg_cnt:%d/%lld dcif sel/set:0x%x_%x",
		sv_dev->id, irq_sof_status,
		irq_channel_status,
		frm_seq_no_inner, frm_seq_no,
		sv_dev->active_group_info[0],
		sv_dev->active_group_info[1],
		sv_dev->active_group_info[2],
		sv_dev->active_group_info[3],
		sv_dev->first_tag,
		sv_dev->last_tag,
		tg_cnt, sv_dev->sof_count,
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_DCIF_SEL),
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_DCIF_SET));

	irq_info.ts_ns = ktime_get_boottime_ns();
	irq_info.frame_idx = frm_seq_no;
	irq_info.frame_idx_inner = frm_seq_no_inner;
	irq_info.tg_cnt = sv_dev->tg_cnt;
	sv_dev->sof_count++;
	/* sof */
	for (i = SVTAG_START; i < SVTAG_END; i++) {
		m = i * CAMSVCENTRAL_SOF_BIT_OFFSET + CAMSVCENTRAL_SOF_BIT_START;
		if (irq_sof_status & BIT(m))
			irq_info.sof_tags |= (1 << i);
	}
	if (irq_info.sof_tags)
		irq_info.irq_type |= (1 << CAMSYS_IRQ_FRAME_START);
	irq_info.fbc_empty = (irq_info.sof_tags & sv_dev->last_tag) ?
		mtk_cam_sv_is_zero_fbc_cnt(sv_dev, enable_tags) : 0;

	/* sw enq error */
	for (i = SVTAG_START; i < SVTAG_END; i++) {
		m = i * CAMSVCENTRAL_SW_ENQ_ERR_BIT_OFFSET +
			CAMSVCENTRAL_SW_ENQ_ERR_BIT_START;
		if (irq_channel_status & BIT(m))
			irq_info.done_tags |= (1 << i);
	}
	if (irq_info.done_tags)
		irq_info.irq_type |= (1 << CAMSYS_IRQ_FRAME_DROP);

	if (tg_cnt < sv_dev->tg_cnt)
		sv_dev->tg_cnt = tg_cnt + BIT(8);
	else
		sv_dev->tg_cnt = tg_cnt;
	if (CAM_DEBUG_ENABLED(EXTISP_SW_CNT))
		irq_info.tg_cnt = sv_dev->sof_count - 1;

	if ((irq_info.sof_tags & sv_dev->last_tag) &&
		is_all_tag_setting_to_inner(sv_dev, frm_seq_no_inner)) {

		engine_handle_sof(&sv_dev->cq_ref,
				  bit_map_bit(MAP_HW_CAMSV, sv_dev->id),
				  irq_info.frame_idx_inner);

		if (debug_ddren_camsv_hw_mode && atomic_read(&sv_dev->is_sw_clr) < 2) {
			atomic_inc(&sv_dev->is_sw_clr);
			if (atomic_read(&sv_dev->is_sw_clr) == 2) {
				CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_DDR_CFG,
					CAMSVCENTRAL_DDR_CFG, DDR_CLEAR, 1);
				CAMSV_WRITE_BITS(sv_dev->base + REG_CAMSVCENTRAL_BW_QOS_CFG,
					CAMSVCENTRAL_BW_QOS_CFG, BW_QOS_CLEAR, 1);
				dev_dbg(sv_dev->dev,"camsv do swclr");
			}
		}
	}

	if (irq_info.irq_type && push_msgfifo(sv_dev, &irq_info) == 0)
		wake_thread = true;

	trace_camsv_irq_sof(sv_dev->dev,
			    frm_seq_no_inner, frm_seq_no,
			    irq_sof_status,
			    irq_channel_status,
			    sv_dev->active_group_info,
			    sv_dev->first_tag,
			    sv_dev->last_tag,
			    tg_cnt);

	return wake_thread ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static irqreturn_t mtk_irq_camsv_debug(int irq, void *data)
{
	struct mtk_camsv_device *sv_dev = (struct mtk_camsv_device *)data;
	struct mtk_camsys_irq_info irq_info;
	unsigned int frm_seq_no, frm_seq_no_inner;
	unsigned int i, first_tag, common_status;
	unsigned int exp_0_bid = 0, exp_1_bid = 0;
	unsigned int addr_frm_seq_no = REG_CAMSVCENTRAL_FH_SPARE_TAG_1;
	bool wake_thread = false;

	memset(&irq_info, 0, sizeof(irq_info));
	irq_info.ts_ns = ktime_get_boottime_ns();

	first_tag =
		readl_relaxed(sv_dev->base_inner + REG_CAMSVCENTRAL_FIRST_TAG);
	for (i = 0; i < CAMSV_MAX_TAGS; i++) {
		if (first_tag & (1 << i)) {
			addr_frm_seq_no = REG_CAMSVCENTRAL_FH_SPARE_TAG_1 +
				CAMSVCENTRAL_FH_SPARE_SHIFT * i;
			break;
		}
	}

	frm_seq_no =
		readl_relaxed(sv_dev->base + addr_frm_seq_no);
	frm_seq_no_inner =
		readl_relaxed(sv_dev->base_inner + addr_frm_seq_no);

	irq_info.frame_idx = frm_seq_no;
	irq_info.frame_idx_inner = frm_seq_no_inner;

	common_status =
		readl_relaxed(sv_dev->base + REG_CAMSVCENTRAL_COMMON_STATUS);

	if (CAM_DEBUG_ENABLED(RAW_INT))
		dev_info(sv_dev->dev, "camsv-%d: common_status:0x%x, frm_seq_no:0x%x/0x%x, ts:%llu\n",
			sv_dev->id, common_status, frm_seq_no, frm_seq_no_inner, irq_info.ts_ns);

	if (first_tag) {
		exp_1_bid = CAMSVCENTRAL_DBG_INT_BIT_START +
			(CAMSVCENTRAL_DBG_INT_BIT_OFFSET * (ffs(first_tag) - 1));
		exp_0_bid = exp_1_bid + 1;

		/* exp0 */
		if (common_status & BIT(exp_0_bid) &&
				!sv_dev->is_skip_raw_unlock_done)
			irq_info.irq_type |= (1 << CAMSYS_IRQ_TUNING_UPDATE);

		/* exp1 */
		if (common_status & BIT(exp_1_bid)) {
			if (sv_dev->is_skip_raw_unlock_done)
				sv_dev->is_skip_raw_unlock_done = false;
			else if (sv_dev->ois_updated_seq < frm_seq_no_inner) {
				writel_relaxed(0, sv_dev->raw_lock_done_sel);
				sv_dev->ois_updated_seq = frm_seq_no_inner;
			}
		}
	}

	if (irq_info.irq_type && push_msgfifo(sv_dev, &irq_info) == 0)
		wake_thread = true;

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

		if (CAM_DEBUG_ENABLED(CTRL))
			dev_info(sv_dev->dev, "ts=%llu irq_type %d, req:0x%x/0x%x, tg_cnt:%d\n",
			irq_info.ts_ns / 1000,
			irq_info.irq_type,
			irq_info.frame_idx_inner,
			irq_info.frame_idx,
			irq_info.tg_cnt);

		/* error case */
		if (unlikely(irq_info.irq_type == (1 << CAMSYS_IRQ_ERROR)) &&
			irq_info.e.err_status != 0) {
			camsv_handle_err(sv_dev, &irq_info);
			continue;
		}

		/* error cq case */
		if (unlikely(irq_info.irq_type == (1 << CAMSYS_IRQ_ERROR)) &&
			irq_info.e.err_status2 != 0) {
			camsv_handle_cq_err(sv_dev, &irq_info);
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
	int ret;

	dev_info_ratelimited(dev, "- %s\n", __func__);

	if (pm_runtime_suspended(dev))
		return 0;

	/* Force ISP HW to idle */
	ret = pm_runtime_force_suspend(dev);
	return ret;
}

static int mtk_camsv_pm_resume(struct device *dev)
{
	int ret;

	dev_info_ratelimited(dev, "- %s\n", __func__);

	if (pm_runtime_suspended(dev))
		return 0;

	/* Force ISP HW to resume */
	ret = pm_runtime_force_resume(dev);
	if (ret)
		return ret;

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
	struct device_node *iommu_node;
	struct of_phandle_args args;
	struct device_link *link;
	struct resource *res;
	unsigned int i, j;
	int ret, num_clks, num_larbs, num_iommus, num_ports, smmus;
	unsigned int larb_idx = 0;
	unsigned int raw_lock_sel_addr = 0;

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

	sv_dev->base_dma_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(sv_dev->base_dma_inner)) {
		dev_dbg(dev, "failed to map register inner base dma\n");
		return PTR_ERR(sv_dev->base_dma_inner);
	}
	dev_dbg(dev, "camsv, map_addr(inner dma)=0x%pK\n", sv_dev->base_dma_inner);

	/* base inner scq register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "inner_base_SCQ");
	if (!res) {
		dev_dbg(dev, "failed to get mem\n");
		return -ENODEV;
	}

	sv_dev->base_scq_inner = devm_ioremap_resource(dev, res);
	if (IS_ERR(sv_dev->base_scq_inner)) {
		dev_dbg(dev, "failed to map register inner base\n");
		return PTR_ERR(sv_dev->base_scq_inner);
	}
	dev_dbg(dev, "camsv, map_addr(inner scq)=0x%pK\n", sv_dev->base_scq_inner);

	for (i = 0; i < CAMSV_IRQ_NUM; i++) {
		sv_dev->irq[i] = platform_get_irq(pdev, i);
		if (!sv_dev->irq[i]) {
			dev_dbg(dev, "failed to get irq\n");
			return -ENODEV;
		}
	}

	/* ois compensation */
	if (sv_dev->id < GET_PLAT_V4L2(raw_pipeline_num)) {
		CALL_PLAT_V4L2(
			get_raw_lock_sel_addr, sv_dev->id, &raw_lock_sel_addr);

		sv_dev->is_skip_raw_unlock_done = false;
		if (raw_lock_sel_addr)
			sv_dev->raw_lock_done_sel =
				ioremap(raw_lock_sel_addr, 0x4);
		else
			sv_dev->raw_lock_done_sel = NULL;

	} else {
		sv_dev->is_skip_raw_unlock_done = false;
		sv_dev->raw_lock_done_sel = NULL;
	}

	for (i = 0; i < CAMSV_IRQ_NUM; i++) {
		if (i == 0)
			ret = devm_request_threaded_irq(dev, sv_dev->irq[i],
						mtk_irq_camsv_hybrid,
						mtk_thread_irq_camsv,
						0, dev_name(dev), sv_dev);
		else if (i == 1)
			ret = devm_request_threaded_irq(dev, sv_dev->irq[i],
						mtk_irq_camsv_sof,
						mtk_thread_irq_camsv,
						0, dev_name(dev), sv_dev);
		else
			ret = devm_request_threaded_irq(dev, sv_dev->irq[i],
						mtk_irq_camsv_debug,
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

	num_clks = of_count_phandle_with_args(pdev->dev.of_node, "clocks",
			"#clock-cells");

	sv_dev->num_clks = (num_clks < 0) ? 0 : num_clks;
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

	num_larbs = of_count_phandle_with_args(
					pdev->dev.of_node, "mediatek,larbs", NULL);
	num_larbs = (num_larbs < 0) ? 0 : num_larbs;
	dev_info(dev, "larb_num:%d\n", num_larbs);

	for (i = 0; i < num_larbs; i++) {
		larb_node = of_parse_phandle(
					pdev->dev.of_node, "mediatek,larbs", i);
		if (!larb_node) {
			dev_info(dev, "failed to get larb node\n");
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
		else
			sv_dev->larb_pdev = larb_pdev;
	}

	num_iommus = of_property_count_strings(
					pdev->dev.of_node, "mediatek,larb-node-names");
	num_iommus = (num_iommus < 0) ? 0 : num_iommus;
	dev_info(dev, "iommu_num:%d\n", num_iommus);

	for (i = 0; i < num_iommus; i++) {
		const char *node_name;

		ret = of_property_read_string_index(
					pdev->dev.of_node, "mediatek,larb-node-names",
					i, &node_name);
		if (ret) {
			dev_info(dev, "failed to read larb node name(%d)\n", i);
			continue;
		}
		iommu_node = of_find_node_by_name(NULL, node_name);
		if (!iommu_node) {
			dev_info(dev, "failed to get iommu node(%s)\n", node_name);
			continue;
		}

		num_ports = of_count_phandle_with_args(
						iommu_node, "iommus", "#iommu-cells");
		num_ports = (num_ports < 0) ? 0 : num_ports;
		dev_info(dev, "port_num:%d\n", num_ports);

		for (j = 0; j < num_ports; j++) {
			if (!of_parse_phandle_with_args(iommu_node,
				"iommus", "#iommu-cells", j, &args)) {
				mtk_iommu_register_fault_callback(
					args.args[0],
					mtk_camsv_translation_fault_callback,
					(void *)sv_dev, false);
				sv_dev->larb_master_id[larb_idx++] = args.args[0];
			}

		}
	}

	num_ports = of_count_phandle_with_args(
					pdev->dev.of_node, "iommus", "#iommu-cells");
	num_ports = (num_ports < 0) ? 0 : num_ports;
	dev_info(dev, "port_num:%d\n", num_ports);

	for (i = 0; i < num_ports; i++) {
		if (!of_parse_phandle_with_args(pdev->dev.of_node,
			"iommus", "#iommu-cells", i, &args)) {
			mtk_iommu_register_fault_callback(
				args.args[0],
				mtk_camsv_translation_fault_callback,
				(void *)sv_dev, false);
			sv_dev->larb_master_id[i] = args.args[0];
		}
	}

	smmus = of_property_count_u32_elems(
		pdev->dev.of_node, "mediatek,smmu-dma-axid");
	smmus = (smmus > 0) ? smmus : 0;
	dev_info(dev, "smmu_num:%d\n", smmus);
	for (i = 0; i < smmus; i++) {
		u32 axid;

		if (!of_property_read_u32_index(
			pdev->dev.of_node, "mediatek,smmu-dma-axid", i, &axid)) {
			mtk_iommu_register_fault_callback(
				axid, mtk_camsv_translation_fault_callback,
				(void *)sv_dev, false);
			sv_dev->larb_master_id[i] = axid;
		}
	}
#ifdef CONFIG_PM_SLEEP
	sv_dev->notifier_blk.notifier_call = mtk_camsv_suspend_pm_event;
	sv_dev->notifier_blk.priority = 0;
	ret = register_pm_notifier(&sv_dev->notifier_blk);
	if (ret) {
		dev_info(dev, "Failed to register PM notifier");
		return -ENODEV;
	}
#endif
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
	unsigned int is_two_smi_out = 0;
	int ret;

	sv_dev = devm_kzalloc(dev, sizeof(*sv_dev), GFP_KERNEL);
	if (!sv_dev)
		return -ENOMEM;

	sv_dev->dev = dev;
	dev_set_drvdata(dev, sv_dev);

	ret = mtk_camsv_of_probe(pdev, sv_dev);
	if (ret)
		return ret;

	CALL_PLAT_V4L2(
		get_sv_smi_setting, sv_dev->id, &is_two_smi_out);
	if (is_two_smi_out) {
		ret = mtk_cam_qos_probe(dev, &sv_dev->qos, SMI_PORT_SV_TYPE0_NUM);
		if (ret)
			goto UNREGISTER_PM_NOTIFIER;
	} else {
		ret = mtk_cam_qos_probe(dev, &sv_dev->qos, SMI_PORT_SV_TYPE1_NUM);
		if (ret)
			goto UNREGISTER_PM_NOTIFIER;
	}

	sv_dev->fifo_size =
		roundup_pow_of_two(8 * sizeof(struct mtk_camsys_irq_info));
	sv_dev->msg_buffer = devm_kzalloc(dev, sv_dev->fifo_size,
					     GFP_KERNEL);
	if (!sv_dev->msg_buffer) {
		ret = -ENOMEM;
		goto UNREGISTER_PM_NOTIFIER;
	}

	pm_runtime_enable(dev);

	ret = component_add(dev, &mtk_camsv_component_ops);

	if (ret)
		goto UNREGISTER_PM_NOTIFIER;

	return ret;

UNREGISTER_PM_NOTIFIER:
	unregister_pm_notifier(&sv_dev->notifier_blk);
	return ret;
}

static int mtk_camsv_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_camsv_device *sv_dev = dev_get_drvdata(dev);

#ifdef CONFIG_PM_SLEEP
	unregister_pm_notifier(&sv_dev->notifier_blk);
#endif
	pm_runtime_disable(dev);

	mtk_cam_qos_remove(&sv_dev->qos);

	component_del(dev, &mtk_camsv_component_ops);
	return 0;
}

int mtk_camsv_runtime_suspend(struct device *dev)
{
	struct mtk_camsv_device *sv_dev = dev_get_drvdata(dev);
	int i, ret = 0;

	dev_dbg(dev, "%s:disable clock\n", __func__);

	if (camsv_fifo_detect) {
		if (atomic_read(&sv_dev->enable_fifo_detect))
			ret |= mtk_cam_sv_stop_fifo_detection(sv_dev);
		if (ret)
			dev_info(dev, "disable fifo_detection fail\n");
	}

	mtk_cam_reset_qos(dev, &sv_dev->qos);

	mtk_cam_bwr_set_chn_bw(sv_dev->cam->bwr,
		get_sv_bwr_engine(sv_dev->id), get_sv_axi_port(sv_dev->id),
		0, -sv_dev->sv_avg_applied_bw_w,
		0, -sv_dev->sv_peak_applied_bw_w, false);

	mtk_cam_bwr_set_ttl_bw(sv_dev->cam->bwr,
		get_sv_bwr_engine(sv_dev->id), -sv_dev->sv_avg_applied_bw_w,
		-sv_dev->sv_peak_applied_bw_w, false);

	sv_dev->sv_avg_applied_bw_w = 0;
	sv_dev->sv_peak_applied_bw_w = 0;

	mtk_cam_sv_golden_set(sv_dev, false);

	for (i = sv_dev->num_clks - 1; i >= 0; i--)
		clk_disable_unprepare(sv_dev->clks[i]);

	return 0;
}

int mtk_camsv_runtime_resume(struct device *dev)
{
	struct mtk_camsv_device *sv_dev = dev_get_drvdata(dev);
	int i, ret;

	/* reset_msgfifo before enable_irq */
	ret = mtk_cam_sv_reset_msgfifo(sv_dev);
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

	if (camsv_fifo_detect) {
		if (atomic_read(&sv_dev->enable_fifo_detect))
			ret |= mtk_cam_sv_start_fifo_detection(sv_dev);
		if (ret)
			dev_info(dev, "enable fifo_detection fail\n");
	}

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

