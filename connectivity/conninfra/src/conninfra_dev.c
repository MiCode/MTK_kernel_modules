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
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/mm.h>
#include "mtk_disp_notify.h"
#else
#include <linux/fb.h>
#endif
#include <linux/workqueue.h>
#include "conninfra.h"
#include "conninfra_conf.h"
#include "conninfra_core.h"
#include "conninfra_dbg.h"
#include "consys_hw.h"
#include "connsys_debug_utility.h"
#include "wmt_build_in_adapter.h"
#include "emi_mng.h"

#ifdef MTK_WCN_REMOVE_KERNEL_MODULE
#include "conn_drv_init.h"
#endif
#ifdef CFG_CONNINFRA_UT_SUPPORT
#include "conninfra_test.h"
#endif

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
#include <devapc_public.h>
#endif

#include <linux/thermal.h>
#include "conn_power_throttling.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#define CONNINFRA_DEV_MAJOR 164
#define CONNINFRA_DEV_NUM 1
#define CONNINFRA_DRVIER_NAME "conninfra_drv"
#define CONNINFRA_DEVICE_NAME "conninfra_dev"

#define CONNINFRA_DEV_IOC_MAGIC 0xc2
#define CONNINFRA_IOCTL_GET_CHIP_ID _IOR(CONNINFRA_DEV_IOC_MAGIC, 0, int)
#define CONNINFRA_IOCTL_SET_COREDUMP_MODE _IOW(CONNINFRA_DEV_IOC_MAGIC, 1, unsigned int)
#define CONNINFRA_IOCTL_DO_MODULE_INIT _IOR(CONNINFRA_DEV_IOC_MAGIC, 2, int)
#define CONNINFRA_IOCTL_GET_ADIE_CHIP_ID _IOR(CONNINFRA_DEV_IOC_MAGIC, 3, int)

#define CONNINFRA_DEV_INIT_TO_MS (2 * 1000)

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0) \
	&& LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
#define SSPM_DEBUG_DUMP
#endif

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/delay.h>

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

enum conninfra_init_status {
	CONNINFRA_INIT_NOT_START,
	CONNINFRA_INIT_START,
	CONNINFRA_INIT_DONE,
};
static int g_conninfra_init_status = CONNINFRA_INIT_NOT_START;
static wait_queue_head_t g_conninfra_init_wq;

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
struct conninfra_pmic_work {
	unsigned int id;
	unsigned int event;
	struct work_struct pmic_work;
};

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
static long conninfra_dev_unlocked_ioctl(
		struct file *filp, unsigned int cmd, unsigned long arg);
#ifdef CONFIG_COMPAT
static long conninfra_dev_compat_ioctl(
		struct file *filp, unsigned int cmd, unsigned long arg);
#endif
static int conninfra_mmap(struct file *pFile, struct vm_area_struct *pVma);
static int conninfra_thermal_query_cb(void);
static void conninfra_clock_fail_dump_cb(void);

static int conninfra_conn_reg_readable(void);
static int conninfra_conn_is_bus_hang(void);

#ifdef SSPM_DEBUG_DUMP
static int conninfra_conn_bus_dump(void);
#endif

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
static void conninfra_devapc_violation_cb(void);
static void conninfra_register_devapc_callback(void);
#endif

static int conninfra_dev_suspend_cb(void);
static int conninfra_dev_resume_cb(void);
static int conninfra_dev_pmic_event_cb(unsigned int, unsigned int);

static int conninfra_dev_do_drv_init(void);
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
	.unlocked_ioctl = conninfra_dev_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = conninfra_dev_compat_ioctl,
#endif
	.mmap = conninfra_mmap,
};

struct wmt_platform_bridge g_plat_bridge = {
	.thermal_query_cb = conninfra_thermal_query_cb,
	.clock_fail_dump_cb  = conninfra_clock_fail_dump_cb,
	.conninfra_reg_readable_cb = conninfra_conn_reg_readable,
#ifdef SSPM_DEBUG_DUMP
	.conninfra_reg_is_bus_hang_cb = conninfra_conn_is_bus_hang,
	.conninfra_reg_is_bus_hang_no_lock_cb = conninfra_conn_bus_dump,
#else
	.conninfra_reg_is_bus_hang_cb = conninfra_conn_is_bus_hang,
#endif
	.debug_write_cb = conninfra_dbg_write,
	.debug_read_cb = conninfra_dbg_read,
};


static int gConnInfraMajor = CONNINFRA_DEV_MAJOR;

/* screen on/off notification */
static struct notifier_block conninfra_fb_notifier;
static struct work_struct gPwrOnOffWork;
static atomic_t g_es_lr_flag_for_blank = ATOMIC_INIT(0); /* for ctrl blank flag */
static int last_thermal_value;
static int g_temp_thermal_value;
#if IS_ENABLED(CONFIG_MTK_DEVAPC)
/* For DEVAPC callback */
static struct work_struct g_conninfra_devapc_work;
static struct devapc_vio_callbacks conninfra_devapc_handle = {
	.id = INFRA_SUBSYS_CONN,
	.debug_dump = conninfra_devapc_violation_cb,
};
#endif
/* For PMIC callback */
static struct conninfra_pmic_work g_conninfra_pmic_work;

static struct conninfra_dev_cb g_conninfra_dev_cb = {
	.conninfra_suspend_cb = conninfra_dev_suspend_cb,
	.conninfra_resume_cb = conninfra_dev_resume_cb,
	.conninfra_pmic_event_notifier = conninfra_dev_pmic_event_cb,
};

static int conninfra_thermal_get_temp_cb(void *data, int *temp);
static const struct thermal_zone_of_device_ops tz_conninfra_thermal_ops = {
	.get_temp = conninfra_thermal_get_temp_cb,
};


/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

int conninfra_dev_open(struct inode *inode, struct file *file)
{
#ifdef MTK_WCN_REMOVE_KERNEL_MODULE
	pr_info("[%s] built-in mode, allow to open", __func__);
#else
	static DEFINE_RATELIMIT_STATE(_rs, HZ, 1);

	if (!wait_event_timeout(
		g_conninfra_init_wq,
		g_conninfra_init_status == CONNINFRA_INIT_DONE,
		msecs_to_jiffies(CONNINFRA_DEV_INIT_TO_MS))) {
		if (__ratelimit(&_rs)) {
			pr_warn("wait_event_timeout (%d)ms,(%lu)jiffies,return -EIO\n",
			        CONNINFRA_DEV_INIT_TO_MS, msecs_to_jiffies(CONNINFRA_DEV_INIT_TO_MS));
		}
		return -EIO;
	}
#endif
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

static long conninfra_dev_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
#ifdef MTK_WCN_REMOVE_KERNEL_MODULE
	static DEFINE_RATELIMIT_STATE(_rs, HZ, 1);
#endif
	int retval = 0;

	pr_info("[%s] cmd (%d),arg(%ld)\n", __func__, cmd, arg);

	/* Special process for module init command */
	if (cmd == CONNINFRA_IOCTL_DO_MODULE_INIT) {
	#ifdef MTK_WCN_REMOVE_KERNEL_MODULE
		retval = conninfra_dev_do_drv_init();
		return retval;
	#else
		pr_info("[%s] KO mode", __func__);
		return 0;
	#endif
	}

#ifdef MTK_WCN_REMOVE_KERNEL_MODULE
	if (!wait_event_timeout(
		g_conninfra_init_wq,
		g_conninfra_init_status == CONNINFRA_INIT_DONE,
		msecs_to_jiffies(CONNINFRA_DEV_INIT_TO_MS))) {
		if (__ratelimit(&_rs)) {
			pr_warn("wait_event_timeout (%d)ms,(%lu)jiffies,return -EIO\n",
			        CONNINFRA_DEV_INIT_TO_MS, msecs_to_jiffies(CONNINFRA_DEV_INIT_TO_MS));
		}
		return -EIO;
	}
#endif

	switch (cmd) {
	case CONNINFRA_IOCTL_GET_CHIP_ID:
		retval = consys_hw_chipid_get();
		break;
	case CONNINFRA_IOCTL_SET_COREDUMP_MODE:
		connsys_coredump_set_dump_mode(arg);
		break;
	case CONNINFRA_IOCTL_GET_ADIE_CHIP_ID:
		retval = consys_hw_detect_adie_chipid();
		break;
	}

	return retval;

}

#ifdef CONFIG_COMPAT
static long conninfra_dev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;

	pr_info("[%s] cmd (%d)\n", __func__, cmd);
	ret = conninfra_dev_unlocked_ioctl(filp, cmd, arg);
	return ret;
}
#endif

static int conninfra_mmap(struct file *pFile, struct vm_area_struct *pVma)
{
	unsigned long bufId = pVma->vm_pgoff;
	struct consys_emi_addr_info* addr_info = emi_mng_get_phy_addr();

	pr_info("conninfra_mmap start:%lu end:%lu size: %lu buffer id=%lu\n",
		pVma->vm_start, pVma->vm_end,
		pVma->vm_end - pVma->vm_start, bufId);

	if (bufId == 0) {
		if (pVma->vm_end - pVma->vm_start > addr_info->emi_size)
			return -EINVAL;
		pr_info("conninfra_mmap size: %lu\n", pVma->vm_end - pVma->vm_start);
		if (remap_pfn_range(pVma, pVma->vm_start, addr_info->emi_ap_phy_addr >> PAGE_SHIFT,
			pVma->vm_end - pVma->vm_start, pVma->vm_page_prot))
			return -EAGAIN;
		return 0;
	} else if (bufId == 1) {
		if (addr_info == NULL)
			return -EINVAL;
		if (addr_info->md_emi_size == 0 ||
		    pVma->vm_end - pVma->vm_start > addr_info->md_emi_size)
			return -EINVAL;
		pr_info("MD direct path size=%u map size=%lu\n",
			addr_info->md_emi_size,
			pVma->vm_end - pVma->vm_start);
		if (remap_pfn_range(pVma, pVma->vm_start,
			addr_info->md_emi_phy_addr >> PAGE_SHIFT,
			pVma->vm_end - pVma->vm_start, pVma->vm_page_prot))
			return -EAGAIN;
		return 0;
	}
	/* Invalid bufId */
	return -EINVAL;
}

static int conninfra_dev_get_blank_state(void)
{
	return atomic_read(&g_es_lr_flag_for_blank);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int conninfra_dev_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *v)
{
	int *data = (int *)v;

	if (event != MTK_DISP_EVENT_BLANK) {
		return 0;
	}

	pr_info("%s+\n", __func__);

	if (*data == MTK_DISP_BLANK_UNBLANK) {
		atomic_set(&g_es_lr_flag_for_blank, 1);
		pr_info("@@@@@@@@@@ Conninfra enter UNBLANK @@@@@@@@@@@@@@\n");
		schedule_work(&gPwrOnOffWork);
	} else if (*data == MTK_DISP_BLANK_POWERDOWN) {
		atomic_set(&g_es_lr_flag_for_blank, 0);
		pr_info("@@@@@@@@@@ Conninfra enter early POWERDOWN @@@@@@@@@@@@@@\n");
		schedule_work(&gPwrOnOffWork);
	}

	pr_info("%s-\n", __func__);

	return 0;
}
#else
int conninfra_dev_fb_notifier_callback(struct notifier_block *self,
				unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int blank;

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;

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
#endif

static void conninfra_dev_pwr_on_off_handler(struct work_struct *work)
{
	/* Update blank on status after wmt power on */
	if (conninfra_dev_get_blank_state() == 1) {
		conninfra_core_screen_on();
	} else {
		conninfra_core_screen_off();
	}
}

static int conninfra_thermal_query_cb(void)
{
	int ret;

	/* if rst is ongoing, return thermal val got from last time */
	if (conninfra_core_is_rst_locking()) {
		pr_info("[%s] rst is locking, return last temp ", __func__);
		return last_thermal_value;
	}
	pr_info("[%s] query thermal", __func__);
	ret = conninfra_core_thermal_query(&g_temp_thermal_value);
	if (ret == 0)
		last_thermal_value = g_temp_thermal_value;
	else if (ret == CONNINFRA_ERR_WAKEUP_FAIL)
		conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_CONNINFRA, "Query thermal wakeup fail");

	return last_thermal_value;
}

/* for Linux thermal framework */
static int conninfra_thermal_get_temp_cb(void *data, int *temp)
{
	int ret;

	if (!temp)
		return 0;

	/* if rst is ongoing, return THERMAL_TEMP_INVALID */
	if (conninfra_core_is_rst_locking()) {
		pr_info("[%s] rst is locking, return invalid value ", __func__);
		*temp = THERMAL_TEMP_INVALID;
		return 0;
	}

	ret = conninfra_core_thermal_query(&g_temp_thermal_value);
	if (ret == 0)
		*temp = g_temp_thermal_value * 1000;
	else if (ret == CONNINFRA_ERR_WAKEUP_FAIL) {
		conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_CONNINFRA, "Query thermal wakeup fail");
		*temp = THERMAL_TEMP_INVALID;
	} else
		*temp = THERMAL_TEMP_INVALID;

	return 0;
}

static void conninfra_register_thermal_callback(void)
{
	struct thermal_zone_device *tz;
	struct platform_device *pdev = get_consys_device();
	int ret;

	if (!pdev) {
		pr_info("get_consys_device return NULL\n");
		return;
	}

	/* register thermal zone */
	tz = devm_thermal_zone_of_sensor_register(
		&pdev->dev, 0, NULL, &tz_conninfra_thermal_ops);

	if (IS_ERR(tz)) {
		ret = PTR_ERR(tz);
		pr_info("Failed to register thermal zone device %d\n", ret);
		return;
	}
	pr_info("Register thermal zone device.\n");
}

static void conninfra_clock_fail_dump_cb(void)
{
	conninfra_core_clock_fail_dump_cb();
}


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/*  THIS FUNCTION IS ONLY FOR AUDIO DRIVER                 */
/* this function go through consys_hw, skip conninfra_core */
/* there is no lock and skip consys power on check         */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
static int conninfra_conn_reg_readable(void)
{
	return consys_hw_reg_readable();
	/*return conninfra_core_reg_readable(); */
}

static int conninfra_conn_is_bus_hang(void)
{
	/* if rst is ongoing, don't dump */
	if (conninfra_core_is_rst_locking()) {
		pr_info("[%s] rst is locking, skip dump", __func__);
		return CONNINFRA_ERR_RST_ONGOING;
	}
	return conninfra_core_is_bus_hang();
}

/* For sspm debug dump
 * To dump connsys bus status when sspm ipi timeout
 */
#ifdef SSPM_DEBUG_DUMP
static int conninfra_conn_bus_dump(void)
{
	return conninfra_core_conn_bus_dump();
}
#endif

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
static void conninfra_devapc_violation_cb(void)
{
	schedule_work(&g_conninfra_devapc_work);
}

static void conninfra_devapc_handler(struct work_struct *work)
{
	conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_CONNINFRA, "CONNSYS DEVAPC");
}

static void conninfra_register_devapc_callback(void)
{
	INIT_WORK(&g_conninfra_devapc_work, conninfra_devapc_handler);
	register_devapc_vio_callback(&conninfra_devapc_handle);
}
#endif

static int conninfra_dev_suspend_cb(void)
{
	connsys_dedicated_log_set_ap_state(0);
	conninfra_core_reset_power_state();
	return 0;
}

static int conninfra_dev_resume_cb(void)
{
	conninfra_core_dump_power_state(NULL, 0);
	connsys_dedicated_log_set_ap_state(1);
	return 0;
}

static int conninfra_dev_pmic_event_cb(unsigned int id, unsigned int event)
{
	g_conninfra_pmic_work.id = id;
	g_conninfra_pmic_work.event = event;
	schedule_work(&g_conninfra_pmic_work.pmic_work);

	return 0;
}

static void conninfra_dev_pmic_event_handler(struct work_struct *work)
{
	unsigned int id, event;
	struct conninfra_pmic_work *pmic_work =
		container_of(work, struct conninfra_pmic_work, pmic_work);

	if (pmic_work) {
		id = pmic_work->id;
		event = pmic_work->event;
		conninfra_core_pmic_event_cb(id, event);
	} else
		pr_info("[%s] pmic_work is null", __func__);

}

static void conninfra_register_pmic_callback(void)
{
	INIT_WORK(&g_conninfra_pmic_work.pmic_work, conninfra_dev_pmic_event_handler);
}

static void conninfra_register_power_throttling_callback(void)
{
	struct conn_pwr_plat_info pwr_info;
	int ret;

	pwr_info.chip_id = consys_hw_chipid_get();
	pwr_info.adie_id = consys_hw_detect_adie_chipid();
	pwr_info.get_temp = conninfra_core_thermal_query;
	ret = conn_pwr_init(&pwr_info);
	if (ret < 0)
		pr_info("conn_pwr_init is failed %d.", ret);
}

/************************************************************************/
static int conninfra_dev_do_drv_init()
{
	static int init_done = 0;
	int iret = 0;

	if (init_done) {
		pr_info("%s already init, return.", __func__);
		return 0;
	}
	init_done = 1;

#ifdef CFG_CONNINFRA_UT_SUPPORT
	iret = conninfra_test_setup();
	if (iret)
		pr_err("init conninfra_test fail, ret = %d\n", iret);
#endif

	iret = conninfra_conf_init();
	if (iret)
		pr_warn("init conf fail\n");

	iret = consys_hw_init(&g_conninfra_dev_cb);
	if (iret) {
		pr_err("init consys_hw fail, ret = %d\n", iret);
		g_conninfra_init_status = CONNINFRA_INIT_NOT_START;
		return -2;
	}

	iret = conninfra_core_init();
	if (iret) {
		pr_err("conninfra init fail");
		g_conninfra_init_status = CONNINFRA_INIT_NOT_START;
		return -3;
	}

	conninfra_dev_dbg_init();

	wmt_export_platform_bridge_register(&g_plat_bridge);

	/* init power on off handler */
	INIT_WORK(&gPwrOnOffWork, conninfra_dev_pwr_on_off_handler);
	conninfra_fb_notifier.notifier_call
				= conninfra_dev_fb_notifier_callback;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#if IS_ENABLED(CONFIG_DRM_MEDIATEK)
	iret = mtk_disp_notifier_register("conninfra_driver", &conninfra_fb_notifier);
#endif
#else
	iret = fb_register_client(&conninfra_fb_notifier);
#endif
	if (iret)
		pr_info("register fb_notifier fail");
	else
		pr_info("register fb_notifier success");

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
	conninfra_register_devapc_callback();
#endif
	conninfra_register_pmic_callback();
	conninfra_register_thermal_callback();
	conninfra_register_power_throttling_callback();

	pr_info("ConnInfra Dev: init (%d)\n", iret);
	g_conninfra_init_status = CONNINFRA_INIT_DONE;

#ifdef MTK_WCN_REMOVE_KERNEL_MODULE
	iret = (int)consys_hw_chipid_get();
	iret = do_connectivity_driver_init(iret);
	if (iret)
		pr_err("Sub driver init fail, iret=%d", iret);
#endif

	return 0;

}


static int conninfra_dev_init(void)
{
	dev_t devID = MKDEV(gConnInfraMajor, 0);
	int cdevErr = -1;
	int iret = 0;

	g_conninfra_init_status = CONNINFRA_INIT_START;
	init_waitqueue_head((wait_queue_head_t *)&g_conninfra_init_wq);

	iret = register_chrdev_region(devID, CONNINFRA_DEV_NUM,
						CONNINFRA_DRVIER_NAME);
	if (iret) {
		pr_err("fail to register chrdev\n");
		g_conninfra_init_status = CONNINFRA_INIT_NOT_START;
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
#ifndef MTK_WCN_REMOVE_KERNEL_MODULE
	iret = conninfra_dev_do_drv_init();
	if (iret) {
		pr_err("conninfra_do_drv_init fail, iret = %d", iret);
		return iret;
	}
#endif
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

	g_conninfra_init_status = CONNINFRA_INIT_NOT_START;
	return -2;
}

static void conninfra_dev_deinit(void)
{
	dev_t dev = MKDEV(gConnInfraMajor, 0);
	int iret = 0;

	g_conninfra_init_status = CONNINFRA_INIT_NOT_START;

	conn_pwr_deinit();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#if IS_ENABLED(CONFIG_DRM_MEDIATEK)
	mtk_disp_notifier_unregister(&conninfra_fb_notifier);
#endif
#else
	fb_unregister_client(&conninfra_fb_notifier);
#endif

	iret = conninfra_dev_dbg_deinit();
#ifdef CFG_CONNINFRA_UT_SUPPORT
	iret = conninfra_test_remove();
#endif

	iret = conninfra_core_deinit();

	iret = consys_hw_deinit();

	iret = conninfra_conf_deinit();

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

