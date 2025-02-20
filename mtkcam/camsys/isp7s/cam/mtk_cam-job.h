/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_JOB_H
#define __MTK_CAM_JOB_H

#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <media/media-request.h>

#include "mtk_cam-pool.h"
#include "mtk_cam-ipi.h"
#include "mtk_camera-v4l2-controls.h"
#include "mtk_cam-engine.h"

struct mtk_cam_job;

/* new state machine */
enum mtk_cam_sensor_state {
	S_SENSOR_NONE,
	S_SENSOR_NOT_SET,
	S_SENSOR_APPLYING,
	S_SENSOR_APPLIED, /* add this if want to confirm i2c transmission is done */
	S_SENSOR_LATCHED,
	NR_S_SENSOR_STATE,
};

enum mtk_cam_isp_state {
	S_ISP_NOT_SET,
	S_ISP_COMPOSING = S_ISP_NOT_SET,
	S_ISP_COMPOSED,
	S_ISP_APPLYING,
	S_ISP_OUTER,
	S_ISP_PROCESSING,
	S_ISP_SENSOR_MISMATCHED,
	S_ISP_DONE,
	S_ISP_DONE_MISMATCHED,
	NR_S_ISP_STATE,
};

static inline bool isp_in_done_state(int state)
{
	return state == S_ISP_DONE ||
		state == S_ISP_DONE_MISMATCHED;
}

enum mtk_cam_job_action {
	ACTION_APPLY_SENSOR = 1,
	ACTION_APPLY_ISP = 2,
	//ACTION_VSYNC_EVENT,
	ACTION_AFO_DONE = 4,
	ACTION_BUFFER_DONE = 8,
	ACTION_TRIGGER = 16, /* trigger m2m start */
};

enum mtk_camsys_event_type {

	CAMSYS_EVENT_IRQ_F_VSYNC, /* 1st vsync */
	CAMSYS_EVENT_IRQ_L_SOF, /* last sof */
	CAMSYS_EVENT_IRQ_L_CQ_DONE, /* last cq done */
	CAMSYS_EVENT_IRQ_FRAME_DONE,

	CAMSYS_EVENT_TIMER_SENSOR,

	CAMSYS_EVENT_ENQUE,
	CAMSYS_EVENT_ACK,
};
const char *str_event(int event);

struct mtk_cam_ctrl_runtime_info {

	int ack_seq_no;
	int outer_seq_no;
	int inner_seq_no;

	u64 sof_ts_ns;
	u64 sof_hw_ts;
};

struct sensor_apply_params {
	u64 i2c_thres_ns; /* valid period from vsync */
};

struct transition_param {
	struct list_head *head;
	struct mtk_cam_ctrl_runtime_info *info;
	int event;
	u64 event_ts;
	struct sensor_apply_params *s_params;
};

struct mtk_cam_job_state;
struct mtk_cam_job_state_cb;
struct mtk_cam_job_state_ops {
	int (*send_event)(struct mtk_cam_job_state *s,
			  struct transition_param *p);

	int (*is_next_sensor_applicable)(struct mtk_cam_job_state *s);
	int (*is_next_isp_applicable)(struct mtk_cam_job_state *s);

	/* for sensor-mismatched case */
	int (*is_sensor_applied)(struct mtk_cam_job_state *s);
};

enum state_type {
	SENSOR_1ST_STATE,
	SENSOR_STATE		= SENSOR_1ST_STATE,

	ISP_1ST_STATE,
	ISP_STATE		= ISP_1ST_STATE,

	SENSOR_2ND_STATE,
	ISP_2ND_STATE,

	NR_STATE_TYPE,
};

/* callback to job */
struct mtk_cam_job_state_cb {
	void (*on_transit)(struct mtk_cam_job_state *s, int state_type,
			   int old_state, int new_state, int act,
			   struct mtk_cam_ctrl_runtime_info *info);
};

struct mtk_cam_job_state {
	struct list_head list;

	const struct mtk_cam_job_state_ops *ops;

	int seq_no;

	atomic_t state[NR_STATE_TYPE];
	atomic_t todo_action;

	const struct mtk_cam_job_state_cb *cb;

	/*
	 * apply sensor/isp by state machine
	 * if this is disabled, statemachine will skip transitions with hw
	 * action
	 */
	bool apply_by_fsm;
};

#define ops_call(s, func, ...) \
({\
	typeof(s) _s = (s); \
	_s->ops->func(_s, ##__VA_ARGS__); \
})

/* TODO(AY): try to remove from this header */
static inline
int mtk_cam_job_state_set(struct mtk_cam_job_state *s,
			  int state_type, int new_state)
{
	return atomic_xchg(&s->state[state_type], new_state);
}

enum EXP_CHANGE_TYPE {
	EXPOSURE_CHANGE_NONE = 0,
	EXPOSURE_CHANGE_3_to_2,
	EXPOSURE_CHANGE_3_to_1,
	EXPOSURE_CHANGE_2_to_3,
	EXPOSURE_CHANGE_2_to_1,
	EXPOSURE_CHANGE_1_to_3,
	EXPOSURE_CHANGE_1_to_2,
	MSTREAM_EXPOSURE_CHANGE = (1 << 4), // TODO(Will): remove this
};
struct mtk_cam_job_event_info {
	int engine;
	int ctx_id;
	u64 ts_ns;
	int frame_idx;
	int frame_idx_inner;
	int write_cnt;
	int fbc_cnt;
	int isp_request_seq_no;
	int reset_seq_no;
	int isp_deq_seq_no; /* swd + sof interrupt case */
	int isp_enq_seq_no; /* smvr */
};
struct mtk_cam_request;
struct mtk_cam_ctx;

struct mtk_cam_job_ops {
	/* job control */
	void (*cancel)(struct mtk_cam_job *job);
	void (*dump)(struct mtk_cam_job *job, int seq_no);

	/* should alway be called for clean-up resources */
	void (*finalize)(struct mtk_cam_job *job);

	void (*compose_done)(struct mtk_cam_job *job,
			     struct mtkcam_ipi_frame_ack_result *cq_ret,
			     int compose_ret);
	/* action */
	int (*compose)(struct mtk_cam_job *job);
	int (*stream_on)(struct mtk_cam_job *job, bool on);
	int (*reset)(struct mtk_cam_job *job);
	int (*apply_sensor)(struct mtk_cam_job *s);
	int (*apply_isp)(struct mtk_cam_job *s);
	int (*trigger_isp)(struct mtk_cam_job *s); /* m2m use */

	int (*mark_afo_done)(struct mtk_cam_job *s, int seq_no);
	int (*mark_engine_done)(struct mtk_cam_job *s,
				int engine_type, int engine_id,
				int seq_no);
	int (*handle_buffer_done)(struct mtk_cam_job *s);
	int (*apply_switch)(struct mtk_cam_job *s);
	int (*dump_aa_info)(struct mtk_cam_job *s, int engine_type);
};
struct mtk_cam_job {
	/* note: to manage life-cycle in state list */
	atomic_t refs;
	struct list_head list;

	struct mtk_cam_request *req;

	/* Note:
	 * it's dangerous to fetch info from src_ctx
	 * src_ctx is just kept to access worker/workqueue.
	 */
	struct mtk_cam_ctx *src_ctx;

	struct mtk_cam_pool_buffer cq;
	struct mtk_cam_pool_buffer ipi;
	struct mtk_cam_pool_buffer img_work_buf;
	struct mtkcam_ipi_frame_ack_result cq_rst;
	unsigned int used_engine;
	bool do_ipi_config;
	struct mtkcam_ipi_config_param ipi_config;
	bool stream_on_seninf;
	bool seamless_switch;
	bool first_frm_switch;
	int switch_type;

	struct completion compose_completion;
	struct completion cq_exe_completion;

	struct mtk_cam_job_state job_state;
	const struct mtk_cam_job_ops *ops;

	/* for cq_done handling */
	struct apply_cq_ref cq_ref;

	struct kthread_work sensor_work;
	struct work_struct frame_done_work;
	wait_queue_head_t done_wq;
	bool done_work_queued;
	bool cancel_done_work;
	atomic_long_t afo_done; /* bit 0: not handled, bit 1: handled */
	atomic_long_t done_set;
	unsigned long done_handled;

	/* aa debug info */
	struct work_struct aa_dump_work;

	int job_type;	/* job type - only job layer */
	int ctx_id;

	int req_seq; /* increase with request cnt */
	int frame_seq_no; /* increase with isp frame */
	int frame_cnt;

	bool composed;
	/* TODO: if margin is set by ctrl */
	//int sensor_set_margin;	/* allow apply sensor before SOF + x (ms)*/
	u64 timestamp;
	u64 timestamp_mono;

	/* for complete only: not null if current request has sensor ctrl */
	struct media_request_object *sensor_hdl_obj;
	struct v4l2_subdev *sensor;

	bool is_sv_pure_raw;
	bool update_sen_mstream_mode;

	struct mtk_cam_scen job_scen;		/* job 's scen by res control */
	struct mtk_cam_scen prev_scen;
	unsigned int sub_ratio;
	int scq_period;
	u64 (*timestamp_buf)[128];
};

static inline struct mtk_cam_job *mtk_cam_job_get(struct mtk_cam_job *job)
{
	if (atomic_add_unless(&job->refs, 1, 0) == 0)
		return NULL;

	return job;
}

void _on_job_last_ref(struct mtk_cam_job *job);
static inline void mtk_cam_job_put(struct mtk_cam_job *job)
{
	if (atomic_dec_and_test(&job->refs))
		_on_job_last_ref(job);
}

void mtk_cam_ctx_job_finish(struct mtk_cam_job *job);
bool mtk_cam_job_has_pending_action(struct mtk_cam_job *job);
int mtk_cam_job_apply_pending_action(struct mtk_cam_job *job);

#define call_jobop(job, func, ...) \
({\
	typeof(job) _job = (job);\
	typeof(_job->ops) _ops = _job->ops;\
	_ops && _ops->func ? _ops->func(_job, ##__VA_ARGS__) : -EINVAL;\
})

#define call_jobop_opt(job, func, ...)\
({\
	typeof(job) _job = (job);\
	_job->ops.func ? _job->ops.func(_job, ##__VA_ARGS__) : 0;\
})

enum MTK_CAMSYS_JOB_TYPE {
	JOB_TYPE_BASIC,
	JOB_TYPE_M2M,
	JOB_TYPE_MSTREAM,
	JOB_TYPE_STAGGER,
	JOB_TYPE_HW_PREISP,
	JOB_TYPE_HW_SUBSAMPLE,
	JOB_TYPE_ONLY_SV = 0x100,

	/* TODO(AY): remove following if we don't need */
	//RAW_JOB_DC,
	//RAW_JOB_DC_MSTREAM,
	//RAW_JOB_DC_STAGGER,
	//RAW_JOB_OFFLINE_STAGGER,
	//RAW_JOB_OTF_RGBW,
	//RAW_JOB_DC_RGBW,
	//RAW_JOB_OFFLINE_RGBW,
	//RAW_JOB_HW_TIMESHARED,
};

/* TODO(AY): remove following xxx_job struct def. */
struct mtk_cam_mstream_job {
	struct mtk_cam_job job; /* always on top */

	struct mtk_cam_pool_buffer cq;
	struct mtk_cam_pool_buffer ipi;

	struct mtkcam_ipi_frame_ack_result cq_rst;
	u8 composed_idx;
	u8 apply_sensor_idx;
	u8 apply_isp_idx;

	bool composed_1st;

#ifdef DOES_MSTREAM_NEED_THESE
	/* TODO */
	wait_queue_head_t expnum_change_wq;
	atomic_t expnum_change;
	struct mtk_cam_scen prev_scen;
	int switch_type;
#endif
};

struct mtk_cam_timeshare_job {
	struct mtk_cam_job job; /* always on top */

	/* TODO */
};

struct mtk_cam_pool_job {
	struct mtk_cam_pool_priv priv;
	struct mtk_cam_job_data *job_data;
};

/* this struct is for job-pool */
struct mtk_cam_job_data {
	struct mtk_cam_pool_job pool_job;

	union {
		struct mtk_cam_job job;
		struct mtk_cam_mstream_job m;
		struct mtk_cam_timeshare_job t;
	};
};

#define BITS_FRAME_SEQ		24
#define _MASK_FRAME_SEQ		(BIT(BITS_FRAME_SEQ) - 1)
/*
 * frame header cookie = [31:24] ctx_id + [23:0] seq_no
 */
static inline unsigned int ctx_from_fh_cookie(unsigned int fh_cookie)
{
	return fh_cookie >> BITS_FRAME_SEQ;
}

static inline unsigned int seq_from_fh_cookie(unsigned int fh_cookie)
{
	return fh_cookie & _MASK_FRAME_SEQ;
}

static inline unsigned int to_fh_cookie(unsigned int ctx, unsigned int seq_no)
{
	return ctx << BITS_FRAME_SEQ | seq_no;
}

static inline unsigned int add_frame_seq(unsigned int seq, int cnt)
{
	return (seq + cnt) & _MASK_FRAME_SEQ;
}

static inline unsigned int next_frame_seq(unsigned int seq)
{
	return add_frame_seq(seq, 1);
}

static inline unsigned int prev_frame_seq(unsigned int seq)
{
	return add_frame_seq(seq, -1);
}

/* delta from a to b */
static inline unsigned int frame_seq_diff(unsigned int b, unsigned int a)
{
	return  (b - a) & _MASK_FRAME_SEQ;
}


static inline struct mtk_cam_job_data *job_to_data(struct mtk_cam_job *job)
{
	return container_of(job, struct mtk_cam_job_data, job);
}

static inline struct mtk_cam_job *data_to_job(struct mtk_cam_job_data *data)
{
	return &data->job;
}

static inline void mtk_cam_job_return(struct mtk_cam_job *job)
{
	struct mtk_cam_job_data *data = job_to_data(job);

	mtk_cam_pool_return(&data->pool_job, sizeof(data->pool_job));
}

int mtk_cam_job_pack(struct mtk_cam_job *job, struct mtk_cam_ctx *ctx,
		     struct mtk_cam_request *req);

static inline void mtk_cam_job_set_no(struct mtk_cam_job *job,
				      int req_no, int seq_no)
{
	job->req_seq = req_no;
	job->frame_seq_no = seq_no;
	job->job_state.seq_no = seq_no;
}

static inline void mtk_cam_job_set_fsm(struct mtk_cam_job *job, bool enable)
{
	job->job_state.apply_by_fsm = enable;
}

struct mtk_cam_dump_param;
int mtk_cam_job_fill_dump_param(struct mtk_cam_job *job,
				struct mtk_cam_dump_param *p,
				const char *desc);

#endif //__MTK_CAM_JOB_H
