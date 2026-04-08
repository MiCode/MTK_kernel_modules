// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2022 MediaTek Inc.

#include <media/videobuf2-core.h>

#include "mtk_cam-request.h"

void mtk_cam_req_reset(struct media_request *req)
{
	struct mtk_cam_request *cam_req = to_mtk_cam_req(req);

	INIT_LIST_HEAD(&cam_req->list);

	strncpy(cam_req->debug_str, req->debug_str,
		sizeof(cam_req->debug_str) - 1);
	cam_req->debug_str[sizeof(cam_req->debug_str) - 1] = '\0';

	cam_req->is_buf_empty = 0;
	cam_req->used_ctx = 0;
	cam_req->used_pipe = 0;

	INIT_LIST_HEAD(&cam_req->buf_list);
}

struct media_request_object *
mtk_cam_req_find_ctrl_obj(struct mtk_cam_request *cam_req, void *ctrl_hdl)
{
	struct media_request *req = &cam_req->req;
	struct media_request_object *obj;
	struct v4l2_ctrl_handler *hdl;
	unsigned long flags;

	/* TODO(AY): store if has_ctrl in req in 1st search for performance */
	/* TODO(AY): should do media_request_object_get before return? */
	/* call flow(?)
	 *    media_reqeust_object_complete(obj);
	 *    media_reqeust_object_put(obj); // unbind & release
	 */

	spin_lock_irqsave(&req->lock, flags);
	list_for_each_entry(obj, &req->objects, list) {
		if (vb2_request_object_is_buffer(obj))
			continue;

		hdl = (struct v4l2_ctrl_handler *)obj->priv;
		if (hdl == ctrl_hdl)
			goto UNLOCK_RETURN;
	}
	hdl = NULL;

UNLOCK_RETURN:
	spin_unlock_irqrestore(&req->lock, flags);
	return hdl ? obj : NULL;
}

struct mtk_cam_buffer *
mtk_cam_req_find_buffer(struct mtk_cam_request *cam_req, int pipe_id, int id)
{
	struct mtk_cam_buffer *buf;
	struct mtk_cam_video_device *node;

	spin_lock(&cam_req->buf_lock);
	list_for_each_entry(buf, &cam_req->buf_list, list) {
		node = mtk_cam_buf_to_vdev(buf);

		if (node->uid.pipe_id == pipe_id &&
		    node->desc.id == id)
			goto UNLOCK_RETURN;
	}
	buf = NULL;

UNLOCK_RETURN:
	spin_unlock(&cam_req->buf_lock);
	return buf;
}

int mtk_cam_req_complete_ctrl_obj(struct media_request_object *obj)
{
	might_sleep();

	if (!obj)
		return 0;

	if (obj->ops)
		obj->ops->unbind(obj);	/* mutex used */

	media_request_object_complete(obj);
	return 0;
}

static int check_req_num_incomplete(struct media_request *req)
{
	unsigned long flags;
	int num_incomplete;

	/* media request internal */
	spin_lock_irqsave(&req->lock, flags);
	num_incomplete = req->num_incomplete_objects;
	spin_unlock_irqrestore(&req->lock, flags);

	if (num_incomplete)
		dev_info(req->mdev->dev, "req:%s num_incomplete: %d\n",
			 req->debug_str, num_incomplete);

	return !num_incomplete;
}

void mtk_cam_req_dump_incomplete_ctrl(struct mtk_cam_request *cam_req)
{
	struct media_request *req = &cam_req->req;
	struct media_request_object *obj;
	unsigned long flags;
	int n;

	if (check_req_num_incomplete(req))
		return;

	n = 0;
	spin_lock_irqsave(&req->lock, flags);
	list_for_each_entry(obj, &req->objects, list) {
		if (vb2_request_object_is_buffer(obj))
			continue;

		/* TODO: find corresponding subdev */
		n++;
	}
	spin_unlock_irqrestore(&req->lock, flags);

	pr_info("%s: error: %d v4l2_ctrl_handler is not completed yet.\n",
		__func__, n);
}

void mtk_cam_req_dump_incomplete_buffer(struct mtk_cam_request *cam_req)
{
	struct media_request *req = &cam_req->req;
	struct device *dev = req->mdev->dev;
	struct mtk_cam_buffer *buf;
	struct mtk_cam_video_device *node;

	if (check_req_num_incomplete(req))
		return;

	spin_lock(&cam_req->buf_lock);
	list_for_each_entry(buf, &cam_req->buf_list, list) {
		node = mtk_cam_buf_to_vdev(buf);

		dev_info(dev, "incomplete buf: %s\n", node->desc.name);
	}
	spin_unlock(&cam_req->buf_lock);
}
