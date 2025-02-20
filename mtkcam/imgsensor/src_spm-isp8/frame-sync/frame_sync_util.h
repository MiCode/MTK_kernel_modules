/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _FRAME_SYNC_UTIL_H
#define _FRAME_SYNC_UTIL_H


#include "frame_sync_def.h"


/*******************************************************************************
 * Frame Sync Util functions.
 ******************************************************************************/
/*
 * 1. act_fl_arr array length is ts_arr array length minus 1.
 * 2. ts_arr is from newest record to oldest record.
 */
void fs_util_calc_act_fl(const unsigned long long ts_arr[],
	unsigned int act_fl_arr[], const unsigned int arr_len,
	const unsigned int tick_factor);


void fs_util_tsrec_dynamic_msg_connector(const unsigned int idx,
	const unsigned int log_str_len, char *log_buf, int len,
	const char *caller);


static inline int alloc_log_buf(const unsigned int alloc_len, char **p_p_buf)
{
	*p_p_buf = FS_CALLOC((alloc_len + 1), sizeof(char));
	if (unlikely(*p_p_buf == NULL))
		return -1;

	*p_p_buf[0] = '\0';

	return 0;
}

static inline long long calc_mod_64(const long long a, const long long b)
{
	return (unlikely(b == 0)) ? 0
		: (((a % b) < 0) ? ((a % b) + b) : (a % b));
}

/* return: (0/1) for (non-valid/valid) */
static inline unsigned int check_idx_valid(const unsigned int _idx)
{
	return (likely(_idx < SENSOR_MAX_NUM)) ? 1 : 0;
}


/*----------------------------------------------------------------------------*/
/* atomic operation related APIs */
/*----------------------------------------------------------------------------*/
/*
 * return: (0/1) or 0xFFFFFFFF
 *     0xFFFFFFFF: when check_idx_valid() return error
 */
unsigned int check_bit_atomic(const unsigned int idx,
	const FS_Atomic_T *p_fs_atomic_val);

/*
 * return: 1 or 0xFFFFFFFF
 *     0xFFFFFFFF: when check_idx_valid() return error
 */
unsigned int write_bit_atomic(const unsigned int idx, const unsigned int en,
	FS_Atomic_T *p_fs_atomic_val);

unsigned int chk_xchg_bit_atomic(const unsigned int idx, const unsigned int en,
	FS_Atomic_T *p_fs_atomic_val);

static inline void clear_all_bit_atomic(FS_Atomic_T *p_fs_atomic_val)
{
	FS_ATOMIC_SET(0, p_fs_atomic_val);
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// sensor related APIs
/*----------------------------------------------------------------------------*/
static inline unsigned int calcLineTimeInNs(
	const unsigned long long pclk, const unsigned int linelength)
{
	return (unlikely(pclk/1000 == 0)) ? 0
		: ((unsigned long long)linelength * 1000000 + ((pclk/1000) - 1))
			/ (pclk/1000);
}

static inline unsigned int convert2TotalTime(
	const unsigned int lineTimeInNs, const unsigned int lc)
{
	return (unlikely(lineTimeInNs == 0)) ? 0
		: ((unsigned int)((unsigned long long)(lc) * lineTimeInNs/1000));
}

static inline unsigned int convert2LineCount(
	const unsigned int lineTimeInNs, const unsigned int val)
{
	return (unlikely(lineTimeInNs == 0)) ? 0
		: (((1000*(unsigned long long)val) / lineTimeInNs) +
			((1000*(unsigned long long)val) % lineTimeInNs ? 1 : 0));
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// timestamp/tick related APIs
/*----------------------------------------------------------------------------*/
static inline unsigned long long convert_timestamp_2_tick(
	const unsigned long long timestamp, const unsigned int tick_factor)
{
	return (likely(tick_factor)) ? (timestamp * tick_factor) : timestamp;
}

static inline unsigned long long convert_tick_2_timestamp(
	const unsigned long long tick, const unsigned int tick_factor)
{
	return (likely(tick_factor)) ? (tick / tick_factor) : tick;
}

static inline unsigned long long calc_time_after_sof(
	const unsigned long long timestamp,
	const unsigned long long tick, const unsigned int tick_factor)
{
	return (unlikely(tick_factor == 0)) ? 0
		: ((tick - convert_timestamp_2_tick(timestamp, tick_factor))
			/ tick_factor);
}

unsigned int check_tick_b_after_a(
	const unsigned long long tick_a_, const unsigned long long tick_b_);

unsigned int check_timestamp_b_after_a(
	const unsigned long long ts_a, const unsigned long long ts_b,
	const unsigned int tick_factor);

unsigned long long get_two_timestamp_diff(
	const unsigned long long ts_a, const unsigned long long ts_b,
	const unsigned int tick_factor);

void get_array_data_from_new_to_old(
	const unsigned long long *in_arr,
	const unsigned int newest_idx, const unsigned int len,
	unsigned long long *res_data);

int get_ts_diff_table_idx(const unsigned int idx_a, const unsigned int idx_b);

long long find_two_sensor_timestamp_diff(
	const unsigned long long *ts_a_arr, const unsigned long long *ts_b_arr,
	const unsigned int ts_arr_len, const unsigned int tick_factor);

int check_sync_result(
	const long long *ts_diff_table_arr, const unsigned int mask,
	const unsigned int len, const unsigned int threshold);
/*----------------------------------------------------------------------------*/


/******************************************************************************/

#endif
