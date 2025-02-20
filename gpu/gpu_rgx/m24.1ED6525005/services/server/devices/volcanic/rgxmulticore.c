/*************************************************************************/ /*!
@File           rgxmulticore.c
@Title          Functions related to multicore devices
@Codingstyle    IMG
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Kernel mode workload estimation functionality.
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

#include "rgxdevice.h"
#include "rgxdefs_km.h"
#include "pdump_km.h"
#include "rgxmulticore.h"
#include "multicore_defs.h"
#include "allocmem.h"
#include "pvr_debug.h"



static PVRSRV_ERROR RGXGetMultiCoreInfo(PVRSRV_DEVICE_NODE *psDeviceNode,
                                        IMG_UINT32 ui32CapsSize,
                                        IMG_UINT32 *pui32NumCores,
                                        IMG_UINT64 *pui64Caps);


/*
 * RGXInitMultiCoreInfo:
 * Return multicore information to clients.
 * Return not supported on cores without multicore.
 */
static PVRSRV_ERROR RGXGetMultiCoreInfo(PVRSRV_DEVICE_NODE *psDeviceNode,
                                 IMG_UINT32 ui32CapsSize,
                                 IMG_UINT32 *pui32NumCores,
                                 IMG_UINT64 *pui64Caps)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (psDevInfo->ui32MultiCoreNumCores == 0)
	{
		/* MULTICORE not supported on this device */
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
	}
	else
	{
		*pui32NumCores = psDevInfo->ui32MultiCoreNumCores;
		if (ui32CapsSize > 0)
		{
			if (ui32CapsSize < psDevInfo->ui32MultiCoreNumCores)
			{
				PVR_DPF((PVR_DBG_ERROR, "Multicore caps buffer too small"));
				eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
			}
			else
			{
				IMG_UINT32 i;

				for (i = 0; i < psDevInfo->ui32MultiCoreNumCores; ++i)
				{
					pui64Caps[i] = psDevInfo->pui64MultiCoreCapabilities[i];
				}
			}
		}
	}

	return eError;
}



/*
 * RGXInitMultiCoreInfo:
 * Read multicore HW registers and fill in data structure for clients.
 * Return not supported on cores without multicore.
 */
PVRSRV_ERROR RGXInitMultiCoreInfo(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (psDeviceNode->pfnGetMultiCoreInfo != NULL)
	{
		/* we only set this up once */
		return PVRSRV_OK;
	}

	/* defaults for non-multicore devices */
	psDevInfo->ui32MultiCoreNumCores = 0;
	psDevInfo->ui32MultiCorePrimaryId = (IMG_UINT32)(-1);
	psDevInfo->pui64MultiCoreCapabilities = NULL;
	psDeviceNode->pfnGetMultiCoreInfo = NULL;

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, GPU_MULTICORE_SUPPORT))
	{
		IMG_BOOL bPowerWasDown;
		IMG_UINT32 ui32MulticoreInfo;
		IMG_UINT32 ui32PrimaryCoreIds;
		IMG_UINT32 ui32PrimaryId;
		IMG_UINT32 ui32TotalCores;
		IMG_UINT32 ui32NumCores;
		IMG_UINT32 id, i;

#if defined(RGX_HOST_SECURE_REGBANK_OFFSET) && defined(XPU_MAX_REGBANKS_ADDR_WIDTH)
		IMG_UINT32 ui32MulticoreRegBankOffset = (1 << RGX_GET_FEATURE_VALUE(psDevInfo, XPU_MAX_REGBANKS_ADDR_WIDTH));

		/* Ensure the HOST_SECURITY reg bank definitions are correct */
		if ((RGX_HOST_SECURE_REGBANK_OFFSET + RGX_HOST_SECURE_REGBANK_SIZE) != ui32MulticoreRegBankOffset)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Register bank definitions for HOST_SECURITY don't match core's configuration.", __func__));
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}
#endif
		bPowerWasDown = ! PVRSRVIsSystemPowered(psDeviceNode);

		/* Power-up the device as required to read the registers */
		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) && bPowerWasDown)
		{
			eError = PVRSRVSetSystemPowerState(psDeviceNode->psDevConfig, PVRSRV_SYS_POWER_STATE_ON);
			PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVSetSystemPowerState ON");
		}

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
		if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
		{
			RGXFWIF_MULTICORE_INFO *psRGXMulticoreInfo;

			psRGXMulticoreInfo = &psDevInfo->psRGXFWIfOsInit->sRGXMulticoreInfo;
			ui32NumCores = psRGXMulticoreInfo->ui32NumCores;
			ui32TotalCores = psRGXMulticoreInfo->ui32TotalCores;
			ui32MulticoreInfo = psRGXMulticoreInfo->ui32MulticoreInfo;

			PVR_LOG(("RGX Guest Device initialised with %u %s",
					 ui32NumCores, (ui32NumCores == 1U) ? "core" : "cores"));
		}
		else
#endif
		{
			ui32NumCores = (OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MULTICORE_DOMAIN)
		                                            & ~RGX_CR_MULTICORE_DOMAIN_GPU_COUNT_CLRMSK)
		                                            >> RGX_CR_MULTICORE_DOMAIN_GPU_COUNT_SHIFT;

			ui32TotalCores = (OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MULTICORE_SYSTEM)
		                                            & ~RGX_CR_MULTICORE_SYSTEM_GPU_COUNT_CLRMSK)
		                                            >> RGX_CR_MULTICORE_SYSTEM_GPU_COUNT_SHIFT;
			ui32MulticoreInfo = OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MULTICORE);
		}
#if defined(NO_HARDWARE)
		/* override to defaults if no hardware */
		ui32NumCores = 8;//RGX_MULTICORE_MAX_NOHW_CORES;
		ui32TotalCores = RGX_MULTICORE_MAX_NOHW_CORES;
		ui32MulticoreInfo = 0;  /* primary id 0 with 7 secondaries */
#endif
		/* ID for this primary is in this register */
		ui32PrimaryId = (ui32MulticoreInfo & ~RGX_CR_MULTICORE_ID_CLRMSK) >> RGX_CR_MULTICORE_ID_SHIFT;

		/* allocate storage for capabilities */
		psDevInfo->pui64MultiCoreCapabilities = OSAllocMem(ui32NumCores * sizeof(psDevInfo->pui64MultiCoreCapabilities[0]));
		if (psDevInfo->pui64MultiCoreCapabilities == NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to alloc memory for Multicore info", __func__));
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}

		ui32PrimaryCoreIds = (ui32MulticoreInfo & ~RGX_CR_MULTICORE_PRIMARY_CORE_ID_CLRMSK)
		                                        >> RGX_CR_MULTICORE_PRIMARY_CORE_ID_SHIFT;

		psDevInfo->ui32MultiCorePrimaryId = ui32PrimaryId;
		psDevInfo->ui32MultiCoreNumCores = ui32NumCores;

		PVR_DPF((PVR_DBG_MESSAGE, "Multicore domain has %d cores with primary id %u\n", ui32NumCores, ui32PrimaryId));
		PDUMPCOMMENT(psDeviceNode,
		             "RGX Multicore domain has %d cores with primary id %u\n",
		             ui32NumCores, ui32PrimaryId);
		for (i = 0, id = 0; id < ui32TotalCores; ++id)
		{
			if ((ui32PrimaryCoreIds & 0x7) == ui32PrimaryId)
			{
				/* currently all cores are identical so have the same capabilities */
				psDevInfo->pui64MultiCoreCapabilities[i] = id
				                    | ((id == ui32PrimaryId) ? RGX_MULTICORE_CAPABILITY_PRIMARY_EN : 0)
				                    | RGX_MULTICORE_CAPABILITY_GEOMETRY_EN
				                    | RGX_MULTICORE_CAPABILITY_COMPUTE_EN
				                    | RGX_MULTICORE_CAPABILITY_FRAGMENT_EN;
				PDUMPCOMMENT(psDeviceNode, "\tCore %u has caps 0x%08x", id,
				             (IMG_UINT32)psDevInfo->pui64MultiCoreCapabilities[i]);
				PVR_DPF((PVR_DBG_MESSAGE, "Core %u has caps 0x%08x", id, (IMG_UINT32)psDevInfo->pui64MultiCoreCapabilities[i]));
				++i;
			}
			ui32PrimaryCoreIds >>= 3;
		}

        /* revert power state to what it was on entry to this function */
		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) && bPowerWasDown)
		{
			eError = PVRSRVSetSystemPowerState(psDeviceNode->psDevConfig, PVRSRV_SYS_POWER_STATE_OFF);
			PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVSetSystemPowerState OFF");
		}

		/* Register callback to return info about multicore setup to client bridge */
		psDeviceNode->pfnGetMultiCoreInfo = RGXGetMultiCoreInfo;
	}
	else
	{
		/* MULTICORE not supported on this device */
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
	}

	return eError;
}


/*
 * RGXDeinitMultiCoreInfo:
 * Release resources and clear the MultiCore values in the DeviceNode.
 */
void RGXDeInitMultiCoreInfo(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	if (psDevInfo->pui64MultiCoreCapabilities != NULL)
	{
		OSFreeMem(psDevInfo->pui64MultiCoreCapabilities);
		psDevInfo->pui64MultiCoreCapabilities = NULL;
		psDevInfo->ui32MultiCoreNumCores = 0;
		psDevInfo->ui32MultiCorePrimaryId = (IMG_UINT32)(-1);
	}
	psDeviceNode->pfnGetMultiCoreInfo = NULL;
}
