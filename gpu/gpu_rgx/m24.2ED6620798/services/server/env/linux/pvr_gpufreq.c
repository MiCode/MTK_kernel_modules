/*************************************************************************/ /*!
@File           pvr_gpufreq.c
@Title          PVR GPU Frequency tracepoint implementation
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
#define CREATE_TRACE_POINTS
#include "gpu_frequency.h"
#undef CREATE_TRACE_POINTS

#include "pvr_gpufreq.h"
#include "rgxdevice.h"
#include "pvrsrv.h"

#define MAX_EVENT_SIZE (16)

typedef struct _PVR_GPU_FREQ_EVENT_ {
	IMG_UINT32               ui32GpuId;
	IMG_UINT64               ui64ClockSpeedInKHz;
} PVR_GPU_FREQ_EVENT;

typedef struct
{
	/* The actual data and refer to gpu frequency metric here. */
	PVR_GPU_FREQ_EVENT       sEvents[MAX_EVENT_SIZE];
	/* The index suggests the next read position. */
	IMG_UINT32               ui32ReadIndex;
	/* The index suggests the next write position. */
	IMG_UINT32               ui32WriteIndex;
	/* Safe to use for multiple threads. */
	POS_SPINLOCK             hBufferLock;
} PVR_GPU_FREQ_EVENT_QUEUE;

typedef struct _PVR_GPU_FREQ_DATA_ {
	ATOMIC_T                 hTraceEnabled;
	IMG_HANDLE               hMISR;
	PVR_GPU_FREQ_EVENT_QUEUE sFreqEventQueue;
} PVR_GPU_FREQ_DATA;

static PVR_GPU_FREQ_DATA gGpuFreqPrivData;

static PVRSRV_ERROR
_InitGpuFreqEventQueue(PVR_GPU_FREQ_EVENT_QUEUE *psEventQueue)
{
	PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;

	if (!psEventQueue)
	{
		goto err_out;
	}

	psEventQueue->ui32ReadIndex = 0;
	psEventQueue->ui32WriteIndex = 0;

	eError = OSSpinLockCreate(&psEventQueue->hBufferLock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create a lock.", __func__));
	}

err_out:
	return eError;
}

static void
_DeInitGpuFreqEventQueue(PVR_GPU_FREQ_EVENT_QUEUE *psEventQueue)
{
	if (!psEventQueue)
	{
		return;
	}

	psEventQueue->ui32ReadIndex = 0;
	psEventQueue->ui32WriteIndex = 0;

	if (psEventQueue->hBufferLock)
	{
		OSSpinLockDestroy(psEventQueue->hBufferLock);
	}
}

static PVRSRV_ERROR
_AddGpuFreqEvent(PVR_GPU_FREQ_EVENT_QUEUE *psEventQueue,
		PVR_GPU_FREQ_EVENT sEvent)
{
	PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;
	IMG_UINT32 ui32NextWriteIndex;
	OS_SPINLOCK_FLAGS uiFlags;

	if (!psEventQueue || !psEventQueue->hBufferLock)
	{
		goto err_out;
	}

	OSSpinLockAcquire(psEventQueue->hBufferLock, uiFlags);

	ui32NextWriteIndex = (psEventQueue->ui32WriteIndex + 1) % MAX_EVENT_SIZE;

	/* Check if the buffer is full. */
	if (ui32NextWriteIndex == psEventQueue->ui32ReadIndex)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: The buffer is full.", __func__));

		goto err_release_lock;
	}

	psEventQueue->sEvents[psEventQueue->ui32WriteIndex].ui32GpuId =
		sEvent.ui32GpuId;
	psEventQueue->sEvents[psEventQueue->ui32WriteIndex].ui64ClockSpeedInKHz =
		sEvent.ui64ClockSpeedInKHz;
	psEventQueue->ui32WriteIndex = ui32NextWriteIndex;

	eError = PVRSRV_OK;

err_release_lock:
	OSSpinLockRelease(psEventQueue->hBufferLock, uiFlags);
err_out:
	return eError;
}

static PVRSRV_ERROR
_GetGpuFreqEvent(PVR_GPU_FREQ_EVENT_QUEUE *psEventQueue,
		PVR_GPU_FREQ_EVENT *psEvent)
{
	PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;
	OS_SPINLOCK_FLAGS uiFlags;

	if (!psEventQueue || !psEventQueue->hBufferLock)
	{
		goto err_out;
	}

	OSSpinLockAcquire(psEventQueue->hBufferLock, uiFlags);

	/* Check if the buffer is empty. */
	if (psEventQueue->ui32ReadIndex == psEventQueue->ui32WriteIndex)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: The buffer is empty.", __func__));

		goto err_release_lock;
	}

	psEvent->ui32GpuId =
		psEventQueue->sEvents[psEventQueue->ui32ReadIndex].ui32GpuId;
	psEvent->ui64ClockSpeedInKHz =
		psEventQueue->sEvents[psEventQueue->ui32ReadIndex].ui64ClockSpeedInKHz;

	psEventQueue->ui32ReadIndex =
		(psEventQueue->ui32ReadIndex + 1) % MAX_EVENT_SIZE;
	eError = PVRSRV_OK;

err_release_lock:
	OSSpinLockRelease(psEventQueue->hBufferLock, uiFlags);
err_out:
	return eError;
}
/* ------------------------------------------------------------------------- */

void GpuTraceFrequency(IMG_UINT32 ui32GpuId, IMG_UINT64 ui64NewClockSpeedInKHz)
{
	PVR_GPU_FREQ_EVENT sEvent = {ui32GpuId, ui64NewClockSpeedInKHz};
	PVR_GPU_FREQ_DATA *psGpuFreqData = &gGpuFreqPrivData;

	if (OSAtomicRead(&psGpuFreqData->hTraceEnabled) == IMG_FALSE)
		return;

	if (_AddGpuFreqEvent(&psGpuFreqData->sFreqEventQueue, sEvent) == PVRSRV_OK)
	{
		OSScheduleMISR(psGpuFreqData->hMISR);
	}
}

/* This gets called when this MISR is fired. */
static void _EventEmitter(void *pvData)
{
	PVR_GPU_FREQ_DATA *psGpuFreqData = (PVR_GPU_FREQ_DATA *)pvData;
	PVR_GPU_FREQ_EVENT sFreqEvent;
	PVRSRV_ERROR eError;

	eError = _GetGpuFreqEvent(&psGpuFreqData->sFreqEventQueue, &sFreqEvent);
	if (eError == PVRSRV_OK)
	{
		trace_gpu_frequency(sFreqEvent.ui64ClockSpeedInKHz,
				sFreqEvent.ui32GpuId);
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
int PVRGpuTraceEnableFreqCallback(void)
#else
void PVRGpuTraceEnableFreqCallback(void)
#endif
{
	PVR_GPU_FREQ_DATA *psGpuFreqData = &gGpuFreqPrivData;
	PVRSRV_ERROR eError;
	IMG_UINT32 i;

	OSAtomicWrite(&psGpuFreqData->hTraceEnabled, IMG_FALSE);

	eError = _InitGpuFreqEventQueue(&psGpuFreqData->sFreqEventQueue);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to init an event queue.", __func__));
		goto err_out;
	}

	eError = OSInstallMISR(&psGpuFreqData->hMISR, _EventEmitter, psGpuFreqData,
			"PVR_GPUFreq");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create misr.", __func__));
		goto err_deinit_queue;
	}

	OSAtomicWrite(&psGpuFreqData->hTraceEnabled, IMG_TRUE);

	for (i = 0; i < PVRSRV_MAX_DEVICES; i++)
	{
		PVRSRV_DEVICE_NODE *psDevNode = PVRSRVGetDeviceInstance(i);
		RGX_DATA *psRGXData;

		if (psDevNode)
		{
			psRGXData = (RGX_DATA *)psDevNode->psDevConfig->hDevData;
			GpuTraceFrequency(i, psRGXData->psRGXTimingInfo->ui32CoreClockSpeed);
		}
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	return 0;
#else
	return;
#endif

err_deinit_queue:
	_DeInitGpuFreqEventQueue(&psGpuFreqData->sFreqEventQueue);
err_out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	return -ENOMEM;
#else
	return;
#endif
}

void PVRGpuTraceDisableFreqCallback(void)
{
	PVR_GPU_FREQ_DATA *psGpuFreqData = &gGpuFreqPrivData;

	OSAtomicWrite(&psGpuFreqData->hTraceEnabled, IMG_FALSE);

	if (OSUninstallMISR(psGpuFreqData->hMISR) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to uninstall misr.", __func__));
	}
	psGpuFreqData->hMISR = NULL;

	_DeInitGpuFreqEventQueue(&psGpuFreqData->sFreqEventQueue);
}
