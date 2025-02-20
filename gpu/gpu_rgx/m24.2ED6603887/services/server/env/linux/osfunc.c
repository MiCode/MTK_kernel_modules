/*************************************************************************/ /*!
@File
@Title          Environment related functions
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

#include <linux/version.h>
#include <asm/page.h>
#include <asm/div64.h>
#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/hugetlb.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/genalloc.h>
#include <linux/string.h>
#include <linux/freezer.h>
#include <asm/hardirq.h>
#include <asm/tlbflush.h>
#include <linux/timer.h>
#include <linux/capability.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/dmaengine.h>
#include <linux/dma-direction.h>
#include <linux/kthread.h>
#include <linux/utsname.h>
#include <linux/scatterlist.h>
#include <linux/interrupt.h>
#include <linux/pfn_t.h>
#include <linux/pfn.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <linux/sched/clock.h>
#include <linux/sched/signal.h>
#else
#include <linux/sched.h>
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)) */
#if defined(SUPPORT_SECURE_ALLOC_KM)
#if defined(PVR_ANDROID_HAS_DMA_HEAP_FIND)
#include <linux/dma-heap.h>
#include "physmem_dmabuf.h"
#else
#include "physmem.h"
#endif
#endif

#include "log2.h"
#include "sysinfo.h"
#include "osfunc.h"
#include "cache_km.h"
#include "img_defs.h"
#include "img_types.h"
#include "allocmem.h"
#include "devicemem_server_utils.h"
#include "event.h"
#include "linkage.h"
#include "pvr_uaccess.h"
#include "pvr_debug.h"
#include "pvr_bridge_k.h"
#include "pvrsrv_memallocflags.h"
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#include "process_stats.h"
#endif
#include "physmem_osmem_linux.h"
#include "dma_support.h"

#include "pvrsrv_sync_server.h"

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
#include "ri_server.h"
#include "pvr_ricommon.h"
#endif

#include "kernel_compatibility.h"

#if !defined(EVENT_OBJECT_TIMEOUT_US)
#error EVENT_OBJECT_TIMEOUT_US should be defined sysinfo.h
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0))
#define PVR_FOLL_LONGTERM (0x0U)
#else
#define PVR_FOLL_LONGTERM FOLL_LONGTERM
#endif

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 6, 0))

#define pvr_pin_user_pages_for_dma(puiAddress, num_pages, bWrite, pages)  get_user_pages_fast( \
    (unsigned long)puiAddress, \
    (int)num_pages, \
    (int) (bWrite ? FOLL_WRITE : 0) | PVR_FOLL_LONGTERM, \
    pages)
#define pvr_unpin_user_page_for_dma(p) put_page(p)

#else

#define pvr_pin_user_pages_for_dma(puiAddress, num_pages, bWrite, pages) pin_user_pages_fast( \
    (unsigned long)puiAddress, \
    (int) num_pages, \
    (int) (bWrite ? FOLL_WRITE : 0) | PVR_FOLL_LONGTERM, \
    pages)
#define pvr_unpin_user_page_for_dma(p) unpin_user_page(p)

#endif

#define _FREEZABLE IMG_TRUE
#define _NON_FREEZABLE IMG_FALSE

typedef struct {
	struct task_struct *kthread;
	PFN_THREAD pfnThread;
	void *hData;
	IMG_CHAR *pszThreadName;
	IMG_BOOL   bIsThreadRunning;
	IMG_BOOL   bIsSupportingThread;
	PFN_THREAD_DEBUG_DUMP pfnDebugDumpCB;
	DLLIST_NODE sNode;
} OSThreadData;

void OSSuspendTaskInterruptible(void)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
}

static DLLIST_NODE gsThreadListHead;

static void _ThreadListAddEntry(OSThreadData *psThreadListNode)
{
	dllist_add_to_tail(&gsThreadListHead, &(psThreadListNode->sNode));
}

static void _ThreadListRemoveEntry(OSThreadData *psThreadListNode)
{
	dllist_remove_node(&(psThreadListNode->sNode));
}

static void _ThreadSetStopped(OSThreadData *psOSThreadData)
{
	psOSThreadData->bIsThreadRunning = IMG_FALSE;
}

static void _OSInitThreadList(void)
{
	dllist_init(&gsThreadListHead);
}

void OSThreadDumpInfo(DUMPDEBUG_PRINTF_FUNC* pfnDumpDebugPrintf,
                      void *pvDumpDebugFile)
{
	PDLLIST_NODE psNodeCurr, psNodeNext;
	LINUX_THREAD_ACTIVITY_STATS sThreadStats;
	PVRSRV_ERROR eError = PVRSRV_OK;

	dllist_foreach_node(&gsThreadListHead, psNodeCurr, psNodeNext)
	{
		OSThreadData *psThreadListNode;
		psThreadListNode = IMG_CONTAINER_OF(psNodeCurr, OSThreadData, sNode);

		PVR_DUMPDEBUG_LOG("  %s : %s",
				  psThreadListNode->pszThreadName,
				  (psThreadListNode->bIsThreadRunning) ? "Running" : "Stopped");

		if (psThreadListNode->pfnDebugDumpCB)
		{
			psThreadListNode->pfnDebugDumpCB(pfnDumpDebugPrintf, pvDumpDebugFile);
		}
	}

	eError = LinuxGetThreadActivityStats(&sThreadStats);
	if (eError == PVRSRV_OK)
	{
		PVR_DUMPDEBUG_LOG("Active Threads (UM/KM): %d / %d",
		                  sThreadStats.i32DriverThreadCount,
		                  sThreadStats.i32KernelThreadCount);

		PVR_DUMPDEBUG_LOG("Suspended Threads: %d",
		                  sThreadStats.i32SuspendedThreadCount);
	}
	else
	{
		PVR_LOG_ERROR(eError, "LinuxGetThreadActivityStats");
	}
}

PVRSRV_ERROR OSPhyContigPagesAlloc(PHYS_HEAP *psPhysHeap, size_t uiSize,
							PG_HANDLE *psMemHandle, IMG_DEV_PHYADDR *psDevPAddr,
							IMG_PID uiPid)
{
	PVRSRV_DEVICE_NODE *psDevNode = PhysHeapDeviceNode(psPhysHeap);
	struct device *psDev = psDevNode->psDevConfig->pvOSDevice;
	IMG_CPU_PHYADDR sCpuPAddr;
	struct page *psPage;
	IMG_UINT32	ui32Order=0;
	gfp_t gfp_flags;

	PVR_ASSERT(uiSize != 0);
	/*Align the size to the page granularity */
	uiSize = PAGE_ALIGN(uiSize);

	/*Get the order to be used with the allocation */
	ui32Order = get_order(uiSize);

	gfp_flags = GFP_KERNEL;

#if !defined(PVR_LINUX_PHYSMEM_USE_HIGHMEM_ONLY)
	if (psDev)
	{
		if (*psDev->dma_mask == DMA_BIT_MASK(32))
		{
			/* Limit to 32 bit.
			 * Achieved by setting __GFP_DMA32 for 64 bit systems */
			gfp_flags |= __GFP_DMA32;
		}
		else if (*psDev->dma_mask < DMA_BIT_MASK(32))
		{
			/* Limit to whatever the size of DMA zone is. */
			gfp_flags |= __GFP_DMA;
		}
	}
#else
	PVR_UNREFERENCED_PARAMETER(psDev);
#endif

	/*allocate the pages */
	psPage = alloc_pages(gfp_flags, ui32Order);
	if (psPage == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	uiSize = (1 << ui32Order) * PAGE_SIZE;

	psMemHandle->u.pvHandle = psPage;
	psMemHandle->uiOrder = ui32Order;
	sCpuPAddr.uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(page_to_phys(psPage));

	/*
	 * Even when more pages are allocated as base MMU object we still need one single physical address because
	 * they are physically contiguous.
	 */
	PhysHeapCpuPAddrToDevPAddr(psPhysHeap, 1, psDevPAddr, &sCpuPAddr);

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsIncrMemAllocStatAndTrack(PVRSRV_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_UMA,
	                                    uiSize,
	                                    (IMG_UINT64)(uintptr_t) psPage,
	                                    uiPid);
#else
	PVRSRVStatsAddMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_UMA,
	                             psPage,
	                             sCpuPAddr,
	                             uiSize,
	                             uiPid
	                             DEBUG_MEMSTATS_VALUES);
#endif
#else
	PVR_UNREFERENCED_PARAMETER(uiPid);
#endif

	return PVRSRV_OK;
}

void OSPhyContigPagesFree(PHYS_HEAP *psPhysHeap, PG_HANDLE *psMemHandle)
{
	struct page *psPage = (struct page*) psMemHandle->u.pvHandle;
	IMG_UINT32 ui32Order;

	PVR_UNREFERENCED_PARAMETER(psPhysHeap);

	ui32Order = psMemHandle->uiOrder;

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsDecrMemAllocStatAndUntrack(PVRSRV_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_UMA,
	                                      (IMG_UINT64)(uintptr_t) psPage);
#else
	PVRSRVStatsRemoveMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_UMA,
	                                (IMG_UINT64)(uintptr_t) psPage,
	                                OSGetCurrentClientProcessIDKM());
#endif
#endif

	__free_pages(psPage, ui32Order);
	psMemHandle->uiOrder = 0;
}

PVRSRV_ERROR OSPhyContigPagesMap(PHYS_HEAP *psPhysHeap, PG_HANDLE *psMemHandle,
						size_t uiSize, IMG_DEV_PHYADDR *psDevPAddr,
						void **pvPtr)
{
	size_t actualSize = 1 << (PAGE_SHIFT + psMemHandle->uiOrder);
	*pvPtr = kmap((struct page*)psMemHandle->u.pvHandle);

	PVR_UNREFERENCED_PARAMETER(psDevPAddr);

	PVR_UNREFERENCED_PARAMETER(actualSize); /* If we don't take an #ifdef path */
	PVR_UNREFERENCED_PARAMETER(uiSize);
	PVR_UNREFERENCED_PARAMETER(psPhysHeap);

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsIncrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_VMAP_PT_UMA, actualSize, OSGetCurrentClientProcessIDKM());
#else
	{
		IMG_CPU_PHYADDR sCpuPAddr;
		sCpuPAddr.uiAddr = 0;

		PVRSRVStatsAddMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_VMAP_PT_UMA,
									 *pvPtr,
									 sCpuPAddr,
									 actualSize,
									 OSGetCurrentClientProcessIDKM()
									 DEBUG_MEMSTATS_VALUES);
	}
#endif
#endif

	return PVRSRV_OK;
}

void OSPhyContigPagesUnmap(PHYS_HEAP *psPhysHeap, PG_HANDLE *psMemHandle, void *pvPtr)
{
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	/* Mapping is done a page at a time */
	PVRSRVStatsDecrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_VMAP_PT_UMA,
	                            (1 << (PAGE_SHIFT + psMemHandle->uiOrder)),
	                            OSGetCurrentClientProcessIDKM());
#else
	PVRSRVStatsRemoveMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_VMAP_PT_UMA,
	                                (IMG_UINT64)(uintptr_t)pvPtr,
	                                OSGetCurrentClientProcessIDKM());
#endif
#endif

	PVR_UNREFERENCED_PARAMETER(psPhysHeap);
	PVR_UNREFERENCED_PARAMETER(pvPtr);

	kunmap((struct page*) psMemHandle->u.pvHandle);
}

PVRSRV_ERROR OSPhyContigPagesClean(PHYS_HEAP *psPhysHeap,
                                   PG_HANDLE *psMemHandle,
                                   IMG_UINT32 uiOffset,
                                   IMG_UINT32 uiLength)
{
	PVRSRV_DEVICE_NODE *psDevNode = PhysHeapDeviceNode(psPhysHeap);
	PVRSRV_ERROR eError = PVRSRV_OK;
	struct page* psPage = (struct page*) psMemHandle->u.pvHandle;

	void* pvVirtAddrStart = kmap(psPage) + uiOffset;
	IMG_CPU_PHYADDR sPhysStart, sPhysEnd;

	IMG_UINT32 ui32Order;

	if (uiLength == 0)
	{
		goto e0;
	}

	ui32Order = psMemHandle->uiOrder;
	if ((uiOffset + uiLength) > ((1 << ui32Order) * PAGE_SIZE))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Invalid size params, uiOffset %u, uiLength %u",
				__func__,
				uiOffset,
				uiLength));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto e0;
	}

	sPhysStart.uiAddr = page_to_phys(psPage) + uiOffset;
	sPhysEnd.uiAddr = sPhysStart.uiAddr + uiLength;

	CacheOpExec(psDevNode,
				pvVirtAddrStart,
				pvVirtAddrStart + uiLength,
				sPhysStart,
				sPhysEnd,
				PVRSRV_CACHE_OP_CLEAN);

e0:
	kunmap(psPage);

	return eError;
}

#if defined(__GNUC__)
#define PVRSRV_MEM_ALIGN __attribute__ ((aligned (0x8)))
#define PVRSRV_MEM_ALIGN_MASK (0x7)
#else
#error "PVRSRV Alignment macros need to be defined for this compiler"
#endif

IMG_UINT32 OSCPUCacheAttributeSize(OS_CPU_CACHE_ATTRIBUTE eCacheAttribute)
{
	IMG_UINT32 uiSize = 0;

	switch (eCacheAttribute)
	{
		case OS_CPU_CACHE_ATTRIBUTE_LINE_SIZE:
			uiSize = cache_line_size();
			break;

		default:
			PVR_DPF((PVR_DBG_ERROR, "%s: Invalid cache attribute type %d",
					__func__, (IMG_UINT32)eCacheAttribute));
			PVR_ASSERT(0);
			break;
	}

	return uiSize;
}

IMG_INT OSMemCmp(void *pvBufA, void *pvBufB, size_t uiLen)
{
	return (IMG_INT)memcmp(pvBufA, pvBufB, uiLen);
}

size_t OSStringLCat(IMG_CHAR *pszDest, const IMG_CHAR *pszSrc, size_t uDstSize)
{
	/*
	 * Let strlcat handle any truncation cases correctly.
	 * We will definitely get a NUL-terminated string set in pszDest
	 */
	size_t uSrcSize = strlcat(pszDest, pszSrc, uDstSize);

#if defined(PVR_DEBUG_STRLCPY) && defined(DEBUG)
	/* Handle truncation by dumping calling stack if debug allows */
	if (uSrcSize >= uDstSize)
	{
		PVR_DPF((PVR_DBG_WARNING,
			"%s: String truncated Src = '<%s>' %ld bytes, Dest = '%s'",
			__func__, pszSrc, (long)uDstSize, pszDest));
		OSDumpStack();
	}
#endif /* defined(PVR_DEBUG_STRLCPY) && defined(DEBUG) */

	return uSrcSize;
}

IMG_INT32 OSSNPrintf(IMG_CHAR *pStr, size_t ui32Size, const IMG_CHAR *pszFormat, ...)
{
	va_list argList;
	IMG_INT32 iCount;

	va_start(argList, pszFormat);
	iCount = vsnprintf(pStr, (size_t)ui32Size, pszFormat, argList);
	va_end(argList);

	return iCount;
}

IMG_INT32 OSVSNPrintf(IMG_CHAR *pStr, size_t ui32Size, const IMG_CHAR* pszFormat, va_list vaArgs)
{
	return vsnprintf(pStr, ui32Size, pszFormat, vaArgs);
}

size_t OSStringLength(const IMG_CHAR *pStr)
{
	return strlen(pStr);
}

size_t OSStringNLength(const IMG_CHAR *pStr, size_t uiCount)
{
	return strnlen(pStr, uiCount);
}

IMG_INT32 OSStringNCompare(const IMG_CHAR *pStr1, const IMG_CHAR *pStr2,
                          size_t uiSize)
{
#if defined(DEBUG)
	/* Double-check that we are not passing NULL parameters in. If we are we
	 * return -1 (for arg1 == NULL, arg2 != NULL)
	 * 0 (for arg1 == NULL, arg2 == NULL
	 * +1 (for arg1 != NULL, arg2 == NULL)
	 * strncmp(arg1, arg2, size) otherwise
	 */
	if (pStr1 == NULL)
	{
		if (pStr2 == NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s(%p, %p, %d): Both args NULL",
				 __func__, pStr1, pStr2, (int)uiSize));
			OSDumpStack();
			return 0;	/* Both NULL */
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "%s(%p, %p, %d): arg1 NULL",
				 __func__, pStr1, pStr2, (int)uiSize));
			OSDumpStack();
			return -1;	/* NULL < non-NULL */
		}
	}
	else
	{
		if (pStr2 == NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s(%p, %p, %d): arg2 NULL",
				 __func__, pStr1, pStr2, (int)uiSize));
			OSDumpStack();
			return +1;	/* non-NULL > NULL */
		}
		else
		{
			return strncmp(pStr1, pStr2, uiSize);
		}
	}
#else
	return strncmp(pStr1, pStr2, uiSize);
#endif
}

PVRSRV_ERROR OSStringToUINT32(const IMG_CHAR *pStr, IMG_UINT32 ui32Base,
                              IMG_UINT32 *ui32Result)
{
	if (kstrtou32(pStr, ui32Base, ui32Result) != 0)
		return PVRSRV_ERROR_CONVERSION_FAILED;

	return PVRSRV_OK;
}

IMG_UINT32 OSStringUINT32ToStr(IMG_CHAR *pszBuf, size_t uSize,
							IMG_UINT32 ui32Num)
{
	IMG_UINT32 ui32i, ui32Len = 0, ui32NumCopy = ui32Num;

	/* calculate string length required to hold the number string */
	do
	{
		ui32Len++;
		ui32NumCopy /= 10;
	} while (ui32NumCopy != 0);

	if (unlikely(ui32Len >= uSize))
	{
		/* insufficient buffer */
		return 0;
	}

	for (ui32i = 0; ui32i < ui32Len; ui32i++)
	{
		pszBuf[ui32Len - (ui32i + 1)] = '0' + ui32Num % 10;
		ui32Num = ui32Num / 10;
	}

	pszBuf[ui32Len] = '\0';
	return ui32Len;
}

#if defined(SUPPORT_NATIVE_FENCE_SYNC) || defined(SUPPORT_BUFFER_SYNC)
static struct workqueue_struct *gpFenceStatusWq;
static struct workqueue_struct *gpFenceCtxDestroyWq;

static PVRSRV_ERROR _NativeSyncInit(void)
{
	gpFenceStatusWq = create_freezable_workqueue("pvr_fence_status");
	if (!gpFenceStatusWq)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create foreign fence status workqueue",
				 __func__));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	gpFenceCtxDestroyWq = create_freezable_workqueue("pvr_fence_context_destroy");
	if (!gpFenceCtxDestroyWq)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create foreign fence context destruction workqueue",
				 __func__));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	return PVRSRV_OK;
}

static void _NativeSyncDeinit(void)
{
	destroy_workqueue(gpFenceStatusWq);
	destroy_workqueue(gpFenceCtxDestroyWq);
}

struct workqueue_struct *NativeSyncGetFenceStatusWq(void)
{
	if (!gpFenceStatusWq)
	{
#if defined(DEBUG)
		PVR_ASSERT(gpFenceStatusWq);
#endif
		return NULL;
	}

	return gpFenceStatusWq;
}

struct workqueue_struct *NativeSyncGetFenceCtxDestroyWq(void)
{
	if (!gpFenceCtxDestroyWq)
	{
#if defined(DEBUG)
		PVR_ASSERT(gpFenceCtxDestroyWq);
#endif
		return NULL;
	}

	return gpFenceCtxDestroyWq;
}
#endif

PVRSRV_ERROR OSInitEnvData(void)
{
	PVRSRV_ERROR eError;

	eError = LinuxInitPhysmem();
	PVR_GOTO_IF_ERROR(eError, error_out);

	_OSInitThreadList();

#if defined(SUPPORT_NATIVE_FENCE_SYNC) || defined(SUPPORT_BUFFER_SYNC)
	eError = _NativeSyncInit();
#endif

error_out:
	return eError;
}

void OSDeInitEnvData(void)
{
#if defined(SUPPORT_NATIVE_FENCE_SYNC) || defined(SUPPORT_BUFFER_SYNC)
	_NativeSyncDeinit();
#endif

	LinuxDeinitPhysmem();
}

void OSReleaseThreadQuanta(void)
{
	schedule();
}

void OSMemoryBarrier(volatile void *hReadback)
{
	mb();

	if (hReadback)
	{
		/* Force a read-back to memory to avoid posted writes on certain buses
		 * e.g. PCI(E)
		 */
		(void) OSReadDeviceMem32(hReadback);
	}
}

void OSWriteMemoryBarrier(volatile void *hReadback)
{
	wmb();

	if (hReadback)
	{
		/* Force a read-back to memory to avoid posted writes on certain buses
		 * e.g. PCI(E)
		 */
		(void) OSReadDeviceMem32(hReadback);
	}
}

/* Not matching/aligning this API to the Clockus() API above to avoid necessary
 * multiplication/division operations in calling code.
 */
static inline IMG_UINT64 Clockns64(void)
{
	IMG_UINT64 timenow;

	/* Kernel thread preempt protection. Some architecture implementations
	 * (ARM) of sched_clock are not preempt safe when the kernel is configured
	 * as such e.g. CONFIG_PREEMPT and others.
	 */
	preempt_disable();

	/* Using sched_clock instead of ktime_get since we need a time stamp that
	 * correlates with that shown in kernel logs and trace data not one that
	 * is a bit behind. */
	timenow = sched_clock();

	preempt_enable();

	return timenow;
}

IMG_UINT64 OSClockns64(void)
{
	return Clockns64();
}

IMG_UINT64 OSClockus64(void)
{
	IMG_UINT64 timenow = Clockns64();
	IMG_UINT32 remainder;

	return OSDivide64r64(timenow, 1000, &remainder);
}

IMG_UINT32 OSClockus(void)
{
	return (IMG_UINT32) OSClockus64();
}

IMG_UINT32 OSClockms(void)
{
	IMG_UINT64 timenow = Clockns64();
	IMG_UINT32 remainder;

	return OSDivide64(timenow, 1000000, &remainder);
}

static inline IMG_UINT64 KClockns64(void)
{
	ktime_t sTime = ktime_get();

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	return sTime;
#else
	return sTime.tv64;
#endif
}

PVRSRV_ERROR OSClockMonotonicns64(IMG_UINT64 *pui64Time)
{
	*pui64Time = KClockns64();
	return PVRSRV_OK;
}

PVRSRV_ERROR OSClockMonotonicus64(IMG_UINT64 *pui64Time)
{
	IMG_UINT64 timenow = KClockns64();
	IMG_UINT32 remainder;

	*pui64Time = OSDivide64r64(timenow, 1000, &remainder);
	return PVRSRV_OK;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
IMG_UINT64 OSClockMonotonicRawns64(void)
{
	struct timespec64 ts;

	ktime_get_raw_ts64(&ts);
	return ts.tv_sec * 1000000000 + ts.tv_nsec;
}
#else
IMG_UINT64 OSClockMonotonicRawns64(void)
{
	struct timespec ts;

	getrawmonotonic(&ts);
	return (IMG_UINT64) ts.tv_sec * 1000000000 + ts.tv_nsec;
}
#endif

IMG_UINT64 OSClockMonotonicRawus64(void)
{
	IMG_UINT32 rem;
	return OSDivide64r64(OSClockMonotonicRawns64(), 1000, &rem);
}

/*
	OSWaitus
*/
void OSWaitus(IMG_UINT32 ui32Timeus)
{
	udelay(ui32Timeus);
}


/*
	OSSleepms
*/
void OSSleepms(IMG_UINT32 ui32Timems)
{
	msleep(ui32Timems);
}


INLINE IMG_UINT64 OSGetCurrentProcessVASpaceSize(void)
{
	return (IMG_UINT64)TASK_SIZE;
}

INLINE IMG_PID OSGetCurrentProcessID(void)
{
	if (in_interrupt())
	{
		return KERNEL_ID;
	}

	return (IMG_PID)task_tgid_nr(current);
}

INLINE IMG_PID OSGetCurrentVirtualProcessID(void)
{
	if (in_interrupt())
	{
		return KERNEL_ID;
	}

	return (IMG_PID)task_tgid_vnr(current);
}

INLINE IMG_CHAR *OSGetCurrentProcessName(void)
{
	return current->comm;
}

INLINE uintptr_t OSGetCurrentThreadID(void)
{
	if (in_interrupt())
	{
		return KERNEL_ID;
	}

	return current->pid;
}

IMG_PID OSGetCurrentClientProcessIDKM(void)
{
	return OSGetCurrentProcessID();
}

IMG_CHAR *OSGetCurrentClientProcessNameKM(void)
{
	return OSGetCurrentProcessName();
}

uintptr_t OSGetCurrentClientThreadIDKM(void)
{
	return OSGetCurrentThreadID();
}

size_t OSGetPageSize(void)
{
	return PAGE_SIZE;
}

size_t OSGetPageShift(void)
{
	return PAGE_SHIFT;
}

size_t OSGetPageMask(void)
{
	return (OSGetPageSize()-1);
}

size_t OSGetOrder(size_t uSize)
{
	return get_order(PAGE_ALIGN(uSize));
}

IMG_UINT64 OSGetRAMSize(void)
{
	struct sysinfo SI;
	si_meminfo(&SI);

	return (PAGE_SIZE * SI.totalram);
}

typedef struct
{
	int os_error;
	PVRSRV_ERROR pvr_error;
} error_map_t;

#define PVRSRV_ERROR_TO_OS_ERROR \
	X(PVRSRV_OK, 0) \
	X(PVRSRV_ERROR_BRIDGE_EFAULT, EFAULT) \
	X(PVRSRV_ERROR_BRIDGE_EINVAL, EINVAL) \
	X(PVRSRV_ERROR_BRIDGE_ENOMEM, ENOMEM) \
	X(PVRSRV_ERROR_BRIDGE_ERANGE, ERANGE) \
	X(PVRSRV_ERROR_BRIDGE_EPERM, EPERM) \
	X(PVRSRV_ERROR_BRIDGE_ENOTTY, ENOTTY) \
	X(PVRSRV_ERROR_BRIDGE_CALL_FAILED, ENOTTY) \
	X(PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL, ERANGE) \
	X(PVRSRV_ERROR_OUT_OF_MEMORY, ENOMEM) \
	X(PVRSRV_ERROR_PMR_NOT_PERMITTED, EACCES) \
	X(PVRSRV_ERROR_INVALID_PARAMS, EINVAL) \
	X(PVRSRV_ERROR_PMR_NO_KERNEL_MAPPING, EPERM) \
	X(PVRSRV_ERROR_NOT_IMPLEMENTED, ENOSYS) \
	X(PVRSRV_ERROR_BAD_MAPPING, EINVAL)

/* return -ve versions of POSIX errors as they are used in this form */
int PVRSRVToNativeError(PVRSRV_ERROR eError)
{
	switch (eError)
	{
#define X(_PVRSRV_ERROR, _OS_ERROR) \
	case (_PVRSRV_ERROR): return -(_OS_ERROR);

		PVRSRV_ERROR_TO_OS_ERROR

#undef X
		default:
			return -EFAULT;
	}
}

typedef struct  _MISR_DATA_ {
	struct workqueue_struct *psWorkQueue;
	struct work_struct sMISRWork;
	const IMG_CHAR* pszName;
	PFN_MISR pfnMISR;
	void *hData;
} MISR_DATA;

/*
	MISRWrapper
*/
static void MISRWrapper(struct work_struct *data)
{
	MISR_DATA *psMISRData = container_of(data, MISR_DATA, sMISRWork);

	PVR_DPF((PVR_DBG_MESSAGE, "Waking up '%s' MISR %p", psMISRData->pszName, psMISRData));

	psMISRData->pfnMISR(psMISRData->hData);
}

/*
	OSInstallMISR
*/
PVRSRV_ERROR OSInstallMISR(IMG_HANDLE *hMISRData, PFN_MISR pfnMISR,
							void *hData, const IMG_CHAR *pszMisrName)
{
	MISR_DATA *psMISRData;

	psMISRData = OSAllocMem(sizeof(*psMISRData));
	PVR_LOG_RETURN_IF_NOMEM(psMISRData, "psMISRData");

	psMISRData->hData = hData;
	psMISRData->pfnMISR = pfnMISR;
	psMISRData->pszName = pszMisrName;

	PVR_DPF((PVR_DBG_MESSAGE, "Installing MISR with cookie %p", psMISRData));

	psMISRData->psWorkQueue = create_singlethread_workqueue("pvr_misr");

	if (psMISRData->psWorkQueue == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSInstallMISR: create_singlethreaded_workqueue failed"));
		OSFreeMem(psMISRData);
		return PVRSRV_ERROR_UNABLE_TO_CREATE_THREAD;
	}

	INIT_WORK(&psMISRData->sMISRWork, MISRWrapper);

	*hMISRData = (IMG_HANDLE) psMISRData;

	return PVRSRV_OK;
}

/*
	OSUninstallMISR
*/
PVRSRV_ERROR OSUninstallMISR(IMG_HANDLE hMISRData)
{
	MISR_DATA *psMISRData = (MISR_DATA *) hMISRData;

	PVR_DPF((PVR_DBG_MESSAGE, "Uninstalling MISR with cookie %p", psMISRData));

	destroy_workqueue(psMISRData->psWorkQueue);
	OSFreeMem(psMISRData);

	return PVRSRV_OK;
}

/*
	OSScheduleMISR
*/
PVRSRV_ERROR OSScheduleMISR(IMG_HANDLE hMISRData)
{
	MISR_DATA *psMISRData = (MISR_DATA *) hMISRData;

	/*
		Note:

		In the case of NO_HARDWARE we want the driver to be synchronous so
		that we don't have to worry about waiting for previous operations
		to complete
	*/
#if defined(NO_HARDWARE)
	psMISRData->pfnMISR(psMISRData->hData);
	return PVRSRV_OK;
#else
	{
		bool rc = queue_work(psMISRData->psWorkQueue, &psMISRData->sMISRWork);
		return rc ? PVRSRV_OK : PVRSRV_ERROR_ALREADY_EXISTS;
	}
#endif
}

void OSSyncIRQ(IMG_UINT32 ui32IRQ)
{
	synchronize_irq(ui32IRQ);
}

/* OS specific values for thread priority */
static const IMG_INT32 ai32OSPriorityValues[OS_THREAD_LAST_PRIORITY] =
{
	  0, /* OS_THREAD_NOSET_PRIORITY */
	-20, /* OS_THREAD_HIGHEST_PRIORITY */
	-10, /* OS_THREAD_HIGH_PRIORITY */
	  0, /* OS_THREAD_NORMAL_PRIORITY */
	  9, /* OS_THREAD_LOW_PRIORITY */
	 19, /* OS_THREAD_LOWEST_PRIORITY */
};

static int OSThreadRun(void *data)
{
	OSThreadData *psOSThreadData = data;

	/* count freezable threads */
	LinuxBridgeNumActiveKernelThreadsIncrement();

	/* Returns true if the thread was frozen, should we do anything with this
	 * information? What do we return? Which one is the error case? */
	set_freezable();

	PVR_DPF((PVR_DBG_MESSAGE, "Starting Thread '%s'...", psOSThreadData->pszThreadName));

	/* Call the client's kernel thread with the client's data pointer */
	psOSThreadData->pfnThread(psOSThreadData->hData);

	if (psOSThreadData->bIsSupportingThread)
	{
		_ThreadSetStopped(psOSThreadData);
	}

	/* Wait for OSThreadDestroy() to call kthread_stop() */
	while (!kthread_freezable_should_stop(NULL))
	{
		schedule();
	}

	LinuxBridgeNumActiveKernelThreadsDecrement();

	return 0;
}

PVRSRV_ERROR OSThreadCreate(IMG_HANDLE *phThread,
                            IMG_CHAR *pszThreadName,
                            PFN_THREAD pfnThread,
                            PFN_THREAD_DEBUG_DUMP pfnDebugDumpCB,
                            IMG_BOOL bIsSupportingThread,
                            void *hData)
{
	return OSThreadCreatePriority(phThread, pszThreadName, pfnThread,
	                              pfnDebugDumpCB, bIsSupportingThread, hData,
	                              OS_THREAD_NOSET_PRIORITY);
}

PVRSRV_ERROR OSThreadCreatePriority(IMG_HANDLE *phThread,
                                    IMG_CHAR *pszThreadName,
                                    PFN_THREAD pfnThread,
                                    PFN_THREAD_DEBUG_DUMP pfnDebugDumpCB,
                                    IMG_BOOL bIsSupportingThread,
                                    void *hData,
                                    OS_THREAD_LEVEL eThreadPriority)
{
	OSThreadData *psOSThreadData;
	PVRSRV_ERROR eError;

	psOSThreadData = OSAllocZMem(sizeof(*psOSThreadData));
	PVR_LOG_GOTO_IF_NOMEM(psOSThreadData, eError, fail_alloc);

	psOSThreadData->pfnThread = pfnThread;
	psOSThreadData->hData = hData;
	psOSThreadData->pszThreadName = pszThreadName;
	psOSThreadData->kthread = kthread_run(OSThreadRun, psOSThreadData, "%s", pszThreadName);

	if (IS_ERR(psOSThreadData->kthread))
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto fail_kthread;
	}

	if (bIsSupportingThread)
	{
		psOSThreadData->pfnDebugDumpCB = pfnDebugDumpCB;
		psOSThreadData->bIsThreadRunning = IMG_TRUE;
		psOSThreadData->bIsSupportingThread = IMG_TRUE;

		_ThreadListAddEntry(psOSThreadData);
	}

	if (eThreadPriority != OS_THREAD_NOSET_PRIORITY &&
	    eThreadPriority < OS_THREAD_LAST_PRIORITY)
	{
		set_user_nice(psOSThreadData->kthread,
		              ai32OSPriorityValues[eThreadPriority]);
	}

	*phThread = psOSThreadData;

	return PVRSRV_OK;

fail_kthread:
	OSFreeMem(psOSThreadData);
fail_alloc:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

PVRSRV_ERROR OSThreadDestroy(IMG_HANDLE hThread)
{
	OSThreadData *psOSThreadData = hThread;
	int ret;

	/* Let the thread know we are ready for it to end and wait for it. */
	ret = kthread_stop(psOSThreadData->kthread);
	if (0 != ret)
	{
		PVR_DPF((PVR_DBG_WARNING, "kthread_stop failed(%d)", ret));
		return PVRSRV_ERROR_RETRY;
	}

	if (psOSThreadData->bIsSupportingThread)
	{
		_ThreadListRemoveEntry(psOSThreadData);
	}

	OSFreeMem(psOSThreadData);

	return PVRSRV_OK;
}

void OSPanic(void)
{
	BUG();

#if defined(__KLOCWORK__)
	/* Klocwork does not understand that BUG is terminal... */
	abort();
#endif
}

IMG_BOOL OSIsMapPhysNonContigSupported(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)) && !defined(CONFIG_VMAP_PFN)
	return IMG_FALSE;
#else
	return IMG_TRUE;
#endif
}

void OSUnMapPhysArrayToLin(void *pvLinAddr, void *pvPrivData)
{
	if (is_vmalloc_addr(pvLinAddr))
	{
#if defined(CONFIG_VMAP_PFN)
		PVR_UNREFERENCED_PARAMETER(pvPrivData);
		vunmap(pvLinAddr);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
		unmap_kernel_range((unsigned long) (uintptr_t) pvLinAddr,
                           get_vm_area_size(pvPrivData));
		free_vm_area(pvPrivData);
#else
		PVR_DPF((PVR_DBG_ERROR,"%s: Cannot map into kernel, no method supported.", __func__));
		PVR_ASSERT(0);
#endif
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Given kernel address is not a vmalloc addr", __func__));
	}
}

#define PagePAToPFN(PageAddr) (PageAddr >> PAGE_SHIFT)

PVRSRV_ERROR OSMapPhysArrayToLin(IMG_CPU_PHYADDR pPagePA[],
                                 IMG_UINT32 uiPagesCount,
                                 void **ppvLinAddr,
                                 void **ppvPrivData)
{
	if (ppvLinAddr == NULL || ppvPrivData == NULL)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if defined(CONFIG_VMAP_PFN)
	{
		IMG_UINT32 i;

		for (i = 0; i < uiPagesCount; i++)
		{
			pPagePA[i].uiAddr = PagePAToPFN(pPagePA[i].uiAddr);
		}

		*ppvLinAddr = vmap_pfn((unsigned long *)pPagePA,
							   (unsigned int)uiPagesCount,
							   pgprot_device(PAGE_KERNEL));
		if (NULL == *ppvLinAddr)
		{
			return PVRSRV_ERROR_PMR_CPU_PAGE_MAP_FAILED;
		}
		*ppvPrivData = NULL;
		return PVRSRV_OK;
	}
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	{
		pte_t *pte[32], **pte_array;
		struct vm_struct *psVMA;
		PVRSRV_ERROR eError = PVRSRV_OK;
		IMG_UINT32 i = 0;

		pte_array = &pte[0];
		if (sizeof(pte) < (sizeof(pte[0]) * uiPagesCount))
		{
			pte_array = kzalloc(uiPagesCount * sizeof(*pte_array), GFP_KERNEL);
			if (NULL == pte_array)
			{
				return PVRSRV_ERROR_OUT_OF_MEMORY;
			}
		}

		psVMA = alloc_vm_area((size_t)(uiPagesCount << PAGE_SHIFT), pte_array);
		if (NULL == psVMA)
		{
			eError = PVRSRV_ERROR_FAILED_TO_ALLOC_VIRT_MEMORY;
			goto FreePTEArray;
		}

		PVR_DPF((PVR_DBG_MESSAGE, "Allocated VM: %s VMA: %p Addr: %p Size: 0x%lx count: %d", __func__,
				psVMA, psVMA->addr, psVMA->size, psVMA->nr_pages));

		for (i = 0; i < uiPagesCount; i++)
		{
			*(pte_array[i]) = pte_mkspecial(pfn_pte((unsigned long) PagePAToPFN(pPagePA[i].uiAddr),
													pgprot_device(PAGE_KERNEL)));
		}

		OSWriteMemoryBarrier(psVMA->addr);

		*ppvLinAddr = psVMA->addr;
		*ppvPrivData = psVMA;

FreePTEArray:
		if (pte_array != pte)
		{
			kfree(pte_array);
		}

		return eError;
	}
#else
	PVR_DPF((PVR_DBG_ERROR,"%s: Cannot map into kernel, no method supported.", __func__));
	PVR_ASSERT(0);
	*ppvLinAddr = NULL;
	return PVRSRV_ERROR_PMR_CPU_PAGE_MAP_FAILED;
#endif
}

void *
OSMapPhysToLin(IMG_CPU_PHYADDR BasePAddr,
			   size_t ui32Bytes,
			   PVRSRV_MEMALLOCFLAGS_T uiMappingFlags)
{
	void __iomem *pvLinAddr;

	if (uiMappingFlags & ~(PVRSRV_MEMALLOCFLAG_CPU_CACHE_MODE_MASK))
	{
		PVR_ASSERT(!"Found non-cpu cache mode flag when mapping to the cpu");
		return NULL;
	}

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	/*
	  This is required to support DMA physheaps for GPU virtualization.
	  Unfortunately, if a region of kernel managed memory is turned into
	  a DMA buffer, conflicting mappings can come about easily on Linux
	  as the original memory is mapped by the kernel as normal cached
	  memory whilst DMA buffers are mapped mostly as uncached device or
	  cache-coherent device memory. In both cases the system will have
	  two conflicting mappings for the same memory region and will have
	  "undefined behaviour" for most processors notably ARMv6 onwards
	  and some x86 micro-architectures. As a result, perform ioremapping
	  manually for DMA physheap allocations by translating from CPU/VA
	  to BUS/PA thereby preventing the creation of conflicting mappings.
	*/
	pvLinAddr = (void __iomem *) SysDmaDevPAddrToCpuVAddr(BasePAddr.uiAddr, ui32Bytes);
	if (pvLinAddr != NULL)
	{
		return (void __force *) pvLinAddr;
	}
#endif

	switch (uiMappingFlags)
	{
		case PVRSRV_MEMALLOCFLAG_CPU_UNCACHED:
			pvLinAddr = (void __iomem *)ioremap(BasePAddr.uiAddr, ui32Bytes);
			break;
		case PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC:
#if defined(CONFIG_X86) || defined(CONFIG_ARM) || defined(CONFIG_ARM64)
			pvLinAddr = (void __iomem *)ioremap_wc(BasePAddr.uiAddr, ui32Bytes);
#else
			pvLinAddr = (void __iomem *)ioremap(BasePAddr.uiAddr, ui32Bytes);
#endif
			break;
		case PVRSRV_MEMALLOCFLAG_CPU_CACHED:
#if defined(CONFIG_X86) || defined(CONFIG_ARM)
			pvLinAddr = (void __iomem *)ioremap_cache(BasePAddr.uiAddr, ui32Bytes);
#else
			pvLinAddr = (void __iomem *)ioremap(BasePAddr.uiAddr, ui32Bytes);
#endif
			break;
		case PVRSRV_MEMALLOCFLAG_CPU_CACHE_COHERENT:
		case PVRSRV_MEMALLOCFLAG_CPU_CACHE_INCOHERENT:
			PVR_ASSERT(!"Unexpected cpu cache mode");
			pvLinAddr = NULL;
			break;
		default:
			PVR_ASSERT(!"Unsupported cpu cache mode");
			pvLinAddr = NULL;
			break;
	}

	return (void __force *) pvLinAddr;
}


IMG_BOOL
OSUnMapPhysToLin(void *pvLinAddr, size_t ui32Bytes)
{
	PVR_UNREFERENCED_PARAMETER(ui32Bytes);

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	if (SysDmaCpuVAddrToDevPAddr(pvLinAddr))
	{
		return IMG_TRUE;
	}
#endif

	iounmap((void __iomem *) pvLinAddr);

	return IMG_TRUE;
}

#define OS_MAX_TIMERS	8

/* Timer callback structure used by OSAddTimer */
typedef struct TIMER_CALLBACK_DATA_TAG
{
	IMG_BOOL			bInUse;
	PFN_TIMER_FUNC		pfnTimerFunc;
	void				*pvData;
	struct timer_list	sTimer;
	IMG_UINT32			ui32Delay;
	IMG_BOOL			bActive;
	struct work_struct	sWork;
}TIMER_CALLBACK_DATA;

static struct workqueue_struct *psTimerWorkQueue;

static TIMER_CALLBACK_DATA sTimers[OS_MAX_TIMERS];

static DEFINE_MUTEX(sTimerStructLock);

static void OSTimerCallbackBody(TIMER_CALLBACK_DATA *psTimerCBData)
{
	if (!psTimerCBData->bActive)
		return;

	/* call timer callback */
	psTimerCBData->pfnTimerFunc(psTimerCBData->pvData);

	/* reset timer */
	mod_timer(&psTimerCBData->sTimer, psTimerCBData->sTimer.expires + psTimerCBData->ui32Delay);
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
/*************************************************************************/ /*!
@Function       OSTimerCallbackWrapper
@Description    OS specific timer callback wrapper function
@Input          psTimer    Timer list structure
*/ /**************************************************************************/
static void OSTimerCallbackWrapper(struct timer_list *psTimer)
{
	TIMER_CALLBACK_DATA *psTimerCBData = from_timer(psTimerCBData, psTimer, sTimer);
#else
/*************************************************************************/ /*!
@Function       OSTimerCallbackWrapper
@Description    OS specific timer callback wrapper function
@Input          uData    Timer callback data
*/ /**************************************************************************/
static void OSTimerCallbackWrapper(uintptr_t uData)
{
	TIMER_CALLBACK_DATA *psTimerCBData = (TIMER_CALLBACK_DATA*)uData;
#endif
	int res;

	res = queue_work(psTimerWorkQueue, &psTimerCBData->sWork);
	if (res == 0)
	{
		PVR_LOG(("OSTimerCallbackWrapper: work already queued"));
	}
}


static void OSTimerWorkQueueCallBack(struct work_struct *psWork)
{
	TIMER_CALLBACK_DATA *psTimerCBData = container_of(psWork, TIMER_CALLBACK_DATA, sWork);

	OSTimerCallbackBody(psTimerCBData);
}

IMG_HANDLE OSAddTimer(PFN_TIMER_FUNC pfnTimerFunc, void *pvData, IMG_UINT32 ui32MsTimeout)
{
	TIMER_CALLBACK_DATA *psTimerCBData;
	IMG_UINT32		ui32i;

	/* check callback */
	if (!pfnTimerFunc)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSAddTimer: passed invalid callback"));
		return NULL;
	}

	/* Allocate timer callback data structure */
	mutex_lock(&sTimerStructLock);
	for (ui32i = 0; ui32i < OS_MAX_TIMERS; ui32i++)
	{
		psTimerCBData = &sTimers[ui32i];
		if (!psTimerCBData->bInUse)
		{
			psTimerCBData->bInUse = IMG_TRUE;
			break;
		}
	}
	mutex_unlock(&sTimerStructLock);
	if (ui32i >= OS_MAX_TIMERS)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSAddTimer: all timers are in use"));
		return NULL;
	}

	psTimerCBData->pfnTimerFunc = pfnTimerFunc;
	psTimerCBData->pvData = pvData;
	psTimerCBData->bActive = IMG_FALSE;

	/*
		HZ = ticks per second
		ui32MsTimeout = required ms delay
		ticks = (Hz * ui32MsTimeout) / 1000
	*/
	psTimerCBData->ui32Delay = ((HZ * ui32MsTimeout) < 1000)
								?	1
								:	((HZ * ui32MsTimeout) / 1000);

	/* initialise object */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	timer_setup(&psTimerCBData->sTimer, OSTimerCallbackWrapper, 0);
#else
	init_timer(&psTimerCBData->sTimer);

	/* setup timer object */
	psTimerCBData->sTimer.function = (void *)OSTimerCallbackWrapper;
	psTimerCBData->sTimer.data = (uintptr_t)psTimerCBData;
#endif

	return (IMG_HANDLE)(uintptr_t)(ui32i + 1);
}


static inline TIMER_CALLBACK_DATA *GetTimerStructure(IMG_HANDLE hTimer)
{
	IMG_UINT32 ui32i = (IMG_UINT32)((uintptr_t)hTimer) - 1;

	PVR_ASSERT(ui32i < OS_MAX_TIMERS);

	return &sTimers[ui32i];
}

PVRSRV_ERROR OSRemoveTimer (IMG_HANDLE hTimer)
{
	TIMER_CALLBACK_DATA *psTimerCBData = GetTimerStructure(hTimer);

	PVR_ASSERT(psTimerCBData->bInUse);
	PVR_ASSERT(!psTimerCBData->bActive);

	/* free timer callback data struct */
	psTimerCBData->bInUse = IMG_FALSE;

	return PVRSRV_OK;
}

PVRSRV_ERROR OSEnableTimer (IMG_HANDLE hTimer)
{
	TIMER_CALLBACK_DATA *psTimerCBData = GetTimerStructure(hTimer);

	PVR_ASSERT(psTimerCBData->bInUse);
	PVR_ASSERT(!psTimerCBData->bActive);

	/* Start timer arming */
	psTimerCBData->bActive = IMG_TRUE;

	/* set the expire time */
	psTimerCBData->sTimer.expires = psTimerCBData->ui32Delay + jiffies;

	/* Add the timer to the list */
	add_timer(&psTimerCBData->sTimer);

	return PVRSRV_OK;
}


PVRSRV_ERROR OSDisableTimer (IMG_HANDLE hTimer)
{
	TIMER_CALLBACK_DATA *psTimerCBData = GetTimerStructure(hTimer);

	PVR_ASSERT(psTimerCBData->bInUse);
	PVR_ASSERT(psTimerCBData->bActive);

	/* Stop timer from arming */
	psTimerCBData->bActive = IMG_FALSE;
	smp_mb();

	flush_workqueue(psTimerWorkQueue);

	/* remove timer */
	del_timer_sync(&psTimerCBData->sTimer);

	/*
	 * This second flush is to catch the case where the timer ran
	 * before we managed to delete it, in which case, it will have
	 * queued more work for the workqueue. Since the bActive flag
	 * has been cleared, this second flush won't result in the
	 * timer being rearmed.
	 */
	flush_workqueue(psTimerWorkQueue);

	return PVRSRV_OK;
}


PVRSRV_ERROR OSEventObjectCreate(const IMG_CHAR *pszName, IMG_HANDLE *hEventObject)
{
	PVR_UNREFERENCED_PARAMETER(pszName);

	PVR_LOG_RETURN_IF_INVALID_PARAM(hEventObject, "hEventObject");

	return LinuxEventObjectListCreate(hEventObject);
}


PVRSRV_ERROR OSEventObjectDestroy(IMG_HANDLE hEventObject)
{
	PVR_LOG_RETURN_IF_INVALID_PARAM(hEventObject, "hEventObject");

	return LinuxEventObjectListDestroy(hEventObject);
}

PVRSRV_ERROR OSEventObjectWaitTimeout(IMG_HANDLE hOSEventKM, IMG_UINT64 uiTimeoutus)
{
	PVRSRV_ERROR eError;

	if (hOSEventKM && uiTimeoutus > 0)
	{
		eError = LinuxEventObjectWait(hOSEventKM, uiTimeoutus, _NON_FREEZABLE);
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "OSEventObjectWait: invalid arguments %p, %lld", hOSEventKM, uiTimeoutus));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
	}

	return eError;
}

PVRSRV_ERROR OSEventObjectWait(IMG_HANDLE hOSEventKM)
{
	return OSEventObjectWaitTimeout(hOSEventKM, EVENT_OBJECT_TIMEOUT_US);
}

PVRSRV_ERROR OSEventObjectWaitKernel(IMG_HANDLE hOSEventKM,
                                     IMG_UINT64 uiTimeoutus)
{
	PVRSRV_ERROR eError;

	if (hOSEventKM != NULL && uiTimeoutus > 0)
	{
		eError = LinuxEventObjectWait(hOSEventKM, uiTimeoutus, _FREEZABLE);
	}
#if defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP)
	else if (hOSEventKM != NULL)
	{
		eError = LinuxEventObjectWaitUntilSignalled(hOSEventKM);
	}
#endif /* defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP) */
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "OSEventObjectWaitKernel: invalid arguments %p",
		        hOSEventKM));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
	}

	return eError;
}

void OSEventObjectDumpDebugInfo(IMG_HANDLE hOSEventKM)
{
	LinuxEventObjectDumpDebugInfo(hOSEventKM);
}

PVRSRV_ERROR OSEventObjectOpen(IMG_HANDLE hEventObject, IMG_HANDLE *phOSEvent)
{
	PVRSRV_ERROR eError;

	PVR_LOG_RETURN_IF_INVALID_PARAM(phOSEvent, "phOSEvent");
	PVR_LOG_GOTO_IF_INVALID_PARAM(hEventObject, eError, error);

	eError = LinuxEventObjectAdd(hEventObject, phOSEvent);
	PVR_LOG_GOTO_IF_ERROR(eError, "LinuxEventObjectAdd", error);

	return PVRSRV_OK;

error:
	*phOSEvent = NULL;
	return eError;
}

PVRSRV_ERROR OSEventObjectClose(IMG_HANDLE hOSEventKM)
{
	PVR_LOG_RETURN_IF_INVALID_PARAM(hOSEventKM, "hOSEventKM");

	return LinuxEventObjectDelete(hOSEventKM);
}

PVRSRV_ERROR OSEventObjectSignal(IMG_HANDLE hEventObject)
{
	PVR_LOG_RETURN_IF_INVALID_PARAM(hEventObject, "hEventObject");

	return LinuxEventObjectSignal(hEventObject);
}

PVRSRV_ERROR OSCopyToUser(void *pvProcess,
						  void __user *pvDest,
						  const void *pvSrc,
						  size_t ui32Bytes)
{
	PVR_UNREFERENCED_PARAMETER(pvProcess);

	if (pvr_copy_to_user(pvDest, pvSrc, ui32Bytes)==0)
		return PVRSRV_OK;
	else
		return PVRSRV_ERROR_FAILED_TO_COPY_VIRT_MEMORY;
}

PVRSRV_ERROR OSCopyFromUser(void *pvProcess,
							void *pvDest,
							const void __user *pvSrc,
							size_t ui32Bytes)
{
	PVR_UNREFERENCED_PARAMETER(pvProcess);

	if (likely(pvr_copy_from_user(pvDest, pvSrc, ui32Bytes)==0))
		return PVRSRV_OK;
	else
		return PVRSRV_ERROR_FAILED_TO_COPY_VIRT_MEMORY;
}

IMG_UINT64 OSDivide64r64(IMG_UINT64 ui64Divident, IMG_UINT32 ui32Divisor, IMG_UINT32 *pui32Remainder)
{
	*pui32Remainder = do_div(ui64Divident, ui32Divisor);

	return ui64Divident;
}

IMG_UINT32 OSDivide64(IMG_UINT64 ui64Divident, IMG_UINT32 ui32Divisor, IMG_UINT32 *pui32Remainder)
{
	*pui32Remainder = do_div(ui64Divident, ui32Divisor);

	return (IMG_UINT32) ui64Divident;
}

/* One time osfunc initialisation */
PVRSRV_ERROR PVROSFuncInit(void)
{
	{
		PVR_ASSERT(!psTimerWorkQueue);

		psTimerWorkQueue = create_freezable_workqueue("pvr_timer");
		if (psTimerWorkQueue == NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: couldn't create timer workqueue",
					 __func__));
			return PVRSRV_ERROR_UNABLE_TO_CREATE_THREAD;
		}
	}

	{
		IMG_UINT32 ui32i;

		for (ui32i = 0; ui32i < OS_MAX_TIMERS; ui32i++)
		{
			TIMER_CALLBACK_DATA *psTimerCBData = &sTimers[ui32i];

			INIT_WORK(&psTimerCBData->sWork, OSTimerWorkQueueCallBack);
		}
	}
	return PVRSRV_OK;
}

/*
 * Osfunc deinitialisation.
 * Note that PVROSFuncInit may not have been called
 */
void PVROSFuncDeInit(void)
{
	if (psTimerWorkQueue != NULL)
	{
		destroy_workqueue(psTimerWorkQueue);
		psTimerWorkQueue = NULL;
	}
}

void OSDumpStack(void)
{
	dump_stack();
}

/*************************************************************************/ /*!
@Function       OSDebugSignalPID
@Description    Sends a SIGTRAP signal to a specific PID in user mode for
                debugging purposes. The user mode process can register a handler
                against this signal.
                This is necessary to support the Rogue debugger. If the Rogue
                debugger is not used then this function may be implemented as
                a stub.
@Input          ui32PID    The PID for the signal.
@Return         PVRSRV_OK on success, a failure code otherwise.
*/ /**************************************************************************/
PVRSRV_ERROR OSDebugSignalPID(IMG_UINT32 ui32PID)
{
	int err;
	struct pid *psPID;

	psPID = find_vpid(ui32PID);
	if (psPID == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get PID struct.", __func__));
		return PVRSRV_ERROR_NOT_FOUND;
	}

	err = kill_pid(psPID, SIGTRAP, 0);
	if (err != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Signal Failure %d", __func__, err));
		return PVRSRV_ERROR_SIGNAL_FAILED;
	}

	return PVRSRV_OK;
}

/*************************************************************************/ /*!
@Function       OSIsKernelThread
@Description    This API determines if the current running thread is a kernel
                thread (i.e. one not associated with any userland process,
                typically an MISR handler.)
@Return         IMG_TRUE if it is a kernel thread, otherwise IMG_FALSE.
*/ /**************************************************************************/
IMG_BOOL OSIsKernelThread(void)
{
	/*
	 * Kernel threads have a NULL memory descriptor.
	 *
	 * See https://www.kernel.org/doc/Documentation/vm/active_mm.txt
	 */
	return current->mm == NULL;
}

void OSDumpVersionInfo(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					   void *pvDumpDebugFile)
{
	PVR_DUMPDEBUG_LOG("OS kernel info: %s %s %s %s",
					utsname()->sysname,
					utsname()->release,
					utsname()->version,
					utsname()->machine);
}
#if defined(SUPPORT_DMA_TRANSFER)

typedef struct _OS_CLEANUP_DATA_
{
	IMG_BOOL bSucceed;
	IMG_BOOL bAdvanceTimeline;
	IMG_UINT uiRefCount;
	IMG_UINT uiNumDMA;
	IMG_UINT uiCount;

	struct dma_async_tx_descriptor** ppsDescriptors;


	PVRSRV_DEVICE_NODE *psDevNode;
	PFN_SERVER_CLEANUP pfnServerCleanup;
	void* pvServerCleanupData;

	enum dma_data_direction eDirection;
	struct sg_table **ppsSg;
	struct page ***pages;
	IMG_UINT32* puiNumPages;
	spinlock_t spinlock;

	struct completion start_cleanup;
	struct completion *sync_completion;

	/* Sparse PMR transfer information */
	IMG_BOOL *pbIsSparse;
	IMG_UINT *uiNumValidPages;
	struct sg_table ***ppsSgSparse;
	struct dma_async_tx_descriptor*** ppsDescriptorsSparse;

} OS_CLEANUP_DATA;

static int cleanup_thread(void *pvData)
{
	IMG_UINT32 i, j;
	struct completion *sync_completion = NULL;
	OS_CLEANUP_DATA *psOSCleanup = (OS_CLEANUP_DATA*)pvData;
	IMG_BOOL bSucceed = psOSCleanup->bSucceed;

	sync_completion = psOSCleanup->sync_completion;

#if defined(DMA_VERBOSE)
	PVR_DPF((PVR_DBG_ERROR, "Cleanup thread waiting (%p) on completion", pvData));
#endif

	wait_for_completion(&psOSCleanup->start_cleanup);

#if defined(DMA_VERBOSE)
	PVR_DPF((PVR_DBG_ERROR, "Cleanup thread notified (%p)", pvData));
#endif
	/* Free resources */
	for (i=0; i<psOSCleanup->uiCount; i++)
	{
		if (!psOSCleanup->pbIsSparse[i])
		{
			dma_sync_sg_for_cpu(psOSCleanup->psDevNode->psDevConfig->pvOSDevice,
								psOSCleanup->ppsSg[i]->sgl,
								psOSCleanup->ppsSg[i]->nents,
								psOSCleanup->eDirection);

			dma_unmap_sg(psOSCleanup->psDevNode->psDevConfig->pvOSDevice,
						 psOSCleanup->ppsSg[i]->sgl,
						 psOSCleanup->ppsSg[i]->nents,
						 psOSCleanup->eDirection);

			sg_free_table(psOSCleanup->ppsSg[i]);

			OSFreeMem(psOSCleanup->ppsSg[i]);

			/* Unpin pages */
			for (j=0; j<psOSCleanup->puiNumPages[i]; j++)
			{
				/*
				 * using DMA_FROM_DEVICE from enum dma_data_direction instead of DMA_DEV_TO_MEM
				 * from enum dma_transfer_direction to avoid casting explicitly
				 */
				if (psOSCleanup->eDirection == DMA_FROM_DEVICE)
				{
					set_page_dirty_lock(psOSCleanup->pages[i][j]);
				}

				pvr_unpin_user_page_for_dma(psOSCleanup->pages[i][j]);
			}
		}
		else
		{
			for (j = 0; j < psOSCleanup->puiNumPages[i]; j++)
			{
				if (psOSCleanup->ppsSgSparse[i][j]) {
					dma_sync_sg_for_cpu(psOSCleanup->psDevNode->psDevConfig->pvOSDevice,
										psOSCleanup->ppsSgSparse[i][j]->sgl,
										psOSCleanup->ppsSgSparse[i][j]->nents,
										psOSCleanup->eDirection);


					dma_unmap_sg(psOSCleanup->psDevNode->psDevConfig->pvOSDevice,
								psOSCleanup->ppsSgSparse[i][j]->sgl,
								psOSCleanup->ppsSgSparse[i][j]->nents,
								psOSCleanup->eDirection);

					sg_free_table(psOSCleanup->ppsSgSparse[i][j]);

					OSFreeMem(psOSCleanup->ppsSgSparse[i][j]);

				}
			}

			OSFreeMem(psOSCleanup->ppsSgSparse[i]);
			OSFreeMem(psOSCleanup->ppsDescriptorsSparse[i]);

			/* Unpin pages */
			for (j=0; j<psOSCleanup->puiNumPages[i]*2; j++)
			{
				/*
				 * Some pages might've been pinned twice
				 * Others may have not been pinned at all
				 */
				if (psOSCleanup->pages[i][j])
				{
					/*
					 * using DMA_FROM_DEVICE from enum dma_data_direction instead of DMA_DEV_TO_MEM
					 * from enum dma_transfer_direction to avoid casting explicitly
					 */
					if (psOSCleanup->eDirection == DMA_FROM_DEVICE)
					{
						set_page_dirty_lock(psOSCleanup->pages[i][j]);
					}

					pvr_unpin_user_page_for_dma(psOSCleanup->pages[i][j]);
				}
			}
		}

		OSFreeMem(psOSCleanup->pages[i]);
	}

	psOSCleanup->pfnServerCleanup(psOSCleanup->pvServerCleanupData,
								  psOSCleanup->bAdvanceTimeline);

	OSFreeMem(psOSCleanup->ppsSg);
	OSFreeMem(psOSCleanup->pages);
	OSFreeMem(psOSCleanup->puiNumPages);
	OSFreeMem(psOSCleanup->ppsSgSparse);
	OSFreeMem(psOSCleanup->ppsDescriptorsSparse);
	OSFreeMem(psOSCleanup->ppsDescriptors);
	OSFreeMem(psOSCleanup->pbIsSparse);
	OSFreeMem(psOSCleanup->uiNumValidPages);
	OSFreeMem(psOSCleanup);

	if (bSucceed)
	{
		kthread_complete_and_exit(sync_completion, 0);
	}
	else
	{
		kthread_complete_and_exit(NULL, 0);
	}
	return 0;
}

static void dma_callback(void *pvOSCleanup)
{
	OS_CLEANUP_DATA *psOSCleanup = (OS_CLEANUP_DATA*)pvOSCleanup;
	unsigned long flags;

#if defined(DMA_VERBOSE)
	PVR_DPF((PVR_DBG_ERROR, "dma_callback (%p) refcount decreased to %d", psOSCleanup, psOSCleanup->uiRefCount - 1));
#endif
	spin_lock_irqsave(&psOSCleanup->spinlock, flags);

	psOSCleanup->uiRefCount--;

	if (psOSCleanup->uiRefCount==0)
	{
		/* Notify the cleanup thread */
		spin_unlock_irqrestore(&psOSCleanup->spinlock, flags);
		complete(&psOSCleanup->start_cleanup);
		return;
	}

	spin_unlock_irqrestore(&psOSCleanup->spinlock, flags);
}

#if defined(PVRSRV_DEBUG_DMA)
static void
DMADumpPhysicalAddresses(struct page **ppsHostMemPages,
						 IMG_UINT32 uiNumPages,
						 IMG_DMA_ADDR *sDmaAddr,
						 IMG_UINT64 ui64Offset)
{
	IMG_CPU_PHYADDR sPagePhysAddr;
	IMG_UINT32 uiIdx;

	PVR_DPF((PVR_DBG_MESSAGE, "DMA Transfer Address Dump"));
	PVR_DPF((PVR_DBG_MESSAGE, "Hostmem phys addresses:"));

	for (uiIdx = 0; uiIdx < uiNumPages; uiIdx++)
	{
		sPagePhysAddr.uiAddr = page_to_phys(ppsHostMemPages[uiIdx]);
		if (uiIdx == 0)
		{
			sPagePhysAddr.uiAddr += ui64Offset;
			PVR_DPF((PVR_DBG_MESSAGE,
					 "\tHost mem start at 0x%" IMG_UINT64_FMTSPECx,
					 (IMG_UINT64)sPagePhysAddr.uiAddr));
		}
		else
		{
			PVR_DPF((PVR_DBG_MESSAGE,
					 "\tHost Mem Page %d at 0x%" IMG_UINT64_FMTSPECx,
					 uiIdx, (IMG_UINT64)sPagePhysAddr.uiAddr));
		}
	}
	PVR_DPF((PVR_DBG_MESSAGE,
			 "Devmem CPU phys address: 0x%" IMG_UINT64_FMTSPECx,
			 (IMG_UINT64)sDmaAddr->uiAddr));
}
#endif

PVRSRV_ERROR OSDmaSubmitTransfer(PVRSRV_DEVICE_NODE *psDevNode, void *pvOSData,
				    void *pvChan, IMG_BOOL bSynchronous)
{
	OS_CLEANUP_DATA *psOSCleanup = (OS_CLEANUP_DATA*)pvOSData;
	struct completion* sync_completion = NULL;

	psOSCleanup->bSucceed = IMG_TRUE;
	psOSCleanup->bAdvanceTimeline = IMG_TRUE;

	if (bSynchronous)
	{
		sync_completion = OSAllocZMem(sizeof(struct completion));
		init_completion(sync_completion);
	}

	PVR_UNREFERENCED_PARAMETER(psDevNode);
	/* Wait only on number of ops scheduled. This might be different to NumDMAs
	in certain error conditions */
	psOSCleanup->uiRefCount = psOSCleanup->uiCount;
	psOSCleanup->sync_completion = sync_completion;

	{
		IMG_UINT32 i,j;
		for (i=0; i<psOSCleanup->uiCount; i++)
		{
			if (psOSCleanup->pbIsSparse[i])
			{
				for (j=0; j<psOSCleanup->puiNumPages[i]; j++)
				{
					if (psOSCleanup->ppsDescriptorsSparse[i][j])
						dmaengine_submit(psOSCleanup->ppsDescriptorsSparse[i][j]);
				}
			}
			else
			{
				dmaengine_submit(psOSCleanup->ppsDescriptors[i]);
			}
		}
	}

	dma_async_issue_pending(pvChan);

	if (bSynchronous)
	{
		wait_for_completion(sync_completion);
		OSFreeMem(sync_completion);
	}

	return PVRSRV_OK;
}

void OSDmaForceCleanup(PVRSRV_DEVICE_NODE *psDevNode, void *pvChan,
					   void *pvOSData, void *pvServerCleanupParam,
					   PFN_SERVER_CLEANUP pfnServerCleanup)
{
	OS_CLEANUP_DATA *psOSCleanup = (OS_CLEANUP_DATA *)pvOSData;
	IMG_UINT ui32Retries;

	PVR_UNREFERENCED_PARAMETER(psDevNode);

	psOSCleanup->bSucceed = IMG_FALSE;
	psOSCleanup->bAdvanceTimeline = IMG_TRUE;

	/* Need to wait for outstanding DMA Engine ops before advancing the
	   user-supplied timeline in case of error. dmaengine_terminate_sync
	   cannot be called from within atomic context, so cannot invoke it
	   from inside the cleanup kernel thread. */
	for (ui32Retries = 0; ui32Retries < DMA_ERROR_SYNC_RETRIES; ui32Retries++)
	{
		if (dmaengine_terminate_sync(pvChan) == 0)
		{
			break;
		}
	}
	if (ui32Retries == DMA_ERROR_SYNC_RETRIES)
	{
		/* We cannot guarantee all outstanding DMAs were terminated
		 * so we let the UM fence time out as a fallback mechanism */
		psOSCleanup->bAdvanceTimeline = IMG_FALSE;
	}

	if (psOSCleanup->uiCount > 0)
	{
		complete(&psOSCleanup->start_cleanup);
	}
	else
	{
		/* Cleanup kthread never run, need to manually wind down */
		pfnServerCleanup(pvServerCleanupParam, psOSCleanup->bAdvanceTimeline);

		OSFreeMem(psOSCleanup->ppsSg);
		OSFreeMem(psOSCleanup->pages);
		OSFreeMem(psOSCleanup->puiNumPages);
		OSFreeMem(psOSCleanup->ppsSgSparse);
		OSFreeMem(psOSCleanup->pbIsSparse);
		OSFreeMem(psOSCleanup->uiNumValidPages);
		OSFreeMem(psOSCleanup->ppsDescriptors);
		OSFreeMem(psOSCleanup->ppsDescriptorsSparse);

		OSFreeMem(psOSCleanup);
	}
}

PVRSRV_ERROR OSDmaAllocData(PVRSRV_DEVICE_NODE *psDevNode, IMG_UINT32 uiNumDMA, void **pvOutData)
{
	PVRSRV_ERROR eError;
	OS_CLEANUP_DATA *psOSCleanup = OSAllocZMem(sizeof(OS_CLEANUP_DATA));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanup, eError, e0);

	psOSCleanup->uiNumDMA = uiNumDMA;
	psOSCleanup->psDevNode = psDevNode;

	spin_lock_init(&psOSCleanup->spinlock);

	init_completion(&psOSCleanup->start_cleanup);

	psOSCleanup->ppsDescriptors = OSAllocZMem(uiNumDMA * sizeof(struct dma_async_tx_descriptor*));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanup->ppsDescriptors, eError, e0);

	psOSCleanup->ppsDescriptorsSparse = OSAllocZMem(uiNumDMA * sizeof(struct dma_async_tx_descriptor*));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanup->ppsDescriptorsSparse, eError, e11);

	psOSCleanup->ppsSg = OSAllocZMem(uiNumDMA * sizeof(struct sg_table*));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanup->ppsSg, eError, e1);

	psOSCleanup->ppsSgSparse = OSAllocZMem(uiNumDMA * sizeof(struct sg_table*));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanup->ppsSgSparse, eError, e12);

	psOSCleanup->pbIsSparse = OSAllocZMem(uiNumDMA * sizeof(IMG_BOOL));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanup->pbIsSparse, eError, e13);

	psOSCleanup->uiNumValidPages = OSAllocZMem(uiNumDMA * sizeof(IMG_UINT));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanup->uiNumValidPages, eError, e14);

	psOSCleanup->pages = OSAllocZMem(uiNumDMA * sizeof(struct page **));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanup->pages, eError, e2);

	psOSCleanup->puiNumPages = OSAllocZMem(uiNumDMA * sizeof(IMG_UINT32));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanup->puiNumPages, eError, e3);

	*pvOutData = psOSCleanup;

	return PVRSRV_OK;

e3:
	OSFreeMem(psOSCleanup->pages);
e2:
	OSFreeMem(psOSCleanup->uiNumValidPages);
e14:
	OSFreeMem(psOSCleanup->pbIsSparse);
e13:
	OSFreeMem(psOSCleanup->ppsSgSparse);
e12:
	OSFreeMem(psOSCleanup->ppsSg);
e1:
	OSFreeMem(psOSCleanup->ppsDescriptorsSparse);
e11:
	OSFreeMem(psOSCleanup->ppsDescriptors);
e0:
	OSFreeMem(psOSCleanup);
	return eError;
}

/*************************************************************************/ /*!
@Function       OSDmaTransfer
@Description    This API is used to ask OS to perform a DMA transfer operation
@Return
*/ /**************************************************************************/
PVRSRV_ERROR OSDmaPrepareTransfer(PVRSRV_DEVICE_NODE *psDevNode,
							   void* pvChan,
							   IMG_DMA_ADDR* psDmaAddr, IMG_UINT64* puiAddress,
							   IMG_UINT64 uiSize, IMG_BOOL bMemToDev,
							   void* pvOSData,
							   void* pvServerCleanupParam, PFN_SERVER_CLEANUP pfnServerCleanup, IMG_BOOL bFirst)
{

	IMG_INT iRet;
	PVRSRV_ERROR eError;
	PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;
	OS_CLEANUP_DATA* psOSCleanupData = pvOSData;

	enum dma_data_direction eDataDirection = bMemToDev ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
	struct dma_slave_config sConfig = {0};
	struct dma_async_tx_descriptor *psDesc;

	unsigned long offset = (unsigned long)puiAddress & ((1 << PAGE_SHIFT) - 1);
	unsigned int num_pages = (uiSize + offset + PAGE_SIZE - 1) >> PAGE_SHIFT;
	int num_pinned_pages = 0;

	struct sg_table *psSg = OSAllocZMem(sizeof(struct sg_table));
	PVR_LOG_GOTO_IF_NOMEM(psSg, eError, e0);

	psOSCleanupData->pages[psOSCleanupData->uiCount] = OSAllocZMem(num_pages * sizeof(struct page *));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanupData->pages[psOSCleanupData->uiCount], eError, e1);

	num_pinned_pages = pvr_pin_user_pages_for_dma(
		puiAddress,
		num_pages,
		!bMemToDev,
		psOSCleanupData->pages[psOSCleanupData->uiCount]);

	psOSCleanupData->puiNumPages[psOSCleanupData->uiCount] = num_pinned_pages;
	if (num_pinned_pages != num_pages)
	{
		PVR_DPF((PVR_DBG_ERROR, "get_user_pages_fast failed: (%d - %u)", num_pinned_pages, num_pages));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto e2; /* Unpin what was pinned and return error */
	}

#if defined(PVRSRV_DEBUG_DMA)
	DMADumpPhysicalAddresses(psOSCleanupData->pages[psOSCleanupData->uiCount],
							 num_pages, psDmaAddr, offset);
#endif

	if (sg_alloc_table_from_pages(psSg, psOSCleanupData->pages[psOSCleanupData->uiCount], num_pages, offset, uiSize, GFP_KERNEL) != 0)
	{
		eError = PVRSRV_ERROR_BAD_MAPPING;
		PVR_DPF((PVR_DBG_ERROR, "sg_alloc_table_from_pages failed"));
		goto e2;
	}

	if (bMemToDev)
	{
		sConfig.direction = DMA_MEM_TO_DEV;
		sConfig.src_addr = 0;
		sConfig.dst_addr = psDmaAddr->uiAddr;
	}
	else
	{
		sConfig.direction = DMA_DEV_TO_MEM;
		sConfig.src_addr = psDmaAddr->uiAddr;
		sConfig.dst_addr = 0;
	}
	dmaengine_slave_config(pvChan, &sConfig);

	iRet = dma_map_sg(psDevConfig->pvOSDevice, psSg->sgl, psSg->nents, eDataDirection);
	if (!iRet)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Error mapping SG list", __func__));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto e3;
	}

	dma_sync_sg_for_device(psDevConfig->pvOSDevice, psSg->sgl,(unsigned int)iRet, eDataDirection);

	psDesc = dmaengine_prep_slave_sg(pvChan, psSg->sgl, (unsigned int)iRet, sConfig.direction, 0);
	if (!psDesc)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: dmaengine_prep_slave_sg failed", __func__));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto e4;
	}

	psOSCleanupData->eDirection = eDataDirection;
	psOSCleanupData->ppsSg[psOSCleanupData->uiCount] = psSg;
	psOSCleanupData->pfnServerCleanup = pfnServerCleanup;
	psOSCleanupData->pvServerCleanupData = pvServerCleanupParam;

	psDesc->callback_param = psOSCleanupData;
	psDesc->callback = dma_callback;

	if (bFirst)
		kthread_run(cleanup_thread, psOSCleanupData, "dma-cleanup-thread");
	psOSCleanupData->ppsDescriptors[psOSCleanupData->uiCount] = psDesc;

	psOSCleanupData->uiCount++;

	return PVRSRV_OK;

e4:
	dma_unmap_sg(psDevConfig->pvOSDevice, psSg->sgl, psSg->nents, eDataDirection);
e3:
	sg_free_table(psSg);
e2:
	{
		IMG_UINT32 i;
		/* Unpin pages */
		for (i=0; i<psOSCleanupData->puiNumPages[psOSCleanupData->uiCount]; i++)
		{
			pvr_unpin_user_page_for_dma(psOSCleanupData->pages[psOSCleanupData->uiCount][i]);
		}
	}
	OSFreeMem(psOSCleanupData->pages[psOSCleanupData->uiCount]);
e1:
	OSFreeMem(psSg);
e0:
	return eError;
}

static IMG_UINT32
CalculateValidPages(IMG_BOOL *pbValid,
					IMG_UINT32 ui32SizeInPages)
{
	IMG_UINT32 ui32nValid;
	IMG_UINT32 ui32Idx;

	for (ui32Idx = 0, ui32nValid = 0; ui32Idx < ui32SizeInPages; ui32Idx++)
	{
		ui32nValid += pbValid[ui32Idx] ? 1 : 0;
	}

	return ui32nValid;
}

PVRSRV_ERROR OSDmaPrepareTransferSparse(PVRSRV_DEVICE_NODE *psDevNode,
										void* pvChan,
										IMG_DMA_ADDR* psDmaAddr,
										IMG_BOOL *pbValid,
										IMG_UINT64* puiAddress,
										IMG_UINT64 uiSize,
										IMG_UINT32 uiOffsetInFirstPMRPage,
										IMG_UINT32 ui32SizeInPages,
										IMG_BOOL bMemToDev,
										void* pvOSData,
										void* pvServerCleanupParam,
										PFN_SERVER_CLEANUP pfnServerCleanup,
										IMG_BOOL bFirst)
{

	IMG_INT iRet;
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;
	OS_CLEANUP_DATA* psOSCleanupData = pvOSData;
	IMG_UINT32 ui32PageSize = OSGetPageSize();
	void *pvNextAddress = puiAddress;
	IMG_UINT32 ui32Idx;
	IMG_INT32 i32Rwd;

	enum dma_data_direction eDataDirection = bMemToDev ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
	struct dma_slave_config sConfig = {0};
	struct dma_async_tx_descriptor *psDesc;

	unsigned long offset = (unsigned long)puiAddress & ((1 << PAGE_SHIFT) - 1);
	unsigned int num_pages = (uiSize + offset + PAGE_SIZE - 1) >> PAGE_SHIFT;
	unsigned int num_valid_pages = CalculateValidPages(pbValid, ui32SizeInPages);
	unsigned int num_pinned_pages = 0;
	unsigned int valid_idx;
	size_t transfer_size;
	struct page ** next_pages;
	struct sg_table *psSg;

	psOSCleanupData->uiNumValidPages[psOSCleanupData->uiCount] = num_valid_pages;
	psOSCleanupData->pbIsSparse[psOSCleanupData->uiCount] = IMG_TRUE;

	/*
	 * If an SG transfer from virtual memory to card memory goes over a page boundary in
	 * main memory, it'll span two different pages - therefore, total number of pages to
	 * keep track of should be twice as many as for a simple transfer. This twice-as-big
	 * allocation is also necessary because the same virtual memory page might be present
	 * in more than one SG DMA transfer, because of differences in first-page offset between
	 * the sparse device PMR and the virtual memory buffer.
	 */
	psOSCleanupData->pages[psOSCleanupData->uiCount] = OSAllocZMem(2*num_valid_pages * sizeof(struct page *));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanupData->pages[psOSCleanupData->uiCount], eError, e0);

	psOSCleanupData->ppsSgSparse[psOSCleanupData->uiCount] = OSAllocZMem(num_valid_pages * sizeof(struct sg_table *));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanupData->ppsSgSparse[psOSCleanupData->uiCount], eError, e1);

	psOSCleanupData->ppsDescriptorsSparse[psOSCleanupData->uiCount] = OSAllocZMem(num_valid_pages * sizeof(struct dma_async_tx_descriptor *));
	PVR_LOG_GOTO_IF_NOMEM(psOSCleanupData->ppsDescriptorsSparse[psOSCleanupData->uiCount], eError, e11);

	for (ui32Idx = 0, valid_idx = 0; ui32Idx < ui32SizeInPages; ui32Idx++)
	{
		if (valid_idx == num_valid_pages)
		{
			break;
		}
		if (!pbValid[ui32Idx])
		{
			pvNextAddress += (ui32Idx == 0) ? ui32PageSize - uiOffsetInFirstPMRPage : ui32PageSize;
			continue;
		}

		/* Pick transfer size */
		if (ui32Idx == 0)
		{
			if (uiOffsetInFirstPMRPage + uiSize <= ui32PageSize)
			{
				PVR_ASSERT(num_valid_pages == 1);
				transfer_size = uiSize;
			}
			else
			{
				transfer_size = ui32PageSize - uiOffsetInFirstPMRPage;
			}
		}
		else
		{
			/* Last valid LMA page */
			if (valid_idx == num_valid_pages - 1)
			{
				transfer_size = ((uiOffsetInFirstPMRPage + uiSize - 1) % ui32PageSize) + 1;
			}
			else
			{
				transfer_size = ui32PageSize;
			}
		}

		if (((unsigned long long)pvNextAddress & (ui32PageSize - 1)) + transfer_size > ui32PageSize)
		{
			num_pages = 2;
		}
		else
		{
			num_pages = 1;
		}

		next_pages = psOSCleanupData->pages[psOSCleanupData->uiCount] + (valid_idx * 2);

		num_pinned_pages = pvr_pin_user_pages_for_dma(
			pvNextAddress,
			num_pages,
			!bMemToDev,
			next_pages);

		if (num_pinned_pages != num_pages)
		{
			PVR_DPF((PVR_DBG_ERROR, "get_user_pages_fast for sparse failed: (%d - %u)", num_pinned_pages, num_pages));
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto e2;
		}

#if defined(PVRSRV_DEBUG_DMA)
		DMADumpPhysicalAddresses(next_pages, num_pages,
								 &psDmaAddr[ui32Idx],
								 (unsigned long)pvNextAddress & (ui32PageSize - 1));
#endif

		psSg = OSAllocZMem(sizeof(struct sg_table));
		PVR_LOG_GOTO_IF_NOMEM(psSg, eError, e3);

		if (sg_alloc_table_from_pages(psSg, next_pages, num_pages,
									  (unsigned long)pvNextAddress & (ui32PageSize - 1),
									  transfer_size,
									  GFP_KERNEL) != 0)
		{
			eError = PVRSRV_ERROR_BAD_MAPPING;
			PVR_DPF((PVR_DBG_ERROR, "sg_alloc_table_from_pages failed"));
			goto e4;
		}

		pvNextAddress += transfer_size;

		if (bMemToDev)
		{
			sConfig.direction = DMA_MEM_TO_DEV;
			sConfig.src_addr = 0;
			sConfig.dst_addr = psDmaAddr[ui32Idx].uiAddr;
		}
		else
		{
			sConfig.direction = DMA_DEV_TO_MEM;
			sConfig.src_addr = psDmaAddr[ui32Idx].uiAddr;
			sConfig.dst_addr = 0;
		}
		dmaengine_slave_config(pvChan, &sConfig);

		iRet = dma_map_sg(psDevConfig->pvOSDevice, psSg->sgl, psSg->nents, eDataDirection);
		if (!iRet)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Error mapping SG list", __func__));
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			goto e5;
		}
		dma_sync_sg_for_device(psDevConfig->pvOSDevice, psSg->sgl,(unsigned int)iRet, eDataDirection);

		psDesc = dmaengine_prep_slave_sg(pvChan, psSg->sgl, (unsigned int)iRet, sConfig.direction, 0);
		if (!psDesc)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: dmaengine_prep_slave_sg failed", __func__));
			eError = PVRSRV_ERROR_INVALID_PARAMS;

		goto e6;
		}

		psOSCleanupData->ppsSgSparse[psOSCleanupData->uiCount][valid_idx] = psSg;
		psOSCleanupData->ppsDescriptorsSparse[psOSCleanupData->uiCount][valid_idx] = psDesc;
		psOSCleanupData->puiNumPages[psOSCleanupData->uiCount] = ++valid_idx;

		if (valid_idx == num_valid_pages)
		{
			psDesc->callback_param = psOSCleanupData;
			psDesc->callback = dma_callback;

			if (bFirst)
			{
				psOSCleanupData->eDirection = eDataDirection;
				psOSCleanupData->pfnServerCleanup = pfnServerCleanup;
				psOSCleanupData->pvServerCleanupData = pvServerCleanupParam;

				kthread_run(cleanup_thread, psOSCleanupData, "dma-cleanup-thread");
			}

			psOSCleanupData->uiCount++;
		}

	}

	return PVRSRV_OK;

e6:
	dma_unmap_sg(psDevConfig->pvOSDevice, psSg->sgl, psSg->nents, eDataDirection);
e5:
	sg_free_table(psSg);
e4:
	OSFreeMem(psSg);
e3:
	/* Unpin last */
	pvr_unpin_user_page_for_dma(psOSCleanupData->pages[psOSCleanupData->uiCount][valid_idx]);

	if (psOSCleanupData->pages[psOSCleanupData->uiCount][valid_idx+1])
	{
		pvr_unpin_user_page_for_dma(psOSCleanupData->pages[psOSCleanupData->uiCount][valid_idx+1]);
	}
e2:
	/* rewind */
	for (i32Rwd=valid_idx-1; i32Rwd >= 0; i32Rwd--)
	{
		IMG_UINT32 i;

		psSg = psOSCleanupData->ppsSgSparse[psOSCleanupData->uiCount][i32Rwd];
		dma_unmap_sg(psDevConfig->pvOSDevice, psSg->sgl, psSg->nents, eDataDirection);
		sg_free_table(psSg);

		/* Unpin pages */
		for (i=0; i < psOSCleanupData->puiNumPages[psOSCleanupData->uiCount]*2; i++)
		{
			if (psOSCleanupData->pages[psOSCleanupData->uiCount][i])
			{
				pvr_unpin_user_page_for_dma(psOSCleanupData->pages[psOSCleanupData->uiCount][i]);
			}
		}
	}
	OSFreeMem(psOSCleanupData->ppsDescriptorsSparse[psOSCleanupData->uiCount]);
e11:
	OSFreeMem(psOSCleanupData->ppsSgSparse[psOSCleanupData->uiCount]);
e1:
	OSFreeMem(psOSCleanupData->pages[psOSCleanupData->uiCount]);
e0:
	return eError;
}

#endif /* SUPPORT_DMA_TRANSFER */

#if defined(SUPPORT_SECURE_ALLOC_KM)
#if defined(PVR_ANDROID_HAS_DMA_HEAP_FIND)
IMG_INTERNAL PVRSRV_ERROR
OSAllocateSecBuf(PVRSRV_DEVICE_NODE *psDeviceNode,
				 IMG_DEVMEM_SIZE_T uiSize,
				 const IMG_CHAR *pszName,
				 PMR **ppsPMR)
{
	struct dma_heap *heap;
	struct dma_buf *buf;
	struct device *dev;
	struct dma_buf_attachment *buf_attachment;

	IMG_UINT32 ui32MappingTable = 0;
	PVRSRV_ERROR eError;
	IMG_CHAR *pszHeapName;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psDeviceNode, "psDeviceNode");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psDeviceNode->psDevConfig->pszSecureDMAHeapName, "pszSecureDMAHeapName");
	PVR_LOG_RETURN_IF_INVALID_PARAM((OSStringLength(psDeviceNode->psDevConfig->pszSecureDMAHeapName) > 0), "pszSecureDMAHeapName length");

	pszHeapName = psDeviceNode->psDevConfig->pszSecureDMAHeapName;
	dev = (struct device*)psDeviceNode->psDevConfig->pvOSDevice;

	heap = dma_heap_find(pszHeapName);
	PVR_LOG_GOTO_IF_NOMEM(heap, eError, ErrorExit);

	buf = dma_heap_buffer_alloc(heap, uiSize, 0, 0);
	PVR_LOG_GOTO_IF_NOMEM(buf, eError, ErrorBufPut);

	if (buf->size < uiSize)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: buffer size (%ld) is less than requested (%lld).",
				 __func__, buf->size, uiSize));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorBufFree;
	}

	buf_attachment = dma_buf_attach(buf, dev);
	PVR_LOG_GOTO_IF_NOMEM(buf_attachment, eError, ErrorBufFree);

	eError = PhysmemCreateNewDmaBufBackedPMR(psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_EXTERNAL],
											 buf_attachment,
											 NULL,
											 PVRSRV_MEMALLOCFLAG_GPU_READABLE
											 | PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE,
											 buf->size,
											 1,
											 1,
											 &ui32MappingTable,
											 OSStringLength(pszName),
											 pszName,
											 ppsPMR);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysmemCreateNewDmaBufBackedPMR", ErrorBufDetach);

	return PVRSRV_OK;

ErrorBufDetach:
	dma_buf_detach(buf, buf_attachment);
ErrorBufFree:
	dma_heap_buffer_free(buf);
ErrorBufPut:
	dma_buf_put(buf);
ErrorExit:

	return eError;
}

IMG_INTERNAL void
OSFreeSecBuf(PMR *psPMR)
{
	struct dma_buf *buf = PhysmemGetDmaBuf(psPMR);
	dma_buf_put(buf);
	dma_heap_buffer_free(buf);

	PMRUnrefPMR(psPMR);
}
#else /* PVR_ANDROID_HAS_DMA_HEAP_FIND */
IMG_INTERNAL PVRSRV_ERROR
OSAllocateSecBuf(PVRSRV_DEVICE_NODE *psDeviceNode,
				 IMG_DEVMEM_SIZE_T uiSize,
				 const IMG_CHAR *pszName,
				 PMR **ppsPMR)
{
	IMG_UINT32 ui32MappingTable = 0;
	PVRSRV_ERROR eError;

	eError = PhysmemNewRamBackedPMR(NULL,
									psDeviceNode,
									uiSize,
									1,
									1,
									&ui32MappingTable,
									ExactLog2(OSGetPageSize()),
									PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(GPU_SECURE)
									| PVRSRV_MEMALLOCFLAG_GPU_READABLE
									| PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE,
									OSStringLength(pszName),
									pszName,
									OSGetCurrentClientProcessIDKM(),
									ppsPMR,
									PDUMP_NONE,
									NULL);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysmemNewRamBackedPMR", ErrorExit);

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
	eError = RIWritePMREntryWithOwnerKM(*ppsPMR, PVR_SYS_ALLOC_PID);
	PVR_LOG_GOTO_IF_ERROR(eError, "RIWritePMREntryWithOwnerKM", ErrorUnrefPMR);
#endif

	return PVRSRV_OK;

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
ErrorUnrefPMR:
	PMRUnrefPMR(*ppsPMR);
#endif
ErrorExit:
	return eError;
}

IMG_INTERNAL void
OSFreeSecBuf(PMR *psPMR)
{
	PMRUnrefPMR(psPMR);
}
#endif
#endif /* SUPPORT_SECURE_ALLOC_KM */
