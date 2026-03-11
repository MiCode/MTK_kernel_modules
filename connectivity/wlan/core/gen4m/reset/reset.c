// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   reset.c
*   \brief  reset module
*
*    This file contains all implementations of reset module
*/


/**********************************************************************
*                         C O M P I L E R   F L A G S
***********************************************************************
*/

/**********************************************************************
*                    E X T E R N A L   R E F E R E N C E S
***********************************************************************
*/
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/reboot.h>
#include <linux/preempt.h>
#include "reset.h"

/**********************************************************************
*                                 M A C R O S
***********************************************************************
*/
MODULE_LICENSE("Dual BSD/GPL");

#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC 1000
#endif

/**********************************************************************
*                              C O N S T A N T S
***********************************************************************
*/
const char *eventName[RFSM_EVENT_MAX + 1] = {
	"RFSM_EVENT_PROBED",
	"RFSM_EVENT_REMOVED",
	"RFSM_EVENT_TIMEOUT",
	"RFSM_EVENT_READY",
	"RFSM_EVENT_TRIGGER_RESET",
	"RFSM_EVENT_RESET_DONE",
	"RFSM_EVENT_TRIGGER_POWER_OFF",
	"RFSM_EVENT_POWER_OFF_DONE",
	"RFSM_EVENT_TRIGGER_POWER_ON",
	"RFSM_EVENT_POWER_ON_DONE",
	"RFSM_EVENT_START_PROBE",
	"RFSM_EVENT_All"
};


/**********************************************************************
*                             D A T A   T Y P E S
***********************************************************************
*/
struct ResetInfo {
	wait_queue_head_t resetko_waitq;
	struct task_struct *resetko_thread;
	struct delayed_work resetWork;

	struct mutex moduleMutex;
	spinlock_t eventLock;
	struct list_head moduleList;
	struct list_head eventList;
};

struct ResetEvent {
	struct list_head node;
	enum ModuleType module;
	enum ResetFsmEvent event;
};

struct NotifyEvent {
	struct list_head node;
	enum ModuleNotifyEvent event;
};

/**********************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
***********************************************************************
*/
static int reset_reboot_notify(struct notifier_block *nb,
				unsigned long event, void *unused);
static void removeResetEvent(enum ModuleType module,
			    enum ResetFsmEvent event);

/**********************************************************************
*                            P U B L I C   D A T A
***********************************************************************
*/

/**********************************************************************
*                           P R I V A T E   D A T A
***********************************************************************
*/
static struct ResetInfo resetInfo = {0};
static char moduleName[RESET_MODULE_TYPE_MAX][RFSM_NAME_MAX_LEN];
static bool fgIsPowerOff;
static bool fgExit;
static struct notifier_block resetRebootNotifier = {
	.notifier_call = reset_reboot_notify,
	.next = NULL,
	.priority = 0,
};

/**********************************************************************
*                              F U N C T I O N S
**********************************************************************/
static struct FsmEntity *findResetFsm(enum ModuleType module)
{
	struct FsmEntity *fsm, *next_fsm;

	list_for_each_entry_safe(fsm, next_fsm, &resetInfo.moduleList, node) {
		if (fsm->eModuleType == module)
			return fsm;
	}

	return NULL;
}

static void removeResetFsm(enum ModuleType module)
{
	struct FsmEntity *fsm, *next_fsm;

	list_for_each_entry_safe(fsm, next_fsm, &resetInfo.moduleList, node) {
		if (fsm->eModuleType == module) {
			list_del(&fsm->node);
			freeResetFsm(fsm);
		}
	}
}

static void addResetFsm(struct FsmEntity *fsm)
{
	if (!fsm)
		return;

	list_add_tail(&fsm->node, &resetInfo.moduleList);
}

static struct ResetEvent *allocResetEvent(void)
{
	return kmalloc(sizeof(struct ResetEvent),
		       in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
}

static void freeResetEvent(struct ResetEvent *event)
{
	if (!event)
		kfree(event);
}

static bool isEventEmpty(void)
{
	unsigned long flags;
	bool ret;

	spin_lock_irqsave(&resetInfo.eventLock, flags);
	if (list_empty(&resetInfo.eventList))
		ret = true;
	else
		ret = false;
	spin_unlock_irqrestore(&resetInfo.eventLock, flags);

	return ret;
}

static void pushResetEvent(struct ResetEvent *event)
{
	unsigned long flags;

	if (!event ||
	    ((unsigned int)event->module >= RESET_MODULE_TYPE_MAX) ||
	    ((unsigned int)event->event >= RFSM_EVENT_MAX)) {
		MR_Err("%s: argument error\n", __func__);
		return;
	}

	spin_lock_irqsave(&resetInfo.eventLock, flags);
	list_add_tail(&event->node, &resetInfo.eventList);
	spin_unlock_irqrestore(&resetInfo.eventLock, flags);
}

static struct ResetEvent *popResetEvent(void)
{
	struct ResetEvent *event = NULL;
	unsigned long flags;

	spin_lock_irqsave(&resetInfo.eventLock, flags);
	if (list_empty(&resetInfo.eventList))
		goto POP_EVT_RETURN;
	event = list_first_entry(&resetInfo.eventList, struct ResetEvent, node);
	list_del(&event->node);

POP_EVT_RETURN:
	spin_unlock_irqrestore(&resetInfo.eventLock, flags);
	return event;
}

static void removeResetEvent(enum ModuleType module,
			    enum ResetFsmEvent event)
{
	struct ResetEvent *cur, *next;
	unsigned long flags;

	spin_lock_irqsave(&resetInfo.eventLock, flags);
	if (list_empty(&resetInfo.eventList)) {
		goto REMOVE_EVT_RETURN;
	}
	list_for_each_entry_safe(cur, next, &resetInfo.eventList, node) {
		if ((cur->module == module) &&
		    ((cur->event == event) || (event == RFSM_EVENT_All))) {
			list_del(&cur->node);
			freeResetEvent(cur);
		}
	}
REMOVE_EVT_RETURN:
	spin_unlock_irqrestore(&resetInfo.eventLock, flags);
}

static int reset_reboot_notify(struct notifier_block *nb,
					  unsigned long event, void *unused)
{
	enum ModuleType module;
	(void)nb;
	(void)unused;

	if (event == SYS_RESTART ||
	    event == SYS_POWER_OFF ||
	    event == SYS_HALT) {
		fgExit = true;
		mutex_lock(&resetInfo.moduleMutex);
		for (module = RESET_MODULE_TYPE_WIFI;
		     module < RESET_MODULE_TYPE_MAX;
		     module++) {
			removeResetEvent(module, RFSM_EVENT_All);
			wakeupSourceRelax(findResetFsm(module));
		}
		mutex_unlock(&resetInfo.moduleMutex);
	}

	return 0;
}

static int resetko_thread_main(void *data)
{
	struct FsmEntity *fsm;
	struct ResetEvent *resetEvent;
	enum ModuleType module, begin, end;
	int ret = 0;
	unsigned int evt;

	MR_Info("%s: start\n", __func__);

	while (!fgExit) {
		do {
			ret = wait_event_interruptible(resetInfo.resetko_waitq,
						       isEventEmpty() == false);
		} while (ret != 0);

		while (resetEvent = popResetEvent(), resetEvent != NULL) {
			evt = (unsigned int)resetEvent->event;
			if (evt > RFSM_EVENT_MAX) {
				freeResetEvent(resetEvent);
				continue;
			}
			mutex_lock(&resetInfo.moduleMutex);
			/* loop for all related module */
			if ((evt == RFSM_EVENT_TRIGGER_RESET) ||
			    (evt == RFSM_EVENT_TRIGGER_POWER_OFF) ||
			    (evt == RFSM_EVENT_TRIGGER_POWER_ON) ||
			    (evt == RFSM_EVENT_RESET_DONE) ||
			    (evt == RFSM_EVENT_POWER_OFF_DONE) ||
			    (evt == RFSM_EVENT_START_PROBE) ||
			    (evt == RFSM_EVENT_POWER_ON_DONE)) {
				begin = 0;
				end = RESET_MODULE_TYPE_MAX - 1;
			} else {
				begin = resetEvent->module;
				end = resetEvent->module;
			}
			for (module = begin; module <= end; module++) {
				fsm = findResetFsm(module);
				if (fsm != NULL) {
					if (evt == RFSM_EVENT_READY)
						fsm->fgReady = ~false;
					MR_Info("[%s] in [%s] state rcv [%s]\n",
						fsm->name,
						fsm->fsmState->name,
						eventName[evt]);
					resetFsmHandlevent(fsm, evt);
				}
			}
			mutex_unlock(&resetInfo.moduleMutex);
			freeResetEvent(resetEvent);
		}
	}

	MR_Info("%s: stop\n", __func__);

	return 0;
}

void resetkoNotifyEvent(struct FsmEntity *fsm, enum ModuleNotifyEvent event)
{
	struct NotifyEvent *prEvent;

	if (!fsm) {
		MR_Err("%s: fsm is NULL\n", __func__);
		return;
	}
	if ((unsigned int)event >= MODULE_NOTIFY_MAX)
		return;
	if (fsm->notifyFunc == NULL)
		return;

	prEvent = kmalloc(sizeof(struct NotifyEvent),
			  in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
	if (!prEvent) {
		MR_Err("%s: alloc notify event (%d) fail\n", __func__, event);
		return;
	}
	prEvent->event = event;
	mutex_lock(&(fsm->notifyEventMutex));
	list_add_tail(&prEvent->node, &(fsm->notifyEventList));
	mutex_unlock(&(fsm->notifyEventMutex));

	schedule_delayed_work(&fsm->notifyWork, 0);
}

void resetkoNotifyWork(struct work_struct *work)
{
	struct NotifyEvent *prEvent;
	struct FsmEntity *fsm;
	struct delayed_work *delay_work;

	delay_work = to_delayed_work(work);
	fsm = container_of(delay_work, struct FsmEntity, notifyWork);

	while (1) {
		mutex_lock(&fsm->notifyEventMutex);
		if (list_empty(&fsm->notifyEventList)) {
			mutex_unlock(&fsm->notifyEventMutex);
			break;
		}
		prEvent = list_first_entry(&fsm->notifyEventList,
					   struct NotifyEvent, node);
		list_del(&prEvent->node);
		mutex_unlock(&(fsm->notifyEventMutex));

		MR_Info("[%s] %s %d\n", fsm->name, __func__, prEvent->event);
		fsm->notifyFunc((unsigned int)prEvent->event, NULL);
		kfree(prEvent);
	}
}

void clearAllModuleReady(void)
{
	struct FsmEntity *fsm, *next_fsm;

	/* mutex is hold in function resetko_thread_main */
	list_for_each_entry_safe(fsm, next_fsm, &resetInfo.moduleList, node) {
		fsm->fgReady = false;
	}
}

bool isAllModuleReady(void)
{
	struct FsmEntity *fsm, *next_fsm;
	bool ret = ~false;

	/* mutex is hold in function resetko_thread_main */
	list_for_each_entry_safe(fsm, next_fsm, &resetInfo.moduleList, node) {
		MR_Info("[%s] %s: %s\n", fsm->name, __func__,
			fsm->fgReady ? "ready" : "not ready");
		if (!fsm->fgReady)
			ret = false;
	}

	return ret;
}

bool isAllModuleInState(const struct FsmState *state)
{
	struct FsmEntity *fsm, *next_fsm;
	bool ret = ~false;

	/* mutex is hold in function resetko_thread_main */
	list_for_each_entry_safe(fsm, next_fsm, &resetInfo.moduleList, node) {
		if (fsm->fsmState != state)
			ret = false;
	}

	return ret;
}

void wakeupSourceStayAwake(struct FsmEntity *fsm)
{
	if (!fsm)
		return;
	if (fsm->wakeupCount <= 0) {
		fsm->wakeupCount++;
#if CFG_RESETKO_ENABLE_WAKE_LOCK
		if (fsm->wakeupSource) {
			MR_Warn("[%s] %s\n", fsm->name, __func__);
			__pm_stay_awake(fsm->wakeupSource);
		}
#endif
	}
}

void wakeupSourceRelax(struct FsmEntity *fsm)
{
	if (!fsm)
		return;
	if (fsm->wakeupCount > 0) {
		fsm->wakeupCount--;
#if CFG_RESETKO_ENABLE_WAKE_LOCK
		if (fsm->wakeupSource) {
			MR_Warn("[%s] %s\n", fsm->name, __func__);
			__pm_relax(fsm->wakeupSource);
		}
#endif
	}
}

void powerOff(void)
{
	if (fgIsPowerOff)
		return;
	/* powerOff by hif type */
	/* 1. host func remove
	 * 2. pull reset pin
	 * 3. power off
	 */
	resetHif_SdioRemoveHost();
	resetHif_ResetGpioPull();
	resetHif_PowerGpioSwitchOff();

	fgIsPowerOff = true;
}

void powerOn(void)
{
	if (!fgIsPowerOff)
		return;

	/* powerON by hif type */
	/* 1. power on
	 * 2. release reset pin
	 * 3. host func add
	 */
	resetHif_PowerGpioSwitchOn();
	resetHif_ResetGpioRelease();
	resetHif_SdioAddHost();

	fgIsPowerOff = false;
}

void powerReset(void)
{
	schedule_delayed_work(&resetInfo.resetWork, 0);
}

void resetkoResetWork(struct work_struct *work)
{
	enum ModuleType module;

	powerOff();
	msleep(50);
	powerOn();

	/* reset done, all timer and event need clear */
	mutex_lock(&resetInfo.moduleMutex);
	for (module = RESET_MODULE_TYPE_WIFI;
	     module < RESET_MODULE_TYPE_MAX;
	     module++) {
		resetkoCancleTimer(findResetFsm(module));
		removeResetEvent(module, RFSM_EVENT_All);
	}
	mutex_unlock(&resetInfo.moduleMutex);

	send_reset_event(RESET_MODULE_TYPE_WIFI, RFSM_EVENT_RESET_DONE);
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void resetkoTimeoutHandler(struct timer_list *timer)
#else
static void resetkoTimeoutHandler(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct FsmEntity *fsm = from_timer(fsm, timer, resetTimer);
#else
	struct FsmEntity *fsm = (struct FsmEntity *)arg;
#endif
	if (!fsm) {
		MR_Err("%s: fsm is null\n", __func__);
		return;
	}

	MR_Info("[%s] %s\n", fsm->name, __func__);
	send_reset_event(fsm->eModuleType, RFSM_EVENT_TIMEOUT);
}

void resetkoStartTimer(struct FsmEntity *fsm, unsigned int ms)
{
	if (!fsm) {
		MR_Err("%s: fsm is null\n", __func__);
		return;
	}
	MR_Info("[%s] %s %dms\n", fsm->name, __func__, ms);
	if (ms == 0)
		return;

	mod_timer(&fsm->resetTimer, jiffies + ms * HZ / MSEC_PER_SEC);
}

void resetkoCancleTimer(struct FsmEntity *fsm)
{
	if (!fsm) {
		MR_Err("%s: fsm is null\n", __func__);
		return;
	}
	MR_Info("[%s] %s\n", fsm->name, __func__);

	del_timer(&fsm->resetTimer);
	removeResetEvent(fsm->eModuleType, RFSM_EVENT_TIMEOUT);
}

enum ReturnStatus send_reset_event(enum ModuleType module,
				 enum ResetFsmEvent event)
{
	struct ResetEvent *resetEvent;

	if (fgExit)
		return RESET_RETURN_STATUS_FAIL;
	dump_stack();

	if (((unsigned int)module >= RESET_MODULE_TYPE_MAX) ||
	    ((unsigned int)event >= RFSM_EVENT_MAX)) {
		MR_Err("%s: args error %d %d\n", __func__, module, event);
		return RESET_RETURN_STATUS_FAIL;
	}

	resetEvent = allocResetEvent();
	if (!resetEvent) {
		MR_Err("%s: allocResetEvent fail\n", __func__);
		return RESET_RETURN_STATUS_FAIL;
	}

	MR_Info("[%s] %s %s\n", moduleName[module], __func__, eventName[event]);
	resetEvent->module = module;
	resetEvent->event = event;
	pushResetEvent(resetEvent);

	wake_up_interruptible(&resetInfo.resetko_waitq);

	return RESET_RETURN_STATUS_SUCCESS;
}
EXPORT_SYMBOL(send_reset_event);

enum ReturnStatus send_msg_to_module(enum ModuleType srcModule,
				    enum ModuleType dstModule,
				    void *msg)
{
	struct FsmEntity *srcfsm, *dstfsm;

	if (fgExit)
		return RESET_RETURN_STATUS_FAIL;
	dump_stack();

	if (!msg) {
		MR_Err("%s: %d -> %d, msg is NULL\n",
			__func__, srcModule, dstModule);
	}

	mutex_lock(&resetInfo.moduleMutex);
	srcfsm = findResetFsm(srcModule);
	if (!srcfsm) {
		MR_Err("%s: src module (%d) not exist\n",
			__func__, srcModule);
		goto SEND_MSG_FAIL;
	}
	dstfsm = findResetFsm(dstModule);
	if (!dstfsm) {
		MR_Err("%s: dst module (%d) not exist\n",
			__func__, dstModule);
		goto SEND_MSG_FAIL;
	}
	if (!dstfsm->notifyFunc) {
		MR_Err("%s: dst module (%d) notifyFunc is NULL\n",
			__func__, dstModule);
		goto SEND_MSG_FAIL;
	}
	if (dstfsm->notifyFunc != NULL) {
		MR_Info("%s: module(%s) -> module(%s)\n",
			__func__, srcfsm->name, dstfsm->name);
		dstfsm->notifyFunc((unsigned int)MODULE_NOTIFY_MESSAGE, msg);
	}
	mutex_unlock(&resetInfo.moduleMutex);
	return RESET_RETURN_STATUS_SUCCESS;

SEND_MSG_FAIL:
	mutex_unlock(&resetInfo.moduleMutex);
	return RESET_RETURN_STATUS_FAIL;
}
EXPORT_SYMBOL(send_msg_to_module);


enum ReturnStatus update_hif_info(enum HifInfoType type, void *info)
{
	if (fgExit)
		return RESET_RETURN_STATUS_FAIL;

	if (!info)
		return RESET_RETURN_STATUS_FAIL;

	if (type == HIF_INFO_SDIO_HOST)
		return resetHif_UpdateSdioHost(info);

	return RESET_RETURN_STATUS_FAIL;
}
EXPORT_SYMBOL(update_hif_info);

enum ReturnStatus resetko_register_module(enum ModuleType module,
					char *name,
					enum TriggerResetApiType resetApiType,
					void *resetFunc,
					void *notifyFunc)
{
	struct FsmEntity *fsm;
	(void)resetApiType;
	(void)resetFunc;

	if (fgExit)
		return RESET_RETURN_STATUS_FAIL;
	dump_stack();

	if (!name) {
		MR_Err("%s: insmod module(%d) with no name\n",
			__func__, module);
		return RESET_RETURN_STATUS_FAIL;
	}
	fsm = allocResetFsm(name, module);
	if (!fsm) {
		MR_Err("%s: allocResetFsm module(%d) fail\n", __func__, module);
		return RESET_RETURN_STATUS_FAIL;
	}
	fsm->notifyFunc = (NotifyFunc)notifyFunc;
	mutex_init(&(fsm->notifyEventMutex));
	INIT_LIST_HEAD(&(fsm->notifyEventList));
	INIT_DELAYED_WORK(&(fsm->notifyWork), resetkoNotifyWork);

	mutex_lock(&resetInfo.moduleMutex);
	if (findResetFsm(module) != NULL) {
		mutex_unlock(&resetInfo.moduleMutex);
		MR_Err("%s: insmod module(%d) existed\n",
			__func__, module);
		freeResetFsm(fsm);
		return RESET_RETURN_STATUS_FAIL;
	}

#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	timer_setup(&fsm->resetTimer, resetkoTimeoutHandler, 0);
#else
	init_timer(&fsm->resetTimer);
	fsm->resetTimer.function = resetkoTimeoutHandler;
	fsm->resetTimer.data = (unsigned long)fsm;
#endif
	memcpy(&moduleName[module][0], fsm->name, RFSM_NAME_MAX_LEN);
	addResetFsm(fsm);
	mutex_unlock(&resetInfo.moduleMutex);

	MR_Info("[%s] %s, module type %d\n", name, __func__, module);

	return RESET_RETURN_STATUS_SUCCESS;
}
EXPORT_SYMBOL(resetko_register_module);


enum ReturnStatus resetko_unregister_module(enum ModuleType module)
{
	struct FsmEntity *fsm;
	struct NotifyEvent *cur, *next;

	dump_stack();

	mutex_lock(&resetInfo.moduleMutex);
	fsm = findResetFsm(module);
	if (!fsm) {
		mutex_unlock(&resetInfo.moduleMutex);
		MR_Err("%s: rmmod module(%d) not exist\n",
			__func__, module);
		return RESET_RETURN_STATUS_SUCCESS;
	}
	resetkoCancleTimer(fsm);
	removeResetEvent(module, RFSM_EVENT_All);

	mutex_lock(&fsm->notifyEventMutex);
	if (list_empty(&fsm->notifyEventList)) {
		mutex_unlock(&fsm->notifyEventMutex);
	} else {
		list_for_each_entry_safe(cur, next,
					 &fsm->notifyEventList, node) {
			list_del(&cur->node);
			kfree(cur);
		}
	}
	mutex_unlock(&fsm->notifyEventMutex);
	flush_delayed_work(&(fsm->notifyWork));

	MR_Info("[%s] %s, module type %d\n", fsm->name, __func__, module);
	removeResetFsm(module);
	mutex_unlock(&resetInfo.moduleMutex);

	return RESET_RETURN_STATUS_SUCCESS;
}
EXPORT_SYMBOL(resetko_unregister_module);


static int __init resetInit(void)
{
	MR_Info("%s\n", __func__);

	fgExit = false;
	fgIsPowerOff = false;

	mutex_init(&resetInfo.moduleMutex);
	spin_lock_init(&resetInfo.eventLock);
	INIT_LIST_HEAD(&resetInfo.moduleList);
	INIT_LIST_HEAD(&resetInfo.eventList);
	init_waitqueue_head(&resetInfo.resetko_waitq);
	INIT_DELAYED_WORK(&(resetInfo.resetWork), resetkoResetWork);
	register_reboot_notifier(&resetRebootNotifier);

	resetHif_Init();

	resetInfo.resetko_thread = kthread_run(resetko_thread_main,
					       NULL, "dongle_resetko_thread");

	return 0;
}

static void __exit resetExit(void)
{
	int i;

	unregister_reboot_notifier(&resetRebootNotifier);
	flush_delayed_work(&(resetInfo.resetWork));
	for (i = 0; i < RESET_MODULE_TYPE_MAX; i++)
		resetko_unregister_module((enum ModuleType)i);
	fgExit = true;
	resetHif_Uninit();

	MR_Info("%s\n", __func__);
}

module_init(resetInit);
module_exit(resetExit);

