/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _PLATFORM_MT6893_CONNSYSLOG_H_
#define _PLATFORM_MT6893_CONNSYSLOG_H_

#include "connsyslog_hw_config.h"
#include "connsys_debug_utility.h"

#ifdef CONFIG_FPGA_EARLY_PORTING
#define CONNLOG_EMI_OFFSET_WIFI 0x0001F000
#define CONNLOG_EMI_OFFSET_BT   0x0002F000
#else
#define CONNLOG_EMI_OFFSET_WIFI 0x0024F000
#define CONNLOG_EMI_OFFSET_BT   0x0004b000
#endif

#define CONNLOG_EMI_SIZE_WIFI   (192*1024)
#define CONNLOG_EMI_SIZE_BT (64*1024)

static struct connlog_emi_config g_connsyslog_config[CONN_DEBUG_TYPE_END] = {
    /* Wi-Fi config */
    {CONNLOG_EMI_OFFSET_WIFI, CONNLOG_EMI_SIZE_WIFI},
    {CONNLOG_EMI_OFFSET_BT, CONNLOG_EMI_SIZE_BT},
};

#endif /* _PLATFORM_MT6893_CONNSYSLOG_H_ */
