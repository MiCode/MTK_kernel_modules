/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/workqueue.h>
#include "conninfra.h"
#include "conninfra_core.h"
#include "conninfra_test.h"
#include "consys_hw.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#define CONNINFRA_DEV_MAJOR 164
//#define WMT_DETECT_MAJOR 154
#define CONNINFRA_DEV_NUM 1
#define CONNINFRA_DRVIER_NAME "conninfra_drv"
#define CONNINFRA_DEVICE_NAME "conninfra_dev"


/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/delay.h>

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static int conninfra_dev_fb_notifier_callback(struct notifier_block *self,
				unsigned long event, void *data);

static int conninfra_dev_open(struct inode *inode, struct file *file);
static int conninfra_dev_close(struct inode *inode, struct file *file);
static ssize_t conninfra_dev_read(struct file *filp, char __user *buf,
				size_t count, loff_t *f_pos);
static ssize_t conninfra_dev_write(struct file *filp,
				const char __user *buf, size_t count,
				loff_t *f_pos);
/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

struct class *pConninfraClass;
struct device *pConninfraDev;
static struct cdev gConninfraCdev;

const struct file_operations gConninfraDevFops = {
	.open = conninfra_dev_open,
	.release = conninfra_dev_close,
	.read = conninfra_dev_read,
	.write = conninfra_dev_write,
	//.unlocked_ioctl = wmt_detect_unlocked_ioctl,
//#ifdef CONFIG_COMPAT
//	.compat_ioctl = WMT_compat_detect_ioctl,
//#endif
};




static int gConnInfraMajor = CONNINFRA_DEV_MAJOR;

static struct notifier_block conninfra_fb_notifier;
static struct work_struct gPwrOnOffWork;
static atomic_t g_es_lr_flag_for_blank = ATOMIC_INIT(0); /* for ctrl blank flag */

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

int conninfra_dev_open(struct inode *inode, struct file *file)
{
	pr_info("open major %d minor %d (pid %d)\n",
			imajor(inode), iminor(inode), current->pid);

	return 0;
}

int conninfra_dev_close(struct inode *inode, struct file *file)
{
	pr_info("close major %d minor %d (pid %d)\n",
			imajor(inode), iminor(inode), current->pid);

	return 0;
}

ssize_t conninfra_dev_read(struct file *filp, char __user *buf,
					size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t conninfra_dev_write(struct file *filp,
			const char __user *buf, size_t count, loff_t *f_pos)
{
	return 0;
}
static int conninfra_dev_get_blank_state(void)
{
	return atomic_read(&g_es_lr_flag_for_blank);
}

int conninfra_dev_fb_notifier_callback(struct notifier_block *self,
				unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int blank;

	pr_debug("conninfra_dev_fb_notifier_callback\n");

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;
	pr_info("fb_notify(blank=%d)\n", blank);

	switch (blank) {
	case FB_BLANK_UNBLANK:
		atomic_set(&g_es_lr_flag_for_blank, 1);
		pr_info("@@@@@@@@@@ Conninfra enter UNBLANK @@@@@@@@@@@@@@\n");
		schedule_work(&gPwrOnOffWork);
		break;
	case FB_BLANK_POWERDOWN:
		atomic_set(&g_es_lr_flag_for_blank, 0);
		pr_info("@@@@@@@@@@ Conninfra enter early POWERDOWN @@@@@@@@@@@@@@\n");
		schedule_work(&gPwrOnOffWork);
		break;
	default:
		break;
	}
	return 0;
}

static void conninfra_dev_pwr_on_off_handler(struct work_struct *work)
{
	pr_debug("conninfra_dev_pwr_on_off_handler start to run\n");

	/* Update blank off status before wmt power off */
	/*if (conninfra_dev_get_blank_state() == 0) {
	}
	*/

	/* Update blank on status after wmt power on */
	if (conninfra_dev_get_blank_state() == 1) {
		//wmt_dev_blank_handler();
		//connsys_log_blank_state_changed(1);
	}
}

/************************************************************************/

static int conninfra_dev_init(void)
{
	dev_t devID = MKDEV(gConnInfraMajor, 0);
	int cdevErr = -1;
	int iret = 0;

	iret = register_chrdev_region(devID, CONNINFRA_DEV_NUM,
						CONNINFRA_DRVIER_NAME);
	if (iret) {
		pr_err("fail to register chrdev\n");
		return -1;
	}

	cdev_init(&gConninfraCdev, &gConninfraDevFops);
	gConninfraCdev.owner = THIS_MODULE;

	cdevErr = cdev_add(&gConninfraCdev, devID, CONNINFRA_DEV_NUM);
	if (cdevErr) {
		pr_err("cdev_add() fails (%d)\n", cdevErr);
		goto err1;
	}

	pConninfraClass = class_create(THIS_MODULE, CONNINFRA_DEVICE_NAME);
	if (IS_ERR(pConninfraClass)) {
		pr_err("class create fail, error code(%ld)\n",
						PTR_ERR(pConninfraClass));
		goto err1;
	}

	pConninfraDev = device_create(pConninfraClass, NULL, devID,
						NULL, CONNINFRA_DEVICE_NAME);
	if (IS_ERR(pConninfraDev)) {
		pr_err("device create fail, error code(%ld)\n",
						PTR_ERR(pConninfraDev));
		goto err2;
	}

	/* init power on off handler */
	INIT_WORK(&gPwrOnOffWork, conninfra_dev_pwr_on_off_handler);
	conninfra_fb_notifier.notifier_call
				= conninfra_dev_fb_notifier_callback;
	iret = fb_register_client(&conninfra_fb_notifier);
	if (iret) {
		pr_err("register fb_notifier fail");
		return -3;
	} else
		pr_info("register fb_notifier success");

	iret = conninfra_test_setup();
	if (iret)
		pr_err("init conninfra_test fail\n", iret);

	iret = consys_hw_init();
	if (iret) {
		pr_err("init consys_hw fail\n", iret);
		return -4;
	}

	iret = conninfra_core_init();
	if (iret) {
		pr_err("conninfra init fail");
		return -5;
	}


	pr_info("ConnInfra Dev: init (%d)\n", iret);
	return 0;

err2:

	pr_err("[conninfra_dev_init] err2");
	if (pConninfraClass) {
		class_destroy(pConninfraClass);
		pConninfraClass = NULL;
	}
err1:
	pr_err("[conninfra_dev_init] err1");
	if (cdevErr == 0)
		cdev_del(&gConninfraCdev);

	if (iret == 0) {
		unregister_chrdev_region(devID, CONNINFRA_DEV_NUM);
		gConnInfraMajor = -1;
	}

	return -2;
}

static void conninfra_dev_deinit(void)
{
	dev_t dev = MKDEV(gConnInfraMajor, 0);
	int iret = 0;

	fb_unregister_client(&conninfra_fb_notifier);

	iret = conninfra_test_remove();

	iret = conninfra_core_deinit();

	iret = consys_hw_deinit();

	if (pConninfraDev) {
		device_destroy(pConninfraClass, dev);
		pConninfraDev = NULL;
	}

	if (pConninfraClass) {
		class_destroy(pConninfraClass);
		pConninfraClass = NULL;
	}

	cdev_del(&gConninfraCdev);
	unregister_chrdev_region(dev, CONNINFRA_DEV_NUM);

	pr_info("ConnInfra: ALPS platform init (%d)\n", iret);
}

module_init(conninfra_dev_init);
module_exit(conninfra_dev_deinit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Willy.Yu @ CTD/SE5/CS5");

module_param(gConnInfraMajor, uint, 0644);

