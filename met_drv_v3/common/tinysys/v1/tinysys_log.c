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

#ifdef MET_SSPM
#include "sspm_reservedmem.h"
#include "sspm_reservedmem_define.h"
#include "sspm/sspm_met_log.h"
#endif

#ifdef MET_MCUPM
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
#ifdef MET_TINYSYS
static void _reset(int status, int type);
#endif


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
#ifdef MET_SSPM
static unsigned int _g_sspm_status;
#endif

#ifdef MET_MCUPM
static unsigned int _g_mcupm_status;
#endif

struct device *tinysys_log_manager_fsdevice;

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

	tinysys_log_manager_fsdevice = dev;

	return 0;
}

#ifdef MET_SSPM
int met_sspm_log_init(void)
{
	if (!(met_sspm_api_ready && met_scmi_api_ready))
		return -1;
	return sspm_log_init(tinysys_log_manager_fsdevice);
}
EXPORT_SYMBOL(met_sspm_log_init);
#endif

#ifdef MET_MCUPM
int met_mcupm_log_init(void)
{
	if (!(met_mcupm_api_ready && met_ipi_api_ready))
		return -1;
	return mcupm_log_init(tinysys_log_manager_fsdevice);
}
EXPORT_SYMBOL(met_mcupm_log_init);
#endif

int tinysys_log_manager_uninit(struct device *dev)
{
#ifdef ONDIEMET_MOUNT_DEBUGFS
	struct dentry *dbgfs_met_dir = NULL;
#else
	struct proc_dir_entry *procfs_met_dir = NULL;
#endif

	tinysys_log_manager_fsdevice = NULL;

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

#ifdef MET_SSPM
int met_sspm_log_uninit(void)
{
	if (!(met_sspm_api_ready && met_scmi_api_ready))
		return -1;
	return sspm_log_uninit(tinysys_log_manager_fsdevice);
}
EXPORT_SYMBOL(met_sspm_log_uninit);
#endif

#ifdef MET_MCUPM
int met_mcupm_log_uninit(void)
{
	if (!(met_mcupm_api_ready && met_ipi_api_ready))
		return -1;
	return mcupm_log_uninit(tinysys_log_manager_fsdevice);
}
EXPORT_SYMBOL(met_mcupm_log_uninit);
#endif

int tinysys_log_manager_start(void)
{
#ifdef MET_SSPM
	if (met_sspm_api_ready && met_scmi_api_ready) {
		sspm_log_start();
		_reset(kTINYSYS_LOG_START, ONDIEMET_SSPM);
	}
#endif

#ifdef MET_MCUPM
	if (met_mcupm_api_ready && met_ipi_api_ready) {
		mcupm_log_start();
		_reset(kTINYSYS_LOG_START, ONDIEMET_MCUPM);
	}
#endif

	return 0;
}


int tinysys_log_manager_stop(void)
{
#ifdef MET_SSPM
	if ((ondiemet_module[ONDIEMET_SSPM] == 0) || (sspm_buffer_size == -1)) {
		if (met_sspm_api_ready && met_scmi_api_ready) {
			if (!ondiemet_record_check[ONDIEMET_SSPM]) {
				sspm_log_stop();
				_reset(kTINYSYS_LOG_STOP, ONDIEMET_SSPM);
			}
		}
	}
#endif

#ifdef MET_MCUPM
	if ((ondiemet_module[ONDIEMET_MCUPM] == 0) || (mcupm_buffer_size == -1)) {
		if (met_mcupm_api_ready && met_ipi_api_ready) {
			if (!ondiemet_record_check[ONDIEMET_MCUPM]) {
				mcupm_log_stop();
				_reset(kTINYSYS_LOG_STOP, ONDIEMET_MCUPM);
			}
		}
	}
#endif

	return 0;
}


/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
#ifdef MET_TINYSYS
static void _reset(int status, int type)
{
	switch (type) {
#ifdef MET_SSPM
	case ONDIEMET_SSPM:
		_g_sspm_status = status;
		break;
#endif

#ifdef MET_MCUPM
	case ONDIEMET_MCUPM:
		_g_mcupm_status = status;
		break;
#endif

	default:
		return;
	}
}
#endif

