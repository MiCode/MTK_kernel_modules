// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   reset_fsm.c
*   \brief  reset fsm
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
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/preempt.h>
#include <linux/version.h>

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


/**********************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
***********************************************************************
*/


/**********************************************************************
*                            P U B L I C   D A T A
***********************************************************************
*/


/**********************************************************************
*                           P R I V A T E   D A T A
***********************************************************************
*/


/**********************************************************************
*                              F U N C T I O N S
**********************************************************************/
struct FsmEntity *allocFsmEntity(uint32_t dongle_id,
				char *name, enum ModuleType eModuleType)
{
	struct FsmEntity *fsm;
#if CFG_RESETKO_ENABLE_WAKE_LOCK
	char wakeupSourceName[RFSM_NAME_MAX_LEN];
	int ret;
#endif
	if ((!name) ||
	    ((unsigned int)eModuleType >= RESET_MODULE_TYPE_MAX))
		return NULL;

	fsm = kmalloc(sizeof(struct FsmEntity),
		      in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
	if (!fsm)
		return NULL;
	fsm->name = kmalloc(RFSM_NAME_MAX_LEN,
			    in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
	if (fsm->name == NULL) {
		kfree(fsm);
		return NULL;
	}
#if KERNEL_VERSION(4, 3, 0) <= LINUX_VERSION_CODE
	strscpy(fsm->name, name, RFSM_NAME_MAX_LEN);
#else
	strncpy(fsm->name, name, RFSM_NAME_MAX_LEN);
	fsm->name[RFSM_NAME_MAX_LEN - 1] = 0;
#endif
	fsm->eModuleType = eModuleType;
	fsm->fgReady = false;
	fsm->notifyFunc = NULL;

	fsm->wakeupCount = 0;
#if CFG_RESETKO_ENABLE_WAKE_LOCK
	ret = snprintf(wakeupSourceName, RFSM_NAME_MAX_LEN,
			"resetko_%s_%d", name, dongle_id);
	if (ret > 0) {
		fsm->wakeupSource = wakeup_source_create(wakeupSourceName);
		if (!fsm->wakeupSource) {
			MR_Err("[%s_%d] fail to create wakeup resource(%s)\n",
				name, dongle_id, wakeupSourceName);
		} else {
			wakeup_source_add(fsm->wakeupSource);
			MR_Info("[%s_%d] create wakeup resource (%s)\n",
				name, dongle_id, wakeupSourceName);
		}
	}
#endif

	return fsm;
}

void freeFsmEntity(struct FsmEntity *fsm)
{
	if (!fsm)
		return;

#if CFG_RESETKO_ENABLE_WAKE_LOCK
	if (fsm->wakeupSource) {
		wakeup_source_remove(fsm->wakeupSource);
		wakeup_source_destroy(fsm->wakeupSource);
		fsm->wakeupSource = NULL;
	}
#endif
	fsm->wakeupCount = 0;

	if (fsm->name != NULL) {
		memset(fsm->name, 0, RFSM_NAME_MAX_LEN);
		kfree(fsm->name);
	}
	memset(fsm, 0, sizeof(struct FsmEntity));
	kfree(fsm);
}

void RFSM_handle_event(struct FsmEntity *fsm, unsigned int event)
{
	struct FsmState *currentFsmState;
	struct FsmState *nextFsmState;
	struct ResetFsmEventAction *currentEventAction;
	int i;

	if ((fsm == NULL) || (fsm->name == NULL) || (fsm->fsmState == NULL)) {
		MR_Err("fsm args error\n");
		return;
	}

	currentFsmState = fsm->fsmState;
	if ((currentFsmState->name == NULL) ||
	    (currentFsmState->eventActionListCount == 0) ||
	    (currentFsmState->eventActionList == NULL)) {
		MR_Err("[%s_%d] RFSM ignore event [%d]\n",
			fsm->name, fsm->dongle_id, event);
		return;
	}

	for (i = 0; i < currentFsmState->eventActionListCount; i++) {
		currentEventAction = &currentFsmState->eventActionList[i];
		if (event == currentEventAction->event) {
			if (currentEventAction->guard_func != NULL) {
				if (!currentEventAction->guard_func(fsm))
					continue;
			}
			if (currentEventAction->action_func != NULL)
				currentEventAction->action_func(fsm);

			nextFsmState = currentEventAction->nextState;
			if (nextFsmState != NULL &&
			    currentFsmState != nextFsmState) {
				if (currentFsmState->leave_func != NULL)
					currentFsmState->leave_func(fsm,
								nextFsmState);
				MR_Info("[%s_%d] state: [%s] -> [%s]",
					fsm->name, fsm->dongle_id,
					currentFsmState->name,
					nextFsmState->name);
				fsm->fsmState = nextFsmState;
				if (nextFsmState->enter_func != NULL)
					nextFsmState->enter_func(fsm,
							currentFsmState);
			}

			/* one event can only trigger one action */
			break;
		}
	}
}
