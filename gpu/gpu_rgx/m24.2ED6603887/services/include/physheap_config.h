/*************************************************************************/ /*!
@File           physheap_config.h
@Title          Physical heap Config API
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Physical heap configs are created in the system layer and
                stored against each device node for use in the Services Server
                common layer.
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

#ifndef PHYSHEAP_CONFIG_H
#define PHYSHEAP_CONFIG_H

#include "img_types.h"
#include "pvr_debug.h"
#include "pvrsrv_error.h"
#include "pvrsrv_memallocflags.h"
#include "pvrsrv_memalloc_physheap.h"

typedef IMG_UINT32 PHYS_HEAP_USAGE_FLAGS;

/**
 * ! IMPORTANT !
 * If you update the PHYS_HEAP_USAGE_FLAGS definitions, you must update the
 * g_asPhysHeapUsageFlagStrings structure within physheap.c and the
 * PHYS_HEAP_USAGE_MASK.
 */
#define PHYS_HEAP_USAGE_CPU_LOCAL      (1U <<PVRSRV_PHYS_HEAP_CPU_LOCAL)
#define PHYS_HEAP_USAGE_GPU_LOCAL      (1U <<PVRSRV_PHYS_HEAP_GPU_LOCAL)
#define PHYS_HEAP_USAGE_GPU_PRIVATE    (1U <<PVRSRV_PHYS_HEAP_GPU_PRIVATE)
#define PHYS_HEAP_USAGE_EXTERNAL       (1U <<PVRSRV_PHYS_HEAP_EXTERNAL)
#define PHYS_HEAP_USAGE_GPU_COHERENT   (1U <<PVRSRV_PHYS_HEAP_GPU_COHERENT)
#define PHYS_HEAP_USAGE_GPU_SECURE     (1U <<PVRSRV_PHYS_HEAP_GPU_SECURE)
#define PHYS_HEAP_USAGE_FW_CODE        (1U <<PVRSRV_PHYS_HEAP_FW_CODE)
#define PHYS_HEAP_USAGE_FW_PRIV_DATA   (1U <<PVRSRV_PHYS_HEAP_FW_PRIV_DATA)
#define PHYS_HEAP_USAGE_FW_PREMAP_PT   (1U <<PVRSRV_PHYS_HEAP_FW_PREMAP_PT)
#define PHYS_HEAP_USAGE_FW_PREMAP      (1U <<PVRSRV_PHYS_HEAP_FW_PREMAP0)
#define PHYS_HEAP_USAGE_WRAP           (1U <<PVRSRV_PHYS_HEAP_WRAP)
#define PHYS_HEAP_USAGE_DISPLAY        (1U <<PVRSRV_PHYS_HEAP_DISPLAY)
#define PHYS_HEAP_USAGE_DLM            (1U <<29)
#define PHYS_HEAP_USAGE_FW_SHARED      (1U <<30)
#define PHYS_HEAP_USAGE_FW_PRIVATE     (1U <<31)

#define PHYS_HEAP_USAGE_MASK (PHYS_HEAP_USAGE_CPU_LOCAL    | \
                              PHYS_HEAP_USAGE_GPU_LOCAL    | \
                              PHYS_HEAP_USAGE_GPU_PRIVATE  | \
                              PHYS_HEAP_USAGE_EXTERNAL     | \
                              PHYS_HEAP_USAGE_GPU_COHERENT | \
                              PHYS_HEAP_USAGE_GPU_SECURE   | \
                              PHYS_HEAP_USAGE_FW_CODE      | \
                              PHYS_HEAP_USAGE_FW_PRIV_DATA | \
                              PHYS_HEAP_USAGE_FW_SHARED    | \
                              PHYS_HEAP_USAGE_FW_PRIVATE   | \
                              PHYS_HEAP_USAGE_FW_PREMAP    | \
                              PHYS_HEAP_USAGE_FW_PREMAP_PT | \
                              PHYS_HEAP_USAGE_WRAP         | \
                              PHYS_HEAP_USAGE_DISPLAY      | \
                              PHYS_HEAP_USAGE_DLM)

#define FIRST_PHYSHEAP_MAPPED_TO_FW_MAIN_DEVMEM    PVRSRV_PHYS_HEAP_FW_CODE

typedef void (*CpuPAddrToDevPAddr)(IMG_HANDLE hPrivData,
                                   IMG_UINT32 ui32NumOfAddr,
                                   IMG_DEV_PHYADDR *psDevPAddr,
                                   IMG_CPU_PHYADDR *psCpuPAddr);

typedef void (*DevPAddrToCpuPAddr)(IMG_HANDLE hPrivData,
                                   IMG_UINT32 ui32NumOfAddr,
                                   IMG_CPU_PHYADDR *psCpuPAddr,
                                   IMG_DEV_PHYADDR *psDevPAddr);

/*! Structure used to hold function pointers used for run-time physical address
 * translation by Services. Gives flexibility to allow the CPU and GPU to see
 * the same pool of physical RAM and different physical bus addresses.
 * Both fields must be valid functions even if the conversion is simple.
 */
typedef struct _PHYS_HEAP_FUNCTIONS_
{
	/*! Translate CPU physical address to device physical address */
	CpuPAddrToDevPAddr	pfnCpuPAddrToDevPAddr;
	/*! Translate device physical address to CPU physical address */
	DevPAddrToCpuPAddr	pfnDevPAddrToCpuPAddr;
} PHYS_HEAP_FUNCTIONS;

/*! Structure used to describe the IPA settings for a physical heap.
 */
typedef struct _IPA_CONFIG_
{
	/*! IPA Policy Default (0-7) */
	IMG_UINT8             ui8IPAPolicyDefault;
	/*! IPA Policy Mask (3-bits for this use-case - 0x7) */
	IMG_UINT8             ui8IPAPolicyMask;
	/*! IPA Policy mask shift (LSB position - policy value will be << by this amount */
	IMG_UINT8             ui8IPAPolicyShift;
	/*! IPA Policy reserved byte for alignment and slop reduction */
	IMG_UINT8             ui8IPAPolicyReserved;
} IPA_CONFIG;

typedef struct _PHYS_HEAP_CONFIG_LMA_
{
	IMG_CHAR*             pszPDumpMemspaceName; /*!< Name given to the heap's symbolic memory
													 space in a PDUMP enabled driver */
	PHYS_HEAP_FUNCTIONS*  psMemFuncs;           /*!< Physical address translation functions */
	IMG_CHAR*             pszHeapName;          /*!< Name given to the heap */
	IMG_CPU_PHYADDR       sStartAddr;           /*!< CPU Physical base address of memory region */
	IMG_DEV_PHYADDR       sCardBase;            /*!< Device physical base address of memory
													 region as seen from the PoV of the GPU */
	IMG_UINT64            uiSize;               /*!< Size of memory region in bytes */
	IMG_HANDLE            hPrivData;            /*!< System layer private data shared with
													 psMemFuncs */
} PHYS_HEAP_CONFIG_LMA;

typedef struct _PHYS_HEAP_CONFIG_UMA_
{
	IMG_CHAR*             pszPDumpMemspaceName; /*!< Name given to the heap's symbolic memory
													 space in a PDUMP enabled driver */
	PHYS_HEAP_FUNCTIONS*  psMemFuncs;           /*!< Physical address translation functions */
	IMG_CHAR*             pszHeapName;          /*!< Name given to the heap */
	IMG_DEV_PHYADDR       sCardBase;            /*!< Optional Device physical base address of memory
													 region as seen from the PoV of the GPU */
	IMG_HANDLE            hPrivData;            /*!< System layer private data shared with
													 psMemFuncs */
} PHYS_HEAP_CONFIG_UMA;

typedef struct _PHYS_HEAP_CONFIG_DLM_
{
	IMG_CHAR*             pszHeapName;          /*!< Name given to the heap */
	IMG_UINT32            ui32Log2PMBSize;      /*!< PMB (Physical Memory Block) Log 2 size in bytes
	                                                 for DLM heap. */
	PHYS_HEAP_FUNCTIONS*  psMemFuncs;           /*!< Physical address translation functions */
	IMG_CPU_PHYADDR       sStartAddr;           /*!< CPU Physical base address of memory region */
	IMG_DEV_PHYADDR       sCardBase;            /*!< Device physical base address of memory
	                                                 region as seen from the PoV of the GPU */
	IMG_UINT64            uiSize;               /*!< Size of memory region in bytes */
	IMG_HANDLE            hPrivData;            /*!< System layer private data shared with
                                                     psMemFuncs */
} PHYS_HEAP_CONFIG_DLM;

typedef struct _PHYS_HEAP_CONFIG_IMA_
{
	IMG_CHAR*             pszPDumpMemspaceName; /*!< Name given to the heap's symbolic memory
	                                                 space in a PDUMP enabled driver */
	PHYS_HEAP_FUNCTIONS*  psMemFuncs;           /*!< Physical address translation functions */
	IMG_CHAR*             pszHeapName;          /*!< Name given to the heap */
	IMG_HANDLE            hPrivData;            /*!< System layer private data shared with
	                                                 psMemFuncs */
	IMG_UINT32            ui32PMBStartingMultiple; /*!< Multiple of PMB size defined in DLM heap
	                                                    to be allocated on creation */
	IMG_UINT32            uiDLMHeapIdx;         /*!< The index in the array of physheaps to a
	                                                 DLM heap to import PMBs from. */
} PHYS_HEAP_CONFIG_IMA;

/*! Structure used to describe a physical Heap supported by a system. A
 * system layer module can declare multiple physical heaps for different
 * purposes. At a minimum a system must provide one physical heap tagged for
 * PHYS_HEAP_USAGE_GPU_LOCAL use.
 * A heap represents a discrete pool of physical memory and how it is managed,
 * as well as associating other properties and address translation logic.
 * The structure fields sStartAddr, sCardBase and uiSize must be given valid
 * values for LMA and DMA physical heaps types.
 */
typedef struct _PHYS_HEAP_CONFIG_
{
	PHYS_HEAP_TYPE        eType;                /*!< Class of heap and PMR factory used */
	PHYS_HEAP_USAGE_FLAGS ui32UsageFlags;       /*!< Supported uses flags, conveys the type of
	                                                 buffers the physical heap can be used for */
	IPA_CONFIG            sIPAConfig;           /*!< IPA configuration to be applied to all
	                                                 requested physical addresses when physically
	                                                 backed */
	union {
		PHYS_HEAP_CONFIG_LMA sLMA;
		PHYS_HEAP_CONFIG_LMA sDMA;
		PHYS_HEAP_CONFIG_UMA sUMA;
		PHYS_HEAP_CONFIG_DLM sDLM;
		PHYS_HEAP_CONFIG_IMA sIMA;
#if defined(SUPPORT_WRAP_EXTMEMOBJECT)
		PHYS_HEAP_CONFIG_UMA sWRAP;
#endif
	} uConfig;
} PHYS_HEAP_CONFIG;

/* Code throughout the driver sometimes relies on the fact that the backing for
 * LMA and DMA heaps are the same struct, this check is not comprehensive as the structs
 * could have changed but kept the same overall size, although this is unlikely.
 */
static_assert(sizeof(((PHYS_HEAP_CONFIG*)0)->uConfig.sLMA) == sizeof(((PHYS_HEAP_CONFIG*)0)->uConfig.sDMA),
              "PHYS_HEAP_CONFIG sizeof sLMA != sDMA, Type has potentially changed");

static INLINE IMG_UINT64 PhysHeapConfigGetSize(PHYS_HEAP_CONFIG *psConfig)
{
	switch (psConfig->eType)
	{
	case PHYS_HEAP_TYPE_LMA:
		return psConfig->uConfig.sLMA.uiSize;
	case PHYS_HEAP_TYPE_IMA:
		PVR_ASSERT(!"IMA Config has no size member");
		return 0;
#if defined(__KERNEL__)
	case PHYS_HEAP_TYPE_DMA:
		return psConfig->uConfig.sDMA.uiSize;
#endif
	case PHYS_HEAP_TYPE_DLM:
		return psConfig->uConfig.sDLM.uiSize;
	case PHYS_HEAP_TYPE_UMA:
		PVR_ASSERT(!"UMA Config has no size member");
		return 0;
	default:
		PVR_ASSERT(!"Not Implemented for Config Type");
		return 0;
	}
}

static INLINE IMG_CPU_PHYADDR PhysHeapConfigGetStartAddr(PHYS_HEAP_CONFIG *psConfig)
{
	IMG_CPU_PHYADDR sUnsupportedPhyAddr = {0};
	switch (psConfig->eType)
	{
	case PHYS_HEAP_TYPE_LMA:
		return psConfig->uConfig.sLMA.sStartAddr;
	case PHYS_HEAP_TYPE_IMA:
		PVR_ASSERT(!"IMA Config has no StartAddr member");
		return sUnsupportedPhyAddr;
#if defined(__KERNEL__)
	case PHYS_HEAP_TYPE_DMA:
		return psConfig->uConfig.sDMA.sStartAddr;
#endif
	case PHYS_HEAP_TYPE_DLM:
		return psConfig->uConfig.sDLM.sStartAddr;
	case PHYS_HEAP_TYPE_UMA:
		PVR_ASSERT(!"UMA Config has no StartAddr member");
		return sUnsupportedPhyAddr;
	default:
		PVR_ASSERT(!"Not Implemented for Config Type");
		return sUnsupportedPhyAddr;
	}
}

static INLINE IMG_DEV_PHYADDR PhysHeapConfigGetCardBase(PHYS_HEAP_CONFIG *psConfig)
{
	IMG_DEV_PHYADDR sUnsupportedPhyAddr = {0};
	switch (psConfig->eType)
	{
	case PHYS_HEAP_TYPE_LMA:
		return psConfig->uConfig.sLMA.sCardBase;
	case PHYS_HEAP_TYPE_IMA:
		PVR_ASSERT(!"IMA Config has no CardBase member");
		return sUnsupportedPhyAddr;
#if defined(__KERNEL__)
	case PHYS_HEAP_TYPE_DMA:
		return psConfig->uConfig.sDMA.sCardBase;
#endif
	case PHYS_HEAP_TYPE_UMA:
		return psConfig->uConfig.sUMA.sCardBase;
	case PHYS_HEAP_TYPE_DLM:
		return psConfig->uConfig.sDLM.sCardBase;
	default:
		PVR_ASSERT(!"Not Implemented for Config Type");
		return sUnsupportedPhyAddr;
	}
}

#define PHYS_HEAP_NAME_SIZE 24

#endif
