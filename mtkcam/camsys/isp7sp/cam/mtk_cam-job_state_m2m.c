// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2022 MediaTek Inc.

#include "mtk_cam-job_state_impl.h"

static struct state_transition STATE_TRANS(m2m, S_ISP_NOT_SET)[] = {
	{
		S_ISP_COMPOSING, CAMSYS_EVENT_ENQUE,
		NULL, ACTION_COMPOSE_CQ
	},
};

static struct state_transition STATE_TRANS(m2m, S_ISP_COMPOSING)[] = {
	{
		S_ISP_APPLYING, CAMSYS_EVENT_ACK,
		guard_ack_apply_m2m_directly, ACTION_APPLY_ISP
	},
	{
		S_ISP_COMPOSED, CAMSYS_EVENT_ACK,
		guard_ack_eq, 0
	},
};

static struct state_transition STATE_TRANS(m2m, S_ISP_COMPOSED)[] = {
	{
		S_ISP_APPLYING, CAMSYS_EVENT_IRQ_FRAME_DONE,
		guard_apply_m2m, ACTION_APPLY_ISP,
	},
};

static struct state_transition STATE_TRANS(m2m, S_ISP_APPLYING)[] = {
	{
		S_ISP_PROCESSING, CAMSYS_EVENT_IRQ_L_CQ_DONE,
		guard_outer_eq, ACTION_TRIGGER
	},
};

static struct state_transition STATE_TRANS(m2m, S_ISP_PROCESSING)[] = {
	{
		S_ISP_DONE, CAMSYS_EVENT_IRQ_FRAME_DONE,
		NULL, 0
	},
};
static struct transitions_entry m2m_isp_entries[NR_S_ISP_STATE] = {
	ADD_TRANS_ENTRY(m2m, S_ISP_NOT_SET),
	ADD_TRANS_ENTRY(m2m, S_ISP_COMPOSING),
	ADD_TRANS_ENTRY(m2m, S_ISP_COMPOSED),
	ADD_TRANS_ENTRY(m2m, S_ISP_APPLYING),
	ADD_TRANS_ENTRY(m2m, S_ISP_PROCESSING),
};
DECL_STATE_TABLE(m2m_isp_tbl, m2m_isp_entries);

static const struct state_accessor_ops _acc_ops = {
	.prev_allow_apply_sensor = sf_prev_allow_apply_sensor,
	.prev_allow_apply_isp = sf_prev_allow_apply_isp,
	.is_next_sensor_applied = sf_is_next_sensor_applied,
	.cur_sensor_state = sf_cur_sensor_state,
	.cur_isp_state = sf_cur_isp_state,
};

static int m2m_send_event(struct mtk_cam_job_state *s,
			    struct transition_param *p)
{
	struct state_accessor s_acc;
	int ret;

	s_acc.head = p->head;
	s_acc.s = s;
	s_acc.seq_no = s->seq_no;
	s_acc.ops = &_acc_ops;
	p->s_params = &s->s_params;

	ret = loop_each_transition(&m2m_isp_tbl, &s_acc, ISP_STATE, p);

	return ret < 0 ? -1 : 0;
}

static int _is_next_sensor_applicable(struct mtk_cam_job_state *s)
{
	return 0;
}

static int _is_next_isp_applicable(struct mtk_cam_job_state *s)
{
	return mtk_cam_job_state_get(s, ISP_STATE) >= S_ISP_DONE;
}

static int _is_sensor_set(struct mtk_cam_job_state *s)
{
	return 0;
}

static struct mtk_cam_job_state_ops m2m_state_ops = {
	.send_event = m2m_send_event,
	.is_next_sensor_applicable = _is_next_sensor_applicable,
	.is_next_isp_applicable = _is_next_isp_applicable,
	.is_sensor_applied = _is_sensor_set,
};

int mtk_cam_job_state_init_m2m(struct mtk_cam_job_state *s,
			       const struct mtk_cam_job_state_cb *cb)
{
	s->ops = &m2m_state_ops;

	mtk_cam_job_state_set(s, ISP_STATE, S_ISP_NOT_SET);

	s->cb = cb;
	s->apply_by_fsm = 1;

	return 0;
}

