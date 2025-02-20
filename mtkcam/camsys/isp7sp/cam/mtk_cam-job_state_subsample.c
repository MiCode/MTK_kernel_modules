// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2022 MediaTek Inc.

#include "mtk_cam-job_state_impl.h"

static struct state_transition STATE_TRANS(subsample_sensor, S_SENSOR_NOT_SET)[] = {
	{
		S_SENSOR_APPLYING, CAMSYS_EVENT_TIMER_SENSOR,
		guard_apply_sensor_subsample, ACTION_APPLY_SENSOR
	},
};

static struct state_transition STATE_TRANS(subsample, S_ISP_NOT_SET)[] = {
	{
		S_ISP_COMPOSING, CAMSYS_EVENT_ENQUE,
		guard_next_compose, ACTION_COMPOSE_CQ
	},
	{
		S_ISP_COMPOSING, CAMSYS_EVENT_ACK,
		guard_next_compose, ACTION_COMPOSE_CQ
	},
};

static struct state_transition STATE_TRANS(subsample, S_ISP_COMPOSING)[] = {
	{
		S_ISP_APPLYING, CAMSYS_EVENT_ACK,
		guard_ack_apply_directly_subsample, ACTION_APPLY_ISP
	},
	{
		S_ISP_COMPOSED, CAMSYS_EVENT_ACK,
		guard_ack_eq, 0
	},
};

static struct state_transition STATE_TRANS(subsample, S_ISP_COMPOSED)[] = {
	{
		S_ISP_APPLYING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_apply_isp_subsample, ACTION_APPLY_ISP,
	},
};

static struct state_transition STATE_TRANS(subsample, S_ISP_APPLYING)[] = {
	{
		S_ISP_OUTER, CAMSYS_EVENT_IRQ_L_CQ_DONE,
		guard_outer_eq_subsample, ACTION_CQ_DONE
	},
	{
		S_ISP_PROCESSING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_inner_eq, 0
	},
};

static struct state_transition STATE_TRANS(subsample, S_ISP_OUTER)[] = {
	{
		S_ISP_PROCESSING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_inner_eq, 0
	},
};

static struct state_transition STATE_TRANS(subsample, S_ISP_PROCESSING)[] = {
	{
		S_ISP_DONE, CAMSYS_EVENT_IRQ_FRAME_DONE,
		guard_inner_eq, 0
	},
	{ /* note: should handle frame_done first if sof/p1done come together */
		S_ISP_SENSOR_MISMATCHED, CAMSYS_EVENT_IRQ_L_SOF,
		guard_hw_retry_mismatched, 0
	},
	{
		S_ISP_PROCESSING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_hw_retry_matched, 0
	},
};

static struct state_transition STATE_TRANS(subsample, S_ISP_SENSOR_MISMATCHED)[] = {
	{
		S_ISP_DONE_MISMATCHED, CAMSYS_EVENT_IRQ_FRAME_DONE,
		guard_inner_eq, 0
	},
#ifdef TO_REMOVE
	{
		S_ISP_DONE_MISMATCHED, CAMSYS_EVENT_IRQ_SOF,
		guard_inner_ge, 0
	},
#endif
};

static struct transitions_entry basic_sensor_entries[NR_S_SENSOR_STATE] = {
	ADD_TRANS_ENTRY(subsample_sensor, S_SENSOR_NOT_SET),
};
DECL_STATE_TABLE(subsample_sensor_tbl, basic_sensor_entries);

static struct transitions_entry basic_isp_entries[NR_S_ISP_STATE] = {
	ADD_TRANS_ENTRY(subsample, S_ISP_NOT_SET),
	ADD_TRANS_ENTRY(subsample, S_ISP_COMPOSING),
	ADD_TRANS_ENTRY(subsample, S_ISP_COMPOSED),
	ADD_TRANS_ENTRY(subsample, S_ISP_APPLYING),
	ADD_TRANS_ENTRY(subsample, S_ISP_OUTER),
	ADD_TRANS_ENTRY(subsample, S_ISP_PROCESSING),
	ADD_TRANS_ENTRY(subsample, S_ISP_SENSOR_MISMATCHED),
	//ADD_TRANS_ENTRY(basic, S_ISP_DONE),
	//ADD_TRANS_ENTRY(basic, S_ISP_DONE_MISMATCHED),
};
DECL_STATE_TABLE(subsample_isp_tbl, basic_isp_entries);

static const struct state_accessor_ops _acc_ops = {
	.prev_allow_apply_sensor = sf_prev_allow_apply_sensor,
	.prev_allow_apply_isp = sf_prev_allow_apply_isp,
	.is_next_sensor_applied = sf_is_next_sensor_applied,
	.cur_sensor_state = sf_cur_sensor_state,
	.cur_isp_state = sf_cur_isp_state,
};

static int subsample_send_event(struct mtk_cam_job_state *s,
			    struct transition_param *p)
{
	struct state_accessor s_acc;

	s_acc.head = p->head;
	s_acc.s = s;
	s_acc.seq_no = s->seq_no;
	s_acc.ops = &_acc_ops;
	p->s_params = &s->s_params;
	p->cq_trigger_thres = s->cq_trigger_thres_ns;
	loop_each_transition(&subsample_sensor_tbl,
				   &s_acc, SENSOR_STATE, p);

	loop_each_transition(&subsample_isp_tbl,
				     &s_acc, ISP_STATE, p);

	return 0;
}

static int _is_next_sensor_applicable(struct mtk_cam_job_state *s)
{
	/* since in subsample, sensor setting is applied after cq
	 * we don't need to consider previous job's isp status.
	 * make sure previous job's sensor i2c transmission is done.
	 */
	return is_sensor_ge_applied(mtk_cam_job_state_get(s, SENSOR_STATE));
}

static int _is_next_isp_applicable(struct mtk_cam_job_state *s)
{
	return is_isp_ge_processing(mtk_cam_job_state_get(s, ISP_STATE));
}

static int _is_sensor_set(struct mtk_cam_job_state *s)
{
	return is_sensor_set(mtk_cam_job_state_get(s, SENSOR_STATE));
}

static const struct mtk_cam_job_state_ops subsample_state_ops = {
	.send_event = subsample_send_event,
	.is_next_sensor_applicable = _is_next_sensor_applicable,
	.is_next_isp_applicable = _is_next_isp_applicable,
	.is_sensor_applied = _is_sensor_set,
};

int mtk_cam_job_state_init_subsample(struct mtk_cam_job_state *s,
				 const struct mtk_cam_job_state_cb *cb,
				 int with_sensor_ctrl)
{
	s->ops = &subsample_state_ops;

	mtk_cam_job_state_set(s, SENSOR_STATE,
			      with_sensor_ctrl ?
			      S_SENSOR_NOT_SET : S_SENSOR_NONE);
	mtk_cam_job_state_set(s, ISP_STATE, S_ISP_NOT_SET);

	s->cb = cb;
	s->apply_by_fsm = 1;
	s->compose_by_fsm = 1;

	return 0;
}

