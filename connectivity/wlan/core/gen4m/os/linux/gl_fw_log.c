// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "precomp.h"
#include "gl_fw_log.h"
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
#include "connv3_debug_utility.h"
#include "connsyslog/connv3_mcu_log.h"
#else
#include "connsys_debug_utility.h"
#endif

#if CFG_SUPPORT_CONNAC1X
#define CONNLOG_TYPE_WF			CONNLOG_TYPE_WIFI
#elif (CFG_SUPPORT_CONNAC2X == 1)
#define CONNLOG_TYPE_WF			CONN_DEBUG_TYPE_WIFI
#else
#define CONNLOG_TYPE_WF			CONNV3_DEBUG_TYPE_WIFI
#endif

struct fw_log_wifi_interface fw_log_wifi_inf;

static int fw_log_wifi_open(struct inode *inode, struct file *file);
static int fw_log_wifi_release(struct inode *inode, struct file *file);
static ssize_t fw_log_wifi_read(struct file *filp, char __user *buf,
	size_t len, loff_t *off);
static unsigned int fw_log_wifi_poll(struct file *filp, poll_table *wait);
static long fw_log_wifi_unlocked_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg);
#ifdef CONFIG_COMPAT
static long fw_log_wifi_compat_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg);
#endif

const struct file_operations fw_log_wifi_fops = {
	.open = fw_log_wifi_open,
	.release = fw_log_wifi_release,
	.read = fw_log_wifi_read,
	.poll = fw_log_wifi_poll,
	.unlocked_ioctl = fw_log_wifi_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = fw_log_wifi_compat_ioctl,
#endif
};

void wifi_fwlog_event_func_register(void (*func)(int, int))
{
	struct fw_log_wifi_interface *prInf = &fw_log_wifi_inf;

	prInf->cb = func;
}

static int fw_log_wifi_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int fw_log_wifi_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t fw_log_wifi_read(struct file *filp, char __user *buf,
	size_t len, loff_t *off)
{
	ssize_t sz = 0;

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	sz = connv3_log_read_to_user(CONNLOG_TYPE_WF, buf, len);
#else
	sz = connsys_log_read_to_user(CONNLOG_TYPE_WF, buf, len);
#endif

	return sz;
}

static unsigned int fw_log_wifi_poll(struct file *filp, poll_table *wait)
{
	struct fw_log_wifi_interface *prInf = &fw_log_wifi_inf;
	unsigned int ret = 0;

	poll_wait(filp, &prInf->wq, wait);

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	if (connv3_log_get_buf_size(CONNLOG_TYPE_WF) > 0)
		ret = (POLLIN | POLLRDNORM);
#else
	if (connsys_log_get_buf_size(CONNLOG_TYPE_WF) > 0)
		ret = (POLLIN | POLLRDNORM);
#endif

	return ret;
}

static long fw_log_wifi_unlocked_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	struct fw_log_wifi_interface *prInf = &fw_log_wifi_inf;
	int ret = 0;

	down(&prInf->ioctl_mtx);

	switch (cmd) {
	case WIFI_FW_LOG_IOCTL_ON_OFF:{
		int log_on_off = (int)arg;

		if (prInf->cb)
			prInf->cb(WIFI_FW_LOG_CMD_ON_OFF, log_on_off);
		break;
	}
	case WIFI_FW_LOG_IOCTL_SET_LEVEL:{
		int log_level = (int)arg;

		if (prInf->cb)
			prInf->cb(WIFI_FW_LOG_CMD_SET_LEVEL, log_level);

		break;
	}
	case WIFI_FW_LOG_IOCTL_GET_VERSION:{
		prInf->ver_length = 0;
		kalMemZero(prInf->ver_name, MANIFEST_BUFFER_SIZE);
		schedule_work(&prInf->getFwVerQ);
		flush_work(&prInf->getFwVerQ);

		if (copy_to_user((char *) arg, prInf->ver_name,
				 prInf->ver_length))
			ret = -EFAULT;

		DBGLOG(INIT, DEBUG, "ver_name=%s\n", prInf->ver_name);
		break;
	}
	default:
		ret = -EINVAL;
	}

	up(&prInf->ioctl_mtx);

	return ret;
}

#ifdef CONFIG_COMPAT
static long fw_log_wifi_compat_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	long ret = 0;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	fw_log_wifi_unlocked_ioctl(filp, cmd, arg);

	return ret;
}
#endif

static void fw_log_wifi_inf_event_cb(void)
{
	struct fw_log_wifi_interface *prInf = &fw_log_wifi_inf;

	wake_up_interruptible(&prInf->wq);
}

static void fw_log_get_version_workQ(struct work_struct *work)
{
	struct fw_log_wifi_interface *prInf = &fw_log_wifi_inf;
	struct mt66xx_chip_info *prChipInfo;
	struct GLUE_INFO *prGlueInfo = wlanGetGlueInfo();

	glGetChipInfo((void **)&prChipInfo);

	if (prChipInfo && prChipInfo->fw_dl_ops->getFwVerInfo)
		prChipInfo->fw_dl_ops->getFwVerInfo(prGlueInfo,
			prInf->ver_name,
			&prInf->ver_length, MANIFEST_BUFFER_SIZE);
}

uint32_t fw_log_notify_rcv(enum ENUM_FW_LOG_CTRL_TYPE type,
	uint8_t *buffer,
	uint32_t size)
{
	uint32_t written = 0;
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	enum connv3_log_type eType = CONNV3_LOG_TYPE_PRIMARY;
#endif

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	switch (type) {
	case ENUM_FW_LOG_CTRL_TYPE_MCU:
		eType = CONNV3_LOG_TYPE_MCU;
		break;
	case ENUM_FW_LOG_CTRL_TYPE_WIFI:
		eType = CONNV3_LOG_TYPE_PRIMARY;
		break;
	default:
		DBGLOG(INIT, ERROR, "invalid type: %d\n",
			type);
		break;
	}
	written = connv3_log_handler(CONNV3_DEBUG_TYPE_WIFI, eType,
		buffer, size);
	if (written == 0)
#if (CFG_TC10_FEATURE == 1)
		if (type != ENUM_FW_LOG_CTRL_TYPE_MCU)
#endif
			DBGLOG(INIT, WARN,
				"[%d] connv3 driver buffer full.\n",
				type);
	else
		DBGLOG(INIT, LOUD,
			"[%d] connv3_log_handler written=%d\n",
			type,
			written);
#endif

	return written;
}

int fw_log_wifi_inf_init(void)
{
	struct fw_log_wifi_interface *prInf = &fw_log_wifi_inf;
	int ret = 0;

	INIT_WORK(&prInf->getFwVerQ, fw_log_get_version_workQ);
	init_waitqueue_head(&prInf->wq);
	sema_init(&prInf->ioctl_mtx, 1);

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	ret = connv3_log_init(CONNLOG_TYPE_WF,
			      RING_BUFFER_SIZE_WF_FW,
			      RING_BUFFER_SIZE_WF_MCU,
			      fw_log_wifi_inf_event_cb);
	if (ret) {
		DBGLOG(INIT, ERROR,
			"connv3_log_init failed, ret: %d\n",
			ret);
		goto return_fn;
	}
#else
	ret = connsys_log_init(CONNLOG_TYPE_WF);
	if (ret) {
		DBGLOG(INIT, ERROR,
			"connsys_log_init failed, ret: %d\n",
			ret);
		goto return_fn;
	}
	connsys_log_register_event_cb(CONNLOG_TYPE_WF,
				      fw_log_wifi_inf_event_cb);
#endif

	ret = alloc_chrdev_region(&prInf->devno, 0, 1,
				  FW_LOG_WIFI_INF_NAME);
	if (ret) {
		DBGLOG(INIT, ERROR,
			"alloc_chrdev_region failed, ret: %d\n",
			ret);
		goto connsys_deinit;
	}

	cdev_init(&prInf->cdev, &fw_log_wifi_fops);
	prInf->cdev.owner = THIS_MODULE;

	ret = cdev_add(&prInf->cdev, prInf->devno, 1);
	if (ret) {
		DBGLOG(INIT, ERROR,
			"cdev_add failed, ret: %d\n",
			ret);
		goto unregister_chrdev_region;
	}

	prInf->driver_class = KAL_CLASS_CREATE(FW_LOG_WIFI_INF_NAME);
	if (IS_ERR(prInf->driver_class)) {
		DBGLOG(INIT, ERROR,
			"class_create failed, ret: %d\n",
			ret);
		ret = PTR_ERR(prInf->driver_class);
		goto cdev_del;
	}

	prInf->class_dev = device_create(prInf->driver_class,
					 NULL, prInf->devno,
					 NULL, FW_LOG_WIFI_INF_NAME);
	if (IS_ERR(prInf->class_dev)) {
		ret = PTR_ERR(prInf->class_dev);
		DBGLOG(INIT, ERROR,
			"class_device_create failed, ret: %d\n",
			ret);
		goto class_destroy;
	}

	goto return_fn;

class_destroy:
	class_destroy(prInf->driver_class);
cdev_del:
	cdev_del(&prInf->cdev);
unregister_chrdev_region:
	unregister_chrdev_region(prInf->devno, 1);
connsys_deinit:
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	connv3_log_deinit(CONNLOG_TYPE_WF);
#else
	connsys_log_register_event_cb(CONNLOG_TYPE_WF, NULL);
	connsys_log_deinit(CONNLOG_TYPE_WF);
#endif
return_fn:
	if (ret)
		DBGLOG(INIT, ERROR, "ret: %d\n",
			ret);

	return ret;
}

void fw_log_wifi_inf_deinit(void)
{
	struct fw_log_wifi_interface *prInf = &fw_log_wifi_inf;

	device_destroy(prInf->driver_class, prInf->devno);
	class_destroy(prInf->driver_class);
	cdev_del(&prInf->cdev);
	unregister_chrdev_region(prInf->devno, 1);

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	connv3_log_deinit(CONNLOG_TYPE_WF);
#else
	connsys_log_register_event_cb(CONNLOG_TYPE_WF, NULL);
	connsys_log_deinit(CONNLOG_TYPE_WF);
#endif
}

