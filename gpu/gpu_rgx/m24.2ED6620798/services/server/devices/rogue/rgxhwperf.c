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

/* This is defined by default to enable producer callbacks.
 * Clients of the TL interface can disable the use of the callback
 * with PVRSRV_STREAM_FLAG_DISABLE_PRODUCER_CALLBACK. */
#define SUPPORT_TL_PRODUCER_CALLBACK 1

/* Maximum enum value to prevent access to RGX_HWPERF_STREAM_ID2_CLIENT stream */
#define RGX_HWPERF_MAX_STREAM_ID (RGX_HWPERF_STREAM_ID2_CLIENT)

/* Defines size of buffers returned from acquire/release calls */
#define FW_STREAM_BUFFER_SIZE (0x80000)
#define HOST_STREAM_BUFFER_SIZE (0x20000)

/* Must be at least as large as two tl packets of maximum size */
static_assert(HOST_STREAM_BUFFER_SIZE >= (PVRSRVTL_MAX_PACKET_SIZE<<1),
              "HOST_STREAM_BUFFER_SIZE is less than (PVRSRVTL_MAX_PACKET_SIZE<<1)");
static_assert(FW_STREAM_BUFFER_SIZE >= (PVRSRVTL_MAX_PACKET_SIZE<<1),
              "FW_STREAM_BUFFER_SIZE is less than (PVRSRVTL_MAX_PACKET_SIZE<<1)");

/******************************************************************************
 * RGX HW Performance Profiling Server API(s)
 *****************************************************************************/

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

		/* If the GROUP is non-zero, convert from e.g. RGX_CNTBLK_ID_USC0 to RGX_CNTBLK_ID_USC_ALL. The table stores the former (plus the
		number of blocks and counters) but PVRScopeServices expects the latter (plus the number of blocks and counters). The conversion
		could always be moved to PVRScopeServices, but it's less code this way. */
		psBlock->ui16BlockID		= (ui16BlockID & RGX_CNTBLK_ID_GROUP_MASK) ? (ui16BlockID | RGX_CNTBLK_ID_UNIT_ALL_MASK) : ui16BlockID;
		if ((ui16BlockID & RGX_CNTBLK_ID_DA_MASK) == RGX_CNTBLK_ID_DA_MASK)
		{
			psBlock->ui16NumCounters	= RGX_CNTBLK_COUNTERS_MAX;
		}
		else
		{
			psBlock->ui16NumCounters	= ui16NumCounters;
		}
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
		OSStringSafeCopy(psBVNC->aszBvncString, pszBVNC, uiStringLength + 1);
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
#if defined(RGX_FEATURE_XT_TOP_INFRASTRUCTURE_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, XT_TOP_INFRASTRUCTURE))
	{
		psBVNC->ui32BvncKmFeatureFlags |= RGX_HWPERF_FEATURE_XT_TOP_INFRASTRUCTURE_FLAG;
	}
#endif
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, PERF_COUNTER_BATCH))
	{
		psBVNC->ui32BvncKmFeatureFlags |= RGX_HWPERF_FEATURE_PERF_COUNTER_BATCH_FLAG;
	}
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, ROGUEXE))
	{
		psBVNC->ui32BvncKmFeatureFlags |= RGX_HWPERF_FEATURE_ROGUEXE_FLAG;
	}
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, PBE2_IN_XE))
	{
		psBVNC->ui32BvncKmFeatureFlags |= RGX_HWPERF_FEATURE_PBE2_IN_XE_FLAG;
	}
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, GPU_MULTICORE_SUPPORT))
	{
		psBVNC->ui32BvncKmFeatureFlags |= RGX_HWPERF_FEATURE_MULTICORE_FLAG;
	}

#ifdef SUPPORT_WORKLOAD_ESTIMATION
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		/* Not a part of BVNC feature line and so doesn't need the feature supported check */
		psBVNC->ui32BvncKmFeatureFlags |= RGX_HWPERF_FEATURE_WORKLOAD_ESTIMATION;
	}
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
			/* psCntBlkInfo->ui8NumUnits gives compile-time info. For BVNC agnosticism, we use this: */
			if (psCntBlkInfo->pfnIsBlkPresent(psCntBlkInfo, psDevInfo, &sCntBlkRtInfo))
			{
				bOk &= RGXServerFeatureFlagsToHWPerfFlagsAddBlock(psBlocks, pui16Count, psCntBlkInfo->ui32CntBlkIdBase, psCntBlkInfo->ui8NumCounters, sCntBlkRtInfo.ui32NumUnits);
			}
		}

		/* If this fails, consider why the static_assert didn't fail, and consider increasing RGX_HWPERF_MAX_BVNC_BLOCK_LEN */
		PVR_ASSERT(bOk);

		// Zero the remaining entries
		uiCount = *pui16Count;
		OSDeviceMemSet(&psBlocks[uiCount], 0, (RGX_HWPERF_MAX_BVNC_BLOCK_LEN - uiCount) * sizeof(*psBlocks));
	}

	/* The GPU core count is overwritten by the FW */
	psBVNC->ui16BvncGPUCores = 0;

	return PVRSRV_OK;
}

/*
	PVRSRVRGXConfigMuxHWPerfCountersKM
 */
PVRSRV_ERROR PVRSRVRGXConfigMuxHWPerfCountersKM(
		CONNECTION_DATA               *psConnection,
		PVRSRV_DEVICE_NODE            *psDeviceNode,
		IMG_UINT32                     ui32ArrayLen,
		RGX_HWPERF_CONFIG_MUX_CNTBLK  *psBlockConfigs)
{
	PVRSRV_ERROR		eError = PVRSRV_OK;
	RGXFWIF_KCCB_CMD	sKccbCmd;
	DEVMEM_MEMDESC*		psFwBlkConfigsMemDesc;
	RGX_HWPERF_CONFIG_MUX_CNTBLK* psFwArray;
	IMG_UINT32			ui32kCCBCommandSlot;
	PVRSRV_RGXDEV_INFO	*psDevice;

	PVR_LOG_RETURN_IF_FALSE(psDeviceNode != NULL, "psDeviceNode is NULL",
	                        PVRSRV_ERROR_INVALID_PARAMS);
	psDevice = psDeviceNode->pvDevice;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	PVR_LOG_RETURN_IF_FALSE(ui32ArrayLen > 0, "ui32ArrayLen is 0",
	                  PVRSRV_ERROR_INVALID_PARAMS);
	PVR_LOG_RETURN_IF_FALSE(psBlockConfigs != NULL, "psBlockConfigs is NULL",
	                  PVRSRV_ERROR_INVALID_PARAMS);

	PVR_DPF_ENTERED;

	/* Fill in the command structure with the parameters needed
	 */
	sKccbCmd.eCmdType = RGXFWIF_KCCB_CMD_HWPERF_CONFIG_ENABLE_BLKS;
	sKccbCmd.uCmdData.sHWPerfCfgEnableBlks.ui32NumBlocks = ui32ArrayLen;

	/* used for passing counters config to the Firmware, write-only for the CPU */
	eError = DevmemFwAllocate(psDevice,
	                          sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK)*ui32ArrayLen,
	                          PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) |
	                          PVRSRV_MEMALLOCFLAG_GPU_READABLE |
	                          PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE |
	                          PVRSRV_MEMALLOCFLAG_GPU_UNCACHED |
	                          PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE |
	                          PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC |
	                          PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE |
	                          PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN),
	                          "FwHWPerfCountersConfigBlock",
	                          &psFwBlkConfigsMemDesc);
	PVR_LOG_RETURN_IF_ERROR(eError, "DevmemFwAllocate");

	eError = RGXSetFirmwareAddress(&sKccbCmd.uCmdData.sHWPerfCfgEnableBlks.sBlockConfigs,
	                      psFwBlkConfigsMemDesc, 0, RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress", fail1);

	eError = DevmemAcquireCpuVirtAddr(psFwBlkConfigsMemDesc, (void **)&psFwArray);
	PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireCpuVirtAddr", fail2);

	OSCachedMemCopyWMB(psFwArray, psBlockConfigs, sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK)*ui32ArrayLen);
	DevmemPDumpLoadMem(psFwBlkConfigsMemDesc,
	                   0,
	                   sizeof(RGX_HWPERF_CONFIG_MUX_CNTBLK)*ui32ArrayLen,
	                   PDUMP_FLAGS_CONTINUOUS);

	/*PVR_DPF((PVR_DBG_VERBOSE, "PVRSRVRGXConfigMuxHWPerfCountersKM parameters set, calling FW"));*/

	/* Ask the FW to carry out the HWPerf configuration command
	 */
	eError = RGXScheduleCommandAndGetKCCBSlot(psDevice,
	                                          RGXFWIF_DM_GP,
											  &sKccbCmd,
											  PDUMP_FLAGS_CONTINUOUS,
											  &ui32kCCBCommandSlot);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXScheduleCommandAndGetKCCBSlot", fail2);

	/*PVR_DPF((PVR_DBG_VERBOSE, "PVRSRVRGXConfigMuxHWPerfCountersKM command scheduled for FW"));*/

	/* Wait for FW to complete */
	eError = RGXWaitForKCCBSlotUpdate(psDevice, ui32kCCBCommandSlot, PDUMP_FLAGS_CONTINUOUS);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXWaitForKCCBSlotUpdate", fail3);

	/* Release temporary memory used for block configuration
	 */
	RGXUnsetFirmwareAddress(psFwBlkConfigsMemDesc);
	DevmemReleaseCpuVirtAddr(psFwBlkConfigsMemDesc);
	DevmemFwUnmapAndFree(psDevice, psFwBlkConfigsMemDesc);

	/*PVR_DPF((PVR_DBG_VERBOSE, "PVRSRVRGXConfigMuxHWPerfCountersKM firmware completed"));*/

	PVR_DPF((PVR_DBG_MESSAGE, "HWPerf %d counter blocks configured and ENABLED", ui32ArrayLen));

	PVR_DPF_RETURN_OK;

fail3:
	DevmemReleaseCpuVirtAddr(psFwBlkConfigsMemDesc);
fail2:
	RGXUnsetFirmwareAddress(psFwBlkConfigsMemDesc);
fail1:
	DevmemFwUnmapAndFree(psDevice, psFwBlkConfigsMemDesc);

	PVR_DPF_RETURN_RC(eError);
}


/*
	PVRSRVRGXConfigCustomCountersReadingHWPerfKM
 */
PVRSRV_ERROR PVRSRVRGXConfigCustomCountersKM(
		CONNECTION_DATA             * psConnection,
		PVRSRV_DEVICE_NODE          * psDeviceNode,
		IMG_UINT16                    ui16CustomBlockID,
		IMG_UINT16                    ui16NumCustomCounters,
		IMG_UINT32                  * pui32CustomCounterIDs)
{
	PVRSRV_ERROR		eError = PVRSRV_OK;
	RGXFWIF_KCCB_CMD	sKccbCmd;
	DEVMEM_MEMDESC*		psFwSelectCntrsMemDesc = NULL;
	IMG_UINT32*			psFwArray;
	IMG_UINT32			ui32kCCBCommandSlot;
	PVRSRV_RGXDEV_INFO	*psDevice = psDeviceNode->pvDevice;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	PVR_DPF_ENTERED;

	PVR_ASSERT(psDeviceNode);

	PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVRGXSelectCustomCountersKM: configure block %u to read %u counters", ui16CustomBlockID, ui16NumCustomCounters));

	/* Fill in the command structure with the parameters needed */
	sKccbCmd.eCmdType = RGXFWIF_KCCB_CMD_HWPERF_SELECT_CUSTOM_CNTRS;
	sKccbCmd.uCmdData.sHWPerfSelectCstmCntrs.ui16NumCounters = ui16NumCustomCounters;
	sKccbCmd.uCmdData.sHWPerfSelectCstmCntrs.ui16CustomBlock = ui16CustomBlockID;

	if (ui16NumCustomCounters > 0)
	{
		PVR_ASSERT(pui32CustomCounterIDs);

		/* used for passing counters config to the Firmware, write-only for the CPU */
		eError = DevmemFwAllocate(psDevice,
		                          sizeof(IMG_UINT32) * ui16NumCustomCounters,
		                          PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) |
		                          PVRSRV_MEMALLOCFLAG_GPU_READABLE |
		                          PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE |
		                          PVRSRV_MEMALLOCFLAG_GPU_UNCACHED |
		                          PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE |
		                          PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC |
		                          PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE |
		                          PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN),
		                          "FwHWPerfConfigCustomCounters",
		                          &psFwSelectCntrsMemDesc);
		PVR_LOG_RETURN_IF_ERROR(eError, "DevmemFwAllocate");

		eError = RGXSetFirmwareAddress(&sKccbCmd.uCmdData.sHWPerfSelectCstmCntrs.sCustomCounterIDs,
		                      psFwSelectCntrsMemDesc, 0, RFW_FWADDR_FLAG_NONE);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress", fail1);

		eError = DevmemAcquireCpuVirtAddr(psFwSelectCntrsMemDesc, (void **)&psFwArray);
		PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireCpuVirtAddr", fail2);

		OSCachedMemCopyWMB(psFwArray, pui32CustomCounterIDs, sizeof(IMG_UINT32) * ui16NumCustomCounters);
		DevmemPDumpLoadMem(psFwSelectCntrsMemDesc,
		                   0,
		                   sizeof(IMG_UINT32) * ui16NumCustomCounters,
		                   PDUMP_FLAGS_CONTINUOUS);
	}

	/* Push in the KCCB the command to configure the custom counters block */
	eError = RGXScheduleCommandAndGetKCCBSlot(psDevice,
	                                          RGXFWIF_DM_GP,
											  &sKccbCmd,
											  PDUMP_FLAGS_CONTINUOUS,
											  &ui32kCCBCommandSlot);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXScheduleCommandAndGetKCCBSlot", fail3);

	PVR_DPF((PVR_DBG_VERBOSE, "PVRSRVRGXSelectCustomCountersKM: Command scheduled"));

	/* Wait for FW to complete */
	eError = RGXWaitForKCCBSlotUpdate(psDevice, ui32kCCBCommandSlot, PDUMP_FLAGS_CONTINUOUS);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXWaitForKCCBSlotUpdate", fail3);

	PVR_DPF((PVR_DBG_VERBOSE, "PVRSRVRGXSelectCustomCountersKM: FW operation completed"));

	if (ui16NumCustomCounters > 0)
	{
		/* Release temporary memory used for block configuration */
		RGXUnsetFirmwareAddress(psFwSelectCntrsMemDesc);
		DevmemReleaseCpuVirtAddr(psFwSelectCntrsMemDesc);
		DevmemFwUnmapAndFree(psDevice, psFwSelectCntrsMemDesc);
	}

	PVR_DPF((PVR_DBG_MESSAGE, "HWPerf custom counters %u reading will be sent with the next HW events", ui16NumCustomCounters));

	PVR_DPF_RETURN_OK;

fail3:
	if (psFwSelectCntrsMemDesc)
	{
		DevmemReleaseCpuVirtAddr(psFwSelectCntrsMemDesc);
	}
fail2:
	if (psFwSelectCntrsMemDesc)
	{
		RGXUnsetFirmwareAddress(psFwSelectCntrsMemDesc);
	}
fail1:
	if (psFwSelectCntrsMemDesc)
	{
		DevmemFwUnmapAndFree(psDevice, psFwSelectCntrsMemDesc);
	}

	PVR_DPF_RETURN_RC(eError);
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
	PVRSRV_ERROR             eError = PVRSRV_OK;
	RGXFWIF_KCCB_CMD         sKccbCmd;
	DEVMEM_MEMDESC           *psFwBlkConfigsMemDesc;
	RGX_HWPERF_CONFIG_CNTBLK *psFwArray;
	IMG_UINT32               ui32kCCBCommandSlot;
	PVRSRV_RGXDEV_INFO       *psDevice;

	PVR_LOG_RETURN_IF_FALSE(psDeviceNode != NULL, "psDeviceNode is NULL",
	                        PVRSRV_ERROR_INVALID_PARAMS);

	psDevice = psDeviceNode->pvDevice;

	PVR_UNREFERENCED_PARAMETER(ui32CtrlWord);

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	PVR_LOG_RETURN_IF_FALSE(ui32ArrayLen > 0, "ui32ArrayLen is 0",
	                        PVRSRV_ERROR_INVALID_PARAMS);
	PVR_LOG_RETURN_IF_FALSE(psBlockConfigs != NULL, "psBlockConfigs is NULL",
	                        PVRSRV_ERROR_INVALID_PARAMS);

	PVR_DPF_ENTERED;

	/* Fill in the command structure with the parameters needed */
	sKccbCmd.eCmdType = RGXFWIF_KCCB_CMD_HWPERF_CONFIG_BLKS;
	sKccbCmd.uCmdData.sHWPerfCfgDABlks.ui32NumBlocks = ui32ArrayLen;

	/* used for passing counters config to the Firmware, write-only for the CPU */
	eError = DevmemFwAllocate(psDevice,
	                          sizeof(RGX_HWPERF_CONFIG_CNTBLK) * ui32ArrayLen,
	                          PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) |
	                          PVRSRV_MEMALLOCFLAG_GPU_READABLE |
	                          PVRSRV_MEMALLOCFLAG_GPU_UNCACHED |
	                          PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE |
	                          PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC |
	                          PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE |
	                          PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN),
	                          "FwHWPerfCountersDAConfigBlock",
	                          &psFwBlkConfigsMemDesc);
	PVR_LOG_RETURN_IF_ERROR(eError, "DevmemFwAllocate");

	eError = RGXSetFirmwareAddress(&sKccbCmd.uCmdData.sHWPerfCfgDABlks.sBlockConfigs,
	                               psFwBlkConfigsMemDesc, 0, RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress", fail1);

	eError = DevmemAcquireCpuVirtAddr(psFwBlkConfigsMemDesc, (void **)&psFwArray);
	PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireCpuVirtAddr", fail2);

	OSCachedMemCopyWMB(psFwArray, psBlockConfigs, sizeof(RGX_HWPERF_CONFIG_CNTBLK)*ui32ArrayLen);
	DevmemPDumpLoadMem(psFwBlkConfigsMemDesc,
	                   0,
	                   sizeof(RGX_HWPERF_CONFIG_CNTBLK)*ui32ArrayLen,
	                   PDUMP_FLAGS_CONTINUOUS);

	/* Ask the FW to carry out the HWPerf configuration command. */
	eError = RGXScheduleCommandAndGetKCCBSlot(psDevice,
	                                          RGXFWIF_DM_GP,
	                                          &sKccbCmd,
	                                          PDUMP_FLAGS_CONTINUOUS,
	                                          &ui32kCCBCommandSlot);

	PVR_LOG_GOTO_IF_ERROR(eError, "RGXScheduleCommandAndGetKCCBSlot", fail2);

	/* Wait for FW to complete */
	eError = RGXWaitForKCCBSlotUpdate(psDevice, ui32kCCBCommandSlot, PDUMP_FLAGS_CONTINUOUS);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXWaitForKCCBSlotUpdate", fail3);

	/* Release temporary memory used for block configuration. */
	RGXUnsetFirmwareAddress(psFwBlkConfigsMemDesc);
	DevmemReleaseCpuVirtAddr(psFwBlkConfigsMemDesc);
	DevmemFwUnmapAndFree(psDevice, psFwBlkConfigsMemDesc);

	PVR_DPF((PVR_DBG_MESSAGE, "HWPerf %d counter blocks configured and ENABLED",
	         ui32ArrayLen));

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

PVRSRV_ERROR RGXHWPerfConfigMuxCounters(
		RGX_HWPERF_CONNECTION           *psHWPerfConnection,
		IMG_UINT32					     ui32NumBlocks,
		RGX_HWPERF_CONFIG_MUX_CNTBLK	*asBlockConfigs)
{
	PVRSRV_ERROR           eError;
	RGX_KM_HWPERF_DEVDATA* psDevData;
	RGX_HWPERF_DEVICE *psHWPerfDev;


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

		PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDevData->psRgxDevNode, PVRSRV_ERROR_NOT_IMPLEMENTED);

		/* Call the internal server API */
		eError = PVRSRVRGXConfigMuxHWPerfCountersKM(NULL,
		                                            psDevData->psRgxDevNode,
		                                            ui32NumBlocks,
		                                            asBlockConfigs);
		PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVRGXConfigMuxHWPerfCountersKM");

		psHWPerfDev = psHWPerfDev->psNext;
	}

	return PVRSRV_OK;
}


PVRSRV_ERROR RGXHWPerfConfigureAndEnableCustomCounters(
		RGX_HWPERF_CONNECTION *psHWPerfConnection,
		IMG_UINT16              ui16CustomBlockID,
		IMG_UINT16          ui16NumCustomCounters,
		IMG_UINT32         *pui32CustomCounterIDs)
{
	PVRSRV_ERROR            eError;
	RGX_HWPERF_DEVICE       *psHWPerfDev;

	/* Validate input arguments supplied by the caller */
	PVR_LOG_RETURN_IF_FALSE((NULL != psHWPerfConnection), "psHWPerfConnection invalid",
	                   PVRSRV_ERROR_INVALID_PARAMS);
	PVR_LOG_RETURN_IF_FALSE((0 != ui16NumCustomCounters), "uiNumBlocks invalid",
			           PVRSRV_ERROR_INVALID_PARAMS);
	PVR_LOG_RETURN_IF_FALSE((NULL != pui32CustomCounterIDs),"asBlockConfigs invalid",
			           PVRSRV_ERROR_INVALID_PARAMS);

	/* Check # of blocks */
	PVR_LOG_RETURN_IF_FALSE((!(ui16CustomBlockID > RGX_HWPERF_MAX_CUSTOM_BLKS)),"ui16CustomBlockID invalid",
			           PVRSRV_ERROR_INVALID_PARAMS);

	/* Check # of counters */
	PVR_LOG_RETURN_IF_FALSE((!(ui16NumCustomCounters > RGX_HWPERF_MAX_CUSTOM_CNTRS)),"ui16NumCustomCounters invalid",
			           PVRSRV_ERROR_INVALID_PARAMS);

	psHWPerfDev = psHWPerfConnection->psHWPerfDevList;

	while (psHWPerfDev)
	{
		RGX_KM_HWPERF_DEVDATA *psDevData = (RGX_KM_HWPERF_DEVDATA *) psHWPerfDev->hDevData;

		PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDevData->psRgxDevNode, PVRSRV_ERROR_NOT_IMPLEMENTED);

		eError = PVRSRVRGXConfigCustomCountersKM(NULL,
				                                 psDevData->psRgxDevNode,
												 ui16CustomBlockID, ui16NumCustomCounters, pui32CustomCounterIDs);
		PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVRGXCtrlCustHWPerfKM");

		psHWPerfDev = psHWPerfDev->psNext;
	}

	return PVRSRV_OK;
}

/*************************************************************************/ /*!
@Function       GetHWPerfBlockTypeByID
@Description    Lookup function to obtain a block type descriptor for a given
                counter block identifier.
@Input          psDevInfo                       A pointer to current device info.
@Input          ui32BlockID                     The block ID for which a type
                                                descriptor should be retrieved.
@Return         RGXFW_HWPERF_CNTBLK_TYPE_MODEL  Block type descriptor.
*/ /**************************************************************************/
static const RGXFW_HWPERF_CNTBLK_TYPE_MODEL *
GetHWPerfBlockTypeByID(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32BlockID)
{
	IMG_UINT32 ui32CntBlkModelLen;
	const RGXFW_HWPERF_CNTBLK_TYPE_MODEL *asCntBlkTypeModel;
	IMG_UINT32 ui32TableIdx = 0xFFFF;
	RGX_HWPERF_CNTBLK_RT_INFO sRtInfo; /* Only used to satisfy pfnIsBlkPresent requirements. */

#if defined(HWPERF_UNIFIED)
	IMG_UINT32 uiBlockID = (IMG_UINT32)(ui32BlockID & ~(RGX_CNTBLK_ID_UNIT_ALL_MASK|RGX_CNTBLK_ID_DA_MASK));
#else
	IMG_UINT32 uiBlockID = (IMG_UINT32)(ui32BlockID & ~RGX_CNTBLK_ID_UNIT_ALL_MASK);
#endif

	ui32CntBlkModelLen = RGXGetHWPerfBlockConfig(&asCntBlkTypeModel);

	/* Is it a direct block? */
	if (uiBlockID < RGX_CNTBLK_ID_DIRECT_LAST)
	{
		ui32TableIdx = uiBlockID;
	}
	/* Is it an indirect block */
	else if ((uiBlockID > RGX_CNTBLK_ID_DIRECT_LAST) && (uiBlockID < RGX_CNTBLK_ID_LAST))
	{
		ui32TableIdx = RGX_CNTBLK_ID_DIRECT_LAST + (((uiBlockID & ~RGX_CNTBLK_ID_UNIT_ALL_MASK) >> RGX_CNTBLK_ID_GROUP_SHIFT) - 1U);
	}
	/* Unknown mapping from CNTBLK_ID to Table index */
	else
	{
		return NULL;
	}

	PVR_ASSERT(ui32TableIdx < ui32CntBlkModelLen);

	if (psDevInfo == NULL)
	{
		PVR_LOG(("psDevInfo invalid"));
		return NULL;
	}

	if ((ui32TableIdx < ui32CntBlkModelLen) &&
		(asCntBlkTypeModel[ui32TableIdx].pfnIsBlkPresent(&asCntBlkTypeModel[ui32TableIdx], psDevInfo, &sRtInfo) != IMG_FALSE))
	{
		return &asCntBlkTypeModel[ui32TableIdx];
	}

	/* Fall through, block not valid from run-time validation */
	return NULL;
}

PVRSRV_ERROR PVRSRVRGXGetConfiguredHWPerfMuxCountersKM(CONNECTION_DATA *psConnection,
                                                       PVRSRV_DEVICE_NODE *psDeviceNode,
                                                       const IMG_UINT32 ui32BlockID,
                                                       RGX_HWPERF_CONFIG_MUX_CNTBLK *psConfiguredMuxCounters)
{
	RGXFWIF_HWPERF_CTL *psHWPerfCtl;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	PVR_LOG_RETURN_IF_FALSE(psDeviceNode != NULL, "psDeviceNode is invalid", PVRSRV_ERROR_INVALID_PARAMS);

	eError = RGXAcquireHWPerfCtlCPUAddr(psDeviceNode, &psHWPerfCtl);
	PVR_LOG_RETURN_IF_ERROR(eError, "RGXAcquireHWPerfCtlCPUAddr");

	eError = PVRSRVRGXGetConfiguredHWPerfMuxCounters(psDeviceNode,
	                                                 psHWPerfCtl,
	                                                 ui32BlockID,
	                                                 psConfiguredMuxCounters);
	PVR_LOG_IF_ERROR(eError, "PVRSRVRGXGetConfiguredHWPerfMuxCounters");

	RGXReleaseHWPerfCtlCPUAddr(psDeviceNode);

	return eError;
}

PVRSRV_ERROR PVRSRVRGXGetConfiguredHWPerfMuxCounters(PVRSRV_DEVICE_NODE *psDevNode,
                                                     RGXFWIF_HWPERF_CTL *psHWPerfCtl,
                                                     IMG_UINT32 ui32BlockID,
                                                     RGX_HWPERF_CONFIG_MUX_CNTBLK *psConfiguredMuxCounters)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = NULL;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_RETURN_IF_FALSE(psDevNode != NULL, PVRSRV_ERROR_INVALID_PARAMS);
	PVR_RETURN_IF_FALSE(psHWPerfCtl != NULL, PVRSRV_ERROR_INVALID_PARAMS);
	PVR_RETURN_IF_FALSE(psConfiguredMuxCounters != NULL, PVRSRV_ERROR_INVALID_PARAMS);

	psDevInfo = (PVRSRV_RGXDEV_INFO*)psDevNode->pvDevice;
	PVR_RETURN_IF_FALSE(psDevInfo != NULL, PVRSRV_ERROR_INVALID_PARAMS);

	if ((ui32BlockID & ~RGX_CNTBLK_ID_UNIT_ALL_MASK) < RGX_CNTBLK_ID_LAST)
	{
		RGXFWIF_HWPERF_CTL_BLK *psBlock = rgxfw_hwperf_get_block_ctl(ui32BlockID, psHWPerfCtl);
		const RGXFW_HWPERF_CNTBLK_TYPE_MODEL *psBlkTypeDesc;
		IMG_UINT32 i, ui32LastCountIdx = 0, ui8CurCountIdx = 0;
		RGX_HWPERF_CONFIG_MUX_CNTBLK sBlockConfig;

		PVR_RETURN_IF_ERROR(PVRSRVPowerLock(psDevNode));

		if (psBlock == NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "Block ID (0x%04x) was invalid.", ui32BlockID));
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, Error);
		}

		if (!psBlock->ui32Enabled || !psBlock->ui32Valid)
		{
			PVR_DPF((PVR_DBG_ERROR, "Block (0x%04x) is not %s",
			         ui32BlockID,
			         !psBlock->ui32Enabled ? "enabled." : "configured."));
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, Error);
		}

		psBlkTypeDesc = GetHWPerfBlockTypeByID(psDevInfo, psBlock->eBlockID);
		if (psBlkTypeDesc == NULL)
		{
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, Error);
		}

		sBlockConfig.ui16BlockID = psBlock->eBlockID;
		sBlockConfig.ui8Mode = 0;

		for (i = 0; ((psBlock->uiCounterMask >> i) != 0) &&
		     (ui8CurCountIdx < psBlkTypeDesc->ui8NumCounters); i++)
		{
			if (psBlock->uiCounterMask & (1 << i))
			{
				IMG_UINT8 ui8Mode = 0;

				ui8Mode = (psBlock->aui64CounterCfg[i] >> psBlkTypeDesc->ui8SelectRegModeShift) & 1U;
				sBlockConfig.ui8Mode |= ui8Mode << ui32LastCountIdx;

				sBlockConfig.aui8GroupSelect[ui32LastCountIdx] =
					(psBlock->aui64CounterCfg[i] >> RGX_CR_TA_PERF_SELECT0_GROUP_SELECT_SHIFT) & 0x1F;

				sBlockConfig.aui16BitSelect[ui32LastCountIdx] =
					(psBlock->aui64CounterCfg[i] >> RGX_CR_TA_PERF_SELECT0_BIT_SELECT_SHIFT) & 0x7FFF;

#if defined(RGX_FEATURE_PERF_COUNTER_BATCH)
				sBlockConfig.aui32BatchMax[ui32LastCountIdx] =
					(psBlock->aui64CounterCfg[i] >> RGX_CR_TA_PERF_SELECT0_BATCH_MAX_SHIFT) & 0x1FFF;

				sBlockConfig.aui32BatchMin[ui32LastCountIdx] =
					(psBlock->aui64CounterCfg[i] >> RGX_CR_TA_PERF_SELECT0_BATCH_MIN_SHIFT) & 0x1FFF;
#endif
				ui32LastCountIdx++;
				ui8CurCountIdx++;
			}
		}

		sBlockConfig.ui8CounterSelect = (1 << ui32LastCountIdx) - 1;
		*psConfiguredMuxCounters = sBlockConfig;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "Block ID (0x%04x) was invalid.", ui32BlockID));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, InvalidIDError);
	}

Error:
	PVRSRVPowerUnlock(psDevNode);

InvalidIDError:
	return eError;
}

PVRSRV_ERROR PVRSRVRGXGetConfiguredHWPerfCounters(PVRSRV_DEVICE_NODE *psDevNode,
                                                  RGXFWIF_HWPERF_CTL *psHWPerfCtl,
                                                  IMG_UINT32 ui32BlockID,
                                                  RGX_HWPERF_CONFIG_CNTBLK *psConfiguredCounters)
{
	RGX_HWPERF_CONFIG_CNTBLK sBlockConfig;
	IMG_UINT32 i;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_RETURN_IF_FALSE(psDevNode != NULL, PVRSRV_ERROR_INVALID_PARAMS);
	PVR_RETURN_IF_FALSE(psHWPerfCtl != NULL, PVRSRV_ERROR_INVALID_PARAMS);
	PVR_RETURN_IF_FALSE(psConfiguredCounters != NULL, PVRSRV_ERROR_INVALID_PARAMS);

	if ((ui32BlockID & RGX_CNTBLK_ID_CUSTOM_MASK) >= RGX_CNTBLK_ID_LAST)
	{
		/* Validate block ID */
		switch (ui32BlockID)
		{
			case RGX_CNTBLK_ID_CUSTOM0:
			case RGX_CNTBLK_ID_CUSTOM1:
			case RGX_CNTBLK_ID_CUSTOM2:
			case RGX_CNTBLK_ID_CUSTOM3:
			case RGX_CNTBLK_ID_CUSTOM4_FW:
			{
				PVR_RETURN_IF_ERROR(PVRSRVPowerLock(psDevNode));

				/* Check to see if this block is enabled */
				if (psHWPerfCtl->ui32SelectedCountersBlockMask & (1 << (ui32BlockID & 0x0F)))
				{
					RGXFW_HWPERF_SELECT *psBlock = &psHWPerfCtl->SelCntr[ui32BlockID & 0x0F];

					sBlockConfig.ui16BlockID = ui32BlockID;
					sBlockConfig.ui16NumCounters = psBlock->ui32NumSelectedCounters;

					for (i = 0; i < psBlock->ui32NumSelectedCounters; i++)
					{
						sBlockConfig.ui16Counters[i] = psBlock->aui32SelectedCountersIDs[i];
					}
				}
				else
				{
					PVR_DPF((PVR_DBG_ERROR, "Block (0x%04x) is not enabled.", ui32BlockID));
					PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, Error);
				}
				break;
			}
			default:
			{
				PVR_DPF((PVR_DBG_ERROR, "Block ID (0x%04x) was invalid.", ui32BlockID));
				PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, InvalidIDError);
			}
		}
	}
#if defined(HWPERF_UNIFIED)
	else if ((ui32BlockID & RGX_CNTBLK_ID_DA_MASK) == RGX_CNTBLK_ID_DA_MASK)
	{
		RGXFWIF_HWPERF_DA_BLK *psBlock = rgxfw_hwperf_get_da_block_ctl(ui32BlockID, psHWPerfCtl);

		PVR_RETURN_IF_ERROR(PVRSRVPowerLock(psDevNode));

		if (psBlock == NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "Block ID (0x%04x) was invalid.", ui32BlockID));
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, Error);
		}

		if (!psBlock->uiEnabled)
		{
			PVR_DPF((PVR_DBG_ERROR, "Block (0x%04x) is not enabled.", ui32BlockID));
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, Error);
		}

		sBlockConfig.ui16BlockID = psBlock->eBlockID;
		sBlockConfig.ui16NumCounters = psBlock->uiNumCounters;

		for (i = 0; i < psBlock->uiNumCounters; i++)
		{
			sBlockConfig.ui16Counters[i] = psBlock->aui32Counters[i];
		}
	}
#endif
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "Block ID (0x%04x) was invalid.", ui32BlockID));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, InvalidIDError);
	}

Error:
	PVRSRVPowerUnlock(psDevNode);

InvalidIDError:
	if (eError == PVRSRV_OK)
	{
		*psConfiguredCounters = sBlockConfig;
	}

	return eError;
}

PVRSRV_ERROR PVRSRVRGXGetEnabledHWPerfBlocks(PVRSRV_DEVICE_NODE *psDevNode,
                                             RGXFWIF_HWPERF_CTL *psHWPerfCtl,
                                             IMG_UINT32 ui32ArrayLen,
                                             IMG_UINT32 *pui32BlockCount,
                                             IMG_UINT32 *pui32EnabledBlockIDs)
{
	IMG_UINT32 ui32LastIdx = 0;
	IMG_UINT32 i;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_RETURN_IF_FALSE(psDevNode != NULL, PVRSRV_ERROR_INVALID_PARAMS);
	PVR_RETURN_IF_FALSE(psHWPerfCtl != NULL, PVRSRV_ERROR_INVALID_PARAMS);
	PVR_RETURN_IF_FALSE(pui32BlockCount != NULL, PVRSRV_ERROR_INVALID_PARAMS);

	*pui32BlockCount = 0;

	if (ui32ArrayLen > 0 && pui32EnabledBlockIDs == NULL)
	{
		PVR_DPF((PVR_DBG_WARNING, "ui32ArrayLen is greater than 0 but pui32EnabledBlockIDs is NULL"));
	}

	PVR_RETURN_IF_ERROR(PVRSRVPowerLock(psDevNode));

	for (i = 0; i < RGX_HWPERF_MAX_MUX_BLKS; i++)
	{
		if (psHWPerfCtl->sBlkCfg[i].ui32Enabled && psHWPerfCtl->sBlkCfg[i].ui32Valid)
		{
			*pui32BlockCount += 1;

			if (pui32EnabledBlockIDs == NULL)
			{
				continue;
			}

			if (ui32LastIdx + 1 > ui32ArrayLen)
			{
				PVR_DPF((PVR_DBG_ERROR, "ui32ArrayLen less than the number of enabled blocks."));
				PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_OUT_OF_MEMORY, Error);
			}

			pui32EnabledBlockIDs[ui32LastIdx] = psHWPerfCtl->sBlkCfg[i].eBlockID;
			ui32LastIdx++;
		}
	}

	for (i = 0; i < RGX_HWPERF_MAX_CUSTOM_BLKS; i++)
	{
		if (psHWPerfCtl->ui32SelectedCountersBlockMask == 0)
		{
			break;
		}

		if (psHWPerfCtl->ui32SelectedCountersBlockMask & (1 << i))
		{
			*pui32BlockCount += 1;

			if (pui32EnabledBlockIDs == NULL)
			{
				continue;
			}

			if (ui32LastIdx + 1 > ui32ArrayLen)
			{
				PVR_DPF((PVR_DBG_ERROR, "ui32ArrayLen less than the number of enabled blocks."));
				PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_OUT_OF_MEMORY, Error);
			}

			pui32EnabledBlockIDs[ui32LastIdx] = RGX_CNTBLK_ID_CUSTOM0 + i;
			ui32LastIdx++;
		}
	}

#if defined(HWPERF_UNIFIED)
	for (i = 0; i < RGX_HWPERF_MAX_DA_BLKS; i++)
	{
		if (psHWPerfCtl->sDABlkCfg[i].uiEnabled)
		{
			*pui32BlockCount += 1;

			if (pui32EnabledBlockIDs == NULL)
			{
				continue;
			}

			if (ui32LastIdx > ui32ArrayLen)
			{
				PVR_DPF((PVR_DBG_ERROR, "ui32ArrayLen less than the number of enabled blocks."));
				PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_OUT_OF_MEMORY, Error);
			}

			pui32EnabledBlockIDs[ui32LastIdx] = psHWPerfCtl->sDABlkCfg[i].eBlockID;
			ui32LastIdx++;
		}
	}
#endif

Error:
	PVRSRVPowerUnlock(psDevNode);
	return eError;
}

/******************************************************************************
 End of file (rgxhwperf.c)
 ******************************************************************************/
