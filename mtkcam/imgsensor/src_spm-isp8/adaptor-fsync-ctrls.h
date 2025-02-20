/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __ADAPTOR_FSYNC_CTRLS_H__
#define __ADAPTOR_FSYNC_CTRLS_H__


/* !!! ONLY for testing or bypass fsync mgr !!! */
/* !!! if you define this, log msg will also be disable !!! */
// #define FORCE_DISABLE_FSYNC_MGR


/*******************************************************************************
 * streaming ctrls
 ******************************************************************************/
void notify_fsync_mgr_streaming(struct adaptor_ctx *ctx,
	const unsigned int flag);


/*******************************************************************************
 * frame ctrl
 ******************************************************************************/
/*
 * return:
 *     1 => fsync_mgr will use set multi shutter frame length
 *          to set exposure and frame length simultaneously.
 *     0 => sensor adaptor directly set this ctrls to driver.
 *          long exposure => must return 0
 */
int chk_if_need_to_use_s_multi_exp_fl_by_fsync_mgr(struct adaptor_ctx *ctx,
	u64 *ae_exp_arr, u32 ae_exp_cnt);

void notify_fsync_mgr_update_tg(struct adaptor_ctx *ctx, const u64 val);
void notify_fsync_mgr_update_target_tg(struct adaptor_ctx *ctx, const u64 val);

void notify_fsync_mgr_set_sync(struct adaptor_ctx *ctx, const u64 en);

void notify_fsync_mgr_set_async_master(struct adaptor_ctx *ctx, const u64 en);

void notify_fsync_mgr_update_auto_flicker_mode(struct adaptor_ctx *ctx,
	const u64 en);

void notify_fsync_mgr_update_min_fl(struct adaptor_ctx *ctx);

void notify_fsync_mgr_set_extend_framelength(struct adaptor_ctx *ctx,
	const u64 ext_fl);

void notify_fsync_mgr_seamless_switch(struct adaptor_ctx *ctx,
	u64 *ae_exp_arr, u32 ae_exp_max_cnt,
	u32 orig_readout_time_us, u32 target_scenario_id);

void notify_fsync_mgr_n_1_en(struct adaptor_ctx *ctx,
	const u64 n, const u64 en);

void notify_fsync_mgr_mstream_en(struct adaptor_ctx *ctx, const u64 en);

void notify_fsync_mgr_subsample_tag(struct adaptor_ctx *ctx, const u64 sub_tag);

void notify_fsync_mgr_set_shutter(struct adaptor_ctx *ctx,
	u64 *ae_exp_arr, u32 ae_exp_cnt);


/* this API only be used (NOT directly return) on HW senor sync pin */
void notify_fsync_mgr_sync_frame(struct adaptor_ctx *ctx,
	const unsigned int flag);


/*******************************************************************************
 * ext ctrl
 ******************************************************************************/
void notify_fsync_mgr_update_sof_cnt(struct adaptor_ctx *ctx,
	const u32 sof_cnt);

void notify_fsync_mgr_vsync(struct adaptor_ctx *ctx);

void notify_fsync_mgr_vsync_by_tsrec(struct adaptor_ctx *ctx);

void notify_fsync_mgr_sensor_hw_pre_latch_by_tsrec(struct adaptor_ctx *ctx,
	const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info);

void notify_fsync_mgr_receive_tsrec_timestamp_info(struct adaptor_ctx *ctx,
	const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info);


void notify_fsync_mgr_g_fl_record_info(struct adaptor_ctx *ctx,
	struct mtk_fs_frame_length_info *p_fl_info);

void notify_fsync_mgr_clear_fl_restore_info_if_needed(struct adaptor_ctx *ctx);


/*******************************************************************************
 * init Frame-Sync Mgr / get all function calls
 ******************************************************************************/
int notify_fsync_mgr(struct adaptor_ctx *ctx, const int on);

#endif
