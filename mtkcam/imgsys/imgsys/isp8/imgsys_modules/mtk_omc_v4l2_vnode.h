/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Floria Huang <floria.huang@mediatek.com>
 *
 */

#ifndef _MTK_OMC_V4L2_VNODE_H_
#define _MTK_OMC_V4L2_VNODE_H_

/*#include <linux/platform_device.h>
 * #include <linux/module.h>
 * #include <linux/of_device.h>
 * #include <linux/pm_runtime.h>
 * #include <linux/remoteproc.h>
 * #include <linux/remoteproc/mtk_scp.h>
 */
#include <linux/videodev2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-event.h>
#include "mtk_imgsys-dev.h"
#include "mtk_imgsys-hw.h"

//#include "mtk_imgsys_v4l2.h"
#include "mtk_header_desc.h"

#define defaultdesc 0

#define MTK_OMC_OUTPUT_MIN_WIDTH		2U
#define MTK_OMC_OUTPUT_MIN_HEIGHT		2U
#define MTK_OMC_OUTPUT_MAX_WIDTH		18472U
#define MTK_OMC_OUTPUT_MAX_HEIGHT		13856U

#define MTK_OMC_MAP_OUTPUT_MIN_WIDTH	2U
#define MTK_OMC_MAP_OUTPUT_MIN_HEIGHT	2U
#define MTK_OMC_MAP_OUTPUT_MAX_WIDTH	640U
#define MTK_OMC_MAP_OUTPUT_MAX_HEIGHT	480U

#define MTK_OMC_CAPTURE_MIN_WIDTH		2U
#define MTK_OMC_CAPTURE_MIN_HEIGHT		2U
#define MTK_OMC_CAPTURE_MAX_WIDTH		18472U
#define MTK_OMC_CAPTURE_MAX_HEIGHT		13856U


static const struct mtk_imgsys_dev_format omc_omci_fmts[] = {
	/* YUV420, 2 plane 8 bit */
	{
		.format = V4L2_PIX_FMT_NV12,
		.depth = { 12 },
		.row_depth = { 8 },
		.num_planes = 1,
		.num_cplanes = 2,
	},
	{
		.format = V4L2_PIX_FMT_NV12M,
		.depth      = { 8, 4 },
		.row_depth  = { 8, 8 },
		.num_planes = 1,
		.num_cplanes = 2,
	},
	{
		.format = V4L2_PIX_FMT_NV21,
		.depth = { 12 },
		.row_depth = { 8 },
		.num_planes = 1,
		.num_cplanes = 2,
	},
	{
		.format = V4L2_PIX_FMT_NV21M,
		.depth      = { 8, 4 },
		.row_depth  = { 8, 8 },
		.num_planes = 1,
		.num_cplanes = 2,
	},
	/* Y8 bit */
	{
		.format	= V4L2_PIX_FMT_GREY,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_cplanes = 1,
	},
	/* Must have for SMVR/Multis-cale for every video_device nodes */
	{
		.format = V4L2_META_FMT_MTISP_DESC,
		.num_planes = 1,
#if defaultdesc
		.depth = { 8 },
		.row_depth = { 8 },
		.num_cplanes = 1,
#endif
		.buffer_size = sizeof(struct header_desc),
	},
	{
		.format = V4L2_META_FMT_MTISP_DESC_NORM,
		.num_planes = 1,
		.buffer_size = sizeof(struct header_desc_norm),
	},

};

static const struct mtk_imgsys_dev_format omc_veci_fmts[] = {
	/* WarpMap, 2 plane, packed in 4-byte */
	{
		.format = V4L2_PIX_FMT_WARP2P,
		.depth = { 32 },
		.row_depth = { 32 },
		.num_planes = 1,
		.num_cplanes = 2,
	},
	/* Must have for SMVR/Multis-cale for every video_device nodes */
	{
		.format = V4L2_META_FMT_MTISP_DESC,
		.num_planes = 1,
#if defaultdesc
		.depth = { 8 },
		.row_depth = { 8 },
		.num_cplanes = 1,
#endif
		.buffer_size = sizeof(struct header_desc),
	},
	{
		.format = V4L2_META_FMT_MTISP_DESC_NORM,
		.num_planes = 1,
		.buffer_size = sizeof(struct header_desc_norm),
	},

};

static const struct mtk_imgsys_dev_format omc_omco_fmts[] = {
	/* YUV420, 2 plane 8 bit */
	{
		.format = V4L2_PIX_FMT_NV12,
		.depth = { 12 },
		.row_depth = { 8 },
		.num_planes = 1,
		.num_cplanes = 2,
	},
	{
		.format = V4L2_PIX_FMT_NV12M,
		.depth      = { 8, 4 },
		.row_depth  = { 8, 8 },
		.num_planes = 1,
		.num_cplanes = 2,
	},
	{
		.format = V4L2_PIX_FMT_NV21,
		.depth = { 12 },
		.row_depth = { 8 },
		.num_planes = 1,
		.num_cplanes = 2,
	},
	{
		.format = V4L2_PIX_FMT_NV21M,
		.depth      = { 8, 4 },
		.row_depth  = { 8, 8 },
		.num_planes = 1,
		.num_cplanes = 2,
	},
	/* Y8 bit */
	{
		.format	= V4L2_PIX_FMT_GREY,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_cplanes = 1,
	},
	/* Must have for SMVR/Multis-cale for every video_device nodes */
	{
		.format = V4L2_META_FMT_MTISP_DESC,
		.num_planes = 1,
#if defaultdesc
		.depth = { 8 },
		.row_depth = { 8 },
		.num_cplanes = 1,
#endif
		.buffer_size = sizeof(struct header_desc),
	},
	{
		.format = V4L2_META_FMT_MTISP_DESC_NORM,
		.num_planes = 1,
		.buffer_size = sizeof(struct header_desc_norm),
	},

};

static const struct mtk_imgsys_dev_format omc_msko_fmts[] = {
	/* Y8 bit */
	{
		.format	= V4L2_PIX_FMT_GREY,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_cplanes = 1,
	},
	/* Must have for SMVR/Multis-cale for every video_device nodes */
	{
		.format = V4L2_META_FMT_MTISP_DESC,
		.num_planes = 1,
#if defaultdesc
		.depth = { 8 },
		.row_depth = { 8 },
		.num_cplanes = 1,
#endif
		.buffer_size = sizeof(struct header_desc),
	},
	{
		.format = V4L2_META_FMT_MTISP_DESC_NORM,
		.num_planes = 1,
		.buffer_size = sizeof(struct header_desc_norm),
	},

};

static const struct v4l2_frmsizeenum omc_in_frmsizeenum = {
	.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
	.stepwise.max_width = MTK_OMC_CAPTURE_MAX_WIDTH,
	.stepwise.min_width = MTK_OMC_CAPTURE_MIN_WIDTH,
	.stepwise.max_height = MTK_OMC_CAPTURE_MAX_HEIGHT,
	.stepwise.min_height = MTK_OMC_CAPTURE_MIN_HEIGHT,
	.stepwise.step_height = 1,
	.stepwise.step_width = 1,
};

static const struct v4l2_frmsizeenum omc_in_map_frmsizeenum = {
	.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
	.stepwise.max_width = MTK_OMC_MAP_OUTPUT_MAX_WIDTH,
	.stepwise.min_width = MTK_OMC_MAP_OUTPUT_MIN_WIDTH,
	.stepwise.max_height = MTK_OMC_MAP_OUTPUT_MAX_HEIGHT,
	.stepwise.min_height = MTK_OMC_MAP_OUTPUT_MIN_HEIGHT,
	.stepwise.step_height = 1,
	.stepwise.step_width = 1,
};

static const struct v4l2_frmsizeenum omc_out_frmsizeenum = {
	.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
	.stepwise.max_width = MTK_OMC_OUTPUT_MAX_WIDTH,
	.stepwise.min_width = MTK_OMC_OUTPUT_MIN_WIDTH,
	.stepwise.max_height = MTK_OMC_OUTPUT_MAX_HEIGHT,
	.stepwise.min_height = MTK_OMC_OUTPUT_MIN_HEIGHT,
	.stepwise.step_height = 1,
	.stepwise.step_width = 1,
};

static const struct mtk_imgsys_video_device_desc omc_setting[] = {
	/* Input Video Node */
	{
		.id = MTK_IMGSYS_VIDEO_NODE_ID_OMCI_OUT,
		.name = "OMCI Input",
		.cap = V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING,
		.buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
		.smem_alloc = 0,
		.flags = MEDIA_LNK_FL_DYNAMIC,
		.fmts = omc_omci_fmts,
		.num_fmts = ARRAY_SIZE(omc_omci_fmts),
		.default_fmt_idx = 0,
		.default_width = MTK_OMC_OUTPUT_MAX_WIDTH,
		.default_height = MTK_OMC_OUTPUT_MAX_HEIGHT,
		.dma_port = 0,
		.frmsizeenum = &omc_in_frmsizeenum,
		.description = "OMC main image input",
	},
	{
		.id = MTK_IMGSYS_VIDEO_NODE_ID_OVECI_OUT,
		.name = "OVECI Input",
		.cap = V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING,
		.buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
		.smem_alloc = 0,
		.flags = MEDIA_LNK_FL_DYNAMIC,
		.fmts = omc_veci_fmts,
		.num_fmts = ARRAY_SIZE(omc_veci_fmts),
		.default_fmt_idx = 0,
		.default_width = MTK_OMC_CAPTURE_MAX_WIDTH,
		.default_height = MTK_OMC_CAPTURE_MAX_HEIGHT,
		.dma_port = 1,
		.frmsizeenum = &omc_in_map_frmsizeenum,
		.description = "OMC MV input",
	},
	/* OMC Output Video Node */
	{
		.id = MTK_IMGSYS_VIDEO_NODE_ID_OMCO_CAPTURE,
		.name = "OMCO Output",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.smem_alloc = 0,
		.flags = MEDIA_LNK_FL_DYNAMIC,
		.fmts = omc_omco_fmts,
		.num_fmts = ARRAY_SIZE(omc_omco_fmts),
		.default_fmt_idx = 0,
		.default_width = MTK_OMC_CAPTURE_MAX_WIDTH,
		.default_height = MTK_OMC_CAPTURE_MAX_HEIGHT,
		.dma_port = 0,
		.frmsizeenum = &omc_out_frmsizeenum,
		.description = "OMC image output",
	},
	{
		.id = MTK_IMGSYS_VIDEO_NODE_ID_OMSKO_CAPTURE,
		.name = "OMSKO Output",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.smem_alloc = 0,
		.flags = MEDIA_LNK_FL_DYNAMIC,
		.fmts = omc_msko_fmts,
		.num_fmts = ARRAY_SIZE(omc_msko_fmts),
		.default_fmt_idx = 0,
		.default_width = MTK_OMC_CAPTURE_MAX_WIDTH,
		.default_height = MTK_OMC_CAPTURE_MAX_HEIGHT,
		.dma_port = 1,
		.frmsizeenum = &omc_out_frmsizeenum,
		.description = "OMC valid map output",
	},
};

#endif // _MTK_OMC_V4L2_VNODE_H_
