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
#define IS_ARRAY(arr) ((void *)&(arr) == &(arr)[0])
#define STATIC_EXP(e) (0 * sizeof(struct {int ARRAY_SIZE_FAILED:(2*(e) - 1); }))
#define ARRAY_SIZE(D) (sizeof(D) / sizeof((D)[0]) + STATIC_EXP(IS_ARRAY(D)))
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
static bool guard_isResetWorkIdle(struct FsmEntity *fsm);

static void action_RestartWaitprobeTimer(struct FsmEntity *fsm);
static void action_PowerReset(struct FsmEntity *fsm);
static void action_PowerOff(struct FsmEntity *fsm);
static void action_PowerOn(struct FsmEntity *fsm);
static void action_NotifyLrwpanResetDone(struct FsmEntity *fsm);


/**********************************************************************
*                            P U B L I C   D A T A
***********************************************************************
*/


/**********************************************************************
*                           P R I V A T E   D A T A
***********************************************************************
*/
bool fgResetWorkIdle = true;

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
#if RESETKO_WAIT_MODULE_PROBE_TIMEOUT
	/* wait probe timeout, trigger reset */
	{RFSM_EVENT_TIMEOUT, NULL, NULL, &RFSM_PRE_RESET}
#endif
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
#if RESETKO_WAIT_MODULE_READY_TIMEOUT
	/* timeout force reset */
	{RFSM_EVENT_TIMEOUT, guard_isResetWorkIdle, action_PowerReset,
		&RFSM_PRE_RESET},
#endif
	/* if trigger power off before reset, ignore reset and run power off */
	{RFSM_EVENT_TRIGGER_POWER_OFF, NULL, NULL, &RFSM_PRE_POWER_OFF},
	{RFSM_EVENT_RESET_DONE, NULL, action_NotifyLrwpanResetDone,
		&RFSM_POWER_UP}
};

static struct ResetFsmEventAction RFSM_EVENT_ACTION_LIST_PRE_POWER_OFF[] = {
	{RFSM_EVENT_READY, gaurd_isAllModuleReady, action_PowerOff,
		&RFSM_POWER_OFF},
#if RESETKO_WAIT_MODULE_READY_TIMEOUT
	/* timeout force power off */
	{RFSM_EVENT_TIMEOUT, NULL, action_PowerOff, &RFSM_POWER_OFF},
#endif
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
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	resetkoStartTimer(fsm, WAIT_PROBE_TIMEOUT);
}

static void leave_POWER_UP(struct FsmEntity *fsm, struct FsmState *next)
{
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	resetkoCancleTimer(fsm);
}

static void enter_PROBED(struct FsmEntity *fsm, struct FsmState *prev)
{
	if (fsm->wakeupCount > 0) {
		wakeupSourceRelax(fsm);
		if (fsm->eModuleType != RESET_MODULE_TYPE_LRWPAN)
			resetkoNotifyEvent(fsm, MODULE_NOTIFY_RESET_DONE);
	}
}

static void enter_PRE_RESET(struct FsmEntity *fsm, struct FsmState *prev)
{
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	if (prev != &RFSM_PRE_POWER_OFF) {
		fsm->fgReady = false;
		resetkoStartTimer(fsm, WAIT_ALL_MODULE_READY_TIMEOUT);
		wakeupSourceStayAwake(fsm);
		resetkoNotifyEvent(fsm, MODULE_NOTIFY_PRE_POWER_OFF);
	}
}

static void leave_PRE_RESET(struct FsmEntity *fsm, struct FsmState *next)
{
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	if (next != &RFSM_PRE_POWER_OFF) {
		resetkoCancleTimer(fsm);
		fsm->fgReady = false;
	}
	fgResetWorkIdle = true;
}

static void enter_PRE_POWER_OFF(struct FsmEntity *fsm, struct FsmState *prev)
{
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	if (prev != &RFSM_PRE_RESET) {
		fsm->fgReady = false;
		resetkoStartTimer(fsm, WAIT_ALL_MODULE_READY_TIMEOUT);
		wakeupSourceStayAwake(fsm);
		resetkoNotifyEvent(fsm, MODULE_NOTIFY_PRE_POWER_OFF);
	}
}
static void leave_PRE_POWER_OFF(struct FsmEntity *fsm, struct FsmState *next)
{
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	if (next != &RFSM_PRE_POWER_OFF) {
		resetkoCancleTimer(fsm);
		fsm->fgReady = false;
		wakeupSourceRelax(fsm);
	}
}

static void enter_POWER_OFF(struct FsmEntity *fsm, struct FsmState *prev)
{
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	resetkoNotifyEvent(fsm, MODULE_NOTIFY_POWER_OFF_DONE);
}

static void leave_POWER_OFF(struct FsmEntity *fsm, struct FsmState *next)
{
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	wakeupSourceStayAwake(fsm);
}

static bool gaurd_isAllModuleReady(struct FsmEntity *fsm)
{
	bool ret = isAllModuleReady(fsm->dongle_id);

	MR_Info("[%s_%d] %s %s\n",
		fsm->name, fsm->dongle_id, __func__, ret ? "pass" : "fail");

	return ret;
}

static bool guard_isResetWorkIdle(struct FsmEntity *fsm)
{
	MR_Info("[%s] %s %s\n", fsm->name, __func__,
		fgResetWorkIdle ? "pass" : "fail");
	return fgResetWorkIdle;
}

static void action_RestartWaitprobeTimer(struct FsmEntity *fsm)
{
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	resetkoStartTimer(fsm, WAIT_PROBE_TIMEOUT);
}

static void action_PowerReset(struct FsmEntity *fsm)
{
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	fgResetWorkIdle = false;
	powerReset(fsm->dongle_id);
}

static void action_PowerOff(struct FsmEntity *fsm)
{
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	uint32_t bus_id;
#endif
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	powerOff(fsm->dongle_id);
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	if (findBusIdByDongleId(fsm->dongle_id, &bus_id))
		send_reset_event(bus_id,
				 fsm->eModuleType, RFSM_EVENT_POWER_OFF_DONE);
	else
		MR_Err("[%s_%d] can't find bus_id\n",
			fsm->name, fsm->dongle_id);
#else
	send_reset_event(fsm->eModuleType, RFSM_EVENT_POWER_OFF_DONE);
#endif
}

static void action_PowerOn(struct FsmEntity *fsm)
{
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	uint32_t bus_id;
#endif
	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	powerOn(fsm->dongle_id);
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	if (findBusIdByDongleId(fsm->dongle_id, &bus_id))
		send_reset_event(bus_id,
				 fsm->eModuleType, RFSM_EVENT_POWER_ON_DONE);
	else
		MR_Err("[%s_%d] can't find bus_id\n",
			fsm->name, fsm->dongle_id);
#else
	send_reset_event(fsm->eModuleType, RFSM_EVENT_POWER_ON_DONE);
#endif
}

static void action_NotifyLrwpanResetDone(struct FsmEntity *fsm)
{
	if (fsm->eModuleType != RESET_MODULE_TYPE_LRWPAN)
		return;

	MR_Info("[%s_%d] %s\n", fsm->name, fsm->dongle_id, __func__);
	resetkoNotifyEvent(fsm, MODULE_NOTIFY_RESET_DONE);
}

struct FsmEntity *allocResetFsm(uint32_t dongle_id,
				char *name, enum ModuleType eModuleType)
{
	struct FsmEntity *fsm = allocFsmEntity(dongle_id, name, eModuleType);

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


