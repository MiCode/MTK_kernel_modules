/*************************************************************************/ /*!
@File
@Title          RGX fw interface alignment checks
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Checks to avoid disalignment in RGX fw data structures
                shared with the host
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

#if !defined(RGX_FWIF_ALIGNCHECKS_H)
#define RGX_FWIF_ALIGNCHECKS_H

/* for the offsetof macro */
#if defined(__KERNEL__) && defined(__linux__)
#include <linux/stddef.h>
#else
#include <stddef.h>
#endif

/*!
 ******************************************************************************
 * Alignment UM/FW checks array
 *****************************************************************************/

#define RGXFW_ALIGN_CHECKS_UM_MAX 128U

#if defined(PM_INTERACTIVE_MODE)
#define HWRTDATA_PM_OFFSET  offsetof(RGXFWIF_HWRTDATA, sPMMListDevVAddr),
#define HWRTDATA_HEAPTABLE_OFFSET offsetof(RGXFWIF_HWRTDATA, psVHeapTableDevVAddr),
#else
#define HWRTDATA_PM_OFFSET  offsetof(RGXFWIF_HWRTDATA, sPMRenderStateDevVAddr),
#define HWRTDATA_HEAPTABLE_OFFSET
#endif

#define RGXFW_ALIGN_CHECKS_INIT0									\
		sizeof(RGXFWIF_TRACEBUF),									\
		offsetof(RGXFWIF_TRACEBUF, ui32LogType),					\
		offsetof(RGXFWIF_TRACEBUF, sTraceBuf),						\
		offsetof(RGXFWIF_TRACEBUF, ui32TraceBufSizeInDWords),		\
		offsetof(RGXFWIF_TRACEBUF, ui32TracebufFlags),				\
																	\
		sizeof(RGXFWIF_SYSDATA),									\
		offsetof(RGXFWIF_SYSDATA, ePowState),						\
		offsetof(RGXFWIF_SYSDATA, ui32HWPerfDropCount),				\
		offsetof(RGXFWIF_SYSDATA, ui32LastDropOrdinal),				\
		offsetof(RGXFWIF_SYSDATA, ui32FWFaults),					\
		offsetof(RGXFWIF_SYSDATA, ui32HWRStateFlags),				\
																	\
		sizeof(RGXFWIF_OSDATA),										\
		offsetof(RGXFWIF_OSDATA, ui32HostSyncCheckMark),			\
		offsetof(RGXFWIF_OSDATA, ui32KCCBCmdsExecuted),				\
																	\
		sizeof(RGXFWIF_HWRINFOBUF),									\
		offsetof(RGXFWIF_HWRINFOBUF, aui32HwrDmLockedUpCount),		\
		offsetof(RGXFWIF_HWRINFOBUF, aui32HwrDmOverranCount),		\
		offsetof(RGXFWIF_HWRINFOBUF, aui32HwrDmRecoveredCount),		\
		offsetof(RGXFWIF_HWRINFOBUF, aui32HwrDmFalseDetectCount),	\
																	\
		/* RGXFWIF_CMDTA checks */									\
		sizeof(RGXFWIF_CMDTA),										\
		offsetof(RGXFWIF_CMDTA, sGeomRegs),							\
																	\
		/* RGXFWIF_CMD3D checks */									\
		sizeof(RGXFWIF_CMD3D),										\
		offsetof(RGXFWIF_CMD3D, s3DRegs),							\
																	\
		/* RGXFWIF_CMD_COMPUTE checks */							\
		sizeof(RGXFWIF_CMD_COMPUTE),								\
		offsetof(RGXFWIF_CMD_COMPUTE, sCDMRegs),					\
																	\
		/* RGXFWIF_FREELIST checks */								\
		sizeof(RGXFWIF_FREELIST), \
		offsetof(RGXFWIF_FREELIST, sFreeListBaseDevVAddr),\
		offsetof(RGXFWIF_FREELIST, sFreeListStateDevVAddr),\
		offsetof(RGXFWIF_FREELIST, sFreeListLastGrowDevVAddr),\
		offsetof(RGXFWIF_FREELIST, ui32MaxPages),\
		offsetof(RGXFWIF_FREELIST, ui32CurrentPages),\
									\
		/* RGXFWIF_HWRTDATA checks */					\
		sizeof(RGXFWIF_HWRTDATA), \
		HWRTDATA_PM_OFFSET \
		HWRTDATA_HEAPTABLE_OFFSET \
		offsetof(RGXFWIF_HWRTDATA, apsFreeLists),\
		/*offsetof(RGXFWIF_HWRTDATA, ui64VCECatBase),*/ \
		offsetof(RGXFWIF_HWRTDATA, eState), \
							\
\
		sizeof(RGXFWIF_HWPERF_CTL), \
		offsetof(RGXFWIF_HWPERF_CTL, sBlkCfg), \
		sizeof(RGXFWIF_CMDTDM), \
		offsetof(RGXFWIF_CMDTDM, sTDMRegs)

#define RGXFW_ALIGN_CHECKS_INIT		RGXFW_ALIGN_CHECKS_INIT0



/*!
 ******************************************************************************
 * Alignment KM checks array
 *****************************************************************************/

#define RGXFW_ALIGN_CHECKS_INIT_KM0                         \
		sizeof(RGXFWIF_SYSINIT),                            \
		offsetof(RGXFWIF_SYSINIT, sFaultPhysAddr),          \
		offsetof(RGXFWIF_SYSINIT, sPDSExecBase),            \
		offsetof(RGXFWIF_SYSINIT, sUSCExecBase),            \
		offsetof(RGXFWIF_SYSINIT, asSigBufCtl),             \
		offsetof(RGXFWIF_SYSINIT, sTraceBufCtl),            \
		offsetof(RGXFWIF_SYSINIT, sFwSysData),              \
		                                                    \
		sizeof(RGXFWIF_OSINIT),                             \
		offsetof(RGXFWIF_OSINIT, psKernelCCBCtl),           \
		offsetof(RGXFWIF_OSINIT, psKernelCCB),              \
		offsetof(RGXFWIF_OSINIT, psFirmwareCCBCtl),         \
		offsetof(RGXFWIF_OSINIT, psFirmwareCCB),            \
		offsetof(RGXFWIF_OSINIT, sFwOsData),                \
		offsetof(RGXFWIF_OSINIT, sRGXCompChecks),           \
		                                                    \
		/* RGXFWIF_FWRENDERCONTEXT checks */                \
		sizeof(RGXFWIF_FWRENDERCONTEXT),                    \
		offsetof(RGXFWIF_FWRENDERCONTEXT, sTAContext),      \
		offsetof(RGXFWIF_FWRENDERCONTEXT, s3DContext),      \
		                                                    \
		sizeof(RGXFWIF_FWCOMPUTECONTEXT),                   \
		offsetof(RGXFWIF_FWCOMPUTECONTEXT, sCDMContext),    \
		offsetof(RGXFWIF_FWCOMPUTECONTEXT, sStaticComputeContextState),\
		offsetof(RGXFWIF_FWCOMPUTECONTEXT, ui32WorkEstCCBSubmitted),\
															\
		sizeof(RGXFWIF_FWTDMCONTEXT),                       \
		offsetof(RGXFWIF_FWTDMCONTEXT, sTDMContext),        \
		offsetof(RGXFWIF_FWTDMCONTEXT, ui32WorkEstCCBSubmitted),\
															\
		sizeof(RGXFWIF_FWCOMMONCONTEXT),                    \
		offsetof(RGXFWIF_FWCOMMONCONTEXT, psFWMemContext),  \
		offsetof(RGXFWIF_FWCOMMONCONTEXT, sRunNode),        \
		offsetof(RGXFWIF_FWCOMMONCONTEXT, psCCB),           \
		                                                    \
		sizeof(RGXFWIF_MMUCACHEDATA),                       \
		offsetof(RGXFWIF_MMUCACHEDATA, ui32CacheFlags),     \
		offsetof(RGXFWIF_MMUCACHEDATA, sMMUCacheSync),      \
		offsetof(RGXFWIF_MMUCACHEDATA, ui32MMUCacheSyncUpdateValue)

#define RGXFW_ALIGN_CHECKS_INIT_KM RGXFW_ALIGN_CHECKS_INIT_KM0

#endif /* RGX_FWIF_ALIGNCHECKS_H */

/******************************************************************************
 End of file (rgx_fwif_alignchecks.h)
******************************************************************************/
