/*
 * Copyright (C) 2016 MediaTek Inc.
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

/*! \file
 * \brief  Declaration of library functions
 * Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
 */

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/
#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__
/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/sched/clock.h> /* local_clock */
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/trace_events.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/current.h>
#include <linux/kfifo.h>
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
#include "connectivity_build_in_adapter.h"
#endif
#include "osal.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

#define GPIO_ASSERT		70
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*string operations*/
unsigned int osal_strlen(const char *str)
{
	return strlen(str);
}

int osal_strcmp(const char *dst, const char *src)
{
	return strcmp(dst, src);
}

int osal_strncmp(const char *dst, const char *src, unsigned int len)
{
	return strncmp(dst, src, len);
}

char *osal_strcpy(char *dst, const char *src)
{
	return strncpy(dst, src, strlen(src)+1);
}

char *osal_strncpy(char *dst, const char *src, unsigned int len)
{
	return strncpy(dst, src, len);
}

char *osal_strcat(char *dst, const char *src)
{
	return strncat(dst, src, strlen(src));
}

char *osal_strncat(char *dst, const char *src, unsigned int len)
{
	return strncat(dst, src, len);
}

char *osal_strchr(const char *str, unsigned char c)
{
	return strchr(str, c);
}

char *osal_strsep(char **str, const char *c)
{
	return strsep(str, c);
}

int osal_strtol(const char *str, unsigned int adecimal, long *res)
{
	if (sizeof(long) == 4)
		return kstrtou32(str, adecimal, (unsigned int *) res);
	else
		return kstrtol(str, adecimal, res);
}

char *osal_strstr(char *str1, const char *str2)
{
	return strstr(str1, str2);
}

char *osal_strnstr(char *str1, const char *str2, int n)
{
	return strnstr(str1, str2, n);
}

void osal_bug_on(unsigned int val)
{
	WARN_ON(val);
}

int osal_snprintf(char *buf, unsigned int len, const char *fmt, ...)
{
	int iRet = 0;
	va_list args;

	/*va_start(args, fmt); */
	va_start(args, fmt);
	/*iRet = snprintf(buf, len, fmt, args); */
	iRet = vsnprintf(buf, len, fmt, args);
	va_end(args);

	return iRet;
}

int osal_sprintf(char *str, const char *format, ...)
{
	int iRet = 0;
	va_list args;

	va_start(args, format);
	iRet = vsnprintf(str, DBG_LOG_STR_SIZE, format, args);
	va_end(args);

	return iRet;
}

void *osal_malloc(unsigned int size)
{
	return vmalloc(size);
}

void osal_free(const void *dst)
{
	vfree(dst);
}

void *osal_memset(void *buf, int i, unsigned int len)
{
	return memset(buf, i, len);
}

void *osal_memcpy(void *dst, const void *src, unsigned int len)
{
	return memcpy(dst, src, len);
}

void osal_memcpy_fromio(void *dst, const void *src, unsigned int len)
{
	return memcpy_fromio(dst, src, len);
}

void osal_memcpy_toio(void *dst, const void *src, unsigned int len)
{
	return memcpy_toio(dst, src, len);
}

int osal_memcmp(const void *buf1, const void *buf2, unsigned int len)
{
	return memcmp(buf1, buf2, len);
}

void osal_dump_thread_state(const unsigned char *name)
{
	//return connectivity_export_dump_thread_state(name);
}

static inline bool __osal_is_valid_thread(P_OSAL_THREAD pThread)
{
	if ((pThread) && !IS_ERR_OR_NULL(pThread->pThread))
		return true;
	else
		return false;
}

void osal_thread_show_stack(P_OSAL_THREAD pThread)
{
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
	if (__osal_is_valid_thread(pThread))
		KERNEL_show_stack(pThread->pThread, NULL);
#endif
}

/*
 * OSAL layer Thread Opeartion related APIs
 *
 */
int osal_thread_create(P_OSAL_THREAD pThread)
{
	struct task_struct *task;

	if (!pThread)
		return -1;

	task = kthread_create(pThread->pThreadFunc,
				pThread->pThreadData, pThread->threadName);
	if (IS_ERR(task)) {
		pr_err("[%s] create %s thread fail", __func__, pThread->threadName);
		return -1;
	}

	pThread->pThread = task;
	return 0;
}

int osal_thread_run(P_OSAL_THREAD pThread)
{
	if (__osal_is_valid_thread(pThread)) {
		wake_up_process(pThread->pThread);
		return 0;
	} else {
		return -1;
	}
}

int osal_thread_stop(P_OSAL_THREAD pThread)
{
	int iRet;

	if (__osal_is_valid_thread(pThread)) {
		iRet = kthread_stop(pThread->pThread);
		pThread->pThread = NULL;
		return iRet;
	}
	return -1;
}

int osal_thread_should_stop(P_OSAL_THREAD pThread)
{
	if (__osal_is_valid_thread(pThread))
		return kthread_should_stop();
	else
		return 1;

}

int osal_thread_wait_for_event(P_OSAL_THREAD pThread,
		P_OSAL_EVENT pEvent, P_OSAL_EVENT_CHECKER pChecker)
{
	/*  P_DEV_WMT pDevWmt;*/

	if (__osal_is_valid_thread(pThread) && (pEvent) && (pChecker)) {
		return wait_event_interruptible(pEvent->waitQueue, (
					   osal_thread_should_stop(pThread)
					   || (*pChecker) (pThread)));
	}
	return -1;
}

int osal_thread_destroy(P_OSAL_THREAD pThread)
{
	if (__osal_is_valid_thread(pThread)) {
		kthread_stop(pThread->pThread);
		pThread->pThread = NULL;
	}
	return 0;
}

/*
 * osal_thread_sched_retrieve
 * Retrieve thread's current scheduling statistics and stored in output "sched".
 * Return value:
 *	 0 : Schedstats successfully retrieved
 *	-1 : Kernel's schedstats feature not enabled
 *	-2 : pThread not yet initialized or sched is a NULL pointer
 */
static int osal_thread_sched_retrieve(P_OSAL_THREAD pThread,
				P_OSAL_THREAD_SCHEDSTATS sched)
{
#ifdef CONFIG_SCHEDSTATS
	struct sched_entity se;
	unsigned long long sec;
	unsigned long usec;

	if (!sched)
		return -2;

	/* always clear sched to simplify error handling at caller side */
	memset(sched, 0, sizeof(OSAL_THREAD_SCHEDSTATS));

	if (!__osal_is_valid_thread(pThread))
		return -2;

	memcpy(&se, &pThread->pThread->se, sizeof(struct sched_entity));
	osal_get_local_time(&sec, &usec);

	sched->time = sec*1000 + usec/1000;
	sched->exec = se.sum_exec_runtime;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0))
	sched->runnable = pThread->pThread->stats.wait_sum;
	sched->iowait = pThread->pThread->stats.iowait_sum;
#else
	sched->runnable = se.statistics.wait_sum;
	sched->iowait = se.statistics.iowait_sum;
#endif

	return 0;
#else
	/* always clear sched to simplify error handling at caller side */
	if (sched)
		memset(sched, 0, sizeof(OSAL_THREAD_SCHEDSTATS));
	return -1;
#endif
}

/*
 * osal_thread_sched_mark
 * Record the thread's current schedstats and stored
 * in output "schedstats" parameter for profiling at
 * later time.
 * Return value:
 *	 0 : Schedstats successfully recorded
 *	-1 : Kernel's schedstats feature not enabled
 *	-2 : pThread not yet initialized or invalid parameters
 */
int osal_thread_sched_mark(P_OSAL_THREAD pThread,
			P_OSAL_THREAD_SCHEDSTATS schedstats)
{
	return osal_thread_sched_retrieve(pThread, schedstats);
}

/*
 * osal_thread_sched_unmark
 * Calculate scheduling statistics against the previously marked point.
 * The result will be filled back into the schedstats output parameter.
 * Return value:
 *	 0 : Schedstats successfully calculated
 *	-1 : Kernel's schedstats feature not enabled
 *	-2 : pThread not yet initialized or invalid parameters
 */
int osal_thread_sched_unmark(P_OSAL_THREAD pThread,
			P_OSAL_THREAD_SCHEDSTATS schedstats)
{
	int ret;
	OSAL_THREAD_SCHEDSTATS sched_now;

	if (unlikely(!schedstats)) {
		ret = -2;
	} else {
		ret = osal_thread_sched_retrieve(pThread, &sched_now);
		if (ret == 0) {
			schedstats->time = sched_now.time - schedstats->time;
			schedstats->exec = sched_now.exec - schedstats->exec;
			schedstats->runnable =
				sched_now.runnable - schedstats->runnable;
			schedstats->iowait =
				sched_now.iowait - schedstats->iowait;
		}
	}
	return ret;
}

/*
 * OSAL layer Signal Opeartion related APIs
 * initialization
 * wait for signal
 * wait for signal timerout
 * raise signal
 * destroy a signal
 *
 */

int osal_signal_init(P_OSAL_SIGNAL pSignal)
{
	if (pSignal) {
		init_completion(&pSignal->comp);
		return 0;
	} else {
		return -1;
	}
}

int osal_wait_for_signal(P_OSAL_SIGNAL pSignal)
{
	if (pSignal) {
		wait_for_completion_interruptible(&pSignal->comp);
		return 0;
	} else {
		return -1;
	}
}

/*
 * osal_wait_for_signal_timeout
 *
 * Wait for a signal to be triggered by the corresponding thread, within the
 * expected timeout specified by the signal's timeoutValue.
 * When the pThread parameter is specified, the thread's scheduling ability is
 * considered, the timeout will be extended when thread cannot acquire CPU
 * resource, and will only extend for a number of times specified by the
 * signal's timeoutExtension should the situation continues.
 *
 * Return value:
 *	 0 : timeout
 *	>0 : signal triggered
 */
int osal_wait_for_signal_timeout(P_OSAL_SIGNAL pSignal, P_OSAL_THREAD pThread)
{
	OSAL_THREAD_SCHEDSTATS schedstats;
	int waitRet;

	/* [ChangeFeature][George] gps driver may be closed by -ERESTARTSYS.
	 * Avoid using *interruptible" version in order to complete our jobs,
	 * such as function off gracefully.
	 */
	if (!pThread || !pThread->pThread)
		return wait_for_completion_timeout(&pSignal->comp,
				msecs_to_jiffies(pSignal->timeoutValue));

	do {
		osal_thread_sched_mark(pThread, &schedstats);
		waitRet = wait_for_completion_timeout(&pSignal->comp,
				msecs_to_jiffies(pSignal->timeoutValue));
		osal_thread_sched_unmark(pThread, &schedstats);

		if (waitRet > 0)
			break;

		if (schedstats.runnable > schedstats.exec) {
			pr_err(
				"[E]%s:wait completion timeout, %s cannot get CPU, extension(%d), show backtrace:\n",
				__func__,
				pThread->threadName,
				pSignal->timeoutExtension);
		} else {
			pr_err(
				"[E]%s:wait completion timeout, show %s backtrace:\n",
				__func__,
				pThread->threadName);
			pSignal->timeoutExtension = 0;
		}
		pr_err(
			"[E]%s:\tduration:%llums, sched(x%llu/r%llu/i%llu)\n",
			__func__,
			schedstats.time,
			schedstats.exec,
			schedstats.runnable,
			schedstats.iowait);
		/*
		 * no need to disginguish combo or A/D die projects
		 * osal_dump_thread_state will just return if target
		 * thread does not exist
		 */
		osal_dump_thread_state("mtk_wmtd");
		osal_dump_thread_state("mtk_wmtd_worker");
		osal_dump_thread_state("btif_rxd");
		osal_dump_thread_state("mtk_stp_psm");
		osal_dump_thread_state("mtk_stp_btm");
		osal_dump_thread_state("stp_sdio_tx_rx");
	} while (pSignal->timeoutExtension--);
	return waitRet;
}

int osal_raise_signal(P_OSAL_SIGNAL pSignal)
{
	if (pSignal) {
		complete(&pSignal->comp);
		return 0;
	} else
		return -1;
}

int osal_signal_active_state(P_OSAL_SIGNAL pSignal)
{
	if (pSignal)
		return pSignal->timeoutValue;
	else
		return -1;
}

int osal_signal_deinit(P_OSAL_SIGNAL pSignal)
{
	if (pSignal) {
		pSignal->timeoutValue = 0;
		return 0;
	} else
		return -1;
}

/*
 * OSAL layer Event Opeartion related APIs
 * initialization
 * wait for signal
 * wait for signal timerout
 * raise signal
 * destroy a signal
 *
 */

int osal_event_init(P_OSAL_EVENT pEvent)
{
	if (pEvent) {
		init_waitqueue_head(&pEvent->waitQueue);
		return 0;
	}
	return -1;
}

int osal_trigger_event(P_OSAL_EVENT pEvent)
{
	int ret = 0;

	if (pEvent) {
		wake_up_interruptible(&pEvent->waitQueue);
		return ret;
	}
	return -1;
}

int osal_wait_for_event(P_OSAL_EVENT pEvent,
			int (*condition)(void *), void *cond_pa)
{
	if (pEvent)
		return wait_event_interruptible(pEvent->waitQueue,
							condition(cond_pa));
	else
		return -1;
}

int osal_wait_for_event_timeout(P_OSAL_EVENT pEvent,
				int (*condition)(void *), void *cond_pa)
{
	if (pEvent)
		return wait_event_interruptible_timeout(pEvent->waitQueue,
					condition(cond_pa),
					msecs_to_jiffies(pEvent->timeoutValue));
	return -1;
}


int osal_event_deinit(P_OSAL_EVENT pEvent)
{
	return 0;
}

long osal_wait_for_event_bit_set(P_OSAL_EVENT pEvent,
			unsigned long *pState, unsigned int bitOffset)
{
	unsigned int ms = 0;

	if (pEvent) {
		ms = pEvent->timeoutValue;
		if (ms != 0)
			return wait_event_interruptible_timeout(
						pEvent->waitQueue,
						test_bit(bitOffset, pState),
						msecs_to_jiffies(ms));
		else
			return wait_event_interruptible(pEvent->waitQueue,
						test_bit(bitOffset, pState));
	} else
		return -1;

}

long osal_wait_for_event_bit_clr(P_OSAL_EVENT pEvent,
		unsigned long *pState, unsigned int bitOffset)
{
	unsigned int ms = 0;

	if (pEvent) {
		ms = pEvent->timeoutValue;
		if (ms != 0)
			return wait_event_interruptible_timeout(
						pEvent->waitQueue,
						!test_bit(bitOffset, pState),
						msecs_to_jiffies(ms));
		else
			return wait_event_interruptible(pEvent->waitQueue,
						!test_bit(bitOffset, pState));
	} else
		return -1;
}

/*
 * bit test and set/clear operations APIs
 */
#if    OS_BIT_OPS_SUPPORT
#define osal_bit_op_lock(x)
#define osal_bit_op_unlock(x)
#else

int osal_bit_op_lock(P_OSAL_UNSLEEPABLE_LOCK pLock)
{

	return 0;
}

int osal_bit_op_unlock(P_OSAL_UNSLEEPABLE_LOCK pLock)
{

	return 0;
}
#endif
int osal_clear_bit(unsigned int bitOffset, P_OSAL_BIT_OP_VAR pData)
{
	osal_bit_op_lock(&(pData->opLock));
	clear_bit(bitOffset, &pData->data);
	osal_bit_op_unlock(&(pData->opLock));
	return 0;
}

int osal_set_bit(unsigned int bitOffset, P_OSAL_BIT_OP_VAR pData)
{
	osal_bit_op_lock(&(pData->opLock));
	set_bit(bitOffset, &pData->data);
	osal_bit_op_unlock(&(pData->opLock));
	return 0;
}

int osal_test_bit(unsigned int bitOffset, P_OSAL_BIT_OP_VAR pData)
{
	unsigned int iRet = 0;

	osal_bit_op_lock(&(pData->opLock));
	iRet = test_bit(bitOffset, &pData->data);
	osal_bit_op_unlock(&(pData->opLock));
	return iRet;
}

int osal_test_and_clear_bit(unsigned int bitOffset, P_OSAL_BIT_OP_VAR pData)
{
	unsigned int iRet = 0;

	osal_bit_op_lock(&(pData->opLock));
	iRet = test_and_clear_bit(bitOffset, &pData->data);
	osal_bit_op_unlock(&(pData->opLock));
	return iRet;

}

int osal_test_and_set_bit(unsigned int bitOffset, P_OSAL_BIT_OP_VAR pData)
{
	unsigned int iRet = 0;

	osal_bit_op_lock(&(pData->opLock));
	iRet = test_and_set_bit(bitOffset, &pData->data);
	osal_bit_op_unlock(&(pData->opLock));
	return iRet;
}

/*
 * tiemr operations APIs
 * create
 * stop
 * modify
 * create
 * delete
 *
 */

int osal_timer_create(P_OSAL_TIMER pTimer)
{
	struct timer_list *timer = &pTimer->timer;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	timer_setup(timer, pTimer->timeoutHandler, 0);
#else
	init_timer(timer);
	timer->function = pTimer->timeoutHandler;
	timer->data = (unsigned long)pTimer->timeroutHandlerData;
#endif
	return 0;
}

int osal_timer_start(P_OSAL_TIMER pTimer, unsigned int ms)
{

	struct timer_list *timer = &pTimer->timer;

	timer->expires = jiffies + (ms / (1000 / HZ));
	add_timer(timer);
	return 0;
}

int osal_timer_stop(P_OSAL_TIMER pTimer)
{
	struct timer_list *timer = &pTimer->timer;

	del_timer(timer);
	return 0;
}

int osal_timer_stop_sync(P_OSAL_TIMER pTimer)
{
	struct timer_list *timer = &pTimer->timer;

	del_timer_sync(timer);
	return 0;
}

int osal_timer_modify(P_OSAL_TIMER pTimer, unsigned int ms)
{

	mod_timer(&pTimer->timer, jiffies + (ms) / (1000 / HZ));
	return 0;
}

int _osal_fifo_init(OSAL_FIFO *pFifo, unsigned char *buf, unsigned int size)
{
	struct kfifo *fifo = NULL;
	int ret = -1;

	if (!pFifo) {
		pr_err("pFifo must be !NULL\n");
		return -1;
	}
	if (pFifo->pFifoBody) {
		pr_err("pFifo->pFifoBody must be NULL\n");
		pr_err("pFifo(0x%p), pFifo->pFifoBody(0x%p)\n",
					pFifo, pFifo->pFifoBody);
		return -1;
	}
	fifo = kzalloc(sizeof(struct kfifo), GFP_ATOMIC);
	if (!buf) {
		/*fifo's buffer is not ready, we allocate automatically */
		ret = kfifo_alloc(fifo, size, /*GFP_KERNEL */ GFP_ATOMIC);
	} else {
		if (is_power_of_2(size)) {
			kfifo_init(fifo, buf, size);
			ret = 0;
		} else {
			kfifo_free(fifo);
			fifo = NULL;
			ret = -1;
		}
	}

	pFifo->pFifoBody = fifo;
	return (ret < 0) ? (-1) : (0);
}

int _osal_fifo_deinit(OSAL_FIFO *pFifo)
{
	struct kfifo *fifo = NULL;

	if (!pFifo || !pFifo->pFifoBody) {
		pr_err("%s:pFifo = NULL or pFifo->pFifoBody = NULL, error\n",
				__func__);
		return -1;
	}

	fifo = (struct kfifo *)pFifo->pFifoBody;

	if (fifo)
		kfifo_free(fifo);

	return 0;
}

int _osal_fifo_size(OSAL_FIFO *pFifo)
{
	struct kfifo *fifo = NULL;
	int ret = 0;

	if (!pFifo || !pFifo->pFifoBody) {
		pr_err("%s:pFifo = NULL or pFifo->pFifoBody = NULL, error\n",
				__func__);
		return -1;
	}

	fifo = (struct kfifo *)pFifo->pFifoBody;

	if (fifo)
		ret = kfifo_size(fifo);

	return ret;
}

/*returns unused bytes in fifo*/
int _osal_fifo_avail_size(OSAL_FIFO *pFifo)
{
	struct kfifo *fifo = NULL;
	int ret = 0;

	if (!pFifo || !pFifo->pFifoBody) {
		pr_err("%s:pFifo = NULL or pFifo->pFifoBody = NULL, error\n",
				__func__);
		return -1;
	}

	fifo = (struct kfifo *)pFifo->pFifoBody;

	if (fifo)
		ret = kfifo_avail(fifo);

	return ret;
}

/*returns used bytes in fifo*/
int _osal_fifo_len(OSAL_FIFO *pFifo)
{
	struct kfifo *fifo = NULL;
	int ret = 0;

	if (!pFifo || !pFifo->pFifoBody) {
		pr_err("%s:pFifo = NULL or pFifo->pFifoBody = NULL, error\n",
				__func__);
		return -1;
	}

	fifo = (struct kfifo *)pFifo->pFifoBody;

	if (fifo)
		ret = kfifo_len(fifo);

	return ret;
}

int _osal_fifo_is_empty(OSAL_FIFO *pFifo)
{
	struct kfifo *fifo = NULL;
	int ret = 0;

	if (!pFifo || !pFifo->pFifoBody) {
		pr_err("%s:pFifo = NULL or pFifo->pFifoBody = NULL, error\n",
				__func__);
		return -1;
	}

	fifo = (struct kfifo *)pFifo->pFifoBody;

	if (fifo)
		ret = kfifo_is_empty(fifo);

	return ret;
}

int _osal_fifo_is_full(OSAL_FIFO *pFifo)
{
	struct kfifo *fifo = NULL;
	int ret = 0;

	if (!pFifo || !pFifo->pFifoBody) {
		pr_err("%s:pFifo = NULL or pFifo->pFifoBody = NULL, error\n",
				__func__);
		return -1;
	}

	fifo = (struct kfifo *)pFifo->pFifoBody;

	if (fifo)
		ret = kfifo_is_full(fifo);

	return ret;
}

int _osal_fifo_data_in(OSAL_FIFO *pFifo, const void *buf, unsigned int len)
{
	struct kfifo *fifo = NULL;
	int ret = 0;

	if (!pFifo || !pFifo->pFifoBody) {
		pr_err("%s:pFifo = NULL or pFifo->pFifoBody = NULL, error\n",
				__func__);
		return -1;
	}

	fifo = (struct kfifo *)pFifo->pFifoBody;

	if (fifo && buf && (len <= _osal_fifo_avail_size(pFifo))) {
		ret = kfifo_in(fifo, buf, len);
	} else {
		pr_err("%s: kfifo_in, error, len = %d, _osal_fifo_avail_size = %d, buf=%p\n",
		       __func__, len, _osal_fifo_avail_size(pFifo), buf);

		ret = 0;
	}

	return ret;
}

int _osal_fifo_data_out(OSAL_FIFO *pFifo, void *buf, unsigned int len)
{
	struct kfifo *fifo = NULL;
	int ret = 0;

	if (!pFifo || !pFifo->pFifoBody) {
		pr_err("%s:pFifo = NULL or pFifo->pFifoBody = NULL, error\n"
				, __func__);
		return -1;
	}

	fifo = (struct kfifo *)pFifo->pFifoBody;

	if (fifo && buf && (len <= _osal_fifo_len(pFifo))) {
		ret = kfifo_out(fifo, buf, len);
	} else {
		pr_err("%s: kfifo_out, error, len = %d, osal_fifo_len = %d, buf=%p\n",
		       __func__, len, _osal_fifo_len(pFifo), buf);

		ret = 0;
	}

	return ret;
}

int _osal_fifo_reset(OSAL_FIFO *pFifo)
{
	struct kfifo *fifo = NULL;

	if (!pFifo || !pFifo->pFifoBody) {
		pr_err("%s:pFifo = NULL or pFifo->pFifoBody = NULL, error\n",
				__func__);
		return -1;
	}

	fifo = (struct kfifo *)pFifo->pFifoBody;

	if (fifo)
		kfifo_reset(fifo);

	return 0;
}

int osal_fifo_init(P_OSAL_FIFO pFifo, unsigned char *buffer, unsigned int size)
{
	if (!pFifo) {
		pr_err("%s:pFifo = NULL, error\n", __func__);
		return -1;
	}

	pFifo->FifoInit = _osal_fifo_init;
	pFifo->FifoDeInit = _osal_fifo_deinit;
	pFifo->FifoSz = _osal_fifo_size;
	pFifo->FifoAvailSz = _osal_fifo_avail_size;
	pFifo->FifoLen = _osal_fifo_len;
	pFifo->FifoIsEmpty = _osal_fifo_is_empty;
	pFifo->FifoIsFull = _osal_fifo_is_full;
	pFifo->FifoDataIn = _osal_fifo_data_in;
	pFifo->FifoDataOut = _osal_fifo_data_out;
	pFifo->FifoReset = _osal_fifo_reset;

	if (pFifo->pFifoBody != NULL) {
		pr_err("%s:Because pFifo room is avialable, we clear the room and allocate them again.\n", __func__);
		pFifo->FifoDeInit(pFifo->pFifoBody);
		pFifo->pFifoBody = NULL;
	}

	pFifo->FifoInit(pFifo, buffer, size);

	return 0;
}

void osal_fifo_deinit(P_OSAL_FIFO pFifo)
{
	if (pFifo)
		pFifo->FifoDeInit(pFifo);
	else {
		pr_err("%s:pFifo = NULL, error\n", __func__);
		return;
	}
	kfree(pFifo->pFifoBody);
}

int osal_fifo_reset(P_OSAL_FIFO pFifo)
{
	int ret = -1;

	if (pFifo) {
		ret = pFifo->FifoReset(pFifo);
	} else {
		pr_err("%s:pFifo = NULL, error\n", __func__);
		ret = -1;
	}
	return ret;
}

unsigned int osal_fifo_in(P_OSAL_FIFO pFifo,
			unsigned char *buffer, unsigned int size)
{
	unsigned int ret = 0;

	if (pFifo) {
		ret = pFifo->FifoDataIn(pFifo, buffer, size);
	} else {
		pr_err("%s:pFifo = NULL, error\n", __func__);
		ret = 0;
	}

	return ret;
}

unsigned int osal_fifo_out(P_OSAL_FIFO pFifo,
			unsigned char *buffer, unsigned int size)
{
	unsigned int ret = 0;

	if (pFifo) {
		ret = pFifo->FifoDataOut(pFifo, buffer, size);
	} else {
		pr_err("%s:pFifo = NULL, error\n", __func__);
		ret = 0;
	}

	return ret;
}

unsigned int osal_fifo_len(P_OSAL_FIFO pFifo)
{
	unsigned int ret = 0;

	if (pFifo) {
		ret = pFifo->FifoLen(pFifo);
	} else {
		pr_err("%s:pFifo = NULL, error\n", __func__);
		ret = 0;
	}

	return ret;
}

unsigned int osal_fifo_sz(P_OSAL_FIFO pFifo)
{
	unsigned int ret = 0;

	if (pFifo) {
		ret = pFifo->FifoSz(pFifo);
	} else {
		pr_err("%s:pFifo = NULL, error\n", __func__);
		ret = 0;
	}

	return ret;
}

unsigned int osal_fifo_avail(P_OSAL_FIFO pFifo)
{
	unsigned int ret = 0;

	if (pFifo) {
		ret = pFifo->FifoAvailSz(pFifo);
	} else {
		pr_err("%s:pFifo = NULL, error\n", __func__);
		ret = 0;
	}

	return ret;
}

unsigned int osal_fifo_is_empty(P_OSAL_FIFO pFifo)
{
	unsigned int ret = 0;

	if (pFifo) {
		ret = pFifo->FifoIsEmpty(pFifo);
	} else {
		pr_err("%s:pFifo = NULL, error\n", __func__);
		ret = 0;
	}

	return ret;
}

unsigned int osal_fifo_is_full(P_OSAL_FIFO pFifo)
{
	unsigned int ret = 0;

	if (pFifo) {
		ret = pFifo->FifoIsFull(pFifo);
	} else {
		pr_err("%s:pFifo = NULL, error\n", __func__);
		ret = 0;
	}
	return ret;
}

/*
 * sleepable lock operations APIs
 * init
 * lock
 * unlock
 * destroy
 *
 */
#if !defined(CONFIG_PROVE_LOCKING)
int osal_unsleepable_lock_init(P_OSAL_UNSLEEPABLE_LOCK pUSL)
{
	spin_lock_init(&(pUSL->lock));
	return 0;
}
#endif

int osal_lock_unsleepable_lock(P_OSAL_UNSLEEPABLE_LOCK pUSL)
{
	spin_lock_irqsave(&(pUSL->lock), pUSL->flag);
	return 0;
}

int osal_unlock_unsleepable_lock(P_OSAL_UNSLEEPABLE_LOCK pUSL)
{
	spin_unlock_irqrestore(&(pUSL->lock), pUSL->flag);
	return 0;
}

int osal_unsleepable_lock_deinit(P_OSAL_UNSLEEPABLE_LOCK pUSL)
{
	return 0;
}

/*
 * unsleepable operations APIs
 * init
 * lock
 * unlock
 * destroy
 *
 */

#if !defined(CONFIG_PROVE_LOCKING)
int osal_sleepable_lock_init(P_OSAL_SLEEPABLE_LOCK pSL)
{
	mutex_init(&pSL->lock);
	return 0;
}
#endif

int osal_lock_sleepable_lock(P_OSAL_SLEEPABLE_LOCK pSL)
{
	mutex_lock(&pSL->lock);
	return 0;
}

int osal_unlock_sleepable_lock(P_OSAL_SLEEPABLE_LOCK pSL)
{
	mutex_unlock(&pSL->lock);
	return 0;
}

int osal_trylock_sleepable_lock(P_OSAL_SLEEPABLE_LOCK pSL)
{
	return mutex_trylock(&pSL->lock);
}

int osal_sleepable_lock_deinit(P_OSAL_SLEEPABLE_LOCK pSL)
{
	mutex_destroy(&pSL->lock);
	return 0;
}

int osal_sleep_ms(unsigned int ms)
{
	msleep(ms);
	return 0;
}

int osal_udelay(unsigned int us)
{
	udelay(us);
	return 0;
}

int osal_usleep_range(unsigned long min, unsigned long max)
{
	usleep_range(min, max);
	return 0;
}

int osal_gettimeofday(struct timespec64 *tv)
{
	if (tv == NULL)
		return -1;

	ktime_get_real_ts64(tv);

	return 0;
}

void osal_get_local_time(unsigned long long *sec, unsigned long *nsec)
{
	if (sec != NULL && nsec != NULL) {
		*sec = local_clock();
		*nsec = do_div(*sec, 1000000000)/1000;
	} else
		pr_err("The input parameters error when get local time\n");
}

unsigned long long osal_elapsed_us(unsigned long long ts, unsigned long usec)
{
	unsigned long long current_ts = 0;
	unsigned long current_usec = 0;

	osal_get_local_time(&current_ts, &current_usec);
	return (current_ts*1000000 + current_usec) - (ts*1000000 + usec);
}

unsigned int osal_op_get_id(P_OSAL_OP pOp)
{
	return (pOp) ? pOp->op.opId : 0xFFFFFFFF;
}

MTK_CONN_BOOL osal_op_is_wait_for_signal(P_OSAL_OP pOp)
{
	return (pOp && pOp->signal.timeoutValue)
			? MTK_CONN_BOOL_TRUE : MTK_CONN_BOOL_FALSE;
}

void osal_op_raise_signal(P_OSAL_OP pOp, int result)
{
	if (pOp) {
		pOp->result = result;
		osal_raise_signal(&pOp->signal);
	}
}

void osal_set_op_result(P_OSAL_OP pOp, int result)
{
	if (pOp)
		pOp->result = result;

}

MTK_CONN_BOOL osal_opq_has_op(P_OSAL_OP_Q pOpQ, P_OSAL_OP pOp)
{
	unsigned int rd;
	unsigned int wt;
	P_OSAL_OP op;

	rd = pOpQ->read;
	wt = pOpQ->write;

	while (rd != wt) {
		op = pOpQ->queue[rd & RB_MASK(pOpQ)];
		if (op == pOp)
			return MTK_CONN_BOOL_TRUE;
		rd++;
	}
	return MTK_CONN_BOOL_FALSE;
}

static void osal_op_history_print_work(struct work_struct *work)
{
	struct osal_op_history *log_history
			= container_of(work, struct osal_op_history, dump_work);
	struct ring *ring_buffer = &log_history->dump_ring_buffer;
	struct ring_segment seg;
	struct osal_op_history_entry *queue = ring_buffer->base;
	struct osal_op_history_entry *entry;
	int index = 0;

	if (queue == NULL) {
		pr_info("queue shouldn't be NULL, %s", log_history->name);
		return;
	}

	RING_READ_FOR_EACH_ITEM(RING_SIZE(ring_buffer), seg, ring_buffer) {
		index = seg.ring_pt - ring_buffer->base;
		entry = &queue[index];
		pr_info("(%llu.%06lu) %s: pOp(%p):%u(%d)-%x-%zx,%zx,%zx,%zx\n",
			entry->ts,
			entry->usec,
			log_history->name,
			entry->opbuf_address,
			entry->op_id,
			entry->opbuf_ref_count,
			entry->op_info_bit,
			entry->param_0,
			entry->param_1,
			entry->param_2,
			entry->param_3);
	}
	kfree(queue);
	ring_buffer->base = NULL;
}

void osal_op_history_init(struct osal_op_history *log_history, int queue_size)
{
	int size = queue_size * sizeof(struct osal_op_history_entry);

	spin_lock_init(&(log_history->lock));

	log_history->queue = kzalloc(size, GFP_ATOMIC);
	if (log_history->queue == NULL)
		return;

	/* queue_size must be power of 2 */
	ring_init(
		&log_history->queue,
		queue_size,
		0,
		0,
		&log_history->ring_buffer);

	INIT_WORK(&log_history->dump_work, osal_op_history_print_work);
}

void osal_op_history_print(struct osal_op_history *log_history, char *name)
{
	struct osal_op_history_entry *queue;
	struct ring *ring_buffer, *dump_ring_buffer;
	int queue_size;
	unsigned long flags;
	struct work_struct *work = &log_history->dump_work;
	spinlock_t *lock = &(log_history->lock);

	if (log_history->queue == NULL) {
		pr_info("Queue is NULL, name: %s\n", name);
		return;
	}

	ring_buffer = &log_history->ring_buffer;
	queue_size = sizeof(struct osal_op_history_entry)
			 * RING_SIZE(ring_buffer);

	/* Allocate memory before getting lock to save time of holding lock */
	queue = kmalloc(queue_size, GFP_KERNEL);
	if (queue == NULL)
		return;

	dump_ring_buffer = &log_history->dump_ring_buffer;

	spin_lock_irqsave(lock, flags);
	if (dump_ring_buffer->base != NULL) {
		spin_unlock_irqrestore(lock, flags);
		kfree(queue);
		pr_info("print is ongoing: %s\n", name);
		return;
	}

	osal_snprintf(log_history->name, sizeof(log_history->name), "%s", name);
	osal_memcpy(queue, log_history->queue, queue_size);
	osal_memcpy(dump_ring_buffer, ring_buffer, sizeof(struct ring));
	/* assign value to base after memory copy */
	dump_ring_buffer->base = queue;
	spin_unlock_irqrestore(lock, flags);
	schedule_work(work);
}

void osal_op_history_save(struct osal_op_history *log_history, P_OSAL_OP pOp)
{
	struct osal_op_history_entry *entry = NULL;
	struct ring_segment seg;
	int index;
	unsigned long long sec = 0;
	unsigned long usec = 0;
	unsigned long flags;

	if (log_history->queue == NULL)
		return;

	osal_get_local_time(&sec, &usec);

	spin_lock_irqsave(&(log_history->lock), flags);
	RING_OVERWRITE_FOR_EACH(1, seg, &log_history->ring_buffer) {
		index = seg.ring_pt - log_history->ring_buffer.base;
		entry = &log_history->queue[index];
	}

	if (entry == NULL) {
		pr_info("Entry is null, size %d\n",
				RING_SIZE(&log_history->ring_buffer));
		spin_unlock_irqrestore(&(log_history->lock), flags);
		return;
	}

	entry->opbuf_address = pOp;
	entry->op_id = pOp->op.opId;
	entry->opbuf_ref_count = atomic_read(&pOp->ref_count);
	entry->op_info_bit = pOp->op.u4InfoBit;
	entry->param_0 = pOp->op.au4OpData[0];
	entry->param_1 = pOp->op.au4OpData[1];
	entry->param_2 = pOp->op.au4OpData[2];
	entry->param_3 = pOp->op.au4OpData[3];
	entry->ts = sec;
	entry->usec = usec;
	spin_unlock_irqrestore(&(log_history->lock), flags);
}

int osal_wake_lock_init(struct osal_wake_lock *pLock)
{
	if (!pLock)
		return -1;
	if (pLock->init_flag == 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 149))
		pLock->wake_lock = wakeup_source_register(NULL, pLock->name);
#else
		pLock->wake_lock = wakeup_source_register(pLock->name);
#endif
		pLock->init_flag = 1;
	}
	return 0;
}

int osal_wake_lock_deinit(struct osal_wake_lock *pLock)
{
	if (!pLock)
		return -1;
	if (pLock->init_flag == 1) {
		wakeup_source_unregister(pLock->wake_lock);
		pLock->init_flag = 0;
	} else
		pr_info("%s: wake_lock is not initialized!\n", __func__);
	return 0;
}

int osal_wake_lock(struct osal_wake_lock *pLock)
{
	if (!pLock)
		return -1;
	if (pLock->init_flag == 1)
		__pm_stay_awake(pLock->wake_lock);
	else
		pr_info("%s: wake_lock is not initialized!\n", __func__);
	return 0;
}

int osal_wake_unlock(struct osal_wake_lock *pLock)
{
	if (!pLock)
		return -1;
	if (pLock->init_flag == 1)
		__pm_relax(pLock->wake_lock);
	else
		pr_info("%s: wake_lock is not initialized!\n", __func__);
	return 0;
}

int osal_wake_lock_count(struct osal_wake_lock *pLock)
{
	int count = 0;
	if (!pLock)
		return -1;
	if (pLock->init_flag == 1)
		count = pLock->wake_lock->active;
	else
		pr_info("%s: wake_lock is not initialized!\n", __func__);
	return count;
}
