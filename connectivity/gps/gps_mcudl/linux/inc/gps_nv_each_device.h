/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _GPS_NV_EACH_DEVICE_H
#define _GPS_NV_EACH_DEVICE_H
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
#include <asm/mmu.h>
#else
#include <asm/memblock.h>
#endif
#include <linux/wait.h>
#include <linux/poll.h>

#include "gps_mcusys_data.h"
#include "gps_each_link.h"

struct gps_nv_each_device_cfg {
	char *dev_name;
	enum gps_mcusys_nv_data_id nv_id;
};

struct gps_nv_each_device {
	struct gps_nv_each_device_cfg cfg;
	enum gps_mcusys_nv_data_id nv_id;
	dev_t devno;
	struct class *cls;
	struct device *dev;
	struct cdev cdev;
	bool is_open;
	struct gps_each_link_waitable wait_read;
	bool epoll_in_flag;
	void *private_data;
};

int  gps_nv_cdev_setup(struct gps_nv_each_device *dev, enum gps_mcusys_nv_data_id nv_id);
void gps_nv_cdev_cleanup(struct gps_nv_each_device *dev, enum gps_mcusys_nv_data_id nv_id);
struct gps_nv_each_device *gps_nv_device_get(enum gps_mcusys_nv_data_id nv_id);
struct gps_each_link_waitable *gps_nv_each_link_get_read_waitable_ptr(enum gps_mcusys_nv_data_id nv_id);

void gps_nv_device_context_init(void);
void gps_nv_device_context_deinit(void);

#endif /* _GPS_NV_EACH_DEVICE_H */


