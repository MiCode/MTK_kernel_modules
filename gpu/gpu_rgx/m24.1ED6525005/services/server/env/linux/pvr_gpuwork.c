/*************************************************************************/ /*!
@File           pvr_gpuwork.c
@Title          PVR GPU Work Period implementation
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
#include <linux/trace_events.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/rbtree.h>
#define CREATE_TRACE_POINTS
#include "gpu_work.h"
#undef CREATE_TRACE_POINTS
#include "pvr_ricommon.h" /* for PVR_SYS_ALLOC_PID */
#include "pvr_gpuwork.h"
#include "pvr_debug.h"
#include "pvrsrv.h"
#include "hash.h"
#include "dllist.h"

#define MS_PER_SEC                              (1000UL)
#define THREAD_DESTROY_TIMEOUT                  (100000ULL)
#define THREAD_DESTROY_RETRIES                  (10U)

#define PVR_GPU_TRACE_WORK_PERIOD_UID_HASH_SIZE (32)

typedef enum THREAD_STATE
{
	THREAD_STATE_NULL,
	THREAD_STATE_ALIVE,
	THREAD_STATE_TERMINATED,
} THREAD_STATE;

typedef struct _PVR_GPU_WORK_PERIOD_ {
	/* RBTree node */
	struct rb_node                     sNode;
	/* Indicates which UID own this work period event stats. */
	IMG_UINT32                         ui32Uid;
	/* ui32GpuId indicates which GPU files this event. */
	IMG_UINT32                         ui32GpuId;
	/* The start time of this period. */
	IMG_UINT64                         ui64StartTimeInNs;
	/* The end time of this period. */
	IMG_UINT64                         ui64EndTimeInNs;
} PVR_GPU_WORK_PERIOD;

typedef struct _PVR_GPU_WORK_PERIOD_EVENT_ {
	/* The is a unique value for each GPU jobs. It's possible to have
	 * distinct tasks with the same job ID, eg. TA and 3D.
	 */
	IMG_UINT32                         ui32JobId;
	/* ui32GpuId indicates which GPU files this event. */
	IMG_UINT32                         ui32GpuId;
	/* The timestamp of this job. */
	IMG_UINT64                         ui64Timestamp[PVR_GPU_WORK_EVENT_MAX];
	/* List node */
	DLLIST_NODE                        sListEntry;
} PVR_GPU_WORK_PERIOD_EVENT;

typedef struct _PVR_GPU_WORK_PERIOD_EVENT_STATS_ {
	/* Indicates which UID own this work period event stats. */
	IMG_UINT32                         ui32Uid;
	/* Having multiple connections in a process is allowed and it's possible
	 * to have multiple processes in one app. The counting stores how many
	 * connections are connecting. The stats is removed from the uid hash
	 * table when there aren't connections in the given app.
	 */
	IMG_UINT32                         ui32RefCount;

	/* The list stores incomplete gpu work events. The `start` and `end` events
	 * happen in pairs.
	 */
	DLLIST_NODE                        sActiveEventList;

	/* The rbtree stores the gpu work events which specify the `start` and `end`
	 * time already but there might be overlapping intervals.
	 */
	struct rb_root                     sEmitEvents;

	/* The previous active_end_time. */
	IMG_UINT64                         ui64LastActiveTimeInNs;
	/* Protects access to work period event list. */
	POS_LOCK                           hListLock;
} PVR_GPU_WORK_PERIOD_EVENT_STATS;

typedef struct _PVR_GPU_WORK_PERIOD_EVENT_DATA_ {
	/* The work period context is initialized when Android OS supports GPU
	 * metrics to allow driver to provide hardware usages. This feature is
	 * implemented on the basis of switch trace event.
	 */
	IMG_BOOL                           bInitialized;
	/* hTraceEnabled is used to indicate if the event is enabled or not. */
	ATOMIC_T                           hTraceEnabled;
	/* The hash table stores work period stats for each apps. */
	HASH_TABLE                         *psUidHashTable;
	/* A thread to process switch events to meet the work period event
	 * specification and writes ftrace events.
	 */
	void                               *hProcessThread;
	/* hProcessEventObject is used to indicate if any pending work period
	 * events to emit.
	 */
	IMG_HANDLE                         hProcessEventObject;
	/* The state of the process thread. */
	ATOMIC_T                           hProcessThreadState;
	/* Protects access to UID hashtable. */
	POS_LOCK                           hHashTableLock;
} PVR_GPU_WORK_PERIOD_EVENT_DATA;

static PVR_GPU_WORK_PERIOD_EVENT_DATA gGpuWorkPeriodEventData;

#define getGpuWorkPeriodEventData() (&gGpuWorkPeriodEventData)

static IMG_UINT32 _GetUID(IMG_PID pid)
{
	struct task_struct *psTask;
	struct pid *psPid;

	psPid = find_get_pid((pid_t)pid);
	if (!psPid)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to lookup PID %u.",
		                        __func__, pid));
		return 0;
	}

	psTask = get_pid_task(psPid, PIDTYPE_PID);
	if (!psTask)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get pid task for PID %u.",
		                        __func__, pid));
	}
	put_pid(psPid);

	return psTask ? from_kuid(&init_user_ns, psTask->cred->uid) : 0;
}

static IMG_BOOL
_IsGpuWorkPeriodEventInitialized(void)
{
	return getGpuWorkPeriodEventData()->bInitialized;
}

/* ---------------- Stack helper with double linked list ------------------- */

struct stackNode
{
	/* List node */
	DLLIST_NODE                        sListEntry;
	/* Private data*/
	void                               *pvData;
};

struct stackImpl
{
	/* List */
	DLLIST_NODE                        sList;
};

static struct stackImpl *_CreateStack(void)
{
	struct stackImpl *psStack = OSAllocZMem(sizeof(struct stackImpl));

	if (!psStack)
		return NULL;

	dllist_init(&psStack->sList);

	return psStack;
}

static void _DeleteStack(struct stackImpl *psStack)
{
	DLLIST_NODE *psNext, *psNode;

	if (!psStack)
		return;

	dllist_foreach_node(&psStack->sList, psNode, psNext)
	{
		struct stackNode *psStackNode =
			IMG_CONTAINER_OF(psNode, struct stackNode, sListEntry);

		dllist_remove_node(&psStackNode->sListEntry);
		OSFreeMem(psStackNode);
	}

	OSFreeMem(psStack);
}

static void _StackPush(struct stackImpl *psStack, void *pvData)
{
	struct stackNode *psStackNode;

	if (!psStack)
	{
		return;
	}

	psStackNode = OSAllocZMem(sizeof(struct stackNode));
	if (!psStackNode)
	{
		return;
	}

	psStackNode->pvData = pvData;
	dllist_add_to_head(&psStack->sList, &psStackNode->sListEntry);
}

static void *_StackPopFromBottom(struct stackImpl *psStack)
{
	struct stackNode *psStackTopNode;
	void *pvData;

	if (!psStack)
	{
		return NULL;
	}

	if (dllist_is_empty(&psStack->sList))
	{
		return NULL;
	}

	psStackTopNode = IMG_CONTAINER_OF(dllist_get_prev_node(&psStack->sList),
			struct stackNode, sListEntry);
	pvData = psStackTopNode->pvData;

	dllist_remove_node(&psStackTopNode->sListEntry);
	OSFreeMem(psStackTopNode);

	return pvData;
}

static void *_StackTop(struct stackImpl *psStack)
{
	struct stackNode *psStackTopNode;
	void *pvData;

	if (!psStack)
	{
		return NULL;
	}

	if (dllist_is_empty(&psStack->sList))
	{
		return NULL;
	}

	psStackTopNode = IMG_CONTAINER_OF(dllist_get_next_node(&psStack->sList),
			struct stackNode, sListEntry);
	pvData = psStackTopNode->pvData;

	return pvData;
}

static bool _StackIsEmpty(struct stackImpl *psStack)
{
	if (!psStack)
	{
		return true;
	}

	return dllist_is_empty(&psStack->sList);
}

/* ---------- Functions to operate the work period event list -------------- */

static PVR_GPU_WORK_PERIOD_EVENT*
_FindGpuWorkPeriodEvent(DLLIST_NODE *psActiveEventList,
		IMG_UINT32 ui32JobId)
{
	PVR_GPU_WORK_PERIOD_EVENT *psGpuWorkPeriodEvent;
	DLLIST_NODE *psNext, *psNode;

	dllist_foreach_node(psActiveEventList, psNode, psNext)
	{
		psGpuWorkPeriodEvent =
			IMG_CONTAINER_OF(psNode, PVR_GPU_WORK_PERIOD_EVENT, sListEntry);

		if (psGpuWorkPeriodEvent->ui32JobId == ui32JobId)
		{
			return psGpuWorkPeriodEvent;
		}
	}

	return NULL;
}

static PVRSRV_ERROR _InsertEmitEvents(struct rb_root *psRoot,
		IMG_UINT64 ui64Timestamp[PVR_GPU_WORK_EVENT_MAX],
		IMG_UINT32 ui32GpuId, IMG_UINT32 ui32Uid)
{
	struct rb_node **psNewNode = &(psRoot->rb_node), *psParent = NULL;
	PVR_GPU_WORK_PERIOD *psGpuWorkPeriod;

	/* Figure out where to put new node */
	while (*psNewNode)
	{
		PVR_GPU_WORK_PERIOD *psPeriodCur =
			IMG_CONTAINER_OF(*psNewNode, PVR_GPU_WORK_PERIOD, sNode);

		psParent = *psNewNode;

		if (ui64Timestamp[PVR_GPU_WORK_EVENT_START] <
				psPeriodCur->ui64StartTimeInNs)
		{
			psNewNode = &((*psNewNode)->rb_left);
		}
		else if (ui64Timestamp[PVR_GPU_WORK_EVENT_START] >
				psPeriodCur->ui64StartTimeInNs)
		{
			psNewNode = &((*psNewNode)->rb_right);
		}
		else
		{
			psPeriodCur->ui64EndTimeInNs = max(
					ui64Timestamp[PVR_GPU_WORK_EVENT_END],
					psPeriodCur->ui64EndTimeInNs);

			return PVRSRV_OK;
		}
	}

	psGpuWorkPeriod = OSAllocZMem(sizeof(PVR_GPU_WORK_PERIOD));
	if (!psGpuWorkPeriod)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psGpuWorkPeriod->ui32GpuId = ui32GpuId;
	psGpuWorkPeriod->ui32Uid = ui32Uid;
	psGpuWorkPeriod->ui64StartTimeInNs = ui64Timestamp[PVR_GPU_WORK_EVENT_START];
	psGpuWorkPeriod->ui64EndTimeInNs = ui64Timestamp[PVR_GPU_WORK_EVENT_END];

	/* Add new node and rebalance tree. */
	rb_link_node(&psGpuWorkPeriod->sNode, psParent, psNewNode);
	rb_insert_color(&psGpuWorkPeriod->sNode, psRoot);

	return PVRSRV_OK;
}

static PVRSRV_ERROR
_InsertGpuWorkPeriodEvent(
		PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats,
		IMG_UINT32 ui32GpuId, IMG_UINT32 ui32JobId, IMG_UINT64 ui64TimeInNs,
		PVR_GPU_WORK_EVENT_TYPE eEventType)
{
	PVR_GPU_WORK_PERIOD_EVENT *psGpuWorkPeriodEvent;
	PVRSRV_ERROR eError = PVRSRV_OK;

	OSLockAcquire(psGpuWorkPeriodEventStats->hListLock);

	if (eEventType == PVR_GPU_WORK_EVENT_START) {
		psGpuWorkPeriodEvent = OSAllocZMem(sizeof(PVR_GPU_WORK_PERIOD_EVENT));
		PVR_LOG_GOTO_IF_NOMEM(psGpuWorkPeriodEvent, eError, err_release);

		psGpuWorkPeriodEvent->ui32GpuId = ui32GpuId;
		psGpuWorkPeriodEvent->ui32JobId = ui32JobId;
		psGpuWorkPeriodEvent->ui64Timestamp[PVR_GPU_WORK_EVENT_START] =
			ui64TimeInNs;

		dllist_add_to_tail(&psGpuWorkPeriodEventStats->sActiveEventList,
				&psGpuWorkPeriodEvent->sListEntry);
	}
	else
	{
		psGpuWorkPeriodEvent = _FindGpuWorkPeriodEvent(
				&psGpuWorkPeriodEventStats->sActiveEventList, ui32JobId);
		if (!psGpuWorkPeriodEvent)
		{
			/* The start event could be trimmed, this is fine. */
			eError = PVRSRV_OK;

			goto err_release;
		}

		/* All data was available. Move this event to the emit list. */
		dllist_remove_node(&psGpuWorkPeriodEvent->sListEntry);

		psGpuWorkPeriodEvent->ui64Timestamp[PVR_GPU_WORK_EVENT_END] =
			ui64TimeInNs;

		eError = _InsertEmitEvents(&psGpuWorkPeriodEventStats->sEmitEvents,
				psGpuWorkPeriodEvent->ui64Timestamp,
				psGpuWorkPeriodEvent->ui32GpuId,
				psGpuWorkPeriodEventStats->ui32Uid);

		OSFreeMem(psGpuWorkPeriodEvent);
	}

err_release:
	OSLockRelease(psGpuWorkPeriodEventStats->hListLock);
	return eError;
}

static void _CleanupEmittedEvents(
		PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats)
{
	struct rb_node *psNode = rb_first(&psGpuWorkPeriodEventStats->sEmitEvents);

	while (psNode)
	{
		PVR_GPU_WORK_PERIOD *psGpuWorkPeriod =
			rb_entry(psNode, PVR_GPU_WORK_PERIOD, sNode);
		struct rb_node *psNext = rb_next(psNode);

		rb_erase(psNode, &psGpuWorkPeriodEventStats->sEmitEvents);
		OSFreeMem(psGpuWorkPeriod);

		psNode = psNext;
	}
}

static void
_CleanupGpuWorkPeriodStats(
		PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats)
{
	PVR_GPU_WORK_PERIOD_EVENT *psGpuWorkPeriodEvent;
	DLLIST_NODE *psNext, *psNode;

	dllist_foreach_node(&psGpuWorkPeriodEventStats->sActiveEventList,
			psNode, psNext)
	{
		psGpuWorkPeriodEvent =
			IMG_CONTAINER_OF(psNode, PVR_GPU_WORK_PERIOD_EVENT, sListEntry);

		dllist_remove_node(&psGpuWorkPeriodEvent->sListEntry);
		OSFreeMem(psGpuWorkPeriodEvent);
	}

	_CleanupEmittedEvents(psGpuWorkPeriodEventStats);
}

static PVRSRV_ERROR
_DeleteGpuWorkPeriodStatsCallback(uintptr_t k, uintptr_t v, void *argv)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)v;
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		(PVR_GPU_WORK_PERIOD_EVENT_DATA *)argv;
	IMG_UINT32 ui32Uid = (IMG_UINT32)k;

	OSLockAcquire(psGpuWorkPeriodEventStats->hListLock);

	_CleanupGpuWorkPeriodStats(psGpuWorkPeriodEventStats);

	OSLockRelease(psGpuWorkPeriodEventStats->hListLock);

	HASH_Remove(psGpuWorkPeriodEventData->psUidHashTable, ui32Uid);

	OSLockDestroy(psGpuWorkPeriodEventStats->hListLock);
	OSFreeMem(psGpuWorkPeriodEventStats);

	return PVRSRV_OK;
}

/* ----------- The entry which binds to the PVR trace feature -------------- */

void GpuTraceWorkPeriod(IMG_PID pid, IMG_UINT32 u32GpuId,
		IMG_UINT64 ui64HWTimestampInOSTime,
		IMG_UINT32 ui32IntJobRef,
		PVR_GPU_WORK_EVENT_TYPE eEventType)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats;
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;
	IMG_UINT32 ui32Uid = _GetUID(pid);

	if (!_IsGpuWorkPeriodEventInitialized())
		return;

	if (OSAtomicRead(&psGpuWorkPeriodEventData->hTraceEnabled) == IMG_FALSE)
		return;

	OSLockAcquire(psGpuWorkPeriodEventData->hHashTableLock);

	psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)HASH_Retrieve(
				psGpuWorkPeriodEventData->psUidHashTable, (uintptr_t)ui32Uid);

	OSLockRelease(psGpuWorkPeriodEventData->hHashTableLock);

	if (!psGpuWorkPeriodEventStats) {
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to lookup PID %u in hash table",
					__func__, pid));
		PVR_LOG_RETURN_VOID_IF_ERROR(eError, "HASH_Retrieve");
	}

	eError = _InsertGpuWorkPeriodEvent(psGpuWorkPeriodEventStats,
			u32GpuId, ui32IntJobRef, ui64HWTimestampInOSTime,
			eEventType);
	PVR_LOG_RETURN_VOID_IF_ERROR(eError, "_InsertGpuWorkPeriodEvent");

	PVR_ASSERT(psGpuWorkPeriodEventData->hProcessEventObject);

	eError = OSEventObjectSignal(
			psGpuWorkPeriodEventData->hProcessEventObject);
	PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
}

static struct stackImpl *
_MergeOverlapping(PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats)
{
	PVR_GPU_WORK_PERIOD *psGpuWorkPeriod;
	struct stackImpl *psStack;
	struct rb_node *psNode;

	if (RB_EMPTY_ROOT(&psGpuWorkPeriodEventStats->sEmitEvents))
	{
		return NULL;
	}

	psStack = _CreateStack();
	if (!psStack)
	{
		return NULL;
	}

	/* Pop the first unreported event from the tree and push it to the stack. */
	for (psNode = rb_first(&psGpuWorkPeriodEventStats->sEmitEvents); psNode;
			psNode = rb_next(psNode))
	{
		psGpuWorkPeriod = rb_entry(psNode, PVR_GPU_WORK_PERIOD, sNode);

		if (psGpuWorkPeriodEventStats->ui64LastActiveTimeInNs <=
				psGpuWorkPeriod->ui64StartTimeInNs)
		{
			_StackPush(psStack, psGpuWorkPeriod);
			break;
		}
	}

	if (_StackIsEmpty(psStack))
	{
		return psStack;
	}

	/* Traverse the tree in increasing order */
	for (psNode = rb_next(psNode); psNode; psNode = rb_next(psNode))
	{
		PVR_GPU_WORK_PERIOD *psGpuWorkPeriodTop;

		psGpuWorkPeriod = rb_entry(psNode, PVR_GPU_WORK_PERIOD, sNode);

		/* Get the event[i - 1]. */
		psGpuWorkPeriodTop = (PVR_GPU_WORK_PERIOD *)(_StackTop(psStack));
		PVR_ASSERT(psGpuWorkPeriodTop != NULL);

		/* In sorted events, if the event_time[i] does not overlap with
		 * event_time[i - 1], then event_time[i + 1] does not overlap
		 * event_time[i - 1] as well. When event_time[i - 1] overlap
		 * event_time[i], merge event_time[i] to event_time[i - 1].
		 */
		if (psGpuWorkPeriod->ui64StartTimeInNs >
				psGpuWorkPeriodTop->ui64EndTimeInNs)
		{
			_StackPush(psStack, (void *)psGpuWorkPeriod);
		}
		else
		{
			psGpuWorkPeriodTop->ui64EndTimeInNs =
				psGpuWorkPeriod->ui64EndTimeInNs;
		}
	}

	return psStack;
}

static void
_EmitEvents(PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats,
		struct stackImpl *psStack)
{
	if (!psStack)
		return;

	while (!_StackIsEmpty(psStack))
	{
		PVR_GPU_WORK_PERIOD *psGpuWorkPeriod;
		IMG_UINT64 ui64LastActiveTimeInNs;
		IMG_UINT64 ui64StartTimeInNs;
		IMG_UINT64 ui64EndTimeInNs;

		/* The events in stack are in decreasing order so traversal the list
		 * from the end as the events must be emitted in strictly increasing
		 * order of their starting time.
		 */
		psGpuWorkPeriod = (PVR_GPU_WORK_PERIOD *)_StackPopFromBottom(psStack);
		if (!psGpuWorkPeriod)
		{
			break;
		}

		ui64StartTimeInNs = psGpuWorkPeriod->ui64StartTimeInNs;
		ui64EndTimeInNs = psGpuWorkPeriod->ui64EndTimeInNs;
		ui64LastActiveTimeInNs =
			psGpuWorkPeriodEventStats->ui64LastActiveTimeInNs;

		if (ui64StartTimeInNs >= ui64LastActiveTimeInNs)
		{
			trace_gpu_work_period(psGpuWorkPeriod->ui32GpuId,
					(IMG_UINT64)psGpuWorkPeriod->ui32Uid, ui64StartTimeInNs,
					ui64EndTimeInNs, ui64EndTimeInNs - ui64StartTimeInNs);

			psGpuWorkPeriodEventStats->ui64LastActiveTimeInNs = ui64EndTimeInNs;
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Invalid event timestamp "
			                        "(%llu, %llu), the last active time "
				                    "was %llu", __func__,
				                     ui64StartTimeInNs,
				                     ui64EndTimeInNs,
				                     ui64LastActiveTimeInNs));
		}
	}
}

static PVRSRV_ERROR
_EmitCallback(uintptr_t k, uintptr_t v, void *argv)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)v;
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	IMG_UINT32 ui32Uid = (IMG_UINT32)k;
	struct stackImpl *psStack;

	PVR_UNREFERENCED_PARAMETER(argv);

	OSLockAcquire(psGpuWorkPeriodEventStats->hListLock);

	psStack = _MergeOverlapping(psGpuWorkPeriodEventStats);
	_EmitEvents(psGpuWorkPeriodEventStats, psStack);
	_DeleteStack(psStack);

	_CleanupEmittedEvents(psGpuWorkPeriodEventStats);

	if (!psGpuWorkPeriodEventStats->ui32RefCount)
	{
		_CleanupGpuWorkPeriodStats(psGpuWorkPeriodEventStats);
		OSLockRelease(psGpuWorkPeriodEventStats->hListLock);

		/* Remove the bucket from the hash table. */
		HASH_Remove(psGpuWorkPeriodEventData->psUidHashTable, ui32Uid);

		OSLockDestroy(psGpuWorkPeriodEventStats->hListLock);
		OSFreeMem(psGpuWorkPeriodEventStats);

		return PVRSRV_OK;
	}

	OSLockRelease(psGpuWorkPeriodEventStats->hListLock);

	return PVRSRV_OK;
}

static void _ProcessGpuWorkPeriodEvents(void *pvData)
{
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		(PVR_GPU_WORK_PERIOD_EVENT_DATA *)pvData;
	IMG_HANDLE hProcessEvent;
	PVRSRV_ERROR eError;

	eError = OSEventObjectOpen(psGpuWorkPeriodEventData->hProcessEventObject,
			&hProcessEvent);
	PVR_LOG_RETURN_VOID_IF_ERROR(eError, "OSEventObjectOpen");

	while (OSAtomicRead(&psGpuWorkPeriodEventData->hProcessThreadState) ==
			THREAD_STATE_ALIVE)
	{
		eError = OSEventObjectWaitKernel(hProcessEvent, (IMG_UINT64)-1);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "OSEventObjectWaitKernel");
			continue;
		}

		OSSleepms(MS_PER_SEC);

		OSLockAcquire(psGpuWorkPeriodEventData->hHashTableLock);

		eError = HASH_Iterate(psGpuWorkPeriodEventData->psUidHashTable,
				_EmitCallback, NULL);
		PVR_LOG_IF_ERROR(eError, "HASH_Iterate");

		OSLockRelease(psGpuWorkPeriodEventData->hHashTableLock);
	}

	OSAtomicWrite(&psGpuWorkPeriodEventData->hProcessThreadState,
			THREAD_STATE_TERMINATED);
}

/* -------- Initialize/Deinitialize for Android GPU metrics ---------------- */

PVRSRV_ERROR GpuTraceWorkPeriodInitialize(void)
{
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	PVRSRV_ERROR eError;

	if (psGpuWorkPeriodEventData->bInitialized == IMG_FALSE)
	{
		eError = OSLockCreate(
				&psGpuWorkPeriodEventData->hHashTableLock);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate", err_out);

		psGpuWorkPeriodEventData->psUidHashTable =
			HASH_Create(PVR_GPU_TRACE_WORK_PERIOD_UID_HASH_SIZE);
		if (!psGpuWorkPeriodEventData->psUidHashTable)
		{
			PVR_LOG_GOTO_WITH_ERROR("HASH_Create", eError,
					PVRSRV_ERROR_OUT_OF_MEMORY, err_deInitialize);
		}

		eError = OSEventObjectCreate("process event object",
				&psGpuWorkPeriodEventData->hProcessEventObject);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSEventObjectCreate", err_deInitialize);

		OSAtomicWrite(&psGpuWorkPeriodEventData->hProcessThreadState,
				THREAD_STATE_NULL);

		OSAtomicWrite(&psGpuWorkPeriodEventData->hTraceEnabled, IMG_FALSE);

		psGpuWorkPeriodEventData->bInitialized = IMG_TRUE;
	}

	eError = PVRSRV_OK;

err_out:
	return eError;
err_deInitialize:
	GpuTraceSupportDeInitialize();
	goto err_out;
}

void GpuTraceSupportDeInitialize(void)
{
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	THREAD_STATE hProcessThreadState;
	PVRSRV_ERROR eError;

	if (!_IsGpuWorkPeriodEventInitialized())
		return;

	hProcessThreadState = OSAtomicCompareExchange(
			&psGpuWorkPeriodEventData->hProcessThreadState,
			THREAD_STATE_ALIVE,
			THREAD_STATE_TERMINATED);
	if (hProcessThreadState == THREAD_STATE_ALIVE)
	{
		if (psGpuWorkPeriodEventData->hProcessEventObject)
		{
			eError = OSEventObjectSignal(
					psGpuWorkPeriodEventData->hProcessEventObject);
			PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
		}

		LOOP_UNTIL_TIMEOUT_US(THREAD_DESTROY_TIMEOUT)
		{
			eError = OSThreadDestroy(psGpuWorkPeriodEventData->hProcessThread);
			if (eError == PVRSRV_OK)
			{
				break;
			}
			OSWaitus(THREAD_DESTROY_TIMEOUT/THREAD_DESTROY_RETRIES);
		} END_LOOP_UNTIL_TIMEOUT_US();

		PVR_LOG_IF_ERROR(eError, "OSThreadDestroy");
	}

	if (psGpuWorkPeriodEventData->hProcessEventObject)
	{
		eError = OSEventObjectDestroy(
				psGpuWorkPeriodEventData->hProcessEventObject);
		PVR_LOG_IF_ERROR(eError, "OSEventObjectDestroy");
	}

	if (psGpuWorkPeriodEventData->psUidHashTable)
	{
		OSLockAcquire(psGpuWorkPeriodEventData->hHashTableLock);

		eError = HASH_Iterate(psGpuWorkPeriodEventData->psUidHashTable,
				_DeleteGpuWorkPeriodStatsCallback, psGpuWorkPeriodEventData);

		OSLockRelease(psGpuWorkPeriodEventData->hHashTableLock);

		PVR_LOG_IF_ERROR(eError, "HASH_Iterate");

		HASH_Delete(psGpuWorkPeriodEventData->psUidHashTable);
		psGpuWorkPeriodEventData->psUidHashTable = NULL;
	}

	if (psGpuWorkPeriodEventData->hHashTableLock)
	{
		OSLockDestroy(psGpuWorkPeriodEventData->hHashTableLock);
		psGpuWorkPeriodEventData->hHashTableLock = NULL;
	}

	psGpuWorkPeriodEventData->bInitialized = IMG_FALSE;
}

static PVRSRV_ERROR
_RegisterProcess(IMG_HANDLE* phGpuWorkPeriodEventStats, IMG_PID ownerPid)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats;
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32Uid;

	PVR_ASSERT(phGpuWorkPeriodEventStats);

	if (!_IsGpuWorkPeriodEventInitialized())
		return PVRSRV_ERROR_NOT_INITIALISED;

	PVR_DPF((PVR_DBG_MESSAGE, "%s: Register process PID %d [%s]",
			__func__, ownerPid, (ownerPid == PVR_SYS_ALLOC_PID)
			? "system" : OSGetCurrentClientProcessNameKM()));

	OSLockAcquire(psGpuWorkPeriodEventData->hHashTableLock);

	ui32Uid = _GetUID(ownerPid);
	psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)HASH_Retrieve(
			psGpuWorkPeriodEventData->psUidHashTable, (uintptr_t)ui32Uid);
	if (psGpuWorkPeriodEventStats)
	{
		psGpuWorkPeriodEventStats->ui32RefCount++;
		*phGpuWorkPeriodEventStats = (IMG_HANDLE)psGpuWorkPeriodEventStats;
		/* The work period event stats was created for this PID.
		 * Take a reference and return the instance immediately.
		 */
		eError = PVRSRV_OK;
		goto err_release;
	}

	psGpuWorkPeriodEventStats =
		OSAllocZMem(sizeof(PVR_GPU_WORK_PERIOD_EVENT_STATS));
	PVR_LOG_GOTO_IF_NOMEM(psGpuWorkPeriodEventStats, eError, err_release);

	psGpuWorkPeriodEventStats->ui32Uid = ui32Uid;
	psGpuWorkPeriodEventStats->ui32RefCount++;

	eError = OSLockCreate(
			&psGpuWorkPeriodEventStats->hListLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSEventObjectCreate", err_release);

	dllist_init(&psGpuWorkPeriodEventStats->sActiveEventList);

	psGpuWorkPeriodEventStats->sEmitEvents = RB_ROOT;

	HASH_Insert(psGpuWorkPeriodEventData->psUidHashTable, (uintptr_t)ui32Uid,
			(uintptr_t)psGpuWorkPeriodEventStats);

	*phGpuWorkPeriodEventStats = (IMG_HANDLE)psGpuWorkPeriodEventStats;
	eError = PVRSRV_OK;

err_release:
	OSLockRelease(psGpuWorkPeriodEventData->hHashTableLock);
	return eError;
}

static bool
_HasValidStats(IMG_PID ownerPid)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats;
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	IMG_UINT32 ui32Uid = _GetUID(ownerPid);

	OSLockAcquire(psGpuWorkPeriodEventData->hHashTableLock);

	psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)HASH_Retrieve(
				psGpuWorkPeriodEventData->psUidHashTable, (uintptr_t)ui32Uid);

	OSLockRelease(psGpuWorkPeriodEventData->hHashTableLock);

	return (psGpuWorkPeriodEventStats != NULL) ? true : false;
}

static void _UnregisterProcess(IMG_HANDLE hProcessStats)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats;
	IMG_UINT32 ui32Uid;

	if (!hProcessStats || !_IsGpuWorkPeriodEventInitialized())
		return;

	if (!_HasValidStats(OSGetCurrentClientProcessIDKM()))
		return;

	psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)hProcessStats;

	OSLockAcquire(psGpuWorkPeriodEventStats->hListLock);

	ui32Uid = psGpuWorkPeriodEventStats->ui32Uid;
	psGpuWorkPeriodEventStats->ui32RefCount--;

	OSLockRelease(psGpuWorkPeriodEventStats->hListLock);
}

PVRSRV_ERROR
GpuTraceWorkPeriodEventStatsRegister(IMG_HANDLE
		*phGpuWorkPeriodEventStats)
{
	return _RegisterProcess(phGpuWorkPeriodEventStats,
			OSGetCurrentClientProcessIDKM());
}

void
GpuTraceWorkPeriodEventStatsUnregister(
		IMG_HANDLE hGpuWorkPeriodEventStats)
{
	_UnregisterProcess(hGpuWorkPeriodEventStats);
}

/* ----- FTrace event callbacks -------------------------------------------- */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
int PVRGpuTraceEnableWorkPeriodCallback(void)
#else
void PVRGpuTraceEnableWorkPeriodCallback(void)
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)) */
{
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	THREAD_STATE hProcessThreadState;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (!_IsGpuWorkPeriodEventInitialized())
	{
		PVR_LOG_GOTO_WITH_ERROR("_IsGpuWorkPeriodEventInitialized", eError,
					PVRSRV_ERROR_NOT_INITIALISED, err_out);
	}

	hProcessThreadState = OSAtomicCompareExchange(
			&psGpuWorkPeriodEventData->hProcessThreadState,
			THREAD_STATE_NULL,
			THREAD_STATE_ALIVE);

	/* if the thread has not been started yet do it */
	if (hProcessThreadState == THREAD_STATE_NULL)
	{
		PVR_ASSERT(psGpuWorkPeriodEventData->hProcessThread == NULL);

		eError = OSThreadCreate(&psGpuWorkPeriodEventData->hProcessThread,
				"gpu_work_period_process", _ProcessGpuWorkPeriodEvents,
				NULL, IMG_FALSE, psGpuWorkPeriodEventData);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSThreadCreate", err_terminate);
	}

	OSAtomicCompareExchange(&psGpuWorkPeriodEventData->hTraceEnabled,
			IMG_FALSE, IMG_TRUE);

err_out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	if (eError != PVRSRV_OK)
		return -ENODEV;
	return 0;
#else
	return;
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)) */
err_terminate:
	OSAtomicWrite(&psGpuWorkPeriodEventData->hProcessThreadState,
			THREAD_STATE_TERMINATED);
	goto err_out;
}

void PVRGpuTraceDisableWorkPeriodCallback(void)
{
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();

	if (!_IsGpuWorkPeriodEventInitialized())
		return;

	OSAtomicCompareExchange(&psGpuWorkPeriodEventData->hTraceEnabled,
			IMG_TRUE, IMG_FALSE);
}
