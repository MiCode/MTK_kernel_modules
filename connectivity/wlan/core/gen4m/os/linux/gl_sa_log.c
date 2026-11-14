// SPDX-License-Identifier: BSD-2-Clause

/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gl_os.h"
#include "debug.h"
#include "precomp.h"

#if (CFG_SUPPORT_SA_LOG == 1)
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
#include "gl_sa_log.h"
#include "wlan_ring.h"

#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
extern int mrdump_mini_add_extra_file(
	unsigned long vaddr,
	unsigned long paddr,
	unsigned long size,
	const char *name);
#endif

#define SA_DBGLOG(_Mod, _Clz, _Fmt, ...) \
do { \
	if ((au2DebugModule[DBG_##_Mod##_IDX] & \
		 DBG_CLASS_##_Clz) == 0) \
		break; \
	pr_info("[%u]%s:(" #_Mod " " #_Clz ") " _Fmt, \
		 KAL_GET_CURRENT_THREAD_ID(), \
		 __func__, ##__VA_ARGS__); \
} while (0)

#define SA_DBGLOG_LIMITED(_Mod, _Clz, _Fmt, ...) \
do { \
	if ((au2DebugModule[DBG_##_Mod##_IDX] & \
		 DBG_CLASS_##_Clz) == 0) \
		break; \
	pr_info_ratelimited(\
		"[%u]%s:(" #_Mod " " #_Clz ") " _Fmt, \
		 KAL_GET_CURRENT_THREAD_ID(), \
		 __func__, ##__VA_ARGS__); \
} while (0)

#define SA_LOG_SIZE (512*1024)
#define SA_WAIT_READY_MAX_CNT 2000
#define SA_WAIT_READY_SLEEP_TIME 100
#define SA_DRIVER_NAME "sa_log_wifi"

#define SA_IOC_MAGIC        (0xfc)
#define SA_IOCTL_ON_OFF     _IOW(SA_IOC_MAGIC, 0, int)
#define SA_IOCTL_SET_LEVEL  _IOW(SA_IOC_MAGIC, 1, int)

#define SA_LOG_CMD_ON_OFF        0
#define SA_LOG_CMD_SET_LEVEL     1

struct sa_ring {
	/* ring related variable */
	struct wlan_ring ring_cache;
	size_t ring_size;
	void *ring_base;
};

struct sa_dev {
	/* device related variable */
	struct cdev cdev;
	dev_t devno;
	struct class *driver_class;
	struct device *class_dev;
	int major;
	/* functional variable */
	struct sa_ring iRing;
	struct semaphore ioctl_mtx;
	salog_event_func_cb pfEventFuncCB;
	wait_queue_head_t wq;
};

/* global variable of sa log */
static struct sa_dev *gSaDev;

/* ring related function */
static int sa_ring_init(struct sa_ring *iRing, size_t size)
{
	int ret = 0;
	void *pBuffer = NULL;

	if (unlikely(iRing->ring_base)) {
		SA_DBGLOG(SA, ERROR, "sa_ring has init?\n");
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
#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
			mrdump_mini_add_extra_file(
				(unsigned long) pBuffer,
				__pa_nodebug(pBuffer),
				size,
				"WIFI_DRIVER_LOG");
#endif
		}
	}

	return ret;
}

static ssize_t sa_ring_read(
	struct sa_ring *iRing,
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
			SA_DBGLOG(SA, TEMP,
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
		SA_DBGLOG(SA, ERROR, "sa_ring not init yet\n");
		read = -EPERM;
	}

return_fn:
	SA_DBGLOG(SA, TEMP, "[Done] read:%ld left:%ld\n", read,
		left_to_read);
	return read;
}

static ssize_t sa_ring_write(struct sa_ring *iRing, char *buf,
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
		SA_DBGLOG(SA, ERROR, "sa_ring not init yet\n");
		written = -EPERM;
	}

skip:
	SA_DBGLOG(SA, TEMP, "[Done] written:%ld left:%ld\n", written,
		left_to_write);
	return written;
}

static ssize_t sa_ring_get_buf_size(struct sa_ring *iRing)
{
	struct wlan_ring *ring = &iRing->ring_cache;

	if (unlikely(iRing->ring_base == NULL)) {
		SA_DBGLOG(SA, ERROR, "sa_ring not init yet\n");
		return -EPERM;
	}

	return WLAN_RING_SIZE(ring);
}

static void sa_ring_deinit(struct sa_ring *iRing)
{
	if (likely(iRing->ring_base)) {
		kvfree(iRing->ring_base);
		iRing->ring_base = NULL;
	}
}

static int sa_open(struct inode *inode, struct file *file)
{
	SA_DBGLOG(SA, TEMP, "major %d minor %d (pid %d)\n",
		imajor(inode), iminor(inode), current->pid);
	return 0;
}

static int sa_release(struct inode *inode, struct file *file)
{
	SA_DBGLOG(SA, TEMP, "major %d minor %d (pid %d)\n",
		imajor(inode), iminor(inode), current->pid);
	return 0;
}

static ssize_t sa_read(struct file *filp, char __user *buf,
	size_t len, loff_t *off)
{
	size_t ret = 0;

	ret = sa_ring_read(&gSaDev->iRing, buf, len);
	return ret;
}

static unsigned int sa_poll(struct file *filp, poll_table *wait)
{
	poll_wait(filp, &gSaDev->wq, wait);

	if (sa_ring_get_buf_size(&gSaDev->iRing) > 0)
		return POLLIN|POLLRDNORM;
	return 0;
}

static long sa_unlocked_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	int ret = 0;

	down(&gSaDev->ioctl_mtx);
	switch (cmd) {
	case SA_IOCTL_SET_LEVEL:{
		unsigned int level = (unsigned int) arg;

		SA_DBGLOG(SA, DEBUG, "SA_IOCTL_SET_LEVEL start\n");

		if (gSaDev->pfEventFuncCB) {
			SA_DBGLOG(SA, DEBUG,
				"SA_IOCTL_SET_LEVEL invoke:%d\n",
				(int)level);
			gSaDev->pfEventFuncCB(SA_LOG_CMD_SET_LEVEL,
				level);
		} else {
			SA_DBGLOG(SA, ERROR,
				"SA_IOCTL_SET_LEVEL invoke failed\n");
		}

		SA_DBGLOG(SA, DEBUG, "SA_IOCTL_SET_LEVEL end\n");
		break;
	}
	case SA_IOCTL_ON_OFF:{
		unsigned int log_on_off = (unsigned int) arg;

		SA_DBGLOG(SA, DEBUG, "SA_IOCTL_ON_OFF start\n");

		if (gSaDev->pfEventFuncCB) {
			SA_DBGLOG(SA, DEBUG,
				"SA_IOCTL_ON_OFF invoke:%d\n",
				(int)log_on_off);
			gSaDev->pfEventFuncCB(SA_LOG_CMD_ON_OFF, log_on_off);
		} else {
			SA_DBGLOG(SA, ERROR,
				"SA_IOCTL_ON_OFF invoke failed\n");
		}

		SA_DBGLOG(SA, DEBUG, "SA_IOCTL_ON_OFF end\n");
		break;
	}
	default:
		ret = -EPERM;
	}
	SA_DBGLOG(SA, DEBUG, "cmd --> %d, ret=%d\n", cmd, ret);
	up(&gSaDev->ioctl_mtx);
	return ret;
}

#ifdef CONFIG_COMPAT
static long sa_compat_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	long ret = 0;
	int32_t wait_cnt = 0;

	SA_DBGLOG(SA, DEBUG, "COMPAT cmd --> %d\n", cmd);

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	while (wait_cnt < SA_WAIT_READY_MAX_CNT) {
		if (gSaDev->pfEventFuncCB)
			break;
		SA_DBGLOG_LIMITED(SA, ERROR,
			"Wi-Fi driver is not ready for 2s\n");
		msleep(SA_WAIT_READY_SLEEP_TIME);
		wait_cnt++;
	}
	sa_unlocked_ioctl(filp, cmd, arg);

	return ret;
}
#endif /* CONFIG_COMPAT */

const struct file_operations sa_fops = {
	.open = sa_open,
	.release = sa_release,
	.read = sa_read,
	.poll = sa_poll,
	.unlocked_ioctl = sa_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sa_compat_ioctl,
#endif /* CONFIG_COMPAT */
};

void wifi_salog_event_func_register(salog_event_func_cb func)
{
	SA_DBGLOG(SA, DEBUG,
		"wifi_salog_event_func_register %p\n", func);
	gSaDev->pfEventFuncCB = func;
}

ssize_t wifi_salog_write(char *buf, size_t count)
{
	ssize_t ret = 0;

	ret = sa_ring_write(&gSaDev->iRing, buf, count);
	if (ret > 0)
		wake_up_interruptible(&gSaDev->wq);

	return ret;
}

int SalogInit(void)
{
	int result = 0;
	int err = 0;

	gSaDev = kzalloc(sizeof(struct sa_dev), GFP_KERNEL);
	if (gSaDev == NULL) {
		result = -ENOMEM;
		goto return_fn;
	}

	gSaDev->devno = MKDEV(gSaDev->major, 0);
	result = alloc_chrdev_region(&gSaDev->devno, 0, 1,
			SA_DRIVER_NAME);
	gSaDev->major = MAJOR(gSaDev->devno);
	SA_DBGLOG(SA, DEBUG,
		"alloc_chrdev_region result %d, major %d\n",
		result, gSaDev->major);

	if (result < 0)
		goto free_dev;

	gSaDev->driver_class = KAL_CLASS_CREATE(SA_DRIVER_NAME);

	if (KAL_IS_ERR(gSaDev->driver_class)) {
		result = -ENOMEM;
		SA_DBGLOG(SA, ERROR, "class_create failed %d.\n",
			result);
		goto unregister_chrdev_region;
	}

	gSaDev->class_dev = device_create(gSaDev->driver_class,
		NULL, gSaDev->devno, NULL, SA_DRIVER_NAME);

	if (!gSaDev->class_dev) {
		result = -ENOMEM;
		SA_DBGLOG(SA, ERROR, "class_device_create failed %d.\n",
			result);
		goto class_destroy;
	}

	err = sa_ring_init(&gSaDev->iRing, SA_LOG_SIZE);
	if (err) {
		result = -ENOMEM;
		SA_DBGLOG(SA, ERROR,
			"Error %d sa_ring_init.\n", err);
		goto device_destroy;
	}

	init_waitqueue_head(&gSaDev->wq);
	sema_init(&gSaDev->ioctl_mtx, 1);
	gSaDev->pfEventFuncCB = NULL;

	cdev_init(&gSaDev->cdev, &sa_fops);

	gSaDev->cdev.owner = THIS_MODULE;
	gSaDev->cdev.ops = &sa_fops;

	err = cdev_add(&gSaDev->cdev, gSaDev->devno, 1);
	if (err) {
		result = -ENOMEM;
		SA_DBGLOG(SA, ERROR,
			"Error %d adding sa dev.\n", err);
		goto sa_ring_deinit;
	}

	goto return_fn;

sa_ring_deinit:
	sa_ring_deinit(&gSaDev->iRing);
device_destroy:
	device_destroy(gSaDev->driver_class, gSaDev->devno);
class_destroy:
	class_destroy(gSaDev->driver_class);
unregister_chrdev_region:
	unregister_chrdev_region(gSaDev->devno, 1);
free_dev:
	kfree(gSaDev);
return_fn:
	return result;
}

int SalogDeInit(void)
{
	sa_ring_deinit(&gSaDev->iRing);
	device_destroy(gSaDev->driver_class, gSaDev->devno);
	class_destroy(gSaDev->driver_class);
	cdev_del(&gSaDev->cdev);
	unregister_chrdev_region(MKDEV(gSaDev->major, 0), 1);
	SA_DBGLOG(SA, DEBUG, "unregister_chrdev_region major %d\n",
		gSaDev->major);
	kfree(gSaDev);
	return 0;
}

#endif /* CFG_SUPPORT_SA_LOG */
