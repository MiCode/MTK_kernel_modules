/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_mcudl_ylink.h"

void gps_mcudl_ylink_on_ap_resume(void)
{
	gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_AP_RESUME);
}

