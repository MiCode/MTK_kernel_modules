/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_EACH_DEVICE_H
#define _GPS_MCUDL_EACH_DEVICE_H

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

#include "gps_each_link.h"
#include "gps_each_device.h"
#include "gps_mcudl_each_link.h"
#include "gps_dl_dma_buf.h"
#include "gps_mcudl_data_intf_type.h"


/* Todo: should not use this const, currently it's a work-around */
#define GPS_LIBMNL_READ_MAX		512

struct gps_mcudl_each_device_cfg {
	char *dev_name;
	enum gps_mcudl_xid xid;
};

struct gps_mcudl_each_device {
	struct gps_mcudl_each_device_cfg cfg;
	struct gps_mcudl_each_link *p_link;
	int index;
	dev_t devno;
	struct class *cls;
	struct device *dev;
	struct cdev cdev;
	bool is_open;
	unsigned char i_buf[GPS_DATA_PATH_BUF_MAX];
	unsigned char o_buf[GPS_DATA_PATH_BUF_MAX];
	unsigned int i_len;
	wait_queue_head_t r_wq;
	void *private_data;
};

int gps_mcudl_cdev_setup(struct gps_mcudl_each_device *dev, enum gps_mcudl_xid xid);
void gps_mcudl_cdev_cleanup(struct gps_mcudl_each_device *dev, enum gps_mcudl_xid xid);
struct gps_mcudl_each_device *gps_mcudl_device_get(enum gps_mcudl_xid xid);

void gps_mcudl_device_context_init(void);
void gps_mcudl_device_context_deinit(void);

#endif /* _GPS_MCUDL_EACH_DEVICE_H */

