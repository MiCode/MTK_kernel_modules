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
			if (!gps_mcudl_each_link_get_bool_flag(x_id, LINK_MISS_MNLBIN_ACK))
				continue;

			if (x_id == GPS_MDLX_LPPM) {
				bool scif_ready = gps_mcusys_scif_is_ready();
				bool miss_mnlbin = false;

				if (!scif_ready) {
					miss_mnlbin = gps_mcudl_each_link_get_bool_flag(
						x_id, LINK_MISS_MNLBIN_ACK);
					if (miss_mnlbin) {
						gps_mcudl_each_link_set_bool_flag(
							x_id, LINK_MISS_MNLBIN_ACK, false);
					}
					MDL_LOGXI(x_id,
						"bypass gps_mcudl_link_open_ack, miss_mnl=%d, scif=%d",
						miss_mnlbin, scif_ready);
					continue;
				}
			}
			MDL_LOGXI(x_id, "gps_mcudl_link_open_ack");
			gps_mcudl_link_open_ack(x_id, true);
			if (x_id == GPS_MDLX_LPPM)
				gps_mcusys_scif_set_lppm_open_ack_done(true);
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
		gps_mcusys_scif_clr_ready();
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

enum gps_mcusys_gpsbin_state gps_mcusys_gpsbin_state_get(void)
{
	return g_gps_mcusys_gpsbin_state_host;
}

bool gps_mcusys_gpsbin_state_is(enum gps_mcusys_gpsbin_state state)
{
	return (g_gps_mcusys_gpsbin_state_host == state);
}


bool g_gps_mcusys_scif_ready;
bool g_gps_mcusys_scif_lppm_open_ack_done;

void gps_mcusys_scif_set_ready(unsigned int ready_tick)
{
	bool miss_mnlbin = gps_mcudl_each_link_get_bool_flag(
		GPS_MDLX_LPPM, LINK_MISS_MNLBIN_ACK);
	bool open_ack_done = g_gps_mcusys_scif_lppm_open_ack_done;
	bool last_ready = g_gps_mcusys_scif_ready;

	/* Bypass open_ack case discussion:
	 *   miss_mnlbin=true, open_ack in gps_mcusys_mnlbin_fsm GPS_MCUSYS_MNLBIN_CTLR_CREATED
	 *   open_ack_done=true, no need because already done
	 *
	 *   last_ready=true, it means there may be a SCP reset,
	 *   in this case open_ack should be done in:
	 *      - gps_mcusys_mnlbin_fsm GPS_MCUSYS_MNLBIN_CTLR_CREATED or
	 *      - gps_mcudl_xlink_event_proc GPS_MCUDL_EVT_LINK_OPEN,
	 *   due to g_gps_mcusys_scif_ready is true
	 *
	 * Once any of them is true, no need to do open_ack here.
	 */
	if (!miss_mnlbin && !open_ack_done && !last_ready) {
		gps_mcudl_link_open_ack(GPS_MDLX_LPPM, true);
		gps_mcusys_scif_set_lppm_open_ack_done(true);
	}
	g_gps_mcusys_scif_ready = true;
	MDL_LOGI("scif_ready, tick=%u, miss_mnl=%d, pre_ack=%d, last_ready=%d",
		ready_tick, miss_mnlbin, open_ack_done, last_ready);
}

void gps_mcusys_scif_clr_ready(void)
{
	g_gps_mcusys_scif_ready = false;
}

bool gps_mcusys_scif_is_ready(void)
{
	return g_gps_mcusys_scif_ready;
}

void gps_mcusys_scif_set_lppm_open_ack_done(bool done)
{
	g_gps_mcusys_scif_lppm_open_ack_done = done;
}

