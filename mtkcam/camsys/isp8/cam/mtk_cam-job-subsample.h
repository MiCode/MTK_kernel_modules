/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_JOB_SUBSAMPLE_H
#define __MTK_CAM_JOB_SUBSAMPLE_H

#include "mtk_cam-job_utils.h"

int fill_imgo_img_buffer_to_ipi_frame_subsample(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node);

int fill_yuvo_img_buffer_to_ipi_frame_subsample(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node);

#endif //__MTK_CAM_JOB_SUBSAMPLE_H

