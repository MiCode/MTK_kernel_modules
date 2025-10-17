/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "mtk_cam-job_state_impl.h"

/* composed -> applying -> outer -> processing -> ready -> applying raw ->outer raw -> process raw -> done
 * 0. not_set      -> composed       : by CAMSYS_EVENT_ENQUE -> ACTION_COMPOSE_CQ
 * 1. composed     -> applying       : by CAMSYS_EVENT_IRQ_L_SOF -> ACTION_APPLY_ISP_EXTMETA_PD_EXTISP
 * 2. applying     -> outer          : by CAMSYS_EVENT_IRQ_EXTMETA_CQ_DONE
 * 3. outer        -> processing	 : by CAMSYS_EVENT_IRQ_L_SOF
 * 4. processing   -> done_ready	 : by CAMSYS_EVENT_IRQ_EXTMETA_FRAME_DONE
 * 5. done_ready   -> applying raw   : by CAMSYS_EVENT_IRQ_EXTMETA_FRAME_DONE/CAMSYS_EVENT_IRQ_FRAME_DONE
 * 6. applying raw -> process raw    : raw's cq done + trigger m2m
 * 7. process raw  -> done           : raw done
 */

static struct state_transition STATE_TRANS(ts_sensor, S_SENSOR_NOT_SET)[] = {
	{
		S_SENSOR_APPLYING, CAMSYS_EVENT_ENQUE,
		guard_apply_sensor, ACTION_APPLY_SENSOR
	},
	{
		S_SENSOR_APPLYING, CAMSYS_EVENT_IRQ_F_VSYNC,
		guard_apply_sensor, ACTION_APPLY_SENSOR
	},
};

static struct state_transition STATE_TRANS(ts_sensor, S_SENSOR_APPLIED)[] = {
	{
		S_SENSOR_LATCHED, CAMSYS_EVENT_IRQ_F_VSYNC,
		NULL, 0
	},
};
static struct state_transition STATE_TRANS(ts, S_ISP_NOT_SET)[] = {
	{
		S_ISP_COMPOSING, CAMSYS_EVENT_ENQUE,
		guard_next_compose, ACTION_COMPOSE_CQ
	},
	{
		S_ISP_COMPOSING, CAMSYS_EVENT_ACK,
		guard_next_compose, ACTION_COMPOSE_CQ
	},
};

static struct state_transition STATE_TRANS(ts, S_ISP_COMPOSING)[] = {
	{
		S_ISP_APPLYING, CAMSYS_EVENT_ACK,
		guard_ack_apply_directly, ACTION_APPLY_ISP_EXTMETA_PD_EXTISP
	},
	{
		S_ISP_COMPOSED, CAMSYS_EVENT_ACK,
		guard_ack_eq, 0
	},
};

static struct state_transition STATE_TRANS(ts, S_ISP_COMPOSED)[] = {
	{
		S_ISP_APPLYING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_apply_isp, ACTION_APPLY_ISP_EXTMETA_PD_EXTISP,
	},
};

static struct state_transition STATE_TRANS(ts, S_ISP_APPLYING)[] = {
	{
		S_ISP_OUTER, CAMSYS_EVENT_IRQ_EXTMETA_CQ_DONE,
		guard_outer_eq, 0
	},
};

static struct state_transition STATE_TRANS(ts, S_ISP_OUTER)[] = {
	{
		S_ISP_PROCESSING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_inner_eq, ACTION_CHECK_PROCESSING
	},
};

static struct state_transition STATE_TRANS(ts, S_ISP_PROCESSING)[] = {
	{
		S_ISP_DONE_READY, CAMSYS_EVENT_IRQ_EXTMETA_FRAME_DONE,
		guard_inner_eq, 0
	},
	{ /* note: should handle frame_done first if sof/p1done come together */
		S_ISP_DONE_READY, CAMSYS_EVENT_IRQ_L_SOF,
		guard_hw_retry_mismatched, 0
	},
	{
		S_ISP_PROCESSING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_hw_retry_matched, 0
	},
};

static struct state_transition STATE_TRANS(ts, S_ISP_DONE_READY)[] = {
	{
		S_ISP_DONE_READY, CAMSYS_EVENT_IRQ_TRY_TS_TRIGGER,
		guard_apply_ts_m2m, ACTION_APPLY_ISP_PROCRAW_EXTISP
	},
};

static struct state_transition STATE_TRANS(ts, S_ISP_APPLYING_PROCRAW)[] = {
	{
		S_ISP_PROCESSING_RAW, CAMSYS_EVENT_IRQ_L_CQ_DONE,
		guard_outer_eq_ts, ACTION_TRIGGER
	},
};

static struct state_transition STATE_TRANS(ts, S_ISP_PROCESSING_RAW)[] = {
	{
		S_ISP_DONE, CAMSYS_EVENT_IRQ_FRAME_DONE,
		NULL, 0
	},
};

static struct transitions_entry ts_sensor_entries[NR_S_SENSOR_STATE] = {
	ADD_TRANS_ENTRY(ts_sensor, S_SENSOR_NOT_SET),
	ADD_TRANS_ENTRY(ts_sensor, S_SENSOR_APPLIED),
};

struct state_table ts_sensor_tbl = {
	.entries = ts_sensor_entries,
	.size = ARRAY_SIZE(ts_sensor_entries),
};

static struct transitions_entry ts_isp_entries[NR_S_ISP_STATE] = {
	ADD_TRANS_ENTRY(ts, S_ISP_NOT_SET),
	ADD_TRANS_ENTRY(ts, S_ISP_COMPOSING),
	ADD_TRANS_ENTRY(ts, S_ISP_COMPOSED),
	ADD_TRANS_ENTRY(ts, S_ISP_APPLYING),
	ADD_TRANS_ENTRY(ts, S_ISP_OUTER),
	ADD_TRANS_ENTRY(ts, S_ISP_PROCESSING),
	ADD_TRANS_ENTRY(ts, S_ISP_DONE_READY),
	ADD_TRANS_ENTRY(ts, S_ISP_APPLYING_PROCRAW),
	ADD_TRANS_ENTRY(ts, S_ISP_PROCESSING_RAW),
};
struct state_table ts_isp_tbl = {
	.entries = ts_isp_entries,
	.size = ARRAY_SIZE(ts_isp_entries),
};
static bool is_isp_ge_processingprocraw(int isp_state)
{
	return isp_state >= S_ISP_DONE;
}

static bool _prev_allow_apply_extisp_procraw(struct state_accessor *s_acc)
{
	struct mtk_cam_job_state *prev_s = prev_state(s_acc->s, s_acc->head);

	return !prev_s || is_isp_ge_processingprocraw(mtk_cam_job_state_get(prev_s, ISP_STATE));
}


static const struct state_accessor_ops ts_acc_ops = {
	.prev_allow_apply_sensor = sf_prev_allow_apply_sensor,
	.prev_allow_apply_isp = sf_prev_allow_apply_isp,
	.prev_allow_apply_extisp_procraw = _prev_allow_apply_extisp_procraw,
	.is_next_sensor_applied = sf_is_next_sensor_applied,
	.cur_sensor_state = sf_cur_sensor_state,
	.cur_isp_state = sf_cur_isp_state,
};

static int ts_send_event(struct mtk_cam_job_state *s,
			    struct transition_param *p)
{
	struct state_accessor s_acc;
	int ret;

	s_acc.head = p->head;
	s_acc.s = s;
	s_acc.seq_no = s->seq_no;
	s_acc.ops = &ts_acc_ops;
	p->s_params = &s->s_params;
	p->cq_trigger_thres = s->cq_trigger_thres_ns;

	ret = loop_each_transition(&ts_sensor_tbl, &s_acc, SENSOR_STATE, p);

	ret = loop_each_transition(&ts_isp_tbl, &s_acc, ISP_STATE, p);

	return 0;
}

static int _is_next_sensor_applicable(struct mtk_cam_job_state *s)
{
	return is_isp_ge_outer(mtk_cam_job_state_get(s, ISP_STATE));
}

static int _is_next_isp_applicable(struct mtk_cam_job_state *s)
{
	return is_isp_ge_processing(mtk_cam_job_state_get(s, ISP_STATE)) &&
		!is_isp_aborted(mtk_cam_job_state_get(s, ISP_STATE));
}

static int _is_sensor_set(struct mtk_cam_job_state *s)
{
	return is_sensor_set(mtk_cam_job_state_get(s, SENSOR_STATE));
}

static const struct mtk_cam_job_state_ops ts_state_ops = {
	.send_event = ts_send_event,
	.is_next_sensor_applicable = _is_next_sensor_applicable,
	.is_next_isp_applicable = _is_next_isp_applicable,
	.is_sensor_applied = _is_sensor_set,
};

int mtk_cam_job_state_init_ts(struct mtk_cam_job_state *s,
				 const struct mtk_cam_job_state_cb *cb,
				 int with_sensor_ctrl)
{
	s->ops = &ts_state_ops;

	mtk_cam_job_state_set(s, SENSOR_STATE,
			      with_sensor_ctrl ?
			      S_SENSOR_NOT_SET : S_SENSOR_NONE);
	mtk_cam_job_state_set(s, ISP_STATE, S_ISP_NOT_SET);

	s->cb = cb;
	s->apply_by_fsm = 1;
	s->compose_by_fsm = 1;

	return 0;
}

