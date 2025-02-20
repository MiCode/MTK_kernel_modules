/*******************************************************************************
@File
@Title          Server bridge for rgxhwperf
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Implements the server side of the bridge for rgxhwperf
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

#include <linux/uaccess.h>

#include "img_defs.h"

#include "rgxhwperf.h"
#include "rgx_fwif_km.h"

#include "common_rgxhwperf_bridge.h"

#include "allocmem.h"
#include "pvr_debug.h"
#include "connection_server.h"
#include "pvr_bridge.h"
#if defined(SUPPORT_RGX)
#include "rgx_bridge.h"
#endif
#include "srvcore.h"
#include "handle.h"

#include <linux/slab.h>

/* ***************************************************************************
 * Server-side bridge entry points
 */

static_assert(1 <= IMG_UINT32_MAX, "1 must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeRGXGetConfiguredHWPerfCounters(IMG_UINT32 ui32DispatchTableEntry,
					   IMG_UINT8 * psRGXGetConfiguredHWPerfCountersIN_UI8,
					   IMG_UINT8 * psRGXGetConfiguredHWPerfCountersOUT_UI8,
					   CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_RGXGETCONFIGUREDHWPERFCOUNTERS *psRGXGetConfiguredHWPerfCountersIN =
	    (PVRSRV_BRIDGE_IN_RGXGETCONFIGUREDHWPERFCOUNTERS *)
	    IMG_OFFSET_ADDR(psRGXGetConfiguredHWPerfCountersIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_RGXGETCONFIGUREDHWPERFCOUNTERS *psRGXGetConfiguredHWPerfCountersOUT =
	    (PVRSRV_BRIDGE_OUT_RGXGETCONFIGUREDHWPERFCOUNTERS *)
	    IMG_OFFSET_ADDR(psRGXGetConfiguredHWPerfCountersOUT_UI8, 0);

	RGX_HWPERF_CONFIG_CNTBLK *psConfiguredCountersInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize = ((IMG_UINT64) 1 * sizeof(RGX_HWPERF_CONFIG_CNTBLK)) + 0;

	psRGXGetConfiguredHWPerfCountersOUT->psConfiguredCounters =
	    psRGXGetConfiguredHWPerfCountersIN->psConfiguredCounters;

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psRGXGetConfiguredHWPerfCountersOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto RGXGetConfiguredHWPerfCounters_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psRGXGetConfiguredHWPerfCountersIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer =
			    (IMG_BYTE *) (void *)psRGXGetConfiguredHWPerfCountersIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psRGXGetConfiguredHWPerfCountersOUT->eError =
				    PVRSRV_ERROR_OUT_OF_MEMORY;
				goto RGXGetConfiguredHWPerfCounters_exit;
			}
		}
	}

	if (IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset) != NULL)
	{
		psConfiguredCountersInt =
		    (RGX_HWPERF_CONFIG_CNTBLK *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset += 1 * sizeof(RGX_HWPERF_CONFIG_CNTBLK);
	}

	psRGXGetConfiguredHWPerfCountersOUT->eError =
	    PVRSRVRGXGetConfiguredHWPerfCountersKM(psConnection, OSGetDevNode(psConnection),
						   psRGXGetConfiguredHWPerfCountersIN->ui32BlockID,
						   psConfiguredCountersInt);
	/* Exit early if bridged call fails */
	if (unlikely(psRGXGetConfiguredHWPerfCountersOUT->eError != PVRSRV_OK))
	{
		goto RGXGetConfiguredHWPerfCounters_exit;
	}

	/* If dest ptr is non-null and we have data to copy */
	if ((psConfiguredCountersInt) && ((1 * sizeof(RGX_HWPERF_CONFIG_CNTBLK)) > 0))
	{
		if (unlikely
		    (OSCopyToUser
		     (NULL,
		      (void __user *)psRGXGetConfiguredHWPerfCountersOUT->psConfiguredCounters,
		      psConfiguredCountersInt,
		      (1 * sizeof(RGX_HWPERF_CONFIG_CNTBLK))) != PVRSRV_OK))
		{
			psRGXGetConfiguredHWPerfCountersOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto RGXGetConfiguredHWPerfCounters_exit;
		}
	}

RGXGetConfiguredHWPerfCounters_exit:

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psRGXGetConfiguredHWPerfCountersOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static IMG_INT
PVRSRVBridgeRGXGetEnabledHWPerfBlocks(IMG_UINT32 ui32DispatchTableEntry,
				      IMG_UINT8 * psRGXGetEnabledHWPerfBlocksIN_UI8,
				      IMG_UINT8 * psRGXGetEnabledHWPerfBlocksOUT_UI8,
				      CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_RGXGETENABLEDHWPERFBLOCKS *psRGXGetEnabledHWPerfBlocksIN =
	    (PVRSRV_BRIDGE_IN_RGXGETENABLEDHWPERFBLOCKS *)
	    IMG_OFFSET_ADDR(psRGXGetEnabledHWPerfBlocksIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_RGXGETENABLEDHWPERFBLOCKS *psRGXGetEnabledHWPerfBlocksOUT =
	    (PVRSRV_BRIDGE_OUT_RGXGETENABLEDHWPERFBLOCKS *)
	    IMG_OFFSET_ADDR(psRGXGetEnabledHWPerfBlocksOUT_UI8, 0);

	IMG_UINT32 *pui32EnabledBlockIDsInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psRGXGetEnabledHWPerfBlocksIN->ui32ArrayLen * sizeof(IMG_UINT32)) + 0;

	psRGXGetEnabledHWPerfBlocksOUT->pui32EnabledBlockIDs =
	    psRGXGetEnabledHWPerfBlocksIN->pui32EnabledBlockIDs;

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psRGXGetEnabledHWPerfBlocksOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto RGXGetEnabledHWPerfBlocks_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psRGXGetEnabledHWPerfBlocksIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer = (IMG_BYTE *) (void *)psRGXGetEnabledHWPerfBlocksIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psRGXGetEnabledHWPerfBlocksOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto RGXGetEnabledHWPerfBlocks_exit;
			}
		}
	}

	if (psRGXGetEnabledHWPerfBlocksIN->ui32ArrayLen != 0)
	{
		pui32EnabledBlockIDsInt =
		    (IMG_UINT32 *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset += psRGXGetEnabledHWPerfBlocksIN->ui32ArrayLen * sizeof(IMG_UINT32);
	}

	psRGXGetEnabledHWPerfBlocksOUT->eError =
	    PVRSRVRGXGetEnabledHWPerfBlocksKM(psConnection, OSGetDevNode(psConnection),
					      psRGXGetEnabledHWPerfBlocksIN->ui32ArrayLen,
					      &psRGXGetEnabledHWPerfBlocksOUT->ui32BlockCount,
					      pui32EnabledBlockIDsInt);
	/* Exit early if bridged call fails */
	if (unlikely(psRGXGetEnabledHWPerfBlocksOUT->eError != PVRSRV_OK))
	{
		goto RGXGetEnabledHWPerfBlocks_exit;
	}

	/* If dest ptr is non-null and we have data to copy */
	if ((pui32EnabledBlockIDsInt) &&
	    ((psRGXGetEnabledHWPerfBlocksIN->ui32ArrayLen * sizeof(IMG_UINT32)) > 0))
	{
		if (unlikely
		    (OSCopyToUser
		     (NULL, (void __user *)psRGXGetEnabledHWPerfBlocksOUT->pui32EnabledBlockIDs,
		      pui32EnabledBlockIDsInt,
		      (psRGXGetEnabledHWPerfBlocksIN->ui32ArrayLen * sizeof(IMG_UINT32))) !=
		     PVRSRV_OK))
		{
			psRGXGetEnabledHWPerfBlocksOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto RGXGetEnabledHWPerfBlocks_exit;
		}
	}

RGXGetEnabledHWPerfBlocks_exit:

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psRGXGetEnabledHWPerfBlocksOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

#if defined(PVRSRV_FORCE_HWPERF_TO_SCHED_CLK)

static IMG_INT
PVRSRVBridgeRGXGetHWPerfTimeStamp(IMG_UINT32 ui32DispatchTableEntry,
				  IMG_UINT8 * psRGXGetHWPerfTimeStampIN_UI8,
				  IMG_UINT8 * psRGXGetHWPerfTimeStampOUT_UI8,
				  CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_RGXGETHWPERFTIMESTAMP *psRGXGetHWPerfTimeStampIN =
	    (PVRSRV_BRIDGE_IN_RGXGETHWPERFTIMESTAMP *)
	    IMG_OFFSET_ADDR(psRGXGetHWPerfTimeStampIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_RGXGETHWPERFTIMESTAMP *psRGXGetHWPerfTimeStampOUT =
	    (PVRSRV_BRIDGE_OUT_RGXGETHWPERFTIMESTAMP *)
	    IMG_OFFSET_ADDR(psRGXGetHWPerfTimeStampOUT_UI8, 0);

	PVR_UNREFERENCED_PARAMETER(psRGXGetHWPerfTimeStampIN);

	psRGXGetHWPerfTimeStampOUT->eError =
	    PVRSRVRGXGetHWPerfTimeStampKM(psConnection, OSGetDevNode(psConnection),
					  &psRGXGetHWPerfTimeStampOUT->ui64TimeStamp);

	return 0;
}

#else
#define PVRSRVBridgeRGXGetHWPerfTimeStamp NULL
#endif

static IMG_INT
PVRSRVBridgeRGXCtrlHWPerf(IMG_UINT32 ui32DispatchTableEntry,
			  IMG_UINT8 * psRGXCtrlHWPerfIN_UI8,
			  IMG_UINT8 * psRGXCtrlHWPerfOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_RGXCTRLHWPERF *psRGXCtrlHWPerfIN =
	    (PVRSRV_BRIDGE_IN_RGXCTRLHWPERF *) IMG_OFFSET_ADDR(psRGXCtrlHWPerfIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_RGXCTRLHWPERF *psRGXCtrlHWPerfOUT =
	    (PVRSRV_BRIDGE_OUT_RGXCTRLHWPERF *) IMG_OFFSET_ADDR(psRGXCtrlHWPerfOUT_UI8, 0);

	psRGXCtrlHWPerfOUT->eError =
	    PVRSRVRGXCtrlHWPerfKM(psConnection, OSGetDevNode(psConnection),
				  psRGXCtrlHWPerfIN->ui32StreamId,
				  psRGXCtrlHWPerfIN->bToggle, psRGXCtrlHWPerfIN->ui64Mask);

	return 0;
}

static IMG_INT
PVRSRVBridgeRGXGetHWPerfBvncFeatureFlags(IMG_UINT32 ui32DispatchTableEntry,
					 IMG_UINT8 * psRGXGetHWPerfBvncFeatureFlagsIN_UI8,
					 IMG_UINT8 * psRGXGetHWPerfBvncFeatureFlagsOUT_UI8,
					 CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_RGXGETHWPERFBVNCFEATUREFLAGS *psRGXGetHWPerfBvncFeatureFlagsIN =
	    (PVRSRV_BRIDGE_IN_RGXGETHWPERFBVNCFEATUREFLAGS *)
	    IMG_OFFSET_ADDR(psRGXGetHWPerfBvncFeatureFlagsIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_RGXGETHWPERFBVNCFEATUREFLAGS *psRGXGetHWPerfBvncFeatureFlagsOUT =
	    (PVRSRV_BRIDGE_OUT_RGXGETHWPERFBVNCFEATUREFLAGS *)
	    IMG_OFFSET_ADDR(psRGXGetHWPerfBvncFeatureFlagsOUT_UI8, 0);

	PVR_UNREFERENCED_PARAMETER(psRGXGetHWPerfBvncFeatureFlagsIN);

	psRGXGetHWPerfBvncFeatureFlagsOUT->eError =
	    PVRSRVRGXGetHWPerfBvncFeatureFlagsKM(psConnection, OSGetDevNode(psConnection),
						 &psRGXGetHWPerfBvncFeatureFlagsOUT->sBVNC);

	return 0;
}

static_assert(RGXFWIF_HWPERF_CTRL_BLKS_MAX <= IMG_UINT32_MAX,
	      "RGXFWIF_HWPERF_CTRL_BLKS_MAX must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeRGXControlHWPerfBlocks(IMG_UINT32 ui32DispatchTableEntry,
				   IMG_UINT8 * psRGXControlHWPerfBlocksIN_UI8,
				   IMG_UINT8 * psRGXControlHWPerfBlocksOUT_UI8,
				   CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_RGXCONTROLHWPERFBLOCKS *psRGXControlHWPerfBlocksIN =
	    (PVRSRV_BRIDGE_IN_RGXCONTROLHWPERFBLOCKS *)
	    IMG_OFFSET_ADDR(psRGXControlHWPerfBlocksIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_RGXCONTROLHWPERFBLOCKS *psRGXControlHWPerfBlocksOUT =
	    (PVRSRV_BRIDGE_OUT_RGXCONTROLHWPERFBLOCKS *)
	    IMG_OFFSET_ADDR(psRGXControlHWPerfBlocksOUT_UI8, 0);

	IMG_UINT16 *ui16BlockIDsInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psRGXControlHWPerfBlocksIN->ui32ArrayLen * sizeof(IMG_UINT16)) + 0;

	if (unlikely(psRGXControlHWPerfBlocksIN->ui32ArrayLen > RGXFWIF_HWPERF_CTRL_BLKS_MAX))
	{
		psRGXControlHWPerfBlocksOUT->eError = PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto RGXControlHWPerfBlocks_exit;
	}

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psRGXControlHWPerfBlocksOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto RGXControlHWPerfBlocks_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psRGXControlHWPerfBlocksIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer = (IMG_BYTE *) (void *)psRGXControlHWPerfBlocksIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psRGXControlHWPerfBlocksOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto RGXControlHWPerfBlocks_exit;
			}
		}
	}

	if (psRGXControlHWPerfBlocksIN->ui32ArrayLen != 0)
	{
		ui16BlockIDsInt = (IMG_UINT16 *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset += psRGXControlHWPerfBlocksIN->ui32ArrayLen * sizeof(IMG_UINT16);
	}

	/* Copy the data over */
	if (psRGXControlHWPerfBlocksIN->ui32ArrayLen * sizeof(IMG_UINT16) > 0)
	{
		if (OSCopyFromUser
		    (NULL, ui16BlockIDsInt,
		     (const void __user *)psRGXControlHWPerfBlocksIN->pui16BlockIDs,
		     psRGXControlHWPerfBlocksIN->ui32ArrayLen * sizeof(IMG_UINT16)) != PVRSRV_OK)
		{
			psRGXControlHWPerfBlocksOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto RGXControlHWPerfBlocks_exit;
		}
	}

	psRGXControlHWPerfBlocksOUT->eError =
	    PVRSRVRGXControlHWPerfBlocksKM(psConnection, OSGetDevNode(psConnection),
					   psRGXControlHWPerfBlocksIN->bEnable,
					   psRGXControlHWPerfBlocksIN->ui32ArrayLen,
					   ui16BlockIDsInt);

RGXControlHWPerfBlocks_exit:

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psRGXControlHWPerfBlocksOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static_assert(RGXFWIF_HWPERF_CTRL_BLKS_MAX <= IMG_UINT32_MAX,
	      "RGXFWIF_HWPERF_CTRL_BLKS_MAX must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeRGXConfigMuxHWPerfCounters(IMG_UINT32 ui32DispatchTableEntry,
				       IMG_UINT8 * psRGXConfigMuxHWPerfCountersIN_UI8,
				       IMG_UINT8 * psRGXConfigMuxHWPerfCountersOUT_UI8,
				       CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_RGXCONFIGMUXHWPERFCOUNTERS *psRGXConfigMuxHWPerfCountersIN =
	    (PVRSRV_BRIDGE_IN_RGXCONFIGMUXHWPERFCOUNTERS *)
	    IMG_OFFSET_ADDR(psRGXConfigMuxHWPerfCountersIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_RGXCONFIGMUXHWPERFCOUNTERS *psRGXConfigMuxHWPerfCountersOUT =
	    (PVRSRV_BRIDGE_OUT_RGXCONFIGMUXHWPERFCOUNTERS *)
	    IMG_OFFSET_ADDR(psRGXConfigMuxHWPerfCountersOUT_UI8, 0);

	RGX_HWPERF_CONFIG_MUX_CNTBLK *psBlockConfigsInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psRGXConfigMuxHWPerfCountersIN->ui32ArrayLen *
	     sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK)) + 0;

	if (unlikely(psRGXConfigMuxHWPerfCountersIN->ui32ArrayLen > RGXFWIF_HWPERF_CTRL_BLKS_MAX))
	{
		psRGXConfigMuxHWPerfCountersOUT->eError = PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto RGXConfigMuxHWPerfCounters_exit;
	}

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psRGXConfigMuxHWPerfCountersOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto RGXConfigMuxHWPerfCounters_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psRGXConfigMuxHWPerfCountersIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer =
			    (IMG_BYTE *) (void *)psRGXConfigMuxHWPerfCountersIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psRGXConfigMuxHWPerfCountersOUT->eError =
				    PVRSRV_ERROR_OUT_OF_MEMORY;
				goto RGXConfigMuxHWPerfCounters_exit;
			}
		}
	}

	if (psRGXConfigMuxHWPerfCountersIN->ui32ArrayLen != 0)
	{
		psBlockConfigsInt =
		    (RGX_HWPERF_CONFIG_MUX_CNTBLK *) IMG_OFFSET_ADDR(pArrayArgsBuffer,
								     ui32NextOffset);
		ui32NextOffset +=
		    psRGXConfigMuxHWPerfCountersIN->ui32ArrayLen *
		    sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK);
	}

	/* Copy the data over */
	if (psRGXConfigMuxHWPerfCountersIN->ui32ArrayLen * sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK) > 0)
	{
		if (OSCopyFromUser
		    (NULL, psBlockConfigsInt,
		     (const void __user *)psRGXConfigMuxHWPerfCountersIN->psBlockConfigs,
		     psRGXConfigMuxHWPerfCountersIN->ui32ArrayLen *
		     sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK)) != PVRSRV_OK)
		{
			psRGXConfigMuxHWPerfCountersOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto RGXConfigMuxHWPerfCounters_exit;
		}
	}

	psRGXConfigMuxHWPerfCountersOUT->eError =
	    PVRSRVRGXConfigMuxHWPerfCountersKM(psConnection, OSGetDevNode(psConnection),
					       psRGXConfigMuxHWPerfCountersIN->ui32ArrayLen,
					       psBlockConfigsInt);

RGXConfigMuxHWPerfCounters_exit:

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psRGXConfigMuxHWPerfCountersOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static_assert(RGX_HWPERF_MAX_CUSTOM_CNTRS <= IMG_UINT32_MAX,
	      "RGX_HWPERF_MAX_CUSTOM_CNTRS must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeRGXConfigCustomCounters(IMG_UINT32 ui32DispatchTableEntry,
				    IMG_UINT8 * psRGXConfigCustomCountersIN_UI8,
				    IMG_UINT8 * psRGXConfigCustomCountersOUT_UI8,
				    CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_RGXCONFIGCUSTOMCOUNTERS *psRGXConfigCustomCountersIN =
	    (PVRSRV_BRIDGE_IN_RGXCONFIGCUSTOMCOUNTERS *)
	    IMG_OFFSET_ADDR(psRGXConfigCustomCountersIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_RGXCONFIGCUSTOMCOUNTERS *psRGXConfigCustomCountersOUT =
	    (PVRSRV_BRIDGE_OUT_RGXCONFIGCUSTOMCOUNTERS *)
	    IMG_OFFSET_ADDR(psRGXConfigCustomCountersOUT_UI8, 0);

	IMG_UINT32 *ui32CustomCounterIDsInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psRGXConfigCustomCountersIN->ui16NumCustomCounters * sizeof(IMG_UINT32)) +
	    0;

	if (unlikely
	    (psRGXConfigCustomCountersIN->ui16NumCustomCounters > RGX_HWPERF_MAX_CUSTOM_CNTRS))
	{
		psRGXConfigCustomCountersOUT->eError = PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto RGXConfigCustomCounters_exit;
	}

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psRGXConfigCustomCountersOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto RGXConfigCustomCounters_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psRGXConfigCustomCountersIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer = (IMG_BYTE *) (void *)psRGXConfigCustomCountersIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psRGXConfigCustomCountersOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto RGXConfigCustomCounters_exit;
			}
		}
	}

	if (psRGXConfigCustomCountersIN->ui16NumCustomCounters != 0)
	{
		ui32CustomCounterIDsInt =
		    (IMG_UINT32 *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset +=
		    psRGXConfigCustomCountersIN->ui16NumCustomCounters * sizeof(IMG_UINT32);
	}

	/* Copy the data over */
	if (psRGXConfigCustomCountersIN->ui16NumCustomCounters * sizeof(IMG_UINT32) > 0)
	{
		if (OSCopyFromUser
		    (NULL, ui32CustomCounterIDsInt,
		     (const void __user *)psRGXConfigCustomCountersIN->pui32CustomCounterIDs,
		     psRGXConfigCustomCountersIN->ui16NumCustomCounters * sizeof(IMG_UINT32)) !=
		    PVRSRV_OK)
		{
			psRGXConfigCustomCountersOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto RGXConfigCustomCounters_exit;
		}
	}

	psRGXConfigCustomCountersOUT->eError =
	    PVRSRVRGXConfigCustomCountersKM(psConnection, OSGetDevNode(psConnection),
					    psRGXConfigCustomCountersIN->ui16CustomBlockID,
					    psRGXConfigCustomCountersIN->ui16NumCustomCounters,
					    ui32CustomCounterIDsInt);

RGXConfigCustomCounters_exit:

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psRGXConfigCustomCountersOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static_assert(RGXFWIF_HWPERF_CTRL_BLKS_MAX <= IMG_UINT32_MAX,
	      "RGXFWIF_HWPERF_CTRL_BLKS_MAX must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeRGXConfigureHWPerfBlocks(IMG_UINT32 ui32DispatchTableEntry,
				     IMG_UINT8 * psRGXConfigureHWPerfBlocksIN_UI8,
				     IMG_UINT8 * psRGXConfigureHWPerfBlocksOUT_UI8,
				     CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_RGXCONFIGUREHWPERFBLOCKS *psRGXConfigureHWPerfBlocksIN =
	    (PVRSRV_BRIDGE_IN_RGXCONFIGUREHWPERFBLOCKS *)
	    IMG_OFFSET_ADDR(psRGXConfigureHWPerfBlocksIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_RGXCONFIGUREHWPERFBLOCKS *psRGXConfigureHWPerfBlocksOUT =
	    (PVRSRV_BRIDGE_OUT_RGXCONFIGUREHWPERFBLOCKS *)
	    IMG_OFFSET_ADDR(psRGXConfigureHWPerfBlocksOUT_UI8, 0);

	RGX_HWPERF_CONFIG_CNTBLK *psBlockConfigsInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psRGXConfigureHWPerfBlocksIN->ui32ArrayLen *
	     sizeof(RGX_HWPERF_CONFIG_CNTBLK)) + 0;

	if (unlikely(psRGXConfigureHWPerfBlocksIN->ui32ArrayLen > RGXFWIF_HWPERF_CTRL_BLKS_MAX))
	{
		psRGXConfigureHWPerfBlocksOUT->eError = PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto RGXConfigureHWPerfBlocks_exit;
	}

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psRGXConfigureHWPerfBlocksOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto RGXConfigureHWPerfBlocks_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psRGXConfigureHWPerfBlocksIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer = (IMG_BYTE *) (void *)psRGXConfigureHWPerfBlocksIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psRGXConfigureHWPerfBlocksOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto RGXConfigureHWPerfBlocks_exit;
			}
		}
	}

	if (psRGXConfigureHWPerfBlocksIN->ui32ArrayLen != 0)
	{
		psBlockConfigsInt =
		    (RGX_HWPERF_CONFIG_CNTBLK *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset +=
		    psRGXConfigureHWPerfBlocksIN->ui32ArrayLen * sizeof(RGX_HWPERF_CONFIG_CNTBLK);
	}

	/* Copy the data over */
	if (psRGXConfigureHWPerfBlocksIN->ui32ArrayLen * sizeof(RGX_HWPERF_CONFIG_CNTBLK) > 0)
	{
		if (OSCopyFromUser
		    (NULL, psBlockConfigsInt,
		     (const void __user *)psRGXConfigureHWPerfBlocksIN->psBlockConfigs,
		     psRGXConfigureHWPerfBlocksIN->ui32ArrayLen *
		     sizeof(RGX_HWPERF_CONFIG_CNTBLK)) != PVRSRV_OK)
		{
			psRGXConfigureHWPerfBlocksOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto RGXConfigureHWPerfBlocks_exit;
		}
	}

	psRGXConfigureHWPerfBlocksOUT->eError =
	    PVRSRVRGXConfigureHWPerfBlocksKM(psConnection, OSGetDevNode(psConnection),
					     psRGXConfigureHWPerfBlocksIN->ui32CtrlWord,
					     psRGXConfigureHWPerfBlocksIN->ui32ArrayLen,
					     psBlockConfigsInt);

RGXConfigureHWPerfBlocks_exit:

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psRGXConfigureHWPerfBlocksOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static_assert(1 <= IMG_UINT32_MAX, "1 must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeRGXGetConfiguredHWPerfMuxCounters(IMG_UINT32 ui32DispatchTableEntry,
					      IMG_UINT8 * psRGXGetConfiguredHWPerfMuxCountersIN_UI8,
					      IMG_UINT8 *
					      psRGXGetConfiguredHWPerfMuxCountersOUT_UI8,
					      CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_RGXGETCONFIGUREDHWPERFMUXCOUNTERS *psRGXGetConfiguredHWPerfMuxCountersIN =
	    (PVRSRV_BRIDGE_IN_RGXGETCONFIGUREDHWPERFMUXCOUNTERS *)
	    IMG_OFFSET_ADDR(psRGXGetConfiguredHWPerfMuxCountersIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_RGXGETCONFIGUREDHWPERFMUXCOUNTERS *psRGXGetConfiguredHWPerfMuxCountersOUT
	    =
	    (PVRSRV_BRIDGE_OUT_RGXGETCONFIGUREDHWPERFMUXCOUNTERS *)
	    IMG_OFFSET_ADDR(psRGXGetConfiguredHWPerfMuxCountersOUT_UI8, 0);

	RGX_HWPERF_CONFIG_MUX_CNTBLK *psConfiguredMuxCountersInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize = ((IMG_UINT64) 1 * sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK)) + 0;

	psRGXGetConfiguredHWPerfMuxCountersOUT->psConfiguredMuxCounters =
	    psRGXGetConfiguredHWPerfMuxCountersIN->psConfiguredMuxCounters;

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psRGXGetConfiguredHWPerfMuxCountersOUT->eError =
		    PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto RGXGetConfiguredHWPerfMuxCounters_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psRGXGetConfiguredHWPerfMuxCountersIN),
			      sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer =
			    (IMG_BYTE *) (void *)psRGXGetConfiguredHWPerfMuxCountersIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psRGXGetConfiguredHWPerfMuxCountersOUT->eError =
				    PVRSRV_ERROR_OUT_OF_MEMORY;
				goto RGXGetConfiguredHWPerfMuxCounters_exit;
			}
		}
	}

	if (IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset) != NULL)
	{
		psConfiguredMuxCountersInt =
		    (RGX_HWPERF_CONFIG_MUX_CNTBLK *) IMG_OFFSET_ADDR(pArrayArgsBuffer,
								     ui32NextOffset);
		ui32NextOffset += 1 * sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK);
	}

	psRGXGetConfiguredHWPerfMuxCountersOUT->eError =
	    PVRSRVRGXGetConfiguredHWPerfMuxCountersKM(psConnection, OSGetDevNode(psConnection),
						      psRGXGetConfiguredHWPerfMuxCountersIN->
						      ui32BlockID, psConfiguredMuxCountersInt);
	/* Exit early if bridged call fails */
	if (unlikely(psRGXGetConfiguredHWPerfMuxCountersOUT->eError != PVRSRV_OK))
	{
		goto RGXGetConfiguredHWPerfMuxCounters_exit;
	}

	/* If dest ptr is non-null and we have data to copy */
	if ((psConfiguredMuxCountersInt) && ((1 * sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK)) > 0))
	{
		if (unlikely
		    (OSCopyToUser
		     (NULL,
		      (void __user *)psRGXGetConfiguredHWPerfMuxCountersOUT->
		      psConfiguredMuxCounters, psConfiguredMuxCountersInt,
		      (1 * sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK))) != PVRSRV_OK))
		{
			psRGXGetConfiguredHWPerfMuxCountersOUT->eError =
			    PVRSRV_ERROR_INVALID_PARAMS;

			goto RGXGetConfiguredHWPerfMuxCounters_exit;
		}
	}

RGXGetConfiguredHWPerfMuxCounters_exit:

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psRGXGetConfiguredHWPerfMuxCountersOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

/* ***************************************************************************
 * Server bridge dispatch related glue
 */

PVRSRV_ERROR InitRGXHWPERFBridge(void);
void DeinitRGXHWPERFBridge(void);

/*
 * Register all RGXHWPERF functions with services
 */
PVRSRV_ERROR InitRGXHWPERFBridge(void)
{

	SetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
			      PVRSRV_BRIDGE_RGXHWPERF_RGXGETCONFIGUREDHWPERFCOUNTERS,
			      PVRSRVBridgeRGXGetConfiguredHWPerfCounters, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_RGXGETCONFIGUREDHWPERFCOUNTERS),
			      sizeof(PVRSRV_BRIDGE_OUT_RGXGETCONFIGUREDHWPERFCOUNTERS));

	SetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
			      PVRSRV_BRIDGE_RGXHWPERF_RGXGETENABLEDHWPERFBLOCKS,
			      PVRSRVBridgeRGXGetEnabledHWPerfBlocks, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_RGXGETENABLEDHWPERFBLOCKS),
			      sizeof(PVRSRV_BRIDGE_OUT_RGXGETENABLEDHWPERFBLOCKS));

	SetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
			      PVRSRV_BRIDGE_RGXHWPERF_RGXGETHWPERFTIMESTAMP,
			      PVRSRVBridgeRGXGetHWPerfTimeStamp, NULL, 0,
			      sizeof(PVRSRV_BRIDGE_OUT_RGXGETHWPERFTIMESTAMP));

	SetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF, PVRSRV_BRIDGE_RGXHWPERF_RGXCTRLHWPERF,
			      PVRSRVBridgeRGXCtrlHWPerf, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_RGXCTRLHWPERF),
			      sizeof(PVRSRV_BRIDGE_OUT_RGXCTRLHWPERF));

	SetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
			      PVRSRV_BRIDGE_RGXHWPERF_RGXGETHWPERFBVNCFEATUREFLAGS,
			      PVRSRVBridgeRGXGetHWPerfBvncFeatureFlags, NULL, 0,
			      sizeof(PVRSRV_BRIDGE_OUT_RGXGETHWPERFBVNCFEATUREFLAGS));

	SetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
			      PVRSRV_BRIDGE_RGXHWPERF_RGXCONTROLHWPERFBLOCKS,
			      PVRSRVBridgeRGXControlHWPerfBlocks, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_RGXCONTROLHWPERFBLOCKS),
			      sizeof(PVRSRV_BRIDGE_OUT_RGXCONTROLHWPERFBLOCKS));

	SetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
			      PVRSRV_BRIDGE_RGXHWPERF_RGXCONFIGMUXHWPERFCOUNTERS,
			      PVRSRVBridgeRGXConfigMuxHWPerfCounters, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_RGXCONFIGMUXHWPERFCOUNTERS),
			      sizeof(PVRSRV_BRIDGE_OUT_RGXCONFIGMUXHWPERFCOUNTERS));

	SetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
			      PVRSRV_BRIDGE_RGXHWPERF_RGXCONFIGCUSTOMCOUNTERS,
			      PVRSRVBridgeRGXConfigCustomCounters, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_RGXCONFIGCUSTOMCOUNTERS),
			      sizeof(PVRSRV_BRIDGE_OUT_RGXCONFIGCUSTOMCOUNTERS));

	SetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
			      PVRSRV_BRIDGE_RGXHWPERF_RGXCONFIGUREHWPERFBLOCKS,
			      PVRSRVBridgeRGXConfigureHWPerfBlocks, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_RGXCONFIGUREHWPERFBLOCKS),
			      sizeof(PVRSRV_BRIDGE_OUT_RGXCONFIGUREHWPERFBLOCKS));

	SetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
			      PVRSRV_BRIDGE_RGXHWPERF_RGXGETCONFIGUREDHWPERFMUXCOUNTERS,
			      PVRSRVBridgeRGXGetConfiguredHWPerfMuxCounters, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_RGXGETCONFIGUREDHWPERFMUXCOUNTERS),
			      sizeof(PVRSRV_BRIDGE_OUT_RGXGETCONFIGUREDHWPERFMUXCOUNTERS));

	return PVRSRV_OK;
}

/*
 * Unregister all rgxhwperf functions with services
 */
void DeinitRGXHWPERFBridge(void)
{

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
				PVRSRV_BRIDGE_RGXHWPERF_RGXGETCONFIGUREDHWPERFCOUNTERS);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
				PVRSRV_BRIDGE_RGXHWPERF_RGXGETENABLEDHWPERFBLOCKS);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
				PVRSRV_BRIDGE_RGXHWPERF_RGXGETHWPERFTIMESTAMP);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF, PVRSRV_BRIDGE_RGXHWPERF_RGXCTRLHWPERF);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
				PVRSRV_BRIDGE_RGXHWPERF_RGXGETHWPERFBVNCFEATUREFLAGS);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
				PVRSRV_BRIDGE_RGXHWPERF_RGXCONTROLHWPERFBLOCKS);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
				PVRSRV_BRIDGE_RGXHWPERF_RGXCONFIGMUXHWPERFCOUNTERS);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
				PVRSRV_BRIDGE_RGXHWPERF_RGXCONFIGCUSTOMCOUNTERS);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
				PVRSRV_BRIDGE_RGXHWPERF_RGXCONFIGUREHWPERFBLOCKS);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_RGXHWPERF,
				PVRSRV_BRIDGE_RGXHWPERF_RGXGETCONFIGUREDHWPERFMUXCOUNTERS);

}
