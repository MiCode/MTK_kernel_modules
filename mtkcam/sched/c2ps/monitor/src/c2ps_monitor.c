// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/string.h>

#include "c2ps_common.h"
#include "c2ps_monitor.h"
#include "c2ps_regulator.h"
#include "c2ps_stat_selector.h"


static bool check_if_update_uclamp(struct c2ps_task_info *tsk_info)
{
	if (tsk_info->tsk_group) {
		u64 remaining_time = tsk_info->tsk_group->group_target_time -
			tsk_info->tsk_group->accumulate_time;
		C2PS_LOGD("task_id: %d, group_head: %d"
			"accumulate_time: %lld, remaining_time: %lld",
			tsk_info->task_id, tsk_info->tsk_group->group_head,
			tsk_info->tsk_group->accumulate_time,
			remaining_time);

		if (tsk_info->task_target_time > remaining_time)
			C2PS_LOGD("Boost is not needed\n");
		else
			C2PS_LOGD("Need boost\n");
	}
	return true;
}

static inline u64 cal_real_exec_runtime(struct c2ps_task_info *tsk_info)
{
	return c2ps_get_sum_exec_runtime(tsk_info->pid) -
		tsk_info->sum_exec_runtime_start;
}

static void reset_history_info(struct c2ps_task_info *tsk_info)
{
	if (unlikely(!tsk_info)) {
		C2PS_LOGE("tsk_info is null\n");
		return;
	}

	c2ps_info_lock(&tsk_info->mlock);
	memset(tsk_info->hist_proc_time, 0, sizeof(tsk_info->hist_proc_time));
	memset(tsk_info->hist_loading, 0, sizeof(tsk_info->hist_loading));
	c2ps_info_unlock(&tsk_info->mlock);
}

int monitor_task_start(int pid, int task_id)
{
	struct c2ps_task_info *tsk_info = c2ps_find_task_info_by_tskid(task_id);

	C2PS_LOGD("+\n");
	if (unlikely(!tsk_info)) {
		C2PS_LOGW_ONCE("tsk_info not found\n");
		C2PS_LOGW("tsk_info not found\n");
		return -1;
	}

	if (tsk_info->is_vip_task) {
		if (!is_task_vip(pid)) {
			C2PS_LOGD("set VIP by monitor: %d", pid);
			set_task_basic_vip(pid);
			tsk_info->vip_set_by_monitor = true;
		}
		if (unlikely(tsk_info->is_enable_dep_thread)) {
			int _i = 0;

			for (; _i < MAX_DEP_THREAD_NUM; _i++) {
				if (tsk_info->dep_thread[_i] <= 0)
					break;
				set_task_basic_vip(tsk_info->dep_thread[_i]);
			}
		}
	}

	tsk_info->pid = pid;
	tsk_info->start_time = c2ps_get_time();
	tsk_info->sum_exec_runtime_start = c2ps_get_sum_exec_runtime(pid);

	C2PS_LOGD("task_id: %d, start_time: %llu\n", task_id, tsk_info->start_time);

	if (tsk_info->tsk_group) {
		c2ps_info_lock(&tsk_info->tsk_group->mlock);
		if (is_group_head(tsk_info)) {
			tsk_info->tsk_group->accumulate_time = 0;
			tsk_info->tsk_group->group_start_time = tsk_info->start_time;
		} else {
			tsk_info->tsk_group->accumulate_time =
				tsk_info->start_time - tsk_info->tsk_group->group_start_time;
		}
		c2ps_info_unlock(&tsk_info->tsk_group->mlock);
	}

	if (check_if_update_uclamp(tsk_info)) {
		struct regulator_req *req = get_regulator_req();

		if (likely(req != NULL)) {
			req->tsk_info = tsk_info;
			req->glb_info = get_glb_info();
			req->curr_um = 100;

			if (likely(req->glb_info)) {
				if (req->glb_info->has_anchor_spec && req->glb_info->curr_um > 0)
					req->curr_um = req->glb_info->curr_um;
				else if (req->glb_info->curr_um_idle > 0)
					req->curr_um = req->glb_info->curr_um_idle;
			}
			send_regulator_req(req);
		}
	}
	C2PS_LOGD("-\n");
	return 0;
}

int monitor_task_end(int pid, int task_id)
{
	struct c2ps_task_info *tsk_info = c2ps_find_task_info_by_tskid(task_id);

	C2PS_LOGD("+\n");
	if (unlikely(!tsk_info)) {
		C2PS_LOGW_ONCE("tsk_info not found\n");
		C2PS_LOGW("tsk_info not found\n");
		return -1;
	}

	tsk_info->end_time = c2ps_get_time();
	tsk_info->proc_time = tsk_info->end_time - tsk_info->start_time;
	tsk_info->real_exec_runtime = cal_real_exec_runtime(tsk_info);
	tsk_info->overlap_task = NULL;

	C2PS_LOGD("task_name: %s, task_id: %d, "
		"end_time: %llu, proc_time: %llu, exec_time: %llu\n",
		tsk_info->task_name, task_id, tsk_info->end_time, tsk_info->proc_time,
		tsk_info->real_exec_runtime);
	c2ps_update_task_info_hist(tsk_info);

	/* debug tool tag */
	c2ps_main_systrace("task_name: %s, task_id: %d, proc_time: %llu, "
			"real exec_time: %llu",
			tsk_info->task_name, tsk_info->task_id,
			tsk_info->proc_time, tsk_info->real_exec_runtime);
	c2ps_critical_task_systrace(tsk_info);

	if (unlikely(tsk_info->is_enable_dep_thread)) {
		if (unlikely(tsk_info->dep_thread_search_count
					< MAX_DEP_THREAD_SEARCH_COUNT)) {
			tsk_info->dep_thread_search_count++;
			c2ps_add_waker_pid_to_task_info(tsk_info);
		}
	}

	reset_task_eas_setting(tsk_info);
	C2PS_LOGD("-\n");
	return 0;
}

inline void cal_um(struct c2ps_anchor *anc)
{
	struct global_info *g_info = get_glb_info();
	struct regulator_req *req = NULL;

	if (unlikely(anc == NULL || g_info == NULL))
		return;

	req = get_regulator_req();

	if (likely(req != NULL)) {
		req->anc_info = anc;
		req->glb_info = g_info;
		req->stat = g_info->stat;
		send_regulator_req(req);
	}
}

int monitor_anchor(
	int anc_id, u32 order, bool register_fixed, u32 anc_type, u32 anc_order,
	u64 cur_ts, u32 latency_spec, u32 jitter_spec)
{
	struct c2ps_anchor *anc_info = c2ps_find_anchor_by_id(anc_id);
	struct global_info *g_info = get_glb_info();

	C2PS_LOGD("check anchor notifier, anc_id: %d, anc_type: %d", anc_id, anc_type);

	if (unlikely(!g_info))
		return -1;

	if (unlikely((g_info->overwrite_util_margin > 0 ||
			g_info->decided_um_placeholder_val > 0))) {
		C2PS_LOGD("skip anchor monitor, use overwrite um: %u or um_placeholder: %u",
				g_info->overwrite_util_margin, g_info->decided_um_placeholder_val);
		return 0;
	}

	if (unlikely(anc_info == NULL)) {
		C2PS_LOGD("[C2PS_CB] add anchor: %d\n", anc_id);
		anc_info = kzalloc(sizeof(*anc_info), GFP_KERNEL);

		if (unlikely(!anc_info)) {
			C2PS_LOGE("OOM\n");
			return -EINVAL;
		}

		hash_init(anc_info->um_table_stbl);
		anc_info->anchor_id = anc_id;
		anc_info->latency_spec = latency_spec;
		anc_info->jitter_spec = jitter_spec * jitter_spec / 1000;

		if (unlikely(c2ps_add_anchor(anc_info))) {
			C2PS_LOGE("add anchor failed\n");
			return -EINVAL;
		}
	}

	if (unlikely(g_info->switch_um_idle_rate_mode)) {
		g_info->has_anchor_spec = false;
		return 0;
	}

	g_info->has_anchor_spec = true;

	switch (anc_type) {
	// anchor start
	case 1:
		if (likely(order > 0))
			anc_info->s_idx = order % ANCHOR_QUEUE_LEN;
		else
			break;

		anc_info->s_hist_t[anc_info->s_idx] = cur_ts;
		C2PS_LOGD("debug: check anchor %d start timing: %llu",
			anc_id, anc_info->s_hist_t[anc_info->s_idx]);
		break;
	// anchor end
	case 2:
		if (likely(order > 0))
			anc_info->e_idx = order % ANCHOR_QUEUE_LEN;
		else
			break;

		C2PS_LOGD("check anc_id:%d, e_idx:%d, order:%d", anc_id, anc_info->e_idx, order);
		anc_info->e_hist_t[anc_info->e_idx] = cur_ts;

		anc_info->latest_duration = anc_info->e_hist_t[anc_info->e_idx] -
			   anc_info->s_hist_t[anc_info->e_idx];

		C2PS_LOGD("debug: check anchor %d end timing: %llu corr start timing: %llu last end timing:: %llu",
			anc_id, anc_info->e_hist_t[anc_info->e_idx],
			anc_info->s_hist_t[anc_info->e_idx],
			anc_info->e_hist_t[anc_info->e_idx-1 >= 0?
				anc_info->e_idx-1:anc_info->e_idx+ANCHOR_QUEUE_LEN-1]);

		c2ps_update_um_table(anc_info);
		cal_um(anc_info);
		break;
	default:
		break;
	}
	C2PS_LOGD("-\n");
	return 0;
}

int monitor_vsync(u64 ts)
{
	C2PS_LOGD("+\n");
	update_vsync_time(ts);
	C2PS_LOGD("-\n");
	return 0;
}

int monitor_camfps(int camfps)
{
	C2PS_LOGD("+\n");
	update_camfps(camfps);
	C2PS_LOGD("-\n");
	return 0;
}

void monitor_system_info(void)
{
	struct global_info *g_info = get_glb_info();

	// Only timer callback will call this function, shouldn't lock in the following
	// functions
	update_cpu_idle_rate();

	if (likely(g_info)) {
		g_info->stat = g_info->is_cpu_boost ? C2PS_STAT_TRANSIENT
							: determine_cur_system_state(g_info);
	}
}

int monitor_task_scene_change(int task_id, int scene_mode)
{
	struct c2ps_task_info *tsk_info = c2ps_find_task_info_by_tskid(task_id);

	C2PS_LOGD("+\n");
	if (unlikely(!tsk_info)) {
		C2PS_LOGW_ONCE("tsk_info not found\n");
		C2PS_LOGW("tsk_info not found\n");
		return -1;
	}

	reset_history_info(tsk_info);
	C2PS_LOGD("-\n");
	return 0;
}
