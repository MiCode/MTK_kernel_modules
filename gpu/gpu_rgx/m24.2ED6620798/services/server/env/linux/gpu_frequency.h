/*************************************************************************/ /*!
@File
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
#undef TRACE_SYSTEM
#define TRACE_SYSTEM power

#if !defined(TRACE_GPU_FREQUENCY_H) || defined(TRACE_HEADER_MULTI_READ)
#define TRACE_GPU_FREQUENCY_H

#include <linux/tracepoint.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
int PVRGpuTraceEnableFreqCallback(void);
#else
void PVRGpuTraceEnableFreqCallback(void);
#endif
void PVRGpuTraceDisableFreqCallback(void);

/*
 * gpu_frequency - Reports the GPU frequency in GPU clock domains.
 *
 * @state : New frequency (in KHz)
 * @gpu_id: Id for each GPU clock domain
 */
TRACE_EVENT_FN(gpu_frequency,

	TP_PROTO(uint32_t state, uint32_t gpu_id),

	TP_ARGS(state, gpu_id),

	TP_STRUCT__entry(
		__field(unsigned int, state)
		__field(unsigned int, gpu_id)
	),

	TP_fast_assign(
		__entry->state = state;
		__entry->gpu_id = gpu_id;
	),

	TP_printk("state=%u gpu_id=%u",
		__entry->state, __entry->gpu_id),

	PVRGpuTraceEnableFreqCallback,
	PVRGpuTraceDisableFreqCallback
);

#endif /* TRACE_GPU_FREQUENCY_H */

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .

/* This is needed because the name of this file doesn't match TRACE_SYSTEM. */
#define TRACE_INCLUDE_FILE gpu_frequency

/* This part must be outside protection */
#include <trace/define_trace.h>
