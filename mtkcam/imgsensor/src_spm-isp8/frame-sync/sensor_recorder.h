/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _SENSOR_RECORDER_H
#define _SENSOR_RECORDER_H

/* TODO: check below files, after refactor some files should not include */
#include "frame_sync.h"


/******************************************************************************/
// frame recorder define/enum/structure
/******************************************************************************/
#define RECORDER_DEPTH 4

#if defined(FS_UT)
#define FL_ACT_CHK_TH 0
#else
#define FL_ACT_CHK_TH 200
#endif

#if defined(TS_TICK_64_BITS) // ISP7sp TSREC use global timer
#define SenRec_TS_T unsigned long long
#else // e.g., CCU, ISP7s TSREC, ISP7sp TSREC use local timer
#define SenRec_TS_T unsigned int
#endif


enum predicted_fl_label {
	PREDICT_NEXT_FL = 0,
	PREDICT_STABLE_FL,
};


/*----------------------------------------------------------------------------*/
/*
 * when the sensor mode's exp cnt is larger then 1 !!!
 * => this enum is for checking which type of logic/algo. is correct for using.
 */
enum multi_exp_type {
	MULTI_EXP_TYPE_STG,
	MULTI_EXP_TYPE_LBMF,
};

#define FL_ARR_IN_LUT_ORDER
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// exp order
/*----------------------------------------------------------------------------*/
enum exp_order {
	EXP_ORDER_LE_1ST = 0,
	EXP_ORDER_SE_1ST,
	EXP_ORDER_MAX
};

static const int exp_order_idx_map[][FS_HDR_MAX+1][FS_HDR_MAX] = {
	/* default order: LE 1st (EXP_ORDER_LE_1ST) */
	{
		/* exp cnt: 0 */
		{FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE},
		/* exp cnt: 1 */
		{FS_HDR_LE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE},
		/* exp cnt: 2 */
		{FS_HDR_LE, FS_HDR_SE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE},
		/* exp cnt: 3 */
		{FS_HDR_LE, FS_HDR_ME, FS_HDR_SE, FS_HDR_NONE, FS_HDR_NONE},
		/* exp cnt: 4 */
		{FS_HDR_LE, FS_HDR_ME, FS_HDR_SE, FS_HDR_SSE, FS_HDR_NONE},
		/* exp cnt: 5 */
		{FS_HDR_LE, FS_HDR_ME, FS_HDR_SE, FS_HDR_SSE, FS_HDR_SSSE}
	},

	/* custom order: SE 1st (EXP_ORDER_SE_1ST) */
	{
		/* exp cnt: 0 */
		{FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE},
		/* exp cnt: 1 */
		{FS_HDR_LE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE},
		/* exp cnt: 2 */
		{FS_HDR_SE, FS_HDR_LE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE},
		/* exp cnt: 3 */
		{FS_HDR_SE, FS_HDR_ME, FS_HDR_LE, FS_HDR_NONE, FS_HDR_NONE},
		/* exp cnt: 4 */
		{FS_HDR_SSE, FS_HDR_SE, FS_HDR_ME, FS_HDR_LE, FS_HDR_NONE},
		/* exp cnt: 5 */
		{FS_HDR_SSE, FS_HDR_SSE, FS_HDR_SE, FS_HDR_ME, FS_HDR_LE}
	}
};
/*----------------------------------------------------------------------------*/


struct FrameRecord {
	/* per-frame dynamic info */
	unsigned int shutter_lc;
	unsigned int framelength_lc;

	unsigned int ae_exp_cnt;
	unsigned int exp_lc_arr[FS_HDR_MAX];
	unsigned int fl_lc_arr[FS_HDR_MAX];

	/* sensor mode related info (maybe do not put here) */
	unsigned int margin_lc;
	unsigned int read_margin_lc;
	unsigned int readout_len_lc;
	unsigned int mode_exp_cnt;
	// enum multi_exp_type m_exp_type;
	unsigned int m_exp_type;
	unsigned int exp_order;

	unsigned long long pclk;
	unsigned int line_length;

	/* info for debug */
	unsigned int mw_req_id;
};


struct predicted_fl_info_st {
	unsigned int pr_curr_fl_lc;
	unsigned int pr_curr_fl_us;

	unsigned int pr_next_fl_lc;
	unsigned int pr_next_fl_us;

	unsigned int pr_stable_fl_lc;
	unsigned int pr_stable_fl_us;

	unsigned int curr_exp_rd_offset_lc[FS_HDR_MAX];
	unsigned int curr_exp_rd_offset_us[FS_HDR_MAX];

	unsigned int next_exp_rd_offset_lc[FS_HDR_MAX];
	unsigned int next_exp_rd_offset_us[FS_HDR_MAX];
};


struct frec_seamless_st {
	struct fs_seamless_property_st prop;
	struct FrameRecord frame_rec; // new ae ctrl for seamless
};


/******************************************************************************/
// frame recorder functions
/******************************************************************************/
/*----------------------------------------------------------------------------*/
// debug/dump function
/*----------------------------------------------------------------------------*/
void frec_dump_cascade_exp_fl_info(const unsigned int idx,
	const unsigned int *exp_cas_arr, const unsigned int *fl_cas_arr,
	const unsigned int length, const char *caller);

void frec_dump_predicted_fl_info_st(const unsigned int idx,
	const struct predicted_fl_info_st *fl_info,
	const char *caller);

void frec_dump_frame_record_info(const struct FrameRecord *p_frame_rec,
	const char *caller);

void frec_dump_recorder(const unsigned int idx, const char *caller);
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// recorder dynamic memory allocate / free functions
/*----------------------------------------------------------------------------*/
void frec_alloc_mem_data(const unsigned int idx, void *dev);
void frec_free_mem_data(const unsigned int idx, void *dev);
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// utilities functions
/*----------------------------------------------------------------------------*/
void frec_setup_frame_rec_by_fs_streaming_st(struct FrameRecord *p_frame_rec,
	const struct fs_streaming_st *sensor_info);

void frec_setup_frame_rec_by_fs_perframe_st(struct FrameRecord *p_frame_rec,
	const struct fs_perframe_st *pf_ctrl);

void frec_setup_seamless_rec_by_fs_seamless_st(
	struct frec_seamless_st *p_seamless_rec,
	const struct fs_seamless_st *p_seamless_info);

void frec_g_valid_min_fl_arr_val_for_lut(const unsigned int idx,
	const struct FrameRecord *curr_rec,
	const unsigned int target_fl_lc,
	unsigned int fl_lc_arr[], const unsigned int arr_len);
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// frame length related functions for user/caller
/*----------------------------------------------------------------------------*/
void frec_get_predicted_frame_length_info(const unsigned int idx,
	const struct FrameRecord *curr_rec,
	struct predicted_fl_info_st *res_pr_fl,
	const char *caller);

unsigned int frec_g_valid_min_fl_lc_for_shutters_by_frame_rec(
	const unsigned int idx, const struct FrameRecord *curr_rec,
	const unsigned int min_fl_lc, const enum predicted_fl_label label);
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// recorder framework related functions
/*----------------------------------------------------------------------------*/
void frec_chk_fl_pr_match_act(const unsigned int idx);

void frec_update_fl_info(const unsigned int idx, const unsigned int fl_lc,
	const unsigned int fl_lc_arr[], const unsigned int arr_len);

void frec_update_record(const unsigned int idx,
	const struct FrameRecord *p_frame_rec);

void frec_push_record(const unsigned int idx);

void frec_reset_records(const unsigned int idx);

void frec_setup_def_records(const unsigned int idx, const unsigned int def_fl_lc,
	const struct FrameRecord *p_frame_rec);

void frec_init_recorder(const unsigned int idx,
	const struct FrameRecord *p_frame_rec,
	const unsigned int def_fl_lc, unsigned int fl_act_delay);

void frec_seamless_switch(const unsigned int idx,
	const unsigned int def_fl_lc, const unsigned int fl_act_delay,
	const struct frec_seamless_st *p_seamless_rec);

void frec_notify_vsync(const unsigned int idx);

void frec_notify_update_timestamp_data(const unsigned int idx,
	const unsigned int tick_factor,
	const SenRec_TS_T ts_us[], const unsigned int length);
/*----------------------------------------------------------------------------*/

#endif
