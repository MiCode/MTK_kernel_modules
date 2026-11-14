/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HAL_STAT_H
#define _GPS_MCUDL_HAL_STAT_H

void gps_mcudl_stat_set_mcu_sid(unsigned int sid);
void gps_mcudl_stat_set_clock_type(unsigned int type);
void gps_mcudl_stat_set_mcu_open_info(unsigned long local_ms, unsigned long ktime_ms,
	unsigned int d_ms);

void gps_mcudl_stat_set_mcu_exception(void);
void gps_mcudl_stat_set_mcu_force_close(void);
void gps_mcudl_stat_set_mcu_close_info(unsigned long local_ms, unsigned long ktime_ms,
	unsigned int d_ms);

void gps_mcudl_stat_set_pwr_state_data(unsigned long local_ms, unsigned long ktime_ms,
	unsigned int dump_index, bool is_on, bool pwr_hi, bool pwr_wrn);

enum gps_mcudl_stat_get_result {
	GPS_MCUDL_STAT_NO_DATA,
	GPS_MCUDL_STAT_OK_NO_MORE,
	GPS_MCUDL_STAT_OK_MORE,
};

enum gps_mcudl_stat_get_user {
	GPS_MCUDL_STAT_GET_BY_TEST,
	GPS_MCUDL_STAT_GET_BY_MBRAIN,

	GPS_MCUDL_STAT_GET_USER_CNT
};

enum gps_mcudl_stat_get_reason {
	GPS_MCUDL_STAT_GET_FOR_TEST,
	GPS_MCUDL_STAT_GET_MBRAIN_PERIODIC,
	GPS_MCUDL_STAT_GET_MBRAIN_APP_SWITCH,
	GPS_MCUDL_STAT_GET_MBRAIN_AP_RESUME,
};

struct gps_mcudl_stat_lp_data {
	u64 dump_local_ms; /* from local_clock */
	u64 dump_ktime_ms; /* from ktime_get */

	u32 dump_index;
	u32 gnss_mcu_sid;
	bool gnss_mcu_is_on;
	bool gnss_pwr_is_hi;
	bool gnss_pwr_wrn;

	/* history statistic */
	u32 gnss_pwr_wrn_cnt;
};

struct gps_mcudl_stat_mcu_data {
	u32 gnss_mcu_sid;   /* last finished one */
	u32 clock_cfg_val;

	u64 open_local_ms;  /* from local_clock */
	u64 open_ktime_ms;  /* from ktime_get */
	u32 open_duration;

	u64 close_local_ms; /* from local_clock */
	u64 close_ktime_ms; /* from ktime_get */
	u32 close_duration;

	bool has_exception;
	bool force_close;

	/* history statistic */
	u32 open_duration_min;
	u32 open_duration_max;

	u32 close_duration_min;
	u32 close_duration_max;

	u32 exception_cnt;
	u32 force_close_cnt;
};

enum gps_mcudl_stat_get_result gps_mcudl_stat_get_lp_data(
	enum gps_mcudl_stat_get_user user,
	enum gps_mcudl_stat_get_reason reason, struct gps_mcudl_stat_lp_data *p);


enum gps_mcudl_stat_get_result gps_mcudl_stat_get_mcu_data(
	enum gps_mcudl_stat_get_user user,
	enum gps_mcudl_stat_get_reason reason, struct gps_mcudl_stat_mcu_data *p);

void gps_mcudl_stat_dump_lp_data(void);
void gps_mcudl_stat_lp_ctx_init(void);
void gps_mcudl_stat_lp_ctx_deinit(void);

void gps_mcudl_stat_dump_mcu_data(void);
void gps_mcudl_stat_mcu_ctx_init(void);
void gps_mcudl_stat_mcu_ctx_deinit(void);

#endif /* _GPS_MCUDL_HAL_STAT_H */

