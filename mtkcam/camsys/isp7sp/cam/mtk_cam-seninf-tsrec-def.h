/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2022 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_TSREC_DEF_H__
#define __MTK_CAM_SENINF_TSREC_DEF_H__

#ifdef FS_UT
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>                  /* Needed by memory allocate */
#else
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/printk.h>
#include <linux/atomic.h>
#include <linux/string.h>
// #include <linux/slab.h>           /* Needed by memory allocate */
#include <linux/device.h>            /* Needed by devm_* function */
#include <linux/pm_runtime.h>
#endif


/*******************************************************************************
 * TSREC common utilities macro
 ******************************************************************************/
#define TSREC_BIT_MASK(n) (((n) == 64) ? (~0ULL) : ((1ULL<<(n))-1))

#define TSREC_SHOW(buf, len, fmt, ...) { \
	len += snprintf(buf + len, PAGE_SIZE - len, fmt, ##__VA_ARGS__); \
}


/******************************************************************************
 * TSREC spec., hardware define
 *****************************************************************************/
/*
 * ISP7s : TSREC HW does not have global timer.
 * ISP7s+: TSREC HW with global timer (use global timer => 64-bits timestamp).
 */
#define TSREC_WITH_GLOBAL_TIMER      (1)

#if (TSREC_WITH_GLOBAL_TIMER)
// default do not use global timer
#define TSREC_USE_GLOBAL_TIMER       (0)
#else
#define TSREC_USE_GLOBAL_TIMER       (0)
#endif

#if (TSREC_USE_GLOBAL_TIMER)
#define TSREC_WITH_64_BITS_TIMER_RG  (1)
#define tsrec_tick_t                 unsigned long long
#else
#define TSREC_WITH_64_BITS_TIMER_RG  (0)
#define tsrec_tick_t                 unsigned int
#endif

#define TSREC_TICK_FACTOR            (208)

#define TSREC_EXP_MAX_CNT            (3)

#define TSREC_TS_REC_MAX_CNT         (4)


/******************************************************************************
 * TSREC software define/macro
 *****************************************************************************/
#define SENINF_IDX_NONE              (255)
#define TSREC_NO_NONE                (255)


#define TSREC_VC_NONE                (0x1f)

/* TSREC_TRIG_SRC: Vsync => 0; 1st-Hsync => 1 */
#define TSREC_EXP_TRIG_SRC_DEF       (0)


/* the EXP that show changes in sensor frame length */
/* --- exp id : 1 / 2 / 3 ===> exp num : 0 / 1 / 2 --- */
#define TSREC_1ST_EXP_NUM            (TSREC_EXP_ID_1 - 1)

/* ONLY enable first EXP interrupt if has signal */
#define TSREC_INTR_EXP_EN_MASK       (1U << TSREC_1ST_EXP_NUM)
/* enable ALL EXP interrupt if has signal */
// #define TSREC_INTR_EXP_EN_MASK       (TSREC_BIT_MASK(TSREC_EXP_MAX_CNT))

#define TSREC_INTR_W_CLR_EN          (0)


/* TSREC interrupt status related */
#define TSREC_INTR_CHK_MASK \
	((TSREC_BIT_MASK(TSREC_EXP_MAX_CNT)) \
		| (TSREC_BIT_MASK(TSREC_EXP_MAX_CNT) << 16))

/* tsrec a~d (0~3) */
#define TSREC_INTR_EN_MAX_NUM        (4)
/* tsrec e~f (4~5) */
#define TSREC_INTR_EN_2_MAX_NUM      (6)


/* using kthread or workqueue */
#define TSREC_WORK_USING_KTHREAD


/*---------------------------------------------------------------------------*/
// TSREC console define/marco
/*---------------------------------------------------------------------------*/
/* for decode tsrec console variable from user */
/* --> (_ _|_ _ _ _ _ _ _ _) */
#define TSREC_CON_CMD_ID_BASE        (100000000)
#define TSREC_CON_CMD_ID_MOD         (100)
#define TSREC_CON_CMD_VAL_BASE       (1)
#define TSREC_CON_CMD_VAL_MOD        (100000000)

/* --> for struct tsrec_con_cfg */
/* --> (_ _|_|_ _|_ _ _ _ _) */
#define TSREC_CON_CFG_EN_BASE        (10000000)
#define TSREC_CON_CFG_EN_MOD         (10)
#define TSREC_CON_CFG_SIDX_BASE      (100000)
#define TSREC_CON_CFG_SIDX_MOD       (100)
#define TSREC_CON_CFG_VAL_BASE       (1)
#define TSREC_CON_CFG_VAL_MOD        (100000)


/******************************************************************************
 * TSREC utilities macro
 *****************************************************************************/
#define TSREC_US_TO_TICK(x) \
	((TSREC_TICK_FACTOR) ? ((x) * TSREC_TICK_FACTOR) : (x))

#define TSREC_TICK_TO_US(x) \
	((TSREC_TICK_FACTOR) ? ((x) / TSREC_TICK_FACTOR) : (x))

#define TSREC_RING_BACK(x, n) \
	(((x)+(TSREC_TS_REC_MAX_CNT-((n)%TSREC_TS_REC_MAX_CNT))) \
		& (TSREC_TS_REC_MAX_CNT-1))

#define TSREC_RING_FORWARD(x, n) \
	(((x)+((n)%TSREC_TS_REC_MAX_CNT)) & (TSREC_TS_REC_MAX_CNT-1))


/******************************************************************************
 * TSREC log define/macro
 *****************************************************************************/
// guess the max length of the mobile log is 1000 characters
// , reserve 200 characters (for time, func name, etc.) first.
#define TSREC_LOG_BUF_STR_LEN        (800)

/* for reducing log */
#define REDUCE_TSREC_LOG
#define REDUCE_TSREC_REGS_LOG
#ifndef FS_UT
// prevent printing log in ISR-related functions
#define REDUCE_TSREC_LOG_IN_ISR_FUNC
#endif


extern unsigned int tsrec_log_ctrl;

#ifndef FS_UT
extern struct mutex tsrec_log_concurrency_lock;
#endif


/*----------------------------------------------------------------------------*/
/* log ctrl */
enum tsrec_log_ctrl_category {
	/* basic category */
	LOG_TSREC_PF = 0,
	LOG_TSREC,
	LOG_TSREC_REG,

	/* special category */
	LOG_TSREC_WORK_HANDLE,
	LOG_TSREC_CB_INFO,

	/* extra category */
	LOG_TSREC_WITH_RAW_TICK = 26,

	/* max category */
	LOG_TSREC_CTRL_CAT_MAX = 26
};


/* log macro/define */
#define _TSREC_LOG_ENABLED(category)	\
	((tsrec_log_ctrl) & (1UL << (category)))


/*----------------------------------------------------------------------------*/
#ifdef FS_UT
#define TSREC_LOG_DBG(format, args...) \
	printf(PFX "[%s] " format, __func__, ##args)
#define TSREC_LOG_DBG_CAT(log_cat, format, args...) \
	printf(PFX "[%s] " format, __func__, ##args)
#define TSREC_LOG_PF_DBG(format, args...) \
	printf(PFX "[%s] " format, __func__, ##args)
#define TSREC_LOG_INF(format, args...) \
	printf(PFX "[%s] " format, __func__, ##args)

#define TSREC_LOG_DBG_LOCK(format, args...) \
	printf(PFX "[%s] " format, __func__, ##args)
#define TSREC_LOG_DBG_CAT_LOCK(log_cat, format, args...) \
	printf(PFX "[%s] " format, __func__, ##args)
#define TSREC_LOG_PF_DBG_LOCK(format, args...) \
	printf(PFX "[%s] " format, __func__, ##args)
#define TSREC_LOG_INF_LOCK(format, args...) \
	printf(PFX "[%s] " format, __func__, ##args)

#else

#define DY_INFO(log_cat, format, args...) \
do { \
	if (unlikely(_TSREC_LOG_ENABLED(log_cat))) { \
		pr_info(PFX "[%s] " format, __func__, ##args); \
	} \
} while (0)

#define DY_INFO_LOCK(log_cat, format, args...) \
do { \
	if (unlikely(_TSREC_LOG_ENABLED(log_cat))) { \
		mutex_lock(&tsrec_log_concurrency_lock); \
		pr_info(PFX "[%s] " format, __func__, ##args); \
		mutex_unlock(&tsrec_log_concurrency_lock); \
	} \
} while (0)

#define TSREC_LOG_DBG(format, args...) \
	DY_INFO(TSREC_LOG_DBG_DEF_CAT, format, args)
#define TSREC_LOG_DBG_CAT(log_cat, format, args...) \
	DY_INFO(log_cat, format, args)
#define TSREC_LOG_PF_DBG(format, args...) \
	DY_INFO(LOG_TSREC_PF, format, args)
#define TSREC_LOG_INF(format, args...) \
	pr_info(PFX "[%s] " format, __func__, ##args)

#define TSREC_LOG_DBG_LOCK(format, args...) \
	DY_INFO_LOCK(log_cat, format, args)
#define TSREC_LOG_DBG_CAT_LOCK(log_cat, format, args...) \
	DY_INFO_LOCK(log_cat, format, args)
#define TSREC_LOG_PF_DBG_LOCK(format, args...) \
	DY_INFO_LOCK(LOG_TSREC_PF, format, args)
#define TSREC_LOG_INF_LOCK(format, args...) \
do { \
	mutex_lock(&tsrec_log_concurrency_lock); \
	pr_info(PFX "[%s] " format, __func__, ##args); \
	mutex_unlock(&tsrec_log_concurrency_lock); \
} while (0)

#endif


#define TSREC_SNPRF(buf_len, buf, len, fmt, ...) \
do{ \
	int ret; \
	ret = snprintf((buf + len), (buf_len - len), fmt, ##__VA_ARGS__); \
	if (unlikely((ret < 0) || (ret >= (buf_len - len)))) { \
		TSREC_LOG_PF_DBG( \
			"WARNING: snprintf ret:%d, space:%u(truncated), set len to buf_len:%u\n", \
			ret, (buf_len - len), buf_len); \
		len = buf_len; \
	} else \
		len += ret; \
} while (0)


/******************************************************************************
 * TSREC atomic define/macro
 *****************************************************************************/
#ifdef FS_UT
#define tsrec_atomic_t atomic_int
#define TSREC_ATOMIC_INIT(n, p)      (atomic_init((p), (n)))
#define TSREC_ATOMIC_SET(n, p)       (atomic_store((p), (n)))
#define TSREC_ATOMIC_READ(p)         (atomic_load(p))
#define TSREC_ATOMIC_FETCH_OR(n, p)  (atomic_fetch_or((p), (n)))
#define TSREC_ATOMIC_FETCH_AND(n, p) (atomic_fetch_and((p), (n)))
#define TSREC_ATOMIC_XCHG(n, p)      (atomic_exchange((p), (n)))
#define TSREC_ATOMIC_INC(p)          (atomic_fetch_add((p), 1))
#define TSREC_ATOMIC_DEC(p)          (atomic_fetch_sub((p), 1))
#else
#define tsrec_atomic_t atomic_t
#define TSREC_ATOMIC_INIT(n, p)      (atomic_set((p), (n)))
#define TSREC_ATOMIC_SET(n, p)       (atomic_set((p), (n)))
#define TSREC_ATOMIC_READ(p)         (atomic_read(p))
#define TSREC_ATOMIC_FETCH_OR(n, p)  (atomic_fetch_or((n), (p)))
#define TSREC_ATOMIC_FETCH_AND(n, p) (atomic_fetch_and((n), (p)))
#define TSREC_ATOMIC_XCHG(n, p)      (atomic_xchg((p), (n)))
#define TSREC_ATOMIC_INC(p)          (atomic_inc(p))
#define TSREC_ATOMIC_DEC(p)          (atomic_dec(p))
#endif // FS_UT


/******************************************************************************
 * TSREC macro for clean code
 *****************************************************************************/
#ifdef FS_UT
#define likely(x)                    (__builtin_expect((x), 1))
#define unlikely(x)                  (__builtin_expect((x), 0))

#define TSREC_PM_GET_SYNC(ctx)       (0)
#define TSREC_PM_PUT_SYNC(ctx)       (0)
#define TSREC_PM_PUT_NOIDLE(ctx)

#define BUILD_BUG_ON_ZERO(e)         ((int)(sizeof(struct { int:(-!!(e)); })))
#define __same_type(a, b) \
	(__builtin_types_compatible_p(typeof(a), typeof(b)))
#define __must_be_array(a) \
	(BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0])))
#define ARRAY_SIZE(arr) \
	(sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#define TSREC_POPCNT_32(n)           (__builtin_popcount(n))
#define TSREC_POPCNT_64(n)           (__builtin_popcount(n))

#define TSREC_KZALLOC(size)          (calloc(1, (size)))
#define TSREC_KCALLOC(n, size)       (calloc((n), (size)))
#define TSREC_KFREE(p)               (free((p)))

#define TSREC_SPIN_LOCK(p)
#define TSREC_SPIN_UNLOCK(p)
#else
#define TSREC_PM_GET_SYNC(ctx)       (pm_runtime_get_sync((ctx)->dev))
#define TSREC_PM_PUT_SYNC(ctx)       (pm_runtime_put_sync((ctx)->dev))
#define TSREC_PM_PUT_NOIDLE(ctx)     (pm_runtime_put_noidle((ctx)->dev))

#define TSREC_POPCNT_32(n)           (hweight32(n))
#define TSREC_POPCNT_64(n)           (hweight64(n))

extern struct device *seninf_dev;
#define TSREC_KZALLOC(size)          (devm_kzalloc(seninf_dev, (size), GFP_ATOMIC))
#define TSREC_KCALLOC(n, size)       (devm_kcalloc(seninf_dev, (n), (size), GFP_ATOMIC))
#define TSREC_KFREE(p)               (devm_kfree(seninf_dev, (p)))

#define TSREC_SPIN_LOCK(p)           (spin_lock(p))
#define TSREC_SPIN_UNLOCK(p)         (spin_unlock(p))
#endif // FS_UT


/*******************************************************************************
 * TSREC common utilities inline functions
 ******************************************************************************/
static inline int alloc_log_buf(const unsigned int alloc_len, char **p_p_buf)
{
	*p_p_buf = TSREC_KCALLOC((alloc_len + 1), sizeof(char));
	if (unlikely(*p_p_buf == NULL))
		return -1;

	*p_p_buf[0] = '\0';

	return 0;
}


#endif
