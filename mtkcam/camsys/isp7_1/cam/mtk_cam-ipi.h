/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_IPI_H__
#define __MTK_CAM_IPI_H__

#define MTK_CAM_IPI_VERSION_MAJOR 0
#define MTK_CAM_IPI_VERSION_MINOR 1

#include <linux/types.h>
#include "mtk_cam-defs.h"


#define MTK_CAM_MAX_RUNNING_JOBS (3)
#define CAM_MAX_PLANENUM (3)
#define CAM_MAX_SUBSAMPLE (32)

/*
 * struct mtkcam_ipi_point - Point
 *
 * @x: x-coordinate of the point (zero-based).
 * @y: y-coordinate of the point (zero-based).
 */
struct mtkcam_ipi_point {
	__u16 x;
	__u16 y;
} __packed;

/*
 * struct mtkcam_ipi_size - Size
 *
 * @w: width (in pixels).
 * @h: height (in pixels).
 */
struct mtkcam_ipi_size {
	__u16 w;
	__u16 h;
} __packed;

/*
 * struct mtkcam_ipi_fract - fraction
 *
 * @numerator: denominator part of the fraction.
 * @denominator: denominator part of the fraction.
 */
struct mtkcam_ipi_fract {
	__u8 numerator;
	__u8 denominator;
};

/*
 * struct mtkcam_ipi_sw_buffer
 *	- Shared buffer between cam-device and co-processor.
 *
 * @iova: DMA address for CAM DMA device.
 * @ccd_fd: fd to ccd
 * @scp_addr: SCP  address for external co-processor unit.
 * @size: buffer size.
 */
struct mtkcam_ipi_sw_buffer {
	__u64 iova;
	union {
		__u32 ccd_fd;
		__u32 scp_addr;
	};
	__u32 size;
} __packed;

/*
 * struct mtkcam_ipi_hw_buffer - DMA buffer for CAM DMA device.
 *
 * @iova: DMA address for CAM DMA device.
 * @size: buffer size.
 */
struct mtkcam_ipi_hw_buffer {
	__u64 iova;
	__u32 size;
} __packed;

struct mtkcam_ipi_pix_fmt {
	__u32			format;
	struct mtkcam_ipi_size	s;
	__u16			stride[CAM_MAX_PLANENUM];
} __packed;

struct mtkcam_ipi_crop {
	struct mtkcam_ipi_point p;
	struct mtkcam_ipi_size s;
} __packed;

struct mtkcam_ipi_uid {
	__u8 pipe_id;
	__u8 id;
} __packed;

#ifdef __KERNEL__

static inline unsigned int mtkcam_raw_ids(void)
{
	return MTKCAM_IPI_RAW_ID_MAX - 1;
}

static inline unsigned int mtkcam_camsv_ids(void)
{
	return MTKCAM_IPI_CAMSV_ID_MAX - 1;
}

static inline unsigned int mtkcam_mraw_ids(void)
{
	return MTKCAM_IPI_MRAW_ID_MAX - 1;
}

static inline unsigned int uid_nbits(void)
{
	return 3 * mtkcam_raw_ids()
		+ 8 * mtkcam_camsv_ids()
		+ 2 * mtkcam_mraw_ids();
}

static inline unsigned int uid_to_bit(struct mtkcam_ipi_uid uid)
{
	unsigned int lbit = 0;

	lbit += (uid.pipe_id - MTKCAM_SUBDEV_RAW_0) * mtkcam_raw_ids();
	if (uid.pipe_id > MTKCAM_SUBDEV_CAMSV_0)
		lbit += (uid.pipe_id - MTKCAM_SUBDEV_CAMSV_0) *
			mtkcam_camsv_ids();
	if (uid.pipe_id > MTKCAM_SUBDEV_MRAW_0)
		lbit += (uid.pipe_id - MTKCAM_SUBDEV_MRAW_0) *
			mtkcam_mraw_ids();
	return lbit + uid.id - 1; /* FIXME: 1 for temporay solution */
}

#endif

struct mtkcam_ipi_img_input {
	struct mtkcam_ipi_uid		uid;
	struct mtkcam_ipi_pix_fmt	fmt;
	struct mtkcam_ipi_sw_buffer	buf[CAM_MAX_PLANENUM];
} __packed;

struct mtkcam_ipi_img_output {
	struct mtkcam_ipi_uid		uid;
	struct mtkcam_ipi_pix_fmt	fmt;
	struct mtkcam_ipi_sw_buffer	buf[CAM_MAX_SUBSAMPLE][CAM_MAX_PLANENUM];
	struct mtkcam_ipi_crop		crop;
} __packed;

struct mtkcam_ipi_meta_input {
	struct mtkcam_ipi_uid		uid;
	struct mtkcam_ipi_sw_buffer	buf;
} __packed;

struct mtkcam_ipi_meta_output {
	struct mtkcam_ipi_uid		uid;
	struct mtkcam_ipi_sw_buffer	buf;
} __packed;

struct mtkcam_ipi_input_param {
	__u32	fmt;
	__u8	raw_pixel_id;
	__u8	data_pattern;
	__u8	pixel_mode;
	__u8	pixel_mode_before_raw;
	__u8	subsample;
	/* u8 continuous; */ /* always 1 */
	/* u16 tg_fps; */ /* not used yet */
	struct mtkcam_ipi_crop in_crop;
} __packed;

struct mtkcam_ipi_raw_frame_param {
	__u8	imgo_path_sel; /* mtkcam_ipi_raw_path_control */
	__u8	hardware_scenario;
	__u32	bin_flag;
	__u8    exposure_num;
	__u8    previous_exposure_num;
	struct mtkcam_ipi_fract	frz_ratio;

	/* blahblah */
} __packed;

/* TODO: support CAMSV */
struct cam_camsv_params {
	/* sth here */
} __packed;

#define MRAW_MAX_IMAGE_OUTPUT (3)

struct mtkcam_ipi_mraw_frame_param {
	__u8 is_config;
	__u8 pixel_mode;

	__u32 tg_pos_x;
	__u32 tg_pos_y;
	__u32 tg_size_w;
	__u32 tg_size_h;
	__u32 tg_fmt;

	__u32 crop_pos_x;
	__u32 crop_pos_y;
	__u32 crop_size_w;
	__u32 crop_size_h;

	struct mtkcam_ipi_meta_input mraw_meta_inputs;
	struct mtkcam_ipi_img_output mraw_img_outputs[MRAW_MAX_IMAGE_OUTPUT];
} __packed;

struct mtkcam_ipi_session_cookie {
	__u8 session_id;
	__u32 frame_no;
} __packed;

struct mtkcam_ipi_session_param {
	struct mtkcam_ipi_sw_buffer workbuf;
	struct mtkcam_ipi_sw_buffer msg_buf;
} __packed;

struct mtkcam_ipi_hw_mapping {
	__u8 pipe_id; /* ref. to mtkcam_pipe_subdev */
	__u16 dev_mask; /* ref. to mtkcam_pipe_dev */
	__u8 exp_order;
} __packed;

/*  Control flags of CAM_CMD_CONFIG */
#define MTK_CAM_IPI_CONFIG_TYPE_INIT			0x0001
#define MTK_CAM_IPI_CONFIG_TYPE_INPUT_CHANGE		0x0002
#define MTK_CAM_IPI_CONFIG_TYPE_EXEC_TWICE		0x0004
#define MTK_CAM_IPI_CONFIG_TYPE_SMVR_PREVIEW		0x0008

struct mtkcam_ipi_config_param {
	__u8 flags;
	struct mtkcam_ipi_input_param	input;
	__u8 n_maps;
	/* maximum # of pipes per stream */
	struct mtkcam_ipi_hw_mapping maps[6];
	/* sub_ratio:8, valid number: 8 */
	__u16 valid_numbers[MTKCAM_IPI_FBCX_LAST];
	__u8	sw_feature;
} __packed;

#define CAM_MAX_IMAGE_INPUT	(5)
#define CAM_MAX_IMAGE_OUTPUT	(15)
#define CAM_MAX_META_OUTPUT	(4)
#define CAM_MAX_PIPE_USED	(4)
#define MRAW_MAX_PIPE_USED  (4)

struct mtkcam_ipi_frame_param {
	__u32 cur_workbuf_offset;
	__u32 cur_workbuf_size;

	struct mtkcam_ipi_raw_frame_param raw_param;
	struct mtkcam_ipi_mraw_frame_param mraw_param[MRAW_MAX_PIPE_USED];

	struct mtkcam_ipi_img_input img_ins[CAM_MAX_IMAGE_INPUT];
	struct mtkcam_ipi_img_output img_outs[CAM_MAX_IMAGE_OUTPUT];
	struct mtkcam_ipi_meta_output meta_outputs[CAM_MAX_META_OUTPUT];
	struct mtkcam_ipi_meta_input meta_inputs[CAM_MAX_PIPE_USED];
} __packed;

struct mtkcam_ipi_frame_info {
	__u32	cur_msgbuf_offset;
	__u32	cur_msgbuf_size;
} __packed;

struct mtkcam_ipi_frame_ack_result {
	__u32 cq_desc_offset;
	__u32 sub_cq_desc_offset;
	__u32 mraw_cq_desc_offset[MRAW_MAX_PIPE_USED];
	__u32 cq_desc_size;
	__u32 sub_cq_desc_size;
	__u32 mraw_cq_desc_size[MRAW_MAX_PIPE_USED];
} __packed;

struct mtkcam_ipi_ack_info {
	__u8 ack_cmd_id;
	__s32 ret;
	struct mtkcam_ipi_frame_ack_result frame_result;
} __packed;

/*
 * The IPI command enumeration.
 */
enum mtkcam_ipi_cmds {
	/* request for a new streaming: mtkcam_ipi_session_param */
	CAM_CMD_CREATE_SESSION,

	/* config the stream: mtkcam_ipi_config_param */
	CAM_CMD_CONFIG,

	/* per-frame: mtkcam_ipi_frame_param */
	CAM_CMD_FRAME,

	/* release certain streaming: mtkcam_ipi_session_param */
	CAM_CMD_DESTROY_SESSION,

	/* ack: mtkcam_ipi_ack_info */
	CAM_CMD_ACK,

	CAM_CMD_RESERVED,
};

struct mtkcam_ipi_event  {
	struct mtkcam_ipi_session_cookie cookie;
	__u8 cmd_id;
	union {
		struct mtkcam_ipi_session_param	session_data;
		struct mtkcam_ipi_config_param	config_data;
		struct mtkcam_ipi_frame_info	frame_data;
		struct mtkcam_ipi_ack_info	ack_data;
	};
} __packed;

#endif /* __MTK_CAM_IPI_H__ */
