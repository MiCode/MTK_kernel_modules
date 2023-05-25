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
#include "fw_log_wifi_mcu.h"
#include "fw_log_mcu.h"

#define DRIVER_NAME "fw_log_wifimcu"
struct fw_log_mcu_info g_wifi_mcu_info;

static ssize_t fw_log_wifi_mcu_read(struct file *filp, char __user *buf,
	size_t count, loff_t *f_pos)
{
	return connsys_log_read_to_user(CONN_DEBUG_TYPE_WIFI_MCU, buf, count);
}

static unsigned int fw_log_wifi_mcu_poll(struct file *filp, poll_table *wait)
{
	poll_wait(filp, &g_wifi_mcu_info.wq, wait);

	if (connsys_log_get_buf_size(CONN_DEBUG_TYPE_WIFI_MCU) > 0)
		return POLLIN | POLLRDNORM;

	return 0;
}

const struct file_operations g_log_wifi_mcu_ops = {
	.open = fw_log_mcu_open,
	.release = fw_log_mcu_close,
	.read = fw_log_wifi_mcu_read,
	.poll = fw_log_wifi_mcu_poll,
};

static void fw_log_wifi_mcu_event_cb(void)
{
	wake_up_interruptible(&g_wifi_mcu_info.wq);
}

int fw_log_wifi_mcu_register_event_cb(void)
{
	return fw_log_mcu_register_event_cb(&g_wifi_mcu_info);
}
EXPORT_SYMBOL(fw_log_wifi_mcu_register_event_cb);

int fw_log_wifi_mcu_init(void)
{
	int ret = 0;

	g_wifi_mcu_info.conn_type = CONN_DEBUG_TYPE_WIFI_MCU;
	g_wifi_mcu_info.driver_name = DRIVER_NAME;
	g_wifi_mcu_info.mcu_event_cb = fw_log_wifi_mcu_event_cb;
	ret = fw_log_mcu_init(&g_wifi_mcu_info, &g_log_wifi_mcu_ops);

	return ret;
}
EXPORT_SYMBOL(fw_log_wifi_mcu_init);

void fw_log_wifi_mcu_deinit(void)
{
	fw_log_mcu_deinit(&g_wifi_mcu_info);
}
EXPORT_SYMBOL(fw_log_wifi_mcu_deinit);
#endif
