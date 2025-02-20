// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2023 MediaTek Inc.

#include "mtk_cam-job_state_impl.h"

static inline int sensor_1st_state(struct state_accessor *s_acc)
{
	return mtk_cam_job_state_get(s_acc->s, SENSOR_1ST_STATE);
}

static inline int isp_1st_state(struct state_accessor *s_acc)
{
	return mtk_cam_job_state_get(s_acc->s, ISP_1ST_STATE);
}

static inline int sensor_2nd_state(struct state_accessor *s_acc)
{
	return mtk_cam_job_state_get(s_acc->s, SENSOR_2ND_STATE);
}

static inline int isp_2nd_state(struct state_accessor *s_acc)
{
	return mtk_cam_job_state_get(s_acc->s, ISP_2ND_STATE);
}

static inline bool is_next_sensor_applied_1st(struct state_accessor *s_acc)
{
	return is_sensor_set(mtk_cam_job_state_get(s_acc->s, SENSOR_2ND_STATE));
}

static inline bool prev_allow_apply_sensor_2nd(struct state_accessor *s_acc)
{
	int isp_state = mtk_cam_job_state_get(s_acc->s, ISP_1ST_STATE);

	return is_isp_ge_outer(isp_state);
}

static inline bool prev_allow_apply_isp_2nd(struct state_accessor *s_acc)
{
	int isp_state = mtk_cam_job_state_get(s_acc->s, ISP_1ST_STATE);

	return is_isp_ge_processing(isp_state);
}

static const struct state_accessor_ops _acc_ops_1st = {
	.prev_allow_apply_sensor = sf_prev_allow_apply_sensor,
	.prev_allow_apply_isp = sf_prev_allow_apply_isp,
	.is_next_sensor_applied = is_next_sensor_applied_1st,
	.cur_sensor_state = sensor_1st_state,
	.cur_isp_state = isp_1st_state,
};

static const struct state_accessor_ops _acc_ops_2nd = {
	.prev_allow_apply_sensor = prev_allow_apply_sensor_2nd,
	.prev_allow_apply_isp = prev_allow_apply_isp_2nd,
	.is_next_sensor_applied = sf_is_next_sensor_applied,
	.cur_sensor_state = sensor_2nd_state,
	.cur_isp_state = isp_2nd_state,
};

static int mstream_send_event(struct mtk_cam_job_state *s,
			      struct transition_param *p)
{
	struct state_accessor s_acc;
	int ret;

	s_acc.head = p->head;
	s_acc.s = s;
	s_acc.seq_no = s->seq_no;
	s_acc.ops = &_acc_ops_1st;

	ret = loop_each_transition(&basic_sensor_tbl, &s_acc,
				   SENSOR_1ST_STATE, p);

	if (!ret)
		loop_each_transition(&basic_isp_tbl, &s_acc,
				     ISP_1ST_STATE, p);

	s_acc.seq_no = next_frame_seq(s->seq_no);
	s_acc.ops = &_acc_ops_2nd;

	ret = loop_each_transition(&basic_sensor_tbl, &s_acc,
				   SENSOR_2ND_STATE, p);

	if (!ret)
		loop_each_transition(&basic_isp_tbl, &s_acc,
				     ISP_2ND_STATE, p);

	return 0;
}

static int _is_next_sensor_applicable(struct mtk_cam_job_state *s)
{
	return is_isp_ge_outer(mtk_cam_job_state_get(s, ISP_2ND_STATE));
}

static int _is_next_isp_applicable(struct mtk_cam_job_state *s)
{
	return is_isp_ge_processing(mtk_cam_job_state_get(s, ISP_2ND_STATE));
}

static int _is_sensor_set(struct mtk_cam_job_state *s)
{
	return is_sensor_set(mtk_cam_job_state_get(s, SENSOR_1ST_STATE));
}

static const struct mtk_cam_job_state_ops mstream_state_ops = {
	.send_event = mstream_send_event,
	.is_next_sensor_applicable = _is_next_sensor_applicable,
	.is_next_isp_applicable = _is_next_isp_applicable,
	.is_sensor_applied = _is_sensor_set,
};

int mtk_cam_job_state_init_mstream(struct mtk_cam_job_state *s,
				   const struct mtk_cam_job_state_cb *cb)
{
	s->ops = &mstream_state_ops;

	mtk_cam_job_state_set(s, SENSOR_1ST_STATE, S_SENSOR_NOT_SET);
	mtk_cam_job_state_set(s, ISP_1ST_STATE, S_ISP_NOT_SET);
	mtk_cam_job_state_set(s, SENSOR_2ND_STATE, S_SENSOR_NOT_SET);
	mtk_cam_job_state_set(s, ISP_2ND_STATE, S_ISP_NOT_SET);

	s->cb = cb;
	s->apply_by_fsm = 1;

	return 0;
}

