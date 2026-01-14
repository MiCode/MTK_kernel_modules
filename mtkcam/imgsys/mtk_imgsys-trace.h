/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Johnson-CH Chiu <Johnson-CH.chiu@mediatek.com>
 *
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM mtk_imgsys

#if !defined(__MTK_IMG_TRACE_H__) || defined(TRACE_HEADER_MULTI_READ)
#define __MTK_IMG_TRACE_H__

#include <linux/kernel.h>
#include <linux/tracepoint.h>
#include <linux/trace_events.h>
#define IMGSYS_FTRACE
#ifdef IMGSYS_FTRACE

#define IMGSYS_TRACE_LEN 1024

TRACE_EVENT(tracing_mark_write,
	TP_PROTO(char *s),
	TP_ARGS(s),
	TP_STRUCT__entry(
		__array(char, s, IMGSYS_TRACE_LEN)
	),
	TP_fast_assign(
	if (snprintf(__entry->s, IMGSYS_TRACE_LEN, "%s", s) < 0)
		__entry->s[0] = '\0';
	),
	TP_printk("%s", __entry->s)
);

#define IMGSYS_TRACE_FORCE_BEGIN(fmt, args...) \
	__imgsys_systrace("B|%d|" fmt "\n", current->tgid, ##args)

#define IMGSYS_TRACE_FORCE_END() \
	__imgsys_systrace("E\n")

#define IMGSYS_SYSTRACE_BEGIN(fmt, args...) do { \
	if (imgsys_core_ftrace_enabled()) { \
		IMGSYS_TRACE_FORCE_BEGIN(fmt, ##args); \
	} \
} while (0)

#define IMGSYS_SYSTRACE_END() do { \
	if (imgsys_core_ftrace_enabled()) { \
		IMGSYS_TRACE_FORCE_END(); \
	} \
} while (0)

bool imgsys_core_ftrace_enabled(void);
void __imgsys_systrace(const char *fmt, ...);

#else

#define IMGSYS_SYSTRACE_BEGIN(fmt, args...)
#define IMGSYS_SYSTRACE_END()

#endif

#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE mtk_imgsys-trace
/* This part must be outside protection */
#include <trace/define_trace.h>