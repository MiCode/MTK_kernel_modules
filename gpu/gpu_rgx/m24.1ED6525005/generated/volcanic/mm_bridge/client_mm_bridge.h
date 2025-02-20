/*******************************************************************************
@File
@Title          Client bridge header for mm
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Exports the client bridge functions for mm
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

#ifndef CLIENT_MM_BRIDGE_H
#define CLIENT_MM_BRIDGE_H

#include "img_defs.h"
#include "pvrsrv_error.h"

#if defined(PVR_INDIRECT_BRIDGE_CLIENTS)
#include "pvr_bridge_client.h"
#include "pvr_bridge.h"
#endif

#include "common_mm_bridge.h"

IMG_INTERNAL PVRSRV_ERROR BridgePMRExportPMR(IMG_HANDLE hBridge,
					     IMG_HANDLE hPMR,
					     IMG_HANDLE * phPMRExport,
					     IMG_UINT64 * pui64Size,
					     IMG_UINT32 * pui32Log2Contig,
					     IMG_UINT64 * pui64Password);

IMG_INTERNAL PVRSRV_ERROR BridgePMRUnexportPMR(IMG_HANDLE hBridge, IMG_HANDLE hPMRExport);

IMG_INTERNAL PVRSRV_ERROR BridgePMRGetUID(IMG_HANDLE hBridge,
					  IMG_HANDLE hPMR, IMG_UINT64 * pui64UID);

IMG_INTERNAL PVRSRV_ERROR BridgePMRMakeLocalImportHandle(IMG_HANDLE hBridge,
							 IMG_HANDLE hBuffer, IMG_HANDLE * phExtMem);

IMG_INTERNAL PVRSRV_ERROR BridgePMRUnmakeLocalImportHandle(IMG_HANDLE hBridge, IMG_HANDLE hExtMem);

IMG_INTERNAL PVRSRV_ERROR BridgePMRImportPMR(IMG_HANDLE hBridge,
					     IMG_HANDLE hPMRExport,
					     IMG_UINT64 ui64uiPassword,
					     IMG_UINT64 ui64uiSize,
					     IMG_UINT32 ui32uiLog2Contig, IMG_HANDLE * phPMR);

IMG_INTERNAL PVRSRV_ERROR BridgePMRLocalImportPMR(IMG_HANDLE hBridge,
						  IMG_HANDLE hExtHandle,
						  IMG_HANDLE * phPMR,
						  IMG_DEVMEM_SIZE_T * puiSize,
						  IMG_DEVMEM_ALIGN_T * puiAlign);

IMG_INTERNAL PVRSRV_ERROR BridgePMRUnrefPMR(IMG_HANDLE hBridge, IMG_HANDLE hPMR);

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
						       PVRSRV_MEMALLOCFLAGS_T * puiOutFlags);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntCtxCreate(IMG_HANDLE hBridge,
						   IMG_BOOL bbKernelMemoryCtx,
						   IMG_HANDLE * phDevMemServerContext,
						   IMG_HANDLE * phPrivData,
						   IMG_UINT32 * pui32CPUCacheLineSize);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntCtxDestroy(IMG_HANDLE hBridge,
						    IMG_HANDLE hDevmemServerContext);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntHeapCreate(IMG_HANDLE hBridge,
						    IMG_HANDLE hDevmemCtx,
						    IMG_UINT32 ui32HeapConfigIndex,
						    IMG_UINT32 ui32HeapIndex,
						    IMG_HANDLE * phDevmemHeapPtr);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntHeapDestroy(IMG_HANDLE hBridge, IMG_HANDLE hDevmemHeap);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntMapPMR(IMG_HANDLE hBridge,
						IMG_HANDLE hReservation, IMG_HANDLE hPMR);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntUnmapPMR(IMG_HANDLE hBridge, IMG_HANDLE hReservation);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntReserveRange(IMG_HANDLE hBridge,
						      IMG_HANDLE hDevmemServerHeap,
						      IMG_DEV_VIRTADDR sAddress,
						      IMG_DEVMEM_SIZE_T uiLength,
						      PVRSRV_MEMALLOCFLAGS_T uiFlags,
						      IMG_HANDLE * phReservation);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntReserveRangeAndMapPMR(IMG_HANDLE hBridge,
							       IMG_HANDLE hDevmemServerHeap,
							       IMG_DEV_VIRTADDR sAddress,
							       IMG_DEVMEM_SIZE_T uiLength,
							       IMG_HANDLE hPMR,
							       PVRSRV_MEMALLOCFLAGS_T uiFlags,
							       IMG_HANDLE * phReservation);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntUnreserveRange(IMG_HANDLE hBridge,
							IMG_HANDLE hReservation);

IMG_INTERNAL PVRSRV_ERROR BridgeChangeSparseMem(IMG_HANDLE hBridge,
						IMG_UINT32 ui32AllocPageCount,
						IMG_UINT32 * pui32AllocPageIndices,
						IMG_UINT32 ui32FreePageCount,
						IMG_UINT32 * pui32FreePageIndices,
						IMG_UINT32 ui32SparseFlags,
						IMG_HANDLE hReservation);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIsVDevAddrValid(IMG_HANDLE hBridge,
						      IMG_HANDLE hDevmemCtx,
						      IMG_DEV_VIRTADDR sAddress);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemInvalidateFBSCTable(IMG_HANDLE hBridge,
							  IMG_HANDLE hDevmemCtx,
							  IMG_UINT64 ui64FBSCEntries);

IMG_INTERNAL PVRSRV_ERROR BridgeHeapCfgHeapConfigCount(IMG_HANDLE hBridge,
						       IMG_UINT32 * pui32NumHeapConfigs);

IMG_INTERNAL PVRSRV_ERROR BridgeHeapCfgHeapCount(IMG_HANDLE hBridge,
						 IMG_UINT32 ui32HeapConfigIndex,
						 IMG_UINT32 * pui32NumHeaps);

IMG_INTERNAL PVRSRV_ERROR BridgeHeapCfgHeapConfigName(IMG_HANDLE hBridge,
						      IMG_UINT32 ui32HeapConfigIndex,
						      IMG_UINT32 ui32HeapConfigNameBufSz,
						      IMG_CHAR * puiHeapConfigName);

IMG_INTERNAL PVRSRV_ERROR BridgeHeapCfgHeapDetails(IMG_HANDLE hBridge,
						   IMG_UINT32 ui32HeapConfigIndex,
						   IMG_UINT32 ui32HeapIndex,
						   IMG_UINT32 ui32HeapNameBufSz,
						   IMG_CHAR * puiHeapNameOut,
						   IMG_DEV_VIRTADDR * psDevVAddrBase,
						   IMG_DEVMEM_SIZE_T * puiHeapLength,
						   IMG_DEVMEM_SIZE_T * puiReservedRegionLength,
						   IMG_UINT32 * pui32Log2DataPageSizeOut,
						   IMG_UINT32 * pui32Log2ImportAlignmentOut);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemIntRegisterPFNotifyKM(IMG_HANDLE hBridge,
							    IMG_HANDLE hDevmemCtx,
							    IMG_BOOL bRegister);

IMG_INTERNAL PVRSRV_ERROR BridgePhysHeapGetMemInfo(IMG_HANDLE hBridge,
						   IMG_UINT32 ui32PhysHeapCount,
						   PVRSRV_PHYS_HEAP * peaPhysHeapID,
						   PHYS_HEAP_MEM_STATS * pasapPhysHeapMemStats);

IMG_INTERNAL PVRSRV_ERROR BridgeGetDefaultPhysicalHeap(IMG_HANDLE hBridge,
						       PVRSRV_PHYS_HEAP * peHeap);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemGetFaultAddress(IMG_HANDLE hBridge,
						      IMG_HANDLE hDevmemCtx,
						      IMG_DEV_VIRTADDR * psFaultAddress);

IMG_INTERNAL PVRSRV_ERROR BridgePVRSRVStatsUpdateOOMStat(IMG_HANDLE hBridge,
							 IMG_UINT32 ui32ui32StatType,
							 IMG_PID ui32pid);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemXIntReserveRange(IMG_HANDLE hBridge,
						       IMG_HANDLE hDevmemServerHeap,
						       IMG_DEV_VIRTADDR sAddress,
						       IMG_DEVMEM_SIZE_T uiLength,
						       IMG_HANDLE * phReservation);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemXIntUnreserveRange(IMG_HANDLE hBridge,
							 IMG_HANDLE hReservation);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemXIntMapPages(IMG_HANDLE hBridge,
						   IMG_HANDLE hReservation,
						   IMG_HANDLE hPMR,
						   IMG_UINT32 ui32PageCount,
						   IMG_UINT32 ui32PhysPageOffset,
						   PVRSRV_MEMALLOCFLAGS_T uiFlags,
						   IMG_UINT32 ui32VirtPageOffset);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemXIntUnmapPages(IMG_HANDLE hBridge,
						     IMG_HANDLE hReservation,
						     IMG_UINT32 ui32VirtPageOffset,
						     IMG_UINT32 ui32PageCount);

IMG_INTERNAL PVRSRV_ERROR BridgeDevmemXIntMapVRangeToBackingPage(IMG_HANDLE hBridge,
								 IMG_HANDLE hReservation,
								 IMG_UINT32 ui32PageCount,
								 PVRSRV_MEMALLOCFLAGS_T uiFlags,
								 IMG_UINT32 ui32VirtPageOffset);

#endif /* CLIENT_MM_BRIDGE_H */
