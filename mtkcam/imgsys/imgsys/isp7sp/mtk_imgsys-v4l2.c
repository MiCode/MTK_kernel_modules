// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Frederic Chen <frederic.chen@mediatek.com>
 *
 */

#include <linux/hashtable.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include <linux/suspend.h>
#include <linux/rtc.h>
//#include <linux/remoteproc/mtk_scp.h>
#include <linux/videodev2.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-event.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <mtk_imgsys-cmdq.h>
//#include <mtk_imgsys-data.h>
#include "mtk_imgsys_v4l2.h"
#include "mtk_imgsys-dev.h"
#include "mtk_imgsys-hw.h"
#include "mtk_imgsys-requesttrack.h"
#include "mtk-hcp.h"
#include "mtk_imgsys_v4l2_vnode.h"
#include "mtk_imgsys-v4l2-debug.h"


#include "mtk_imgsys-probe.h"
#include "iommu_debug.h"
#define CLK_READY


static struct device *imgsys_pm_dev;

static int mtk_imgsys_sd_subscribe_event(struct v4l2_subdev *subdev,
				      struct v4l2_fh *fh,
				      struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_FRAME_SYNC:
		return v4l2_event_subscribe(fh, sub, 64, NULL);
	default:
		return -EINVAL;
	}
}


static int mtk_imgsys_subdev_s_stream(struct v4l2_subdev *sd,
				   int enable)
{
	struct mtk_imgsys_pipe *pipe = mtk_imgsys_subdev_to_pipe(sd);
	int ret;

	if (enable) {
		ret = mtk_imgsys_hw_streamon(pipe);
		if (ret)
			dev_info(pipe->imgsys_dev->dev,
				"%s:%s: pipe(%d) streamon failed\n",
				__func__, pipe->desc->name, pipe->desc->id);
	} else {
		ret = mtk_imgsys_hw_streamoff(pipe);
		if (ret)
			dev_info(pipe->imgsys_dev->dev,
				"%s:%s: pipe(%d) streamon off with errors\n",
				__func__, pipe->desc->name, pipe->desc->id);
	}

	return ret;
}

static int mtk_imgsys_subdev_get_fmt(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_format *fmt)
{
	struct mtk_imgsys_pipe *imgsys_pipe = mtk_imgsys_subdev_to_pipe(sd);
	struct v4l2_mbus_framefmt *mf;

	u32 pad = fmt->pad;

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
		fmt->format = imgsys_pipe->nodes[pad].pad_fmt;
	} else {
#if (KERNEL_VERSION(6, 8, 0) > LINUX_VERSION_CODE)
		mf = v4l2_subdev_get_try_format(sd, state, pad);
#else
		mf = v4l2_subdev_state_get_format(state, pad);
#endif
		fmt->format = *mf;
	}

    if (imgsys_dbg_enable())
	dev_dbg(imgsys_pipe->imgsys_dev->dev,
		"get(try:%d) node:%s(pad:%d)fmt: %dx%d",
		fmt->which, imgsys_pipe->nodes[pad].desc->name, pad,
		fmt->format.width, fmt->format.height);

	return 0;
}

static int mtk_imgsys_subdev_set_fmt(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_format *fmt)
{
	struct mtk_imgsys_pipe *imgsys_pipe = mtk_imgsys_subdev_to_pipe(sd);
	struct v4l2_mbus_framefmt *mf;
	u32 pad = fmt->pad;

    if (imgsys_dbg_enable())
	dev_dbg(imgsys_pipe->imgsys_dev->dev, "set(try:%d) node:%s(pad:%d)fmt: %dx%d",
		fmt->which, imgsys_pipe->nodes[pad].desc->name, pad,
		fmt->format.width, fmt->format.height);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
#if (KERNEL_VERSION(6, 8, 0) > LINUX_VERSION_CODE)
		mf = v4l2_subdev_get_try_format(sd, state, pad);
#else
		mf = v4l2_subdev_state_get_format(state, pad);
#endif
	else
		mf = &imgsys_pipe->nodes[pad].pad_fmt;

	fmt->format.code = mf->code;

	/* Clamp the w and h based on the hardware capabilities */
	if (imgsys_pipe->subdev_pads[pad].flags & MEDIA_PAD_FL_SOURCE) {
		fmt->format.width = clamp(fmt->format.width,
					  MTK_DIP_CAPTURE_MIN_WIDTH,
					  MTK_DIP_CAPTURE_MAX_WIDTH);
		fmt->format.height = clamp(fmt->format.height,
					   MTK_DIP_CAPTURE_MIN_HEIGHT,
					   MTK_DIP_CAPTURE_MAX_HEIGHT);
	} else {
		fmt->format.width = clamp(fmt->format.width,
					  MTK_DIP_OUTPUT_MIN_WIDTH,
					  MTK_DIP_OUTPUT_MAX_WIDTH);
		fmt->format.height = clamp(fmt->format.height,
					   MTK_DIP_OUTPUT_MIN_WIDTH,
					   MTK_DIP_OUTPUT_MAX_WIDTH);
	}

	*mf = fmt->format;

	return 0;
}

static int mtk_imgsys_subdev_get_selection(struct v4l2_subdev *sd,
					struct v4l2_subdev_state *state,
					struct v4l2_subdev_selection *sel)
{
	struct v4l2_rect *try_sel, *r;
	struct mtk_imgsys_pipe *imgsys_pipe = mtk_imgsys_subdev_to_pipe(sd);

    if (imgsys_dbg_enable())
	dev_dbg(imgsys_pipe->imgsys_dev->dev,
		"get subdev %s sel which %d target 0x%4x rect [%dx%d]",
		imgsys_pipe->desc->name, sel->which, sel->target,
		sel->r.width, sel->r.height);

	if (sel->pad != MTK_IMGSYS_VIDEO_NODE_ID_WROT_A_CAPTURE &&
	    sel->pad != MTK_IMGSYS_VIDEO_NODE_ID_WROT_B_CAPTURE) {
	    if (imgsys_dbg_enable())
		dev_dbg(imgsys_pipe->imgsys_dev->dev,
			"g_select failed(%s:%d):not support\n",
			imgsys_pipe->nodes[sel->pad].desc->name, sel->pad);
		return -EINVAL;
	}

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
#if (KERNEL_VERSION(6, 8, 0) > LINUX_VERSION_CODE)
		try_sel = v4l2_subdev_get_try_crop(sd, state, sel->pad);
#else
		try_sel = v4l2_subdev_state_get_crop(state, sel->pad);
#endif
		/* effective resolution */
		r = &imgsys_pipe->nodes[sel->pad].crop;
		break;
	default:
        if (imgsys_dbg_enable())
		dev_dbg(imgsys_pipe->imgsys_dev->dev,
			"s_select failed(%s:%d):target(%d) not support\n",
			imgsys_pipe->nodes[sel->pad].desc->name, sel->pad,
			sel->target);
		return -EINVAL;
	}

	if (sel->which == V4L2_SUBDEV_FORMAT_TRY)
		sel->r = *try_sel;
	else
		sel->r = *r;

	return 0;
}

static int mtk_imgsys_subdev_set_selection(struct v4l2_subdev *sd,
					struct v4l2_subdev_state *state,
					struct v4l2_subdev_selection *sel)
{
	struct v4l2_rect *rect, *try_sel;
	struct mtk_imgsys_pipe *imgsys_pipe = mtk_imgsys_subdev_to_pipe(sd);

    if (imgsys_dbg_enable())
	dev_dbg(imgsys_pipe->imgsys_dev->dev,
		"set subdev %s sel which %d target 0x%4x rect [%dx%d]",
		imgsys_pipe->desc->name, sel->which, sel->target,
		sel->r.width, sel->r.height);

	if (sel->pad != MTK_IMGSYS_VIDEO_NODE_ID_WROT_A_CAPTURE &&
	    sel->pad != MTK_IMGSYS_VIDEO_NODE_ID_WROT_B_CAPTURE) {
	    if (imgsys_dbg_enable())
		dev_dbg(imgsys_pipe->imgsys_dev->dev,
			"g_select failed(%s:%d):not support\n",
			imgsys_pipe->nodes[sel->pad].desc->name, sel->pad);
		return -EINVAL;
	}

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
#if (KERNEL_VERSION(6, 8, 0) > LINUX_VERSION_CODE)
		try_sel = v4l2_subdev_get_try_crop(sd, state, sel->pad);
#else
		try_sel = v4l2_subdev_state_get_crop(state, sel->pad);
#endif
		rect = &imgsys_pipe->nodes[sel->pad].crop;
		break;
	default:
        if (imgsys_dbg_enable())
		dev_dbg(imgsys_pipe->imgsys_dev->dev,
			"s_select failed(%s:%d):target(%d) not support\n",
			imgsys_pipe->nodes[sel->pad].desc->name, sel->pad,
			sel->target);
		return -EINVAL;
	}

	if (sel->which == V4L2_SUBDEV_FORMAT_TRY)
		*try_sel = sel->r;
	else
		*rect = sel->r;

	return 0;
}

static int mtk_imgsys_link_setup(struct media_entity *entity,
			      const struct media_pad *local,
			      const struct media_pad *remote,
			      u32 flags)
{
	struct mtk_imgsys_pipe *pipe =
		container_of(entity, struct mtk_imgsys_pipe, subdev.entity);
	u32 pad = local->index;

	WARN_ON(entity->obj_type != MEDIA_ENTITY_TYPE_V4L2_SUBDEV);
	WARN_ON(pad >= pipe->desc->total_queues);

	mutex_lock(&pipe->lock);

	if (flags & MEDIA_LNK_FL_ENABLED)
		pipe->nodes_enabled++;
	else
		pipe->nodes_enabled--;

	pipe->nodes[pad].flags &= ~MEDIA_LNK_FL_ENABLED;
	pipe->nodes[pad].flags |= flags & MEDIA_LNK_FL_ENABLED;

    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
		"%s: link setup, flags(0x%x), (%s)%d -->(%s)%d, nodes_enabled(0x%llx)\n",
		pipe->desc->name, flags, local->entity->name, local->index,
		remote->entity->name, remote->index, pipe->nodes_enabled);

	mutex_unlock(&pipe->lock);

	return 0;
}

static int mtk_imgsys_vb2_meta_buf_prepare(struct vb2_buffer *vb)
{
	struct mtk_imgsys_pipe *pipe = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_imgsys_video_device *node =
					mtk_imgsys_vbq_to_node(vb->vb2_queue);
	struct device *dev = pipe->imgsys_dev->dev;
	const struct v4l2_format *fmt = &node->vdev_fmt;

	if (vb->planes[0].length < fmt->fmt.meta.buffersize) {
        if (imgsys_dbg_enable())
		dev_dbg(dev,
			"%s:%s:%s: size error(user:%d, required:%d)\n",
			__func__, pipe->desc->name, node->desc->name,
			vb->planes[0].length, fmt->fmt.meta.buffersize);
		return -EINVAL;
	}
#ifdef SIZE_CHECK
	if (vb->planes[0].bytesused != fmt->fmt.meta.buffersize) {
		dev_info(dev,
			"%s:%s:%s: bytesused(%d) must be %d\n",
			__func__, pipe->desc->name, node->desc->name,
			vb->planes[0].bytesused,
			fmt->fmt.meta.buffersize);
		return -EINVAL;
	}
#endif

	return 0;
}

static int mtk_imgsys_vb2_video_buf_prepare(struct vb2_buffer *vb)
{
	struct mtk_imgsys_pipe *pipe = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_imgsys_video_device *node =
					mtk_imgsys_vbq_to_node(vb->vb2_queue);
	struct device *dev = pipe->imgsys_dev->dev;
	const struct v4l2_format *fmt = &node->vdev_fmt;
	unsigned int size;
	int i;

	for (i = 0; i < vb->num_planes; i++) {
		size = fmt->fmt.pix_mp.plane_fmt[i].sizeimage;
		if (vb->planes[i].length < size) {
            if (imgsys_dbg_enable())
			dev_dbg(dev,
				"%s:%s:%s: size error(user:%d, max:%d)\n",
				__func__, pipe->desc->name, node->desc->name,
				vb->planes[i].length, size);
			return -EINVAL;
		}
	}

	return 0;
}

static int mtk_imgsys_vb2_buf_out_validate(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);

	if (v4l2_buf->field == V4L2_FIELD_ANY)
		v4l2_buf->field = V4L2_FIELD_NONE;

	if (v4l2_buf->field != V4L2_FIELD_NONE)
		return -EINVAL;

	return 0;
}

static int mtk_imgsys_vb2_meta_buf_init(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *b = to_vb2_v4l2_buffer(vb);
	struct mtk_imgsys_dev_buffer *dev_buf =
					mtk_imgsys_vb2_buf_to_dev_buf(vb);
	struct mtk_imgsys_pipe *pipe = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_imgsys_video_device *node =
					mtk_imgsys_vbq_to_node(vb->vb2_queue);
	/* phys_addr_t buf_paddr; */

    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
		"%s:%s:%s: buf type(%d), idx(%d), mem(%d)\n",
		__func__, pipe->desc->name, node->desc->name, b->vb2_buf.type,
		b->vb2_buf.index, b->vb2_buf.memory);

	if (b->vb2_buf.memory == VB2_MEMORY_DMABUF) {

		/* dev_buf->scp_daddr[0] = vb2_dma_contig_plane_dma_addr(vb, 0); */

		/*
		 * We got the incorrect physical address mapped when
		 * using dma_map_single() so I used dma_map_page_attrs()
		 * directly to workaround here. Please see the detail in
		 * mtk_imgsys-sys.c
		 */
		/* buf_paddr = dev_buf->scp_daddr[0];
		 * dev_buf->isp_daddr[0] =	dma_map_page_attrs(
		 *					pipe->imgsys_dev->dev,
		 *					phys_to_page(buf_paddr),
		 *					0, vb->planes[0].length,
		 *					DMA_BIDIRECTIONAL,
		 *					DMA_ATTR_SKIP_CPU_SYNC);
		 * if (dma_mapping_error(pipe->imgsys_dev->dev,
		 *		      dev_buf->isp_daddr[0])) {
		 *	dev_info(pipe->imgsys_dev->dev,
		 *		"%s:%s: failed to map buffer: s_daddr(0x%llx)\n",
		 *		pipe->desc->name, node->desc->name,
		 *		dev_buf->scp_daddr[0]);
		 *	return -EINVAL;
		 * }
		 */
	} else if (b->vb2_buf.memory == VB2_MEMORY_MMAP) {
#if MTK_CM4_SUPPORT == 0
		dev_buf->va_daddr[0] = (u64)vb2_plane_vaddr(vb, 0);
#endif
	}

    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
		"%s:%s: buf type(%d), idx(%d), mem(%d), p(%d) i_daddr(0x%llx), s_daddr(0x%llx), va_daddr(0x%llx)\n",
		pipe->desc->name, node->desc->name, b->vb2_buf.type,
		b->vb2_buf.index, b->vb2_buf.memory, 0,
		dev_buf->isp_daddr[0], dev_buf->scp_daddr[0],
		dev_buf->va_daddr[0]);

	return 0;
}

static int mtk_imgsys_vb2_video_buf_init(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *b = to_vb2_v4l2_buffer(vb);
	struct mtk_imgsys_dev_buffer *dev_buf =
					mtk_imgsys_vb2_buf_to_dev_buf(vb);
	struct mtk_imgsys_pipe *pipe = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_imgsys_video_device *node =
					mtk_imgsys_vbq_to_node(vb->vb2_queue);
	int i;

    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
		"%s:%s:%s: buf type(%d), idx(%d), mem(%d)\n",
		__func__, pipe->desc->name, node->desc->name, b->vb2_buf.type,
		b->vb2_buf.index, b->vb2_buf.memory);

	for (i = 0; i < vb->num_planes; i++) {
		dev_buf->scp_daddr[i] = 0;
		dev_buf->isp_daddr[i] =	vb2_dma_contig_plane_dma_addr(vb, i);

        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
			"%s:%s: buf type(%d), idx(%d), mem(%d), p(%d) i_daddr(0x%llx), s_daddr(0x%llx)\n",
			pipe->desc->name, node->desc->name, b->vb2_buf.type,
			b->vb2_buf.index, b->vb2_buf.memory, i,
			dev_buf->isp_daddr[i], dev_buf->scp_daddr[i]);
	}
	return 0;
}

static void mtk_imgsys_vb2_queue_meta_buf_cleanup(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *b = to_vb2_v4l2_buffer(vb);
	// struct mtk_imgsys_dev_buffer *dev_buf =
					// mtk_imgsys_vb2_buf_to_dev_buf(vb);
	// struct mtk_imgsys_pipe *pipe = vb2_get_drv_priv(vb->vb2_queue);
	if (b->vb2_buf.memory == VB2_MEMORY_DMABUF) {
		// dma_unmap_page_attrs(pipe->imgsys_dev->dev,
				     // dev_buf->isp_daddr[0],
				     // vb->planes[0].length, DMA_BIDIRECTIONAL,
				     // DMA_ATTR_SKIP_CPU_SYNC);
	}
}

static void mtk_imgsys_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *b = to_vb2_v4l2_buffer(vb);
	struct mtk_imgsys_dev_buffer *dev_buf =
					mtk_imgsys_vb2_buf_to_dev_buf(vb);
	struct mtk_imgsys_request *req =
				mtk_imgsys_media_req_to_imgsys_req(vb->request);
	struct mtk_imgsys_video_device *node =
					mtk_imgsys_vbq_to_node(vb->vb2_queue);
	struct mtk_imgsys_pipe *pipe = vb2_get_drv_priv(vb->vb2_queue);
	int buf_count;

	dev_buf->dev_fmt = node->dev_q.dev_fmt;
	//support std mode dynamic change buf info & fmt
	//get default node info to devbuf for desc mode(+)
	if (is_desc_fmt(dev_buf->dev_fmt)) {
		dev_buf->fmt = node->vdev_fmt;
		dev_buf->dma_port = node->desc->dma_port;
		dev_buf->rotation = node->rotation;
		dev_buf->crop.c = node->crop;
		dev_buf->compose = node->compose;
	}
	//get qbuf dynamic info to devbuf for desc mode(-)
#ifdef BUF_QUEUE_LOG
    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
			"[%s] portid(%d), rotat(%d), hflip(%d), vflip(%d)\n",
			__func__,
			dev_buf->dma_port,
			dev_buf->rotation,
			dev_buf->hflip,
			dev_buf->vflip);

	for (i = 0; i < dev_buf->fmt.fmt.pix_mp.num_planes; i++) {
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
			"[%s] width(%d), width(%d), sizeimage(%d), bytesperline(%d)\n",
			__func__,
			dev_buf->fmt.fmt.pix_mp.width,
			dev_buf->fmt.fmt.pix_mp.height,
			dev_buf->fmt.fmt.pix_mp.plane_fmt[i].sizeimage,
			dev_buf->fmt.fmt.pix_mp.plane_fmt[i].bytesperline);
	}

    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
		"%s:%s: buf type(%d), idx(%d), mem(%d), i_daddr(0x%llx), s_daddr(0x%llx)\n",
		req->imgsys_pipe->desc->name, node->desc->name, b->vb2_buf.type,
		b->vb2_buf.index, b->vb2_buf.memory, dev_buf->isp_daddr[0],
		dev_buf->scp_daddr[0]);
#endif
	spin_lock(&node->buf_list_lock);
	list_add_tail(&dev_buf->list, &node->buf_list);
	spin_unlock(&node->buf_list_lock);

	buf_count = atomic_dec_return(&req->buf_count);
	if (!buf_count) {
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
			"framo_no: (%d), reqfd-%d\n",
			req->img_fparam.frameparam.frame_no, b->request_fd);
		req->tstate.req_fd = b->request_fd;
		mutex_lock(&req->imgsys_pipe->lock);
		mtk_imgsys_pipe_try_enqueue(req->imgsys_pipe);
		mutex_unlock(&req->imgsys_pipe->lock);
	}
#ifdef REQ_TIMESTAMP
	req->tstate.time_qreq = ktime_get_boottime_ns()/1000;
#endif
}

static int mtk_imgsys_vb2_meta_queue_setup(struct vb2_queue *vq,
					unsigned int *num_buffers,
					unsigned int *num_planes,
					unsigned int sizes[],
					struct device *alloc_devs[])
{
	struct mtk_imgsys_pipe *pipe = vb2_get_drv_priv(vq);
	struct mtk_imgsys_video_device *node = mtk_imgsys_vbq_to_node(vq);
	const struct v4l2_format *fmt = &node->vdev_fmt;
	unsigned int size;

	if (!*num_planes) {
		*num_planes = 1;
		sizes[0] = fmt->fmt.meta.buffersize;
	}

	/* vq->dma_attrs |= DMA_ATTR_NON_CONSISTENT; */
		size = fmt->fmt.meta.buffersize;
		*num_buffers = clamp_val(*num_buffers, 1, VB2_MAX_FRAME);
		sizes[0] = size;

    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
		"%s:%s:%s: n_p(%d), n_b(%d), s[%d](%u)\n",
		__func__, pipe->desc->name, node->desc->name,
		*num_planes, *num_buffers, 0, sizes[0]);
	return 0;
}

static int mtk_imgsys_vb2_video_queue_setup(struct vb2_queue *vq,
					 unsigned int *num_buffers,
					 unsigned int *num_planes,
					 unsigned int sizes[],
					 struct device *alloc_devs[])
{
	struct mtk_imgsys_pipe *pipe = vb2_get_drv_priv(vq);
	struct mtk_imgsys_video_device *node = mtk_imgsys_vbq_to_node(vq);
	const struct v4l2_format *fmt = &node->vdev_fmt;
	unsigned int size;
	int i;

	if (!*num_planes) {
		*num_planes = 1;
		sizes[0] = fmt->fmt.pix_mp.plane_fmt[0].sizeimage;
	}

	for (i = 0; i < *num_planes; i++) {
		if (sizes[i] < fmt->fmt.pix_mp.plane_fmt[i].sizeimage) {
			size = fmt->fmt.pix_mp.plane_fmt[i].sizeimage;
            if (imgsys_dbg_enable())
			dev_dbg(pipe->imgsys_dev->dev,
				"%s:%s:%s: invalid buf: %u < %u\n",
				__func__, pipe->desc->name,
				node->desc->name, sizes[0], size);
				return -EINVAL;
		}
		sizes[i] = fmt->fmt.pix_mp.plane_fmt[i].sizeimage;
		*num_buffers = clamp_val(*num_buffers, 1,
					 VB2_MAX_FRAME);

        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
			"%s:%s:%s: n_p(%d), n_b(%d), s[%d](%u)\n",
			__func__, pipe->desc->name, node->desc->name,
			*num_planes, *num_buffers, i, sizes[i]);
	}
	return 0;
}

static unsigned long mtk_imgsys_vb2_get_contiguous_size(struct sg_table *sgt)
{
	struct scatterlist *s;
	dma_addr_t expected = sg_dma_address(sgt->sgl);
	unsigned int i;
	unsigned long size = 0;

	for_each_sgtable_dma_sg(sgt, s, i) {
		if (sg_dma_address(s) != expected)
			break;
		expected += sg_dma_len(s);
		size += sg_dma_len(s);
	}
	return size;
}

/*********************************************/
/*         callbacks for all buffers         */
/*********************************************/

static void *mtk_imgsys_vb2_cookie(struct vb2_buffer *vb, void *buf_priv)
{
	struct mtk_imgsys_vb2_buf *buf = buf_priv;

	return &buf->dma_addr;
}

static void *mtk_imgsys_vb2_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct mtk_imgsys_vb2_buf *buf = buf_priv;

	if (buf->vaddr)
		return buf->vaddr;

	if (buf->db_attach) {
		struct iosys_map map;
		#ifdef NEW_DMA_BUF_API
		if (!dma_buf_vmap_unlocked(buf->db_attach->dmabuf, &map))
			buf->vaddr = map.vaddr;
		#else
		if (!dma_buf_vmap(buf->db_attach->dmabuf, &map))
			buf->vaddr = map.vaddr;
		#endif
		return buf->vaddr;
	}

	if (buf->non_coherent_mem)
		buf->vaddr = dma_vmap_noncontiguous(buf->dev, buf->size,
						    buf->dma_sgt);
	return buf->vaddr;
}

static void mtk_imgsys_vb2_prepare(void *buf_priv)
{
	struct mtk_imgsys_vb2_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;

	/* This takes care of DMABUF and user-enforced cache sync hint */
	if (buf->vb->skip_cache_sync_on_prepare)
		return;

	if (!buf->non_coherent_mem)
		return;

	/* Non-coherent MMAP only */
	if (buf->vaddr)
		flush_kernel_vmap_range(buf->vaddr, buf->size);

	/* For both USERPTR and non-coherent MMAP */
	dma_sync_sgtable_for_device(buf->dev, sgt, buf->dma_dir);
}

static void mtk_imgsys_vb2_finish(void *buf_priv)
{
	struct mtk_imgsys_vb2_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;

	/* This takes care of DMABUF and user-enforced cache sync hint */
	if (buf->vb->skip_cache_sync_on_finish)
		return;

	if (!buf->non_coherent_mem)
		return;

	/* Non-coherent MMAP only */
	if (buf->vaddr)
		invalidate_kernel_vmap_range(buf->vaddr, buf->size);

	/* For both USERPTR and non-coherent MMAP */
	dma_sync_sgtable_for_cpu(buf->dev, sgt, buf->dma_dir);
}

static int mtk_imgsys_vb2_map_dmabuf(void *mem_priv)
{
	struct mtk_imgsys_vb2_buf *buf = mem_priv;
	struct sg_table *sgt;
	unsigned long contig_size;

	if (WARN_ON(!buf->db_attach)) {
		pr_err("trying to pin a non attached buffer\n");
		return -EINVAL;
	}

	if (WARN_ON(buf->dma_sgt)) {
		pr_err("dmabuf buffer is already pinned\n");
		return 0;
	}

	/* get the associated scatterlist for this buffer */
	#ifdef NEW_DMA_BUF_API
	sgt = dma_buf_map_attachment_unlocked(buf->db_attach, buf->dma_dir);
	#else
	sgt = dma_buf_map_attachment(buf->db_attach, buf->dma_dir);
	#endif
	if (IS_ERR(sgt)) {
		pr_err("Error getting dmabuf scatterlist\n");
		return -EINVAL;
	}

	/* checking if dmabuf is big enough to store contiguous chunk */
	contig_size = mtk_imgsys_vb2_get_contiguous_size(sgt);
	if (contig_size < buf->size) {
		pr_err("contiguous chunk is too small %lu/%lu\n",
		       contig_size, buf->size);
		#ifdef NEW_DMA_BUF_API
		dma_buf_unmap_attachment_unlocked(buf->db_attach, sgt, buf->dma_dir);
		#else
		dma_buf_unmap_attachment(buf->db_attach, sgt, buf->dma_dir);
		#endif
		return -EFAULT;
	}

	buf->dma_addr = sg_dma_address(sgt->sgl);
	buf->dma_sgt = sgt;
	buf->vaddr = NULL;

	return 0;
}

static void mtk_imgsys_vb2_unmap_dmabuf(void *mem_priv)
{
	struct mtk_imgsys_vb2_buf *buf = mem_priv;
	struct sg_table *sgt = buf->dma_sgt;
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(buf->vaddr);

	if (WARN_ON(!buf->db_attach)) {
		pr_err("trying to unpin a not attached buffer\n");
		return;
	}

	if (WARN_ON(!sgt)) {
		pr_err("dmabuf buffer is already unpinned\n");
		return;
	}

	if (buf->vaddr) {
		#ifdef NEW_DMA_BUF_API
		dma_buf_vunmap_unlocked(buf->db_attach->dmabuf, &map);
		#else
		dma_buf_vunmap(buf->db_attach->dmabuf, &map);
		#endif
		buf->vaddr = NULL;
	}
	#ifdef NEW_DMA_BUF_API
	dma_buf_unmap_attachment_unlocked(buf->db_attach, sgt, buf->dma_dir);
	#else
	dma_buf_unmap_attachment(buf->db_attach, sgt, buf->dma_dir);
	#endif

	buf->dma_addr = 0;
	buf->dma_sgt = NULL;
}

static void *mtk_imgsys_vb2_attach_dmabuf(struct vb2_buffer *vb, struct device *dev,
				  struct dma_buf *dbuf, unsigned long size)
{
	struct mtk_imgsys_vb2_buf *buf;
	struct dma_buf_attachment *dba;

	if (dbuf->size < size)
		return ERR_PTR(-EFAULT);

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dev = dev;
	buf->vb = vb;

	/* create attachment for the dmabuf with the user device */
	dba = dma_buf_attach(dbuf, buf->dev);
	if (IS_ERR(dba)) {
		pr_err("failed to attach dmabuf\n");
		kfree(buf);
		return dba;
	}

	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->size = size;
	buf->db_attach = dba;

	return buf;
}

static void mtk_imgsys_vb2_detach_dmabuf(void *mem_priv)
{
	struct mtk_imgsys_vb2_buf *buf = mem_priv;

	/* if vb2 works correctly you should never detach mapped buffer */
	if (WARN_ON(buf->dma_addr))
		mtk_imgsys_vb2_unmap_dmabuf(buf);

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

static unsigned int mtk_imgsys_vb2_num_users(void *buf_priv)
{
	struct mtk_imgsys_vb2_buf *buf = buf_priv;

	return refcount_read(&buf->refcount);
}

static void mtk_imgsys_return_all_buffers(struct mtk_imgsys_pipe *pipe,
				       struct mtk_imgsys_video_device *node,
				       enum vb2_buffer_state state)
{
	struct mtk_imgsys_dev_buffer *b, *b0;

	spin_lock(&node->buf_list_lock);
	list_for_each_entry_safe(b, b0, &node->buf_list, list) {
		list_del(&b->list);
		vb2_buffer_done(&b->vbb.vb2_buf, state);
	}
	spin_unlock(&node->buf_list_lock);
}

static int mtk_imgsys_vb2_start_streaming(struct vb2_queue *vq,
							unsigned int count)
{
	struct mtk_imgsys_pipe *pipe = vb2_get_drv_priv(vq);
	struct mtk_imgsys_video_device *node = mtk_imgsys_vbq_to_node(vq);
	int ret;


	if (!pipe->nodes_streaming) {
		ret = media_pipeline_start(&node->vdev.entity.pads[0], &pipe->pipeline);
		if (ret < 0) {
			dev_info(pipe->imgsys_dev->dev,
				"%s:%s: media_pipeline_start failed(%d)\n",
				pipe->desc->name, node->desc->name, ret);
			goto fail_return_bufs;
		}
	}

	mutex_lock(&pipe->lock);
	if (!(node->flags & MEDIA_LNK_FL_ENABLED)) {
		dev_info(pipe->imgsys_dev->dev,
			"%s:%s: stream on failed, node is not enabled\n",
			pipe->desc->name, node->desc->name);

		ret = -ENOLINK;
		goto fail_stop_pipeline;
	}

	pipe->nodes_streaming++;
	if (pipe->nodes_streaming == pipe->nodes_enabled) {
		/* Start streaming of the whole pipeline */
		ret = v4l2_subdev_call(&pipe->subdev, video, s_stream, 1);
		if (ret < 0) {
			dev_info(pipe->imgsys_dev->dev,
				"%s:%s: sub dev s_stream(1) failed(%d)\n",
				pipe->desc->name, node->desc->name, ret);

			goto fail_stop_pipeline;
		}
	}

    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
		"%s:%s:%s nodes_streaming(0x%llx), nodes_enable(0x%llx)\n",
		__func__, pipe->desc->name, node->desc->name,
		pipe->nodes_streaming, pipe->nodes_enabled);

	mutex_unlock(&pipe->lock);

	return 0;

fail_stop_pipeline:
	mutex_unlock(&pipe->lock);
	media_pipeline_stop(&node->vdev.entity.pads[0]);

fail_return_bufs:
	mtk_imgsys_return_all_buffers(pipe, node, VB2_BUF_STATE_QUEUED);


	return ret;
}

static void mtk_imgsys_vb2_stop_streaming(struct vb2_queue *vq)
{
	struct mtk_imgsys_pipe *pipe = vb2_get_drv_priv(vq);
	struct mtk_imgsys_video_device *node = mtk_imgsys_vbq_to_node(vq);
	int ret;

	mutex_lock(&pipe->lock);

	if (WARN_ON(!(node->flags & MEDIA_LNK_FL_ENABLED))) {
		mutex_lock(&pipe->imgsys_dev->hw_op_lock);
		pipe->imgsys_dev->abnormal_stop = true;
		mutex_unlock(&pipe->imgsys_dev->hw_op_lock);
	}

	/*
	 * We can check with pipe->nodes_streaming and pipe->nodes_enabled
	 * if the legacy user lib removed all dynamic link operations
	 */
	if (pipe->streaming) {
		ret = v4l2_subdev_call(&pipe->subdev, video, s_stream, 0);
		if (ret)
			dev_info(pipe->imgsys_dev->dev,
				"%s:%s: sub dev s_stream(0) failed(%d)\n",
				pipe->desc->name, node->desc->name, ret);
	}

	pipe->nodes_streaming--;

	mutex_unlock(&pipe->lock);

	if (!pipe->nodes_streaming)
		media_pipeline_stop(&node->vdev.entity.pads[0]);

	mtk_imgsys_return_all_buffers(pipe, node, VB2_BUF_STATE_ERROR);

    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
		"%s:%s:%s nodes_streaming(0x%llx), nodes_enable(0x%llx)\n",
		__func__, pipe->desc->name, node->desc->name,
		pipe->nodes_streaming, pipe->nodes_enabled);


}

static void mtk_imgsys_vb2_request_complete(struct vb2_buffer *vb)
{
	struct mtk_imgsys_video_device *node =
					mtk_imgsys_vbq_to_node(vb->vb2_queue);

	v4l2_ctrl_request_complete(vb->req_obj.req,
				   &node->ctrl_handler);
}

static int mtk_imgsys_videoc_querycap(struct file *file, void *fh,
				   struct v4l2_capability *cap)
{
	struct mtk_imgsys_pipe *pipe = video_drvdata(file);
	int j = 0;

	strscpy(cap->driver, pipe->desc->name,
		sizeof(cap->driver));
	strscpy(cap->card, pipe->desc->name,
		sizeof(cap->card));
	j = snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "platform:%s", dev_name(pipe->imgsys_dev->mdev.dev));
	if (j < 0)
		return -1;

	return 0;
}

static int mtk_imgsys_videoc_try_fmt(struct file *file, void *fh,
				  struct v4l2_format *f)
{
	struct mtk_imgsys_pipe *pipe = video_drvdata(file);
	struct mtk_imgsys_video_device *node = mtk_imgsys_file_to_node(file);
	const struct mtk_imgsys_dev_format *dev_fmt;
	struct v4l2_format try_fmt;

	memset(&try_fmt, 0, sizeof(try_fmt));

	dev_fmt = mtk_imgsys_pipe_find_fmt(pipe, node,
					f->fmt.pix_mp.pixelformat);
	if (!dev_fmt) {
		dev_fmt = &node->desc->fmts[node->desc->default_fmt_idx];
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
			"%s:%s:%s: dev_fmt(%d) not found, use default(%d)\n",
			__func__, pipe->desc->name, node->desc->name,
			f->fmt.pix_mp.pixelformat, dev_fmt->format);
	}

	mtk_imgsys_pipe_try_fmt(pipe, node, &try_fmt, f, dev_fmt);
	*f = try_fmt;

	return 0;
}

static int mtk_imgsys_videoc_g_fmt(struct file *file, void *fh,
				struct v4l2_format *f)
{
	struct mtk_imgsys_video_device *node = mtk_imgsys_file_to_node(file);

	*f = node->vdev_fmt;

	return 0;
}

static int mtk_imgsys_videoc_s_fmt(struct file *file, void *fh,
				struct v4l2_format *f)
{
	struct mtk_imgsys_video_device *node = mtk_imgsys_file_to_node(file);
	struct mtk_imgsys_pipe *pipe = video_drvdata(file);
	const struct mtk_imgsys_dev_format *dev_fmt;

	if (pipe->streaming || vb2_is_busy(&node->dev_q.vbq))
		return -EBUSY;

	dev_fmt = mtk_imgsys_pipe_find_fmt(pipe, node,
					f->fmt.pix_mp.pixelformat);
	if (!dev_fmt) {
		dev_fmt = &node->desc->fmts[node->desc->default_fmt_idx];
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
			"%s:%s:%s: dev_fmt(%d) not found, use default(%d)\n",
			__func__, pipe->desc->name, node->desc->name,
			f->fmt.pix_mp.pixelformat, dev_fmt->format);
	}

	memset(&node->vdev_fmt, 0, sizeof(node->vdev_fmt));

	mtk_imgsys_pipe_try_fmt(pipe, node, &node->vdev_fmt, f, dev_fmt);
	*f = node->vdev_fmt;

	node->dev_q.dev_fmt = dev_fmt;
	node->vdev_fmt = *f;
	node->crop.left = 0; /* reset crop setting of nodes */
	node->crop.top = 0;
	node->crop.width = f->fmt.pix_mp.width;
	node->crop.height = f->fmt.pix_mp.height;
	node->compose.left = 0;
	node->compose.top = 0;
	node->compose.width = f->fmt.pix_mp.width;
	node->compose.height = f->fmt.pix_mp.height;

	return 0;
}

static int mtk_imgsys_videoc_enum_framesizes(struct file *file, void *priv,
					  struct v4l2_frmsizeenum *sizes)
{
	struct mtk_imgsys_pipe *pipe = video_drvdata(file);
	struct mtk_imgsys_video_device *node = mtk_imgsys_file_to_node(file);
	const struct mtk_imgsys_dev_format *dev_fmt;

	dev_fmt = mtk_imgsys_pipe_find_fmt(pipe, node, sizes->pixel_format);

	if (!dev_fmt || sizes->index)
		return -EINVAL;

	sizes->type = node->desc->frmsizeenum->type;
	sizes->stepwise.max_width =
		node->desc->frmsizeenum->stepwise.max_width;
	sizes->stepwise.min_width =
		node->desc->frmsizeenum->stepwise.min_width;
	sizes->stepwise.max_height =
		node->desc->frmsizeenum->stepwise.max_height;
	sizes->stepwise.min_height =
		node->desc->frmsizeenum->stepwise.min_height;
	sizes->stepwise.step_height =
		node->desc->frmsizeenum->stepwise.step_height;
	sizes->stepwise.step_width =
		node->desc->frmsizeenum->stepwise.step_width;

	return 0;
}

static int mtk_imgsys_videoc_enum_fmt(struct file *file, void *fh,
				   struct v4l2_fmtdesc *f)
{
	struct mtk_imgsys_video_device *node = mtk_imgsys_file_to_node(file);

	if (f->index >= node->desc->num_fmts)
		return -EINVAL;

	strscpy(f->description, node->desc->description,
		sizeof(f->description));
	f->pixelformat = node->desc->fmts[f->index].format;
	f->flags = 0;

	return 0;
}

static int mtk_imgsys_meta_enum_format(struct file *file, void *fh,
				    struct v4l2_fmtdesc *f)
{
	struct mtk_imgsys_video_device *node = mtk_imgsys_file_to_node(file);

	if (f->index > 0)
		return -EINVAL;

	strscpy(f->description, node->desc->description,
		sizeof(f->description));

	f->pixelformat = node->vdev_fmt.fmt.meta.dataformat;
	f->flags = 0;

	return 0;
}

static int mtk_imgsys_videoc_g_meta_fmt(struct file *file, void *fh,
				     struct v4l2_format *f)
{
	struct mtk_imgsys_video_device *node = mtk_imgsys_file_to_node(file);
	*f = node->vdev_fmt;

	return 0;
}

static int mtk_imgsys_videoc_s_meta_fmt(struct file *file, void *fh,
				     struct v4l2_format *f)
{
	struct mtk_imgsys_video_device *node = mtk_imgsys_file_to_node(file);
	struct mtk_imgsys_pipe *pipe = video_drvdata(file);
	const struct mtk_imgsys_dev_format *dev_fmt;

	if (pipe->streaming || vb2_is_busy(&node->dev_q.vbq))
		return -EBUSY;

	dev_fmt = mtk_imgsys_pipe_find_fmt(pipe, node,
						f->fmt.meta.dataformat);

		if (!dev_fmt) {
			dev_fmt =
				&node->desc->fmts[node->desc->default_fmt_idx];
			dev_info(pipe->imgsys_dev->dev,
				"%s:%s:%s: dev_fmt(%d) not found, use default(%d)\n",
				__func__, pipe->desc->name, node->desc->name,
				f->fmt.meta.dataformat, dev_fmt->format);
		}

	memset(&node->vdev_fmt, 0, sizeof(node->vdev_fmt));

	f->fmt.meta.dataformat = dev_fmt->format;

	if (dev_fmt->buffer_size <= 0) {
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
			"%s: Invalid meta buf size(%u), use default(%u)\n",
			pipe->desc->name, dev_fmt->buffer_size,
			MTK_DIP_DEV_META_BUF_DEFAULT_SIZE);

		f->fmt.meta.buffersize =
			MTK_DIP_DEV_META_BUF_DEFAULT_SIZE;
	} else {
		f->fmt.meta.buffersize = dev_fmt->buffer_size;
	}

	node->dev_q.dev_fmt = dev_fmt;
	node->vdev_fmt = *f;

	return 0;
}


static int mtk_imgsys_video_device_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_imgsys_video_device *node =
		container_of(ctrl->handler,
			     struct mtk_imgsys_video_device, ctrl_handler);

	if (ctrl->id != V4L2_CID_ROTATE) {
        if (imgsys_dbg_enable()) {
		pr_debug("[%s] doesn't support ctrl(%d)\n",
			 node->desc->name, ctrl->id);
        }
		return -EINVAL;
	}

	node->rotation = ctrl->val;

	return 0;
}

static int mtk_imgsys_vidioc_qbuf(struct file *file, void *priv,
				  struct v4l2_buffer *buf)
{
	struct mtk_imgsys_pipe *pipe = video_drvdata(file);
	struct mtk_imgsys_video_device *node = mtk_imgsys_file_to_node(file);
	struct vb2_buffer *vb;
	struct mtk_imgsys_dev_buffer *dev_buf;
	struct buf_info dyn_buf_info;
	int ret = 0, i = 0;
	unsigned long user_ptr = 0;
#ifdef REQ_TIMESTAMP
	struct mtk_imgsys_request *imgsys_req;
	struct media_request *req;
#endif
#ifndef USE_V4L2_FMT
	struct v4l2_plane_pix_format *vfmt;
	struct plane_pix_format *bfmt;
#endif
	if (!pipe->streaming) {
		dev_info(pipe->imgsys_dev->dev, "[%s] illegal process access \n", __func__);
		return -EINVAL;
	}
	if (buf->index >= VB2_MAX_FRAME) {
		dev_info(pipe->imgsys_dev->dev, "[%s] error vb2 index %d\n", __func__, buf->index);
		return -EINVAL;
	}

	vb = node->dev_q.vbq.bufs[buf->index];
	dev_buf = mtk_imgsys_vb2_buf_to_dev_buf(vb);
	if (!dev_buf) {
		dev_dbg(pipe->imgsys_dev->dev, "[%s] NULL dev_buf obtained with idx %d\n", __func__,
											buf->index);
		return -EINVAL;
	}

	//support dynamic change size&fmt for std mode flow
#ifdef REQ_TIMESTAMP
	req = media_request_get_by_fd(&pipe->imgsys_dev->mdev, buf->request_fd);
	imgsys_req = mtk_imgsys_media_req_to_imgsys_req(req);
	imgsys_req->tstate.time_qbuf = ktime_get_boottime_ns()/1000;
	media_request_put(req);
#endif
	if (!is_desc_fmt(node->dev_q.dev_fmt)) {
		user_ptr =
			(((unsigned long)(buf->m.planes[0].reserved[0]) << 32) |
			((unsigned long)buf->m.planes[0].reserved[1]));

		if (user_ptr) {
			ret = copy_from_user(&dyn_buf_info,
						   (void *)(size_t)user_ptr,
						   sizeof(struct buf_info));
			if (ret != 0) {
				dev_dbg(pipe->imgsys_dev->dev,
					"[%s]%s:%s:copy_from_user fail !!!\n",
					__func__,
					pipe->desc->name, node->desc->name);
				return -EINVAL;
			}

#ifdef USE_V4L2_FMT
			dev_buf->fmt.fmt.pix_mp = dyn_buf_info.fmt.fmt.pix_mp;
#else
			dev_buf->fmt.fmt.pix_mp.width =
					dyn_buf_info.fmt.fmt.pix_mp.width;
			dev_buf->fmt.fmt.pix_mp.height =
					dyn_buf_info.fmt.fmt.pix_mp.height;
			dev_buf->fmt.fmt.pix_mp.pixelformat =
					dyn_buf_info.fmt.fmt.pix_mp.pixelformat;
			for (i = 0; i < dyn_buf_info.buf.num_planes; i++) {
				vfmt = &dev_buf->fmt.fmt.pix_mp.plane_fmt[i];
				bfmt =
				&dyn_buf_info.fmt.fmt.pix_mp.plane_fmt[i];
				vfmt->sizeimage = bfmt->sizeimage;
				vfmt->bytesperline = bfmt->sizeimage;
			}
#endif

			dev_buf->crop = dyn_buf_info.crop;
			/* dev_buf->compose = dyn_buf_info.compose; */
			dev_buf->rotation = dyn_buf_info.rotation;
			dev_buf->hflip = dyn_buf_info.hflip;
			dev_buf->vflip = dyn_buf_info.vflip;

            if (imgsys_dbg_enable()) {
				dev_dbg(pipe->imgsys_dev->dev,
						"[%s] portid(%d), rotat(%d), hflip(%d), vflip(%d)\n",
						__func__,
						dev_buf->dma_port,
						dev_buf->rotation,
						dev_buf->hflip,
						dev_buf->vflip);
			}

			for (i = 0;
				i < dev_buf->fmt.fmt.pix_mp.num_planes; i++) {
                if (imgsys_dbg_enable()) {
				dev_dbg(pipe->imgsys_dev->dev,
					"[%s] width(%d), width(%d), sizeimage(%d), bytesperline(%d)\n",
					__func__,
					dev_buf->fmt.fmt.pix_mp.width,
					dev_buf->fmt.fmt.pix_mp.width,
					dev_buf->fmt.fmt.pix_mp.plane_fmt[i].sizeimage,
					dev_buf->fmt.fmt.pix_mp.plane_fmt[i].bytesperline);
				}

			}
		} else {
		    if (imgsys_dbg_enable()) {
				dev_dbg(pipe->imgsys_dev->dev,
					"[%s]%s: stdmode videonode(%s) qbuf bufinfo(reserved) is null!\n",
					__func__, pipe->desc->name, node->desc->name);
			}
			/*
			 * update fiexd pipeline node info for update devbuf
			 * lately
			 */
			{
				dev_buf->fmt = node->vdev_fmt;
				dev_buf->dma_port = node->desc->dma_port;
				dev_buf->rotation = node->rotation;
				dev_buf->crop.c = node->crop;
				dev_buf->compose = node->compose;
			}

		}
	} else {
        if (imgsys_dbg_enable()) {
        dev_dbg(pipe->imgsys_dev->dev,
			"[%s]%s:%s: no need to cache bufinfo,videonode fmt is DESC or SingleDevice(%d)!\n",
			__func__,
			pipe->desc->name,
			node->desc->name,
			node->dev_q.dev_fmt->format);
		}
	}

	if ((node->desc->id == MTK_IMGSYS_VIDEO_NODE_ID_CTRLMETA_OUT) ||
		(node->desc->id == MTK_IMGSYS_VIDEO_NODE_ID_TUNING_OUT)) {
		dev_buf->dataofst = buf->reserved2;
	} else {
		dev_buf->dataofst = buf->m.planes[0].reserved[2];
	}

	ret =  vb2_ioctl_qbuf(file, priv, buf);
	if (ret != 0) {
		pr_info("[%s] %s:%s: vb2_ioctl_qbuf fail\n",
			__func__,
			pipe->desc->name, node->desc->name);
		return -EINVAL;
	}

	return ret;
}

static bool get_user_by_file(struct file *filp, struct mtk_imgsys_user **user)
{
	bool found = false;
	struct mtk_imgsys_pipe *pipe = video_drvdata(filp);
	struct mtk_imgsys_dev *imgsys_dev = pipe->imgsys_dev;

	mutex_lock(&imgsys_dev->imgsys_users.user_lock);
	list_for_each_entry((*user), &imgsys_dev->imgsys_users.list, entry) {
		if ((*user)->fh == filp->private_data) {
			found = true;
            if (imgsys_dbg_enable()) {
			pr_debug("%s: user(%lx) found! id(%d)", __func__, (unsigned long)(*user),
				(*user)->id);
            }
			break;
		}
	}
	mutex_unlock(&imgsys_dev->imgsys_users.user_lock);
	return found;
}

static int mtk_imgsys_subdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct mtk_imgsys_dma_buf_iova_get_info *iova_info, *tmp;
	struct mtk_imgsys_pipe *pipe = sd->dev_priv;

	list_for_each_entry_safe(iova_info, tmp,
		&pipe->iova_cache.list, list_entry) {
		mtk_imgsys_put_dma_buf(iova_info->dma_buf,
		iova_info->attach,
		iova_info->sgt);
		spin_lock(&pipe->iova_cache.lock);
		list_del(&iova_info->list_entry);
		hash_del(&iova_info->hnode);
		spin_unlock(&pipe->iova_cache.lock);
		vfree(iova_info);
	}
	pr_info("imgsys_fw:%s", __func__);

	return 0;
}

#ifdef BATCH_MODE_V3
static int mtkdip_ioc_dqbuf(struct file *filp, void *arg)
{
	struct mtk_imgsys_user *user = NULL;
	int ret = 0;

	if (!get_user_by_file(filp, &user))
		return -EINVAL;

	if (list_empty(&user->done_list)
	    && user->state == DIP_STATE_STREAMON) {
		pr_info("%s: list empty, wait", __func__);

		ret = wait_event_interruptible(user->done_wq,
						!list_empty(&user->done_list)
					|| user->state != DIP_STATE_STREAMON);
	}

	if (ret == 0) {
		struct frame_param_pack *pack = (struct frame_param_pack *)arg;
		struct done_frame_pack *dp;
		unsigned long flags;

		spin_lock_irqsave(&user->lock, flags);
		if (user->state == DIP_STATE_STREAMON &&
			(!list_empty(&user->done_list) && user->dqdonestate)) {
			dp = list_first_entry(&user->done_list,
				struct done_frame_pack, done_entry);
			list_del(&dp->done_entry);
			memcpy(pack, &dp->pack,
			       sizeof(struct frame_param_pack));
			vfree(dp);
		}
		spin_unlock_irqrestore(&user->lock, flags);

		pr_info(
		"[dip-drv][j.deque] user(%d) deque success! frame_pack=(num_frames(%d), seq_no(%llx), cookie(%x))\n",
		user->id, pack->num_frames, pack->seq_num, pack->cookie);
	}

	return ret;
}

static void mtkdip_save_pack(struct frame_param_pack *pack,
			    struct mtk_imgsys_request *imgsys_req)
{
	imgsys_req->done_pack = vzalloc(sizeof(struct done_frame_pack),
				     GFP_KERNEL);
	memcpy(&imgsys_req->done_pack->pack, pack,
	       sizeof(struct frame_param_pack));


}

static int mtkdip_fill_req(struct mtk_imgsys_dev *imgsys_dev,
			      struct mtk_imgsys_request *req,
			      struct frame_param_pack *pack,
			      struct mtk_imgsys_user *user)
{
	int i, ret;
	struct img_ipi_frameparam *packed_frame = pack->frame_params;

	req->unprocessed_count = pack->num_frames;
	for (i = 0; i < req->unprocessed_count; i++) {
		struct mtk_imgsys_hw_subframe *subframe;
		struct img_ipi_frameparam *frame, *prev_frame;

		/**
		 * Note: subframe->buffer is tile buffer,
		 *       subframe->frameparam is img_ipi_frameparam
		 *       we set offset in scp_daddr for user daemon access
		 */

		subframe = imgsys_working_buf_alloc_helper(imgsys_dev);
		if (subframe == NULL) {
			pr_info("%s: Failed to allocate subframe(%p), free(%d),used(%d)\n",
				__func__,
				subframe,
				imgsys_dev->imgsys_freebufferlist.cnt,
				imgsys_dev->imgsys_usedbufferlist.cnt);
			ret = wait_event_interruptible(user->enque_wq,
				imgsys_working_buf_alloc_helper(imgsys_dev));
			return ret;
		}

		frame = (struct img_ipi_frameparam *)subframe->frameparam.vaddr;

		if (i == 0) {
			struct img_sw_addr *ipi_addr;

			ipi_addr = &req->img_fparam.frameparam.self_data;
			ipi_addr->va = (u64)subframe->frameparam.vaddr;
			ipi_addr->pa = (u32)subframe->frameparam.scp_daddr
					- imgsys_dev->working_buf_mem_scp_daddr;
		}

		copy_from_user((void *)subframe->frameparam.vaddr,
				       (void *)packed_frame,
				       sizeof(struct img_ipi_frameparam));

		spin_lock(&imgsys_dev->imgsys_usedbufferlist.lock);
		list_del(&subframe->list_entry);
		imgsys_dev->imgsys_usedbufferlist.cnt--;
		spin_unlock(&imgsys_dev->imgsys_usedbufferlist.lock);

		spin_lock(&req->working_buf_list.lock);
		list_add_tail(&subframe->list_entry,
			      &req->working_buf_list.list);
		req->working_buf_list.cnt++;
		spin_unlock(&req->working_buf_list.lock);

		frame->framepack_buf_va = (u64)subframe->frameparam.vaddr;

		/* Use our own tuning buffer */
		frame->tuning_data.iova = subframe->tuning_buf.isp_daddr;
		frame->tuning_data.present = ((struct img_ipi_frameparam *)
				frame->framepack_buf_va)->tuning_data.present;
		frame->tuning_data.pa = subframe->tuning_buf.scp_daddr
					- imgsys_dev->working_buf_mem_scp_daddr;
		frame->tuning_data.va = (u64)subframe->tuning_buf.vaddr
			- (u64)mtk_hcp_get_reserve_mem_virt(DIP_MEM_FOR_HW_ID);
		memset(subframe->tuning_buf.vaddr, 0, DIP_TUNING_SZ);
        if (imgsys_dbg_enable())
		dev_dbg(imgsys_dev->dev,
			"packed_frame->tuning_data.present:0x%llx, idx(%d), f_no(%d), in(%d), out(%d)\n",
			frame->tuning_data.present,
			frame->index, frame->frame_no,
			frame->num_inputs, frame->num_outputs);

		if (frame->tuning_data.present)
			copy_from_user(subframe->tuning_buf.vaddr,
			       (void *)frame->tuning_data.present,
			       DIP_TUNING_SZ);

		memset(subframe->buffer.vaddr, 0, DIP_SUB_FRM_SZ);

		frame->config_data.va = (u64)subframe->config_data.vaddr;
		frame->config_data.pa = subframe->config_data.scp_daddr
					- imgsys_dev->working_buf_mem_scp_daddr;
		frame->config_data.offset = (u64)subframe->config_data.vaddr
			- (u64)mtk_hcp_get_reserve_mem_virt(DIP_MEM_FOR_HW_ID);
		memset(subframe->config_data.vaddr, 0, DIP_COMP_SZ);

		/* fill the current buffer address in the previous frame */
		if (i > 0 && i < (req->unprocessed_count)) {

			/**
			 * we send offset of the frameparam to user space
			 */
			prev_frame->dip_param.next_frame =
					subframe->frameparam.scp_daddr
					- imgsys_dev->working_buf_mem_scp_daddr;
            if (imgsys_dbg_enable())
			dev_dbg(imgsys_dev->dev, "next_frame offset:%x\n",
				 prev_frame->dip_param.next_frame);
		}

		prev_frame = frame;

		if (i == (req->unprocessed_count - 1))
			prev_frame->dip_param.next_frame = 0;

		/* Get next frame in the frame_param_pack */
		packed_frame++;
	}
	return 0;
}

static void init_buffer_list(struct mtk_imgsys_hw_working_buf_list *list)
{
	INIT_LIST_HEAD(&list->list);
	spin_lock_init(&list->lock);
	list->cnt = 0;
}

static int mtkdip_ioc_qbuf(struct file *filp, void *arg)
{
	struct mtk_imgsys_user *user = NULL;
	struct mtk_imgsys_pipe *pipe = video_drvdata(filp);
	struct mtk_imgsys_dev *imgsys_dev = pipe->imgsys_dev;
	struct mtk_imgsys_request *imgsys_req;
	struct frame_param_pack *pack = (struct frame_param_pack *)arg;
	int ret = 0;
	unsigned long flag;

	ret = get_user_by_file(filp, &user);
	if (!ret) {
		pr_info("%s: cannot find user\n", __func__);
		return -EINVAL;
	}

	if (user->state != DIP_STATE_STREAMON) {
		pr_info("%s: cannot queue buffer before streamon\n", __func__);
		return -EINVAL;
	}

	// generate mtk dip request, with pipe and id
	imgsys_req = vzalloc(sizeof(*imgsys_req), GFP_KERNEL);
	if (!imgsys_req) {
		ret = -ENOMEM;
		pr_info("%s: Failed to allocate dip request\n", __func__);
		return ret;
	}
	imgsys_req->imgsys_pipe = pipe;
	imgsys_req->id = mtk_imgsys_pipe_next_job_id_batch_mode(pipe, user->id);
	init_buffer_list(&imgsys_req->working_buf_list);
	init_buffer_list(&imgsys_req->scp_done_list);
	init_buffer_list(&imgsys_req->runner_done_list);
	init_buffer_list(&imgsys_req->mdp_done_list);

	if (!mtk_imgsys_can_enqueue(imgsys_dev, pack->num_frames)) {
		ret = wait_event_interruptible(user->enque_wq,
			mtk_imgsys_can_enqueue(imgsys_dev, pack->num_frames));
		if (ret == -ERESTARTSYS) {
			pr_info("%s: interrupted by a signal!\n", __func__);
			return ret;
		}
	}

	// put frame parameter in mtk dip request
	ret = mtkdip_fill_req(imgsys_dev, imgsys_req, pack, user);
	if (ret) {
		pr_info("%s: Failed to fill dip request\n", __func__);
		return ret;
	}
	mtkdip_save_pack(pack, imgsys_req);
	imgsys_req->is_batch_mode = 1;
	// add job to pipe->pipe_job_running_list
	spin_lock_irqsave(&pipe->running_job_lock, flag);
	list_add_tail(&imgsys_req->list, &pipe->pipe_job_running_list);
	pipe->num_jobs++;
	spin_unlock_irqrestore(&pipe->running_job_lock, flag);

	mtk_imgsys_hw_enqueue(imgsys_dev, imgsys_req);
	return 0;
}

static int mtkdip_ioc_streamon(struct file *filp)
{
	struct mtk_imgsys_user *user = NULL;
	struct mtk_imgsys_pipe *pipe = NULL;
	int ret = 0;

	ret = get_user_by_file(filp, &user);
	if (!ret) {
		pr_info("%s: cannot find user\n", __func__);
		return -EINVAL;
	}
	pr_info("%s: id(%d)\n", __func__, user->id);
	pipe = video_drvdata(filp);
	ret = mtk_imgsys_hw_streamon(pipe);
	if (ret == 0) {
		unsigned long flags;

		spin_lock_irqsave(&user->lock, flags);
		user->state = DIP_STATE_STREAMON;
		user->dqdonestate = false;
		spin_unlock_irqrestore(&user->lock, flags);
	}
	return ret;
}

static int mtkdip_ioc_streamoff(struct file *filp)
{
	struct mtk_imgsys_user *user = NULL;
	struct mtk_imgsys_pipe *pipe = NULL;
	int ret = 0;

	ret = get_user_by_file(filp, &user);
	if (!ret) {
		pr_info("%s: cannot find user\n", __func__);
		return -EINVAL;
	}
	pr_info("%s: id(%d)\n", __func__, user->id);
	pipe = video_drvdata(filp);

	ret = mtk_imgsys_hw_streamoff(pipe);
	if (ret == 0) {
		struct done_frame_pack *dp;
		unsigned long flags;

		spin_lock_irqsave(&user->lock, flags);
		list_for_each_entry(dp, &user->done_list, done_entry) {
			list_del(&dp->done_entry);
			vfree(dp);
		}
		user->state = DIP_STATE_STREAMOFF;
		user->dqdonestate = false;
		spin_unlock_irqrestore(&user->lock, flags);
		wake_up_all(&user->done_wq);
	}
#if SMVR_DECOUPLE
smvr_flag = 0;
    capture_flag = 0;
    imgsys_user_count = 0;
#endif
	return ret;
}

static int mtkdip_ioc_g_reg_size(struct file *filp, void *arg)
{
	struct mtk_imgsys_user *user = NULL;
	struct mtk_imgsys_pipe *pipe = NULL;
	struct mtk_imgsys_dev *imgsys_dev = NULL;
	int ret = 0;
	int *reg_size = (int *)arg;

	ret = get_user_by_file(filp, &user);
	if (!ret) {
		pr_info("%s: cannot find user\n", __func__);
		return -EINVAL;
	}
	pr_info("%s: id(%d)\n", __func__, user->id);
	pipe = video_drvdata(filp);
	imgsys_dev = pipe->imgsys_dev;
	*reg_size = imgsys_dev->reg_table_size;

	return 0;
}

static int mtkdip_ioc_g_isp_version(struct file *filp, void *arg)
{
	struct mtk_imgsys_user *user = NULL;
	struct mtk_imgsys_pipe *pipe = NULL;
	struct mtk_imgsys_dev *imgsys_dev = NULL;
	int ret = 0;
	int *isp_version = (int *)arg;

	ret = get_user_by_file(filp, &user);
	if (!ret) {
		pr_info("%s: cannot find user\n", __func__);
		return -EINVAL;
	}
	pr_info("%s: id(%d)\n", __func__, user->id);
	pipe = video_drvdata(filp);
	imgsys_dev = pipe->imgsys_dev;
	*isp_version = imgsys_dev->isp_version;
	return 0;
}

static int mtkdip_ioc_set_user_enum(struct file *filp, void *arg)
{
	int ret = 0;
	struct mtk_imgsys_user *user;

	ret = get_user_by_file(filp, &user);
	if (!ret) {
		pr_info("%s: cannot find user\n", __func__);
		return -EINVAL;
	}

	user->user_enum = *(int *)arg;
	pr_info("%s: id(%d) user_enum(%d)\n", __func__,
		user->id, user->user_enum);
	return 0;

}

long mtk_imgsys_vidioc_default(struct file *file, void *fh,
			   bool valid_prio, unsigned int cmd, void *arg)
{
	struct v4l2_fh *handle = fh;

    if (imgsys_dbg_enable()) {
	pr_debug("%s cmd: %d handle: %p\n", __func__, cmd, handle);
    }

	switch (cmd) {
	case MTKDIP_IOC_QBUF:
		return mtkdip_ioc_qbuf(file, arg);
	case MTKDIP_IOC_DQBUF:
		return mtkdip_ioc_dqbuf(file, arg);
	case MTKDIP_IOC_STREAMON:
		return mtkdip_ioc_streamon(file);
	case MTKDIP_IOC_STREAMOFF:
		return mtkdip_ioc_streamoff(file);
	case MTKDIP_IOC_G_REG_SIZE:
		return mtkdip_ioc_g_reg_size(file, arg);
	case MTKDIP_IOC_G_ISP_VERSION:
		return mtkdip_ioc_g_isp_version(file, arg);
	case MTKDIP_IOC_S_USER_ENUM:
		return mtkdip_ioc_set_user_enum(file, arg);
	default:
		pr_info("%s: non-supported cmd\n", __func__);
		return -ENOTTY;
	}

	return 0;
}
#endif

static int mtkdip_ioc_add_kva(struct v4l2_subdev *subdev, void *arg)
{
	struct mtk_imgsys_pipe *imgsys_pipe = mtk_imgsys_subdev_to_pipe(subdev);
	struct fd_info *fd_info = (struct fd_info *)arg;
	struct fd_kva_list_t *kva_list;
	struct buf_va_info_t *buf_va_info;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	struct iosys_map map;
	int ret = 0;
	int i;

	if (fd_info->fd_num > FD_MAX)
		return -EINVAL;

	kva_list = get_fd_kva_list();
	for (i = 0; i < fd_info->fd_num; i++) {
		buf_va_info = (struct buf_va_info_t *)
			vzalloc(sizeof(vlist_type(struct buf_va_info_t)));
		if (buf_va_info == NULL) {
			dev_info(imgsys_pipe->imgsys_dev->dev, "%s: null buf_va_info\n",
								__func__);
			return -EINVAL;
		}
		INIT_LIST_HEAD(vlist_link(buf_va_info, struct buf_va_info_t));

		if (!fd_info->fds[i]) {
			dev_info(imgsys_pipe->imgsys_dev->dev, "%s: zero fd bypassed\n",
								__func__);
			continue;
		}

		buf_va_info->buf_fd = fd_info->fds[i];
		dmabuf = dma_buf_get(fd_info->fds[i]);

		if (IS_ERR(dmabuf)) {
			dev_info(imgsys_pipe->imgsys_dev->dev, "%s:err fd %d",
						__func__, fd_info->fds[i]);
			vfree(buf_va_info);
			continue;
		}

		fd_info->fds_size[i] = dmabuf->size;

		dma_buf_begin_cpu_access(dmabuf, DMA_BIDIRECTIONAL);
		#ifdef NEW_DMA_BUF_API
		ret = dma_buf_vmap_unlocked(dmabuf, &map);
		#else
		ret = dma_buf_vmap(dmabuf, &map);
		#endif
		if (ret)
			pr_info("%s, map kernel va failed\n", __func__);
		buf_va_info->kva = (u64)map.vaddr;
		buf_va_info->map = map;
		buf_va_info->dma_buf_putkva = dmabuf;

		attach = dma_buf_attach(dmabuf, imgsys_pipe->imgsys_dev->smmu_dev);
		if (IS_ERR(attach)) {
			#ifdef NEW_DMA_BUF_API
			dma_buf_vunmap_unlocked(dmabuf, &buf_va_info->map);
			#else
			dma_buf_vunmap(dmabuf, &buf_va_info->map);
			#endif
			dma_buf_end_cpu_access(dmabuf, DMA_BIDIRECTIONAL);
			dma_buf_put(dmabuf);
			vfree(buf_va_info);
			pr_info("dma_buf_attach fail fd:%d\n", fd_info->fds[i]);
			continue;
		}

		#ifdef NEW_DMA_BUF_API
		sgt = dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
		#else
		sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
		#endif
		if (IS_ERR(sgt)) {
			#ifdef NEW_DMA_BUF_API
			dma_buf_vunmap_unlocked(dmabuf, &buf_va_info->map);
			#else
			dma_buf_vunmap(dmabuf, &buf_va_info->map);
			#endif
			dma_buf_end_cpu_access(dmabuf, DMA_BIDIRECTIONAL);
			dma_buf_detach(dmabuf, attach);
			dma_buf_put(dmabuf);
			pr_info("%s:dma_buf_map_attachment sgt err: fd %d\n",
						__func__, fd_info->fds[i]);
			vfree(buf_va_info);

			continue;
		}
		dma_addr = sg_dma_address(sgt->sgl);

		buf_va_info->attach = attach;
		buf_va_info->sgt = sgt;
		buf_va_info->dma_addr = dma_addr;

		mutex_lock(&(kva_list->mymutex));
		list_add_tail(vlist_link(buf_va_info, struct buf_va_info_t),
		   &kva_list->mylist);
		mutex_unlock(&(kva_list->mymutex));
        if (imgsys_dbg_enable()) {
		pr_debug("%s: fd(%d) size(%lx) cached\n", __func__,
					fd_info->fds[i], fd_info->fds_size[i]);
	}
	}

	mtk_hcp_send_async(imgsys_pipe->imgsys_dev->scp_pdev,
				HCP_IMGSYS_UVA_FDS_ADD_ID, fd_info,
				sizeof(struct fd_info), 0);

	return 0;
}

static int mtkdip_ioc_del_kva(struct v4l2_subdev *subdev, void *arg)
{
	struct mtk_imgsys_pipe *imgsys_pipe = mtk_imgsys_subdev_to_pipe(subdev);
	struct fd_info *fd_info = (struct fd_info *)arg;
	struct fd_kva_list_t *kva_list;
	struct buf_va_info_t *buf_va_info;
	struct dma_buf *dmabuf;
	bool find = false;
	struct list_head *ptr = NULL;
	int i;

	if (fd_info->fd_num > FD_MAX)
		return -EINVAL;

	kva_list = get_fd_kva_list();
	for (i = 0; i < fd_info->fd_num; i++) {
		find = false;
		mutex_lock(&(kva_list->mymutex));
		list_for_each(ptr, &(kva_list->mylist)) {
			buf_va_info = vlist_node_of(ptr, struct buf_va_info_t);
			if (buf_va_info->buf_fd == fd_info->fds[i]) {
				find = true;
				break;
			}
		}

		if (!find) {
			mutex_unlock(&(kva_list->mymutex));
			dev_info(imgsys_pipe->imgsys_dev->dev, "%s: fd(%d) not found\n",
						__func__, fd_info->fds[i]);
			continue;
		}
		list_del_init(vlist_link(buf_va_info, struct buf_va_info_t));
		mutex_unlock(&(kva_list->mymutex));

		dmabuf = buf_va_info->dma_buf_putkva;
		#ifdef NEW_DMA_BUF_API
		dma_buf_vunmap_unlocked(dmabuf, &buf_va_info->map);

		dma_buf_end_cpu_access(dmabuf, DMA_BIDIRECTIONAL);

		dma_buf_unmap_attachment_unlocked(buf_va_info->attach, buf_va_info->sgt,
			DMA_BIDIRECTIONAL);
		#else
		dma_buf_vunmap(dmabuf, &buf_va_info->map);

		dma_buf_end_cpu_access(dmabuf, DMA_BIDIRECTIONAL);

		dma_buf_unmap_attachment(buf_va_info->attach, buf_va_info->sgt,
			DMA_BIDIRECTIONAL);
		#endif
		dma_buf_detach(dmabuf, buf_va_info->attach);
		fd_info->fds_size[i] = dmabuf->size;
		dma_buf_put(dmabuf);
		vfree(buf_va_info);
		buf_va_info = NULL;
        if (imgsys_dbg_enable()) {
		pr_debug("%s: fd(%d) size (%lx) cache invalidated\n", __func__,
					fd_info->fds[i], fd_info->fds_size[i]);
	}
	}

	mtk_hcp_send_async(imgsys_pipe->imgsys_dev->scp_pdev,
				HCP_IMGSYS_UVA_FDS_DEL_ID, fd_info,
				sizeof(struct fd_info), 0);

	return 0;
}


static int mtkdip_ioc_add_iova(struct v4l2_subdev *subdev, void *arg)
{
	struct mtk_imgsys_pipe *pipe = mtk_imgsys_subdev_to_pipe(subdev);
	struct mtk_imgsys_dma_buf_iova_get_info *fd_iova;
	struct fd_tbl *fd_tbl = (struct fd_tbl *)arg;
	struct fd_info fd_info;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	unsigned int *kfd;
	size_t size;
	int i, ret;

	if (!fd_tbl->fds) {
		dev_dbg(pipe->imgsys_dev->dev, "%s:NULL usrptr\n", __func__);
		return -EINVAL;
	}

	if (fd_tbl->fd_num > FD_MAX)
		return -EINVAL;

	size = sizeof(*kfd) * fd_tbl->fd_num;
	kfd = vzalloc(size);
	if (kfd == NULL)
		return -ENOMEM;
	ret = copy_from_user(kfd, (void *)fd_tbl->fds, size);
	if (ret != 0) {
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
			"[%s]%s:copy_from_user fail !!!\n",
			__func__,
			pipe->desc->name);
		vfree(kfd);
		return -EINVAL;
	}

	fd_info.fd_num = fd_tbl->fd_num;

	for (i = 0; i < fd_tbl->fd_num; i++) {

		if (!kfd[i]) {
			fd_info.fds[i] = 0;
			fd_info.fds_size[i] = 0;
			continue;
		}

		dmabuf = dma_buf_get(kfd[i]);
		if (IS_ERR(dmabuf))
			return -ENOMEM;
#ifdef IMG_MEM_G_ID_DEBUG
		spin_lock(&dmabuf->name_lock);
		if (!strncmp("IMG_MEM_G_ID", dmabuf->name, 12))
			dev_info(pipe->imgsys_dev->dev,
			"[%s]%s: fd(%d) GCE buffer used\n", __func__, dmabuf->name, kfd[i]);
		spin_unlock(&dmabuf->name_lock);
#endif
		attach = dma_buf_attach(dmabuf, pipe->imgsys_dev->smmu_dev);
		if (IS_ERR(attach)) {
			dma_buf_put(dmabuf);
			pr_info("dma_buf_attach fail fd:%d\n", kfd[i]);
			return -ENOMEM;
		}

		#ifdef NEW_DMA_BUF_API
		sgt = dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
		#else
		sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
		#endif
		if (IS_ERR(sgt)) {
			dma_buf_detach(dmabuf, attach);
			dma_buf_put(dmabuf);
			pr_info("%s:dma_buf_map_attachment sgt err: fd %d\n",
						__func__, kfd[i]);
			return -ENOMEM;
		}

		dma_addr = sg_dma_address(sgt->sgl);

		fd_iova = vzalloc(sizeof(*fd_iova));
		if (fd_iova == NULL)
			return -ENOMEM;
		fd_iova->ionfd = kfd[i];
		fd_iova->dma_addr = dma_addr;
		fd_iova->dma_buf = dmabuf;
		fd_iova->attach = attach;
		fd_iova->sgt = sgt;
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
				"%s:dma_buf:%lx,attach:%lx,sgt:%lx\n", __func__,
			(unsigned long)fd_iova->dma_buf, (unsigned long)fd_iova->attach, (unsigned long)fd_iova->sgt);

		spin_lock(&pipe->iova_cache.lock);
		list_add_tail(&fd_iova->list_entry, &pipe->iova_cache.list);
		hash_add(pipe->iova_cache.hlists, &fd_iova->hnode, fd_iova->ionfd);
		spin_unlock(&pipe->iova_cache.lock);
		fd_info.fds_size[i] = dmabuf->size;
		fd_info.fds[i] = kfd[i];
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev, "%s: fd(%d) size (%lx) cache added\n", __func__,
					fd_info.fds[i], fd_info.fds_size[i]);

	}


	vfree(kfd);

	return 0;
}

static int mtkdip_ioc_del_iova(struct v4l2_subdev *subdev, void *arg)
{
	struct mtk_imgsys_pipe *pipe = mtk_imgsys_subdev_to_pipe(subdev);
	struct mtk_imgsys_dma_buf_iova_get_info *iova_info, *tmp;
	struct fd_tbl *fd_tbl = (struct fd_tbl *)arg;
	struct fd_info fd_info;
	struct dma_buf *dmabuf;
	unsigned int *kfd;
	size_t size;
	int i, ret;
	bool found = false;

	if ((!fd_tbl->fds) || (!fd_tbl->fd_num) || (fd_tbl->fd_num > FD_MAX)) {
		dev_dbg(pipe->imgsys_dev->dev, "%s:NULL usrptr\n", __func__);
		return -EINVAL;
	}

	size = sizeof(*kfd) * fd_tbl->fd_num;
	kfd = vzalloc(size);
	if (kfd == NULL)
		return -ENOMEM;
	ret = copy_from_user(kfd, (void *)fd_tbl->fds, size);
	if (ret != 0) {
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
			"[%s]%s:copy_from_user fail !!!\n",
			__func__,
			pipe->desc->name);
		vfree(kfd);
		return -EINVAL;
	}

	fd_info.fd_num = fd_tbl->fd_num;

	for (i = 0; i < fd_tbl->fd_num; i++) {
		unsigned int fd = kfd[i];

		if (!fd) {
			fd_info.fds[i] = 0;
			fd_info.fds_size[i] = 0;
			continue;
		}
		dmabuf = dma_buf_get(fd);
		if (IS_ERR(dmabuf))
			continue;
		found = false;
		spin_lock(&pipe->iova_cache.lock);
		list_for_each_entry_safe(iova_info, tmp,
					&pipe->iova_cache.list, list_entry) {
			if ((iova_info->ionfd == fd) &&
						(iova_info->dma_buf == dmabuf)) {
				list_del(&iova_info->list_entry);
				hash_del(&iova_info->hnode);
				found = true;
				break;
			}
		}
		spin_unlock(&pipe->iova_cache.lock);

		if (found) {
			mtk_imgsys_put_dma_buf(iova_info->dma_buf,
					iova_info->attach,
					iova_info->sgt);
			vfree(iova_info);
		}
		fd_info.fds_size[i] = dmabuf->size;
		dma_buf_put(dmabuf);

		fd_info.fds[i] = kfd[i];
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev, "%s: fd(%d) size (%lx) cache invalidated\n",
					__func__, fd_info.fds[i], fd_info.fds_size[i]);

	}


	vfree(kfd);

	return 0;
}

#if SMVR_DECOUPLE
static int imgsys_send(struct platform_device *pdev, enum hcp_id id,
		    void *buf, unsigned int  len, int req_fd, unsigned int wait)
{
	int ret = 0;
#if MTK_CM4_SUPPORT
	ret = scp_ipi_send(imgsys_dev->scp_pdev, SCP_IPI_DIP, &ipi_param,
			   sizeof(ipi_param), 0);
#else
	if (wait)
		ret = mtk_hcp_send(pdev, id, buf, len, req_fd);
	else
		ret = mtk_hcp_send_async(pdev, id, buf, len, req_fd);
#endif
	return ret;
}
#endif
static int mtkdip_ioc_s_init_info(struct v4l2_subdev *subdev, void *arg)
{
	/* struct mtk_imgsys_pipe *pipe = mtk_imgsys_subdev_to_pipe(subdev); */
	struct init_info *info = (struct init_info *)arg;
	struct mtk_imgsys_pipe *pipe;

	pipe = container_of(subdev, struct mtk_imgsys_pipe, subdev);
	#if SMVR_DECOUPLE
	pipe->ini_info = *info;
	#else
	pipe->init_info = *info;
	#endif
	/* TODO: HCP API */

	pr_info("%s sensor_info width:%d height:%d\n", __func__,
				info->sensor.full_wd, info->sensor.full_ht);

	return 0;
}
#if SMVR_DECOUPLE
static int mtkdip_ioc_alloc_buffer(struct v4l2_subdev *subdev, void *arg)
{
	/* struct mtk_imgsys_pipe *pipe = mtk_imgsys_subdev_to_pipe(subdev); */
	struct mem_info *info = (struct mem_info *)arg;
	struct mtk_imgsys_pipe *pipe;
    struct img_init_info working_buf_info;
    int ret = 0;
	struct resource *imgsys_resource;
	struct buf_va_info_t *buf;
	struct dma_buf *dbuf;
    unsigned int gce_buf_en = 0;


	pipe = container_of(subdev, struct mtk_imgsys_pipe, subdev);
	imgsys_resource = pipe->imgsys_dev->imgsys_resource;
    if (pipe->streaming) {
		/* IMGSYS HW INIT */
		memset(&working_buf_info, 0, sizeof(working_buf_info));
		working_buf_info.drv_data = (u64)&pipe->imgsys_dev;
		working_buf_info.header_version = HEADER_VER;
		working_buf_info.dip_param_size = sizeof(struct dip_param);
		working_buf_info.param_pack_size = sizeof(struct frame_param_pack);
		working_buf_info.frameparam_size = sizeof(struct img_ipi_frameparam);
		working_buf_info.reg_phys_addr = imgsys_resource->start;
		working_buf_info.reg_range = resource_size(imgsys_resource);
		//working_buf_info.is_capture = info->is_capture;
		//working_buf_info.smvr_mode = info->is_smvr;
		buf = get_first_sd_buf();
		if (!buf) {
			pr_debug("%s: no single device buff added\n", __func__);
		} else {
			dbuf = (struct dma_buf *)buf->dma_buf_putkva;
			working_buf_info.hw_buf_size = dbuf->size;
			working_buf_info.hw_buf_fd = buf->buf_fd;
		}
        pr_info("before-%s working buf is_smvr/is_capture/smvr/capture/streaming/user_count(%d/%d/%d/%d/%d/%d)\n",
				__func__,
				info->is_smvr, info->is_capture,
				pipe->imgsys_dev->imgsys_pipe[0].smvr_alloc,
				pipe->imgsys_dev->imgsys_pipe[0].capture_alloc,
				pipe->imgsys_dev->imgsys_pipe[0].streaming_alloc,
				pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count);
		working_buf_info.sec_tag = pipe->imgsys_dev->imgsys_pipe[0].ini_info.sec_tag;
		working_buf_info.full_wd = pipe->imgsys_dev->imgsys_pipe[0].ini_info.sensor.full_wd;
		working_buf_info.full_ht = pipe->imgsys_dev->imgsys_pipe[0].ini_info.sensor.full_ht;
        if (info->is_capture) {
            if (!pipe->imgsys_dev->imgsys_pipe[0].capture_alloc) {
                    //if (pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count == 0) {
                    //    gce_buf_en = 0;
                    //    pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count++;
                    //}
                mtk_hcp_allocate_working_buffer(pipe->imgsys_dev->scp_pdev, imgsys_capture, gce_buf_en);
                working_buf_info.smvr_mode = 0;
                working_buf_info.is_capture = info->is_capture;
            mtk_hcp_get_init_info(pipe->imgsys_dev->scp_pdev, &working_buf_info);
            ret = imgsys_send(pipe->imgsys_dev->scp_pdev, HCP_IMGSYS_ALOC_WORKING_BUF_ID,
			(void *)&working_buf_info, sizeof(working_buf_info), 0, 1);
                    }
            pipe->imgsys_dev->imgsys_pipe[0].capture_alloc++;
            pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count++;
		pr_info("cap buf fd(%d/%d/%d/%d)",
			working_buf_info.module_info_capture[0].c_wbuf_fd,
			working_buf_info.module_info_capture[0].t_wbuf_fd,
			working_buf_info.module_info_capture[3].c_wbuf_fd,
			working_buf_info.module_info_capture[3].t_wbuf_fd);
        } else {
            if (info->is_smvr) {
                if (!pipe->imgsys_dev->imgsys_pipe[0].smvr_alloc) {
                        //if (pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count == 0) {
                        //    gce_buf_en = 0;
                        //    pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count++;
                        //}
                    mtk_hcp_allocate_working_buffer(pipe->imgsys_dev->scp_pdev, imgsys_smvr, gce_buf_en);
                    working_buf_info.smvr_mode = info->is_smvr;
                    working_buf_info.is_capture = 0;
                mtk_hcp_get_init_info(pipe->imgsys_dev->scp_pdev, &working_buf_info);
                ret = imgsys_send(pipe->imgsys_dev->scp_pdev, HCP_IMGSYS_ALOC_WORKING_BUF_ID,
			       (void *)&working_buf_info, sizeof(working_buf_info), 0, 1);
                }
                pipe->imgsys_dev->imgsys_pipe[0].smvr_alloc++;
                pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count++;
		pr_info("smvr buf fd(%d/%d/%d/%d)",
			working_buf_info.module_info_smvr[0].c_wbuf_fd,
			working_buf_info.module_info_smvr[0].t_wbuf_fd,
			working_buf_info.module_info_smvr[3].c_wbuf_fd,
			working_buf_info.module_info_smvr[3].t_wbuf_fd);
            } else {
                if (!pipe->imgsys_dev->imgsys_pipe[0].streaming_alloc) {
                    //if (pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count == 0) {
                    //    gce_buf_en = 0;
                    //    pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count++;
                    //}
                mtk_hcp_allocate_working_buffer(pipe->imgsys_dev->scp_pdev, imgsys_streaming, gce_buf_en);
                    working_buf_info.smvr_mode = 0;
                    working_buf_info.is_capture = 0;
                mtk_hcp_get_init_info(pipe->imgsys_dev->scp_pdev, &working_buf_info);
                ret = imgsys_send(pipe->imgsys_dev->scp_pdev, HCP_IMGSYS_ALOC_WORKING_BUF_ID,
			         (void *)&working_buf_info, sizeof(working_buf_info), 0, 1);
                }
                pipe->imgsys_dev->imgsys_pipe[0].streaming_alloc++;
                pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count++;
		pr_info("streaming buf fd(%d/%d/%d/%d)",
			working_buf_info.module_info_streaming[0].c_wbuf_fd,
			working_buf_info.module_info_streaming[0].t_wbuf_fd,
			working_buf_info.module_info_streaming[3].c_wbuf_fd,
			working_buf_info.module_info_streaming[3].t_wbuf_fd);
            }
        }
    }
	pipe->meminfo = *info;
	/* TODO: HCP API */

	pr_info("%s working buf is_smvr/is_capture/smvr/capture/streaming/user_count(%d/%d/%d/%d/%d/%d)\n", __func__,
				info->is_smvr, info->is_capture,
				pipe->imgsys_dev->imgsys_pipe[0].smvr_alloc,
				pipe->imgsys_dev->imgsys_pipe[0].capture_alloc,
				pipe->imgsys_dev->imgsys_pipe[0].streaming_alloc,
				pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count);

	return ret;
}

static int mtkdip_ioc_free_buffer(struct v4l2_subdev *subdev, void *arg)
{
	/* struct mtk_imgsys_pipe *pipe = mtk_imgsys_subdev_to_pipe(subdev); */
	struct mem_info *info = (struct mem_info *)arg;
	struct mtk_imgsys_pipe *pipe;
    struct img_init_info working_buf_info;
    int ret;
	//struct buf_va_info_t *buf;
	//struct dma_buf *dbuf;
	struct resource *imgsys_resource;


	pipe = container_of(subdev, struct mtk_imgsys_pipe, subdev);
	imgsys_resource = pipe->imgsys_dev->imgsys_resource;

    memset(&working_buf_info, 0, sizeof(working_buf_info));
    working_buf_info.drv_data = (u64)&pipe->imgsys_dev;
    working_buf_info.header_version = HEADER_VER;
	working_buf_info.dip_param_size = sizeof(struct dip_param);
	working_buf_info.param_pack_size = sizeof(struct frame_param_pack);
	working_buf_info.frameparam_size = sizeof(struct img_ipi_frameparam);
	working_buf_info.reg_phys_addr = imgsys_resource->start;
	working_buf_info.reg_range = resource_size(imgsys_resource);


    pr_info("%s: before free buf is_smvr/is_capture/smvr/capture//streaming/user_count(%d/%d/%d/%d/%d/%d)",
			__func__,
    		info->is_smvr, info->is_capture,
           pipe->imgsys_dev->imgsys_pipe[0].smvr_alloc,
    		pipe->imgsys_dev->imgsys_pipe[0].capture_alloc,
			pipe->imgsys_dev->imgsys_pipe[0].streaming_alloc,
			pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count);
    if (pipe->streaming) {
        if ((info->is_capture) && (pipe->imgsys_dev->imgsys_pipe[0].capture_alloc)) {
                if (pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count != 0)
                    pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count--;
                pipe->imgsys_dev->imgsys_pipe[0].capture_alloc--;
                if (pipe->imgsys_dev->imgsys_pipe[0].capture_alloc == 0) {
                mtk_hcp_ioc_release_working_buffer(pipe->imgsys_dev->scp_pdev, imgsys_capture);
                working_buf_info.is_capture = 1;
                working_buf_info.smvr_mode = 0;
            ret = imgsys_send(pipe->imgsys_dev->scp_pdev, HCP_IMGSYS_FREE_WORKING_BUF_ID,
			(void *)&working_buf_info, sizeof(working_buf_info), 0, 1);
                }
        } else {
            if ((info->is_smvr) && (pipe->imgsys_dev->imgsys_pipe[0].smvr_alloc)) {
                if (pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count != 0)
                    pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count--;
                pipe->imgsys_dev->imgsys_pipe[0].smvr_alloc--;
                if (pipe->imgsys_dev->imgsys_pipe[0].smvr_alloc == 0) {
                mtk_hcp_ioc_release_working_buffer(pipe->imgsys_dev->scp_pdev, imgsys_smvr);
                working_buf_info.smvr_mode = 1;
                working_buf_info.is_capture = 0;
                ret = imgsys_send(pipe->imgsys_dev->scp_pdev, HCP_IMGSYS_FREE_WORKING_BUF_ID,
			       (void *)&working_buf_info, sizeof(working_buf_info), 0, 1);
                }
            } else {
                if (pipe->imgsys_dev->imgsys_pipe[0].streaming_alloc) {
                    if (pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count != 0)
                        pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count--;
                    pipe->imgsys_dev->imgsys_pipe[0].streaming_alloc--;
                    if (pipe->imgsys_dev->imgsys_pipe[0].streaming_alloc == 0) {
                mtk_hcp_ioc_release_working_buffer(pipe->imgsys_dev->scp_pdev, imgsys_streaming);
                    working_buf_info.is_capture = 0;
    		        working_buf_info.smvr_mode = 0;
                ret = imgsys_send(pipe->imgsys_dev->scp_pdev, HCP_IMGSYS_FREE_WORKING_BUF_ID,
			         (void *)&working_buf_info, sizeof(working_buf_info), 0, 1);
                }
            }
        }
    }
    }

	pr_info("%s free working buf is_smvr/is_capture/smvr/capture/streaming/user_count(%d/%d/%d/%d/%d/%d)\n",
				__func__,
				info->is_smvr, info->is_capture,
				pipe->imgsys_dev->imgsys_pipe[0].smvr_alloc,
				pipe->imgsys_dev->imgsys_pipe[0].capture_alloc,
				pipe->imgsys_dev->imgsys_pipe[0].streaming_alloc,
				pipe->imgsys_dev->imgsys_pipe[0].imgsys_user_count);
	return 0;
}
#endif
static int mtkdip_ioc_acquire_iova(struct v4l2_subdev *subdev, void *arg)
{
	struct mtk_imgsys_pipe *pipe = mtk_imgsys_subdev_to_pipe(subdev);
	struct mtk_imgsys_dma_buf_iova_get_info *fd_iova;
	struct mtk_imgsys_fd_info_tbl *fd_tbl = (struct mtk_imgsys_fd_info_tbl *)arg;
	struct mtk_imgsys_fd_info fd_info;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	int i;
	uint8_t acp_coherence_enable;

	if (!fd_tbl->fds[0].fd) {
		dev_dbg(pipe->imgsys_dev->dev, "%s:NULL usrptr\n", __func__);
		return -EINVAL;
	}

	if (fd_tbl->fd_num > FD_MAX)
		return -EINVAL;

	if (imgsys_dbg_enable())
		dev_info(pipe->imgsys_dev->dev,
			"[%s]%s-%d:iova num !!!\n",
			__func__,
			pipe->desc->name, fd_tbl->fd_num);

	for (i = 0; i < fd_tbl->fd_num; i++) {
		if (!fd_tbl->fds[i].fd) {
			fd_info.fd = 0;
			continue;
		}

		acp_coherence_enable = fd_tbl->fds[i].enable_acp_coherance;

		dmabuf = dma_buf_get(fd_tbl->fds[i].fd);
		if (IS_ERR(dmabuf))
			return -ENOMEM;
#ifdef IMG_MEM_G_ID_DEBUG
		spin_lock(&dmabuf->name_lock);
		if (!strncmp("IMG_MEM_G_ID", dmabuf->name, 12))
			dev_info(pipe->imgsys_dev->dev,
			"[%s]%s: fd(%d) GCE buffer used\n", __func__, dmabuf->name, fd_tbl->fds[i].fd);
		spin_unlock(&dmabuf->name_lock);
#endif
		if (!pipe->imgsys_dev->acp_enable) {
			attach = dma_buf_attach(dmabuf, pipe->imgsys_dev->smmu_dev);
		} else {
			if (acp_coherence_enable)
				attach = dma_buf_attach(dmabuf, pipe->imgsys_dev->acp_smmu_dev);
			else
				attach = dma_buf_attach(dmabuf, pipe->imgsys_dev->smmu_dev);
		}
		if (IS_ERR(attach)) {
			dma_buf_put(dmabuf);
			pr_info("dma_buf_attach fail fd:%d\n", fd_tbl->fds[i].fd);
			return -ENOMEM;
		}

		#ifdef NEW_DMA_BUF_API
		sgt = dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
		#else
		sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
		#endif
		if (IS_ERR(sgt)) {
			dma_buf_detach(dmabuf, attach);
			dma_buf_put(dmabuf);
			pr_info("%s:dma_buf_map_attachment sgt err: fd %d\n",
						__func__, fd_tbl->fds[i].fd);
			return -ENOMEM;
		}

		dma_addr = sg_dma_address(sgt->sgl);

		fd_iova = vzalloc(sizeof(*fd_iova));
		if (fd_iova == NULL)
			return -ENOMEM;
		fd_iova->ionfd = fd_tbl->fds[i].fd;
		fd_iova->dma_addr = dma_addr;
		fd_iova->dma_buf = dmabuf;
		fd_iova->attach = attach;
		fd_iova->sgt = sgt;
		if (imgsys_dbg_enable())
			dev_info(pipe->imgsys_dev->dev,
				"%s:dma_buf:%lx,attach:%lx,sgt:%lx,acp(%d)\n",
				__func__,
				(unsigned long)fd_iova->dma_buf,
				(unsigned long)fd_iova->attach,
				(unsigned long)fd_iova->sgt,
				acp_coherence_enable);

		spin_lock(&pipe->iova_cache.lock);
		list_add_tail(&fd_iova->list_entry, &pipe->iova_cache.list);
		hash_add(pipe->iova_cache.hlists, &fd_iova->hnode, fd_iova->ionfd);
		spin_unlock(&pipe->iova_cache.lock);
		fd_info.fd = fd_tbl->fds[i].fd;
		if (imgsys_dbg_enable())
			dev_dbg(pipe->imgsys_dev->dev, "%s: fd(%d) cache added\n",
				__func__,
				fd_info.fd);
	}

	return 0;
}

static int mtkdip_ioc_release_iova(struct v4l2_subdev *subdev, void *arg)
{
	struct mtk_imgsys_pipe *pipe = mtk_imgsys_subdev_to_pipe(subdev);
	struct mtk_imgsys_dma_buf_iova_get_info *iova_info, *tmp;
	struct mtk_imgsys_fd_info_tbl *fd_tbl = (struct mtk_imgsys_fd_info_tbl *)arg;
	struct mtk_imgsys_fd_info fd_info;
	struct dma_buf *dmabuf;
	int i;
	bool found = false;

	if ((!fd_tbl->fds[0].fd) || (!fd_tbl->fd_num) || (fd_tbl->fd_num > FD_MAX)) {
		dev_dbg(pipe->imgsys_dev->dev, "%s:NULL usrptr\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < fd_tbl->fd_num; i++) {
		unsigned int fd = fd_tbl->fds[i].fd;

		if (!fd)
			continue;

		dmabuf = dma_buf_get(fd);
		if (IS_ERR(dmabuf))
			continue;
		found = false;
		spin_lock(&pipe->iova_cache.lock);
		list_for_each_entry_safe(iova_info, tmp,
					&pipe->iova_cache.list, list_entry) {
			if ((iova_info->ionfd == fd) &&
						(iova_info->dma_buf == dmabuf)) {
				list_del(&iova_info->list_entry);
				hash_del(&iova_info->hnode);
				found = true;
				break;
			}
		}
		spin_unlock(&pipe->iova_cache.lock);

		if (found) {
			mtk_imgsys_put_dma_buf(iova_info->dma_buf,
					iova_info->attach,
					iova_info->sgt);
			vfree(iova_info);
		}
		dma_buf_put(dmabuf);

		fd_info.fd = fd_tbl->fds[i].fd;
		if (imgsys_dbg_enable())
			dev_dbg(pipe->imgsys_dev->dev,
				"%s: fd(%d) cache invalidated\n",
				__func__, fd_info.fd);

	}

	return 0;
}

static int mtkdip_ioc_set_control(struct v4l2_subdev *subdev, void *arg)
{
	struct mtk_imgsys_pipe *pipe = mtk_imgsys_subdev_to_pipe(subdev);
	struct ctrl_info *ctrl = (struct ctrl_info *)arg;
	int ret = -1;

	if (!ctrl) {
		return -EINVAL;
        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev, "%s:NULL usrptr\n", __func__);
	}

	pr_info("%s set control:%d, %d\n", __func__, ctrl->id, ctrl->value);

	switch (ctrl->id) {
	case V4L2_CID_IMGSYS_APU_DC:
		ret = mtk_hcp_set_apu_dc(pipe->imgsys_dev->scp_pdev,
			ctrl->value, sizeof(ctrl->value));
		break;
	default:
	  pr_info("%s: non-supported ctrl id(%x)\n", __func__, ctrl->id);
		break;
	}

	return ret;
}

long mtk_imgsys_subdev_ioctl(struct v4l2_subdev *subdev, unsigned int cmd,
								void *arg)
{
    if (imgsys_dbg_enable()) {
	pr_debug("%s cmd: %d\n", __func__, cmd);
    }

	switch (cmd) {
	case MTKDIP_IOC_ADD_KVA:
		return mtkdip_ioc_add_kva(subdev, arg);
	case MTKDIP_IOC_DEL_KVA:
		return mtkdip_ioc_del_kva(subdev, arg);
	case MTKDIP_IOC_ADD_IOVA:
		return mtkdip_ioc_add_iova(subdev, arg);
	case MTKDIP_IOC_DEL_IOVA:
		return mtkdip_ioc_del_iova(subdev, arg);
	case MTKDIP_IOC_S_INIT_INFO:
		return mtkdip_ioc_s_init_info(subdev, arg);
	case MTKDIP_IOC_SET_CONTROL:
		return mtkdip_ioc_set_control(subdev, arg);
    case MTKDIP_IOC_ALOC_BUF:
		return mtkdip_ioc_alloc_buffer(subdev, arg);
    case MTKDIP_IOC_FREE_BUF:
		return mtkdip_ioc_free_buffer(subdev, arg);
	case MTKDIP_IOC_ACQUIRE_IOVA:
		return mtkdip_ioc_acquire_iova(subdev, arg);
	case MTKDIP_IOC_RELEASE_IOVA:
		return mtkdip_ioc_release_iova(subdev, arg);
	default:
		pr_info("%s: non-supported cmd(%x)\n", __func__, cmd);
		return -ENOTTY;
	}

	return 0;
}


static struct media_request *mtk_imgsys_request_alloc(struct media_device *mdev)
{
	struct mtk_imgsys_request *imgsys_req = NULL;
	struct media_request *req = NULL;
	struct mtk_imgsys_dev *imgsys = mtk_imgsys_mdev_to_dev(mdev);
	size_t bufs_size;

	imgsys_req = vzalloc(sizeof(*imgsys_req));

	if (imgsys_req) {
		bufs_size = imgsys->imgsys_pipe[0].desc->total_queues *
					sizeof(struct mtk_imgsys_dev_buffer *);
		imgsys_req->buf_map = vzalloc(bufs_size);
		req = &imgsys_req->req;
	} else
		pr_info("%s: vzalloc fails\n", __func__);

	return req;
}

static void mtk_imgsys_request_free(struct media_request *req)
{
	struct mtk_imgsys_request *imgsys_req =
					mtk_imgsys_media_req_to_imgsys_req(req);
	long leavetime = 0;

	if (imgsys_req->used)
		leavetime = wait_for_completion_timeout(&imgsys_req->done, HZ);
	if (leavetime < 0)
		pr_info("request_free_timeout(%ld)", leavetime);
	vfree(imgsys_req->buf_map);
	vfree(imgsys_req);

}

static int mtk_imgsys_vb2_request_validate(struct media_request *req)
{
	struct media_request_object *obj;
	struct mtk_imgsys_dev *imgsys_dev = mtk_imgsys_mdev_to_dev(req->mdev);
	struct mtk_imgsys_request *imgsys_req =
					mtk_imgsys_media_req_to_imgsys_req(req);
	struct mtk_imgsys_pipe *pipe = NULL;
	struct mtk_imgsys_pipe *pipe_prev = NULL;
	struct mtk_imgsys_dev_buffer **buf_map = imgsys_req->buf_map;
	int buf_count = 0;
	int i;

	imgsys_req->buf_fd = 0;
	imgsys_req->buf_va_daddr = 0;
	imgsys_req->buf_same = false;

	for (i = 0; i < imgsys_dev->imgsys_pipe[0].desc->total_queues; i++)
		buf_map[i] = NULL;

	list_for_each_entry(obj, &req->objects, list) {
		struct vb2_buffer *vb;
		struct mtk_imgsys_dev_buffer *dev_buf;
		struct mtk_imgsys_video_device *node;

		if (!vb2_request_object_is_buffer(obj))
			continue;

		vb = container_of(obj, struct vb2_buffer, req_obj);
		node = mtk_imgsys_vbq_to_node(vb->vb2_queue);
		pipe = vb2_get_drv_priv(vb->vb2_queue);
		if (pipe_prev && pipe != pipe_prev) {
            if (imgsys_dbg_enable())
			dev_dbg(imgsys_dev->dev,
				"%s:%s:%s:found buf of different pipes(%p,%p)\n",
				__func__, node->desc->name,
				req->debug_str, pipe, pipe_prev);
			return -EINVAL;
		}

		pipe_prev = pipe;
		dev_buf = mtk_imgsys_vb2_buf_to_dev_buf(vb);
		imgsys_req->buf_map[node->desc->id] = dev_buf;
		buf_count++;

        if (imgsys_dbg_enable())
		dev_dbg(pipe->imgsys_dev->dev,
			"%s:%s: added buf(%p) to pipe-job(%p), buf_count(%d)\n",
			pipe->desc->name, node->desc->name, dev_buf,
			imgsys_req, buf_count);
	}

	if (!pipe) {
        if (imgsys_dbg_enable())
		dev_dbg(imgsys_dev->dev,
			"%s: no buffer in the request(%p)\n",
			req->debug_str, req);

		return -EINVAL;
	}

	atomic_set(&imgsys_req->buf_count, buf_count);
	atomic_set(&imgsys_req->swfrm_cnt, 0);
	imgsys_req->id = mtk_imgsys_pipe_next_job_id(pipe);
	imgsys_req->imgsys_pipe = pipe;
	imgsys_req->tstate.time_sendtask = 0;
	init_completion(&imgsys_req->done);
	imgsys_req->used = true;
	mtk_imgsys_pipe_debug_job(pipe, imgsys_req);

	return vb2_request_validate(req);
}

static void mtk_imgsys_vb2_request_queue(struct media_request *req)
{
	struct mtk_imgsys_dev *imgsys_dev = mtk_imgsys_mdev_to_dev(req->mdev);
	struct mtk_imgsys_request *imgsys_req =
					mtk_imgsys_media_req_to_imgsys_req(req);
	struct mtk_imgsys_pipe *pipe = imgsys_req->imgsys_pipe;
	unsigned long flag;

	spin_lock_irqsave(&pipe->pending_job_lock, flag);
	list_add_tail(&imgsys_req->list, &pipe->pipe_job_pending_list);
	pipe->num_pending_jobs++;
    if (imgsys_dbg_enable())
	dev_dbg(imgsys_dev->dev,
		"%s:%s: current num of pending jobs(%d)\n",
		__func__, pipe->desc->name, pipe->num_pending_jobs);
	spin_unlock_irqrestore(&pipe->pending_job_lock, flag);
#if MTK_V4L2_BATCH_MODE_SUPPORT == 1
	#ifdef BATCH_MODE_V3
	imgsys_req->is_batch_mode = false;
    if (imgsys_dbg_enable())
	dev_dbg(imgsys_dev->dev,
		"%s:is_batch_mode(%d)\n",
		__func__, imgsys_req->is_batch_mode);
	#endif
#endif
	vb2_request_queue(req);

	if (imgsys_req->req_stat) {
		union request_track *req_track = (union request_track *)imgsys_req->req_stat;

		req_track->subflow_kernel++;
		req_track->mainflow_from = REQUEST_FROM_KERNEL_TO_IMGSTREAM;
	}
}

int mtk_imgsys_v4l2_fh_open(struct file *filp)
{
	struct mtk_imgsys_user *user;
	struct mtk_imgsys_pipe *pipe = video_drvdata(filp);
	//struct mtk_imgsys_video_device *node = mtk_imgsys_file_to_node(filp);
	struct mtk_imgsys_dev *imgsys_dev = pipe->imgsys_dev;
	static unsigned short count = MTK_IMGSYS_PIPE_ID_REPROCESS;

	v4l2_fh_open(filp);
	// keep fh in user structure
	user = vzalloc(sizeof(*user));
	if (user == NULL)
		return -ENOMEM;

	user->fh = filp->private_data;
	user->id = ++count;
	init_waitqueue_head(&user->done_wq);
	init_waitqueue_head(&user->enque_wq);
	INIT_LIST_HEAD(&user->done_list);
	user->dqdonestate = false;
	user->state = DIP_STATE_INIT;
	spin_lock_init(&user->lock);

	// add user to list head
	mutex_lock(&imgsys_dev->imgsys_users.user_lock);
	list_add_tail(&user->entry, &imgsys_dev->imgsys_users.list);
	mutex_unlock(&imgsys_dev->imgsys_users.user_lock);
    if (imgsys_dbg_enable()) {
	pr_debug("%s: id(%d)\n", __func__, user->id);
    }
	return 0;
}

int mtk_imgsys_v4l2_fh_release(struct file *filp)
{
	struct mtk_imgsys_user *user;
	struct mtk_imgsys_pipe *pipe = video_drvdata(filp);
	struct mtk_imgsys_dev *imgsys_dev = pipe->imgsys_dev;
	int ret = 0;

    if (imgsys_dbg_enable()) {
	pr_debug("%s: filp(%lx)\n", __func__, (unsigned long)filp);
    }
	ret = get_user_by_file(filp, &user);
	if (ret < 0) {
		pr_info("%s: cannot find user\n", __func__);
	} else {
		// delete user to list head
		mutex_lock(&imgsys_dev->imgsys_users.user_lock);
		list_del(&user->entry);
		mutex_unlock(&imgsys_dev->imgsys_users.user_lock);
        if (imgsys_dbg_enable()) {
		pr_debug("%s: id(%d)\n", __func__, user->id);
        }
		vfree(user);
	}
	vb2_fop_release(filp);

	return 0;
}

int mtk_imgsys_dev_media_register(struct device *dev,
			       struct media_device *media_dev)
{
	int ret;
	int j = 0;

	media_dev->dev = dev;
	strscpy(media_dev->model, MTK_DIP_DEV_DIP_MEDIA_MODEL_NAME,
		sizeof(media_dev->model));
	j = snprintf(media_dev->bus_info, sizeof(media_dev->bus_info),
		 "platform:%s", dev_name(dev));
	if (j < 0)
		pr_info("imgsys_fw: write dev name fail(%d)\n", j);
	media_dev->hw_revision = 0;
	media_dev->ops = &mtk_imgsys_media_req_ops;
	media_device_init(media_dev);

	ret = media_device_register(media_dev);
	if (ret) {
		dev_info(dev, "failed to register media device (%d)\n", ret);
		media_device_unregister(media_dev);
		media_device_cleanup(media_dev);
		return ret;
	}

    if (imgsys_dbg_enable())
	dev_dbg(dev, "Registered media device: %s, %p", media_dev->model,
		media_dev);

	return 0;
}

static int mtk_imgsys_video_device_v4l2_register(struct mtk_imgsys_pipe *pipe,
					   struct mtk_imgsys_video_device *node)
{
	struct vb2_queue *vbq = &node->dev_q.vbq;
	struct video_device *vdev = &node->vdev;
	struct media_link *link;
	int ret;

	mutex_init(&node->dev_q.lock);

	vdev->device_caps = node->desc->cap;
	if (vdev->device_caps & V4L2_CAP_VIDEO_OUTPUT_MPLANE)
		vdev->ioctl_ops = &mtk_imgsys_v4l2_video_out_ioctl_ops;
	else if (vdev->device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		vdev->ioctl_ops = &mtk_imgsys_v4l2_video_cap_ioctl_ops;
	else if (vdev->device_caps & V4L2_CAP_META_OUTPUT)
		vdev->ioctl_ops = &mtk_imgsys_v4l2_meta_out_ioctl_ops;
	else
		dev_info(pipe->imgsys_dev->dev, "%s unsupported cap %x\n",
						__func__, vdev->device_caps);

	node->vdev_fmt.type = node->desc->buf_type;
	mtk_imgsys_pipe_load_default_fmt(pipe, node, &node->vdev_fmt);

	node->vdev_pad.flags = V4L2_TYPE_IS_OUTPUT(node->desc->buf_type) ?
		MEDIA_PAD_FL_SOURCE : MEDIA_PAD_FL_SINK;

	snprintf(vdev->name, sizeof(vdev->name), "%s %s", pipe->desc->name,
		 node->desc->name);
	vdev->entity.name = vdev->name;
	vdev->entity.function = MEDIA_ENT_F_IO_V4L;
	vdev->entity.ops = NULL;
	vdev->release = video_device_release_empty;
	vdev->fops = &mtk_imgsys_v4l2_fops;
	vdev->lock = &node->dev_q.lock;

	ret = media_entity_pads_init(&vdev->entity, 1, &node->vdev_pad);
	if (ret) {
		dev_info(pipe->imgsys_dev->dev,
			"failed initialize media entity (%d)\n", ret);
		goto err_mutex_destroy;
	}


	vbq->type = node->vdev_fmt.type;
	vbq->io_modes = VB2_MMAP | VB2_DMABUF;
	if (vbq->type & V4L2_BUF_TYPE_META_OUTPUT)
		vbq->ops = &mtk_imgsys_vb2_meta_ops;
	else if (vbq->type & V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		vbq->ops = &mtk_imgsys_vb2_video_ops;
	else
		dev_info(pipe->imgsys_dev->dev, "%s unsupported buf_type %x\n",
						__func__, vdev->device_caps);

	vbq->mem_ops = &mtk_imgsys_dma_contig_memops;
	vbq->supports_requests = true;
	vbq->buf_struct_size = sizeof(struct mtk_imgsys_dev_buffer);
	vbq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
#if (KERNEL_VERSION(6, 8, 0) > LINUX_VERSION_CODE)
	vbq->min_buffers_needed = 0;
#else
	vbq->min_queued_buffers = 0;
#endif
	vbq->drv_priv = pipe;
	vbq->lock = &node->dev_q.lock;

	ret = vb2_queue_init(vbq);
	if (ret) {
		dev_info(pipe->imgsys_dev->dev,
			"%s:%s:%s: failed to init vb2 queue(%d)\n",
			__func__, pipe->desc->name, node->desc->name,
			ret);
		goto err_media_entity_cleanup;
	}

	if (node->desc->supports_ctrls)
		vdev->ctrl_handler = &node->ctrl_handler;
	else
		vdev->ctrl_handler = NULL;
	vdev->v4l2_dev = &pipe->imgsys_dev->v4l2_dev;
	vdev->queue = &node->dev_q.vbq;
	vdev->vfl_dir = V4L2_TYPE_IS_OUTPUT(node->desc->buf_type) ?
		VFL_DIR_TX : VFL_DIR_RX;

	if (node->desc->smem_alloc) {
		vdev->queue->dev = &pipe->imgsys_dev->scp_pdev->dev;
		if (imgsys_dbg_enable())
			dev_info(pipe->imgsys_dev->dev,
				"%s:%s: select smem_vb2_alloc_ctx(%p)\n",
				pipe->desc->name, node->desc->name,
				vdev->queue->dev);
	} else {
		vdev->queue->dev = pipe->imgsys_dev->smmu_dev;
		if (imgsys_dbg_enable())
			dev_info(pipe->imgsys_dev->dev,
				"%s:%s: select default_vb2_alloc_ctx(%p)\n",
				pipe->desc->name, node->desc->name,
				pipe->imgsys_dev->dev);
	}

	video_set_drvdata(vdev, pipe);

	ret = video_register_device(vdev, VFL_TYPE_VIDEO, 6);
	if (ret) {
		dev_info(pipe->imgsys_dev->dev,
			"failed to register video device (%d)\n", ret);
		goto err_vb2_queue_release;
	}
	if (imgsys_dbg_enable())
		dev_info(pipe->imgsys_dev->dev, "registered vdev: %s\n",
			vdev->name);

	if (V4L2_TYPE_IS_OUTPUT(node->desc->buf_type)) {
		/* coverity[callee_ptr_arith : SUPPRESS] */
		ret = media_create_pad_link(&vdev->entity, 0,
					    &pipe->subdev.entity,
					    node->desc->id, node->flags);
	}
	else {
		/* coverity[callee_ptr_arith : SUPPRESS] */
		ret = media_create_pad_link(&pipe->subdev.entity,
					    node->desc->id, &vdev->entity,
					    0, node->flags);
	}
	if (ret)
		goto err_video_unregister_device;

	vdev->intf_devnode = media_devnode_create(&pipe->imgsys_dev->mdev,
						MEDIA_INTF_T_V4L_VIDEO, 0,
						VIDEO_MAJOR, vdev->minor);
	if (!vdev->intf_devnode) {
		ret = -ENOMEM;
		goto err_rm_links;
	}

	link = media_create_intf_link(&vdev->entity,
				&vdev->intf_devnode->intf,
				node->flags);
	if (!link) {
		ret = -ENOMEM;
		goto err_rm_devnode;
	}

	return 0;

err_rm_devnode:
	media_devnode_remove(vdev->intf_devnode);

err_rm_links:
	media_entity_remove_links(&vdev->entity);

err_video_unregister_device:
	video_unregister_device(vdev);

err_vb2_queue_release:
	vb2_queue_release(&node->dev_q.vbq);

err_media_entity_cleanup:
	media_entity_cleanup(&node->vdev.entity);

err_mutex_destroy:
	mutex_destroy(&node->dev_q.lock);

	return ret;
}

static int mtk_imgsys_pipe_v4l2_ctrl_init(struct mtk_imgsys_pipe *imgsys_pipe)
{
	int i, ret;
	struct mtk_imgsys_video_device *ctrl_node;

	for (i = 0; i < MTK_IMGSYS_VIDEO_NODE_ID_TOTAL_NUM; i++) {
		ctrl_node = &imgsys_pipe->nodes[i];
		if (!ctrl_node->desc->supports_ctrls)
			continue;

		v4l2_ctrl_handler_init(&ctrl_node->ctrl_handler, 1);
		v4l2_ctrl_new_std(&ctrl_node->ctrl_handler,
			&mtk_imgsys_video_device_ctrl_ops, V4L2_CID_ROTATE,
				  0, 270, 90, 0);
		ret = ctrl_node->ctrl_handler.error;
		if (ret) {
			dev_info(imgsys_pipe->imgsys_dev->dev,
				"%s create rotate ctrl failed:(%d)",
				ctrl_node->desc->name, ret);
			goto err_free_ctrl_handlers;
		}
	}

	return 0;

err_free_ctrl_handlers:
	for (; i >= 0; i--) {
		ctrl_node = &imgsys_pipe->nodes[i];
		if (!ctrl_node->desc->supports_ctrls)
			continue;
		v4l2_ctrl_handler_free(&ctrl_node->ctrl_handler);
	}

	return ret;
}

static void mtk_imgsys_pipe_v4l2_ctrl_release(
					struct mtk_imgsys_pipe *imgsys_pipe)
{
	struct mtk_imgsys_video_device *ctrl_node =
		&imgsys_pipe->nodes[MTK_IMGSYS_VIDEO_NODE_ID_WROT_A_CAPTURE];

	v4l2_ctrl_handler_free(&ctrl_node->ctrl_handler);
}

int mtk_imgsys_pipe_v4l2_register(struct mtk_imgsys_pipe *pipe,
			       struct media_device *media_dev,
			       struct v4l2_device *v4l2_dev)
{
	int i, ret, j;

	ret = mtk_imgsys_pipe_v4l2_ctrl_init(pipe);
	if (ret) {
		dev_info(pipe->imgsys_dev->dev,
			"%s: failed(%d) to initialize ctrls\n",
			pipe->desc->name, ret);

		return ret;
	}

	pipe->streaming = 0;

	/* Initialize subdev media entity */
	pipe->subdev_pads = devm_kcalloc(pipe->imgsys_dev->dev,
					 pipe->desc->total_queues,
					 sizeof(*pipe->subdev_pads),
					 GFP_KERNEL);
	if (!pipe->subdev_pads) {
		dev_info(pipe->imgsys_dev->dev,
			"failed to alloc pipe->subdev_pads (%d)\n", ret);
		ret = -ENOMEM;
		goto err_release_ctrl;
	}
	/* Initialize subdev */
	v4l2_subdev_init(&pipe->subdev, &mtk_imgsys_subdev_ops);

	pipe->subdev.entity.function =
		MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER;
	pipe->subdev.entity.ops = &mtk_imgsys_media_ops;
	pipe->subdev.flags =
		V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	pipe->subdev.ctrl_handler = NULL;
	pipe->subdev.internal_ops = &mtk_imgsys_subdev_int_ops;
	pipe->subdev.entity.flags =
		V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;

	for (i = 0; i < pipe->desc->total_queues; i++)
		pipe->subdev_pads[i].flags =
			V4L2_TYPE_IS_OUTPUT(pipe->nodes[i].desc->buf_type) ?
			MEDIA_PAD_FL_SINK : MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&pipe->subdev.entity,
				pipe->desc->total_queues,
				pipe->subdev_pads);
	if (ret) {
		dev_info(pipe->imgsys_dev->dev,
			"failed initialize subdev media entity (%d)\n", ret);
		goto err_free_subdev_pads;
	}

	j = snprintf(pipe->subdev.name, sizeof(pipe->subdev.name),
		 "%s", pipe->desc->name);
	if (j < 0)
		pr_info("register name fail\n");
	v4l2_set_subdevdata(&pipe->subdev, pipe);

	ret = v4l2_device_register_subdev(&pipe->imgsys_dev->v4l2_dev,
					  &pipe->subdev);
	if (ret) {
		dev_info(pipe->imgsys_dev->dev,
			"failed initialize subdev (%d)\n", ret);
		goto err_media_entity_cleanup;
	}

	if (imgsys_dbg_enable())
		dev_info(pipe->imgsys_dev->dev,
			"register subdev: %s, ctrl_handler %p total queue(%d)\n",
			pipe->subdev.name, pipe->subdev.ctrl_handler, pipe->desc->total_queues);


	/* Create video nodes and links */
	for (i = 0; i < pipe->desc->total_queues; i++) {
		ret = mtk_imgsys_video_device_v4l2_register(pipe,
							 &pipe->nodes[i]);
		if (ret)
			goto err_unregister_subdev;
	}

	return 0;

err_unregister_subdev:
	v4l2_device_unregister_subdev(&pipe->subdev);

err_media_entity_cleanup:
	media_entity_cleanup(&pipe->subdev.entity);

err_free_subdev_pads:
	devm_kfree(pipe->imgsys_dev->dev, pipe->subdev_pads);

err_release_ctrl:
	mtk_imgsys_pipe_v4l2_ctrl_release(pipe);

	return ret;
}

void mtk_imgsys_pipe_v4l2_unregister(struct mtk_imgsys_pipe *pipe)
{
	unsigned int i;

	for (i = 0; i < pipe->desc->total_queues; i++) {
		video_unregister_device(&pipe->nodes[i].vdev);
		vb2_queue_release(&pipe->nodes[i].dev_q.vbq);
		media_entity_cleanup(&pipe->nodes[i].vdev.entity);
		mutex_destroy(&pipe->nodes[i].dev_q.lock);
	}

	v4l2_device_unregister_subdev(&pipe->subdev);
	media_entity_cleanup(&pipe->subdev.entity);
	mtk_imgsys_pipe_v4l2_ctrl_release(pipe);
}

static void mtk_imgsys_dev_media_unregister(struct mtk_imgsys_dev *imgsys_dev)
{
	media_device_unregister(&imgsys_dev->mdev);
	media_device_cleanup(&imgsys_dev->mdev);
}

static int mtk_imgsys_dev_v4l2_init(struct mtk_imgsys_dev *imgsys_dev)
{
	struct media_device *media_dev = &imgsys_dev->mdev;
	struct v4l2_device *v4l2_dev = &imgsys_dev->v4l2_dev;
	int i;
	int ret;

	ret = mtk_imgsys_dev_media_register(imgsys_dev->dev, media_dev);
	if (ret) {
		dev_info(imgsys_dev->dev,
			"%s: media device register failed(%d)\n",
			__func__, ret);
		return ret;
	}

	v4l2_dev->mdev = media_dev;
	v4l2_dev->ctrl_handler = NULL;

	ret = v4l2_device_register(imgsys_dev->dev, v4l2_dev);
	if (ret) {
		dev_info(imgsys_dev->dev,
			"%s: v4l2 device register failed(%d)\n",
			__func__, ret);
		goto err_release_media_device;
	}

	for (i = 0; i < MTK_IMGSYS_PIPE_ID_TOTAL_NUM; i++) {
		ret = mtk_imgsys_pipe_init(imgsys_dev,
					&imgsys_dev->imgsys_pipe[i],
					&imgsys_dev->cust_pipes[i]);
		if (ret) {
			dev_info(imgsys_dev->dev,
				"%s: Pipe id(%d) init failed(%d)\n",
				imgsys_dev->imgsys_pipe[i].desc->name,
				i, ret);
			goto err_release_pipe;
		}
	}

	ret = v4l2_device_register_subdev_nodes(&imgsys_dev->v4l2_dev);
	if (ret) {
		dev_info(imgsys_dev->dev,
			"failed to register subdevs (%d)\n", ret);
		goto err_release_pipe;
	}

#ifdef MTK_IOVA_SINK2KERNEL
	imgsys_dev->imgsys_get_iova = mtk_imgsys_get_iova;
	imgsys_dev->is_singledev_mode = is_singledev_mode;
#endif

	return 0;

err_release_pipe:
	for (i--; i >= 0; i--)
		mtk_imgsys_pipe_release(&imgsys_dev->imgsys_pipe[i]);

	v4l2_device_unregister(v4l2_dev);

err_release_media_device:
	mtk_imgsys_dev_media_unregister(imgsys_dev);

	return ret;
}

void mtk_imgsys_dev_v4l2_release(struct mtk_imgsys_dev *imgsys_dev)
{
	int i;

	for (i = 0; i < MTK_IMGSYS_PIPE_ID_TOTAL_NUM; i++)
		mtk_imgsys_pipe_release(&imgsys_dev->imgsys_pipe[i]);

	v4l2_device_unregister(&imgsys_dev->v4l2_dev);
	media_device_unregister(&imgsys_dev->mdev);
	media_device_cleanup(&imgsys_dev->mdev);
}

static int mtk_imgsys_res_init(struct platform_device *pdev,
			    struct mtk_imgsys_dev *imgsys_dev)
{
	int ret;
	unsigned int i;

	/* ToDo: porting mdp3
	 * imgsys_dev->mdp_pdev = mdp_get_plat_device(pdev);
	 * if (!imgsys_dev->mdp_pdev) {
	 *	dev_dbg(imgsys_dev->dev,
	 *	"%s: failed to get MDP device\n",
	 *		__func__);
	 *	return -EINVAL;
	 * }
	 */

	imgsys_dev->mdpcb_wq =
		alloc_ordered_workqueue("%s",
					__WQ_LEGACY | WQ_MEM_RECLAIM |
					WQ_FREEZABLE,
					"imgsys_mdp_callback");
	if (!imgsys_dev->mdpcb_wq) {
		dev_info(imgsys_dev->dev,
			"%s: unable to alloc mdpcb workqueue\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	imgsys_dev->enqueue_wq =
		alloc_ordered_workqueue("%s",
					__WQ_LEGACY | WQ_MEM_RECLAIM |
					WQ_FREEZABLE | WQ_HIGHPRI,
					"imgsys_enqueue");
	if (!imgsys_dev->enqueue_wq) {
		dev_info(imgsys_dev->dev,
			"%s: unable to alloc enqueue workqueue\n", __func__);
		ret = -ENOMEM;
		return ret;
	}


	imgsys_dev->composer_wq =
		alloc_ordered_workqueue("%s",
					__WQ_LEGACY | WQ_MEM_RECLAIM |
					WQ_FREEZABLE,
					"imgsys_composer");
	if (!imgsys_dev->composer_wq) {
		dev_info(imgsys_dev->dev,
			"%s: unable to alloc composer workqueue\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	for (i = 0; i < RUNNER_WQ_NR; i++) {
		imgsys_dev->mdp_wq[i] =
			alloc_ordered_workqueue("%s_%d",
					__WQ_LEGACY | WQ_MEM_RECLAIM |
					WQ_FREEZABLE,
					"imgsys_runner", i);
		if (!imgsys_dev->mdp_wq[i]) {
			dev_info(imgsys_dev->dev,
				"%s: unable to alloc imgsys_runner\n", __func__);
			ret = -ENOMEM;
			return ret;
		}
	}

	init_waitqueue_head(&imgsys_dev->flushing_waitq);

	return 0;
}

static void mtk_imgsys_res_release(struct mtk_imgsys_dev *imgsys_dev)
{
	int i;

	for (i = 0; i < RUNNER_WQ_NR; i++) {
		flush_workqueue(imgsys_dev->mdp_wq[i]);
		destroy_workqueue(imgsys_dev->mdp_wq[i]);
		imgsys_dev->mdp_wq[i] = NULL;
	}

	flush_workqueue(imgsys_dev->mdpcb_wq);
	destroy_workqueue(imgsys_dev->mdpcb_wq);
	imgsys_dev->mdpcb_wq = NULL;

	flush_workqueue(imgsys_dev->composer_wq);
	destroy_workqueue(imgsys_dev->composer_wq);
	imgsys_dev->composer_wq = NULL;

	flush_workqueue(imgsys_dev->enqueue_wq);
	destroy_workqueue(imgsys_dev->enqueue_wq);
	imgsys_dev->enqueue_wq = NULL;

	atomic_set(&imgsys_dev->num_composing, 0);
	atomic_set(&imgsys_dev->imgsys_enqueue_cnt, 0);
	atomic_set(&imgsys_dev->imgsys_user_cnt, 0);

	mutex_destroy(&imgsys_dev->imgsys_users.user_lock);
}

static bool mtk_imgsys_idle(struct device *dev)
{
	int num;
	bool idle = true;
	struct mtk_imgsys_dev *imgsys_dev = dev_get_drvdata(dev);
	struct mtk_imgsys_pipe *pipe = &imgsys_dev->imgsys_pipe[0];

	num = atomic_read(&imgsys_dev->num_composing);
	if (pipe->streaming || num)
		idle = false;

	return idle;
}

int __maybe_unused mtk_imgsys_pm_suspend(struct device *dev)
{
	struct mtk_imgsys_dev *imgsys_dev = dev_get_drvdata(dev);
	int ret, num;
	bool idle = true;

	ret = wait_event_timeout
		(imgsys_dev->flushing_waitq, (idle = mtk_imgsys_idle(dev)),
		 msecs_to_jiffies(1000 / 30 * DIP_COMPOSING_MAX_NUM * 3));
	num = atomic_read(&imgsys_dev->num_composing);
	if (!ret && !idle) {
		dev_info(dev, "%s: flushing SCP job timeout, num(%d), idle(%d)\n",
			__func__, num, idle);

		return NOTIFY_BAD;
	}
#ifdef NEED_PM

	if (pm_runtime_suspended(dev)) {
		dev_info(dev, "%s: pm_runtime_suspended is true, no action\n",
			__func__);
		return NOTIFY_DONE;
	}

	ret = pm_runtime_put_sync(dev);
	if (ret) {
		dev_info(dev, "%s: pm_runtime_put_sync failed:(%d)\n",
			__func__, ret);
		return NOTIFY_BAD;
	}
#endif
	return NOTIFY_DONE;
}

int __maybe_unused mtk_imgsys_pm_resume(struct device *dev)
{
#ifdef NEED_PM
	int ret;

	if (pm_runtime_suspended(dev)) {
		dev_info(dev, "%s: pm_runtime_suspended is true, no action\n",
			__func__);
		return NOTIFY_DONE;
	}

	ret = pm_runtime_get_sync(dev);
	if (ret) {
		dev_info(dev, "%s: pm_runtime_get_sync failed:(%d)\n",
			__func__, ret);
		return NOTIFY_BAD;
	}
#endif
	return NOTIFY_DONE;
}


#if IS_ENABLED(CONFIG_PM)
static int imgsys_pm_event(struct notifier_block *notifier,
			unsigned long pm_event, void *unused)
{
	struct timespec64 ts;
	struct rtc_time tm;
	int ret = NOTIFY_OK;
	ktime_get_ts64(&ts);
	rtc_time64_to_tm(ts.tv_sec, &tm);

	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:
		ret = NOTIFY_DONE;
		break;
	case PM_RESTORE_PREPARE:
		ret = NOTIFY_DONE;
		break;
	case PM_POST_HIBERNATION:
		ret = NOTIFY_DONE;
		break;
	case PM_SUSPEND_PREPARE: /*enter suspend*/
		ret = mtk_imgsys_pm_suspend(imgsys_pm_dev);
		break;
	case PM_POST_SUSPEND:    /*after resume*/
		ret = mtk_imgsys_pm_resume(imgsys_pm_dev);
		break;
	}

	return ret;
}

static struct notifier_block imgsys_notifier_block = {
	.notifier_call = imgsys_pm_event,
	.priority = 0,
};
#endif

static void mtk_imgsys_get_ccu_phandle(struct mtk_imgsys_dev *imgsys_dev)
{
	struct device *dev = imgsys_dev->dev;
	struct device_node *node;
	phandle rproc_ccu_phandle;
	int ret;

	node = of_find_compatible_node(NULL, NULL, "mediatek,camera_imgsys_ccu");
	if (node == NULL) {
		dev_info(dev, "of_find mediatek,camera_imgsys_ccu fail\n");
		goto out;
	}

	ret = of_property_read_u32(node, "mediatek,ccu_rproc",
				   &rproc_ccu_phandle);
	if (ret) {
		dev_info(dev, "fail to get rproc_ccu_phandle:%d\n", ret);
		goto out;
	}

	imgsys_dev->rproc_ccu_handle = rproc_get_by_phandle(rproc_ccu_phandle);
	if (imgsys_dev->rproc_ccu_handle == NULL) {
		dev_info(imgsys_dev->dev, "Get ccu handle fail\n");
		goto out;
	}

out:
	return;
}

int mtk_imgsys_probe(struct platform_device *pdev)
{
	struct mtk_imgsys_dev *imgsys_dev;
	struct device **larb_devs;
	struct device_node *dev_node;
	struct platform_device *pdev_temp;
	const struct cust_data *data;
#if MTK_CM4_SUPPORT
	phandle rproc_phandle;
#endif
	struct device_link *link;
	int larbs_num, i;
	int ret;
	const char *coherent_status = NULL;

	imgsys_dev = devm_kzalloc(&pdev->dev, sizeof(*imgsys_dev), GFP_KERNEL);
	if (!imgsys_dev)
		return -ENOMEM;

	data = of_device_get_match_data(&pdev->dev);

	if (data == NULL)
		return -EINVAL;

	init_imgsys_pipeline(data);

	mtk_imgsys_get_ccu_phandle(imgsys_dev);

	imgsys_dev->cust_pipes = data->pipe_settings;
	imgsys_dev->modules = data->imgsys_modules;

	imgsys_dev->dev = &pdev->dev;
	imgsys_dev->imgsys_resource = &pdev->resource[0];
	dev_set_drvdata(&pdev->dev, imgsys_dev);
	imgsys_dev->imgsys_stream_cnt = 0;
	imgsys_dev->clks = data->clks;
	imgsys_dev->num_clks = data->clk_num;
	imgsys_dev->num_mods = data->mod_num;
	imgsys_dev->modules_num = data->imgsys_modules_num;
	imgsys_dev->dump = data->dump;
	imgsys_dev->dma_ports = data->imgsys_ports;
	imgsys_dev->dma_ports_num = data->imgsys_ports_num;

	/* register iommu TF cb */
	dev_info(imgsys_dev->dev, "%s: dma_ports_num(0x%x)\n",
		__func__, imgsys_dev->dma_ports_num);
	for (i = 0; i < imgsys_dev->dma_ports_num; i++) {
		if (imgsys_dev->dma_ports[i].fn == NULL)
			continue;
		mtk_iommu_register_fault_callback(imgsys_dev->dma_ports[i].port,
			(mtk_iommu_fault_callback_t)imgsys_dev->dma_ports[i].fn,
			NULL, false);
		dev_info(imgsys_dev->dev,
		"%s: [%d] register iommu cb(0x%x)\n",
		__func__, i, imgsys_dev->dma_ports[i].port);
	}

#ifdef CLK_READY
	ret = devm_clk_bulk_get(&pdev->dev, imgsys_dev->num_clks,
							imgsys_dev->clks);
	if (ret) {
		dev_info(&pdev->dev, "Failed to get clks:%d\n",
			ret);
	}
#endif
	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(34)))
		dev_info(&pdev->dev, "%s:No DMA available\n", __func__);

	if (!pdev->dev.dma_parms) {
		pdev->dev.dma_parms =
			devm_kzalloc(imgsys_dev->dev, sizeof(*pdev->dev.dma_parms), GFP_KERNEL);
	}
	if (pdev->dev.dma_parms) {
		ret = dma_set_max_seg_size(imgsys_dev->dev, (unsigned int)DMA_BIT_MASK(34));
		if (ret)
			dev_info(imgsys_dev->dev, "Failed to set DMA segment size\n");
	}

#if MTK_CM4_SUPPORT
	imgsys_dev->scp_pdev = scp_get_pdev(pdev);
	if (!imgsys_dev->scp_pdev) {
		dev_info(imgsys_dev->dev,
			"%s: failed to get scp device\n",
			__func__);
		return -EINVAL;
	}

	if (of_property_read_u32(imgsys_dev->dev->of_node, "mediatek,scp",
				 &rproc_phandle)) {
		dev_info(imgsys_dev->dev,
			"%s: could not get scp device\n",
			__func__);
		return -EINVAL;
	}

	imgsys_dev->rproc_handle = rproc_get_by_phandle(rproc_phandle);
	if (!imgsys_dev->rproc_handle) {
		dev_info(imgsys_dev->dev,
			"%s: could not get DIP's rproc_handle\n",
			__func__);
		return -EINVAL;
	}
#else
	imgsys_dev->scp_pdev = mtk_hcp_get_plat_device(pdev);
	if (!imgsys_dev->scp_pdev) {
		dev_info(imgsys_dev->dev,
			"%s: failed to get hcp device\n",
			__func__);
		return -EINVAL;
	}
	imgsys_dev->imgcmdq_pdev = mtk_imgsys_cmdq_get_plat_dev(pdev);
	if (!imgsys_dev->imgcmdq_pdev) {
		dev_info(imgsys_dev->dev,
			"%s: failed to get imgsys cmdq device\n",
			__func__);
		return -EINVAL;
	}
	imgsys_dev->smmu_dev = mtk_smmu_get_shared_device(&pdev->dev);
	if (!imgsys_dev->smmu_dev) {
		dev_info(imgsys_dev->dev,
			"%s: failed to get imgsys smmu device\n",
			__func__);
		return -EINVAL;
	}

	dev_node = of_find_node_by_name(NULL, "imgsys-acp");
	pdev_temp = of_find_device_by_node(dev_node);
	ret = of_property_read_string(dev_node, "mediatek,imgsys-coherent", &coherent_status);
	dev_info(imgsys_dev->dev, "test coherent status(%s)", coherent_status);
	if (!strcmp(coherent_status, "disable")) {
		imgsys_dev->acp_enable = 0;
		dev_info(imgsys_dev->dev, "imgsys-fw: coherence not enable\n");
	}
	imgsys_dev->acp_smmu_dev = mtk_smmu_get_shared_device(&pdev_temp->dev);
	if (!imgsys_dev->acp_smmu_dev) {
		dev_info(imgsys_dev->dev,
			"%s: failed to get imgsys acp smmu share device\n",
			__func__);
		return -EINVAL;
	}
#endif

	larbs_num = of_count_phandle_with_args(pdev->dev.of_node,
						"mediatek,larbs", NULL);
	dev_info(imgsys_dev->dev, "%d larbs to be added", larbs_num);
	if (larbs_num < 0) {
		larbs_num = 0;
		goto bypass_larbs;
	}

	larb_devs = devm_kzalloc(&pdev->dev, sizeof(larb_devs) * larbs_num, GFP_KERNEL);
	if (!larb_devs)
		return -ENOMEM;

	for (i = 0; i < larbs_num; i++) {
		struct device_node *larb_node;
		struct platform_device *larb_pdev;

		larb_node = of_parse_phandle(pdev->dev.of_node, "mediatek,larbs", i);
		if (!larb_node) {
			dev_info(imgsys_dev->dev,
				"%s: %d larb node not found\n", __func__, i);
			continue;
		}

		larb_pdev = of_find_device_by_node(larb_node);
		if (!larb_pdev) {
			of_node_put(larb_node);
			dev_info(imgsys_dev->dev,
				"%s: %d larb device not found\n", __func__, i);
			continue;
		}
		of_node_put(larb_node);

		link = device_link_add(&pdev->dev, &larb_pdev->dev,
				DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
		if (!link)
			dev_info(imgsys_dev->dev, "unable to link SMI LARB idx %d\n", i);

		larb_devs[i] = &larb_pdev->dev;
	}
	imgsys_dev->larbs = larb_devs;
bypass_larbs:

	imgsys_dev->larbs_num = larbs_num;

	atomic_set(&imgsys_dev->imgsys_enqueue_cnt, 0);
	atomic_set(&imgsys_dev->imgsys_user_cnt, 0);
	atomic_set(&imgsys_dev->num_composing, 0);
	mutex_init(&imgsys_dev->hw_op_lock);
	/* Limited by the co-processor side's stack size */
	sema_init(&imgsys_dev->sem, DIP_COMPOSING_MAX_NUM);

	ret = mtk_imgsys_hw_working_buf_pool_init(imgsys_dev);
	if (ret) {
		dev_info(&pdev->dev, "working buffer init failed(%d)\n", ret);
		return ret;
	}

	ret = mtk_imgsys_dev_v4l2_init(imgsys_dev);
	if (ret) {
		dev_info(&pdev->dev, "v4l2 init failed(%d)\n", ret);

		goto err_release_working_buf_pool;
	}

	ret = mtk_imgsys_res_init(pdev, imgsys_dev);
	if (ret) {
		dev_info(imgsys_dev->dev,
			"%s: mtk_imgsys_res_init failed(%d)\n", __func__, ret);

		ret = -EBUSY;
		goto err_release_deinit_v4l2;
	}

	imgsys_cmdq_init(imgsys_dev, 1);

	#if DVFS_QOS_READY
	mtk_imgsys_mmdvfs_init(imgsys_dev);

	mtk_imgsys_mmqos_init(imgsys_dev);
	#endif

	//pm_runtime_set_autosuspend_delay(&pdev->dev, 3000);
	//pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	INIT_LIST_HEAD(&imgsys_dev->imgsys_users.list);
	mutex_init(&imgsys_dev->imgsys_users.user_lock);

	imgsys_pm_dev = &pdev->dev;
#if IS_ENABLED(CONFIG_PM)
	ret = register_pm_notifier(&imgsys_notifier_block);
	if (ret) {
		dev_info(imgsys_dev->dev, "failed to register notifier block.\n");
		return ret;
	}
#endif
	return 0;

err_release_deinit_v4l2:
	mtk_imgsys_dev_v4l2_release(imgsys_dev);
err_release_working_buf_pool:
	mtk_imgsys_hw_working_buf_pool_release(imgsys_dev);
	return ret;
}
EXPORT_SYMBOL(mtk_imgsys_probe);

int mtk_imgsys_remove(struct platform_device *pdev)
{
	struct mtk_imgsys_dev *imgsys_dev = dev_get_drvdata(&pdev->dev);

	mtk_imgsys_res_release(imgsys_dev);
	pm_runtime_disable(&pdev->dev);
	mtk_imgsys_dev_v4l2_release(imgsys_dev);
	mtk_imgsys_hw_working_buf_pool_release(imgsys_dev);
	mutex_destroy(&imgsys_dev->hw_op_lock);
	#if DVFS_QOS_READY
	mtk_imgsys_mmqos_uninit(imgsys_dev);
	mtk_imgsys_mmdvfs_uninit(imgsys_dev);
	#endif
	imgsys_cmdq_release(imgsys_dev);

	return 0;
}
EXPORT_SYMBOL(mtk_imgsys_remove);

void mtk_imgsys_shutdown(struct platform_device *pdev)
{
	struct mtk_imgsys_dev *imgsys_dev = dev_get_drvdata(&pdev->dev);
    struct mtk_imgsys_pipe *pipe = &imgsys_dev->imgsys_pipe[0];

	dev_info(imgsys_dev->dev, "%s shutdown +\n", __func__);
    if (pipe->streaming != 0) {
        mtk_imgsys_hw_streamoff(pipe);
    }
    dev_info(imgsys_dev->dev, "%s shutdown -\n", __func__);
}
EXPORT_SYMBOL(mtk_imgsys_shutdown);

int mtk_imgsys_runtime_suspend(struct device *dev)
{
	struct mtk_imgsys_dev *imgsys_dev = dev_get_drvdata(dev);

#if MTK_CM4_SUPPORT
	rproc_shutdown(imgsys_dev->rproc_handle);
#endif

	clk_bulk_disable_unprepare(imgsys_dev->num_clks,
				   imgsys_dev->clks);

    if (imgsys_dbg_enable())
	dev_dbg(dev, "%s: disabled imgsys clks\n", __func__);

	return 0;
}
EXPORT_SYMBOL(mtk_imgsys_runtime_suspend);

int mtk_imgsys_runtime_resume(struct device *dev)
{
	struct mtk_imgsys_dev *imgsys_dev = dev_get_drvdata(dev);
	int ret;

	ret = clk_bulk_prepare_enable(imgsys_dev->num_clks,
				      imgsys_dev->clks);

    if (imgsys_dbg_enable())
	dev_dbg(dev, "%s: enabled imgsys clks\n", __func__);

#if MTK_CM4_SUPPORT
	ret = rproc_boot(imgsys_dev->rproc_handle);
#endif
	if (ret) {
#if MTK_CM4_SUPPORT
		dev_info(dev, "%s: FW load failed(rproc:%p):%d\n",
			__func__, imgsys_dev->rproc_handle,	ret);
#endif
		dev_info(imgsys_dev->dev,
			"%s: failed to enable dip clks(%d)\n",
			__func__, ret);
		clk_bulk_disable_unprepare(imgsys_dev->num_clks,
					   imgsys_dev->clks);

		return ret;
	}
#if MTK_CM4_SUPPORT
    if (imgsys_dbg_enable())
	dev_dbg(dev, "%s: FW loaded(rproc:%p)\n",
		__func__, imgsys_dev->rproc_handle);
#endif

	return 0;
}
EXPORT_SYMBOL(mtk_imgsys_runtime_resume);
#if 0
static const struct dev_pm_ops mtk_imgsys_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_imgsys_runtime_suspend,
						mtk_imgsys_runtime_resume, NULL)
};

static const struct of_device_id mtk_imgsys_of_match[] = {
	{ .compatible = "mediatek,imgsys", .data = (void *)&imgsys_data},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_imgsys_of_match);

static struct platform_driver mtk_imgsys_driver = {
	.probe   = mtk_imgsys_probe,
	.remove  = mtk_imgsys_remove,
	.shutdown = NULL,
	.driver  = {
		.name = "camera-dip",
		.owner	= THIS_MODULE,
		.pm = &mtk_imgsys_pm_ops,
		.of_match_table = of_match_ptr(mtk_imgsys_of_match),
	}
};

module_platform_driver(mtk_imgsys_driver);
#endif
MODULE_AUTHOR("Frederic Chen <frederic.chen@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_DESCRIPTION("Mediatek DIP driver");

