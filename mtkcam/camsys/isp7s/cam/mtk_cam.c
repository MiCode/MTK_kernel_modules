// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/component.h>
#include <linux/freezer.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <linux/platform_data/mtk_ccd.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include <linux/rpmsg/mtk_ccd_rpmsg.h>
#include <uapi/linux/mtk_ccd_controls.h>
#include <linux/regulator/consumer.h>

#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/kthread.h>
#include <linux/media.h>
#include <linux/jiffies.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mc.h>
#include <media/v4l2-subdev.h>
#include <media/media-entity.h>
#include <uapi/linux/sched/types.h>

#include <mtk_heap.h>

#include "mtk_cam.h"
#include "mtk_cam-plat.h"
#include "mtk_cam-larb.h"
#include "mtk_cam-trace.h"
#include "mtk_cam-timesync.h"
#include "mtk_cam-job.h"
#include "mtk_cam-fmt_utils.h"
#include "mtk_cam-job_utils.h"

#ifdef CONFIG_VIDEO_MTK_ISP_CAMSYS_DUBUG
static unsigned int debug_ae = 1;
#else
static unsigned int debug_ae;
#endif

module_param(debug_ae, uint, 0644);
MODULE_PARM_DESC(debug_ae, "activates debug ae info");

#define CAM_DEBUG 0

/* Zero out the end of the struct pointed to by p.  Everything after, but
 * not including, the specified field is cleared.
 */
#define CLEAR_AFTER_FIELD(p, field) \
	memset((u8 *)(p) + offsetof(typeof(*(p)), field) + sizeof((p)->field), \
	0, sizeof(*(p)) - offsetof(typeof(*(p)), field) -sizeof((p)->field))

static const struct of_device_id mtk_cam_of_ids[] = {
#ifdef CAMSYS_ISP7S_MT6985
	{.compatible = "mediatek,mt6985-camisp", .data = &mt6985_data},
#endif
#ifdef CAMSYS_ISP7S_MT6886
	{.compatible = "mediatek,mt6886-camisp", .data = &mt6886_data},
#endif
	{}
};

MODULE_DEVICE_TABLE(of, mtk_cam_of_ids);

static struct device *camsys_root_dev;
struct device *mtk_cam_root_dev(void)
{
	return camsys_root_dev;
}

static int mtk_cam_req_try_update_used_ctx(struct media_request *req);

static int set_dev_to_arr(struct device **arr, int num,
			     int idx, struct device *dev)
{
	if (idx < 0 || idx >= num) {
		dev_info(dev, "failed to update engine idx=%d/%d\n", idx, num);
		return -1;
	}

	if (dev)
		arr[idx] = dev;
	return 0;
}

int mtk_cam_set_dev_raw(struct device *dev, int idx,
			 struct device *raw, struct device *yuv)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);
	struct mtk_cam_engines *eng = &cam_dev->engines;

	return set_dev_to_arr(eng->raw_devs, eng->num_raw_devices, idx, raw) ||
		set_dev_to_arr(eng->yuv_devs, eng->num_raw_devices, idx, yuv);
}

int mtk_cam_set_dev_sv(struct device *dev, int idx, struct device *sv)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);
	struct mtk_cam_engines *eng = &cam_dev->engines;

	return set_dev_to_arr(eng->sv_devs, eng->num_camsv_devices, idx, sv);
}

int mtk_cam_set_dev_mraw(struct device *dev, int idx, struct device *mraw)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);
	struct mtk_cam_engines *eng = &cam_dev->engines;

	return set_dev_to_arr(eng->mraw_devs, eng->num_mraw_devices, idx, mraw);
}

static DEFINE_SPINLOCK(larb_probe_lock);
int mtk_cam_set_dev_larb(struct device *dev, struct device *larb)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);
	struct mtk_cam_engines *eng = &cam_dev->engines;
	int i;

	spin_lock(&larb_probe_lock);
	for (i = 0; i < eng->num_larb_devices; ++i) {
		if (!eng->larb_devs[i]) {
			eng->larb_devs[i] = larb;
			break;
		}
	}
	spin_unlock(&larb_probe_lock);
	return (i < eng->num_larb_devices) ? 0 : 1;
}

struct device *mtk_cam_get_larb(struct device *dev, int larb_id)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);
	struct mtk_cam_engines *eng = &cam_dev->engines;
	struct device *larb_dev;
	int i;

	for (i = 0; i < eng->num_larb_devices; ++i) {
		larb_dev = eng->larb_devs[i];
		if (larb_dev && larb_id != mtk_cam_larb_id(larb_dev))
			return larb_dev;
	}

	dev_info(dev, "failed to get larb %d\n", larb_id);
	return NULL;
}

static bool _get_permit_to_queue(struct mtk_cam_device *cam)
{
	return !atomic_cmpxchg(&cam->is_queuing, 0, 1);
}

static void _put_permit_to_queue(struct mtk_cam_device *cam)
{
	atomic_set(&cam->is_queuing, 0);
}

static void append_to_running_list(struct mtk_cam_device *cam,
				   struct mtk_cam_request *req)
{
	int cnt;

	media_request_get(&req->req);

	spin_lock(&cam->running_job_lock);
	cnt = ++cam->running_job_count;
	list_add_tail(&req->list, &cam->running_job_list);
	spin_unlock(&cam->running_job_lock);

	if (CAM_DEBUG || cnt == 1)
		dev_info(cam->dev, "%s: req:%s running cnt %d\n",
			 __func__, req->req.debug_str, cnt);
}

static void remove_from_running_list(struct mtk_cam_device *cam,
				      struct mtk_cam_request *req)
{
	int cnt;

	spin_lock(&cam->running_job_lock);
	cnt = --cam->running_job_count;
	list_del(&req->list);
	spin_unlock(&cam->running_job_lock);

	if (CAM_DEBUG || !cnt)
		dev_info(cam->dev, "%s: req:%s running cnt %d\n",
			 __func__, req->req.debug_str, cnt);

	media_request_put(&req->req);
}

static void update_ctxs_available_jobs(struct mtk_cam_device *cam)
{
	struct mtk_cam_ctx *ctx;
	unsigned long streaming_ctx;
	int i;

	spin_lock(&cam->streaming_lock);
	streaming_ctx = cam->streaming_ctx;
	spin_unlock(&cam->streaming_lock);

	for (i = 0; i < cam->max_stream_num; i++) {
		ctx = &cam->ctxs[i];

		if (!(streaming_ctx & bit_map_bit(MAP_STREAM, i))) {
			ctx->available_jobs = 0;
			continue;
		}

		ctx->available_jobs = mtk_cam_pool_available_cnt(&ctx->job_pool);
	}
}

static bool mtk_cam_test_available_jobs(struct mtk_cam_device *cam,
					unsigned long stream_mask)
{
	struct mtk_cam_ctx *ctx;
	int i;

	for (i = 0; i < cam->max_stream_num; i++) {
		ctx = &cam->ctxs[i];

		if (!(stream_mask & bit_map_bit(MAP_STREAM, i)))
			continue;

		if (!ctx->available_jobs)
			goto fail_to_fetch_job;

		--ctx->available_jobs;
	}

	return true;

fail_to_fetch_job:
	dev_info(cam->dev, "%s: ctx %d has no available job\n",
		 __func__, ctx->stream_id);

	for (i = i - 1; i >= 0; i--) {
		if (!(stream_mask & bit_map_bit(MAP_STREAM, i)))
			continue;
		++ctx->available_jobs;
	}
	return false;
}

static int mtk_cam_get_reqs_to_enque(struct mtk_cam_device *cam,
				     struct list_head *enqueue_list)
{
	struct mtk_cam_request *req, *req_prev;
	int cnt;

	/*
	 * for each req in pending_list
	 *    try update req's ctx
	 *    if all required ctxs are streaming
	 *    if all required ctxs have available job
	 *    => ready to enque
	 */
	update_ctxs_available_jobs(cam);

	cnt = 0;
	spin_lock(&cam->pending_job_lock);
	list_for_each_entry_safe(req, req_prev, &cam->pending_job_list, list) {
		unsigned long used_ctx;

		if (mtk_cam_req_try_update_used_ctx(&req->req))
			continue;

		used_ctx = mtk_cam_req_used_ctx(&req->req);
		if (!mtk_cam_are_all_streaming(cam, used_ctx))
			continue;

		if (!mtk_cam_test_available_jobs(cam, used_ctx))
			continue;

		cnt++;
		list_move_tail(&req->list, enqueue_list);
	}
	spin_unlock(&cam->pending_job_lock);

	return cnt;
}

static int mtk_cam_enque_list(struct mtk_cam_device *cam,
			      struct list_head *enqueue_list)
{
	struct mtk_cam_request *req, *req_prev;

	list_for_each_entry_safe(req, req_prev, enqueue_list, list) {

		append_to_running_list(cam, req);
		mtk_cam_dev_req_enqueue(cam, req);
	}

	return 0;
}

/* Note:
 * this funciton will be called from userspace's context & workqueue
 */
void mtk_cam_dev_req_try_queue(struct mtk_cam_device *cam)
{
	struct list_head enqueue_list;

	if (!_get_permit_to_queue(cam))
		return;

	if (!mtk_cam_is_any_streaming(cam))
		goto put_permit;

	INIT_LIST_HEAD(&enqueue_list);
	if (!mtk_cam_get_reqs_to_enque(cam, &enqueue_list))
		goto put_permit;

	mtk_cam_enque_list(cam, &enqueue_list);

put_permit:
	_put_permit_to_queue(cam);
}

static struct media_request *mtk_cam_req_alloc(struct media_device *mdev)
{
	struct mtk_cam_request *cam_req;

	cam_req = vzalloc(sizeof(*cam_req));

	spin_lock_init(&cam_req->buf_lock);
	frame_sync_init(&cam_req->fs);

	return &cam_req->req;
}

static void mtk_cam_req_free(struct media_request *req)
{
	struct mtk_cam_request *cam_req = to_mtk_cam_req(req);

	vfree(cam_req);
}

static void mtk_cam_req_reset(struct media_request *req)
{
	struct mtk_cam_request *cam_req = to_mtk_cam_req(req);

	INIT_LIST_HEAD(&cam_req->list);

	cam_req->used_ctx = 0;
	cam_req->used_pipe = 0;

	INIT_LIST_HEAD(&cam_req->buf_list);
	/* todo: init fs */
}

/* may fail to get contexts if not stream-on yet */
static int mtk_cam_req_try_update_used_ctx(struct media_request *req)
{
	struct mtk_cam_request *cam_req = to_mtk_cam_req(req);
	struct mtk_cam_device *cam =
		container_of(req->mdev, struct mtk_cam_device, media_dev);
	struct mtk_cam_v4l2_pipelines *ppls = &cam->pipelines;
	unsigned long used_pipe = cam_req->used_pipe;
	unsigned long used_ctx = 0;
	unsigned long submask;
	bool not_streamon_yet = false;
	struct mtk_cam_ctx *ctx;
	int i;

	if (cam_req->used_ctx)
		return 0;

	submask = bit_map_subset_of(MAP_SUBDEV_RAW, used_pipe);
	for (i = 0; i < ppls->num_raw && submask; i++, submask >>= 1) {

		if (!(submask & 0x1))
			continue;

		ctx = mtk_cam_find_ctx(cam, &ppls->raw[i].subdev.entity);
		if (!ctx) {
			/* not all pipes are stream-on */
			not_streamon_yet = true;
			break;
		}

		used_ctx |= bit_map_bit(MAP_STREAM, ctx->stream_id);
	}

	submask = bit_map_subset_of(MAP_SUBDEV_CAMSV, used_pipe);
	for (i = 0; i < ppls->num_camsv && submask; i++, submask >>= 1) {

		if (!(submask & 0x1))
			continue;

		ctx = mtk_cam_find_ctx(cam, &ppls->camsv[i].subdev.entity);
		if (!ctx) {
			/* not all pipes are stream-on */
			not_streamon_yet = true;
			break;
		}

		used_ctx |= bit_map_bit(MAP_STREAM, ctx->stream_id);
	}

	submask = bit_map_subset_of(MAP_SUBDEV_MRAW, used_pipe);
	for (i = 0; i < ppls->num_mraw && submask; i++, submask >>= 1) {

		if (!(submask & 0x1))
			continue;

		ctx = mtk_cam_find_ctx(cam, &ppls->mraw[i].subdev.entity);
		if (!ctx) {
			/* not all pipes are stream-on */
			not_streamon_yet = true;
			break;
		}

		used_ctx |= bit_map_bit(MAP_STREAM, ctx->stream_id);
	}

	dev_dbg(cam->dev, "%s: req %s used_ctx 0x%lx used_pipe 0x%lx\n",
		__func__, req->debug_str, used_ctx, used_pipe);
	cam_req->used_ctx = !not_streamon_yet ? used_ctx : 0;
	return !not_streamon_yet ? 0 : -1;
}

static int mtk_cam_setup_pipe_ctrl(struct media_request *req)
{
	struct mtk_cam_request *cam_req = to_mtk_cam_req(req);
	struct mtk_cam_device *cam =
		container_of(req->mdev, struct mtk_cam_device, media_dev);
	struct mtk_cam_v4l2_pipelines *ppls = &cam->pipelines;
	struct v4l2_ctrl_handler *hdl;
	unsigned long submask;
	int i, ret;

	ret = 0;

	submask = bit_map_subset_of(MAP_SUBDEV_RAW, cam_req->used_pipe);
	for (i = 0; i < ppls->num_raw && submask; i++, submask >>= 1) {
		struct media_request_object *obj;

		if (!(submask & 0x1))
			continue;

		hdl = &ppls->raw[i].ctrl_handler;

		obj = mtk_cam_req_find_ctrl_obj(cam_req, hdl);
		if (!obj)
			continue;

		ret = v4l2_ctrl_request_setup(req, hdl);
		if (ret) {
			dev_info(cam->dev, "failed to setup ctrl of %s\n",
				ppls->raw[i].subdev.entity.name);
			ret = -1;
			break;
		}

		mtk_cam_req_complete_ctrl_obj(obj);
	}

	return ret;
}

static void mtk_cam_reset_rc_data(
	struct mtk_raw_ctrl_data_read_clear *rc_data)
{
	if (!rc_data)
		return;

	memset(rc_data, 0, sizeof(struct mtk_raw_ctrl_data_read_clear));
}

static unsigned int mtk_cam_get_used_pipe(struct mtk_cam_device *cam,
	struct mtk_cam_request *req)
{
	unsigned int used_ctx = req->used_ctx;
	unsigned int i, stream_bit, used_pipe = 0;
	struct mtk_cam_ctx *ctx;

	for (i = 0, stream_bit = bit_map_bit(MAP_STREAM, 0);
			i < cam->max_stream_num && used_ctx;
			++i, stream_bit <<= 1) {
		if (!(used_ctx & stream_bit))
			continue;

		used_ctx &= ~stream_bit;
		ctx = &cam->ctxs[i];
		used_pipe |= ctx->used_pipe;
	}

	return used_pipe;
}

static void mtk_cam_clone_pipe_data_to_req(struct media_request *req)
{
	struct mtk_cam_request *cam_req = to_mtk_cam_req(req);
	struct mtk_cam_device *cam =
		container_of(req->mdev, struct mtk_cam_device, media_dev);
	struct mtk_cam_v4l2_pipelines *ppls = &cam->pipelines;
	unsigned int used_pipe, i;
	unsigned long submask;

	submask = bit_map_subset_of(MAP_SUBDEV_RAW, cam_req->used_pipe);
	for (i = 0; i < ppls->num_raw && submask; i++, submask >>= 1) {
		struct mtk_raw_request_data *data;
		struct mtk_raw_pipeline *raw;
		struct mtk_raw_pad_config *pad;

		if (!(submask & 0x1))
			continue;

		data = &cam_req->raw_data[i];
		raw = &ppls->raw[i];
		pad = &raw->pad_cfg[MTK_RAW_SINK];

		data->ctrl = raw->ctrl_data;
		mtk_cam_reset_rc_data(&raw->ctrl_data.rc_data);

		// TODO(Will): store entire v4l2_mbus_framefmt into mtk_raw_sink_data,
		// if other members of mbus_fmt may be changed
		data->sink.width = pad->mbus_fmt.width;
		data->sink.height = pad->mbus_fmt.height;
		data->sink.mbus_code = pad->mbus_fmt.code;

		/* todo: support tg crop */
		//data->sink.crop = pad->crop;
		data->sink.crop = (struct v4l2_rect) {
			.left = 0,
			.top = 0,
			.width = pad->mbus_fmt.width,
			.height = pad->mbus_fmt.height,
		};
	}

	/* config w/o enque, so still update data for all used pipes of all used ctxs */
	used_pipe = mtk_cam_get_used_pipe(cam, cam_req);

	submask = bit_map_subset_of(MAP_SUBDEV_CAMSV, used_pipe);
	for (i = 0; i < ppls->num_camsv && submask; i++, submask >>= 1) {
		struct mtk_camsv_request_data *data;
		struct mtk_camsv_pipeline *sv;
		struct mtk_camsv_pad_config *pad;

		if (!(submask & 0x1))
			continue;

		data = &cam_req->sv_data[i];
		sv = &ppls->camsv[i];
		pad = &sv->pad_cfg[MTK_CAMSV_SINK];

		data->sink.width = pad->mbus_fmt.width;
		data->sink.height = pad->mbus_fmt.height;
		data->sink.mbus_code = pad->mbus_fmt.code;

		pad = &sv->pad_cfg[MTK_CAMSV_MAIN_STREAM_OUT];
		data->sink.crop = (struct v4l2_rect) {
			.left = 0,
			.top = 0,
			.width = pad->mbus_fmt.width,
			.height = pad->mbus_fmt.height,
		};
	}

	submask = bit_map_subset_of(MAP_SUBDEV_MRAW, used_pipe);
	for (i = 0; i < ppls->num_mraw && submask; i++, submask >>= 1) {
		struct mtk_mraw_request_data *data;
		struct mtk_mraw_pipeline *mraw;
		struct mtk_mraw_pad_config *pad;

		if (!(submask & 0x1))
			continue;

		data = &cam_req->mraw_data[i];
		mraw = &ppls->mraw[i];
		pad = &mraw->pad_cfg[MTK_MRAW_SINK];

		data->sink.width = pad->mbus_fmt.width;
		data->sink.height = pad->mbus_fmt.height;
		data->sink.mbus_code = pad->mbus_fmt.code;

		data->sink.crop = (struct v4l2_rect) {
			.left = 0,
			.top = 0,
			.width = pad->mbus_fmt.width,
			.height = pad->mbus_fmt.height,
		};
	}
}

static void mtk_cam_store_pipe_data_to_ctx(
	struct mtk_cam_ctx *ctx, struct mtk_cam_request *req)
{
	struct mtk_raw_request_data *data;
	unsigned long raw_pipe_idx;

	raw_pipe_idx = get_raw_subdev_idx(ctx->used_pipe);
	if (raw_pipe_idx == -1)
		return;

	data = &req->raw_data[raw_pipe_idx];
	ctx->ctrldata = data->ctrl;
	ctx->ctrldata_stored = true;
}

static void mtk_cam_req_queue(struct media_request *req)
{
	struct mtk_cam_request *cam_req = to_mtk_cam_req(req);
	struct mtk_cam_device *cam =
		container_of(req->mdev, struct mtk_cam_device, media_dev);

	// reset req
	mtk_cam_req_reset(req);

	/* update following in mtk_cam_vb2_buf_queue (.buf_queue)
	 *   add mtk_cam_buffer to req->buf_list
	 *   req->used_pipe
	 */
	vb2_request_queue(req);
	WARN_ON(!cam_req->used_pipe);

	if (mtk_cam_req_try_update_used_ctx(req))
		dev_info(cam->dev,
			 "req %s enqueued before stream-on\n", req->debug_str);

	/* setup ctrl handler */
	WARN_ON(mtk_cam_setup_pipe_ctrl(req));

	mtk_cam_clone_pipe_data_to_req(req);

	spin_lock(&cam->pending_job_lock);
	list_add_tail(&cam_req->list, &cam->pending_job_list);
	spin_unlock(&cam->pending_job_lock);

	if (1 || CAM_DEBUG_ENABLED(V4L2))
		dev_info(cam->dev, "%s: req %s\n", __func__, req->debug_str);

	mtk_cam_dev_req_try_queue(cam);
}

static int mtk_cam_link_notify(struct media_link *link, u32 flags,
			      unsigned int notification)
{
	return v4l2_pipeline_link_notify(link, flags, notification);
}

static const struct media_device_ops mtk_cam_dev_ops = {
	.link_notify = mtk_cam_link_notify,
	.req_alloc = mtk_cam_req_alloc,
	.req_free = mtk_cam_req_free,
	.req_validate = vb2_request_validate,
	.req_queue = mtk_cam_req_queue,
};

#ifdef REMOVE
static int mtk_cam_get_ccu_phandle(struct mtk_cam_device *cam)
{
	struct device *dev = cam->dev;
	struct device_node *node;
	int ret = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,camera_camsys_ccu");
	if (node == NULL) {
		dev_info(dev, "of_find mediatek,camera_camsys_ccu fail\n");
		ret = PTR_ERR(node);
		goto out;
	}

	ret = of_property_read_u32(node, "mediatek,ccu_rproc",
				   &cam->rproc_ccu_phandle);
	if (ret) {
		dev_info(dev, "fail to get ccu rproc_phandle:%d\n", ret);
		ret = -EINVAL;
		goto out;
	}

out:
	return ret;
}
#endif

static int mtk_cam_of_rproc(struct mtk_cam_device *cam)
{
	struct device *dev = cam->dev;
	int ret;

	ret = of_property_read_u32(dev->of_node, "mediatek,ccd",
				   &cam->rproc_phandle);
	if (ret) {
		dev_dbg(dev, "fail to get rproc_phandle:%d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static struct mtk_cam_job *mtk_cam_ctx_fetch_job(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_pool_job pool_job;

	if (WARN_ON(mtk_cam_pool_fetch(&ctx->job_pool,
				       &pool_job, sizeof(pool_job))))
		return NULL;

	pool_job.job_data->pool_job = pool_job;
	return data_to_job(pool_job.job_data);
}

int mtk_cam_dev_req_enqueue(struct mtk_cam_device *cam,
			    struct mtk_cam_request *req)
{
	unsigned int used_ctx = req->used_ctx;
	struct mtk_cam_ctx *ctx;
	struct mtk_cam_job *job;
	struct list_head job_list;
	int job_cnt;
	int i;
	unsigned int stream_bit;

	WARN_ON(!used_ctx);

	INIT_LIST_HEAD(&job_list);
	job_cnt = 0;

	/*
	 * for each context involved:
	 *   pack into job
	 *     fetch ipi/cq buffer
	 *   select hw resources
	 *   ipi - config
	 *   ipi - frame
	 */
	for (i = 0, stream_bit = bit_map_bit(MAP_STREAM, 0);
	     i < cam->max_stream_num && used_ctx;
	     ++i, stream_bit <<= 1) {

		if (!(used_ctx & stream_bit))
			continue;

		used_ctx &= ~stream_bit;

		ctx = &cam->ctxs[i];

		job = mtk_cam_ctx_fetch_job(ctx);
		if (!job) {
			dev_info(cam->dev, "failed to get job from ctx %d\n",
				 ctx->stream_id);
			return -1;
		}

		/* TODO(AY): return buffer in failed case */
		if (mtk_cam_job_pack(job, ctx, req)) {
			mtk_cam_job_return(job);
			return -1;
		}
		mtk_cam_store_pipe_data_to_ctx(ctx, req);

		list_add_tail(&job->list, &job_list);
		++job_cnt;
	}

	frame_sync_set(&req->fs, job_cnt);

	list_for_each_entry(job, &job_list, list) {

		ctx  = job->src_ctx;

		// enque to ctrl ; job will send ipi
		mtk_cam_ctrl_job_enque(&ctx->cam_ctrl, job);
	}

	return 0;
}

void mtk_cam_req_buffer_done(struct mtk_cam_job *job,
			     int pipe_id, int node_id, int buf_state, u64 ts,
			     bool is_proc)
{
	struct mtk_cam_request *req = job->req;
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct device *dev = req->req.mdev->dev;
	struct mtk_cam_video_device *node;
	struct mtk_cam_buffer *buf, *buf_next;
	struct list_head done_list;
	bool is_buf_empty;
	unsigned long ids = 0;

	INIT_LIST_HEAD(&done_list);
	is_buf_empty = false;

	media_request_get(&req->req);

	spin_lock(&req->buf_lock);

	list_for_each_entry_safe(buf, buf_next, &req->buf_list, list) {

		node = mtk_cam_buf_to_vdev(buf);

		if (node->uid.pipe_id != pipe_id)
			continue;

		if (node_id != -1 && node->uid.id != node_id)
			continue;

		/* sv pure raw */
		if (node_id == -1 && is_sv_pure_raw(job) &&
		    node->uid.id == MTKCAM_IPI_RAW_IMGO && is_proc) {
			if (CAM_DEBUG_ENABLED(JOB))
				dev_info(dev,
					 "%s: ctx-%d req:%s(%d) pipe_id:%d, node_id:%d bypass raw imgo\n",
					 __func__, ctx->stream_id,
					 req->req.debug_str,  job->req_seq,
					 pipe_id, node_id);
			continue;
		}

		ids |= BIT(node->uid.id);
		list_move_tail(&buf->list, &done_list);
	}
	is_buf_empty = list_empty(&req->buf_list);

	spin_unlock(&req->buf_lock);

	dev_info(dev, "%s: ctx-%d req:%s(%d) pipe_id:%d, node_id:%d bufs: 0x%lx ts:%lld is_empty %d\n",
		 __func__, ctx->stream_id, req->req.debug_str, job->req_seq,
		 pipe_id, node_id, ids, ts, is_buf_empty);

	if (list_empty(&done_list)) {
		dev_info(dev,
			 "%s: req:%s failed to find pipe_id:%d, node_id:%d, ts:%lld, is_empty %d\n",
			 __func__, req->req.debug_str,
			 pipe_id, node_id, ts, is_buf_empty);
		goto REQ_PUT;
	}

	if (is_buf_empty) {
		/* assume: all ctrls are finished before buffers */

		// remove from running job list
		remove_from_running_list(dev_get_drvdata(dev), req);
	}

	list_for_each_entry_safe(buf, buf_next, &done_list, list) {
		buf->vbb.vb2_buf.timestamp = ts;
		vb2_buffer_done(&buf->vbb.vb2_buf, buf_state);
	}

	if (is_buf_empty)
		mtk_cam_req_dump_incomplete_ctrl(req);
REQ_PUT:
	media_request_put(&req->req);
}

static int get_ipi_id(int stream_id)
{
	int ipi_id = stream_id + CCD_IPI_ISP_MAIN;

	if (WARN_ON(ipi_id < CCD_IPI_ISP_MAIN || ipi_id > CCD_IPI_ISP_TRICAM))
		return -1;

	return ipi_id;
}

#if CCD_READY
int isp_composer_create_session(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtkcam_ipi_event event;
	struct mtkcam_ipi_session_cookie *session = &event.cookie;
	struct mtkcam_ipi_session_param	*session_data = &event.session_data;
	int ret;

	memset(&event, 0, sizeof(event));
	event.cmd_id = CAM_CMD_CREATE_SESSION;
	session->session_id = ctx->stream_id;

	session_data->workbuf.iova = ctx->cq_buffer.daddr;
	session_data->workbuf.ccd_fd = mtk_cam_device_buf_fd(&ctx->cq_buffer);
	session_data->workbuf.size = ctx->cq_buffer.size;

	session_data->msg_buf.iova = 0; /* no need */
	session_data->msg_buf.ccd_fd = mtk_cam_device_buf_fd(&ctx->ipi_buffer);
	session_data->msg_buf.size = ctx->ipi_buffer.size;

	ret = rpmsg_send(ctx->rpmsg_dev->rpdev.ept, &event, sizeof(event));
	dev_info(cam->dev,
		"%s: rpmsg_send id: %d cq_buf(fd:%d,sz:%d) msg_buf(fd:%d,sz%d) ret(%d)\n",
		__func__, event.cmd_id, session_data->workbuf.ccd_fd,
		session_data->workbuf.size, session_data->msg_buf.ccd_fd,
		session_data->msg_buf.size,
		ret);

	return ret;
}

void isp_composer_destroy_session(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtkcam_ipi_event event;
	struct mtkcam_ipi_session_cookie *session = &event.cookie;

	memset(&event, 0, sizeof(event));
	event.cmd_id = CAM_CMD_DESTROY_SESSION;
	session->session_id = ctx->stream_id;
	rpmsg_send(ctx->rpmsg_dev->rpdev.ept, &event, sizeof(event));

	dev_info(cam->dev, "rpmsg_send: DESTROY_SESSION\n");
}

/* forward decl. */
static int isp_composer_handler(struct rpmsg_device *rpdev, void *data,
				int len, void *priv, u32 src);
static int isp_composer_init(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct device *dev = cam->dev;
	struct mtk_ccd *ccd;
	struct rproc_subdev *rpmsg_subdev;
	struct rpmsg_channel_info *msg = &ctx->rpmsg_channel;
	int ipi_id;

	/* Create message client */
	ccd = (struct mtk_ccd *)cam->rproc_handle->priv;
	rpmsg_subdev = ccd->rpmsg_subdev;

	ipi_id = get_ipi_id(ctx->stream_id);
	if (ipi_id < 0)
		return -EINVAL;

	ctx->ipi_id = ipi_id;

	(void)snprintf(msg->name, RPMSG_NAME_SIZE, "mtk-camsys\%d", ctx->stream_id);
	msg->src = ctx->ipi_id;

	ctx->rpmsg_dev = mtk_create_client_msgdevice(rpmsg_subdev, msg);
	if (!ctx->rpmsg_dev)
		return -EINVAL;

	ctx->rpmsg_dev->rpdev.ept = rpmsg_create_ept(&ctx->rpmsg_dev->rpdev,
						     isp_composer_handler,
						     cam, *msg);
	if (IS_ERR(ctx->rpmsg_dev->rpdev.ept)) {
		dev_info(dev, "%s failed rpmsg_create_ept, ctx:%d\n",
			 __func__, ctx->stream_id);
		goto faile_release_msg_dev;
	}

	dev_info(dev, "%s initialized composer of ctx:%d\n",
		 __func__, ctx->stream_id);

	return 0;

faile_release_msg_dev:
	mtk_destroy_client_msgdevice(rpmsg_subdev, &ctx->rpmsg_channel);
	ctx->rpmsg_dev = NULL;
	return -EINVAL;
}

static void isp_composer_uninit(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_ccd *ccd = cam->rproc_handle->priv;

	mtk_destroy_client_msgdevice(ccd->rpmsg_subdev, &ctx->rpmsg_channel);
	ctx->rpmsg_dev = NULL;
}
#endif

__maybe_unused static int isp_composer_handle_ack(struct mtk_cam_device *cam,
				   struct mtkcam_ipi_event *ipi_msg)
{
	/* TODO */
	return 0;
}

static int mtk_cam_power_rproc(struct mtk_cam_device *cam, int on)
{
	int ret = 0;

	if (on) {
		WARN_ON(cam->rproc_handle);

		/* Get the remote proc device of composers */
		cam->rproc_handle = rproc_get_by_phandle(cam->rproc_phandle);
		if (!cam->rproc_handle) {
			dev_info(cam->dev, "fail to get rproc_handle\n");
			return -1;
		}

		/* Power on remote proc device of composers*/
		ret = rproc_boot(cam->rproc_handle);
		if (ret)
			dev_info(cam->dev, "failed to rproc_boot:%d\n", ret);
	} else {
		rproc_shutdown(cam->rproc_handle);
		rproc_put(cam->rproc_handle);
		cam->rproc_handle = NULL;
	}

	return ret;
}

#ifdef REMOVE
static int mtk_cam_power_ctrl_ccu(struct device *dev, int on_off)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);
	struct mtk_camsys_dvfs *dvfs = &cam_dev->dvfs;
	int ret = 0;

	if (on_off) {

		ret = mtk_cam_get_ccu_phandle(cam_dev);
		if (ret)
			goto out;
		cam_dev->rproc_ccu_handle = rproc_get_by_phandle(cam_dev->rproc_ccu_phandle);
		if (cam_dev->rproc_ccu_handle == NULL) {
			dev_info(dev, "Get ccu handle fail\n");
			ret = PTR_ERR(cam_dev->rproc_ccu_handle);
			goto out;
		}

		ret = rproc_boot(cam_dev->rproc_ccu_handle);
		if (ret)
			dev_info(dev, "boot ccu rproc fail\n");

#ifdef DVFS_REGULATOR
		mtk_cam_dvfs_regulator_enable(dvfs);
#endif
		mtk_cam_dvfs_reset_runtime_info(dvfs);
	} else {

#ifdef DVFS_REGULATOR
		mtk_cam_dvfs_regulator_disable(dvfs);
#endif

#ifdef REMOVE
		if (cam_dev->rproc_ccu_handle) {
			rproc_shutdown(cam_dev->rproc_ccu_handle);
			ret = 0;
		} else
			ret = -EINVAL;
#endif
	}
out:
	return ret;
}
#endif

static int mtk_cam_initialize(struct mtk_cam_device *cam)
{
	int ret;

	if (atomic_add_return(1, &cam->initialize_cnt) > 1)
		return 0;

	dev_info(cam->dev, "camsys initialize\n");

	//mtk_cam_power_ctrl_ccu(cam->dev, 1);

	mtk_cam_dvfs_reset_runtime_info(&cam->dvfs);

	if (WARN_ON(pm_runtime_resume_and_get(cam->dev)))
		return -1;

	ret = mtk_cam_power_rproc(cam, 1);
	if (ret)
		return ret; //TODO: goto

	mtk_cam_debug_exp_reset(&cam->dbg);

	return ret;
}

static int mtk_cam_uninitialize(struct mtk_cam_device *cam)
{
	if (!atomic_sub_and_test(1, &cam->initialize_cnt))
		return 0;

	dev_info(cam->dev, "camsys uninitialize\n");

	mtk_cam_power_rproc(cam, 0);
	pm_runtime_put_sync(cam->dev);

	//mtk_cam_power_ctrl_ccu(cam->dev, 0);

	return 0;
}

static int isp_composer_handler(struct rpmsg_device *rpdev, void *data,
				int len, void *priv, u32 src)
{
	struct mtk_cam_device *cam = (struct mtk_cam_device *)priv;
	struct device *dev = cam->dev;
	struct mtkcam_ipi_event *ipi_msg;
	struct mtk_cam_ctx *ctx;

	ipi_msg = (struct mtkcam_ipi_event *)data;
	if (!ipi_msg)
		return -EINVAL;

	if (len < offsetofend(struct mtkcam_ipi_event, ack_data)) {
		dev_dbg(dev, "wrong IPI len:%d\n", len);
		return -EINVAL;
	}

	if (ipi_msg->cmd_id != CAM_CMD_ACK ||
	    (ipi_msg->ack_data.ack_cmd_id != CAM_CMD_FRAME &&
	     ipi_msg->ack_data.ack_cmd_id != CAM_CMD_DESTROY_SESSION))
		return -EINVAL;

	if (ipi_msg->ack_data.ack_cmd_id == CAM_CMD_FRAME) {
		ctx = &cam->ctxs[ipi_msg->cookie.session_id];
		//MTK_CAM_TRACE_BEGIN(BASIC, "ipi_frame_ack:%d",
		//		    ipi_msg->cookie.frame_no);

		if (CAM_DEBUG_ENABLED(JOB))
			dev_info(dev, "ipi_frame_ack: ctx-%d seq-%d\n",
				 ctx->stream_id,
				 (int)seq_from_fh_cookie(ipi_msg->cookie.frame_no));

		mtk_cam_ctrl_job_composed(&ctx->cam_ctrl,
					  ipi_msg->cookie.frame_no,
					  &ipi_msg->ack_data.frame_result,
					  ipi_msg->ack_data.ret);

		//MTK_CAM_TRACE_END(BASIC);
		return 0;

	} else if (ipi_msg->ack_data.ack_cmd_id == CAM_CMD_DESTROY_SESSION) {
		ctx = &cam->ctxs[ipi_msg->cookie.session_id];
		complete(&ctx->session_complete);
		dev_info(dev, "%s:ctx(%d): session destroyed",
			 __func__, ctx->stream_id);
	}

	return 0;
}

static int mtk_cam_in_ctx(struct mtk_cam_ctx *ctx, struct media_entity *entity)
{
	return media_entity_pipeline(entity) == &ctx->pipeline;
}

struct mtk_cam_ctx *mtk_cam_find_ctx(struct mtk_cam_device *cam,
				     struct media_entity *entity)
{
	unsigned int i;

	for (i = 0;  i < cam->max_stream_num; i++) {
		if (media_entity_pipeline(entity) == &cam->ctxs[i].pipeline)
			return &cam->ctxs[i];
	}

	return NULL;
}

static bool _is_streaming_locked(struct mtk_cam_device *cam, int stream_id)
{
	return cam->streaming_ctx & (1 << stream_id);
}

static void _update_streaming_locked(struct mtk_cam_device *cam, int stream_id,
				     bool streaming)
{
	if (WARN_ON(_is_streaming_locked(cam, stream_id) == streaming))
		pr_info("streaming_ctx %x: stread_id %d, streaming %d\n",
			cam->streaming_ctx, stream_id, streaming);

	if (streaming)
		cam->streaming_ctx |= 1 << stream_id;
	else
		cam->streaming_ctx &= ~(1 << stream_id);
}

static struct mtk_cam_ctx *mtk_cam_ctx_get(struct mtk_cam_device *cam)
{
	struct mtk_cam_ctx *ctx = NULL;
	unsigned int i;

	spin_lock(&cam->streaming_lock);
	for (i = 0; i < cam->max_stream_num; i++) {
		if (!(cam->streaming_ctx & (1 << i))) {

			ctx = &cam->ctxs[i];
			_update_streaming_locked(cam, i, 1);
			break;
		}
	}
	spin_unlock(&cam->streaming_lock);

	if (!ctx) {
		dev_info(cam->dev, "%s: failed to get ctx: streaming 0x%x\n",
			 __func__, cam->streaming_ctx);
		return NULL;
	}

	WARN_ON(ctx->stream_id != i);
	return ctx;
}

static void mtk_cam_ctx_put(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;

	spin_lock(&cam->streaming_lock);
	_update_streaming_locked(ctx->cam, ctx->stream_id, 0);
	spin_unlock(&cam->streaming_lock);
}

static int _find_raw_sd_idx(struct mtk_raw_pipeline *raw, int num_raw,
			    struct v4l2_subdev *sd)
{
	int i;

	for (i = 0; i < num_raw; i++, raw++)
		if (sd == &raw->subdev)
			return i;
	return -1;
}

static int _find_sv_sd_idx(struct mtk_camsv_pipeline *sv, int num_sv,
			    struct v4l2_subdev *sd)
{
	int i;

	for (i = 0; i < num_sv; i++, sv++)
		if (sd == &sv->subdev)
			return i;
	return -1;
}

static int _find_mraw_sd_idx(struct mtk_mraw_pipeline *mraw, int num_mraw,
			    struct v4l2_subdev *sd)
{
	int i;

	for (i = 0; i < num_mraw; i++, mraw++)
		if (sd == &mraw->subdev)
			return i;
	return -1;
}

static void mtk_cam_ctx_match_pipe_subdevs(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_v4l2_pipelines *ppls = &ctx->cam->pipelines;
	struct v4l2_subdev **sd = ctx->pipe_subdevs;
	int idx, i;

	/* raw */
	ctx->raw_subdev_idx = -1;

	/* camsv */
	ctx->num_sv_subdevs = 0;
	for (i = 0; i < MAX_SV_PIPES_PER_STREAM; i++)
		ctx->sv_subdev_idx[i] = -1;

	/* mraw */
	ctx->num_mraw_subdevs = 0;
	for (i = 0; i < MAX_MRAW_PIPES_PER_STREAM; i++)
		ctx->mraw_subdev_idx[i] = -1;

	while (*sd) {
		idx = _find_raw_sd_idx(ppls->raw, ppls->num_raw, *sd);
		if (idx >= 0)
			ctx->raw_subdev_idx = idx;

		idx = _find_sv_sd_idx(ppls->camsv, ppls->num_camsv, *sd);
		if (idx >= 0)
			ctx->sv_subdev_idx[ctx->num_sv_subdevs++] = idx;

		idx = _find_mraw_sd_idx(ppls->mraw, ppls->num_mraw, *sd);
		if (idx >= 0)
			ctx->mraw_subdev_idx[ctx->num_mraw_subdevs++] = idx;

		++sd;
	}
}

static int mtk_cam_ctx_pipeline_start(struct mtk_cam_ctx *ctx,
				      struct media_entity *entity)
{
	struct device *dev = ctx->cam->dev;
	struct v4l2_subdev **target_sd;
	struct mtk_cam_video_device *mtk_vdev;
	struct media_pipeline_pad *ppad;
	struct media_entity *entity_walked[32] = {0};
	int last_entity_walked = 0;
	bool walked;
	int i, j, ret;

	ret = media_pipeline_start(&entity->pads[0], &ctx->pipeline);
	if (ret) {
		dev_info(dev,
			 "%s:failed %s in media_pipeline_start:%d\n",
			 __func__, entity->name, ret);
		return ret;
	}

	/* traverse to update used subdevs & number of nodes */

	mutex_lock(&ctx->cam->v4l2_dev.mdev->graph_mutex);

	i = 0;
	list_for_each_entry(ppad, &ctx->pipeline.pads, list) {
		walked = false;
		entity = ppad->pad->entity;

		if (CAM_DEBUG_ENABLED(V4L2))
			dev_info(dev, "linked entity %s, pad idx: %d, func: %d\n",
				 entity->name, ppad->pad->index, entity->function);

		for (j = 0;
		     j <= last_entity_walked && j < ARRAY_SIZE(entity_walked);
		     j++) {
			if (entity_walked[j] == entity) {
				walked = true;
				break;
			}
		}

		if (!walked) {
			last_entity_walked++;
			entity_walked[last_entity_walked] = entity;
		} else {
			/* The owner entity of this pad was already walked */
			continue;
		}

		target_sd = NULL;

		switch (entity->function) {
		case MEDIA_ENT_F_IO_V4L: /* node */
			ctx->enabled_node_cnt++;
			mtk_vdev = media_entity_to_mtk_vdev(entity);
			if (is_raw_subdev(mtk_vdev->uid.pipe_id))
				ctx->has_raw_subdev = 1;

			break;
		case MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER: /* pipeline */
			if (i >= MAX_PIPES_PER_STREAM)
				goto fail_stop_pipeline;

			target_sd = ctx->pipe_subdevs + i;
			i++;
			break;
		case MEDIA_ENT_F_VID_IF_BRIDGE: /* seninf */
			target_sd = &ctx->seninf;
			break;
		case MEDIA_ENT_F_CAM_SENSOR:
			target_sd = &ctx->sensor;
			break;
		default:
			break;
		}

		if (!target_sd)
			continue;

		if (*target_sd) {
			dev_info(dev, "duplicated subdevs: %s!\n", entity->name);
			goto fail_stop_pipeline;
		}

		if (is_media_entity_v4l2_subdev(entity))
			*target_sd = media_entity_to_v4l2_subdev(entity);
	}

	mutex_unlock(&ctx->cam->v4l2_dev.mdev->graph_mutex);

	mtk_cam_ctx_match_pipe_subdevs(ctx);

	return 0;

fail_stop_pipeline:
	mutex_unlock(&ctx->cam->v4l2_dev.mdev->graph_mutex);
	media_pipeline_stop(&entity->pads[0]);
	return -EPIPE;
}

static void mtk_cam_ctx_pipeline_stop(struct mtk_cam_ctx *ctx,
				      struct media_entity *entity)
{
	int i;

	media_pipeline_stop(&entity->pads[0]);

	ctx->sensor = NULL;
	ctx->seninf = NULL;
	for (i = 0; i < MAX_PIPES_PER_STREAM; i++)
		ctx->pipe_subdevs[i] = NULL;
}

static int mtk_cam_ctx_alloc_workers(struct mtk_cam_ctx *ctx)
{
	struct device *dev = ctx->cam->dev;

	kthread_init_worker(&ctx->sensor_worker);
	ctx->sensor_worker_task = kthread_run(kthread_worker_fn,
					      &ctx->sensor_worker,
					      "sensor_worker-%d",
					      ctx->stream_id);
	if (IS_ERR(ctx->sensor_worker_task)) {
		dev_info(dev, "%s:ctx(%d): could not create sensor_worker_task\n",
			 __func__, ctx->stream_id);
		return -1;
	}

	sched_set_fifo(ctx->sensor_worker_task);

	ctx->composer_wq = alloc_ordered_workqueue(dev_name(dev),
						   WQ_HIGHPRI | WQ_FREEZABLE);
	if (!ctx->composer_wq) {
		dev_info(dev, "failed to alloc composer workqueue\n");
		goto fail_uninit_sensor_worker_task;
	}

	ctx->frame_done_wq =
			alloc_ordered_workqueue(dev_name(dev),
						WQ_HIGHPRI | WQ_FREEZABLE);
	if (!ctx->frame_done_wq) {
		dev_info(dev, "failed to alloc frame_done workqueue\n");
		goto fail_uninit_composer_wq;
	}

	ctx->aa_dump_wq =
			alloc_ordered_workqueue(dev_name(dev),
						WQ_HIGHPRI | WQ_FREEZABLE);
	if (!ctx->aa_dump_wq) {
		dev_info(dev, "failed to alloc aa_dump workqueue\n");
		goto fail_uninit_frame_done_wq;
	}

	return 0;

fail_uninit_frame_done_wq:
	destroy_workqueue(ctx->frame_done_wq);
fail_uninit_composer_wq:
	destroy_workqueue(ctx->composer_wq);
fail_uninit_sensor_worker_task:
	kthread_stop(ctx->sensor_worker_task);
	ctx->sensor_worker_task = NULL;

	return -1;
}

static void mtk_cam_ctx_destroy_workers(struct mtk_cam_ctx *ctx)
{
	kthread_stop(ctx->sensor_worker_task);
	ctx->sensor_worker_task = NULL;

	destroy_workqueue(ctx->composer_wq);
	destroy_workqueue(ctx->frame_done_wq);
	destroy_workqueue(ctx->aa_dump_wq);
}

static struct dma_buf *_alloc_dma_buf(const char *name,
				      int size, bool cacheable)
{
	struct dma_buf *dbuf;

	if (cacheable)
		dbuf = mtk_cam_cached_buffer_alloc(size);
	else
		dbuf = mtk_cam_noncached_buffer_alloc(size);

	if (!dbuf) {
		pr_info("%s: failed\n", __func__);
		return NULL;
	}

	mtk_dma_buf_set_name(dbuf, name);
	return dbuf;
}

static int _alloc_pool_by_dbuf(struct mtk_cam_device_buf *buf,
			       struct mtk_cam_pool *pool,
			       struct device *dev,
			       struct dma_buf *dbuf, int total_size, int num)
{
	int ret;

	if (!dbuf)
		return -1;

	ret = mtk_cam_device_buf_init(buf, dbuf, dev, total_size)
		|| mtk_cam_device_buf_vmap(buf);
	if (ret)
		return ret;

	ret = mtk_cam_buffer_pool_alloc(pool, buf, num);
	if (ret)
		goto fail_device_buf_uninit;

	return 0;

fail_device_buf_uninit:
	mtk_cam_device_buf_uninit(buf);
	return ret;
}

static int _alloc_pool(const char *name,
		       struct mtk_cam_device_buf *buf,
		       struct mtk_cam_pool *pool,
		       struct device *dev, int size, int num, bool cacheable)
{
	int total_size = size * num;
	struct dma_buf *dbuf;
	int ret;

	dbuf = _alloc_dma_buf(name, total_size, cacheable);
	if (!dbuf)
		return -1;

	ret = _alloc_pool_by_dbuf(buf, pool, dev, dbuf, total_size, num);
	dma_heap_buffer_free(dbuf);

	return ret;
}

static void _destroy_pool(struct mtk_cam_device_buf *buf,
			  struct mtk_cam_pool *pool)
{
	mtk_cam_pool_destroy(pool);
	mtk_cam_device_buf_uninit(buf);
}

// TODO(Will): get offset from rgb path?
#define MTK_CAM_CACI_TABLE_SIZE (50000)
static int mtk_cam_ctx_alloc_rgbw_caci_buf(struct mtk_cam_ctx *ctx)
{
	int ret = 0;
	struct device *dev_to_attach;
	struct mtk_raw_pipeline *raw_pipe;
	bool is_rgbw;

	if (!ctx->has_raw_subdev)
		return 0;

	raw_pipe = &ctx->cam->pipelines.raw[ctx->raw_subdev_idx];
	is_rgbw =
		raw_pipe->ctrl_data.resource.user_data.raw_res.scen.scen.normal.w_chn_supported;

	if (is_rgbw) {
		struct mtk_cam_device_buf *buf;
		struct dma_buf *dbuf;

		dev_to_attach = ctx->cam->engines.raw_devs[0];
		buf = &ctx->w_caci_buf;

		dbuf = _alloc_dma_buf("CAM_W_CACI_ID", MTK_CAM_CACI_TABLE_SIZE, false);
		ret = (!dbuf) ? -1 : 0;

		ret = ret || mtk_cam_device_buf_init(buf, dbuf, dev_to_attach,
						MTK_CAM_CACI_TABLE_SIZE)
				|| mtk_cam_device_buf_vmap(buf);
	}

	if (!ret)
		memset(ctx->w_caci_buf.vaddr, 0, ctx->w_caci_buf.size);

	return ret;
}

static int mtk_cam_ctx_destroy_rgbw_caci_buf(struct mtk_cam_ctx *ctx)
{
	if (!ctx->has_raw_subdev)
		return 0;

	if (ctx->w_caci_buf.size)
		mtk_cam_device_buf_uninit(&ctx->w_caci_buf);

	return 0;
}

/* for cq working buffers */
#define CQ_BUF_SIZE  0x10000
#define CAM_CQ_BUF_NUM  (JOB_NUM_PER_STREAM * 2) /* 2 for mstream */
#define IPI_FRAME_BUF_SIZE ALIGN(sizeof(struct mtkcam_ipi_frame_param), SZ_1K)
static int mtk_cam_ctx_alloc_pool(struct mtk_cam_ctx *ctx)
{
	struct device *dev_to_attach;
	int ret;

	dev_to_attach = ctx->cam->engines.raw_devs[0];

	ret = _alloc_pool("CAM_MEM_CQ_ID", &ctx->cq_buffer, &ctx->cq_pool,
			  dev_to_attach, CQ_BUF_SIZE, CAM_CQ_BUF_NUM,
			  false);
	if (ret)
		return ret;

	ret = _alloc_pool("CAM_MEM_MSG_ID", &ctx->ipi_buffer, &ctx->ipi_pool,
			  dev_to_attach, IPI_FRAME_BUF_SIZE, CAM_CQ_BUF_NUM,
			  true);
	if (ret)
		goto fail_destroy_cq;

	return 0;

fail_destroy_cq:
	_destroy_pool(&ctx->cq_buffer, &ctx->cq_pool);
	return ret;
}

/* buffer format operation */
struct mtk_cam_buf_fmt_desc *get_fmt_desc(
	struct mtk_cam_driver_buf_desc *buf_desc)
{
	return &buf_desc->fmt_desc[buf_desc->fmt_sel];
}

int set_fmt_select(int sel,
	struct mtk_cam_driver_buf_desc *buf_desc)
{
	if (sel >= MTKCAM_BUF_FMT_TYPE_START &&
		sel < MTKCAM_BUF_FMT_TYPE_CNT)
		buf_desc->fmt_sel = sel;
	else {
		pr_info("%s: error: invalid fmt_sel %d",
				__func__, sel);
		return -1;
	}

	return 0;
}

static int update_from_mbus_bayer(struct mtk_cam_buf_fmt_desc *fmt_desc,
	struct v4l2_mbus_framefmt *mf)
{
	fmt_desc->ipi_fmt = sensor_mbus_to_ipi_fmt(mf->code);
	fmt_desc->pixel_fmt = sensor_mbus_to_pixel_format(mf->code);
	fmt_desc->width = mf->width;
	fmt_desc->height = mf->height;

	return (fmt_desc->ipi_fmt == MTKCAM_IPI_IMG_FMT_UNKNOWN) ? 1 : 0;
}

static int calc_buf_param_bayer(struct mtk_cam_buf_fmt_desc *fmt_desc)
{
	fmt_desc->stride[0] = mtk_cam_dmao_xsize(fmt_desc->width, fmt_desc->ipi_fmt, 4);
	fmt_desc->stride[1] = 0;
	fmt_desc->stride[2] = 0;

	fmt_desc->size = fmt_desc->stride[0] * fmt_desc->height;

	return 0;
}

static int update_from_mbus_ufbc(struct mtk_cam_buf_fmt_desc *fmt_desc,
	struct v4l2_mbus_framefmt *mf)
{
	fmt_desc->ipi_fmt = sensor_mbus_to_ipi_fmt_ufbc(mf->code);
	fmt_desc->pixel_fmt = sensor_mbus_to_pixel_format_ufbc(mf->code);
	fmt_desc->width = mf->width;
	fmt_desc->height = mf->height;

	return (fmt_desc->ipi_fmt == MTKCAM_IPI_IMG_FMT_UNKNOWN) ? 1 : 0;
}

static int calc_buf_param_ufbc(struct mtk_cam_buf_fmt_desc *fmt_desc)
{
	int ret = 0;
	__u32 stride, size;

	ret = get_bayer_ufbc_stride_and_size(
			(u32)fmt_desc->width, (u32)fmt_desc->height,
			mtk_format_info(fmt_desc->pixel_fmt), 0,
			&stride, &size);

	fmt_desc->size = size;
	fmt_desc->stride[0] = stride;
	fmt_desc->stride[1] = fmt_desc->stride[2] = 0;

	return ret;
}

struct buf_fmt_ops {
	int (*update_input_param)(struct mtk_cam_buf_fmt_desc *fmt_desc,
		struct v4l2_mbus_framefmt *mf);
	int (*calc_buf_param)(struct mtk_cam_buf_fmt_desc *fmt_desc);
};

static struct buf_fmt_ops buf_fmt_ops_bayer = {
	.update_input_param = update_from_mbus_bayer,
	.calc_buf_param = calc_buf_param_bayer,
};

static struct buf_fmt_ops buf_fmt_ops_ufbc = {
	.update_input_param = update_from_mbus_ufbc,
	.calc_buf_param = calc_buf_param_ufbc,
};

int update_buf_fmt_desc(struct mtk_cam_driver_buf_desc *desc,
				struct v4l2_mbus_framefmt *mf)
{
	int ret = 0, i = 0;
	struct buf_fmt_ops *ops[MTKCAM_BUF_FMT_TYPE_CNT] = {
		&buf_fmt_ops_bayer,
		&buf_fmt_ops_ufbc
	};

	for (i = MTKCAM_BUF_FMT_TYPE_START; i < MTKCAM_BUF_FMT_TYPE_CNT && !ret; ++i) {
		ret = ret || ops[i]->update_input_param(&desc->fmt_desc[i], mf);
		ret = ret || ops[i]->calc_buf_param(&desc->fmt_desc[i]);

		if (ret) {
			WARN_ON_ONCE(1);
			break;
		}

		desc->max_size = max(desc->max_size, desc->fmt_desc[i].size);
	}

	return ret;
}

static int mtk_cam_ctx_alloc_img_pool(struct mtk_cam_ctx *ctx)
{
	struct device *dev_to_attach;
	struct mtk_cam_driver_buf_desc *desc = &ctx->img_work_buf_desc;
	struct mtk_raw_pipeline *raw_pipe;
	struct mtk_raw_ctrl_data *ctrl_data;
	struct mtk_cam_resource_raw_v2 *raw_res;
	struct v4l2_mbus_framefmt *mf;
	int i;
	int ret = 0;

	if (!ctx->has_raw_subdev)
		goto EXIT;

	dev_to_attach = ctx->cam->engines.raw_devs[0];
	raw_pipe = &ctx->cam->pipelines.raw[ctx->raw_subdev_idx];
	ctrl_data = &raw_pipe->ctrl_data;
	raw_res = &ctrl_data->resource.user_data.raw_res;

	if (raw_res->img_wbuf_num == 0)
		goto EXIT;

	// TODO(Will): for BAYER10_MIPI, use MTK_RAW_MAIN_STREAM_OUT fmt instead;
	mf = &raw_pipe->pad_cfg[MTK_RAW_SINK].mbus_fmt;
	if (update_buf_fmt_desc(desc, mf))
		goto EXIT;

	if (ctrl_data->pre_alloc_mem.num
	    && desc->max_size >= ctrl_data->pre_alloc_mem.bufs[0].length) {
		ret = _alloc_pool_by_dbuf(
			  &ctx->img_work_buffer, &ctx->img_work_pool,
			  dev_to_attach,
			  ctrl_data->pre_alloc_dbuf,
			  ctrl_data->pre_alloc_mem.bufs[0].length,
			  raw_res->img_wbuf_num);
		desc->fd = ctrl_data->pre_alloc_mem.bufs[0].fd;
	} else {
		ret = _alloc_pool(
			  "CAM_MEM_IMG_ID",
			  &ctx->img_work_buffer, &ctx->img_work_pool,
			  dev_to_attach, desc->max_size,
			  raw_res->img_wbuf_num,
			  false);
#ifdef CLOSE_FD_READY
		/* FIXME: close fd */
		desc->fd = mtk_cam_device_buf_fd(&ctx->img_work_buffer);
#else
		desc->fd = -1;
#endif
	}
	desc->has_pool = (ret == 0) ? true : false;

	for (i = MTKCAM_BUF_FMT_TYPE_START; i < MTKCAM_BUF_FMT_TYPE_CNT; ++i) {
		struct mtk_cam_buf_fmt_desc *fmt_desc = &desc->fmt_desc[i];

		dev_info(dev_to_attach, "[%s]: fmt_desc[%d](%d/%d/%d/sz:%zu/ipi:%d)",
				 __func__, i,
				 fmt_desc->width, fmt_desc->height,
				 fmt_desc->stride[0], fmt_desc->size,
				 fmt_desc->ipi_fmt);
	}

	dev_info(dev_to_attach, "[%s]: desc(%zu/0x%x) alloc_mem(%d/%d/%d) img_wbuf_num(%d/%d)\n",
		__func__,
		desc->max_size, mf->code,
		ctrl_data->pre_alloc_mem.bufs[0].fd,
		ctrl_data->pre_alloc_mem.bufs[0].length,
		ctrl_data->pre_alloc_mem.num,
		raw_res->img_wbuf_num,
		raw_res->img_wbuf_size);

	return ret;

EXIT:
	desc->has_pool = false;
	return ret;
}

static void mtk_cam_ctx_destroy_pool(struct mtk_cam_ctx *ctx)
{
	_destroy_pool(&ctx->cq_buffer, &ctx->cq_pool);
	_destroy_pool(&ctx->ipi_buffer, &ctx->ipi_pool);
}

static void mtk_cam_ctx_destroy_img_pool(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_driver_buf_desc *desc = &ctx->img_work_buf_desc;

	if (!ctx->has_raw_subdev)
		return;

	if (desc->has_pool) {
		desc->has_pool = false;
		_destroy_pool(&ctx->img_work_buffer, &ctx->img_work_pool);
	}
}

static int mtk_cam_ctx_prepare_session(struct mtk_cam_ctx *ctx)
{
	int ret;

	ret = isp_composer_init(ctx);
	if (ret)
		return ret;

	init_completion(&ctx->session_complete);
	ret = isp_composer_create_session(ctx);
	if (ret) {
		complete(&ctx->session_complete);
		isp_composer_uninit(ctx);
		return -EBUSY;
	}
	ctx->session_created = 1;
	return ret;
}

static int mtk_cam_ctx_unprepare_session(struct mtk_cam_ctx *ctx)
{
	struct device *dev = ctx->cam->dev;

	if (ctx->session_created) {
		int ret;

		dev_dbg(dev, "%s:ctx(%d): wait for session destroy\n",
			__func__, ctx->stream_id);

		isp_composer_destroy_session(ctx);

		ret = wait_for_completion_timeout(&ctx->session_complete,
						  msecs_to_jiffies(1000));
		if (ret == 0)
			dev_info(dev, "%s:ctx(%d): wait session_complete timeout\n",
				 __func__, ctx->stream_id);

		isp_composer_uninit(ctx);

		ctx->session_created = 0;
	}
	return 0;
}

static bool _ctx_find_subdev(struct mtk_cam_ctx *ctx, struct v4l2_subdev *target)
{
	struct v4l2_subdev **subdev;
	int j;

	for (j = 0, subdev = ctx->pipe_subdevs;
	     j < MAX_PIPES_PER_STREAM; j++, subdev++) {
		if (*subdev == NULL)
			break;

		if (*subdev == target)
			return true;
	}

	return false;
}

static void mtk_cam_update_pipe_used(struct mtk_cam_ctx *ctx,
				     struct mtk_cam_v4l2_pipelines *ppls)
{
	int i;
	unsigned int used_pipe = 0;

	for (i = 0; i < ppls->num_raw; i++)
		if (_ctx_find_subdev(ctx, &ppls->raw[i].subdev))
			used_pipe |= bit_map_bit(MAP_SUBDEV_RAW,
						 ppls->raw[i].id);

	for (i = 0; i < ppls->num_camsv; i++)
		if (_ctx_find_subdev(ctx, &ppls->camsv[i].subdev))
			used_pipe |= bit_map_bit(MAP_SUBDEV_CAMSV,
				ppls->camsv[i].id - MTKCAM_SUBDEV_CAMSV_START);

	for (i = 0; i < ppls->num_mraw; i++)
		if (_ctx_find_subdev(ctx, &ppls->mraw[i].subdev))
			used_pipe |= bit_map_bit(MAP_SUBDEV_MRAW,
				ppls->mraw[i].id - MTKCAM_SUBDEV_MRAW_START);

	ctx->used_pipe = used_pipe;
	dev_info(ctx->cam->dev, "%s: ctx %d pipe_used %x\n",
		 __func__, ctx->stream_id, ctx->used_pipe);
}

static void mtk_cam_ctx_reset(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	unsigned int stream_id = ctx->stream_id;

	/* clear all, except cam & stream_id */
	memset(ctx, 0, sizeof(*ctx));
	ctx->cam = cam;
	ctx->stream_id = stream_id;
}

static void config_pool_job(void *data, int index, void *element)
{
	struct mtk_cam_job_data *job_data = data;
	struct mtk_cam_pool_job *job_element = element;

	job_element->job_data = job_data + index;
}

static int mtk_cam_ctx_init_job_pool(struct mtk_cam_ctx *ctx)
{
	int ret;

	ret = mtk_cam_pool_alloc(&ctx->job_pool,
				 sizeof(struct mtk_cam_pool_job),
				 JOB_NUM_PER_STREAM);
	if (ret) {
		dev_info(ctx->cam->dev, "failed to alloc job pool of ctx %d\n",
			 ctx->stream_id);
		return ret;
	}

	return mtk_cam_pool_config(&ctx->job_pool, config_pool_job, ctx->jobs);
}

struct mtk_cam_ctx *mtk_cam_start_ctx(struct mtk_cam_device *cam,
				      struct mtk_cam_video_device *node)
{
	struct device *dev = cam->dev;
	struct media_entity *entity = &node->vdev.entity;
	struct mtk_cam_ctx *ctx;

	ctx = mtk_cam_find_ctx(cam, entity);
	if (ctx) /* has been started */
		return ctx;

	dev_info(dev, "%s: by node %s\n", __func__, entity->name);

	if (mtk_cam_initialize(cam) < 0)
		return NULL;

	ctx = mtk_cam_ctx_get(cam);
	if (!ctx)
		goto fail_uninitialze;

	mtk_cam_ctx_reset(ctx);

	if (mtk_cam_ctx_pipeline_start(ctx, entity))
		goto fail_ctx_put;

	if (mtk_cam_ctx_alloc_workers(ctx))
		goto fail_pipeline_stop;

	if (mtk_cam_ctx_alloc_pool(ctx))
		goto fail_destroy_workers;

	if (mtk_cam_ctx_alloc_rgbw_caci_buf(ctx))
		goto fail_destroy_pools;

	if (mtk_cam_ctx_alloc_img_pool(ctx))
		goto fail_destroy_caci_buf;

	if (mtk_cam_ctx_prepare_session(ctx))
		goto fail_destroy_img_pool;

	if (mtk_cam_ctx_init_job_pool(ctx))
		goto fail_unprepare_session;

	mtk_cam_update_pipe_used(ctx, &cam->pipelines);
	mtk_cam_ctrl_start(&ctx->cam_ctrl, ctx);

	return ctx;

fail_unprepare_session:
	mtk_cam_ctx_unprepare_session(ctx);
fail_destroy_img_pool:
	mtk_cam_ctx_destroy_img_pool(ctx);
fail_destroy_caci_buf:
	mtk_cam_ctx_destroy_rgbw_caci_buf(ctx);
fail_destroy_pools:
	mtk_cam_ctx_destroy_pool(ctx);
fail_destroy_workers:
	mtk_cam_ctx_destroy_workers(ctx);
fail_pipeline_stop:
	mtk_cam_ctx_pipeline_stop(ctx, entity);
fail_ctx_put:
	mtk_cam_ctx_put(ctx);
fail_uninitialze:
	mtk_cam_uninitialize(cam);

	WARN(1, "%s: failed\n", __func__);
	return NULL;
}

void mtk_cam_stop_ctx(struct mtk_cam_ctx *ctx, struct media_entity *entity)
{
	struct mtk_cam_device *cam = ctx->cam;

	if (!mtk_cam_in_ctx(ctx, entity))
		return;

	dev_info(cam->dev, "%s: by node %s\n", __func__, entity->name);

	mtk_cam_ctx_unprepare_session(ctx);
	mtk_cam_ctx_destroy_pool(ctx);
	mtk_cam_ctx_destroy_img_pool(ctx);
	mtk_cam_ctx_destroy_rgbw_caci_buf(ctx);
	mtk_cam_ctx_destroy_workers(ctx);
	mtk_cam_ctx_pipeline_stop(ctx, entity);
	mtk_cam_pool_destroy(&ctx->job_pool);

	if (ctx->used_engine) {
		mtk_cam_pm_runtime_engines(&cam->engines, ctx->used_engine, 0);
		mtk_cam_release_engine(ctx->cam, ctx->used_engine);
	}

	ctx->used_pipe = 0;
	mtk_cam_ctx_put(ctx);

	mtk_cam_uninitialize(cam);
}

int mtk_cam_ctx_all_nodes_streaming(struct mtk_cam_ctx *ctx)
{
	return ctx->streaming_node_cnt == ctx->enabled_node_cnt;
}

int mtk_cam_ctx_all_nodes_idle(struct mtk_cam_ctx *ctx)
{
	return ctx->streaming_node_cnt == 0;
}

static int ctx_stream_on_pipe_subdev(struct mtk_cam_ctx *ctx, int enable)
{
	struct device *dev = ctx->cam->dev;
	int i, ret;

	for (i = 0; i < MAX_PIPES_PER_STREAM && ctx->pipe_subdevs[i]; i++) {

		ret = v4l2_subdev_call(ctx->pipe_subdevs[i], video,
				       s_stream, enable);
		if (ret) {
			dev_info(dev, "failed to stream_on %s, %d: %d\n",
				 ctx->pipe_subdevs[i]->name, enable, ret);
			goto fail_pipe_off;
		}
	}

	return ret;

fail_pipe_off:

	if (enable)
		for (i = i - 1; i >= 0 && ctx->pipe_subdevs[i]; i--)
			v4l2_subdev_call(ctx->pipe_subdevs[i], video,
					 s_stream, 0);
	return ret;
}

#ifdef NOT_USE_YET
int PipeIDtoTGIDX(int pipe_id)
{
	/* camsv/mraw's cammux id is defined in its own dts */

	switch (pipe_id) {
	case MTKCAM_SUBDEV_RAW_0:
		return 0;
	case MTKCAM_SUBDEV_RAW_1:
		return 1;
	case MTKCAM_SUBDEV_RAW_2:
		return 2;
	default:
		break;
	}
	return -1;
}
#endif

int ctx_stream_on_seninf_sensor(struct mtk_cam_ctx *ctx,
				int enable,
				int seninf_pad, int raw_tg_idx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct v4l2_subdev *seninf = ctx->seninf;
	int ret;
	int i;

	if (!seninf)
		return -1;

	if (!enable) {
		/* use cached exp num */
		seninf_pad = ctx->seninf_pad;
		raw_tg_idx = ctx->raw_tg_idx;
	}

	/* RAW */
	if (raw_tg_idx >= 0 && ctx->hw_raw[0]) {
		int pixel_mode = 3;

		mtk_cam_seninf_set_camtg(seninf, seninf_pad, raw_tg_idx);
		mtk_cam_seninf_set_pixelmode(seninf, seninf_pad, pixel_mode);
	}

	if (ctx->hw_sv) {
		struct mtk_camsv_device *sv_dev;

		sv_dev = dev_get_drvdata(ctx->hw_sv);
		for (i = SVTAG_START; i < SVTAG_END; i++) {
			if (sv_dev->enabled_tags & (1 << i)) {
				unsigned int sv_cammux_id =
					mtk_cam_get_sv_cammux_id(sv_dev, i);

				mtk_cam_seninf_set_camtg_camsv(seninf,
					sv_dev->tag_info[i].seninf_padidx,
					sv_cammux_id, i);
				mtk_cam_seninf_set_pixelmode(seninf,
					sv_dev->tag_info[i].seninf_padidx, 3);
			}
		}
	}

	for (i = 0; i < ctx->num_mraw_subdevs; i++) {
		if (ctx->hw_mraw[i]) {
			struct mtk_mraw_device *mraw_dev =
				dev_get_drvdata(ctx->hw_mraw[i]);
			struct mtk_mraw_pipeline *mraw_pipe =
				&cam->pipelines.mraw[ctx->mraw_subdev_idx[i]];

			mtk_cam_seninf_set_camtg(seninf,
				mraw_pipe->seninf_padidx, mraw_dev->cammux_id);
			mtk_cam_seninf_set_pixelmode(seninf,
				mraw_pipe->seninf_padidx, MRAW_TG_PIXEL_MODE);
		}
	}

	ret = v4l2_subdev_call(ctx->seninf, video, s_stream, enable);
	if (ret) {
		dev_info(cam->dev, "ctx %d failed to stream_on %s %d:%d\n",
			 ctx->stream_id, seninf->name, enable, ret);
		return -EPERM;
	}

	/* cache for stop */
	if (enable) {
		ctx->seninf_pad = seninf_pad;
		ctx->raw_tg_idx = raw_tg_idx;
	}

	return ret;
}

int mtk_cam_ctx_stream_on(struct mtk_cam_ctx *ctx)
{
	int ret;

	dev_info(ctx->cam->dev, "%s: ctx-%d\n", __func__, ctx->stream_id);

	/* if already stream on */
	if (atomic_cmpxchg(&ctx->streaming, 0, 1))
		return 0;

	ret = ctx_stream_on_pipe_subdev(ctx, 1);

	mtk_cam_dev_req_try_queue(ctx->cam);

	//ctx_stream_on_seninf_sensor later
	/* note
	 * 1. collect 1st request info
	 * 2. select HW engines
	 * 3. set cam mux
	 * 4. stream on seninf
	 */

	return 0;
}

int mtk_cam_ctx_stream_off(struct mtk_cam_ctx *ctx)
{
	/* if already stream off */
	if (!atomic_cmpxchg(&ctx->streaming, 1, 0))
		return 0;

	mtk_cam_ctrl_stop(&ctx->cam_ctrl);

	ctx_stream_on_pipe_subdev(ctx, 0);

	/* reset dvfs */
	mtk_cam_dvfs_update(&ctx->cam->dvfs, ctx->stream_id, 0);

	return 0;
}

void mtk_cam_ctx_engine_off(struct mtk_cam_ctx *ctx)
{
	struct mtk_raw_device *raw_dev;
	struct mtk_camsv_device *sv_dev;
	struct mtk_mraw_device *mraw_dev;
	int i;

	dev_info(ctx->cam->dev, "%s: ctx-%d pipe 0x%x engine 0x%x\n",
		 __func__, ctx->stream_id,
		 ctx->used_pipe, ctx->used_engine);

	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);
			stream_on(raw_dev, false);
		}
	}

	if (ctx->hw_sv) {
		sv_dev = dev_get_drvdata(ctx->hw_sv);
		mtk_cam_sv_dev_stream_on(sv_dev, false);
	}

	for (i = 0; i < ctx->num_mraw_subdevs; i++) {
		if (ctx->hw_mraw[i]) {
			mraw_dev = dev_get_drvdata(ctx->hw_mraw[i]);
			mtk_cam_mraw_dev_stream_on(mraw_dev, false);
		}
	}
}

void mtk_cam_ctx_engine_disable_irq(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam;
	struct mtk_raw_device *raw_dev;
	struct mtk_yuv_device *yuv_dev;
	struct mtk_camsv_device *sv_dev;
	struct mtk_mraw_device *mraw_dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);
			disable_irq(raw_dev->irq);

			cam = raw_dev->cam;
			yuv_dev = dev_get_drvdata(
				cam->engines.yuv_devs[raw_dev->id]);
			disable_irq(yuv_dev->irq);
		}
	}

	if (ctx->hw_sv) {
		sv_dev = dev_get_drvdata(ctx->hw_sv);
		for (i = 0; i < ARRAY_SIZE(sv_dev->irq); i++)
			disable_irq(sv_dev->irq[i]);
	}

	for (i = 0; i < ARRAY_SIZE(ctx->hw_mraw); i++) {
		if (ctx->hw_mraw[i]) {
			mraw_dev = dev_get_drvdata(ctx->hw_mraw[i]);
			disable_irq(mraw_dev->irq);
		}
	}
}

void mtk_cam_ctx_engine_reset(struct mtk_cam_ctx *ctx)
{
	struct mtk_raw_device *raw_dev;
	struct mtk_camsv_device *sv_dev;
	struct mtk_mraw_device *mraw_dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);
			reset(raw_dev);
		}
	}

	if (ctx->hw_sv) {
		sv_dev = dev_get_drvdata(ctx->hw_sv);
		sv_reset(sv_dev);
	}

	for (i = 0; i < ARRAY_SIZE(ctx->hw_mraw); i++) {
		if (ctx->hw_mraw[i]) {
			mraw_dev = dev_get_drvdata(ctx->hw_mraw[i]);
			mraw_reset(mraw_dev);
		}
	}
}

int mtk_cam_ctx_send_raw_event(struct mtk_cam_ctx *ctx,
			       struct v4l2_event *event)
{
	struct v4l2_subdev *sd;

	WARN_ON(ctx->raw_subdev_idx < 0);

	sd = &ctx->cam->pipelines.raw[ctx->raw_subdev_idx].subdev;
	v4l2_event_queue(sd->devnode, event);
	return 0;
}

int mtk_cam_ctx_send_sv_event(struct mtk_cam_ctx *ctx,
			       struct v4l2_event *event)
{
	struct v4l2_subdev *sd;

	WARN_ON(ctx->sv_subdev_idx[0] < 0);

	sd = &ctx->cam->pipelines.camsv[ctx->sv_subdev_idx[0]].subdev;
	v4l2_event_queue(sd->devnode, event);
	return 0;
}

int mtk_cam_ctx_queue_sensor_worker(struct mtk_cam_ctx *ctx,
				    struct kthread_work *work)
{
	int ret;

	if (WARN_ON(!ctx))
		return -1;

	ret = kthread_queue_work(&ctx->sensor_worker, work) ? 0 : -1;
	if (ret)
		pr_info("%s: failed\n", __func__);

	return ret;
}

int mtk_cam_ctx_queue_done_wq(struct mtk_cam_ctx *ctx, struct work_struct *work)
{
	int ret;

	if (WARN_ON(!ctx || !ctx->frame_done_wq))
		return -1;

	ret = queue_work(ctx->frame_done_wq, work) ? 0 : -1;
	if (ret)
		pr_info("%s: failed\n", __func__);

	return ret;
}

int mtk_cam_ctx_queue_aa_dump_wq(struct mtk_cam_ctx *ctx, struct work_struct *work)
{
	int ret;

	if (WARN_ON(!ctx || !ctx->aa_dump_wq))
		return -1;

	ret = queue_work(ctx->aa_dump_wq, work) ? 0 : -1;
	if (ret)
		pr_info("%s: failed\n", __func__);

	return ret;
}

/* fetch devs & reset unused elements */
static int fill_devs(struct device **arr_dev, int arr_dev_size,
		     struct device **arr_eng, int arr_eng_size,
		     unsigned long mask)
{
	int i, cnt;

	cnt = 0;
	for (i = 0; i < arr_eng_size && mask; i++) {

		if (!(mask & BIT(i)))
			continue;

		if (cnt >= arr_dev_size)
			return -1;

		arr_dev[cnt++] = arr_eng[i];
	}

	if (cnt < arr_dev_size)
		memset(arr_dev + cnt, 0,
		       (arr_dev_size - cnt) * sizeof(arr_dev[0]));

	return 0;
}

int mtk_cam_ctx_fetch_devices(struct mtk_cam_ctx *ctx, unsigned long engines)
{
	struct mtk_cam_device *cam = ctx->cam;
	int ret;

	dev_info(cam->dev, "%s: engines: 0x%lx\n", __func__, engines);
	ret = fill_devs(ctx->hw_raw, ARRAY_SIZE(ctx->hw_raw),
			cam->engines.raw_devs,
			cam->engines.num_raw_devices,
			bit_map_subset_of(MAP_HW_RAW, engines));

	ret = ret || fill_devs(&ctx->hw_sv, 1,
			       cam->engines.sv_devs,
			       cam->engines.num_camsv_devices,
			       bit_map_subset_of(MAP_HW_CAMSV, engines));

	ret = ret || fill_devs(ctx->hw_mraw, ARRAY_SIZE(ctx->hw_mraw),
			       cam->engines.mraw_devs,
			       cam->engines.num_mraw_devices,
			       bit_map_subset_of(MAP_HW_MRAW, engines));

	if (ret)
		dev_info(cam->dev, "%s: failed. engines = %lx\n",
			 __func__, engines);
	return ret;
}

static int _dynamic_link_seninf_pipe(struct device *dev,
				     struct media_entity *seninf, u16 src_pad,
				     struct media_entity *pipe, u16 sink_pad)
{
	int ret;

	dev_info(dev, "create pad link %s %s\n", seninf->name, pipe->name);
	ret = media_create_pad_link(seninf, src_pad,
				    pipe, sink_pad,
				    MEDIA_LNK_FL_DYNAMIC);
	if (ret) {
		dev_info(dev, "failed to create pad link %s %s err:%d\n",
			 seninf->name, pipe->name, ret);
		return ret;
	}

	return 0;
}

static int config_bridge_pad_links(struct mtk_cam_device *cam,
				   struct v4l2_subdev *seninf)
{
	struct device *dev = cam->dev;
	struct mtk_cam_v4l2_pipelines *ppl = &cam->pipelines;
	struct media_entity *pipe_entity;
	unsigned int i, j;
	int ret;

	/* seninf <-> raw */
	for (i = 0; i < ppl->num_raw; i++) {
		pipe_entity = &ppl->raw[i].subdev.entity;

		ret = _dynamic_link_seninf_pipe(dev,
						&seninf->entity,
						PAD_SRC_RAW0,
						pipe_entity,
						MTK_RAW_SINK);
		if (ret)
			return ret;
	}

	/* seninf <-> camsv */
	for (i = 0; i < ppl->num_camsv; i++) {
		pipe_entity = &ppl->camsv[i].subdev.entity;

		for (j = PAD_SRC_RAW0; j <= PAD_SRC_RAW2; j++) {
			ret = _dynamic_link_seninf_pipe(dev,
							&seninf->entity,
							j,
							pipe_entity,
							MTK_CAMSV_SINK);
			if (ret)
				return ret;
		}

		for (j = PAD_SRC_PDAF0; j <= PAD_SRC_PDAF5; j++) {
			ret = _dynamic_link_seninf_pipe(dev,
							&seninf->entity,
							j,
							pipe_entity,
							MTK_CAMSV_SINK);
			if (ret)
				return ret;
		}
	}

	/* seninf <-> mraw */
	for (i = 0; i < ppl->num_mraw; i++) {
		pipe_entity = &ppl->mraw[i].subdev.entity;

		ret = _dynamic_link_seninf_pipe(dev,
						&seninf->entity,
						PAD_SRC_RAW0,
						pipe_entity,
						MTK_MRAW_SINK);
		if (ret)
			return ret;

		for (j = PAD_SRC_PDAF0; j <= PAD_SRC_PDAF5; j++) {
			ret = _dynamic_link_seninf_pipe(dev,
							&seninf->entity,
							j,
							pipe_entity,
							MTK_MRAW_SINK);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int mtk_cam_create_links(struct mtk_cam_device *cam)
{
	struct v4l2_subdev *sd;
	int i, num;
	int ret;

	num = cam->engines.num_seninf_devices;
	i = 0;
	v4l2_device_for_each_subdev(sd, &cam->v4l2_dev) {
		if (i < num &&
		    sd->entity.function == MEDIA_ENT_F_VID_IF_BRIDGE) {
			ret = config_bridge_pad_links(cam, sd);
			i++;
		}
	}

	return ret;
}

static int mtk_alloc_pipelines(struct mtk_cam_device *cam)
{
	struct mtk_cam_v4l2_pipelines *ppls = &cam->pipelines;
	int ret = 0;

	ppls->num_raw = GET_PLAT_V4L2(raw_pipeline_num);
	ppls->raw = mtk_raw_pipeline_create(cam->dev, ppls->num_raw);
	if (ppls->num_raw && !ppls->raw) {
		dev_info(cam->dev, "%s: failed at alloc raw pipelines: %d\n",
			 __func__, ppls->num_raw);
		ret = -ENOMEM;
	}
	ppls->num_camsv = GET_PLAT_V4L2(camsv_pipeline_num);
	ppls->camsv = mtk_camsv_pipeline_create(cam->dev, ppls->num_camsv);
	if (ppls->num_camsv && !ppls->camsv) {
		dev_info(cam->dev, "%s: failed at alloc camsv pipelines: %d\n",
			 __func__, ppls->num_camsv);
		ret = -ENOMEM;
	}
	ppls->num_mraw = GET_PLAT_V4L2(mraw_pipeline_num);
	ppls->mraw = mtk_mraw_pipeline_create(cam->dev, ppls->num_mraw);
	if (ppls->num_mraw && !ppls->mraw) {
		dev_info(cam->dev, "%s: failed at alloc mraw pipelines: %d\n",
			 __func__, ppls->num_mraw);
		ret = -ENOMEM;
	}
	dev_info(cam->dev, "pipeline num: raw %d camsv %d mraw %d\n",
		 ppls->num_raw,
		 ppls->num_camsv,
		 ppls->num_mraw);
	return ret;
}

static int mtk_cam_master_bind(struct device *dev)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);
	struct media_device *media_dev = &cam_dev->media_dev;
	int ret;

	//dev_info(dev, "%s\n", __func__);

	media_dev->dev = cam_dev->dev;
	strscpy(media_dev->model, dev_driver_string(dev),
		sizeof(media_dev->model));
	(void)snprintf(media_dev->bus_info, sizeof(media_dev->bus_info),
		       "platform:%s", dev_name(dev));
	media_dev->hw_revision = 0;
	media_dev->ops = &mtk_cam_dev_ops;
	media_device_init(media_dev);

	cam_dev->v4l2_dev.mdev = media_dev;
	ret = v4l2_device_register(cam_dev->dev, &cam_dev->v4l2_dev);
	if (ret) {
		dev_dbg(dev, "Failed to register V4L2 device: %d\n", ret);
		goto fail_media_device_cleanup;
	}

	ret = media_device_register(media_dev);
	if (ret) {
		dev_dbg(dev, "Failed to register media device: %d\n",
			ret);
		goto fail_v4l2_device_unreg;
	}

	ret = component_bind_all(dev, cam_dev);
	if (ret) {
		dev_dbg(dev, "Failed to bind all component: %d\n", ret);
		goto fail_media_device_unreg;
	}

	ret = mtk_raw_setup_dependencies(&cam_dev->engines);
	if (ret) {
		dev_dbg(dev, "Failed to mtk_raw_setup_dependencies: %d\n", ret);
		goto fail_unbind_all;
	}

	ret = mtk_alloc_pipelines(cam_dev);
	if (ret)
		dev_info(dev, "failed to update pipeline num\n");

	ret = mtk_raw_register_entities(cam_dev->pipelines.raw,
					cam_dev->pipelines.num_raw,
					&cam_dev->v4l2_dev);
	if (ret) {
		dev_dbg(dev, "Failed to init raw subdevs: %d\n", ret);
		goto fail_remove_dependencies;
	}

	ret = mtk_camsv_register_entities(cam_dev->pipelines.camsv,
					cam_dev->pipelines.num_camsv,
					&cam_dev->v4l2_dev);
	if (ret) {
		dev_dbg(dev, "Failed to init camsv subdevs: %d\n", ret);
		goto fail_unreg_raw_entities;
	}

	ret = mtk_mraw_register_entities(cam_dev->pipelines.mraw,
					cam_dev->pipelines.num_mraw,
					&cam_dev->v4l2_dev);
	if (ret) {
		dev_dbg(dev, "Failed to init mraw subdevs: %d\n", ret);
		goto fail_unreg_camsv_entities;
	}

	mutex_lock(&cam_dev->v4l2_dev.mdev->graph_mutex);
	mtk_cam_create_links(cam_dev);
	/* Expose all subdev's nodes */
	ret = v4l2_device_register_subdev_nodes(&cam_dev->v4l2_dev);
	mutex_unlock(&cam_dev->v4l2_dev.mdev->graph_mutex);
	if (ret) {
		dev_dbg(dev, "Failed to register subdev nodes\n");
		goto fail_unreg_mraw_entities;
	}

	mtk_cam_dvfs_probe(cam_dev->dev,
			   &cam_dev->dvfs, cam_dev->max_stream_num);

	dev_info(dev, "%s success\n", __func__);
	return 0;

fail_unreg_mraw_entities:
	mtk_mraw_unregister_entities(cam_dev->pipelines.mraw,
				    cam_dev->pipelines.num_mraw);

fail_unreg_camsv_entities:
	mtk_camsv_unregister_entities(cam_dev->pipelines.camsv,
				    cam_dev->pipelines.num_camsv);

fail_unreg_raw_entities:
	mtk_raw_unregister_entities(cam_dev->pipelines.raw,
				    cam_dev->pipelines.num_raw);

fail_remove_dependencies:
	/* nothing to do for now */

fail_unbind_all:
	component_unbind_all(dev, cam_dev);

fail_media_device_unreg:
	media_device_unregister(&cam_dev->media_dev);

fail_v4l2_device_unreg:
	v4l2_device_unregister(&cam_dev->v4l2_dev);

fail_media_device_cleanup:
	media_device_cleanup(&cam_dev->media_dev);

	return ret;
}

static void mtk_cam_master_unbind(struct device *dev)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);

	//dev_info(dev, "%s\n", __func__);

	mtk_raw_unregister_entities(cam_dev->pipelines.raw,
				    cam_dev->pipelines.num_raw);

	mtk_camsv_unregister_entities(cam_dev->pipelines.camsv,
				    cam_dev->pipelines.num_camsv);

	mtk_mraw_unregister_entities(cam_dev->pipelines.mraw,
				    cam_dev->pipelines.num_mraw);

	mtk_cam_dvfs_remove(&cam_dev->dvfs);
	component_unbind_all(dev, cam_dev);

	media_device_unregister(&cam_dev->media_dev);
	v4l2_device_unregister(&cam_dev->v4l2_dev);
	media_device_cleanup(&cam_dev->media_dev);
}

static int compare_dev(struct device *dev, void *data)
{
	return dev == (struct device *)data;
}

static void mtk_cam_match_remove(struct device *dev)
{
	(void) dev;
}

static int add_match_by_driver(struct device *dev,
			       struct component_match **match,
			       struct platform_driver *drv)
{
	struct device *p = NULL, *d;
	int num = 0;

	do {
		d = platform_find_device_by_driver(p, &drv->driver);
		put_device(p);
		p = d;
		if (!d)
			break;

		component_match_add(dev, match, compare_dev, d);
		num++;
	} while (true);

	return num;
}

static int mtk_cam_alloc_for_engine(struct device *dev)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);
	struct mtk_cam_engines *eng = &cam_dev->engines;
	struct device **dev_arr;
	int num = eng->num_raw_devices * 2 /* raw + yuv */
		+ eng->num_camsv_devices
		+ eng->num_mraw_devices
		+ eng->num_larb_devices;

	dev_arr = devm_kzalloc(dev, sizeof(*dev) * num, GFP_KERNEL);
	if (!dev_arr)
		return -ENOMEM;

	eng->raw_devs = dev_arr;
	dev_arr += eng->num_raw_devices;

	eng->yuv_devs = dev_arr;
	dev_arr += eng->num_raw_devices;

	eng->sv_devs = dev_arr;
	dev_arr += eng->num_camsv_devices;

	eng->mraw_devs = dev_arr;
	dev_arr += eng->num_mraw_devices;

	eng->larb_devs = dev_arr;
	dev_arr += eng->num_larb_devices;

	return 0;
}

static struct component_match *mtk_cam_match_add(struct device *dev)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);
	struct mtk_cam_engines *eng = &cam_dev->engines;
	struct component_match *match = NULL;
	int yuv_num;

	eng->num_raw_devices =
		add_match_by_driver(dev, &match, &mtk_cam_raw_driver);

	yuv_num = add_match_by_driver(dev, &match, &mtk_cam_yuv_driver);

#ifdef REMOVE
	eng->num_larb_devices =
		add_match_by_driver(dev, &match, &mtk_cam_larb_driver);
#endif

	eng->num_camsv_devices =
		add_match_by_driver(dev, &match, &mtk_cam_sv_driver);

	eng->num_mraw_devices =
		add_match_by_driver(dev, &match, &mtk_cam_mraw_driver);

	eng->num_seninf_devices =
		add_match_by_driver(dev, &match, &seninf_pdrv);

	if (IS_ERR(match) || mtk_cam_alloc_for_engine(dev))
		mtk_cam_match_remove(dev);

	dev_info(dev, "#: raw %d, yuv %d, larb %d, sv %d, seninf %d, mraw %d\n",
		 eng->num_raw_devices, yuv_num,
		 eng->num_larb_devices,
		 eng->num_camsv_devices,
		 eng->num_seninf_devices,
		 eng->num_mraw_devices);

	return match ? match : ERR_PTR(-ENODEV);
}

static const struct component_master_ops mtk_cam_master_ops = {
	.bind = mtk_cam_master_bind,
	.unbind = mtk_cam_master_unbind,
};

static void mtk_cam_ctx_init(struct mtk_cam_ctx *ctx,
			     struct mtk_cam_device *cam,
			     unsigned int stream_id)
{
	ctx->cam = cam;
	ctx->stream_id = stream_id;
}

static int mtk_cam_v4l2_subdev_link_validate(struct v4l2_subdev *sd,
				      struct media_link *link,
				      struct v4l2_subdev_format *source_fmt,
				      struct v4l2_subdev_format *sink_fmt,
				      bool bypass_size_check)
{
	bool pass = true;

	/* The width, height and code must match. */
	if (source_fmt->format.width != sink_fmt->format.width) {
		dev_dbg(sd->entity.graph_obj.mdev->dev,
			"%s: width does not match (source %u, sink %u)\n",
			__func__,
			source_fmt->format.width, sink_fmt->format.width);
		pass = (bypass_size_check) ? true : false;
	}

	if (source_fmt->format.height != sink_fmt->format.height) {
		dev_dbg(sd->entity.graph_obj.mdev->dev,
			"%s: height does not match (source %u, sink %u)\n",
			__func__,
			source_fmt->format.height, sink_fmt->format.height);
		pass = (bypass_size_check) ? true : false;
	}

	if (source_fmt->format.code != sink_fmt->format.code) {
		dev_info(sd->entity.graph_obj.mdev->dev,
			"%s: warn: media bus code does not match (source 0x%8.8x, sink 0x%8.8x)\n",
			__func__,
			source_fmt->format.code, sink_fmt->format.code);
	}

	if (pass)
		return 0;

	dev_dbg(sd->entity.graph_obj.mdev->dev,
		"%s: link was \"%s\":%u -> \"%s\":%u\n", __func__,
		link->source->entity->name, link->source->index,
		link->sink->entity->name, link->sink->index);

	return -EPIPE;
}

int mtk_cam_link_validate(struct v4l2_subdev *sd,
			  struct media_link *link,
			  struct v4l2_subdev_format *source_fmt,
			  struct v4l2_subdev_format *sink_fmt)
{
	struct device *dev;
	int ret = 0;

	dev = sd->v4l2_dev->dev;

	ret = mtk_cam_v4l2_subdev_link_validate(sd, link, source_fmt, sink_fmt, false);
	if (ret)
		dev_info(dev, "%s: link validate failed pad/code/w/h: SRC(%d/0x%x/%d/%d), SINK(%d:0x%x/%d/%d)\n",
			 sd->name, source_fmt->pad, source_fmt->format.code,
			 source_fmt->format.width, source_fmt->format.height,
			 sink_fmt->pad, sink_fmt->format.code,
			 sink_fmt->format.width, sink_fmt->format.height);

	return ret;
}

int mtk_cam_seninf_link_validate(struct v4l2_subdev *sd,
			  struct media_link *link,
			  struct v4l2_subdev_format *source_fmt,
			  struct v4l2_subdev_format *sink_fmt)
{
	struct device *dev;
	int ret = 0;

	dev = sd->v4l2_dev->dev;

	ret = mtk_cam_v4l2_subdev_link_validate(sd, link, source_fmt, sink_fmt, true);
	if (ret)
		dev_info(dev, "%s: link validate failed pad/code/w/h: SRC(%d/0x%x/%d/%d), SINK(%d:0x%x/%d/%d)\n",
			 sd->name, source_fmt->pad, source_fmt->format.code,
			 source_fmt->format.width, source_fmt->format.height,
			 sink_fmt->pad, sink_fmt->format.code,
			 sink_fmt->format.width, sink_fmt->format.height);

	return ret;
}

int mtk_cam_sv_link_validate(struct v4l2_subdev *sd,
			  struct media_link *link,
			  struct v4l2_subdev_format *source_fmt,
			  struct v4l2_subdev_format *sink_fmt)
{
	struct device *dev;
	int ret = 0;

	dev = sd->v4l2_dev->dev;

	ret = mtk_cam_v4l2_subdev_link_validate(sd, link, source_fmt, sink_fmt, true);
	if (ret)
		dev_info(dev, "%s: link validate failed pad/code/w/h: SRC(%d/0x%x/%d/%d), SINK(%d:0x%x/%d/%d)\n",
			 sd->name, source_fmt->pad, source_fmt->format.code,
			 source_fmt->format.width, source_fmt->format.height,
			 sink_fmt->pad, sink_fmt->format.code,
			 sink_fmt->format.width, sink_fmt->format.height);

	return ret;
}

int mtk_cam_mraw_link_validate(struct v4l2_subdev *sd,
			  struct media_link *link,
			  struct v4l2_subdev_format *source_fmt,
			  struct v4l2_subdev_format *sink_fmt)
{
	struct device *dev;
	int ret = 0;

	dev = sd->v4l2_dev->dev;

	ret = mtk_cam_v4l2_subdev_link_validate(sd, link, source_fmt, sink_fmt, true);
	if (ret)
		dev_info(dev, "%s: link validate failed pad/code/w/h: SRC(%d/0x%x/%d/%d), SINK(%d:0x%x/%d/%d)\n",
			 sd->name, source_fmt->pad, source_fmt->format.code,
			 source_fmt->format.width, source_fmt->format.height,
			 sink_fmt->pad, sink_fmt->format.code,
			 sink_fmt->format.width, sink_fmt->format.height);

	return ret;
}

bool mtk_cam_is_any_streaming(struct mtk_cam_device *cam)
{
	bool res;

	spin_lock(&cam->streaming_lock);
	res = !!cam->streaming_ctx;
	spin_unlock(&cam->streaming_lock);

	return res;
}

bool mtk_cam_are_all_streaming(struct mtk_cam_device *cam,
			       unsigned long stream_mask)
{
	unsigned long streaming_ctx;
	bool res;

	spin_lock(&cam->streaming_lock);
	streaming_ctx = cam->streaming_ctx;
	spin_unlock(&cam->streaming_lock);

	res = is_mask_containing(streaming_ctx, stream_mask);

	if (!res)
		dev_info(cam->dev,
			 "%s: ctx not ready: streaming 0x%lx desried 0x%lx\n",
			 __func__, streaming_ctx, stream_mask);
	return res;
}

static unsigned long get_engine_full_set(struct mtk_cam_engines *engines)
{
	unsigned long set = 0;
	int i;

	for (i = 0; i < engines->num_raw_devices; i++)
		set |= bit_map_bit(MAP_HW_RAW, i);

	for (i = 0; i < engines->num_camsv_devices; i++)
		set |= bit_map_bit(MAP_HW_CAMSV, i);

	for (i = 0; i < engines->num_mraw_devices; i++)
		set |= bit_map_bit(MAP_HW_MRAW, i);

	return set;
}

int mtk_cam_get_available_engine(struct mtk_cam_device *cam)
{
	unsigned long occupied;

	spin_lock(&cam->streaming_lock);
	occupied = cam->engines.occupied_engine;
	spin_unlock(&cam->streaming_lock);

	if (!cam->engines.full_set)
		cam->engines.full_set = get_engine_full_set(&cam->engines);

	return cam->engines.full_set & ~occupied;
}

int mtk_cam_update_engine_status(struct mtk_cam_device *cam,
				 unsigned long engine_mask,
				 bool available)
{
	unsigned long err_mask, occupied;

	spin_lock(&cam->streaming_lock);

	occupied = cam->engines.occupied_engine;
	if (available) {
		err_mask = (occupied & engine_mask) ^ engine_mask;
		occupied &= ~engine_mask;
	} else {
		err_mask = occupied & engine_mask;
		occupied |= engine_mask;
	}

	if (!err_mask)
		cam->engines.occupied_engine = occupied;

	spin_unlock(&cam->streaming_lock);

	if (WARN_ON(err_mask)) {
		dev_info(cam->dev, "%s: set %d, engine 0x%lx err 0x%lx\n",
			 __func__, available, engine_mask, err_mask);
		return -1;
	}

	dev_info(cam->dev, "%s: mark engine 0x%lx available %d\n",
		 __func__, engine_mask, available);
	return 0;
}

static int loop_each_engine(struct mtk_cam_engines *eng,
			    unsigned long engine_mask,
			    int (*func)(struct device *dev))
{
	unsigned long submask;
	int i;

	submask = bit_map_subset_of(MAP_HW_RAW, engine_mask);
	for (i = 0; i < eng->num_raw_devices && submask; i++, submask >>= 1) {
		if (!(submask & 0x1))
			continue;
		func(eng->raw_devs[i]);
	}

	submask = bit_map_subset_of(MAP_HW_CAMSV, engine_mask);
	for (i = 0; i < eng->num_camsv_devices && submask; i++, submask >>= 1) {
		if (!(submask & 0x1))
			continue;
		func(eng->sv_devs[i]);
	}

	submask = bit_map_subset_of(MAP_HW_MRAW, engine_mask);
	for (i = 0; i < eng->num_mraw_devices && submask; i++, submask >>= 1) {
		if (!(submask & 0x1))
			continue;
		func(eng->mraw_devs[i]);
	}

	return 0;
}

int mtk_cam_pm_runtime_engines(struct mtk_cam_engines *eng,
			       unsigned long engine_mask, int enable)
{
	if (enable)
		loop_each_engine(eng, engine_mask, pm_runtime_resume_and_get);
	else
		loop_each_engine(eng, engine_mask, pm_runtime_put_sync);

	return 0;
}

void mtk_engine_dump_debug_status(struct mtk_cam_device *cam,
				  unsigned long engines)
{
	struct mtk_raw_device *dev;
	unsigned long subset;
	int i;

	subset = bit_map_subset_of(MAP_HW_RAW, engines);
	for (i = 0; i < cam->engines.num_raw_devices; i++) {

		if (subset & BIT(i)) {
			dev = dev_get_drvdata(cam->engines.raw_devs[i]);

			raw_dump_debug_status(dev);
		}
	}
}

static int register_sub_drivers(struct device *dev)
{
	struct component_match *match = NULL;
	int ret;

#ifdef REMOVE
	ret = platform_driver_register(&mtk_cam_larb_driver);
	if (ret) {
		dev_info(dev, "%s mtk_cam_larb_driver fail\n", __func__);
		goto REGISTER_LARB_FAIL;
	}
#endif

	ret = platform_driver_register(&seninf_pdrv);
	if (ret) {
		dev_info(dev, "%s seninf_pdrv fail\n", __func__);
		goto REGISTER_SENINF_FAIL;
	}

	ret = platform_driver_register(&seninf_core_pdrv);
	if (ret) {
		dev_info(dev, "%s seninf_core_pdrv fail\n", __func__);
		goto REGISTER_SENINF_CORE_FAIL;
	}


	ret = platform_driver_register(&mtk_cam_sv_driver);
	if (ret) {
		dev_info(dev, "%s mtk_cam_sv_driver fail\n", __func__);
		goto REGISTER_CAMSV_FAIL;
	}

	ret = platform_driver_register(&mtk_cam_mraw_driver);
	if (ret) {
		dev_info(dev, "%s mtk_cam_mraw_driver fail\n", __func__);
		goto REGISTER_MRAW_FAIL;
	}

	ret = platform_driver_register(&mtk_cam_raw_driver);
	if (ret) {
		dev_info(dev, "%s mtk_cam_raw_driver fail\n", __func__);
		goto REGISTER_RAW_FAIL;
	}

	ret = platform_driver_register(&mtk_cam_yuv_driver);
	if (ret) {
		dev_info(dev, "%s mtk_cam_raw_driver fail\n", __func__);
		goto REGISTER_YUV_FAIL;
	}

	match = mtk_cam_match_add(dev);
	if (IS_ERR(match)) {
		ret = PTR_ERR(match);
		goto ADD_MATCH_FAIL;
	}

	ret = component_master_add_with_match(dev, &mtk_cam_master_ops, match);
	if (ret < 0)
		goto MASTER_ADD_MATCH_FAIL;

	return 0;

MASTER_ADD_MATCH_FAIL:
	mtk_cam_match_remove(dev);

ADD_MATCH_FAIL:
	platform_driver_unregister(&mtk_cam_yuv_driver);

REGISTER_YUV_FAIL:
	platform_driver_unregister(&mtk_cam_raw_driver);

REGISTER_RAW_FAIL:
	platform_driver_unregister(&mtk_cam_mraw_driver);

REGISTER_MRAW_FAIL:
	platform_driver_unregister(&mtk_cam_sv_driver);

REGISTER_CAMSV_FAIL:
	platform_driver_unregister(&seninf_core_pdrv);

REGISTER_SENINF_CORE_FAIL:
	platform_driver_unregister(&seninf_pdrv);

REGISTER_SENINF_FAIL:
#ifdef REMOVE
	platform_driver_unregister(&mtk_cam_larb_driver);

REGISTER_LARB_FAIL:
#endif
	return ret;
}

static int mtk_cam_probe(struct platform_device *pdev)
{
	struct mtk_cam_device *cam_dev;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;
	unsigned int i;
	const struct camsys_platform_data *platform_data;

	//dev_info(dev, "%s\n", __func__);
	platform_data = of_device_get_match_data(dev);
	if (!platform_data) {
		dev_info(dev, "Error: failed to get match data\n");
		return -ENODEV;
	}
	set_platform_data(platform_data);
	dev_info(dev, "platform = %s\n", platform_data->platform);

	camsys_root_dev = dev;

	/* initialize structure */
	cam_dev = devm_kzalloc(dev, sizeof(*cam_dev), GFP_KERNEL);
	if (!cam_dev)
		return -ENOMEM;

	if (dma_set_mask_and_coherent(dev, DMA_BIT_MASK(34)))
		dev_info(dev, "%s: No suitable DMA available\n", __func__);

	if (!dev->dma_parms) {
		dev->dma_parms =
			devm_kzalloc(dev, sizeof(*dev->dma_parms), GFP_KERNEL);
		if (!dev->dma_parms)
			return -ENOMEM;
	}

	if (dev->dma_parms) {
		ret = dma_set_max_seg_size(dev, UINT_MAX);
		if (ret)
			dev_info(dev, "Failed to set DMA segment size\n");
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	cam_dev->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(cam_dev->base)) {
		dev_dbg(dev, "failed to map register base\n");
		return PTR_ERR(cam_dev->base);
	}

	cam_dev->dev = dev;
	dev_set_drvdata(dev, cam_dev);

	/* FIXME: decide max raw stream num by seninf num */
	cam_dev->max_stream_num = 8; /* TODO: how */
	cam_dev->ctxs = devm_kcalloc(dev, cam_dev->max_stream_num,
				     sizeof(*cam_dev->ctxs), GFP_KERNEL);
	if (!cam_dev->ctxs)
		return -ENOMEM;

	for (i = 0; i < cam_dev->max_stream_num; i++)
		mtk_cam_ctx_init(cam_dev->ctxs + i, cam_dev, i);

	spin_lock_init(&cam_dev->streaming_lock);
	spin_lock_init(&cam_dev->pending_job_lock);
	spin_lock_init(&cam_dev->running_job_lock);
	INIT_LIST_HEAD(&cam_dev->pending_job_list);
	INIT_LIST_HEAD(&cam_dev->running_job_list);

	pm_runtime_enable(dev);

	ret = mtk_cam_of_rproc(cam_dev);
	if (ret)
		goto fail_return;

	ret = register_sub_drivers(dev);
	if (ret) {
		dev_info(dev, "fail to register_sub_drivers\n");
		goto fail_return;
	}

	mtk_cam_debug_init(&cam_dev->dbg, cam_dev);

	return 0;

fail_return:

	return ret;
}

static int mtk_cam_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);

	pm_runtime_disable(dev);

	component_master_del(dev, &mtk_cam_master_ops);
	mtk_cam_match_remove(dev);

	mtk_cam_debug_deinit(&cam_dev->dbg);

	platform_driver_unregister(&mtk_cam_mraw_driver);
	platform_driver_unregister(&mtk_cam_sv_driver);
	platform_driver_unregister(&mtk_cam_raw_driver);
#ifdef REMOVE
	platform_driver_unregister(&mtk_cam_larb_driver);
#endif
	platform_driver_unregister(&seninf_core_pdrv);
	platform_driver_unregister(&seninf_pdrv);

	return 0;
}

static int mtk_cam_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "- %s\n", __func__);

	return 0;
}

static int mtk_cam_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "- %s\n", __func__);

	mtk_cam_timesync_init(true);

	return 0;
}

static const struct dev_pm_ops mtk_cam_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_cam_runtime_suspend, mtk_cam_runtime_resume,
			   NULL)
};

static struct platform_driver mtk_cam_driver = {
	.probe   = mtk_cam_probe,
	.remove  = mtk_cam_remove,
	.driver  = {
		.name  = "mtk-cam",
		.of_match_table = of_match_ptr(mtk_cam_of_ids),
		.pm     = &mtk_cam_pm_ops,
	}
};

static int __init mtk_cam_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_cam_driver);
	return ret;
}

static void __exit mtk_cam_exit(void)
{
	platform_driver_unregister(&mtk_cam_driver);
}

module_init(mtk_cam_init);
module_exit(mtk_cam_exit);

MODULE_DESCRIPTION("Camera ISP driver");
MODULE_LICENSE("GPL");
