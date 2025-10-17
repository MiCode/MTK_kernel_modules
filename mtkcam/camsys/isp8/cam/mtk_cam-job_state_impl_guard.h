/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_CAM_JOB_STATE_IMPL_GUARD_H
#define __MTK_CAM_JOB_STATE_IMPL_GUARD_H

#define I2C_THRES_FROM_L_SOF_NS 3000000
#define SQC_THRES_FROM_L_SOF_NS 3000000
#define SCQ_THRES_FROM_F_SOF_NS 15000000

#include "mtk_cam-job.h"

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
	 * to check if previous frame's status is allowing to apply current isp setting
	 */
	bool (*prev_allow_apply_extisp_procraw)(struct state_accessor *s_acc);
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
static inline bool allow_composing(struct state_accessor *s_acc)
{
	return s_acc->s->compose_by_fsm;
}

static inline int guard_next_compose_m2m(struct state_accessor *s_acc,
			       struct transition_param *p)
{
	return allow_composing(s_acc);
}

static inline int guard_next_compose(struct state_accessor *s_acc,
			       struct transition_param *p)
{
	int ret = 0;

	spin_lock(p->info_lock);
	ret = allow_composing(s_acc) &&
		frame_seq_diff(cur_seq_no(s_acc), p->info->ack_seq_no) == 1;
	if (ret == 0 && allow_composing(s_acc))
		pr_info("[mtk-cam:guard_next_compose] allow/cur/ack:%d/%d/%d (%llu)",
			allow_composing(s_acc), cur_seq_no(s_acc), p->info->ack_seq_no,
			ktime_get_boottime_ns());
	spin_unlock(p->info_lock);

	return ret;
}

static inline int guard_ack_eq(struct state_accessor *s_acc,
			       struct transition_param *p)
{
	int ret = 0;

	spin_lock(p->info_lock);
	ret = p->info->ack_seq_no == cur_seq_no(s_acc);
	spin_unlock(p->info_lock);

	return ret;
}
static inline int guard_outer_eq_ts(struct state_accessor *s_acc,
				 struct transition_param *p)
{
	int ret = 0;

	spin_lock(p->info_lock);
	ret = p->info->outer_seq_no_ts == cur_seq_no(s_acc);
	spin_unlock(p->info_lock);

	return ret;
}

static inline int guard_outer_eq(struct state_accessor *s_acc,
				 struct transition_param *p)
{
	int ret = 0;

	spin_lock(p->info_lock);
	ret = p->info->outer_seq_no == cur_seq_no(s_acc);
	spin_unlock(p->info_lock);

	return ret;
}

static inline int guard_outer_eq_subsample(struct state_accessor *s_acc,
				 struct transition_param *p)
{
	int cur_seq = 0, ret = 0;

	spin_lock(p->info_lock);
	/* outer regs doesn't updated when cq done in subsample mode */
	cur_seq = p->info->outer_seq_no == p->info->inner_seq_no ?
			p->info->outer_seq_no + 1 : p->info->outer_seq_no;
	ret = ( cur_seq == cur_seq_no(s_acc)) &&
		ops_call(s_acc, cur_isp_state) >= S_ISP_APPLYING;
	spin_unlock(p->info_lock);

	return ret;
}

static inline int guard_inner_eq(struct state_accessor *s_acc,
				 struct transition_param *p)
{
	int ret = 0;

	spin_lock(p->info_lock);
	ret = p->info->inner_seq_no == cur_seq_no(s_acc);
	spin_unlock(p->info_lock);

	return ret;
}

static inline int guard_inner_ge(struct state_accessor *s_acc,
				 struct transition_param *p)
{
	int ret = 0;

	spin_lock(p->info_lock);
	ret = frame_seq_ge(p->info->inner_seq_no, cur_seq_no(s_acc));
	spin_unlock(p->info_lock);

	return ret;
}

static inline int guard_inner_greater(struct state_accessor *s_acc,
				 struct transition_param *p)
{
	int ret = 0;

	spin_lock(p->info_lock);
	ret = frame_seq_gt(p->info->inner_seq_no, cur_seq_no(s_acc));
	spin_unlock(p->info_lock);

	return ret;
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

static inline bool bypass_guard_check(struct transition_param *p)
{
	if (unlikely(!p->s_params))
		return false;

	return p->s_params->always_allow;
}

static inline bool valid_i2c_period(struct transition_param *p)
{
	int ret = false;

	if (unlikely(!p->s_params))
		return false;
	spin_lock(p->info_lock);
	ret = ((p->event_ts - p->info->sof_ts_ns) < p->s_params->i2c_thres_ns) ||
		((p->event_ts - p->info->sof_l_ts_ns) < I2C_THRES_FROM_L_SOF_NS);
	spin_unlock(p->info_lock);

	return ret;
}

static inline bool allow_subsample_2_i2c_by_ts(
	struct state_accessor *s_acc, struct transition_param *p)
{
	bool ret = false;

	spin_lock(p->info_lock);
	ret = ((s_acc->s->s_params.subsample == 2) &&
		frame_seq_diff(s_acc->seq_no, p->info->outer_seq_no) == 1 &&
		(p->event_ts - p->info->sof_ts_ns) < 2000000);
	spin_unlock(p->info_lock);

	return ret;
}
static inline bool allow_subsample_4_i2c_by_inner(
	struct state_accessor *s_acc, struct transition_param *p)
{
	bool ret = false;

	spin_lock(p->info_lock);
	ret = ((s_acc->s->s_params.subsample == 4) &&
		frame_seq_diff(s_acc->seq_no, p->info->inner_seq_no) == 1);
	spin_unlock(p->info_lock);

	return ret;
}

static inline bool valid_i2c_period_l(struct transition_param *p)
{
	bool ret = false;
	if (unlikely(!p->s_params))
		return false;
	spin_lock(p->info_lock);
	ret = (p->event_ts - p->info->sof_ts_ns) >= (p->event_ts - p->info->sof_l_ts_ns);
	spin_unlock(p->info_lock);

	return ret;
}

static inline bool valid_cq_execution_subsample(
	struct state_accessor *s_acc, struct transition_param *p)
{
	bool ret = false;

	if (unlikely(!p->s_params))
		return ret;

	spin_lock(p->info_lock);
	ret = (p->info->sof_ts_ns <= p->info->sof_l_ts_ns) &&
		((p->event_ts - p->info->sof_ts_ns) < p->cq_trigger_thres) &&
		frame_seq_diff(s_acc->seq_no, p->info->inner_seq_no) == 1;
	if (ret == false)
		pr_info("[mtk-cam:vld_cq_exesubsample:%d/%d] event/l_sof/cq:%llu/%llu/%llu sof:%llu(%llu)",
			s_acc->seq_no, p->info->inner_seq_no,
			p->event_ts, p->info->sof_l_ts_ns, p->cq_trigger_thres, p->info->sof_ts_ns,
			ktime_get_boottime_ns());
	spin_unlock(p->info_lock);

	return ret;
}

static inline bool valid_cq_execution(struct transition_param *p)
{
	bool ret = false;

	if (unlikely(!p->s_params))
		return ret;
	/* check if ack between camsv/raw and mraw sof */
	/* for sentest/dual stream: large NE -> SE duration over 25ms case*/
	spin_lock(p->info_lock);
	ret = (p->info->sof_ts_ns <= p->info->sof_l_ts_ns) &&
	(((p->event_ts - p->info->sof_ts_ns) < p->cq_trigger_thres) ||
	((p->event_ts - p->info->sof_l_ts_ns) < SQC_THRES_FROM_L_SOF_NS));

	if (ret == false)
		pr_info("[mtk-cam:valid_cq_execution] event/l_sof/cq:%llu/%llu/%llu sof:%llu(%llu)",
			p->event_ts, p->info->sof_l_ts_ns, p->cq_trigger_thres, p->info->sof_ts_ns,
			ktime_get_boottime_ns());
	spin_unlock(p->info_lock);

	return ret;
}

#define SCQ_THRES_FOR_AEWA 27000000

static inline bool valid_cq_execution_ref_sof(struct transition_param *p)
{
	bool ret = false;

	if (unlikely(!p->s_params))
		return false;

	if (!p->reference_sof_ns)
		return false;
	/* check if ack between camsv/raw and mraw sof */
	/* for sentest NE -> SE duration 25ms case*/
	spin_lock(p->info_lock);
	ret = (p->info->sof_ts_ns <= p->info->sof_l_ts_ns) &&
		(((p->event_ts < (p->cq_trigger_thres + p->reference_sof_ns))) ||
	((p->event_ts - p->info->sof_l_ts_ns) < SQC_THRES_FROM_L_SOF_NS));
	if (ret == false)
		pr_info("[mtk-cam:valid_cq_execution] event/l_sof/cq:%llu/%llu/%llu sof:%llu(%llu) ref:%llu",
			p->event_ts, p->info->sof_l_ts_ns, p->cq_trigger_thres, p->info->sof_ts_ns,
			ktime_get_boottime_ns(), p->reference_sof_ns);
	spin_unlock(p->info_lock);

	return ret;
}

static inline bool valid_cq_execution_avoid_race_with_topirq(
	struct state_accessor *s_acc, struct transition_param *p)
{
	bool ret = false;

	if (unlikely(!p->s_params))
		return ret;
	if (p->info->ae_wa_enable == 0)
		return true;
	spin_lock(p->info_lock);
	ret = (p->event_ts - p->info->sof_l_ts_ns) > SCQ_THRES_FOR_AEWA ? false : true;

	if (ret == false)
		pr_info("[mtk-cam:WA] race with top-half case, event/f_sof/l_sof:%llu/%llu/%llu (%llu)",
			p->event_ts, p->info->sof_ts_ns, p->info->sof_l_ts_ns,
			ktime_get_boottime_ns());
	spin_unlock(p->info_lock);
	return ret;
}
static inline bool valid_cq_execution_threaded_irq_race_with_topirq(
	struct state_accessor *s_acc, struct transition_param *p)
{
	bool ret = true;

	if (unlikely(!p->s_params))
		return false;
	if (p->info->ae_wa_enable == 0)
		return true;
	if (s_acc->s->bypass_by_aewa == 0)
		return true;
	/* for case that one engines lost sof signal case */
	spin_lock(p->info_lock);
	if ((p->info->sof_l_ts_ns - p->info->sof_ts_ns) > 30000000 &&
		(p->event_ts - p->info->sof_ts_ns) > p->cq_trigger_thres) {
		ret = false;
		pr_info("[mtk-cam:WA] lost sof case, event/f_sof/l_sof:%llu/%llu/%llu (%llu)",
			p->event_ts, p->info->sof_ts_ns, p->info->sof_l_ts_ns,
			ktime_get_boottime_ns());
	}
	spin_unlock(p->info_lock);

	return ret;
}
static inline int guard_apply_sensor_subsample_2(struct state_accessor *s_acc,
					       struct transition_param *p)
{
	/* TODO: add ts check */
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_sensor) &&
		((s_acc->s->s_params.subsample == 2) &&
		ops_call(s_acc, prev_allow_apply_isp) &&
		(p->event_ts - p->info->sof_ts_ns) < 4000000);
}

static inline int guard_apply_sensor_subsample(struct state_accessor *s_acc,
					       struct transition_param *p)
{
	/* TODO: add ts check */
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_sensor) &&
		(ops_call(s_acc, cur_isp_state) >= S_ISP_APPLYING ||
		allow_subsample_2_i2c_by_ts(s_acc, p) ||
		allow_subsample_4_i2c_by_inner(s_acc, p));
}

static inline int guard_apply_sensor(struct state_accessor *s_acc,
				     struct transition_param *p)
{
	return allow_applying_hw(s_acc) &&
		(bypass_guard_check(p) ||
		 (ops_call(s_acc, prev_allow_apply_sensor) &&
		  valid_i2c_period(p)));
}

static inline int guard_apply_sensor_l(struct state_accessor *s_acc,
				     struct transition_param *p)
{
	return allow_applying_hw(s_acc) &&
		(bypass_guard_check(p) ||
		 (ops_call(s_acc, prev_allow_apply_sensor) &&
		  valid_i2c_period_l(p)));
}

static inline bool is_sensor_set(int sensor_state)
{
	return sensor_state >= S_SENSOR_LATCHED;
}
static inline bool is_sensor_ge_applied(int sensor_state)
{
	return sensor_state >= S_SENSOR_APPLIED;
}

static inline bool is_isp_ge_outer(int isp_state)
{
	return isp_state >= S_ISP_OUTER;
}

static inline bool is_isp_ge_processing(int isp_state)
{
	return isp_state >= S_ISP_PROCESSING;
}

static inline bool is_isp_aborted(int isp_state)
{
	return isp_state == S_ISP_ABORTED;
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
		current_sensor_ready(s_acc) &&
			valid_cq_execution(p) &&
			valid_cq_execution_threaded_irq_race_with_topirq(s_acc, p);
}
static inline int guard_apply_m2m(struct state_accessor *s_acc,
				  struct transition_param *p)
{
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_isp);
}
static inline int guard_apply_ts_m2m(struct state_accessor *s_acc,
				  struct transition_param *p)
{	/*TBD: time share raw atomic on_using */
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_extisp_procraw);
}

static inline int guard_apply_isp_subsample(struct state_accessor *s_acc,
				  struct transition_param *p)
{
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_isp) &&
		valid_cq_execution_subsample(s_acc, p);
}

static inline int guard_ack_apply_directly(struct state_accessor *s_acc,
					   struct transition_param *p)
{
	return guard_ack_eq(s_acc, p) && guard_apply_isp(s_acc, p) &&
			valid_cq_execution(p) &&
			valid_cq_execution_avoid_race_with_topirq(s_acc, p);
}

static inline int guard_ack_apply_directly_ref_sof(struct state_accessor *s_acc,
					   struct transition_param *p)
{
	return guard_ack_eq(s_acc, p) &&
		valid_cq_execution_ref_sof(p) &&
		allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_isp);
}

static inline int guard_ack_apply_directly_subsample(struct state_accessor *s_acc,
					   struct transition_param *p)
{
	return guard_ack_eq(s_acc, p) &&
		guard_apply_isp_subsample(s_acc, p);
}

static inline int guard_ack_apply_m2m_directly(struct state_accessor *s_acc,
					       struct transition_param *p)
{
	return guard_ack_eq(s_acc, p) && guard_apply_m2m(s_acc, p);
}

static inline int handle_frame_done_loss_subsample(struct state_accessor *s_acc,
					   struct transition_param *p)
{
	return guard_inner_greater(s_acc, p);
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
