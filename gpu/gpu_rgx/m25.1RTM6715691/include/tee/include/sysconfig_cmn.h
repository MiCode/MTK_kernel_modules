/*****************************************************************************
@File
@Title          System layer helpers that can be used in system layers
@Codingstyle    IMG
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares the signatures for system layer helper functions.
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
******************************************************************************/

#ifndef SYSCONFIG_CMN_H
#define SYSCONFIG_CMN_H

#include "img_types.h"
#include "physheap_config.h"

/**************************************************************************/ /*!
@Function       SysRGXErrorNotify
@Description    Error reporting callback function, registered as the
                pfnSysDevErrorNotify member of the PVRSRV_DEVICE_CONFIG
                struct. System layer will be notified of device errors and
                resets via this callback.
                NB. implementers should ensure that the minimal amount of
                work is done in this callback function, as it will be
                executed in the main RGX MISR. (e.g. any blocking or lengthy
                work should be performed by a worker queue/thread instead).
@Input          hSysData      pointer to the system data of the device
@Output         psErrorData   structure containing details of the reported error
@Return         None.
*/ /***************************************************************************/
void SysRGXErrorNotify(IMG_HANDLE hSysData,
                       PVRSRV_ROBUSTNESS_NOTIFY_DATA *psErrorData);

/**************************************************************************/ /*!
@Function       SysRestrictGpuLocalPhysheap
@Description    If the Restriction apphint has been set, validate the
                restriction value and return the new GPU_LOCAL heap size.

@Input          uiHeapSize      Current syslayer detected GPU_LOCAL heap size.
@Return         IMG_UINT64      New GPU_LOCAL heap size in bytes.
*/ /***************************************************************************/
IMG_UINT64 SysRestrictGpuLocalPhysheap(IMG_UINT64 uiHeapSize);

/**************************************************************************/ /*!
@Function       SysRestrictGpuLocalAddPrivateHeap
@Description    Determine if the restriction apphint has been set.

@Return         IMG_BOOL        IMG_TRUE if the restriction apphint has been
                                set.
*/ /***************************************************************************/
IMG_BOOL SysRestrictGpuLocalAddPrivateHeap(void);

/**************************************************************************/ /*!
@Function       SysDefaultToCpuLocalHeap
@Description    Determine if the Default Heap should be CPU_LOCAL
                Can only be used on TC_MEMORY_HYBRID systems.

@Return         IMG_BOOL        IMG_TRUE if the Default heap apphint has been
                                set.
*/ /***************************************************************************/
IMG_BOOL SysDefaultToCpuLocalHeap(void);

/*************************************************************************/ /*!
@Description    Structure defining nop heap functions.
                Useful for UMA system layers.
*/ /**************************************************************************/
extern PHYS_HEAP_FUNCTIONS g_sUmaHeapFns;

#if defined(SUPPORT_NATIVE_FENCE_SYNC)
IMG_BOOL SysDevExtractFFToken(IMG_HANDLE hSysData,
                              IMG_HANDLE hEnvFenceObjPtr,
                              IMG_UINT16 *pui16FFToken);
#endif

#endif /* SYSCONFIG_CMN_H */
