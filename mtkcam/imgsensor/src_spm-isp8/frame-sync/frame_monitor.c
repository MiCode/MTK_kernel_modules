// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "frame_monitor.h"
#include "frame_sync_util.h"

#ifndef FS_UT
#include <linux/of.h>
#include <linux/of_platform.h>

/* ==> define in frame_sync_def.h which is included by frame_monitor.h */
#ifdef SUPPORT_USING_CCU
#include <linux/remoteproc.h>
#include <linux/remoteproc/mtk_ccu.h>
#endif
#endif // !FS_UT


#ifdef FS_UT
// #include <stdint.h> // for uint32_t
typedef unsigned long long uint64_t;
#endif


/******************************************************************************/
// Log message
/******************************************************************************/
#include "frame_sync_log.h"

#define REDUCE_FRM_LOG
#define PFX "FrameMonitor"
#define FS_LOG_DBG_DEF_CAT LOG_FRM
/******************************************************************************/





/******************************************************************************/
// Frame Monitor Instance Structure (private structure)
/******************************************************************************/
//--------------------------- frame measurement ------------------------------//
struct FrameResult {
	/* TODO: add a auto check process for pred. act. mismatch */
	/* predicted_fl actual_fl is result match */
	// unsigned int is_result_match:1;

	/* predict current framelength */
	unsigned int predicted_fl_us;
	unsigned int predicted_fl_lc;

	/* actual framelength (by timestamp's diff) */
	unsigned long long actual_fl_us;
};


struct FrameMeasurement {
	unsigned int is_init:1;

	/* according to the number of vsync passed */
	/* ex: current idx is 2 and the number of vsync passed is 7 */
	/*     => 7 % VSYNCS_MAX = 3, current idx => (2 + 3) % VSYNCS_MAX = 1 */
	/*     => or (2 + 7) % VSYNCS_MAX = 1 */
	/*     all operation based on idx 1 to calculate correct data */
	unsigned int idx;
	struct FrameResult results[VSYNCS_MAX];

	unsigned long long timestamps[VSYNCS_MAX];
};
//----------------------------------------------------------------------------//


struct FrameInfo {
	unsigned int sensor_id;
	unsigned int sensor_idx;
	unsigned int tg;

	unsigned int wait_for_setting_predicted_fl:1;
	struct FrameMeasurement fmeas;

	/* vsync data information obtained by query */
	/* - vsync for sensor FL */
	struct vsync_time rec;
	unsigned long long query_ts_at_tick;
	unsigned long long query_ts_at_us;
	unsigned long long time_after_vsync;


#ifdef FS_UT
	unsigned int predicted_curr_fl_us; /* current predicted framelength (us) */
	unsigned int predicted_next_fl_us; /* next predicted framelength (us) */
	unsigned int sensor_curr_fl_us;    /* current framelength set to sensor */
	unsigned int next_vts_bias;        /* next vsync timestamp bias / shift */
#endif // FS_UT
};


struct FrameMonitorInst {
//----------------------------------------------------------------------------//

#ifndef FS_UT
	struct platform_device *ccu_pdev;
	phandle handle;
#endif

#ifdef SUPPORT_USING_CCU
	int ccu_pwr_on_cnt;
#endif

//----------------------------------------------------------------------------//

	enum fs_timestamp_src_type ts_src_type;

	unsigned long long cur_tick;
	unsigned int tick_factor;

	struct FrameInfo f_info[SENSOR_MAX_NUM];

	/* TSREC timestamp data */
	struct mtk_cam_seninf_tsrec_timestamp_info ts_info[SENSOR_MAX_NUM];

//----------------------------------------------------------------------------//

#ifdef FS_UT
	/* flag for using fake information (only for ut test and check) */
	unsigned int debug_flag:1;
#endif // FS_UT

//----------------------------------------------------------------------------//
};
static struct FrameMonitorInst frm_inst;


#if defined(FS_UT)
#define FS_UT_TG_MAPPING_SIZE 23
static const int ut_tg_mapping[FS_UT_TG_MAPPING_SIZE] = {
	-1, -1, -1, 0, 1,
	2, 3, 4, 5, 10,
	11, 12, 13, 14, 15,
	-1, -1, -1, -1, 6,
	7, 8, 9
};
#endif
/******************************************************************************/





/******************************************************************************/
// debug/utilities/dump functions
/******************************************************************************/
int frm_get_ts_src_type(void)
{
	return frm_inst.ts_src_type;
}


void dump_frame_info(const unsigned int idx, const char *caller)
{
	struct FrameInfo *p_f_info = &frm_inst.f_info[idx];

	LOG_MUST(
#if defined(TS_TICK_64_BITS)
		"[%s]: [%u] ID:%#x(sidx:%u), tg:%u, rec:(id:%u, vsyncs:%u, ts:(%llu/%llu/%llu/%llu)), query at:(%llu/+%llu(%llu))\n",
#else
		"[%s]: [%u] ID:%#x(sidx:%u), tg:%u, rec:(id:%u, vsyncs:%u, ts:(%u/%u/%u/%u)), query at:(%llu/+%llu(%llu))\n",
#endif
		caller, idx,
		p_f_info->sensor_id,
		p_f_info->sensor_idx,
		p_f_info->tg,
		p_f_info->rec.id,
		p_f_info->rec.vsyncs,
		p_f_info->rec.timestamps[0],
		p_f_info->rec.timestamps[1],
		p_f_info->rec.timestamps[2],
		p_f_info->rec.timestamps[3],
		p_f_info->query_ts_at_us,
		p_f_info->time_after_vsync,
		p_f_info->query_ts_at_tick);
}


void dump_vsync_recs(const struct vsync_rec (*pData), const char *caller)
{
	unsigned int i = 0;

	LOG_MUST(
#if defined(TS_TICK_64_BITS)
		"[%s]: ids:%u, cur_tick:%llu, tick_factor:%u\n",
#else
		"[%s]: ids:%u, cur_tick:%u, tick_factor:%u\n",
#endif
		caller,
		pData->ids,
		pData->cur_tick,
		pData->tick_factor);

	for (i = 0; i < pData->ids; ++i) {
		LOG_MUST(
#if defined(TS_TICK_64_BITS)
			"[%s]: recs[%u]: id:%u (TG), vsyncs:%u, ts:(%llu/%llu/%llu/%llu)\n",
#else
			"[%s]: recs[%u]: id:%u (TG), vsyncs:%u, ts:(%u/%u/%u/%u)\n",
#endif
			caller,
			i,
			pData->recs[i].id,
			pData->recs[i].vsyncs,
			pData->recs[i].timestamps[0],
			pData->recs[i].timestamps[1],
			pData->recs[i].timestamps[2],
			pData->recs[i].timestamps[3]);
	}
}


void frm_dump_measurement_data(const unsigned int idx,
	const unsigned int passed_vsyncs)
{
	struct FrameMeasurement *p_fmeas = &frm_inst.f_info[idx].fmeas;

	if (p_fmeas->idx == 0) {
		LOG_PF_INF(
			"[%u] ID:%#x (sidx:%u), tg:%d, vsync:%u, pred/act fl:(curr:%u,*0:%u(%u)/%llu, 1:%u(%u)/%llu, 2:%u(%u)/%llu, 3:%u(%u)/%llu), ts_tg_%u:(%llu/%llu/%llu/%llu), query_vts_at:%llu (SOF + %llu)\n",
			idx,
			frm_inst.f_info[idx].sensor_id,
			frm_inst.f_info[idx].sensor_idx,
			frm_inst.f_info[idx].tg,
			passed_vsyncs,
			p_fmeas->idx,
			p_fmeas->results[0].predicted_fl_us,
			p_fmeas->results[0].predicted_fl_lc,
			p_fmeas->results[0].actual_fl_us,
			p_fmeas->results[1].predicted_fl_us,
			p_fmeas->results[1].predicted_fl_lc,
			p_fmeas->results[1].actual_fl_us,
			p_fmeas->results[2].predicted_fl_us,
			p_fmeas->results[2].predicted_fl_lc,
			p_fmeas->results[2].actual_fl_us,
			p_fmeas->results[3].predicted_fl_us,
			p_fmeas->results[3].predicted_fl_lc,
			p_fmeas->results[3].actual_fl_us,
			frm_inst.f_info[idx].tg,
			p_fmeas->timestamps[0],
			p_fmeas->timestamps[1],
			p_fmeas->timestamps[2],
			p_fmeas->timestamps[3],
			frm_inst.f_info[idx].query_ts_at_us,
			frm_inst.f_info[idx].time_after_vsync);
	} else if (p_fmeas->idx == 1) {
		LOG_PF_INF(
			"[%u] ID:%#x (sidx:%u), tg:%d, vsync:%u, pred/act fl:(curr:%u, 0:%u(%u)/%llu,*1:%u(%u)/%llu, 2:%u(%u)/%llu, 3:%u(%u)/%llu), ts_tg_%u:(%llu/%llu/%llu/%llu), query_vts_at:%llu (SOF + %llu)\n",
			idx,
			frm_inst.f_info[idx].sensor_id,
			frm_inst.f_info[idx].sensor_idx,
			frm_inst.f_info[idx].tg,
			passed_vsyncs,
			p_fmeas->idx,
			p_fmeas->results[0].predicted_fl_us,
			p_fmeas->results[0].predicted_fl_lc,
			p_fmeas->results[0].actual_fl_us,
			p_fmeas->results[1].predicted_fl_us,
			p_fmeas->results[1].predicted_fl_lc,
			p_fmeas->results[1].actual_fl_us,
			p_fmeas->results[2].predicted_fl_us,
			p_fmeas->results[2].predicted_fl_lc,
			p_fmeas->results[2].actual_fl_us,
			p_fmeas->results[3].predicted_fl_us,
			p_fmeas->results[3].predicted_fl_lc,
			p_fmeas->results[3].actual_fl_us,
			frm_inst.f_info[idx].tg,
			p_fmeas->timestamps[0],
			p_fmeas->timestamps[1],
			p_fmeas->timestamps[2],
			p_fmeas->timestamps[3],
			frm_inst.f_info[idx].query_ts_at_us,
			frm_inst.f_info[idx].time_after_vsync);
	} else if (p_fmeas->idx == 2) {
		LOG_PF_INF(
			"[%u] ID:%#x (sidx:%u), tg:%d, vsync:%u, pred/act fl:(curr:%u, 0:%u(%u)/%llu, 1:%u(%u)/%llu,*2:%u(%u)/%llu, 3:%u(%u)/%llu), ts_tg_%u:(%llu/%llu/%llu/%llu), query_vts_at:%llu (SOF + %llu)\n",
			idx,
			frm_inst.f_info[idx].sensor_id,
			frm_inst.f_info[idx].sensor_idx,
			frm_inst.f_info[idx].tg,
			passed_vsyncs,
			p_fmeas->idx,
			p_fmeas->results[0].predicted_fl_us,
			p_fmeas->results[0].predicted_fl_lc,
			p_fmeas->results[0].actual_fl_us,
			p_fmeas->results[1].predicted_fl_us,
			p_fmeas->results[1].predicted_fl_lc,
			p_fmeas->results[1].actual_fl_us,
			p_fmeas->results[2].predicted_fl_us,
			p_fmeas->results[2].predicted_fl_lc,
			p_fmeas->results[2].actual_fl_us,
			p_fmeas->results[3].predicted_fl_us,
			p_fmeas->results[3].predicted_fl_lc,
			p_fmeas->results[3].actual_fl_us,
			frm_inst.f_info[idx].tg,
			p_fmeas->timestamps[0],
			p_fmeas->timestamps[1],
			p_fmeas->timestamps[2],
			p_fmeas->timestamps[3],
			frm_inst.f_info[idx].query_ts_at_us,
			frm_inst.f_info[idx].time_after_vsync);
	} else if (p_fmeas->idx == 3) {
		LOG_PF_INF(
			"[%u] ID:%#x (sidx:%u), tg:%d, vsync:%u, pred/act fl:(curr:%u, 0:%u(%u)/%llu, 1:%u(%u)/%llu, 2:%u(%u)/%llu,*3:%u(%u)/%llu), ts_tg_%u:(%llu/%llu/%llu/%llu), query_vts_at:%llu (SOF + %llu)\n",
			idx,
			frm_inst.f_info[idx].sensor_id,
			frm_inst.f_info[idx].sensor_idx,
			frm_inst.f_info[idx].tg,
			passed_vsyncs,
			p_fmeas->idx,
			p_fmeas->results[0].predicted_fl_us,
			p_fmeas->results[0].predicted_fl_lc,
			p_fmeas->results[0].actual_fl_us,
			p_fmeas->results[1].predicted_fl_us,
			p_fmeas->results[1].predicted_fl_lc,
			p_fmeas->results[1].actual_fl_us,
			p_fmeas->results[2].predicted_fl_us,
			p_fmeas->results[2].predicted_fl_lc,
			p_fmeas->results[2].actual_fl_us,
			p_fmeas->results[3].predicted_fl_us,
			p_fmeas->results[3].predicted_fl_lc,
			p_fmeas->results[3].actual_fl_us,
			frm_inst.f_info[idx].tg,
			p_fmeas->timestamps[0],
			p_fmeas->timestamps[1],
			p_fmeas->timestamps[2],
			p_fmeas->timestamps[3],
			frm_inst.f_info[idx].query_ts_at_us,
			frm_inst.f_info[idx].time_after_vsync);
	}
}
/******************************************************************************/





/******************************************************************************/
// Frame Monitor static function (private function)
/******************************************************************************/
/*
 * for timestamp source using CCU: plz dts add camera-fsync-ccu node.
 * for timestamp source using TSREC: dts "do NOT" add camera-fsync-ccu node.
 *
 * return: 0: NO error.
 *         1: find camera-fsync-ccu compatiable node failed.
 *         2: get ccu_pdev failed.
 *         < 0: read node property failed.
 */
static int get_dts_ccu_device_info(const char *caller)
{
#ifndef FS_UT
#ifdef SUPPORT_USING_CCU
	struct device_node *node = NULL, *rproc_np = NULL;
	phandle handle;
	int ret = 0;

	/* clear data */
	frm_inst.ccu_pdev = NULL;
	frm_inst.handle = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,camera-fsync-ccu");
	if (!node) {
		ret = 1;
		LOG_MUST(
			"[%s]: NOTICE: find DTS compatiable node:(mediatek,camera-fsync-ccu) failed, ret:%d\n",
			caller, ret);
		return ret;
	}

	/* return: 0 on success, */
	/*         -EINVAL if the property does not exist, */
	/*         -ENODATA if property does not have a value, and */
	/*         -EOVERFLOW if the property data isn't large enough */
	ret = of_property_read_u32(node, "mediatek,ccu-rproc", &handle);
	if (unlikely(ret < 0)) {
		LOG_MUST(
			"[%s]: ERROR: read DTS node:(mediatek,camera-fsync-ccu) property:(mediatek,ccu-rproc) failed, ret:%d\n",
			caller, ret);
		return ret;
	}

	rproc_np = of_find_node_by_phandle(handle);
	if (likely(rproc_np)) {
		frm_inst.ccu_pdev = of_find_device_by_node(rproc_np);
		if (unlikely(frm_inst.ccu_pdev == NULL)) {
			LOG_MUST(
				"[%s]: ERROR: find DTS device by node failed (ccu rproc pdev), ret:%d->2\n",
				caller, ret);
			ret = 2;
			return ret;
		}

		/* keep for rproc_get_by_phandle() using */
		frm_inst.handle = handle;
		LOG_MUST(
			"[%s]: get ccu proc pdev successfully, frm(ccu_pdev:%p), ret:%d\n",
			caller, frm_inst.ccu_pdev, ret);
	}

	return ret;

#else
	/* force to choose TSREC */
	LOG_MUST(
		"NOTICE: NOT define SUPPORT_USING_CCU => timestamp source set to TSREC, return 1\n");
	return 1;
#endif /* SUPPORT_USING_CCU */

#else
	/* for FS UT test, direct change return value for testing */
	return 0;
#endif /* !FS_UT */
}


static void frm_set_frame_info_vsync_rec_data(const struct vsync_rec (*pData))
{
	unsigned int i = 0, j = 0;

#if defined(FS_UT)
	dump_vsync_recs(pData, __func__);
#endif

	/* always keeps newest tick info (and tick factor) */
	frm_inst.cur_tick = pData->cur_tick;
	frm_inst.tick_factor = pData->tick_factor;

	for (i = 0; i < SENSOR_MAX_NUM; ++i) {
		struct FrameInfo *p_f_info = &frm_inst.f_info[i];

		/* for SA Frame-Sync, "ids" equal to 1 */
		for (j = 0; j < pData->ids; ++j) {
			const unsigned int rec_id = pData->recs[j].id;

			/* check each sensor info (if tg match) */
			if (p_f_info->tg == rec_id) {
				/* copy data */
				p_f_info->rec = pData->recs[j];
				p_f_info->query_ts_at_tick = pData->cur_tick;

				/* update frame info data */
				p_f_info->query_ts_at_us =
					(frm_inst.tick_factor > 0)
					? (p_f_info->query_ts_at_tick
						/ frm_inst.tick_factor)
					: 0;
				p_f_info->time_after_vsync =
					(p_f_info->query_ts_at_us > 0)
					? (p_f_info->query_ts_at_us
						- p_f_info->rec.timestamps[0])
					: 0;

#if defined(FS_UT)
				dump_frame_info(i, __func__);
#endif
			}
		}
	}
}


static void frm_save_vsync_timestamp(struct vsync_rec (*pData))
{
#ifdef FS_UT
	/* run in test mode */
	if (frm_inst.debug_flag) {
		// frm_inst.debug_flag = 0;
		frm_debug_copy_frame_info_vsync_rec_data(pData);

		return;
	}
#endif

	frm_set_frame_info_vsync_rec_data(pData);
}

#ifdef SUPPORT_USING_CCU
/*
 * This function is used by function frm_query_vsync_data()
 * that ONLY For case SUPPORT_USING_CCU
 */
static void frm_set_wait_for_setting_fmeas_by_tg(const unsigned int tgs[],
	const unsigned int len)
{
	unsigned int i = 0, j = 0;

	for (i = 0; i < SENSOR_MAX_NUM; ++i) {
		if (frm_inst.f_info[i].tg == 0)
			continue;

		for (j = 0; j < len; ++j) {
			if (frm_inst.f_info[i].tg == tgs[j]) {
				frm_inst.f_info[i]
					.wait_for_setting_predicted_fl = 1;
			}
		}
	}
}
#endif

static void frm_set_wait_for_setting_fmeas_by_idx(const unsigned int idxs[],
	const unsigned int len)
{
	unsigned int i;

	for (i = 0; (i < len) && (i < SENSOR_MAX_NUM); ++i) {
		const unsigned int idx = idxs[i];

		frm_inst.f_info[idx].wait_for_setting_predicted_fl = 1;
	}
}


static int frm_get_camsv_id(const unsigned int id)
{
#if !defined(FS_UT)

	struct device_node *dev_node = NULL;
	unsigned int camsv_id, cammux_id;
	int ret = -1;

	do {
		dev_node = of_find_compatible_node(dev_node, NULL,
			"mediatek,camsv");

		if (dev_node) {
			if (of_property_read_u32(dev_node,
					"mediatek,camsv-id", &camsv_id)
				|| of_property_read_u32(dev_node,
					"mediatek,cammux-id", &cammux_id)) {
				/* property not found */
				continue;
			}

			if (cammux_id == (id - 1)) {
				ret = camsv_id;
				break;
			}
		}
	} while (dev_node);

#if !defined(REDUCE_FRM_LOG)
	LOG_MUST(
		"get cammux_id:%u(from 1), camsv_id:%u(from 0), cammux_id:%u, ret:%d\n",
		id, camsv_id, cammux_id, ret);
#endif

	return ret;

#else
	return (id > 0 && id <= FS_UT_TG_MAPPING_SIZE)
		? ut_tg_mapping[id-1] : -1;
#endif // FS_UT
}
/******************************************************************************/





#ifdef SUPPORT_USING_CCU
/******************************************************************************/
// frame monitor --- CCU related functions (rproc, mtk_ccu, etc.)
/******************************************************************************/
static int query_ccu_vsync_data(struct vsync_rec (*pData))
{
	int ret = -1;

#ifdef FS_UT
	/* run in test mode */
	if (frm_inst.debug_flag)
		return 0;

#else /* ==> !FS_UT */
	if (unlikely(!frm_inst.ccu_pdev)) {
		ret = get_dts_ccu_device_info(__func__);
		if (unlikely(ret != 0))
			return ret;
	}

	/* using ccu_rproc_ipc_send to get vsync timestamp data */
	ret = mtk_ccu_rproc_ipc_send(
		frm_inst.ccu_pdev,
		MTK_CCU_FEATURE_FMCTRL,
		MSG_TO_CCU_GET_VSYNC_TIMESTAMP,
		(void *)pData, sizeof(struct vsync_rec));
#endif

	if (unlikely(ret != 0))
		LOG_MUST("ERROR: query CCU vsync data, ret:%d\n", ret);

	return ret;
}


void frm_reset_ccu_vsync_timestamp(
	const unsigned int idx, const unsigned int en)
{
	uint64_t selbits = 0;
	unsigned int tg = 0;
	int ret = 0;

	tg = frm_inst.f_info[idx].tg;

	/* case handling */
	if (unlikely(tg == 0 || tg == CAMMUX_ID_INVALID)) {
		LOG_MUST(
			"NOTICE: [%u] ID:%#x(sidx:%u), tg:%u(invalid), skip to call CCU reset(1)/clear(0):%u rproc\n",
			idx,
			frm_inst.f_info[idx].sensor_id,
			frm_inst.f_info[idx].sensor_idx,
			tg, en);
		return;
	}

	/* bit 0 no use, so "bit 1" --> means 2 */
	/* TG_1 -> bit 1, TG_2 -> bit 2, TG_3 -> bit 3 */
	selbits = ((uint64_t)1 << tg);

#ifndef FS_UT
	if (unlikely(!frm_inst.ccu_pdev)) {
		ret = get_dts_ccu_device_info(__func__);
		if (unlikely(ret != 0))
			return;
	}

	/* call CCU to reset vsync timestamp */
	ret = mtk_ccu_rproc_ipc_send(
		frm_inst.ccu_pdev,
		MTK_CCU_FEATURE_FMCTRL,
		(en)
			? MSG_TO_CCU_RESET_VSYNC_TIMESTAMP
			: MSG_TO_CCU_CLEAR_VSYNC_TIMESTAMP,
		(void *)&selbits, sizeof(selbits));
#endif

	if (unlikely(ret != 0))
		LOG_MUST(
			"ERROR: call CCU reset(1)/clear(0):%u, tg:%u (selbits:%#llx) vsync data, ret:%d\n",
			en, tg, selbits, ret);
	else
		LOG_MUST(
			"called CCU reset(1)/clear(0):%u, tg:%u (selbits:%#llx) vsync data, ret:%d\n",
			en, tg, selbits, ret);
}


int frm_get_ccu_pwn_cnt(void)
{
	return frm_inst.ccu_pwr_on_cnt;
}


void frm_power_on_ccu(const unsigned int flag)
{
#ifndef FS_UT
	struct rproc *ccu_rproc = NULL;
	int ret = 0;

	if (unlikely(!frm_inst.ccu_pdev)) {
		ret = get_dts_ccu_device_info(__func__);
		if (unlikely(ret != 0))
			return;
	}

	ccu_rproc = rproc_get_by_phandle(frm_inst.handle);
	if (unlikely(ccu_rproc == NULL)) {
		LOG_MUST(
			"ERROR: ccu rproc_get_by_phandle failed, ccu_rproc:%p NULL, return\n",
			ccu_rproc);
		return;
	}

	if (flag > 0) {
		/* boot up ccu */
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
		ret = rproc_bootx(ccu_rproc, RPROC_UID_FS);
#else
		ret = rproc_boot(ccu_rproc);
#endif

		if (unlikely(ret != 0)) {
			LOG_MUST(
				"ERROR: call ccu rproc_boot failed, ret:%d\n",
				ret);
			return;
		}

		frm_inst.ccu_pwr_on_cnt++;
	} else {
		/* shutdown ccu */
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
		rproc_shutdownx(ccu_rproc, RPROC_UID_FS);
#else
		rproc_shutdown(ccu_rproc);
#endif

		frm_inst.ccu_pwr_on_cnt--;
	}
#endif // !FS_UT
}
/******************************************************************************/
#endif // SUPPORT_USING_CCU





/******************************************************************************/
// frame measurement function
/******************************************************************************/
static void frm_init_measurement(const unsigned int idx,
	const unsigned int def_fl_us, const unsigned int def_fl_lc)
{
	struct FrameMeasurement *p_fmeas = &frm_inst.f_info[idx].fmeas;

	memset(&frm_inst.f_info[idx].fmeas, 0,
		sizeof(frm_inst.f_info[idx].fmeas));

	p_fmeas->results[p_fmeas->idx].predicted_fl_us = def_fl_us;
	p_fmeas->results[p_fmeas->idx].predicted_fl_lc = def_fl_lc;

	p_fmeas->results[p_fmeas->idx + 1].predicted_fl_us = def_fl_us;
	p_fmeas->results[p_fmeas->idx + 1].predicted_fl_lc = def_fl_lc;

	p_fmeas->is_init = 1;
}


static void frm_prepare_frame_measurement_info(
	const unsigned int idx, unsigned long long vdiff[])
{
	struct FrameInfo *p_f_info = &frm_inst.f_info[idx];
	unsigned int i = 0;

	/* update timestamp data in fmeas */
	for (i = 0; i < VSYNCS_MAX; ++i)
		p_f_info->fmeas.timestamps[i] = p_f_info->rec.timestamps[i];

	for (i = 0; i < VSYNCS_MAX-1; ++i) {
		/* correctly show info for debugging */
		const fs_timestamp_t tick_a =
			p_f_info->fmeas.timestamps[i] * frm_inst.tick_factor;
		const fs_timestamp_t tick_b =
			p_f_info->fmeas.timestamps[i+1] * frm_inst.tick_factor;

		vdiff[i] = (frm_inst.tick_factor)
			? ((tick_a - tick_b) / frm_inst.tick_factor) : 0;
	}
}


void frm_set_frame_measurement(
	unsigned int idx, unsigned int passed_vsyncs,
	unsigned int curr_fl_us, unsigned int curr_fl_lc,
	unsigned int next_fl_us, unsigned int next_fl_lc)
{
	struct FrameMeasurement *p_fmeas = &frm_inst.f_info[idx].fmeas;
	unsigned long long vdiff[VSYNCS_MAX] = {0};
	unsigned int meas_idx = 0, vts_idx = 0;
	unsigned int i = 0;


	if (unlikely(frm_inst.f_info[idx].wait_for_setting_predicted_fl == 0)) {
#if !defined(REDUCE_FRM_LOG) || defined(FS_UT)
		LOG_MUST(
			"ERROR: check flag, not wait for setting predicted fl, return\n");
#endif
		return;
	}

	if (unlikely(p_fmeas->is_init == 0))
		frm_init_measurement(idx, curr_fl_us, curr_fl_lc);


#ifdef FS_UT
	/* 0. for UT test */
	frm_update_predicted_fl_us(idx, curr_fl_us, next_fl_us);
#endif


	/* 1. update frame measurement reference idx */
	p_fmeas->idx = (p_fmeas->idx + passed_vsyncs) % VSYNCS_MAX;


	/* 2. set frame measurement predicted next frame length */
	meas_idx = p_fmeas->idx;


	/* 3. calculate actual frame length using vsync timestamp */
	frm_prepare_frame_measurement_info(idx, vdiff);

	/* ring back */
	vts_idx = (p_fmeas->idx + (VSYNCS_MAX-1)) % VSYNCS_MAX;
	for (i = 0; i < VSYNCS_MAX-1; ++i) {
		p_fmeas->results[vts_idx].actual_fl_us = vdiff[i];

		vts_idx = (vts_idx + (VSYNCS_MAX-1)) % VSYNCS_MAX;
	}

	p_fmeas->results[meas_idx].predicted_fl_us = curr_fl_us;
	p_fmeas->results[meas_idx].predicted_fl_lc = curr_fl_lc;
	/* for passing more vsyncs case */
	/*     i from 1 for preventing passed_vsyncs is 0 */
	/*     and (0-1) compare to (unsigned int) will cause overflow */
	/*     and preventing passed_vsyncs is a big value cause timeout */
	for (i = 1; (i < passed_vsyncs) && (i < VSYNCS_MAX+1); ++i) {
		p_fmeas->results[meas_idx].predicted_fl_us = curr_fl_us;
		p_fmeas->results[meas_idx].predicted_fl_lc = curr_fl_lc;

		meas_idx = (meas_idx + (VSYNCS_MAX-1)) % VSYNCS_MAX;
	}


	/* 4. dump frame measurement */
	// frm_dump_measurement_data(idx, passed_vsyncs);


	/* 5. update newest predict fl (lc / us) */
	/* ring forward the measurement idx */
	/* (because current predicted fl => +0; next predicted fl => +1) */
	meas_idx = (p_fmeas->idx + 1) % VSYNCS_MAX;

	p_fmeas->results[meas_idx].predicted_fl_us = next_fl_us;
	p_fmeas->results[meas_idx].predicted_fl_lc = next_fl_lc;


	/* x. clear check flag */
	frm_inst.f_info[idx].wait_for_setting_predicted_fl = 0;
}


void frm_get_curr_frame_mesurement_and_ts_data(
	const unsigned int idx, unsigned int *p_fmeas_idx,
	unsigned int *p_pr_fl_us, unsigned int *p_pr_fl_lc,
	unsigned long long *p_act_fl_us, unsigned long long *p_ts_arr)
{
	struct FrameMeasurement *p_fmeas = &frm_inst.f_info[idx].fmeas;
	unsigned int fmeas_idx = 0;
	unsigned int i = 0;

	if (unlikely(p_fmeas == NULL)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u), tg:%u, p_fmeas is NULL\n",
			idx,
			frm_inst.f_info[idx].sensor_id,
			frm_inst.f_info[idx].sensor_idx,
			frm_inst.f_info[idx].tg);
		return;
	}


	/* current result => ring back for get latest result */
	fmeas_idx = ((p_fmeas->idx) + (VSYNCS_MAX - 1)) % VSYNCS_MAX;
	if (p_fmeas_idx != NULL)
		*p_fmeas_idx = fmeas_idx;

	if (p_pr_fl_us != NULL)
		*p_pr_fl_us = p_fmeas->results[fmeas_idx].predicted_fl_us;
	if (p_pr_fl_lc != NULL)
		*p_pr_fl_lc = p_fmeas->results[fmeas_idx].predicted_fl_lc;
	if (p_act_fl_us != NULL)
		*p_act_fl_us = p_fmeas->results[fmeas_idx].actual_fl_us;
	if (p_ts_arr != NULL) {
		for (i = 0; i < VSYNCS_MAX; ++i)
			p_ts_arr[i] = p_fmeas->timestamps[i];
	}
}
/******************************************************************************/





/******************************************************************************/
// frame monitor function
/******************************************************************************/
void frm_reset_frame_info(const unsigned int idx)
{
	memset(&frm_inst.f_info[idx], 0, sizeof(frm_inst.f_info[idx]));
}


void frm_init_frame_info_st_data(const unsigned int idx,
	const unsigned int sensor_id, const unsigned int sensor_idx,
	const unsigned int tg)
{
	frm_reset_frame_info(idx);

	frm_inst.f_info[idx].sensor_id = sensor_id;
	frm_inst.f_info[idx].sensor_idx = sensor_idx;
	frm_inst.f_info[idx].tg = tg;

#ifdef SUPPORT_USING_CCU
#ifndef DELAY_CCU_OP
	if (frm_get_ts_src_type() == FS_TS_SRC_CCU
			&& (tg != 0 && tg != CAMMUX_ID_INVALID))
		frm_reset_ccu_vsync_timestamp(idx, 1);
#endif
#endif
}


void frm_update_tg(const unsigned int idx, const unsigned int tg)
{
	frm_inst.f_info[idx].tg = tg;
}


unsigned int frm_convert_cammux_id_to_ccu_tg_id(const unsigned int cammux_id)
{
	int camsv_id = frm_get_camsv_id(cammux_id);
	unsigned int ccu_tg_id;

	ccu_tg_id = (camsv_id >= 0) ? (camsv_id + CAMSV_TG_MIN) : cammux_id;

	LOG_MUST(
		"get cammux_id:%u(from 1), camsv_id:%d(from 0), ccu_tg_id:%u(CAMSV_TG_MIN:%u, CAMSV_TG_MAX:%u)\n",
		cammux_id, camsv_id, ccu_tg_id, CAMSV_TG_MIN, CAMSV_TG_MAX);

	return ccu_tg_id;
}


unsigned int frm_chk_and_get_tg_value(const unsigned int cammux_id,
	const unsigned int target_tg)
{
	unsigned int tg = CAMMUX_ID_INVALID;

	if ((cammux_id != 0) && (cammux_id != CAMMUX_ID_INVALID))
		tg = frm_convert_cammux_id_to_ccu_tg_id(cammux_id);

	if ((target_tg != 0) && (target_tg != CAMMUX_ID_INVALID))
		tg = target_tg;

	LOG_MUST(
		"get cammux_id:%u, target_tg:%u, => ret tg:%u\n",
		cammux_id, target_tg, tg);

	return tg;
}

#ifdef SUPPORT_USING_CCU
/*
 * This API is only for timestamp source from CCU!
 *
 * input:
 *     tgs: array of tg num for query timestamp
 *     len: array length
 *     *pData: pointer for timestamp dat of query needs to be written to
 *
 * return (0/non 0) for (done/error)
 */
int frm_query_vsync_data(const unsigned int tgs[], const unsigned int len,
	struct vsync_rec *pData)
{
	struct vsync_rec vsyncs_data = {0};
	unsigned int i = 0;
	int ret = 0;

	/* boundary checking */
	if (unlikely(len > TG_MAX_NUM)) {
		ret = 1;
		LOG_MUST(
			"ERROR: too many TGs:%u (bigger than CCU TG_MAX:%d), ret:%d\n",
			len, TG_MAX_NUM, ret);
		return ret;
	}

	/* 1. setup input Data */
	/*    for query vsync data from CCU, put TG(s) in the structure */
	vsyncs_data.ids = len;
	for (i = 0; i < len; ++i)
		vsyncs_data.recs[i].id = tgs[i];

	if (frm_get_ts_src_type() == FS_TS_SRC_CCU) {
		/* 2. get vsync data from CCU using rproc ipc send */
		ret = query_ccu_vsync_data(&vsyncs_data);
		if (unlikely(ret != 0))
			return ret;
	}

	/* 3. save data (in buffer) querying before to frame monitor */
	frm_save_vsync_timestamp(&vsyncs_data);

	/* 4. write back for caller */
	*pData = vsyncs_data;

#ifndef REDUCE_FRM_LOG
	dump_vsync_recs(pData, __func__);
#endif

	frm_set_wait_for_setting_fmeas_by_tg(tgs, len);

	return 0;
}
#endif /* SUPPORT_USING_CCU */

void frm_query_vsync_data_by_tsrec(
	const unsigned int idxs[], const unsigned int len,
	struct vsync_rec *pData)
{
	const unsigned int exp_id = TSREC_1ST_EXP_ID;   /* 1st exp => FL exp */
	unsigned long long latest_curr_tick = 0;
	unsigned int tick_factor = 0;
	unsigned int i, j;

	/* manually copy/setup info for vsync_time struct */
	for (i = 0; (i < len) && (i < SENSOR_MAX_NUM); ++i) {
		const unsigned int idx = idxs[i];
		const struct mtk_cam_seninf_tsrec_timestamp_info
			*p_ts_info = &frm_inst.ts_info[idx];

		/* tick factor (each one must be the same value) */
		tick_factor = p_ts_info->tick_factor;

		/* setup info to vsync_time struct */
		/* TODO: !!! FIX ME !!! this is workaround, */
		/*       using tg number to let sw flow can be run. */
		// pData->recs[i].id = p_ts_info->seninf_idx;
		pData->recs[i].id = frm_inst.f_info[idx].tg;
		pData->recs[i].vsyncs = 1; // for TSREC case.
		for (j = 0; j < VSYNCS_MAX; ++j) {
			pData->recs[i].timestamps[j] =
				p_ts_info->exp_recs[exp_id].ts_us[j];
		}

		/* use first curr_tick to init this variable */
		if (latest_curr_tick == 0)
			latest_curr_tick = p_ts_info->tsrec_curr_tick;
		/* , then find out latest curr_tick */
		if (check_tick_b_after_a(
				latest_curr_tick,
				p_ts_info->tsrec_curr_tick))
			latest_curr_tick = p_ts_info->tsrec_curr_tick;
	}

	/* manually copy/setup info for vsync_rec struct */
	pData->ids = len;
	pData->cur_tick = latest_curr_tick;
	pData->tick_factor = tick_factor;

	/* save data into frame monitor */
	frm_save_vsync_timestamp(pData);


#if !defined(REDUCE_FRM_LOG)
	dump_vsync_recs(pData, __func__);
#endif


	frm_set_wait_for_setting_fmeas_by_idx(idxs, len);
}


void frm_receive_tsrec_timestamp_info(const unsigned int idx,
	const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info)
{
	/* error handling (unexpected case) */
	if (unlikely(idx >= SENSOR_MAX_NUM)) {
		LOG_MUST(
			"ERROR: non-valid idx:%u (SENSOR_MAX_NUM:%u), return\n",
			idx, SENSOR_MAX_NUM);
		return;
	}

	memcpy(&frm_inst.ts_info[idx], ts_info, sizeof(frm_inst.ts_info[idx]));

#if !defined(REDUCE_FRM_LOG)
	LOG_MUST(
		"[%u] ID:%#x(sidx:%u), ts_info(tsrec_no:%u, seninf_idx:%u, irq(sys_ts:%llu(ns), tsrec_ts:%llu(us)), query at tsrec tick:%llu(%uMHz), exp_ts_us(0:(%llu/%llu/%llu/%llu), 1:(%llu/%llu/%llu/%llu), 2:(%llu/%llu/%llu/%llu)))\n",
		idx,
		frm_inst.f_info[idx].sensor_id,
		frm_inst.f_info[idx].sensor_idx,
		frm_inst.ts_info[idx].tsrec_no,
		frm_inst.ts_info[idx].seninf_idx,
		frm_inst.ts_info[idx].irq_sys_time_ns,
		frm_inst.ts_info[idx].irq_tsrec_ts_us,
		frm_inst.ts_info[idx].tsrec_curr_tick,
		frm_inst.ts_info[idx].tick_factor,
		frm_inst.ts_info[idx].exp_recs[0].ts_us[0],
		frm_inst.ts_info[idx].exp_recs[0].ts_us[1],
		frm_inst.ts_info[idx].exp_recs[0].ts_us[2],
		frm_inst.ts_info[idx].exp_recs[0].ts_us[3],
		frm_inst.ts_info[idx].exp_recs[1].ts_us[0],
		frm_inst.ts_info[idx].exp_recs[1].ts_us[1],
		frm_inst.ts_info[idx].exp_recs[1].ts_us[2],
		frm_inst.ts_info[idx].exp_recs[1].ts_us[3],
		frm_inst.ts_info[idx].exp_recs[2].ts_us[0],
		frm_inst.ts_info[idx].exp_recs[2].ts_us[1],
		frm_inst.ts_info[idx].exp_recs[2].ts_us[2],
		frm_inst.ts_info[idx].exp_recs[2].ts_us[3]);
#endif
}


const struct mtk_cam_seninf_tsrec_timestamp_info *
frm_get_tsrec_timestamp_info_ptr(const unsigned int idx)
{
	/* error handling (unexpected case) */
	if (unlikely(idx >= SENSOR_MAX_NUM)) {
		LOG_MUST(
			"ERROR: non-valid idx:%u (SENSOR_MAX_NUM:%u), return\n",
			idx, SENSOR_MAX_NUM);
		return NULL;
	}

	return (const struct mtk_cam_seninf_tsrec_timestamp_info *)
		&frm_inst.ts_info[idx];
}
/******************************************************************************/





/******************************************************************************/
// Debug function
/******************************************************************************/
#ifdef FS_UT
/* only for FrameSync Driver and ut test used */
int frm_get_instance_idx_by_tg(unsigned int tg)
{
	unsigned int i = 0;
	int ret = -1;

	for (i = 0; i < SENSOR_MAX_NUM; ++i) {
		if (frm_inst.f_info[i].tg == tg) {
			ret = i;
			break;
		}
	}

	return ret;
}


void frm_update_predicted_curr_fl_us(unsigned int idx, unsigned int fl_us)
{
	frm_inst.f_info[idx].predicted_curr_fl_us = fl_us;
}


void frm_update_next_vts_bias_us(unsigned int idx, unsigned int vts_bias)
{
	frm_inst.f_info[idx].next_vts_bias = vts_bias;
}


void frm_set_sensor_curr_fl_us(unsigned int idx, unsigned int fl_us)
{
	frm_inst.f_info[idx].sensor_curr_fl_us = fl_us;
}


void frm_update_predicted_fl_us(
	unsigned int idx,
	unsigned int curr_fl_us, unsigned int next_fl_us)
{
	frm_inst.f_info[idx].predicted_curr_fl_us = curr_fl_us;
	frm_inst.f_info[idx].predicted_next_fl_us = next_fl_us;
}

unsigned int frm_get_predicted_curr_fl_us(unsigned int idx)
{
	return frm_inst.f_info[idx].predicted_curr_fl_us;
}


void frm_get_predicted_fl_us(
	unsigned int idx,
	unsigned int fl_us[], unsigned int *sensor_curr_fl_us)
{
	fl_us[0] = frm_inst.f_info[idx].predicted_curr_fl_us;
	fl_us[1] = frm_inst.f_info[idx].predicted_next_fl_us;
	*sensor_curr_fl_us = frm_inst.f_info[idx].sensor_curr_fl_us;

#if !defined(REDUCE_FRM_LOG)
	LOG_INF(
		"f_info[%u](sensor_id:%#x / sensor_idx:%u / tg:%u / predicted_fl(c:%u, n:%u) / sensor_curr_fl:%u)\n",
		idx,
		frm_inst.f_info[idx].sensor_id,
		frm_inst.f_info[idx].sensor_idx,
		frm_inst.f_info[idx].tg,
		frm_inst.f_info[idx].predicted_curr_fl_us,
		frm_inst.f_info[idx].predicted_next_fl_us,
		frm_inst.f_info[idx].sensor_curr_fl_us);
#endif
}


void frm_get_next_vts_bias_us(unsigned int idx, unsigned int *vts_bias)
{
	*vts_bias = frm_inst.f_info[idx].next_vts_bias;
}


void frm_debug_copy_frame_info_vsync_rec_data(struct vsync_rec (*p_vsync_res))
{
	unsigned int i = 0, j = 0;

	/* always keeps newest tick info (and tick factor) */
	p_vsync_res->cur_tick = frm_inst.cur_tick;
	p_vsync_res->tick_factor = frm_inst.tick_factor;

#if !defined(REDUCE_FRM_LOG)
	LOG_MUST(
		"sync vsync rec data (cur tick:%u, tick factor:%u)\n",
		frm_inst.cur_tick,
		frm_inst.tick_factor);
#endif // REDUCE_FRM_LOG

	for (i = 0; i < p_vsync_res->ids; ++i) {
		unsigned int rec_id = p_vsync_res->recs[i].id;

		for (j = 0; j < SENSOR_MAX_NUM; ++j) {
			/* check info match */
			if (frm_inst.f_info[j].tg == rec_id) {
				/* copy data */
				p_vsync_res->recs[i] = frm_inst.f_info[j].rec;

#if !defined(REDUCE_FRM_LOG)
				LOG_MUST(
#if defined(TS_TICK_64_BITS)
					"rec_id:%u/tg:%u, sync data vsyncs:%u, ts:(%llu/%llu/%llu/%llu)\n",
#else
					"rec_id:%u/tg:%u, sync data vsyncs:%u, ts:(%u/%u/%u/%u)\n",
#endif
					p_vsync_res->recs[i].id,
					frm_inst.f_info[j].tg,
					p_vsync_res->recs[i].vsyncs,
					p_vsync_res->recs[i].timestamps[0],
					p_vsync_res->recs[i].timestamps[1],
					p_vsync_res->recs[i].timestamps[2],
					p_vsync_res->recs[i].timestamps[3]);
#endif
			}
		}
	}

	dump_vsync_recs(p_vsync_res, __func__);
}


void frm_debug_set_last_vsync_data(struct vsync_rec (*pData))
{
	frm_inst.debug_flag = 1;
	frm_set_frame_info_vsync_rec_data(pData);
}
#endif // FS_UT
/******************************************************************************/


/******************************************************************************/
// Frame Monitor init function.
/******************************************************************************/
void frm_init(void)
{
	int ret;

	if (likely(frm_inst.ts_src_type == FS_TS_SRC_UNKNOWN)) {
		ret = get_dts_ccu_device_info(__func__);

		switch (ret) {
		case 1:
			/* doesn't have camera-fsync-ccu node */
			frm_inst.ts_src_type = FS_TS_SRC_TSREC;
			break;
		case 0:
			/* has camera-fsync-ccu node and correctly get ccu dev */
			frm_inst.ts_src_type = FS_TS_SRC_CCU;
			break;
		default:
			/* has camera-fsync-ccu node but get ccu dev failed */
			frm_inst.ts_src_type = FS_TS_SRC_CCU;
			LOG_MUST(
				"ERROR: dts has camera-fsync-ccu node, but get ccu device failed(ret:%d)\n",
				ret);
			break;
		}

		LOG_MUST(
			"NOTICE: [get_dts_ccu_device_info]:ret:%d, => ts_src_type:%d(0:unknown/1:CCU/2:TSREC)\n",
			ret, frm_inst.ts_src_type);
	}
}
