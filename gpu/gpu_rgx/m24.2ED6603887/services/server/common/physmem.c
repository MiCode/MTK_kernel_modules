/*************************************************************************/ /*!
@File           physmem.c
@Title          Physmem
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Common entry point for creation of RAM backed PMR's
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
*/ /***************************************************************************/
#include "img_types.h"
#include "img_defs.h"
#include "pvrsrv_error.h"
#include "pvrsrv_memallocflags.h"
#include "device.h"
#include "physmem.h"
#include "pvrsrv.h"
#include "osfunc.h"
#include "pdump_physmem.h"
#include "pdump_km.h"
#include "pvr_ricommon.h"
#include "allocmem.h"

#include "physmem_lma.h"
#include "physmem_osmem.h"

#if defined(DEBUG)
static IMG_UINT32 PMRAllocFail;

#if defined(__linux__)
#include <linux/moduleparam.h>

module_param(PMRAllocFail, uint, 0644);
MODULE_PARM_DESC(PMRAllocFail, "When number of PMR allocs reaches "
				 "this value, it will fail (default value is 0 which "
				 "means that alloc function will behave normally).");
#endif /* defined(__linux__) */
#endif /* defined(DEBUG) */

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#include "process_stats.h"
#include "proc_stats.h"
#endif

/** Computes division using log2 of divisor. */
#define LOG2_DIV(x, log2) ((x) >> (log2))

/** Computes modulo of a power of 2. */
#define LOG2_MOD(x, log2) ((x) & ((1 << (log2)) - 1))

PVRSRV_ERROR DevPhysMemAlloc(PVRSRV_DEVICE_NODE	*psDevNode,
                             IMG_UINT32 ui32MemSize,
                             IMG_UINT32 ui32Log2Align,
                             const IMG_UINT8 u8Value,
                             IMG_BOOL bInitPage,
#if defined(PDUMP)
                             const IMG_CHAR *pszDevSpace,
                             const IMG_CHAR *pszSymbolicAddress,
                             IMG_HANDLE *phHandlePtr,
#endif
                             IMG_PID uiPid,
                             IMG_HANDLE hMemHandle,
                             IMG_DEV_PHYADDR *psDevPhysAddr)
{
	void *pvCpuVAddr;
	PVRSRV_ERROR eError;
#if defined(PDUMP)
	IMG_CHAR szFilenameOut[PDUMP_PARAM_MAX_FILE_NAME];
	PDUMP_FILEOFFSET_T uiOffsetOut;
	IMG_UINT32 ui32PageSize;
	IMG_UINT32 ui32PDumpMemSize = ui32MemSize;
	PVRSRV_ERROR ePDumpError;
#endif
	PG_HANDLE *psMemHandle;
	IMG_UINT64 uiMask;
	IMG_DEV_PHYADDR sDevPhysAddr_int;

	psMemHandle = hMemHandle;

	/* Allocate the pages */
	eError = PhysHeapPagesAlloc(psDevNode->psMMUPhysHeap,
	                            TRUNCATE_64BITS_TO_SIZE_T(ui32MemSize),
	                            psMemHandle,
	                            &sDevPhysAddr_int,
	                            uiPid);
	PVR_LOG_RETURN_IF_ERROR(eError, "pfnDevPxAlloc:1");

	/* Check to see if the page allocator returned pages with our desired
	 * alignment, which is not unlikely
	 */
	uiMask = IMG_PAGE2BYTES32(ui32Log2Align) - 1;
	if (ui32Log2Align && (sDevPhysAddr_int.uiAddr & uiMask))
	{
		/* use over allocation instead */
		PhysHeapPagesFree(psDevNode->psMMUPhysHeap, psMemHandle);

		ui32MemSize += (IMG_UINT32) uiMask;
		eError = PhysHeapPagesAlloc(psDevNode->psMMUPhysHeap,
		                            TRUNCATE_64BITS_TO_SIZE_T(ui32MemSize),
		                            psMemHandle,
		                            &sDevPhysAddr_int,
		                            uiPid);
		PVR_LOG_RETURN_IF_ERROR(eError, "pfnDevPxAlloc:2");

		sDevPhysAddr_int.uiAddr += uiMask;
		sDevPhysAddr_int.uiAddr &= ~uiMask;
	}
	*psDevPhysAddr = sDevPhysAddr_int;

#if defined(PDUMP)
	ui32PageSize = ui32Log2Align? IMG_PAGE2BYTES32(ui32Log2Align) : OSGetPageSize();
	eError = PDumpMalloc(psDevNode,
	                     pszDevSpace,
	                     pszSymbolicAddress,
	                     ui32PDumpMemSize,
	                     ui32PageSize,
	                     IMG_FALSE,
	                     0,
	                     phHandlePtr,
	                     PDUMP_NONE);
	if (PVRSRV_OK != eError)
	{
		PDUMPCOMMENT(psDevNode, "Allocating pages failed");
		*phHandlePtr = NULL;
	}
	ePDumpError = eError;
#endif

	if (bInitPage)
	{
		/*Map the page to the CPU VA space */
		eError = PhysHeapPagesMap(psDevNode->psMMUPhysHeap,
		                          psMemHandle,
		                          ui32MemSize,
		                          &sDevPhysAddr_int,
		                          &pvCpuVAddr);
		if (PVRSRV_OK != eError)
		{
			PVR_LOG_ERROR(eError, "DevPxMap");
			PhysHeapPagesFree(psDevNode->psMMUPhysHeap, psMemHandle);
			return eError;
		}

		/*Fill the memory with given content */
		OSDeviceMemSet(pvCpuVAddr, u8Value, ui32MemSize);

		/*Map the page to the CPU VA space */
		eError = PhysHeapPagesClean(psDevNode->psMMUPhysHeap,
		                            psMemHandle,
		                            0,
		                            ui32MemSize);
		if (PVRSRV_OK != eError)
		{
			PVR_LOG_ERROR(eError, "DevPxClean");
			PhysHeapPagesUnMap(psDevNode->psMMUPhysHeap, psMemHandle, pvCpuVAddr);
			PhysHeapPagesFree(psDevNode->psMMUPhysHeap, psMemHandle);
			return eError;
		}

#if defined(PDUMP)
		if (ePDumpError != PVRSRV_ERROR_PDUMP_CAPTURE_BOUND_TO_ANOTHER_DEVICE)
		{
			/* PDumping of the page contents can be done in two ways
			 * 1. Store the single byte init value to the .prm file
			 *    and load the same value to the entire dummy page buffer
			 *    This method requires lot of LDB's inserted into the out2.txt
			 *
			 * 2. Store the entire contents of the buffer to the .prm file
			 *    and load them back.
			 *    This only needs a single LDB instruction in the .prm file
			 *    and chosen this method
			 *    size of .prm file might go up but that's not huge at least
			 *    for this allocation
			 */
			/* Write the buffer contents to the prm file */
			eError = PDumpWriteParameterBlob(psDevNode,
											 pvCpuVAddr,
											 ui32PDumpMemSize,
											 PDUMP_FLAGS_CONTINUOUS,
											 szFilenameOut,
											 sizeof(szFilenameOut),
											 &uiOffsetOut);
			if (PVRSRV_OK == eError)
			{
				/* Load the buffer back to the allocated memory when playing the pdump */
				eError = PDumpPMRLDB(psDevNode,
									 pszDevSpace,
									 pszSymbolicAddress,
									 0,
									 ui32PDumpMemSize,
									 szFilenameOut,
									 uiOffsetOut,
									 PDUMP_FLAGS_CONTINUOUS);
				if (PVRSRV_OK != eError)
				{
					PDUMP_ERROR(psDevNode, eError, "Failed to write LDB statement to script file");
					PVR_LOG_ERROR(eError, "PDumpPMRLDB");
				}
			}
			else if (eError != PVRSRV_ERROR_PDUMP_NOT_ALLOWED)
			{
				PDUMP_ERROR(psDevNode, eError, "Failed to write device allocation to parameter file");
				PVR_LOG_ERROR(eError, "PDumpWriteParameterBlob");
			}
			else
			{
				/* Else write to parameter file prevented under the flags and
				 * current state of the driver so skip write to script and error IF.
				 * This is expected e.g., if not in the capture range.
				 */
				eError = PVRSRV_OK;
			}
		}
#endif

		/* Unmap the page */
		PhysHeapPagesUnMap(psDevNode->psMMUPhysHeap,
		                   psMemHandle,
		                   pvCpuVAddr);
	}

	return PVRSRV_OK;
}

void DevPhysMemFree(PVRSRV_DEVICE_NODE *psDevNode,
#if defined(PDUMP)
							IMG_HANDLE hPDUMPMemHandle,
#endif
							IMG_HANDLE	hMemHandle)
{
	PG_HANDLE *psMemHandle;

	psMemHandle = hMemHandle;
	PhysHeapPagesFree(psDevNode->psMMUPhysHeap, psMemHandle);
#if defined(PDUMP)
	if (NULL != hPDUMPMemHandle)
	{
		PDumpFree(psDevNode, hPDUMPMemHandle);
	}
#endif

}

PVRSRV_ERROR PhysMemValidateMappingTable(IMG_UINT32 ui32TotalNumVirtChunks,
                                         IMG_UINT32 ui32IndexCount,
                                         const IMG_UINT32 *pui32MappingTable)
{
	IMG_UINT8 *paui8TrackedIndices;
	IMG_UINT32 ui32BytesToTrackIndicies;
	IMG_UINT32 i;
	PVRSRV_ERROR eError = PVRSRV_OK;

	/* Allocate memory for a bitmask to track indices.
	 * We allocate 'n' bytes with 1 bit representing each index, to allow
	 * us to check for any repeated entries in pui32MappingTable.
	 */
	ui32BytesToTrackIndicies = LOG2_DIV(ui32TotalNumVirtChunks, 3);
	if (LOG2_MOD(ui32TotalNumVirtChunks, 3) != 0)
	{
		++ui32BytesToTrackIndicies;
	}
	paui8TrackedIndices = OSAllocZMem(ui32BytesToTrackIndicies);
	if (paui8TrackedIndices == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	for (i = 0; i < ui32IndexCount; i++)
	{
		IMG_UINT32 ui32LogicalIndex = pui32MappingTable[i];

		/* Check that index is within the bounds of the allocation */
		if (ui32LogicalIndex >= ui32TotalNumVirtChunks)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Index %u is OOB",
			         __func__,
			         ui32LogicalIndex));
			eError = PVRSRV_ERROR_PMR_INVALID_MAP_INDEX_ARRAY;
			break;
		}

		/* Check that index is not repeated */
		if (BIT_ISSET(paui8TrackedIndices[LOG2_DIV(ui32LogicalIndex, 3)], LOG2_MOD(ui32LogicalIndex, 3)))
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Duplicate index found: %u",
			         __func__,
			         ui32LogicalIndex));

			eError = PVRSRV_ERROR_PMR_INVALID_MAP_INDEX_ARRAY;
			break;
		}
		BIT_SET(paui8TrackedIndices[LOG2_DIV(ui32LogicalIndex, 3)], LOG2_MOD(ui32LogicalIndex, 3));
	}

	OSFreeMem(paui8TrackedIndices);

	return eError;
}

/* Checks the input parameters and adjusts them if possible and necessary */
PVRSRV_ERROR PhysMemValidateParams(PVRSRV_DEVICE_NODE *psDevNode,
                                   IMG_UINT32 ui32NumPhysChunks,
                                   IMG_UINT32 ui32NumVirtChunks,
                                   IMG_UINT32 *pui32MappingTable,
                                   PVRSRV_MEMALLOCFLAGS_T uiFlags,
                                   IMG_UINT32 *puiLog2AllocPageSize,
                                   IMG_DEVMEM_SIZE_T *puiSize)
{
	IMG_UINT32 uiLog2AllocPageSize = *puiLog2AllocPageSize;
	IMG_UINT32 ui32PageSize = IMG_PAGE2BYTES32(uiLog2AllocPageSize);
	IMG_DEVMEM_SIZE_T uiSize = *puiSize;
	/* Sparse if we have different number of virtual and physical chunks plus
	 * in general all allocations with more than one virtual chunk */
	IMG_BOOL bIsSparse = (ui32NumVirtChunks != ui32NumPhysChunks ||
			ui32NumVirtChunks > 1) ? IMG_TRUE : IMG_FALSE;

	if (PVRSRV_CHECK_ON_DEMAND(uiFlags) &&
	    PVRSRV_CHECK_PHYS_ALLOC_NOW(uiFlags))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid to specify both ON_DEMAND and NOW phys alloc flags: 0x%" IMG_UINT64_FMTSPECX, __func__, uiFlags));
		return PVRSRV_ERROR_INVALID_FLAGS;
	}

	/* Sparse allocations must be backed immediately as the requested
	 * pui32MappingTable is not retained in any structure if not immediately
	 * actioned on allocation.
	 */
	if (PVRSRV_CHECK_ON_DEMAND(uiFlags) && bIsSparse)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid to specify ON_DEMAND for a sparse allocation: 0x%" IMG_UINT64_FMTSPECX, __func__, uiFlags));
		return PVRSRV_ERROR_INVALID_FLAGS;
	}

	if (ui32NumVirtChunks == 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Number of virtual chunks cannot be 0",
		         __func__));

		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* Protect against invalid page sizes */
	if ((ui32PageSize & psDevNode->psMMUDevAttrs->ui32ValidPageSizeMask) == 0)
	{
		PVR_LOG_VA(PVR_DBG_ERROR, "Page size of %u is invalid", ui32PageSize);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* Range check of the alloc size */
	if (!PMRValidateSize(uiSize))
	{
		PVR_LOG_VA(PVR_DBG_ERROR,
				   "PMR size exceeds limit #Chunks: %u ChunkSz 0x%08X",
				   ui32NumVirtChunks,
				   IMG_PAGE2BYTES32(uiLog2AllocPageSize));
		return PVRSRV_ERROR_PMR_TOO_LARGE;
	}

	/* Fail if requesting coherency on one side but uncached on the other */
	if (PVRSRV_CHECK_CPU_CACHE_COHERENT(uiFlags) &&
	    (PVRSRV_CHECK_GPU_UNCACHED(uiFlags) || PVRSRV_CHECK_GPU_WRITE_COMBINE(uiFlags)))
	{
		PVR_DPF((PVR_DBG_ERROR, "Request for CPU coherency but specifying GPU uncached "
				"Please use GPU cached flags for coherency."));
		return PVRSRV_ERROR_UNSUPPORTED_CACHE_MODE;
	}

	if (PVRSRV_CHECK_GPU_CACHE_COHERENT(uiFlags) &&
	    (PVRSRV_CHECK_CPU_UNCACHED(uiFlags) || PVRSRV_CHECK_CPU_WRITE_COMBINE(uiFlags)))
	{
		PVR_DPF((PVR_DBG_ERROR, "Request for GPU coherency but specifying CPU uncached "
				"Please use CPU cached flags for coherency."));
		return PVRSRV_ERROR_UNSUPPORTED_CACHE_MODE;
	}

	if (PVRSRV_CHECK_ZERO_ON_ALLOC(uiFlags) && PVRSRV_CHECK_POISON_ON_ALLOC(uiFlags))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Zero on Alloc and Poison on Alloc are mutually exclusive.",
				__func__));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (bIsSparse)
	{
		/* For sparse we need correct parameters like a suitable page size....  */
		if (OSGetPageShift() > uiLog2AllocPageSize)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Invalid log2-contiguity for sparse allocation. "
					"Requested %u, required minimum %zd",
					__func__,
					uiLog2AllocPageSize,
					OSGetPageShift() ));

			return PVRSRV_ERROR_INVALID_PARAMS;
		}

		if (IMG_PAGES2BYTES64(ui32NumVirtChunks, uiLog2AllocPageSize) != uiSize)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: Total alloc size (%#" IMG_UINT64_FMTSPECx ") "
					 "is not equal to virtual chunks * chunk size "
					 "(%#" IMG_UINT64_FMTSPECx ")",
					 __func__,
					 uiSize,
					 IMG_PAGES2BYTES64(ui32NumVirtChunks, uiLog2AllocPageSize)));

			return PVRSRV_ERROR_PMR_NOT_PAGE_MULTIPLE;
		}

		if (ui32NumPhysChunks > ui32NumVirtChunks)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Number of physical chunks (%u) must not be greater "
					"than number of virtual chunks (%u)",
					__func__,
					ui32NumPhysChunks,
					ui32NumVirtChunks));

			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}
	else
	{
		/*
		 * Silently round up alignment/pagesize if request was less that PAGE_SHIFT
		 * because it would never be harmful for memory to be _more_ contiguous that
		 * was desired.
		 */
		uiLog2AllocPageSize = OSGetPageShift() > uiLog2AllocPageSize ?
				OSGetPageShift() : uiLog2AllocPageSize;

		/* Same for total size */
		uiSize = PVR_ALIGN(uiSize, (IMG_DEVMEM_SIZE_T)OSGetPageSize());
	}

	if ((uiSize & (IMG_PAGE2BYTES32(uiLog2AllocPageSize) - 1)) != 0)
	{
		PVR_DPF((PVR_DBG_ERROR,
		        "%s: Total size (%#" IMG_UINT64_FMTSPECx ") "
		        "must be a multiple of the requested contiguity (%u)",
		        __func__,
		        uiSize,
		        IMG_PAGE2BYTES32(uiLog2AllocPageSize)));
		return PVRSRV_ERROR_PMR_NOT_PAGE_MULTIPLE;
	}

	/* Parameter validation - Mapping table entries */
	{
		PVRSRV_ERROR eErr = PhysMemValidateMappingTable(ui32NumVirtChunks,
		                                                ui32NumPhysChunks,
		                                                pui32MappingTable);
		PVR_RETURN_IF_ERROR(eErr);
	}

	*puiLog2AllocPageSize = uiLog2AllocPageSize;
	*puiSize = uiSize;

	return PVRSRV_OK;
}

static PVRSRV_ERROR _DevPhysHeapFromFlags(PVRSRV_MEMALLOCFLAGS_T uiFlags,
										  PVRSRV_PHYS_HEAP *peDevPhysHeap,
										  PVRSRV_DEVICE_NODE *psDevNode)
{
	PVRSRV_PHYS_HEAP eHeap = PVRSRV_GET_PHYS_HEAP_HINT(uiFlags);

	switch (eHeap)
	{
		case PVRSRV_PHYS_HEAP_FW_PREMAP0:
		case PVRSRV_PHYS_HEAP_FW_PREMAP1:
		case PVRSRV_PHYS_HEAP_FW_PREMAP2:
		case PVRSRV_PHYS_HEAP_FW_PREMAP3:
		case PVRSRV_PHYS_HEAP_FW_PREMAP4:
		case PVRSRV_PHYS_HEAP_FW_PREMAP5:
		case PVRSRV_PHYS_HEAP_FW_PREMAP6:
		case PVRSRV_PHYS_HEAP_FW_PREMAP7:
		{
			/* keep heap (with check) */
			PVR_RETURN_IF_INVALID_PARAM(!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDevNode));
			break;
		}
		case PVRSRV_PHYS_HEAP_LAST:
		{
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
		default:
		{
			break;
		}
	}

	*peDevPhysHeap = eHeap;

	return PVRSRV_OK;
}

static INLINE void _PromoteToCpuCached(PVRSRV_MEMALLOCFLAGS_T *puiFlags)
{
	if ((*puiFlags & (PVRSRV_MEMALLOCFLAG_CPU_READABLE |
	                  PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE |
	                  PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE)) == 0)
	{
		/* We don't need to upgrade if we don't map into the CPU */
		return;
	}

	/* Clear the existing CPU cache flags */
	*puiFlags &= ~(PVRSRV_MEMALLOCFLAG_CPU_CACHE_MODE_MASK);

	/* Add CPU cached flags */
	*puiFlags |= PVRSRV_MEMALLOCFLAG_CPU_CACHE_INCOHERENT;
}

PVRSRV_ERROR
PhysmemNewRamBackedPMR_direct(CONNECTION_DATA *psConnection,
                       PVRSRV_DEVICE_NODE *psDevNode,
                       IMG_DEVMEM_SIZE_T uiSize,
                       IMG_UINT32 ui32NumPhysChunks,
                       IMG_UINT32 ui32NumVirtChunks,
                       IMG_UINT32 *pui32MappingTable,
                       IMG_UINT32 uiLog2AllocPageSize,
                       PVRSRV_MEMALLOCFLAGS_T uiFlags,
                       IMG_UINT32 uiAnnotationLength,
                       const IMG_CHAR *pszAnnotation,
                       IMG_PID uiPid,
                       PMR **ppsPMRPtr,
                       IMG_UINT32 ui32PDumpFlags,
                       PVRSRV_MEMALLOCFLAGS_T *puiPMRFlags)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 i;
	PVRSRV_PHYS_HEAP ePhysHeapIdx;
	PVRSRV_MEMALLOCFLAGS_T uiPMRFlags = uiFlags;
	uiPid = (psConnection != NULL) ? OSGetCurrentClientProcessIDKM() : uiPid;

	/* This is where we would expect to validate the uiAnnotationLength parameter
	   (to confirm it is sufficient to store the string in pszAnnotation plus a
	   terminating NULL). However, we do not make reference to this value when
	   we copy the string in PMRCreatePMR() - instead there we use strlcpy()
	   to copy at most chars and ensure whatever is copied is null-terminated.
	   The parameter is only used by the generated bridge code.
	 */
	PVR_UNREFERENCED_PARAMETER(uiAnnotationLength);

	if (PVRSRVSystemSnoopingOfCPUCache(psDevNode->psDevConfig) &&
		psDevNode->pfnGetDeviceSnoopMode(psDevNode) == PVRSRV_DEVICE_SNOOP_CPU_ONLY)
	{
		_PromoteToCpuCached(&uiPMRFlags);
	}

	eError = PhysMemValidateParams(psDevNode,
	                               ui32NumPhysChunks,
	                               ui32NumVirtChunks,
	                               pui32MappingTable,
	                               uiFlags,
	                               &uiLog2AllocPageSize,
	                               &uiSize);
	PVR_RETURN_IF_ERROR(eError);

#if defined(SUPPORT_STATIC_IPA)
#if !defined(PVRSRV_INTERNAL_IPA_FEATURE_TESTING)
	/* Do not permit IPA PMR allocation flags to be passed through to the
	 * new PMR.
	 */
	uiPMRFlags &= ~PVRSRV_MEMALLOCFLAG_IPA_POLICY_MASK;
#endif
#endif

#if defined(PDUMP)
	eError = PDumpValidateUMFlags(ui32PDumpFlags);
	PVR_RETURN_IF_ERROR(eError);
#endif

	for (i = 0; i < ui32NumPhysChunks; i++)
	{
		PVR_LOG_RETURN_IF_FALSE(pui32MappingTable[i] < ui32NumVirtChunks,
		                        "Mapping table value exceeds ui32NumVirtChunks",
		                        PVRSRV_ERROR_INVALID_PARAMS);
	}

	eError = _DevPhysHeapFromFlags(uiFlags, &ePhysHeapIdx, psDevNode);
	PVR_RETURN_IF_ERROR(eError);

	if (ePhysHeapIdx == PVRSRV_PHYS_HEAP_DEFAULT)
	{
		ePhysHeapIdx = psDevNode->psDevConfig->eDefaultHeap;
		PVRSRV_CHANGE_PHYS_HEAP_HINT(ePhysHeapIdx, uiPMRFlags);
	}

	if (ePhysHeapIdx == PVRSRV_PHYS_HEAP_GPU_LOCAL)
	{
		if ((uiFlags & PVRSRV_MEMALLOCFLAGS_CPU_MAPPABLE_MASK) == 0)
		{
			ePhysHeapIdx = PVRSRV_PHYS_HEAP_GPU_PRIVATE;
			PVRSRV_SET_PHYS_HEAP_HINT(GPU_PRIVATE, uiPMRFlags);
			PVR_DPF((PVR_DBG_VERBOSE, "%s: Consider explicit use of GPU_PRIVATE for PMR %s."
			        " Implicit conversion to GPU PRIVATE performed",
			        __func__, pszAnnotation));
		}
		else if (PVRSRV_CHECK_GPU_CACHE_COHERENT(uiFlags) &&
				 PVRSRVSystemSnoopingOfCPUCache(psDevNode->psDevConfig))
		{
			ePhysHeapIdx = PVRSRV_PHYS_HEAP_GPU_COHERENT;
			PVRSRV_SET_PHYS_HEAP_HINT(GPU_COHERENT, uiPMRFlags);
		}
	}
	else if (ePhysHeapIdx == PVRSRV_PHYS_HEAP_GPU_PRIVATE)
	{
		if (uiFlags & PVRSRV_MEMALLOCFLAGS_CPU_MAPPABLE_MASK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Invalid flags for PMR %s!"
			        " Client requested GPU_PRIVATE physical heap with CPU access flags.",
			        __func__, pszAnnotation));
			return PVRSRV_ERROR_INVALID_HEAP;
		}
	}

	if (NULL == psDevNode->apsPhysHeap[ePhysHeapIdx])
	{
		/* In case a heap hasn't been acquired for this type, return invalid heap error */
		PVR_DPF((PVR_DBG_ERROR, "%s: Requested allocation on device node (%p) from "
		        "an invalid heap (HeapIndex=%d)",
		        __func__, psDevNode, ePhysHeapIdx));
		return PVRSRV_ERROR_INVALID_HEAP;
	}

#if defined(DEBUG)
	if (PMRAllocFail > 0)
	{
		static IMG_UINT32 ui32AllocCount = 1;

		if (ui32AllocCount < PMRAllocFail)
		{
			ui32AllocCount++;
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "%s failed on %d allocation.",
			         __func__, ui32AllocCount));
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}
	}
#endif /* defined(DEBUG) */

	/* If the driver is in an 'init' state all of the allocated memory
	 * should be attributed to the driver (PID 1) rather than to the
	 * process those allocations are made under. Same applies to the memory
	 * allocated for the Firmware. */
	if (psDevNode->eDevState < PVRSRV_DEVICE_STATE_ACTIVE ||
	    PVRSRV_CHECK_FW_MAIN(uiFlags))
	{
		uiPid = PVR_SYS_ALLOC_PID;
	}

	/* ePhysHeapIdx and PhysHeap hint in uiPMRFlags match and provide the
	 * intended PhysHeap to use at this point, but systems vary so the next
	 * call may fallback (apsPhysHeap[]) or demote (OutOfMem) and not be the
	 * heap that was intended, e.g. GPU_PRIVATE index may fallback to GPU_LOCAL,
	 * GPU_LOCAL may demote to CPU_LOCAL.
	 * On output uiPMRFlags show the PhysHeap finally used.
	 */
	eError = PhysHeapCreatePMR(psDevNode->apsPhysHeap[ePhysHeapIdx],
							   psConnection,
							   uiSize,
							   ui32NumPhysChunks,
							   ui32NumVirtChunks,
							   pui32MappingTable,
							   uiLog2AllocPageSize,
							   uiPMRFlags,
							   pszAnnotation,
							   uiPid,
							   ppsPMRPtr,
							   ui32PDumpFlags,
							   &uiPMRFlags);

	if (puiPMRFlags != NULL)
	{
		*puiPMRFlags = uiPMRFlags;
	}

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	if (eError != PVRSRV_OK)
	{
		PVRSRVStatsUpdateOOMStat(psConnection,
		                          psDevNode,
		                          PVRSRV_DEVICE_STAT_TYPE_OOM_PHYSMEM_COUNT,
		                          OSGetCurrentClientProcessIDKM());
	}
#endif

	return eError;
}

PVRSRV_ERROR
PhysmemNewRamBackedPMR(CONNECTION_DATA *psConnection,
                       PVRSRV_DEVICE_NODE *psDevNode,
                       IMG_DEVMEM_SIZE_T uiSize,
                       IMG_UINT32 ui32NumPhysChunks,
                       IMG_UINT32 ui32NumVirtChunks,
                       IMG_UINT32 *pui32MappingTable,
                       IMG_UINT32 uiLog2AllocPageSize,
                       PVRSRV_MEMALLOCFLAGS_T uiFlags,
                       IMG_UINT32 uiAnnotationLength,
                       const IMG_CHAR *pszAnnotation,
                       IMG_PID uiPid,
                       PMR **ppsPMRPtr,
                       IMG_UINT32 ui32PDumpFlags,
                       PVRSRV_MEMALLOCFLAGS_T *puiPMRFlags)
{
	PVRSRV_PHYS_HEAP ePhysHeap = PVRSRV_GET_PHYS_HEAP_HINT(uiFlags);
	PVRSRV_ERROR eError;

	PVR_LOG_RETURN_IF_INVALID_PARAM(ePhysHeap < PVRSRV_PHYS_HEAP_LAST, "uiFlags");
	PVR_LOG_RETURN_IF_INVALID_PARAM(uiAnnotationLength != 0, "uiAnnotationLength");
	PVR_LOG_RETURN_IF_INVALID_PARAM(pszAnnotation != NULL, "pszAnnotation");

	if (ePhysHeap == PVRSRV_PHYS_HEAP_DEFAULT)
	{
		ePhysHeap = psDevNode->psDevConfig->eDefaultHeap;
	}

	if (!PhysHeapUserModeAlloc(ePhysHeap))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid phys heap hint: %d.", __func__, ePhysHeap));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = PhysmemNewRamBackedPMR_direct(psConnection,
										  psDevNode,
										  uiSize,
										  ui32NumPhysChunks,
										  ui32NumVirtChunks,
										  pui32MappingTable,
										  uiLog2AllocPageSize,
										  uiFlags,
										  uiAnnotationLength,
										  pszAnnotation,
										  uiPid,
										  ppsPMRPtr,
										  ui32PDumpFlags,
										  puiPMRFlags);
	if (eError == PVRSRV_OK)
	{
		/* Lock phys addresses if backing was allocated */
		if (PVRSRV_CHECK_PHYS_ALLOC_NOW(uiFlags))
		{
			eError = PMRLockSysPhysAddresses(*ppsPMRPtr);
		}
	}

	return eError;
}

PVRSRV_ERROR
PVRSRVGetDefaultPhysicalHeapKM(CONNECTION_DATA *psConnection,
                               PVRSRV_DEVICE_NODE *psDevNode,
                               PVRSRV_PHYS_HEAP *peHeap)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	*peHeap = psDevNode->psDevConfig->eDefaultHeap;
	return PVRSRV_OK;
}

PVRSRV_ERROR
PVRSRVPhysHeapGetMemInfoKM(CONNECTION_DATA *psConnection,
                           PVRSRV_DEVICE_NODE *psDevNode,
                           IMG_UINT32 ui32PhysHeapCount,
                           PVRSRV_PHYS_HEAP *paePhysHeapID,
                           PHYS_HEAP_MEM_STATS *paPhysHeapMemStats)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	return PhysHeapGetMemInfo(psDevNode,
	                          ui32PhysHeapCount,
	                          paePhysHeapID,
	                          paPhysHeapMemStats);
}

/* 'Wrapper' function to call PMRImportPMR(), which first checks the PMR is
 * for the current device. This avoids the need to do this in pmr.c, which
 * would then need PVRSRV_DEVICE_NODE (defining this type in pmr.h causes a
 * typedef redefinition issue).
 */
#if defined(SUPPORT_INSECURE_EXPORT)
PVRSRV_ERROR
PhysmemImportPMR(CONNECTION_DATA *psConnection,
             PVRSRV_DEVICE_NODE *psDevNode,
             PMR_EXPORT *psPMRExport,
             PMR_PASSWORD_T uiPassword,
             PMR_SIZE_T uiSize,
             PMR_LOG2ALIGN_T uiLog2Contig,
             PMR **ppsPMR)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);

	if (PMRGetExportDeviceNode(psPMRExport) != psDevNode)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: PMR invalid for this device", __func__));
		return PVRSRV_ERROR_PMR_NOT_PERMITTED;
	}

	return PMRImportPMR(psPMRExport,
	                    uiPassword,
	                    uiSize,
	                    uiLog2Contig,
	                    ppsPMR);
}
#endif /* if defined(SUPPORT_INSECURE_EXPORT) */
