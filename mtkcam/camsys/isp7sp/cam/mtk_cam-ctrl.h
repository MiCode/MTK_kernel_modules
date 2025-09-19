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
	unsigned char is_extmeta  : 1;

	int inner_cookie;
};

#define VSYNC_HIST_NUM	10
struct vsync_collector {
	unsigned int desired;
	unsigned int collected;
	unsigned int collected_first;

	/* vsync history */
	spinlock_t history_lock;
	unsigned int cur_history_idx;
	struct vsync_history {
		int engine;
		int id;
		u64 ts_ns;
	} history[VSYNC_HIST_NUM];
};

static inline void vsync_collector_init(struct vsync_collector *c)
{
	c->desired = c->collected = c->collected_first = 0;
	spin_lock_init(&c->history_lock);
	c->cur_history_idx = 0;
	memset(c->history, 0, sizeof(c->history));
}

void vsync_collector_dump(struct vsync_collector *c);

static inline void vsync_set_desired(struct vsync_collector *c,
				     unsigned int desried)
{
	c->desired = desried;
	c->collected = 0;
	c->collected_first = 0;
}

static inline void vsync_clear_collected(struct vsync_collector *c)
{
	c->collected = c->collected_first = 0;
}

static inline void vsync_rewait(struct vsync_collector *c,
				     unsigned int rewait)
{
	c->collected &= ~rewait;
	c->collected_first = 0;
}

int vsync_update(struct vsync_collector *c,
		  int engine_type, int irq_type, int idx,
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


/*per stream (sensor) */
struct mtk_cam_ctrl {
	struct mtk_cam_ctx *ctx;

	u64 frame_interval_ns;

	bool initial_req;
	unsigned int enqueued_req_cnt;
	unsigned int enqueued_frame_seq_no;	/* enque job counter - ctrl maintain */
	unsigned int frame_sync_event_cnt;
	int fs_event_subframe_cnt;
	int fs_event_subframe_idx;

	atomic_t stopped;
	atomic_t ref_cnt;

	atomic_t stream_on_cnt;

	wait_queue_head_t event_wq;

	struct kthread_work done_work;
	wait_queue_head_t done_wq;

	/* note:
	 *   this send_lock is only used in send_event func to guarantee that send_event
	 *   is executed in an exclusive manner.
	 */
	spinlock_t send_lock;
	rwlock_t list_lock;
	struct list_head camsys_state_list;
	wait_queue_head_t state_list_wq;

	spinlock_t info_lock;
	struct mtk_cam_ctrl_runtime_info r_info;
	wait_queue_head_t stop_wq;
	struct vsync_collector vsync_col;
	struct apply_cq_ref *cur_cq_ref;

	struct mtk_cam_watchdog watchdog;
	unsigned int hw_hang_count_down;
	unsigned int sensor_seq;
	unsigned int frame_sync_id;
	unsigned int sensor_sync_id;
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
			      int inner_cookie, const char *desc);
int mtk_cam_ctrl_notify_hw_hang(struct mtk_cam_device *cam,
				int engine_type, unsigned engine_id,
				int inner_cookie);

/* ctx_stream_on */
void mtk_cam_ctrl_start(struct mtk_cam_ctrl *cam_ctrl,
	struct mtk_cam_ctx *ctx);
/* ctx_stream_off */
void mtk_cam_ctrl_stop(struct mtk_cam_ctrl *cam_ctrl);
/* enque job */
void mtk_cam_ctrl_job_enque(struct mtk_cam_ctrl *cam_ctrl,
	struct mtk_cam_job *job);
void mtk_cam_ctrl_sensor_job_enque(struct mtk_cam_ctrl *cam_ctrl,
	struct mtk_cam_job *job);
void mtk_cam_ctrl_isp_job_enque(struct mtk_cam_ctrl *cam_ctrl,
	struct mtk_cam_job *job);


/* inform job composed */
void mtk_cam_ctrl_job_composed(struct mtk_cam_ctrl *cam_ctrl,
			       unsigned int fh_cookie,
			       struct mtkcam_ipi_frame_ack_result *cq_ret,
			       int ack_ret);

void mtk_cam_ctrl_handle_done_loop(struct mtk_cam_ctrl *cam_ctrl);
struct mtk_cam_job *mtk_cam_ctrl_get_job_by_req_id(
				struct mtk_cam_ctrl *cam_ctrl,
				unsigned int req_info_id);

void mtk_cam_event_frame_sync(struct mtk_cam_ctrl *cam_ctrl,
			      unsigned int frame_seq_no);
void mtk_cam_event_error(struct mtk_cam_ctrl *cam_ctrl, const char *msg);
void mtk_cam_event_request_dumped(struct mtk_cam_ctrl *cam_ctrl,
				  unsigned int frame_seq_no);
/* extisp specifically used */
void mtk_cam_event_sensor_trigger(struct mtk_cam_ctrl *cam_ctrl,
			      unsigned int frame_seq_no);
int extisp_listen_each_cq_done(struct mtk_cam_ctrl *ctrl);
int vsync_update_extisp(struct mtk_cam_ctrl *ctrl,
		  int engine_type, int irq_type, int idx,
		  struct vsync_result *res);

#endif
