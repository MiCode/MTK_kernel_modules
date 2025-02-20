// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2022 MediaTek Inc.

#include "mtk_cam-job_state_impl.h"

static struct state_transition STATE_TRANS(basic_sensor, S_SENSOR_NOT_SET)[] = {
	{
		S_SENSOR_APPLYING, CAMSYS_EVENT_ENQUE,
		guard_apply_sensor, ACTION_APPLY_SENSOR
	},
	{
		S_SENSOR_APPLYING, CAMSYS_EVENT_IRQ_L_CQ_DONE,
		guard_apply_sensor, ACTION_APPLY_SENSOR
	},
	{
		S_SENSOR_APPLYING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_apply_sensor, ACTION_APPLY_SENSOR
	},
};

static struct state_transition STATE_TRANS(basic_sensor, S_SENSOR_APPLIED)[] = {
	{
		S_SENSOR_LATCHED, CAMSYS_EVENT_IRQ_F_VSYNC,
		NULL, 0
	},
};

static struct state_transition STATE_TRANS(basic, S_ISP_COMPOSING)[] = {
	{
		S_ISP_APPLYING, CAMSYS_EVENT_ACK,
		guard_ack_apply_directly, ACTION_APPLY_ISP
	},
	{
		S_ISP_COMPOSED, CAMSYS_EVENT_ACK,
		guard_ack_eq, 0
	},
};

static struct state_transition STATE_TRANS(basic, S_ISP_COMPOSED)[] = {
	{
		S_ISP_APPLYING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_apply_isp, ACTION_APPLY_ISP,
	},
};

static struct state_transition STATE_TRANS(basic, S_ISP_APPLYING)[] = {
#ifdef DO_WE_NEED_THIS /* is it possible to miss cq_done? */
	{
		S_ISP_PROCESSING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_inner_eq, 0
	},
#endif
	{
		S_ISP_OUTER, CAMSYS_EVENT_IRQ_L_CQ_DONE,
		guard_outer_eq, 0
	},
};

static struct state_transition STATE_TRANS(basic, S_ISP_OUTER)[] = {
	{
		S_ISP_PROCESSING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_inner_eq, 0
	},
};

static struct state_transition STATE_TRANS(basic, S_ISP_PROCESSING)[] = {
	{
		S_ISP_DONE, CAMSYS_EVENT_IRQ_FRAME_DONE,
		guard_inner_eq, ACTION_BUFFER_DONE
	},
	{ /* note: should handle frame_done first if sof/p1done come together */
		S_ISP_SENSOR_MISMATCHED, CAMSYS_EVENT_IRQ_L_SOF,
		guard_hw_retry_mismatched, 0
	},
	{
		S_ISP_PROCESSING, CAMSYS_EVENT_IRQ_L_SOF,
		guard_hw_retry_matched, 0
	},
#ifdef TO_REMOVE
	{
		S_ISP_DONE, CAMSYS_EVENT_IRQ_SOF,
		guard_inner_ge, ACTION_BUFFER_DONE
	},
#endif
};

static struct state_transition STATE_TRANS(basic, S_ISP_SENSOR_MISMATCHED)[] = {
	{
		S_ISP_DONE_MISMATCHED, CAMSYS_EVENT_IRQ_FRAME_DONE,
		guard_inner_eq, ACTION_BUFFER_DONE
	},
#ifdef TO_REMOVE
	{
		S_ISP_DONE_MISMATCHED, CAMSYS_EVENT_IRQ_SOF,
		guard_inner_ge, ACTION_BUFFER_DONE
	},
#endif
};

static struct transitions_entry basic_sensor_entries[NR_S_SENSOR_STATE] = {
	ADD_TRANS_ENTRY(basic_sensor, S_SENSOR_NOT_SET),
	ADD_TRANS_ENTRY(basic_sensor, S_SENSOR_APPLIED),
};
struct state_table basic_sensor_tbl = {
	.entries = basic_sensor_entries,
	.size = ARRAY_SIZE(basic_sensor_entries),
};

static struct transitions_entry basic_isp_entries[NR_S_ISP_STATE] = {
	ADD_TRANS_ENTRY(basic, S_ISP_COMPOSING),
	ADD_TRANS_ENTRY(basic, S_ISP_COMPOSED),
	ADD_TRANS_ENTRY(basic, S_ISP_APPLYING),
	ADD_TRANS_ENTRY(basic, S_ISP_OUTER),
	ADD_TRANS_ENTRY(basic, S_ISP_PROCESSING),
	ADD_TRANS_ENTRY(basic, S_ISP_SENSOR_MISMATCHED),
	//ADD_TRANS_ENTRY(basic, S_ISP_DONE),
	//ADD_TRANS_ENTRY(basic, S_ISP_DONE_MISMATCHED),
};
struct state_table basic_isp_tbl = {
	.entries = basic_isp_entries,
	.size = ARRAY_SIZE(basic_isp_entries),
};

static const struct state_accessor_ops _acc_ops = {
	.prev_allow_apply_sensor = sf_prev_allow_apply_sensor,
	.prev_allow_apply_isp = sf_prev_allow_apply_isp,
	.is_next_sensor_applied = sf_is_next_sensor_applied,
	.cur_sensor_state = sf_cur_sensor_state,
	.cur_isp_state = sf_cur_isp_state,
};

static int basic_send_event(struct mtk_cam_job_state *s,
			    struct transition_param *p)
{
	struct state_accessor s_acc;
	int ret;

	s_acc.head = p->head;
	s_acc.s = s;
	s_acc.seq_no = s->seq_no;
	s_acc.ops = &_acc_ops;

	ret = loop_each_transition(&basic_sensor_tbl, &s_acc, SENSOR_STATE, p);

	/* note: beware of '!ret' here
	 * for current scenarios, we won't update sensor & isp state at same event
	 * use '!ret' to skip isp transition if sensor already did.
	 */
	if (!ret)
		loop_each_transition(&basic_isp_tbl, &s_acc, ISP_STATE, p);

	return 0;
}

static int _is_next_sensor_applicable(struct mtk_cam_job_state *s)
{
	return is_isp_ge_outer(mtk_cam_job_state_get(s, ISP_STATE));
}

static int _is_next_isp_applicable(struct mtk_cam_job_state *s)
{
	return is_isp_ge_processing(mtk_cam_job_state_get(s, ISP_STATE));
}

static int _is_sensor_set(struct mtk_cam_job_state *s)
{
	return is_sensor_set(mtk_cam_job_state_get(s, SENSOR_STATE));
}

static const struct mtk_cam_job_state_ops basic_state_ops = {
	.send_event = basic_send_event,
	.is_next_sensor_applicable = _is_next_sensor_applicable,
	.is_next_isp_applicable = _is_next_isp_applicable,
	.is_sensor_applied = _is_sensor_set,
};

int mtk_cam_job_state_init_basic(struct mtk_cam_job_state *s,
				 const struct mtk_cam_job_state_cb *cb,
				 int with_sensor_ctrl)
{
	s->ops = &basic_state_ops;

	mtk_cam_job_state_set(s, SENSOR_STATE,
			      with_sensor_ctrl ?
			      S_SENSOR_NOT_SET : S_SENSOR_NONE);
	mtk_cam_job_state_set(s, ISP_STATE, S_ISP_NOT_SET);

	s->cb = cb;
	s->apply_by_fsm = 1;

	return 0;
}

