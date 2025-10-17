/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM mtk_camsys

#if !defined(_MTK_CAM_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _MTK_CAM_TRACE_H

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

TRACE_EVENT(seamless_apply_sensor,
	TP_PROTO(const char *sensor_name,
		 int ctx_id,
		 int frame_seq,
		 bool begin),
	TP_ARGS(sensor_name, ctx_id, frame_seq, begin),
	TP_STRUCT__entry(
		__string(sensor, sensor_name)
		__field(int, ctx_id)
		__field(int, frame_seq)
		__field(bool, begin)
		),
	TP_fast_assign(
		__assign_str(sensor, sensor_name);
		__entry->ctx_id = ctx_id;
		__entry->frame_seq = frame_seq;
		__entry->begin = begin;
		),
	TP_printk("%s ctx=%d frame=0x%x %s",
		  __get_str(sensor), __entry->ctx_id, __entry->frame_seq,
		  __entry->begin ? "begin" : "end"
	)
);

#endif /*_MTK_CAM_TRACE_H */

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE mtk_cam-trace
/* This part must be outside protection */
#include <trace/define_trace.h>


#ifndef __MTK_CAM_TRACE_H
#define __MTK_CAM_TRACE_H

#if IS_ENABLED(CONFIG_TRACING) && defined(MTK_CAM_TRACE_SUPPORT)

#include <linux/sched.h>
#include <linux/kernel.h>

int mtk_cam_trace_enabled_tags(void);

#define _MTK_CAM_TRACE_ENABLED(category)	\
	(mtk_cam_trace_enabled_tags() & (1UL << category))

__printf(1, 2)
void mtk_cam_trace(const char *fmt, ...);

#define _MTK_CAM_TRACE(category, fmt, args...)			\
do {								\
	if (unlikely(_MTK_CAM_TRACE_ENABLED(category)))		\
		mtk_cam_trace(fmt, ##args);			\
} while (0)

#else

#define _MTK_CAM_TRACE_ENABLED(category)	0
#define _MTK_CAM_TRACE(category, fmt, args...)

#endif

enum trace_category {
	TRACE_BASIC,
	TRACE_HW_IRQ,
	TRACE_BUFFER,
	TRACE_FBC,
};

#define _TRACE_CAT(cat)		TRACE_ ## cat

#define MTK_CAM_TRACE_ENABLED(category)	\
	_MTK_CAM_TRACE_ENABLED(_TRACE_CAT(category))

#define MTK_CAM_TRACE(category, fmt, args...)				\
	_MTK_CAM_TRACE(_TRACE_CAT(category), "camsys:" fmt, ##args)

/*
 * systrace format
 */

#define MTK_CAM_TRACE_BEGIN(category, fmt, args...)			\
	_MTK_CAM_TRACE(_TRACE_CAT(category), "B|%d|camsys:" fmt,	\
		      task_tgid_nr(current), ##args)

#define MTK_CAM_TRACE_END(category)					\
	_MTK_CAM_TRACE(_TRACE_CAT(category), "E|%d",			\
		      task_tgid_nr(current))

#define MTK_CAM_TRACE_FUNC_BEGIN(category)				\
	MTK_CAM_TRACE_BEGIN(category, "%s", __func__)

#endif /* __MTK_CAM_TRACE_H */
