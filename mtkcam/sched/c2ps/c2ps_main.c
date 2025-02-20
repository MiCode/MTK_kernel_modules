// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/atomic.h>

#include "c2ps_common.h"
#include "c2ps_monitor.h"
#include "c2ps_regulator.h"
#include "c2ps_usedext.h"
#include "c2ps_sysfs.h"

enum C2PS_NOTIFIER_PUSH_TYPE {
	C2PS_NOTIFIER_UNINIT            = 0x00,
	C2PS_NOTIFIER_TASK_SINGLE_SHOT  = 0x01,
	C2PS_NOTIFIER_ANCHOR            = 0x02,
};

struct C2PS_NOTIFIER_PUSH_TAG {
	enum C2PS_NOTIFIER_PUSH_TYPE ePushType;
	u32 order;
	int pid;
	int task_id;
	u64 cur_ts;
	int overwrite_uclamp_max[MAX_NUMBER_OF_CLUSTERS];
	int idle_rate_alert;
	int vip_prior;
	u32 vip_throttle_time;
	int uclamp_max_placeholder1[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_placeholder2[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_placeholder3[MAX_NUMBER_OF_CLUSTERS];
	u32 util_margin;
	u32 um_placeholder1;
	u32 um_placeholder2;
	u32 um_placeholder3;
	bool reset_param;
	bool set_task_idle_prefer;
	bool enable_ineff_cpufreq;
	bool switch_um_idle_rate_mode;
	int critical_task_ids[MAX_CRITICAL_TASKS];
	int critical_task_uclamp[MAX_CRITICAL_TASKS];
	int reserved_1;
	int reserved_2;
	int reserved_3;
	bool anc_register_fixed;
	int anc_type;
	u32 anc_order;
	u32 latency_spec;
	u32 jitter_spec;
	struct list_head queue_list;
};

static struct kobject *main_base_kobj;
static struct task_struct *c2ps_tsk;
static LIST_HEAD(head);
static int condition_notifier_wq;
static bool condition_notifier_exit;
static DEFINE_MUTEX(notifier_wq_lock);
static DECLARE_WAIT_QUEUE_HEAD(notifier_wq_queue);
static void self_uninit_timer_callback(struct timer_list *t);
static int picked_wl_table = 0;
static unsigned int background_monitor_duration = BACKGROUND_MONITOR_DURATION;
static unsigned int c2ps_vip_throttle_time = 12;
static atomic_t processing_count = ATOMIC_INIT(0);
unsigned int c2ps_nr_clusters;

struct timer_list background_info_update_timer;
struct timer_list self_uninit_timer;

module_param(picked_wl_table, int, 0644);
module_param(background_monitor_duration, int, 0644);
module_param(c2ps_vip_throttle_time, int, 0644);

static void background_info_update_timer_callback(struct timer_list *t)
{
	if (unlikely(background_monitor_duration == 0))
		background_monitor_duration = BACKGROUND_MONITOR_DURATION;
	mod_timer(t, jiffies + background_monitor_duration*HZ / 1000 / 2);
	monitor_system_info();
}

static void trigger_bg_policy(void)
{
	if (need_update_background()) {
		struct regulator_req *req = NULL;
		struct global_info *g_info = get_glb_info();

		if (unlikely(!g_info))
			return;

		req = get_regulator_req();

		if (likely(req != NULL)) {
			req->glb_info = g_info;
			req->stat = g_info->stat;
			send_regulator_req(req);
			reset_need_update_status();
		}
	}
}

static void c2ps_notifier_init(int cfg_camfps)
{
	if (unlikely(init_c2ps_common(cfg_camfps))) {
		C2PS_LOGD("init_c2ps_common failed\n");
		return;
	}

	self_uninit_timer.expires = jiffies + 5*HZ;
	timer_setup(&self_uninit_timer, self_uninit_timer_callback, 0);
	add_timer(&self_uninit_timer);

	background_info_update_timer.expires = jiffies;
	timer_setup(&background_info_update_timer,
				background_info_update_timer_callback, 0);
	add_timer(&background_info_update_timer);
	if (picked_wl_table < get_nr_wl())
		set_wl_manual(picked_wl_table);
	else
		set_wl_manual(0);
	c2ps_regulator_init();
}

static void c2ps_notifier_uninit(void)
{
	short _idx = 0;

	C2PS_LOGD("[C2PS_CB] uninit\n");
	// disable sugov per-gear uclamp max feature
	set_gear_uclamp_ctrl(0);
	// disable sugov curr_uclamp feature
	set_curr_uclamp_ctrl(0);
	reset_eas_setting();
	c2ps_regulator_flush();
	del_timer_sync(&background_info_update_timer);
	exit_c2ps_common();
	del_timer_sync(&self_uninit_timer);
	set_wl_manual(-1);

	// reset util margin to default
	for (; _idx < c2ps_nr_clusters; ++_idx)
		c2ps_set_turn_point_freq(_idx, 0);
}

static void c2ps_notifier_add_task(
	u32 task_id, u32 task_target_time, u32 default_uclamp,
	int group_head, u32 task_group_target_time, bool is_vip_task,
	bool is_dynamic_tid, bool is_enable_dep_thread, const char *task_name)
{
	struct c2ps_task_info *tsk_info = NULL;
	C2PS_LOGD("[C2PS_CB] add_task task_id: %d task_name: %s\n",
		task_id, task_name);

	tsk_info = kzalloc(sizeof(*tsk_info), GFP_KERNEL);

	if (unlikely(!tsk_info)) {
		C2PS_LOGE("OOM\n");
		return;
	}

	tsk_info->task_id = task_id;
	tsk_info->task_target_time = task_target_time * NSEC_PER_MSEC;
	tsk_info->default_uclamp = default_uclamp;
	tsk_info->is_vip_task = is_vip_task;
	tsk_info->is_dynamic_tid = is_dynamic_tid;
	tsk_info->is_enable_dep_thread = is_enable_dep_thread;
	mutex_init(&tsk_info->mlock);
	strncpy(tsk_info->task_name, task_name, sizeof(tsk_info->task_name));

	if (unlikely(c2ps_add_task_info(tsk_info))) {
		C2PS_LOGE("add task failed\n");
		return;
	}

	// task group enabled
	if (group_head > 0) {
		// is task group head
		if (tsk_info->task_id == group_head) {
			if (unlikely(c2ps_create_task_group(
					group_head, task_group_target_time))) {
				C2PS_LOGE("create task group failed\n");
				return;
			}
		}
		if (unlikely(c2ps_add_task_to_group(tsk_info, group_head))) {
			C2PS_LOGE("add task to group failed\n");
			return;
		}
	}
}

static void c2ps_notifier_task_single_shot(
	int pid, int *uclamp_max, int idle_rate_alert, int vip_prior,
	u32 vip_throttle_time, int *uclamp_max_placeholder1,
	int *uclamp_max_placeholder2, int *uclamp_max_placeholder3,
	bool reset_param, bool set_task_idle_prefer,
	int *critical_task_ids, int *critical_task_uclamp, u32 util_margin,
	u32 um_placeholder1, u32 um_placeholder2, u32 um_placeholder3,
	bool enable_ineff_cpufreq, bool switch_um_idle_rate_mode,
	int reserved_1, int reserved_2, int reserved_3)
{
	struct global_info *g_info = get_glb_info();
	unsigned int _vip_throttle_time = vip_throttle_time > 0 ?
							vip_throttle_time : c2ps_vip_throttle_time;

	C2PS_LOGD("thread: %d, vip_prior: %d, throttle_time: %u, set_task_idle_prefer: %d",
			pid, vip_prior, _vip_throttle_time, set_task_idle_prefer);
	c2ps_set_vip_task(pid, vip_prior, _vip_throttle_time);
	if (set_task_idle_prefer) {
		if (reset_param)
			unset_task_ls(pid);
		else
			set_task_ls(pid);
	}

	if (unlikely(!g_info)) {
		C2PS_LOGD("glb_info is null\n");
		return;
	}

	if (need_update_single_shot_uclamp_max(uclamp_max))
		memcpy(g_info->overwrite_uclamp_max, uclamp_max,
			c2ps_nr_clusters * sizeof(int));
	if (unlikely(need_update_single_shot_uclamp_max(uclamp_max_placeholder1)))
		memcpy(g_info->uclamp_max_placeholder1, uclamp_max_placeholder1,
			c2ps_nr_clusters * sizeof(int));
	if (unlikely(need_update_single_shot_uclamp_max(uclamp_max_placeholder2)))
		memcpy(g_info->uclamp_max_placeholder2, uclamp_max_placeholder2,
			c2ps_nr_clusters * sizeof(int));
	if (unlikely(need_update_single_shot_uclamp_max(uclamp_max_placeholder3)))
		memcpy(g_info->uclamp_max_placeholder3, uclamp_max_placeholder3,
			c2ps_nr_clusters * sizeof(int));
	if (idle_rate_alert != -1)
		g_info->overwrite_idle_alert = idle_rate_alert;
	if (need_update_critical_task_uclamp(critical_task_uclamp))
		update_critical_task_uclamp_by_tsk_id(
			critical_task_ids, critical_task_uclamp);

	if (util_margin == RESET_VAL) {
		g_info->overwrite_util_margin = 0;
	} else if (util_margin > 0) {
		struct regulator_req *req = get_regulator_req();

		g_info->overwrite_util_margin = util_margin;
		if (likely(req != NULL)) {
			req->glb_info = g_info;
			req->stat = C2PS_STAT_TRANSIENT;
			send_regulator_req(req);
		}
	}

	if (enable_ineff_cpufreq) {
		short cluster_index = 0;

		if (!reset_param) {
			g_info->single_shot_enable_ineff_cpufreq_cnt++;
			for (; cluster_index < c2ps_nr_clusters; cluster_index++)
				c2ps_update_cpu_freq_ceiling(cluster_index, FREQ_QOS_MAX_DEFAULT_VALUE);
		} else {
			g_info->single_shot_enable_ineff_cpufreq_cnt--;
			if (!g_info->single_shot_enable_ineff_cpufreq_cnt) {
				for (; cluster_index < c2ps_nr_clusters; cluster_index++)
					c2ps_reset_cpu_freq_ceiling(cluster_index);
			}
		}
	}
	if (switch_um_idle_rate_mode)
		g_info->switch_um_idle_rate_mode = reset_param ? false : true;

	if (um_placeholder1)
		g_info->um_placeholder1 = um_placeholder1;
	if (um_placeholder2)
		g_info->um_placeholder2 = um_placeholder2;
	if (um_placeholder3)
		g_info->um_placeholder3 = um_placeholder3;
}

static void c2ps_queue_work(struct C2PS_NOTIFIER_PUSH_TAG *vpPush, bool update_timer)
{
	if (update_timer && likely(timer_pending(&self_uninit_timer)))
		mod_timer(&self_uninit_timer, jiffies + 5*HZ);
	mutex_lock(&notifier_wq_lock);
	list_add_tail(&vpPush->queue_list, &head);
	condition_notifier_wq = 1;
	mutex_unlock(&notifier_wq_lock);

	wake_up_interruptible(&notifier_wq_queue);
}

static int c2ps_uninit_wo_lock(void)
{
	struct C2PS_NOTIFIER_PUSH_TAG *vpPush = NULL;
	int ret = 0;

	C2PS_LOGD("+\n");

	vpPush = (struct C2PS_NOTIFIER_PUSH_TAG *)
		c2ps_alloc_atomic(sizeof(*vpPush));

	if (unlikely(!vpPush)) {
		C2PS_LOGE("OOM\n");
		ret = -ENOMEM;
		goto out;
	}

	if (unlikely(!c2ps_tsk)) {
		C2PS_LOGE("NULL WorkQueue\n");
		c2ps_free(vpPush, sizeof(*vpPush));
		ret = -EINVAL;
		goto out;
	}

	vpPush->ePushType = C2PS_NOTIFIER_UNINIT;
	list_add_tail(&vpPush->queue_list, &head);
	condition_notifier_wq = 1;

	wake_up_interruptible(&notifier_wq_queue);
out:
	return ret;
}

static void c2ps_notifier_wq_cb(void)
{
	struct C2PS_NOTIFIER_PUSH_TAG *vpPush;

	wait_event_interruptible(notifier_wq_queue,
		condition_notifier_wq || condition_notifier_exit);

	if (unlikely(condition_notifier_exit))
		return;

	mutex_lock(&notifier_wq_lock);

	if (unlikely(!list_empty(&head))) {
		vpPush = list_first_entry(&head,
			struct C2PS_NOTIFIER_PUSH_TAG, queue_list);

		if (vpPush->ePushType == C2PS_NOTIFIER_UNINIT &&
			atomic_read(&processing_count) > 0)  {
			mutex_unlock(&notifier_wq_lock);
			C2PS_LOGW("work still processing, shouldn't uninit now, use count: %d",
				atomic_read(&processing_count));
			return;
		}

		list_del(&vpPush->queue_list);
		if (list_empty(&head))
			condition_notifier_wq = 0;
		mutex_unlock(&notifier_wq_lock);
	} else {
		condition_notifier_wq = 0;
		mutex_unlock(&notifier_wq_lock);
		return;
	}

	switch (vpPush->ePushType) {
	case C2PS_NOTIFIER_UNINIT:
		c2ps_notifier_uninit();
		break;
	case C2PS_NOTIFIER_TASK_SINGLE_SHOT:
		c2ps_notifier_task_single_shot(
			vpPush->pid, vpPush->overwrite_uclamp_max, vpPush->idle_rate_alert,
			vpPush->vip_prior, vpPush->vip_throttle_time,
			vpPush->uclamp_max_placeholder1, vpPush->uclamp_max_placeholder2,
			vpPush->uclamp_max_placeholder3, vpPush->reset_param,
			vpPush->set_task_idle_prefer, vpPush->critical_task_ids,
			vpPush->critical_task_uclamp, vpPush->util_margin,
			vpPush->um_placeholder1, vpPush->um_placeholder2,
			vpPush->um_placeholder3, vpPush->enable_ineff_cpufreq,
			vpPush->switch_um_idle_rate_mode, vpPush->reserved_1,
			vpPush->reserved_2, vpPush->reserved_3);
		break;
	case C2PS_NOTIFIER_ANCHOR:
		monitor_anchor(vpPush->task_id, vpPush->order, vpPush->anc_register_fixed,
						vpPush->anc_type, vpPush->anc_order, vpPush->cur_ts,
						vpPush->latency_spec, vpPush->jitter_spec);
		break;
	default:
		C2PS_LOGE("unhandled push type = %d\n",
				vpPush->ePushType);
		break;
	}
	c2ps_free(vpPush, sizeof(*vpPush));
	trigger_bg_policy();
}

static int c2ps_thread_loop(void *arg)
{
	set_user_nice(current, -20);

	while (!kthread_should_stop())
		c2ps_notifier_wq_cb();

	return 0;
}

int c2ps_notify_init(
	int cfg_camfps, int max_uclamp_cluster0, int max_uclamp_cluster1,
	int max_uclamp_cluster2, int ineff_cpu_ceiling_freq0,
	int ineff_cpu_ceiling_freq1, int ineff_cpu_ceiling_freq2,
	int lcore_mcore_um_ratio, int um_floor)
{
	C2PS_LOGD(
		"config camfps (frames per 1000 seconds): %d, max_uclamp_cluster0: %d, max_uclamp_cluster1: %d, max_uclamp_cluster2: %d",
		cfg_camfps, max_uclamp_cluster0, max_uclamp_cluster1, max_uclamp_cluster2);
	C2PS_LOGD(
		"ineff_cpu_ceiling_freq0: %d, ineff_cpu_ceiling_freq1: %d, ineff_cpu_ceiling_freq2: %d",
		ineff_cpu_ceiling_freq0, ineff_cpu_ceiling_freq1, ineff_cpu_ceiling_freq2);
	C2PS_LOGD("lcore_mcore_um_ratio: %d, um_floor: %d",
		lcore_mcore_um_ratio, um_floor);

	// enable sugov per-gear uclamp max feature
	set_gear_uclamp_ctrl(1);
	set_gear_uclamp_max(0, max_uclamp_cluster0);
	set_gear_uclamp_max(1, max_uclamp_cluster1);
	set_gear_uclamp_max(2, max_uclamp_cluster2);

	// enable sugov curr_uclamp feature
	set_curr_uclamp_ctrl(1);
	set_eas_setting();
	atomic_set(&processing_count, 0);
	c2ps_notifier_init(cfg_camfps);

	// QoS setting
	c2ps_set_ineff_cpu_freq_ceiling(0, ineff_cpu_ceiling_freq0);
	c2ps_set_ineff_cpu_freq_ceiling(1, ineff_cpu_ceiling_freq1);
	c2ps_set_ineff_cpu_freq_ceiling(2, ineff_cpu_ceiling_freq2);

	cache_possible_config_cpu_freq_info();

	c2ps_lcore_mcore_um_ratio = lcore_mcore_um_ratio > 0 ?
						min(LMCORE_UM_RATIO_MAX, lcore_mcore_um_ratio) : 10;

	c2ps_regulator_um_min = um_floor > 0 ? um_floor : DEFAULT_UM_MIN;

	trigger_bg_policy();
	return 0;
}

int c2ps_notify_uninit(void)
{
	struct C2PS_NOTIFIER_PUSH_TAG *vpPush = NULL;
	int ret = 0;

	C2PS_LOGD("+\n");

	vpPush = (struct C2PS_NOTIFIER_PUSH_TAG *)
		c2ps_alloc_atomic(sizeof(*vpPush));

	if (unlikely(!vpPush)) {
		C2PS_LOGE("OOM\n");
		ret = -ENOMEM;
		goto out;
	}

	if (unlikely(!c2ps_tsk)) {
		C2PS_LOGE("NULL WorkQueue\n");
		c2ps_free(vpPush, sizeof(*vpPush));
		ret = -EINVAL;
		goto out;
	}

	vpPush->ePushType = C2PS_NOTIFIER_UNINIT;
	c2ps_queue_work(vpPush, true /* update uninit timer */);

out:
	return ret;
}

int c2ps_notify_add_task(
	u32 task_id, u32 task_target_time, u32 default_uclamp,
	int group_head, u32 task_group_target_time,
	bool is_vip_task, bool is_dynamic_tid,
	bool is_enable_dep_thread, const char *task_name)
{
	C2PS_LOGD("task_id: %d\n", task_id);
	if (likely(timer_pending(&self_uninit_timer)))
		mod_timer(&self_uninit_timer, jiffies + 5*HZ);
	c2ps_notifier_add_task(task_id, task_target_time, default_uclamp,
			group_head, task_group_target_time, is_vip_task, is_dynamic_tid,
			is_enable_dep_thread, task_name);
	trigger_bg_policy();
	return 0;
}

int c2ps_notify_task_start(int pid, int task_id)
{
	C2PS_LOGD("task_id: %d\n", task_id);

	atomic_inc(&processing_count);
	if (likely(timer_pending(&self_uninit_timer)))
		mod_timer(&self_uninit_timer, jiffies + 5*HZ);
	if (unlikely(monitor_task_start(pid, task_id))) {
		C2PS_LOGW_ONCE("monitor_task_start failed\n");
		C2PS_LOGW("monitor_task_start failed\n");
		atomic_dec(&processing_count);
		return -1;
	}
	trigger_bg_policy();
	atomic_dec(&processing_count);
	return 0;
}

int c2ps_notify_task_end(int pid, int task_id)
{
	C2PS_LOGD("task_id: %d\n", task_id);

	atomic_inc(&processing_count);
	if (unlikely(monitor_task_end(pid, task_id))) {
		C2PS_LOGW_ONCE("monitor_task_end failed\n");
		C2PS_LOGW("monitor_task_end failed\n");
		atomic_dec(&processing_count);
		return -1;
	}
	trigger_bg_policy();
	atomic_dec(&processing_count);
	return 0;
}

int c2ps_notify_task_scene_change(int task_id, int scene_mode)
{
	C2PS_LOGD("task_id: %d\n", task_id);
	if (unlikely(monitor_task_scene_change(task_id, scene_mode))) {
		C2PS_LOGE("monitor_task_scene_change failed\n");
		return -1;
	}
	return 0;
}

int c2ps_notify_vsync(void)
{
	C2PS_LOGD("+\n");
	if (unlikely(monitor_vsync(c2ps_get_time()))) {
		C2PS_LOGE("monitor_vsync failed\n");
		return -1;
	}
	return 0;
}

int c2ps_notify_camfps(int camfps)
{
	C2PS_LOGD("camfps: %d\n", camfps);
	if (unlikely(monitor_camfps(camfps))) {
		C2PS_LOGE("monitor_camfps failed\n");
		return -1;
	}
	return 0;
}

int c2ps_notify_single_shot_control(
	int pid, int *uclamp_max, int idle_rate_alert,
	int vip_prior, u32 vip_throttle_time, int *uclamp_max_placeholder1,
	int *uclamp_max_placeholder2, int *uclamp_max_placeholder3,
	bool reset_param, bool set_task_idle_prefer,
	int *critical_task_ids, int *critical_task_uclamp, u32 util_margin,
	u32 um_placeholder1, u32 um_placeholder2, u32 um_placeholder3,
	bool enable_ineff_cpufreq, bool switch_um_idle_rate_mode,
	int reserved_1, int reserved_2, int reserved_3)
{
	struct C2PS_NOTIFIER_PUSH_TAG *vpPush = NULL;
	int ret = 0;

	if (!uclamp_max || !uclamp_max_placeholder1 ||
		!uclamp_max_placeholder2 || !uclamp_max_placeholder3 ||
		!critical_task_ids || !critical_task_uclamp) {
		C2PS_LOGE("single shot control null pointer\n");
		ret = -EINVAL;
		goto out;
	}

	vpPush = (struct C2PS_NOTIFIER_PUSH_TAG *)
		c2ps_alloc_atomic(sizeof(*vpPush));

	if (unlikely(!vpPush)) {
		C2PS_LOGE("OOM\n");
		ret = -ENOMEM;
		goto out;
	}

	if (unlikely(!c2ps_tsk)) {
		C2PS_LOGE("NULL WorkQueue\n");
		c2ps_free(vpPush, sizeof(*vpPush));
		ret = -EINVAL;
		goto out;
	}
	memset(vpPush->overwrite_uclamp_max, 0,
		MAX_NUMBER_OF_CLUSTERS * sizeof(int));
	memcpy(vpPush->overwrite_uclamp_max, uclamp_max,
		MAX_NUMBER_OF_CLUSTERS * sizeof(int));
	memset(vpPush->uclamp_max_placeholder1, 0,
		MAX_NUMBER_OF_CLUSTERS * sizeof(int));
	memcpy(vpPush->uclamp_max_placeholder1, uclamp_max_placeholder1,
		MAX_NUMBER_OF_CLUSTERS * sizeof(int));
	memset(vpPush->uclamp_max_placeholder2, 0,
		MAX_NUMBER_OF_CLUSTERS * sizeof(int));
	memcpy(vpPush->uclamp_max_placeholder2, uclamp_max_placeholder2,
		MAX_NUMBER_OF_CLUSTERS * sizeof(int));
	memset(vpPush->uclamp_max_placeholder3, 0,
		MAX_NUMBER_OF_CLUSTERS * sizeof(int));
	memcpy(vpPush->uclamp_max_placeholder3, uclamp_max_placeholder3,
		MAX_NUMBER_OF_CLUSTERS * sizeof(int));
	memset(vpPush->critical_task_ids, 0,
		MAX_CRITICAL_TASKS * sizeof(int));
	memcpy(vpPush->critical_task_ids, critical_task_ids,
		MAX_CRITICAL_TASKS * sizeof(int));
	memset(vpPush->critical_task_uclamp, 0,
		MAX_CRITICAL_TASKS * sizeof(int));
	memcpy(vpPush->critical_task_uclamp, critical_task_uclamp,
		MAX_CRITICAL_TASKS * sizeof(int));
	vpPush->pid = pid;
	vpPush->idle_rate_alert = idle_rate_alert;
	vpPush->vip_prior = vip_prior;
	vpPush->vip_throttle_time = vip_throttle_time;
	vpPush->reset_param = reset_param;
	vpPush->set_task_idle_prefer = set_task_idle_prefer;
	vpPush->util_margin = util_margin;
	vpPush->reserved_1 = reserved_1;
	vpPush->reserved_2 = reserved_2;
	vpPush->reserved_3 = reserved_3;
	vpPush->um_placeholder1 = um_placeholder1;
	vpPush->um_placeholder2 = um_placeholder2;
	vpPush->um_placeholder3 = um_placeholder3;
	vpPush->enable_ineff_cpufreq = enable_ineff_cpufreq;
	vpPush->switch_um_idle_rate_mode = switch_um_idle_rate_mode;
	vpPush->ePushType = C2PS_NOTIFIER_TASK_SINGLE_SHOT;

	c2ps_queue_work(vpPush, false /* do not update uninit timer */);

out:
	return ret;
}

int c2ps_notify_single_shot_task_start(int pid, u32 uclamp)
{
	C2PS_LOGD("pid: %d, uclamp: %d", pid, uclamp);
	set_uclamp(pid, uclamp, uclamp);
	return 0;
}

int c2ps_notify_single_shot_task_end(int pid)
{
	C2PS_LOGD("pid: %d", pid);
	reset_task_uclamp(pid);
	return 0;
}

int c2ps_notify_anchor(
	int anc_id, bool register_fixed, u32 anc_type, u32 anc_order, u32 order,
	u32 latency_spec, u32 jitter_spec)
{
	struct C2PS_NOTIFIER_PUSH_TAG *vpPush = NULL;
	int ret = 0;

	C2PS_LOGD("anchor(%d) start(1)/end(2):%u\n", anc_id, anc_type);

	vpPush = (struct C2PS_NOTIFIER_PUSH_TAG *)
		c2ps_alloc_atomic(sizeof(*vpPush));

	if (unlikely(!vpPush)) {
		C2PS_LOGE("OOM\n");
		ret = -ENOMEM;
		goto out;
	}

	vpPush->ePushType = C2PS_NOTIFIER_ANCHOR;
	vpPush->order = order;
	// use task_id as anchor name
	vpPush->task_id = anc_id;
	vpPush->anc_register_fixed = register_fixed;
	vpPush->anc_type = anc_type;
	vpPush->anc_order = anc_order;
	vpPush->cur_ts = c2ps_get_time();
	vpPush->latency_spec = latency_spec;
	vpPush->jitter_spec = jitter_spec;
	c2ps_queue_work(vpPush, true /* update uninit timer */);

out:
	return ret;
}

static void self_uninit_timer_callback(struct timer_list *t)
{
	C2PS_LOGD("uninit expired");
	c2ps_uninit_wo_lock();
}

static ssize_t um_placeholder_store(struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	int val = -1;
	char *buffer = NULL;
	struct global_info *g_info = get_glb_info();

	buffer = kcalloc(C2PS_SYSFS_MAX_BUFF_SIZE, sizeof(char), GFP_KERNEL);
	if (unlikely(!buffer))
		goto out;

	if ((count > 0) && (count < C2PS_SYSFS_MAX_BUFF_SIZE)) {
		if (scnprintf(buffer, C2PS_SYSFS_MAX_BUFF_SIZE, "%s", buf)) {
			if (kstrtoint(buffer, 0, &val) != 0)
				goto out;
		}
	}

	if (unlikely(val < 0))
		goto out;

	if (likely(g_info)) {
		switch (val) {
		case 1:
			g_info->decided_um_placeholder_val = g_info->um_placeholder1;
			break;
		case 2:
			g_info->decided_um_placeholder_val = g_info->um_placeholder2;
			break;
		case 3:
			g_info->decided_um_placeholder_val = g_info->um_placeholder3;
			break;
		default:
			g_info->decided_um_placeholder_val = 0;
			goto out;
		}
		C2PS_LOGD("check decided_um: %u", g_info->decided_um_placeholder_val);

		if (likely(g_info->decided_um_placeholder_val)) {
			struct regulator_req *req = get_regulator_req();

			if (likely(req != NULL)) {
				req->glb_info = g_info;
				req->stat = C2PS_STAT_TRANSIENT;
				send_regulator_req(req);
			}
		}
	}

out:
	kfree(buffer);
	return count;
}

static ssize_t um_placeholder_show(struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	int length = 0;
	struct global_info *g_info = get_glb_info();

	if (unlikely(g_info == NULL))
		return 0;

	length += scnprintf(buf + length, PAGE_SIZE - length,
			"%d\n",
			g_info->decided_um_placeholder_val);

	return length;
}

static KOBJ_ATTR_RW(um_placeholder);

static int __init c2ps_init(void)
{
	C2PS_LOGD("+\n");

	c2ps_nr_clusters = get_nr_gears();
	c2ps_tsk = kthread_create(c2ps_thread_loop, NULL, "c2ps_thread_loop");

	if (unlikely(c2ps_tsk == NULL))
		return -EFAULT;
	wake_up_process(c2ps_tsk);

	c2ps_notify_init_fp = c2ps_notify_init;
	c2ps_notify_uninit_fp = c2ps_notify_uninit;
	c2ps_notify_add_task_fp = c2ps_notify_add_task;
	c2ps_notify_task_start_fp = c2ps_notify_task_start;
	c2ps_notify_task_end_fp = c2ps_notify_task_end;
	c2ps_notify_vsync_fp = c2ps_notify_vsync;
	c2ps_notify_camfps_fp = c2ps_notify_camfps;
	c2ps_notify_task_scene_change_fp = c2ps_notify_task_scene_change;
	c2ps_notify_single_shot_control_fp = c2ps_notify_single_shot_control;
	c2ps_notify_single_shot_task_start_fp = c2ps_notify_single_shot_task_start;
	c2ps_notify_single_shot_task_end_fp = c2ps_notify_single_shot_task_end;
	c2ps_notify_anchor_fp = c2ps_notify_anchor;

	c2ps_sysfs_init();
	if (unlikely(regulator_init())) {
		C2PS_LOGD("regulator_init failed\n");
		return -EFAULT;
	}

	if (!c2ps_sysfs_create_dir(NULL, "main", &main_base_kobj))
		c2ps_sysfs_create_file(main_base_kobj, &kobj_attr_um_placeholder);

	C2PS_LOGD("-\n");
	return 0;
}

static void __exit c2ps_exit(void)
{
	C2PS_LOGD("+\n");

	condition_notifier_exit = true;
	if (likely(c2ps_tsk))
		kthread_stop(c2ps_tsk);

	c2ps_sysfs_remove_file(main_base_kobj, &kobj_attr_um_placeholder);
	c2ps_sysfs_remove_dir(&main_base_kobj);

	c2ps_sysfs_exit();
	regulator_exit();
	C2PS_LOGD("- \n");
}

module_init(c2ps_init);
module_exit(c2ps_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek C2PS");
MODULE_AUTHOR("MediaTek Inc.");
