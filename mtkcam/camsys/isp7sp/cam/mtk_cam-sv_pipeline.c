// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-subdev.h>

#include "mtk_cam.h"
#include "mtk_cam-sv_pipeline.h"
#include "mtk_cam-plat.h"
#include "mtk_cam-fmt_utils.h"

#define MTK_CAMSV_TOTAL_CAPTURE_QUEUES 2

static const struct v4l2_mbus_framefmt sv_mfmt_default = {
	.code = MEDIA_BUS_FMT_SBGGR10_1X10,
	.width = DEFAULT_WIDTH,
	.height = DEFAULT_HEIGHT,
	.field = V4L2_FIELD_NONE,
	.colorspace = V4L2_COLORSPACE_SRGB,
	.xfer_func = V4L2_XFER_FUNC_DEFAULT,
	.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
	.quantization = V4L2_QUANTIZATION_DEFAULT,
};

static int mtk_camsv_sd_subscribe_event(struct v4l2_subdev *subdev,
				      struct v4l2_fh *fh,
				      struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_FRAME_SYNC:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_REQUEST_DRAINED:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_EOS:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	default:
		return -EINVAL;
	}
}

static int mtk_camsv_sd_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct mtk_camsv_pipeline *pipe =
		container_of(sd, struct mtk_camsv_pipeline, subdev);

	if (!enable) {
		atomic_set(&pipe->is_sentest_param_updated, 0);
		memset(&pipe->sentest_param, 0,
			sizeof(struct mtk_cam_seninf_sentest_param));
		pipe->feature_pending = 0;
	}

	return 0;
}

static int mtk_camsv_init_cfg(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state)
{
	struct v4l2_mbus_framefmt *mf;
	unsigned int i;
	struct mtk_camsv_pipeline *pipe =
		container_of(sd, struct mtk_camsv_pipeline, subdev);

	for (i = 0; i < sd->entity.num_pads; i++) {
		mf = v4l2_subdev_get_try_format(sd, state, i);
		*mf = sv_mfmt_default;
		pipe->pad_cfg[i].mbus_fmt = sv_mfmt_default;

		dev_dbg(sd->v4l2_dev->dev, "%s init pad:%d format:0x%x\n",
			sd->name, i, mf->code);
	}

	return 0;
}

static int mtk_camsv_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_format *fmt)
{
	struct mtk_camsv_pipeline *pipe =
		container_of(sd, struct mtk_camsv_pipeline, subdev);
	unsigned int sensor_fmt = sensor_mbus_to_ipi_fmt(fmt->format.code);

	dev_dbg(sd->v4l2_dev->dev, "%s try format 0x%x, w:%d, h:%d field:%d\n",
		sd->name, fmt->format.code, fmt->format.width,
		fmt->format.height, fmt->format.field);

	/* check sensor format */
	if (!sensor_fmt || fmt->pad == MTK_CAMSV_SINK)
		return sensor_fmt;
	else if (fmt->pad < MTK_CAMSV_PIPELINE_PADS_NUM) {
		/* check vdev node format */
		unsigned int img_fmt, i;
		struct mtk_cam_video_device *node =
			&pipe->vdev_nodes[fmt->pad - MTK_CAMSV_SINK_NUM];

		dev_dbg(sd->v4l2_dev->dev, "node:%s num_fmts:%d",
				node->desc.name, node->desc.num_fmts);
		for (i = 0; i < node->desc.num_fmts; i++) {
			img_fmt = mtk_cam_get_img_fmt(
				node->desc.fmts[i].vfmt.fmt.pix_mp.pixelformat);
			dev_dbg(sd->v4l2_dev->dev,
				"try format sensor_fmt 0x%x img_fmt 0x%x",
				sensor_fmt, img_fmt);
			if (sensor_fmt == img_fmt)
				return img_fmt;
		}
	}

	return MTKCAM_IPI_IMG_FMT_UNKNOWN;
}

static struct v4l2_mbus_framefmt *get_sv_fmt(struct mtk_camsv_pipeline *pipe,
					  struct v4l2_subdev_state *state,
					  unsigned int padid, int which)
{
	/* format invalid and return default format */
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return v4l2_subdev_get_try_format(&pipe->subdev, state, padid);

	if (WARN_ON(padid >= pipe->subdev.entity.num_pads))
		return &pipe->pad_cfg[0].mbus_fmt;

	return &pipe->pad_cfg[padid].mbus_fmt;
}

static int mtk_camsv_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *state,
			  struct v4l2_subdev_format *fmt)
{
	struct mtk_camsv_pipeline *pipe =
		container_of(sd, struct mtk_camsv_pipeline, subdev);
	struct v4l2_mbus_framefmt *mf;

	if (!sd || !fmt) {
		dev_dbg(sd->v4l2_dev->dev, "%s: Required sd(%p), fmt(%p)\n",
			__func__, sd, fmt);
		return -EINVAL;
	}

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY && !state) {
		dev_dbg(sd->v4l2_dev->dev, "%s: Required sd(%p), state(%p) for FORMAT_TRY\n",
					__func__, sd, state);
		return -EINVAL;
	}

	if (mtk_camsv_try_fmt(sd, fmt) == MTKCAM_IPI_IMG_FMT_UNKNOWN) {
		mf = get_sv_fmt(pipe, state, fmt->pad, fmt->which);
		fmt->format = *mf;

		dev_info(sd->v4l2_dev->dev,
			"sd:%s pad:%d set format w/h/code/which %d/%d/0x%x/%d\n",
			sd->name, fmt->pad, mf->width, mf->height, mf->code, fmt->which);
	} else {
		mf = get_sv_fmt(pipe, state, fmt->pad, fmt->which);
		*mf = fmt->format;

		dev_info(sd->v4l2_dev->dev,
			"sd:%s pad:%d set format w/h/code/which %d/%d/0x%x/%d\n",
			sd->name, fmt->pad, mf->width, mf->height, mf->code, fmt->which);
	}

	return 0;
}

static int mtk_camsv_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *state,
			  struct v4l2_subdev_format *fmt)
{
	struct mtk_camsv_pipeline *pipe =
		container_of(sd, struct mtk_camsv_pipeline, subdev);
	struct v4l2_mbus_framefmt *mf;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		mf = v4l2_subdev_get_try_format(sd, state, fmt->pad);
	else {
		if (WARN_ON(fmt->pad >= sd->entity.num_pads))
			mf = &pipe->pad_cfg[0].mbus_fmt;
		else
			mf = &pipe->pad_cfg[fmt->pad].mbus_fmt;
	}

	fmt->format = *mf;
	dev_dbg(sd->v4l2_dev->dev, "sd:%s pad:%d get format 0x%x\n",
		sd->name, fmt->pad, fmt->format.code);

	return 0;
}

static int mtk_camsv_media_link_setup(struct media_entity *entity,
				    const struct media_pad *local,
				    const struct media_pad *remote, u32 flags)
{
	struct mtk_camsv_pipeline *pipe =
		container_of(entity, struct mtk_camsv_pipeline, subdev.entity);
	u32 pad = local->index;

	dev_info(pipe->subdev.v4l2_dev->dev, "%s: camsv %d: %d->%d flags:0x%x\n",
		__func__, pipe->id, remote->index, local->index, flags);

	if (pad == MTK_CAMSV_SINK)
		pipe->seninf_padidx = remote->index;

	if (pad < MTK_CAMSV_PIPELINE_PADS_NUM && pad != MTK_CAMSV_SINK)
		pipe->vdev_nodes[pad-MTK_CAMSV_SINK_NUM].enabled =
			!!(flags & MEDIA_LNK_FL_ENABLED);

	if (!(flags & MEDIA_LNK_FL_ENABLED))
		memset(pipe->pad_cfg, 0, sizeof(pipe->pad_cfg));

	return 0;
}

static const struct v4l2_subdev_core_ops mtk_camsv_subdev_core_ops = {
	.subscribe_event = mtk_camsv_sd_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops mtk_camsv_subdev_video_ops = {
	.s_stream =  mtk_camsv_sd_s_stream,
};

static const struct v4l2_subdev_pad_ops mtk_camsv_subdev_pad_ops = {
	.link_validate = mtk_cam_sv_link_validate,
	.init_cfg = mtk_camsv_init_cfg,
	.set_fmt = mtk_camsv_set_fmt,
	.get_fmt = mtk_camsv_get_fmt,
};

static const struct v4l2_subdev_ops mtk_camsv_subdev_ops = {
	.core = &mtk_camsv_subdev_core_ops,
	.video = &mtk_camsv_subdev_video_ops,
	.pad = &mtk_camsv_subdev_pad_ops,
};

static const struct media_entity_operations mtk_camsv_media_entity_ops = {
	.link_setup = mtk_camsv_media_link_setup,
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_ioctl_ops mtk_camsv_v4l2_vcap_ioctl_ops = {
	.vidioc_querycap = mtk_cam_vidioc_querycap,
	.vidioc_enum_framesizes = mtk_cam_vidioc_enum_framesizes,
	.vidioc_enum_fmt_vid_cap = mtk_cam_vidioc_enum_fmt,
	.vidioc_g_fmt_vid_cap_mplane = mtk_cam_vidioc_g_fmt,
	.vidioc_s_fmt_vid_cap_mplane = mtk_cam_vidioc_s_fmt,
	.vidioc_try_fmt_vid_cap_mplane = mtk_cam_vidioc_try_fmt,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = mtk_cam_vidioc_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static const struct mtk_cam_format_desc sv_stream_out_fmts[] = {
	/* This is a default image format */
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SBGGR10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR8,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SBGGR12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SBGGR14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG8,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGBRG10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGBRG12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGBRG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG8,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRBG10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRBG12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRBG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB8,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SRGGB10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SRGGB12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SRGGB14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR10P,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG10P,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG10P,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB10P,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12_10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21_10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV12_10P,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV21_10P,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_BAYER8_UFBC,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_BAYER10_UFBC,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_BAYER12_UFBC,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = SV_IMG_MAX_WIDTH,
			.height = SV_IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_BAYER14_UFBC,
			.num_planes = 1,
		},
	},
};

static const struct
mtk_cam_dev_node_desc sv_capture_queues[] = {
	{
		.id = MTK_CAMSV_MAIN_STREAM_OUT,
		.name = "main stream",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_CAMSV_MAIN_OUT,
		.fmts = sv_stream_out_fmts,
		.num_fmts = ARRAY_SIZE(sv_stream_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_camsv_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = SV_IMG_MAX_WIDTH,
				.min_width = SV_IMG_MIN_WIDTH,
				.max_height = SV_IMG_MAX_HEIGHT,
				.min_height = SV_IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_CAMSV_EXT_STREAM_OUT,
		.name = "ext stream",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_CAMSV_MAIN_OUT,
		.fmts = sv_stream_out_fmts,
		.num_fmts = ARRAY_SIZE(sv_stream_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_camsv_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = SV_IMG_MAX_WIDTH,
				.min_width = SV_IMG_MIN_WIDTH,
				.max_height = SV_IMG_MAX_HEIGHT,
				.min_height = SV_IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
};

static const char *sv_capture_queue_names[MAX_SV_PIPELINE_NUN][MTK_CAMSV_TOTAL_CAPTURE_QUEUES] = {
	{"mtk-cam camsv-0 main-stream",
		"mtk-cam camsv-0 ext-stream"},
	{"mtk-cam camsv-1 main-stream",
		"mtk-cam camsv-1 ext-stream"},
	{"mtk-cam camsv-2 main-stream",
		"mtk-cam camsv-2 ext-stream"},
	{"mtk-cam camsv-3 main-stream",
		"mtk-cam camsv-3 ext-stream"},
	{"mtk-cam camsv-4 main-stream",
		"mtk-cam camsv-4 ext-stream"},
	{"mtk-cam camsv-5 main-stream",
		"mtk-cam camsv-5 ext-stream"},
	{"mtk-cam camsv-6 main-stream",
		"mtk-cam camsv-6 ext-stream"},
	{"mtk-cam camsv-7 main-stream",
		"mtk-cam camsv-7 ext-stream"},
	{"mtk-cam camsv-8 main-stream",
		"mtk-cam camsv-8 ext-stream"},
	{"mtk-cam camsv-9 main-stream",
		"mtk-cam camsv-9 ext-stream"},
	{"mtk-cam camsv-10 main-stream",
		"mtk-cam camsv-10 ext-stream"},
	{"mtk-cam camsv-11 main-stream",
		"mtk-cam camsv-11 ext-stream"},
	{"mtk-cam camsv-12 main-stream",
		"mtk-cam camsv-12 ext-stream"},
	{"mtk-cam camsv-13 main-stream",
		"mtk-cam camsv-13 ext-stream"},
	{"mtk-cam camsv-14 main-stream",
		"mtk-cam camsv-14 ext-stream"},
	{"mtk-cam camsv-15 main-stream",
		"mtk-cam camsv-15 ext-stream"},
};

static void mtk_camsv_pipeline_queue_setup(
	struct mtk_camsv_pipeline *pipe)
{
	unsigned int node_idx, i;

	node_idx = 0;
	/* setup the capture queue */
	for (i = 0; i < MTK_CAMSV_TOTAL_CAPTURE_QUEUES; i++) {
		pipe->vdev_nodes[node_idx].desc = sv_capture_queues[i];
		pipe->vdev_nodes[node_idx++].desc.name =
			sv_capture_queue_names[pipe->id - MTKCAM_SUBDEV_CAMSV_START][i];
	}
}

static int mtk_camsv_pipeline_register(const char *str,
	unsigned int id,
	struct mtk_camsv_pipeline *pipe,
	struct v4l2_device *v4l2_dev)
{
	struct v4l2_subdev *sd = &pipe->subdev;
	struct mtk_cam_video_device *video;
	int i;
	int ret;

	pipe->id = id;

	/* initialize subdev */
	v4l2_subdev_init(sd, &mtk_camsv_subdev_ops);
	sd->entity.function = MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER;
	sd->entity.ops = &mtk_camsv_media_entity_ops;
	sd->flags = V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	ret = snprintf(sd->name, sizeof(sd->name), "%s-%d", str,
		pipe->id - MTKCAM_SUBDEV_CAMSV_START);
	if (ret < 0) {
		pr_info("Failed to compose device name: %d\n", ret);
		return ret;
	}
	v4l2_set_subdevdata(sd, pipe);

	//pr_info("%s: %s\n", __func__, sd->name);

	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (ret < 0) {
		pr_info("Failed to register subdev: %d\n", ret);
		return ret;
	}

	mtk_camsv_pipeline_queue_setup(pipe);

	/* setup pads of camsv pipeline */
	for (i = 0; i < ARRAY_SIZE(pipe->pads); i++) {
		pipe->pads[i].flags = i < MTK_CAMSV_SOURCE_BEGIN ?
			MEDIA_PAD_FL_SINK : MEDIA_PAD_FL_SOURCE;
	}

	media_entity_pads_init(&sd->entity, ARRAY_SIZE(pipe->pads), pipe->pads);

	/* setup video node */
	for (i = 0; i < ARRAY_SIZE(pipe->vdev_nodes); i++) {
		video = pipe->vdev_nodes + i;

		video->uid.pipe_id = pipe->id;
		video->uid.id = video->desc.dma_port;

		ret = mtk_cam_video_register(video, v4l2_dev);
		if (ret)
			goto fail_unregister_video;

		if (V4L2_TYPE_IS_OUTPUT(video->desc.buf_type))
			ret = media_create_pad_link(&video->vdev.entity, 0,
						    &sd->entity,
						    video->desc.id,
						    video->desc.link_flags);
		else
			ret = media_create_pad_link(&sd->entity,
						    video->desc.id,
						    &video->vdev.entity, 0,
						    video->desc.link_flags);

		if (ret)
			goto fail_unregister_video;
	}

	return 0;

fail_unregister_video:
	for (i = i - 1; i >= 0; i--)
		mtk_cam_video_unregister(pipe->vdev_nodes + i);
	return ret;
}

static void mtk_camsv_pipeline_unregister(
	struct mtk_camsv_pipeline *pipe)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pipe->vdev_nodes); i++)
		mtk_cam_video_unregister(pipe->vdev_nodes + i);

	v4l2_device_unregister_subdev(&pipe->subdev);
	media_entity_cleanup(&pipe->subdev.entity);
}

int mtk_camsv_register_entities(struct mtk_camsv_pipeline *arr_pipe,
	int num, struct v4l2_device *v4l2_dev)
{
	unsigned int i;
	int ret;

	for (i = 0; i < num; i++) {
		struct mtk_camsv_pipeline *pipe = arr_pipe + i;

		memset(pipe->pad_cfg, 0, sizeof(*pipe->pad_cfg));
		ret = mtk_camsv_pipeline_register(
						"mtk-cam camsv",
						MTKCAM_SUBDEV_CAMSV_START + i,
						pipe, v4l2_dev);
		if (ret)
			return ret;
	}

	return 0;
}

void mtk_camsv_unregister_entities(struct mtk_camsv_pipeline *arr_pipe, int num)
{
	unsigned int i;

	for (i = 0; i < num; i++)
		mtk_camsv_pipeline_unregister(arr_pipe + i);
}

struct mtk_camsv_pipeline *
mtk_camsv_pipeline_create(struct device *dev, int n)
{
	if (n <= 0)
		return NULL;
	return devm_kcalloc(dev, n, sizeof(struct mtk_camsv_pipeline),
			    GFP_KERNEL);
}

int mtk_cam_sv_update_feature(struct mtk_cam_video_device *node)
{
	struct mtk_cam_device *cam = video_get_drvdata(&node->vdev);
	struct mtk_camsv_pipeline *sv_pipe;
	int i;

	if (node->desc.id != MTK_CAMSV_EXT_STREAM_OUT)
		return 0;

	for (i = 0; i < cam->pipelines.num_camsv; i++) {
		sv_pipe = &cam->pipelines.camsv[i];
		if (sv_pipe->id == node->uid.pipe_id) {
			sv_pipe->feature_pending |= DISPLAY_IC;
			break;
		}
	}

	return 0;
}

int mtk_cam_sv_update_image_size(struct mtk_cam_video_device *node,
	struct v4l2_format *f)
{
	if (node->desc.dma_port == MTKCAM_IPI_CAMSV_MAIN_OUT &&
		node->desc.id == MTK_CAMSV_MAIN_STREAM_OUT)
		f->fmt.pix_mp.plane_fmt[0].sizeimage +=
			(GET_PLAT_V4L2(meta_sv_ext_size) + 16);

	return 0;
}

