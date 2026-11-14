/*************************************************************************/ /*!
@File
@Title          Transport Layer kernel side API implementation.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Transport Layer functions available to driver components in
                the driver.
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
// #define PVR_DPF_FUNCTION_TRACE_ON 1
#undef PVR_DPF_FUNCTION_TRACE_ON
#include "pvr_debug.h"

#include "allocmem.h"
#include "pvrsrv_error.h"
#include "osfunc.h"
#include "dllist.h"
#include "devicemem.h"

#include "pvrsrv_tlcommon.h"
#include "tlintern.h"
#if defined(__KERNEL__)
#include "tlstream.h"
#endif

/*
 * Make functions
 */
PTL_STREAM_DESC
TLMakeStreamDesc(PTL_SNODE f1, IMG_UINT32 f2, IMG_HANDLE f3)
{
	PTL_STREAM_DESC ps = OSAllocZMem(sizeof(TL_STREAM_DESC));
	if (ps == NULL)
	{
		return NULL;
	}
	ps->psNode = f1;
	ps->ui32Flags = f2;
	ps->hReadEvent = f3;
	ps->uiRefCount = 1;

	if (f2 & PVRSRV_STREAM_FLAG_READ_LIMIT)
	{
		ps->ui32ReadLimit = f1->psStream->ui32Write;
	}
	return ps;
}

PTL_SNODE
TLMakeSNode(IMG_HANDLE f2, TL_STREAM *f3, TL_STREAM_DESC *f4)
{
	PTL_SNODE ps = OSAllocZMem(sizeof(TL_SNODE));
	if (ps == NULL)
	{
		return NULL;
	}
	ps->hReadEventObj = f2;
	ps->psStream = f3;
	ps->psRDesc = f4;
	f3->psNode = ps;
	return ps;
}

/*
 * Transport Layer Global top variables and functions
 */
static TL_GLOBAL_DATA sTLGlobalData;

TL_GLOBAL_DATA *TLGGD(void) /* TLGetGlobalData() */
{
	return &sTLGlobalData;
}

/* TLInit must only be called once at driver initialisation.
 * An assert is provided to check this condition on debug builds.
 */
PVRSRV_ERROR
TLInit(void)
{
	PVRSRV_ERROR eError;

	PVR_DPF_ENTERED;

	PVR_ASSERT(sTLGlobalData.hTLGDLock == NULL && sTLGlobalData.hTLEventObj == NULL);

	dllist_init(&sTLGlobalData.sNodeListHead);

	/* Allocate a lock for TL global data, to be used while updating the TL data.
	 * This is for making TL global data multi-thread safe */
	eError = OSLockCreate(&sTLGlobalData.hTLGDLock);
	PVR_GOTO_IF_ERROR(eError, e0);

	/* Allocate the event object used to signal global TL events such as
	 * a new stream created */
	eError = OSEventObjectCreate("TLGlobalEventObj", &sTLGlobalData.hTLEventObj);
	PVR_GOTO_IF_ERROR(eError, e1);

	PVR_DPF_RETURN_OK;

/* Don't allow the driver to start up on error */
e1:
	OSLockDestroy (sTLGlobalData.hTLGDLock);
	sTLGlobalData.hTLGDLock = NULL;
e0:
	PVR_DPF_RETURN_RC (eError);
}

static void RemoveAndFreeStreamNode(PTL_SNODE psRemove)
{
	TL_GLOBAL_DATA*  psGD = TLGGD();
	PVRSRV_ERROR     eError;
	IMG_CHAR         aszName[PVRSRVTL_MAX_STREAM_NAME_SIZE];
	IMG_BOOL         bHadRDesc = IMG_FALSE;
	IMG_BOOL         bHadStream = IMG_FALSE;

	PVR_UNREFERENCED_PARAMETER(bHadRDesc);

	PVR_DPF_ENTERED;

	/* Unlink the stream node from the master list */
	PVR_ASSERT(!dllist_is_empty(&psGD->sNodeListHead));
	OSLockHeldAssert(psGD->hTLGDLock);

	dllist_remove_node(&psRemove->sNodeList);
	if (psRemove->psRDesc)
	{
		bHadRDesc = IMG_TRUE;
		OSFreeMem(psRemove->psRDesc);
		psRemove->psRDesc = NULL;
	}
	if (psRemove->psStream)
	{
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
		PVR_DPF((PVR_DBG_WARNING, "%s: Freeing '%s'", __func__,
		         psRemove->psStream->szName));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */
		OSStringSafeCopy(aszName, psRemove->psStream->szName,
		                 PVRSRVTL_MAX_STREAM_NAME_SIZE);
		OSFreeMem(psRemove->psStream);
		psRemove->psStream = NULL;
		bHadStream = IMG_TRUE;
	}

	/* Release the event list object owned by the stream node */
	if (psRemove->hReadEventObj && bHadStream)
	{
		eError = OSEventObjectDestroy(psRemove->hReadEventObj);
		PVR_LOG_IF_ERROR(eError, "OSEventObjectDestroy");
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: hReadEventObj = %p, RDesc = '%s'",
			         __func__, psRemove->hReadEventObj,
			         bHadRDesc ? "True" : "False"));
			PVR_DPF((PVR_DBG_WARNING, "%s: Stream Name = <%s>", __func__,
			         aszName));

		}
		psRemove->hReadEventObj = NULL;
	}

	/* Release the memory of the stream node */
	OSFreeMem(psRemove);

	PVR_DPF_RETURN;
}

static void FreeGlobalData(void)
{
	PVRSRV_ERROR eError;
	PDLLIST_NODE psNode, psNext;

	TL_GLOBAL_DATA*  psGD = TLGGD();

	PVR_DPF_ENTERED;

	/* Clean up the SNODE list */
	dllist_foreach_node(&psGD->sNodeListHead, psNode, psNext)
	{
		TL_SNODE *psTLSNode = IMG_CONTAINER_OF(psNode, TL_SNODE, sNodeList);

		dllist_remove_node(psNode);
		psGD->uiClientCnt--;

#if defined(PVR_DPF_FUNCTION_TRACE_ON)
		PVR_DPF((PVR_DBG_WARNING, "%s: Clearing out node @ %p, Stream '%s'",
		         __func__, psTLSNode, psTLSNode->psStream ?
		         psTLSNode->psStream->szName : "Unknown!!"));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */

		if (psTLSNode->psRDesc)
		{
			OSFreeMem(psTLSNode->psRDesc);
			psTLSNode->psRDesc = NULL;
		}
		if (psTLSNode->psStream)
		{
			OSFreeMem(psTLSNode->psStream);
			psTLSNode->psStream = NULL;
		}

		/* Release the event list object owned by the stream node */
		if (psTLSNode->hReadEventObj)
		{
			eError = OSEventObjectDestroy(psTLSNode->hReadEventObj);
			PVR_LOG_IF_ERROR(eError, "OSEventObjectDestroy");

			psTLSNode->hReadEventObj = NULL;
		}

		OSFreeMem(psTLSNode);

	}

	PVR_DPF_RETURN;
}

void
TLDeInit(void)
{
	DLLIST_NODE *psNode, *psNext;

	PVR_DPF_ENTERED;

	if (sTLGlobalData.uiClientCnt)
	{
		PVR_DPF((PVR_DBG_ERROR, "TLDeInit transport layer but %d client streams are still connected", sTLGlobalData.uiClientCnt));
		sTLGlobalData.uiClientCnt = 0;
	}

	if (!dllist_is_empty(&sTLGlobalData.sNodeListHead))
	{
		if (!TL_DEFERRED_FREE_COUNT(sTLGlobalData.uiTLDeferredFrees))
		{
			PVR_DPF((PVR_DBG_ERROR,
		         "TLDeInit transport layer - resources still allocated"));
		}
		dllist_foreach_node(&sTLGlobalData.sNodeListHead, psNode, psNext)
		{
			TL_SNODE *psTLNode = IMG_CONTAINER_OF(
			    psNode, TL_SNODE, sNodeList);

			if (psTLNode->psStream)
			{
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
				PVR_DPF((PVR_DBG_WARNING,
				         "Decoupling '%s'", psTLNode->psStream->szName));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */

#if defined(__KERNEL__)
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
				PVR_DPF((PVR_DBG_WARNING, "%s: Attempting to close '%s'",
				         __func__, psTLNode->psStream->szName));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */
				TLStreamClose((IMG_HANDLE)(psTLNode->psStream));
#else
				dllist_remove_node(psNode);
				OSFreeMem(psTLNode);
#endif
			}
			else
			{
				dllist_remove_node(psNode);
				OSFreeMem(psTLNode);
			}
		}
	}

	FreeGlobalData();

	/* Clean up the TL global event object */
	if (sTLGlobalData.hTLEventObj)
	{
		OSEventObjectDestroy(sTLGlobalData.hTLEventObj);
		sTLGlobalData.hTLEventObj = NULL;
	}

	/* Destroy the TL global data lock */
	if (sTLGlobalData.hTLGDLock)
	{
		OSLockDestroy (sTLGlobalData.hTLGDLock);
		sTLGlobalData.hTLGDLock = NULL;
	}

	PVR_DPF_RETURN;
}

void TLAddStreamNode(PTL_SNODE psAdd)
{
	TL_GLOBAL_DATA *psGD = TLGGD();

	PVR_DPF_ENTERED;

	PVR_ASSERT(psAdd);
	OSLockHeldAssert(psGD->hTLGDLock);

	dllist_add_to_head(&psGD->sNodeListHead, &psAdd->sNodeList);

	PVR_DPF_RETURN;
}

PTL_SNODE TLFindStreamNodeByName(const IMG_CHAR *pszName)
{
	TL_GLOBAL_DATA* psGD = TLGGD();
	PTL_SNODE psn;
	PDLLIST_NODE psNext, psNode;

	PVR_DPF_ENTERED;

	PVR_ASSERT(pszName);
	OSLockHeldAssert(psGD->hTLGDLock);

	dllist_foreach_node(&psGD->sNodeListHead, psNode, psNext)
	{
		psn = IMG_CONTAINER_OF(psNode, TL_SNODE, sNodeList);

		if (psn->psStream && OSStringNCompare(psn->psStream->szName,
		    pszName, PVRSRVTL_MAX_STREAM_NAME_SIZE)==0)
		{
			PVR_DPF_RETURN_VAL(psn);
		}
	}

	PVR_DPF_RETURN_VAL(NULL);
}

PTL_SNODE TLFindStreamNodeByDesc(PTL_STREAM_DESC psDesc)
{
	TL_GLOBAL_DATA* psGD = TLGGD();
	PTL_SNODE psn;
	PDLLIST_NODE psNext, psNode;

	PVR_DPF_ENTERED;

	PVR_ASSERT(psDesc);
	OSLockHeldAssert(psGD->hTLGDLock);

	dllist_foreach_node(&psGD->sNodeListHead, psNode, psNext)
	{
		psn = IMG_CONTAINER_OF(psNode, TL_SNODE, sNodeList);

		if (psn->psRDesc == psDesc || psn->psWDesc == psDesc)
		{
			PVR_DPF_RETURN_VAL(psn);
		}
	}
	PVR_DPF_RETURN_VAL(NULL);
}

IMG_UINT32 TLDiscoverStreamNodes(const IMG_CHAR *pszNamePattern,
                          IMG_CHAR aaszStreams[][PVRSRVTL_MAX_STREAM_NAME_SIZE],
                          IMG_UINT32 ui32Max)
{
	TL_GLOBAL_DATA *psGD = TLGGD();
	PTL_SNODE psn;
	IMG_UINT32 ui32Count = 0;
	size_t uiLen;
	PDLLIST_NODE psNode, psNext;

	PVR_ASSERT(pszNamePattern);
	OSLockHeldAssert(psGD->hTLGDLock);

	if ((uiLen = OSStringLength(pszNamePattern)) == 0)
		return 0;

	dllist_foreach_node(&psGD->sNodeListHead, psNode, psNext)
	{
		psn = IMG_CONTAINER_OF(psNode, TL_SNODE, sNodeList);

		if (OSStringNCompare(pszNamePattern, psn->psStream->szName, uiLen) != 0)
			continue;

		/* If aaszStreams is NULL we only count how many string match
		 * the given pattern. If it's a valid pointer we also return
		 * the names. */
		if (aaszStreams != NULL)
		{
			if (ui32Count >= ui32Max)
				break;

			/* all of names are shorter than MAX and null terminated */
			OSStringSafeCopy(aaszStreams[ui32Count], psn->psStream->szName,
			              PVRSRVTL_MAX_STREAM_NAME_SIZE);
		}

		ui32Count++;
	}

	return ui32Count;
}

PTL_SNODE TLFindAndGetStreamNodeByDesc(PTL_STREAM_DESC psDesc)
{
	PTL_SNODE psn;

	PVR_DPF_ENTERED;

	psn = TLFindStreamNodeByDesc(psDesc);
	if (psn == NULL)
		PVR_DPF_RETURN_VAL(NULL);

	PVR_ASSERT(psDesc == psn->psWDesc);

	psn->uiWRefCount++;
	psDesc->uiRefCount++;

	PVR_DPF_RETURN_VAL(psn);
}

void TLReturnStreamNode(PTL_SNODE psNode)
{
	psNode->uiWRefCount--;
	psNode->psWDesc->uiRefCount--;

	PVR_ASSERT(psNode->uiWRefCount > 0);
	PVR_ASSERT(psNode->psWDesc->uiRefCount > 0);
}

IMG_BOOL TLTryRemoveStreamAndFreeStreamNode(PTL_SNODE psRemove)
{
	PVR_DPF_ENTERED;

	PVR_ASSERT(psRemove);

#if defined(PVR_DPF_FUNCTION_TRACE_ON)
	PVR_DPF((PVR_DBG_WARNING, "%s: Trying to remove '%s' Descs = {%p, %p}",
	         __func__, psRemove->psStream->szName, psRemove->psRDesc,
	         psRemove->psWDesc));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */

	/* If there is a client connected to this stream, defer stream's deletion */
	if (psRemove->psRDesc != NULL || psRemove->psWDesc != NULL)
	{
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
		if (psRemove->psRDesc != NULL)
		{
			PVR_DPF((PVR_DBG_WARNING,
			         "%s: RDescs present - WRefCount [%x] DescRefCount [%x]",
		         __func__, psRemove->uiWRefCount, psRemove->psRDesc->uiRefCount));
		}
		if (psRemove->psWDesc != NULL)
		{
			PVR_DPF((PVR_DBG_WARNING,
			         "%s: WDescs present - WRefCount [%x] DescRefCount [%x]",
			         __func__, psRemove->uiWRefCount, psRemove->psWDesc->uiRefCount));
		}
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */
		PVR_DPF_RETURN_VAL(IMG_FALSE);
	}

	/* Remove stream from TL_GLOBAL_DATA's list and free stream node */
	psRemove->psStream = NULL;
	RemoveAndFreeStreamNode(psRemove);

	PVR_DPF_RETURN_VAL(IMG_TRUE);
}

IMG_BOOL TLUnrefDescAndTryFreeStreamNode(PTL_SNODE psNodeToRemove,
                                          PTL_STREAM_DESC psSD)
{
	PVR_DPF_ENTERED;

	PVR_ASSERT(psNodeToRemove);
	PVR_ASSERT(psSD);

	/* Decrement the reference count of the PTL_STREAM_DESC only if we're
	 * releasing the last reference held by the psNodeToRemove associated
	 * structure.
	 */
	if ((psNodeToRemove->i32RefCount > 0) ||
	    (psNodeToRemove->psStream->i32RefCount > 0))
	{
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
		PVR_DPF((PVR_DBG_WARNING, "%s: Dropping '%s' refcount [%d, %d]"
		         "RRef [%x]. WRef [%x], WRefCount [%x]",
		         __func__,
		         psNodeToRemove->psStream->szName,
		         psNodeToRemove->i32RefCount,
		         psNodeToRemove->psStream->i32RefCount,
		         psNodeToRemove->psRDesc ? psNodeToRemove->psRDesc->uiRefCount : 0,
		         psNodeToRemove->psWDesc ? psNodeToRemove->psWDesc->uiRefCount : 0,
		         psSD->uiRefCount));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */

		if (psNodeToRemove->psStream->i32RefCount > 0)
		{
			psNodeToRemove->psStream->i32RefCount--;
		}
		if (psNodeToRemove->i32RefCount > 0)
		{
			psNodeToRemove->i32RefCount--;
		}

#if defined(PVR_DPF_FUNCTION_TRACE_ON)
		PVR_DPF((PVR_DBG_WARNING,
		         "%s: Async close = '%s' for '%s'",
		         __func__,
		         psNodeToRemove->psStream->bAsyncClose ? "TRUE" : "FALSE",
		         psNodeToRemove->psStream->szName));

		PVR_DPF((PVR_DBG_WARNING, "%s: SNode: WRefCount [%x], RDesc [%x], WDesc [%x]",
		         __func__, psNodeToRemove->uiWRefCount,
		         psNodeToRemove->psRDesc ? psNodeToRemove->psRDesc->uiRefCount : 0,
		         psNodeToRemove->psWDesc ? psNodeToRemove->psWDesc->uiRefCount : 0));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */
	}
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
	else
	{
		PVR_DPF((PVR_DBG_WARNING,
		         "%s: Freeing '%s' refCount {%d, %d}, WRefCount [%x], Ref [%x]",
		         __func__,
		         psNodeToRemove->psStream->szName,
		         psNodeToRemove->i32RefCount,
		         psNodeToRemove->psStream->i32RefCount,
		         psNodeToRemove->uiWRefCount, psSD->uiRefCount));
	}
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */

	/* Decrement reference count. For descriptor obtained by reader it must
	 * reach 0 (only single reader allowed) and for descriptors obtained by
	 * writers it must reach value greater or equal to 0 (multiple writers
	 * model). */
	if (psSD->uiRefCount > 0)
	{
		psSD->uiRefCount--;
	}
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: '%s': Potential underrun detected!",
		         __func__, psNodeToRemove->psStream->szName));
	}
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */

	if (psSD == psNodeToRemove->psRDesc)
	{
		if (0 == psSD->uiRefCount)
		{
			/* Remove stream descriptor (i.e. stream reader context) */
			psNodeToRemove->psRDesc = NULL;
		}
	}
	else if (psSD == psNodeToRemove->psWDesc)
	{
		PVR_ASSERT(0 <= psSD->uiRefCount);

		if (psNodeToRemove->uiWRefCount > 0)
		{
			psNodeToRemove->uiWRefCount--;
		}

		/* Remove stream descriptor if reference == 0 */
		if (0 == psSD->uiRefCount)
		{
			psNodeToRemove->psWDesc = NULL;
		}
	}

	/* Do not Free Stream Node if there is a write reference (a producer
	 * context) to the stream */
	if (NULL != psNodeToRemove->psRDesc || NULL != psNodeToRemove->psWDesc ||
	    0 != psNodeToRemove->uiWRefCount)
	{
		PVR_DPF_RETURN_VAL(IMG_FALSE);
	}

	/* Make stream pointer NULL to prevent it from being destroyed in
	 * RemoveAndFreeStreamNode. Cleanup of stream should be done by the
	 * calling context */
	psNodeToRemove->psStream = NULL;
	RemoveAndFreeStreamNode(psNodeToRemove);

	PVR_DPF_RETURN_VAL(IMG_TRUE);
}

void TLActivateDeferredFree(void)
{
	TL_GLOBAL_DATA *psGD = TLGGD();
	PTL_SNODE psTLSNode;
	PDLLIST_NODE psNode, psNext;

	OSLockAcquire (psGD->hTLGDLock);

	if (TL_HAS_DEFERRED_FREE(psGD->uiTLDeferredFrees))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Deferred Free marked as present already",
		         __func__));

		OSLockRelease (psGD->hTLGDLock);
		return;
	}

	/* Find all current Stream Descriptors and bump their i32RefCount values
	 * so that the streams persist until the TLDeactivateDeferredFree() routine
	 * is called at consumer destruction time.
	 */
	psGD->uiTLDeferredFrees = TL_DEFERRED_FREE_BIT;
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
	PVR_DPF((PVR_DBG_WARNING, "%s: %d Clients present", __func__,
	         psGD->uiClientCnt));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */

	dllist_foreach_node(&psGD->sNodeListHead, psNode, psNext)
	{
		psTLSNode = IMG_CONTAINER_OF(psNode, TL_SNODE, sNodeList);

#if defined(PVR_DPF_FUNCTION_TRACE_ON)
		PVR_DPF((PVR_DBG_WARNING, "%s: Stream '%s', RefCount [%d, %d]",
		         __func__, psTLSNode->psStream->szName,
		         psTLSNode->i32RefCount,psTLSNode->psStream->i32RefCount));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */

		psTLSNode->i32RefCount++;
		psTLSNode->psStream->i32RefCount++;

		/* Bump the uiWRefCount for the SNODE and increment the SDESC uiRefCount
		 * for those descriptors present (psRDesc / psWDesc)
		 */
		psTLSNode->uiWRefCount++;
		if (psTLSNode->psRDesc)
		{
			psTLSNode->psRDesc->uiRefCount++;
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
			/* Dump OSEvent associated with descriptor */
			PVR_DPF((PVR_DBG_WARNING, "%s: RDesc Event %p",
			         __func__, psTLSNode->psRDesc->hReadEvent));
#if defined(__KERNEL__)
			OSEventObjectDumpDebugInfo(psTLSNode->psRDesc->hReadEvent);
#endif
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */
		}
		if (psTLSNode->psWDesc)
		{
			psTLSNode->psWDesc->uiRefCount++;
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
			/* Dump OSEvent associated with descriptor */
			PVR_DPF((PVR_DBG_WARNING, "%s: WDesc Event %p",
			         __func__, psTLSNode->psWDesc->hReadEvent));
#if defined(__KERNEL__)
			OSEventObjectDumpDebugInfo(psTLSNode->psWDesc->hReadEvent);
#endif
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */
		}
		TL_DEFERRED_FREE_INC(psGD->uiTLDeferredFrees);
	}

	OSLockRelease (psGD->hTLGDLock);
}

void TLDeactivateDeferredFree(void)
{
	TL_GLOBAL_DATA *psGD = TLGGD();
	PTL_SNODE psTLSNode;
	PDLLIST_NODE psNode, psNext;

	OSLockAcquire (psGD->hTLGDLock);

	if (!TL_HAS_DEFERRED_FREE(psGD->uiTLDeferredFrees))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Missing Deferred Free flag", __func__));

		OSLockRelease (psGD->hTLGDLock);
		return;
	}

	/* Save the number of still active clients as our DeferredFrees count */
	psGD->uiTLDeferredFrees = 0U;

	dllist_foreach_node(&psGD->sNodeListHead, psNode, psNext)
	{
		psTLSNode = IMG_CONTAINER_OF(psNode, TL_SNODE, sNodeList);

#if defined(PVR_DPF_FUNCTION_TRACE_ON)
		PVR_DPF((PVR_DBG_WARNING, "%s: Stream '%s', RefCount [%d, %d]",
		         __func__, psTLSNode->psStream->szName,
		         psTLSNode->i32RefCount,psTLSNode->psStream->i32RefCount));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */

		if (psTLSNode->i32RefCount > 0)
		{
			psTLSNode->i32RefCount--;
		}
		if (psTLSNode->psStream->i32RefCount > 0)
		{
			psTLSNode->psStream->i32RefCount--;
		}

		if ((psTLSNode->psStream->i32RefCount == 0) &&
		    (psTLSNode->i32RefCount == 0))
		{
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
			PVR_DPF((PVR_DBG_WARNING,
			         "%s: Deferred close candidate '%s'", __func__,
			         psTLSNode->psStream->szName));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */
		}

		/* Drop the extra bumped SNode reference counts obtained during the
		 * TLActivateDeferredFree() processing.
		 */
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
		PVR_DPF((PVR_DBG_WARNING, "%s: uiWRefCount = [%x]", __func__,
		         psTLSNode->uiWRefCount));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */

		if (psTLSNode->uiWRefCount > 0)
		{
			psTLSNode->uiWRefCount--;
		}
		else
		{
#if defined(PVR_DPF_FUNCTION_TRACE_ON)
			PVR_DPF((PVR_DBG_WARNING, "%s: Potential under-run",
			         __func__));
#endif	/* PVR_DPF_FUNCTION_TRACE_ON */
		}
		if (psTLSNode->psRDesc)
		{
			psTLSNode->psRDesc->uiRefCount--;
		}
		if (psTLSNode->psWDesc)
		{
			psTLSNode->psWDesc->uiRefCount--;
		}
		TL_DEFERRED_FREE_INC(psGD->uiTLDeferredFrees);
	}

	OSLockRelease (psGD->hTLGDLock);
}
