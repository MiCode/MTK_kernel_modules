/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2022 MediaTek Inc.
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
#include "reset_fsm.h"

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
struct FsmEntity *allocFsmEntity(char *name,
				enum ModuleType eModuleType,
				enum TriggerResetApiType resetApiType)
{
	struct FsmEntity *fsm;

	if ((!name) ||
	    ((unsigned int)eModuleType >= RESET_MODULE_TYPE_MAX) ||
	    ((unsigned int)resetApiType >= TRIGGER_RESET_API_TYPE_MAX))
		return NULL;

	fsm = kmalloc(sizeof(struct FsmEntity), GFP_KERNEL);
	if (!fsm)
		return NULL;
	fsm->name = kmalloc(RFSM_NAME_MAX_LEN, GFP_KERNEL);
	if (fsm->name == NULL) {
		kfree(fsm);
		return NULL;
	}
	strncpy(fsm->name, name, RFSM_NAME_MAX_LEN);
	fsm->eModuleType = eModuleType;
	fsm->resetApiType = resetApiType;
	fsm->fgReadyForReset = false;
	fsm->resetFunc = NULL;
	fsm->notifyFunc = NULL;

	return fsm;
}

void freeFsmEntity(struct FsmEntity *fsm)
{
	if (!fsm)
		return;

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
		RFSM_Err("fsm args error\n");
		return;
	}

	currentFsmState = fsm->fsmState;
	if ((currentFsmState->name == NULL) ||
	    (currentFsmState->eventActionList == 0) ||
	    (currentFsmState->eventActionList == NULL)) {
		RFSM_Info("[%s] RFSM ignore event [%d]\n", fsm->name, event);
		return;
	}
	RFSM_Info("[%s] RFSM in [%s] state handl event [%d]\n",
		fsm->name, currentFsmState->name, event);

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
				RFSM_Info("[%s] RFSM: [%s] -> [%s]",
					fsm->name,
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
