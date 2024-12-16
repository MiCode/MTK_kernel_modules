/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

/*! \file   reset_fsm_def.h
*   \brief  reset_fsm_def.h
*
*
*/


/**********************************************************************
*                         C O M P I L E R   F L A G S
***********************************************************************
*/
#ifndef _RESET_FSM_DEF_H
#define _RESET_FSM_DEF_H

/**********************************************************************
*                    E X T E R N A L   R E F E R E N C E S
***********************************************************************
*/
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
enum ResetFsmEvent {
	RFSM_EVENT_TRIGGER_RESET = 0,
	RFSM_EVENT_TIMEOUT,
	RFSM_EVENT_PROBE_START,
	RFSM_EVENT_PROBE_FAIL,
	RFSM_EVENT_PROBE_SUCCESS,
	RFSM_EVENT_REMOVE,
	RFSM_EVENT_L05_START,
	RFSM_EVENT_L05_FAIL,
	RFSM_EVENT_L05_SUCCESS,
	RFSM_EVENT_L0_RESET_READY,
	RFSM_EVENT_L0_RESET_GOING,
	RFSM_EVENT_L0_RESET_DONE,

	RFSM_EVENT_MAX
};

enum ModuleNotifyEvent {
	MODULE_NOTIFY_MESSAGE = 0,

	MODULE_NOTIFY_PRE_RESET,
	MODULE_NOTIFY_RESET_GOING,
	MODULE_NOTIFY_RESET_DONE,

	MODULE_NOTIFY_MAX
};

/**********************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
***********************************************************************
*/
/* functions in reset_fsm_def.c */
struct FsmEntity *allocResetFsm(char *name,
				enum ModuleType eModuleType,
				enum TriggerResetApiType resetApiType);
void freeResetFsm(struct FsmEntity *fsm);

void resetFsmHandlevent(struct FsmEntity *fsm, enum ResetFsmEvent event);

/* functions in reset_ko.c */
void resetkoNotifyEvent(struct FsmEntity *fsm, enum ModuleNotifyEvent event);

void clearAllModuleReadyForReset(void);
bool isAllModuleReadyForReset(void);

void callResetFuncByResetApiType(struct FsmEntity *fsm);
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


#endif  /* _RESET_FSM_DEF_H */

