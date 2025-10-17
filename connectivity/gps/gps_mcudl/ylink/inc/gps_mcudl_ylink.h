/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_YLINK_H
#define _GPS_MCUDL_YLINK_H


#include "gps_mcudl_data_intf_type.h"

enum gps_mcudl_ylink_event_id {
	GPS_MCUDL_YLINK_EVT_ID_RX_DATA_READY,
	GPS_MCUDL_YLINK_EVT_ID_SLOT_FLUSH_ON_RECV_STA,
	GPS_MCUDL_YLINK_EVT_ID_MCU_RESET_START,
	GPS_MCUDL_YLINK_EVT_ID_MCU_RESET_END,
	GPS_MCUDL_YLINK_EVT_ID_CCIF_ISR_ABNORMAL,
	GPS_MCUDL_YLINK_EVT_ID_MCU_SET_FW_OWN,
	GPS_MCUDL_YLINK_EVT_ID_CCIF_CLR_FW_OWN,
	GPS_MCUDL_YLINK_EVT_ID_TEST_CLR_FW_OWN,
	GPS_MCUDL_YLINK_EVT_ID_MCU_WDT_DUMP,
	GPS_MCUDL_YLINK_EVT_ID_AP_RESUME,
	GPS_MCUDL_YLINK_EVT_NUM
};

void gps_mcudl_ylink_event_send(enum gps_mcudl_yid y_id, enum gps_mcudl_ylink_event_id evt);

void gps_mcudl_ylink_event_proc(enum gps_mcudl_yid y_id, enum gps_mcudl_ylink_event_id evt);

void gps_mcudl_ylink_on_ap_resume(void);

gpsmdl_u32 gps_mcudl_ylink_get_xbitmask(enum gps_mcudl_yid y_id);
unsigned int gps_mcudl_hal_get_open_flag(void);

#endif /* _GPS_MCUDL_YLINK_H */

