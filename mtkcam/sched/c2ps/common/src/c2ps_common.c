// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/sched/cputime.h>
#include <linux/sched/clock.h>
#include <linux/prefetch.h>
#include <linux/preempt.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <trace/trace.h>

#include "sched/sched.h"
#include "c2ps_common.h"
#include "c2ps_sysfs.h"

#ifndef CREATE_TRACE_POINTS
#define CREATE_TRACE_POINTS
#endif
#include "c2ps_trace_event.h"

static DEFINE_HASHTABLE(task_info_tbl, 3);
static DEFINE_MUTEX(task_info_tbl_lock);
static DEFINE_HASHTABLE(task_group_info_tbl, 3);
static DEFINE_MUTEX(task_group_info_tbl_lock);
static DEFINE_HASHTABLE(anchor_tbl, 3);
static DEFINE_MUTEX(anchor_tbl_lock);

static struct kobject *common_base_kobj;
static struct global_info *glb_info;
static struct eas_settings *pre_eas_settings;

u8 Prime_Table[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
	59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137,
	139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199};

bool is_release_uclamp_max = false;
int proc_time_window_size = 1;
int debug_log_on = 0;
int background_idlerate_alert = 15;
int background_idlerate_dangerous = 5;
int c2ps_placeholder;
bool recovery_uclamp_max_immediately;
bool need_boost_uclamp_max = true;
bool ignore_bcpu_idle_rate;
int um_min_virtual_ceiling = 80;
module_param(proc_time_window_size, int, 0644);
module_param(debug_log_on, int, 0644);
module_param(background_idlerate_alert, int, 0644);
module_param(background_idlerate_dangerous, int, 0644);
module_param(c2ps_placeholder, int, 0644);
module_param(recovery_uclamp_max_immediately, bool, 0644);
module_param(need_boost_uclamp_max, bool, 0644);
module_param(um_min_virtual_ceiling, int, 0644);
module_param(ignore_bcpu_idle_rate, bool, 0644);

struct c2ps_task_info *c2ps_find_task_info_by_tskid(int task_id)
{
	struct c2ps_task_info *tsk_info = NULL;

	C2PS_LOGD("+\n");
	c2ps_task_info_tbl_lock(__func__);

	if (unlikely(hash_empty(task_info_tbl))) {
		C2PS_LOGD("empty task info table\n");
		goto out;
	}
	C2PS_LOGD("task_id: %d\n", task_id);
	C2PS_LOGD("find +\n");
	hash_for_each_possible(task_info_tbl, tsk_info, hlist, task_id) {
		if (tsk_info->task_id == task_id) {
			C2PS_LOGD("task_id: %d\n", tsk_info->task_id);
			goto out;
		}
	}
	C2PS_LOGD("find -\n");

out:
	c2ps_task_info_tbl_unlock(__func__);
	C2PS_LOGD("-\n");
	return tsk_info;
}

int c2ps_add_task_info(struct c2ps_task_info *tsk_info)
{
	if (unlikely(!tsk_info)) {
		C2PS_LOGE("tsk_info is null\n");
		return -EINVAL;
	}
	C2PS_LOGD("task_id: %d", tsk_info->task_id);
	c2ps_task_info_tbl_lock(__func__);
	hash_add(task_info_tbl, &tsk_info->hlist, tsk_info->task_id);
	c2ps_task_info_tbl_unlock(__func__);
	C2PS_LOGD("-\n");
	return 0;
}

void c2ps_clear_task_info_table(void)
{
	struct c2ps_task_info *tsk_info = NULL;
	struct hlist_node *tmp = NULL;
	int bkt = 0;

	C2PS_LOGD("+\n");
	c2ps_task_info_tbl_lock(__func__);
	if (unlikely(hash_empty(task_info_tbl))) {
		C2PS_LOGD("task info table is empty\n");
		goto out;
	}

	hash_for_each_safe(
		task_info_tbl, bkt, tmp, tsk_info, hlist) {
		hash_del(&tsk_info->hlist);
		kfree(tsk_info);
		tsk_info = NULL;
	}

out:
	c2ps_task_info_tbl_unlock(__func__);
	C2PS_LOGD("-\n");
}

struct c2ps_anchor *c2ps_find_anchor_by_id(int anc_id)
{
	struct c2ps_anchor *anc_info = NULL;

	C2PS_LOGD("+\n");
	c2ps_anchor_tbl_lock();

	if (unlikely(hash_empty(anchor_tbl))) {
		C2PS_LOGD("empty anchor table\n");
		goto out;
	}
	C2PS_LOGD("anc_id: %d\n", anc_id);
	C2PS_LOGD("find +\n");
	hash_for_each_possible(anchor_tbl, anc_info, hlist, anc_id) {
		if (anc_info->anchor_id == anc_id) {
			C2PS_LOGD("anc_id: %d\n", anc_info->anchor_id);
			goto out;
		}
	}
	C2PS_LOGD("find -\n");

out:
	c2ps_anchor_tbl_unlock();
	C2PS_LOGD("-\n");
	return anc_info;
}

int c2ps_add_anchor(struct c2ps_anchor *anc_info)
{
	if (unlikely(!anc_info)) {
		C2PS_LOGE("anc_info is null\n");
		return -EINVAL;
	}
	C2PS_LOGD("anchor_id: %d", anc_info->anchor_id);
	c2ps_anchor_tbl_lock();
	hash_add(anchor_tbl, &anc_info->hlist, anc_info->anchor_id);
	c2ps_anchor_tbl_unlock();
	C2PS_LOGD("-\n");
	return 0;
}

void c2ps_clear_anchor_table(void)
{
	struct c2ps_anchor *anc_info = NULL;
	struct hlist_node *tmp1 = NULL, *tmp2 = NULL;
	struct um_table_item *table_item = NULL;
	int bkt1 = 0, bkt2 = 0;

	C2PS_LOGD("+\n");
	c2ps_anchor_tbl_lock();
	if (unlikely(hash_empty(anchor_tbl))) {
		C2PS_LOGD("anchor table_item is empty\n");
		goto out;
	}

	hash_for_each_safe(
		anchor_tbl, bkt1, tmp1, anc_info, hlist) {
		hash_for_each_safe(anc_info->um_table_stbl, bkt2, tmp2, table_item, hlist) {
			hash_del(&table_item->hlist);
			kfree(table_item);
			table_item = NULL;
		}

		hash_del(&anc_info->hlist);
		kfree(anc_info);
		anc_info = NULL;
	}

out:
	c2ps_anchor_tbl_unlock();
	C2PS_LOGD("-\n");
}

int c2ps_create_task_group(int group_head, u64 task_group_target_time)
{
	struct task_group_info *tsk_grp_info = NULL;

	C2PS_LOGD("group_head: %d, task_group_target_time: %lld",
			group_head, task_group_target_time);

	tsk_grp_info = kzalloc(sizeof(*tsk_grp_info), GFP_KERNEL);

	if (unlikely(!tsk_grp_info)) {
		C2PS_LOGE("OOM\n");
		return -ENOMEM;
	}

	tsk_grp_info->group_head = group_head;
	tsk_grp_info->group_target_time = task_group_target_time * NSEC_PER_MSEC;
	mutex_init(&tsk_grp_info->mlock);

	c2ps_task_group_info_tbl_lock(__func__);
	hash_add(
		task_group_info_tbl, &tsk_grp_info->hlist, tsk_grp_info->group_head);
	c2ps_task_group_info_tbl_unlock(__func__);
	C2PS_LOGD("-\n");
	return 0;
}

struct task_group_info *c2ps_find_task_group_info_by_grphd(int group_head)
{
	struct task_group_info *tsk_grp_info = NULL;

	C2PS_LOGD("+\n");
	c2ps_task_group_info_tbl_lock(__func__);

	if (hash_empty(task_group_info_tbl)) {
		C2PS_LOGD("empty task group info table\n");
		goto out;
	}
	C2PS_LOGD("group_head: %d\n", group_head);
	C2PS_LOGD("find +\n");
	hash_for_each_possible(task_group_info_tbl, tsk_grp_info, hlist, group_head) {
		if (tsk_grp_info->group_head == group_head) {
			C2PS_LOGD("group_head: %d\n", tsk_grp_info->group_head);
			goto out;
		}
	}
	C2PS_LOGD("find -\n");

out:
	c2ps_task_group_info_tbl_unlock(__func__);
	C2PS_LOGD("-\n");
	return tsk_grp_info;
}

int c2ps_add_task_to_group(struct c2ps_task_info *tsk_info, int group_head)
{
	struct task_group_info *tsk_grp_info =
		c2ps_find_task_group_info_by_grphd(group_head);

	C2PS_LOGD("+\n");
	if (unlikely(!tsk_info)) {
		C2PS_LOGE("tsk_info is null\n");
		return -EINVAL;
	}

	if (unlikely(!tsk_grp_info)) {
		C2PS_LOGE("task group not exist\n");
		return -EINVAL;
	}
	tsk_info->tsk_group = tsk_grp_info;
	C2PS_LOGD("-\n");
	return 0;
}

void c2ps_clear_task_group_info_table(void)
{
	struct task_group_info *tsk_grp_info = NULL;
	struct hlist_node *tmp = NULL;
	int bkt = 0;

	C2PS_LOGD("+\n");
	c2ps_task_group_info_tbl_lock(__func__);
	if (unlikely(hash_empty(task_group_info_tbl))) {
		C2PS_LOGD("task group info table is empty\n");
		goto out;
	}

	hash_for_each_safe(
		task_group_info_tbl, bkt, tmp, tsk_grp_info, hlist) {
		hash_del(&tsk_grp_info->hlist);
		kfree(tsk_grp_info);
		tsk_grp_info = NULL;
	}

out:
	c2ps_task_group_info_tbl_unlock(__func__);
	C2PS_LOGD("-\n");
}

static inline void c2ps_prefetch_curr_exec_start(struct task_struct *p)
{
#if IS_ENABLED(CONFIG_FAIR_GROUP_SCHED)
	struct sched_entity *curr = (&p->se)->cfs_rq->curr;
#else
	struct sched_entity *curr = (&task_rq(p)->cfs)->curr;
#endif
	prefetch(curr);
	prefetch(&curr->exec_start);
}

extern struct rq *task_rq_lock(struct task_struct *p, struct rq_flags *rf);
/*
 * Return accounted runtime for the task.
 * In case the task is currently running, return the runtime plus current's
 * pending runtime that have not been accounted yet.
 */
u64 c2ps_task_sched_runtime(struct task_struct *p)
{
	struct rq_flags rf;
	struct rq *rq;
	u64 ns;

#if IS_ENABLED(CONFIG_64BIT) && IS_ENABLED(CONFIG_SMP)
	if (!p->on_cpu || !task_on_rq_queued(p))
		return p->se.sum_exec_runtime;
#endif

	rq = task_rq_lock(p, &rf);

	if (task_current(rq, p) && task_on_rq_queued(p)) {
		c2ps_prefetch_curr_exec_start(p);
		update_rq_clock(rq);
		p->sched_class->update_curr(rq);
	}
	ns = p->se.sum_exec_runtime;
	task_rq_unlock(rq, p, &rf);

	return ns;
}

u64 c2ps_get_sum_exec_runtime(int pid)
{
	struct task_struct *tsk;
	u64 curr_sum_exec_runtime = 0;

	rcu_read_lock();
	tsk = find_task_by_vpid(pid);
	if (likely(tsk))
		get_task_struct(tsk);
	rcu_read_unlock();

	if (unlikely(!tsk))
		return 0;
	curr_sum_exec_runtime = c2ps_task_sched_runtime(tsk);
	put_task_struct(tsk);

	return curr_sum_exec_runtime;
}

// be caution when using this function, DO NOT call it frequently
struct task_struct *c2ps_find_waker_task(struct task_struct *cur_task)
{
	struct task_struct *temp = NULL, *task = NULL;

	for_each_process_thread(temp, task) {
		if (task->last_wakee == cur_task) {
			C2PS_LOGD("%s(%d) is waked up by %s(%d)\n",
				cur_task->comm, cur_task->pid, task->comm, task->pid);
			return task;
		}
	}
	return NULL;
}

// be caution when using this function, DO NOT call it frequently
int c2ps_find_waker_pid(int cur_task_pid)
{
	struct task_struct *cur_task = NULL, *waker_task = NULL;

	rcu_read_lock();
	cur_task = find_task_by_vpid(cur_task_pid);
	if (likely(cur_task))
		get_task_struct(cur_task);
	rcu_read_unlock();

	if (likely(cur_task)) {
		waker_task = c2ps_find_waker_task(cur_task);
		put_task_struct(cur_task);
	}

	if (likely(waker_task != NULL))
		return waker_task->pid;
	else
		return -1;
}

// be caution when using this function, DO NOT call it frequently
void c2ps_add_waker_pid_to_task_info(struct c2ps_task_info *tsk_info)
{
	int _i = 0;
	int waker_pid = c2ps_find_waker_pid(tsk_info->pid);

	if (waker_pid <= 0)
		return;

	for (; _i < MAX_DEP_THREAD_NUM; _i++) {
		if (tsk_info->dep_thread[_i] == waker_pid)
			break;
		if (tsk_info->dep_thread[_i] <= 0) {
			tsk_info->dep_thread[_i] = waker_pid;
			break;
		}
	}
}

inline void c2ps_task_info_tbl_lock(const char *tag)
{
	mutex_lock(&task_info_tbl_lock);
}

inline void c2ps_task_info_tbl_unlock(const char *tag)
{
	mutex_unlock(&task_info_tbl_lock);
}

inline void c2ps_task_group_info_tbl_lock(const char *tag)
{
	mutex_lock(&task_group_info_tbl_lock);
}

inline void c2ps_task_group_info_tbl_unlock(const char *tag)
{
	mutex_unlock(&task_group_info_tbl_lock);
}

inline void c2ps_info_lock(struct mutex *mlock)
{
	mutex_lock(mlock);
}

inline void c2ps_info_unlock(struct mutex *mlock)
{
	mutex_unlock(mlock);
}

inline void c2ps_anchor_tbl_lock(void)
{
	mutex_lock(&anchor_tbl_lock);
}

inline void c2ps_anchor_tbl_unlock(void)
{
	mutex_unlock(&anchor_tbl_lock);
}

struct um_table_item *c2ps_find_um_table_by_um(struct c2ps_anchor *anc, int um)
{
	struct um_table_item *_um_table_item = NULL;

	if (unlikely(anc == NULL))
		return NULL;

	C2PS_LOGD("+\n");

	if (unlikely(hash_empty(anc->um_table_stbl))) {
		C2PS_LOGD("empty anchor um table\n");
		goto out;
	}
	C2PS_LOGD("find +\n");
	hash_for_each_possible(anc->um_table_stbl, _um_table_item, hlist, um) {
		if (_um_table_item->um == um) {
			C2PS_LOGD("um: %d\n", um);
			goto out;
		}
	}
	C2PS_LOGD("find -\n");

out:
	C2PS_LOGD("-\n");
	return _um_table_item;
}

int c2ps_add_um_table(struct c2ps_anchor *anc, struct um_table_item *table_item)
{
	C2PS_LOGD("+\n");
	if (unlikely(!anc)) {
		C2PS_LOGE("anc_info is null\n");
		return -EINVAL;
	}
	hash_add(anc->um_table_stbl, &table_item->hlist, table_item->um);
	C2PS_LOGD("-\n");
	return 0;
}

inline void c2ps_init_kf(
	struct kf_est *kf, int64_t q_val, int64_t meas_err, int64_t min_est_err)
{
	kf->q_val = q_val;
	kf->meas_err = meas_err;
	kf->est_err = min_est_err;
	kf->min_est_err = min_est_err;
}

u64 c2ps_cal_kf_est(struct kf_est *kf, u64 cur_obs)
{
	if (unlikely(kf->q_val <= 0 || kf->meas_err <= 0)) {
		C2PS_LOGE("kf is not properly set");
		return 0;
	}

	if (unlikely(kf->est_val <= 0)) {
		kf->est_val = cur_obs;
	} else {
		int64_t temp_est_val;
		int64_t temp_measure = cur_obs;

		temp_est_val = kf->est_val +
				(kf->est_err * (temp_measure - kf->est_val) /
					(kf->est_err + kf->meas_err));
		kf->est_err = kf->est_err * kf->meas_err /
			(kf->est_err + kf->meas_err)
			+ (abs(kf->est_val - temp_est_val) * kf->q_val / 1000);
		kf->est_err = kf->est_err < kf->min_est_err? kf->min_est_err:kf->est_err;
		kf->est_val = temp_est_val;
	}

	return abs(kf->est_val);
}

void c2ps_update_um_table(struct c2ps_anchor *anc)
{
	struct um_table_item *_item = c2ps_find_um_table_by_um(anc, glb_info->curr_um);

	if (unlikely(glb_info->stat == C2PS_STAT_TRANSIENT))
		return;

	C2PS_LOGD("check um table, anc_id: %d, um: %u",
				anc->anchor_id, glb_info->curr_um);
	if (unlikely(_item == NULL)) {
		struct um_table_item *_prev_item =
			c2ps_find_um_table_by_um(anc,
				glb_info->curr_um + c2ps_regulator_base_update_um);

		C2PS_LOGD("[C2PS_CB] add um table (ancid:%d), um:%d\n",
					anc->anchor_id, glb_info->curr_um);
		_item = kzalloc(sizeof(*_item), GFP_KERNEL);

		if (unlikely(!_item)) {
			C2PS_LOGE("OOM\n");
			return;
		}
		_item->um = glb_info->curr_um;
		c2ps_init_kf(&(_item->lat_est), ANC_KF_QVAL,
				ANC_KF_MEAS_ERR, ANC_KF_MIN_EST_ERR);

		if (likely(_prev_item)) {
			_item->lat_est.est_val = (_prev_item->latency + anc->latest_duration/1000)/2;
			C2PS_LOGD("init um:%u latency to %llu", _item->um, _item->lat_est.est_val);
		}

		if (anc->jitter_spec > 0) {
			c2ps_init_kf(&(_item->end_diff_est), ANC_KF_QVAL,
					ANC_KF_MEAS_ERR, ANC_KF_MIN_EST_ERR);
			c2ps_init_kf(&(_item->jit_est), ANC_KF_QVAL,
					ANC_KF_MEAS_ERR*ANC_KF_MEAS_ERR/1000, ANC_KF_MIN_EST_ERR);
			_item->end_diff_est.est_val = 1000000 / glb_info->cfg_camfps;
			_item->jit_est.est_val = anc->jitter_spec;
		}

		if (unlikely(c2ps_add_um_table(anc, _item))) {
			C2PS_LOGE("add um table failed\n");
			return;
		}
	}

	_item->latency = c2ps_cal_kf_est(&(_item->lat_est), anc->latest_duration/1000);
	C2PS_LOGD("check um table, ancid: %d, um:%d, ori: %llu, after_proc:%llu",
		anc->anchor_id, _item->um, anc->latest_duration, _item->latency);

	if (anc->jitter_spec > 0) {
		u64 _end_diff = (anc->e_hist_t[anc->e_idx] -
						anc->e_hist_t[anc->e_idx-1 >= 0?
							anc->e_idx-1 : anc->e_idx+ANCHOR_QUEUE_LEN-1])/1000;
		u64 _square = 0;

		// add an end_diff upper bound to avoid initial value too large
		// 100000 represents 100ms
		if (unlikely(_end_diff > 100000))
			_end_diff = 1000000 / glb_info->cfg_camfps;

		_item->end_diff = c2ps_cal_kf_est(&(_item->end_diff_est), _end_diff);
		_square = (_end_diff - _item->end_diff) *
					(_end_diff - _item->end_diff);
		_item->jitter = c2ps_cal_kf_est(&(_item->jit_est), _square/1000);

		C2PS_LOGD(
			"check um table, ancid: %d, um:%d, after_proc jitter:%llu, end diff: %llu jitter raw data: %llu",
			anc->anchor_id, _item->um, _item->jitter, _item->end_diff,
			(_end_diff-_item->end_diff) * (_end_diff-_item->end_diff));
		c2ps_main_systrace(
			"check um table, ancid: %d, um:%d, after_proc jitter:%llu, end diff: %llu jitter raw data: %llu",
			anc->anchor_id, _item->um, _item->jitter, _item->end_diff,
			(_end_diff-_item->end_diff) * (_end_diff-_item->end_diff));
	}

	c2ps_check_last_anc(anc);

	c2ps_main_systrace("check um table, ancid: %d, um:%d, ori: %llu, after_proc:%llu",
		anc->anchor_id, _item->um, anc->latest_duration, _item->latency);
}

void c2ps_check_last_anc(struct c2ps_anchor *anc)
{
	if (unlikely(glb_info->um_vote.total_voter_mul == 0)) {
		glb_info->um_vote.total_voter_mul = 1;
		glb_info->um_vote.curr_voter_mul = 1;
		glb_info->um_vote.last_anchor_decided = false;
		anc->is_last_anchor = false;
	}

	if (unlikely(anc->anchor_id < 0))
		return;

	if (likely(glb_info->um_vote.last_anchor_decided))
		goto out;

	if (unlikely(glb_info->um_vote.total_voter_mul % Prime_Table[anc->anchor_id] != 0)) {
		glb_info->um_vote.total_voter_mul *= Prime_Table[anc->anchor_id];

		// find is_last_anchor only when all anchor are detected
		goto out;
	}

	if (likely(glb_info->um_vote.curr_voter_mul % Prime_Table[anc->anchor_id] != 0)) {
		glb_info->um_vote.curr_voter_mul *= Prime_Table[anc->anchor_id];
		if (likely(glb_info->um_vote.curr_voter_mul <
					glb_info->um_vote.total_voter_mul)) {
			anc->is_last_anchor = false;
			goto out;
		}
	}

	glb_info->um_vote.curr_voter_mul = 1;

	// if the same anchor is determined is_last_anchor last time, finish the
	// search
	if (anc->is_last_anchor)
		glb_info->um_vote.last_anchor_decided = true;

	anc->is_last_anchor = true;

out:
	C2PS_LOGD("anc um vote: %d, prime: %u last_anc: %d, total voter: %u, cur voter: %u",
		anc->anchor_id, Prime_Table[anc->anchor_id], anc->is_last_anchor,
		glb_info->um_vote.total_voter_mul, glb_info->um_vote.curr_voter_mul);
}

u64 c2ps_get_time(void)
{
	u64 tmp;

	preempt_disable();
	tmp = cpu_clock(smp_processor_id());
	preempt_enable();

	return tmp;
}

void c2ps_update_task_info_hist(struct c2ps_task_info *tsk_info)
{
	C2PS_LOGD("+\n");
	c2ps_info_lock(&tsk_info->mlock);

	if (tsk_info->hist_proc_time_sum >=
		tsk_info->hist_proc_time[tsk_info->nr_hist_info]) {
		tsk_info->hist_proc_time_sum -=
	                          tsk_info->hist_proc_time[tsk_info->nr_hist_info];
	} else {
		tsk_info->hist_proc_time_sum = 0;
	}

	tsk_info->hist_proc_time[tsk_info->nr_hist_info] = tsk_info->proc_time;
	tsk_info->hist_proc_time_sum +=
		tsk_info->hist_proc_time[tsk_info->nr_hist_info];
	tsk_info->hist_loading[tsk_info->nr_hist_info] = tsk_info->loading;
	++tsk_info->nr_hist_info;
	tsk_info->nr_hist_info %= (min(proc_time_window_size , MAX_WINDOW_SIZE));
	c2ps_info_unlock(&tsk_info->mlock);
	C2PS_LOGD("-\n");
}

struct global_info *get_glb_info(void)
{
	return glb_info;
}

inline void set_config_camfps(int camfps)
{
	if (unlikely(!glb_info)) {
		C2PS_LOGE("glb_info is null\n");
		return;
	}
	c2ps_info_lock(&glb_info->mlock);
	glb_info->cfg_camfps = camfps;
	c2ps_info_unlock(&glb_info->mlock);
}

void decide_special_uclamp_max(int placeholder_type)
{
	size_t cluster_size = c2ps_nr_clusters * sizeof(int);

	if (unlikely(!glb_info)) {
		C2PS_LOGE("glb_info is null\n");
		return;
	}

	c2ps_info_lock(&glb_info->mlock);
	switch (placeholder_type) {
	// For backward compatibility, set special uclamp max to
	// placeholder1 setting for case 0
	case 0:
	case 1:
		memcpy(glb_info->special_uclamp_max, glb_info->uclamp_max_placeholder1, cluster_size);
		break;
	case 2:
		memcpy(glb_info->special_uclamp_max, glb_info->uclamp_max_placeholder2, cluster_size);
		break;
	case 3:
		memcpy(glb_info->special_uclamp_max, glb_info->uclamp_max_placeholder3, cluster_size);
		break;
	default:
		break;
	}
	memcpy(glb_info->uclamp_max_floor, glb_info->special_uclamp_max, cluster_size);
	c2ps_info_unlock(&glb_info->mlock);
	C2PS_LOGD(
		"special_cluster_0_util=%d special_cluster_1_util=%d "
		"special_cluster_2_util=%d\n",
		glb_info->special_uclamp_max[0], glb_info->special_uclamp_max[1],
		glb_info->special_uclamp_max[2]);
}

inline void set_glb_info_bg_uclamp_max(void)
{
	if (unlikely(!glb_info)) {
		C2PS_LOGE("glb_info is null\n");
		return;
	}
	c2ps_info_lock(&glb_info->mlock);
	{
		short _idx = 0;
		for (; _idx < c2ps_nr_clusters; _idx++) {
			glb_info->max_uclamp[_idx] = get_gear_uclamp_max(_idx);
			glb_info->curr_max_uclamp[_idx] = glb_info->max_uclamp[_idx];
			glb_info->uclamp_max_ceiling[_idx] = glb_info->max_uclamp[_idx];
			glb_info->overwrite_uclamp_max[_idx] = glb_info->max_uclamp[_idx];
			glb_info->scn_cpu_freq_floor[_idx] = INT_MAX;
		}
	}
	c2ps_info_unlock(&glb_info->mlock);
}

inline void set_glb_info_bg_util_margin(void)
{
	if (unlikely(!glb_info)) {
		C2PS_LOGE("glb_info is null\n");
		return;
	}
	c2ps_info_lock(&glb_info->mlock);
	{
		glb_info->curr_um = 125;
		glb_info->curr_um_idle = 125;
	}
	c2ps_info_unlock(&glb_info->mlock);
}

inline void update_vsync_time(u64 ts)
{
	if (unlikely(!glb_info)) {
		C2PS_LOGE("glb_info is null\n");
		return;
	}
	c2ps_info_lock(&glb_info->mlock);
	glb_info->vsync_time = ts;
	c2ps_info_unlock(&glb_info->mlock);
}

inline void update_camfps(int camfps)
{
	if (unlikely(!glb_info)) {
		C2PS_LOGE("glb_info is null\n");
		return;
	}
	c2ps_info_lock(&glb_info->mlock);
	glb_info->camfps = camfps;
	c2ps_info_unlock(&glb_info->mlock);
}

inline bool is_group_head(struct c2ps_task_info *tsk_info)
{
	if (unlikely(!tsk_info)) {
		C2PS_LOGE("tsk_info is null\n");
		return false;
	}
	return tsk_info->task_id == tsk_info->tsk_group->group_head;
}

void c2ps_systrace_c(pid_t pid, int val, const char *fmt, ...)
{
	char log[256];
	va_list args;
	int len;
	char buf[256];

	if (likely(!trace_c2ps_systrace_enabled()))
		return;

	memset(log, ' ', sizeof(log));
	va_start(args, fmt);
	len = vsnprintf(log, sizeof(log), fmt, args);
	va_end(args);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		log[255] = '\0';

	len = snprintf(buf, sizeof(buf), "C|%d|%s|%d\n", pid, log, val);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		buf[255] = '\0';

	trace_c2ps_systrace(buf);
}

void c2ps_main_systrace(const char *fmt, ...)
{
	char log[256];
	va_list args;
	int len;
	char buf[256];

	if (likely(!trace_c2ps_main_trace_enabled()))
		return;

	memset(log, ' ', sizeof(log));
	va_start(args, fmt);
	len = vsnprintf(log, sizeof(log), fmt, args);
	va_end(args);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		log[255] = '\0';

	len = snprintf(buf, sizeof(buf), "%s\n", log);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		buf[255] = '\0';

	trace_c2ps_main_trace(buf);
}

void c2ps_bg_info_systrace(const char *fmt, ...)
{
	char log[256];
	va_list args;
	int len;
	char buf[256];

	if (likely(!trace_c2ps_bg_info_enabled()))
		return;

	memset(log, ' ', sizeof(log));
	va_start(args, fmt);
	len = vsnprintf(log, sizeof(log), fmt, args);
	va_end(args);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		log[255] = '\0';

	len = snprintf(buf, sizeof(buf), "%s\n", log);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		buf[255] = '\0';

	trace_c2ps_bg_info(buf);
}

void c2ps_bg_info_um_default_systrace(const char *fmt, ...)
{
	char log[256];
	va_list args;
	int len;
	char buf[256];

	if (likely(!trace_c2ps_bg_info_um_default_enabled()))
		return;

	memset(log, ' ', sizeof(log));
	va_start(args, fmt);
	len = vsnprintf(log, sizeof(log), fmt, args);
	va_end(args);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		log[255] = '\0';

	len = snprintf(buf, sizeof(buf), "%s\n", log);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		buf[255] = '\0';

	trace_c2ps_bg_info_um_default(buf);
}

void c2ps_bg_info_um_systrace(const char *fmt, ...)
{
	char log[256];
	va_list args;
	int len;
	char buf[256];

	if (likely(!trace_c2ps_bg_info_um_enabled()))
		return;

	memset(log, ' ', sizeof(log));
	va_start(args, fmt);
	len = vsnprintf(log, sizeof(log), fmt, args);
	va_end(args);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		log[255] = '\0';

	len = snprintf(buf, sizeof(buf), "%s\n", log);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		buf[255] = '\0';

	trace_c2ps_bg_info_um(buf);
}

void c2ps_critical_task_systrace(struct c2ps_task_info *tsk_info)
{
	int len;
	unsigned int curr_cpu = 0;
	unsigned long curr_freq = 0;
	unsigned int curr_util = 0;
	char buf[256];
	struct task_struct *p;

	if (unlikely(!tsk_info)) {
		C2PS_LOGE("tsk_info is null\n");
		return;
	}
	if (likely(!trace_c2ps_critical_task_enabled()))
		return;

	curr_util = tsk_info->latest_uclamp;

	rcu_read_lock();
	p = find_task_by_vpid(tsk_info->pid);

	if (likely(p)) {
		get_task_struct(p);
		curr_cpu = task_cpu(p);
		curr_freq = c2ps_get_uclamp_freq(curr_cpu, curr_util);
		put_task_struct(p);
	}
	rcu_read_unlock();

	len = snprintf(buf, sizeof(buf),
		"task_name=%s_%d util=%d freq=%ld\n",
		tsk_info->task_name,  tsk_info->task_id,
		curr_util, curr_freq);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == 256))
		buf[255] = '\0';

	trace_c2ps_critical_task(buf);
}

void *c2ps_alloc_atomic(int i32Size)
{
	void *pvBuf;

	if (i32Size <= PAGE_SIZE)
		pvBuf = kmalloc(i32Size, GFP_ATOMIC);
	else
		pvBuf = vmalloc(i32Size);

	return pvBuf;
}

void c2ps_free(void *pvBuf, int i32Size)
{
	if (unlikely(!pvBuf))
		return;

	if (i32Size <= PAGE_SIZE)
		kfree(pvBuf);
	else
		vfree(pvBuf);
}

unsigned long c2ps_get_um_virtual_ceiling(int cpu, unsigned int um)
{
	unsigned long _util = 0;

	if (um >= 100)
		return INT_MAX;

	um = max_t(int, um, um_min_virtual_ceiling);
	_util = pd_get_freq_util(cpu, INT_MAX) * um / 100;
	return pd_get_util_freq(cpu, _util);
}

unsigned long c2ps_get_uclamp_freq(int cpu, unsigned int uclamp)
{
	unsigned long am_util = 0;

	am_util = (uclamp * get_adaptive_margin(cpu)) >> SCHED_CAPACITY_SHIFT;
	return pd_get_util_freq(cpu, am_util);
}

bool c2ps_get_cur_cpu_floor_uclamp(const int cpu, int *floor_uclamp, int floor_freq)
{
	if (unlikely(!floor_uclamp))
		return false;

	if (likely(floor_freq)) {
		C2PS_LOGD("cpu%d floor freq: %d", cpu, floor_freq);
		*floor_uclamp = (pd_get_freq_util(cpu, floor_freq) << SCHED_CAPACITY_SHIFT) /
						get_adaptive_margin(cpu) - MIN_UCLAMP_MARGIN;
		return true;
	}

	return false;
}

u32 c2ps_get_cur_cpu_freq_floor(const int cpu)
{
	u32 cur_cpu_floor = 0;
	struct cpufreq_policy *_policy;

	_policy = cpufreq_cpu_get(cpu);
	if (likely(_policy)) {
		cur_cpu_floor = _policy->min;
		cpufreq_cpu_put(_policy);
		C2PS_LOGD("cpu%d floor freq: %d", cpu, cur_cpu_floor);
	}
	return cur_cpu_floor;
}

u32 c2ps_get_cur_cpu_freq(const int cpu)
{
	u32 cur_cpu_freq = 0;
	struct cpufreq_policy *_policy;

	_policy = cpufreq_cpu_get(cpu);
	if (likely(_policy))
		cur_cpu_freq = _policy->cached_target_freq;

	return cur_cpu_freq;
}

inline int c2ps_get_cpu_min_uclamp(const int cpu)
{
	return pd_get_freq_util(cpu, 0);
}

inline int c2ps_get_cpu_max_uclamp(const int cpu)
{
	return pd_get_freq_util(cpu, INT_MAX);
}

bool c2ps_boost_cur_uclamp_max(
	const int cluster, int cpu_floor_freq, struct global_info *g_info)
{
	int cpu_index = 0;
	int cur_floor_uclamp = 0;
	int cur_uclamp_max_freq = 0;
	int *cur_uclamp_max = &(g_info->curr_max_uclamp[cluster]);

	if (unlikely(!need_boost_uclamp_max))
		return false;

	cpu_index = c2ps_get_first_cpu_of_cluster(cluster);

	if (unlikely(cpu_index == -1))
		return false;

	if (unlikely(!c2ps_get_cur_cpu_floor_uclamp(
				cpu_index, &cur_floor_uclamp, cpu_floor_freq)))
		return false;

	cur_uclamp_max_freq = c2ps_get_uclamp_freq(cpu_index, *cur_uclamp_max);
	if (cur_uclamp_max_freq < cpu_floor_freq &&
		*cur_uclamp_max < cur_floor_uclamp) {
		*cur_uclamp_max = cur_floor_uclamp;
		*cur_uclamp_max = min(*cur_uclamp_max, c2ps_get_cpu_max_uclamp(cpu_index));
		C2PS_LOGD("boost cpu%d uclamp max to: %d", cpu_index, *cur_uclamp_max);
		c2ps_main_systrace("boost cpu%d uclamp max to: %d",
							cpu_index, *cur_uclamp_max);
		return true;
	}
	return false;
}

inline unsigned long c2ps_get_cluster_uclamp_freq(int cluster,  unsigned int uclamp)
{
	int cpu = c2ps_get_first_cpu_of_cluster(cluster);

	if (unlikely(cpu == -1))
		return 0;
	return c2ps_get_uclamp_freq(cpu, uclamp);
}

int c2ps_get_first_cpu_of_cluster(int cluster)
{
	struct cpumask *gear_cpus;
	int cpu = 0;

	if (unlikely(cluster >= c2ps_nr_clusters))
		return -1;
	gear_cpus = get_gear_cpumask(cluster);

	if (!gear_cpus)
		return -1;

	cpu = cpumask_first(gear_cpus);
	return cpu;
}

int c2ps_get_nr_cpus_of_cluster(int cluster)
{
	struct cpumask *gear_cpus;
	int nr_cpus = 0;

	if (unlikely(cluster >= c2ps_nr_clusters))
		return 0;
	gear_cpus = get_gear_cpumask(cluster);

	if (!gear_cpus)
		return 0;

	nr_cpus = cpumask_weight(gear_cpus);
	return nr_cpus;
}

void reset_task_eas_setting(struct c2ps_task_info *tsk_info)
{
	if (unlikely(!tsk_info)) {
		C2PS_LOGE("tsk_info is null\n");
		return;
	}

	if (tsk_info->pid < 0)
		return;
	set_curr_uclamp_hint(tsk_info->pid, 0);
	reset_task_uclamp(tsk_info->pid);

	if (tsk_info->is_vip_task && tsk_info->vip_set_by_monitor) {
		C2PS_LOGD("unset VIP by monitor: %d", tsk_info->pid);
		unset_task_basic_vip(tsk_info->pid);
		tsk_info->vip_set_by_monitor = false;
	}
	if (unlikely(tsk_info->is_enable_dep_thread)) {
		int _i = 0;

		for (; _i < MAX_DEP_THREAD_NUM; _i++) {
			if (tsk_info->dep_thread[_i] <= 0)
				break;
			set_curr_uclamp_hint(tsk_info->dep_thread[_i], 0);
			reset_task_uclamp(tsk_info->dep_thread[_i]);
			if (tsk_info->is_vip_task)
				unset_task_basic_vip(tsk_info->dep_thread[_i]);
		}
	}
}

void set_uclamp(const int pid, unsigned int max_util, unsigned int min_util)
{
	int ret = -1;
	struct task_struct *p;
	// unsigned long task_load;
	struct sched_attr attr = {};

	if (pid < 0)
		return;

	min_util = clamp(min_util, 1U, 1024U);
	max_util = clamp(max_util, 1U, 1024U);

	attr.sched_policy = SCHED_NORMAL;
	attr.sched_flags = SCHED_FLAG_KEEP_ALL | SCHED_FLAG_UTIL_CLAMP;

	attr.sched_util_min = min_util;
	attr.sched_util_max = max_util;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	if (likely(p))
		get_task_struct(p);
	rcu_read_unlock();

	if (likely(p)) {
		C2PS_LOGD("check pid name: %s, pid: %d", p->comm, pid);
		ret = sched_setattr_nocheck(p, &attr);
		/* set this task to break system uclamp max limitation */
		set_curr_uclamp_hint(pid, 1);

		if (ret == 0) {
			C2PS_LOGD("set uclamp(%d, %d) to %s successfully",
					  attr.sched_util_min, attr.sched_util_max,
					  p->comm);
		} else {
			C2PS_LOGD("set uclamp(%d, %d) to %s failed",
					  attr.sched_util_min, attr.sched_util_max,
					  p->comm);
		}
		put_task_struct(p);
	}
	c2ps_systrace_c(pid, min_util, "uclamp min");
	c2ps_systrace_c(pid, max_util, "uclamp max");
}

void reset_task_uclamp(int pid)
{
	int ret = -1;
	struct task_struct *p;
	struct sched_attr attr = {};
	struct global_info *glb_info = get_glb_info();

	if (unlikely(glb_info == NULL))
		return;

	attr.sched_policy = SCHED_NORMAL;
	attr.sched_flags = SCHED_FLAG_KEEP_ALL | SCHED_FLAG_UTIL_CLAMP;

	attr.sched_util_min = 1;
	if (likely(glb_info != NULL)) {
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
}

inline bool need_update_single_shot_uclamp_max(int *uclamp_max)
{
	short cluster_index = 0;

	for (; cluster_index < c2ps_nr_clusters; ++cluster_index) {
		if (uclamp_max[cluster_index] <= 0)
			return false;
	}
	C2PS_LOGD("update single shot uclamp_max: %d, %d, %d",
			uclamp_max[0], uclamp_max[1], uclamp_max[2]);
	return true;
}

inline bool need_update_critical_task_uclamp(int *critical_task_uclamp)
{
	short task_idx = 0;

	for (; task_idx < MAX_CRITICAL_TASKS; ++task_idx) {
		if (critical_task_uclamp[task_idx] > 0)
			return true;
	}
	return false;
}

void update_critical_task_uclamp_by_tsk_id(
	int *critical_task_ids, int *critical_task_uclamp)
{
	short task_idx = 0;

	if (!critical_task_ids || !critical_task_uclamp) {
		C2PS_LOGE(
			"critical_task_ids or critical_task_uclamp is NULL");
		return;
	}

	for (; task_idx < MAX_CRITICAL_TASKS; ++task_idx) {
		int task_id = critical_task_ids[task_idx];

		if (task_id > 0) {
			struct c2ps_task_info *tsk_info =
						c2ps_find_task_info_by_tskid(task_id);

			if (!tsk_info) {
				C2PS_LOGE("tsk_info not found\n");
				return;
			}

			c2ps_info_lock(&tsk_info->mlock);
			tsk_info->default_uclamp = critical_task_uclamp[task_idx];
			c2ps_info_unlock(&tsk_info->mlock);
		}
	}
}

static inline void update_short_period_idle_rate(
	struct per_cpu_idle_rate *idle_rate, u64 *cur_idle_time, u64 *cur_wall_time)
{
	if (unlikely(idle_rate == NULL))
		return;

	idle_rate->s_idle = (100 * (*cur_idle_time - idle_rate->s_idle_time)) /
				(*cur_wall_time - idle_rate->s_wall_time);

	idle_rate->s_idle_time = *cur_idle_time;
	idle_rate->s_wall_time = *cur_wall_time;
}

static inline void update_long_period_idle_rate(
	struct per_cpu_idle_rate *idle_rate, u64 *cur_idle_time, u64 *cur_wall_time)
{
	if (unlikely(idle_rate == NULL))
		return;

	idle_rate->l_idle = (100 * (*cur_idle_time - idle_rate->l_idle_time)) /
				(*cur_wall_time - idle_rate->l_wall_time);

	idle_rate->l_idle_time = *cur_idle_time;
	idle_rate->l_wall_time = *cur_wall_time;
}

static inline bool need_update_long_period_idle_rate(
	struct per_cpu_idle_rate *idle_rate)
{
	return likely(idle_rate)? (++idle_rate->counter) % 2 : false;
}

void update_cpu_idle_rate(void)
{
	u64 _idle_time, _wall_time;
	unsigned int _cpu_index = 0, _cluster_index = 0;
	unsigned int _num_of_cpu[MAX_NUMBER_OF_CLUSTERS] = {0};
	unsigned int _l_sum_of_idlerate[MAX_NUMBER_OF_CLUSTERS] = {0};
	unsigned int _s_sum_of_idlerate[MAX_NUMBER_OF_CLUSTERS] = {0};
	unsigned int _total_num_of_cpu = 0;
	unsigned int _total_idlerate = 0;
	bool _dangerous_idle_rate_state = false;
	int _alert = 0;
	bool _need_update_long_period = false;

	if (unlikely(!glb_info))
		return;

	_alert = glb_info->overwrite_idle_alert > 0 ?
			glb_info->overwrite_idle_alert : background_idlerate_alert;
	C2PS_LOGD("check idle rate alert: %d", _alert);

	glb_info->is_cpu_boost = false;

	// Only timer callback will call this function, shouldn't lock
	for (; _cpu_index < MAX_CPU_NUM; _cpu_index++) {
		struct per_cpu_idle_rate *idle_rate =
			&glb_info->cpu_idle_rates[_cpu_index];
		int _cluster_idx = 0;
		_idle_time = get_cpu_idle_time(_cpu_index, &_wall_time, 1);

		_cluster_idx = topology_cluster_id(_cpu_index);

		update_short_period_idle_rate(idle_rate, &_idle_time, &_wall_time);

		if (need_update_long_period_idle_rate(idle_rate)) {
			update_long_period_idle_rate(idle_rate, &_idle_time, &_wall_time);
			C2PS_LOGD("check l_idle rate: %u for cpu: %d", idle_rate->l_idle, _cpu_index);
			c2ps_main_systrace("check l_idle rate: %u for cpu: %d",
										idle_rate->l_idle, _cpu_index);
			_l_sum_of_idlerate[_cluster_idx] += idle_rate->l_idle;
			_total_idlerate += idle_rate->l_idle;
			_need_update_long_period = true;
		}

		_num_of_cpu[_cluster_idx]++;
		_total_num_of_cpu++;
	}

	if (_need_update_long_period) {
		if (ignore_bcpu_idle_rate) {
			_total_idlerate -= _l_sum_of_idlerate[2];
			_total_num_of_cpu -= c2ps_get_nr_cpus_of_cluster(2);
		}
		if (_total_idlerate < background_idlerate_dangerous * _total_num_of_cpu &&
			glb_info->last_sum_idle_rate <
							background_idlerate_dangerous * _total_num_of_cpu) {
			_dangerous_idle_rate_state = true;
			c2ps_main_systrace("current l_idle rate:%d touches dangerous l_idle rate",
								_total_idlerate/_total_num_of_cpu);
			C2PS_LOGD("current l_idle rate:%d touches dangerous l_idle rate",
								_total_idlerate/_total_num_of_cpu);
		}
		glb_info->last_sum_idle_rate = _total_idlerate;
	}

	for (; _cluster_index < c2ps_nr_clusters; _cluster_index++) {
		u64 cur_cpu_freq = c2ps_get_cur_cpu_freq(
					c2ps_get_first_cpu_of_cluster(_cluster_index));
		u32 cur_cpu_floor = c2ps_get_cur_cpu_freq_floor(
					c2ps_get_first_cpu_of_cluster(_cluster_index));
		bool is_cpu_boost = false;

		if (c2ps_boost_cur_uclamp_max(_cluster_index, cur_cpu_floor, glb_info))
			continue;

		glb_info->scn_cpu_freq_floor[_cluster_index] =
			min(cur_cpu_floor, glb_info->scn_cpu_freq_floor[_cluster_index]);
		is_cpu_boost =
			(cur_cpu_floor > glb_info->scn_cpu_freq_floor[_cluster_index] ||
			cur_cpu_floor > glb_info->possible_config_cpu_freq[_cluster_index]);

		if (is_cpu_boost) {
			C2PS_LOGD("is_cpu_boost");
			c2ps_main_systrace("cpu boost: %u", cur_cpu_floor);
			glb_info->is_cpu_boost = true;
			glb_info->need_update_bg[0] = 1;
		}

		glb_info->s_loadxfreq[_cluster_index] =
			(100-_s_sum_of_idlerate[_cluster_index]/ _num_of_cpu[_cluster_index])
			* cur_cpu_freq;

		if (_need_update_long_period) {
			glb_info->avg_cluster_idle_rate[_cluster_index] =
				_l_sum_of_idlerate[_cluster_index]/_num_of_cpu[_cluster_index];
			// per-gear uclamp max only reference cpu l_idle rate
			if (_dangerous_idle_rate_state) {
				glb_info->need_update_bg[0] = 1;
				glb_info->need_update_bg[1 + _cluster_index] = 2;
			} else if (glb_info->avg_cluster_idle_rate[_cluster_index] < _alert) {
				glb_info->need_update_bg[0] = 1;
				glb_info->need_update_bg[1 + _cluster_index] = 1;
				c2ps_main_systrace("cluster: %u touches alert l_idle rate",
										_cluster_index);
				C2PS_LOGD("cluster: %u touches alert l_idle rate",
										_cluster_index);
			} else if ((!c2ps_um_mode_on && (glb_info->curr_max_uclamp[_cluster_index] >
						glb_info->max_uclamp[_cluster_index])) ||
					(c2ps_um_mode_on && (glb_info->curr_um_idle >= c2ps_regulator_um_min))) {
				glb_info->need_update_bg[0] = 1;
				if (glb_info->avg_cluster_idle_rate[_cluster_index] > _alert * 2)
					glb_info->need_update_bg[1 + _cluster_index] = -2;
				else
					glb_info->need_update_bg[1 + _cluster_index] = -1;
			}

			glb_info->l_loadxfreq[_cluster_index] =
				(100 - glb_info->avg_cluster_idle_rate[_cluster_index]) * cur_cpu_freq;

			// FIXME: only for current debug, remove it later
			C2PS_LOGD("check l_idle average: %d, cluster: %d, loading*freq: %llu, freq: %llu",
				glb_info->avg_cluster_idle_rate[_cluster_index], _cluster_index,
				glb_info->l_loadxfreq[_cluster_index], cur_cpu_freq);
		}
	}

	c2ps_bg_info_systrace(
		"cluster_0_util=%d cluster_1_util=%d cluster_2_util=%d "
		"cluster_0_freq=%ld cluster_1_freq=%ld cluster_2_freq=%ld",
		glb_info->curr_max_uclamp[0], glb_info->curr_max_uclamp[1],
		glb_info->curr_max_uclamp[2],
		c2ps_get_cluster_uclamp_freq(0, glb_info->curr_max_uclamp[0]),
		c2ps_get_cluster_uclamp_freq(1, glb_info->curr_max_uclamp[1]),
		c2ps_get_cluster_uclamp_freq(2, glb_info->curr_max_uclamp[2]));
}

bool need_update_background(void)
{
	if (unlikely(!glb_info))
		return false;

	if (c2ps_placeholder) {
		decide_special_uclamp_max(c2ps_placeholder);
		if (!is_release_uclamp_max) {
			c2ps_info_lock(&glb_info->mlock);
			{
				short _idx = 0;

				for (; _idx < c2ps_nr_clusters; _idx++) {
					glb_info->recovery_uclamp_max[_idx] =
								glb_info->curr_max_uclamp[_idx];
				}

				glb_info->use_uclamp_max_floor = true;
			}
			c2ps_info_unlock(&glb_info->mlock);
			is_release_uclamp_max = true;
			return true;
		}
	} else if (use_overwrite_uclamp_max()) {
		size_t cluster_size = c2ps_nr_clusters * sizeof(int);

		c2ps_info_lock(&glb_info->mlock);
		memcpy(glb_info->uclamp_max_floor, glb_info->overwrite_uclamp_max, cluster_size);
		c2ps_info_unlock(&glb_info->mlock);

		if (!is_release_uclamp_max) {
			c2ps_info_lock(&glb_info->mlock);
			{
				short _idx = 0;

				for (; _idx < c2ps_nr_clusters; _idx++) {
					glb_info->recovery_uclamp_max[_idx] =
								glb_info->curr_max_uclamp[_idx];
				}

				glb_info->use_uclamp_max_floor = true;
			}
			c2ps_info_unlock(&glb_info->mlock);
			is_release_uclamp_max = true;
			return true;
		}
	}

	if (is_release_uclamp_max)
		reset_need_update_status();

	return (glb_info->need_update_bg[0] &&
			!(c2ps_um_mode_on && glb_info->has_anchor_spec));
}

void reset_need_update_status(void)
{
	if (unlikely(!glb_info))
		return;

	if (is_release_uclamp_max) {
		if (!c2ps_placeholder && !use_overwrite_uclamp_max()) {
			int _recovery[MAX_NUMBER_OF_CLUSTERS] = {0};
			short _cluster_idx = 0;

			for (; _cluster_idx < c2ps_nr_clusters; _cluster_idx++) {
				_recovery[_cluster_idx] =
					max(glb_info->recovery_uclamp_max[_cluster_idx],
						glb_info->max_uclamp[_cluster_idx]);
				if (recovery_uclamp_max_immediately)
					set_gear_uclamp_max(_cluster_idx, _recovery[_cluster_idx]);
			}

			c2ps_info_lock(&glb_info->mlock);

			if (recovery_uclamp_max_immediately) {
				short _idx = 0;

				for (; _idx < c2ps_nr_clusters; _idx++) {
					glb_info->curr_max_uclamp[_idx] = _recovery[_idx];
					glb_info->recovery_uclamp_max[_idx] = 0;
				}
			}

			glb_info->use_uclamp_max_floor = false;
			c2ps_info_unlock(&glb_info->mlock);
			is_release_uclamp_max = false;
		}
	} else {
		glb_info->need_update_bg[0] = 0;
	}
}

inline bool use_overwrite_uclamp_max(void)
{
	short _cluster_idx = 0;

	if (unlikely(!glb_info))
		return false;

	for (; _cluster_idx < c2ps_nr_clusters; _cluster_idx++) {
		if (glb_info->overwrite_uclamp_max[_cluster_idx] !=
			glb_info->max_uclamp[_cluster_idx])
			return true;
	}
	return false;
}

inline void c2ps_set_util_margin(int cluster, int um)
{
	int margin = 100 - (10000/um);
	int cpu;
	unsigned int virtual_ceiling_freq;
	struct cpumask *cpus;

	if (unlikely(cluster < 0 || cluster >= c2ps_nr_clusters))
		return;

	cpus = get_gear_cpumask(cluster);

	if (!cpus)
		return;

	cpu = cpumask_first(cpus);
	virtual_ceiling_freq = c2ps_get_um_virtual_ceiling(cpu, um);
	C2PS_LOGD("check util margin:%d, cluster%d virtual ceiling:%u",
			margin, cluster, virtual_ceiling_freq);
	for_each_cpu(cpu, cpus) {
		set_target_margin_low(cpu, margin);
		set_target_margin(cpu, 0);
		set_turn_point_freq(cpu, virtual_ceiling_freq);
	}
}

inline void c2ps_set_turn_point_freq(int cluster, unsigned int freq)
{
	int cpu;
	struct cpumask *cpus;

	if (unlikely(cluster < 0 || cluster >= c2ps_nr_clusters))
		return;

	cpus = get_gear_cpumask(cluster);
	for_each_cpu(cpu, cpus)
		set_turn_point_freq(cpu, freq);
}

void set_eas_setting(void)
{
	if (likely(pre_eas_settings == NULL))
		pre_eas_settings = kmalloc(sizeof(*pre_eas_settings), GFP_KERNEL);
	if (unlikely(!pre_eas_settings)) {
		C2PS_LOGE("pre_eas_settings OOM\n");
		return;
	}

	pre_eas_settings->flt_ctrl_force = flt_ctrl_force_get();
	pre_eas_settings->group_get_mode = group_get_mode();
	pre_eas_settings->grp_dvfs_ctrl = get_grp_dvfs_ctrl();
	pre_eas_settings->ignore_idle_ctrl = get_ignore_idle_ctrl();

	// set flt off, ignore idle on
	flt_ctrl_force_set(1);
	group_set_mode(0);
	set_grp_dvfs_ctrl(0);
	set_ignore_idle_ctrl(1);
}

void reset_eas_setting(void)
{
	if (likely(!pre_eas_settings))
		return;

	flt_ctrl_force_set(0);
	group_set_mode(pre_eas_settings->group_get_mode);
	set_grp_dvfs_ctrl(pre_eas_settings->grp_dvfs_ctrl);
	set_ignore_idle_ctrl(pre_eas_settings->ignore_idle_ctrl);
}


void c2ps_set_vip_task(int pid, int vip_prior,
					unsigned int vip_throttle_time __maybe_unused)
{
	#ifdef NEW_C2PS_API_K66
		switch (vip_prior) {
		case 0:
			set_task_basic_vip_and_throttle(pid, vip_throttle_time);
			break;
		case 1:
		case 2:
		case 3:
			set_task_priority_based_vip_and_throttle(
				pid, vip_prior, vip_throttle_time);
			break;
		case 4:
			set_task_vvip_and_throttle(pid, vip_throttle_time);
			break;
		case -2:
			if (is_task_vip(pid)) {
				C2PS_LOGD("thread: %d unset VIP", pid);
				c2ps_unset_vip_task(pid);
			}
			break;
		default:
			break;
		}
	#else
		switch (vip_prior) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			set_task_basic_vip(pid);
			break;
		case -2:
			C2PS_LOGD("thread: %d unset VIP", pid);
			unset_task_basic_vip(pid);
			break;
		default:
			break;
		}
	#endif
}

inline bool is_task_vip(int pid)
{
	#ifdef NEW_C2PS_API_K66
		struct task_struct *p;
		int vip_prio = -1;
		bool task_vip = false;

		rcu_read_lock();
		p = find_task_by_vpid(pid);
		if (p) {
			get_task_struct(p);
			vip_prio = get_vip_task_prio(p);
			if (prio_is_vip(vip_prio, -1))
				task_vip = true;
			put_task_struct(p);
		}
		rcu_read_unlock();
		return task_vip;
	#else
		return false;
	#endif
}

#ifdef NEW_C2PS_API_K66
inline void c2ps_unset_vip_task(int pid)
{
	int vip_prior = get_vip_task_prio_by_pid(pid);

	switch (vip_prior) {
	case 0:
		unset_task_basic_vip(pid);
		break;
	case 1:
	case 2:
	case 3:
		unset_task_priority_based_vip(pid);
		break;
	case 4:
		unset_task_vvip(pid);
		break;
	default:
		break;
	}
}

inline int get_vip_task_prio_by_pid(int pid)
{
	struct task_struct *p;
	int vip_prior = -1;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	if (p) {
		get_task_struct(p);
		vip_prior = get_vip_task_prio(p);
		put_task_struct(p);
	}
	rcu_read_unlock();
	return vip_prior;
}
#endif

void c2ps_set_ineff_cpu_freq_ceiling(int cluster, int ineff_cpu_ceiling_freq)
{
	int cpu;
	struct cpumask *cpus;
	struct cpufreq_policy *policy;

	if (unlikely(glb_info == NULL || cluster >= c2ps_nr_clusters || ineff_cpu_ceiling_freq <= 0))
		return;

	glb_info->ineff_cpu_freq[cluster] = ineff_cpu_ceiling_freq;

	cpus = get_gear_cpumask(cluster);
	for_each_cpu(cpu, cpus) {
		policy = cpufreq_cpu_get(cpu);
		if (policy && policy->cpu == cpu) {
			int ret = freq_qos_add_request(&policy->constraints,
				&glb_info->qos_req[cluster], FREQ_QOS_MAX, ineff_cpu_ceiling_freq);

			if (ret < 0) {
				C2PS_LOGW("Fail to set cluster %d cpu ceiling %d",
					cluster, ineff_cpu_ceiling_freq);
			} else {
				C2PS_LOGD("set cluster %d cpu ceiling %d",
					cluster, ineff_cpu_ceiling_freq);
			}
			break;
		}
	}
}

inline void c2ps_update_cpu_freq_ceiling(int cluster, int cpu_ceiling_freq)
{
	if (unlikely(glb_info == NULL || cluster >= c2ps_nr_clusters || cpu_ceiling_freq <= 0))
		return;
	if (freq_qos_request_active(&glb_info->qos_req[cluster])) {
		c2ps_main_systrace("update_cpu_freq_ceiling: cluster=%d freq=%d",
						cluster, cpu_ceiling_freq);
		freq_qos_update_request(&glb_info->qos_req[cluster], cpu_ceiling_freq);
	}
}

inline void c2ps_reset_cpu_freq_ceiling(int cluster)
{
	if (unlikely(glb_info == NULL || cluster >= c2ps_nr_clusters ||
		glb_info->ineff_cpu_freq[cluster] <= 0))
		return;
	if (freq_qos_request_active(&glb_info->qos_req[cluster])) {
		c2ps_main_systrace("reset_cpu_freq_ceiling: cluster=%d freq=%d",
					cluster, glb_info->ineff_cpu_freq[cluster]);
		freq_qos_update_request(&glb_info->qos_req[cluster], glb_info->ineff_cpu_freq[cluster]);
	}
}

inline void c2ps_remove_qos_setting(void)
{
	int _cluster;

	if (unlikely(glb_info == NULL))
		return;

	for (_cluster = 0; _cluster < c2ps_nr_clusters; _cluster++) {
		if (freq_qos_request_active(&glb_info->qos_req[_cluster]))
			freq_qos_remove_request(&glb_info->qos_req[_cluster]);
	}
}

inline void cache_possible_config_cpu_freq_info(void)
{
	int _cluster_index = 0;

	if (unlikely(glb_info == NULL))
		return;

	for (; _cluster_index < c2ps_nr_clusters; _cluster_index++) {
		int first_cpu = c2ps_get_first_cpu_of_cluster(_cluster_index);
		unsigned long _util = 0;

		glb_info->possible_config_cpu_freq[_cluster_index] = INT_MAX;

		if (unlikely(first_cpu == -1))
			continue;

		_util = pd_get_freq_util(first_cpu, INT_MAX) * 70 / 100;

		glb_info->possible_config_cpu_freq[_cluster_index] = pd_get_util_freq(first_cpu, _util);
		C2PS_LOGD("possible_config_cpu_freq cpu: %d freq %u",
			first_cpu, glb_info->possible_config_cpu_freq[_cluster_index]);
	}
}

static ssize_t task_info_show(struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	struct c2ps_task_info *tsk_info;
	char *temp = NULL;
	int pos = 0;
	int length = 0;
	int bkt = 0;

	temp = kcalloc(C2PS_SYSFS_MAX_BUFF_SIZE, sizeof(char), GFP_KERNEL);
	if (unlikely(!temp))
		goto out;

	length = scnprintf(temp + pos, C2PS_SYSFS_MAX_BUFF_SIZE - pos,
		"\nTASKID\tPID\tTASK_NAME\tINIT_UCLAMP\tTASK_TARGET_TIME\tVIP_TASK\t");
	pos += length;
	length = scnprintf(temp + pos, C2PS_SYSFS_MAX_BUFF_SIZE - pos,
		"START_TIME\tEND_TIME\tPROC_TIME\tEXEC_TIME\tLATEST_UCLAMP\n");
	pos += length;

	c2ps_task_info_tbl_lock(__func__);

	hash_for_each(task_info_tbl, bkt, tsk_info, hlist) {
		length = scnprintf(temp + pos,
			C2PS_SYSFS_MAX_BUFF_SIZE - pos,
			"%-2d\t%-5d\t%*s\t%-4u\t\t%-8llu\t\t%d\t\t",
			tsk_info->task_id, tsk_info->pid,
			-MAX_TASK_NAME_SIZE, tsk_info->task_name,
			tsk_info->default_uclamp, tsk_info->task_target_time,
			tsk_info->is_vip_task);
		pos += length;
		length = scnprintf(temp + pos,
			C2PS_SYSFS_MAX_BUFF_SIZE - pos,
			"%-12llu\t%-12llu\t%-10llu\t%-10llu\t%-4d\n",
			tsk_info->start_time, tsk_info->end_time,
			tsk_info->proc_time, tsk_info->real_exec_runtime,
			tsk_info->latest_uclamp);
		pos += length;
	}

	c2ps_task_info_tbl_unlock(__func__);

	length = scnprintf(buf, PAGE_SIZE, "%s", temp);

out:
	kfree(temp);
	return length;
}

static KOBJ_ATTR_RO(task_info);

static ssize_t gear_uclamp_max_store(struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	int gearid = -1;
	int val = -1;
	char *buffer = NULL;

	buffer = kcalloc(C2PS_SYSFS_MAX_BUFF_SIZE, sizeof(char), GFP_KERNEL);
	if (unlikely(!buffer))
		goto out;

	if ((count > 0) && (count < C2PS_SYSFS_MAX_BUFF_SIZE)) {
		if (scnprintf(buffer, C2PS_SYSFS_MAX_BUFF_SIZE, "%s", buf)) {
			if (sscanf(buffer, "%d %d", &gearid, &val) != 2)
				goto out;
		}
	}

	if (unlikely(gearid < 0 || gearid >= get_nr_gears() || val < 0))
		goto out;

	set_gear_uclamp_max(gearid, val);

out:
	kfree(buffer);
	return count;
}

static ssize_t gear_uclamp_max_show(struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	int length = 0;

	for (int i = 0; i < get_nr_gears(); ++i) {
		length += scnprintf(buf + length, PAGE_SIZE - length,
				"gear: %d, uclamp_max: %d \n",
				i, get_gear_uclamp_max(i));
	}

	return length;
}

static KOBJ_ATTR_RW(gear_uclamp_max);

int init_c2ps_common(int cfg_camfps)
{
	int ret = 0;
	u8 _cluster_idx = 0;

	hash_init(task_info_tbl);
	hash_init(task_group_info_tbl);
	hash_init(anchor_tbl);
	glb_info = kzalloc(sizeof(*glb_info), GFP_KERNEL);

	if (unlikely(!glb_info)) {
		C2PS_LOGE("OOM\n");
		return -ENOMEM;
	}

	mutex_init(&glb_info->mlock);

	for (; _cluster_idx < c2ps_nr_clusters; _cluster_idx++) {
		c2ps_init_kf(&(glb_info->slow_lxf_est[_cluster_idx]), LxF_S_KF_QVAL,
			LxF_KF_MEAS_ERR, LxF_KF_MIN_EST_ERR);
		c2ps_init_kf(&(glb_info->fast_lxf_est[_cluster_idx]), LxF_F_KF_QVAL,
			LxF_KF_MEAS_ERR, LxF_KF_MIN_EST_ERR);
	}

	ret = c2ps_sysfs_create_dir(NULL, "common", &common_base_kobj);

	if (!ret) {
		c2ps_sysfs_create_file(common_base_kobj, &kobj_attr_task_info);
		c2ps_sysfs_create_file(common_base_kobj, &kobj_attr_gear_uclamp_max);
	}

	set_glb_info_bg_uclamp_max();
	set_glb_info_bg_util_margin();
	set_config_camfps(cfg_camfps);

	return ret;
}

void exit_c2ps_common(void)
{
	c2ps_clear_task_info_table();
	c2ps_clear_task_group_info_table();
	c2ps_clear_anchor_table();
	c2ps_remove_qos_setting();
	kfree(glb_info);
	glb_info = NULL;

	if (likely(pre_eas_settings != NULL)) {
		kfree(pre_eas_settings);
		pre_eas_settings = NULL;
	}

	is_release_uclamp_max = false;
	c2ps_sysfs_remove_file(common_base_kobj, &kobj_attr_task_info);
	c2ps_sysfs_remove_file(common_base_kobj, &kobj_attr_gear_uclamp_max);
	c2ps_sysfs_remove_dir(&common_base_kobj);
}
