/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _FRAME_SYNC_DEF_H
#define _FRAME_SYNC_DEF_H

#ifdef FS_UT
#include <string.h>
#include <stdlib.h>         /* Needed by memory allocate */

#include "fs-ut-test/ut_fs_tsrec.h"
#else
/* INSTEAD of using stdio.h, you have to use the following include */
#include <linux/slab.h>     /* Needed by memory allocate */
#include <linux/string.h>

#include "imgsensor-user.h"
#endif // FS_UT


/******************************************************************************/
// global define / variable / macro
/******************************************************************************/
#define SENSOR_MAX_NUM 6
#define CAMMUX_ID_INVALID 256

#define TS_DIFF_TABLE_LEN (((SENSOR_MAX_NUM)*(SENSOR_MAX_NUM-1))/2)

#define FS_TOLERANCE 1000

// #define FS_FL_AUTO_RESTORE_DISABLE
#define FS_FL_AUTO_RESTORE_TH 1000

#define ALGO_AUTO_LISTEN_VSYNC 0


/******************************************************************************/
// timestamp source HW / timestamp data type
/******************************************************************************/
/*
 * (choose ONE) timestamp by using bellow method.
 * e.g. CCU / N3D / TSREC (default) / etc.
 */
enum fs_timestamp_src_type {
	FS_TS_SRC_UNKNOWN = 0,
	FS_TS_SRC_CCU,
	FS_TS_SRC_TSREC,
};

#define SUPPORT_USING_CCU
#ifdef SUPPORT_USING_CCU
/*
 * delay power ON/OFF and operate CCU to fs_set_sync()
 *
 * P.S: due to sensor streaming on before seninf/tg setup,
 *      this is the way to get correct tg information
 *      after seninf being config compeleted.
 */
#define DELAY_CCU_OP
#endif

#define USING_TSREC
#ifdef USING_TSREC
/*
 * ISP7s  : 32-bits;
 * ISP7s+ : if use global timer => 64-bits timestamp
 *          if use local timer => 32-bits timestamp
 */
// #define TS_TICK_64_BITS // for using global timer => 64-bits timestamp
#endif
#define TSREC_1ST_EXP_ID 0


/*
 * timestamp data type define macro
 */
#if defined(TS_TICK_64_BITS)
#define fs_timestamp_t unsigned long long
#else
#define fs_timestamp_t unsigned int
#endif
/******************************************************************************/


#define SUPPORT_FS_NEW_METHOD
#ifdef SUPPORT_FS_NEW_METHOD
#define MASTER_IDX_NONE 255

#define SUPPORT_AUTO_EN_SA_MODE

#define FORCE_USING_SA_MODE

/*
 * force adjust smaller diff one for MW-frame no. matching
 */
#define FORCE_ADJUST_SMALLER_DIFF

#endif // SUPPORT_FS_NEW_METHOD

#ifdef FS_UT
#include <stdatomic.h>
#define FS_Atomic_T atomic_int
#define FS_ATOMIC_INIT(n, p)      (atomic_init((p), (n)))
#define FS_ATOMIC_SET(n, p)       (atomic_store((p), (n)))
#define FS_ATOMIC_READ(p)         (atomic_load(p))
#define FS_ATOMIC_FETCH_OR(n, p)  (atomic_fetch_or((p), (n)))
#define FS_ATOMIC_FETCH_AND(n, p) (atomic_fetch_and((p), (n)))
#define FS_ATOMIC_XCHG(n, p)      (atomic_exchange((p), (n)))
#define FS_ATOMIC_CMPXCHG(n, m, p) (atomic_compare_exchange_weak((p), &(n), (m)))
#else
#include <linux/atomic.h>
#define FS_Atomic_T atomic_t
#define FS_ATOMIC_INIT(n, p)      (atomic_set((p), (n)))
#define FS_ATOMIC_SET(n, p)       (atomic_set((p), (n)))
#define FS_ATOMIC_READ(p)         (atomic_read(p))
#define FS_ATOMIC_FETCH_OR(n, p)  (atomic_fetch_or((n), (p)))
#define FS_ATOMIC_FETCH_AND(n, p) (atomic_fetch_and((n), (p)))
#define FS_ATOMIC_XCHG(n, p)      (atomic_xchg((p), (n)))
#define FS_ATOMIC_CMPXCHG(n, m, p) (atomic_cmpxchg((p), (n), (m)) == (n))
#endif // FS_UT


/*
 * macro for clear code
 */
#ifdef FS_UT
#define likely(x)          (__builtin_expect((x), 1))
#define unlikely(x)        (__builtin_expect((x), 0))
#define FS_POPCOUNT(n)     (__builtin_popcount(n))
#define fs_spin_init(p)
#define fs_spin_lock(p)
#define fs_spin_unlock(p)
#define fs_mutex_lock(p)
#define fs_mutex_unlock(p)
#define FS_CALLOC(LEN, T)  (calloc((LEN), (T)))
#define FS_DEV_ZALLOC(dev, T)    (calloc((1), (T)))
#define FS_DEV_CALLOC(dev, n, T) (calloc((n), (T)))
#define FS_FREE(buf)       (free(buf))

#define __same_type(a, b)  (__builtin_types_compatible_p(typeof(a), typeof(b)))
#define __must_be_array(a) (BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0])))
#define ARRAY_SIZE(arr)    (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#define do_div(n, base)    ({\
	unsigned int __base = (base);\
	unsigned int __rem;\
	__rem = ((unsigned long long)(n)) % __base;\
	(n) = ((unsigned long long)(n)) / __base;\
	__rem;\
})

#else

#define FS_POPCOUNT(n)     (hweight32(n))
#define fs_spin_init(p)    (spin_lock_init(p))
#define fs_spin_lock(p)    (spin_lock(p))
#define fs_spin_unlock(p)  (spin_unlock(p))
#define fs_mutex_lock(p)   (mutex_lock(p))
#define fs_mutex_unlock(p) (mutex_unlock(p))
#define FS_CALLOC(LEN, T)  (kcalloc((LEN), (T), (GFP_ATOMIC)))
#define FS_DEV_ZALLOC(dev, T)    (devm_kzalloc((dev), (T), GFP_ATOMIC))
#define FS_DEV_CALLOC(dev, n, T) (devm_kcalloc((dev), (n), (T), GFP_ATOMIC))
#define FS_FREE(buf)       (kfree(buf))
#endif // FS_UT


#define FS_CHECK_BIT(n, p)    (check_bit_atomic((n), (p)))
#define FS_WRITE_BIT(n, i, p) (write_bit_atomic((n), (i), (p)))
#define FS_READ_BITS(p)       (FS_ATOMIC_READ((p)))


#define FS_RING_BACK(x, base, n)    (((x) + ((base)-((n)%(base)))) & ((base)-1))
#define FS_RING_FORWARD(x, base, n) (((x) + ((n)%(base))) & ((base)-1))


/*  Implement ceiling, floor functions.  */
#define FS_CEIL(n, d)   (((n) < 0) ? (-((-(n))/(d))) : (n)/(d) + ((n)%(d) != 0))
#define FS_FLOOR(n, d)  (((n) < 0) ? (-((-(n))/(d))) - ((n)%(d) != 0) : (n)/(d))


/* using v4l2_ctrl_request_setup */
#define USING_V4L2_CTRL_REQUEST_SETUP


/*
 * for test using, sync with diff => un-sync
 */
// #define SYNC_WITH_CUSTOM_DIFF
#if defined(SYNC_WITH_CUSTOM_DIFF)
#define CUSTOM_DIFF_SENSOR_IDX 255
#define CUSTOM_DIFF_US 0
#endif // SYNC_WITH_CUSTOM_DIFF


#if !defined(FS_UT)
/*
 * frame_sync_console
 */
#include <linux/device.h>  /* for device structure */
#endif // FS_UT
/******************************************************************************/

#endif
