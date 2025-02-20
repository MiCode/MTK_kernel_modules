/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _FRAME_SYNC_H
#define _FRAME_SYNC_H

#include "frame_sync_def.h"


/*******************************************************************************
 * The Method for FrameSync Register Sensor (default pls using sensor_idx).
 ******************************************************************************/
enum CHECK_SENSOR_INFO_METHOD {
	BY_NONE = 0,
	BY_SENSOR_ID = 1,
	BY_SENSOR_IDX = 2,
};

/* set your choose */
#define REGISTER_METHOD BY_SENSOR_IDX
#define REG_INFO "SENSOR_IDX" // for log showing info


struct SensorInfo {
	unsigned int sensor_id;         // imx586 -> 0x0586; s5k3m5sx -> 0x30D5
	unsigned int sensor_idx;        // main1 -> 0; sub1 -> 1;
					// main2 -> 2; sub2 -> 3; main3 -> 4;
	unsigned int seninf_idx;

	/* for using devm functions to allocate memory */
	void *dev;                      // adaptor/i2c client dev (ctx->dev)
};
/******************************************************************************/


/*******************************************************************************
 * Frame-Sync basic define / enum
 ******************************************************************************/
/* The Method for FrameSync standalone (SA) algorithm */
enum FS_SA_METHOD {
	FS_SA_ADAPTIVE_MASTER = 0,
	FS_SA_ASYNC = 2,
};


struct fs_sa_cfg {
	unsigned int idx;
	int sa_method;
	int m_idx;
	int valid_sync_bits;
	int async_m_idx;
	int async_s_bits;
	int rout_center_en_bits;
};
/*----------------------------------------------------------------------------*/


/* info for using callback function */
enum fsync_ctrl_fl_cmd_id {
	FSYNC_CTRL_FL_CMD_ID_NONE = 0,
	FSYNC_CTRL_FL_CMD_ID_EXP_WITH_FL = 1,
	FSYNC_CTRL_FL_CMD_ID_FL = 2,
};


/* call back function prototype, see adaptor-ctrl.h */
typedef int (*callback_func_set_fl_info)(void *p_ctx, const unsigned int cmd_id,
	const void *pf_info, const unsigned int fl_lc,
	const unsigned int fl_lc_arr[], const unsigned int arr_len);
/******************************************************************************/


/*******************************************************************************
 * For HW sensor sync. --- MAX value sync to SENSOR_MAX_NUM
 ******************************************************************************/
enum FS_HW_SYNC_GROUP_ID {
	FS_HW_SYNC_GROUP_ID_MIN = 0,

	FS_HW_SYNC_GROUP_ID_0 = FS_HW_SYNC_GROUP_ID_MIN,
	FS_HW_SYNC_GROUP_ID_1,
	FS_HW_SYNC_GROUP_ID_2,
	FS_HW_SYNC_GROUP_ID_3,
	FS_HW_SYNC_GROUP_ID_4,
	FS_HW_SYNC_GROUP_ID_5,

	FS_HW_SYNC_GROUP_ID_MCSS,
	FS_HW_SYNC_GROUP_ID_MAX
};
/******************************************************************************/


#ifdef FS_UT
/*******************************************************************************
 * The Method for FrameSync sync type/methods.
 ******************************************************************************/
/*
 * !!! MUST sync/check this enum in imgsensor-user.h file. !!!
 *     imgsensor-user.h in:
 *         imgsensor/inc/
 *         device/mediatek/common/kernel-headers/
 * !!! MUST sync/check this enum in imgsensor-user.h file. !!!
 */
enum FS_SYNC_TYPE {
	FS_SYNC_TYPE_NONE = 0,

	/* below tags, chose one (default use VSYNC, mutually exclusive) */
	FS_SYNC_TYPE_VSYNC = 1 << 1,
	FS_SYNC_TYPE_READOUT_CENTER = 1 << 2,

	/* below tags, chose one (default use LE, mutually exclusive) */
	FS_SYNC_TYPE_LE = 1 << 3,
	FS_SYNC_TYPE_SE = 1 << 4,

	/* SA - Async mode */
	FS_SYNC_TYPE_ASYNC_MODE = 1 << 8,

	/* Optional flags */
	FS_SYNC_TYPE_AUTO_CLR_ASYNC_BIT = 1 << 12,

	/* Custom input parameter */
	FS_SYNC_TYPE_CUST_INPUT_PARA_BIT = 1 << 20,
};
/******************************************************************************/
#endif // FS_UT


/*******************************************************************************
 * FrameSync HDR structure & variables
 ******************************************************************************/
/* The HDR feature mode for FrameSync */
enum FS_HDR_FT_MODE {
	FS_HDR_FT_MODE_NORMAL = 0,
	FS_HDR_FT_MODE_STG_HDR = 1,

	/* N:1 / M-Stream */
	FS_HDR_FT_MODE_FRAME_TAG = 1 << 1, /* (N:1) Not 1:1 sync */
	FS_HDR_FT_MODE_ASSIGN_FRAME_TAG = 1 << 2, /* (M-Stream) Not 1:1 sync */

	FS_HDR_FT_MODE_N_1_ON = 1 << 3,
	FS_HDR_FT_MODE_N_1_KEEP = 1 << 4,
	FS_HDR_FT_MODE_N_1_OFF = 1 << 5,
};
/*----------------------------------------------------------------------------*/


enum FS_HDR_EXP {
	FS_HDR_NONE = -1,
	FS_HDR_LE = 0,
	FS_HDR_ME = 1,
	FS_HDR_SE = 2,
	FS_HDR_SSE = 3,
	FS_HDR_SSSE = 4,
	FS_HDR_MAX
};


/* get exp location by hdr_exp_idx_map[exp_cnt][exp] */
/* -1: error handle for mapping to a non valid idx and */
/*     a info/hint for fs_alg_setup_multi_exp_value() */
static const int hdr_exp_idx_map[][FS_HDR_MAX] = {
	/* order => LE:0 / ME:1 / SE:2 / SSE:3 / SSSE:4, (MAX:5) */
	{FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE}, // exp cnt:0
	{FS_HDR_LE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE}, // exp cnt:1
	{FS_HDR_LE, FS_HDR_SE, FS_HDR_NONE, FS_HDR_NONE, FS_HDR_NONE}, // exp cnt:2
	{FS_HDR_LE, FS_HDR_ME, FS_HDR_SE, FS_HDR_NONE, FS_HDR_NONE}, // exp cnt:3

	/* T.B.D. */
	{FS_HDR_LE, FS_HDR_ME, FS_HDR_SE, FS_HDR_SSE, FS_HDR_NONE}, // exp cnt:4
	{FS_HDR_LE, FS_HDR_ME, FS_HDR_SE, FS_HDR_SSE, FS_HDR_SSSE}  // exp cnt:5
};


struct fs_hdr_exp_st {
	unsigned int mode_exp_cnt;       // exp cnt from HDR mode
	unsigned int ae_exp_cnt;         // exp cnt from ae set ctrl

	unsigned int exp_lc[FS_HDR_MAX];
	unsigned int fl_lc[FS_HDR_MAX];

	/* stagger/LB-MF readout length, read margin info */
	unsigned int readout_len_lc;
	unsigned int read_margin_lc;

	/* see sensor recorder header (only by pass) */
	unsigned int multi_exp_type;
	unsigned int dol_type;
	unsigned int exp_order;
	unsigned int min_vblank_lc;
};
/******************************************************************************/


struct fs_streaming_st {
	unsigned int sensor_id;
	unsigned int sensor_idx;

	unsigned int cammux_id;          // need to map to CCU TG ID
	unsigned int target_tg;          // ISP7s: already direct map to CCU tg ID
	unsigned int tg;

	unsigned int fl_active_delay;
	unsigned int def_fl_lc;          // default framelength_lc
	unsigned int max_fl_lc;          // for framelength boundary check
	unsigned int def_shutter_lc;     // default shutter_lc
	unsigned int margin_lc;

	/* for HW sensor sync */
	unsigned int sync_mode;          // sync operate mode. none/master/slave
	unsigned int hw_sync_group_id;   // hw sync group ID
	unsigned int hw_sync_method;     // legacy:0, MCSS:1

	struct fs_hdr_exp_st hdr_exp;    // hdr exposure settings

	/* sensor mode info */
	unsigned long long pclk;
	unsigned int linelength;
	unsigned int lineTimeInNs;

	/* callback function */
	callback_func_set_fl_info func_ptr;
	void *p_ctx;
};


struct fs_perframe_st {
	unsigned int sensor_id;
	unsigned int sensor_idx;

	/* bellow items can be query from "subdrv_ctx" */
	unsigned int min_fl_lc;          // also means max frame rate
	unsigned int shutter_lc;
	unsigned int margin_lc;
	unsigned int flicker_en;
	unsigned int out_fl_lc;

	struct fs_hdr_exp_st hdr_exp;    // hdr exposure settings

	/* for on-the-fly mode change */
	unsigned long long pclk;               // write_shutter(), set_max_framerate()
	unsigned int linelength;         // write_shutter(), set_max_framerate()
	/* lineTimeInNs ~= 10^9 * (linelength/pclk) */
	unsigned int lineTimeInNs;
	unsigned int readout_time_us;    // current mode read out time.

	/* callback function using */
	unsigned int cmd_id;

	/* debug variables */
	int req_id;                      // from mtk hdr ae structure
};


/* seamless switch information */
enum fs_seamless_switch_type {
	FREC_SEAMLESS_SWITCH_CUT_VB_INIT_SHUT = 0,
	FREC_SEAMLESS_SWITCH_ORIG_VB_INIT_SHUT,
	FREC_SEAMLESS_SWITCH_ORIG_VB_ORIG_IMG,
};

struct fs_seamless_property_st {
	enum fs_seamless_switch_type type_id;

	unsigned int orig_readout_time_us;
	unsigned int ctrl_receive_time_us;
	unsigned int hw_re_init_time_us;
	unsigned int prsh_length_lc; // new mode's prsh length if has
};

struct fs_seamless_st {
	struct fs_seamless_property_st prop;
	struct fs_perframe_st seamless_pf_ctrl;

	/* info that may be changed through seamless switch */
	unsigned int fl_active_delay;
};


struct fs_fl_restore_info_st {
	/* debug variables */
	unsigned int magic_num;
	int req_id;

	/* restore FL info */
	unsigned int restored_fl_lc;
	/* ==> for LB-MF sensor */
	unsigned int restored_fl_lc_arr[FS_HDR_MAX];
};
/******************************************************************************/





/*******************************************************************************
 * Frame Sync member functions.
 ******************************************************************************/
struct FrameSync {
	unsigned int (*fs_register_sensor)(const struct SensorInfo *sensor_info,
		const enum CHECK_SENSOR_INFO_METHOD method);
	void (*fs_unregister_sensor)(const struct SensorInfo *sensor_info,
		const enum CHECK_SENSOR_INFO_METHOD method);

	unsigned int (*fs_streaming)(const unsigned int flag,
		struct fs_streaming_st *streaming_st);


	/* enable / disable frame sync processing for this sensor ident */
	void (*fs_set_sync)(const unsigned int ident, unsigned int flag);

	/* for MW assign async mode master sensor idx */
	void (*fs_sa_set_user_async_master)(const unsigned int sidx,
		const unsigned int flag);


	/**********************************************************************/
	// API is defined in frame_sync_camsys.h
	//
	// flag = 1 => Start
	//     return a integer represents how many sensors that
	//     are valid for sync.
	//
	// flag = 0 => End
	//     return a integer represents how many Perframe_Ctrl is achieve.
	/**********************************************************************/
	unsigned int (*fs_sync_frame)(unsigned int flag);


	/* frame sync set shutter */
	void (*fs_set_shutter)(struct fs_perframe_st *pf_ctrl);
	void (*fs_update_shutter)(struct fs_perframe_st *pf_ctrl);
	void (*fs_update_frame_length)(const unsigned int ident,
		const unsigned int fl_lc,
		const unsigned int fl_lc_arr[], const unsigned int arr_len);


	/* for cam mux switch and sensor streaming on before setup cam mux */
	void (*fs_update_tg)(const unsigned int ident, unsigned int tg);
	/* ISP7s HW change, seninf assign target tg ID (direct map to CCU tg ID) */
	void (*fs_update_target_tg)(const unsigned int ident,
		const unsigned int target_tg);


	/* update fs_perframe_st data */
	/*     (for v4l2_ctrl_request_setup order) */
	/*     (set_framelength is called after set_anti_flicker) */
	void (*fs_update_auto_flicker_mode)(
				unsigned int ident, unsigned int en);


	/* update fs_perframe_st data */
	/*     (for v4l2_ctrl_request_setup order) */
	/*     (set_max_framerate_by_scenario is called after set_shutter) */
	void (*fs_update_min_fl_lc)(
		const unsigned int ident, const unsigned int min_fl_lc,
		const unsigned int fl_lc,
		const unsigned int fl_lc_arr[], const unsigned int arr_len);


	/* set extend framelength for STG sensor doing seamless switch */
	/*     parameter: set lc => ext_fl_lc != 0 && ext_fl_us == 0 */
	/*                set us => ext_fl_lc == 0 && ext_fl_us != 0 */
	/*                clear  => ext_fl_lc == 0 && ext_fl_us == 0 */
	void (*fs_set_extend_framelength)(
		unsigned int ident,
		unsigned int ext_fl_lc, unsigned int ext_fl_us);


	/* for notify FrameSync sensor doing seamless switch using */
	void (*fs_chk_exit_seamless_switch_frame)(const unsigned int ident);
	void (*fs_chk_valid_for_doing_seamless_switch)(const unsigned int ident);
	void (*fs_seamless_switch)(const unsigned int ident,
		struct fs_seamless_st *p_seamless_info,
		const unsigned int seamless_sof_cnt);


	/* for choosing FrameSync StandAlone algorithm */
	void (*fs_set_using_sa_mode)(const unsigned int en);


	/* for cam-sys assign taget vsync at subsample */
	/* e.g: SE:0/LE:1, target vsync:0 (in this case is SE) */
	/* => f_tag:0, 1, 0, 1, ... */
	void (*fs_set_frame_tag)(unsigned int ident, unsigned int f_tag);


	void (*fs_n_1_en)(unsigned int ident, unsigned int n, unsigned int en);


	void (*fs_mstream_en)(unsigned int ident, unsigned int en);


	void (*fs_set_debug_info_sof_cnt)(const unsigned int ident,
		const unsigned int sof_cnt);

	void (*fs_notify_vsync)(const unsigned int ident);

	void (*fs_notify_vsync_by_tsrec)(const unsigned int ident);

	void (*fs_notify_sensor_hw_pre_latch_by_tsrec)(
		const unsigned int ident);

	void (*fs_receive_tsrec_timestamp_info)(const unsigned int ident,
		const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info);


	/**********************************************************************/
	/* get frame sync status for this sensor_id */
	/* return: (0 / 1) => (disable / enable) */
	/**********************************************************************/
	unsigned int (*fs_is_set_sync)(const unsigned int ident);

	unsigned int (*fs_is_hw_sync)(const unsigned int ident);

	void (*fs_get_fl_record_info)(const unsigned int ident,
		unsigned int *p_target_min_fl_us, unsigned int *p_out_fl_us);

	void (*fs_clear_fl_restore_status_if_needed)(const unsigned int ident);


	unsigned int (*fs_is_ts_src_type_tsrec)(void);
};


/******************************************************************************/
#if defined(SUPPORT_FS_NEW_METHOD)
void fs_sa_request_switch_master(unsigned int idx);
#endif


unsigned int fs_get_reg_sensor_id(const unsigned int idx);
unsigned int fs_get_reg_sensor_idx(const unsigned int idx);
unsigned int fs_get_reg_sensor_inf_idx(const unsigned int idx);

void fs_setup_sensor_info_st_by_fs_streaming_st(
	const struct fs_streaming_st *streaming_info, struct SensorInfo *info);

void fs_setup_fl_restore_status(const unsigned int idx,
	const struct fs_fl_restore_info_st *p_fl_restore_info);
/******************************************************************************/
/*
 * Frame Sync init function.
 *
 *    init FrameSync object.
 *    get FrameSync function for operation.
 *
 *    return: (0 / 1) => (no error / error)
 */
#if !defined(FS_UT)
unsigned int FrameSyncInit(struct FrameSync **pframeSync, struct device *dev);
void FrameSyncUnInit(struct device *dev);
#else // FS_UT
unsigned int FrameSyncInit(struct FrameSync **framesync);
#endif // FS_UT


#endif
