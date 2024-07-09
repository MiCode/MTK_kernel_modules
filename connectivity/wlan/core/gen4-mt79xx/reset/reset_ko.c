/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

/*! \file   reset_ko.c
*   \brief  reset ko
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
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>

#include "reset_ko.h"
#include "reset.h"


/**********************************************************************
*                                 M A C R O S
***********************************************************************
*/


/**********************************************************************
*                              C O N S T A N T S
***********************************************************************
*/


/**********************************************************************
*                             D A T A   T Y P E S
***********************************************************************
*/
struct ResetInfo {
	int fgInited;

	wait_queue_head_t resetko_waitq;
	struct task_struct *resetko_thread;

	struct mutex moduleMutex;
	struct mutex eventMutex;
	struct list_head moduleList;
	struct list_head eventList;
};

struct ResetEvent {
	struct list_head node;
	enum ModuleType module;
	enum ResetFsmEvent event;
};


/**********************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
***********************************************************************
*/
static void rstHandleEventWork(struct work_struct *work);

/**********************************************************************
*                            P U B L I C   D A T A
***********************************************************************
*/


/**********************************************************************
*                           P R I V A T E   D A T A
***********************************************************************
*/
struct ResetInfo resetInfo = {0};

static bool fgL0ResetDone;

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
	return kmalloc(sizeof(struct ResetEvent), GFP_KERNEL);
}

static void freeResetEvent(struct ResetEvent *event)
{
	if (!event)
		kfree(event);
}

static bool isEventEmpty(void)
{
	mutex_lock(&resetInfo.eventMutex);
	if (list_empty(&resetInfo.eventList)) {
		mutex_unlock(&resetInfo.eventMutex);
		return true;
	}
	mutex_unlock(&resetInfo.eventMutex);
	return false;
}

static void pushResetEvent(struct ResetEvent *event)
{
	if (!event ||
	    ((unsigned int)event->module >= RESET_MODULE_TYPE_MAX) ||
	    ((unsigned int)event->event >= RFSM_EVENT_MAX))
		return;

	mutex_lock(&resetInfo.eventMutex);
	list_add_tail(&event->node, &resetInfo.eventList);
	mutex_unlock(&resetInfo.eventMutex);
}

static struct ResetEvent *popResetEvent(void)
{
	struct ResetEvent *event;

	mutex_lock(&resetInfo.eventMutex);
	if (list_empty(&resetInfo.eventList)) {
		mutex_unlock(&resetInfo.eventMutex);
		return NULL;
	}
	event = list_first_entry(&resetInfo.eventList, struct ResetEvent, node);
	list_del(&event->node);
	mutex_unlock(&resetInfo.eventMutex);
	return event;
}

static int resetko_thread_main(void *data)
{
	struct FsmEntity *fsm;
	struct ResetEvent *resetEvent;
	enum ModuleType module, begin, end;
	int ret = 0;

	while (true) {
		do {
			ret = wait_event_interruptible(resetInfo.resetko_waitq,
						       isEventEmpty() == false);
		} while (ret != 0);

		while (resetEvent = popResetEvent(), resetEvent != NULL) {
			mutex_lock(&resetInfo.moduleMutex);
			/* loop for all related module */
			if ((resetEvent->event == RFSM_EVENT_TRIGGER_RESET) ||
			    (resetEvent->event == RFSM_EVENT_L0_RESET_DONE)) {
				begin = 0;
				end = RESET_MODULE_TYPE_MAX - 1;
			} else {
				begin = resetEvent->module;
				end = resetEvent->module;
			}
			for (module = begin; module <= end; module++) {
				fsm = findResetFsm(module);
				if (fsm != NULL) {
					if (resetEvent->event ==
					    RFSM_EVENT_L0_RESET_READY)
						fsm->fgReadyForReset = ~false;
					resetFsmHandlevent(fsm,
							   resetEvent->event);
				}
			}
			mutex_unlock(&resetInfo.moduleMutex);
		};
	}

	return 0;
}

void resetkoNotifyEvent(struct FsmEntity *fsm, enum ModuleNotifyEvent event)
{
	if (!fsm) {
		MR_Info("%s: fsm is NULL\n", __func__);
		return;
	}
	if (fsm->notifyFunc != NULL) {
		MR_Info("%s: notify event %d to %s module\n",
			__func__, event, fsm->name);
		fsm->notifyFunc((unsigned int)event, NULL);
	}
}

void clearAllModuleReadyForReset(void)
{
	struct FsmEntity *fsm, *next_fsm;

	/* mutex is hold in function resetko_thread_main */
	list_for_each_entry_safe(fsm, next_fsm, &resetInfo.moduleList, node) {
		fsm->fgReadyForReset = false;
	}
}

bool isAllModuleReadyForReset(void)
{
	struct FsmEntity *fsm, *next_fsm;
	bool ret = ~false;

	/* mutex is hold in function resetko_thread_main */
	list_for_each_entry_safe(fsm, next_fsm, &resetInfo.moduleList, node) {
		MR_Info("%s: module %s is %s\n", __func__, fsm->name,
			fsm->fgReadyForReset ? "ready" : "not ready");
		if (!fsm->fgReadyForReset)
			ret = false;
	}

	/* set flag for L0ResetDone when all module is ready for reset */
	if (ret)
		fgL0ResetDone = false;

	return ret;
}

void callResetFuncByResetApiType(struct FsmEntity *fsm)
{
	struct FsmEntity *cur_fsm, *next_fsm;
	unsigned int i;

	if (fgL0ResetDone)
		return;

	for (i = TRIGGER_RESET_TYPE_UNSUPPORT; i < TRIGGER_RESET_API_TYPE_MAX;
	     i++) {
		MR_Info("%s: try to find resetFunc by resetApiType %d\n",
			__func__, i);
		list_for_each_entry_safe(cur_fsm, next_fsm,
					 &resetInfo.moduleList, node) {
			if (cur_fsm->resetApiType ==
			    TRIGGER_RESET_TYPE_UNSUPPORT){
				MR_Err("%s: %s module don't support reset\n",
					__func__, cur_fsm->name);
				fgL0ResetDone = true;
				return;
			}
			if ((cur_fsm->resetApiType == i) &&
			    (cur_fsm->resetFunc != NULL)) {
				MR_Info("%s: call resetFunc in %s module\n",
					__func__, cur_fsm->name);
				cur_fsm->resetFunc();
				break;
			}
		}
	}
	fgL0ResetDone = true;

	/* internal send reset done event */
	send_reset_event(fsm->eModuleType, RFSM_EVENT_L0_RESET_DONE);
}

enum ReturnStatus send_reset_event(enum ModuleType module,
				 enum ResetFsmEvent event)
{
	struct ResetEvent *resetEvent;

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

	MR_Info("%s: module %d send event %d\n", __func__, module, event);
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


enum ReturnStatus resetko_register_module(enum ModuleType module,
					char *name,
					enum TriggerResetApiType resetApiType,
					void *resetFunc,
					void *notifyFunc)
{
	struct FsmEntity *fsm;

	if (!name) {
		MR_Err("%s: insmod module(%d) with no name\n",
			__func__, module);
		return RESET_RETURN_STATUS_FAIL;
	}
	fsm = allocResetFsm(name, module, resetApiType);
	if (!fsm) {
		MR_Err("%s: allocResetFsm module(%d) fail\n", __func__, module);
		return RESET_RETURN_STATUS_FAIL;
	}
	fsm->notifyFunc = (NotifyFunc)notifyFunc;
	fsm->resetFunc = (ResetFunc)resetFunc;

	mutex_lock(&resetInfo.moduleMutex);
	if (findResetFsm(module) != NULL) {
		mutex_unlock(&resetInfo.moduleMutex);
		MR_Err("%s: insmod module(%d) existed\n",
			__func__, module);
		return RESET_RETURN_STATUS_FAIL;
	}
	addResetFsm(fsm);
	mutex_unlock(&resetInfo.moduleMutex);

	return RESET_RETURN_STATUS_SUCCESS;
}
EXPORT_SYMBOL(resetko_register_module);


enum ReturnStatus resetko_unregister_module(enum ModuleType module)
{
	struct FsmEntity *fsm;

	mutex_lock(&resetInfo.moduleMutex);
	fsm = findResetFsm(module);
	if (!fsm) {
		mutex_unlock(&resetInfo.moduleMutex);
		MR_Err("%s: rmmod module(%d) not exist\n",
			__func__, module);
		return RESET_RETURN_STATUS_SUCCESS;
	}
	removeResetFsm(module);
	mutex_unlock(&resetInfo.moduleMutex);

	return RESET_RETURN_STATUS_SUCCESS;
}
EXPORT_SYMBOL(resetko_unregister_module);


int resetKoInit(void)
{
	MR_Err("%s\n", __func__);

	if (resetInfo.fgInited != 0) {
		MR_Err("%s: reset ko already inited\n", __func__);
		return -1;
	}
	resetInfo.fgInited = 1;
	fgL0ResetDone = false;

	mutex_init(&resetInfo.moduleMutex);
	mutex_init(&resetInfo.eventMutex);
	INIT_LIST_HEAD(&resetInfo.moduleList);
	INIT_LIST_HEAD(&resetInfo.eventList);
	init_waitqueue_head(&resetInfo.resetko_waitq);
	resetInfo.resetko_thread = kthread_run(resetko_thread_main,
						NULL, "resetko_thread");

	return 0;
}


void resetKoExit(void)
{
	resetInfo.fgInited = 0;
	MR_Dbg("%s\n", __func__);
}
