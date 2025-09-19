/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_SV_H
#define __MTK_CAM_SV_H

#include <linux/kfifo.h>
#include <linux/suspend.h>

#include "mtk_cam-engine.h"

#define MAX_SV_HW_TAGS 8
#define MAX_SV_HW_GROUPS 4
#define CAMSV_IRQ_NUM 4

/* refer to ipi hw path definition */
enum mtkcam_sv_hw_path_control {
	MTKCAM_SV_SPECIAL_SCENARIO_ADDITIONAL_RAW = 24,
	MTKCAM_SV_SPECIAL_SCENARIO_EXT_ISP,
	MTKCAM_SV_SPECIAL_SCENARIO_DISPLAY_IC
};

#define CAMSV_EXT_META_0_WIDTH 1024
#define CAMSV_EXT_META_0_HEIGHT 1024
#define CAMSV_EXT_META_1_WIDTH 1024
#define CAMSV_EXT_META_1_HEIGHT 1024
#define CAMSV_EXT_META_2_WIDTH 1024
#define CAMSV_EXT_META_2_HEIGHT 1024

#define MTK_CAMSV_SUPPORTED_SPECIAL_HW_SCENARIO	(\
			(1 << MTKCAM_SV_SPECIAL_SCENARIO_ADDITIONAL_RAW) |\
			(1 << MTKCAM_SV_SPECIAL_SCENARIO_EXT_ISP) |\
			(1 << MTKCAM_SV_SPECIAL_SCENARIO_DISPLAY_IC) |\
			(1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_STAGGER)) |\
			(1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_DC_STAGGER)) |\
			(1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_OFFLINE_STAGGER)) |\
			(1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_OFFLINE)))

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

struct mtk_cam_ctx;

enum camsv_function_id {
	DISPLAY_IC = (1 << 0)
};

enum camsv_module_id {
	CAMSV_START = 0,
	CAMSV_0 = CAMSV_START,
	CAMSV_1 = 1,
	CAMSV_2 = 2,
	CAMSV_3 = 3,
	CAMSV_4 = 4,
	CAMSV_5 = 5,
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
	SV_INT_EN_INT_WCLR_EN              = (1L<<31),
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

struct mtk_camsv_tag_param {
	unsigned int tag_idx;
	unsigned int seninf_padidx;
	unsigned int tag_order;
	bool is_w;
};

struct mtk_camsv_tag_info {
	struct mtk_camsv_pipeline *sv_pipe;
	unsigned int seninf_padidx;
	unsigned int hw_scen;
	unsigned int tag_order;
	struct mtkcam_ipi_input_param cfg_in_param;
};

struct mtk_camsv_device {
	struct device *dev;
	struct mtk_cam_device *cam;
	unsigned int id;
	int irq[CAMSV_IRQ_NUM];
	void __iomem *base;
	void __iomem *base_inner;
	void __iomem *base_dma;
	void __iomem *base_inner_dma;
	void __iomem *base_scq;
	void __iomem *base_inner_scq;
	unsigned int num_clks;
	struct clk **clks;
	unsigned int cammux_id;
	unsigned int used_tag_cnt;
	unsigned int streaming_tag_cnt;
	unsigned int enabled_tags;
	struct mtk_camsv_tag_info tag_info[MAX_SV_HW_TAGS];
	unsigned int active_group_info[MAX_SV_HW_GROUPS];
	unsigned int enque_tags;
	unsigned int first_tag;
	unsigned int last_tag;
	unsigned int last_done_tag;

	unsigned int group_handled;
	unsigned int used_group;

	int fifo_size;
	void *msg_buffer;
	struct kfifo msg_fifo;
	atomic_t is_fifo_overflow;

	struct engine_fsm fsm;
	struct apply_cq_ref *cq_ref;

	unsigned int sof_count;
	/* for preisp - for sof counter sync.*/
	int tg_cnt;
	unsigned int frame_wait_to_process;
	struct notifier_block notifier_blk;
	u64 sof_timestamp;
};

void sv_reset(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_dev_config(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_cq_config(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_cq_enable(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_cq_disable(struct mtk_camsv_device *sv_dev);
int mtk_cam_get_sv_cammux_id(struct mtk_camsv_device *sv_dev, int tag_idx);
int mtk_cam_sv_dev_pertag_stream_on(
	struct mtk_camsv_device *sv_dev, unsigned int tag_idx, bool on);
int mtk_cam_sv_dev_stream_on(struct mtk_camsv_device *sv_dev, bool on);
int mtk_cam_sv_dmao_common_config(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_print_fbc_status(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_toggle_tg_db(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_toggle_db(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_central_common_enable(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_central_common_disable(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_fbc_disable(struct mtk_camsv_device *sv_dev, unsigned int tag_idx);
unsigned int mtk_cam_get_sv_tag_index(
	struct mtk_camsv_device *sv_dev, unsigned int pipe_id);
int mtk_cam_sv_dev_pertag_write_rcnt(
	struct mtk_camsv_device *sv_dev, unsigned int tag_idx);
void mtk_cam_sv_vf_reset(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_is_zero_fbc_cnt(struct mtk_camsv_device *sv_dev, unsigned int tag_idx);
void mtk_cam_sv_check_fbc_cnt(
	struct mtk_camsv_device *sv_dev, unsigned int tag_idx);
int mtk_cam_sv_frame_no_inner(struct mtk_camsv_device *sv_dev);
void mtk_cam_sv_fill_tag_info(struct mtk_camsv_tag_info *tag_info,
	struct mtk_camsv_tag_param *tag_param, unsigned int hw_scen,
	unsigned int pixelmode, unsigned int sub_ratio,
	unsigned int mbus_width, unsigned int mbus_height,
	unsigned int mbus_code,	struct mtk_camsv_pipeline *pipeline);
void mtk_cam_sv_reset_tag_info(struct mtk_camsv_device *sv_dev);
int mtk_cam_sv_get_tag_param(struct mtk_camsv_tag_param *arr_tag_param,
	unsigned int hw_scen, unsigned int exp_no, unsigned int req_amount);
void apply_camsv_cq(struct mtk_camsv_device *sv_dev,
	      struct apply_cq_ref *ref,
	      dma_addr_t cq_addr, unsigned int cq_size, unsigned int cq_offset,
	      int initial);
bool mtk_cam_is_display_ic(struct mtk_cam_ctx *ctx);
#ifdef CAMSYS_TF_DUMP_7S
int mtk_camsv_translation_fault_callback(int port, dma_addr_t mva, void *data);
#endif

extern struct platform_driver mtk_cam_sv_driver;

#endif
