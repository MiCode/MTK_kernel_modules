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
#include "gps_dl_config.h"

#include "gps_dl_osal.h"
#include "gps_dl_log.h"

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

int osal_err_print(const char *str, ...)
{
	va_list args;
	char tempString[DBG_LOG_STR_SIZE];

	va_start(args, str);
	vsnprintf(tempString, DBG_LOG_STR_SIZE, str, args);
	va_end(args);

	GDL_LOGE("%s", tempString);

	return 0;
}

int osal_dbg_print(const char *str, ...)
{
	va_list args;
	char tempString[DBG_LOG_STR_SIZE];

	va_start(args, str);
	vsnprintf(tempString, DBG_LOG_STR_SIZE, str, args);
	va_end(args);

	GDL_LOGD("%s", tempString);

	return 0;
}

int osal_warn_print(const char *str, ...)
{
	va_list args;
	char tempString[DBG_LOG_STR_SIZE];

	va_start(args, str);
	vsnprintf(tempString, DBG_LOG_STR_SIZE, str, args);
	va_end(args);

	GDL_LOGW("%s", tempString);

	return 0;
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

int osal_thread_create(struct gps_dl_osal_thread *pThread)
{
	if (!pThread)
		return -1;

	pThread->pThread = kthread_create(pThread->pThreadFunc, pThread->pThreadData, pThread->threadName);
	if (pThread->pThread == NULL)
		return -1;

	return 0;
}

int osal_thread_run(struct gps_dl_osal_thread *pThread)
{
	if ((pThread) && (pThread->pThread)) {
		wake_up_process(pThread->pThread);
		return 0;
	} else
		return -1;
}

int osal_thread_stop(struct gps_dl_osal_thread *pThread)
{
	int iRet;

	if ((pThread) && (pThread->pThread)) {
		iRet = kthread_stop(pThread->pThread);
		/* pThread->pThread = NULL; */
		return iRet;
	}
	return -1;
}

int osal_thread_should_stop(struct gps_dl_osal_thread *pThread)
{
	if ((pThread) && (pThread->pThread))
		return kthread_should_stop();
	else
		return 1;

}

int osal_thread_destroy(struct gps_dl_osal_thread *pThread)
{
	if (pThread && (pThread->pThread)) {
		kthread_stop(pThread->pThread);
		pThread->pThread = NULL;
	}
	return 0;
}

int osal_thread_wait_for_event(struct gps_dl_osal_thread *pThread,
	struct gps_dl_osal_event *pEvent, OSAL_EVENT_CHECKER pChecker)
{
	if ((pThread) && (pThread->pThread) && (pEvent) && (pChecker)) {
		return wait_event_interruptible(pEvent->waitQueue, (
									   osal_thread_should_stop(pThread)
									   || (*pChecker) (pThread)));
	}
	return -1;
}

int osal_signal_init(struct gps_dl_osal_signal *pSignal)
{
	if (pSignal) {
		init_completion(&pSignal->comp);
		return 0;
	} else
		return -1;
}

int osal_wait_for_signal(struct gps_dl_osal_signal *pSignal)
{
	if (pSignal) {
		wait_for_completion_interruptible(&pSignal->comp);
		return 0;
	} else
		return -1;
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
int osal_wait_for_signal_timeout(struct gps_dl_osal_signal *pSignal, struct gps_dl_osal_thread *pThread)
{
	/* struct gps_dl_osal_thread_schedstats schedstats; */
	/* int waitRet; */

	/* return wait_for_completion_interruptible_timeout(&pSignal->comp, msecs_to_jiffies(pSignal->timeoutValue)); */
	/* [ChangeFeature][George] gps driver may be closed by -ERESTARTSYS.
	 * Avoid using *interruptible" version in order to complete our jobs, such
	 * as function off gracefully.
	 */
	if (!pThread || !pThread->pThread)
		return wait_for_completion_timeout(&pSignal->comp, msecs_to_jiffies(pSignal->timeoutValue));
#if 0
	do {
		osal_thread_sched_mark(pThread, &schedstats);
		waitRet = wait_for_completion_timeout(&pSignal->comp, msecs_to_jiffies(pSignal->timeoutValue));
		osal_thread_sched_unmark(pThread, &schedstats);

		if (waitRet > 0)
			break;

		if (schedstats.runnable > schedstats.exec) {
			osal_err_print(
				"[E]%s:wait completion timeout, %s cannot get CPU, extension(%d), show backtrace:\n",
				__func__,
				pThread->threadName,
				pSignal->timeoutExtension);
		} else {
			osal_err_print("[E]%s:wait completion timeout, show %s backtrace:\n",
				__func__,
				pThread->threadName);
			pSignal->timeoutExtension = 0;
		}
		osal_err_print("[E]%s:\tduration:%llums, sched(x%llu/r%llu/i%llu)\n",
			__func__,
			schedstats.time,
			schedstats.exec,
			schedstats.runnable,
			schedstats.iowait);
		/*
		 * no need to disginguish combo or A/D die projects
		 * osal_dump_thread_state will just return if target thread does not exist
		 */
		osal_dump_thread_state("mtk_wmtd");
		osal_dump_thread_state("btif_rxd");
		osal_dump_thread_state("mtk_stp_psm");
		osal_dump_thread_state("mtk_stp_btm");
		osal_dump_thread_state("stp_sdio_tx_rx");
	} while (pSignal->timeoutExtension--);
	return waitRet;
#endif
	return 1;
}

int osal_raise_signal(struct gps_dl_osal_signal *pSignal)
{
	if (pSignal) {
		complete(&pSignal->comp);
		return 0;
	} else
		return -1;
}

int osal_signal_active_state(struct gps_dl_osal_signal *pSignal)
{
	if (pSignal)
		return pSignal->timeoutValue;
	else
		return -1;
}

int osal_op_is_wait_for_signal(struct gps_dl_osal_lxop *pOp)
{
	return (pOp && pOp->signal.timeoutValue) ? 1 : 0;
}

void osal_op_raise_signal(struct gps_dl_osal_lxop *pOp, int result)
{
	if (pOp) {
		pOp->result = result;
		osal_raise_signal(&pOp->signal);
	}
}

int osal_signal_deinit(struct gps_dl_osal_signal *pSignal)
{
	if (pSignal) {
		pSignal->timeoutValue = 0;
		return 0;
	} else
		return -1;
}

/*
  *OSAL layer Event Opeartion related APIs
  *initialization
  *wait for signal
  *wait for signal timerout
  *raise signal
  *destroy a signal
  *
*/

int osal_event_init(struct gps_dl_osal_event *pEvent)
{
	if (pEvent) {
		init_waitqueue_head(&pEvent->waitQueue);
		return 0;
	}
	return -1;
}

int osal_trigger_event(struct gps_dl_osal_event *pEvent)
{
	int ret = 0;

	if (pEvent) {
		wake_up_interruptible(&pEvent->waitQueue);
		return ret;
	}
	return -1;
}

int osal_wait_for_event(struct gps_dl_osal_event *pEvent, int (*condition)(void *), void *cond_pa)
{
	if (pEvent)
		return wait_event_interruptible(pEvent->waitQueue, condition(cond_pa));
	else
		return -1;
}

int osal_wait_for_event_timeout(struct gps_dl_osal_event *pEvent, int (*condition)(void *), void *cond_pa)
{
	if (pEvent)
		return wait_event_interruptible_timeout(pEvent->waitQueue,
							condition(cond_pa),
							msecs_to_jiffies(pEvent->timeoutValue));
	return -1;
}

int osal_event_deinit(struct gps_dl_osal_event *pEvent)
{
	return 0;
}

int osal_sleepable_lock_init(struct gps_dl_osal_sleepable_lock *pSL)
{
	mutex_init(&pSL->lock);
	return 0;
}

int osal_lock_sleepable_lock(struct gps_dl_osal_sleepable_lock *pSL)
{
	return mutex_lock_killable(&pSL->lock);
}

int osal_unlock_sleepable_lock(struct gps_dl_osal_sleepable_lock *pSL)
{
	mutex_unlock(&pSL->lock);
	return 0;
}

int osal_trylock_sleepable_lock(struct gps_dl_osal_sleepable_lock *pSL)
{
	return mutex_trylock(&pSL->lock);
}

int osal_sleepable_lock_deinit(struct gps_dl_osal_sleepable_lock *pSL)
{
	mutex_destroy(&pSL->lock);
	return 0;
}

int osal_unsleepable_lock_init(struct gps_dl_osal_unsleepable_lock *pUSL)
{
	spin_lock_init(&(pUSL->lock));
	return 0;
}

int osal_lock_unsleepable_lock(struct gps_dl_osal_unsleepable_lock *pUSL)
{
	spin_lock_irqsave(&(pUSL->lock), pUSL->flag);
	return 0;
}

int osal_unlock_unsleepable_lock(struct gps_dl_osal_unsleepable_lock *pUSL)
{
	spin_unlock_irqrestore(&(pUSL->lock), pUSL->flag);
	return 0;
}

