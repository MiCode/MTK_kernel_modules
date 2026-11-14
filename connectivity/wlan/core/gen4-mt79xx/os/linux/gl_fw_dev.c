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

#define FW_INDEX_LOG_NUM 256
#define FW_INDEX_LOG_DRIVER_NAME "fw_log_index"

struct index_dev {
	u_int8_t fgReady;
	/* device related variable */
	struct cdev cdev;
	dev_t devno;
	struct class *driver_class;
	struct device *class_dev;
	int major;
	/* functional variable */
	struct sk_buff_head queue;
	int queue_num;
	wait_queue_head_t wq;
	spinlock_t lock;
};

/* global variable of index log */
static struct index_dev *gIndexDev;

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
	struct sk_buff *skb = NULL;
	int copyLen = 0;
	unsigned long ret_len = 0;

	if ((gIndexDev == NULL) || (gIndexDev->fgReady == FALSE))
		return -ENOENT;

	if (gIndexDev->queue_num <= 0) {
		DBGLOG(ICS, ERROR, "no idex log to read, queue num %d\n",
			gIndexDev->queue_num);
		return -EFAULT;
	}

	spin_lock_bh(&gIndexDev->lock);
	skb = skb_dequeue(&gIndexDev->queue);
	gIndexDev->queue_num--;
	spin_unlock_bh(&gIndexDev->lock);

	if (skb == NULL)
		return 0;

	if (skb->len <= len) {
		ret_len = copy_to_user(buf, skb->data, skb->len);
		if (ret_len) {
			DBGLOG(ICS, ERROR,
				"copy_to_user failed, skb->len = %d, ret_len = %d, len = %d",
				skb->len, ret_len, len);
			/* copy_to_user failed, add skb to fw log queue */
			spin_lock_bh(&gIndexDev->lock);
			skb_queue_head(&gIndexDev->queue, skb);
			gIndexDev->queue_num++;
			spin_unlock_bh(&gIndexDev->lock);
			copyLen = -EFAULT;
			goto out;
		}
		copyLen = skb->len;
	} else
		DBGLOG(ICS, ERROR,
			"socket buffer length error(len: %d, skb.len: %d)",
			len, skb->len);

	kfree_skb(skb);

out:
	return copyLen;
}

static unsigned int fw_log_index_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;

	if ((gIndexDev == NULL) || (gIndexDev->fgReady == FALSE))
		return 0;

	poll_wait(filp, &gIndexDev->wq, wait);
	if (gIndexDev->queue_num > 0)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

const struct file_operations fw_log_index_fops = {
	.open = fw_log_index_open,
	.release = fw_log_index_release,
	.read = fw_log_index_read,
	.poll = fw_log_index_poll,
};

void FwLogWrite(char *buf, size_t count)
{
	struct sk_buff *skb_tmp = NULL;

	if ((gIndexDev == NULL) || (gIndexDev->fgReady == FALSE)) {
		DBGLOG(ICS, ERROR, "proc node not ready\n");
		return;
	}

	if (buf == NULL || count == 0) {
		DBGLOG(ICS, ERROR, "input null data\n");
		return;
	}

	if (gIndexDev->queue_num >= FW_INDEX_LOG_NUM) {
		DBGLOG(ICS, ERROR, "index log full, queue_num:%d\n",
			gIndexDev->queue_num);
		return;
	}

	skb_tmp = alloc_skb(count, GFP_ATOMIC);
	if (!skb_tmp) {
		DBGLOG(ICS, ERROR, "alloc skb fail\n");
		return;
	}

	memcpy(skb_tmp->data, buf, count);
	skb_tmp->len = count;

	spin_lock_bh(&gIndexDev->lock);
	skb_queue_tail(&gIndexDev->queue, skb_tmp);
	gIndexDev->queue_num++;
	spin_unlock_bh(&gIndexDev->lock);

	wake_up_interruptible(&gIndexDev->wq);
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
	DBGLOG(ICS, INFO,
		"alloc_chrdev_region result %d, major %d\n",
		result, gIndexDev->major);

	if (result < 0)
		goto free_dev;

#if KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE
	gIndexDev->driver_class =
		class_create(FW_INDEX_LOG_DRIVER_NAME);
#else
	gIndexDev->driver_class =
		class_create(THIS_MODULE, FW_INDEX_LOG_DRIVER_NAME);
#endif

	if (IS_ERR(gIndexDev->driver_class)) {
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

	skb_queue_head_init(&gIndexDev->queue);
	gIndexDev->queue_num = 0;
	init_waitqueue_head(&gIndexDev->wq);
	spin_lock_init(&gIndexDev->lock);

	cdev_init(&gIndexDev->cdev, &fw_log_index_fops);

	gIndexDev->cdev.owner = THIS_MODULE;
	gIndexDev->cdev.ops = &fw_log_index_fops;

	err = cdev_add(&gIndexDev->cdev, gIndexDev->devno, 1);
	if (err) {
		result = -ENOMEM;
		DBGLOG(ICS, ERROR,
			"Error %d adding fw_log_ics dev.\n", err);
		goto index_queue_deinit;
	}

	gIndexDev->fgReady = TRUE;
	goto return_fn;

index_queue_deinit:
	skb_queue_purge(&gIndexDev->queue);
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
	struct sk_buff *skb = NULL;

	if (gIndexDev == NULL) {
		DBGLOG(ICS, ERROR, "gIndexDev is null\n");
		return 0;
	}

	gIndexDev->fgReady = FALSE;
	while (gIndexDev->queue_num > 0) {
		spin_lock_bh(&gIndexDev->lock);
		skb = skb_dequeue(&gIndexDev->queue);
		gIndexDev->queue_num--;
		spin_unlock_bh(&gIndexDev->lock);
		if (skb)
			kfree_skb(skb);
	}

	skb_queue_purge(&gIndexDev->queue);
	device_destroy(gIndexDev->driver_class, gIndexDev->devno);
	class_destroy(gIndexDev->driver_class);
	cdev_del(&gIndexDev->cdev);
	unregister_chrdev_region(MKDEV(gIndexDev->major, 0), 1);
	DBGLOG(ICS, INFO, "unregister_chrdev_region major %d\n",
		gIndexDev->major);
	kfree(gIndexDev);
	gIndexDev = NULL;
	return 0;
}
#endif
