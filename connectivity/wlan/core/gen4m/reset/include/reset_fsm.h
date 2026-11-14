/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
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

#ifndef CFG_RESETKO_ENABLE_WAKE_LOCK
#define CFG_RESETKO_ENABLE_WAKE_LOCK 1
#endif

/**********************************************************************
*                    E X T E R N A L   R E F E R E N C E S
***********************************************************************
*/
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/device.h>

/**********************************************************************
*                                 M A C R O S
***********************************************************************
*/
#define MR_Info(_Fmt...)  pr_info("[reset] " _Fmt)
#define MR_Warn(_Fmt...)  pr_warn("[reset] " _Fmt)
#define MR_Err(_Fmt...) pr_err("[reset] " _Fmt)

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
	RESET_MODULE_TYPE_LRWPAN = RESET_MODULE_TYPE_ZB,

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

	uint32_t dongle_id;
	char *name;
	enum ModuleType eModuleType;

	bool fgReady;
	NotifyFunc notifyFunc;
	struct mutex notifyEventMutex;
	struct list_head notifyEventList;
	struct delayed_work notifyWork;

#if CFG_RESETKO_ENABLE_WAKE_LOCK
	struct wakeup_source *wakeupSource;
#endif
	int wakeupCount;

	struct timer_list resetTimer;
	struct FsmState *fsmState;

};

/**********************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
***********************************************************************
*/
struct FsmEntity *allocFsmEntity(uint32_t dongle_id,
				char *name, enum ModuleType eModuleType);
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

