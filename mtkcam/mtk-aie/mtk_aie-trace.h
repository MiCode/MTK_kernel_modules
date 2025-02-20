// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 * Author: Yuan-Jung Kuo <yuan-jung.kuo@mediatek.com>
 *
 */
#ifndef __MTK_IMG_TRACE_H__
#define __MTK_IMG_TRACE_H__

#include <linux/kernel.h>
#include <linux/trace_events.h>
#define AIE_FTRACE
#ifdef AIE_FTRACE

#define AIE_TRACE_LEN 1024

#define AIE_TRACE_FORCE_BEGIN(fmt, args...) \
	__aie_systrace("B|%d|" fmt "\n", current->tgid, ##args)

#define AIE_TRACE_FORCE_END() \
	__aie_systrace("E\n")

#define AIE_SYSTRACE_BEGIN(fmt, args...) do { \
	if (aie_core_ftrace_enabled()) { \
		AIE_TRACE_FORCE_BEGIN(fmt, ##args); \
	} \
} while (0)

#define AIE_SYSTRACE_END() do { \
	if (aie_core_ftrace_enabled()) { \
		AIE_TRACE_FORCE_END(); \
	} \
} while (0)

bool aie_core_ftrace_enabled(void);
void __aie_systrace(const char *fmt, ...);

#else

#define AIE_SYSTRACE_BEGIN(fmt, args...)
#define AIE_SYSTRACE_END()

#endif

#endif
