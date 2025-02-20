/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_RAW_PIPELINE_H
#define __MTK_CAM_RAW_PIPELINE_H

#include <linux/kfifo.h>
#include <media/v4l2-subdev.h>
#include "mtk_cam-video.h"
#include "mtk_cam-tg-flash.h"
#include "mtk_camera-v4l2-controls-7sp.h"
#include "mtk_cam-raw_pads.h"

/* FIXME: dynamic config image max/min w/h */
#define IMG_MAX_WIDTH		18000
#define IMG_MAX_HEIGHT		16000
#define IMG_MIN_WIDTH		8
#define IMG_MIN_HEIGHT		1
#define YUV_GROUP1_MAX_WIDTH	8192
#define YUV_GROUP1_MAX_HEIGHT	3896
#define YUV_GROUP2_MAX_WIDTH	3060
#define YUV_GROUP2_MAX_HEIGHT	1145
#define YUV1_MAX_WIDTH		8192
#define YUV1_MAX_HEIGHT		2290
#define YUV2_MAX_WIDTH		3060
#define YUV2_MAX_HEIGHT		1145
#define YUV3_MAX_WIDTH		7794
#define YUV3_MAX_HEIGHT		3896
#define YUV4_MAX_WIDTH		1530
#define YUV4_MAX_HEIGHT		572
#define YUV5_MAX_WIDTH		1530
#define YUV5_MAX_HEIGHT		572
#define DRZS4NO1_MAX_WIDTH	2400
#define DRZS4NO1_MAX_HEIGHT	1080
#define DRZS4NO2_MAX_WIDTH	2400
#define DRZS4NO2_MAX_HEIGHT	1080
#define DRZS4NO3_MAX_WIDTH	576
#define DRZS4NO3_MAX_HEIGHT	576
#define RZH1N2TO1_MAX_WIDTH	1280
#define RZH1N2TO1_MAX_HEIGHT	600
#define RZH1N2TO2_MAX_WIDTH	512
#define RZH1N2TO2_MAX_HEIGHT	512
#define RZH1N2TO3_MAX_WIDTH	1280
#define RZH1N2TO3_MAX_HEIGHT	600
#define DRZB2NO1_MAX_WIDTH	5416
#define DRZB2NO1_MAX_HEIGHT	4062
#define IPUO_MAX_WIDTH	2400
#define IPUO_MAX_HEIGHT	1080

#define IMG_PIX_ALIGN		2

#define DUMMY_BUF_SIZE PAGE_SIZE

#define PREISP

#define MTK_RAW_TOTAL_NODES (MTK_RAW_PIPELINE_PADS_NUM - MTK_RAW_SINK_NUM)

struct mtk_raw_pad_config {
	struct v4l2_mbus_framefmt mbus_fmt;
	struct v4l2_rect crop;
};

struct mtk_raw_stagger_select {
	int hw_mode;
	int enabled_raw;
};

struct mtk_cam_resource_driver {

	/* expose to userspace, v4l2_ctrl */
	struct mtk_cam_resource_v2 user_data;

	/* driver internally cached */
	int tgo_pxl_mode;
	int tgo_pxl_mode_before_raw;
};

static inline int bin_ratio(u8 bin_type)
{
	if (bin_type == MTK_CAM_BIN_ON ||
	    bin_type == MTK_CAM_CBN_2X2_ON)
		return 2;

	/* note: MTK_CAM_QBND_ON
	 *   this value is defined as (1 << 8).
	 *   however, bin is in type of __u8...
	 */
#ifdef CAM_QBND_READY
	if (bin_type == MTK_CAM_QBND_ON)
		return 2;
#endif

	if (bin_type == MTK_CAM_CBN_3X3_ON)
		return 3;

	if (bin_type == MTK_CAM_CBN_4X4_ON)
		return 4;

	return 1;
}

struct mtk_raw_ctrl_data_read_clear {
	bool sensor_mode_update;
	bool sensor_update;
};

struct mtk_raw_ctrl_data {
	struct mtk_cam_resource_driver resource;
	int raw_path;

	bool enqueued_tg_flash_req; /* need a better way to collect the request */
	struct mtk_cam_tg_flash_config tg_flash_config;

	s64 sync_id;
	struct mtk_cam_mstream_exposure mstream_exp;
	bool valid_mstream_exp;

	struct mtk_cam_apu_info apu_info;
	bool valid_apu_info;

	struct mtk_cam_req_info req_info;
	bool valid_req_info;

	struct mtk_cam_internal_mem pre_alloc_mem;
	struct dma_buf *pre_alloc_dbuf;

	struct v4l2_subdev *sensor;
	struct v4l2_subdev *seninf;

	u32 enable_hsf_raw;
	u32 trigger_cq_deadline;
	struct mtk_raw_ctrl_data_read_clear rc_data;
};

struct mtk_raw_sink_data {
	unsigned int width;
	unsigned int height;
	unsigned int mbus_code;
	struct v4l2_rect crop;
};

bool is_raw_sink_eq(struct mtk_raw_sink_data *a, struct mtk_raw_sink_data *b);

struct mtk_raw_request_data {
	struct mtk_raw_sink_data sink;
	struct mtk_raw_ctrl_data ctrl;
};

struct slbc_data;
/*
 * struct mtk_raw_pipeline - sub dev to use raws.
 */
struct mtk_raw_pipeline {
	unsigned int id;
	struct v4l2_subdev subdev;
	struct mtk_cam_video_device vdev_nodes[MTK_RAW_TOTAL_NODES];
	struct media_pad pads[MTK_RAW_PIPELINE_PADS_NUM];
	struct mtk_raw_pad_config pad_cfg[MTK_RAW_PIPELINE_PADS_NUM];
	struct v4l2_ctrl_handler ctrl_handler;

	/* special resource management
	 * some resources may be required before stream-on, should handle cases
	 * like sd being closed without stream-on/off.
	 */
	atomic_t open_cnt;
	struct slbc_data *early_request_slb_data;

	/*** v4l2 ctrl related data ***/
	/* changed with request */
	struct mtk_raw_ctrl_data ctrl_data;

	/* vhdr timestamp */
	int hdr_ts_fifo_size;
	void *hdr_ts_buffer;
	struct kfifo hdr_ts_fifo;
	atomic_t is_hdr_ts_fifo_overflow;

	/**
	 * Only cached to save the sensor of a job considering raw switch.
	 * Please don't use it in any other flow.
	 */
	struct v4l2_subdev *sensor;
	struct v4l2_subdev *seninf;
};

static inline struct mtk_raw_pipeline *
mtk_cam_ctrl_handler_to_raw_pipeline(struct v4l2_ctrl_handler *handler)
{
	return container_of(handler, struct mtk_raw_pipeline, ctrl_handler);
};

struct mtk_raw_pipeline *mtk_raw_pipeline_create(struct device *dev, int n);

struct mtk_cam_engines;
int mtk_raw_setup_dependencies(struct mtk_cam_engines *eng);

int mtk_raw_register_entities(struct mtk_raw_pipeline *arr_pipe, int num,
			      struct v4l2_device *v4l2_dev);
void mtk_raw_unregister_entities(struct mtk_raw_pipeline *arr_pipe, int num);

/* choose from seninf or rawi */
struct mtk_raw_pad_config *mtk_raw_current_sink(struct mtk_raw_pipeline *pipe);

/* TODO: move to cam */
struct v4l2_subdev *mtk_cam_find_sensor(struct mtk_cam_ctx *ctx,
					struct media_entity *entity);

/* helper function */
static inline struct mtk_cam_video_device *
mtk_raw_get_node(struct mtk_raw_pipeline *pipe, int pad)
{
	int idx;

	if (WARN_ON(pad < MTK_RAW_SINK_NUM))
		return NULL;

	idx = raw_pad_to_node_idx(pad);
	if (WARN_ON(idx < 0 || idx >= ARRAY_SIZE(pipe->vdev_nodes)))
		return NULL;

	return &pipe->vdev_nodes[idx];
}

/* HDR timestamp */
int mtk_raw_hdr_tsfifo_init(struct mtk_raw_pipeline *arr_pipe, int num);
int mtk_raw_hdr_tsfifo_reset(struct mtk_cam_ctx *ctx);

void mtk_raw_hdr_tsfifo_push(struct mtk_raw_pipeline *pipex,
						struct mtk_cam_hdr_timestamp_info *ts_info);
void mtk_raw_hdr_tsfifo_pop(struct mtk_raw_pipeline *pipe,
						struct mtk_cam_hdr_timestamp_info *ts_info);

void mtk_raw_reset_early_slb(struct mtk_raw_pipeline *pipe);

#endif /*__MTK_CAM_RAW_PIPELINE_H*/
