/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _FRAME_SYNC_LOG_H
#define _FRAME_SYNC_LOG_H

#ifndef FS_UT
#include <linux/printk.h>		/* for kernel log reduction */
#include <linux/mutex.h>
#include <linux/spinlock.h>
#else
#include <stdio.h>			/* printf */
#endif


/******************************************************************************/
// log CTRL extern variables
/******************************************************************************/
#ifndef FS_UT
/* declare in frame_sync_console.c */
extern unsigned int fs_log_tracer;

extern struct mutex fs_log_concurrency_lock;
#else /* => FS_UT */
#define fs_log_tracer 0xffffffff
#endif


/******************************************************************************/
// Log CTRL define/enum/macro
/******************************************************************************/
// guess the max length of the mobile log is 1000 characters
// , reserve 200 characters (for time, func name, etc.) first.
#define LOG_BUF_STR_LEN 800

#if defined(FS_UT)
#define TRACE_FS_FREC_LOG
#else
// #define TRACE_FS_FREC_LOG
#endif

enum fs_log_ctrl_category {
	/* basic category */
	LOG_FS_PF = 0,
	LOG_FS,
	LOG_FS_ALGO,
	LOG_FRM,
	LOG_SEN_REC,
	LOG_FS_UTIL,

	/* custom category */
	LOG_FS_ALGO_FPS_INFO,
	LOG_SEN_REC_SEAMLESS_DUMP,

	/* extra category */
	LOG_FS_USER_QUERY_INFO = 25,
	LOG_DISABLE = 26,

	/* max category */
	LOG_FS_CTRL_CAT_MAX = 26
};

#define LOG_TRACER_DEF 0

#define _FS_LOG_ENABLED(category) \
	((fs_log_tracer) & (1UL << (category)))


/******************************************************************************/
// Log message macro
/******************************************************************************/
#ifdef FS_UT
#define LOG_INF(format, args...) printf(PFX "[%s] " format, __func__, ##args)
#define LOG_INF_CAT(log_cat, format, args...) printf(PFX "[%s] " format, __func__, ##args)
#define LOG_PF_INF(format, args...) printf(PFX "[%s] " format, __func__, ##args)
#define LOG_MUST(format, args...) printf(PFX "[%s] " format, __func__, ##args)
#define LOG_INF_CAT_LOCK(log_cat, format, args...) printf(PFX "[%s] " format, __func__, ##args)
#define LOG_PF_INF_LOCK(format, args...) printf(PFX "[%s] " format, __func__, ##args)
#define LOG_MUST_LOCK(format, args...) printf(PFX "[%s] " format, __func__, ##args)
#define LOG_PR_WARN(format, args...) printf(PFX "[%s] " format, __func__, ##args)
#define LOG_PR_ERR(format, args...) printf(PFX "[%s] " format, __func__, ##args)

#else /* => !FS_UT */

#define DY_INFO(log_cat, format, args...) \
do { \
	if (unlikely(!_FS_LOG_ENABLED(LOG_DISABLE))) { \
		if (unlikely(_FS_LOG_ENABLED(log_cat))) { \
			pr_info(PFX "[%s] " format, __func__, ##args); \
		} \
	} \
} while (0)

#define DY_INFO_LOCK(log_cat, format, args...) \
do { \
	if (unlikely(!_FS_LOG_ENABLED(LOG_DISABLE))) { \
		if (unlikely(_FS_LOG_ENABLED(log_cat))) { \
			mutex_lock(&fs_log_concurrency_lock); \
			pr_info(PFX "[%s] " format, __func__, ##args); \
			mutex_unlock(&fs_log_concurrency_lock); \
		} \
	} \
} while (0)

#define LOG_INF(format, args...) DY_INFO(FS_LOG_DBG_DEF_CAT, format, args)
#define LOG_INF_CAT(log_cat, format, args...) DY_INFO(log_cat, format, args)
#define LOG_PF_INF(format, args...) DY_INFO(LOG_FS_PF, format, args)
#define LOG_MUST(format, args...) \
do { \
	if (unlikely(!_FS_LOG_ENABLED(LOG_DISABLE))) { \
		pr_info(PFX "[%s] " format, __func__, ##args); \
	} \
} while (0)

#define LOG_INF_CAT_LOCK(log_cat, format, args...) DY_INFO_LOCK(log_cat, format, args)
#define LOG_PF_INF_LOCK(format, args...) DY_INFO_LOCK(LOG_FS_PF, format, args)
#define LOG_MUST_LOCK(format, args...) \
do { \
	if (unlikely(!_FS_LOG_ENABLED(LOG_DISABLE))) { \
		mutex_lock(&fs_log_concurrency_lock); \
		pr_info(PFX "[%s] " format, __func__, ##args); \
		mutex_unlock(&fs_log_concurrency_lock); \
	} \
} while (0)


#define LOG_PR_WARN(format, args...) pr_warn(PFX "[%s] " format, __func__, ##args)
#define LOG_PR_ERR(format, args...) pr_err(PFX "[%s] " format, __func__, ##args)
#endif // FS_UT


#define FS_SNPRF(buf_len, buf, len, fmt, ...) \
do { \
	int ret; \
	ret = snprintf((buf + len), (buf_len - len), fmt, ##__VA_ARGS__); \
	if (unlikely((ret < 0) || (ret >= (buf_len - len)))) { \
		LOG_PF_INF( \
			"WARNING: snprintf ret:%d, space:%u(truncated), set len to buf_len:%u\n", \
			ret, buf_len - len, buf_len); \
		len = buf_len; \
	} else \
		len += ret; \
} while (0)

/******************************************************************************/


#endif
