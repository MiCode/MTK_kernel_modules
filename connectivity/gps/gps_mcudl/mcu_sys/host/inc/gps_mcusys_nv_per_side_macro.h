/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _GPS_MCUSYS_NV_PER_SIDE_MACRO_H
#define _GPS_MCUSYS_NV_PER_SIDE_MACRO_H

/* I'm on AP side */
#include "gps_dl_log.h"
#include "gps_dl_time_tick.h"


#define NV_IS_ON_MCU (false)
#define GPS_OFL_TRC GDL_LOGI
#define GPS_OFL_DBG GDL_LOGD

#define GPSMDL_PLAT_TICK_TYPE  unsigned long
#define GPSMDL_PLAT_TICK_GET() gps_dl_tick_get_us()
#define GPSMDL_PLAT_TICK_UNIT_STR "us"

#endif /* _GPS_MCUSYS_NV_PER_SIDE_MACRO_H */

