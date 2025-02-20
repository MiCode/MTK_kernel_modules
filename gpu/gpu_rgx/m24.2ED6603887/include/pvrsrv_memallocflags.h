/*************************************************************************/ /*!
@File
@Title          Device Memory Management
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    This file defines flags used on memory allocations and mappings
                These flags are relevant throughout the memory management
                software stack and are specified by users of services and
                understood by all levels of the memory management in both
                client and server.
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

#ifndef PVRSRV_MEMALLOCFLAGS_H
#define PVRSRV_MEMALLOCFLAGS_H

#include "img_types.h"
#include "pvrsrv_memalloc_physheap.h"

/*!
  Type for specifying memory allocation flags.
 */

typedef IMG_UINT64 PVRSRV_MEMALLOCFLAGS_T;
#define PVRSRV_MEMALLOCFLAGS_FMTSPEC IMG_UINT64_FMTSPECx

#if defined(__KERNEL__)
#include "pvrsrv_memallocflags_internal.h"
#endif /* __KERNEL__ */

/*
 * --- MAPPING FLAGS      0..14 (15-bits) ---
 * | 0-3    | 4-7    | 8-10        | 11-13       | 14          |
 * | GPU-RW | CPU-RW | GPU-Caching | CPU-Caching | KM-Mappable |
 *
 * --- MISC FLAGS         15..20 (9-bits) ---
 * | 15    | 16        | 17  | 18         | 19              | 20      |
 * | Defer | Alloc-Now | SVM | Scratch-Pg | CPU-Cache-Clean | Zero-Pg |
 *
 * --- RI FLAGS  21..23 (3-bits) ---
 * | 21     | 22       | 23        |
 * | Import | Suballoc | FW alloc  |
 *
 * --- DEV CONTROL FLAGS  26..27 (2-bits) ---
 * | 24-25 | 26-27        |
 * | ...   | Device-Flags |
 *
 * --- MISC FLAGS         28..31 (4-bits) ---
 * | 28             | 29             | 30          | 31            |
 * | No-Cache-Align | Poison-On-Free | P.-On-Alloc | Zero-On-Alloc |
 *
 * --- VALIDATION FLAGS ---
 * | 35             |
 * | Shared-buffer  |
 *
 * --- OS SPECIFIC FLAGS ---
 * | 36             | 37            |
 * | Linux Pref CMA | Linux Movable |
 *
 * --- IPA Policy ---
 * | 53-55      |
 * | IPA Policy |
 *
 * --- PHYS HEAP HINTS ---
 * | 56            | 57-58 | 59-63          |
 * | Mandate Heap  |       | PhysHeap Hints |
 *
 */

/*
 *  **********************************************************
 *  *                                                        *
 *  *                      MAPPING FLAGS                     *
 *  *                                                        *
 *  **********************************************************
 */

/*!
 * This flag affects the device MMU protection flags, and specifies
 * that the memory may be read by the GPU.
 *
 * Typically all device memory allocations would specify this flag.
 *
 * At the moment, memory allocations without this flag are not supported
 *
 * This flag will live with the PMR, thus subsequent mappings would
 * honour this flag.
 *
 * This is a dual purpose flag.  It specifies that memory is permitted
 * to be read by the GPU, and also requests that the allocation is
 * mapped into the GPU as a readable mapping
 *
 * To be clear:
 * - When used as an argument on PMR creation; it specifies
 *       that GPU readable mappings will be _permitted_
 * - When used as an argument to a "map" function: it specifies
 *       that a GPU readable mapping is _desired_
 * - When used as an argument to "AllocDeviceMem": it specifies
 *       that the PMR will be created with permission to be mapped
 *       with a GPU readable mapping, _and_ that this PMR will be
 *       mapped with a GPU readable mapping.
 * This distinction becomes important when (a) we export allocations;
 * and (b) when we separate the creation of the PMR from the mapping.
 */
#define PVRSRV_MEMALLOCFLAG_GPU_READABLE		(IMG_UINT64_C(1)<<0)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_GPU_READABLE flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_GPU_READABLE(uiFlags)		(((uiFlags) & PVRSRV_MEMALLOCFLAG_GPU_READABLE) != 0U)

/*!
 * This flag affects the device MMU protection flags, and specifies
 * that the memory may be written by the GPU
 *
 * Using this flag on an allocation signifies that the allocation is
 * intended to be written by the GPU.
 *
 * Omitting this flag causes a read-only mapping.
 *
 * This flag will live with the PMR, thus subsequent mappings would
 * honour this flag.
 *
 * This is a dual purpose flag.  It specifies that memory is permitted
 * to be written by the GPU, and also requests that the allocation is
 * mapped into the GPU as a writable mapping (see note above about
 * permission vs. mapping mode, and why this flag causes permissions
 * to be inferred from mapping mode on first allocation)
 *
 * N.B.  This flag has no relevance to the CPU's MMU mapping, if any,
 * and would therefore not enforce read-only mapping on CPU.
 */
#define PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE       (IMG_UINT64_C(1)<<1)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_GPU_WRITEABLE(uiFlags)				(((uiFlags) & PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE) != 0U)

/*!
  The flag indicates whether an allocation can be mapped as GPU readable in another GPU memory context.
 */
#define PVRSRV_MEMALLOCFLAG_GPU_READ_PERMITTED  (IMG_UINT64_C(1)<<2)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_GPU_READ_PERMITTED flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_GPU_READ_PERMITTED(uiFlags)		(((uiFlags) & PVRSRV_MEMALLOCFLAG_GPU_READ_PERMITTED) != 0U)

/*!
  The flag indicates whether an allocation can be mapped as GPU writable in another GPU memory context.
 */
#define PVRSRV_MEMALLOCFLAG_GPU_WRITE_PERMITTED (IMG_UINT64_C(1)<<3)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_GPU_WRITE_PERMITTED flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_GPU_WRITE_PERMITTED(uiFlags)		(((uiFlags) & PVRSRV_MEMALLOCFLAG_GPU_WRITE_PERMITTED) != 0U)

/*!
  The flag indicates that an allocation is mapped as readable to the CPU.
 */
#define PVRSRV_MEMALLOCFLAG_CPU_READABLE        (IMG_UINT64_C(1)<<4)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CPU_READABLE flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_CPU_READABLE(uiFlags)				(((uiFlags) & PVRSRV_MEMALLOCFLAG_CPU_READABLE) != 0U)

/*!
  The flag indicates that an allocation is mapped as writable to the CPU.
 */
#define PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE       (IMG_UINT64_C(1)<<5)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_CPU_WRITEABLE(uiFlags)				(((uiFlags) & PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE) != 0U)

/*!
  The flag indicates whether an allocation can be mapped as CPU readable in another CPU memory context.
 */
#define PVRSRV_MEMALLOCFLAG_CPU_READ_PERMITTED  (IMG_UINT64_C(1)<<6)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CPU_READ_PERMITTED flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_CPU_READ_PERMITTED(uiFlags)		(((uiFlags) & PVRSRV_MEMALLOCFLAG_CPU_READ_PERMITTED) != 0U)

/*!
  The flag indicates whether an allocation can be mapped as CPU writable in another CPU memory context.
 */
#define PVRSRV_MEMALLOCFLAG_CPU_WRITE_PERMITTED (IMG_UINT64_C(1)<<7)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CPU_WRITE_PERMITTED flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_CPU_WRITE_PERMITTED(uiFlags)		(((uiFlags) & PVRSRV_MEMALLOCFLAG_CPU_WRITE_PERMITTED) != 0U)


/*
 *  **********************************************************
 *  *                                                        *
 *  *                   CACHE CONTROL FLAGS                  *
 *  *                                                        *
 *  **********************************************************
 */

/*
	GPU domain
	==========

	The following defines are used to control the GPU cache bit field.
	The defines are mutually exclusive.

	A helper macro, PVRSRV_GPU_CACHE_MODE, is provided to obtain just the GPU
	cache bit field from the flags. This should be used whenever the GPU cache
	mode needs to be determined.
*/

/*!
  GPU domain. Flag indicating uncached memory. This means that any writes to memory
  allocated with this flag are written straight to memory and thus are
  coherent for any device in the system.
*/
#define PVRSRV_MEMALLOCFLAG_GPU_UNCACHED				(IMG_UINT64_C(1)<<8)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_GPU_UNCACHED mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_GPU_UNCACHED(uiFlags)				(PVRSRV_GPU_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_GPU_UNCACHED)

/*!
   GPU domain. Use write combiner (if supported) to combine sequential writes
   together to reduce memory access by doing burst writes.
*/
#define PVRSRV_MEMALLOCFLAG_GPU_UNCACHED_WC			(IMG_UINT64_C(0)<<8)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_GPU_UNCACHED_WC mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_GPU_WRITE_COMBINE(uiFlags)			(PVRSRV_GPU_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_GPU_UNCACHED_WC)

/*!
    GPU domain. This flag affects the GPU MMU protection flags.
    The allocation will be cached.
    Services will try to set the coherent bit in the GPU MMU tables so the
    GPU cache is snooping the CPU cache. If coherency is not supported the
    caller is responsible to ensure the caches are up to date.
*/
#define PVRSRV_MEMALLOCFLAG_GPU_CACHE_COHERENT			(IMG_UINT64_C(2)<<8)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_GPU_CACHE_COHERENT mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_GPU_CACHE_COHERENT(uiFlags)		(PVRSRV_GPU_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_GPU_CACHE_COHERENT)

/*!
   GPU domain. Request cached memory, but not coherent (i.e. no cache
   snooping). Services will flush the GPU internal caches after every GPU
   task so no cache maintenance requests from the users are necessary.

    Note: We reserve 3 bits in the CPU/GPU cache mode to allow for future
    expansion.
*/
#define PVRSRV_MEMALLOCFLAG_GPU_CACHE_INCOHERENT		(IMG_UINT64_C(3)<<8)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_GPU_CACHE_INCOHERENT mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_GPU_CACHE_INCOHERENT(uiFlags)		(PVRSRV_GPU_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_GPU_CACHE_INCOHERENT)

/*!
    GPU domain. This flag is for internal use only and is used to indicate
    that the underlying allocation should be cached on the GPU after all
    the snooping and coherent checks have been done
*/
#define PVRSRV_MEMALLOCFLAG_GPU_CACHED					(IMG_UINT64_C(7)<<8)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_GPU_CACHED mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_GPU_CACHED(uiFlags)				(PVRSRV_GPU_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_GPU_CACHED)

/*!
    GPU domain. GPU cache mode mask.
*/
#define PVRSRV_MEMALLOCFLAG_GPU_CACHE_MODE_MASK			(IMG_UINT64_C(7)<<8)

/*!
  @Description    A helper macro to obtain just the GPU	cache bit field from the flags.
                  This should be used whenever the GPU cache mode needs to be determined.
  @Input  uiFlags Allocation flags.
  @Return         Value of the GPU cache bit field.
 */
#define PVRSRV_GPU_CACHE_MODE(uiFlags)					((uiFlags) & PVRSRV_MEMALLOCFLAG_GPU_CACHE_MODE_MASK)


/*
	CPU domain
	==========

	The following defines are used to control the CPU cache bit field.
	The defines are mutually exclusive.

	A helper macro, PVRSRV_CPU_CACHE_MODE, is provided to obtain just the CPU
	cache bit field from the flags. This should be used whenever the CPU cache
	mode needs to be determined.
*/

/*!
   CPU domain. Use write combiner (if supported) to combine sequential writes
   together to reduce memory access by doing burst writes.
*/
#define PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC			(IMG_UINT64_C(0)<<11)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_CPU_WRITE_COMBINE(uiFlags)			(PVRSRV_CPU_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC)

/*!
    CPU domain. This flag affects the CPU MMU protection flags.
    The allocation will be cached.
    Services will try to set the coherent bit in the CPU MMU tables so the
    CPU cache is snooping the GPU cache. If coherency is not supported the
    caller is responsible to ensure the caches are up to date.
*/
#define PVRSRV_MEMALLOCFLAG_CPU_CACHE_COHERENT			(IMG_UINT64_C(2)<<11)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CPU_CACHE_COHERENT mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_CPU_CACHE_COHERENT(uiFlags)		(PVRSRV_CPU_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_CPU_CACHE_COHERENT)

/*!
    CPU domain. Request cached memory, but not coherent (i.e. no cache
    snooping). This means that if the allocation needs to transition from
    one device to another services has to be informed so it can
    flush/invalidate the appropriate caches.

    Note: We reserve 3 bits in the CPU/GPU cache mode to allow for future
    expansion.
*/
#define PVRSRV_MEMALLOCFLAG_CPU_CACHE_INCOHERENT		(IMG_UINT64_C(3)<<11)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CPU_CACHE_INCOHERENT mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_CPU_CACHE_INCOHERENT(uiFlags)		(PVRSRV_CPU_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_CPU_CACHE_INCOHERENT)

/*!
    CPU domain. This flag is for internal use only and is used to indicate
    that the underlying allocation should be cached on the CPU
    after all the snooping and coherent checks have been done
*/
#define PVRSRV_MEMALLOCFLAG_CPU_CACHED					(IMG_UINT64_C(7)<<11)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CPU_CACHED mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_CPU_CACHED(uiFlags)				(PVRSRV_CPU_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_CPU_CACHED)

/*!
	CPU domain. CPU cache mode mask
*/
#define PVRSRV_MEMALLOCFLAG_CPU_CACHE_MODE_MASK			(IMG_UINT64_C(7)<<11)

/*!
  @Description    A helper macro to obtain just the CPU	cache bit field from the flags.
                  This should be used whenever the CPU cache mode needs to be determined.
  @Input  uiFlags Allocation flags.
  @Return         Value of the CPU cache bit field.
 */
#define PVRSRV_CPU_CACHE_MODE(uiFlags)					((uiFlags) & PVRSRV_MEMALLOCFLAG_CPU_CACHE_MODE_MASK)

/* Helper flags for usual cases */

/*!
 * Memory will be write-combined on CPU and GPU
 */
#define PVRSRV_MEMALLOCFLAG_UNCACHED_WC		(PVRSRV_MEMALLOCFLAG_GPU_UNCACHED_WC | PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_UNCACHED_WC mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_WRITE_COMBINE(uiFlags)				(PVRSRV_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_UNCACHED_WC)

/*!
 * Memory will be cached.
 * Services will try to set the correct flags in the MMU tables.
 * In case there is no coherency support the caller has to ensure caches are up to date */
#define PVRSRV_MEMALLOCFLAG_CACHE_COHERENT				(PVRSRV_MEMALLOCFLAG_GPU_CACHE_COHERENT | PVRSRV_MEMALLOCFLAG_CPU_CACHE_COHERENT)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CACHE_COHERENT mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_CACHE_COHERENT(uiFlags)			(PVRSRV_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_CACHE_COHERENT)

/*!
 * Memory will be cache-incoherent on CPU and GPU
 */
#define PVRSRV_MEMALLOCFLAG_CACHE_INCOHERENT			(PVRSRV_MEMALLOCFLAG_GPU_CACHE_INCOHERENT | PVRSRV_MEMALLOCFLAG_CPU_CACHE_INCOHERENT)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CACHE_INCOHERENT mode is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the mode is set, false otherwise
 */
#define PVRSRV_CHECK_CACHE_INCOHERENT(uiFlags)			(PVRSRV_CACHE_MODE(uiFlags) == PVRSRV_MEMALLOCFLAG_CACHE_INCOHERENT)

/*!
	Cache mode mask
*/
#define PVRSRV_CACHE_MODE(uiFlags)						(PVRSRV_GPU_CACHE_MODE(uiFlags) | PVRSRV_CPU_CACHE_MODE(uiFlags))


/*!
   CPU MMU Flags mask -- intended for use internal to services only
 */
#define PVRSRV_MEMALLOCFLAGS_CPU_MMUFLAGSMASK  (PVRSRV_MEMALLOCFLAG_CPU_READABLE | \
												PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE | \
												PVRSRV_MEMALLOCFLAG_CPU_CACHE_MODE_MASK)

/*!
   MMU Flags mask -- intended for use internal to services only - used for
   partitioning the flags bits and determining which flags to pass down to
   mmu_common.c
 */
#define PVRSRV_MEMALLOCFLAGS_GPU_MMUFLAGSMASK  (PVRSRV_MEMALLOCFLAG_GPU_READABLE | \
                                                PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE | \
                                                PVRSRV_MEMALLOCFLAG_GPU_CACHE_MODE_MASK)

/*!
    Indicates that the PMR created due to this allocation will support
    in-kernel CPU mappings.  Only privileged processes may use this flag as
    it may cause wastage of precious kernel virtual memory on some platforms.
 */
#define PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE			(IMG_UINT64_C(1)<<14)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_KERNEL_CPU_MAPPABLE(uiFlags)		(((uiFlags) & PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE) != 0U)



/*
 *
 *  **********************************************************
 *  *                                                        *
 *  *                   ALLOC MEMORY FLAGS                   *
 *  *                                                        *
 *  **********************************************************
 */

/*!	----- Bit 15

    Indicates when the allocation of physical memory pages backing the PMR
    is carried out. When set, pages are not allocated at PMR creation but are
    instead deferred until they are first needed, i.e. "on demand".
    When unset, the pages may be allocated at the same time the PMR is created
    or deferred (at the KM/Server's discretion).
    See also PVRSRV_MEMALLOCFLAG_PHYS_ALLOC_NOW (below). Note that at most one
    of these two flags may be set.
 */
#define PVRSRV_MEMALLOCFLAG_DEFER_PHYS_ALLOC			(IMG_UINT64_C(1)<<15)
/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_DEFER_PHYS_ALLOC flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_ON_DEMAND(uiFlags)					(((uiFlags) & PVRSRV_MEMALLOCFLAG_DEFER_PHYS_ALLOC) != 0U)

/*! ----- Bit 16

    Indicates when the allocation of physical memory pages backing the PMR
    is carried out. When set, pages are allocated at PMR creation.
    When unset, the pages may be allocated at the same time the PMR is created
    or deferred (at the KM/Server's discretion).
    See also PVRSRV_MEMALLOCFLAG_DEFER_PHYS_ALLOC (above). Note that at most one
    of these two flags may be set.
 */
#define PVRSRV_MEMALLOCFLAG_PHYS_ALLOC_NOW				(IMG_UINT64_C(1)<<16)
/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_PHYS_ALLOC_NOW flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_PHYS_ALLOC_NOW(uiFlags)			(((uiFlags) & PVRSRV_MEMALLOCFLAG_PHYS_ALLOC_NOW) != 0U)

/*! ----- Bit 17

    Indicates that the allocation will be accessed by the CPU and GPU using
    the same virtual address, i.e. for all SVM allocs,
    IMG_CPU_VIRTADDR == IMG_DEV_VIRTADDR
 */
#define PVRSRV_MEMALLOCFLAG_SVM_ALLOC					(IMG_UINT64_C(1)<<17)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_SVM_ALLOC flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_SVM_ALLOC(uiFlags)					(((uiFlags) & PVRSRV_MEMALLOCFLAG_SVM_ALLOC) != 0U)

/*! ----- Bit 18

    Indicates the particular memory that's being allocated is sparse and the
    sparse regions should not be backed by scratch page
 */
#define PVRSRV_MEMALLOCFLAG_SPARSE_NO_SCRATCH_BACKING		(IMG_UINT64_C(1) << 18)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_SPARSE_NO_SCRATCH_BACKING flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_IS_SPARSE_SCRATCH_BACKING_REQUIRED(uiFlags)		(((uiFlags) & PVRSRV_MEMALLOCFLAG_SPARSE_NO_SCRATCH_BACKING) == 0U)

/*! ----- Bit 19

  Used to force Services to carry out at least one CPU cache flush and
  invalidate on a CPU cached buffer during allocation of the memory. Applicable
  to incoherent systems, it must be used for buffers which are CPU cached and
  which will not be 100% written to by the CPU before the GPU accesses it. For
  performance reasons, avoid usage if the whole buffer that is allocated is
  written to by the CPU anyway before the next GPU kick, or if the system is
  coherent.
 */
#define PVRSRV_MEMALLOCFLAG_CPU_CACHE_CLEAN				(IMG_UINT64_C(1)<<19)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_CPU_CACHE_CLEAN flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_CPU_CACHE_CLEAN(uiFlags)			(((uiFlags) & PVRSRV_MEMALLOCFLAG_CPU_CACHE_CLEAN) != 0U)

/*! ----- Bit 20

    Indicates the particular memory region should be backed by zero page.
    This is different with zero on alloc flag such that only physically unbacked
    pages are backed by zero page at the time of mapping.
    The zero backed page is always with read only attribute irrespective of its
    original attributes.
 */
#define PVRSRV_MEMALLOCFLAG_ZERO_BACKING			(IMG_UINT64_C(1) << 20)
#define PVRSRV_IS_ZERO_BACKING_REQUIRED(uiFlags)		(((uiFlags) & \
			PVRSRV_MEMALLOCFLAG_ZERO_BACKING) == PVRSRV_MEMALLOCFLAG_ZERO_BACKING)

/*
 ************************************************************
 *                         RI flags                         *
 ************************************************************
 * The Flags are used exclusively by the RI (Reference Info)
 * server for tracking KM allocations by various processes
 *
 * Import - Handle imported from other process
 * Suballoc - Handle Suballocation of existing PMR
 * FW Alloc - Used by FW during driver initialisation
 *
 * --- RI FLAGS  21..23 (3-bits) ---
 * | 21     | 22       | 23        |
 * | Import | Suballoc | FW alloc  |
 *
 */

/*! ----- Bit 21
  Used by RI server to register Allocation as an import from other process
 */
#define PVRSRV_MEMALLOCFLAG_RI_IMPORT					(IMG_UINT64_C(1)<<21)
/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_RI_IMPORT flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_RI_IMPORT(uiFlags)			(((uiFlags) & PVRSRV_MEMALLOCFLAG_RI_IMPORT) != 0U)

/*! ----- Bit 22
  Used by RI server to register Allocation as a suballocation of existing PMR
 */
#define PVRSRV_MEMALLOCFLAG_RI_SUBALLOC					(IMG_UINT64_C(1)<<22)
/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_RI_SUBALLOC flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_RI_SUBALLOC(uiFlags)			(((uiFlags) & PVRSRV_MEMALLOCFLAG_RI_SUBALLOC) != 0U)

/*! ----- Bit 23
  Used by RI server to register Allocation as FW/System process
  'FW Alloc' also means the allocation is considered to belong to the SYS process
  (ie that it will have a lifetime longer than the process which allocated it)
 */
#define PVRSRV_MEMALLOCFLAG_RI_FWKMD_ALLOC				(IMG_UINT64_C(1)<<23)
/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_RI_FWKMD_ALLOC flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_RI_FWKMD_ALLOC(uiFlags)			(((uiFlags) & PVRSRV_MEMALLOCFLAG_RI_FWKMD_ALLOC) != 0U)

/*!
  @Description    Macro passing the PVRSRV_MEMALLOCFLAG_RI_ Flags.
  @Input  uiFlags Allocation flags.
  @Return         Value of the RI server bit field
 */
#define PVRSRV_MEMALLOCFLAG_RI_MASK(uiFlags)			((uiFlags) & (IMG_UINT64_C(7)<<21))


/*! ----- Bit 24
  *
    Not used.
 */

/*
 *
 *  **********************************************************
 *  *                                                        *
 *  *           MEMORY ZEROING AND POISONING FLAGS           *
 *  *                                                        *
 *  **********************************************************
 *
 * Zero / Poison, on alloc/free
 *
 * We think the following usecases are required:
 *
 *  don't poison or zero on alloc or free
 *     (normal operation, also most efficient)
 *  poison on alloc
 *     (for helping to highlight bugs)
 *  poison on alloc and free
 *     (for helping to highlight bugs)
 *  zero on alloc
 *     (avoid highlighting security issues in other uses of memory)
 *  zero on alloc and poison on free
 *     (avoid highlighting security issues in other uses of memory, while
 *      helping to highlight a subset of bugs e.g. memory freed prematurely)
 *
 * Since there are more than 4, we can't encode this in just two bits,
 * so we might as well have a separate flag for each of the three
 * actions.
 */

/*!
    Ensures that the memory allocated is initialised with zeroes.
 */
#define PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC				(IMG_UINT64_C(1)<<31)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_ZERO_ON_ALLOC(uiFlags)				(((uiFlags) & PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC) != 0U)

/*!
    Scribbles over the allocated memory with a poison value

    Not compatible with ZERO_ON_ALLOC

    Poisoning is very deliberately _not_ reflected in PDump as we want
    a simulation to cry loudly if the initialised data propagates to a
    result.
 */
#define PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC				(IMG_UINT64_C(1)<<30)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_POISON_ON_ALLOC(uiFlags)			(((uiFlags) & PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC) != 0U)

#if defined(DEBUG)
/*!
    Causes memory to be trashed when freed, used when debugging only, not to be used
    as a security measure.
 */
#define PVRSRV_MEMALLOCFLAG_POISON_ON_FREE				(IMG_UINT64_C(1)<<29)

/*!
  @Description    Macro checking whether the PVRSRV_MEMALLOCFLAG_POISON_ON_FREE flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_POISON_ON_FREE(uiFlags)			(((uiFlags) & PVRSRV_MEMALLOCFLAG_POISON_ON_FREE) != 0U)
#endif /* DEBUG */

/*!
    Avoid address alignment to a CPU or GPU cache line size.
 */
#define PVRSRV_MEMALLOCFLAG_NO_CACHE_LINE_ALIGN			(IMG_UINT64_C(1)<<28)

/*!
  @Description    Macro checking whether the PVRSRV_CHECK_NO_CACHE_LINE_ALIGN flag is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the flag is set, false otherwise
 */
#define PVRSRV_CHECK_NO_CACHE_LINE_ALIGN(uiFlags)		(((uiFlags) & PVRSRV_MEMALLOCFLAG_NO_CACHE_LINE_ALIGN) != 0U)


/*
 *
 *  **********************************************************
 *  *                                                        *
 *  *                Device specific MMU flags               *
 *  *                                                        *
 *  **********************************************************
 *
 * (Bits 26 to 27)
 *
 * Some services controlled devices have device specific control bits in
 * their page table entries, we need to allow these flags to be passed down
 * the memory management layers so the user can control these bits.
 * For example, RGX device has the file rgx_memallocflags.h
 */

/*!
 * Offset of device specific MMU flags.
 */
#define PVRSRV_MEMALLOCFLAG_DEVICE_FLAGS_OFFSET		26

/*!
 * Mask for retrieving device specific MMU flags.
 */
#define PVRSRV_MEMALLOCFLAG_DEVICE_FLAGS_MASK		(IMG_UINT64_C(3) << PVRSRV_MEMALLOCFLAG_DEVICE_FLAGS_OFFSET)

/*!
  @Description    Helper macro for setting device specific MMU flags.
  @Input  uiFlags Flag index.
  @Return         Flag vector with the specified bit set.
 */
#define PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(uiFlags)	\
			(((PVRSRV_MEMALLOCFLAGS_T)(uiFlags) << PVRSRV_MEMALLOCFLAG_DEVICE_FLAGS_OFFSET) & \
			PVRSRV_MEMALLOCFLAG_DEVICE_FLAGS_MASK)

/*
 *
 *  **********************************************************
 *  *                                                        *
 *  *                 Secure validation flags                *
 *  *                                                        *
 *  **********************************************************
 *
 * (Bit 35)
 *
 */

/*!
    PVRSRV_MEMALLOCFLAG_VAL_SHARED_BUFFER
 */

#define PVRSRV_MEMALLOCFLAG_VAL_SHARED_BUFFER           (IMG_UINT64_C(1)<<35)
#define PVRSRV_CHECK_SHARED_BUFFER(uiFlags)             (((uiFlags) & PVRSRV_MEMALLOCFLAG_VAL_SHARED_BUFFER) != 0U)

/*
 *
 *  **********************************************************
 *  *                                                        *
 *  *                OS Specific alloc flags                 *
 *  *                                                        *
 *  **********************************************************
 *
 * (Bits 36 to 37)
 *
 */
#define PVRSRV_MEMALLOCFLAG_OS_ALLOCFLAG_OFFSET 36
#define PVRSRV_MEMALLOCFLAG_OS_ALLOCFLAG_MASK          (IMG_UINT64_C(3) << PVRSRV_MEMALLOCFLAG_OS_ALLOCFLAG_OFFSET)

#define PVRSRV_MEMALLOCFLAG_OS_LINUX_PREFER_CMA         (IMG_UINT64_C(1)<<36)
#define PVRSRV_CHECK_OS_LINUX_PREFER_CMA(uiFlags)       (((uiFlags) & PVRSRV_MEMALLOCFLAG_OS_LINUX_PREFER_CMA) != 0U)

#define PVRSRV_MEMALLOCFLAG_OS_LINUX_MOVABLE           (IMG_UINT64_C(1)<<37)
#define PVRSRV_CHECK_OS_LINUX_MOVABLE(uiFlags)         (((uiFlags) & PVRSRV_MEMALLOCFLAG_OS_LINUX_MOVABLE) != 0U)

/*
 *
 *  **********************************************************
 *  *                                                        *
 *  *                       IPA Policy                       *
 *  *                                                        *
 *  **********************************************************
 *
 * (Bits 53 to 55)
 *
 */

/*!
 * Offset of Intermediate Physical Address (IPA) policy.
 */
#define PVRSRV_MEMALLOCFLAG_IPA_POLICY_OFFSET      53

/*!
 * Mask for retrieving IPA policy.
 */
#define PVRSRV_MEMALLOCFLAG_IPA_POLICY_MASK        (IMG_UINT64_C(7) << PVRSRV_MEMALLOCFLAG_IPA_POLICY_OFFSET)
#define PVRSRV_MEMALLOCFLAG_IPA_POLICY(uiFlags)    (((uiFlags) & PVRSRV_MEMALLOCFLAG_IPA_POLICY_MASK) >> PVRSRV_MEMALLOCFLAG_IPA_POLICY_OFFSET)

/*
 *
 *  **********************************************************
 *  *                                                        *
 *  *                 Phys Heap Hints                        *
 *  *                                                        *
 *  **********************************************************
 *
 * (Bits 56 to 63)
 *
 */

/*!
 * Ensures Physheap isn't reassigned when considered favourable by driver under a OOM condition.
 */
#define PVRSRV_MEMALLOCFLAG_MANDATE_PHYSHEAP            (IMG_UINT64_C(1)<<56)
#define PVRSRV_CHECK_MANDATED_PHYSHEAP(uiFlags)         (((uiFlags) & PVRSRV_MEMALLOCFLAG_MANDATE_PHYSHEAP) != 0U)

/*!
 * Value of enum PVRSRV_PHYS_HEAP stored in memalloc flags. If not set
 * i.e. PVRSRV_PHYS_HEAP_DEFAULT (value 0) used, the system layer defined default physical heap is used.
 */
#define PVRSRV_PHYS_HEAP_HINT_SHIFT        (59)
#define PVRSRV_PHYS_HEAP_HINT_MASK         (IMG_UINT64_C(0x1F) << PVRSRV_PHYS_HEAP_HINT_SHIFT)


/*!
  @Description    Macro extracting the Phys Heap hint from memalloc flag value.
  @Input uiFlags  Allocation flags
  @Return         returns the value of the PHYS_HEAP_HINT bitfield
 */
#define PVRSRV_GET_PHYS_HEAP_HINT(uiFlags)      ((PVRSRV_PHYS_HEAP)(((uiFlags) & PVRSRV_PHYS_HEAP_HINT_MASK) \
                                                 >> PVRSRV_PHYS_HEAP_HINT_SHIFT))

/*!
  @Description    Macro converting a Phys Heap value into a memalloc bitfield
  @Input uiFlags  Device Phys Heap
  @Return         returns a shifted bitfield with the Device Phys Heap value
 */
#define PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(PhysHeap)      ((((PVRSRV_MEMALLOCFLAGS_T)PVRSRV_PHYS_HEAP_ ## PhysHeap) << \
                                                            PVRSRV_PHYS_HEAP_HINT_SHIFT) \
                                                           & PVRSRV_PHYS_HEAP_HINT_MASK)
/*!
  @Description    Macro to replace an existing phys heap hint value in flags.
  @Input PhysHeap Phys Heap Macro
  @Input uiFlags  Allocation flags
  @Return         N/A
 */
#define PVRSRV_SET_PHYS_HEAP_HINT(PhysHeap, uiFlags)	  (uiFlags) = ((uiFlags) & ~PVRSRV_PHYS_HEAP_HINT_MASK) | \
                                                           PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(PhysHeap)

/*!
  @Description    Macro to replace an existing phys heap hint value using Phys Heap value.
  @Input PhysHeap Phys Heap Value
  @Input uiFlags  Allocation flags
  @Return         N/A
 */
#define PVRSRV_CHANGE_PHYS_HEAP_HINT(Physheap, uiFlags)	  (uiFlags) = ((uiFlags) & ~PVRSRV_PHYS_HEAP_HINT_MASK) | \
                                                           (((PVRSRV_MEMALLOCFLAGS_T)(Physheap) << \
                                                            PVRSRV_PHYS_HEAP_HINT_SHIFT) \
                                                           & PVRSRV_PHYS_HEAP_HINT_MASK)

/*!
  @Description    Macros checking if a Phys Heap hint is set.
  @Input  uiFlags Allocation flags.
  @Return         True if the hint is set, false otherwise
 */
#define PVRSRV_CHECK_PHYS_HEAP(PhysHeap, uiFlags) (PVRSRV_PHYS_HEAP_ ## PhysHeap == PVRSRV_GET_PHYS_HEAP_HINT(uiFlags))

#define PVRSRV_CHECK_FW_MAIN(uiFlags)            (PVRSRV_CHECK_PHYS_HEAP(FW_MAIN, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_CONFIG, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_CODE, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_PRIV_DATA, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_PREMAP0, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_PREMAP1, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_PREMAP2, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_PREMAP3, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_PREMAP4, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_PREMAP5, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_PREMAP6, uiFlags) || \
                                                  PVRSRV_CHECK_PHYS_HEAP(FW_PREMAP7, uiFlags))

/*!
 * Secure buffer mask -- Flags in the mask are allowed for secure buffers
 * because they are not related to CPU mappings.
 */
#define PVRSRV_MEMALLOCFLAGS_SECBUFMASK  ~(PVRSRV_MEMALLOCFLAG_CPU_CACHE_MODE_MASK | \
                                           PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE | \
                                           PVRSRV_MEMALLOCFLAG_CPU_READABLE | \
                                           PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                           PVRSRV_MEMALLOCFLAG_CPU_CACHE_CLEAN | \
                                           PVRSRV_MEMALLOCFLAG_SVM_ALLOC | \
                                           PVRSRV_MEMALLOCFLAG_CPU_READ_PERMITTED | \
                                           PVRSRV_MEMALLOCFLAG_CPU_WRITE_PERMITTED)

/*!
 * Trusted device mask -- Flags in the mask are allowed for trusted device
 * because the driver cannot access the memory
 */
#if defined(DEBUG)
#define PVRSRV_MEMALLOCFLAGS_TDFWMASK    ~(PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                           PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                           PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC | \
                                           PVRSRV_MEMALLOCFLAG_POISON_ON_FREE | \
                                           PVRSRV_MEMALLOCFLAGS_CPU_MMUFLAGSMASK | \
                                           PVRSRV_MEMALLOCFLAG_SPARSE_NO_SCRATCH_BACKING)
#else
#define PVRSRV_MEMALLOCFLAGS_TDFWMASK    ~(PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                           PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                           PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC | \
                                           PVRSRV_MEMALLOCFLAGS_CPU_MMUFLAGSMASK | \
                                           PVRSRV_MEMALLOCFLAG_SPARSE_NO_SCRATCH_BACKING)
#endif

/*!
  PMR flags mask -- for internal services use only.  This is the set of flags
  that will be passed down and stored with the PMR, this also includes the
  MMU flags which the PMR has to pass down to mm_common.c at PMRMap time.
*/
#if defined(DEBUG)
#define PVRSRV_MEMALLOCFLAGS_PMRFLAGSMASK  (PVRSRV_MEMALLOCFLAG_DEVICE_FLAGS_MASK | \
                                            PVRSRV_MEMALLOCFLAG_CPU_CACHE_CLEAN | \
                                            PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                            PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                            PVRSRV_MEMALLOCFLAG_SVM_ALLOC | \
                                            PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC | \
                                            PVRSRV_MEMALLOCFLAG_POISON_ON_FREE | \
                                            PVRSRV_MEMALLOCFLAGS_GPU_MMUFLAGSMASK | \
                                            PVRSRV_MEMALLOCFLAGS_CPU_MMUFLAGSMASK | \
                                            PVRSRV_MEMALLOCFLAG_DEFER_PHYS_ALLOC | \
                                            PVRSRV_MEMALLOCFLAG_PHYS_ALLOC_NOW | \
                                            PVRSRV_MEMALLOCFLAG_SPARSE_NO_SCRATCH_BACKING | \
                                            PVRSRV_MEMALLOCFLAG_ZERO_BACKING | \
                                            PVRSRV_MEMALLOCFLAG_VAL_SHARED_BUFFER | \
                                            PVRSRV_MEMALLOCFLAG_MANDATE_PHYSHEAP | \
                                            PVRSRV_MEMALLOCFLAG_OS_ALLOCFLAG_MASK | \
                                            PVRSRV_MEMALLOCFLAG_IPA_POLICY_MASK | \
                                            PVRSRV_PHYS_HEAP_HINT_MASK)
#else
#define PVRSRV_MEMALLOCFLAGS_PMRFLAGSMASK  (PVRSRV_MEMALLOCFLAG_DEVICE_FLAGS_MASK | \
                                            PVRSRV_MEMALLOCFLAG_CPU_CACHE_CLEAN | \
                                            PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                            PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                            PVRSRV_MEMALLOCFLAG_SVM_ALLOC | \
                                            PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC | \
                                            PVRSRV_MEMALLOCFLAGS_GPU_MMUFLAGSMASK | \
                                            PVRSRV_MEMALLOCFLAGS_CPU_MMUFLAGSMASK | \
                                            PVRSRV_MEMALLOCFLAG_DEFER_PHYS_ALLOC | \
                                            PVRSRV_MEMALLOCFLAG_PHYS_ALLOC_NOW | \
                                            PVRSRV_MEMALLOCFLAG_SPARSE_NO_SCRATCH_BACKING | \
                                            PVRSRV_MEMALLOCFLAG_ZERO_BACKING | \
                                            PVRSRV_MEMALLOCFLAG_VAL_SHARED_BUFFER | \
                                            PVRSRV_MEMALLOCFLAG_MANDATE_PHYSHEAP  | \
                                            PVRSRV_MEMALLOCFLAG_OS_ALLOCFLAG_MASK | \
                                            PVRSRV_MEMALLOCFLAG_IPA_POLICY_MASK | \
                                            PVRSRV_PHYS_HEAP_HINT_MASK)
#endif

/*!
 * CPU mappable mask -- Any flag set in the mask requires memory to be CPU mappable
 */
#define PVRSRV_MEMALLOCFLAGS_CPU_MAPPABLE_MASK (PVRSRV_MEMALLOCFLAG_CPU_READABLE | \
                                                PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE | \
                                                PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                                PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC | \
                                                PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE)
/*!
  RA differentiation mask

  for use internal to services

  this is the set of flags bits that are able to determine whether a pair of
  allocations are permitted to live in the same page table. Allocations
  whose flags differ in any of these places would be allocated from separate
  RA Imports and therefore would never coexist in the same page.
  Special cases are zeroing and poisoning of memory. The caller is responsible
  to set the sub-allocations to the value he wants it to be. To differentiate
  between zeroed and poisoned RA Imports does not make sense because the
  memory might be reused.

*/
#if defined(DEBUG)
#define PVRSRV_MEMALLOCFLAGS_RA_DIFFERENTIATION_MASK (PVRSRV_MEMALLOCFLAGS_PMRFLAGSMASK \
                                                      & \
                                                      ~(PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC   | \
                                                        PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC | \
                                                        PVRSRV_MEMALLOCFLAG_POISON_ON_FREE))
#else
#define PVRSRV_MEMALLOCFLAGS_RA_DIFFERENTIATION_MASK (PVRSRV_MEMALLOCFLAGS_PMRFLAGSMASK \
                                                      & \
                                                      ~(PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC   | \
                                                        PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC))
#endif
/*!
  Flags that affect _allocation_
*/
#define PVRSRV_MEMALLOCFLAGS_PERALLOCFLAGSMASK (0xFFFFFFFFU)

/*!
  Flags that affect _mapping_
*/
#define PVRSRV_MEMALLOCFLAGS_PERMAPPINGFLAGSMASK   (PVRSRV_MEMALLOCFLAG_DEVICE_FLAGS_MASK | \
                                                    PVRSRV_MEMALLOCFLAGS_GPU_MMUFLAGSMASK | \
                                                    PVRSRV_MEMALLOCFLAGS_CPU_MMUFLAGSMASK | \
                                                    PVRSRV_MEMALLOCFLAG_DEFER_PHYS_ALLOC | \
                                                    PVRSRV_MEMALLOCFLAG_SVM_ALLOC | \
                                                    PVRSRV_MEMALLOCFLAG_ZERO_BACKING | \
                                                    PVRSRV_MEMALLOCFLAG_SPARSE_NO_SCRATCH_BACKING)

#if ((~(PVRSRV_MEMALLOCFLAGS_RA_DIFFERENTIATION_MASK) & PVRSRV_MEMALLOCFLAGS_PERMAPPINGFLAGSMASK) != 0U)
#error PVRSRV_MEMALLOCFLAGS_PERMAPPINGFLAGSMASK is not a subset of PVRSRV_MEMALLOCFLAGS_RA_DIFFERENTIATION_MASK
#endif


/*!
  Flags that affect _physical allocations_ in the DevMemX API
 */
#if defined(DEBUG)
#define PVRSRV_MEMALLOCFLAGS_DEVMEMX_PHYSICAL_MASK (PVRSRV_MEMALLOCFLAGS_CPU_MMUFLAGSMASK | \
                                                    PVRSRV_MEMALLOCFLAG_GPU_CACHE_MODE_MASK | \
                                                    PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE | \
                                                    PVRSRV_MEMALLOCFLAG_GPU_READABLE | \
                                                    PVRSRV_MEMALLOCFLAG_CPU_READ_PERMITTED | \
                                                    PVRSRV_MEMALLOCFLAG_CPU_WRITE_PERMITTED | \
                                                    PVRSRV_MEMALLOCFLAG_CPU_CACHE_CLEAN | \
                                                    PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                                    PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC | \
                                                    PVRSRV_MEMALLOCFLAG_POISON_ON_FREE | \
                                                    PVRSRV_MEMALLOCFLAG_PHYS_ALLOC_NOW | \
                                                    PVRSRV_MEMALLOCFLAG_MANDATE_PHYSHEAP | \
                                                    PVRSRV_MEMALLOCFLAG_IPA_POLICY_MASK | \
                                                    PVRSRV_PHYS_HEAP_HINT_MASK)
#else
#define PVRSRV_MEMALLOCFLAGS_DEVMEMX_PHYSICAL_MASK (PVRSRV_MEMALLOCFLAGS_CPU_MMUFLAGSMASK | \
                                                    PVRSRV_MEMALLOCFLAG_GPU_CACHE_MODE_MASK | \
                                                    PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE | \
                                                    PVRSRV_MEMALLOCFLAG_GPU_READABLE | \
                                                    PVRSRV_MEMALLOCFLAG_CPU_READ_PERMITTED | \
                                                    PVRSRV_MEMALLOCFLAG_CPU_WRITE_PERMITTED | \
                                                    PVRSRV_MEMALLOCFLAG_CPU_CACHE_CLEAN | \
                                                    PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                                    PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC | \
                                                    PVRSRV_MEMALLOCFLAG_PHYS_ALLOC_NOW | \
                                                    PVRSRV_MEMALLOCFLAG_MANDATE_PHYSHEAP | \
                                                    PVRSRV_MEMALLOCFLAG_IPA_POLICY_MASK | \
                                                    PVRSRV_PHYS_HEAP_HINT_MASK)
#endif

/*!
  Flags that affect _virtual allocations_ in the DevMemX API
 */
#define PVRSRV_MEMALLOCFLAGS_DEVMEMX_VIRTUAL_MASK  (PVRSRV_MEMALLOCFLAGS_GPU_MMUFLAGSMASK | \
                                                    PVRSRV_MEMALLOCFLAG_GPU_READ_PERMITTED | \
                                                    PVRSRV_MEMALLOCFLAG_GPU_WRITE_PERMITTED)

#endif /* PVRSRV_MEMALLOCFLAGS_H */
