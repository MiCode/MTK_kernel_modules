/*************************************************************************/ /*!
@File
@Title          Physical heap management header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Defines the interface for the physical heap management
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

#include "img_types.h"
#include "pvrsrv_error.h"
#include "pvrsrv_memallocflags.h"
#include "devicemem_typedefs.h"
#include "opaque_types.h"
#include "pmr_impl.h"
#include "physheap_config.h"
#include "pvrsrv_device.h"
#include "ra.h"

#ifndef PHYSHEAP_H
#define PHYSHEAP_H

#define B2KB(x) ((x) >> 10)
#define B2MB(x) ((x) >> 20)

static inline IMG_UINT64 KB2B(IMG_UINT64 ui64Kilobytes) { return ui64Kilobytes << 10; }
static inline IMG_UINT64 MB2B(IMG_UINT64 ui64Megabytes) { return ui64Megabytes << 20; }

typedef struct _PHYS_HEAP_ PHYS_HEAP;
#define INVALID_PHYS_HEAP 0xDEADDEAD

/* PMB (Physical Memory Block) */
typedef struct _PMB_ PMB;

typedef IMG_UINT32 PHYS_HEAP_POLICY;

/* Heap has default allocation policy and does not require
 * any additional OS Functionality. Physically contiguous
 * allocations are required for this physheap.
 */
#define PHYS_HEAP_POLICY_DEFAULT (0U)

/*
 * Heap has allocation strategy that may produce non
 * physically contiguous allocations, additional OS functionality
 * is required to map these allocations into the kernel.
 */
#define PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG      (1U)
#define PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG_MASK (1U)

struct _CONNECTION_DATA_;

typedef struct _PG_HANDLE_
{
	union
	{
		void *pvHandle;
		IMG_UINT64 ui64Handle;
	}u;
	/* The allocation order is log2 value of the number of pages to allocate.
	 * As such this is a correspondingly small value. E.g, for order 4 we
	 * are talking 2^4 * PAGE_SIZE contiguous allocation.
	 * DevPxAlloc API does not need to support orders higher than 4.
	 */
#if defined(SUPPORT_GPUVIRT_VALIDATION)
	IMG_BYTE    uiOrder;    /* Order of the corresponding allocation */
	IMG_BYTE    uiOSid;     /* OSid to use for allocation arena.
	                         * Connection-specific. */
	IMG_BYTE    uiPad1,
	            uiPad2;     /* Spare */
#else
	IMG_BYTE    uiOrder;    /* Order of the corresponding allocation */
	IMG_BYTE    uiPad1,
	            uiPad2,
	            uiPad3;     /* Spare */
#endif
} PG_HANDLE;

/*! Pointer to private implementation specific data */
typedef void *PHEAP_IMPL_DATA;

/*************************************************************************/ /*!
@Function       Callback function PFN_DESTROY_DATA
@Description    Destroy private implementation specific data.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
*/ /**************************************************************************/
typedef void (*PFN_DESTROY_DATA)(PHEAP_IMPL_DATA);
/*************************************************************************/ /*!
@Function       Callback function PFN_GET_DEV_PADDR
@Description    Get heap device physical address.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
@Output         IMG_DEV_PHYADDR    Device physical address.
@Return         PVRSRV_ERROR       PVRSRV_OK or error code
*/ /**************************************************************************/
typedef PVRSRV_ERROR (*PFN_GET_DEV_PADDR)(PHEAP_IMPL_DATA, IMG_DEV_PHYADDR*);
/*************************************************************************/ /*!
@Function       Callback function PFN_GET_CPU_PADDR
@Description    Get heap CPU physical address.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
@Output         IMG_CPU_PHYADDR    CPU physical address.
@Return         PVRSRV_ERROR       PVRSRV_OK or error code
*/ /**************************************************************************/
typedef PVRSRV_ERROR (*PFN_GET_CPU_PADDR)(PHEAP_IMPL_DATA, IMG_CPU_PHYADDR*);
/*************************************************************************/ /*!
@Function       Callback function PFN_GET_SIZE
@Description    Get size of heap.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
@Output         IMG_UINT64         Size of heap.
@Return         PVRSRV_ERROR       PVRSRV_OK or error code
*/ /**************************************************************************/
typedef PVRSRV_ERROR (*PFN_GET_SIZE)(PHEAP_IMPL_DATA, IMG_UINT64*);
/*************************************************************************/ /*!
@Function       Callback function PFN_GET_PAGE_SHIFT
@Description    Get heap log2 page shift.
@Return         IMG_UINT32         Log2 page shift
*/ /**************************************************************************/
typedef IMG_UINT32 (*PFN_GET_PAGE_SHIFT)(void);

/*************************************************************************/ /*!
@Function       Callback function PFN_GET_MEM_STATS
@Description    Get total and free memory size of the physical heap managed by
                the PMR Factory.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
@Output         IMG_UINT64         total Size of heap.
@Output         IMG_UINT64         free Size available in a heap.
@Return         none
*/ /**************************************************************************/
typedef void (*PFN_GET_MEM_STATS)(PHEAP_IMPL_DATA, IMG_UINT64 *, IMG_UINT64 *);

/*************************************************************************/ /*!
@Function       Callback function PFN_GET_HEAP_STATS_STR_ITER
@Description    Get string of heap memory spans constituting the heap. This
                function can be iterated on to print sequential lines of the
                heap data. Iterate until IMG_FALSE is returned
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
@InOut          IMG_CHAR           Pointer to string buffer to be populated
                                   with sequential heap data.
@Input          IMG_UINT32         Size of the string buffer.
@InOut          void**             Iter handle.
@Return         IMG_BOOL
*/ /**************************************************************************/
typedef IMG_BOOL (*PFN_GET_HEAP_SPANS_STR_ITER)(PHEAP_IMPL_DATA, IMG_CHAR *, IMG_UINT32, void **);

/*************************************************************************/ /*!
@Function       Callback function PFN_GET_HEAP_DLM_BACKING
@Description    Extract reference to DLM heap backing the current IMA heap.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
@InOut          PHYS_HEAP**        Pointer to DLM backing heap.
@Return         void
*/ /**************************************************************************/
typedef void (*PFN_GET_HEAP_DLM_BACKING)(PHEAP_IMPL_DATA, PHYS_HEAP **);

#if defined(SUPPORT_GPUVIRT_VALIDATION)
typedef PVRSRV_ERROR (*PFN_PAGES_ALLOC_GPV)(PHYS_HEAP *psPhysHeap, size_t uiSize,
                                            PG_HANDLE *psMemHandle, IMG_DEV_PHYADDR *psDevPAddr,
                                            IMG_UINT32 ui32OSid, IMG_PID uiPid);
#endif
typedef PVRSRV_ERROR (*PFN_PAGES_ALLOC)(PHYS_HEAP *psPhysHeap, size_t uiSize,
                                        PG_HANDLE *psMemHandle, IMG_DEV_PHYADDR *psDevPAddr,
                                        IMG_PID uiPid);

typedef void (*PFN_PAGES_FREE)(PHYS_HEAP *psPhysHeap, PG_HANDLE *psMemHandle);

typedef PVRSRV_ERROR (*PFN_PAGES_MAP)(PHYS_HEAP *psPhysHeap, PG_HANDLE *pshMemHandle,
                                      size_t uiSize, IMG_DEV_PHYADDR *psDevPAddr,
                                      void **pvPtr);

typedef void (*PFN_PAGES_UNMAP)(PHYS_HEAP *psPhysHeap,
                                PG_HANDLE *psMemHandle, void *pvPtr);

typedef PVRSRV_ERROR (*PFN_PAGES_CLEAN)(PHYS_HEAP *psPhysHeap,
                                        PG_HANDLE *pshMemHandle,
                                        IMG_UINT32 uiOffset,
                                        IMG_UINT32 uiLength);

/*************************************************************************/ /*!
@Function       Callback function PFN_CREATE_PMR
@Description    Create a PMR physical allocation and back with RAM on creation,
                if required. The RAM page comes either directly from
                the Phys Heap's associated pool of memory or from an OS API.
@Input          psPhysHeap         Pointer to Phys Heap.
@Input          psConnection       Pointer to device connection.
@Input          uiSize             Allocation size.
@Input          ui32NumPhysChunks  Physical chunk count.
@Input          ui32NumVirtChunks  Virtual chunk count.
@Input          pui32MappingTable  Mapping Table.
@Input          uiLog2PageSize     Page size.
@Input          uiFlags            Memalloc flags.
@Input          pszAnnotation      Annotation.
@Input          uiPid              Process ID.
@Output         ppsPMRPtr          Pointer to PMR.
@Input          ui32PDumpFlag      PDump flags.
@Return         PVRSRV_ERROR       PVRSRV_OK or error code
*/ /**************************************************************************/
typedef PVRSRV_ERROR (*PFN_CREATE_PMR)(PHYS_HEAP *psPhysHeap,
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
									   IMG_UINT32 ui32PDumpFlags);

/*************************************************************************/ /*!
@Function       Callback function PFN_CREATE_PMB
@Description    Create a PMB physical allocation and back with Card memory
                on creation, if required. The card memory comes
                directly from the DLM Phys Heap's associated pool of memory.
@Input          psPhysHeap         Pointer to Phys Heap to create the PMB on,
                                   physheap should be DLM type.
@Input          uiSize             Allocation size.
@Input          pszAnnotation      Annotation.
@Output         ppsPMBPtr          Pointer to PMB created.
@Output         puiBase            Out pointer to RA Base of PMB.
@Output         puiSize            Out pointer to size of PMB
@Return         PVRSRV_ERROR       PVRSRV_OK or error code
*/ /**************************************************************************/
typedef PVRSRV_ERROR (*PFN_CREATE_PMB)(PHYS_HEAP *psPhysHeap,
                                       IMG_DEVMEM_SIZE_T uiSize,
                                       const IMG_CHAR *pszAnnotation,
                                       PMB **ppsPMBPtr,
                                       RA_BASE_T *puiBase,
                                       RA_LENGTH_T *puiSize);

/*! Implementation specific function table */
typedef struct PHEAP_IMPL_FUNCS_TAG
{
	PFN_DESTROY_DATA pfnDestroyData;
	PFN_GET_DEV_PADDR pfnGetDevPAddr;
	PFN_GET_CPU_PADDR pfnGetCPUPAddr;
	PFN_GET_SIZE pfnGetSize;
	PFN_GET_PAGE_SHIFT pfnGetPageShift;
	PFN_GET_MEM_STATS pfnGetFactoryMemStats;
	PFN_GET_HEAP_SPANS_STR_ITER pfnGetHeapSpansStringIter;
	PFN_GET_HEAP_DLM_BACKING pfnGetHeapDLMBacking;
	PFN_CREATE_PMR pfnCreatePMR;
	PFN_CREATE_PMB pfnCreatePMB;
#if defined(SUPPORT_GPUVIRT_VALIDATION)
	PFN_PAGES_ALLOC_GPV pfnPagesAllocGPV;
#endif
	PFN_PAGES_ALLOC pfnPagesAlloc;
	PFN_PAGES_FREE pfnPagesFree;
	PFN_PAGES_MAP pfnPagesMap;
	PFN_PAGES_UNMAP pfnPagesUnMap;
	PFN_PAGES_CLEAN pfnPagesClean;
} PHEAP_IMPL_FUNCS;

/*************************************************************************/ /*!
@Function       PhysHeapInitDeviceHeaps
@Description    Registers and acquires physical memory heaps
@Input          psDeviceNode pointer to device node
@Input          psDevConfig  pointer to device config
@Return         PVRSRV_ERROR PVRSRV_OK on success, or a PVRSRV_ error code
*/ /**************************************************************************/
PVRSRV_ERROR PhysHeapInitDeviceHeaps(PPVRSRV_DEVICE_NODE psDeviceNode, PVRSRV_DEVICE_CONFIG *psDevConfig);

/*************************************************************************/ /*!
@Function       PhysHeapDeInitDeviceHeaps
@Description    Releases and unregisters physical memory heaps
@Input          psDeviceNode pointer to device node
@Return         PVRSRV_ERROR PVRSRV_OK on success, or a PVRSRV_ error code
*/ /**************************************************************************/
void PhysHeapDeInitDeviceHeaps(PPVRSRV_DEVICE_NODE psDeviceNode);

/*************************************************************************/ /*!
@Function       PhysHeapCreateHeapFromConfig
@Description    Create a new heap. Calls specific heap API depending
                on heap type.
@Input          psDevNode    Pointer to device node struct.
@Input          psConfig     Heap configuration.
@Output         ppsPhysHeap  Optional pointer to the created heap. Can be NULL
@Return         PVRSRV_ERROR PVRSRV_OK or error code
*/ /**************************************************************************/
PVRSRV_ERROR
PhysHeapCreateHeapFromConfig(PPVRSRV_DEVICE_NODE psDevNode,
							 PHYS_HEAP_CONFIG *psConfig,
							 PHYS_HEAP **ppsPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapCreate
@Description    Create a new heap. Allocated and stored internally.
                Destroy with PhysHeapDestroy when no longer required.
@Input          psDevNode    Pointer to device node struct
@Input          psConfig     Heap configuration.
@Input          uiPolicy     Phys heap allocation policy.
@Input          pvImplData   Implementation specific data. Can be NULL.
@Input          psImplFuncs  Implementation specific function table. Must be
                             a valid pointer.
@Output         ppsPhysHeap  Optional pointer to the created heap. Can be NULL
@Return         PVRSRV_ERROR PVRSRV_OK or error code
*/ /**************************************************************************/
PVRSRV_ERROR PhysHeapCreate(PPVRSRV_DEVICE_NODE psDevNode,
							PHYS_HEAP_CONFIG *psConfig,
							PHYS_HEAP_POLICY uiPolicy,
							PHEAP_IMPL_DATA pvImplData,
							PHEAP_IMPL_FUNCS *psImplFuncs,
							PHYS_HEAP **ppsPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapDestroyDeviceHeaps
@Description    Destroys all heaps referenced by a device.
@Input          psDevNode Pointer to a device node struct.
@Return         void
*/ /**************************************************************************/
void PhysHeapDestroyDeviceHeaps(PPVRSRV_DEVICE_NODE psDevNode);

void PhysHeapDestroy(PHYS_HEAP *psPhysHeap);

PVRSRV_ERROR PhysHeapAcquire(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapAcquireByID
@Description    Acquire PhysHeap by DevPhysHeap.
@Input          eDevPhysHeap Device Phys Heap.
@Input          psDevNode    Pointer to device node struct
@Output         ppsPhysHeap  PhysHeap if found.
@Return         PVRSRV_ERROR PVRSRV_OK or error code
*/ /**************************************************************************/
PVRSRV_ERROR PhysHeapAcquireByID(PVRSRV_PHYS_HEAP eDevPhysHeap,
								 PPVRSRV_DEVICE_NODE psDevNode,
								 PHYS_HEAP **ppsPhysHeap);

void PhysHeapRelease(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapGetImplData
@Description    Get physical heap implementation specific data.
@Input          psPhysHeap   Pointer to physical heap.
@Input          psConfig     Heap configuration.
@Return         pvImplData   Implementation specific data. Can be NULL.
*/ /**************************************************************************/
PHEAP_IMPL_DATA PhysHeapGetImplData(PHYS_HEAP *psPhysHeap);

PHYS_HEAP_TYPE PhysHeapGetType(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapGetPolicy
@Description    Get phys heap allocation policy flags.
@Input          psPhysHeap   Pointer to physical heap.
@Return         PHYS_HEAP_POLICY Phys heap policy flags.
*/ /**************************************************************************/
PHYS_HEAP_POLICY PhysHeapGetPolicy(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapGetFlags
@Description    Get phys heap usage flags.
@Input          psPhysHeap   Pointer to physical heap.
@Return         PHYS_HEAP_USAGE_FLAGS Phys heap usage flags.
*/ /**************************************************************************/
PHYS_HEAP_USAGE_FLAGS PhysHeapGetFlags(PHYS_HEAP *psPhysHeap);

IMG_BOOL PhysHeapValidateDefaultHeapExists(PPVRSRV_DEVICE_NODE psDevNode);

#if defined(SUPPORT_STATIC_IPA)
IMG_UINT32 PhysHeapGetIPAValue(PHYS_HEAP *psPhysHeap);

IMG_UINT32 PhysHeapGetIPAMask(PHYS_HEAP *psPhysHeap);

IMG_UINT32 PhysHeapGetIPAShift(PHYS_HEAP *psPhysHeap);
#endif

PVRSRV_ERROR PhysHeapGetCpuPAddr(PHYS_HEAP *psPhysHeap,
									   IMG_CPU_PHYADDR *psCpuPAddr);


PVRSRV_ERROR PhysHeapGetSize(PHYS_HEAP *psPhysHeap,
								   IMG_UINT64 *puiSize);

/*************************************************************************/ /*!
@Function       PhysHeapGetMemInfo
@Description    Get phys heap memory statistics for a given physical heap ID.
@Input          psDevNode          Pointer to device node struct
@Input          ui32PhysHeapCount  Physical heap count
@Input          paePhysHeapID      Physical heap ID
@Output         paPhysHeapMemStats Buffer that holds the memory statistics
@Return         PVRSRV_ERROR PVRSRV_OK or error code
*/ /**************************************************************************/
PVRSRV_ERROR
PhysHeapGetMemInfo(PPVRSRV_DEVICE_NODE psDevNode,
                   IMG_UINT32 ui32PhysHeapCount,
                   PVRSRV_PHYS_HEAP *paePhysHeapID,
                   PHYS_HEAP_MEM_STATS_PTR paPhysHeapMemStats);

PVRSRV_ERROR PhysHeapGetDevPAddr(PHYS_HEAP *psPhysHeap,
								 IMG_DEV_PHYADDR *psDevPAddr);

void PhysHeapCpuPAddrToDevPAddr(PHYS_HEAP *psPhysHeap,
								IMG_UINT32 ui32NumOfAddr,
								IMG_DEV_PHYADDR *psDevPAddr,
								IMG_CPU_PHYADDR *psCpuPAddr);

void PhysHeapDevPAddrToCpuPAddr(PHYS_HEAP *psPhysHeap,
								IMG_UINT32 ui32NumOfAddr,
								IMG_CPU_PHYADDR *psCpuPAddr,
								IMG_DEV_PHYADDR *psDevPAddr);

IMG_CHAR *PhysHeapPDumpMemspaceName(PHYS_HEAP *psPhysHeap);

const IMG_CHAR *PhysHeapName(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapCreatePMR
@Description    Function calls an implementation-specific function pointer.
                See function pointer for details.
@Return         PVRSRV_ERROR       PVRSRV_OK or error code
*/ /**************************************************************************/
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
							   PVRSRV_MEMALLOCFLAGS_T *uiOutFlags);

/*************************************************************************/ /*!
@Function       PhysHeapCreatePMB
@Description    Function calls an implementation-specific function pointer.
                See @Ref PFN_CREATE_PMB "PFN_CREATE_PMB" for details.
@Return         PVRSRV_ERROR       PVRSRV_OK or error code
*/ /**************************************************************************/
PVRSRV_ERROR PhysHeapCreatePMB(PHYS_HEAP *psPhysHeap,
                               IMG_DEVMEM_SIZE_T uiSize,
                               const IMG_CHAR *pszAnnotation,
                               PMB **ppsPMRPtr,
                               RA_BASE_T *puiBase,
                               RA_LENGTH_T *puiSize);

/*************************************************************************/ /*!
@Function       PhysHeapDeviceNode
@Description    Get pointer to the device node this heap belongs to.
@Input          psPhysHeap          Pointer to physical heap.
@Return         PPVRSRV_DEVICE_NODE Pointer to device node.
*/ /**************************************************************************/
PPVRSRV_DEVICE_NODE PhysHeapDeviceNode(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapInitByPVRLayer
@Description    Is phys heap to be initialised in PVR layer?
@Input          ePhysHeap           phys heap
@Return         IMG_BOOL            return IMG_TRUE if yes
*/ /**************************************************************************/
IMG_BOOL PhysHeapInitByPVRLayer(PVRSRV_PHYS_HEAP ePhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapUserModeAlloc
@Description    Is allocation from UM allowed?
@Input          ePhysHeap           phys heap
@Return         IMG_BOOL            return IMG_TRUE if yes
*/ /**************************************************************************/
IMG_BOOL PhysHeapUserModeAlloc(PVRSRV_PHYS_HEAP ePhysHeap);

#if defined(SUPPORT_GPUVIRT_VALIDATION)
PVRSRV_ERROR PhysHeapPagesAllocGPV(PHYS_HEAP *psPhysHeap,
                                   size_t uiSize,
                                   PG_HANDLE *psMemHandle,
                                   IMG_DEV_PHYADDR *psDevPAddr,
                                   IMG_UINT32 ui32OSid, IMG_PID uiPid);
#endif

PVRSRV_ERROR PhysHeapPagesAlloc(PHYS_HEAP *psPhysHeap,
                                size_t uiSize,
                                PG_HANDLE *psMemHandle,
                                IMG_DEV_PHYADDR *psDevPAddr,
                                IMG_PID uiPid);

void PhysHeapPagesFree(PHYS_HEAP *psPhysHeap,
                       PG_HANDLE *psMemHandle);

PVRSRV_ERROR PhysHeapPagesMap(PHYS_HEAP *psPhysHeap,
                              PG_HANDLE *pshMemHandle,
                              size_t uiSize,
                              IMG_DEV_PHYADDR *psDevPAddr,
                              void **pvPtr);

void PhysHeapPagesUnMap(PHYS_HEAP *psPhysHeap,
                        PG_HANDLE *psMemHandle,
                        void *pvPtr);

PVRSRV_ERROR PhysHeapPagesClean(PHYS_HEAP *psPhysHeap,
                                PG_HANDLE *pshMemHandle,
                                IMG_UINT32 uiOffset,
                                IMG_UINT32 uiLength);

/*************************************************************************/ /*!
@Function       PhysHeapGetPageShift
@Description    Get phys heap page shift.
@Input          psPhysHeap   Pointer to physical heap.
@Return         IMG_UINT32   Log2 page shift
*/ /**************************************************************************/
IMG_UINT32 PhysHeapGetPageShift(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapFreeMemCheck
@Description    Check a physheap has the required amount of free memory.

@Input          psPhysHeap          Pointer to physical heap.
@Input          ui64MinRequiredMem  The minimum free memory for success (bytes).
@Output         pui64FreeMem        The free memory in the physical heap (bytes).

@Return         PVRSRV_ERROR    If successful PVRSRV_OK else a PVRSRV_ERROR code.
*/ /**************************************************************************/
PVRSRV_ERROR PhysHeapFreeMemCheck(PHYS_HEAP *psPhysHeap,
                                  IMG_UINT64 ui64MinRequiredMem,
                                  IMG_UINT64 *pui64FreeMem);


#endif /* PHYSHEAP_H */
