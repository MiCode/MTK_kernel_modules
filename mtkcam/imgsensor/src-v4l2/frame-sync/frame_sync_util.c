// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "frame_sync_util.h"
#include "frame_monitor.h"


/******************************************************************************/
// Log message
/******************************************************************************/
#include "frame_sync_log.h"

#define REDUCE_FS_UTIL_LOG
#define PFX "FrameSyncUtil"
#define FS_LOG_DBG_DEF_CAT LOG_FS_UTIL
/******************************************************************************/





/******************************************************************************/
// Frame Sync Utilities function
/******************************************************************************/
void fs_util_calc_act_fl(const unsigned long long ts_arr[],
	unsigned int act_fl_arr[], const unsigned int arr_len,
	const unsigned int tick_factor)
{
	unsigned int i;

	/* error handling */
	if (unlikely((ts_arr == NULL) || (act_fl_arr == NULL)))
		return;

	for (i = 0; i < (arr_len - 1); ++i) {
		fs_timestamp_t tick_a, tick_b;

		/* check if tick factor is valid */
		if (unlikely(tick_factor == 0))
			break;

		/* update actual frame length by timestamp diff */
		tick_a = (tick_factor * ts_arr[i]);
		tick_b = (tick_factor * ts_arr[i+1]);

		act_fl_arr[i] = (tick_b != 0)
			? ((tick_a - tick_b) / tick_factor) : 0;
	}
}


void fs_util_tsrec_dynamic_msg_connector(const unsigned int idx,
	const unsigned int log_str_len, char *log_buf, int len,
	const char *caller)
{
#if defined(USING_TSREC)
	const struct mtk_cam_seninf_tsrec_timestamp_info
		*p_ts_info = frm_get_tsrec_timestamp_info_ptr(idx);
	const unsigned int exp_id = TSREC_1ST_EXP_ID;

	if (frm_get_ts_src_type() != FS_TS_SRC_TSREC)
		return;
	if (unlikely(p_ts_info == NULL)) {
		LOG_MUST(
			"[%s] ERROR: USING_TSREC timestamp, but get p_ts_info:%p, skip dump tsrec ts info\n",
			caller, p_ts_info);
		return;
	}

	/* print TSREC info */
	FS_SNPRF(log_str_len, log_buf, len,
		", tsrec[(%u/inf:%u,irq(%llu/%llu)(+%llu)):(0:(%llu/%llu/%llu/%llu)/1:(%llu/%llu/%llu/%llu)/2:(%llu/%llu/%llu/%llu))]",
		p_ts_info->tsrec_no, p_ts_info->seninf_idx,
		p_ts_info->irq_sys_time_ns/1000,
		p_ts_info->irq_tsrec_ts_us,
		(convert_tick_2_timestamp(
			(p_ts_info->tsrec_curr_tick
				- (p_ts_info->exp_recs[exp_id].ts_us[0]
					* p_ts_info->tick_factor)),
			p_ts_info->tick_factor)),
		p_ts_info->exp_recs[0].ts_us[0],
		p_ts_info->exp_recs[0].ts_us[1],
		p_ts_info->exp_recs[0].ts_us[2],
		p_ts_info->exp_recs[0].ts_us[3],
		p_ts_info->exp_recs[1].ts_us[0],
		p_ts_info->exp_recs[1].ts_us[1],
		p_ts_info->exp_recs[1].ts_us[2],
		p_ts_info->exp_recs[1].ts_us[3],
		p_ts_info->exp_recs[2].ts_us[0],
		p_ts_info->exp_recs[2].ts_us[1],
		p_ts_info->exp_recs[2].ts_us[2],
		p_ts_info->exp_recs[2].ts_us[3]);
#endif
}


/*
 * return: (0/1) or 0xFFFFFFFF
 *     0xFFFFFFFF: when check_idx_valid() return error
 */
unsigned int check_bit_atomic(const unsigned int idx,
	const FS_Atomic_T *p_fs_atomic_val)
{
	const unsigned int ret = check_idx_valid(idx);
	unsigned int result = 0;

	if (unlikely(ret == 0))
		return 0xFFFFFFFF;

	result = FS_ATOMIC_READ(p_fs_atomic_val);
	result = ((result >> idx) & 1UL);

	return result;
}


/*
 * return: 1 or 0xFFFFFFFF
 *     0xFFFFFFFF: when check_idx_valid() return error
 */
unsigned int write_bit_atomic(const unsigned int idx, const unsigned int en,
	FS_Atomic_T *p_fs_atomic_val)
{
	const unsigned int ret = check_idx_valid(idx);

	if (unlikely(ret == 0))
		return 0xFFFFFFFF;

	/* en > 0 => set ; en == 0 => clear */
	if (en > 0)
		FS_ATOMIC_FETCH_OR((1UL << idx), p_fs_atomic_val);
	else
		FS_ATOMIC_FETCH_AND((~(1UL << idx)), p_fs_atomic_val);

	return ret;
}


unsigned int chk_xchg_bit_atomic(const unsigned int idx, const unsigned int en,
	FS_Atomic_T *p_fs_atomic_val)
{
	unsigned int bit_needs_to_be_changed;
	int val_ori, val_target;

	do {
		val_ori = FS_ATOMIC_READ(p_fs_atomic_val);
		if (en) {
			/* !!! want to set bit !!! */
			/* if bit status: 0, set it to 1 (need to setup data) */
			bit_needs_to_be_changed =
				(((val_ori >> idx) & 1UL) == 0) ? 1 : 0;

			val_target = (val_ori | (1UL << idx));
		} else {
			/* !!! want to clr bit !!! */
			/* if bit status: 1, set it to 1 (need to clear data) */
			bit_needs_to_be_changed =
				(((val_ori >> idx) & 1UL) == 1) ? 1 : 0;

			val_target = (val_ori & (~(1UL << idx)));
		}
	} while (FS_ATOMIC_CMPXCHG(val_ori, val_target, p_fs_atomic_val) == 0);

	return bit_needs_to_be_changed;
}


/**
 * return:
 * @1: tick_b is after tick_a (assuming the interval is a short period)
 * @0: tick_a is after tick_b (assuming the interval is a short period)
 */
inline unsigned int check_tick_b_after_a(
	const unsigned long long tick_a_, const unsigned long long tick_b_)
{
#if defined(TS_TICK_64_BITS)
	unsigned long long tick_a = tick_a_, tick_b = tick_b_;
	unsigned long long tick_diff = (tick_b - tick_a);
	unsigned long long tick_half = 0, tick_max = (unsigned long long)(-1);
#else
	unsigned int tick_a = tick_a_, tick_b = tick_b_;
	unsigned int tick_diff = (tick_b - tick_a);
	unsigned int tick_half = 0, tick_max = (unsigned int)(-1);
#endif

	if (tick_a == tick_b)
		return 0;


	tick_half = tick_max >> 1;

#if !defined(REDUCE_FS_UTIL_LOG)
	LOG_INF(
#if defined(TS_TICK_64_BITS)
		"[Tick] (b-a)=%llu (a:%llu/b:%llu), max:%llu, half:%llu, ret:%u\n",
#else
		"[Tick] (b-a)=%u (a:%u/b:%u), max:%u, half:%u, ret:%u\n",
#endif
		(tick_b-tick_a), tick_a, tick_b,
		tick_max, tick_half,
		(tick_diff < tick_half) ? 1 : 0
	);
#endif

	return (tick_diff < tick_half) ? 1 : 0;
}


inline unsigned int check_timestamp_b_after_a(
	const unsigned long long ts_a, const unsigned long long ts_b,
	const unsigned int tick_factor)
{
	unsigned long long tick_a, tick_b;

	tick_a = convert_timestamp_2_tick(ts_a, tick_factor);
	tick_b = convert_timestamp_2_tick(ts_b, tick_factor);

	return check_tick_b_after_a(tick_a, tick_b);
}


inline unsigned long long get_two_timestamp_diff(
	const unsigned long long ts_a, const unsigned long long ts_b,
	const unsigned int tick_factor)
{
	unsigned long long tick_a, tick_b;

	if (tick_factor == 0) {
		LOG_MUST(
			"WARNING: tick_factor:%u, abort after calc, return 0\n",
			tick_factor);
		return 0;
	}

	tick_a = convert_timestamp_2_tick(ts_a, tick_factor);
	tick_b = convert_timestamp_2_tick(ts_b, tick_factor);

	return (check_tick_b_after_a(tick_a, tick_b))
		? ((tick_b - tick_a) / tick_factor)
		: ((tick_a - tick_b) / tick_factor);
}


void get_array_data_from_new_to_old(
	const unsigned long long *in_data,
	const unsigned int newest_idx, const unsigned int len,
	unsigned long long *res_data)
{
	unsigned int i = 0, idx = 0;

	for (i = 0; i < len; ++i) {
		idx = (newest_idx + (len - i)) % len;
		res_data[i] = in_data[idx];
	}
}


/*
 * return:
 *     positive: ts diff table idx
 *     negative: idx_a == idx_b, same sensor
 */
int get_ts_diff_table_idx(const unsigned int idx_a, const unsigned int idx_b)
{
	if (idx_a == idx_b) {
		LOG_MUST(
			"WARNING: non-valid input, idx:(%u/%u) same idx, abort return -1\n",
			idx_a, idx_b);
		return -1;
	}

	if (idx_a < idx_b)
		return (idx_a*SENSOR_MAX_NUM)+idx_b-((idx_a+1)*(idx_a+2))/2;
	else
		return (idx_b*SENSOR_MAX_NUM)+idx_a-((idx_b+1)*(idx_b+2))/2;
}


static void find_latest_n_timestamps(
	const unsigned long long *ts_a_arr, const unsigned long long *ts_b_arr,
	const unsigned int ts_arr_len, const unsigned int ts_ordered_cnt,
	const unsigned int tick_factor,
	unsigned long long *ts_ordered_arr, unsigned long long *ts_label_arr)
{
	unsigned int i = 0, j = 0, k = 0;


	/* non-valid input checking */
	if (ts_arr_len < 3 || ts_ordered_cnt > ts_arr_len || tick_factor == 0) {
		LOG_MUST(
			"WARNING: non-valid input, ts_arr_len:%u (<3), ts_ordered_cnt:%u (>ts_arr_len), tick_factor:%u (==0), skip for finding out latest TS, return\n",
			ts_arr_len, ts_ordered_cnt, tick_factor);
		return;
	}

	/* merge two sorted arrays into result array */
	while (i < ts_ordered_cnt && j < ts_ordered_cnt && k < ts_ordered_cnt) {
		/* index => (i: ts_a_arr / j: ts_b_arr / k: ts_ordered_arr) */
		if (check_timestamp_b_after_a(
				ts_a_arr[i], ts_b_arr[j], tick_factor)) {

			/* pick ts b => mark 'b' to '2' */
			ts_ordered_arr[k] = ts_b_arr[j];
			ts_label_arr[k] = 2;
			k++;
			j++;

		} else {
			/* pick ts a => mark 'a' to '1' */
			ts_ordered_arr[k] = ts_a_arr[i];
			ts_label_arr[k] = 1;
			k++;
			i++;
		}
	}
}


/*
 * input:
 *     ts_a_arr & ts_b_arr: MUST be sorted (in ordered)
 *
 * function:
 *     find out timestamp diff of two target.
 *     ONLY bellow ordered type can find out valid result
 *         |a|b|a| or |b|a|b|
 *
 * return:
 *     positive: find out two target timestamp diff.
 *     negative: can NOT find out target timestamp diff.
 */
long long find_two_sensor_timestamp_diff(
	const unsigned long long *ts_a_arr, const unsigned long long *ts_b_arr,
	const unsigned int ts_arr_len, const unsigned int tick_factor)
{
	long long diff[3] = {-1}, out_diff = 0, min_diff = 0x7FFFFFFFFFFFFFFF;
	unsigned long long ts_ordered[4] = {0}, ts_label[4] = {0};
	unsigned int valid = 1;
	int i = 0;


	/* non-valid input checking */
	if (ts_arr_len < 3 || tick_factor == 0) {
		LOG_MUST(
			"WARNING: non-valid input, ts_arr_len:%u, tick_factor:%u, skip for updating TS diff, return -1\n",
			ts_arr_len, tick_factor);
		return -1;
	}


	/* find out latest N timestamp from two timestamp array */
	find_latest_n_timestamps(
		ts_a_arr, ts_b_arr,
		ts_arr_len, 4, tick_factor,
		ts_ordered, ts_label);

	/* calculate timestamp diff */
	for (i = 0; i < 3; ++i) {
		if (check_timestamp_b_after_a(
				ts_ordered[i], ts_ordered[i+1], tick_factor)) {

			LOG_MUST(
				"WARNING: ts_arr data seems not from new to old, ts_ordered:(%llu/%llu/%llu/%llu), ts_label:(%llu/%llu/%llu/%llu), skip for updating TS diff, return -1 [ts(a:(%llu/%llu/%llu/%llu)/(b:(%llu/%llu/%llu/%llu)), a:1/b:2]\n",
				ts_ordered[0], ts_ordered[1], ts_ordered[2], ts_ordered[3],
				ts_label[0], ts_label[1], ts_label[2], ts_label[3],
				ts_a_arr[0], ts_a_arr[1], ts_a_arr[2], ts_a_arr[3],
				ts_b_arr[0], ts_b_arr[1], ts_b_arr[2], ts_b_arr[3]);

			return -1;
		}

		diff[i] = get_two_timestamp_diff(
			ts_ordered[i], ts_ordered[i+1], tick_factor);

		if (diff[i] < min_diff)
			min_diff = diff[i];
	}

	/* check result is valid or not by using ts label */
	valid = ((ts_label[0] == ts_label[1])
		&& (ts_label[1] == ts_label[2]))
		? 0 : 1;

	out_diff = (valid) ? min_diff : (-1);

	LOG_INF(
		"diff:%lld(%llu/%llu/%llu, min_diff:%llu(valid:%u)), ts_ordered:(%llu/%llu/%llu/%llu), ts_label:(%llu/%llu/%llu/%llu) [ts(a:(%llu/%llu/%llu/%llu)/(b:(%llu/%llu/%llu/%llu)), a:1/b:2, len/factor:(%u/%u)]\n",
		out_diff, diff[0], diff[1], diff[2], min_diff, valid,
		ts_ordered[0], ts_ordered[1], ts_ordered[2], ts_ordered[3],
		ts_label[0], ts_label[1], ts_label[2], ts_label[3],
		ts_a_arr[0], ts_a_arr[1], ts_a_arr[2], ts_a_arr[3],
		ts_b_arr[0], ts_b_arr[1], ts_b_arr[2], ts_b_arr[3],
		ts_arr_len, tick_factor);

	return out_diff;
}


/*
 * return:
 *     1: all results of sensor are synced.
 *     0: one of results of sensor is un-synced.
 *     -1: situation not defined, user should check it manually.
 *
 * input:
 *     mask: bits value for notifying which data will be check.
 *     len: default it should be equal to TS_DIFF_TABLE_LEN.
 */
int check_sync_result(
	const long long *ts_diff_table_arr, const unsigned int mask,
	const unsigned int len, const unsigned int threshold)
{
	int ret = -1;
	unsigned int i = 0;

	/* for error input checking */
	if (len > TS_DIFF_TABLE_LEN) {
		LOG_MUST(
			"WARNING: non-valid input, arr_len:%u > TS_DIFF_TABLE_LEN:%u, abort return -1\n",
			len, TS_DIFF_TABLE_LEN);
		return -1;
	}

	for (i = 0; i < len; ++i) {
		if ((mask >> i) == 0 || ts_diff_table_arr[i] < 0)
			continue;

		ret = (ts_diff_table_arr[i] <= threshold)
			? 1 : 0;
	}

	return ret;
}


/******************************************************************************/
