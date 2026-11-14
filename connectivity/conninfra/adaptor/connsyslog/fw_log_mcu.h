/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _FW_LOG_MCU_H_
#define _FW_LOG_MCU_H_

#include <linux/cdev.h>
#include <linux/device.h>
#include "connsyslog_to_user.h"
#include "connsyslog_macro.h"

struct fw_log_mcu_info {
	int conn_type;
	char *driver_name;
	dev_t dev_id;
	struct cdev mcu_cdev;
	struct class *mcu_class;
	struct device *mcu_dev;
	wait_queue_head_t wq;
	struct connlog_to_user_cb callback;
};


#define CONN_FW_LOG_SYS_LIST(F) \
F(wifi) \
F(bt) \
F(gps)

CONN_FW_LOG_SYS_LIST(DECLARE_FW_LOG_SYS)

int fw_log_mcu_init(struct fw_log_mcu_info *info, const struct file_operations *ops);
void fw_log_mcu_deinit(struct fw_log_mcu_info *info);
int fw_log_mcu_open(struct inode *inode, struct file *file);
int fw_log_mcu_close(struct inode *inode, struct file *file);
int fw_log_mcu_register_event_cb(struct fw_log_mcu_info *info);

#endif /*_FW_LOG_MCU_H_*/
