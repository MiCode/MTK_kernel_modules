/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#ifndef _GPS_DL_OSAL_H
#define _GPS_DL_OSAL_H
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/current.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/kfifo.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/log2.h>
#include <linux/atomic.h>
#include <linux/ratelimit.h>

#define DBG_LOG_STR_SIZE    256

#define RB_LATEST(prb) ((prb)->write - 1)
#define RB_SIZE(prb) ((prb)->size)
#define RB_MASK(prb) (RB_SIZE(prb) - 1)
#define RB_COUNT(prb) ((prb)->write - (prb)->read)
#define RB_FULL(prb) (RB_COUNT(prb) >= RB_SIZE(prb))
#define RB_EMPTY(prb) ((prb)->write == (prb)->read)

#define RB_INIT(prb, qsize) \
do { \
	(prb)->read = (prb)->write = 0; \
	(prb)->size = (qsize); \
} while (0)

#define RB_PUT(prb, value) \
do { \
	if (!RB_FULL(prb)) { \
		(prb)->queue[(prb)->write & RB_MASK(prb)] = value; \
		++((prb)->write); \
	} \
	else { \
		osal_assert(!RB_FULL(prb)); \
	} \
} while (0)

#define RB_GET(prb, value) \
do { \
	if (!RB_EMPTY(prb)) { \
		value = (prb)->queue[(prb)->read & RB_MASK(prb)]; \
		++((prb)->read); \
		if (RB_EMPTY(prb)) { \
			(prb)->read = (prb)->write = 0; \
		} \
	} \
	else { \
		value = NULL; \
		osal_assert(!RB_EMPTY(prb)); \
	} \
} while (0)

#define RB_GET_LATEST(prb, value) \
do { \
	if (!RB_EMPTY(prb)) { \
		value = (prb)->queue[RB_LATEST(prb) & RB_MASK(prb)]; \
		if (RB_EMPTY(prb)) { \
			(prb)->read = (prb)->write = 0; \
		} \
	} \
	else { \
		value = NULL; \
	} \
} while (0)

#define MAX_THREAD_NAME_LEN 128
#define OSAL_OP_DATA_SIZE   32
#define OSAL_OP_BUF_SIZE    16

typedef void(*P_TIMEOUT_HANDLER) (unsigned long);
typedef int(*P_COND) (void *);

struct gps_dl_osal_timer {
	struct timer_list timer;
	P_TIMEOUT_HANDLER timeoutHandler;
	unsigned long timeroutHandlerData;
};

struct gps_dl_osal_unsleepable_lock {
	spinlock_t lock;
	unsigned long flag;
};

struct gps_dl_osal_sleepable_lock {
	struct mutex lock;
};

struct gps_dl_osal_signal {
	struct completion comp;
	unsigned int timeoutValue;
	unsigned int timeoutExtension;	/* max number of timeout caused by thread not able to acquire CPU */
};

struct gps_dl_osal_event {
	wait_queue_head_t waitQueue;
	unsigned int timeoutValue;
	int waitFlag;
};

struct gps_dl_osal_op_dat {
	unsigned int opId;	/* Event ID */
	unsigned int u4InfoBit;	/* Reserved */
	unsigned long au4OpData[OSAL_OP_DATA_SIZE];	/* OP Data */
};

struct gps_dl_osal_lxop {
	struct gps_dl_osal_op_dat op;
	struct gps_dl_osal_signal signal;
	int result;
	atomic_t ref_count;
};

struct gps_dl_osal_lxop_q {
	struct gps_dl_osal_sleepable_lock sLock;
	unsigned int write;
	unsigned int read;
	unsigned int size;
	struct gps_dl_osal_lxop *queue[OSAL_OP_BUF_SIZE];
};

struct gps_dl_osal_thread {
	struct task_struct *pThread;
	void *pThreadFunc;
	void *pThreadData;
	char threadName[MAX_THREAD_NAME_LEN];
};

typedef unsigned int(*OSAL_EVENT_CHECKER) (struct gps_dl_osal_thread *pThread);

unsigned int osal_strlen(const char *str);
int osal_strcmp(const char *dst, const char *src);
int osal_strncmp(const char *dst, const char *src, unsigned int len);
char *osal_strcpy(char *dst, const char *src);
char *osal_strncpy(char *dst, const char *src, unsigned int len);
char *osal_strcat(char *dst, const char *src);
char *osal_strncat(char *dst, const char *src, unsigned int len);
char *osal_strchr(const char *str, unsigned char c);
char *osal_strsep(char **str, const char *c);
int osal_strtol(const char *str, unsigned int adecimal, long *res);
int osal_snprintf(char *buf, unsigned int len, const char *fmt, ...);
int osal_err_print(const char *str, ...);
int osal_dbg_print(const char *str, ...);
int osal_warn_print(const char *str, ...);
void *osal_malloc(unsigned int size);
void osal_free(const void *dst);
void *osal_memset(void *buf, int i, unsigned int len);
void *osal_memcpy(void *dst, const void *src, unsigned int len);
void osal_memcpy_fromio(void *dst, const void *src, unsigned int len);
void osal_memcpy_toio(void *dst, const void *src, unsigned int len);
int osal_memcmp(const void *buf1, const void *buf2, unsigned int len);
int osal_thread_create(struct gps_dl_osal_thread *pThread);
int osal_thread_run(struct gps_dl_osal_thread *pThread);
int osal_thread_stop(struct gps_dl_osal_thread *pThread);
int osal_thread_should_stop(struct gps_dl_osal_thread *pThread);
int osal_thread_destroy(struct gps_dl_osal_thread *pThread);
int osal_thread_wait_for_event(struct gps_dl_osal_thread *pThread,
	struct gps_dl_osal_event *pEvent, OSAL_EVENT_CHECKER pChecker);
int osal_signal_init(struct gps_dl_osal_signal *pSignal);
int osal_wait_for_signal(struct gps_dl_osal_signal *pSignal);
int osal_wait_for_signal_timeout(struct gps_dl_osal_signal *pSignal, struct gps_dl_osal_thread *pThread);
int osal_raise_signal(struct gps_dl_osal_signal *pSignal);
int osal_signal_active_state(struct gps_dl_osal_signal *pSignal);
int osal_op_is_wait_for_signal(struct gps_dl_osal_lxop *pOp);
void osal_op_raise_signal(struct gps_dl_osal_lxop *pOp, int result);
int osal_signal_deinit(struct gps_dl_osal_signal *pSignal);
int osal_event_init(struct gps_dl_osal_event *pEvent);
int osal_trigger_event(struct gps_dl_osal_event *pEvent);
int osal_wait_for_event(struct gps_dl_osal_event *pEvent, int (*condition)(void *), void *cond_pa);
int osal_wait_for_event_timeout(struct gps_dl_osal_event *pEvent, int (*condition)(void *), void *cond_pa);
int osal_event_deinit(struct gps_dl_osal_event *pEvent);
int osal_sleepable_lock_init(struct gps_dl_osal_sleepable_lock *pSL);
int osal_lock_sleepable_lock(struct gps_dl_osal_sleepable_lock *pSL);
int osal_unlock_sleepable_lock(struct gps_dl_osal_sleepable_lock *pSL);
int osal_trylock_sleepable_lock(struct gps_dl_osal_sleepable_lock *pSL);
int osal_sleepable_lock_deinit(struct gps_dl_osal_sleepable_lock *pSL);
int osal_unsleepable_lock_init(struct gps_dl_osal_unsleepable_lock *pUSL);
int osal_lock_unsleepable_lock(struct gps_dl_osal_unsleepable_lock *pUSL);
int osal_unlock_unsleepable_lock(struct gps_dl_osal_unsleepable_lock *pUSL);

#define osal_assert(condition) \
do { \
	if (!(condition)) \
		osal_err_print("%s, %d, (%s)\n", __FILE__, __LINE__, #condition); \
} while (0)

#endif /* _GPS_DL_OSAL_H */

