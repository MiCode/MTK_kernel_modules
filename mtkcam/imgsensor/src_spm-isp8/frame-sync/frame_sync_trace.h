/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _FRAME_SYNC_TRACE_H
#define _FRAME_SYNC_TRACE_H


#define USING_ADAPTOR_TRACE


#ifndef FS_UT
#ifdef USING_ADAPTOR_TRACE
#include "adaptor-trace.h"
#endif
#endif


/******************************************************************************/
// trace CTRL extern variables
/******************************************************************************/
#ifndef FS_UT
/* declare in frame_sync_console.c */
extern unsigned int fs_trace_tags;
#else /* => FS_UT */
#define fs_trace_tags 0xffffffff
#endif


/******************************************************************************/
// trace CTRL define/enum/macro
/******************************************************************************/
enum fs_trace_category {
	/* basic category */
	TRACE_FS_LOG_INF = 0,
	TRACE_FS_BASIC,

	/* extra category */

	/* max category */
	TRACE_FS_CAT_MAX = 26
};

#define TRACE_FS_DEF 1


#define _FS_TRACE_ENABLED(category) \
	((fs_trace_tags) & (1UL << (category)))


#ifdef FS_UT
#define FS_TRACE_BEGIN(trace_cat, fmt, args...)
#define FS_TRACE_FUNC_BEGIN(trace_cat) \
	FS_TRACE_BEGIN(trace_cat, "%s", __func__)
#define FS_TRACE_END()
#define FS_TRACE_PR_LOG_INF(fmt, args...)

#else /* => !FS_UT */
#ifdef USING_ADAPTOR_TRACE
#define FS_TRACE_BEGIN(trace_cat, fmt, args...) \
do { \
	if (unlikely(_FS_TRACE_ENABLED(trace_cat))) { \
		if (adaptor_trace_enabled()) { \
			__adaptor_systrace( \
				"B|%d|%s::" fmt, task_tgid_nr(current), PFX, ##args); \
		} \
	} \
} while (0)

#define FS_TRACE_FUNC_BEGIN(trace_cat) \
	FS_TRACE_BEGIN(trace_cat, "%s", __func__)

#define FS_TRACE_END(trace_cat) \
do { \
	if (unlikely(_FS_TRACE_ENABLED(trace_cat))) { \
		if (adaptor_trace_enabled()) { \
			__adaptor_systrace("E|%d", task_tgid_nr(current)); \
		} \
	} \
} while (0)

#define FS_TRACE_PR_LOG_INF(fmt, args...) \
do { \
	if (likely(_FS_TRACE_ENABLED(TRACE_FS_LOG_INF))) { \
		if (adaptor_trace_enabled()) { \
			__adaptor_systrace( \
				"B|%d|%s[%s]" fmt, task_tgid_nr(current), PFX, __func__, ##args); \
			__adaptor_systrace("E|%d", task_tgid_nr(current)); \
		} \
	} \
} while (0)

#endif // USING_ADAPTOR_TRACE
#endif // FS_UT


#endif /* _FRAME_SYNC_TRACE_H */
