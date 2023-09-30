/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_mcudl_xlink.h"

#if GPS_DL_ON_LINUX
/* Make sure num for RETURN_NAME_IN_LIST is const to detect coding error such
 * as swapping the position of num and index.
 * MASK_BE_CONST can be empty if compiler not support the macros used.
 */
#define MUST_BE_CONST(num) BUILD_BUG_ON(!__builtin_constant_p(num))
#else
#define MUST_BE_CONST(num)
#endif
#define NAME_ON_NULL "(NULL)"
#define RETURN_NAME_IN_LIST(list, num, index, retval)                          \
	do {                                                                   \
		MUST_BE_CONST(num);                                            \
		if (((index) >= 0) && ((index) < (num))) {                     \
			if ((list)[index])                                     \
				retval = (list)[index];                        \
			else {                                                 \
				GDL_LOGW("name is null for index: %d", index); \
				retval = NAME_ON_NULL;                         \
			}                                                      \
		} else {                                                       \
			GDL_LOGW("name index: %d out of range", index);        \
			retval = (list)[num];                                  \
		}                                                              \
	} while (0)


const char *const gpsmdl_xid_name_list[GPS_MDLX_CH_NUM + 1] = {
	[GPS_MDLX_MCUSYS] = "SYS",
	[GPS_MDLX_MCUFN]  = "XFN",
	[GPS_MDLX_MNL]    = "MNL",
	[GPS_MDLX_AGENT]  = "AGT",
	[GPS_MDLX_NMEA]   = "NMA",
	[GPS_MDLX_GDLOG]  = "GLG",
	[GPS_MDLX_PMTK]   = "PTK",
	[GPS_MDLX_MEAS]   = "MEA",
	[GPS_MDLX_CORR]   = "COR",
	[GPS_MDLX_DSP0]   = "DP0",
	[GPS_MDLX_DSP1]   = "DP1",
	[GPS_MDLX_GDLOG2] = "GL2",
	[GPS_MDLX_AOL_TEST] = "ALT",
	[GPS_MDLX_MPE_TEST] = "MPT",
	[GPS_MDLX_SCP_TEST] = "SCT",
	[GPS_MDLX_LPPM]     = "LPM",
	[GPS_MDLX_MPELOG]   = "MLG",
	[GPS_MDLX_MPELOG2]  = "ML2",
	[GPS_MDLX_CH_NUM] = "???",
};

const char *gpsmdl_xid_name(enum gps_mcudl_xid xid)
{
	const char *retval = NULL;

	RETURN_NAME_IN_LIST(gpsmdl_xid_name_list, GPS_MDLX_CH_NUM, xid, retval);
	return retval;
}

const char *const gps_mcudl_xlink_event_name_list[GPS_MCUDL_LINK_EVT_NUM + 1] = {
	[GPS_MCUDL_EVT_LINK_OPEN]  = "XLINK_OPEN",
	[GPS_MCUDL_EVT_LINK_CLOSE] = "XLINK_CLOSE",
	[GPS_MCUDL_EVT_LINK_WRITE] = "XLINK_WRITE",
	[GPS_MCUDL_EVT_LINK_READ]  = "XLINK_READ",
	[GPS_MCUDL_EVT_LINK_RESET] = "XLINK_RESET",
	[GPS_MCUDL_EVT_LINK_RESET2] = "XLINK_RESET2",
	[GPS_MCUDL_EVT_LINK_PRE_CONN_RESET]  = "PRE_CONN_RESET",
	[GPS_MCUDL_EVT_LINK_POST_CONN_RESET] = "POST_CONN_RESET",
	[GPS_MCUDL_EVT_LINK_UPDATE_SETTING]  = "UPDATE_SETTING",
	[GPS_MCUDL_EVT_LINK_PRINT_HW_STATUS] = "PRINT_HW_STATUS",
	[GPS_MCUDL_EVT_LINK_PRINT_DATA_STATUS] = "PRINT_DATA_STATUS",
	[GPS_MCUDL_EVT_LINK_FW_LOG_ON]       = "FW_LOG_ON",
	[GPS_MCUDL_EVT_LINK_FW_LOG_OFF]      = "FW_LOG_OFF",
	[GPS_MCUDL_LINK_EVT_NUM]             = "XLINK_INVALID_EVT"
};

const char *gps_mcudl_xlink_event_name(enum gps_mcudl_xlink_event_id event)
{
	const char *retval = NULL;

	RETURN_NAME_IN_LIST(gps_mcudl_xlink_event_name_list,
		GPS_MCUDL_LINK_EVT_NUM, event, retval);
	return retval;
}

