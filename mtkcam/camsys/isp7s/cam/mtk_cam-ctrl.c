// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/list.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>

#include <media/v4l2-event.h>
#include <media/v4l2-subdev.h>

#include "mtk_cam.h"
#include "mtk_cam-ctrl.h"
#include "mtk_cam-debug.h"
#include "mtk_cam-dvfs_qos.h"
#include "mtk_cam-hsf.h"
#include "mtk_cam-pool.h"
//#include "mtk_cam-tg-flash.h"
#include "mtk_camera-v4l2-controls.h"
#include "mtk_camera-videodev2.h"
#include "mtk_cam-trace.h"
#include "mtk_cam-job_utils.h"

#define WATCHDOG_INTERVAL_MS		400
#define WATCHDOG_MAX_HWTIME_MS		400
#define WATCHDOG_MAX_SENSOR_RETRY_CNT	3

unsigned long engine_idx_to_bit(int engine_type, int idx)
{
	unsigned int map_hw = 0;

	if (engine_type == CAMSYS_ENGINE_RAW)
		map_hw = MAP_HW_RAW;
	else if (engine_type == CAMSYS_ENGINE_MRAW)
		map_hw = MAP_HW_MRAW;
	else if (engine_type == CAMSYS_ENGINE_CAMSV)
		map_hw = MAP_HW_CAMSV;

	return bit_map_bit(map_hw, idx);
}

static int mtk_cam_ctrl_get(struct mtk_cam_ctrl *cam_ctrl)
{
	atomic_inc(&cam_ctrl->ref_cnt);

	if (unlikely(atomic_read(&cam_ctrl->stopped))) {
		if (atomic_dec_and_test(&cam_ctrl->ref_cnt))
			wake_up_interruptible(&cam_ctrl->stop_wq);
		return -1;
	}

	return 0;
}

static int mtk_cam_ctrl_put(struct mtk_cam_ctrl *cam_ctrl)
{
	if (atomic_dec_and_test(&cam_ctrl->ref_cnt))
		if (unlikely(atomic_read(&cam_ctrl->stopped)))
			wake_up_interruptible(&cam_ctrl->stop_wq);
	return 0;
}

/*
 * this function is blocked until no one could successfully do ctrl_get
 */
static int mtk_cam_ctrl_wait_all_released(struct mtk_cam_ctrl *cam_ctrl)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;

	dev_info(ctx->cam->dev, "[%s] ctx:%d waiting\n",
		 __func__, ctx->stream_id);

	wait_event_interruptible(cam_ctrl->stop_wq,
				 !atomic_read(&cam_ctrl->ref_cnt));
	return 0;
}

bool cond_first_job(struct mtk_cam_job *job, void *arg)
{
	return 1;
}

bool cond_frame_no_belong(struct mtk_cam_job *job, void *arg)
{
	int no = *(int *)arg;

	return frame_seq_diff(no, job->frame_seq_no) < job->frame_cnt;
}

bool cond_switch_job_first(struct mtk_cam_job *job, void *arg)
{
	return job->seamless_switch;
}

bool cond_job_with_action(struct mtk_cam_job *job, void *arg)
{
	return mtk_cam_job_has_pending_action(job);
}

static int ctrl_enable_job_fsm(struct mtk_cam_job *job, void *arg)
{
	if (CAM_DEBUG_ENABLED(CTRL))
		pr_info("%s: #%d\n", __func__,  job->req_seq);
	mtk_cam_job_set_fsm(job, 1);
	return 0;
}

typedef int (*for_each_func_t)(struct mtk_cam_job *, void *);
static int mtk_cam_ctrl_loop_job(struct mtk_cam_ctrl *ctrl,
				 for_each_func_t func, void *arg)
{
	struct mtk_cam_job *job;
	struct mtk_cam_job_state *state;
	int ret = 0;

	read_lock(&ctrl->list_lock);

	list_for_each_entry(state, &ctrl->camsys_state_list, list) {
		job = container_of(state, struct mtk_cam_job, job_state);

		/* skip if return non-zero */
		ret = ret || func(job, arg);
	}

	read_unlock(&ctrl->list_lock);

	return ret;
}

typedef bool (*cond_func_t)(struct mtk_cam_job *, void *);
static struct mtk_cam_job *mtk_cam_ctrl_get_job(struct mtk_cam_ctrl *ctrl,
						cond_func_t cond_func,
						void *arg)
{
	struct mtk_cam_job *job;
	struct mtk_cam_job_state *state;
	bool found = 0;

	read_lock(&ctrl->list_lock);

	list_for_each_entry(state, &ctrl->camsys_state_list, list) {
		job = container_of(state, struct mtk_cam_job, job_state);

		found = cond_func(job, arg);
		if (found) {
			job = mtk_cam_job_get(job);
			break;
		}
	}

	read_unlock(&ctrl->list_lock);

	return found ? job : NULL;
}

static void log_event(const char *func, int ctx_id, struct v4l2_event *e)
{
	switch (e->type) {
	case V4L2_EVENT_EOS:
		pr_info("%s: ctx-%d\n", func, ctx_id);
		break;
	case V4L2_EVENT_FRAME_SYNC:
	case V4L2_EVENT_REQUEST_DUMPED:
		pr_info("%s: ctx-%d seq %u\n", func, ctx_id,
			e->u.frame_sync.frame_sequence);
		break;
	case V4L2_EVENT_ERROR:
		pr_info("%s: ctx-%d %s\n", func, ctx_id, e->u.data);
		break;
	default:
		pr_info("%s: ctx-%d event type %d\n", func, ctx_id, e->type);
		break;
	}
}

static void mtk_cam_event_eos(struct mtk_cam_ctrl *cam_ctrl)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;
	struct v4l2_event event = {
		.type = V4L2_EVENT_EOS,
	};

	if (ctx->has_raw_subdev)
		mtk_cam_ctx_send_raw_event(ctx, &event);
	else
		mtk_cam_ctx_send_sv_event(ctx, &event);

	if (CAM_DEBUG_ENABLED(V4L2_EVENT))
		log_event(__func__, ctx->stream_id, &event);
}

void mtk_cam_event_frame_sync(struct mtk_cam_ctrl *cam_ctrl,
			      unsigned int frame_seq_no)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;
	struct v4l2_event event = {
		.type = V4L2_EVENT_FRAME_SYNC,
		.u.frame_sync.frame_sequence = frame_seq_no,
	};

	if (ctx->has_raw_subdev)
		mtk_cam_ctx_send_raw_event(ctx, &event);
	else
		mtk_cam_ctx_send_sv_event(ctx, &event);

	if (CAM_DEBUG_ENABLED(V4L2_EVENT))
		log_event(__func__, ctx->stream_id, &event);
}

void mtk_cam_event_request_dumped(struct mtk_cam_ctrl *cam_ctrl,
				  unsigned int frame_seq_no)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;
	struct v4l2_event event = {
		.type = V4L2_EVENT_REQUEST_DUMPED,
		.u.frame_sync.frame_sequence = frame_seq_no,
	};

	if (ctx->has_raw_subdev)
		mtk_cam_ctx_send_raw_event(ctx, &event);

	if (CAM_DEBUG_ENABLED(V4L2_EVENT))
		log_event(__func__, ctx->stream_id, &event);
}

void mtk_cam_event_error(struct mtk_cam_ctrl *cam_ctrl, const char *msg)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;
	struct v4l2_event event = {
		.type = V4L2_EVENT_ERROR,
	};

	strncpy(event.u.data, msg, min(strlen(msg), sizeof(event.u.data)));
	event.u.data[63] = '\0';

	if (ctx->has_raw_subdev)
		mtk_cam_ctx_send_raw_event(ctx, &event);

	if (CAM_DEBUG_ENABLED(V4L2_EVENT))
		log_event(__func__, ctx->stream_id, &event);
}

static void dump_runtime_info(struct mtk_cam_ctrl_runtime_info *info)
{
	pr_info("[%s] ack 0x%x out/in 0x%x/0x%x\n", __func__,
		info->ack_seq_no, info->outer_seq_no, info->inner_seq_no);
	pr_info("[%s] sof_ts_ns %lld\n", __func__, info->sof_ts_ns);
}

static void ctrl_send_event(struct mtk_cam_ctrl *ctrl,
			    struct transition_param *p)
{
	struct mtk_cam_job_state *state;

	MTK_CAM_TRACE_FUNC_BEGIN(BASIC);

	read_lock(&ctrl->list_lock);
	spin_lock(&ctrl->send_lock);
	list_for_each_entry(state, &ctrl->camsys_state_list, list) {
		state->ops->send_event(state, p);
	}
	spin_unlock(&ctrl->send_lock);
	read_unlock(&ctrl->list_lock);

	MTK_CAM_TRACE_END(BASIC);
}

static int ctrl_apply_actions(struct mtk_cam_ctrl *ctrl)
{
	struct mtk_cam_job *job;

	MTK_CAM_TRACE_FUNC_BEGIN(BASIC);

	do {
		job = mtk_cam_ctrl_get_job(ctrl, cond_job_with_action, NULL);
		if (job) {
			mtk_cam_job_apply_pending_action(job);
			mtk_cam_job_put(job);
		}
	} while (job);

	MTK_CAM_TRACE_END(BASIC);

	return 0;
}

static void debug_send_event(const struct transition_param *p)
{
	struct mtk_cam_ctrl_runtime_info *info;
	bool print_ts;

	info = p->info;

	print_ts = (p->event == CAMSYS_EVENT_ENQUE);

	if (print_ts)
		pr_info("[%s] out/in:0x%x/0x%x event: %s@%llu (sof %llu)\n",
			__func__,
			info->outer_seq_no, info->inner_seq_no,
			str_event(p->event),
			p->event_ts, info->sof_ts_ns);
	else
		pr_info("[%s] out/in:0x%x/0x%x event: %s\n",
			__func__,
			info->outer_seq_no, info->inner_seq_no,
			str_event(p->event));
}

static int mtk_cam_ctrl_send_event(struct mtk_cam_ctrl *ctrl, int event)
{
	struct mtk_cam_ctrl_runtime_info local_info;
	struct transition_param p;

	spin_lock(&ctrl->info_lock);
	local_info = ctrl->r_info;
	spin_unlock(&ctrl->info_lock);

	p.head = &ctrl->camsys_state_list;
	p.info = &local_info;
	p.event = event;
	p.event_ts = ktime_get_boottime_ns();
	p.s_params = &ctrl->s_params;

	if (0 && CAM_DEBUG_ENABLED(STATE))
		dump_runtime_info(p.info);

	if (CAM_DEBUG_ENABLED(STATE))
		debug_send_event(&p);


	ctrl_send_event(ctrl, &p);

	ctrl_apply_actions(ctrl);

	return 0;
}

static void handle_setting_done(struct mtk_cam_ctrl *cam_ctrl)
{
	mtk_cam_ctrl_send_event(cam_ctrl, CAMSYS_EVENT_IRQ_L_CQ_DONE);
}

static void handle_meta1_done(struct mtk_cam_ctrl *ctrl, int seq_no)
{
	struct mtk_cam_job *job;

	job = mtk_cam_ctrl_get_job(ctrl, cond_frame_no_belong, &seq_no);

	if (!job) {
		pr_info("%s: warn. job not found seq 0x%x\n",
			__func__,  seq_no);
		return;
	}

	call_jobop(job, mark_afo_done, seq_no);

	mtk_cam_job_put(job);
}

static void handle_frame_done(struct mtk_cam_ctrl *ctrl,
			      int engine_type, int engine_id,
			      int seq_no)
{
	struct mtk_cam_job *job;

	job = mtk_cam_ctrl_get_job(ctrl, cond_frame_no_belong, &seq_no);

	/*
	 *
	 * 1. handle each done => mark_engine_done
	 *      TODO: check state
	 *      if in wrong state, force transit?
	 * 2. check if is last done,
	 * 3. send_event(IRQ_FRAME_DONE)
	 *    need to send_event IRQ_FRAME_DONE for m2m trigger
	 */
	if (!job) {
		pr_info("%s: warn. job not found seq 0x%x\n",
			__func__,  seq_no);
		return;
	}

	if (call_jobop(job, mark_engine_done,
		       engine_type, engine_id, seq_no)) {

		/* last done: trigger FSM */
		mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_IRQ_FRAME_DONE);
	}

	mtk_cam_job_put(job);
}
static void handle_ss_try_set_sensor(struct mtk_cam_ctrl *cam_ctrl)
{
	mtk_cam_ctrl_send_event(cam_ctrl, CAMSYS_EVENT_TIMER_SENSOR);
}

static void ctrl_vsync_preprocess(struct mtk_cam_ctrl *ctrl,
				  enum MTK_CAMSYS_ENGINE_TYPE engine_type,
				  unsigned int engine_id,
				  struct mtk_camsys_irq_info *irq_info,
				  struct vsync_result *vsync_res)
{
	bool hint_inner_err = 0;
	bool complete_vsync = false;
	int cookie;

	if (vsync_update(&ctrl->vsync_col, engine_type, engine_id, vsync_res))
		return;

	spin_lock(&ctrl->info_lock);

	if (vsync_res->is_first)
		ctrl->r_info.sof_ts_ns = irq_info->ts_ns;

	if (vsync_res->is_last) {

		if (ctrl->cur_cq_ref) {
			struct apply_cq_ref *cq_ref = ctrl->cur_cq_ref;

			if (apply_cq_ref_is_to_inner(cq_ref)) {
				ctrl->r_info.inner_seq_no =
					seq_from_fh_cookie(cq_ref->cookie);
				ctrl->cur_cq_ref = 0;
			} else {
				hint_inner_err = 1;
				cookie = cq_ref->cookie;
			}
		}

		if (atomic_read(&ctrl->await_switching_seq)
		    == ctrl->r_info.inner_seq_no)
			complete_vsync = 1;
	}

	spin_unlock(&ctrl->info_lock);

	if (complete_vsync) {
		pr_info("%s: signal for switching\n", __func__);
		complete(&ctrl->vsync_complete);
	}

	if (hint_inner_err)
		pr_info("%s: inner not updated to 0x%x\n", __func__, cookie);
}

static int frame_no_to_fs_req_no(struct mtk_cam_ctrl *ctrl, int frame_no,
				 int *req_no)
{
	struct mtk_cam_job *job;
	int do_send_evnt;

	job = mtk_cam_ctrl_get_job(ctrl, cond_frame_no_belong, &frame_no);
	if (job) {

		if (ctrl->frame_sync_event_cnt != job->req_seq) {
			ctrl->fs_event_subframe_cnt = job->frame_cnt;
			ctrl->fs_event_subframe_idx = 0;
		}

		ctrl->frame_sync_event_cnt = job->req_seq;

		mtk_cam_job_put(job);
	}

	do_send_evnt = ctrl->fs_event_subframe_idx == 0;
	ctrl->fs_event_subframe_idx =
		(ctrl->fs_event_subframe_idx + 1) % ctrl->fs_event_subframe_cnt;

	if (req_no)
		*req_no = ctrl->frame_sync_event_cnt;

	return do_send_evnt;
}

static void handle_engine_frame_start(struct mtk_cam_ctrl *ctrl,
				      struct mtk_camsys_irq_info *irq_info,
				      bool is_first, bool is_last)
{

	if (is_first) {
		int frame_sync_no;
		int req_no;

		frame_sync_no = seq_from_fh_cookie(irq_info->frame_idx_inner);

		if (frame_no_to_fs_req_no(ctrl, frame_sync_no, &req_no))
			mtk_cam_event_frame_sync(ctrl, req_no);

		mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_IRQ_F_VSYNC);
	}

	if (is_last)
		mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_IRQ_L_SOF);

}

static int mtk_cam_event_handle_raw(struct mtk_cam_ctrl *ctrl,
				       unsigned int engine_id,
				       struct mtk_camsys_irq_info *irq_info)
{

	MTK_CAM_TRACE_FUNC_BEGIN(BASIC);

	/* raw's DMA done, we only allow AFO done here */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_AFO_DONE))
		handle_meta1_done(ctrl,
				  seq_from_fh_cookie(irq_info->cookie_done));

	/* raw's SW done */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_DONE))
		handle_frame_done(ctrl,
				  CAMSYS_ENGINE_RAW, engine_id,
				  seq_from_fh_cookie(irq_info->cookie_done));

	/* raw's subsample n-2 vsync coming */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_TRY_SENSOR_SET))
		handle_ss_try_set_sensor(ctrl);

	/* raw's SOF (proc engine frame start) */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_START)) {
		struct vsync_result vsync_res;

		ctrl_vsync_preprocess(ctrl,
				      CAMSYS_ENGINE_RAW, engine_id, irq_info,
				      &vsync_res);

		handle_engine_frame_start(ctrl, irq_info,
					  vsync_res.is_first,
					  vsync_res.is_last);
	}

	/* note: should handle SOF before CQ done for trigger delay cases */
	/* raw's CQ done */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_SETTING_DONE)) {
		spin_lock(&ctrl->info_lock);
		ctrl->r_info.outer_seq_no =
			seq_from_fh_cookie(irq_info->frame_idx);
		spin_unlock(&ctrl->info_lock);

		handle_setting_done(ctrl);
	}

	/* DCIF' SOF (dc link engine frame start (first exposure) ) */
	//if (irq_info->irq_type & (1 << CAMSYS_IRQ_FRAME_START_DCIF_MAIN)) {
		// handle_dcif_frame_start(); - TBC
	//}

	MTK_CAM_TRACE_END(BASIC);
	return 0;
}

static int mtk_camsys_event_handle_camsv(struct mtk_cam_ctrl *ctrl,
				       unsigned int engine_id,
				       struct mtk_camsys_irq_info *irq_info)
{

	/* camsv's SW done */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_DONE))
		handle_frame_done(ctrl,
				  CAMSYS_ENGINE_CAMSV, engine_id,
				  seq_from_fh_cookie(irq_info->cookie_done));

	/* camsv's SOF (proc engine frame start) */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_START)) {
		struct vsync_result vsync_res;

		ctrl_vsync_preprocess(ctrl,
				      CAMSYS_ENGINE_CAMSV, engine_id, irq_info,
				      &vsync_res);

		handle_engine_frame_start(ctrl, irq_info,
					  vsync_res.is_first,
					  vsync_res.is_last);
	}

	/* note: should handle SOF before CQ done for trigger delay cases */
	/* camsv's CQ done */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_SETTING_DONE)) {
		spin_lock(&ctrl->info_lock);
		ctrl->r_info.outer_seq_no =
			seq_from_fh_cookie(irq_info->frame_idx);
		spin_unlock(&ctrl->info_lock);
		handle_setting_done(ctrl);
	}

	return 0;
}

static int mtk_camsys_event_handle_mraw(struct mtk_cam_ctrl *ctrl,
					unsigned int engine_id,
					struct mtk_camsys_irq_info *irq_info)
{

	/* mraw's SW done */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_DONE))
		handle_frame_done(ctrl,
				  CAMSYS_ENGINE_MRAW, engine_id,
				  seq_from_fh_cookie(irq_info->cookie_done));

	/* mraw's SOF (proc engine frame start) */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_START)) {
		struct vsync_result vsync_res;

		ctrl_vsync_preprocess(ctrl,
				      CAMSYS_ENGINE_MRAW, engine_id, irq_info,
				      &vsync_res);

		handle_engine_frame_start(ctrl, irq_info,
					  vsync_res.is_first,
					  vsync_res.is_last);
	}

	/* note: should handle SOF before CQ done for trigger delay cases */
	/* mraw's CQ done */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_SETTING_DONE)) {
		spin_lock(&ctrl->info_lock);
		ctrl->r_info.outer_seq_no =
			seq_from_fh_cookie(irq_info->frame_idx);
		spin_unlock(&ctrl->info_lock);
		handle_setting_done(ctrl);
	}

	return 0;
}

int mtk_cam_ctrl_isr_event(struct mtk_cam_device *cam,
			   int engine_type, unsigned int engine_id,
			   struct mtk_camsys_irq_info *irq_info)
{
	unsigned int ctx_id = ctx_from_fh_cookie(irq_info->frame_idx);
	struct mtk_cam_ctrl *cam_ctrl = &cam->ctxs[ctx_id].cam_ctrl;
	int ret = 0;

	if (mtk_cam_ctrl_get(cam_ctrl))
		return 0;

	/* TBC
	 *  MTK_CAM_TRACE_BEGIN(BASIC, "irq_type %d, inner %d",
	 *  irq_info->irq_type, irq_info->frame_idx_inner);
	 */
	/**
	 * Here it will be implemented dispatch rules for some scenarios
	 * like twin/stagger/m-stream,
	 * such cases that camsys will collect all coworked sub-engine's
	 * signals and trigger some engine of them to do some job
	 * individually.
	 * twin - rawx2
	 * stagger - rawx1, camsv x2
	 * m-stream - rawx1 , camsv x2
	 */

	switch (engine_type) {
	case CAMSYS_ENGINE_RAW:
		ret = mtk_cam_event_handle_raw(cam_ctrl, engine_id, irq_info);
		break;
	case CAMSYS_ENGINE_MRAW:
		ret = mtk_camsys_event_handle_mraw(cam_ctrl, engine_id, irq_info);
		break;
	case CAMSYS_ENGINE_CAMSV:
		ret = mtk_camsys_event_handle_camsv(cam_ctrl, engine_id, irq_info);
		break;
	case CAMSYS_ENGINE_SENINF:
		/* ToDo - cam mux setting delay handling */
		if (irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_DROP))
			dev_info(cam->dev, "MTK_CAMSYS_ENGINE_SENINF_TAG engine:%d type:0x%x\n",
				engine_id, irq_info->irq_type);
		break;
	default:
		break;
	}

	mtk_cam_ctrl_put(cam_ctrl);

	/* TBC
	 * MTK_CAM_TRACE_END(BASIC);
	 */
	return ret;
}

static u64 query_interval_from_sensor(struct v4l2_subdev *sensor)
{
	struct v4l2_subdev_frame_interval fi; /* in seconds */
	u64 frame_interval_ns;

	if (!sensor) {
		pr_info("%s: warn. without sensor\n", __func__);
		return 0;
	}

	memset(&fi, 0, sizeof(fi));

	fi.pad = 0;
	v4l2_subdev_call(sensor, video, g_frame_interval, &fi);

	if (fi.interval.denominator)
		frame_interval_ns = (fi.interval.numerator * 1000000000ULL) /
			fi.interval.denominator;
	else {
		pr_info("%s: warn. wrong fi (%u/%u)\n", __func__,
			fi.interval.numerator,
			fi.interval.denominator);
		frame_interval_ns = 1000000000ULL / 30ULL;
	}

	pr_info("%s: fi %llu ns\n", __func__, frame_interval_ns);
	return frame_interval_ns;
}

static void mtk_cam_ctrl_stream_on_work(struct work_struct *work)
{
	struct mtk_cam_sys_wq_work *sys_wq_work =
		container_of(work, struct mtk_cam_sys_wq_work, work);
	struct mtk_cam_ctx *ctx = mtk_cam_wq_work_get_ctx(sys_wq_work);
	struct mtk_cam_ctrl *ctrl = &ctx->cam_ctrl;
	struct mtk_cam_job *job = sys_wq_work->job;
	struct device *dev = ctx->cam->dev;
	unsigned long timeout = msecs_to_jiffies(1000);

	dev_info(dev, "[%s] ctx %d begin\n", __func__, ctrl->ctx->stream_id);

	if (!job)
		return;

	mtk_cam_job_state_set(&job->job_state, SENSOR_STATE, S_SENSOR_APPLYING);
	call_jobop(job, apply_sensor);
	mtk_cam_job_state_set(&job->job_state, SENSOR_STATE, S_SENSOR_LATCHED);

	if (!wait_for_completion_timeout(&job->compose_completion, timeout)) {
		pr_info("[%s] error: wait for job composed timeout\n",
			__func__);
		return;
	}

	mtk_cam_job_state_set(&job->job_state, ISP_STATE, S_ISP_APPLYING);
	call_jobop(job, apply_isp);

	if (!wait_for_completion_timeout(&job->cq_exe_completion, timeout)) {
		pr_info("[%s] error: wait for job cq exe\n",
			__func__);
		return;
	}

	ctrl->s_params.i2c_thres_ns =
		infer_i2c_deadline_ns(&job->job_scen,
				      query_interval_from_sensor(ctx->sensor));
	dev_info(dev, "%s: i2c thres %llu\n",
		 __func__, ctrl->s_params.i2c_thres_ns);

	/* should set ts for second job's apply_sensor */
	ctrl->r_info.sof_ts_ns = ktime_get_boottime_ns();

	call_jobop(job, stream_on, true);

	mtk_cam_watchdog_start(&ctrl->watchdog, 1);

	/* non multi-frame job: e.g., mstream */
	if (job->frame_cnt > 1) {
		mtk_cam_job_state_set(&job->job_state,
				      SENSOR_2ND_STATE, S_SENSOR_APPLYING);
		call_jobop(job, apply_sensor);

	} else {
		int seq;

		seq = next_frame_seq(job->frame_seq_no);
		job = mtk_cam_ctrl_get_job(ctrl, cond_frame_no_belong, &seq);
		if (job) {
			mtk_cam_job_state_set(&job->job_state,
					      SENSOR_STATE, S_SENSOR_APPLYING);
			call_jobop(job, apply_sensor);
			mtk_cam_job_put(job);
		}
	}

	atomic_set(&ctrl->stream_on_done, 1);
	mtk_cam_ctrl_loop_job(ctrl,  ctrl_enable_job_fsm, NULL);

	dev_info(dev, "[%s] ctx %d finish\n", __func__, ctrl->ctx->stream_id);
}

/*
 * note: this threshold due to
 * 1. having a stagger sensor, it takes 10ms between 1st and 2nd vsync
 * 2. little margin for sw: 3ms
 */
#define VALID_SWITCH_PERIOD_FROM_VSYNC_MS	13
static int check_valid_sync(struct mtk_cam_ctrl *ctrl, int seq)
{
	u64 first_sof_ts;
	int inner_seq;

	spin_lock(&ctrl->info_lock);
	inner_seq = ctrl->r_info.inner_seq_no;
	first_sof_ts = ctrl->r_info.sof_ts_ns;
	spin_unlock(&ctrl->info_lock);

	if (inner_seq == seq) {
		u64 ts = ktime_get_boottime_ns();

		return ts - first_sof_ts <
			VALID_SWITCH_PERIOD_FROM_VSYNC_MS * 1000000;
	}

	return 0;
}

/*
 * wait until prev_seq is in inner (updated at last vsync)
 */
static int mtk_cam_ctrl_wait_for_switch(struct mtk_cam_ctrl *ctrl,
					int prev_seq)
{
	unsigned long timeout = msecs_to_jiffies(1000);

	/*
	 * case 1: prev_seq is in inner and vsync has came already
	 * case 2: prev_seq is not innner
	 */
	if (check_valid_sync(ctrl, prev_seq)) {
		pr_info("%s: vsync has came\n", __func__);
		return 0;
	}

	pr_info("%s: wait for next vsync\n", __func__);

	atomic_set(&ctrl->await_switching_seq, prev_seq);
	reinit_completion(&ctrl->vsync_complete);

	if (!wait_for_completion_timeout(&ctrl->vsync_complete, timeout)) {
		pr_info("[%s] error: wait for vsync timeout\n",
			__func__);
		return -1;
	}
	/* reset */
	atomic_set(&ctrl->await_switching_seq, -1);

	return 0;
}

static void mtk_cam_ctrl_seamless_switch_work(struct work_struct *work)
{
	struct mtk_cam_sys_wq_work *sys_wq_work =
		container_of(work, struct mtk_cam_sys_wq_work, work);
	struct mtk_cam_ctx *ctx = mtk_cam_wq_work_get_ctx(sys_wq_work);
	struct mtk_cam_ctrl *ctrl = &ctx->cam_ctrl;
	struct mtk_cam_job *job = sys_wq_work->job;
	struct device *dev = ctx->cam->dev;

	unsigned long timeout = msecs_to_jiffies(200);

	if (!job)
		return;

	if (mtk_cam_ctrl_wait_for_switch(ctrl,
					 prev_frame_seq(job->frame_seq_no)))
		goto SWITCH_FAILURE;

	dev_info(dev, "[%s] begin waiting switch no:%d seq 0x%x\n",
		__func__, job->req_seq, job->frame_seq_no);

	trace_seamless_apply_sensor(job->sensor->name,
				    ctx->stream_id, job->frame_seq_no, 1);
	mtk_cam_job_state_set(&job->job_state, SENSOR_STATE, S_SENSOR_APPLYING);
	call_jobop(job, apply_sensor);
	mtk_cam_job_state_set(&job->job_state, SENSOR_STATE, S_SENSOR_LATCHED);
	trace_seamless_apply_sensor(job->sensor->name,
				    ctx->stream_id, job->frame_seq_no, 0);

	if (!wait_for_completion_timeout(&job->compose_completion, timeout)) {
		pr_info("[%s] error: wait for job composed timeout\n",
			__func__);
		goto SWITCH_FAILURE;
	}

	mtk_cam_job_state_set(&job->job_state, ISP_STATE, S_ISP_APPLYING);
	call_jobop(job, apply_isp);

	if (!wait_for_completion_timeout(&job->cq_exe_completion, timeout)) {
		dev_info(dev, "[%s] error: wait for cq_exe timeout\n",
			 __func__);
		goto SWITCH_FAILURE;
	}

	call_jobop(job, apply_switch);
	vsync_set_desired(&ctrl->vsync_col, _get_master_engines(job->used_engine));

	dev_info(dev, "[%s] finish, used_engine:0x%x\n",
		 __func__, job->used_engine);
	return;

SWITCH_FAILURE:
	dev_info(dev, "[%s] failed: ctx-%d job %d frame_seq 0x%x\n",
		 __func__, ctx->stream_id, job->req_seq, job->frame_seq_no);

	WRAP_AEE_EXCEPTION(MSG_SWITCH_FAILURE, __func__);
}

/* request queue */
void mtk_cam_ctrl_job_enque(struct mtk_cam_ctrl *cam_ctrl,
			    struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx;
	u32 req_seq, frame_seq;

	if (mtk_cam_ctrl_get(cam_ctrl))
		return;

	ctx = cam_ctrl->ctx;

	frame_seq = cam_ctrl->enqueued_frame_seq_no;
	cam_ctrl->enqueued_frame_seq_no =
		add_frame_seq(cam_ctrl->enqueued_frame_seq_no, job->frame_cnt);

	req_seq = atomic_inc_return(&cam_ctrl->enqueued_req_cnt);
	mtk_cam_job_set_no(job, req_seq, frame_seq);

	if (job->seamless_switch)
		mtk_cam_job_set_fsm(job, 0);

	/* EnQ this request's state element to state_list (STATE:READY) */
	write_lock(&cam_ctrl->list_lock);
	list_add_tail(&job->job_state.list, &cam_ctrl->camsys_state_list);
	write_unlock(&cam_ctrl->list_lock);

	// to be removed
	if (frame_seq == 0) {
		vsync_set_desired(&cam_ctrl->vsync_col,
				  _get_master_engines(job->used_engine));

		/* TODO(AY): refine this */
		if (job->job_scen.id == MTK_CAM_SCEN_M2M_NORMAL ||
		    job->job_scen.id == MTK_CAM_SCEN_ODT_NORMAL ||
		    job->job_scen.id == MTK_CAM_SCEN_ODT_MSTREAM) {

			atomic_set(&cam_ctrl->stream_on_done, 1);
			mtk_cam_watchdog_start(&cam_ctrl->watchdog, 0);
		}
	}

	if (!atomic_read(&cam_ctrl->stream_on_done)) {

		mtk_cam_job_set_fsm(job, 0);
		if (CAM_DEBUG_ENABLED(CTRL))
			pr_info("disable job #%d's fsm\n", job->req_seq);
	}

	call_jobop(job, compose);
	mtk_cam_ctrl_send_event(cam_ctrl, CAMSYS_EVENT_ENQUE);
	dev_dbg(ctx->cam->dev, "[%s] ctx:%d, req_no:%d frame_no:%d\n",
		__func__, ctx->stream_id, req_seq, frame_seq);

	if (job->stream_on_seninf)
		mtk_cam_wq_ctrl_queue_work(&cam_ctrl->highpri_wq_ctrl,
			mtk_cam_ctrl_stream_on_work, job);

	if (job->seamless_switch)
		mtk_cam_wq_ctrl_queue_work(&cam_ctrl->highpri_wq_ctrl,
			mtk_cam_ctrl_seamless_switch_work, job);

	mtk_cam_ctrl_put(cam_ctrl);
}

void mtk_cam_ctrl_job_composed(struct mtk_cam_ctrl *cam_ctrl,
			       unsigned int fh_cookie,
			       struct mtkcam_ipi_frame_ack_result *cq_ret,
			       int ack_ret)
{
	struct mtk_cam_job *job_composed;
	struct mtk_cam_device *cam;
	int ctx_id, seq;

	if (mtk_cam_ctrl_get(cam_ctrl))
		return;

	cam = cam_ctrl->ctx->cam;
	ctx_id = ctx_from_fh_cookie(fh_cookie);
	seq = seq_from_fh_cookie(fh_cookie);

	job_composed = mtk_cam_ctrl_get_job(cam_ctrl,
					    cond_frame_no_belong, &seq);

	if (WARN_ON(!job_composed)) {
		dev_info(cam->dev, "%s: failed to find job ctx_id/frame = %d/%d\n",
			 __func__, ctx_id, seq);
		goto PUT_CTRL;
	}

	call_jobop(job_composed, compose_done, cq_ret, ack_ret);
	mtk_cam_job_put(job_composed);

	spin_lock(&cam_ctrl->info_lock);
	cam_ctrl->r_info.ack_seq_no = seq;
	spin_unlock(&cam_ctrl->info_lock);

	mtk_cam_ctrl_send_event(cam_ctrl, CAMSYS_EVENT_ACK);

PUT_CTRL:
	mtk_cam_ctrl_put(cam_ctrl);
}

static void reset_runtime_info(struct mtk_cam_ctrl *ctrl)
{
	struct mtk_cam_ctrl_runtime_info *info = &ctrl->r_info;

	spin_lock(&ctrl->info_lock);

	memset(info, 0, sizeof(*info));
	info->ack_seq_no = -1;
	info->outer_seq_no = -1;
	info->inner_seq_no = -1;

	spin_unlock(&ctrl->info_lock);
}

void mtk_cam_ctrl_start(struct mtk_cam_ctrl *cam_ctrl, struct mtk_cam_ctx *ctx)
{
	cam_ctrl->ctx = ctx;

	atomic_set(&cam_ctrl->enqueued_req_cnt, 0);
	cam_ctrl->enqueued_frame_seq_no = 0;
	cam_ctrl->fs_event_subframe_cnt = 0;
	cam_ctrl->fs_event_subframe_idx = 0;

	atomic_set(&cam_ctrl->stopped, 0);
	atomic_set(&cam_ctrl->stream_on_done, 0);

	atomic_set(&cam_ctrl->await_switching_seq, -1);
	init_completion(&cam_ctrl->vsync_complete);

	spin_lock_init(&cam_ctrl->send_lock);
	rwlock_init(&cam_ctrl->list_lock);
	INIT_LIST_HEAD(&cam_ctrl->camsys_state_list);

	spin_lock_init(&cam_ctrl->info_lock);
	reset_runtime_info(cam_ctrl);

	init_waitqueue_head(&cam_ctrl->stop_wq);

	vsync_reset(&cam_ctrl->vsync_col);
	cam_ctrl->cur_cq_ref = 0;

	mtk_cam_watchdog_init(&cam_ctrl->watchdog);

	// TODO(Will): create dedicated workqueue to guarantee in order
	/* note: not sure if using system_highpri_wq is suitable */
	mtk_cam_wq_ctrl_init(&cam_ctrl->highpri_wq_ctrl,
		system_highpri_wq, ctx);

	dev_info(ctx->cam->dev, "[%s] ctx:%d\n", __func__, ctx->stream_id);
}

void mtk_cam_ctrl_stop(struct mtk_cam_ctrl *cam_ctrl)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;
	struct mtk_cam_job_state *job_s;
	struct mtk_cam_job *job;
	struct list_head job_list;

	/* stop procedure
	 * 1. mark 'stopped' status to skip further processing
	 * 2. stop all working context
	 *   a. disable_irq for threaded_irq
	 *   b. workqueue: cancel_work_sync & (drain/flush)_workqueue
	 *   c. kthread: cancel_work_sync & flush_worker
	 * 3. Now, all contexts are stopped. return resources
	 */
	atomic_set(&cam_ctrl->stopped, 1);

	/* should wait stream-on/seamless switch finished before stopping */
	mtk_cam_wq_ctrl_wait_finish(&cam_ctrl->highpri_wq_ctrl);

	mtk_cam_ctx_engine_off(ctx);

	/* disable irq first */
	mtk_cam_ctx_engine_disable_irq(ctx);

	/* note: after hw disabled, stop buffer_done worker */
	read_lock(&cam_ctrl->list_lock);
	list_for_each_entry(job_s, &cam_ctrl->camsys_state_list, list) {
		job = container_of(job_s, struct mtk_cam_job, job_state);

		call_jobop(job, cancel);
	}
	read_unlock(&cam_ctrl->list_lock);

	mtk_cam_watchdog_stop(&cam_ctrl->watchdog);

	/* this would be time consuming */
	ctx_stream_on_seninf_sensor(ctx, 0, 0, 0);

	mtk_cam_ctrl_wait_all_released(cam_ctrl);

	/* reset hw */
	mtk_cam_ctx_engine_reset(ctx);

	mtk_cam_event_eos(cam_ctrl);

	drain_workqueue(ctx->frame_done_wq);
	drain_workqueue(ctx->aa_dump_wq);
	kthread_flush_worker(&ctx->sensor_worker);

	INIT_LIST_HEAD(&job_list);

	write_lock(&cam_ctrl->list_lock);
	list_splice_init(&cam_ctrl->camsys_state_list, &job_list);
	write_unlock(&cam_ctrl->list_lock);

	list_for_each_entry(job_s, &job_list, list) {
		job = container_of(job_s, struct mtk_cam_job, job_state);

		/*
		 * note: call mtk_cam_ctx_job_finish directly here
		 */
		mtk_cam_ctx_job_finish(job);
	}

	dev_info(ctx->cam->dev, "[%s] ctx:%d\n", __func__, ctx->stream_id);
}

int vsync_update(struct vsync_collector *c,
		  int engine_type, int idx,
		  struct vsync_result *res)
{
	unsigned int coming;

	if (!res)
		return 1;

	coming = engine_idx_to_bit(engine_type, idx);

	if (!(coming & c->desired))
		return 1;

	c->collected |= (coming & c->desired);

	if (CAM_DEBUG_ENABLED(CTRL))
		pr_info("%s: vsync desired/collected/coming %x/%x/%x\n",
			__func__, c->desired, c->collected, (coming & c->desired));

	res->is_first = !(c->collected & (c->collected - 1));
	res->is_last = c->collected == c->desired;

	if (res->is_last)
		c->collected = 0;

	return 0;
}

static inline u64 mtk_cam_ctrl_latest_sof(struct mtk_cam_ctrl *ctrl)
{
	u64 ts_ns;

	spin_lock(&ctrl->info_lock);
	ts_ns = ctrl->r_info.sof_ts_ns;
	spin_unlock(&ctrl->info_lock);

	return ts_ns;
}

struct watchdog_debug_work {
	struct work_struct work;
	struct mtk_cam_watchdog *wd;
	bool seninf_check_timeout;
};

static void mtk_cam_ctrl_dump_first_job(struct mtk_cam_ctrl *ctrl, int *seq)
{
	struct mtk_cam_job *job;

	job = mtk_cam_ctrl_get_job(ctrl, cond_first_job, 0);
	if (job) {
		int seq_no;

		if (seq)
			seq_no = *seq;
		else {
			spin_lock(&ctrl->info_lock);
			seq_no = ctrl->r_info.inner_seq_no;
			spin_unlock(&ctrl->info_lock);
		}

		call_jobop(job, dump, seq_no);
		mtk_cam_job_put(job);
	} else
		pr_info("%s: no job to dump", __func__);
}

static void mtk_dump_debug_for_no_vsync(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_cam_ctrl *ctrl = &ctx->cam_ctrl;
	struct mtk_cam_job *job;

	dump_runtime_info(&ctrl->r_info);

	job = mtk_cam_ctrl_get_job(ctrl, cond_first_job, 0);
	if (job) {
		mtk_engine_dump_debug_status(cam, job->used_engine);
		mtk_cam_job_put(job);
	} else {
		mtk_engine_dump_debug_status(cam, ctx->used_engine);
	}
}

static void mtk_cam_watchdog_sensor_worker(struct work_struct *work)
{
	struct watchdog_debug_work *dbg_work;
	struct mtk_cam_watchdog *wd;
	struct mtk_cam_ctrl *ctrl;
	struct mtk_cam_ctx *ctx;
	int seq_no;

	dbg_work = container_of(work, struct watchdog_debug_work, work);
	wd = dbg_work->wd;
	if (!wd)
		goto FREE_WORK;

	ctrl = container_of(wd, struct mtk_cam_ctrl, watchdog);
	ctx = ctrl->ctx;
	if (!ctx || !ctx->seninf || atomic_read(&ctrl->stopped))
		goto EXIT_WORK;

	if (dbg_work->seninf_check_timeout) {
		u64 diff_ns;
		int timeout;

		diff_ns = ktime_get_boottime_ns() - mtk_cam_ctrl_latest_sof(ctrl);
		timeout = mtk_cam_seninf_check_timeout(ctx->seninf, diff_ns);

		if (!timeout)
			goto EXIT_WORK;
	}

	spin_lock(&ctrl->info_lock);
	seq_no = ctrl->r_info.inner_seq_no;
	spin_unlock(&ctrl->info_lock);

	/* handle timeout */
	if (mtk_cam_seninf_dump(ctx->seninf, seq_no, true)) {
		//mtk_cam_event_esd_recovery(ctx->pipe, ctx->dequeued_frame_seq_no);
		pr_info("%s: TODO: add esd event\n", __func__);

	}

	if (atomic_read(&wd->reset_sensor_cnt) < WATCHDOG_MAX_SENSOR_RETRY_CNT)
		goto EXIT_WORK;

	dev_info(ctx->cam->dev, "ctx-%d reset sensor failed\n", ctx->stream_id);
	mtk_dump_debug_for_no_vsync(ctx);

	mtk_cam_event_error(ctrl, MSG_VSYNC_TIMEOUT);
	WRAP_AEE_EXCEPTION(MSG_VSYNC_TIMEOUT, "watchdog timeout");

EXIT_WORK:
	complete(&wd->work_complete);
FREE_WORK:
	kfree(dbg_work);
}

static void mtk_cam_watchdog_job_worker(struct work_struct *work)
{
	struct watchdog_debug_work *dbg_work;
	struct mtk_cam_watchdog *wd;
	struct mtk_cam_ctrl *ctrl;
	struct mtk_cam_ctx *ctx;

	dbg_work = container_of(work, struct watchdog_debug_work, work);
	wd = dbg_work->wd;
	if (!wd)
		goto FREE_WORK;

	ctrl = container_of(wd, struct mtk_cam_ctrl, watchdog);
	ctx = ctrl->ctx;
	if (!ctx || atomic_read(&ctrl->stopped))
		goto EXIT_WORK;

	mtk_cam_ctrl_dump_first_job(ctrl, NULL);

EXIT_WORK:
	complete(&wd->work_complete);
FREE_WORK:
	kfree(dbg_work);
}

static int watchdog_schedule_debug_work(struct mtk_cam_watchdog *wd,
					void (*func)(struct work_struct *work),
					bool seninf_check_timeout)
{
	struct watchdog_debug_work *work;

	work = kmalloc(sizeof(*work), GFP_ATOMIC);
	if (WARN_ON(!work))
		return -1;

	work->wd = wd;
	INIT_WORK(&work->work, func);
	work->seninf_check_timeout = seninf_check_timeout;

	schedule_work(&work->work);
	return 0;
}


static int mtk_cam_watchdog_schedule_sensor_reset(struct mtk_cam_watchdog *wd,
						  bool check_timeout)
{
	return watchdog_schedule_debug_work(wd, mtk_cam_watchdog_sensor_worker,
					    check_timeout);
}

static int mtk_cam_watchdog_schedule_job_dump(struct mtk_cam_watchdog *wd)
{
	return watchdog_schedule_debug_work(wd, mtk_cam_watchdog_job_worker, 0);
}

static int try_launch_watchdog_sensor_worker(struct mtk_cam_watchdog *wd,
					     bool check_timeout)
{
	struct mtk_cam_ctrl *ctrl =
		container_of(wd, struct mtk_cam_ctrl, watchdog);
	struct mtk_cam_ctx *ctx = ctrl->ctx;
	bool completed;
	int reset_cnt = -1;

	completed = try_wait_for_completion(&wd->work_complete);
	if (!completed)
		goto SKIP_SCHEDULE_WORK;

	reset_cnt = atomic_inc_return(&wd->reset_sensor_cnt);
	if (reset_cnt > WATCHDOG_MAX_SENSOR_RETRY_CNT) {
		complete(&wd->work_complete);
		goto SKIP_SCHEDULE_WORK;
	}

	dev_info(ctx->cam->dev, "schedule work for sensor_reset: ctx-%d\n",
		 ctx->stream_id);
	mtk_cam_watchdog_schedule_sensor_reset(wd, check_timeout);
	return 0;

SKIP_SCHEDULE_WORK:
	dev_info_ratelimited(ctx->cam->dev,
		 "%s:ctx-%d skip schedule watchdog work running %d, retry cnt %d\n",
		 __func__, ctx->stream_id, !completed, reset_cnt);
	return -1;
}

static int mtk_cam_watchdog_monitor_vsync(struct mtk_cam_watchdog *wd)
{
	struct mtk_cam_ctrl *ctrl =
		container_of(wd, struct mtk_cam_ctrl, watchdog);
	struct mtk_cam_ctx *ctx = ctrl->ctx;
	u64 new_sof;

	if (!ctx)
		return -1;

	new_sof = mtk_cam_ctrl_latest_sof(ctrl);
	if (new_sof != wd->last_sof_ts) {
		wd->last_sof_ts = new_sof;

		atomic_set(&wd->reset_sensor_cnt, 0);
		return 0;
	}

	dev_info_ratelimited(ctx->cam->dev,
			     "%s: vsync may timeout, last ts = %lld\n",
			     __func__, wd->last_sof_ts);

	try_launch_watchdog_sensor_worker(wd, 1);
	return -1;
}

static bool in_valid_hw_processing_time(u64 diff_ns)
{
	return (diff_ns / 1000000ULL) < WATCHDOG_MAX_HWTIME_MS;
}

static int mtk_cam_watchdog_monitor_job(struct mtk_cam_watchdog *wd)
{
	struct mtk_cam_ctrl *ctrl =
		container_of(wd, struct mtk_cam_ctrl, watchdog);
	struct mtk_cam_ctx *ctx = ctrl->ctx;
	struct mtk_cam_job *job;
	int req_seq;
	u64 job_ts;
	u64 ts;
	bool completed;

	if (!ctx)
		return -1;

	job = mtk_cam_ctrl_get_job(ctrl, cond_first_job, 0);
	if (!job)
		return 0;

	req_seq = job->req_seq;
	job_ts = job->timestamp;
	mtk_cam_job_put(job);

	if (req_seq != wd->req_seq) {
		wd->req_seq = req_seq;
		return 0;
	}

	ts = ktime_get_boottime_ns();
	if (!job_ts || in_valid_hw_processing_time(ts - job_ts)) {
		dev_info(ctx->cam->dev, "job #%d job_ts %llu ts %llu, skip\n",
			 req_seq, job_ts, ts);
		return 0;
	}

	completed = try_wait_for_completion(&wd->work_complete);
	if (!completed)
		goto SKIP_SCHEDULE_WORK;

	if (atomic_cmpxchg(&wd->dump_job, 0, 1)) {
		complete(&wd->work_complete);
		goto SKIP_SCHEDULE_WORK;
	}

	/* job is not updated */
	dev_info(ctx->cam->dev, "schedule work for job_dump: ctx-%d req %d\n",
		 ctx->stream_id, wd->req_seq);
	mtk_cam_watchdog_schedule_job_dump(wd);
	return -1;

SKIP_SCHEDULE_WORK:
	dev_info_ratelimited(ctx->cam->dev,
		 "%s:ctx-%d req_seq %d skip schedule watchdog work running %d, dumped %d\n",
		 __func__, ctx->stream_id, req_seq,
		 !completed, atomic_read(&wd->dump_job));
	return -1;
}

static void mtk_cam_watchdog_check(struct mtk_cam_watchdog *wd)
{
	int ret;

	ret = wd->monitor_vsync ?
		mtk_cam_watchdog_monitor_vsync(wd) : 0;
	if (ret)
		return;

	mtk_cam_watchdog_monitor_job(wd);
}

struct mtk_cam_watchdog_monitor {
	struct work_struct work;
	struct mtk_cam_watchdog *wd;
};

static bool do_monitor_check(struct mtk_cam_watchdog *wd)
{
	return atomic_cmpxchg(&wd->timer_signaled, 1, 0);
}

static void mtk_cam_watchdog_monitor_loop(struct work_struct *work)
{
	struct mtk_cam_watchdog_monitor *monitor;
	struct mtk_cam_watchdog *wd;
	int ret;

	monitor = container_of(work, struct mtk_cam_watchdog_monitor, work);
	wd = monitor->wd;

	if (WARN_ON(!monitor->wd))
		return;

	do {
		ret = wait_event_interruptible(wd->monitor_wq,
					       do_monitor_check(wd));

		if (!atomic_read(&wd->started))
			break;

		if (ret)
			continue;

		mtk_cam_watchdog_check(wd);
	} while (atomic_read(&wd->started));

	complete(&wd->monitor_complete);
	pr_info("%s: existed\n", __func__);

	kfree(monitor);
}

static void mtk_cam_watchdog_timer_callback(struct timer_list *t)
{
	struct mtk_cam_watchdog *wd = from_timer(wd, t, timer);

	atomic_set(&wd->timer_signaled, 1);
	wake_up_interruptible(&wd->monitor_wq);

	// renew timer
	wd->timer.expires = jiffies + msecs_to_jiffies(WATCHDOG_INTERVAL_MS);
	add_timer(&wd->timer);
}

void mtk_cam_watchdog_init(struct mtk_cam_watchdog *wd)
{
	atomic_set(&wd->started, 0);

	timer_setup(&wd->timer, mtk_cam_watchdog_timer_callback, 0);
	atomic_set(&wd->timer_signaled, 0);
	init_waitqueue_head(&wd->monitor_wq);

	init_completion(&wd->monitor_complete);
	init_completion(&wd->work_complete);
	complete(&wd->work_complete);

	atomic_set(&wd->reset_sensor_cnt, 0);
	atomic_set(&wd->dump_job, 0);
}

static int launch_monitor_work(struct mtk_cam_watchdog *wd)
{
	struct mtk_cam_watchdog_monitor *monitor;

	monitor = kmalloc(sizeof(*monitor), GFP_ATOMIC);
	if (WARN_ON(!monitor))
		return -1;

	monitor->wd = wd;
	INIT_WORK(&monitor->work, mtk_cam_watchdog_monitor_loop);

	schedule_work(&monitor->work);
	return 0;
}

int mtk_cam_watchdog_start(struct mtk_cam_watchdog *wd, bool monitor_vsync)
{
	struct mtk_cam_ctrl *ctrl =
		container_of(wd, struct mtk_cam_ctrl, watchdog);

	atomic_set(&wd->started, 1);

	wd->monitor_vsync = monitor_vsync;

	wd->last_sof_ts = mtk_cam_ctrl_latest_sof(ctrl);

	wd->req_seq = 0;

	launch_monitor_work(wd);

	wd->timer.expires = jiffies + msecs_to_jiffies(WATCHDOG_INTERVAL_MS);
	add_timer(&wd->timer);
	return 0;
}

void mtk_cam_watchdog_stop(struct mtk_cam_watchdog *wd)
{
	if (!atomic_cmpxchg(&wd->started, 1, 0))
		return;

	del_timer_sync(&wd->timer);

	atomic_set(&wd->timer_signaled, 1);
	wake_up_interruptible(&wd->monitor_wq);

	wait_for_completion(&wd->monitor_complete);
	wait_for_completion(&wd->work_complete);
}

static void mtk_cam_wq_ctrl_runner(struct work_struct *work)
{
	struct mtk_cam_sys_wq_work *sys_wq_work =
		container_of(work, struct mtk_cam_sys_wq_work, work);
	struct mtk_cam_sys_wq_ctrl *wq_ctrl = sys_wq_work->wq_ctrl;

	if (atomic_read(&wq_ctrl->stopped)) {
		pr_info("wq_ctrl already stopped");
		goto EXIT;
	}

	sys_wq_work->exec(work);

EXIT:
	atomic_dec(&wq_ctrl->running);
	mtk_cam_job_put(sys_wq_work->job);
	complete(&wq_ctrl->work_done);
	kfree(sys_wq_work);
}

void mtk_cam_wq_ctrl_init(struct mtk_cam_sys_wq_ctrl *wq_ctrl,
			  struct workqueue_struct *wq,
			  struct mtk_cam_ctx *ctx)
{
	wq_ctrl->wq = wq;
	wq_ctrl->ctx = ctx;

	atomic_set(&wq_ctrl->running, 0);
	atomic_set(&wq_ctrl->stopped, 0);
	init_completion(&wq_ctrl->work_done);
}

int mtk_cam_wq_ctrl_queue_work(struct mtk_cam_sys_wq_ctrl *wq_ctrl,
			       void (*exec)(struct work_struct *work),
			       struct mtk_cam_job *job)
{
	struct mtk_cam_sys_wq_work *sys_wq_work;

	sys_wq_work = kmalloc(sizeof(*sys_wq_work), GFP_ATOMIC);
	if (WARN_ON(!sys_wq_work))
		return -1;

	INIT_WORK(&sys_wq_work->work, mtk_cam_wq_ctrl_runner);
	sys_wq_work->wq_ctrl = wq_ctrl;
	sys_wq_work->exec = exec;
	sys_wq_work->job = job;

	mtk_cam_job_get(sys_wq_work->job);
	atomic_inc(&wq_ctrl->running);
	queue_work(wq_ctrl->wq, &sys_wq_work->work);

	return 0;
}

void mtk_cam_wq_ctrl_wait_finish(struct mtk_cam_sys_wq_ctrl *wq_ctrl)
{
	atomic_set(&wq_ctrl->stopped, 1);
	while (atomic_read(&wq_ctrl->running) > 0)
		wait_for_completion(&wq_ctrl->work_done);
}

int mtk_cam_ctrl_reset_sensor(struct mtk_cam_device *cam,
			      int engine_type, unsigned int engine_id,
			      int inner_cookie)
{
	unsigned int ctx_id = ctx_from_fh_cookie(inner_cookie);
	struct mtk_cam_ctrl *ctrl = &cam->ctxs[ctx_id].cam_ctrl;
	bool check_timeout = 0; /* not to check if timeout */

	dev_info(cam->dev, "%s: engine %d id %d seq 0x%x\n",
		 __func__, engine_type, engine_id, inner_cookie);

	if (mtk_cam_ctrl_get(ctrl))
		return 0;

	try_launch_watchdog_sensor_worker(&ctrl->watchdog, check_timeout);

	mtk_cam_ctrl_put(ctrl);
	return 0;
}

int mtk_cam_ctrl_dump_request(struct mtk_cam_device *cam,
			      int engine_type, unsigned int engine_id,
			      int inner_cookie)
{
	unsigned int ctx_id = ctx_from_fh_cookie(inner_cookie);
	int seq = seq_from_fh_cookie(inner_cookie);
	struct mtk_cam_ctrl *ctrl = &cam->ctxs[ctx_id].cam_ctrl;

	dev_info(cam->dev, "%s: engine %d id %d seq 0x%x\n",
		 __func__, engine_type, engine_id, inner_cookie);

	if (mtk_cam_ctrl_get(ctrl))
		return 0;

	mtk_cam_ctrl_dump_first_job(ctrl, &seq);

	mtk_cam_ctrl_put(ctrl);
	return 0;
}
