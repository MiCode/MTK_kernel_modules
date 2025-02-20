// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-subdev.h>

#include "mtk_cam.h"
#include "mtk_cam-mraw_pipeline.h"
#include "mtk_cam-fmt_utils.h"

#define MTK_MRAW_TOTAL_OUTPUT_QUEUES 1
#define MTK_MRAW_TOTAL_CAPTURE_QUEUES 1

static const struct v4l2_mbus_framefmt mraw_mfmt_default = {
	.code = MEDIA_BUS_FMT_SBGGR10_1X10,
	.width = DEFAULT_WIDTH,
	.height = DEFAULT_HEIGHT,
	.field = V4L2_FIELD_NONE,
	.colorspace = V4L2_COLORSPACE_SRGB,
	.xfer_func = V4L2_XFER_FUNC_DEFAULT,
	.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
	.quantization = V4L2_QUANTIZATION_DEFAULT,
};

static int mtk_mraw_sd_subscribe_event(struct v4l2_subdev *subdev,
				      struct v4l2_fh *fh,
				      struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_FRAME_SYNC:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_REQUEST_DRAINED:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	default:
		return -EINVAL;
	}
}

static int mtk_mraw_sd_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int mtk_mraw_init_cfg(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state)
{
	struct v4l2_mbus_framefmt *mf;
	unsigned int i;
	struct mtk_mraw_pipeline *pipe =
		container_of(sd, struct mtk_mraw_pipeline, subdev);

	for (i = 0; i < sd->entity.num_pads; i++) {
		mf = v4l2_subdev_get_try_format(sd, state, i);
		*mf = mraw_mfmt_default;
		pipe->pad_cfg[i].mbus_fmt = mraw_mfmt_default;

		dev_dbg(sd->v4l2_dev->dev, "%s init pad:%d format:0x%x\n",
			sd->name, i, mf->code);
	}

	return 0;
}

static int mtk_mraw_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_format *fmt)
{
	struct mtk_mraw_pipeline *pipe =
		container_of(sd, struct mtk_mraw_pipeline, subdev);
	unsigned int sensor_fmt = sensor_mbus_to_ipi_fmt(fmt->format.code);

	dev_dbg(sd->v4l2_dev->dev, "%s try format 0x%x, w:%d, h:%d field:%d\n",
		sd->name, fmt->format.code, fmt->format.width,
		fmt->format.height, fmt->format.field);

	/* check sensor format */
	if (!sensor_fmt || fmt->pad == MTK_MRAW_SINK)
		return sensor_fmt;
	else if (fmt->pad < MTK_MRAW_PIPELINE_PADS_NUM) {
		/* check vdev node format */
		unsigned int img_fmt, i;
		struct mtk_cam_video_device *node =
			&pipe->vdev_nodes[fmt->pad - MTK_MRAW_SINK_NUM];

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

static struct v4l2_mbus_framefmt *get_mraw_fmt(struct mtk_mraw_pipeline *pipe,
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

static int mtk_mraw_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *state,
			  struct v4l2_subdev_format *fmt)
{
	struct mtk_mraw_pipeline *pipe =
		container_of(sd, struct mtk_mraw_pipeline, subdev);
	struct v4l2_mbus_framefmt *mf;
	unsigned int ipi_fmt;

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

	if (mtk_mraw_try_fmt(sd, fmt) == MTKCAM_IPI_IMG_FMT_UNKNOWN) {
		mf = get_mraw_fmt(pipe, state, fmt->pad, fmt->which);
		fmt->format = *mf;
		dev_info(sd->v4l2_dev->dev,
			"sd:%s pad:%d set format w/h/code/which %d/%d/0x%x/%d\n",
			sd->name, fmt->pad, mf->width, mf->height, mf->code, fmt->which);
	} else {
		mf = get_mraw_fmt(pipe, state, fmt->pad, fmt->which);
		*mf = fmt->format;
		dev_info(sd->v4l2_dev->dev,
			"sd:%s pad:%d set format w/h/code/which %d/%d/0x%x/%d\n",
			sd->name, fmt->pad, mf->width, mf->height, mf->code, fmt->which);

		if (fmt->pad == MTK_MRAW_SINK &&
			fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
			dev_dbg(sd->v4l2_dev->dev, "%s: set mraw res_config", __func__);
			/* set cfg buffer for tg/crp info. */
			ipi_fmt = sensor_mbus_to_ipi_fmt(fmt->format.code);
			if (ipi_fmt == MTKCAM_IPI_IMG_FMT_UNKNOWN) {
				dev_info(sd->v4l2_dev->dev, "%s:Unknown pixelfmt:%d\n"
					, __func__, ipi_fmt);
			}
		}
	}

	return 0;
}

static int mtk_mraw_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *state,
			  struct v4l2_subdev_format *fmt)
{
	struct mtk_mraw_pipeline *pipe =
		container_of(sd, struct mtk_mraw_pipeline, subdev);
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

static int mtk_mraw_media_link_setup(struct media_entity *entity,
				    const struct media_pad *local,
				    const struct media_pad *remote, u32 flags)
{
	struct mtk_mraw_pipeline *pipe =
		container_of(entity, struct mtk_mraw_pipeline, subdev.entity);
	u32 pad = local->index;

	dev_info(pipe->subdev.v4l2_dev->dev, "%s: mraw %d: %d->%d flags:0x%x\n",
		__func__, pipe->id, remote->index, local->index, flags);

	if (pad == MTK_MRAW_SINK)
		pipe->seninf_padidx = remote->index;

	if (pad < MTK_MRAW_PIPELINE_PADS_NUM && pad != MTK_MRAW_SINK)
		pipe->vdev_nodes[pad-MTK_MRAW_SINK_NUM].enabled =
			!!(flags & MEDIA_LNK_FL_ENABLED);

	if (!(flags & MEDIA_LNK_FL_ENABLED))
		memset(pipe->pad_cfg, 0, sizeof(pipe->pad_cfg));

	return 0;
}

static const struct v4l2_subdev_core_ops mtk_mraw_subdev_core_ops = {
	.subscribe_event = mtk_mraw_sd_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops mtk_mraw_subdev_video_ops = {
	.s_stream =  mtk_mraw_sd_s_stream,
};

static const struct v4l2_subdev_pad_ops mtk_mraw_subdev_pad_ops = {
	.link_validate = mtk_cam_mraw_link_validate,
	.init_cfg = mtk_mraw_init_cfg,
	.set_fmt = mtk_mraw_set_fmt,
	.get_fmt = mtk_mraw_get_fmt,
};

static const struct v4l2_subdev_ops mtk_mraw_subdev_ops = {
	.core = &mtk_mraw_subdev_core_ops,
	.video = &mtk_mraw_subdev_video_ops,
	.pad = &mtk_mraw_subdev_pad_ops,
};

static const struct media_entity_operations mtk_mraw_media_entity_ops = {
	.link_setup = mtk_mraw_media_link_setup,
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_ioctl_ops mtk_mraw_v4l2_meta_cap_ioctl_ops = {
	.vidioc_querycap = mtk_cam_vidioc_querycap,
	.vidioc_enum_fmt_meta_cap = mtk_cam_vidioc_meta_enum_fmt,
	.vidioc_g_fmt_meta_cap = mtk_cam_vidioc_g_meta_fmt,
	.vidioc_s_fmt_meta_cap = mtk_cam_vidioc_g_meta_fmt,
	.vidioc_try_fmt_meta_cap = mtk_cam_vidioc_g_meta_fmt,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = mtk_cam_vidioc_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_expbuf = vb2_ioctl_expbuf,
};

static const struct v4l2_ioctl_ops mtk_mraw_v4l2_meta_out_ioctl_ops = {
	.vidioc_querycap = mtk_cam_vidioc_querycap,
	.vidioc_enum_fmt_meta_out = mtk_cam_vidioc_meta_enum_fmt,
	.vidioc_g_fmt_meta_out = mtk_cam_vidioc_g_meta_fmt,
	.vidioc_s_fmt_meta_out = mtk_cam_vidioc_g_meta_fmt,
	.vidioc_try_fmt_meta_out = mtk_cam_vidioc_g_meta_fmt,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = mtk_cam_vidioc_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_expbuf = vb2_ioctl_expbuf,
};

static const struct mtk_cam_format_desc meta_fmts[] = {
	{
		.vfmt.fmt.meta = {
			.dataformat = V4L2_META_FMT_MTISP_PARAMS,
			.buffersize = SZ_1K,
		},
	},
	{
		.vfmt.fmt.meta = {
			.dataformat = V4L2_META_FMT_MTISP_PARAMS,
			.buffersize = SZ_1K * 4,
		},
	},
};

static const struct
mtk_cam_dev_node_desc mraw_output_queues[] = {
	{
		.id = MTK_MRAW_META_IN,
		.name = "meta input",
		.cap = V4L2_CAP_META_OUTPUT,
		.buf_type = V4L2_BUF_TYPE_META_OUTPUT,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = false,
#ifdef CONFIG_MTK_SCP
		.smem_alloc = true,
#else
		.smem_alloc = false,
#endif
		.dma_port = MTKCAM_IPI_MRAW_META_STATS_CFG,
		.fmts = meta_fmts,
		.default_fmt_idx = 0,
		.max_buf_count = 16,
		.ioctl_ops = &mtk_mraw_v4l2_meta_out_ioctl_ops,
	},
};

static const char *mraw_output_queue_names[MRAW_PIPELINE_NUM]
	[MTK_MRAW_TOTAL_OUTPUT_QUEUES] = {
	{"mtk-cam mraw-0 meta-input"},
	{"mtk-cam mraw-1 meta-input"},
	{"mtk-cam mraw-2 meta-input"},
	{"mtk-cam mraw-3 meta-input"}
};

static const struct
mtk_cam_dev_node_desc mraw_capture_queues[] = {
	{
		.id = MTK_MRAW_META_OUT,
		.name = "meta output",
		.cap = V4L2_CAP_META_CAPTURE,
		.buf_type = V4L2_BUF_TYPE_META_CAPTURE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = false,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_MRAW_META_STATS_0,
		.fmts = meta_fmts,
		.default_fmt_idx = 1,
		.max_buf_count = 16,
		.ioctl_ops = &mtk_mraw_v4l2_meta_cap_ioctl_ops,
	}
};

static const char *mraw_capture_queue_names[MRAW_PIPELINE_NUM]
	[MTK_MRAW_TOTAL_CAPTURE_QUEUES] = {
	{"mtk-cam mraw-0 partial-meta-0"},
	{"mtk-cam mraw-1 partial-meta-0"},
	{"mtk-cam mraw-2 partial-meta-0"},
	{"mtk-cam mraw-3 partial-meta-0"}
};

static void mtk_mraw_pipeline_queue_setup(
	struct mtk_mraw_pipeline *pipe)
{
	unsigned int node_idx, i;

	node_idx = 0;

	/* setup the output queue */
	for (i = 0; i < MTK_MRAW_TOTAL_OUTPUT_QUEUES; i++) {
		pipe->vdev_nodes[node_idx].desc = mraw_output_queues[i];
		pipe->vdev_nodes[node_idx++].desc.name =
			mraw_output_queue_names[pipe->id - MTKCAM_SUBDEV_MRAW_START][i];
	}
	/* setup the capture queue */
	for (i = 0; i < MTK_MRAW_TOTAL_CAPTURE_QUEUES; i++) {
		pipe->vdev_nodes[node_idx].desc = mraw_capture_queues[i];
		pipe->vdev_nodes[node_idx++].desc.name =
			mraw_capture_queue_names[pipe->id - MTKCAM_SUBDEV_MRAW_START][i];
	}
}

static int  mtk_mraw_pipeline_register(const char *str,
	unsigned int id,
	struct mtk_mraw_pipeline *pipe,
	struct v4l2_device *v4l2_dev)
{
	struct v4l2_subdev *sd = &pipe->subdev;
	struct mtk_cam_video_device *video;
	int i;
	int ret;

	pipe->id = id;

	/* initialize subdev */
	v4l2_subdev_init(sd, &mtk_mraw_subdev_ops);
	sd->entity.function = MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER;
	sd->entity.ops = &mtk_mraw_media_entity_ops;
	sd->flags = V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	ret = snprintf(sd->name, sizeof(sd->name), "%s-%d",
		str, pipe->id - MTKCAM_SUBDEV_MRAW_START);
	if (ret < 0) {
		pr_info("Failed to compose device name: %d\n", ret);
		return ret;
	}
	v4l2_set_subdevdata(sd, pipe);

	pr_info("%s: %s\n", __func__, sd->name);

	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (ret < 0) {
		pr_info("Failed to register subdev: %d\n", ret);
		return ret;
	}

	mtk_mraw_pipeline_queue_setup(pipe);

	//setup pads of mraw pipeline
	for (i = 0; i < ARRAY_SIZE(pipe->pads); i++) {
		pipe->pads[i].flags = i < MTK_MRAW_SOURCE_BEGIN ?
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

static void mtk_mraw_pipeline_unregister(
	struct mtk_mraw_pipeline *pipe)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pipe->vdev_nodes); i++)
		mtk_cam_video_unregister(pipe->vdev_nodes + i);

	v4l2_device_unregister_subdev(&pipe->subdev);
	media_entity_cleanup(&pipe->subdev.entity);
}

int mtk_mraw_register_entities(struct mtk_mraw_pipeline *arr_pipe,
	int num, struct v4l2_device *v4l2_dev)
{
	unsigned int i;
	int ret;

	for (i = 0; i < num; i++) {
		struct mtk_mraw_pipeline *pipe = arr_pipe + i;
		memset(pipe->pad_cfg, 0, sizeof(*pipe->pad_cfg));
		ret = mtk_mraw_pipeline_register(
						"mtk-cam mraw",
						MTKCAM_SUBDEV_MRAW_START + i,
						pipe, v4l2_dev);
		if (ret)
			return ret;
	}

	return 0;
}

void mtk_mraw_unregister_entities(struct mtk_mraw_pipeline *arr_pipe, int num)
{
	unsigned int i;

	for (i = 0; i < num; i++)
		mtk_mraw_pipeline_unregister(arr_pipe + i);
}

struct mtk_mraw_pipeline *
mtk_mraw_pipeline_create(struct device *dev, int n)
{
	if (n <= 0)
		return NULL;
	return devm_kcalloc(dev, n, sizeof(struct mtk_mraw_pipeline),
			    GFP_KERNEL);
}

