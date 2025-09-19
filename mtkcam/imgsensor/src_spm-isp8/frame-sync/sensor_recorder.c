// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef FS_UT
#include <linux/delay.h>        /* for get ktime */
#endif

#include "sensor_recorder.h"
#include "frame_sync.h"
#include "frame_sync_log.h"
#include "frame_sync_def.h"
#include "frame_sync_trace.h"
/* TODO: check below files, after refactor some files should not include */
#include "frame_sync_algo.h"
#include "frame_sync_util.h"
#include "frame_monitor.h"


/******************************************************************************/
// frame recorder macro/define/enum
/******************************************************************************/
#define REDUCE_SEN_REC_LOG
#define PFX "SensorRecorder"
#define FS_LOG_DBG_DEF_CAT LOG_SEN_REC


#define RING_BACK(x, n) \
	(((x)+(RECORDER_DEPTH-((n)%RECORDER_DEPTH))) & (RECORDER_DEPTH-1))

#define RING_FORWARD(x, n) \
	(((x)+((n)%RECORDER_DEPTH)) & (RECORDER_DEPTH-1))


#if !defined(FS_UT)
#define frec_mutex_lock_init(p)        mutex_init(p)
#define frec_mutex_lock(p)             mutex_lock(p)
#define frec_mutex_unlock(p)           mutex_unlock(p)

#define frec_spin_lock_init(p)         spin_lock_init(p)
#define frec_spin_lock(p)              spin_lock(p)
#define frec_spin_unlock(p)            spin_unlock(p)
#else
#define frec_mutex_lock_init(p)
#define frec_mutex_lock(p)
#define frec_mutex_unlock(p)

#define frec_spin_lock_init(p)
#define frec_spin_lock(p)
#define frec_spin_unlock(p)
#endif


/******************************************************************************/
// frame recorder static structure
/******************************************************************************/
struct FrameRecorder {
	/* is_init: */
	/*     0 => records data need be setup to sensor initial value. */
	/*       => need setup default shutter and fl values */
	unsigned int is_init;

	unsigned int fl_act_delay; // (N+2) => 3; (N+1) => 2;
	unsigned int def_fl_lc;

	/* sensor record */
	FS_Atomic_T depth_idx;
	struct FrameRecord frame_recs[RECORDER_DEPTH];

#if !defined(FS_UT)
	struct mutex frame_recs_update_lock;
#endif

	/* !!! frame length related info !!! */
	/* predict frame length */
	unsigned int curr_predicted_fl_lc;
	unsigned int curr_predicted_fl_us;
	unsigned int prev_predicted_fl_lc;
	unsigned int prev_predicted_fl_us;
	/* predict read offset of each shutters */
	unsigned int next_predicted_rd_offset_lc[FS_HDR_MAX];
	unsigned int next_predicted_rd_offset_us[FS_HDR_MAX];
	unsigned int curr_predicted_rd_offset_lc[FS_HDR_MAX];
	unsigned int curr_predicted_rd_offset_us[FS_HDR_MAX];


	/* timestamp info */
	unsigned int tick_factor;
	SenRec_TS_T ts_exp_0[VSYNCS_MAX];
	/* actual frame length (by timestamp diff.) */
	unsigned int act_fl_us;


	/* info for debug */
	// unsigned int p1_sof_cnt;
	/* recorder push/update system timestamp for debugging */
	unsigned long long sys_ts_recs[RECORDER_DEPTH];
};
static struct FrameRecorder *frm_recorders[SENSOR_MAX_NUM];
/******************************************************************************/


/******************************************************************************/
// Frame Recorder function
/******************************************************************************/
static struct FrameRecorder *frec_g_recorder_ctx(const unsigned int idx,
	const char *caller)
{
	struct FrameRecorder *pfrec = frm_recorders[idx];

	if (unlikely(pfrec == NULL)) {
		LOG_MUST(
			"[%s] ERROR: [%u] ID:%#x(sidx:%u/inf:%u), recs[%u]:%p is nullptr, return\n",
			caller, idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			idx, pfrec);
	}

	return pfrec;
}


/*----------------------------------------------------------------------------*/
// tool function
/*----------------------------------------------------------------------------*/
static unsigned int divide_num(const unsigned int idx,
	const unsigned int n, const unsigned int base, const char *caller)
{
	unsigned int val = n;

	/* error handle */
	if (unlikely(base == 0)) {
		LOG_MUST(
			"[%s]: ERROR: [%u] ID:%#x(sidx:%u/inf:%u), divide by zero (plz chk input params), n:%u/base:%u, return:%u\n",
			caller, idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			n, base, n);
		return n;
	}

	do_div(val, base);

	return val;
}


/*----------------------------------------------------------------------------*/
// debug/dump function
/*----------------------------------------------------------------------------*/
void frec_dump_cascade_exp_fl_info(const unsigned int idx,
	const unsigned int *exp_cas_arr, const unsigned int *fl_cas_arr,
	const unsigned int arr_len, const char *caller)
{
	const unsigned int log_str_len = 512;
	unsigned int i;
	char *log_buf = NULL;
	int len = 0, ret;

	ret = alloc_log_buf(log_str_len, &log_buf);
	if (unlikely(ret != 0)) {
		LOG_MUST("ERROR: log_buf allocate memory failed\n");
		return;
	}

	FS_SNPRF(log_str_len, log_buf, len,
		"[%s]: [%u] ID:%#x(sidx:%u/inf:%u), cas_exp:(",
		caller,
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx));

	for (i = 0; i < arr_len; ++i) {
		FS_SNPRF(log_str_len, log_buf, len, "%u", exp_cas_arr[i]);
		if ((i + 1) < arr_len)
			FS_SNPRF(log_str_len, log_buf, len, "/");
	}

	FS_SNPRF(log_str_len, log_buf, len, "), cas_fl:(");

	for (i = 0; i < arr_len; ++i) {
		FS_SNPRF(log_str_len, log_buf, len, "%u", fl_cas_arr[i]);
		if ((i + 1) < arr_len)
			FS_SNPRF(log_str_len, log_buf, len, "/");
	}

	FS_SNPRF(log_str_len, log_buf, len, "), arr_len:%u", arr_len);

	LOG_MUST("%s\n", log_buf);
	FS_FREE(log_buf);
}


void frec_dump_predicted_fl_info_st(const unsigned int idx,
	const struct predicted_fl_info_st *fl_info,
	const char *caller)
{
	LOG_MUST(
		"[%s]: [%u] ID:%#x(sidx:%u/inf:%u), pr_fl(c:%u(%u)/n:%u(%u)/s:%u(%u)), r_offset(c:(%u(%u)/%u(%u)/%u(%u)/%u(%u)/%u(%u), n:(%u(%u)/%u(%u)/%u(%u)/%u(%u)/%u(%u)))\n",
		caller,
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx),
		fl_info->pr_curr_fl_us,
		fl_info->pr_curr_fl_lc,
		fl_info->pr_next_fl_us,
		fl_info->pr_next_fl_lc,
		fl_info->pr_stable_fl_us,
		fl_info->pr_stable_fl_lc,
		fl_info->curr_exp_rd_offset_us[0],
		fl_info->curr_exp_rd_offset_lc[0],
		fl_info->curr_exp_rd_offset_us[1],
		fl_info->curr_exp_rd_offset_lc[1],
		fl_info->curr_exp_rd_offset_us[2],
		fl_info->curr_exp_rd_offset_lc[2],
		fl_info->curr_exp_rd_offset_us[3],
		fl_info->curr_exp_rd_offset_lc[3],
		fl_info->curr_exp_rd_offset_us[4],
		fl_info->curr_exp_rd_offset_lc[4],
		fl_info->next_exp_rd_offset_us[0],
		fl_info->next_exp_rd_offset_lc[1],
		fl_info->next_exp_rd_offset_us[1],
		fl_info->next_exp_rd_offset_lc[1],
		fl_info->next_exp_rd_offset_us[2],
		fl_info->next_exp_rd_offset_lc[2],
		fl_info->next_exp_rd_offset_us[3],
		fl_info->next_exp_rd_offset_lc[3],
		fl_info->next_exp_rd_offset_us[4],
		fl_info->next_exp_rd_offset_lc[4]);
}


void frec_dump_frame_record_info(const struct FrameRecord *p_frame_rec,
	const char *caller)
{
	LOG_MUST(
		"[%s]: req_id:%d, (exp_lc:%u/fl_lc:%u), (a:%u/m:%u(%u,%u), exp:%u/%u/%u/%u/%u, fl:%u/%u/%u/%u/%u), margin_lc:(%u, read:%u), readout_len_lc:%u, pclk:%llu, line_length:%u\n",
		caller,
		p_frame_rec->mw_req_id,
		p_frame_rec->shutter_lc,
		p_frame_rec->framelength_lc,
		p_frame_rec->ae_exp_cnt,
		p_frame_rec->mode_exp_cnt,
		p_frame_rec->m_exp_type,
		p_frame_rec->exp_order,
		p_frame_rec->exp_lc_arr[0],
		p_frame_rec->exp_lc_arr[1],
		p_frame_rec->exp_lc_arr[2],
		p_frame_rec->exp_lc_arr[3],
		p_frame_rec->exp_lc_arr[4],
		p_frame_rec->fl_lc_arr[0],
		p_frame_rec->fl_lc_arr[1],
		p_frame_rec->fl_lc_arr[2],
		p_frame_rec->fl_lc_arr[3],
		p_frame_rec->fl_lc_arr[4],
		p_frame_rec->margin_lc,
		p_frame_rec->read_margin_lc,
		p_frame_rec->readout_len_lc,
		p_frame_rec->pclk,
		p_frame_rec->line_length);
}


void frec_dump_recorder(const unsigned int idx, const char *caller)
{
	const struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	const unsigned int log_str_len = LOG_BUF_STR_LEN;
	unsigned int act_fl_arr[RECORDER_DEPTH-1] = {0};
	unsigned int depth_idx;
	unsigned int i;
	char *log_buf = NULL;
	int len = 0, ret;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	depth_idx = FS_ATOMIC_READ(&pfrec->depth_idx);

	ret = alloc_log_buf(log_str_len, &log_buf);
	if (unlikely(ret != 0)) {
		LOG_MUST("ERROR: log_buf allocate memory failed\n");
		return;
	}

	FS_SNPRF(log_str_len, log_buf, len,
		"[%s]: [%u] ID:%#x(sidx:%u), fdelay:%u/def_fl:%u/lineT:%u/mar(%u,r:%u)/routL:%u",
		caller,
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		pfrec->fl_act_delay,
		pfrec->def_fl_lc,
		calcLineTimeInNs(
			pfrec->frame_recs[depth_idx].pclk,
			pfrec->frame_recs[depth_idx].line_length),
		pfrec->frame_recs[depth_idx].margin_lc,
		pfrec->frame_recs[depth_idx].read_margin_lc,
		pfrec->frame_recs[depth_idx].readout_len_lc);

	for (i = 0; i < RECORDER_DEPTH; ++i) {
		/* dump data from newest to old */
		/* newest => copy from previous, at this time sensor driver not get AE ctrl yet */
		const unsigned int idx = RING_BACK(depth_idx, i);

		FS_SNPRF(log_str_len, log_buf, len,
			", ([%u](%llu/req:%d):(%u/%u),(a:%u/m:%u(t:%u,o:%u),%u/%u/%u/%u/%u",
			idx,
			pfrec->sys_ts_recs[idx]/1000,
			pfrec->frame_recs[idx].mw_req_id,
			pfrec->frame_recs[idx].shutter_lc,
			pfrec->frame_recs[idx].framelength_lc,
			pfrec->frame_recs[idx].ae_exp_cnt,
			pfrec->frame_recs[idx].mode_exp_cnt,
			pfrec->frame_recs[idx].m_exp_type,
			pfrec->frame_recs[idx].exp_order,
			pfrec->frame_recs[idx].exp_lc_arr[0],
			pfrec->frame_recs[idx].exp_lc_arr[1],
			pfrec->frame_recs[idx].exp_lc_arr[2],
			pfrec->frame_recs[idx].exp_lc_arr[3],
			pfrec->frame_recs[idx].exp_lc_arr[4]);

		if (pfrec->frame_recs[idx].m_exp_type == MULTI_EXP_TYPE_LBMF) {
			FS_SNPRF(log_str_len, log_buf, len,
				",%u/%u/%u/%u/%u",
				pfrec->frame_recs[idx].fl_lc_arr[0],
				pfrec->frame_recs[idx].fl_lc_arr[1],
				pfrec->frame_recs[idx].fl_lc_arr[2],
				pfrec->frame_recs[idx].fl_lc_arr[3],
				pfrec->frame_recs[idx].fl_lc_arr[4]);
		}

		FS_SNPRF(log_str_len, log_buf, len, "))");
	}

	FS_SNPRF(log_str_len, log_buf, len,
		", r_offset(%u(%u)/%u(%u)/%u(%u)/%u(%u)/%u(%u))",
		pfrec->curr_predicted_rd_offset_us[0],
		pfrec->curr_predicted_rd_offset_lc[0],
		pfrec->curr_predicted_rd_offset_us[1],
		pfrec->curr_predicted_rd_offset_lc[1],
		pfrec->curr_predicted_rd_offset_us[2],
		pfrec->curr_predicted_rd_offset_lc[2],
		pfrec->curr_predicted_rd_offset_us[3],
		pfrec->curr_predicted_rd_offset_lc[3],
		pfrec->curr_predicted_rd_offset_us[4],
		pfrec->curr_predicted_rd_offset_lc[4]);

	FS_SNPRF(log_str_len, log_buf, len,
		", pr(p(%u(%u)/act:%u)/c(%u(%u))",
		pfrec->prev_predicted_fl_us,
		pfrec->prev_predicted_fl_lc,
		pfrec->act_fl_us,
		pfrec->curr_predicted_fl_us,
		pfrec->curr_predicted_fl_lc);

	for (i = 0; i < RECORDER_DEPTH-1; ++i) {
		SenRec_TS_T tick_a, tick_b;

		/* check if tick factor is valid */
		if (unlikely(pfrec->tick_factor == 0))
			break;

		/* update actual frame length by timestamp diff */
		tick_a = pfrec->ts_exp_0[i] * pfrec->tick_factor;
		tick_b = pfrec->ts_exp_0[i+1] * pfrec->tick_factor;

		act_fl_arr[i] = (tick_b != 0)
			? ((tick_a - tick_b) / (pfrec->tick_factor)) : 0;
	}

	if (frm_get_ts_src_type() != FS_TS_SRC_TSREC) {
		FS_SNPRF(log_str_len, log_buf, len,
#ifdef TS_TICK_64_BITS
			", ts(%u/%u/%u,%llu/%llu/%llu/%llu)",
#else
			", ts(%u/%u/%u,%u/%u/%u/%u)",
#endif
			act_fl_arr[0],
			act_fl_arr[1],
			act_fl_arr[2],
			pfrec->ts_exp_0[0],
			pfrec->ts_exp_0[1],
			pfrec->ts_exp_0[2],
			pfrec->ts_exp_0[3]);
	} else {
		FS_SNPRF(log_str_len, log_buf, len,
			", ts(%u/%u/%u)",
			act_fl_arr[0],
			act_fl_arr[1],
			act_fl_arr[2]);

		fs_util_tsrec_dynamic_msg_connector(idx,
			log_str_len, log_buf, len, __func__);
	}

	LOG_MUST_LOCK("%s\n", log_buf);
	FS_TRACE_PR_LOG_INF("%s", log_buf);

	FS_FREE(log_buf);
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// recorder dynamic memory allocate / free functions
/*----------------------------------------------------------------------------*/
static void frec_data_init(const unsigned int idx)
{
#if !defined(FS_UT)
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);

	if (unlikely(pfrec == NULL))
		return;

	frec_mutex_lock_init(&pfrec->frame_recs_update_lock);
#endif
}


void frec_alloc_mem_data(const unsigned int idx, void *dev)
{
	struct FrameRecorder *ptr = NULL;

	if (unlikely(frm_recorders[idx] != NULL)) {
		LOG_MUST(
			"NOTICE: [%u] ID:%#x(sidx:%u/inf:%u), mem already allocated(recs[%u]:%p), return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			idx, frm_recorders[idx]);
		return;
	}

	ptr = FS_DEV_ZALLOC(dev, sizeof(*ptr));
	if (unlikely(ptr == NULL)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), mem allocate failed(ptr:%p, recs[%u]:%p), return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			ptr, idx, frm_recorders[idx]);
		return;
	}

	frm_recorders[idx] = ptr;

	/* init allocated data */
	frec_data_init(idx);

	LOG_INF(
		"[%u] ID:%#x(sidx:%u/inf:%u), mem allocated (ptr:%p, recs[%u]:%p)\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx),
		ptr, idx, frm_recorders[idx]);
}


void frec_free_mem_data(const unsigned int idx, void *dev)
{
	if (unlikely(frm_recorders[idx] == NULL)) {
		LOG_MUST(
			"NOTICE: [%u] ID:%#x(sidx:%u/inf:%u), mem already null(recs[%u]:%p), return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			idx, frm_recorders[idx]);
		return;
	}

	FS_FREE(frm_recorders[idx]);
	frm_recorders[idx] = NULL;

	LOG_INF(
		"[%u] ID:%#x(sidx:%u/inf:%u), mem freed (recs[%u]:%p)\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx),
		idx, frm_recorders[idx]);
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// utilities functions
/*----------------------------------------------------------------------------*/
/*
 * Return:
 *      @0 => non valid
 *      @1 => valid
 */
static unsigned int frec_chk_fdelay_is_valid(const unsigned int idx,
	const unsigned int fdelay, const char *caller)
{
	/* error handle, check sensor fl_active_delay value */
	if (unlikely((fdelay < 2) || (fdelay > 3))) {
		LOG_MUST(
			"[%s]: ERROR: [%u] ID:%#x(sidx:%u/inf:%u), sensor driver's frame_time_delay_frame:%u is not valid (MUST be 2 or 3), plz check sensor driver for getting correct value\n",
			caller, idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			fdelay);
		return 0;
	}
	return 1;
}


static inline int chk_exp_order_valid(const unsigned int exp_order)
{
	return (likely(exp_order < EXP_ORDER_MAX)) ? 1 : 0;
}


static inline int chk_exp_cnt_valid(const unsigned int exp_cnt)
{
	return (likely(exp_cnt < (FS_HDR_MAX + 1))) ? 1 : 0;
}


static inline int chk_exp_no_valid(const unsigned int exp_no)
{
	return (likely(exp_no < FS_HDR_MAX)) ? 1 : 0;
}


static inline int g_exp_order_idx_mapping(const unsigned int idx,
	const unsigned int m_exp_order, const unsigned int m_exp_cnt,
	const unsigned int exp_no, const char *caller)
{
	/* error handling */
	if (unlikely((!chk_exp_order_valid(m_exp_order))
			|| (!chk_exp_cnt_valid(m_exp_cnt))
			|| (!chk_exp_no_valid(exp_no)))) {
		LOG_MUST(
			"[%s] ERROR: [%u] ID:%#x(sidx:%u/inf:%u), get invalid para, exp(order:%u/cnt:%u/no:%u) => return exp_idx:%d\n",
			caller,
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			m_exp_order,
			m_exp_cnt,
			exp_no,
			FS_HDR_NONE);

		return FS_HDR_NONE;
	}

	return (exp_order_idx_map[m_exp_order][m_exp_cnt][exp_no]);
}


static int frec_g_mode_last_exp_idx(const unsigned int idx,
	const struct FrameRecorder *pfrec, const unsigned int depth_idx)
{
	unsigned int m_exp_cnt, m_exp_order, last_exp_no;
	int last_exp_idx;

	m_exp_order = pfrec->frame_recs[depth_idx].exp_order;

	m_exp_cnt = pfrec->frame_recs[depth_idx].mode_exp_cnt;
	if (unlikely(m_exp_cnt == 0)) {
		m_exp_cnt = 1;
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), m_exp_cnt:0 => assign m_exp_cnt:%u\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			m_exp_cnt);
	}
	last_exp_no = (m_exp_cnt - 1);

	last_exp_idx = g_exp_order_idx_mapping(idx, m_exp_order, m_exp_cnt, last_exp_no, __func__);
	if (unlikely(last_exp_idx < 0)) {
		last_exp_idx = 0;
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), exp_order_idx_map[%u][%u][%u]:(< 0) => assign last_exp_idx:%d\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			m_exp_order,
			m_exp_cnt,
			last_exp_no,
			last_exp_idx);
	}

	return last_exp_idx;
}


void frec_setup_frame_rec_by_fs_streaming_st(struct FrameRecord *p_frame_rec,
	const struct fs_streaming_st *sensor_info)
{
	memset(p_frame_rec, 0, sizeof(*p_frame_rec));

	/* init/setup sensor frame recorder */
	p_frame_rec->shutter_lc = sensor_info->def_shutter_lc;
	p_frame_rec->framelength_lc = sensor_info->def_fl_lc;

	p_frame_rec->ae_exp_cnt = sensor_info->hdr_exp.ae_exp_cnt;
	memcpy(p_frame_rec->exp_lc_arr, sensor_info->hdr_exp.exp_lc,
		sizeof(unsigned int) * FS_HDR_MAX);
	memcpy(p_frame_rec->fl_lc_arr, sensor_info->hdr_exp.fl_lc,
		sizeof(unsigned int) * FS_HDR_MAX);

	p_frame_rec->margin_lc = sensor_info->margin_lc;
	p_frame_rec->read_margin_lc = sensor_info->hdr_exp.read_margin_lc;
	p_frame_rec->readout_len_lc = sensor_info->hdr_exp.readout_len_lc;
	p_frame_rec->mode_exp_cnt = sensor_info->hdr_exp.mode_exp_cnt;
	p_frame_rec->m_exp_type = sensor_info->hdr_exp.multi_exp_type;
	p_frame_rec->exp_order = sensor_info->hdr_exp.exp_order;

	p_frame_rec->pclk = sensor_info->pclk;
	p_frame_rec->line_length = sensor_info->linelength;

	// p_frame_rec->mw_req_id = // NOT has this info now.

	// frec_dump_frame_record_info(p_frame_rec, __func__);
}


void frec_setup_frame_rec_by_fs_perframe_st(struct FrameRecord *p_frame_rec,
	const struct fs_perframe_st *pf_ctrl)
{
	memset(p_frame_rec, 0, sizeof(*p_frame_rec));

	/* init/setup sensor frame recorder */
	p_frame_rec->shutter_lc = pf_ctrl->shutter_lc;
	p_frame_rec->framelength_lc = pf_ctrl->out_fl_lc;

	p_frame_rec->ae_exp_cnt = pf_ctrl->hdr_exp.ae_exp_cnt;
	memcpy(p_frame_rec->exp_lc_arr, pf_ctrl->hdr_exp.exp_lc,
		sizeof(unsigned int) * FS_HDR_MAX);
	memcpy(p_frame_rec->fl_lc_arr, pf_ctrl->hdr_exp.fl_lc,
		sizeof(unsigned int) * FS_HDR_MAX);

	p_frame_rec->margin_lc = pf_ctrl->margin_lc;
	p_frame_rec->read_margin_lc = pf_ctrl->hdr_exp.read_margin_lc;
	p_frame_rec->readout_len_lc = pf_ctrl->hdr_exp.readout_len_lc;
	p_frame_rec->mode_exp_cnt = pf_ctrl->hdr_exp.mode_exp_cnt;
	p_frame_rec->m_exp_type = pf_ctrl->hdr_exp.multi_exp_type;
	p_frame_rec->exp_order = pf_ctrl->hdr_exp.exp_order;

	p_frame_rec->pclk = pf_ctrl->pclk;
	p_frame_rec->line_length = pf_ctrl->linelength;

	p_frame_rec->mw_req_id = pf_ctrl->req_id;

	// frec_dump_frame_record_info(p_frame_rec, __func__);
}


void frec_setup_seamless_rec_by_fs_seamless_st(
	struct frec_seamless_st *p_seamless_rec,
	const struct fs_seamless_st *p_seamless_info)
{
	memset(p_seamless_rec, 0, sizeof(*p_seamless_rec));

	memcpy(&p_seamless_rec->prop, &p_seamless_info->prop,
		sizeof(p_seamless_rec->prop));

	/* setup frame record data */
	frec_setup_frame_rec_by_fs_perframe_st(
		&p_seamless_rec->frame_rec, &p_seamless_info->seamless_pf_ctrl);

	// frec_dump_frame_record_info(&p_seamless_rec->frame_rec, __func__);
}


/*----------------------------------------------------------------------------*/
// NDOL / LB-MF utilities functions
/*----------------------------------------------------------------------------*/
static void frec_get_cascade_exp_fl_settings(const unsigned int idx,
	const struct FrameRecord *recs_arr[], const unsigned int recs_len,
	unsigned int *exp_cas_arr, unsigned int *fl_cas_arr,
	const unsigned int arr_len)
{
	unsigned int total_exp_cnt = 0, accumulated_exp_cnt = 0;
	unsigned int i, j;

	memset(exp_cas_arr, 0, sizeof(unsigned int) * arr_len);
	memset(fl_cas_arr, 0, sizeof(unsigned int) * arr_len);

	/* check if the arr len for using is sufficient */
	for (i = 0; i < recs_len; ++i) {
		const struct FrameRecord *rec = recs_arr[i];

		total_exp_cnt += rec->mode_exp_cnt;
	}
	/* error case check */
	if (unlikely(arr_len < total_exp_cnt)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), cascade array size is too short (%u < total_mode_exp_cnt:%u), return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			arr_len,
			total_exp_cnt);
		return;
	}

	for (i = 0; i < recs_len; ++i) {
		const struct FrameRecord *rec = recs_arr[i];
		const unsigned int mode_exp_cnt = rec->mode_exp_cnt;
		const unsigned int order = rec->exp_order;

		/* copy each LUT CIT/FLL settings */
		for (j = 0; j < mode_exp_cnt; ++j) {
			const unsigned int ii = j + accumulated_exp_cnt;
			const int exp_idx =
				g_exp_order_idx_mapping(idx, order, mode_exp_cnt, j, __func__);

			if (unlikely(exp_idx < 0)) {
				LOG_MUST(
					"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), exp_order_idx_map[%u][%u][%u]:%d(< 0), wants to write to idx:%u\n",
					idx,
					fs_get_reg_sensor_id(idx),
					fs_get_reg_sensor_idx(idx),
					fs_get_reg_sensor_inf_idx(idx),
					order,
					mode_exp_cnt,
					j,
					exp_idx,
					ii);
				return;
			}

			exp_cas_arr[ii] = rec->exp_lc_arr[exp_idx];

#ifdef FL_ARR_IN_LUT_ORDER
			fl_cas_arr[ii] = rec->fl_lc_arr[j];
#else
			fl_cas_arr[ii] = rec->fl_lc_arr[exp_idx];
#endif
		}

		accumulated_exp_cnt += mode_exp_cnt;
	}
}


void frec_g_valid_min_fl_arr_val_for_lut(const unsigned int idx,
	const struct FrameRecord *curr_rec,
	const unsigned int target_fl_lc,
	unsigned int fl_lc_arr[], const unsigned int arr_len)
{
	const struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	const struct FrameRecord *recs[2] = {curr_rec, curr_rec};
	const unsigned int curr_mode_exp_cnt = curr_rec->mode_exp_cnt;
	const unsigned int based_min_fl_lc =
		curr_rec->readout_len_lc + curr_rec->read_margin_lc;
	unsigned int exp_cas[2*FS_HDR_MAX] = {0}, fl_cas[2*FS_HDR_MAX] = {0};
	unsigned int equiv_min_fl_lc = 0;
	unsigned int margin_lc_per_exp;
	unsigned int i;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	/* ONLY for LBMF that using LUT sensor */
	if (curr_mode_exp_cnt <= 1)
		return;
	/* chk sensor fdelay type, currently ONLY for FL N+2 / fdelay:3 */
	if (unlikely(pfrec->fl_act_delay != 3)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), NOT expected sensor type (fdelay:%u)\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			pfrec->fl_act_delay);
		return;
	}
	/* error handle */
	if (unlikely(fl_lc_arr == NULL)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), get fl_lc_arr:%p nullptr, return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			fl_lc_arr);
		return;
	}
	memset(fl_lc_arr, 0, sizeof(unsigned int) * arr_len);
	if (unlikely(curr_rec->m_exp_type != MULTI_EXP_TYPE_LBMF)) {
		LOG_MUST(
			"WARNING: [%u] ID:%#x(sidx:%u/inf:%u), sensor curr multi_exp_type:%u(STG:%u/LBMF:%u) is not for LBMF, return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			curr_rec->m_exp_type,
			MULTI_EXP_TYPE_STG, MULTI_EXP_TYPE_LBMF);
		return;
	}


	frec_get_cascade_exp_fl_settings(
		idx, recs, 2, exp_cas, fl_cas, (2*FS_HDR_MAX));

#ifndef REDUCE_SEN_REC_LOG
	frec_dump_cascade_exp_fl_info(
		idx, exp_cas_arr, fl_cas_arr, (2*FS_HDR_MAX), __func__);
#endif

	margin_lc_per_exp = divide_num(idx,
		curr_rec->margin_lc, curr_mode_exp_cnt, __func__);

	for (i = 0; i < curr_mode_exp_cnt; ++i) {
		const unsigned int ref_idx = i + 1;
		const unsigned int sm_lc = exp_cas[ref_idx] + margin_lc_per_exp;

		/* max(based and valid min fl, s+m) */
		fl_lc_arr[i] = (sm_lc > based_min_fl_lc)
			? sm_lc : based_min_fl_lc;

		/* keep total fps match to target fps */
		if (ref_idx == curr_mode_exp_cnt) {
			const unsigned int temp_min_fl_lc =
				(target_fl_lc > equiv_min_fl_lc)
				? (target_fl_lc - equiv_min_fl_lc) : 0;

			fl_lc_arr[i] = (temp_min_fl_lc > fl_lc_arr[i])
				? temp_min_fl_lc : fl_lc_arr[i];
		}

		equiv_min_fl_lc += fl_lc_arr[i];
	}

	LOG_INF(
		"[%u] ID:%#x(sidx:%u/inf:%u), fdelay:%u, target_fl_lc:%u, fl_lc_arr:(%u/%u/%u/%u/%u), (a:%u/m:%u(%u,%u), %u/%u/%u/%u/%u), margin(per_exp):%u, based:%u(rout_l:%u/r_m:%u)\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx),
		pfrec->fl_act_delay,
		target_fl_lc,
		fl_lc_arr[0],
		fl_lc_arr[1],
		fl_lc_arr[2],
		fl_lc_arr[3],
		fl_lc_arr[4],
		curr_rec->ae_exp_cnt, curr_rec->mode_exp_cnt,
		curr_rec->m_exp_type, curr_rec->exp_order,
		curr_rec->exp_lc_arr[0],
		curr_rec->exp_lc_arr[1],
		curr_rec->exp_lc_arr[2],
		curr_rec->exp_lc_arr[3],
		curr_rec->exp_lc_arr[4],
		margin_lc_per_exp,
		based_min_fl_lc,
		curr_rec->readout_len_lc, curr_rec->read_margin_lc);
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// NDOL / LB-MF functions
/*----------------------------------------------------------------------------*/
static void frec_calc_lbmf_read_offset_fdelay_3(const unsigned int idx,
	const struct FrameRecord *curr_rec,
	unsigned int *p_next_pr_rd_offset_lc,
	unsigned int *p_next_pr_rd_offset_us)
{
	const struct FrameRecord *recs[1] = {curr_rec};
	const unsigned int order = curr_rec->exp_order;
	const unsigned int curr_mode_exp_cnt = curr_rec->mode_exp_cnt;
	const unsigned int margin_lc_per_exp = divide_num(idx,
		curr_rec->margin_lc, curr_mode_exp_cnt, __func__);
	const unsigned int based_min_fl_lc =
		curr_rec->readout_len_lc + curr_rec->read_margin_lc;
	unsigned int exp_cas[FS_HDR_MAX] = {0}, fl_cas[FS_HDR_MAX] = {0};
	unsigned int bias = 0;
	unsigned int i;

	memset(p_next_pr_rd_offset_lc, 0, sizeof(unsigned int) * FS_HDR_MAX);
	memset(p_next_pr_rd_offset_us, 0, sizeof(unsigned int) * FS_HDR_MAX);

	frec_get_cascade_exp_fl_settings(
		idx, recs, 1, exp_cas, fl_cas, (FS_HDR_MAX));

	for (i = 0; i < (curr_mode_exp_cnt - 1); ++i) {
		const unsigned int ref_idx = i + 1;
		const int exp_idx =
			g_exp_order_idx_mapping(idx, order, curr_mode_exp_cnt, ref_idx, __func__);
		const unsigned int sm_lc = exp_cas[ref_idx] + margin_lc_per_exp;
		unsigned int min_fl_lc = based_min_fl_lc;

		if (unlikely(exp_idx < 0)) {
			LOG_MUST(
				"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), exp_order_idx_map[%u][%u][%u]:%d(< 0)\n",
				idx,
				fs_get_reg_sensor_id(idx),
				fs_get_reg_sensor_idx(idx),
				fs_get_reg_sensor_inf_idx(idx),
				order,
				curr_mode_exp_cnt,
				ref_idx,
				exp_idx);
			return;
		}

		/* max(based and valid min fl, s+m) */
		min_fl_lc = (sm_lc > min_fl_lc) ? sm_lc : min_fl_lc;
		min_fl_lc = (fl_cas[i] > min_fl_lc) ? fl_cas[i] : min_fl_lc;

		bias += min_fl_lc;
		p_next_pr_rd_offset_lc[exp_idx] = bias;
		p_next_pr_rd_offset_us[exp_idx] =
			convert2TotalTime(
				calcLineTimeInNs(
					curr_rec->pclk, curr_rec->line_length),
				p_next_pr_rd_offset_lc[exp_idx]);

#ifndef REDUCE_SEN_REC_LOG
		LOG_MUST(
			"[%u] ID:%#x(sidx:%u/inf:%u), i:%u(ref_idx:%u)/cnt:%u/exp_idx:%d, (s:%u/m:%u/b:%u, fl:%u) => r_offset:%u(%u)\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			i, ref_idx, curr_mode_exp_cnt, exp_idx,
			exp_cas[ref_idx],
			margin_lc_per_exp,
			based_min_fl_lc,
			fl_cas[i],
			p_next_pr_rd_offset_us[exp_idx],
			p_next_pr_rd_offset_lc[exp_idx]);
#endif
	}
}


static unsigned int frec_calc_lbmf_valid_min_fl_lc_for_shutters_fdelay_3(
	const unsigned int idx,
	const struct FrameRecord *curr_rec, const struct FrameRecord *prev_rec,
	const unsigned int target_min_fl_lc)
{
#ifndef REDUCE_SEN_REC_LOG
	const unsigned int log_str_len = 512;
	char *log_buf = NULL;
	int len = 0, ret;
#endif
	const struct FrameRecord *recs[2] = {prev_rec, curr_rec};
	const unsigned int prev_mode_exp_cnt = prev_rec->mode_exp_cnt;
	const unsigned int margin_lc_per_exp = divide_num(idx,
		prev_rec->margin_lc, prev_mode_exp_cnt, __func__);
	const unsigned int based_min_fl_lc =
		prev_rec->readout_len_lc + prev_rec->read_margin_lc;
	unsigned int exp_cas[2*FS_HDR_MAX] = {0}, fl_cas[2*FS_HDR_MAX] = {0};
	unsigned int equiv_min_fl_lc = 0;
	unsigned int i;

#ifndef REDUCE_SEN_REC_LOG
	/* prepare for log print */
	ret = alloc_log_buf(log_str_len, &log_buf);
	if (unlikely(ret != 0)) {
		LOG_MUST("ERROR: log_buf allocate memory failed\n");
		return 0;
	}

	FS_SNPRF(log_str_len, log_buf, len,
		"[%u] ID:%#x(sidx:%u/inf:%u)",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx));
#endif

	frec_get_cascade_exp_fl_settings(
		idx, recs, 2, exp_cas, fl_cas, (2*FS_HDR_MAX));

	/* N+2 type, e.g., general SONY sensor */
	/* check each CIT/FLL settings in LUT */
	for (i = 0; i < prev_mode_exp_cnt; ++i) {
		const unsigned int ref_idx = i + 1;
		const unsigned int sm_lc = exp_cas[ref_idx] + margin_lc_per_exp;
		const unsigned int total_min_fl_lc = (target_min_fl_lc > 0)
			? target_min_fl_lc : prev_rec->framelength_lc;
		unsigned int min_fl_lc = based_min_fl_lc;

		/* max(based and valid min fl, s+m) */
		min_fl_lc = (sm_lc > min_fl_lc) ? sm_lc : min_fl_lc;
		min_fl_lc = (fl_cas[i] > min_fl_lc) ? fl_cas[i] : min_fl_lc;

		/* keep total fps match to max fps */
		if (ref_idx == prev_mode_exp_cnt) {
			const unsigned int temp_min_fl_lc =
				(total_min_fl_lc > equiv_min_fl_lc)
				? (total_min_fl_lc - equiv_min_fl_lc) : 0;

			min_fl_lc = (temp_min_fl_lc > min_fl_lc)
				? temp_min_fl_lc : min_fl_lc;
		}

		equiv_min_fl_lc += min_fl_lc;

#ifndef REDUCE_SEN_REC_LOG
		FS_SNPRF(log_str_len, log_buf, len,
			", i:%u(ref_idx:%u)/cnt:%u, equiv_min_fl_lc:%u(min_fl_lc:%u(s+m:%u(%u/%u), based:%u(rout_l:%u/r_m:%u), fl:%u, total_min:%u(prev:%u/target:%u)))",
			i, ref_idx, prev_mode_exp_cnt,
			equiv_min_fl_lc,
			min_fl_lc,
			sm_lc, exp_cas[i+1], margin_lc_per_exp,
			based_min_fl_lc,
			prev_rec->readout_len_lc, prev_rec->read_margin_lc,
			fl_cas[i],
			total_min_fl_lc, prev_rec->framelength_lc, target_min_fl_lc);
#endif
	}

#ifndef REDUCE_SEN_REC_LOG
	LOG_INF("%s\n", log_buf);
	FS_FREE(log_buf);
#endif

	return equiv_min_fl_lc;
}


static void frec_calc_lbmf_read_offset(const unsigned int idx,
	const struct FrameRecord *curr_rec,
	unsigned int *p_next_pr_rd_offset_lc,
	unsigned int *p_next_pr_rd_offset_us)
{
	const struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;
	if (unlikely(!frec_chk_fdelay_is_valid(
			idx, pfrec->fl_act_delay, __func__)))
		return;

	switch (pfrec->fl_act_delay) {
	case 3:
		/* N+2 type, e.g., general SONY sensor */
		frec_calc_lbmf_read_offset_fdelay_3(idx, curr_rec,
			p_next_pr_rd_offset_lc, p_next_pr_rd_offset_us);
		break;
	case 2:
		/* N+1 type, e.g., non-SONY sensor */
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), fl_act_delay:%u, run in unexpected flow, plz chk, return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			pfrec->fl_act_delay);
		break;
	}
}


static unsigned int frec_calc_lbmf_valid_min_fl_lc_for_shutters(
	const unsigned int idx,
	const struct FrameRecord *curr_rec, const struct FrameRecord *prev_rec,
	const unsigned int target_min_fl_lc)
{
	const struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	unsigned int equiv_min_fl_lc = 0;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return curr_rec->framelength_lc;
	if (unlikely(!frec_chk_fdelay_is_valid(
			idx, pfrec->fl_act_delay, __func__)))
		return curr_rec->framelength_lc;

	switch (pfrec->fl_act_delay) {
	case 3:
		/* N+2 type, e.g., general SONY sensor */
		equiv_min_fl_lc =
			frec_calc_lbmf_valid_min_fl_lc_for_shutters_fdelay_3(
				idx, curr_rec, prev_rec, target_min_fl_lc);
		break;
	case 2:
		/* N+1 type, e.g., non-SONY sensor */
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), fl_act_delay:%u, run in unexpected flow, plz chk, return fl:%u\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			pfrec->fl_act_delay,
			curr_rec->framelength_lc);
		equiv_min_fl_lc = curr_rec->framelength_lc;
		break;
	}

	return equiv_min_fl_lc;
}


/*----------------------------------------------------------------------------*/
// !!! END !!! ---  NDOL / LB-MF functions
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// FDOL / Stagger functions
/*----------------------------------------------------------------------------*/
static void frec_calc_stg_read_offset(const unsigned int idx,
	const struct FrameRecord *curr_rec,
	unsigned int *p_next_pr_rd_offset_lc,
	unsigned int *p_next_pr_rd_offset_us)
{
	const unsigned int curr_mode_exp_cnt = curr_rec->mode_exp_cnt;
	const unsigned int margin_lc_per_exp = (curr_mode_exp_cnt != 0)
		? (curr_rec->margin_lc / curr_mode_exp_cnt)
		: curr_rec->margin_lc;
	unsigned int bias = 0;
	unsigned int i;

	memset(p_next_pr_rd_offset_lc, 0, sizeof(unsigned int) * FS_HDR_MAX);
	memset(p_next_pr_rd_offset_us, 0, sizeof(unsigned int) * FS_HDR_MAX);

	for (i = 1; i < curr_mode_exp_cnt; ++i) {
		int hdr_idx = hdr_exp_idx_map[curr_mode_exp_cnt][i];

		if (unlikely(hdr_idx < 0)) {
			LOG_MUST(
				"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), hdr_exp_idx_map[%u][%u] = %d\n",
				idx,
				fs_get_reg_sensor_id(idx),
				fs_get_reg_sensor_idx(idx),
				fs_get_reg_sensor_inf_idx(idx),
				curr_mode_exp_cnt,
				i,
				hdr_idx);
			return;
		}

		bias += margin_lc_per_exp + curr_rec->exp_lc_arr[hdr_idx];
		p_next_pr_rd_offset_lc[hdr_idx] = bias;
		p_next_pr_rd_offset_us[hdr_idx] =
			convert2TotalTime(
				calcLineTimeInNs(
					curr_rec->pclk, curr_rec->line_length),
				p_next_pr_rd_offset_lc[hdr_idx]);

#ifndef REDUCE_SEN_REC_LOG
		LOG_MUST(
			"[%u] ID:%#x(sidx:%u/inf:%u), i:%u/cnt:%u/hdr_idx:%d, exp_lc:%u, margin_per_exp:%u => r_offset:%u(%u)\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			i, curr_mode_exp_cnt, hdr_idx,
			curr_rec->exp_lc_arr[hdr_idx],
			margin_lc_per_exp,
			p_next_pr_rd_offset_us[hdr_idx],
			p_next_pr_rd_offset_lc[hdr_idx]);
#endif
	}

#ifndef REDUCE_SEN_REC_LOG
	LOG_MUST(
		"[%u] ID:%#x(sidx:%u/inf:%u), mode_exp_cnt:%u, r_offset(%u(%u)/%u(%u)/%u(%u)/%u(%u))\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx),
		curr_mode_exp_cnt,
		p_next_pr_rd_offset_us[1],
		p_next_pr_rd_offset_lc[1],
		p_next_pr_rd_offset_us[2],
		p_next_pr_rd_offset_lc[2],
		p_next_pr_rd_offset_us[3],
		p_next_pr_rd_offset_lc[3],
		p_next_pr_rd_offset_us[4],
		p_next_pr_rd_offset_lc[4]);
#endif
}


static unsigned int frec_chk_stg_fl_rule_1(const unsigned int idx,
	const struct FrameRecord *curr_rec, const struct FrameRecord *prev_rec)
{
	const unsigned int prev_mode_exp_cnt = prev_rec->mode_exp_cnt;
	unsigned int shutter_margin_lc;
	unsigned int i;

	/* check for auto-extended rule */
	shutter_margin_lc = curr_rec->exp_lc_arr[0];
	for (i = 1; i < prev_mode_exp_cnt; ++i) {
		int hdr_idx = hdr_exp_idx_map[prev_mode_exp_cnt][i];

		if (unlikely(hdr_idx < 0)) {
			LOG_MUST(
				"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), hdr_exp_idx_map[%u][%u] = %d\n",
				idx,
				fs_get_reg_sensor_id(idx),
				fs_get_reg_sensor_idx(idx),
				fs_get_reg_sensor_inf_idx(idx),
				prev_mode_exp_cnt,
				i,
				hdr_idx);
			return 0;
		}

		shutter_margin_lc += prev_rec->exp_lc_arr[hdr_idx];
	}
	shutter_margin_lc += prev_rec->margin_lc;

	return shutter_margin_lc;
}


static unsigned int frec_chk_stg_fl_rule_2(const unsigned int idx,
	const struct FrameRecord *curr_rec, const struct FrameRecord *prev_rec)
{
	const unsigned int curr_mode_exp_cnt = curr_rec->mode_exp_cnt;
	const unsigned int readout_len_lc = curr_rec->readout_len_lc;
	const unsigned int read_margin_lc = curr_rec->read_margin_lc;
	unsigned int readout_fl_lc = 0, readout_min_fl_lc = 0;
	unsigned int i;
	int read_offset_diff = 0;

	/* error case highlight */
	if (unlikely((readout_len_lc == 0) || (read_margin_lc == 0))) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), sensor driver's readout_length:%u, read_margin:%u is/are not valid, plz check sensor driver for getting correct value\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			readout_len_lc,
			read_margin_lc);
		return 0;
	}

	/* check trigger auto-extended for preventing readout overlap */
	for (i = 1; i < curr_mode_exp_cnt; ++i) {
		int hdr_idx = hdr_exp_idx_map[curr_mode_exp_cnt][i];

		if (unlikely(hdr_idx < 0)) {
			LOG_MUST(
				"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), hdr_exp_idx_map[%u][%u] = %d\n",
				idx,
				fs_get_reg_sensor_id(idx),
				fs_get_reg_sensor_idx(idx),
				fs_get_reg_sensor_inf_idx(idx),
				curr_mode_exp_cnt,
				i,
				hdr_idx);
			return 0;
		}

		read_offset_diff +=
			prev_rec->exp_lc_arr[hdr_idx] -
			curr_rec->exp_lc_arr[hdr_idx];

		readout_fl_lc = (read_offset_diff > 0)
			? (readout_len_lc + read_margin_lc + read_offset_diff)
			: (readout_len_lc + read_margin_lc);

		if (readout_min_fl_lc < readout_fl_lc)
			readout_min_fl_lc = readout_fl_lc;
	}

	return readout_min_fl_lc;
}


static unsigned int frec_chk_stg_fl_sw_rule_1(const unsigned int idx,
	const struct FrameRecord *curr_rec)
{
	const unsigned int curr_mode_exp_cnt = curr_rec->mode_exp_cnt;
	unsigned int shutter_margin_lc = 0;
	unsigned int i;

	/* check for preventing w/o auto-extended sensor exp not enough */
	for (i = 0; i < curr_mode_exp_cnt; ++i) {
		int hdr_idx = hdr_exp_idx_map[curr_mode_exp_cnt][i];

		if (unlikely(hdr_idx < 0)) {
			LOG_MUST(
				"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), hdr_exp_idx_map[%u][%u] = %d\n",
				idx,
				fs_get_reg_sensor_id(idx),
				fs_get_reg_sensor_idx(idx),
				fs_get_reg_sensor_inf_idx(idx),
				curr_mode_exp_cnt,
				i,
				hdr_idx);
			return 0;
		}

		shutter_margin_lc += curr_rec->exp_lc_arr[hdr_idx];
	}
	shutter_margin_lc += curr_rec->margin_lc;

	return shutter_margin_lc;
}


static unsigned int frec_calc_stg_valid_min_fl_lc_for_shutters(
	const unsigned int idx,
	const struct FrameRecord *curr_rec, const struct FrameRecord *prev_rec)
{
	unsigned int result_1, result_2;
	unsigned int min_fl_lc = 0;

	/* ONLY when stagger/HDR mode ===> mode exp cnt > 1 */
	if (curr_rec->mode_exp_cnt <= 1)
		return 0;

	/* only take HW needed min frame length for shutters into account */
	result_1 = frec_chk_stg_fl_rule_1(idx, curr_rec, prev_rec);
	result_2 = frec_chk_stg_fl_rule_2(idx, curr_rec, prev_rec);

	min_fl_lc = (min_fl_lc > result_1) ? min_fl_lc : result_1;
	min_fl_lc = (min_fl_lc > result_2) ? min_fl_lc : result_2;

	return min_fl_lc;
}
/*----------------------------------------------------------------------------*/
// !!! END !!! ---  FDOL / Stagger functions
/*----------------------------------------------------------------------------*/


static unsigned int frec_calc_valid_min_fl_lc_for_shutters(
	const unsigned int idx, const unsigned int fdelay,
	const struct FrameRecord *curr_rec, const struct FrameRecord *prev_rec,
	const unsigned int target_min_fl_lc)
{
	const unsigned int m_exp_type = curr_rec->m_exp_type;
	unsigned int min_fl_lc = 0;

	/* multi-exp / HDR sensor => mode exp cnt > 1 */
	if (curr_rec->mode_exp_cnt > 1) {
		switch (m_exp_type) {
		case MULTI_EXP_TYPE_LBMF:
			min_fl_lc = frec_calc_lbmf_valid_min_fl_lc_for_shutters(
				idx, curr_rec, prev_rec, target_min_fl_lc);
			break;
		case MULTI_EXP_TYPE_STG:
		default:
		{
			/* ONLY when stagger/HDR ===> mode exp cnt > 1 */
			min_fl_lc = frec_calc_stg_valid_min_fl_lc_for_shutters(
				idx, curr_rec, prev_rec);

			/* check N+1 type for protection not per-frame ctrl */
			if (fdelay == 2) {
				const unsigned int temp_min_fl_lc =
					frec_chk_stg_fl_sw_rule_1(idx, curr_rec);

				min_fl_lc = (min_fl_lc > temp_min_fl_lc)
					? min_fl_lc : temp_min_fl_lc;
			}
		}
			break;
		}
	} else {
		/* 1-exp / normal sensor => mode exp cnt = 1 */
		min_fl_lc = curr_rec->shutter_lc + curr_rec->margin_lc;
	}

	return min_fl_lc;
}


static void frec_predict_shutters_read_offset_by_curr_rec(const unsigned int idx,
	const struct FrameRecord *curr_rec,
	unsigned int *p_next_pr_rd_offset_lc,
	unsigned int *p_next_pr_rd_offset_us)
{
	const unsigned int m_exp_type = curr_rec->m_exp_type;
	const unsigned int curr_mode_exp_cnt = curr_rec->mode_exp_cnt;

	/* not multi-exp sensor mode */
	if (curr_mode_exp_cnt <= 1) {
		memset(p_next_pr_rd_offset_lc, 0,
			sizeof(unsigned int) * FS_HDR_MAX);
		memset(p_next_pr_rd_offset_us, 0,
			sizeof(unsigned int) * FS_HDR_MAX);
		return;
	}

	switch (m_exp_type) {
	case MULTI_EXP_TYPE_LBMF:
		frec_calc_lbmf_read_offset(idx, curr_rec,
			p_next_pr_rd_offset_lc,
			p_next_pr_rd_offset_us);
		break;
	case MULTI_EXP_TYPE_STG:
	default:
		frec_calc_stg_read_offset(idx, curr_rec,
			p_next_pr_rd_offset_lc,
			p_next_pr_rd_offset_us);
		break;
	}
}


static unsigned int frec_predict_fl_lc_by_curr_rec(const unsigned int idx,
	const struct FrameRecord *curr_rec, const enum predicted_fl_label label)
{
	const struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	const struct FrameRecord *prev_rec = NULL;
	unsigned int min_fl_lc, next_fl_lc;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return curr_rec->framelength_lc;
	if (unlikely(!frec_chk_fdelay_is_valid(
			idx, pfrec->fl_act_delay, __func__)))
		return curr_rec->framelength_lc;

	/* check case get corresponding frame record pointer */
	switch (label) {
	case PREDICT_NEXT_FL:
	{
		const unsigned int curr_idx = FS_ATOMIC_READ(&pfrec->depth_idx);
		const unsigned int target_idx = RING_BACK(curr_idx, 1);

		prev_rec = &pfrec->frame_recs[target_idx];
	}
		break;
	case PREDICT_STABLE_FL:
	default:
		prev_rec = curr_rec;
		break;
	}

	min_fl_lc = frec_calc_valid_min_fl_lc_for_shutters(
		idx, pfrec->fl_act_delay, curr_rec, prev_rec, 0);

	switch (pfrec->fl_act_delay) {
	case 3:
		/* N+2 type, e.g., general SONY sensor */
		next_fl_lc = (prev_rec->framelength_lc > min_fl_lc)
			? prev_rec->framelength_lc : min_fl_lc;
		break;
	case 2:
		/* N+1 type, e.g., non-SONY sensor */
		next_fl_lc = (min_fl_lc > curr_rec->framelength_lc)
			? min_fl_lc : curr_rec->framelength_lc;
		break;
	}

	return next_fl_lc;
}


void frec_get_predicted_frame_length_info(const unsigned int idx,
	const struct FrameRecord *curr_rec,
	struct predicted_fl_info_st *fl_info,
	const char *caller)
{
	const struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;
	/* case handle */
	if (unlikely((curr_rec == NULL) || (fl_info == NULL))) {
		LOG_MUST(
			"[%s]: ERROR: [%u] ID:%#x(sidx:%u/inf:%u), get nullptr of curr_rec:%p or fl_info:%p, return\n",
			caller, idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			curr_rec, fl_info);
		return;
	}

	/* !!! copy already calculated info !!! */
	/* copy current predicted frame length info */
	fl_info->pr_curr_fl_lc = pfrec->curr_predicted_fl_lc;
	fl_info->pr_curr_fl_us = pfrec->curr_predicted_fl_us;

	/* copy current read offset info and calculate next read offset info */
	memcpy(fl_info->curr_exp_rd_offset_us,
		pfrec->curr_predicted_rd_offset_us,
		sizeof(unsigned int) * FS_HDR_MAX);


	/* !!! calculate new info by current sensor frame record !!! */
	/* calculate next predicted frame length info */
	fl_info->pr_next_fl_lc =
		frec_predict_fl_lc_by_curr_rec(
			idx, curr_rec, PREDICT_NEXT_FL);
	fl_info->pr_next_fl_us =
		convert2TotalTime(
			calcLineTimeInNs(
				curr_rec->pclk, curr_rec->line_length),
			fl_info->pr_next_fl_lc);

	/* calculate stable predicted frame length info */
	fl_info->pr_stable_fl_lc = frec_predict_fl_lc_by_curr_rec(
		idx, curr_rec, PREDICT_STABLE_FL);
	fl_info->pr_stable_fl_us =
		convert2TotalTime(
			calcLineTimeInNs(
				curr_rec->pclk, curr_rec->line_length),
			fl_info->pr_stable_fl_lc);

	/* calculate next read offset info */
	frec_predict_shutters_read_offset_by_curr_rec(idx, curr_rec,
		fl_info->next_exp_rd_offset_lc,
		fl_info->next_exp_rd_offset_us);

#if defined(TRACE_FS_FREC_LOG)
	frec_dump_predicted_fl_info_st(idx, fl_info, caller);
#endif
}


unsigned int frec_g_valid_min_fl_lc_for_shutters_by_frame_rec(
	const unsigned int idx, const struct FrameRecord *curr_rec,
	const unsigned int min_fl_lc, const enum predicted_fl_label label)
{
	const struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	const struct FrameRecord *frame_rec = NULL;
	unsigned int s_m_min_lc = 0;
	unsigned int result;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return min_fl_lc;
	if (unlikely(!frec_chk_fdelay_is_valid(
			idx, pfrec->fl_act_delay, __func__)))
		return min_fl_lc;

	switch (label) {
	case PREDICT_NEXT_FL:
	{
		const unsigned int curr_idx = FS_ATOMIC_READ(&pfrec->depth_idx);
		const unsigned int target_idx = RING_BACK(curr_idx, 1);

		frame_rec = &pfrec->frame_recs[target_idx];
	}
		break;
	case PREDICT_STABLE_FL:
	default:
		frame_rec = curr_rec;
		break;
	}

	/* check min frame length that shutter needed */
	s_m_min_lc = frec_calc_valid_min_fl_lc_for_shutters(
		idx, pfrec->fl_act_delay, curr_rec, frame_rec, min_fl_lc);

	result = (s_m_min_lc > min_fl_lc) ? s_m_min_lc : min_fl_lc;

	LOG_INF(
		"[%u] ID:%#x(sidx:%u/inf:%u), result:%u (fdelay:%u/min_fl(s+m):%u/min_fl(maxFPS):%u), predict:%u(next:%u/stable:%u)\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx),
		result,
		pfrec->fl_act_delay,
		s_m_min_lc,
		min_fl_lc,
		label,
		PREDICT_NEXT_FL,
		PREDICT_STABLE_FL);

	return result;
}


static unsigned int frec_calc_seamless_frame_length(const unsigned int idx,
	const struct FrameRecorder *pfrec,
	const struct frec_seamless_st *p_seamless_rec)
{
	const struct FrameRecord *frame_rec = &p_seamless_rec->frame_rec;
	const struct fs_seamless_property_st *ss_prop = &p_seamless_rec->prop;
	const unsigned int orig_fl_us = pfrec->curr_predicted_fl_us;
	const unsigned int new_mode_line_time_ns =
		calcLineTimeInNs(
			p_seamless_rec->frame_rec.pclk,
			p_seamless_rec->frame_rec.line_length);
	const int first_exp_idx =
		g_exp_order_idx_mapping(idx,
			frame_rec->exp_order, frame_rec->mode_exp_cnt, 0, __func__);
	const unsigned int log_str_len = 512;
	unsigned int fl_us_composition[3] = {0};
	unsigned int curr_exp_read_offset;
	unsigned int depth_idx, seamless_shutter_lc = 0;
	unsigned int result = 0;
	int orig_last_exp_idx;
	int len = 0, ret;
	char *log_buf = NULL;

	/* Part-1: calculate end of readout time us */
	depth_idx = FS_ATOMIC_READ(&pfrec->depth_idx);
	orig_last_exp_idx = frec_g_mode_last_exp_idx(idx, pfrec, depth_idx);
	curr_exp_read_offset =
		pfrec->curr_predicted_rd_offset_us[orig_last_exp_idx];
	fl_us_composition[0] =
		curr_exp_read_offset + ss_prop->orig_readout_time_us;

	/* Part-3: calculate seamless (new) mode re-shutter time us */
	if (unlikely(first_exp_idx < 0)) {
		seamless_shutter_lc = 0;
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), exp_order_idx_map[%u][%u][0]:%d(< 0) => assign seamless_shutter_lc:%u\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			frame_rec->exp_order,
			frame_rec->mode_exp_cnt,
			first_exp_idx,
			seamless_shutter_lc);
	} else {
		/* check normal or hdr situation (normal: shutter_lc / hdr: hdr_exp) */
		seamless_shutter_lc = (p_seamless_rec->frame_rec.ae_exp_cnt > 1)
			? (p_seamless_rec->frame_rec.exp_lc_arr[first_exp_idx])
			: (p_seamless_rec->frame_rec.shutter_lc);
	}
	fl_us_composition[2] =
		convert2TotalTime(new_mode_line_time_ns, seamless_shutter_lc);

	/* Part-2: by seamless switch type, has different behavior => need differnt process */
	switch (ss_prop->type_id) {
	case FREC_SEAMLESS_SWITCH_CUT_VB_INIT_SHUT:
		if (ss_prop->prsh_length_lc > 0) {
			if (unlikely(ss_prop->prsh_length_lc < seamless_shutter_lc)) {
				/* !!! unexpected case !!! */
				/* dump more info */
				frec_dump_frame_record_info(
					&p_seamless_rec->frame_rec, __func__);

				LOG_MUST(
					"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), get prsh_length_lc:%u but seamless_shutter_lc(NE):%u, prsh_length_lc value may be wrong\n",
					idx,
					fs_get_reg_sensor_id(idx),
					fs_get_reg_sensor_idx(idx),
					fs_get_reg_sensor_inf_idx(idx),
					ss_prop->prsh_length_lc,
					seamless_shutter_lc);

				fl_us_composition[1] =
					ss_prop->hw_re_init_time_us;
			} else {
				fl_us_composition[1] =
					ss_prop->hw_re_init_time_us +
					convert2TotalTime(
						new_mode_line_time_ns,
						ss_prop->prsh_length_lc
							- seamless_shutter_lc);
			}
		} else {
			fl_us_composition[1] =
				ss_prop->hw_re_init_time_us;
		}
		break;
	case FREC_SEAMLESS_SWITCH_ORIG_VB_INIT_SHUT:
		fl_us_composition[1] =
			orig_fl_us + ss_prop->hw_re_init_time_us;
		break;
	case FREC_SEAMLESS_SWITCH_ORIG_VB_ORIG_IMG:
		fl_us_composition[1] = orig_fl_us;
		break;
	default:
		break;
	}

	/* calculate the frame length of seamless switch frame */
	result =
		fl_us_composition[0] +
		fl_us_composition[1] +
		fl_us_composition[2];

	/* for print info */
	ret = alloc_log_buf(log_str_len, &log_buf);
	if (unlikely(ret != 0)) {
		LOG_MUST(
			"ERROR: [%u] log_buf allocate memory failed, only return result:%u\n",
			idx, result);
		return result;
	}

	FS_SNPRF(log_str_len, log_buf, len,
		"NOTICE: [%u] ID:%#x(sidx:%u/inf:%u), seamless_fl_us:%u(%u/%u/%u(%u)), new_mode_line_t:%u, r_offset[%u]:%u, type_id:%u, orig_readout_t:%u, hw_re_init_t:%u, prsh_length_lc:%u, (exp_lc:%u, (a:%u/m:%u(%u,%u), exp:%u/%u/%u/%u/%u)",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx),
		result,
		fl_us_composition[0],
		fl_us_composition[1],
		fl_us_composition[2],
		seamless_shutter_lc,
		new_mode_line_time_ns,
		orig_last_exp_idx,
		curr_exp_read_offset,
		ss_prop->type_id,
		ss_prop->orig_readout_time_us,
		ss_prop->hw_re_init_time_us,
		ss_prop->prsh_length_lc,
		p_seamless_rec->frame_rec.shutter_lc,
		p_seamless_rec->frame_rec.ae_exp_cnt,
		p_seamless_rec->frame_rec.mode_exp_cnt,
		p_seamless_rec->frame_rec.m_exp_type,
		p_seamless_rec->frame_rec.exp_order,
		p_seamless_rec->frame_rec.exp_lc_arr[0],
		p_seamless_rec->frame_rec.exp_lc_arr[1],
		p_seamless_rec->frame_rec.exp_lc_arr[2],
		p_seamless_rec->frame_rec.exp_lc_arr[3],
		p_seamless_rec->frame_rec.exp_lc_arr[4]);
	LOG_MUST("%s\n", log_buf);
	FS_TRACE_PR_LOG_INF("%s", log_buf);

	FS_FREE(log_buf);

	return result;
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// sensor recorder framework functions
/*----------------------------------------------------------------------------*/
void frec_chk_fl_pr_match_act(const unsigned int idx)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	const unsigned int diff_th = FL_ACT_CHK_TH;
	unsigned int diff;

	/* error handle */
	if (unlikely((pfrec == NULL) || (pfrec->act_fl_us == 0)))
		return;

	diff = (pfrec->prev_predicted_fl_us > pfrec->act_fl_us)
		? (pfrec->prev_predicted_fl_us - pfrec->act_fl_us)
		: (pfrec->act_fl_us - pfrec->prev_predicted_fl_us);

	if (unlikely(diff > diff_th)) {
		LOG_MUST_LOCK(
			"WARNING: [%u] ID:%#x(sidx:%u/inf:%u), frame length (fdelay:%u): pr(p)(%u(%u)/act:%u) seems not match, plz check manually\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			pfrec->fl_act_delay,
			pfrec->prev_predicted_fl_us,
			pfrec->prev_predicted_fl_lc,
			pfrec->act_fl_us);

		frec_dump_recorder(idx, __func__);
	} else {
		if (unlikely(_FS_LOG_ENABLED(LOG_FS_PF)))
			frec_dump_recorder(idx, __func__);
	}
}


/*
 * Notify fs algo and frame monitor the data in the frame recorder have been
 * updated
 *
 * This function should be call after having any frame recorder operation
 *
 *
 * description:
 *     fs algo will use these information to predict current and
 *     next framelength when calculating vsync diff.
 */
static void frec_notify_setting_frame_record_st_data(const unsigned int idx)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	struct FrameRecord *recs_ordered[RECORDER_DEPTH];
	struct predicted_fl_info_st fl_info = {0};
	unsigned int depth_idx;
	unsigned int i;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	if (unlikely(!pfrec->is_init)) {
		LOG_INF(
			"NOTICE: [%u] frec has not been initialized(setup def val, fdelay), is_init:%u, return\n",
			idx,
			pfrec->is_init);
		return;
	}

	depth_idx = FS_ATOMIC_READ(&pfrec->depth_idx);

	/* prepare frame settings in the recorder */
	/*    => 0:newest, 1:second, 2:third */
	for (i = 0; i < RECORDER_DEPTH; ++i) {
		recs_ordered[i] = &pfrec->frame_recs[depth_idx];

		/* ring back depth_idx */
		depth_idx = RING_BACK(depth_idx, 1);
	}
	frec_get_predicted_frame_length_info(
		idx, recs_ordered[0], &fl_info, __func__);

	/* call fs alg set to update frame record data */
	fs_alg_set_frame_record_st_data(idx, recs_ordered, &fl_info);
}


static void frec_init_recorder_fl_related_info(const unsigned int idx,
	const struct FrameRecord *p_frame_rec)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	struct FrameRecord *curr_rec = NULL;
	struct predicted_fl_info_st fl_info = {0};

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	curr_rec = &pfrec->frame_recs[0];

	/* init predicted frame length */
	pfrec->prev_predicted_fl_lc = 0;
	pfrec->prev_predicted_fl_us = 0;

	/* trigger calculate newest predicted fl info */
	frec_get_predicted_frame_length_info(
		idx, curr_rec, &fl_info, __func__);

	/* copy newest curr predicted fl */
	pfrec->curr_predicted_fl_lc = fl_info.pr_next_fl_lc;
	pfrec->curr_predicted_fl_us = fl_info.pr_next_fl_us;

	/* copy result of newest next read offset info */
	memcpy(pfrec->next_predicted_rd_offset_lc,
		&fl_info.next_exp_rd_offset_lc,
		sizeof(unsigned int) * (FS_HDR_MAX));
	memcpy(pfrec->next_predicted_rd_offset_us,
		&fl_info.next_exp_rd_offset_us,
		sizeof(unsigned int) * (FS_HDR_MAX));

	/* init read offset value of each shutters */
	memcpy(pfrec->curr_predicted_rd_offset_lc,
		pfrec->next_predicted_rd_offset_lc,
		sizeof(unsigned int) * (FS_HDR_MAX));
	memcpy(pfrec->curr_predicted_rd_offset_us,
		pfrec->next_predicted_rd_offset_us,
		sizeof(unsigned int) * (FS_HDR_MAX));
}


static void frec_init_recorder_seamless_fl_related_info(const unsigned int idx,
	const unsigned int seamless_fl_us,
	const struct FrameRecord *p_frame_rec)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	struct FrameRecord *curr_rec = NULL;
	struct predicted_fl_info_st fl_info = {0};

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	curr_rec = &pfrec->frame_recs[0];

	/* setup predicted frame length (seamless frame) */
	pfrec->curr_predicted_fl_lc =
		convert2LineCount(
			calcLineTimeInNs(
				p_frame_rec->pclk, p_frame_rec->line_length),
			seamless_fl_us);
	pfrec->curr_predicted_fl_us = seamless_fl_us;

	/* trigger calculate newest predicted fl info */
	frec_get_predicted_frame_length_info(
		idx, curr_rec, &fl_info, __func__);

	/* copy result of newest next read offset info */
	memcpy(pfrec->next_predicted_rd_offset_lc,
		&fl_info.next_exp_rd_offset_lc,
		sizeof(unsigned int) * (FS_HDR_MAX));
	memcpy(pfrec->next_predicted_rd_offset_us,
		&fl_info.next_exp_rd_offset_us,
		sizeof(unsigned int) * (FS_HDR_MAX));
}


static void frec_notify_pre_latch_setup_fl_related_info(const unsigned int idx)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	struct FrameRecord *curr_rec = NULL;
	struct predicted_fl_info_st fl_info = {0};
	unsigned int curr_idx;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	curr_idx = FS_ATOMIC_READ(&pfrec->depth_idx);
	curr_rec = &pfrec->frame_recs[curr_idx];

	/* copy latest curr predicted fl to prev */
	pfrec->prev_predicted_fl_lc = pfrec->curr_predicted_fl_lc;
	pfrec->prev_predicted_fl_us = pfrec->curr_predicted_fl_us;

	/* copy read offset info of next to current */
	memcpy(pfrec->curr_predicted_rd_offset_lc,
		pfrec->next_predicted_rd_offset_lc,
		sizeof(unsigned int) * (FS_HDR_MAX));
	memcpy(pfrec->curr_predicted_rd_offset_us,
		pfrec->next_predicted_rd_offset_us,
		sizeof(unsigned int) * (FS_HDR_MAX));

	/* trigger calculate newest predicted fl info */
	frec_get_predicted_frame_length_info(
		idx, curr_rec, &fl_info, __func__);

	/* copy result of newest curr predicted fl */
	pfrec->curr_predicted_fl_lc = fl_info.pr_next_fl_lc;
	pfrec->curr_predicted_fl_us = fl_info.pr_next_fl_us;

	/* copy result of newest next read offset info */
	memcpy(pfrec->next_predicted_rd_offset_lc,
		&fl_info.next_exp_rd_offset_lc,
		sizeof(unsigned int) * (FS_HDR_MAX));
	memcpy(pfrec->next_predicted_rd_offset_us,
		&fl_info.next_exp_rd_offset_us,
		sizeof(unsigned int) * (FS_HDR_MAX));
}


void frec_update_fl_info(const unsigned int idx, const unsigned int fl_lc,
	const unsigned int fl_lc_arr[], const unsigned int arr_len)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	unsigned int depth_idx;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;
	if (unlikely(fl_lc == 0)) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u) get: fl_lc:%u, do NOT update frec data, return\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			fl_lc);

		frec_dump_recorder(idx, __func__);
		return;
	}

	frec_mutex_lock(&pfrec->frame_recs_update_lock);

	depth_idx = FS_ATOMIC_READ(&pfrec->depth_idx);

	/* update values */
	pfrec->frame_recs[depth_idx].framelength_lc = fl_lc;
	memcpy(pfrec->frame_recs[depth_idx].fl_lc_arr, fl_lc_arr,
		sizeof(unsigned int) * arr_len);

#if defined(TRACE_FS_FREC_LOG)
	frec_dump_frame_record_info(&pfrec->frame_recs[depth_idx], __func__);
#endif

	/* set the results to fs algo and frame monitor */
	frec_notify_setting_frame_record_st_data(idx);

	frec_mutex_unlock(&pfrec->frame_recs_update_lock);
}


void frec_update_record(const unsigned int idx,
	const struct FrameRecord *p_frame_rec)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	unsigned int curr_depth_idx;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	frec_mutex_lock(&pfrec->frame_recs_update_lock);

	curr_depth_idx = FS_ATOMIC_READ(&pfrec->depth_idx);

	/* setup/update record data */
	memcpy(&pfrec->frame_recs[curr_depth_idx], p_frame_rec,
		sizeof(pfrec->frame_recs[curr_depth_idx]));

#if defined(TRACE_FS_FREC_LOG)
	frec_dump_recorder(idx, __func__);
#endif

	/* set the results to fs algo and frame monitor */
	frec_notify_setting_frame_record_st_data(idx);

	frec_mutex_unlock(&pfrec->frame_recs_update_lock);
}


void frec_push_record(const unsigned int idx)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	unsigned long long sys_ts = 0;
	unsigned int curr_depth_idx, next_depth_idx;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;
	if (unlikely(pfrec->is_init == 0)) {
		LOG_MUST(
			"WARNING: [%u] ID:%#x(sidx:%u/inf:%u) MUST initialize sensor recorder first\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx));
	}

	frec_mutex_lock(&pfrec->frame_recs_update_lock);

#ifndef FS_UT
	sys_ts = ktime_get_boottime_ns();
#endif
	curr_depth_idx = FS_ATOMIC_READ(&pfrec->depth_idx);
	next_depth_idx = RING_FORWARD(curr_depth_idx, 1);

	/* depth idx ring forward */
	FS_ATOMIC_SET(next_depth_idx, &pfrec->depth_idx);

	/* copy latest sensor record to newest depth idx */
	memcpy(&pfrec->frame_recs[next_depth_idx],
		&pfrec->frame_recs[curr_depth_idx],
		sizeof(pfrec->frame_recs[next_depth_idx]));

	/* update system timestamp for debugging */
	pfrec->sys_ts_recs[next_depth_idx] = sys_ts;

#if defined(TRACE_FS_FREC_LOG)
	LOG_MUST(
		"[%u] ID:%#x(sidx:%u/inf:%u) => curr/latest at recs[%u]=(%u/%u), sys_ts:%llu, recs:(depth_idx:%u(new), (0:%u/%u), (1:%u/%u), (2:%u/%u), (3:%u/%u) (fl_lc/shut_lc))\n",
		idx,
		fs_get_reg_sensor_id(idx),
		fs_get_reg_sensor_idx(idx),
		fs_get_reg_sensor_inf_idx(idx),
		curr_depth_idx,
		pfrec->frame_recs[curr_depth_idx].framelength_lc,
		pfrec->frame_recs[curr_depth_idx].shutter_lc,
		sys_ts,
		next_depth_idx,
		pfrec->frame_recs[0].framelength_lc,
		pfrec->frame_recs[0].shutter_lc,
		pfrec->frame_recs[1].framelength_lc,
		pfrec->frame_recs[1].shutter_lc,
		pfrec->frame_recs[2].framelength_lc,
		pfrec->frame_recs[2].shutter_lc,
		pfrec->frame_recs[3].framelength_lc,
		pfrec->frame_recs[3].shutter_lc);
#endif

	/* set the results to fs algo and frame monitor */
	frec_notify_setting_frame_record_st_data(idx);

	frec_mutex_unlock(&pfrec->frame_recs_update_lock);
}


void frec_reset_records(const unsigned int idx)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	unsigned int i;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	frec_mutex_lock(&pfrec->frame_recs_update_lock);

	for (i = 0; i < RECORDER_DEPTH; ++i) {
		memset(&pfrec->frame_recs[i], 0, sizeof(pfrec->frame_recs[i]));

		pfrec->sys_ts_recs[i] = 0;
	}

	FS_ATOMIC_SET(0, &pfrec->depth_idx);
	pfrec->is_init = 0;

#if defined(TRACE_FS_FREC_LOG)
	frec_dump_recorder(idx, __func__);
#endif

	frec_mutex_unlock(&pfrec->frame_recs_update_lock);
}


void frec_setup_def_records(const unsigned int idx, const unsigned int def_fl_lc,
	const struct FrameRecord *p_frame_rec)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	unsigned int i;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;
	if (unlikely(pfrec->is_init)) {
		LOG_MUST(
			"WARNING: [%u] frec had been initialized(setup def/init value), is_init:%u, return\n",
			idx,
			pfrec->is_init);

		frec_dump_recorder(idx, __func__);
		return;
	}

	frec_mutex_lock(&pfrec->frame_recs_update_lock);

	/* init all frec value to default shutter and framelength */
	pfrec->is_init = 1;
	FS_ATOMIC_SET(0, &pfrec->depth_idx);
	for (i = 0; i < RECORDER_DEPTH; ++i) {
		memcpy(&pfrec->frame_recs[i], p_frame_rec,
			sizeof(pfrec->frame_recs[i]));

		pfrec->sys_ts_recs[i] = 0;
	}
	pfrec->def_fl_lc = def_fl_lc;
	frec_init_recorder_fl_related_info(idx, p_frame_rec);

	/* set the results to fs algo and frame monitor */
	frec_notify_setting_frame_record_st_data(idx);

	frec_mutex_unlock(&pfrec->frame_recs_update_lock);

	frec_dump_recorder(idx, __func__);
}


void frec_init_recorder(const unsigned int idx,
	const struct FrameRecord *p_frame_rec,
	const unsigned int def_fl_lc, const unsigned int fl_act_delay)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	/* setup basic sensor info */
	pfrec->fl_act_delay = fl_act_delay;

	/* error handle, check sensor fl_active_delay value */
	if (unlikely((fl_act_delay < 2) || (fl_act_delay > 3))) {
		LOG_MUST(
			"ERROR: [%u] ID:%#x(sidx:%u/inf:%u), sensor driver's frame_time_delay_frame:%u is not valid (MUST be 2 or 3), plz check sensor driver for getting correct value\n",
			idx,
			fs_get_reg_sensor_id(idx),
			fs_get_reg_sensor_idx(idx),
			fs_get_reg_sensor_inf_idx(idx),
			fl_act_delay);
	}

	frec_setup_def_records(idx, def_fl_lc, p_frame_rec);
}


void frec_seamless_switch(const unsigned int idx,
	const unsigned int def_fl_lc, const unsigned int fl_act_delay,
	const struct frec_seamless_st *p_seamless_rec)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	unsigned int seamless_fl_us;
	unsigned int i;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	if (_FS_LOG_ENABLED(LOG_SEN_REC_SEAMLESS_DUMP))
		frec_dump_recorder(idx, __func__);

	frec_mutex_lock(&pfrec->frame_recs_update_lock);

	seamless_fl_us =
		frec_calc_seamless_frame_length(idx, pfrec, p_seamless_rec);

	FS_ATOMIC_SET(0, &pfrec->depth_idx);
	for (i = 0; i < RECORDER_DEPTH; ++i) {
		memcpy(&pfrec->frame_recs[i], &p_seamless_rec->frame_rec,
			sizeof(pfrec->frame_recs[i]));

		pfrec->sys_ts_recs[i] = 0;
	}
	pfrec->fl_act_delay = fl_act_delay;
	pfrec->def_fl_lc = def_fl_lc;
	frec_init_recorder_seamless_fl_related_info(
		idx, seamless_fl_us, &p_seamless_rec->frame_rec);

	/* set the results to fs algo and frame monitor */
	frec_notify_setting_frame_record_st_data(idx);

	frec_mutex_unlock(&pfrec->frame_recs_update_lock);

	if (_FS_LOG_ENABLED(LOG_SEN_REC_SEAMLESS_DUMP))
		frec_dump_recorder(idx, __func__);
}


// void frec_notify_sensor_pre_latch(const unsigned int idx)
void frec_notify_vsync(const unsigned int idx)
{
	const struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	/* !!! do each thing that needed at sensor pre latch timing !!! */

	/* first, calculate newest next predict frame length */
	/* and update to current predicted frame length */
	frec_notify_pre_latch_setup_fl_related_info(idx);

	/* then, update/push newest sensor frame record */
	frec_push_record(idx);
}


void frec_notify_update_timestamp_data(const unsigned int idx,
	const unsigned int tick_factor,
	const SenRec_TS_T ts_us[], const unsigned int arr_len)
{
	struct FrameRecorder *pfrec = frec_g_recorder_ctx(idx, __func__);
	SenRec_TS_T tick_a, tick_b;

	/* error handle */
	if (unlikely(pfrec == NULL))
		return;

	/* copy/update newest timestamp data */
	memcpy(pfrec->ts_exp_0, ts_us, sizeof(SenRec_TS_T) * arr_len);
	pfrec->tick_factor = tick_factor;

	/* check if this is first timestamp */
	if (unlikely(pfrec->ts_exp_0[1] == 0)) {
		pfrec->act_fl_us = 0;
		return;
	}

	/* update actual frame length by timestamp diff */
	tick_a = pfrec->ts_exp_0[0] * pfrec->tick_factor;
	tick_b = pfrec->ts_exp_0[1] * pfrec->tick_factor;
	pfrec->act_fl_us = divide_num(idx,
		(tick_a - tick_b), pfrec->tick_factor, __func__);
}
/******************************************************************************/

