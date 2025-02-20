/*************************************************************************/ /*!
@File
@Title          PowerVR services server header file
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

#ifndef PVRSRV_H
#define PVRSRV_H

#include "connection_server.h"
#include "pvrsrv_pool.h"
#include "device.h"
#include "power.h"
#include "syscommon.h"
#include "sysinfo.h"
#include "physheap.h"
#include "cache_ops.h"
#include "pvr_notifier.h"
#include "pvr_bridge.h"
#if defined(SUPPORT_RGX)
#include "rgx_bridge.h"
#endif
#if defined(__KERNEL__) && defined(__linux__) && !defined(__GENKSYMS__)
#define __pvrsrv_defined_struct_enum__
#include <services_kernel_client.h>
#endif


#if defined(SUPPORT_GPUVIRT_VALIDATION)
#include "virt_validation_defs.h"
#endif

#include "dma_support.h"
#include "vz_vmm_pvz.h"

/*!
 * For OSThreadDestroy(), which may require a retry
 * Try for 100 ms to destroy an OS thread before failing
 */
#define OS_THREAD_DESTROY_TIMEOUT_US 100000ULL
#define OS_THREAD_DESTROY_RETRY_COUNT 10

typedef enum _POLL_FLAGS_
{
	POLL_FLAG_NONE = 0, /* No message or dump is printed on poll timeout */
	POLL_FLAG_LOG_ERROR = 1, /* Log error on poll timeout */
	POLL_FLAG_DEBUG_DUMP = 2 /* Print debug dump on poll timeout */
} POLL_FLAGS;

typedef struct _BUILD_INFO_
{
	IMG_UINT32	ui32BuildOptions;
	IMG_UINT32	ui32BuildVersion;
	IMG_UINT32	ui32BuildRevision;
	IMG_UINT32	ui32BuildType;
#define BUILD_TYPE_DEBUG	0
#define BUILD_TYPE_RELEASE	1
	/* The above fields are self explanatory */
	/* B.V.N.C can be added later if required */
} BUILD_INFO;

typedef struct _DRIVER_INFO_
{
	BUILD_INFO	sUMBuildInfo;
	BUILD_INFO	sKMBuildInfo;
	IMG_UINT8	ui8UMSupportedArch;
	IMG_UINT8	ui8KMBitArch;

#define	BUILD_ARCH_64BIT			(1 << 0)
#define	BUILD_ARCH_32BIT			(1 << 1)
#define	BUILD_ARCH_BOTH		(BUILD_ARCH_32BIT | BUILD_ARCH_64BIT)
	IMG_BOOL	bIsNoMatch;
}DRIVER_INFO;


typedef struct PVRSRV_DATA_TAG
{
	DRIVER_INFO           sDriverInfo;
	IMG_UINT32            ui32DPFErrorCount;                 /*!< Number of Fatal/Error DPFs */

	POSWR_LOCK            hDeviceNodeListLock;            /*!< Read-Write lock to protect the list of devices */
	PVRSRV_DEVICE_NODE    *psDeviceNodeList;              /*!< List head of device nodes */
	IMG_UINT32            ui32RegisteredDevices;
	PVRSRV_DEVICE_NODE    *psHostMemDeviceNode;           /*!< DeviceNode to be used for device independent
	                                                        host based memory allocations where the DevMem
	                                                        framework is to be used e.g. TL */
	PVRSRV_SERVICES_STATE eServicesState;                 /*!< global driver state */

	IMG_HANDLE            hGlobalEventObject;             /*!< OS Global Event Object */
	IMG_UINT32            ui32GEOConsecutiveTimeouts;     /*!< OS Global Event Object Timeouts */

	IMG_HANDLE            hCleanupThread;                 /*!< Cleanup thread */
	IMG_HANDLE            hCleanupEventObject;            /*!< Event object to drive cleanup thread */
	POS_SPINLOCK          hCleanupThreadWorkListLock;     /*!< Lock protecting the cleanup thread work list */
	IMG_PID               cleanupThreadPid;               /*!< Cleanup thread process id */
	uintptr_t             cleanupThreadTid;               /*!< Cleanup thread id */
	ATOMIC_T              i32NumCleanupItemsQueued;       /*!< Number of items in cleanup thread work list */
	ATOMIC_T              i32NumCleanupItemsNotCompleted; /*!< Number of items dropped from cleanup thread work list
	                                                           after retry limit reached */
	ATOMIC_T              i32CleanupItemTypes[PVRSRV_CLEANUP_TYPE_LAST];         /*!< Array containing the counts for different cleanup item types. */

	IMG_HANDLE            hDevicesWatchdogThread;         /*!< Devices watchdog thread */
	IMG_HANDLE            hDevicesWatchdogEvObj;          /*! Event object to drive devices watchdog thread */
	volatile IMG_UINT32   ui32DevicesWatchdogPwrTrans;    /*! Number of off -> on power state transitions */
#if !defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP)
	volatile IMG_UINT32   ui32DevicesWatchdogTimeout;     /*! Timeout for the Devices watchdog Thread */
#endif

#if defined(SUPPORT_AUTOVZ)
	IMG_HANDLE            hAutoVzWatchdogThread;          /*!< Devices watchdog thread */
	IMG_HANDLE            hAutoVzWatchdogEvObj;           /*! Event object to drive devices watchdog thread */
#endif

	POS_LOCK              hHWPerfHostPeriodicThread_Lock; /*!< Lock for the HWPerf Host periodic thread */
	IMG_HANDLE            hHWPerfHostPeriodicThread;      /*!< HWPerf Host periodic thread */
	IMG_HANDLE            hHWPerfHostPeriodicEvObj;       /*! Event object to drive HWPerf thread */
	volatile IMG_BOOL     bHWPerfHostThreadStop;
	IMG_UINT32            ui32HWPerfHostThreadTimeout;

	IMG_HANDLE            hPvzConnection;                 /*!< PVZ connection used for cross-VM hyper-calls */
	POS_LOCK              hPvzConnectionLock;             /*!< Lock protecting PVZ connection */

	IMG_BOOL              bUnload;                        /*!< Driver unload is in progress */

	IMG_HANDLE            hTLCtrlStream;                  /*! Control plane for TL streams */

	IMG_HANDLE            hDriverThreadEventObject;       /*! Event object relating to multi-threading in the Server */
	IMG_BOOL              bDriverSuspended;               /*! if TRUE, the driver is suspended and new threads should not enter */
	ATOMIC_T              iNumActiveDriverThreads;        /*! Number of threads active in the Server */

	PMR                   *psInfoPagePMR;                 /*! Handle to exportable PMR of the information page. */
	IMG_UINT32            *pui32InfoPage;                 /*! CPU memory mapping for information page. */
	DEVMEM_MEMDESC        *psInfoPageMemDesc;             /*! Memory descriptor of the information page. */
	POS_LOCK              hInfoPageLock;                  /*! Lock guarding access to information page. */

	IMG_HANDLE            hThreadsDbgReqNotify;


	IMG_UINT32            ui32PDumpBoundDevice;           /*!< PDump is bound to the device first connected to */
	ATOMIC_T              iNumDriverTasksActive;          /*!< Number of device-agnostic tasks active in the server */
	PVRSRV_DRIVER_MODE    aeModuleParamDriverMode[PVRSRV_MAX_DEVICES]; /*!< Driver Mode for each device requested at launch */
} PVRSRV_DATA;

/* Function pointer used to invalidate cache between loops in wait/poll for value functions */
typedef PVRSRV_ERROR (*PFN_INVALIDATE_CACHEFUNC)(const volatile void*, IMG_UINT64, PVRSRV_CACHE_OP);

/*!
******************************************************************************
 @Function	PVRSRVGetPVRSRVData

 @Description	Get a pointer to the global data

 @Return   PVRSRV_DATA *
******************************************************************************/
PVRSRV_DATA *PVRSRVGetPVRSRVData(void);
PVRSRV_DRIVER_MODE PVRSRVGetVzModeByDevNum(IMG_UINT32 ui32DevNum);

#define PVRSRV_KM_ERRORS                     ( PVRSRVGetPVRSRVData() ? PVRSRVGetPVRSRVData()->ui32DPFErrorCount : IMG_UINT32_MAX)
#define PVRSRV_ERROR_LIMIT_REACHED                (PVRSRV_KM_ERRORS == IMG_UINT32_MAX)
#define PVRSRV_REPORT_ERROR()                do { if (!(PVRSRV_ERROR_LIMIT_REACHED)) { PVRSRVGetPVRSRVData()->ui32DPFErrorCount++; } } while (0)

#define PVRSRV_VZ_MODE_FROM_DEVNODE(pnode)     (pnode->psDevConfig->eDriverMode)
#define PVRSRV_VZ_MODE_FROM_DEVINFO(pdevinfo)  (pdevinfo->psDeviceNode->psDevConfig->eDriverMode)
#define PVRSRV_VZ_MODE_FROM_DEVCFG(pdevcfg)    (pdevcfg->eDriverMode)
#define PVRSRV_VZ_MODE_FROM_DEVID(devid)       (PVRSRVGetVzModeByDevNum(devid))

#define PVRSRV_VZ_MODE_IS(_expr, _struct, dev) (DRIVER_MODE_##_expr == PVRSRV_VZ_MODE_FROM_##_struct(dev))

#define PVRSRV_VZ_RETN_IF_MODE(_expr, _struct, dev)         do { if (  PVRSRV_VZ_MODE_IS(_expr, _struct, dev)) { return; } } while (0)
#define PVRSRV_VZ_RET_IF_MODE(_expr, _struct, dev, _rc)     do { if (  PVRSRV_VZ_MODE_IS(_expr, _struct, dev)) { return (_rc); } } while (0)

#define PVRSRV_VZ_TIME_SLICE_MAX	(100UL)

typedef struct _PHYS_HEAP_ITERATOR_ PHYS_HEAP_ITERATOR;

/*!
******************************************************************************
 @Function LMA_HeapIteratorCreate

 @Description
 Creates iterator for traversing physical heap requested by ePhysHeap. The
 iterator will go through all of the segments (a segment is physically
 contiguous) of the physical heap and return their CPU physical address and
 size.

 @Input psDevNode: Pointer to device node struct.
 @Input ePhysHeap: Find the matching heap.
 @Output ppsIter: Pointer to the iterator object.

 @Return PVRSRV_OK upon success and PVRSRV_ERROR otherwise.
******************************************************************************/
PVRSRV_ERROR LMA_HeapIteratorCreate(PVRSRV_DEVICE_NODE *psDevNode,
                                    PVRSRV_PHYS_HEAP ePhysHeap,
                                    PHYS_HEAP_ITERATOR **ppsIter);

/*!
******************************************************************************
 @Function LMA_HeapIteratorDestroy

 @Description
 Frees the iterator object created with LMA_HeapIteratorCreate.

 @Input psIter: Pointer to the iterator object.
******************************************************************************/
void LMA_HeapIteratorDestroy(PHYS_HEAP_ITERATOR *psIter);

/*!
******************************************************************************
 @Function LMA_HeapIteratorReset

 @Description
 Resets the iterator the first segment of the physical heap.

 @Input psIter: Pointer to the iterator object.
******************************************************************************/
PVRSRV_ERROR LMA_HeapIteratorReset(PHYS_HEAP_ITERATOR *psIter);

/*!
******************************************************************************
 @Function LMA_HeapIteratorNext

 @Description
 Retrieves current segment's physical device address and size and moves the
 iterator to the next element (if exists). If the iterator reached an end of
 the heap and no segment was retrieved, this function returns IMG_FALSE.

 @Input psIter: Pointer to the iterator object.
 @Output psDevPAddr: Device physical address of the current segment.
 @Output puiSize: Size of the current segment.

 @Return IMG TRUE if a segment was found and retrieved, IMG_FALSE otherwise.
******************************************************************************/
IMG_BOOL LMA_HeapIteratorNext(PHYS_HEAP_ITERATOR *psIter,
                              IMG_DEV_PHYADDR *psDevPAddr,
                              IMG_UINT64 *puiSize);

/*!
******************************************************************************
 @Function LMA_HeapIteratorGetHeapStats

 @Description
 Retrieves phys heap's usage statistics.

 @Input psPhysHeap: Pointer to the physical heap object.
 @Output puiTotalSize: Total size of the physical heap.
 @Output puiInUseSize: Used space in the physical heap.

 @Return PVRSRV_OK upon success and PVRSRV_otherwise.
******************************************************************************/
PVRSRV_ERROR LMA_HeapIteratorGetHeapStats(PHYS_HEAP_ITERATOR *psIter,
                                          IMG_UINT64 *puiTotalSize,
                                          IMG_UINT64 *puiInUseSize);

/*!
******************************************************************************
 @Function	PVRSRVPollForValueKM

 @Description
 Polls for a value to match a masked read

 @Input psDevNode : Pointer to device node struct
 @Input pui32LinMemAddr : CPU linear address to poll
 @Input ui32Value : required value
 @Input ui32Mask : Mask
 @Input bDebugDumpOnFailure : Whether poll failure should result into a debug
        dump. CAUTION: When calling this function from code paths which are
        also used by debug-dumping code, this argument MUST be IMG_FALSE
        otherwise, we might end up requesting debug-dump in recursion and
        eventually blow-up call stack.
 @Input pfnFwInvalidate : Function pointer to invalidation function used
        each loop / poll. This is only used for FWmemctx allocations.

 @Return   PVRSRV_ERROR :
******************************************************************************/
PVRSRV_ERROR PVRSRVPollForValueKM(PVRSRV_DEVICE_NODE *psDevNode,
		volatile IMG_UINT32 __iomem *pui32LinMemAddr,
		IMG_UINT32                   ui32Value,
		IMG_UINT32                   ui32Mask,
		POLL_FLAGS                   ePollFlags,
		PFN_INVALIDATE_CACHEFUNC     pfnFwInvalidate);

/*!
******************************************************************************
 @Function	PVRSRVWaitForValueKM

 @Description
 Waits (using EventObjects) for a value to match a masked read

 @Input  pui32LinMemAddr       : CPU linear address to poll
 @Input  ui32Value             : Required value
 @Input  ui32Mask              : Mask to be applied before checking against
                                 ui32Value
 @Input  pfnFwInvalidate       : Function pointer to invalidation function used
                                 each loop / wait. This is only used for
                                 FWmemctx allocations.
 @Return PVRSRV_ERROR          :
******************************************************************************/
PVRSRV_ERROR
PVRSRVWaitForValueKM(volatile IMG_UINT32 __iomem *pui32LinMemAddr,
                     IMG_UINT32                  ui32Value,
                     IMG_UINT32                  ui32Mask,
                     PFN_INVALIDATE_CACHEFUNC    pfnFwInvalidate);

/*!
******************************************************************************
 @Function	: PVRSRVSystemHasCacheSnooping

 @Description	: Returns whether the system has cache snooping

 @Return : IMG_TRUE if the system has cache snooping
******************************************************************************/
IMG_BOOL PVRSRVSystemHasCacheSnooping(PVRSRV_DEVICE_CONFIG *psDevConfig);

/*!
******************************************************************************
 @Function	: PVRSRVSystemSnoopingIsEmulated

 @Description : Returns whether system cache snooping support is emulated

 @Return : IMG_TRUE if the system cache snooping is emulated in software
******************************************************************************/
IMG_BOOL PVRSRVSystemSnoopingIsEmulated(PVRSRV_DEVICE_CONFIG *psDevConfig);

/*!
******************************************************************************
 @Function	: PVRSRVSystemSnoopingOfCPUCache

 @Description	: Returns whether the system supports snooping of the CPU cache

 @Return : IMG_TRUE if the system has CPU cache snooping
******************************************************************************/
IMG_BOOL PVRSRVSystemSnoopingOfCPUCache(PVRSRV_DEVICE_CONFIG *psDevConfig);

/*!
******************************************************************************
 @Function	: PVRSRVSystemHasNonMappableLocalMemory

 @Description	: Returns whether the device has non-mappable part of local memory

 @Return : IMG_TRUE if the device has non-mappable part of local memory
******************************************************************************/
IMG_BOOL PVRSRVSystemHasNonMappableLocalMemory(PVRSRV_DEVICE_CONFIG *psDevConfig);

/*!
******************************************************************************
 @Function	: PVRSRVSystemWaitCycles

 @Description	: Waits for at least ui32Cycles of the Device clk.
******************************************************************************/
void PVRSRVSystemWaitCycles(PVRSRV_DEVICE_CONFIG *psDevConfig, IMG_UINT32 ui32Cycles);

PVRSRV_ERROR PVRSRVSystemInstallDeviceLISR(void *pvOSDevice,
										   IMG_UINT32 ui32IRQ,
										   const IMG_CHAR *pszName,
										   PFN_LISR pfnLISR,
										   void *pvData,
										   IMG_HANDLE *phLISRData);

PVRSRV_ERROR PVRSRVSystemUninstallDeviceLISR(IMG_HANDLE hLISRData);

int PVRSRVGetDriverStatus(void);

/*!
******************************************************************************
 @Function	: PVRSRVIsBridgeEnabled

 @Description	: Returns whether the given bridge group is enabled

 @Return : IMG_TRUE if the given bridge group is enabled
******************************************************************************/
static inline IMG_BOOL PVRSRVIsBridgeEnabled(IMG_HANDLE hServices, IMG_UINT32 ui32BridgeGroup)
{
	IMG_UINT32 ui32Bridges;
	IMG_UINT32 ui32Offset;

	PVR_UNREFERENCED_PARAMETER(hServices);

#if defined(SUPPORT_RGX)
	if (ui32BridgeGroup >= PVRSRV_BRIDGE_RGX_FIRST)
	{
		ui32Bridges = gui32RGXBridges;
		ui32Offset = PVRSRV_BRIDGE_RGX_FIRST;
	}
	else
#endif /* SUPPORT_RGX */
	{
		ui32Bridges = gui32PVRBridges;
		ui32Offset = PVRSRV_BRIDGE_FIRST;
	}

	return (IMG_BOOL)(((1U << (ui32BridgeGroup - ui32Offset)) & ui32Bridges) != 0);
}


#if defined(SUPPORT_GPUVIRT_VALIDATION)
#if defined(EMULATOR)
	void SetAxiProtOSid(IMG_HANDLE hSysData, IMG_UINT32 ui32OSid, IMG_BOOL bState);
	void SetTrustedDeviceAceEnabled(IMG_HANDLE hSysData);
#endif
#endif

/*!
******************************************************************************
 @Function			: PVRSRVCreateHWPerfHostThread

 @Description		: Creates HWPerf event object and thread unless already created

 @Input ui32Timeout	: Initial timeout (ms) between updates on the HWPerf thread

 @Return			: PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
******************************************************************************/
PVRSRV_ERROR PVRSRVCreateHWPerfHostThread(IMG_UINT32 ui32Timeout);

/*!
******************************************************************************
 @Function			: PVRSRVDestroyHWPerfHostThread

 @Description		: Destroys HWPerf event object and thread if created

 @Return			: PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
******************************************************************************/
PVRSRV_ERROR PVRSRVDestroyHWPerfHostThread(void);

/*************************************************************************/ /*!
@Function       PVRSRVFindPhysHeapConfig
@Description    Find Phys Heap Config from Device Config.
@Input          psDevConfig  Pointer to device config.
@Input          ui32Flags    Find heap that matches flags.
@Return         PHYS_HEAP_CONFIG*  Return a config, or NULL if not found.
*/ /**************************************************************************/
PHYS_HEAP_CONFIG* PVRSRVFindPhysHeapConfig(PVRSRV_DEVICE_CONFIG *psDevConfig,
										   PHYS_HEAP_USAGE_FLAGS ui32Flags);

/*************************************************************************/ /*!
@Function       PVRSRVGetDeviceInstance
@Description    Return the specified device instance from Device node list.
@Input          ui32Instance         Device instance to find
@Return         PVRSRV_DEVICE_NODE*  Return a device node, or NULL if not found.
*/ /**************************************************************************/
PVRSRV_DEVICE_NODE* PVRSRVGetDeviceInstance(IMG_UINT32 ui32Instance);

/*************************************************************************/ /*!
@Function       PVRSRVGetDeviceInstanceByKernelDevID
@Description    Return the specified device instance by OS Id.
@Input          i32OSInstance        OS device Id to find
@Return         PVRSRV_DEVICE_NODE*  Return a device node, or NULL if not found.
*/ /**************************************************************************/
PVRSRV_DEVICE_NODE *PVRSRVGetDeviceInstanceByKernelDevID(IMG_INT32 i32OSInstance);

/*************************************************************************/ /*!
@Function       PVRSRVAcquireInternalID
@Description    Returns the lowest free device ID.
@Output         pui32InternalID  The device ID
@Return         PVRSRV_ERROR     PVRSRV_OK or an error code
*/ /**************************************************************************/
PVRSRV_ERROR PVRSRVAcquireInternalID(IMG_UINT32 *pui32InternalID);

/*************************************************************************/ /*!
@Function       PVRSRVDeviceFreeze
@Description    Stops further processing from occurring on the device after the
                 current work ends and blocks user-mode tasks on the specified
                 device. Device node is put into POWER-DOWN state.
@Input          PVRSRV_DEVICE_NODE*     Device node
@Return         PVRSRV_ERROR            PVRSRV_OK on success otherwise a
                                        PVRSRV_ERR_ code on failure
*/ /**************************************************************************/
PVRSRV_ERROR PVRSRVDeviceFreeze(PVRSRV_DEVICE_NODE *psDevNode);

/*************************************************************************/ /*!
@Function       PVRSRVDeviceThaw
@Description    Unfreezes a previously frozen device. Restarts work on the
                 device, if work queues are non-empty, and unblocks any
                 user-mode tasks on the specified device.
@Input          PVRSRV_DEVICE_NODE*     Device node
@Return         PVRSRV_ERROR            PVRSRV_OK on success otherwise a
                                        PVRSRV_ERR_ code on failure
*/ /**************************************************************************/
PVRSRV_ERROR PVRSRVDeviceThaw(PVRSRV_DEVICE_NODE *psDevNode);

#endif /* PVRSRV_H */
