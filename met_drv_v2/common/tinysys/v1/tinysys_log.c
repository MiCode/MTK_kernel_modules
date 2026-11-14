// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/module.h>   /* symbol_get */
#include <linux/jiffies.h> /* timespec_to_jiffies */
#include <linux/uaccess.h>  /* copy_to_user */
#include <asm/io.h>  /* for ioremap and iounmap */

#include <linux/miscdevice.h>
#include "interface.h"
#include "core_plf_init.h"

#include "tinysys_mgr.h"
#include "tinysys_log.h"

#if FEATURE_SSPM_NUM
#include "sspm_reservedmem.h"
#include "sspm_reservedmem_define.h"
#include "sspm/sspm_met_log.h"
#endif

#if FEATURE_MCUPM_NUM
#include "mcupm_driver.h"
#include "mcupm/mcupm_met_log.h"
#endif


/*****************************************************************************
 * define declaration
 *****************************************************************************/
#define IPI_COMMOND_SIZE	4


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/
enum {
	kTINYSYS_LOG_START,
	kTINYSYS_LOG_STOP,
	kTINYSYS_LOG_IDLE,
};


/*****************************************************************************
 * external function declaration
 *****************************************************************************/


/*****************************************************************************
 * internal function declaration
 *****************************************************************************/
static void _reset(int status, int type);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
#if FEATURE_SSPM_NUM
static unsigned int _g_sspm_status;
#endif

#if FEATURE_MCUPM_NUM
static unsigned int _g_mcupm_status;
#endif


/*****************************************************************************
 * external function ipmlement
 *****************************************************************************/
int tinysys_log_manager_init(struct device *dev)
{
#ifdef ONDIEMET_MOUNT_DEBUGFS
	struct dentry *dbgfs_met_dir = NULL;

	dbgfs_met_dir = debugfs_create_dir("ondiemet", NULL);
	if (dbgfs_met_dir == NULL) {
		PR_BOOTMSG("[MET] can not create debugfs directory: ondiemet\n");
		return -ENOMEM;
	}
	dev_set_drvdata(dev, dbgfs_met_dir);
#else
	struct proc_dir_entry *procfs_met_dir = NULL;

	procfs_met_dir = proc_mkdir("ondiemet", NULL);
	if (procfs_met_dir == NULL) {
		PR_BOOTMSG("[MET] can not create procfs directory: ondiemet\n");
		return -ENOMEM;
	}
	dev_set_drvdata(dev, procfs_met_dir);
#endif

#if FEATURE_SSPM_NUM
	sspm_log_init(dev);
#endif

#if FEATURE_MCUPM_NUM
	mcupm_log_init(dev);
#endif

	return 0;
}


int tinysys_log_manager_uninit(struct device *dev)
{
#ifdef ONDIEMET_MOUNT_DEBUGFS
	struct dentry *dbgfs_met_dir = NULL;
#else
	struct proc_dir_entry *procfs_met_dir = NULL;
#endif

#if FEATURE_SSPM_NUM
	sspm_log_uninit(dev);
#endif

#if FEATURE_MCUPM_NUM
	mcupm_log_uninit(dev);
#endif

#ifdef ONDIEMET_MOUNT_DEBUGFS
	dbgfs_met_dir = dev_get_drvdata(dev);
	if (dbgfs_met_dir) {
		debugfs_remove_recursive(dbgfs_met_dir);
		dev_set_drvdata(dev, met_device.this_device);
	}
#else
	procfs_met_dir = dev_get_drvdata(dev);
	if (procfs_met_dir) {
		proc_remove(procfs_met_dir);
		dev_set_drvdata(dev, met_device.this_device);
	}
#endif

	return 0;
}


int tinysys_log_manager_start(void)
{
#if FEATURE_SSPM_NUM
	sspm_log_start();
	_reset(kTINYSYS_LOG_START, ONDIEMET_SSPM);
#endif

#if FEATURE_MCUPM_NUM
	mcupm_log_start();
	_reset(kTINYSYS_LOG_START, ONDIEMET_MCUPM);
#endif

	return 0;
}


int tinysys_log_manager_stop(void)
{
#if FEATURE_SSPM_NUM
	sspm_log_stop();
	_reset(kTINYSYS_LOG_STOP, ONDIEMET_SSPM);
#endif

#if FEATURE_MCUPM_NUM
	mcupm_log_stop();
	_reset(kTINYSYS_LOG_STOP, ONDIEMET_MCUPM);
#endif

	return 0;
}


/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
static void _reset(int status, int type)
{
	switch (type) {
#if FEATURE_SSPM_NUM
	case ONDIEMET_SSPM:
		_g_sspm_status = status;
		break;
#endif

#if FEATURE_MCUPM_NUM
	case ONDIEMET_MCUPM:
		_g_mcupm_status = status;
		break;
#endif

	default:
		return;
	}
}

