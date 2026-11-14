/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef _CONNV3_DEBUG_UTILITY_H_
#define _CONNV3_DEBUG_UTILITY_H_

#include <linux/types.h>
#include <linux/compiler.h>

#include "coredump/connv3_coredump.h"
#include "connsyslog/connv3_mcu_log.h"

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
// Only support WIFI and BT for connsyslog init
//#define CONN_DEBUG_PRIMARY_END 2

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

enum CONNV3_DEBUG_TYPE {
	CONNV3_DEBUG_TYPE_WIFI = 0,
	CONNV3_DEBUG_TYPE_BT = 1,
	CONNV3_DEBUG_TYPE_SIZE,
};

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/


#endif /*_CONNV3_DEBUG_UTILITY_H_*/
