/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_MRAW_H
#define __MTK_CAM_MRAW_H

#include <linux/kfifo.h>
#include <linux/suspend.h>

#include "mtk_cam-video.h"
#include "mtk_cam-plat.h"

/* The maximum mraw hw num in the same ISP generation */
#define MRAW_PIPELINE_NUM 4

#define MAX_MRAW_VIDEO_DEV_NUM 2
#define USING_MRAW_SCQ 1
#define CHECK_MRAW_NODEQ 1
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

struct mtk_cam_ctx;
struct mtk_cam_request;

enum mraw_module_id {
	MRAW_START = 0,
	MRAW_0 = MRAW_START,
	MRAW_1 = 1,
	MRAW_2 = 2,
	MRAW_3 = 3,
	MRAW_END
};

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
	MRAW_TG_FMT_RAW8      = 0,
	MRAW_TG_FMT_RAW10     = 1,
	MRAW_TG_FMT_RAW12     = 2,
	MRAW_TG_FMT_YUV422    = 3,
	MRAW_TG_FMT_RAW14     = 4,
	MRAW_TG_FMT_RMRAW1      = 5,
	MRAW_TG_FMT_RMRAW2      = 6,
	MRAW_TG_FMT_JPG       = 7,
};

enum mraw_tg_swap {
	MRAW_TG_SW_UYVY = 0,
	MRAW_TG_SW_YUYV = 1,
	MRAW_TG_SW_VYUY = 2,
	MRAW_TG_SW_YVYU = 3,
};

/* enum for pads of mraw pipeline */
enum {
	MTK_MRAW_SINK_BEGIN = 0,
	MTK_MRAW_SINK = MTK_MRAW_SINK_BEGIN,
	MTK_MRAW_SINK_NUM,
	MTK_MRAW_META_IN = MTK_MRAW_SINK_NUM,
	MTK_MRAW_SOURCE_BEGIN,
	MTK_MRAW_META_OUT_BEGIN = MTK_MRAW_SOURCE_BEGIN,
	MTK_MRAW_META_OUT = MTK_MRAW_META_OUT_BEGIN,
	MTK_MRAW_PIPELINE_PADS_NUM,
};

enum mqe_mode {
	UL_MODE = 0,
	UR_MODE,
	DL_MODE,
	DR_MODE,
	PD_L_MODE,
	PD_R_MODE,
	PD_M_MODE,
	PD_B01_MODE,
	PD_B02_MODE,
};

enum mbn_dir {
	MBN_POW_VERTICAL = 0,
	MBN_POW_HORIZONTAL,
	MBN_POW_SPARSE_CONCATENATION,
	MBN_POW_SPARSE_INTERLEVING,
};

enum cpi_dir {
	CPI_POW_VERTICAL = 0,
	CPI_POW_HORIZONTAL,
	CPI_POW_SPARSE_CONCATENATION,
	CPI_POW_SPARSE_INTERLEVING,
};

#define MTK_MRAW_TOTAL_NODES (MTK_MRAW_PIPELINE_PADS_NUM - MTK_MRAW_SINK_NUM)

struct mtk_mraw_pad_config {
	struct v4l2_mbus_framefmt mbus_fmt;
};

struct mtk_cam_mraw_resource_config {
	void *vaddr[MAX_MRAW_VIDEO_DEV_NUM];
	__u64 daddr[MAX_MRAW_VIDEO_DEV_NUM];
	__u32 enque_num;
	struct mtkcam_ipi_crop tg_crop;
	__u32 tg_fmt;
	__u32 pixel_mode;
	__u32 subsample;
	__u32 mraw_dma_size[mraw_dmao_num];
	atomic_t is_fmt_change;
	struct mraw_stats_cfg_param stats_cfg_param;
};

struct mtk_mraw_pipeline {
	unsigned int id;
	struct v4l2_subdev subdev;
	struct media_pad pads[MTK_MRAW_PIPELINE_PADS_NUM];
	struct mtk_cam_video_device vdev_nodes[MTK_MRAW_TOTAL_NODES];
	struct mtk_mraw *mraw;
	struct mtk_mraw_pad_config cfg[MTK_MRAW_PIPELINE_PADS_NUM];

	/* cached settings */
	unsigned int enabled_mraw;
	unsigned long long enabled_dmas;
	struct mtk_cam_mraw_resource_config res_config;

	/* seninf pad index */
	u32 seninf_padidx;
	unsigned int cammux_id;

	unsigned int req_pfmt_update;
	struct v4l2_subdev_format req_pad_fmt[MTK_MRAW_PIPELINE_PADS_NUM];
};

struct mtk_mraw_device {
	struct device *dev;
	struct mtk_cam_device *cam;
	unsigned int id;
	int irq;
	void __iomem *base;
	void __iomem *base_inner;
	unsigned int num_clks;
	struct clk **clks;
	struct mtk_mraw_pipeline *pipeline;
	unsigned int cammux_id;

	int fifo_size;
	void *msg_buffer;
	struct kfifo msg_fifo;
	atomic_t is_fifo_overflow;

	unsigned int sof_count;
	u64 last_sof_time_ns;
	struct notifier_block notifier_blk;

	atomic_t is_enqueued;
	atomic_t is_first_frame;
#ifdef CHECK_MRAW_NODEQ
	u64 last_wcnt;
	u64 wcnt_no_dup_cnt;
#endif
};

struct mtk_mraw {
	struct device *cam_dev;
	struct device *devs[MRAW_PIPELINE_NUM];
	struct mtk_mraw_pipeline pipelines[MRAW_PIPELINE_NUM];
};

struct mtk_larb;
int mtk_mraw_register_entities(
	struct mtk_mraw *mraw, struct v4l2_device *v4l2_dev);
void mtk_mraw_unregister_entities(struct mtk_mraw *mraw);
int mtk_mraw_call_pending_set_fmt(struct v4l2_subdev *sd,
				 struct v4l2_subdev_format *fmt);
int mtk_cam_mraw_select(struct mtk_mraw_pipeline *pipe);
void mraw_reset(struct mtk_mraw_device *dev);
int mtk_cam_mraw_pipeline_config(struct mtk_cam_ctx *ctx, unsigned int idx);
struct device *mtk_cam_find_mraw_dev(
	struct mtk_cam_device *cam, unsigned int mraw_mask);
int mtk_cam_mraw_apply_all_buffers(struct mtk_cam_ctx *ctx, int initial);
int mtk_cam_mraw_dev_config(
	struct mtk_cam_ctx *ctx, unsigned int idx);
int mtk_cam_mraw_dev_stream_on(struct mtk_cam_ctx *ctx,
	struct mtk_mraw_device *mraw_dev, unsigned int streaming);
int mtk_cam_mraw_top_config(struct mtk_mraw_device *dev);
int mtk_cam_mraw_dma_config(struct mtk_mraw_device *dev);
int mtk_cam_mraw_fbc_config(struct mtk_mraw_device *dev);
int mtk_cam_mraw_top_enable(struct mtk_mraw_device *dev);
int mtk_cam_mraw_dmao_enable(
	struct mtk_mraw_device *dev);
int mtk_cam_mraw_fbc_enable(
	struct mtk_mraw_device *dev);
int mtk_cam_mraw_tg_disable(struct mtk_mraw_device *dev);
int mtk_cam_mraw_top_disable(struct mtk_mraw_device *dev);
int mtk_cam_mraw_dmao_disable(struct mtk_mraw_device *dev);
int mtk_cam_mraw_fbc_disable(struct mtk_mraw_device *dev);
int mtk_cam_mraw_vf_on(struct mtk_mraw_device *dev, unsigned int is_on);
int mtk_cam_mraw_is_vf_on(struct mtk_mraw_device *dev);
int mtk_cam_mraw_toggle_tg_db(struct mtk_mraw_device *dev);
int mtk_cam_mraw_toggle_db(struct mtk_mraw_device *dev);
bool mtk_cam_mraw_finish_buf(struct mtk_cam_request_stream_data *s_data);
int mtk_cam_find_mraw_dev_index(struct mtk_cam_ctx *ctx, unsigned int idx);
void apply_mraw_cq(struct mtk_mraw_device *dev,
	      dma_addr_t cq_addr, unsigned int cq_size, unsigned int cq_offset,
	      int initial);
int mtk_cam_config_mraw_stats_out(struct mtk_cam_request_stream_data *s_data,
	struct vb2_buffer *vb);
int mtk_cam_mraw_cal_cfg_info(struct mtk_cam_device *cam,
	unsigned int pipe_id, struct mtkcam_ipi_mraw_frame_param *mraw_param);
#ifdef CAMSYS_TF_DUMP_7S
int mtk_mraw_translation_fault_callback(int port, dma_addr_t mva, void *data);
#endif
void mtk_cam_mraw_get_mqe_size(struct mtk_cam_device *cam, unsigned int pipe_id,
	unsigned int *width, unsigned int *height);
void mtk_cam_mraw_get_mbn_size(struct mtk_cam_device *cam, unsigned int pipe_id,
	unsigned int *width, unsigned int *height);
void mtk_cam_mraw_get_cpi_size(struct mtk_cam_device *cam, unsigned int pipe_id,
	unsigned int *width, unsigned int *height);
bool mtk_cam_mraw_is_zero_fbc_cnt(struct mtk_cam_ctx *ctx,
	struct mtk_mraw_device *mraw_dev);
#ifdef CHECK_MRAW_NODEQ
void mraw_check_fbc_no_deque(struct mtk_cam_ctx *ctx,
	struct mtk_mraw_device *mraw_dev,
	int fbc_cnt, int write_cnt, unsigned int dequeued_frame_seq_no);
#endif

extern struct platform_driver mtk_cam_mraw_driver;

#endif
