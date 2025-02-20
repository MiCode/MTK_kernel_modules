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

#include "pvrsrv.h"
#include "pvrsrv_device.h"
#include "syscommon.h"
#include "vz_vmm_pvz.h"
#include "allocmem.h"
#include "sysinfo.h"
#include "sysconfig.h"
#include "physheap.h"
#include "pvr_debug.h"
#if defined(SUPPORT_ION)
#include "ion_support.h"
#endif
#if defined(__linux__)
#include <linux/dma-mapping.h>
#endif

#if defined(SUPPORT_VALIDATION) && defined(PDUMP)
#include "validation_soc.h"
#endif

/*
 * In systems that support trusted device address protection, there are three
 * physical heaps from which pages should be allocated:
 * - one heap for normal allocations
 * - one heap for allocations holding META code memory
 * - one heap for allocations holding secured DRM data
 */

#define PHYS_HEAP_IDX_GENERAL     0
#define PHYS_HEAP_IDX_FW          1

#if defined(SUPPORT_TRUSTED_DEVICE)
#define PHYS_HEAP_IDX_TDFWMEM     2
#define PHYS_HEAP_IDX_TDSECUREBUF 3
#elif defined(SUPPORT_DEDICATED_FW_MEMORY)
#define PHYS_HEAP_IDX_FW_MEMORY   2
#endif

/* Change to test CPU_LOCAL sys layers*/
#define UMA_HEAP_USAGE_FLAG PHYS_HEAP_USAGE_GPU_LOCAL
//#define UMA_HEAP_USAGE_FLAG PHYS_HEAP_USAGE_CPU_LOCAL

#define UMA_DEFAULT_HEAP PVRSRV_PHYS_HEAP_GPU_LOCAL
//#define UMA_DEFAULT_HEAP PVRSRV_PHYS_HEAP_CPU_LOCAL

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

static PHYS_HEAP_FUNCTIONS gsPhysHeapFuncs =
{
	/* pfnCpuPAddrToDevPAddr */
	UMAPhysHeapCpuPAddrToDevPAddr,
	/* pfnDevPAddrToCpuPAddr */
	UMAPhysHeapDevPAddrToCpuPAddr,
};

static PVRSRV_ERROR PhysHeapsCreate(PHYS_HEAP_CONFIG **ppasPhysHeapsOut,
									IMG_UINT32 *puiPhysHeapCountOut)
{
	/*
	 * This function is called during device initialisation, which on Linux,
	 * means it won't be called concurrently. As such, there's no need to
	 * protect it with a lock or use an atomic variable.
	 */
	PHYS_HEAP_CONFIG *pasPhysHeaps;
	IMG_UINT32 uiHeapCount = 2;

#if defined(SUPPORT_TRUSTED_DEVICE)
	uiHeapCount += 2;
#elif defined(SUPPORT_DEDICATED_FW_MEMORY)
	uiHeapCount += 1;
#endif

	pasPhysHeaps = OSAllocZMem(sizeof(*pasPhysHeaps) * uiHeapCount);
	if (!pasPhysHeaps)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].eType = PHYS_HEAP_TYPE_UMA;
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].ui32UsageFlags = UMA_HEAP_USAGE_FLAG;
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM";
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].uConfig.sUMA.psMemFuncs = &gsPhysHeapFuncs;
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].uConfig.sUMA.pszHeapName = "uma_local";
#if defined(PVRSRV_INTERNAL_IPA_FEATURE_TESTING)
	/* Provide a specific set of Intermediate Physical Address modifiers
	 * for the default heap. Also name this as "SYSMEM_IPA" to aid PDump
	 * processing.
	 * Modifiers are:
	 *  IPAPolicyDefault = 3
	 *  IPAPolicyMask = 7
	 *  IPAPolicyShift = 52
	 * This will result in the PhysAddr values having bits 53..55 set with '3'
	 * by default.
	 */
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM_IPA";
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].sIPAConfig.ui8IPAPolicyDefault = 3;
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].sIPAConfig.ui8IPAPolicyMask = 7;
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].sIPAConfig.ui8IPAPolicyShift = 52;
#endif

	 pasPhysHeaps[PHYS_HEAP_IDX_FW].eType = PHYS_HEAP_TYPE_UMA;
	 pasPhysHeaps[PHYS_HEAP_IDX_FW].ui32UsageFlags = PHYS_HEAP_USAGE_FW_SHARED;
	 pasPhysHeaps[PHYS_HEAP_IDX_FW].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM_FW";
	 pasPhysHeaps[PHYS_HEAP_IDX_FW].uConfig.sUMA.psMemFuncs = &gsPhysHeapFuncs;
	 pasPhysHeaps[PHYS_HEAP_IDX_FW].uConfig.sUMA.pszHeapName = "uma_fw_shared";


#if defined(SUPPORT_TRUSTED_DEVICE)
	pasPhysHeaps[PHYS_HEAP_IDX_TDFWMEM].eType = PHYS_HEAP_TYPE_UMA;
	pasPhysHeaps[PHYS_HEAP_IDX_TDFWMEM].ui32UsageFlags =
		PHYS_HEAP_USAGE_FW_CODE | PHYS_HEAP_USAGE_FW_PRIV_DATA;
	pasPhysHeaps[PHYS_HEAP_IDX_TDFWMEM].uConfig.sUMA.pszPDumpMemspaceName = "TDFWMEM";
	pasPhysHeaps[PHYS_HEAP_IDX_TDFWMEM].uConfig.sUMA.psMemFuncs = &gsPhysHeapFuncs;
	pasPhysHeaps[PHYS_HEAP_IDX_TDFWMEM].uConfig.sUMA.pszHeapName = "uma_tdfwmem";

	pasPhysHeaps[PHYS_HEAP_IDX_TDSECUREBUF].eType = PHYS_HEAP_TYPE_UMA;
	pasPhysHeaps[PHYS_HEAP_IDX_TDSECUREBUF].ui32UsageFlags = PHYS_HEAP_USAGE_GPU_SECURE;
	pasPhysHeaps[PHYS_HEAP_IDX_TDSECUREBUF].uConfig.sUMA.pszPDumpMemspaceName = "TDSECBUFMEM";
	pasPhysHeaps[PHYS_HEAP_IDX_TDSECUREBUF].uConfig.sUMA.psMemFuncs = &gsPhysHeapFuncs;
	pasPhysHeaps[PHYS_HEAP_IDX_TDSECUREBUF].uConfig.sUMA.pszHeapName = "uma_tdsecbufmem";

#elif defined(SUPPORT_DEDICATED_FW_MEMORY)
	pasPhysHeaps[PHYS_HEAP_IDX_FW_MEMORY].eType = PHYS_HEAP_TYPE_UMA;
	pasPhysHeaps[PHYS_HEAP_IDX_FW_MEMORY].ui32UsageFlags =
		PHYS_HEAP_USAGE_FW_CODE | PHYS_HEAP_USAGE_FW_PRIV_DATA;
	pasPhysHeaps[PHYS_HEAP_IDX_FW_MEMORY].uConfig.sUMA.pszPDumpMemspaceName = "DEDICATEDFWMEM";
	pasPhysHeaps[PHYS_HEAP_IDX_FW_MEMORY].uConfig.sUMA.psMemFuncs = &gsPhysHeapFuncs;
	pasPhysHeaps[PHYS_HEAP_IDX_FW_MEMORY].uConfig.sUMA.pszHeapName = "uma_dedicatedfwmem";
#endif

	*ppasPhysHeapsOut = pasPhysHeaps;
	*puiPhysHeapCountOut = uiHeapCount;

	return PVRSRV_OK;
}

static void PhysHeapsDestroy(PHYS_HEAP_CONFIG *pasPhysHeaps)
{
	OSFreeMem(pasPhysHeaps);
}

static void SysDevFeatureDepInit(PVRSRV_DEVICE_CONFIG *psDevConfig, IMG_UINT64 ui64Features)
{
#if defined(SUPPORT_AXI_ACE_TEST)
		if ( ui64Features & RGX_FEATURE_AXI_ACELITE_BIT_MASK)
		{
			psDevConfig->eCacheSnoopingMode		= PVRSRV_DEVICE_SNOOP_CPU_ONLY;
		}else
#endif
		{
			psDevConfig->eCacheSnoopingMode		= PVRSRV_DEVICE_SNOOP_NONE;
		}
}

PVRSRV_ERROR SysDevInit(void *pvOSDevice, PVRSRV_DEVICE_CONFIG **ppsDevConfig)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig;
	RGX_DATA *psRGXData;
	RGX_TIMING_INFORMATION *psRGXTimingInfo;
	PHYS_HEAP_CONFIG *pasPhysHeaps;
	IMG_UINT32 uiPhysHeapCount;
	PVRSRV_ERROR eError;

#if defined(__linux__)
	dma_set_mask(pvOSDevice, DMA_BIT_MASK(40));
#endif

	psDevConfig = OSAllocZMem(sizeof(*psDevConfig) +
							  sizeof(*psRGXData) +
							  sizeof(*psRGXTimingInfo));
	if (!psDevConfig)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psRGXData = (RGX_DATA *)((IMG_CHAR *)psDevConfig + sizeof(*psDevConfig));
	psRGXTimingInfo = (RGX_TIMING_INFORMATION *)((IMG_CHAR *)psRGXData + sizeof(*psRGXData));

	eError = PhysHeapsCreate(&pasPhysHeaps, &uiPhysHeapCount);
	if (eError)
	{
		goto ErrorFreeDevConfig;
	}

	/* Setup RGX specific timing data */
	psRGXTimingInfo->ui32CoreClockSpeed        = RGX_NOHW_CORE_CLOCK_SPEED;
	psRGXTimingInfo->bEnableActivePM           = IMG_FALSE;
	psRGXTimingInfo->bEnableRDPowIsland        = IMG_FALSE;
	psRGXTimingInfo->ui32ActivePMLatencyms     = SYS_RGX_ACTIVE_POWER_LATENCY_MS;

	/* Set up the RGX data */
	psRGXData->psRGXTimingInfo = psRGXTimingInfo;

	/* Setup the device config */
	psDevConfig->pvOSDevice				= pvOSDevice;
	psDevConfig->pszName                = "nohw";
	psDevConfig->pszVersion             = NULL;

	/* Device setup information */
	psDevConfig->sRegsCpuPBase.uiAddr   = 0x00f00000;
	psDevConfig->ui32RegsSize           = 0x100000;
	psDevConfig->ui32IRQ                = 0x00000bad;

	psDevConfig->pasPhysHeaps			= pasPhysHeaps;
	psDevConfig->ui32PhysHeapCount		= uiPhysHeapCount;
	psDevConfig->eDefaultHeap = UMA_DEFAULT_HEAP;

	/* No power management on no HW system */
	psDevConfig->pfnPrePowerState       = NULL;
	psDevConfig->pfnPostPowerState      = NULL;

	/* No clock frequency either */
	psDevConfig->pfnClockFreqGet        = NULL;

	psDevConfig->hDevData               = psRGXData;

	psDevConfig->bDevicePA0IsValid       = IMG_FALSE;
	psDevConfig->pfnSysDevFeatureDepInit = SysDevFeatureDepInit;

	/* Setup other system specific stuff */
#if defined(SUPPORT_ION)
	IonInit(NULL);
#endif

	/* Pdump validation system registers */
#if defined(SUPPORT_VALIDATION) && defined(PDUMP) && defined(SUPPORT_VOLCANIC_TB)
	{
		/* 'Magic' cookie used in this file only, where no psDeviceNode is available
		 * but writing to the PDump log should be permitted
		 */
		#define PDUMP_MAGIC_COOKIE 0x9E0FF

		PVRSRVConfigureSysCtrl((PVRSRV_DEVICE_NODE*)PDUMP_MAGIC_COOKIE, NULL, PDUMP_FLAGS_CONTINUOUS);
#if defined(SUPPORT_SECURITY_VALIDATION)
		PVRSRVConfigureTrustedDevice((PVRSRV_DEVICE_NODE*)PDUMP_MAGIC_COOKIE, NULL, PDUMP_FLAGS_CONTINUOUS);
#endif
	}
#endif

#if !defined(HW_ERN_66622) && !defined(VOLCANIC_FBCDCV31)
	psDevConfig->bHasFBCDCVersion31 = IMG_FALSE;
#else
	psDevConfig->bHasFBCDCVersion31 = IMG_TRUE;
#endif

	/* Set psDevConfig->pfnSysDevErrorNotify callback */
	psDevConfig->pfnSysDevErrorNotify = SysRGXErrorNotify;

	*ppsDevConfig = psDevConfig;

	return PVRSRV_OK;

ErrorFreeDevConfig:
	OSFreeMem(psDevConfig);
	return eError;
}

void SysDevDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
#if defined(SUPPORT_ION)
	IonDeinit();
#endif

	PhysHeapsDestroy(psDevConfig->pasPhysHeaps);
	OSFreeMem(psDevConfig);
}

PVRSRV_ERROR SysInstallDeviceLISR(IMG_HANDLE hSysData,
								  IMG_UINT32 ui32IRQ,
								  const IMG_CHAR *pszName,
								  PFN_LISR pfnLISR,
								  void *pvData,
								  IMG_HANDLE *phLISRData)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);
	PVR_UNREFERENCED_PARAMETER(ui32IRQ);
	PVR_UNREFERENCED_PARAMETER(pszName);
	PVR_UNREFERENCED_PARAMETER(pfnLISR);
	PVR_UNREFERENCED_PARAMETER(pvData);
	PVR_UNREFERENCED_PARAMETER(phLISRData);

	return PVRSRV_OK;
}

PVRSRV_ERROR SysUninstallDeviceLISR(IMG_HANDLE hLISRData)
{
	PVR_UNREFERENCED_PARAMETER(hLISRData);

	return PVRSRV_OK;
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
