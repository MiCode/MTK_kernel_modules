/*************************************************************************/ /*!
@File           physmem_dmabuf_fbc_tracker.c
@Title          dmabuf memory fbc tracker
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of the memory management. This module is responsible for
                implementing the function callbacks for dmabuf memory.
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

#include <linux/spinlock.h>
#include <linux/rbtree.h>
#include <linux/dma-buf.h>

#include "physmem_dmabuf.h"
#include "lock.h"

#if !defined(RGX_FBC_MAX_DESCRIPTORS)
#if defined(RGX_FEATURE_FBC_MAX_DEFAULT_DESCRIPTORS)
#define RGX_FBC_MAX_DESCRIPTORS (RGX_FEATURE_FBC_MAX_DEFAULT_DESCRIPTORS)
#else
#define RGX_FBC_MAX_DESCRIPTORS (0)
#endif
#endif

#if RGX_FBC_MAX_DESCRIPTORS > 0

static atomic_t gInitialized = ATOMIC_INIT(0);

struct pvr_dmabuf {
	struct rb_node sNode;
	const struct dma_buf *psDmaBuf;
};

static struct  pvr_dmabuf_allocator_tracker {
	struct rb_root sRoot;
	spinlock_t     sLock;
	IMG_PID        uiAllocatorServicePID;
} gAllocDmaBufTracker;

static struct pvr_dmabuf_allocator_tracker *getAllocatorDmaBufTracker(void)
{
	struct pvr_dmabuf_allocator_tracker *psTracker = &gAllocDmaBufTracker;

	if (atomic_cmpxchg(&gInitialized, 0, 1) == 0)
	{
		psTracker->sRoot = RB_ROOT;
		spin_lock_init(&psTracker->sLock);
		psTracker->uiAllocatorServicePID = OSGetCurrentClientProcessIDKM();
		PVR_DPF((PVR_DBG_VERBOSE, "Allocator PID:%d",
					psTracker->uiAllocatorServicePID));
	}

	spin_lock(&psTracker->sLock);

	return psTracker;
}

static void putAllocatorDmaBufTracker(
		struct pvr_dmabuf_allocator_tracker *psTracker)
{
	spin_unlock(&psTracker->sLock);
}

static struct pvr_dmabuf *pvr_dmabuf_search_locked(
		const struct pvr_dmabuf_allocator_tracker *psTracker,
		const struct dma_buf *psDmaBuf)
{
	struct rb_node *psNode = psTracker->sRoot.rb_node;
	struct pvr_dmabuf *psPVRDmaBuf = NULL;
	IMG_BOOL bIsFound = IMG_FALSE;

	while (psNode)
	{
		psPVRDmaBuf = rb_entry(psNode, struct pvr_dmabuf, sNode);
		if ((uintptr_t)psPVRDmaBuf->psDmaBuf < (uintptr_t)psDmaBuf)
		{
			psNode = psNode->rb_left;
		}
		else if ((uintptr_t)psPVRDmaBuf->psDmaBuf > (uintptr_t)psDmaBuf)
		{
			psNode = psNode->rb_right;
		}
		else
			break;
	}

	if (psPVRDmaBuf && psPVRDmaBuf->psDmaBuf == psDmaBuf)
	{
		bIsFound = IMG_TRUE;
	}

	return bIsFound ? psPVRDmaBuf : NULL;
}

static IMG_BOOL pvr_dmabuf_allocator_tracker_search(struct dma_buf *psDmaBuf)
{
	struct pvr_dmabuf_allocator_tracker *psTracker =
		getAllocatorDmaBufTracker();
	struct pvr_dmabuf *psPVRDmaBuf;

	psPVRDmaBuf = pvr_dmabuf_search_locked(psTracker, psDmaBuf);

	putAllocatorDmaBufTracker(psTracker);

	return psPVRDmaBuf != NULL;
}

static void pvr_dmabuf_allocator_tracker_add(struct dma_buf *psDmaBuf)
{
	struct pvr_dmabuf_allocator_tracker *psTracker =
		getAllocatorDmaBufTracker();
	struct rb_node **ppsNewNode, *psParent = NULL;
	struct pvr_dmabuf *psPVRDmaBuf;

	/* If the client is not Allocator Service, do not add this DMA buffer
	 * to the tracker.
	 */
	if (OSGetCurrentClientProcessIDKM() != psTracker->uiAllocatorServicePID)
	{
		goto tracker_put;
	}

	/* If the DMA buffer is already tracked, return with success. */
	if (pvr_dmabuf_search_locked(psTracker, psDmaBuf) != NULL)
	{
		goto tracker_put;
	}

	psPVRDmaBuf = OSAllocZMem(sizeof(*psPVRDmaBuf));
	PVR_LOG_GOTO_IF_FALSE(psPVRDmaBuf != NULL, "OSAllocZMem", tracker_put);
	psPVRDmaBuf->psDmaBuf = psDmaBuf;

	ppsNewNode = &psTracker->sRoot.rb_node;

	while (*ppsNewNode)
	{
		struct pvr_dmabuf *psCurDmaBuf = rb_entry(*ppsNewNode,
				struct pvr_dmabuf, sNode);
		ptrdiff_t diff = (uintptr_t)(psPVRDmaBuf->psDmaBuf) -
			(uintptr_t)(psCurDmaBuf->psDmaBuf);

		psParent = *ppsNewNode;

		if (diff < 0)
		{
			ppsNewNode = &((*ppsNewNode)->rb_left);
		}
		else if (diff > 0)
		{
			ppsNewNode = &((*ppsNewNode)->rb_right);
		}
		else
		{
			PVR_ASSERT(0);
		}
	}

	rb_link_node(&psPVRDmaBuf->sNode, psParent, ppsNewNode);
	rb_insert_color(&psPVRDmaBuf->sNode, &psTracker->sRoot);

tracker_put:
	putAllocatorDmaBufTracker(psTracker);
}

static IMG_BOOL pvr_dmabuf_allocator_tracker_remove(
		const struct dma_buf *psDmaBuf)
{
	struct pvr_dmabuf_allocator_tracker *psTracker =
		getAllocatorDmaBufTracker();
	struct pvr_dmabuf *psPVRDmaBuf = NULL;
	IMG_BOOL bHasRemoved = IMG_FALSE;

	PVR_LOG_GOTO_IF_FALSE(psDmaBuf != NULL, "dma_buf is invalid", tracker_put);

	/* Gralloc will import the DMA buffer in Services but it will be released
	 * shortly. After the binder transition, the DMA buffer will be imported
	 * again in the client processes. Therefore, the deletion operation in the
	 * Allocator server process should be ignored. The non-Allocator is the
	 * final indication that the buffer is no longer needed.
	 */
	if (OSGetCurrentClientProcessIDKM() == psTracker->uiAllocatorServicePID)
	{
		goto tracker_put;
	}

	psPVRDmaBuf = pvr_dmabuf_search_locked(psTracker, psDmaBuf);
	if (psPVRDmaBuf)
	{
		rb_erase(&psPVRDmaBuf->sNode, &psTracker->sRoot);
		OSFreeMem(psPVRDmaBuf);
		bHasRemoved = IMG_TRUE;
	}

tracker_put:
	putAllocatorDmaBufTracker(psTracker);

	return bHasRemoved;
}

#endif /* RGX_FBC_MAX_DESCRIPTORS > 0 */

PVRSRV_ERROR
PhysmemRequestFBC(CONNECTION_DATA *psConnection, PVRSRV_DEVICE_NODE *psDevNode,
                  int fd)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
#if RGX_FBC_MAX_DESCRIPTORS > 0
	struct dma_buf *psDmaBuf;
	IMG_INT32 i32Counter;

	psDmaBuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(psDmaBuf))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get dma-buf from fd (err=%ld)",
		                        __func__, psDmaBuf ?
		                        PTR_ERR(psDmaBuf) : -ENOMEM));
		return PVRSRV_ERROR_BAD_MAPPING;
	}

	/* If the dma-buf has already claimed a FBC surface, return with success. */
	if (pvr_dmabuf_allocator_tracker_search(psDmaBuf) == IMG_TRUE)
	{
		dma_buf_put(psDmaBuf);

		return PVRSRV_OK;
	}

	i32Counter = OSAtomicIncrement(&psDevNode->iFBCSurfaceCount);
	if (i32Counter >= RGX_FBC_MAX_DESCRIPTORS)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: The fbc slots are out of memory max: %d",
					__func__, RGX_FBC_MAX_DESCRIPTORS));

		OSAtomicDecrement(&psDevNode->iFBCSurfaceCount);
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	if (eError == PVRSRV_OK)
	{
		pvr_dmabuf_allocator_tracker_add(psDmaBuf);
	}
	else
	{
		dma_buf_put(psDmaBuf);
	}
#endif

	return eError;
}

PVRSRV_ERROR
PhysmemFreeFBC(CONNECTION_DATA *psConnection, PVRSRV_DEVICE_NODE *psDevNode,
		struct dma_buf *psDmaBuf)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
#if RGX_FBC_MAX_DESCRIPTORS > 0
	if (pvr_dmabuf_allocator_tracker_remove(psDmaBuf) == IMG_FALSE)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: dma-buf %p was not claimed a FBC surface",
					__func__, psDmaBuf));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	else
	{
		/* Dereference the DMA buffer when adding it to the tree. */
		dma_buf_put(psDmaBuf);
	}

	OSAtomicDecrement(&psDevNode->iFBCSurfaceCount);
#endif

	return eError;
}
