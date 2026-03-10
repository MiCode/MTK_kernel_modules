/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gl_os.h"
#include "debug.h"
#include "precomp.h"

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/string.h>
#include "ics.h"
#include "wlan_ring.h"

#define ICS_LOG_SIZE (128*1024)
#define ICS_WAIT_READY_MAX_CNT 2000
#define ICS_WAIT_READY_SLEEP_TIME 100
#define FW_LOG_ICS_DRIVER_NAME "fw_log_ics"

#define ICS_FW_LOG_IOC_MAGIC        (0xfc)
#define ICS_FW_LOG_IOCTL_ON_OFF     _IOW(ICS_FW_LOG_IOC_MAGIC, 0, int)
#define ICS_FW_LOG_IOCTL_SET_LEVEL  _IOW(ICS_FW_LOG_IOC_MAGIC, 1, int)

#define ICS_LOG_CMD_ON_OFF        0
#define ICS_LOG_CMD_SET_LEVEL     1

struct ics_ring {
	/* ring related variable */
	struct wlan_ring ring_cache;
	size_t ring_size;
	void *ring_base;
};

struct ics_dev {
	/* device related variable */
	struct cdev cdev;
	dev_t devno;
	struct class *driver_class;
	struct device *class_dev;
	int major;
	/* functional variable */
	struct ics_ring iRing;
	struct semaphore ioctl_mtx;
	ics_fwlog_event_func_cb pfFwEventFuncCB;
	wait_queue_head_t wq;
};

/* global variable of ics log */
static struct ics_dev *gIcsDev;

/* ring related function */
static int ics_ring_init(struct ics_ring *iRing, size_t size)
{
	int ret = 0;
	void *pBuffer = NULL;

	if (unlikely(iRing->ring_base)) {
		DBGLOG(ICS, ERROR, "ics_ring has init?\n");
		ret = -EPERM;
	} else {
		pBuffer = kmalloc(size, GFP_KERNEL);
		if (pBuffer == NULL && size > PAGE_SIZE)
			pBuffer = vmalloc(size);

		if (!pBuffer)
			ret = -ENOMEM;
		else {
			iRing->ring_base = pBuffer;
			iRing->ring_size = size;
			WLAN_RING_INIT(
				iRing->ring_base,
				iRing->ring_size,
				0,
				0,
				&iRing->ring_cache);
		}
	}

	return ret;
}

static ssize_t ics_ring_read(
	struct ics_ring *iRing,
	char __user *buf,
	size_t count)
{
	ssize_t read = 0;
	struct wlan_ring_segment ring_seg;
	struct wlan_ring *ring = &iRing->ring_cache;
	ssize_t left_to_read = 0;
	int ret;

	if (likely(iRing->ring_base)) {
		left_to_read = count < WLAN_RING_SIZE(ring)
				? count : WLAN_RING_SIZE(ring);
		if (WLAN_RING_EMPTY(ring) ||
			!WLAN_RING_READ_PREPARE(left_to_read,
				&ring_seg, ring)) {
			DBGLOG(ICS, TEMP,
				"no data/taken by other reader?\n");
			goto return_fn;
		}

		WLAN_RING_READ_FOR_EACH(left_to_read, ring_seg, ring) {
			ret = copy_to_user(buf + read, ring_seg.ring_pt,
				ring_seg.sz);
			if (ret)
				goto return_fn;
			left_to_read -= ring_seg.sz;
			read += ring_seg.sz;
		}
	} else {
		DBGLOG(ICS, ERROR, "ics_ring not init yet\n");
		read = -EPERM;
	}

return_fn:
	DBGLOG(ICS, TEMP, "[Done] read:%d left:%d\n", read,
		left_to_read);
	return read;
}

static ssize_t ics_ring_write(struct ics_ring *iRing, char *buf,
	size_t count)
{
	ssize_t written = 0;
	struct wlan_ring_segment ring_seg;
	struct wlan_ring *ring = &iRing->ring_cache;
	ssize_t left_to_write = count;

	if (likely(iRing->ring_base)) {
		/* no enough buffer to write */
		if (WLAN_RING_WRITE_REMAIN_SIZE(ring) < left_to_write)
			goto skip;

		WLAN_RING_WRITE_FOR_EACH(left_to_write, ring_seg, ring) {
			memcpy(ring_seg.ring_pt, buf + written, ring_seg.sz);
			left_to_write -= ring_seg.sz;
			written += ring_seg.sz;
		}

	} else {
		DBGLOG(ICS, ERROR, "ics_ring not init yet\n");
		written = -EPERM;
	}

skip:
	DBGLOG(ICS, TEMP, "[Done] written:%d left:%d\n", written,
		left_to_write);
	return written;
}

static ssize_t ics_ring_get_buf_size(struct ics_ring *iRing)
{
	struct wlan_ring *ring = &iRing->ring_cache;

	if (unlikely(iRing->ring_base == NULL)) {
		DBGLOG(ICS, ERROR, "ics_ring not init yet\n");
		return -EPERM;
	}

	return WLAN_RING_SIZE(ring);
}

static void ics_ring_deinit(struct ics_ring *iRing)
{
	if (likely(iRing->ring_base)) {
		kvfree(iRing->ring_base);
		iRing->ring_base = NULL;
	}
}

static int fw_log_ics_open(struct inode *inode, struct file *file)
{
	DBGLOG(ICS, TEMP, "major %d minor %d (pid %d)\n",
		imajor(inode), iminor(inode), current->pid);
	return 0;
}

static int fw_log_ics_release(struct inode *inode, struct file *file)
{
	DBGLOG(ICS, TEMP, "major %d minor %d (pid %d)\n",
		imajor(inode), iminor(inode), current->pid);
	return 0;
}

static ssize_t fw_log_ics_read(struct file *filp, char __user *buf,
	size_t len, loff_t *off)
{
	size_t ret = 0;

	ret = ics_ring_read(&gIcsDev->iRing, buf, len);
	return ret;
}

static unsigned int fw_log_ics_poll(struct file *filp, poll_table *wait)
{
	poll_wait(filp, &gIcsDev->wq, wait);

	if (ics_ring_get_buf_size(&gIcsDev->iRing) > 0)
		return POLLIN|POLLRDNORM;
	return 0;
}

static long fw_log_ics_unlocked_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	int ret = 0;

	down(&gIcsDev->ioctl_mtx);
	switch (cmd) {
	case ICS_FW_LOG_IOCTL_SET_LEVEL:{
		unsigned int level = (unsigned int) arg;

		DBGLOG(ICS, INFO, "ICS_FW_LOG_IOCTL_SET_LEVEL start\n");

		if (gIcsDev->pfFwEventFuncCB) {
			DBGLOG(ICS, INFO,
				"ICS_FW_LOG_IOCTL_SET_LEVEL invoke:%d\n",
				(int)level);
			gIcsDev->pfFwEventFuncCB(ICS_LOG_CMD_SET_LEVEL,
				level);
		} else {
			DBGLOG(ICS, ERROR,
				"ICS_FW_LOG_IOCTL_SET_LEVEL invoke failed\n");
		}

		DBGLOG(ICS, INFO, "ICS_FW_LOG_IOCTL_SET_LEVEL end\n");
		break;
	}
	case ICS_FW_LOG_IOCTL_ON_OFF:{
		unsigned int log_on_off = (unsigned int) arg;

		DBGLOG(ICS, INFO, "ICS_FW_LOG_IOCTL_ON_OFF start\n");

		if (gIcsDev->pfFwEventFuncCB) {
			DBGLOG(ICS, INFO,
				"ICS_FW_LOG_IOCTL_ON_OFF invoke:%d\n",
				(int)log_on_off);
			gIcsDev->pfFwEventFuncCB(ICS_LOG_CMD_ON_OFF, log_on_off);
		} else {
			DBGLOG(ICS, ERROR,
				"ICS_FW_LOG_IOCTL_ON_OFF invoke failed\n");
		}

		DBGLOG(ICS, INFO, "ICS_FW_LOG_IOCTL_ON_OFF end\n");
		break;
	}
	default:
		ret = -EPERM;
	}
	DBGLOG(ICS, INFO, "cmd --> %d, ret=%d\n", cmd, ret);
	up(&gIcsDev->ioctl_mtx);
	return ret;
}

#ifdef CONFIG_COMPAT
static long fw_log_ics_compat_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	long ret = 0;
	int32_t wait_cnt = 0;

	DBGLOG(ICS, INFO, "COMPAT cmd --> %d\n", cmd);

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	while (wait_cnt < ICS_WAIT_READY_MAX_CNT) {
		if (gIcsDev->pfFwEventFuncCB)
			break;
		DBGLOG_LIMITED(ICS, ERROR,
			"Wi-Fi driver is not ready for 2s\n");
		msleep(ICS_WAIT_READY_SLEEP_TIME);
		wait_cnt++;
	}
	fw_log_ics_unlocked_ioctl(filp, cmd, arg);

	return ret;
}
#endif /* CONFIG_COMPAT */

const struct file_operations fw_log_ics_fops = {
	.open = fw_log_ics_open,
	.release = fw_log_ics_release,
	.read = fw_log_ics_read,
	.poll = fw_log_ics_poll,
	.unlocked_ioctl = fw_log_ics_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = fw_log_ics_compat_ioctl,
#endif /* CONFIG_COMPAT */
};

void wifi_ics_event_func_register(ics_fwlog_event_func_cb func)
{
	DBGLOG(ICS, INFO, "wifi_ics_event_func_register %p\n", func);
	gIcsDev->pfFwEventFuncCB = func;
}

ssize_t wifi_ics_fwlog_write(char *buf, size_t count)
{
	ssize_t ret = 0;

	ret = ics_ring_write(&gIcsDev->iRing, buf, count);
	if (ret > 0)
		wake_up_interruptible(&gIcsDev->wq);

	return ret;
}

int IcsInit(void)
{
	int result = 0;
	int err = 0;

	gIcsDev = kzalloc(sizeof(struct ics_dev), GFP_KERNEL);
	if (gIcsDev == NULL) {
		result = -ENOMEM;
		goto return_fn;
	}

	gIcsDev->devno = MKDEV(gIcsDev->major, 0);
	result = alloc_chrdev_region(&gIcsDev->devno, 0, 1,
			FW_LOG_ICS_DRIVER_NAME);
	gIcsDev->major = MAJOR(gIcsDev->devno);
	DBGLOG(ICS, INFO,
		"alloc_chrdev_region result %d, major %d\n",
		result, gIcsDev->major);

	if (result < 0)
		goto free_dev;

	gIcsDev->driver_class = class_create(THIS_MODULE,
		FW_LOG_ICS_DRIVER_NAME);

	if (KAL_IS_ERR(gIcsDev->driver_class)) {
		result = -ENOMEM;
		DBGLOG(ICS, ERROR, "class_create failed %d.\n",
			result);
		goto unregister_chrdev_region;
	}

	gIcsDev->class_dev = device_create(gIcsDev->driver_class,
		NULL, gIcsDev->devno, NULL, FW_LOG_ICS_DRIVER_NAME);

	if (!gIcsDev->class_dev) {
		result = -ENOMEM;
		DBGLOG(ICS, ERROR, "class_device_create failed %d.\n",
			result);
		goto class_destroy;
	}

	err = ics_ring_init(&gIcsDev->iRing, ICS_LOG_SIZE);
	if (err) {
		result = -ENOMEM;
		DBGLOG(ICS, ERROR,
			"Error %d ics_ring_init.\n", err);
		goto device_destroy;
	}

	init_waitqueue_head(&gIcsDev->wq);
	sema_init(&gIcsDev->ioctl_mtx, 1);
	gIcsDev->pfFwEventFuncCB = NULL;

	cdev_init(&gIcsDev->cdev, &fw_log_ics_fops);

	gIcsDev->cdev.owner = THIS_MODULE;
	gIcsDev->cdev.ops = &fw_log_ics_fops;

	err = cdev_add(&gIcsDev->cdev, gIcsDev->devno, 1);
	if (err) {
		result = -ENOMEM;
		DBGLOG(ICS, ERROR,
			"Error %d adding fw_log_ics dev.\n", err);
		goto ics_ring_deinit;
	}

	goto return_fn;

ics_ring_deinit:
	ics_ring_deinit(&gIcsDev->iRing);
device_destroy:
	device_destroy(gIcsDev->driver_class, gIcsDev->devno);
class_destroy:
	class_destroy(gIcsDev->driver_class);
unregister_chrdev_region:
	unregister_chrdev_region(gIcsDev->devno, 1);
free_dev:
	kfree(gIcsDev);
return_fn:
	return result;
}

int IcsDeInit(void)
{
	ics_ring_deinit(&gIcsDev->iRing);
	device_destroy(gIcsDev->driver_class, gIcsDev->devno);
	class_destroy(gIcsDev->driver_class);
	cdev_del(&gIcsDev->cdev);
	unregister_chrdev_region(MKDEV(gIcsDev->major, 0), 1);
	DBGLOG(ICS, INFO, "unregister_chrdev_region major %d\n",
		gIcsDev->major);
	kfree(gIcsDev);
	return 0;
}

#endif /* CFG_SUPPORT_ICS */
