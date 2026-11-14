/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "reset.h"
*   \brief  This file contains the declairation of reset module
*/


/**********************************************************************
*                         C O M P I L E R   F L A G S
***********************************************************************
*/
#ifndef _RESET_H
#define _RESET_H

/**********************************************************************
*                    E X T E R N A L   R E F E R E N C E S
***********************************************************************
*/
#include "reset_fsm.h"
#include "reset_fsm_def.h"
#include "reset_hif.h"

/**********************************************************************
*                                 M A C R O S
***********************************************************************
*/

/* Implements a timeout mechanism to detect and handle exceptions. */
#define RESETKO_SUPPORT_WAIT_TIMEOUT 1

/* Wait for the module to probe;
 * if a timeout occurs, perform the reset action again.
 */
#define RESETKO_WAIT_MODULE_PROBE_TIMEOUT 0

/* Wait for the module to send a ready event;
 * if it times out, forcibly execute the reset action.
 */
#define RESETKO_WAIT_MODULE_READY_TIMEOUT 1

#if defined(CFG_SUPPORT_MULTI_CARD) && (CFG_SUPPORT_MULTI_CARD == 1)
#define CFG_RESETKO_SUPPORT_MULTI_CARD 1
#define MAX_DONGLE_NUM 2
#else
#define CFG_RESETKO_SUPPORT_MULTI_CARD 0
#define MAX_DONGLE_NUM 1
#endif

/**********************************************************************
*                              C O N S T A N T S
***********************************************************************
*/

/**********************************************************************
*                             D A T A   T Y P E S
***********************************************************************
*/
enum ReturnStatus {
	RESET_RETURN_STATUS_SUCCESS = 0,
	RESET_RETURN_STATUS_FAIL,

	RESET_RETURN_STATUS_MAX
};


enum ModuleMsgId {
	BT_TO_WIFI_SET_WIFI_DRIVER_OWN = 0,

	WIFI_TO_BT_SET_DRIVER_OWN,
	WIFI_TO_BT_READ_WIFI_MCU_PC,

	RESET_MODULE_MSG_ID_MAX
};

enum HifInfoType {
	HIF_INFO_SDIO_HOST = 0,

	HIF_INFO_MAX
};

struct ModuleMsg {
	enum ModuleMsgId msgId;
	void *input;  // pointer to the function's input parameters
	void *output; // pointer to the function's return value
#if CFG_RESETKO_SUPPORT_MULTI_CARD
	uint32_t bus_id;
#endif
};

/**********************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
***********************************************************************
*/
enum ReturnStatus resetko_register_module(enum ModuleType module,
					char *name,
					enum TriggerResetApiType resetApiType,
					void *resetFunc,
					void *notifyFunc);
enum ReturnStatus resetko_unregister_module(enum ModuleType module);

#if CFG_RESETKO_SUPPORT_MULTI_CARD
enum ReturnStatus send_reset_event(uint32_t bus_id,
					enum ModuleType module,
					enum ResetFsmEvent event);

enum ReturnStatus send_msg_to_module(uint32_t bus_id,
					enum ModuleType srcModule,
					enum ModuleType dstModule,
					struct ModuleMsg *msg);

enum ReturnStatus update_hif_info(uint32_t bus_id,
					enum HifInfoType type, void *info);
#else
enum ReturnStatus send_reset_event(enum ModuleType module,
				enum ResetFsmEvent event);

enum ReturnStatus send_msg_to_module(enum ModuleType srcModule,
				    enum ModuleType dstModule,
				    struct ModuleMsg *msg);

enum ReturnStatus update_hif_info(enum HifInfoType type, void *info);
#endif

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

#endif /* _RESET_H */

