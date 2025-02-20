// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "c2ps_regulator_policy.h"
#include <linux/math64.h>
#include <linux/string.h>


static unsigned int c2ps_regulator_debug_max_uclamp = 1000;
static unsigned int c2ps_regulator_debug_min_uclamp = 1000;
static unsigned int c2ps_regulator_base_update_uclamp;
static unsigned int c2ps_uclamp_up_margin;
static unsigned int c2ps_uclamp_down_margin;
static unsigned int c2ps_regulator_bg_update_uclamp = 20;
static unsigned int c2ps_regulator_bg_update_uclamp_fast;
static unsigned int c2ps_uclamp_bg_up_margin_cluster0 = 1000;
static unsigned int c2ps_uclamp_bg_up_margin_cluster1 = 1000;
static unsigned int c2ps_uclamp_bg_up_margin_cluster2 = 1000;
static unsigned int c2ps_max_cpu_idle_rate = 50;

/**************************************************************************/
int c2ps_regulator_base_update_um = 5;
int c2ps_regulator_um_min = 65;
int c2ps_lcore_mcore_um_ratio = 10;
static int c2ps_regulator_um_max = 125;
static int c2ps_fix_um;
static int c2ps_converge_target = 50;
static int c2ps_um_monitor;
static int c2ps_safe_idle_rate = 7;
static bool skip_jitter;
static int lat_th = 1500;
/**************************************************************************/



module_param(c2ps_regulator_debug_max_uclamp, int, 0644);
module_param(c2ps_regulator_debug_min_uclamp, int, 0644);
module_param(c2ps_regulator_base_update_uclamp, int, 0644);
module_param(c2ps_uclamp_up_margin, int, 0644);
module_param(c2ps_uclamp_down_margin, int, 0644);
module_param(c2ps_regulator_bg_update_uclamp, int, 0644);
module_param(c2ps_regulator_bg_update_uclamp_fast, int, 0644);
module_param(c2ps_uclamp_bg_up_margin_cluster0, int, 0644);
module_param(c2ps_uclamp_bg_up_margin_cluster1, int, 0644);
module_param(c2ps_uclamp_bg_up_margin_cluster2, int, 0644);
module_param(c2ps_max_cpu_idle_rate, int, 0644);
module_param(c2ps_regulator_base_update_um, int, 0644);
module_param(c2ps_regulator_um_min, int, 0644);
module_param(c2ps_regulator_um_max, int, 0644);
module_param(c2ps_converge_target, int, 0644);
module_param(c2ps_safe_idle_rate, int, 0644);
module_param(skip_jitter, bool, 0644);
module_param(lat_th, int, 0644);

/**************************************************************************/
module_param(c2ps_fix_um, int, 0644);
module_param(c2ps_lcore_mcore_um_ratio, int, 0644);
module_param(c2ps_um_monitor, int, 0644);
/**************************************************************************/

/**
 * @brief      map to C2PS_REGULATOR_MODE_FIX
 */
void c2ps_regulator_policy_fix_uclamp(struct regulator_req *req)
{
	C2PS_LOGD("task_id (%d) use fix uclamp policy", req->tsk_info->task_id);
	req->tsk_info->latest_uclamp = req->tsk_info->default_uclamp;
	set_uclamp(req->tsk_info->pid,
		req->tsk_info->default_uclamp,
		req->tsk_info->default_uclamp);

	if (unlikely(req->tsk_info->is_enable_dep_thread)) {
		int _i = 0;

		for (; _i < MAX_DEP_THREAD_NUM; _i++) {
			if (req->tsk_info->dep_thread[_i] <= 0)
				break;
			C2PS_LOGD("thread (%d) set dep thread: %d",
				req->tsk_info->pid, req->tsk_info->dep_thread[_i]);
			set_uclamp(req->tsk_info->dep_thread[_i],
				req->tsk_info->default_uclamp,
				req->tsk_info->default_uclamp);
		}
	}
}

/**
 * @brief      map to C2PS_REGULATOR_MODE_SIMPLE
 * Tuning Param:
 *	- c2ps_regulator_base_update_uclamp
 *	- c2ps_uclamp_down_margin
 *	- c2ps_uclamp_up_margin
 */
void c2ps_regulator_policy_simple(struct regulator_req *req)
{
	u64 average_proc_time = req->tsk_info->hist_proc_time_sum /
							proc_time_window_size;
	s64 update_ratio_numer =
		(s64)average_proc_time - (s64)req->tsk_info->task_target_time;
	s64 update_ratio_denom = req->tsk_info->task_target_time;
	s64 update_uclamp = 0;
	int new_uclamp = 0;

	C2PS_LOGD("task_id (%d) use simple uclamp policy",
		  req->tsk_info->task_id);

	if (unlikely(!req->tsk_info->task_target_time)) {
		C2PS_LOGD("task target time equals to zero");
		return;
	}

	C2PS_LOGD(
		"check task_target_time: %lld, proc_time: %llu, proc_time_avg: %llu, diff: %lld task_id: %d",
		 req->tsk_info->task_target_time, req->tsk_info->proc_time,
		 average_proc_time, update_ratio_numer,
		 req->tsk_info->task_id);

	if (unlikely(req->tsk_info->latest_uclamp <= 0))
		req->tsk_info->latest_uclamp = req->tsk_info->default_uclamp;

	update_ratio_numer =
		c2ps_regulator_base_update_uclamp * update_ratio_numer * 100;
	update_uclamp =
		div64_s64(update_ratio_numer, update_ratio_denom) / 100;

	new_uclamp = (int)req->tsk_info->latest_uclamp + (int)update_uclamp;

	/* limit new_uclamp in default uclamp +- deviation */
	new_uclamp = max((int)(req->tsk_info->default_uclamp *
						  (100 - c2ps_uclamp_down_margin) / 100),
				 min((int)(req->tsk_info->default_uclamp *
						  (100 + c2ps_uclamp_up_margin) / 100),
				 new_uclamp));

	new_uclamp = new_uclamp * 100 / req->curr_um;
	new_uclamp = max(0, min(1024, new_uclamp));
	req->tsk_info->latest_uclamp = new_uclamp;


	set_uclamp(req->tsk_info->pid,
		req->tsk_info->latest_uclamp,
		req->tsk_info->latest_uclamp);

	if (unlikely(req->tsk_info->is_enable_dep_thread)) {
		int _i = 0;

		for (; _i < MAX_DEP_THREAD_NUM; _i++) {
			if (req->tsk_info->dep_thread[_i] <= 0)
				break;
			C2PS_LOGD("thread (%d) set dep thread: %d",
				req->tsk_info->pid, req->tsk_info->dep_thread[_i]);
			set_uclamp(req->tsk_info->dep_thread[_i],
				req->tsk_info->latest_uclamp,
				req->tsk_info->latest_uclamp);
		}
	}

	/* debug tool tag */
	c2ps_main_systrace(
		"c2ps simple policy: task: %d average_proc_time: %llu, realtime_proc_time: %llu",
		req->tsk_info->task_id, average_proc_time,
		req->tsk_info->proc_time);
}

/**
 * @brief      map to C2PS_REGULATOR_MODE_DEBUG
 * Tuning Param:
 *	- c2ps_regulator_debug_max_uclamp
 *	- c2ps_regulator_debug_min_uclamp
 */
void c2ps_regulator_policy_debug_uclamp(struct regulator_req *req)
{
	C2PS_LOGD("task_id (%d) use fix uclamp policy", req->tsk_info->task_id);
	req->tsk_info->latest_uclamp = c2ps_regulator_debug_max_uclamp;
	set_uclamp(req->tsk_info->pid,
		c2ps_regulator_debug_max_uclamp,
		c2ps_regulator_debug_min_uclamp);

	if (unlikely(req->tsk_info->is_enable_dep_thread)) {
		int _i = 0;

		for (; _i < MAX_DEP_THREAD_NUM; _i++) {
			if (req->tsk_info->dep_thread[_i] <= 0)
				break;
			C2PS_LOGD("thread (%d) set dep thread: %d",
				req->tsk_info->pid, req->tsk_info->dep_thread[_i]);
			set_uclamp(req->tsk_info->dep_thread[_i],
				c2ps_regulator_debug_max_uclamp,
				c2ps_regulator_debug_max_uclamp);
		}
	}
}

/**
 * @brief      map to C2PS_REGULATOR_BGMODE_SIMPLE
 * Tuning Param:
 *  - c2ps_uclamp_bg_up_margin
 *  - c2ps_regulator_bg_update_uclamp
 *  - c2ps_uclamp_bg_up_margin_cluster0
 *  - c2ps_uclamp_bg_up_margin_cluster1
 *  - c2ps_uclamp_bg_up_margin_cluster2
 */
void c2ps_regulator_bgpolicy_simple(struct regulator_req *req)
{
	int cluster_index = 0;
	unsigned int *_bg_uclamp_up_margin[3] = {
			&c2ps_uclamp_bg_up_margin_cluster0,
			&c2ps_uclamp_bg_up_margin_cluster1,
			&c2ps_uclamp_bg_up_margin_cluster2};

	if (unlikely(!req->glb_info))
		return;

	for (; cluster_index < c2ps_nr_clusters; cluster_index++) {
		int *_cur_bg_uclamp = &(req->glb_info->curr_max_uclamp[cluster_index]);
		int cpu = c2ps_get_first_cpu_of_cluster(cluster_index);
		int _uclamp_max_floor = req->glb_info->use_uclamp_max_floor?
							req->glb_info->uclamp_max_floor[cluster_index]:0;
		int *_uclamp_max_ceiling = &(req->glb_info->uclamp_max_ceiling[cluster_index]);
		int _max_uclamp_max = (int)(req->glb_info->max_uclamp[cluster_index]
				      * (100 + (*_bg_uclamp_up_margin[cluster_index])) / 100);

		C2PS_LOGD("cluster: %d, uclamp_max_floor: %d", cluster_index, _uclamp_max_floor);

		if (unlikely(cpu < 0))
			continue;

		if ((*_bg_uclamp_up_margin[cluster_index]) == 0 &&
			 req->glb_info->need_update_bg[1 + cluster_index] >= 0 &&
			 !req->glb_info->special_uclamp_max[cluster_index])
			continue;

		if (req->glb_info->need_update_bg[1 + cluster_index] == 2) {
			*_uclamp_max_ceiling = MAX_UCLAMP;
			*_cur_bg_uclamp = MAX_UCLAMP;
		} else if (req->glb_info->need_update_bg[1 + cluster_index] == 1) {
			_max_uclamp_max = max(_max_uclamp_max, *_uclamp_max_ceiling);
			*_cur_bg_uclamp += c2ps_regulator_bg_update_uclamp;
			*_cur_bg_uclamp = min(*_cur_bg_uclamp, _max_uclamp_max);
		} else if (req->glb_info->need_update_bg[1 + cluster_index] == -2) {
			*_uclamp_max_ceiling = _max_uclamp_max;
			if (c2ps_regulator_bg_update_uclamp_fast <= 0)
				c2ps_regulator_bg_update_uclamp_fast =
									c2ps_regulator_bg_update_uclamp;
			*_cur_bg_uclamp -= c2ps_regulator_bg_update_uclamp_fast;
			*_cur_bg_uclamp = max(*_cur_bg_uclamp,
						req->glb_info->max_uclamp[cluster_index]);
		} else if (req->glb_info->need_update_bg[1 + cluster_index] == -1) {
			*_uclamp_max_ceiling = _max_uclamp_max;
			*_cur_bg_uclamp -= c2ps_regulator_bg_update_uclamp;
			*_cur_bg_uclamp = max(*_cur_bg_uclamp,
						req->glb_info->max_uclamp[cluster_index]);
		} else {
			continue;
		}
		*_cur_bg_uclamp = max(*_cur_bg_uclamp, _uclamp_max_floor);
		*_cur_bg_uclamp = min(*_cur_bg_uclamp, c2ps_get_cpu_max_uclamp(cpu));
		set_gear_uclamp_max(cluster_index, *_cur_bg_uclamp);
	}

	c2ps_bg_info_systrace(
		"cluster_0_util=%d cluster_1_util=%d cluster_2_util=%d cluster_0_freq=%ld cluster_1_freq=%ld cluster_2_freq=%ld",
		req->glb_info->curr_max_uclamp[0], req->glb_info->curr_max_uclamp[1],
		req->glb_info->curr_max_uclamp[2],
		c2ps_get_cluster_uclamp_freq(0, req->glb_info->curr_max_uclamp[0]),
		c2ps_get_cluster_uclamp_freq(1, req->glb_info->curr_max_uclamp[1]),
		c2ps_get_cluster_uclamp_freq(2, req->glb_info->curr_max_uclamp[2]));
	c2ps_main_systrace(
		"special_cluster_0_util=%d special_cluster_1_util=%d special_cluster_2_util=%d ",
		req->glb_info->special_uclamp_max[0], req->glb_info->special_uclamp_max[1],
		req->glb_info->special_uclamp_max[2]);
	C2PS_LOGD("debug: uclamp max: %d, %d, %d special uclamp max: %d, %d, %d",
			  req->glb_info->curr_max_uclamp[0],
			  req->glb_info->curr_max_uclamp[1],
			  req->glb_info->curr_max_uclamp[2],
			  req->glb_info->special_uclamp_max[0],
			  req->glb_info->special_uclamp_max[1],
			  req->glb_info->special_uclamp_max[2]);
}

/**
 * @brief      map to C2PS_REGULATOR_BGMODE_UM_STABLE_DEFAULT
 */
void c2ps_regulator_bgpolicy_um_stable_default(struct regulator_req *req)
{
	int cluster_index = 0;
	int curr_um = 0;
	bool decrease_um = true;
	bool dangerous_idle_rate = false;

	if (unlikely(!req->glb_info))
		return;

	curr_um = req->glb_info->curr_um_idle;

	for (; cluster_index < c2ps_nr_clusters; cluster_index++) {
		if (req->glb_info->need_update_bg[1 + cluster_index] == 2 ||
			req->glb_info->single_shot_enable_ineff_cpufreq_cnt)
			c2ps_update_cpu_freq_ceiling(cluster_index, FREQ_QOS_MAX_DEFAULT_VALUE);
		else
			c2ps_reset_cpu_freq_ceiling(cluster_index);
		if (req->glb_info->need_update_bg[1 + cluster_index] > 0)
			decrease_um = false;
		if (req->glb_info->need_update_bg[1 + cluster_index] == 2)
			dangerous_idle_rate = true;
	}

	if (decrease_um)
		curr_um -= c2ps_regulator_base_update_um;
	else
		curr_um += c2ps_regulator_base_update_um;

	if (dangerous_idle_rate)
		curr_um = max(curr_um, 100);

	curr_um = min(c2ps_regulator_um_max, max(curr_um, c2ps_regulator_um_min));
	c2ps_set_util_margin(0, curr_um);
	c2ps_set_util_margin(1, curr_um);
	c2ps_set_util_margin(2, curr_um);

	req->glb_info->curr_um_idle = curr_um;

	c2ps_bg_info_um_default_systrace("um=%d", curr_um);
	C2PS_LOGD("debug: um: %d ", curr_um);
}

static int _cal_latency_um(
	struct regulator_req *req,
	struct um_table_item *cur_item,
	struct um_table_item *prev_item)
{
	int latency_um = req->glb_info->curr_um;
	u64 est_latency_diff = cur_item->latency - prev_item->latency;
	u64 est_latency_1 = cur_item->latency + est_latency_diff;
	u64 est_latency_2 = cur_item->latency +  2 * est_latency_diff;
	u32 latency_spec = req->anc_info->latency_spec;
	int64_t converge_lat_val =
		(cur_item->lat_est.est_err - cur_item->lat_est.min_est_err) * 100 /
				cur_item->lat_est.min_est_err;

	if (unlikely(cur_item->latency > prev_item->latency + lat_th)) {
		prev_item->latency = (cur_item->latency + prev_item->latency)/2;
		prev_item->lat_est.est_val = prev_item->latency;
	}

	if (converge_lat_val > c2ps_converge_target) {
		C2PS_LOGD("latency not converge yet: %lld", converge_lat_val);
		goto skip_lat_um;
	}
	if (cur_item->latency >= prev_item->latency) {
		if (est_latency_1 < latency_spec &&
			est_latency_2 < latency_spec)
			latency_um -= c2ps_regulator_base_update_um;
		else if (est_latency_1 > latency_spec)
			latency_um += c2ps_regulator_base_update_um;
	} else {
		C2PS_LOGD("prev_latency is larger (%llu, %llu)",
			prev_item->latency, cur_item->latency);
		prev_item->latency = cur_item->latency;
	}

skip_lat_um:
	C2PS_LOGD(
		"check anchor %d latency: %llu prev_latency: %llu est_diff: %llu est_1: %llu est_2: %llu latency_spec: %u latency_um: %d, cur_um: %d, hit latency: %d",
		req->anc_info->anchor_id, cur_item->latency, prev_item->latency,
		est_latency_diff, est_latency_1, est_latency_2, latency_spec,
		latency_um, req->glb_info->curr_um, latency_um>req->glb_info->curr_um);

	return latency_um;
}

static int _cal_jitter_um(
	struct regulator_req *req,
	struct um_table_item *cur_item,
	struct um_table_item *prev_item)
{
	int jitter_um = req->glb_info->curr_um;

	cur_item->jitter_total_access++;
	if (cur_item->jitter < req->anc_info->jitter_spec) {
		bool decrease_jitter_um = true;
		int _possible_next_um = max(jitter_um - c2ps_regulator_base_update_um,
								c2ps_regulator_um_min);
		struct um_table_item *_possible_next_item =
				c2ps_find_um_table_by_um(req->anc_info, _possible_next_um);

		cur_item->jitter_hit_cnt = cur_item->jitter_hit_cnt > 0?
						cur_item->jitter_hit_cnt-1:cur_item->jitter_hit_cnt;

		if (likely(_possible_next_item &&
				_possible_next_item->jitter_total_access > 20)) {
			// FIXME: debug only, remove it laters
			C2PS_LOGD("check possible next um: %d, jitter pass rate: %llu",
				_possible_next_um,
				(_possible_next_item->jitter_hit_cnt*100)/
				_possible_next_item->jitter_total_access);

			// 33% chance jitter larger than spec
			if (_possible_next_item->jitter_hit_cnt >
				_possible_next_item->jitter_total_access/3) {
				decrease_jitter_um = false;
				if (cur_item->um_stay_cnt > 5)
					_possible_next_item->jitter_hit_cnt -= 3;
			}
		}

		if (decrease_jitter_um)
			jitter_um -= c2ps_regulator_base_update_um;
	} else {
		cur_item->jitter_hit_cnt++;

		if (likely(cur_item->jitter_total_access > 20)) {
			// 33% chance jitter larger than spec
			if (cur_item->jitter_hit_cnt > cur_item->jitter_total_access/3)
				jitter_um += c2ps_regulator_base_update_um;
		}
	}

	// reduce the jitter_total_access to prevent too large number
	if (unlikely(cur_item->jitter_total_access >= 100)) {
		cur_item->jitter_hit_cnt /= 2;
		cur_item->jitter_total_access /= 2;
	}

	// FIXME: debug only, reduce the necessary log later
	C2PS_LOGD(
		"check anchor %d jitter: %llu jitter_spec: %u jitter_um: %d cur_um: %d est_err: %lld jitter_hit_cnt: %llu (%llu, %llu) hit jitter: %d",
		req->anc_info->anchor_id, cur_item->jitter,
		req->anc_info->jitter_spec, jitter_um, req->glb_info->curr_um,
		cur_item->jit_est.est_err, cur_item->jitter_hit_cnt,
		cur_item->jitter_total_access,
		(cur_item->jitter_hit_cnt*100)/cur_item->jitter_total_access,
		jitter_um>req->glb_info->curr_um);

	return jitter_um;
}

static int _cal_idle_rate_um(
	struct regulator_req *req, bool *force_use_idle_rate_um)
{
	short _cluster_index = 0;
	bool is_safe_idle_rate = true;
	int idle_rate_um = req->glb_info->curr_um;
	bool is_dangerous_idle_rate = false;

	*force_use_idle_rate_um = true;

	for (; _cluster_index < c2ps_nr_clusters; _cluster_index++) {
		if (req->glb_info->need_update_bg[1 + _cluster_index] == 2 ||
			req->glb_info->single_shot_enable_ineff_cpufreq_cnt)
			c2ps_update_cpu_freq_ceiling(_cluster_index, FREQ_QOS_MAX_DEFAULT_VALUE);
		else
			c2ps_reset_cpu_freq_ceiling(_cluster_index);
		if (req->glb_info->avg_cluster_idle_rate[_cluster_index] <
			c2ps_safe_idle_rate) {
			is_safe_idle_rate = false;
		}
		if (req->glb_info->avg_cluster_idle_rate[_cluster_index] <
					c2ps_max_cpu_idle_rate) {
			*force_use_idle_rate_um = false;
		}
		if (req->glb_info->need_update_bg[1 + _cluster_index] == 2)
			is_dangerous_idle_rate = true;
	}

	if (!is_safe_idle_rate)
		idle_rate_um += c2ps_regulator_base_update_um;
	else
		idle_rate_um -= c2ps_regulator_base_update_um;

	if (is_dangerous_idle_rate)
		idle_rate_um = max(idle_rate_um, 100);

	return idle_rate_um;
}

/**
 * @brief      map to C2PS_REGULATOR_BGMODE_UM_STABLE
 */
void c2ps_regulator_bgpolicy_um_stable(struct regulator_req *req)
{
	int curr_um = req->glb_info->curr_um;
	int action_um = 0;
	int prev_um = min(curr_um + c2ps_regulator_base_update_um, c2ps_regulator_um_max);
	struct um_table_item *_item = c2ps_find_um_table_by_um(req->anc_info, curr_um);
	struct um_table_item *_prev_item = c2ps_find_um_table_by_um(req->anc_info, prev_um);
	bool need_update_um = true;

	if (unlikely(!_item))
		return;

	if (unlikely(c2ps_fix_um)) {
		action_um = c2ps_fix_um;
	} else {
		int latency_um = 0;
		int jitter_um = 0;
		int idle_rate_um = 0;
		bool force_use_idle_rate_um = false;

		if (likely(_item && _prev_item)) {
			idle_rate_um = _cal_idle_rate_um(req, &force_use_idle_rate_um);
			if (req->anc_info->latency_spec > 0)
				latency_um = _cal_latency_um(req, _item, _prev_item);
			if (req->anc_info->jitter_spec > 0 && !skip_jitter)
				jitter_um = _cal_jitter_um(req, _item, _prev_item);
		}

		action_um = max(idle_rate_um, max(latency_um, jitter_um));
		if (force_use_idle_rate_um) {
			action_um = idle_rate_um;
		}

		C2PS_LOGD(
			"force_use_idle_rate_um: %d, action_um: %d, idle_rate_um: %d, latency_um: %d, jitter_um: %d",
			force_use_idle_rate_um, action_um, idle_rate_um, latency_um, jitter_um);
		c2ps_main_systrace(
			"force_use_idle_rate_um: %d, action_um: %d, idle_rate_um: %d, latency_um: %d, jitter_um: %d",
			force_use_idle_rate_um, action_um, idle_rate_um, latency_um, jitter_um);

		if (action_um > curr_um) {
			if (req->glb_info->um_vote.vote_result > 0)
				need_update_um = false;
			req->glb_info->um_vote.vote_result = 1;
		} else if (action_um == curr_um) {
			if (req->glb_info->um_vote.vote_result < 0)
				req->glb_info->um_vote.vote_result = 0;
			need_update_um = false;
		} else if (action_um < curr_um) {
			need_update_um = false;
			if (req->glb_info->um_vote.vote_result < 0 &&
				req->anc_info->is_last_anchor)
				need_update_um = true;
		}
	}

	C2PS_LOGD("anchor_id=%d um=%d latency=%llu jitter=%llu est_err=%lld min_est_err=%lld",
			req->anc_info->anchor_id, req->glb_info->curr_um,
			_item->latency, _item->jitter,
			_item->lat_est.est_err, _item->lat_est.min_est_err);
	c2ps_bg_info_um_systrace(
			"stable state anchor_id=%d um=%d latency=%llu(%u) jitter=%llu(%u) est_err=%lld min_est_err=%lld",
			req->anc_info->anchor_id, req->glb_info->curr_um,
			_item->latency, req->anc_info->latency_spec, _item->jitter,
			req->anc_info->jitter_spec, _item->lat_est.est_err,
			_item->lat_est.min_est_err);

	if (need_update_um) {
		action_um = min(c2ps_regulator_um_max,
							max(action_um, c2ps_regulator_um_min));
		c2ps_set_util_margin(0, action_um);
		c2ps_set_util_margin(1, action_um*10/c2ps_lcore_mcore_um_ratio);
		c2ps_set_util_margin(2, action_um);

		req->glb_info->curr_um = action_um;
		c2ps_um_monitor = action_um;
		_item->um_stay_cnt = 0;
		C2PS_LOGD("anchor id: %d, update um to %d, is_last_anchor: %d, c2ps_lcore_mcore_um_ratio: %d",
			req->anc_info->anchor_id, action_um, req->anc_info->is_last_anchor, c2ps_lcore_mcore_um_ratio);
	} else {
		_item->um_stay_cnt++;
	}

	if (req->anc_info->is_last_anchor)
		req->glb_info->um_vote.vote_result = -1;
}

/**
 * @brief      map to C2PS_REGULATOR_BGMODE_UM_TRANSIENT
 */
void c2ps_regulator_bgpolicy_um_transient(struct regulator_req *req)
{
	int action_um = 125;
	int cluster_index = 0;

	if (unlikely(req->glb_info->overwrite_util_margin ||
				req->glb_info->decided_um_placeholder_val)) {
		action_um = max(req->glb_info->overwrite_util_margin,
						req->glb_info->decided_um_placeholder_val);
	}

	for (; cluster_index < c2ps_nr_clusters; cluster_index++) {
		if (req->glb_info->need_update_bg[1 + cluster_index] == 2 ||
			req->glb_info->single_shot_enable_ineff_cpufreq_cnt)
			c2ps_update_cpu_freq_ceiling(cluster_index, FREQ_QOS_MAX_DEFAULT_VALUE);
		else
			c2ps_reset_cpu_freq_ceiling(cluster_index);
	}

	c2ps_set_util_margin(0, action_um);
	c2ps_set_util_margin(1, action_um);
	c2ps_set_util_margin(2, action_um);
	C2PS_LOGD("transient state um=%d", action_um);
	c2ps_bg_info_um_systrace("transient state um=%d", action_um);

	req->glb_info->curr_um = max(req->glb_info->curr_um, 100);
	req->glb_info->curr_um_idle = max(req->glb_info->curr_um_idle, 100);
}
