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

DECLARE_EVENT_CLASS(imgsys_qof_event,
	TP_PROTO(struct va_format *vaf),
	TP_ARGS(vaf),
	TP_STRUCT__entry(__vstring(msg, vaf->fmt, vaf->va)),
	TP_fast_assign(
		__assign_vstr(msg, vaf->fmt, vaf->va);
	),
	TP_printk("%s", __get_str(msg))
);

#define DEFINE_IMGSYS_QOF_EVENT(name) \
	DEFINE_EVENT(imgsys_qof_event, imgsys__qof_##name, \
		TP_PROTO(struct va_format *vaf), \
		TP_ARGS(vaf) \
	)

DEFINE_IMGSYS_QOF_EVENT(mod0);
void ftrace_imgsys_qof_mod0(const char *fmt, ...);
DEFINE_IMGSYS_QOF_EVENT(mod1);
void ftrace_imgsys_qof_mod1(const char *fmt, ...);
DEFINE_IMGSYS_QOF_EVENT(mod2);
void ftrace_imgsys_qof_mod2(const char *fmt, ...);
DEFINE_IMGSYS_QOF_EVENT(mod3);
void ftrace_imgsys_qof_mod3(const char *fmt, ...);
DEFINE_IMGSYS_QOF_EVENT(mod4);
void ftrace_imgsys_qof_mod4(const char *fmt, ...);

DECLARE_EVENT_CLASS(imgsys_hwqos_event,
	TP_PROTO(struct va_format *vaf),
	TP_ARGS(vaf),
	TP_STRUCT__entry(__vstring(msg, vaf->fmt, vaf->va)),
	TP_fast_assign(
		__assign_vstr(msg, vaf->fmt, vaf->va);
	),
	TP_printk("%s", __get_str(msg))
);

#define DEFINE_IMGSYS_HWQOS_EVENT(name) \
	DEFINE_EVENT(imgsys_hwqos_event, imgsys__hwqos_##name, \
		TP_PROTO(struct va_format *vaf), \
		TP_ARGS(vaf) \
	)

DEFINE_IMGSYS_HWQOS_EVENT(bwr);
DEFINE_IMGSYS_HWQOS_EVENT(bls);
DEFINE_IMGSYS_HWQOS_EVENT(ostdl);

void ftrace_imgsys_hwqos_bwr(const char *fmt, ...);
void ftrace_imgsys_hwqos_bls(const char *fmt, ...);
void ftrace_imgsys_hwqos_ostdl(const char *fmt, ...);

TRACE_EVENT(imgsys__hwqos_dbg_reg_read,
	TP_PROTO(u32 pa, u32 value),
	TP_ARGS(pa, value),
	TP_STRUCT__entry(
		__field(u32, pa)
		__field(u32, value)
	),
	TP_fast_assign(
		__entry->pa = pa;
		__entry->value = value;
	),
	TP_printk("addr:0x%08X, value:0x%08X",
		(int)__entry->pa,
		(int)__entry->value)
);

void ftrace_imgsys_hwqos_dbg_reg_read(u32 pa, u32 value);

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