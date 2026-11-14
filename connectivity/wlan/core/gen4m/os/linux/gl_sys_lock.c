// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "precomp.h"
#include <linux/sched/debug.h>
#include <linux/stacktrace.h>

static void wfsys_lock_print_trace_work(struct work_struct *unused);
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
static void wfsys_lock_hold_long(struct timer_list *unused);
static DEFINE_TIMER(wfsys_lock_timer, wfsys_lock_hold_long);
#else
static void wfsys_lock_hold_long(unsigned long unused);
static DEFINE_TIMER(wfsys_lock_timer, wfsys_lock_hold_long, 0, 0);
#endif
static DEFINE_MUTEX(wfsys_mutex);
static DECLARE_WORK(wfsys_work, wfsys_lock_print_trace_work);
static struct wfsys_lock_dbg_t wfsys_dbg;

#if (KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE)
static int __save_stack_trace(unsigned long *trace)
{
	return stack_trace_save(trace, WFSYS_LOCK_MAX_TRACE, 0);
}
#endif

static void wfsys_lock_record_trace(struct task_struct *who)
{
	wfsys_dbg.task = who;
	wfsys_dbg.pid = who->pid;
	wfsys_dbg.start_time_sec = local_clock();
	wfsys_dbg.start_time_nsec = do_div(wfsys_dbg.start_time_sec,
					   NSEC_PER_SEC) / MSEC_PER_SEC;
	wfsys_dbg.start_time = sched_clock();
#if (KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE)
	wfsys_dbg.nr_entries = __save_stack_trace(wfsys_dbg.addrs);
#endif

	mod_timer(&wfsys_lock_timer, jiffies +
		  MSEC_TO_JIFFIES(WFSYS_LOCK_MAX_HOLD_TIME * MSEC_PER_SEC));
}

static void wfsys_lock_print_trace_work(struct work_struct *unused)
{
	uint8_t comm[TASK_COMM_LEN];

	get_task_comm(comm, wfsys_dbg.task);
	comm[TASK_COMM_LEN - 1] = '\0';
	DBGLOG(INIT, WARN,
		"[%d %s][%c] hold wfsys lock more than %d sec from %llu.%06llu\n",
		wfsys_dbg.pid,
		comm,
		task_state_to_char(wfsys_dbg.task),
		WFSYS_LOCK_MAX_HOLD_TIME,
		wfsys_dbg.start_time_sec,
		wfsys_dbg.start_time_nsec);
#if (KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE)
	stack_trace_print(wfsys_dbg.addrs, wfsys_dbg.nr_entries, 0);
	sched_show_task(wfsys_dbg.task);
#endif

	mod_timer(&wfsys_lock_timer, jiffies +
		  MSEC_TO_JIFFIES(WFSYS_LOCK_PRINT_PERIOD * MSEC_PER_SEC));
}

static void wfsys_lock_release_trace(void)
{
	uint64_t end_time;
	uint64_t end_time_sec;
	uint64_t end_time_nsec;
	uint64_t timeout;
	uint8_t comm[TASK_COMM_LEN];

	flush_work(&wfsys_work);
#if (KERNEL_VERSION(6, 15, 0) > CFG80211_VERSION_CODE)
	del_timer_sync(&wfsys_lock_timer);
#else
	timer_delete_sync(&wfsys_lock_timer);
#endif

	end_time_sec = local_clock();
	end_time_nsec = do_div(end_time_sec, NSEC_PER_SEC) / MSEC_PER_SEC;
	end_time = sched_clock();
	timeout = WFSYS_LOCK_MAX_HOLD_TIME;
	timeout *= NSEC_PER_SEC;

	if ((end_time - wfsys_dbg.start_time) > timeout) {
		get_task_comm(comm, wfsys_dbg.task);
		comm[TASK_COMM_LEN - 1] = '\0';
		DBGLOG(INIT, WARN,
			"wfsys lock is held by [%d %s] from %llu.%06llu to %llu.%06llu\n",
			wfsys_dbg.pid,
			comm,
			wfsys_dbg.start_time_sec,
			wfsys_dbg.start_time_nsec,
			end_time_sec,
			end_time_nsec);
	}
}

#if (KERNEL_VERSION(4, 15, 0) <= CFG80211_VERSION_CODE)
static void wfsys_lock_hold_long(struct timer_list *unused)
#else
static void wfsys_lock_hold_long(unsigned long unused)
#endif
{
	schedule_work(&wfsys_work);
}

void wfsys_lock_extend(void)
{
	mod_timer(&wfsys_lock_timer, jiffies +
		  MSEC_TO_JIFFIES(WFSYS_LOCK_MAX_HOLD_TIME * MSEC_PER_SEC));
}

void wfsys_lock(void)
{
	mutex_lock(&wfsys_mutex);
	wfsys_lock_record_trace(current);
}

int wfsys_trylock(void)
{
	int ret = 0;

	ret = mutex_trylock(&wfsys_mutex);
	if (ret)
		wfsys_lock_record_trace(current);

	return ret;
}

void wfsys_unlock(void)
{
	wfsys_lock_release_trace();
	mutex_unlock(&wfsys_mutex);
}

int wfsys_is_locked(void)
{
	return mutex_is_locked(&wfsys_mutex);
}

