// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#if !defined(FS_UT)
#include <linux/init.h>
#include <linux/delay.h>        /* for get ktime */
#include <linux/module.h>
#include <linux/kernel.h>
#endif // FS_UT

#include "frame_sync.h"
#include "frame_sync_camsys.h"
#include "frame_sync_algo.h"
#include "frame_monitor.h"
#include "sensor_recorder.h"

#include "hw_sensor_sync_algo.h"

#if !defined(FS_UT)
#include "frame_sync_console.h"
#endif // FS_UT


/******************************************************************************/
// Log message
/******************************************************************************/
#include "frame_sync_log.h"

#define REDUCE_FS_DRV_LOG
#define PFX "FrameSync"
#define FS_LOG_DBG_DEF_CAT LOG_FS
/******************************************************************************/





/******************************************************************************/
// Frame Sync Mgr structure / enum
/******************************************************************************/
enum FS_STATUS {
	FS_NONE = 0,
	FS_INITIALIZED = 1,
	FS_WAIT_FOR_SYNCFRAME_START = 2,
	FS_START_TO_GET_PERFRAME_CTRL = 3,
	FS_STATUS_UNKNOWN
};

//------------------------ fs preset perframe st -----------------------------//
struct fs_preset_perframe_st {
	unsigned int is_valid;
	unsigned int is_streaming;
	struct fs_perframe_st preset_pf_ctrl;
};
//----------------------------------------------------------------------------//


//----------------------- fs seamless perframe st ----------------------------//
struct fs_seamless_ctrl_st {
	FS_Atomic_T wait_for_processing;
	unsigned int seamless_sof_cnt;
	struct fs_seamless_st seamless_info;
};
//----------------------------------------------------------------------------//


//------------------------ fs mgr register sensor-----------------------------//
struct SensorTable {
	FS_Atomic_T reg_cnt;
	struct SensorInfo sensors[SENSOR_MAX_NUM];
};
//----------------------------------------------------------------------------//


//------------------------ call back function data ---------------------------//
struct callback_info_st {
	void *p_ctx;
	unsigned int cmd_id;
	callback_func_set_fl_info func_ptr;
};
//----------------------------------------------------------------------------//


//-------------------------------- fs mgr ------------------------------------//
struct FrameSyncMgr {
	/* Sensor Register Table */
	struct SensorTable reg_table;

	/* Frame Sync Status information */
	unsigned int user_counter;  // also fs_init() cnt

	FS_Atomic_T fs_status;
	FS_Atomic_T streaming_bits;
	FS_Atomic_T enSync_bits;
	FS_Atomic_T validSync_bits;
	FS_Atomic_T pf_ctrl_bits;
	FS_Atomic_T fl_restore_ctrl_bits;
	FS_Atomic_T setup_complete_bits;
	FS_Atomic_T seamless_bits;  // notify which sensor is doing seamless switch

	unsigned int last_pf_ctrl_bits;
	unsigned int last_setup_complete_bits;
	unsigned int trigger_ctrl_bits;

	/* For support HW sync */
	unsigned int hw_sync_group_id[SENSOR_MAX_NUM];
	FS_Atomic_T hw_sync_bits;
	FS_Atomic_T hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_MAX];
	FS_Atomic_T hw_sync_non_valid_group_bits;
	FS_Atomic_T setup_complete_hw_group_bits[FS_HW_SYNC_GROUP_ID_MAX];
	/* --- fs act cnt (for ctrl pair trigger) --- */
	unsigned int act_cnt[FS_HW_SYNC_GROUP_ID_MAX];

	/* For keeping things before register/streaming on */
	FS_Atomic_T set_sync_idx_table[SENSOR_MAX_NUM];


#ifdef SUPPORT_USING_CCU
	FS_Atomic_T power_on_ccu_bits;
#endif


	/* for different fps sync sensor sync together */
	/* e.g. fps: (60 vs 30) */
	/* => frame_cell_size: 2 */
	/* => frame_tag: 0, 1, 0, 1, 0, ... */
	enum FS_HDR_FT_MODE hdr_ft_mode[SENSOR_MAX_NUM];
	unsigned int frame_cell_size[SENSOR_MAX_NUM];
	unsigned int frame_tag[SENSOR_MAX_NUM];


	/* keep for supporting trigger by ext vsync */
	struct fs_perframe_st pf_ctrl[SENSOR_MAX_NUM];

	/* for FL auto restore */
	struct fs_fl_restore_info_st *fl_restore_info[SENSOR_MAX_NUM];


	/* for set ae ctrl before streaming on */
	struct fs_preset_perframe_st preset_ctrl[SENSOR_MAX_NUM];

	/* for seamless switch */
	struct fs_seamless_ctrl_st seamless_ctrl[SENSOR_MAX_NUM];


	/* call back */
	struct callback_info_st cb_info[SENSOR_MAX_NUM];


#ifdef SUPPORT_FS_NEW_METHOD
	unsigned int user_set_sa:1;             // user config using SA
	FS_Atomic_T user_async_master_sidx;     // user config async master sidx

	FS_Atomic_T using_sa_ver;               // flag - using standalone ver
	FS_Atomic_T sa_bits;                    // for needing standalone ver
	FS_Atomic_T sa_method;                  // 0:adaptive switch
	FS_Atomic_T master_idx;                 // SA master idx
	FS_Atomic_T async_mode_bits;            // SA async mode bits
	FS_Atomic_T async_master_idx;           // SA async mode m_idx
#endif // SUPPORT_FS_NEW_METHOD


	unsigned int sof_cnt_arr[SENSOR_MAX_NUM];   // debug, from p1 sof cnt
	/* for checking receive ctrl timing */
	unsigned int notify_vsync_sof_cnt_arr[SENSOR_MAX_NUM];


#ifndef FS_UT
	/* Lock */
	spinlock_t fl_restore_info_update_spinlock[SENSOR_MAX_NUM];
#endif
};
static struct FrameSyncMgr fs_mgr;
//----------------------------------------------------------------------------//
/******************************************************************************/





/******************************************************************************/
// Operate & Basic & utility function
/******************************************************************************/
static inline void change_fs_status(const enum FS_STATUS status)
{
	FS_ATOMIC_SET((int)status, &fs_mgr.fs_status);
}


/*
 * return: enum FS_STATUS value
 *     FS_STATUS_UNKNOWN:
 *         when reading atomic value with
 *         the value is out of valid range.
 */
static enum FS_STATUS get_fs_status(void)
{
	enum FS_STATUS status;
	int val = FS_ATOMIC_READ(&fs_mgr.fs_status);

	/* check if value valid for enum */
	if (unlikely(!(val >= FS_NONE && val < FS_STATUS_UNKNOWN))) {
		LOG_INF(
			"ERROR: get status val:%d, treat as FS_STATUS_UNKNOWN\n",
			val);

		return FS_STATUS_UNKNOWN;
	}

	status = (enum FS_STATUS)val;

	return status;
}


#if defined(SUPPORT_FS_NEW_METHOD)
static inline unsigned int fs_user_sa_config(void)
{
#ifdef FORCE_USING_SA_MODE
	return 1;
#else
	return fs_mgr.user_set_sa;
#endif
}
#endif
/******************************************************************************/





/******************************************************************************/
// Dump & Debug function
/******************************************************************************/
static void fs_dump_status(const int idx, const int flag, const char *caller,
	const char *msg)
{
	const unsigned int log_str_len = 1024;
	char *log_buf = NULL;
	int len = 0, ret;

	ret = alloc_log_buf(log_str_len, &log_buf);
	if (unlikely(ret != 0)) {
		LOG_MUST(
			"[%s]: ERROR: [%u] flag:%u, log_buf allocate memory failed\n",
			caller, idx, flag);
		return;
	}

	FS_SNPRF(log_str_len, log_buf, len,
		"[%s:%d/%d %s]: stat:%u, stream:%#x/enSync:%#x(%#x/%#x/%#x/%#x/%#x/%#x)/valid:%#x",
		caller, idx, flag, msg,
		get_fs_status(),
		FS_ATOMIC_READ(&fs_mgr.streaming_bits),
		FS_ATOMIC_READ(&fs_mgr.enSync_bits),
		FS_ATOMIC_READ(&fs_mgr.set_sync_idx_table[0]),
		FS_ATOMIC_READ(&fs_mgr.set_sync_idx_table[1]),
		FS_ATOMIC_READ(&fs_mgr.set_sync_idx_table[2]),
		FS_ATOMIC_READ(&fs_mgr.set_sync_idx_table[3]),
		FS_ATOMIC_READ(&fs_mgr.set_sync_idx_table[4]),
		FS_ATOMIC_READ(&fs_mgr.set_sync_idx_table[5]),
		FS_ATOMIC_READ(&fs_mgr.validSync_bits));

	/* has sensor in HW sync mode ==> add more info */
	if (FS_POPCOUNT(FS_ATOMIC_READ(&fs_mgr.hw_sync_bits))) {
		FS_SNPRF(log_str_len, log_buf, len,
			", hw_sync:%#x(%#x)(%#x/%#x/%#x/%#x/%#x/%#x), trigger:%#x, pf_ctrl:%#x(%#x), complete:%#x(%#x)(hw:%#x/%#x/%#x/%#x/%#x/%#x), act(%u/%u/%u/%u/%u/%u)",
			FS_ATOMIC_READ(&fs_mgr.hw_sync_bits),
			FS_ATOMIC_READ(&fs_mgr.hw_sync_non_valid_group_bits),
			FS_ATOMIC_READ(&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_0]),
			FS_ATOMIC_READ(&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_1]),
			FS_ATOMIC_READ(&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_2]),
			FS_ATOMIC_READ(&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_3]),
			FS_ATOMIC_READ(&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_4]),
			FS_ATOMIC_READ(&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_5]),
			fs_mgr.trigger_ctrl_bits,
			FS_ATOMIC_READ(&fs_mgr.pf_ctrl_bits),
			fs_mgr.last_pf_ctrl_bits,
			FS_ATOMIC_READ(&fs_mgr.setup_complete_bits),
			fs_mgr.last_setup_complete_bits,
			FS_ATOMIC_READ(&fs_mgr.setup_complete_hw_group_bits[FS_HW_SYNC_GROUP_ID_0]),
			FS_ATOMIC_READ(&fs_mgr.setup_complete_hw_group_bits[FS_HW_SYNC_GROUP_ID_1]),
			FS_ATOMIC_READ(&fs_mgr.setup_complete_hw_group_bits[FS_HW_SYNC_GROUP_ID_2]),
			FS_ATOMIC_READ(&fs_mgr.setup_complete_hw_group_bits[FS_HW_SYNC_GROUP_ID_3]),
			FS_ATOMIC_READ(&fs_mgr.setup_complete_hw_group_bits[FS_HW_SYNC_GROUP_ID_4]),
			FS_ATOMIC_READ(&fs_mgr.setup_complete_hw_group_bits[FS_HW_SYNC_GROUP_ID_5]),
			fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_0],
			fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_1],
			fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_2],
			fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_3],
			fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_4],
			fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_5]);
	}

	FS_SNPRF(log_str_len, log_buf, len,
		", SA(%d):(%#x/m_idx:%d, async(%#x, m_idx:%d(sidx:%d))), seamless:%#x, hdr_ft_mode(%u/%u/%u/%u/%u/%u)",
		FS_ATOMIC_READ(&fs_mgr.using_sa_ver),
		FS_ATOMIC_READ(&fs_mgr.sa_bits),
		FS_ATOMIC_READ(&fs_mgr.master_idx),
		FS_ATOMIC_READ(&fs_mgr.async_mode_bits),
		FS_ATOMIC_READ(&fs_mgr.async_master_idx),
		FS_ATOMIC_READ(&fs_mgr.user_async_master_sidx),
		FS_ATOMIC_READ(&fs_mgr.seamless_bits),
		fs_mgr.hdr_ft_mode[0],
		fs_mgr.hdr_ft_mode[1],
		fs_mgr.hdr_ft_mode[2],
		fs_mgr.hdr_ft_mode[3],
		fs_mgr.hdr_ft_mode[4],
		fs_mgr.hdr_ft_mode[5]);

	FS_SNPRF(log_str_len, log_buf, len,
		", ts_src_type:%d(0:unknown/1:CCU/2:TSREC)",
		frm_get_ts_src_type());

#if defined(SUPPORT_USING_CCU)
	FS_SNPRF(log_str_len, log_buf, len,
		", pw_ccu:%#x(cnt:%d)",
		FS_ATOMIC_READ(&fs_mgr.power_on_ccu_bits),
		frm_get_ccu_pwn_cnt());
#endif

	LOG_MUST_LOCK("%s\n", log_buf);

	FS_FREE(log_buf);
}
/******************************************************************************/





/******************************************************************************/
// Register Sensor function  /  SensorInfo & SensorTable
//
// PS: The method register "BY_SENSOR_IDX" is't been tested in this SW flow.
/******************************************************************************/
static inline unsigned int compare_sensor_id(
	const unsigned int id1, const unsigned int id2)
{
	return (id1 == id2 && id1 != 0) ? 1 : 0;
}


static inline unsigned int compare_sensor_idx(
	const unsigned int idx1, const unsigned int idx2)
{
	return (idx1 == idx2) ? 1 : 0;
}


static inline unsigned int check_sensor_info(
	const struct SensorInfo *info1, const struct SensorInfo *info2,
	const enum CHECK_SENSOR_INFO_METHOD method)
{
	switch (method) {
	case BY_SENSOR_ID:
		return compare_sensor_id(info1->sensor_id, info2->sensor_id);
	case BY_SENSOR_IDX:
	default:
		return compare_sensor_idx(info1->sensor_idx, info2->sensor_idx);
	}
}


static inline unsigned int fs_get_reg_sensor_ident(
	const unsigned int sid, const unsigned int sidx)
{
	unsigned int ident = 0;

	switch (REGISTER_METHOD) {
	case BY_SENSOR_ID:
		ident = sid;
		break;
	case BY_SENSOR_IDX:
	default:
		ident = sidx;
		break;
	}

	return ident;
}


unsigned int fs_get_reg_sensor_id(const unsigned int idx)
{
	return fs_mgr.reg_table.sensors[idx].sensor_id;
}


unsigned int fs_get_reg_sensor_idx(const unsigned int idx)
{
	return fs_mgr.reg_table.sensors[idx].sensor_idx;
}


unsigned int fs_get_reg_sensor_inf_idx(const unsigned int idx)
{
	return fs_mgr.reg_table.sensors[idx].seninf_idx;
}


void fs_dump_all_registered_sensor_info(const char *caller)
{
	const unsigned int log_str_len = 512;
	const int reg_cnt = FS_ATOMIC_READ(&fs_mgr.reg_table.reg_cnt);
	unsigned int i;
	int len = 0, ret;
	char *log_buf = NULL;

	ret = alloc_log_buf(log_str_len, &log_buf);
	if (unlikely(ret != 0)) {
		LOG_MUST(
			"[%s]: ERROR: log_buf allocate memory failed\n",
			caller);
		return;
	}

	FS_SNPRF(log_str_len, log_buf, len,
		"[%s] sensor registered cnt:%d, sensor info:(",
		caller, reg_cnt);
	for (i = 0; i < reg_cnt; ++i) {
		FS_SNPRF(log_str_len, log_buf, len,
			"[%u] ID:%#x(sidx:%u/inf:%u)/",
			i,
			fs_get_reg_sensor_id(i),
			fs_get_reg_sensor_idx(i),
			fs_get_reg_sensor_inf_idx(i));
	}
	FS_SNPRF(log_str_len, log_buf, len, ")");

	LOG_MUST("%s\n", log_buf);
	FS_FREE(log_buf);
}


void fs_setup_sensor_info_st_by_fs_streaming_st(
	const struct fs_streaming_st *streaming_info, struct SensorInfo *info)
{
	memset(info, 0, sizeof(*info));

	info->sensor_id = streaming_info->sensor_id;
	info->sensor_idx = streaming_info->sensor_idx;
	// info->seninf_idx = streaming_info->seninf_idx;
}


static void fs_notify_alloc_mem_data(const unsigned int idx, void *dev)
{
	/* !!! call every object that need to allocate memory !!! */
	frec_alloc_mem_data(idx, dev);
}


static void fs_notify_free_mem_data(const unsigned int idx, void *dev)
{
	/* !!! call every object that need to free memory !!! */
	frec_free_mem_data(idx, dev);
}


/*
 * return: uint_t or 0xffffffff
 *     uint_t: array position for the registered sensor save in.
 *     0xffffffff: not found this sensor ID in "SensorTable".
 */
static unsigned int fs_search_reg_sensors(const struct SensorInfo *sensor_info,
	const enum CHECK_SENSOR_INFO_METHOD method)
{
	unsigned int i = 0;
	int reg_cnt = FS_ATOMIC_READ(&fs_mgr.reg_table.reg_cnt);

	for (i = 0; i < reg_cnt; ++i) {
		const struct SensorInfo *pInfo = &fs_mgr.reg_table.sensors[i];

		if (check_sensor_info(pInfo, sensor_info, method))
			return i;
	}

	return 0xffffffff;
}


/*
 * return: uint_t or 0xffffffff
 *     uint_t: array position for the registered sensor save in.
 *     0xffffffff: reach maximum register capacity, couldn't register it.
 *
 * !!! You have check the return idx before using it. !!!
 */
static unsigned int fs_push_sensor(const struct SensorInfo *sensor_info)
{
	struct SensorTable (*pSensorTable) = &fs_mgr.reg_table;
	FS_Atomic_T *p_fs_atomic_val = &fs_mgr.reg_table.reg_cnt;
	int cnt_now = 0, cnt_new = 0;

	do {
		cnt_now = FS_ATOMIC_READ(p_fs_atomic_val);
		cnt_new = cnt_now + 1;

		/* boundary check and check if reach maximum capacity */
		if (!((cnt_now >= 0) && (cnt_now < SENSOR_MAX_NUM)))
			return 0xFFFFFFFF;
	} while (FS_ATOMIC_CMPXCHG(cnt_now, cnt_new, p_fs_atomic_val) == 0);

	/* => cnt_now is correct and avalible */
	pSensorTable->sensors[(unsigned int)cnt_now] = *sensor_info;

	return cnt_now;
}


/*
 * return: uint_t or 0xffffffff
 *     uint_t: array position for the registered sensor save in.
 *     0xffffffff: couldn't register sensor.
 *                 (reach maximum capacity or sensor ID is 0)
 *
 * !!! You have better to check the return idx before using it. !!!
 */
unsigned int fs_register_sensor(const struct SensorInfo *sensor_info,
	const enum CHECK_SENSOR_INFO_METHOD method)
{
	unsigned int idx;

	/* check invalid sensor id */
	if (unlikely(method == BY_SENSOR_ID && sensor_info->sensor_id == 0)) {
		LOG_MUST(
			"ERROR: ID:%#x(sidx:%u/inf:%u), method:%s(%d)(N:0/ID:1/IDX:2), but sensor ID is %#x\n",
			sensor_info->sensor_id,
			sensor_info->sensor_idx,
			sensor_info->seninf_idx,
			REG_INFO, method,
			sensor_info->sensor_id);
		return 0xffffffff;
	}

	/* search registered sensors */
	idx = fs_search_reg_sensors(sensor_info, method);
	if (likely(idx == 0xffffffff)) {
		/* 2-1. add / push this sensor, regitser it */
		/*      two results: */
		/*          1. register successfully; */
		/*          2. can't register */
		idx = fs_push_sensor(sensor_info);
		fs_dump_all_registered_sensor_info(__func__);
		if (unlikely(!check_idx_valid(idx))) {
			LOG_MUST(
				"ERROR: Reach max sensor capacity:%u\n",
				SENSOR_MAX_NUM);
			return idx;
		}
	} else {
		/* 2-2. this sensor has been registered, do nothing */
		LOG_INF(
			"[%u] ID:%#x(sidx:%u/inf:%u), method:%s(%d)(N:0/ID:1/IDX:2), already registered\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			REG_INFO, method);

		if (likely(method == BY_SENSOR_IDX)) {
			if (unlikely(fs_mgr.reg_table.sensors[idx].sensor_id !=
				sensor_info->sensor_id)) {

				LOG_MUST(
					"OVERWRITE: [%u] ID:%#x(sidx:%u/inf:%u), method:%s(%d)(N:0/ID:1/IDX:2)\n",
					idx,
					sensor_info->sensor_id,
					sensor_info->sensor_idx,
					sensor_info->seninf_idx,
					REG_INFO, method);

				fs_mgr.reg_table.sensors[idx].sensor_id =
					sensor_info->sensor_id;
				fs_mgr.reg_table.sensors[idx].seninf_idx =
					sensor_info->seninf_idx;
			}
		}
	}

	/* allocate memory anyway, callee func. handle really allocate or not */
	fs_notify_alloc_mem_data(idx, sensor_info->dev);

	return idx;
}


void fs_unregister_sensor(const struct SensorInfo *sensor_info,
	const enum CHECK_SENSOR_INFO_METHOD method)
{
	unsigned int idx;

	/* check invalid sensor id */
	if (unlikely(method == BY_SENSOR_ID && sensor_info->sensor_id == 0)) {
		LOG_MUST(
			"ERROR: ID:%#x(sidx:%u/inf:%u), method:%s(%d)(N:0/ID:1/IDX:2), but sensor ID is %#x\n",
			sensor_info->sensor_id,
			sensor_info->sensor_idx,
			sensor_info->seninf_idx,
			REG_INFO, method,
			sensor_info->sensor_id);
		return;
	}

	/* search registered sensors */
	idx = fs_search_reg_sensors(sensor_info, method);
	if (unlikely(idx == 0xffffffff)) {
		LOG_MUST(
			"ERROR: [%d] ID:%#x(sidx:%u/inf:%u), method:%s(%d)(N:0/ID:1/IDX:2), but sensor is not found, return\n",
			idx,
			sensor_info->sensor_id,
			sensor_info->sensor_idx,
			sensor_info->seninf_idx,
			REG_INFO, method);
		return;
	}

	/* !!!! ONLY call free mem that allocated at registered sensor !!!! */
	fs_notify_free_mem_data(idx, sensor_info->dev);
}


/* return: 0 => found idx; non-0 => not found. */
static unsigned int fs_g_registered_idx_by_ident(const unsigned int ident,
	unsigned int *p_idx, const char *caller)
{
	struct SensorInfo info = {0};

	switch (REGISTER_METHOD) {
	case BY_SENSOR_ID:
		info.sensor_id = ident;
		break;
	case BY_SENSOR_IDX:
	default:
		info.sensor_idx = ident;
		break;
	}

	*p_idx = fs_search_reg_sensors(&info, REGISTER_METHOD);
	if (unlikely(!check_idx_valid(*p_idx))) {
		LOG_MUST(
			"[%s] ERROR: [%d] %s(%d)(N:0/ID:1/IDX:2) is not register, input ident:%u, return\n",
			caller, *p_idx, REG_INFO, REGISTER_METHOD, ident);
		return 1;
	}

	return 0;
}


/* return: 0 => found idx; non-0 => not found. */
static unsigned int fs_g_registered_idx(
	const unsigned int sid, const unsigned int sidx,
	unsigned int *p_idx, const char *caller)
{
	const unsigned int ident = fs_get_reg_sensor_ident(sid, sidx);

	return fs_g_registered_idx_by_ident(ident, p_idx, caller);
}
/******************************************************************************/





/******************************************************************************/
// call back struct data operation
/******************************************************************************/
static inline void fs_reset_cb_info(const unsigned int idx)
{
	memset(&fs_mgr.cb_info[idx], 0, sizeof(fs_mgr.cb_info[idx]));
}


static inline void fs_init_cb_info(const unsigned int idx,
	void *p_ctx, const callback_func_set_fl_info func_ptr)
{
	fs_mgr.cb_info[idx].func_ptr = func_ptr;
	fs_mgr.cb_info[idx].p_ctx = p_ctx;
}


static inline void fs_set_cb_cmd_id(const unsigned int idx,
	const unsigned int cmd_id)
{
	fs_mgr.cb_info[idx].cmd_id = cmd_id;
}


/******************************************************************************/





/******************************************************************************/
// preset perframe structure functions
/******************************************************************************/
static void fs_reset_preset_perframe_data(const unsigned int idx)
{
	/* reset/clear data */
	memset(&fs_mgr.preset_ctrl[idx], 0, sizeof(fs_mgr.preset_ctrl[idx]));
}


static void fs_try_setup_preset_perframe_data(const unsigned int idx,
	const struct fs_perframe_st *p_pf_ctrl)
{
	/* only copy pf_ctrl data when sensor is streaming OFF */
	if (fs_mgr.preset_ctrl[idx].is_streaming)
		return;

	/* keep preset pf_ctrl data */
	fs_mgr.preset_ctrl[idx].is_valid = 1;
	fs_mgr.preset_ctrl[idx].preset_pf_ctrl = *p_pf_ctrl;
}


static unsigned int fs_get_preset_perframe_data(const unsigned int idx,
	struct fs_perframe_st **p_p_pf_ctrl)
{
	unsigned int ret = 0;

	/* notify streaming ON */
	fs_mgr.preset_ctrl[idx].is_streaming = 1;

	if (fs_mgr.preset_ctrl[idx].is_valid) {
		/* copy preset pf_ctrl data */
		*p_p_pf_ctrl = &fs_mgr.preset_ctrl[idx].preset_pf_ctrl;
		ret = 1;
	}

	return ret;
}
/******************************************************************************/





/******************************************************************************/
// preset/overwrite set sync operation functions (set_sync_idx_table)
/******************************************************************************/
static void fs_update_set_sync_idx_table(const unsigned int idx,
	const unsigned int flag)
{
	FS_ATOMIC_SET(flag, &fs_mgr.set_sync_idx_table[idx]);

	LOG_INF(
		"[%u] ID:%#x(sidx:%u), flag:%u, => set_sync_idx_table[%u]:%d\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		flag, idx,
		FS_ATOMIC_READ(&fs_mgr.set_sync_idx_table[idx]));
}


static int fs_check_set_sync_idx_table(const unsigned int idx)
{
	int ret = -1;

	ret = FS_ATOMIC_READ(&fs_mgr.set_sync_idx_table[idx]);

	LOG_INF(
		"[%u] ID:%#x(sidx:%u), set_sync_idx_table[%u]:%d, => ret:%d\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		idx,
		FS_ATOMIC_READ(&fs_mgr.set_sync_idx_table[idx]),
		ret);

	return ret;
}


static unsigned int fs_chk_con_user_ovwr_set_sync_op(const unsigned int idx,
	unsigned int *p_flag, const char *caller)
{
#if !defined(FS_UT)
	const unsigned int sidx = fs_get_reg_sensor_idx(idx);
	unsigned int owr_flag = *p_flag;

	/* user cmd force disable frame-sync set_sync */
	if (unlikely(fs_con_chk_force_to_ignore_set_sync())) {
		LOG_MUST(
			"[%s] WARNING: [%u] ID:%#x(sidx:%u), USER set force ignoring set sync op, return 0\n",
			caller, idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx));
		return 0;
	}

	/* user cmd overwrite set sync flag */
	if (unlikely(fs_con_chk_en_overwrite_set_sync(sidx, &owr_flag))) {
		LOG_MUST(
			"[%s] NOTICE: [%u] ID:%#x(sidx:%u), USER set overwrite set sync value:(%u -> %u), return 2\n",
			caller, idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			*p_flag, owr_flag);
		*p_flag = owr_flag;
		return 2;
	}
#endif

	return 1;
}
/******************************************************************************/





/******************************************************************************/
// Frame Sync Mgr function
/******************************************************************************/
#ifdef SUPPORT_FS_NEW_METHOD
static void fs_init_members(void)
{
	unsigned int i = 0;

	FS_ATOMIC_INIT(0, &fs_mgr.reg_table.reg_cnt);
	FS_ATOMIC_INIT(0, &fs_mgr.fs_status);
	FS_ATOMIC_INIT(0, &fs_mgr.streaming_bits);
	FS_ATOMIC_INIT(0, &fs_mgr.enSync_bits);
	FS_ATOMIC_INIT(0, &fs_mgr.validSync_bits);
	FS_ATOMIC_INIT(0, &fs_mgr.pf_ctrl_bits);
	FS_ATOMIC_INIT(0, &fs_mgr.fl_restore_ctrl_bits);
	FS_ATOMIC_INIT(0, &fs_mgr.setup_complete_bits);
	FS_ATOMIC_INIT(0, &fs_mgr.seamless_bits);

	FS_ATOMIC_INIT(0, &fs_mgr.hw_sync_bits);
	for (i = 0; i < FS_HW_SYNC_GROUP_ID_MAX; ++i) {
		FS_ATOMIC_INIT(0, &fs_mgr.hw_sync_group_bits[i]);
		FS_ATOMIC_INIT(0, &fs_mgr.setup_complete_hw_group_bits[i]);
	}
	FS_ATOMIC_INIT(0, &fs_mgr.hw_sync_non_valid_group_bits);

	for (i = 0; i < SENSOR_MAX_NUM; ++i) {
		FS_ATOMIC_INIT(0, &fs_mgr.set_sync_idx_table[i]);

		FS_ATOMIC_INIT(0, &fs_mgr.seamless_ctrl[i].wait_for_processing);
	}

#if defined(SUPPORT_USING_CCU)
	FS_ATOMIC_INIT(0, &fs_mgr.power_on_ccu_bits);
#endif

	FS_ATOMIC_INIT(0, &fs_mgr.using_sa_ver);
	FS_ATOMIC_INIT(0, &fs_mgr.sa_bits);
	FS_ATOMIC_INIT(0, &fs_mgr.sa_method);
	FS_ATOMIC_INIT(MASTER_IDX_NONE, &fs_mgr.master_idx);
	FS_ATOMIC_INIT(0, &fs_mgr.async_mode_bits);
	FS_ATOMIC_INIT(MASTER_IDX_NONE, &fs_mgr.user_async_master_sidx);
	FS_ATOMIC_INIT(MASTER_IDX_NONE, &fs_mgr.async_master_idx);


#ifndef FS_UT
	for (i = 0; i < SENSOR_MAX_NUM; ++i)
		spin_lock_init(&fs_mgr.fl_restore_info_update_spinlock[i]);
#endif
}
#endif // SUPPORT_FS_NEW_METHOD


static void fs_init(void)
{
	enum FS_STATUS status = get_fs_status();

	fs_mgr.user_counter++;

	if (status == FS_NONE) {
#ifdef SUPPORT_FS_NEW_METHOD
		fs_init_members();
#endif // SUPPORT_FS_NEW_METHOD

		change_fs_status(FS_INITIALIZED);

		LOG_INF("FrameSync init. (User:%u)\n", fs_mgr.user_counter);
	} else if (status == FS_INITIALIZED)
		LOG_INF("Initialized. (User:%u)\n", fs_mgr.user_counter);

	// else if () => for re-init.
}


/*----------------------------------------------------------------------------*/
// functions --- for fl restore info struct/data
/*----------------------------------------------------------------------------*/
static void fs_clear_fl_restore_info(const unsigned int idx)
{
	struct fs_fl_restore_info_st *ptr = NULL;

	fs_spin_lock(&fs_mgr.fl_restore_info_update_spinlock[idx]);

	ptr = fs_mgr.fl_restore_info[idx];
	if (ptr == NULL)
		goto end_of_clear_fl_restore_info;

	memset(ptr, 0, sizeof(*ptr));
	FS_FREE(ptr);
	fs_mgr.fl_restore_info[idx] = NULL;

end_of_clear_fl_restore_info:
	fs_spin_unlock(&fs_mgr.fl_restore_info_update_spinlock[idx]);
}


static void fs_setup_fl_restore_info(const unsigned int idx,
	const struct fs_fl_restore_info_st *p_fl_restore_info)
{
	struct fs_fl_restore_info_st *ptr = NULL;

	fs_spin_lock(&fs_mgr.fl_restore_info_update_spinlock[idx]);

	ptr = fs_mgr.fl_restore_info[idx];
	/* check case */
	if (unlikely(ptr != NULL)) {
		LOG_MUST(
			"WARNING: [%u] ID:%#x(sidx:%u), detect previous FL restore info before overwrite it (setup FL restore info(%#x, #%u, req_id:%d, %u, %u/%u/%u/%u/%u))\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			FS_ATOMIC_READ(&fs_mgr.fl_restore_ctrl_bits),
			ptr->magic_num,
			ptr->req_id,
			ptr->restored_fl_lc,
			ptr->restored_fl_lc_arr[0],
			ptr->restored_fl_lc_arr[1],
			ptr->restored_fl_lc_arr[2],
			ptr->restored_fl_lc_arr[3],
			ptr->restored_fl_lc_arr[4]);
	} else {
		struct fs_fl_restore_info_st *ptr_new = NULL;

		/* alloc a mem */
		ptr_new = FS_CALLOC(1, sizeof(*ptr_new));
		if (unlikely(ptr_new == NULL)) {
			LOG_MUST(
				"ERROR: [%u] ID:%#x(sidx:%u), FL restore info struct allocate memory failed\n",
				idx,
				fs_get_reg_sensor_id(idx),
				fs_get_reg_sensor_idx(idx));
			goto end_of_set_fl_restore_info;
		}
		/* sync new mem address */
		fs_mgr.fl_restore_info[idx] = ptr_new;
		ptr = fs_mgr.fl_restore_info[idx];
	}
	memcpy(ptr, p_fl_restore_info, sizeof(*ptr));

end_of_set_fl_restore_info:
	fs_spin_unlock(&fs_mgr.fl_restore_info_update_spinlock[idx]);
}


static void fs_get_fl_restore_info(const unsigned int idx,
	struct fs_fl_restore_info_st *p_fl_restore_info)
{
	struct fs_fl_restore_info_st *ptr = NULL;

	fs_spin_lock(&fs_mgr.fl_restore_info_update_spinlock[idx]);

	ptr = fs_mgr.fl_restore_info[idx];
	/* error handling */
	if (unlikely(ptr == NULL)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u), fl_restore_ctrl_bits:%#x, but FL restore info is nullptr, return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			FS_ATOMIC_READ(&fs_mgr.fl_restore_ctrl_bits));
		goto end_of_get_fl_restore_info;
	}
	/* copy info */
	memcpy(p_fl_restore_info, ptr, sizeof(*p_fl_restore_info));

end_of_get_fl_restore_info:
	fs_spin_unlock(&fs_mgr.fl_restore_info_update_spinlock[idx]);
}


static void fs_reset_fl_restore_status(const unsigned int idx)
{
	if (likely(chk_xchg_bit_atomic(idx, 0, &fs_mgr.fl_restore_ctrl_bits) == 0))
		return;

	fs_clear_fl_restore_info(idx);

	LOG_INF(
		"NOTICE: [%u] ID:%#x(sidx:%u), clear FL restore info, (%#x, info:%p)\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		FS_ATOMIC_READ(&fs_mgr.fl_restore_ctrl_bits),
		fs_mgr.fl_restore_info[idx]);
}


void fs_setup_fl_restore_status(const unsigned int idx,
	const struct fs_fl_restore_info_st *p_fl_restore_info)
{
	fs_setup_fl_restore_info(idx, p_fl_restore_info);
	write_bit_atomic(idx, 1, &fs_mgr.fl_restore_ctrl_bits);

	LOG_INF(
		"[%u] ID:%#x(sidx:%u), setup FL restore info(%#x, #%u, req_id:%d, %u, %u/%u/%u/%u/%u)\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		FS_ATOMIC_READ(&fs_mgr.fl_restore_ctrl_bits),
		fs_mgr.fl_restore_info[idx]->magic_num,
		fs_mgr.fl_restore_info[idx]->req_id,
		fs_mgr.fl_restore_info[idx]->restored_fl_lc,
		fs_mgr.fl_restore_info[idx]->restored_fl_lc_arr[0],
		fs_mgr.fl_restore_info[idx]->restored_fl_lc_arr[1],
		fs_mgr.fl_restore_info[idx]->restored_fl_lc_arr[2],
		fs_mgr.fl_restore_info[idx]->restored_fl_lc_arr[3],
		fs_mgr.fl_restore_info[idx]->restored_fl_lc_arr[4]);
}
/*----------------------------------------------------------------------------*/


static void fs_chk_fl_info_updated_from_drv(const unsigned int idx,
	const struct fs_perframe_st *update_ctrl, const char *caller)
{
	const struct fs_perframe_st *output_ctrl = &fs_mgr.pf_ctrl[idx];
	const unsigned int chk_diff_th = 32; // lines count
	unsigned int chk_diff;
	unsigned int i;

	chk_diff = (update_ctrl->out_fl_lc >= output_ctrl->out_fl_lc)
		? (update_ctrl->out_fl_lc - output_ctrl->out_fl_lc)
		: (output_ctrl->out_fl_lc - update_ctrl->out_fl_lc);
	if (unlikely(chk_diff >= chk_diff_th)) {
		LOG_MUST(
			"[%s] ERROR: [%u] ID:%#x(sidx:%u), (%d/%u), out_fl_lc(FS:%u/drv:%u), diff(%u/th:%u), FL value set by sensor drv seems to not match the value output by Frame-Sync\n",
			caller,
			idx,
			output_ctrl->sensor_id,
			output_ctrl->sensor_idx,
			output_ctrl->req_id,
			fs_mgr.sof_cnt_arr[idx],
			output_ctrl->out_fl_lc,
			update_ctrl->out_fl_lc,
			chk_diff, chk_diff_th);
	}

	for (i = 0; i < FS_HDR_MAX; ++i) {
		chk_diff = (update_ctrl->hdr_exp.fl_lc[i] >= output_ctrl->hdr_exp.fl_lc[i])
			? (update_ctrl->hdr_exp.fl_lc[i] - output_ctrl->hdr_exp.fl_lc[i])
			: (output_ctrl->hdr_exp.fl_lc[i] - update_ctrl->hdr_exp.fl_lc[i]);
		if (unlikely(chk_diff >= chk_diff_th)) {
			LOG_MUST(
				"[%s] ERROR: [%u] ID:%#x(sidx:%u), (%d/%u), hdr_exp.fl_lc[%u](FS:%u/drv:%u), diff(%u/th:%u), FL value set by sensor drv seems to not match the value output by Frame-Sync\n",
				caller,
				idx,
				output_ctrl->sensor_id,
				output_ctrl->sensor_idx,
				output_ctrl->req_id,
				fs_mgr.sof_cnt_arr[idx],
				i,
				output_ctrl->hdr_exp.fl_lc[i],
				update_ctrl->hdr_exp.fl_lc[i],
				chk_diff, chk_diff_th);
		}
	}
}


#ifdef SUPPORT_FS_NEW_METHOD
/*
 * Support user/custom setting for using FrameSync StandAlone(SA) mode
 */
void fs_set_using_sa_mode(const unsigned int en)
{
	fs_mgr.user_set_sa = (en > 0) ? 1 : 0;
}


unsigned int fs_is_hw_sync(const unsigned int ident)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return 0;

	return FS_CHECK_BIT(idx, &fs_mgr.hw_sync_bits);
}


static void fs_set_hw_sync_info(const unsigned int idx, const unsigned int flag,
	const unsigned int hw_sync_mode, const unsigned int hw_sync_group_id)
{
	/* means no using HW solution */
	/* hw sync mode equal to 0 (means using SW solution) */
	/* will be retruned at the start of this function, */
	/* so exclude that case should all using hw sync solution */
	if (hw_sync_mode == 0)
		return;

	/* error handling */
	if (unlikely(check_idx_valid(idx) == 0)) {
		LOG_MUST(
			"ERROR: [idx:%u] is not valid (MIN:0/MAX:%u), return\n",
			idx, SENSOR_MAX_NUM);
		return;
	}


	/* set(stream ON)/clear(stream OFF) hw sync bits */
	if (flag)
		FS_WRITE_BIT(idx, 1, &fs_mgr.hw_sync_bits);
	else
		FS_WRITE_BIT(idx, 0, &fs_mgr.hw_sync_bits);


	/* set hw sync group bits */
	fs_mgr.hw_sync_group_id[idx] = hw_sync_group_id;
	if (hw_sync_group_id >= FS_HW_SYNC_GROUP_ID_MAX) {
		/* error handling (de-reference non-valid address) */
		if (flag) {
			FS_WRITE_BIT(idx, 1,
				&fs_mgr.hw_sync_non_valid_group_bits);
		} else {
			FS_WRITE_BIT(idx, 0,
				&fs_mgr.hw_sync_non_valid_group_bits);
		}

		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u) Non-valid value of hw_sync_group_id:%d (MIN:%d/MAX:%d), no apply(curr:%u), non_valid_group_bits:%d  [en:%u]\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			hw_sync_group_id,
			FS_HW_SYNC_GROUP_ID_MIN,
			FS_HW_SYNC_GROUP_ID_MAX,
			fs_mgr.hw_sync_group_id[idx],
			FS_READ_BITS(&fs_mgr.hw_sync_non_valid_group_bits),
			flag);

		return;
	}


	if (flag) {
		FS_WRITE_BIT(idx, 1,
			&fs_mgr.hw_sync_group_bits[hw_sync_group_id]);
	} else {
		FS_WRITE_BIT(idx, 0,
			&fs_mgr.hw_sync_group_bits[hw_sync_group_id]);
	}

	/* clear/reset ctrls setup complete hw group bit */
	FS_WRITE_BIT(idx, 0,
		&fs_mgr.setup_complete_hw_group_bits[hw_sync_group_id]);

	/* clear/reset frame-sync act cnt */
	fs_mgr.act_cnt[hw_sync_group_id] = 0;


	LOG_MUST(
		"[%u] ID:%#x(sidx:%u) en:%u, hw_sync(mode:%u(N:0/M:1/S:2), group_id:%u) [hw_sync(bits:%u, group_bits(%d/%d/%d/%d/%d/%d), setup_complete(%d/%d/%d/%d/%d/%d), act_cnt(%u/%u/%u/%u/%u/%u)), non_valid_group_bits:%d]\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		flag,
		hw_sync_mode,
		fs_mgr.hw_sync_group_id[idx],
		FS_READ_BITS(&fs_mgr.hw_sync_bits),
		FS_READ_BITS(
			&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_0]),
		FS_READ_BITS(
			&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_1]),
		FS_READ_BITS(
			&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_2]),
		FS_READ_BITS(
			&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_3]),
		FS_READ_BITS(
			&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_4]),
		FS_READ_BITS(
			&fs_mgr.hw_sync_group_bits[FS_HW_SYNC_GROUP_ID_5]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_0]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_1]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_2]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_3]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_4]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_5]),
		fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_0],
		fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_1],
		fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_2],
		fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_3],
		fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_4],
		fs_mgr.act_cnt[FS_HW_SYNC_GROUP_ID_5],
		FS_READ_BITS(&fs_mgr.hw_sync_non_valid_group_bits));
}
#endif // SUPPORT_FS_NEW_METHOD


static int fs_update_valid_sync_bit(void)
{
	int streaming = 0, en_sync = 0;

	streaming = FS_ATOMIC_READ(&fs_mgr.streaming_bits);
	en_sync = FS_ATOMIC_READ(&fs_mgr.enSync_bits);

	FS_ATOMIC_SET((streaming & en_sync), &fs_mgr.validSync_bits);

	return (streaming & en_sync);
}


static void fs_update_status(const unsigned int idx,
	const unsigned int flag, const char *caller)
{
	unsigned int cnt = 0;
	int valid_sync = 0;

	/* update validSync_bits value */
	valid_sync = fs_update_valid_sync_bit();

	/* change status or not by counting the number of valid sync sensors */
	cnt = FS_POPCOUNT(valid_sync);

	if (cnt > 1 && cnt <= SENSOR_MAX_NUM)
		change_fs_status(FS_WAIT_FOR_SYNCFRAME_START);
	else
		change_fs_status(FS_INITIALIZED);

	fs_dump_status(idx, flag, caller, "");
}


static inline void fs_set_status_bits(const unsigned int idx,
	const unsigned int flag, FS_Atomic_T *bits)
{
	if (flag > 0)
		write_bit_atomic(idx, 1, bits);
	else
		write_bit_atomic(idx, 0, bits);
}


static inline void fs_set_stream(unsigned int idx, unsigned int flag)
{
	fs_set_status_bits(idx, flag, &fs_mgr.streaming_bits);
}


static inline void fs_reset_ft_mode_data(unsigned int idx, unsigned int flag)
{
	if (flag > 0)
		return;

	fs_mgr.hdr_ft_mode[idx] = 0;
	fs_mgr.frame_cell_size[idx] = 0;
	fs_mgr.frame_tag[idx] = 0;
}


static void fs_reset_perframe_stage_data(
	const unsigned int idx, const unsigned int flag)
{
	struct fs_perframe_st clear_st = {0};

	if (flag > 0)
		return;

	fs_mgr.pf_ctrl[idx] = clear_st;

	fs_alg_reset_vsync_data(idx);
}


/*---------------------------------------------------------------------------*/
// seamless switch functions
/*---------------------------------------------------------------------------*/
static unsigned int fs_chk_seamless_switch_status(const unsigned int idx)
{
	return FS_CHECK_BIT(idx, &fs_mgr.seamless_bits);
}


static void fs_clr_seamless_switch_info(const unsigned int idx)
{
	/* error handling (unexpected case) */
	if (unlikely(check_idx_valid(idx) == 0)) {
		LOG_MUST(
			"NOTICE: [%u] %s is not register, return\n",
			idx, REG_INFO);
		return;
	}

	fs_set_status_bits(idx, 0, &fs_mgr.seamless_bits);

	memset(&fs_mgr.seamless_ctrl[idx], 0, sizeof(fs_mgr.seamless_ctrl[idx]));

	LOG_INF(
		"[%u] ID:%#x(sidx:%u), seamless:%#x, seamless_sof_cnt:%u (cleared), SA(m_idx:%d)\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		FS_ATOMIC_READ(&fs_mgr.seamless_bits),
		fs_mgr.seamless_ctrl->seamless_sof_cnt,
		FS_ATOMIC_READ(&fs_mgr.master_idx));
}


static void fs_set_seamless_switch_info(const unsigned int idx,
	struct fs_seamless_st *p_seamless_info,
	const unsigned int seamless_sof_cnt)
{
	/* error handling (unexpected case) */
	if (unlikely(check_idx_valid(idx) == 0)) {
		LOG_MUST(
			"NOTICE: [%u] %s is not register, return\n",
			idx, REG_INFO);
		return;
	}

	if (unlikely(p_seamless_info == NULL)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u), get p_seamless_info:%p, return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			p_seamless_info);
		return;
	}

	fs_set_status_bits(idx, 1, &fs_mgr.seamless_bits);

	/* !!! setup fs_seamless_ctrl_st data !!! */
	/* keep seamless switch ctrl information */
	/* keep here & check for clear data when exit seamless frame */
	FS_ATOMIC_SET(1, &fs_mgr.seamless_ctrl[idx].wait_for_processing);
	fs_mgr.seamless_ctrl[idx].seamless_sof_cnt = seamless_sof_cnt;
	fs_mgr.seamless_ctrl[idx].seamless_info = *p_seamless_info;

	/* change SA alg master to this sensor */
	/* before keepping seamless ctrl */
	fs_sa_request_switch_master(idx);

	LOG_INF(
		"[%u] ID:%#x(sidx:%u), seamless:%#x, seamless_sof_cnt:%u, wait_for_processing:%d, SA(m_idx:%d), orig_readout_time_us:%u\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		FS_ATOMIC_READ(&fs_mgr.seamless_bits),
		fs_mgr.seamless_ctrl[idx].seamless_sof_cnt,
		FS_ATOMIC_READ(&fs_mgr.seamless_ctrl[idx].wait_for_processing),
		FS_ATOMIC_READ(&fs_mgr.master_idx),
		fs_mgr.seamless_ctrl[idx].seamless_info.prop.orig_readout_time_us);
}


static void fs_do_seamless_switch_proc(const unsigned int idx,
	const struct fs_sa_cfg *p_sa_cfg)
{
	struct fs_seamless_st *p_seamless_info = NULL;
	struct fs_perframe_st *p_seamless_ctrl = NULL;
	struct frec_seamless_st seamless_rec = {0};

	/* error handling (unexpected case) */
	if (unlikely(check_idx_valid(idx) == 0)) {
		LOG_MUST(
			"NOTICE: [%u] %s is not register, return\n",
			idx, REG_INFO);
		return;
	}

	if (unlikely(p_sa_cfg == NULL)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u), get p_sa_cfg:%p, return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			p_sa_cfg);
		return;
	}

	/* read back/get seamless ctrl that keep last time */
	/* and prevent any issue overwriting last pf ctrl exp and fl */
	p_seamless_info = &fs_mgr.seamless_ctrl[idx].seamless_info;
	p_seamless_ctrl = &fs_mgr.seamless_ctrl[idx].seamless_info.seamless_pf_ctrl;

	frec_setup_seamless_rec_by_fs_seamless_st(
		&seamless_rec, p_seamless_info);
	frec_seamless_switch(idx, p_seamless_ctrl->min_fl_lc, &seamless_rec);

	fs_alg_set_perframe_st_data(idx, &p_seamless_info->seamless_pf_ctrl);
	fs_alg_seamless_switch(idx, p_seamless_info, p_sa_cfg);
}
/*---------------------------------------------------------------------------*/


#ifdef SUPPORT_FS_NEW_METHOD
static inline void fs_check_sync_need_sa_mode(
	unsigned int idx, enum FS_HDR_FT_MODE flag)
{
	/* NOT FS_HDR_FT_MODE_NORMAL => using SA mode */
	if (flag != FS_HDR_FT_MODE_NORMAL) {
		write_bit_atomic(idx, 1, &fs_mgr.sa_bits);

#if !defined(REDUCE_FS_DRV_LOG)
		LOG_INF(
			"[%u] ID:%#x(sidx:%u), hdr_ft_mode:%u(need SA mode, except 0 for normal)   [sa_bits:%d]\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			flag,
			FS_ATOMIC_READ(&fs_mgr.sa_bits));
#endif // REDUCE_FS_DRV_LOG
	}
}


static void fs_decision_maker(unsigned int idx)
{
	int sa_bits_now = 0;
	int need_sa_ver = 0;
	int user_sa_en = 0;


	/* 1. check sync tag of the sensor need use SA mode or not */
	fs_check_sync_need_sa_mode(idx, fs_mgr.hdr_ft_mode[idx]);


	/* 2. check all sensor for using SA mode or not */
	do {
		sa_bits_now = FS_ATOMIC_READ(&fs_mgr.sa_bits);

		need_sa_ver = (FS_POPCOUNT(sa_bits_now) > 0) ? 1 : 0;

#ifdef FS_UT
	} while (!atomic_compare_exchange_weak(&fs_mgr.sa_bits,
			&sa_bits_now, sa_bits_now));
#else
	} while (atomic_cmpxchg(&fs_mgr.sa_bits,
			sa_bits_now, sa_bits_now) != sa_bits_now);
#endif // FS_UT


	/* 3. check user/custom SA config EN */
	user_sa_en = fs_user_sa_config();


	/* X. setup result of using SA mode or not */
	if (need_sa_ver > 0 || user_sa_en > 0)
		FS_ATOMIC_SET(1, &fs_mgr.using_sa_ver);
	else
		FS_ATOMIC_SET(0, &fs_mgr.using_sa_ver);


#if !defined(FORCE_USING_SA_MODE) || !defined(REDUCE_FS_DRV_LOG)
	LOG_MUST(
		"using_sa:%d (user_sa_en:%u), hdr_ft_mode:%u, sa_bits:%d [idx:%u]\n",
		FS_ATOMIC_READ(&fs_mgr.using_sa_ver),
		user_sa_en,
		fs_mgr.hdr_ft_mode[idx],
		FS_ATOMIC_READ(&fs_mgr.sa_bits),
		idx);
#endif
}


/*
 * if the input, idx, un-set FrameSync set sync, and is SA master idx,
 * => true: reset SA master idx to MASTER_IDX_NONE
 * => false: do nothing.
 */
static void fs_sa_try_reset_master_idx(const unsigned int idx,
	const unsigned int flag)
{
	int master_idx = MASTER_IDX_NONE;


	/* The sensor status is already set sync => do nothing */
	if (flag > 0)
		return;

	/* check situation for needing reset SA master idx */
	do {
		master_idx = FS_ATOMIC_READ(&fs_mgr.master_idx);

#ifdef FS_UT
	} while (!atomic_compare_exchange_weak(&fs_mgr.master_idx,
			&master_idx,
			((idx == master_idx)
				? MASTER_IDX_NONE : master_idx)));
#else
	} while (atomic_cmpxchg(&fs_mgr.master_idx,
			master_idx,
			((idx == master_idx)
				? MASTER_IDX_NONE : master_idx))
		!= master_idx);
#endif // FS_UT
}


/*
 * This function will change async_master_idx value immediately!
 * Only call this function when you confirmed that
 *     this instance idx of sensor is valid for doing frame-sync!
 */
static void fs_sa_update_async_master_idx(const unsigned int idx,
	const unsigned int flag)
{
	const unsigned int sidx = fs_get_reg_sensor_idx(idx);
	const int async_m_idx = FS_ATOMIC_READ(&fs_mgr.async_master_idx);
	int user_async_m_sidx = FS_ATOMIC_READ(&fs_mgr.user_async_master_sidx);

#if !defined(FS_UT)
	int ret = fs_con_chk_usr_async_m_sidx();

	/* user cmd overwrite set sync flag */
	if (unlikely(ret != MASTER_IDX_NONE)) {
		user_async_m_sidx = ret;
		LOG_MUST(
			"NOTICE: [%u] ID:%#x(sidx:%u), USER set async master sidx:%u => user_async_m_sidx:%d\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			ret,
			user_async_m_sidx);
	}
#endif // FS_UT

	/* case handling */
	/* --- check user input value is valid or not */
	if (unlikely((user_async_m_sidx != MASTER_IDX_NONE)
			&& (user_async_m_sidx < 0))) {
		LOG_MUST(
			"[%u] ERROR: ID:%#x(sidx:%u), invalid user_async_master_sidx:%d value, return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			user_async_m_sidx);
		return;
	}

	if (flag > 0) {
		if (sidx != user_async_m_sidx)
			return;

		/* set to this instance idx for using */
		FS_ATOMIC_SET(idx, &fs_mgr.async_master_idx);
	} else {
		if (idx != async_m_idx)
			return;

		FS_ATOMIC_SET(MASTER_IDX_NONE, &fs_mgr.async_master_idx);
	}

	LOG_INF(
		"[%u] ID:%#x(sidx:%u), flag:%u, async_master(user_sidx:%d/curr_idx:%d->%d) [streaming:%d/enSync:%d/validSync:%d]\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		flag,
		user_async_m_sidx,
		async_m_idx,
		FS_ATOMIC_READ(&fs_mgr.async_master_idx),
		FS_ATOMIC_READ(&fs_mgr.streaming_bits),
		FS_ATOMIC_READ(&fs_mgr.enSync_bits),
		FS_ATOMIC_READ(&fs_mgr.validSync_bits));
}


void fs_sa_set_user_async_master(const unsigned int sidx,
	const unsigned int flag)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(sidx, &idx, __func__)))
		return;

	if (flag > 0)
		FS_ATOMIC_SET(sidx, &fs_mgr.user_async_master_sidx);
	else
		FS_ATOMIC_SET(MASTER_IDX_NONE, &fs_mgr.user_async_master_sidx);

	/* if sensor is registered and valid for doing frame-sync, */
	/* (streaming ON + set sync) update async master idx simultaneously */
	if (check_idx_valid(idx)
		&& FS_CHECK_BIT(idx, &fs_mgr.validSync_bits)) {
		fs_sa_update_async_master_idx(idx, flag);

		/* log info can be see by above function */
		return;
	}
}


static inline void fs_sa_set_async_info(const unsigned int idx,
	const unsigned int flag)
{
	const unsigned int async_en = (flag & FS_SYNC_TYPE_ASYNC_MODE);

	FS_WRITE_BIT(idx, async_en, &fs_mgr.async_mode_bits);
}
#endif // SUPPORT_FS_NEW_METHOD


static void fs_set_sync_status(unsigned int idx, unsigned int flag)
{
	/* unset sync => reset pf_ctrl_bits data of this idx */
	if (flag == 0) {
		FS_WRITE_BIT(idx, 0, &fs_mgr.pf_ctrl_bits);
		FS_WRITE_BIT(idx, 0, &fs_mgr.setup_complete_bits);
	}

#if defined(SUPPORT_USING_CCU) && defined(DELAY_CCU_OP)
	if (frm_get_ts_src_type() == FS_TS_SRC_CCU) {
		if (flag == 0) {
			if (FS_CHECK_BIT(idx, &fs_mgr.power_on_ccu_bits) == 1) {
				FS_WRITE_BIT(idx, 0, &fs_mgr.power_on_ccu_bits);

				/* power off CCU and disable vsync INTR */
				frm_reset_ccu_vsync_timestamp(idx, 0);
				frm_power_on_ccu(0);

				fs_alg_reset_vsync_data(idx);
			}
		} else {
			if (FS_CHECK_BIT(idx, &fs_mgr.power_on_ccu_bits) == 0) {
				FS_WRITE_BIT(idx, 1, &fs_mgr.power_on_ccu_bits);

				fs_alg_reset_vsync_data(idx);

				/* power on CCU and enable vsync INTR */
				frm_power_on_ccu(1);
				frm_reset_ccu_vsync_timestamp(idx, 1);
			}
		}
	}
#endif

	fs_set_status_bits(idx, flag, &fs_mgr.enSync_bits);
}


static void fs_set_sync_idx(unsigned int idx, unsigned int flag)
{
	fs_set_sync_status(idx, flag);

	fs_alg_set_sync_type(idx, flag);

	fs_sa_set_async_info(idx, flag);

#if defined(FS_UT)
	fs_reset_ft_mode_data(idx, flag);
#endif // FS_UT

	fs_sa_try_reset_master_idx(idx, flag);

	fs_sa_update_async_master_idx(idx, flag);

	fs_decision_maker(idx);

	fs_update_status(idx, flag, __func__);
}


void fs_set_sync(unsigned int ident, unsigned int flag)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

	if (unlikely(!fs_chk_con_user_ovwr_set_sync_op(idx, &flag, __func__)))
		return;

	fs_update_set_sync_idx_table(idx, flag);
	fs_set_sync_idx(idx, flag);
}


unsigned int fs_is_set_sync(unsigned int ident)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return 0;

	return FS_CHECK_BIT(idx, &fs_mgr.enSync_bits);
}


static void fs_cb_fsync_ctrl_to_set_fl_info(
	const unsigned int idx, const unsigned int cmd_id,
	const unsigned int fl_lc,
	const unsigned int fl_lc_arr[], const unsigned int arr_len)
{
	const struct callback_info_st *cb_info = &fs_mgr.cb_info[idx];
	int ret;

	if (unlikely(cb_info->func_ptr == NULL)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u), callback function pointer:%p is NULL\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			cb_info->func_ptr);
		return;
	}

	/* use callback function to call to sensor driver for setup fl info */
	ret = cb_info->func_ptr(cb_info->p_ctx, cmd_id, &fs_mgr.pf_ctrl[idx],
		fl_lc, fl_lc_arr, FS_HDR_MAX);

	if (unlikely(ret != 0)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u), ret:%d, p_ctx:%p, cmd_id:%u(NONE:%u/shutter_with_FL:%u/FL:%u), &pf_info:%p, fl_lc:%u, fl_lc_arr:(%u/%u/%u/%u/%u), arr_len:%u\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			ret,
			cb_info->p_ctx,
			cmd_id,
			FSYNC_CTRL_FL_CMD_ID_NONE,
			FSYNC_CTRL_FL_CMD_ID_EXP_WITH_FL,
			FSYNC_CTRL_FL_CMD_ID_FL,
			&fs_mgr.pf_ctrl[idx],
			fl_lc,
			fl_lc_arr[0],
			fl_lc_arr[1],
			fl_lc_arr[2],
			fl_lc_arr[3],
			fl_lc_arr[4],
			FS_HDR_MAX);
	}
}


/*
 * set shutter w/ frame length register operation must use this API
 * SW Frame-Sync default should be:
 *     FSYNC_CTRL_FL_CMD_ID_EXP_WITH_FL corresponds to
 *     SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME
 */
static void fs_set_framelength_lc(const unsigned int idx,
	const unsigned int fl_lc)
{
	const unsigned int cmd_id = fs_mgr.cb_info[idx].cmd_id;
	unsigned int out_fl_lc = 0;
	unsigned int fl_lc_arr[FS_HDR_MAX] = {0};

	if (unlikely(fl_lc == 0))
		return;

	/* get fs algo output fl info and sync results */
	fs_alg_get_out_fl_info(idx, &out_fl_lc, fl_lc_arr, FS_HDR_MAX);

	/* update to pf ctrl */
	fs_mgr.pf_ctrl[idx].out_fl_lc = out_fl_lc;
	memcpy(fs_mgr.pf_ctrl[idx].hdr_exp.fl_lc, fl_lc_arr,
		(sizeof(unsigned int) * FS_HDR_MAX));

	fs_cb_fsync_ctrl_to_set_fl_info(idx, cmd_id,
		fl_lc, fl_lc_arr, FS_HDR_MAX);

#ifdef FS_UT
	fs_alg_setup_frame_monitor_fmeas_data(idx);
#endif
}


static void fs_reject_frame_length_ctrl(const unsigned int idx)
{
	const unsigned int cmd_id = fs_mgr.cb_info[idx].cmd_id;
	const unsigned int fl_drv_handle_val = 0;
	const unsigned int fl_lc_arr[FS_HDR_MAX] = {fl_drv_handle_val};

	/* let fl to drv self ctrl => set value to 0 */
	fs_cb_fsync_ctrl_to_set_fl_info(idx, cmd_id,
		fl_drv_handle_val, fl_lc_arr, FS_HDR_MAX);

	LOG_MUST(
		"NOTICE: [%u] ID:%#x(sidx:%u), let FL be controlled by sensor drv, set FL:(%u, %u/%u/%u/%u/%u)\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fl_drv_handle_val,
		fl_lc_arr[0],
		fl_lc_arr[1],
		fl_lc_arr[2],
		fl_lc_arr[3],
		fl_lc_arr[4]);
}


#ifdef SUPPORT_FS_NEW_METHOD
static inline unsigned int fs_check_n_1_status_ctrl(
	unsigned int idx, unsigned int en)
{
	enum FS_HDR_FT_MODE mode_status = fs_mgr.hdr_ft_mode[idx];

	if (en) {
		/* only normal mode can turn ON N:1 mode */
		return (mode_status == 0) ? 1 : 0;
	}

	/* only FRAME_TAG and N_1_KEEP can turn OFF N:1 mode */
	return (mode_status
		& (FS_HDR_FT_MODE_FRAME_TAG | FS_HDR_FT_MODE_N_1_KEEP))
		? 1 : 0;
}


static inline void fs_check_n_1_status_extra_ctrl(unsigned int idx)
{
	enum FS_HDR_FT_MODE old_ft_status;

	old_ft_status = fs_mgr.hdr_ft_mode[idx];

	if (fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_N_1_ON) {
		fs_mgr.hdr_ft_mode[idx] &= ~(FS_HDR_FT_MODE_N_1_ON);
		fs_mgr.hdr_ft_mode[idx] |= FS_HDR_FT_MODE_N_1_KEEP;

		LOG_MUST(
			"[%u] ID:%#x(sidx:%u), feature mode status change (%u->%u), ON:%u/KEEP:%u\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			old_ft_status,
			fs_mgr.hdr_ft_mode[idx],
			fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_N_1_ON,
			fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_N_1_KEEP
		);

		/* turn off FS algo n_1_on_off flag */
		/* for calculating FL normally */
		fs_alg_set_n_1_on_off_flag(idx, 0);

	} else if (fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_N_1_OFF) {
		fs_mgr.hdr_ft_mode[idx] &= ~(FS_HDR_FT_MODE_N_1_OFF);
		fs_mgr.hdr_ft_mode[idx] &= ~(FS_HDR_FT_MODE_FRAME_TAG);
		fs_mgr.hdr_ft_mode[idx] |= FS_HDR_FT_MODE_NORMAL;

		LOG_MUST(
			"[%u] ID:%#x(sidx:%u), feature mode status change (%u->%u), OFF:%u/FRAME_TAG:%u/NORMAL:%u\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			old_ft_status,
			fs_mgr.hdr_ft_mode[idx],
			fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_N_1_OFF,
			fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_FRAME_TAG,
			fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_NORMAL
		);

		/* turn off FS algo n_1_on_off flag */
		/* for calculating FL normally */
		fs_alg_set_n_1_on_off_flag(idx, 0);
	}
}


static inline void fs_feature_status_extra_ctrl(unsigned int idx)
{
	fs_check_n_1_status_extra_ctrl(idx);
}


void fs_sa_request_switch_master(unsigned int idx)
{
	int sa_method = -1;


	sa_method = FS_ATOMIC_READ(&fs_mgr.sa_method);

	if (sa_method != FS_SA_ADAPTIVE_MASTER) {
		LOG_MUST(
			"ERROR: SA method:%d, but request (from idx:%u) switch master sensor, vdiff will not be adjusted\n",
			sa_method,
			idx);

		return;
	}


	FS_ATOMIC_SET(idx, &fs_mgr.master_idx);


#if !defined(REDUCE_FS_DRV_LOG)
	LOG_INF(
		"switch master instance idx to %d  (from idx:%u)\n",
		FS_ATOMIC_READ(&fs_mgr.master_idx),
		idx);
#endif // REDUCE_FS_DRV_LOG
}


static int fs_get_valid_master_instance_idx(const unsigned int idx)
{
	const int async_mode_bits = FS_ATOMIC_READ(&fs_mgr.async_mode_bits);
	const int valid_bits = FS_ATOMIC_READ(&fs_mgr.validSync_bits);
	const int m_idx = FS_ATOMIC_READ(&fs_mgr.master_idx);
	int i = 0;

	/* check case */
	/* --- user not config master sensor */
	/*     or config a invalid sensor as master */
	/*     invalid: (not set sync || uses async mode) */
	if (m_idx == MASTER_IDX_NONE
		|| (((valid_bits >> m_idx) & 0x01) == 0)
		|| (((async_mode_bits >> m_idx) & 0x01) == 1)) {
		/* auto select a master sensor */
		i = 0;
		while ((i < SENSOR_MAX_NUM)
			&& ((((valid_bits >> i) & 0x01) != 1)
				|| ((async_mode_bits >> i) & 0x01) == 1))
			i++;

		/* no match => set to itself & preventing OOB */
		if (i >= SENSOR_MAX_NUM) {
			i = idx;

			/* unexpected case */
			if (async_mode_bits == 0) {
				LOG_MUST(
					"[%u] NOTICE: can't auto select SA master idx, force set to itself, m_idx:%d [validSync:%u/Async_Mode_bits:%d]\n",
					idx, i, valid_bits, async_mode_bits);
			}
		}

		FS_ATOMIC_SET(i, &fs_mgr.master_idx);

#if !defined(REDUCE_FS_DRV_LOG)
		LOG_MUST(
			"NOTICE: current master_idx:%d is not valid for doing frame-sync, validSync_bits:%d, Async_Mode_bits:%d, auto set to idx:%d\n",
			m_idx, valid_bits, async_mode_bits, i);
#endif

		return i;
	}

	return m_idx;
}


static int fs_get_valid_async_master_instance_idx(const unsigned int idx)
{
	const int async_mode_bits = FS_READ_BITS(&fs_mgr.async_mode_bits);
	const int valid_bits = FS_ATOMIC_READ(&fs_mgr.validSync_bits);
	const int async_m_idx = FS_ATOMIC_READ(&fs_mgr.async_master_idx);

	/* check async master idx */
	/* --- no sensor uses async mode => it's fine */
	if (async_mode_bits == 0)
		return MASTER_IDX_NONE;

	/* --- has async_mode sensor => check async_m_idx is valid */
	if ((async_mode_bits != 0) && ((async_m_idx != MASTER_IDX_NONE)
		&& ((valid_bits >> async_m_idx) & 0x01)))
		return async_m_idx;

	/* un-wish case handling */
	/* --- async master idx is not valid */
	if ((async_mode_bits != 0) && ((async_m_idx == MASTER_IDX_NONE)
		|| (((valid_bits >> async_m_idx) & 0x01) == 0))) {
		fs_dump_status(idx, -1, __func__,
			"WARNING: async_m_sidx is invalid for using, auto assign to itself");
		return idx;
	}

	LOG_MUST(
		"[%u] WARNING: undefined case, return async_m_idx:%u (itself) for using\n",
		idx, idx);

	return idx;
}


static inline void fs_sa_setup_perframe_cfg_info(const unsigned int idx,
	struct fs_sa_cfg *p_sa_cfg)
{
	p_sa_cfg->idx = idx;
	p_sa_cfg->sa_method = FS_ATOMIC_READ(&fs_mgr.sa_method);
	p_sa_cfg->m_idx = fs_get_valid_master_instance_idx(idx);
	p_sa_cfg->valid_sync_bits = FS_READ_BITS(&fs_mgr.validSync_bits);
	p_sa_cfg->async_m_idx = fs_get_valid_async_master_instance_idx(idx);
	p_sa_cfg->async_s_bits = FS_READ_BITS(&fs_mgr.async_mode_bits);

#if !defined(REDUCE_FS_DRV_LOG)
	LOG_MUST(
		"[%u] idx:%u, sa_mthod:%u, m_idx:%d, valid_sync_bits:%#x, async_m_idx:%d, async_s_bits:%#x\n",
		idx,
		p_sa_cfg->idx,
		p_sa_cfg->sa_method,
		p_sa_cfg->m_idx,
		p_sa_cfg->valid_sync_bits,
		p_sa_cfg->async_m_idx,
		p_sa_cfg->async_s_bits);
#endif
}


static void fs_try_trigger_frame_sync_sa(const unsigned int idx)
{
	struct fs_sa_cfg sa_cfg = {0};
	unsigned int fl_lc = 0;
	unsigned int ret = 0;

	/* unexpected case check (but it should be detect in previous caller) */
	if (unlikely(
		(FS_CHECK_BIT(idx, &fs_mgr.validSync_bits) == 0)
		|| (FS_CHECK_BIT(idx, &fs_mgr.pf_ctrl_bits) == 0)
		|| (FS_CHECK_BIT(idx, &fs_mgr.setup_complete_bits) == 0))) {
		/* call dump all info for seeing what's happened */
		fs_dump_status(idx, -1, __func__,
			"ERROR: invalid for doing frame-sync, check validSync/pf_ctrl/setup_complete");
		return;
	}

	/* clear status bit */
	/* --- because it's valid for trigger FL calculation */
	FS_WRITE_BIT(idx, 0, &fs_mgr.pf_ctrl_bits);
	FS_WRITE_BIT(idx, 0, &fs_mgr.setup_complete_bits);

	/* trigger FS-SA for calculating frame length, */
	/*    but only set FL to sensor driver if return with no error */
	fs_sa_setup_perframe_cfg_info(idx, &sa_cfg);
	ret = fs_alg_solve_frame_length_sa(&sa_cfg, &fl_lc);

	if (ret != 0) { // !0 => had error
		LOG_INF(
			"ERROR: [%u] ID:%#x(sidx:%u), fs_alg_solve_frame_length_sa return error:%u, auto set fl_lc:%u to min_fl_lc for sensor driver",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			ret,
			fl_lc);
	}

	/* set framelength (all FL operation must use this API) */
	fs_set_framelength_lc(idx, fl_lc);

	/* check/change feature mode status */
	fs_feature_status_extra_ctrl(idx);
}


static void fs_reset_frame_tag(unsigned int idx)
{
	fs_mgr.frame_tag[idx] = 0;

	fs_alg_set_frame_tag(idx, fs_mgr.frame_tag[idx]);
}


static inline void fs_try_set_auto_frame_tag(unsigned int idx)
{
	unsigned int f_tag = 0, f_cell = 0;

	if (((fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_ASSIGN_FRAME_TAG) == 0)
		&& (fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_FRAME_TAG)) {
		/* N:1 case */
		if (fs_mgr.frame_cell_size[idx] == 0) {
			LOG_MUST(
				"NOTICE: [%u] call set auto frame_tag, feature_mode:%u, but frame_cell_size:%u not valid, return\n",
				idx,
				fs_mgr.hdr_ft_mode[idx],
				fs_mgr.frame_cell_size[idx]
			);

			return;
		}

		fs_alg_set_frame_tag(idx, fs_mgr.frame_tag[idx]);

		/* update new frame tag */
		f_tag = (fs_mgr.frame_tag[idx] + 1);
		f_cell = (fs_mgr.frame_cell_size[idx]);
		fs_mgr.frame_tag[idx] = f_tag % f_cell;
	}
}


void fs_set_frame_tag(unsigned int ident, unsigned int f_tag)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;
	if (FS_CHECK_BIT(idx, &fs_mgr.validSync_bits) == 0)
		return;


	if ((fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_ASSIGN_FRAME_TAG)
		&& (fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_FRAME_TAG)) {
		/* M-Stream case */
		fs_mgr.frame_tag[idx] = f_tag;
		fs_alg_set_frame_tag(idx, fs_mgr.frame_tag[idx]);
	} else {
		LOG_MUST(
			"WARNING: [%u] call set frame_tag:%u, but feature_mode:%u not allow\n",
			idx,
			f_tag,
			fs_mgr.hdr_ft_mode[idx]
		);
	}
}


void fs_n_1_en(unsigned int ident, unsigned int n, unsigned int en)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

	if (fs_check_n_1_status_ctrl(idx, en) == 0) {
		/* feature mode status is non valid for this ctrl */
		LOG_MUST(
			"NOTICE: [%u] ID:%#x(sidx:%u), set N:%u, but ft ctrl non valid, feature_mode:%u (FRAME_TAG:%u/ON:%u/KEEP:%u/OFF:%u), return  [en:%u]\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			n,
			fs_mgr.hdr_ft_mode[idx],
			fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_FRAME_TAG,
			fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_N_1_ON,
			fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_N_1_KEEP,
			fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_N_1_OFF,
			en);
		return;
	}

	if (en) {
		fs_mgr.frame_cell_size[idx] = n;
		fs_mgr.hdr_ft_mode[idx] =
			(FS_HDR_FT_MODE_FRAME_TAG | FS_HDR_FT_MODE_N_1_ON);
	} else {
		fs_mgr.frame_cell_size[idx] = 0;
		fs_mgr.hdr_ft_mode[idx] =
			(FS_HDR_FT_MODE_FRAME_TAG | FS_HDR_FT_MODE_N_1_OFF);
	}

	fs_alg_set_frame_cell_size(idx, fs_mgr.frame_cell_size[idx]);


	/* reset frame tag cnt */
	fs_reset_frame_tag(idx);


	/* notify frame-sync algo at ON/OFF case for setting correct FL */
	/* this will be turn off when FL be calculated done */
	fs_alg_set_n_1_on_off_flag(idx, 1);


	LOG_MUST(
		"[%u] ID:%#x(sidx:%u), N:%u, feature_mode:%u (FRAME_TAG:%u/ON:%u/OFF:%u)  [en:%u]\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		n,
		fs_mgr.hdr_ft_mode[idx],
		fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_FRAME_TAG,
		fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_N_1_ON,
		fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_N_1_OFF,
		en);
}


void fs_mstream_en(unsigned int ident, unsigned int en)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

	fs_n_1_en(ident, 2, en);

	if (en) {
		/* set M-Stream mode */
		fs_mgr.hdr_ft_mode[idx] |= FS_HDR_FT_MODE_ASSIGN_FRAME_TAG;
	} else {
		/* unset M-Stream mode */
		fs_mgr.hdr_ft_mode[idx] &= ~(FS_HDR_FT_MODE_ASSIGN_FRAME_TAG);
	}

	LOG_MUST(
		"[%u] ID:%#x(sidx:%u), N:2, feature_mode:%u (ASSIGN_FRAME_TAG:%u)  [en:%u]\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_mgr.hdr_ft_mode[idx],
		fs_mgr.hdr_ft_mode[idx] & FS_HDR_FT_MODE_ASSIGN_FRAME_TAG,
		en);
}
#endif // SUPPORT_FS_NEW_METHOD


void fs_set_extend_framelength(
	unsigned int ident, unsigned int ext_fl_lc, unsigned int ext_fl_us)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

	fs_alg_set_extend_framelength(idx, ext_fl_lc, ext_fl_us);
}


void fs_chk_exit_seamless_switch_frame(const unsigned int ident)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

	/* check if new vsync sof cnt been updated and is NOT match to seamless sof cnt */
	/* because it is the next frame that after seamless switch frame */
	/* if yes, clear/reset seamless ctrl data */
	if (fs_chk_seamless_switch_status(idx)) {
		if (fs_mgr.seamless_ctrl[idx].seamless_sof_cnt
				!= fs_mgr.sof_cnt_arr[idx]
			|| (FS_ATOMIC_READ(&fs_mgr.seamless_ctrl[idx].wait_for_processing) == 0)) {
			LOG_MUST(
				"NOTICE: [%u] ID:%#x(sidx:%u), wait_for_processing:%d or (current sof cnt:%u)/(seamelss sof cnt:%u) is different => It is NOT seamless switch frame => enter to NORMAL frame => clear seamless ctrl that keep before\n",
				idx,
				fs_mgr.seamless_ctrl[idx].seamless_info.seamless_pf_ctrl.sensor_id,
				fs_mgr.seamless_ctrl[idx].seamless_info.seamless_pf_ctrl.sensor_idx,
				FS_ATOMIC_READ(&fs_mgr.seamless_ctrl[idx].wait_for_processing),
				fs_mgr.sof_cnt_arr[idx],
				fs_mgr.seamless_ctrl[idx].seamless_sof_cnt);

			fs_clr_seamless_switch_info(idx);
		}
	}
}


void fs_chk_valid_for_doing_seamless_switch(const unsigned int ident)
{
	struct fs_sa_cfg sa_cfg = {0};
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

	/* check if new vsync sof cnt been updated and is match to seamless sof cnt */
	/* if not, keep seamless ctrl for vsync notify using. */
	if (fs_chk_seamless_switch_status(idx)) {
		if (FS_ATOMIC_READ(&fs_mgr.seamless_ctrl[idx].wait_for_processing) == 0) {
			LOG_PF_INF(
				"NOTICE: [%u] ID:%#x(sidx:%u), seamless:%#x, wait_for_processing:%d, through (current sof cnt:%u)/(seamelss sof cnt:%u) is same, SA(m_idx:%d), keep this seamless ctrl timing\n",
				idx,
				fs_mgr.seamless_ctrl[idx].seamless_info.seamless_pf_ctrl.sensor_id,
				fs_mgr.seamless_ctrl[idx].seamless_info.seamless_pf_ctrl.sensor_idx,
				FS_ATOMIC_READ(&fs_mgr.seamless_bits),
				FS_ATOMIC_READ(&fs_mgr.seamless_ctrl[idx].wait_for_processing),
				fs_mgr.sof_cnt_arr[idx],
				fs_mgr.seamless_ctrl[idx].seamless_sof_cnt,
				FS_ATOMIC_READ(&fs_mgr.master_idx));
			return;
		}
		if (fs_mgr.seamless_ctrl[idx].seamless_sof_cnt
				!= fs_mgr.sof_cnt_arr[idx]) {
			LOG_MUST(
				"NOTICE: [%u] ID:%#x(sidx:%u), seamless:%#x, wait_for_processing:%d, (current sof cnt:%u)/(seamelss sof cnt:%u) is different, SA(m_idx:%d), keep this seamless ctrl\n",
				idx,
				fs_mgr.seamless_ctrl[idx].seamless_info.seamless_pf_ctrl.sensor_id,
				fs_mgr.seamless_ctrl[idx].seamless_info.seamless_pf_ctrl.sensor_idx,
				FS_ATOMIC_READ(&fs_mgr.seamless_bits),
				FS_ATOMIC_READ(&fs_mgr.seamless_ctrl[idx].wait_for_processing),
				fs_mgr.sof_cnt_arr[idx],
				fs_mgr.seamless_ctrl[idx].seamless_sof_cnt,
				FS_ATOMIC_READ(&fs_mgr.master_idx));
			return;
		}

		/* !!! can do seamless frame-sync flow !!! */
		fs_sa_setup_perframe_cfg_info(idx, &sa_cfg);

		LOG_MUST(
			"NOTICE: [%u] ID:%#x(sidx:%u), seamless_bits:%#x, wait_for_processing:%d, (current sof cnt:%u)/(seamelss sof cnt:%u) is same, SA(idx:%u/m_idx:%d/async_m_idx:%u/async_s:%#x/valid_sync:%#x/method:%u) => do seamless switch process\n",
			idx,
			fs_mgr.seamless_ctrl[idx].seamless_info.seamless_pf_ctrl.sensor_id,
			fs_mgr.seamless_ctrl[idx].seamless_info.seamless_pf_ctrl.sensor_idx,
			FS_ATOMIC_READ(&fs_mgr.seamless_bits),
			FS_ATOMIC_READ(&fs_mgr.seamless_ctrl[idx].wait_for_processing),
			fs_mgr.sof_cnt_arr[idx],
			fs_mgr.seamless_ctrl[idx].seamless_sof_cnt,
			sa_cfg.idx,
			sa_cfg.m_idx,
			sa_cfg.async_m_idx,
			sa_cfg.async_s_bits,
			sa_cfg.valid_sync_bits,
			sa_cfg.sa_method);

		FS_ATOMIC_SET(0, &fs_mgr.seamless_ctrl[idx].wait_for_processing);
		fs_do_seamless_switch_proc(idx, &sa_cfg);
	}
}


void fs_seamless_switch(const unsigned int ident,
	struct fs_seamless_st *p_seamless_info,
	const unsigned int seamless_sof_cnt)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

	fs_set_seamless_switch_info(idx, p_seamless_info, seamless_sof_cnt);

	fs_chk_valid_for_doing_seamless_switch(ident);
}


/*
 * update fs_streaming_st data
 *     (for cam_mux switch & sensor stream on before cam mux setup)
 */
void fs_update_tg(const unsigned int ident, unsigned int tg)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

#ifdef SUPPORT_USING_CCU
	if (frm_get_ts_src_type() == FS_TS_SRC_CCU) {
		/* check ccu pwr ON, and disable vsync INT(previous tg) */
		if (FS_CHECK_BIT(idx, &fs_mgr.power_on_ccu_bits))
			frm_reset_ccu_vsync_timestamp(idx, 0);
	}
#endif

	/* convert cammux id to ccu tg id */
	tg = frm_convert_cammux_id_to_ccu_tg_id(tg);

	/* update the fs_streaming_st data */
	fs_alg_update_tg(idx, tg);
	frm_update_tg(idx, tg);

#ifdef SUPPORT_USING_CCU
	if (frm_get_ts_src_type() == FS_TS_SRC_CCU) {
		/* on the fly change cam_mux/tg */
		/* ==> reset/re-init frame info data for after using */
		if (FS_CHECK_BIT(idx, &fs_mgr.power_on_ccu_bits))
			frm_reset_ccu_vsync_timestamp(idx, 1);
	}
#endif
}


/*
 * update fs_streaming_st data
 *     (for cam_mux switch & sensor stream on before cam mux setup)
 *     ISP7s HW change, seninf assign target tg ID (direct map to CCU tg ID)
 */
void fs_update_target_tg(const unsigned int ident,
	const unsigned int target_tg)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

#ifdef SUPPORT_USING_CCU
	if (frm_get_ts_src_type() == FS_TS_SRC_CCU) {
		/* check ccu pwr ON, and disable INT(previous tg) */
		if (FS_CHECK_BIT(idx, &fs_mgr.power_on_ccu_bits))
			frm_reset_ccu_vsync_timestamp(idx, 0);
	}
#endif

	/* update the fs_streaming_st data */
	fs_alg_update_tg(idx, target_tg);
	frm_update_tg(idx, target_tg);

#ifdef SUPPORT_USING_CCU
	if (frm_get_ts_src_type() == FS_TS_SRC_CCU) {
		/* on the fly change cam_mux/tg */
		/* ==> reset/re-init frame info data for after using */
		if (FS_CHECK_BIT(idx, &fs_mgr.power_on_ccu_bits))
			frm_reset_ccu_vsync_timestamp(idx, 1);
	}
#endif
}


static void fs_reset_idx_ctx(const unsigned int idx)
{
	/* unset sync */
	fs_set_sync_idx(idx, 0);

	/* reset frm frame info data */
	frm_reset_frame_info(idx);

	/* reset fs instance data (algo -> fs_inst[idx]) */
	fs_alg_reset_fs_inst(idx);

	/* reset frame recorder data */
	frec_reset_records(idx);

	/* clear/reset perframe stage data */
	fs_reset_perframe_stage_data(idx, 0);

	/* unset stream (stream off) */
	fs_set_stream(idx, 0);

	/* clear/reset callback function pointer */
	fs_reset_cb_info(idx);
}


static void fs_streaming_chk_restore_preset_set_sync_status(
	const unsigned int idx, const struct fs_streaming_st *sensor_info)
{
	int ret;

#if !defined(FS_UT)
	unsigned int owr_flag = 1;

	/* check streaming with enable set sync if user set it */
	if (unlikely(fs_con_chk_default_en_set_sync())) {
		/* user cmd overwrite set sync flag */
		if (fs_con_chk_en_overwrite_set_sync(
				sensor_info->sensor_idx, &owr_flag)) {
			LOG_MUST(
				"NOTICE: ID:%#x(sidx:%u), USER set overwrite set sync value:(1 -> %u)\n",
				sensor_info->sensor_id,
				sensor_info->sensor_idx,
				owr_flag);
		}

		fs_update_set_sync_idx_table(idx, owr_flag);

		LOG_MUST(
			"NOTICE: ID:%#x(sidx:%u), USER set frame-sync force enable set sync, set_sync_idx_table[%u]:%d\n",
			sensor_info->sensor_id,
			sensor_info->sensor_idx,
			sensor_info->sensor_idx,
			FS_ATOMIC_READ(&fs_mgr.set_sync_idx_table[
				sensor_info->sensor_idx]));
	}
#endif

	/* check if set sync before streaming on */
	ret = fs_check_set_sync_idx_table(idx);
	if (ret > 0) {
		LOG_MUST(
			"NOTICE: [%u] ID:%#x(sidx:%u), apply set sync procedure, due to set_sync_idx_table[%u]:%d\n",
			idx,
			sensor_info->sensor_id,
			sensor_info->sensor_idx,
			sensor_info->sensor_idx,
			ret);

		fs_set_sync_idx(idx, ret);
	}
}


static void fs_streaming_setup_fs_streaming_st_data(const unsigned int idx,
	struct fs_streaming_st *sensor_info)
{
	struct fs_perframe_st *p_preset_pf_ctrl = NULL;
	int ret;

	ret = fs_get_preset_perframe_data(idx, &p_preset_pf_ctrl);
	if (ret)
		fs_alg_set_preset_perframe_streaming_st_data(idx,
			sensor_info, p_preset_pf_ctrl);
	else
		fs_alg_set_streaming_st_data(idx, sensor_info);

	hw_fs_alg_set_streaming_st_data(idx, sensor_info);
}


unsigned int fs_streaming(const unsigned int flag,
	struct fs_streaming_st *sensor_info)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx(
			sensor_info->sensor_id, sensor_info->sensor_idx,
			&idx, __func__)))
		return 1;

	/* reset this idx item and reset CCU vsync timestamp */
	fs_reset_idx_ctx(idx);

	/* set/clear hw sensor sync info */
	fs_set_hw_sync_info(idx, flag,
		sensor_info->sync_mode, sensor_info->hw_sync_group_id);

	/* if streaming ON, set information of this idx correctlly */
	if (flag > 0) {
		struct FrameRecord frame_rec;

		/* set/init callback data */
		fs_init_cb_info(idx, sensor_info->p_ctx, sensor_info->func_ptr);

		/* init/setup frame monitor info */
		/* --- get tg value by check cammux_id and target_tg value */
		/* --- overwrite sensor info's tg value */
		sensor_info->tg = frm_chk_and_get_tg_value(
			sensor_info->cammux_id, sensor_info->target_tg);
		frm_init_frame_info_st_data(idx,
			sensor_info->sensor_id, sensor_info->sensor_idx,
			sensor_info->tg);

		/* setup streaming info */
		fs_streaming_setup_fs_streaming_st_data(
			idx, sensor_info);

		/* init/setup sensor recorder */
		frec_setup_frame_rec_by_fs_streaming_st(
			&frame_rec, sensor_info);
		frec_init_recorder(idx, &frame_rec,
			sensor_info->def_fl_lc, sensor_info->fl_active_delay);

		/* init/setup fs algo SA dynamic infos */
		fs_alg_sa_update_dynamic_infos(idx, 0);

		fs_set_stream(idx, 1);
		fs_streaming_chk_restore_preset_set_sync_status(
			idx, sensor_info);
	} else {
		fs_reset_fl_restore_status(idx);
		fs_reset_ft_mode_data(idx, flag);

		/* reset/clear set sync sidx table value */
		fs_update_set_sync_idx_table(idx, 0);
		fs_reset_preset_perframe_data(idx);
	}

	LOG_INF(
		"[%u] ID:%#x(sidx:%u/inf:%u), flag:%u(ON:1/OFF:0)   [streaming_bits:%#x]\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx),
		flag,
		FS_READ_BITS(&fs_mgr.streaming_bits));

	return 0;
}


static void fs_notify_sensor_ctrl_setup_complete(unsigned int idx)
{
	unsigned int hw_sync_group_id = FS_HW_SYNC_GROUP_ID_MIN;

	if (FS_CHECK_BIT(idx, &fs_mgr.validSync_bits) == 0) {
		/* no start frame sync, return */
		return;
	}

	/* set setup complete */
	FS_WRITE_BIT(idx, 1, &fs_mgr.setup_complete_bits);

	/* checking for hw sync method */
	if (FS_CHECK_BIT(idx, &fs_mgr.hw_sync_bits)) {
		hw_sync_group_id = fs_mgr.hw_sync_group_id[idx];

		if (hw_sync_group_id < FS_HW_SYNC_GROUP_ID_MAX) {
			/* hw group id is a valid value */
			FS_WRITE_BIT(idx, 1,
				&fs_mgr.setup_complete_hw_group_bits[
					hw_sync_group_id]);
		}
	}


#if !defined(REDUCE_FS_DRV_LOG)
	LOG_INF(
		"[%u] ID:%#x(sidx:%u), hw_sync(bits:%d, group_id:%u)  [setup_complete(%d, hw_group(%d/%d/%d/%d/%d/%d))]\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		FS_READ_BITS(&fs_mgr.hw_sync_bits),
		fs_mgr.hw_sync_group_id[idx],
		FS_READ_BITS(&fs_mgr.setup_complete_bits),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_0]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_1]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_2]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_3]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_4]),
		FS_READ_BITS(
			&fs_mgr.setup_complete_hw_group_bits[
				FS_HW_SYNC_GROUP_ID_5]));
#endif


#ifdef SUPPORT_FS_NEW_METHOD
	/* setup compeleted => tirgger FS standalone method */
	if (FS_ATOMIC_READ(&fs_mgr.using_sa_ver)
		&& !FS_CHECK_BIT(idx, &fs_mgr.hw_sync_bits))
		fs_try_trigger_frame_sync_sa(idx);
#endif // SUPPORT_FS_NEW_METHOD
}


/*
 * update fs_perframe_st data
 *     (for v4l2_ctrl_request_setup order)
 *     (set_framelength is called after set_anti_flicker)
 */
void fs_update_auto_flicker_mode(unsigned int ident, unsigned int en)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

	/* update the fs_perframe_st data in fs algo */
	fs_alg_set_anti_flicker(idx, en);
}


/*
 * update fs_perframe_st data
 *     (for v4l2_ctrl_request_setup order)
 *     (set_max_framerate_by_scenario is called after set_shutter)
 */
void fs_update_min_fl_lc(const unsigned int ident, const unsigned int min_fl_lc,
	const unsigned int fl_lc,
	const unsigned int fl_lc_arr[], const unsigned int arr_len)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;
	if (FS_CHECK_BIT(idx, &fs_mgr.streaming_bits) == 0)
		return;

	/* update the fs_perframe_st data in fs algo */
	fs_alg_update_min_fl_lc(idx, min_fl_lc);
	hw_fs_alg_update_min_fl_lc(idx, min_fl_lc);

	/* update sensor recorder fl settings */
	frec_update_fl_info(idx, fl_lc, fl_lc_arr, arr_len);


#ifndef USING_V4L2_CTRL_REQUEST_SETUP
	/* if this is the last ctrl needed by FrameSync, notify setup complete */
	fs_notify_sensor_ctrl_setup_complete(idx);
#endif // USING_V4L2_CTRL_REQUEST_SETUP
}


/*
 * Run Frame Sync Method (for pair ctrl trigger)
 *
 * ex:
 *     1. hardware sensor frame sync
 *        => call hw fs algo API to solve framelength
 *
 *     2. software frame sync
 *        => call fs algo API to solve framelength
 */
static inline unsigned int fs_run_frame_sync_proc(
	unsigned int solveIdxs[], unsigned int framelength[], unsigned int len)
{
	unsigned int ret = 0;

	if (handle_by_hw_sensor_sync(solveIdxs, len))
		ret = hw_fs_alg_solve_frame_length(solveIdxs, framelength, len);
	else
		ret = fs_alg_solve_frame_length(solveIdxs, framelength, len);

	return ret;
}


unsigned int fs_try_trigger_frame_sync(void)
{
	unsigned int i = 0, ret = 0;
	unsigned int fs_act = 0;
	unsigned int len = 0; /* how many sensors wait for doing frame sync */
	unsigned int solveIdxs[SENSOR_MAX_NUM] = {0};
	unsigned int fl_lc[SENSOR_MAX_NUM] = {0};


	/* if validSync_bits == pf_ctrl_bits => "DO" FrameSync */
	fs_act = (
		FS_READ_BITS(&fs_mgr.validSync_bits) ==
		FS_READ_BITS(&fs_mgr.pf_ctrl_bits))
		? 1 : 0;

	/* add for checking all ctrl needed by FrameSync have been setup */
	if (fs_act == 1) {
		fs_act = (
			FS_READ_BITS(&fs_mgr.pf_ctrl_bits) ==
			FS_READ_BITS(&fs_mgr.setup_complete_bits))
			? 1 : 0;

		if (fs_act == 0) {
			LOG_MUST(
				"WARNING: sensor ctrl has not been setup yet, validSync:%2d, pf_ctrl:%2d, setup_complete:%2d\n",
				FS_READ_BITS(&fs_mgr.validSync_bits),
				FS_READ_BITS(&fs_mgr.pf_ctrl_bits),
				FS_READ_BITS(&fs_mgr.setup_complete_bits));
		}
	}


	if (fs_act == 1) {
		// LOG_INF("fs_activate:%u, validSync:%2d, pf_ctrl:%2d, cnt:%u\n",
		LOG_INF("fs_activate:%u, validSync:%2d, pf_ctrl:%2d\n",
			fs_act,
			FS_READ_BITS(&fs_mgr.validSync_bits),
			FS_READ_BITS(&fs_mgr.pf_ctrl_bits));
			// ++fs_mgr.act_cnt);


		/* 1 pick up validSync sensor information correctlly */
		for (i = 0; i < SENSOR_MAX_NUM; ++i) {
			if (FS_CHECK_BIT(i, &fs_mgr.validSync_bits) == 1) {
				/* pick up sensor registered location */
				solveIdxs[len++] = i;
			}
		}


		/* 2 run frame sync proc to solve frame length */
		/*   if no error */
		/*       update framelength to frame recorder and then */
		/*       use callback function to set framelength to sensor */
		ret = fs_run_frame_sync_proc(solveIdxs, fl_lc, len);
		if (ret == 0) { // 0 => No error
			for (i = 0; i < len; ++i) {
				unsigned int idx = solveIdxs[i];

				/* set framelength */
				/* all framelength operation must use this API */
				fs_set_framelength_lc(idx, fl_lc[idx]);
			}
		}

	} else {
		LOG_MUST(
			"wait for other pf_ctrl setup, validSync:%2d, pf_ctrl:%2d, setup_complete:%2d\n",
			FS_READ_BITS(&fs_mgr.validSync_bits),
			FS_READ_BITS(&fs_mgr.pf_ctrl_bits),
			FS_READ_BITS(&fs_mgr.setup_complete_bits));
	}


	return fs_act;
}


/*
 * For trigger HW sync Frame-Sync solution
 *
 * return:
 *     pf ctrl that be triggered successfully by Frame-Sync
 */
int fs_try_trigger_hw_frame_sync(void)
{
	int pf_ctrl_bits = 0, trigger_ctrl_bits = 0;
	int setup_complete_hw_group_bits = 0;
	unsigned int i = 0, j = 0, ret = 1;
	unsigned int len = 0; /* how many sensors wait for doing frame sync */
	unsigned int solveIdxs[SENSOR_MAX_NUM] = {0};
	unsigned int fl_lc[SENSOR_MAX_NUM] = {0};


	/* trigger Frame-Sync proc by group (find out 1 group) */
	for (i = FS_HW_SYNC_GROUP_ID_MIN; i < FS_HW_SYNC_GROUP_ID_MAX; ++i) {
		if (FS_READ_BITS(&fs_mgr.hw_sync_group_bits[i]) == 0)
			continue;

		do {
			ret = 1;

			/* reset/clear tmp data for current run */
			len = 0;
			for (j = 0; j < SENSOR_MAX_NUM; ++j) {
				solveIdxs[j] = 0;
				fl_lc[j] = 0;
			}

			setup_complete_hw_group_bits =
				FS_READ_BITS(
					&fs_mgr.setup_complete_hw_group_bits[i]);
			pf_ctrl_bits =
				FS_READ_BITS(&fs_mgr.pf_ctrl_bits) &
				FS_READ_BITS(&fs_mgr.validSync_bits);

			/* do NOT expect */
			if (pf_ctrl_bits == 0) {
				LOG_MUST(
					"WARNING: try trigger, but validSync:%d, pf_ctrl:%d, setup_complete:%d(%d/%d/%d/%d/%d/%d), abort\n",
					FS_READ_BITS(&fs_mgr.validSync_bits),
					FS_READ_BITS(&fs_mgr.pf_ctrl_bits),
					FS_READ_BITS(
						&fs_mgr.setup_complete_bits),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_0]),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_1]),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_2]),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_3]),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_4]),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_5]));

				ret = 1;
				break;
			}

			/* check group match for triggering pf ctrl */
			trigger_ctrl_bits =
				(pf_ctrl_bits
				& FS_READ_BITS(&fs_mgr.hw_sync_group_bits[i]));

			if (trigger_ctrl_bits ==
				(FS_READ_BITS(&fs_mgr.hw_sync_group_bits[i])
				& FS_READ_BITS(&fs_mgr.validSync_bits))) {

				LOG_PF_INF(
					"Trigger group id:%u, cnt:%u, valid_sync:%d, trigger_ctrl:%u/%u, pf_ctrl:%d, setup_complete:%d(%d/%d/%d/%d/%d/%d)\n",
					i,
					fs_mgr.act_cnt[i] + 1,
					FS_READ_BITS(&fs_mgr.validSync_bits),
					trigger_ctrl_bits,
					setup_complete_hw_group_bits,
					FS_READ_BITS(&fs_mgr.pf_ctrl_bits),
					FS_READ_BITS(
						&fs_mgr.setup_complete_bits),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_0]),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_1]),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_2]),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_3]),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_4]),
					FS_READ_BITS(
						&fs_mgr.setup_complete_hw_group_bits[
							FS_HW_SYNC_GROUP_ID_5]));

				/* pick up sensor information */
				for (j = 0; j < SENSOR_MAX_NUM; ++j) {
					if ((FS_CHECK_BIT(j,
							&fs_mgr.hw_sync_group_bits[i])
						& FS_CHECK_BIT(j,
							&fs_mgr.validSync_bits))
						== 1) {
						/* pick up sensor registered location */
						solveIdxs[len++] = j;
					}
				}

				/* run frame sync proc (hw) to solve frame length */
				ret = hw_fs_alg_solve_frame_length(
					solveIdxs, fl_lc, len);
			}
#ifdef FS_UT
		} while (
			!atomic_compare_exchange_weak(
				&fs_mgr.setup_complete_hw_group_bits[i],
				&setup_complete_hw_group_bits, 0));
#else
		} while (
			atomic_cmpxchg(
				&fs_mgr.setup_complete_hw_group_bits[i],
				setup_complete_hw_group_bits, 0)
			!= setup_complete_hw_group_bits);
#endif // FS_UT


		/* if this group proc. has some error occurred, */
		/* move on to next group for preventing */
		/* each time entering this function jump out at this group */
		if (ret == 0) { // 0 => no error
			/* increase frame-sync act cnt */
			fs_mgr.act_cnt[i]++;
			break;
		}
	}


	/* clear pf ctrl & setup complete bits */
	/* that those ctrls are been triggered */
	/* and update framelength to frame recorder and then */
	/* use callback function to set framelength to sensor */
	if (ret == 0) { // 0 => No error
		FS_ATOMIC_FETCH_AND(
			(~(trigger_ctrl_bits)), &fs_mgr.pf_ctrl_bits);
		FS_ATOMIC_FETCH_AND(
			(~(trigger_ctrl_bits)), &fs_mgr.setup_complete_bits);


		for (j = 0; j < len; ++j) {
			unsigned int idx = solveIdxs[j];

			/* set framelength */
			fs_set_framelength_lc(idx, fl_lc[j]);
		}
	}


	return (ret == 0) ? trigger_ctrl_bits : 0;
}


static void fs_check_frame_sync_ctrl(
	unsigned int idx, struct fs_perframe_st (*pf_ctrl))
{
	/* before doing frame sync, check some situation */
	unsigned int ret = 0;


	/* 1. check this sensor is valid for sync or not */
	ret = FS_CHECK_BIT(idx, &fs_mgr.validSync_bits);

	if (ret == 0) {

#ifndef REDUCE_FS_DRV_LOG
		LOG_PR_WARN(
			"WARNING: Not valid for doing sync. [%u] ID:%#x\n",
			idx,
			pf_ctrl->sensor_id);
#endif // REDUCE_FS_DRV_LOG

		return;
	}


	/* 2. check this perframe ctrl is valid or not */
	ret = FS_CHECK_BIT(idx, &fs_mgr.pf_ctrl_bits);

	if (ret == 1) {
		LOG_PR_WARN(
			"WARNING: Set same sensor, return. [%u] ID:%#x  [pf_ctrl_bits:%d]\n",
			idx,
			pf_ctrl->sensor_id,
			FS_READ_BITS(&fs_mgr.pf_ctrl_bits));

		return;

	} else {
		FS_WRITE_BIT(idx, 1, &fs_mgr.pf_ctrl_bits);

		fs_set_cb_cmd_id(idx, pf_ctrl->cmd_id);
	}


	/* trigger framesync at ::fs_sync_frame(0) */
	/* 3. check for running frame sync processing or not */
	// fs_try_trigger_frame_sync();
}


/*
 * description:
 *     call by sensor driver,
 *     fs_perframe_st is a settings for a certain sensor.
 *
 * input:
 *     pf_ctrl: struct fs_perframe_st*
 */
void fs_set_shutter(struct fs_perframe_st (*pf_ctrl))
{
	struct FrameRecord frame_rec;
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx(pf_ctrl->sensor_id, pf_ctrl->sensor_idx,
			&idx, __func__)))
		return;
	/* check case */
	if (FS_CHECK_BIT(idx, &fs_mgr.validSync_bits) == 0)
		return;
	if (unlikely(fs_chk_seamless_switch_status(idx))) {
		LOG_MUST(
			"NOTICE: [%u] ID:%#x(sidx:%u), (%d/%u), in seamless frame, seamless(%#x, sof_cnt:%u), skip/return\n",
			idx,
			pf_ctrl->sensor_id,
			pf_ctrl->sensor_idx,
			pf_ctrl->req_id,
			fs_mgr.sof_cnt_arr[idx],
			FS_ATOMIC_READ(&fs_mgr.seamless_bits),
			fs_mgr.seamless_ctrl[idx].seamless_sof_cnt);
		fs_reject_frame_length_ctrl(idx);
		return;
	}

	fs_reset_fl_restore_status(idx);

	fs_mgr.pf_ctrl[idx] = *pf_ctrl;


#if !defined(FORCE_USING_SA_MODE)
#if defined(SUPPORT_AUTO_EN_SA_MODE) && !defined(FS_UT)
	/* check needed SA */
	if (pf_ctrl->hdr_exp.mode_exp_cnt > 0)
		fs_mgr.hdr_ft_mode[idx] |= FS_HDR_FT_MODE_STG_HDR;
	else
		fs_mgr.hdr_ft_mode[idx] &= ~FS_HDR_FT_MODE_STG_HDR;


	fs_decision_maker(idx);
#endif
#endif // FORCE_USING_SA_MODE


	/* for FPS non 1-1 frame-sync case */
	fs_try_set_auto_frame_tag(idx);


	/* 1. set perframe ctrl data to fs algo */
	fs_alg_set_perframe_st_data(idx, pf_ctrl);
	hw_fs_alg_set_perframe_st_data(idx, pf_ctrl);


	/* 3. update frame recorder data */
	frec_setup_frame_rec_by_fs_perframe_st(&frame_rec, pf_ctrl);
	frec_update_record(idx, &frame_rec);


	/* 4. frame sync ctrl */
	fs_check_frame_sync_ctrl(idx, pf_ctrl);


#ifdef USING_V4L2_CTRL_REQUEST_SETUP
	/* if this is the last ctrl needed by FrameSync, notify setup complete */
	fs_notify_sensor_ctrl_setup_complete(idx);
#endif // USING_V4L2_CTRL_REQUEST_SETUP
}


void fs_update_shutter(struct fs_perframe_st (*pf_ctrl))
{
	struct FrameRecord frame_rec;
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx(pf_ctrl->sensor_id, pf_ctrl->sensor_idx,
			&idx, __func__)))
		return;

	fs_try_setup_preset_perframe_data(idx, pf_ctrl);

	/* check case */
	if (unlikely(FS_CHECK_BIT(idx, &fs_mgr.streaming_bits) == 0))
		return;
	if (unlikely(fs_chk_seamless_switch_status(idx))) {
		LOG_MUST(
			"NOTICE: [%u] ID:%#x(sidx:%u), (%d/%u), in seamless frame, seamless(%#x, sof_cnt:%u), skip/return\n",
			idx,
			pf_ctrl->sensor_id,
			pf_ctrl->sensor_idx,
			pf_ctrl->req_id,
			fs_mgr.sof_cnt_arr[idx],
			FS_ATOMIC_READ(&fs_mgr.seamless_bits),
			fs_mgr.seamless_ctrl[idx].seamless_sof_cnt);
		return;
	}
	if (FS_CHECK_BIT(idx, &fs_mgr.validSync_bits) == 1) {
		/* check fl info drv set match Frame-Sync gave */
		fs_chk_fl_info_updated_from_drv(idx, pf_ctrl, __func__);
	}

	/* update/overwrite fl info drv set */
	fs_mgr.pf_ctrl[idx] = *pf_ctrl;

	/* keep tracking per-frame settings even when NOT enable sync */
	// if (FS_CHECK_BIT(idx, &fs_mgr.validSync_bits) == 0) {
	//	fs_alg_set_perframe_st_data(idx, pf_ctrl);
	//	fs_alg_sa_update_dynamic_infos(idx, 1);
	// }

	/* update frame recorder data */
	frec_setup_frame_rec_by_fs_perframe_st(&frame_rec, pf_ctrl);
	frec_update_record(idx, &frame_rec);
}


void fs_update_frame_length(const unsigned int ident,
	const unsigned int fl_lc,
	const unsigned int fl_lc_arr[], const unsigned int arr_len)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;
	if (FS_CHECK_BIT(idx, &fs_mgr.streaming_bits) == 0)
		return;

	frec_update_fl_info(idx, fl_lc, fl_lc_arr, arr_len);
}


void fs_get_fl_record_info(const unsigned int ident,
	unsigned int *p_target_min_fl_us, unsigned int *p_out_fl_us)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

	fs_alg_get_fl_rec_st_info(idx, p_target_min_fl_us, p_out_fl_us);
}


void fs_clear_fl_restore_status_if_needed(const unsigned int ident)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;
	if (chk_xchg_bit_atomic(idx, 0, &fs_mgr.fl_restore_ctrl_bits) == 0)
		return;

	fs_clear_fl_restore_info(idx);

	LOG_MUST(
		"NOTICE: [%u] ID:%#x(sidx:%u), clear FL restore info, fl_restore_ctrl_bits:%#x\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		FS_ATOMIC_READ(&fs_mgr.fl_restore_ctrl_bits));
}


static void fs_do_fl_restore_proc_if_needed(const unsigned int idx)
{
	struct fs_fl_restore_info_st fl_restore_info = {0};
	const unsigned int cmd_id = FSYNC_CTRL_FL_CMD_ID_FL;
	unsigned long long curr_sys_ts = 0;

	if (chk_xchg_bit_atomic(idx, 0, &fs_mgr.fl_restore_ctrl_bits) == 0)
		return;

	fs_get_fl_restore_info(idx, &fl_restore_info);

#ifndef FS_UT
	curr_sys_ts = ktime_get_boottime_ns();
#endif

	LOG_MUST(
		"NOTICE: [%u] ID:%#x(sidx:%u), do FL restore, cmd_id:%u(NONE:%u/shutter_with_FL:%u/FL:%u), (%#x, #%u, req_id:%d, %u, %u/%u/%u/%u/%u), curr_sys_ts:%llu\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		cmd_id,
		FSYNC_CTRL_FL_CMD_ID_NONE,
		FSYNC_CTRL_FL_CMD_ID_EXP_WITH_FL,
		FSYNC_CTRL_FL_CMD_ID_FL,
		FS_ATOMIC_READ(&fs_mgr.fl_restore_ctrl_bits),
		fl_restore_info.magic_num,
		fl_restore_info.req_id,
		fl_restore_info.restored_fl_lc,
		fl_restore_info.restored_fl_lc_arr[0],
		fl_restore_info.restored_fl_lc_arr[1],
		fl_restore_info.restored_fl_lc_arr[2],
		fl_restore_info.restored_fl_lc_arr[3],
		fl_restore_info.restored_fl_lc_arr[4],
		curr_sys_ts);

	/* call back to set FL */
	fs_cb_fsync_ctrl_to_set_fl_info(idx, cmd_id,
		fl_restore_info.restored_fl_lc,
		fl_restore_info.restored_fl_lc_arr, FS_HDR_MAX);

	fs_clear_fl_restore_info(idx);
}


static void fs_debug_hw_sync(unsigned int idx)
{
#if !defined(FS_UT)
	unsigned int arr[1] = {idx};

	fs_alg_setup_frame_monitor_fmeas_data(idx);
	frec_notify_vsync(idx);
	fs_alg_get_vsync_data(arr, 1);
	hw_fs_dump_dynamic_para(idx);
#endif
}


static void fs_update_notify_vsync_sof_cnt(const unsigned int idx)
{
	fs_mgr.notify_vsync_sof_cnt_arr[idx] = fs_mgr.sof_cnt_arr[idx];
}


void fs_set_debug_info_sof_cnt(const unsigned int ident,
	const unsigned int sof_cnt)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;

	/* update debug info --- p1 sof cnt immediately */
	fs_mgr.sof_cnt_arr[idx] = sof_cnt;
	fs_alg_set_debug_info_sof_cnt(idx, sof_cnt);

#ifndef REDUCE_FS_DRV_LOG
	LOG_MUST("NOTICE: [%u] ID:%#x(sidx:%u/inf:%u), get sof_cnt:%u\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx),
		sof_cnt);
#endif

	/* check special ctrl that using p1 sof cnt (e.g., seamless switch) */
	fs_chk_exit_seamless_switch_frame(ident);
}


void fs_notify_vsync(const unsigned int ident)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;
	if (FS_CHECK_BIT(idx, &fs_mgr.validSync_bits) == 0)
		return;
	if (FS_CHECK_BIT(idx, &fs_mgr.hw_sync_bits)) {
		fs_debug_hw_sync(idx);
		return;
	}
#if defined(SUPPORT_USING_CCU) || defined(FS_UT)
	if (frm_get_ts_src_type() != FS_TS_SRC_CCU)
		return;


	/* !!! start here !!! */
	fs_alg_sa_notify_setup_all_frame_info(idx);
	frec_notify_vsync(idx);
	fs_alg_sa_notify_vsync(idx);
	frec_chk_fl_pr_match_act(idx);

	if (unlikely(_FS_LOG_ENABLED(LOG_FS_PF)))
		fs_alg_sa_dump_dynamic_para(idx);

	fs_do_fl_restore_proc_if_needed(idx);

	/* update ctrl's p1 sof cnt for checking ctrl timing */
	fs_update_notify_vsync_sof_cnt(idx);

	/* check special ctrl that using p1 sof cnt (e.g., seamless switch) */
	fs_chk_valid_for_doing_seamless_switch(ident);
#endif
}


void fs_notify_vsync_by_tsrec(const unsigned int ident)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;
	if (FS_CHECK_BIT(idx, &fs_mgr.streaming_bits) == 0)
		return;
	if (frm_get_ts_src_type() != FS_TS_SRC_TSREC)
		return;


	/* !!! start here !!! */
	/* ==> warning / becareful */
	/* ==> below flow make sure TSREC will guarantee the order */
	/* ==> of ctrl (HW pre-latch by tsrec >> notify vsync by tsrec) */
	/* update ctrl's p1 sof cnt for checking ctrl timing */
	fs_update_notify_vsync_sof_cnt(idx);

	/* check special ctrl that using p1 sof cnt (e.g., seamless switch) */
	fs_chk_valid_for_doing_seamless_switch(ident);
}


void fs_notify_sensor_hw_pre_latch_by_tsrec(const unsigned int ident)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;
	if (FS_CHECK_BIT(idx, &fs_mgr.streaming_bits) == 0)
		return;
	if (frm_get_ts_src_type() != FS_TS_SRC_TSREC)
		return;


	/* !!! start here !!! */
	/* below function using sensor recorder for replacement */
	// fs_alg_sa_notify_setup_all_frame_info(idx);
	frec_notify_vsync(idx); // sensor recorder hw pre-latch

	fs_do_fl_restore_proc_if_needed(idx);
}


void fs_receive_tsrec_timestamp_info(const unsigned int ident,
	const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info)
{
	unsigned int idx;

	/* get registered idx and check if it is valid */
	if (unlikely(fs_g_registered_idx_by_ident(ident, &idx, __func__)))
		return;
	if (FS_CHECK_BIT(idx, &fs_mgr.streaming_bits) == 0)
		return;
	if (unlikely(ts_info == NULL)) {
		LOG_MUST(
			"ERROR: get non-valid input, ts_info:%p, return\n",
			ts_info);
		return;
	}
	/* save tsrec timestamp info even if not using tsrec ts info */
	frm_receive_tsrec_timestamp_info(idx, ts_info);
	if (frm_get_ts_src_type() != FS_TS_SRC_TSREC)
		return;


	/* !!! start here !!! */
	/* call this function after receive TSREC timestamp info */
	fs_alg_sa_notify_vsync(idx);
	frec_chk_fl_pr_match_act(idx);

	if (unlikely(_FS_LOG_ENABLED(LOG_FS_PF)))
		if (FS_CHECK_BIT(idx, &fs_mgr.enSync_bits))
			fs_alg_sa_dump_dynamic_para(idx);
}


unsigned int fs_is_ts_src_type_tsrec(void)
{
	return (frm_get_ts_src_type() == FS_TS_SRC_TSREC);
}


/*
 * return:
 *     0 for error,
 *     non 0 for (Start -> cnt of validSync_bits;
 *                  End -> cnt of pf_ctrl_bits)
 *
 * input:
 *     flag: "non 0" -> Start sync frame; "0" -> End sync frame;
 *
 * header file:
 *     frame_sync_camsys.h
 *
 * descriptions:
 *     By default, SW Frame-Sync Algo is using SA algo,
 *     so this function now is only for HW Frame-Sync Algo using.
 */
unsigned int fs_sync_frame(unsigned int flag)
{
	enum FS_STATUS status = get_fs_status();
	int valid_sync = FS_ATOMIC_READ(&fs_mgr.validSync_bits);


#if !defined(FS_UT)
	/* user cmd force disable frame-sync set_sync */
	if (fs_con_chk_force_to_ignore_set_sync())
		return 0;
#endif

#ifdef SUPPORT_FS_NEW_METHOD
	if (FS_READ_BITS(&fs_mgr.hw_sync_bits) == 0
		&& (FS_ATOMIC_READ(&fs_mgr.using_sa_ver) != 0
			|| fs_user_sa_config())) {
		/* Only using SW soltuion and using SA algo for Frame-Sync */
		return 0;
	}
#endif


	/* check sync frame start/end */
	/*     flag > 0 : cam-sys call - start sync frame */
	/*     flag = 0 : cam-sys call - end sync frame */
	if (flag > 0) {
		/* check status is ready for starting sync frame or not */
		if (status < FS_WAIT_FOR_SYNCFRAME_START) {
			fs_dump_status(-1, flag, __func__, "Start:1 Notice");
			return 0;
		}

		if (unlikely(_FS_LOG_ENABLED(LOG_FS_PF)))
			fs_dump_status(-1, flag, __func__, "Start:1");

		/* return the number of valid sync sensors */
		return FS_POPCOUNT(valid_sync);

	} else {
		/* fs_sync_frame(0) flow */
		fs_mgr.last_pf_ctrl_bits =
			FS_READ_BITS(&fs_mgr.pf_ctrl_bits);
		fs_mgr.last_setup_complete_bits =
			FS_READ_BITS(&fs_mgr.setup_complete_bits);

		/* try to trigger frame sync processing */
		fs_mgr.trigger_ctrl_bits =
			// fs_try_trigger_frame_sync();
			fs_try_trigger_hw_frame_sync();

		if (likely(fs_mgr.trigger_ctrl_bits)) {
			/* framesync trigger DONE */
			if (unlikely(_FS_LOG_ENABLED(LOG_FS_PF)))
				fs_dump_status(-1, flag, __func__, "Start:0 DONE");
		} else {
			/* framesync trigger FAIL */
			fs_dump_status(-1, flag, __func__, "Start:0 WARNING: FAIL");
		}

		/* return the number of perframe ctrl sensors */
		return FS_POPCOUNT(fs_mgr.trigger_ctrl_bits);
	}
}
#if !defined(FS_UT)
EXPORT_SYMBOL(fs_sync_frame);
#endif // FS_UT
/******************************************************************************/





/******************************************************************************/
// Frame Sync entry function
/******************************************************************************/
// export some Frame Sync member function.
static struct FrameSync frameSync = {
	fs_register_sensor,
	fs_unregister_sensor,
	fs_streaming,
	fs_set_sync,
	fs_sa_set_user_async_master,
	fs_sync_frame,
	fs_set_shutter,
	fs_update_shutter,
	fs_update_frame_length,
	fs_update_tg,
	fs_update_target_tg,
	fs_update_auto_flicker_mode,
	fs_update_min_fl_lc,
	fs_set_extend_framelength,
	fs_chk_exit_seamless_switch_frame,
	fs_chk_valid_for_doing_seamless_switch,
	fs_seamless_switch,
	fs_set_using_sa_mode,
	fs_set_frame_tag,
	fs_n_1_en,
	fs_mstream_en,
	fs_set_debug_info_sof_cnt,
	fs_notify_vsync,
	fs_notify_vsync_by_tsrec,
	fs_notify_sensor_hw_pre_latch_by_tsrec,
	fs_receive_tsrec_timestamp_info,
	fs_is_set_sync,
	fs_is_hw_sync,
	fs_get_fl_record_info,
	fs_clear_fl_restore_status_if_needed,
	fs_is_ts_src_type_tsrec,
};


/*
 * Frame Sync init function.
 *
 *    init FrameSync object.
 *    get FrameSync function for operation.
 *
 *    return: (0 / 1) => (no error / error)
 */
#if !defined(FS_UT)
unsigned int FrameSyncInit(struct FrameSync **pframeSync, struct device *dev)
#else /* ==> FS_UT */
unsigned int FrameSyncInit(struct FrameSync **pframeSync)
#endif
{
	int ret = 0;

	/* check NULL pointer */
	if (unlikely(pframeSync == NULL)) {
		LOG_MUST("ERROR: pframeSync is NULL, return 1");
		return 1;
	}

	fs_init();
	frm_init();

	*pframeSync = &frameSync;

#if !defined(FS_UT)
	ret = fs_con_create_sysfs_file(dev);
#endif

	return ret;
}


#if !defined(FS_UT)
void FrameSyncUnInit(struct device *dev)
{
	fs_con_remove_sysfs_file(dev);
}
#endif // FS_UT
/******************************************************************************/
