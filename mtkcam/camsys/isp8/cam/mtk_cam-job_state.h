/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_CAM_JOB_STATE_H
#define __MTK_CAM_JOB_STATE_H

/* FIXME(AY): refine naming */
const char *str_sensor_state(int state);
const char *str_isp_state(int state);
const char *str_state_type(int state_type);
const char *str_state(int state_type, int state);

static inline
int mtk_cam_job_state_get(struct mtk_cam_job_state *s,
			  int state_type)
{
	return atomic_read(&s->state[state_type]);
}

/**
 * Return: old state for checking
 */
static inline
int mtk_cam_job_state_set(struct mtk_cam_job_state *s,
			  int state_type, int new_state)
{
	return atomic_xchg(&s->state[state_type], new_state);
}

static inline
const char *mtk_cam_job_state_str(struct mtk_cam_job_state *s,
				  int state_type)
{
	return str_state(state_type, mtk_cam_job_state_get(s, state_type));
}

static inline
void mtk_cam_job_state_set_action(struct mtk_cam_job_state *s, int act)
{
	atomic_or(act, &s->todo_action);
}

static inline
bool mtk_cam_job_state_has_action(struct mtk_cam_job_state *s)
{
	return !!atomic_read(&s->todo_action);
}

static inline
int mtk_cam_job_state_fetch_and_clear_action(struct mtk_cam_job_state *s)
{
	return atomic_xchg(&s->todo_action, 0);
}

/*
 * state-machines
 */

int mtk_cam_job_state_init_basic(struct mtk_cam_job_state *s,
				 const struct mtk_cam_job_state_cb *cb,
				 int with_sensor_ctrl);

int mtk_cam_job_state_init_m2m(struct mtk_cam_job_state *s,
			       const struct mtk_cam_job_state_cb *cb);

int mtk_cam_job_state_init_subsample(struct mtk_cam_job_state *s,
				const struct mtk_cam_job_state_cb *cb,
				int with_sensor_ctrl);

int mtk_cam_job_state_init_mstream(struct mtk_cam_job_state *s,
				   const struct mtk_cam_job_state_cb *cb,
				   int with_sensor_ctrl);
int mtk_cam_job_state_init_extisp(struct mtk_cam_job_state *s,
				const struct mtk_cam_job_state_cb *cb,
				int with_sensor_ctrl);
int mtk_cam_job_state_init_ts(struct mtk_cam_job_state *s,
				 const struct mtk_cam_job_state_cb *cb,
				 int with_sensor_ctrl);


#endif //__MTK_CAM_JOB_STATE_H
