/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _FRAME_MONITOR_H
#define _FRAME_MONITOR_H

#include "frame_sync_def.h"


/*******************************************************************************
 * CCU rpmsg data structure
 ******************************************************************************/
/* Sync To: "ccu_control_extif.h" */
#define CCU_CAM_TG_MIN 1
#define CCU_CAM_TG_MAX 4

/* Sync To: "ccu_platform.h" */
#define CAMSV_MAX       (16)
#define CAMSV_TG_MIN    (5)
#define CAMSV_TG_MAX    (CAMSV_TG_MIN+CAMSV_MAX)
#define FM_TG_CNT       (CAMSV_TG_MAX)


/* for per sensor */
#define VSYNCS_MAX 4
struct vsync_time {
	unsigned int id;        // tg (min is 1, start from 1)
	unsigned int vsyncs;    // how many vsyncs have been passed
				// since last call to CCU ?
	fs_timestamp_t timestamps[VSYNCS_MAX];
};

/* for per Rproc IPC send */
/* TODO : add a general param for array size, and sync this for fs, algo, fm */
// #define TG_MAX_NUM (CCU_CAM_TG_MAX - CCU_CAM_TG_MIN)
#define TG_MAX_NUM (6)
struct vsync_rec {
	unsigned int ids;
	fs_timestamp_t cur_tick;
	unsigned int tick_factor;
	struct vsync_time recs[TG_MAX_NUM];
};
/******************************************************************************/
#define MSG_TO_CCU_RESET_VSYNC_TIMESTAMP 0
#define MSG_TO_CCU_CLEAR_VSYNC_TIMESTAMP 1
#define MSG_TO_CCU_GET_VSYNC_TIMESTAMP 2
/******************************************************************************/





/******************************************************************************/
// debug/utilities/dump functions
/******************************************************************************/
int frm_get_ts_src_type(void);
/******************************************************************************/


#ifdef SUPPORT_USING_CCU
/******************************************************************************/
// frame monitor --- ccu related function
/******************************************************************************/
/* ==> CCU operation */
void frm_reset_ccu_vsync_timestamp(
	const unsigned int idx, const unsigned int en);
int frm_get_ccu_pwn_cnt(void);
void frm_power_on_ccu(const unsigned int flag);
/******************************************************************************/
#endif


/*******************************************************************************
 * frame monitor function
 ******************************************************************************/
/* ==> frame measurement */
void frm_set_frame_measurement(
	unsigned int idx, unsigned int passed_vsyncs,
	unsigned int curr_fl_us, unsigned int curr_fl_lc,
	unsigned int next_fl_us, unsigned int next_fl_lc);
void frm_get_curr_frame_mesurement_and_ts_data(
	const unsigned int idx, unsigned int *p_fmeas_idx,
	unsigned int *p_pr_fl_us, unsigned int *p_pr_fl_lc,
	unsigned long long *p_act_fl_us, unsigned long long *p_ts_arr);


void frm_reset_frame_info(const unsigned int idx);
void frm_init_frame_info_st_data(const unsigned int idx,
	const unsigned int sensor_id, const unsigned int sensor_idx,
	const unsigned int tg);


/* ==> tg */
void frm_update_tg(unsigned int idx, unsigned int tg);
unsigned int frm_convert_cammux_id_to_ccu_tg_id(const unsigned int cammux_id);
unsigned int frm_chk_and_get_tg_value(const unsigned int cammux_id,
	const unsigned int target_tg);


/* ==> timestamp data (CCU) */
/*
 * return: (0/non 0) for (done/error)
 *
 * input:
 *     tgs -> all TG you want to get vsync from CCU;
 *     len -> array length;
 */
int frm_query_vsync_data(const unsigned int tgs[], const unsigned int len,
	struct vsync_rec *pData);


/* ==> timestamp data (TSREC) */
void frm_query_vsync_data_by_tsrec(
	const unsigned int idxs[], const unsigned int len,
	struct vsync_rec *pData);
void frm_receive_tsrec_timestamp_info(const unsigned int idx,
	const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info);
const struct mtk_cam_seninf_tsrec_timestamp_info *
frm_get_tsrec_timestamp_info_ptr(const unsigned int idx);
/******************************************************************************/


#ifdef FS_UT
/*******************************************************************************
 * !!! please only use bellow function on software debug or ut_test !!!
 ******************************************************************************/
/* only for FrameSync Driver and ut test used */
int frm_get_instance_idx_by_tg(unsigned int tg);

void frm_update_predicted_curr_fl_us(unsigned int idx, unsigned int fl_us);

void frm_update_next_vts_bias_us(unsigned int idx, unsigned int vts_bias);

void frm_set_sensor_curr_fl_us(unsigned int idx, unsigned int fl_us);

void frm_update_predicted_fl_us(
	unsigned int idx,
	unsigned int curr_fl_us, unsigned int next_fl_us);

unsigned int frm_get_predicted_curr_fl_us(unsigned int idx);

void frm_get_predicted_fl_us(
	unsigned int idx,
	unsigned int fl_us[], unsigned int *sensor_curr_fl_us);

void frm_get_next_vts_bias_us(unsigned int idx, unsigned int *vts_bias);

void frm_debug_copy_frame_info_vsync_rec_data(struct vsync_rec *p_vsync_res);

void frm_debug_set_last_vsync_data(struct vsync_rec *v_rec);
/******************************************************************************/
#endif // FS_UT


/******************************************************************************/
// Frame Monitor init function.
/******************************************************************************/
void frm_init(void);
/******************************************************************************/

#endif
