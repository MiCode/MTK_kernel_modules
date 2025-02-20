/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/usb.h>
#include <linux/usb/quirks.h>
#include <linux/firmware.h>
#include <asm/unaligned.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <linux/pm_wakeup.h>

#include "btmtk_usb.h"

static struct usb_driver btusb_driver;
#ifndef CFG_SUPPORT_CHIP_RESET_KO
#if (CFG_GKI_SUPPORT == 0)
static struct btmtk_cif_chip_reset reset_func;
#endif
#endif
static int intf_to_idx[BT_MCU_INTERFACE_NUM_MAX] = {0, -1, -1, 1};
static struct btmtk_usb_dev g_usb_dev[BT_MCU_MINIMUM_INTERFACE_NUM][BT_MCU_NUM_MAX];

static const struct usb_device_id btusb_table[] = {
	/* Mediatek MT7961 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7961, 0xe0, 0x01, 0x01) },
	/* Mediatek MT7915 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7915, 0xe0, 0x01, 0x01) },
	/* Mediatek MT7922 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7922, 0xe0, 0x01, 0x01) },
	/* Mediatek MT7902 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7902, 0xe0, 0x01, 0x01) },
	/* Mediatek MT6639 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x6639, 0xe0, 0x01, 0x01) },
	/* Mediatek MT7925 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7925, 0xe0, 0x01, 0x01) },
	{ }	/* Terminating entry */
};

static char event_need_compare[EVENT_COMPARE_SIZE] = {0};
static char event_need_compare_len;
static char event_compare_status;

static DEFINE_MUTEX(btmtk_usb_ops_mutex);
#define USB_OPS_MUTEX_LOCK()	mutex_lock(&btmtk_usb_ops_mutex)
#define USB_OPS_MUTEX_UNLOCK()	mutex_unlock(&btmtk_usb_ops_mutex)

MODULE_DEVICE_TABLE(usb, btusb_table);

#if BTMTK_ISOC_TEST
/* remove #define BTUSB_MAX_ISOC_FRAMES	10
 * ISCO_FRAMES max is 24
 * BTUSB_MAX_ISOC_FRAMES 15 for usb_sco_test
 */
#define BTUSB_MAX_ISOC_FRAMES	15
#else
#define BTUSB_MAX_ISOC_FRAMES	24
#endif

#define BTUSB_INTR_RUNNING	0
#define BTUSB_BULK_RUNNING	1
#define BTUSB_ISOC_RUNNING	2
#define BTUSB_SUSPENDING	3
#define BTUSB_DID_ISO_RESUME	4
#define BTUSB_BLE_ISOC_RUNNING	5
#define BTUSB_WMT_RUNNING	6
#define BTUSB_OPENED		7

#define DEVICE_VENDOR_REQUEST_IN	0xc0
#define DEVICE_CLASS_REQUEST_OUT	0x20
#define USB_CTRL_IO_TIMO		1000

#define BTMTK_IS_BT_0_INTF(ifnum_base) \
	(ifnum_base == BT0_MCU_INTERFACE_NUM)

#define BTMTK_IS_BT_1_INTF(ifnum_base) \
	(ifnum_base == BT1_MCU_INTERFACE_NUM)

#define BTMTK_CIF_GET_DEV_PRIV(bdev, intf, ifnum_base) \
	do { \
		bdev = usb_get_intfdata(intf); \
		ifnum_base = intf->cur_altsetting->desc.bInterfaceNumber; \
	} while (0)

static int btmtk_cif_allocate_memory(struct btmtk_usb_dev *cif_dev);
static void btmtk_cif_free_memory(struct btmtk_usb_dev *cif_dev);
static int btmtk_cif_write_uhw_register(struct btmtk_dev *bdev, u32 reg, u32 val);
static int btmtk_cif_read_uhw_register(struct btmtk_dev *bdev, u32 reg, u32 *val);

static int btmtk_usb_send_and_recv(struct btmtk_dev *bdev,
		struct sk_buff *skb,
		const uint8_t *event, const int event_len,
		int delay, int retry, int pkt_type, bool flag);
static void btmtk_usb_chip_reset_notify(struct btmtk_dev *bdev);
static int btmtk_usb_event_filter(struct btmtk_dev *bdev, struct sk_buff *skb);
static int btmtk_usb_send_cmd(struct btmtk_dev *bdev, struct sk_buff *skb,
		int delay, int retry, int pkt_type, bool flag);
static int btmtk_usb_read_register(struct btmtk_dev *bdev, u32 reg, u32 *val);
static int btmtk_usb_write_register(struct btmtk_dev *bdev, u32 reg, u32 val);

#if BTMTK_ISOC_TEST
/* For sco dev node setting */
static int btusb_submit_isoc_urb(struct hci_dev *hdev, gfp_t mem_flags);
static struct btmtk_fops_sco *g_sco;
static unsigned short sco_handle;	/* So far only support one SCO link */
static DECLARE_WAIT_QUEUE_HEAD(BT_sco_wq);

static int btmtk_usb_set_isoc_interface(bool close)
{
	int ret = -1, i = 0;
	struct usb_endpoint_descriptor *ep_desc;
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)g_sco->bdev->cif_dev;
	struct usb_interface *intf = cif_dev->isoc;

	if (g_sco == NULL) {
		BTMTK_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENXIO;
	}

	if ((sco_handle && g_sco->isoc_urb_submitted && close == false)
			|| (sco_handle == 0 && g_sco->isoc_urb_submitted == 0)) {
		return 0;

	} else if (sco_handle && g_sco->isoc_urb_submitted == 0) {
		/* Alternate setting */
		ret = usb_set_interface(cif_dev->udev, 1, g_sco->isoc_alt_setting);
		if (ret < 0) {
			BTMTK_ERR("%s: Set ISOC alternate(%d) fail", __func__, g_sco->isoc_alt_setting);
			return ret;
		}
		BTMTK_INFO("%s: Set alternate to %d", __func__, g_sco->isoc_alt_setting);

		for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
			ep_desc = &intf->cur_altsetting->endpoint[i].desc;

			if (usb_endpoint_is_isoc_out(ep_desc)) {
				cif_dev->isoc_tx_ep = ep_desc;
				BTMTK_INFO("iso_out: length: %d, addr: 0x%02X, maxSize: %d, interval: %d",
						ep_desc->bLength, ep_desc->bEndpointAddress,
						ep_desc->wMaxPacketSize, ep_desc->bInterval);
				continue;
			}

			if (usb_endpoint_is_isoc_in(ep_desc)) {
				cif_dev->isoc_rx_ep = ep_desc;
				BTMTK_INFO("iso_in: length: %d, addr: 0x%02X, maxSize: %d, interval: %d",
						ep_desc->bLength, ep_desc->bEndpointAddress,
						ep_desc->wMaxPacketSize, ep_desc->bInterval);
				continue;
			}
		}
		if (!cif_dev->isoc_tx_ep || !cif_dev->isoc_rx_ep) {
			BTMTK_ERR("Invalid SCO descriptors");
			return -ENODEV;
		}

		ret = btusb_submit_isoc_urb(g_sco->bdev->hdev, GFP_KERNEL);
		if (ret < 0)
			return ret;
		BTMTK_INFO("%s: Start isoc_in.", __func__);

	} else if ((sco_handle == 0 || close == true) && g_sco->isoc_urb_submitted) {
		u8 count = 0;

		while (atomic_read(&g_sco->isoc_out_count) && ++count <= RETRY_TIMES) {
			BTMTK_INFO("There are isoc out packet remaining: %d ",
					atomic_read(&g_sco->isoc_out_count));
			mdelay(10);
		}
		usb_kill_anchored_urbs(&cif_dev->isoc_anchor);
		g_sco->isoc_urb_submitted = 0;
		BTMTK_INFO("%s: Stop isoc_in.", __func__);

		ret = usb_set_interface(cif_dev->udev, 1, 0);
		if (ret < 0) {
			BTMTK_ERR("%s: Set ISOC alternate(0) fail", __func__);
			return ret;
		}
		BTMTK_INFO("%s: Set alternate to 0", __func__);

	} else {
		BTMTK_INFO("%s: sco: 0x%04X, isoc_urb_submitted: %d",
				__func__, sco_handle, g_sco->isoc_urb_submitted);
	}
	return 0;
}

static ssize_t btmtk_fops_sco_read(struct file *file, char __user *buf,
		size_t count, loff_t *f_pos)
{
	ssize_t retval = 0;
	int iso_packet_size = 0;
	struct sk_buff *skb = NULL;
	unsigned long flags = 0;
	unsigned long ret_len = 0;

	BTMTK_DBG("%s", __func__);
	if (g_sco == NULL || buf == NULL || count <= 0) {
		BTMTK_ERR("%s: ERROR, %s is NULL!", __func__,
				g_sco == NULL ? "g_sco" : buf == NULL ? "buf" : "count");
		return -ENODEV;
	}

	down(&g_sco->isoc_rd_mtx);
	retval = btmtk_usb_set_isoc_interface(false);
	if (retval < 0 || g_sco->isoc_urb_submitted == 0)
		goto OUT;
	/* means the buffer is empty */
	while (skb_queue_len(&g_sco->isoc_in_queue) == 0) {
		/* If nonblocking mode, return directly O_NONBLOCK is specified during open() */
		if (file->f_flags & O_NONBLOCK) {
			/* BTUSB_DBG("Non-blocking btmtk_usb_fops_read()"); */
			retval = -EAGAIN;
			goto OUT;
		}
		wait_event(BT_sco_wq, skb_queue_len(&g_sco->isoc_in_queue) > 0);
	}

	if (skb_queue_len(&g_sco->isoc_in_queue) > 0) {
		u32 remain = 0;
		u32 shift = 0;
		u8 real_num = 0;
		u8 i = 1;

		spin_lock_irqsave(&g_sco->isoc_lock, flags);
		skb = skb_dequeue(&g_sco->isoc_in_queue);
		spin_unlock_irqrestore(&g_sco->isoc_lock, flags);
		if (skb == NULL) {
			BTMTK_ERR("sbk is NULL");
			goto OUT;
		}

		real_num = skb->len / iso_packet_size;
		if (skb->len % iso_packet_size)
			real_num += 1;
		BTMTK_DBG("real_num: %d, mod: %d", real_num, skb->len % iso_packet_size);
		if (count < skb->len - real_num * HCI_SCO_HDR_SIZE) {
			int num = count / (iso_packet_size - HCI_SCO_HDR_SIZE);
			struct sk_buff *new_skb = NULL;

			remain = skb->len - num * iso_packet_size;
			new_skb = alloc_skb(remain, GFP_KERNEL);
			if (new_skb == NULL) {
				BTMTK_ERR("%s: alloc_skb return 0, error", __func__);
				kfree_skb(skb);
				retval = -ENOMEM;
				goto OUT;
			}

			memcpy(new_skb->data, skb->data + num * iso_packet_size, remain);
			new_skb->len = remain;
			spin_lock_irqsave(&g_sco->isoc_lock, flags);
			skb_queue_head(&g_sco->isoc_in_queue, new_skb);
			spin_unlock_irqrestore(&g_sco->isoc_lock, flags);
		}
		retval = skb->len - remain;	/* Include 3 bytes header */
		shift = 0;
		while (retval > 0) {
			size_t copy = *(skb->data + ((i - 1) * HCI_SCO_HDR_SIZE) + shift + 2);

			ret_len = copy_to_user(buf + shift, skb->data + (i * HCI_SCO_HDR_SIZE) + shift, copy);
			if (ret_len) {
				BTMTK_ERR("copy to user fail, copy = %ld, ret_len = %zd, count = %zd",
							ret_len, copy, count);
			}

			shift += copy;
			i++;
			retval -= (HCI_SCO_HDR_SIZE + copy);
			BTMTK_DBG("copy: %d, shift: %d, retval: %d", (int)copy, (int)shift, (int)retval);
		}
		kfree_skb(skb);
		retval = shift;			/* 3 bytes header removed */
	}

OUT:
	up(&g_sco->isoc_rd_mtx);
	BTMTK_DBG("Read: %d", (int)retval);
	return retval;
}

static ssize_t btmtk_fops_sco_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	int real_num;
	int pos = 0;
	int remain = 0;
	int multiple = 0;
	int iso_packet_size = 0;
	int i = 0, ret = 0;
	struct sk_buff *skb = NULL;
	u8 *tmp = NULL;

	if (g_sco == NULL || buf == NULL || count <= 0) {
		BTMTK_ERR("%s: ERROR, %s is NULL!", __func__,
				g_sco == NULL ? "g_data" : buf == NULL ? "buf" : "count");
		return -ENODEV;
	}

	BTMTK_DBG("%s start(%d)", __func__, (int)count);
	/* semaphore mechanism, the waited process will sleep */
	down(&g_sco->isoc_wr_mtx);
	ret = btmtk_usb_set_isoc_interface(false);
	if (ret < 0 || g_sco->isoc_urb_submitted == 0)
		goto free_skb;

	if (g_sco->isoc_alt_setting == ISOC_IF_ALT_MSBC)
		iso_packet_size = ISOC_HCI_PKT_SIZE_MSBC;
	else if (g_sco->isoc_alt_setting == ISOC_IF_ALT_CVSD)
		iso_packet_size = ISOC_HCI_PKT_SIZE_CVSD;
	else
		iso_packet_size = ISOC_HCI_PKT_SIZE_DEFAULT;

	g_sco->o_sco_buf[0] = HCI_SCODATA_PKT;
	tmp = g_sco->o_sco_buf + 1;

	real_num = (SCO_BUFFER_SIZE - 1) / iso_packet_size;
	/* check remain buffer, could send more data but not a whole iso_packet_size */
	if ((SCO_BUFFER_SIZE - 1) > (real_num * iso_packet_size + HCI_SCO_HDR_SIZE))
		real_num += 1;

	/* Upper layer should take care if write size more then driver buffer */
	if (count > SCO_BUFFER_SIZE - 1 - (real_num * HCI_SCO_HDR_SIZE)) {
		BTMTK_WARN("%s: Write length more than driver buffer size(%d/%d)",
				__func__, (int)count, SCO_BUFFER_SIZE - 1 - real_num * HCI_SCO_HDR_SIZE);
		count = SCO_BUFFER_SIZE - 1 - (real_num * HCI_SCO_HDR_SIZE);
	}

	multiple = count / (iso_packet_size - HCI_SCO_HDR_SIZE);

	if (count % (iso_packet_size - HCI_SCO_HDR_SIZE))
		multiple += 1;

	remain = count;
	BTMTK_DBG("remain = %d, multiple = %d", remain, multiple);
	for (i = 0; i < multiple; i++) {
		*tmp = (u8)(sco_handle & 0x00FF);
		*(tmp + 1) = sco_handle >> 8;
		*(tmp + 2) = remain < iso_packet_size - HCI_SCO_HDR_SIZE
			? remain : iso_packet_size - HCI_SCO_HDR_SIZE;
		remain -= *(tmp + 2);
		BTMTK_DBG("remain = %d, pkt_len = %d", remain, *(tmp + 2));

		if (copy_from_user(tmp + 3, buf + pos, *(tmp + 2))) {
			ret = -EFAULT;
			BTMTK_ERR("%s: copy data from user fail", __func__);
			goto free_skb;
		}
		pos += *(tmp + 2);
		tmp += (3 + *(tmp + 2));
	}

	skb = alloc_skb((count - remain) + HCI_SCO_HDR_SIZE * multiple + BT_SKB_RESERVE, GFP_ATOMIC);
	if (!skb) {
		BTMTK_ERR("%s allocate skb failed!!", __func__);
		ret = -ENOMEM;
		goto exit;
	}

	/* send HCI command */
	bt_cb(skb)->pkt_type = HCI_SCODATA_PKT;

	memcpy(skb->data, g_sco->o_sco_buf, (count - remain) + HCI_SCO_HDR_SIZE * multiple + 1);
	skb->len = (count - remain) + HCI_SCO_HDR_SIZE * multiple + 1;
	ret = btmtk_usb_send_cmd(g_sco->bdev, skb, 0, 0, (int)BTMTK_TX_PKT_FROM_HOST);
	if (ret < 0) {
		BTMTK_ERR("%s failed!!", __func__);
		goto free_skb;
	} else
		BTMTK_DBG("%s: OK", __func__);

	BTMTK_DBG("%s: Write end(len: %d)", __func__, skb->len);
	ret = count;
	goto exit;

free_skb:
	kfree_skb(skb);
	skb = NULL;

exit:
	up(&g_sco->isoc_wr_mtx);
	return ret;
}

static int btmtk_fops_sco_open(struct inode *inode, struct file *file)
{
	unsigned long flags = 0;

	if (g_sco == NULL) {
		BTMTK_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	atomic_set(&g_sco->isoc_out_count, 0);
	spin_lock_irqsave(&g_sco->isoc_lock, flags);
	skb_queue_purge(&g_sco->isoc_in_queue);
	spin_unlock_irqrestore(&g_sco->isoc_lock, flags);
	BTMTK_INFO("%s: OK", __func__);
	return 0;
}

static int btmtk_fops_sco_close(struct inode *inode, struct file *file)
{
	unsigned long flags = 0;
	int ret = 0;

	if (g_sco == NULL) {
		BTMTK_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	ret = btmtk_usb_set_isoc_interface(true);

	spin_lock_irqsave(&g_sco->isoc_lock, flags);
	skb_queue_purge(&g_sco->isoc_in_queue);
	spin_unlock_irqrestore(&g_sco->isoc_lock, flags);
	if (ret == 0)
		BTMTK_INFO("%s: OK", __func__);
	return ret;
}

static long btmtk_fops_sco_unlocked_ioctl(struct file *file, unsigned int cmd,
					  unsigned long arg)
{
	long retval = 0;

	if (g_sco == NULL) {
		BTMTK_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}
	return retval;
}

static unsigned int btmtk_fops_sco_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	if (g_sco == NULL) {
		BTMTK_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	if (skb_queue_len(&g_sco->isoc_in_queue) == 0) {
		poll_wait(file, &g_sco->inq_isoc, wait);

		/* empty let select sleep */
		if (skb_queue_len(&g_sco->isoc_in_queue) > 0)
			mask |= POLLIN | POLLRDNORM;		/* readable */
	} else
		mask |= POLLIN | POLLRDNORM;			/* readable */

	/* do we need condition? */
	mask |= POLLOUT | POLLWRNORM;				/* writable */

	return mask;
}

const struct file_operations BT_sco_fops = {
	.open = btmtk_fops_sco_open,
	.release = btmtk_fops_sco_close,
	.read = btmtk_fops_sco_read,
	.write = btmtk_fops_sco_write,
	.poll = btmtk_fops_sco_poll,
	.unlocked_ioctl = btmtk_fops_sco_unlocked_ioctl,
};

static int btmtk_usb_fops_sco_init(struct btmtk_dev *bdev)
{
	static int BT_major_sco;
	dev_t devID_sco = MKDEV(BT_major_sco, 0);
	int ret = 0;
	int cdevErr = 0;
	int major_sco = 0;

	BTMTK_INFO("%s: Start", __func__);

	if (!bdev->bt_cfg.support_usb_sco_test) {
		BTMTK_WARN("%s: not support sco test", __func__);
		return -1;
	}

	if (g_sco == NULL) {
		g_sco = kzalloc(sizeof(*g_sco), GFP_KERNEL);
		if (!g_sco) {
			BTMTK_ERR("%s: alloc memory fail (g_sco)", __func__);
			return -1;
		}
	}

	if (g_sco->o_sco_buf == NULL) {
		g_sco->o_sco_buf = kzalloc(SCO_BUFFER_SIZE, GFP_KERNEL);
		if (g_sco->o_sco_buf == NULL) {
			BTMTK_ERR("%s: alloc memory fail (g_data->o_sco_buf)",
				__func__);
			return -1;
		} else
			memset(g_sco->o_sco_buf, 0, SCO_BUFFER_SIZE);
	}

	BTMTK_INFO("%s: g_sco init", __func__);
	spin_lock_init(&g_sco->isoc_lock);
	skb_queue_head_init(&g_sco->isoc_in_queue);
	init_waitqueue_head(&g_sco->inq_isoc);
	sema_init(&g_sco->isoc_wr_mtx, 1);
	sema_init(&g_sco->isoc_rd_mtx, 1);
	sco_handle = 0;
	g_sco->isoc_urb_submitted = 0;

	/* device node for sco */
	ret = alloc_chrdev_region(&devID_sco, 0, 1, BT_CHR_DEV_SCO);
	if (ret) {
		BTMTK_ERR("%s: fail to allocate chrdev", __func__);
		return ret;
	}

	BT_major_sco = major_sco = MAJOR(devID_sco);

	cdev_init(&g_sco->BT_cdevsco, &BT_sco_fops);
	g_sco->BT_cdevsco.owner = THIS_MODULE;

	cdevErr = cdev_add(&g_sco->BT_cdevsco, devID_sco, 1);
	if (cdevErr)
		goto error;

	g_sco->pBTClass = class_create(THIS_MODULE, BT_CHR_DEV_SCO);
	if (IS_ERR(g_sco->pBTClass)) {
		BTMTK_ERR("%s: class create fail, error code(%ld)", __func__, PTR_ERR(g_sco->pBTClass));
		goto err1;
	}

	g_sco->pBTDevsco = device_create(g_sco->pBTClass, NULL, devID_sco, NULL, BT_DEV_NODE_SCO);
	if (IS_ERR(g_sco->pBTDevsco)) {
		BTMTK_ERR("device(stpbt_sco) create fail, error code(%ld)", PTR_ERR(g_sco->pBTDevsco));
		goto error;
	}

	g_sco->g_devIDsco = devID_sco;
	BTMTK_INFO("%s: BT_major_sco %d, devID_sco %d", __func__, BT_major_sco, devID_sco);


	return 0;

err1:
	if (g_sco->pBTClass) {
		class_destroy(g_sco->pBTClass);
		g_sco->pBTClass = NULL;
	}

error:
	if (cdevErr == 0)
		cdev_del(&g_sco->BT_cdevsco);

	if (ret == 0)
		unregister_chrdev_region(devID_sco, 1);

	return -1;
}

static int btmtk_usb_fops_sco_exit(struct btmtk_dev *bdev)
{
	dev_t devID_sco = g_sco->g_devIDsco;

	BTMTK_INFO("%s: Start", __func__);

	if (!bdev->bt_cfg.support_usb_sco_test) {
		BTMTK_WARN("%s: not support sco test", __func__);
		return -1;
	}

	if (g_sco->pBTDevsco) {
		device_destroy(g_sco->pBTClass, devID_sco);
		g_sco->pBTDevsco = NULL;
	}

	if (g_sco->pBTClass) {
		class_destroy(g_sco->pBTClass);
		g_sco->pBTClass = NULL;
	}

	if (g_sco->o_sco_buf) {
		kfree(g_sco->o_sco_buf);
		g_sco->o_sco_buf = NULL;
	}

	BTMTK_INFO("%s: pBTDevsco, pBTClass done", __func__);
	cdev_del(&g_sco->BT_cdevsco);
	unregister_chrdev_region(devID_sco, 1);
	BTMTK_INFO("%s: BT_chrdevsco driver removed", __func__);
	kfree(g_sco);
	g_sco = NULL;

	return 0;
}
#endif

static void btmtk_usb_dump_debug_register(struct btmtk_dev *bdev,
		struct debug_reg_struct debug_reg)
{
	u32 value = 0, i = 0, count = 0;
	static u32 reg_page[] = {0, 0};

	count = debug_reg.num;
	for (i = 0; i < count; i++) {
		if (!debug_reg.reg[i].length)
			continue;

		switch (debug_reg.reg[i].length) {
		case 1:
			/* reg read address */
			if (btmtk_cif_read_uhw_register(bdev,
				debug_reg.reg[i].content[0], &value))
				return;
			BTMTK_INFO("%s R(0x%08X) = 0x%08X",
				__func__,
				debug_reg.reg[i].content[0], value);
			break;
		case 2:
			/* write reg address and value */
			if (btmtk_cif_write_uhw_register(bdev, debug_reg.reg[i].content[0],
				debug_reg.reg[i].content[1]))
				return;
			reg_page[0] = debug_reg.reg[i].content[0];
			reg_page[1] = debug_reg.reg[i].content[1];
			BTMTK_INFO("%s W(0x%08X) = 0x%08X",
				__func__,
				debug_reg.reg[i].content[0], debug_reg.reg[i].content[1]);
			break;
		case 3:
			/* write reg and read reg */
			if (btmtk_cif_write_uhw_register(bdev, debug_reg.reg[i].content[0],
				debug_reg.reg[i].content[1]))
				return;
			if (btmtk_cif_read_uhw_register(bdev,
				debug_reg.reg[i].content[2], &value))
				return;
			BTMTK_INFO("%s W(0x%08X) = 0x%08X, W(0x%08X) = 0x%08X, R(0x%08X) = 0x%08X",
				__func__,
				reg_page[0], reg_page[1],
				debug_reg.reg[i].content[0], debug_reg.reg[i].content[1],
				debug_reg.reg[i].content[2], value);
			break;
		default:
			BTMTK_WARN("%s: Unknown result: %d", __func__, debug_reg.reg[i].length);
			break;
		}
	}
}

int btmtk_r_debug_cr(struct btmtk_dev *bdev, char* section_name, u32 dump_index, u32 dump_r_addr, bool uhw, u32 *val)
{
	int status, retry = RETRY_CR_BOUNDARY;

	do {
		if (uhw)
			status = btmtk_cif_read_uhw_register(bdev, dump_r_addr, val);
		else
			status = btmtk_usb_read_register(bdev, dump_r_addr, val);

		if (!status) {
			if (memcmp(section_name, "Current PC", sizeof("Current PC")) == 0
						|| memcmp(section_name, "PC log index", sizeof("PC log index")) == 0
						|| memcmp(section_name, "LR log index", sizeof("LR log index")) == 0) {
				BTMTK_INFO("%s: %s=0x%08X", __func__, section_name, *val);
			} else if (memcmp(section_name, "PC log", sizeof("PC log")) == 0
				|| memcmp(section_name, "LR log", sizeof("LR log")) == 0) {
				BTMTK_INFO("%s: %s(%d)=0x%08X", __func__, section_name, dump_index, *val);
			} else if (strlen(section_name) >= PSOP_STRING_LEN) {
				BTMTK_INFO("%s: =%s%02d=0x%08X=0x%08X", __func__, section_name, dump_index, *val, dump_r_addr);
			} else {
				BTMTK_INFO("%s: R(0x%08X) = 0x%08X", __func__, dump_r_addr, *val);
			}

			return status;
		}

		BTMTK_INFO("%s: read %x fail(%d)", __func__, dump_r_addr, retry);
	} while (--retry);

	return status;
}

int btmtk_w_debug_cr(struct btmtk_dev *bdev, u32 reg, u32 val, bool uhw)
{
	int status, retry = RETRY_CR_BOUNDARY;

	do {
		if (uhw)
			status = btmtk_cif_write_uhw_register(bdev, reg, val);
		else
			status = btmtk_usb_write_register(bdev, reg, val);

		if (!status)
			break;
	} while (--retry);

	return status;
}

int btmtk_wr_debug_cr(struct btmtk_dev *bdev, char* section_name, u32 dump_index,
								u32 flag_w_addr, u32 flag_w_val, u32 dump_r_addr, bool uhw, u32 *val)
{
	int status;

	status = btmtk_w_debug_cr(bdev, flag_w_addr, flag_w_val, uhw);
	if (status)
		return status;

	btmtk_r_debug_cr(bdev, section_name, dump_index, dump_r_addr, uhw, val);

	return status;
}

void btmtk_show_dump_item(struct btmtk_dev *bdev, char* section_name, u32 dump_index,
							u32 flag_w_addr, u32 flag_r_addr, u32 dump_value_start, u32 dump_w_offset,
							u32 flag_num, bool uhw)
{
	int status, i;
	u32 val;
	u32 offset = 0;

	for (i = 0; i < flag_num; i++) {
		status = btmtk_wr_debug_cr(bdev, section_name, dump_index,
						flag_w_addr,dump_value_start + offset, flag_r_addr, uhw, &val);
		if (!status) {
			offset += dump_w_offset;
			dump_index += 1;
		} else
			break;
	}
}

void btmtk_usb_check_DBG_status_dump(struct btmtk_dev *bdev)
{
	int val;
	int r_index = 1;
	int check_conn_infra_clock = 0;
	int status;
	int i;

	if (is_connac3(bdev->chip_id)) {
		BTMTK_INFO("%s: Debug info is dumping. Chip ID: %04x",
					__func__, bdev->chip_id);
		status = btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023000, TRUE, &val);
		if (status) {
			BTMTK_INFO("%s: Avoid jamming, so end it directly.", __func__);
			return;
		}

		val = val | BIT(0);
		btmtk_w_debug_cr(bdev, 0x18023000, val, TRUE);

		// Check the same addr which may be different
		for (i = 0; i < POLLING_CR_BOUNDARY; i++) {
			btmtk_r_debug_cr(bdev, "PSOP_1_3_A", r_index, 0x18023000, TRUE, &val);
			check_conn_infra_clock = check_conn_infra_clock | val;
			r_index += 1;
		}

		BTMTK_INFO("%s: ======CONN_INFRA_STATUS======", __func__);
		if (is_mt6639(bdev->chip_id) && (check_conn_infra_clock & BIT(1)) && (check_conn_infra_clock & BIT(2))) {
			BTMTK_INFO("%s: CONN_INFRA Clock: PASS", __func__);
		} else if (is_mt7925(bdev->chip_id) && (check_conn_infra_clock & BIT(1)) && (check_conn_infra_clock & BIT(3))) {
			BTMTK_INFO("%s: CONN_INFRA Clock: PASS", __func__);
		} else {
			BTMTK_INFO("%s: CONN_INFRA Clock: Error", __func__);
		}

		btmtk_r_debug_cr(bdev, "PSOP_1_3_A", 6, 0x18011000, TRUE, &val);
		if (is_mt6639(bdev->chip_id) &&  ((val== 0x03010001) || (val == 0x03030002))) {
			BTMTK_INFO("%s: CONN_INFRA ID: PASS", __func__);
		} else if (is_mt7925(bdev->chip_id) && (val == 0x03030002)) {
			BTMTK_INFO("%s: CONN_INFRA ID: PASS", __func__);
		} else {
			BTMTK_INFO("%s: CONN_INFRA ID: Error", __func__);
		}

		btmtk_r_debug_cr(bdev, "PSOP_1_3_A", 7, 0x18023400, TRUE, &val);
		if (is_mt6639(bdev->chip_id) && ((val & 0x7) == 0x0)) {
			BTMTK_INFO("%s: CONN_INFRA bus timeout: PASS", __func__);
		} else if (is_mt7925(bdev->chip_id) && ((val & 0x3FF) == 0x0)) {
			BTMTK_INFO("%s: CONN_INFRA bus timeout: PASS", __func__);
		} else {
			BTMTK_INFO("%s: CONN_INFRA bus timeout: Error", __func__);
		}

		if (bdev->chip_id == 0x6639) {
			BTMTK_INFO("%s: ======WF_STATUS======", __func__);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18048A14, TRUE, &val);
			if ((val & BIT(26)) == 0x0) {
				BTMTK_INFO("%s: WF sleep protect: PASS", __func__);
			} else {
				BTMTK_INFO("%s: WF sleep protect: Error", __func__);
			}

			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x184B0010, TRUE, &val);
			if (val == 0x03010001) {
				BTMTK_INFO("%s: WF ID: PASS", __func__);
			} else {
				BTMTK_INFO("%s: WF ID: Error", __func__);
			}

			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1802362C, TRUE, &val);
			if ((val & BIT(0)) == 0x0) {
				BTMTK_INFO("%s: WF bus timeout: PASS", __func__);
			} else {
				BTMTK_INFO("%s:WF bus timeout: Error", __func__);
			}

			BTMTK_INFO("%s: ======BT_STATUS======", __func__);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011454, TRUE, &val);
			if (((val & BIT(22)) == 0x0) && ((val & BIT(23)) == 0x0)) {
				BTMTK_INFO("%s: BT sleep protect: PASS", __func__);
			} else {
				BTMTK_INFO("%s: BT sleep protect: Error", __func__);
			}

			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18812000, TRUE, &val);
			if (val == 0x03000000) {
				BTMTK_INFO("%s: BT ID: PASS", __func__);
			} else {
				BTMTK_INFO("%s: BT ID: Error", __func__);
			}

			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023A0C, TRUE, &val);
		} else if (bdev->chip_id == 0x7925) {
			BTMTK_INFO("%s: ======WF_STATUS======", __func__);
			btmtk_r_debug_cr(bdev, "PSOP_1_3_A", 8, 0x1804F304, TRUE, &val);
			if ((val & BIT(26)) == 0x0) {
				BTMTK_INFO("%s: WF sleep protect: PASS", __func__);
			} else {
				BTMTK_INFO("%s: WF sleep protect: Error", __func__);
			}

			btmtk_r_debug_cr(bdev, "PSOP_1_3_A", 9, 0x184B0010, TRUE, &val);
			if (val == 0x03030002) {
				BTMTK_INFO("%s: WF ID: PASS", __func__);
			} else {
				BTMTK_INFO("%s: WF ID: Error", __func__);
			}

			btmtk_r_debug_cr(bdev, "PSOP_1_3_A", 10, 0x1802362C, TRUE, &val);
			if ((val & BIT(0)) == 0x0) {
				BTMTK_INFO("%s: WF bus timeout: PASS", __func__);
			} else {
				BTMTK_INFO("%s: WF bus timeout: Error", __func__);
			}

			BTMTK_INFO("%s: ======BT_STATUS======", __func__);
			btmtk_r_debug_cr(bdev, "PSOP_1_3_A", 11, 0x18011454, TRUE, &val);
			if (((val & BIT(22)) == 0x0) && ((val & BIT(23)) == 0x0)) {
				BTMTK_INFO("%s: BT sleep protect: PASS", __func__);
			} else {
				BTMTK_INFO("%s: BT sleep protect: Error", __func__);
			}

			btmtk_r_debug_cr(bdev, "PSOP_1_3_A", 12, 0x18812000, TRUE, &val);
			if (val == 0x03020000) {
				BTMTK_INFO("%s: BT ID: PASS", __func__);
			} else {
				BTMTK_INFO("%s: BT ID: Error", __func__);
			}

			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18001108, TRUE, &val);
			val = val & ~(BIT(16) | BIT(17));
			btmtk_w_debug_cr(bdev, 0x18001108, val, TRUE);
			btmtk_w_debug_cr(bdev, 0x18023A0C, 0xC0041F00, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023A10, TRUE, &val);
			if ((val & BIT(24)) != 0x0) {
				BTMTK_INFO("%s: BT bus timeout: PASS", __func__);
			} else {
				BTMTK_INFO("%s: BT bus timeout: Error", __func__);
			}

			btmtk_r_debug_cr(bdev, "PSOP_1_3_A", 13, 0x18011474, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_3_A", 14, 0x18C12000, TRUE, &val);
			btmtk_wr_debug_cr(bdev, "PSOP_1_3_A", 15, 0x18023A0C, 0xC0041F00, 0x18023A10, TRUE, &val);
		}
	}

	BTMTK_INFO("%s: END", __func__);
}

void btmtk_usb_pc_lr_dump(struct btmtk_dev *bdev)
{
	u32 val, w_value;
	int status, i;

	BTMTK_INFO("%s: Debug info is dumping. Chip ID: %04X", __func__, bdev->chip_id);

	if (is_connac3(bdev->chip_id)) {
		BTMTK_INFO("%s: BT PC/LR LOG", __func__);

		// Check Current PC which may be different
		for (i = 0; i < POLLING_CURRENT_PC; i++) {
			status = btmtk_wr_debug_cr(bdev, "Current PC", 0, 0x18023A0C, 0xC0040D2A, 0x18023A10, TRUE, &val);
			if (status) {
				BTMTK_INFO("%s: Avoid jamming, so end it directly.", __func__);
				return;
			}
		}

		btmtk_wr_debug_cr(bdev, "PC log index", 0, 0x18023A0C, 0xC0040D20, 0x18023A10, TRUE, &val);
		btmtk_show_dump_item(bdev, "PC log", 0, 0x18023A0C, 0x18023A10, 0xC0040D00, 0x1, 0x20, TRUE);
		btmtk_wr_debug_cr(bdev, "LR log index", 0, 0x18023A0C, 0xC0040D4C, 0x18023A10, TRUE, &val);
		btmtk_show_dump_item(bdev, "LR log", 0, 0x18023A0C, 0x18023A10, 0xC0040D2C, 0x1, 0x20, TRUE);

		BTMTK_INFO("%s: MCU common register", __func__);
		btmtk_show_dump_item(bdev, "SKIP", 0, 0x18023A0C, 0x18023A10, 0xC0040D21, 0x1, 0x9, TRUE);

		BTMTK_INFO("%s: WF PC/LR LOG", __func__);

		// Check Current PC which may be different
		for (i = 0; i < POLLING_CURRENT_PC; i++) {
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1802360C, TRUE, &val);
			val = (val & 0b11111111111111111111111111000000) | 0x22;
			btmtk_w_debug_cr(bdev, 0x1802360C, val, TRUE);
			btmtk_r_debug_cr(bdev, "Current PC", 0, 0x18023610, TRUE, &val);
		}

		btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1802360C, TRUE, &val);
		val = (val & 0b11111111111111111111111111000000) | 0x20;
		btmtk_w_debug_cr(bdev, 0x1802360C, val, TRUE);
		btmtk_r_debug_cr(bdev, "PC log index", 0, 0x18023610, TRUE, &val);
		for (w_value = 0x0; w_value < 0x20; w_value++) {
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1802360C, TRUE, &val);
			val = (val & 0b11111111111111111111111111000000) | w_value;
			btmtk_w_debug_cr(bdev, 0x1802360C, val, TRUE);
			btmtk_r_debug_cr(bdev, "PC log", 0, 0x18023610, TRUE, &val);
		}

		btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023614, TRUE, &val);
		val = (val & 0b11111111111111111111111111000000) | 0x20;
		btmtk_w_debug_cr(bdev, 0x18023614, val, TRUE);
		btmtk_r_debug_cr(bdev, "LR log index", 0, 0x18023608, TRUE, &val);
		for (w_value = 0x0; w_value < 0x20; w_value++) {
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023614, TRUE, &val);
			val = (val & 0b11111111111111111111111111000000) | w_value;
			btmtk_w_debug_cr(bdev, 0x18023614, val, TRUE);
			btmtk_r_debug_cr(bdev, "LR log", 0, 0x18023608, TRUE, &val);
		}
	}

	BTMTK_INFO("%s: END", __func__);
}

void btmtk_usb_CONN_TOP_SPI_RD(struct btmtk_dev *bdev, char* section_name, u32 dump_index, u32 dump_r_addr, bool uhw)
{
	int status;
	u32 val = 0;

	// Driver need to read A die CR with RFSPI_RD_CMD sequence
	if (is_connac3(bdev->chip_id)) {
		btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18098000, uhw, &val);
		if ((val & BIT(5)) != BIT(5)) {
			val = 0x0000B000 | dump_r_addr;
			btmtk_w_debug_cr(bdev, 0x18098050, val, uhw);
			btmtk_w_debug_cr(bdev, 0x18098054, 0x0, uhw);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18098000, uhw, &val);
			if ((val & BIT(5)) != BIT(5)) {
				status = btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18098058, uhw, &val);
				if ((!status) && (strlen(section_name) >= PSOP_STRING_LEN))
					BTMTK_INFO("%s: =%s%02d=0x%08X=0x%08X", __func__,
						section_name, dump_index, val, dump_r_addr);
			}
		}
	}
}


void btmtk_usb_slave_no_response_dump(struct btmtk_dev *bdev, u8 mode)
{
	u32 val = 0, version, r_address, r_index;
	bool  check = FALSE;
	int status, i, j;

	BTMTK_INFO("%s: mode: %02X", __func__, mode);

	if (is_connac3(bdev->chip_id)) {
		if (bdev->chip_id == 0x6639) {
			BTMTK_INFO("%s: 8.CB_INFRA_CBTOP - sheet(Type 1)", __func__);
			status = btmtk_r_debug_cr(bdev, "SKIP", 0, 0x7002801C, TRUE, &val);
			if (status) {
				BTMTK_INFO("%s: Avoid jamming, so end it directly.", __func__);
				return;
			}

			val = val | 0x10000000;
			btmtk_w_debug_cr(bdev, 0x7002801C, val, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025300, TRUE, &val);
			val = val | 0x00000001;
			btmtk_w_debug_cr(bdev, 0x70025300, val, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011404, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023C00, TRUE, &val);

			BTMTK_INFO("%s: 8.CB_INFRA_CBTOP - sheet(Type 2)",	__func__);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026008, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1800EA04, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025300, TRUE, &val);
			val = val | 0x00001110;
			btmtk_w_debug_cr(bdev, 0x70025300, val, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025304, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025300, TRUE, &val);
			val = val | 0x00000E0F;
			btmtk_w_debug_cr(bdev, 0x70025300, val, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025304, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025300, TRUE, &val);
			val = val | 0x00000C0D;
			btmtk_w_debug_cr(bdev, 0x70025300, val, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025304, TRUE, &val);

			BTMTK_INFO("%s: 8.CB_INFRA_CBTOP - sheet(Type 3)", __func__);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x7002801C, TRUE, &val);
			val = val | 0x10000000;
			btmtk_w_debug_cr(bdev, 0x7002801C, val, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025300, TRUE, &val);
			val = val | 0x00000001;
			btmtk_w_debug_cr(bdev, 0x70025300, val, TRUE);
			btmtk_w_debug_cr(bdev, 0x70006150, 0x2, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011404, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026000, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70006154, TRUE, &val);

			BTMTK_INFO("%s: 8.CB_INFRA_CBTOP - sheet(Type 4)", __func__);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026040, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026050, TRUE, &val);

			BTMTK_INFO("%s: 8.CB_INFRA_CBTOP - sheet(WFDMA2AP)", __func__);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026100, TRUE, &val);
		} else if (bdev->chip_id == 0x7925) {
			BTMTK_INFO("%s: 8.CB_INFRA_CBTOP - presetting", __func__);
			status = btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026550, TRUE, &val);
			if (status) {
				BTMTK_INFO("%s: Avoid jamming, so end it directly.", __func__);
				return;
			}

			val = (val & 0xFFFF0000) | 0x0C0B;
			btmtk_w_debug_cr(bdev, 0x70026550, val, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025300, TRUE, &val);
			val = (val & 0xFFFFFF00) | 0x1;
			btmtk_w_debug_cr(bdev, 0x70025300, val, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x7002801C, TRUE, &val);
			val = val | BIT(28);
			btmtk_w_debug_cr(bdev, 0x7002801C, val, TRUE);

			BTMTK_INFO("%s: 8.CB_INFRA_CBTOP", __func__);
			btmtk_w_debug_cr(bdev, 0x70026558, 0x7000, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70007204, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x7002500C, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025004, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025014, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025400, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025404, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026008, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026000, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026100, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025300, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026550, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x7002801C, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026554, TRUE, &val);
			val = (val & 0x0000FFFF) | 0x74030000;
			btmtk_w_debug_cr(bdev, 0x70026554, val, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x74030E48, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x74030E40, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x74030E44, TRUE, &val);
			btmtk_w_debug_cr(bdev, 0x70003020, 0x0, TRUE);
			btmtk_w_debug_cr(bdev, 0x70007150, 0x2, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70007154, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x74030150, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x74030154, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x74030184, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x74031010, TRUE, &val);
			btmtk_w_debug_cr(bdev, 0x74030168, 0x22CC0100, TRUE);
			btmtk_w_debug_cr(bdev, 0x74030164, 0x81804845, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x7403002C, TRUE, &val);
		}

		BTMTK_INFO("%s: 2.connsys_power_debug - sheet(dump_list_conn_infra_on)", __func__);
		btmtk_r_debug_cr(bdev, "PSOP_2_1_B", 1, 0x18060A10, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_2_1_B", 2, 0x18060014, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_2_1_B", 3, 0x18060054, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_2_1_B", 4, 0x18060010, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_2_1_B", 5, 0x18060050, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_2_1_B", 6, 0x18060018, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_2_1_B", 7, 0x18060058, TRUE, &val);

		BTMTK_INFO("%s: 1.conn_infra_bus_debug - sheet(status result) - A", __func__);
		btmtk_w_debug_cr(bdev, 0x18023408, 0x00000000, TRUE);
		btmtk_show_dump_item(bdev, "PSOP_1_1_A", 1, 0x1802340C, 0x18023404, 0x00010001, 0x00010000, 0x3, TRUE);
		btmtk_wr_debug_cr(bdev, "PSOP_1_1_A", 4, 0x1802340C, 0x00010002, 0x18023404, TRUE, &val);
		btmtk_show_dump_item(bdev, "PSOP_1_1_A", 5, 0x1802340C, 0x18023404, 0x00000003, 0x00010000, 0x4, TRUE);
		btmtk_show_dump_item(bdev, "PSOP_1_1_A", 9, 0x1802340C, 0x18023404, 0x00010004, 0x00010000, 0x3, TRUE);
		btmtk_r_debug_cr(bdev, "PSOP_1_1_A", 12, 0x18023400, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_1_1_A", 13, 0x18023410, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_1_1_A", 14, 0x18023414, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_1_1_A", 15, 0x18023418, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_1_1_A", 16, 0x18023434, TRUE, &val);
		btmtk_r_debug_cr(bdev, "PSOP_1_1_A", 17, 0x1800EA04, TRUE, &val);

		BTMTK_INFO("%s: 7.conn_infra_cfg_clk - sheet(dump_list) - A", __func__);
		if (bdev->chip_id == 0x6639) {
			btmtk_r_debug_cr(bdev, "PSOP_7_1_A", 1, 0x18060A00, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_7_1_A", 2, 0x18060A0C, TRUE, &val);
			btmtk_show_dump_item(bdev, "PSOP_7_1_A", 3, 0x1806015C, 0x18060A04, 0x0, 0x1, 0x8, TRUE);
			btmtk_show_dump_item(bdev, "PSOP_7_1_A", 11, 0x18060160, 0x18060A08, 0x0, 0x1, 0x6, TRUE);
		} else if (bdev->chip_id == 0x7925) {
			btmtk_r_debug_cr(bdev, "PSOP_8_1_A", 1, 0x18060A00, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_8_1_A", 2, 0x18060A0C, TRUE, &val);
			btmtk_show_dump_item(bdev, "PSOP_8_1_A", 3, 0x1806015C, 0x18060A04, 0x0, 0x1, 0x8, TRUE);
			btmtk_show_dump_item(bdev, "PSOP_8_1_A", 11, 0x18060160, 0x18060A08, 0x0, 0x1, 0x6, TRUE);
			btmtk_wr_debug_cr(bdev, "PSOP_8_1_A", 17, 0x18060160, 0x7, 0x18060A08, TRUE, &val);
			btmtk_wr_debug_cr(bdev, "PSOP_8_1_A", 18, 0x18060160, 0x8, 0x18060A08, TRUE, &val);
		}

		// AP2CONN_INFRA OFF
		for (j = 0; j <= RETRY_CR_BOUNDARY; j++) {
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023000, TRUE, &val);
			val = val | BIT(0);
			btmtk_w_debug_cr(bdev, 0x18023000, val, TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023000, TRUE, &val);
		}

		btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011000, TRUE, &version);
		if (is_mt6639(bdev->chip_id)) {
			if ((val & BIT(1)) && (val & BIT(3)) &&
				((version == 0x03010001) || (version == 0x03010002))) {
				BTMTK_INFO("%s: AP2CONN_INFRA_OFF readable", __func__);
				check = TRUE;
			}
		} else if (is_mt7925(bdev->chip_id)) {
			if ((val & BIT(1)) && (val & BIT(3)) && (version == 0x03030002)) {
				BTMTK_INFO("%s: AP2CONN_INFRA_OFF readable", __func__);
				check = TRUE;
			}
		}

		if (check || mode) {
			check = FALSE;
			BTMTK_INFO("%s: 2.connsys_power_debug - sheet(dump_list_conn_infra_off)", __func__);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 1, 0x18011030, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 2, 0x18012050, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 3, 0x18001344, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 4, 0x18000400, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 5, 0x18000404, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 6, 0x180910A8, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 7, 0x18091120, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 8, 0x18091124, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 9, 0x18091128, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 10, 0x1809112C, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 11, 0x18091130, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 12, 0x18091134, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
			val = val & ~(BIT(21) | BIT(22));
			btmtk_wr_debug_cr(bdev, "PSOP_2_1_C", 13, 0x18011100, val, 0x18011134, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
			val = (val & ~BIT(22)) | BIT(21);
			btmtk_wr_debug_cr(bdev, "PSOP_2_1_C", 14, 0x18011100, val, 0x18011134, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
			val = (val & ~BIT(21)) | BIT(22);
			btmtk_wr_debug_cr(bdev, "PSOP_2_1_C", 15, 0x18011100, val, 0x18011134, TRUE, &val);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
			val = (val | BIT(21)) | BIT(22);
			btmtk_wr_debug_cr(bdev, "PSOP_2_1_C", 16, 0x18011100, val, 0x18011134, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 17, 0x18050C50, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 18, 0x18050C54, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 19, 0x18050C58, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 20, 0x18050C5C, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 21, 0x18050C60, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 22, 0x18050C64, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 23, 0x18098000, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 24, 0x18098050, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 25, 0x18098054, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 26, 0x18098058, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 27, 0x18098108, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 28, 0x18098004, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 29, 0x18001620, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 30, 0x18001610, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_2_1_C", 31, 0x18001600, TRUE, &val);

			if (is_mt6639(bdev->chip_id)) {
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 1, 0xA10, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 2, 0x090, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 3, 0x08C, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 4, 0x0A0, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 5, 0x09C, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 6, 0xA40, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 7, 0xA48, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 8, 0x8E0, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 9, 0x094, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 10, 0x5B4, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 11, 0x2CC, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 12, 0x84C, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 13, 0x860, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 14, 0x861, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 15, 0xA70, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 16, 0xA20, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 17, 0x850, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 18, 0x85D, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 19, 0xC08, TRUE);
			} else if (is_mt7925(bdev->chip_id)) {
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 1, 0xA10, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 2, 0x090, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 3, 0x08C, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 4, 0x0A0, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 5, 0x09C, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 9, 0x094, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 10, 0x5B4, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 11, 0x2CC, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 15, 0xA70, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 16, 0xA20, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 19, 0xC08, TRUE);
				btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_D", 20, 0xAA0, TRUE);
			}

			btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_E", 1, 0x02C, TRUE);
			btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_E", 2, 0x000, TRUE);
			btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_E", 3, 0x750, TRUE);
			btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_E", 4, 0xC50, TRUE);
			btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_E", 5, 0xB08, TRUE);
			btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_E", 6, 0x0B4, TRUE);
			btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_E", 7, 0x580, TRUE);
			btmtk_usb_CONN_TOP_SPI_RD(bdev, "PSOP_2_1_E", 8, 0x30C, TRUE);

			BTMTK_INFO("%s: 1.conn_infra_bus_debug - sheet(status result) - B & C", __func__);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 1, 0x1802341C, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 2, 0x18023420, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 3, 0x18023424, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 4, 0x18023428, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 5, 0x1802342C, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 6, 0x18023430, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 7, 0x1800E128, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 8, 0x1800E12C, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 9, 0x1800E130, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 10, 0x1800E134, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 11, 0x1800E138, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 12, 0x1800E13C, TRUE, &val);
			btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 13, 0x1800E140, TRUE, &val);
			btmtk_show_dump_item(bdev, "PSOP_1_1_B", 14, 0x18023408, 0x18023404, 0x13, 0x1, 0x1B, TRUE);
			btmtk_show_dump_item(bdev, "PSOP_1_1_B", 41, 0x18023408, 0x18023404, 0x1, 0x1, 0x12, TRUE);

			if (is_mt6639(bdev->chip_id)) {
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 59, 0x18048280, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 60, 0x18048284, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 61, 0x18048288, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 62, 0x1804828C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 63, 0x18048290, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 64, 0x18049408, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 65, 0x1804940C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 66, 0x18049410, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 67, 0x18049414, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 68, 0x18049418, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 69, 0x1804941C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 70, 0x18049420, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 71, 0x18049424, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 72, 0x18049428, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 73, 0x1804942C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 74, 0x18049430, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 75, 0x18048320, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 76, 0x18048400, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 77, 0x18040D04, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 78, 0x18040D08, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 79, 0x18040D0C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 80, 0x18048310, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 81, 0x18048314, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 82, 0x18048318, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 83, 0x1804831C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_2_A", 1, 0x18011404, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_2_A", 2, 0x18011408, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_2_A", 3, 0x18011434, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_2_A", 4, 0x18011444, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_2_A", 5, 0x18011454, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_2_A", 6, 0x18011464, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_2_A", 7, 0x18011474, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_2_A", 8, 0x18011484, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_2_A", 9, 0x1801148C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_2_A", 10, 0x70028730, TRUE, &val);
			} else if (bdev->chip_id == 0x7925) {
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 59, 0x1804F280, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 60, 0x1804F284, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 61, 0x1804F288, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 62, 0x1804F28C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 63, 0x1804F290, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 64, 0x1804F294, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 65, 0x1804F298, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 66, 0x1804F29C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 67, 0x1804F2A0, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 68, 0x18049408, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 69, 0x1804940C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 70, 0x18049410, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 71, 0x18049414, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 72, 0x18049418, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 73, 0x1804941C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 74, 0x18049420, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 75, 0x18049424, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 76, 0x18049428, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 77, 0x1804942C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 78, 0x18049430, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 79, 0x1804F650, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 80, 0x1804F700, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 81, 0x18060D04, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 82, 0x18060D08, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 83, 0x18060D0C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 84, 0x1804F610, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 85, 0x1804F614, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 86, 0x1804F618, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_B", 87, 0x1804F61C, TRUE, &val);
				btmtk_wr_debug_cr(bdev, "PSOP_1_1_B", 88, 0x18023408, 0x0000002E, 0x18023404, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 1, 0x18011404, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 2, 0x18011408, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 3, 0x18011434, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 4, 0x180114A4, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 5, 0x18011454, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 6, 0x18011464, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 7, 0x18011474, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 8, 0x18011484, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 9, 0x1801148C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 10, 0x1801149C, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 11, 0x180114AC, TRUE, &val);
				btmtk_r_debug_cr(bdev, "PSOP_1_1_C", 12, 0x70028730, TRUE, &val);
			}

			BTMTK_INFO("%s: 7.conn_infra_cfg_clk - sheet(dump_list) - B", __func__);
			if (is_mt6639(bdev->chip_id)) {
				btmtk_r_debug_cr(bdev, "PSOP_7_1_B", 1, 0x18023200, TRUE , &val);
				btmtk_r_debug_cr(bdev, "PSOP_7_1_B", 2, 0x18011130, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE , &val);
				val = val & ~(BIT(21) | BIT(22));
				btmtk_wr_debug_cr(bdev, "PSOP_7_1_B", 3, 0x18011100, val, 0x18011134, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE , &val);
				val = (val & ~BIT(22)) | BIT(21);
				btmtk_wr_debug_cr(bdev, "PSOP_7_1_B", 4, 0x18011100, val, 0x18011134, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE , &val);
				val = (val & ~BIT(21)) | BIT(22);
				btmtk_wr_debug_cr(bdev, "PSOP_7_1_B", 5, 0x18011100, val, 0x18011134, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE , &val);
				val = val | BIT(21) | BIT(22);
				btmtk_wr_debug_cr(bdev, "PSOP_7_1_B", 6, 0x18011100, val, 0x18011134, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE , &val);
				val = (val & 0b11111111111111111111111111000000) | 0b1110;
				btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE , &val);
				val = (val & 0b11111111111111111111000000111111) | 0b10001000000;
				btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
				btmtk_w_debug_cr(bdev, 0x1801601C, 0x03020100, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016020, 0x07060504, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016024, 0x0B0A0908, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016028, 0x0F0E0D0C, TRUE);
				btmtk_w_debug_cr(bdev, 0x1801602C, 0x13121110, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016030, 0x17161514, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016034, 0x1B1A1918, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016038, 0x1F1E1D1C, TRUE);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18016058, TRUE , &val);
				val = val | BIT(1);
				btmtk_w_debug_cr(bdev, 0x18016058, val, TRUE);
				btmtk_r_debug_cr(bdev, "PSOP_7_1_B", 7, 0x18023200, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE , &val);
				val = (val & 0b11111111111111111111111111000000) | 0b11000;
				btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
				btmtk_w_debug_cr(bdev, 0x1801601C, 0x03020100, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016020, 0x07060504, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016024, 0x0B0A0908, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016028, 0x0F0E0D0C, TRUE);
				btmtk_w_debug_cr(bdev, 0x1801602C, 0x13121110, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016030, 0x17161514, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016034, 0x1B1A1918, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016038, 0x1F1E1D1C, TRUE);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18016058, TRUE , &val);
				val = val | BIT(1);
				btmtk_w_debug_cr(bdev, 0x18016058, val, TRUE);
				btmtk_r_debug_cr(bdev, "PSOP_7_1_B", 8, 0x18023200, TRUE , &val);

			} else if (is_mt7925(bdev->chip_id)) {
				btmtk_r_debug_cr(bdev, "PSOP_8_1_B", 1, 0x18023200, TRUE , &val);
				btmtk_r_debug_cr(bdev, "PSOP_8_1_B", 2, 0x18011130, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE , &val);
				val = val & ~(BIT(21) | BIT(22));
				btmtk_wr_debug_cr(bdev, "PSOP_8_1_B", 3, 0x18011100, val, 0x18011134, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE , &val);
				val = (val & ~BIT(22)) | BIT(21);
				btmtk_wr_debug_cr(bdev, "PSOP_8_1_B", 4, 0x18011100, val, 0x18011134, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE , &val);
				val = (val & ~BIT(21)) | BIT(22);
				btmtk_wr_debug_cr(bdev, "PSOP_8_1_B", 5, 0x18011100, val, 0x18011134, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE , &val);
				val = val | BIT(21) | BIT(22);
				btmtk_wr_debug_cr(bdev, "PSOP_8_1_B", 6, 0x18011100, val, 0x18011134, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE , &val);
				val = (val & 0b11111111111111111111111111000000) | 0b1110;
				btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE , &val);
				val = (val & 0b11111111111111111111000000111111) | 0b10001000000;
				btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
				btmtk_w_debug_cr(bdev, 0x1801601C, 0x03020100, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016020, 0x07060504, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016024, 0x0B0A0908, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016028, 0x0F0E0D0C, TRUE);
				btmtk_w_debug_cr(bdev, 0x1801602C, 0x13121110, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016030, 0x17161514, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016034, 0x1B1A1918, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016038, 0x1F1E1D1C, TRUE);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18016058, TRUE , &val);
				val = val | BIT(1);
				btmtk_w_debug_cr(bdev, 0x18016058, val, TRUE);
				btmtk_r_debug_cr(bdev, "PSOP_8_1_B", 7, 0x18023200, TRUE , &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE , &val);
				val = (val & 0b11111111111111111111111111000000) | 0b11000;
				btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
				btmtk_w_debug_cr(bdev, 0x1801601C, 0x03020100, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016020, 0x07060504, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016024, 0x0B0A0908, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016028, 0x0F0E0D0C, TRUE);
				btmtk_w_debug_cr(bdev, 0x1801602C, 0x13121110, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016030, 0x17161514, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016034, 0x1B1A1918, TRUE);
				btmtk_w_debug_cr(bdev, 0x18016038, 0x1F1E1D1C, TRUE);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18016058, TRUE , &val);
				val = val | BIT(1);
				btmtk_w_debug_cr(bdev, 0x18016058, val, TRUE);
				btmtk_r_debug_cr(bdev, "PSOP_8_1_B", 8, 0x18023200, TRUE , &val);

			}

			// Check WFSYS
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1802362C, TRUE, &val);
			if (val & BIT(0)) {
				BTMTK_INFO("%s: Go to WFSYS", __func__);
				check = TRUE;
			}

			if (check || mode) {
				check = FALSE;
				BTMTK_INFO("%s: 3.WF_TOP_debug - sheet(dump_list) - A & B", __func__);
				btmtk_w_debug_cr(bdev, 0x18060B00, 0x1, TRUE);
				btmtk_show_dump_item(bdev, "PSOP_3_1_A", 1, 0x18060B04, 0x18060B10, 0x0, 0x1, 0x5, TRUE);
				btmtk_wr_debug_cr(bdev, "PSOP_3_1_A", 6, 0x18060B04, 0x10, 0x18060B10, TRUE, &val);
				btmtk_wr_debug_cr(bdev, "PSOP_3_1_A", 7, 0x18060B04, 0x12, 0x18060B10, TRUE, &val);
				btmtk_show_dump_item(bdev, "PSOP_3_1_A", 8, 0x18060B04, 0x18060B10, 0x17, 0x1, 0x5, TRUE);
				btmtk_wr_debug_cr(bdev, "PSOP_3_1_A", 13, 0x18060B04, 0x1D, 0x18060B10, TRUE, &val);
				btmtk_w_debug_cr(bdev, 0x18060B18, 0x1, TRUE);
				btmtk_show_dump_item(bdev, "PSOP_3_1_B", 1, 0x18060B1C, 0x18023638, 0x0, 0x1, 0x5, TRUE);
				btmtk_wr_debug_cr(bdev, "PSOP_3_1_B", 6, 0x18060B1C, 0x8, 0x18023638, TRUE, &val);

				BTMTK_INFO("%s: 4.WF_BUS_debug - sheet(dump_list) - A", __func__);
				btmtk_r_debug_cr(bdev, "PSOP_4_1_A", 0, 0x1802362C, TRUE, &val);
				btmtk_w_debug_cr(bdev, 0x18023604, 0x4, TRUE);
				btmtk_show_dump_item(bdev, "PSOP_4_1_A", 1, 0x18023628, 0x18023608, 0x10001, 0x10000, 0x9, TRUE);
				if (is_mt6639(bdev->chip_id))
					btmtk_show_dump_item(bdev, "PSOP_4_1_A", 10, 0x18023628, 0x18023608, 0x10002, 0x10000, 0x5, TRUE);
				else if (is_mt7925(bdev->chip_id))
					btmtk_show_dump_item(bdev, "PSOP_4_1_A", 10, 0x18023628, 0x18023608, 0x10002, 0x10000, 0x4, TRUE);

				// Check [AP2WF] readable step2.3.
				if (is_mt6639(bdev->chip_id)) {
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18048A14, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x184B0010, TRUE, &version);
					if (((val & BIT(26)) != BIT(26)) && (version == 0x03010001)) {
						BTMTK_INFO("%s: AP2WF readable", __func__);
						check = TRUE;
					}
				} else if (is_mt7925(bdev->chip_id)) {
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1804F304, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x184B0010, TRUE, &version);
					if (((val & BIT(26)) != BIT(26)) && (version == 0x03030002)) {
						BTMTK_INFO("%s: AP2WF readable", __func__);
						check = TRUE;
					}
				}

				if (check || mode) {
					check = FALSE;
					BTMTK_INFO("%s: 3.WF_TOP_debug - sheet(dump_list) - C", __func__);
					r_address = 0x184C1B00;
					r_index = 1;
					btmtk_r_debug_cr(bdev, "PSOP_3_1_C", r_index, r_address, TRUE, &val);
					for (i = 0; i < 0x10; i++) {
						r_address = r_address + 0x4;
						r_index++;
						btmtk_r_debug_cr(bdev, "PSOP_3_1_C", r_index, r_address, TRUE, &val);
					}

					r_address = 0x184C1B50;
					r_index = 18;
					btmtk_r_debug_cr(bdev, "PSOP_3_1_C", r_index, r_address, TRUE, &val);
					for (i = 0; i < 0x10; i++) {
						r_address = r_address + 0x4;
						r_index++;
						btmtk_r_debug_cr(bdev, "PSOP_3_1_C", r_index, r_address, TRUE, &val);
					}

					BTMTK_INFO("%s: 4.WF_BUS_debug - sheet(dump_list) - B & C", __func__);
					if (is_mt6639(bdev->chip_id)) {
						btmtk_w_debug_cr(bdev, 0x18400120, 0x810F0000, TRUE);
						r_address = 0x18500408;
						r_index = 2;
						btmtk_r_debug_cr(bdev, "PSOP_4_1_B", r_index, r_address, TRUE, &val);
						for (i = 0; i < 0xD; i++) {
							r_address = r_address + 0x4;
							r_index++;
							btmtk_r_debug_cr(bdev, "PSOP_4_1_B", r_index, r_address, TRUE, &val);
						}

						btmtk_r_debug_cr(bdev, "PSOP_4_1_B", 16, 0x18500000, TRUE, &val);
					} else if (is_mt7925(bdev->chip_id)) {
						btmtk_r_debug_cr(bdev, "PSOP_4_1_B", 0, 0x1802362C, TRUE, &val);
						btmtk_w_debug_cr(bdev, 0x18023604, 0x4, TRUE);
						btmtk_w_debug_cr(bdev, 0x18400120, 0x810F0000, TRUE);
						r_address = 0x18500408;
						r_index = 2;
						btmtk_r_debug_cr(bdev, "PSOP_4_1_B", r_index, r_address, TRUE, &val);
						for (i = 0; i < 0xC; i++) {
							r_address = r_address + 0x4;
							r_index++;
							btmtk_r_debug_cr(bdev, "PSOP_4_1_B", r_index, r_address, TRUE, &val);
						}

						btmtk_r_debug_cr(bdev, "PSOP_4_1_B", 15, 0x18500000, TRUE, &val);
					}

					btmtk_w_debug_cr(bdev, 0x18400120, 0x830C0000, TRUE);
					btmtk_r_debug_cr(bdev, "PSOP_4_1_C", 1, 0x18501004, TRUE, &val);
					btmtk_r_debug_cr(bdev, "PSOP_4_1_C", 2, 0x18501010, TRUE, &val);
					btmtk_r_debug_cr(bdev, "PSOP_4_1_C", 3, 0x18501008, TRUE, &val);
					btmtk_r_debug_cr(bdev, "PSOP_4_1_C", 4, 0x1850100C, TRUE, &val);
					btmtk_r_debug_cr(bdev, "PSOP_4_1_C", 5, 0x18501000, TRUE, &val);
				}
			} else {
				BTMTK_INFO("%s: Don't go to WFSYS", __func__);
			}

			if (is_mt6639(bdev->chip_id)) {
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023A0C, TRUE, &val);
				if ((val & BIT(24)) != BIT(24)) {
					BTMTK_INFO("%s: Go to BGSYS", __func__);
					check = TRUE;
				}
			} else if (is_mt7925(bdev->chip_id)) {
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18001108, TRUE, &val);
				val = val & ~(BIT(16) | BIT(17));
				btmtk_w_debug_cr(bdev, 0x18001108, val, TRUE);
				btmtk_w_debug_cr(bdev, 0x18023A0C, 0xC0041F00, TRUE);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023A10, TRUE, &val);
				if ((val & BIT(24)) != BIT(24)) {
					BTMTK_INFO("%s: Go to BGSYS", __func__);
					check = TRUE;
				}
			}

			if (check || mode) {
				check = FALSE;
				if (is_mt6639(bdev->chip_id)) {
					BTMTK_INFO("%s: 5.BGFSYS status - sheet(bg_top_host_csr_dbg)", __func__);
					btmtk_show_dump_item(bdev, "PSOP_5_1_A", 1, 0x18060C04, 0x18060C00, 0x00030000, 0x10, 0x3, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_B", 1, 0x18023A1C, 0x18023A20, 0x00300000, 0x1, 0xB, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_C", 1, 0x18023A1C, 0x18023A20, 0x00300040, 0x1, 0x19, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_D", 1, 0x18023A1C, 0x18023A20, 0x00300080, 0x1, 0x4, TRUE);
					btmtk_wr_debug_cr(bdev, "PSOP_5_1_E", 1, 0x18023A1C, 0x00300100, 0x18023A20, TRUE, &val);
					btmtk_show_dump_item(bdev, "PSOP_5_1_F", 1, 0x18023A0C, 0x18023A10, 0xC0000000, 0x1, 0x4, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_G", 1, 0x18023A0C, 0x18023A10, 0xC0000100, 0x1, 0x7, TRUE);
				} else if (is_mt7925(bdev->chip_id)) {
					BTMTK_INFO("%s: 5.BGFSYS status - sheet(vcore_off)", __func__);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18001108, TRUE, &val);
					val = val & ~(0x00030000);
					btmtk_w_debug_cr(bdev, 0x18001108, val, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_B", 1, 0x18023A0C, 0x18023A10, 0xC0000000, 0x1, 0x6, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_C", 1, 0x18023A0C, 0x18023A10, 0xC0000100, 0x1, 0x6, TRUE);
					btmtk_wr_debug_cr(bdev, "PSOP_5_1_D", 1, 0x18023A0C, 0xC0000200, 0x18023A10, TRUE, &val);

					BTMTK_INFO("%s: 5.BGFSYS status - sheet(vcore_on)", __func__);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18001108, TRUE, &val);
					val = val & ~(0x000C0000);
					btmtk_w_debug_cr(bdev, 0x18001108, val, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_E", 1, 0x18023A1C, 0x18023A20, 0x00300040, 0x1, 0x17, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_F", 1, 0x18023A1C, 0x18023A20, 0x00300001, 0x1, 0xC, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_G", 1, 0x18023A1C, 0x18023A20, 0x00300080, 0x1, 0x5, TRUE);
					btmtk_wr_debug_cr(bdev, "PSOP_5_1_G", 6, 0x18023A1C, 0x00300100, 0x18023A20, TRUE, &val);
					btmtk_wr_debug_cr(bdev, "PSOP_5_1_G", 7, 0x18023A1C, 0x00300140, 0x18023A20, TRUE, &val);

					BTMTK_INFO("%s: 5.BGFSYS status - sheet(vlp)", __func__);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023A24, TRUE, &val);
					val = val & ~(0x00000007);
					btmtk_w_debug_cr(bdev, 0x18023A24, val, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_H", 1, 0x18060C04, 0x18060C00, 0x00030000, 0x10, 0x3, TRUE);
					btmtk_wr_debug_cr(bdev, "PSOP_5_1_H", 4, 0x18060C04, 0x00030050, 0x18060C00, TRUE, &val);
				}

				BTMTK_INFO("%s: 6.BGFSYS BUS debug method - sheet(dump cr via host_cr)", __func__);
				if (is_mt7925(bdev->chip_id)) {
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18001108, TRUE, &val);
					val = val & 0b11111111111111001111111111111111;
					btmtk_w_debug_cr(bdev, 0x18001108, val, TRUE);
				}

				btmtk_show_dump_item(bdev, "PSOP_6_2_A", 1, 0x18023A0C, 0x18023A10, 0xC0041F00, 0x1, 0x11, TRUE);
				if (is_mt7925(bdev->chip_id)) {
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18001108, TRUE, &val);
					val = val & 0b11111111111111001111111111111111;
					btmtk_w_debug_cr(bdev, 0x18001108, val, TRUE);
				}

				btmtk_show_dump_item(bdev, "PSOP_6_2_B", 1, 0x18023A0C, 0x18023A10, 0xC0041910, 0x10, 0x9, TRUE);
				if (is_mt7925(bdev->chip_id)) {
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18001108, TRUE, &val);
					val = val & 0b11111111111111001111111111111111;
					btmtk_w_debug_cr(bdev, 0x18001108, val, TRUE);
				}

				btmtk_show_dump_item(bdev, "PSOP_6_2_C", 1, 0x18023A0C, 0x18023A10, 0xC0041B00, 0x1, 0x13, TRUE);

				// [AP2BT] step 2. & 3.
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011454, TRUE, &val);
				btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18812000, TRUE, &version);
				if (is_mt6639(bdev->chip_id)) {
					if (((val & BIT(22)) != BIT(22)) && ((val & BIT(23)) != BIT(23)) &&
							version == 0x03000000) {
						BTMTK_INFO("%s: AP2BT readable", __func__);
						check = TRUE;
					}
				} else if (is_mt7925(bdev->chip_id)) {
					if (((val & BIT(22)) != BIT(22)) && ((val & BIT(23)) != BIT(23)) &&
							version == 0x03020000) {
						BTMTK_INFO("%s: AP2BT readable", __func__);
						check = TRUE;
					}
				}

				if (check || mode) {
					check = FALSE;
					BTMTK_INFO("%s: 5.BGFSYS status - sheet(bg_top_apb_cr_dbg)", __func__);
					btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 1, 0x18860000, TRUE, &val);
					btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 2, 0x18860020, TRUE, &val);
					if (is_mt6639(bdev->chip_id)) {
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 3, 0x18860024, TRUE, &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 4, 0x18860030, TRUE, &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 5, 0x18860034, TRUE, &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 6, 0x18860128, TRUE, &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 7, 0x18821120, TRUE, &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 8, 0x18821148, TRUE, &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 9, 0x18821200, TRUE, &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 10, 0x18821210, TRUE, &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 11, 0x18821504, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 12, 0x18821508, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 13, 0x1882150C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 14, 0x18821510, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 15, 0x18821640, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 16, 0x18821644, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 17, 0x18821648, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 18, 0x1882164C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 19, 0x18821700, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 20, 0x18820004, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 21, 0x18820010, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 22, 0x1882006C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 23, 0x18820088, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 24, 0x18820090, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 25, 0x18812100, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 26, 0x18812104, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 27, 0x18812108, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 28, 0x18812114, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 29, 0x18812118, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 30, 0x18812120, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 31, 0x18812124, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 32, 0x18812150, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 33, 0x18812168, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 34, 0x1881216C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 35, 0x18812170, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 36, 0x18812174, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 37, 0x18812178, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 38, 0x1881217C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 39, 0x18812400, TRUE , &val);
					} else if (is_mt7925(bdev->chip_id)) {
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 3, 0x18860028, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 4, 0x18860030, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 5, 0x18860034, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 6, 0x18860128, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 7, 0x1886012C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 8, 0x18821120, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 9, 0x18821148, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 10, 0x18821200, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 11, 0x18821210, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 12, 0x18821504, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 13, 0x18821508, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 14, 0x1882150C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 15, 0x18821510, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 16, 0x18821640, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 17, 0x18821644, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 18, 0x18821648, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 19, 0x1882164C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 20, 0x18821700, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 21, 0x18820004, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 22, 0x18820010, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 23, 0x1882006C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 24, 0x18820088, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 25, 0x18820090, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 26, 0x18812100, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 27, 0x18812104, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 28, 0x18812108, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 29, 0x18812114, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 30, 0x18812118, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 31, 0x18812120, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 32, 0x18812124, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 33, 0x18812150, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 34, 0x18812168, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 35, 0x1881216C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 36, 0x18812170, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 37, 0x18812174, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 38, 0x18812178, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 39, 0x1881217C, TRUE , &val);
						btmtk_r_debug_cr(bdev, "PSOP_5_2_A", 40, 0x18812400, TRUE , &val);
					}
				} else
					BTMTK_INFO("%s: Don't go to BGSYS", __func__);
			} else
				BTMTK_INFO("%s: CONN_INFRA top owner", __func__);
		}
	}

	BTMTK_INFO("%s: END", __func__);
}

void btmtk_usb_low_power_dump(struct btmtk_dev *bdev, u8 mode)
{
	u32 val, wfsys_status, btsys_status;
	int status;

	BTMTK_INFO("%s: mode: %02X", __func__, mode);

	if (is_connac3(bdev->chip_id)) {
		BTMTK_INFO("%s: 1.Power check - sheet1", __func__);
		status = btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18060A00, TRUE, &val);
		if (status) {
			BTMTK_INFO("%s: Avoid jamming, so end it directly.", __func__);
			return;
		}

		BTMTK_INFO("%s: 1.Power check - sheet3", __func__);
		btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18060200, TRUE, &wfsys_status);
		if ((wfsys_status & BIT(31)) || mode) {
			BTMTK_INFO("%s: 3.Power check - sheet(dump_list) - A & B", __func__);
			btmtk_w_debug_cr(bdev, 0x18060B00, 0x1, TRUE);
			btmtk_show_dump_item(bdev, "PSOP_3_1_A", 1, 0x18060B04, 0x18060B10, 0x0, 0x1, 0x5, TRUE);
			btmtk_wr_debug_cr(bdev, "PSOP_3_1_A", 6, 0x18060B04, 0x10, 0x18060B10, TRUE, &val);
			btmtk_wr_debug_cr(bdev, "PSOP_3_1_A", 7, 0x18060B04, 0x12, 0x18060B10, TRUE, &val);
			btmtk_show_dump_item(bdev, "PSOP_3_1_A", 8, 0x18060B04, 0x18060B10, 0x17, 0x1, 0x5, TRUE);
			btmtk_wr_debug_cr(bdev, "PSOP_3_1_A", 13, 0x18060B04, 0x1D, 0x18060B10, TRUE, &val);
			btmtk_w_debug_cr(bdev, 0x18060B18, 0x1, TRUE);
			btmtk_show_dump_item(bdev, "PSOP_3_1_B", 1, 0x18060B1C, 0x18023638, 0x0, 0x1, 0x5, TRUE);
			btmtk_wr_debug_cr(bdev, "PSOP_3_1_B", 6, 0x18060B1C, 0x8, 0x18023638, TRUE, &val);
		}

		if (!(wfsys_status & BIT(31)) || mode) {
			BTMTK_INFO("%s: 1.Power check - sheet2", __func__);
			btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18060230, TRUE, &btsys_status);

			if ((btsys_status & BIT(3)) || mode) {
				if (is_mt6639(bdev->chip_id)) {
					BTMTK_INFO("%s: 2.BGFSYS status - sheet(bg_top_host_csr_dbg)", __func__);
					btmtk_show_dump_item(bdev, "SKIP", 0, 0x18060C04, 0x18060C00, 0x30000, 0x10, 0x3, TRUE);
					btmtk_show_dump_item(bdev, "SKIP", 0, 0x18023A1C, 0x18023A20, 0x300000, 0x1, 0xB, TRUE);
					btmtk_show_dump_item(bdev, "SKIP", 0, 0x18023A1C, 0x18023A20, 0x300040, 0x1, 0x19, TRUE);
					btmtk_show_dump_item(bdev, "SKIP", 0, 0x18023A1C, 0x18023A20, 0x300080, 0x1, 0x04, TRUE);
					btmtk_wr_debug_cr(bdev, "SKIP", 0, 0x18023A1C, 0x00300100, 0x18023A20, TRUE, &val);
					btmtk_show_dump_item(bdev, "SKIP", 0, 0x18023A0C, 0x18023A10, 0xc0000000, 0x1, 0x04, TRUE);
					btmtk_show_dump_item(bdev, "SKIP", 0, 0x18023A0C, 0x18023A10, 0xc0000100, 0x1, 0x07, TRUE);
				} else if (is_mt7925(bdev->chip_id)) {
					BTMTK_INFO("%s: 2.BGFSYS status - sheet(vcore_off)", __func__);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18001108, TRUE, &val);
					val = val & ~(0x00030000);
					btmtk_w_debug_cr(bdev, 0x18001108, val, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_B", 1, 0x18023A0C, 0x18023A10, 0xC0000000, 0x1, 0x6, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_C", 1, 0x18023A0C, 0x18023A10, 0xC0000100, 0x1, 0x6, TRUE);
					btmtk_wr_debug_cr(bdev, "PSOP_5_1_D", 1, 0x18023A0C, 0xC0000200, 0x18023A10, TRUE, &val);

					BTMTK_INFO("%s: 2.BGFSYS status - sheet(vcore_on)", __func__);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18001108, TRUE, &val);
					val = val & ~(0x000C0000);
					btmtk_w_debug_cr(bdev, 0x18001108, val, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_E", 1, 0x18023A1C, 0x18023A20, 0x00300040, 0x1, 0x17, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_F", 1, 0x18023A1C, 0x18023A20, 0x00300001, 0x1, 0xC, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_G", 1, 0x18023A1C, 0x18023A20, 0x00300080, 0x1, 0x5, TRUE);
					btmtk_wr_debug_cr(bdev, "PSOP_5_1_G", 6, 0x18023A1C, 0x00300100, 0x18023A20, TRUE, &val);
					btmtk_wr_debug_cr(bdev, "PSOP_5_1_G", 7, 0x18023A1C, 0x00300140, 0x18023A20, TRUE, &val);

					BTMTK_INFO("%s: 2.BGFSYS status - sheet(vlp)", __func__);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18023A24, TRUE, &val);
					val = val & ~(0x00000007);
					btmtk_w_debug_cr(bdev, 0x18023A24, val, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_5_1_H", 1, 0x18060C04, 0x18060C00, 0x00030000, 0x10, 0x3, TRUE);
					btmtk_wr_debug_cr(bdev, "PSOP_5_1_H", 4, 0x18060C04, 0x00030050, 0x18060C00, TRUE, &val);
				}
			}

			if (((btsys_status & BIT(3)) != BIT(3)) || mode) {
				BTMTK_INFO("%s: 4.CONN INFRA status - sheet(dump_list)", __func__);
				if (is_mt6639(bdev->chip_id)) {
					btmtk_r_debug_cr(bdev, "PSOP_7_1_A", 1, 0x18060A00, TRUE, &val);
					btmtk_r_debug_cr(bdev, "PSOP_7_1_A", 2, 0x18060A0C, TRUE, &val);
					btmtk_show_dump_item(bdev, "PSOP_7_1_A", 3, 0x1806015C, 0x18060A04, 0x0, 0x1, 0x8, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_7_1_A", 11, 0x18060160, 0x18060A08, 0x0, 0x1, 0x6, TRUE);
					btmtk_r_debug_cr(bdev, "PSOP_7_1_B", 1, 0x18023200, TRUE, &val);
					btmtk_r_debug_cr(bdev, "PSOP_7_1_B", 2, 0x18011130, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
					val = val & ~(BIT(21) | BIT(22));
					btmtk_wr_debug_cr(bdev, "PSOP_7_1_B", 3, 0x18011100, val, 0x18011134, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
					val = (val & ~BIT(22)) | BIT(21);
					btmtk_wr_debug_cr(bdev, "PSOP_7_1_B", 4, 0x18011100, val, 0x18011134, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
					val = (val & ~BIT(21)) | BIT(22);
					btmtk_wr_debug_cr(bdev, "PSOP_7_1_B", 5, 0x18011100, val, 0x18011134, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
					val = val | BIT(21) | BIT(22);
					btmtk_wr_debug_cr(bdev, "PSOP_7_1_B", 6, 0x18011100, val, 0x18011134, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE, &val);
					val = (val & 0b11111111111111111111111111000000) | 0b1110;
					btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE, &val);
					val = (val & 0b11111111111111111111000000111111) | 0b10001000000;
					btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
					btmtk_w_debug_cr(bdev, 0x1801601C, 0x03020100, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016020, 0x07060504, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016024, 0x0B0A0908, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016028, 0x0F0E0D0C, TRUE);
					btmtk_w_debug_cr(bdev, 0x1801602C, 0x13121110, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016030, 0x17161514, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016034, 0x1B1A1918, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016038, 0x1F1E1D1C, TRUE);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18016058, TRUE, &val);
					val = val | BIT(1);
					btmtk_w_debug_cr(bdev, 0x18016058, val, TRUE);
					btmtk_r_debug_cr(bdev, "PSOP_7_1_B", 7, 0x18023200, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE, &val);
					val = (val & 0b11111111111111111111111111000000) | 0b11000;
					btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
					btmtk_w_debug_cr(bdev, 0x1801601C, 0x03020100, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016020, 0x07060504, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016024, 0x0B0A0908, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016028, 0x0F0E0D0C, TRUE);
					btmtk_w_debug_cr(bdev, 0x1801602C, 0x13121110, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016030, 0x17161514, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016034, 0x1B1A1918, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016038, 0x1F1E1D1C, TRUE);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18016058, TRUE, &val);
					val = val | BIT(1);
					btmtk_w_debug_cr(bdev, 0x18016058, val, TRUE);
					btmtk_r_debug_cr(bdev, "PSOP_7_1_B", 8, 0x18023200, TRUE, &val);
				} else if (is_mt7925(bdev->chip_id)) {
					btmtk_r_debug_cr(bdev, "PSOP_8_1_A", 1, 0x18060A00, TRUE, &val);
					btmtk_r_debug_cr(bdev, "PSOP_8_1_A", 2, 0x18060A0C, TRUE, &val);
					btmtk_show_dump_item(bdev, "PSOP_8_1_A", 3, 0x1806015C, 0x18060A04, 0x0, 0x1, 0x8, TRUE);
					btmtk_show_dump_item(bdev, "PSOP_8_1_A", 11, 0x18060160, 0x18060A08, 0x0, 0x1, 0x6, TRUE);
					btmtk_wr_debug_cr(bdev, "PSOP_8_1_A", 17, 0x18060160, 0x7, 0x18060A08, TRUE, &val);
					btmtk_wr_debug_cr(bdev, "PSOP_8_1_A", 18, 0x18060160, 0x8, 0x18060A08, TRUE, &val);
					btmtk_r_debug_cr(bdev, "PSOP_8_1_B", 1, 0x18023200, TRUE, &val);
					btmtk_r_debug_cr(bdev, "PSOP_8_1_B", 2, 0x18011130, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
					val = val & ~(BIT(21) | BIT(22));
					btmtk_wr_debug_cr(bdev, "PSOP_8_1_B", 3, 0x18011100, val, 0x18011134, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
					val = (val & ~BIT(22)) | BIT(21);
					btmtk_wr_debug_cr(bdev, "PSOP_8_1_B", 4, 0x18011100, val, 0x18011134, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
					val = (val & ~BIT(21)) | BIT(22);
					btmtk_wr_debug_cr(bdev, "PSOP_8_1_B", 5, 0x18011100, val, 0x18011134, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18011100, TRUE, &val);
					val = val | BIT(21) | BIT(22);
					btmtk_wr_debug_cr(bdev, "PSOP_8_1_B", 6, 0x18011100, val, 0x18011134, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE, &val);
					val = (val & 0b11111111111111111111111111000000) | 0b1110;
					btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE, &val);
					val = (val & 0b11111111111111111111000000111111) | 0b10001000000;
					btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
					btmtk_w_debug_cr(bdev, 0x1801601C, 0x03020100, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016020, 0x07060504, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016024, 0x0B0A0908, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016028, 0x0F0E0D0C, TRUE);
					btmtk_w_debug_cr(bdev, 0x1801602C, 0x13121110, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016030, 0x17161514, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016034, 0x1B1A1918, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016038, 0x1F1E1D1C, TRUE);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18016058, TRUE, &val);
					val = val | BIT(1);
					btmtk_w_debug_cr(bdev, 0x18016058, val, TRUE);
					btmtk_r_debug_cr(bdev, "PSOP_8_1_B", 7, 0x18023200, TRUE, &val);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x1801603C, TRUE, &val);
					val = (val & 0b11111111111111111111111111000000) | 0b11000;
					btmtk_w_debug_cr(bdev, 0x1801603C, val, TRUE);
					btmtk_w_debug_cr(bdev, 0x1801601C, 0x03020100, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016020, 0x07060504, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016024, 0x0B0A0908, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016028, 0x0F0E0D0C, TRUE);
					btmtk_w_debug_cr(bdev, 0x1801602C, 0x13121110, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016030, 0x17161514, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016034, 0x1B1A1918, TRUE);
					btmtk_w_debug_cr(bdev, 0x18016038, 0x1F1E1D1C, TRUE);
					btmtk_r_debug_cr(bdev, "SKIP", 0, 0x18016058, TRUE, &val);
					val = val | BIT(1);
					btmtk_w_debug_cr(bdev, 0x18016058, val, TRUE);
					btmtk_r_debug_cr(bdev, "PSOP_8_1_B", 8, 0x18023200, TRUE, &val);
				}
			}
		}
	}

	BTMTK_INFO("%s: END", __func__);
}

void btmtk_usb_trx_info_dump(struct btmtk_dev *bdev)
{
	u32 val;

	BTMTK_INFO("%s: Debug info is dumping. Chip ID: %04x", __func__, bdev->chip_id);

	btmtk_r_debug_cr(bdev, "SKIP", 0, TX3CSR2_TXFIFOADDR, TRUE, &val);
	btmtk_r_debug_cr(bdev, "SKIP", 0, RX3CSR2_RXFIFOADDR, TRUE, &val);
}

void btmtk_usb_enable_DMA_debug(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s: Debug info is dumping. Chip ID: %04X", __func__, bdev->chip_id);

    if (is_connac3(bdev->chip_id)) {
        btmtk_w_debug_cr(bdev, 0x740003BC, 0x00000001, TRUE);
        btmtk_w_debug_cr(bdev, 0x74000018, 0x00000001, TRUE);
    } else {
         BTMTK_INFO("%s: Unsupported Chip ID: %04X", __func__, bdev->chip_id);
    }
}

void btmtk_usb_cbinfra_dump(struct btmtk_dev *bdev)
{
	u32 val = 0;
	u32 w_address, r_address = 0;
	u32 r_index;
	u32 CbInfraVlpReadOnly[] = {
			0x70025000, 0x70025004, 0x70025008,
			0x7002500C, 0x70025010, 0x70025014,
			0x70025018, 0x7002501C, 0x70025020,
			0x70025024, 0x70025028, 0x7002502C,
			0x70025030, 0x70025034, 0x70025038,
			0x7002503C, 0x70025040, 0x70025044,
			0x70025048, 0x7002504C, 0x70025050,
			0x70025054, 0x70025058, 0x7002505C,
			0x70025060, 0x70025064, 0x70025068,
			0x7002506C, 0x70025070, 0x70025074,
			0x70025078, 0x7002507C, 0x70025080,
			0x70025084, 0x700250F0, 0x70025100,
			0x70025104, 0x70025108, 0x7002510C,
			0x70025110, 0x70025114, 0x70025118,
			0x7002511C, 0x70025120, 0x70025130,
			0x70025140, 0x70025180, 0x70025210,
			0x70025400, 0x70025404, 0x70025408,
			0x7002540C, 0x70025410, 0x70025414};
	u32 CbInfraVlpWriteRead[] = {
			0x00010001, 0x00010203, 0x00010405,
			0x00010607, 0x00010809, 0x00010A0B,
			0x00010C0D, 0x00010E0F, 0x00011011,
			0x00011213, 0x00011415, 0x00011617,
			0x00011819};
	u32 CbInfraVcoreOn[] = {
			0x7002802C, 0x700283C0, 0x70028424,
			0x70028428, 0x7002842C, 0x70028430,
			0x70028434, 0x70028438, 0x7002843C,
			0x70028520, 0x70028524, 0x70028528,
			0x7002852C, 0x70028730, 0x70028734,
			0x70028738, 0x7002873C, 0x70028900};
	u32 CbInfraVcoreOff[] = {
			0x70026000, 0x70026004, 0x70026008,
			0x7002600C, 0x70026100, 0x70026104,
			0x70026550, 0x70026554, 0x70026558,
			0x70026A50, 0x70026A58, 0x70026A5C,
			0x70026A60, 0x70026A64, 0x70026A68,
			0x70026A6C, 0x70020004, 0x70020008,
			0x70020080, 0x70020110, 0x70021060,
			0x70021064, 0x70021068, 0x7002106C,
			0x70021070, 0x70021074, 0x70021078,
			0x7002107C, 0x70021080, 0x70021084,
			0x70021088, 0x7002108C, 0x70021090,
			0x70021094, 0x70021098, 0x7002109C,
			0x700210A0};
	u32 Pcie[] = {
			0x74030150, 0x74030154, 0x74030184,
			0x74031010};
	u32 Cbtop[] = {
			0x70000104, 0x70000248, 0x7000024C,
			0x70007400, 0x70007404, 0x70003000,
			0x74031204, 0x74031210, 0x74030184,
			0x740331C0};

	if (is_mt7925(bdev->chip_id)) {
		BTMTK_INFO("%s: ==========Start dump extra CB top info==========", __func__);

		BTMTK_INFO("%s: ==========Section E==========", __func__);
		btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026558, TRUE, &val);
		val = (val & 0xFFFF0000) | 0x7000;
		btmtk_w_debug_cr(bdev, 0x18016038, 0x1F1E1D1C, TRUE);
		btmtk_w_debug_cr(bdev, 0x70026558, val, TRUE);
		for (r_index = 0; r_index < sizeof(Cbtop) / sizeof(u32); r_index++) {
			r_address = Cbtop[r_index];
			btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, TRUE, &val);
		}

		BTMTK_INFO("%s: ==========Section A==========", __func__);
		for (r_index = 0; r_index < sizeof(CbInfraVlpReadOnly) / sizeof(u32); r_index++) {
			r_address = CbInfraVlpReadOnly[r_index];
			btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, TRUE, &val);
		}

		for (r_index = 0; r_index < sizeof(CbInfraVlpWriteRead) / sizeof(u32); r_index++) {
			w_address = 0x70025300;
			r_address = 0x70025304;
			btmtk_w_debug_cr(bdev, w_address, CbInfraVlpWriteRead[r_index], TRUE);
			btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, TRUE, &val);
		}

		BTMTK_INFO("%s: ==========Section B==========", __func__);
		for (r_index = 0; r_index < sizeof(CbInfraVcoreOn) / sizeof(u32); r_index++) {
			r_address = CbInfraVcoreOn[r_index];
			btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, TRUE, &val);
		}

		BTMTK_INFO("%s: ==========Section C==========", __func__);
		for (r_index = 0; r_index < sizeof(CbInfraVcoreOff) / sizeof(u32); r_index++) {
			r_address = CbInfraVcoreOff[r_index];
			btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, TRUE, &val);
		}

		BTMTK_INFO("%s: ==========Section D==========", __func__);
		btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70026554, TRUE, &val);
		val = (val & 0x0000FFFF) | 0x74030000;
		btmtk_w_debug_cr(bdev, 0x70026554, val, TRUE);
		for (r_index = 0; r_index < sizeof(Pcie) / sizeof(u32); r_index++) {
			r_address = Pcie[r_index];
			btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, TRUE, &val);
		}

		btmtk_w_debug_cr(bdev, 0x74030168, 0x22CC0100, TRUE);
		btmtk_w_debug_cr(bdev, 0x74030164, 0x81804845, TRUE);
		btmtk_r_debug_cr(bdev, "SKIP", 0, 0x7403002C, TRUE, &val);

		BTMTK_INFO("%s: ==========Finish dump extra CB top info==========", __func__);
	}
}

void btmtk_usb_info_dump(struct btmtk_dev *bdev)
{
	u32 val;
	int status, i;
	u32 r_address;

	BTMTK_INFO("%s: Debug info is dumping. Chip ID: %04X", __func__, bdev->chip_id);

	if (is_connac3(bdev->chip_id)) {
		status = btmtk_r_debug_cr(bdev, "SKIP", 0, POWER_MANAGEMENT, TRUE, &val);
		if (status) {
			BTMTK_INFO("%s: Avoid jamming, so end it directly.", __func__);
			return;
		}

		btmtk_r_debug_cr(bdev, "SKIP", 0, USB20_OPSTATE_SYS, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, SSUSB_IP_DEV_PDN, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, SSUSB_U2_PORT_PDN, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, MISC_CTRL, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, SSUSB_IP_SLEEP_STS, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, EPISR, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, EPIER, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, EPISR_MD, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, EPIER_MD, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, EPISR_UHW, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, EPIER_UHW, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, EP0CSR, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, EP0DISPATCH, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, SSUSB_IP_SPARE0, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, SSUSB_IP_SPARE1, TRUE, &val);
	}

	if (is_mt7925(bdev->chip_id)) {
		btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025050, TRUE, &val);
		btmtk_r_debug_cr(bdev, "SKIP", 0, 0x70025054, TRUE, &val);
		BTMTK_INFO("%s: Data patch.", __func__);
		r_address = 0x7C059410;
		status = btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, FALSE, &val);
		if (status) {
			BTMTK_INFO("%s: Avoid jamming, so end it directly.", __func__);
			return;
		}

		for (i = 0; i < 0x2F; i++) {
			r_address = r_address + 0x4;
			btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, FALSE, &val);
		}

		BTMTK_INFO("%s: Ctrl patch.", __func__);
		r_address = 0x7C0594D0;
		btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, FALSE, &val);
		for (i = 0; i < 0x2F; i++) {
			r_address = r_address + 0x4;
			btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, FALSE, &val);
		}
	} else if (is_mt6639(bdev->chip_id)) {
		BTMTK_INFO("%s: Data patch.", __func__);
		r_address = 0x7C05B160;
		status = btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, FALSE, &val);
		if (status) {
			BTMTK_INFO("%s: Avoid jamming, so end it directly.", __func__);
			return;
		}

		for (i = 0; i < 0x2F; i++) {
			r_address = r_address + 0x4;
			btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, FALSE, &val);
		}

		BTMTK_INFO("%s: Ctrl patch.", __func__);
		r_address = 0x7C05B220;
		btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, FALSE, &val);
		for (i = 0; i < 0x2F; i++) {
			r_address = r_address + 0x4;
			btmtk_r_debug_cr(bdev, "SKIP", 0, r_address, FALSE, &val);
		}
	}
}

void btmtk_usb_get_dump_mode(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s: mode = %d", __func__, bdev->bt_cfg.debug_sop_mode);
	bdev->dump_cnt++;
	BTMTK_INFO("%s: =PSOP_START=%d, %04x, BT_DRV, 1",
				__func__, bdev->dump_cnt, bdev->chip_id);

	if (is_connac3(bdev->chip_id) && bdev->bt_cfg.debug_sop_mode != BTMTK_DBG_DEFAULT_STATE) {
		btmtk_usb_cbinfra_dump(bdev);
		btmtk_usb_info_dump(bdev);
		btmtk_usb_pc_lr_dump(bdev);
	}

	switch (bdev->bt_cfg.debug_sop_mode) {
	case BTMTK_DBG_DEFAULT_STATE:
		BTMTK_INFO("%s: dump mode is 0, don't dump CR", __func__);
		break;
	case BTMTK_CHECK_DBG_STATUS:
		btmtk_usb_check_DBG_status_dump(bdev);
		break;
	case BTMTK_PDBG_SLAVE_NO_RESPONSE_CONDITIONAL:
		btmtk_usb_check_DBG_status_dump(bdev);
		// mode 0: conditional dump, mode 1: full dump
		btmtk_usb_slave_no_response_dump(bdev, 0);
		break;
	case BTMTK_PDBG_SLAVE_NO_RESPONSE_ALL:
		btmtk_usb_check_DBG_status_dump(bdev);
		// mode 0: conditional dump, mode 1: full dump
		BTMTK_INFO("%s: trigger all btmtk_slave_no_response_dump.", __func__);
		btmtk_usb_slave_no_response_dump(bdev, 1);
		break;
	case BTMTK_LOW_POWER_CONDITIONAL:
		// mode 0: conditional dump, mode 1: full dump
		btmtk_usb_low_power_dump(bdev, 0);
		break;
	case BTMTK_LOW_POWER_ALL:
		// mode 0: conditional dump, mode 1: full dump
		BTMTK_INFO("%s: trigger all btmtk_low_power_dump.", __func__);
		btmtk_usb_low_power_dump(bdev, 1);
		break;
	case BTMTK_DBG_INFO:
		btmtk_usb_info_dump(bdev);
		btmtk_usb_trx_info_dump(bdev);
		break;
	default:
		BTMTK_INFO("%s: trigger all relevant CR. dump_mode: %d", __func__, bdev->bt_cfg.debug_sop_mode);
		btmtk_usb_pc_lr_dump(bdev);
		btmtk_usb_check_DBG_status_dump(bdev);
		btmtk_usb_slave_no_response_dump(bdev, 1);
		btmtk_usb_low_power_dump(bdev, 1);
		btmtk_usb_info_dump(bdev);
		btmtk_usb_trx_info_dump(bdev);
		break;
	}
}

static void btmtk_usb_dump_debug_sop(struct btmtk_dev *bdev)
{
	/* dump mcu_sleep_wakeup_debug(BGFSYS_status),
	 * only for PCIE, USB/SDIO not support
	 */
	if (bdev == NULL) {
		BTMTK_ERR("%s bdev is NULL", __func__);
		return;
	}

	if (btmtk_get_chip_state(bdev) == BTMTK_STATE_DISCONNECT) {
		BTMTK_ERR("%s chip state is disconnect, don't dump debug SOP", __func__);
		return;
	}

	BTMTK_INFO("%s -debug sop dump start", __func__);
	if (is_connac3(bdev->chip_id))
		btmtk_usb_get_dump_mode(bdev);
	else
		btmtk_usb_dump_debug_register(bdev, bdev->debug_sop_reg_dump);

	BTMTK_INFO("%s -debug sop dump end", __func__);
}

static void btmtk_usb_cif_mutex_lock(struct btmtk_dev *bdev)
{
	USB_OPS_MUTEX_LOCK();
}

static void btmtk_usb_cif_mutex_unlock(struct btmtk_dev *bdev)
{
	USB_OPS_MUTEX_UNLOCK();
}

static inline void btusb_free_frags(struct btmtk_dev *bdev)
{
	unsigned long flags;

	spin_lock_irqsave(&bdev->rxlock, flags);

	kfree_skb(bdev->evt_skb);
	bdev->evt_skb = NULL;

	kfree_skb(bdev->sco_skb);
	bdev->sco_skb = NULL;

	spin_unlock_irqrestore(&bdev->rxlock, flags);
}

static int btusb_recv_isoc(struct btmtk_dev *bdev, void *buffer, int count)
{
	struct sk_buff *skb;
	int err = 0;
	unsigned char *skb_tmp = NULL;

	spin_lock(&bdev->rxlock);
	skb = bdev->sco_skb;

	while (count) {
		int len;

		if (!skb) {
			skb = bt_skb_alloc(HCI_MAX_SCO_SIZE, GFP_ATOMIC);
			if (!skb) {
				err = -ENOMEM;
				break;
			}

			hci_skb_pkt_type(skb) = HCI_SCODATA_PKT;
			hci_skb_expect(skb) = HCI_SCO_HDR_SIZE;
		}

		len = min_t(uint, hci_skb_expect(skb), count);
		if (skb_tailroom(skb) >= len) {
			skb_tmp = skb_put(skb, len);
			if (!skb_tmp) {
				BTMTK_ERR("%s, skb_put failed. Len = %d!", __func__, len);
				kfree_skb(skb);
				spin_unlock(&bdev->rxlock);
				return -ENOMEM;
			}
			memcpy(skb_tmp, buffer, len);

			count -= len;
			buffer += len;
			hci_skb_expect(skb) -= len;

			if (skb->len == HCI_SCO_HDR_SIZE) {
				/* Complete SCO header */
				hci_skb_expect(skb) = hci_sco_hdr(skb)->dlen;

				if (skb_tailroom(skb) < hci_skb_expect(skb)) {
					kfree_skb(skb);
					skb = NULL;
					err = -EILSEQ;
					break;
				}
			}
			if (!hci_skb_expect(skb)) {
				/* Complete frame */
				hci_recv_frame(bdev->hdev, skb);
			}
		} else {
			BTMTK_INFO("%s: the size of skb is too small", __func__);
		}
	}

	bdev->sco_skb = skb;
	spin_unlock(&bdev->rxlock);

	return err;
}

static void btusb_intr_complete(struct urb *urb)
{
	struct hci_dev *hdev = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_usb_dev *cif_dev = NULL;
	int err;
	u8 *buf;
	static u8 intr_blocking_usb_warn;

	if (urb == NULL) {
		BTMTK_ERR("%s: ERROR, urb is NULL!", __func__);
		return;
	}

	hdev = urb->context;
	if (hdev == NULL) {
		BTMTK_ERR("%s: ERROR, hdev is NULL!", __func__);
		return;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL) {
		BTMTK_ERR("%s: ERROR, bdev is NULL!", __func__);
		return;
	}

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: ERROR, cif_dev is NULL!", __func__);
		return;
	}

	BTMTK_DBG("urb %p status %d count %d", urb, urb->status,
	       urb->actual_length);

	if (urb->status != 0 && intr_blocking_usb_warn < 10) {
		intr_blocking_usb_warn++;
		BTMTK_WARN("%s: urb %p urb->status %d count %d", __func__,
			urb, urb->status, urb->actual_length);
	} else if (urb->status == 0 && urb->actual_length != 0)
		intr_blocking_usb_warn = 0;

	if (urb->status == 0) {
		buf = urb->transfer_buffer;
		if (buf[0] == 0x3E)
			btmtk_hci_snoop_save(HCI_SNOOP_TYPE_ADV_EVT_HIF,
				buf, urb->actual_length);
		else if (buf[0] == 0x13)
			btmtk_hci_snoop_save(HCI_SNOOP_TYPE_NOCP_EVT_HIF,
				buf, urb->actual_length);
		else
			btmtk_hci_snoop_save(HCI_SNOOP_TYPE_EVT_HIF,
				buf, urb->actual_length);

		hdev->stat.byte_rx += urb->actual_length;

		if (!cif_dev->urb_intr_buf) {
			BTMTK_ERR("%s: bdev->urb_intr_buf is NULL!", __func__);
			return;
		}

		if (urb->actual_length >= URB_MAX_BUFFER_SIZE ||
			(urb->actual_length != get_pkt_len(HCI_EVENT_PKT, buf) && urb->actual_length > 1)) {
			BTMTK_ERR("%s: urb->actual_length is invalid, buf[1] = %d!",
				__func__, buf[1]);
			btmtk_hci_snoop_print(urb->transfer_buffer, urb->actual_length);
			goto intr_resub;
		}
		memset(cif_dev->urb_intr_buf, 0, URB_MAX_BUFFER_SIZE);
		cif_dev->urb_intr_buf[0] = HCI_EVENT_PKT;
		memcpy(cif_dev->urb_intr_buf + 1, urb->transfer_buffer, urb->actual_length);

		BTMTK_DBG("%s ,urb->actual_length = %d", __func__, urb->actual_length);
		BTMTK_DBG_RAW(cif_dev->urb_intr_buf, urb->actual_length + 1, "%s, recv evt", __func__);
		BTMTK_DBG_RAW(urb->transfer_buffer, urb->actual_length, "%s, recv evt", __func__);
		if (cif_dev->urb_intr_buf[1] == 0xFF && urb->actual_length == 1) {
			/* We can't use usb_control_msg in interrupt.
			 * If you use usb_control_msg , it will cause crash.
			 * Receive a bytes 0xFF from controller, it's WDT interrupt to driver.
			 * WDT interrupt is a mechanism to do L0.5 reset.
			 */

			/* USB hif: recv both 0xff and fw dump end, then do chip reset */
			bdev->chip_reset_signal |= (1 << 0);
			BTMTK_INFO("%s ,chip_reset_signal = %02x", __func__, bdev->chip_reset_signal);
			if ((!bdev->dualBT && bdev->chip_reset_signal == 0x03)
				|| (bdev->dualBT && bdev->chip_reset_signal == 0x07)) {
				bdev->chip_reset_signal = 0x00;
				DUMP_TIME_STAMP("notify_chip_reset");
				btmtk_reset_trigger(bdev);
			}

			goto intr_resub;
		}

#if BTMTK_ISOC_TEST
		if (urb->actual_length == HCE_SYNC_CONN_COMPLETE_LEN &&
				buf[0] == HCE_SYNC_CONN_COMPLETE && buf[2] == 0x00 &&
				bdev->bt_cfg.support_usb_sco_test) {
			if (sco_handle)
					BTMTK_WARN("More than ONE SCO link");

			sco_handle = buf[3] + (buf[4] << 8);
			if (buf[HCE_SYNC_CONN_COMPLETE_AIR_MODE_OFFSET] == HCE_SYNC_CONN_COMPLETE_AIR_MODE_CVSD)
				g_sco->isoc_alt_setting = ISOC_IF_ALT_CVSD;
			else if (buf[HCE_SYNC_CONN_COMPLETE_AIR_MODE_OFFSET] == HCE_SYNC_CONN_COMPLETE_AIR_MODE_TRANSPARENT)
				g_sco->isoc_alt_setting = ISOC_IF_ALT_MSBC;
			else
				g_sco->isoc_alt_setting = ISOC_IF_ALT_DEFAULT;
			hdev->conn_hash.sco_num++;
			bdev->sco_num = hdev->conn_hash.sco_num;
			BTMTK_INFO("Synchronous Connection Complete, Handle=0x%04X, Isoc_Alt_Setting=%d",
						sco_handle, g_sco->isoc_alt_setting);
		} else if (buf[0] == HCE_DIS_CONN_COMPLETE && buf[2] == 0x00 &&
					bdev->bt_cfg.support_usb_sco_test) {
			if ((buf[3] + (buf[4] << 8)) == sco_handle) {
				hdev->conn_hash.sco_num--;
				bdev->sco_num = hdev->conn_hash.sco_num;
				BTMTK_INFO("Synchronous Disconnection Complete, 0x%04X", sco_handle);
				sco_handle = 0;
			}
		}
#endif
		err = btmtk_recv(hdev, cif_dev->urb_intr_buf, urb->actual_length + 1);
		if (err) {
			BTMTK_ERR("corrupted event packet, urb_intr_buf = %p, transfer_buffer = %p",
				cif_dev->urb_intr_buf, urb->transfer_buffer);
			btmtk_hci_snoop_print(urb->transfer_buffer, urb->actual_length);
			btmtk_hci_snoop_print(cif_dev->urb_intr_buf, urb->actual_length + 1);
			hdev->stat.err_rx++;
		}
	} else if (urb->status == -ENOENT) {
		BTMTK_INFO("%s: urb->status is ENOENT!", __func__);
		return;
	}

intr_resub:
	if (!test_bit(BTUSB_INTR_RUNNING, &bdev->flags)) {
		BTMTK_INFO("%s: test_bit is not running!", __func__);
		return;
	}

	usb_mark_last_busy(cif_dev->udev);
	usb_anchor_urb(urb, &cif_dev->intr_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected
		 */
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("urb %p failed to resubmit (%d)",
			       urb, -err);
		usb_unanchor_urb(urb);
	}
}

static int btusb_submit_intr_reset_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;
	struct btmtk_usb_dev *cif_dev = NULL;
#if BTMTK_RUNTIME_ENABLE
	int state;
#endif

	/* If WDT reset happened, fw will send a bytes (FF) to host */
	BTMTK_DBG("%s", __func__);

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (!cif_dev->reset_intr_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;
	/* Default size is 16 */
	/* size = le16_to_cpu(data->intr_ep->wMaxPacketSize); */
	/* Buzzard Endpoint description.
	 * bEndpointAddress     0x8f  EP 15 IN
	 * wMaxPacketSize     0x0001  1x 1 bytes
	 */
	size = le16_to_cpu(HCI_MAX_EVENT_SIZE);

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvintpipe(cif_dev->udev, cif_dev->reset_intr_ep->bEndpointAddress);

	/* fw issue, we need to submit urb with a byte
	 * If driver set size = le16_to_cpu(HCI_MAX_EVENT_SIZE) to usb_fill_int_urb
	 * We can't get interrupt callback from bus.
	 */
	usb_fill_int_urb(urb, cif_dev->udev, pipe, buf, 1,
			 btusb_intr_complete, hdev, cif_dev->reset_intr_ep->bInterval);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &cif_dev->intr_anchor);

#if BTMTK_RUNTIME_ENABLE
	state = btmtk_get_chip_state(bdev);
	if (state != BTMTK_STATE_RESUME) {
		err = usb_autopm_get_interface(cif_dev->intf);
		if (err < 0) {
			BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
		}
	}
#endif

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("%s: urb %p submission failed (%d)",
				   __func__, urb, -err);
		usb_unanchor_urb(urb);
	}

#if BTMTK_RUNTIME_ENABLE
	if (state != BTMTK_STATE_RESUME) {
		usb_autopm_put_interface(cif_dev->intf);
	}
#endif

	usb_free_urb(urb);

	return err;
}

static void btusb_mtk_wmt_recv(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = NULL;
	struct sk_buff *skb;
	unsigned char *skb_tmp = NULL;
	int err;

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;

	BTMTK_DBG("%s : urb %p status %d count %d", __func__, urb, urb->status,
	       urb->actual_length);

	if (urb->status == 0 && urb->actual_length > 0) {
		BTMTK_DBG_RAW(urb->transfer_buffer, urb->actual_length, "%s, recv evt", __func__);
		hdev->stat.byte_rx += urb->actual_length;
		skb = bt_skb_alloc(HCI_MAX_EVENT_SIZE, GFP_ATOMIC);
		if (!skb) {
			BTMTK_ERR("%s skb is null!", __func__);
			hdev->stat.err_rx++;
			goto exit;
		}

		if (urb->actual_length >= HCI_MAX_EVENT_SIZE) {
			BTMTK_ERR("%s urb->actual_length is invalid!", __func__);
			BTMTK_INFO_RAW(urb->transfer_buffer, urb->actual_length,
				"urb->actual_length:%d, urb->transfer_buffer:%p",
				urb->actual_length, urb->transfer_buffer);
			kfree_skb(skb);
			hdev->stat.err_rx++;
			goto exit;
		}
		hci_skb_pkt_type(skb) = HCI_EVENT_PKT;
		if (skb_tailroom(skb) >= urb->actual_length) {
			skb_tmp = skb_put(skb, urb->actual_length);
			if (!skb_tmp) {
				BTMTK_ERR("%s, skb_put failed!", __func__);
				kfree_skb(skb);
				return;
			}
			memcpy(skb_tmp, urb->transfer_buffer, urb->actual_length);
			BTMTK_DBG_RAW(skb->data, skb->len, "%s, skb recv evt", __func__);

			hci_recv_frame(hdev, skb);
			return;
		} else {
			BTMTK_INFO("%s: the size of skb is too small!", __func__);
			kfree_skb(skb);
			return;
		}
	} else if (urb->status == -ENOENT) {
		/* it's error case, don't re-submit urb. */
		BTMTK_INFO("%s: urb->status is ENOENT!", __func__);
		return;
	}

	usb_mark_last_busy(cif_dev->udev);
	udelay(100);

	if (!test_bit(BTUSB_WMT_RUNNING, &bdev->flags)) {
		BTMTK_INFO("%s test flag failed", __func__);
		goto exit;
	}

	usb_anchor_urb(urb, &cif_dev->ctrl_anchor);
	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		kfree(urb->setup_packet);
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected
		 */
		if (err != -EPERM && err != -ENODEV)
			usb_unanchor_urb(urb);
	}

	return;

exit:
	kfree(urb->setup_packet);
}

static int btusb_submit_wmt_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = NULL;
	struct usb_ctrlrequest *dr;
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;
	unsigned int ifnum_base;

	BTMTK_DBG("%s", __func__);

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;

	size = le16_to_cpu(HCI_MAX_EVENT_SIZE);

	dr = kmalloc(sizeof(*dr), GFP_KERNEL);
	if (!dr) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;

	ifnum_base = cif_dev->intf->cur_altsetting->desc.bInterfaceNumber;

	if (BTMTK_IS_BT_0_INTF(ifnum_base)) {
		dr->bRequestType = 0xC0;
		dr->bRequest     = 0x01;
		dr->wIndex       = 0;
		dr->wValue       = 0x30;
		dr->wLength      = __cpu_to_le16(size);
	} else if (BTMTK_IS_BT_1_INTF(ifnum_base)) {
		dr->bRequestType = 0xA1;
		dr->bRequest     = 0x01;
		dr->wIndex       = 0x03;
		dr->wValue       = 0x30;
		dr->wLength      = __cpu_to_le16(size);
	}

	pipe = usb_rcvctrlpipe(cif_dev->udev, 0);

	buf = kmalloc(size, GFP_KERNEL);
	if (!buf) {
		kfree(dr);
		usb_free_urb(urb);
		return -ENOMEM;
	}

	usb_fill_control_urb(urb, cif_dev->udev, pipe, (void *)dr,
			     buf, size, btusb_mtk_wmt_recv, hdev);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &cif_dev->ctrl_anchor);

#if BTMTK_RUNTIME_ENABLE
	err = usb_autopm_get_interface(cif_dev->intf);
	if (err < 0) {
		BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
	}
#endif

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("%s: urb %p submission failed (%d)",
					__func__, urb, -err);
		kfree(dr);
		usb_unanchor_urb(urb);
	}

#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif

	usb_free_urb(urb);

	return err;
}

static int btusb_submit_intr_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = NULL;
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;
#if BTMTK_RUNTIME_ENABLE
	int state;
#endif

	BTMTK_DBG("%s", __func__);

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;

	if (!cif_dev->intr_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;

	/* size = le16_to_cpu(data->intr_ep->wMaxPacketSize); */
	/* Buzzard Endpoint description.
	 * bEndpointAddress     0x81  EP 1 IN
	 * wMaxPacketSize     0x0010  1x 16 bytes
	 */
	size = le16_to_cpu(HCI_MAX_EVENT_SIZE);
	BTMTK_INFO("%s: maximum packet size:%d", __func__, size);

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvintpipe(cif_dev->udev, cif_dev->intr_ep->bEndpointAddress);

	usb_fill_int_urb(urb, cif_dev->udev, pipe, buf, size,
			 btusb_intr_complete, hdev, cif_dev->intr_ep->bInterval);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &cif_dev->intr_anchor);

#if BTMTK_RUNTIME_ENABLE
	state = btmtk_get_chip_state(bdev);
	if (state != BTMTK_STATE_RESUME) {
		err = usb_autopm_get_interface(cif_dev->intf);
		if (err < 0) {
			BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
		}
	}
#endif

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("%s: urb %p submission failed (%d)",
			       __func__, urb, -err);
		usb_unanchor_urb(urb);
	}

#if BTMTK_RUNTIME_ENABLE
	if (state != BTMTK_STATE_RESUME) {
		usb_autopm_put_interface(cif_dev->intf);
	}
#endif

	usb_free_urb(urb);

	return err;
}

static void btusb_bulk_complete(struct urb *urb)
{
	struct hci_dev *hdev = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_usb_dev *cif_dev = NULL;
	int err;
	u8 *buf;
	u16 len = 0;
	static u8 block_bulkin_usb_warn;

	if (urb == NULL) {
		BTMTK_ERR("%s: ERROR, urb is NULL!", __func__);
		return;
	}

	hdev = urb->context;
	if (hdev == NULL) {
		BTMTK_ERR("%s: ERROR, hdev is NULL!", __func__);
		return;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL) {
		BTMTK_ERR("%s: ERROR, bdev is NULL!", __func__);
		return;
	}

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: ERROR, cif_dev is NULL!", __func__);
		return;
	}

	if (urb->status != 0 && block_bulkin_usb_warn < 10) {
		block_bulkin_usb_warn++;
		BTMTK_INFO("%s: urb %p urb->status %d count %d", __func__, urb,
			urb->status, urb->actual_length);
	} else if (urb->status == 0 && urb->actual_length != 0)
		block_bulkin_usb_warn = 0;

	/*
	 * This flag didn't support in kernel 4.x
	 * Driver will remove it
	 * if (!test_bit(HCI_RUNNING, &hdev->flags))
	 * return;
	 */
	if (urb->status == 0) {
		buf = urb->transfer_buffer;
		if (!((buf[0] == 0x6f && buf[1] == 0xfc) ||
			((buf[0] == 0xff || buf[0] == 0xfe) && buf[1] == 0x05)))
			btmtk_hci_snoop_save(HCI_SNOOP_TYPE_RX_ACL_HIF,
				buf, urb->actual_length);

		hdev->stat.byte_rx += urb->actual_length;
		if (!cif_dev->urb_bulk_buf) {
			BTMTK_ERR("%s: bdev->urb_bulk_buf is NULL!", __func__);
			return;
		}

		len = get_pkt_len(HCI_ACLDATA_PKT, buf);
		if (urb->actual_length >= URB_MAX_BUFFER_SIZE ||
			urb->actual_length != len) {
			BTMTK_ERR("%s urb->actual_length is invalid, len = %d!", __func__, len);
			btmtk_hci_snoop_print(urb->transfer_buffer, urb->actual_length);
			goto bulk_resub;
		}
		memset(cif_dev->urb_bulk_buf, 0, URB_MAX_BUFFER_SIZE);
		cif_dev->urb_bulk_buf[0] = HCI_ACLDATA_PKT;
		memcpy(cif_dev->urb_bulk_buf + 1, urb->transfer_buffer, urb->actual_length);

		/* BTMTK_DBG_RAW(bdev->urb_bulk_buf, urb->actual_length + 1, "%s, recv from bulk", __func__); */
		err = btmtk_recv(hdev, cif_dev->urb_bulk_buf, urb->actual_length + 1);
		if (err) {
			BTMTK_ERR("corrupted ACL packet, urb_bulk_buf = %p, transfer_buffer = %p",
				cif_dev->urb_bulk_buf, urb->transfer_buffer);
			btmtk_hci_snoop_print(urb->transfer_buffer, urb->actual_length);
			btmtk_hci_snoop_print(cif_dev->urb_bulk_buf, urb->actual_length + 1);
			hdev->stat.err_rx++;
		}
	} else if (urb->status == -ENOENT) {
		/* Avoid suspend failed when usb_kill_urb */
		BTMTK_INFO("urb %p status %d count %d", urb, urb->status,
			urb->actual_length);
		return;
	}

bulk_resub:
	if (!test_bit(BTUSB_BULK_RUNNING, &bdev->flags)) {
		BTMTK_INFO("%s test flag failed", __func__);
		return;
	}

	usb_anchor_urb(urb, &cif_dev->bulk_anchor);
	usb_mark_last_busy(cif_dev->udev);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected
		 */
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("urb %p failed to resubmit (%d)",
			       urb, -err);
		usb_unanchor_urb(urb);
	}
}

static int btusb_submit_bulk_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = NULL;
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size = HCI_MAX_FRAME_SIZE;
#if BTMTK_RUNTIME_ENABLE
	int state;
#endif

	BTMTK_DBG("%s", __func__);

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;

	if (!cif_dev->bulk_rx_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvbulkpipe(cif_dev->udev, cif_dev->bulk_rx_ep->bEndpointAddress);

	usb_fill_bulk_urb(urb, cif_dev->udev, pipe, buf, size,
			  btusb_bulk_complete, hdev);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_mark_last_busy(cif_dev->udev);
	usb_anchor_urb(urb, &cif_dev->bulk_anchor);

#if BTMTK_RUNTIME_ENABLE
	state = btmtk_get_chip_state(bdev);
	if (state != BTMTK_STATE_RESUME) {
		err = usb_autopm_get_interface(cif_dev->intf);
		if (err < 0) {
			BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
		}
	}
#endif

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("%s: urb %p submission failed (%d)",
			       __func__, urb, -err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

#if BTMTK_RUNTIME_ENABLE
	if (state != BTMTK_STATE_RESUME) {
		usb_autopm_put_interface(cif_dev->intf);
	}
#endif

	return err;
}

static void btusb_ble_isoc_complete(struct urb *urb)
{
	struct hci_dev *hdev = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_usb_dev *cif_dev = NULL;
	int err;
	u8 *isoc_buf;
	int isoc_pkt_len;

	/*
	 * This flag didn't support in kernel 4.x
	 * Driver will remove it
	 * if (!test_bit(HCI_RUNNING, &hdev->flags))
	 * return;
	 */
	if (urb == NULL) {
		BTMTK_ERR("%s: ERROR, urb is NULL!", __func__);
		return;
	}

	hdev = urb->context;
	if (hdev == NULL) {
		BTMTK_ERR("%s: ERROR, hdev is NULL!", __func__);
		return;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL) {
		BTMTK_ERR("%s: ERROR, bdev is NULL!", __func__);
		return;
	}

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: ERROR, cif_dev is NULL!", __func__);
		return;
	}

	if (urb->status == 0) {
		isoc_buf = urb->transfer_buffer;
		btmtk_hci_snoop_save(HCI_SNOOP_TYPE_RX_ISO_HIF,
			isoc_buf, urb->actual_length);
		hdev->stat.byte_rx += urb->actual_length;

		if (!cif_dev->urb_ble_isoc_buf) {
			BTMTK_ERR("%s: bdev->urb_ble_isoc_buf is NULL!", __func__);
			return;
		}

		isoc_pkt_len = get_pkt_len(HCI_ISO_PKT, isoc_buf);
		/* Skip padding */
		BTMTK_DBG("%s: isoc_pkt_len = %d, urb->actual_length = %d", __func__, isoc_pkt_len, urb->actual_length);
		if (isoc_pkt_len == HCI_ISO_PKT_HEADER_SIZE) {
			BTMTK_DBG("%s: goto ble_iso_resub", __func__);
			goto ble_iso_resub;
		}

		if (urb->actual_length + HCI_ISO_PKT_WITH_ACL_HEADER_SIZE > URB_MAX_BUFFER_SIZE) {
			BTMTK_ERR("%s urb->actual_length is invalid!", __func__);
			btmtk_hci_snoop_print(urb->transfer_buffer, urb->actual_length);
			goto ble_iso_resub;
		}
		/* It's mtk specific heade for stack
		 * hci layered didn't support 0x05 for ble iso, it will drop the packet type with 0x05
		 * Driver will replace 0x05 to 0x02
		 * header format : 0x02 0x00 0x44 xx xx + isoc packet header & payload
		 */
		memset(cif_dev->urb_ble_isoc_buf, 0, URB_MAX_BUFFER_SIZE);
		cif_dev->urb_ble_isoc_buf[0] = HCI_ACLDATA_PKT;
		cif_dev->urb_ble_isoc_buf[1] = 0x00;
		cif_dev->urb_ble_isoc_buf[2] = 0x44;
		cif_dev->urb_ble_isoc_buf[3] = (isoc_pkt_len & 0x00ff);
		cif_dev->urb_ble_isoc_buf[4] = (isoc_pkt_len >> 8);
		memcpy(cif_dev->urb_ble_isoc_buf + HCI_ISO_PKT_WITH_ACL_HEADER_SIZE,
			urb->transfer_buffer, isoc_pkt_len);

		BTMTK_DBG_RAW(cif_dev->urb_ble_isoc_buf,
			isoc_pkt_len + HCI_ISO_PKT_WITH_ACL_HEADER_SIZE,
			"%s: raw data is :", __func__);

		err = btmtk_recv(hdev, cif_dev->urb_ble_isoc_buf,
			isoc_pkt_len + HCI_ISO_PKT_WITH_ACL_HEADER_SIZE);
		if (err) {
			BTMTK_ERR("corrupted ACL packet");
			hdev->stat.err_rx++;
		}
	} else if (urb->status == -ENOENT) {
		BTMTK_INFO("%s: urb->status is ENOENT!", __func__);
		return;
	}

ble_iso_resub:
	if (!test_bit(BTUSB_BLE_ISOC_RUNNING, &bdev->flags)) {
		BTMTK_INFO("%s: bdev->flags is not RUNNING!", __func__);
		return;
	}

	usb_anchor_urb(urb, &cif_dev->ble_isoc_anchor);
	usb_mark_last_busy(cif_dev->udev);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected
		 */
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("urb %p failed to resubmit (%d)",
			       urb, -err);
		usb_unanchor_urb(urb);
	}
}

static int btusb_submit_intr_ble_isoc_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = NULL;
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;
#if BTMTK_RUNTIME_ENABLE
	int state;
#endif

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;

	if (!cif_dev->intr_iso_rx_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;
	/* Default size is 16 */
	/* size = le16_to_cpu(data->intr_ep->wMaxPacketSize); */
	/* we need to consider the wMaxPacketSize in BLE ISO */
	size = le16_to_cpu(2000);

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvintpipe(cif_dev->udev, cif_dev->intr_iso_rx_ep->bEndpointAddress);
	BTMTK_INFO("btusb_submit_intr_iso_urb : polling  0x%02X",  cif_dev->intr_iso_rx_ep->bEndpointAddress);

	usb_fill_int_urb(urb, cif_dev->udev, pipe, buf, size,
			 btusb_ble_isoc_complete, hdev, cif_dev->intr_iso_rx_ep->bInterval);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &cif_dev->ble_isoc_anchor);

#if BTMTK_RUNTIME_ENABLE
	state = btmtk_get_chip_state(bdev);
	if (state != BTMTK_STATE_RESUME) {
		err = usb_autopm_get_interface(cif_dev->intf);
		if (err < 0) {
			BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
		}
	}
#endif

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("%s: urb %p submission failed (%d)",
				   __func__, urb, -err);
		usb_unanchor_urb(urb);
	}

#if BTMTK_RUNTIME_ENABLE
	if (state != BTMTK_STATE_RESUME) {
		usb_autopm_put_interface(cif_dev->intf);
	}
#endif

	usb_free_urb(urb);

	return err;
}

static void btusb_isoc_complete(struct urb *urb)
{
	struct hci_dev *hdev = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_usb_dev *cif_dev = NULL;
	int i, err;

	if (urb == NULL) {
		BTMTK_ERR("%s: ERROR, urb is NULL!", __func__);
		return;
	}

	hdev = urb->context;
	if (hdev == NULL) {
		BTMTK_ERR("%s: ERROR, hdev is NULL!", __func__);
		return;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL) {
		BTMTK_ERR("%s: ERROR, bdev is NULL!", __func__);
		return;
	}

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: ERROR, cif_dev is NULL!", __func__);
		return;
	}


	BTMTK_DBG("urb %p status %d count %d", urb, urb->status,
	       urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return;

	if (urb->status == 0) {
		for (i = 0; i < urb->number_of_packets; i++) {
			unsigned int offset = urb->iso_frame_desc[i].offset;
			unsigned int length = urb->iso_frame_desc[i].actual_length;

			if (urb->iso_frame_desc[i].status)
				continue;

			hdev->stat.byte_rx += length;

			if (btusb_recv_isoc(bdev, urb->transfer_buffer + offset,
					    length) < 0) {
				BTMTK_ERR("corrupted SCO packet");
				hdev->stat.err_rx++;
			}
		}
	} else if (urb->status == -ENOENT) {
		BTMTK_INFO("%s: urb->status is ENOENT!", __func__);
		return;
	}

	if (!test_bit(BTUSB_ISOC_RUNNING, &bdev->flags)) {
		BTMTK_INFO("%s: bdev->flags is RUNNING!", __func__);
		return;
	}

	usb_anchor_urb(urb, &cif_dev->isoc_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected
		 */
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("urb %p failed to resubmit (%d)",
			       urb, -err);
		usb_unanchor_urb(urb);
	}
}

static inline void __fill_isoc_descriptor(struct urb *urb, int len, int mtu)
{
	int i, offset = 0;

	BTMTK_DBG("len %d mtu %d", len, mtu);

	for (i = 0; i < BTUSB_MAX_ISOC_FRAMES && len >= mtu;
					i++, offset += mtu, len -= mtu) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = mtu;
	}

	if (len && i < BTUSB_MAX_ISOC_FRAMES) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = len;
		i++;
	}

	urb->number_of_packets = i;
}

static int btusb_submit_isoc_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = NULL;
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;
#if BTMTK_RUNTIME_ENABLE
	int state;
#endif

	BTMTK_DBG("%s", __func__);

#if BTMTK_ISOC_TEST
	if (bdev->bt_cfg.support_usb_sco_test) {
		if (g_sco->isoc_urb_submitted) {
			BTMTK_WARN("%s: already submitted", __func__);
			return 0;
		}
		g_sco->isoc_urb_submitted = 0;
	}
#endif

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;

	if (!cif_dev->isoc_rx_ep)
		return -ENODEV;

	urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, mem_flags);
	if (!urb)
		return -ENOMEM;

	size = le16_to_cpu(cif_dev->isoc_rx_ep->wMaxPacketSize) *
						BTUSB_MAX_ISOC_FRAMES;

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvisocpipe(cif_dev->udev, cif_dev->isoc_rx_ep->bEndpointAddress);

	usb_fill_int_urb(urb, cif_dev->udev, pipe, buf, size, btusb_isoc_complete,
			 hdev, cif_dev->isoc_rx_ep->bInterval);

	urb->transfer_flags = URB_FREE_BUFFER | URB_ISO_ASAP;

	__fill_isoc_descriptor(urb, size,
			       le16_to_cpu(cif_dev->isoc_rx_ep->wMaxPacketSize));

	usb_anchor_urb(urb, &cif_dev->isoc_anchor);

#if BTMTK_RUNTIME_ENABLE
	state = btmtk_get_chip_state(bdev);
	if (state != BTMTK_STATE_RESUME) {
		err = usb_autopm_get_interface(cif_dev->intf);
		if (err < 0) {
			BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
		}
	}
#endif

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("%s: urb %p submission failed (%d)",
			       __func__, urb, -err);
		usb_unanchor_urb(urb);
#if BTMTK_ISOC_TEST
	} else if (bdev->bt_cfg.support_usb_sco_test) {
		g_sco->isoc_urb_submitted = 1;
#endif
	}

#if BTMTK_RUNTIME_ENABLE
	if (state != BTMTK_STATE_RESUME) {
		usb_autopm_put_interface(cif_dev->intf);
	}
#endif

	usb_free_urb(urb);

	return err;
}

static void btusb_tx_complete(struct urb *urb)
{
	struct sk_buff *skb = urb->context;
	struct hci_dev *hdev = (struct hci_dev *)skb->dev;
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	unsigned long flags;

	BTMTK_DBG("urb %p status %d count %d", urb, urb->status,
	       urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	if (!urb->status)
		hdev->stat.byte_tx += urb->transfer_buffer_length;
	else
		hdev->stat.err_tx++;

done:
	spin_lock_irqsave(&bdev->txlock, flags);
	bdev->tx_in_flight--;
	spin_unlock_irqrestore(&bdev->txlock, flags);

	kfree(urb->setup_packet);

	kfree_skb(skb);
}

static void btusb_isoc_tx_complete(struct urb *urb)
{
	struct sk_buff *skb = urb->context;
	struct hci_dev *hdev = (struct hci_dev *)skb->dev;
#if BTMTK_ISOC_TEST
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
#endif

	BTMTK_DBG("urb %p status %d count %d", urb, urb->status,
	       urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	if (!urb->status)
		hdev->stat.byte_tx += urb->transfer_buffer_length;
	else
		hdev->stat.err_tx++;

#if BTMTK_ISOC_TEST
	if (bdev->bt_cfg.support_usb_sco_test) {
		if (atomic_read(&g_sco->isoc_out_count))
			atomic_dec(&g_sco->isoc_out_count);
	}
#endif
done:
	kfree(urb->setup_packet);

	kfree_skb(skb);
}

static int btmtk_usb_open(struct hci_dev *hdev)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	int err;
	unsigned int ifnum_base;
	unsigned char state = 0;

	if (test_bit(BTUSB_OPENED, &bdev->flags)) {
		BTMTK_INFO("%s: usb is opened, return", __func__);
		return -EBUSY;
	}

	BTMTK_INFO("%s enter!", __func__);

#if BTMTK_ISOC_TEST
	if (bdev->bt_cfg.support_usb_sco_test) {
		err = btmtk_usb_fops_sco_init(bdev);
		if (err < 0) {
			BTMTK_ERR("[ERR] create stpbt_sco failed!");
		} else if (err == 0 && !g_sco->bdev) {
			BTMTK_DBG("create g_sco->bdev");
			g_sco->bdev = bdev;
		}
	}
#endif

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_DISCONNECT) {
		BTMTK_ERR("%s: state is disconnect, return", __func__);
		return -1;
	}
	if (state != BTMTK_STATE_SUSPEND
		&& state != BTMTK_STATE_RESUME
		&& state != BTMTK_STATE_STANDBY) {
		err = usb_autopm_get_interface(cif_dev->intf);
		if (err < 0)
			return err;
	}

	cif_dev->intf->needs_remote_wakeup = 1;

	if (test_and_set_bit(BTUSB_INTR_RUNNING, &bdev->flags))
		goto done;

	ifnum_base = cif_dev->intf->cur_altsetting->desc.bInterfaceNumber;

	if (is_mt6639(bdev->chip_id) || is_mt7902(bdev->chip_id)
			|| is_mt7922(bdev->chip_id) || is_mt7961(bdev->chip_id)
			|| is_mt7925(bdev->chip_id)) {
		BTMTK_INFO("%s 79xx submit urb", __func__);
		if (BTMTK_IS_BT_0_INTF(ifnum_base)) {
			if (cif_dev->reset_intr_ep) {
				err = btusb_submit_intr_reset_urb(hdev, GFP_KERNEL);
				if (err < 0)
					goto failed;
			} else
				BTMTK_INFO("%s, reset_intr_ep missing, don't submit_intr_reset_urb!",
					__func__);

			if (cif_dev->intr_iso_rx_ep) {
				err = btusb_submit_intr_ble_isoc_urb(hdev, GFP_KERNEL);
				if (err < 0) {
					usb_kill_anchored_urbs(&cif_dev->ble_isoc_anchor);
					goto failed;
				}
				set_bit(BTUSB_BLE_ISOC_RUNNING, &bdev->flags);
			} else
				BTMTK_INFO("%s, intr_iso_rx_ep missing, don't submit_intr_ble_isoc_urb!",
					__func__);
		} else if (BTMTK_IS_BT_1_INTF(ifnum_base)) {
			/*need to do in bt_open in btmtk_main.c */
			/* btmtk_usb_send_power_on_cmd_7668(hdev); */
		}
	}

	err = btusb_submit_intr_urb(hdev, GFP_KERNEL);
	if (err < 0)
		goto failed;

	err = btusb_submit_bulk_urb(hdev, GFP_KERNEL);
	if (err < 0) {
		usb_kill_anchored_urbs(&cif_dev->intr_anchor);
		goto failed;
	}

	set_bit(BTUSB_BULK_RUNNING, &bdev->flags);
	set_bit(BTUSB_OPENED, &bdev->flags);

done:
	if (state != BTMTK_STATE_SUSPEND
		&& state != BTMTK_STATE_RESUME
		&& state != BTMTK_STATE_STANDBY)
		usb_autopm_put_interface(cif_dev->intf);
	return 0;

failed:
	clear_bit(BTUSB_INTR_RUNNING, &bdev->flags);
	if (state != BTMTK_STATE_SUSPEND
		&& state != BTMTK_STATE_RESUME
		&& state != BTMTK_STATE_STANDBY)
		usb_autopm_put_interface(cif_dev->intf);
	return err;
}

static void btusb_stop_traffic(struct btmtk_usb_dev *cif_dev)
{
	usb_kill_anchored_urbs(&cif_dev->intr_anchor);
	usb_kill_anchored_urbs(&cif_dev->bulk_anchor);
	usb_kill_anchored_urbs(&cif_dev->isoc_anchor);
	usb_kill_anchored_urbs(&cif_dev->ctrl_anchor);
	usb_kill_anchored_urbs(&cif_dev->ble_isoc_anchor);
}

static int btmtk_usb_close(struct hci_dev *hdev)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	int err;
	unsigned char state = 0;

	BTMTK_INFO("%s enter!", __func__);

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_DISCONNECT) {
		BTMTK_ERR("%s: state is disconnect, return", __func__);
		return -1;
	}

#if BTMTK_ISOC_TEST
	if (bdev->bt_cfg.support_usb_sco_test) {
		btmtk_usb_fops_sco_exit(bdev);
	}
#endif

	cancel_work_sync(&bdev->work);
	cancel_work_sync(&bdev->waker);

	clear_bit(BTUSB_BLE_ISOC_RUNNING, &bdev->flags);
	clear_bit(BTUSB_ISOC_RUNNING, &bdev->flags);
	clear_bit(BTUSB_BULK_RUNNING, &bdev->flags);
	clear_bit(BTUSB_INTR_RUNNING, &bdev->flags);
	clear_bit(BTUSB_WMT_RUNNING, &bdev->flags);
	clear_bit(BTUSB_OPENED, &bdev->flags);

	btusb_stop_traffic(cif_dev);
	btusb_free_frags(bdev);

	state = btmtk_get_chip_state(bdev);
	if (state != BTMTK_STATE_SUSPEND
		&& state != BTMTK_STATE_RESUME
		&& state != BTMTK_STATE_STANDBY) {
		err = usb_autopm_get_interface(cif_dev->intf);
		if (err < 0)
			goto failed;
	}

	cif_dev->intf->needs_remote_wakeup = 0;

	if (state != BTMTK_STATE_SUSPEND
		&& state != BTMTK_STATE_RESUME
		&& state != BTMTK_STATE_STANDBY)
		usb_autopm_put_interface(cif_dev->intf);

failed:
	return 0;
}

/* Maybe will be used in the future*/
#if 0
static int btusb_flush(struct hci_dev *hdev)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);

	BTMTK_DBG("%s", __func__);

	usb_kill_anchored_urbs(&bdev->tx_anchor);
	btusb_free_frags(bdev);

	return 0;
}
#endif

static struct urb *alloc_intr_iso_urb(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	struct urb *urb;
	unsigned int pipe;

	if (!cif_dev->intr_iso_tx_ep)
		return ERR_PTR(-ENODEV);

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return ERR_PTR(-ENOMEM);

	pipe = usb_sndintpipe(cif_dev->udev, cif_dev->intr_iso_tx_ep->bEndpointAddress);

	usb_fill_int_urb(urb, cif_dev->udev, pipe,
			  skb->data, skb->len, btusb_tx_complete, skb, 1);

	skb->dev = (void *)hdev;

	return urb;
}

static struct urb *alloc_ctrl_bgf1_urb(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	struct usb_ctrlrequest *dr;
	struct urb *urb;
	unsigned int pipe;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return ERR_PTR(-ENOMEM);

	BTMTK_DBG("%s", __func__);
	dr = kmalloc(sizeof(*dr), GFP_KERNEL);
	if (!dr) {
		usb_free_urb(urb);
		return ERR_PTR(-ENOMEM);
	}

	dr->bRequestType = 0x21;
	dr->bRequest	 = 0x00;
	dr->wIndex	   = 3;
	dr->wValue	   = 0;
	dr->wLength	  = __cpu_to_le16(skb->len);

	pipe = usb_sndctrlpipe(cif_dev->udev, 0x00);

	usb_fill_control_urb(urb, cif_dev->udev, pipe, (void *)dr,
				 skb->data, skb->len, btusb_tx_complete, skb);

	skb->dev = (void *)hdev;

	return urb;
}

static struct urb *alloc_bulk_cmd_urb(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	struct urb *urb;
	unsigned int pipe;

	BTMTK_DBG("%s start", __func__);
	if (!cif_dev->bulk_cmd_tx_ep)
		return ERR_PTR(-ENODEV);

	BTMTK_DBG("%s", __func__);
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return ERR_PTR(-ENOMEM);

	pipe = usb_sndbulkpipe(cif_dev->udev, cif_dev->bulk_cmd_tx_ep->bEndpointAddress);

	usb_fill_bulk_urb(urb, cif_dev->udev, pipe,
			  skb->data, skb->len, btusb_tx_complete, skb);

	skb->dev = (void *)hdev;

	return urb;
}

static struct urb *alloc_ctrl_urb(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	struct usb_ctrlrequest *dr;
	struct urb *urb;
	unsigned int pipe;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return ERR_PTR(-ENOMEM);

	dr = kmalloc(sizeof(*dr), GFP_KERNEL);
	if (!dr) {
		usb_free_urb(urb);
		return ERR_PTR(-ENOMEM);
	}

	dr->bRequestType = cif_dev->cmdreq_type;
	dr->bRequest     = cif_dev->cmdreq;
	dr->wIndex       = 0;
	dr->wValue       = 0;
	dr->wLength      = __cpu_to_le16(skb->len);

	pipe = usb_sndctrlpipe(cif_dev->udev, 0x00);

	usb_fill_control_urb(urb, cif_dev->udev, pipe, (void *)dr,
			     skb->data, skb->len, btusb_tx_complete, skb);

	skb->dev = (void *)hdev;

	return urb;
}

static struct urb *alloc_bulk_urb(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	struct urb *urb;
	unsigned int pipe;

	if (!cif_dev->bulk_tx_ep)
		return ERR_PTR(-ENODEV);

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return ERR_PTR(-ENOMEM);

	pipe = usb_sndbulkpipe(cif_dev->udev, cif_dev->bulk_tx_ep->bEndpointAddress);

	usb_fill_bulk_urb(urb, cif_dev->udev, pipe,
			  skb->data, skb->len, btusb_tx_complete, skb);

	skb->dev = (void *)hdev;

	return urb;
}

static struct urb *alloc_isoc_urb(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	struct urb *urb;
	unsigned int pipe;

	if (!cif_dev->isoc_tx_ep)
		return ERR_PTR(-ENODEV);

	urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, GFP_KERNEL);
	if (!urb)
		return ERR_PTR(-ENOMEM);

	pipe = usb_sndisocpipe(cif_dev->udev, cif_dev->isoc_tx_ep->bEndpointAddress);

	usb_fill_int_urb(urb, cif_dev->udev, pipe,
			 skb->data, skb->len, btusb_isoc_tx_complete,
			 skb, cif_dev->isoc_tx_ep->bInterval);

	urb->transfer_flags  = URB_ISO_ASAP;

	__fill_isoc_descriptor(urb, skb->len,
			       le16_to_cpu(cif_dev->isoc_tx_ep->wMaxPacketSize));

	skb->dev = (void *)hdev;

	return urb;
}

static int submit_tx_urb(struct hci_dev *hdev, struct urb *urb, int type)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	int err;

	switch (type) {
	case HCI_COMMAND_PKT:
		btmtk_hci_snoop_save(HCI_SNOOP_TYPE_CMD_HIF,
			urb->transfer_buffer, urb->transfer_buffer_length);
		break;
	case HCI_ACLDATA_PKT:
		btmtk_hci_snoop_save(HCI_SNOOP_TYPE_TX_ACL_HIF,
			urb->transfer_buffer, urb->transfer_buffer_length);
		break;
	case HCI_SCODATA_PKT:
		break;
	case HCI_ISO_PKT:
		btmtk_hci_snoop_save(HCI_SNOOP_TYPE_TX_ISO_HIF,
			urb->transfer_buffer, urb->transfer_buffer_length);
		break;
	default:
		BTMTK_INFO("%s: invalid type(%d)", __func__, type);
	}

	usb_anchor_urb(urb, &cif_dev->tx_anchor);
#if BTMTK_RUNTIME_ENABLE
	err = usb_autopm_get_interface(cif_dev->intf);
	if (err < 0) {
		BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
	}
#endif
	err = usb_submit_urb(urb, GFP_KERNEL);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTMTK_ERR("%s: urb %p submission failed (%d)",
			       __func__, urb, -err);
		kfree(urb->setup_packet);
		usb_unanchor_urb(urb);
	} else {
		usb_mark_last_busy(cif_dev->udev);
	}

#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif

	usb_free_urb(urb);
	return err;
}

static int submit_or_queue_tx_urb(struct hci_dev *hdev, struct urb *urb, int type)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	unsigned long flags;
	bool suspending;

	spin_lock_irqsave(&bdev->txlock, flags);
	suspending = test_bit(BTUSB_SUSPENDING, &bdev->flags);
	if (!suspending)
		bdev->tx_in_flight++;
	spin_unlock_irqrestore(&bdev->txlock, flags);

	if (!suspending)
		return submit_tx_urb(hdev, urb, type);

	schedule_work(&bdev->waker);

	usb_free_urb(urb);
	return 0;
}

static int btusb_send_frame(struct hci_dev *hdev, struct sk_buff *skb, bool flag)
{
	struct urb *urb = NULL;
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	unsigned int ifnum_base;
	int ret = 0;
	struct sk_buff *iso_skb = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	struct sk_buff *evt_skb;
	u16 crBaseAddr = 0, crRegOffset = 0;
	unsigned char *skb_tmp = NULL;
	struct data_struct wmt_trigger_assert = {0}, notify_alt_evt = {0};

	BTMTK_DBG("%s enter", __func__);

	if (skb->len <= 0) {
		ret = -EFAULT;
		BTMTK_ERR("%s: target packet length:%zu is not allowed", __func__, (size_t)skb->len);
	}

	ifnum_base = cif_dev->intf->cur_altsetting->desc.bInterfaceNumber;

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, WMT_ASSERT_CMD, wmt_trigger_assert);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, NOTIFY_ALT_EVT, notify_alt_evt);

	skb_pull(skb, 1);
	BTMTK_DBG_RAW(skb->data, skb->len, "%s, send_frame, type = %d", __func__,
		hci_skb_pkt_type(skb));
	switch (hci_skb_pkt_type(skb)) {
	case HCI_COMMAND_PKT:
		if (skb->len > 7) {
			if (skb->data[0] == 0x6f && skb->data[1] == 0xfc &&
					skb->data[2] == 0x06 && skb->data[3] == 0x01 &&
					skb->data[4] == 0xff && skb->data[5] == 0x03 &&
					skb->data[6] == 0x00 && skb->data[7] == 0x00) {
				/* return evt to upper layered */
				evt_skb = skb_copy(skb, GFP_KERNEL);
				if (!evt_skb) {
					BTMTK_ERR("%s skb_copy failed", __func__);
					return -ENOMEM;
				}

				bt_cb(evt_skb)->pkt_type = notify_alt_evt.content[0];
				memcpy(evt_skb->data, &notify_alt_evt.content[1], notify_alt_evt.len - 1);
				evt_skb->len = notify_alt_evt.len - 1;
				/* After set alternate setting, we will return evt to boots */
				hci_recv_frame(hdev, evt_skb);
				hdev->conn_hash.sco_num++;
				bdev->sco_num = hdev->conn_hash.sco_num;
				cif_dev->new_isoc_altsetting = skb->data[8];
				BTMTK_INFO("alt_setting = %d, new_isoc_altsetting_interface = %d",
						cif_dev->new_isoc_altsetting, cif_dev->new_isoc_altsetting_interface);
				schedule_work(&bdev->work);
				msleep(20);
				kfree_skb(skb);
				skb = NULL;
				return 0;
			} else if (skb->data[0] == 0x6f && skb->data[1] == 0xfc &&
					skb->data[2] == 0x07 && skb->data[3] == 0x01 &&
					skb->data[4] == 0xff && skb->data[5] == 0x03 &&
					skb->data[6] == 0x00 && skb->data[7] == 0x00 && skb->data[9] == 0x00) {
				evt_skb = skb_copy(skb, GFP_KERNEL);
				if (!evt_skb) {
					BTMTK_ERR("%s skb_copy failed", __func__);
					return -ENOMEM;
				}

				bt_cb(evt_skb)->pkt_type = notify_alt_evt.content[0];
				memcpy(evt_skb->data, &notify_alt_evt.content[1], notify_alt_evt.len - 1);
				evt_skb->len = notify_alt_evt.len - 1;
				/* After set alternate setting, we will return evt to boots */
				hci_recv_frame(hdev, evt_skb);
				/* if sco_num == 0, btusb_work will set alternate setting to zero */
				hdev->conn_hash.sco_num--;
				bdev->sco_num = hdev->conn_hash.sco_num;
				cif_dev->new_isoc_altsetting_interface = skb->data[8];
				BTMTK_INFO("alt_setting to = %d, new_isoc_altsetting_interface = %d",
						cif_dev->new_isoc_altsetting, cif_dev->new_isoc_altsetting_interface);
				schedule_work(&bdev->work);
				/* If we don't sleep 50ms, it will failed to set alternate setting to zero */
				msleep(50);
				kfree_skb(skb);
				skb = NULL;
				return 0;
			} else if (skb->data[0] == 0x6f && skb->data[1] == 0xfc &&
					skb->data[2] == 0x09 && skb->data[3] == 0x01 &&
					skb->data[4] == 0xff && skb->data[5] == 0x05 &&
					skb->data[6] == 0x00 && skb->data[7] == 0x01) {
				BTMTK_INFO("read CR skb->data = %02x %02x %02x %02x", skb->data[8],
					skb->data[9], skb->data[10], skb->data[11]);
				crBaseAddr = (skb->data[8]<<8) + skb->data[9];
				crRegOffset = (skb->data[10]<<8) + skb->data[11];
				BTMTK_INFO("base + offset = %04x %04x", crBaseAddr, crRegOffset);
				memset(bdev->io_buf, 0, IO_BUF_SIZE);
#if BTMTK_RUNTIME_ENABLE
				ret = usb_autopm_get_interface(cif_dev->intf);
				if (ret < 0) {
					BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
				}
#endif
				ret = usb_control_msg(cif_dev->udev, usb_rcvctrlpipe(cif_dev->udev, 0),
						1, 0xDE, crBaseAddr, crRegOffset,
						bdev->io_buf, 4, USB_CTRL_IO_TIMO);
				if (ret < 0)
					BTMTK_ERR("read CR(%04X[%04X]) FAILED", crBaseAddr, crRegOffset);
				else
					BTMTK_INFO("read CR(%04X[%04X]) value = 0x%02x%02x%02x%02x",
						crBaseAddr, crRegOffset,
						bdev->io_buf[3], bdev->io_buf[2],
						bdev->io_buf[1], bdev->io_buf[0]);
				kfree_skb(skb);
				skb = NULL;
#if BTMTK_RUNTIME_ENABLE
				usb_autopm_put_interface(cif_dev->intf);
#endif
				return 0;
			} else if (skb->data[0] == 0x6f && skb->data[1] == 0xfc &&
					skb->data[2] == 0x0D && skb->data[3] == 0x01 &&
					skb->data[4] == 0xff && skb->data[5] == 0x09 &&
					skb->data[6] == 0x00 && skb->data[7] == 0x02) {
				crBaseAddr = (skb->data[8] << 8) + skb->data[9];
				crRegOffset = (skb->data[10] << 8) + skb->data[11];
				BTMTK_INFO("base + offset = %04x %04x", crBaseAddr, crRegOffset);
				memset(cif_dev->o_usb_buf, 0, HCI_MAX_COMMAND_SIZE);
				cif_dev->o_usb_buf[0] = skb->data[12];
				cif_dev->o_usb_buf[1] = skb->data[13];
				cif_dev->o_usb_buf[2] = skb->data[14];
				cif_dev->o_usb_buf[3] = skb->data[15];
#if BTMTK_RUNTIME_ENABLE
				ret = usb_autopm_get_interface(cif_dev->intf);
				if (ret < 0) {
					BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
				}
#endif
				ret = usb_control_msg(cif_dev->udev, usb_sndctrlpipe(cif_dev->udev, 0),
						2, 0x5E, crBaseAddr, crRegOffset,
						cif_dev->o_usb_buf, 4, USB_CTRL_IO_TIMO);
#if BTMTK_RUNTIME_ENABLE
				usb_autopm_put_interface(cif_dev->intf);
#endif
				if (ret < 0)
					BTMTK_ERR("write CR(%04X[%04X]) FAILED", crBaseAddr, crRegOffset);
				else
					BTMTK_INFO("write CR(%04X[%04X]) value = 0x%02x%02x%02x%02x",
						crBaseAddr, crRegOffset,
						cif_dev->o_usb_buf[3], cif_dev->o_usb_buf[2],
						cif_dev->o_usb_buf[1], cif_dev->o_usb_buf[0]);
				kfree_skb(skb);
				skb = NULL;
				return 0;
			}
		}

		/* For wmt cmd/evt */
		if (!memcmp(skb->data, &bmain_info->wmt_over_hci_header[1], WMT_OVER_HCI_HEADER_SIZE - 1)) {
			skb_push(skb, 1);
			skb->data[0] = MTK_HCI_COMMAND_PKT;
			BTMTK_DBG_RAW(skb->data, skb->len, "%s, 6ffc send_frame", __func__);
			/* No event for wmt trigger assert command */
			if (memcmp(skb->data, wmt_trigger_assert.content, wmt_trigger_assert.len)) {
				ret = btmtk_usb_send_cmd(bdev, skb, WMT_DELAY_TIMES, RETRY_TIMES,
					BTMTK_TX_CMD_FROM_DRV, flag);
				if (ret < 0) {
					BTMTK_ERR("%s: send wmt cmd failed", __func__);
					return ret;
				}
				set_bit(BTUSB_WMT_RUNNING, &bdev->flags);
				ret = btusb_submit_wmt_urb(hdev, GFP_KERNEL);
			} else {
				BTMTK_INFO("%s: Trigger FW assert by WMT command", __func__);
				ret = btmtk_usb_send_cmd(bdev, skb, WMT_DELAY_TIMES, RETRY_TIMES,
					BTMTK_TX_CMD_FROM_DRV, flag);
			}
			return ret;
		}

		if (BTMTK_IS_BT_0_INTF(ifnum_base)) {
			if ((is_mt6639(bdev->chip_id) || is_mt7902(bdev->chip_id)
					|| is_mt7922(bdev->chip_id) || is_mt7961(bdev->chip_id)
					|| is_mt7925(bdev->chip_id)) &&
					cif_dev->bulk_cmd_tx_ep)
				urb = alloc_bulk_cmd_urb(hdev, skb);
			else
				urb = alloc_ctrl_urb(hdev, skb);
		} else if (BTMTK_IS_BT_1_INTF(ifnum_base)) {
			if (is_mt7961(bdev->chip_id) || is_mt7925(bdev->chip_id)) {
				if (cif_dev->bulk_cmd_tx_ep) {
					UNUSED(alloc_ctrl_bgf1_urb);
					urb = alloc_bulk_cmd_urb(hdev, skb);
				} else
					urb = alloc_ctrl_bgf1_urb(hdev, skb);
			} else {
				BTMTK_ERR("%s: chip_id(%d) is invalid", __func__, bdev->chip_id);
				return -ENODEV;
			}
		} else {
			BTMTK_ERR("%s: ifnum_base(%d) is invalid", __func__, ifnum_base);
			return -ENODEV;
		}

		if (IS_ERR(urb))
			return PTR_ERR(urb);

		hdev->stat.cmd_tx++;
		return submit_or_queue_tx_urb(hdev, urb, HCI_COMMAND_PKT);

	case HCI_ACLDATA_PKT:
		if (skb->data[0] == 0x00 && skb->data[1] == 0x44) {
			if (cif_dev->iso_channel && bdev->iso_threshold) {
				int isoc_pkt_len = 0;
				int isoc_pkt_padding = 0;

				skb_pull(skb, 4);
				isoc_pkt_len = skb->data[2] + (skb->data[3] << 8) + HCI_ISO_PKT_HEADER_SIZE;
				isoc_pkt_padding = bdev->iso_threshold - isoc_pkt_len;

				if (skb_tailroom(skb) >= isoc_pkt_padding) {
					skb_tmp = skb_put(skb, isoc_pkt_padding);
					if (!skb_tmp) {
						BTMTK_ERR("%s, skb_put failed!", __func__);
						kfree_skb(skb);
						return -ENOMEM;
					}
					memset(skb_tmp, 0, isoc_pkt_padding);
					urb = alloc_intr_iso_urb(hdev, skb);
					BTMTK_DBG_RAW(skb->data, skb->len, "%s, it's ble iso packet",
						__func__);
					if (IS_ERR(urb))
						return PTR_ERR(urb);
				} else {
					/* hci driver alllocate the size of skb that is to small, need re-allocate */
					iso_skb = alloc_skb(HCI_MAX_ISO_SIZE + BT_SKB_RESERVE, GFP_ATOMIC);
					if (!iso_skb) {
						BTMTK_ERR("%s allocate skb failed!!", __func__);
						return -ENOMEM;
					}
					/* copy skb data into iso_skb */
					skb_copy_bits(skb, 0, skb_put(iso_skb, skb->len), skb->len);
					if (skb_tailroom(iso_skb) >= isoc_pkt_padding) {
						skb_tmp = skb_put(iso_skb, isoc_pkt_padding);
						if (!skb_tmp) {
							BTMTK_ERR("%s, skb_put failed!", __func__);
							kfree_skb(iso_skb);
							kfree_skb(skb);
							return -ENOMEM;
						}
						memset(skb_tmp, 0, isoc_pkt_padding);

						/* After call back, bt drive will free iso_skb */
						urb = alloc_intr_iso_urb(hdev, iso_skb);
						BTMTK_DBG_RAW(iso_skb->data, iso_skb->len, "%s, it's ble iso packet",
							__func__);

						if (IS_ERR(urb)) {
							kfree_skb(iso_skb);
							return PTR_ERR(urb);
						}

						hdev->stat.acl_tx++;
						ret = submit_or_queue_tx_urb(hdev, urb, HCI_ISO_PKT);
						if (ret < 0) {
							/* when ret < 0, skb will be free in hci_send_frame,
							 * but need to free iso_skb, because iso_skb alloc in bt driver
							 */
							kfree_skb(iso_skb);
							goto exit;
						}

						/* It's alloc by hci drver, bt driver must free it when ret >=0. */
						kfree_skb(skb);
exit:
						return ret;
					} else {
						BTMTK_INFO("%s: the size of skb is too small!", __func__);
						return -ENOMEM;
					}
				}
			} else {
				BTMTK_WARN("btusb_send_frame send iso data, but iso channel not exit, %d",
						bdev->iso_threshold);
				/* if iso channel not exist, we need to drop iso data then free the skb */
				kfree_skb(skb);
				return 0;
			}
		} else {
			urb = alloc_bulk_urb(hdev, skb);
			if (IS_ERR(urb))
				return PTR_ERR(urb);
		}
		hdev->stat.acl_tx++;
		return submit_or_queue_tx_urb(hdev, urb, HCI_ACLDATA_PKT);

	case HCI_SCODATA_PKT:
		if (hci_conn_num(hdev, SCO_LINK) < 1) {
			BTMTK_INFO("btusb_send_frame hci_conn sco link = %d", hci_conn_num(hdev, SCO_LINK));
#if CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT
			return -ENODEV;
#else
			/* We need to study how to solve this in dvt case.*/
#endif
		}

		urb = alloc_isoc_urb(hdev, skb);
		if (IS_ERR(urb))
			return PTR_ERR(urb);

		hdev->stat.sco_tx++;
		return submit_tx_urb(hdev, urb, HCI_SCODATA_PKT);
	}

	return -EILSEQ;
}

static int btmtk_usb_load_fw_patch_using_dma(struct btmtk_dev *bdev, u8 *image,
		u8 *fwbuf, int section_dl_size, int section_offset, int patch_flag)
{
	int cur_len = 0;
	int ret = 0;
	u32 value = 0;
	s32 sent_len;
	int retry = WMT_DELAY_TIMES;
	int delay = 0;
	u32 dma_done_cr_r, dma_done_cr_w;
	u32 reg_value = 0;
	u32 dma_done_value = 0;
	struct data_struct cmd = {0, NULL}, event = {0, NULL};

	if (bdev == NULL || image == NULL || fwbuf == NULL) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		ret = -1;
		goto exit;
	}

	BTMTK_INFO("%s: loading rom patch... start", __func__);
	while (1) {
		/* MT7925 DMA download needs 4 bytes align */
		if (is_mt7925(bdev->chip_id))
			sent_len = (section_dl_size - cur_len) >= (UPLOAD_PATCH_UNIT - 4) ?
					(UPLOAD_PATCH_UNIT - 4) : (section_dl_size - cur_len);
		else
			sent_len = (section_dl_size - cur_len) >= (UPLOAD_PATCH_UNIT - HCI_TYPE_SIZE) ?
					(UPLOAD_PATCH_UNIT - HCI_TYPE_SIZE) : (section_dl_size - cur_len);

		if (sent_len > 0) {
			/* btmtk_cif_send_bulk_out will send from image[1],
			 * image[0] will be ingored
			 */
			image[0] = HCI_ACLDATA_PKT;
			memcpy(&image[HCI_TYPE_SIZE], fwbuf + section_offset + cur_len, sent_len);
			BTMTK_DBG("%s: sent_len = %d, cur_len = %d", __func__,
					sent_len, cur_len);
			ret = btmtk_main_send_cmd(bdev,
					image, sent_len + HCI_TYPE_SIZE,
					NULL, -1,
					0, 0, BTMTK_TX_ACL_FROM_DRV, CMD_NO_NEED_FILTER);
			if (ret < 0) {
				BTMTK_ERR("%s: send patch failed, terminate", __func__);
				goto exit;
			}
			cur_len += sent_len;
		} else
			break;
	}

	if (is_mt6639(bdev->chip_id) || is_mt7925(bdev->chip_id)) {
		delay = PATCH_DOWNLOAD_PHASE3_SECURE_BOOT_DELAY_TIME;
		reg_value = BT_GDMA_DONE_6639_VALUE_W;
	} else if (is_mt7922(bdev->chip_id) || is_mt7902(bdev->chip_id)) {
		delay = PATCH_DOWNLOAD_PHASE3_SECURE_BOOT_DELAY_TIME;
		reg_value = BT_GDMA_DONE_7922_VALUE_W;
	} else {
		delay = DELAY_TIMES;
		reg_value = BT_GDMA_DONE_7921_VALUE_W;
	}

	if (is_mt6639(bdev->chip_id) || is_mt7925(bdev->chip_id)) {
		dma_done_cr_w = BT_GDMA_DONE_6639_ADDR_W;
		dma_done_cr_r = BT_GDMA_DONE_6639_ADDR_R;
		dma_done_value = BT_GDMA_DONE_6639_VALUE_R;
	} else {
		dma_done_cr_w = BT_GDMA_DONE_ADDR_W;
		dma_done_cr_r = BT_GDMA_DONE_ADDR_R;
		dma_done_value = BT_GDMA_DONE_VALUE_R;
	}

	/* Poll the register until dma dl is completed */
	if (is_mt6639(bdev->chip_id) || is_mt7961(bdev->chip_id) || is_mt7922(bdev->chip_id) || is_mt7925(bdev->chip_id)) {
		do {
			btmtk_cif_write_uhw_register(bdev, dma_done_cr_w, reg_value);
			btmtk_cif_read_uhw_register(bdev, dma_done_cr_r, &value);
			if ((value & dma_done_value) == value)
				break;
			msleep(DELAY_TIMES);
		} while (retry-- > 0);

		if ((value & dma_done_value) != value) {
			BTMTK_INFO("%s: DL Failed cr=%08X", __func__, value);
			ret = -1;
			btmtk_send_assert_cmd(bdev);
			goto exit;
		}
	}

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, LD_PATCH_CMD_USB, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, LD_PATCH_EVT, event);
	if ((is_mt6639(bdev->chip_id) || is_mt7925(bdev->chip_id)) && patch_flag == WIFI_DOWNLOAD) {
		event.content[4] = 0x54; /* the byte will be modified to 0x54 when download Connac3 wifi */
		cmd.content[5] = 0x54;
	} else {
		event.content[4] = 0x01; /* the byte will be modified to 0x01 when download BT */
		cmd.content[5] = 0x01;
	}

	ret = btmtk_main_send_cmd(bdev,
		cmd.content, cmd.len,
		event.content, event.len,
		delay, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: send wmd dl cmd failed, terminate!", __func__);
	BTMTK_INFO("%s: loading rom patch... Done", __func__);

exit:
	return ret;
}

static void btusb_notify(struct hci_dev *hdev, unsigned int evt)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);

	BTMTK_DBG("evt %d", evt);

	if (hci_conn_num(hdev, SCO_LINK) != bdev->sco_num) {
		bdev->sco_num = hci_conn_num(hdev, SCO_LINK);
		schedule_work(&bdev->work);
	}
}

static inline int __set_isoc_interface(struct hci_dev *hdev, int altsetting)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	struct usb_interface *intf = cif_dev->isoc;
	struct usb_endpoint_descriptor *ep_desc;
	int i, err;
	unsigned int ifnum_base;

	if (!cif_dev->isoc)
		return -ENODEV;

	ifnum_base = cif_dev->intf->cur_altsetting->desc.bInterfaceNumber;
	if (BTMTK_IS_BT_0_INTF(ifnum_base))
		cif_dev->new_isoc_altsetting_interface = 1;
	else if (BTMTK_IS_BT_1_INTF(ifnum_base))
		cif_dev->new_isoc_altsetting_interface = 4;
	err = usb_set_interface(cif_dev->udev, cif_dev->new_isoc_altsetting_interface, altsetting);
	BTMTK_DBG("setting interface alt = %d, interface = %d", altsetting, cif_dev->new_isoc_altsetting_interface);

	if (err < 0) {
		BTMTK_ERR("setting interface failed (%d)", -err);
		return err;
	}

	bdev->isoc_altsetting = altsetting;

	cif_dev->isoc_tx_ep = NULL;
	cif_dev->isoc_rx_ep = NULL;

	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep_desc = &intf->cur_altsetting->endpoint[i].desc;

		if (!cif_dev->isoc_tx_ep && usb_endpoint_is_isoc_out(ep_desc)) {
			cif_dev->isoc_tx_ep = ep_desc;
			continue;
		}

		if (!cif_dev->isoc_rx_ep && usb_endpoint_is_isoc_in(ep_desc)) {
			cif_dev->isoc_rx_ep = ep_desc;
			continue;
		}
	}

	if (!cif_dev->isoc_tx_ep || !cif_dev->isoc_rx_ep) {
		BTMTK_ERR("invalid SCO descriptors");
		return -ENODEV;
	}

	return 0;
}

static void btusb_work(struct work_struct *work)
{
	struct btmtk_dev *bdev = container_of(work, struct btmtk_dev, work);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	struct hci_dev *hdev = bdev->hdev;
	int new_alts;
	int err;
	unsigned long flags;

	if (bdev->sco_num > 0) {
		if (!test_bit(BTUSB_DID_ISO_RESUME, &bdev->flags)) {
			err = usb_autopm_get_interface(cif_dev->isoc ? cif_dev->isoc : cif_dev->intf);
			if (err < 0) {
				clear_bit(BTUSB_ISOC_RUNNING, &bdev->flags);
				usb_kill_anchored_urbs(&cif_dev->isoc_anchor);
				return;
			}

			set_bit(BTUSB_DID_ISO_RESUME, &bdev->flags);
		}

#if CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT
		new_alts = cif_dev->new_isoc_altsetting;
#else
		if (hdev->voice_setting & 0x0020) {
			static const int alts[3] = { 2, 4, 5 };

			new_alts = alts[bdev->sco_num - 1];
		} else {
			new_alts = bdev->sco_num;
		}
#endif /* CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT */

		clear_bit(BTUSB_ISOC_RUNNING, &bdev->flags);
		usb_kill_anchored_urbs(&cif_dev->isoc_anchor);

		/* When isochronous alternate setting needs to be
		 * changed, because SCO connection has been added
		 * or removed, a packet fragment may be left in the
		 * reassembling state. This could lead to wrongly
		 * assembled fragments.
		 *
		 * Clear outstanding fragment when selecting a new
		 * alternate setting.
		 */
		spin_lock_irqsave(&bdev->rxlock, flags);
		kfree_skb(bdev->sco_skb);
		bdev->sco_skb = NULL;
		spin_unlock_irqrestore(&bdev->rxlock, flags);

		if (__set_isoc_interface(hdev, new_alts) < 0)
			return;

		if (!test_and_set_bit(BTUSB_ISOC_RUNNING, &bdev->flags)) {
			if (btusb_submit_isoc_urb(hdev, GFP_KERNEL) < 0)
				clear_bit(BTUSB_ISOC_RUNNING, &bdev->flags);
			else
				btusb_submit_isoc_urb(hdev, GFP_KERNEL);
		}
	} else {
		clear_bit(BTUSB_ISOC_RUNNING, &bdev->flags);
		usb_kill_anchored_urbs(&cif_dev->isoc_anchor);
		BTMTK_INFO("%s set alt to zero", __func__);
		__set_isoc_interface(hdev, 0);
		if (test_and_clear_bit(BTUSB_DID_ISO_RESUME, &bdev->flags))
			usb_autopm_put_interface(cif_dev->isoc ? cif_dev->isoc : cif_dev->intf);
	}
}

static void btusb_waker(struct work_struct *work)
{
	struct btmtk_dev *bdev = container_of(work, struct btmtk_dev, waker);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	int err;

	err = usb_autopm_get_interface(cif_dev->intf);
	if (err < 0)
		return;

	usb_autopm_put_interface(cif_dev->intf);
}

int btmtk_usb_toggle_rst_pin(struct btmtk_dev *bdev)
{
#ifndef CFG_SUPPORT_CHIP_RESET_KO
	struct device_node *node;
	int rst_pin_num = 0;
	int status = 0;
#endif

	if (!bdev) {
		BTMTK_WARN("%s: bdev is NULL!", __func__);
		return -1;
	}
	if (bdev->bt_cfg.dongle_reset_gpio_pin == -1) {
		BTMTK_WARN("%s: bt driver is not ready, please don't call chip reset!", __func__);
		return -1;
	}

	BTMTK_INFO("%s: begin", __func__);
#ifndef CFG_SUPPORT_CHIP_RESET_KO
	/* Initialize the interface specific function pointers */
#if (CFG_GKI_SUPPORT == 0)
	reset_func.pf_pdwndFunc = (pdwnc_func) btmtk_kallsyms_lookup_name("PDWNC_SetBTInResetState");
	if (reset_func.pf_pdwndFunc)
		BTMTK_INFO("%s: Found PDWNC_SetBTInResetState", __func__);
	else
		BTMTK_WARN("%s: No Exported Func Found PDWNC_SetBTInResetState", __func__);

	reset_func.pf_resetFunc2 = (reset_func_ptr2) btmtk_kallsyms_lookup_name("mtk_gpio_set_value");
	if (!reset_func.pf_resetFunc2)
		BTMTK_ERR("%s: No Exported Func Found mtk_gpio_set_value", __func__);
	else
		BTMTK_INFO("%s: Found mtk_gpio_set_value", __func__);

	reset_func.pf_lowFunc = (set_gpio_low) btmtk_kallsyms_lookup_name("MDrv_GPIO_Set_Low");
	reset_func.pf_highFunc = (set_gpio_high) btmtk_kallsyms_lookup_name("MDrv_GPIO_Set_High");
	if (!reset_func.pf_lowFunc || !reset_func.pf_highFunc)
		BTMTK_WARN("%s: No Exported Func Found MDrv_GPIO_Set_Low or High", __func__);
	else
		BTMTK_INFO("%s: Found MDrv_GPIO_Set_Low & MDrv_GPIO_Set_High", __func__);

	if (reset_func.pf_pdwndFunc) {
		BTMTK_INFO("%s: Invoke PDWNC_SetBTInResetState(%d)", __func__, 1);
		reset_func.pf_pdwndFunc(1);
	} else
		BTMTK_INFO("%s: No Exported Func Found PDWNC_SetBTInResetState", __func__);

	if (reset_func.pf_resetFunc2) {
		rst_pin_num = bdev->bt_cfg.dongle_reset_gpio_pin;
		BTMTK_INFO("%s: Invoke bdev->pf_resetFunc2(%d,%d)", __func__, rst_pin_num, 0);
		reset_func.pf_resetFunc2(rst_pin_num, 0);
		msleep(RESET_PIN_SET_LOW_TIME);
		BTMTK_INFO("%s: Invoke bdev->pf_resetFunc2(%d,%d)", __func__, rst_pin_num, 1);
		reset_func.pf_resetFunc2(rst_pin_num, 1);
		goto exit;
	}

	node = of_find_compatible_node(NULL, NULL, "mstar,gpio-wifi-ctl");
	if (node) {
		if (of_property_read_u32(node, "wifi-ctl-gpio", &rst_pin_num) == 0) {
			if (reset_func.pf_lowFunc && reset_func.pf_highFunc) {
				BTMTK_INFO("%s: Invoke bdev->pf_lowFunc(%d)", __func__, rst_pin_num);
				reset_func.pf_lowFunc(rst_pin_num);
				msleep(RESET_PIN_SET_LOW_TIME);
				BTMTK_INFO("%s: Invoke bdev->pf_highFunc(%d)", __func__, rst_pin_num);
				reset_func.pf_highFunc(rst_pin_num);
				goto exit;
			}
		} else
			BTMTK_WARN("%s, failed to obtain wifi control gpio", __func__);
	} else {
		if (reset_func.pf_lowFunc && reset_func.pf_highFunc) {
			rst_pin_num = bdev->bt_cfg.dongle_reset_gpio_pin;
			BTMTK_INFO("%s: Invoke bdev->pf_lowFunc(%d)", __func__, rst_pin_num);
			reset_func.pf_lowFunc(rst_pin_num);
			msleep(RESET_PIN_SET_LOW_TIME);
			BTMTK_INFO("%s: Invoke bdev->pf_highFunc(%d)", __func__, rst_pin_num);
			reset_func.pf_highFunc(rst_pin_num);
			goto exit;
		}
	}
#endif
	node = of_find_compatible_node(NULL, NULL, "mediatek,mtk-wifi-reset");
	if (node) {
		status = of_get_named_gpio(node, "wifireset-gpios", 0);
		if (status >= 0) {
			rst_pin_num = status;
			status = 0;
		} else if (of_property_read_u32(node, "wifireset-gpios", &rst_pin_num) == 0) {
			status = 0;
		} else {
			status = -1;
			BTMTK_ERR("%s: failed to obtain wifi control gpio", __func__);
		}

		if (status == 0) {
			BTMTK_INFO("%s: Found wifireset-gpios: %d\n", __func__, rst_pin_num);

			status = gpio_request(rst_pin_num, "wifireset-gpios");
			if (status < 0) {
				BTMTK_ERR("%s: gpio_request failed(%d)", __func__, status);
				return -1;
			}

			status = gpio_direction_output(rst_pin_num, 0);
			if (status < 0) {
				BTMTK_ERR("%s: gpio_direction_output %d failed", __func__, 0);
				gpio_free(rst_pin_num);
				return -1;
			} else {
				BTMTK_INFO("%s: gpio_direction_output(%d,%d)", __func__, rst_pin_num, 0);
			}

			msleep(RESET_PIN_SET_LOW_TIME);
			status = gpio_direction_output(rst_pin_num, 1);
			if (status < 0) {
				BTMTK_ERR("%s: gpio_direction_output %d failed", __func__, 1);
				gpio_free(rst_pin_num);
				return -1;
			} else {
				BTMTK_INFO("%s: gpio_direction_output(%d,%d)", __func__, rst_pin_num, 1);
			}

			BTMTK_INFO("%s: set gpio done", __func__);
			gpio_free(rst_pin_num);
			goto exit;
		}
	} else {
		BTMTK_ERR("%s: Not found node mediatek,mtk-wifi-reset", __func__);
	}
#endif
	/* use linux kernel common api */
	do {
		struct device_node *node;
		int mt76xx_reset_gpio = bdev->bt_cfg.dongle_reset_gpio_pin;

		node = of_find_compatible_node(NULL, NULL, "mediatek,connectivity-combo");
		if (node) {
			mt76xx_reset_gpio = of_get_named_gpio(node, "mt76xx-reset-gpio", 0);
			if (gpio_is_valid(mt76xx_reset_gpio))
				BTMTK_INFO("%s: Get chip reset gpio(%d)", __func__, mt76xx_reset_gpio);
			else
				mt76xx_reset_gpio = bdev->bt_cfg.dongle_reset_gpio_pin;
		}

		BTMTK_INFO("%s: Invoke Low(%d)", __func__, mt76xx_reset_gpio);
		gpio_direction_output(mt76xx_reset_gpio, 0);
		msleep(RESET_PIN_SET_LOW_TIME);
		BTMTK_INFO("%s: Invoke High(%d)", __func__, mt76xx_reset_gpio);
		gpio_direction_output(mt76xx_reset_gpio, 1);
		goto exit;
	} while (0);

exit:
	BTMTK_INFO("%s: end", __func__);
	return 0;
}
EXPORT_SYMBOL(btmtk_usb_toggle_rst_pin);

static int btmtk_usb_subsys_reset(struct btmtk_dev *bdev)
{
	int ret = 0;
	int state = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	struct device_node *node;
	int status;
	uint32_t gpio_num = 0;
	int val = 0;


	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL, return", __func__);
		return -1;
	}

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_DISCONNECT) {
		BTMTK_INFO("%s:usb disconnect is doing, return", __func__);
		return -1;
	}

	cancel_work_sync(&bdev->work);
	cancel_work_sync(&bdev->waker);

	clear_bit(BTUSB_BLE_ISOC_RUNNING, &bdev->flags);
	clear_bit(BTUSB_ISOC_RUNNING, &bdev->flags);
	clear_bit(BTUSB_BULK_RUNNING, &bdev->flags);
	clear_bit(BTUSB_INTR_RUNNING, &bdev->flags);
	clear_bit(BTUSB_WMT_RUNNING, &bdev->flags);
	clear_bit(BTUSB_SUSPENDING, &bdev->flags);
	clear_bit(BTUSB_OPENED, &bdev->flags);
	bdev->sco_num = 0;

	btusb_stop_traffic((struct btmtk_usb_dev *)bdev->cif_dev);

	if (bdev->bt_cfg.support_dts_subsys_rst) {
		/* For reset */
		btmtk_cif_write_uhw_register(bdev, EP_RST_OPT, EP_RST_IN_OUT_OPT);

		/* read interrupt EP15 CR */
		btmtk_cif_read_uhw_register(bdev, BT_WDT_STATUS, &val);

		node = of_find_compatible_node(NULL, NULL, CHIP_RESET_DTS_NODE_NAME);
		if (node) {
			status = of_get_named_gpio(node, CHIP_RESET_GPIO_PROPERTY_NAME, 0);
			if (status >= 0) {
				gpio_num = status;
				status = 0;
			} else if (of_property_read_u32(node, CHIP_RESET_GPIO_PROPERTY_NAME, &gpio_num) == 0) {
				status = 0;
			} else {
				status = -1;
				BTMTK_ERR("%s: failed to get gpio number", __func__);
			}
		} else {
			status = -1;
			BTMTK_ERR("%s: failed to find DTS node", __func__);
		}

		if (status != 0) {
			return -1;
		} else {
			BTMTK_INFO("%s: gpio num for subsys reset is %d", __func__, gpio_num);
		}

		status = gpio_request(gpio_num, CHIP_RESET_GPIO_PROPERTY_NAME);
		if (status < 0) {
			BTMTK_ERR("%s: gpio_request failed(%d)", __func__, status);
			return -1;
		}
		status = gpio_direction_output(gpio_num, 0);
		if (status < 0) {
			BTMTK_ERR("%s: gpio_direction_output %d failed", __func__, 0);
			gpio_free(gpio_num);
			return -1;
		} else {
			BTMTK_INFO("%s: gpio_direction_output(%d,%d)",
				   __func__, gpio_num, 0);
		}

		msleep(100);
		status = gpio_direction_output(gpio_num, 1);
		if (status < 0) {
			BTMTK_ERR("%s: gpio_direction_output %d failed", __func__, 1);
			gpio_free(gpio_num);
			return -1;
		} else {
			BTMTK_INFO("%s: gpio_direction_output(%d,%d)",
				   __func__, gpio_num, 1);
		}

		BTMTK_INFO("%s: set gpio done", __func__);
		gpio_free(gpio_num);
		btmtk_usb_open(bdev->hdev);
	} else {
		if (bmain_info->hif_hook_chip.bt_subsys_reset) {
			ret = bmain_info->hif_hook_chip.bt_subsys_reset(bdev);
			btmtk_usb_open(bdev->hdev);
			return ret;
		}
	}

	BTMTK_ERR("%s: subsys reset no register", __func__);
	return -1;
}

static int btmtk_usb_send_shutdown_cmd(struct btmtk_dev *bdev){
	struct btmtk_usb_dev *cif_dev = NULL;
	int ret = 0;

	if (bdev == NULL)
		return -1;

	BTMTK_INFO("%s: enter", __func__);

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (cif_dev->udev == NULL) {
		BTMTK_ERR("%s: cif_dev is invalid", __func__);
		ret = -1;
		goto exit;
	}

	ret = usb_control_msg(cif_dev->udev, usb_sndctrlpipe(cif_dev->udev, 0),
			0x55, 0x40, 0x07, 0x00, NULL, 0, USB_CTRL_IO_TIMO);

	if (ret < 0) {
		BTMTK_ERR("%s: command send failed(%d)", __func__, ret);
		goto exit;
	}
exit:
	BTMTK_INFO("%s: end", __func__);
	return ret;
}

static int btmtk_usb_enter_standby(void)
{
	int ret = 0;
	int i = 0;
	int cif_event = 0;
	int state = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_usb_dev *cif_dev = NULL;
	struct btmtk_woble *bt_woble = NULL;
	int intf_num = btmtk_get_interface_num();
	struct btmtk_dev **pp_bdev = btmtk_get_pp_bdev();
	int val = 0;
	unsigned long radio_off_cr = 0;

	BTMTK_INFO("%s: enter", __func__);
	btmtk_handle_mutex_lock(bdev);

	for (i = 0; i < intf_num; i++) {
		/* Find valid dev for already probe interface. */
		if (pp_bdev[i]->hdev != NULL) {
			bdev = pp_bdev[i];
			state = btmtk_get_chip_state(bdev);
			if (state != BTMTK_STATE_WORKING) {
				BTMTK_WARN("%s: not in working(%d).", __func__, state);
				break;
			}

			cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
			bt_woble = &cif_dev->bt_woble;

			/* Retrieve current HIF event state */
			cif_event = HIF_EVENT_STANDBY;
			if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
				/* Error */
				BTMTK_WARN("%s parameter is NULL", __func__);
				btmtk_handle_mutex_unlock(bdev);
				return -ENODEV;
			}

			cif_state = &bdev->cif_state[cif_event];

			btmtk_usb_cif_mutex_lock(bdev);
			/* Set Entering state */
			btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

			/* Do HIF events */
			ret = btmtk_woble_suspend(bt_woble);

			/* If usb2.0(USB_SPEED_HIGH) not need write cr */
			if(cif_dev->udev->speed > USB_SPEED_HIGH && ret == 0){
				if (is_mt6639(bdev->chip_id) || is_mt7925(bdev->chip_id)) {
					if (is_mt6639(bdev->chip_id))
						radio_off_cr = BT_RADIO_OFF_DONE_6639;
					else if (is_mt7925(bdev->chip_id))
						radio_off_cr = BT_RADIO_OFF_DONE_7925;

					btmtk_handle_mutex_unlock(bdev);
					ret = btmtk_usb_read_register(bdev, radio_off_cr, &val);
					if(ret < 0)
						BTMTK_INFO("%s: read radio off  CR fail", __func__);
					else {
						BTMTK_INFO("%s: read radio off CR : 0x%08x", __func__, val);
						if (is_mt6639(bdev->chip_id))
							val |= (1 << 15);
						else if (is_mt7925(bdev->chip_id))
							val |= (1 << 14);

						BTMTK_INFO("%s: write 1 to radio off CR : 0x%08x", __func__, val);
						ret = btmtk_usb_write_register(bdev, radio_off_cr, val);
						if(ret < 0){
							BTMTK_INFO("%s: write radio off  CR fail", __func__);
						} else
							btmtk_usb_send_shutdown_cmd(bdev);
					}
					btmtk_handle_mutex_lock(bdev);
				}
			}

			/* Set End/Error state */
			if (ret == 0)
				btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
			else
				btmtk_set_chip_state((void *)bdev, cif_state->ops_error);

			btmtk_usb_cif_mutex_unlock(bdev);

			if (ret)
				break;
		}
	}

	BTMTK_INFO("%s: end", __func__);
	btmtk_handle_mutex_unlock(bdev);
	return ret;
}

static int btusb_set_pinmux(struct btmtk_dev *bdev)
{
	u32 value = 0;
	int err = 0;

	BTMTK_INFO("%s", __func__);
	err = btmtk_cif_read_uhw_register(bdev, BT_PINMUX_CTRL_REG, &value);
	if (err)
		return -1;

	value |= BT_SUBSYS_RST_PINMUX;
	err = btmtk_cif_write_uhw_register(bdev, BT_PINMUX_CTRL_REG, value);
	if (err)
		return -1;

	err = btmtk_cif_read_uhw_register(bdev, BT_PINMUX_CTRL_REG, &value);
	if (err)
		return -1;

	BTMTK_INFO("%s: BT_PINMUX_CTRL_REG = 0x%x", __func__, value);

	err = btmtk_cif_read_uhw_register(bdev, BT_SUBSYS_RST_REG, &value);
	if (err)
		return -1;

	value |= BT_SUBSYS_RST_ENABLE;
	err = btmtk_cif_write_uhw_register(bdev, BT_SUBSYS_RST_REG, value);
	if (err)
		return -1;

	err = btmtk_cif_read_uhw_register(bdev, BT_SUBSYS_RST_REG, &value);
	if (err)
		return -1;

	BTMTK_INFO("%s: BT_SUBSYS_RST_REG = 0x%x", __func__, value);

	return 0;
}

#if ((CFG_SUPPORT_HW_DVT == 0) || CFG_SUPPORT_BLUEZ)
static int btmtk_probe_retry(struct btmtk_dev *bdev)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	int ret = 0;

	bmain_info->chip_reset_flag = 0;
	set_bit(BT_PROBE_FAIL_FOR_SUBSYS_RESET, &bdev->flags);
	btmtk_handle_mutex_unlock(bdev);

	btmtk_reset_trigger(bdev);
	ret = wait_event_interruptible_timeout(bdev->probe_fail_wq, test_bit(BT_PROBE_RESET_DONE, &bdev->flags),
			msecs_to_jiffies(20000));
	clear_bit(BT_PROBE_FAIL_FOR_SUBSYS_RESET, &bdev->flags);
	btmtk_handle_mutex_lock(bdev);

	if (ret > 0) {
		BTMTK_INFO("%s: end", __func__);
		clear_bit(BT_PROBE_RESET_DONE, &bdev->flags);
		return 0;
	} else {
		BTMTK_ERR("%s: timeout", __func__);
		if (test_bit(BT_PROBE_RESET_DONE, &bdev->flags)) {
			clear_bit(BT_PROBE_RESET_DONE,&bdev->flags);
			return 0;
		} else {
			BTMTK_ERR("%s: chip reset timeout", __func__);
			return -1;
		}
	}
}
#endif

static int btusb_probe(struct usb_interface *intf,
		       const struct usb_device_id *id)
{
	struct usb_endpoint_descriptor *ep_desc;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_usb_dev *cif_dev = NULL;
	unsigned int ifnum_base;
	int i, err = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
#if (CFG_SUPPORT_HW_DVT == 0)
	u8 load_patch_fail = 0;
#endif
#if CFG_SUPPORT_BLUEZ
	u8 send_init_fail = 0;
#endif
	u8 cap_init_fail = 0;
#if ((CFG_SUPPORT_HW_DVT == 0) || CFG_SUPPORT_BLUEZ)
	int retry = 4;
#endif

	ifnum_base = intf->cur_altsetting->desc.bInterfaceNumber;
	BTMTK_INFO("intf %p id %p, interfacenum = %d", intf, id, ifnum_base);
	bmain_info->chip_reset_flag = 0;

	bdev = usb_get_intfdata(intf);
	if (!bdev) {
		BTMTK_ERR("[ERR] bdev is NULL");
		goto end;
	}
	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;

	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep_desc = &intf->cur_altsetting->endpoint[i].desc;

		/* reset_intr_ep must be initialized before intr_ep,
		 * otherwise its address may be the intr_ep address
		 */
		if (!cif_dev->reset_intr_ep && ep_desc->bEndpointAddress == 0x8f &&
			usb_endpoint_is_int_in(ep_desc)) {
			BTMTK_INFO("intr_reset_rx__ep i = %d  Endpoints 0x%02X, number_of_endpoints=%d",
				i, ep_desc->bEndpointAddress, intf->cur_altsetting->desc.bNumEndpoints);
			cif_dev->reset_intr_ep = ep_desc;
			continue;
		}

		/* bulk_cmd_tx_ep must be initialized before bulk_tx_ep,
		 * otherwise its address will be the bulk_tx_ep address
		 */
		if (!cif_dev->bulk_cmd_tx_ep && usb_endpoint_is_bulk_out(ep_desc) &&
			(ep_desc->bEndpointAddress == 0x01 || ep_desc->bEndpointAddress == 0x0b)) {
			cif_dev->bulk_cmd_tx_ep = ep_desc;
			BTMTK_INFO(" bulk_cmd_tx_ep i = %d  Endpoints 0x%02X, number_of_endpoints=%d",
				i, ep_desc->bEndpointAddress, intf->cur_altsetting->desc.bNumEndpoints);
			continue;
		}

		if (!cif_dev->intr_ep && usb_endpoint_is_int_in(ep_desc)) {
			cif_dev->intr_ep = ep_desc;
			BTMTK_INFO("intr_rx_ep i = %d  Endpoints 0x%02X, number_of_endpoints=%d",
				i, ep_desc->bEndpointAddress, intf->cur_altsetting->desc.bNumEndpoints);
			continue;
		}

		if (!cif_dev->bulk_tx_ep && usb_endpoint_is_bulk_out(ep_desc)) {
			cif_dev->bulk_tx_ep = ep_desc;
			BTMTK_INFO("bulk_tx_ep i = %d  Endpoints 0x%02X, number_of_endpoints=%d",
				i, ep_desc->bEndpointAddress, intf->cur_altsetting->desc.bNumEndpoints);
			continue;
		}

		if (!cif_dev->bulk_rx_ep && usb_endpoint_is_bulk_in(ep_desc)) {
			cif_dev->bulk_rx_ep = ep_desc;
			BTMTK_INFO("bulk_rx_ep i = %d  Endpoints 0x%02X, number_of_endpoints=%d",
				i, ep_desc->bEndpointAddress, intf->cur_altsetting->desc.bNumEndpoints);
			continue;
		}
	}

	if (!cif_dev->intr_ep || !cif_dev->bulk_tx_ep || !cif_dev->bulk_rx_ep) {
		BTMTK_ERR("[ERR] intr_ep or bulk_tx_ep or bulk_rx_ep is NULL");
		goto end;
	}

	cif_dev->cmdreq_type = USB_TYPE_CLASS;
	cif_dev->cmdreq = 0x00;


	cif_dev->udev = interface_to_usbdev(intf);
	cif_dev->intf = intf;
	bdev->intf_dev = &cif_dev->udev->dev;

	INIT_WORK(&bdev->work, btusb_work);
	INIT_WORK(&bdev->waker, btusb_waker);
	/* it's for L0/L0.5 reset */
	INIT_WORK(&bdev->reset_waker, btmtk_reset_waker);
	init_usb_anchor(&cif_dev->tx_anchor);
	spin_lock_init(&bdev->txlock);

	init_usb_anchor(&cif_dev->intr_anchor);
	init_usb_anchor(&cif_dev->bulk_anchor);
	init_usb_anchor(&cif_dev->isoc_anchor);
	init_usb_anchor(&cif_dev->ctrl_anchor);
	init_usb_anchor(&cif_dev->ble_isoc_anchor);
	spin_lock_init(&bdev->rxlock);

	init_waitqueue_head(&bdev->p_woble_fail_q);
	init_waitqueue_head(&bdev->probe_fail_wq);
	init_waitqueue_head(&bdev->compare_event_wq);

	err = btmtk_cif_allocate_memory(cif_dev);
	if (err < 0) {
		BTMTK_ERR("[ERR] btmtk_cif_allocate_memory failed!");
		goto end;
	}

	err = btmtk_main_cif_initialize(bdev, HCI_USB);
	if (err < 0) {
		if (err == -EIO) {
			BTMTK_ERR("[ERR] btmtk_main_cif_initialize failed, do chip reset!!!");
			cap_init_fail = 1;
			goto end;
		} else {
			BTMTK_ERR("[ERR] btmtk_main_cif_initialize failed!");
			goto free_mem;
		}
	}

	/* only usb interface need this callback to allocate isoc trx endpoint
	 * There is no need for other interface such as sdio to use this function
	 */
	bdev->hdev->notify = btusb_notify;

	SET_HCIDEV_DEV(bdev->hdev, &cif_dev->intf->dev);

#if CFG_SUPPORT_HW_DVT
	/* We don't need to download patch during bring-up stage. */
	BTMTK_INFO("SKIP downlaod patch");
#else
	if (BTMTK_IS_BT_0_INTF(ifnum_base)) {
		err = btmtk_load_rom_patch(bdev);
		if (err < 0) {
			BTMTK_ERR("btmtk load rom patch failed, do chip reset!!!");
			if (retry > 2) {
				load_patch_fail = 1;
				retry--;
			}

			goto end;
		}
	} else
		BTMTK_INFO("interface = %d, don't download patch", ifnum_base);
#endif /* CFG_SUPPORT_HW_DVT */

#if (CFG_SUPPORT_HW_DVT == 0)
LOAD_PATCH_RETRY:
#endif

	/* For reset */
	btmtk_cif_write_uhw_register(bdev, EP_RST_OPT, 0x00010001);

	if (bdev->bt_cfg.support_dts_subsys_rst) {
		err = btusb_set_pinmux(bdev);
		if (err < 0) {
			BTMTK_ERR("Set pinmux fail!");
		}
	}

	/* Interface numbers are hardcoded in the specification */
	if (BTMTK_IS_BT_0_INTF(ifnum_base)) {
		cif_dev->isoc = usb_ifnum_to_if(cif_dev->udev, 1);

		BTMTK_INFO("set interface number 2 for iso ");
		cif_dev->iso_channel = usb_ifnum_to_if(cif_dev->udev, 2);
		usb_set_interface(cif_dev->udev, 2, 1);
		if (cif_dev->iso_channel) {
			for (i = 0; i < cif_dev->iso_channel->cur_altsetting->desc.bNumEndpoints; i++) {
				ep_desc = &cif_dev->iso_channel->cur_altsetting->endpoint[i].desc;

				if (!cif_dev->intr_iso_tx_ep && usb_endpoint_is_int_out(ep_desc)) {
					cif_dev->intr_iso_tx_ep = ep_desc;
					BTMTK_INFO("intr_iso_tx_ep i = %d\t"
						"Endpoints 0x%02X, number_of_endpoints=%d",
						i, ep_desc->bEndpointAddress,
						intf->cur_altsetting->desc.bNumEndpoints);
					continue;
				}

				if (!cif_dev->intr_iso_rx_ep && usb_endpoint_is_int_in(ep_desc)) {
					cif_dev->intr_iso_rx_ep = ep_desc;
					BTMTK_INFO("intr_iso_rx_ep i = %d\t"
						"Endpoints 0x%02X, number_of_endpoints=%d",
						i, ep_desc->bEndpointAddress,
						intf->cur_altsetting->desc.bNumEndpoints);
					continue;
				}
			}

			err = usb_driver_claim_interface(&btusb_driver,
							 cif_dev->iso_channel, bdev);
			if (err < 0)
				goto deinit1;
		}
	} else if (BTMTK_IS_BT_1_INTF(ifnum_base)) {
		BTMTK_INFO("interface number = 3, set interface number 4");
		cif_dev->isoc = usb_ifnum_to_if(cif_dev->udev, 4);
	}

	if (cif_dev->isoc) {
		err = usb_driver_claim_interface(&btusb_driver,
						 cif_dev->isoc, bdev);
		if (err < 0)
			goto deinit1;
	}

	/* dongle_index - 1 since BT1 is in same interface */
	if (BTMTK_IS_BT_1_INTF(ifnum_base))
		bdev->dongle_index--;
	BTMTK_DBG("%s: bdev->dongle_index = %d ", __func__, bdev->dongle_index);

	usb_set_intfdata(intf, bdev);

	err = btmtk_woble_initialize(bdev, &cif_dev->bt_woble);
	if (err < 0) {
		BTMTK_ERR("btmtk_main_woble_initialize failed, do chip reset!!!");
		goto end;
	}

	btmtk_woble_wake_unlock(bdev);
	btmtk_assert_wake_unlock();

#if CFG_SUPPORT_BLUEZ
SEND_INIT_RETRY:
	if (bmain_info->hif_hook_chip.bt_open_handler) {
		err = bmain_info->hif_hook_chip.bt_open_handler(bdev);
		if (err < 0) {
			BTMTK_ERR("bt_open_handler fail");
			goto free_setting;
		}
	}

	err = btmtk_send_init_cmds(bdev);
	if (err < 0) {
		BTMTK_ERR("%s, btmtk_send_init_cmds failed, err = %d", __func__, err);
		if (retry > 0) {
			send_init_fail = 1;
			retry--;
			goto end;
		} else
			goto free_setting;
	}
#endif /* CFG_SUPPORT_BLUEZ */

	err = btmtk_register_hci_device(bdev);
	if (err < 0) {
		BTMTK_ERR("btmtk_register_hci_device failed!");
		goto free_setting;
	}

	err = 0;
	goto end;

free_setting:
	btmtk_woble_uninitialize(&cif_dev->bt_woble);
deinit1:
	btmtk_main_cif_uninitialize(bdev, HCI_USB);
free_mem:
	btmtk_cif_free_memory(cif_dev);
end:
#if BTMTK_RUNTIME_ENABLE
	usb_enable_autosuspend(cif_dev->udev);
	device_set_wakeup_enable(&cif_dev->udev->dev, TRUE);
#endif
	if (cap_init_fail == 1) {
		bmain_info->chip_reset_flag = 1;
		cap_init_fail = 0;
		set_bit(BT_PROBE_FAIL_FOR_WHOLE_RESET, &bdev->flags);
		btmtk_reset_trigger(bdev);
		atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DONE);
		return 0;
	}
#if (CFG_SUPPORT_HW_DVT == 0)
	if (load_patch_fail == 1) {
		load_patch_fail = 0;
		err = btmtk_probe_retry(bdev);
		if (err < 0) {
			atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DONE);
			BTMTK_INFO("%s: load patch fail, retry fail", __func__);
			return -1;
		}
		if (test_bit(BT_PROBE_DO_WHOLE_CHIP_RESET, &bdev->flags)) {
			clear_bit(BT_PROBE_DO_WHOLE_CHIP_RESET, &bdev->flags);
			atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DONE);
			BTMTK_INFO("%s: load patch fail, retry do L0", __func__);
			return -1;
		}
		goto LOAD_PATCH_RETRY;
	}
#endif
#if CFG_SUPPORT_BLUEZ
	if (send_init_fail == 1) {
		send_init_fail = 0;
		err = btmtk_probe_retry(bdev);
		if (err < 0) {
			atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DONE);
			return -1;
		}
		if (test_bit(BT_PROBE_DO_WHOLE_CHIP_RESET, &bdev->flags)) {
			clear_bit(BT_PROBE_DO_WHOLE_CHIP_RESET, &bdev->flags);
			atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DONE);
			return -1;
		}

		goto SEND_INIT_RETRY;

	}
#endif

	atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DONE);
	return err;
}

static void btusb_disconnect(struct usb_interface *intf)
{
	struct btmtk_dev *bdev = NULL;
	struct btmtk_usb_dev *cif_dev = NULL;
	struct hci_dev *hdev;

	BTMTK_DBG("intf %p", intf);
	bdev = usb_get_intfdata(intf);
	if (!bdev) {
		BTMTK_WARN("%s: bdev is NULL!", __func__);
		return;
	}

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (!cif_dev) {
		BTMTK_WARN("%s: cif_dev is NULL!", __func__);
		return;
	}

	hdev = bdev->hdev;
	usb_set_intfdata(cif_dev->intf, NULL);

	if (cif_dev->isoc)
		usb_set_intfdata(cif_dev->isoc, NULL);

	if (cif_dev->iso_channel)
		usb_set_intfdata(cif_dev->iso_channel, NULL);

	if (intf == cif_dev->intf) {
		if (cif_dev->isoc)
			usb_driver_release_interface(&btusb_driver, cif_dev->isoc);
		if (cif_dev->iso_channel)
			usb_driver_release_interface(&btusb_driver, cif_dev->iso_channel);
	} else if (intf == cif_dev->isoc) {
		usb_driver_release_interface(&btusb_driver, cif_dev->intf);
	} else if (intf == cif_dev->iso_channel) {
		usb_driver_release_interface(&btusb_driver, cif_dev->intf);
	}

	btmtk_main_cif_disconnect_notify(bdev, HCI_USB);
	btmtk_woble_uninitialize(&cif_dev->bt_woble);
	btmtk_cif_free_memory(cif_dev);

}

#ifdef CONFIG_PM
static int btusb_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct btmtk_dev *bdev = usb_get_intfdata(intf);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
#if CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT
#else
	struct btmtk_woble *bt_woble = &cif_dev->bt_woble;
#endif
	int ret = 0;

	BTMTK_DBG("intf %p", intf);

	if (bdev->suspend_count++) {
		BTMTK_WARN("%s: Has suspended. suspend_count: %d end", __func__, bdev->suspend_count);
		return 0;
	}

#if CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT
	BTMTK_INFO("%s: SKIP Driver woble_suspend flow", __func__);
#else
	ret = btmtk_woble_suspend(bt_woble);
	if (ret < 0)
		BTMTK_ERR("%s: btmtk_woble_suspend return fail %d", __func__, ret);
#endif /* CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT */

	spin_lock_irq(&bdev->txlock);
	if (!(PMSG_IS_AUTO(message) && bdev->tx_in_flight)) {
		set_bit(BTUSB_SUSPENDING, &bdev->flags);
		spin_unlock_irq(&bdev->txlock);
	} else {
		spin_unlock_irq(&bdev->txlock);
		bdev->suspend_count--;
		return -EBUSY;
	}

	cancel_work_sync(&bdev->work);

	btusb_stop_traffic(cif_dev);
	usb_kill_anchored_urbs(&cif_dev->tx_anchor);

	BTMTK_INFO("%s end, suspend_count = %d", __func__, bdev->suspend_count);

	return ret;
}

static int btusb_resume(struct usb_interface *intf)
{
	struct btmtk_dev *bdev = usb_get_intfdata(intf);
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	struct hci_dev *hdev = bdev->hdev;
#if CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT
#else
	struct btmtk_woble *bt_woble = &cif_dev->bt_woble;
#endif
	int err = 0;
	unsigned int ifnum_base = intf->cur_altsetting->desc.bInterfaceNumber;

	BTMTK_INFO("%s begin", __func__);

	if (--bdev->suspend_count) {
		BTMTK_WARN("%s: bdev->suspend_count %d, return 0", __func__,
				bdev->suspend_count);
		return 0;
	}

	/* need to remove it when BT off, need support woble case*/
	/* if (!test_bit(HCI_RUNNING, &hdev->flags)) {
	 * BTMTK_WARN("%s: hdev flags is not hci running. return", __func__);
	 * goto done;
	 * }
	 */

	/* when BT off, BTUSB_INTR_RUNNING will be clear,
	 * so we need to start traffic in btmtk_woble_resume when BT off
	 */
	if (test_bit(BTUSB_INTR_RUNNING, &bdev->flags)) {
		err = btusb_submit_intr_urb(hdev, GFP_NOIO);
		if (err < 0) {
			clear_bit(BTUSB_INTR_RUNNING, &bdev->flags);
			goto done;
		}

		if ((is_mt7961(bdev->chip_id) || is_mt7925(bdev->chip_id)) && BTMTK_IS_BT_0_INTF(ifnum_base)) {
			BTMTK_INFO("%s submit urb", __func__);
			if (cif_dev->reset_intr_ep) {
				err = btusb_submit_intr_reset_urb(hdev, GFP_KERNEL);
				if (err < 0) {
					clear_bit(BTUSB_INTR_RUNNING, &bdev->flags);
					goto done;
				}
			} else
				BTMTK_INFO("%s, reset_intr_ep missing, don't submit_intr_reset_urb!",
					__func__);

			if (cif_dev->intr_iso_rx_ep) {
				err = btusb_submit_intr_ble_isoc_urb(hdev, GFP_KERNEL);
				if (err < 0) {
					usb_kill_anchored_urbs(&cif_dev->ble_isoc_anchor);
					clear_bit(BTUSB_INTR_RUNNING, &bdev->flags);
					goto done;
				}
			} else
				BTMTK_INFO("%s, intr_iso_rx_ep missing, don't submit_intr_ble_isoc_urb!",
					__func__);
		}
	}

	if (test_bit(BTUSB_BULK_RUNNING, &bdev->flags)) {
		err = btusb_submit_bulk_urb(hdev, GFP_NOIO);
		if (err < 0) {
			clear_bit(BTUSB_BULK_RUNNING, &bdev->flags);
			goto done;
		}
	}

	if (test_bit(BTUSB_ISOC_RUNNING, &bdev->flags)) {
		if (btusb_submit_isoc_urb(hdev, GFP_NOIO) < 0)
			clear_bit(BTUSB_ISOC_RUNNING, &bdev->flags);
	}

	if (test_bit(BTUSB_BLE_ISOC_RUNNING, &bdev->flags)) {
		if (btusb_submit_intr_ble_isoc_urb(hdev, GFP_NOIO) < 0)
			clear_bit(BTUSB_BLE_ISOC_RUNNING, &bdev->flags);
	}

	spin_lock_irq(&bdev->txlock);
	clear_bit(BTUSB_SUSPENDING, &bdev->flags);
	spin_unlock_irq(&bdev->txlock);
	schedule_work(&bdev->work);

#if CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT
	BTMTK_INFO("%s: SKIP Driver woble_resume flow", __func__);
#else
	err = btmtk_woble_resume(bt_woble);
	if (err < 0) {
		BTMTK_ERR("%s: btmtk_woble_resume return fail %d", __func__, err);
		goto done;
	}
#endif /* CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT */

	BTMTK_INFO("%s end", __func__);

	return 0;

done:
	spin_lock_irq(&bdev->txlock);
	clear_bit(BTUSB_SUSPENDING, &bdev->flags);
	spin_unlock_irq(&bdev->txlock);

	return err;
}
#endif

static int btmtk_cif_probe(struct usb_interface *intf,
		       const struct usb_device_id *id)
{
	int ret = 0;
	int cif_event = 0;
	unsigned int ifnum_base;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;

	/* Mediatek Driver Version */
	BTMTK_INFO("%s: MTK BT Driver Version : %s", __func__, VERSION);

	DUMP_TIME_STAMP("probe_start");
	btmtk_handle_mutex_lock(bdev);

#ifdef CFG_SUPPORT_CHIP_RESET_KO
	send_reset_event(RESET_MODULE_TYPE_BT, RFSM_EVENT_PROBE_START);
#endif

	/* USB interface only.
	 * USB will need to identify thru descriptor's interface numbering.
	 */
	ifnum_base = intf->cur_altsetting->desc.bInterfaceNumber;
	BTMTK_DBG("intf %p id %p, interfacenum = %d", intf, id, ifnum_base);

	/* interface numbers are hardcoded in the spec */
	if (ifnum_base != BT0_MCU_INTERFACE_NUM &&
		ifnum_base != BT1_MCU_INTERFACE_NUM) {
		btmtk_handle_mutex_unlock(bdev);
		return -ENODEV;
	}

	/* Retrieve priv data and set to interface structure */
	bdev = btmtk_get_dev();
	if (!bdev) {
		BTMTK_WARN("%s: bdev is NULL!", __func__);
		btmtk_handle_mutex_unlock(bdev);
		return -ENODEV;
	}
	usb_set_intfdata(intf, bdev);
	bdev->cif_dev = &g_usb_dev[bdev->dongle_index][intf_to_idx[ifnum_base]];

	/* Retrieve current HIF event state */
	cif_event = HIF_EVENT_PROBE;
	if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
		/* Error */
		BTMTK_WARN("%s intf[%d] priv setting is NULL", __func__, ifnum_base);
		btmtk_handle_mutex_unlock(bdev);
		return -ENODEV;
	}

	cif_state = &bdev->cif_state[cif_event];

	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

	/* Do HIF events */
	ret = btusb_probe(intf, id);

	/* Set End/Error state */
	if (ret == 0)
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);

#ifdef CFG_SUPPORT_CHIP_RESET_KO
	send_reset_event(RESET_MODULE_TYPE_BT, RFSM_EVENT_PROBE_SUCCESS);
#endif

	DUMP_TIME_STAMP("probe_end");
	btmtk_handle_mutex_unlock(bdev);
	return ret;
}

static void btmtk_cif_disconnect(struct usb_interface *intf)
{
	int cif_event = 0;
	int state = 0;
	unsigned int ifnum_base;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_usb_dev *cif_dev = NULL;

	BTMTK_WARN("%s: begin", __func__);

	BTMTK_CIF_GET_DEV_PRIV(bdev, intf, ifnum_base);
	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL, return", __func__);
		return;
	}

	btmtk_handle_mutex_lock(bdev);

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_DISCONNECT) {
		BTMTK_INFO("%s:usb disconnect is doing, return", __func__);
		btmtk_handle_mutex_unlock(bdev);
		return;
	}


	/* Retrieve current HIF event state */
	cif_event = HIF_EVENT_DISCONNECT;
	if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
		/* Error */
		BTMTK_WARN("%s intf[%d] priv setting is NULL", __func__, ifnum_base);
		btmtk_handle_mutex_unlock(bdev);
		return;
	}

	cif_state = &bdev->cif_state[cif_event];

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (!cif_dev) {
		BTMTK_WARN("%s: cif_dev is NULL!", __func__);
		btmtk_handle_mutex_unlock(bdev);
		return;
	}

	clear_bit(BTUSB_INTR_RUNNING, &bdev->flags);
	clear_bit(BTUSB_BULK_RUNNING, &bdev->flags);
	clear_bit(BTUSB_ISOC_RUNNING, &bdev->flags);
	clear_bit(BTUSB_BLE_ISOC_RUNNING, &bdev->flags);
	clear_bit(BTUSB_WMT_RUNNING, &bdev->flags);
	clear_bit(BTUSB_SUSPENDING, &bdev->flags);
	clear_bit(BTUSB_OPENED, &bdev->flags);
#if BTMTK_RUNTIME_ENABLE
	usb_disable_autosuspend(cif_dev->udev);
#endif
	btusb_stop_traffic(cif_dev);

	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

	/* Do HIF events */
	btusb_disconnect(intf);

#ifdef CFG_SUPPORT_CHIP_RESET_KO
	send_reset_event(RESET_MODULE_TYPE_BT, RFSM_EVENT_REMOVE);
#endif

	/* Set End/Error state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	btmtk_handle_mutex_unlock(bdev);
}

#ifdef CONFIG_PM
static int btmtk_cif_suspend(struct usb_interface *intf, pm_message_t message)
{
	int ret = 0;
	unsigned int ifnum_base;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;
	int state = BTMTK_STATE_UNKNOWN;

	BTMTK_INFO("%s, enter", __func__);
	btmtk_handle_mutex_lock(bdev);
	BTMTK_CIF_GET_DEV_PRIV(bdev, intf, ifnum_base);

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_DISCONNECT) {
		BTMTK_ERR("%s: state is disconnect, return", __func__);
		btmtk_handle_mutex_unlock(bdev);
		return -1;
	}
	/* Retrieve current HIF event state */
	if (state == BTMTK_STATE_FW_DUMP) {
		BTMTK_WARN("%s: FW dumping ongoing, don't dos suspend flow!!!", __func__);
		cif_event = HIF_EVENT_FW_DUMP;
	} else
		cif_event = HIF_EVENT_SUSPEND;

	if (BTMTK_IS_BT_0_INTF(ifnum_base) || BTMTK_IS_BT_1_INTF(ifnum_base)) {
		if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
			/* Error */
			BTMTK_WARN("%s intf[%d] priv setting is NULL", __func__, ifnum_base);
			btmtk_handle_mutex_unlock(bdev);
			return -ENODEV;
		}

		cif_state = &bdev->cif_state[cif_event];

		/* Set Entering state */
		btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

		/* Do HIF events */
		ret = btusb_suspend(intf, message);

		/* Set End/Error state */
		if (ret == 0)
			btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
		else
			btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
	} else
		BTMTK_INFO("%s, interface num is for isoc interface, do't do suspend!", __func__);

	BTMTK_INFO("%s, end. ret = %d", __func__, ret);
	btmtk_handle_mutex_unlock(bdev);
	return ret;
}

static int btmtk_cif_resume(struct usb_interface *intf)
{
	int ret = 0;
	unsigned int ifnum_base;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;
	int state = 0;

	BTMTK_INFO("%s, enter", __func__);
	btmtk_handle_mutex_lock(bdev);
	BTMTK_CIF_GET_DEV_PRIV(bdev, intf, ifnum_base);
	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_DISCONNECT) {
		BTMTK_ERR("%s: state is disconnect, return", __func__);
		btmtk_handle_mutex_unlock(bdev);
		return -1;
	}

	if (BTMTK_IS_BT_0_INTF(ifnum_base) || BTMTK_IS_BT_1_INTF(ifnum_base)) {
		/* Retrieve current HIF event state */
		cif_event = HIF_EVENT_RESUME;
		if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
			/* Error */
			BTMTK_WARN("%s intf[%d] priv setting is NULL", __func__, ifnum_base);
			btmtk_handle_mutex_unlock(bdev);
			return -ENODEV;
		}

		cif_state = &bdev->cif_state[cif_event];

		/* Set Entering state */
		btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

		/* Do HIF events */
		ret = btusb_resume(intf);

#if CFG_SUPPORT_LEAUDIO_CLK
		{
			extern int btmtk_le_audio_clk_enable(struct btmtk_dev *bdev);
			btmtk_le_audio_clk_enable(bdev);
		}
#endif

		/* Set End/Error state */
		if (ret == 0)
			btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
		else
			btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
	} else
		BTMTK_INFO("%s, interface num is for isoc interface, do't do resume!", __func__);

	BTMTK_INFO("%s, end. ret = %d", __func__, ret);
	btmtk_handle_mutex_unlock(bdev);
	return ret;
}
#endif	/* CONFIG_PM */

#if !BT_DISABLE_RESET_RESUME
static int btmtk_cif_reset_resume(struct usb_interface *intf)
{
	BTMTK_INFO("%s: Call resume directly", __func__);
	return btmtk_cif_resume(intf);
}
#endif

static struct usb_driver btusb_driver = {
	.name		= "btusb",
	.probe		= btmtk_cif_probe,
	.disconnect	= btmtk_cif_disconnect,
#ifdef CONFIG_PM
	.suspend	= btmtk_cif_suspend,
	.resume		= btmtk_cif_resume,
#endif
#if !BT_DISABLE_RESET_RESUME
	.reset_resume = btmtk_cif_reset_resume,
#endif
	.id_table	= btusb_table,
	.supports_autosuspend = 1,
	.disable_hub_initiated_lpm = 1,
};

int btmtk_cif_register(void)
{
	int retval = 0;
	struct hif_hook_ptr hook;

	BTMTK_INFO("%s", __func__);

	/* register system power off callback function. */
#if (CFG_GKI_SUPPORT == 0)
	do {
		typedef void (*func_ptr) (int (*f) (void));
		char *func_name = "RegisterPdwncCallback";
		func_ptr pFunc = (func_ptr) btmtk_kallsyms_lookup_name(func_name);

		if (pFunc) {
			BTMTK_INFO("%s: Register Pdwnc callback success.", __func__);
			pFunc(&btmtk_usb_enter_standby);
		} else
			BTMTK_WARN("%s: No Exported Func Found [%s], just skip!", __func__, func_name);
	} while (0);
#endif
	memset(&hook, 0, sizeof(struct hif_hook_ptr));
	hook.open = btmtk_usb_open;
	hook.close = btmtk_usb_close;
	hook.reg_read = btmtk_usb_read_register;
	hook.reg_write = btmtk_usb_write_register;
	hook.send_cmd = btmtk_usb_send_cmd;
	hook.send_and_recv = btmtk_usb_send_and_recv;
	hook.event_filter = btmtk_usb_event_filter;
	hook.subsys_reset = btmtk_usb_subsys_reset;
	hook.whole_reset = btmtk_usb_toggle_rst_pin;
	hook.chip_reset_notify = btmtk_usb_chip_reset_notify;
	hook.cif_mutex_lock = btmtk_usb_cif_mutex_lock;
	hook.cif_mutex_unlock = btmtk_usb_cif_mutex_unlock;
	hook.dl_dma = btmtk_usb_load_fw_patch_using_dma;
	hook.dump_debug_sop = btmtk_usb_dump_debug_sop;
	hook.enter_standby = btmtk_usb_enter_standby;
	hook.write_uhw_register = btmtk_cif_write_uhw_register;
	hook.read_uhw_register = btmtk_cif_read_uhw_register;
	btmtk_reg_hif_hook(&hook);

	retval = usb_register(&btusb_driver);
	if (retval)
		BTMTK_ERR("*** USB registration fail(%d)! ***", retval);
	else
		BTMTK_INFO("%s, usb registration success!", __func__);
	return retval;
}

int btmtk_cif_deregister(void)
{
	BTMTK_INFO("%s", __func__);
	usb_deregister(&btusb_driver);
	BTMTK_INFO("%s: Done", __func__);
	return 0;
}

static int btmtk_cif_allocate_memory(struct btmtk_usb_dev *cif_dev)
{
	int err = -1;

	BTMTK_INFO("%s Begin", __func__);

	if (cif_dev->o_usb_buf == NULL) {
		cif_dev->o_usb_buf = kzalloc(HCI_MAX_COMMAND_SIZE, GFP_KERNEL);
		if (!cif_dev->o_usb_buf) {
			BTMTK_ERR("%s: alloc memory fail (bdev->o_usb_buf)", __func__);
			goto end;
		}
	}

	if (cif_dev->urb_intr_buf == NULL) {
		cif_dev->urb_intr_buf = kzalloc(URB_MAX_BUFFER_SIZE, GFP_KERNEL);
		if (!cif_dev->urb_intr_buf) {
			BTMTK_ERR("%s: alloc memory fail (bdev->urb_intr_buf)", __func__);
			goto err2;
		}
	}
	if (cif_dev->urb_bulk_buf == NULL) {
		cif_dev->urb_bulk_buf = kzalloc(URB_MAX_BUFFER_SIZE, GFP_KERNEL);
		if (!cif_dev->urb_bulk_buf) {
			BTMTK_ERR("%s: alloc memory fail (bdev->urb_bulk_buf)", __func__);
			goto err1;
		}
	}
	if (cif_dev->urb_ble_isoc_buf == NULL) {
		cif_dev->urb_ble_isoc_buf = kzalloc(URB_MAX_BUFFER_SIZE, GFP_KERNEL);
		if (!cif_dev->urb_ble_isoc_buf) {
			BTMTK_ERR("%s: alloc memory fail (bdev->urb_ble_isoc_buf)", __func__);
			goto err0;
		}
	}

	BTMTK_INFO("%s End", __func__);
	return 0;

err0:
	kfree(cif_dev->urb_bulk_buf);
	cif_dev->urb_bulk_buf = NULL;
err1:
	kfree(cif_dev->urb_intr_buf);
	cif_dev->urb_intr_buf = NULL;
err2:
	kfree(cif_dev->o_usb_buf);
	cif_dev->o_usb_buf = NULL;
end:
	return err;
}

static void btmtk_cif_free_memory(struct btmtk_usb_dev *cif_dev)
{
	if (!cif_dev) {
		BTMTK_ERR("%s: bdev is NULL!", __func__);
		return;
	}

	kfree(cif_dev->o_usb_buf);
	cif_dev->o_usb_buf = NULL;

	kfree(cif_dev->urb_intr_buf);
	cif_dev->urb_intr_buf = NULL;

	kfree(cif_dev->urb_bulk_buf);
	cif_dev->urb_bulk_buf = NULL;

	kfree(cif_dev->urb_ble_isoc_buf);
	cif_dev->urb_ble_isoc_buf = NULL;

	memset(cif_dev, 0, sizeof(struct btmtk_usb_dev));

	BTMTK_INFO("%s: Success", __func__);
}

static int btmtk_cif_write_uhw_register(struct btmtk_dev *bdev, u32 reg, u32 val)
{
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	int ret = 0;
	__le16 reg_high;
	__le16 reg_low;
	u8 reset_buf[4];

	if (btmtk_get_chip_state(bdev) == BTMTK_STATE_DISCONNECT)
		return -ENODEV;

	reg_high = ((reg >> 16) & 0xffff);
	reg_low = (reg & 0xffff);

	reset_buf[0] = (val & 0x00ff);
	reset_buf[1] = ((val >> 8) & 0x00ff);
	reset_buf[2] = ((val >> 16) & 0x00ff);
	reset_buf[3] = ((val >> 24) & 0x00ff);

	memcpy(cif_dev->o_usb_buf, reset_buf, sizeof(reset_buf));
#if BTMTK_RUNTIME_ENABLE
	ret = usb_autopm_get_interface(cif_dev->intf);
	if (ret < 0) {
		BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
	}
#endif
	ret = usb_control_msg(cif_dev->udev, usb_sndctrlpipe(cif_dev->udev, 0),
			0x02,						/* bRequest */
			0x5E,						/* bRequestType */
			reg_high,					/* wValue */
			reg_low,					/* wIndex */
			cif_dev->o_usb_buf,
			sizeof(reset_buf), USB_CTRL_IO_TIMO);
#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif
	BTMTK_DBG("%s: high=%x, reg_low=%x, val=%x", __func__, reg_high, reg_low, val);
	BTMTK_DBG("%s: reset_buf = %x %x %x %x", __func__, reset_buf[3], reset_buf[2], reset_buf[1], reset_buf[0]);

	if (ret < 0) {
		val = 0xffffffff;
		BTMTK_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, val);
		return ret;
	}
	return 0;
}

static int btmtk_cif_read_uhw_register(struct btmtk_dev *bdev, u32 reg, u32 *val)
{
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	int ret = 0;
	__le16 reg_high;
	__le16 reg_low;

	if (btmtk_get_chip_state(bdev) == BTMTK_STATE_DISCONNECT || bdev->io_buf == NULL)
		return -ENODEV;

	reg_high = ((reg >> 16) & 0xffff);
	reg_low = (reg & 0xffff);

	memset(bdev->io_buf, 0, IO_BUF_SIZE);
#if BTMTK_RUNTIME_ENABLE
	ret = usb_autopm_get_interface(cif_dev->intf);
	if (ret < 0) {
		BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
	}
#endif
	ret = usb_control_msg(cif_dev->udev, usb_rcvctrlpipe(cif_dev->udev, 0),
			0x01,						/* bRequest */
			0xDE,						/* bRequestType */
			reg_high,					/* wValue */
			reg_low,					/* wIndex */
			bdev->io_buf,
			4, USB_CTRL_IO_TIMO);
#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif
	if (ret < 0) {
		*val = 0xffffffff;
		BTMTK_ERR("%s: error(%d), reg=%x, value=0x%08x", __func__, ret, reg, *val);
		return ret;
	}

	memmove(val, bdev->io_buf, sizeof(u32));
	*val = le32_to_cpu(*val);

	BTMTK_DBG("%s: reg=%x, value=0x%08x", __func__, reg, *val);

	return 0;
}

static int btmtk_usb_read_register(struct btmtk_dev *bdev, u32 reg, u32 *val)
{
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	int ret = 0;
	__le16 reg_high;
	__le16 reg_low;

	if (btmtk_get_chip_state(bdev) == BTMTK_STATE_DISCONNECT || bdev->io_buf == NULL)
		return -ENODEV;

	reg_high = ((reg >> 16) & 0xffff);
	reg_low = (reg & 0xffff);

	memset(bdev->io_buf, 0, IO_BUF_SIZE);
#if BTMTK_RUNTIME_ENABLE
	ret = usb_autopm_get_interface(cif_dev->intf);
	if (ret < 0) {
		BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
	}
#endif
	ret = usb_control_msg(cif_dev->udev, usb_rcvctrlpipe(cif_dev->udev, 0),
			0x63,						/* bRequest */
			DEVICE_VENDOR_REQUEST_IN,	/* bRequestType */
			reg_high,					/* wValue */
			reg_low,					/* wIndex */
			bdev->io_buf,
			sizeof(u32), USB_CTRL_IO_TIMO);
#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif

	if (ret < 0) {
		*val = 0xffffffff;
		BTMTK_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, *val);
		btmtk_handle_mutex_unlock(bdev);
		if (btmtk_get_chip_state(bdev) != BTMTK_STATE_FW_DUMP)
			btmtk_reset_trigger(bdev);
		return ret;
	}

	memmove(val, bdev->io_buf, sizeof(u32));
	*val = le32_to_cpu(*val);

	btmtk_handle_mutex_unlock(bdev);
	return 0;
}

static int btmtk_usb_write_register(struct btmtk_dev *bdev, u32 reg, u32 val)
{
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	int ret = 0;
	__le16 reg_high;
	__le16 reg_low;
	u8 buf[4];
	int state = 0;

	btmtk_handle_mutex_lock(bdev);
	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_DISCONNECT) {
		BTMTK_ERR("%s: state is disconnect, return", __func__);
		btmtk_handle_mutex_unlock(bdev);
		return -1;
	}

	reg_high = ((reg >> 16) & 0xffff);
	reg_low = (reg & 0xffff);

	buf[0] = (val & 0x00ff);
	buf[1] = ((val >> 8) & 0x00ff);
	buf[2] = ((val >> 16) & 0x00ff);
	buf[3] = ((val >> 24) & 0x00ff);

	memcpy(cif_dev->o_usb_buf, buf, sizeof(buf));
#if BTMTK_RUNTIME_ENABLE
	ret = usb_autopm_get_interface(cif_dev->intf);
	if (ret < 0) {
		BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
	}
#endif
	ret = usb_control_msg(cif_dev->udev, usb_sndctrlpipe(cif_dev->udev, 0),
			0x66,						/* bRequest */
			0x40,	/* bRequestType */
			reg_high,					/* wValue */
			reg_low,					/* wIndex */
			cif_dev->o_usb_buf,
			sizeof(buf), USB_CTRL_IO_TIMO);
#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif
	BTMTK_DBG("%s: buf = %x %x %x %x", __func__, buf[3], buf[2], buf[1], buf[0]);

	if (ret < 0) {
		val = 0xffffffff;
		BTMTK_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, val);
		btmtk_handle_mutex_unlock(bdev);
		return ret;
	}

	btmtk_handle_mutex_unlock(bdev);
	return 0;
}

static void btmtk_cif_load_rom_patch_complete(struct urb *urb)
{
	struct completion *sent_to_mcu_done = (struct completion *)urb->context;

	complete(sent_to_mcu_done);
}

int btmtk_cif_send_control_out(struct btmtk_dev *bdev, struct sk_buff *skb, int delay, int retry)
{
	struct btmtk_usb_dev *cif_dev = NULL;
	int ret = 0;
	unsigned int ifnum_base;

	if (bdev == NULL || bdev->hdev == NULL || bdev->io_buf == NULL || skb == NULL ||
		skb->len > HCI_MAX_COMMAND_SIZE || skb->len <= 0) {
		BTMTK_ERR("%s: incorrect parameter", __func__);
		ret = -1;
		goto exit;
	}

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (cif_dev->udev == NULL || cif_dev->o_usb_buf == NULL) {
		BTMTK_ERR("%s: cif_dev is invalid", __func__);
		ret = -1;
		goto exit;
	}

	ifnum_base = cif_dev->intf->cur_altsetting->desc.bInterfaceNumber;

	/* send wmt command */
	memcpy(cif_dev->o_usb_buf, skb->data + 1, skb->len - 1);
	BTMTK_INFO_RAW(skb->data + 1, skb->len - 1, "%s: cmd:", __func__);
	if (BTMTK_IS_BT_0_INTF(ifnum_base)) {
#if BTMTK_RUNTIME_ENABLE
		ret = usb_autopm_get_interface(cif_dev->intf);
		if (ret < 0) {
			BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
		}
#endif
		ret = usb_control_msg(cif_dev->udev, usb_sndctrlpipe(cif_dev->udev, 0),
				0x01, DEVICE_CLASS_REQUEST_OUT, 0x30, 0x00, (void *)cif_dev->o_usb_buf,
				skb->len - 1, USB_CTRL_IO_TIMO);
#if BTMTK_RUNTIME_ENABLE
		usb_autopm_put_interface(cif_dev->intf);
#endif
	} else if (BTMTK_IS_BT_1_INTF(ifnum_base)) {
#if BTMTK_RUNTIME_ENABLE
		ret = usb_autopm_get_interface(cif_dev->intf);
		if (ret < 0) {
			BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
		}
#endif
		ret = usb_control_msg(cif_dev->udev, usb_sndctrlpipe(cif_dev->udev, 0),
				0x00, 0x21, 0x00, 0x03, (void *)cif_dev->o_usb_buf, skb->len - 1, USB_CTRL_IO_TIMO);
#if BTMTK_RUNTIME_ENABLE
		usb_autopm_put_interface(cif_dev->intf);
#endif
	}

	if (ret < 0) {
		BTMTK_ERR("%s: command send failed(%d)", __func__, ret);
		goto exit;
	}

	/* only when  ret = 0 need to free skb; when ret < 0, it will be free in btmtk_main_send_cmd */
	kfree_skb(skb);
	skb = NULL;

exit:
	return ret;
}

static int btmtk_cif_send_bulk_out(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	int ret = 0;
	struct urb *urb;
	unsigned int pipe;
	struct completion sent_to_mcu_done;
	void *buf;
	struct btmtk_usb_dev *cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		ret = -ENOMEM;
		goto exit;
	}

	/* why need to alloc dma buffer??*/
	buf = usb_alloc_coherent(cif_dev->udev, UPLOAD_PATCH_UNIT, GFP_KERNEL, &urb->transfer_dma);
	if (!buf) {
		ret = -ENOMEM;
		goto error1;
	}
	init_completion(&sent_to_mcu_done);

	pipe = usb_sndbulkpipe(cif_dev->udev, cif_dev->bulk_tx_ep->bEndpointAddress);

	memcpy(buf, skb->data + 1, skb->len - 1);
	usb_fill_bulk_urb(urb,
			cif_dev->udev,
			pipe,
			buf,
			skb->len - 1,
			(usb_complete_t)btmtk_cif_load_rom_patch_complete,
			&sent_to_mcu_done);

	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
#if BTMTK_RUNTIME_ENABLE
	ret = usb_autopm_get_interface(cif_dev->intf);
	if (ret < 0) {
		BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
	}
#endif
	ret = usb_submit_urb(urb, GFP_KERNEL);
	if (ret < 0) {
#if BTMTK_RUNTIME_ENABLE
		usb_autopm_put_interface(cif_dev->intf);
#endif
		BTMTK_ERR("%s: submit urb failed (%d)", __func__, ret);
		goto error0;
	}
#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif
	if (!wait_for_completion_timeout
			(&sent_to_mcu_done, msecs_to_jiffies(3000))) {
		usb_kill_urb(urb);
		BTMTK_ERR("%s: upload rom_patch timeout", __func__);
		ret = -ETIME;
		goto error0;
	}

	kfree_skb(skb);
	skb = NULL;
error0:
	usb_free_coherent(cif_dev->udev, UPLOAD_PATCH_UNIT, buf, urb->transfer_dma);
error1:
	usb_free_urb(urb);
exit:
	return ret;
}

int btmtk_usb_send_cmd(struct btmtk_dev *bdev, struct sk_buff *skb,
		int delay, int retry, int pkt_type, bool flag)
{
	int ret = 0;
	btmtk_save_filter_vendor_cmd(skb, bdev, flag);

	if (pkt_type == BTMTK_TX_CMD_FROM_DRV) {
		/* handle wmt cmd from driver */
		ret = btmtk_cif_send_control_out(bdev, skb, delay, retry);
	} else if (pkt_type == BTMTK_TX_ACL_FROM_DRV) {
		/* bulk out for load rom patch*/
		ret = btmtk_cif_send_bulk_out(bdev, skb);
	} else if (pkt_type == BTMTK_TX_PKT_FROM_HOST) {
		/* handle hci cmd and acl pkt from host, handle hci cmd from driver */
		ret = btusb_send_frame(bdev->hdev, skb, flag);
	}

	return ret;
}

static int btmtk_cif_recv_evt(struct btmtk_dev *bdev, int delay, int retry)
{
	struct btmtk_usb_dev *cif_dev = NULL;
	int ret = -1;	/* if successful, 0 */
	unsigned int ifnum_base;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev == NULL!", __func__);
		return ret;
	}

	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (!cif_dev->udev || !bdev->hdev) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		return ret;
	}

	ifnum_base = cif_dev->intf->cur_altsetting->desc.bInterfaceNumber;
get_response_again:
	/* us delay */
	usleep_range(delay * TIME_MULTIPL, delay * TIME_MULTIPL + TIME_US_OFFSET_RANGE);

	/* check WMT event */
	memset(bdev->io_buf, 0, IO_BUF_SIZE);
	bdev->io_buf[0] = HCI_EVENT_PKT;
	if (BTMTK_IS_BT_0_INTF(ifnum_base)) {
#if BTMTK_RUNTIME_ENABLE
		ret = usb_autopm_get_interface(cif_dev->intf);
		if (ret < 0) {
			BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
		}
#endif
		ret = usb_control_msg(cif_dev->udev, usb_rcvctrlpipe(cif_dev->udev, 0),
				0x01, DEVICE_VENDOR_REQUEST_IN, 0x30, 0x00, bdev->io_buf + 1,
				HCI_USB_IO_BUF_SIZE, USB_CTRL_IO_TIMO);
#if BTMTK_RUNTIME_ENABLE
		usb_autopm_put_interface(cif_dev->intf);
#endif
	} else if (BTMTK_IS_BT_1_INTF(ifnum_base)) {
#if BTMTK_RUNTIME_ENABLE
		ret = usb_autopm_get_interface(cif_dev->intf);
		if (ret < 0) {
			BTMTK_ERR("%s: usb_autopm_get_interface fail", __func__);
		}
#endif
		ret = usb_control_msg(cif_dev->udev, usb_rcvctrlpipe(cif_dev->udev, 0),
				0x01, 0xA1, 0x30, 0x03, bdev->io_buf + 1, HCI_USB_IO_BUF_SIZE,
				USB_CTRL_IO_TIMO);
#if BTMTK_RUNTIME_ENABLE
		usb_autopm_put_interface(cif_dev->intf);
#endif
	}
	if (ret < 0) {
		BTMTK_ERR("%s: event get failed(%d)", __func__, ret);
		return ret;
	}

	if (ret > 0) {
		BTMTK_DBG_RAW(bdev->io_buf, ret + 1, "%s OK: EVT:", __func__);
		return ret + 1; /* return read length */
	} else if (retry > 0) {
		BTMTK_WARN("%s: Trying to get response... (%d)", __func__, retry);
		retry--;
		goto get_response_again;
	} else
		BTMTK_ERR("%s NG: do not got response:(%d)", __func__, ret);

	return -1;
}

int btmtk_usb_send_and_recv(struct btmtk_dev *bdev,
		struct sk_buff *skb,
		const uint8_t *event, const int event_len,
		int delay, int retry, int pkt_type, bool flag)
{
	unsigned long start_time = 0;
	int ret = 0;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev == NULL!", __func__);
		ret = -1;
		return ret;
	}

	if ((pkt_type == BTMTK_TX_CMD_FROM_DRV || pkt_type == BTMTK_TX_ACL_FROM_DRV)) {
		BTMTK_DBG_RAW(skb->data, skb->len, "%s, send, len = %d", __func__, skb->len);

		ret = btmtk_usb_send_cmd(bdev, skb, delay, retry, pkt_type, flag);
		if (ret < 0) {
			BTMTK_ERR("%s btmtk_usb_send_cmd failed!!", __func__);
			goto fw_assert;
		}

		if (event && event_len > 0) {
			bdev->recv_evt_len = btmtk_cif_recv_evt(bdev, delay, retry);
			if (bdev->recv_evt_len < 0) {
				BTMTK_ERR("%s btmtk_cif_recv_evt failed!!", __func__);
				ret = -ERRNUM;
				goto fw_assert;
			}

			if (bdev->io_buf && bdev->recv_evt_len >= event_len) {
				if (memcmp(bdev->io_buf, event, event_len) == 0) {
					ret = 0;
					goto exit;
				}
			}
			BTMTK_INFO("%s compare fail", __func__);
			BTMTK_INFO_RAW(event, event_len, "%s: event_need_compare:", __func__);
			if (bdev->io_buf)
				BTMTK_INFO_RAW(bdev->io_buf, bdev->recv_evt_len,
					"%s: RCV:", __func__);
			ret = -ERRNUM;
		} else {
			ret = 0;
		}
	} else {
		if (event) {
			if (event_len > EVENT_COMPARE_SIZE) {
				BTMTK_ERR("%s, event_len (%d) > EVENT_COMPARE_SIZE(%d), error",
					__func__, event_len, EVENT_COMPARE_SIZE);
				ret = -1;
				goto exit;
			}
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE;
			memcpy(event_need_compare, event + 1, event_len - 1);
			event_need_compare_len = event_len - 1;

			start_time = jiffies;
			/* check hci event /wmt event for SDIO/UART interface, check hci
			 * event for USB interface
			 */
			BTMTK_INFO("event_need_compare_len %d, event_compare_status %d",
				event_need_compare_len, event_compare_status);
		} else {
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
		}

		BTMTK_DBG_RAW(skb->data, skb->len, "%s, send, len = %d", __func__, skb->len);

		ret = btmtk_usb_send_cmd(bdev, skb, delay, retry, pkt_type, flag);
		if (ret < 0) {
			BTMTK_ERR("%s btmtk_usb_send_cmd failed!!", __func__);
			goto fw_assert;
		}


		if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS) {
			ret = 0;
		} else {
			BTMTK_INFO("%s: begin wait event interruptible", __func__);
			wait_event_timeout(bdev->compare_event_wq, (event_compare_status == BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS),
					msecs_to_jiffies(WOBLE_COMP_EVENT_TIMO));
			if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS) {
				ret = 0;
			}
		}

		if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE) {
			BTMTK_ERR("%s wait expect event timeout!!", __func__);
			ret = -ERRNUM;
			goto fw_assert;
		}

		event_compare_status = BTMTK_EVENT_COMPARE_STATE_NOTHING_NEED_COMPARE;
	}
	goto exit;
fw_assert:
	btmtk_send_assert_cmd(bdev);
exit:
	return ret;
}

void btmtk_usb_chip_reset_notify(struct btmtk_dev *bdev)
{
	cancel_work_sync(&bdev->work);
	cancel_work_sync(&bdev->waker);
}

int btmtk_usb_event_filter(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	struct data_struct read_address_event = {0};

	BTMTK_DBG("%s enter", __func__);

	if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE &&
		skb->len >= event_need_compare_len) {
		BTMTK_GET_CMD_OR_EVENT_DATA(bdev, READ_ADDRESS_EVT, read_address_event);

		if (memcmp(skb->data, &read_address_event.content[1], read_address_event.len - 1) == 0
			&& (skb->len == (read_address_event.len - 1 + BD_ADDRESS_SIZE))) {
			memcpy(bdev->bdaddr, &skb->data[READ_ADDRESS_EVT_PAYLOAD_OFFSET - 1], BD_ADDRESS_SIZE);
			BTMTK_INFO("%s: GET BDADDR = "MACSTR, __func__, MAC2STR(bdev->bdaddr));

			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
			wake_up(&bdev->compare_event_wq);
		} else if (memcmp(skb->data, event_need_compare,
					event_need_compare_len) == 0) {
			/* if it is wobx debug event, just print in kernel log, drop it
			 * by driver, don't send to stack
			 */
			if (skb->data[0] == WOBLE_DEBUG_EVT_TYPE)
				BTMTK_INFO_RAW(skb->data, skb->len, "%s: wobx debug log:", __func__);

			/* If driver need to check result from skb, it can get from io_buf
			 * Such as chip_id, fw_version, etc.
			 */
			bdev->io_buf[0] = bt_cb(skb)->pkt_type;
			memcpy(&bdev->io_buf[1], skb->data, skb->len);

			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
			wake_up(&bdev->compare_event_wq);
			BTMTK_INFO("%s, compare success", __func__);
		} else {
			if (skb->data[0] != BLE_EVT_TYPE) {
				/* Don't care BLE event */
				BTMTK_INFO("%s compare fail", __func__);
				BTMTK_INFO_RAW(event_need_compare, event_need_compare_len,
					"%s: event_need_compare:", __func__);
				BTMTK_INFO_RAW(skb->data, skb->len, "%s: skb->data:", __func__);
			}

			if (btmtk_vendor_cmd_filter(bdev, skb))
				return 1;

			return 0;
		}

		return 1;
	} else {
		if (btmtk_vendor_cmd_filter(bdev, skb))
			return 1;
	}

	return 0;
}

