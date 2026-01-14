/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _GPS_MCUSYS_DATA_WRAPPER_H_
#define _GPS_MCUSYS_DATA_WRAPPER_H_

#include "gps_mcudl_data_intf_type.h"
#include "gps_mcusys_data.h"

enum gps_mcusys_data_sync_type {
	GPS_MCUSYS_DST_MNLBIN_EVENT,
	GPS_MCUSYS_DST_NVLOCK_EVENT,
	GPS_MCUSYS_DST_NVDATA_EVENT,
	GPS_MCUSYS_DST_HOST_STATUS_COMMAND,
	GPS_MCU_DST_HOST_LPPMODE_STATUS_CMD,
	GPS_MCUSYS_DST_SCIF_READY_EVENT,    /* 5 */
	GPS_MCUSYS_DST_1ST_TIMER_EVENT,     /* 6 */
	GPS_MCUSYS_DST_NUM
};

#if 0
enum gps_mcusys_data_sync_dir {
	GPS_MCUSYS_DIR_H2M,
	GPS_MCUSYS_DIR_M2H,

	GPS_MCUSYS_DIR_NUM
};
#endif

union gps_mcusys_data_sync_content {
	enum gps_mcusys_mnlbin_event mnlbin_event;
	struct gps_mcusys_nvlock_event nvlock_event;
	struct gps_mcusys_nvdata_event nvdata_event;
	struct gps_mcusys_host_status_cmd host_status_cmd;
	bool lpp_mode_status_cmd;
	unsigned int scif_ready_tick;
};

struct gps_mcusys_data_sync_frame {
	enum gps_mcusys_data_sync_type type;
#if 0
	enum  gps_mcusys_data_sync_dir     dir;
#endif
	gpsmdl_u16 data_size;
	union gps_mcusys_data_sync_content data;
};

#endif /* _GPS_MCUSYS_DATA_WRAPPER_H_ */

