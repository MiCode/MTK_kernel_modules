/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_osal.h"
#include "gps_dl_time_tick.h"
#include "gps_mcudl_config.h"
#include "gps_mcudl_hal_stat.h"
#include "gps_mcudl_ylink.h"

#define GPS_MCUDL_STAT_REC_SIZE (8)

struct gps_mcudl_stat_history_ctx {
	unsigned int open_duration_min;
	unsigned int open_duration_max;

	unsigned int close_duration_min;
	unsigned int close_duration_max;

	unsigned int exception_cnt;
	unsigned int force_close_cnt;

	unsigned int curr_mcu_sid;
	unsigned int pwr_wrn_cnt;
	unsigned long pwr_dump_ktime_ms;
	unsigned long pwr_trig_ktime_ms;
};

struct gps_mcudl_stat_lp_ctx {
	unsigned long get_idx[GPS_MCUDL_STAT_GET_USER_CNT];
	unsigned long set_idx;
	struct gps_mcudl_stat_lp_data data[GPS_MCUDL_STAT_REC_SIZE];
	struct gps_dl_osal_sleepable_lock lock;
};

struct gps_mcudl_stat_mcu_ctx {
	unsigned long get_idx[GPS_MCUDL_STAT_GET_USER_CNT];
	unsigned long set_idx;
	struct gps_mcudl_stat_mcu_data data[GPS_MCUDL_STAT_REC_SIZE];
	struct gps_dl_osal_sleepable_lock lock;
};

struct gps_mcudl_stat_history_ctx g_gps_history_data;
struct gps_mcudl_stat_lp_ctx g_gps_lp_data;
struct gps_mcudl_stat_mcu_ctx g_gps_mcu_data;

void gps_mcudl_stat_lp_ctx_init(void)
{
	enum gps_mcudl_stat_get_user user;

	for (user = 0; user < GPS_MCUDL_STAT_GET_USER_CNT; user++)
		g_gps_lp_data.get_idx[user] = 0;

	g_gps_lp_data.set_idx = 0;
	g_gps_history_data.pwr_dump_ktime_ms = 0;
	g_gps_history_data.pwr_trig_ktime_ms = 0;
	(void)gps_dl_osal_sleepable_lock_init(&g_gps_lp_data.lock);
}

void gps_mcudl_stat_lp_ctx_deinit(void)
{
	(void)gps_dl_osal_sleepable_lock_deinit(&g_gps_lp_data.lock);
}

void gps_mcudl_stat_mcu_ctx_init(void)
{
	enum gps_mcudl_stat_get_user user;

	for (user = 0; user < GPS_MCUDL_STAT_GET_USER_CNT; user++)
		g_gps_mcu_data.get_idx[user] = 0;

	g_gps_mcu_data.set_idx = 0;
	(void)gps_dl_osal_sleepable_lock_init(&g_gps_mcu_data.lock);
}

void gps_mcudl_stat_mcu_ctx_deinit(void)
{
	(void)gps_dl_osal_sleepable_lock_deinit(&g_gps_mcu_data.lock);
}

/* Ref: GPS_MCUDL_HAL_PWR_WRN_MS */
#define GPS_MCUDL_PWR_DUMP_TRIG_INTERVAL_MS (305 * 1000) /* 5m + 5s */
enum gps_mcudl_stat_get_result gps_mcudl_stat_get_lp_data(
	enum gps_mcudl_stat_get_user user,
	enum gps_mcudl_stat_get_reason reason, struct gps_mcudl_stat_lp_data *p)
{
	enum gps_mcudl_stat_get_result ret = GPS_MCUDL_STAT_NO_DATA;
	unsigned long get_idx = 0;
	unsigned long get_idx2 = 0;
	unsigned long set_idx = 0;
	struct gps_mcudl_stat_lp_data *p_data = NULL;
	int mutex_take_retval;
	unsigned long curr_ktime, curr_nohop_ktime_ms, d_dump_ktime_ms, d_trig_ktims_ms;
	bool trig_dump = false;

	curr_ktime = gps_dl_tick_get_ktime_ms();
	curr_nohop_ktime_ms = gps_dl_tick_get_no_hop_ktime_ms();
	if (user >= GPS_MCUDL_STAT_GET_USER_CNT) {
		ret = GPS_MCUDL_STAT_NO_DATA;
		GDL_LOGW("user=%d, reason=%d, ret=%d, invalid user",
			user, reason, ret);
		return ret;
	}

	mutex_take_retval = gps_dl_osal_lock_sleepable_lock(&g_gps_lp_data.lock);
	if (mutex_take_retval) {
		ret = GPS_MCUDL_STAT_NO_DATA;
		GDL_LOGW("user=%d, reason=%d, ret=%d, mutex fail=%d",
			user, reason, ret, mutex_take_retval);
		return ret;
	}

	get_idx = g_gps_lp_data.get_idx[user];
	set_idx = g_gps_lp_data.set_idx;

	d_dump_ktime_ms = curr_nohop_ktime_ms - g_gps_history_data.pwr_dump_ktime_ms;
	d_trig_ktims_ms = curr_nohop_ktime_ms - g_gps_history_data.pwr_trig_ktime_ms;
	if ((d_dump_ktime_ms >= GPS_MCUDL_PWR_DUMP_TRIG_INTERVAL_MS) &&
		(d_trig_ktims_ms >= GPS_MCUDL_PWR_DUMP_TRIG_INTERVAL_MS)) {
		g_gps_history_data.pwr_trig_ktime_ms = curr_nohop_ktime_ms;
		trig_dump = true;
	} else
		trig_dump = false;

	if ((get_idx == set_idx) || (get_idx > set_idx)) {
		ret = GPS_MCUDL_STAT_NO_DATA;
		if (get_idx > set_idx) {
			/* fix get_idx */
			g_gps_lp_data.get_idx[user] = set_idx;
		}
		(void)gps_dl_osal_unlock_sleepable_lock(&g_gps_lp_data.lock);

		GDL_LOGI("user=%d, reason=%d, ret=%d, ktime=%lu,%lu,%lu,%d, idx=%lu,%lu",
			user, reason, ret,
			curr_ktime, d_dump_ktime_ms, d_trig_ktims_ms, trig_dump,
			set_idx, get_idx);
	} else {

		if (get_idx + GPS_MCUDL_STAT_REC_SIZE - 1 < set_idx)
			get_idx2 = (set_idx + 1 - GPS_MCUDL_STAT_REC_SIZE);
		else
			get_idx2 = get_idx;

		p_data = &g_gps_lp_data.data[get_idx2 % GPS_MCUDL_STAT_REC_SIZE];
		*p = *p_data;
		if (get_idx2 + 1 == set_idx)
			ret = GPS_MCUDL_STAT_OK_NO_MORE;
		else
			ret = GPS_MCUDL_STAT_OK_MORE;
		g_gps_lp_data.get_idx[user] = get_idx2 + 1;
		(void)gps_dl_osal_unlock_sleepable_lock(&g_gps_lp_data.lock);

		GDL_LOGI("user=%d, reason=%d, ret=%d, ktime=%lu,%lu,%lu,%d, idx=%lu,%lu,%lu",
			user, reason, ret,
			curr_ktime, d_dump_ktime_ms, d_trig_ktims_ms, trig_dump,
			set_idx, get_idx, get_idx2);
	}

	if (trig_dump)
		gps_mcudl_ylink_on_ap_resume();

	return ret;
}

void gps_mcudl_stat_set_pwr_state_data(unsigned long local_ms, unsigned long ktime_ms,
	unsigned int dump_index, bool is_on, bool pwr_hi, bool pwr_wrn)
{
	unsigned int set_idx = (g_gps_lp_data.set_idx % GPS_MCUDL_STAT_REC_SIZE);
	struct gps_mcudl_stat_lp_data *p = &g_gps_lp_data.data[set_idx];
	unsigned long curr_nohop_ktime_ms = gps_dl_tick_get_no_hop_ktime_ms();
	int mutex_take_retval;

	p->dump_index = dump_index;
	p->dump_local_ms = (u64)local_ms;
	p->dump_ktime_ms = (u64)ktime_ms;
	p->gnss_mcu_sid = g_gps_history_data.curr_mcu_sid;
	p->gnss_mcu_is_on = is_on;
	p->gnss_pwr_is_hi = pwr_hi;
	p->gnss_pwr_wrn = pwr_wrn;
	if (pwr_wrn)
		g_gps_history_data.pwr_wrn_cnt++;
	p->gnss_pwr_wrn_cnt = g_gps_history_data.pwr_wrn_cnt;

	mutex_take_retval = gps_dl_osal_lock_sleepable_lock(&g_gps_lp_data.lock);
	if (mutex_take_retval) {
		GDL_LOGW("mutex fail=%d, idx=%lu",
			mutex_take_retval, g_gps_lp_data.set_idx);
		return;
	}

	g_gps_lp_data.set_idx++;
	g_gps_history_data.pwr_dump_ktime_ms = curr_nohop_ktime_ms;
	(void)gps_dl_osal_unlock_sleepable_lock(&g_gps_lp_data.lock);
}

void gps_mcudl_stat_set_open_duration_to_history(unsigned int ms)
{
	if ((g_gps_history_data.open_duration_min > ms) ||
		(g_gps_history_data.open_duration_min == 0)) {
		g_gps_history_data.open_duration_min = ms;
	}

	if ((g_gps_history_data.open_duration_max < ms) ||
		(g_gps_history_data.open_duration_max == 0)) {
		g_gps_history_data.open_duration_max = ms;
	}
}

void gps_mcudl_stat_set_close_duration_to_history(unsigned int ms)
{
	if ((g_gps_history_data.close_duration_min > ms) ||
		(g_gps_history_data.close_duration_min == 0)) {
		g_gps_history_data.close_duration_min = ms;
	}

	if ((g_gps_history_data.close_duration_max < ms) ||
		(g_gps_history_data.close_duration_max == 0)) {
		g_gps_history_data.close_duration_max = ms;
	}
}

enum gps_mcudl_stat_get_result gps_mcudl_stat_get_mcu_data(
	enum gps_mcudl_stat_get_user user,
	enum gps_mcudl_stat_get_reason reason, struct gps_mcudl_stat_mcu_data *p)
{
	enum gps_mcudl_stat_get_result ret = GPS_MCUDL_STAT_NO_DATA;
	unsigned long get_idx = 0;
	unsigned long get_idx2 = 0;
	unsigned long set_idx = 0;
	struct gps_mcudl_stat_mcu_data *p_data = NULL;
	int mutex_take_retval;

	if (user >= GPS_MCUDL_STAT_GET_USER_CNT) {
		ret = GPS_MCUDL_STAT_NO_DATA;
		GDL_LOGW("user=%d, reason=%d, ret=%d, invalid user",
			user, reason, ret);
		return ret;
	}

	mutex_take_retval = gps_dl_osal_lock_sleepable_lock(&g_gps_mcu_data.lock);
	if (mutex_take_retval) {
		ret = GPS_MCUDL_STAT_NO_DATA;
		GDL_LOGW("user=%d, reason=%d, ret=%d, mutex fail=%d",
			user, reason, ret, mutex_take_retval);
		return ret;
	}

	get_idx = g_gps_mcu_data.get_idx[user];
	set_idx = g_gps_mcu_data.set_idx;

	if ((get_idx == set_idx) || (get_idx > set_idx)) {
		ret = GPS_MCUDL_STAT_NO_DATA;
		if (get_idx > set_idx) {
			/* fix get_idx */
			g_gps_mcu_data.get_idx[user] = set_idx;
		}
		(void)gps_dl_osal_unlock_sleepable_lock(&g_gps_mcu_data.lock);

		GDL_LOGI("user=%d, reason=%d, ret=%d, idx=%lu,%lu",
			user, reason, ret, set_idx, get_idx);
		return ret;
	}

	if (get_idx + GPS_MCUDL_STAT_REC_SIZE - 1 < set_idx)
		get_idx2 = (set_idx + 1 - GPS_MCUDL_STAT_REC_SIZE);
	else
		get_idx2 = get_idx;

	p_data = &g_gps_mcu_data.data[get_idx2 % GPS_MCUDL_STAT_REC_SIZE];
	*p = *p_data;
	if (get_idx2 + 1 == set_idx)
		ret = GPS_MCUDL_STAT_OK_NO_MORE;
	else
		ret = GPS_MCUDL_STAT_OK_MORE;
	g_gps_mcu_data.get_idx[user] = get_idx2 + 1;
	(void)gps_dl_osal_unlock_sleepable_lock(&g_gps_mcu_data.lock);

	GDL_LOGI("user=%d, reason=%d, ret=%d, idx=%lu,%lu,%lu",
		user, reason, ret, set_idx, get_idx, get_idx2);
	return ret;
}

void gps_mcudl_stat_set_mcu_sid(unsigned int sid)
{
	unsigned long set_idx = (g_gps_mcu_data.set_idx % GPS_MCUDL_STAT_REC_SIZE);
	struct gps_mcudl_stat_mcu_data *p = &g_gps_mcu_data.data[set_idx];

	p->gnss_mcu_sid = sid;
	p->has_exception = false;
	p->force_close = false;
	g_gps_history_data.curr_mcu_sid = sid;
}

void gps_mcudl_stat_set_clock_type(unsigned int type)
{
	unsigned long set_idx = (g_gps_mcu_data.set_idx % GPS_MCUDL_STAT_REC_SIZE);
	struct gps_mcudl_stat_mcu_data *p = &g_gps_mcu_data.data[set_idx];

	p->clock_cfg_val = type;
}

void gps_mcudl_stat_set_mcu_open_info(unsigned long local_ms, unsigned long ktime_ms, unsigned int d_ms)
{
	unsigned long set_idx = (g_gps_mcu_data.set_idx % GPS_MCUDL_STAT_REC_SIZE);
	struct gps_mcudl_stat_mcu_data *p = &g_gps_mcu_data.data[set_idx];

	p->open_local_ms = (u64)local_ms;
	p->open_ktime_ms = (u64)ktime_ms;
	p->open_duration = d_ms;
	gps_mcudl_stat_set_open_duration_to_history(d_ms);
}

void gps_mcudl_stat_set_mcu_exception(void)
{
	unsigned long set_idx = (g_gps_mcu_data.set_idx % GPS_MCUDL_STAT_REC_SIZE);
	struct gps_mcudl_stat_mcu_data *p = &g_gps_mcu_data.data[set_idx];

	p->has_exception = true;
	g_gps_history_data.exception_cnt++;
}

void gps_mcudl_stat_set_mcu_force_close(void)
{
	unsigned long set_idx = (g_gps_mcu_data.set_idx % GPS_MCUDL_STAT_REC_SIZE);
	struct gps_mcudl_stat_mcu_data *p = &g_gps_mcu_data.data[set_idx];

	p->force_close = true;
	g_gps_history_data.force_close_cnt++;
}

void gps_mcudl_stat_set_mcu_close_info(unsigned long local_ms, unsigned long ktime_ms, unsigned int d_ms)
{
	unsigned long set_idx = (g_gps_mcu_data.set_idx % GPS_MCUDL_STAT_REC_SIZE);
	struct gps_mcudl_stat_mcu_data *p = &g_gps_mcu_data.data[set_idx];
	int mutex_take_retval;

	p->close_local_ms = (u64)local_ms;
	p->close_ktime_ms = (u64)ktime_ms;
	p->close_duration = d_ms;
	gps_mcudl_stat_set_close_duration_to_history(d_ms);
	p->exception_cnt      = g_gps_history_data.exception_cnt;
	p->force_close_cnt    = g_gps_history_data.force_close_cnt;
	p->open_duration_min  = g_gps_history_data.open_duration_min;
	p->open_duration_max  = g_gps_history_data.open_duration_max;
	p->close_duration_min = g_gps_history_data.close_duration_min;
	p->close_duration_max = g_gps_history_data.close_duration_max;

	mutex_take_retval = gps_dl_osal_lock_sleepable_lock(&g_gps_mcu_data.lock);
	if (mutex_take_retval) {
		GDL_LOGI("mutex fail=%d, idx=%lu",
			mutex_take_retval, g_gps_mcu_data.set_idx);
		return;
	}

	g_gps_mcu_data.set_idx++;
	(void)gps_dl_osal_unlock_sleepable_lock(&g_gps_mcu_data.lock);
}

void gps_mcudl_stat_dump_lp_data(void)
{
	enum gps_mcudl_stat_get_result ret;
	struct gps_mcudl_stat_lp_data lp_data;

dump_more:
	ret = gps_mcudl_stat_get_lp_data(
		GPS_MCUDL_STAT_GET_BY_TEST, GPS_MCUDL_STAT_GET_FOR_TEST, &lp_data);
	if (ret == GPS_MCUDL_STAT_NO_DATA) {
		GDL_LOGI("lp_data_dump: ret=%d", ret);
		return;
	}

	GDL_LOGI("lp_data_dump: ret=%d, idx=%u, ts=%llu,%llu, sid=%u, st=%d,%d,%d,%u",
		ret,
		lp_data.dump_index,
		lp_data.dump_local_ms,
		lp_data.dump_ktime_ms,
		lp_data.gnss_mcu_sid,
		lp_data.gnss_mcu_is_on,
		lp_data.gnss_pwr_is_hi,
		lp_data.gnss_pwr_wrn,
		lp_data.gnss_pwr_wrn_cnt
	);

	if (ret == GPS_MCUDL_STAT_OK_MORE)
		goto dump_more;
}

void gps_mcudl_stat_dump_mcu_data(void)
{
	enum gps_mcudl_stat_get_result ret;
	struct gps_mcudl_stat_mcu_data mcu_data;

dump_more:
	ret = gps_mcudl_stat_get_mcu_data(
		GPS_MCUDL_STAT_GET_BY_TEST, GPS_MCUDL_STAT_GET_FOR_TEST, &mcu_data);
	if (ret == GPS_MCUDL_STAT_NO_DATA) {
		GDL_LOGI("mcu_data_dump: ret=%d", ret);
		return;
	}

	GDL_LOGI("mcu_data_dump: ret=%d, sid=%u,0x%x, o=%llu,%llu,%u[%u,%u], e=%d,%d,%u,%u c=%llu,%llu,%u[%u,%u]",
		ret,
		mcu_data.gnss_mcu_sid,
		mcu_data.clock_cfg_val,

		mcu_data.open_local_ms,
		mcu_data.open_ktime_ms,
		mcu_data.open_duration,
		mcu_data.open_duration_min,
		mcu_data.open_duration_max,

		mcu_data.has_exception,
		mcu_data.force_close,
		mcu_data.exception_cnt,
		mcu_data.force_close_cnt,

		mcu_data.close_local_ms,
		mcu_data.close_ktime_ms,
		mcu_data.close_duration,
		mcu_data.close_duration_min,
		mcu_data.close_duration_max
	);

	if (ret == GPS_MCUDL_STAT_OK_MORE)
		goto dump_more;
}

