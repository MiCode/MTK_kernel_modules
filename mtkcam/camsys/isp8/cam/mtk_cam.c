// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/component.h>
#include <linux/freezer.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>

#include <linux/platform_data/mtk_ccd.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include <linux/rpmsg/mtk_ccd_rpmsg.h>
#include <uapi/linux/mtk_ccd_controls.h>
#include <linux/regulator/consumer.h>

#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/kthread.h>
#include <linux/kref.h>
#include <linux/media.h>
#include <linux/jiffies.h>

#include <media/videobuf2-v4l2.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mc.h>
#include <media/v4l2-subdev.h>
#include <media/media-entity.h>
#include <uapi/linux/sched/types.h>

#include <soc/mediatek/smi.h>
#include <mtk_heap.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include <slbc_ops.h>

#include "mtk_cam.h"
#include "mtk_cam-plat.h"
#include "mtk_cam-larb.h"
#include "mtk_cam-trace.h"
#include "mtk_cam-timesync.h"
#include "mtk_cam-job.h"
#include "mtk_cam-fmt_utils.h"
#include "mtk_cam-job_utils.h"
#include "mtk_cam-raw_ctrl.h"
#include "mtk_cam-bwr.h"
#include "mtk_cam-hsf.h"
#include "mtk_cam-qof.h"
#include "mtk_cam-qof_regs.h"
#include "mtk_cam-reg_utils.h"
#include "iommu_debug.h"

static unsigned int debug_sensor_meta_dump = 0;
module_param(debug_sensor_meta_dump, uint, 0644);
MODULE_PARM_DESC(debug_sensor_meta_dump, "activates sensor meta dump");

static unsigned int rms_freerun;
module_param(rms_freerun, int, 0644);
MODULE_PARM_DESC(rms_freerun, "rms_freerun");

#define CAM_DEBUG 0
#define ENABLE_CCU

static const struct of_device_id mtk_cam_of_ids[] = {
#ifdef CAMSYS_ISP8_MT6899
		{.compatible = "mediatek,mt6899-camisp", .data = &mt6899_data},
#endif
#ifdef CAMSYS_ISP8_MT6991
		{.compatible = "mediatek,mt6991-camisp", .data = &mt6991_data},
#endif
	{}
};
MODULE_DEVICE_TABLE(of, mtk_cam_of_ids);

static const struct of_device_id mtk_cam_vcore_of_ids[] = {
#ifdef CAMSYS_ISP8_MT6899
		{.compatible = "mediatek,mt6899-camisp-vcore",},
#endif
#ifdef CAMSYS_ISP8_MT6991
		{.compatible = "mediatek,mt6991-camisp-vcore",},
#endif
	{}
};
MODULE_DEVICE_TABLE(of, mtk_cam_vcore_of_ids);

static struct device *camsys_root_dev;
struct device *mtk_cam_root_dev(void)
{
	return camsys_root_dev;
}

static int mtk_cam_req_try_update_used_ctx(struct media_request *req);

#define LTMSGO_BUF_SZ		(130 * 8)
#define LTMSGO_BUF_RESERVE_CNT	2

struct mtk_ltms_buf_pool {
	bool flip;
	struct mtk_cam_device_buf buffer;
	struct mtk_cam_pool pool;
	struct mtk_cam_pool_buffer buf_1;
	struct mtk_cam_pool_buffer buf_2;
};

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
			struct device *raw, struct device *yuv,
			struct device *rms)
{
	struct mtk_cam_device *cam_dev = dev_get_drvdata(dev);
	struct mtk_cam_engines *eng = &cam_dev->engines;

	return set_dev_to_arr(eng->raw_devs, eng->num_raw_devices, idx, raw) ||
		set_dev_to_arr(eng->yuv_devs, eng->num_raw_devices, idx, yuv) ||
		set_dev_to_arr(eng->rms_devs, eng->num_raw_devices, idx, rms);
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

	if (CAM_DEBUG)
		dev_info(cam->dev, "%s: req:%s running cnt %d\n",
			 __func__, req->debug_str, cnt);
}

static void remove_from_running_list(struct mtk_cam_device *cam,
				      struct mtk_cam_request *req)
{
	int cnt;

	spin_lock(&cam->running_job_lock);
	cnt = --cam->running_job_count;
	list_del(&req->list);
	spin_unlock(&cam->running_job_lock);

	if (CAM_DEBUG)
		dev_info(cam->dev, "%s: req:%s running cnt %d\n",
			 __func__, req->debug_str, cnt);

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
static int get_req_type(struct mtk_cam_ctx *ctx,
	struct mtk_cam_request *req)
{
	unsigned long raw_pipe_idx;

	raw_pipe_idx = get_raw_subdev_idx(ctx->used_pipe);
	if (raw_pipe_idx == -1)
		return 0;
	return req->raw_data[raw_pipe_idx].ctrl.req_info.req_type;
}
static unsigned int get_req_sync_id(struct mtk_cam_ctx *ctx,
	struct mtk_cam_request *req)
{
	unsigned long raw_pipe_idx;

	raw_pipe_idx = get_raw_subdev_idx(ctx->used_pipe);
	if (raw_pipe_idx == -1)
		return 0;
	return req->raw_data[raw_pipe_idx].ctrl.req_info.req_sync_id;
}

static bool mtk_cam_test_available_jobs(struct mtk_cam_device *cam,
			struct mtk_cam_request *req, unsigned long stream_mask)
{
	struct mtk_cam_ctx *ctx;
	int i;

	for (i = 0; i < cam->max_stream_num; i++) {
		ctx = &cam->ctxs[i];

		if (!(stream_mask & bit_map_bit(MAP_STREAM, i)))
			continue;

		if (!ctx->available_jobs)
			goto fail_to_fetch_job;

		/* 2nd enque request won't be involved in job create */
		if (get_req_type(ctx, req) == ISP_REQUEST)
			continue;

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

		if (!mtk_cam_test_available_jobs(cam, req, used_ctx))
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
	if (WARN_ON(!cam_req))
		return NULL;

	spin_lock_init(&cam_req->buf_lock);
	frame_sync_init(&cam_req->fs);

	return &cam_req->req;
}

static void mtk_cam_req_free(struct media_request *req)
{
	struct mtk_cam_request *cam_req = to_mtk_cam_req(req);

	vfree(cam_req);
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
		if (!ctx || !atomic_read(&ctx->streaming)) {
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
		if (!ctx || !atomic_read(&ctx->streaming)) {
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
		if (!ctx || !atomic_read(&ctx->streaming)) {
			/* not all pipes are stream-on */
			not_streamon_yet = true;
			break;
		}

		used_ctx |= bit_map_bit(MAP_STREAM, ctx->stream_id);
	}

	if (CAM_DEBUG_ENABLED(V4L2))
		dev_info(cam->dev, "%s: req %s used_ctx 0x%lx used_pipe 0x%lx\n",
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
		pad = mtk_raw_current_sink(raw);

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
	if (raw_pipe_idx >= MTKCAM_SUBDEV_RAW_NUM)
		return;

	data = &req->raw_data[raw_pipe_idx];
	ctx->ctrldata = data->ctrl;

	if (!ctx->ctrldata_stored) {
		// NOTE: update "enable_luma_dump" in request enque stage
		// only for the first time; later, it should be updated
		// after frame done to avoid QOF voter-- skipped due to
		// ctrldata changed in between SOF and frame done
		ctx->enable_luma_dump =
			CAM_DEBUG_ENABLED(AA) && // TODO: remove CAM_DEBUG_AA?
			ctx->ctrldata.resource.user_data.raw_res.luma_debug;
	}

	ctx->ctrldata_stored = true;
}

static void mtk_cam_update_wbuf_fmt_desc(struct mtk_cam_ctx *ctx)
{
	struct mtk_raw_pipeline *pipe = NULL;
	struct mtk_raw_pad_config *sink_pad = NULL;

	if (!ctx || !ctx->has_raw_subdev)
		return;

	pipe = &ctx->cam->pipelines.raw[ctx->raw_subdev_idx];
	sink_pad = &pipe->pad_cfg[MTK_RAW_SINK];

	if (!ctx->configured ||
		(ctx->ipi_config.input.fmt !=
		 sensor_mbus_to_ipi_fmt(sink_pad->mbus_fmt.code)) ||
		 (sink_pad->mbus_fmt.width != ctx->ipi_config.input.in_crop.s.w) ||
		 (sink_pad->mbus_fmt.height != ctx->ipi_config.input.in_crop.s.h))
		update_buf_fmt_desc(&ctx->img_work_buf_desc, &sink_pad->mbus_fmt);
}

static void mtk_cam_req_queue(struct media_request *req)
{
	struct mtk_cam_request *cam_req = to_mtk_cam_req(req);
	struct mtk_cam_device *cam =
		container_of(req->mdev, struct mtk_cam_device, media_dev);

	MTK_CAM_TRACE_FUNC_BEGIN(BASIC);

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

	if (CAM_DEBUG_ENABLED(V4L2))
		dev_info(cam->dev, "%s: req %s\n", __func__, req->debug_str);

	mtk_cam_dev_req_try_queue(cam);

	MTK_CAM_TRACE_END(BASIC);
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
	int req_type;
	int ret = 0;

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

		req_type = get_req_type(ctx, req);
		/* using request type to find job or fetch new job */
		if (req_type == NORMAL_REQUEST)
			job = mtk_cam_ctx_fetch_job(ctx);
		else if (req_type == SENSOR_REQUEST)
			job = mtk_cam_ctx_fetch_job(ctx);
		else if (req_type == ISP_REQUEST)
			job = mtk_cam_ctrl_get_job_by_req_id(&ctx->cam_ctrl,
				get_req_sync_id(ctx, req));
		else
			dev_info(cam->dev, "failed to get valid req_info from ctx %d, type:%d\n",
				 ctx->stream_id, req_type);
		if (!job) {
			dev_info(cam->dev, "failed to get job from ctx %d\n",
				 ctx->stream_id);
			return -1;
		}
		if (req_type == NORMAL_REQUEST ||
			req_type == ISP_REQUEST) {
			mtk_cam_update_wbuf_fmt_desc(ctx);
			if (!ctx->scenario_init)
				WARN_ON(mtk_cam_ctx_init_scenario(ctx));
		}
		/* TODO(AY): return buffer in failed case */
		if (req_type == NORMAL_REQUEST)
			ret = mtk_cam_job_pack(job, ctx, req);
		else if (req_type == SENSOR_REQUEST)
			ret = mtk_cam_sensor_job_pack(job, ctx, req);
		else if (req_type == ISP_REQUEST)
			ret = mtk_cam_isp_job_pack(job, ctx, req);
		else
			dev_info(cam->dev, "failed to get valid req_info from ctx %d, type:%d\n",
				 ctx->stream_id, req_type);

		if (ret) {
			mtk_cam_job_return(job);
			return -1;
		}
		mtk_cam_store_pipe_data_to_ctx(ctx, req);
		ctx->enable_hsf_raw = job->enable_hsf_raw;

		list_add_tail(&job->list, &job_list);
		++job_cnt;
	}

	frame_sync_set(&req->fs, job_cnt);

	list_for_each_entry(job, &job_list, list) {

		ctx = job->src_ctx;
		req_type = get_req_type(ctx, req);
		// enque to ctrl ; job will send ipi
		if (req_type == NORMAL_REQUEST)
			mtk_cam_ctrl_job_enque(&ctx->cam_ctrl, job);
		else if (req_type == SENSOR_REQUEST)
			mtk_cam_ctrl_sensor_job_enque(&ctx->cam_ctrl, job);
		else if (req_type == ISP_REQUEST)
			mtk_cam_ctrl_isp_job_enque(&ctx->cam_ctrl, job);
		else
			dev_info(cam->dev, "failed to get valid req_info from ctx %d, type:%d\n",
				 ctx->stream_id, req_type);
	}

	return 0;
}

/**
 * mtk_cam_req_collect_vb_bufs
 * @return:
 *    1: request has remaining buffers. continue
 *    0: request is completed
 */
int mtk_cam_req_collect_vb_bufs(struct mtk_cam_request *req,
				       int pipe_id, int node_id,
				       bool skip_pure_raw,
				       struct list_head *done_list,
				       unsigned long *ids)
{
	struct mtk_cam_video_device *node;
	struct mtk_cam_buffer *buf, *buf_next;
	bool pure_raw_skipped = false;
	int ret;

	spin_lock(&req->buf_lock);

	list_for_each_entry_safe(buf, buf_next, &req->buf_list, list) {

		node = mtk_cam_buf_to_vdev(buf);

		if (node->uid.pipe_id != pipe_id)
			continue;

		/* check V4L2 node id */
		if (node_id != -1 && node->desc.id != node_id)
			continue;

		/* sv pure raw */
		if (node_id == -1 && skip_pure_raw &&
		    node->desc.id == MTK_RAW_PURE_RAW_OUT) {
			pure_raw_skipped = true;
			continue;
		}
		if (CAM_DEBUG_ENABLED(JOB))
			pr_info("%s: req:%s pipe_id:%d node:%s\n",
			__func__, req->debug_str, pipe_id, node->desc.name);
		list_move_tail(&buf->list, done_list);
		*ids |= BIT(node->uid.id);
	}

	ret = list_empty(&req->buf_list) ? 0 : 1;

	spin_unlock(&req->buf_lock);

	if (CAM_DEBUG_ENABLED(JOB) && pure_raw_skipped)
		pr_info("%s: req:%s pipe_id:%d bypass pure raw node\n",
			__func__, req->debug_str, pipe_id);
	return ret;
}

static void mark_each_buffer_done(struct list_head *done_list,
				  int buf_state, u64 ts_boot, u64 ts_mono)
{
	struct mtk_cam_video_device *node;
	struct mtk_cam_buffer *buf, *buf_next;

	list_for_each_entry_safe(buf, buf_next, done_list, list) {
		if (buf->vbb.vb2_buf.vb2_queue->timestamp_flags &
		    V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC)
			buf->vbb.vb2_buf.timestamp = ts_mono;
		else
			buf->vbb.vb2_buf.timestamp = ts_boot;
		node = mtk_cam_buf_to_vdev(buf);
		atomic_dec(&node->queued_cnt);
		vb2_buffer_done(&buf->vbb.vb2_buf, buf_state);
	}
}
void mtk_cam_sensor_req_buffer_done(struct mtk_cam_job *job,
			     int pipe_id, int node_id, int buf_state,
			     bool is_proc)
{
	struct mtk_cam_request *req = job->req_sensor;
	struct device *dev;
	struct media_request *mreq = &req->req;
	struct list_head done_list_sensor;
	unsigned long ids_sensor;
	bool is_buf_empty_sensor;
	bool buf_error = buf_state == VB2_BUF_STATE_ERROR;

	if (node_id != -1 ||
		pipe_id >= MTKCAM_SUBDEV_RAW_END)
		return;
	if (!mreq)
		return;
	media_request_get(&req->req);
	dev = req->req.mdev->dev;
	INIT_LIST_HEAD(&done_list_sensor);
	ids_sensor = 0;
	is_buf_empty_sensor = !mtk_cam_req_collect_vb_bufs(req,
			pipe_id, node_id,
			is_sv_pure_raw(job) && is_proc,
			&done_list_sensor, &ids_sensor);
	if (CAM_DEBUG_ENABLED(V4L2))
		dev_info(dev, "%s: ctx-%d req:%s(%d) pipe_id:%d node_id:%d bufs:0x%lx ts:%lld%s%s\n",
			 __func__, job->src_ctx->stream_id,
			 req->debug_str, job->req_seq,
			 pipe_id, node_id, ids_sensor,
			 job->timestamp,
			 buf_error ? " error" : "",
			 is_buf_empty_sensor ? " (empty)" : "");
	if (unlikely(list_empty(&done_list_sensor))) {
		dev_info(dev,
			 "%s: req:%s failed to find pipe_id:%d node_id:%d%s\n",
			 __func__, req->debug_str,
			 pipe_id, node_id,
			 is_buf_empty_sensor ? " (empty)" : "");
		goto REQ_PUT;
	}
	if (is_buf_empty_sensor)
		remove_from_running_list(dev_get_drvdata(dev), req);
	mark_each_buffer_done(&done_list_sensor, buf_state,
		job->timestamp, job->timestamp_mono);
	if (is_buf_empty_sensor)
		mtk_cam_req_dump_incomplete_ctrl(req);
	if (CAM_DEBUG_ENABLED(JOB))
		dev_info(dev,
		"%s: req:%s pipe_id:%d sensor req done and completed\n",
		__func__, job->req_sensor->debug_str, pipe_id);
REQ_PUT:
	media_request_put(&req->req);

}
void mtk_cam_req_buffer_done(struct mtk_cam_job *job,
			     int pipe_id, int node_id, int buf_state,
			     bool is_proc)
{
	struct mtk_cam_request *req = job->req;
	struct device *dev = req->req.mdev->dev;
	bool buf_error = buf_state == VB2_BUF_STATE_ERROR;
	struct list_head done_list;
	unsigned long ids;
	bool is_buf_empty;

	media_request_get(&req->req);

	INIT_LIST_HEAD(&done_list);
	ids = 0;
	is_buf_empty = !mtk_cam_req_collect_vb_bufs(req,
				pipe_id, node_id,
				is_sv_pure_raw(job) && is_proc && !is_offline_timeshare(job),
				&done_list, &ids);

	if (node_id == -1)
		job->done_pipe |= BIT(pipe_id);

	if (CAM_DEBUG_ENABLED(V4L2))
		dev_info(dev, "%s: ctx-%d req:%s(%d) pipe_id:%d node_id:%d bufs:0x%lx ts:%lld%s%s\n",
			 __func__, job->src_ctx->stream_id,
			 req->debug_str, job->req_seq,
			 pipe_id, node_id, ids,
			 job->timestamp,
			 buf_error ? " error" : "",
			 is_buf_empty ? " (empty)" : "");

	if (unlikely(list_empty(&done_list))) {
		dev_info(dev,
			 "%s: req:%s failed to find pipe_id:%d node_id:%d%s\n",
			 __func__, req->debug_str,
			 pipe_id, node_id,
			 is_buf_empty ? " (empty)" : "");
		goto REQ_PUT;
	}
	if (job->req_sensor &&
		(job->req_sensor != job->req))
		mtk_cam_sensor_req_buffer_done(job, pipe_id, node_id,
			buf_state, is_proc);

	if (is_buf_empty) {
		/* assume: all ctrls are finished before buffers */
		req->is_buf_empty = 1;
		// remove from running job list
		remove_from_running_list(dev_get_drvdata(dev), req);
	}

	mark_each_buffer_done(&done_list, buf_state,
			      job->timestamp, job->timestamp_mono);

	if (is_buf_empty)
		mtk_cam_req_dump_incomplete_ctrl(req);
REQ_PUT:
	media_request_put(&req->req);
}

u64 mtk_cam_query_interval_from_sensor(struct v4l2_subdev *sensor)
{
	struct v4l2_subdev_frame_interval fi; /* in seconds */
	u64 frame_interval_ns = 1000000000ULL / 30ULL;

	if (!sensor) {
		pr_info("%s: warn. without sensor\n", __func__);
		return frame_interval_ns;
	}

	memset(&fi, 0, sizeof(fi));

	fi.pad = 0;

#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
	v4l2_subdev_call_state_active(sensor, pad, get_frame_interval, &fi);
#else
	v4l2_subdev_call(sensor, video, g_frame_interval, &fi);
#endif

	if (fi.interval.denominator)
		frame_interval_ns = (fi.interval.numerator * 1000000000ULL) /
			fi.interval.denominator;
	else {
		pr_info("%s: warn. wrong fi (%u/%u)\n", __func__,
			fi.interval.numerator,
			fi.interval.denominator);
		frame_interval_ns = 1000000000ULL / 30ULL;
	}

	if (CAM_DEBUG_ENABLED(CTRL))
		pr_info("%s: fi %llu ns\n", __func__, frame_interval_ns);

	return frame_interval_ns;
}

u64 mtk_cam_query_interval_from_ctrl_data(struct mtk_cam_ctx *ctx)
{
	struct v4l2_fract fi; /* in seconds */
	u64 frame_interval_ns = 1000000000ULL / 30ULL;
	struct mtk_raw_pipeline *pipeline;
	struct mtk_raw_ctrl_data *ctrl_data;

	if (!ctx || !ctx->has_raw_subdev)
		return frame_interval_ns;

	pipeline = &ctx->cam->pipelines.raw[ctx->raw_subdev_idx];
	ctrl_data = &pipeline->ctrl_data;
	fi = ctrl_data->resource.user_data.sensor_res.interval;

	if (fi.denominator)
		frame_interval_ns = (fi.numerator * 1000000000ULL) /
			fi.denominator;
	else {
		pr_info("%s: warn. wrong fi (%u/%u)\n", __func__,
			fi.numerator, fi.denominator);
		frame_interval_ns = 1000000000ULL / 30ULL;
	}

	if (CAM_DEBUG_ENABLED(CTRL))
		pr_info("%s: fi %llu ns\n", __func__, frame_interval_ns);

	return frame_interval_ns;
}

/* use engine's entity to find seninf or use seninf to find sensor */
struct v4l2_subdev
*mtk_cam_find_sensor_seninf(struct v4l2_subdev *subdev, int media_func)
{
	struct v4l2_subdev *sensor_seninf = NULL;
	struct media_link *link;

	if (!subdev)
		return NULL;

	if (media_func != MEDIA_ENT_F_CAM_SENSOR &&
	    media_func != MEDIA_ENT_F_VID_IF_BRIDGE) {
		pr_info("%s: media_func must be 0x%x or 0x%x", __func__,
			MEDIA_ENT_F_CAM_SENSOR, MEDIA_ENT_F_VID_IF_BRIDGE);
		return NULL;
	}

	for_each_media_entity_data_link(&subdev->entity, link) {
		if (link->flags & MEDIA_LNK_FL_ENABLED &&
		    link->sink->entity == &subdev->entity &&
		    link->source->entity->function == media_func) {
			sensor_seninf = media_entity_to_v4l2_subdev(link->source->entity);
			if (CAM_DEBUG_ENABLED(V4L2))
				dev_info(subdev->v4l2_dev->dev,
					 "%s:link(0x%lx), sink:%s/%p, source:%s, subdev:%s/%p\n",
					 __func__, link->flags, link->sink->entity->name,
					 link->sink->entity, link->source->entity->name,
					 subdev->entity.name, &subdev->entity);
			break;
		}
	}

	return sensor_seninf;
}

static int get_ipi_id(int stream_id)
{
	int ipi_id = stream_id + CCD_IPI_ISP_MAIN;

	if (WARN_ON(ipi_id < CCD_IPI_ISP_MAIN || ipi_id > CCD_IPI_MRAW_CMD))
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

	dev_info(cam->dev, "rpmsg_send: ctx-%d DESTROY_SESSION\n",
		 ctx->stream_id);
}

void isp_composer_flush_session(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtkcam_ipi_event event;
	struct mtkcam_ipi_session_cookie *session = &event.cookie;

	memset(&event, 0, sizeof(event));
	event.cmd_id = CAM_CMD_FLUSH;
	session->session_id = ctx->stream_id;
	rpmsg_send(ctx->rpmsg_dev->rpdev.ept, &event, sizeof(event));
	if (CAM_DEBUG_ENABLED(JOB))
		dev_info(cam->dev, "rpmsg_send: ctx-%d FLUSH\n", ctx->stream_id);
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

	ctx->rpmsg_dev = mtk_get_client_msgdevice(rpmsg_subdev, msg,
						  isp_composer_handler, cam);

	if (!ctx->rpmsg_dev) {
		dev_info(dev, "%s failed get_client_msgdevice, ctx:%d\n",
			 __func__, ctx->stream_id);
		return -EINVAL;
	}

	if (CAM_DEBUG_ENABLED(V4L2_TRY))
		dev_info(dev, "%s initialized composer of ctx:%d\n",
		 __func__, ctx->stream_id);

	return 0;
}

static void isp_composer_uninit(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;

	if (!cam->rproc_handle)
		return;

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

int mtk_cam_power_rproc(struct mtk_cam_device *cam, int on)
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

#ifdef ENABLE_CCU
static int mtk_cam_get_ccu_phandle(struct mtk_cam_device *cam)
{
	struct device *dev = cam->dev;
	struct device_node *node;
	int ret;

	node = of_find_compatible_node(NULL, NULL, "mediatek,camera-camsys-ccu");
	if (node == NULL) {
		dev_info(dev, "of_find mediatek,camera-camsys-ccu fail\n");
		return -1;
	}

	ret = of_property_read_u32(node, "mediatek,ccu-rproc",
				   &cam->rproc_ccu_phandle);
	if (ret)
		dev_info(dev, "fail to get ccu rproc_phandle:%d\n", ret);

	of_node_put(node);
	return ret;
}

int mtk_cam_power_ctrl_ccu(struct device *dev, int on_off)
{
	struct mtk_cam_device *cam = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&cam->ccu_lock);

	if (on_off) {
		struct device_node *rproc_np;

		if (cam->ccu_use_cnt) {
			++cam->ccu_use_cnt;
			goto EXIT;
		}

		WARN_ON(cam->rproc_ccu_handle);

		ret = mtk_cam_get_ccu_phandle(cam);
		if (ret)
			goto EXIT;

		cam->rproc_ccu_handle = rproc_get_by_phandle(cam->rproc_ccu_phandle);
		if (!cam->rproc_ccu_handle) {
			dev_info(dev, "Get ccu handle fail\n");

			ret = -1;
			goto EXIT;
		}

		rproc_np = of_find_node_by_phandle(cam->rproc_ccu_phandle);
		if (!rproc_np) {
			dev_info(dev, "failed to find node of ccu rproc\n");
			rproc_put(cam->rproc_ccu_handle);
			cam->rproc_ccu_handle = NULL;

			ret = -1;
			goto EXIT;
		}

		cam->ccu_pdev = of_find_device_by_node(rproc_np);
		if (!cam->ccu_pdev) {
			dev_info(dev, "failed to find ccu rproc pdev\n");
			of_node_put(rproc_np);
			rproc_put(cam->rproc_ccu_handle);
			cam->rproc_ccu_handle = NULL;

			ret = -1;
			goto EXIT;
		}

#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
		ret = rproc_bootx(cam->rproc_ccu_handle, RPROC_UID_CAM);
#else
		ret = rproc_boot(cam->rproc_ccu_handle);
#endif
		if (ret) {
			dev_info(dev, "boot ccu rproc fail, ret=%d\n", ret);
			goto EXIT;
		}

		++cam->ccu_use_cnt;
	} else {

		if (WARN_ON(!cam->ccu_use_cnt)) {
			ret = -1;
			goto EXIT;
		}

		--cam->ccu_use_cnt;
		if (cam->ccu_use_cnt)
			goto EXIT;

		if (!cam->rproc_ccu_handle)
			goto EXIT;

#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
		rproc_shutdownx(cam->rproc_ccu_handle, RPROC_UID_CAM);
#else
		rproc_shutdown(cam->rproc_ccu_handle);
#endif

		rproc_put(cam->rproc_ccu_handle);
		platform_device_put(cam->ccu_pdev);

		cam->rproc_ccu_handle = NULL;
		cam->ccu_pdev = NULL;
	}

EXIT:
	dev_info(dev, "%s: on %d cnt %d, ret = %d\n", __func__,
		 on_off, cam->ccu_use_cnt, ret);
	mutex_unlock(&cam->ccu_lock);
	return ret;
}
#endif

static void mtk_cam_plat_resource_ctrl(struct mtk_cam_device *cam, int on_off)
{
	int ret, ack = 0;

	writel(on_off ? 0x1fd : 0x0, cam->vcore_ddren_en);

	if (on_off) {
		ret = readx_poll_timeout(readl, cam->vcore_ddren_ack,
					 ack,
					 ack & 0x1fc,
					 1 /* delay, us */,
					 2000 /* timeout, us */);
		if (ret < 0) {
			dev_info(cam->dev, "%s: error: timeout!, (ack 0x%x)\n",
				__func__, ack);
			return;
		}
	}
	if (CAM_DEBUG_ENABLED(V4L2_TRY))
		dev_info(cam->dev, "%s: ddren:0x%x, ack:0x%x", __func__,
		readl_relaxed(cam->vcore_ddren_en),
		readl_relaxed(cam->vcore_ddren_ack));
}

static int mtk_cam_initialize(struct mtk_cam_device *cam)
{
	int ret;

	if (atomic_add_return(1, &cam->initialize_cnt) > 1)
		return 0;

	dev_info(cam->dev, "camsys initialize\n");

	mtk_cam_dvfs_reset_runtime_info(&cam->dvfs);

	WARN_ON(pm_runtime_get_sync(cam->dev));

	ret = mtk_cam_power_rproc(cam, 1);
	if (ret)
		return ret; //TODO: goto

	mtk_cam_debug_exp_reset(&cam->dbg);

	return ret;
}

int mtk_cam_uninitialize(struct mtk_cam_device *cam)
{
	if (!atomic_sub_and_test(1, &cam->initialize_cnt))
		return 0;

	dev_info(cam->dev, "camsys uninitialize\n");

	mtk_cam_power_rproc(cam, 0);
	pm_runtime_put_sync(cam->dev);
	wake_up(&cam->shutdown_wq);

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

	if (ipi_msg->cmd_id != CAM_CMD_ACK)
		return -EINVAL;

	if (ipi_msg->ack_data.ack_cmd_id == CAM_CMD_FRAME) {
		ctx = &cam->ctxs[ipi_msg->cookie.session_id];

		MTK_CAM_TRACE_BEGIN(BASIC, "ipi_frame_ack:%d",
				    ipi_msg->cookie.frame_no);

		if (CAM_DEBUG_ENABLED(JOB))
			dev_info(dev, "ipi_frame_ack: ctx-%d seq-%d\n",
				 ctx->stream_id,
				 (int)seq_from_fh_cookie(ipi_msg->cookie.frame_no));

		mtk_cam_ctrl_job_composed(&ctx->cam_ctrl,
					  ipi_msg->cookie.frame_no,
					  &ipi_msg->ack_data.frame_result,
					  ipi_msg->ack_data.ret);

		MTK_CAM_TRACE_END(BASIC);
		return 0;

	} else if (ipi_msg->ack_data.ack_cmd_id == CAM_CMD_FLUSH) {
		ctx = &cam->ctxs[ipi_msg->cookie.session_id];
		complete(&ctx->session_flush);
		if (CAM_DEBUG_ENABLED(JOB))
			dev_info(dev, "%s:ctx(%d): session flushed",
			 __func__, ctx->stream_id);
	} else if (ipi_msg->ack_data.ack_cmd_id == CAM_CMD_DESTROY_SESSION) {
		ctx = &cam->ctxs[ipi_msg->cookie.session_id];
		complete(&ctx->session_complete);
		if (CAM_DEBUG_ENABLED(JOB))
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

void mtk_cam_ctx_put(struct mtk_cam_ctx *ctx)
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

/**
 *  Max connected entities of a context:
 *  sensor, seninf, engines and their video devices
 */
#define MTK_CAM_CTX_MAX_ENTITIES (2 + (1 + MTK_RAW_TOTAL_NODES) + \
				  (1 + MTK_CAMSV_TOTAL_NODES) * MAX_SV_PIPES_PER_STREAM + \
				  (1 + MTK_MRAW_TOTAL_NODES) * MAX_MRAW_PIPES_PER_STREAM)

static int mtk_cam_ctx_pipeline_start(struct mtk_cam_ctx *ctx,
				      struct media_entity *entity)
{
	struct device *dev = ctx->cam->dev;
	struct v4l2_subdev **target_sd;
	struct mtk_cam_video_device *mtk_vdev;
	struct media_pipeline_pad *ppad;
	struct media_entity *entity_walked[MTK_CAM_CTX_MAX_ENTITIES] = {0};
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
		     j <= last_entity_walked && j < MTK_CAM_CTX_MAX_ENTITIES;
		     j++) {
			if (entity_walked[j] == entity) {
				walked = true;
				break;
			}
		}

		if (!walked) {
			if (last_entity_walked > (MTK_CAM_CTX_MAX_ENTITIES - 1)) {
				dev_info(dev,
					 "ctx-%d abnormal entity counts:%d\n",
					 ctx->stream_id, last_entity_walked);
				goto fail_stop_pipeline;
			}
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

static struct task_struct *
mtk_cam_ctx_create_task(struct mtk_cam_ctx *ctx,
			const char *prefix,
			struct kthread_worker *worker,
			bool set_fifo)
{
	struct device *dev = ctx->cam->dev;
	struct task_struct *task;

	task = kthread_run(kthread_worker_fn, worker,
			   "%s-%d", prefix, ctx->stream_id);

	if (IS_ERR(task)) {
		dev_info(dev, "%s: failed. could not create %s-%d\n",
			 __func__, prefix, ctx->stream_id);
		return NULL;
	}

	if (set_fifo)
		sched_set_fifo(task);
	else
		sched_set_normal(task, -20);

	return task;
}

static int mtk_cam_ctx_alloc_workers(struct mtk_cam_ctx *ctx)
{
	kthread_init_worker(&ctx->sensor_worker);
	ctx->sensor_worker_task =
		mtk_cam_ctx_create_task(ctx, "sensor_worker",
					&ctx->sensor_worker, true);
	if (!ctx->sensor_worker_task)
		return -1;

	kthread_init_worker(&ctx->flow_worker);
	ctx->flow_task =
		mtk_cam_ctx_create_task(ctx, "camsys_worker",
					&ctx->flow_worker, true);
	if (!ctx->flow_task)
		goto fail_uninit_sensor_worker_task;

	kthread_init_worker(&ctx->done_worker);
	ctx->done_task =
		mtk_cam_ctx_create_task(ctx, "camsys_done",
					     &ctx->done_worker, true);
	if (!ctx->done_task)
		goto fail_uninit_flow_worker_task;

	kthread_init_worker(&ctx->tuning_worker);
	ctx->tuning_task =
		mtk_cam_ctx_create_task(ctx, "camsys_tuning",
					     &ctx->tuning_worker, true);
	if (!ctx->tuning_task)
		goto fail_uninit_done_worker_task;

	return 0;

fail_uninit_done_worker_task:
	kthread_stop(ctx->done_task);
	ctx->done_task = NULL;
fail_uninit_flow_worker_task:
	kthread_stop(ctx->flow_task);
	ctx->flow_task = NULL;
fail_uninit_sensor_worker_task:
	kthread_stop(ctx->sensor_worker_task);
	ctx->sensor_worker_task = NULL;

	return -1;
}

static void mtk_cam_ctx_destroy_workers(struct mtk_cam_ctx *ctx)
{
	kthread_stop(ctx->sensor_worker_task);
	ctx->sensor_worker_task = NULL;
	kthread_stop(ctx->flow_task);
	ctx->flow_task = NULL;
	kthread_stop(ctx->done_task);
	ctx->done_task = NULL;
	kthread_stop(ctx->tuning_task);
	ctx->tuning_task = NULL;
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

static struct device *get_dev_to_attach(struct mtk_cam_ctx *ctx)
{
	return ctx->cam->smmu_dev ? : ctx->cam->engines.raw_devs[0];
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

int mtk_cam_ctx_alloc_rgbw_caci_buf(struct mtk_cam_ctx *ctx, int w, int h)
{
	struct device *dev_to_attach;

	size_t caci_size = 0;

	dev_to_attach = get_dev_to_attach(ctx);

	if (CALL_PLAT_HW(query_caci_size, w, h, &caci_size) || caci_size == 0)
		return -EINVAL;

	ctx->w_caci_buf = mtk_cam_device_refcnt_buf_create(dev_to_attach,
						 "CAM_W_CACI_ID",
						 caci_size);
	if (ctx->w_caci_buf)
		return 0;
	else
		return -ENOMEM;
}

void mtk_cam_ctx_clean_rgbw_caci_buf(struct mtk_cam_ctx *ctx)
{
	if (ctx->w_caci_buf) {
		mtk_cam_device_refcnt_buf_put(ctx->w_caci_buf);
		ctx->w_caci_buf = NULL;
	}
}

static void mtk_cam_ctx_reset_slb(struct mtk_cam_ctx *ctx)
{
	ctx->slb_uid = 0;
	ctx->slb_addr = 0;
	ctx->slb_iova = 0;
	ctx->slb_used_size = 0;
	ctx->slb_size = 0;
}

static void mtk_cam_ctx_release_slb(struct mtk_cam_ctx *ctx);
static int mtk_cam_ctx_request_slb(struct mtk_cam_ctx *ctx, int uid,
				   bool map_iova,
				   struct slbc_data *early_request_slb)
{
	struct device *dev = ctx->cam->dev;
	struct slbc_data slb;
	struct device *dma_dev;
	dma_addr_t iova;
	int ret;

	if (!early_request_slb) {

		slb.uid = uid;
		slb.type = TP_BUFFER;

		ret = slbc_request(&slb);
		if (ret < 0) {
			dev_info(dev, "%s: allocate slb fail\n", __func__);
			return -1;
		}
	} else {

		if (WARN_ON(early_request_slb->uid != uid))
			return -1;

		slb = *early_request_slb;
	}

	ctx->slb_uid = uid;
	ctx->slb_addr = slb.paddr;
	ctx->slb_size = slb.size;
	ctx->slb_iova = 0;

	if (!map_iova)
		goto SUCCESS_REQUEST;

	dma_dev = ctx->cam->smmu_dev;
	iova = dma_map_resource(dma_dev, (phys_addr_t)slb.emi_paddr, ctx->slb_size,
				DMA_BIDIRECTIONAL, 0);
	if (dma_mapping_error(dma_dev, iova)) {
		dev_info(dev, "%s: failed to map resource\n", __func__);
		mtk_cam_ctx_release_slb(ctx);
		return -1;
	}
	ctx->slb_iova = iova;

SUCCESS_REQUEST:
	dev_info(dev, "%s: slb buffer uid %d base(0x%lx/%pad), size(%ld)",
		 __func__, uid, (uintptr_t)slb.paddr, &ctx->slb_iova, slb.size);
	return 0;
}

static void mtk_cam_ctx_release_slb(struct mtk_cam_ctx *ctx)
{
	struct device *dev = ctx->cam->dev;
	struct slbc_data slb;
	int ret;

	if (!ctx->slb_addr)
		return;

	if (ctx->slb_iova) {
		struct device *dma_dev;

		dma_dev = ctx->cam->smmu_dev;
		dma_unmap_resource(dma_dev, ctx->slb_iova, ctx->slb_size,
				   DMA_BIDIRECTIONAL, 0);
	}

	slb.uid = ctx->slb_uid;
	slb.type = TP_BUFFER;

	ret = slbc_release(&slb);
	if (ret < 0)
		dev_info(dev, "failed to release slb buffer\n");

	mtk_cam_ctx_reset_slb(ctx);

	/* reset aid: not necessary */
}
static int mtk_cam_ctx_request_slc(struct mtk_cam_ctx *ctx, u8 slc_mode)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_MTK_SLBC)
	ctx->slc_data.sign = SLC_DATA_MAGIC;
        ctx->slc_data.flag = (slc_mode == SLC_WITH_DISCARD) ? (GS_RD | GS_M) : GS_M;
	ctx->slc_gid = -1;
	ret = slbc_gid_request(ID_CAM, &ctx->slc_gid, &ctx->slc_data);
	dev_info(ctx->cam->dev, "%s: slc_data gid/bw/flag/dma size:%d/%d/%d/%d\n", __func__,
		ctx->slc_gid, ctx->slc_data.bw, ctx->slc_data.flag, ctx->slc_data.dma_size);
	ctx->slc_data_valid = true;
#endif
	return ret;
}

static int mtk_cam_ctx_release_slc(struct mtk_cam_ctx *ctx)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_MTK_SLBC)
	if (ctx->slc_data_valid) {
		ret = slbc_gid_release(ID_CAM, ctx->slc_gid);
		dev_info(ctx->cam->dev, "%s: slc_data bw/dma size:%d/%d\n", __func__,
			ctx->slc_data.bw, ctx->slc_data.dma_size);
		ctx->slc_data_valid = false;
	}
#endif
	return ret;
}
static int mtk_cam_ctx_validate_slc(struct mtk_cam_ctx *ctx)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_MTK_SLBC)
	if (ctx->slc_validated == false) {
		ret = slbc_validate(ID_CAM, ctx->slc_gid);
		dev_info(ctx->cam->dev, "%s: gid:%d slc_data bw/dma size:%d/%d\n", __func__,
			ctx->slc_gid, ctx->slc_data.bw, ctx->slc_data.dma_size);
		ctx->slc_validated = true;
	}
#endif
	return ret;
}

static int mtk_cam_ctx_invalidate_slc(struct mtk_cam_ctx *ctx)
{
	int ret = 0;

#if IS_ENABLED(CONFIG_MTK_SLBC)
	if (ctx->slc_validated) {
		ret = slbc_invalidate(ID_CAM, ctx->slc_gid);
		dev_info(ctx->cam->dev, "%s: gid:%d slc_data bw/dma size:%d/%d\n", __func__,
			ctx->slc_gid, ctx->slc_data.bw, ctx->slc_data.dma_size);
		ctx->slc_validated = false;
	}
#endif
	return ret;
}
static int mtk_cam_ctx_slc_read_invalidate(struct mtk_cam_ctx *ctx, bool en)
{
	int ret = 0;

#if IS_ENABLED(CONFIG_MTK_SLBC)
	ret = slbc_read_invalidate(ID_CAM, ctx->slc_gid, en);
	dev_info(ctx->cam->dev, "%s: en:%d gid:%d slc_data bw/dma size:%d/%d\n", __func__,
		en, ctx->slc_gid, ctx->slc_data.bw, ctx->slc_data.dma_size);
#endif
	return ret;
}
int mtk_cam_ctx_slc_stream(struct mtk_cam_ctx *ctx, bool on, int mode)
{
	int ret = 0;

	if (!ctx->slc_data_valid || !mode)
		return ret;

	if (on) {
		mtk_cam_ctx_validate_slc(ctx);
		mtk_cam_ctx_slc_read_invalidate(ctx, mode == SLC_NO_DISCARD);
	} else {
		mtk_cam_ctx_invalidate_slc(ctx);
	}

	return ret;
}

/* LTMS buffer */
static int deinit_ltms_buf_pool(struct mtk_cam_ctx *ctx)
{
	mtk_cam_buffer_pool_return(&ctx->ltms_buf->buf_1);
	mtk_cam_buffer_pool_return(&ctx->ltms_buf->buf_2);
	_destroy_pool(&ctx->ltms_buf->buffer, &ctx->ltms_buf->pool);
	kfree(ctx->ltms_buf);
	ctx->ltms_buf = NULL;

	return 0;
}

static int init_ltms_buf_pool(struct mtk_cam_ctx *ctx)
{
	int ret;

	if (WARN_ON(ctx->ltms_buf))
		return 0;

	ctx->ltms_buf =
		kmalloc(sizeof(struct mtk_ltms_buf_pool), GFP_KERNEL);

	if (!ctx->ltms_buf)
		goto fail_to_kmalloc;

	ctx->ltms_buf->flip = false;

	ret = _alloc_pool("CAM_MEM_LTMS_ID",
			&ctx->ltms_buf->buffer, &ctx->ltms_buf->pool,
			get_dev_to_attach(ctx),
			LTMSGO_BUF_SZ, LTMSGO_BUF_RESERVE_CNT,
			false); // TODO: check if cachable
	if (ret)
		goto fail_to_alloc;

	ret = mtk_cam_buffer_pool_fetch(&ctx->ltms_buf->pool,
									&ctx->ltms_buf->buf_1);
	if (ret)
		goto fail_to_fetch_first;

	ret = mtk_cam_buffer_pool_fetch(&ctx->ltms_buf->pool,
									&ctx->ltms_buf->buf_2);
	if (ret)
		goto fail_to_fetch_second;

	return 0;

fail_to_fetch_second:
	mtk_cam_buffer_pool_return(&ctx->ltms_buf->buf_1);
fail_to_fetch_first:
	_destroy_pool(&ctx->ltms_buf->buffer, &ctx->ltms_buf->pool);
fail_to_alloc:
	kfree(ctx->ltms_buf);
	ctx->ltms_buf = NULL;
fail_to_kmalloc:
	return -ENOMEM;
}

int mtk_cam_assign_ltms_buffer(struct mtk_cam_ctx *ctx,
			 struct mtk_cam_pool_buffer *in,
			 struct mtk_cam_pool_buffer *out)
{
	if (ctx->ltms_buf->flip) {
		memcpy(in, &ctx->ltms_buf->buf_1, sizeof(struct mtk_cam_pool_buffer));
		memcpy(out, &ctx->ltms_buf->buf_2, sizeof(struct mtk_cam_pool_buffer));
	} else {
		memcpy(out, &ctx->ltms_buf->buf_1, sizeof(struct mtk_cam_pool_buffer));
		memcpy(in, &ctx->ltms_buf->buf_2, sizeof(struct mtk_cam_pool_buffer));
	}

	ctx->ltms_buf->flip = !ctx->ltms_buf->flip;

	return 0;
}

/* for cq working buffers */
#define IPI_FRAME_BUF_SIZE ALIGN(sizeof(struct mtkcam_ipi_frame_param), SZ_1K)
static int mtk_cam_ctx_alloc_pool(struct mtk_cam_ctx *ctx)
{
	struct device *dev_to_attach;
	int ret;

	dev_to_attach = get_dev_to_attach(ctx);

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

	ret = init_ltms_buf_pool(ctx);
	if (ret)
		goto fail_destroy_ipi;

	return 0;

fail_destroy_ipi:
	_destroy_pool(&ctx->ipi_buffer, &ctx->ipi_pool);
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

static int update_from_mbus_bayer(struct mtk_cam_buf_fmt_desc *fmt_desc,
	struct v4l2_mbus_framefmt *mf)
{
	fmt_desc->ipi_fmt = sensor_mbus_to_ipi_fmt(mf->code);
	fmt_desc->pixel_fmt = sensor_mbus_to_pixel_format(mf->code);
	fmt_desc->width = mf->width;
	fmt_desc->height = mf->height;

	return (fmt_desc->ipi_fmt == MTKCAM_IPI_IMG_FMT_UNKNOWN) ? -1 : 0;
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

	return (fmt_desc->ipi_fmt == MTKCAM_IPI_IMG_FMT_UNKNOWN) ? -1 : 0;
}

static int calc_buf_param_ufbc(struct mtk_cam_buf_fmt_desc *fmt_desc)
{
	int ret = 0;
	u32 stride = 0, size = 0;

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
	size_t max_size = 0;
	struct buf_fmt_ops *ops[] = {
		&buf_fmt_ops_bayer,
		&buf_fmt_ops_ufbc
	};

	for (i = 0; i < ARRAY_SIZE(ops) && !ret; ++i) {
		ret = ret || ops[i]->update_input_param(&desc->fmt_desc[i], mf);
		ret = ret || ops[i]->calc_buf_param(&desc->fmt_desc[i]);

		if (ret) {
			WARN_ON_ONCE(1);
			break;
		}

		max_size = max(max_size, desc->fmt_desc[i].size);
	}
	desc->max_size = max_size;

	return ret;
}

bool mtk_cam_check_img_pool_need(struct device *dev_to_attach,
	struct mtk_raw_ctrl_data *ctrl_data,
	struct v4l2_mbus_framefmt *mf,
	struct mtk_cam_driver_buf_desc *desc)
{
	struct mtk_cam_resource_raw_v2 *raw_res;

	raw_res = &ctrl_data->resource.user_data.raw_res;
	if (raw_res->img_wbuf_num == 0)
		goto EXIT_NO_POOL;

	if (update_buf_fmt_desc(desc, mf))
		goto EXIT_NO_POOL;

	return true;

EXIT_NO_POOL:
	return false;
}

int mtk_cam_alloc_img_pool(struct device *dev_to_attach,
	struct mtk_raw_ctrl_data *ctrl_data,
	struct v4l2_mbus_framefmt *mf,
	struct mtk_cam_driver_buf_desc *desc,
	struct mtk_cam_device_buf *img_work_buffer,
	struct mtk_cam_pool *img_work_pool)
{
	struct mtk_cam_resource_raw_v2 *raw_res;
	int i;
	int ret = 0;
	int total_size;

	raw_res = &ctrl_data->resource.user_data.raw_res;
	total_size = desc->max_size * raw_res->img_wbuf_num;
	if (ctrl_data->pre_alloc_mem.num &&
	    total_size <= ctrl_data->pre_alloc_mem.bufs[0].length) {
		ret = _alloc_pool_by_dbuf(
			  img_work_buffer, img_work_pool,
			  dev_to_attach,
			  ctrl_data->pre_alloc_dbuf,
			  total_size,
			  raw_res->img_wbuf_num);
	} else {
		ret = _alloc_pool(
			  "CAM_MEM_IMG_ID",
			  img_work_buffer, img_work_pool,
			  dev_to_attach, desc->max_size,
			  raw_res->img_wbuf_num,
			  false);
	}

	for (i = 0; i < ARRAY_SIZE(desc->fmt_desc); ++i) {
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
}

static void
mtk_cam_device_refcnt_buf_free(struct kref *ref)
{
	struct mtk_cam_device_refcnt_buf *refcnt_buf;

	refcnt_buf = container_of(ref, struct mtk_cam_device_refcnt_buf, refcount);
	kfree(refcnt_buf);
	if (CAM_DEBUG_ENABLED(IPI_BUF))
		pr_info("%s:free object\n", __func__);
}

void
mtk_cam_device_refcnt_buf_destroy(struct kref *ref)
{
	struct mtk_cam_device_refcnt_buf *refcnt_buf;

	refcnt_buf = container_of(ref, struct mtk_cam_device_refcnt_buf, refcount);
	if (refcnt_buf->buf.size)
		mtk_cam_device_buf_uninit(&refcnt_buf->buf);
	else
		pr_info("%s: refcnt_buf(%p) size is 0, uninit not called",
			__func__, refcnt_buf);

	mtk_cam_device_refcnt_buf_free(ref);
}

struct mtk_cam_device_refcnt_buf*
mtk_cam_device_refcnt_buf_create(struct device *dev_to_attach,
			 const char *buf_name,
			 size_t size)
{
	struct mtk_cam_device_refcnt_buf *refcnt_buf;
	int ret;

	refcnt_buf = kmalloc(sizeof(*refcnt_buf), GFP_KERNEL);
	if (!refcnt_buf)
		return NULL;

	kref_init(&refcnt_buf->refcount);

	refcnt_buf->buf.dbuf = _alloc_dma_buf(buf_name, size, false);
	ret = (!refcnt_buf->buf.dbuf) ? -1 : 0;

	ret = ret
		|| mtk_cam_device_buf_init(&refcnt_buf->buf, refcnt_buf->buf.dbuf,
					   dev_to_attach, size)
		|| mtk_cam_device_buf_vmap(&refcnt_buf->buf);

	dma_heap_buffer_free(refcnt_buf->buf.dbuf);

	if (!ret) {
		memset(refcnt_buf->buf.vaddr, 0, refcnt_buf->buf.size);
	} else {
		if (CAM_DEBUG_ENABLED(IPI_BUF))
			pr_info("%s:kfree:%p\n", __func__, refcnt_buf);
		kfree(refcnt_buf);
		return NULL;
	}

	return refcnt_buf;
}

void mtk_cam_device_refcnt_buf_get(struct mtk_cam_device_refcnt_buf *buf)
{
	if (CAM_DEBUG_ENABLED(IPI_BUF))
		pr_info("%s:kref_get:%p\n", __func__, buf);
	kref_get(&buf->refcount);

}

void mtk_cam_device_refcnt_buf_put(struct mtk_cam_device_refcnt_buf *buf)
{
	if (CAM_DEBUG_ENABLED(IPI_BUF))
		pr_info("%s kref_put:%p\n", __func__, buf);
	kref_put(&buf->refcount, mtk_cam_device_refcnt_buf_destroy);
}

static void
mtk_cam_pool_wrapper_free(struct kref *ref)
{
	struct mtk_cam_pool_wrapper *wrapper;

	wrapper = container_of(ref, struct mtk_cam_pool_wrapper, refcount);
	kfree(wrapper);
	if (CAM_DEBUG_ENABLED(IPI_BUF))
		pr_info("%s:free object\n", __func__);
}

void
mtk_cam_pool_wrapper_destroy(struct kref *ref)
{
	struct mtk_cam_pool_wrapper *wrapper;

	wrapper = container_of(ref, struct mtk_cam_pool_wrapper, refcount);
	_destroy_pool(&wrapper->mem, &wrapper->pool);
	mtk_cam_pool_wrapper_free(ref);
}

void
mtk_cam_pool_wrapper_get(struct mtk_cam_pool_wrapper *wrapper)
{
	if (CAM_DEBUG_ENABLED(IPI_BUF))
		pr_info("%s:kref_get:%p\n", __func__, wrapper);

	kref_get(&wrapper->refcount);

}

void
mtk_cam_pool_wrapper_put(struct mtk_cam_pool_wrapper *wrapper)
{
	if (CAM_DEBUG_ENABLED(IPI_BUF))
		pr_info("%s:kref_put:%p\n", __func__, wrapper);

	kref_put(&wrapper->refcount, mtk_cam_pool_wrapper_destroy);
}

/* create and set the refcount to 1*/
struct mtk_cam_pool_wrapper*
mtk_cam_pool_wrapper_create(struct device *dev_to_attach,
			    struct mtk_raw_ctrl_data *ctrl_data,
			    struct v4l2_mbus_framefmt *mf,
			    struct mtk_cam_driver_buf_desc *desc)
{
	struct mtk_cam_pool_wrapper *wrapper;
	int ret;

	wrapper = kmalloc(sizeof(*wrapper), GFP_KERNEL);
	if (!wrapper)
		return NULL;

	kref_init(&wrapper->refcount);
	ret = mtk_cam_alloc_img_pool(dev_to_attach, ctrl_data, mf,
				     desc, &wrapper->mem, &wrapper->pool);
	if (ret) {
		if (CAM_DEBUG_ENABLED(IPI_BUF))
			pr_info("%s kfree:%p\n", __func__, wrapper);
		kfree(wrapper);
		return NULL;
	}

	return wrapper;
}

int mtk_cam_ctx_alloc_img_pool(struct mtk_cam_ctx *ctx, struct mtk_raw_ctrl_data *ctrl_data)
{
	struct device *dev_to_attach;
	struct mtk_cam_driver_buf_desc *desc = &ctx->img_work_buf_desc;
	struct mtk_raw_pipeline *raw_pipe;
	struct v4l2_mbus_framefmt *mf;
	int ret = 0;

	if (!ctx->has_raw_subdev)
		goto EXIT_NO_POOL;

	raw_pipe = &ctx->cam->pipelines.raw[ctx->raw_subdev_idx];
	if (!ctrl_data)
		ctrl_data = &raw_pipe->ctrl_data;

	// TODO(Will): for BAYER10_MIPI, use MTK_RAW_MAIN_STREAM_OUT fmt instead;
	mf = &raw_pipe->pad_cfg[MTK_RAW_SINK].mbus_fmt;
	if (!mtk_cam_check_img_pool_need(ctx->cam->engines.raw_devs[0],
					 ctrl_data, mf, desc))
		goto EXIT_NO_POOL;

	dev_to_attach = get_dev_to_attach(ctx);

	/* init ref pool_wrapper cnt to 1 for the user of current ctx */
	ctx->pack_job_img_wbuf_pool_wrapper =
		mtk_cam_pool_wrapper_create(dev_to_attach,
					    ctrl_data, mf, desc);
	if (!ctx->pack_job_img_wbuf_pool_wrapper) {
		dev_info(ctx->cam->dev,
			 "%s: ctx-%d pack_job_img_wbuf_pool_wrapper alloc failed, can't be recovered\n",
			 __func__, ctx->stream_id);
		ret = -ENOMEM;
		goto EXIT_NO_POOL;
	}

	if (CAM_DEBUG_ENABLED(IPI_BUF))
		dev_info(ctx->cam->dev,
			 "%s: ctx-%d pack_job_img_wbuf_pool_wrapper(%p) created\n",
			 __func__, ctx->stream_id, ctx->pack_job_img_wbuf_pool_wrapper);

	return ret;

EXIT_NO_POOL:
	ctx->pack_job_img_wbuf_pool_wrapper = NULL;

	return ret;
}

static int mtk_cam_ctx_alloc_sensor_meta_pool(struct mtk_cam_ctx *ctx)
{
	struct device *dev_to_attach;
	int ret = 0;

	ctx->enable_sensor_meta_dump = (debug_sensor_meta_dump) ? true : false;

	if (debug_sensor_meta_dump) {
		dev_to_attach = get_dev_to_attach(ctx);

		ret = _alloc_pool("SENSOR_META_ID", &ctx->sensor_meta_buffer,
				&ctx->sensor_meta_pool,
				dev_to_attach, SENSOR_META_BUF_SIZE, SENSOR_META_BUF_NUM,
				false);
	}

	return ret;
}

static void mtk_cam_ctx_destroy_pool(struct mtk_cam_ctx *ctx)
{
	_destroy_pool(&ctx->cq_buffer, &ctx->cq_pool);
	_destroy_pool(&ctx->ipi_buffer, &ctx->ipi_pool);
	deinit_ltms_buf_pool(ctx);
}

/* Put img pool and clean ctx's related field */
void mtk_cam_ctx_clean_img_pool(struct mtk_cam_ctx *ctx)
{
	if (!ctx->pack_job_img_wbuf_pool_wrapper)
		return;

	if (CAM_DEBUG_ENABLED(IPI_BUF))
		dev_info(ctx->cam->dev,
			 "%s: ctx-%d img_wbuf_pool_wrapper(%p): put\n",
			 __func__, ctx->stream_id,
			 ctx->pack_job_img_wbuf_pool_wrapper);

	mtk_cam_pool_wrapper_put(ctx->pack_job_img_wbuf_pool_wrapper);
	ctx->pack_job_img_wbuf_pool_wrapper = NULL;
}

static void mtk_cam_ctx_destroy_sensor_meta_pool(struct mtk_cam_ctx *ctx)
{
	if (debug_sensor_meta_dump)
		_destroy_pool(&ctx->sensor_meta_buffer, &ctx->sensor_meta_pool);
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

int mtk_cam_ctx_unprepare_session(struct mtk_cam_ctx *ctx)
{
	struct device *dev = ctx->cam->dev;
	int ret;

	if (!ctx->session_created)
		return 0;

	dev_info(dev, "%s:ctx(%d): wait for session destroy\n",
		__func__, ctx->stream_id);

	isp_composer_destroy_session(ctx);

	ret = wait_for_completion_timeout(&ctx->session_complete,
					  msecs_to_jiffies(500));
	if (ret == 0)
		dev_info(dev, "%s:ctx(%d): wait session_complete timeout\n",
			 __func__, ctx->stream_id);

	isp_composer_uninit(ctx);

	ctx->session_created = 0;
	return 0;
}

int mtk_cam_ctx_flush_session(struct mtk_cam_ctx *ctx)
{
	struct device *dev = ctx->cam->dev;
	int ret;

	if (!ctx->session_created)
		return 0;

	init_completion(&ctx->session_flush);
	isp_composer_flush_session(ctx);

	ret = wait_for_completion_timeout(&ctx->session_flush,
					  msecs_to_jiffies(500));
	if (ret == 0)
		dev_info(dev, "%s:ctx(%d): wait session_flush timeout\n",
			 __func__, ctx->stream_id);
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
	if (CAM_DEBUG_ENABLED(V4L2))
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
	ctx->rms_disable = 0;
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

	if (mtk_cam_is_display_ic(ctx))
		ret = mtk_cam_pool_alloc(&ctx->job_pool,
					sizeof(struct mtk_cam_pool_job),
					JOB_NUM_PER_STREAM_DISPLAY_IC);
	else if (mtk_cam_is_non_comb_ic(ctx))
		ret = mtk_cam_pool_alloc(&ctx->job_pool,
					sizeof(struct mtk_cam_pool_job),
					JOB_NUM_PER_STREAM_NON_COMB_IC);
	else
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

	/* note: too early. move into mtk_cam_ctx_init_scenario */
	if (mtk_cam_ctx_alloc_img_pool(ctx, NULL))
		goto fail_destroy_pools;

	if (mtk_cam_ctx_alloc_sensor_meta_pool(ctx))
		goto fail_destroy_img_pool;

	if (mtk_cam_ctx_prepare_session(ctx))
		goto fail_destroy_sensor_meta_pool;

	if (mtk_cam_ctx_init_job_pool(ctx))
		goto fail_unprepare_session;

	mtk_cam_update_pipe_used(ctx, &cam->pipelines);
	mtk_cam_ctrl_start(&ctx->cam_ctrl, ctx);
	mtk_raw_hdr_tsfifo_reset(ctx);

	if (cam->cmdq_clt) {
		cmdq_mbox_enable(cam->cmdq_clt->chan);
		ctx->cmdq_enabled = 1;
	}

	return ctx;

fail_unprepare_session:
	mtk_cam_ctx_unprepare_session(ctx);
fail_destroy_sensor_meta_pool:
	mtk_cam_ctx_destroy_sensor_meta_pool(ctx);
fail_destroy_img_pool:
	mtk_cam_ctx_clean_img_pool(ctx);
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

	if (!atomic_read(&ctx->cam_ctrl.stopped)) {
		dev_info(cam->dev, "%s: cam_ctl still started!\n", __func__);
		mtk_cam_ctrl_stop(&ctx->cam_ctrl);
	}

	mtk_cam_ctx_destroy_sensor_meta_pool(ctx);
	mtk_cam_ctx_destroy_pool(ctx);
	mtk_cam_ctx_clean_img_pool(ctx);
	mtk_cam_ctx_clean_rgbw_caci_buf(ctx);
	mtk_cam_ctx_release_slb(ctx);
	mtk_cam_ctx_release_slc(ctx);
	if (ctx->cmdq_enabled)
		cmdq_mbox_disable(cam->cmdq_clt->chan);

	mtk_cam_ctx_destroy_workers(ctx);
	mtk_cam_ctx_pipeline_stop(ctx, entity);
	mtk_cam_pool_destroy(&ctx->job_pool);

	if (ctx->used_engine) {
		if (CAM_DEBUG_ENABLED(RAW_CG))
			pr_info("%s++:get: vcore cg/main cg0 cg1:0x%x/0x%x/0x%x", __func__,
		readl(cam->vcore_cg_con + 0x00),
		readl(cam->base + 0x00),
		readl(cam->base + 0x4c));
		mtk_cam_pm_runtime_engines(&cam->engines, ctx->used_engine, 0);
		mtk_cam_sv_set_fifo_detect_status(&cam->engines, ctx->used_engine, 1);
		if (CAM_DEBUG_ENABLED(RAW_CG))
			pr_info("%s--:get: vcore cg/main cg0 cg1:0x%x/0x%x/0x%x", __func__,
		readl(cam->vcore_cg_con + 0x00),
		readl(cam->base + 0x00),
		readl(cam->base + 0x4c));
		mtk_cam_release_engine(ctx->cam, ctx->used_engine);
	}

	ctx->used_pipe = 0;
}

int mtk_cam_ctx_init_scenario(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_raw_pipeline *raw_pipe;
	struct mtk_raw_ctrl_data *ctrl_data;
	struct mtk_cam_resource_raw_v2 *res;
	struct mtk_cam_scen *scen;
	int ret = 0;

	if (ctx->scenario_init)
		return 0;

	if (ctx->raw_subdev_idx < 0)
		return 0;

	raw_pipe = &ctx->cam->pipelines.raw[ctx->raw_subdev_idx];
	ctrl_data = &raw_pipe->ctrl_data;

	res = &ctrl_data->resource.user_data.raw_res;
	scen = &res->scen;

	if (scen_support_rgbw(scen)) {
		int sink_w, sink_h;

		sink_w = raw_pipe->pad_cfg[MTK_RAW_SINK].mbus_fmt.width;
		sink_h = raw_pipe->pad_cfg[MTK_RAW_SINK].mbus_fmt.height;

		ret = mtk_cam_ctx_alloc_rgbw_caci_buf(ctx, sink_w, sink_h);
		if (ret)
			dev_info(cam->dev, "%s: failed to alloc for caci buf\n",
				 __func__);

	} else if (ctrl_data->valid_apu_info &&
		   scen_is_m2m_apu(scen, &ctrl_data->apu_info)) {

		ret = mtk_cam_ctx_request_slb(ctx, UID_SH_P1, false, NULL);

	} else if (res_raw_is_dc_mode(res) && res->slc_mode) {
		/* dcif + slc buffer case */
		ret = mtk_cam_ctx_request_slc(ctx, res->slc_mode);
		if (ret) {
			dev_info(cam->dev, "%s: slbc_gid_request warn.\n", __func__);
			ret = mtk_cam_ctx_release_slc(ctx);
			if (ret)
				dev_info(cam->dev, "%s: slbc_gid_release warn.\n", __func__);
			ret = 0;
		}

	} else if (res_raw_is_dc_mode(res) && res->slb_size) {
		/* dcif + slb ring buffer case */

		ret = mtk_cam_ctx_request_slb(ctx, UID_SENSOR, true,
					      raw_pipe->early_request_slb_data);

		/* let ctx control the life-cyle of slb_buffer */
		mtk_raw_reset_early_slb(raw_pipe);

		if (ctx->slb_size < res->slb_size) {
			dev_info(cam->dev, "%s: warn. slb size not enough: %u<%u\n",
				 __func__, ctx->slb_size, res->slb_size);

			mtk_cam_ctx_release_slb(ctx);
			ret = 0;
		}
	}


	ctx->scenario_init = true;
	return ret;
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

int ctx_stream_on_seninf_sensor(struct mtk_cam_job *job,
				int seninf_pad_bitmask, int raw_tg_idx)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_device *cam = ctx->cam;
	struct v4l2_subdev *seninf = ctx->seninf;
	unsigned int mraw_idx, max_pixel_mode = 0;
	int cur_exp = job_exp_num(job);
	int ret;
	int i;

	if (!seninf)
		return -1;

	MTK_CAM_TRACE_BEGIN(BASIC, "%s ctx=%d", __func__, ctx->stream_id);

	if (ctx->enable_hsf_raw) {
		// ctx->used_engine: bit mask of raw/camsv/mraw
		// => raw dev id
		unsigned long raws;
		int raw_idx;

		raws = bit_map_subset_of(MAP_HW_RAW, ctx->used_engine);
		raw_idx = find_first_bit_set(raws);
		if (raw_idx >= 0) {
			ret = mtk_cam_hsf_config(ctx, raw_idx);
			if (ret != 0) {
				dev_info(cam->dev, "Error:mtk_cam_hsf_config fail\n");
				return -EPERM;
			}
			mtk_cam_seninf_set_secure(seninf, 1,
				ctx->hsf->share_buf->chunk_hsfhandle);
		}
	} else {
		mtk_cam_seninf_set_secure(seninf, 0, 0);
	}

	/* raw */
	if (raw_tg_idx >= 0 && ctx->hw_raw[0]) {
		int seninf_pad, i;
		int raw_set = MTK_SENINF_RAW_SET1;

		for (seninf_pad = PAD_SRC_RAW0, i = 0;
			  seninf_pad <= PAD_SRC_RAW2; ++seninf_pad, ++i)
			if (seninf_pad_bitmask & 1 << seninf_pad) {
				/* To-Do DCG AP merge , TCG case */
				mtk_cam_seninf_set_camtg_multiraw(
					seninf, seninf_pad, raw_tg_idx,
					raw_set++);
				mtk_cam_seninf_set_pixelmode(seninf, seninf_pad, 3);
			}
	}

	/* camsv */
	if (ctx->hw_sv) {
		struct mtk_camsv_device *sv_dev;
		int tag_idx;

		sv_dev = dev_get_drvdata(ctx->hw_sv);
		CALL_PLAT_V4L2(get_sv_max_pixel_mode, sv_dev->id, &max_pixel_mode);

		if (ctx->has_raw_subdev &&
			(scen_is_normal(&job->job_scen)) &&
			(job_prev_exp_num_seamless(job) != job_exp_num(job))) {
			/* image */
			for (i = 0; i < cur_exp; i++) {
				tag_idx = (cur_exp > 1 && (i + 1) == cur_exp) ?
					get_sv_tag_idx(cur_exp, MTKCAM_IPI_ORDER_LAST_TAG, false) :
					get_sv_tag_idx(cur_exp, i, false);

				mtk_cam_seninf_set_camtg_camsv(seninf,
					PAD_SRC_RAW0 + i,
					sv_dev->cammux_id, tag_idx);
				mtk_cam_seninf_set_pixelmode_camsv(seninf,
					PAD_SRC_RAW0 + i,
					max_pixel_mode,
					sv_dev->cammux_id);

				if (is_rgbw(job)) {
					tag_idx = (cur_exp > 1 && (i + 1) == cur_exp) ?
						get_sv_tag_idx(cur_exp, MTKCAM_IPI_ORDER_LAST_TAG, true) :
						get_sv_tag_idx(cur_exp, i, true);

					mtk_cam_seninf_set_camtg_camsv(seninf,
						PAD_SRC_RAW_W0 + i,
						sv_dev->cammux_id, tag_idx);
					mtk_cam_seninf_set_pixelmode_camsv(seninf,
						PAD_SRC_RAW_W0 + i,
						max_pixel_mode,
						sv_dev->cammux_id);
				}
			}

			/* meta */
			for (i = SVTAG_META_START; i < SVTAG_META_END; i++) {
				if (job->enabled_tags & (1 << i)) {
					mtk_cam_seninf_set_camtg_camsv(seninf,
						job->tag_info[i].seninf_padidx,
						sv_dev->cammux_id, i);
					mtk_cam_seninf_set_pixelmode_camsv(seninf,
						job->tag_info[i].seninf_padidx,
						max_pixel_mode,
						sv_dev->cammux_id);
				}
			}
		} else {
			for (i = SVTAG_START; i < SVTAG_END; i++) {
				if (job->enabled_tags & (1 << i)) {
					mtk_cam_seninf_set_camtg_camsv(seninf,
						job->tag_info[i].seninf_padidx,
						sv_dev->cammux_id, i);
					mtk_cam_seninf_set_pixelmode_camsv(seninf,
						job->tag_info[i].seninf_padidx,
						max_pixel_mode,
						sv_dev->cammux_id);
				}
			}
		}
	}

	/* mraw */
	for (i = 0; i < ctx->num_mraw_subdevs; i++) {
		mraw_idx = ctx->mraw_subdev_idx[i];
		if (cam->engines.mraw_devs[mraw_idx]) {
			struct mtk_mraw_device *mraw_dev =
				dev_get_drvdata(cam->engines.mraw_devs[mraw_idx]);
			struct mtk_mraw_pipeline *mraw_pipe =
				&cam->pipelines.mraw[ctx->mraw_subdev_idx[i]];

			mtk_cam_seninf_set_camtg(seninf,
				mraw_pipe->seninf_padidx, mraw_dev->cammux_id);
			mtk_cam_seninf_set_pixelmode(seninf,
				mraw_pipe->seninf_padidx,
				mraw_pipe->res_config.pixel_mode);
		}
	}

	ret = v4l2_subdev_call(ctx->seninf, video, s_stream, 1);
	if (ret) {
		dev_info(cam->dev, "ctx %d failed to stream_on %s %d\n",
			 ctx->stream_id, seninf->name, ret);
		return -EPERM;
	}

	MTK_CAM_TRACE_END(BASIC);
	return ret;
}

int ctx_stream_off_seninf_sensor(struct mtk_cam_ctx *ctx)
{
	int ret = 0;

	if (ctx->enable_hsf_raw) {
		ret = mtk_cam_hsf_uninit(ctx);
		if (ret != 0) {
			dev_info(ctx->cam->dev, "Error: mtk_cam_hsf_uninit fail\n");
			return -EPERM;
		}
	}

	if (!ctx->seninf)
		return ret;

	ret = v4l2_subdev_call(ctx->seninf, video, s_stream, 0);
	if (ret) {
		dev_info(ctx->cam->dev, "ctx %d failed to stream_off %s %d\n",
			 ctx->stream_id, ctx->seninf->name, ret);
		return -EPERM;
	}

	return ret;
}

int mtk_cam_ctx_stream_on(struct mtk_cam_ctx *ctx)
{
	int ret;

	if (CAM_DEBUG_ENABLED(V4L2))
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
	mtk_cam_dvfs_update(&ctx->cam->dvfs, ctx->stream_id, 0, 0);

	return 0;
}

static void mtk_cam_ctx_raw_qof_disable(struct mtk_cam_ctx *ctx)
{
	int i;
	struct mtk_raw_device *raw;
	struct mtk_camsv_device *sv;

	qof_mtcmos_voter_handle(&ctx->cam->engines, 0, &ctx->DOL_not_support);
#ifdef QOF_CCU_READY
	mtk_cam_power_ctrl_ccu(ctx->cam->dev, 1);
#endif
	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (!ctx->hw_raw[i])
			continue;

		raw = dev_get_drvdata(ctx->hw_raw[i]);
		qof_setup_twin(raw, true, false);
		qof_enable(raw, false);
		if (ctx->hw_sv) {
			sv = dev_get_drvdata(ctx->hw_sv);
			mtk_cam_sv_set_queue_mode(sv, false);
		}
	}
#ifdef QOF_CCU_READY
	mtk_cam_power_ctrl_ccu(ctx->cam->dev, 0);
#endif

	qof_reset_mtcmos_voter(ctx);
	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (!ctx->hw_raw[i])
			continue;
		raw = dev_get_drvdata(ctx->hw_raw[i]);
		qof_reset(raw);
	}
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

	qof_mtcmos_voter(&ctx->cam->engines, ctx->used_engine, true);

	if (ctx->hw_sv) {
		sv_dev = dev_get_drvdata(ctx->hw_sv);
		mtk_cam_sv_dev_stream_on(sv_dev, false, 0, 0);
	}

	for (i = 0 ; i < ARRAY_SIZE(ctx->hw_mraw); i++) {
		if (ctx->hw_mraw[i]) {
			mraw_dev = dev_get_drvdata(ctx->hw_mraw[i]);
			mtk_cam_mraw_dev_stream_on(mraw_dev, false);
		}
	}

	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);

			if (raw_dev->is_slave)
				continue;
			if (raw_dev->is_timeshared) {
				if (!atomic_sub_and_test(1, &raw_dev->time_share_used)) {
					dev_info(raw_dev->dev, "time-share: ctx:%d return pass uninitialize",
						ctx->stream_id);
					continue;
				}
				raw_dev->is_timeshared = false;
				atomic_set(&raw_dev->time_share_on_process, 0);
				dev_info(raw_dev->dev, "time-share: ctx:%d last uninitialize",
						ctx->stream_id);
			}
			if (ctx->enable_hsf_raw)
				ccu_stream_on(ctx, false);
			else
				stream_on(raw_dev, false, true);
		}
	}

	mtk_cam_ctx_raw_qof_disable(ctx);

	if (ctx->set_adl_aid) {
		mtk_cam_hsf_aid(ctx, 0, AID_VAINR, ctx->used_engine);
		ctx->set_adl_aid = 0;
	}
}

/* note: only raw switch using */
void mtk_cam_ctx_engine_enable_irq(struct mtk_cam_ctx *ctx)
{
	struct mtk_raw_device *raw_dev;
	struct mtk_camsv_device *sv_dev;
	struct mtk_mraw_device *mraw_dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);
			enable_irq(raw_dev->irq);
		}
	}

	if (ctx->hw_sv) {
		sv_dev = dev_get_drvdata(ctx->hw_sv);
		for (i = 0; i < ARRAY_SIZE(sv_dev->irq); i++)
			enable_irq(sv_dev->irq[i]);
	}

	for (i = 0; i < ARRAY_SIZE(ctx->hw_mraw); i++) {
		if (ctx->hw_mraw[i]) {
			mraw_dev = dev_get_drvdata(ctx->hw_mraw[i]);
			enable_irq(mraw_dev->irq);
		}
	}
}

void mtk_cam_ctx_engine_disable_irq(struct mtk_cam_ctx *ctx)
{
	struct mtk_raw_device *raw_dev;
	struct mtk_camsv_device *sv_dev;
	struct mtk_mraw_device *mraw_dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);
			disable_irq(raw_dev->irq);
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

void mtk_cam_ctx_engine_reset_msgfifo(struct mtk_cam_ctx *ctx)
{
	struct mtk_raw_device *raw_dev;
	struct mtk_camsv_device *sv_dev;
	struct mtk_mraw_device *mraw_dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);
			mtk_cam_raw_reset_msgfifo(raw_dev);
		}
	}

	if (ctx->hw_sv) {
		sv_dev = dev_get_drvdata(ctx->hw_sv);
		for (i = 0; i < ARRAY_SIZE(sv_dev->irq); i++)
			mtk_cam_sv_reset_msgfifo(sv_dev);
	}

	for (i = 0; i < ARRAY_SIZE(ctx->hw_mraw); i++) {
		if (ctx->hw_mraw[i]) {
			mraw_dev = dev_get_drvdata(ctx->hw_mraw[i]);
			mtk_cam_mraw_reset_msgfifo(mraw_dev);
		}
	}
}

void mtk_cam_ctx_engine_clear(struct mtk_cam_ctx *ctx)
{
	struct mtk_raw_device *raw_dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);
			clear_reg(raw_dev);
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

void mtk_cam_ctx_engine_dc_sw_recovery(struct mtk_cam_ctx *ctx)
{
	struct mtk_raw_device *raw_dev;
	struct mtk_camsv_device *sv_dev;
	int i;

	// disable vf: camsv -> raw
	if (ctx->hw_sv) {
		sv_dev = dev_get_drvdata(ctx->hw_sv);

		mtk_cam_sv_backup(sv_dev);
		mtk_cam_sv_dev_stream_on(sv_dev, 0, 0, 0);
	}

	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);
			if (raw_dev->is_slave)
				continue;

			stream_on(raw_dev, 0, 0);
		}
	}

	// reset
	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);
			reset(raw_dev);
		}
	}

	/* camsv already reset in stream off */

	// enable vf: raw -> camsv
	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);

			toggle_db(raw_dev);

			if (raw_dev->is_slave)
				continue;

			stream_on(raw_dev, 1, 0);
		}
	}

	if (ctx->hw_sv) {
		sv_dev = dev_get_drvdata(ctx->hw_sv);
		mtk_cam_sv_dev_config(sv_dev, 0, -1);
		mtk_cam_sv_restore(sv_dev);
		mtk_cam_sv_dev_stream_on(sv_dev, 1,
					 ctx->enabled_tags, ctx->used_tag_cnt);
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

static int ctx_kthread_queue_work(struct mtk_cam_ctx *ctx,
				  struct kthread_worker *worker,
				  struct kthread_work *work,
				  const char *caller)
{
	int ret;

	ret = kthread_queue_work(worker, work) ? 0 : -1;
	if (ret)
		pr_info("%s: ctx-%d failed\n", caller, ctx->stream_id);
	return ret;
}

int mtk_cam_ctx_queue_sensor_worker(struct mtk_cam_ctx *ctx,
				    struct kthread_work *work)
{
	return ctx_kthread_queue_work(ctx, &ctx->sensor_worker, work, __func__);
}

int mtk_cam_ctx_queue_done_worker(struct mtk_cam_ctx *ctx,
				  struct kthread_work *work)
{
	return ctx_kthread_queue_work(ctx, &ctx->done_worker, work, __func__);
}

int mtk_cam_ctx_queue_flow_worker(struct mtk_cam_ctx *ctx,
				  struct kthread_work *work)
{
	return ctx_kthread_queue_work(ctx, &ctx->flow_worker, work, __func__);
}

int mtk_cam_ctx_queue_tuning_worker(struct mtk_cam_ctx *ctx,
				  struct kthread_work *work)
{
	return ctx_kthread_queue_work(ctx, &ctx->tuning_worker, work, __func__);
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

	if (CAM_DEBUG_ENABLED(V4L2))
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

	//dev_info(dev, "create pad link %s %s\n", seninf->name, pipe->name);
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

		for (j = PAD_SRC_RAW_W0; j <= PAD_SRC_RAW_W2; j++) {
			ret = _dynamic_link_seninf_pipe(dev,
							&seninf->entity,
							j,
							pipe_entity,
							MTK_CAMSV_SINK);
			if (ret)
				return ret;

		}

		for (j = PAD_SRC_META0; j <= PAD_SRC_META1; j++) {
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
	int ret = 0;

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

	ret = mtk_raw_setup_dependencies(cam_dev);
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

	mutex_init(&cam_dev->ccu_lock);

	mtk_cam_dvfs_probe(cam_dev->dev,
			   &cam_dev->dvfs, cam_dev->max_stream_num);

	mtk_raw_hdr_tsfifo_init(cam_dev->pipelines.raw,
					cam_dev->pipelines.num_raw);

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
	int num = eng->num_raw_devices * 3 /* raw + yuv + rms */
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

	eng->rms_devs = dev_arr;
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
	int yuv_num, rms_num, camsv_real_hw_num;

	eng->num_raw_devices =
		add_match_by_driver(dev, &match, &mtk_cam_raw_driver);

	yuv_num = add_match_by_driver(dev, &match, &mtk_cam_yuv_driver);

	rms_num = add_match_by_driver(dev, &match, &mtk_cam_rms_driver);

	eng->num_larb_devices =
		add_match_by_driver(dev, &match, &mtk_cam_larb_driver);

	camsv_real_hw_num =
		add_match_by_driver(dev, &match, &mtk_cam_sv_driver);

	eng->num_camsv_devices = MAX_SV_HW_NUM;

	eng->num_mraw_devices =
		add_match_by_driver(dev, &match, &mtk_cam_mraw_driver);

	eng->num_seninf_devices =
		add_match_by_driver(dev, &match, &seninf_pdrv);

	if (GET_PLAT_HW(bwr_support))
		add_match_by_driver(dev, &match, &mtk_cam_bwr_driver);

	if (IS_ERR(match) || mtk_cam_alloc_for_engine(dev))
		mtk_cam_match_remove(dev);

	dev_info(dev, "#: raw %d yuv %d rms %d larb %d, sv %d, seninf %d, mraw %d\n",
		 eng->num_raw_devices, yuv_num, rms_num,
		 eng->num_larb_devices,
		 camsv_real_hw_num,
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
struct tag_chipid {
	u32 size;
	u32 hw_code;
	u32 hw_subcode;
	u32 hw_ver;
	u32 sw_ver;
};
void mtk_cam_get_chipid(struct mtk_cam_device *cam)
{
	struct device_node *node;
	struct tag_chipid *chip_id = NULL;
	int len;

	node = of_find_node_by_path("/chosen");
	if (!node)
		node = of_find_node_by_path("/chosen@0");
	if (node) {
		chip_id = (struct tag_chipid *) of_get_property(node, "atag,chipid", &len);
		if (!chip_id)
			pr_info("could not found atag,chipid in chosen\n");
	} else {
		pr_info("chosen node not found in device tree\n");
	}
	if (chip_id)
		cam->sw_ver = chip_id->sw_ver;
	dev_info(cam->dev, "current sw version:0x%x\n", cam->sw_ver);
}

int mtk_cam_update_engine_status(struct mtk_cam_device *cam,
				 unsigned long engine_mask,
				 bool available)
{
	unsigned long err_mask, occupied;
	unsigned long pass_check;

	spin_lock(&cam->streaming_lock);

	occupied = cam->engines.occupied_engine;
	pass_check = cam->engines.timeshared_engine;
	if (available) {
		err_mask = (occupied & engine_mask) ^ engine_mask;
		occupied &= ~engine_mask;
	} else {
		err_mask = occupied & engine_mask;
		err_mask &= ~pass_check;
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
	if (CAM_DEBUG_ENABLED(V4L2_TRY))
		dev_info(cam->dev, "%s: mark engine 0x%lx available %d\n",
		 __func__, engine_mask, available);
	return 0;
}

int mtk_cam_sv_set_fifo_detect_status(struct mtk_cam_engines *eng,
			    unsigned long engine_mask, unsigned int is_hsf_enable)
{
	unsigned long submask;
	int i;
	struct mtk_camsv_device *sv_dev;

	submask = bit_map_subset_of(MAP_HW_CAMSV, engine_mask);
	for (i = 0; i < eng->num_camsv_devices && submask; i++, submask >>= 1) {
		if (!(submask & 0x1))
			continue;
		sv_dev = dev_get_drvdata(eng->sv_devs[i]);
		if (is_hsf_enable)
			atomic_set(&sv_dev->enable_fifo_detect, 0);
		else
			atomic_set(&sv_dev->enable_fifo_detect, 1);
	}
	return 0;
}


static int loop_each_engine(struct mtk_cam_engines *eng,
			    unsigned long engine_mask,
			    int (*func)(struct device *dev), int enable)
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
		loop_each_engine(eng, engine_mask, pm_runtime_get_sync, enable);
	else
		loop_each_engine(eng, engine_mask, pm_runtime_put_sync, enable);

	return 0;
}

static int loop_each_engine_rms(struct mtk_cam_engines *eng,
			    unsigned long engine_mask,
			    int (*func)(struct device *dev), int enable)
{

	unsigned long submask;
	int i;


	submask = bit_map_subset_of(MAP_HW_RAW, engine_mask);
	for (i = 0; i < eng->num_raw_devices && submask; i++, submask >>= 1) {
		if (!(submask & 0x1))
			continue;
		func(eng->rms_devs[i]);
	}

	return 0;
}

int mtk_cam_pm_runtime_rms_engines(
	struct mtk_cam_ctx *ctx, struct mtk_cam_engines *eng,
	unsigned long engine_mask, int enable)
{
	if (enable) {
		if (rms_freerun) {
			loop_each_engine_rms(eng, engine_mask, pm_runtime_get, enable);
			pr_info("%s:get: engine_mask:0x%lx", __func__, engine_mask);
		}

		ctx->rms_disable = 0;
	} else {
		if (rms_freerun) {
			loop_each_engine_rms(eng, engine_mask, pm_runtime_put, enable);
			pr_info("%s:put: engine_mask:0x%lx", __func__, engine_mask);
		}
		ctx->rms_disable = 1;
	}
	return 0;
}

static int disable_camctl3_mod_en_reset_bpc2_pcrp(struct device *dev)
{
	struct mtk_rms_device *rms = dev_get_drvdata(dev);
	struct mtk_cam_device *cam = rms->cam;
	struct mtk_raw_device *raw;

	raw = dev_get_drvdata(cam->engines.rms_devs[rms->id]);
	diable_rms_module(raw);
	return 0;
}

void clear_camctl3_mod_en(struct mtk_cam_ctx *ctx,
		struct mtk_cam_engines *eng, unsigned long engine_mask)
{
	loop_each_engine_rms(eng, engine_mask, disable_camctl3_mod_en_reset_bpc2_pcrp, true);
}

static int disable_pcrp(struct device *dev)
{
	struct mtk_rms_device *rms = dev_get_drvdata(dev);
	struct mtk_cam_device *cam = rms->cam;
	struct mtk_raw_device *raw;

	raw = dev_get_drvdata(cam->engines.rms_devs[rms->id]);
	diable_rms_pcrp(raw);
	return 0;
}

void clear_pcrp(struct mtk_cam_ctx *ctx,
		struct mtk_cam_engines *eng, unsigned long engine_mask)
{
	loop_each_engine_rms(eng, engine_mask, disable_pcrp, true);
}

void mtk_engine_dump_debug_status(struct mtk_cam_device *cam,
				  unsigned long engines, int dma_debug_dump)
{
	struct mtk_raw_device *dev;
	struct mtk_camsv_device *sv_dev;
	struct mtk_mraw_device *mraw_dev;
	unsigned long subset;
	int i;
	bool need_smi_dump = false;

	subset = bit_map_subset_of(MAP_HW_RAW, engines);
	for (i = 0; i < cam->engines.num_raw_devices; i++) {

		if (subset & BIT(i)) {
			dev = dev_get_drvdata(cam->engines.raw_devs[i]);

			need_smi_dump |= raw_dump_debug_status(dev, dma_debug_dump);
		}
	}

	subset = bit_map_subset_of(MAP_HW_CAMSV, engines);
	for (i = 0; i < cam->engines.num_camsv_devices; i++) {

		if (subset & BIT(i)) {
			sv_dev = dev_get_drvdata(cam->engines.sv_devs[i]);

			need_smi_dump |= mtk_cam_sv_debug_dump(sv_dev, 0);
		}
	}

	subset = bit_map_subset_of(MAP_HW_MRAW, engines);
	for (i = 0; i < cam->engines.num_mraw_devices; i++) {

		if (subset & BIT(i)) {
			mraw_dev = dev_get_drvdata(cam->engines.mraw_devs[i]);

			mtk_cam_mraw_debug_dump(mraw_dev);
		}
	}

	if(need_smi_dump)
		mtk_smi_dbg_hang_detect("camsys");
}

static int register_sub_drivers(struct device *dev)
{
	struct component_match *match = NULL;
	int ret;

	ret = platform_driver_register(&mtk_cam_larb_driver);
	if (ret) {
		dev_err(dev, "%s mtk_cam_larb_driver fail\n", __func__);
		goto REGISTER_LARB_FAIL;
	}

	ret = platform_driver_register(&seninf_pdrv);
	if (ret) {
		dev_err(dev, "%s seninf_pdrv fail\n", __func__);
		goto REGISTER_SENINF_FAIL;
	}

	ret = platform_driver_register(&seninf_core_pdrv);
	if (ret) {
		dev_err(dev, "%s seninf_core_pdrv fail\n", __func__);
		goto REGISTER_SENINF_CORE_FAIL;
	}

	ret = platform_driver_register(&mtk_cam_sv_driver);
	if (ret) {
		dev_err(dev, "%s mtk_cam_sv_driver fail\n", __func__);
		goto REGISTER_CAMSV_FAIL;
	}

	ret = platform_driver_register(&mtk_cam_mraw_driver);
	if (ret) {
		dev_err(dev, "%s mtk_cam_mraw_driver fail\n", __func__);
		goto REGISTER_MRAW_FAIL;
	}

	ret = platform_driver_register(&mtk_cam_raw_driver);
	if (ret) {
		dev_err(dev, "%s mtk_cam_raw_driver fail\n", __func__);
		goto REGISTER_RAW_FAIL;
	}

	ret = platform_driver_register(&mtk_cam_yuv_driver);
	if (ret) {
		dev_err(dev, "%s mtk_cam_raw_driver fail\n", __func__);
		goto REGISTER_YUV_FAIL;
	}

	ret = platform_driver_register(&mtk_cam_rms_driver);
	if (ret) {
		dev_err(dev, "%s mtk_cam_rms_driver fail\n", __func__);
		goto REGISTER_RMS_FAIL;
	}

	if (GET_PLAT_HW(bwr_support)) {
		ret = platform_driver_register(&mtk_cam_bwr_driver);
		if (ret) {
			dev_err(dev, "%s mtk_cam_bwr_driver fail\n", __func__);
			goto REGISTER_BWR_FAIL;
		}
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
	if (GET_PLAT_HW(bwr_support))
		platform_driver_unregister(&mtk_cam_bwr_driver);

REGISTER_BWR_FAIL:
	platform_driver_unregister(&mtk_cam_rms_driver);

REGISTER_RMS_FAIL:
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
	platform_driver_unregister(&mtk_cam_larb_driver);

REGISTER_LARB_FAIL:
	return ret;
}

static irqreturn_t mtk_irq_adlrd(int irq, void *data)
{
	struct mtk_cam_device *drvdata = (struct mtk_cam_device *)data;
	struct device *dev = drvdata->dev;

	unsigned int irq_status;

	irq_status = readl_relaxed(drvdata->adlrd_base + 0x08a4);

	dev_info(dev, "ADL-INT: INT 0x%x\n", irq_status);

	return IRQ_HANDLED;
}
static irqreturn_t mtk_irq_qof(int irq, void *data)
{
	struct mtk_cam_device *drvdata = (struct mtk_cam_device *)data;
	struct device *dev = drvdata->dev;
	struct mtk_cam_engines *eng = &drvdata->engines;
	int i;

	unsigned int int_status;

	int_status = readl_relaxed(
		drvdata->qoftop_base + REG_QOF_CAM_TOP_QOF_INT_STATUS);

	// TODO: consider trigger KE or fix state machine
	dev_info(dev, "qof: QOFTOP-INT: INT 0x%x\n", int_status);

	for (i = 0; i < eng->num_raw_devices; ++i) {
		struct mtk_raw_device *raw = dev_get_drvdata(eng->raw_devs[i]);

		if (int_status & FBIT(QOF_CAM_TOP_MTC_CYC_OV_INT_ST_1) && raw->id == 0) {
			qof_dump_voter(raw);
			qof_dump_power_state(raw);
			qof_dump_hw_timer(raw);
			dev_info(dev, "qof: QOF_CAM_TOP_MTC_CYC_OV_INT_ST: raw %u\n", raw->id);
			qof_set_force_dump(raw, true);
			qof_dump_trigger_cnt(raw);
			qof_dump_voter(raw);
			qof_dump_hw_timer(raw);
			qof_dump_ctrl(raw);
			qof_set_force_dump(raw, false);
		} else if (int_status & FBIT(QOF_CAM_TOP_MTC_CYC_OV_INT_ST_2) && raw->id == 1) {
			qof_dump_voter(raw);
			qof_dump_power_state(raw);
			qof_dump_hw_timer(raw);
			dev_info(dev, "qof: QOF_CAM_TOP_MTC_CYC_OV_INT_ST: raw %u\n", raw->id);
			qof_set_force_dump(raw, true);
			qof_dump_trigger_cnt(raw);
			qof_dump_voter(raw);
			qof_dump_hw_timer(raw);
			qof_dump_ctrl(raw);
			qof_set_force_dump(raw, false);
		} else if (int_status & FBIT(QOF_CAM_TOP_MTC_CYC_OV_INT_ST_3) && raw->id == 2) {
			qof_dump_voter(raw);
			qof_dump_power_state(raw);
			qof_dump_hw_timer(raw);
			dev_info(dev, "qof: QOF_CAM_TOP_MTC_CYC_OV_INT_ST: raw %u\n", raw->id);
			qof_set_force_dump(raw, true);
			qof_dump_trigger_cnt(raw);
			qof_dump_voter(raw);
			qof_dump_hw_timer(raw);
			qof_dump_ctrl(raw);
			qof_set_force_dump(raw, false);
		}
	}

	return IRQ_HANDLED;
}
static int mtk_cam_vcore_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_cam_vcore_device *drvdata;
	struct device *alloc_dev;
	int i, ret, clks;

	dev_info(dev, "%s++\n", __func__);

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;
	if (smmu_v3_enabled()) {
		drvdata->smmu_dev_acp = mtk_smmu_get_shared_device(&pdev->dev);
		if (!drvdata->smmu_dev_acp) {
			dev_err(dev, "%s: Get SMMU Device failed\n", __func__);
			WRAP_AEE_EXCEPTION("mtk_cam_probe", "Get SMMU Device");
			return -ENODEV;
		}
		dev_info(dev, "[%s] find smmu_dev_acp cam vcore dev %p\n",
			__func__, drvdata->smmu_dev_acp);
	}

	alloc_dev = drvdata->smmu_dev_acp ? : dev;
	if (dma_set_mask_and_coherent(alloc_dev, DMA_BIT_MASK(34)))
		dev_err(dev, "%s: No suitable DMA available\n", __func__);

	if (!alloc_dev->dma_parms) {
		alloc_dev->dma_parms =
			devm_kzalloc(alloc_dev, sizeof(*alloc_dev->dma_parms), GFP_KERNEL);
		if (!dev->dma_parms) {
			dev_err(dev, "%s: kzalloc alloc_dev->dma_parms failed\n", __func__);
			WRAP_AEE_EXCEPTION("mtk_cam_vcore_probe", "Kzalloc");
			return -ENOMEM;
		}
	}

	if (alloc_dev->dma_parms) {
		ret = dma_set_max_seg_size(alloc_dev, UINT_MAX);
		if (ret)
			dev_err(dev, "%s: Failed to set DMA segment size\n", __func__);
	}
	clks = of_count_phandle_with_args(
				pdev->dev.of_node, "clocks", "#clock-cells");
	drvdata->num_clks = (clks == -ENOENT) ? 0 : clks;
	dev_info(dev, "clk_num:%d\n", drvdata->num_clks);

	if (drvdata->num_clks) {
		drvdata->clks = devm_kcalloc(
				dev, drvdata->num_clks, sizeof(*drvdata->clks), GFP_KERNEL);
		if (!drvdata->clks)
			return -ENOMEM;
	}

	for (i = 0; i < drvdata->num_clks; i++) {
		drvdata->clks[i] = of_clk_get(pdev->dev.of_node, i);
		if (IS_ERR(drvdata->clks[i])) {
			dev_info(dev, "failed to get clk %d\n", i);
			return -ENODEV;
		}
	}

	drvdata->dev = &pdev->dev;
	dev_set_drvdata(dev, drvdata);

	pm_runtime_enable(dev);

	return 0;
}

static int mtk_cam_vcore_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pm_runtime_disable(dev);

	return 0;
}

static int mtk_cam_vcore_runtime_suspend(struct device *dev)
{
	struct mtk_cam_vcore_device *cam_vcore  = dev_get_drvdata(dev);
	int i;
	if (CAM_DEBUG_ENABLED(RAW_CG))
		dev_info(dev, "- %s\n", __func__);
	for (i = cam_vcore->num_clks - 1; i >= 0; i--)
		clk_disable_unprepare(cam_vcore->clks[i]);

	return 0;
}

static int mtk_cam_vcore_runtime_resume(struct device *dev)
{
	struct mtk_cam_vcore_device *cam_vcore  = dev_get_drvdata(dev);
	int i, ret;

	for (i = 0; i < cam_vcore->num_clks; i++) {
		ret = clk_prepare_enable(cam_vcore->clks[i]);
		if (ret)
			dev_info(dev, "%s:i=%d check this", __func__, i);
	}

	return 0;
}

static int mtk_cam_probe(struct platform_device *pdev)
{
	struct platform_device *vcore_pdev;
	struct mtk_cam_device *cam_dev;
	struct mtk_cam_vcore_device *cam_vcore_dev;
	struct device *dev = &pdev->dev;
	struct device *alloc_dev;
	struct device_node *node;
	struct device_link *link;
	int ret;
	unsigned int i, clks;
	struct resource *res_base;
	const struct camsys_platform_data *platform_data;
	int irq;
	int cam_vcore_base,
	cam_main_rawa_base, cam_main_rawb_base, cam_main_rawc_base,
	cam_main_rmsa_base, cam_main_rmsb_base, cam_main_rmsc_base,
	cam_main_yuva_base, cam_main_yuvb_base, cam_main_yuvc_base;

	//dev_info(dev, "%s\n", __func__);
	platform_data = of_device_get_match_data(dev);
	if (!platform_data) {
		dev_err(dev, "%s: of_device_get_match_data failed\n", __func__);
		WRAP_AEE_EXCEPTION("mtk_cam_probe", "Get Match Data");
		return -ENODEV;
	}
	set_platform_data(platform_data);
	dev_info(dev, "platform = %s\n", platform_data->platform);

	camsys_root_dev = dev;

	/* initialize module base address */
	CALL_PLAT_HW(query_module_base, CAM_VCORE, &cam_vcore_base);
	CALL_PLAT_HW(query_module_base, CAM_MAIN_RAWA, &cam_main_rawa_base);
	CALL_PLAT_HW(query_module_base, CAM_MAIN_RAWB, &cam_main_rawb_base);
	CALL_PLAT_HW(query_module_base, CAM_MAIN_RAWC, &cam_main_rawc_base);
	CALL_PLAT_HW(query_module_base, CAM_MAIN_RMSA, &cam_main_rmsa_base);
	CALL_PLAT_HW(query_module_base, CAM_MAIN_RMSB, &cam_main_rmsb_base);
	CALL_PLAT_HW(query_module_base, CAM_MAIN_RMSC, &cam_main_rmsc_base);
	CALL_PLAT_HW(query_module_base, CAM_MAIN_YUVA, &cam_main_yuva_base);
	CALL_PLAT_HW(query_module_base, CAM_MAIN_YUVB, &cam_main_yuvb_base);
	CALL_PLAT_HW(query_module_base, CAM_MAIN_YUVC, &cam_main_yuvc_base);

	/* initialize structure */
	cam_dev = devm_kzalloc(dev, sizeof(*cam_dev), GFP_KERNEL);
	if (!cam_dev) {
		dev_err(dev, "%s: kzalloc cam_dev failed\n", __func__);
		WRAP_AEE_EXCEPTION("mtk_cam_probe", "Kzalloc");
		return -ENOMEM;
	}

	if (smmu_v3_enabled()) {
		cam_dev->smmu_dev = mtk_smmu_get_shared_device(&pdev->dev);
		if (!cam_dev->smmu_dev) {
			dev_err(dev, "%s: Get SMMU Device failed\n", __func__);
			WRAP_AEE_EXCEPTION("mtk_cam_probe", "Get SMMU Device");
			return -ENODEV;
		}
	}

	alloc_dev = cam_dev->smmu_dev ? : dev;
	if (dma_set_mask_and_coherent(alloc_dev, DMA_BIT_MASK(34)))
		dev_err(dev, "%s: No suitable DMA available\n", __func__);

	if (!alloc_dev->dma_parms) {
		alloc_dev->dma_parms =
			devm_kzalloc(alloc_dev, sizeof(*alloc_dev->dma_parms), GFP_KERNEL);
		if (!dev->dma_parms) {
			dev_err(dev, "%s: kzalloc alloc_dev->dma_parms failed\n", __func__);
			WRAP_AEE_EXCEPTION("mtk_cam_probe", "Kzalloc");
			return -ENOMEM;
		}
	}

	if (alloc_dev->dma_parms) {
		ret = dma_set_max_seg_size(alloc_dev, UINT_MAX);
		if (ret)
			dev_err(dev, "%s: Failed to set DMA segment size\n", __func__);
	}

	cam_dev->base =  devm_platform_ioremap_resource_byname(pdev, "base");
	if (IS_ERR(cam_dev->base)) {
		dev_err(dev, "%s: ioremap base failed\n", __func__);
		WRAP_AEE_EXCEPTION("mtk_cam_probe", "ioremap base");
		return PTR_ERR(cam_dev->base);
	}

	res_base = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base");
	if (!res_base) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}
	cam_dev->base_reg_addr = res_base->start;

	cam_dev->adlwr_base = devm_platform_ioremap_resource_byname(pdev, "adlwr");
	if (IS_ERR(cam_dev->adlwr_base)) {
		dev_err(dev, "%s: failed to map adlwr_base\n", __func__);
		cam_dev->adlwr_base = NULL;
	}

	cam_dev->adlrd_base = devm_platform_ioremap_resource_byname(pdev, "adlrd");
	if (IS_ERR(cam_dev->adlrd_base)) {
		dev_err(dev, "%s: failed to map adlrd_base\n", __func__);
		cam_dev->adlrd_base = NULL;
	}

	cam_dev->qoftop_base = devm_platform_ioremap_resource_byname(pdev, "qof_base");
	if (IS_ERR(cam_dev->qoftop_base)) {
		dev_err(dev, "%s: failed to map qoftop_base\n", __func__);
		cam_dev->qoftop_base = NULL;
	}

	cam_dev->vcore_ddren_en = ioremap(cam_vcore_base + CAM_VCORE_DDREN_EN, 0x4);
	if (IS_ERR(cam_dev->vcore_ddren_en)) {
		dev_err(dev, "%s: failed to map vcore_ddren_en\n", __func__);
		cam_dev->vcore_ddren_en = NULL;
	}

	cam_dev->vcore_ddren_ack = ioremap(cam_vcore_base + CAM_VCORE_DDREN_ACK, 0x4);
	if (IS_ERR(cam_dev->vcore_ddren_ack)) {
		dev_err(dev, "%s: failed to map vcore_ddren_ack\n", __func__);
		cam_dev->vcore_ddren_ack = NULL;
	}

	cam_dev->vcore_cg_con = ioremap(cam_vcore_base + CAM_VCORE_CG_CON, 0x4);
	if (IS_ERR(cam_dev->vcore_cg_con)) {
		dev_err(dev, "%s: failed to map vcore_cg_con\n", __func__);
		cam_dev->vcore_cg_con = NULL;
	}
	cam_dev->rawa_cg_con = ioremap(cam_main_rawa_base, 0xc);
	if (IS_ERR(cam_dev->rawa_cg_con)) {
		dev_err(dev, "%s: failed to map rawa_cg_con\n", __func__);
		cam_dev->rawa_cg_con = NULL;
	}

	cam_dev->rawb_cg_con = ioremap(cam_main_rawb_base, 0xc);
	if (IS_ERR(cam_dev->rawb_cg_con)) {
		dev_err(dev, "%s: failed to map rawb_cg_con\n", __func__);
		cam_dev->rawb_cg_con = NULL;
	}

	cam_dev->rawc_cg_con = ioremap(cam_main_rawc_base, 0xc);
	if (IS_ERR(cam_dev->rawc_cg_con)) {
		dev_err(dev, "%s: failed to map rawc_cg_con\n", __func__);
		cam_dev->rawc_cg_con = NULL;
	}

	cam_dev->rmsa_cg_con = ioremap(cam_main_rmsa_base, 0xc);
	if (IS_ERR(cam_dev->rmsa_cg_con)) {
		dev_err(dev, "%s: failed to map rmsa_cg_con\n", __func__);
		cam_dev->rmsa_cg_con = NULL;
	}

	cam_dev->rmsb_cg_con = ioremap(cam_main_rmsb_base, 0xc);
	if (IS_ERR(cam_dev->rmsb_cg_con)) {
		dev_err(dev, "%s: failed to map rmsb_cg_con\n", __func__);
		cam_dev->rmsb_cg_con = NULL;
	}

	cam_dev->rmsc_cg_con = ioremap(cam_main_rmsc_base, 0xc);
	if (IS_ERR(cam_dev->rmsc_cg_con)) {
		dev_err(dev, "%s: failed to map rmsc_cg_con\n", __func__);
		cam_dev->rmsc_cg_con = NULL;
	}

	cam_dev->yuva_cg_con = ioremap(cam_main_yuva_base, 0xc);
	if (IS_ERR(cam_dev->yuva_cg_con)) {
		dev_err(dev, "%s: failed to map yuva_cg_con\n", __func__);
		cam_dev->yuva_cg_con = NULL;
	}

	cam_dev->yuvb_cg_con = ioremap(cam_main_yuvb_base, 0xc);
	if (IS_ERR(cam_dev->yuvb_cg_con)) {
		dev_err(dev, "%s: failed to map yuvb_cg_con\n", __func__);
		cam_dev->yuvb_cg_con = NULL;
	}

	cam_dev->yuvc_cg_con = ioremap(cam_main_yuvc_base, 0xc);
	if (IS_ERR(cam_dev->yuvc_cg_con)) {
		dev_err(dev, "%s: failed to map yuvc_cg_con\n", __func__);
		cam_dev->yuvc_cg_con = NULL;
	}

	// adlrd_rdone
	irq = platform_get_irq_byname(pdev, "adlrd_rdone");
	if (irq < 0) {
		dev_err(dev, "%s: failed to get adlrd_rdone irq\n", __func__);
		goto SKIP_ADLRD_IRQ;
	}

	ret = devm_request_irq(dev, irq, mtk_irq_adlrd, IRQF_NO_AUTOEN,
			       dev_name(dev), cam_dev);
	if (ret) {
		dev_err(dev, "%s: Request adlrd_rdone failed\n", __func__);
		WRAP_AEE_EXCEPTION("mtk_cam_probe", "Request IRQF_NO_AUTOEN");
		return ret;
	}
	dev_dbg(dev, "registered adlrd_rdone irq=%d\n", irq);
	//enable_irq(irq);

	// adlrd
	irq = platform_get_irq_byname(pdev, "adlrd");
	if (irq < 0) {
		dev_err(dev, "%s: failed to get adlrd irq\n", __func__);
		goto SKIP_ADLRD_IRQ;
	}

	ret = devm_request_irq(dev, irq, mtk_irq_adlrd, IRQF_NO_AUTOEN,
			       dev_name(dev), cam_dev);
	if (ret) {
		dev_err(dev, "%s: Request adlrd failed\n", __func__);
		WRAP_AEE_EXCEPTION("mtk_cam_probe", "Request IRQF_NO_AUTOEN");
		return ret;
	}
	dev_dbg(dev, "registered adlrd irq=%d\n", irq);
	//enable_irq(irq);

	// qof
	irq = platform_get_irq_byname(pdev, "qoftop");
	if (irq < 0) {
		dev_err(dev, "%s: failed to get qoftop irq\n", __func__);
		goto SKIP_ADLRD_IRQ;
	}

	cam_dev->qoftop_irq = irq;
	ret = devm_request_irq(dev, cam_dev->qoftop_irq, mtk_irq_qof, IRQF_NO_AUTOEN,
			       dev_name(dev), cam_dev);
	if (ret) {
		dev_err(dev, "%s: Request qoftop failed\n", __func__);
		WRAP_AEE_EXCEPTION("mtk_cam_probe", "Request IRQF_NO_AUTOEN");
		return ret;
	}
	dev_dbg(dev, "registered qoftop irq=%d\n", cam_dev->qoftop_irq);

	cam_dev->cmdq_clt = cmdq_mbox_create(dev, 0);
	if (!cam_dev->cmdq_clt)
		pr_err("probe cmdq_mbox_create fail\n");

	clks = of_count_phandle_with_args(
					pdev->dev.of_node, "clocks", "#clock-cells");
	cam_dev->num_clks = (clks == -ENOENT) ? 0 : clks;
	dev_info(dev, "clk_num:%d\n", cam_dev->num_clks);

	if (cam_dev->num_clks) {
		cam_dev->clks = devm_kcalloc(
					dev, cam_dev->num_clks, sizeof(*cam_dev->clks), GFP_KERNEL);
		if (!cam_dev->clks)
			return -ENOMEM;
	}

	for (i = 0; i < cam_dev->num_clks; i++) {
		cam_dev->clks[i] = of_clk_get(pdev->dev.of_node, i);
		if (IS_ERR(cam_dev->clks[i])) {
			dev_info(dev, "failed to get clk %d\n", i);
			return -ENODEV;
		}
	}

	/* cam_vcore */
	node = of_parse_phandle(
				pdev->dev.of_node, "mediatek,camisp-vcore", 0);
	if (!node) {
		dev_info(dev, "failed to get camisp vcore phandle\n");
		return -ENODEV;
	}

	vcore_pdev = of_find_device_by_node(node);
	if (WARN_ON(!vcore_pdev)) {
		of_node_put(node);
		dev_info(dev, "failed to get camisp vcore pdev\n");
		return -ENODEV;
	}
	of_node_put(node);

	link = device_link_add(dev, &vcore_pdev->dev,
					DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
	if (!link)
		dev_info(dev, "unable to link cam vcore\n");
	cam_vcore_dev  = dev_get_drvdata(&vcore_pdev->dev);
	if (cam_vcore_dev && cam_vcore_dev->smmu_dev_acp) {
		cam_dev->smmu_dev_acp = cam_vcore_dev->smmu_dev_acp;
		dev_info(cam_vcore_dev->dev, "[%s] find smmu_dev_acp cam vcore dev %p\n",
			__func__, cam_dev->smmu_dev_acp);
	}

SKIP_ADLRD_IRQ:
	cam_dev->dev = dev;
	dev_set_drvdata(dev, cam_dev);

	/* FIXME: decide max raw stream num by seninf num */
	cam_dev->max_stream_num = 8; /* TODO: how */
	cam_dev->ctxs = devm_kcalloc(dev, cam_dev->max_stream_num,
				     sizeof(*cam_dev->ctxs), GFP_KERNEL);
	if (!cam_dev->ctxs) {
		dev_err(dev, "%s: kcalloc cam_dev->ctxs failed\n", __func__);
		WRAP_AEE_EXCEPTION("mtk_cam_probe", "Kcalloc");
		return -ENOMEM;
	}

	for (i = 0; i < cam_dev->max_stream_num; i++)
		mtk_cam_ctx_init(cam_dev->ctxs + i, cam_dev, i);

	spin_lock_init(&cam_dev->streaming_lock);
	spin_lock_init(&cam_dev->pending_job_lock);
	spin_lock_init(&cam_dev->running_job_lock);
	INIT_LIST_HEAD(&cam_dev->pending_job_list);
	INIT_LIST_HEAD(&cam_dev->running_job_list);

	ret = mtk_cam_of_rproc(cam_dev);
	if (ret)
		goto fail_return;

	ret = register_sub_drivers(dev);
	if (ret) {
		dev_err(dev, "%s: fail to register_sub_drivers\n", __func__);
		goto fail_return;
	}

	/* after v4l2_device_register to avoid get_sync/put_sync ops */
	/* for freerun mtcmos/cg check by ccf */
	pm_runtime_enable(dev);

	mtk_cam_debug_init(&cam_dev->dbg, cam_dev);
	init_waitqueue_head(&cam_dev->shutdown_wq);

	mtk_cam_get_chipid(cam_dev);
	mtk_cam_tuning_probe();

	return 0;

fail_return:
	dev_err(dev, "%s: fail_return\n", __func__);
	WRAP_AEE_EXCEPTION("mtk_cam_probe", "fail_return");

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
	platform_driver_unregister(&mtk_cam_larb_driver);
	platform_driver_unregister(&seninf_core_pdrv);
	platform_driver_unregister(&seninf_pdrv);
	if (GET_PLAT_HW(bwr_support))
		platform_driver_unregister(&mtk_cam_bwr_driver);

	return 0;
}

#define SHUTDOWN_TIMEOUT 10000
static void mtk_cam_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_cam_device *cam  = dev_get_drvdata(dev);
	long timeout;

	dev_info(dev, "%s: shutdown\n", __func__);

	timeout = wait_event_timeout(cam->shutdown_wq,
				 !atomic_read(&cam->initialize_cnt),
				 msecs_to_jiffies(SHUTDOWN_TIMEOUT));

	if (timeout == 0) {
		pr_info("%s: wait for shutdown %dms timeout\n",
			__func__, SHUTDOWN_TIMEOUT);

		//todo: stop all ctx
	}
}

static int mtk_cam_runtime_suspend(struct device *dev)
{
	struct mtk_cam_device *cam_dev  = dev_get_drvdata(dev);
	int i;

	dev_info(dev, "%s:suspend\n", __func__);

	disable_irq(cam_dev->qoftop_irq);
	mtk_cam_bwr_disable(cam_dev->bwr);
	mtk_cam_plat_resource_ctrl(cam_dev, 0);

	if (CAM_DEBUG_ENABLED(RAW_CG))
		dev_dbg(dev, "%s++:get: vcore cg/main cg0 cg1:0x%x/0x%x/0x%x", __func__,
		readl(cam_dev->vcore_cg_con + 0x00),
		readl(cam_dev->base + 0x00),
		readl(cam_dev->base + 0x4c));
	for (i = cam_dev->num_clks - 1; i >= 0; i--)
		clk_disable_unprepare(cam_dev->clks[i]);

	if (CAM_DEBUG_ENABLED(RAW_CG))
		dev_dbg(dev, "%s--:get: vcore cg/main cg0 cg1:0x%x/0x%x/0x%x", __func__,
		readl(cam_dev->vcore_cg_con + 0x00),
		readl(cam_dev->base + 0x00),
		readl(cam_dev->base + 0x4c));

	return 0;
}

static void init_camsys_main_adl_setting(struct mtk_cam_device *cam_dev)
{

	/* CAM_MAIN_ADLWR_CTRL set RAWA/B/C CQ to super priority */
	writel_relaxed(0xe0, cam_dev->base + 0x328);

	/* CAM_MAIN_DRZB2N_RAW_SEL */
	writel_relaxed(0, cam_dev->base + 0x3ac);

	/* CAM_MAIN_DRZB2N_HDR_RAW_SEL */
	writel_relaxed(0, cam_dev->base + 0x3b0);

	/* CAM_MAIN_DRZB2N_SRC[1-3]_SEL */
	writel_relaxed(0, cam_dev->base + 0x3b4);
	writel_relaxed(0, cam_dev->base + 0x3b8);
	writel_relaxed(0, cam_dev->base + 0x3bc);
}

static int mtk_cam_runtime_resume(struct device *dev)
{
	struct mtk_cam_device *cam_dev  = dev_get_drvdata(dev);
	int i, ret;

	dev_info(dev, "%s: resume\n", __func__);
	if (CAM_DEBUG_ENABLED(RAW_CG))
		dev_dbg(dev, "%s++:get: vcore cg/main cg0 cg1:0x%x/0x%x/0x%x", __func__,
		readl(cam_dev->vcore_cg_con + 0x00),
		readl(cam_dev->base + 0x00),
		readl(cam_dev->base + 0x4c));
	for (i = 0; i < cam_dev->num_clks; i++) {
		ret = clk_prepare_enable(cam_dev->clks[i]);
		if (ret)
			dev_info(dev, "%s:i=%d check this", __func__, i);
	}
	if (CAM_DEBUG_ENABLED(RAW_CG))
		dev_dbg(dev,"%s--:get: vcore cg/main cg0 cg1:0x%x/0x%x/0x%x", __func__,
		readl(cam_dev->vcore_cg_con + 0x00),
		readl(cam_dev->base + 0x00),
		readl(cam_dev->base + 0x4c));

	init_camsys_main_adl_setting(cam_dev);
	mtk_cam_timesync_init(true);

	mtk_cam_plat_resource_ctrl(cam_dev, 1);
	mtk_cam_bwr_enable(cam_dev->bwr);

	if (GET_PLAT_HW(qof_support))
		mtk_cam_reset_itc(cam_dev);

	enable_irq(cam_dev->qoftop_irq);

	return 0;
}

static const struct dev_pm_ops mtk_cam_vcore_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_cam_vcore_runtime_suspend,
					   mtk_cam_vcore_runtime_resume, NULL)
};

static struct platform_driver mtk_cam_vcore_driver = {
	.probe   = mtk_cam_vcore_probe,
	.remove  = mtk_cam_vcore_remove,
	.driver  = {
		.name  = "mtk-cam-vcore",
		.of_match_table = of_match_ptr(mtk_cam_vcore_of_ids),
		.pm     = &mtk_cam_vcore_pm_ops,
	}
};
static const struct dev_pm_ops mtk_cam_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_cam_runtime_suspend, mtk_cam_runtime_resume,
			   NULL)
};

static struct platform_driver mtk_cam_driver = {
	.probe   = mtk_cam_probe,
	.remove  = mtk_cam_remove,
	.shutdown  = mtk_cam_shutdown,
	.driver  = {
		.name  = "mtk-cam",
		.of_match_table = of_match_ptr(mtk_cam_of_ids),
		.pm     = &mtk_cam_pm_ops,
	}
};

static int __init mtk_cam_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_cam_vcore_driver);
	if (ret)
		pr_info("%s camisp vcore register fail\n", __func__);

	ret = platform_driver_register(&mtk_cam_driver);
	if (ret)
		pr_info("%s camisp register fail\n", __func__);

	return ret;
}

static void __exit mtk_cam_exit(void)
{
	platform_driver_unregister(&mtk_cam_driver);
	platform_driver_unregister(&mtk_cam_vcore_driver);
}

bool mtk_cam_is_dcif_slb_supported(void)
{
	static int is_supported;
	struct slbc_data slb;
	int ret;

	if (is_supported != 0)
		return is_supported > 0;

	if (!GET_PLAT_HW(dcif_slb_support)) {
		is_supported = -1;
		pr_info("%s: disabled\n", __func__);
		return 0;
	}

	slb.uid = UID_SENSOR;
	slb.type = TP_BUFFER;

	ret = slbc_status(&slb);
	is_supported = ret < 0 ? -1 : 1;

	if (is_supported < 0)
		pr_info("%s: not supported\n", __func__);

	return is_supported > 0;
}

bool mtk_cam_ctx_is_raw_sink_changed(struct mtk_cam_ctx *ctx,
				     struct mtk_raw_sink_data *sink)
{
	struct mtk_raw_sink_data *config_sink;
	bool changed;

	if (!sink)
		return false;

	config_sink = &ctx->configured_raw_sink;

	changed = !is_raw_sink_eq(config_sink, sink);
	if (changed)
		dev_info(ctx->cam->dev, "%s: changed\n", __func__);

	return changed;
}

module_init(mtk_cam_init);
module_exit(mtk_cam_exit);

MODULE_DESCRIPTION("Camera ISP driver");
MODULE_LICENSE("GPL");
