/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcusys_data_api.h"
#include "gps_mcusys_data_sync2target.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcudl_ylink.h"


void gps_mcusys_data_send_frame(struct gps_mcusys_data_sync_frame *p_frame)
{
	bool to_notify;
	enum gps_mcudl_yid yid;

	yid = GPS_MDLY_NORMAL;

	/* TODO: send_len = &p_frame->data - p_frame + p_frame->data_size */
	gps_mcudl_ap2mcu_ydata_send(yid, GPS_MDLYPL_MCUSYS,
		(gpsmdl_u8 *)p_frame, (gpsmdl_u32)(sizeof(*p_frame)));
	to_notify = !gps_mcudl_ap2mcu_get_wait_flush_flag(yid);
	if (to_notify) {
		gps_mcudl_ap2mcu_set_wait_flush_flag(yid, true);
		gps_mcudl_ylink_event_send(yid,
			GPS_MCUDL_YLINK_EVT_ID_SLOT_FLUSH_ON_RECV_STA);
	}
}

#if 0
void gps_mcusys_data_sync2host_mnlbin_event(enum gps_mcusys_mnlbin_event evt)
{
	gps_mcusys_data_sync_frame frame;

	frame.type = GPS_MCUSYS_DST_MNLBIN_EVENT;
	frame.data.mnlbin_event = evt;
	frame.data_size = sizeof(evt);
	gps_mcusys_data_frame_send(&frame);
}
#endif

void gps_mcusys_data_sync2target_host_status_cmd_post_on(void)
{
	struct gps_mcusys_data_sync_frame frame;

	frame.type = GPS_MCUSYS_DST_HOST_STATUS_COMMAND;
	frame.data.host_status_cmd.cmd_id = GPS_MCUSYS_HOST_STATUS_POST_ON;
	frame.data.host_status_cmd.xstatus_bitmask = 0;
	frame.data_size = sizeof(frame.data.host_status_cmd);
	gps_mcusys_data_send_frame(&frame);
}

void gps_mcusys_data_sync2target_lpp_mode_status_cmd(int op)
{
	struct gps_mcusys_data_sync_frame frame;

	frame.type = GPS_MCU_DST_HOST_LPPMODE_STATUS_CMD;
	if (op == 1)
		frame.data.lpp_mode_status_cmd = true;
	else
		frame.data.lpp_mode_status_cmd = false;
	frame.data_size = sizeof(frame.data.lpp_mode_status_cmd);
	gps_mcusys_data_send_frame(&frame);
}
