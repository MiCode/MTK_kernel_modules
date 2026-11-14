/*************************************************************************/ /*!
@File           vz_vmm_pvz.c
@Title          VM manager para-virtualization APIs
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    VM manager para-virtualization management
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

#include "pvrsrv.h"
#include "img_defs.h"
#include "img_types.h"
#include "pvrsrv_error.h"
#include "allocmem.h"
#include "pvrsrv.h"
#include "vz_vmm_pvz.h"
#include "vmm_impl.h"

void PvzServerLockAcquire(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	OSLockAcquire(psPVRSRVData->psPvzConfig->hPvzServerLock);
}

void PvzServerLockRelease(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	OSLockRelease(psPVRSRVData->psPvzConfig->hPvzServerLock);
}

void PvzClientLockAcquire(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	OSLockAcquire(psPVRSRVData->psPvzConfig->hPvzClientLock);
}

void PvzClientLockRelease(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	OSLockRelease(psPVRSRVData->psPvzConfig->hPvzClientLock);
}

PVRSRV_ERROR PvzConfigInit(void)
{
	PVRSRV_ERROR eError;
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();

#if defined(SUPPORT_AUTOVZ)
	/*
	 *  AutoVz setup: no paravirtualisation support
	 */
	PVR_DPF((PVR_DBG_MESSAGE, "%s: AutoVz setup", __func__));
#elif defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS)
	/*
	 *  Static PVZ setup
	 *
	 *  This setup uses carve-out memory, has no hypercall mechanism & does not support
	 *  out-of-order initialisation of host/guest VMs/drivers. The host driver has all
	 *  the information needed to initialize all Drivers firmware state when it's loaded
	 *  and its PVZ layer must mark all guest Drivers as being online as part of its PVZ
	 *  initialisation. Having no out-of-order initialisation support, the guest driver
	 *  can only submit a workload to the device after the host driver has completely
	 *  initialized the firmware, the VZ hypervisor/VM setup must guarantee this.
	 */
	PVR_DPF((PVR_DBG_MESSAGE, "%s: Using static memory setup", __func__));
#else
	/*
	 *  Dynamic PVZ setup
	 *
	 *  This setup uses guest memory, has PVZ hypercall mechanism & supports out-of-order
	 *  initialisation of host/guest VMs/drivers. The host driver initializes only its
	 *  own Driver-0 firmware state when it's loaded and each guest driver will use its PVZ
	 *  interface to hypercall to the host driver to both synchronise its initialisation
	 *  so it does not submit any workload to the firmware before the host driver has
	 *  had a chance to initialize the firmware and to also initialize its own Driver-x
	 *  firmware state.
	 */
	PVR_DPF((PVR_DBG_MESSAGE, "%s: Using dynamic memory setup", __func__));
#endif

	psPVRSRVData->psPvzConfig = OSAllocZMemNoStats(sizeof(PVRSRV_PVZ_CONFIG));
	PVR_GOTO_IF_NOMEM(psPVRSRVData->psPvzConfig, eError, Error);

	eError = OSLockCreate(&psPVRSRVData->psPvzConfig->hPvzServerLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(hPvzServerLock)", Error);

	eError = OSLockCreate(&psPVRSRVData->psPvzConfig->hPvzClientLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(hPvzClientLock)", Error);

	return eError;
Error:
	PvzConfigDeInit();

	return eError;
}

void PvzConfigDeInit(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_PVZ_CONFIG *psPvzConfig = psPVRSRVData->psPvzConfig;

	if (psPvzConfig != NULL)
	{
		if (psPvzConfig->hPvzServerLock != NULL)
		{
			OSLockDestroy(psPvzConfig->hPvzServerLock);
			psPvzConfig->hPvzServerLock = NULL;
		}

		if (psPvzConfig->hPvzClientLock != NULL)
		{
			OSLockDestroy(psPvzConfig->hPvzClientLock);
			psPvzConfig->hPvzClientLock = NULL;
		}

		if (psPvzConfig->hPvzServerConnection != NULL)
		{
			VMMDestroyPvzServerConnection(&psPvzConfig->hPvzServerConnection);
		}

		if (psPvzConfig->hPvzClientConnection != NULL)
		{
			VMMDestroyPvzClientConnection(&psPvzConfig->hPvzClientConnection);
		}

		OSFreeMemNoStats(psPvzConfig);
	}
}

PVRSRV_ERROR PvzConnectionInit(PVRSRV_DRIVER_MODE eDriverMode)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_PVZ_CONFIG *psPvzConfig = psPVRSRVData->psPvzConfig;

	switch (eDriverMode)
	{
		case DRIVER_MODE_HOST:
			if (psPvzConfig->hPvzServerConnection == NULL)
			{
				eError = VMMCreatePvzServerConnection(&psPvzConfig->hPvzServerConnection);
			}
			PVR_LOG_RETURN_IF_ERROR(eError, "VMMCreatePvzServerConnection");
			break;
		case DRIVER_MODE_GUEST:
			if (psPvzConfig->hPvzClientConnection == NULL)
			{
				eError = VMMCreatePvzClientConnection(&psPvzConfig->hPvzClientConnection);
			}
			PVR_LOG_RETURN_IF_ERROR(eError, "VMMCreatePvzClientConnection");
			break;
		default:
			/* Virtualization services not needed */
			break;
	}

	return eError;
}


/******************************************************************************
 End of file (vz_vmm_pvz.c)
******************************************************************************/
