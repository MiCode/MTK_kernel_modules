/*******************************************************************************
@File
@Title          Direct client bridge for mm
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Implements the client side of the bridge for mm
                which is used in calls from Server context.
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
*******************************************************************************/

#include "client_mm_bridge.h"
#include "img_defs.h"
#include "pvr_debug.h"

/* Module specific includes */
#include "pvrsrv_memallocflags.h"
#include "pvrsrv_memalloc_physheap.h"
#include "devicemem_typedefs.h"

#include "pvrsrv_memalloc_physheap.h"
#include "devicemem.h"
#include "devicemem_server.h"
#include "pmr.h"
#include "devicemem_heapcfg.h"
#include "physmem.h"
#include "devicemem_utils.h"
#include "process_stats.h"

IMG_INTERNAL PVRSRV_ERROR BridgePMRExportPMR(IMG_HANDLE hBridge,
					     IMG_HANDLE hPMR,
					     IMG_HANDLE * phPMRExport,
					     IMG_UINT64 * pui64Size,
					     IMG_UINT32 * pui32Log2Contig,
					     IMG_UINT64 * pui64Password)
{
#if defined(SUPPORT_INSECURE_EXPORT)
	PVRSRV_ERROR eError;
	PMR *psPMRInt;
	PMR_EXPORT *psPMRExportInt = NULL;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psPMRInt = (PMR *) hPMR;

	eError = PMRExportPMR(psPMRInt, &psPMRExportInt, pui64Size, pui32Log2Contig, pui64Password);

	*phPMRExport = psPMRExportInt;
	return eError;
#else
	PVR_UNREFERENCED_PARAMETER(hBridge);
	PVR_UNREFERENCED_PARAMETER(hPMR);
	PVR_UNREFERENCED_PARAMETER(phPMRExport);
	PVR_UNREFERENCED_PARAMETER(pui64Size);
	PVR_UNREFERENCED_PARAMETER(pui32Log2Contig);
	PVR_UNREFERENCED_PARAMETER(pui64Password);

	return PVRSRV_ERROR_NOT_IMPLEMENTED;
#endif
}

IMG_INTERNAL PVRSRV_ERROR BridgePMRUnexportPMR(IMG_HANDLE hBridge, IMG_HANDLE hPMRExport)
{
#if defined(SUPPORT_INSECURE_EXPORT)
	PVRSRV_ERROR eError;
	PMR_EXPORT *psPMRExportInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psPMRExportInt = (PMR_EXPORT *) hPMRExport;

	eError = PMRUnexportPMR(psPMRExportInt);

	return eError;
#else
	PVR_UNREFERENCED_PARAMETER(hBridge);
	PVR_UNREFERENCED_PARAMETER(hPMRExport);

	return PVRSRV_ERROR_NOT_IMPLEMENTED;
#endif
}

IMG_INTERNAL PVRSRV_ERROR BridgePMRGetUID(IMG_HANDLE hBridge,
					  IMG_HANDLE hPMR, IMG_UINT64 * pui64UID)
{
	PVRSRV_ERROR eError;
	PMR *psPMRInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psPMRInt = (PMR *) hPMR;

	eError = PMRGetUID(psPMRInt, pui64UID);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgePMRMakeLocalImportHandle(IMG_HANDLE hBridge,
							 IMG_HANDLE hBuffer, IMG_HANDLE * phExtMem)
{
	PVRSRV_ERROR eError;
	PMR *psBufferInt;
	PMR *psExtMemInt = NULL;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psBufferInt = (PMR *) hBuffer;

	eError = PMRMakeLocalImportHandle(psBufferInt, &psExtMemInt);

	*phExtMem = psExtMemInt;
	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgePMRUnmakeLocalImportHandle(IMG_HANDLE hBridge, IMG_HANDLE hExtMem)
{
	PVRSRV_ERROR eError;
	PMR *psExtMemInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psExtMemInt = (PMR *) hExtMem;

	eError = PMRUnmakeLocalImportHandle(psExtMemInt);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgePMRImportPMR(IMG_HANDLE hBridge,
					     IMG_HANDLE hPMRExport,
					     IMG_UINT64 ui64uiPassword,
					     IMG_UINT64 ui64uiSize,
					     IMG_UINT32 ui32uiLog2Contig, IMG_HANDLE * phPMR)
{
#if defined(SUPPORT_INSECURE_EXPORT)
	PVRSRV_ERROR eError;
	PMR_EXPORT *psPMRExportInt;
	PMR *psPMRInt = NULL;

	psPMRExportInt = (PMR_EXPORT *) hPMRExport;

	eError =
	    PhysmemImportPMR(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
			     psPMRExportInt,
			     ui64uiPassword, ui64uiSize, ui32uiLog2Contig, &psPMRInt);

	*phPMR = psPMRInt;
	return eError;
#else
	PVR_UNREFERENCED_PARAMETER(hPMRExport);
	PVR_UNREFERENCED_PARAMETER(ui64uiPassword);
	PVR_UNREFERENCED_PARAMETER(ui64uiSize);
	PVR_UNREFERENCED_PARAMETER(ui32uiLog2Contig);
	PVR_UNREFERENCED_PARAMETER(phPMR);

	return PVRSRV_ERROR_NOT_IMPLEMENTED;
#endif
}

IMG_INTERNAL PVRSRV_ERROR BridgePMRLocalImportPMR(IMG_HANDLE hBridge,
						  IMG_HANDLE hExtHandle,
						  IMG_HANDLE * phPMR,
						  IMG_DEVMEM_SIZE_T * puiSize,
						  IMG_DEVMEM_ALIGN_T * puiAlign)
{
	PVRSRV_ERROR eError;
	PMR *psExtHandleInt;
	PMR *psPMRInt = NULL;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psExtHandleInt = (PMR *) hExtHandle;

	eError = PMRLocalImportPMR(psExtHandleInt, &psPMRInt, puiSize, puiAlign);

	*phPMR = psPMRInt;
	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgePMRUnrefPMR(IMG_HANDLE hBridge, IMG_HANDLE hPMR)
{
	PVRSRV_ERROR eError;
	PMR *psPMRInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psPMRInt = (PMR *) hPMR;

	eError = PMRUnrefPMR(psPMRInt);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgePhysmemNewRamBackedPMR(IMG_HANDLE hBridge,
						       IMG_DEVMEM_SIZE_T uiSize,
						       IMG_UINT32 ui32NumPhysChunks,
						       IMG_UINT32 ui32NumVirtChunks,
						       IMG_UINT32 * pui32MappingTable,
						       IMG_UINT32 ui32Log2PageSize,
						       PVRSRV_MEMALLOCFLAGS_T uiFlags,
						       IMG_UINT32 ui32AnnotationLength,
						       const IMG_CHAR * puiAnnotation,
						       IMG_PID ui32PID,
						       IMG_HANDLE * phPMRPtr,
						       IMG_UINT32 ui32PDumpFlags,
						       PVRSRV_MEMALLOCFLAGS_T * puiOutFlags)
{
	PVRSRV_ERROR eError;
	PMR *psPMRPtrInt = NULL;

	eError =
	    PhysmemNewRamBackedPMR_direct(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
					  uiSize,
					  ui32NumPhysChunks,
					  ui32NumVirtChunks,
					  pui32MappingTable,
					  ui32Log2PageSize,
					  uiFlags,
					  ui32AnnotationLength,
					  puiAnnotation,
					  ui32PID, &psPMRPtrInt, ui32PDumpFlags, puiOutFlags);

	*phPMRPtr = psPMRPtrInt;
	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntCtxCreate(IMG_HANDLE hBridge,
						   IMG_BOOL bbKernelMemoryCtx,
						   IMG_HANDLE * phDevMemServerContext,
						   IMG_HANDLE * phPrivData,
						   IMG_UINT32 * pui32CPUCacheLineSize)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_CTX *psDevMemServerContextInt = NULL;
	IMG_HANDLE hPrivDataInt = NULL;

	eError =
	    DevmemIntCtxCreate(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
			       bbKernelMemoryCtx,
			       &psDevMemServerContextInt, &hPrivDataInt, pui32CPUCacheLineSize);

	*phDevMemServerContext = psDevMemServerContextInt;
	*phPrivData = hPrivDataInt;
	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntCtxDestroy(IMG_HANDLE hBridge,
						    IMG_HANDLE hDevmemServerContext)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_CTX *psDevmemServerContextInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psDevmemServerContextInt = (DEVMEMINT_CTX *) hDevmemServerContext;

	eError = DevmemIntCtxDestroy(psDevmemServerContextInt);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntHeapCreate(IMG_HANDLE hBridge,
						    IMG_HANDLE hDevmemCtx,
						    IMG_UINT32 ui32HeapConfigIndex,
						    IMG_UINT32 ui32HeapIndex,
						    IMG_HANDLE * phDevmemHeapPtr)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_CTX *psDevmemCtxInt;
	DEVMEMINT_HEAP *psDevmemHeapPtrInt = NULL;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psDevmemCtxInt = (DEVMEMINT_CTX *) hDevmemCtx;

	eError =
	    DevmemIntHeapCreate(psDevmemCtxInt,
				ui32HeapConfigIndex, ui32HeapIndex, &psDevmemHeapPtrInt);

	*phDevmemHeapPtr = psDevmemHeapPtrInt;
	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntHeapDestroy(IMG_HANDLE hBridge, IMG_HANDLE hDevmemHeap)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_HEAP *psDevmemHeapInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psDevmemHeapInt = (DEVMEMINT_HEAP *) hDevmemHeap;

	eError = DevmemIntHeapDestroy(psDevmemHeapInt);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntMapPMR(IMG_HANDLE hBridge,
						IMG_HANDLE hReservation, IMG_HANDLE hPMR)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_RESERVATION *psReservationInt;
	PMR *psPMRInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psReservationInt = (DEVMEMINT_RESERVATION *) hReservation;
	psPMRInt = (PMR *) hPMR;

	eError = DevmemIntMapPMR(psReservationInt, psPMRInt);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntUnmapPMR(IMG_HANDLE hBridge, IMG_HANDLE hReservation)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_RESERVATION *psReservationInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psReservationInt = (DEVMEMINT_RESERVATION *) hReservation;

	eError = DevmemIntUnmapPMR(psReservationInt);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntReserveRange(IMG_HANDLE hBridge,
						      IMG_HANDLE hDevmemServerHeap,
						      IMG_DEV_VIRTADDR sAddress,
						      IMG_DEVMEM_SIZE_T uiLength,
						      PVRSRV_MEMALLOCFLAGS_T uiFlags,
						      IMG_HANDLE * phReservation)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_HEAP *psDevmemServerHeapInt;
	DEVMEMINT_RESERVATION *psReservationInt = NULL;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psDevmemServerHeapInt = (DEVMEMINT_HEAP *) hDevmemServerHeap;

	eError =
	    DevmemIntReserveRange(psDevmemServerHeapInt,
				  sAddress, uiLength, uiFlags, &psReservationInt);

	*phReservation = psReservationInt;
	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntReserveRangeAndMapPMR(IMG_HANDLE hBridge,
							       IMG_HANDLE hDevmemServerHeap,
							       IMG_DEV_VIRTADDR sAddress,
							       IMG_DEVMEM_SIZE_T uiLength,
							       IMG_HANDLE hPMR,
							       PVRSRV_MEMALLOCFLAGS_T uiFlags,
							       IMG_HANDLE * phReservation)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_HEAP *psDevmemServerHeapInt;
	PMR *psPMRInt;
	DEVMEMINT_RESERVATION *psReservationInt = NULL;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psDevmemServerHeapInt = (DEVMEMINT_HEAP *) hDevmemServerHeap;
	psPMRInt = (PMR *) hPMR;

	eError =
	    DevmemIntReserveRangeAndMapPMR(psDevmemServerHeapInt,
					   sAddress,
					   uiLength, psPMRInt, uiFlags, &psReservationInt);

	*phReservation = psReservationInt;
	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntUnreserveRange(IMG_HANDLE hBridge, IMG_HANDLE hReservation)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_RESERVATION *psReservationInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psReservationInt = (DEVMEMINT_RESERVATION *) hReservation;

	eError = DevmemIntUnreserveRange(psReservationInt);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeChangeSparseMem(IMG_HANDLE hBridge,
						IMG_UINT32 ui32AllocPageCount,
						IMG_UINT32 * pui32AllocPageIndices,
						IMG_UINT32 ui32FreePageCount,
						IMG_UINT32 * pui32FreePageIndices,
						IMG_UINT32 ui32SparseFlags, IMG_HANDLE hReservation)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_RESERVATION *psReservationInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psReservationInt = (DEVMEMINT_RESERVATION *) hReservation;

	eError =
	    DevmemIntChangeSparse(ui32AllocPageCount,
				  pui32AllocPageIndices,
				  ui32FreePageCount,
				  pui32FreePageIndices, ui32SparseFlags, psReservationInt);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIsVDevAddrValid(IMG_HANDLE hBridge,
						      IMG_HANDLE hDevmemCtx,
						      IMG_DEV_VIRTADDR sAddress)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_CTX *psDevmemCtxInt;

	psDevmemCtxInt = (DEVMEMINT_CTX *) hDevmemCtx;

	eError =
	    DevmemIntIsVDevAddrValid(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
				     psDevmemCtxInt, sAddress);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemInvalidateFBSCTable(IMG_HANDLE hBridge,
							  IMG_HANDLE hDevmemCtx,
							  IMG_UINT64 ui64FBSCEntries)
{
#if defined(RGX_FEATURE_FBCDC)
	PVRSRV_ERROR eError;
	DEVMEMINT_CTX *psDevmemCtxInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psDevmemCtxInt = (DEVMEMINT_CTX *) hDevmemCtx;

	eError = DevmemIntInvalidateFBSCTable(psDevmemCtxInt, ui64FBSCEntries);

	return eError;
#else
	PVR_UNREFERENCED_PARAMETER(hBridge);
	PVR_UNREFERENCED_PARAMETER(hDevmemCtx);
	PVR_UNREFERENCED_PARAMETER(ui64FBSCEntries);

	return PVRSRV_ERROR_NOT_IMPLEMENTED;
#endif
}

IMG_INTERNAL PVRSRV_ERROR BridgeHeapCfgHeapConfigCount(IMG_HANDLE hBridge,
						       IMG_UINT32 * pui32NumHeapConfigs)
{
	PVRSRV_ERROR eError;

	eError =
	    HeapCfgHeapConfigCount(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
				   pui32NumHeapConfigs);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeHeapCfgHeapCount(IMG_HANDLE hBridge,
						 IMG_UINT32 ui32HeapConfigIndex,
						 IMG_UINT32 * pui32NumHeaps)
{
	PVRSRV_ERROR eError;

	eError =
	    HeapCfgHeapCount(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
			     ui32HeapConfigIndex, pui32NumHeaps);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeHeapCfgHeapConfigName(IMG_HANDLE hBridge,
						      IMG_UINT32 ui32HeapConfigIndex,
						      IMG_UINT32 ui32HeapConfigNameBufSz,
						      IMG_CHAR * puiHeapConfigName)
{
	PVRSRV_ERROR eError;

	eError =
	    HeapCfgHeapConfigName(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
				  ui32HeapConfigIndex, ui32HeapConfigNameBufSz, puiHeapConfigName);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeHeapCfgHeapDetails(IMG_HANDLE hBridge,
						   IMG_UINT32 ui32HeapConfigIndex,
						   IMG_UINT32 ui32HeapIndex,
						   IMG_UINT32 ui32HeapNameBufSz,
						   IMG_CHAR * puiHeapNameOut,
						   IMG_DEV_VIRTADDR * psDevVAddrBase,
						   IMG_DEVMEM_SIZE_T * puiHeapLength,
						   IMG_DEVMEM_SIZE_T * puiReservedRegionLength,
						   IMG_UINT32 * pui32Log2DataPageSizeOut,
						   IMG_UINT32 * pui32Log2ImportAlignmentOut)
{
	PVRSRV_ERROR eError;

	eError =
	    HeapCfgHeapDetails(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
			       ui32HeapConfigIndex,
			       ui32HeapIndex,
			       ui32HeapNameBufSz,
			       puiHeapNameOut,
			       psDevVAddrBase,
			       puiHeapLength,
			       puiReservedRegionLength,
			       pui32Log2DataPageSizeOut, pui32Log2ImportAlignmentOut);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntRegisterPFNotifyKM(IMG_HANDLE hBridge,
							    IMG_HANDLE hDevmemCtx,
							    IMG_BOOL bRegister)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_CTX *psDevmemCtxInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psDevmemCtxInt = (DEVMEMINT_CTX *) hDevmemCtx;

	eError = DevmemIntRegisterPFNotifyKM(psDevmemCtxInt, bRegister);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgePhysHeapGetMemInfo(IMG_HANDLE hBridge,
						   IMG_UINT32 ui32PhysHeapCount,
						   PVRSRV_PHYS_HEAP * peaPhysHeapID,
						   PHYS_HEAP_MEM_STATS * pasapPhysHeapMemStats)
{
	PVRSRV_ERROR eError;

	eError =
	    PVRSRVPhysHeapGetMemInfoKM(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
				       ui32PhysHeapCount, peaPhysHeapID, pasapPhysHeapMemStats);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeGetDefaultPhysicalHeap(IMG_HANDLE hBridge,
						       PVRSRV_PHYS_HEAP * peHeap)
{
	PVRSRV_ERROR eError;

	eError =
	    PVRSRVGetDefaultPhysicalHeapKM(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge), peHeap);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemGetFaultAddress(IMG_HANDLE hBridge,
						      IMG_HANDLE hDevmemCtx,
						      IMG_DEV_VIRTADDR * psFaultAddress)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_CTX *psDevmemCtxInt;

	psDevmemCtxInt = (DEVMEMINT_CTX *) hDevmemCtx;

	eError =
	    DevmemIntGetFaultAddress(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
				     psDevmemCtxInt, psFaultAddress);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgePVRSRVStatsUpdateOOMStat(IMG_HANDLE hBridge,
							 IMG_UINT32 ui32ui32StatType,
							 IMG_PID ui32pid)
{
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	PVRSRV_ERROR eError;

	eError =
	    PVRSRVStatsUpdateOOMStat(NULL, (PVRSRV_DEVICE_NODE *) ((void *)hBridge),
				     ui32ui32StatType, ui32pid);

	return eError;
#else
	PVR_UNREFERENCED_PARAMETER(ui32ui32StatType);
	PVR_UNREFERENCED_PARAMETER(ui32pid);

	return PVRSRV_ERROR_NOT_IMPLEMENTED;
#endif
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemXIntReserveRange(IMG_HANDLE hBridge,
						       IMG_HANDLE hDevmemServerHeap,
						       IMG_DEV_VIRTADDR sAddress,
						       IMG_DEVMEM_SIZE_T uiLength,
						       IMG_HANDLE * phReservation)
{
	PVRSRV_ERROR eError;
	DEVMEMINT_HEAP *psDevmemServerHeapInt;
	DEVMEMXINT_RESERVATION *psReservationInt = NULL;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psDevmemServerHeapInt = (DEVMEMINT_HEAP *) hDevmemServerHeap;

	eError =
	    DevmemXIntReserveRange(psDevmemServerHeapInt, sAddress, uiLength, &psReservationInt);

	*phReservation = psReservationInt;
	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemXIntUnreserveRange(IMG_HANDLE hBridge,
							 IMG_HANDLE hReservation)
{
	PVRSRV_ERROR eError;
	DEVMEMXINT_RESERVATION *psReservationInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psReservationInt = (DEVMEMXINT_RESERVATION *) hReservation;

	eError = DevmemXIntUnreserveRange(psReservationInt);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemXIntMapPages(IMG_HANDLE hBridge,
						   IMG_HANDLE hReservation,
						   IMG_HANDLE hPMR,
						   IMG_UINT32 ui32PageCount,
						   IMG_UINT32 ui32PhysPageOffset,
						   PVRSRV_MEMALLOCFLAGS_T uiFlags,
						   IMG_UINT32 ui32VirtPageOffset)
{
	PVRSRV_ERROR eError;
	DEVMEMXINT_RESERVATION *psReservationInt;
	PMR *psPMRInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psReservationInt = (DEVMEMXINT_RESERVATION *) hReservation;
	psPMRInt = (PMR *) hPMR;

	eError =
	    DevmemXIntMapPages(psReservationInt,
			       psPMRInt,
			       ui32PageCount, ui32PhysPageOffset, uiFlags, ui32VirtPageOffset);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemXIntUnmapPages(IMG_HANDLE hBridge,
						     IMG_HANDLE hReservation,
						     IMG_UINT32 ui32VirtPageOffset,
						     IMG_UINT32 ui32PageCount)
{
	PVRSRV_ERROR eError;
	DEVMEMXINT_RESERVATION *psReservationInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psReservationInt = (DEVMEMXINT_RESERVATION *) hReservation;

	eError = DevmemXIntUnmapPages(psReservationInt, ui32VirtPageOffset, ui32PageCount);

	return eError;
}

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemXIntMapVRangeToBackingPage(IMG_HANDLE hBridge,
								 IMG_HANDLE hReservation,
								 IMG_UINT32 ui32PageCount,
								 PVRSRV_MEMALLOCFLAGS_T uiFlags,
								 IMG_UINT32 ui32VirtPageOffset)
{
	PVRSRV_ERROR eError;
	DEVMEMXINT_RESERVATION *psReservationInt;
	PVR_UNREFERENCED_PARAMETER(hBridge);

	psReservationInt = (DEVMEMXINT_RESERVATION *) hReservation;

	eError =
	    DevmemXIntMapVRangeToBackingPage(psReservationInt,
					     ui32PageCount, uiFlags, ui32VirtPageOffset);

	return eError;
}
