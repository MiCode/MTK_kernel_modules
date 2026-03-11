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
#define RESETKO_API_VERSION 1

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

enum HifInfoType {
	HIF_INFO_SDIO_HOST = 0,

	HIF_INFO_MAX
};

struct ModuleMsg {
	unsigned int msgId;
	void *msgData;
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

enum ReturnStatus send_reset_event(enum ModuleType module,
				enum ResetFsmEvent event);

enum ReturnStatus send_msg_to_module(enum ModuleType srcModule,
				    enum ModuleType dstModule,
				    void *msg);

enum ReturnStatus update_hif_info(enum HifInfoType type, void *info);


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

