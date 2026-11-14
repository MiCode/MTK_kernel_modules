// SPDX-License-Identifier: BSD-2-Clause

#if (CFG_SUPPORT_FW_IDX_LOG_SAVE == 1)
#include "gl_os.h"
#include "debug.h"
#include "precomp.h"

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
#include "gl_fw_dev.h"
#include "wlan_ring.h"

#define FW_INDEX_LOG_SIZE (128*1024)
#define FW_INDEX_LOG_DRIVER_NAME "fw_log_index"

struct index_ring {
	/* ring related variable */
	struct wlan_ring ring_cache;
	size_t ring_size;
	void *ring_base;
};

struct index_dev {
	u_int8_t fgReady;
	/* device related variable */
	struct cdev cdev;
	dev_t devno;
	struct class *driver_class;
	struct device *class_dev;
	int major;
	/* functional variable */
	struct index_ring iRing;
	wait_queue_head_t wq;
};

/* global variable of index log */
static struct index_dev *gIndexDev;

/* ring related function */
static int index_ring_init(struct index_ring *iRing, size_t size)
{
	int ret = 0;
	void *pBuffer = NULL;

	if (unlikely(iRing->ring_base)) {
		DBGLOG(ICS, ERROR, "index_ring has init?\n");
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

static ssize_t index_ring_read(
	struct index_ring *iRing,
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
		DBGLOG(ICS, ERROR, "index_ring not init yet\n");
		read = -EPERM;
	}

return_fn:
	DBGLOG(ICS, TEMP, "[Done] read:%d left:%d\n", read,
		left_to_read);
	return read;
}

static ssize_t index_ring_write(struct index_ring *iRing, char *buf,
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
			memcpy(ring_seg.ring_pt, buf, ring_seg.sz);
			left_to_write -= ring_seg.sz;
			written += ring_seg.sz;
		}

	} else {
		DBGLOG(ICS, ERROR, "index_ring not init yet\n");
		written = -EPERM;
	}

skip:
	DBGLOG(ICS, TEMP, "[Done] written:%d left:%d\n", written,
		left_to_write);
	return written;
}

static ssize_t index_ring_get_buf_size(struct index_ring *iRing)
{
	struct wlan_ring *ring = &iRing->ring_cache;

	if (unlikely(iRing->ring_base == NULL)) {
		DBGLOG(ICS, ERROR, "index_ring not init yet\n");
		return -EPERM;
	}

	return WLAN_RING_SIZE(ring);
}

static void index_ring_deinit(struct index_ring *iRing)
{
	if (likely(iRing->ring_base)) {
		kvfree(iRing->ring_base);
		iRing->ring_base = NULL;
	}
}

static int fw_log_index_open(struct inode *inode, struct file *file)
{
	DBGLOG(ICS, TEMP, "major %d minor %d (pid %d)\n",
		imajor(inode), iminor(inode), current->pid);
	return 0;
}

static int fw_log_index_release(struct inode *inode, struct file *file)
{
	DBGLOG(ICS, TEMP, "major %d minor %d (pid %d)\n",
		imajor(inode), iminor(inode), current->pid);
	return 0;
}

static ssize_t fw_log_index_read(struct file *filp, char __user *buf,
	size_t len, loff_t *off)
{
	size_t ret = 0;

	if ((gIndexDev == NULL) || (gIndexDev->fgReady == FALSE))
		return 0;

	ret = index_ring_read(&gIndexDev->iRing, buf, len);
	return ret;
}

static unsigned int fw_log_index_poll(struct file *filp, poll_table *wait)
{
	if ((gIndexDev == NULL) || (gIndexDev->fgReady == FALSE))
		return 0;

	poll_wait(filp, &gIndexDev->wq, wait);
	if (index_ring_get_buf_size(&gIndexDev->iRing) > 0)
		return POLLIN|POLLRDNORM;
	return 0;
}

const struct file_operations fw_log_index_fops = {
	.open = fw_log_index_open,
	.release = fw_log_index_release,
	.read = fw_log_index_read,
	.poll = fw_log_index_poll,
};

ssize_t wifi_index_fwlog_write(char *buf, size_t count)
{
	ssize_t ret = 0;

	if ((gIndexDev == NULL) || (gIndexDev->fgReady == FALSE))
		return 0;

	ret = index_ring_write(&(gIndexDev->iRing), buf, count);
	if (ret > 0)
		wake_up_interruptible(&(gIndexDev->wq));
	return ret;
}

int FwLogDevInit(void)
{
	int result = 0;
	int err = 0;

	gIndexDev = kzalloc(sizeof(struct index_dev), GFP_KERNEL);
	if (gIndexDev == NULL) {
		result = -ENOMEM;
		goto return_fn;
	}

	gIndexDev->devno = MKDEV(gIndexDev->major, 0);
	result = alloc_chrdev_region(&gIndexDev->devno, 0, 1,
			FW_INDEX_LOG_DRIVER_NAME);
	gIndexDev->major = MAJOR(gIndexDev->devno);
	DBGLOG(ICS, DEBUG,
		"alloc_chrdev_region result %d, major %d\n",
		result, gIndexDev->major);

	if (result < 0)
		goto free_dev;

	gIndexDev->driver_class = KAL_CLASS_CREATE(FW_INDEX_LOG_DRIVER_NAME);

	if (KAL_IS_ERR(gIndexDev->driver_class)) {
		result = -ENOMEM;
		DBGLOG(ICS, ERROR, "class_create failed %d.\n",
			result);
		goto unregister_chrdev_region;
	}

	gIndexDev->class_dev = device_create(gIndexDev->driver_class,
		NULL, gIndexDev->devno, NULL, FW_INDEX_LOG_DRIVER_NAME);

	if (!gIndexDev->class_dev) {
		result = -ENOMEM;
		DBGLOG(ICS, ERROR, "class_device_create failed %d.\n",
			result);
		goto class_destroy;
	}

	err = index_ring_init(&gIndexDev->iRing, FW_INDEX_LOG_SIZE);
	if (err) {
		result = -ENOMEM;
		DBGLOG(ICS, ERROR,
			"Error %d ics_ring_init.\n", err);
		goto device_destroy;
	}

	init_waitqueue_head(&gIndexDev->wq);

	cdev_init(&gIndexDev->cdev, &fw_log_index_fops);

	gIndexDev->cdev.owner = THIS_MODULE;
	gIndexDev->cdev.ops = &fw_log_index_fops;

	err = cdev_add(&gIndexDev->cdev, gIndexDev->devno, 1);
	if (err) {
		result = -ENOMEM;
		DBGLOG(ICS, ERROR,
			"Error %d adding fw_log_ics dev.\n", err);
		goto index_ring_deinit;
	}

	gIndexDev->fgReady = TRUE;
	goto return_fn;

index_ring_deinit:
	index_ring_deinit(&gIndexDev->iRing);
device_destroy:
	device_destroy(gIndexDev->driver_class, gIndexDev->devno);
class_destroy:
	class_destroy(gIndexDev->driver_class);
unregister_chrdev_region:
	unregister_chrdev_region(gIndexDev->devno, 1);
free_dev:
	kfree(gIndexDev);
	gIndexDev = NULL;
return_fn:
	return result;
}

int FwLogDevUninit(void)
{
	gIndexDev->fgReady = FALSE;
	index_ring_deinit(&gIndexDev->iRing);
	device_destroy(gIndexDev->driver_class, gIndexDev->devno);
	class_destroy(gIndexDev->driver_class);
	cdev_del(&gIndexDev->cdev);
	unregister_chrdev_region(MKDEV(gIndexDev->major, 0), 1);
	DBGLOG(ICS, DEBUG, "unregister_chrdev_region major %d\n",
		gIndexDev->major);
	kfree(gIndexDev);
	gIndexDev = NULL;
	return 0;
}
#endif


