/*************************************************************************/ /*!
@File
@Title          RGX firmware utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX firmware utility routines
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

#ifndef RGXFWUTILS_H
#define RGXFWUTILS_H

#include "rgx_memallocflags.h"
#include "log2.h"
#include "rgxdevice.h"
#include "rgxccb.h"
#include "devicemem.h"
#include "device.h"
#include "pvr_notifier.h"
#include "pvrsrv.h"
#include "connection_server.h"
#include "rgxta3d.h"
#include "devicemem_utils.h"
#include "rgxmem.h"
#include "rgxfwmemctx.h"
#include "rgxinit_apphints.h"

#define RGX_FIRMWARE_GUEST_RAW_HEAP_IDENT   "FwRawDriverID%d" /*!< RGX Raw Firmware Heap identifier */

static INLINE PVRSRV_ERROR _SelectDevMemHeap(PVRSRV_RGXDEV_INFO *psDevInfo,
											 PVRSRV_MEMALLOCFLAGS_T *puiFlags,
											 DEVMEM_HEAP **ppsFwHeap)
{
	PVRSRV_PHYS_HEAP ePhysHeap = (PVRSRV_PHYS_HEAP)PVRSRV_GET_PHYS_HEAP_HINT(*puiFlags);
	PVRSRV_ERROR eError = PVRSRV_OK;

	switch (ePhysHeap)
	{
		case PVRSRV_PHYS_HEAP_FW_CODE:
		case PVRSRV_PHYS_HEAP_FW_PRIV_DATA:
		case PVRSRV_PHYS_HEAP_FW_MAIN:
		{
			*ppsFwHeap = psDevInfo->psFirmwareMainHeap;
			break;
		}
		case PVRSRV_PHYS_HEAP_FW_CONFIG:
		{
			*ppsFwHeap = psDevInfo->psFirmwareConfigHeap;
			break;
		}
		case PVRSRV_PHYS_HEAP_FW_PREMAP0:
		case PVRSRV_PHYS_HEAP_FW_PREMAP1:
		case PVRSRV_PHYS_HEAP_FW_PREMAP2:
		case PVRSRV_PHYS_HEAP_FW_PREMAP3:
		case PVRSRV_PHYS_HEAP_FW_PREMAP4:
		case PVRSRV_PHYS_HEAP_FW_PREMAP5:
		case PVRSRV_PHYS_HEAP_FW_PREMAP6:
		case PVRSRV_PHYS_HEAP_FW_PREMAP7:
		{
			IMG_UINT32 ui32DriverID = ePhysHeap - PVRSRV_PHYS_HEAP_FW_PREMAP0;

			PVR_LOG_RETURN_IF_INVALID_PARAM(ui32DriverID < RGX_NUM_DRIVERS_SUPPORTED, "ui32DriverID");
			*ppsFwHeap = psDevInfo->psPremappedFwRawHeap[ui32DriverID];
			break;
		}
		default:
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: invalid phys heap", __func__));
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			break;
		}
	}

	return eError;
}

/*
 * Firmware-only allocation (which are initialised by the host) must be aligned to the SLC cache line size.
 * This is because firmware-only allocations are GPU_CACHE_INCOHERENT and this causes problems
 * if two allocations share the same cache line; e.g. the initialisation of the second allocation won't
 * make it into the SLC cache because it has been already loaded when accessing the content of the first allocation.
 */
static INLINE PVRSRV_ERROR DevmemFwAllocate(PVRSRV_RGXDEV_INFO *psDevInfo,
											IMG_DEVMEM_SIZE_T uiSize,
											PVRSRV_MEMALLOCFLAGS_T uiFlags,
											const IMG_CHAR *pszText,
											DEVMEM_MEMDESC **ppsMemDescPtr)
{
	IMG_DEV_VIRTADDR sTmpDevVAddr;
	PVRSRV_ERROR eError;
	DEVMEM_HEAP *psFwHeap;
	IMG_DEVMEM_ALIGN_T uiAlign;

	PVR_DPF_ENTERED;

	/* Enforce the standard pre-fix naming scheme callers must follow */
	PVR_ASSERT((pszText != NULL) && (pszText[0] == 'F') && (pszText[1] == 'w'));

	/* Imported from AppHint , flag to poison allocations when freed */
	uiFlags |= psDevInfo->uiFWPoisonOnFreeFlag;

	eError = _SelectDevMemHeap(psDevInfo, &uiFlags, &psFwHeap);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF_RETURN_RC(eError);
	}

	if (psFwHeap == psDevInfo->psFirmwareConfigHeap)
	{
		/*
		 * All structures allocated from the Firmware Config sub-heap must start at the same pre-determined
		 * offsets, regardless of the system's page size (e.g. 4k,16k,64k). The alignment requirement is
		 * satisfied for virtual addresses during the mapping stage. Physical allocations do not take
		 * alignment into consideration.
		 * VZ drivers usually preallocate and pre-map the entire Firmware heap range. Any allocations from
		 * this heap are physical allocations only, having their device VAs derived from their PAs. This makes
		 * it impossible to fulfil alignment requirements.
		 * To work around this limitation, allocation sizes are rounded to the nearest multiple of 64kb,
		 * regardless of the actual size of object.
		 */
		uiAlign = RGX_FIRMWARE_CONFIG_HEAP_ALLOC_GRANULARITY;

		uiSize = PVR_ALIGN(uiSize, RGX_FIRMWARE_CONFIG_HEAP_ALLOC_GRANULARITY);
	}
	else
	{
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
		if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
		{
			/* Aligning FW based allocations for MIPS based rogue cores at cache line boundary(16 bytes) instead
			 * of SLC(64 bytes) to have more compact memory with less waste and hopefully save some TLB misses.
			 * MIPS CPU cores alignment.
			 */
			uiAlign = RGXMIPSFW_MICROAPTIVEAP_CACHELINE_SIZE;
		}
		else
#endif
		{
			/* Non-MIPS CPU cores alignment */
			uiAlign = (GET_ROGUE_CACHE_LINE_SIZE(RGX_GET_FEATURE_VALUE(psDevInfo, SLC_CACHE_LINE_SIZE_BITS)));
		}
	}

	RGXFwSharedMemCPUCacheMode(psDevInfo->psDeviceNode,
	                           &uiFlags);

	eError = DevmemAllocateAndMap(psFwHeap,
				uiSize,
				uiAlign,
				uiFlags,
				pszText,
				ppsMemDescPtr,
				&sTmpDevVAddr);

	PVR_DPF_RETURN_RC(eError);
}

static INLINE PVRSRV_ERROR DevmemFwAllocateExportable(PVRSRV_DEVICE_NODE *psDeviceNode,
													  IMG_DEVMEM_SIZE_T uiSize,
													  IMG_DEVMEM_ALIGN_T uiAlign,
													  PVRSRV_MEMALLOCFLAGS_T uiFlags,
													  const IMG_CHAR *pszText,
													  DEVMEM_MEMDESC **ppsMemDescPtr)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *) psDeviceNode->pvDevice;
	IMG_DEV_VIRTADDR sTmpDevVAddr;
	PVRSRV_ERROR eError;
	DEVMEM_HEAP *psFwHeap;
	IMG_UINT32 ui32HeapLog2PageSize;

	PVR_DPF_ENTERED;

	/* Enforce the standard pre-fix naming scheme callers must follow */
	PVR_ASSERT((pszText != NULL) &&
			(pszText[0] == 'F') && (pszText[1] == 'w') &&
			(pszText[2] == 'E') && (pszText[3] == 'x'));

	/* Imported from AppHint , flag to poison allocations when freed */
	uiFlags |= psDevInfo->uiFWPoisonOnFreeFlag;

	eError = _SelectDevMemHeap(psDevInfo, &uiFlags, &psFwHeap);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF_RETURN_RC(eError);
	}

	RGXFwSharedMemCPUCacheMode(psDevInfo->psDeviceNode,
	                           &uiFlags);

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		/* MIPS cores */
		ui32HeapLog2PageSize = ExactLog2(uiAlign);
	}
	else
#endif
	{
		/* Meta and RiscV cores */
		ui32HeapLog2PageSize = DevmemGetHeapLog2PageSize(psFwHeap);
	}

	eError = DevmemAllocateExportable(psDeviceNode,
									  uiSize,
									  uiAlign,
									  ui32HeapLog2PageSize,
									  uiFlags,
									  pszText,
									  ppsMemDescPtr);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FW DevmemAllocateExportable failed (%u)", eError));
		PVR_DPF_RETURN_RC(eError);
	}

	/*
		We need to map it so the heap for this allocation
		is set
	*/
	eError = DevmemMapToDevice(*ppsMemDescPtr,
							   psFwHeap,
							   &sTmpDevVAddr);
	if (eError != PVRSRV_OK)
	{
		DevmemFree(*ppsMemDescPtr);
		PVR_DPF((PVR_DBG_ERROR, "FW DevmemMapToDevice failed (%u)", eError));
	}

	PVR_DPF_RETURN_RC1(eError, *ppsMemDescPtr);
}

static INLINE PVRSRV_ERROR DevmemFwAllocateSparse(PVRSRV_RGXDEV_INFO *psDevInfo,
												IMG_DEVMEM_SIZE_T uiSize,
												IMG_UINT32 ui32NumPhysChunks,
												IMG_UINT32 ui32NumVirtChunks,
												IMG_UINT32 *pui32MappingTable,
												PVRSRV_MEMALLOCFLAGS_T uiFlags,
												const IMG_CHAR *pszText,
												DEVMEM_MEMDESC **ppsMemDescPtr)
{
	IMG_DEV_VIRTADDR sTmpDevVAddr;
	PVRSRV_ERROR eError;
	DEVMEM_HEAP *psFwHeap;
	IMG_UINT32 ui32Align;

	PVR_DPF_ENTERED;

	/* Enforce the standard pre-fix naming scheme callers must follow */
	PVR_ASSERT((pszText != NULL) && (pszText[0] == 'F') && (pszText[1] == 'w'));
	ui32Align = GET_ROGUE_CACHE_LINE_SIZE(RGX_GET_FEATURE_VALUE(psDevInfo, SLC_CACHE_LINE_SIZE_BITS));

	/* Imported from AppHint , flag to poison allocations when freed */
	uiFlags |= psDevInfo->uiFWPoisonOnFreeFlag;

	eError = _SelectDevMemHeap(psDevInfo, &uiFlags, &psFwHeap);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF_RETURN_RC(eError);
	}

	RGXFwSharedMemCPUCacheMode(psDevInfo->psDeviceNode,
	                           &uiFlags);

	eError = DevmemAllocateSparse(psDevInfo->psDeviceNode,
								uiSize,
								ui32NumPhysChunks,
								ui32NumVirtChunks,
								pui32MappingTable,
								ui32Align,
								DevmemGetHeapLog2PageSize(psFwHeap),
								uiFlags | PVRSRV_MEMALLOCFLAG_SPARSE_NO_SCRATCH_BACKING,
								pszText,
								ppsMemDescPtr);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF_RETURN_RC(eError);
	}
	/*
		We need to map it so the heap for this allocation
		is set
	*/
	eError = DevmemMapToDevice(*ppsMemDescPtr,
				   psFwHeap,
				   &sTmpDevVAddr);
	if (eError != PVRSRV_OK)
	{
		DevmemFree(*ppsMemDescPtr);
		PVR_DPF_RETURN_RC(eError);
	}

	PVR_DPF_RETURN_RC(eError);
}


static INLINE void DevmemFwUnmapAndFree(PVRSRV_RGXDEV_INFO *psDevInfo,
								DEVMEM_MEMDESC *psMemDesc)
{
	PVR_DPF_ENTERED1(psMemDesc);

	PVR_UNREFERENCED_PARAMETER(psDevInfo);

	DevmemReleaseDevVirtAddr(psMemDesc);
	DevmemFree(psMemDesc);

	PVR_DPF_RETURN;
}

/*
 * This function returns the value of the hardware register RGX_CR_TIMER
 * which is a timer counting in ticks.
 */

static INLINE IMG_UINT64 RGXReadHWTimerReg(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_UINT64 ui64Time = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_TIMER);

	/*
	*  In order to avoid having to issue three 32-bit reads to detect the
	*  lower 32-bits wrapping, the MSB of the low 32-bit word is duplicated
	*  in the MSB of the high 32-bit word. If the wrap happens, we just read
	*  the register again (it will not wrap again so soon).
	*/
	if ((ui64Time ^ (ui64Time << 32)) & ~RGX_CR_TIMER_BIT31_CLRMSK)
	{
		ui64Time = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_TIMER);
	}

	return (ui64Time & ~RGX_CR_TIMER_VALUE_CLRMSK) >> RGX_CR_TIMER_VALUE_SHIFT;
}

/*
 * This FW Common Context is only mapped into kernel for initialisation and cleanup purposes.
 * Otherwise this allocation is only used by the FW.
 * Therefore the GPU cache doesn't need coherency, and write-combine will
 * suffice on the CPU side (WC buffer will be flushed at the first kick)
 */
#define RGX_FWCOMCTX_ALLOCFLAGS      (PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) | \
                                      PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED)| \
                                      PVRSRV_MEMALLOCFLAG_GPU_READABLE | \
                                      PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE | \
                                      PVRSRV_MEMALLOCFLAG_GPU_CACHE_INCOHERENT | \
                                      PVRSRV_MEMALLOCFLAG_CPU_READABLE | \
                                      PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE | \
                                      PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC | \
                                      PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                      PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                      PVRSRV_MEMALLOCFLAG_RI_FWKMD_ALLOC | \
                                      PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN))

#define RGX_FWCODEDATA_ALLOCFLAGS    (PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) | \
                                      PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED) | \
                                      PVRSRV_MEMALLOCFLAG_GPU_READABLE | \
                                      PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE | \
                                      PVRSRV_MEMALLOCFLAG_CPU_READABLE | \
                                      PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE | \
                                      PVRSRV_MEMALLOCFLAG_GPU_CACHE_INCOHERENT | \
                                      PVRSRV_MEMALLOCFLAG_RI_FWKMD_ALLOC | \
                                      PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE)

#define RGX_FWSHAREDMEM_MAIN_ALLOCFLAGS (PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) | \
                                         PVRSRV_MEMALLOCFLAG_GPU_READABLE | \
                                         PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE | \
                                         PVRSRV_MEMALLOCFLAG_GPU_UNCACHED | \
                                         PVRSRV_MEMALLOCFLAG_CPU_READABLE | \
                                         PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE | \
                                         PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC | \
                                         PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                         PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                         PVRSRV_MEMALLOCFLAG_RI_FWKMD_ALLOC | \
                                         PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN))

#define RGX_FWSHAREDMEM_CONFIG_ALLOCFLAGS (PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) | \
                                           PVRSRV_MEMALLOCFLAG_GPU_READABLE | \
                                           PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE | \
                                           PVRSRV_MEMALLOCFLAG_GPU_UNCACHED | \
                                           PVRSRV_MEMALLOCFLAG_CPU_READABLE | \
                                           PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE | \
                                           PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC | \
                                           PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                           PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                           PVRSRV_MEMALLOCFLAG_RI_FWKMD_ALLOC | \
                                           PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_CONFIG))

#define RGX_FWSHAREDMEM_GPU_RO_ALLOCFLAGS (PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) | \
                                           PVRSRV_MEMALLOCFLAG_GPU_READABLE | \
                                           PVRSRV_MEMALLOCFLAG_GPU_UNCACHED | \
                                           PVRSRV_MEMALLOCFLAG_CPU_READABLE | \
                                           PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE | \
                                           PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC | \
                                           PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                           PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                           PVRSRV_MEMALLOCFLAG_RI_FWKMD_ALLOC | \
                                           PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN))

/* Firmware memory that is not accessible by the CPU. */
#define RGX_FWSHAREDMEM_GPU_ONLY_ALLOCFLAGS (PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) | \
                                             PVRSRV_MEMALLOCFLAG_GPU_READABLE | \
                                             PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE | \
                                             PVRSRV_MEMALLOCFLAG_GPU_UNCACHED | \
                                             PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                             PVRSRV_MEMALLOCFLAG_RI_FWKMD_ALLOC)

/* Firmware shared memory that is supposed to be read-only to the CPU.
 * In reality it isn't due to ZERO_ON_ALLOC which enforces CPU_WRITEABLE
 * flag on the allocations. */
#define RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS (PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) | \
                                           PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN) | \
                                           PVRSRV_MEMALLOCFLAG_GPU_READABLE | \
                                           PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE | \
                                           PVRSRV_MEMALLOCFLAG_CPU_READABLE | \
                                           PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                           PVRSRV_MEMALLOCFLAG_GPU_UNCACHED | \
                                           PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC | \
                                           PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                           PVRSRV_MEMALLOCFLAG_RI_FWKMD_ALLOC)

/* data content being kept from previous boot cycles from physical memory must not be cleared during allocation */
#define RGX_AUTOVZ_KEEP_FW_DATA_MASK(bKeepMem) ((bKeepMem) ? (~PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC) : (~0ULL))

/******************************************************************************
 * RGXSetFirmwareAddress Flags
 *****************************************************************************/
#define RFW_FWADDR_FLAG_NONE		(0)			/*!< Void flag */
#define RFW_FWADDR_NOREF_FLAG		(1U << 0)	/*!< It is safe to immediately release the reference to the pointer,
												  otherwise RGXUnsetFirmwareAddress() must be call when finished. */

/*************************************************************************/ /*!
@Function       RGXTraceBufferIsInitRequired

@Description    Returns true if the firmware trace buffer is not allocated and
                might be required by the firmware soon. Trace buffer allocated
                on-demand to reduce RAM footprint on systems not needing
                firmware trace.

@Input          psDevInfo RGX device info

@Return         IMG_BOOL  Whether on-demand allocation(s) is/are needed or not
*/ /**************************************************************************/
FORCE_INLINE IMG_BOOL RGXTraceBufferIsInitRequired(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	RGXFWIF_TRACEBUF*  psTraceBufCtl = psDevInfo->psRGXFWIfTraceBufCtl;

	RGXFwSharedMemCacheOpValue(psTraceBufCtl->ui32LogType, INVALIDATE);

	/* The firmware expects a trace buffer only when:
	 *	- Logtype is "trace" AND
	 *	- at least one LogGroup is configured
	 *	- the Driver Mode is not Guest
	 */
	if ((psDevInfo->psRGXFWIfTraceBufferMemDesc[0] == NULL)
		&& (psTraceBufCtl->ui32LogType & RGXFWIF_LOG_TYPE_TRACE)
		&& (psTraceBufCtl->ui32LogType & RGXFWIF_LOG_TYPE_GROUP_MASK)
		&& !PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		return IMG_TRUE;
	}

	return IMG_FALSE;
}

PVRSRV_ERROR RGXTraceBufferInitOnDemandResources(PVRSRV_RGXDEV_INFO* psDevInfo, PVRSRV_MEMALLOCFLAGS_T uiAllocFlags);

#if defined(SUPPORT_TBI_INTERFACE)
/*************************************************************************/ /*!
@Function       RGXTBIBufferIsInitRequired

@Description    Returns true if the firmware tbi buffer is not allocated and
                might be required by the firmware soon. TBI buffer allocated
                on-demand to reduce RAM footprint on systems not needing
                tbi.

@Input          psDevInfo RGX device info

@Return         IMG_BOOL  Whether on-demand allocation(s) is/are needed or not
*/ /**************************************************************************/
FORCE_INLINE IMG_BOOL RGXTBIBufferIsInitRequired(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	RGXFWIF_TRACEBUF*  psTraceBufCtl = psDevInfo->psRGXFWIfTraceBufCtl;

	RGXFwSharedMemCacheOpValue(psTraceBufCtl->ui32LogType, INVALIDATE);

	/* The firmware expects a tbi buffer only when:
	 *	- Logtype is "tbi"
	 */
	if ((psDevInfo->psRGXFWIfTBIBufferMemDesc == NULL)
		 && (psTraceBufCtl->ui32LogType & ~RGXFWIF_LOG_TYPE_TRACE)
		 && (psTraceBufCtl->ui32LogType & RGXFWIF_LOG_TYPE_GROUP_MASK))
	{
		return IMG_TRUE;
	}

	return IMG_FALSE;
}

PVRSRV_ERROR RGXTBIBufferInitOnDemandResources(PVRSRV_RGXDEV_INFO *psDevInfo);
#endif

PVRSRV_ERROR RGXSetupFirmware(PVRSRV_DEVICE_NODE       *psDeviceNode,
                              RGX_INIT_APPHINTS        *psApphints,
                              IMG_UINT32               ui32ConfigFlags,
                              IMG_UINT32               ui32ConfigFlagsExt,
                              IMG_UINT32               ui32FwOsCfgFlags);


void RGXFreeFirmware(PVRSRV_RGXDEV_INFO *psDevInfo);

/*************************************************************************/ /*!
@Function       RGXSetupFwAllocation

@Description    Sets a pointer in a firmware data structure.

@Input          psDevInfo       Device Info struct
@Input          uiAllocFlags    Flags determining type of memory allocation
@Input          ui32Size        Size of memory allocation
@Input          pszName         Allocation label
@Input          psFwPtr         Address of the firmware pointer to set
@Input          ppvCpuPtr       Address of the cpu pointer to set
@Input          ui32DevVAFlags  Any combination of  RFW_FWADDR_*_FLAG

@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RGXSetupFwAllocation(PVRSRV_RGXDEV_INFO   *psDevInfo,
								  PVRSRV_MEMALLOCFLAGS_T uiAllocFlags,
								  IMG_UINT32           ui32Size,
								  const IMG_CHAR       *pszName,
								  DEVMEM_MEMDESC       **ppsMemDesc,
								  RGXFWIF_DEV_VIRTADDR *psFwPtr,
								  void                 **ppvCpuPtr,
								  IMG_UINT32           ui32DevVAFlags);

/*************************************************************************/ /*!
@Function       RGXSetFirmwareAddress

@Description    Sets a pointer in a firmware data structure.

@Input          ppDest          Address of the pointer to set
@Input          psSrc           MemDesc describing the pointer
@Input          ui32Flags       Any combination of RFW_FWADDR_*_FLAG

@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RGXSetFirmwareAddress(RGXFWIF_DEV_VIRTADDR	*ppDest,
								   DEVMEM_MEMDESC		*psSrc,
								   IMG_UINT32			uiOffset,
								   IMG_UINT32			ui32Flags);


/*************************************************************************/ /*!
@Function       RGXSetMetaDMAAddress

@Description    Fills a Firmware structure used to setup the Meta DMA with two
                pointers to the same data, one on 40 bit and one on 32 bit
                (pointer in the FW memory space).

@Input          ppDest          Address of the structure to set
@Input          psSrcMemDesc    MemDesc describing the pointer
@Input          psSrcFWDevVAddr Firmware memory space pointer

@Return         void
*/ /**************************************************************************/
void RGXSetMetaDMAAddress(RGXFWIF_DMA_ADDR		*psDest,
						  DEVMEM_MEMDESC		*psSrcMemDesc,
						  RGXFWIF_DEV_VIRTADDR	*psSrcFWDevVAddr,
						  IMG_UINT32			uiOffset);


/*************************************************************************/ /*!
@Function       RGXUnsetFirmwareAddress

@Description    Unsets a pointer in a firmware data structure

@Input          psSrc           MemDesc describing the pointer

@Return         void
*/ /**************************************************************************/
void RGXUnsetFirmwareAddress(DEVMEM_MEMDESC *psSrc);

/*!
*******************************************************************************
@Function       RGXScheduleProcessQueuesKM

@Description    Software command complete handler
                (sends uncounted kicks for all the DMs through the MISR)

@Input          hCmdCompHandle  RGX device node

@Return         None
******************************************************************************/
void RGXScheduleProcessQueuesKM(PVRSRV_CMDCOMP_HANDLE hCmdCompHandle);


/*!
*******************************************************************************

@Function       RGXInstallProcessQueuesMISR

@Description    Installs the MISR to handle Process Queues operations

@Input          phMISR          Pointer to the MISR handler
@Input          psDeviceNode    RGX Device node

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXInstallProcessQueuesMISR(IMG_HANDLE *phMISR, PVRSRV_DEVICE_NODE *psDeviceNode);

PVRSRV_ERROR RGXSendCommandsFromDeferredList(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_BOOL bPoll);

/*************************************************************************/ /*!
@Function       RGXSendCommandWithPowLockAndGetKCCBSlot

@Description    Sends a command to a particular DM without honouring
                pending cache operations but taking the power lock.

@Input          psDevInfo       Device Info
@Input          psKCCBCmd       The cmd to send.
@Input          ui32PDumpFlags  Pdump flags
@Output         pui32CmdKCCBSlot   When non-NULL:
                                   - Pointer on return contains the kCCB slot
                                     number in which the command was enqueued.
                                   - Resets the value of the allotted slot to
                                     RGXFWIF_KCCB_RTN_SLOT_RST
@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RGXSendCommandWithPowLockAndGetKCCBSlot(PVRSRV_RGXDEV_INFO	*psDevInfo,
													 RGXFWIF_KCCB_CMD	*psKCCBCmd,
													 IMG_UINT32			ui32PDumpFlags,
													 IMG_UINT32			*pui32CmdKCCBSlot);

#define RGXSendCommandWithPowLock(psDevInfo, psKCCBCmd, ui32PDumpFlags) \
  RGXSendCommandWithPowLockAndGetKCCBSlot(psDevInfo, psKCCBCmd, ui32PDumpFlags, NULL)

/*************************************************************************/ /*!
@Function       RGXSendCommandAndGetKCCBSlot

@Description    Sends a command to a particular DM without honouring
                pending cache operations or the power lock.
                The function flushes any deferred KCCB commands first.

@Input          psDevInfo       Device Info
@Input          psKCCBCmd       The cmd to send.
@Input          uiPdumpFlags    PDump flags.
@Output         pui32CmdKCCBSlot   When non-NULL:
                                   - Pointer on return contains the kCCB slot
                                     number in which the command was enqueued.
                                   - Resets the value of the allotted slot to
                                     RGXFWIF_KCCB_RTN_SLOT_RST
@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RGXSendCommandAndGetKCCBSlot(PVRSRV_RGXDEV_INFO *psDevInfo,
										  RGXFWIF_KCCB_CMD   *psKCCBCmd,
										  PDUMP_FLAGS_T      uiPdumpFlags,
										  IMG_UINT32         *pui32CmdKCCBSlot);

#define RGXSendCommand(psDevInfo, psKCCBCmd, ui32PDumpFlags) \
  RGXSendCommandAndGetKCCBSlot(psDevInfo, psKCCBCmd, ui32PDumpFlags, NULL)

/*************************************************************************/ /*!
@Function       _RGXScheduleCommandAndGetKCCBSlot

@Description    Sends a command to a particular DM and kicks the firmware but
                first schedules any commands which have to happen before
                handle

@Input          psDevInfo          Device Info
@Input          eDM                To which DM the cmd is sent.
@Input          psKCCBCmd          The cmd to send.
@Input          ui32PDumpFlags     PDump flags
@Input          bCallerHasPwrLock  Caller already has power lock
@Output         pui32CmdKCCBSlot   When non-NULL:
                                   - Pointer on return contains the kCCB slot
                                     number in which the command was enqueued.
                                   - Resets the value of the allotted slot to
                                     RGXFWIF_KCCB_RTN_SLOT_RST
@Return			PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR _RGXScheduleCommandAndGetKCCBSlot(PVRSRV_RGXDEV_INFO *psDevInfo,
											   RGXFWIF_DM         eKCCBType,
											   RGXFWIF_KCCB_CMD   *psKCCBCmd,
											   IMG_UINT32         ui32PDumpFlags,
											   IMG_UINT32         *pui32CmdKCCBSlot,
											   IMG_BOOL           bCallerHasPwrLock);

#define RGXScheduleCommandAndGetKCCBSlot(psDevInfo, eKCCBType, psKCCBCmd, ui32PDumpFlags, pui32CmdKCCBSlot) \
	_RGXScheduleCommandAndGetKCCBSlot(psDevInfo, eKCCBType, psKCCBCmd, ui32PDumpFlags, pui32CmdKCCBSlot, IMG_FALSE)

#define RGXScheduleCommand(psDevInfo, eKCCBType, psKCCBCmd, ui32PDumpFlags) \
  RGXScheduleCommandAndGetKCCBSlot(psDevInfo, eKCCBType, psKCCBCmd, ui32PDumpFlags, NULL)

#define RGXScheduleCommandWithoutPowerLock(psDevInfo, eKCCBType, psKCCBCmd, ui32PDumpFlags) \
	_RGXScheduleCommandAndGetKCCBSlot(psDevInfo, eKCCBType, psKCCBCmd, ui32PDumpFlags, NULL, IMG_TRUE)

/*************************************************************************/ /*!
@Function       RGXWaitForKCCBSlotUpdate

@Description    Waits until the required kCCB slot value is updated by the FW
                (signifies command completion). Additionally, dumps a relevant
                PDump poll command.

@Input          psDevInfo       Device Info
@Input          ui32SlotNum     The kCCB slot number to wait for an update on
@Input          ui32PDumpFlags

@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RGXWaitForKCCBSlotUpdate(PVRSRV_RGXDEV_INFO *psDevInfo,
                                      IMG_UINT32 ui32SlotNum,
									  IMG_UINT32 ui32PDumpFlags);


/*************************************************************************/ /*!
@Function       PVRSRVRGXFrameworkCopyCommand

@Description    Copy framework command into FW addressable buffer

@param          psDeviceNode
@param          psFWFrameworkMemDesc
@param          pbyGPUFRegisterList
@param          ui32FrameworkRegisterSize

@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR PVRSRVRGXFrameworkCopyCommand(PVRSRV_DEVICE_NODE *psDeviceNode,
										   DEVMEM_MEMDESC   *psFWFrameworkMemDesc,
										   IMG_PBYTE        pbyGPUFRegisterList,
										   IMG_UINT32       ui32FrameworkRegisterSize);


/*************************************************************************/ /*!
@Function       PVRSRVRGXFrameworkCreateKM

@Description    Create FW addressable buffer for framework

@param          psDeviceNode
@param          ppsFWFrameworkMemDesc
@param          ui32FrameworkRegisterSize

@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR PVRSRVRGXFrameworkCreateKM(PVRSRV_DEVICE_NODE * psDeviceNode,
										DEVMEM_MEMDESC     ** ppsFWFrameworkMemDesc,
										IMG_UINT32         ui32FrameworkRegisterSize);

/*************************************************************************/ /*!
@Function       RGXPollForGPCommandCompletion

@Description    Polls for completion of a submitted GP command. Poll is done
                on a value matching a masked read from the address.

@Input          psDevNode       Pointer to device node struct
@Input          pui32LinMemAddr CPU linear address to poll
@Input          ui32Value       Required value
@Input          ui32Mask        Mask

@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RGXPollForGPCommandCompletion(PVRSRV_DEVICE_NODE *psDevNode,
									volatile IMG_UINT32 __iomem *pui32LinMemAddr,
									IMG_UINT32                   ui32Value,
									IMG_UINT32                   ui32Mask);

/*************************************************************************/ /*!
@Function       RGXStateFlagCtrl

@Description    Set and return FW internal state flags.

@Input          psDevInfo       Device Info
@Input          ui32Config      AppHint config flags
@Output         pui32State      Current AppHint state flag configuration
@Input          bSetNotClear    Set or clear the provided config flags

@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RGXStateFlagCtrl(PVRSRV_RGXDEV_INFO *psDevInfo,
				IMG_UINT32 ui32Config,
				IMG_UINT32 *pui32State,
				IMG_BOOL bSetNotClear);

/*!
*******************************************************************************
@Function       RGXFWRequestCommonContextCleanUp

@Description    Schedules a FW common context cleanup. The firmware doesn't
                block waiting for the resource to become idle but rather
                notifies the host that the resource is busy.

@Input          psDeviceNode    pointer to device node
@Input          psServerCommonContext context to be cleaned up
@Input          eDM             Data master, to which the cleanup command should
                                be sent
@Input          ui32PDumpFlags  PDump continuous flag

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXFWRequestCommonContextCleanUp(PVRSRV_DEVICE_NODE *psDeviceNode,
											  RGX_SERVER_COMMON_CONTEXT *psServerCommonContext,
											  RGXFWIF_DM eDM,
											  IMG_UINT32 ui32PDumpFlags);

/*!
*******************************************************************************
@Function       RGXFWRequestHWRTDataCleanUp

@Description    Schedules a FW HWRTData memory cleanup. The firmware doesn't
                block waiting for the resource to become idle but rather
                notifies the host that the resource is busy.

@Input          psDeviceNode    pointer to device node
@Input          psHWRTData      firmware address of the HWRTData for clean-up

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXFWRequestHWRTDataCleanUp(PVRSRV_DEVICE_NODE *psDeviceNode,
										 PRGXFWIF_HWRTDATA psHWRTData);

/*!
*******************************************************************************
@Function       RGXFWRequestFreeListCleanUp

@Description    Schedules a FW FreeList cleanup. The firmware doesn't block
                waiting for the resource to become idle but rather notifies the
                host that the resource is busy.

@Input          psDeviceNode    pointer to device node
@Input          psFWFreeList    firmware address of the FreeList for clean-up

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXFWRequestFreeListCleanUp(PVRSRV_RGXDEV_INFO *psDeviceNode,
										 PRGXFWIF_FREELIST psFWFreeList);

/*!
*******************************************************************************
@Function       RGXFWRequestZSBufferCleanUp

@Description    Schedules a FW ZS Buffer cleanup. The firmware doesn't block
                waiting for the resource to become idle but rather notifies the
                host that the resource is busy.

@Input          psDevInfo       pointer to device node
@Input          psFWZSBuffer    firmware address of the ZS Buffer for clean-up

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXFWRequestZSBufferCleanUp(PVRSRV_RGXDEV_INFO *psDevInfo,
										 PRGXFWIF_ZSBUFFER psFWZSBuffer);

/*!
*******************************************************************************
@Function       RGXFWSetHCSDeadline

@Description    Requests the Firmware to set a new Hard Context Switch timeout
                deadline. Context switches that surpass that deadline cause the
                system to kill the currently running workloads.

@Input          psDeviceNode    pointer to device node
@Input          ui32HCSDeadlineMs  The deadline in milliseconds.

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXFWSetHCSDeadline(PVRSRV_RGXDEV_INFO *psDevInfo,
								IMG_UINT32 ui32HCSDeadlineMs);

/*!
*******************************************************************************
@Function       RGXFWHealthCheckCmdInt

@Description    Ping the firmware to check if it is responsive.

@Input          psDevInfo          pointer to device info
@Input          bCallerHasPwrLock  Caller already has power lock

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXFWHealthCheckCmdInt(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_BOOL bCallerHasPwrLock);

#define RGXFWHealthCheckCmd(psDevInfo)	\
	RGXFWHealthCheckCmdInt(psDevInfo, IMG_FALSE)

#define RGXFWHealthCheckCmdWithoutPowerLock(psDevInfo)	\
	RGXFWHealthCheckCmdInt(psDevInfo, IMG_TRUE)

/*!
*******************************************************************************
@Function       RGXFWSetFwOsState

@Description    Requests the Firmware to change the guest OS Online states.
                This should be initiated by the VMM when a guest VM comes
                online or goes offline. If offline, the FW offloads any current
                resource from that DriverID. The request is repeated until the
                FW has had time to free all the resources or has waited for
                workloads to finish.

@Input          psDevInfo       pointer to device info
@Input          ui32DriverID    The driver whose state is being altered
@Input          eOSOnlineState  The new state (Online or Offline)

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXFWSetFwOsState(PVRSRV_RGXDEV_INFO *psDevInfo,
								IMG_UINT32 ui32DriverID,
								RGXFWIF_OS_STATE_CHANGE eOSOnlineState);

#if defined(SUPPORT_AUTOVZ)
/*!
*******************************************************************************
@Function       RGXUpdateAutoVzWdgToken

@Description    If the driver-firmware connection is active, read the
                firmware's watchdog token and copy its value back into the OS
                token. This indicates to the firmware that this driver is alive
                and responsive.

@Input          psDevInfo       pointer to device info
******************************************************************************/
void RGXUpdateAutoVzWdgToken(PVRSRV_RGXDEV_INFO *psDevInfo);

/*!
*******************************************************************************
@Function       RGXDisconnectAllGuests

@Description    Send requests to FW to disconnect all guest connections.

@Input          psDeviceNode    pointer to device node
******************************************************************************/
PVRSRV_ERROR RGXDisconnectAllGuests(PVRSRV_DEVICE_NODE *psDeviceNode);
#endif

/*!
*******************************************************************************
@Function       RGXFWConfigPHR

@Description    Configure the Periodic Hardware Reset functionality

@Input          psDevInfo       pointer to device info
@Input          ui32PHRMode     desired PHR mode

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXFWConfigPHR(PVRSRV_RGXDEV_INFO *psDevInfo,
                            IMG_UINT32 ui32PHRMode);

/*!
*******************************************************************************
@Function       RGXFWConfigWdg

@Description    Configure the Safety watchdog trigger period

@Input          psDevInfo        pointer to device info
@Input          ui32WdgPeriodUs  requested period in microseconds

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXFWConfigWdg(PVRSRV_RGXDEV_INFO *psDevInfo,
                            IMG_UINT32 ui32WdgPeriod);

/*!
*******************************************************************************
@Function       RGXCheckFirmwareCCB

@Description    Processes all commands that are found in the Firmware CCB.

@Input          psDevInfo       pointer to device

@Return         None
******************************************************************************/
void RGXCheckFirmwareCCB(PVRSRV_RGXDEV_INFO *psDevInfo);

/*!
*******************************************************************************
@Function       RGXCheckForStalledClientContexts

@Description    Checks all client contexts, for the device with device info
                provided, to see if any are waiting for a fence to signal and
                optionally force signalling of the fence for the context which
                has been waiting the longest.
                This function is called by RGXUpdateHealthStatus() and also
                may be invoked from other trigger points.

@Input          psDevInfo       pointer to device info
@Input          bIgnorePrevious If IMG_TRUE, any stalled contexts will be
                                indicated immediately, rather than only
                                checking against any previous stalled contexts

@Return         None
******************************************************************************/
void RGXCheckForStalledClientContexts(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_BOOL bIgnorePrevious);

/*!
*******************************************************************************
@Function       RGXUpdateHealthStatus

@Description    Tests a number of conditions which might indicate a fatal error
                has occurred in the firmware. The result is stored in the
                device node eHealthStatus.

@Input         psDevNode        Pointer to device node structure.
@Input         bCheckAfterTimePassed  When TRUE, the function will also test
                                for firmware queues and polls not changing
                                since the previous test.

                                Note: if not enough time has passed since the
                                last call, false positives may occur.

@Return        PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXUpdateHealthStatus(PVRSRV_DEVICE_NODE* psDevNode,
                                   IMG_BOOL bCheckAfterTimePassed);

#if defined(SUPPORT_AUTOVZ)
/*!
*******************************************************************************
@Function       RGXUpdateAutoVzWatchdog

@Description    Updates AutoVz watchdog that maintains the fw-driver connection

@Input         psDevNode        Pointer to device node structure.
******************************************************************************/
void RGXUpdateAutoVzWatchdog(PVRSRV_DEVICE_NODE* psDevNode);
#endif /* SUPPORT_AUTOVZ */

/*!
*******************************************************************************
@Function       AttachKickResourcesCleanupCtls

@Description    Attaches the cleanup structures to a kick command so that
                submission reference counting can be performed when the
                firmware processes the command

@Output         apsCleanupCtl   Array of CleanupCtl structure pointers to populate.
@Output         pui32NumCleanupCtl  Number of CleanupCtl structure pointers written out.
@Input          eDM             Which data master is the subject of the command.
@Input          bKick           TRUE if the client originally wanted to kick this DM.
@Input          psRTDataCleanup Optional RTData cleanup associated with the command.
@Input          psZBuffer       Optional ZSBuffer associated with the command.

@Return        PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR AttachKickResourcesCleanupCtls(PRGXFWIF_CLEANUP_CTL *apsCleanupCtl,
									IMG_UINT32 *pui32NumCleanupCtl,
									RGXFWIF_DM eDM,
									IMG_BOOL bKick,
									RGX_KM_HW_RT_DATASET           *psKMHWRTDataSet,
									RGX_ZSBUFFER_DATA              *psZSBuffer,
									RGX_ZSBUFFER_DATA              *psMSAAScratchBuffer);

/*!
*******************************************************************************
@Function       RGXResetHWRLogs

@Description    Resets the HWR Logs buffer
                (the hardware recovery count is not reset)

@Input          psDevNode       Pointer to the device

@Return         PVRSRV_ERROR    PVRSRV_OK on success.
                                Otherwise, a PVRSRV error code
******************************************************************************/
PVRSRV_ERROR RGXResetHWRLogs(PVRSRV_DEVICE_NODE *psDevNode);

/*!
*******************************************************************************
@Function       RGXGetPhyAddr

@Description    Get the physical address of a PMR at an offset within it

@Input          psPMR           PMR of the allocation
@Input          ui32LogicalOffset  Logical offset

@Output         psPhyAddr       Physical address of the allocation

@Return         PVRSRV_ERROR    PVRSRV_OK on success.
                                Otherwise, a PVRSRV error code
******************************************************************************/
PVRSRV_ERROR RGXGetPhyAddr(PMR *psPMR,
						   IMG_DEV_PHYADDR *psPhyAddr,
						   IMG_UINT32 ui32LogicalOffset,
						   IMG_UINT32 ui32Log2PageSize,
						   IMG_UINT32 ui32NumOfPages,
						   IMG_BOOL *bValid);

#if defined(PDUMP)
/*!
*******************************************************************************
@Function       RGXPdumpDrainKCCB

@Description    Wait for the firmware to execute all the commands in the kCCB

@Input          psDevInfo       Pointer to the device
@Input          ui32WriteOffset Woff we have to POL for the Roff to be equal to

@Return         PVRSRV_ERROR    PVRSRV_OK on success.
                                Otherwise, a PVRSRV error code
******************************************************************************/
PVRSRV_ERROR RGXPdumpDrainKCCB(PVRSRV_RGXDEV_INFO *psDevInfo,
							   IMG_UINT32 ui32WriteOffset);
#endif /* PDUMP */

/*!
*******************************************************************************
@Function       RGXFwRawHeapAllocMap

@Description    Register and maps to device, a raw firmware physheap

@Return         PVRSRV_ERROR    PVRSRV_OK on success.
                                Otherwise, a PVRSRV error code
******************************************************************************/
PVRSRV_ERROR RGXFwRawHeapAllocMap(PVRSRV_DEVICE_NODE *psDeviceNode,
								  IMG_UINT32 ui32DriverID,
								  IMG_DEV_PHYADDR sDevPAddr,
								  IMG_UINT64 ui64DevPSize);

/*!
*******************************************************************************
@Function       RGXFwRawHeapUnmapFree

@Description    Unregister and unmap from device, a raw firmware physheap
******************************************************************************/
void RGXFwRawHeapUnmapFree(PVRSRV_DEVICE_NODE *psDeviceNode,
						   IMG_UINT32 ui32DriverID);

/*!
*******************************************************************************
@Function       RGXReadFWModuleAddr

@Description    Read a value at the given address in META or RISCV memory space

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in META or RISCV memory space

@Output         pui32Value      Read value

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXReadFWModuleAddr(PVRSRV_RGXDEV_INFO *psDevInfo,
                                 IMG_UINT32 ui32Addr,
                                 IMG_UINT32 *pui32Value);

/*!
*******************************************************************************
@Function       RGXWriteFWModuleAddr

@Description    Write a value to the given address in META or RISC memory space

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space
@Input          ui32Value       Write value

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXWriteFWModuleAddr(PVRSRV_RGXDEV_INFO *psDevInfo,
                                  IMG_UINT32 ui32MemAddr,
                                  IMG_UINT32 ui32Value);

/*!
*******************************************************************************
@Function       RGXGetFwMapping

@Description    Retrieve any of the CPU Physical Address, Device Physical
                Address or the raw value of the page table entry associated
                with the firmware virtual address given.

@Input          psDevInfo       Pointer to device info
@Input          ui32FwVA        The Fw VA that needs decoding
@Output         psCpuPA         Pointer to the resulting CPU PA
@Output         psDevPA         Pointer to the resulting Dev PA
@Output         pui64RawPTE     Pointer to the raw Page Table Entry value

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXGetFwMapping(PVRSRV_RGXDEV_INFO *psDevInfo,
                                    IMG_UINT32 ui32FwVA,
                                    IMG_CPU_PHYADDR *psCpuPA,
                                    IMG_DEV_PHYADDR *psDevPA,
                                    IMG_UINT64 *pui64RawPTE);

/*!
*******************************************************************************
@Function       RGXFWInjectFault

@Description    Injecting firmware fault to validate recovery through Host

@Input          psDevInfo       Pointer to device info

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXFWInjectFault(PVRSRV_RGXDEV_INFO *psDevInfo);

/*!
*******************************************************************************
@Function       RGXFWSetVzConnectionCooldownPeriod

@Description    Set Vz connection cooldown period

@Input          psDevInfo                              pointer to device info
@Input          ui32VzConnectionCooldownPeriodInSec    Cooldown period in secs
******************************************************************************/
PVRSRV_ERROR
RGXFWSetVzConnectionCooldownPeriod(PVRSRV_RGXDEV_INFO *psDevInfo,
				   IMG_UINT32 ui32VzConnectionCooldownPeriodInSec);

#if defined(SUPPORT_AUTOVZ_HW_REGS) && !defined(SUPPORT_AUTOVZ)
#error "VZ build configuration error: use of OS scratch registers supported only in AutoVz drivers."
#endif

#if defined(SUPPORT_AUTOVZ_HW_REGS)
/* AutoVz with hw support */
#define KM_GET_FW_CONNECTION(psDevInfo)				OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH3)
#define KM_GET_OS_CONNECTION(psDevInfo)				OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH2)
#define KM_SET_OS_CONNECTION(val, psDevInfo)		OSWriteHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH2, RGXFW_CONNECTION_OS_##val)

#define KM_GET_FW_ALIVE_TOKEN(psDevInfo)			OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH1)
#define KM_GET_OS_ALIVE_TOKEN(psDevInfo)			OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH0)
#define KM_SET_OS_ALIVE_TOKEN(val, psDevInfo)		OSWriteHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH0, val)

#define KM_ALIVE_TOKEN_CACHEOP(Target, CacheOp)
#define KM_CONNECTION_CACHEOP(Target, CacheOp)

#else

#if defined(SUPPORT_AUTOVZ)
#define KM_GET_FW_ALIVE_TOKEN(psDevInfo)			(psDevInfo->psRGXFWIfConnectionCtl->ui32AliveFwToken)
#define KM_GET_OS_ALIVE_TOKEN(psDevInfo)			(psDevInfo->psRGXFWIfConnectionCtl->ui32AliveOsToken)
#define KM_SET_OS_ALIVE_TOKEN(val, psDevInfo)		do { \
                                                    OSWriteDeviceMem32WithWMB((volatile IMG_UINT32 *) &psDevInfo->psRGXFWIfConnectionCtl->ui32AliveOsToken, val); \
                                                    KM_ALIVE_TOKEN_CACHEOP(Os, FLUSH); \
                                                    } while (0)

#define KM_ALIVE_TOKEN_CACHEOP(Target, CacheOp)     RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfConnectionCtl->ui32Alive##Target##Token, \
													                           CacheOp);
#endif /* defined(SUPPORT_AUTOVZ) */

#if !defined(NO_HARDWARE) && defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
/* static, dynamic and AutoVz DDKs using shared memory */
#define KM_GET_FW_CONNECTION(psDevInfo)			(psDevInfo->psRGXFWIfConnectionCtl->eConnectionFwState)
#define KM_GET_OS_CONNECTION(psDevInfo)			(psDevInfo->psRGXFWIfConnectionCtl->eConnectionOsState)
#define KM_SET_OS_CONNECTION(val, psDevInfo)	do { \
                                                OSWriteDeviceMem32WithWMB((void*)&psDevInfo->psRGXFWIfConnectionCtl->eConnectionOsState, RGXFW_CONNECTION_OS_##val); \
                                                KM_CONNECTION_CACHEOP(Os, FLUSH); \
                                                } while (0)

#define KM_CONNECTION_CACHEOP(Target, CacheOp)  RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfConnectionCtl->eConnection##Target##State, \
												                           CacheOp);
#else
/* nohw & native */
#define KM_GET_FW_CONNECTION(psDevInfo)			(RGXFW_CONNECTION_FW_ACTIVE)
#define KM_GET_OS_CONNECTION(psDevInfo)			(RGXFW_CONNECTION_OS_ACTIVE)
#define KM_SET_OS_CONNECTION(val, psDevInfo)
#define KM_CONNECTION_CACHEOP(Target, CacheOp)
#endif /* defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED == 1) */
#endif /* defined(SUPPORT_AUTOVZ_HW_REGS) */

#if defined(RGX_PREMAP_FW_HEAPS)
#define RGX_FIRST_RAW_HEAP_DRIVER_ID		RGXFW_HOST_DRIVER_ID
#else
#define RGX_FIRST_RAW_HEAP_DRIVER_ID		RGXFW_GUEST_DRIVER_ID_START
#endif

#define KM_OS_CONNECTION_IS(val, psDevInfo)		(KM_GET_OS_CONNECTION(psDevInfo) == RGXFW_CONNECTION_OS_##val)
#define KM_FW_CONNECTION_IS(val, psDevInfo)		(KM_GET_FW_CONNECTION(psDevInfo) == RGXFW_CONNECTION_FW_##val)

#endif /* RGXFWUTILS_H */
/******************************************************************************
 End of file (rgxfwutils.h)
******************************************************************************/
