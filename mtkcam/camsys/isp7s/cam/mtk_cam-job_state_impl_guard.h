/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_CAM_JOB_STATE_IMPL_GUARD_H
#define __MTK_CAM_JOB_STATE_IMPL_GUARD_H

struct state_accessor;
struct state_accessor_ops {
	/*
	 * to check if previous frame's status is allowing to apply current sensor setting
	 */
	bool (*prev_allow_apply_sensor)(struct state_accessor *s_acc);

	/*
	 * to check if previous frame's status is allowing to apply current isp setting
	 */
	bool (*prev_allow_apply_isp)(struct state_accessor *s_acc);

	/*
	 * to check if next frame's sensor is already applied (for sensor-mismatched case)
	 */
	bool (*is_next_sensor_applied)(struct state_accessor *s_acc);

	/*
	 * fetch current status
	 */
	int (*cur_sensor_state)(struct state_accessor *s_acc);
	int (*cur_isp_state)(struct state_accessor *s_acc);
};

struct state_accessor {
	struct list_head *head;
	struct mtk_cam_job_state *s;
	int seq_no;

	const struct state_accessor_ops *ops;
};

static inline int cur_seq_no(struct state_accessor *s_acc)
{
	return s_acc->seq_no;
}

static inline struct mtk_cam_job_state *prev_state(struct mtk_cam_job_state *s,
						   struct list_head *head)
{
	return list_is_first(&s->list, head) ?
		NULL : list_prev_entry(s, list);
}

static inline struct mtk_cam_job_state *next_state(struct mtk_cam_job_state *s,
						   struct list_head *head)
{
	return list_is_last(&s->list, head) ?
		NULL : list_next_entry(s, list);
}

/* common state_accessor_ops implementations */
static inline bool _state_ge(struct mtk_cam_job_state *s,
			     int state_type, int state)
{
	return mtk_cam_job_state_get(s, ISP_STATE) >= state;
}

/* note: 'sf' stands for single frame */
static inline bool sf_prev_allow_apply_sensor(struct state_accessor *s_acc)
{
	struct mtk_cam_job_state *prev_s = prev_state(s_acc->s, s_acc->head);

	return !prev_s || ops_call(prev_s, is_next_sensor_applicable);
}

static inline bool sf_prev_allow_apply_isp(struct state_accessor *s_acc)
{
	struct mtk_cam_job_state *prev_s = prev_state(s_acc->s, s_acc->head);

	return !prev_s || ops_call(prev_s, is_next_isp_applicable);
}

static inline bool sf_is_next_sensor_applied(struct state_accessor *s_acc)
{
	struct mtk_cam_job_state *next_s = next_state(s_acc->s, s_acc->head);

	return next_s && ops_call(next_s, is_sensor_applied);
}

static inline int sf_cur_sensor_state(struct state_accessor *s_acc)
{
	return mtk_cam_job_state_get(s_acc->s, SENSOR_STATE);
}

static inline int sf_cur_isp_state(struct state_accessor *s_acc)
{
	return mtk_cam_job_state_get(s_acc->s, ISP_STATE);
}

/*
 * guard functions
 */

static inline int guard_ack_eq(struct state_accessor *s_acc,
			       struct transition_param *p)
{
	return p->info->ack_seq_no == cur_seq_no(s_acc);
}

static inline int guard_outer_eq(struct state_accessor *s_acc,
				 struct transition_param *p)
{
	return p->info->outer_seq_no == cur_seq_no(s_acc);
}

static inline int guard_inner_eq(struct state_accessor *s_acc,
				 struct transition_param *p)
{
	return p->info->inner_seq_no == cur_seq_no(s_acc);
}

static inline int guard_inner_ge(struct state_accessor *s_acc,
				 struct transition_param *p)
{
	return p->info->inner_seq_no >= cur_seq_no(s_acc);
}

/* TODO(AY): may be removed */
static inline int prev_isp_state_ge(struct mtk_cam_job_state *s,
				    struct list_head *list_head,
				    int state)
{
	struct mtk_cam_job_state *prv_s = prev_state(s, list_head);

	return !prv_s || mtk_cam_job_state_get(prv_s, ISP_STATE) >= state;
}

static inline bool allow_applying_hw(struct state_accessor *s_acc)
{
	return s_acc->s->apply_by_fsm;
}

static inline bool valid_i2c_period(struct transition_param *p)
{
	if (unlikely(!p->s_params))
		return false;

	return (p->event_ts - p->info->sof_ts_ns) < p->s_params->i2c_thres_ns;
}

static inline int guard_apply_sensor_subsample(struct state_accessor *s_acc,
					       struct transition_param *p)
{
	/* TODO: add ts check */
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, cur_isp_state) >= S_ISP_APPLYING;
}

static inline int guard_apply_sensor(struct state_accessor *s_acc,
				     struct transition_param *p)
{
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_sensor) &&
		valid_i2c_period(p);
}

static inline bool is_sensor_set(int sensor_state)
{
	return sensor_state >= S_SENSOR_LATCHED;
}

static inline bool is_isp_ge_outer(int isp_state)
{
	return isp_state >= S_ISP_OUTER;
}

static inline bool is_isp_ge_processing(int isp_state)
{
	return isp_state >= S_ISP_PROCESSING;
}

static inline bool current_sensor_ready(struct state_accessor *s_acc)
{
	int s_state = ops_call(s_acc, cur_sensor_state);

	return is_sensor_set(s_state) || s_state == S_SENSOR_NONE;
}

static inline int guard_apply_isp(struct state_accessor *s_acc,
				  struct transition_param *p)
{
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_isp) &&
		current_sensor_ready(s_acc);
}

static inline int guard_apply_m2m(struct state_accessor *s_acc,
				  struct transition_param *p)
{
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_isp);
}
static inline int guard_apply_isp_subsample(struct state_accessor *s_acc,
				  struct transition_param *p)
{
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_isp);
}

static inline int guard_ack_apply_directly(struct state_accessor *s_acc,
					   struct transition_param *p)
{
	/* TODO: check timer? */
	return guard_ack_eq(s_acc, p) && guard_apply_isp(s_acc, p);
}

static inline int guard_ack_apply_m2m_directly(struct state_accessor *s_acc,
					       struct transition_param *p)
{
	return guard_ack_eq(s_acc, p) && guard_apply_m2m(s_acc, p);
}

static inline int guard_hw_retry_mismatched(struct state_accessor *s_acc,
					    struct transition_param *p)
{
	return ops_call(s_acc, is_next_sensor_applied);
}

static inline int guard_hw_retry_matched(struct state_accessor *s_acc,
					 struct transition_param *p)
{
	return !guard_hw_retry_mismatched(s_acc, p);
}

#endif //__MTK_CAM_JOB_STATE_IMPL_GUARD_H
