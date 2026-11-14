/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _CONNV3_MCU_LOG_H_
#define _CONNV3_MCU_LOG_H_

#include <linux/types.h>
#include <linux/compiler.h>
#include "connv3_debug_utility.h"

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
/* Close debug log */
//#define DEBUG_LOG_ON 1

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
enum FW_LOG_MODE {
	PRINT_TO_KERNEL_LOG = 0,
	LOG_TO_FILE = 1,
};

enum connv3_log_type {
	CONNV3_LOG_TYPE_PRIMARY,
	CONNV3_LOG_TYPE_MCU,
	CONNV3_LOG_TYPE_SIZE
};

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

void connv3_log_set_log_mode(int mode);
int connv3_log_get_log_mode(void);

/* For subsys */
int connv3_log_init(int conn_type, int primary_buf_size, int mcu_buf_size,
			void (*log_event_cb)(void));
int connv3_log_deinit(int conn_type);

u32 connv3_log_handler(int conn_type, enum connv3_log_type type, u8 *buf, u32 size);

unsigned int connv3_log_get_buf_size(int conn_type);
ssize_t connv3_log_read_to_user(int conn_type, char __user *buf, size_t count);
ssize_t connv3_log_read(int conn_type, char *buf, size_t count);


#endif /* _CONNV3_MCU_LOG_H_ */
