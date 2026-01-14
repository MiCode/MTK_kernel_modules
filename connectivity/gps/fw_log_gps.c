/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

/*******************************************************************************
* Dependency
*******************************************************************************/
#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
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
#include <linux/poll.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
#include <asm/mmu.h>
#else
#include <asm/memblock.h>
#endif
#include <linux/wait.h>
#include "connsys_debug_utility.h"
#include "fw_log_gps_lib.h"
#include "gps_data_link_devices.h"
#include "gps_dl_config.h"
#include "gps_dl_time_tick.h"
#if GPS_DL_HAS_MCUDL
#include "gps_mcudl_xlink.h"
#endif
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

/******************************************************************************
 * Definition
******************************************************************************/
/* device name and major number */
#define GPSFWLOG_DEVNAME            "fw_log_gps"
#define GPS_FW_LOG_IOC_MAGIC          (0xfc)
#define GPS_FW_LOG_IOCTL_ON_OFF      _IOW(GPS_FW_LOG_IOC_MAGIC, 0, int)
#define GPS_FW_LOG_IOCTL_SET_LEVEL   _IOW(GPS_FW_LOG_IOC_MAGIC, 1, int)
#define GPS_FW_LOG_IOCTL_GET_LEVEL   _IOW(GPS_FW_LOG_IOC_MAGIC, 2, int)

/*******************************************************************************
* structure & enumeration
*******************************************************************************/
/*---------------------------------------------------------------------------*/
struct gps_fw_log_dev {
	struct class *cls;
	struct device *dev;
	dev_t devno;
	struct cdev chdev;
};
/* typedef unsigned char   UINT8, *PUINT8, **PPUINT8; */

/******************************************************************************
 * local variables
******************************************************************************/
/* static int flag; */
struct gps_fw_log_dev *logdevobj;
#if USE_FW_LOG_GPS_LIB
#if GPS_DL_HAS_MCUDL
static void log_event_cb2(void);
#endif
#else
#if GPS_DL_HAS_MCUDL
static void log_event_cb(void);
#endif
static wait_queue_head_t GPS_log_wq;
#endif
bool fgGps_fw_log_ON;

/*---------------------------------------------------------------------------*/
long fw_log_gps_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	switch (cmd) {
	case GPS_FW_LOG_IOCTL_ON_OFF:
		pr_info("gps PS_FW_LOG_IOCTL_ON_OFF(%lu)\n", arg);
		/*GPS_fwlog_ctrl((bool)arg);*/
#if GPS_DL_HAS_MCUDL
		gps_mcudl_xlink_fw_log_ctrl((bool)arg);
#endif
		break;

	case GPS_FW_LOG_IOCTL_SET_LEVEL:
		pr_info("gps GPS_FW_LOG_IOCTL_SET_LEVEL\n");
		break;
	case GPS_FW_LOG_IOCTL_GET_LEVEL:
		pr_info("gps GPS_FW_LOG_IOCTL_GET_LEVEL\n");
		break;

	default:
		pr_info("gps unknown cmd (%d)\n", cmd);
		break;
	}
	return retval;
}

long fw_log_gps_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return fw_log_gps_unlocked_ioctl(filp, cmd, arg);
}

/******************************************************************************/
/*****************************************************************************/
static int fw_log_open(struct inode *inode, struct file *file)
{
	int retval = 0;

#if USE_FW_LOG_GPS_LIB
	struct gps_fw_log_each_ctx *p_ctx;

	p_ctx = gps_fw_log_each_ctx_alloc();
	if (p_ctx == NULL)
		retval = -ENOMEM;
	else
		file->private_data = p_ctx;
#endif
	pr_info("%s: gps major %d minor %d (pid %d), retval=%d\n",
		__func__, imajor(inode), iminor(inode), current->pid, retval);
	return retval;
}

/*****************************************************************************/


/*****************************************************************************/
static int fw_log_close(struct inode *inode, struct file *file)
{
#if USE_FW_LOG_GPS_LIB
	struct gps_fw_log_each_ctx *p_ctx;

	p_ctx = (struct gps_fw_log_each_ctx *)file->private_data;
	if (p_ctx) {
		gps_fw_log_each_ctx_free(p_ctx);
		file->private_data = NULL;
	}
	pr_info("%s: gps major %d minor %d (pid %d), p_ctx=0x%p\n",
		__func__, imajor(inode), iminor(inode), current->pid, p_ctx);
#else
	pr_info("%s: gps major %d minor %d (pid %d)\n",
		__func__, imajor(inode), iminor(inode), current->pid);
#endif
	return 0;
}

/******************************************************************************/
static ssize_t fw_log_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
#if USE_FW_LOG_GPS_LIB
	struct gps_fw_log_each_ctx *p_ctx;
#endif
	int retval;

	#if 0
	pr_info("GPS fw_log_read,len=%d\n", count);
	#endif

#if USE_FW_LOG_GPS_LIB
	p_ctx = (struct gps_fw_log_each_ctx *)file->private_data;
	if (p_ctx == NULL) {
		pr_info("%s: p_ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (!p_ctx->is_allocated) {
		pr_info("%s: not allocated\n", __func__);
		return -EINVAL;
	}

	retval = gps_fw_log_data_copy_to_user(p_ctx, buf, count);
#else
#if GPS_DL_HAS_CONNINFRA_DRV
	retval = connsys_log_read_to_user(CONN_DEBUG_TYPE_GPS, buf, count);
#endif
#endif
	return retval;
}
/******************************************************************************/
#if USE_FW_LOG_GPS_LIB
static unsigned int fw_log_poll2(struct file *file, poll_table *wait)
{
	struct gps_fw_log_each_ctx *p_ctx;
	unsigned int mask = 0;

	p_ctx = (struct gps_fw_log_each_ctx *)file->private_data;
	if (p_ctx == NULL) {
		pr_info("%s: p_ctx is NULL\n", __func__);
		return 0;
	}

	if (!p_ctx->is_allocated) {
		pr_info("%s: not allocated\n", __func__);
		return 0;
	}

	poll_wait(file, &p_ctx->wq, wait);
	if (gps_fw_log_data_get_size(p_ctx) > 0)
		mask = (POLLIN | POLLRDNORM);

	return mask;
}
#else
static unsigned int fw_log_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &GPS_log_wq, wait);
	if (connsys_log_get_buf_size(CONN_DEBUG_TYPE_GPS) > 0)
		mask = (POLLIN | POLLRDNORM);

	return mask;
}
#endif


/*****************************************************************************/
/* Kernel interface */
static const struct file_operations gps_fw_log_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = fw_log_gps_unlocked_ioctl,
	.compat_ioctl = fw_log_gps_compat_ioctl,
	.open = fw_log_open,
	.read = fw_log_read,
	.release = fw_log_close,
#if USE_FW_LOG_GPS_LIB
	.poll = fw_log_poll2,
#else
	.poll = fw_log_poll,
#endif
};

#if USE_FW_LOG_GPS_LIB
#if GPS_DL_HAS_MCUDL
unsigned char log_event_cb2_buf[20*1024];
unsigned long log_ret;
void log_event_cb2(void)
{
	int retval = 0;
	unsigned long curr_tick;
	static unsigned long last_tick;

#if GPS_DL_HAS_CONNINFRA_DRV
	retval = connsys_log_read(CONN_DEBUG_TYPE_GPS, &log_event_cb2_buf[0], 20*1024);
	if (retval <= 0) {
		pr_info("gps_fw_log_event: read retval=%d", retval);
		return;
	}
#endif
	gps_fw_log_data_submit_to_all(&log_event_cb2_buf[0], retval);

	/*Prevent too much log, print once per 1s*/
	curr_tick = gps_dl_tick_get_us();
	log_ret = log_ret + retval;

	if ((curr_tick - last_tick) >= 1000000) {
		pr_info("gps_fw_log_event: read retval=%lu", log_ret);
		last_tick = curr_tick;
		log_ret = 0;
	}
}
#endif
#else
#if GPS_DL_HAS_MCUDL
void log_event_cb(void)
{
	wake_up_interruptible(&GPS_log_wq);
}
#endif
#endif

static int gps_fw_log_init(void)
{
	int ret = 0;
	int err = 0;

#if GPS_DL_HAS_MCUDL
#if GPS_DL_HAS_CONNINFRA_DRV
	connsys_log_init(CONN_DEBUG_TYPE_GPS);
#endif
#endif
#if USE_FW_LOG_GPS_LIB
	ret = gps_fw_log_all_ctx_init();
	if (ret < 0) {
		err = ret;
		goto err_out;
	}
#if GPS_DL_HAS_MCUDL
#if GPS_DL_HAS_CONNINFRA_DRV
	connsys_log_register_event_cb(CONN_DEBUG_TYPE_GPS, log_event_cb2);
#endif
#endif
#else
	init_waitqueue_head(&GPS_log_wq);
#if GPS_DL_HAS_MCUDL
#if GPS_DL_HAS_CONNINFRA_DRV
	connsys_log_register_event_cb(CONN_DEBUG_TYPE_GPS, log_event_cb);
#endif
#endif
#endif

	logdevobj = kzalloc(sizeof(*logdevobj), GFP_KERNEL);
	if (logdevobj == NULL) {
		err = -ENOMEM;
		ret = -ENOMEM;
		goto err_out;
	}

	pr_info("Registering chardev\n");
	ret = alloc_chrdev_region(&logdevobj->devno, 0, 1, GPSFWLOG_DEVNAME);
	if (ret) {
		pr_info("alloc_chrdev_region fail: %d\n", ret);
		err = -ENOMEM;
		goto err_out;
	} else {
		pr_info("major: %d, minor: %d\n", MAJOR(logdevobj->devno), MINOR(logdevobj->devno));
	}
	cdev_init(&logdevobj->chdev, &gps_fw_log_fops);
	logdevobj->chdev.owner = THIS_MODULE;
	err = cdev_add(&logdevobj->chdev, logdevobj->devno, 1);
	if (err) {
		pr_info("cdev_add fail: %d\n", err);
		goto err_out;
	}
	logdevobj->cls = class_create(THIS_MODULE, "gpsfwlog");
	if (IS_ERR(logdevobj->cls)) {
		pr_info("Unable to create class, err = %d\n", (int)PTR_ERR(logdevobj->cls));
		goto err_out;
	}

	logdevobj->dev = device_create(logdevobj->cls, NULL, logdevobj->devno, logdevobj, "fw_log_gps");

	pr_info("GPS FW LOG device init Done\n");
	return 0;

err_out:
	if (logdevobj != NULL) {
		if (err == 0)
			cdev_del(&logdevobj->chdev);
		if (ret == 0)
			unregister_chrdev_region(logdevobj->devno, 1);
		kfree(logdevobj);
		logdevobj = NULL;
	}
	return -1;
}

/*****************************************************************************/
static void gps_fw_log_exit(void)
{
	if (!logdevobj) {
		pr_info("null pointer: %p\n", logdevobj);
		return;
	}

	pr_info("Unregistering chardev\n");
#if GPS_DL_HAS_CONNINFRA_DRV
	connsys_log_deinit(CONN_DEBUG_TYPE_GPS);
#endif
	cdev_del(&logdevobj->chdev);
	unregister_chrdev_region(logdevobj->devno, 1);
	device_destroy(logdevobj->cls, logdevobj->devno);
	class_destroy(logdevobj->cls);
	kfree(logdevobj);
	logdevobj = NULL;

#if GPS_DL_HAS_CONNINFRA_DRV
	connsys_log_register_event_cb(CONN_DEBUG_TYPE_GPS, NULL);
#endif
#if USE_FW_LOG_GPS_LIB
	gps_fw_log_all_ctx_exit();
#endif
#if GPS_DL_HAS_CONNINFRA_DRV
	connsys_log_deinit(CONN_DEBUG_TYPE_GPS);
#endif
	pr_info("Done\n");
}

int mtk_gps_fw_log_init(void)
{
	pr_info("gps fw log init begin");
	return gps_fw_log_init();
}

void mtk_gps_fw_log_exit(void)
{
	pr_info("gps fw log exit begin");
	return gps_fw_log_exit();
}

/*****************************************************************************/
#if 0
module_init(gps_emi_mod_init);
module_exit(gps_emi_mod_exit);
#endif
/*****************************************************************************/
MODULE_AUTHOR("Chaoran Zhang <Chaoran.Zhang@mediatek.com>");
MODULE_DESCRIPTION("GPS FW log driver");
MODULE_LICENSE("GPL");
#endif
