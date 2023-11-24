/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

/*! \file   reset_fsm_def.c
*   \brief  reset fsm def
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
#include "reset_fsm_def.h"

/**********************************************************************
*                                 M A C R O S
***********************************************************************
*/
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

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
static void enter_INIT(struct FsmEntity *fsm, struct FsmState *prev);
static void enter_L0_PRE_RESET(struct FsmEntity *fsm, struct FsmState *prev);
static void enter_L0_RESET_GOING(struct FsmEntity *fsm, struct FsmState *prev);
static bool gaurd_L0_PRE_RESET_L0_RESET_READY(struct FsmEntity *fsm);


/**********************************************************************
*                            P U B L I C   D A T A
***********************************************************************
*/


/**********************************************************************
*                           P R I V A T E   D A T A
***********************************************************************
*/
static struct FsmState RFSM_INIT;
static struct FsmState RFSM_WAIT_PROBE;
static struct FsmState RFSM_PROBED;
static struct FsmState RFSM_L05_RESET_GOING;
static struct FsmState RFSM_L0_PRE_RESET;
static struct FsmState RFSM_L0_RESET_GOING;

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_INIT[] = {
	{RFSM_EVENT_PROBE_START, NULL, NULL, &RFSM_WAIT_PROBE},
	{RFSM_EVENT_TRIGGER_RESET, NULL, NULL, &RFSM_L0_PRE_RESET}
};

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_WAIT_PROBE[] = {
	{RFSM_EVENT_TIMEOUT, NULL, NULL, &RFSM_L0_PRE_RESET},
	{RFSM_EVENT_PROBE_FAIL, NULL, NULL, &RFSM_L0_PRE_RESET},
	{RFSM_EVENT_PROBE_SUCCESS, NULL, NULL, &RFSM_PROBED},
	{RFSM_EVENT_TRIGGER_RESET, NULL, NULL, &RFSM_L0_PRE_RESET},
	{RFSM_EVENT_L0_RESET_DONE, NULL, NULL, &RFSM_INIT}
};

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_PROBED[] = {
	{RFSM_EVENT_REMOVE, NULL, NULL, &RFSM_INIT},
	{RFSM_EVENT_L05_START, NULL, NULL, &RFSM_L05_RESET_GOING},
	{RFSM_EVENT_TRIGGER_RESET, NULL, NULL, &RFSM_L0_PRE_RESET},
	{RFSM_EVENT_L0_RESET_DONE, NULL, NULL, &RFSM_INIT}
};

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_L05_RESET_GOING[] = {
	{RFSM_EVENT_L05_FAIL, NULL, NULL, &RFSM_L0_PRE_RESET},
	{RFSM_EVENT_L05_SUCCESS, NULL, NULL, &RFSM_PROBED},
	{RFSM_EVENT_TRIGGER_RESET, NULL, NULL, &RFSM_L0_PRE_RESET},
	{RFSM_EVENT_L0_RESET_DONE, NULL, NULL, &RFSM_INIT}
};

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_L0_PRE_RESET[] = {
	{RFSM_EVENT_TIMEOUT, NULL, NULL, &RFSM_L0_RESET_GOING},
	{RFSM_EVENT_L0_RESET_READY, gaurd_L0_PRE_RESET_L0_RESET_READY, NULL,
		&RFSM_L0_RESET_GOING},
	{RFSM_EVENT_L0_RESET_DONE, NULL, NULL, &RFSM_INIT}
};

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_L0_RESET_GOING[] = {
	{RFSM_EVENT_L0_RESET_DONE, NULL, NULL, &RFSM_INIT},
};

static struct FsmState RFSM_INIT = {
	.name = "INIT",
	.enter_func = NULL,
	.leave_func = NULL,
	.eventActionListCount = ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_INIT),
	.eventActionList = RFSM_EVENT_ACTION_LIST_INIT
};
static struct FsmState RFSM_WAIT_PROBE = {
	.name = "WAIT_PROBE",
	.enter_func = NULL,
	.leave_func = NULL,
	.eventActionListCount = ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_WAIT_PROBE),
	.eventActionList = RFSM_EVENT_ACTION_LIST_WAIT_PROBE
};
static struct FsmState RFSM_PROBED = {
	.name = "PROBED",
	.enter_func = NULL,
	.leave_func = NULL,
	.eventActionListCount = ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_PROBED),
	.eventActionList = RFSM_EVENT_ACTION_LIST_PROBED
};
static struct FsmState RFSM_L05_RESET_GOING = {
	.name = "L05_RESET_GOING",
	.enter_func = NULL,
	.leave_func = NULL,
	.eventActionListCount =
		ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_L05_RESET_GOING),
	.eventActionList = RFSM_EVENT_ACTION_LIST_L05_RESET_GOING
};
static struct FsmState RFSM_L0_PRE_RESET = {
	.name = "L0_PRE_RESET",
	.enter_func = enter_L0_PRE_RESET,
	.leave_func = NULL,
	.eventActionListCount =
		ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_L0_PRE_RESET),
	.eventActionList = RFSM_EVENT_ACTION_LIST_L0_PRE_RESET
};
static struct FsmState RFSM_L0_RESET_GOING = {
	.name = "L0_RESET_GOING",
	.enter_func = enter_L0_RESET_GOING,
	.leave_func = NULL,
	.eventActionListCount =
		ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_L0_RESET_GOING),
	.eventActionList = RFSM_EVENT_ACTION_LIST_L0_RESET_GOING
};


/**********************************************************************
*                              F U N C T I O N S
**********************************************************************/
static void enter_INIT(struct FsmEntity *fsm, struct FsmState *prev)
{
	RFSM_Info("module [%s]: %s\n", fsm->name, __func__);
	clearAllModuleReadyForReset();
	if (prev == &RFSM_L0_RESET_GOING)
		resetkoNotifyEvent(fsm, MODULE_NOTIFY_RESET_DONE);
}

static void enter_L0_PRE_RESET(struct FsmEntity *fsm, struct FsmState *prev)
{
	RFSM_Info("module [%s]: %s\n", fsm->name, __func__);
	resetkoNotifyEvent(fsm, MODULE_NOTIFY_PRE_RESET);
}

static void enter_L0_RESET_GOING(struct FsmEntity *fsm, struct FsmState *prev)
{
	RFSM_Info("module [%s]: %s\n", fsm->name, __func__);
	resetkoNotifyEvent(fsm, MODULE_NOTIFY_RESET_GOING);
	callResetFuncByResetApiType(fsm);
}

static bool gaurd_L0_PRE_RESET_L0_RESET_READY(struct FsmEntity *fsm)
{
	bool ret = isAllModuleReadyForReset();

	RFSM_Info("module [%s]: %s %s\n",
		  fsm->name, __func__, ret ? "pass" : "fail");

	return ret;
}

struct FsmEntity *allocResetFsm(char *name,
				enum ModuleType eModuleType,
				enum TriggerResetApiType resetApiType)
{
	struct FsmEntity *fsm = allocFsmEntity(name, eModuleType, resetApiType);

	if (!fsm)
		return NULL;

	fsm->fsmState = &RFSM_INIT;

	return fsm;
}

void freeResetFsm(struct FsmEntity *fsm)
{
	if (fsm)
		freeFsmEntity(fsm);
}

void resetFsmHandlevent(struct FsmEntity *fsm, enum ResetFsmEvent event)
{
	if ((fsm != NULL) && ((unsigned int)event < RFSM_EVENT_MAX))
		RFSM_handle_event(fsm, (unsigned int)event);
}


