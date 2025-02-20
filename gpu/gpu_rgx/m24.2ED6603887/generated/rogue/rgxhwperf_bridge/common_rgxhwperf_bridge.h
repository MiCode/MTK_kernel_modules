/*******************************************************************************
@File
@Title          Common bridge header for rgxhwperf
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for rgxhwperf
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
*******************************************************************************/

#ifndef COMMON_RGXHWPERF_BRIDGE_H
#define COMMON_RGXHWPERF_BRIDGE_H

#include <powervr/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "pvrsrv_error.h"

#include "rgx_bridge.h"
#include "rgx_hwperf.h"

#define PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST			0
#define PVRSRV_BRIDGE_RGXHWPERF_RGXGETCONFIGUREDHWPERFCOUNTERS			PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+0
#define PVRSRV_BRIDGE_RGXHWPERF_RGXGETENABLEDHWPERFBLOCKS			PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+1
#define PVRSRV_BRIDGE_RGXHWPERF_RGXGETHWPERFTIMESTAMP			PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+2
#define PVRSRV_BRIDGE_RGXHWPERF_RGXCTRLHWPERF			PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+3
#define PVRSRV_BRIDGE_RGXHWPERF_RGXGETHWPERFBVNCFEATUREFLAGS			PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+4
#define PVRSRV_BRIDGE_RGXHWPERF_RGXCONTROLHWPERFBLOCKS			PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+5
#define PVRSRV_BRIDGE_RGXHWPERF_RGXCONFIGMUXHWPERFCOUNTERS			PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+6
#define PVRSRV_BRIDGE_RGXHWPERF_RGXCONFIGCUSTOMCOUNTERS			PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+7
#define PVRSRV_BRIDGE_RGXHWPERF_RGXCONFIGUREHWPERFBLOCKS			PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+8
#define PVRSRV_BRIDGE_RGXHWPERF_RGXGETCONFIGUREDHWPERFMUXCOUNTERS			PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+9
#define PVRSRV_BRIDGE_RGXHWPERF_CMD_LAST			(PVRSRV_BRIDGE_RGXHWPERF_CMD_FIRST+9)

/*******************************************
            RGXGetConfiguredHWPerfCounters
 *******************************************/

/* Bridge in structure for RGXGetConfiguredHWPerfCounters */
typedef struct PVRSRV_BRIDGE_IN_RGXGETCONFIGUREDHWPERFCOUNTERS_TAG
{
	RGX_HWPERF_CONFIG_CNTBLK *psConfiguredCounters;
	IMG_UINT32 ui32BlockID;
} __packed PVRSRV_BRIDGE_IN_RGXGETCONFIGUREDHWPERFCOUNTERS;

/* Bridge out structure for RGXGetConfiguredHWPerfCounters */
typedef struct PVRSRV_BRIDGE_OUT_RGXGETCONFIGUREDHWPERFCOUNTERS_TAG
{
	RGX_HWPERF_CONFIG_CNTBLK *psConfiguredCounters;
	PVRSRV_ERROR eError;
} __packed PVRSRV_BRIDGE_OUT_RGXGETCONFIGUREDHWPERFCOUNTERS;

/*******************************************
            RGXGetEnabledHWPerfBlocks
 *******************************************/

/* Bridge in structure for RGXGetEnabledHWPerfBlocks */
typedef struct PVRSRV_BRIDGE_IN_RGXGETENABLEDHWPERFBLOCKS_TAG
{
	IMG_UINT32 *pui32EnabledBlockIDs;
	IMG_UINT32 ui32ArrayLen;
} __packed PVRSRV_BRIDGE_IN_RGXGETENABLEDHWPERFBLOCKS;

/* Bridge out structure for RGXGetEnabledHWPerfBlocks */
typedef struct PVRSRV_BRIDGE_OUT_RGXGETENABLEDHWPERFBLOCKS_TAG
{
	IMG_UINT32 *pui32EnabledBlockIDs;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32BlockCount;
} __packed PVRSRV_BRIDGE_OUT_RGXGETENABLEDHWPERFBLOCKS;

/*******************************************
            RGXGetHWPerfTimeStamp
 *******************************************/

/* Bridge in structure for RGXGetHWPerfTimeStamp */
typedef struct PVRSRV_BRIDGE_IN_RGXGETHWPERFTIMESTAMP_TAG
{
	IMG_UINT32 ui32EmptyStructPlaceholder;
} __packed PVRSRV_BRIDGE_IN_RGXGETHWPERFTIMESTAMP;

/* Bridge out structure for RGXGetHWPerfTimeStamp */
typedef struct PVRSRV_BRIDGE_OUT_RGXGETHWPERFTIMESTAMP_TAG
{
	IMG_UINT64 ui64TimeStamp;
	PVRSRV_ERROR eError;
} __packed PVRSRV_BRIDGE_OUT_RGXGETHWPERFTIMESTAMP;

/*******************************************
            RGXCtrlHWPerf
 *******************************************/

/* Bridge in structure for RGXCtrlHWPerf */
typedef struct PVRSRV_BRIDGE_IN_RGXCTRLHWPERF_TAG
{
	IMG_UINT64 ui64Mask;
	IMG_UINT32 ui32StreamId;
	IMG_BOOL bToggle;
} __packed PVRSRV_BRIDGE_IN_RGXCTRLHWPERF;

/* Bridge out structure for RGXCtrlHWPerf */
typedef struct PVRSRV_BRIDGE_OUT_RGXCTRLHWPERF_TAG
{
	PVRSRV_ERROR eError;
} __packed PVRSRV_BRIDGE_OUT_RGXCTRLHWPERF;

/*******************************************
            RGXGetHWPerfBvncFeatureFlags
 *******************************************/

/* Bridge in structure for RGXGetHWPerfBvncFeatureFlags */
typedef struct PVRSRV_BRIDGE_IN_RGXGETHWPERFBVNCFEATUREFLAGS_TAG
{
	IMG_UINT32 ui32EmptyStructPlaceholder;
} __packed PVRSRV_BRIDGE_IN_RGXGETHWPERFBVNCFEATUREFLAGS;

/* Bridge out structure for RGXGetHWPerfBvncFeatureFlags */
typedef struct PVRSRV_BRIDGE_OUT_RGXGETHWPERFBVNCFEATUREFLAGS_TAG
{
	RGX_HWPERF_BVNC sBVNC;
	PVRSRV_ERROR eError;
} __packed PVRSRV_BRIDGE_OUT_RGXGETHWPERFBVNCFEATUREFLAGS;

/*******************************************
            RGXControlHWPerfBlocks
 *******************************************/

/* Bridge in structure for RGXControlHWPerfBlocks */
typedef struct PVRSRV_BRIDGE_IN_RGXCONTROLHWPERFBLOCKS_TAG
{
	IMG_UINT16 *pui16BlockIDs;
	IMG_UINT32 ui32ArrayLen;
	IMG_BOOL bEnable;
} __packed PVRSRV_BRIDGE_IN_RGXCONTROLHWPERFBLOCKS;

/* Bridge out structure for RGXControlHWPerfBlocks */
typedef struct PVRSRV_BRIDGE_OUT_RGXCONTROLHWPERFBLOCKS_TAG
{
	PVRSRV_ERROR eError;
} __packed PVRSRV_BRIDGE_OUT_RGXCONTROLHWPERFBLOCKS;

/*******************************************
            RGXConfigMuxHWPerfCounters
 *******************************************/

/* Bridge in structure for RGXConfigMuxHWPerfCounters */
typedef struct PVRSRV_BRIDGE_IN_RGXCONFIGMUXHWPERFCOUNTERS_TAG
{
	RGX_HWPERF_CONFIG_MUX_CNTBLK *psBlockConfigs;
	IMG_UINT32 ui32ArrayLen;
} __packed PVRSRV_BRIDGE_IN_RGXCONFIGMUXHWPERFCOUNTERS;

/* Bridge out structure for RGXConfigMuxHWPerfCounters */
typedef struct PVRSRV_BRIDGE_OUT_RGXCONFIGMUXHWPERFCOUNTERS_TAG
{
	PVRSRV_ERROR eError;
} __packed PVRSRV_BRIDGE_OUT_RGXCONFIGMUXHWPERFCOUNTERS;

/*******************************************
            RGXConfigCustomCounters
 *******************************************/

/* Bridge in structure for RGXConfigCustomCounters */
typedef struct PVRSRV_BRIDGE_IN_RGXCONFIGCUSTOMCOUNTERS_TAG
{
	IMG_UINT32 *pui32CustomCounterIDs;
	IMG_UINT16 ui16CustomBlockID;
	IMG_UINT16 ui16NumCustomCounters;
} __packed PVRSRV_BRIDGE_IN_RGXCONFIGCUSTOMCOUNTERS;

/* Bridge out structure for RGXConfigCustomCounters */
typedef struct PVRSRV_BRIDGE_OUT_RGXCONFIGCUSTOMCOUNTERS_TAG
{
	PVRSRV_ERROR eError;
} __packed PVRSRV_BRIDGE_OUT_RGXCONFIGCUSTOMCOUNTERS;

/*******************************************
            RGXConfigureHWPerfBlocks
 *******************************************/

/* Bridge in structure for RGXConfigureHWPerfBlocks */
typedef struct PVRSRV_BRIDGE_IN_RGXCONFIGUREHWPERFBLOCKS_TAG
{
	RGX_HWPERF_CONFIG_CNTBLK *psBlockConfigs;
	IMG_UINT32 ui32ArrayLen;
	IMG_UINT32 ui32CtrlWord;
} __packed PVRSRV_BRIDGE_IN_RGXCONFIGUREHWPERFBLOCKS;

/* Bridge out structure for RGXConfigureHWPerfBlocks */
typedef struct PVRSRV_BRIDGE_OUT_RGXCONFIGUREHWPERFBLOCKS_TAG
{
	PVRSRV_ERROR eError;
} __packed PVRSRV_BRIDGE_OUT_RGXCONFIGUREHWPERFBLOCKS;

/*******************************************
            RGXGetConfiguredHWPerfMuxCounters
 *******************************************/

/* Bridge in structure for RGXGetConfiguredHWPerfMuxCounters */
typedef struct PVRSRV_BRIDGE_IN_RGXGETCONFIGUREDHWPERFMUXCOUNTERS_TAG
{
	RGX_HWPERF_CONFIG_MUX_CNTBLK *psConfiguredMuxCounters;
	IMG_UINT32 ui32BlockID;
} __packed PVRSRV_BRIDGE_IN_RGXGETCONFIGUREDHWPERFMUXCOUNTERS;

/* Bridge out structure for RGXGetConfiguredHWPerfMuxCounters */
typedef struct PVRSRV_BRIDGE_OUT_RGXGETCONFIGUREDHWPERFMUXCOUNTERS_TAG
{
	RGX_HWPERF_CONFIG_MUX_CNTBLK *psConfiguredMuxCounters;
	PVRSRV_ERROR eError;
} __packed PVRSRV_BRIDGE_OUT_RGXGETCONFIGUREDHWPERFMUXCOUNTERS;

#endif /* COMMON_RGXHWPERF_BRIDGE_H */
