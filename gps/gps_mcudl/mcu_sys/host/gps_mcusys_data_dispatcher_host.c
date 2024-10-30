/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_log.h"

#include "gps_mcusys_data_api.h"
#include "gps_mcusys_fsm.h"


void gps_mcusys_data_frame_proc(const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len)
{
	struct gps_mcusys_data_sync_frame frame;
	gpsmdl_u16 copy_len;
	gpsmdl_u16 frame_size = sizeof(frame);

	copy_len = payload_len;
	if (copy_len > frame_size) {
		copy_len = frame_size;
		memcpy(&frame, payload_ptr, copy_len);
		GDL_LOGW("type=%d, data_size=%d, sizeof(frame)=%d, recv_len=%d is too long!",
			frame.type, frame.data_size, frame_size, payload_len);
	} else {
		memcpy(&frame, payload_ptr, copy_len);
		GDL_LOGI("type=%d, data_size=%d, sizeof(frame)=%d, recv_len=%d",
			frame.type, frame.data_size, frame_size, payload_len);
	}

	/* TODO: check frame.data_size with copy_len */
	switch (frame.type) {
	case GPS_MCUSYS_DST_MNLBIN_EVENT:
		/* TODO: enable the check */
		/*if (frame.data_size < sizeof(frame.data.mnlbin_event))*/
		/*	break;*/
		gps_mcusys_mnlbin_fsm(frame.data.mnlbin_event);
		break;

	case GPS_MCUSYS_DST_NVLOCK_EVENT:
		gps_mcusys_nvlock_fsm(
			frame.data.nvlock_event.nv_id,
			frame.data.nvlock_event.evt_id);
		break;

	case GPS_MCUSYS_DST_NVDATA_EVENT:
		gps_mcusys_nvdata_on_remote_event(
			frame.data.nvdata_event.nv_id,
			frame.data.nvdata_event.evt_id);
		break;

	case GPS_MCUSYS_DST_SCIF_READY_EVENT:
		gps_mcusys_scif_set_ready(frame.data.scif_ready_tick);
		break;

	default:
		break;
	}
}

