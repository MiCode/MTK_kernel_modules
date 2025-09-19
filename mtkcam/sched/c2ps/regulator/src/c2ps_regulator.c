// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "c2ps_common.h"
#include "c2ps_regulator.h"
#include "c2ps_regulator_policy.h"

static struct task_struct *c2ps_regulator_thread;
static struct kmem_cache *regulator_reqs;
static LIST_HEAD(regulator_wq);
static int regulator_condition_notifier_wq;
static bool regulator_condition_notifier_exit;
static DEFINE_MUTEX(regulator_notifier_wq_lock);
static DECLARE_WAIT_QUEUE_HEAD(regulator_notifier_wq_queue);
static bool regulator_flush_finish;
static DECLARE_WAIT_QUEUE_HEAD(regulator_flush_wq);
static int c2ps_regulator_process_mode;
static unsigned int c2ps_remote_monitor_proc_time;
static unsigned int c2ps_remote_monitor_uclamp;
static char c2ps_remote_monitor_task[30] = "None";
bool c2ps_um_mode_on = true;
bool c2ps_regulator_is_flushed;

module_param(c2ps_regulator_process_mode, int, 0644);
module_param(c2ps_remote_monitor_proc_time, int, 0644);
module_param(c2ps_remote_monitor_uclamp, int, 0644);
module_param_string(c2ps_remote_monitor_task,
			c2ps_remote_monitor_task, 30, 0644);
module_param(c2ps_um_mode_on, bool, 0644);

static inline enum c2ps_regulator_mode
decide_process_type(struct regulator_req *req)
{
	if (req->tsk_info)
		return c2ps_regulator_process_mode;
	if (c2ps_um_mode_on) {
		switch (req->stat) {
		case C2PS_STAT_NODEF:
			break;
		case C2PS_STAT_STABLE:
			if (req->glb_info && (req->glb_info->overwrite_util_margin ||
								req->glb_info->decided_um_placeholder_val))
				return C2PS_REGULATOR_BGMODE_UM_TRANSIENT;
			if (req->glb_info && req->anc_info)
				return C2PS_REGULATOR_BGMODE_UM_STABLE;
			if (req->glb_info && !req->glb_info->has_anchor_spec)
				return C2PS_REGULATOR_BGMODE_UM_STABLE_DEFAULT;
			break;
		case C2PS_STAT_TRANSIENT:
			return C2PS_REGULATOR_BGMODE_UM_TRANSIENT;
		default:
			break;
		}
	}
	return C2PS_REGULATOR_BGMODE_SIMPLE;
}

static void regulator_process(struct regulator_req *req)
{
	if (unlikely(req == NULL))
		return;

	if (unlikely(req->is_flush)) {
		regulator_flush_finish = true;
		wake_up_interruptible(&regulator_flush_wq);
		kmem_cache_free(regulator_reqs, req);
		c2ps_regulator_is_flushed = true;
		return;
	}

	if (unlikely(c2ps_regulator_is_flushed)) {
		C2PS_LOGE("c2ps regulator is already flushed");
		kmem_cache_free(regulator_reqs, req);
		return;
	}

	if (req->tsk_info &&
		strstr(c2ps_remote_monitor_task, req->tsk_info->task_name)) {
		c2ps_remote_monitor_uclamp = req->tsk_info->latest_uclamp;
		c2ps_remote_monitor_proc_time = req->tsk_info->hist_proc_time_sum /
							proc_time_window_size;
	}

	switch (decide_process_type(req)) {
	case C2PS_REGULATOR_MODE_FIX:
		c2ps_regulator_policy_fix_uclamp(req);
		break;

	case C2PS_REGULATOR_MODE_SIMPLE:
		c2ps_regulator_policy_simple(req);
		break;

	case C2PS_REGULATOR_MODE_DEBUG:
		c2ps_regulator_policy_debug_uclamp(req);
		break;

	case C2PS_REGULATOR_BGMODE_SIMPLE:
		c2ps_regulator_bgpolicy_simple(req);
		break;

	case C2PS_REGULATOR_BGMODE_UM_STABLE_DEFAULT:
		c2ps_regulator_bgpolicy_um_stable_default(req);
		break;

	case C2PS_REGULATOR_BGMODE_UM_STABLE:
		c2ps_regulator_bgpolicy_um_stable(req);
		break;

	case C2PS_REGULATOR_BGMODE_UM_TRANSIENT:
		c2ps_regulator_bgpolicy_um_transient(req);
		break;

	default:
		break;
	}

	kmem_cache_free(regulator_reqs, req);
}

static int c2ps_regulator_loop(void *arg)
{
	while (!kthread_should_stop()) {
		struct regulator_req *req = NULL;

		C2PS_LOGD("+\n");
		wait_event_interruptible(regulator_notifier_wq_queue,
					 regulator_condition_notifier_wq ||
					 regulator_condition_notifier_exit);

		if (unlikely(regulator_condition_notifier_exit))
			return 0;
		mutex_lock(&regulator_notifier_wq_lock);

		if (unlikely(!list_empty(&regulator_wq))) {
			req = list_first_entry(&regulator_wq,
					      struct regulator_req, queue_list);
			list_del(&req->queue_list);
			if (unlikely(list_empty(&regulator_wq)))
				regulator_condition_notifier_wq = 0;
			mutex_unlock(&regulator_notifier_wq_lock);
			regulator_process(req);
		} else {
			regulator_condition_notifier_wq = 0;
			mutex_unlock(&regulator_notifier_wq_lock);
		}
	}
	C2PS_LOGD("-\n");
	return 0;
}

void send_regulator_req(struct regulator_req *req)
{
	if (unlikely(req == NULL)) {
		C2PS_LOGD("[C2PS] NULL regulator request");
		return;
	}

	mutex_lock(&regulator_notifier_wq_lock);
	list_add_tail(&req->queue_list, &regulator_wq);
	regulator_condition_notifier_wq = 1;
	mutex_unlock(&regulator_notifier_wq_lock);

	wake_up_interruptible(&regulator_notifier_wq_queue);
}

struct regulator_req *get_regulator_req(void)
{
	struct regulator_req *req;

	req = kmem_cache_alloc(regulator_reqs, GFP_KERNEL | __GFP_ZERO);
	if (unlikely(req == NULL)) {
		C2PS_LOGD("[C2PS] create regulator req failed");
		return NULL;
	}

	req->is_flush = false;
	req->stat = C2PS_STAT_NODEF;
	return req;
}

int calculate_uclamp_value(struct c2ps_task_info *tsk_info)
{
	C2PS_LOGD("[C2PS] %s", __func__);
	return 0;
}

void c2ps_regulator_flush(void)
{
	struct regulator_req *flush_req = get_regulator_req();

	if (unlikely(flush_req == NULL)) {
		C2PS_LOGE("NULL flush_req");
		return;
	}
	flush_req->is_flush = true;
	send_regulator_req(flush_req);
	wait_event_interruptible(regulator_flush_wq, regulator_flush_finish);
	regulator_flush_finish = false;
}

void c2ps_regulator_init(void)
{
	c2ps_regulator_is_flushed = false;
}

int regulator_init(void)
{
	C2PS_LOGD("[C2PS] %s", __func__);
	regulator_condition_notifier_exit = false;
	c2ps_regulator_thread =
		kthread_create(c2ps_regulator_loop, NULL, "c2ps_regulator_loop");
	if (unlikely(c2ps_regulator_thread == NULL))
		return -EFAULT;

	if (likely(regulator_reqs == NULL)) {
		regulator_reqs = kmem_cache_create("regulator_reqs",
		  sizeof(struct regulator_req), 0, SLAB_HWCACHE_ALIGN, NULL);
	}

	mutex_init(&regulator_notifier_wq_lock);
	wake_up_process(c2ps_regulator_thread);
	return 0;
}

void regulator_exit(void)
{
	C2PS_LOGD("+\n");
	regulator_condition_notifier_exit = true;

	if (likely(c2ps_regulator_thread))
		kthread_stop(c2ps_regulator_thread);
	c2ps_regulator_thread = NULL;

	if (likely(regulator_reqs))
		kmem_cache_destroy(regulator_reqs);
	regulator_reqs = NULL;

	C2PS_LOGD("-\n");
}

