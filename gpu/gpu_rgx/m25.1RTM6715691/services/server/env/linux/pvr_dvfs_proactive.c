/*************************************************************************/ /*!
@File           pvr_dvfs_proactive.c
@Title          PowerVR devfreq device common utilities
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Proactive DVFS kernel/OS code
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#if !defined(NO_HARDWARE)

#include <linux/devfreq.h>
#include <linux/version.h>
#include <linux/device.h>
#include <drm/drm.h>
#if defined(CONFIG_PM_OPP)
#include <linux/pm_opp.h>
#endif

#include "pvrsrv.h"

/*
 * Proactive DVFS support code with a devfreq driver wrapper around
 * the FW governor implementation. The devfreq wrapper is agnostic
 * to the underlying algorithm but proactive is anticipated to be
 * the optimal governor.
 */
#include "include/power.h"
#include "pvrsrv.h"
#include "pvrsrv_device.h"

#include "rgxdevice.h"
#include "rgxinit.h"

//#include "syscommon.h"

#include "pvr_dvfs.h"
#include "pvr_dvfs_proactive.h"
#include "pvr_dvfs_common.h"

#include "kernel_compatibility.h"

#if defined(SUPPORT_PDVFS_DEVFREQ)
/* governor.h API for use with firmware-based DVFS */
int devfreq_update_target(struct devfreq *devfreq, unsigned long freq);
#endif

/*************************************************************************/ /*!
@Function       InitPDVFS

@Description    Initialise the device for Proactive DVFS support.
                Prepares the OPP table from the devicetree, if enabled.

@Input          psDeviceNode       Device node
@Return         PVRSRV_ERROR
*/ /**************************************************************************/
#if defined(SUPPORT_PDVFS)
PVRSRV_ERROR InitPDVFS(PPVRSRV_DEVICE_NODE psDeviceNode)
{
#if !(defined(CONFIG_PM_OPP) && defined(CONFIG_OF))
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	return PVRSRV_OK;
#else
	IMG_DVFS_DEVICE_CFG    *psDVFSDeviceCfg = NULL;
	struct device          *psDev;
	int                     err;

	if (!psDeviceNode)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVR_ASSERT(psDeviceNode->psDevConfig);

	psDev = psDeviceNode->psDevConfig->pvOSDevice;
	psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;
	psDeviceNode->psDevConfig->sDVFS.sPDVFSDevice.eState = PVR_DVFS_STATE_INIT_PENDING;

	/* Setup the OPP table from the device tree for Proactive DVFS. */
	err = dev_pm_opp_of_add_table(psDev);
	if (err == 0)
	{
		psDVFSDeviceCfg->bDTConfig = IMG_TRUE;
	}
	else
	{
		/*
		 * If there are no device tree or system layer provided operating points
		 * then return an error
		 */
		if (psDVFSDeviceCfg->pasOPPTable)
		{
			psDVFSDeviceCfg->bDTConfig = IMG_FALSE;
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "No system or device tree opp points found, %d", err));
			return PVRSRV_ERROR_RESOURCE_UNAVAILABLE;
		}
	}
	return PVRSRV_OK;
#endif
}

/*************************************************************************/ /*!
@Function       DeinitPDVFS

@Description    De-Initialise the device for Proactive DVFS support.

@Input          psDeviceNode       Device node
@Return         None
*/ /**************************************************************************/
void DeinitPDVFS(PPVRSRV_DEVICE_NODE psDeviceNode)
{
#if !(defined(CONFIG_PM_OPP) && defined(CONFIG_OF))
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
#else
	IMG_DVFS_DEVICE_CFG *psDVFSDeviceCfg = NULL;
	struct device *psDev = NULL;

	/* Check the device exists */
	if (!psDeviceNode)
	{
		return;
	}

	psDev = psDeviceNode->psDevConfig->pvOSDevice;
	psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;

	if (psDVFSDeviceCfg->bDTConfig)
	{
		/*
		 * Remove OPP entries for this device; only static entries from
		 * the device tree are present.
		 */
		dev_pm_opp_of_remove_table(psDev);
	}
#endif
}

#if defined(SUPPORT_PDVFS_DEVFREQ)
#if !defined(CONFIG_PM_DEVFREQ)
#error "PVR Proactive DVFS governor requires kernel support for devfreq (CONFIG_PM_DEVFREQ = 1)"
#endif

static IMG_INT32 devfreq_target(struct device *dev, unsigned long *requested_freq, IMG_UINT32 flags)
{
	IMG_UINT32		ui32Freq, ui32Volt;
	struct dev_pm_opp *opp;

	/* Target clock freq is calculated in the FW, here we sync the devfreq view of the GPU clock */
	dev_info(dev, "Frequency notification from firmware-based governor: %lu\n", *requested_freq);

	opp = devfreq_recommended_opp(dev, requested_freq, flags);
	if (IS_ERR(opp)) {
		PVR_DPF((PVR_DBG_ERROR, "Invalid OPP"));
		return PTR_ERR(opp);
	}

	ui32Freq = dev_pm_opp_get_freq(opp);
	ui32Volt = dev_pm_opp_get_voltage(opp);
	dev_info(dev, "Requested new voltage %u and freq %u\n", ui32Volt, ui32Freq);

	dev_pm_opp_put(opp);

	return 0;
}

static IMG_INT32 devfreq_cur_freq(struct device *dev, unsigned long *freq)
{
	int deviceId = GetDevID(dev);
	PVRSRV_DEVICE_NODE *psDeviceNode = PVRSRVGetDeviceInstanceByKernelDevID(deviceId);
	RGX_DATA *psRGXData = NULL;

	/* Check the device is registered */
	if (!psDeviceNode)
	{
		return -ENODEV;
	}

	psRGXData = (RGX_DATA*) psDeviceNode->psDevConfig->hDevData;

	/* Check the RGX device is initialised */
	if (!psRGXData)
	{
		return -ENODATA;
	}

	*freq = psRGXData->psRGXTimingInfo->ui32CoreClockSpeed;

	return 0;
}

static struct devfreq_dev_profile img_devfreq_proactive =
{
	.polling_ms         = 10,
	.target             = devfreq_target,
	.get_dev_status     = NULL,		/* not used in the UM governor */
	.get_cur_freq       = devfreq_cur_freq,
};

/*************************************************************************/ /*!
@Function       NotifyCoreClkChange

@Description    Update the userspace devfreq after the firmware-based
                governor has recalculated the core frequency.

@Input          psDeviceNode       Device node
@Input          ui32NewFreq        New GPU clock frequency
@Return         PVRSRV_ERROR
*/ /**************************************************************************/
static PVRSRV_ERROR NotifyCoreClkChange(PPVRSRV_DEVICE_NODE psDeviceNode, IMG_UINT32 ui32NewFreq)
{
	int err = 0;

	IMG_PDVFS_DEVICE *psPDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sPDVFSDevice;

	if (psPDVFSDevice->eState != PVR_DVFS_STATE_READY)
	{
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	/* Update devfreq UM driver */
	mutex_lock(&psPDVFSDevice->psDevFreq->lock);

	err = devfreq_update_target(psPDVFSDevice->psDevFreq, ui32NewFreq);
	if (err)
	{
		pr_err("%s: failed to notify governor %d\n", __func__, err);
	}

	mutex_unlock(&psPDVFSDevice->psDevFreq->lock);

	if (err)
	{
		return TO_IMG_ERR(err);
	}
	return PVRSRV_OK;
}

#if defined(SUPPORT_DVFS_RUNTIME_CONFIG)
static ssize_t capacity_headroom_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	PVRSRV_DEVICE_NODE	*psDeviceNode = PVRSRVGetDeviceInstanceByKernelDevID(
		GetDevID(dev->parent));
	IMG_DVFS_DEVICE_CFG	*psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;

	return scnprintf(buf, PAGE_SIZE, "%d\n",
		psDVFSDeviceCfg->i32CapacityHeadroom);
}

static ssize_t capacity_headroom_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	PVRSRV_DEVICE_NODE	*psDeviceNode = PVRSRVGetDeviceInstanceByKernelDevID(
		GetDevID(dev->parent));
	IMG_DVFS_DEVICE_CFG	*psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;
	IMG_INT32		i32CapacityHeadroom;

	if (kstrtoint(buf, 0, &i32CapacityHeadroom))
		return -EINVAL;
	if (i32CapacityHeadroom < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "Unsupported capacity headroom value %d which should be greater than zero.", i32CapacityHeadroom));
		return -EINVAL;
	}
	psDVFSDeviceCfg->i32CapacityHeadroom = i32CapacityHeadroom;

	return count;
}

static DEVICE_ATTR_RW(capacity_headroom);

static void RegisterHeadroomFile(struct devfreq *devfreq)
{
	int ret = sysfs_create_file(&devfreq->dev.kobj, &dev_attr_capacity_headroom.attr);
	if (ret < 0)
	{
		dev_warn(&devfreq->dev, "Unable to create capacity headroom file");
	}
}

static void UnregisterHeadroomFile(struct devfreq *devfreq)
{
	sysfs_remove_file(&devfreq->dev.kobj, &dev_attr_capacity_headroom.attr);
}
#endif

/*************************************************************************/ /*!
@Function       RegisterPDVFSDevice

@Description    Initialise the device for Proactive DVFS support.
                Prepares the OPP table from the devicetree, if enabled.

@Input          psDeviceNode       Device node
@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RegisterPDVFSDevice(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	IMG_PDVFS_DEVICE       *psPDVFSDevice = NULL;
	IMG_DVFS_DEVICE_CFG    *psDVFSDeviceCfg = NULL;
	RGX_TIMING_INFORMATION *psRGXTimingInfo = NULL;
	struct device          *psDev;
	PVRSRV_ERROR            eError;

	if (!psDeviceNode)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (psDeviceNode->psDevConfig->sDVFS.sPDVFSDevice.eState != PVR_DVFS_STATE_INIT_PENDING)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "Proactive DVFS initialise not yet pending for device node %p",
				 psDeviceNode));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	/*
	 * Create hierarchy for PDVFS device/config.
	 * See pvr_dvfs.h
	 */
	psDev = psDeviceNode->psDevConfig->pvOSDevice;
	psPDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sPDVFSDevice;
	psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;
	psRGXTimingInfo = ((RGX_DATA *)psDeviceNode->psDevConfig->hDevData)->psRGXTimingInfo;
	psDeviceNode->psDevConfig->sDVFS.sPDVFSDevice.eState = PVR_DVFS_STATE_READY;

	/* create the devfreq device */
	psPDVFSDevice->psDevFreq = devm_devfreq_add_device(psDev,
													   &img_devfreq_proactive,
													   DEVFREQ_GOV_USERSPACE,
													   NULL);

	if (IS_ERR(psPDVFSDevice->psDevFreq))
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "Failed to add as devfreq device %p, %ld",
				 psPDVFSDevice->psDevFreq,
				 PTR_ERR(psPDVFSDevice->psDevFreq)));
		eError = TO_IMG_ERR(PTR_ERR(psPDVFSDevice->psDevFreq));
		goto err_exit;
	}

	/*
	 * Register the devfreq userspace governor notification.
	 * Equivalent to entering 'set_freq_store' from userspace.
	 */
	psDVFSDeviceCfg->pfnNotifyCoreClkChange = NotifyCoreClkChange;

#if defined(SUPPORT_DVFS_RUNTIME_CONFIG)
	RegisterHeadroomFile(psPDVFSDevice->psDevFreq);
#endif

	dev_info(psDev, "%s: devfreq device registered.", __func__);

err_exit:
	return PVRSRV_OK;
}

/*************************************************************************/ /*!
@Function       UnregisterPDVFSDevice

@Description    De-Initialise the device for Proactive DVFS support.

@Input          psDeviceNode       Device node
@Return         None
*/ /**************************************************************************/
void UnregisterPDVFSDevice(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	IMG_PDVFS_DEVICE *psPDVFSDevice = NULL;
	struct device *psDev = NULL;
	IMG_INT32 i32Error;

	/* Check the device exists */
	if (!psDeviceNode)
	{
		return;
	}

	PVRSRV_VZ_RETN_IF_MODE(GUEST, DEVNODE, psDeviceNode);

	psPDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sPDVFSDevice;
	psDev = psDeviceNode->psDevConfig->pvOSDevice;

	if (!psPDVFSDevice)
	{
		return;
	}

	if (psPDVFSDevice->psDevFreq)
	{
		i32Error = devfreq_unregister_opp_notifier(psDev, psPDVFSDevice->psDevFreq);
		if (i32Error < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to unregister OPP notifier"));
		}

#if defined(SUPPORT_DVFS_RUNTIME_CONFIG)
		UnregisterHeadroomFile(psPDVFSDevice->psDevFreq);
#endif
		devm_devfreq_remove_device(psDev, psPDVFSDevice->psDevFreq);
		psPDVFSDevice->psDevFreq = NULL;
	}

	psPDVFSDevice->eState = PVR_DVFS_STATE_DEINIT;
}

#endif /* SUPPORT_PDVFS_DEVFREQ */
#endif /* SUPPORT_PDVFS */

#endif /* !NO_HARDWARE */
