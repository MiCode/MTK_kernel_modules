// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2022 MediaTek Inc.

#include "mtk_cam-debug_option.h"
#include "mtk_cam-job.h"
#include "mtk_cam-job_state.h"
#include "mtk_cam-job_state_impl.h"

static const char *value_to_str(const char * const str_arr[], size_t size,
				int value)
{
	const char *str;

	if (WARN_ON((unsigned int)value >= size))
		return "(not-found)";

	str = str_arr[value];
	return str ? str : "null";
}

const char *str_event(int event)
{
	static const char * const str[] = {
		[CAMSYS_EVENT_IRQ_F_VSYNC] = "f_vsync",
		[CAMSYS_EVENT_IRQ_L_SOF] = "l_sof",
		[CAMSYS_EVENT_IRQ_L_CQ_DONE] = "l_cq_done",
		[CAMSYS_EVENT_IRQ_FRAME_DONE] = "frame_done",

		[CAMSYS_EVENT_TIMER_SENSOR] = "timer_sensor",

		[CAMSYS_EVENT_ENQUE] = "enq",
		[CAMSYS_EVENT_ACK] = "ack",
		[CAMSYS_EVENT_IRQ_EXTMETA_SOF] = "extisp meta sof",
		[CAMSYS_EVENT_IRQ_EXTMETA_CQ_DONE] = "extisp cq done",

		[CAMSYS_EVENT_HW_HANG] = "hw hang",
	};

	return value_to_str(str, ARRAY_SIZE(str), event);
}

const char *str_sensor_state(int state)
{
	static const char * const str[] = {
		[S_SENSOR_NONE] = "none",
		[S_SENSOR_NOT_SET] = "not-set",
		[S_SENSOR_APPLYING] = "applying",
		[S_SENSOR_APPLIED] = "applied",
		[S_SENSOR_LATCHED] = "latched",
	};

	return value_to_str(str, ARRAY_SIZE(str), state);
}

const char *str_isp_state(int state)
{
	static const char * const str[] = {
		[S_ISP_NOT_SET] = "not-set",
		[S_ISP_COMPOSING] = "composing",
		[S_ISP_COMPOSED] = "composed",
		[S_ISP_APPLYING] = "applying",
		[S_ISP_OUTER] = "outer",
		[S_ISP_APPLYING_PROCRAW] = "applying proc raw",
		[S_ISP_OUTER_PROCRAW] = "outer proc raw",
		[S_ISP_PROCESSING] = "processing",
		[S_ISP_PROCESSING_PROCRAW] = "processing proc raw",
		[S_ISP_SENSOR_MISMATCHED] = "s-mismatched",
		[S_ISP_DONE] = "done",
		[S_ISP_DONE_MISMATCHED] = "done-mismatched",
		[S_ISP_ABORTED] = "aborted",
	};

	return value_to_str(str, ARRAY_SIZE(str), state);
}

const char *str_state(int state_type, int state)
{
	static const char * (*str_fn[NR_STATE_TYPE])(int) = {
		[SENSOR_1ST_STATE] = str_sensor_state,
		[ISP_1ST_STATE] = str_isp_state,
		[SENSOR_2ND_STATE] = str_sensor_state,
		[ISP_2ND_STATE] = str_isp_state,
	};

	if (WARN_ON((unsigned int)state_type >= ARRAY_SIZE(str_fn)))
		return "(not-found)";

	return str_fn[(unsigned int)state_type](state);
}

const char *str_state_type(int state_type)
{
	static const char *str[] = {
		[SENSOR_STATE] = "sensor_state",
		[ISP_STATE] = "isp_state",
		[SENSOR_2ND_STATE] = "sensor_2nd_state",
		[ISP_2ND_STATE] = "isp_2nd_state",
	};

	return value_to_str(str, ARRAY_SIZE(str), state_type);
}

static void _transit_state(struct mtk_cam_job_state *s, int state_type,
			   int old_state, int new_state, int act,
			   void *data)
{
	int prv_state;

	prv_state = mtk_cam_job_state_set(s, state_type, new_state);
	mtk_cam_job_state_set_action(s, act);

	if (s->cb && s->cb->on_transit)
		s->cb->on_transit(s, state_type,
				  old_state, new_state, act, data);

	/* this warning indicates racing condition is just happened */
	if (unlikely(prv_state != old_state))
		pr_info("%s: #%d %s:warn. expected old_state %s, but get %s\n",
			__func__, s->seq_no,
			str_state_type(state_type),
			str_state(state_type, old_state),
			str_state(state_type, prv_state));

	if (CAM_DEBUG_ENABLED(STATE))
		pr_info("%s: #%d %s: %s -> %s\n",
			__func__, s->seq_no,
			str_state_type(state_type),
			str_state(state_type, old_state),
			str_state(state_type, new_state));
}

static int update_entry_supported_events(struct transitions_entry *entry)
{
	struct state_transition *trans;
	int i;

	if (!entry->size)
		return 0;

	for (i = 0, trans = entry->trans; i < entry->size; ++trans, ++i)
		entry->cached_event_mask |= BIT(trans->on_event);

	return entry->cached_event_mask;
}

static int update_table_supported_events(struct state_table *tbl)
{
	struct transitions_entry *entry;
	int i;
	int cached_event;

	cached_event = 0;
	for (i = 0, entry = tbl->entries; i < tbl->size; ++entry, ++i)
		cached_event |= update_entry_supported_events(entry);

	tbl->cached_tbl_event_mask = cached_event;

	return cached_event;
}

static inline
bool is_entry_supported_event(struct transitions_entry *entry, int event)
{
	return entry->cached_event_mask & BIT(event);
}

static bool is_tbl_supported_event(struct state_table *tbl, int event)
{
	int event_bit = BIT(event);

	if (unlikely(!tbl->cached_tbl_event_mask))
		return update_table_supported_events(tbl) & event_bit;

	return tbl->cached_tbl_event_mask & event_bit;
}

int loop_each_transition(struct state_table *tbl,
			 struct state_accessor *s_acc, int state_type,
			 struct transition_param *p)
{
	struct transitions_entry *entry;
	const struct state_transition *trans;
	int trans_size;
	int state, event;
	int ret = 0;

	event = p->event;
	state = mtk_cam_job_state_get(s_acc->s, state_type);

	if (CAM_DEBUG_ENABLED(STATE) && 0)
		pr_info("%s: ...#%d %s [%s]\n",
			__func__,
			cur_seq_no(s_acc),
			str_state_type(state_type),
			str_state(state_type, state));

	/* check with supported event types for performance */
	if (state >= tbl->size || !is_tbl_supported_event(tbl, event))
		return 0;

	/* check with supported event types for performance */
	entry = &tbl->entries[state];
	if (!is_entry_supported_event(entry, event))
		return 0;

	trans = entry->trans;
	trans_size = entry->size;

	for (; trans_size; ++trans, --trans_size) {
		if (trans->on_event != event)
			continue;

		ret = trans->cam_guard ? trans->cam_guard(s_acc, p) : 1;
		if (ret > 0) {
			_transit_state(s_acc->s, state_type, state,
				       trans->dst_state, trans->action,
				       p->info);
			break;
		}
	}

	return ret;
}

