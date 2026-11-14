/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2025 MediaTek Inc.
 */

#ifndef _GPS_DEF_H_
#define _GPS_DEF_H_

#if IS_ENABLED(CONFIG_GPS_DL_HAS_MOCK)
#define GPS_DL_HAS_MOCK 1
#endif

#define MTK_GENERIC_HAL

#define GPS_DL_HAS_CONNINFRA_DRV 1

#if IS_ENABLED(CONFIG_GPSMDL)
#define GPS_DL_HAS_MCUDL 1

#if IS_ENABLED(CONFIG_GPSMDL_HAL)
#define GPS_DL_HAS_MCUDL_HAL 1
#define GPS_DL_HAS_MCUDL_HAL_STAT 1

#if IS_ENABLED(CONFIG_MTK_MBRAINK_BRIDGE)
#define GPS_DL_HAS_MCUDL_IF_WITH_MBRAIN 1
#endif/*CONFIG_MTK_MBRAINK_BRIDGE*/

#else
#define GPS_DL_HAS_MCUDL_HAL 0
#endif/*CONFIG_GPSMDL_HAL*/

#if IS_ENABLED(CONFIG_GPSMDL_HW)
#define GPS_DL_HAS_MCUDL_HW 1
#else
#define GPS_DL_HAS_MCUDL_HW 0
#endif /*CONFIG_GPSMDL_HW*/

#if IS_ENABLED(CONFIG_GPSMDL_FW)
#define GPS_DL_HAS_MCUDL_FW 1
#else
#define GPS_DL_HAS_MCUDL_FW 0
#endif /*CONFIG_GPSMDL_FW*/

#endif /*CONFIG_GPSMDL*/

#endif /* _GPS_DEF_H_ */
