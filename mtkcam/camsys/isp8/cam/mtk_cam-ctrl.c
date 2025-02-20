// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/list.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>

#include <media/v4l2-event.h>
#include <media/v4l2-subdev.h>

#include <soc/mediatek/smi.h>

#include "mtk_cam.h"
#include "mtk_cam-ctrl.h"
#include "mtk_cam-debug.h"
#include "mtk_cam-dvfs_qos.h"
#include "mtk_cam-hsf.h"
#include "mtk_cam-pool.h"
//#include "mtk_cam-tg-flash.h"
#include "mtk_camera-v4l2-controls-8.h"
#include "mtk_camera-videodev2.h"
#include "mtk_cam-trace.h"
#include "mtk_cam-qof.h"
#include "mtk_cam-bwr.h"
#include "mtk_cam-job_utils.h"
#include "mtk_cam-raw_ctrl.h"

#define WATCHDOG_INTERVAL_MS		800
/*
 * note:
 *   there's a kind of sensor failure would be
 *   after receiving vsync, nothing happens for a while
 *   (w.o. any error interrupt & done)
 *   so, set MAX_HWTIME_MS to WATCHDOG_INTERVAL_MS * 2 for this case
 */
#define WATCHDOG_MAX_HWTIME_MS		(WATCHDOG_INTERVAL_MS * 2)
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
	if (!cam_ctrl)
		return -1;
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

static int ctrl_fetch_inner(struct mtk_cam_ctrl *ctrl)
{
	int inner_seq;

	spin_lock(&ctrl->info_lock);
	inner_seq = ctrl->r_info.inner_seq_no;
	spin_unlock(&ctrl->info_lock);

	return inner_seq;
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
bool cond_req_id_equal(struct mtk_cam_job *job, void *arg)
{
	unsigned int no = *(int *)arg;

	return no == job->req_info_id;
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

static int ctrl_enable_job_fsm_until_switch(struct mtk_cam_job *job,
					    void *arg)
{
	struct mtk_cam_job *switched_job = (struct mtk_cam_job *)arg;

	if (switched_job != job && job->raw_switch)
		return -1;

	if (CAM_DEBUG_ENABLED(CTRL))
		pr_info("%s: enable#%d\n", __func__, job->req_seq);

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

/* note: without 'get' */
static struct mtk_cam_job *
mtk_cam_ctrl_fetch_first_unfinished_job(struct mtk_cam_ctrl *ctrl)
{
	struct mtk_cam_job *job = NULL;
	struct mtk_cam_job_state *state;
	bool found = 0;

	read_lock(&ctrl->list_lock);

	list_for_each_entry(state, &ctrl->camsys_state_list, list) {
		job = container_of(state, struct mtk_cam_job, job_state);

		found = !mtk_cam_job_is_done(job);
		if (found)
			break;
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
		pr_info("%s: ctx-%d seq %u/%u/%u/%u\n", func, ctx_id,
			e->u.frame_sync.frame_sequence, (unsigned int)e->u.data[4],
			(unsigned int)e->u.data[8], (unsigned int)e->u.data[12]);
		break;
	case V4L2_EVENT_REQUEST_SENSOR_TRIGGER:
		pr_info("%s: ctx-%d tg_cnt:%d frame_seq:%d\n", func, ctx_id,
			(unsigned int)e->u.data[0], (unsigned int)e->u.data[4]);
		break;
	case V4L2_EVENT_ERROR:
		pr_info("%s: ctx-%d %s\n", func, ctx_id, e->u.data);
		break;
	case V4L2_EVENT_CAMSYS_RESOURCE_READY:
		pr_info("%s: ctx-%d: 0x%x\n", func, ctx_id, (unsigned int)e->u.data[0]);
		break;
	default:
		pr_info("%s: ctx-%d event type %d\n", func, ctx_id, e->type);
		break;
	}
}

void mtk_cam_event_eos(struct mtk_cam_ctrl *cam_ctrl)
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

void mtk_cam_event_esd_recovery(struct mtk_cam_ctrl *cam_ctrl,
				     unsigned int frame_seq_no)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;
	struct v4l2_event event = {
		.type = V4L2_EVENT_ESD_RECOVERY,
		.u.frame_sync.frame_sequence = frame_seq_no,
	};
	mtk_cam_ctx_send_raw_event(ctx, &event);
	log_event(__func__, ctx->stream_id, &event);
}

void mtk_cam_event_sensor_trigger(struct mtk_cam_ctrl *cam_ctrl,
			      unsigned int frame_seq_no)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;
	int tg_cnt = cam_ctrl->r_info.extisp_tg_cnt[EXTISP_DATA_META];
	struct mtk_cam_event_sensor_trigger data = {
		.tg_cnt = tg_cnt,
		.sensor_seq = frame_seq_no,
	};
	struct v4l2_event event = {
		.type = V4L2_EVENT_REQUEST_SENSOR_TRIGGER,
		.u.frame_sync.frame_sequence = frame_seq_no,
	};
	memcpy(event.u.data, &data, 8);
	if (ctx->has_raw_subdev)
		mtk_cam_ctx_send_raw_event(ctx, &event);
	else
		mtk_cam_ctx_send_sv_event(ctx, &event);

	if (CAM_DEBUG_ENABLED(V4L2_EVENT))
		log_event(__func__, ctx->stream_id, &event);
}
void mtk_cam_event_extisp_camsys_ready(struct mtk_cam_ctrl *cam_ctrl)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;
	struct v4l2_event event = {
		.type = V4L2_EVENT_EXTISP_CAMSYS_READY,
	};
	if (ctx->has_raw_subdev)
		mtk_cam_ctx_send_raw_event(ctx, &event);
	else
		mtk_cam_ctx_send_sv_event(ctx, &event);

	log_event(__func__, ctx->stream_id, &event);
}
void mtk_cam_event_camsys_resource_ready(struct mtk_cam_ctrl *cam_ctrl,	u32 raw_ready)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;
	struct mtk_cam_event_camsys_resource_ready data = {
		.raw_ready = raw_ready,
	};
	struct v4l2_event event = {
		.type = V4L2_EVENT_CAMSYS_RESOURCE_READY,
	};

	memcpy(event.u.data, &data, 4);
	if (ctx->has_raw_subdev)
		mtk_cam_ctx_send_raw_event(ctx, &event);

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
	if (frame_seq_no >= cam_ctrl->frame_seq)
		cam_ctrl->frame_seq = frame_seq_no;
	else
		pr_info("%s:revert frame_seq %d < %d", __func__,
			frame_seq_no, cam_ctrl->frame_seq);
	struct mtk_cam_event_frame_sync_data data = {
		.frame_sequence = cam_ctrl->frame_seq,
		.sensor_sequence = cam_ctrl->sensor_seq,
		.frame_sync_id = cam_ctrl->frame_sync_id,
		.sensor_sync_id = cam_ctrl->sensor_sync_id,
		.ts_ns = cam_ctrl->r_info.sof_ts_ns,
	};
	memcpy(event.u.data, &data, 24);
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
	else
		mtk_cam_ctx_send_sv_event(ctx, &event);

	if (CAM_DEBUG_ENABLED(V4L2_EVENT))
		log_event(__func__, ctx->stream_id, &event);
}

static void dump_runtime_info(struct mtk_cam_ctrl *ctrl)
{
	struct mtk_cam_ctrl_runtime_info *info = &ctrl->r_info;

	spin_lock(&ctrl->info_lock);
	pr_info("[%s] ack 0x%x out/in 0x%x/0x%x\n", __func__,
		info->ack_seq_no, info->outer_seq_no, info->inner_seq_no);
	pr_info("[%s] sof_ts_ns %lld\n", __func__, info->sof_ts_ns);
	spin_unlock(&ctrl->info_lock);
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
	spin_lock(p->info_lock);
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
	spin_unlock(p->info_lock);
}

static const int waitable_event =
	BIT(CAMSYS_EVENT_IRQ_L_SOF) |
	BIT(CAMSYS_EVENT_IRQ_FRAME_DONE) |
	BIT(CAMSYS_EVENT_IRQ_L_CQ_DONE) |
	BIT(CAMSYS_EVENT_ACK) |
	BIT(CAMSYS_EVENT_OFF);

static void mtk_cam_ctrl_wake_up_on_event(struct mtk_cam_ctrl *ctrl, int event)
{

	if (BIT(event) & (~waitable_event))
		return;

	wake_up(&ctrl->event_wq);
}

/* note: just to support little margin for sw latency here */
#define VALID_SWITCH_PERIOD_FROM_VSYNC_MS	23
#define VALID_SWITCH_PERIOD_120FPS_FROM_VSYNC_NS	3300000
#define VALID_SWITCH_PERIOD_60FPS_FROM_VSYNC_MS		11600000
#define VALID_SWITCH_PERIOD_30FPS_FROM_VSYNC_MS		23000000


struct seamless_check_args {
	int expect_inner;
	int expect_ack;
};

static bool check_for_seamless(struct mtk_cam_ctrl *ctrl, void *arg)
{
	struct seamless_check_args *args = arg;
	u64 last_sof_ts, first_sof_ts;
	int inner_seq;
	int ack_seq;
	u64 ts;
	u64 frame_interval_ns, ts_margin;

	spin_lock(&ctrl->info_lock);
	inner_seq = ctrl->r_info.inner_seq_no;
	last_sof_ts = ctrl->r_info.sof_l_ts_ns;
	first_sof_ts = ctrl->r_info.sof_ts_ns;
	ack_seq = ctrl->r_info.ack_seq_no;
	spin_unlock(&ctrl->info_lock);

	if (atomic_read(&ctrl->ctx->streaming) == 0)
		return 1;
	if (inner_seq != args->expect_inner)
		return 0;

	ts = ktime_get_boottime_ns();
	frame_interval_ns = ctrl->frame_interval_ns;
	if (frame_interval_ns < INTERVAL_NS(60))
		ts_margin = VALID_SWITCH_PERIOD_120FPS_FROM_VSYNC_NS;
	else if (INTERVAL_NS(60) <= frame_interval_ns &&
		 frame_interval_ns < INTERVAL_NS(30))
		ts_margin = VALID_SWITCH_PERIOD_60FPS_FROM_VSYNC_MS;
	else
		ts_margin = VALID_SWITCH_PERIOD_30FPS_FROM_VSYNC_MS;
	if (ts - last_sof_ts >= ts_margin)
		return 0;
	/*
	 * check if already got ack
	 * if not, await another vsync for switching
	 */

	if (!frame_seq_ge(ack_seq, args->expect_ack)) {
		pr_info("%s: not ack 0x%x yet, current=0x%x\n", __func__,
			args->expect_ack, ack_seq);
		return 0;
	}

	return 1;
}

static bool check_for_inner(struct mtk_cam_ctrl *ctrl, void *arg)
{
	struct seamless_check_args *args = arg;
	int inner_seq;

	if (atomic_read(&ctrl->ctx->streaming) == 0)
		return 1;

	spin_lock(&ctrl->info_lock);
	inner_seq = ctrl->r_info.inner_seq_no;
	spin_unlock(&ctrl->info_lock);

	return frame_seq_ge(inner_seq, args->expect_inner);
}

static bool check_done(struct mtk_cam_ctrl *ctrl, void *arg)
{
	int arg_seq = *(int *)arg;
	int done_seq;

	spin_lock(&ctrl->info_lock);
	done_seq = ctrl->r_info.done_seq_no;
	spin_unlock(&ctrl->info_lock);

	return frame_seq_ge(done_seq, arg_seq);
}

static bool check_setting_done(struct mtk_cam_ctrl *ctrl, void *arg)
{
	int arg_seq = *(int *)arg;
	int setting_seq;

	spin_lock(&ctrl->info_lock);
	setting_seq = ctrl->r_info.outer_seq_no;
	spin_unlock(&ctrl->info_lock);

	return frame_seq_ge(setting_seq, arg_seq);
}

static int mtk_cam_ctrl_wait_event(struct mtk_cam_ctrl *ctrl,
				   bool (*cond)(struct mtk_cam_ctrl *, void *),
				   void *arg, int timeout_ms)
{
	long timeout;

	timeout = wait_event_timeout(ctrl->event_wq, cond(ctrl, arg),
				     msecs_to_jiffies(timeout_ms));
	if (timeout == 0) {
		pr_info("%s: error: wait for %ps: %dms timeout\n",
			__func__, cond, timeout_ms);
		return -1;
	}

	return 0;
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
	p.info_lock = &ctrl->info_lock;

	if (CAM_DEBUG_ENABLED(STATE))
		dump_runtime_info(ctrl);

	if (CAM_DEBUG_ENABLED(STATE))
		debug_send_event(&p);

	ctrl_send_event(ctrl, &p);

	mtk_cam_ctrl_wake_up_on_event(ctrl, event);
	ctrl_apply_actions(ctrl);

	return 0;
}
static void handle_extmeta_setting_done(struct mtk_cam_ctrl *cam_ctrl)
{
	mtk_cam_ctrl_send_event(cam_ctrl, CAMSYS_EVENT_IRQ_EXTMETA_CQ_DONE);
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
	if (ctrl->r_info.timeshare_enable &&
		engine_type == CAMSYS_ENGINE_CAMSV) {
		mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_IRQ_EXTMETA_FRAME_DONE);
		mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_IRQ_TRY_TS_TRIGGER);
	}
	if (call_jobop(job, mark_engine_done,
		       engine_type, engine_id, seq_no)) {

		/* last done: trigger FSM */

		spin_lock(&ctrl->info_lock);
		ctrl->r_info.done_seq_no = seq_no;
		spin_unlock(&ctrl->info_lock);

		mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_IRQ_FRAME_DONE);
		if (ctrl->r_info.timeshare_enable) {
			struct mtk_cam_ctx *ctx = ctrl->ctx;
			int i;
			/* unlock time share raw on process */
			for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
				if (ctx->hw_raw[i]) {
					struct mtk_raw_device *raw_dev =
						dev_get_drvdata(ctx->hw_raw[i]);

					if (atomic_read(&raw_dev->time_share_on_process) > 0)
						atomic_dec(&raw_dev->time_share_on_process);
					else
						dev_info(raw_dev->dev, "timeshare:warning count <= 0");
					break;
				}
			}
			if (ctx) {
				struct mtk_cam_ctrl *same_ts_raw_ctrl = NULL;
				int group_id = ctx->ctrldata.resource.user_data
					.raw_res.scen.scen.timeshare.group;
				int ctx_id;

				for (ctx_id = 0; ctx_id < ctx->cam->max_stream_num; ctx_id++) {
					/* no need to check current ctx */
					if (ctx_id == ctx->stream_id)
						continue;
					/* check same group id ctx */
					if (group_id ==
						ctx->cam->ctxs[ctx_id].ctrldata.resource.user_data
						.raw_res.scen.scen.timeshare.group) {
						same_ts_raw_ctrl = &ctrl->ctx->cam->ctxs[ctx_id].cam_ctrl;
						dev_info(ctx->cam->dev, "[%s] TRY_TS_TRIGGER other same groupid:%d ctx:%d\n",
							__func__, group_id, ctx_id);
						break;
					}
				}
				if (same_ts_raw_ctrl)
					mtk_cam_ctrl_send_event(same_ts_raw_ctrl, CAMSYS_EVENT_IRQ_TRY_TS_TRIGGER);
			}
		}
	}

	mtk_cam_job_put(job);
}

static void handle_ss_try_set_sensor(struct mtk_cam_ctrl *cam_ctrl)
{
	mtk_cam_ctrl_send_event(cam_ctrl, CAMSYS_EVENT_TIMER_SENSOR);
}

static void ctrl_vsync_history_push(struct vsync_collector *c,
				    int engine, int engine_id, u64 ts)
{
	struct vsync_history *h;

	spin_lock(&c->history_lock);
	h = c->history + c->cur_history_idx;

	h->engine = engine;
	h->id = engine_id;
	h->ts_ns = ts;

	c->cur_history_idx = (c->cur_history_idx + 1) % VSYNC_HIST_NUM;

	spin_unlock(&c->history_lock);
}

static int vsync_history_prev_idx(unsigned int i)
{
	int idx = i - 1;

	return idx < 0 ? VSYNC_HIST_NUM - 1 : idx;
}

void vsync_collector_dump(struct vsync_collector *c)
{
	struct vsync_history *history;
	int i;
	int idx;

	history = kmalloc(sizeof(c->history), GFP_ATOMIC);
	if (!history) {
		pr_info("%s: failed to alloc for dump\n", __func__);
		return;
	}

	spin_lock(&c->history_lock);

	memcpy(history, c->history, sizeof(c->history));
	idx = c->cur_history_idx;

	spin_unlock(&c->history_lock);

	pr_info("%s: === dump begin ===\n", __func__);
	i = 0;
	idx = vsync_history_prev_idx(idx);
	while (i < VSYNC_HIST_NUM) {
		struct vsync_history *h = history + idx;
		u64 ts = h->ts_ns;

		if (!ts)
			break;

		/* note:
		 * this timestamp is not consitent w. the local_clock() used in printk
		 */
		pr_info("%s: [%d] engine %d-%d, ts %llu\n",
			__func__, i, h->engine, h->id, ts);

		idx = vsync_history_prev_idx(idx);
		++i;
	}

	kfree(history);
}

static void ctrl_vsync_preprocess_extisp(struct mtk_cam_ctrl *ctrl,
				  enum MTK_CAMSYS_ENGINE_TYPE engine_type,
				  unsigned int engine_id,
				  struct mtk_camsys_irq_info *irq_info,
				  struct vsync_result *vsync_res)
{
	bool hint_inner_err = 0;
	int cookie;
	long cq_not_ready, inner_not_ready;
	struct apply_cq_ref *cq_ref;

	ctrl_vsync_history_push(&ctrl->vsync_col,
				engine_type, engine_id, irq_info->ts_ns);

	if (vsync_update_extisp(ctrl, engine_type,
			irq_info->irq_type, engine_id, vsync_res))
		return;

	cq_ref = READ_ONCE(ctrl->cur_cq_ref);
	vsync_res->inner_cookie =
		(vsync_res->is_first && cq_ref) ? cq_ref->cookie : -1;

	spin_lock(&ctrl->info_lock);

	if (vsync_res->is_first) {
		ctrl->r_info.sof_ts_ns = irq_info->ts_ns;
		ctrl->r_info.sof_ts_mono_ns = ktime_get_ns();
	}

	if (vsync_res->is_last) {
		ctrl->r_info.sof_l_ts_ns = irq_info->ts_ns;
		ctrl->r_info.sof_l_ts_mono_ns = ktime_get_ns();

		if (cq_ref) {
			if (apply_cq_ref_is_to_inner(cq_ref)) {
				ctrl->r_info.inner_seq_no =
					seq_from_fh_cookie(cq_ref->cookie);
				WRITE_ONCE(ctrl->cur_cq_ref, 0);
			} else {
				hint_inner_err = 1;
				cookie = cq_ref->cookie;
				cq_not_ready =
					atomic_long_read(&cq_ref->cq_not_ready);
				inner_not_ready =
					atomic_long_read(&cq_ref->inner_not_ready);
			}
		}
	}
	ctrl->r_info.inner_seq_no = seq_from_fh_cookie(irq_info->frame_idx_inner);
	spin_unlock(&ctrl->info_lock);
	if (ctrl->r_info.extisp_enable) {
		if (engine_type == CAMSYS_ENGINE_RAW &&
			ctrl->r_info.extisp_enable & BIT(EXTISP_DATA_PROCRAW))
			ctrl->r_info.extisp_tg_cnt[EXTISP_DATA_PROCRAW] = irq_info->tg_cnt;
		if (engine_type == CAMSYS_ENGINE_CAMSV &&
			ctrl->r_info.extisp_enable & BIT(EXTISP_DATA_META))
			ctrl->r_info.extisp_tg_cnt[EXTISP_DATA_META] = irq_info->tg_cnt;
		if (engine_type == CAMSYS_ENGINE_MRAW &&
			ctrl->r_info.extisp_enable & BIT(EXTISP_DATA_PD))
			ctrl->r_info.extisp_tg_cnt[EXTISP_DATA_PD] = irq_info->tg_cnt;
		pr_info("%s: extisp vysnc engine_type:%d, tg_cnt:%d, pd/meta/procraw:%d/%d/%d\n",
			__func__, engine_type, irq_info->tg_cnt,
			vsync_res->is_first, vsync_res->is_extmeta, vsync_res->is_last);
	}
	if (hint_inner_err)
		pr_info("%s: warn. inner not updated to 0x%x, cq not ready: 0x%lx engine not ready: 0x%lx\n",
			__func__, cookie, cq_not_ready, inner_not_ready);
}

static void ctrl_vsync_preprocess(struct mtk_cam_ctrl *ctrl,
				  enum MTK_CAMSYS_ENGINE_TYPE engine_type,
				  unsigned int engine_id,
				  struct mtk_camsys_irq_info *irq_info,
				  struct vsync_result *vsync_res)
{
	bool hint_inner_err = 0;
	int cookie;
	long cq_not_ready, inner_not_ready;
	struct apply_cq_ref *cq_ref;

	ctrl_vsync_history_push(&ctrl->vsync_col,
				engine_type, engine_id, irq_info->ts_ns);

	if (vsync_update(&ctrl->vsync_col, engine_type,
			irq_info->irq_type, engine_id, vsync_res))
		return;

	cq_ref = READ_ONCE(ctrl->cur_cq_ref);
	vsync_res->inner_cookie =
		(vsync_res->is_first && cq_ref) ? cq_ref->cookie : -1;

	if (vsync_res->is_last && ctrl->hw_hang_count_down) {

		pr_info("%s: hw_hang_count_down %d\n", __func__,
			ctrl->hw_hang_count_down);

		--ctrl->hw_hang_count_down;
		if (!ctrl->hw_hang_count_down)
			mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_HW_HANG);
	}

	spin_lock(&ctrl->info_lock);

	if (vsync_res->is_first) {
		ctrl->r_info.sof_ts_ns = irq_info->ts_ns;
		ctrl->r_info.sof_ts_mono_ns = ktime_get_ns();
	}

	if (vsync_res->is_last) {
		ctrl->r_info.sof_l_ts_ns =
			max(irq_info->ts_ns, ctrl->r_info.sof_ts_ns);
		ctrl->r_info.sof_l_ts_mono_ns =
			max(ktime_get_ns(), ctrl->r_info.sof_ts_mono_ns);

		if (cq_ref) {
			if (apply_cq_ref_is_to_inner(cq_ref)) {
				ctrl->r_info.inner_seq_no =
					seq_from_fh_cookie(cq_ref->cookie);
				WRITE_ONCE(ctrl->cur_cq_ref, 0);
			} else {
				vsync_res->is_last = 0;
				hint_inner_err = 1;
				cookie = cq_ref->cookie;
				cq_not_ready =
					atomic_long_read(&cq_ref->cq_not_ready);
				inner_not_ready =
					atomic_long_read(&cq_ref->inner_not_ready);
			}
		}

		/* note:
		 *   cpu0 busy or low performance may causes sof out of order.
		 *   to correct this order, rewait sof that module is not to inner.
		 */
		if (hint_inner_err)
			vsync_rewait(&ctrl->vsync_col, inner_not_ready);
		else
			vsync_clear_collected(&ctrl->vsync_col);
	}

	spin_unlock(&ctrl->info_lock);

	if (hint_inner_err)
		pr_info("%s: warn. inner not updated to 0x%x, cq not ready: 0x%lx engine not ready: 0x%lx\n",
			__func__, cookie, cq_not_ready, inner_not_ready);
}

static int frame_no_to_fs_req_no(struct mtk_cam_ctrl *ctrl, int frame_no,
				 int *req_no)
{
	struct mtk_cam_job *job;
	int do_send_evnt;

	if (frame_no == -1)
		goto SKIP_FIND_JOB;

	job = mtk_cam_ctrl_get_job(ctrl, cond_frame_no_belong, &frame_no);
	if (job) {

		if (ctrl->frame_sync_event_cnt != job->req_seq) {
			ctrl->fs_event_subframe_cnt = job->frame_cnt;
			ctrl->fs_event_subframe_idx = 0;
		}

		ctrl->frame_sync_event_cnt = job->req_seq;

		mtk_cam_job_put(job);
	} else {
		spin_lock(&ctrl->info_lock);
		ctrl->frame_sync_event_cnt = ctrl->r_info.done_seq_no;
		spin_unlock(&ctrl->info_lock);
	}

SKIP_FIND_JOB:
	do_send_evnt = ctrl->fs_event_subframe_idx == 0;
	ctrl->fs_event_subframe_idx =
		(ctrl->fs_event_subframe_idx + 1) % ctrl->fs_event_subframe_cnt;

	if (req_no)
		*req_no = ctrl->frame_sync_event_cnt;

	return do_send_evnt;
}

static void handle_engine_frame_start(struct mtk_cam_ctrl *ctrl,
				      struct mtk_camsys_irq_info *irq_info,
				      struct vsync_result *vsync_res)
{

	if (vsync_res->is_first) {
		int frame_sync_no;
		int req_no;

		if (vsync_res->inner_cookie != -1)
			frame_sync_no =
				seq_from_fh_cookie(vsync_res->inner_cookie);
		else
			frame_sync_no = -1;

		mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_IRQ_F_VSYNC);

		if (frame_no_to_fs_req_no(ctrl, frame_sync_no, &req_no)) {
			struct mtk_seninf_sof_notify_param param;

			mtk_cam_event_frame_sync(ctrl, req_no);

			/* notify sof to sensor*/
			param.sd = ctrl->ctx->seninf;
			param.sof_cnt = req_no;
			mtk_cam_seninf_sof_notify(&param);
		}
	}

	if (vsync_res->is_last)
		mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_IRQ_L_SOF);

	if (vsync_res->is_extmeta)
		mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_IRQ_EXTMETA_SOF);

}

static void handle_tuning_update(struct mtk_cam_ctrl *ctrl, int seq_no, u64 ts_ns)
{
	struct mtk_cam_job *job;

	job = mtk_cam_ctrl_get_job(ctrl, cond_frame_no_belong, &seq_no);
	if (!job) {
		pr_info("%s: warn. job not found seq 0x%x\n",
			__func__, seq_no);
		return;
	}

	if (atomic_cmpxchg(&job->tuning_work_queued, 0, 1)) {
		pr_info("%s: warn. tuning work is queued 0x%x\n", __func__, seq_no);
		mtk_cam_job_put(job);
		return;
	}

	job->tuning_param.begin_ts_ns = ts_ns;
	spin_lock(&ctrl->info_lock);
	job->tuning_param.sof_boottime_ns = ctrl->r_info.sof_ts_ns;
	spin_unlock(&ctrl->info_lock);

	if (mtk_cam_ctx_queue_tuning_worker(ctrl->ctx, &job->tuning_work))
		mtk_cam_job_put(job);
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


	/* raw's SOF (proc engine frame start) */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_START)) {
		struct vsync_result vsync_res;

		memset(&vsync_res, 0, sizeof(vsync_res));
		if (ctrl->r_info.extisp_enable)
			ctrl_vsync_preprocess_extisp(ctrl,
				      CAMSYS_ENGINE_RAW, engine_id, irq_info,
				      &vsync_res);
		else
			ctrl_vsync_preprocess(ctrl,
				      CAMSYS_ENGINE_RAW, engine_id, irq_info,
				      &vsync_res);

		handle_engine_frame_start(ctrl, irq_info,
					  &vsync_res);
	}

	/* raw's subsample n-2 vsync coming */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_TRY_SENSOR_SET))
		handle_ss_try_set_sensor(ctrl);

	/* note: should handle SOF before CQ done for trigger delay cases */
	/* raw's CQ done */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_SETTING_DONE)) {
		if (ctrl->r_info.timeshare_enable) {
			spin_lock(&ctrl->info_lock);
			ctrl->r_info.outer_seq_no_ts =
				seq_from_fh_cookie(irq_info->frame_idx);
			spin_unlock(&ctrl->info_lock);
		} else {
			spin_lock(&ctrl->info_lock);
			ctrl->r_info.outer_seq_no =
				seq_from_fh_cookie(irq_info->frame_idx);
			spin_unlock(&ctrl->info_lock);
		}
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
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_START) ||
		irq_info->irq_type & BIT(CAMSYS_IRQ_FRAME_START_DCIF_MAIN)) {
		struct vsync_result vsync_res;

		memset(&vsync_res, 0, sizeof(vsync_res));
		if (ctrl->r_info.extisp_enable)
			ctrl_vsync_preprocess_extisp(ctrl,
				      CAMSYS_ENGINE_CAMSV, engine_id, irq_info,
				      &vsync_res);
		else
			ctrl_vsync_preprocess(ctrl,
				      CAMSYS_ENGINE_CAMSV, engine_id, irq_info,
				      &vsync_res);

		handle_engine_frame_start(ctrl, irq_info,
					  &vsync_res);

		qof_dump_ctx(ctrl->ctx, qof_dump_power_state);
	}

	/* note: should handle SOF before CQ done for trigger delay cases */
	/* camsv's CQ done */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_SETTING_DONE)) {
		spin_lock(&ctrl->info_lock);
		ctrl->r_info.outer_seq_no =
			seq_from_fh_cookie(irq_info->frame_idx);
		spin_unlock(&ctrl->info_lock);
		if (extisp_listen_each_cq_done(ctrl) ||
			timeshare_pureraw_pd_cq_done(ctrl))
			handle_extmeta_setting_done(ctrl);
		else
			handle_setting_done(ctrl);
	}

	/* ois compensation */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_TUNING_UPDATE))
		handle_tuning_update(ctrl,
			seq_from_fh_cookie(irq_info->frame_idx_inner), irq_info->ts_ns);

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

		memset(&vsync_res, 0, sizeof(vsync_res));
		if (ctrl->r_info.extisp_enable)
			ctrl_vsync_preprocess_extisp(ctrl,
				      CAMSYS_ENGINE_MRAW, engine_id, irq_info,
				      &vsync_res);
		else
			ctrl_vsync_preprocess(ctrl,
				      CAMSYS_ENGINE_MRAW, engine_id, irq_info,
				      &vsync_res);

		handle_engine_frame_start(ctrl, irq_info,
					  &vsync_res);
	}

	/* note: should handle SOF before CQ done for trigger delay cases */
	/* mraw's CQ done */
	if (irq_info->irq_type & BIT(CAMSYS_IRQ_SETTING_DONE)) {
		spin_lock(&ctrl->info_lock);
		ctrl->r_info.outer_seq_no =
			seq_from_fh_cookie(irq_info->frame_idx);
		spin_unlock(&ctrl->info_lock);
		if (timeshare_pureraw_pd_cq_done(ctrl))
			handle_extmeta_setting_done(ctrl);
		else
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

	MTK_CAM_TRACE_BEGIN(BASIC, "irq_type %d, inner 0x%x",
			    irq_info->irq_type, irq_info->frame_idx_inner);

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

	MTK_CAM_TRACE_END(BASIC);

	return ret;
}

/* raw switch also resue it to stream on */
static int mtk_cam_ctrl_stream_on_job(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_ctrl *ctrl = &ctx->cam_ctrl;
	struct device *dev = ctx->cam->dev;

	if (job->do_pending_aid_config) {
#ifdef SUPPORT_SLB_DC
		if (is_dc_mode(job) && ctx->slb_addr)
			mtk_cam_hsf_aid(ctx, 1, AID_CAM_DC, job->used_engine);
#else
		if (is_dc_mode(job) && ctx->slc_data_valid)
			mtk_cam_hsf_aid(ctx, 1, AID_CAM_DC, job->used_engine);
#endif
	}

	if (mtk_cam_job_manually_apply_sensor(job))
		goto STREAM_ON_FAIL;

	if (mtk_cam_job_manually_apply_isp_sync(job))
		goto STREAM_ON_FAIL;

	ctrl->frame_interval_ns =
			mtk_cam_query_interval_from_sensor(ctx->sensor);

	/* should set ts for second job's apply_sensor */
	ctrl->r_info.sof_ts_ns = ktime_get_boottime_ns();
	ctrl->r_info.sof_l_ts_ns = ktime_get_boottime_ns();
	ctrl->fs_event_subframe_cnt = job->frame_cnt;

	call_jobop(job, stream_on, true);
	if (ctrl->r_info.extisp_enable ||
		!ctx->has_raw_subdev)
		mtk_cam_event_extisp_camsys_ready(ctrl);
	mtk_cam_watchdog_start(&ctrl->watchdog, 1);

	return 0;

STREAM_ON_FAIL:
	dev_info(dev, "%s: failed. ctx=%d\n", __func__, ctx->stream_id);
	mtk_smi_dbg_hang_detect("camsys-raw");
	mtk_cam_event_error(ctrl, MSG_STREAM_ON_ERROR);
	WRAP_AEE_EXCEPTION(MSG_STREAM_ON_ERROR, "stream on failed");
	return -1;
}

static void trigger_fake_sof_event(struct mtk_cam_ctrl *ctrl)
{
	/*
	 * 'Fake' sof event to trigger next 'enqueued' job's apply sensor.
	 *
	 * We need to trigger 2nd frame's apply sensor, though some CID may be
	 * skipped by sensor driver due to sensor constaint.
	 *
	 *  sensor
	 *     i2c  oo1oo       (oo2oo)      oo3o
	 *     exp                  \\1\\\       \\2\\\
	 *    vsync                      |            |
	 *                    | <- 'fake' vsync event
	 *         ---------^-----------------------------------------
	 *             (stream-on)
	 *     isp                       xx1xxxx      xx2xxxx
	 *
	 * special cases:
	 *   - mstream: 1 job contains 2 frames
	 *   - smvr: different applying order for sensor/isp
	 *   - lbmf: trigger sensor via last sof
	 */

	/* note: on purpose not to update ctrl's runtime info */
	pr_info("%s:ctx=%d, sof:%lld, sof_l:%lld, ts:%lld\n", __func__,
		ctrl->ctx->stream_id, ctrl->r_info.sof_ts_ns,
		ctrl->r_info.sof_l_ts_ns, ktime_get_boottime_ns());
	mtk_cam_ctrl_send_event(ctrl, CAMSYS_EVENT_IRQ_L_SOF);
}

static void mtk_cam_ctrl_stream_on_flow(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_ctrl *ctrl = &ctx->cam_ctrl;
	struct device *dev = ctx->cam->dev;

	dev_info(dev, "[%s] ctx %d begin\n", __func__, ctrl->ctx->stream_id);
	mtk_cam_job_update_clk(job);
	if (mtk_cam_ctrl_stream_on_job(job))
		return;

	atomic_dec(&ctrl->stream_on_cnt);
	mtk_cam_ctrl_loop_job(ctrl, ctrl_enable_job_fsm_until_switch, NULL);

	trigger_fake_sof_event(ctrl);

	dev_info(dev, "[%s] ctx %d finish\n", __func__, ctrl->ctx->stream_id);
}

static int dynamic_raw_change_stream_on(struct mtk_cam_job *job, int unit_engs)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_cam_ctrl *ctrl = &ctx->cam_ctrl;
	int i;

	if (job->raw_change) {
		if (job->raw_change == JOB_RAW_MASTER_UNCHANGED) {
			for (i = 0; i < cam->engines.num_raw_devices; i++) {
				if (BIT(i) & job->master_engine) {
					struct mtk_raw_device *raw_dev;

					raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);
					/* OTF mode - master raw check*/
					if (!raw_dev->is_slave && !is_dc_mode(job))
						check_master_raw_vf_en(raw_dev);
				}
			}
		} else {
			vsync_set_desired(&ctrl->vsync_col, job->master_engine);
			call_jobop(job, stream_on, true);
		}
	}

	return 0;
}

static void mtk_cam_ctrl_dynamic_raws_change_flow(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_ctrl *ctrl = &ctx->cam_ctrl;
	struct device *dev = ctx->cam->dev;
	struct mtk_cam_device *cam = ctx->cam;
	struct seamless_check_args check_args;
	int prev_seq;
	int no = job->frame_seq_no;
	int i;
	int engine_uninit = job->raw_change_uninit_engine;
	int raw_after_change = bit_map_subset_of(MAP_HW_RAW, job->used_engine);
	int raw_uninit = bit_map_subset_of(MAP_HW_RAW, job->raw_change_uninit_engine);
	int ois_comp = is_ois_compensation(job);

	dev_info(dev, "[%s] begin waiting 1.dynamic raw changes no:%d seq 0x%x cq done\n",
		__func__, job->req_seq, job->frame_seq_no);

	for (i = 0; i < cam->engines.num_raw_devices; i++) {
		if (BIT(i) & (raw_after_change | raw_uninit)) {
			struct mtk_raw_device *raw_dev;

			raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);

			qof_mtcmos_raw_voter(raw_dev, true);
			raw_dev->log_en = true;
		}
	}

#ifdef QOF_CCU_READY
	if (GET_PLAT_HW(qof_support) && mtk_cam_power_ctrl_ccu(cam->dev, 1))
		goto SWITCH_FAILURE;
#endif

	dev_info(dev, "[%s] master raw changed case : wait engines:0x%x setting done\n",
			 __func__, ctx->used_engine);
	if (mtk_cam_ctrl_wait_event(ctrl, check_setting_done, &no, 30000)) {
		dev_info(dev, "[%s] check for dynamic_raws_change timeout: outer = 0x%x\n",
				 __func__, no);
		goto SWITCH_FAILURE;
	}

	mtk_cam_job_update_clk_switching(job, 1);

	if (dynamic_raw_change_stream_on(job, engine_uninit))
		goto SWITCH_FAILURE;

	prev_seq = prev_frame_seq(job->frame_seq_no);
	dev_info(dev, "[%s] wait 2.prev engines done req:0x%x\n",
			__func__, prev_seq);
	if (mtk_cam_ctrl_wait_event(ctrl, check_done, &prev_seq, 30000)) {
		dev_info(dev, "[%s] check for dynamic_raws_change timeout: prev_seq=0x%x\n",
			 __func__, prev_seq);
		goto SWITCH_FAILURE;
	}

	if (!ois_comp) {
		if (mtk_cam_job_uninit_engine(job, engine_uninit)) {
			dev_info(dev, "[%s] uninit engine failed, uninit raw:0x%x\n",
				__func__, job->raw_change_uninit_engine);
			goto SWITCH_FAILURE;
		}

		if (engine_uninit)
			mtk_cam_event_camsys_resource_ready(&ctx->cam_ctrl, engine_uninit);
	}

	/* NOTE: qof_setup_twin has been called in job_raw_change_hw_init */
	for (i = 0; i < cam->engines.num_raw_devices; i++) {
		bool is_master = false;
		struct mtk_raw_device *raw_dev;

		if (BIT(i) & raw_after_change) {
			bool next_raw =
				(i + 1 < cam->engines.num_raw_devices) &&
				(BIT(i + 1) & raw_after_change);

			raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);
			is_master = (BIT(i) & raw_after_change) && !raw_dev->is_slave;

			qof_setup_twin(raw_dev, is_master, next_raw);
		} else if (BIT(i) & raw_uninit) {
			raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);
			qof_setup_twin(raw_dev, true, false);
		}
	}

	dev_info(dev, "[%s] wait 3.new engines(0x%x) processing seq:0x%x\n",
			__func__, ctx->used_engine, job->frame_seq_no);
	check_args.expect_inner = job->frame_seq_no;
	check_args.expect_ack = job->frame_seq_no;
	if (mtk_cam_ctrl_wait_event(ctrl, check_for_inner, &check_args, 30000)) {
		dev_info(dev, "[%s] check for dynamic_raws_change timeout: expected in=0x%x ack=0x%x\n",
			 __func__,
			 check_args.expect_inner, check_args.expect_ack);
		goto SWITCH_FAILURE;
	}

	if (ois_comp) {
		if (mtk_cam_job_uninit_engine(job, engine_uninit)) {
			dev_info(dev, "[%s] uninit engine failed, uninit raw:0x%x\n",
				__func__, job->raw_change_uninit_engine);
			goto SWITCH_FAILURE;
		}

		if (engine_uninit)
			mtk_cam_event_camsys_resource_ready(&ctx->cam_ctrl, engine_uninit);
	}

	mtk_cam_job_update_clk_switching(job, 0);

	if (is_stagger_dol(job))
		qof_mtcmos_voter_handle(&ctx->cam->engines,
			raw_after_change, &ctx->DOL_not_support);
	else
		qof_mtcmos_voter_handle(&ctx->cam->engines,
			0, &ctx->DOL_not_support);

	for (i = 0; i < cam->engines.num_raw_devices; i++) {
		if (BIT(i) & raw_after_change) {
			struct mtk_raw_device *raw_dev;

			raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);
			qof_mtcmos_raw_voter(raw_dev, false);
			raw_dev->log_en = false;
		}
	}

#ifdef QOF_CCU_READY
	if (GET_PLAT_HW(qof_support))
		mtk_cam_power_ctrl_ccu(cam->dev, 0);
#endif

	dev_info(dev, "[%s] finish, uninit engines:0x%x, new frame inner:%d\n",
		__func__, engine_uninit, check_args.expect_inner);

	return;

SWITCH_FAILURE:
#ifdef QOF_CCU_READY
	if (GET_PLAT_HW(qof_support))
		mtk_cam_power_ctrl_ccu(cam->dev, 0);
#endif
	dev_info(dev, "[%s] failed: ctx-%d job %d frame_seq 0x%x\n",
		 __func__, ctx->stream_id, job->req_seq, job->frame_seq_no);

	vsync_collector_dump(&ctrl->vsync_col);
	if (mtk_cam_seninf_dump(ctx->seninf, job->frame_seq_no, true, true)
		!= -ESTRPIPE) {
		mtk_engine_dump_debug_status(ctx->cam, job->used_engine, DD_DUMP_NONE);
		mtk_cam_job_uninit_engine(job, engine_uninit);
		WRAP_AEE_EXCEPTION(MSG_RAW_CHANGE_FAILURE, __func__);
	} else {
		mtk_cam_event_error(ctrl, MSG_SENINF_FRAME_ERROR);
	}
}

static void mtk_cam_ctrl_seamless_switch_flow(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_ctrl *ctrl = &ctx->cam_ctrl;
	struct device *dev = ctx->cam->dev;
	struct mtk_cam_device *cam = ctx->cam;
	struct seamless_check_args check_args;
	int prev_seq;
	int i;
	int engine_uninit = job->raw_change_uninit_engine;
	int raw_after_change = bit_map_subset_of(MAP_HW_RAW, job->used_engine);
	int raw_uninit = bit_map_subset_of(MAP_HW_RAW, job->raw_change_uninit_engine);
	int raw_all = raw_after_change | raw_uninit;

	dev_info(dev, "[%s] begin waiting switch no:%d seq 0x%x\n",
		__func__, job->req_seq, job->frame_seq_no);

	prev_seq = prev_frame_seq(job->frame_seq_no);
	check_args.expect_inner = prev_seq;
	check_args.expect_ack = job->frame_seq_no;

	for (i = 0; i < cam->engines.num_raw_devices; i++) {
		if (BIT(i) & raw_all) {
			struct mtk_raw_device *raw_dev;

			raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);

			qof_mtcmos_raw_voter(raw_dev, true);
			raw_dev->log_en = true;
		}
	}

#ifdef QOF_CCU_READY
	if (GET_PLAT_HW(qof_support) && mtk_cam_power_ctrl_ccu(cam->dev, 1))
		goto SWITCH_FAILURE;
#endif

	if (mtk_cam_ctrl_wait_event(ctrl, check_for_seamless, &check_args,
				    5000)) {
		int inner, ack;

		spin_lock(&ctrl->info_lock);
		inner = ctrl->r_info.inner_seq_no;
		ack = ctrl->r_info.ack_seq_no;
		spin_unlock(&ctrl->info_lock);
		dev_info(dev, "[%s] check_for_seamless timeout: expected in=0x%x ack=0x%x (in=0x%x ack=0x%x)\n",
			 __func__,
			 check_args.expect_inner, check_args.expect_ack,
			 inner, ack);
		mtk_cam_job_uninit_engine(job, job->raw_change_uninit_engine);
		goto SWITCH_FAILURE;
	}
	if (atomic_read(&ctx->streaming) == 0) {
		mtk_cam_job_uninit_engine(job, job->raw_change_uninit_engine);
		goto SWITCH_FAILURE;
	}
	mtk_cam_job_update_clk_switching(job, 1);

	if (dynamic_raw_change_stream_on(job, engine_uninit))
		goto SWITCH_FAILURE;

	call_job_seamless_ops(job, before_sensor);
	dev_info(dev, "[%s] begin sensor mode switch seq 0x%x, %llu/%llu\n",
		__func__, job->frame_seq_no,
		ktime_get_boottime_ns() - ctrl->r_info.sof_ts_ns,
		ktime_get_boottime_ns() - ctrl->r_info.sof_l_ts_ns);
	mtk_cam_job_manually_apply_sensor(job);

	for (i = 0; i < cam->engines.num_raw_devices; ++i) {
		struct mtk_raw_device *raw = NULL;
		struct mtk_raw_ctrl_data *ctrl = NULL;
		const struct mtk_cam_resource_v2 *res = NULL;
		int exp, sv_last_tag;

		if (!(BIT(i) & raw_after_change))
			continue;

		raw = dev_get_drvdata(cam->engines.raw_devs[i]);
		ctrl = get_raw_ctrl_data(job);

		res = &ctrl->resource.user_data;
		exp = job_exp_num(job);
		sv_last_tag = (exp == 1) ?
		get_sv_tag_idx(exp, MTKCAM_IPI_ORDER_FIRST_TAG, false) :
		get_sv_tag_idx(exp, MTKCAM_IPI_ORDER_LAST_TAG, false);

		qof_sof_src_sel(raw, job_exp_num(job),
					!res_raw_is_dc_mode(&res->raw_res), sv_last_tag);
		qof_setup_hw_timer(raw, get_sensor_interval_us(job));
		qof_set_cq_start_max(raw, -1);
		qof_enable_cq_trigger_by_qof(raw, false);
	}

	if (call_job_seamless_ops(job, after_sensor))
		goto SWITCH_FAILURE;

	vsync_set_desired(&ctrl->vsync_col, job->master_engine);

	if (mtk_cam_ctrl_wait_event(ctrl, check_done, &prev_seq, 4999)) {
		dev_info(dev, "[%s] check_done timeout: prev_seq=0x%x\n",
			 __func__, prev_seq);
		mtk_cam_job_uninit_engine(job, job->raw_change_uninit_engine);
		goto SWITCH_FAILURE;
	}
	/* should set ts for next job's apply_sensor */
	ctrl->frame_interval_ns =
			mtk_cam_query_interval_from_sensor(ctx->sensor);
	ctrl->r_info.sof_ts_ns = ktime_get_boottime_ns();
	ctrl->r_info.sof_l_ts_ns = ctrl->r_info.sof_ts_ns;

	for (i = 0; i < cam->engines.num_raw_devices; i++) {
		bool is_master = false;
		struct mtk_raw_device *raw_dev;

		if (BIT(i) & raw_after_change) {
			bool next_raw =
				(i + 1 < cam->engines.num_raw_devices) &&
				(BIT(i + 1) & raw_after_change);

			raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);
			is_master = (BIT(i) & raw_after_change) && !raw_dev->is_slave;

			qof_setup_twin(raw_dev, is_master, next_raw);
		} else if (BIT(i) & raw_uninit) {
			raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);
			qof_setup_twin(raw_dev, true, false);
		}
	}

	call_job_seamless_ops(job, after_prev_frame_done);

	/* uninit_eng in after_prev_frame_done */
	if (engine_uninit)
		mtk_cam_event_camsys_resource_ready(&ctx->cam_ctrl, engine_uninit);

	if (is_stagger_dol(job))
		qof_mtcmos_voter_handle(&ctx->cam->engines,
			raw_after_change, &ctx->DOL_not_support);
	else
		qof_mtcmos_voter_handle(&ctx->cam->engines,
			0, &ctx->DOL_not_support);

	trigger_fake_sof_event(ctrl);
	check_args.expect_inner = job->frame_seq_no;
	dev_info(dev, "[%s] begin waiting check for inner no:%d seq 0x%x\n",
		__func__, job->req_seq, job->frame_seq_no);
	if (mtk_cam_ctrl_wait_event(ctrl, check_for_inner, &check_args,
				    30000)) {
		dev_info(dev, "[%s] check_for_inner timeout: expected in=0x%x\n",
			 __func__, check_args.expect_inner);
		goto SWITCH_FAILURE;
	}

	mtk_cam_job_update_clk_switching(job, 0);

	for (i = 0; i < cam->engines.num_raw_devices; ++i) {
		struct mtk_raw_device *raw = NULL;

		if (!(BIT(i) & raw_after_change))
			continue;

		raw = dev_get_drvdata(cam->engines.raw_devs[i]);
		qof_enable_cq_trigger_by_qof(raw, true);
	}

	for (i = 0; i < cam->engines.num_raw_devices; i++) {
		if (BIT(i) & raw_after_change) {
			struct mtk_raw_device *raw_dev;

			raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);
			qof_mtcmos_raw_voter(raw_dev, false);
			raw_dev->log_en = false;
		}
	}

#ifdef QOF_CCU_READY
	if (GET_PLAT_HW(qof_support))
		mtk_cam_power_ctrl_ccu(cam->dev, 0);
#endif

	dev_info(dev, "[%s] finish, used_engine:0x%x\n",
		 __func__, job->used_engine);
	return;

SWITCH_FAILURE:
#ifdef QOF_CCU_READY
	if (GET_PLAT_HW(qof_support))
		mtk_cam_power_ctrl_ccu(cam->dev, 0);
#endif
	dev_info(dev, "[%s] failed: ctx-%d job %d frame_seq 0x%x (streaming:%d)\n",
		 __func__, ctx->stream_id, job->req_seq, job->frame_seq_no,
		 atomic_read(&ctx->streaming));
	vsync_collector_dump(&ctrl->vsync_col);
	if (atomic_read(&ctx->streaming))
		WRAP_AEE_EXCEPTION(MSG_SWITCH_FAILURE, __func__);
	for (i = 0; i < cam->engines.num_raw_devices; i++) {
		if (BIT(i) & raw_all) {
			struct mtk_raw_device *raw_dev;

			raw_dev = dev_get_drvdata(cam->engines.raw_devs[i]);
			qof_mtcmos_raw_voter(raw_dev, false);
		}
	}
	if (atomic_read(&ctx->streaming))
		mtk_cam_watchdog_schedule_job_dump(&ctrl->watchdog,
			is_dc_mode(job) ? MSG_DC_SKIP_FRAME : MSG_DEQUE_ERROR);
}

static void mtk_cam_ctrl_raw_switch_flow(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_ctrl *ctrl = &ctx->cam_ctrl;
	struct device *dev = ctx->cam->dev;
	int prev_seq;
	int r;

	prev_seq = prev_frame_seq(job->frame_seq_no);
	if (mtk_cam_ctrl_wait_event(ctrl, check_done, &prev_seq, 30000))
		dev_info(dev, "[%s] check_done timeout: prev_seq=0x%x\n",
			 __func__, prev_seq);

	dev_info(dev, "[%s] begin waiting raw switch no:%d\n",
		 __func__, job->frame_seq_no);

	/* stop the isp but doesn't power off raws */

	mtk_cam_ctx_engine_off(ctx);
	mtk_cam_watchdog_stop(&ctrl->watchdog);
	/* disable irq first */
	mtk_cam_ctx_engine_disable_irq(ctx);
	mtk_cam_ctx_engine_reset(ctx);

	mtk_cam_ctx_engine_reset_msgfifo(ctx);

	/* re-initialized the new stream required engines with raw switch flow */
	mtk_cam_job_initialize_engines(ctx, job, job->init_params);

	/**
	 * Stream off the previous sensor
	 * TODO: (Fred)
	 * Split the cammux disabling part from this function.
	 * We just want to disable cammux here.
	 */
	r = ctx_stream_off_seninf_sensor(ctx);
	if (r)
		dev_info(dev,
			 "[%s] failed to stream off the sensor:%d\n",
			 __func__, r);
	else
		dev_info(dev,
			 "[%s] stream off sensor %s\n",
			__func__, job->seninf_prev->entity.name);

	mtk_cam_job_clean_prev_img_pool(job);

	mtk_cam_watchdog_init(&ctrl->watchdog);

	/* TBC: do we need vsync_set_desired here? */
	vsync_set_desired(&ctrl->vsync_col, job->master_engine);

	/* Update the context's sensor and seninf */
	ctx->sensor = job->sensor;
	ctx->seninf = job->seninf;

	mtk_cam_job_update_clk(job);

	/* enable irq before stream on */
	mtk_cam_ctx_engine_enable_irq(ctx);
	/**
	 * Reuse stream on flow to start the new stream of the new sensor
	 */
	mtk_cam_ctrl_stream_on_job(job);
	atomic_dec(&ctrl->stream_on_cnt);
	mtk_cam_ctrl_loop_job(ctrl, ctrl_enable_job_fsm_until_switch, job);

	trigger_fake_sof_event(ctrl);

	dev_info(dev, "[%s] finish, used_engine:0x%x\n",
		 __func__, job->used_engine);
}

static void mtk_cam_ctrl_update_seq(struct mtk_cam_ctrl *ctrl,
				    struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx;
	u32 req_seq, frame_seq;

	ctx = ctrl->ctx;

	frame_seq = ctrl->enqueued_frame_seq_no;
	ctrl->enqueued_frame_seq_no =
		add_frame_seq(ctrl->enqueued_frame_seq_no, job->frame_cnt);

	req_seq = ++ctrl->enqueued_req_cnt;

	mtk_cam_job_set_no(job, req_seq, frame_seq);

	if (CAM_DEBUG_ENABLED(CTRL))
		dev_info(ctx->cam->dev, "[%s] ctx:%d, req_no:%d frame_no:%d\n",
			 __func__, ctx->stream_id, req_seq, frame_seq);
}

struct mtk_cam_flow_work {

	struct kthread_work work;
	struct mtk_cam_job *job;

	void (*exec)(struct mtk_cam_job *job);
};

static void mtk_cam_flow_runner(struct kthread_work *work)
{
	struct mtk_cam_flow_work *flow_work =
		container_of(work, struct mtk_cam_flow_work, work);

	if (flow_work->exec && flow_work->job)
		flow_work->exec(flow_work->job);

	kfree(flow_work);
}

static int mtk_cam_ctrl_queue_job_for_flow(struct mtk_cam_ctrl *ctrl,
					   void (*fn)(struct mtk_cam_job *),
					   struct mtk_cam_job *job)
{
	struct mtk_cam_flow_work *flow_work;

	flow_work = kmalloc(sizeof(*flow_work), GFP_ATOMIC);
	if (WARN_ON(!flow_work))
		return -1;

	kthread_init_work(&flow_work->work, mtk_cam_flow_runner);
	flow_work->job = job;
	flow_work->exec = fn;

	return mtk_cam_ctx_queue_flow_worker(ctrl->ctx,
					     &flow_work->work);
}

static void mtk_cam_ctrl_queue_for_flow_control(struct mtk_cam_ctrl *ctrl,
						struct mtk_cam_job *job)
{
	void (*func)(struct mtk_cam_job *) = NULL;

	if (job->stream_on_seninf)
		func = mtk_cam_ctrl_stream_on_flow;

	if (job->seamless_switch)
		func = mtk_cam_ctrl_seamless_switch_flow;
	else if (job->raw_change)
		func = mtk_cam_ctrl_dynamic_raws_change_flow;

	if (job->raw_switch)
		func = mtk_cam_ctrl_raw_switch_flow;

	if (CAM_DEBUG_ENABLED(CTRL) || func)
		pr_info("%s: job #%d %d/%d/%d/%d %ps\n",
			__func__,
			job->req_seq,
			job->stream_on_seninf,
			job->seamless_switch,
			job->raw_switch,
			job->raw_change,
			func);

	if (func)
		mtk_cam_ctrl_queue_job_for_flow(ctrl, func, job);
}

/* request queue */
void mtk_cam_ctrl_job_enque(struct mtk_cam_ctrl *cam_ctrl,
			    struct mtk_cam_job *job)
{
	if (mtk_cam_ctrl_get(cam_ctrl))
		return;

	mtk_cam_ctrl_update_seq(cam_ctrl, job);

	if (job->seamless_switch)
		mtk_cam_job_set_fsm(job, 0);

	if (job->raw_switch)
		atomic_inc(&cam_ctrl->stream_on_cnt);

	/* initial request */
	if (cam_ctrl->initial_req) {
		cam_ctrl->initial_req = 0;

		vsync_set_desired(&cam_ctrl->vsync_col, job->master_engine);

		if (is_m2m(job)) {

			atomic_set(&cam_ctrl->stream_on_cnt, 0);
			mtk_cam_watchdog_start(&cam_ctrl->watchdog, 0);
		}
		if (is_extisp(job)) {
			cam_ctrl->r_info.extisp_enable = job->extisp_data;
			pr_info("[%s:extisp] ctx:%d, extisp_enable:0x%x\n",
				__func__, cam_ctrl->ctx->stream_id, cam_ctrl->r_info.extisp_enable);
		}
	}

	/* add to statemachine */
	write_lock(&cam_ctrl->list_lock);

	/* note:
	 *   fsm enabling is part of statemachine.
	 *   should be protected by 'cam_ctrl->list_lock'
	 */
	if (atomic_read(&cam_ctrl->stream_on_cnt)) {
		mtk_cam_job_set_fsm(job, 0);
		if (CAM_DEBUG_ENABLED(CTRL))
			pr_info("disable job #%d's fsm, stream_on_cnt(%d)\n",
				job->req_seq, atomic_read(&cam_ctrl->stream_on_cnt));
	}

	list_add_tail(&job->job_state.list, &cam_ctrl->camsys_state_list);
	write_unlock(&cam_ctrl->list_lock);

	/* following would trigger actions */
	mtk_cam_ctrl_send_event(cam_ctrl, CAMSYS_EVENT_ENQUE);
	mtk_cam_ctrl_queue_for_flow_control(cam_ctrl, job);

	mtk_cam_ctrl_put(cam_ctrl);
}
struct mtk_cam_job *mtk_cam_ctrl_get_job_by_req_id(struct mtk_cam_ctrl *cam_ctrl,
	unsigned int req_info_id)
{
	struct mtk_cam_job *job;

	job = mtk_cam_ctrl_get_job(cam_ctrl,
			cond_req_id_equal, &req_info_id);
	if (job)
		mtk_cam_job_put(job);
	return job;
}
void mtk_cam_ctrl_sensor_job_enque(struct mtk_cam_ctrl *cam_ctrl,
			    struct mtk_cam_job *job)
{
	if (mtk_cam_ctrl_get(cam_ctrl))
		return;

	mtk_cam_ctrl_update_seq(cam_ctrl, job);

	if (!job->sensor_hdl_obj) {
		cam_ctrl->sensor_sync_id= job->req_info_id;
		cam_ctrl->sensor_seq = job->req_seq;
		pr_info("no sensor obj: #%d , sync_id:%d\n",
				job->req_seq, job->req_info_id);
	}
	if (job->seamless_switch)
		mtk_cam_job_set_fsm(job, 0);
	if (job->raw_switch)
		atomic_inc(&cam_ctrl->stream_on_cnt);
	mtk_cam_job_set_fsm_compose(job, 0);
	/* add to statemachine */
	write_lock(&cam_ctrl->list_lock);
	/* note:
	 *	 fsm enabling is part of statemachine.
	 *	 should be protected by 'cam_ctrl->list_lock'
	 */
	if (atomic_read(&cam_ctrl->stream_on_cnt)) {
		mtk_cam_job_set_fsm(job, 0);
		if (CAM_DEBUG_ENABLED(CTRL))
			pr_info("disable job #%d's fsm, stream_on_cnt(%d)\n",
				job->req_seq, atomic_read(&cam_ctrl->stream_on_cnt));
	}
	list_add_tail(&job->job_state.list, &cam_ctrl->camsys_state_list);
	write_unlock(&cam_ctrl->list_lock);

	/* following would trigger actions */
	mtk_cam_ctrl_send_event(cam_ctrl, CAMSYS_EVENT_ENQUE);
	mtk_cam_ctrl_put(cam_ctrl);
}
void mtk_cam_ctrl_isp_job_enque(struct mtk_cam_ctrl *cam_ctrl,
			    struct mtk_cam_job *job)
{
	if (mtk_cam_ctrl_get(cam_ctrl))
		return;

	if (job->seamless_switch)
		mtk_cam_job_set_fsm(job, 0);

	/* initial request */
	if (cam_ctrl->initial_req) {
		cam_ctrl->initial_req = 0;

		vsync_set_desired(&cam_ctrl->vsync_col, job->master_engine);

		if (is_m2m(job)) {

			atomic_set(&cam_ctrl->stream_on_cnt, 0);
			mtk_cam_watchdog_start(&cam_ctrl->watchdog, 0);
		}
		if (is_extisp(job)) {
			cam_ctrl->r_info.extisp_enable = job->extisp_data;
			pr_info("[%s:extisp] ctx:%d, extisp_enable:0x%x\n",
				__func__, cam_ctrl->ctx->stream_id, cam_ctrl->r_info.extisp_enable);
		}
		if (is_offline_timeshare(job)) {
			cam_ctrl->r_info.timeshare_enable = 1;
			vsync_set_desired_ts(&cam_ctrl->vsync_col, job->master_engine);
			pr_info("[%s:timeshare] ctx:%d, timeshare_enable:0x%x\n",
				__func__, cam_ctrl->ctx->stream_id, cam_ctrl->r_info.timeshare_enable);
		}
	}

	/* following would trigger actions */
	mtk_cam_job_set_fsm_compose(job, 1);
	mtk_cam_ctrl_send_event(cam_ctrl, CAMSYS_EVENT_ENQUE);
	mtk_cam_ctrl_queue_for_flow_control(cam_ctrl, job);

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

static bool handle_job_done_or_stopped(struct mtk_cam_ctrl *ctrl,
				       struct mtk_cam_job **job,
				       bool *stopped)
{
	struct mtk_cam_job *local_job;

	*stopped = atomic_read(&ctrl->stopped);
	if (*stopped)
		return true;

	local_job = mtk_cam_ctrl_fetch_first_unfinished_job(ctrl);

	if (!local_job)
		return false;

	if (!job_has_done_pending(local_job))
		return false;

	*job = local_job;
	return true;
}

void mtk_cam_ctrl_handle_done_loop(struct mtk_cam_ctrl *ctrl)
{
	struct mtk_cam_ctx *ctx = ctrl->ctx;
	struct device *dev = ctx->cam->dev;
	struct mtk_cam_job *job;
	bool stopped;
	int ret;

	if (CAM_DEBUG_ENABLED(CTRL))
		dev_info(dev, "%s: ctx %d enter\n", __func__, ctx->stream_id);

	do {
		job = NULL;
		stopped = 0;

		ret = wait_event_interruptible(ctrl->done_wq,
			handle_job_done_or_stopped(ctrl, &job, &stopped));

		if (stopped)
			break;

		if (ret)
			continue;

		if (WARN_ON(!job))
			continue;

		ret = job_handle_done(job);
		if (ret > 0)
			mtk_cam_job_put(job);
	} while (1);

	if (CAM_DEBUG_ENABLED(CTRL))
		dev_info(dev, "%s: ctx %d exited\n", __func__, ctx->stream_id);
}

static void mtk_cam_ctrl_done_work(struct kthread_work *work)
{
	struct mtk_cam_ctrl *ctrl =
		container_of(work, struct mtk_cam_ctrl, done_work);

	mtk_cam_ctrl_handle_done_loop(ctrl);
}

static void reset_runtime_info(struct mtk_cam_ctrl *ctrl)
{
	struct mtk_cam_ctrl_runtime_info *info = &ctrl->r_info;

	spin_lock(&ctrl->info_lock);

	memset(info, 0, sizeof(*info));
	info->ack_seq_no = -1;
	info->outer_seq_no = -1;
	info->inner_seq_no = -1;
	info->done_seq_no = -1;
	info->ae_wa_enable = ctrl->ctx->raw_subdev_idx >= 0 &&
		ctrl->ctx->cam->sw_ver != 0x0001;

	spin_unlock(&ctrl->info_lock);
}

void mtk_cam_ctrl_start(struct mtk_cam_ctrl *cam_ctrl, struct mtk_cam_ctx *ctx)
{
	cam_ctrl->ctx = ctx;

	cam_ctrl->initial_req = 1;
	cam_ctrl->enqueued_req_cnt = 0;
	cam_ctrl->enqueued_frame_seq_no = 0;
	/* special case: if failed to find job in 1st vsync */
	cam_ctrl->frame_sync_event_cnt = 1;
	cam_ctrl->fs_event_subframe_cnt = 0;
	cam_ctrl->fs_event_subframe_idx = 0;
	cam_ctrl->frame_interval_ns =
			mtk_cam_query_interval_from_ctrl_data(ctx);
	cam_ctrl->sensor_sync_id = 0;
	cam_ctrl->frame_sync_id = 0;
	cam_ctrl->sensor_seq = 0;
	cam_ctrl->frame_seq = 0;

	atomic_set(&cam_ctrl->stopped, 0);
	atomic_set(&cam_ctrl->stream_on_cnt, 1);

	init_waitqueue_head(&cam_ctrl->event_wq);
	init_waitqueue_head(&cam_ctrl->done_wq);
	init_waitqueue_head(&cam_ctrl->stop_wq);
	kthread_init_work(&cam_ctrl->done_work, mtk_cam_ctrl_done_work);

	spin_lock_init(&cam_ctrl->send_lock);
	rwlock_init(&cam_ctrl->list_lock);
	INIT_LIST_HEAD(&cam_ctrl->camsys_state_list);
	init_waitqueue_head(&cam_ctrl->state_list_wq);

	spin_lock_init(&cam_ctrl->info_lock);
	reset_runtime_info(cam_ctrl);

	vsync_collector_init(&cam_ctrl->vsync_col);
	cam_ctrl->cur_cq_ref = 0;

	mtk_cam_watchdog_init(&cam_ctrl->watchdog);
	cam_ctrl->hw_hang_count_down = 0;

	mtk_cam_ctx_queue_done_worker(ctx, &cam_ctrl->done_work);

	dev_info(ctx->cam->dev, "[%s] ctx:%d\n", __func__, ctx->stream_id);
}

static bool ctrl_is_state_list_empty(struct mtk_cam_ctrl *ctrl)
{
	bool empty;

	read_lock(&ctrl->list_lock);
	empty = list_empty(&ctrl->camsys_state_list);
	read_unlock(&ctrl->list_lock);

	return empty;
}

static void mtk_cam_ctrl_wait_list_empty(struct mtk_cam_ctrl *ctrl)
{
	int timeout_ms = 200;
	long ret;

	ret = wait_event_interruptible_timeout(ctrl->state_list_wq,
					ctrl_is_state_list_empty(ctrl),
					msecs_to_jiffies(timeout_ms));
	if (ret == 0)
		pr_info("%s: error: wait for list empty: %dms timeout\n",
			__func__, timeout_ms);
	else if (ret < 0)
		pr_info("%s: error: interrupted by signal\n", __func__);
	else
		return;
}

void disable_adlrd(struct mtk_cam_ctx *ctx)
{
	/* set ADLRD trigger src to local */
	writel(1, ctx->cam->adlrd_base + 0x0888);
	/* set ADLRD enable to 0 */
	writel(0, ctx->cam->adlrd_base + 0x0804);
	/* toggle DB */
	writel(1, ctx->cam->adlrd_base + 0x088C);

	mtk_cam_bwr_clr_bw(ctx->cam->bwr, ENGINE_CAM_MAIN, MDP0_PORT);
}

void mtk_cam_ctrl_stop(struct mtk_cam_ctrl *cam_ctrl)
{
	struct mtk_cam_ctx *ctx = cam_ctrl->ctx;
	struct mtk_cam_job_state *job_s;
	struct mtk_cam_job *job;
	struct list_head job_list;

	mtk_cam_ctrl_wake_up_on_event(cam_ctrl, CAMSYS_EVENT_OFF);
	// if adl flow, await all job done to avoid hw abnormal issue
	if (mtk_cam_ctx_is_adl_flow(ctx)) {
		mtk_cam_ctrl_wait_list_empty(cam_ctrl);
		disable_adlrd(ctx);
	}
	/* should wait stream-on/seamless switch finished before stopping */
	kthread_flush_worker(&ctx->flow_worker);

	/* stop procedure
	 * 1. mark 'stopped' status to skip further processing
	 * 2. stop all working context
	 *   a. disable_irq for threaded_irq
	 *   b. workqueue: cancel_work_sync & (drain/flush)_workqueue
	 *   c. kthread: cancel_work_sync & flush_worker
	 * 3. Now, all contexts are stopped. return resources
	 */
	atomic_set(&cam_ctrl->stopped, 1);
	wake_up_interruptible(&cam_ctrl->done_wq);
	mtk_cam_ctx_slc_stream(ctx, false, 0xFF);
	mtk_cam_ctx_flush_adl_work(ctx);
	mtk_cam_ctx_engine_off(ctx);

	/* disable irq first */
	mtk_cam_ctx_engine_disable_irq(ctx);

	/* flush seesion & wait */
	mtk_cam_ctx_flush_session(ctx);

	/* note: after hw disabled, stop buffer_done worker */
	read_lock(&cam_ctrl->list_lock);
	list_for_each_entry(job_s, &cam_ctrl->camsys_state_list, list) {
		job = container_of(job_s, struct mtk_cam_job, job_state);

		call_jobop(job, cancel);
	}
	read_unlock(&cam_ctrl->list_lock);
	if (ctx->seninf)
		mtk_cam_seninf_set_abort(ctx->seninf);
	mtk_cam_watchdog_stop(&cam_ctrl->watchdog);

	/* this would be time consuming */
	ctx_stream_off_seninf_sensor(ctx);

	mtk_cam_ctrl_wait_all_released(cam_ctrl);

	/* reset hw */
	mtk_cam_ctx_engine_reset(ctx);
	/* clear hw some regs */
	mtk_cam_ctx_engine_clear(ctx);

	/* await done work finished */
	kthread_flush_worker(&ctx->done_worker);
	kthread_flush_worker(&ctx->sensor_worker);
	kthread_flush_worker(&ctx->tuning_worker);

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
int extisp_listen_each_cq_done(
	struct mtk_cam_ctrl *ctrl)
{
	int ret = 0;

	spin_lock(&ctrl->info_lock);
	ret = ctrl->r_info.extisp_enable && ctrl->r_info.outer_seq_no > 0;
	spin_unlock(&ctrl->info_lock);

	return ret;
}
int timeshare_pureraw_pd_cq_done(
	struct mtk_cam_ctrl *ctrl)
{
	return ctrl->r_info.timeshare_enable;
}

int vsync_update_extisp(struct mtk_cam_ctrl *ctrl,
		  int engine_type, int irq_type,
		  int idx, struct vsync_result *res)
{
	struct mtk_cam_ctx *ctx = ctrl->ctx;

	if (!res || !ctx)
		return 1;

	if (engine_type == CAMSYS_ENGINE_RAW)
		res->is_last = 1;
	if (engine_type == CAMSYS_ENGINE_CAMSV) {
		if (irq_type & BIT(CAMSYS_IRQ_FRAME_START))
			res->is_extmeta = 1;
		if ((ctx->num_sv_subdevs &&
			(irq_type & BIT(CAMSYS_IRQ_FRAME_START_DCIF_MAIN))) ||
			((ctrl->r_info.extisp_enable & BIT(EXTISP_DATA_PD)) == 0))
			res->is_first = 1;
	}
	if (engine_type == CAMSYS_ENGINE_MRAW)
		res->is_first = 1;

	return 0;
}

int vsync_update(struct vsync_collector *c,
		  int engine_type, int irq_type,
		  int idx, struct vsync_result *res)
{
	unsigned int coming;

	coming = engine_idx_to_bit(engine_type, idx);

	if (!(coming & c->desired))
		goto SKIP_VSYNC;

	if (irq_type & BIT(CAMSYS_IRQ_FRAME_START))
		c->collected |= (coming & c->desired);

	if (CAM_DEBUG_ENABLED(CTRL))
		pr_info("%s: vsync desired/collected/coming/first %x/%x/%x/%x\n",
			__func__, c->desired,
			c->collected, (coming & c->desired), c->collected_first);

	res->is_first = c->collected_first ? 0 :
			!(c->collected & (c->collected - 1)) ||
			(!c->collected && irq_type & BIT(CAMSYS_IRQ_FRAME_START_DCIF_MAIN));
	res->is_last = c->collected == c->desired;
	res->is_extmeta = 0;
	if (res->is_first)
		c->collected_first = 1;

	return 0;

SKIP_VSYNC:
	res->is_first = 0;
	res->is_extmeta = 0;
	res->is_last = 0;
	res->inner_cookie = -1;
	return 1;
}

static inline u64 mtk_cam_ctrl_latest_sof(struct mtk_cam_ctrl *ctrl)
{
	u64 ts_ns;

	spin_lock(&ctrl->info_lock);
	ts_ns = ctrl->r_info.sof_l_ts_ns;
	spin_unlock(&ctrl->info_lock);

	return ts_ns;
}

struct watchdog_debug_work {
	struct work_struct work;
	struct mtk_cam_watchdog *wd;
	bool seninf_check_timeout;
	const char *desc;
};

static void mtk_cam_ctrl_dump_first_job(struct mtk_cam_ctrl *ctrl,
			int *seq, const char *desc)
{
	struct mtk_cam_job *job;

	job = mtk_cam_ctrl_get_job(ctrl, cond_first_job, 0);
	if (job) {
		int seq_no;

		if (seq)
			seq_no = *seq;
		else
			seq_no = ctrl_fetch_inner(ctrl);

		call_jobop(job, dump, seq_no, desc);
		mtk_cam_job_put(job);
	} else
		pr_info("%s: no job to dump", __func__);
}

static void mtk_dump_debug_for_no_vsync(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_cam_ctrl *ctrl = &ctx->cam_ctrl;
	struct mtk_cam_job *job;

	dump_runtime_info(ctrl);

	job = mtk_cam_ctrl_get_job(ctrl, cond_first_job, 0);
	if (job) {
		mtk_engine_dump_debug_status(cam, job->used_engine, DD_DUMP_NONE);
		mtk_cam_job_put(job);
	} else {
		mtk_engine_dump_debug_status(cam, ctx->used_engine, DD_DUMP_NONE);
	}
}

static void mtk_cam_watchdog_sensor_worker(struct work_struct *work)
{
	struct watchdog_debug_work *dbg_work;
	struct mtk_cam_watchdog *wd;
	struct mtk_cam_ctrl *ctrl;
	struct mtk_cam_ctx *ctx;
	int seq_no, ret = 0;

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

		if (!timeout) {

			/* not timeout yet, reset the counter */
			atomic_dec(&wd->reset_sensor_cnt);
			goto EXIT_WORK;
		}
	}

	seq_no = ctrl_fetch_inner(ctrl);

	/* handle timeout */
	if (mtk_cam_seninf_dump(ctx->seninf, seq_no, true, false)) {
		mtk_cam_event_esd_recovery(ctrl, seq_no);
		pr_info("%s: TODO: add esd event\n", __func__);

	}

	if (atomic_read(&wd->reset_sensor_cnt) < WATCHDOG_MAX_SENSOR_RETRY_CNT)
		goto EXIT_WORK;

	dev_info(ctx->cam->dev, "ctx-%d reset sensor failed%s\n",
		ctx->stream_id, ctx->is_sv_mraw_error ?
		" due to " MSG_SENINF_FRAME_ERROR : "");
	mtk_dump_debug_for_no_vsync(ctx);
	vsync_collector_dump(&ctrl->vsync_col);

	ret = mtk_cam_seninf_dump(ctx->seninf, seq_no, true, true);
	if (!mtk_cam_is_display_ic(ctx)) {
		if (ret != -ESTRPIPE && !ctx->is_sv_mraw_error) {
			mtk_cam_event_error(ctrl, MSG_VSYNC_TIMEOUT);
			WRAP_AEE_EXCEPTION(MSG_VSYNC_TIMEOUT, "watchdog timeout");
		} else {
			/**
			 * NOTE: skip raise aee exception if
			 * camsv/mraw has received error interrupt status
			 */
			ctx->is_sv_mraw_error = false;
			mtk_cam_event_error(ctrl, MSG_SENINF_FRAME_ERROR);
		}
	}

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

	vsync_collector_dump(&ctrl->vsync_col);
	mtk_cam_ctrl_dump_first_job(ctrl, NULL, dbg_work->desc);

EXIT_WORK:
	complete(&wd->work_complete);
FREE_WORK:
	kfree(dbg_work);
}

static int watchdog_schedule_debug_work(struct mtk_cam_watchdog *wd,
					void (*func)(struct work_struct *work),
					const char *desc,
					bool seninf_check_timeout)
{
	struct watchdog_debug_work *work;

	work = kmalloc(sizeof(*work), GFP_ATOMIC);
	if (WARN_ON(!work))
		return -1;

	work->wd = wd;
	INIT_WORK(&work->work, func);
	work->seninf_check_timeout = seninf_check_timeout;
	work->desc = desc;

	schedule_work(&work->work);
	return 0;
}


static int mtk_cam_watchdog_schedule_sensor_reset(struct mtk_cam_watchdog *wd,
						 const char *desc, bool check_timeout)
{
	return watchdog_schedule_debug_work(wd, mtk_cam_watchdog_sensor_worker,
					     desc, check_timeout);
}

int mtk_cam_watchdog_schedule_job_dump(struct mtk_cam_watchdog *wd,
						  const char *desc)
{
	return watchdog_schedule_debug_work(wd, mtk_cam_watchdog_job_worker,
						 desc, 0);
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
	if (reset_cnt == 1)
		vsync_collector_dump(&ctrl->vsync_col);
	if (reset_cnt > WATCHDOG_MAX_SENSOR_RETRY_CNT) {
		complete(&wd->work_complete);
		goto SKIP_SCHEDULE_WORK;
	}

	dev_info(ctx->cam->dev, "schedule work for sensor_reset: ctx-%d\n",
		 ctx->stream_id);
	mtk_cam_watchdog_schedule_sensor_reset(wd, MSG_VSYNC_TIMEOUT, check_timeout);
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
	int i;

	if (!ctx)
		return -1;

	new_sof = mtk_cam_ctrl_latest_sof(ctrl);
	if (new_sof != wd->last_sof_ts) {
		wd->last_sof_ts = new_sof;

		atomic_set(&wd->reset_sensor_cnt, 0);
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		if (ctx->hw_raw[i]) {
			dev_info_ratelimited(ctx->cam->dev,
			"%s: vsync may timeout, ctx:%d(%d/0x%x), last ts = %lld\n",
			__func__, ctx->stream_id, i, ctx->used_engine, wd->last_sof_ts);
		}
	}
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
	int req_seq, is_dc;
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
	is_dc = is_dc_mode(job);
	mtk_cam_job_put(job);

	if (req_seq != wd->req_seq) {
		wd->req_seq = req_seq;
		return 0;
	}

	ts = ktime_get_boottime_ns();
	if (!job_ts || in_valid_hw_processing_time(ts - job_ts)) {
		dev_info(ctx->cam->dev, "[inner check] job #%d job_ts %llu ts %llu, skip\n",
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
	mtk_cam_watchdog_schedule_job_dump(wd,
		is_dc ? MSG_DC_SKIP_FRAME : MSG_DEQUE_ERROR);
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
	u64 ts_timer, ts_wait_monitor;

	if (!atomic_cmpxchg(&wd->started, 1, 0))
		return;

	del_timer_sync(&wd->timer);
	ts_timer = ktime_get_boottime_ns();
	atomic_set(&wd->timer_signaled, 1);
	wake_up_interruptible(&wd->monitor_wq);

	wait_for_completion(&wd->monitor_complete);
	ts_wait_monitor = ktime_get_boottime_ns();
	wait_for_completion(&wd->work_complete);
	pr_info("[%s] %llu/%llu/%llu\n",
		__func__, ts_timer, ts_wait_monitor, ktime_get_boottime_ns());
}

int mtk_cam_ctrl_ae_workaround(struct mtk_cam_device *cam,
			      int engine_type, unsigned int engine_id,
			      unsigned int inner_cookie)
{
	unsigned int ctx_id = ctx_from_fh_cookie(inner_cookie);
	struct mtk_cam_ctx *ctx = &cam->ctxs[ctx_id];
	struct mtk_cam_ctrl *ctrl = &cam->ctxs[ctx_id].cam_ctrl;
	struct mtk_raw_device *raw_dev;
	int i = 0;

	if (ctx->cam->sw_ver == 0x0001)
		return 0;

	if (mtk_cam_ctrl_get(ctrl))
		return 0;
	for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) {
		// TODO: QOF voter
		if (ctx->hw_raw[i] && (ctx->enable_hsf_raw == 0)) {
			raw_dev = dev_get_drvdata(ctx->hw_raw[i]);
			/* raw change case at bad performance */
			/* raw b sof will disable raw c ae_stat */
			if ((engine_id != raw_dev->id) &&
				(raw_dev->is_slave == false))
				dev_info(cam->dev, "%s: engine %d id %d seq 0x%x (id:%d)\n",
					__func__, engine_type, engine_id, inner_cookie,
					raw_dev->id);
			else
				ae_disable(raw_dev);
		}
	}
	mtk_cam_ctrl_put(ctrl);
	return 0;
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
			      int inner_cookie, const char *desc)
{
	unsigned int ctx_id = ctx_from_fh_cookie(inner_cookie);
	struct mtk_cam_ctrl *ctrl = &cam->ctxs[ctx_id].cam_ctrl;
	struct mtk_cam_watchdog *wd = &ctrl->watchdog;
	bool completed;

	dev_info(cam->dev, "%s: engine %d id %d seq 0x%x\n",
		 __func__, engine_type, engine_id, inner_cookie);

	completed = try_wait_for_completion(&wd->work_complete);
	if (!completed)
		goto SKIP_SCHEDULE_WORK;

	if (atomic_cmpxchg(&wd->dump_job, 0, 1)) {
		complete(&wd->work_complete);
		goto SKIP_SCHEDULE_WORK;
	}

	if (mtk_cam_ctrl_get(ctrl)) {
		complete(&wd->work_complete);
		goto SKIP_SCHEDULE_WORK;
	}

	mtk_cam_watchdog_schedule_job_dump(wd, desc);

	mtk_cam_ctrl_put(ctrl);

	return 0;

SKIP_SCHEDULE_WORK:
	dev_info_ratelimited(cam->dev, "%s: skip dump for seq 0x%x\n",
		 __func__, inner_cookie);
	return 0;
}

int mtk_cam_ctrl_notify_hw_hang(struct mtk_cam_device *cam,
				int engine_type, unsigned engine_id,
				int inner_cookie)
{
	unsigned int ctx_id = ctx_from_fh_cookie(inner_cookie);
	struct mtk_cam_ctrl *ctrl = &cam->ctxs[ctx_id].cam_ctrl;
	struct mtk_cam_job *job;

	dev_info(cam->dev, "%s: warn. eng %d-%d seq 0x%x\n",
		 __func__, engine_type, engine_id, inner_cookie);

	job = mtk_cam_ctrl_get_job(ctrl, cond_frame_no_belong, &inner_cookie);
	if (!job)
		return 0;

	if (is_dc_mode(job)) {
		/*
		 * count frames before doing recovery to avoid various hw timing.
		 * 'set 10 to enable recovery'
		 */
		ctrl->hw_hang_count_down = (DISABLE_RECOVER_FLOW) ? 0 : 10;
		job->is_error = 1;
	}
	mtk_cam_job_put(job);

	return 0;
}
