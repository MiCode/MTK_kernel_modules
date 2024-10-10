/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef MT6989_CONNSYSLOG_H
#define MT6989_CONNSYSLOG_H

#include "connsys_debug_utility.h"
#include "connsyslog_hw_config.h"

#define CONNLOG_EMI_OFFSET_GPS 0x00490000

#define CONNLOG_EMI_SIZE_GPS         (96*1024)
#define CONNLOG_EMI_BLOCK_GPS        (64*1024)
#define CONNLOG_EMI_BLOCK_GPS_MCU    (16*1024)

static struct connlog_emi_config g_connsyslog_config_mt6989[CONN_DEBUG_TYPE_END] = {
	/* Wi-Fi config */
	{0, 0,
		{
			{0},
			{0}
		}
	},
	{0, 0,
		{
			{0},
			{0}
		}
	},
	{CONNLOG_EMI_OFFSET_GPS, CONNLOG_EMI_SIZE_GPS,
		{
			{CONNLOG_EMI_BLOCK_GPS},
			{CONNLOG_EMI_BLOCK_GPS_MCU}
		}
	},
};

#endif /* MT6989_CONNSYSLOG_H */
