// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/string.h>

#include "c2ps_common.h"
#include "c2ps_monitor.h"
#include "c2ps_uclamp_regulator.h"


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
	if (!tsk_info) {
		C2PS_LOGE("tsk_info is null\n");
		return;
	}

	c2ps_info_lock(&tsk_info->mlock);
	memset(tsk_info->hist_proc_time, 0, sizeof(tsk_info->hist_proc_time));
	memset(tsk_info->hist_loading, 0, sizeof(tsk_info->hist_loading));
	c2ps_info_unlock(&tsk_info->mlock);
}

static void reset_task_eas_setting(struct c2ps_task_info *tsk_info)
{
	int ret = -1;
	int pid = -1;
	struct task_struct *p;
	struct sched_attr attr = {};
	struct global_info *glb_info = get_glb_info();

	if (!tsk_info) {
		C2PS_LOGE("tsk_info is null\n");
		return;
	}

	pid = tsk_info->pid;
	if (pid < 0)
		return;
	set_curr_uclamp_hint(pid, 0);

	attr.sched_policy = SCHED_NORMAL;
	attr.sched_flags = SCHED_FLAG_KEEP_ALL   |
			   SCHED_FLAG_UTIL_CLAMP;

	attr.sched_util_min = 1;
	if (glb_info != NULL) {
		attr.sched_util_max = max(glb_info->max_uclamp[0],
			max(glb_info->max_uclamp[1], glb_info->max_uclamp[2]));
		attr.sched_util_max = clamp(attr.sched_util_max, 1U, 1024U);
	}

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	if (likely(p))
		get_task_struct(p);
	rcu_read_unlock();

	if (likely(p)) {
		ret = sched_setattr_nocheck(p, &attr);
		if (ret == 0) {
			C2PS_LOGD("reset uclamp(%d, %d) to %s successfully",
				attr.sched_util_min, attr.sched_util_max,
				p->comm);
		} else {
			C2PS_LOGD("reset uclamp(%d, %d) to %s failed",
				attr.sched_util_min, attr.sched_util_max,
				p->comm);
		}
		put_task_struct(p);
	}
	if (tsk_info->is_vip_task)
		unset_task_basic_vip(pid);
}

int monitor_task_start(int pid, int task_id)
{
	struct c2ps_task_info *tsk_info = c2ps_find_task_info_by_tskid(task_id);

	C2PS_LOGD("+ \n");
	if (!tsk_info) {
		C2PS_LOGW_ONCE("tsk_info not found\n");
		C2PS_LOGW("tsk_info not found\n");
		return -1;
	}

	if (tsk_info->is_vip_task)
		set_task_basic_vip(pid);

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

		if (req != NULL) {
			req->tsk_info = tsk_info;
			req->glb_info = get_glb_info();

			send_regulator_req(req);
		}
	}
	C2PS_LOGD("- \n");
	return 0;
}

int monitor_task_end(int pid, int task_id)
{
	struct c2ps_task_info *tsk_info = c2ps_find_task_info_by_tskid(task_id);

	C2PS_LOGD("+ \n");
	if (!tsk_info) {
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

	reset_task_eas_setting(tsk_info);
	C2PS_LOGD("- \n");
	return 0;
}

int monitor_vsync(u64 ts)
{
	C2PS_LOGD("+ \n");
	update_vsync_time(ts);
	C2PS_LOGD("- \n");
	return 0;
}

int monitor_camfps(int camfps)
{
	C2PS_LOGD("+ \n");
	update_camfps(camfps);
	C2PS_LOGD("- \n");
	return 0;
}

int monitor_task_scene_change(int task_id, int scene_mode)
{
	struct c2ps_task_info *tsk_info = c2ps_find_task_info_by_tskid(task_id);

	C2PS_LOGD("+ \n");
	if (!tsk_info) {
		C2PS_LOGW_ONCE("tsk_info not found\n");
		C2PS_LOGW("tsk_info not found\n");
		return -1;
	}

	reset_history_info(tsk_info);
	C2PS_LOGD("- \n");
	return 0;
}
