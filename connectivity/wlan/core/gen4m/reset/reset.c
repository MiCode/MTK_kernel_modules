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
#include <linux/of_device.h>
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
	uint32_t dongle_id;
	uint32_t bus_id;
	bool fgIsProbed;
	bool fgIsPowerOff;
	char moduleName[RESET_MODULE_TYPE_MAX][RFSM_NAME_MAX_LEN];

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
static void removeResetEvent(uint32_t dongle_id, enum ModuleType module,
			    enum ResetFsmEvent event);

#if CFG_RESETKO_SUPPORT_MULTI_CARD
static int resetko_probe(struct platform_device *pdev);
static int resetko_remove(struct platform_device *pdev);
static bool tryRegisterModule(uint32_t bus_id, enum ModuleType module,
				uint32_t *dongle_id);
#endif
/**********************************************************************
*                            P U B L I C   D A T A
***********************************************************************
*/

/**********************************************************************
*                           P R I V A T E   D A T A
***********************************************************************
*/
#if CFG_RESETKO_SUPPORT_MULTI_CARD
static char moduleName[RESET_MODULE_TYPE_MAX][RFSM_NAME_MAX_LEN];
static void *moduleNotifyFunc[RESET_MODULE_TYPE_MAX];

#ifdef CONFIG_OF
const struct of_device_id mtk_resetko_of_ids[] = {
	{.compatible = "mediatek,resetko",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_resetko_of_ids);
#endif  /* CONFIG_OF */

static struct platform_driver mtk_resetko_driver = {
	.driver = {
		.name = "resetko",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mtk_resetko_of_ids,
#endif
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
	},
	.probe = resetko_probe,
	.remove = resetko_remove,
};
#endif  /* CFG_RESETKO_SUPPORT_MULTI_CARD */

static struct ResetInfo resetInfo[MAX_DONGLE_NUM];
static bool fgExit;
static struct notifier_block resetRebootNotifier = {
	.notifier_call = reset_reboot_notify,
	.next = NULL,
	.priority = 0,
};

/**********************************************************************
*                              F U N C T I O N S
**********************************************************************/
bool findBusIdByDongleId(uint32_t dongle_id, uint32_t *bus_id)
{
	int i;

	if (!bus_id)
		return false;

	for (i = 0; i < MAX_DONGLE_NUM; i++) {
		if (resetInfo[i].fgIsProbed &&
		    (resetInfo[i].dongle_id == dongle_id)) {
			*bus_id = resetInfo[i].bus_id;
			return true;
		}
	}

	return false;
}

static struct FsmEntity *findResetFsm(uint32_t dongle_id,
				      enum ModuleType module)
{
	struct FsmEntity *fsm, *next_fsm;

	list_for_each_entry_safe(fsm, next_fsm,
				 &resetInfo[dongle_id].moduleList, node) {
		if (fsm->eModuleType == module)
			return fsm;
	}

	return NULL;
}

static void removeResetFsm(uint32_t dongle_id, enum ModuleType module)
{
	struct FsmEntity *fsm, *next_fsm;

	list_for_each_entry_safe(fsm, next_fsm,
				 &resetInfo[dongle_id].moduleList, node) {
		if (fsm->eModuleType == module) {
			list_del(&fsm->node);
			freeResetFsm(fsm);
		}
	}
}

static void addResetFsm(uint32_t dongle_id, struct FsmEntity *fsm)
{
	if (!fsm)
		return;

	list_add_tail(&fsm->node, &resetInfo[dongle_id].moduleList);
}

static struct ResetEvent *allocResetEvent(void)
{
	return kmalloc(sizeof(struct ResetEvent),
		       in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
}

static void freeResetEvent(struct ResetEvent *event)
{
	if (event)
		kfree(event);
}

static bool isEventEmpty(uint32_t dongle_id)
{
	unsigned long flags;
	bool ret;

	spin_lock_irqsave(&resetInfo[dongle_id].eventLock, flags);
	if (list_empty(&resetInfo[dongle_id].eventList))
		ret = true;
	else
		ret = false;
	spin_unlock_irqrestore(&resetInfo[dongle_id].eventLock, flags);

	return ret;
}

static void pushResetEvent(uint32_t dongle_id, struct ResetEvent *event)
{
	unsigned long flags;

	if (!event ||
	    ((unsigned int)event->module >= RESET_MODULE_TYPE_MAX) ||
	    ((unsigned int)event->event >= RFSM_EVENT_MAX)) {
		MR_Err("%s: argument error\n", __func__);
		freeResetEvent(event);
		return;
	}

	spin_lock_irqsave(&resetInfo[dongle_id].eventLock, flags);
	list_add_tail(&event->node, &resetInfo[dongle_id].eventList);
	spin_unlock_irqrestore(&resetInfo[dongle_id].eventLock, flags);
}

static struct ResetEvent *popResetEvent(uint32_t dongle_id)
{
	struct ResetEvent *event = NULL;
	unsigned long flags;

	spin_lock_irqsave(&resetInfo[dongle_id].eventLock, flags);
	if (list_empty(&resetInfo[dongle_id].eventList))
		goto POP_EVT_RETURN;
	event = list_first_entry(&resetInfo[dongle_id].eventList,
				 struct ResetEvent, node);
	list_del(&event->node);

POP_EVT_RETURN:
	spin_unlock_irqrestore(&resetInfo[dongle_id].eventLock, flags);
	return event;
}

static void removeResetEvent(uint32_t dongle_id, enum ModuleType module,
			    enum ResetFsmEvent event)
{
	struct ResetEvent *cur, *next;
	unsigned long flags;

	spin_lock_irqsave(&resetInfo[dongle_id].eventLock, flags);
	if (list_empty(&resetInfo[dongle_id].eventList))
		goto REMOVE_EVT_RETURN;

	list_for_each_entry_safe(cur, next,
				 &resetInfo[dongle_id].eventList, node) {
		if ((cur->module == module) &&
		    ((cur->event == event) || (event == RFSM_EVENT_All))) {
			list_del(&cur->node);
			freeResetEvent(cur);
		}
	}
REMOVE_EVT_RETURN:
	spin_unlock_irqrestore(&resetInfo[dongle_id].eventLock, flags);
}

static int reset_reboot_notify(struct notifier_block *nb,
				unsigned long event, void *unused)
{
	uint32_t dongle_id;
	enum ModuleType module;
	(void)nb;
	(void)unused;

	if (event == SYS_RESTART ||
	    event == SYS_POWER_OFF ||
	    event == SYS_HALT) {
		fgExit = true;
		for (dongle_id = 0; dongle_id < MAX_DONGLE_NUM; dongle_id++) {
			if (!resetInfo[dongle_id].fgIsProbed)
				continue;
			mutex_lock(&resetInfo[dongle_id].moduleMutex);
			for (module = RESET_MODULE_TYPE_WIFI;
			     module < RESET_MODULE_TYPE_MAX;
			     module++) {
				removeResetEvent(dongle_id, module,
						 RFSM_EVENT_All);
				wakeupSourceRelax(findResetFsm(dongle_id,
								module));
			}
			mutex_unlock(&resetInfo[dongle_id].moduleMutex);
		}
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
	struct ResetInfo *resetInfo = (struct ResetInfo *)data;

	if (!resetInfo)
		return 0;

	MR_Info("[%d] %s: resetko thread for dongle on bus[%d] start\n",
		resetInfo->dongle_id, __func__, resetInfo->bus_id);

	while (!fgExit) {
		do {
			ret = wait_event_interruptible(resetInfo->resetko_waitq,
				   isEventEmpty(resetInfo->dongle_id) == false);
		} while (ret != 0);

		while (resetEvent = popResetEvent(resetInfo->dongle_id),
		       resetEvent != NULL) {
			evt = (unsigned int)resetEvent->event;
			if (evt > RFSM_EVENT_MAX) {
				freeResetEvent(resetEvent);
				continue;
			}
			mutex_lock(&resetInfo->moduleMutex);
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
				fsm = findResetFsm(resetInfo->dongle_id,
						   module);
				if (fsm != NULL) {
					if (evt == RFSM_EVENT_READY)
						fsm->fgReady = ~false;
					MR_Info(
					     "[%s_%d] in [%s] state rcv [%s]\n",
						fsm->name, fsm->dongle_id,
						fsm->fsmState->name,
						eventName[evt]);
					resetFsmHandlevent(fsm, evt);
				}
			}
			mutex_unlock(&resetInfo->moduleMutex);
			freeResetEvent(resetEvent);
		}
	}

	MR_Info("[%d] %s: resetko thread for dongle on bus[%d] stop\n",
		resetInfo->dongle_id, __func__, resetInfo->bus_id);

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
	uint32_t bus_id = 0;

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

		MR_Info("[%s_%d] %s %d\n",
			fsm->name, fsm->dongle_id, __func__, prEvent->event);
#if CFG_RESETKO_SUPPORT_MULTI_CARD
		if (!findBusIdByDongleId(fsm->dongle_id, &bus_id))
			MR_Err("[%s_%d] can't find bus_id\n",
				fsm->name, fsm->dongle_id);
		else
#endif
			fsm->notifyFunc((unsigned int)prEvent->event, &bus_id);

		kfree(prEvent);
	}
}

void clearAllModuleReady(uint32_t dongle_id)
{
	struct FsmEntity *fsm, *next_fsm;

	/* mutex is hold in function resetko_thread_main */
	list_for_each_entry_safe(fsm, next_fsm,
				 &resetInfo[dongle_id].moduleList, node) {
		fsm->fgReady = false;
	}
}

bool isAllModuleReady(uint32_t dongle_id)
{
	struct FsmEntity *fsm, *next_fsm;
	bool ret = ~false;

	/* mutex is hold in function resetko_thread_main */
	list_for_each_entry_safe(fsm, next_fsm,
				 &resetInfo[dongle_id].moduleList, node) {
		MR_Info("[%s_%d] %s: %s\n", fsm->name, fsm->dongle_id, __func__,
			fsm->fgReady ? "ready" : "not ready");
		if (!fsm->fgReady)
			ret = false;
	}

	return ret;
}

bool isAllModuleInState(uint32_t dongle_id, const struct FsmState *state)
{
	struct FsmEntity *fsm, *next_fsm;
	bool ret = ~false;

	/* mutex is hold in function resetko_thread_main */
	list_for_each_entry_safe(fsm, next_fsm,
				 &resetInfo[dongle_id].moduleList, node) {
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
			MR_Warn("[%s_%d] %s\n",
				fsm->name, fsm->dongle_id, __func__);
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
			MR_Warn("[%s_%d] %s\n",
				fsm->name, fsm->dongle_id, __func__);
			__pm_relax(fsm->wakeupSource);
		}
#endif
	}
}

void powerOff(uint32_t dongle_id)
{
	if (resetInfo[dongle_id].fgIsPowerOff)
		return;
	/* powerOff by hif type */
	/* 1. host func remove
	 * 2. pull reset pin
	 * 3. power off
	 */
#if (CFG_RESETKO_SUPPORT_MULTI_CARD == 0)
	resetHif_SdioRemoveHost();
#endif
	resetHif_ResetGpioPull(dongle_id);
	resetHif_PowerGpioSwitchOff(dongle_id);

	resetInfo[dongle_id].fgIsPowerOff = true;
}

void powerOn(uint32_t dongle_id)
{
	if (!resetInfo[dongle_id].fgIsPowerOff)
		return;

	/* powerON by hif type */
	/* 1. power on
	 * 2. release reset pin
	 * 3. host func add
	 */
	resetHif_PowerGpioSwitchOn(dongle_id);
	resetHif_ResetGpioRelease(dongle_id);
#if (CFG_RESETKO_SUPPORT_MULTI_CARD == 0)
	resetHif_SdioAddHost();
#endif

	resetInfo[dongle_id].fgIsPowerOff = false;
}

void powerReset(uint32_t dongle_id)
{
	schedule_delayed_work(&resetInfo[dongle_id].resetWork, 0);
}

void resetkoResetWork(struct work_struct *work)
{
	enum ModuleType module;
	struct delayed_work *delay_work;
	struct ResetInfo *resetInfo;
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	uint32_t bus_id;
#endif

	delay_work = to_delayed_work(work);
	resetInfo = container_of(delay_work, struct ResetInfo, resetWork);

	powerOff(resetInfo->dongle_id);
	msleep(50);
	powerOn(resetInfo->dongle_id);

	/* reset done, all timer and event need clear */
	mutex_lock(&resetInfo->moduleMutex);
	for (module = RESET_MODULE_TYPE_WIFI;
	     module < RESET_MODULE_TYPE_MAX;
	     module++) {
		resetkoCancleTimer(findResetFsm(resetInfo->dongle_id, module));
		removeResetEvent(resetInfo->dongle_id, module, RFSM_EVENT_All);
	}
	mutex_unlock(&resetInfo->moduleMutex);

#if CFG_RESETKO_SUPPORT_MULTI_CARD
	if (findBusIdByDongleId(resetInfo->dongle_id, &bus_id))
		send_reset_event(bus_id,
				 RESET_MODULE_TYPE_WIFI, RFSM_EVENT_RESET_DONE);
	else
		MR_Err("[%d] can't find bus_id\n", resetInfo->dongle_id);
#else
	send_reset_event(RESET_MODULE_TYPE_WIFI, RFSM_EVENT_RESET_DONE);
#endif
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
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	uint32_t bus_id;
#endif

	if (!fsm) {
		MR_Err("%s: fsm is null\n", __func__);
		return;
	}

	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	if (findBusIdByDongleId(resetInfo->dongle_id, &bus_id))
		send_reset_event(bus_id, fsm->eModuleType, RFSM_EVENT_TIMEOUT);
	else
		MR_Err("[%d] can't find bus_id\n", resetInfo->dongle_id);
#else
	send_reset_event(fsm->eModuleType, RFSM_EVENT_TIMEOUT);
#endif
}

void resetkoStartTimer(struct FsmEntity *fsm, unsigned int ms)
{
#if RESETKO_SUPPORT_WAIT_TIMEOUT
	if (!fsm) {
		MR_Err("%s: fsm is null\n", __func__);
		return;
	}
	MR_Info("[%s_%d] %s %dms\n", fsm->name, fsm->dongle_id, __func__, ms);
	if (ms == 0)
		return;

	mod_timer(&fsm->resetTimer, jiffies + ms * HZ / MSEC_PER_SEC);
#endif
}

void resetkoCancleTimer(struct FsmEntity *fsm)
{
#if RESETKO_SUPPORT_WAIT_TIMEOUT
	if (!fsm)
		return;
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);

	del_timer(&fsm->resetTimer);
	removeResetEvent(fsm->dongle_id, fsm->eModuleType, RFSM_EVENT_TIMEOUT);
#endif
}

static enum ReturnStatus _send_reset_event(uint32_t dongle_id,
					   enum ModuleType module,
					   enum ResetFsmEvent event)
{
	struct ResetEvent *resetEvent;

	if (fgExit)
		return RESET_RETURN_STATUS_FAIL;

	if ((dongle_id >= MAX_DONGLE_NUM) ||
	    ((unsigned int)module >= RESET_MODULE_TYPE_MAX) ||
	    ((unsigned int)event >= RFSM_EVENT_MAX)) {
		MR_Err("%s: args error %d %d %d\n",
			__func__, dongle_id, module, event);
		return RESET_RETURN_STATUS_FAIL;
	}

	if (event == RFSM_EVENT_TRIGGER_RESET)
		dump_stack();

	resetEvent = allocResetEvent();
	if (!resetEvent) {
		MR_Err("[%s_%d] %s: allocResetEvent fail\n",
			resetInfo[dongle_id].moduleName[module], dongle_id,
			__func__);
		return RESET_RETURN_STATUS_FAIL;
	}

	MR_Info("[%s_%d] %s %s\n",
		resetInfo[dongle_id].moduleName[module], dongle_id,
		__func__, eventName[event]);
	resetEvent->module = module;
	resetEvent->event = event;
	pushResetEvent(dongle_id, resetEvent);

	wake_up_interruptible(&resetInfo[dongle_id].resetko_waitq);

	return RESET_RETURN_STATUS_SUCCESS;
}

#if CFG_RESETKO_SUPPORT_MULTI_CARD
enum ReturnStatus send_reset_event(uint32_t bus_id, enum ModuleType module,
				 enum ResetFsmEvent event)
{
	uint32_t dongle_id = 0;

	if (tryRegisterModule(bus_id, module, &dongle_id))
		return _send_reset_event(dongle_id, module, event);

	MR_Err("%s: Failed to find dongle_id for bus_id %d\n",
		__func__, bus_id);
	return RESET_RETURN_STATUS_FAIL;
}
#else
enum ReturnStatus send_reset_event(enum ModuleType module,
				 enum ResetFsmEvent event)
{
	return _send_reset_event(0, module, event);
}
#endif
EXPORT_SYMBOL(send_reset_event);

enum ReturnStatus _send_msg_to_module(uint32_t dongle_id,
				    enum ModuleType srcModule,
				    enum ModuleType dstModule,
				    struct ModuleMsg *msg)
{
	struct FsmEntity *srcfsm, *dstfsm;

	if (fgExit)
		return RESET_RETURN_STATUS_FAIL;

	if (!msg)
		MR_Err("%s: %d -> %d, msg is NULL\n",
			__func__, srcModule, dstModule);

	mutex_lock(&resetInfo[dongle_id].moduleMutex);
	srcfsm = findResetFsm(dongle_id, srcModule);
	if (!srcfsm) {
		MR_Err("%s: src module (%d) not exist\n",
			__func__, srcModule);
		goto SEND_MSG_FAIL;
	}
	dstfsm = findResetFsm(dongle_id, dstModule);
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
		dstfsm->notifyFunc((unsigned int)MODULE_NOTIFY_MESSAGE,
				   (void *)msg);
	}
	mutex_unlock(&resetInfo[dongle_id].moduleMutex);
	return RESET_RETURN_STATUS_SUCCESS;

SEND_MSG_FAIL:
	mutex_unlock(&resetInfo[dongle_id].moduleMutex);
	return RESET_RETURN_STATUS_FAIL;
}
#if CFG_RESETKO_SUPPORT_MULTI_CARD
enum ReturnStatus send_msg_to_module(uint32_t bus_id, enum ModuleType srcModule,
				    enum ModuleType dstModule,
				    struct ModuleMsg *msg)
{
	uint32_t dongle_id = 0;

	if (!msg)
		return RESET_RETURN_STATUS_FAIL;
	msg->bus_id = bus_id;

	if (tryRegisterModule(bus_id, srcModule, &dongle_id))
		return _send_msg_to_module(dongle_id, srcModule,
					   dstModule, msg);

	MR_Err("%s: Failed to find dongle_id for bus_id %d\n",
		__func__, bus_id);
	return RESET_RETURN_STATUS_FAIL;
}
#else
enum ReturnStatus send_msg_to_module(enum ModuleType srcModule,
				    enum ModuleType dstModule,
				    struct ModuleMsg *msg)
{
	return _send_msg_to_module(0, srcModule, dstModule, msg);
}
#endif
EXPORT_SYMBOL(send_msg_to_module);

#if CFG_RESETKO_SUPPORT_MULTI_CARD
enum ReturnStatus update_hif_info(uint32_t bus_id,
				  enum HifInfoType type, void *info)
{
	MR_Err("%s: resetko multi donlge only support usb hif\n", __func__);
	return RESET_RETURN_STATUS_FAIL;
}
#else
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
#endif
EXPORT_SYMBOL(update_hif_info);

#if CFG_RESETKO_SUPPORT_MULTI_CARD
static int resetko_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	uint32_t dongle_id;
	uint32_t bus_id;

	of_property_read_u32(dev->of_node, "bus_id", &bus_id);
	for (dongle_id = 0; dongle_id < MAX_DONGLE_NUM; dongle_id++) {
		if (resetInfo[dongle_id].fgIsProbed == false) {
			resetInfo[dongle_id].fgIsProbed = true;
			resetInfo[dongle_id].bus_id = bus_id;
			resetInfo[dongle_id].dongle_id = dongle_id;
			break;
		}
	}
	MR_Info("[%d] resetko probe function called, bus_id = %d\n",
		dongle_id, bus_id);
	if (dongle_id >= MAX_DONGLE_NUM) {
		MR_Err("MAX_DONGLE_NUM is %d\n", MAX_DONGLE_NUM);
		return -1;
	}

	resetHif_Init(dongle_id, dev->of_node);
	resetInfo[dongle_id].fgIsPowerOff = false;

	mutex_init(&resetInfo[dongle_id].moduleMutex);
	spin_lock_init(&resetInfo[dongle_id].eventLock);
	INIT_LIST_HEAD(&resetInfo[dongle_id].moduleList);
	INIT_LIST_HEAD(&resetInfo[dongle_id].eventList);
	init_waitqueue_head(&resetInfo[dongle_id].resetko_waitq);
	INIT_DELAYED_WORK(&(resetInfo[dongle_id].resetWork), resetkoResetWork);

	resetInfo[dongle_id].resetko_thread = kthread_run(resetko_thread_main,
						  &resetInfo[dongle_id],
						  "dongle%d_resetko_thread",
						  dongle_id);

	return 0;
}

static int resetko_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	uint32_t dongle_id;
	uint32_t bus_id;
	int i;

	of_property_read_u32(dev->of_node, "bus_id", &bus_id);
	for (dongle_id = 0; dongle_id < MAX_DONGLE_NUM; dongle_id++) {
		if (resetInfo[dongle_id].bus_id == bus_id) {
			MR_Info(
			   "[%d] resetko remove function called, bus_id = %d\n",
			   dongle_id, bus_id);
			resetInfo[dongle_id].fgIsProbed = false;
			flush_delayed_work(&(resetInfo[dongle_id].resetWork));
			for (i = 0; i < RESET_MODULE_TYPE_MAX; i++)
				resetko_unregister_module((enum ModuleType)i);
			resetHif_Uninit(dongle_id);
			break;
		}
	}

	return 0;
}
#endif

enum ReturnStatus _resetko_register_module(uint32_t dongle_id,
					enum ModuleType module,
					char *name,
					void *notifyFunc)
{
	struct FsmEntity *fsm;

	if (fgExit)
		return RESET_RETURN_STATUS_FAIL;

	if (!name) {
		MR_Err(
		    "%s: insmod module(%d) with no name\n",
		    __func__, module);
		return RESET_RETURN_STATUS_FAIL;
	}
	fsm = allocResetFsm(dongle_id, name, module);
	if (!fsm) {
		MR_Err(
		     "%s: allocResetFsm module(%d) fail\n",
		     __func__, module);
		return RESET_RETURN_STATUS_FAIL;
	}
	fsm->dongle_id = dongle_id;
	fsm->notifyFunc = (NotifyFunc)notifyFunc;
	mutex_init(&(fsm->notifyEventMutex));
	INIT_LIST_HEAD(&(fsm->notifyEventList));
	INIT_DELAYED_WORK(&(fsm->notifyWork), resetkoNotifyWork);

	mutex_lock(&resetInfo[dongle_id].moduleMutex);
	if (findResetFsm(dongle_id, module) != NULL) {
		mutex_unlock(&resetInfo[dongle_id].moduleMutex);
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
	memcpy(&resetInfo[dongle_id].moduleName[module][0],
		fsm->name, RFSM_NAME_MAX_LEN);
	addResetFsm(dongle_id, fsm);
	mutex_unlock(&resetInfo[dongle_id].moduleMutex);

	MR_Info("[%s_%d] %s, module type %d\n",
		name, dongle_id, __func__, module);

	return RESET_RETURN_STATUS_SUCCESS;
}

#if CFG_RESETKO_SUPPORT_MULTI_CARD
static bool tryRegisterModule(uint32_t bus_id, enum ModuleType module,
				uint32_t *dongle_id)
{
	int i;

	for (i = 0; i < MAX_DONGLE_NUM; i++) {
		if ((resetInfo[i].bus_id == bus_id) &&
		    resetInfo[i].fgIsProbed) {
			if (findResetFsm(i, module) ||
			    (RESET_RETURN_STATUS_SUCCESS ==
					_resetko_register_module(i, module,
						moduleName[module],
						moduleNotifyFunc[module]))) {
				*dongle_id = i;
				return true;
			}
			break;
		}
	}
	MR_Err("can not find or register dongle for bus %d\n", bus_id);
	return false;
}

enum ReturnStatus resetko_register_module(enum ModuleType module,
					char *name,
					enum TriggerResetApiType resetApiType,
					void *resetFunc,
					void *notifyFunc)
{
#if KERNEL_VERSION(4, 3, 0) <= LINUX_VERSION_CODE
	strscpy(&moduleName[module][0], name, RFSM_NAME_MAX_LEN);
#else
	strncpy(&moduleName[module][0], name, RFSM_NAME_MAX_LEN);
	moduleName[module][RFSM_NAME_MAX_LEN - 1] = 0;
#endif
	moduleNotifyFunc[module] = notifyFunc;

	return RESET_RETURN_STATUS_SUCCESS;
}
#else
enum ReturnStatus resetko_register_module(enum ModuleType module,
					char *name,
					enum TriggerResetApiType resetApiType,
					void *resetFunc,
					void *notifyFunc)
{
	return _resetko_register_module(0, module, name, notifyFunc);
}
#endif
EXPORT_SYMBOL(resetko_register_module);


enum ReturnStatus _resetko_unregister_module(uint32_t dongle_id,
					     enum ModuleType module)
{
	struct FsmEntity *fsm;
	struct NotifyEvent *cur, *next;

	mutex_lock(&resetInfo[dongle_id].moduleMutex);
	fsm = findResetFsm(dongle_id, module);
	if (!fsm) {
		mutex_unlock(&resetInfo[dongle_id].moduleMutex);
		return RESET_RETURN_STATUS_SUCCESS;
	}
	resetkoCancleTimer(fsm);
	removeResetEvent(dongle_id, module, RFSM_EVENT_All);

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

	MR_Info("[%s_%d] %s, module type %d\n",
		fsm->name, fsm->dongle_id, __func__, module);
	removeResetFsm(dongle_id, module);
	mutex_unlock(&resetInfo[dongle_id].moduleMutex);

	return RESET_RETURN_STATUS_SUCCESS;
}

#if CFG_RESETKO_SUPPORT_MULTI_CARD
enum ReturnStatus resetko_unregister_module(enum ModuleType module)
{
	int i;

	for (i = 0; i < MAX_DONGLE_NUM; i++) {
		if (resetInfo[i].fgIsProbed)
			_resetko_unregister_module(i, module);
	}
	return RESET_RETURN_STATUS_SUCCESS;
}
#else
enum ReturnStatus resetko_unregister_module(enum ModuleType module)
{
	return _resetko_unregister_module(0, module);
}
#endif
EXPORT_SYMBOL(resetko_unregister_module);


static int __init resetInit(void)
{
	MR_Info("%s\n", __func__);

	fgExit = false;

#if CFG_RESETKO_SUPPORT_MULTI_CARD
	if (platform_driver_register(&mtk_resetko_driver)) {
		MR_Err("platform_driver_register fail\n");
		return -1;
	}
#else
	resetInfo[0].fgIsPowerOff = false;
	mutex_init(&resetInfo[0].moduleMutex);
	spin_lock_init(&resetInfo[0].eventLock);
	INIT_LIST_HEAD(&resetInfo[0].moduleList);
	INIT_LIST_HEAD(&resetInfo[0].eventList);
	init_waitqueue_head(&resetInfo[0].resetko_waitq);
	INIT_DELAYED_WORK(&(resetInfo[0].resetWork), resetkoResetWork);
	resetInfo[0].resetko_thread = kthread_run(resetko_thread_main,
						  &resetInfo[0],
						  "dongle_resetko_thread");
	resetHif_Init(0, NULL);
#endif
	register_reboot_notifier(&resetRebootNotifier);

	return 0;
}

static void __exit resetExit(void)
{
#if (CFG_RESETKO_SUPPORT_MULTI_CARD == 0)
	int i;
#endif
	unregister_reboot_notifier(&resetRebootNotifier);

#if CFG_RESETKO_SUPPORT_MULTI_CARD
	platform_driver_unregister(&mtk_resetko_driver);
#else
	flush_delayed_work(&(resetInfo[0].resetWork));
	for (i = 0; i < RESET_MODULE_TYPE_MAX; i++)
		resetko_unregister_module((enum ModuleType)i);

	resetHif_Uninit(0);
#endif
	fgExit = true;

	MR_Info("%s\n", __func__);
}

module_init(resetInit);
module_exit(resetExit);

