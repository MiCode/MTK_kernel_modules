/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_log.h"
#include "gps_mcudl_xlink.h"
#include "gps_mcudl_link_sync.h"
#include "gps_mcudl_link_state.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcusys_fsm.h"
#include "gps_mcusys_nv_data_api.h"
#include "gps_mcusys_data_sync2target.h"


static enum gps_mcusys_mnlbin_state g_gps_mcusys_mnlbin_state_host;
void gps_mcusys_mnlbin_state_change_to(enum gps_mcusys_mnlbin_state state)
{
	enum gps_mcusys_mnlbin_state old_state;

	old_state = g_gps_mcusys_mnlbin_state_host;
	g_gps_mcusys_mnlbin_state_host = state;
	MDL_LOGD("%d -> %d", old_state, state);
}

enum gps_mcusys_mnlbin_state gps_mcusys_mnlbin_state_get(void)
{
	return g_gps_mcusys_mnlbin_state_host;
}

bool gps_mcusys_mnlbin_state_is(enum gps_mcusys_mnlbin_state state)
{
	return (g_gps_mcusys_mnlbin_state_host == state);
}

void gps_mcusys_mnlbin_fsm(enum gps_mcusys_mnlbin_event evt)
{
	enum gps_mcudl_xid x_id;
	enum gps_mcusys_mnlbin_state old_state;

	old_state = gps_mcusys_mnlbin_state_get();
	switch (evt) {
	case GPS_MCUSYS_MNLBIN_SYS_ON:
		gps_mcusys_mnlbin_state_change_to(GPS_MCUSYS_MNLBIN_ST_UNINIT);
		break;
	case GPS_MCUSYS_MNLBIN_INIT_DONE:
		gps_mcusys_mnlbin_state_change_to(GPS_MCUSYS_MNLBIN_ST_INIT_DONE);
		break;
	case GPS_MCUSYS_MNLBIN_CTLR_CREATED:
		for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++) {
			if (gps_mcudl_each_link_get_state(x_id) != LINK_OPENING)
				continue;
			MDL_LOGXI(x_id, "gps_mcudl_link_open_ack");
			gps_mcudl_link_open_ack(x_id, true);
		}
		gps_mcusys_mnlbin_state_change_to(GPS_MCUSYS_MNLBIN_ST_CTLR_CREATED);
		break;
	case GPS_MCUSYS_MNLBIN_SYS_RESET_START:
		if (gps_mcusys_mnlbin_state_is(GPS_MCUSYS_MNLBIN_ST_RESETTING)) {
			MDL_LOGI("GPS_MCUSYS_MNLBIN_ST_RESETTING, direct break");
			break;
		}

		for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++) {
			if (gps_mcudl_each_link_get_state(x_id) != LINK_OPENING)
				continue;
			MDL_LOGXI(x_id, "gps_mcudl_link_open_ack by reset_start");
			gps_mcudl_link_open_ack(x_id, false);
		}
		gps_mcusys_mnlbin_state_change_to(GPS_MCUSYS_MNLBIN_ST_RESETTING);
		break;
	case GPS_MCUSYS_MNLBIN_SYS_RESET_END:
		if (!gps_mcusys_mnlbin_state_is(GPS_MCUSYS_MNLBIN_ST_RESETTING)) {
			MDL_LOGI("Not GPS_MCUSYS_MNLBIN_SYS_RESET_END, direct break");
			break;
		}
		gps_mcusys_mnlbin_state_change_to(GPS_MCUSYS_MNLBIN_ST_UNINIT);
		break;
	default:
		break;
	}

	MDL_LOGI("evt=%d, state:%d -> %d", evt, old_state, gps_mcusys_mnlbin_state_get());
}

static enum gps_mcusys_gpsbin_state g_gps_mcusys_gpsbin_state_host;
void gps_mcusys_gpsbin_state_set(enum gps_mcusys_gpsbin_state state)
{
	enum gps_mcusys_gpsbin_state old_state;

	old_state = g_gps_mcusys_gpsbin_state_host;
	switch (state) {
	case GPS_MCUSYS_GPSBIN_PRE_ON:
		gps_mcusys_nv_data_on_gpsbin_state(state);
		break;
	case GPS_MCUSYS_GPSBIN_POST_ON:
		gps_mcusys_nv_data_on_gpsbin_state(state);
		gps_mcusys_data_sync2target_host_status_cmd_post_on();
		break;
	case GPS_MCUSYS_GPSBIN_PRE_OFF:
		gps_mcusys_nv_data_on_gpsbin_state(state);
		break;
	case GPS_MCUSYS_GPSBIN_POST_OFF:
		gps_mcusys_nv_data_on_gpsbin_state(state);
		break;
	default:
		break;
	}
	g_gps_mcusys_gpsbin_state_host = state;
	MDL_LOGI("state:%d -> %d", old_state, state);
}

bool gps_mcusys_gpsbin_state_is(enum gps_mcusys_gpsbin_state state)
{
	return (g_gps_mcusys_gpsbin_state_host == state);
}

