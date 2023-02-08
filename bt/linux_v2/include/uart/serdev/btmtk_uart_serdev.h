/* SPDX-License-Identifier: GPL-2.0 */  
/*
 * Copyright (c) 2016,2017 MediaTek Inc.
 */

#ifndef _BTMTK_UART_H_
#define _BTMTK_UART_H_
#include <linux/serdev.h>
#include "btmtk_define.h"
#include "btmtk_main.h"
#include "btmtk_buffer_mode.h"

#ifndef UART_DEBUG
#define UART_DEBUG 0
#endif

/**
 * Card-relate definition.
 */
#define HCI_HEADER_LEN	4

#define MTK_STP_TLR_SIZE	2
#define STP_HEADER_LEN	4
#define STP_HEADER_CRC_LEN	2
#define HCI_MAX_COMMAND_SIZE	255
#define MAX_BUFFER_SIZE	(4*1024)

/* CMD&Event sent by driver */
#define READ_REGISTER_CMD_LEN		16
#define READ_REGISTER_EVT_HDR_LEN		11

/* MCU address offset */
#define MCU_ADDRESS_OFFSET_CMD 12
#define MCU_ADDRESS_OFFSET_EVT 16

typedef int (*pdwnc_func) (u8 fgReset);
typedef int (*reset_func_ptr2) (unsigned int gpio, int init_value);
typedef int (*set_gpio_low)(u8 gpio);
typedef int (*set_gpio_high)(u8 gpio);

/**
 * Send cmd dispatch evt
 */
#define HCI_EV_VENDOR			0xff

#define READ_ADDRESS_EVT_HDR_LEN 7
#define READ_ADDRESS_EVT_PAYLOAD_OFFSET 7
#define WOBLE_DEBUG_EVT_TYPE 0xE8

#define LD_PATCH_CMD_LEN 10
#define LD_PATCH_EVT_LEN 8

struct btmtk_uart_dev {
	struct serdev_device *serdev;
	struct clk *clk;
	struct clk *osc;
	unsigned char	*transfer_buf;
};
#endif
