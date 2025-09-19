/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_CAM_CTRL_H
#define __MTK_CAM_CTRL_H

#include <linux/hrtimer.h>
#include <linux/timer.h>
#include <linux/completion.h>

#include "mtk_cam-job.h"

struct mtk_cam_device;
struct mtk_raw_device;
struct mtk_camsv_device;

unsigned long engine_idx_to_bit(int engine_type, int idx);

struct vsync_result {
	unsigned char is_first : 1;
	unsigned char is_last  : 1;
};

struct vsync_collector {
	unsigned int desired;
	unsigned int collected;
};

static inline void vsync_reset(struct vsync_collector *c)
{
	c->desired = c->collected = 0;
}

//TODO(Will): desired with _get_master_engines
static inline void vsync_set_desired(struct vsync_collector *c,
				     unsigned int desried)
{
	c->desired = desried;
	c->collected = 0;
}

int vsync_update(struct vsync_collector *c,
		  int engine_type, int idx,
		  struct vsync_result *res);

struct mtk_cam_watchdog {

	atomic_t started;
	bool monitor_vsync;

	struct timer_list timer;
	atomic_t timer_signaled;
	wait_queue_head_t monitor_wq;

	u64 last_sof_ts;
	int req_seq;

	struct completion monitor_complete;
	struct completion work_complete;
	atomic_t reset_sensor_cnt;
	atomic_t dump_job;
};

void mtk_cam_watchdog_init(struct mtk_cam_watchdog *wd);
int mtk_cam_watchdog_start(struct mtk_cam_watchdog *wd, bool monitor_vsync);
void mtk_cam_watchdog_stop(struct mtk_cam_watchdog *wd);

/* system wq ctrl */
struct mtk_cam_sys_wq_ctrl;
struct mtk_cam_sys_wq_work {
	struct work_struct work;
	void (*exec)(struct work_struct *work);
	struct mtk_cam_job *job;

	struct mtk_cam_sys_wq_ctrl *wq_ctrl;
};

struct mtk_cam_sys_wq_ctrl {
	struct workqueue_struct *wq;
	atomic_t stopped;
	atomic_t running;
	struct completion work_done;

	struct mtk_cam_ctx *ctx;
};

void mtk_cam_wq_ctrl_init(struct mtk_cam_sys_wq_ctrl *wq_ctrl,
		struct workqueue_struct *wq,
		struct mtk_cam_ctx *ctx);
int mtk_cam_wq_ctrl_queue_work(struct mtk_cam_sys_wq_ctrl *wq_ctrl,
		void (*exec)(struct work_struct *work),
		struct mtk_cam_job *job);
void mtk_cam_wq_ctrl_wait_finish(struct mtk_cam_sys_wq_ctrl *wq_ctrl);

static inline struct mtk_cam_ctx *mtk_cam_wq_work_get_ctx(
		struct mtk_cam_sys_wq_work *wq_work)
{
	return wq_work->wq_ctrl->ctx;
}


/*per stream (sensor) */
struct mtk_cam_ctrl {
	struct mtk_cam_ctx *ctx;

	struct sensor_apply_params s_params;

	atomic_t enqueued_req_cnt;
	unsigned int enqueued_frame_seq_no;	/* enque job counter - ctrl maintain */
	unsigned int frame_sync_event_cnt;
	int fs_event_subframe_cnt;
	int fs_event_subframe_idx;

	atomic_t stopped;
	atomic_t ref_cnt;

	atomic_t stream_on_done;

	/* use for awaiting next vsync. e.g., seamless switch */
	atomic_t await_switching_seq;
	struct completion vsync_complete;
	/* note:
	 *   this send_lock is only used in send_event func to guarantee that send_event
	 *   is executed in an exclusive manner.
	 */
	spinlock_t send_lock;
	rwlock_t list_lock;
	struct list_head camsys_state_list;

	spinlock_t info_lock;
	struct mtk_cam_ctrl_runtime_info r_info;
	wait_queue_head_t stop_wq;

	struct vsync_collector vsync_col;
	struct apply_cq_ref *cur_cq_ref;

	struct mtk_cam_watchdog watchdog;

	struct mtk_cam_sys_wq_ctrl highpri_wq_ctrl;
};

/* engine's callback functions */
int mtk_cam_ctrl_isr_event(struct mtk_cam_device *cam,
			   int engine_type, unsigned int engine_id,
			   struct mtk_camsys_irq_info *irq_info);
int mtk_cam_ctrl_reset_sensor(struct mtk_cam_device *cam,
			      int engine_type, unsigned int engine_id,
			      int inner_cookie);
int mtk_cam_ctrl_dump_request(struct mtk_cam_device *cam,
			      int engine_type, unsigned int engine_id,
			      int inner_cookie);

/* ctx_stream_on */
void mtk_cam_ctrl_start(struct mtk_cam_ctrl *cam_ctrl,
	struct mtk_cam_ctx *ctx);
/* ctx_stream_off */
void mtk_cam_ctrl_stop(struct mtk_cam_ctrl *cam_ctrl);
/* enque job */
void mtk_cam_ctrl_job_enque(struct mtk_cam_ctrl *cam_ctrl,
	struct mtk_cam_job *job);
/* inform job composed */
void mtk_cam_ctrl_job_composed(struct mtk_cam_ctrl *cam_ctrl,
			       unsigned int fh_cookie,
			       struct mtkcam_ipi_frame_ack_result *cq_ret,
			       int ack_ret);

void mtk_cam_event_frame_sync(struct mtk_cam_ctrl *cam_ctrl,
			      unsigned int frame_seq_no);
void mtk_cam_event_error(struct mtk_cam_ctrl *cam_ctrl, const char *msg);
void mtk_cam_event_request_dumped(struct mtk_cam_ctrl *cam_ctrl,
				  unsigned int frame_seq_no);

#endif
