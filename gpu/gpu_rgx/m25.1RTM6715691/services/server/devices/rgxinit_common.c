/*************************************************************************/ /*!
@File           rgxinit_common.c
@Title          RGX device specific internal/external initialisation routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX device specific functions
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

#include "rgxinit.h"
#include "rgxinit_internal.h"

#include "allocmem.h"
#include "rgx_heaps_server.h"

#include "rgxdebug_common.h"

IMG_PCHAR RGXDevBVNCString(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_PCHAR psz = psDevInfo->sDevFeatureCfg.pszBVNCString;
	if (NULL == psz)
	{
		IMG_CHAR pszBVNCInfo[RGX_HWPERF_MAX_BVNC_LEN];
		size_t uiBVNCStringSize;
		size_t uiStringLength;

		uiStringLength = OSSNPrintf(pszBVNCInfo,
		                            RGX_HWPERF_MAX_BVNC_LEN,
		                            "%d.%d.%d.%d",
		                            psDevInfo->sDevFeatureCfg.ui32B,
		                            psDevInfo->sDevFeatureCfg.ui32V,
		                            psDevInfo->sDevFeatureCfg.ui32N,
		                            psDevInfo->sDevFeatureCfg.ui32C);
		PVR_ASSERT(uiStringLength < RGX_HWPERF_MAX_BVNC_LEN);

		uiBVNCStringSize = (uiStringLength + 1) * sizeof(IMG_CHAR);
		psz = OSAllocMem(uiBVNCStringSize);
		if (NULL != psz)
		{
			OSCachedMemCopy(psz, pszBVNCInfo, uiBVNCStringSize);
			psDevInfo->sDevFeatureCfg.pszBVNCString = psz;
		}
		else
		{
			PVR_DPF((PVR_DBG_MESSAGE,
			         "%s: Allocating memory for BVNC Info string failed",
			         __func__));
		}
	}

	return psz;
}

PVRSRV_ERROR RGXDevVersionString(PVRSRV_DEVICE_NODE *psDeviceNode,
                                 IMG_CHAR **ppszVersionString)
{
#if defined(NO_HARDWARE) || defined(EMULATOR)
	const IMG_CHAR szFormatString[] = "GPU variant BVNC: %s (SW)";
#else
	const IMG_CHAR szFormatString[] = "GPU variant BVNC: %s (HW)";
#endif
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_PCHAR pszBVNC;
	size_t uiStringLength;

	if (psDeviceNode == NULL || ppszVersionString == NULL)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRSRV_RGXDEV_INFO *)psDeviceNode->pvDevice;
	pszBVNC = RGXDevBVNCString(psDevInfo);

	if (NULL == pszBVNC)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	uiStringLength = OSStringLength(pszBVNC);
	uiStringLength += (sizeof(szFormatString) - 2); /* sizeof includes the null, -2 for "%s" */
	*ppszVersionString = OSAllocMem(uiStringLength * sizeof(IMG_CHAR));
	if (*ppszVersionString == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	OSSNPrintf(*ppszVersionString, uiStringLength, szFormatString,
		pszBVNC);

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXDevClockSpeed(PVRSRV_DEVICE_NODE *psDeviceNode,
                              IMG_PUINT32  pui32RGXClockSpeed)
{
	RGX_DATA *psRGXData = (RGX_DATA*) psDeviceNode->psDevConfig->hDevData;

	/* get clock speed */
	*pui32RGXClockSpeed = psRGXData->psRGXTimingInfo->ui32CoreClockSpeed;

	return PVRSRV_OK;
}

IMG_UINT32 RGXHeapDerivePageSize(IMG_UINT32 uiLog2PageSize)
{
	IMG_BOOL bFound = IMG_FALSE;
	IMG_UINT32 ui32PageSizeMask = RGXGetValidHeapPageSizeMask();

	/* OS page shift must be at least RGX_HEAP_4KB_PAGE_SHIFT,
	 * max RGX_HEAP_2MB_PAGE_SHIFT, non-zero and a power of two*/
	if (uiLog2PageSize == 0U ||
	    (uiLog2PageSize < RGX_HEAP_4KB_PAGE_SHIFT) ||
	    (uiLog2PageSize > RGX_HEAP_2MB_PAGE_SHIFT))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Provided incompatible log2 page size %u",
				__func__,
				uiLog2PageSize));
		PVR_ASSERT(0);
		return 0;
	}

	do
	{
		if ((IMG_PAGE2BYTES32(uiLog2PageSize) & ui32PageSizeMask) == 0)
		{
			/* We have to fall back to a smaller device
			 * page size than given page size because there
			 * is no exact match for any supported size. */
			uiLog2PageSize -= 1U;
		}
		else
		{
			/* All good, RGX page size equals given page size
			 * => use it as default for heaps */
			bFound = IMG_TRUE;
		}
	} while (!bFound);

	return uiLog2PageSize;
}

#if !defined(NO_HARDWARE)
IMG_BOOL SampleIRQCount(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_BOOL bReturnVal = IMG_FALSE;
	volatile IMG_UINT32 *pui32SampleIrqCount = psDevInfo->aui32SampleIRQCount;
	IMG_UINT32 ui32IrqCnt;

#if defined(RGX_FW_IRQ_OS_COUNTERS)
	if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		bReturnVal = IMG_TRUE;
	}
	else
	{
		get_irq_cnt_val(ui32IrqCnt, RGXFW_HOST_DRIVER_ID, psDevInfo);

		if (ui32IrqCnt != pui32SampleIrqCount[RGXFW_THREAD_0])
		{
			pui32SampleIrqCount[RGXFW_THREAD_0] = ui32IrqCnt;
			bReturnVal = IMG_TRUE;
		}
	}
#else
	IMG_UINT32 ui32TID;

	for_each_irq_cnt(ui32TID)
	{
		get_irq_cnt_val(ui32IrqCnt, ui32TID, psDevInfo);

		/* treat unhandled interrupts here to align host count with fw count */
		if (pui32SampleIrqCount[ui32TID] != ui32IrqCnt)
		{
			pui32SampleIrqCount[ui32TID] = ui32IrqCnt;
			bReturnVal = IMG_TRUE;
		}
	}
#endif

	return bReturnVal;
}

static IMG_BOOL _WaitForInterruptsTimeoutCheck(PVRSRV_RGXDEV_INFO *psDevInfo)
{
#if defined(PVRSRV_DEBUG_LISR_EXECUTION)
	PVRSRV_DEVICE_NODE *psDeviceNode = psDevInfo->psDeviceNode;
	IMG_UINT32 ui32idx;
#endif

	RGXDEBUG_PRINT_IRQ_COUNT(psDevInfo);

#if defined(PVRSRV_DEBUG_LISR_EXECUTION)
	PVR_DPF((PVR_DBG_ERROR,
	        "Last RGX_LISRHandler State (DevID %u): 0x%08X Clock: %" IMG_UINT64_FMTSPEC,
	        psDeviceNode->sDevId.ui32InternalID,
	        psDeviceNode->sLISRExecutionInfo.ui32Status,
	        psDeviceNode->sLISRExecutionInfo.ui64Clockns));

	for_each_irq_cnt(ui32idx)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         MSG_IRQ_CNT_TYPE " %u: InterruptCountSnapshot: 0x%X",
		         ui32idx, psDeviceNode->sLISRExecutionInfo.aui32InterruptCountSnapshot[ui32idx]));
	}
#else
	PVR_DPF((PVR_DBG_ERROR, "No further information available. Please enable PVRSRV_DEBUG_LISR_EXECUTION"));
#endif

	return SampleIRQCount(psDevInfo);
}

void RGX_WaitForInterruptsTimeout(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_BOOL bScheduleMISR;

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		bScheduleMISR = IMG_TRUE;
	}
	else
	{
		bScheduleMISR = _WaitForInterruptsTimeoutCheck(psDevInfo);
	}

	if (bScheduleMISR)
	{
		OSScheduleMISR(psDevInfo->pvMISRData);

		if (psDevInfo->pvAPMISRData != NULL)
		{
			OSScheduleMISR(psDevInfo->pvAPMISRData);
		}
	}
}

IMG_BOOL RGXAckHwIrq(PVRSRV_RGXDEV_INFO *psDevInfo,
                     IMG_UINT32 ui32IRQStatusReg,
                     IMG_UINT32 ui32IRQStatusEventMsk,
                     IMG_UINT32 ui32IRQClearReg,
                     IMG_UINT32 ui32IRQClearMask)
{
	IMG_UINT32 ui32IRQStatus = OSReadHWReg32(psDevInfo->pvRegsBaseKM, ui32IRQStatusReg);

	/* clear only the pending bit of the thread that triggered this interrupt */
	ui32IRQClearMask &= ui32IRQStatus;

	if (ui32IRQStatus & ui32IRQStatusEventMsk)
	{
		/* acknowledge and clear the interrupt */
		OSWriteHWReg32(psDevInfo->pvRegsBaseKM, ui32IRQClearReg, ui32IRQClearMask);
		/* Perform a readback as barrier here after clearing the interrupt.
		 * If host side mem read happens before we clear the interrupt it is possible
		 * that we read the stale value then fw updates the second interrupt which is
		 * ignored and then we clear interrupt which would mean host will end up with
		 * a stale IRQCount value.
		 */
		(void) OSReadHWReg32(psDevInfo->pvRegsBaseKM, ui32IRQClearReg);

		return IMG_TRUE;
	}
	else
	{
		/* spurious interrupt */
		return IMG_FALSE;
	}
}

IMG_BOOL RGXAckIrqDedicated(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	/* status & clearing registers are available on both Host and Guests
	 * and are agnostic of the Fw CPU type. Due to the remappings done by
	 * the 2nd stage device MMU, all drivers assume they are accessing
	 * register bank 0 */
	return RGXAckHwIrq(psDevInfo,
	                   RGX_CR_IRQ_OS0_EVENT_STATUS,
	                   ~RGX_CR_IRQ_OS0_EVENT_STATUS_SOURCE_CLRMSK,
	                   RGX_CR_IRQ_OS0_EVENT_CLEAR,
	                   ~RGX_CR_IRQ_OS0_EVENT_CLEAR_SOURCE_CLRMSK);
}
#endif
