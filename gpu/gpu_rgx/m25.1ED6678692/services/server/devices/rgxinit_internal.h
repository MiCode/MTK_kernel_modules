/*************************************************************************/ /*!
@File           rgxinit_internal.h
@Title          RGX initialisation header file
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Header for common RGX initialisation functions used in
                architecture specific rgxinit.c files. Functions declared in
                this header would usually be found statically defined within an
                architecture specific rgxinit.c file.
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

#ifndef RGX_INIT_INTERNAL_H
#define RGX_INIT_INTERNAL_H

#include "device.h"
#include "rgxdevice.h"

/*************************************************************************/ /*!
@Function       RGXDevVersionString
@Description    Gets the version string for the given device node and returns
                a pointer to it in ppszVersionString. It is then the
                responsibility of the caller to free this memory.
@Input          psDeviceNode        Device node from which to obtain the
                                    version string
@Output	        ppszVersionString   Contains the version string upon return
@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RGXDevVersionString(PVRSRV_DEVICE_NODE *psDeviceNode,
                                 IMG_CHAR **ppszVersionString);

/**************************************************************************/ /*!
@Function       RGXDevClockSpeed
@Description    Gets the clock speed for the given device node and returns
                it in pui32RGXClockSpeed.
@Input          psDeviceNode        Device node
@Output         pui32RGXClockSpeed  Variable for storing the clock speed
@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RGXDevClockSpeed(PVRSRV_DEVICE_NODE *psDeviceNode,
                              IMG_PUINT32  pui32RGXClockSpeed);

#if !defined(NO_HARDWARE)
/*************************************************************************/ /*!
@Function       SampleIRQCount
@Description    Utility function taking snapshots of RGX FW interrupt count.
@Input          psDevInfo    Device Info structure

@Return         IMG_BOOL     Returns IMG_TRUE if RGX FW IRQ is not equal to
                             sampled RGX FW IRQ count for any RGX FW thread.
 */ /*************************************************************************/
IMG_BOOL SampleIRQCount(PVRSRV_RGXDEV_INFO *psDevInfo);

IMG_BOOL RGXAckHwIrq(PVRSRV_RGXDEV_INFO *psDevInfo,
                     IMG_UINT32 ui32IRQStatusReg,
                     IMG_UINT32 ui32IRQStatusEventMsk,
                     IMG_UINT32 ui32IRQClearReg,
                     IMG_UINT32 ui32IRQClearMask);

IMG_BOOL RGXAckIrqDedicated(PVRSRV_RGXDEV_INFO *psDevInfo);
#endif

#endif /* RGX_INIT_INTERNAL_H */
