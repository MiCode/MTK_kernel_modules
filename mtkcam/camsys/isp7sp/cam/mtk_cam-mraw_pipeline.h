/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_MRAW_PIPELINE_H
#define __MTK_CAM_MRAW_PIPELINE_H

#include <media/v4l2-subdev.h>
#include "mtk_cam-video.h"
#include "mtk_cam-plat.h"

#define MRAW_PIPELINE_NUM 4

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

#define MTK_MRAW_TOTAL_NODES (MTK_MRAW_PIPELINE_PADS_NUM - MTK_MRAW_SINK_NUM)

struct mtk_mraw_pad_config {
	struct v4l2_mbus_framefmt mbus_fmt;
};

struct mtk_cam_mraw_resource_config {
	void *vaddr[MTK_MRAW_TOTAL_NODES];
	__u64 daddr[MTK_MRAW_TOTAL_NODES];
	struct mtkcam_ipi_crop tg_crop;
	__u32 tg_fmt;
	__u32 pixel_mode;
	__u32 mraw_dma_size[mraw_dmao_num];
	atomic_t enque_node_num;
	atomic_t is_fmt_change;
	struct mraw_stats_cfg_param stats_cfg_param;
};

struct mtk_mraw_pipeline {
	unsigned int id;
	struct v4l2_subdev subdev;
	struct media_pad pads[MTK_MRAW_PIPELINE_PADS_NUM];
	struct mtk_cam_video_device vdev_nodes[MTK_MRAW_TOTAL_NODES];
	struct mtk_mraw_pad_config pad_cfg[MTK_MRAW_PIPELINE_PADS_NUM];

	/* cached settings */
	struct mtk_cam_mraw_resource_config res_config;

	/* seninf pad index */
	unsigned int seninf_padidx;
};

struct mtk_mraw_sink_data {
	unsigned int width;
	unsigned int height;
	unsigned int mbus_code;
	struct v4l2_rect crop;
};

struct mtk_mraw_request_data {
	struct mtk_mraw_sink_data sink;
};

struct mtk_mraw_pipeline *
mtk_mraw_pipeline_create(struct device *dev, int n);
int mtk_mraw_register_entities(struct mtk_mraw_pipeline *arr_pipe,
	int num, struct v4l2_device *v4l2_dev);
void mtk_mraw_unregister_entities(struct mtk_mraw_pipeline *arr_pipe, int num);

#endif /*__MTK_CAM_MRAW_PIPELINE_H*/

