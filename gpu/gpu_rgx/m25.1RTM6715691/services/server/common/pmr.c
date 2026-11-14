/*************************************************************************/ /*!
@File
@Title          Physmem (PMR) abstraction
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of the memory management.  This module is responsible for
                the "PMR" abstraction.  A PMR (Physical Memory Resource)
                represents some unit of physical memory which is
                allocated/freed/mapped/unmapped as an indivisible unit
                (higher software levels provide an abstraction above that
                to deal with dividing this down into smaller manageable units).
                Importantly, this module knows nothing of virtual memory, or
                of MMUs etc., with one excusable exception.  We have the
                concept of a "page size", which really means nothing in
                physical memory, but represents a "contiguity quantum" such
                that the higher level modules which map this memory are able
                to verify that it matches the needs of the page size for the
                virtual realm into which it is being mapped.
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
#include "img_defs.h"
#include "pvr_debug.h"
#include "pvrsrv_error.h"

#include "pdump.h"
#include "devicemem_server_utils.h"
#include "devicemem_server.h"

#include "osfunc.h"
#include "pdump_km.h"
#include "pdump_physmem.h"
#include "pmr_impl.h"
#include "pmr_os.h"
#include "pmr_env.h"
#include "pvrsrv.h"

#include "allocmem.h"
#include "lock.h"
#include "uniq_key_splay_tree.h"

#if defined(SUPPORT_SECURE_EXPORT)
#include "secure_export.h"
#include "ossecure_export.h"
#endif

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
#include "ri_server.h"
#endif

/* ourselves */
#include "pmr.h"

#if defined(PVRSRV_ENABLE_LINUX_MMAP_STATS)
#include "mmap_stats.h"
#endif

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#include "process_stats.h"
#include "proc_stats.h"
#endif

#include "pdump_km.h"

#define PMR_FLAG_INTERNAL_SPARSE_ALLOC        (1 << 0)
#define PMR_FLAG_INTERNAL_NO_LAYOUT_CHANGE    (1 << 1)
#if defined(SUPPORT_PMR_DEFERRED_FREE)
#define PMR_FLAG_INTERNAL_DEFER_FREE          (1 << 2)
#define PMR_FLAG_INTERNAL_IS_ZOMBIE_PMR       (1 << 3)
#define PMR_FLAG_INTERNAL_IS_ZOMBIE_PMR_EMPTY (1 << 5)
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */
#define PMR_FLAG_INTERNAL_IS_EXCLUSIVE        (1 << 4)

#if defined(SUPPORT_PMR_DEFERRED_FREE)
/* Indicates PMR should be destroyed immediately and not deferred. */
#define PMR_NO_ZOMBIE_FENCE IMG_UINT64_MAX
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

/* Memalloc flags can be converted into pmr, ra or psplay flags.
 * Ensure flags types are same size.
 */
static_assert(sizeof(PVRSRV_MEMALLOCFLAGS_T) == sizeof(PMR_FLAGS_T),
			  "Mismatch memalloc and pmr flags type size.");
static_assert(sizeof(PVRSRV_MEMALLOCFLAGS_T) == sizeof(RA_FLAGS_T),
			  "Mismatch memalloc and ra flags type size.");
static_assert(sizeof(PVRSRV_MEMALLOCFLAGS_T) == sizeof(IMG_PSPLAY_FLAGS_T),
			  "Mismatch memalloc and psplay flags type size.");

/* A "context" for the physical memory block resource allocator.
 *
 * Context is probably the wrong word.
 *
 * There is almost certainly only one of these, ever, in the system.
 * But, let's keep the notion of a context anyway, "just-in-case".
 */
static struct _PMR_CTX_
{
	/* For debugging, and PDump, etc., let's issue a forever incrementing
	 * serial number to each allocation.
	 */
	IMG_UINT64 uiNextSerialNum;

	/* For security, we only allow a PMR to be mapped if the caller knows
	 * its key. We can pseudo-randomly generate keys
	 */
	IMG_UINT64 uiNextKey;

	/* For debugging only, I guess: Number of live PMRs */
	ATOMIC_T uiNumLivePMRs;

	/* Lock for this structure */
	POS_LOCK hLock;

	/* In order to seed the uiNextKey, we enforce initialisation at driver
	 * load time. Also, we can debug check at driver unload that the PMR
	 * count is zero.
	 */
	IMG_BOOL bModuleInitialised;
} _gsSingletonPMRContext = { 1, 0, {0}, NULL, IMG_FALSE };

#if defined(SUPPORT_PMR_DEFERRED_FREE)
typedef enum _PMR_ZOMBIE_TYPE_ {
	PMR_ZOMBIE_TYPE_PMR,
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	PMR_ZOMBIE_TYPE_PAGES,
#endif /* defined(SUPPORT_PMR_PAGES_DEFERRED_FREE) */
#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
	PMR_ZOMBIE_TYPE_DEVICE_IMPORT,
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */
} PMR_ZOMBIE_TYPE;

typedef struct _PMR_HEADER_
{
	/* List node used to put the header on the zombie list
	 * (psDevNode->sPMRZombieList). */
	DLLIST_NODE sZombieNode;

	PMR_ZOMBIE_TYPE eZombieType;
} PMR_HEADER;
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
/*
 * A structure describing zombie pages.
 */
typedef struct _PMR_ZOMBIE_PAGES_
{
	PMR_HEADER sHeader;
	PMR_IMPL_ZOMBIEPAGES pvFactoryPages;
	PFN_FREE_ZOMBIE_PAGES_FN pfnFactoryFreeZombies;
	PMR_IMPL_TYPE ePagesFlavour;
} PMR_ZOMBIE_PAGES;
#endif

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
typedef struct _PMR_DEVICE_IMPORT_
{
	PMR_HEADER sHeader;             /* psDevNode zombie queue list node. */
	DLLIST_NODE sNext;              /* PMR::sXDeviceImports list node. */
	PVRSRV_DEVICE_NODE *psDevNode;  /* Device this import is representing. */
	PMR *psParent;                  /* PMR the import belongs to. */
} PMR_DEVICE_IMPORT;
#endif

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
typedef enum _PMR_STATE_
{
	PMR_STATE_INIT,
	PMR_STATE_ACTIVE,
	PMR_STATE_PAGES_IN_MIGRATE
} PMR_STATE;
#endif

/* A PMR. One per physical allocation. May be "shared".
 *
 * "shared" is ambiguous. We need to be careful with terminology.
 * There are two ways in which a PMR may be "shared" and we need to be sure
 * that we are clear which we mean.
 *
 * i)   multiple small allocations living together inside one PMR.
 *
 * ii)  one single allocation filling a PMR but mapped into multiple memory
 *      contexts.
 *
 * This is more important further up the stack - at this level, all we care is
 * that the PMR is being referenced multiple times.
 */
struct _PMR_
{
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	/* A Common header structure shared between PMR and PMR-like PMR_ZOMBIE_PAGES object */
	PMR_HEADER sHeader;
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

	/* This object is strictly refcounted. References include:
	 * - mapping
	 * - live handles (to this object)
	 * - live export handles
	 * (thus it is normal for allocated and exported memory to have a refcount of 3)
	 * The object is destroyed when and only when the refcount reaches 0
	 */

	/* Physical address translation (device <> cpu) is done on a per device
	 * basis which means we need the physical heap info
	 */
	PHYS_HEAP *psPhysHeap;

	/* Reference count of the PMR. */
	IMG_UINT32 uiRefCount;
	/* Lock protecting reference counting (uiRefCount). */
	POS_SPINLOCK hRefCountLock;

	/* Client CPU mapping count - this is the number of times the PMR has been
	 * mapped by the client to the CPU. It is used to determine when it
	 * is safe to permit modification of a sparse allocation's layout.
	 * Note that the process of mapping also increments uiRefCount
	 * independently (as that is used to determine when a PMR may safely
	 * be destroyed).
	 */
	ATOMIC_T iClientCpuMapCount;

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)

	/* Kernel CPU mapping count - number of times the PMR has been
	 * mapped into the kernel. Used to signal if migrate is allowed
	 * to occur.
	 */
	ATOMIC_T iKernelCpuMapCount;
	/*
	 * GPU mappings associated with PMR.
	 * Must be protected with PMR lock.
	 */
	DLLIST_NODE sGpuMappingListHead;

	/*
	 * Current state of PMR
	 * Must be protected by PMR lock.
	 * Used to describe the current state of the PMR and determine which interactions
	 * are possible. Pages could be in progress of migration during which physical
	 * addresses cannot be requested.
	 */
	PMR_STATE eState;
#endif

	/* Count of how many reservations refer to this
	 * PMR as a part of a GPU mapping. Must be protected
	 * by PMR lock.
	 */
	IMG_INT32 iAssociatedResCount;

	/* Lock count - this is the number of times PMRLockPhysAddresses()
	 * has been called, less the number of PMRUnlockPhysAddresses()
	 * calls. This is arguably here for debug reasons only, as the refcount
	 * is already incremented as a matter of course.
	 * Really, this just allows us to trap protocol errors: i.e. calling
	 * PMR_DevPhysAddr(), without a lock, or calling
	 * PMRUnlockPhysAddresses() too many or too few times.
	 */
	ATOMIC_T iLockCount;

	/* Lock for this structure */
	POS_LOCK hLock;
	/* Protects: `uiInternalFlags` & `uiDevImportBitmap` */
	POS_SPINLOCK hBitmapLock;

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) || defined(PVRSRV_ENABLE_XD_MEM)
	/* See PMR_ImportedDevicesMask()
	 * Protected by hBitmapLock. */
	IMG_UINT64 uiDevImportBitmap;
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) || defined(PVRSRV_ENABLE_XD_MEM) */
#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
	/* List of PMR_DEVICE_IMPORT's */
	DLLIST_NODE sXDeviceImports;
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

	/* Incrementing serial number to each allocation. */
	IMG_UINT64 uiSerialNum;

	/* For security, we only allow a PMR to be mapped if the caller knows
	 * its key. We can pseudo-randomly generate keys
	 */
	PMR_PASSWORD_T uiKey;

	/* Callbacks for per-flavour functions */
	const PMR_IMPL_FUNCTAB *psFuncTab;

	/* Data associated with the "subtype" */
	PMR_IMPL_PRIVDATA pvFlavourData;

	/* Environment specific data */
	DECLARE_PMR_ENV_DATA(sEnvData)

	/* What kind of PMR do we have? */
	PMR_IMPL_TYPE eFlavour;

	/* And for pdump */
	const IMG_CHAR *pszPDumpDefaultMemspaceName;

	/* Allocation annotation */
	IMG_CHAR szAnnotation[DEVMEM_ANNOTATION_MAX_LEN];

#if defined(PDUMP)

	IMG_HANDLE hPDumpAllocHandle;

	IMG_UINT32 uiNumPDumpBlocks;
#endif

	/* Logical size of allocation. "logical", because a PMR can represent
	 * memory that will never physically exist.  This is the amount of
	 * virtual space that the PMR would consume when it's mapped into a
	 * virtual allocation.
	 */
	PMR_SIZE_T uiLogicalSize;

	/* Mapping table for the allocation.
	 * PMR's can be sparse in which case not all the "logic" addresses in
	 * it are valid. We need to know which addresses are and aren't valid
	 * when mapping or reading the PMR.
	 * The mapping table translates "logical" offsets into physical offsets
	 * which is what we always pass to the PMR factory (so it doesn't have
	 * to be concerned about sparseness issues)
	 */
	PMR_MAPPING_TABLE *psMappingTable;

	/* Minimum Physical Contiguity Guarantee.  Might be called "page size",
	 * but that would be incorrect, as page size is something meaningful
	 * only in virtual realm. This contiguity guarantee provides an
	 * inequality that can be verified/asserted/whatever to ensure that
	 * this PMR conforms to the page size requirement of the place the PMR
	 * gets mapped. (May be used to select an appropriate heap in variable
	 * page size systems)
	 *
	 * The absolutely necessary condition is this:
	 *
	 *    device MMU page size <= actual physical contiguity.
	 *
	 * We go one step further in order to be able to provide an early
	 * warning / early compatibility check and say this:
	 *
	 *     device MMU page size <=
	 *         2**(uiLog2ContiguityGuarantee) <=
	 *             actual physical contiguity.
	 *
	 * In this way, it is possible to make the page table reservation
	 * in the device MMU without even knowing the granularity of the
	 * physical memory (i.e. useful for being able to allocate virtual
	 * before physical)
	 */
	PMR_LOG2ALIGN_T uiLog2ContiguityGuarantee;

	/* Flags. We store a copy of the "PMR flags" (usually a subset of the
	 * flags given at allocation time) and return them to any caller of
	 * PMR_Flags(). The intention of these flags is that the ones stored
	 * here are used to represent permissions, such that no one is able
	 * to map a PMR in a mode in which they are not allowed, e.g.,
	 * writeable for a read-only PMR, etc.
	 */
	PMR_FLAGS_T uiFlags;

	/* Various flags informing about PMR's state:
	 *
	 * SPARSE_ALLOC:
	 *   indicates whether this PMR has been allocated as sparse.
	 *   The condition for this variable to be set at allocation time is:
	 *   (numVirtChunks != numPhysChunks) || (numVirtChunks > 1)
	 *
	 * NO_LAYOUT_CHANGE:
	 *   Flag that conveys mutability of the PMR:
	 *   - set:   indicates the PMR is immutable (no more memory changes)
	 *   - unset: means the memory layout associated with the PMR is mutable
	 *
	 *   A PMR is always mutable by default but is marked immutable on the
	 *   first export for the rest of its life.
	 *
	 *   Also, any PMRs that track the same memory through imports are
	 *   marked immutable as well.
	 *
	 * DEFER_FREE:
	 *   If present the PMR is marked to be freed by the CleanupThread.
	 *
	 * IS_ZOMBIE:
	 *   Indicates if the PMR is in the zombie state (marked for free in the
	 *   CleanupThread). */
	IMG_UINT32 uiInternalFlags;

	/* Do we really need this?
	 * For now we'll keep it, until we know we don't.
	 * NB: this is not the "memory context" in client terms - this is
	 * _purely_ the "PMR" context, of which there is almost certainly only
	 * ever one per system as a whole, but we'll keep the concept anyway,
	 * just-in-case.
	 */
	struct _PMR_CTX_ *psContext;

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
	/* Stored handle to PMR RI entry */
	void		*hRIHandle;
#endif
};

/* Do we need a struct for the export handle?
 * I'll use one for now, but if nothing goes in it, we'll lose it
 */
struct _PMR_EXPORT_
{
	struct _PMR_ *psPMR;
};

struct _PMR_PAGELIST_
{
	struct _PMR_ *psReferencePMR;
};

static INLINE IMG_UINT32
_GetRef(const PMR *const psPMR)
{
	OS_SPINLOCK_FLAGS uiFlags;
	IMG_UINT32 uiRefCount;

	OSSpinLockAcquire(psPMR->hRefCountLock, uiFlags);
	uiRefCount = psPMR->uiRefCount;
	OSSpinLockRelease(psPMR->hRefCountLock, uiFlags);

	return uiRefCount;
}

#if defined(PDUMP)
static INLINE IMG_BOOL _IsHostDevicePMR(const PMR *const psPMR)
{
	const PVRSRV_DEVICE_NODE *psDevNode = PVRSRVGetPVRSRVData()->psHostMemDeviceNode;
	return psPMR->psPhysHeap == psDevNode->apsPhysHeap[PVRSRV_PHYS_HEAP_CPU_LOCAL];
}

static void
PDumpPMRFreePMR(PMR *psPMR,
                IMG_DEVMEM_SIZE_T uiSize,
                IMG_DEVMEM_ALIGN_T uiBlockSize,
                IMG_UINT32 uiLog2Contiguity,
                IMG_HANDLE hPDumpAllocationInfoHandle);

static void
PDumpPMRMallocPMR(PMR *psPMR,
                  IMG_DEVMEM_SIZE_T uiSize,
                  IMG_UINT32 ui32ChunkSize,
                  IMG_UINT32 ui32NumPhysChunks,
                  IMG_UINT32 ui32NumLogicalChunks,
                  IMG_UINT32 *puiMappingTable,
                  IMG_UINT32 uiLog2Contiguity,
                  IMG_BOOL bInitialise,
                  IMG_UINT8 ui8InitValue,
                  IMG_HANDLE *phPDumpAllocInfoOut,
                  IMG_UINT32 ui32PDumpFlags);

static void
PDumpPMRChangeSparsePMR(PMR *psPMR,
                        IMG_UINT32 uiBlockSize,
                        IMG_UINT32 ui32AllocPageCount,
                        IMG_UINT32 *pai32AllocIndices,
                        IMG_UINT32 ui32FreePageCount,
                        IMG_UINT32 *pai32FreeIndices,
                        IMG_BOOL bInitialise,
                        IMG_UINT8 ui8InitValue,
                        IMG_HANDLE *phPDumpAllocInfoOut);
#endif /* defined PDUMP */

IMG_INT32 PMRGetLiveCount(void)
{
	return OSAtomicRead(&_gsSingletonPMRContext.uiNumLivePMRs);
}

#if defined(SUPPORT_INSECURE_EXPORT)
PPVRSRV_DEVICE_NODE PMRGetExportDeviceNode(PMR_EXPORT *psExportPMR)
{
	PPVRSRV_DEVICE_NODE psReturnedDeviceNode = NULL;

	PVR_ASSERT(psExportPMR != NULL);
	if (psExportPMR)
	{
		PVR_ASSERT(psExportPMR->psPMR != NULL);
		if (psExportPMR->psPMR)
		{
			IMG_UINT32 uiRefCount = _GetRef(psExportPMR->psPMR);
			if (uiRefCount > 0)
			{
				psReturnedDeviceNode = PMR_DeviceNode(psExportPMR->psPMR);
			}
#ifdef PVRSRV_NEED_PVR_ASSERT
			else
			{
				PVR_ASSERT(IMG_FALSE);
			}
#endif
		}
	}

	return psReturnedDeviceNode;
}
#endif /* defined(SUPPORT_INSECURE_EXPORT) */

static PVRSRV_ERROR
_PMRCreate(PMR_SIZE_T uiLogicalSize,
           IMG_UINT32 ui32NumPhysChunks,
           IMG_UINT32 ui32NumLogicalChunks,
           IMG_UINT32 *pui32MappingTable,
           PMR_LOG2ALIGN_T uiLog2ContiguityGuarantee,
           PMR_FLAGS_T uiFlags,
           PMR **ppsPMR)
{
	void *pvPMRLinAddr;
	PMR *psPMR;
	PMR_MAPPING_TABLE *psMappingTable;
	struct _PMR_CTX_ *psContext;
	IMG_UINT32 i, ui32Temp = 0;
	PVRSRV_ERROR eError;
	IMG_BOOL bSparse = IMG_FALSE;
	PMR_SIZE_T uiChunkSize;

	psContext = &_gsSingletonPMRContext;

	/* Do we have a sparse allocation? */
	if ( (ui32NumLogicalChunks != ui32NumPhysChunks) ||
			(ui32NumLogicalChunks > 1) )
	{
		bSparse = IMG_TRUE;
		uiChunkSize = 1ULL << uiLog2ContiguityGuarantee;
	}
	else
	{
		uiChunkSize = uiLogicalSize;
	}

	/* Extra checks required for sparse PMRs */
	if (bSparse)
	{
		/* Check the logical size and chunk information agree with each other */
		if (uiLogicalSize != (uiChunkSize * ui32NumLogicalChunks))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Bad mapping size (uiLogicalSize = 0x%llx, uiChunkSize = 0x%llx, ui32NumLogicalChunks = %d)",
					__func__, (unsigned long long)uiLogicalSize, (unsigned long long)uiChunkSize, ui32NumLogicalChunks));
			return PVRSRV_ERROR_PMR_BAD_MAPPINGTABLE_SIZE;
		}
	}

	pvPMRLinAddr = OSAllocMem(sizeof(*psPMR) + sizeof(*psMappingTable) + IMG_FLEX_ARRAY_SIZE(sizeof(IMG_UINT32), ui32NumLogicalChunks));
	PVR_RETURN_IF_NOMEM(pvPMRLinAddr);

	psPMR = (PMR *) pvPMRLinAddr;
	psMappingTable = IMG_OFFSET_ADDR(pvPMRLinAddr, sizeof(*psPMR));

	/* Setup the mapping table */
	psMappingTable->uiChunkSize = uiChunkSize;
	psMappingTable->ui32NumLogicalChunks = ui32NumLogicalChunks;
	psMappingTable->ui32NumPhysChunks = ui32NumPhysChunks;
	OSCachedMemSet(psMappingTable->aui32Translation, 0xFF,
	               sizeof(psMappingTable->aui32Translation[0]) * ui32NumLogicalChunks);
	for (i=0; i<ui32NumPhysChunks; i++)
	{
		ui32Temp = pui32MappingTable[i];
		PVR_ASSERT(ui32Temp < ui32NumLogicalChunks);
		psMappingTable->aui32Translation[ui32Temp] = ui32Temp;
	}

	eError = OSLockCreate(&psPMR->hLock);
	PVR_GOTO_IF_ERROR(eError, ErrFreePMR);

	eError = OSSpinLockCreate(&psPMR->hBitmapLock);
	PVR_GOTO_IF_ERROR(eError, ErrFreePMRLock);

	eError = OSSpinLockCreate(&psPMR->hRefCountLock);
	PVR_GOTO_IF_ERROR(eError, ErrFreeBitmapLock);

	/* Setup the PMR */
	psPMR->uiRefCount = 0;
	OSAtomicWrite(&psPMR->iClientCpuMapCount, 0);
#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	OSAtomicWrite(&psPMR->iKernelCpuMapCount, 0);
	dllist_init(&psPMR->sGpuMappingListHead);
#endif
	psPMR->iAssociatedResCount = 0;

	/* If allocation is not made on demand, it will be backed now and
	 * backing will not be removed until the PMR is destroyed, therefore
	 * we can initialise the iLockCount to 1 rather than 0.
	 */
	OSAtomicWrite(&psPMR->iLockCount, (PVRSRV_CHECK_ON_DEMAND(uiFlags) ? 0 : 1));

	psPMR->psContext = psContext;
	psPMR->uiLogicalSize = uiLogicalSize;
	psPMR->uiLog2ContiguityGuarantee = uiLog2ContiguityGuarantee;
	psPMR->uiFlags = uiFlags;
	psPMR->psMappingTable = psMappingTable;
	psPMR->uiInternalFlags = bSparse ? PMR_FLAG_INTERNAL_SPARSE_ALLOC : 0;
	psPMR->szAnnotation[0] = '\0';
	PMR_ENV_INITIALIZE(psPMR, sEnvData);

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) || defined(PVRSRV_ENABLE_XD_MEM)
	psPMR->uiDevImportBitmap = 0;
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) || defined(PVRSRV_ENABLE_XD_MEM) */

#if defined(SUPPORT_PMR_DEFERRED_FREE)
	psPMR->sHeader.eZombieType = PMR_ZOMBIE_TYPE_PMR;
	dllist_init(&psPMR->sHeader.sZombieNode);
#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
	dllist_init(&psPMR->sXDeviceImports);
#endif /* defined(SUPPORT_DEVICE_IMPORT_DEFERRED_FREE) */
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
	psPMR->hRIHandle = NULL;
#endif
	OSLockAcquire(psContext->hLock);
	psPMR->uiKey = psContext->uiNextKey;
	psPMR->uiSerialNum = psContext->uiNextSerialNum;
	psContext->uiNextKey = (0x80200003 * psContext->uiNextKey)
								^ (0xf00f0081 * (uintptr_t)pvPMRLinAddr);
	psContext->uiNextSerialNum++;
	*ppsPMR = psPMR;
	OSLockRelease(psContext->hLock);

	PVR_DPF((PVR_DBG_MESSAGE, "%s: 0x%p, key:0x%016" IMG_UINT64_FMTSPECX ", numLive:%d",
			__func__, psPMR, psPMR->uiKey, OSAtomicRead(&psPMR->psContext->uiNumLivePMRs)));

	/* Increment live PMR count */
	OSAtomicIncrement(&psContext->uiNumLivePMRs);

	return PVRSRV_OK;

ErrFreeBitmapLock:
	OSSpinLockDestroy(psPMR->hBitmapLock);
ErrFreePMRLock:
	OSLockDestroy(psPMR->hLock);
ErrFreePMR:
	OSFreeMem(psPMR);

	return eError;
}

static PVRSRV_ERROR
_Ref(PMR *psPMR, IMG_UINT32 uiRefCount)
{
	OS_SPINLOCK_FLAGS uiFlags;

	OSSpinLockAcquire(psPMR->hRefCountLock, uiFlags);

	if (psPMR->uiRefCount == 0)
	{
		OSSpinLockRelease(psPMR->hRefCountLock, uiFlags);

		PVR_DPF((PVR_DBG_ERROR, "pmr.c: Ref Count == 0 PMR: @0x%p Annot: %s",
		                        psPMR,
		                        psPMR->szAnnotation));
		OSWarnOn(1);

		return PVRSRV_ERROR_REFCOUNT_OVERFLOW;
	}
	else if (psPMR->uiRefCount >= IMG_UINT32_MAX - uiRefCount)
	{
		OSSpinLockRelease(psPMR->hRefCountLock, uiFlags);

		PVR_DPF((PVR_DBG_ERROR, "pmr.c: Ref Count >= IMG_UINT32_MAX PMR: @0x%p "
		                        "Annot: %s RefCount: %u",
		                        psPMR,
		                        psPMR->szAnnotation,
		                        uiRefCount));
		OSWarnOn(1);

		return PVRSRV_ERROR_REFCOUNT_OVERFLOW;
	}

	psPMR->uiRefCount += uiRefCount;

	OSSpinLockRelease(psPMR->hRefCountLock, uiFlags);

	return PVRSRV_OK;
}

static PVRSRV_ERROR
_Unref(PMR *psPMR, IMG_INT32 uiRefCount, IMG_UINT32 *pui32RefCount)
{
	OS_SPINLOCK_FLAGS uiFlags;

	OSSpinLockAcquire(psPMR->hRefCountLock, uiFlags);

	if (psPMR->uiRefCount == 0)
	{
		OSSpinLockRelease(psPMR->hRefCountLock, uiFlags);

		PVR_DPF((PVR_DBG_ERROR, "pmr.c: Unref Count = 0 PMR: @0x%p Annot: %s "
		                        "RefCount: %u",
		                        psPMR,
		                        psPMR->szAnnotation,
		                        uiRefCount));
		OSWarnOn(1);

		return PVRSRV_ERROR_REFCOUNT_OVERFLOW;
	}

	psPMR->uiRefCount -= uiRefCount;

	if (pui32RefCount != NULL)
	{
		*pui32RefCount = psPMR->uiRefCount;
	}

	OSSpinLockRelease(psPMR->hRefCountLock, uiFlags);

	return PVRSRV_OK;
}

#if defined(DEBUG)
void PMRLockHeldAssert(const PMR *psPMR)
{
	OSLockHeldAssert(psPMR->hLock);
}
#endif

void
PMRLockPMR(const PMR *psPMR)
{
	OSLockAcquire(psPMR->hLock);	/* Uses same lock as PhysAddresses */
}

void
PMRUnlockPMR(const PMR *psPMR)
{
	OSLockRelease(psPMR->hLock);	/* Uses same lock as PhysAddresses */
}

static INLINE void _IntFlagSet(PMR *psPMR, const IMG_UINT32 uiValue)
{
	OS_SPINLOCK_FLAGS uiLockingFlags;

	OSSpinLockAcquire(psPMR->hBitmapLock, uiLockingFlags);
	BITMASK_SET(psPMR->uiInternalFlags, uiValue);
	OSSpinLockRelease(psPMR->hBitmapLock, uiLockingFlags);
}

static INLINE void _IntFlagClr(PMR *psPMR, const IMG_UINT32 uiValue)
{
	OS_SPINLOCK_FLAGS uiLockingFlags;

	OSSpinLockAcquire(psPMR->hBitmapLock, uiLockingFlags);
	BITMASK_UNSET(psPMR->uiInternalFlags, uiValue);
	OSSpinLockRelease(psPMR->hBitmapLock, uiLockingFlags);
}

static INLINE IMG_BOOL _IntFlagIsSet(const PMR *psPMR, const IMG_UINT32 uiValue)
{
	OS_SPINLOCK_FLAGS uiLockingFlags;
	IMG_BOOL bIsSet;

	OSSpinLockAcquire(psPMR->hBitmapLock, uiLockingFlags);
	bIsSet = BITMASK_HAS(psPMR->uiInternalFlags, uiValue);
	OSSpinLockRelease(psPMR->hBitmapLock, uiLockingFlags);

	return bIsSet;
}

static INLINE IMG_BOOL _IntFlagSetIfNotSet(PMR *psPMR, const IMG_UINT32 uiValue)
{
	OS_SPINLOCK_FLAGS uiLockingFlags;
	IMG_BOOL bIsSet;

	OSSpinLockAcquire(psPMR->hBitmapLock, uiLockingFlags);
	bIsSet = BITMASK_HAS(psPMR->uiInternalFlags, uiValue);
	if (!bIsSet)
	{
		BITMASK_SET(psPMR->uiInternalFlags, uiValue);
	}
	OSSpinLockRelease(psPMR->hBitmapLock, uiLockingFlags);

	return !bIsSet;
}

static INLINE void
_FactoryLock(const PMR_IMPL_FUNCTAB *psFuncTable)
{
	if (psFuncTable->pfnGetPMRFactoryLock != NULL)
	{
		psFuncTable->pfnGetPMRFactoryLock();
	}
}

static INLINE void
_FactoryUnlock(const PMR_IMPL_FUNCTAB *psFuncTable)
{
	if (psFuncTable->pfnReleasePMRFactoryLock != NULL)
	{
		psFuncTable->pfnReleasePMRFactoryLock();
	}
}

#if defined(SUPPORT_PMR_DEFERRED_FREE)
/* Protects:
 * - `psDevNode->sPMRZombieList`
 * - `uiPMRZombieCount`
 * - `uiPMRZombieCountInCleanup`
 */

void PMR_SetZombieIsPMREmptyFlag(PMR *psPMR)
{
	(void) _IntFlagSetIfNotSet(psPMR, PMR_FLAG_INTERNAL_IS_ZOMBIE_PMR_EMPTY);
}

static INLINE void
_ZombieListLock(PPVRSRV_DEVICE_NODE psDevNode)
{
	OSLockAcquire(psDevNode->hPMRZombieListLock);
}

static INLINE void
_ZombieListUnlock(PPVRSRV_DEVICE_NODE psDevNode)
{
	OSLockRelease(psDevNode->hPMRZombieListLock);
}

static IMG_BOOL _IsDeviceOnAndOperating(PVRSRV_DEVICE_NODE *psDevNode)
{
	PVRSRV_ERROR eError;
	PVRSRV_DEV_POWER_STATE ePowerState;

	eError = PVRSRVGetDevicePowerState(psDevNode, &ePowerState);
	if (eError != PVRSRV_OK)
	{
		/* Treat unknown power state as ON. */
		ePowerState = PVRSRV_DEV_POWER_STATE_ON;
	}

	/* The device does not accept zombies when its power is OFF as
	 * the cache invalidation comes as a given. */
	return !( ePowerState == PVRSRV_DEV_POWER_STATE_OFF
	       || psDevNode->eDevState == PVRSRV_DEVICE_STATE_PCI_ERROR);
}
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) || defined(PVRSRV_ENABLE_XD_MEM)
static IMG_UINT64
_DeviceImportBitmapGet(const PMR *psPMR)
{
	OS_SPINLOCK_FLAGS uiLockingFlags;
	IMG_UINT64 uiDevImportBitmap;

	OSSpinLockAcquire(psPMR->hBitmapLock, uiLockingFlags);
	uiDevImportBitmap = psPMR->uiDevImportBitmap;
	OSSpinLockRelease(psPMR->hBitmapLock, uiLockingFlags);

	return uiDevImportBitmap;
}
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) || defined(PVRSRV_ENABLE_XD_MEM) */

#if defined(PVRSRV_ENABLE_XD_MEM) && !defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
static void
_DeviceImportBitmapSet(PMR *psPMR, const PPVRSRV_DEVICE_NODE psDevNode)
{
	OS_SPINLOCK_FLAGS uiLockingFlags;

	OSSpinLockAcquire(psPMR->hBitmapLock, uiLockingFlags);
	BITMASK_SET(psPMR->uiDevImportBitmap, IMG_UINT64_C(1) << psDevNode->sDevId.ui32InternalID);
	OSSpinLockRelease(psPMR->hBitmapLock, uiLockingFlags);
}
#endif /* defined(PVRSRV_ENABLE_XD_MEM) && !defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
static void
_DeviceImportBitmapClr(PMR *psPMR, const PPVRSRV_DEVICE_NODE psDevNode)
{
	OS_SPINLOCK_FLAGS uiLockingFlags;

	OSSpinLockAcquire(psPMR->hBitmapLock, uiLockingFlags);
	BITMASK_UNSET(psPMR->uiDevImportBitmap, IMG_UINT64_C(1) << psDevNode->sDevId.ui32InternalID);
	OSSpinLockRelease(psPMR->hBitmapLock, uiLockingFlags);
}

static IMG_BOOL
_DeviceImportBitmapIsSet(const PMR *psPMR, const PPVRSRV_DEVICE_NODE psDevNode)
{
	OS_SPINLOCK_FLAGS uiLockingFlags;
	IMG_BOOL bIsSet;

	OSSpinLockAcquire(psPMR->hBitmapLock, uiLockingFlags);
	bIsSet = BITMASK_HAS(psPMR->uiDevImportBitmap,
	                     IMG_UINT64_C(1) << psDevNode->sDevId.ui32InternalID);
	OSSpinLockRelease(psPMR->hBitmapLock, uiLockingFlags);

	return bIsSet;
}

static IMG_BOOL
/* Atomically, return if the `psDevNode` is set in the bitmap and then set it. */
_DeviceImportBitmapFetchAndSet(PMR *psPMR, const PPVRSRV_DEVICE_NODE psDevNode)
{
	OS_SPINLOCK_FLAGS uiLockingFlags;
	IMG_BOOL bIsSet;

	OSSpinLockAcquire(psPMR->hBitmapLock, uiLockingFlags);
	bIsSet = BITMASK_HAS(psPMR->uiDevImportBitmap,
	                     IMG_UINT64_C(1) << psDevNode->sDevId.ui32InternalID);
	BITMASK_SET(psPMR->uiDevImportBitmap,
	            IMG_UINT64_C(1) << psDevNode->sDevId.ui32InternalID);
	OSSpinLockRelease(psPMR->hBitmapLock, uiLockingFlags);

	return bIsSet;
}

#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */


#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
static PVRSRV_ERROR
_DeviceImportRegister(PMR *psPMR, PPVRSRV_DEVICE_NODE psDevNode)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PMR_DEVICE_IMPORT *psImport;

	PVR_ASSERT(psPMR);
	PVR_ASSERT(psDevNode);
	PVR_ASSERT(PMR_DeviceNode(psPMR) != psDevNode);

	/* Explicitly reject:
	 * - PVRSRV_MEMALLOCFLAG_DEFER_PHYS_ALLOC
	 * - !PMR_FLAG_INTERNAL_NO_LAYOUT_CHANGE
	 * as XD PMRs don't have support for
	 * SUPPORT_PMR_PAGES_DEFERRED_FREE. */
	if (PVRSRV_CHECK_ON_DEMAND(psPMR->uiFlags) ||
	    !_IntFlagIsSet(psPMR, PMR_FLAG_INTERNAL_NO_LAYOUT_CHANGE))
	{
		eError = PVRSRV_ERROR_PMR_NOT_PERMITTED;
		PVR_LOG_ERROR(eError,
		              "PVRSRV_CHECK_ON_DEMAND || !PMR_FLAG_INTERNAL_NO_LAYOUT_CHANGE");
		return eError;
	}

	/* Check if the device is already imported */
	if (_DeviceImportBitmapFetchAndSet(psPMR, psDevNode))
	{
		return PVRSRV_OK;
	}

	psImport = OSAllocMem(sizeof(*psImport));
	PVR_LOG_RETURN_IF_NOMEM(psImport, "PMR_DEVICE_IMPORT");

	psImport->psParent = psPMR;
	psImport->psDevNode = psDevNode;
	dllist_init(&psImport->sHeader.sZombieNode);
	psImport->sHeader.eZombieType = PMR_ZOMBIE_TYPE_DEVICE_IMPORT;

	PMRLockPMR(psPMR);
	dllist_add_to_tail(&psPMR->sXDeviceImports, &psImport->sNext);
	PMRUnlockPMR(psPMR);

	return eError;
}

static void
_DeviceImportFreeImportZombie(PMR_DEVICE_IMPORT *psImport)
{
	PVR_ASSERT(_DeviceImportBitmapIsSet(psImport->psParent, psImport->psDevNode));
	_DeviceImportBitmapClr(psImport->psParent, psImport->psDevNode);

	PMRLockPMR(psImport->psParent);
	dllist_remove_node(&psImport->sNext);
	PMRUnlockPMR(psImport->psParent);

	OSFreeMem(psImport);
}

static IMG_BOOL
_DeviceImportEnqueueZombie(PMR_DEVICE_IMPORT *psImport)
{
	PVR_ASSERT(_DeviceImportBitmapIsSet(psImport->psParent, psImport->psDevNode));
	PMRLockHeldAssert(psImport->psParent);


	if (!_IsDeviceOnAndOperating(psImport->psDevNode))
	{
		_DeviceImportFreeImportZombie(psImport);
		return IMG_FALSE;
	}

	_ZombieListLock(psImport->psDevNode);
	dllist_add_to_tail(&psImport->psDevNode->sPMRZombieList,
	                   &psImport->sHeader.sZombieNode);
	psImport->psDevNode->uiPMRZombieCount++;
	_ZombieListUnlock(psImport->psDevNode);

	return IMG_TRUE;
}

static IMG_BOOL
_DeviceImportsEnqueueZombies(PMR *psPMR)
{
	PDLLIST_NODE psNode, psNext;
	PMR_DEVICE_IMPORT *psImport;
	IMG_BOOL bEnqueued = IMG_FALSE;

	PMRLockPMR(psPMR);

	dllist_foreach_node(&psPMR->sXDeviceImports, psNode, psNext)
	{
		psImport = IMG_CONTAINER_OF(psNode, PMR_DEVICE_IMPORT, sNext);
		bEnqueued |= _DeviceImportEnqueueZombie(psImport);
	}

	PMRUnlockPMR(psPMR);

	return bEnqueued;
}

static void
_DeviceImportsUnregisterAll(PMR *psPMR)
{
	OS_SPINLOCK_FLAGS uiLockingFlags;
	PDLLIST_NODE psNode, psNext;

	PMRLockPMR(psPMR);
	dllist_foreach_node(&psPMR->sXDeviceImports, psNode, psNext)
	{
		PMR_DEVICE_IMPORT *psImport = IMG_CONTAINER_OF(psNode, PMR_DEVICE_IMPORT, sNext);
		PVR_ASSERT(_DeviceImportBitmapIsSet(psPMR, psImport->psDevNode));
		OSFreeMem(psImport);
	}
	dllist_init(&psPMR->sXDeviceImports);

	OSSpinLockAcquire(psPMR->hBitmapLock, uiLockingFlags);
	psPMR->uiDevImportBitmap = 0;
	OSSpinLockRelease(psPMR->hBitmapLock, uiLockingFlags);
	PMRUnlockPMR(psPMR);
}
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

static void
_PMRDestroy(PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	PMR_ENV_FINALIZE(psPMR, sEnvData);

	if (psPMR->psFuncTab->pfnFinalize != NULL)
	{
		psPMR->psFuncTab->pfnFinalize(psPMR->pvFlavourData);
	}

#if defined(PDUMP)
	/* if allocation is done on the host node don't include it in the PDUMP */
	if (!_IsHostDevicePMR(psPMR))
	{
		PDumpPMRFreePMR(psPMR,
		                psPMR->uiLogicalSize,
		                IMG_PAGE2BYTES64(psPMR->uiLog2ContiguityGuarantee),
		                psPMR->uiLog2ContiguityGuarantee,
		                psPMR->hPDumpAllocHandle);
	}
#endif

#if defined(PVRSRV_ENABLE_LINUX_MMAP_STATS)
	/* This PMR is about to be destroyed, update its mmap stats record (if present)
	 * to avoid dangling pointer. Additionally, this is required because mmap stats
	 * are identified by PMRs and a new PMR down the line "might" get the same address
	 * as the one we're about to free and we'd like 2 different entries in mmaps
	 * stats for such cases */
	MMapStatsRemovePMR(psPMR);
#endif

#ifdef PVRSRV_NEED_PVR_ASSERT
	/* If not backed on demand, iLockCount should be 1 otherwise it should be 0 */
	PVR_ASSERT(OSAtomicRead(&psPMR->iLockCount) == (PVRSRV_CHECK_ON_DEMAND(psPMR->uiFlags) ? 0 : 1));
#endif

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
	/* Delete RI entry */
	if (psPMR->hRIHandle)
	{
		PVRSRV_ERROR eError = RIDeletePMREntryKM(psPMR->hRIHandle);
		PVR_LOG_IF_ERROR(eError, "RIDeletePMREntryKM");
		/* continue destroying the PMR */
	}
#endif /* if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO) */

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
	_DeviceImportsUnregisterAll(psPMR);
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

	/* Decrement live PMR count. Probably only of interest for debugging */
	PVR_ASSERT(OSAtomicRead(&psPMR->psContext->uiNumLivePMRs) > 0);
	OSAtomicDecrement(&psPMR->psContext->uiNumLivePMRs);

	PVR_DPF((PVR_DBG_MESSAGE, "%s: 0x%p, key:0x%016" IMG_UINT64_FMTSPECX ", numLive:%d",
			__func__, psPMR, psPMR->uiKey, OSAtomicRead(&psPMR->psContext->uiNumLivePMRs)));

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	/* Detect programming errors here, either a reference on the PMR
	 * has not been taken by a user or the mapping records added
	 * to the list head have not been destroyed correctly.
	 */
	PVR_ASSERT(dllist_is_empty(&psPMR->sGpuMappingListHead));
#endif

	OSSpinLockDestroy(psPMR->hRefCountLock);
	OSSpinLockDestroy(psPMR->hBitmapLock);
	OSLockDestroy(psPMR->hLock);
	OSFreeMem(psPMR);
}

#if defined(SUPPORT_PMR_DEFERRED_FREE)
static INLINE PMR_ZOMBIE_TYPE
PMR_GetZombieTypeFromNode(const DLLIST_NODE *psNode)
{
	PMR_HEADER *psPMRHeader = IMG_CONTAINER_OF(psNode, PMR_HEADER, sZombieNode);
	PVR_ASSERT(psPMRHeader != NULL);
	return psPMRHeader->eZombieType;
}

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
static INLINE PMR_ZOMBIE_PAGES*
PMR_GetZombiePagesFromNode(const DLLIST_NODE *psNode)
{
	PMR_HEADER *psPMRHeader = IMG_CONTAINER_OF(psNode, PMR_HEADER, sZombieNode);
	PVR_ASSERT(psPMRHeader != NULL);
	return IMG_CONTAINER_OF(psPMRHeader, PMR_ZOMBIE_PAGES, sHeader);
}
#endif /* defined(SUPPORT_PMR_PAGES_DEFERRED_FREE) */

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
static INLINE PMR_DEVICE_IMPORT*
PMR_GetDeviceImportFromNode(const DLLIST_NODE *psNode)
{
	PMR_HEADER *psPMRHeader = IMG_CONTAINER_OF(psNode, PMR_HEADER, sZombieNode);
	PVR_ASSERT(psPMRHeader != NULL);
	return IMG_CONTAINER_OF(psPMRHeader, PMR_DEVICE_IMPORT, sHeader);
}
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

static INLINE PMR*
PMR_GetPMRFromNode(const DLLIST_NODE *psNode)
{
	PMR_HEADER *psPMRHeader = IMG_CONTAINER_OF(psNode, PMR_HEADER, sZombieNode);
	PVR_ASSERT(psPMRHeader != NULL);
	return IMG_CONTAINER_OF(psPMRHeader, PMR, sHeader);
}
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

static PVRSRV_ERROR
_UnrefAndMaybeDestroy(PMR *psPMR, IMG_UINT32 uiRefCount)
{
	const PMR_IMPL_FUNCTAB *psFuncTable;
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	PVRSRV_DEVICE_NODE *psDevNode;
	IMG_BOOL bQueuedDeviceImports = IMG_FALSE;
#endif
	PVRSRV_ERROR eError;

	PVR_ASSERT(psPMR != NULL);

	psFuncTable = psPMR->psFuncTab;

	_FactoryLock(psFuncTable);

	eError = _Unref(psPMR, uiRefCount, &uiRefCount);
	PVR_LOG_GOTO_IF_ERROR(eError, "_Unref", ErrFactoryUnlock);

	if (uiRefCount > 0)
	{
		/* PMR is still referenced so just return */
		_FactoryUnlock(psFuncTable);
		return PVRSRV_OK;
	}

#if !defined(SUPPORT_PMR_DEFERRED_FREE)
	/* Don't defer PMR destruction in NoHW and PDUMP drivers. */
	_PMRDestroy(psPMR);
#else /* !defined(SUPPORT_PMR_DEFERRED_FREE) */
	psDevNode = PhysHeapDeviceNode(psPMR->psPhysHeap);

	/* PMRs that are not marked for deferred free can be freed right away.
	 * Those are the PMRs that have not been mapped to the device.
	 * All PMRs that have been mapped to the device need to go through
	 * the defer free path unless the power is OFF for the PMR's device
	 * and for all of the device imports. If power is OFF
	 * the cache invalidation comes as a given. */
	if (!_IntFlagIsSet(psPMR, PMR_FLAG_INTERNAL_DEFER_FREE))
	{
		_PMRDestroy(psPMR);
		goto exit_;
	}

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
	bQueuedDeviceImports = _DeviceImportsEnqueueZombies(psPMR);
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

	if (!bQueuedDeviceImports
	    && !_IsDeviceOnAndOperating(psDevNode)
#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	    && !PVRSRV_CHECK_OS_LINUX_MOVABLE(psPMR->uiFlags)
#endif
	    )
	{
		_PMRDestroy(psPMR);
	}
	else if (_IntFlagIsSet(psPMR, PMR_FLAG_INTERNAL_IS_ZOMBIE_PMR_EMPTY))
	{
		_PMRDestroy(psPMR);
	}
	else
	{
		/* Defer freeing the PMR until the Firmware invalidates the caches. */
		_ZombieListLock(psDevNode);

		_IntFlagSet(psPMR, PMR_FLAG_INTERNAL_IS_ZOMBIE_PMR);

		dllist_add_to_tail(&psDevNode->sPMRZombieList, &psPMR->sHeader.sZombieNode);
		psDevNode->uiPMRZombieCount++;

		/* PMR pages are accounted by the driver/process stats. Those stats
		 * are available on page level hence they need to be adjusted by
		 * the factories. This is done by the pfnZombify callback.
		 * Operation needs to be done while holding hPMRZombieListLock
		 * to prevent CleanupThread from freeing pages while memory stats
		 * accounting is ongoing. */
		if (psPMR->psFuncTab->pfnZombify != NULL)
		{
			eError = psPMR->psFuncTab->pfnZombify(psPMR->pvFlavourData, psPMR);
			PVR_LOG_IF_ERROR(eError, "pfnZombify");
		}

		_ZombieListUnlock(psDevNode);
	}
exit_:
#endif /* !defined(SUPPORT_PMR_DEFERRED_FREE) */

	_FactoryUnlock(psFuncTable);

	return PVRSRV_OK;

ErrFactoryUnlock:
	_FactoryUnlock(psFuncTable);

	return eError;
}

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
/*
   If possible, free zombie pages immediately (and free the wrapper).
   If not possible, wrap and queue the zombie pages object for deferred freeing.
   If there are no zombie pages free the provided wrapper (if allocated) */
static PVRSRV_ERROR
_ProcessZombiePages(void* pvFactoryZombiePages, PMR_ZOMBIE_PAGES* psPMRZombiePagesWrapper, PMR* psPMR)
{
	PVRSRV_ERROR eError;

	if (pvFactoryZombiePages != NULL)
	{
		PVRSRV_DEVICE_NODE *psDevNode = PhysHeapDeviceNode(psPMR->psPhysHeap);
		if (psPMRZombiePagesWrapper == NULL)
		{
			PVR_LOG_GOTO_WITH_ERROR("Error when processing zombies", eError,
			    PVRSRV_ERROR_INVALID_PARAMS, e0);
		}

		if (!_IsDeviceOnAndOperating(psDevNode))
		{
			/* Free psPMRZombiePages as these won't be used*/
			OSFreeMem(psPMRZombiePagesWrapper);

			eError = psPMR->psFuncTab->pfnFreeZombiePages(pvFactoryZombiePages);
			PVR_LOG_GOTO_IF_ERROR(eError, "Error when trying to free zombies immediately.", e0);
		}
		else
		{
			psPMRZombiePagesWrapper->sHeader.eZombieType = PMR_ZOMBIE_TYPE_PAGES;
			psPMRZombiePagesWrapper->pfnFactoryFreeZombies = psPMR->psFuncTab->pfnFreeZombiePages;
			psPMRZombiePagesWrapper->pvFactoryPages = pvFactoryZombiePages;
			psPMRZombiePagesWrapper->ePagesFlavour = psPMR->eFlavour;

			OSLockAcquire(psDevNode->hPMRZombieListLock);
			dllist_add_to_tail(&psDevNode->sPMRZombieList, &psPMRZombiePagesWrapper->sHeader.sZombieNode);
			psDevNode->uiPMRZombieCount++;
			OSLockRelease(psDevNode->hPMRZombieListLock);
		}
	}
	else if (psPMRZombiePagesWrapper != NULL)
	{
		/* Free psPMRZombiePages as change sparse has not produced any zombie pages */
		OSFreeMem(psPMRZombiePagesWrapper);
	}

	return PVRSRV_OK;
e0:
	return eError;
}
#endif

#if defined(SUPPORT_PMR_DEFERRED_FREE)
typedef struct _PMR_ZOMBIE_CLEANUP_ITEM_
{
	PVRSRV_CLEANUP_THREAD_WORK sCleanupThreadFn;
	DLLIST_NODE sZombieList;
	PPVRSRV_DEVICE_NODE psDevNode;
	PVRSRV_CLIENT_SYNC_PRIM *psSync;
	IMG_UINT32 uiRequiredSyncValue;
	IMG_UINT32 uiRequiredPowerOffCounter;
} PMR_ZOMBIE_CLEANUP_ITEM;

static INLINE IMG_BOOL
_CanNotFreeZombies(const PMR_ZOMBIE_CLEANUP_ITEM *psCleanupItem)
{
	const PVRSRV_DEVICE_NODE *psDevNode = psCleanupItem->psDevNode;

	/* For a zombie PMR to be eligible to be freed either the GPU MMU caches
	 * need to be flushed (the Firmware updates the sync) or the GPU power needs
	 * to be off. */
	return !PVRSRVHasCounter32Advanced(OSReadDeviceMem32(psCleanupItem->psSync->pui32LinAddr),
	                                   psCleanupItem->uiRequiredSyncValue) &&
	       !PVRSRVHasCounter32Advanced(psDevNode->uiPowerOffCounter,
	                                   psCleanupItem->uiRequiredPowerOffCounter);
}

static PVRSRV_ERROR _PmrZombieCleanup(void *pvData)
{
	PMR_ZOMBIE_CLEANUP_ITEM *psCleanupItem = pvData;
	DLLIST_NODE *psNode;
	DLLIST_NODE sRetryHead;
	IMG_UINT32 uiRetryCount = 0;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (_CanNotFreeZombies(psCleanupItem))
	{
		return PVRSRV_ERROR_RETRY;
	}
	dllist_init(&sRetryHead);

	do
	{
		psNode = dllist_get_next_node(&psCleanupItem->sZombieList);
		if (psNode == NULL)
		{
			continue;
		}

		dllist_remove_node(psNode);
		_ZombieListLock(psCleanupItem->psDevNode);
		psCleanupItem->psDevNode->uiPMRZombieCountInCleanup--;
		_ZombieListUnlock(psCleanupItem->psDevNode);

		switch (PMR_GetZombieTypeFromNode(psNode))
		{
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
			case PMR_ZOMBIE_TYPE_PAGES:
			{
				PMR_ZOMBIE_PAGES* psZombiePages = PMR_GetZombiePagesFromNode(psNode);
				eError = psZombiePages->pfnFactoryFreeZombies(psZombiePages->pvFactoryPages);
				if (eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR, "Cannot free zombie pages! Skipping object %p", psZombiePages));
					dllist_add_to_tail(&sRetryHead, psNode);
					uiRetryCount++;
				}
				else
				{
					OSFreeMem(psZombiePages);
				}
				break;
			}
#endif

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
			case PMR_ZOMBIE_TYPE_DEVICE_IMPORT:
			{
				PMR_DEVICE_IMPORT *psImport = PMR_GetDeviceImportFromNode(psNode);
				_DeviceImportFreeImportZombie(psImport);
				break;
			}
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

			case PMR_ZOMBIE_TYPE_PMR:
			{
				PMR* psPMR = PMR_GetPMRFromNode(psNode);
				const PMR_IMPL_FUNCTAB *psFuncTable = psPMR->psFuncTab;

				_FactoryLock(psFuncTable);

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
				/* The PMR cannot be freed if other devices are
				 * still waiting for the cache flush. */
				if (_DeviceImportBitmapGet(psPMR) != 0)
				{
					/* Request it to be retried and continue
					 * to the next zombie item. */
					dllist_add_to_tail(&sRetryHead, psNode);
					uiRetryCount++;
					_FactoryUnlock(psFuncTable);
					continue;
				}
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

				/* Unlock here to avoid locking dependency with the power lock.
				 * It's okay to do it here since the factory lock is the one
				 * that needs to be held during PMR destruction. */
				_PMRDestroy(psPMR);
				_FactoryUnlock(psFuncTable);
				break;
			}
		}
	} while (psNode != NULL);

	if (uiRetryCount)
	{
		eError = PVRSRV_ERROR_RETRY;
		/* Add the retry items back to this cleanup item for when the
		 * cleanup item is retried. Oldest items will reside at the head of
		 * the list. The cleanup item will be placed at the back of the cleanup
		 * queue to process other dependencies first. */
		dllist_insert_list_at_head(&psCleanupItem->sZombieList, &sRetryHead);
		_ZombieListLock(psCleanupItem->psDevNode);
		psCleanupItem->psDevNode->uiPMRZombieCountInCleanup += uiRetryCount;
		_ZombieListUnlock(psCleanupItem->psDevNode);
	}
	else
	{
		OSFreeMem(psCleanupItem);
	}

	return eError;
}

IMG_BOOL PMRQueueZombiesForCleanup(PPVRSRV_DEVICE_NODE psDevNode)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PMR_ZOMBIE_CLEANUP_ITEM *psCleanupItem;

	/* Don't defer the freeing if we are currently unloading the driver
	 * or if the sync has been destroyed */
	if (psPVRSRVData->bUnload || psDevNode->psMMUCacheSyncPrim == NULL)
	{
		return IMG_FALSE;
	}

	_ZombieListLock(psDevNode);

	if (dllist_is_empty(&psDevNode->sPMRZombieList))
	{
		_ZombieListUnlock(psDevNode);
		return IMG_FALSE;
	}

	psCleanupItem = OSAllocMem(sizeof(*psCleanupItem));
	if (psCleanupItem == NULL)
	{
		_ZombieListUnlock(psDevNode);
		return IMG_FALSE;
	}

	psCleanupItem->sCleanupThreadFn.pfnFree = _PmrZombieCleanup;
	psCleanupItem->sCleanupThreadFn.pvData = psCleanupItem;
	psCleanupItem->sCleanupThreadFn.bDependsOnHW = IMG_TRUE;
	psCleanupItem->sCleanupThreadFn.eCleanupType = PVRSRV_CLEANUP_TYPE_PMR;
	CLEANUP_THREAD_SET_RETRY_TIMEOUT(&psCleanupItem->sCleanupThreadFn,
	                                 CLEANUP_THREAD_RETRY_TIMEOUT_MS_DEFAULT);

	psCleanupItem->psDevNode = psDevNode;
	psCleanupItem->psSync = psDevNode->psMMUCacheSyncPrim;
	psCleanupItem->uiRequiredSyncValue = psDevNode->ui32NextMMUInvalidateUpdate;
	psCleanupItem->uiRequiredPowerOffCounter = psDevNode->uiPowerOffCounterNext;

	/* This moves the zombie list to the cleanup item. */
	dllist_replace_head(&psDevNode->sPMRZombieList, &psCleanupItem->sZombieList);
	psDevNode->uiPMRZombieCountInCleanup += psDevNode->uiPMRZombieCount;
	psDevNode->uiPMRZombieCount = 0;

	OSLockRelease(psDevNode->hPMRZombieListLock);

	PVRSRVCleanupThreadAddWork(psDevNode, &psCleanupItem->sCleanupThreadFn);

	return IMG_TRUE;
}

void
PMRMarkForDeferFree(PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	if (PVRSRV_CHECK_ON_DEMAND(psPMR->uiFlags))
	{
		/* If PMR pages are allocated on demand the freeing is handled
		 * by `SUPPORT_PMR_PAGES_DEFERRED_FREE` path in
		 * `PMRUnlockPhysAddressesNested()`. */
		return;
	}

	_IntFlagSet(psPMR, PMR_FLAG_INTERNAL_DEFER_FREE);
}
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

static INLINE IMG_BOOL _PMRIsSparse(const PMR *psPMR)
{
	return _IntFlagIsSet(psPMR, PMR_FLAG_INTERNAL_SPARSE_ALLOC);
}

PVRSRV_ERROR
PMRCreatePMR(PHYS_HEAP *psPhysHeap,
             PMR_SIZE_T uiLogicalSize,
             IMG_UINT32 ui32NumPhysChunks,
             IMG_UINT32 ui32NumLogicalChunks,
             IMG_UINT32 *pui32MappingTable,
             PMR_LOG2ALIGN_T uiLog2ContiguityGuarantee,
             PMR_FLAGS_T uiFlags,
             const IMG_CHAR *pszAnnotation,
             const PMR_IMPL_FUNCTAB *psFuncTab,
             PMR_IMPL_PRIVDATA pvPrivData,
             PMR_IMPL_TYPE eType,
             PMR **ppsPMRPtr,
             IMG_UINT32 ui32PDumpFlags)
{
	PMR *psPMR = NULL;
	PVRSRV_ERROR eError;

	PVR_LOG_RETURN_IF_INVALID_PARAM(pszAnnotation != NULL, "pszAnnotation");

	eError = _PMRCreate(uiLogicalSize,
	                    ui32NumPhysChunks,
	                    ui32NumLogicalChunks,
	                    pui32MappingTable,
	                    uiLog2ContiguityGuarantee,
	                    uiFlags,
	                    &psPMR);
	PVR_GOTO_IF_ERROR(eError, e0);

	psPMR->psPhysHeap = psPhysHeap;
	psPMR->psFuncTab = psFuncTab;
	psPMR->pszPDumpDefaultMemspaceName = PhysHeapPDumpMemspaceName(psPhysHeap);
	psPMR->pvFlavourData = pvPrivData;
	psPMR->eFlavour = eType;
	psPMR->uiRefCount = 1;

	OSStringSafeCopy(psPMR->szAnnotation, pszAnnotation, DEVMEM_ANNOTATION_MAX_LEN);

#if defined(PDUMP)
	/* if allocation was done on the host node don't include it in the PDUMP */
	if (!_IsHostDevicePMR(psPMR))
	{
		PMR_FLAGS_T uiFlags = psPMR->uiFlags;
		IMG_BOOL bInitialise = IMG_FALSE;
		IMG_UINT8 ui8InitValue = 0;

		if (PVRSRV_CHECK_ZERO_ON_ALLOC(uiFlags))
		{
			bInitialise = IMG_TRUE;
		}
		else if (PVRSRV_CHECK_POISON_ON_ALLOC(uiFlags))
		{
			ui8InitValue = (IMG_UINT8)PVRSRV_POISON_ON_ALLOC_VALUE;
			bInitialise = IMG_TRUE;
		}

		PDumpPMRMallocPMR(psPMR,
		                  uiLogicalSize,
		                  IMG_PAGE2BYTES64(psPMR->uiLog2ContiguityGuarantee),
		                  ui32NumPhysChunks,
		                  ui32NumLogicalChunks,
		                  pui32MappingTable,
		                  uiLog2ContiguityGuarantee,
		                  bInitialise,
		                  ui8InitValue,
		                  &psPMR->hPDumpAllocHandle,
		                  ui32PDumpFlags);
	}
#else
	PVR_UNREFERENCED_PARAMETER(ui32PDumpFlags);
#endif

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	psPMR->eState = PMR_STATE_ACTIVE;
#endif

	*ppsPMRPtr = psPMR;

	return PVRSRV_OK;

	/* Error exit paths follow */
e0:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

PVRSRV_ERROR
PMRLockPhysAddresses(PMR *psPMR)
{
	return PMRLockPhysAddressesNested(psPMR, 1, 0);
}

PVRSRV_ERROR
PMRLockPhysAddressesN(PMR *psPMR, IMG_UINT32 uiLockCount)
{
	return PMRLockPhysAddressesNested(psPMR, uiLockCount, 0);
}

PVRSRV_ERROR PMRLockPhysAddressesNested(PMR *psPMR,
                                        IMG_UINT32 uiLockCount,
                                        IMG_UINT32 ui32NestingLevel)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 uiCallbackValue;

	PVR_ASSERT(psPMR != NULL);

	/* Note: taking this lock is not required to protect the PMR reference
	 * count, because the PMR reference count is atomic. Rather, taking
	 * the lock here guarantees that no caller will exit this function
	 * without the underlying physical addresses being locked.
	 */
	OSLockAcquireNested(psPMR->hLock, ui32NestingLevel);
	/* We also count the locks as references, so that the PMR is not freed
	 * while someone is using a physical address.
	 * "lock" here simply means incrementing the refcount. It means the
	 * refcount is multipurpose, but that's okay. We only have to promise
	 * that physical addresses are valid after this point, and remain valid
	 * until the corresponding PMRUnlockPhysAddressesOSMem()
	 */
	eError = _Ref(psPMR, uiLockCount);
	if (eError != PVRSRV_OK)
	{
		OSLockRelease(psPMR->hLock);
		return eError;
	}

	/* Also count locks separately from other types of references, to
	 * allow for debug assertions
	 */

	/* Only call callback if lock count transitions from 0 to 1 (or 1 to 2 if not
	 * backed on demand) */
	uiCallbackValue = uiLockCount + (PVRSRV_CHECK_ON_DEMAND(psPMR->uiFlags) ? 0 : 1);

	if (OSAtomicAdd(&psPMR->iLockCount, uiLockCount) == uiCallbackValue)
	{
		if (psPMR->psFuncTab->pfnLockPhysAddresses != NULL)
		{
			/* must always have lock and unlock in pairs! */
			PVR_ASSERT(psPMR->psFuncTab->pfnUnlockPhysAddresses != NULL);

			eError = psPMR->psFuncTab->pfnLockPhysAddresses(psPMR->pvFlavourData);

			PVR_GOTO_IF_ERROR(eError, e1);
		}
	}
	OSLockRelease(psPMR->hLock);

	return PVRSRV_OK;

e1:
	OSAtomicSubtract(&psPMR->iLockCount, uiLockCount);
	PVR_ASSERT(_GetRef(psPMR) != 0);
	OSLockRelease(psPMR->hLock);
	PVR_ASSERT(eError != PVRSRV_OK);
	/* The ref was taken after the lock above (as we are also
	 * using it to count the number of times the phys addrs have
	 * been locked), but it is safer to drop the reference last
	 * here, just in case other references were already dropped.
	 */
	(void) _Unref(psPMR, uiLockCount, NULL);
	return eError;
}

PVRSRV_ERROR
PMRUnlockPhysAddresses(PMR *psPMR)
{
	return PMRUnlockPhysAddressesNested(psPMR, 1, 2);
}

PVRSRV_ERROR
PMRUnlockPhysAddressesN(PMR *psPMR, IMG_UINT32 uiLockCount)
{
	return PMRUnlockPhysAddressesNested(psPMR, uiLockCount, 2);
}

PVRSRV_ERROR
PMRUnlockPhysAddressesNested(PMR *psPMR,
                             IMG_UINT32 uiLockCount,
                             IMG_UINT32 ui32NestingLevel)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	PMR_IMPL_ZOMBIEPAGES pvZombiePages = NULL;
	PMR_ZOMBIE_PAGES* psPMRZombiePages = NULL;
#endif

	PVR_ASSERT(psPMR != NULL);

	/* Acquiring the lock here, as well as during the Lock operation ensures
	 * the lock count hitting zero and the unlocking of the phys addresses is
	 * an atomic operation
	 */
	OSLockAcquireNested(psPMR->hLock, ui32NestingLevel);

	PVR_ASSERT(OSAtomicRead(&psPMR->iLockCount) >= uiLockCount);

	if (OSAtomicSubtract(&psPMR->iLockCount, uiLockCount) == (PVRSRV_CHECK_ON_DEMAND(psPMR->uiFlags) ? 0 : 1))
	{
		if (psPMR->psFuncTab->pfnUnlockPhysAddresses != NULL)
		{
			PVR_ASSERT(psPMR->psFuncTab->pfnLockPhysAddresses != NULL);

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
			eError = psPMR->psFuncTab->pfnUnlockPhysAddresses(psPMR->pvFlavourData,
			                                                  &pvZombiePages);
#else
			eError = psPMR->psFuncTab->pfnUnlockPhysAddresses(psPMR->pvFlavourData);
#endif
			PVR_LOG_IF_ERROR(eError, "pfnUnlockPhysAddresses");
		}
	}

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	if (pvZombiePages != NULL)
	{
		psPMRZombiePages = OSAllocZMem(sizeof(PMR_ZOMBIE_PAGES));
		PVR_GOTO_IF_NOMEM(psPMRZombiePages, eError, ErrRelockPhysAddresses);
	}
#endif

	OSLockRelease(psPMR->hLock);

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	eError = _ProcessZombiePages(pvZombiePages, psPMRZombiePages, psPMR);
	PVR_GOTO_IF_ERROR(eError, ErrReturn);
#endif

	/* We also count the locks as references, so that the PMR is not
	 * freed while someone is using a physical address.
	 */
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	(void) _UnrefAndMaybeDestroy(psPMR, uiLockCount);

	return eError;
#else
	return _UnrefAndMaybeDestroy(psPMR, uiLockCount);
#endif

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
ErrRelockPhysAddresses:
	{
		IMG_UINT32 uiCallbackValue =
		    uiLockCount + (PVRSRV_CHECK_ON_DEMAND(psPMR->uiFlags) ? 0 : 1);

		if (OSAtomicAdd(&psPMR->iLockCount, uiLockCount) == uiCallbackValue)
		{
			if (psPMR->psFuncTab->pfnLockPhysAddresses != NULL)
			{
				eError = psPMR->psFuncTab->pfnLockPhysAddresses(psPMR->pvFlavourData);
				PVR_LOG_IF_ERROR(eError, "pfnLockPhysAddresses");
			}
		}
	}

	OSLockRelease(psPMR->hLock);

ErrReturn:
	return eError;
#endif
}

PVRSRV_ERROR
PMRMakeLocalImportHandle(PMR *psPMR,
                         PMR **ppsPMR)
{
	PVRSRV_ERROR eError = PMRRefPMR(psPMR);
	PVR_RETURN_IF_ERROR(eError);

	*ppsPMR = psPMR;

	return PVRSRV_OK;
}

PVRSRV_ERROR
PMRUnmakeLocalImportHandle(PMR *psPMR)
{
	return PMRUnrefPMR(psPMR);
}

/*
	Note:
	We pass back the PMR as it was passed in as a different handle type
	(DEVMEM_MEM_IMPORT) and it allows us to change the import structure
	type if we should need to embed any meta data in it.
 */
PVRSRV_ERROR
PMRLocalImportPMR(PMR *psPMR,
                  PMR **ppsPMR,
                  IMG_DEVMEM_SIZE_T *puiSize,
                  IMG_DEVMEM_ALIGN_T *puiAlign)
{
	PVRSRV_ERROR eError = _Ref(psPMR, 1);
	PVR_RETURN_IF_ERROR(eError);

	/* Return the PMR */
	*ppsPMR = psPMR;
	*puiSize = psPMR->uiLogicalSize;
	*puiAlign = IMG_PAGE2BYTES64(psPMR->uiLog2ContiguityGuarantee);
	return PVRSRV_OK;
}

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
IMG_UINT64 PMRGetSerialNum(PMR *psPMR)
{
	return psPMR != NULL ? psPMR->uiSerialNum : (IMG_UINT64) -1ULL;
}
#endif

inline IMG_UINT64
PMRInternalGetUID(PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	return psPMR->uiSerialNum;
}

PVRSRV_ERROR
PMRGetUID(PMR *psPMR,
          IMG_UINT64 *pui64UID)
{
	PVR_ASSERT(psPMR != NULL);

	*pui64UID = PMRInternalGetUID(psPMR);

	return PVRSRV_OK;
}

#if defined(SUPPORT_INSECURE_EXPORT)
PVRSRV_ERROR
PMRExportPMR(PMR *psPMR,
             PMR_EXPORT **ppsPMRExportPtr,
             PMR_SIZE_T *puiSize,
             PMR_LOG2ALIGN_T *puiLog2Contig,
             PMR_PASSWORD_T *puiPassword)
{
	IMG_UINT64 uiPassword;
	PMR_EXPORT *psPMRExport;
	PVRSRV_ERROR eError;

	uiPassword = psPMR->uiKey;

	psPMRExport = OSAllocMem(sizeof(*psPMRExport));
	PVR_RETURN_IF_NOMEM(psPMRExport);

	psPMRExport->psPMR = psPMR;

	eError = _Ref(psPMR, 1);
	PVR_GOTO_IF_ERROR(eError, ErrFreePMRExport);

	/* The layout of a PMR can't change once exported
	 * to make sure the importers view of the memory is
	 * the same as exporter. */
	PMR_SetLayoutFixed(psPMR, IMG_TRUE);

	*ppsPMRExportPtr = psPMRExport;
	*puiSize = psPMR->uiLogicalSize;
	*puiLog2Contig = psPMR->uiLog2ContiguityGuarantee;
	*puiPassword = uiPassword;

	return PVRSRV_OK;

ErrFreePMRExport:
	OSFreeMem(psPMRExport);

	return eError;
}


PVRSRV_ERROR
PMRUnexportPMR(PMR_EXPORT *psPMRExport)
{
	PVRSRV_ERROR eError;

	PVR_ASSERT(psPMRExport != NULL);
	PVR_ASSERT(psPMRExport->psPMR != NULL);
	PVR_ASSERT(psPMRExport->psPMR->uiRefCount > 0);

	eError = _UnrefAndMaybeDestroy(psPMRExport->psPMR, 1);
	PVR_LOG_RETURN_IF_ERROR(eError, "_UnrefAndMaybeDestroy");

	OSFreeMem(psPMRExport);

	return PVRSRV_OK;
}


PVRSRV_ERROR
PMRImportPMR(PMR_EXPORT *psPMRExport,
             PMR_PASSWORD_T uiPassword,
             PMR_SIZE_T uiSize,
             PMR_LOG2ALIGN_T uiLog2Contig,
             PMR **ppsPMR)
{
	PMR *psPMR;
	PVRSRV_ERROR eError;

	PVR_ASSERT(psPMRExport->psPMR->uiRefCount > 0);

	psPMR = psPMRExport->psPMR;

	PVR_ASSERT(PMR_IsMemLayoutFixed(psPMR));

	if (psPMR->uiKey != uiPassword)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"PMRImport: Import failed, password specified does not match the export"));
		return PVRSRV_ERROR_PMR_WRONG_PASSWORD_OR_STALE_PMR;
	}

	if (psPMR->uiLogicalSize != uiSize || psPMR->uiLog2ContiguityGuarantee != uiLog2Contig)
	{
		return PVRSRV_ERROR_PMR_MISMATCHED_ATTRIBUTES;
	}

	eError = _Ref(psPMR, 1);
	PVR_RETURN_IF_ERROR(eError);

	*ppsPMR = psPMR;

	return PVRSRV_OK;
}

PVRSRV_ERROR
PMRUnimportPMR(PMR *psPMR)
{
	return _UnrefAndMaybeDestroy(psPMR, 1);
}

#endif /* if defined(SUPPORT_INSECURE_EXPORT) */

#if defined(SUPPORT_SECURE_EXPORT)
PVRSRV_ERROR PMRSecureUnexportPMR(PMR *psPMR)
{
	return _UnrefAndMaybeDestroy(psPMR, 1);
}

static PVRSRV_ERROR _ReleaseSecurePMR(void *psExport)
{
	return PMRSecureUnexportPMR(psExport);
}

PVRSRV_ERROR PMRSecureExportPMR(CONNECTION_DATA *psConnection,
                                PVRSRV_DEVICE_NODE * psDevNode,
                                PMR *psPMR,
                                IMG_SECURE_TYPE *phSecure,
                                PMR **ppsPMR,
                                CONNECTION_DATA **ppsSecureConnection)
{
	PVRSRV_ERROR eError;

	PVR_UNREFERENCED_PARAMETER(psDevNode);
	PVR_UNREFERENCED_PARAMETER(ppsSecureConnection);

	/* We are acquiring reference to PMR here because OSSecureExport
	 * releases bridge lock and PMR lock for a moment and we don't want PMR
	 * to be removed by other thread in the meantime. */
	eError = _Ref(psPMR, 1);
	PVR_RETURN_IF_ERROR(eError);

	eError = OSSecureExport("secure_pmr",
	                        _ReleaseSecurePMR,
	                        (void *) psPMR,
	                        phSecure);
	PVR_GOTO_IF_ERROR(eError, e0);

	*ppsPMR = psPMR;

	/* Mark the PMR immutable once exported
	 * This allows the importers and exporter to have
	 * the same view of the memory */
	PMR_SetLayoutFixed(psPMR, IMG_TRUE);

	return PVRSRV_OK;
e0:
	PVR_ASSERT(eError != PVRSRV_OK);
	(void) _UnrefAndMaybeDestroy(psPMR, 1);
	return eError;
}

PVRSRV_ERROR PMRSecureImportPMR(CONNECTION_DATA *psConnection,
                                PVRSRV_DEVICE_NODE *psDevNode,
                                IMG_SECURE_TYPE hSecure,
                                PMR **ppsPMR,
                                IMG_DEVMEM_SIZE_T *puiSize,
                                IMG_DEVMEM_ALIGN_T *puiAlign)
{
	PVRSRV_ERROR eError;
	PMR *psPMR;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	eError = OSSecureImport(hSecure, (void **) &psPMR);
	PVR_GOTO_IF_ERROR(eError, ErrReturnError);

	PVR_LOG_RETURN_IF_FALSE(PhysHeapDeviceNode(psPMR->psPhysHeap) == psDevNode,
	                        "PMR invalid for this device",
	                        PVRSRV_ERROR_PMR_NOT_PERMITTED);

	eError = _Ref(psPMR, 1);
	PVR_GOTO_IF_ERROR(eError, ErrReturnError);

	/* The PMR should be immutable once exported
	 * This allows the importers and exporter to have
	 * the same view of the memory */
	PVR_ASSERT(PMR_IsMemLayoutFixed(psPMR));

	/* Return the PMR */
	*ppsPMR = psPMR;
	*puiSize = psPMR->uiLogicalSize;
	*puiAlign = IMG_PAGE2BYTES64(psPMR->uiLog2ContiguityGuarantee);

	return PVRSRV_OK;

ErrReturnError:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

PVRSRV_ERROR PMRSecureUnimportPMR(PMR *psPMR)
{
	return _UnrefAndMaybeDestroy(psPMR, 1);
}
#endif

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
PVRSRV_ERROR
PMRStoreRIHandle(PMR *psPMR,
                 void *hRIHandle)
{
	PVR_ASSERT(psPMR != NULL);

	psPMR->hRIHandle = hRIHandle;
	return PVRSRV_OK;
}
#endif

static PVRSRV_ERROR
_PMRAcquireKernelMappingData(PMR *psPMR,
                             size_t uiLogicalOffset,
                             size_t uiSize,
                             void **ppvKernelAddressOut,
                             size_t *puiLengthOut,
                             IMG_HANDLE *phPrivOut,
                             IMG_BOOL bMapSparse)
{
	PVRSRV_ERROR eError;
	void *pvKernelAddress;
	IMG_HANDLE hPriv;

	PVR_ASSERT(psPMR != NULL);

	if (_PMRIsSparse(psPMR) && !bMapSparse)
	{
		/* Mapping of sparse allocations must be signalled. */
		return PVRSRV_ERROR_PMR_NOT_PERMITTED;
	}

	/* Acquire/Release functions must be overridden in pairs */
	if (psPMR->psFuncTab->pfnAcquireKernelMappingData == NULL)
	{
		PVR_ASSERT (psPMR->psFuncTab->pfnReleaseKernelMappingData == NULL);

		/* If PMR implementation does not supply this pair of
		 * functions, it means they do not permit the PMR to be mapped
		 * into kernel memory at all
		 */
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_NOT_PERMITTED, e0);
	}
	PVR_ASSERT (psPMR->psFuncTab->pfnReleaseKernelMappingData != NULL);

	eError = psPMR->psFuncTab->pfnAcquireKernelMappingData(psPMR->pvFlavourData,
	                                                       uiLogicalOffset,
	                                                       uiSize,
	                                                       &pvKernelAddress,
	                                                       &hPriv,
	                                                       psPMR->uiFlags);
	PVR_GOTO_IF_ERROR(eError, e0);

	*ppvKernelAddressOut = pvKernelAddress;
	if (uiSize == 0)
	{
		/* Zero size means map in the whole PMR ... */
		*puiLengthOut = (size_t)psPMR->uiLogicalSize;
	}
	else if (uiSize > IMG_PAGE2BYTES64(psPMR->uiLog2ContiguityGuarantee))
	{
		/* ... map in the requested pages ... */
		*puiLengthOut = uiSize;
	}
	else
	{
		/* ... otherwise we just map in one page */
		*puiLengthOut = IMG_PAGE2BYTES64(psPMR->uiLog2ContiguityGuarantee);
	}
	*phPrivOut = hPriv;

	return PVRSRV_OK;

e0:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

PVRSRV_ERROR
PMRAcquireKernelMappingData(PMR *psPMR,
                            size_t uiLogicalOffset,
                            size_t uiSize,
                            void **ppvKernelAddressOut,
                            size_t *puiLengthOut,
                            IMG_HANDLE *phPrivOut)
{
	PVRSRV_ERROR eError;

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	PMRKernelCpuMapCountIncr(psPMR);

	if (PVRSRV_CHECK_OS_LINUX_MOVABLE(PMR_Flags(psPMR)))
	{
		do
		{
			eError = _PMRAcquireKernelMappingData(psPMR,
			                                      uiLogicalOffset,
			                                      uiSize,
			                                      ppvKernelAddressOut,
			                                      puiLengthOut,
			                                      phPrivOut,
			                                      IMG_FALSE);
		}
		while (eError == PVRSRV_ERROR_RETRY);
		PVR_LOG_GOTO_IF_ERROR(eError, "_PMRAcquireKernelMappingData", error_fail_decr);
	}
	else
#endif
	{
		eError = _PMRAcquireKernelMappingData(psPMR,
		                                      uiLogicalOffset,
		                                      uiSize,
		                                      ppvKernelAddressOut,
		                                      puiLengthOut,
		                                      phPrivOut,
		                                      IMG_FALSE);
		PVR_LOG_GOTO_IF_ERROR(eError, "_PMRAcquireKernelMappingData", error_fail_decr);
	}

	return eError;

error_fail_decr:
#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	PMRKernelCpuMapCountDecr(psPMR);
#endif
	return eError;
}

PVRSRV_ERROR
PMRAcquireSparseKernelMappingData(PMR *psPMR,
                                  size_t uiLogicalOffset,
                                  size_t uiSize,
                                  void **ppvKernelAddressOut,
                                  size_t *puiLengthOut,
                                  IMG_HANDLE *phPrivOut)
{
	PVRSRV_ERROR eError;

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	PMRKernelCpuMapCountIncr(psPMR);
#endif

	eError =  _PMRAcquireKernelMappingData(psPMR,
	                                    uiLogicalOffset,
	                                    uiSize,
	                                    ppvKernelAddressOut,
	                                    puiLengthOut,
	                                    phPrivOut,
	                                    IMG_TRUE);
	PVR_LOG_GOTO_IF_ERROR(eError, "_PMRAcquireKernelMappingData", error_fail_decr);

	return eError;

error_fail_decr:
#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	PMRKernelCpuMapCountDecr(psPMR);
#endif
	return eError;
}

PVRSRV_ERROR
PMRReleaseKernelMappingData(PMR *psPMR,
                            IMG_HANDLE hPriv)
{
	PVR_ASSERT (psPMR->psFuncTab->pfnAcquireKernelMappingData != NULL);
	PVR_ASSERT (psPMR->psFuncTab->pfnReleaseKernelMappingData != NULL);


	psPMR->psFuncTab->pfnReleaseKernelMappingData(psPMR->pvFlavourData,
	                                              hPriv);

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	PMRKernelCpuMapCountDecr(psPMR);
#endif

	return PVRSRV_OK;
}

/*
	_PMRLogicalOffsetToPhysicalOffset

	Translate between the "logical" offset which the upper levels
	provide and the physical offset which is what the PMR
	factories works on.

	As well as returning the physical offset we return the number of
	bytes remaining till the next chunk and if this chunk is valid.

	For multi-page operations, upper layers communicate their
	Log2PageSize else argument is redundant (set to zero).
 */

static PVRSRV_ERROR
_PMRLogicalOffsetToPhysicalOffset(const PMR *psPMR,
                                  IMG_UINT32 ui32Log2PageSize,
                                  IMG_UINT32 ui32NumOfPages,
                                  IMG_DEVMEM_OFFSET_T uiLogicalOffset,
                                  IMG_DEVMEM_OFFSET_T *puiPhysicalOffset,
                                  IMG_UINT32 *pui32BytesRemain,
                                  IMG_BOOL *bValid)
{
	PMR_MAPPING_TABLE *psMappingTable = psPMR->psMappingTable;
	IMG_DEVMEM_OFFSET_T uiPageSize = 1ULL << ui32Log2PageSize;
	IMG_DEVMEM_OFFSET_T uiOffset = uiLogicalOffset;
	IMG_UINT64 ui64ChunkIndex;
	IMG_UINT32 ui32Remain;
	IMG_UINT32 idx;

	/* Must be translating at least a page */
	PVR_ASSERT(ui32NumOfPages > 0);

	if (psMappingTable->ui32NumPhysChunks == psMappingTable->ui32NumLogicalChunks)
	{
		/* Fast path the common case, as logical and physical offsets are
		 * equal we assume the ui32NumOfPages span is also valid */
		*pui32BytesRemain = TRUNCATE_64BITS_TO_32BITS(psPMR->uiLogicalSize - uiOffset);
		puiPhysicalOffset[0] = uiOffset;
		bValid[0] = IMG_TRUE;

		if (ui32NumOfPages > 1)
		{
			/* initial offset may not be page aligned, round down */
			uiOffset &= ~(uiPageSize-1);
			for (idx = 1; idx < ui32NumOfPages; idx++)
			{
				uiOffset += uiPageSize;
				puiPhysicalOffset[idx] = uiOffset;
				bValid[idx] = IMG_TRUE;
			}
		}
	}
	else
	{
		for (idx = 0; idx < ui32NumOfPages; idx++)
		{
			IMG_UINT32 uiTranslation;
			const IMG_UINT32 uiChunkSize = psMappingTable->uiChunkSize;

			ui64ChunkIndex = OSDivide64r64(
					uiOffset,
					TRUNCATE_64BITS_TO_32BITS(uiChunkSize),
					&ui32Remain);

			/* In some cases ui32NumOfPages can come from the user space which
			 * means that the uiOffset could go out-of-bounds when the number
			 * of pages is invalid. */
			if (ui64ChunkIndex >= psMappingTable->ui32NumLogicalChunks)
			{
				return PVRSRV_ERROR_BAD_MAPPING;
			}

			uiTranslation = psMappingTable->aui32Translation[ui64ChunkIndex];

			if (uiTranslation == TRANSLATION_INVALID)
			{
				bValid[idx] = IMG_FALSE;
				/* explicitly set to an invalid value */
				puiPhysicalOffset[idx] = IMG_UINT64_C(0xffffffffffffffff);
			}
			else
			{
				bValid[idx] = IMG_TRUE;
				puiPhysicalOffset[idx] = uiTranslation * uiChunkSize + ui32Remain;
			}

			if (idx == 0)
			{
				*pui32BytesRemain = TRUNCATE_64BITS_TO_32BITS(uiChunkSize - ui32Remain);
				/* initial offset may not be page aligned, round down */
				uiOffset &= ~(uiPageSize-1);
			}

			uiOffset += uiPageSize;
		}
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR
_PMR_ReadBytesPhysical(PMR *psPMR,
                       IMG_DEVMEM_OFFSET_T uiPhysicalOffset,
                       IMG_UINT8 *pcBuffer,
                       size_t uiBufSz,
                       size_t *puiNumBytes)
{
	PVRSRV_ERROR eError;

	if (psPMR->psFuncTab->pfnReadBytes != NULL)
	{
		/* defer to callback if present */

		eError = PMRLockPhysAddresses(psPMR);
		PVR_GOTO_IF_ERROR(eError, e0);

		eError = psPMR->psFuncTab->pfnReadBytes(psPMR->pvFlavourData,
		                                        uiPhysicalOffset,
		                                        pcBuffer,
		                                        uiBufSz,
		                                        puiNumBytes);
		PMRUnlockPhysAddresses(psPMR);
		PVR_GOTO_IF_ERROR(eError, e0);
	}
	else if (psPMR->psFuncTab->pfnAcquireKernelMappingData)
	{
		/* "default" handler for reading bytes */

		IMG_HANDLE hKernelMappingHandle;
		IMG_UINT8 *pcKernelAddress;

		eError = psPMR->psFuncTab->pfnAcquireKernelMappingData(psPMR->pvFlavourData,
		                                                       (size_t) uiPhysicalOffset,
		                                                       uiBufSz,
		                                                       (void **)&pcKernelAddress,
		                                                       &hKernelMappingHandle,
		                                                       psPMR->uiFlags);
		PVR_GOTO_IF_ERROR(eError, e0);

		/* Use the conservative 'DeviceMemCopy' here because we can't
		 * know if this PMR will be mapped cached.
		 */

		OSDeviceMemCopy(&pcBuffer[0], pcKernelAddress, uiBufSz);
		*puiNumBytes = uiBufSz;

		psPMR->psFuncTab->pfnReleaseKernelMappingData(psPMR->pvFlavourData,
		                                              hKernelMappingHandle);
	}
	else
	{
		OSPanic();
#ifndef __CHECKER__ /* exclude for Smatch */
		PVR_LOG_GOTO_WITH_ERROR("psPMR->psFuncTab", eError, PVRSRV_ERROR_INVALID_PARAMS, e0);
#endif
	}

	return PVRSRV_OK;

	/* Error exit paths follow */
e0:
	PVR_ASSERT(eError != PVRSRV_OK);
	*puiNumBytes = 0;
	return eError;
}

PVRSRV_ERROR
PMR_ReadBytes(PMR *psPMR,
              IMG_DEVMEM_OFFSET_T uiLogicalOffset,
              IMG_UINT8 *pcBuffer,
              size_t uiBufSz,
              size_t *puiNumBytes)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_DEVMEM_OFFSET_T uiPhysicalOffset;
	size_t uiBytesCopied = 0;

	/* Check for integer overflow as uiLogicalOffset might come from the client */
	if (uiLogicalOffset + uiBufSz < uiLogicalOffset)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (uiLogicalOffset + uiBufSz > psPMR->uiLogicalSize)
	{
		uiBufSz = TRUNCATE_64BITS_TO_32BITS(psPMR->uiLogicalSize - uiLogicalOffset);
	}
	PVR_ASSERT(uiBufSz > 0);
	PVR_ASSERT(uiBufSz <= psPMR->uiLogicalSize);

	/* PMR implementations can override this. If they don't, a "default"
	 * handler uses kernel virtual mappings.  If the kernel can't
	 * provide a kernel virtual mapping, this function fails.
	 */
	PVR_ASSERT(psPMR->psFuncTab->pfnAcquireKernelMappingData != NULL ||
	           psPMR->psFuncTab->pfnReadBytes != NULL);

	while (uiBytesCopied != uiBufSz)
	{
		IMG_UINT32 ui32Remain;
		size_t uiBytesToCopy;
		size_t uiRead;
		IMG_BOOL bValid;

		eError = _PMRLogicalOffsetToPhysicalOffset(psPMR,
		                                           0,
		                                           1,
		                                           uiLogicalOffset,
		                                           &uiPhysicalOffset,
		                                           &ui32Remain,
		                                           &bValid);
		PVR_LOG_RETURN_IF_ERROR(eError, "_PMRLogicalOffsetToPhysicalOffset");

		/* Copy till either then end of the chunk or end
		 * of the buffer
		 */
		uiBytesToCopy = MIN(uiBufSz - uiBytesCopied, ui32Remain);

		if (bValid)
		{
			/* Read the data from the PMR */
			eError = _PMR_ReadBytesPhysical(psPMR,
			                                uiPhysicalOffset,
			                                &pcBuffer[uiBytesCopied],
			                                uiBytesToCopy,
			                                &uiRead);
			if ((eError != PVRSRV_OK) || (uiRead != uiBytesToCopy))
			{
				PVR_DPF((PVR_DBG_ERROR,
						"%s: Failed to read chunk (eError = %s, uiRead = " IMG_SIZE_FMTSPEC " uiBytesToCopy = " IMG_SIZE_FMTSPEC ")",
						__func__,
						PVRSRVGetErrorString(eError),
						uiRead,
						uiBytesToCopy));
				/* Bail out as soon as we hit an error */
				break;
			}
		}
		else
		{
			PVR_DPF((PVR_DBG_WARNING,
					"%s: Invalid phys offset at logical offset (" IMG_DEVMEM_OFFSET_FMTSPEC ") logical size (" IMG_DEVMEM_OFFSET_FMTSPEC ")",
					__func__,
					uiLogicalOffset,
					psPMR->uiLogicalSize));
			/* Fill invalid chunks with 0 */
			OSCachedMemSet(&pcBuffer[uiBytesCopied], 0, uiBytesToCopy);
			uiRead = uiBytesToCopy;
			eError = PVRSRV_ERROR_FAILED_TO_GET_PHYS_ADDR;
		}
		uiLogicalOffset += uiRead;
		uiBytesCopied += uiRead;
	}

	*puiNumBytes = uiBytesCopied;
	return eError;
}

static PVRSRV_ERROR
_PMR_WriteBytesPhysical(PMR *psPMR,
                        IMG_DEVMEM_OFFSET_T uiPhysicalOffset,
                        IMG_UINT8 *pcBuffer,
                        size_t uiBufSz,
                        size_t *puiNumBytes)
{
	PVRSRV_ERROR eError;

	if (psPMR->psFuncTab->pfnWriteBytes != NULL)
	{
		/* defer to callback if present */

		eError = PMRLockPhysAddresses(psPMR);
		PVR_GOTO_IF_ERROR(eError, e0);

		eError = psPMR->psFuncTab->pfnWriteBytes(psPMR->pvFlavourData,
		                                         uiPhysicalOffset,
		                                         pcBuffer,
		                                         uiBufSz,
		                                         puiNumBytes);
		PMRUnlockPhysAddresses(psPMR);
		PVR_GOTO_IF_ERROR(eError, e0);
	}
	else if (psPMR->psFuncTab->pfnAcquireKernelMappingData)
	{
		/* "default" handler for reading bytes */

		IMG_HANDLE hKernelMappingHandle;
		IMG_UINT8 *pcKernelAddress;

		eError = psPMR->psFuncTab->pfnAcquireKernelMappingData(psPMR->pvFlavourData,
		                                                       (size_t) uiPhysicalOffset,
		                                                       uiBufSz,
		                                                       (void **)&pcKernelAddress,
		                                                       &hKernelMappingHandle,
		                                                       psPMR->uiFlags);
		PVR_GOTO_IF_ERROR(eError, e0);

		/* Use the conservative 'DeviceMemCopy' here because we can't know
		 * if this PMR will be mapped cached.
		 */

		OSDeviceMemCopy(pcKernelAddress, &pcBuffer[0], uiBufSz);
		*puiNumBytes = uiBufSz;

		psPMR->psFuncTab->pfnReleaseKernelMappingData(psPMR->pvFlavourData,
		                                              hKernelMappingHandle);
	}
	else
	{
		/* The write callback is optional as it's only required by the
		 * debug tools
		 */
		OSPanic();
#ifndef __CHECKER__ /* exclude for Smatch */
		PVR_LOG_GOTO_WITH_ERROR("psPMR->psFuncTab", eError, PVRSRV_ERROR_PMR_NOT_PERMITTED, e0);
#endif
	}

	return PVRSRV_OK;

	/* Error exit paths follow */
e0:
	PVR_ASSERT(eError != PVRSRV_OK);
	*puiNumBytes = 0;
	return eError;
}

PVRSRV_ERROR
PMR_WriteBytes(PMR *psPMR,
               IMG_DEVMEM_OFFSET_T uiLogicalOffset,
               IMG_UINT8 *pcBuffer,
               size_t uiBufSz,
               size_t *puiNumBytes)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_DEVMEM_OFFSET_T uiPhysicalOffset;
	size_t uiBytesCopied = 0;

	/* Check for integer overflow as uiLogicalOffset might come from the client */
	if (uiLogicalOffset + uiBufSz < uiLogicalOffset)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (uiLogicalOffset + uiBufSz > psPMR->uiLogicalSize)
	{
		uiBufSz = TRUNCATE_64BITS_TO_32BITS(psPMR->uiLogicalSize - uiLogicalOffset);
	}
	PVR_ASSERT(uiBufSz > 0);
	PVR_ASSERT(uiBufSz <= psPMR->uiLogicalSize);

	/* PMR implementations can override this. If they don't, a "default"
	 * handler uses kernel virtual mappings. If the kernel can't provide
	 * a kernel virtual mapping, this function fails.
	 */
	PVR_ASSERT(psPMR->psFuncTab->pfnAcquireKernelMappingData != NULL ||
	           psPMR->psFuncTab->pfnWriteBytes != NULL);

	while (uiBytesCopied != uiBufSz)
	{
		IMG_UINT32 ui32Remain;
		size_t uiBytesToCopy;
		size_t uiWrite;
		IMG_BOOL bValid;

		eError = _PMRLogicalOffsetToPhysicalOffset(psPMR,
		                                           0,
		                                           1,
		                                           uiLogicalOffset,
		                                           &uiPhysicalOffset,
		                                           &ui32Remain,
		                                           &bValid);
		PVR_LOG_RETURN_IF_ERROR(eError, "_PMRLogicalOffsetToPhysicalOffset");

		/* Copy till either then end of the chunk or end of the buffer
		 */
		uiBytesToCopy = MIN(uiBufSz - uiBytesCopied, ui32Remain);

		if (bValid)
		{
			/* Write the data to the PMR */
			eError = _PMR_WriteBytesPhysical(psPMR,
			                                 uiPhysicalOffset,
			                                 &pcBuffer[uiBytesCopied],
			                                 uiBytesToCopy,
			                                 &uiWrite);
			if ((eError != PVRSRV_OK) || (uiWrite != uiBytesToCopy))
			{
				PVR_DPF((PVR_DBG_ERROR,
						"%s: Failed to read chunk (eError = %s, uiWrite = " IMG_SIZE_FMTSPEC " uiBytesToCopy = " IMG_SIZE_FMTSPEC ")",
						__func__,
						PVRSRVGetErrorString(eError),
						uiWrite,
						uiBytesToCopy));
				/* Bail out as soon as we hit an error */
				break;
			}
		}
		else
		{
			/* Ignore writes to invalid pages */
			uiWrite = uiBytesToCopy;
		}
		uiLogicalOffset += uiWrite;
		uiBytesCopied += uiWrite;
	}

	*puiNumBytes = uiBytesCopied;
	return eError;
}

PVRSRV_ERROR
PMRMMapPMR(PMR *psPMR, PMR_MMAP_DATA pOSMMapData, PVRSRV_MEMALLOCFLAGS_T uiFlags)
{
	/* if writeable mapping is requested on non-writeable PMR then fail */
	PVR_RETURN_IF_FALSE(PVRSRV_CHECK_CPU_WRITEABLE(psPMR->uiFlags) ||
	                    !PVRSRV_CHECK_CPU_WRITEABLE(uiFlags),
	                    PVRSRV_ERROR_PMR_NOT_PERMITTED);

	/* if readable mapping is requested on non-readable PMR then fail */
	PVR_RETURN_IF_FALSE(PVRSRV_CHECK_CPU_READABLE(psPMR->uiFlags) ||
	                    !PVRSRV_CHECK_CPU_READABLE(uiFlags),
	                    PVRSRV_ERROR_PMR_NOT_PERMITTED);

	if (psPMR->psFuncTab->pfnMMap)
	{
		return psPMR->psFuncTab->pfnMMap(psPMR->pvFlavourData, psPMR, pOSMMapData);
	}

	return OSMMapPMRGeneric(psPMR, pOSMMapData);
}

PVRSRV_ERROR
PMRRefPMR(PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);
	return _Ref(psPMR, 1);
}

PVRSRV_ERROR
PMRUnrefPMR(PMR *psPMR)
{
	return _UnrefAndMaybeDestroy(psPMR, 1);
}

PVRSRV_ERROR
PMRRefPMRN(PMR *psPMR, IMG_UINT32 uiRefCount)
{
	PVR_ASSERT(psPMR != NULL);
	return _Ref(psPMR, uiRefCount);
}

PVRSRV_ERROR
PMRUnrefPMRN(PMR *psPMR, IMG_UINT32 uiRefCount)
{
	PVR_ASSERT(psPMR != NULL);
	return _UnrefAndMaybeDestroy(psPMR, uiRefCount);
}

#define PMR_MAPCOUNT_MIN 0
#define PMR_MAPCOUNT_MAX IMG_INT32_MAX

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
PVRSRV_ERROR PMRTryRefPMR(PMR *psPMR)
{
	OS_SPINLOCK_FLAGS uiFlags;

	PVR_ASSERT(psPMR != NULL);

	OSSpinLockAcquire(psPMR->hRefCountLock, uiFlags);

	if (psPMR->uiRefCount == 0 || psPMR->uiRefCount == IMG_UINT32_MAX)
	{
		OSSpinLockRelease(psPMR->hRefCountLock, uiFlags);

		return PVRSRV_ERROR_PMR_NOT_PERMITTED;
	}

	psPMR->uiRefCount++;

	OSSpinLockRelease(psPMR->hRefCountLock, uiFlags);

	return PVRSRV_OK;
}

void
PMRKernelCpuMapCountIncr(PMR *psPMR)
{
	if (OSAtomicAddUnless(&psPMR->iKernelCpuMapCount, 1, PMR_MAPCOUNT_MAX) == PMR_MAPCOUNT_MAX)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: iKernelCpuMapCount for PMR: @0x%p (%s) has overflowed.",
		                        __func__,
		                        psPMR,
		                        psPMR->szAnnotation));
		OSWarnOn(1);
	}
}

void
PMRKernelCpuMapCountDecr(PMR *psPMR)
{
	if (OSAtomicSubtractUnless(&psPMR->iKernelCpuMapCount, 1, PMR_MAPCOUNT_MIN) == PMR_MAPCOUNT_MIN)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: iKernelCpuMapCount (now %d) for PMR: @0x%p (%s) has underflowed.",
		                        __func__,
		                        (IMG_INT32) OSAtomicRead(&psPMR->iKernelCpuMapCount),
		                        psPMR,
		                        psPMR->szAnnotation));
		OSWarnOn(1);
	}
}

IMG_BOOL
PMR_IsKernelCpuMapped(PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	return (OSAtomicRead(&psPMR->iKernelCpuMapCount) > 0);
}

#endif /* #if defined(SUPPORT_LINUX_OSPAGE_MIGRATION) */

void
PMRClientCpuMapCountIncr(PMR *psPMR)
{
	if (OSAtomicAddUnless(&psPMR->iClientCpuMapCount, 1, PMR_MAPCOUNT_MAX) == PMR_MAPCOUNT_MAX)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: iClientCpuMapCount for PMR: @0x%p (%s) has overflowed.",
		                        __func__,
		                        psPMR,
		                        psPMR->szAnnotation));
		OSWarnOn(1);
	}
}

void
PMRClientCpuMapCountDecr(PMR *psPMR)
{
	if (OSAtomicSubtractUnless(&psPMR->iClientCpuMapCount, 1, PMR_MAPCOUNT_MIN) == PMR_MAPCOUNT_MIN)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: iClientCpuMapCount (now %d) for PMR: @0x%p (%s) has underflowed.",
		                        __func__,
		                        (IMG_INT32) OSAtomicRead(&psPMR->iClientCpuMapCount),
		                        psPMR,
		                        psPMR->szAnnotation));
		OSWarnOn(1);
	}
}

IMG_BOOL
PMR_IsClientCpuMapped(PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	return (OSAtomicRead(&psPMR->iClientCpuMapCount) > 0);
}

static void
PMRGpuResCountIncr(PMR *psPMR)
{
	if (psPMR->iAssociatedResCount == PMR_MAPCOUNT_MAX)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: iAssociatedResCount for PMR: @0x%p (%s) has overflowed.",
		                        __func__,
		                        psPMR,
		                        psPMR->szAnnotation));
		OSWarnOn(1);
		return;
	}

	psPMR->iAssociatedResCount++;
}

static void
PMRGpuResCountDecr(PMR *psPMR)
{
	if (psPMR->iAssociatedResCount == PMR_MAPCOUNT_MIN)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: iAssociatedResCount (now %d) for PMR: @0x%p (%s) has underflowed.",
		                        __func__,
		                        psPMR->iAssociatedResCount,
		                        psPMR,
		                        psPMR->szAnnotation));
		OSWarnOn(1);
		return;
	}

	psPMR->iAssociatedResCount--;
}

IMG_BOOL
PMR_IsGpuMultiMapped(PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	return psPMR->iAssociatedResCount > 1;
}

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
void
PMRLinkGPUMapping(PMR *psPMR, DLLIST_NODE *psMappingNode)
#else
void
PMRLinkGPUMapping(PMR *psPMR)
#endif
{
	PMRLockHeldAssert(psPMR);

	PMRGpuResCountIncr(psPMR);

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	dllist_add_to_head(&psPMR->sGpuMappingListHead, psMappingNode);
#endif
}

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
void
PMRUnlinkGPUMapping(PMR *psPMR, DLLIST_NODE *psMappingNode)
#else
void
PMRUnlinkGPUMapping(PMR *psPMR)
#endif

{
	PMRLockHeldAssert(psPMR);

	PMRGpuResCountDecr(psPMR);

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	dllist_remove_node(psMappingNode);
#endif
}

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
void
PMRNotifyMigrateInProgress(PMR *psPMR)
{
	PMRLockHeldAssert(psPMR);

	psPMR->eState = PMR_STATE_PAGES_IN_MIGRATE;
}

void
PMRNotifyMigrateComplete(PMR *psPMR)
{
	PMRLockHeldAssert(psPMR);

	psPMR->eState = PMR_STATE_ACTIVE;
}

PVRSRV_ERROR
PMRRemapGPUPMR(PMR *psPMR, IMG_UINT32 ui32LogicalPgOffset)
{
	if (psPMR->eState != PMR_STATE_PAGES_IN_MIGRATE)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Remap requested on PMR not in migrate state.", __func__));
		return PVRSRV_ERROR_PMR_NOT_PERMITTED;
	}

	return DevmemIntRemapPageInPMR(psPMR, &psPMR->sGpuMappingListHead, ui32LogicalPgOffset);
}
#endif

PVRSRV_DEVICE_NODE *
PMR_DeviceNode(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	return PhysHeapDeviceNode(psPMR->psPhysHeap);
}

PMR_FLAGS_T
PMR_Flags(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	return psPMR->uiFlags;
}

IMG_BOOL
PMR_IsSparse(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	return _PMRIsSparse(psPMR);
}

#if defined(SUPPORT_PMR_DEFERRED_FREE)
IMG_BOOL
PMR_IsZombie(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	return _IntFlagIsSet(psPMR, PMR_FLAG_INTERNAL_IS_ZOMBIE_PMR);
}
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

/* Exclusive flag tracks if the PMR is supporting or is used by another bridge resource */

/* Function to set the exclusive use flag.
   Returns IMG_FALSE if flag couldn't be set because it is already set.
   IMG_TRUE otherwise. */
IMG_BOOL
PMR_SetExclusiveUse(PMR *psPMR, IMG_BOOL bFlag)
{
	PVR_ASSERT(psPMR != NULL);

	if (bFlag)
	{
		return _IntFlagSetIfNotSet(psPMR, PMR_FLAG_INTERNAL_IS_EXCLUSIVE);
	}
	else
	{
		_IntFlagClr(psPMR, PMR_FLAG_INTERNAL_IS_EXCLUSIVE);
		return IMG_TRUE;
	}
}

/* Function that alters the mutability property
 * of the PMR
 * Setting it to TRUE makes sure the PMR memory layout
 * can't be changed through future calls */
void
PMR_SetLayoutFixed(PMR *psPMR, IMG_BOOL bFlag)
{
	PVR_ASSERT(psPMR != NULL);

	if (bFlag)
	{
		_IntFlagSet(psPMR, PMR_FLAG_INTERNAL_NO_LAYOUT_CHANGE);
	}
	else
	{
		_IntFlagClr(psPMR, PMR_FLAG_INTERNAL_NO_LAYOUT_CHANGE);
	}
}

IMG_BOOL PMR_IsMemLayoutFixed(PMR *psPMR)
{

	PVR_ASSERT(psPMR != NULL);

	return _IntFlagIsSet(psPMR, PMR_FLAG_INTERNAL_NO_LAYOUT_CHANGE);
}

IMG_DEVMEM_SIZE_T
PMR_PhysicalSize(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	/* iLockCount will be > 0 for any backed PMR (backed on demand or not) */
	if (OSAtomicRead(&psPMR->iLockCount) > 0)
	{
		if (_PMRIsSparse(psPMR))
		{
			return psPMR->psMappingTable->uiChunkSize * psPMR->psMappingTable->ui32NumPhysChunks;
		}
		else
		{
			return psPMR->uiLogicalSize;
		}
	}

	return 0;
}

PVRSRV_ERROR
PMR_IsOffsetValid(const PMR *psPMR,
                  IMG_UINT32 ui32Log2PageSize,
                  IMG_UINT32 ui32NumOfPages,
                  IMG_DEVMEM_OFFSET_T uiLogicalOffset,
                  IMG_BOOL *pbValid)
{
	IMG_DEVMEM_OFFSET_T auiPhysicalOffset[PMR_MAX_TRANSLATION_STACK_ALLOC];
	IMG_UINT32 aui32BytesRemain[PMR_MAX_TRANSLATION_STACK_ALLOC];
	IMG_DEVMEM_OFFSET_T *puiPhysicalOffset = auiPhysicalOffset;
	IMG_UINT32 *pui32BytesRemain = aui32BytesRemain;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_ASSERT(psPMR != NULL);
	PVR_ASSERT(psPMR->uiLogicalSize >= uiLogicalOffset);

	if (ui32NumOfPages > PMR_MAX_TRANSLATION_STACK_ALLOC)
	{
		puiPhysicalOffset = OSAllocMem(ui32NumOfPages * sizeof(IMG_DEVMEM_OFFSET_T));
		PVR_GOTO_IF_NOMEM(puiPhysicalOffset, eError, e0);

		pui32BytesRemain = OSAllocMem(ui32NumOfPages * sizeof(IMG_UINT32));
		PVR_GOTO_IF_NOMEM(pui32BytesRemain, eError, e0);
	}

	eError = _PMRLogicalOffsetToPhysicalOffset(psPMR,
	                                           ui32Log2PageSize,
	                                           ui32NumOfPages,
	                                           uiLogicalOffset,
	                                           puiPhysicalOffset,
	                                           pui32BytesRemain,
	                                           pbValid);
	PVR_LOG_IF_ERROR(eError, "_PMRLogicalOffsetToPhysicalOffset");

e0:
	if (puiPhysicalOffset != auiPhysicalOffset && puiPhysicalOffset != NULL)
	{
		OSFreeMem(puiPhysicalOffset);
	}

	if (pui32BytesRemain != aui32BytesRemain && pui32BytesRemain != NULL)
	{
		OSFreeMem(pui32BytesRemain);
	}

	return eError;
}

PHYS_HEAP *
PMR_PhysHeap(const PMR *psPMR)
{
	return psPMR->psPhysHeap;
}

PMR_MAPPING_TABLE *
PMR_GetMappingTable(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);
	return psPMR->psMappingTable;

}

IMG_UINT32
PMR_GetLog2Contiguity(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);
	return psPMR->uiLog2ContiguityGuarantee;
}

IMG_DEVMEM_SIZE_T
PMR_LogicalSize(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	return psPMR->uiLogicalSize;
}

IMG_UINT32 PMR_GetLogicalChunkCount(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	return psPMR->psMappingTable->ui32NumLogicalChunks;
}

const IMG_CHAR *
PMR_GetAnnotation(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);
	return psPMR->szAnnotation;
}

PMR_IMPL_TYPE
PMR_GetType(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);
	return psPMR->eFlavour;
}

static IMG_CHAR*
_GetFlavourStr(PMR_IMPL_TYPE eFlavour)
{
	static IMG_CHAR *pszFlavour[] = {
#define X(type) #type
		PMR_IMPL_TYPES
#undef X
	};
	if (eFlavour >= PMR_TYPE_LAST)
	{
		return "INVALID";
	}
	return pszFlavour[eFlavour];
}

IMG_CHAR *
PMR_GetTypeStr(const PMR *psPMR)
{
	return _GetFlavourStr(psPMR->eFlavour);
}

IMG_INT32
PMR_GetRefCount(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);
	return _GetRef(psPMR);
}

PVRSRV_ERROR
PMR_IsExportable(const PMR *psPMR)
{
	PVR_ASSERT(psPMR != NULL);

	if (!PMR_DeviceNode(psPMR)->pfnValidateExportableFlags(psPMR->uiFlags))
	{
		return PVRSRV_ERROR_INVALID_FLAGS;
	}

	return PVRSRV_OK;
}

#if defined(PVRSRV_INTERNAL_IPA_FEATURE_TESTING)
PVRSRV_ERROR
PMRGetIPAPolicy(PMR *psPMR,
                IMG_UINT8 *pui8IPAPolicy)
{
	IMG_UINT32 ui32FlagsIPAPolicy;

	PVR_LOG_RETURN_IF_INVALID_PARAM(pui8IPAPolicy != NULL, "pui8IPAPolicy");
	ui32FlagsIPAPolicy = PVRSRV_MEMALLOCFLAG_IPA_POLICY(psPMR->uiFlags);

	*pui8IPAPolicy = (IMG_UINT8)ui32FlagsIPAPolicy;
	return PVRSRV_OK;
}
#endif /* defined(PVRSRV_INTERNAL_IPA_FEATURE_TESTING) */

#if defined(PVRSRV_INTERNAL_IPA_FEATURE_TESTING)
PVRSRV_ERROR
PMRGetIPAInfo(PMR *psPMR, IMG_UINT32 *pui32IPAPolicy, IMG_UINT32 *pui32IPAShift,
              IMG_UINT32 *pui32IPAMask, IMG_UINT32 *pui32IPAFlagsValue)
{
	IMG_UINT32 ui32IPAFlagsPolicyValue;

	PVR_LOG_RETURN_IF_INVALID_PARAM(pui32IPAPolicy != NULL, "pui32IPAPolicy");
	PVR_LOG_RETURN_IF_INVALID_PARAM(pui32IPAShift != NULL, "pui32IPAShift");
	PVR_LOG_RETURN_IF_INVALID_PARAM(pui32IPAMask != NULL, "pui32IPAMask");
	PVR_LOG_RETURN_IF_INVALID_PARAM(pui32IPAFlagsValue != NULL, "pui32IPAFlagsValue");

	/* Get the underlying heap-provided default IPA settings (if any) */
	*pui32IPAShift = PhysHeapGetIPAShift(psPMR->psPhysHeap);
	*pui32IPAPolicy = PhysHeapGetIPAValue(psPMR->psPhysHeap);
	*pui32IPAMask = PhysHeapGetIPAMask(psPMR->psPhysHeap);

	PVR_DPF((PVR_DBG_MESSAGE,
	        "%s: Shift, Policy, Mask for Heap %p = %d, %d, %d", __func__,
	        psPMR->psPhysHeap, *pui32IPAShift, *pui32IPAPolicy, *pui32IPAMask));

	/* Query the current PMR flags settings for current IPA policy */
	ui32IPAFlagsPolicyValue = PVRSRV_MEMALLOCFLAG_IPA_POLICY(psPMR->uiFlags);
	*pui32IPAFlagsValue = ui32IPAFlagsPolicyValue;
	return PVRSRV_OK;
}
#endif	/* PVRSRV_INTERNAL_IPA_FEATURE_TESTING */

#if defined(PVRSRV_INTERNAL_IPA_FEATURE_TESTING)
PVRSRV_ERROR
PMRModifyIPAPolicy(PMR *psPMR,
                   IMG_UINT8 ui8NewIPAPolicy)
{
	IMG_UINT64 ui64IPAFlagsPolicyValue;
	IMG_UINT32 ui32IPAPolicyMaskValue;

	ui32IPAPolicyMaskValue = (IMG_UINT32)(PVRSRV_MEMALLOCFLAG_IPA_POLICY_MASK >> PVRSRV_MEMALLOCFLAG_IPA_POLICY_OFFSET);

	/* ui8NewIPAPolicy must be between 0 .. uiMaskValue inclusive */
	PVR_LOG_RETURN_IF_INVALID_PARAM(ui8NewIPAPolicy <= ui32IPAPolicyMaskValue,
	                                "ui8NewIPAPolicy");

	/* Set the appropriate policy bits in the PMR */
	ui64IPAFlagsPolicyValue = ((IMG_UINT64)(ui8NewIPAPolicy) << PVRSRV_MEMALLOCFLAG_IPA_POLICY_OFFSET) &
	                           PVRSRV_MEMALLOCFLAG_IPA_POLICY_MASK;

	psPMR->uiFlags &= ~PVRSRV_MEMALLOCFLAG_IPA_POLICY_MASK;
	psPMR->uiFlags |= ui64IPAFlagsPolicyValue;

	PVR_DPF((PVR_DBG_MESSAGE,
	         "%s: ui32IPAPolicy Mask = 0x%x, Value = 0x%x",
	         __func__, ui32IPAPolicyMaskValue, (IMG_UINT32)ui8NewIPAPolicy));
	PVR_DPF((PVR_DBG_MESSAGE,
	         "%s: uiFlags = 0x%016" IMG_UINT64_FMTSPECx ", ui64IPAFlags = 0x%016" IMG_UINT64_FMTSPECx, __func__, psPMR->uiFlags, ui64IPAFlagsPolicyValue));

	return PVRSRV_OK;
}
#endif /* defined(PVRSRV_INTERNAL_IPA_FEATURE_TESTING) */

/* must have called PMRLockPhysAddresses() before calling this! */
PVRSRV_ERROR
PMR_DevPhysAddr(const PMR *psPMR,
                IMG_UINT32 ui32Log2PageSize,
                IMG_UINT32 ui32NumOfPages,
                IMG_DEVMEM_OFFSET_T uiLogicalOffset,
                IMG_DEV_PHYADDR *psDevAddrPtr,
                IMG_BOOL *pbValid,
                PMR_PHYSADDRMODE_TYPE uiPMRUsage)
{
	IMG_UINT32 ui32Remain;
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_DEVMEM_OFFSET_T auiPhysicalOffset[PMR_MAX_TRANSLATION_STACK_ALLOC];
	IMG_DEVMEM_OFFSET_T *puiPhysicalOffset = auiPhysicalOffset;
#if defined(SUPPORT_STATIC_IPA)
	IMG_UINT32 ui32FlagsIPAPolicy;      /* Local value for the IPA policy */
	IMG_UINT32 ui32IPAHeapShift;        /* Phys-heap bit-shift value */
	IMG_UINT32 ui32IPAHeapPolicyValue;  /* Phys-heap default policy value */
	IMG_UINT32 ui32IPAHeapClearMask;    /* Phys-heap ClearMask bitmask */
	IMG_UINT64 ui64IPAPolicy;           /* IPAPolicy value to be applied to physical address(es) */
	IMG_UINT64 ui64IPAClearMask;        /* IPAClearMask to be applied to physical address(es) */
#endif

	PVR_ASSERT(psPMR != NULL);
	PVR_ASSERT(ui32NumOfPages > 0);
	PVR_ASSERT(psPMR->psFuncTab->pfnDevPhysAddr != NULL);

	if (OSAtomicRead(&psPMR->iLockCount) <= 0)
	{
		PVR_DPF((PVR_DBG_ERROR,"Attempt to obtain DevPhysAddr of non-backed PMR"));
		return PVRSRV_ERROR_PMR_NOT_PERMITTED;
	}

	if (ui32NumOfPages > PMR_MAX_TRANSLATION_STACK_ALLOC)
	{
		puiPhysicalOffset = OSAllocMem(ui32NumOfPages * sizeof(IMG_DEVMEM_OFFSET_T));
		PVR_RETURN_IF_NOMEM(puiPhysicalOffset);
	}

	eError = _PMRLogicalOffsetToPhysicalOffset(psPMR,
	                                           ui32Log2PageSize,
	                                           ui32NumOfPages,
	                                           uiLogicalOffset,
	                                           puiPhysicalOffset,
	                                           &ui32Remain,
	                                           pbValid);
	PVR_LOG_GOTO_IF_ERROR(eError, "_PMRLogicalOffsetToPhysicalOffset", FreeOffsetArray);

#if defined(SUPPORT_STATIC_IPA)
	/* Need to determine the values to pass into the pfnDevPhysAddr
	 * for Intermediate Physical Address settings associated with
	 * this PMR.
	 * If the ui32FlagsIPAPolicy value is non-zero, the value will be used in
	 * preference to the default value specified in the physheap config.
	 * Whichever value is used the associated physheap configuration bit shift and
	 * mask values are passed to the pfnDevPhysAddr PMR factory function to modify
	 * the returned address(es).
	 */

	PVR_ASSERT(psPMR->psPhysHeap != NULL);

	ui32IPAHeapShift = PhysHeapGetIPAShift(psPMR->psPhysHeap);
	ui32IPAHeapPolicyValue = PhysHeapGetIPAValue(psPMR->psPhysHeap);
	ui32IPAHeapClearMask = PhysHeapGetIPAMask(psPMR->psPhysHeap);
#if defined(PVRSRV_INTERNAL_IPA_FEATURE_TESTING)
	ui32FlagsIPAPolicy = PVRSRV_MEMALLOCFLAG_IPA_POLICY(psPMR->uiFlags);
	if (ui32FlagsIPAPolicy == 0U)
	{
		ui32FlagsIPAPolicy = ui32IPAHeapPolicyValue;
	}
#else
	ui32FlagsIPAPolicy = ui32IPAHeapPolicyValue;	/* Use heap default values*/
#endif	/* PVRSRV_INTERNAL_IPA_FEATURE_TESTING */
	/* To handle the 'disabling' of IPAPolicy setting for some callers we
	 * check to see if the uiPMRUsage is set to DEVICE_USE.
	 * If so, we simply use the calculated shifts and policy values determined
	 * above. If disabled BIT_ISSET(uiPMRUsage, CPU_USE) we pass 0 values to the PMR
	 * factory which will result in no IPA modification being made to the
	 * phys_heap physical addresses.
	 */
	if (unlikely(BITMASK_HAS(uiPMRUsage, CPU_USE)))
	{
		ui32IPAHeapClearMask = 0U;
		ui32FlagsIPAPolicy = 0U;
	}
	ui64IPAPolicy = (IMG_UINT64)ui32FlagsIPAPolicy << ui32IPAHeapShift;
	ui64IPAClearMask = (IMG_UINT64)ui32IPAHeapClearMask << ui32IPAHeapShift;
#endif	/* SUPPORT_STATIC_IPA */

#if defined(SUPPORT_LINUX_OSPAGE_MIGRATION)
	/* PMRs marked with migrate ability need to wait for migrate to complete
	 * whilst in the CPU mapping path. PMR lock must be held in this path to
	 * synchronise obtaining phys addrs.
	 */
	if (BITMASK_HAS(uiPMRUsage, MAPPING_USE) && BITMASK_HAS(uiPMRUsage, CPU_USE))
	{
		PMRLockHeldAssert(psPMR);

		/* We must wait and retry until we are allowed to obtain phys addrs, no timeout
		 * because we need this to complete and migrate is guaranteed to finish eventually
		 */
		while (1)
		{
			if (psPMR->eState == PMR_STATE_ACTIVE)
			{
				break;
			}
			/* Allow another thread to take the lock, this should allow migration to continue
			 * and complete before we take physical addresses for use
			 */
			PMRUnlockPMR(psPMR);
			OSReleaseThreadQuanta();
			PMRLockPMR(psPMR);
		}

		eError = psPMR->psFuncTab->pfnDevPhysAddr(psPMR->pvFlavourData,
												  ui32Log2PageSize,
												  ui32NumOfPages,
												  puiPhysicalOffset,
#if defined(SUPPORT_STATIC_IPA)
												  ui64IPAPolicy,
												  ui64IPAClearMask,
#endif
												  pbValid,
												  psDevAddrPtr);
	}
	/* PMRs marked with migrate ability may need to retry an attempt at Device mappings
	 * for a migrate to complete whilst in the mapping path. This is because we hold higher
	 * synchronisation methods in device mapping paths that also need to release for the
	 * migrate to complete.
	 */
	else if (BITMASK_HAS(uiPMRUsage, MAPPING_USE) && BITMASK_HAS(uiPMRUsage, DEVICE_USE))
	{
		PMRLockHeldAssert(psPMR);

		/* If PMR is in migrate state then ask the requester to retry */
		if (psPMR->eState != PMR_STATE_ACTIVE)
		{
			eError = PVRSRV_ERROR_RETRY;
			goto FreeOffsetArray;
		}

		eError = psPMR->psFuncTab->pfnDevPhysAddr(psPMR->pvFlavourData,
												  ui32Log2PageSize,
												  ui32NumOfPages,
												  puiPhysicalOffset,
#if defined(SUPPORT_STATIC_IPA)
												  ui64IPAPolicy,
												  ui64IPAClearMask,
#endif
												  pbValid,
												  psDevAddrPtr);
	}
	else
	/* All other paths that are not used in GPU mapping will never return
	 * PVRSRV_ERROR_RETRY.
	 */
#endif
	{
		/* Sparse PMR may not always have the first page valid */
		eError = psPMR->psFuncTab->pfnDevPhysAddr(psPMR->pvFlavourData,
												  ui32Log2PageSize,
												  ui32NumOfPages,
												  puiPhysicalOffset,
#if defined(SUPPORT_STATIC_IPA)
												  ui64IPAPolicy,
												  ui64IPAClearMask,
#endif
												  pbValid,
												  psDevAddrPtr);
		PVR_GOTO_IF_ERROR(eError, FreeOffsetArray);
	}

#if defined(PVR_PMR_TRANSLATE_UMA_ADDRESSES)
	/* Currently excluded from the default build because of performance
	 * concerns.
	 * We do not need this part in all systems because the GPU has the same
	 * address view of system RAM as the CPU.
	 * Alternatively this could be implemented as part of the PMR-factories
	 * directly */
	if (PhysHeapGetType(psPMR->psPhysHeap) == PHYS_HEAP_TYPE_UMA ||
		PhysHeapGetType(psPMR->psPhysHeap) == PHYS_HEAP_TYPE_DMA)
	{
		IMG_UINT32 i;
		IMG_DEV_PHYADDR sDevPAddrCorrected;

		/* Copy the translated addresses to the correct array */
		for (i = 0; i < ui32NumOfPages; i++)
		{
			PhysHeapCpuPAddrToDevPAddr(psPMR->psPhysHeap,
									   1,
									   &sDevPAddrCorrected,
									   (IMG_CPU_PHYADDR *) &psDevAddrPtr[i]);
			psDevAddrPtr[i].uiAddr = sDevPAddrCorrected.uiAddr;
		}
	}
#endif


FreeOffsetArray:
	if (puiPhysicalOffset != auiPhysicalOffset)
	{
		OSFreeMem(puiPhysicalOffset);
	}

	return eError;
}

/* must have called PMRLockPhysAddresses() before calling this! */
PVRSRV_ERROR
PMR_CpuPhysAddr(const PMR *psPMR,
                IMG_UINT32 ui32Log2PageSize,
                IMG_UINT32 ui32NumOfPages,
                IMG_DEVMEM_OFFSET_T uiLogicalOffset,
                IMG_CPU_PHYADDR *psCpuAddrPtr,
                IMG_BOOL *pbValid,
                PMR_PHYSADDRMODE_TYPE uiPMRUsage)
{
	IMG_UINT32 idx;
	PVRSRV_ERROR eError;
	IMG_DEV_PHYADDR asDevPAddr[PMR_MAX_TRANSLATION_STACK_ALLOC];
	IMG_DEV_PHYADDR *psDevPAddr = asDevPAddr;

	if (ui32NumOfPages > PMR_MAX_TRANSLATION_STACK_ALLOC)
	{
		psDevPAddr = OSAllocMem(ui32NumOfPages * sizeof(IMG_DEV_PHYADDR));
		PVR_GOTO_IF_NOMEM(psDevPAddr, eError, e0);
	}

	eError = PMR_DevPhysAddr(psPMR, ui32Log2PageSize, ui32NumOfPages,
	                         uiLogicalOffset, psDevPAddr, pbValid, uiPMRUsage);
	PVR_GOTO_IF_ERROR(eError, e1);

	if (_PMRIsSparse(psPMR))
	{
		/* Loop over each page.
		 * If Dev addr valid, populate the CPU addr from the Dev addr
		 */
		for (idx = 0; idx < ui32NumOfPages; idx++)
		{
			if (pbValid[idx])
			{
				PhysHeapDevPAddrToCpuPAddr(psPMR->psPhysHeap, 1, &psCpuAddrPtr[idx], &psDevPAddr[idx]);
			}
		}
	}
	else
	{
		/* In this case all addrs will be valid, so we can block translate */
		PhysHeapDevPAddrToCpuPAddr(psPMR->psPhysHeap, ui32NumOfPages, psCpuAddrPtr, psDevPAddr);
	}

	if (ui32NumOfPages > PMR_MAX_TRANSLATION_STACK_ALLOC)
	{
		OSFreeMem(psDevPAddr);
	}

	return PVRSRV_OK;
e1:
	if (psDevPAddr != asDevPAddr)
	{
		OSFreeMem(psDevPAddr);
	}
e0:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

PVRSRV_ERROR PMR_ChangeSparseMem(PMR *psPMR,
                                 IMG_UINT32 ui32AllocPageCount,
                                 IMG_UINT32 *pai32AllocIndices,
                                 IMG_UINT32 ui32FreePageCount,
                                 IMG_UINT32 *pai32FreeIndices,
                                 IMG_UINT32 uiSparseFlags)
{
	PVRSRV_ERROR eError;

	PMRLockPMR(psPMR);

	eError = PMR_ChangeSparseMemUnlocked(psPMR,
	                                     ui32AllocPageCount,
	                                     pai32AllocIndices,
	                                     ui32FreePageCount,
	                                     pai32FreeIndices,
	                                     uiSparseFlags);

	PMRUnlockPMR(psPMR);

	return eError;
}

PVRSRV_ERROR PMR_ChangeSparseMemUnlocked(PMR *psPMR,
                                 IMG_UINT32 ui32AllocPageCount,
                                 IMG_UINT32 *pai32AllocIndices,
                                 IMG_UINT32 ui32FreePageCount,
                                 IMG_UINT32 *pai32FreeIndices,
                                 IMG_UINT32 uiSparseFlags)
{
	PVRSRV_ERROR eError;
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	PMR_IMPL_ZOMBIEPAGES pvZombiePages = NULL;
	PMR_ZOMBIE_PAGES* psPMRZombiePages = NULL;
#endif

	if (PMR_IsMemLayoutFixed(psPMR) || PMR_IsClientCpuMapped(psPMR))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: This PMR layout cannot be changed - PMR_IsMemLayoutFixed()=%c, _PMR_IsClientCpuMapped()=%c",
				__func__,
				PMR_IsMemLayoutFixed(psPMR) ? 'Y' : 'n',
				PMR_IsClientCpuMapped(psPMR) ? 'Y' : 'n'));
		return PVRSRV_ERROR_PMR_NOT_PERMITTED;
	}

	if (NULL == psPMR->psFuncTab->pfnChangeSparseMem)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: This type of sparse PMR cannot be changed.",
				__func__));
		return PVRSRV_ERROR_NOT_IMPLEMENTED;
	}

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	if (uiSparseFlags & SPARSE_RESIZE_FREE)
	{
		/* Speculatively preallocate in order to simplify error handling later */
		psPMRZombiePages = OSAllocZMem(sizeof(PMR_ZOMBIE_PAGES));
		PVR_GOTO_IF_NOMEM(psPMRZombiePages, eError, returnErr);
	}
#endif

	eError = psPMR->psFuncTab->pfnChangeSparseMem(psPMR->pvFlavourData,
	                                              psPMR,
	                                              ui32AllocPageCount,
	                                              pai32AllocIndices,
	                                              ui32FreePageCount,
	                                              pai32FreeIndices,
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	                                              &pvZombiePages,
#endif
	                                              uiSparseFlags);
	if (eError != PVRSRV_OK)
	{
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
		if (eError == PVRSRV_ERROR_PMR_FAILED_TO_ALLOC_PAGES)
		{
			PVRSRVStatsUpdateOOMStat(NULL,
									  PMR_DeviceNode(psPMR),
									  PVRSRV_DEVICE_STAT_TYPE_OOM_PHYSMEM_COUNT,
									  OSGetCurrentClientProcessIDKM());
		}
#endif
		PVR_LOG_GOTO_ERROR(eError, "pfnChangeSparseMem", freePmrZombiePages);
	}

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	/* This PVR_ASSERT is only relevant when uiSparseFlags does not have the
	 * SPARSE_RESIZE_FREE bit set. */
	PVR_ASSERT((uiSparseFlags & SPARSE_RESIZE_FREE) ? IMG_TRUE : pvZombiePages == NULL);

	/* Call _ProcessZombiePage() to either queue the excess pages for deferred free or if
	 * no excess pages then free the pre-allocated psPMRZombiePages which is no longer needed.
	 * This explains why the error check/path after call goes to e0.
	 */
	eError = _ProcessZombiePages(pvZombiePages, psPMRZombiePages, psPMR);
	PVR_LOG_GOTO_IF_ERROR(eError, "_ProcessZombiePages", returnErr);
#endif

#if defined(PDUMP)
	{
		IMG_BOOL bInitialise = IMG_FALSE;
		IMG_UINT8 ui8InitValue = 0;

		if (PVRSRV_CHECK_ZERO_ON_ALLOC(PMR_Flags(psPMR)))
		{
			bInitialise = IMG_TRUE;
		}
		else if (PVRSRV_CHECK_POISON_ON_ALLOC(PMR_Flags(psPMR)))
		{
			ui8InitValue = (IMG_UINT8)PVRSRV_POISON_ON_ALLOC_VALUE;
			bInitialise = IMG_TRUE;
		}

		PDumpPMRChangeSparsePMR(psPMR,
		                        IMG_PAGE2BYTES32(psPMR->uiLog2ContiguityGuarantee),
		                        ui32AllocPageCount,
		                        pai32AllocIndices,
		                        ui32FreePageCount,
		                        pai32FreeIndices,
		                        bInitialise,
		                        ui8InitValue,
		                        &psPMR->hPDumpAllocHandle);
	}

#endif

	return PVRSRV_OK;

freePmrZombiePages:
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	if (uiSparseFlags & SPARSE_RESIZE_FREE)
	{
		OSFreeMem(psPMRZombiePages);
	}
returnErr:
#endif
	return eError;
}

#if defined(PDUMP)

static PVRSRV_ERROR
_PMR_PDumpSymbolicAddrPhysical(const PMR *psPMR,
                               IMG_DEVMEM_OFFSET_T uiPhysicalOffset,
                               IMG_UINT32 ui32MemspaceNameLen,
                               IMG_CHAR *pszMemspaceName,
                               IMG_UINT32 ui32SymbolicAddrLen,
                               IMG_CHAR *pszSymbolicAddr,
                               IMG_DEVMEM_OFFSET_T *puiNewOffset,
                               IMG_DEVMEM_OFFSET_T *puiNextSymName)
{
	PVRSRV_DEVICE_NODE *psDevNode = PhysHeapDeviceNode(psPMR->psPhysHeap);
	PVRSRV_ERROR eError = PVRSRV_OK;

#if defined(SUPPORT_SECURITY_VALIDATION)
	if (PVRSRV_CHECK_PHYS_HEAP(FW_CODE, psPMR->uiFlags) ||
		PVRSRV_CHECK_PHYS_HEAP(FW_PRIV_DATA, psPMR->uiFlags) ||
	    PVRSRV_CHECK_PHYS_HEAP(GPU_SECURE, psPMR->uiFlags))
	{
		OSSNPrintf(pszMemspaceName, ui32MemspaceNameLen, PMR_MEMSPACE_FMTSPEC,
		           psPMR->pszPDumpDefaultMemspaceName);
	}
	else
#endif
	if (DevmemDeviceCacheCoherency(psDevNode, psPMR->uiFlags))
	{
		OSSNPrintf(pszMemspaceName,
		           ui32MemspaceNameLen,
		           PMR_MEMSPACE_CACHE_COHERENT_FMTSPEC,
		           psPMR->pszPDumpDefaultMemspaceName);
	}
	else
	{
		OSSNPrintf(pszMemspaceName, ui32MemspaceNameLen, PMR_MEMSPACE_FMTSPEC,
		           psPMR->pszPDumpDefaultMemspaceName);
	}

	OSSNPrintf(pszSymbolicAddr,
	           ui32SymbolicAddrLen,
	           PMR_SYMBOLICADDR_FMTSPEC,
	           PMR_IsSparse(psPMR) ? PMR_SPARSE_PREFIX : PMR_DEFAULT_PREFIX,
	           psPMR->uiSerialNum,
	           uiPhysicalOffset >> PMR_GetLog2Contiguity(psPMR),
	           psPMR->szAnnotation);

	if (pszSymbolicAddr)
	{
		PDumpMakeStringValid(pszSymbolicAddr, OSStringLength(pszSymbolicAddr));
	}


	*puiNewOffset = uiPhysicalOffset & (IMG_PAGE2BYTES64(PMR_GetLog2Contiguity(psPMR))-1);
	*puiNextSymName = (IMG_DEVMEM_OFFSET_T) (((uiPhysicalOffset >> PMR_GetLog2Contiguity(psPMR))+1)
			<< PMR_GetLog2Contiguity(psPMR));

	return eError;
}


PVRSRV_ERROR
PMR_PDumpSymbolicAddr(const PMR *psPMR,
                      IMG_DEVMEM_OFFSET_T uiLogicalOffset,
                      IMG_UINT32 ui32MemspaceNameLen,
                      IMG_CHAR *pszMemspaceName,
                      IMG_UINT32 ui32SymbolicAddrLen,
                      IMG_CHAR *pszSymbolicAddr,
                      IMG_DEVMEM_OFFSET_T *puiNewOffset,
                      IMG_DEVMEM_OFFSET_T *puiNextSymName
)
{
	IMG_DEVMEM_OFFSET_T uiPhysicalOffset;
	IMG_UINT32 ui32Remain;
	IMG_BOOL bValid;
	PVRSRV_ERROR eError;

	PVR_ASSERT(uiLogicalOffset < psPMR->uiLogicalSize);

	/* Confirm that the device node's ui32InternalID matches the bound
	 * PDump device stored* in PVRSRV_DATA.
	 */
	if (!PDumpIsDevicePermitted(PMR_DeviceNode(psPMR)))
	{
		return PVRSRV_OK;
	}

	eError = _PMRLogicalOffsetToPhysicalOffset(psPMR,
	                                           0,
	                                           1,
	                                           uiLogicalOffset,
	                                           &uiPhysicalOffset,
	                                           &ui32Remain,
	                                           &bValid);
	PVR_LOG_RETURN_IF_ERROR(eError, "_PMRLogicalOffsetToPhysicalOffset");

	if (!bValid)
	{
		/* For sparse allocations, for a given logical address, there
		 * may not be a physical memory backing, the virtual range can
		 * still be valid.
		 */
		uiPhysicalOffset = uiLogicalOffset;
	}

	return _PMR_PDumpSymbolicAddrPhysical(psPMR,
	                                      uiPhysicalOffset,
	                                      ui32MemspaceNameLen,
	                                      pszMemspaceName,
	                                      ui32SymbolicAddrLen,
	                                      pszSymbolicAddr,
	                                      puiNewOffset,
	                                      puiNextSymName);
}

/*!
 * @brief Writes a WRW command to the script2 buffer, representing a
 *        dword write to a physical allocation. Size is always
 *        sizeof(IMG_UINT32).
 * @param psPMR - PMR object representing allocation
 * @param uiLogicalOffset - offset
 * @param ui32Value - value to write
 * @param uiPDumpFlags - pdump flags
 * @return PVRSRV_ERROR
 */
PVRSRV_ERROR
PMRPDumpLoadMemValue32(PMR *psPMR,
                       IMG_DEVMEM_OFFSET_T uiLogicalOffset,
                       IMG_UINT32 ui32Value,
                       PDUMP_FLAGS_T uiPDumpFlags)
{
	PVRSRV_ERROR eError;
	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiPDumpSymbolicOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName;
	IMG_UINT32 uiPMRPageSize = 1 << psPMR->uiLog2ContiguityGuarantee;

	/* Confirm that the device node's ui32InternalID matches the bound
	 * PDump device stored* in PVRSRV_DATA.
	 */
	if (!PDumpIsDevicePermitted(PMR_DeviceNode(psPMR)))
	{
		return PVRSRV_OK;
	}

	PVR_ASSERT(uiLogicalOffset + sizeof(ui32Value) <= psPMR->uiLogicalSize);
	/* Especially make sure to not cross a block boundary */
	PVR_ASSERT(( ((uiLogicalOffset & (uiPMRPageSize-1)) + sizeof(ui32Value))
			<= uiPMRPageSize));

	eError = PMRLockPhysAddresses(psPMR);
	PVR_ASSERT(eError == PVRSRV_OK);

	/* Get the symbolic address of the PMR */
	eError = PMR_PDumpSymbolicAddr(psPMR,
	                               uiLogicalOffset,
	                               sizeof(aszMemspaceName),
	                               &aszMemspaceName[0],
	                               sizeof(aszSymbolicName),
	                               &aszSymbolicName[0],
	                               &uiPDumpSymbolicOffset,
	                               &uiNextSymName);
	PVR_ASSERT(eError == PVRSRV_OK);

	/* Write the WRW script command */
	eError = PDumpPMRWRW32(PMR_DeviceNode(psPMR),
	                       aszMemspaceName,
	                       aszSymbolicName,
	                       uiPDumpSymbolicOffset,
	                       ui32Value,
	                       uiPDumpFlags);
	PVR_ASSERT(eError == PVRSRV_OK);

	eError = PMRUnlockPhysAddresses(psPMR);
	PVR_ASSERT(eError == PVRSRV_OK);

	return PVRSRV_OK;
}

/*!
 * @brief Writes a RDW followed by a WRW command to the pdump script to perform
 *        an effective copy from memory to memory. Memory copied is of size
 *        sizeof(IMG_UINT32)
 *
 * @param psDstPMR - PMR object representing allocation of destination
 * @param uiDstLogicalOffset - destination offset
 * @param psSrcPMR - PMR object representing allocation of source
 * @param uiSrcLogicalOffset - source offset
 * @param pszTmpVar - pdump temporary variable used during the copy
 * @param uiPDumpFlags - pdump flags
 * @return PVRSRV_ERROR
 */
PVRSRV_ERROR
PMRPDumpCopyMem32(PMR *psDstPMR,
                  IMG_DEVMEM_OFFSET_T uiDstLogicalOffset,
                  PMR *psSrcPMR,
                  IMG_DEVMEM_OFFSET_T uiSrcLogicalOffset,
                  const IMG_CHAR *pszTmpVar,
                  PDUMP_FLAGS_T uiPDumpFlags)
{
	PVRSRV_ERROR eError;
	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiPDumpSymbolicOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName;
	const IMG_UINT32 uiDstPMRPageSize = 1 << psDstPMR->uiLog2ContiguityGuarantee;
	const IMG_UINT32 uiSrcPMRPageSize = 1 << psSrcPMR->uiLog2ContiguityGuarantee;

	PVR_ASSERT(uiSrcLogicalOffset + sizeof(IMG_UINT32) <= psSrcPMR->uiLogicalSize);
	/* Especially make sure to not cross a block boundary */
	PVR_ASSERT(( ((uiSrcLogicalOffset & (uiSrcPMRPageSize-1)) + sizeof(IMG_UINT32))
			<= uiSrcPMRPageSize));

	PVR_ASSERT(uiDstLogicalOffset + sizeof(IMG_UINT32) <= psDstPMR->uiLogicalSize);
	/* Especially make sure to not cross a block boundary */
	PVR_ASSERT(( ((uiDstLogicalOffset & (uiDstPMRPageSize-1)) + sizeof(IMG_UINT32))
			<= uiDstPMRPageSize));

	eError = PMRLockPhysAddresses(psSrcPMR);
	PVR_ASSERT(eError == PVRSRV_OK);

	/* Get the symbolic address of the source PMR */
	eError = PMR_PDumpSymbolicAddr(psSrcPMR,
	                               uiSrcLogicalOffset,
	                               sizeof(aszMemspaceName),
	                               &aszMemspaceName[0],
	                               sizeof(aszSymbolicName),
	                               &aszSymbolicName[0],
	                               &uiPDumpSymbolicOffset,
	                               &uiNextSymName);
	PVR_ASSERT(eError == PVRSRV_OK);

	/* Issue PDump read command */
	eError = PDumpPMRRDW32MemToInternalVar(PMR_DeviceNode(psSrcPMR),
	                                       pszTmpVar,
	                                       aszMemspaceName,
	                                       aszSymbolicName,
	                                       uiPDumpSymbolicOffset,
	                                       uiPDumpFlags);
	PVR_ASSERT(eError == PVRSRV_OK);

	eError = PMRUnlockPhysAddresses(psSrcPMR);
	PVR_ASSERT(eError == PVRSRV_OK);



	eError = PMRLockPhysAddresses(psDstPMR);
	PVR_ASSERT(eError == PVRSRV_OK);


	/* Get the symbolic address of the destination PMR */
	eError = PMR_PDumpSymbolicAddr(psDstPMR,
	                               uiDstLogicalOffset,
	                               sizeof(aszMemspaceName),
	                               &aszMemspaceName[0],
	                               sizeof(aszSymbolicName),
	                               &aszSymbolicName[0],
	                               &uiPDumpSymbolicOffset,
	                               &uiNextSymName);
	PVR_ASSERT(eError == PVRSRV_OK);


	/* Write the WRW script command */
	eError = PDumpPMRWRW32InternalVarToMem(PMR_DeviceNode(psDstPMR),
	                                       aszMemspaceName,
	                                       aszSymbolicName,
	                                       uiPDumpSymbolicOffset,
	                                       pszTmpVar,
	                                       uiPDumpFlags);
	PVR_ASSERT(eError == PVRSRV_OK);


	eError = PMRUnlockPhysAddresses(psDstPMR);
	PVR_ASSERT(eError == PVRSRV_OK);

	return PVRSRV_OK;
}

/*!
 * @brief Writes a WRW64 command to the script2 buffer, representing a
 *        dword write to a physical allocation. Size is always
 *        sizeof(IMG_UINT64).
 * @param psPMR - PMR object representing allocation
 * @param uiLogicalOffset - offset
 * @param ui64Value - value to write
 * @param uiPDumpFlags - pdump flags
 * @return PVRSRV_ERROR
 */
PVRSRV_ERROR
PMRPDumpLoadMemValue64(PMR *psPMR,
                       IMG_DEVMEM_OFFSET_T uiLogicalOffset,
                       IMG_UINT64 ui64Value,
                       PDUMP_FLAGS_T uiPDumpFlags)
{
	PVRSRV_ERROR eError;
	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiPDumpSymbolicOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName;
	PMR_SIZE_T ui64PMRPageSize = IMG_PAGE2BYTES64(psPMR->uiLog2ContiguityGuarantee);

	/* Confirm that the device node's ui32InternalID matches the bound
	 * PDump device stored in PVRSRV_DATA.
	 */
	if (!PDumpIsDevicePermitted(PMR_DeviceNode(psPMR)))
	{
		return PVRSRV_OK;
	}

	PVR_ASSERT(uiLogicalOffset + sizeof(ui64Value) <= psPMR->uiLogicalSize);
	/* Especially make sure to not cross a block boundary */
	PVR_ASSERT(( ((uiLogicalOffset & (ui64PMRPageSize-1)) + sizeof(ui64Value))
			<= ui64PMRPageSize));

	eError = PMRLockPhysAddresses(psPMR);
	PVR_ASSERT(eError == PVRSRV_OK);

	/* Get the symbolic address of the PMR */
	eError = PMR_PDumpSymbolicAddr(psPMR,
	                               uiLogicalOffset,
	                               sizeof(aszMemspaceName),
	                               &aszMemspaceName[0],
	                               sizeof(aszSymbolicName),
	                               &aszSymbolicName[0],
	                               &uiPDumpSymbolicOffset,
	                               &uiNextSymName);
	PVR_ASSERT(eError == PVRSRV_OK);

	/* Write the WRW script command */
	eError = PDumpPMRWRW64(PMR_DeviceNode(psPMR),
	                       aszMemspaceName,
	                       aszSymbolicName,
	                       uiPDumpSymbolicOffset,
	                       ui64Value,
	                       uiPDumpFlags);
	PVR_ASSERT(eError == PVRSRV_OK);

	eError = PMRUnlockPhysAddresses(psPMR);
	PVR_ASSERT(eError == PVRSRV_OK);

	return PVRSRV_OK;
}

/*!
 * @brief Writes a RDW64 followed by a WRW64 command to the pdump script to
 *        perform an effective copy from memory to memory. Memory copied is of
 *        size sizeof(IMG_UINT32)
 *
 * @param psDstPMR - PMR object representing allocation of destination
 * @param uiDstLogicalOffset - destination offset
 * @param psSrcPMR - PMR object representing allocation of source
 * @param uiSrcLogicalOffset - source offset
 * @param pszTmpVar - pdump temporary variable used during the copy
 * @param uiPDumpFlags - pdump flags
 * @return PVRSRV_ERROR
 */
PVRSRV_ERROR
PMRPDumpCopyMem64(PMR *psDstPMR,
                  IMG_DEVMEM_OFFSET_T uiDstLogicalOffset,
                  PMR *psSrcPMR,
                  IMG_DEVMEM_OFFSET_T uiSrcLogicalOffset,
                  const IMG_CHAR *pszTmpVar,
                  PDUMP_FLAGS_T uiPDumpFlags)
{
	PVRSRV_ERROR eError;
	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiPDumpSymbolicOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName;
	const PMR_SIZE_T ui64DstPMRPageSize = IMG_PAGE2BYTES64(psDstPMR->uiLog2ContiguityGuarantee);
	const PMR_SIZE_T ui64SrcPMRPageSize = IMG_PAGE2BYTES64(psSrcPMR->uiLog2ContiguityGuarantee);

	PVR_ASSERT(uiSrcLogicalOffset + sizeof(IMG_UINT32) <= psSrcPMR->uiLogicalSize);
	/* Especially make sure to not cross a block boundary */
	PVR_ASSERT(( ((uiSrcLogicalOffset & (ui64SrcPMRPageSize-1)) + sizeof(IMG_UINT32))
			<= ui64SrcPMRPageSize));

	PVR_ASSERT(uiDstLogicalOffset + sizeof(IMG_UINT32) <= psDstPMR->uiLogicalSize);
	/* Especially make sure to not cross a block boundary */
	PVR_ASSERT(( ((uiDstLogicalOffset & (ui64DstPMRPageSize-1)) + sizeof(IMG_UINT32))
			<= ui64DstPMRPageSize));

	eError = PMRLockPhysAddresses(psSrcPMR);
	PVR_ASSERT(eError == PVRSRV_OK);

	/* Get the symbolic address of the source PMR */
	eError = PMR_PDumpSymbolicAddr(psSrcPMR,
	                               uiSrcLogicalOffset,
	                               sizeof(aszMemspaceName),
	                               &aszMemspaceName[0],
	                               sizeof(aszSymbolicName),
	                               &aszSymbolicName[0],
	                               &uiPDumpSymbolicOffset,
	                               &uiNextSymName);
	PVR_ASSERT(eError == PVRSRV_OK);

	/* Issue PDump read command */
	eError = PDumpPMRRDW64MemToInternalVar(PMR_DeviceNode(psSrcPMR),
	                                       pszTmpVar,
	                                       aszMemspaceName,
	                                       aszSymbolicName,
	                                       uiPDumpSymbolicOffset,
	                                       uiPDumpFlags);
	PVR_ASSERT(eError == PVRSRV_OK);

	eError = PMRUnlockPhysAddresses(psSrcPMR);
	PVR_ASSERT(eError == PVRSRV_OK);



	eError = PMRLockPhysAddresses(psDstPMR);
	PVR_ASSERT(eError == PVRSRV_OK);


	/* Get the symbolic address of the destination PMR */
	eError = PMR_PDumpSymbolicAddr(psDstPMR,
	                               uiDstLogicalOffset,
	                               sizeof(aszMemspaceName),
	                               &aszMemspaceName[0],
	                               sizeof(aszSymbolicName),
	                               &aszSymbolicName[0],
	                               &uiPDumpSymbolicOffset,
	                               &uiNextSymName);
	PVR_ASSERT(eError == PVRSRV_OK);


	/* Write the WRW script command */
	eError = PDumpPMRWRW64InternalVarToMem(PMR_DeviceNode(psDstPMR),
	                                       aszMemspaceName,
	                                       aszSymbolicName,
	                                       uiPDumpSymbolicOffset,
	                                       pszTmpVar,
	                                       uiPDumpFlags);
	PVR_ASSERT(eError == PVRSRV_OK);


	eError = PMRUnlockPhysAddresses(psDstPMR);
	PVR_ASSERT(eError == PVRSRV_OK);

	return PVRSRV_OK;
}

/*!
 * @brief PDumps the contents of the given allocation.
 * If bZero is IMG_TRUE then the zero page in the parameter stream is used
 * as the source of data, rather than the allocation's actual backing.
 * @param psPMR - PMR object representing allocation
 * @param uiLogicalOffset - Offset to write at
 * @param uiSize - Number of bytes to write
 * @param uiPDumpFlags - PDump flags
 * @param bZero - Use the PDump zero page as the source
 * @return PVRSRV_ERROR
 */
PVRSRV_ERROR
PMRPDumpLoadMem(PMR *psPMR,
                IMG_DEVMEM_OFFSET_T uiLogicalOffset,
                IMG_DEVMEM_SIZE_T uiSize,
                PDUMP_FLAGS_T uiPDumpFlags,
                IMG_BOOL bZero)
{
	PVRSRV_ERROR eError;
	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiOutOffset;
	IMG_DEVMEM_OFFSET_T uiCurrentOffset = uiLogicalOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName = 0;
	const IMG_CHAR *pszParamStreamFileName;
	PDUMP_FILEOFFSET_T uiParamStreamFileOffset;

	/* required when !bZero */
#define PMR_MAX_PDUMP_BUFSZ (1<<21)
	IMG_CHAR aszParamStreamFilename[PDUMP_PARAM_MAX_FILE_NAME];
	IMG_UINT8 *pcBuffer = NULL;
	size_t ui64BufSz;
	IMG_BOOL bValid;
	IMG_DEVMEM_SIZE_T uiSizeRemain = uiSize;
	PVRSRV_DEVICE_NODE *psDevNode = PMR_DeviceNode(psPMR);

	/* Confirm that the device node's ui32InternalID matches the bound
	 * PDump device stored* in PVRSRV_DATA.
	 */
	if (!PDumpIsDevicePermitted(psDevNode))
	{
		return PVRSRV_OK;
	}

	/* Forcibly initialise the name to a 'NULL' 0-length string */
	aszParamStreamFilename[0] = '\0';

	PVR_ASSERT(uiLogicalOffset + uiSize <= psPMR->uiLogicalSize);

	/* Check if pdump client is connected */
	if (!PDumpCheckFlagsWrite(psDevNode,
		                      PDUMP_FLAGS_CONTINUOUS))
	{
		/* Dumping of memory in Pdump buffer will be rejected for no client connected case.
		 * So return early and save reading of data from PMR. */
		return PVRSRV_OK;
	}

	/* Get the correct PDump stream file name */
	if (bZero)
	{
		PDumpCommentWithFlags(psDevNode,
		                      uiPDumpFlags,
		                      "Zeroing allocation (" IMG_DEVMEM_SIZE_FMTSPEC " bytes)",
		                      uiSize);

		/* get the zero page information. it is constant for this function */
		PDumpGetParameterZeroPageInfo(&uiParamStreamFileOffset,
		                              &ui64BufSz,
		                              &pszParamStreamFileName);
	}
	else
	{

		ui64BufSz = IMG_PAGE2BYTES64(PMR_GetLog2Contiguity(psPMR));
		PVR_ASSERT(IMG_PAGE2BYTES64(PMR_GetLog2Contiguity(psPMR)) <= PMR_MAX_PDUMP_BUFSZ);

		pcBuffer = OSAllocMem(ui64BufSz);

		PVR_LOG_RETURN_IF_NOMEM(pcBuffer, "OSAllocMem");

		eError = PMRLockPhysAddresses(psPMR);
		PVR_ASSERT(eError == PVRSRV_OK);

		pszParamStreamFileName = aszParamStreamFilename;
	}

	/* Loop over all touched symbolic addresses of the PMR and
	 * emit LDBs to load the contents. */
	while (uiCurrentOffset < (uiLogicalOffset + uiSize))
	{
		/* Get the correct symbolic name for the current offset */
		eError = PMR_PDumpSymbolicAddr(psPMR,
		                               uiCurrentOffset,
		                               sizeof(aszMemspaceName),
		                               &aszMemspaceName[0],
		                               sizeof(aszSymbolicName),
		                               &aszSymbolicName[0],
		                               &uiOutOffset,
		                               &uiNextSymName);
		PVR_ASSERT(eError == PVRSRV_OK);
		PVR_ASSERT((uiNextSymName - uiCurrentOffset) <= ui64BufSz);

		PMR_IsOffsetValid(psPMR,
		                  0,
		                  1,
		                  uiCurrentOffset,
		                  &bValid);

		/* Either just LDB the zeros or read from the PMR and store that
		 * in the pdump stream */
		if (bValid)
		{
			size_t uiNumBytes;
			IMG_BOOL bOk2Write = IMG_TRUE;

			if (bZero)
			{
				uiNumBytes = MIN(uiSizeRemain, uiNextSymName - uiCurrentOffset);
			}
			else
			{
				IMG_DEVMEM_OFFSET_T uiReadOffset;
				uiReadOffset = ((uiNextSymName > (uiLogicalOffset + uiSize)) ?
						uiLogicalOffset + uiSize - uiCurrentOffset :
						uiNextSymName - uiCurrentOffset);

				eError = PMR_ReadBytes(psPMR,
				                       uiCurrentOffset,
				                       pcBuffer,
				                       uiReadOffset,
				                       &uiNumBytes);
				PVR_ASSERT(eError == PVRSRV_OK);

				eError = PDumpWriteParameterBlob(psDevNode,
				                          pcBuffer,
				                          uiNumBytes,
				                          uiPDumpFlags,
				                          &aszParamStreamFilename[0],
				                          sizeof(aszParamStreamFilename),
				                          &uiParamStreamFileOffset);

				if (eError == PVRSRV_ERROR_PDUMP_NOT_ALLOWED)
				{
					/* Write to parameter file prevented under the flags and
					 * current state of the driver so skip further writes.
					 */
					eError = PVRSRV_OK;
					bOk2Write = IMG_FALSE;	/* Do *NOT* write anything */
				}
				else if (eError != PVRSRV_OK)
				{
					PDUMP_ERROR(psDevNode,
					            eError, "Failed to write PMR memory to parameter file");
					bOk2Write = IMG_FALSE;	/* Do *NOT* write anything */
				}
			}

			if (bOk2Write)
			{
				/* Emit the LDB command to the current symbolic address */
				eError = PDumpPMRLDB(psDevNode,
				                     aszMemspaceName,
				                     aszSymbolicName,
				                     uiOutOffset,
				                     uiNumBytes,
				                     pszParamStreamFileName,
				                     uiParamStreamFileOffset,
				                     uiPDumpFlags);
			}
			uiSizeRemain = uiSizeRemain - uiNumBytes;
		}
		uiCurrentOffset = uiNextSymName;
	}

	if (!bZero)
	{
		eError = PMRUnlockPhysAddresses(psPMR);
		PVR_ASSERT(eError == PVRSRV_OK);

		OSFreeMem(pcBuffer);
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR
PMRPDumpSaveToFile(const PMR *psPMR,
                   IMG_DEVMEM_OFFSET_T uiLogicalOffset,
                   IMG_DEVMEM_SIZE_T uiSize,
                   IMG_UINT32 uiArraySize,
                   const IMG_CHAR *pszFilename,
                   IMG_UINT32 uiFileOffset)
{
	return PMRPDumpSaveToFileWFlags(psPMR,
	                                uiLogicalOffset,
	                                uiSize,
	                                uiArraySize,
	                                pszFilename,
	                                uiFileOffset,
	                                0);
}

PVRSRV_ERROR
PMRPDumpSaveToFileWFlags(const PMR *psPMR,
                         IMG_DEVMEM_OFFSET_T uiLogicalOffset,
                         IMG_DEVMEM_SIZE_T uiSize,
                         IMG_UINT32 uiArraySize,
                         const IMG_CHAR *pszFilename,
                         IMG_UINT32 uiFileOffset,
                         IMG_UINT32 uiPDumpFlags)
{
	PVRSRV_ERROR eError;
	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiOutOffset;
	IMG_DEVMEM_OFFSET_T uiCurrentOffset = uiLogicalOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName = 0;
	IMG_UINT32 uiCurrentFileOffset = uiFileOffset;

	PVR_UNREFERENCED_PARAMETER(uiArraySize);

	/* Confirm that the device node's ui32InternalID matches the bound
	 * PDump device stored* in PVRSRV_DATA.
	 */
	if (!PDumpIsDevicePermitted(PMR_DeviceNode(psPMR)))
	{
		return PVRSRV_OK;
	}

	PVR_ASSERT(uiLogicalOffset + uiSize <= psPMR->uiLogicalSize);

	while (uiCurrentOffset < (uiLogicalOffset + uiSize))
	{
		IMG_DEVMEM_OFFSET_T uiReadOffset;

		eError = PMR_PDumpSymbolicAddr(psPMR,
		                               uiCurrentOffset,
		                               sizeof(aszMemspaceName),
		                               &aszMemspaceName[0],
		                               sizeof(aszSymbolicName),
		                               &aszSymbolicName[0],
		                               &uiOutOffset,
		                               &uiNextSymName);
		PVR_ASSERT(eError == PVRSRV_OK);
		PVR_ASSERT(uiNextSymName <= psPMR->uiLogicalSize);

		uiReadOffset = ((uiNextSymName > (uiLogicalOffset + uiSize)) ?
				uiLogicalOffset + uiSize - uiCurrentOffset :
				uiNextSymName - uiCurrentOffset);

		eError = PDumpPMRSAB(PMR_DeviceNode(psPMR),
		                     aszMemspaceName,
		                     aszSymbolicName,
		                     uiOutOffset,
		                     uiReadOffset,
		                     pszFilename,
		                     uiCurrentFileOffset,
		                     uiPDumpFlags);
		PVR_ASSERT(eError == PVRSRV_OK);

		uiCurrentFileOffset += uiNextSymName - uiCurrentOffset;
		uiCurrentOffset = uiNextSymName;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR
PMRPDumpPol32(const PMR *psPMR,
              IMG_DEVMEM_OFFSET_T uiLogicalOffset,
              IMG_UINT32 ui32Value,
              IMG_UINT32 ui32Mask,
              PDUMP_POLL_OPERATOR eOperator,
              PDUMP_FLAGS_T uiPDumpFlags)
{
	PVRSRV_ERROR eError;
	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiPDumpOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName;
	IMG_UINT32 uiPMRPageSize = 1 << psPMR->uiLog2ContiguityGuarantee;

	/* Confirm that the device node's ui32InternalID matches the bound
	 * PDump device stored* in PVRSRV_DATA.
	 */
	if (!PDumpIsDevicePermitted(PMR_DeviceNode(psPMR)))
	{
		return PVRSRV_OK;
	}

	/* Make sure to not cross a block boundary */
	PVR_ASSERT(( ((uiLogicalOffset & (uiPMRPageSize-1)) + sizeof(ui32Value))
			<= uiPMRPageSize));

	eError = PMR_PDumpSymbolicAddr(psPMR,
	                               uiLogicalOffset,
	                               sizeof(aszMemspaceName),
	                               &aszMemspaceName[0],
	                               sizeof(aszSymbolicName),
	                               &aszSymbolicName[0],
	                               &uiPDumpOffset,
	                               &uiNextSymName);
	PVR_GOTO_IF_ERROR(eError, e0);

#define _MEMPOLL_DELAY		(1000)
#define _MEMPOLL_COUNT		(2000000000 / _MEMPOLL_DELAY)

	eError = PDumpPMRPOL(PMR_DeviceNode(psPMR),
	                     aszMemspaceName,
	                     aszSymbolicName,
	                     uiPDumpOffset,
	                     ui32Value,
	                     ui32Mask,
	                     eOperator,
	                     _MEMPOLL_COUNT,
	                     _MEMPOLL_DELAY,
	                     uiPDumpFlags);
	PVR_GOTO_IF_ERROR(eError, e0);

	return PVRSRV_OK;

	/* Error exit paths follow */
e0:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

PVRSRV_ERROR
PMRPDumpCheck32(const PMR *psPMR,
				IMG_DEVMEM_OFFSET_T uiLogicalOffset,
				IMG_UINT32 ui32Value,
				IMG_UINT32 ui32Mask,
				PDUMP_POLL_OPERATOR eOperator,
				PDUMP_FLAGS_T uiPDumpFlags)
{
	PVRSRV_ERROR eError;
	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiPDumpOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName;
	IMG_UINT32 uiPMRPageSize = 1 << psPMR->uiLog2ContiguityGuarantee;

	/* Confirm that the device node's ui32InternalID matches the bound
	 * PDump device stored* in PVRSRV_DATA.
	 */
	if (!PDumpIsDevicePermitted(PMR_DeviceNode(psPMR)))
	{
		return PVRSRV_OK;
	}

	/* Make sure to not cross a block boundary */
	PVR_ASSERT(( ((uiLogicalOffset & (uiPMRPageSize-1)) + sizeof(ui32Value))
	           < uiPMRPageSize));

	eError = PMR_PDumpSymbolicAddr(psPMR,
	                               uiLogicalOffset,
	                               sizeof(aszMemspaceName),
	                               &aszMemspaceName[0],
	                               sizeof(aszSymbolicName),
	                               &aszSymbolicName[0],
	                               &uiPDumpOffset,
	                               &uiNextSymName);
	if (eError != PVRSRV_OK)
	{
		goto e0;
	}

	eError = PDumpPMRPOL(PMR_DeviceNode(psPMR),
	                     aszMemspaceName,
	                     aszSymbolicName,
	                     uiPDumpOffset,
	                     ui32Value,
	                     ui32Mask,
	                     eOperator,
	                     1,
	                     1,
	                     uiPDumpFlags);
	if (eError != PVRSRV_OK)
	{
		goto e0;
	}

	return PVRSRV_OK;

	/* Error exit paths follow */
e0:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

PVRSRV_ERROR
PMRPDumpCBP(const PMR *psPMR,
            IMG_DEVMEM_OFFSET_T uiReadOffset,
            IMG_DEVMEM_OFFSET_T uiWriteOffset,
            IMG_DEVMEM_SIZE_T uiPacketSize,
            IMG_DEVMEM_SIZE_T uiBufferSize)
{
	PVRSRV_ERROR eError;
	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiPDumpOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName;

	/* Confirm that the device node's ui32InternalID matches the bound
	 * PDump device stored* in PVRSRV_DATA.
	 */
	if (!PDumpIsDevicePermitted(PMR_DeviceNode(psPMR)))
	{
		return PVRSRV_OK;
	}

	eError = PMR_PDumpSymbolicAddr(psPMR,
	                               uiReadOffset,
	                               sizeof(aszMemspaceName),
	                               &aszMemspaceName[0],
	                               sizeof(aszSymbolicName),
	                               &aszSymbolicName[0],
	                               &uiPDumpOffset,
	                               &uiNextSymName);
	PVR_GOTO_IF_ERROR(eError, e0);

	eError = PDumpPMRCBP(PMR_DeviceNode(psPMR),
	                     aszMemspaceName,
	                     aszSymbolicName,
	                     uiPDumpOffset,
	                     uiWriteOffset,
	                     uiPacketSize,
	                     uiBufferSize);
	PVR_GOTO_IF_ERROR(eError, e0);

	return PVRSRV_OK;

	/* Error exit paths follow */
e0:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

static void
PDumpPMRChangeSparsePMR(PMR *psPMR,
                        IMG_UINT32 uiBlockSize,
                        IMG_UINT32 ui32AllocPageCount,
                        IMG_UINT32 *pai32AllocIndices,
                        IMG_UINT32 ui32FreePageCount,
                        IMG_UINT32 *pai32FreeIndices,
                        IMG_BOOL bInitialise,
                        IMG_UINT8 ui8InitValue,
                        IMG_HANDLE *phPDumpAllocInfoOut)
{
	PVRSRV_ERROR eError;
	IMG_HANDLE *phPDumpAllocInfo = (IMG_HANDLE*) psPMR->hPDumpAllocHandle;

	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName;
	IMG_UINT32 i, uiIndex;
	PVRSRV_DEVICE_NODE *psDevNode = PMR_DeviceNode(psPMR);

	/* Remove pages from the PMR */
	for (i = 0; i < ui32FreePageCount; i++)
	{
		uiIndex = pai32FreeIndices[i];

		eError = PDumpFree(psDevNode,
		                   phPDumpAllocInfo[uiIndex]);
		PVR_ASSERT(eError == PVRSRV_OK);
		phPDumpAllocInfo[uiIndex] = NULL;
	}

	/* Add new pages to the PMR */
	for (i = 0; i < ui32AllocPageCount; i++)
	{
		uiIndex = pai32AllocIndices[i];

		PVR_ASSERT(phPDumpAllocInfo[uiIndex] == NULL);

		eError = PMR_PDumpSymbolicAddr(psPMR,
		                               uiIndex * uiBlockSize,
		                               sizeof(aszMemspaceName),
		                               &aszMemspaceName[0],
		                               sizeof(aszSymbolicName),
		                               &aszSymbolicName[0],
		                               &uiOffset,
		                               &uiNextSymName);
		PVR_ASSERT(eError == PVRSRV_OK);

		eError = PDumpMalloc(psDevNode,
		                     aszMemspaceName,
		                     aszSymbolicName,
		                     uiBlockSize,
		                     uiBlockSize,
		                     bInitialise,
		                     ui8InitValue,
		                     &phPDumpAllocInfo[uiIndex],
		                     PDUMP_NONE);
		PVR_ASSERT(eError == PVRSRV_OK);
	}

	/* (IMG_HANDLE) <- (IMG_HANDLE*) */
	*phPDumpAllocInfoOut = (IMG_HANDLE) phPDumpAllocInfo;
}

static void
PDumpPMRFreePMR(PMR *psPMR,
                IMG_DEVMEM_SIZE_T uiSize,
                IMG_DEVMEM_ALIGN_T uiBlockSize,
                IMG_UINT32 uiLog2Contiguity,
                IMG_HANDLE hPDumpAllocationInfoHandle)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 i;

	/* (IMG_HANDLE*) <- (IMG_HANDLE) */
	IMG_HANDLE *ahPDumpAllocHandleArray = (IMG_HANDLE*) hPDumpAllocationInfoHandle;

	PDUMP_LOCK(PDUMP_FLAGS_NONE);

	for (i = 0; i < psPMR->uiNumPDumpBlocks; i++)
	{
		if (ahPDumpAllocHandleArray[i] != NULL)
		{
			eError = PDumpFreeUnlocked(PMR_DeviceNode(psPMR),
			                           ahPDumpAllocHandleArray[i]);
			PVR_ASSERT(eError == PVRSRV_OK);
			ahPDumpAllocHandleArray[i] = NULL;
		}
	}

	PDUMP_UNLOCK(PDUMP_FLAGS_NONE);
	OSFreeMem(ahPDumpAllocHandleArray);
}

static void
PDumpPMRMallocPMR(PMR *psPMR,
                  IMG_DEVMEM_SIZE_T uiSize,
                  IMG_UINT32 ui32ChunkSize,
                  IMG_UINT32 ui32NumPhysChunks,
                  IMG_UINT32 ui32NumLogicalChunks,
                  IMG_UINT32 *puiMappingTable,
                  IMG_UINT32 uiLog2Contiguity,
                  IMG_BOOL bInitialise,
                  IMG_UINT8 ui8InitValue,
                  IMG_HANDLE *phPDumpAllocInfoOut,
                  IMG_UINT32 ui32PDumpFlags)
{
	PVRSRV_ERROR eError;
	IMG_HANDLE *phPDumpAllocInfo;

	IMG_CHAR aszMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName;
	IMG_UINT32 uiNumPhysBlocks;
	IMG_UINT32 uiNumVirtBlocks;
	IMG_UINT32 i, uiIndex;

	if (PMR_IsSparse(psPMR))
	{
		uiNumPhysBlocks = (ui32ChunkSize * ui32NumPhysChunks) >> uiLog2Contiguity;
		/* Make sure we did not cut off anything */
		PVR_ASSERT(uiNumPhysBlocks << uiLog2Contiguity == (ui32ChunkSize * ui32NumPhysChunks));
	}
	else
	{
		uiNumPhysBlocks = uiSize >> uiLog2Contiguity;
		/* Make sure we did not cut off anything */
		PVR_ASSERT(uiNumPhysBlocks << uiLog2Contiguity == uiSize);
	}

	uiNumVirtBlocks = uiSize >> uiLog2Contiguity;
	PVR_ASSERT(uiNumVirtBlocks << uiLog2Contiguity == uiSize);

	psPMR->uiNumPDumpBlocks = uiNumVirtBlocks;

	phPDumpAllocInfo = (IMG_HANDLE*) OSAllocZMem(uiNumVirtBlocks * sizeof(IMG_HANDLE));

	PDUMP_LOCK(ui32PDumpFlags);

	for (i = 0; i < uiNumPhysBlocks; i++)
	{
		uiIndex = PMR_IsSparse(psPMR) ? puiMappingTable[i] : i;

		eError = PMR_PDumpSymbolicAddr(psPMR,
		                               uiIndex * ui32ChunkSize,
		                               sizeof(aszMemspaceName),
		                               &aszMemspaceName[0],
		                               sizeof(aszSymbolicName),
		                               &aszSymbolicName[0],
		                               &uiOffset,
		                               &uiNextSymName);
		PVR_ASSERT(eError == PVRSRV_OK);

		eError = PDumpMallocUnlocked(PMR_DeviceNode(psPMR),
		                             aszMemspaceName,
		                             aszSymbolicName,
		                             ui32ChunkSize,
		                             ui32ChunkSize,
		                             bInitialise,
		                             ui8InitValue,
		                             &phPDumpAllocInfo[uiIndex],
		                             ui32PDumpFlags);
		PVR_LOG_RETURN_VOID_IF_FALSE((eError != PVRSRV_ERROR_PDUMP_CAPTURE_BOUND_TO_ANOTHER_DEVICE),
		                             "PDumpPMRMalloc PDump capture bound to other device");
		PVR_ASSERT(eError == PVRSRV_OK);
	}

	PDUMP_UNLOCK(ui32PDumpFlags);

	/* (IMG_HANDLE) <- (IMG_HANDLE*) */
	*phPDumpAllocInfoOut = (IMG_HANDLE) phPDumpAllocInfo;

}
#endif	/* PDUMP */


void *PMRGetPrivateData(const PMR *psPMR,
                        const PMR_IMPL_FUNCTAB *psFuncTab)
{
	return (psFuncTab == psPMR->psFuncTab) ? psPMR->pvFlavourData : NULL;
}

#define PMR_PM_WORD_SIZE 4

PVRSRV_ERROR
PMRWritePMPageList(/* Target PMR, offset, and length */
		PMR *psPageListPMR,
		IMG_DEVMEM_OFFSET_T uiTableOffset,
		IMG_DEVMEM_SIZE_T  uiTableLength,
		/* Referenced PMR, and "page" granularity */
		PMR *psReferencePMR,
		IMG_DEVMEM_LOG2ALIGN_T uiLog2PageSize,
		PMR_PAGELIST **ppsPageList)
{
	PVRSRV_ERROR eError;
	IMG_DEVMEM_SIZE_T uiWordSize;
	IMG_UINT32 uiNumPages;
	IMG_UINT32 uiPageIndex;
	PMR_PAGELIST *psPageList;
#if defined(PDUMP)
	IMG_CHAR aszTableEntryMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszTableEntrySymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiTableEntryPDumpOffset;
	IMG_CHAR aszPageMemspaceName[PHYSMEM_PDUMP_MEMSPACE_MAX_LENGTH];
	IMG_CHAR aszPageSymbolicName[PHYSMEM_PDUMP_SYMNAME_MAX_LENGTH];
	IMG_DEVMEM_OFFSET_T uiPagePDumpOffset;
	IMG_DEVMEM_OFFSET_T uiNextSymName;
#endif
#if !defined(NO_HARDWARE)
	PMR_SIZE_T ui64PageListPageSize = IMG_PAGE2BYTES64(psPageListPMR->uiLog2ContiguityGuarantee);
	IMG_UINT32 uiPageBase = 0;
	IMG_DEV_PHYADDR asDevPAddr[PMR_MAX_TRANSLATION_STACK_ALLOC];
	IMG_UINT32 aui32AddrShifted[PMR_MAX_TRANSLATION_STACK_ALLOC];
	IMG_BOOL abValid[PMR_MAX_TRANSLATION_STACK_ALLOC];
	IMG_DEV_PHYADDR *pasDevAddrPtr;
	IMG_UINT32 *paui32AddrShifted;
	IMG_BOOL *pbPageIsValid;
	IMG_BOOL bUseAlloc;
#endif

	uiWordSize = PMR_PM_WORD_SIZE;

	/* check we're being asked to write the same number of 4-byte units as there are pages */
	uiNumPages = (IMG_UINT32)(psReferencePMR->uiLogicalSize >> uiLog2PageSize);

	if ((PMR_SIZE_T)uiNumPages << uiLog2PageSize != psReferencePMR->uiLogicalSize)
	{
		/* Strictly speaking, it's possible to provoke this error in two ways:
			(i) if it's not a whole multiple of the page size; or
			(ii) if there are more than 4 billion pages.
			The latter is unlikely. :) but the check is required in order to justify the cast.
		 */
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_NOT_PAGE_MULTIPLE, return_error);
	}
	uiWordSize = (IMG_UINT32)uiTableLength / uiNumPages;
	if (uiNumPages * uiWordSize != uiTableLength)
	{
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_NOT_PAGE_MULTIPLE, return_error);
	}

	/* Check for integer overflow */
	PVR_GOTO_IF_INVALID_PARAM(uiTableOffset + uiTableLength > uiTableOffset, eError, return_error);
	/* Check we're not being asked to write off the end of the PMR */
	PVR_GOTO_IF_INVALID_PARAM(uiTableOffset + uiTableLength <= psPageListPMR->uiLogicalSize, eError, return_error);

	if (_PMRIsSparse(psReferencePMR))
	{
		PVR_LOG_GOTO_WITH_ERROR("psReferencePMR", eError, PVRSRV_ERROR_INVALID_PARAMS, return_error);
	}

	psPageList = OSAllocMem(sizeof(PMR_PAGELIST));
	PVR_LOG_GOTO_IF_NOMEM(psPageList, eError, return_error);

	psPageList->psReferencePMR = psReferencePMR;

	/* Need to lock down the physical addresses of the reference PMR */
	/* N.B.  This also checks that the requested "contiguity" is achievable */
	eError = PMRLockPhysAddresses(psReferencePMR);
	PVR_GOTO_IF_ERROR(eError, free_page_list);

#if !defined(NO_HARDWARE)
	if (uiNumPages > PMR_MAX_TRANSLATION_STACK_ALLOC)
	{
		bUseAlloc = IMG_TRUE;

		pasDevAddrPtr = OSAllocMem(uiNumPages * sizeof(IMG_DEV_PHYADDR));
		PVR_LOG_GOTO_IF_NOMEM(pasDevAddrPtr, eError, unlock_phys_addrs);

		paui32AddrShifted = OSAllocMem(uiNumPages * sizeof(IMG_UINT32));
		PVR_LOG_GOTO_IF_NOMEM(paui32AddrShifted, eError, free_devaddr_array);

		pbPageIsValid = OSAllocMem(uiNumPages * sizeof(IMG_BOOL));
		PVR_LOG_GOTO_IF_NOMEM(pbPageIsValid, eError, free_addrshift_array);
	}
	else
	{
		bUseAlloc = IMG_FALSE;

		pasDevAddrPtr = asDevPAddr;
		paui32AddrShifted = aui32AddrShifted;
		pbPageIsValid = abValid;
	}

	eError = PMR_DevPhysAddr(psReferencePMR, uiLog2PageSize, uiNumPages, 0,
	                         pasDevAddrPtr, pbPageIsValid, DEVICE_USE);
	PVR_LOG_GOTO_IF_ERROR(eError, "PMR_DevPhysAddr", free_valid_array);
#endif

	for (uiPageIndex = 0; uiPageIndex < uiNumPages; uiPageIndex++)
	{
		IMG_DEVMEM_OFFSET_T uiPMROffset = uiTableOffset + (uiWordSize * uiPageIndex);
#if !defined(NO_HARDWARE)
		IMG_UINT64 uiNextPageListPMRPage = 0;
		IMG_UINT64 uiPageListPMRPage = 0;
#endif

#if defined(PDUMP)
		eError = PMR_PDumpSymbolicAddr(psPageListPMR,
		                               uiPMROffset,
		                               sizeof(aszTableEntryMemspaceName),
		                               &aszTableEntryMemspaceName[0],
		                               sizeof(aszTableEntrySymbolicName),
		                               &aszTableEntrySymbolicName[0],
		                               &uiTableEntryPDumpOffset,
		                               &uiNextSymName);
		PVR_ASSERT(eError == PVRSRV_OK);

		eError = PMR_PDumpSymbolicAddr(psReferencePMR,
		                               IMG_PAGES2BYTES64(uiPageIndex, uiLog2PageSize),
		                               sizeof(aszPageMemspaceName),
		                               &aszPageMemspaceName[0],
		                               sizeof(aszPageSymbolicName),
		                               &aszPageSymbolicName[0],
		                               &uiPagePDumpOffset,
		                               &uiNextSymName);
		PVR_ASSERT(eError == PVRSRV_OK);

		eError = PDumpWriteShiftedMaskedValue(PMR_DeviceNode(psReferencePMR),
		                                      /* destination */
		                                      aszTableEntryMemspaceName,
		                                      aszTableEntrySymbolicName,
		                                      uiTableEntryPDumpOffset,
		                                      /* source */
		                                      aszPageMemspaceName,
		                                      aszPageSymbolicName,
		                                      uiPagePDumpOffset,
		                                      /* shift right */
		                                      uiLog2PageSize,
		                                      /* shift left */
		                                      0,
		                                      /* mask */
		                                      0xffffffff,
		                                      /* word size */
		                                      uiWordSize,
		                                      /* flags */
		                                      PDUMP_FLAGS_CONTINUOUS);
		PVR_ASSERT(eError == PVRSRV_OK);
#else
		PVR_UNREFERENCED_PARAMETER(uiPMROffset);
#endif

#if !defined(NO_HARDWARE)

		/*
			We check for sparse PMR's at function entry, but as we can,
			check that every page is valid
		 */
		PVR_ASSERT(pbPageIsValid[uiPageIndex]);
		PVR_ASSERT(pasDevAddrPtr[uiPageIndex].uiAddr != 0);
		PVR_ASSERT(((pasDevAddrPtr[uiPageIndex].uiAddr >> uiLog2PageSize) & 0xFFFFFFFF00000000LL) == 0);

		uiPageListPMRPage = uiPMROffset >> psReferencePMR->uiLog2ContiguityGuarantee;
		uiNextPageListPMRPage = (uiPMROffset + uiWordSize) >> psReferencePMR->uiLog2ContiguityGuarantee;

		paui32AddrShifted[uiPageIndex] = TRUNCATE_64BITS_TO_32BITS(pasDevAddrPtr[uiPageIndex].uiAddr >> uiLog2PageSize);

		/* Last page or new page so memcpy */
		if (uiPageIndex == (uiNumPages - 1) || (uiNextPageListPMRPage != uiPageListPMRPage))
		{
			IMG_DEVMEM_OFFSET_T uiPMROffsetBase = uiTableOffset + (uiWordSize * uiPageBase);
			size_t uiMappingOffset = uiPMROffsetBase & (~(ui64PageListPageSize - 1));
			void *pvKernAddr = NULL;
			size_t uiMappedSize;
			IMG_HANDLE hPrivData = NULL;
			IMG_UINT32 *pui32Src;
			IMG_UINT32 *pui32Dst;
			IMG_UINT32 ui32Copysize;

			eError = PMRAcquireKernelMappingData(psPageListPMR,
			                                     uiMappingOffset,
			                                     ui64PageListPageSize,
			                                     &pvKernAddr,
			                                     &uiMappedSize,
			                                     &hPrivData);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "Error mapping page list PMR page (%" IMG_UINT64_FMTSPEC ") into kernel (%d)",
						uiPageListPMRPage, eError));
				goto free_valid_array;
			}

			PVR_ASSERT(uiMappedSize >= ui64PageListPageSize);
			PVR_ASSERT(pvKernAddr != NULL);

			pui32Src = paui32AddrShifted + uiPageBase;
			pui32Dst = IMG_OFFSET_ADDR(pvKernAddr, (uiPMROffsetBase & (ui64PageListPageSize - 1)));
			ui32Copysize = ((uiPageIndex + 1) - uiPageBase) * sizeof(IMG_UINT32);

			OSCachedMemCopy(pui32Dst, pui32Src, ui32Copysize);

			PMRReleaseKernelMappingData(psPageListPMR, hPrivData);

			uiPageBase = uiPageIndex + 1;
		}
#endif
	}

	OSWriteMemoryBarrier(NULL);

#if !defined(NO_HARDWARE)
	if (bUseAlloc)
	{
		OSFreeMem(pbPageIsValid);
		OSFreeMem(paui32AddrShifted);
		OSFreeMem(pasDevAddrPtr);
	}
#endif
	*ppsPageList = psPageList;
	return PVRSRV_OK;

	/* Error exit paths follow */
#if !defined(NO_HARDWARE)

free_valid_array:
	if (bUseAlloc)
	{
		OSFreeMem(pbPageIsValid);
	}

free_addrshift_array:
	if (bUseAlloc)
	{
		OSFreeMem(paui32AddrShifted);
	}

free_devaddr_array:
	if (bUseAlloc)
	{
		OSFreeMem(pasDevAddrPtr);
	}

unlock_phys_addrs:
	PMRUnlockPhysAddresses(psReferencePMR);
#endif

free_page_list:
	OSFreeMem(psPageList);

return_error:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}


PVRSRV_ERROR
PMRUnwritePMPageList(PMR_PAGELIST *psPageList)
{
	PVRSRV_ERROR eError;

	eError = PMRUnlockPhysAddresses(psPageList->psReferencePMR);
	PVR_ASSERT(eError == PVRSRV_OK);
	OSFreeMem(psPageList);

	return PVRSRV_OK;
}

PVRSRV_ERROR
PMRZeroingPMR(PMR *psPMR,
              IMG_DEVMEM_LOG2ALIGN_T uiLog2PageSize)
{
	IMG_UINT32 uiNumPages;
	IMG_UINT32 uiPageIndex;
	PMR_SIZE_T ui64PageSize = IMG_PAGE2BYTES64(uiLog2PageSize);
	IMG_HANDLE hPrivData = NULL;
	void *pvKernAddr = NULL;
	PVRSRV_ERROR eError = PVRSRV_OK;
	size_t uiMappedSize;

	PVR_ASSERT(psPMR);

	/* Calculate number of pages in this PMR */
	uiNumPages = (IMG_UINT32)(psPMR->uiLogicalSize >> uiLog2PageSize);

	/* Verify the logical Size is a multiple or the physical page size */
	if ((PMR_SIZE_T)uiNumPages << uiLog2PageSize != psPMR->uiLogicalSize)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: PMR is not a multiple of %" IMG_UINT64_FMTSPEC,
		         __func__,
		         ui64PageSize));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_NOT_PAGE_MULTIPLE, MultiPage_Error);
	}

	if (_PMRIsSparse(psPMR))
	{
		PVR_LOG_GOTO_WITH_ERROR("psPMR", eError, PVRSRV_ERROR_INVALID_PARAMS, Sparse_Error);
	}

	/* Scan through all pages of the PMR */
	for (uiPageIndex = 0; uiPageIndex < uiNumPages; uiPageIndex++)
	{
		/* map the physical page (for a given PMR offset) into kernel space */
		eError = PMRAcquireKernelMappingData(psPMR,
		                                     IMG_PAGES2BYTES64(uiPageIndex, uiLog2PageSize),
		                                     ui64PageSize,
		                                     &pvKernAddr,
		                                     &uiMappedSize,
		                                     &hPrivData);
		PVR_LOG_GOTO_IF_ERROR(eError, "PMRAcquireKernelMappingData", AcquireKernelMapping_Error);

		/* ensure the mapped page size is the same as the physical page size */
		if (uiMappedSize != ui64PageSize)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Physical Page size = 0x%08" IMG_UINT64_FMTSPECx ", Size of Mapping = 0x%016" IMG_UINT64_FMTSPECx,
			         __func__,
			         ui64PageSize,
			         (IMG_UINT64)uiMappedSize));
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, MappingSize_Error);
		}

		/* Use the conservative 'DeviceMemSet' here because we can't know
		 * if this PMR will be mapped cached.
		 */
		OSDeviceMemSet(pvKernAddr, 0, ui64PageSize);

		/* release mapping */
		PMRReleaseKernelMappingData(psPMR, hPrivData);

	}

	PVR_DPF((PVR_DBG_MESSAGE,
	         "%s: Zeroing PMR %p done (num pages %u, page size %" IMG_UINT64_FMTSPEC ")",
	         __func__,
	         psPMR,
	         uiNumPages,
	         ui64PageSize));

	return PVRSRV_OK;


	/* Error handling */

MappingSize_Error:
	PMRReleaseKernelMappingData(psPMR, hPrivData);

AcquireKernelMapping_Error:
Sparse_Error:
MultiPage_Error:

	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

PVRSRV_ERROR
PMRDumpPageList(PMR *psPMR,
                IMG_DEVMEM_LOG2ALIGN_T uiLog2PageSize)
{
	IMG_DEV_PHYADDR sDevAddrPtr;
	IMG_UINT32 uiNumPages;
	IMG_UINT32 uiPageIndex;
	IMG_BOOL bPageIsValid;
	IMG_UINT32 ui32Col = 16;
	IMG_UINT32 ui32SizePerCol = 11;
	IMG_UINT32 ui32ByteCount = 0;
	IMG_CHAR pszBuffer[16 /* ui32Col */ * 11 /* ui32SizePerCol */ + 1];
	PVRSRV_ERROR eError = PVRSRV_OK;

	/* Get number of pages */
	uiNumPages = (IMG_UINT32)(psPMR->uiLogicalSize >> uiLog2PageSize);

	/* Verify the logical Size is a multiple or the physical page size */
	if ((PMR_SIZE_T)uiNumPages << uiLog2PageSize != psPMR->uiLogicalSize)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: PMR is not a multiple of %" IMG_UINT64_FMTSPEC,
		        __func__, (IMG_UINT64) (1ULL << uiLog2PageSize)));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_NOT_PAGE_MULTIPLE, MultiPage_Error);
	}

	if (_PMRIsSparse(psPMR))
	{
		PVR_LOG_GOTO_WITH_ERROR("psPMR", eError, PVRSRV_ERROR_INVALID_PARAMS, Sparse_Error);
	}

	PVR_LOG(("    PMR %p, Number of pages %u, Log2PageSize %d", psPMR, uiNumPages, uiLog2PageSize));

	/* Print the address of the physical pages */
	for (uiPageIndex = 0; uiPageIndex < uiNumPages; uiPageIndex++)
	{
		/* Get Device physical Address */
		eError = PMR_DevPhysAddr(psPMR,
		                         uiLog2PageSize,
		                         1,
		                         IMG_PAGES2BYTES64(uiPageIndex, uiLog2PageSize),
		                         &sDevAddrPtr,
		                         &bPageIsValid,
		                         DEVICE_USE);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: PMR %p failed to get DevPhysAddr with error %u",
					__func__,
					psPMR,
					eError));
			goto DevPhysAddr_Error;
		}

		ui32ByteCount += OSSNPrintf(pszBuffer + ui32ByteCount, ui32SizePerCol + 1, "%08x ", (IMG_UINT32)(sDevAddrPtr.uiAddr >> uiLog2PageSize));
		PVR_ASSERT(ui32ByteCount < ui32Col * ui32SizePerCol);

		if (uiPageIndex % ui32Col == ui32Col-1)
		{
			PVR_LOG(("      Phys Page: %s", pszBuffer));
			ui32ByteCount = 0;
		}
	}
	if (ui32ByteCount > 0)
	{
		PVR_LOG(("      Phys Page: %s", pszBuffer));
	}

	return PVRSRV_OK;

	/* Error handling */
DevPhysAddr_Error:
Sparse_Error:
MultiPage_Error:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

DEFINE_PMR_ENV_GET_DATA(psPMR, sEnvData)

PVRSRV_ERROR
PMRInit(void)
{
	PVRSRV_ERROR eError;

	/* Singleton PMR context already initialised */
	if (_gsSingletonPMRContext.bModuleInitialised)
	{
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_UNRECOVERABLE_ERROR, out);
	}

	eError = OSLockCreate(&_gsSingletonPMRContext.hLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate", out);

	_gsSingletonPMRContext.uiNextSerialNum = 1;

	_gsSingletonPMRContext.uiNextKey = 0x8300f001 * (uintptr_t)&_gsSingletonPMRContext;

	_gsSingletonPMRContext.bModuleInitialised = IMG_TRUE;

	OSAtomicWrite(&_gsSingletonPMRContext.uiNumLivePMRs, 0);

#if defined(PVRSRV_ENABLE_LINUX_MMAP_STATS)
	eError = MMapStatsInit();
	PVR_LOG_GOTO_IF_ERROR(eError, "MMapStatsInit", destroy_context_lock);
#endif

	return PVRSRV_OK;

#if defined(PVRSRV_ENABLE_LINUX_MMAP_STATS)
destroy_context_lock:
	OSLockDestroy(_gsSingletonPMRContext.hLock);
	_gsSingletonPMRContext.hLock = NULL;
#endif
out:
	return eError;
}

PVRSRV_ERROR
PMRDeInit(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (psPVRSRVData->eServicesState != PVRSRV_SERVICES_STATE_OK)
	{
		goto out;
	}

	/* Singleton PMR context is not initialised */
	if (!_gsSingletonPMRContext.bModuleInitialised)
	{
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_UNRECOVERABLE_ERROR, out);
	}

#if defined(PVRSRV_ENABLE_LINUX_MMAP_STATS)
	MMapStatsDeInit();
#endif

	if (OSAtomicRead(&_gsSingletonPMRContext.uiNumLivePMRs) != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Error: %d live PMRs remain",
				__func__,
				OSAtomicRead(&_gsSingletonPMRContext.uiNumLivePMRs)));
		PVR_DPF((PVR_DBG_ERROR, "%s: This is an unrecoverable error; a subsequent crash is inevitable",
				__func__));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_UNRECOVERABLE_ERROR, out);
	}

	if (_gsSingletonPMRContext.hLock != NULL)
	{
		OSLockDestroy(_gsSingletonPMRContext.hLock);
	}

	_gsSingletonPMRContext.bModuleInitialised = IMG_FALSE;

out:
	PVR_ASSERT(eError == PVRSRV_OK);
	return eError;
}

#if defined(SUPPORT_PMR_DEFERRED_FREE)
PVRSRV_ERROR
PMRInitDevice(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	PVRSRV_ERROR eError;

	eError = OSLockCreate(&psDeviceNode->hPMRZombieListLock);
	PVR_LOG_RETURN_IF_ERROR(eError, "OSLockCreate");

	dllist_init(&psDeviceNode->sPMRZombieList);
	psDeviceNode->uiPMRZombieCount = 0;
	psDeviceNode->uiPMRZombieCountInCleanup = 0;

	return PVRSRV_OK;
}

void
PMRFreeZombies(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	DECLARE_DLLIST(sZombieList);
	DLLIST_NODE *psThis, *psNext;
	IMG_INT32 uiZombieCount;

	_ZombieListLock(psDeviceNode);
	/* Move the zombie list to a local copy. The original list will become
	 * an empty list. This will allow us to process the list without holding
	 * the list lock. */
	dllist_replace_head(&psDeviceNode->sPMRZombieList, &sZombieList);
	uiZombieCount = psDeviceNode->uiPMRZombieCount;
	psDeviceNode->uiPMRZombieCount = 0;
	_ZombieListUnlock(psDeviceNode);

	dllist_foreach_node(&sZombieList, psThis, psNext)
	{
		dllist_remove_node(psThis);
		switch (PMR_GetZombieTypeFromNode(psThis))
		{
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
			case PMR_ZOMBIE_TYPE_PAGES:
			{
				PVRSRV_ERROR eError;
				PMR_ZOMBIE_PAGES* psZombiePages = PMR_GetZombiePagesFromNode(psThis);

				eError = psZombiePages->pfnFactoryFreeZombies(psZombiePages->pvFactoryPages);
				if (eError != PVRSRV_OK)
				{
					/* In case of failure to free zombie pages, remove it from
					* the sZombieList and add back to the original list. */
					_ZombieListLock(psDeviceNode);
					dllist_add_to_tail(&psDeviceNode->sPMRZombieList, psThis);
					psDeviceNode->uiPMRZombieCount++;
					_ZombieListUnlock(psDeviceNode);

					PVR_DPF((PVR_DBG_ERROR, "Cannot free zombie pages!"));
					continue;
				}

				OSFreeMem(psZombiePages);
				break;
			}
#endif

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
			case PMR_ZOMBIE_TYPE_DEVICE_IMPORT:
			{
				PMR_DEVICE_IMPORT *psImport = PMR_GetDeviceImportFromNode(psThis);
				_DeviceImportFreeImportZombie(psImport);
				break;
			}
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

			case PMR_ZOMBIE_TYPE_PMR:
			{
				PMR *psPMR = PMR_GetPMRFromNode(psThis);
				const PMR_IMPL_FUNCTAB *psFuncTable = psPMR->psFuncTab;

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
				/* The PMR cannot be freed as other devices are
				 * still waiting for the cache flush. */
				PMRLockPMR(psPMR);
				if (_DeviceImportBitmapGet(psPMR) != 0)
				{
					PDLLIST_NODE psNodeImport;
					PMR_DEVICE_IMPORT *psImport;
					/* Transfer the ownership to a different
					 * device queue that has not been processed yet.
					 * There will be a PMR_DEVICE_IMPORT on the same
					 * queue, however, this doesn't have any knock on affects as
					 * it will be freed before the PMR is reached again. */
					psNodeImport = dllist_get_next_node(&psPMR->sXDeviceImports);
					PVR_ASSERT(psNodeImport);
					psImport = IMG_CONTAINER_OF(psNodeImport, PMR_DEVICE_IMPORT, sNext);
					_ZombieListLock(psImport->psDevNode);
					dllist_add_to_tail(&psImport->psDevNode->sPMRZombieList, psThis);
					psImport->psDevNode->uiPMRZombieCount++;
					_ZombieListUnlock(psImport->psDevNode);
					PMRUnlockPMR(psPMR);
					break;
				}
				PMRUnlockPMR(psPMR);
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

				_FactoryLock(psFuncTable);
				_PMRDestroy(psPMR);
				_FactoryUnlock(psFuncTable);
				break;
			}
		}
		uiZombieCount--;
	}

	PVR_ASSERT(uiZombieCount == 0);
}

void
PMRDumpZombies(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	DLLIST_NODE *psThis, *psNext;

	_ZombieListLock(psDeviceNode);

	PVR_DPF((PVR_DBG_ERROR, "Items in zombie list: %u",
	        psDeviceNode->uiPMRZombieCount));

	dllist_foreach_node(&psDeviceNode->sPMRZombieList, psThis, psNext)
	{
		switch (PMR_GetZombieTypeFromNode(psThis))
		{
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
			case PMR_ZOMBIE_TYPE_PAGES:
			{
				PMR_ZOMBIE_PAGES* psZombiePages = PMR_GetZombiePagesFromNode(psThis);
				PVR_DPF((PVR_DBG_ERROR, "Zombie Pages = %p, Flavour = %s",
				         psZombiePages, _GetFlavourStr(psZombiePages->ePagesFlavour)));
				break;
			}
#endif

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
			case PMR_ZOMBIE_TYPE_DEVICE_IMPORT:
			{
				PMR_DEVICE_IMPORT* psImport = PMR_GetDeviceImportFromNode(psThis);
				PVR_DPF((PVR_DBG_ERROR, "Device Import = %p, DevID = %u, PMR = %px (%s)",
				         psImport,
				         psImport->psDevNode->sDevId.ui32InternalID,
				         psImport->psParent,
				         PMR_GetAnnotation(psImport->psParent)));
				break;
			}
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */

			case PMR_ZOMBIE_TYPE_PMR:
			{
				PMR *psPMR = PMR_GetPMRFromNode(psThis);
				PVR_DPF((PVR_DBG_ERROR, "PMR = %px, Flavour = %s, Annotation: %s",
						psPMR, PMR_GetTypeStr(psPMR), PMR_GetAnnotation(psPMR)));
				break;
			}
		}
	}

	_ZombieListUnlock(psDeviceNode);
}

void
PMRDeInitDevice(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	PMRFreeZombies(psDeviceNode);

	OSLockDestroy(psDeviceNode->hPMRZombieListLock);
}
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

#if defined(PVRSRV_ENABLE_XD_MEM)
IMG_UINT64
PMR_ImportedDevicesMask(const PMR* psPMR)
{
	static_assert(PVRSRV_MAX_DEVICES <= 64, "Code works under assumption that PVRSRV_MAX_DEVICES cannot be more than 64.");
	return _DeviceImportBitmapGet(psPMR) & ((PVRSRV_MAX_DEVICES == 64) ? ~IMG_UINT64_C(0) : ((IMG_UINT64_C(1) << PVRSRV_MAX_DEVICES) - 1));
}
#endif /* defined(PVRSRV_ENABLE_XD_MEM) */

#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) || defined(PVRSRV_ENABLE_XD_MEM)
PVRSRV_ERROR
PMR_RegisterDeviceImport(PMR* psPMR, PPVRSRV_DEVICE_NODE psDevNode)
{
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	PVR_ASSERT(!PMR_IsZombie(psPMR));
#endif /* !defined(SUPPORT_PMR_DEFERRED_FREE) */

	if (PMR_DeviceNode(psPMR) != psDevNode)
	{
#if defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE)
		PVRSRV_ERROR eError = _DeviceImportRegister(psPMR, psDevNode);
		PVR_LOG_RETURN_IF_ERROR(eError, "_DeviceImportRegister");
#else
		/* `_DeviceImportRegister` already sets the bitmap.
		 * This is still needs to be set without device import zombie support
		 * for debugging information, i.e. the RI. */
		_DeviceImportBitmapSet(psPMR, psDevNode);
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) */
	}
	/* else: We explicitly don't add the PMR's dev node to the list because
	 *       this bitmask lets us know if the PMR is cross device. It's not
	 *       an error to register with the original dev node, as the user is
	 *       declaring "The PMR is using `psDevNode`", not that it's a new
	 *       devnode. */
	return PVRSRV_OK;
}
#endif /* defined(SUPPORT_PMR_DEVICE_IMPORT_DEFERRED_FREE) || defined(PVRSRV_ENABLE_XD_MEM) */
