/*************************************************************************/ /*!
@File
@Title          Common debug definitions and functions.
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

#ifndef DEBUG_COMMON_H
#define DEBUG_COMMON_H

#include "pvrsrv_error.h"
#include "device.h"

PVRSRV_ERROR DebugCommonInitDriver(void);
void DebugCommonDeInitDriver(void);

const IMG_CHAR *PVRSRVGetDebugDevStateString(PVRSRV_DEVICE_STATE eDevState);
const IMG_CHAR *PVRSRVGetDebugHealthStatusString(PVRSRV_DEVICE_HEALTH_STATUS eHealthStatus);
const IMG_CHAR *PVRSRVGetDebugHealthReasonString(PVRSRV_DEVICE_HEALTH_REASON eHealthReason);

PVRSRV_ERROR DebugCommonInitDevice(PVRSRV_DEVICE_NODE *psDeviceNode);
void DebugCommonDeInitDevice(PVRSRV_DEVICE_NODE *psDeviceNode);

typedef struct _IMG_FLAGS2DESC_
{
	IMG_UINT32		uiFlag;
	const IMG_CHAR	*pszLabel;
} IMG_FLAGS2DESC;

void DebugCommonFlagStrings(IMG_CHAR *psDesc,
							IMG_UINT32 ui32DescSize,
							const IMG_FLAGS2DESC *psConvTable,
							IMG_UINT32 ui32TableSize,
							IMG_UINT32 ui32Flags);

#endif /* DEBUG_COMMON_H */
