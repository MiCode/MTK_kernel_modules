/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _FW_LOG_MCU_H_
#define _FW_LOG_MCU_H_

struct fw_log_mcu_info {
	int conn_type;
	char *driver_name;
	dev_t dev_id;
	struct cdev mcu_cdev;
	struct class *mcu_class;
	struct device *mcu_dev;
	wait_queue_head_t wq;
	void (*mcu_event_cb) (void);
};

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
int fw_log_mcu_init(struct fw_log_mcu_info *info, const struct file_operations *ops);
void fw_log_mcu_deinit(struct fw_log_mcu_info *info);
int fw_log_mcu_open(struct inode *inode, struct file *file);
int fw_log_mcu_close(struct inode *inode, struct file *file);
int fw_log_mcu_register_event_cb(struct fw_log_mcu_info *info);
#endif

#endif /*_FW_LOG_MCU_H_*/
