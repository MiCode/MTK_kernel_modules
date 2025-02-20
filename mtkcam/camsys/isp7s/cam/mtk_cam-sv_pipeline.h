/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_SV_PIPELINE_H
#define __MTK_CAM_SV_PIPELINE_H

#include <media/v4l2-subdev.h>
#include "mtk_cam-video.h"

#define MAX_SV_PIPELINE_NUN 16

#define SV_IMG_MAX_WIDTH	8192
#define SV_IMG_MAX_HEIGHT	6144
#define SV_IMG_MIN_WIDTH	0
#define SV_IMG_MIN_HEIGHT	0

/* enum for pads of camsv pipeline */
enum {
	MTK_CAMSV_SINK_BEGIN = 0,
	MTK_CAMSV_SINK = MTK_CAMSV_SINK_BEGIN,
	MTK_CAMSV_SINK_NUM,

	MTK_CAMSV_SOURCE_BEGIN = MTK_CAMSV_SINK_NUM,
	MTK_CAMSV_MAIN_STREAM_OUT = MTK_CAMSV_SOURCE_BEGIN,
	MTK_CAMSV_EXT_STREAM_OUT,
	MTK_CAMSV_PIPELINE_PADS_NUM,
};

#define MTK_CAMSV_TOTAL_NODES (MTK_CAMSV_PIPELINE_PADS_NUM - MTK_CAMSV_SINK_NUM)

struct mtk_camsv_pad_config {
	struct v4l2_mbus_framefmt mbus_fmt;
};

struct mtk_camsv_pipeline {
	unsigned int id;
	struct v4l2_subdev subdev;
	struct media_pad pads[MTK_CAMSV_PIPELINE_PADS_NUM];
	struct mtk_cam_video_device vdev_nodes[MTK_CAMSV_TOTAL_NODES];
	struct mtk_camsv_pad_config pad_cfg[MTK_CAMSV_PIPELINE_PADS_NUM];

	/* seninf pad index */
	u32 seninf_padidx;

	/* display ic */
	u32 feature_pending;
};

struct mtk_camsv_sink_data {
	unsigned int width;
	unsigned int height;
	unsigned int mbus_code;
	struct v4l2_rect crop;
};

struct mtk_camsv_request_data {
	struct mtk_camsv_sink_data sink;
};

struct mtk_camsv_pipeline *
mtk_camsv_pipeline_create(struct device *dev, int n);
int mtk_camsv_register_entities(struct mtk_camsv_pipeline *arr_pipe,
	int num, struct v4l2_device *v4l2_dev);
void mtk_camsv_unregister_entities(struct mtk_camsv_pipeline *arr_pipe, int num);
int mtk_cam_sv_update_feature(struct mtk_cam_video_device *node);
int mtk_cam_sv_update_image_size(struct mtk_cam_video_device *node, struct v4l2_format *f);

#endif /*__MTK_CAM_SV_PIPELINE_H*/
