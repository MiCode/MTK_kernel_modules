// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2022 MediaTek Inc.

#include "mtk_cam-job_state_impl.h"

/*

  composed -> applying -> outer -> applying proc -> outer proc -> process proc -> done

  i. composed -> applying        : triggering pd and meta's cq
 ii. applying -> outer           : pd and meta's both cq done
iii. outer -> applying proc      : triggering proc's cq
 iv. applying proc -> outer proc : proc's cq done
  v. outer porc->process proc    : proc raw sof
 vi. process proc -> done        : proc raw done

*/
static inline bool tg_cnt_proc_check(struct state_accessor *s_acc,
					struct transition_param *p)
{
	return s_acc->s->tg_cnt == p->info->extisp_tg_cnt[EXTISP_DATA_PROCRAW];
}

static int guard_apply_extisp_procraw(struct state_accessor *s_acc,
					struct transition_param *p)
{
	return allow_applying_hw(s_acc) &&
		ops_call(s_acc, prev_allow_apply_extisp_procraw) &&
		current_sensor_ready(s_acc) &&
		tg_cnt_proc_check(s_acc, p);
}
static int guard_get_f_vsync_timestamp(struct state_accessor *s_acc,
					struct transition_param *p)
{
	if (s_acc->s->extisp_data_timestamp[EXTISP_DATA_PD] == 0) {
		s_acc->s->extisp_data_timestamp[EXTISP_DATA_PD] = p->info->sof_ts_ns;
		pr_info("%s:#%d %lld", __func__, s_acc->seq_no,
			s_acc->s->extisp_data_timestamp[EXTISP_DATA_PD]);
	}

	return false;
}
static int guard_get_extmeta_sof_timestamp(struct state_accessor *s_acc,
					struct transition_param *p)
{
	if (s_acc->s->extisp_data_timestamp[EXTISP_DATA_META] == 0) {
		s_acc->s->extisp_data_timestamp[EXTISP_DATA_META] = p->info->sof_ts_ns;
		pr_info("%s:#%d %lld", __func__, s_acc->seq_no,
			s_acc->s->extisp_data_timestamp[EXTISP_DATA_META]);
	}

	return false;
}
static int guard_get_l_sof_timestamp_inner_eq(struct state_accessor *s_acc,
				 struct transition_param *p)
{
	if (s_acc->s->extisp_data_timestamp[EXTISP_DATA_PROCRAW] == 0) {
		s_acc->s->extisp_data_timestamp[EXTISP_DATA_PROCRAW] = p->info->sof_l_ts_ns;
		pr_info("%s:#%d %lld", __func__, s_acc->seq_no,
			s_acc->s->extisp_data_timestamp[EXTISP_DATA_PROCRAW]);
	}

	return p->info->inner_seq_no == cur_seq_no(s_acc);
}


static struct state_transition STATE_TRANS(extisp_sensor, S_SENSOR_NOT_SET)[] = {
	{
		S_SENSOR_APPLYING, CAMSYS_EVENT_ENQUE,
		guard_apply_sensor, ACTION_APPLY_SENSOR
	},
	{
		S_SENSOR_APPLYING, CAMSYS_EVENT_IRQ_F_VSYNC,
		guard_apply_sensor, ACTION_APPLY_SENSOR
	},
};

static struct state_transition STATE_TRANS(extisp_sensor, S_SENSOR_APPLIED)[] = {
	{
		S_SENSOR_LATCHED, CAMSYS_EVENT_IRQ_F_VSYNC,
		NULL, 0
	},
};
static struct state_transition STATE_TRANS(extisp, S_ISP_NOT_SET)[] = {
	{
		S_ISP_COMPOSING, CAMSYS_EVENT_ENQUE,
		guard_next_compose, ACTION_COMPOSE_CQ
	},
	{
		S_ISP_COMPOSING, CAMSYS_EVENT_ACK,
		guard_next_compose, ACTION_COMPOSE_CQ
	},
};

static struct state_transition STATE_TRANS(extisp, S_ISP_COMPOSING)[] = {
	{
		S_ISP_APPLYING, CAMSYS_EVENT_ACK,
		guard_ack_apply_directly, ACTION_APPLY_ISP_EXTMETA_PD_EXTISP
	},
	{
		S_ISP_COMPOSED, CAMSYS_EVENT_ACK,
		guard_ack_eq, 0
	},
};

static struct state_transition STATE_TRANS(extisp, S_ISP_COMPOSED)[] = {
	{
		S_ISP_APPLYING, CAMSYS_EVENT_IRQ_EXTMETA_SOF,
		guard_apply_isp, ACTION_APPLY_ISP_EXTMETA_PD_EXTISP,
	},
};

static struct state_transition STATE_TRANS(extisp, S_ISP_APPLYING)[] = {
	{
		S_ISP_OUTER, CAMSYS_EVENT_IRQ_EXTMETA_CQ_DONE,
		guard_outer_eq, 0
	},
};

static struct state_transition STATE_TRANS(extisp, S_ISP_OUTER)[] = {
	{
		S_ISP_APPLYING_PROCRAW, CAMSYS_EVENT_IRQ_L_SOF,
		guard_apply_extisp_procraw, ACTION_APPLY_ISP_PROCRAW_EXTISP
	},
	/* only handle timestamp */
	{
		S_ISP_OUTER, CAMSYS_EVENT_IRQ_F_VSYNC,
		guard_get_f_vsync_timestamp, 0
	},
	/* only handle timestamp */
	{
		S_ISP_OUTER, CAMSYS_EVENT_IRQ_EXTMETA_SOF,
		guard_get_extmeta_sof_timestamp, 0
	},
};

static struct state_transition STATE_TRANS(extisp, S_ISP_APPLYING_PROCRAW)[] = {
	{
		S_ISP_OUTER_PROCRAW, CAMSYS_EVENT_IRQ_L_CQ_DONE,
		guard_outer_eq, 0
	},
	/* only handle timestamp */
	{
		S_ISP_APPLYING_PROCRAW, CAMSYS_EVENT_IRQ_F_VSYNC,
		guard_get_f_vsync_timestamp, 0
	},
	/* only handle timestamp */
	{
		S_ISP_APPLYING_PROCRAW, CAMSYS_EVENT_IRQ_EXTMETA_SOF,
		guard_get_extmeta_sof_timestamp, 0
	},
};

static struct state_transition STATE_TRANS(extisp, S_ISP_OUTER_PROCRAW)[] = {
	{
		S_ISP_PROCESSING_PROCRAW, CAMSYS_EVENT_IRQ_L_SOF,
		guard_get_l_sof_timestamp_inner_eq, 0
	},
	/* only handle timestamp */
	{
		S_ISP_OUTER_PROCRAW, CAMSYS_EVENT_IRQ_F_VSYNC,
		guard_get_f_vsync_timestamp, 0
	},
	/* only handle timestamp */
	{
		S_ISP_OUTER_PROCRAW, CAMSYS_EVENT_IRQ_EXTMETA_SOF,
		guard_get_extmeta_sof_timestamp, 0
	},
};

static struct state_transition STATE_TRANS(extisp, S_ISP_PROCESSING_PROCRAW)[] = {
	{
		S_ISP_DONE, CAMSYS_EVENT_IRQ_FRAME_DONE,
		guard_inner_eq, 0
	},
	{
		S_ISP_PROCESSING_PROCRAW, CAMSYS_EVENT_IRQ_EXTMETA_FRAME_DONE,
		guard_inner_eq, ACTION_BUFFER_EXTMETA_PD_DONE
	},
	/* only handle timestamp */
	{
		S_ISP_PROCESSING_PROCRAW, CAMSYS_EVENT_IRQ_F_VSYNC,
		guard_get_f_vsync_timestamp, 0
	},
	/* only handle timestamp */
	{
		S_ISP_PROCESSING_PROCRAW, CAMSYS_EVENT_IRQ_EXTMETA_SOF,
		guard_get_extmeta_sof_timestamp, 0
	},
};

static struct transitions_entry extisp_sensor_entries[NR_S_SENSOR_STATE] = {
	ADD_TRANS_ENTRY(extisp_sensor, S_SENSOR_NOT_SET),
	ADD_TRANS_ENTRY(extisp_sensor, S_SENSOR_APPLIED),
};
struct state_table extisp_sensor_tbl = {
	.entries = extisp_sensor_entries,
	.size = ARRAY_SIZE(extisp_sensor_entries),
};

static struct transitions_entry extisp_isp_entries[NR_S_ISP_STATE] = {
	ADD_TRANS_ENTRY(extisp, S_ISP_NOT_SET),
	ADD_TRANS_ENTRY(extisp, S_ISP_COMPOSING),
	ADD_TRANS_ENTRY(extisp, S_ISP_COMPOSED),
	ADD_TRANS_ENTRY(extisp, S_ISP_APPLYING),
	ADD_TRANS_ENTRY(extisp, S_ISP_OUTER),
	ADD_TRANS_ENTRY(extisp, S_ISP_APPLYING_PROCRAW),
	ADD_TRANS_ENTRY(extisp, S_ISP_OUTER_PROCRAW),
	ADD_TRANS_ENTRY(extisp, S_ISP_PROCESSING_PROCRAW),
};
struct state_table extisp_isp_tbl = {
	.entries = extisp_isp_entries,
	.size = ARRAY_SIZE(extisp_isp_entries),
};
static bool is_isp_ge_processingprocraw(int isp_state)
{
	return isp_state >= S_ISP_PROCESSING_PROCRAW;
}

static bool _prev_allow_apply_extisp_procraw(struct state_accessor *s_acc)
{
	struct mtk_cam_job_state *prev_s = prev_state(s_acc->s, s_acc->head);

	return !prev_s || is_isp_ge_processingprocraw(mtk_cam_job_state_get(prev_s, ISP_STATE));
}


static const struct state_accessor_ops extisp_acc_ops = {
	.prev_allow_apply_sensor = sf_prev_allow_apply_sensor,
	.prev_allow_apply_isp = sf_prev_allow_apply_isp,
	.prev_allow_apply_extisp_procraw = _prev_allow_apply_extisp_procraw,
	.is_next_sensor_applied = sf_is_next_sensor_applied,
	.cur_sensor_state = sf_cur_sensor_state,
	.cur_isp_state = sf_cur_isp_state,
};

static int extisp_send_event(struct mtk_cam_job_state *s,
			    struct transition_param *p)
{
	struct state_accessor s_acc;
	int ret;

	s_acc.head = p->head;
	s_acc.s = s;
	s_acc.seq_no = s->seq_no;
	s_acc.ops = &extisp_acc_ops;
	p->s_params = &s->s_params;
	p->cq_trigger_thres = s->cq_trigger_thres_ns;

	ret = loop_each_transition(&extisp_sensor_tbl, &s_acc, SENSOR_STATE, p);

	/* note: beware of '!ret' here
	 * for current scenarios, we won't update sensor & isp state at same event
	 * use '!ret' to skip isp transition if sensor already did.
	 */
	if (!ret)
		loop_each_transition(&extisp_isp_tbl, &s_acc, ISP_STATE, p);

	return 0;
}

static int _is_next_sensor_applicable(struct mtk_cam_job_state *s)
{
	return is_isp_ge_outer(mtk_cam_job_state_get(s, ISP_STATE));
}

static int _is_next_extisp_metapd_applicable(struct mtk_cam_job_state *s)
{
	return is_isp_ge_outer(mtk_cam_job_state_get(s, ISP_STATE));
}

static int _is_sensor_set(struct mtk_cam_job_state *s)
{
	return is_sensor_set(mtk_cam_job_state_get(s, SENSOR_STATE));
}

static const struct mtk_cam_job_state_ops extisp_state_ops = {
	.send_event = extisp_send_event,
	.is_next_sensor_applicable = _is_next_sensor_applicable,
	.is_next_isp_applicable = _is_next_extisp_metapd_applicable,
	.is_sensor_applied = _is_sensor_set,
};

int mtk_cam_job_state_init_extisp(struct mtk_cam_job_state *s,
				 const struct mtk_cam_job_state_cb *cb,
				 int with_sensor_ctrl)
{
	s->ops = &extisp_state_ops;

	mtk_cam_job_state_set(s, SENSOR_STATE, S_SENSOR_NOT_SET);
	mtk_cam_job_state_set(s, ISP_STATE, S_ISP_NOT_SET);

	s->cb = cb;
	s->apply_by_fsm = 1;
	s->compose_by_fsm = 1;

	return 0;
}

