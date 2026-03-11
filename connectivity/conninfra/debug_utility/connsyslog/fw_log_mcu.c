/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/poll.h>
#include "connsys_debug_utility.h"
#include "fw_log_mcu.h"

int fw_log_mcu_open(struct inode *inode, struct file *file)
{
	pr_info("[%s] major %d minor %d (pid %d)\n",
		__func__, imajor(inode), iminor(inode), current->pid);
	return 0;
}

int fw_log_mcu_close(struct inode *inode, struct file *file)
{
	pr_info("[%s] major %d minor %d (pid %d)\n",
		__func__, imajor(inode), iminor(inode), current->pid);
	return 0;
}

int fw_log_mcu_register_event_cb(struct fw_log_mcu_info *info)
{
	int ret = 0;

	ret = connsys_log_register_event_cb(info->conn_type,
		info->mcu_event_cb);
	if (ret)
		pr_notice("[%s] Fail to register connsys_log_register_event_cb, conn_type=[%d]\n",
			__func__, info->conn_type);

	return ret;
}

int fw_log_mcu_init(struct fw_log_mcu_info *info, const struct file_operations *ops)
{
	int cdevErr = -1;
	int ret = -1;

	ret = alloc_chrdev_region(&info->dev_id, 0, 1, info->driver_name);
	if (ret)
		pr_notice("[%s] Fail to alloc_chrdev_region, conn_type=[%d]\n", __func__, info->conn_type);

	cdev_init(&info->mcu_cdev, ops);
	info->mcu_cdev.owner = THIS_MODULE;

	cdevErr = cdev_add(&info->mcu_cdev, info->dev_id, 1);
	if (cdevErr) {
		pr_notice("[%s] cdev_add fail (%d), conn_type=[%d]\n", __func__, cdevErr, info->conn_type);
		goto error;
	}

	info->mcu_class = class_create(THIS_MODULE, info->driver_name);
	if (IS_ERR(info->mcu_class)) {
		pr_notice("[%s] class_create fail, conn_type=[%d]\n", __func__, info->conn_type);
		goto error;
	}

	info->mcu_dev = device_create(info->mcu_class, NULL, info->dev_id, NULL, info->driver_name);
	if (IS_ERR(info->mcu_dev)) {
		pr_notice("[%s] device_create fail, conn_type=[%d]\n", __func__, info->conn_type);
		goto error;
	}

	init_waitqueue_head(&info->wq);

	pr_info("[%s] conn_type=[%d] major=[%d] init done\n",
		__func__, info->conn_type, MAJOR(info->dev_id));

	return 0;

error:
	if (!(IS_ERR(info->mcu_dev)))
		device_destroy(info->mcu_class, info->dev_id);
	if (!(IS_ERR(info->mcu_class))) {
		class_destroy(info->mcu_class);
		info->mcu_class = NULL;
	}

	if (cdevErr == 0)
		cdev_del(&info->mcu_cdev);
	if (ret == 0)
		unregister_chrdev_region(info->dev_id, 1);
	pr_notice("fw_log_mcu_init fail, conn_type=[%d]\n", info->conn_type);
	return -1;
}
EXPORT_SYMBOL(fw_log_mcu_init);

void fw_log_mcu_deinit(struct fw_log_mcu_info *info)
{
	if (info->mcu_dev) {
		device_destroy(info->mcu_class, info->dev_id);
		info->mcu_dev = NULL;
	}

	if (info->mcu_class) {
		class_destroy(info->mcu_class);
		info->mcu_class = NULL;
	}

	cdev_del(&info->mcu_cdev);
	unregister_chrdev_region(info->dev_id, 1);
	pr_info("[%s] conn_type=[%d] major=[%d] deinit done\n",
		__func__, info->conn_type, MAJOR(info->dev_id));
}
EXPORT_SYMBOL(fw_log_mcu_deinit);
#endif
