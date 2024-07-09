/*************************************************************************/ /*!
@File
@Title          RGX HW Performance implementation
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX HW Performance implementation
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

//#define PVR_DPF_FUNCTION_TRACE_ON 1
#undef PVR_DPF_FUNCTION_TRACE_ON

#include "img_defs.h"
#include "pvr_debug.h"
#include "rgxdevice.h"
#include "pvrsrv_error.h"
#include "pvr_notifier.h"
#include "osfunc.h"
#include "allocmem.h"

#include "pvrsrv.h"
#include "pvrsrv_tlstreams.h"
#include "pvrsrv_tlcommon.h"
#include "tlclient.h"
#include "tlstream.h"

#include "rgxhwperf.h"
#include "rgxapi_km.h"
#include "rgxfwutils.h"
#include "rgxtimecorr.h"
#include "devicemem.h"
#include "devicemem_pdump.h"
#include "pdump_km.h"
#include "pvrsrv_apphint.h"
#include "process_stats.h"
#include "rgx_hwperf_table.h"
#include "rgxinit.h"

#include "info_page_defs.h"

/* Maximum enum value to prevent access to RGX_HWPERF_STREAM_ID2_CLIENT stream */
#define RGX_HWPERF_MAX_STREAM_ID (RGX_HWPERF_STREAM_ID2_CLIENT)


IMG_INTERNAL /*static inline*/ IMG_UINT32 RGXGetHWPerfBlockConfig(const RGXFW_HWPERF_CNTBLK_TYPE_MODEL **);

static IMG_BOOL RGXServerFeatureFlagsToHWPerfFlagsAddBlock(
	RGX_HWPERF_BVNC_BLOCK	* const psBlocks,
	IMG_UINT16				* const pui16Count,
	const IMG_UINT16		ui16BlockID, /* see RGX_HWPERF_CNTBLK_ID */
	const IMG_UINT16		ui16NumCounters,
	const IMG_UINT16		ui16NumBlocks)
{
	const IMG_UINT16 ui16Count = *pui16Count;

	if (ui16Count < RGX_HWPERF_MAX_BVNC_BLOCK_LEN)
	{
		RGX_HWPERF_BVNC_BLOCK * const psBlock = &psBlocks[ui16Count];

		/* If the GROUP is non-zero, convert from e.g. RGX_CNTBLK_ID_USC0 to
		 * RGX_CNTBLK_ID_USC_ALL. The table stores the former (plus the
		 * number of blocks and counters) but PVRScopeServices expects the
		 * latter (plus the number of blocks and counters). The conversion
		 * could always be moved to PVRScopeServices, but it's less code this
		 * way.
		 * For SLC0 we generate a single SLCBANK_ALL which has NUM_MEMBUS
		 * instances.
		 * This replaces the SLC0 .. SLC3 entries.
		 */
		if ((ui16BlockID == RGX_CNTBLK_ID_SLCBANK0) || (ui16BlockID & RGX_CNTBLK_ID_GROUP_MASK))
		{
			psBlock->ui16BlockID	= ui16BlockID | RGX_CNTBLK_ID_UNIT_ALL_MASK;
		}
		else
		{
			psBlock->ui16BlockID	= ui16BlockID;
		}
		psBlock->ui16NumCounters	= ui16NumCounters;
		psBlock->ui16NumBlocks		= ui16NumBlocks;

		*pui16Count = ui16Count + 1;
		return IMG_TRUE;
	}
	return IMG_FALSE;
}

PVRSRV_ERROR RGXServerFeatureFlagsToHWPerfFlags(PVRSRV_RGXDEV_INFO *psDevInfo, RGX_HWPERF_BVNC *psBVNC)
{
	IMG_PCHAR pszBVNC;
	PVR_LOG_RETURN_IF_FALSE((NULL != psDevInfo), "psDevInfo invalid", PVRSRV_ERROR_INVALID_PARAMS);

	if ((pszBVNC = RGXDevBVNCString(psDevInfo)))
	{
		size_t uiStringLength = OSStringNLength(pszBVNC, RGX_HWPERF_MAX_BVNC_LEN - 1);
		OSStringLCopy(psBVNC->aszBvncString, pszBVNC, uiStringLength + 1);
		memset(&psBVNC->aszBvncString[uiStringLength], 0, RGX_HWPERF_MAX_BVNC_LEN - uiStringLength);
	}
	else
	{
		*psBVNC->aszBvncString = 0;
	}

	psBVNC->ui32BvncKmFeatureFlags = 0x0;

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, PERFBUS))
	{
		psBVNC->ui32BvncKmFeatureFlags |= RGX_HWPERF_FEATURE_PERFBUS_FLAG;
	}

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, GPU_MULTICORE_SUPPORT))
	{
		psBVNC->ui32BvncKmFeatureFlags |= RGX_HWPERF_FEATURE_MULTICORE_FLAG;
	}

#ifdef SUPPORT_WORKLOAD_ESTIMATION
	/* Not a part of BVNC feature line and so doesn't need the feature supported check */
	psBVNC->ui32BvncKmFeatureFlags |= RGX_HWPERF_FEATURE_WORKLOAD_ESTIMATION;
#endif

	/* Define the HW counter block counts. */
	{
		RGX_HWPERF_BVNC_BLOCK					* const psBlocks	= psBVNC->aBvncBlocks;
		IMG_UINT16								* const pui16Count	= &psBVNC->ui16BvncBlocks;
		const RGXFW_HWPERF_CNTBLK_TYPE_MODEL	*asCntBlkTypeModel;
		const IMG_UINT32						ui32CntBlkModelLen	= RGXGetHWPerfBlockConfig(&asCntBlkTypeModel);
		IMG_UINT32								ui32BlkCfgIdx;
		size_t									uiCount;
		IMG_BOOL								bOk					= IMG_TRUE;

		// Initialise to zero blocks
		*pui16Count = 0;

		// Add all the blocks
		for (ui32BlkCfgIdx = 0; ui32BlkCfgIdx < ui32CntBlkModelLen; ui32BlkCfgIdx++)
		{
			const RGXFW_HWPERF_CNTBLK_TYPE_MODEL	* const psCntBlkInfo = &asCntBlkTypeModel[ui32BlkCfgIdx];
			RGX_HWPERF_CNTBLK_RT_INFO				sCntBlkRtInfo;
			/* psCntBlkInfo->uiNumUnits gives compile-time info. For BVNC agnosticism, we use this: */
			if (psCntBlkInfo->pfnIsBlkPresent(psCntBlkInfo, psDevInfo, &sCntBlkRtInfo))
			{
				IMG_UINT32	uiNumUnits;

				switch (psCntBlkInfo->uiCntBlkIdBase)
				{
					case RGX_CNTBLK_ID_SLCBANK0:
						/* Generate the SLCBANK_ALL block for SLC0..SLC3
						 * we have to special-case this as the iteration will
						 * generate entries starting at SLC0 and we need to
						 * defer until we are processing the last 'present'
						 * entry.
						 * The SLC_BLKID_ALL is keyed from SLC0. Need to access
						 * the NUM_MEMBUS feature to see how many are physically
						 * present.
						 */
						uiNumUnits = RGX_GET_FEATURE_VALUE(psDevInfo, NUM_MEMBUS);
						break;
					case RGX_CNTBLK_ID_SLCBANK1:
					case RGX_CNTBLK_ID_SLCBANK2:
					case RGX_CNTBLK_ID_SLCBANK3:
						/* These are contained within SLCBANK_ALL block */
						continue;
					default:
						uiNumUnits = sCntBlkRtInfo.uiNumUnits;
						break;
				}
				bOk &= RGXServerFeatureFlagsToHWPerfFlagsAddBlock(psBlocks, pui16Count, psCntBlkInfo->uiCntBlkIdBase, RGX_CNTBLK_COUNTERS_MAX, uiNumUnits);
			}
		}

		/* If this fails, consider why the static_assert didn't fail, and consider increasing RGX_HWPERF_MAX_BVNC_BLOCK_LEN */
		PVR_ASSERT(bOk);

		// Zero the remaining entries
		uiCount = *pui16Count;
		OSDeviceMemSet(&psBlocks[uiCount], 0, (RGX_HWPERF_MAX_BVNC_BLOCK_LEN - uiCount) * sizeof(*psBlocks));
	}

	return PVRSRV_OK;
}

/*
	PVRSRVRGXConfigureHWPerfBlocksKM
 */
PVRSRV_ERROR PVRSRVRGXConfigureHWPerfBlocksKM(
		CONNECTION_DATA          * psConnection,
		PVRSRV_DEVICE_NODE       * psDeviceNode,
		IMG_UINT32                 ui32CtrlWord,
		IMG_UINT32                 ui32ArrayLen,
		RGX_HWPERF_CONFIG_CNTBLK * psBlockConfigs)
{
	PVRSRV_ERROR		eError = PVRSRV_OK;
	RGXFWIF_KCCB_CMD	sKccbCmd;
	DEVMEM_MEMDESC*		psFwBlkConfigsMemDesc;
	RGX_HWPERF_CONFIG_CNTBLK* psFwArray;
	IMG_UINT32			ui32kCCBCommandSlot;
	PVRSRV_RGXDEV_INFO	*psDevice;

	PVR_LOG_RETURN_IF_FALSE(psDeviceNode != NULL, "psDeviceNode is NULL",
	                        PVRSRV_ERROR_INVALID_PARAMS);

	psDevice = psDeviceNode->pvDevice;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	PVRSRV_VZ_RET_IF_MODE(GUEST, PVRSRV_ERROR_NOT_IMPLEMENTED);

	PVR_LOG_RETURN_IF_FALSE(ui32ArrayLen > 0, "ui32ArrayLen is 0",
	                        PVRSRV_ERROR_INVALID_PARAMS);
	PVR_LOG_RETURN_IF_FALSE(psBlockConfigs != NULL, "psBlockConfigs is NULL",
	                        PVRSRV_ERROR_INVALID_PARAMS);

	PVR_DPF_ENTERED;

	/* Fill in the command structure with the parameters needed
	 */
	sKccbCmd.eCmdType = RGXFWIF_KCCB_CMD_HWPERF_CONFIG_BLKS;
	sKccbCmd.uCmdData.sHWPerfCfgEnableBlks.ui32CtrlWord = ui32CtrlWord;
	sKccbCmd.uCmdData.sHWPerfCfgEnableBlks.ui32NumBlocks = ui32ArrayLen;

	eError = DevmemFwAllocate(psDevice,
	                          sizeof(RGX_HWPERF_CONFIG_CNTBLK)*ui32ArrayLen,
	                          PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) |
	                          PVRSRV_MEMALLOCFLAG_GPU_READABLE |
	                          PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE |
	                          PVRSRV_MEMALLOCFLAG_CPU_READABLE |
	                          PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE |
	                          PVRSRV_MEMALLOCFLAG_UNCACHED |
	                          PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC |
	                          PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN),
	                          "FwHWPerfCountersConfigBlock",
	                          &psFwBlkConfigsMemDesc);
	PVR_LOG_RETURN_IF_ERROR(eError, "DevmemFwAllocate");

	eError = RGXSetFirmwareAddress(&sKccbCmd.uCmdData.sHWPerfCfgEnableBlks.sBlockConfigs,
	                      psFwBlkConfigsMemDesc, 0, RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress", fail1);

	eError = DevmemAcquireCpuVirtAddr(psFwBlkConfigsMemDesc, (void **)&psFwArray);
	PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireCpuVirtAddr", fail2);

	OSDeviceMemCopy(psFwArray, psBlockConfigs, sizeof(RGX_HWPERF_CONFIG_CNTBLK)*ui32ArrayLen);
	DevmemPDumpLoadMem(psFwBlkConfigsMemDesc,
	                   0,
	                   sizeof(RGX_HWPERF_CONFIG_CNTBLK)*ui32ArrayLen,
	                   PDUMP_FLAGS_CONTINUOUS);

	/* Ask the FW to carry out the HWPerf configuration command
	 */
	eError = RGXScheduleCommandAndGetKCCBSlot(psDevice,
											  RGXFWIF_DM_GP,
											  &sKccbCmd,
											  0,
											  PDUMP_FLAGS_CONTINUOUS,
											  &ui32kCCBCommandSlot);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXScheduleCommandAndGetKCCBSlot", fail2);

	/* Wait for FW to complete */
	eError = RGXWaitForKCCBSlotUpdate(psDevice, ui32kCCBCommandSlot, PDUMP_FLAGS_CONTINUOUS);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXWaitForKCCBSlotUpdate", fail3);

	/* Release temporary memory used for block configuration
	 */
	RGXUnsetFirmwareAddress(psFwBlkConfigsMemDesc);
	DevmemReleaseCpuVirtAddr(psFwBlkConfigsMemDesc);
	DevmemFwUnmapAndFree(psDevice, psFwBlkConfigsMemDesc);

	PVR_DPF((PVR_DBG_WARNING, "HWPerf %d counter blocks configured and ENABLED", ui32ArrayLen));

	PVR_DPF_RETURN_OK;

fail3:
	DevmemReleaseCpuVirtAddr(psFwBlkConfigsMemDesc);
fail2:
	RGXUnsetFirmwareAddress(psFwBlkConfigsMemDesc);
fail1:
	DevmemFwUnmapAndFree(psDevice, psFwBlkConfigsMemDesc);

	PVR_DPF_RETURN_RC(eError);
}

/******************************************************************************
 * Currently only implemented on Linux. Feature can be enabled to provide
 * an interface to 3rd-party kernel modules that wish to access the
 * HWPerf data. The API is documented in the rgxapi_km.h header and
 * the rgx_hwperf* headers.
 *****************************************************************************/

/* Internal HWPerf kernel connection/device data object to track the state
 * of a client session.
 */
typedef struct
{
	PVRSRV_DEVICE_NODE* psRgxDevNode;
	PVRSRV_RGXDEV_INFO* psRgxDevInfo;

	/* TL Open/close state */
	IMG_HANDLE          hSD[RGX_HWPERF_MAX_STREAM_ID];

	/* TL Acquire/release state */
	IMG_PBYTE			pHwpBuf[RGX_HWPERF_MAX_STREAM_ID];			/*!< buffer returned to user in acquire call */
	IMG_PBYTE			pHwpBufEnd[RGX_HWPERF_MAX_STREAM_ID];		/*!< pointer to end of HwpBuf */
	IMG_PBYTE			pTlBuf[RGX_HWPERF_MAX_STREAM_ID];			/*!< buffer obtained via TlAcquireData */
	IMG_PBYTE			pTlBufPos[RGX_HWPERF_MAX_STREAM_ID];		/*!< initial position in TlBuf to acquire packets */
	IMG_PBYTE			pTlBufRead[RGX_HWPERF_MAX_STREAM_ID];		/*!< pointer to the last packet read */
	IMG_UINT32			ui32AcqDataLen[RGX_HWPERF_MAX_STREAM_ID];	/*!< length of acquired TlBuf */
	IMG_BOOL			bRelease[RGX_HWPERF_MAX_STREAM_ID];		/*!< used to determine whether or not to release currently held TlBuf */


} RGX_KM_HWPERF_DEVDATA;

PVRSRV_ERROR RGXHWPerfConfigureCounters(
		RGX_HWPERF_CONNECTION *psHWPerfConnection,
		IMG_UINT32					ui32CtrlWord,
		IMG_UINT32					ui32NumBlocks,
		RGX_HWPERF_CONFIG_CNTBLK*	asBlockConfigs)
{
	PVRSRV_ERROR           eError;
	RGX_KM_HWPERF_DEVDATA* psDevData;
	RGX_HWPERF_DEVICE *psHWPerfDev;

	PVRSRV_VZ_RET_IF_MODE(GUEST, PVRSRV_ERROR_NOT_IMPLEMENTED);

	/* Validate input argument values supplied by the caller */
	if (!psHWPerfConnection || ui32NumBlocks==0 || !asBlockConfigs)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (ui32NumBlocks > RGXFWIF_HWPERF_CTRL_BLKS_MAX)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psHWPerfDev = psHWPerfConnection->psHWPerfDevList;

	while (psHWPerfDev)
	{
		psDevData = (RGX_KM_HWPERF_DEVDATA *) psHWPerfDev->hDevData;

		/* Call the internal server API */
		eError = PVRSRVRGXConfigureHWPerfBlocksKM(NULL,
		                                          psDevData->psRgxDevNode,
		                                          ui32CtrlWord,
		                                          ui32NumBlocks,
		                                          asBlockConfigs);

		PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVRGXConfigureHWPerfBlocksKM");

		psHWPerfDev = psHWPerfDev->psNext;
	}

	return PVRSRV_OK;
}

/******************************************************************************
 End of file (rgxhwperf.c)
 ******************************************************************************/
