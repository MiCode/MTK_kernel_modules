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
#include "mtk_camera-v4l2-controls-7sp.h"
#include "mtk_cam-engine.h"
#include "mtk_cam-dvfs_qos.h"

#define JOB_NUM_PER_STREAM 8
#define JOB_NUM_PER_STREAM_DISPLAY_IC 16
#define MAX_JOB_NUM_PER_STREAM \
			(JOB_NUM_PER_STREAM > JOB_NUM_PER_STREAM_DISPLAY_IC) ? \
			JOB_NUM_PER_STREAM : JOB_NUM_PER_STREAM_DISPLAY_IC
#define MAX_PIPES_PER_STREAM 5
#define MAX_RAW_PER_STREAM 3 // twin, 3raw
#define MAX_SV_PIPES_PER_STREAM (MAX_PIPES_PER_STREAM - 1)
#define MAX_MRAW_PIPES_PER_STREAM (MAX_PIPES_PER_STREAM - 1)

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
	S_ISP_COMPOSING,
	S_ISP_COMPOSED,
	S_ISP_APPLYING,
	S_ISP_OUTER,
	S_ISP_APPLYING_PROCRAW,
	S_ISP_OUTER_PROCRAW,
	S_ISP_PROCESSING,
	S_ISP_PROCESSING_PROCRAW, /* extisp used */
	S_ISP_SENSOR_MISMATCHED,
	S_ISP_DONE,
	S_ISP_DONE_MISMATCHED,
	S_ISP_ABORTED,
	NR_S_ISP_STATE,
};

enum mtkcam_buf_fmt_type {
	MTKCAM_BUF_FMT_TYPE_START = 0,
	MTKCAM_BUF_FMT_TYPE_BAYER = MTKCAM_BUF_FMT_TYPE_START,
	MTKCAM_BUF_FMT_TYPE_UFBC,
	MTKCAM_BUF_FMT_TYPE_CNT,
};

struct mtk_cam_buf_fmt_desc {
	int ipi_fmt;
	int pixel_fmt;
	int width;
	int height;
	int stride[3];
	size_t size;
};

struct mtk_cam_driver_buf_desc {
	int fmt_sel;
	struct mtk_cam_buf_fmt_desc fmt_desc[MTKCAM_BUF_FMT_TYPE_CNT];

	size_t max_size; //largest size among all fmt type

	/* for userspace only */
	dma_addr_t daddr;
	int fd;
	/* for buf pool release */
	bool has_pool;
};

struct mtk_camsv_tag_info {
	struct mtk_camsv_pipeline *sv_pipe;
	unsigned int seninf_padidx;
	unsigned int hw_scen;
	unsigned int tag_order;
	unsigned int pixel_mode;
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
	ACTION_COMPOSE_CQ = 8,
	ACTION_TRIGGER = 16, /* trigger m2m start */
	ACTION_CHECK_PROCESSING = 32,
	ACTION_APPLY_ISP_EXTMETA_PD_EXTISP = 64, /* extisp used for apply cq for extmeta data */
	ACTION_APPLY_ISP_PROCRAW_EXTISP = 128, /* extisp used for apply cq for proc raw data */
	ACTION_BUFFER_EXTMETA_PD_DONE = 256,
	ACTION_CQ_DONE = 512,

	ACTION_ABORT_SW_RECOVERY = 1024, /* abort & sw recovery for hw hang */
};

enum mtk_camsys_event_type {

	CAMSYS_EVENT_IRQ_F_VSYNC, /* 1st vsync */
	CAMSYS_EVENT_IRQ_L_SOF, /* last sof */
	CAMSYS_EVENT_IRQ_L_CQ_DONE, /* last cq done */
	CAMSYS_EVENT_IRQ_FRAME_DONE,

	CAMSYS_EVENT_TIMER_SENSOR,

	CAMSYS_EVENT_ENQUE,
	CAMSYS_EVENT_ACK,
	CAMSYS_EVENT_IRQ_EXTMETA_SOF, /* extisp meta's vsync */
	CAMSYS_EVENT_IRQ_EXTMETA_CQ_DONE, /* extisp meta's cq done */
	CAMSYS_EVENT_IRQ_EXTMETA_FRAME_DONE, /* extisp meta's frame done */

	CAMSYS_EVENT_HW_HANG, /* hw unrecoverable error */
};
const char *str_event(int event);
enum extisp_data {
	EXTISP_DATA_PD,
	EXTISP_DATA_META,
	EXTISP_DATA_PROCRAW,
	NR_EXTISP_DATA,
};

struct mtk_cam_ctrl_runtime_info {

	int ack_seq_no;
	int outer_seq_no;
	int inner_seq_no;
	int done_seq_no;

	u64 sof_ts_ns;
	u64 sof_ts_mono_ns;
	u64 sof_l_ts_ns;
	u64 sof_l_ts_mono_ns;
	int extisp_enable; /* extisp used */
	int extisp_tg_cnt[NR_EXTISP_DATA]; /* extisp used */
};

enum mtk_cam_sensor_latch {
	SENSOR_LATCHED_F_SOF,
	SENSOR_LATCHED_L_SOF,
};

struct sensor_apply_params {
	u64 i2c_thres_ns; /* valid period from vsync */
	int latched_timing;
	bool always_allow;
};

struct transition_param {
	struct list_head *head;
	struct mtk_cam_ctrl_runtime_info *info;
	int event;
	u64 event_ts;
	struct sensor_apply_params *s_params;
	u64 cq_trigger_thres;
	spinlock_t *info_lock; /* only for info lock */
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
	bool compose_by_fsm;

	/* for different sensor latched timing */
	struct sensor_apply_params s_params;
	u64 cq_trigger_thres_ns; /* cq valid period from vsync */
	struct state_table *sensor_tbl;
	/* for extisp */
	int tg_cnt;
	u64 extisp_data_timestamp[NR_EXTISP_DATA]; /* extisp used */
};

#define ops_call(s, func, ...) \
({\
	typeof(s) _s = (s); \
	_s->ops->func(_s, ##__VA_ARGS__); \
})

// raw imgo:1 + camsv tag:4 + yuvo_r1/r3:2
#define UBFC_HEADER_PARAM_MAX		7

struct mtk_cam_ufbc_header_entry {
	int ipi_id;
	void *vaddr;
	struct mtkcam_ipi_img_ufo_param *param;
};

struct mtk_cam_ufbc_header {
	int used;
	struct mtk_cam_ufbc_header_entry entry[UBFC_HEADER_PARAM_MAX];
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
struct mmqos_bw {
	u32 peak_bw;
	u32 avg_bw;
};

struct mtk_cam_seamless_ops {
	int (*before_sensor)(struct mtk_cam_job *s);
	int (*after_sensor)(struct mtk_cam_job *s);
	int (*after_prev_frame_done)(struct mtk_cam_job *s);
};

struct mtk_cam_job_ops {
	/* job control */
	void (*cancel)(struct mtk_cam_job *job);
	void (*dump)(struct mtk_cam_job *job, int seq_no, const char *desc);

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
	int (*dump_aa_info)(struct mtk_cam_job *s);
	int (*apply_extisp_meta_pd)(struct mtk_cam_job *s); /* extisp use */
	int (*apply_extisp_procraw)(struct mtk_cam_job *s); /* extisp use */

	int (*sw_recovery)(struct mtk_cam_job *s);
	struct mtk_cam_seamless_ops *seamless_ops;
};

struct initialize_params {
	int (*master_raw_init)(struct device *dev, struct mtk_cam_job *job);
};

struct mtk_cam_job {
	/* note: to manage life-cycle in state list */
	atomic_t refs;
	struct list_head list;

	struct mtk_cam_request *req;
	struct mtk_cam_request *req_sensor;
	unsigned int req_info_id;
	/* Note:
	 * it's dangerous to fetch info from src_ctx
	 * src_ctx is just kept to access worker/workqueue.
	 */
	struct mtk_cam_ctx *src_ctx;
	u32 enable_hsf_raw;

	struct mtk_cam_pool_buffer cq;
	struct mtk_cam_pool_buffer ipi;
	//struct mtk_cam_pool_buffer img_work_buf;

	/* for raw switch */
	struct mtk_cam_pool_wrapper *img_wbuf_pool_wrapper;
	struct mtk_cam_pool_wrapper *img_wbuf_pool_wrapper_prev;
	struct mtk_cam_device_refcnt_buf *w_caci_buf;

	struct mtkcam_ipi_frame_ack_result cq_rst;
	unsigned int used_engine;
	unsigned int master_engine;

	bool do_ipi_config;
	struct mtkcam_ipi_config_param ipi_config;
	bool stream_on_seninf;
	bool seamless_switch;
	bool first_frm_switch;
	bool first_job;

	bool raw_switch;

	struct completion compose_completion;
	struct completion cq_exe_completion;

	struct mtk_cam_job_state job_state;
	const struct mtk_cam_job_ops *ops;
	const struct initialize_params *init_params;

	/* for cq_done handling */
	struct apply_cq_ref cq_ref;

	struct kthread_work sensor_work;
	atomic_long_t afo_done; /* bit 0: not handled, bit 1: handled */
	atomic_long_t done_set;
	unsigned long done_handled;
	unsigned int done_pipe;

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
	struct mtk_cam_hdr_timestamp_info hdr_ts_cache;

	/* for complete only: not null if current request has sensor ctrl */
	struct media_request_object *sensor_hdl_obj;
	struct v4l2_subdev *sensor;
	struct v4l2_subdev *seninf; /* for raw switch */
	struct v4l2_subdev *seninf_prev; /* TODO: (Fred) check if we can remove it */

	bool is_sv_pure_raw;

	struct mtk_cam_scen job_scen;		/* job 's scen by res control */
	struct mtk_cam_scen prev_scen;
	char scen_str[40];

	unsigned int sub_ratio;
	int scq_period;
	u64 (*timestamp_buf)[128];
	int extisp_data; /* extisp used */
	struct mmqos_bw raw_mmqos[SMI_PORT_RAW_NUM];
	struct mmqos_bw raw_w_mmqos[SMI_PORT_RAW_NUM];
	struct mmqos_bw yuv_mmqos[SMI_PORT_YUV_NUM];
	struct mmqos_bw sv_mmqos[SMI_PORT_SV_NUM];
	struct mmqos_bw mraw_mmqos[MAX_MRAW_PIPES_PER_STREAM][SMI_PORT_MRAW_NUM];

	/* sensor meta dump */
	bool is_sensor_meta_dump;
	struct mtk_cam_driver_buf_desc seninf_meta_buf_desc;
	struct mtk_cam_pool_buffer sensor_meta_buf;

	/* sv tag control */
	unsigned int used_tag_cnt;
	unsigned int enabled_tags;
	struct mtk_camsv_tag_info tag_info[CAMSV_MAX_TAGS];

	struct mtk_cam_ufbc_header ufbc_header;

	/* error status */
	bool is_error;

	/* debug only: use local_clock() to be consitent with printk */
	u64 local_enqueue_ts;
	u64 local_apply_sensor_ts;
	u64 local_enqueue_isp_ts;
	u64 local_compose_isp_ts;
	u64 local_ack_isp_ts;
	u64 local_trigger_cq_ts;
	u64 local_ispdone_ts;
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

/* note: should beware of data-racing when use this function */
static inline bool mtk_cam_job_is_done(struct mtk_cam_job *job)
{
	return (unsigned long)job->master_engine == job->done_handled;
}

#define call_jobop(job, func, ...) \
({\
	typeof(job) _job = (job);\
	typeof(_job->ops) _ops = _job->ops;\
	_ops && _ops->func ? _ops->func(_job, ##__VA_ARGS__) : -EINVAL;\
})

#define call_job_seamless_ops(job, func, ...) \
({\
	typeof(job) _job = (job);\
	typeof(_job->ops) _ops = _job->ops;\
	_ops && _ops->seamless_ops && _ops->seamless_ops->func ?\
		_ops->seamless_ops->func(_job, ##__VA_ARGS__) : 0;\
})

#define call_jobop_opt(job, func, ...)\
({\
	typeof(job) _job = (job);\
	typeof(_job->ops) _ops = _job->ops;\
	_ops && _job->ops->func ? _ops->func(_job, ##__VA_ARGS__) : 0;\
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

/* return a >= b */
static inline unsigned int frame_seq_ge(unsigned int a, unsigned int b)
{
	/* e.g.,
	 * a = 1, b = 0 => diff = 1 => true
	 * a = 0, b = 1 => diff = -1 => false
	 */
	return frame_seq_diff(a, b) < (_MASK_FRAME_SEQ / 2);
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
int job_pack(struct mtk_cam_job *job);

int mtk_cam_job_pack(struct mtk_cam_job *job, struct mtk_cam_ctx *ctx,
		     struct mtk_cam_request *req);
int mtk_cam_sensor_job_pack(struct mtk_cam_job *job, struct mtk_cam_ctx *ctx,
		     struct mtk_cam_request *req);
int mtk_cam_isp_job_pack(struct mtk_cam_job *job, struct mtk_cam_ctx *ctx,
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
static inline void mtk_cam_job_set_fsm_compose(struct mtk_cam_job *job, bool enable)
{
	job->job_state.compose_by_fsm = enable;
}

struct mtk_cam_dump_param;
int mtk_cam_job_fill_dump_param(struct mtk_cam_job *job,
				struct mtk_cam_dump_param *p,
				const char *desc);

bool job_has_done_pending(struct mtk_cam_job *job);
/*
 * handle job's done
 *
 * returns:
 *   1: all engines are done
 *   0: waiting for other engine's done
 *  -1: failure
 */
int job_handle_done(struct mtk_cam_job *job);

/* functions used in flow control */
int mtk_cam_job_manually_apply_sensor(struct mtk_cam_job *job);
int mtk_cam_job_manually_apply_isp(struct mtk_cam_job *job,
				   bool wait_completion);

static inline int mtk_cam_job_manually_apply_isp_sync(struct mtk_cam_job *job)
{
	return mtk_cam_job_manually_apply_isp(job, 1);
}

static inline int mtk_cam_job_manually_apply_isp_async(struct mtk_cam_job *job)
{
	return mtk_cam_job_manually_apply_isp(job, 0);
}

int mtk_cam_job_update_clk(struct mtk_cam_job *job);
int mtk_cam_job_update_clk_switching(struct mtk_cam_job *job, bool begin);

int mtk_cam_job_initialize_engines(struct mtk_cam_ctx *ctx,
				   struct mtk_cam_job *job,
				   const struct initialize_params *params);
void mtk_cam_job_clean_prev_img_pool(struct mtk_cam_job *job);

#endif //__MTK_CAM_JOB_H
