/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUSYS_FSM_H_
#define _GPS_MCUSYS_FSM_H_

#include "gps_mcudl_data_intf_type.h"
#include "gps_mcusys_data.h"

enum gps_mcusys_mnlbin_state {
	GPS_MCUSYS_MNLBIN_ST_UNINIT,
	GPS_MCUSYS_MNLBIN_ST_INIT_DONE,
	GPS_MCUSYS_MNLBIN_ST_CTLR_CREATED,
	GPS_MCUSYS_MNLBIN_ST_RESETTING,
	GPS_MCUSYS_MNLBIN_ST_NUM
};

enum gps_mcusys_mnlbin_state gps_mcusys_mnlbin_state_get(void);
void gps_mcusys_mnlbin_state_change_to(enum gps_mcusys_mnlbin_state state);
bool gps_mcusys_mnlbin_state_is(enum gps_mcusys_mnlbin_state state);
void gps_mcusys_mnlbin_fsm(enum gps_mcusys_mnlbin_event evt);


enum gps_mcusys_gpsbin_state {
	GPS_MCUSYS_GPSBIN_PRE_ON,
	GPS_MCUSYS_GPSBIN_POST_ON,
	GPS_MCUSYS_GPSBIN_PRE_OFF,
	GPS_MCUSYS_GPSBIN_POST_OFF,
};

enum gps_mcusys_gpsbin_state gps_mcusys_gpsbin_state_get(void);
void gps_mcusys_gpsbin_state_set(enum gps_mcusys_gpsbin_state state);
bool gps_mcusys_gpsbin_state_is(enum gps_mcusys_gpsbin_state state);

void gps_mcusys_nvlock_fsm(enum gps_mcusys_nv_data_id nv_id,
	enum gps_mcusys_nvlock_event_id lock_evt);

void gps_mcusys_nvdata_on_local_event(enum gps_mcusys_nv_data_id nv_id,
	enum gps_mcusys_nvdata_event_id data_evt);
void gps_mcusys_nvdata_on_remote_event(enum gps_mcusys_nv_data_id nv_id,
	enum gps_mcusys_nvdata_event_id data_evt);

void gps_mcusys_scif_set_ready(unsigned int ready_tick);
void gps_mcusys_scif_clr_ready(void);
bool gps_mcusys_scif_is_ready(void);
void gps_mcusys_scif_set_lppm_open_ack_done(bool done);

#endif /* _GPS_MCUSYS_FSM_H_ */

