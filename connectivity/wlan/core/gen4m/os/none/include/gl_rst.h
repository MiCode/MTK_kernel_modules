/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_rst.h
 *    \brief  Declaration of functions and finite state machine for
 *	    MT6620 Whole-Chip Reset Mechanism
 */

#ifndef _GL_RST_H
#define _GL_RST_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_typedef.h"

#if 0
#include "mtk_porting.h"
#endif
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define RST_FLAG_CHIP_RESET        0

#define RST_FLAG_DO_CORE_DUMP              BIT(0)
#define RST_FLAG_PREVENT_POWER_OFF         BIT(1)
#define RST_FLAG_DO_WHOLE_RESET            BIT(2)
#define RST_FLAG_DO_L0P5_RESET             BIT(3)
#define RST_FLAG_DO_L1_RESET               BIT(4)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
enum ENUM_RESET_STATUS {
	RESET_FAIL,
	RESET_SUCCESS
};

/* TODO: os-related, implementation reference */
#if 0
struct RESET_STRUCT {
	enum ENUM_RESET_STATUS rst_data;
	struct work_struct rst_work;
	struct work_struct rst_trigger_work;
	uint32_t rst_trigger_flag;
};
#endif

#if (CFG_SUPPORT_CONNINFRA == 1)
enum ENUM_WMTMSG_TYPE {
	WMTMSG_TYPE_POWER_ON = 0,
	WMTMSG_TYPE_POWER_OFF = 1,
	WMTMSG_TYPE_RESET = 2,
	WMTMSG_TYPE_STP_RDY = 3,
	WMTMSG_TYPE_HW_FUNC_ON = 4,
	WMTMSG_TYPE_MAX
};

enum ENUM_WMTRSTMSG_TYPE {
	WMTRSTMSG_RESET_START = 0x0,  /*whole chip reset (include other radio)*/
	WMTRSTMSG_RESET_END = 0x1,
	WMTRSTMSG_RESET_END_FAIL = 0x2,
	WMTRSTMSG_0P5RESET_START = 0x3, /*wfsys reset ( wifi only )*/
	WMTRSTMSG_RESET_MAX,
	WMTRSTMSG_RESET_INVALID = 0xff
};

enum ENUM_WF_RST_SOURCE {
	WF_RST_SOURCE_NONE = 0x0,
	WF_RST_SOURCE_DRIVER = 0x1,
	WF_RST_SOURCE_FW = 0x2,
	WF_RST_SOURCE_MAX
};
#endif

enum _ENUM_CHIP_RESET_REASON_TYPE_T {
	RST_PROCESS_ABNORMAL_INT = 1,
	RST_DRV_OWN_FAIL,
	RST_FW_ASSERT,
	RST_FW_ASSERT_TIMEOUT,
	RST_BT_TRIGGER,
	RST_OID_TIMEOUT,
	RST_CMD_TRIGGER,
	RST_REQ_CHL_FAIL,
	RST_FW_DL_FAIL,
	RST_SER_TIMEOUT,
	RST_SLP_PROT_TIMEOUT,
	RST_REG_READ_DEADFEED,
	RST_P2P_CHNL_GRANT_INVALID_TYPE,
	RST_P2P_CHNL_GRANT_INVALID_STATE,
	RST_SCAN_RECOVERY,
	RST_ACCESS_REG_FAIL,
	RST_WIFI_ON_DRV_OWN_FAIL,
	RST_CHECK_READY_BIT_TIMEOUT,
	RST_ALLOC_CMD_FAIL,
	RST_SDIO_RX_ERROR,
	RST_WHOLE_CHIP_TRIGGER,
	RST_SER_L0P5_FAIL,
	RST_WDT,
	RST_REASON_MAX
};

enum ENUM_WFSYS_RESET_STATE_TYPE_T {
	WFSYS_RESET_STATE_IDLE = 0,
	WFSYS_RESET_STATE_RESET,
	WFSYS_RESET_STATE_REINIT,
	WFSYS_RESET_STATE_POSTPONE,
	WFSYS_RESET_STATE_MAX
};

/*******************************************************************************
 *                    E X T E R N A L   F U N C T I O N S
 *******************************************************************************
 */

#if CFG_CHIP_RESET_SUPPORT
extern int wifi_reset_start(void);
extern int wifi_reset_end(enum ENUM_RESET_STATUS);
#endif

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#if CFG_CHIP_RESET_SUPPORT
/* Each reset trigger reason has corresponding default reset action, which is
 * defined in glResetSelectAction(). You can use this macro to trigger default
 * action.
 */
#define GL_DEFAULT_RESET_TRIGGER(_prAdapter, _eReason)		\
do { \
	glSetRstReason(_eReason);    \
	glResetTrigger(_prAdapter, glResetSelectAction(_prAdapter),	\
		       (const uint8_t *)__FILE__, __LINE__);    \
} while (FALSE)

/* You can use this macro to trigger user defined reset actions instead of the
 * default ones.
 */
#define GL_USER_DEFINE_RESET_TRIGGER(_prAdapter, _eReason, _u4Flags)    \
do { \
	glSetRstReason(_eReason);    \
	glResetTrigger(_prAdapter, _u4Flags,	\
		       (const uint8_t *)__FILE__, __LINE__);    \
} while (FALSE)
#else
#define GL_DEFAULT_RESET_TRIGGER(_prAdapter, _eReason) \
	DBGLOG(INIT, DEBUG, "DO NOT support chip reset\n")

#define GL_USER_DEFINE_RESET_TRIGGER(_prAdapter, _eReason, _u4Flags)    \
	DBGLOG(INIT, DEBUG, "DO NOT support chip reset\n")
#endif

extern uint64_t u8ResetTime;
/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void glResetInit(void);

void glResetUninit(void);

void glSendResetRequest(void);

int32_t glIsWmtCodeDump(void);

#ifdef CFG_REMIND_IMPLEMENT

#define glSetRstReason(_eReason) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define glGetRstReason() \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define glResetSelectAction(_prAdapter) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define glResetTrigger(prAdapter, u4RstFlag, pucFile, u4Line) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define glSetWfsysResetState(_prAdapter, _state) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define glReSchWfsysReset(_prAdapter) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define WfsysResetHdlr(_work) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
void glSetRstReason(enum _ENUM_CHIP_RESET_REASON_TYPE_T
		    eReason);

int glSetRstReason(void);

uint32_t glResetSelectAction(struct ADAPTER *prAdapter);

void glResetTrigger(struct ADAPTER *prAdapter,
		    uint32_t u4RstFlag, const uint8_t *pucFile,
		    uint32_t u4Line);

void glSetWfsysResetState(struct ADAPTER *prAdapter,
			  enum ENUM_WFSYS_RESET_STATE_TYPE_T state);

u_int8_t glReSchWfsysReset(struct ADAPTER *prAdapter);

void WfsysResetHdlr(struct work_struct *work);
#endif

#endif /* _GL_RST_H */
