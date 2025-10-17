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

#include <linux/version.h>

#include "sysinfo.h"
#include "apollo_regs.h"

#include "pvrsrv.h"
#include "pvrsrv_device.h"
#include "rgxdevice.h"
#include "syscommon.h"
#include "allocmem.h"
#include "pvr_debug.h"

#if defined(SUPPORT_ION)
#include PVR_ANDROID_ION_HEADER
#include "ion_support.h"
#include "ion_sys.h"
#endif

#include "tc_drv.h"

#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#define SECURE_FW_MEM_SIZE (0x400000)  /*  4MB */
#define SECURE_MEM_SIZE    (0x4000000) /* 64MB */

typedef struct
{
	PHYS_HEAP_USAGE_FLAGS ui32UsageFlags;
	IMG_UINT64 uiSize;
	IMG_BOOL bUsed;
} CARD_PHYS_HEAP_CONFIG_SPEC;

#define HEAP_SPEC_IDX_GPU_PRIVATE (0U)
#define HEAP_SPEC_IDX_GPU_LOCAL   (1U)

static const CARD_PHYS_HEAP_CONFIG_SPEC gasCardHeapTemplate[] =
{
	{
	 PHYS_HEAP_USAGE_GPU_PRIVATE,
	 0,					/* determined at runtime by apphints */
	 false				/* determined at runtime by apphints */
	},
	{
	 PHYS_HEAP_USAGE_GPU_LOCAL,
	 0,					/* determined at runtime */
	 true
	},
	{
	 PHYS_HEAP_USAGE_GPU_SECURE,
	 SECURE_MEM_SIZE,
#if defined(SUPPORT_SECURITY_VALIDATION)
	 true
#else
	 false
#endif
	},
	{
	 PHYS_HEAP_USAGE_FW_PRIVATE,
	 SECURE_FW_MEM_SIZE,
#if defined(SUPPORT_SECURITY_VALIDATION)
	 true
#else
	 false
#endif
	},
	{
	 PHYS_HEAP_USAGE_FW_SHARED,
#if defined(SUPPORT_SECURITY_VALIDATION) && defined(RGX_PREMAP_FW_HEAPS)
	 /* simultaneous virtualisation and security support requires premapped heaps,
	  * i.e. FW_PRIVATE and FW_SHARED must fit contiguously into Fw's VA heap (RGX_FIRMWARE_RAW_HEAP_SIZE) */
	 RGX_FIRMWARE_RAW_HEAP_SIZE - SECURE_FW_MEM_SIZE,
#else
	 RGX_FIRMWARE_RAW_HEAP_SIZE,
#endif
#if defined(RGX_PREMAP_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED > 1)
	true				/* VZ drivers need dedicated Fw heaps */
#else
	false				/* Native drivers can fallback on GPU_LOCAL for Fw mem */
#endif
	},
	{
	 PHYS_HEAP_USAGE_FW_PREMAP_PT,
	 RGX_FIRMWARE_MAX_PAGETABLE_SIZE,
#if defined(RGX_PREMAP_FW_HEAPS)
	 true
#else
	 false
#endif
	}
};

#define ODIN_MEMORY_HYBRID_DEVICE_BASE 0x400000000

#define SYS_RGX_ACTIVE_POWER_LATENCY_MS (10)

#define UI64_TOPWORD_IS_ZERO(ui64) ((ui64 >> 32) == 0)

#if defined(SUPPORT_LINUX_DVFS) || defined(SUPPORT_PDVFS)

/* Fake DVFS configuration used purely for testing purposes */

static const IMG_OPP asOPPTable[] =
{
	{ 8,  25000000},
	{ 16, 50000000},
	{ 32, 75000000},
	{ 64, 100000000},
};

#define LEVEL_COUNT (sizeof(asOPPTable) / sizeof(IMG_OPP))

static void SetFrequency(IMG_HANDLE hSysData, IMG_UINT32 ui32Frequency)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);

	PVR_DPF((PVR_DBG_ERROR, "SetFrequency %u", ui32Frequency));
}

static void SetVoltage(IMG_HANDLE hSysData, IMG_UINT32 ui32Voltage)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);

	PVR_DPF((PVR_DBG_ERROR, "SetVoltage %u", ui32Voltage));
}

#endif

static void TCLocalCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
				      IMG_UINT32 ui32NumOfAddr,
				      IMG_DEV_PHYADDR *psDevPAddr,
				      IMG_CPU_PHYADDR *psCpuPAddr);

static void TCLocalDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
				      IMG_UINT32 ui32NumOfAddr,
				      IMG_CPU_PHYADDR *psCpuPAddr,
				      IMG_DEV_PHYADDR *psDevPAddr);

static PHYS_HEAP_FUNCTIONS gsLocalPhysHeapFuncs =
{
	.pfnCpuPAddrToDevPAddr = TCLocalCpuPAddrToDevPAddr,
	.pfnDevPAddrToCpuPAddr = TCLocalDevPAddrToCpuPAddr,
};

static void TCHostCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
									 IMG_UINT32 ui32NumOfAddr,
									 IMG_DEV_PHYADDR *psDevPAddr,
									 IMG_CPU_PHYADDR *psCpuPAddr);

static void TCHostDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
									 IMG_UINT32 ui32NumOfAddr,
									 IMG_CPU_PHYADDR *psCpuPAddr,
									 IMG_DEV_PHYADDR *psDevPAddr);

static PHYS_HEAP_FUNCTIONS gsHostPhysHeapFuncs =
{
	.pfnCpuPAddrToDevPAddr = TCHostCpuPAddrToDevPAddr,
	.pfnDevPAddrToCpuPAddr = TCHostDevPAddrToCpuPAddr,
};

static void TCHybridCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
                                       IMG_UINT32 ui32NumOfAddr,
                                       IMG_DEV_PHYADDR *psDevPAddr,
                                       IMG_CPU_PHYADDR *psCpuPAddr);

static void TCHybridDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
                                       IMG_UINT32 ui32NumOfAddr,
                                       IMG_CPU_PHYADDR *psCpuPAddr,
                                       IMG_DEV_PHYADDR *psDevPAddr);

static PHYS_HEAP_FUNCTIONS gsHybridPhysHeapFuncs =
{
	.pfnCpuPAddrToDevPAddr = TCHybridCpuPAddrToDevPAddr,
	.pfnDevPAddrToCpuPAddr = TCHybridDevPAddrToCpuPAddr
};

typedef struct _SYS_DATA_ SYS_DATA;

struct _SYS_DATA_
{
	struct platform_device *pdev;

	struct tc_rogue_platform_data *pdata;

	struct resource *registers;

#if defined(SUPPORT_ION) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
	struct ion_client *ion_client;
	struct ion_handle *ion_rogue_allocation;
#endif

#if defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__)
	PVRSRV_DEVICE_CONFIG *psDevConfig;

	PHYS_HEAP_ITERATOR *psHeapIter;
	void *pvS3Buffer;
#endif /* defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__) */
};

#define SYSTEM_INFO_FORMAT_STRING "FPGA Revision: %s - TCF Core Revision: %s - TCF Core Target Build ID: %s - PCI Version: %s - Macro Version: %s"
#define FPGA_REV_MAX_LEN      8 /* current longest format: "x.y.z" */
#define TCF_CORE_REV_MAX_LEN  8 /* current longest format: "x.y.z" */
#define TCF_CORE_CFG_MAX_LEN  4 /* current longest format: "x" */
#define PCI_VERSION_MAX_LEN   4 /* current longest format: "x" */
#define MACRO_VERSION_MAX_LEN 8 /* current longest format: "x.yz" */

static IMG_CHAR *GetDeviceVersionString(SYS_DATA *psSysData)
{
	int err;
	char str_fpga_rev[FPGA_REV_MAX_LEN]={0};
	char str_tcf_core_rev[TCF_CORE_REV_MAX_LEN]={0};
	char str_tcf_core_target_build_id[TCF_CORE_CFG_MAX_LEN]={0};
	char str_pci_ver[PCI_VERSION_MAX_LEN]={0};
	char str_macro_ver[MACRO_VERSION_MAX_LEN]={0};

	IMG_CHAR *pszVersion;
	IMG_UINT32 ui32StringLength;

	err = tc_sys_strings(psSysData->pdev->dev.parent,
							 str_fpga_rev, sizeof(str_fpga_rev),
							 str_tcf_core_rev, sizeof(str_tcf_core_rev),
							 str_tcf_core_target_build_id, sizeof(str_tcf_core_target_build_id),
							 str_pci_ver, sizeof(str_pci_ver),
							 str_macro_ver, sizeof(str_macro_ver));
	if (err)
	{
		return NULL;
	}

	/* Calculate how much space we need to allocate for the string */
	ui32StringLength = OSStringLength(SYSTEM_INFO_FORMAT_STRING);
	ui32StringLength += OSStringLength(str_fpga_rev);
	ui32StringLength += OSStringLength(str_tcf_core_rev);
	ui32StringLength += OSStringLength(str_tcf_core_target_build_id);
	ui32StringLength += OSStringLength(str_pci_ver);
	ui32StringLength += OSStringLength(str_macro_ver);

	/* Create the version string */
	pszVersion = OSAllocMem(ui32StringLength * sizeof(IMG_CHAR));
	if (pszVersion)
	{
		OSSNPrintf(&pszVersion[0], ui32StringLength,
				   SYSTEM_INFO_FORMAT_STRING,
				   str_fpga_rev,
				   str_tcf_core_rev,
				   str_tcf_core_target_build_id,
				   str_pci_ver,
				   str_macro_ver);
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: failed to create format string", __func__));
	}

	return pszVersion;
}

#if defined(SUPPORT_ION) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
static SYS_DATA *gpsIonPrivateData;

PVRSRV_ERROR IonInit(void *pvPrivateData)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	SYS_DATA *psSysData = pvPrivateData;
	gpsIonPrivateData = psSysData;

	psSysData->ion_client = ion_client_create(psSysData->pdata->ion_device, SYS_RGX_DEV_NAME);
	if (IS_ERR(psSysData->ion_client))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create ION client (%ld)", __func__, PTR_ERR(psSysData->ion_client)));
		eError = PVRSRV_ERROR_ION_NO_CLIENT;
		goto err_out;
	}
	/* Allocate the whole rogue ion heap and pass that to services to manage */
	psSysData->ion_rogue_allocation = ion_alloc(psSysData->ion_client, psSysData->pdata->rogue_heap_memory_size, 4096, (1 << psSysData->pdata->ion_heap_id), 0);
	if (IS_ERR(psSysData->ion_rogue_allocation))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate ION rogue buffer (%ld)", __func__, PTR_ERR(psSysData->ion_rogue_allocation)));
		eError = PVRSRV_ERROR_ION_FAILED_TO_ALLOC;
		goto err_destroy_client;

	}

	return PVRSRV_OK;
err_destroy_client:
	ion_client_destroy(psSysData->ion_client);
	psSysData->ion_client = NULL;
err_out:
	return eError;
}

void IonDeinit(void)
{
	SYS_DATA *psSysData = gpsIonPrivateData;
	ion_free(psSysData->ion_client, psSysData->ion_rogue_allocation);
	psSysData->ion_rogue_allocation = NULL;
	ion_client_destroy(psSysData->ion_client);
	psSysData->ion_client = NULL;
}

struct ion_device *IonDevAcquire(void)
{
	return gpsIonPrivateData->pdata->ion_device;
}

void IonDevRelease(struct ion_device *ion_device)
{
	PVR_ASSERT(ion_device == gpsIonPrivateData->pdata->ion_device);
}
#endif /* defined(SUPPORT_ION) */

static void TCLocalCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
				      IMG_UINT32 ui32NumOfAddr,
				      IMG_DEV_PHYADDR *psDevPAddr,
				      IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig = (PVRSRV_DEVICE_CONFIG *)hPrivData;
	SYS_DATA *psSysData = psDevConfig->hSysData;
	IMG_UINT32 ui32Idx;

	for (ui32Idx = 0; ui32Idx < ui32NumOfAddr; ui32Idx++)
	{
		psDevPAddr[ui32Idx].uiAddr =
			psCpuPAddr[ui32Idx].uiAddr - psSysData->pdata->tc_memory_base;
	}
}

static void TCLocalDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
				      IMG_UINT32 ui32NumOfAddr,
				      IMG_CPU_PHYADDR *psCpuPAddr,
				      IMG_DEV_PHYADDR *psDevPAddr)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig = (PVRSRV_DEVICE_CONFIG *)hPrivData;
	SYS_DATA *psSysData = psDevConfig->hSysData;
	IMG_UINT32 ui32Idx;

	for (ui32Idx = 0; ui32Idx < ui32NumOfAddr; ui32Idx++)
	{
		psCpuPAddr[ui32Idx].uiAddr =
			psDevPAddr[ui32Idx].uiAddr + psSysData->pdata->tc_memory_base;
	}
}

static void TCHostCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
									 IMG_UINT32 uiNumOfAddr,
									 IMG_DEV_PHYADDR *psDevPAddr,
									 IMG_CPU_PHYADDR *psCpuPAddr)
{
	if (sizeof(*psDevPAddr) == sizeof(*psCpuPAddr))
	{
		OSCachedMemCopy(psDevPAddr, psCpuPAddr, uiNumOfAddr * sizeof(*psDevPAddr));
		return;
	}

	/* In this case we may have a 32bit host, so we can't do a memcpy */
	/* Optimise common case */
	psDevPAddr[0].uiAddr = psCpuPAddr[0].uiAddr;
	if (uiNumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < uiNumOfAddr; ++ui32Idx)
		{
			psDevPAddr[ui32Idx].uiAddr = psCpuPAddr[ui32Idx].uiAddr;
		}
	}
}

static void TCHostDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
									 IMG_UINT32 uiNumOfAddr,
									 IMG_CPU_PHYADDR *psCpuPAddr,
									 IMG_DEV_PHYADDR *psDevPAddr)
{
	if (sizeof(*psCpuPAddr) == sizeof(*psDevPAddr))
	{
		OSCachedMemCopy(psCpuPAddr, psDevPAddr, uiNumOfAddr * sizeof(*psCpuPAddr));
		return;
	}

	/* In this case we may have a 32bit host, so we can't do a memcpy.
	 * Check we are not dropping any data from the 64bit dev addr */
	PVR_ASSERT(UI64_TOPWORD_IS_ZERO(psDevPAddr[0].uiAddr));
	/* Optimise common case */
	psCpuPAddr[0].uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(psDevPAddr[0].uiAddr);
	if (uiNumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < uiNumOfAddr; ++ui32Idx)
		{
			PVR_ASSERT(UI64_TOPWORD_IS_ZERO(psDevPAddr[ui32Idx].uiAddr));
			psCpuPAddr[ui32Idx].uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(psDevPAddr[ui32Idx].uiAddr);
		}
	}

}

static inline
IMG_CHAR* GetHeapName(PHYS_HEAP_USAGE_FLAGS ui32Flags)
{
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_GPU_LOCAL))    return "lma_gpu_local";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_GPU_SECURE))   return "lma_gpu_secure";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_GPU_PRIVATE))  return "lma_gpu_private";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_FW_PRIVATE))   return "lma_fw_private";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_FW_SHARED))    return "lma_fw_shared";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_FW_PREMAP_PT)) return "lma_fw_pagetables";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_CPU_LOCAL))    return "lma_cpu_local";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_DISPLAY))      return "lma_gpu_display";
	else                                                     return "Unexpected Heap";
}

static void TCHybridCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
                                       IMG_UINT32 ui32NumOfAddr,
                                       IMG_DEV_PHYADDR *psDevPAddr,
                                       IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig = (PVRSRV_DEVICE_CONFIG *)hPrivData;
	SYS_DATA *psSysData = psDevConfig->hSysData;
	IMG_UINT32 ui32Idx;

	for (ui32Idx = 0; ui32Idx < ui32NumOfAddr; ui32Idx++)
	{
		psDevPAddr[ui32Idx].uiAddr =
		    (psCpuPAddr[ui32Idx].uiAddr - psSysData->pdata->tc_memory_base) +
		    ODIN_MEMORY_HYBRID_DEVICE_BASE;
	}
}

static void TCHybridDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
                                       IMG_UINT32 ui32NumOfAddr,
                                       IMG_CPU_PHYADDR *psCpuPAddr,
                                       IMG_DEV_PHYADDR *psDevPAddr)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig = (PVRSRV_DEVICE_CONFIG *)hPrivData;
	SYS_DATA *psSysData = psDevConfig->hSysData;
	IMG_UINT32 ui32Idx;

	for (ui32Idx = 0; ui32Idx < ui32NumOfAddr; ui32Idx++)
	{
		psCpuPAddr[ui32Idx].uiAddr =
		    (psDevPAddr[ui32Idx].uiAddr - ODIN_MEMORY_HYBRID_DEVICE_BASE) +
		    psSysData->pdata->tc_memory_base;
	}
}

static PVRSRV_ERROR
InitLocalHeap(PHYS_HEAP_CONFIG *psPhysHeap,
			  IMG_UINT64 uiBaseAddr, IMG_UINT64 uiStartAddr,
			  IMG_UINT64 uiSize, PHYS_HEAP_FUNCTIONS *psFuncs,
			  PHYS_HEAP_USAGE_FLAGS ui32Flags)
{
	psPhysHeap->eType = PHYS_HEAP_TYPE_LMA;
	psPhysHeap->ui32UsageFlags = ui32Flags;
	psPhysHeap->uConfig.sLMA.pszPDumpMemspaceName = "LMA";
	psPhysHeap->uConfig.sLMA.psMemFuncs = psFuncs;
	psPhysHeap->uConfig.sLMA.pszHeapName = GetHeapName(ui32Flags);
	psPhysHeap->uConfig.sLMA.sCardBase.uiAddr = uiBaseAddr;
	psPhysHeap->uConfig.sLMA.sStartAddr.uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(uiStartAddr);
	psPhysHeap->uConfig.sLMA.uiSize = uiSize;

	return PVRSRV_OK;
}

static PVRSRV_ERROR
CreateCardGPUHeaps(const SYS_DATA *psSysData,
				   CARD_PHYS_HEAP_CONFIG_SPEC *pasCardHeapSpec,
				   PHYS_HEAP_CONFIG *pasPhysHeaps,
				   PHYS_HEAP_FUNCTIONS *psHeapFuncs,
				   IMG_UINT32 *pui32HeapIdx,
				   IMG_UINT64 ui64CardAddr)
{
	PVRSRV_ERROR eError;
	IMG_UINT64 ui64StartAddr = psSysData->pdata->rogue_heap_memory_base;
	IMG_UINT32 ui32SpecIdx;

	for (ui32SpecIdx = 0; ui32SpecIdx < ARRAY_SIZE(gasCardHeapTemplate); ui32SpecIdx++)
	{
		if (pasCardHeapSpec[ui32SpecIdx].bUsed)
		{
			IMG_UINT64 ui64HeapSize = pasCardHeapSpec[ui32SpecIdx].uiSize;

			eError = InitLocalHeap(&pasPhysHeaps[*pui32HeapIdx],
								   ui64CardAddr,
								   IMG_CAST_TO_CPUPHYADDR_UINT(ui64StartAddr),
								   ui64HeapSize,
								   psHeapFuncs,
								   pasCardHeapSpec[ui32SpecIdx].ui32UsageFlags);
			if (eError != PVRSRV_OK)
			{
				return eError;
			}

			ui64CardAddr  += pasCardHeapSpec[ui32SpecIdx].uiSize;
			ui64StartAddr += pasCardHeapSpec[ui32SpecIdx].uiSize;
			(*pui32HeapIdx)++;
		}
	}

	return PVRSRV_OK;
}

#if TC_DISPLAY_MEM_SIZE != 0
static PVRSRV_ERROR
CreateCardEXTHeap(const SYS_DATA *psSysData,
				  PHYS_HEAP_CONFIG *pasPhysHeaps,
				  PHYS_HEAP_FUNCTIONS *psHeapFuncs,
				  IMG_UINT32 *pui32HeapIdx,
				  IMG_UINT64 ui64CardBase)
{
	IMG_UINT64 ui64StartAddr = psSysData->pdata->pdp_heap_memory_base;
	IMG_UINT64 ui64Size = psSysData->pdata->pdp_heap_memory_size;
	PVRSRV_ERROR eError;

	eError = InitLocalHeap(&pasPhysHeaps[*pui32HeapIdx],
						   ui64CardBase + psSysData->pdata->rogue_heap_memory_size,
						   IMG_CAST_TO_CPUPHYADDR_UINT(ui64StartAddr),
						   ui64Size, psHeapFuncs,
						   PHYS_HEAP_USAGE_EXTERNAL | PHYS_HEAP_USAGE_DISPLAY);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	(*pui32HeapIdx)++;

	return PVRSRV_OK;
}
#endif

static PVRSRV_ERROR
InitLocalHeaps(const SYS_DATA *psSysData,
			   CARD_PHYS_HEAP_CONFIG_SPEC *pasCardHeapSpec,
			   PHYS_HEAP_CONFIG *pasPhysHeaps,
			   IMG_UINT32 *pui32HeapIdx)
{
	PHYS_HEAP_FUNCTIONS *psHeapFuncs;
	PVRSRV_ERROR eError;
	IMG_UINT64 ui64CardBase;

	if (psSysData->pdata->baseboard == TC_BASEBOARD_ODIN &&
	    psSysData->pdata->mem_mode == TC_MEMORY_HYBRID)
	{
		psHeapFuncs = &gsHybridPhysHeapFuncs;
		ui64CardBase = ODIN_MEMORY_HYBRID_DEVICE_BASE;
	}
	else if (psSysData->pdata->mem_mode == TC_MEMORY_HYBRID)
	{
		psHeapFuncs = &gsHostPhysHeapFuncs;
		ui64CardBase = psSysData->pdata->tc_memory_base;
	}
	else
	{
		psHeapFuncs = &gsLocalPhysHeapFuncs;
		ui64CardBase = psSysData->pdata->rogue_heap_memory_base - psSysData->pdata->tc_memory_base;
	}

	eError = CreateCardGPUHeaps(psSysData, pasCardHeapSpec, pasPhysHeaps, psHeapFuncs, pui32HeapIdx, ui64CardBase);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

#if TC_DISPLAY_MEM_SIZE != 0
	eError = CreateCardEXTHeap(psSysData, pasPhysHeaps, psHeapFuncs, pui32HeapIdx, ui64CardBase);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}
#endif

	return PVRSRV_OK;
}

static PVRSRV_ERROR
InitHostHeaps(const SYS_DATA *psSysData, PHYS_HEAP_CONFIG *pasPhysHeaps, IMG_UINT32 *pui32HeapIdx)
{
	if (psSysData->pdata->mem_mode != TC_MEMORY_LOCAL)
	{
		pasPhysHeaps[*pui32HeapIdx].eType = PHYS_HEAP_TYPE_UMA;
		pasPhysHeaps[*pui32HeapIdx].ui32UsageFlags = PHYS_HEAP_USAGE_CPU_LOCAL;
		pasPhysHeaps[*pui32HeapIdx].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM";
		pasPhysHeaps[*pui32HeapIdx].uConfig.sUMA.psMemFuncs = &gsHostPhysHeapFuncs;
		pasPhysHeaps[*pui32HeapIdx].uConfig.sUMA.pszHeapName = "uma_cpu_local";

		(*pui32HeapIdx)++;

		PVR_DPF((PVR_DBG_WARNING,
				 "Initialising CPU_LOCAL UMA Host PhysHeaps with memory mode: %d",
				 psSysData->pdata->mem_mode));
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR
PhysHeapsInit(const SYS_DATA *psSysData,
			  CARD_PHYS_HEAP_CONFIG_SPEC *pasCardHeapSpec,
			  PHYS_HEAP_CONFIG *pasPhysHeaps,
			  void *pvPrivData, IMG_UINT32 ui32NumHeaps)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 i;
	IMG_UINT32 ui32HeapCounter = 0;

	eError = InitLocalHeaps(psSysData, pasCardHeapSpec, pasPhysHeaps, &ui32HeapCounter);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	eError = InitHostHeaps(psSysData, pasPhysHeaps, &ui32HeapCounter);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	PVR_LOG_RETURN_IF_FALSE((ui32HeapCounter == ui32NumHeaps),
							"Number of PhysHeapConfigs set up doesn't match the initial requirement.",
							PVRSRV_ERROR_PHYSHEAP_CONFIG);

	/* Initialise fields that don't change between memory modes.
	 * Fix up heap IDs. This is needed for multi-testchip systems to
	 * ensure the heap IDs are unique as this is what Services expects.
	 */
	for (i = 0; i < ui32NumHeaps; i++)
	{
		switch (pasPhysHeaps[i].eType)
		{
		case PHYS_HEAP_TYPE_UMA:
			pasPhysHeaps[i].uConfig.sUMA.hPrivData = pvPrivData;
			break;
		case PHYS_HEAP_TYPE_LMA:
			pasPhysHeaps[i].uConfig.sLMA.hPrivData = pvPrivData;
			break;
		default:
			PVR_DPF((PVR_DBG_ERROR, "Invalid PHYS_HEAP_TYPE: %u in %s",
			                        pasPhysHeaps[i].eType,
			                        __func__));
		}
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR
PhysHeapSetRequirements(const SYS_DATA *psSysData,
						CARD_PHYS_HEAP_CONFIG_SPEC *pasCardHeapSpec,
						IMG_UINT32 *pui32CardPhysHeapCfgCount)
{
	IMG_UINT32 i;
	IMG_UINT64 ui64FreeCardMemory = psSysData->pdata->rogue_heap_memory_size;

	PVR_LOG_RETURN_IF_FALSE(
		BITMASK_HAS(pasCardHeapSpec[HEAP_SPEC_IDX_GPU_PRIVATE].ui32UsageFlags, PHYS_HEAP_USAGE_GPU_PRIVATE) &&
		BITMASK_HAS(pasCardHeapSpec[HEAP_SPEC_IDX_GPU_LOCAL].ui32UsageFlags, PHYS_HEAP_USAGE_GPU_LOCAL),
		"PhysHeapConfigs not set correctly in the system layer.", PVRSRV_ERROR_PHYSHEAP_CONFIG);

	for (i = 0; i < ARRAY_SIZE(gasCardHeapTemplate); i++)
	{
		if (pasCardHeapSpec[i].bUsed)
		{
			/* Determine the memory requirements of heaps with a fixed size */
			ui64FreeCardMemory -= pasCardHeapSpec[i].uiSize;

			/* Count card physheap configs used by the system */
			(*pui32CardPhysHeapCfgCount)++;
		}
	}

	if (SysRestrictGpuLocalAddPrivateHeap())
	{
		IMG_UINT64 ui64GpuSharedMem = SysRestrictGpuLocalPhysheap(ui64FreeCardMemory);

		if (ui64GpuSharedMem == ui64FreeCardMemory)
		{
			/* No memory reserved for GPU private use, special heap not needed */
		}
		else
		{
			/* Set up the GPU private heap */
			pasCardHeapSpec[HEAP_SPEC_IDX_GPU_PRIVATE].bUsed = true;
			pasCardHeapSpec[HEAP_SPEC_IDX_GPU_PRIVATE].uiSize = ui64FreeCardMemory - ui64GpuSharedMem;
			ui64FreeCardMemory = ui64GpuSharedMem;
			(*pui32CardPhysHeapCfgCount)++;
		}
	}

	/* all remaining memory card memory goes to GPU_LOCAL */
	pasCardHeapSpec[HEAP_SPEC_IDX_GPU_LOCAL].uiSize = ui64FreeCardMemory;

	return PVRSRV_OK;
}

static PVRSRV_ERROR
PhysHeapsCreate(const SYS_DATA *psSysData, PVRSRV_DEVICE_CONFIG *psDevConfig,
				PHYS_HEAP_CONFIG **ppasPhysHeapsOut,
				IMG_UINT32 *puiPhysHeapCountOut)
{
	PHYS_HEAP_CONFIG *pasPhysHeaps;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32NumHeaps = 0;
	CARD_PHYS_HEAP_CONFIG_SPEC asCardHeapSpec[ARRAY_SIZE(gasCardHeapTemplate)];

	PVR_LOG_RETURN_IF_FALSE((psSysData->pdata->mem_mode == TC_MEMORY_LOCAL) ||
							(psSysData->pdata->mem_mode == TC_MEMORY_HYBRID),
							"Unsupported memory mode", PVRSRV_ERROR_NOT_IMPLEMENTED);

	/* Initialise the local heap specs with the build-time template */
	memcpy(asCardHeapSpec, gasCardHeapTemplate, sizeof(gasCardHeapTemplate));

	eError = PhysHeapSetRequirements(psSysData, asCardHeapSpec, &ui32NumHeaps);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	psDevConfig->bHasNonMappableLocalMemory = asCardHeapSpec[HEAP_SPEC_IDX_GPU_PRIVATE].bUsed;

	if (psSysData->pdata->mem_mode == TC_MEMORY_HYBRID)
	{
		/* CPU_LOCAL heap */
		ui32NumHeaps++;
	}

#if TC_DISPLAY_MEM_SIZE != 0
	if (psSysData->pdata->mem_mode == TC_MEMORY_LOCAL ||
		psSysData->pdata->mem_mode == TC_MEMORY_HYBRID)
	{
		/* EXTERNAL / DISPLAY heap */
		ui32NumHeaps++;
	}
#endif

	pasPhysHeaps = OSAllocMem(sizeof(*pasPhysHeaps) * ui32NumHeaps);
	if (!pasPhysHeaps)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	eError = PhysHeapsInit(psSysData, asCardHeapSpec, pasPhysHeaps, psDevConfig, ui32NumHeaps);
	if (eError != PVRSRV_OK)
	{
		OSFreeMem(pasPhysHeaps);
		return eError;
	}

	*ppasPhysHeapsOut = pasPhysHeaps;
	*puiPhysHeapCountOut = ui32NumHeaps;

	return PVRSRV_OK;
}

static void DeviceConfigDestroy(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	if (psDevConfig->pszVersion)
	{
		OSFreeMem(psDevConfig->pszVersion);
	}

	OSFreeMem(psDevConfig->pasPhysHeaps);

	OSFreeMem(psDevConfig);
}

static void odinTCDevPhysAddr2DmaAddr(PVRSRV_DEVICE_CONFIG *psDevConfig,
									  IMG_DMA_ADDR *psDmaAddr,
									  IMG_DEV_PHYADDR *psDevPAddr,
									  IMG_BOOL *pbValid,
									  IMG_UINT32 ui32NumAddr,
									  IMG_BOOL bSparseAlloc)
{
	IMG_CPU_PHYADDR sCpuPAddr = {0};
	IMG_UINT32 ui32Idx;

	/* Fast path */
	if (!bSparseAlloc)
	{
		/* In Odin, DMA address space is the same as host CPU */
		TCLocalDevPAddrToCpuPAddr(psDevConfig,
								  1,
								  &sCpuPAddr,
								  psDevPAddr);
		psDmaAddr->uiAddr = sCpuPAddr.uiAddr;
	}
	else
	{
		for (ui32Idx = 0; ui32Idx < ui32NumAddr; ui32Idx++)
		{
			if (pbValid[ui32Idx])
			{
				TCLocalDevPAddrToCpuPAddr(psDevConfig,
										  1,
										  &sCpuPAddr,
										  &psDevPAddr[ui32Idx]);
				psDmaAddr[ui32Idx].uiAddr = sCpuPAddr.uiAddr;
			}
			else
			{
				/* Invalid DMA address marker */
				psDmaAddr[ui32Idx].uiAddr = ~((IMG_UINT64)0x0);
			}
		}
	}
}

static void * odinTCgetCDMAChan(PVRSRV_DEVICE_CONFIG *psDevConfig, char *name)
{
	struct device* psDev = (struct device*) psDevConfig->pvOSDevice;
	return tc_dma_chan(psDev->parent, name);
}

static void odinTCFreeCDMAChan(PVRSRV_DEVICE_CONFIG *psDevConfig,
							   void* channel)
{

	struct device* psDev = (struct device*) psDevConfig->pvOSDevice;
	struct dma_chan *chan = (struct dma_chan*) channel;

	tc_dma_chan_free(psDev->parent, chan);
}

static void GetDriverMode(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	IMG_UINT32 ui32DeviceID;

	/*
	 * Drivers with virtualization support should check if the mode in which the
	 * driver must control a device has been explicitly specified at load time
	 * through module parameters.
	 * Multi-device platforms must find the internal ID of the device currently
	 * being created when checking for its associated DriverMode parameter.
	 */
	if (PVRSRVAcquireInternalID(&ui32DeviceID) != PVRSRV_OK)
	{
		psDevConfig->eDriverMode = DRIVER_MODE_NATIVE;
		return;
	}

	if (psPVRSRVData->aeModuleParamDriverMode[ui32DeviceID] == DRIVER_MODE_DEFAULT)
	{
#if (RGX_NUM_DRIVERS_SUPPORTED > 1)
		void __iomem *pvRegBase;

		pvRegBase = (void __iomem *) OSMapPhysToLin(psDevConfig->sRegsCpuPBase, psDevConfig->ui32RegsSize, PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);

		if (pvRegBase == NULL)
		{
			/* failed to map register bank, default to native mode */
			psDevConfig->eDriverMode = DRIVER_MODE_NATIVE;
		}
		else
		{
			IMG_UINT64 ui64ClkCtrl;

			/* the CLK_CTRL register is valid only in the Os 0 (Host) register bank
			 * if it reads 0 then we can conclude this Os is set up to run as Guest */
#if defined(RGX_CR_CLK_CTRL)
			ui64ClkCtrl = OSReadHWReg64(pvRegBase, RGX_CR_CLK_CTRL);
#else
			ui64ClkCtrl = OSReadHWReg64(pvRegBase, RGX_CR_CLK_CTRL1);
#endif
			OSUnMapPhysToLin((void __force *) pvRegBase, psDevConfig->ui32RegsSize);

			psDevConfig->eDriverMode = (ui64ClkCtrl != 0) ? (DRIVER_MODE_HOST) : (DRIVER_MODE_GUEST);
		}
#else
		psDevConfig->eDriverMode = DRIVER_MODE_NATIVE;
#endif
	}
	else
	{
		psDevConfig->eDriverMode = psPVRSRVData->aeModuleParamDriverMode[ui32DeviceID];
	}
}

static PVRSRV_ERROR DeviceConfigCreate(SYS_DATA *psSysData,
									   PVRSRV_DEVICE_CONFIG **ppsDevConfigOut)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig;
	RGX_DATA *psRGXData;
	RGX_TIMING_INFORMATION *psRGXTimingInfo;
	PHYS_HEAP_CONFIG *pasPhysHeaps;
	IMG_UINT32 uiPhysHeapCount;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32DeviceID;

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	PVR_LOG_RETURN_IF_FALSE((psSysData->pdata->baseboard == TC_BASEBOARD_ODIN &&
							 psSysData->pdata->mem_mode == TC_MEMORY_LOCAL),
							"Multidevice virtualization setup supported only on Odin device with TC_MEMORY_LOCAL",
							PVRSRV_ERROR_INVALID_DEVICE);
#endif

	psDevConfig = OSAllocZMem(sizeof(*psDevConfig) +
							  sizeof(*psRGXData) +
							  sizeof(*psRGXTimingInfo));
	if (!psDevConfig)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psRGXData = (RGX_DATA *) IMG_OFFSET_ADDR(psDevConfig, sizeof(*psDevConfig));
	psRGXTimingInfo = (RGX_TIMING_INFORMATION *) IMG_OFFSET_ADDR(psRGXData, sizeof(*psRGXData));

	/* Setup RGX specific timing data */
#if defined(TC_APOLLO_BONNIE)
	/* For BonnieTC there seems to be an additional 5x multiplier that occurs to the clock as measured speed is 540Mhz not 108Mhz. */
	psRGXTimingInfo->ui32CoreClockSpeed = tc_core_clock_speed(psSysData->pdev->dev.parent)  * 6 * 5;
#elif defined(TC_APOLLO_ES2)
	psRGXTimingInfo->ui32CoreClockSpeed = tc_core_clock_speed(psSysData->pdev->dev.parent)  * 6;
#else
	psRGXTimingInfo->ui32CoreClockSpeed = tc_core_clock_speed(psSysData->pdev->dev.parent) /
											tc_core_clock_multiplex(psSysData->pdev->dev.parent);
#endif
	psRGXTimingInfo->bEnableActivePM = IMG_FALSE;
	psRGXTimingInfo->bEnableRDPowIsland = IMG_FALSE;
	psRGXTimingInfo->ui32ActivePMLatencyms = SYS_RGX_ACTIVE_POWER_LATENCY_MS;

	/* Set up the RGX data */
	psRGXData->psRGXTimingInfo = psRGXTimingInfo;

	/* Setup the device config */
	psDevConfig->pvOSDevice = &psSysData->pdev->dev;
	psDevConfig->pszName = "tc";
	psDevConfig->pszVersion = GetDeviceVersionString(psSysData);

	psDevConfig->sRegsCpuPBase.uiAddr = psSysData->registers->start;
	psDevConfig->ui32RegsSize = resource_size(psSysData->registers);

	PVRSRVAcquireInternalID(&ui32DeviceID);
#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	/* Rogue FPGA images are correctly routing the OSID interrupts
	 * for cores with the IRQ_PER_OS feature */
	psDevConfig->ui32IRQ = TC_INTERRUPT_OSID0 + ui32DeviceID;
#else
	psDevConfig->ui32IRQ = TC_INTERRUPT_EXT;
#endif

	GetDriverMode(psDevConfig);

	eError = PhysHeapsCreate(psSysData, psDevConfig, &pasPhysHeaps, &uiPhysHeapCount);
	if (eError != PVRSRV_OK)
	{
		goto ErrorFreeDevConfig;
	}

	if (psSysData->pdata->baseboard == TC_BASEBOARD_ODIN &&
	    psSysData->pdata->mem_mode == TC_MEMORY_HYBRID)
	{
		psDevConfig->eDefaultHeap = SysDefaultToCpuLocalHeap() ?
		    PVRSRV_PHYS_HEAP_CPU_LOCAL : PVRSRV_PHYS_HEAP_GPU_LOCAL;
	}
	else
	{
		psDevConfig->eDefaultHeap = PVRSRV_PHYS_HEAP_GPU_LOCAL;
	}

	psDevConfig->eCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_NONE;

	psDevConfig->pasPhysHeaps = pasPhysHeaps;
	psDevConfig->ui32PhysHeapCount = uiPhysHeapCount;

	/* Only required for LMA but having this always set shouldn't be a problem */
	psDevConfig->bDevicePA0IsValid = IMG_TRUE;

	psDevConfig->hDevData = psRGXData;
	psDevConfig->hSysData = psSysData;

#if defined(SUPPORT_ALT_REGBASE)
	if (psSysData->pdata->mem_mode != TC_MEMORY_LOCAL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: alternative GPU register base is "
					"supported only in LMA mode", __func__));
		goto ErrorFreeDevConfig;
	}
	else
	{
		/* Using display memory base as the alternative GPU register base,
		 * since the display memory range is not used by the firmware. */
		IMG_CPU_PHYADDR  sDisplayMemAddr;

		sDisplayMemAddr.uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(psSysData->pdata->pdp_heap_memory_base);
		TCLocalCpuPAddrToDevPAddr(psDevConfig, 1,
		                          &psDevConfig->sAltRegsGpuPBase,
		                          &sDisplayMemAddr);
	}
#endif

#if defined(SUPPORT_LINUX_DVFS) || defined(SUPPORT_PDVFS)
	/* Fake DVFS configuration used purely for testing purposes */
	psDevConfig->sDVFS.sDVFSDeviceCfg.pasOPPTable = asOPPTable;
	psDevConfig->sDVFS.sDVFSDeviceCfg.ui32OPPTableSize = LEVEL_COUNT;
	psDevConfig->sDVFS.sDVFSDeviceCfg.pfnSetFrequency = SetFrequency;
	psDevConfig->sDVFS.sDVFSDeviceCfg.pfnSetVoltage = SetVoltage;
#endif
#if defined(SUPPORT_LINUX_DVFS)
	psDevConfig->sDVFS.sDVFSDeviceCfg.ui32PollMs = 1000;
	psDevConfig->sDVFS.sDVFSDeviceCfg.bIdleReq = IMG_TRUE;
	psDevConfig->sDVFS.sDVFSGovernorCfg.ui32UpThreshold = 90;
	psDevConfig->sDVFS.sDVFSGovernorCfg.ui32DownDifferential = 10;
#endif

	/* DMA channel config */
	psDevConfig->pfnSlaveDMAGetChan = odinTCgetCDMAChan;
	psDevConfig->pfnSlaveDMAFreeChan = odinTCFreeCDMAChan;
	psDevConfig->pfnDevPhysAddr2DmaAddr = odinTCDevPhysAddr2DmaAddr;
	psDevConfig->pszDmaTxChanName = psSysData->pdata->tc_dma_tx_chan_name;
	psDevConfig->pszDmaRxChanName = psSysData->pdata->tc_dma_rx_chan_name;
	psDevConfig->bHasDma = IMG_TRUE;
	/* Following two values are expressed in number of bytes */
	psDevConfig->ui32DmaTransferUnit = 1;
	psDevConfig->ui32DmaAlignment = 1;

	*ppsDevConfigOut = psDevConfig;

	return PVRSRV_OK;

ErrorFreeDevConfig:
	OSFreeMem(psDevConfig);
	return eError;
}

#if defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__)
/* #define _DBG(...) PVR_LOG((__VA_ARGS__)) */
#define _DBG(...)

static PVRSRV_ERROR PrePower(IMG_HANDLE hSysData,
                             PVRSRV_SYS_POWER_STATE eNewPowerState,
                             PVRSRV_SYS_POWER_STATE eCurrentPowerState,
                             PVRSRV_POWER_FLAGS ePwrFlags)
{
	SYS_DATA *psSysData = (SYS_DATA *) hSysData;
	IMG_DEV_PHYADDR sDevPAddr = {0};
	IMG_UINT64 uiHeapTotalSize, uiHeapUsedSize, uiHeapFreeSize;
	IMG_UINT64 uiSize = 0, uiOffset = 0;
	PVRSRV_ERROR eError;

	_DBG("(%s()) state: current=%s, new=%s; flags: 0x%08x", __func__,
	     PVRSRVSysPowerStateToString(eCurrentPowerState),
	     PVRSRVSysPowerStateToString(eNewPowerState), ePwrFlags);

	/* The transition might be both from ON or OFF states to OFF state so check
	 * only for the *new* state. Also this is only valid for suspend requests. */
	if (eNewPowerState != PVRSRV_SYS_POWER_STATE_OFF ||
	    !BITMASK_HAS(ePwrFlags, PVRSRV_POWER_FLAGS_OSPM_SUSPEND_REQ))
	{
		return PVRSRV_OK;
	}

	eError = LMA_HeapIteratorCreate(psSysData->psDevConfig->psDevNode,
	                                PVRSRV_PHYS_HEAP_GPU_LOCAL,
	                                &psSysData->psHeapIter);
	PVR_LOG_GOTO_IF_ERROR(eError, "LMA_HeapIteratorCreate", return_error);

	eError = LMA_HeapIteratorGetHeapStats(psSysData->psHeapIter, &uiHeapTotalSize,
	                                      &uiHeapUsedSize);
	PVR_LOG_GOTO_IF_ERROR(eError, "LMA_HeapIteratorGetHeapStats",
	                      return_error);
	uiHeapFreeSize = uiHeapTotalSize - uiHeapUsedSize;

	_DBG("(%s()) heap stats: total=0x%" IMG_UINT64_FMTSPECx ", "
	     "used=0x%" IMG_UINT64_FMTSPECx ", free=0x%" IMG_UINT64_FMTSPECx,
	     __func__, uiHeapTotalSize, uiHeapUsedSize, uiHeapFreeSize);

	psSysData->pvS3Buffer = OSAllocMem(uiHeapUsedSize);
	PVR_LOG_GOTO_IF_NOMEM(psSysData->pvS3Buffer, eError, destroy_iterator);

	while (LMA_HeapIteratorNext(psSysData->psHeapIter, &sDevPAddr, &uiSize))
	{
		void *pvCpuVAddr;
		IMG_CPU_PHYADDR sCpuPAddr = {0};

		if (uiOffset + uiSize > uiHeapUsedSize)
		{
			PVR_DPF((PVR_DBG_ERROR, "uiOffset = %" IMG_UINT64_FMTSPECx ", "
			        "uiSize = %" IMG_UINT64_FMTSPECx, uiOffset, uiSize));

			PVR_LOG_GOTO_WITH_ERROR("LMA_HeapIteratorNext", eError,
			                        PVRSRV_ERROR_INVALID_OFFSET,
			                        free_buffer);
		}

		TCLocalDevPAddrToCpuPAddr(psSysData->psDevConfig, 1, &sCpuPAddr,
		                          &sDevPAddr);

		pvCpuVAddr = OSMapPhysToLin(sCpuPAddr, uiSize,
		                            PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC);

		_DBG("(%s()) iterator: dev_paddr=%px, cpu_paddr=%px, cpu_vaddr=%px, "
		     "size=0x%05" IMG_UINT64_FMTSPECx, __func__,
		     (void *) sDevPAddr.uiAddr, (void *) sCpuPAddr.uiAddr,
		     pvCpuVAddr, uiSize);

		/* copy memory */
		memcpy((IMG_BYTE *) psSysData->pvS3Buffer + uiOffset, pvCpuVAddr,
		       uiSize);
		/* and now poison it */
		memset(pvCpuVAddr, 0x9b, uiSize);

		uiOffset += uiSize;

		OSUnMapPhysToLin(pvCpuVAddr, uiSize);
	}

	return PVRSRV_OK;

free_buffer:
	OSFreeMem(psSysData->pvS3Buffer);
	psSysData->pvS3Buffer = NULL;
destroy_iterator:
	LMA_HeapIteratorDestroy(psSysData->psHeapIter);
	psSysData->psHeapIter = NULL;
return_error:
	return eError;
}

static PVRSRV_ERROR PostPower(IMG_HANDLE hSysData,
                              PVRSRV_SYS_POWER_STATE eNewPowerState,
                              PVRSRV_SYS_POWER_STATE eCurrentPowerState,
                              PVRSRV_POWER_FLAGS ePwrFlags)
{
	SYS_DATA *psSysData = (SYS_DATA *) hSysData;
	IMG_DEV_PHYADDR sDevPAddr = {0};
	IMG_UINT64 uiSize = 0, uiOffset = 0;
	PVRSRV_ERROR eError;

	_DBG("(%s()) state: current=%s, new=%s; flags=0x%08x; buffer=%px", __func__,
	     PVRSRVSysPowerStateToString(eCurrentPowerState),
	     PVRSRVSysPowerStateToString(eNewPowerState), ePwrFlags,
	     psSysData->pvS3Buffer);

	/* The transition might be both to ON or OFF states from OFF state so check
	 * only for the *current* state. Also this is only valid for resume requests. */
	if (eCurrentPowerState != PVRSRV_SYS_POWER_STATE_OFF ||
	    !BITMASK_HAS(ePwrFlags, PVRSRV_POWER_FLAGS_OSPM_RESUME_REQ) ||
	    psSysData->pvS3Buffer == NULL)
	{
		return PVRSRV_OK;
	}

	eError = LMA_HeapIteratorReset(psSysData->psHeapIter);
	PVR_LOG_GOTO_IF_ERROR(eError, "LMA_HeapIteratorReset", free_buffer);

	while (LMA_HeapIteratorNext(psSysData->psHeapIter, &sDevPAddr, &uiSize))
	{
		void *pvCpuVAddr;
		IMG_CPU_PHYADDR sCpuPAddr = {0};

		TCLocalDevPAddrToCpuPAddr(psSysData->psDevConfig, 1, &sCpuPAddr,
		                          &sDevPAddr);

		pvCpuVAddr = OSMapPhysToLin(sCpuPAddr, uiSize,
		                       PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC);

		_DBG("(%s()) iterator: dev_paddr=%px, cpu_paddr=%px, cpu_vaddr=%px, "
		     "size=0x%05" IMG_UINT64_FMTSPECx, __func__,
		     (void *) sDevPAddr.uiAddr, (void *) sCpuPAddr.uiAddr,
		     pvCpuVAddr, uiSize);

		/* copy memory */
		memcpy(pvCpuVAddr, (IMG_BYTE *) psSysData->pvS3Buffer + uiOffset,
		       uiSize);

		uiOffset += uiSize;

		OSUnMapPhysToLin(pvCpuVAddr, uiSize);
	}

	LMA_HeapIteratorDestroy(psSysData->psHeapIter);
	psSysData->psHeapIter = NULL;

	OSFreeMem(psSysData->pvS3Buffer);
	psSysData->pvS3Buffer = NULL;

	return PVRSRV_OK;

free_buffer:
	OSFreeMem(psSysData->pvS3Buffer);
	psSysData->pvS3Buffer = NULL;

	return eError;
}
#endif /* defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__) */

PVRSRV_ERROR SysDevInit(void *pvOSDevice, PVRSRV_DEVICE_CONFIG **ppsDevConfig)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig;
	SYS_DATA *psSysData;
	resource_size_t uiRegistersSize;
	IMG_UINT32 ui32MinRegBankSize;
	PVRSRV_ERROR eError;
	int err = 0;

	PVR_ASSERT(pvOSDevice);

	psSysData = OSAllocZMem(sizeof(*psSysData));
	if (psSysData == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psSysData->pdev = to_platform_device((struct device *)pvOSDevice);
	psSysData->pdata = psSysData->pdev->dev.platform_data;

	/*
	 * The device cannot address system memory, so there is no DMA
	 * limitation.
	 */
	if (psSysData->pdata->mem_mode == TC_MEMORY_LOCAL)
	{
		dma_set_mask(pvOSDevice, DMA_BIT_MASK(64));
	}
	else
	{
		dma_set_mask(pvOSDevice, DMA_BIT_MASK(32));
	}

	err = tc_enable(psSysData->pdev->dev.parent);
	if (err)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to enable PCI device (%d)", __func__, err));
		eError = PVRSRV_ERROR_PCI_CALL_FAILED;
		goto ErrFreeSysData;
	}

	psSysData->registers = platform_get_resource_byname(psSysData->pdev,
														IORESOURCE_MEM,
														"rogue-regs");
	if (!psSysData->registers)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to get Rogue register information",
				 __func__));
		eError = PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;
		goto ErrorDevDisable;
	}

	/* Check the address range is large enough. */
	uiRegistersSize = resource_size(psSysData->registers);
#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	/* each GPU instance gets the minimum 64kb register range */
	ui32MinRegBankSize = RGX_CR_MTS_SCHEDULE1 - RGX_CR_MTS_SCHEDULE;
#else
	/* the GPU gets the entire 64MB IO range */
	ui32MinRegBankSize = SYS_RGX_REG_REGION_SIZE;
#endif

	if (uiRegistersSize < ui32MinRegBankSize)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Rogue register region isn't big enough (was %pa, required 0x%08x)",
				 __func__, &uiRegistersSize, ui32MinRegBankSize));

		eError = PVRSRV_ERROR_PCI_REGION_TOO_SMALL;
		goto ErrorDevDisable;
	}

	/* Reserve the address range */
	if (!request_mem_region(psSysData->registers->start,
							resource_size(psSysData->registers),
							SYS_RGX_DEV_NAME))
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Rogue register memory region not available",
				 __func__));
		eError = PVRSRV_ERROR_PCI_CALL_FAILED;

		goto ErrorDevDisable;
	}

	eError = DeviceConfigCreate(psSysData, &psDevConfig);
	if (eError != PVRSRV_OK)
	{
		goto ErrorReleaseMemRegion;
	}

#if defined(SUPPORT_ION) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
	eError = IonInit(psSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to initialise ION", __func__));
		goto ErrorDeviceConfigDestroy;
	}
#endif

	/* Set psDevConfig->pfnSysDevErrorNotify callback */
	psDevConfig->pfnSysDevErrorNotify = SysRGXErrorNotify;

#if defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__)
	/* power functions */
	psDevConfig->pfnPrePowerState = PrePower;
	psDevConfig->pfnPostPowerState = PostPower;

	psSysData->psDevConfig = psDevConfig;
#endif /* defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__) */

	*ppsDevConfig = psDevConfig;

	return PVRSRV_OK;

#if defined(SUPPORT_ION) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
ErrorDeviceConfigDestroy:
	DeviceConfigDestroy(psDevConfig);
#endif
ErrorReleaseMemRegion:
	release_mem_region(psSysData->registers->start,
					   resource_size(psSysData->registers));
ErrorDevDisable:
	tc_disable(psSysData->pdev->dev.parent);
ErrFreeSysData:
	OSFreeMem(psSysData);
	return eError;
}

void SysDevDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	SYS_DATA *psSysData = (SYS_DATA *)psDevConfig->hSysData;

#if defined(SUPPORT_ION) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
	IonDeinit();
#endif

	DeviceConfigDestroy(psDevConfig);

	release_mem_region(psSysData->registers->start,
					   resource_size(psSysData->registers));
	tc_disable(psSysData->pdev->dev.parent);

	OSFreeMem(psSysData);
}

PVRSRV_ERROR SysDebugInfo(PVRSRV_DEVICE_CONFIG *psDevConfig,
				DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				void *pvDumpDebugFile)
{
#if defined(TC_APOLLO_TCF5)
	PVR_UNREFERENCED_PARAMETER(psDevConfig);
	PVR_UNREFERENCED_PARAMETER(pfnDumpDebugPrintf);
	return PVRSRV_OK;
#else
	SYS_DATA *psSysData = psDevConfig->hSysData;
	PVRSRV_ERROR eError = PVRSRV_OK;
	u32 tmp = 0;
	u32 pll;

	PVR_DUMPDEBUG_LOG("------[ rgx_tc system debug ]------");

	if (tc_sys_info(psSysData->pdev->dev.parent, &tmp, &pll))
		goto err_out;

	if (tmp > 0)
		PVR_DUMPDEBUG_LOG("Chip temperature: %d degrees C", tmp);
	PVR_DUMPDEBUG_LOG("PLL status: %x", pll);

err_out:
	return eError;
#endif
}

typedef struct
{
	struct device *psDev;
	int iInterruptID;
	void *pvData;
	PFN_LISR pfnLISR;
} LISR_DATA;

static void TCInterruptHandler(void* pvData)
{
	LISR_DATA *psLISRData = pvData;
	psLISRData->pfnLISR(psLISRData->pvData);
}

PVRSRV_ERROR SysInstallDeviceLISR(IMG_HANDLE hSysData,
				  IMG_UINT32 ui32IRQ,
				  const IMG_CHAR *pszName,
				  PFN_LISR pfnLISR,
				  void *pvData,
				  IMG_HANDLE *phLISRData)
{
	SYS_DATA *psSysData = (SYS_DATA *)hSysData;
	LISR_DATA *psLISRData;
	PVRSRV_ERROR eError;
	int err;

	if ((ui32IRQ != TC_INTERRUPT_EXT) &&
		(ui32IRQ < TC_INTERRUPT_OSID0) && (ui32IRQ > TC_INTERRUPT_OSID7))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: No device matching IRQ %d", __func__, ui32IRQ));
		return PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
	}

	psLISRData = OSAllocZMem(sizeof(*psLISRData));
	if (!psLISRData)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto err_out;
	}

	psLISRData->pfnLISR = pfnLISR;
	psLISRData->pvData = pvData;
	psLISRData->iInterruptID = ui32IRQ;
	psLISRData->psDev = psSysData->pdev->dev.parent;

	err = tc_set_interrupt_handler(psLISRData->psDev, psLISRData->iInterruptID, TCInterruptHandler, psLISRData);
	if (err)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: tc_set_interrupt_handler() failed (%d)", __func__, err));
		eError = PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
		goto err_free_data;
	}

	err = tc_enable_interrupt(psLISRData->psDev, psLISRData->iInterruptID);
	if (err)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: tc_enable_interrupt() failed (%d)", __func__, err));
		eError = PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
		goto err_unset_interrupt_handler;
	}

	*phLISRData = psLISRData;
	eError = PVRSRV_OK;

	PVR_TRACE(("Installed device LISR " IMG_PFN_FMTSPEC " with tc module to ID %u",
			   pfnLISR, ui32IRQ));

err_out:
	return eError;
err_unset_interrupt_handler:
	tc_set_interrupt_handler(psLISRData->psDev, psLISRData->iInterruptID, NULL, NULL);
err_free_data:
	OSFreeMem(psLISRData);
	goto err_out;
}

PVRSRV_ERROR SysUninstallDeviceLISR(IMG_HANDLE hLISRData)
{
	LISR_DATA *psLISRData = (LISR_DATA *) hLISRData;
	int err;

	err = tc_disable_interrupt(psLISRData->psDev, psLISRData->iInterruptID);
	if (err)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: tc_disable_interrupt() failed (%d)", __func__, err));
	}

	err = tc_set_interrupt_handler(psLISRData->psDev, psLISRData->iInterruptID, NULL, NULL);
	if (err)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: tc_set_interrupt_handler() failed (%d)", __func__, err));
	}

	PVR_TRACE(("Uninstalled device LISR " IMG_PFN_FMTSPEC " with tc module from ID %u",
			   psLISRData->pfnLISR, psLISRData->iInterruptID));

	OSFreeMem(psLISRData);

	return PVRSRV_OK;
}
