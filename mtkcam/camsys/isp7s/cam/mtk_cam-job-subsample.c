// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include "mtk_cam.h"
#include "mtk_cam-job-subsample.h"
#include "mtk_cam-job_utils.h"


int fill_imgo_img_buffer_to_ipi_frame_subsample(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node)
{
	struct mtkcam_ipi_frame_param *fp = helper->fp;
	struct mtkcam_ipi_img_output *out;
	int ret = -1;

	out = &fp->img_outs[helper->io_idx];
	++helper->io_idx;
	ret = fill_imgo_out_subsample(out, buf, node, helper->job->sub_ratio);

	return ret;

}
int fill_yuvo_img_buffer_to_ipi_frame_subsample(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node)
{
	struct mtkcam_ipi_frame_param *fp = helper->fp;
	struct mtkcam_ipi_img_output *out;
	int ret = -1;

	out = &fp->img_outs[helper->io_idx];
	++helper->io_idx;

	ret = fill_yuvo_out_subsample(out, buf, node, helper->job->sub_ratio);

	return ret;

}
