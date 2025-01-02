/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

/*! \file   reset_fsm.h
*   \brief  reset_fsm.h
*
*
*/


/**********************************************************************
*                         C O M P I L E R   F L A G S
***********************************************************************
*/
#ifndef _RESET_FSM
#define _RESET_FSM

/**********************************************************************
*                    E X T E R N A L   R E F E R E N C E S
***********************************************************************
*/
#include <linux/kernel.h>

/**********************************************************************
*                                 M A C R O S
***********************************************************************
*/
#define RFSM_Info(_Fmt...)  pr_info("[reset][FSM] " _Fmt)
#define RFSM_Warn(_Fmt...)  pr_info("[reset][FSM] " _Fmt)
#define RFSM_Err(_Fmt...) pr_info("[reset][FSM] " _Fmt)

#define RFSM_NAME_MAX_LEN 32

/**********************************************************************
*                              C O N S T A N T S
***********************************************************************
*/


/**********************************************************************
*                             D A T A   T Y P E S
***********************************************************************
*/
enum ModuleType {
	RESET_MODULE_TYPE_WIFI = 0,
	RESET_MODULE_TYPE_BT,
	RESET_MODULE_TYPE_ZB,

	RESET_MODULE_TYPE_MAX
};

enum TriggerResetApiType {
	TRIGGER_RESET_TYPE_UNSUPPORT = 0,
	TRIGGER_RESET_TYPE_GPIO_API,
	TRIGGER_RESET_TYPE_SDIO_API,
	TRIGGER_RESET_TYPE_PCIE_API,
	TRIGGER_RESET_TYPE_SPI_API,

	TRIGGER_RESET_API_TYPE_MAX
};

struct FsmState;
struct FsmEntity;
typedef void (*fsmEnterFunc)(struct FsmEntity *fsm, struct FsmState *prev);
typedef void (*fsmLeaveFunc)(struct FsmEntity *fsm, struct FsmState *next);
typedef bool (*fsmGaurdFunc)(struct FsmEntity *fsm);
typedef void (*fsmActionFunc)(struct FsmEntity *fsm);

typedef void (*ResetFunc)(void);
typedef void (*NotifyFunc)(unsigned int event, void *data);


struct ResetFsmEventAction {
	unsigned int event;
	fsmGaurdFunc guard_func;
	fsmActionFunc action_func;
	struct FsmState *nextState;
};

struct FsmState {
	char *name;
	fsmEnterFunc enter_func;
	fsmLeaveFunc leave_func;

	unsigned int eventActionListCount;
	struct ResetFsmEventAction *eventActionList;
};

struct FsmEntity {
	struct list_head node;

	char *name;
	enum ModuleType eModuleType;

	enum TriggerResetApiType resetApiType;
	bool fgReadyForReset;
	ResetFunc resetFunc;
	NotifyFunc notifyFunc;

	struct FsmState *fsmState;
};

/**********************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
***********************************************************************
*/
struct FsmEntity *allocFsmEntity(char *name,
				 enum ModuleType eModuleType,
				 enum TriggerResetApiType resetApiType);
void freeFsmEntity(struct FsmEntity *fsm);

void RFSM_handle_event(struct FsmEntity *fsm, unsigned int event);

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


#endif  /* _RESET_FSM */

