/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef MT6897_CONNSYSLOG_H
#define MT6897_CONNSYSLOG_H

#include "connsys_debug_utility.h"
#include "connsyslog_hw_config.h"

#ifdef CONFIG_FPGA_EARLY_PORTING
#define CONNLOG_EMI_OFFSET_WIFI 0x000c8000
#define CONNLOG_EMI_OFFSET_BT   0x0002F000
#else
#define CONNLOG_EMI_OFFSET_WIFI 0x00b00000
#define CONNLOG_EMI_OFFSET_BT   0x00b48000

#endif

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

#endif /* MT6897_CONNSYSLOG_H */
