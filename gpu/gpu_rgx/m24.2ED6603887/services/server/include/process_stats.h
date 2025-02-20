/*************************************************************************/ /*!
@File
@Title          Functions for creating and reading proc filesystem entries.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#ifndef PROCESS_STATS_H
#define PROCESS_STATS_H

#include <powervr/mem_types.h>

#include "pvrsrv_error.h"
#include "allocmem.h"
#include "cache_ops.h"
#include "device.h"
#include "connection_server.h"

/*
 * The publishing of Process Stats is controlled by the
 * PVRSRV_ENABLE_PROCESS_STATS build option. The recording of all Memory
 * allocations is controlled by the PVRSRV_ENABLE_MEMORY_STATS build option.
 *
 * Note: There will be a performance degradation with memory allocation
 *       recording enabled!
 */


/*
 * Memory types which can be tracked...
 */
typedef enum {
	PVRSRV_MEM_ALLOC_TYPE_KMALLOC,				/* memory allocated by kmalloc() */
	PVRSRV_MEM_ALLOC_TYPE_VMALLOC,				/* memory allocated by vmalloc() */
	PVRSRV_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_UMA,	/* pages allocated from UMA to hold page table information */
	PVRSRV_MEM_ALLOC_TYPE_VMAP_PT_UMA,			/* ALLOC_PAGES_PT_UMA mapped to kernel address space */
	PVRSRV_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_LMA,	/* pages allocated from LMA to hold page table information */
	PVRSRV_MEM_ALLOC_TYPE_IOREMAP_PT_LMA,		/* ALLOC_PAGES_PT_LMA mapped to kernel address space */
	PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES,		/* pages allocated from LMA */
	PVRSRV_MEM_ALLOC_TYPE_ALLOC_UMA_PAGES,		/* pages allocated from UMA */
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	PVRSRV_MEM_ALLOC_TYPE_ZOMBIE_LMA_PAGES,		/* zombie pages allocated from LMA */
	PVRSRV_MEM_ALLOC_TYPE_ZOMBIE_UMA_PAGES,		/* zombie pages allocated from UMA */
#endif
	PVRSRV_MEM_ALLOC_TYPE_MAP_UMA_LMA_PAGES,	/* mapped UMA/LMA pages */
	PVRSRV_MEM_ALLOC_TYPE_UMA_POOL_PAGES,		/* pages in the page pool */
	PVRSRV_MEM_ALLOC_TYPE_DMA_BUF_IMPORT,       /* dma-buf imports */
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	PVRSRV_MEM_ALLOC_TYPE_DMA_BUF_ZOMBIE,       /* dma-buf zombie */
#endif

	/* Must be the last enum...*/
	PVRSRV_MEM_ALLOC_TYPE_COUNT
} PVRSRV_MEM_ALLOC_TYPE;

/*
 * Functions for managing the processes recorded...
 */
PVRSRV_ERROR PVRSRVStatsInitialise(void);
void PVRSRVStatsDestroy(void);
void PVRSRVStatsDestroyDI(void);

PVRSRV_ERROR PVRSRVStatsRegisterProcess(IMG_HANDLE* phProcessStats);

void PVRSRVStatsDeregisterProcess(IMG_HANDLE hProcessStats);

PVRSRV_ERROR PVRSRVStatsDeviceConnect(PVRSRV_DEVICE_NODE *psDeviceNode);

void PVRSRVStatsDeviceDisconnect(PVRSRV_DEVICE_NODE *psDeviceNode);

/*
 * Functions for recording the statistics...
 */

void PVRSRVStatsAddMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE eAllocType,
								  void *pvCpuVAddr,
								  IMG_CPU_PHYADDR sCpuPAddr,
								  size_t uiBytes,
								  IMG_PID uiPid
								  DEBUG_MEMSTATS_PARAMS);

void PVRSRVStatsRemoveMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE eAllocType,
									 IMG_UINT64 ui64Key,
									 IMG_PID uiPid);

#if defined(SUPPORT_PMR_DEFERRED_FREE)
void PVRSRVStatsTransferMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE eCurrentType,
                                       PVRSRV_MEM_ALLOC_TYPE eTargetType,
                                       IMG_UINT64 ui64Key,
                                       IMG_PID currentPid
                                       DEBUG_MEMSTATS_PARAMS);
#endif

void PVRSRVStatsIncrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE eAllocType,
								 size_t uiBytes,
								 IMG_PID uiPid);

/*
 * Increases the memory stat for eAllocType. Tracks the allocation size value
 * by inserting a value into a hash table with uiCpuVAddr as key.
 * Pair with PVRSRVStatsDecrMemAllocStatAndUntrack().
 */
void PVRSRVStatsIncrMemAllocStatAndTrack(PVRSRV_MEM_ALLOC_TYPE eAllocType,
										 size_t uiBytes,
										 IMG_UINT64 uiCpuVAddr,
										 IMG_PID uiPid);

void PVRSRVStatsDecrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE eAllocType,
								 size_t uiBytes,
								 IMG_PID uiPid);

void PVRSRVStatsDecrMemKAllocStat(size_t uiBytes,
								  IMG_PID decrPID);

/*
 * Decrease the memory stat for eAllocType. Takes the allocation size value
 * from the hash table with uiCpuVAddr as key.
 * Pair with PVRSRVStatsIncrMemAllocStatAndTrack().
 */
void PVRSRVStatsDecrMemAllocStatAndUntrack(PVRSRV_MEM_ALLOC_TYPE eAllocType,
									IMG_UINT64 uiCpuVAddr);

void
PVRSRVStatsIncrMemAllocPoolStat(size_t uiBytes);

void
PVRSRVStatsDecrMemAllocPoolStat(size_t uiBytes);

PVRSRV_ERROR
PVRSRVStatsUpdateOOMStat(CONNECTION_DATA *psConnection,
						  PVRSRV_DEVICE_NODE *psDeviceNode,
						  IMG_UINT32 ui32OOMStatType,
						  IMG_PID pidOwner);

void PVRSRVStatsUpdateRenderContextStats(PVRSRV_DEVICE_NODE *psDeviceNode,
										 IMG_UINT32 ui32TotalNumPartialRenders,
										 IMG_UINT32 ui32TotalNumOutOfMemory,
										 IMG_UINT32 ui32TotalTAStores,
										 IMG_UINT32 ui32Total3DStores,
										 IMG_UINT32 ui32TotalCDMStores,
										 IMG_UINT32 ui32TotalTDMStores,
										 IMG_UINT32 ui32NumRayStores,
										 IMG_PID owner);

void PVRSRVStatsUpdateZSBufferStats(PVRSRV_DEVICE_NODE *psDeviceNode,
									IMG_UINT32 ui32NumReqByApp,
									IMG_UINT32 ui32NumReqByFW,
									IMG_PID owner);

void PVRSRVStatsUpdateFreelistStats(PVRSRV_DEVICE_NODE *psDeviceNode,
									IMG_UINT32 ui32NumGrowReqByApp,
									IMG_UINT32 ui32NumGrowReqByFW,
									IMG_UINT32 ui32InitFLPages,
									IMG_UINT32 ui32NumHighPages,
									IMG_PID    ownerPid);
#if defined(PVRSRV_ENABLE_CACHEOP_STATS)
void PVRSRVStatsUpdateCacheOpStats(PVRSRV_CACHE_OP uiCacheOp,
#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO) && defined(DEBUG)
								   IMG_DEV_VIRTADDR sDevVAddr,
								   IMG_DEV_PHYADDR sDevPAddr,
#endif
								   IMG_DEVMEM_SIZE_T uiOffset,
								   IMG_DEVMEM_SIZE_T uiSize,
								   IMG_UINT64 ui64ExecuteTimeMs,
								   IMG_BOOL bUserModeFlush,
								   IMG_PID ownerPid);
#endif

/* Functions used for calculating the memory usage statistics of a process */
PVRSRV_ERROR PVRSRVFindProcessMemStats(IMG_PID pid,
                                       IMG_UINT32 ui32ArrSize,
                                       IMG_BOOL bAllProcessStats,
                                       IMG_UINT64 *pui64MemoryStats);

typedef struct {
	IMG_UINT32 ui32Pid;
	IMG_UINT64 ui64KernelMemUsage;
	IMG_UINT64 ui64GraphicsMemUsage;
} PVRSRV_PER_PROCESS_MEM_USAGE;

PVRSRV_ERROR PVRSRVGetProcessMemUsage(IMG_UINT64 *pui64TotalMem,
                                      IMG_UINT32 *pui32NumberOfLivePids,
                                      PVRSRV_PER_PROCESS_MEM_USAGE **ppsPerProcessMemUsageData);

#if defined(MTK_FULL_PORTING)
extern unsigned int (*mtk_get_gpu_memory_usage_fp)(void);
extern bool (*mtk_dump_gpu_memory_usage_fp)(void);
IMG_UINT32 MTKGetMemStat(void);
bool MTKGetMemStatDump(void);
#endif /* MTK_FULL_PORTING */

#endif /* PROCESS_STATS_H */
