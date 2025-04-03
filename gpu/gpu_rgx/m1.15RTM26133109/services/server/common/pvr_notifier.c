/*************************************************************************/ /*!
@File
@Title          PowerVR notifier interface
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

#include "img_defs.h"
#include "allocmem.h"
#include "dllist.h"

#include "device.h"
#include "pvr_notifier.h"
#include "pvrsrv.h"
#include "pvrversion.h"
#include "connection_server.h"

#include "osfunc.h"
#include "sofunc_pvr.h"

#define PVR_DUMP_DRIVER_INFO(x, y)														\
	PVR_DUMPDEBUG_LOG("%s info: %d.%d @ %8d (%s) build options: 0x%08x",				\
					   (x),																\
					   PVRVERSION_UNPACK_MAJ((y).ui32BuildVersion),						\
					   PVRVERSION_UNPACK_MIN((y).ui32BuildVersion),						\
					   (y).ui32BuildRevision,											\
					   (BUILD_TYPE_DEBUG == (y).ui32BuildType) ? "debug":"release",		\
					   (y).ui32BuildOptions);

#if !defined(WINDOW_SYSTEM)
#define WINDOW_SYSTEM "Unknown"
#endif

#define IS_DECLARED(x) (x[0] != '\0')

/*************************************************************************/ /*!
Command Complete Notifier Interface
*/ /**************************************************************************/

typedef struct PVRSRV_CMDCOMP_NOTIFY_TAG
{
	PVRSRV_CMDCOMP_HANDLE	hCmdCompHandle;
	PFN_CMDCOMP_NOTIFY		pfnCmdCompleteNotify;
	DLLIST_NODE				sListNode;
} PVRSRV_CMDCOMP_NOTIFY;

/* Head of the list of callbacks called when command complete happens */
static DLLIST_NODE g_sCmdCompNotifyHead;
static POSWR_LOCK g_hCmdCompNotifyLock;

PVRSRV_ERROR
PVRSRVCmdCompleteInit(void)
{
	PVRSRV_ERROR eError;

	eError = OSWRLockCreate(&g_hCmdCompNotifyLock);
	PVR_RETURN_IF_ERROR(eError);

	dllist_init(&g_sCmdCompNotifyHead);

	return PVRSRV_OK;
}

void
PVRSRVCmdCompleteDeinit(void)
{
	/* Check that all notify function have been unregistered */
	if (!dllist_is_empty(&g_sCmdCompNotifyHead))
	{
		PDLLIST_NODE psNode;

		PVR_DPF((PVR_DBG_ERROR,
				"%s: Command complete notify list is not empty!", __func__));

		/* Clean up any stragglers */
		psNode = dllist_get_next_node(&g_sCmdCompNotifyHead);
		while (psNode)
		{
			PVRSRV_CMDCOMP_NOTIFY *psNotify;

			dllist_remove_node(psNode);

			psNotify = IMG_CONTAINER_OF(psNode, PVRSRV_CMDCOMP_NOTIFY, sListNode);
			OSFreeMem(psNotify);

			psNode = dllist_get_next_node(&g_sCmdCompNotifyHead);
		}
	}

	if (g_hCmdCompNotifyLock)
	{
		OSWRLockDestroy(g_hCmdCompNotifyLock);
	}
}

PVRSRV_ERROR
PVRSRVRegisterCmdCompleteNotify(IMG_HANDLE *phNotify,
								PFN_CMDCOMP_NOTIFY pfnCmdCompleteNotify,
								PVRSRV_CMDCOMP_HANDLE hCmdCompHandle)
{
	PVRSRV_CMDCOMP_NOTIFY *psNotify;

	PVR_LOG_RETURN_IF_INVALID_PARAM(phNotify, "phNotify");
	PVR_LOG_RETURN_IF_INVALID_PARAM(pfnCmdCompleteNotify, "pfnCmdCompleteNotify");
	PVR_LOG_RETURN_IF_INVALID_PARAM(hCmdCompHandle, "hCmdCompHandle");

	psNotify = OSAllocMem(sizeof(*psNotify));
	PVR_LOG_RETURN_IF_NOMEM(psNotify, "psNotify");

	/* Set-up the notify data */
	psNotify->hCmdCompHandle = hCmdCompHandle;
	psNotify->pfnCmdCompleteNotify = pfnCmdCompleteNotify;

	/* Add it to the list of Notify functions */
	OSWRLockAcquireWrite(g_hCmdCompNotifyLock);
	dllist_add_to_tail(&g_sCmdCompNotifyHead, &psNotify->sListNode);
	OSWRLockReleaseWrite(g_hCmdCompNotifyLock);

	*phNotify = psNotify;

	return PVRSRV_OK;
}

PVRSRV_ERROR
PVRSRVUnregisterCmdCompleteNotify(IMG_HANDLE hNotify)
{
	PVRSRV_CMDCOMP_NOTIFY *psNotify;

	psNotify = (PVRSRV_CMDCOMP_NOTIFY *) hNotify;
	PVR_LOG_RETURN_IF_INVALID_PARAM(psNotify, "hNotify");

	OSWRLockAcquireWrite(g_hCmdCompNotifyLock);
	dllist_remove_node(&psNotify->sListNode);
	OSWRLockReleaseWrite(g_hCmdCompNotifyLock);

	OSFreeMem(psNotify);

	return PVRSRV_OK;
}

void
PVRSRVNotifyCommandCompletion(PVRSRV_CMDCOMP_HANDLE hCmdCompCallerHandle)
{
#if !defined(NO_HARDWARE)
	DLLIST_NODE *psNode, *psNext;

	/* Call notify callbacks to check if blocked work items can now proceed */
	OSWRLockAcquireRead(g_hCmdCompNotifyLock);
	dllist_foreach_node(&g_sCmdCompNotifyHead, psNode, psNext)
	{
		PVRSRV_CMDCOMP_NOTIFY *psNotify =
			IMG_CONTAINER_OF(psNode, PVRSRV_CMDCOMP_NOTIFY, sListNode);

		if (hCmdCompCallerHandle != psNotify->hCmdCompHandle)
		{
			psNotify->pfnCmdCompleteNotify(psNotify->hCmdCompHandle);
		}
	}
	OSWRLockReleaseRead(g_hCmdCompNotifyLock);
#endif
}

inline void
PVRSRVSignalGlobalEO(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();

	if (psPVRSRVData->hGlobalEventObject)
	{
		OSEventObjectSignal(psPVRSRVData->hGlobalEventObject);
	}
}

inline void
PVRSRVCheckStatus(PVRSRV_CMDCOMP_HANDLE hCmdCompCallerHandle)
{
	PVRSRVNotifyCommandCompletion(hCmdCompCallerHandle);
	PVRSRVSignalGlobalEO();
}

/*************************************************************************/ /*!
Debug Notifier Interface
*/ /**************************************************************************/

typedef struct DEBUG_REQUEST_ENTRY_TAG
{
	IMG_UINT32		ui32RequesterID;
	DLLIST_NODE		sListHead;
} DEBUG_REQUEST_ENTRY;

typedef struct DEBUG_REQUEST_TABLE_TAG
{
	POSWR_LOCK				hLock;
	IMG_UINT32				ui32RequestCount;
	DEBUG_REQUEST_ENTRY		asEntry[1];
} DEBUG_REQUEST_TABLE;

typedef struct DEBUG_REQUEST_NOTIFY_TAG
{
	PVRSRV_DEVICE_NODE		*psDevNode;
	PVRSRV_DBGREQ_HANDLE	hDbgRequestHandle;
	PFN_DBGREQ_NOTIFY		pfnDbgRequestNotify;
	IMG_UINT32				ui32RequesterID;
	DLLIST_NODE				sListNode;
} DEBUG_REQUEST_NOTIFY;


PVRSRV_ERROR
PVRSRVRegisterDbgTable(PVRSRV_DEVICE_NODE *psDevNode,
						const IMG_UINT32 *paui32Table, IMG_UINT32 ui32Length)
{
	DEBUG_REQUEST_TABLE *psDebugTable;
	IMG_UINT32 i;
	PVRSRV_ERROR eError;

	if (psDevNode->hDebugTable)
	{
		return PVRSRV_ERROR_DBGTABLE_ALREADY_REGISTERED;
	}

	psDebugTable = OSAllocMem(sizeof(DEBUG_REQUEST_TABLE) +
							  (sizeof(DEBUG_REQUEST_ENTRY) * (ui32Length-1)));
	PVR_RETURN_IF_NOMEM(psDebugTable);

	eError = OSWRLockCreate(&psDebugTable->hLock);
	PVR_GOTO_IF_ERROR(eError, ErrorFreeDebugTable);

	psDebugTable->ui32RequestCount = ui32Length;

	/* Init the list heads */
	for (i = 0; i < ui32Length; i++)
	{
		psDebugTable->asEntry[i].ui32RequesterID = paui32Table[i];
		dllist_init(&psDebugTable->asEntry[i].sListHead);
	}

	psDevNode->hDebugTable = (IMG_HANDLE *) psDebugTable;

	return PVRSRV_OK;

ErrorFreeDebugTable:
	OSFreeMem(psDebugTable);

	return eError;
}

void
PVRSRVUnregisterDbgTable(PVRSRV_DEVICE_NODE *psDevNode)
{
	DEBUG_REQUEST_TABLE *psDebugTable;
	IMG_UINT32 i;

	PVR_ASSERT(psDevNode->hDebugTable);
	psDebugTable = (DEBUG_REQUEST_TABLE *) psDevNode->hDebugTable;
	psDevNode->hDebugTable = NULL;

	for (i = 0; i < psDebugTable->ui32RequestCount; i++)
	{
		if (!dllist_is_empty(&psDebugTable->asEntry[i].sListHead))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Found registered callback(s) on %d",
					 __func__, i));
		}
	}

	OSWRLockDestroy(psDebugTable->hLock);
	psDebugTable->hLock = NULL;

	OSFreeMem(psDebugTable);
}

PVRSRV_ERROR
PVRSRVRegisterDbgRequestNotify(IMG_HANDLE *phNotify,
							   PVRSRV_DEVICE_NODE *psDevNode,
							   PFN_DBGREQ_NOTIFY pfnDbgRequestNotify,
							   IMG_UINT32 ui32RequesterID,
							   PVRSRV_DBGREQ_HANDLE hDbgRequestHandle)
{
	DEBUG_REQUEST_TABLE *psDebugTable;
	DEBUG_REQUEST_NOTIFY *psNotify;
	PDLLIST_NODE psHead = NULL;
	IMG_UINT32 i;
	PVRSRV_ERROR eError;

	PVR_LOG_RETURN_IF_INVALID_PARAM(phNotify, "phNotify");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevNode, "psDevNode");
	PVR_LOG_RETURN_IF_INVALID_PARAM(pfnDbgRequestNotify, "pfnDbRequestNotify");

	psDebugTable = (DEBUG_REQUEST_TABLE *) psDevNode->hDebugTable;

	PVR_ASSERT(psDebugTable);

	/* NoStats used since this may be called outside of the register/de-register
	 * process calls which track memory use. */
	psNotify = OSAllocMemNoStats(sizeof(*psNotify));
	PVR_LOG_RETURN_IF_NOMEM(psNotify, "psNotify");

	/* Set-up the notify data */
	psNotify->psDevNode = psDevNode;
	psNotify->hDbgRequestHandle = hDbgRequestHandle;
	psNotify->pfnDbgRequestNotify = pfnDbgRequestNotify;
	psNotify->ui32RequesterID = ui32RequesterID;

	/* Lock down all the lists */
	OSWRLockAcquireWrite(psDebugTable->hLock);

	/* Find which list to add it to */
	for (i = 0; i < psDebugTable->ui32RequestCount; i++)
	{
		if (psDebugTable->asEntry[i].ui32RequesterID == ui32RequesterID)
		{
			psHead = &psDebugTable->asEntry[i].sListHead;
		}
	}

	/* Failed to find debug requester */
	PVR_LOG_GOTO_IF_INVALID_PARAM(psHead, eError, ErrorReleaseLock);

	/* Add it to the list of Notify functions */
	dllist_add_to_tail(psHead, &psNotify->sListNode);

	/* Unlock the lists */
	OSWRLockReleaseWrite(psDebugTable->hLock);

	*phNotify = psNotify;

	return PVRSRV_OK;

ErrorReleaseLock:
	OSWRLockReleaseWrite(psDebugTable->hLock);
	OSFreeMem(psNotify);

	return eError;
}

PVRSRV_ERROR
SOPvrDbgRequestNotifyRegister(IMG_HANDLE *phNotify,
							  PVRSRV_DEVICE_NODE *psDevNode,
							  PFN_DBGREQ_NOTIFY pfnDbgRequestNotify,
							  IMG_UINT32 ui32RequesterID,
							  PVRSRV_DBGREQ_HANDLE hDbgRequestHandle)
{
	return PVRSRVRegisterDbgRequestNotify(phNotify,
			psDevNode,
			pfnDbgRequestNotify,
			ui32RequesterID,
			hDbgRequestHandle);
}

PVRSRV_ERROR
PVRSRVUnregisterDbgRequestNotify(IMG_HANDLE hNotify)
{
	DEBUG_REQUEST_NOTIFY *psNotify = (DEBUG_REQUEST_NOTIFY *) hNotify;
	DEBUG_REQUEST_TABLE *psDebugTable;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psNotify, "psNotify");

	psDebugTable = (DEBUG_REQUEST_TABLE *) psNotify->psDevNode->hDebugTable;

	OSWRLockAcquireWrite(psDebugTable->hLock);
	dllist_remove_node(&psNotify->sListNode);
	OSWRLockReleaseWrite(psDebugTable->hLock);

	OSFreeMemNoStats(psNotify);

	return PVRSRV_OK;
}

PVRSRV_ERROR
SOPvrDbgRequestNotifyUnregister(IMG_HANDLE hNotify)
{
	return PVRSRVUnregisterDbgRequestNotify(hNotify);
}

#if defined(MTK_DEBUG_PROC_PRINT)
IMG_BOOL bQuiet;
IMG_BOOL MTK_PVRSRVDebugRequestGetSilence(void)
{
	return bQuiet;
}

void MTK_PVRSRVDebugRequestSetSilence(IMG_BOOL bEnable)
{
	bQuiet = bEnable;
	if (bQuiet == IMG_TRUE)
		g_use_id = MTKPP_ID_SHOT_FW;
	else
		g_use_id = MTKPP_ID_FW;
}
#endif /* MTK_DEBUG_PROC_PRINT */

void
PVRSRVDebugRequest(PVRSRV_DEVICE_NODE *psDevNode,
				   IMG_UINT32 ui32VerbLevel,
				   DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				   void *pvDumpDebugFile)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	DEBUG_REQUEST_TABLE *psDebugTable =
		(DEBUG_REQUEST_TABLE *) psDevNode->hDebugTable;
	static const IMG_CHAR *apszVerbosityTable[] = { "Low", "Medium", "High" };
	const IMG_CHAR *szVerbosityLevel;
	const IMG_CHAR *Bit32 = "32 Bit", *Bit64 = "64 Bit";
	IMG_UINT32 i;

	static_assert(ARRAY_SIZE(apszVerbosityTable) == DEBUG_REQUEST_VERBOSITY_MAX+1,
	              "Incorrect number of verbosity levels");

	PVR_ASSERT(psDebugTable);

	OSWRLockAcquireRead(psDebugTable->hLock);

	if (ui32VerbLevel < ARRAY_SIZE(apszVerbosityTable))
	{
		szVerbosityLevel = apszVerbosityTable[ui32VerbLevel];
	}
	else
	{
		szVerbosityLevel = "unknown";
		PVR_ASSERT(!"Invalid verbosity level received");
	}

	PVR_DUMPDEBUG_LOG("------------[ PVR DBG: START (%s) ]------------",
	                  szVerbosityLevel);

	OSDumpVersionInfo(pfnDumpDebugPrintf, pvDumpDebugFile);

	PVR_DUMPDEBUG_LOG("DDK info: %s (%s) %s",
	                  PVRVERSION_STRING, PVR_BUILD_TYPE, PVR_BUILD_DIR);

	PVR_DUMPDEBUG_LOG("Time now: %" IMG_UINT64_FMTSPEC "us",
	                  OSClockus64());

#if defined(MTK_DEBUG_PROC_PRINT)
	if (!pfnDumpDebugPrintf) {
		MTKPP_LOGTIME(g_use_id, "Dump Debug Data");
	}
#endif /* MTK_DEBUG_PROC_PRINT */

	switch (psPVRSRVData->eServicesState)
	{
		case PVRSRV_SERVICES_STATE_OK:
			PVR_DUMPDEBUG_LOG("Services State: OK");
			break;
		case PVRSRV_SERVICES_STATE_BAD:
			PVR_DUMPDEBUG_LOG("Services State: BAD");
			break;
		case PVRSRV_SERVICES_STATE_UNDEFINED:
			PVR_DUMPDEBUG_LOG("Services State: UNDEFINED");
			break;
		default:
			PVR_DUMPDEBUG_LOG("Services State: UNKNOWN (%d)",
			                  psPVRSRVData->eServicesState);
			break;
	}

	PVR_DUMPDEBUG_LOG("Server Errors: %d",
	          PVRSRV_KM_ERRORS);

	PVRSRVConnectionDebugNotify(pfnDumpDebugPrintf, pvDumpDebugFile);

	PVR_DUMPDEBUG_LOG("------[ Driver Info ]------");

	PVR_DUMPDEBUG_LOG("Comparison of UM/KM components: %s",
	                 (psPVRSRVData->sDriverInfo.bIsNoMatch) ? "MISMATCH" : "MATCHING");

	PVR_DUMPDEBUG_LOG("KM Arch: %s",
	                  (psPVRSRVData->sDriverInfo.ui8KMBitArch & BUILD_ARCH_64BIT) ? Bit64 : Bit32);

	if (!PVRSRV_VZ_MODE_IS(NATIVE))
	{
		PVR_DUMPDEBUG_LOG("Driver Mode: %s",
		                  (PVRSRV_VZ_MODE_IS(HOST)) ? "Host":"Guest");
	}

	if (psPVRSRVData->sDriverInfo.ui8UMSupportedArch)
	{
		if ((psPVRSRVData->sDriverInfo.ui8UMSupportedArch & BUILD_ARCH_BOTH) ==
			BUILD_ARCH_BOTH)
		{
			PVR_DUMPDEBUG_LOG("UM Connected Clients Arch: %s and %s", Bit64, Bit32);

		}else
		{
			PVR_DUMPDEBUG_LOG("UM Connected Clients: %s",
			                  (psPVRSRVData->sDriverInfo.ui8UMSupportedArch & BUILD_ARCH_64BIT) ? Bit64 : Bit32);
		}
	}

	PVR_DUMP_DRIVER_INFO("UM", psPVRSRVData->sDriverInfo.sUMBuildInfo);
	PVR_DUMP_DRIVER_INFO("KM", psPVRSRVData->sDriverInfo.sKMBuildInfo);

	PVR_DUMPDEBUG_LOG("Window system: %s", (IS_DECLARED(WINDOW_SYSTEM)) ? (WINDOW_SYSTEM) : "Not declared");

	/* For each requester */
	for (i = 0; i < psDebugTable->ui32RequestCount; i++)
	{
		DLLIST_NODE *psNode;
		DLLIST_NODE *psNext;

		/* For each notifier on this requestor */
		dllist_foreach_node(&psDebugTable->asEntry[i].sListHead, psNode, psNext)
		{
			DEBUG_REQUEST_NOTIFY *psNotify =
				IMG_CONTAINER_OF(psNode, DEBUG_REQUEST_NOTIFY, sListNode);
			psNotify->pfnDbgRequestNotify(psNotify->hDbgRequestHandle, ui32VerbLevel,
							pfnDumpDebugPrintf, pvDumpDebugFile);
		}
	}

	PVR_DUMPDEBUG_LOG("------------[ PVR DBG: END ]------------");
	OSWRLockReleaseRead(psDebugTable->hLock);

	if (!pfnDumpDebugPrintf)
	{
		/* Only notify OS of an issue if the debug dump has gone there */
		OSWarnOn(IMG_TRUE);
	}
}
