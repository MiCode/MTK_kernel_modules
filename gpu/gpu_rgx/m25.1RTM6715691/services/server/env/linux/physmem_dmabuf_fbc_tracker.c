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

#include <linux/rbtree.h>
#include <linux/dma-buf.h>
#include <linux/hashtable.h>
#include <linux/sched/mm.h>

#include "physmem_dmabuf.h"

#if !defined(RGX_FBC_MAX_DESCRIPTORS)
#if defined(RGX_FEATURE_FBC_MAX_DEFAULT_DESCRIPTORS)
#define RGX_FBC_MAX_DESCRIPTORS (RGX_FEATURE_FBC_MAX_DEFAULT_DESCRIPTORS)
#else
#define RGX_FBC_MAX_DESCRIPTORS (0)
#endif
#endif

#if RGX_FBC_MAX_DESCRIPTORS > 0

#define CMDLINE_BUF_SIZE (256)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
#define MMAP_LOCK(mm) (&(mm)->mmap_lock)
#else
#define MMAP_LOCK(mm) (&(mm)->mmap_sem)
#endif

static DEFINE_MUTEX(g_sTrackerMutex);
static DEFINE_HASHTABLE(g_sDevNodeTable, 4);

static IMG_BOOL g_bInitialized = IMG_FALSE;

struct pvr_dev_node_entry {
	uintptr_t         vaddr;
	int               iFBCSurfaceCount;
	struct hlist_node sNode;
};

struct pvr_dmabuf {
	struct rb_node       sNode;
	const struct dma_buf *psDmaBuf;
};

static struct pvr_dmabuf_allocator_tracker {
	struct rb_root sRoot;
	IMG_PID        uiAllocatorServicePID;
} g_AllocDmaBufTracker;

#if defined(ANDROID)
/* On Android, system services are implemented as Binder-based processes that
 * spawn multiple worker threads. Each thread can independently set its name
 * that updates the kernel-level 'task->comm' field. As a result, many
 * threads—including the main thread—may appear with names like
 * 'binder:<pid>_<index>', rather than the actual service name.
 *
 * Therefore, 'task->comm' is not a reliable way to identify the true identity
 * of a service process in Android. To address this, the function instead
 * resolves the process identity by inspecting its command-line arguments
 * (cmdline), which accurately reflect the original executable and its intent.
 */
static void _get_process_name(IMG_CHAR *pszBuffer, size_t uiBufferLen)
{
	IMG_CHAR szCmdLine[CMDLINE_BUF_SIZE] = {0};
	struct DIR_STRING { const char *const pszString; size_t uiLen; };
	static struct DIR_STRING apszDirs[] =
	{
#define X(str) { (str), sizeof(str)-1 }
		X("/system/bin/hw/"),
		X("/vendor/bin/hw/"),
		X("/system/bin/"),
		X("/vendor/bin/"),
#undef X
	};
	unsigned long ulStart, ulEnd;
	IMG_CHAR *pszProcessName;
	struct mm_struct *psMM;
	size_t uiLen = 0, i;

	psMM = get_task_mm(current);
	PVR_LOG_GOTO_IF_FALSE(psMM != NULL, "get_task_mm", use_fallback);

	down_read(MMAP_LOCK(psMM));

	ulStart = psMM->arg_start;
	ulEnd   = psMM->arg_end;
	PVR_LOG_GOTO_IF_FALSE(ulEnd >= ulStart, "ulStart > ulEnd",
		unlock_and_use_fallback);

	uiLen = min_t(size_t, ARRAY_SIZE(szCmdLine) - 1, ulEnd - ulStart);
	PVR_LOG_GOTO_IF_ERROR(OSCopyFromUser(NULL, szCmdLine,
		(const void __user *)ulStart, uiLen), "OSCopyFromUser",
		unlock_and_use_fallback);

	/* Append null-terminator explicitly since OSCopyFromUser doesn't
	 * guarantee it
	 */
	szCmdLine[uiLen] = '\0';

	up_read(MMAP_LOCK(psMM));
	mmput(psMM);

	PVR_LOG_GOTO_IF_FALSE(OSStringLength(szCmdLine) != 0, "OSStringLength",
		use_fallback);
	pszProcessName = szCmdLine;
	for (i = 0; i < ARRAY_SIZE(apszDirs); i++)
	{
		size_t ui32DirLen = apszDirs[i].uiLen;
		if (OSStringNCompare(pszProcessName, apszDirs[i].pszString,
			ui32DirLen) == 0)
		{
			pszProcessName += ui32DirLen;
			break;
		}
	}

	OSStringSafeCopy(pszBuffer, pszProcessName, uiBufferLen);
	return;

unlock_and_use_fallback:
	up_read(MMAP_LOCK(psMM));
	mmput(psMM);

use_fallback:
	OSStringSafeCopy(pszBuffer, OSGetCurrentProcessName(), uiBufferLen);
}

static IMG_BOOL _is_allocator_process(void)
{
	const char *pszServiceName = "android.hardware.graphics.allocator";
	IMG_CHAR szProcessName[CMDLINE_BUF_SIZE];

	_get_process_name(szProcessName, CMDLINE_BUF_SIZE);
	/* Valid process names should contain the substring allocator, such as
	 * android.hardware.graphics.allocator-service or
	 * android.hardware.graphics.allocator@2.0-service for HIDL.
	 */
	if (strnstr(szProcessName, pszServiceName, OSStringLength(szProcessName)))
	{
		PVR_DPF((PVR_DBG_VERBOSE, "Found allocator service: %s", szProcessName));
		return IMG_TRUE;
	}

	return IMG_FALSE;
}
#endif

static struct pvr_dmabuf_allocator_tracker *_get_tracker(void)
{
	struct pvr_dmabuf_allocator_tracker *psTracker = &g_AllocDmaBufTracker;

	mutex_lock(&g_sTrackerMutex);

	if (g_bInitialized == IMG_TRUE)
	{
		return &g_AllocDmaBufTracker;
	}

#if defined(ANDROID)
	if (_is_allocator_process() == IMG_FALSE)
	{
		PVR_DPF((PVR_DBG_VERBOSE, "Allocator service unavailable. "
			"Possible gralloc issue."));
		goto err_out;
	}
#endif

	psTracker->uiAllocatorServicePID = OSGetCurrentClientProcessIDKM();
	PVR_DPF((PVR_DBG_MESSAGE, "Allocator PID:%d",
		psTracker->uiAllocatorServicePID));
	psTracker->sRoot = RB_ROOT;
	g_bInitialized = IMG_TRUE;

	return psTracker;

err_out:
	mutex_unlock(&g_sTrackerMutex);
	return NULL;
}

static void _put_tracker(void)
{
	mutex_unlock(&g_sTrackerMutex);
}

static int _get_fbc_surface_count_locked(PVRSRV_DEVICE_NODE *psDevNode)
{
	uintptr_t vaddr = (uintptr_t)psDevNode;
	struct pvr_dev_node_entry *psEntry;

	hash_for_each_possible(g_sDevNodeTable, psEntry, sNode, vaddr)
	{
		if (psEntry->vaddr == vaddr)
		{
			return psEntry->iFBCSurfaceCount;
		}
	}

	return 0;
}

static PVRSRV_ERROR _increase_fbc_surface_count_locked(
	PVRSRV_DEVICE_NODE *psDevNode)
{
	uintptr_t vaddr = (uintptr_t)psDevNode;
	struct pvr_dev_node_entry *psEntry;

	hash_for_each_possible(g_sDevNodeTable, psEntry, sNode, vaddr)
	{
		if (psEntry->vaddr == vaddr)
		{
			psEntry->iFBCSurfaceCount++;
			PVR_DPF((PVR_DBG_VERBOSE, "%s: (+): ref=%d", __func__,
				psEntry->iFBCSurfaceCount));

			return PVRSRV_OK;
		}
	}

	psEntry = OSAllocZMem(sizeof(*psEntry));
	PVR_LOG_RETURN_IF_NOMEM(psEntry, "OSAllocZMem");

	psEntry->vaddr = vaddr;
	psEntry->iFBCSurfaceCount = 1;
	hash_add(g_sDevNodeTable, &psEntry->sNode, vaddr);

	return PVRSRV_OK;
}

static PVRSRV_ERROR _decrease_fbc_surface_count_locked(PVRSRV_DEVICE_NODE *psDevNode)
{
	uintptr_t vaddr = (uintptr_t)psDevNode;
	struct pvr_dev_node_entry *psEntry;
	struct hlist_node *tmp;

	hash_for_each_possible_safe(g_sDevNodeTable, psEntry, tmp, sNode, vaddr)
	{
		if (psEntry->vaddr == vaddr)
		{
			psEntry->iFBCSurfaceCount--;
			PVR_DPF((PVR_DBG_VERBOSE, "%s: (-): ref=%d", __func__,
				psEntry->iFBCSurfaceCount));

			if (psEntry->iFBCSurfaceCount < 0)
			{
				PVR_DPF((PVR_DBG_ERROR, "FBC count underflow for node %p",
					psDevNode));
				return PVRSRV_ERROR_INVALID_PARAMS;
			}

			if (psEntry->iFBCSurfaceCount == 0)
			{
				hash_del(&psEntry->sNode);
				OSFreeMem(psEntry);
			}
			return PVRSRV_OK;
		}
	}

	PVR_DPF((PVR_DBG_ERROR, "Attempt to decrement FBC count for unknown "
		"device node"));
	return PVRSRV_ERROR_INVALID_PARAMS;
}

static struct pvr_dmabuf *_tracker_search_locked(
	const struct pvr_dmabuf_allocator_tracker *psTracker,
	const struct dma_buf *psDmaBuf)
{
	struct rb_node *psNode = psTracker->sRoot.rb_node;

	while (psNode)
	{
		struct pvr_dmabuf *psEntry = rb_entry(psNode, struct pvr_dmabuf, sNode);

		if ((uintptr_t)psEntry->psDmaBuf < (uintptr_t)psDmaBuf)
		{
			psNode = psNode->rb_right;
		}
		else if ((uintptr_t)psEntry->psDmaBuf > (uintptr_t)psDmaBuf)
		{
			psNode = psNode->rb_left;
		}
		else
		{
			return psEntry;
		}
	}

	return NULL;
}

static PVRSRV_ERROR _tracker_add_locked(PVRSRV_DEVICE_NODE *psDevNode,
	struct pvr_dmabuf_allocator_tracker *psTracker,
	struct dma_buf *psDmaBuf)
{
	struct rb_node **ppsLink = &psTracker->sRoot.rb_node, *psParent = NULL;
	struct pvr_dmabuf *psNewEntry;
	PVRSRV_ERROR eError;

	psNewEntry = OSAllocZMem(sizeof(*psNewEntry));
	PVR_LOG_RETURN_IF_NOMEM(psNewEntry, "OSAllocZMem");

	psNewEntry->psDmaBuf = psDmaBuf;

	while (*ppsLink) {
		struct pvr_dmabuf *psEntry = rb_entry(*ppsLink, struct pvr_dmabuf, sNode);
		psParent = *ppsLink;

		if ((uintptr_t)psNewEntry->psDmaBuf < (uintptr_t)psEntry->psDmaBuf)
		{
			ppsLink = &(*ppsLink)->rb_left;
		}
		else if ((uintptr_t)psNewEntry->psDmaBuf > (uintptr_t)psEntry->psDmaBuf)
		{
			ppsLink = &(*ppsLink)->rb_right;
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Seeing a duplicate dmabuf", __func__));
			OSFreeMem(psNewEntry);

			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}

	eError = _increase_fbc_surface_count_locked(psDevNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to increase FBC surface count",
			__func__));
		OSFreeMem(psNewEntry);

		return eError;
	}

	rb_link_node(&psNewEntry->sNode, psParent, ppsLink);
	rb_insert_color(&psNewEntry->sNode, &psTracker->sRoot);

	return PVRSRV_OK;
}

static PVRSRV_ERROR _tracker_remove_locked(PVRSRV_DEVICE_NODE *psDevNode,
	struct pvr_dmabuf_allocator_tracker *psTracker,
	const struct dma_buf *psDmaBuf)
{
	struct pvr_dmabuf *psEntry = _tracker_search_locked(psTracker, psDmaBuf);
	PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;

	if (psEntry)
	{
		rb_erase(&psEntry->sNode, &psTracker->sRoot);
		OSFreeMem(psEntry);
		eError = _decrease_fbc_surface_count_locked(psDevNode);
	}

	return eError;
}

static IMG_BOOL _is_allocator_service(IMG_PID pid)
{
	return (OSGetCurrentClientProcessIDKM() == pid) ? IMG_TRUE : IMG_FALSE;
}

#endif /* RGX_FBC_MAX_DESCRIPTORS > 0 */

PVRSRV_ERROR
PhysmemRequestFBC(CONNECTION_DATA *psConnection, PVRSRV_DEVICE_NODE *psDevNode,
	IMG_INT iFd)
{
#if RGX_FBC_MAX_DESCRIPTORS > 0
	struct pvr_dmabuf_allocator_tracker *psTracker;
	IMG_BOOL bAdded = IMG_FALSE;
	struct dma_buf *psDmaBuf;
#endif
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_LOG_RETURN_IF_NOMEM(psConnection, "PhysmemRequestFBC.1");
	PVR_LOG_RETURN_IF_NOMEM(psDevNode, "PhysmemRequestFBC.2");

#if RGX_FBC_MAX_DESCRIPTORS > 0
	psTracker = _get_tracker();
	if (!psTracker)
	{
		PVR_DPF((PVR_DBG_VERBOSE, "%s: Cannot initialize counter because the "
			"allocator process has not yet imported any buffers", __func__));

		/* This issue may occur if the allocator process has not yet imported
		 * any buffers. Typically, using the first process to import a dmabuf
		 * avoids failure. However, some customized gralloc implementations
		 * may not integrate this feature correctly, potentially causing
		 * problems. As a result, to reduce noise and prevent unnecessary
		 * errors, return success in this case but log an error message to
		 * indicate that gralloc should address this integration issue.
		 */
		return PVRSRV_OK;
	}

	psDmaBuf = dma_buf_get(iFd);
	PVR_LOG_GOTO_IF_NOMEM(psDmaBuf, eError, err_put);

	if (_get_fbc_surface_count_locked(psDevNode) >= RGX_FBC_MAX_DESCRIPTORS)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: The fbc slots are out of memory max: %d",
			__func__, RGX_FBC_MAX_DESCRIPTORS));

		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto err_dmabuf_put;
	}

	if ((_is_allocator_service(psTracker->uiAllocatorServicePID) == IMG_TRUE) &&
		!_tracker_search_locked(psTracker, psDmaBuf))
	{
		if (_tracker_add_locked(psDevNode, psTracker, psDmaBuf) == PVRSRV_OK)
		{
			bAdded = IMG_TRUE;
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to add dmabuf in tracker",
				__func__));
			eError = PVRSRV_ERROR_INVALID_PARAMS;
		}
	}

err_dmabuf_put:
	if (bAdded == IMG_FALSE)
	{
		dma_buf_put(psDmaBuf);
	}
err_put:
	_put_tracker();
#else
	PVR_UNREFERENCED_PARAMETER(iFd);
#endif

	return eError;
}

void
PhysmemFreeFBC(CONNECTION_DATA *psConnection, PVRSRV_DEVICE_NODE *psDevNode,
		struct dma_buf *psDmaBuf)
{
#if RGX_FBC_MAX_DESCRIPTORS > 0
	struct pvr_dmabuf_allocator_tracker *psTracker;
#endif

	PVR_LOG_RETURN_VOID_IF_FALSE(psDevNode != NULL, "PhysmemFreeFBC.1");
	PVR_LOG_RETURN_VOID_IF_FALSE(psDmaBuf != NULL, "PhysmemFreeFBC.2");
	PVR_UNREFERENCED_PARAMETER(psConnection);

#if RGX_FBC_MAX_DESCRIPTORS > 0
	psTracker = _get_tracker();
	if (!psTracker)
	{
		return;
	}

	/* Gralloc will import the DMA buffer in Services but it will be released
	 * shortly. After the binder transition, the DMA buffer will be imported
	 * again in the client processes. Therefore, the deletion operation in the
	 * Allocator server process should be ignored. The non-Allocator is the
	 * final indication that the buffer is no longer needed.
	 */
	if (_is_allocator_service(psTracker->uiAllocatorServicePID) != IMG_TRUE)
	{
		if (_tracker_remove_locked(psDevNode, psTracker, psDmaBuf) == PVRSRV_OK)
		{
			dma_buf_put(psDmaBuf);
		}
	}

	_put_tracker();
#endif
}
