/*************************************************************************/ /*!
@File
@Title          Device specific utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Device specific functions
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

#include "rgx_fwif_km.h"
#include "pdump_km.h"
#include "osfunc.h"
#include "allocmem.h"
#include "pvr_debug.h"
#include "rgxutils.h"
#include "power.h"
#include "pvrsrv.h"
#include "sync_internal.h"
#include "rgxfwutils.h"
#include "rgxlayer.h"
#include "rgxmmudefs_km.h"
#include "rgxta3d.h"
#include "devicemem_server.h"

PVRSRV_ERROR RGXQueryAPMState(const PVRSRV_DEVICE_NODE *psDeviceNode,
	const void *pvPrivateData,
	IMG_UINT32 *pui32State)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_UNREFERENCED_PARAMETER(pvPrivateData);

	if (!psDeviceNode)
		return PVRSRV_ERROR_INVALID_PARAMS;

	psDevInfo = psDeviceNode->pvDevice;
	*pui32State = psDevInfo->eActivePMConf;

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXSetAPMState(const PVRSRV_DEVICE_NODE *psDeviceNode,
	const void *pvPrivateData,
	IMG_UINT32 ui32State)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
#if !defined(NO_HARDWARE)
	PVRSRV_RGXDEV_INFO *psDevInfo;
#endif

	PVR_UNREFERENCED_PARAMETER(pvPrivateData);

	if (!psDeviceNode || !psDeviceNode->pvDevice)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (RGX_ACTIVEPM_FORCE_OFF != ui32State)
	{
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

#if !defined(NO_HARDWARE)
	psDevInfo = psDeviceNode->pvDevice;

	if (psDevInfo->pvAPMISRData)
	{
		psDevInfo->eActivePMConf = RGX_ACTIVEPM_FORCE_OFF;
		psDevInfo->pvAPMISRData = NULL;
		eError = PVRSRVSetDeviceDefaultPowerState((PPVRSRV_DEVICE_NODE)psDeviceNode,
		                                          PVRSRV_DEV_POWER_STATE_ON);
	}
#endif

	return eError;
}

PVRSRV_ERROR RGXQueryPdumpPanicDisable(const PVRSRV_DEVICE_NODE *psDeviceNode,
	const void *pvPrivateData,
	IMG_BOOL *pbDisabled)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_UNREFERENCED_PARAMETER(pvPrivateData);

	if (!psDeviceNode || !psDeviceNode->pvDevice)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = psDeviceNode->pvDevice;

	*pbDisabled = !psDevInfo->bPDPEnabled;

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXSetPdumpPanicDisable(const PVRSRV_DEVICE_NODE *psDeviceNode,
	const void *pvPrivateData,
	IMG_BOOL bDisable)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_UNREFERENCED_PARAMETER(pvPrivateData);

	if (!psDeviceNode || !psDeviceNode->pvDevice)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = psDeviceNode->pvDevice;

	psDevInfo->bPDPEnabled = !bDisable;

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXGetDeviceFlags(PVRSRV_RGXDEV_INFO *psDevInfo,
				IMG_UINT32 *pui32DeviceFlags)
{
	if (!pui32DeviceFlags || !psDevInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*pui32DeviceFlags = psDevInfo->ui32DeviceFlags;

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXSetDeviceFlags(PVRSRV_RGXDEV_INFO *psDevInfo,
				IMG_UINT32 ui32Config,
				IMG_BOOL bSetNotClear)
{
	if (!psDevInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if ((ui32Config & ~RGXKM_DEVICE_STATE_MASK) != 0)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Bits outside of device state mask set (input: 0x%x, mask: 0x%x)",
				 __func__, ui32Config, RGXKM_DEVICE_STATE_MASK));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (bSetNotClear)
	{
		psDevInfo->ui32DeviceFlags |= ui32Config;
	}
	else
	{
		psDevInfo->ui32DeviceFlags &= ~ui32Config;
	}

	return PVRSRV_OK;
}

inline const char * RGXStringifyKickTypeDM(RGX_KICK_TYPE_DM eKickTypeDM)
{
	PVR_ASSERT(eKickTypeDM < RGX_KICK_TYPE_DM_LAST);

	switch (eKickTypeDM) {
		case RGX_KICK_TYPE_DM_GP:
			return "GP ";
		case RGX_KICK_TYPE_DM_TDM_2D:
			return "TDM/2D ";
		case RGX_KICK_TYPE_DM_TA:
			return "TA ";
		case RGX_KICK_TYPE_DM_3D:
			return "3D ";
		case RGX_KICK_TYPE_DM_CDM:
			return "CDM ";
		case RGX_KICK_TYPE_DM_RTU:
			return "RTU ";
		case RGX_KICK_TYPE_DM_SHG:
			return "SHG ";
		case RGX_KICK_TYPE_DM_TQ2D:
			return "TQ2D ";
		case RGX_KICK_TYPE_DM_TQ3D:
			return "TQ3D ";
		default:
			return "Invalid DM ";
	}
}

PHYS_HEAP_POLICY RGXPhysHeapGetLMAPolicy(PHYS_HEAP_USAGE_FLAGS ui32UsageFlags, PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PHYS_HEAP_POLICY ui32Policy;

	if (OSIsMapPhysNonContigSupported())
	{
		ui32Policy = PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG;

		if (BITMASK_ANY(ui32UsageFlags,
			(PHYS_HEAP_USAGE_FW_SHARED    |
			 PHYS_HEAP_USAGE_FW_PRIVATE   |
			 PHYS_HEAP_USAGE_FW_PREMAP_PT |
			 PHYS_HEAP_USAGE_FW_CODE      |
			 PHYS_HEAP_USAGE_FW_PRIV_DATA)))
		{
			if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
			{
				/* Guest Firmware heaps are always premapped */
				ui32Policy = PHYS_HEAP_POLICY_DEFAULT;
			}
#if defined(RGX_PREMAP_FW_HEAPS)
			else if (PVRSRV_VZ_MODE_IS(HOST, DEVNODE, psDeviceNode))
			{
				/* All Firmware heaps are premapped under AutoVz*/
				ui32Policy = PHYS_HEAP_POLICY_DEFAULT;
			}
#endif
		}

		if (BITMASK_ANY(ui32UsageFlags, PHYS_HEAP_USAGE_FW_PREMAP))
		{
			ui32Policy = PHYS_HEAP_POLICY_DEFAULT;
		}
	}
	else
	{
		ui32Policy = PHYS_HEAP_POLICY_DEFAULT;
	}

	return ui32Policy;
}

IMG_BOOL RGXIsErrorAndDeviceRecoverable(PVRSRV_DEVICE_NODE *psDeviceNode, PVRSRV_ERROR *peError)
{
	IMG_BOOL bRecoverable = IMG_TRUE;

	if (*peError == PVRSRV_OK)
	{
		/* No recovery required */
		return IMG_FALSE;
	}

	if (!PVRSRVIsStatusRecoverable(OSAtomicRead(&psDeviceNode->eHealthStatus)))
	{
		bRecoverable = IMG_FALSE;
	}
	else
	{
		RGXUpdateHealthStatus(psDeviceNode, IMG_FALSE);

		if (!PVRSRVIsStatusRecoverable(OSAtomicRead(&psDeviceNode->eHealthStatus)))
		{
			bRecoverable = IMG_FALSE;
		}
	}

	if (bRecoverable && !PVRSRVIsRetryError(*peError))
	{
		PVR_DPF((PVR_DBG_WARNING,
				 "%s: Device is recoverable. Changing error type (%s) to retry.",
				 __func__, PVRSRVGetErrorString(*peError)));
		*peError = PVRSRV_ERROR_RETRY;
	}

	if (!bRecoverable && PVRSRVIsRetryError(*peError))
	{
		PVR_DPF((PVR_DBG_WARNING,
				 "%s: Device is not recoverable. Error type should not be retry (%s).",
				 __func__, PVRSRVGetErrorString(*peError)));
		*peError = PVRSRV_ERROR_INVALID_PARAMS;
	}

	return bRecoverable;
}

/*
 *  Function that returns the MList Size required for a given max PB size.
 *
 *  The maximum MList size required always depends on the maximum PB Size
 *  chosen and must also take into account the additional pages that will
 *  be provided by a local PB.
 */
IMG_UINT32 RGXCalcMListSize(PVRSRV_DEVICE_NODE *psDeviceNode,
                            IMG_UINT64 ui64MaxLocalPBSize,
                            IMG_UINT64 ui64MaxGlobalPBSize)
{
	IMG_UINT32  ui32PTEPages = 0, ui32PDEPages = 0, ui32PCEPages = 0, ui32MListSize = 0;
	IMG_UINT32  ui32NumOfPipes = 1;
	IMG_UINT64  ui64TotalPages = 0;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	/*
	 *  Assert if Size of PB exceeds maximum theoretical limit
	 *  RGX_PM_MAX_PB_VIRT_ADDR_SPACE represents the 16G address space#
	 */
	PVR_ASSERT(ui64MaxLocalPBSize+ui64MaxGlobalPBSize <= RGX_PM_MAX_PB_VIRT_ADDR_SPACE);

	/* Calculate the total number of pages which is the number of Page table entries */
	ui64TotalPages = ((ui64MaxLocalPBSize+ui64MaxGlobalPBSize)/RGX_BIF_PM_PHYSICAL_PAGE_SIZE);

	/* Calculate the total number of pages required for the PTE's (minimum of 1) */
	ui32PTEPages = (IMG_UINT32)(ui64TotalPages/RGX_MMUCTRL_ENTRIES_PT_VALUE);
	if (ui32PTEPages == 0U)
	{
		ui32PTEPages = 1;
	}

	/* Calculate the total number of pages required to hold the PDE's (minimum of 1) */
	ui32PDEPages = ui32PTEPages/RGX_MMUCTRL_ENTRIES_PD_VALUE;
	if (ui32PDEPages == 0U)
	{
		ui32PDEPages = 1;
	}

	/* Calculate the total number of pages required to hold the PCE's (minimum of 1) */
	ui32PCEPages = ui32PDEPages/RGX_MMUCTRL_ENTRIES_PC_VALUE;
	if (ui32PCEPages == 0U)
	{
		ui32PCEPages = 1;
	}

	/* Calculate the maximum number of TA/VCE pipes */
#if defined(RGX_FEATURE_SCALABLE_TE_ARCH_IDX)
	{
		IMG_UINT32 ui32Val = RGX_GET_FEATURE_VALUE(psDevInfo, RGX_FEATURE_SCALABLE_TE_ARCH);
		if (ui32Val > ui32NumOfPipes)
		{
			ui32NumOfPipes = ui32Val;
		}
	}
#endif

#if defined(RGX_FEATURE_SCALABLE_TE_ARCH_IDX)
	{
		IMG_UINT32 ui32Val = RGX_GET_FEATURE_VALUE(psDevInfo, RGX_FEATURE_SCALABLE_VCE);
		if (ui32Val > ui32NumOfPipes)
		{
			ui32NumOfPipes = ui32Val;
		}
	}
#endif

	/*
	 *  Calculate the MList size considering the total number of pages in PB are shared
	 *  among all the PM address spaces...
	 */
	ui32MListSize = (ui32PCEPages + ui32PDEPages + ui32PTEPages) *
					RGX_NUM_PM_ADDR_SPACES * ui32NumOfPipes * RGX_MLIST_ENTRY_STRIDE;

	/* Round it off to the nearest page granularity */
	ui32MListSize = PVR_ALIGN(ui32MListSize, RGX_BIF_PM_PHYSICAL_PAGE_SIZE);

	return ui32MListSize;
}

/*
 * Critical PMRs are PMRs that are created by client that might contain physical page addresses.
 * We need to validate if they were allocated with proper flags.
 */
static PVRSRV_ERROR
_ValidateCriticalPMR(PMR* psPMR, IMG_DEVMEM_SIZE_T ui64MinSize)
{
	PVRSRV_ERROR eError;
	PVRSRV_DEVICE_NODE *psDevNode = PMR_DeviceNode(psPMR);

	IMG_BOOL bCPUCacheSnoop =
		(PVRSRVSystemSnoopingOfCPUCache(psDevNode->psDevConfig) &&
		 psDevNode->pfnGetDeviceSnoopMode(psDevNode) == PVRSRV_DEVICE_SNOOP_CPU_ONLY);

	PMR_FLAGS_T uiFlags = PMR_Flags(psPMR);

	if (PMR_IsSparse(psPMR))
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Critical PMR cannot be sparse!",
		         __func__));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, return_error);
	}

	/* Critical PMR cannot be user CPU mappable */
	if (PVRSRV_CHECK_CPU_READABLE(uiFlags) ||
	    PVRSRV_CHECK_CPU_WRITEABLE(uiFlags))
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Critical PMR allows CPU mapping (0x%" PVRSRV_MEMALLOCFLAGS_FMTSPEC ")",
		         __func__, uiFlags));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_DEVICEMEM_INVALID_PMR_FLAGS, return_error);
	}

	/* Critical PMR must not be user CPU cacheable (unless snooping is on) */
	if (!bCPUCacheSnoop &&
	    (PVRSRV_CHECK_CPU_CACHE_INCOHERENT(uiFlags) ||
	     PVRSRV_CHECK_CPU_CACHE_COHERENT(uiFlags) ||
	     PVRSRV_CHECK_CPU_CACHED(uiFlags)))
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Critical PMR allows CPU caching (0x%" PVRSRV_MEMALLOCFLAGS_FMTSPEC ")",
		         __func__, uiFlags));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_DEVICEMEM_INVALID_PMR_FLAGS, return_error);
	}

	/* Critical PMRs must be allocated with PMMETA_PROTECT */
	if ((uiFlags & PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT)) == 0)
	{
		PVR_DPF((PVR_DBG_ERROR,
		        "%s: Critical PMR must have PMMETA_PROTECT set",
		        __func__));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_DEVICEMEM_INVALID_PMR_FLAGS, return_error);
	}

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	if (PVRSRV_CHECK_OS_LINUX_MOVABLE(uiFlags))
	{
		PVR_DPF((PVR_DBG_ERROR,
		        "%s: Critical PMR must not have OS_LINUX_MOVABLE set",
		        __func__));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_DEVICEMEM_INVALID_PMR_FLAGS, return_error);
	}
#endif

	if (PMR_LogicalSize(psPMR) < ui64MinSize)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Critical PMR doesn't have sufficient size",
		         __func__));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, return_error);
	}

	return PVRSRV_OK;
return_error:
	return eError;
}

/* Check if all global freelists have the same size and if all local freelists have the same size.*/
PVRSRV_ERROR ValidateFreeListSizes(RGX_FREELIST* apsFreeLists[RGXMKIF_NUM_RTDATA_FREELISTS],
                                   IMG_UINT32*   pui32LocalFLMaxPages,
                                   IMG_UINT32*   pui32GlobalFLMaxPages)
{
	IMG_UINT32 i,j;
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 ui32GlobalFLMaxPages = apsFreeLists[RGXFW_GLOBAL_FREELIST]->ui32MaxFLPages;
	IMG_UINT32 ui32LocalFLMaxPages = apsFreeLists[RGXFW_LOCAL_FREELIST]->ui32MaxFLPages;
	IMG_UINT32 ui32NumFLPerGD = RGXMKIF_NUM_RTDATA_FREELISTS/RGXMKIF_NUM_GEOMDATAS;

	for (i=0; i<RGXMKIF_NUM_GEOMDATAS; i++)
	{
		/* Check if all local freelists have the same size */
		if (apsFreeLists[ui32NumFLPerGD * i + RGXFW_LOCAL_FREELIST]->ui32MaxFLPages != ui32LocalFLMaxPages)
		{
			eError = PVRSRV_ERROR_INVALID_PARAMS;
		}

		/* Check if all global freelists have the same size */
		for (j=RGXFW_GLOBAL_FREELIST; j<ui32NumFLPerGD; j++)
		{
			if (apsFreeLists[ui32NumFLPerGD * i + j]->ui32MaxFLPages != ui32GlobalFLMaxPages)
			{
				eError = PVRSRV_ERROR_INVALID_PARAMS;
			}
		}
	}

	*pui32LocalFLMaxPages = ui32LocalFLMaxPages;
	*pui32GlobalFLMaxPages = ui32GlobalFLMaxPages;

	return eError;
}

PVRSRV_ERROR
AcquireValidateRefCriticalBuffer(PVRSRV_DEVICE_NODE*     psDevNode,
                                 DEVMEMINT_RESERVATION*  psReservation,
                                 IMG_DEVMEM_SIZE_T       ui64MinSize,
                                 PMR**                   ppsPMR,
                                 IMG_DEV_VIRTADDR*       psDevVAddr)
{
	PVRSRV_ERROR eError;

	/* Obtain reference to reservation object */
	if (!DevmemIntReservationAcquire(psReservation))
	{
		eError = PVRSRV_ERROR_REFCOUNT_OVERFLOW;
		PVR_LOG_GOTO_IF_ERROR_VA(eError, ReturnError,
		    "%s: Failed to acquire reservation for critical buffer", __func__);
	}

	eError = DevmemIntGetReservationData(psReservation, ppsPMR, psDevVAddr);
	PVR_LOG_GOTO_IF_ERROR_VA(eError, RollbackReservation,
	    "%s: Error from DevmemIntGetReservationData for critical buffer: %s",
	    __func__, PVRSRVGetErrorString(eError));


	/* Check buffer sizes and flags are as required */
	eError = _ValidateCriticalPMR(*ppsPMR, ui64MinSize);
	PVR_LOG_GOTO_IF_ERROR_VA(eError, RollbackReservation,
	    "%s: Validation of critical PMR failed: %s",
	    __func__, PVRSRVGetErrorString(eError));

	/* Check exclusive flag and set if possible */
	if (!PMR_SetExclusiveUse(*ppsPMR, IMG_TRUE))
	{
		PVR_DPF((PVR_DBG_ERROR,
		     "%s: Critical PMR already in use (exclusive flag)!",
		     __func__));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, RollbackReservation);
	}

	/* If no error on validation ref the PMR */
	eError = PMRRefPMR(*ppsPMR);
	PVR_LOG_GOTO_IF_ERROR_VA(eError, UnsetExclusive,
	    "%s: Cannot ref critical PMR: %s",
	    __func__, PVRSRVGetErrorString(eError));

	return PVRSRV_OK;

UnsetExclusive:
	PMR_SetExclusiveUse(*ppsPMR, IMG_FALSE);
RollbackReservation:
	DevmemIntReservationRelease(psReservation);
ReturnError:
	return eError;
}

void UnrefAndReleaseCriticalBuffer(DEVMEMINT_RESERVATION* psReservation)
{
	PVRSRV_ERROR eError;
	PMR* psPMR;
	IMG_DEV_VIRTADDR sUnused;
	/* Skip error check. If this function is called it means we already
	   Acquired a reservation and confirmed that mapping exists. */
	eError = DevmemIntGetReservationData(psReservation, &psPMR, &sUnused);
	PVR_LOG_IF_ERROR(eError, "DevmemIntGetReservationData");

	/* Ignore return value. Clearing the flag cannot fail. */
	PMR_SetExclusiveUse(psPMR, IMG_FALSE);

	eError = PMRUnrefPMR(psPMR);
	PVR_LOG_IF_ERROR(eError, "PMRUnrefPMR");

	DevmemIntReservationRelease(psReservation);
}

/******************************************************************************
 End of file (rgxutils.c)
******************************************************************************/
