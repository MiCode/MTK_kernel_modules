/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM adaptor

#if !defined(_ADAPTOR_TRACE_H__) || defined(TRACE_HEADER_MULTI_READ)
#define _ADAPTOR_TRACE_H__

#include <linux/tracepoint.h>
#include <linux/trace_events.h>

TRACE_EVENT(tracing_mark_write,
	TP_PROTO(const char *fmt, va_list *va),
	TP_ARGS(fmt, va),
	TP_STRUCT__entry(
		__vstring(vstr, fmt, va)
	),
	TP_fast_assign(
		__assign_vstr(vstr, fmt, va);
	),
	TP_printk("%s", __get_str(vstr))
);

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE adaptor-trace
/* This part must be outside protection */

#include <trace/define_trace.h>

#include <linux/kernel.h>
#include <linux/sched.h>

#define ADAPTOR_TRACE_FORCE_BEGIN(catetory, fmt, args...)	\
	__adaptor_systrace("B|%d|%s" fmt, task_tgid_nr(current), catetory, ##args)

#define ADAPTOR_TRACE_FORCE_END()	\
	__adaptor_systrace("E|%d", task_tgid_nr(current))


#define ADAPTOR_SYSTRACE_BEGIN(fmt, args...) do { \
	if (adaptor_trace_enabled()) { \
		ADAPTOR_TRACE_FORCE_BEGIN("adaptor::", fmt, ##args); \
	} \
} while (0)

#define ADAPTOR_SYSTRACE_END() do { \
	if (adaptor_trace_enabled()) { \
		ADAPTOR_TRACE_FORCE_END(); \
	} \
} while (0)

bool inline adaptor_trace_enabled(void);
void __adaptor_systrace(const char *fmt, ...);


#endif  /* _ADAPTOR_TRACE_H__ */
