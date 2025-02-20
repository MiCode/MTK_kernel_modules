/*************************************************************************/ /*!
@File           physheap.c
@Title          Physical heap management
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Management functions for the physical heap(s). A heap contains
                all the information required by services when using memory from
                that heap (such as CPU <> Device physical address translation).
                A system must register one heap but can have more then one which
                is why a heap must register with a (system) unique ID.
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
*/ /***************************************************************************/
#include "img_types.h"
#include "img_defs.h"
#include "physheap.h"
#include "allocmem.h"
#include "pvr_debug.h"
#include "osfunc.h"
#include "pvrsrv.h"
#include "physmem.h"
#include "physmem_hostmem.h"
#include "physmem_lma.h"
#include "physmem_dlm.h"
#include "physmem_ima.h"
#include "physmem_osmem.h"
#include "debug_common.h"

struct _PHYS_HEAP_
{
	/*! The type of this heap */
	PHYS_HEAP_TYPE			eType;

	/*! The allocation policy for this heap */
	PHYS_HEAP_POLICY uiPolicy;

	/* Config flags */
	PHYS_HEAP_USAGE_FLAGS		ui32UsageFlags;

	/* OOM Detection state */
#if !defined(PVRSRV_PHYSHEAP_DISABLE_OOM_DEMOTION)
	ATOMIC_T					sOOMDetected;
#endif

	/*! Pointer to device node struct */
	PPVRSRV_DEVICE_NODE         psDevNode;
	/*! PDump name of this physical memory heap */
	IMG_CHAR					*pszPDumpMemspaceName;
	/*! Physheap name of this physical memory heap */
	IMG_CHAR					aszName[PHYS_HEAP_NAME_SIZE];
	/*! Private data for the translate routines */
	IMG_HANDLE					hPrivData;
	/*! Function callbacks */
	PHYS_HEAP_FUNCTIONS			*psMemFuncs;

	/*! Refcount */
	IMG_UINT32					ui32RefCount;

	/*! Implementation specific */
	PHEAP_IMPL_DATA             pvImplData;
	PHEAP_IMPL_FUNCS            *psImplFuncs;

	/*! Pointer to next physical heap */
	struct _PHYS_HEAP_		*psNext;

#if defined(PVRSRV_SUPPORT_IPA_FEATURE)
	/*! IPA Policy value from Heap Config */
	IMG_UINT32                  ui32IPAPolicyValue;

	/*! IPA Clear Mask value from Heap Config */
	IMG_UINT32                  ui32IPAClearMask;

	/*! IPA Bit Shift value from Heap Config */
	IMG_UINT32                  ui32IPAShift;
#endif /* defined(PVRSRV_SUPPORT_IPA_FEATURE) */
};

#if defined(REFCOUNT_DEBUG)
#define PHYSHEAP_REFCOUNT_PRINT(fmt, ...)	\
	PVRSRVDebugPrintf(PVR_DBG_WARNING,	\
			  __FILE__,		\
			  __LINE__,		\
			  fmt,			\
			  __VA_ARGS__)
#else
#define PHYSHEAP_REFCOUNT_PRINT(fmt, ...)
#endif

#define IsOOMError(err) ((err == PVRSRV_ERROR_PMR_FAILED_TO_ALLOC_PAGES) | \
						 (err == PVRSRV_ERROR_OUT_OF_MEMORY) | \
						 (err == PVRSRV_ERROR_PMR_TOO_LARGE))

typedef enum _PVR_LAYER_HEAP_ACTION_
{
	PVR_LAYER_HEAP_ACTION_IGNORE,          /* skip heap during heap init */
	PVR_LAYER_HEAP_ACTION_INSTANTIATE,     /* instantiate heap but don't acquire */
	PVR_LAYER_HEAP_ACTION_INITIALISE       /* instantiate and acquire */

} PVR_LAYER_HEAP_ACTION;

typedef struct PHYS_HEAP_PROPERTIES_TAG
{
	PVRSRV_PHYS_HEAP eFallbackHeap;
	PVR_LAYER_HEAP_ACTION ePVRLayerAction;
	IMG_BOOL bUserModeAlloc;
} PHYS_HEAP_PROPERTIES;

/* NOTE: Table entries and order must match enum PVRSRV_PHYS_HEAP to ensure
 * correct operation of PhysHeapCreatePMR().
 */
static PHYS_HEAP_PROPERTIES gasHeapProperties[PVRSRV_PHYS_HEAP_LAST] =
{
	/* eFallbackHeap,               ePVRLayerAction,                   bUserModeAlloc */
    {  PVRSRV_PHYS_HEAP_DEFAULT,    PVR_LAYER_HEAP_ACTION_INITIALISE,  IMG_TRUE  }, /* DEFAULT */
    {  PVRSRV_PHYS_HEAP_DEFAULT,    PVR_LAYER_HEAP_ACTION_INITIALISE,  IMG_TRUE  }, /* CPU_LOCAL */
    {  PVRSRV_PHYS_HEAP_DEFAULT,    PVR_LAYER_HEAP_ACTION_INITIALISE,  IMG_TRUE  }, /* GPU_LOCAL */
    {  PVRSRV_PHYS_HEAP_GPU_LOCAL,  PVR_LAYER_HEAP_ACTION_INITIALISE,  IMG_TRUE  }, /* GPU_PRIVATE */
    {  PVRSRV_PHYS_HEAP_GPU_LOCAL,  PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_MAIN */
    {  PVRSRV_PHYS_HEAP_GPU_LOCAL,  PVR_LAYER_HEAP_ACTION_INITIALISE,  IMG_FALSE }, /* EXTERNAL */
    {  PVRSRV_PHYS_HEAP_GPU_LOCAL,  PVR_LAYER_HEAP_ACTION_INITIALISE,  IMG_FALSE }, /* GPU_COHERENT */
    {  PVRSRV_PHYS_HEAP_GPU_LOCAL,  PVR_LAYER_HEAP_ACTION_INITIALISE,  IMG_TRUE  }, /* GPU_SECURE */
    {  PVRSRV_PHYS_HEAP_GPU_LOCAL,  PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_CONFIG */
    {  PVRSRV_PHYS_HEAP_FW_MAIN,    PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_CODE */
    {  PVRSRV_PHYS_HEAP_FW_MAIN,    PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_PRIV_DATA */
    {  PVRSRV_PHYS_HEAP_GPU_LOCAL,  PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_PREMAP_PT */
    {  PVRSRV_PHYS_HEAP_FW_PREMAP0, PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_PREMAP0 */
    {  PVRSRV_PHYS_HEAP_FW_PREMAP1, PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_PREMAP1 */
    {  PVRSRV_PHYS_HEAP_FW_PREMAP2, PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_PREMAP2 */
    {  PVRSRV_PHYS_HEAP_FW_PREMAP3, PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_PREMAP3 */
    {  PVRSRV_PHYS_HEAP_FW_PREMAP4, PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_PREMAP4 */
    {  PVRSRV_PHYS_HEAP_FW_PREMAP5, PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_PREMAP5 */
    {  PVRSRV_PHYS_HEAP_FW_PREMAP6, PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_PREMAP6 */
    {  PVRSRV_PHYS_HEAP_FW_PREMAP7, PVR_LAYER_HEAP_ACTION_IGNORE,      IMG_FALSE }, /* FW_PREMAP7 */
    {  PVRSRV_PHYS_HEAP_WRAP,       PVR_LAYER_HEAP_ACTION_INSTANTIATE, IMG_FALSE }, /* WRAP */
    {  PVRSRV_PHYS_HEAP_DISPLAY,    PVR_LAYER_HEAP_ACTION_INSTANTIATE, IMG_FALSE }, /* DISPLAY */
};

static_assert((ARRAY_SIZE(gasHeapProperties) == PVRSRV_PHYS_HEAP_LAST),
	"Size or order of gasHeapProperties entries incorrect for PVRSRV_PHYS_HEAP enum");

static IMG_BOOL PhysHeapCreatedByPVRLayer(PVRSRV_PHYS_HEAP ePhysHeap);
static IMG_BOOL PhysHeapAcquiredByPVRLayer(PVRSRV_PHYS_HEAP ePhysHeap);

/**
 * ! IMPORTANT !
 * Do not change this string array unless the usage flag definitions in
 * physheap_config.h have changed.
 *
 * NOTE: Use DebugCommonFlagStrings or GetPhysHeapUsageString to get
 * usage flags string.
 */
static const IMG_FLAGS2DESC g_asPhysHeapUsageFlagStrings[] =
{
	{PHYS_HEAP_USAGE_CPU_LOCAL,		"CPU_LOCAL"},
	{PHYS_HEAP_USAGE_GPU_LOCAL,		"GPU_LOCAL"},
	{PHYS_HEAP_USAGE_GPU_PRIVATE,	"GPU_PRIVATE"},
	{PHYS_HEAP_USAGE_EXTERNAL,		"EXTERNAL"},
	{PHYS_HEAP_USAGE_GPU_COHERENT,	"GPU_COHERENT"},
	{PHYS_HEAP_USAGE_GPU_SECURE,	"GPU_SECURE"},
	{PHYS_HEAP_USAGE_FW_SHARED,		"FW_SHARED"},
	{PHYS_HEAP_USAGE_FW_PRIVATE,	"FW_PRIVATE"},
	{PHYS_HEAP_USAGE_FW_CODE,		"FW_CODE"},
	{PHYS_HEAP_USAGE_FW_PRIV_DATA,	"FW_PRIV_DATA"},
	{PHYS_HEAP_USAGE_FW_PREMAP_PT,	"FW_PREMAP_PT"},
	{PHYS_HEAP_USAGE_FW_PREMAP,		"FW_PREMAP"},
	{PHYS_HEAP_USAGE_WRAP,			"WRAP"},
	{PHYS_HEAP_USAGE_DISPLAY,		"DISPLAY"},
	{PHYS_HEAP_USAGE_DLM,			"DLM"}
};

/*************************************************************************/ /*!
@Function       PhysHeapCheckValidUsageFlags
@Description    Checks if any bits were set outside of the valid ones within
                PHYS_HEAP_USAGE_FLAGS.

@Input          ui32PhysHeapUsage  The value of the usage flag.

@Return         True or False depending on whether there were only valid bits set.
*/ /**************************************************************************/
static inline IMG_BOOL PhysHeapCheckValidUsageFlags(PHYS_HEAP_USAGE_FLAGS ui32PhysHeapUsage)
{
	return !(ui32PhysHeapUsage & ~PHYS_HEAP_USAGE_MASK);
}

/*************************************************************************/ /*!
@Function       GetPhysHeapUsageString
@Description    This function is used to create a comma separated string of all
                usage flags passed in as a bitfield.

@Input          ui32UsageFlags     The bitfield of usage flags.
@Input          ui32Size           The size of the memory pointed to by
                                   pszUsageString.
@Output         pszUsageString     A pointer to memory where the created string
                                   will be stored.

@Return         If successful PVRSRV_OK, else a PVRSRV_ERROR.
*/ /**************************************************************************/
static PVRSRV_ERROR GetPhysHeapUsageString(PHYS_HEAP_USAGE_FLAGS ui32UsageFlags,
                                          IMG_UINT32 ui32Size,
                                          IMG_CHAR *const pszUsageString)
{
	IMG_UINT32 i;
	IMG_BOOL bFirst = IMG_TRUE;
	size_t uiSize = 0;

	PVR_LOG_RETURN_IF_INVALID_PARAM(pszUsageString != NULL, "pszUsageString");
	PVR_LOG_RETURN_IF_INVALID_PARAM(ui32Size > 0, "ui32Size");

	/* Initialise the string to be null terminated at the beginning */
	uiSize = OSStringLCopy(pszUsageString, "\0", sizeof(IMG_CHAR));

	if (ui32UsageFlags == 0)
	{
		uiSize = OSStringLCopy(pszUsageString, "NONE", (size_t)ui32Size);
		PVR_LOG_RETURN_IF_FALSE((uiSize < ui32Size), "OSStringLCopy", PVRSRV_ERROR_OUT_OF_MEMORY);

		return PVRSRV_OK;
	}

	/* Process from left to right. */
	for (i = (sizeof(PHYS_HEAP_USAGE_FLAGS) * BITS_PER_BYTE - 1); i > 0; i--)
	{
		IMG_UINT32 ui32Flag = BIT(i);

		if (BITMASK_HAS(ui32UsageFlags, ui32Flag))
		{
			IMG_CHAR pszString[32] = "\0";

			if (PhysHeapCheckValidUsageFlags(ui32Flag))
			{
				DebugCommonFlagStrings(pszString,
				                   sizeof(pszString),
				                   g_asPhysHeapUsageFlagStrings,
				                   ARRAY_SIZE(g_asPhysHeapUsageFlagStrings),
				                   ui32Flag);
			}
			else
			{
				uiSize = OSStringLCat(pszString,
				                      "INVALID",
				                      sizeof(pszString));
				PVR_LOG_RETURN_IF_FALSE((uiSize < sizeof(pszString)), "OSStringLCat", PVRSRV_ERROR_OUT_OF_MEMORY);
			}

			if (!bFirst)
			{
				uiSize = OSStringLCat(pszUsageString,
				                      ", ",
				                      (size_t)ui32Size);
				PVR_LOG_RETURN_IF_FALSE((uiSize < ui32Size), "OSStringLCat", PVRSRV_ERROR_OUT_OF_MEMORY);
			}
			else
			{
				bFirst = IMG_FALSE;
			}

			uiSize = OSStringLCat(pszUsageString,
			                      pszString,
			                      (size_t)ui32Size);
			PVR_LOG_RETURN_IF_FALSE((uiSize < ui32Size), "OSStringLCat", PVRSRV_ERROR_OUT_OF_MEMORY);
		}
	}

	return PVRSRV_OK;
}

/*************************************************************************/ /*!
@Function       PhysHeapPrintHeapProperties
@Description    This function is used to print properties
                of the specified physheap.

@Input          psPhysHeap          The physheap to create the string from.
@Input          pfnDumpDebugPrintf  The specified print function that should be
                                    used to dump any debug information
                                    (see PVRSRVDebugRequest).
@Input          pvDumpDebugFile     Optional file identifier to be passed to
                                    the print function if required.

@Return         If successful PVRSRV_OK, else a PVRSRV_ERROR.
*/ /**************************************************************************/
static PVRSRV_ERROR PhysHeapPrintHeapProperties(PHYS_HEAP *psPhysHeap,
                                                IMG_BOOL bDefaultHeap,
                                                DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                                                void *pvDumpDebugFile)
{
	static const IMG_CHAR *const pszTypeStrings[] = {
	#define X(_name) #_name,
		PHYS_HEAP_TYPE_LIST
	#undef X
	};

	IMG_UINT64 ui64TotalSize;
	IMG_UINT64 ui64FreeSize;
	IMG_CHAR pszUsageString[127] = "\0";
	PVRSRV_ERROR eError;

	if (psPhysHeap->eType >= ARRAY_SIZE(pszTypeStrings))
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "PhysHeap at address %p eType is not a PHYS_HEAP_TYPE",
		         psPhysHeap));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_HEAPINFO, failure);
	}

	psPhysHeap->psImplFuncs->pfnGetFactoryMemStats(psPhysHeap->pvImplData,
	                                               &ui64TotalSize,
	                                               &ui64FreeSize);

	eError = GetPhysHeapUsageString(psPhysHeap->ui32UsageFlags,
	                                sizeof(pszUsageString),
	                                pszUsageString);
	PVR_LOG_GOTO_IF_ERROR(eError, "GetPhysHeapUsageString", failure);

	if ((psPhysHeap->eType == PHYS_HEAP_TYPE_LMA) ||
	    (psPhysHeap->eType == PHYS_HEAP_TYPE_DLM_BAR) ||
		(psPhysHeap->eType == PHYS_HEAP_TYPE_DLM_PRIV))
	{
		IMG_CPU_PHYADDR sCPUPAddr;
		IMG_DEV_PHYADDR sGPUPAddr;

		PVR_ASSERT(psPhysHeap->psImplFuncs->pfnGetCPUPAddr != NULL);
		PVR_ASSERT(psPhysHeap->psImplFuncs->pfnGetDevPAddr != NULL);

		eError = psPhysHeap->psImplFuncs->pfnGetCPUPAddr(psPhysHeap->pvImplData,
		                                                 &sCPUPAddr);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "pfnGetCPUPAddr");
			sCPUPAddr.uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(IMG_UINT64_MAX);
		}

		eError = psPhysHeap->psImplFuncs->pfnGetDevPAddr(psPhysHeap->pvImplData,
		                                                 &sGPUPAddr);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "pfnGetDevPAddr");
			sGPUPAddr.uiAddr = IMG_UINT64_MAX;
		}

		PVR_DUMPDEBUG_LOG("0x%p -> PdMs: %s, Type: %s, %s, "
		                  "CPU PA Base: " CPUPHYADDR_UINT_FMTSPEC", "
		                  "GPU PA Base: 0x%08"IMG_UINT64_FMTSPECx", "
		                  "Usage Flags: 0x%08x (%s), Refs: %d, "
		                  "Free Size: %"IMG_UINT64_FMTSPEC"B, "
		                  "Total Size: %"IMG_UINT64_FMTSPEC"B",
		                  psPhysHeap,
		                  psPhysHeap->pszPDumpMemspaceName,
		                  pszTypeStrings[psPhysHeap->eType],
		                  bDefaultHeap ? "default" : "-",
		                  CPUPHYADDR_FMTARG(sCPUPAddr.uiAddr),
		                  sGPUPAddr.uiAddr,
		                  psPhysHeap->ui32UsageFlags,
		                  pszUsageString,
		                  psPhysHeap->ui32RefCount,
		                  ui64FreeSize,
		                  ui64TotalSize);
	}
	else if ((psPhysHeap->eType == PHYS_HEAP_TYPE_IMA_BAR) ||
	         (psPhysHeap->eType == PHYS_HEAP_TYPE_IMA_PRIV))
	{
		IMG_CHAR pszSpanString[128] = "\0";
		void *pvIterHandle = NULL;

		PVR_DUMPDEBUG_LOG("0x%p -> PdMs: %s, Type: %s, %s, "
		                  "Usage Flags: 0x%08x (%s), Refs: %d, "
		                  "Free Size: %"IMG_UINT64_FMTSPEC"B, "
		                  "Total Size: %"IMG_UINT64_FMTSPEC"B Spans:",
		                  psPhysHeap,
		                  psPhysHeap->pszPDumpMemspaceName,
		                  pszTypeStrings[psPhysHeap->eType],
		                  bDefaultHeap ? "default" : "-",
		                  psPhysHeap->ui32UsageFlags,
		                  pszUsageString,
		                  psPhysHeap->ui32RefCount,
		                  ui64FreeSize,
		                  ui64TotalSize);

		while (psPhysHeap->psImplFuncs->pfnGetHeapSpansStringIter(psPhysHeap->pvImplData,
		                                                                   pszSpanString,
		                                                                   sizeof(pszSpanString),
		                                                                   &pvIterHandle))
		{
			PVR_DUMPDEBUG_LOG("%s", pszSpanString);
		}
	}
	else
	{
		PVR_DUMPDEBUG_LOG("0x%p -> PdMs: %s, Type: %s, %s, "
		                  "Usage Flags: 0x%08x (%s), Refs: %d, "
		                  "Free Size: %"IMG_UINT64_FMTSPEC"B, "
		                  "Total Size: %"IMG_UINT64_FMTSPEC"B",
		                  psPhysHeap,
		                  psPhysHeap->pszPDumpMemspaceName,
		                  pszTypeStrings[psPhysHeap->eType],
		                  bDefaultHeap ? "default" : "-",
		                  psPhysHeap->ui32UsageFlags,
		                  pszUsageString,
		                  psPhysHeap->ui32RefCount,
		                  ui64FreeSize,
		                  ui64TotalSize);
	}

	return PVRSRV_OK;

failure:
	return eError;
}

/*************************************************************************/ /*!
@Function       PhysHeapDebugRequest
@Description    This function is used to output debug information for a given
                device's PhysHeaps.
@Input          pfnDbgRequestHandle Data required by this function that is
                                    passed through the RegisterDeviceDbgRequestNotify
                                    function.
@Input          ui32VerbLevel       The maximum verbosity of the debug request.
@Input          pfnDumpDebugPrintf  The specified print function that should be
                                    used to dump any debug information
                                    (see PVRSRVDebugRequest).
@Input          pvDumpDebugFile     Optional file identifier to be passed to
                                    the print function if required.
@Return         void
*/ /**************************************************************************/
static void PhysHeapDebugRequest(PVRSRV_DBGREQ_HANDLE pfnDbgRequestHandle,
                                  IMG_UINT32 ui32VerbLevel,
                                  DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                                  void *pvDumpDebugFile)
{
	PPVRSRV_DEVICE_NODE psDeviceNode = (PPVRSRV_DEVICE_NODE)pfnDbgRequestHandle;
	PHYS_HEAP *psPhysHeap;

	PVR_UNREFERENCED_PARAMETER(ui32VerbLevel);

	PVR_LOG_RETURN_VOID_IF_FALSE(psDeviceNode != NULL,
	                             "Phys Heap debug request failed. psDeviceNode was NULL");

	PVR_DUMPDEBUG_LOG("------[ Device ID: %d - Phys Heaps ]------",
	                  psDeviceNode->sDevId.i32KernelDeviceID);

	for (psPhysHeap = psDeviceNode->psPhysHeapList; psPhysHeap != NULL; psPhysHeap = psPhysHeap->psNext)
	{
		PVRSRV_ERROR eError = PVRSRV_OK;
		IMG_BOOL bDefaultHeap = psPhysHeap == psDeviceNode->apsPhysHeap[psDeviceNode->psDevConfig->eDefaultHeap];

		eError = PhysHeapPrintHeapProperties(psPhysHeap,
		                                     bDefaultHeap,
		                                     pfnDumpDebugPrintf,
		                                     pvDumpDebugFile);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "PhysHeapCreateProperties");
			continue;
		}
	}

#if defined(SUPPORT_PMR_DEFERRED_FREE)
	OSLockAcquire(psDeviceNode->hPMRZombieListLock);
	PVR_DUMPDEBUG_LOG("PMR Zombie Count: %u, PMR Zombie Count In Cleanup: %u",
	                  psDeviceNode->uiPMRZombieCount,
	                  psDeviceNode->uiPMRZombieCountInCleanup);
	OSLockRelease(psDeviceNode->hPMRZombieListLock);
#endif
	PVR_DUMPDEBUG_LOG("PMR Live Count: %d", PMRGetLiveCount());
}

/*************************************************************************/ /*!
@Function       HeapCfgUsedByPVRLayer
@Description    Checks if a physheap config must be handled by the PVR Layer
@Input          psConfig PhysHeapConfig
@Return         IMG_BOOL
*/ /**************************************************************************/
static IMG_BOOL HeapCfgUsedByPVRLayer(PHYS_HEAP_CONFIG *psConfig)
{
	PVRSRV_PHYS_HEAP eHeap;
	IMG_BOOL bPVRHeap = IMG_FALSE;

	/* Heaps are triaged for initialisation by either
	 * the PVR Layer or the device-specific heap handler. */
	for (eHeap = PVRSRV_PHYS_HEAP_DEFAULT;
		 eHeap < PVRSRV_PHYS_HEAP_LAST;
		 eHeap++)
	{
		if ((BIT_ISSET(psConfig->ui32UsageFlags, eHeap) &&
		     PhysHeapCreatedByPVRLayer(eHeap)))
		{
			bPVRHeap = IMG_TRUE;
			break;
		}
	}

	return bPVRHeap;
}

#if defined(PVRSRV_SUPPORT_DYNAMIC_PHYSHEAPS)
/*************************************************************************/ /*!
@Function       PhysHeapCreateDLMIMAHeapsFromConfig
@Description    Create new heaps for a device from DLM and IMA configs.
                This function will both create and construct the link
                between them.
@Input          psDevNode      Pointer to device node struct
@Input          pasConfigs     Pointer to array of Heap configurations.
@Input          ui32NumConfigs Number of configurations in array.
@Return         PVRSRV_ERROR PVRSRV_OK or error code
*/ /**************************************************************************/
static PVRSRV_ERROR
PhysHeapCreateDLMIMAHeapsFromConfig(PVRSRV_DEVICE_NODE *psDevNode,
                                    PHYS_HEAP_CONFIG *pasConfigs,
                                    IMG_UINT32 ui32NumConfigs)
{
	/* The DLM heaps must be created before IMA heaps so the configs are iterated twice
	 * as the config order is not well-defined. */

	PVRSRV_ERROR eError;
	IMG_UINT32 i;
	PHYS_HEAP *psDLMBarHeap, *psDLMPrivHeap;
	IMG_INT32 iDLMBarHeapIdx = -1, iDLMPrivHeapIdx = -1;

	/* First iterate and create the DLM heaps */
	for (i = 0; i < ui32NumConfigs; i++)
	{
		if (pasConfigs[i].eType == PHYS_HEAP_TYPE_DLM_BAR)
		{
			eError = PhysmemCreateHeapDLM(psDevNode,
			                               PHYS_HEAP_POLICY_DEFAULT,
			                               &pasConfigs[i],
			                               "GPU DLM (BAR)",
			                               &psDLMBarHeap);
			PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemCreateHeapDLM");
			iDLMBarHeapIdx = i;
		}
		else if (pasConfigs[i].eType == PHYS_HEAP_TYPE_DLM_PRIV)
		{
			eError = PhysmemCreateHeapDLM(psDevNode,
			                               PHYS_HEAP_POLICY_DEFAULT,
			                               &pasConfigs[i],
			                               "GPU DLM (PRIV)",
			                               &psDLMPrivHeap);
			PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemCreateHeapDLM");
			iDLMPrivHeapIdx = i;
		}
	}

	/* Then iterate and create the IMA heaps and link to DLM */
	for (i = 0; i < ui32NumConfigs; i++)
	{
		if (pasConfigs[i].eType == PHYS_HEAP_TYPE_IMA_BAR)
		{
			PVR_LOG_RETURN_IF_FALSE(psDLMBarHeap != NULL,
			                        "DLM Bar heap not defined when attempting to "
			                        "construct IMA heap",
			                        PVRSRV_ERROR_INVALID_HEAP);

			PhysmemCreateHeapIMA(psDevNode,
			                     PHYS_HEAP_POLICY_DEFAULT,
			                     &pasConfigs[i],
			                     pasConfigs[i].uConfig.sIMA.pszHeapName,
			                     psDLMBarHeap,
			                     MB2B(pasConfigs[iDLMBarHeapIdx].uConfig.sDLM.ui32DLMHeapPMBSizeMB),
			                     NULL);
			PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemCreateHeapIMA");
		}
		else if (pasConfigs[i].eType == PHYS_HEAP_TYPE_IMA_PRIV)
		{
			PVR_LOG_RETURN_IF_FALSE(psDLMPrivHeap != NULL,
			                        "DLM Priv heap not defined when attempting to "
			                        "construct IMA heap",
			                        PVRSRV_ERROR_INVALID_HEAP);

			PhysmemCreateHeapIMA(psDevNode,
			                     PHYS_HEAP_POLICY_DEFAULT,
			                     &pasConfigs[i],
			                     pasConfigs[i].uConfig.sIMA.pszHeapName,
			                     psDLMPrivHeap,
			                     MB2B(pasConfigs[iDLMPrivHeapIdx].uConfig.sDLM.ui32DLMHeapPMBSizeMB),
			                     NULL);
			PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemCreateHeapIMA");
		}
	}

	return PVRSRV_OK;
}

static void
PhysHeapDestroyDLMIMAHeaps(PVRSRV_DEVICE_NODE *psDevNode)
{
	PHYS_HEAP *psNode = psDevNode->psPhysHeapList;

	/* We need to ensure IMA heaps are destroyed before DLM so that
	 * we don't cause RA leaks by freeing the DLM first.
	 */
	while (psNode)
	{
		PHYS_HEAP *psTmp = psNode;

		psNode = psNode->psNext;

		if (psTmp->eType == PHYS_HEAP_TYPE_IMA_BAR ||
		    psTmp->eType == PHYS_HEAP_TYPE_IMA_PRIV)
		{
			PhysHeapDestroy(psTmp);
		}
	}

	/* Reset the loop */
	psNode = psDevNode->psPhysHeapList;

	while (psNode)
	{
		PHYS_HEAP *psTmp = psNode;

		psNode = psNode->psNext;

		if (psTmp->eType == PHYS_HEAP_TYPE_DLM_BAR ||
		    psTmp->eType == PHYS_HEAP_TYPE_DLM_PRIV)
		{
			PhysHeapDestroy(psTmp);
		}
	}
}

#endif

/*************************************************************************/ /*!
@Function       PhysHeapCreateDeviceHeapsFromConfigs
@Description    Create new heaps for a device from configs.
@Input          psDevNode      Pointer to device node struct
@Input          pasConfigs     Pointer to array of Heap configurations.
@Input          ui32NumConfigs Number of configurations in array.
@Return         PVRSRV_ERROR PVRSRV_OK or error code
*/ /**************************************************************************/
static PVRSRV_ERROR
PhysHeapCreateDeviceHeapsFromConfigs(PPVRSRV_DEVICE_NODE psDevNode,
                                     PHYS_HEAP_CONFIG *pasConfigs,
                                     IMG_UINT32 ui32NumConfigs)
{
	IMG_UINT32 i;
	PVRSRV_ERROR eError;

	psDevNode->psPhysHeapList = NULL;

#if defined(PVRSRV_SUPPORT_DYNAMIC_PHYSHEAPS)
	/* DLM/IMA heaps must be constructed in a specific order */
	eError = PhysHeapCreateDLMIMAHeapsFromConfig(psDevNode,
	                                             pasConfigs,
	                                             ui32NumConfigs);
	PVR_LOG_RETURN_IF_ERROR(eError, "PhysHeapCreateDLMIMAHeapsFromConfig");
#endif


	for (i = 0; i < ui32NumConfigs; i++)
	{
		/* A PhysHeapConfig can have multiple usage flags. If any flag in a
		 * heap's set points to a heap type that is handled by the PVR Layer
		 * then we assume that a single heap is shared between multiple
		 * allocators and it is safe to instantiate it here. If the heap
		 * is not marked to be initialised by the PVR Layer, leave it
		 * to the device specific handler.
		 * DLM Heaps have usage flags but don't have element in the
		 * fallback table, this check will prevent instantiate them twice.
		 */
		if (HeapCfgUsedByPVRLayer(&pasConfigs[i]))
		{
			eError = PhysHeapCreateHeapFromConfig(psDevNode, &pasConfigs[i], NULL);
			PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemCreateHeap");
		}
	}

#if defined(SUPPORT_PHYSMEM_TEST)
	/* For a temporary device node there will never be a debug dump
	 * request targeting it */
	if (psDevNode->hDebugTable != NULL)
#endif
	{
		eError = PVRSRVRegisterDeviceDbgRequestNotify(&psDevNode->hPhysHeapDbgReqNotify,
		                                              psDevNode,
		                                              PhysHeapDebugRequest,
		                                              DEBUG_REQUEST_SYS,
		                                              psDevNode);

		PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVRegisterDeviceDbgRequestNotify");
	}
	return PVRSRV_OK;
}

PVRSRV_ERROR
PhysHeapCreateHeapFromConfig(PVRSRV_DEVICE_NODE *psDevNode,
							 PHYS_HEAP_CONFIG *psConfig,
							 PHYS_HEAP **ppsPhysHeap)
{
	PVRSRV_ERROR eResult;

	if (psConfig->eType == PHYS_HEAP_TYPE_UMA
#if defined(SUPPORT_WRAP_EXTMEMOBJECT)
		|| psConfig->eType == PHYS_HEAP_TYPE_WRAP
#endif
		)
	{
		eResult = PhysmemCreateHeapOSMEM(psDevNode,
		                                 PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG,
		                                 psConfig,
		                                 ppsPhysHeap);
	}
	else if ((psConfig->eType == PHYS_HEAP_TYPE_LMA) ||
	         (psConfig->eType == PHYS_HEAP_TYPE_DMA))
	{
		PHYS_HEAP_POLICY uiHeapPolicy;

		if (psDevNode->pfnPhysHeapGetLMAPolicy != NULL)
		{
			uiHeapPolicy = psDevNode->pfnPhysHeapGetLMAPolicy(psConfig->ui32UsageFlags, psDevNode);
		}
		else
		{
			uiHeapPolicy = OSIsMapPhysNonContigSupported() ?
			               PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG :
			               PHYS_HEAP_POLICY_DEFAULT;
		}

		eResult = PhysmemCreateHeapLMA(psDevNode,
		                               uiHeapPolicy,
		                               psConfig,
		                               (psConfig->eType == PHYS_HEAP_TYPE_LMA) ?
		                                "GPU LMA (Sys)" :
		                                "GPU LMA DMA (Sys)",
		                               ppsPhysHeap);
	}
	else if ((psConfig->eType == PHYS_HEAP_TYPE_DLM_BAR)  ||
	         (psConfig->eType == PHYS_HEAP_TYPE_DLM_PRIV) ||
	         (psConfig->eType == PHYS_HEAP_TYPE_IMA_BAR)  ||
	         (psConfig->eType == PHYS_HEAP_TYPE_IMA_PRIV))
	{
		/* These heaps have already been instantiated in
		 * PhysHeapCreateDLMIMAHeapsFromConfig
		 */
		eResult = PVRSRV_OK;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "%s Invalid phys heap type: %d",
				 __func__, psConfig->eType));
		eResult = PVRSRV_ERROR_INVALID_PARAMS;
	}

	return eResult;
}

#define PVRSRV_MIN_DEFAULT_LMA_PHYS_HEAP_SIZE (0x100000ULL * 32ULL) /* 32MB */
#define PVRSRV_MIN_DLM_PHYS_HEAP_SIZE (0x100000ULL * 128ULL) /* 128MB */

static PVRSRV_ERROR PVRSRVValidatePhysHeapConfig(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	IMG_UINT32 ui32FlagsAccumulate = 0;
	IMG_UINT32 i;

	PVR_LOG_RETURN_IF_FALSE(psDevConfig->ui32PhysHeapCount > 0,
							"Device config must specify at least one phys heap config.",
							PVRSRV_ERROR_PHYSHEAP_CONFIG);

	for (i = 0; i < psDevConfig->ui32PhysHeapCount; i++)
	{
		PHYS_HEAP_CONFIG *psHeapConf = &psDevConfig->pasPhysHeaps[i];

		/* We may have multiple DLM heap definitions using the same usage flag */
		if (!(psHeapConf->eType == PHYS_HEAP_TYPE_DLM_BAR) &&
		    !(psHeapConf->eType == PHYS_HEAP_TYPE_DLM_PRIV))
		{
			PVR_LOG_RETURN_IF_FALSE_VA(psHeapConf->ui32UsageFlags != 0,
									   PVRSRV_ERROR_PHYSHEAP_CONFIG,
									   "Phys heap config %d: must specify usage flags.", i);

			PVR_LOG_RETURN_IF_FALSE_VA((ui32FlagsAccumulate & psHeapConf->ui32UsageFlags) == 0,
									PVRSRV_ERROR_PHYSHEAP_CONFIG,
									"Phys heap config %d: duplicate usage flags.", i);
		}
		else
		{
			if (psHeapConf->uConfig.sDLM.uiSize < PVRSRV_MIN_DLM_PHYS_HEAP_SIZE)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Size of DLM_%s heap is 0x%" IMG_UINT64_FMTSPECX
				         " (recommended minimum heap size is 0x%llx)",
				         __func__,
				         (psHeapConf->eType == PHYS_HEAP_TYPE_DLM_BAR) ? "BAR":"PRIV",
				         psHeapConf->uConfig.sDLM.uiSize,
				         PVRSRV_MIN_DLM_PHYS_HEAP_SIZE));
			}
		}


		ui32FlagsAccumulate |= psHeapConf->ui32UsageFlags;

		/* Output message if default heap is LMA and smaller than recommended minimum */
		if (BITMASK_ANY((1U << psDevConfig->eDefaultHeap), PHYS_HEAP_USAGE_MASK) &&
			BITMASK_ANY((1U << psDevConfig->eDefaultHeap), psHeapConf->ui32UsageFlags) &&
#if defined(__KERNEL__)
		    ((psHeapConf->eType == PHYS_HEAP_TYPE_LMA) ||
		     (psHeapConf->eType == PHYS_HEAP_TYPE_DMA)) &&
#else
		    (psHeapConf->eType == PHYS_HEAP_TYPE_LMA) &&
#endif
		    (psHeapConf->uConfig.sLMA.uiSize < PVRSRV_MIN_DEFAULT_LMA_PHYS_HEAP_SIZE))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Size of default heap is 0x%" IMG_UINT64_FMTSPECX
			         " (recommended minimum heap size is 0x%llx)",
			         __func__, psHeapConf->uConfig.sLMA.uiSize,
			         PVRSRV_MIN_DEFAULT_LMA_PHYS_HEAP_SIZE));
		}
	}

	if (psDevConfig->eDefaultHeap == PVRSRV_PHYS_HEAP_GPU_LOCAL)
	{
		PVR_LOG_RETURN_IF_FALSE(((ui32FlagsAccumulate & PHYS_HEAP_USAGE_GPU_LOCAL) != 0) ,
							"Device config must specify GPU local phys heap config.",
							PVRSRV_ERROR_PHYSHEAP_CONFIG);
	}
	else if (psDevConfig->eDefaultHeap == PVRSRV_PHYS_HEAP_CPU_LOCAL)
	{
		PVR_LOG_RETURN_IF_FALSE(((ui32FlagsAccumulate & PHYS_HEAP_USAGE_CPU_LOCAL) != 0) ,
						"Device config must specify CPU local phys heap config.",
						PVRSRV_ERROR_PHYSHEAP_CONFIG);
	}

	return PVRSRV_OK;
}

#if defined(SUPPORT_GPUVIRT_VALIDATION)
/*************************************************************************/ /*!
@Function       CreateGpuVirtValArenas
@Description    Create virtualization validation arenas
@Input          psDeviceNode The device node
@Return         PVRSRV_ERROR PVRSRV_OK on success
*/ /**************************************************************************/
static PVRSRV_ERROR CreateGpuVirtValArenas(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	/* aui64OSidMin and aui64OSidMax are what we program into HW registers.
	   The values are different from base/size of arenas. */
	IMG_UINT64 aui64OSidMin[GPUVIRT_VALIDATION_NUM_REGIONS][GPUVIRT_VALIDATION_NUM_OS];
	IMG_UINT64 aui64OSidMax[GPUVIRT_VALIDATION_NUM_REGIONS][GPUVIRT_VALIDATION_NUM_OS];
	PHYS_HEAP_CONFIG *psGPULocalHeap = PVRSRVFindPhysHeapConfig(psDeviceNode->psDevConfig, PHYS_HEAP_USAGE_GPU_LOCAL);
	IMG_DEV_PHYADDR sGPULocalCardBase = PhysHeapConfigGetCardBase(psGPULocalHeap);
	IMG_UINT64 uiGPULocalBaseSize = PhysHeapConfigGetSize(psGPULocalHeap);
	PHYS_HEAP_CONFIG *psDisplayHeap = PVRSRVFindPhysHeapConfig(psDeviceNode->psDevConfig, PHYS_HEAP_USAGE_DISPLAY);
	IMG_UINT64 uBase;
	IMG_UINT64 uSize;
	IMG_UINT64 uBaseShared;
	IMG_UINT64 uSizeShared;
	IMG_UINT64 uSizeSharedReg;
	IMG_UINT32 i;

	/* Shared region is fixed size, the remaining space is divided amongst OSes */
	uSizeShared = PVR_ALIGN(GPUVIRT_SIZEOF_SHARED, (IMG_DEVMEM_SIZE_T)OSGetPageSize());
	uSize = uiGPULocalBaseSize - uSizeShared;
	uSize /= GPUVIRT_VALIDATION_NUM_OS;
	uSize = uSize & ~((IMG_UINT64)OSGetPageSize() - 1ULL); /* Align, round down */

	uBase = sGPULocalCardBase.uiAddr;
	uBaseShared = uBase + uSize * GPUVIRT_VALIDATION_NUM_OS;
	uSizeShared = uiGPULocalBaseSize - (uBaseShared - uBase);

	PVR_LOG(("GPUVIRT_VALIDATION split GPU_LOCAL base: 0x%" IMG_UINT64_FMTSPECX ", size: 0x%" IMG_UINT64_FMTSPECX ".",
			 sGPULocalCardBase.uiAddr,
			 uiGPULocalBaseSize));

	/* If a display heap config exists, include the display heap in the non-secure regions */
	if (psDisplayHeap)
	{
		IMG_DEV_PHYADDR sGPUDisplayCardBase = PhysHeapConfigGetCardBase(psDisplayHeap);
		IMG_UINT64 uiGPUDisplaySize = PhysHeapConfigGetSize(psDisplayHeap);

		/* Only works when DISPLAY heap follows GPU_LOCAL heap. */
		PVR_LOG(("GPUVIRT_VALIDATION include DISPLAY in shared, base: 0x%" IMG_UINT64_FMTSPECX ", size: 0x%" IMG_UINT64_FMTSPECX ".",
		         sGPUDisplayCardBase.uiAddr,
		         uiGPUDisplaySize));

		uSizeSharedReg = uSizeShared + uiGPUDisplaySize;
	}
	else
	{
		uSizeSharedReg = uSizeShared;
	}

	PVR_ASSERT(uSize >= GPUVIRT_MIN_SIZE);
	PVR_ASSERT(uSizeSharedReg >= GPUVIRT_SIZEOF_SHARED);

	FOREACH_VALIDATION_OSID(i)
	{
		IMG_CHAR aszOSRAName[RA_MAX_NAME_LENGTH];

		PVR_LOG(("GPUVIRT_VALIDATION create arena OS: %d, base: 0x%" IMG_UINT64_FMTSPECX ", size: 0x%" IMG_UINT64_FMTSPECX ".", i, uBase, uSize));

		OSSNPrintf(aszOSRAName, RA_MAX_NAME_LENGTH, "GPUVIRT_OS%d", i);

		psDeviceNode->psOSidSubArena[i] = RA_Create_With_Span(aszOSRAName,
		                                                      OSGetPageShift(),
		                                                      0,
		                                                      uBase,
		                                                      uSize,
		                                                      RA_POLICY_DEFAULT);
		PVR_LOG_RETURN_IF_NOMEM(psDeviceNode->psOSidSubArena[i], "RA_Create_With_Span");

		aui64OSidMin[GPUVIRT_VAL_REGION_SECURE][i] = uBase;

		if (i == 0)
		{
			/* OSid0 has access to all regions */
			aui64OSidMax[GPUVIRT_VAL_REGION_SECURE][i] = uiGPULocalBaseSize - 1ULL;
		}
		else
		{
			aui64OSidMax[GPUVIRT_VAL_REGION_SECURE][i] = uBase + uSize - 1ULL;
		}

		/* uSizeSharedReg includes display heap */
		aui64OSidMin[GPUVIRT_VAL_REGION_SHARED][i] = uBaseShared;
		aui64OSidMax[GPUVIRT_VAL_REGION_SHARED][i] = uBaseShared + uSizeSharedReg - 1ULL;

		PVR_LOG(("GPUVIRT_VALIDATION HW reg regions %d: min[0]: 0x%" IMG_UINT64_FMTSPECX ", max[0]: 0x%" IMG_UINT64_FMTSPECX ", min[1]: 0x%" IMG_UINT64_FMTSPECX ", max[1]: 0x%" IMG_UINT64_FMTSPECX ",",
				 i,
				 aui64OSidMin[GPUVIRT_VAL_REGION_SECURE][i],
				 aui64OSidMax[GPUVIRT_VAL_REGION_SECURE][i],
				 aui64OSidMin[GPUVIRT_VAL_REGION_SHARED][i],
				 aui64OSidMax[GPUVIRT_VAL_REGION_SHARED][i]));
		uBase += uSize;
	}

	PVR_LOG(("GPUVIRT_VALIDATION create arena Shared, base: 0x%" IMG_UINT64_FMTSPECX ", size: 0x%" IMG_UINT64_FMTSPECX ".", uBaseShared, uSizeShared));

	PVR_ASSERT(uSizeShared >= GPUVIRT_SIZEOF_SHARED);

	/* uSizeShared does not include  display heap */
	psDeviceNode->psOSSharedArena = RA_Create_With_Span("GPUVIRT_SHARED",
	                                                    OSGetPageShift(),
	                                                    0,
	                                                    uBaseShared,
	                                                    uSizeShared,
	                                                    RA_POLICY_DEFAULT);
	PVR_LOG_RETURN_IF_NOMEM(psDeviceNode->psOSSharedArena, "RA_Create_With_Span");

	if (psDeviceNode->psDevConfig->pfnSysDevVirtInit != NULL)
	{
		psDeviceNode->psDevConfig->pfnSysDevVirtInit(psDeviceNode->psDevConfig->hSysData, aui64OSidMin, aui64OSidMax);
	}

	return PVRSRV_OK;
}

/*
 * Counter-part to CreateGpuVirtValArenas.
 */
static void DestroyGpuVirtValArenas(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	IMG_UINT32	uiCounter = 0;

	FOREACH_VALIDATION_OSID(uiCounter)
	{
		if (uiCounter == RGXFW_HOST_DRIVER_ID)
		{
			/*
			 * NOTE: We overload psOSidSubArena[0] into the psLocalMemArena so we must
			 * not free it here as it gets cleared later.
			 */
			continue;
		}

		if (psDeviceNode->psOSidSubArena[uiCounter] == NULL)
		{
			continue;
		}
		RA_Delete(psDeviceNode->psOSidSubArena[uiCounter]);
	}

	if (psDeviceNode->psOSSharedArena != NULL)
	{
		RA_Delete(psDeviceNode->psOSSharedArena);
	}
}
#endif

/*************************************************************************/ /*!
@Function       PhysHeapMMUPxSetup
@Description    Setup MMU Px allocation function pointers.
@Input          psDeviceNode Pointer to device node struct
@Return         PVRSRV_ERROR PVRSRV_OK on success.
*/ /**************************************************************************/
static PVRSRV_ERROR PhysHeapMMUPxSetup(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	PHYS_HEAP_TYPE eHeapType;
	PVRSRV_ERROR eError;

	eError = PhysHeapAcquireByID(psDeviceNode->psDevConfig->eDefaultHeap,
	                                      psDeviceNode, &psDeviceNode->psMMUPhysHeap);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapAcquireByID", ErrorDeinit);

	eHeapType = PhysHeapGetType(psDeviceNode->psMMUPhysHeap);

	if (eHeapType == PHYS_HEAP_TYPE_UMA)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: GPU physical heap uses OS System memory (UMA)", __func__));

#if defined(SUPPORT_GPUVIRT_VALIDATION)
		PVR_DPF((PVR_DBG_ERROR, "%s: Virtualisation Validation builds are currently only"
								 " supported on systems with local memory (LMA).", __func__));
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
		goto ErrorDeinit;
#endif
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: GPU physical heap uses local memory managed by the driver (LMA)", __func__));

#if defined(SUPPORT_GPUVIRT_VALIDATION)
		eError = CreateGpuVirtValArenas(psDeviceNode);
		PVR_LOG_GOTO_IF_ERROR(eError, "CreateGpuVirtValArenas", ErrorDeinit);
#endif
	}

	return PVRSRV_OK;
ErrorDeinit:
	return eError;
}

/*************************************************************************/ /*!
@Function       PhysHeapMMUPxDeInit
@Description    Deinit after PhysHeapMMUPxSetup.
@Input          psDeviceNode Pointer to device node struct
*/ /**************************************************************************/
static void PhysHeapMMUPxDeInit(PPVRSRV_DEVICE_NODE psDeviceNode)
{
#if defined(SUPPORT_GPUVIRT_VALIDATION)
	/* Remove local LMA subarenas */
	DestroyGpuVirtValArenas(psDeviceNode);
#endif	/* defined(SUPPORT_GPUVIRT_VALIDATION) */

	if (psDeviceNode->psMMUPhysHeap != NULL)
	{
		PhysHeapRelease(psDeviceNode->psMMUPhysHeap);
		psDeviceNode->psMMUPhysHeap = NULL;
	}
}

PVRSRV_ERROR PhysHeapInitDeviceHeaps(PVRSRV_DEVICE_NODE *psDeviceNode, PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_ERROR eError;
	PVRSRV_PHYS_HEAP ePhysHeap;

	eError = OSLockCreate(&psDeviceNode->hPhysHeapLock);
	PVR_LOG_RETURN_IF_ERROR(eError, "OSLockCreate");

	eError = PVRSRVValidatePhysHeapConfig(psDevConfig);
	PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVValidatePhysHeapConfig");

	eError = PhysHeapCreateDeviceHeapsFromConfigs(psDeviceNode,
	                                              psDevConfig->pasPhysHeaps,
	                                              psDevConfig->ui32PhysHeapCount);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapCreateDeviceHeapsFromConfigs", ErrorDeinit);

	/* Must loop from the 2nd heap to the last */
	PVR_ASSERT(PVRSRV_PHYS_HEAP_DEFAULT == 0);
	for (ePhysHeap = (PVRSRV_PHYS_HEAP)(PVRSRV_PHYS_HEAP_DEFAULT+1); ePhysHeap < PVRSRV_PHYS_HEAP_LAST; ePhysHeap++)
	{
		if (PhysHeapAcquiredByPVRLayer(ePhysHeap))
		{
			eError = PhysHeapAcquireByID(ePhysHeap, psDeviceNode, &psDeviceNode->apsPhysHeap[ePhysHeap]);
			PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapAcquireByID", ErrorDeinit);
		}
	}

	if (PhysHeapValidateDefaultHeapExists(psDeviceNode))
	{
		PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapValidateDefaultHeapExists", ErrorDeinit);
	}

	eError = PhysHeapMMUPxSetup(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapMMUPxSetup", ErrorDeinit);

	return PVRSRV_OK;

ErrorDeinit:
	PVR_ASSERT(IMG_FALSE);
	PhysHeapDeInitDeviceHeaps(psDeviceNode);

	return eError;
}

void PhysHeapDeInitDeviceHeaps(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_PHYS_HEAP ePhysHeapIdx;
	IMG_UINT32 i;

	PhysHeapMMUPxDeInit(psDeviceNode);

	/* Release heaps */
	for (ePhysHeapIdx = PVRSRV_PHYS_HEAP_DEFAULT;
		 ePhysHeapIdx < ARRAY_SIZE(psDeviceNode->apsPhysHeap);
		 ePhysHeapIdx++)
	{
		if (psDeviceNode->apsPhysHeap[ePhysHeapIdx])
		{
			PhysHeapRelease(psDeviceNode->apsPhysHeap[ePhysHeapIdx]);
		}
	}

	FOREACH_SUPPORTED_DRIVER(i)
	{
		if (psDeviceNode->apsFWPremapPhysHeap[i])
		{
			PhysHeapDestroy(psDeviceNode->apsFWPremapPhysHeap[i]);
			psDeviceNode->apsFWPremapPhysHeap[i] = NULL;
		}
	}

	PhysHeapDestroyDeviceHeaps(psDeviceNode);

	OSLockDestroy(psDeviceNode->hPhysHeapLock);
}

PVRSRV_ERROR PhysHeapCreate(PPVRSRV_DEVICE_NODE psDevNode,
							  PHYS_HEAP_CONFIG *psConfig,
							  PHYS_HEAP_POLICY uiPolicy,
							  PHEAP_IMPL_DATA pvImplData,
							  PHEAP_IMPL_FUNCS *psImplFuncs,
							  PHYS_HEAP **ppsPhysHeap)
{
	PHYS_HEAP *psNew;

	PVR_DPF_ENTERED;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevNode != NULL, "psDevNode");

	if (psConfig->eType == PHYS_HEAP_TYPE_UNKNOWN)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVR_LOG_RETURN_IF_INVALID_PARAM(psImplFuncs != NULL, "psImplFuncs");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psImplFuncs->pfnCreatePMR != NULL ||
	                                psImplFuncs->pfnCreatePMB,
	                                "psImplFuncs->pfnCreatePMR || psImplFuncs->pfnCreatePMB");

	psNew = OSAllocMem(sizeof(PHYS_HEAP));
	PVR_RETURN_IF_NOMEM(psNew);

	switch (psConfig->eType)
	{
	case PHYS_HEAP_TYPE_LMA:
		psNew->psMemFuncs = psConfig->uConfig.sLMA.psMemFuncs;
		psNew->hPrivData = psConfig->uConfig.sLMA.hPrivData;
		psNew->pszPDumpMemspaceName = psConfig->uConfig.sLMA.pszPDumpMemspaceName;
		OSStringLCopy(psNew->aszName,
					  (psConfig->uConfig.sLMA.pszHeapName) ? psConfig->uConfig.sLMA.pszHeapName :
					                                         "Unknown PhysHeap",
					  PHYS_HEAP_NAME_SIZE);
		break;
	case PHYS_HEAP_TYPE_IMA_BAR:
	case PHYS_HEAP_TYPE_IMA_PRIV:
		psNew->psMemFuncs = psConfig->uConfig.sIMA.psMemFuncs;
		psNew->hPrivData = psConfig->uConfig.sIMA.hPrivData;
		psNew->pszPDumpMemspaceName = psConfig->uConfig.sIMA.pszPDumpMemspaceName;
		OSStringLCopy(psNew->aszName,
					  (psConfig->uConfig.sIMA.pszHeapName) ? psConfig->uConfig.sIMA.pszHeapName :
					                                         "Unknown PhysHeap",
					  PHYS_HEAP_NAME_SIZE);
		break;
	case PHYS_HEAP_TYPE_DMA:
		psNew->psMemFuncs = psConfig->uConfig.sDMA.psMemFuncs;
		psNew->hPrivData = psConfig->uConfig.sDMA.hPrivData;
		psNew->pszPDumpMemspaceName = psConfig->uConfig.sDMA.pszPDumpMemspaceName;
		OSStringLCopy(psNew->aszName,
					  (psConfig->uConfig.sDMA.pszHeapName) ? psConfig->uConfig.sDMA.pszHeapName :
					                                         "Unknown PhysHeap",
					  PHYS_HEAP_NAME_SIZE);
		break;
	case PHYS_HEAP_TYPE_UMA:
		psNew->psMemFuncs = psConfig->uConfig.sUMA.psMemFuncs;
		psNew->hPrivData = psConfig->uConfig.sUMA.hPrivData;
		psNew->pszPDumpMemspaceName = psConfig->uConfig.sUMA.pszPDumpMemspaceName;
		OSStringLCopy(psNew->aszName,
					  (psConfig->uConfig.sUMA.pszHeapName) ? psConfig->uConfig.sUMA.pszHeapName :
					                                         "Unknown PhysHeap",
					  PHYS_HEAP_NAME_SIZE);
		break;
	case PHYS_HEAP_TYPE_DLM_BAR:
	case PHYS_HEAP_TYPE_DLM_PRIV:
		psNew->psMemFuncs = psConfig->uConfig.sDLM.psMemFuncs;
		psNew->hPrivData = psConfig->uConfig.sDLM.hPrivData;
		psNew->pszPDumpMemspaceName = "None";
		OSStringLCopy(psNew->aszName,
					  (psConfig->uConfig.sDLM.pszHeapName) ? psConfig->uConfig.sDLM.pszHeapName :
					                                         "Unknown PhysHeap",
					  PHYS_HEAP_NAME_SIZE);
		break;
	default:
		PVR_LOG_ERROR(PVRSRV_ERROR_NOT_IMPLEMENTED, "psConfig->eType not implemented");
	}

	psNew->eType = psConfig->eType;
	psNew->ui32UsageFlags = psConfig->ui32UsageFlags;
	psNew->uiPolicy = uiPolicy;
	psNew->ui32RefCount = 0;
	psNew->psDevNode = psDevNode;
#if !defined(PVRSRV_PHYSHEAP_DISABLE_OOM_DEMOTION)
	OSAtomicWrite(&psNew->sOOMDetected, IMG_FALSE);
#endif
	psNew->pvImplData = pvImplData;
	psNew->psImplFuncs = psImplFuncs;

#if defined(PVRSRV_SUPPORT_IPA_FEATURE)
	{
		IMG_UINT8  ui8Val;

		/* Ensure we do not cause an address fault by accessing beyond
		 * the end of the psConfig->sIPAConfig structure.
		 */
		ui8Val = psConfig->sIPAConfig.ui8IPAPolicyDefault;
		psNew->ui32IPAPolicyValue = (IMG_UINT32)ui8Val;

		ui8Val = psConfig->sIPAConfig.ui8IPAPolicyMask;
		psNew->ui32IPAClearMask = (IMG_UINT32)ui8Val;

		ui8Val = psConfig->sIPAConfig.ui8IPAPolicyShift;
		psNew->ui32IPAShift = (IMG_UINT32)ui8Val;
		PVR_LOG_VA(PVR_DBG_MESSAGE, "%s: Physheap <%p> ['%s'] Config @ <%p> IPA = [0x%x, 0x%x, 0x%x]",
		           __func__, psNew, psNew->aszName,
		           psConfig, psNew->ui32IPAPolicyValue,
		           psNew->ui32IPAClearMask, psNew->ui32IPAShift);
	}
#endif

	if (ppsPhysHeap != NULL)
	{
		*ppsPhysHeap = psNew;
	}

	psNew->psNext = psDevNode->psPhysHeapList;
	psDevNode->psPhysHeapList = psNew;

	PVR_DPF_RETURN_RC1(PVRSRV_OK, psNew);
}

void PhysHeapDestroyDeviceHeaps(PPVRSRV_DEVICE_NODE psDevNode)
{
	PHYS_HEAP *psNode;

	if (psDevNode->hPhysHeapDbgReqNotify)
	{
		PVRSRVUnregisterDeviceDbgRequestNotify(psDevNode->hPhysHeapDbgReqNotify);
	}

#if defined(PVRSRV_SUPPORT_DYNAMIC_PHYSHEAPS)
	/* DLM/IMA heaps must be destroyed in a specific order */
	PhysHeapDestroyDLMIMAHeaps(psDevNode);
#endif

	psNode = psDevNode->psPhysHeapList;

	while (psNode)
	{
		PHYS_HEAP *psTmp = psNode;

		psNode = psNode->psNext;
		PhysHeapDestroy(psTmp);
	}
}

void PhysHeapDestroy(PHYS_HEAP *psPhysHeap)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;
	PPVRSRV_DEVICE_NODE psDevNode = psPhysHeap->psDevNode;

	PVR_DPF_ENTERED1(psPhysHeap);

#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
	if (PVRSRVGetPVRSRVData()->eServicesState == PVRSRV_SERVICES_STATE_OK)
#endif
	{
		PVR_ASSERT(psPhysHeap->ui32RefCount == 0);
	}

	if (psDevNode->psPhysHeapList == psPhysHeap)
	{
		psDevNode->psPhysHeapList = psPhysHeap->psNext;
	}
	else
	{
		PHYS_HEAP *psTmp = psDevNode->psPhysHeapList;

		while (psTmp->psNext != psPhysHeap)
		{
			psTmp = psTmp->psNext;
		}
		psTmp->psNext = psPhysHeap->psNext;
	}

	if (psImplFuncs->pfnDestroyData != NULL)
	{
		psImplFuncs->pfnDestroyData(psPhysHeap->pvImplData);
	}

	OSFreeMem(psPhysHeap);

	PVR_DPF_RETURN;
}

static void _PhysHeapCountUserModeHeaps(PPVRSRV_DEVICE_NODE psDevNode,
										PHYS_HEAP_USAGE_FLAGS ui32UsageFlags)
{
	PVRSRV_PHYS_HEAP eHeap;

	for (eHeap = PVRSRV_PHYS_HEAP_DEFAULT;
		 eHeap <= PVRSRV_PHYS_HEAP_LAST;
		 eHeap++)
	{
		if (BIT_ISSET(ui32UsageFlags, eHeap) &&
			PhysHeapUserModeAlloc(eHeap))
		{
			psDevNode->ui32UserAllocHeapCount++;
			break;
		}
	}
}

PVRSRV_ERROR PhysHeapAcquire(PHYS_HEAP *psPhysHeap)
{
	PVR_LOG_RETURN_IF_INVALID_PARAM(psPhysHeap != NULL, "psPhysHeap");

	psPhysHeap->ui32RefCount++;

	/* When acquiring a heap for the 1st time, perform a check and
	 * calculate the total number of user accessible physical heaps */
	if (psPhysHeap->ui32RefCount == 1)
	{
		_PhysHeapCountUserModeHeaps(psPhysHeap->psDevNode,
									psPhysHeap->ui32UsageFlags);
	}

	return PVRSRV_OK;
}

static PHYS_HEAP * _PhysHeapFindHeapOrFallback(PVRSRV_PHYS_HEAP ePhysHeap,
								   PPVRSRV_DEVICE_NODE psDevNode)
{
	PHYS_HEAP *psPhysHeapNode = psDevNode->psPhysHeapList;
	PVRSRV_PHYS_HEAP eFallback;

	/* Swap the default heap alias for the system's real default PhysHeap */
	if (ePhysHeap == PVRSRV_PHYS_HEAP_DEFAULT)
	{
		ePhysHeap = psDevNode->psDevConfig->eDefaultHeap;
	}

	/* Check cache of PhysHeaps to see if it has been resolved before */
	if (psDevNode->apsPhysHeap[ePhysHeap] != NULL)
	{
		return psDevNode->apsPhysHeap[ePhysHeap];
	}

	/* Cache not ready, carry out search with fallback */
	while (psPhysHeapNode)
	{
		if (BIT_ISSET(psPhysHeapNode->ui32UsageFlags, ePhysHeap))
		{
			return psPhysHeapNode;
		}

		psPhysHeapNode = psPhysHeapNode->psNext;
	}

	/* Find fallback PhysHeap */
	eFallback = gasHeapProperties[ePhysHeap].eFallbackHeap;
	if (ePhysHeap == eFallback)
	{
		return NULL;
	}
	else
	{
		return _PhysHeapFindHeapOrFallback(eFallback, psDevNode);
	}
}

/*
 * Acquire heap, no fallback, no recursion: single loop acquisition
 */
static PHYS_HEAP* _PhysHeapFindRealHeapNoFallback(PVRSRV_PHYS_HEAP ePhysHeap,
                                          PPVRSRV_DEVICE_NODE psDevNode)
{
	PHYS_HEAP *psPhysHeapNode = psDevNode->psPhysHeapList;

	/* Swap the default heap alias for the system's real default PhysHeap */
	if (ePhysHeap == PVRSRV_PHYS_HEAP_DEFAULT)
	{
		ePhysHeap = psDevNode->psDevConfig->eDefaultHeap;
	}

	/* Check cache of PhysHeaps to see if it has been resolved before.
	 * The physheap must initialised to access its cache. */
	if (psDevNode->apsPhysHeap[ePhysHeap] != NULL &&
	    BIT_ISSET(psDevNode->apsPhysHeap[ePhysHeap]->ui32UsageFlags, ePhysHeap))
	{
		return psDevNode->apsPhysHeap[ePhysHeap];
	}

	/* Cache not ready, carry out search for real PhysHeap, no fallback */
	while (psPhysHeapNode)
	{
		if (BIT_ISSET(psPhysHeapNode->ui32UsageFlags, ePhysHeap))
		{
			return psPhysHeapNode;
		}

		psPhysHeapNode = psPhysHeapNode->psNext;
	}
	return NULL;
}

PVRSRV_ERROR PhysHeapAcquireByID(PVRSRV_PHYS_HEAP eDevPhysHeap,
								 PPVRSRV_DEVICE_NODE psDevNode,
								 PHYS_HEAP **ppsPhysHeap)
{
	PHYS_HEAP *psPhysHeap;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_LOG_RETURN_IF_INVALID_PARAM(eDevPhysHeap < PVRSRV_PHYS_HEAP_LAST, "eDevPhysHeap");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevNode != NULL, "psDevNode");

	PVR_DPF_ENTERED1(ui32Flags);

	OSLockAcquire(psDevNode->hPhysHeapLock);

	psPhysHeap = _PhysHeapFindHeapOrFallback(eDevPhysHeap, psDevNode);

	if (psPhysHeap != NULL)
	{
		psPhysHeap->ui32RefCount++;
		PHYSHEAP_REFCOUNT_PRINT("%s: Heap %p, refcount = %d",
								__func__, psPhysHeap, psPhysHeap->ui32RefCount);

		/* When acquiring a heap for the 1st time, perform a check and
		 * calculate the total number of user accessible physical heaps */
		if (psPhysHeap->ui32RefCount == 1)
		{
			_PhysHeapCountUserModeHeaps(psDevNode, BIT(eDevPhysHeap));
		}
	}
	else
	{
		eError = PVRSRV_ERROR_PHYSHEAP_ID_INVALID;
	}

	OSLockRelease(psDevNode->hPhysHeapLock);

	*ppsPhysHeap = psPhysHeap;
	PVR_DPF_RETURN_RC1(eError, *ppsPhysHeap);
}

void PhysHeapRelease(PHYS_HEAP *psPhysHeap)
{
	PVR_DPF_ENTERED1(psPhysHeap);

	OSLockAcquire(psPhysHeap->psDevNode->hPhysHeapLock);
	psPhysHeap->ui32RefCount--;
	PHYSHEAP_REFCOUNT_PRINT("%s: Heap %p, refcount = %d",
							__func__, psPhysHeap, psPhysHeap->ui32RefCount);
	OSLockRelease(psPhysHeap->psDevNode->hPhysHeapLock);

	PVR_DPF_RETURN;
}

PHEAP_IMPL_DATA PhysHeapGetImplData(PHYS_HEAP *psPhysHeap)
{
	return psPhysHeap->pvImplData;
}

PHYS_HEAP_TYPE PhysHeapGetType(PHYS_HEAP *psPhysHeap)
{
	PVR_ASSERT(psPhysHeap->eType != PHYS_HEAP_TYPE_UNKNOWN);
	return psPhysHeap->eType;
}

PHYS_HEAP_POLICY PhysHeapGetPolicy(PHYS_HEAP *psPhysHeap)
{
	return psPhysHeap->uiPolicy;
}

PHYS_HEAP_USAGE_FLAGS PhysHeapGetFlags(PHYS_HEAP *psPhysHeap)
{
	return psPhysHeap->ui32UsageFlags;
}

const IMG_CHAR *PhysHeapName(PHYS_HEAP *psPhysHeap)
{
	return psPhysHeap->aszName;
}

IMG_BOOL PhysHeapValidateDefaultHeapExists(PPVRSRV_DEVICE_NODE psDevNode)
{
	PVRSRV_PHYS_HEAP eDefaultHeap = psDevNode->psDevConfig->eDefaultHeap;

	return ((psDevNode->apsPhysHeap[PVRSRV_PHYS_HEAP_DEFAULT] != NULL) &&
			((psDevNode->apsPhysHeap[PVRSRV_PHYS_HEAP_DEFAULT] ==
			  psDevNode->apsPhysHeap[eDefaultHeap])));
}

#if defined(PVRSRV_SUPPORT_IPA_FEATURE)
IMG_UINT32 PhysHeapGetIPAValue(PHYS_HEAP *psPhysHeap)
{
	return psPhysHeap->ui32IPAPolicyValue;
}

IMG_UINT32 PhysHeapGetIPAMask(PHYS_HEAP *psPhysHeap)
{
	return psPhysHeap->ui32IPAClearMask;
}

IMG_UINT32 PhysHeapGetIPAShift(PHYS_HEAP *psPhysHeap)
{
	return psPhysHeap->ui32IPAShift;
}
#endif

/*
 * This function will set the psDevPAddr to whatever the system layer
 * has set it for the referenced region.
 * It will not fail if the psDevPAddr is invalid.
 */
PVRSRV_ERROR PhysHeapGetDevPAddr(PHYS_HEAP *psPhysHeap,
									   IMG_DEV_PHYADDR *psDevPAddr)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;
	PVRSRV_ERROR eResult = PVRSRV_ERROR_NOT_IMPLEMENTED;

	if (psImplFuncs->pfnGetDevPAddr != NULL)
	{
		eResult = psImplFuncs->pfnGetDevPAddr(psPhysHeap->pvImplData,
											  psDevPAddr);
	}

	return eResult;
}

/*
 * This function will set the psCpuPAddr to whatever the system layer
 * has set it for the referenced region.
 * It will not fail if the psCpuPAddr is invalid.
 */
PVRSRV_ERROR PhysHeapGetCpuPAddr(PHYS_HEAP *psPhysHeap,
								IMG_CPU_PHYADDR *psCpuPAddr)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;
	PVRSRV_ERROR eResult = PVRSRV_ERROR_NOT_IMPLEMENTED;

	if (psImplFuncs->pfnGetCPUPAddr != NULL)
	{
		eResult = psImplFuncs->pfnGetCPUPAddr(psPhysHeap->pvImplData,
											  psCpuPAddr);
	}

	return eResult;
}

PVRSRV_ERROR PhysHeapGetSize(PHYS_HEAP *psPhysHeap,
								   IMG_UINT64 *puiSize)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;
	PVRSRV_ERROR eResult = PVRSRV_ERROR_NOT_IMPLEMENTED;

	if (psImplFuncs->pfnGetSize != NULL)
	{
		eResult = psImplFuncs->pfnGetSize(psPhysHeap->pvImplData,
										  puiSize);
	}

	return eResult;
}

/* Used to add the total and free sizes of an IMA heap and the backing
 * DLM heap
 */
static void PhysHeapIMAGetMemInfo(PHYS_HEAP *psPhysHeap,
                                  IMG_UINT64 *puiTotalSize,
                                  IMG_UINT64 *puiFreeSize)
{
	PHYS_HEAP *psDLMHeap;
	IMG_UINT64 ui64TotalSize;
	IMG_UINT64 ui64FreeSize;
	IMG_UINT64 ui64DLMTotalSize;
	IMG_UINT64 ui64DLMFreeSize;

	PVR_LOG_RETURN_VOID_IF_FALSE(
		psPhysHeap->eType != PHYS_HEAP_TYPE_IMA_BAR ||
		psPhysHeap->eType != PHYS_HEAP_TYPE_IMA_PRIV,
		"Physheap type not IMA");

	/* Obtain the DLM heap chained to this IMA heap */
	PVR_ASSERT(psPhysHeap->psImplFuncs->pfnGetHeapDLMBacking);
	psPhysHeap->psImplFuncs->pfnGetHeapDLMBacking(psPhysHeap->pvImplData,
	                                              &psDLMHeap);
	PVR_LOG_RETURN_VOID_IF_FALSE(psDLMHeap != NULL, "pfnGetHeapDLMBacking");

	/* Obtain the memstats for the current IMA heap */
	PVR_ASSERT(psPhysHeap->psImplFuncs->pfnGetFactoryMemStats);
	psPhysHeap->psImplFuncs->pfnGetFactoryMemStats(psPhysHeap->pvImplData,
	                                               &ui64TotalSize,
	                                               &ui64FreeSize);

	/* Obtain the memstats for the chained DLM heap */
	PVR_ASSERT(psDLMHeap->psImplFuncs->pfnGetFactoryMemStats);
	psDLMHeap->psImplFuncs->pfnGetFactoryMemStats(psDLMHeap->pvImplData,
	                                              &ui64DLMTotalSize,
	                                              &ui64DLMFreeSize);

	/* Total size is just the DLM heap size as the IMA heap imports a
	 * subset of that total size.
	 * Free size is a combination of the two because DLM heap may
	 * have spare PMBs to provide and the IMA heap may have space
	 * in an imported PMB.
	 */
	*puiTotalSize = ui64DLMTotalSize;
	*puiFreeSize = ui64FreeSize + ui64DLMFreeSize;
}

PVRSRV_ERROR
PhysHeapGetMemInfo(PVRSRV_DEVICE_NODE *psDevNode,
                   IMG_UINT32 ui32PhysHeapCount,
                   PVRSRV_PHYS_HEAP *paePhysHeapID,
                   PHYS_HEAP_MEM_STATS_PTR paPhysHeapMemStats)
{
	IMG_UINT32 i = 0;
	PHYS_HEAP *psPhysHeap;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevNode != NULL, "psDevNode invalid");
	PVR_LOG_RETURN_IF_INVALID_PARAM(ui32PhysHeapCount <= MAX_USER_MODE_ALLOC_PHYS_HEAPS, "ui32PhysHeapCount invalid");
	PVR_LOG_RETURN_IF_INVALID_PARAM(paePhysHeapID != NULL, "paePhysHeapID invalid");
	PVR_LOG_RETURN_IF_INVALID_PARAM(paPhysHeapMemStats != NULL, "paPhysHeapMemStats invalid");

	for (i = 0; i < ui32PhysHeapCount; i++)
	{
		if (paePhysHeapID[i] >= PVRSRV_PHYS_HEAP_LAST)
		{
			return PVRSRV_ERROR_PHYSHEAP_ID_INVALID;
		}

		psPhysHeap = _PhysHeapFindRealHeapNoFallback(paePhysHeapID[i], psDevNode);

		paPhysHeapMemStats[i].ui32PhysHeapFlags = 0;

		if (psPhysHeap && PhysHeapUserModeAlloc(paePhysHeapID[i])
				&& psPhysHeap->psImplFuncs->pfnGetFactoryMemStats)
		{
			if ((psPhysHeap->eType == PHYS_HEAP_TYPE_IMA_BAR) ||
				(psPhysHeap->eType == PHYS_HEAP_TYPE_IMA_PRIV))
			{
				PhysHeapIMAGetMemInfo(psPhysHeap,
				                      &paPhysHeapMemStats[i].ui64TotalSize,
				                      &paPhysHeapMemStats[i].ui64FreeSize);
			}
			else
			{
				psPhysHeap->psImplFuncs->pfnGetFactoryMemStats(psPhysHeap->pvImplData,
				                                               &paPhysHeapMemStats[i].ui64TotalSize,
				                                               &paPhysHeapMemStats[i].ui64FreeSize);
			}

			if (paePhysHeapID[i] == psDevNode->psDevConfig->eDefaultHeap)
			{
				paPhysHeapMemStats[i].ui32PhysHeapFlags |= PVRSRV_PHYS_HEAP_FLAGS_IS_DEFAULT;
			}

			paPhysHeapMemStats[i].ePhysHeapType = psPhysHeap->eType;
		}
		else
		{
			paPhysHeapMemStats[i].ui64TotalSize = 0;
			paPhysHeapMemStats[i].ui64FreeSize = 0;
			paPhysHeapMemStats[i].ePhysHeapType = PHYS_HEAP_TYPE_UNKNOWN;
		}
	}

	return PVRSRV_OK;
}

void PhysHeapCpuPAddrToDevPAddr(PHYS_HEAP *psPhysHeap,
								IMG_UINT32 ui32NumOfAddr,
								IMG_DEV_PHYADDR *psDevPAddr,
								IMG_CPU_PHYADDR *psCpuPAddr)
{
	psPhysHeap->psMemFuncs->pfnCpuPAddrToDevPAddr(psPhysHeap->hPrivData,
												 ui32NumOfAddr,
												 psDevPAddr,
												 psCpuPAddr);
}

void PhysHeapDevPAddrToCpuPAddr(PHYS_HEAP *psPhysHeap,
								IMG_UINT32 ui32NumOfAddr,
								IMG_CPU_PHYADDR *psCpuPAddr,
								IMG_DEV_PHYADDR *psDevPAddr)
{
	psPhysHeap->psMemFuncs->pfnDevPAddrToCpuPAddr(psPhysHeap->hPrivData,
												 ui32NumOfAddr,
												 psCpuPAddr,
												 psDevPAddr);
}

IMG_CHAR *PhysHeapPDumpMemspaceName(PHYS_HEAP *psPhysHeap)
{
	return psPhysHeap->pszPDumpMemspaceName;
}

#if !defined(PVRSRV_PHYSHEAP_DISABLE_OOM_DEMOTION)
static inline void _LogOOMDetection(IMG_BOOL isOOMDetected, PHYS_HEAP *psPhysHeap, PVRSRV_MEMALLOCFLAGS_T uiFlags)
{
	IMG_BOOL bExistingVal = OSAtomicExchange(&psPhysHeap->sOOMDetected, isOOMDetected);
	PVRSRV_PHYS_HEAP ePhysIdx = PVRSRV_GET_PHYS_HEAP_HINT(uiFlags);

	if (bExistingVal != isOOMDetected)
	{
		PVR_LOG(("Device: %d Physheap: %s OOM: %s",
				(psPhysHeap->psDevNode->sDevId.ui32InternalID),
				g_asPhysHeapUsageFlagStrings[ePhysIdx-1].pszLabel,
				(isOOMDetected) ? "Detected" : "Resolved"));
	}
}
#endif

PVRSRV_ERROR PhysHeapCreatePMR(PHYS_HEAP *psPhysHeap,
							   struct _CONNECTION_DATA_ *psConnection,
							   IMG_DEVMEM_SIZE_T uiSize,
							   IMG_UINT32 ui32NumPhysChunks,
							   IMG_UINT32 ui32NumVirtChunks,
							   IMG_UINT32 *pui32MappingTable,
							   IMG_UINT32 uiLog2PageSize,
							   PVRSRV_MEMALLOCFLAGS_T uiFlags,
							   const IMG_CHAR *pszAnnotation,
							   IMG_PID uiPid,
							   PMR **ppsPMRPtr,
							   IMG_UINT32 ui32PDumpFlags,
							   PVRSRV_MEMALLOCFLAGS_T *puiOutFlags)
{
	PVRSRV_ERROR eError;
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;
#if !defined(PVRSRV_PHYSHEAP_DISABLE_OOM_DEMOTION)
	IMG_UINT64 uiFreeBytes;
	PVRSRV_PHYS_HEAP eDemotionPhysIdx;
	PVRSRV_MEMALLOCFLAGS_T uiDemotionFlags = uiFlags;
	PVRSRV_PHYS_HEAP ePhysIdx = PVRSRV_GET_PHYS_HEAP_HINT(uiFlags);
	PHYS_HEAP *psDemotionHeap = NULL;
#endif
	eError = psImplFuncs->pfnCreatePMR(psPhysHeap,
									 psConnection,
									 uiSize,
									 ui32NumPhysChunks,
									 ui32NumVirtChunks,
									 pui32MappingTable,
									 uiLog2PageSize,
									 uiFlags,
									 pszAnnotation,
									 uiPid,
									 ppsPMRPtr,
									 ui32PDumpFlags);

#if !defined(PVRSRV_PHYSHEAP_DISABLE_OOM_DEMOTION)
	/* Check for OOM error, return if otherwise */
	_LogOOMDetection(((IsOOMError(eError)) ? IMG_TRUE : IMG_FALSE), psPhysHeap, uiFlags);
	if (eError == PVRSRV_OK)
	{
		if (puiOutFlags)
		{
			*puiOutFlags = uiFlags;
		}
		return eError;
	}
	PVR_LOG_RETURN_IF_FALSE((IsOOMError(eError)), "Failed to allocate PMR", eError);

	/* Skip logic and return if mandate flag is set */
	if (PVRSRV_CHECK_MANDATED_PHYSHEAP(uiFlags))
	{
		return eError;
	}

	/* Demotion only occurs on CPU_LOCAL,GPU_LOCAL,GPU_PRIVATE */
	if (ePhysIdx > PVRSRV_PHYS_HEAP_GPU_PRIVATE)
	{
		return eError;
	}

	eError = PVRSRV_ERROR_OUT_OF_MEMORY;
	for (eDemotionPhysIdx = (PVRSRV_PHYS_HEAP)(ePhysIdx-1); eDemotionPhysIdx != PVRSRV_PHYS_HEAP_DEFAULT; eDemotionPhysIdx--)
	{
		PVRSRV_CHANGE_PHYS_HEAP_HINT(eDemotionPhysIdx, uiDemotionFlags);
		PVR_LOG_IF_FALSE_VA(PVR_DBG_MESSAGE, (ePhysIdx-eDemotionPhysIdx < 2), "Demoted from %s to CPU_LOCAL. "
						"Expect Performance to be affected!", g_asPhysHeapUsageFlagStrings[ePhysIdx-1].pszLabel);
		psDemotionHeap = _PhysHeapFindRealHeapNoFallback(eDemotionPhysIdx, psPhysHeap->psDevNode);

		/* Either no alternative available, or allocation already failed on selected heap */
		if (psDemotionHeap == NULL || psPhysHeap == psDemotionHeap)
		{
			continue;
		}

		if (PhysHeapFreeMemCheck(psDemotionHeap, uiSize, &uiFreeBytes) != PVRSRV_OK)
		{
			_LogOOMDetection(IMG_TRUE, psDemotionHeap, uiDemotionFlags);
			continue;
		}

		psImplFuncs = psDemotionHeap->psImplFuncs;
		eError = psImplFuncs->pfnCreatePMR(psDemotionHeap,
						 psConnection,
						 uiSize,
						 ui32NumPhysChunks,
						 ui32NumVirtChunks,
						 pui32MappingTable,
						 uiLog2PageSize,
						 uiDemotionFlags,
						 pszAnnotation,
						 uiPid,
						 ppsPMRPtr,
						 ui32PDumpFlags);
		_LogOOMDetection(((IsOOMError(eError)) ? IMG_TRUE : IMG_FALSE), psDemotionHeap, uiDemotionFlags);

		if (eError == PVRSRV_OK)
		{
			if (puiOutFlags)
			{
				*puiOutFlags = uiDemotionFlags;
			}
			break;
		}
	}
	if (eError == PVRSRV_OK)
	{
		/* Success demotion worked error Ok - emit warning. */
		PVR_LOG_VA(PVR_DBG_WARNING, "PhysHeap(%s) failed to allocate PMR. Demoted to %s" ,
						g_asPhysHeapUsageFlagStrings[ePhysIdx-1].pszLabel,
						g_asPhysHeapUsageFlagStrings[eDemotionPhysIdx-1].pszLabel);
	}
	else
	{
		/* Unable to create PMR (Heap not found or CreatePMR failed) - emit Error */
		PVR_LOG_VA(PVR_DBG_ERROR, "Error raised %s : Unable to %s." ,
		                          PVRSRVGETERRORSTRING(eError),
		                          (psDemotionHeap == NULL) ?  "find heaps for demotion" :
		                                                      "allocate PMR via Demotion heap");
#if defined(SUPPORT_PMR_DEFERRED_FREE)
		{
			PPVRSRV_DEVICE_NODE psDevNode = PhysHeapDeviceNode(psPhysHeap);
			OSLockAcquire(psDevNode->hPMRZombieListLock);
			PVR_LOG_VA(PVR_DBG_ERROR, "PMR Zombie Count: %u, PMR Zombie Count In Cleanup: %u",
			                          psDevNode->uiPMRZombieCount,
			                          psDevNode->uiPMRZombieCountInCleanup);
			OSLockRelease(psDevNode->hPMRZombieListLock);
		}
#endif
	}
#endif
	return eError;
}

PVRSRV_ERROR PhysHeapCreatePMB(PHYS_HEAP *psPhysHeap,
                               IMG_DEVMEM_SIZE_T uiSize,
                               const IMG_CHAR *pszAnnotation,
                               PMB **ppsPMRPtr,
                               RA_BASE_T *puiBase,
                               RA_LENGTH_T *puiSize)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;

	PVR_LOG_RETURN_IF_FALSE(
		psPhysHeap->eType != PHYS_HEAP_TYPE_DLM_BAR ||
		psPhysHeap->eType != PHYS_HEAP_TYPE_DLM_PRIV,
		"Physheap type not DLM",
		PVRSRV_ERROR_INVALID_PARAMS);

	PVR_ASSERT(psImplFuncs->pfnCreatePMB);

	return psImplFuncs->pfnCreatePMB(psPhysHeap,
	                                 uiSize,
	                                 pszAnnotation,
	                                 ppsPMRPtr,
	                                 puiBase,
	                                 puiSize);
}

PPVRSRV_DEVICE_NODE PhysHeapDeviceNode(PHYS_HEAP *psPhysHeap)
{
	PVR_ASSERT(psPhysHeap != NULL);

	return psPhysHeap->psDevNode;
}

static IMG_BOOL PhysHeapCreatedByPVRLayer(PVRSRV_PHYS_HEAP ePhysHeap)
{
	PVR_ASSERT(ePhysHeap < PVRSRV_PHYS_HEAP_LAST);

	return (gasHeapProperties[ePhysHeap].ePVRLayerAction != PVR_LAYER_HEAP_ACTION_IGNORE);
}

static IMG_BOOL PhysHeapAcquiredByPVRLayer(PVRSRV_PHYS_HEAP ePhysHeap)
{
	PVR_ASSERT(ePhysHeap < PVRSRV_PHYS_HEAP_LAST);

	return (gasHeapProperties[ePhysHeap].ePVRLayerAction == PVR_LAYER_HEAP_ACTION_INITIALISE);
}

IMG_BOOL PhysHeapUserModeAlloc(PVRSRV_PHYS_HEAP ePhysHeap)
{
	PVR_ASSERT(ePhysHeap < PVRSRV_PHYS_HEAP_LAST);

	return gasHeapProperties[ePhysHeap].bUserModeAlloc;
}

#if defined(SUPPORT_GPUVIRT_VALIDATION)
PVRSRV_ERROR PhysHeapPagesAllocGPV(PHYS_HEAP *psPhysHeap, size_t uiSize,
                                   PG_HANDLE *psMemHandle,
                                   IMG_DEV_PHYADDR *psDevPAddr,
                                   IMG_UINT32 ui32OSid, IMG_PID uiPid)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;
	PVRSRV_ERROR eResult = PVRSRV_ERROR_NOT_IMPLEMENTED;

	if (psImplFuncs->pfnPagesAllocGPV != NULL)
	{
		eResult = psImplFuncs->pfnPagesAllocGPV(psPhysHeap,
		                                        uiSize, psMemHandle, psDevPAddr, ui32OSid, uiPid);
	}

	return eResult;
}
#endif

PVRSRV_ERROR PhysHeapPagesAlloc(PHYS_HEAP *psPhysHeap, size_t uiSize,
								PG_HANDLE *psMemHandle,
								IMG_DEV_PHYADDR *psDevPAddr,
								IMG_PID uiPid)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;
	PVRSRV_ERROR eResult = PVRSRV_ERROR_NOT_IMPLEMENTED;

	if (psImplFuncs->pfnPagesAlloc != NULL)
	{
		eResult = psImplFuncs->pfnPagesAlloc(psPhysHeap,
		                                       uiSize, psMemHandle, psDevPAddr, uiPid);
	}

	return eResult;
}

void PhysHeapPagesFree(PHYS_HEAP *psPhysHeap, PG_HANDLE *psMemHandle)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;

	PVR_ASSERT(psImplFuncs->pfnPagesFree != NULL);

	if (psImplFuncs->pfnPagesFree != NULL)
	{
		psImplFuncs->pfnPagesFree(psPhysHeap,
		                          psMemHandle);
	}
}

PVRSRV_ERROR PhysHeapPagesMap(PHYS_HEAP *psPhysHeap, PG_HANDLE *pshMemHandle, size_t uiSize, IMG_DEV_PHYADDR *psDevPAddr,
							  void **pvPtr)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;
	PVRSRV_ERROR eResult = PVRSRV_ERROR_NOT_IMPLEMENTED;

	if (psImplFuncs->pfnPagesMap != NULL)
	{
		eResult = psImplFuncs->pfnPagesMap(psPhysHeap,
		                                   pshMemHandle, uiSize, psDevPAddr, pvPtr);
	}

	return eResult;
}

void PhysHeapPagesUnMap(PHYS_HEAP *psPhysHeap, PG_HANDLE *psMemHandle, void *pvPtr)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;

	PVR_ASSERT(psImplFuncs->pfnPagesUnMap != NULL);

	if (psImplFuncs->pfnPagesUnMap != NULL)
	{
		psImplFuncs->pfnPagesUnMap(psPhysHeap,
		                           psMemHandle, pvPtr);
	}
}

PVRSRV_ERROR PhysHeapPagesClean(PHYS_HEAP *psPhysHeap, PG_HANDLE *pshMemHandle,
							  IMG_UINT32 uiOffset,
							  IMG_UINT32 uiLength)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;
	PVRSRV_ERROR eResult = PVRSRV_ERROR_NOT_IMPLEMENTED;

	if (psImplFuncs->pfnPagesClean != NULL)
	{
		eResult = psImplFuncs->pfnPagesClean(psPhysHeap,
		                                     pshMemHandle, uiOffset, uiLength);
	}

	return eResult;
}

IMG_UINT32 PhysHeapGetPageShift(PHYS_HEAP *psPhysHeap)
{
	PHEAP_IMPL_FUNCS *psImplFuncs = psPhysHeap->psImplFuncs;
	IMG_UINT32 ui32PageShift = 0;

	PVR_ASSERT(psImplFuncs->pfnGetPageShift != NULL);

	if (psImplFuncs->pfnGetPageShift != NULL)
	{
		ui32PageShift = psImplFuncs->pfnGetPageShift();
	}

	return ui32PageShift;
}

PVRSRV_ERROR PhysHeapFreeMemCheck(PHYS_HEAP *psPhysHeap,
                                  IMG_UINT64 ui64MinRequiredMem,
                                  IMG_UINT64 *pui64FreeMem)
{
	IMG_UINT64 ui64TotalSize;
	IMG_UINT64 ui64FreeSize;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psPhysHeap != NULL, "psPhysHeap");
	PVR_LOG_RETURN_IF_INVALID_PARAM(pui64FreeMem != NULL, "pui64FreeMem");


	if ((psPhysHeap->eType == PHYS_HEAP_TYPE_IMA_BAR) ||
		(psPhysHeap->eType == PHYS_HEAP_TYPE_IMA_PRIV))
	{
		PhysHeapIMAGetMemInfo(psPhysHeap->pvImplData,
		                            &ui64TotalSize,
		                            &ui64FreeSize);
	}
	else
	{
		psPhysHeap->psImplFuncs->pfnGetFactoryMemStats(psPhysHeap->pvImplData,
		                                               &ui64TotalSize,
		                                               &ui64FreeSize);
	}

	*pui64FreeMem = ui64FreeSize;
	if (ui64MinRequiredMem >= *pui64FreeMem)
	{
		eError = PVRSRV_ERROR_INSUFFICIENT_PHYS_HEAP_MEMORY;
	}

	return eError;
}

