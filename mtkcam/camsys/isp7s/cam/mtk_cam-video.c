// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>

#include "mtk_cam.h"
#include "mtk_cam-video.h"
#include "mtk_cam-fmt_utils.h"
#include "mtk_camera-videodev2.h"
#include "mtk_cam-ufbc-def.h"
#include "mtk_cam-plat.h"
#include "mtk_cam_vb2-dma-contig.h"
#include "mtk_cam-trace.h"

#define MAX_SUBSAMPLE_PLANE_NUM VB2_MAX_PLANES /* 8 */

static void log_fmt_mp(const char *name, struct v4l2_pix_format_mplane *fmt,
		       const char *caller)
{
	struct v4l2_plane_pix_format *plane;

	plane = &fmt->plane_fmt[0];

	pr_info("%s: %s pixelformat " FMT_FOURCC " %ux%u np %d 0:%u/%u\n",
		caller, name, MEMBER_FOURCC(fmt->pixelformat),
		fmt->width, fmt->height,
		fmt->num_planes,
		plane->bytesperline, plane->sizeimage);
}

static void log_fmt_ops(struct mtk_cam_video_device *node,
			struct v4l2_format *f,
			const char *caller)
{
	struct media_pad *remote_pad;
	const char *remote_name = "null";

	remote_pad = media_pad_remote_pad_unique(&node->pad);
	if (remote_pad)
		remote_name = remote_pad->entity->name;

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;

		log_fmt_mp(remote_pad->entity->name, pix_mp, caller);

	} else if (f->type == V4L2_BUF_TYPE_META_CAPTURE ||
		   f->type == V4L2_BUF_TYPE_META_OUTPUT) {
		struct v4l2_meta_format *meta = &f->fmt.meta;

		pr_info("%s: %s node %s: meta (format " FMT_FOURCC " size %u)\n",
			caller, remote_pad->entity->name, node->desc.name,
			MEMBER_FOURCC(meta->dataformat), meta->buffersize);
	} else {
		pr_info("%s: not-supported yet type %d\n", __func__, f->type);
	}
}

static int mtk_cam_video_set_fmt(struct mtk_cam_video_device *node,
				 struct v4l2_format *f);

static int mtk_cam_vb2_queue_setup(struct vb2_queue *vq,
				   unsigned int *num_buffers,
				   unsigned int *num_planes,
				   unsigned int sizes[],
				   struct device *alloc_devs[])
{
	struct mtk_cam_device *cam = vb2_get_drv_priv(vq);
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vq);
	unsigned int max_buffer_count = node->desc.max_buf_count;
	const struct v4l2_format *fmt = &node->active_fmt;
	unsigned int size;
	int i;
	int min_buf_sz;

	min_buf_sz = ALIGN(IMG_MIN_WIDTH, IMG_PIX_ALIGN) * IMG_MIN_HEIGHT;

	/* Check the limitation of buffer size */
	if (max_buffer_count)
		*num_buffers = clamp_val(*num_buffers, 1, max_buffer_count);

	if (node->desc.smem_alloc)
		vq->dma_attrs |= DMA_ATTR_NO_KERNEL_MAPPING;

	if (vq->type == V4L2_BUF_TYPE_META_OUTPUT ||
	    vq->type == V4L2_BUF_TYPE_META_CAPTURE)
		size = fmt->fmt.meta.buffersize;
	else
		size = min_buf_sz;

	/* Add for q.create_bufs with fmt.g_sizeimage(p) / 2 test */
	if (*num_planes) {
		if (sizes[0] < size || *num_planes != 1)
			return -EINVAL;
	} else {
		*num_planes = 1;
		sizes[0] = size;
		/* workaround */
		if (fmt->fmt.pix_mp.num_planes > 1) {
			*num_planes = fmt->fmt.pix_mp.num_planes;
			for (i = 0; i < *num_planes; i++)
				sizes[i] = size;
		}
		for (i = 0; i < *num_planes; i++)
			dev_dbg(cam->dev, "[%s] id:%d, name:%s, np:%d, i:%d, size:%d\n",
				__func__,
				node->desc.id, node->desc.name,
				*num_planes, i, sizes[i]);
	}
	return 0;
}

static int mtk_cam_vb2_buf_init(struct vb2_buffer *vb)
{
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vb->vb2_queue);
	struct device *dev = vb->vb2_queue->dev;
	struct mtk_cam_buffer *buf;
	dma_addr_t addr;

	buf = mtk_cam_vb2_buf_to_dev_buf(vb);

	/* note: flags should be reset here */
	/* buf->flags = 0; */
	buf->daddr = vb2_dma_contig_plane_dma_addr(vb, 0);
	buf->scp_addr = 0;

	/* SCP address is only valid for meta input buffer */
	if (!node->desc.smem_alloc)
		return 0;

	buf = mtk_cam_vb2_buf_to_dev_buf(vb);
	/* Use coherent address to get iova address */
	addr = dma_map_resource(dev, buf->daddr, vb->planes[0].length,
				DMA_BIDIRECTIONAL, DMA_ATTR_SKIP_CPU_SYNC);
	if (dma_mapping_error(dev, addr)) {
		dev_info(dev, "failed to map meta addr:%pad\n", &buf->daddr);
		return -EFAULT;
	}
	buf->scp_addr = buf->daddr;
	buf->daddr = addr;

	return 0;
}

/* TODO: combine following operation into s_fmt */
static int cache_image_info_mp(struct mtk_cam_cached_image_info *cached,
			       const struct v4l2_pix_format_mplane *fmt)
{
	const struct mtk_format_info *mtk_info;
	const struct v4l2_format_info *v4l2_info;

	unsigned int pixelformat = fmt->pixelformat;
	unsigned int stride0;
	int i;

	cached->v4l2_pixelformat = pixelformat;
	cached->width = fmt->width;
	cached->height = fmt->height;

	memset(cached->bytesperline, 0, sizeof(cached->bytesperline));
	memset(cached->size, 0, sizeof(cached->size));

	/* convert v4l2 single-plane description into multi-plane */
	/* cached->bytesperline/size */

	mtk_info = mtk_format_info(pixelformat);
	v4l2_info = mtk_info ? NULL : v4l2_format_info(pixelformat);

	if (mtk_info) {
		const struct mtk_format_info *info = mtk_info;

		if (info->mem_planes != 1) {
			pr_info("%s: mtk_format do not support non-contiguous mplane\n",
				__func__);
			return -EINVAL;
		}

		if (is_yuv_ufo(pixelformat) || is_raw_ufo(pixelformat)) {
			/* 1-plane, use directly */
			cached->bytesperline[0] = fmt->plane_fmt[0].bytesperline;
			cached->size[0] = fmt->plane_fmt[0].sizeimage;
		} else {
			stride0 = fmt->plane_fmt[0].bytesperline;
			for (i = 0; i < info->comp_planes; i++) {
				unsigned int stride_p = (i == 0) ?
					stride0 :
					mtk_format_calc_stride(info, i,
							       fmt->width,
							       stride0);

				cached->bytesperline[i] = stride_p;
				cached->size[i] =
					mtk_format_calc_planesize(info, i,
								  fmt->height,
								  stride_p);
			}
		}
	} else if (v4l2_info) {
		const struct v4l2_format_info *info = v4l2_info;

		if (info->mem_planes != 1) {
			pr_info("%s: v4l2_format do not support non-contiguous mplane\n",
				__func__);
			return -EINVAL;
		}

		stride0 = fmt->plane_fmt[0].bytesperline;
		for (i = 0; i < info->comp_planes; i++) {
			unsigned int hdiv = (i == 0) ? 1 : info->hdiv;
			unsigned int vdiv = (i == 0) ? 1 : info->vdiv;

			unsigned int stride_p = (i == 0) ? stride0 :
				DIV_ROUND_UP(stride0 * info->bpp[i] / info->bpp[0],
					     hdiv);

			cached->bytesperline[i] = stride_p;
			cached->size[i] = stride_p * DIV_ROUND_UP(fmt->height, vdiv);
		}
	} else {
		pr_info("%s: not found pixelformat " FMT_FOURCC "\n",
			__func__, MEMBER_FOURCC(pixelformat));
		return -EINVAL;
	}
	return 0;
}

static int mtk_cam_video_cache_fmt(struct mtk_cam_video_device *node)
{
	const struct v4l2_format *f = &node->active_fmt;

	if (node->desc.image) {
		const struct v4l2_pix_format_mplane *fmt_mp = &f->fmt.pix_mp;

		if (WARN_ON_ONCE(f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
				 && f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE))
			return -EINVAL;

		if (WARN_ON_ONCE(cache_image_info_mp(&node->image_info, fmt_mp)))
			return -EINVAL;

	} else {
		const struct v4l2_meta_format *meta_fmt = &f->fmt.meta;
		struct mtk_cam_cached_meta_info *cached = &node->meta_info;

		if (WARN_ON_ONCE(f->type != V4L2_BUF_TYPE_META_CAPTURE
				 && f->type != V4L2_BUF_TYPE_META_OUTPUT))
			return -EINVAL;

		cached->v4l2_pixelformat = meta_fmt->dataformat;
		cached->buffersize = meta_fmt->buffersize;
	}

	return 0;
}

static int check_valid_selection(int sink_w, int sink_h,
				 struct v4l2_rect *r)
{
	return r->left >= 0 && r->left + r->width <= sink_w &&
		r->top >= 0 && r->top + r->height <= sink_h &&
		r->width && r->height;
}

static int refine_valid_selection(struct mtk_cam_video_device *node,
				  struct v4l2_selection *s)
{
	int ret = 0;

	/* workaround for user's wrong crop */
	if (is_raw_subdev(node->uid.pipe_id)) {
		struct media_pad *remote_pad;
		struct mtk_raw_pipeline *pipe;
		int sink_w, sink_h;

		remote_pad = media_pad_remote_pad_unique(&node->pad);
		if (WARN_ON_ONCE(!remote_pad))
			return -1;

		if (CAM_DEBUG_ENABLED(V4L2))
			pr_info("%s: remote %s node %s: sel (%d,%d %ux%u)\n",
				__func__,
				remote_pad->entity->name, node->desc.name,
				s->r.left, s->r.top, s->r.width, s->r.height);

		pipe = container_of(remote_pad->entity,
				    struct mtk_raw_pipeline, subdev.entity);

		sink_w = pipe->pad_cfg[MTK_RAW_SINK].mbus_fmt.width;
		sink_h = pipe->pad_cfg[MTK_RAW_SINK].mbus_fmt.height;

		if (!check_valid_selection(sink_w, sink_h, &s->r)) {
			pr_info("%s: warn. wrong selection: %d, %d, %ux%u, sink %dx%d\n",
				__func__,
				s->r.left, s->r.top, s->r.width, s->r.height,
				sink_w, sink_h);

			s->r = (struct v4l2_rect) {
				.left = 0,
				.top = 0,
				.width = sink_w,
				.height = sink_h,
			};
			ret = -1;
		}

	}
	return ret;
}

static void mtk_cam_vb2_buf_collect_image_info(struct vb2_buffer *vb)
{
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vb->vb2_queue);
	struct mtk_cam_buffer *buf = mtk_cam_vb2_buf_to_dev_buf(vb);
	struct mtk_cam_cached_image_info *cached = &buf->image_info;

	refine_valid_selection(node, &node->active_crop);

	/* update crop here to handle cases like s_selection is not called yet */
	node->image_info.crop = node->active_crop.r;

	/* clone into vb2_buffer */
	*cached = node->image_info;
}

static void mtk_cam_vb2_buf_collect_meta_info(struct vb2_buffer *vb)
{
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vb->vb2_queue);
	struct mtk_cam_buffer *buf = mtk_cam_vb2_buf_to_dev_buf(vb);
	struct mtk_cam_cached_meta_info *cached = &buf->meta_info;
	const struct v4l2_meta_format *meta_fmt = &node->active_fmt.fmt.meta;

	cached->v4l2_pixelformat = meta_fmt->dataformat;
	cached->buffersize = meta_fmt->buffersize;
}

static int mtk_cam_vb2_buf_prepare(struct vb2_buffer *vb)
{
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vb->vb2_queue);
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);
	struct mtk_cam_buffer *mtk_buf = mtk_cam_vb2_buf_to_dev_buf(vb);
	const struct v4l2_format *fmt = &node->active_fmt;
	unsigned int size;
	unsigned int plane;

	if (V4L2_TYPE_IS_OUTPUT(vb->type) &&
	    !(mtk_buf->flags & FLAG_NO_CACHE_CLEAN)) {
		dev_dbg(vb->vb2_queue->dev, "%s: %s\n",
			__func__, node->desc.name);
		for (plane = 0; plane < vb->num_planes; ++plane)
			mtk_cam_vb2_sync_for_device(vb->planes[plane].mem_priv);
	}

	if (vb->vb2_queue->type == V4L2_BUF_TYPE_META_OUTPUT ||
	    vb->vb2_queue->type == V4L2_BUF_TYPE_META_CAPTURE)
		size = fmt->fmt.meta.buffersize;
	else
		size = fmt->fmt.pix_mp.plane_fmt[0].sizeimage;

	if (vb2_plane_size(vb, 0) < size) {
		dev_info_ratelimited(vb->vb2_queue->dev, "%s: plane size is too small:%lu<%u\n",
			 node->desc.name, vb2_plane_size(vb, 0), size);
		/* avoid extra mips on warn_on_once in vb2_set_plane_payload following */
		size = vb2_plane_size(vb, 0);
		/* return -EINVAL; */
	}

	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		if ((vb2_get_plane_payload(vb, 0) != size) && (vb->vb2_queue->streaming)) {
			dev_dbg(vb->vb2_queue->dev, "%s: plane payload is mismatch:%lu:%u\n",
				node->desc.name,
				vb2_get_plane_payload(vb, 0), size);
			/* todo: user must set correct byteused */
			/* return -EINVAL;*/
		}
		return 0;
	}

	v4l2_buf->field = V4L2_FIELD_NONE;
	vb2_set_plane_payload(vb, 0, size);

	return 0;
}

static void mtk_cam_vb2_buf_finish(struct vb2_buffer *vb)
{
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vb->vb2_queue);
	struct mtk_cam_buffer *mtk_buf = mtk_cam_vb2_buf_to_dev_buf(vb);
	unsigned int plane;

	if (V4L2_TYPE_IS_CAPTURE(vb->type) &&
	    !(mtk_buf->flags & FLAG_NO_CACHE_INVALIDATE)) {
		dev_dbg(vb->vb2_queue->dev, "%s: %s\n",
			__func__, node->desc.name);
		for (plane = 0; plane < vb->num_planes; ++plane)
			mtk_cam_vb2_sync_for_cpu(vb->planes[plane].mem_priv);
	}
}

static int check_node_linked(struct vb2_queue *vq)
{
	struct mtk_cam_device *cam = vb2_get_drv_priv(vq);
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vq);
	int ret = 0;

	if (!node->enabled) {
		dev_info(cam->dev,
			"%s: stream on failed, node is not enabled\n",
			node->desc.name);
		ret = -ENOLINK;
	}

	return ret;
}

static int mtk_cam_vb2_start_streaming(struct vb2_queue *vq,
				       unsigned int count)
{
	struct mtk_cam_device *cam = vb2_get_drv_priv(vq);
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vq);
	struct mtk_cam_ctx *ctx;
	int ret;

	if (CAM_DEBUG_ENABLED(V4L2))
		dev_info(cam->dev, "%s: node %s\n", __func__, node->desc.name);

	ret = check_node_linked(vq);
	if (ret)
		return ret;

	ctx = mtk_cam_start_ctx(cam, node);
	if (!ctx)
		return -EPIPE;

	++ctx->streaming_node_cnt;
	if (mtk_cam_ctx_all_nodes_streaming(ctx))
		mtk_cam_ctx_stream_on(ctx);

	return 0;
}

static void mtk_cam_vb2_stop_streaming(struct vb2_queue *vq)
{
	struct mtk_cam_device *cam = vb2_get_drv_priv(vq);
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vq);
	struct mtk_cam_ctx *ctx;

	ctx = mtk_cam_find_ctx(cam, &node->vdev.entity);

	if (!ctx) {
		// TODO: clean pending?
		return;
	}

	if (mtk_cam_ctx_all_nodes_streaming(ctx))
		mtk_cam_ctx_stream_off(ctx);

	--ctx->streaming_node_cnt;

	// TODO: clean pending req?

	if (!mtk_cam_ctx_all_nodes_idle(ctx))
		return;
	mtk_cam_stop_ctx(ctx, &node->vdev.entity);

}

static void mtk_cam_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct mtk_cam_buffer *buf = mtk_cam_vb2_buf_to_dev_buf(vb);
	struct mtk_cam_request *req = to_mtk_cam_req(vb->request);
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vb->vb2_queue);
	int ipi_pipe_id = node->uid.pipe_id;

	if (WARN_ON(!req))
		return;

	if (node->desc.image)
		mtk_cam_vb2_buf_collect_image_info(vb);
	else
		mtk_cam_vb2_buf_collect_meta_info(vb);

	req->used_pipe |= ipi_pipe_id_to_bit(ipi_pipe_id);
	list_add_tail(&buf->list, &req->buf_list);
}

static void mtk_cam_vb2_buf_cleanup(struct vb2_buffer *vb)
{
	struct mtk_cam_video_device *node = mtk_cam_vbq_to_vdev(vb->vb2_queue);
	struct mtk_cam_buffer *buf;
	struct device *dev = vb->vb2_queue->dev;

	if (!node->desc.smem_alloc)
		return;

	buf = mtk_cam_vb2_buf_to_dev_buf(vb);
	dma_unmap_page_attrs(dev, buf->daddr,
			     vb->planes[0].length,
			     DMA_BIDIRECTIONAL,
			     DMA_ATTR_SKIP_CPU_SYNC);
}

static void mtk_cam_vb2_request_complete(struct vb2_buffer *vb)
{
	struct mtk_cam_device *cam = vb2_get_drv_priv(vb->vb2_queue);

	//dev_dbg(vb->vb2_queue->dev, "%s\n", __func__);

	v4l2_ctrl_request_complete(vb->req_obj.req,
				   cam->v4l2_dev.ctrl_handler);
}

static int mtk_cam_vb2_buf_out_validate(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);

	if (vb->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (v4l2_buf->field == V4L2_FIELD_ANY)
			v4l2_buf->field = V4L2_FIELD_NONE;

		if (v4l2_buf->field != V4L2_FIELD_NONE)
			return -EINVAL;
	}
	return 0;
}

static const struct vb2_ops mtk_cam_vb2_ops = {
	.queue_setup = mtk_cam_vb2_queue_setup,

	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,

	.buf_out_validate = mtk_cam_vb2_buf_out_validate,
	.buf_init = mtk_cam_vb2_buf_init,
	.buf_prepare = mtk_cam_vb2_buf_prepare,
	.buf_finish = mtk_cam_vb2_buf_finish,
	.buf_cleanup = mtk_cam_vb2_buf_cleanup,

	.start_streaming = mtk_cam_vb2_start_streaming,
	.stop_streaming = mtk_cam_vb2_stop_streaming,

	.buf_queue = mtk_cam_vb2_buf_queue,
	.buf_request_complete = mtk_cam_vb2_request_complete,
};

static const struct v4l2_file_operations mtk_cam_v4l2_fops = {
	.unlocked_ioctl = video_ioctl2,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.poll = vb2_fop_poll,
	.mmap = vb2_fop_mmap,
};

/* ref. v4l2_fill_pixfmt_mp */
static int mtk_cam_fill_v4l2_pixfmt_mp(const struct v4l2_format_info *info,
				       struct v4l2_pix_format_mplane *pixfmt,
				       u32 pixelformat, u32 width, u32 height)
{
	struct v4l2_plane_pix_format *plane;
	unsigned int stride;
	int i;

	pixfmt->width = width;
	pixfmt->height = height;
	pixfmt->pixelformat = pixelformat;
	pixfmt->num_planes = info->mem_planes;

	/* treat info as mem_planes = 1 */
	if (info->mem_planes != 1) {
		pr_info("%s: failed wrong mem_planes(%d) for " FMT_FOURCC "\n",
			__func__, info->mem_planes, MEMBER_FOURCC(pixelformat));
		return -EINVAL;
	}


	plane = &pixfmt->plane_fmt[0];
	stride = v4l2_format_calc_stride(info, 0, width,
					 plane->bytesperline);

	plane->bytesperline = stride;
	plane->sizeimage = 0;
	for (i = 0; i < info->comp_planes; i++) {
		unsigned int stride_p;

		stride_p = v4l2_format_calc_stride(info, i, width, stride);

		plane->sizeimage +=
			v4l2_format_calc_planesize(info, i, height, stride_p);
	}
	return 0;
}

static int mtk_cam_fill_mtk_pixfmt_mp(const struct mtk_format_info *info,
				      struct v4l2_pix_format_mplane *pixfmt,
				      u32 pixelformat, u32 width, u32 height)
{
	struct v4l2_plane_pix_format *plane;
	unsigned int stride;
	int i;

	pixfmt->width = width;
	pixfmt->height = height;
	pixfmt->pixelformat = pixelformat;
	pixfmt->num_planes = info->mem_planes;

	/* treat info as mem_planes = 1 */
	if (info->mem_planes != 1) {
		pr_info("%s: failed wrong mem_planes(%d) for " FMT_FOURCC "\n",
			__func__, info->mem_planes, MEMBER_FOURCC(pixelformat));
		return -EINVAL;
	}

	plane = &pixfmt->plane_fmt[0];

	if (is_yuv_ufo(pixelformat)) {
		/* NOTICE:
		 * ufbc fmt is special case, user fill stride to buffer size,
		 * so, don't use it to calculate image size.
		 */
		u32 aligned_width;

		/* UFO format width should align 64 pixel */
		aligned_width = ALIGN(width, 64);
		stride = aligned_width * info->bitpp[0] / 8;

		plane->sizeimage = stride * height;
		plane->sizeimage += stride * height / 2;
		plane->sizeimage += ALIGN((aligned_width / 64), 8) * height;
		plane->sizeimage += ALIGN((aligned_width / 64), 8) * height / 2;
		plane->sizeimage += sizeof(struct UfbcBufferHeader);

		plane->bytesperline = max(plane->bytesperline, stride);
	} else if (is_raw_ufo(pixelformat)) {
		if (CAM_DEBUG_ENABLED(V4L2))
			pr_info("%s: raw ufo: (%d/%d/Bpl:%d)",
				__func__, width, height, plane->bytesperline);

		get_bayer_ufbc_stride_and_size(
			width, height, info, plane->bytesperline,
			&plane->bytesperline, &plane->sizeimage);

		if (CAM_DEBUG_ENABLED(V4L2))
			pr_info("%s: raw ufo: (Bpl:%d/sz:%d)",
				__func__, plane->bytesperline, plane->sizeimage);
	} else {

		stride = mtk_format_calc_stride(info, 0, width,
						plane->bytesperline);

		plane->bytesperline = stride;

		plane->sizeimage = 0;
		for (i = 0; i < info->comp_planes; i++) {
			unsigned int stride_p;

			stride_p = mtk_format_calc_stride(info, i, width,
							  stride);

			plane->sizeimage +=
				mtk_format_calc_planesize(info, i, height,
							  stride_p);
		}
	}
	return 0;
}

static int _fill_image_pix_mp(struct v4l2_pix_format_mplane *mp,
			      u32 pixelformat, u32 width, u32 height)
{
	const struct mtk_format_info *mtk_info;
	const struct v4l2_format_info *v4l2_info;
	int ret;

	mtk_info = mtk_format_info(pixelformat);
	v4l2_info = mtk_info ? NULL : v4l2_format_info(pixelformat);

	if (mtk_info)
		ret = mtk_cam_fill_mtk_pixfmt_mp(mtk_info, mp,
						 pixelformat, width, height);
	else if (v4l2_info)
		ret = mtk_cam_fill_v4l2_pixfmt_mp(v4l2_info, mp,
						  pixelformat, width, height);
	else {
		pr_info("%s: not found pixelformat " FMT_FOURCC "\n",
			__func__, MEMBER_FOURCC(mp->pixelformat));
		ret = -EINVAL;
	}

	return ret;
}

static u32 try_fmt_mp_pixelformat(struct mtk_cam_dev_node_desc *desc,
				  u32 pixelformat)
{
	const struct v4l2_format *dev_fmt;

	/* Validate pixelformat */
	dev_fmt = mtk_cam_dev_find_fmt(desc, pixelformat);
	if (!dev_fmt) {

		if (CAM_DEBUG_ENABLED(V4L2_TRY))
			pr_info("%s: warn. %s not-supportd pixelformat " FMT_FOURCC "\n",
				__func__, desc->name,
				MEMBER_FOURCC(pixelformat));

		/* use default instead */
		dev_fmt = &desc->fmts[desc->default_fmt_idx].vfmt;

		if (CAM_DEBUG_ENABLED(V4L2_TRY))
			pr_info("%s: warn. use pixelformat " FMT_FOURCC " instead.\n",
				__func__,
				MEMBER_FOURCC(dev_fmt->fmt.pix_mp.pixelformat));
	}

	return dev_fmt->fmt.pix_mp.pixelformat;
}

static int fill_fmt_mp_constrainted_by_hw(struct v4l2_pix_format_mplane *fmt,
					  struct mtk_cam_dev_node_desc *desc,
					  u32 pixelformat, u32 width, u32 height)
{
	int ret;

	width = clamp_val(ALIGN(width, 2),
			  IMG_MIN_WIDTH, desc->frmsizes->stepwise.max_width);
	height = clamp_val(ALIGN(height, 2),
			   IMG_MIN_HEIGHT, desc->frmsizes->stepwise.max_height);

	ret = _fill_image_pix_mp(fmt, pixelformat, width, height);

	return 0;
}

static int mtk_video_init_format(struct mtk_cam_video_device *video)
{
	struct mtk_cam_dev_node_desc *desc = &video->desc;
	struct v4l2_format *active = &video->active_fmt;
	const struct v4l2_format *default_fmt =
		&desc->fmts[desc->default_fmt_idx].vfmt;

	active->type = desc->buf_type;

	if (!default_fmt)
		return -1;

	if (!desc->image) {
		active->fmt.meta.dataformat = default_fmt->fmt.meta.dataformat;
		active->fmt.meta.buffersize = default_fmt->fmt.meta.buffersize;
		return 0;
	}

	active->fmt.pix_mp.pixelformat = default_fmt->fmt.pix_mp.pixelformat;

	fill_fmt_mp_constrainted_by_hw(&active->fmt.pix_mp,
				       desc,
				       default_fmt->fmt.pix_mp.pixelformat,
				       default_fmt->fmt.pix_mp.width,
				       default_fmt->fmt.pix_mp.height);

	/**
	 * TODO: to support multi-plane: for example, yuv or do it as
	 * following?
	 */
	active->fmt.pix_mp.num_planes = 1;
	active->fmt.pix_mp.field = V4L2_FIELD_NONE;
	active->fmt.pix_mp.colorspace = V4L2_COLORSPACE_SRGB;
	active->fmt.pix_mp.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	active->fmt.pix_mp.quantization = V4L2_QUANTIZATION_DEFAULT;
	active->fmt.pix_mp.xfer_func = V4L2_XFER_FUNC_SRGB;

	mtk_cam_video_cache_fmt(video);

	return 0;
}

int mtk_cam_video_register(struct mtk_cam_video_device *video,
			   struct v4l2_device *v4l2_dev)
{
	struct mtk_cam_device *cam =
		container_of(v4l2_dev, struct mtk_cam_device, v4l2_dev);
	struct media_pad *pad = &video->pad;
	struct video_device *vdev = &video->vdev;
	struct vb2_queue *q = &video->vb2_q;
	unsigned int output = V4L2_TYPE_IS_OUTPUT(video->desc.buf_type);
	int ret;

	if (video->desc.link_flags & MEDIA_LNK_FL_ENABLED)
		video->enabled = true;
	else
		video->enabled = false;

	mutex_init(&video->q_lock);

	/* initialize vb2_queue */
	q->type = video->desc.buf_type;
	q->io_modes = VB2_MMAP | VB2_DMABUF;

	if (q->type == V4L2_BUF_TYPE_META_OUTPUT || video->desc.id == MTK_RAW_META_OUT_1)
		q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	else
		/**
		 *  Actually we want to configure it as boot time but
		 *  there is no such option now. We will upstream
		 *  a new flag such as V4L2_BUF_FLAG_TIMESTAMP_BOOT
		 *  and use that in the future.
		 */
		q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_BOOT;

	if (video->desc.smem_alloc) {
		q->bidirectional = 1;
		/* q->dev = cam->smem_dev; FIXME impl for real SCP */
		q->dev = cam->dev;
	} else {
		if (video->uid.pipe_id >= MTKCAM_SUBDEV_CAMSV_START &&
			video->uid.pipe_id < MTKCAM_SUBDEV_CAMSV_END) {
			q->dev = cam->engines.sv_devs[0];
		} else if (video->uid.pipe_id >= MTKCAM_SUBDEV_MRAW_START &&
			video->uid.pipe_id < MTKCAM_SUBDEV_MRAW_END) {
			q->dev =
				cam->engines.mraw_devs[video->uid.pipe_id - MTKCAM_SUBDEV_MRAW_START];
		} else {
			switch (video->desc.id) {
			case MTK_RAW_YUVO_1_OUT:
			case MTK_RAW_YUVO_2_OUT:
			case MTK_RAW_YUVO_3_OUT:
			case MTK_RAW_YUVO_4_OUT:
			case MTK_RAW_YUVO_5_OUT:
			case MTK_RAW_DRZS4NO_1_OUT:
			case MTK_RAW_DRZS4NO_3_OUT:
			case MTK_RAW_RZH1N2TO_1_OUT:
			case MTK_RAW_RZH1N2TO_2_OUT:
			case MTK_RAW_RZH1N2TO_3_OUT:
				/* should have a better implementation here */
				q->dev = cam->engines.yuv_devs[0];
				break;
			default:
				q->dev = cam->engines.raw_devs[0];
				break;
			}
		}
	}

	q->supports_requests = true;
	q->lock = &video->q_lock;
	q->ops = &mtk_cam_vb2_ops;
	q->mem_ops = &mtk_cam_dma_contig_memops;
	q->drv_priv = cam;
	q->buf_struct_size = sizeof(struct mtk_cam_buffer);

	if (output)
		q->timestamp_flags |= V4L2_BUF_FLAG_TSTAMP_SRC_EOF;
	else
		q->timestamp_flags |= V4L2_BUF_FLAG_TSTAMP_SRC_SOE;

	/* No minimum buffers limitation */
	q->min_buffers_needed = 0;

	ret = vb2_queue_init(q);
	if (ret < 0) {
		dev_info(v4l2_dev->dev, "Failed to init vb2 queue: %d\n", ret);
		goto error_vb2_init;
	}

	pad->flags = output ? MEDIA_PAD_FL_SOURCE : MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&vdev->entity, 1, pad);
	if (ret < 0) {
		dev_info(v4l2_dev->dev, "Failed to init video entity: %d\n",
			ret);
		goto error_media_init;
	}

	ret = mtk_video_init_format(video);
	if (ret < 0) {
		dev_info(v4l2_dev->dev, "Failed to init format: %d\n", ret);
		goto error_video_register;
	}

	vdev->entity.function = MEDIA_ENT_F_IO_V4L;
	vdev->entity.ops = NULL;
	vdev->fops = &mtk_cam_v4l2_fops;
	vdev->device_caps = video->desc.cap | V4L2_CAP_STREAMING;
	vdev->v4l2_dev = v4l2_dev;

	vdev->vfl_dir = output ? VFL_DIR_TX : VFL_DIR_RX;
	vdev->queue = &video->vb2_q;
	vdev->ioctl_ops = video->desc.ioctl_ops;
	vdev->release = video_device_release_empty;
	/* TODO: share q_lock or use another lock? */
	vdev->lock = &video->q_lock;
	strscpy(vdev->name, video->desc.name, sizeof(vdev->name));

	ret = video_register_device(vdev, VFL_TYPE_VIDEO, CAMSYS_VIDEO_DEV_NR);
	if (ret < 0) {
		dev_info(v4l2_dev->dev, "Failed to register video device: %d\n",
			ret);
		goto error_video_register;
	}
	video_set_drvdata(vdev, cam);

	dev_dbg(v4l2_dev->dev, "registered vdev:%d:%s\n",
		video->desc.id, vdev->name);

	return 0;

error_video_register:
	media_entity_cleanup(&vdev->entity);
error_media_init:
	vb2_queue_release(&video->vb2_q);
error_vb2_init:
	mutex_destroy(&video->q_lock);

	return ret;
}

void mtk_cam_video_unregister(struct mtk_cam_video_device *video)
{
	video_unregister_device(&video->vdev);
	vb2_queue_release(&video->vb2_q);
	media_entity_cleanup(&video->vdev.entity);
	mutex_destroy(&video->q_lock);
}

const struct v4l2_format *
mtk_cam_dev_find_fmt(struct mtk_cam_dev_node_desc *desc, u32 format)
{
	unsigned int i;
	const struct v4l2_format *fmt;

	for (i = 0; i < desc->num_fmts; i++) {
		fmt = &desc->fmts[i].vfmt;
		if (desc->buf_type == V4L2_BUF_TYPE_META_OUTPUT ||
			desc->buf_type == V4L2_BUF_TYPE_META_CAPTURE) {
			if (fmt->fmt.meta.dataformat == format)
				return fmt;
		} else if (fmt->fmt.pix_mp.pixelformat == format)
			return fmt;
	}

	return NULL;
}

int mtk_cam_vidioc_querycap(struct file *file, void *fh,
			    struct v4l2_capability *cap)
{
	struct mtk_cam_device *cam = video_drvdata(file);

	strscpy(cap->driver, dev_driver_string(cam->dev), sizeof(cap->driver));
	strscpy(cap->card, dev_driver_string(cam->dev), sizeof(cap->card));
	(void)snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		       dev_name(cam->dev));

	return 0;
}

int mtk_cam_vidioc_enum_framesizes(struct file *filp, void *priv,
				   struct v4l2_frmsizeenum *sizes)
{
	struct mtk_cam_video_device *node = file_to_mtk_cam_node(filp);
	const struct v4l2_format *dev_fmt;

	dev_fmt = mtk_cam_dev_find_fmt(&node->desc, sizes->pixel_format);
	if (!dev_fmt || sizes->index)
		return -EINVAL;

	sizes->type = node->desc.frmsizes->type;
	memcpy(&sizes->stepwise, &node->desc.frmsizes->stepwise,
	       sizeof(sizes->stepwise));
	return 0;
}

int mtk_cam_vidioc_enum_fmt(struct file *file, void *fh,
			    struct v4l2_fmtdesc *f)
{
	struct mtk_cam_video_device *node = file_to_mtk_cam_node(file);

	if (f->index >= node->desc.num_fmts)
		return -EINVAL;

	f->pixelformat = node->desc.fmts[f->index].vfmt.fmt.pix_mp.pixelformat;
	fill_ext_mtkcam_fmtdesc(f);
	f->flags = 0;
	return 0;
}

int mtk_cam_vidioc_g_fmt(struct file *file, void *fh,
			 struct v4l2_format *f)
{
	struct mtk_cam_video_device *node = file_to_mtk_cam_node(file);

	f->fmt = node->active_fmt.fmt;

	if (CAM_DEBUG_ENABLED(V4L2))
		log_fmt_ops(node, f, __func__);

	return 0;
}

int mtk_cam_vidioc_s_fmt(struct file *file, void *fh,
			 struct v4l2_format *f)
{
	struct mtk_cam_video_device *node = file_to_mtk_cam_node(file);
	int ret;

	ret = mtk_cam_video_set_fmt(node, f);
	if (!ret) {
		node->active_fmt = *f;

		mtk_cam_video_cache_fmt(node);

		/* display ic */
		if (is_camsv_subdev(node->uid.pipe_id))
			mtk_cam_sv_update_feature(node);
	}

	if (CAM_DEBUG_ENABLED(V4L2))
		log_fmt_ops(node, f, __func__);

	return ret;
}

int mtk_cam_video_set_fmt(struct mtk_cam_video_device *node,
			  struct v4l2_format *f)
{
	//struct mtk_cam_device *cam = video_get_drvdata(&node->vdev);
	struct mtk_cam_dev_node_desc *desc = &node->desc;
	struct v4l2_format try_fmt;
	u32 pixelformat;

	try_fmt.type = f->type;
	try_fmt.fmt.pix_mp = f->fmt.pix_mp;

	pixelformat = try_fmt_mp_pixelformat(desc,
					     f->fmt.pix_mp.pixelformat);
	fill_fmt_mp_constrainted_by_hw(&try_fmt.fmt.pix_mp,
				       desc,
				       pixelformat,
				       f->fmt.pix_mp.width,
				       f->fmt.pix_mp.height);

	/* Constant format fields */
	try_fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
	try_fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_SRGB;
	try_fmt.fmt.pix_mp.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	try_fmt.fmt.pix_mp.quantization = V4L2_QUANTIZATION_DEFAULT;
	try_fmt.fmt.pix_mp.xfer_func = V4L2_XFER_FUNC_SRGB;

	*f = try_fmt;

	return 0;
}

int mtk_cam_vidioc_try_fmt(struct file *file, void *fh,
			   struct v4l2_format *f)
{
	struct mtk_cam_video_device *node = file_to_mtk_cam_node(file);
	int ret;

	ret = mtk_cam_video_set_fmt(node, f);

	if (is_camsv_subdev(node->uid.pipe_id))
		ret |= mtk_cam_sv_update_image_size(node, f);

	if (CAM_DEBUG_ENABLED(V4L2_TRY))
		log_fmt_ops(node, f, __func__);

	return ret;
}

int mtk_cam_vidioc_meta_enum_fmt(struct file *file, void *fh,
				 struct v4l2_fmtdesc *f)
{
	struct mtk_cam_video_device *node = file_to_mtk_cam_node(file);

	if (f->index)
		return -EINVAL;

	f->pixelformat = node->active_fmt.fmt.meta.dataformat;
	fill_ext_mtkcam_fmtdesc(f);
	f->flags = 0;

	return 0;
}

int mtk_cam_vidioc_g_meta_fmt(struct file *file, void *fh,
			      struct v4l2_format *f)
{
	struct mtk_cam_device *cam = video_drvdata(file);
	struct mtk_cam_video_device *node = file_to_mtk_cam_node(file);
#ifdef NOT_READY
	struct mtk_cam_dev_node_desc *desc = &node->desc;
	const struct v4l2_format *default_fmt =
		&desc->fmts[desc->default_fmt_idx].vfmt;
	u32 extmeta_size = 0;
#endif

#ifdef NOT_READY
	switch (node->desc.id) {
	case MTK_RAW_META_SV_OUT_0:
	case MTK_RAW_META_SV_OUT_1:
	case MTK_RAW_META_SV_OUT_2:
		if (node->enabled && node->ctx)
			extmeta_size = cam->raw.pipelines[node->uid.pipe_id]
				.cfg[MTK_RAW_META_SV_OUT_0].mbus_fmt.width *
				cam->raw.pipelines[node->uid.pipe_id]
				.cfg[MTK_RAW_META_SV_OUT_0].mbus_fmt.height;
		if (extmeta_size)
			node->active_fmt.fmt.meta.buffersize = extmeta_size;
		else
			node->active_fmt.fmt.meta.buffersize =
				CAMSV_EXT_META_0_WIDTH * CAMSV_EXT_META_0_HEIGHT;
		dev_dbg(cam->dev,
			"%s:extmeta name:%s buffersize:%d\n",
			__func__, node->desc.name, node->active_fmt.fmt.meta.buffersize);
		break;
	default:
		break;
	}
#endif

	f->fmt.meta.dataformat = node->active_fmt.fmt.meta.dataformat;
	f->fmt.meta.buffersize = node->active_fmt.fmt.meta.buffersize;
	dev_dbg(cam->dev,
		"%s: node:%s dataformat:%d buffersize:%d\n",
		__func__, node->desc.name, f->fmt.meta.dataformat, f->fmt.meta.buffersize);

	if (CAM_DEBUG_ENABLED(V4L2))
		log_fmt_ops(node, f, __func__);

	return 0;
}

int mtk_cam_vidioc_s_meta_fmt(struct file *file, void *fh,
			      struct v4l2_format *f)
{
	struct mtk_cam_video_device *node = file_to_mtk_cam_node(file);
	const struct v4l2_format *fmt;

	switch (node->desc.dma_port) {
	case MTKCAM_IPI_RAW_META_STATS_CFG:
	case MTKCAM_IPI_RAW_META_STATS_0:
	case MTKCAM_IPI_RAW_META_STATS_1:
		fmt = mtk_cam_dev_find_fmt(&node->desc, f->fmt.meta.dataformat);

		if (fmt) {
			node->active_fmt.fmt.meta.dataformat = fmt->fmt.meta.dataformat;
			node->active_fmt.fmt.meta.buffersize = f->fmt.meta.buffersize;
		}
		log_fmt_ops(node, f, __func__);
		return (fmt) ? 0 : -EINVAL;
	default:
		break;
	}

	return mtk_cam_vidioc_g_meta_fmt(file, fh, f);
}

int mtk_cam_vidioc_try_meta_fmt(struct file *file, void *fh,
			      struct v4l2_format *f)
{
	struct mtk_cam_video_device *node = file_to_mtk_cam_node(file);
	const struct v4l2_format *fmt;

	switch (node->desc.dma_port) {
	case MTKCAM_IPI_RAW_META_STATS_CFG:
	case MTKCAM_IPI_RAW_META_STATS_0:
	case MTKCAM_IPI_RAW_META_STATS_1:
		fmt = mtk_cam_dev_find_fmt(&node->desc, f->fmt.meta.dataformat);
		if (fmt) {
			f->fmt.meta.dataformat = fmt->fmt.meta.dataformat;
			f->fmt.meta.buffersize = max(f->fmt.meta.buffersize,
						     fmt->fmt.meta.buffersize);
		}

		if (CAM_DEBUG_ENABLED(V4L2_TRY))
			log_fmt_ops(node, f, __func__);

		return (fmt) ? 0 : -EINVAL;
	default:
		break;
	}

	return mtk_cam_vidioc_g_meta_fmt(file, fh, f);
}

int mtk_cam_vidioc_s_selection(struct file *file, void *fh,
				struct v4l2_selection *s)
{
	struct mtk_cam_video_device *node = file_to_mtk_cam_node(file);

	/* Note:
	 * this is workaround for current user who does not follow format
	 * negotiation...
	 */
	refine_valid_selection(node, s);
	node->active_crop = *s;

	if (node->desc.image)
		node->image_info.crop = s->r;

	return 0;
}

static struct mtk_cam_buffer *
mtk_cam_vb2_queue_get_mtkbuf(struct vb2_queue *q, struct v4l2_buffer *b)
{
	struct vb2_buffer *vb;

	if (b->index >= q->num_buffers) {
		dev_info(q->dev, "%s: buffer index out of range (idx/num: %d/%d)\n",
			 __func__, b->index, q->num_buffers);
		return NULL;
	}

	vb = q->bufs[b->index];
	if (vb == NULL) {
		/* Should never happen */
		dev_info(q->dev, "%s: buffer is NULL\n", __func__);
		return NULL;
	}

	return mtk_cam_vb2_buf_to_dev_buf(vb);
}

int mtk_cam_vidioc_qbuf(struct file *file, void *priv,
			struct v4l2_buffer *buf)
{
	struct video_device *vdev = video_devdata(file);
	struct mtk_cam_buffer *cam_buf;

	cam_buf = mtk_cam_vb2_queue_get_mtkbuf(vdev->queue, buf);
	if (cam_buf == NULL)
		return -EINVAL;

	cam_buf->flags = 0;
	if (buf->flags & V4L2_BUF_FLAG_NO_CACHE_CLEAN)
		cam_buf->flags |= FLAG_NO_CACHE_CLEAN;

	if (buf->flags & V4L2_BUF_FLAG_NO_CACHE_INVALIDATE)
		cam_buf->flags |= FLAG_NO_CACHE_INVALIDATE;

	return vb2_qbuf(vdev->queue, vdev->v4l2_dev->mdev, buf);
}
