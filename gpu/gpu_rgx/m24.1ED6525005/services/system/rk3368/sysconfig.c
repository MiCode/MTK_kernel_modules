/*************************************************************************/ /*!
@File
@Title          System Configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System Configuration functions
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

#include <linux/platform_device.h>

#include "interrupt_support.h"
#include "pvrsrv_device.h"
#include "syscommon.h"
#include "sysconfig.h"
#include "physheap.h"
#if defined(SUPPORT_ION)
#include "ion_support.h"
#endif
#include "rk_init.h"
#include "vz_vmm_pvz.h"

static PHYS_HEAP_FUNCTIONS		gsPhysHeapFuncs;

#define RK3368_SYSTEM_NAME "rk3368"
#define RK3368_NUM_PHYS_HEAPS 2U

/*
	CPU to Device physical address translation
*/
static
void UMAPhysHeapCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
								   IMG_UINT32 ui32NumOfAddr,
								   IMG_DEV_PHYADDR *psDevPAddr,
								   IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psDevPAddr[0].uiAddr = psCpuPAddr[0].uiAddr;
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psDevPAddr[ui32Idx].uiAddr = psCpuPAddr[ui32Idx].uiAddr;
		}
	}
}

/*
	Device to CPU physical address translation
*/
static
void UMAPhysHeapDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
								   IMG_UINT32 ui32NumOfAddr,
								   IMG_CPU_PHYADDR *psCpuPAddr,
								   IMG_DEV_PHYADDR *psDevPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psCpuPAddr[0].uiAddr = psDevPAddr[0].uiAddr;
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psCpuPAddr[ui32Idx].uiAddr = psDevPAddr[ui32Idx].uiAddr;
		}
	}
}

static PVRSRV_ERROR DeviceConfigCreate(void *pvOSDevice,
									   PVRSRV_DEVICE_CONFIG **ppsDevConfigOut)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig;
	RGX_DATA *psRGXData;
	RGX_TIMING_INFORMATION *psRGXTimingInfo;
	PHYS_HEAP_CONFIG *psPhysHeapConfig;

	psDevConfig = OSAllocZMem(sizeof(*psDevConfig) +
							  sizeof(*psRGXData) +
							  sizeof(*psRGXTimingInfo) +
							  sizeof(*psPhysHeapConfig) * RK3368_NUM_PHYS_HEAPS);
	if (!psDevConfig)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psRGXData = (RGX_DATA *)((IMG_CHAR *)psDevConfig + sizeof(*psDevConfig));
	psRGXTimingInfo = (RGX_TIMING_INFORMATION *)((IMG_CHAR *)psRGXData + sizeof(*psRGXData));
	psPhysHeapConfig = (PHYS_HEAP_CONFIG *)((IMG_CHAR *)psRGXTimingInfo + sizeof(*psRGXTimingInfo));

	/* Set up the RGX timing information */
	psRGXTimingInfo->ui32CoreClockSpeed = RGX_RK_CORE_CLOCK_SPEED;
	psRGXTimingInfo->bEnableActivePM = IMG_TRUE;
	psRGXTimingInfo->bEnableRDPowIsland = IMG_FALSE;
	psRGXTimingInfo->ui32ActivePMLatencyms = SYS_RGX_ACTIVE_POWER_LATENCY_MS;

	/* Set up the RGX data */
	psRGXData->psRGXTimingInfo = psRGXTimingInfo;

	psPhysHeapConfig[0].eType = PHYS_HEAP_TYPE_UMA;
	psPhysHeapConfig[0].ui32UsageFlags = PHYS_HEAP_USAGE_GPU_LOCAL;
	psPhysHeapConfig[0].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM";
	psPhysHeapConfig[0].uConfig.sUMA.psMemFuncs = &gsPhysHeapFuncs;
	psPhysHeapConfig[0].uConfig.sUMA.pszHeapName = "uma_gpu_local";
	psPhysHeapConfig[0].uConfig.sUMA.hPrivData = NULL;

	psPhysHeapConfig[1].eType = PHYS_HEAP_TYPE_UMA;
	psPhysHeapConfig[1].ui32UsageFlags = PHYS_HEAP_USAGE_FW_SHARED;
	psPhysHeapConfig[1].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM";
	psPhysHeapConfig[1].uConfig.sUMA.psMemFuncs = &gsPhysHeapFuncs;
	psPhysHeapConfig[1].uConfig.sUMA.pszHeapName = "uma_fw_main";
	psPhysHeapConfig[1].uConfig.sUMA.hPrivData = NULL;

	psDevConfig->pasPhysHeaps = psPhysHeapConfig;
	psDevConfig->ui32PhysHeapCount = RK3368_NUM_PHYS_HEAPS;

	psDevConfig->pvOSDevice = pvOSDevice;
	psDevConfig->pszName = RK3368_SYSTEM_NAME;
	psDevConfig->pszVersion = NULL;

	psDevConfig->eDefaultHeap = PVRSRV_PHYS_HEAP_GPU_LOCAL;

	psDevConfig->bHasFBCDCVersion31 = IMG_FALSE;
	psDevConfig->bDevicePA0IsValid = IMG_FALSE;

	psDevConfig->hDevData = psRGXData;

	psDevConfig->pfnSysDevFeatureDepInit = NULL;

	/* power management on HW system */
	psDevConfig->pfnPrePowerState = RkPrePowerState;
	psDevConfig->pfnPostPowerState = RkPostPowerState;

	/* clock frequency */
	psDevConfig->pfnClockFreqGet = NULL;

	/* device error notify callback function */
	psDevConfig->pfnSysDevErrorNotify = NULL;

	psDevConfig->eCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_EMULATED;

	*ppsDevConfigOut = psDevConfig;

	return PVRSRV_OK;
}

static void DeviceConfigDestroy(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	/*
	 * The device config, RGX data and RGX timing info are part of the same
	 * allocation so do only one free.
	 */
	OSFreeMem(psDevConfig);
}

PVRSRV_ERROR SysDevInit(void *pvOSDevice, PVRSRV_DEVICE_CONFIG **ppsDevConfig)
{
	int iIrq;
	struct resource *psDevMemRes = NULL;
	struct platform_device *psDev;
	PVRSRV_DEVICE_CONFIG *psDevConfig;
	PVRSRV_ERROR eError;

	psDev = to_platform_device((struct device *)pvOSDevice);

	/*
	 * Setup information about physical memory heap(s) we have
	 */
	gsPhysHeapFuncs.pfnCpuPAddrToDevPAddr = UMAPhysHeapCpuPAddrToDevPAddr;
	gsPhysHeapFuncs.pfnDevPAddrToCpuPAddr = UMAPhysHeapDevPAddrToCpuPAddr;

	/* Device setup information */
	eError = DeviceConfigCreate(pvOSDevice, &psDevConfig);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	psDevMemRes = platform_get_resource(psDev, IORESOURCE_MEM, 0);
	if (psDevMemRes)
	{
		psDevConfig->sRegsCpuPBase.uiAddr = psDevMemRes->start;
		psDevConfig->ui32RegsSize         = (unsigned int)(psDevMemRes->end - psDevMemRes->start);
	}
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: platform_get_resource failed", __func__));
		psDevConfig->sRegsCpuPBase.uiAddr = RK_GPU_PBASE;
		psDevConfig->ui32RegsSize         = RK_GPU_SIZE;
	}

	iIrq = platform_get_irq(psDev, 0);
	if (iIrq >= 0)
	{
		psDevConfig->ui32IRQ  = (IMG_UINT32) iIrq;
	}
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: platform_get_irq failed (%d)", __func__, -iIrq));
		psDevConfig->ui32IRQ = RK_IRQ_GPU;
	}

	/* Rk Init */
	psDevConfig->hSysData = (IMG_HANDLE)RgxRkInit(psDevConfig);
	if (!psDevConfig->hSysData)
	{
		DeviceConfigDestroy(psDevConfig);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* Setup other system specific stuff */
#if defined(SUPPORT_ION)
	IonInit(NULL);
#endif

	*ppsDevConfig = psDevConfig;

	return PVRSRV_OK;
}

void SysDevDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);

	/* Rk UnInit */
	RgxRkUnInit(psDevConfig->hSysData);

#if defined(SUPPORT_ION)
	IonDeinit();
#endif

	DeviceConfigDestroy(psDevConfig);
}

PVRSRV_ERROR SysInstallDeviceLISR(IMG_HANDLE hSysData,
								  IMG_UINT32 ui32IRQ,
								  const IMG_CHAR *pszName,
								  PFN_LISR pfnLISR,
								  void *pvData,
								  IMG_HANDLE *phLISRData)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);
	return OSInstallSystemLISR(phLISRData, ui32IRQ, pszName, pfnLISR, pvData,
							   SYS_IRQ_FLAG_TRIGGER_DEFAULT);
}

PVRSRV_ERROR SysUninstallDeviceLISR(IMG_HANDLE hLISRData)
{
	return OSUninstallSystemLISR(hLISRData);
}

PVRSRV_ERROR SysDebugInfo(PVRSRV_DEVICE_CONFIG *psDevConfig,
						  DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
						  void *pvDumpDebugFile)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);
	PVR_UNREFERENCED_PARAMETER(pfnDumpDebugPrintf);
	PVR_UNREFERENCED_PARAMETER(pvDumpDebugFile);

	return PVRSRV_OK;
}

/******************************************************************************
 End of file (sysconfig.c)
******************************************************************************/
