/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _PLATFORM_MT6877_CONNSYSLOG_H_
#define _PLATFORM_MT6877_CONNSYSLOG_H_

#include "connsyslog_hw_config.h"
#include "connsys_debug_utility.h"

#define CONNLOG_EMI_OFFSET_WIFI 0x00465400
#define CONNLOG_EMI_OFFSET_BT   0x00033c00

#define CONNLOG_EMI_SIZE_WIFI       (192*1024)
#define CONNLOG_EMI_SIZE_BT         (64*1024)

#define CONNLOG_EMI_BLOCK_WIFI      (128*1024)
#define CONNLOG_EMI_BLOCK_WIFI_MCU  (32*1024)

#define CONNLOG_EMI_BLOCK_BT        (32*1024)
#define CONNLOG_EMI_BLOCK_BT_MCU    (16*1024)

static struct connlog_emi_config g_connsyslog_config[CONN_DEBUG_TYPE_END] = {
	/* Wi-Fi config */
	{CONNLOG_EMI_OFFSET_WIFI, CONNLOG_EMI_SIZE_WIFI,
		{
			{CONNLOG_EMI_BLOCK_WIFI},
			{CONNLOG_EMI_BLOCK_WIFI_MCU}
		}
	},
	{CONNLOG_EMI_OFFSET_BT, CONNLOG_EMI_SIZE_BT,
		{
			{CONNLOG_EMI_BLOCK_BT},
			{CONNLOG_EMI_BLOCK_BT_MCU}
		}
	},
};

#endif /* _PLATFORM_MT6877_CONNSYSLOG_H_ */
