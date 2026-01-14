// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include "frame-sync/frame_sync.h"
#include "frame-sync/sensor_recorder.h"

#include "mtk_camera-v4l2-controls-common.h"
#include "kd_imgsensor_define_v4l2.h"
#include "imgsensor-user.h"

#include "adaptor.h"
#include "adaptor-def.h"
#include "adaptor-common-ctrl.h"
#include "adaptor-subdrv-ctrl.h"
#include "adaptor-fsync-ctrls.h"
#include "adaptor-tsrec-cb-ctrl-impl.h"


/*******************************************************************************
 * fsync mgr log ctrl
 ******************************************************************************/
#define REDUCE_FSYNC_CTRLS_LOG
#define REDUCE_FSYNC_CTRLS_DBG_LOG

#define PFX "AdaptorFsyncCtrls"

#if !defined(FORCE_DISABLE_FSYNC_MGR)
#define FSYNC_MGR_LOG_INF(ctx, format, ...) { \
	dev_info(ctx->dev, PFX "[%s] " format, __func__, ##__VA_ARGS__); \
}

#define FSYNC_MGR_LOGD(ctx, format, ...) { \
	adaptor_logd(ctx, PFX "[%s] " format, __func__, ##__VA_ARGS__); \
}

#define FSYNC_MGR_LOGI(ctx, format, ...) { \
	adaptor_logi(ctx, PFX "[%s] " format, __func__, ##__VA_ARGS__); \
}

#else
#define FSYNC_MGR_LOG_INF(ctx, format, ...)
#define FSYNC_MGR_LOGD(ctx, format, ...)
#define FSYNC_MGR_LOGI(ctx, format, ...)
#endif // !FORCE_DISABLE_FSYNC_MGR


/*******************************************************************************
 * fsync mgr define/enum/structure
 ******************************************************************************/
#define FSYNC_WAIT_TSREC_UPDATE_DELAY_CNT (5)

static unsigned int is_fsync_ts_src_type_tsrec;


/*******************************************************************************
 * fsync mgr variables
 ******************************************************************************/
/* for checking if any sensor enter long exposure mode */
static atomic_t long_exp_mode_bits = ATOMIC_INIT(0);


/*******************************************************************************
 * fsync mgr static functions
 ******************************************************************************/
static void fsync_mgr_clear_output_info(struct adaptor_ctx *ctx)
{
	ctx->fsync_out_fl = 0;
	memset(ctx->fsync_out_fl_arr, 0,
		(sizeof(unsigned int) * IMGSENSOR_STAGGER_EXPOSURE_CNT));
	ctx->needs_fsync_assign_fl = 0;
}


static void fsync_mgr_reset_fsync_related_info(struct adaptor_ctx *ctx)
{
	fsync_mgr_clear_output_info(ctx);
	atomic_fetch_and((~(1UL << ctx->idx)), &long_exp_mode_bits);
}


/*******************************************************************************
 * for tsrec sen hw pre-latch info st
 ******************************************************************************/
void fsync_mgr_dump_tsrec_ts_info(struct adaptor_ctx *ctx,
	const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info,
	const char *caller)
{
	adaptor_logi(ctx,
		"[%s] idx:%d, ts_info(tsrec_no:%u, seninf_idx:%u, tick_factor:%u, pre_latch_exp_no:%u, sys_ts:%llu(ns), tsrec_ts:%llu(us), tick:%llu, ts(0:(%llu/%llu/%llu/%llu), 1:(%llu/%llu/%llu/%llu), 2:(%llu/%llu/%llu/%llu)))\n",
		caller,
		ctx->idx,
		ts_info->tsrec_no,
		ts_info->seninf_idx,
		ts_info->tick_factor,
		ts_info->irq_pre_latch_exp_no,
		ts_info->irq_sys_time_ns,
		ts_info->irq_tsrec_ts_us,
		ts_info->tsrec_curr_tick,
		ts_info->exp_recs[0].ts_us[0],
		ts_info->exp_recs[0].ts_us[1],
		ts_info->exp_recs[0].ts_us[2],
		ts_info->exp_recs[0].ts_us[3],
		ts_info->exp_recs[1].ts_us[0],
		ts_info->exp_recs[1].ts_us[1],
		ts_info->exp_recs[1].ts_us[2],
		ts_info->exp_recs[1].ts_us[3],
		ts_info->exp_recs[2].ts_us[0],
		ts_info->exp_recs[2].ts_us[1],
		ts_info->exp_recs[2].ts_us[2],
		ts_info->exp_recs[2].ts_us[3]);
}


static inline int fsync_mgr_cb_tsrec_read_ts_recs(struct adaptor_ctx *ctx,
	struct mtk_cam_seninf_tsrec_timestamp_info *ts_info)
{
	return (adaptor_tsrec_cb_ctrl_execute(ctx,
		TSREC_CB_CMD_READ_TS_INFO, ts_info, __func__));
}


static inline void fsync_mgr_init_tsrec_sen_hw_pre_latch_info(
	struct adaptor_ctx *ctx)
{
	if (unlikely(ctx->fsync_mgr == NULL))
		is_fsync_ts_src_type_tsrec = 0;
	else {
		is_fsync_ts_src_type_tsrec =
			ctx->fsync_mgr->fs_is_ts_src_type_tsrec();
	}
	spin_lock_init(&ctx->fsync_pre_latch_ts_info_update_lock);
}


static inline void fsync_mgr_reset_tsrec_sen_hw_pre_latch_ts_info(
	struct adaptor_ctx *ctx)
{
	spin_lock(&ctx->fsync_pre_latch_ts_info_update_lock);
	memset(&ctx->fsync_pre_latch_ts_info, 0,
		sizeof(ctx->fsync_pre_latch_ts_info));
	spin_unlock(&ctx->fsync_pre_latch_ts_info_update_lock);
}


static inline void fsync_mgr_update_tsrec_sen_hw_pre_latch_ts_info(
	struct adaptor_ctx *ctx,
	const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info)
{
	spin_lock(&ctx->fsync_pre_latch_ts_info_update_lock);
	ctx->fsync_pre_latch_ts_info = *ts_info;
	spin_unlock(&ctx->fsync_pre_latch_ts_info_update_lock);
}


/* return: 0 => all match (or ts_info read from tsrec is all 0) => skip waiting */
static int fsync_mgr_cmp_tsrec_sen_hw_pre_latch_ts_info(
	struct adaptor_ctx *ctx,
	const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info)
{
	unsigned int unmatch_exp_id_bits = 0;
	unsigned int i;

	spin_lock(&ctx->fsync_pre_latch_ts_info_update_lock);
	/* compare timestamp of each exp id */
	for (i = 0; i < TSREC_EXP_MAX_CNT; ++i) {
		const unsigned int pre_latch_exp_no =
			ctx->fsync_pre_latch_ts_info.irq_pre_latch_exp_no;

		if ((i != pre_latch_exp_no) || (ts_info->exp_recs[i].ts_us[0] == 0))
			continue;
		if (ctx->fsync_pre_latch_ts_info.exp_recs[i].ts_us[0]
				!= ts_info->exp_recs[i].ts_us[0])
			unmatch_exp_id_bits |= (1UL << i);
	}
	spin_unlock(&ctx->fsync_pre_latch_ts_info_update_lock);

	return unmatch_exp_id_bits;
}


static void fsync_mgr_chk_wait_tsrec_hw_pre_latch_updated(
	struct adaptor_ctx *ctx)
{
	struct mtk_cam_seninf_tsrec_timestamp_info ts_info = {0};
	const unsigned int delay_us[FSYNC_WAIT_TSREC_UPDATE_DELAY_CNT] =
		{500, 500, 400, 300, 300};
	unsigned long long curr_ts, after_vsync;
	unsigned int factor;
	unsigned int i = 0;
	int ret;

	/* bypass case */
	if ((is_fsync_ts_src_type_tsrec == 0) || (!ctx->is_streaming))
		return;

	ret = fsync_mgr_cb_tsrec_read_ts_recs(ctx, &ts_info);
	if (unlikely(ret != TSREC_CB_CTRL_ERR_NONE)) {
		/* for sensor that data not pass through tsrec hw, e.g., AOV */
		if (unlikely(ret == TSREC_CB_CTRL_ERR_NOT_CONNECTED_TO_TSREC))
			return;

		adaptor_logi(ctx,
			"ERROR: idx:%d, cb tsrec to read ts, ret:%d(!= TSREC_CB_CTRL_ERR_NONE:%d), skip compare ts info\n",
			ctx->idx, ret, TSREC_CB_CTRL_ERR_NONE);
		return;
	}

	/* waiting & compare */
	while ((fsync_mgr_cmp_tsrec_sen_hw_pre_latch_ts_info(ctx, &ts_info) > 0)
			&& (i < FSYNC_WAIT_TSREC_UPDATE_DELAY_CNT))
		udelay(delay_us[i++]);

	/* prepare info for printing */
	factor = (ts_info.tick_factor) ? ts_info.tick_factor : 1;
	curr_ts = (ts_info.tsrec_curr_tick / factor);
	after_vsync =
		(ts_info.tsrec_curr_tick - (ts_info.exp_recs[0].ts_us[0] * factor)) / factor;

	if (unlikely(i >= FSYNC_WAIT_TSREC_UPDATE_DELAY_CNT)) {
		adaptor_logi(ctx,
			"WARNING: timeout i:%u[%u/%u/%u/%u/%u](us) => bypass, ts(%u/inf:%u, pre_latch_exp:%u,%llu(+%llu),(0:(%llu/%llu/%llu/%llu)/1:(%llu/%llu/%llu/%llu)/2:(%llu/%llu/%llu/%llu)))\n",
			i, delay_us[0], delay_us[1], delay_us[2], delay_us[3], delay_us[4],
			ts_info.tsrec_no, ts_info.seninf_idx, ts_info.irq_pre_latch_exp_no,
			curr_ts, after_vsync,
			ts_info.exp_recs[0].ts_us[0],
			ts_info.exp_recs[0].ts_us[1],
			ts_info.exp_recs[0].ts_us[2],
			ts_info.exp_recs[0].ts_us[3],
			ts_info.exp_recs[1].ts_us[0],
			ts_info.exp_recs[1].ts_us[1],
			ts_info.exp_recs[1].ts_us[2],
			ts_info.exp_recs[1].ts_us[3],
			ts_info.exp_recs[2].ts_us[0],
			ts_info.exp_recs[2].ts_us[1],
			ts_info.exp_recs[2].ts_us[2],
			ts_info.exp_recs[2].ts_us[3]);
	} else if (unlikely(i > 0)) {
		adaptor_logd(ctx,
			"NOTICE: i:%u[%u/%u/%u/%u/%u](us) => waiting is over, ts(%u/inf:%u, pre_latch_exp:%u,%llu(+%llu),(0:(%llu/%llu/%llu/%llu)/1:(%llu/%llu/%llu/%llu)/2:(%llu/%llu/%llu/%llu)))\n",
			i, delay_us[0], delay_us[1], delay_us[2], delay_us[3], delay_us[4],
			ts_info.tsrec_no, ts_info.seninf_idx, ts_info.irq_pre_latch_exp_no,
			curr_ts, after_vsync,
			ts_info.exp_recs[0].ts_us[0],
			ts_info.exp_recs[0].ts_us[1],
			ts_info.exp_recs[0].ts_us[2],
			ts_info.exp_recs[0].ts_us[3],
			ts_info.exp_recs[1].ts_us[0],
			ts_info.exp_recs[1].ts_us[1],
			ts_info.exp_recs[1].ts_us[2],
			ts_info.exp_recs[1].ts_us[3],
			ts_info.exp_recs[2].ts_us[0],
			ts_info.exp_recs[2].ts_us[1],
			ts_info.exp_recs[2].ts_us[2],
			ts_info.exp_recs[2].ts_us[3]);
	}
}


/*******************************************************************************
 * sensor driver feature ctrls
 ******************************************************************************/
static u32 fsync_mgr_g_sensor_hw_sync_mode(struct adaptor_ctx *ctx)
{
	union feature_para para;
	u32 sync_mode = 0;
	u32 len;

	para.u32[0] = 0;

	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_SENSOR_SYNC_MODE,
		para.u8, &len);

	sync_mode = para.u32[0];

#if !defined(REDUCE_FSYNC_CTRLS_LOG)
	FSYNC_MGR_LOGI(ctx,
		"sidx:%d, get hw sync mode:%u(N:0/M:1/S:2)\n",
		ctx->idx,
		sync_mode);
#endif

	return sync_mode;
}

static void fsync_mgr_s_frame_length(struct adaptor_ctx *ctx)
{
	enum ACDK_SENSOR_FEATURE_ENUM cmd;
	union feature_para para;
	u32 len;

	para.u64[0] = ctx->fsync_out_fl;
	para.u64[1] = (u64)ctx->fsync_out_fl_arr;

	cmd = (ctx->fsync_out_fl_arr[0])
		? SENSOR_FEATURE_SET_FRAMELENGTH_IN_LUT
		: SENSOR_FEATURE_SET_FRAMELENGTH;

	subdrv_call(ctx, feature_control, cmd, para.u8, &len);
}

static void fsync_mgr_s_multi_shutter_frame_length(
	struct adaptor_ctx *ctx,
	u64 *ae_exp_arr, u32 ae_exp_cnt,
	const unsigned int multi_exp_type)
{
	enum ACDK_SENSOR_FEATURE_ENUM cmd;
	union feature_para para;
	u64 fsync_exp[IMGSENSOR_STAGGER_EXPOSURE_CNT] = {0};
	u32 len = 0;
	int i;

	if (likely(ae_exp_arr != NULL)) {
		for (i = 0;
			(i < ae_exp_cnt) && (i < IMGSENSOR_STAGGER_EXPOSURE_CNT);
			++i)
			fsync_exp[i] = (*(ae_exp_arr + i));
	}

	para.u64[0] = (u64)fsync_exp;
	para.u64[1] = min_t(u32, ae_exp_cnt, (u32)IMGSENSOR_STAGGER_EXPOSURE_CNT);
	para.u64[2] = ctx->fsync_out_fl;
	para.u64[3] = (u64)ctx->fsync_out_fl_arr;

	cmd = (multi_exp_type == MULTI_EXP_TYPE_LBMF)
		? SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME_IN_LUT
		: SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME;

	subdrv_call(ctx, feature_control, cmd, para.u8, &len);
}


/*******************************************************************************
 * fsync mgr static functions
 ******************************************************************************/
static void fsync_mgr_update_sensor_actual_fl_info(struct adaptor_ctx *ctx,
	struct fs_perframe_st *p_pf_ctrl)
{
	const unsigned int mode_exp_cnt = p_pf_ctrl->hdr_exp.mode_exp_cnt;
	unsigned int i;

	p_pf_ctrl->out_fl_lc = ctx->subctx.frame_length_rg;

	if (p_pf_ctrl->hdr_exp.multi_exp_type == MULTI_EXP_TYPE_LBMF) {
		for (i = 0; i < mode_exp_cnt; ++i) {
#ifdef FL_ARR_IN_LUT_ORDER
			const int fl_idx = i;
#else
			const int fl_idx = hdr_exp_idx_map[mode_exp_cnt][i];
#endif

			p_pf_ctrl->hdr_exp.fl_lc[fl_idx] =
				ctx->subctx.frame_length_in_lut_rg[i];
		}
	}
}


static void fsync_mgr_chk_long_exposure(struct adaptor_ctx *ctx,
	const u64 *ae_exp_arr, const u32 ae_exp_cnt)
{
	unsigned int i = 0;
	int has_long_exp = 0;
	int fine_integ_line = 0;

	fine_integ_line =
		g_sensor_fine_integ_line(ctx, ctx->subctx.current_scenario_id);

	for (i = 0; i < ae_exp_cnt; ++i) {
		u32 exp_lc = (u32)
			FINE_INTEG_CONVERT(ae_exp_arr[i], fine_integ_line);

		/* check if any exp will enter long exposure mode */
		if ((exp_lc + ctx->subctx.margin) >=
				ctx->subctx.max_frame_length) {
			has_long_exp = 1;
			break;
		}
	}

	/* has_long_exp > 0 => set bits ; has_long_exp == 0 => clear bits */
	if (has_long_exp != 0) {
		atomic_fetch_or((1UL << ctx->idx), &long_exp_mode_bits);

		/* clear FL restore info if needed immediately */
		/* due to flow will return to driver not into Frame-Sync */
		ctx->fsync_mgr->fs_clear_fl_restore_status_if_needed(ctx->idx);

		/* clear fsync output info */
		/* due to flow will return to driver not into Frame-Sync */
		fsync_mgr_clear_output_info(ctx);
	} else
		atomic_fetch_and((~(1UL << ctx->idx)), &long_exp_mode_bits);

#if !defined(REDUCE_FSYNC_CTRLS_LOG)
	FSYNC_MGR_LOGI(ctx,
		"NOTICE: sidx:%d, detect long exp:%d, long_exp_mode_bits:%#x\n",
		ctx->idx,
		has_long_exp, atomic_read(&long_exp_mode_bits));
#endif
}


static void fsync_mgr_setup_sensor_hdr_info(struct adaptor_ctx *ctx,
	struct fs_hdr_exp_st *p_hdr_exp, const u32 mode_id)
{
	struct adaptor_sensor_lbmf_property_st lbmf_prop = {0};

	/* set mode exp cnt (e.g., stagger: 1 ~ 3) */
	p_hdr_exp->mode_exp_cnt = g_scenario_exposure_cnt(ctx, mode_id);
	p_hdr_exp->readout_len_lc = ctx->subctx.readout_length;
	p_hdr_exp->read_margin_lc = ctx->subctx.read_margin;

	/* setup multi exp type (HDR type, e.g., stagger, LB-MF, etc.) */
	if (g_sensor_lbmf_property(ctx, mode_id, &lbmf_prop)) {
		p_hdr_exp->multi_exp_type = MULTI_EXP_TYPE_LBMF;

		switch (lbmf_prop.exp_order) {
		case IMGSENSOR_LBMF_EXPOSURE_LE_FIRST:
			p_hdr_exp->exp_order = EXP_ORDER_LE_1ST;
			break;
		case IMGSENSOR_LBMF_EXPOSURE_SE_FIRST:
			p_hdr_exp->exp_order = EXP_ORDER_SE_1ST;
			break;
		case IMGSENSOR_LBMF_EXPOSURE_ORDER_SUPPORT_NONE:
			FSYNC_MGR_LOGI(ctx,
				"ERROR: sidx:%d, detect sensor mode is LBMF HDR, but exp order is SUPPORT NONE, treat as SE first\n",
				ctx->idx);
			p_hdr_exp->exp_order = EXP_ORDER_SE_1ST;
			break;
		default:
			FSYNC_MGR_LOGI(ctx,
				"ERROR: sidx:%d, detect sensor mode is LBMF HDR, but exp order is unknown (%d), treat as SE first\n",
				ctx->idx, lbmf_prop.exp_order);
			p_hdr_exp->exp_order = EXP_ORDER_SE_1ST;
			break;
		}
	} else if (g_sensor_dcg_property(ctx, mode_id)) {
		/* => DCG */
		p_hdr_exp->mode_exp_cnt = 1;
	} else {
		/* => stagger */
		p_hdr_exp->multi_exp_type = MULTI_EXP_TYPE_STG;
	}

#ifndef REDUCE_FSYNC_CTRLS_DBG_LOG
	FSYNC_MGR_LOGI(ctx,
		"sidx:%d, set p_hdr_exp(multi_exp_type:%u(STG:%d/LBMF:%d)/mode_exp_cnt:%u/exp_order:%u)\n",
		ctx->idx,
		p_hdr_exp->multi_exp_type,
		MULTI_EXP_TYPE_STG,
		MULTI_EXP_TYPE_LBMF,
		p_hdr_exp->mode_exp_cnt,
		p_hdr_exp->exp_order);
#endif
}

static void fsync_mgr_setup_all_exp_data(struct adaptor_ctx *ctx,
	unsigned int *p_shutter_lc, struct fs_hdr_exp_st *p_hdr_exp,
	const u64 *ae_exp_arr, const u32 ae_exp_cnt, const u32 mode_id)
{
	struct mtk_stagger_info info = {0};
	unsigned int i = 0;
	int fine_integ_line;
	int ret = 0;
	u64 ae_exp;

	/* error handle */
	if (unlikely(p_hdr_exp == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, get p_hdr_exp:%p is nullptr, return\n",
			ctx->idx, p_hdr_exp);
		return;
	}

	/* !!! start setup all exp data (shutter_lc & fs_hdr_exp_st) !!! */
	fine_integ_line = g_sensor_fine_integ_line(ctx, mode_id);

	/* ==> for hdr-exp settings, e.g. STG sensor */
	info.scenario_id = SENSOR_SCENARIO_ID_NONE;
	ret = g_stagger_info(ctx, mode_id, &info);
	if (!ret) {
		fsync_mgr_setup_sensor_hdr_info(ctx, p_hdr_exp, mode_id);

		// p_hdr_exp->mode_exp_cnt = info.count;
		p_hdr_exp->ae_exp_cnt = ae_exp_cnt;
		for (i = 0; i < ae_exp_cnt; ++i) {
			const int idx = hdr_exp_idx_map[ae_exp_cnt][i];

#ifdef FL_ARR_IN_LUT_ORDER
			const int fl_idx = i;
#else
			const int fl_idx = idx;
#endif

			if (likely(idx >= 0)) {
				ae_exp = ae_exp_arr[i];
				if (fine_integ_line) {
					p_hdr_exp->exp_lc[idx] =
						FINE_INTEG_CONVERT(ae_exp, fine_integ_line);
				} else
					p_hdr_exp->exp_lc[idx] = ae_exp;

				if (p_hdr_exp->multi_exp_type ==
						MULTI_EXP_TYPE_LBMF) {
					p_hdr_exp->fl_lc[fl_idx] = 0;
						// ctx->subctx.frame_length_in_lut_rg[i];
				}

#ifndef REDUCE_FSYNC_CTRLS_DBG_LOG
				FSYNC_MGR_LOGI(ctx,
					"ae_exp_arr[%u]:%u, fine_integ_line:%d, p_hdr_exp(exp_lc[%d]:%u, fl_lc[%d]:%u)\n",
					i, ae_exp_arr[i], fine_integ_line,
					idx, p_hdr_exp->exp_lc[idx],
					fl_idx, p_hdr_exp->fl_lc[fl_idx]);
#endif

			} else {
				FSYNC_MGR_LOGI(ctx,
					"ERROR: idx:%d (< 0) = hdr_exp_idx_map[%u][%u]\n",
					idx, ae_exp_cnt, i);
			}
		}
	}

	/* ==> for ae_exp_cnt == 1 or mode_exp_cnt == 1, e.g. DCG (ae:2/mode:1) */
	ae_exp = (ae_exp_cnt == 1 || p_hdr_exp->mode_exp_cnt == 1)
		? *(ae_exp_arr + 0) : 0;
	*p_shutter_lc = (fine_integ_line)
		? FINE_INTEG_CONVERT(ae_exp, fine_integ_line) : (u32)ae_exp;
}

static void fsync_mgr_setup_exp_data(struct adaptor_ctx *ctx,
	struct fs_perframe_st *p_pf_ctrl, const u32 mode_id,
	u64 *ae_exp_arr, int ae_exp_cnt)
{
	int i;

	/* error handle */
	if (unlikely(ae_exp_arr == NULL || ae_exp_cnt == 0)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, get ae_exp_arr:%p is NULL, ae_exp_cnt:%u, return\n",
			ctx->idx, ae_exp_arr, ae_exp_cnt);
		return;
	}
	if (unlikely(ae_exp_cnt < 0)) {
		ae_exp_cnt = 0;
		for (i = 0; i < IMGSENSOR_STAGGER_EXPOSURE_CNT; ++i) {
			/* check how many non zero exp setting */
			if (*(ae_exp_arr + i) != 0)
				ae_exp_cnt++;
		}
	}

	fsync_mgr_setup_all_exp_data(ctx,
		&p_pf_ctrl->shutter_lc, &p_pf_ctrl->hdr_exp,
		ae_exp_arr, ae_exp_cnt, mode_id);
}


/*******************************************************************************
 * call back function for Frame-Sync set frame length using
 ******************************************************************************/
/* return: 0 => No-Error ; non-0 => Error */
int cb_func_fsync_mgr_set_fl_info(void *p_ctx, const unsigned int cmd_id,
	const void *pf_info, const unsigned int fl_lc,
	const unsigned int fl_lc_arr[], const unsigned int arr_len)
{
	struct adaptor_ctx *ctx;
	const unsigned int cnt = min_t(u32, arr_len, FS_HDR_MAX);
	unsigned int i;
	int ret = 0;

	/* error handle */
	if (unlikely(p_ctx == NULL)) {
		pr_info(
			"[%s] ERROR: get nullptr, p_ctx:%p, pf_info:%p  [cmd_id:%u, fl_lc:%u, fl_lc_arr:(%u/%u/%u/%u/%u), arr_len:%u], return:1\n",
			__func__,
			p_ctx, pf_info,
			cmd_id, fl_lc,
			fl_lc_arr[0],
			fl_lc_arr[1],
			fl_lc_arr[2],
			fl_lc_arr[3],
			fl_lc_arr[4],
			arr_len);
		return 1;
	}

	ctx = (struct adaptor_ctx *)p_ctx;
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL (cmd_id:%u, fl_lc:%u, fl_lc_arr:(%u/%u/%u/%u/%u)), arr_len:%u, return:2\n",
			ctx->idx, ctx->fsync_mgr,
			cmd_id, fl_lc,
			fl_lc_arr[0],
			fl_lc_arr[1],
			fl_lc_arr[2],
			fl_lc_arr[3],
			fl_lc_arr[4],
			arr_len);
		return 2;
	}

	/* update fl info to ctx */
	ctx->fsync_out_fl = fl_lc;
	for (i = 0; i < cnt; ++i) {
#ifdef FL_ARR_IN_LUT_ORDER
		const int fl_idx = i;
#else
		const int fl_idx = hdr_exp_idx_map[mode_exp_cnt][i];
#endif

		ctx->fsync_out_fl_arr[fl_idx] = fl_lc_arr[i];
	}


#ifndef REDUCE_FSYNC_CTRLS_DBG_LOG
	FSYNC_MGR_LOGI(ctx,
		"sidx:%d, update fsync_out_fl(lc):(%u, %u/%u/%u/%u/%u)\n",
		ctx->idx,
		ctx->fsync_out_fl,
		ctx->fsync_out_fl_arr[0],
		ctx->fsync_out_fl_arr[1],
		ctx->fsync_out_fl_arr[2],
		ctx->fsync_out_fl_arr[3],
		ctx->fsync_out_fl_arr[4]);
#endif


	switch (cmd_id) {
	case FSYNC_CTRL_FL_CMD_ID_EXP_WITH_FL:
	{
		// cmd_type = SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME;
		/* !!! set exp with fl & update fl at set shutter func !!! */
	}
		break;
	case FSYNC_CTRL_FL_CMD_ID_FL:
	{
		// struct fs_perframe_st *p_pf_ctrl;

		// cmd_type = SENSOR_FEATURE_SET_FRAMELENGTH;
		// p_pf_ctrl = (struct fs_perframe_st *)pf_info;

		/* set frame length */
		fsync_mgr_s_frame_length(ctx);

		/* update sensor current fl to Frame-Sync */
		ctx->fsync_mgr->fs_update_frame_length(ctx->idx,
			ctx->fsync_out_fl, ctx->fsync_out_fl_arr, FS_HDR_MAX);
	}
		break;
	default:
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, get unknown cmd_id:%u, do nothing\n",
			ctx->idx, cmd_id);
		break;
	}

	return ret;
}


/*******************************************************************************
 * streaming ctrls
 ******************************************************************************/
void fsync_mgr_dump_fs_streaming_st(struct adaptor_ctx *ctx,
	const struct fs_streaming_st *s_info, const unsigned int flag,
	const char *caller)
{
	FSYNC_MGR_LOGI(ctx,
		"[%s] flag:%u, ID:%#x(sidx:%u), cammux_id:%u/target_tg:%u, fdelay:%u(must be 3 or 2), fl_lc(def:%u/max:%u), def_exp_lc:%u, margin_lc:%u, lineTime:%u(pclk:%llu/linelength:%u), hw_sync_mode:%u(N:0/M:1/S:2)\n",
		caller,
		flag,
		s_info->sensor_id,
		s_info->sensor_idx,
		s_info->cammux_id,
		s_info->target_tg,
		s_info->fl_active_delay,
		s_info->def_fl_lc,
		s_info->max_fl_lc,
		s_info->def_shutter_lc,
		s_info->margin_lc,
		s_info->lineTimeInNs,
		s_info->pclk,
		s_info->linelength,
		s_info->sync_mode);
}

static void fsync_mgr_setup_fs_streaming_st(struct adaptor_ctx *ctx,
	struct fs_streaming_st *s_info)
{
	const unsigned int mode_id = ctx->subctx.current_scenario_id;
	unsigned int i;

	memset(s_info, 0, sizeof(*s_info));

	s_info->sensor_id = (ctx->subdrv) ? (ctx->subdrv->id) : 0;
	s_info->sensor_idx = ctx->idx;

	/* fsync_map_id is cam_mux no */
	s_info->cammux_id = (ctx->fsync_map_id->val > 0)
		? (ctx->fsync_map_id->val + 1) : 0;
	/* 7s use fsync_listen_target to update ccu tg id, so init from this */
	s_info->target_tg = ctx->fsync_listen_target->val;

	s_info->fl_active_delay = ctx->subctx.frame_time_delay_frame;

	/* for any settings before streaming on */
	s_info->def_fl_lc = ctx->subctx.frame_length_rg;
	s_info->max_fl_lc = ctx->subctx.max_frame_length;

	/* frame sync sensor operate mode. none/master/slave */
	s_info->sync_mode = fsync_mgr_g_sensor_hw_sync_mode(ctx);


	/* using ctx->subctx.shutter instead of ctx->subctx.exposure_def */
	/* for any settings before streaming on */
	s_info->def_shutter_lc = ctx->subctx.shutter;
	s_info->margin_lc = g_sensor_margin(ctx, ctx->subctx.current_scenario_id);

	fsync_mgr_setup_sensor_hdr_info(ctx, &s_info->hdr_exp, mode_id);
	if (s_info->hdr_exp.mode_exp_cnt > 1) {
		for (i = 0; i < s_info->hdr_exp.mode_exp_cnt; ++i) {
			const int idx =
				hdr_exp_idx_map[s_info->hdr_exp.mode_exp_cnt][i];

			if (unlikely(idx < 0)) {
				FSYNC_MGR_LOGI(ctx,
					"ERROR: idx:%d (< 0) = hdr_exp_idx_map[%u][%u]\n",
					idx, s_info->hdr_exp.mode_exp_cnt, i);
				continue;
			}

			s_info->hdr_exp.exp_lc[idx] = ctx->subctx.exposure[i];
			if (s_info->hdr_exp.multi_exp_type == MULTI_EXP_TYPE_LBMF) {
#ifdef FL_ARR_IN_LUT_ORDER
				const int fl_idx = i;
#else
				const int fl_idx = idx;
#endif

				s_info->hdr_exp.fl_lc[fl_idx] =
					ctx->subctx.frame_length_in_lut_rg[i];
			}
		}
	}


	/* sensor mode info */
	s_info->pclk = ctx->subctx.pclk;
	s_info->linelength = ctx->subctx.line_length;
	s_info->lineTimeInNs =
		CALC_LINE_TIME_IN_NS(s_info->pclk, s_info->linelength);


	/* callback info */
	s_info->func_ptr = cb_func_fsync_mgr_set_fl_info;
	s_info->p_ctx = ctx;
}

void notify_fsync_mgr_streaming(struct adaptor_ctx *ctx, unsigned int flag)
{
	struct fs_streaming_st s_info;
	unsigned int ret;

	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, flag:%u, return\n",
			ctx->idx, ctx->fsync_mgr, flag);
		return;
	}
	fsync_mgr_reset_fsync_related_info(ctx);

	/* setup structure data */
	fsync_mgr_setup_fs_streaming_st(ctx, &s_info);

	/* call frame-sync streaming ON/OFF */
	ret = ctx->fsync_mgr->fs_streaming(flag, &s_info);
	if (unlikely(ret != 0)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, call frame-sync streaming, flag:%u, ret:%u\n",
			ctx->idx, flag, ret);
	}


#ifndef REDUCE_FSYNC_CTRLS_DBG_LOG
	fsync_mgr_dump_fs_streaming_st(ctx, &s_info, flag, __func__);
#endif
}


/*******************************************************************************
 * frame ctrls
 ******************************************************************************/
void fsync_mgr_dump_fs_perframe_st(struct adaptor_ctx *ctx,
	const struct fs_perframe_st *pf_ctrl, const unsigned int mode_id,
	const char *caller)
{
	FSYNC_MGR_LOGD(ctx,
		"[%s] sof_cnt:%u, ID:%#x(sidx:%u), req_id:%d, (a:%u/m:%u(%u/%u)), fl(%u, %u/%u/%u/%u/%u), exp(%u, %u/%u/%u/%u/%u), rout_l:%u, margin:%u(r:%u), min_fl_lc:%u, flk:%u, lineT:%u(ns)(pclk:%llu/ll:%u), rout_T(us):%u(mode_id:%u), set_exp_with_fl(%u => %u/%u, %u/%u/%u/%u/%u / %u/%u/%u/%u/%u), cmd_id:%d\n",
		caller,
		ctx->sof_cnt,
		pf_ctrl->sensor_id,
		pf_ctrl->sensor_idx,
		pf_ctrl->req_id,
		pf_ctrl->hdr_exp.ae_exp_cnt,
		pf_ctrl->hdr_exp.mode_exp_cnt,
		pf_ctrl->hdr_exp.multi_exp_type,
		pf_ctrl->hdr_exp.exp_order,
		pf_ctrl->out_fl_lc,
		pf_ctrl->hdr_exp.fl_lc[0],
		pf_ctrl->hdr_exp.fl_lc[1],
		pf_ctrl->hdr_exp.fl_lc[2],
		pf_ctrl->hdr_exp.fl_lc[3],
		pf_ctrl->hdr_exp.fl_lc[4],
		pf_ctrl->shutter_lc,
		pf_ctrl->hdr_exp.exp_lc[0],
		pf_ctrl->hdr_exp.exp_lc[1],
		pf_ctrl->hdr_exp.exp_lc[2],
		pf_ctrl->hdr_exp.exp_lc[3],
		pf_ctrl->hdr_exp.exp_lc[4],
		pf_ctrl->hdr_exp.readout_len_lc,
		pf_ctrl->margin_lc,
		pf_ctrl->hdr_exp.read_margin_lc,
		pf_ctrl->min_fl_lc,
		pf_ctrl->flicker_en,
		pf_ctrl->lineTimeInNs,
		pf_ctrl->pclk,
		pf_ctrl->linelength,
		pf_ctrl->readout_time_us,
		mode_id,
		ctx->needs_fsync_assign_fl,
		ctx->fsync_out_fl,
		ctx->subctx.frame_length_rg,
		ctx->fsync_out_fl_arr[0],
		ctx->fsync_out_fl_arr[1],
		ctx->fsync_out_fl_arr[2],
		ctx->fsync_out_fl_arr[3],
		ctx->fsync_out_fl_arr[4],
		ctx->subctx.frame_length_in_lut_rg[0],
		ctx->subctx.frame_length_in_lut_rg[1],
		ctx->subctx.frame_length_in_lut_rg[2],
		ctx->subctx.frame_length_in_lut_rg[3],
		ctx->subctx.frame_length_in_lut_rg[4],
		pf_ctrl->cmd_id);
}

void fsync_mgr_dump_fs_seamless_st(struct adaptor_ctx *ctx,
	const struct fs_seamless_st *seamless_info, const unsigned int mode_id,
	const char *caller)
{
	FSYNC_MGR_LOGD(ctx,
		"[%s] sidx:%d, seamless switch prop:(type_id:%u, orig_readout_time_us:%u, hw_re_init_time_us:%u, prsh_length_lc:%u)\n",
		caller,
		ctx->idx,
		seamless_info->prop.type_id,
		seamless_info->prop.orig_readout_time_us,
		seamless_info->prop.hw_re_init_time_us,
		seamless_info->prop.prsh_length_lc);

	fsync_mgr_dump_fs_perframe_st(ctx,
		&seamless_info->seamless_pf_ctrl, mode_id, caller);
}

static void fsync_mgr_prepare_mode_related_info(struct adaptor_ctx *ctx,
	const unsigned int mode_id,
	unsigned int *p_mode_crop_height,
	unsigned int *p_mode_linetime_readout_ns,
	const char *caller)
{
	if (unlikely(mode_id >= MODE_MAXCNT)) {
		/* not expected case */
		*p_mode_crop_height = 0;
		*p_mode_linetime_readout_ns = 0;

		FSYNC_MGR_LOGI(ctx,
			"[%s] ERROR: sidx:%d, mode_id:%u >= MODE_MAXCNT:%u, auto set mode_crop_height:%u, mode_linetime_readout_ns:%u\n",
			caller,
			ctx->idx,
			mode_id,
			MODE_MAXCNT,
			*p_mode_crop_height,
			*p_mode_linetime_readout_ns);
	} else {
		*p_mode_crop_height = ctx->mode[mode_id].height;
		*p_mode_linetime_readout_ns =
			ctx->mode[mode_id].linetime_in_ns_readout;
	}
}

static void fsync_mgr_setup_basic_fs_perframe_st(struct adaptor_ctx *ctx,
	struct fs_perframe_st *pf_ctrl, const unsigned int mode_id)
{
	unsigned int mode_crop_height, mode_linetime_readout_ns;

	/* prepare sensor mode property/info */
	fsync_mgr_prepare_mode_related_info(ctx, mode_id,
		&mode_crop_height, &mode_linetime_readout_ns, __func__);

	memset(pf_ctrl, 0, sizeof(*pf_ctrl));

	pf_ctrl->req_id = ctx->req_id;

	pf_ctrl->sensor_id = (ctx->subdrv) ? (ctx->subdrv->id) : 0;
	pf_ctrl->sensor_idx = ctx->idx;

	pf_ctrl->min_fl_lc = ctx->subctx.min_frame_length;
	pf_ctrl->margin_lc = g_sensor_margin(ctx, mode_id);
	pf_ctrl->flicker_en = ctx->subctx.autoflicker_en;
	pf_ctrl->out_fl_lc = ctx->subctx.frame_length_rg; // sensor current fl_lc

	/* preventing issue (seamless switch not update ctx->cur_mode data) */
	pf_ctrl->pclk = ctx->subctx.pclk;
	pf_ctrl->linelength = ctx->subctx.line_length;
	pf_ctrl->lineTimeInNs =
		CALC_LINE_TIME_IN_NS(pf_ctrl->pclk, pf_ctrl->linelength);
	pf_ctrl->readout_time_us =
		(mode_crop_height * mode_linetime_readout_ns / 1000);
}

static inline void fsync_mgr_setup_seamless_property(struct adaptor_ctx *ctx,
	const u32 orig_readout_time_us,
	struct fs_seamless_st *seamless_info)
{
	/* !!! setup all seamless switch property that needed !!! */
	/* setup original mode readout time */
	seamless_info->prop.orig_readout_time_us = orig_readout_time_us;

	switch ( ctx->subctx.s_ctx.seamless_switch_type ) {
	case SEAMLESS_SWITCH_CUT_VB_INIT_SHUT :
		seamless_info->prop.type_id = FREC_SEAMLESS_SWITCH_CUT_VB_INIT_SHUT;
		break;
	case SEAMLESS_SWITCH_ORIG_VB_INIT_SHUT :
		seamless_info->prop.type_id = FREC_SEAMLESS_SWITCH_ORIG_VB_INIT_SHUT;
		break;
	case SEAMLESS_SWITCH_ORIG_VB_ORIG_IMG :
		seamless_info->prop.type_id = FREC_SEAMLESS_SWITCH_ORIG_VB_ORIG_IMG;
		break;
	default:
		FSYNC_MGR_LOG_INF(ctx,
			"ERROR: sidx:%d, get unknown type:%u, do nothing\n",
			ctx->idx, ctx->subctx.s_ctx.seamless_switch_type);
		break;
	}

	seamless_info->prop.hw_re_init_time_us = ctx->subctx.s_ctx.seamless_switch_hw_re_init_time_ns / 1000;
	seamless_info->prop.prsh_length_lc = ctx->subctx.s_ctx.seamless_switch_prsh_length_lc;
}

static void fsync_mgr_setup_cb_func_cmd_id(struct adaptor_ctx *ctx,
	struct fs_perframe_st *pf_ctrl)
{
	pf_ctrl->cmd_id = (ctx->needs_fsync_assign_fl)
		? (unsigned int)FSYNC_CTRL_FL_CMD_ID_EXP_WITH_FL
		: (unsigned int)FSYNC_CTRL_FL_CMD_ID_FL;
}

int chk_if_need_to_use_s_multi_exp_fl_by_fsync_mgr(struct adaptor_ctx *ctx,
	u64 *ae_exp_arr, u32 ae_exp_cnt)
{
	int en_fsync = 0;

	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		ctx->needs_fsync_assign_fl = 0;
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, return:%d\n",
			ctx->idx, ctx->fsync_mgr, ctx->needs_fsync_assign_fl);
		return ctx->needs_fsync_assign_fl;
	}
	/* check situation */
	if (unlikely(ctx->is_streaming == 0)) {
		ctx->needs_fsync_assign_fl = 0;
		return ctx->needs_fsync_assign_fl;
	}
	fsync_mgr_chk_wait_tsrec_hw_pre_latch_updated(ctx);

	en_fsync = (ctx->fsync_mgr->fs_is_set_sync(ctx->idx));
	if (en_fsync) {
		/* check if sensor driver lock i2c operation */
		if (unlikely(ctx->subctx.fast_mode_on)) {
			fsync_mgr_clear_output_info(ctx);
			FSYNC_MGR_LOGI(ctx,
				"NOTICE: sidx:%d, detect fast_mode_on:%u, return:0  [en_fsync:%#x, fsync(%d):(%u,%u/%u/%u/%u/%u)]\n",
				ctx->idx,
				ctx->subctx.fast_mode_on,
				en_fsync,
				ctx->needs_fsync_assign_fl,
				ctx->fsync_out_fl,
				ctx->fsync_out_fl_arr[0],
				ctx->fsync_out_fl_arr[1],
				ctx->fsync_out_fl_arr[2],
				ctx->fsync_out_fl_arr[3],
				ctx->fsync_out_fl_arr[4]);
			return 0;
		}

		fsync_mgr_chk_long_exposure(ctx, ae_exp_arr, ae_exp_cnt);
		if (atomic_read(&long_exp_mode_bits) != 0) {
			FSYNC_MGR_LOGI(ctx,
				"NOTICE: sidx:%d, detect enable fsync sensor in long exp mode, long_exp_mode_bits:%#x => CTRL flow for sensor drv, return:0\n",
				ctx->idx,
				atomic_read(&long_exp_mode_bits));
			return 0;
		}
	}

	/* get result from above situation check */
	ctx->needs_fsync_assign_fl = en_fsync;

	return ctx->needs_fsync_assign_fl;
}

void notify_fsync_mgr_update_tg(struct adaptor_ctx *ctx, const u64 val)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, val:%llu, return\n",
			ctx->idx, ctx->fsync_mgr, val);
		return;
	}

	ctx->fsync_mgr->fs_update_tg(ctx->idx, val + 1);
}

/* ISP7S new add */
void notify_fsync_mgr_update_target_tg(struct adaptor_ctx *ctx, const u64 val)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, val:%llu, return\n",
			ctx->idx, ctx->fsync_mgr, val);
		return;
	}

	ctx->fsync_mgr->fs_update_target_tg(ctx->idx, val);
}

void notify_fsync_mgr_set_sync(struct adaptor_ctx *ctx, const u64 en)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, en:%llu, return\n",
			ctx->idx, ctx->fsync_mgr, en);
		return;
	}

	ctx->fsync_mgr->fs_set_sync(ctx->idx, en);

	if (en == 0)
		fsync_mgr_reset_fsync_related_info(ctx);
}

void notify_fsync_mgr_set_async_master(struct adaptor_ctx *ctx, const u64 en)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, en:%llu, return\n",
			ctx->idx, ctx->fsync_mgr, en);
		return;
	}

	ctx->fsync_mgr->fs_sa_set_user_async_master(ctx->idx, en);
}

void notify_fsync_mgr_update_auto_flicker_mode(struct adaptor_ctx *ctx,
	const u64 en)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, en:%llu, return\n",
			ctx->idx, ctx->fsync_mgr, en);
		return;
	}

	ctx->fsync_mgr->fs_update_auto_flicker_mode(ctx->idx, en);
}

void notify_fsync_mgr_update_min_fl(struct adaptor_ctx *ctx)
{
	/* TODO: LB-MF should update to LUT's val */
	unsigned int fl_lc_arr[FS_HDR_MAX] = {0};

	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, min_fl_lc:%u (fl_lc:%u), return\n",
			ctx->idx, ctx->fsync_mgr, ctx->subctx.min_frame_length,
			ctx->subctx.frame_length_rg);
		return;
	}
	/* check if sensor driver lock i2c operation */
	if (unlikely(ctx->subctx.fast_mode_on)) {
		FSYNC_MGR_LOGD(ctx,
			"NOTICE: sidx:%d, detect fast_mode_on:%u, return\n",
			ctx->idx,
			ctx->subctx.fast_mode_on);
		return;
	}
	fsync_mgr_chk_wait_tsrec_hw_pre_latch_updated(ctx);

	ctx->fsync_mgr->fs_update_min_fl_lc(ctx->idx,
		ctx->subctx.min_frame_length,
		ctx->subctx.frame_length_rg, fl_lc_arr, FS_HDR_MAX);
}

void notify_fsync_mgr_set_extend_framelength(struct adaptor_ctx *ctx,
	const u64 ext_fl)
{
	unsigned int ext_fl_us = 0;

	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, extend fl:%llu(ns), return\n",
			ctx->idx, ctx->fsync_mgr, ext_fl);
		return;
	}
	/* check if sensor driver lock i2c operation */
	if (unlikely(ctx->subctx.fast_mode_on)) {
		FSYNC_MGR_LOGI(ctx,
			"NOTICE: sidx:%d, detect fast_mode_on:%u, return\n",
			ctx->idx,
			ctx->subctx.fast_mode_on);
		return;
	}

	/* ext_fl (input) is ns */
	ext_fl_us = ext_fl / 1000;

	/* args:(ident / ext_fl_lc / ext_fl_us) */
	ctx->fsync_mgr->fs_set_extend_framelength(ctx->idx, 0, ext_fl_us);
}

void notify_fsync_mgr_seamless_switch(struct adaptor_ctx *ctx,
	u64 *ae_exp_arr, u32 ae_exp_max_cnt,
	u32 orig_readout_time_us, u32 target_scenario_id)
{
	struct fs_seamless_st seamless_info = {0};

	fsync_mgr_clear_output_info(ctx);

	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, return\n",
			ctx->idx, ctx->fsync_mgr);
		return;
	}


	/* !!! start here !!! */
	/* setup basic structure, exp info */
	fsync_mgr_setup_basic_fs_perframe_st(ctx,
		&seamless_info.seamless_pf_ctrl, target_scenario_id);
	fsync_mgr_setup_exp_data(ctx,
		&seamless_info.seamless_pf_ctrl, target_scenario_id,
		ae_exp_arr, -1);

	/* then setup other fs_seamless_st info */
	fsync_mgr_setup_seamless_property(ctx,
		orig_readout_time_us, &seamless_info);


	/* call frame-sync fs seamless switch */
	ctx->fsync_mgr->fs_seamless_switch(ctx->idx,
		&seamless_info, ctx->sof_cnt);


// #ifndef REDUCE_FSYNC_CTRLS_DBG_LOG
	fsync_mgr_dump_fs_seamless_st(ctx,
		&seamless_info, target_scenario_id, __func__);
// #endif
}

void notify_fsync_mgr_n_1_en(struct adaptor_ctx *ctx, u64 n, u64 en)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, N(%llu):1, en:%llu, return\n",
			ctx->idx, ctx->fsync_mgr, n, en);
		return;
	}

	ctx->fsync_mgr->fs_n_1_en(ctx->idx, n, en);
}

void notify_fsync_mgr_mstream_en(struct adaptor_ctx *ctx, u64 en)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, en:%llu, return\n",
			ctx->idx, ctx->fsync_mgr, en);
		return;
	}

	ctx->fsync_mgr->fs_mstream_en(ctx->idx, en);
}

void notify_fsync_mgr_subsample_tag(struct adaptor_ctx *ctx, u64 sub_tag)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, sub_tag:%llu, return\n",
			ctx->idx, ctx->fsync_mgr, sub_tag);
		return;
	}
	if (unlikely(sub_tag < 1)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, sub_tag:%llu should larger than 1, return\n",
			ctx->idx, sub_tag);
		return;
	}

	ctx->fsync_mgr->fs_set_frame_tag(ctx->idx, sub_tag - 1);
}

void notify_fsync_mgr_set_shutter(struct adaptor_ctx *ctx,
	u64 *ae_exp_arr, u32 ae_exp_cnt)
{
	struct fs_perframe_st pf_ctrl;
	const unsigned int mode_id = ctx->subctx.current_scenario_id;

	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, return\n",
			ctx->idx, ctx->fsync_mgr);
		return;
	}
	/* check if sensor driver lock i2c operation */
	if (unlikely(ctx->subctx.fast_mode_on)) {
		FSYNC_MGR_LOGD(ctx,
			"NOTICE: sidx:%d, detect fast_mode_on:%u, return\n",
			ctx->idx,
			ctx->subctx.fast_mode_on);
		return;
	}
	if (unlikely(atomic_read(&long_exp_mode_bits) != 0)) {
		FSYNC_MGR_LOGI(ctx,
			"NOTICE: sidx:%d, detect enable sync sensor in long exp mode, long_exp_mode_bits:%#x => return [needs_fsync_assign_fl:%d]\n",
			ctx->idx,
			atomic_read(&long_exp_mode_bits),
			ctx->needs_fsync_assign_fl);
		return;
	}


	/* !!! start here !!! */
	/* setup basic structure, exp info */
	fsync_mgr_setup_basic_fs_perframe_st(ctx, &pf_ctrl, mode_id);
	fsync_mgr_setup_exp_data(ctx, &pf_ctrl, mode_id, ae_exp_arr, ae_exp_cnt);

	/* setup cmd id for call back function using */
	/* !!! MUST call this after setup exp data !!! */
	fsync_mgr_setup_cb_func_cmd_id(ctx, &pf_ctrl);

	/* call frame-sync fs set shutter */
	ctx->fsync_mgr->fs_set_shutter(&pf_ctrl);
	if (ctx->needs_fsync_assign_fl) {
		/* Enable frame-sync && using SW sync (SA algo) solution */
		/* set exp with fl (ctx->fsync_out_fl) */
		fsync_mgr_s_multi_shutter_frame_length(ctx,
			ae_exp_arr, ae_exp_cnt,
			pf_ctrl.hdr_exp.multi_exp_type);
	}
	/* update sensor current fl_lc */
	fsync_mgr_update_sensor_actual_fl_info(ctx, &pf_ctrl);
	/* update sensor current fl_lc to Frame-Sync */
	ctx->fsync_mgr->fs_update_shutter(&pf_ctrl);


// #ifndef REDUCE_FSYNC_CTRLS_DBG_LOG
	fsync_mgr_dump_fs_perframe_st(ctx, &pf_ctrl, mode_id, __func__);
// #endif
}


void notify_fsync_mgr_sync_frame(struct adaptor_ctx *ctx,
	const unsigned int flag)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, flag:%u, return\n",
			ctx->idx, ctx->fsync_mgr, flag);
		return;
	}

	// FSYNC_MGR_LOGD(ctx, "sidx:%d, flag:%u\n", ctx->idx, flag);

	ctx->fsync_mgr->fs_sync_frame(flag);
}


/*******************************************************************************
 * ext ctrls
 ******************************************************************************/
void notify_fsync_mgr_update_sof_cnt(struct adaptor_ctx *ctx, const u32 sof_cnt)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, get sof_cnt:%u, return\n",
			ctx->idx, ctx->fsync_mgr, sof_cnt);
		return;
	}

	ctx->fsync_mgr->fs_set_debug_info_sof_cnt(ctx->idx, sof_cnt);
}


void notify_fsync_mgr_vsync(struct adaptor_ctx *ctx)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, sof_cnt:%u, return\n",
			ctx->idx, ctx->fsync_mgr, ctx->sof_cnt);
		return;
	}

	ctx->fsync_mgr->fs_notify_vsync(ctx->idx);
}


void notify_fsync_mgr_vsync_by_tsrec(struct adaptor_ctx *ctx)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, return\n",
			ctx->idx, ctx->fsync_mgr);
		return;
	}

	ctx->fsync_mgr->fs_notify_vsync_by_tsrec(ctx->idx);
}


void notify_fsync_mgr_sensor_hw_pre_latch_by_tsrec(struct adaptor_ctx *ctx,
	const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, return\n",
			ctx->idx, ctx->fsync_mgr);
		return;
	}

	/* first, notify sensor hw pr-latch */
	ctx->fsync_mgr->fs_notify_sensor_hw_pre_latch_by_tsrec(ctx->idx);
	/* then, update TS data for check FL result */
	ctx->fsync_mgr->fs_receive_tsrec_timestamp_info(ctx->idx, ts_info);
	/* finally, update TS data for timing ckecker */
	fsync_mgr_update_tsrec_sen_hw_pre_latch_ts_info(ctx, ts_info);
}


void notify_fsync_mgr_receive_tsrec_timestamp_info(struct adaptor_ctx *ctx,
	const struct mtk_cam_seninf_tsrec_timestamp_info *ts_info)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, return\n",
			ctx->idx, ctx->fsync_mgr);
		return;
	}

	// ctx->fsync_mgr->fs_receive_tsrec_timestamp_info(ctx->idx, ts_info);
}


void notify_fsync_mgr_g_fl_record_info(struct adaptor_ctx *ctx,
	struct mtk_fs_frame_length_info *p_fl_info)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, return\n",
			ctx->idx, ctx->fsync_mgr);
		return;
	}

	ctx->fsync_mgr->fs_get_fl_record_info(ctx->idx,
		&p_fl_info->target_min_fl_us, &p_fl_info->out_fl_us);
}


void notify_fsync_mgr_clear_fl_restore_info_if_needed(struct adaptor_ctx *ctx)
{
	/* not expected case */
	if (unlikely(ctx->fsync_mgr == NULL)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr:%p is NULL, return\n",
			ctx->idx, ctx->fsync_mgr);
		return;
	}

	ctx->fsync_mgr->fs_clear_fl_restore_status_if_needed(ctx->idx);
}


/*******************************************************************************
 * init Frame-Sync Mgr / get all function calls
 ******************************************************************************/
int notify_fsync_mgr(struct adaptor_ctx *ctx, const int on)
{
	struct device_node *seninf_np;
	struct SensorInfo info = {0};
	const char *seninf_port = NULL;
	int ret, seninf_idx = 0;
	char c_ab;

	/* setup some sensor info st (w/o seninf idx) for fsync mgr using */
	info.sensor_id = (ctx->subdrv) ? (ctx->subdrv->id) : 0;
	info.sensor_idx = ctx->idx;
	info.dev = (void *)ctx->dev;

	/* check if it is imgsensor remove flow */
	if (!on) {
		if (unlikely(!ctx->fsync_mgr))
			return 0;

		/* imgsensor remove => free mem, remove sysfs file */
		ctx->fsync_mgr->fs_unregister_sensor(&info, REGISTER_METHOD);
		FrameSyncUnInit(ctx->dev);
		return 0;
	}

	/* at imgsensor probe flow */
	seninf_np = of_graph_get_remote_node(ctx->dev->of_node, 0, 0);
	if (unlikely(!seninf_np)) {
		FSYNC_MGR_LOGI(ctx, "no remote device node\n");
		return -EINVAL;
	}

	ret = of_property_read_string(seninf_np, "csi-port", &seninf_port);

	of_node_put(seninf_np);

	if (unlikely(ret || !seninf_port)) {
		FSYNC_MGR_LOGI(ctx, "no seninf csi-port\n");
		return -EINVAL;
	}

	/* convert seninf-port to seninf-idx */
	ret = sscanf(seninf_port, "%d%c", &seninf_idx, &c_ab);
	seninf_idx <<= 1;
	seninf_idx += (ret == 2 && (c_ab == 'b' || c_ab == 'B'));
	ctx->seninf_idx = seninf_idx;

	/* notify frame-sync mgr of sensor-idx and seninf-idx */
#if !defined(FORCE_DISABLE_FSYNC_MGR)
	/* frame-sync init */
	ret = FrameSyncInit(&ctx->fsync_mgr, ctx->dev);
	if (unlikely(ret != 0)) {
		FSYNC_MGR_LOGI(ctx,
			"ERROR: sidx:%d, ctx->fsync_mgr init failed!\n",
			ctx->idx);
		ctx->fsync_mgr = NULL;
	} else {
		FSYNC_MGR_LOGD(ctx,
			"sidx:%d, ctx->fsync_mgr:%p init done, ret:%d",
			ctx->idx, ctx->fsync_mgr, ret);

		fsync_mgr_init_tsrec_sen_hw_pre_latch_info(ctx);

		/* update seninf idx and call register sensor */
		info.seninf_idx = ctx->seninf_idx;
		ctx->fsync_mgr->fs_register_sensor(&info, REGISTER_METHOD);
	}
#else
	ctx->fsync_mgr = NULL;
	FSYNC_MGR_LOGI(ctx,
		"WARNING: sidx:%d, ctx->fsync_mgr:%p is NULL(set FORCE_DISABLE_FSYNC_MGR compile flag)\n",
		ctx->idx, ctx->fsync_mgr);
#endif

	FSYNC_MGR_LOGI(ctx,
		"sensor_idx:%d, seninf_port:%s, seninf_idx:%d, is_fsync_ts_src_type_tsrec:%u\n",
		ctx->idx, seninf_port, ctx->seninf_idx,
		is_fsync_ts_src_type_tsrec);

	return 0;
}
