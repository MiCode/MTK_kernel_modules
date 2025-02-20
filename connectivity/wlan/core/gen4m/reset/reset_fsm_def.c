// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
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
#include "reset.h"

/**********************************************************************
*                                 M A C R O S
***********************************************************************
*/
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)		(sizeof(arr)/sizeof((arr)[0]))
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
static void enter_POWER_UP(struct FsmEntity *fsm, struct FsmState *prev);
static void leave_POWER_UP(struct FsmEntity *fsm, struct FsmState *next);
static void enter_PROBED(struct FsmEntity *fsm, struct FsmState *prev);
static void enter_PRE_RESET(struct FsmEntity *fsm, struct FsmState *prev);
static void leave_PRE_RESET(struct FsmEntity *fsm, struct FsmState *next);
static void enter_PRE_POWER_OFF(struct FsmEntity *fsm, struct FsmState *prev);
static void leave_PRE_POWER_OFF(struct FsmEntity *fsm, struct FsmState *next);
static void enter_POWER_OFF(struct FsmEntity *fsm, struct FsmState *prev);
static void leave_POWER_OFF(struct FsmEntity *fsm, struct FsmState *next);

static bool gaurd_isAllModuleReady(struct FsmEntity *fsm);

static void action_RestartWaitprobeTimer(struct FsmEntity *fsm);
static void action_PowerReset(struct FsmEntity *fsm);
static void action_PowerOff(struct FsmEntity *fsm);
static void action_PowerOn(struct FsmEntity *fsm);


/**********************************************************************
*                            P U B L I C   D A T A
***********************************************************************
*/


/**********************************************************************
*                           P R I V A T E   D A T A
***********************************************************************
*/
static struct FsmState RFSM_POWER_UP;
static struct FsmState RFSM_PROBED;
static struct FsmState RFSM_PRE_RESET;
static struct FsmState RFSM_PRE_POWER_OFF;
static struct FsmState RFSM_POWER_OFF;

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_POWER_UP[] = {
	{RFSM_EVENT_PROBED, NULL, NULL, &RFSM_PROBED},
	{RFSM_EVENT_TRIGGER_RESET, NULL, NULL, &RFSM_PRE_RESET},
	{RFSM_EVENT_TRIGGER_POWER_OFF, NULL, NULL, &RFSM_PRE_POWER_OFF},
	{RFSM_EVENT_START_PROBE, NULL, action_RestartWaitprobeTimer, NULL},
	/* wait probe timeout, trigger reset */
	{RFSM_EVENT_TIMEOUT, NULL, NULL, &RFSM_PRE_RESET}
};

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_PROBED[] = {
	{RFSM_EVENT_REMOVED, NULL, NULL, &RFSM_POWER_UP},
	{RFSM_EVENT_TRIGGER_RESET, NULL, NULL, &RFSM_PRE_RESET},
	{RFSM_EVENT_TRIGGER_POWER_OFF, NULL, NULL, &RFSM_PRE_POWER_OFF},
	{RFSM_EVENT_RESET_DONE, NULL, NULL, &RFSM_POWER_UP}
};

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_PRE_RESET[] = {
	{RFSM_EVENT_READY, gaurd_isAllModuleReady, action_PowerReset,
		&RFSM_PRE_RESET},
	/* timeout force reset */
	{RFSM_EVENT_TIMEOUT, NULL, action_PowerReset, &RFSM_PRE_RESET},
	/* if trigger power off before reset, ignore reset and run power off */
	{RFSM_EVENT_TRIGGER_POWER_OFF, NULL, NULL, &RFSM_PRE_POWER_OFF},
	{RFSM_EVENT_RESET_DONE, NULL, NULL, &RFSM_POWER_UP}
};

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_PRE_POWER_OFF[] = {
	{RFSM_EVENT_READY, gaurd_isAllModuleReady, action_PowerOff,
		&RFSM_POWER_OFF},
	/* timeout force power off */
	{RFSM_EVENT_TIMEOUT, NULL, action_PowerOff, &RFSM_POWER_OFF},
	/* power off can't be cancel, so goto PRE_RESET, don't reset timer */
	{RFSM_EVENT_TRIGGER_POWER_ON, NULL, NULL, &RFSM_PRE_RESET},
	{RFSM_EVENT_POWER_OFF_DONE, NULL, NULL, &RFSM_POWER_OFF},
	{RFSM_EVENT_RESET_DONE, NULL, NULL, &RFSM_POWER_UP}
};

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_POWER_OFF[] = {
	{RFSM_EVENT_TRIGGER_POWER_ON, NULL, action_PowerOn, &RFSM_POWER_UP},
	{RFSM_EVENT_POWER_ON_DONE, NULL, NULL, &RFSM_POWER_UP},
	{RFSM_EVENT_RESET_DONE, NULL, NULL, &RFSM_POWER_UP}
};

static struct FsmState RFSM_POWER_UP = {
	.name = "POWER_UP",
	.enter_func = enter_POWER_UP,
	.leave_func = leave_POWER_UP,
	.eventActionListCount = ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_POWER_UP),
	.eventActionList = RFSM_EVENT_ACTION_LIST_POWER_UP
};
static struct FsmState RFSM_PROBED = {
	.name = "PROBED",
	.enter_func = enter_PROBED,
	.leave_func = NULL,
	.eventActionListCount = ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_PROBED),
	.eventActionList = RFSM_EVENT_ACTION_LIST_PROBED
};
static struct FsmState RFSM_PRE_RESET = {
	.name = "PRE_RESET",
	.enter_func = enter_PRE_RESET,
	.leave_func = leave_PRE_RESET,
	.eventActionListCount =
		ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_PRE_RESET),
	.eventActionList = RFSM_EVENT_ACTION_LIST_PRE_RESET
};
static struct FsmState RFSM_PRE_POWER_OFF = {
	.name = "PRE_POWER_OFF",
	.enter_func = enter_PRE_POWER_OFF,
	.leave_func = leave_PRE_POWER_OFF,
	.eventActionListCount =
		ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_PRE_POWER_OFF),
	.eventActionList = RFSM_EVENT_ACTION_LIST_PRE_POWER_OFF
};
static struct FsmState RFSM_POWER_OFF = {
	.name = "POWER_OFF",
	.enter_func = enter_POWER_OFF,
	.leave_func = leave_POWER_OFF,
	.eventActionListCount =
		ARRAY_SIZE(RFSM_EVENT_ACTION_LIST_POWER_OFF),
	.eventActionList = RFSM_EVENT_ACTION_LIST_POWER_OFF
};


/**********************************************************************
*                              F U N C T I O N S
**********************************************************************/
static void enter_POWER_UP(struct FsmEntity *fsm, struct FsmState *prev)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	resetkoStartTimer(fsm, WAIT_PROBE_TIMEOUT);
}

static void leave_POWER_UP(struct FsmEntity *fsm, struct FsmState *next)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	resetkoCancleTimer(fsm);
}

static void enter_PROBED(struct FsmEntity *fsm, struct FsmState *prev)
{
	if (fsm->wakeupCount > 0) {
		wakeupSourceRelax(fsm);
		resetkoNotifyEvent(fsm, MODULE_NOTIFY_RESET_DONE);
	}
}

static void enter_PRE_RESET(struct FsmEntity *fsm, struct FsmState *prev)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	if (prev != &RFSM_PRE_POWER_OFF) {
		fsm->fgReady = false;
		resetkoStartTimer(fsm, WAIT_ALL_MODULE_READY_TIMEOUT);
		wakeupSourceStayAwake(fsm);
		resetkoNotifyEvent(fsm, MODULE_NOTIFY_PRE_POWER_OFF);
	}
}

static void leave_PRE_RESET(struct FsmEntity *fsm, struct FsmState *next)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	if (next != &RFSM_PRE_POWER_OFF) {
		resetkoCancleTimer(fsm);
		fsm->fgReady = false;
	}
}

static void enter_PRE_POWER_OFF(struct FsmEntity *fsm, struct FsmState *prev)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	if (prev != &RFSM_PRE_RESET) {
		fsm->fgReady = false;
		resetkoStartTimer(fsm, WAIT_ALL_MODULE_READY_TIMEOUT);
		wakeupSourceStayAwake(fsm);
		resetkoNotifyEvent(fsm, MODULE_NOTIFY_PRE_POWER_OFF);
	}
}
static void leave_PRE_POWER_OFF(struct FsmEntity *fsm, struct FsmState *next)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	if (next != &RFSM_PRE_POWER_OFF) {
		resetkoCancleTimer(fsm);
		fsm->fgReady = false;
		wakeupSourceRelax(fsm);
	}
}

static void enter_POWER_OFF(struct FsmEntity *fsm, struct FsmState *prev)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	resetkoNotifyEvent(fsm, MODULE_NOTIFY_POWER_OFF_DONE);
}

static void leave_POWER_OFF(struct FsmEntity *fsm, struct FsmState *next)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	wakeupSourceStayAwake(fsm);
}

static bool gaurd_isAllModuleReady(struct FsmEntity *fsm)
{
	bool ret = isAllModuleReady();

	MR_Info("[%s] %s %s\n", fsm->name, __func__, ret ? "pass" : "fail");

	return ret;
}

static void action_RestartWaitprobeTimer(struct FsmEntity *fsm)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	resetkoStartTimer(fsm, WAIT_PROBE_TIMEOUT);
}

static void action_PowerReset(struct FsmEntity *fsm)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	powerReset();
}

static void action_PowerOff(struct FsmEntity *fsm)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	powerOff();
	send_reset_event(fsm->eModuleType, RFSM_EVENT_POWER_OFF_DONE);
}

static void action_PowerOn(struct FsmEntity *fsm)
{
	MR_Info("[%s] %s\n", fsm->name, __func__);
	powerOn();
	send_reset_event(fsm->eModuleType, RFSM_EVENT_POWER_ON_DONE);
}

struct FsmEntity *allocResetFsm(char *name, enum ModuleType eModuleType)
{
	struct FsmEntity *fsm = allocFsmEntity(name, eModuleType);

	if (!fsm)
		return NULL;

	fsm->fsmState = &RFSM_POWER_UP;

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


