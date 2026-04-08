/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux/include
 *     /gl_rst.h#1
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


/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define RST_FLAG_DO_CORE_DUMP              BIT(0)
#define RST_FLAG_PREVENT_POWER_OFF         BIT(1)
#define RST_FLAG_DO_WHOLE_RESET            BIT(2)
#define RST_FLAG_DO_L0P5_RESET             BIT(3)
#define RST_FLAG_DO_L1_RESET               BIT(4)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* If there's any modification in this enum, then sync to apcReason[] in
 * glResetTriggerCommon().
 */
enum _ENUM_CHIP_RESET_REASON_TYPE_T {
	RST_UNKNOWN = 0,
	RST_PROCESS_ABNORMAL_INT,
	RST_DRV_OWN_FAIL,
	RST_FW_ASSERT_DONE,
	RST_FW_ASSERT_TIMEOUT,
	RST_BT_TRIGGER,
	RST_OID_TIMEOUT,
	RST_CMD_TRIGGER,
	RST_CR_ACCESS_FAIL,
	RST_CMD_EVT_FAIL,
	RST_GROUP4_NULL,
	RST_TX_ERROR,
	RST_RX_ERROR,
	RST_WDT,
	RST_SER_L1_FAIL,    /* TODO */
	RST_SER_L0P5_FAIL,  /* TODO */
	RST_REASON_MAX
};

/* L0.5 reset state */
enum ENUM_WFSYS_RESET_STATE_TYPE_T {
	WFSYS_RESET_STATE_IDLE = 0,
	WFSYS_RESET_STATE_DETECT,
	WFSYS_RESET_STATE_RESET,
	WFSYS_RESET_STATE_REINIT,
	WFSYS_RESET_STATE_POSTPONE,
	WFSYS_RESET_STATE_MAX
};

struct CHIP_RESET_INFO {
	u_int8_t fgIsInitialized;

	struct GLUE_INFO *prGlueInfo;
	uint32_t u4ProbeCount;
	uint64_t u8ResetTime;
	u_int8_t fgIsResetting;
	u_int8_t fgSimplifyResetFlow;
	enum _ENUM_CHIP_RESET_REASON_TYPE_T eResetReason;

	u_int8_t fgIsRstPreventFwOwn;
	struct work_struct rWfsysResetWork;

#if CFG_CHIP_RESET_KO_SUPPORT
	uint32_t u4RstCount;
	uint32_t u4PowerOffCount;
	u_int8_t fgIsPendingForReady;
#endif
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T *prWlanChipResetWakeLock;
#endif
};

typedef void pFuncL0p5Work(struct work_struct *work);


/*******************************************************************************
 *                    E X T E R N A L   F U N C T I O N S
 *******************************************************************************
 */

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
#define glResetSelectAction(_prAdapter)    glResetSelectActionCe(_prAdapter)

/* Each reset trigger reason has corresponding default reset action, which is
 * defined in glResetSelectAction(). You can use this macro to trigger default
 * action.
 */
#define GL_DEFAULT_RESET_TRIGGER(_prAdapter, _eReason)		\
{     \
	glSetRstReason(_prAdapter, _eReason);    \
	glResetTrigger(_prAdapter, glResetSelectAction(_prAdapter),	\
		       (const uint8_t *)__FILE__, __LINE__);    \
}

/* You can use this macro to trigger user defined reset actions instead of the
 * default ones.
 */
#define GL_USER_DEFINE_RESET_TRIGGER(_prAdapter, _eReason, _u4Flags)    \
{     \
	glSetRstReason(_prAdapter, _eReason);    \
	glResetTrigger(_prAdapter, _u4Flags,	\
		       (const uint8_t *)__FILE__, __LINE__);    \
}

#define GL_SET_WFSYS_RESET_POSTPONE(_prAdapter, _fgPostpone)    \
	(_prAdapter->fgWfsysResetPostpone = _fgPostpone)

#define GL_GET_WFSYS_RESET_POSTPONE(_prAdapter)    \
	(_prAdapter->fgWfsysResetPostpone)

#else /* CFG_CHIP_RESET_SUPPORT */

#define glResetSelectAction(_prAdapter)    \
{    \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n");    \
}

#define GL_DEFAULT_RESET_TRIGGER(_prAdapter, _eReason) \
{    \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n");    \
}

#define GL_USER_DEFINE_RESET_TRIGGER(_prAdapter, _eReason, _u4Flags)    \
{    \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n");    \
}

#define GL_SET_WFSYS_RESET_POSTPONE(_prAdapter, _fgPostpone)    \
{    \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n");    \
}

#define GL_GET_WFSYS_RESET_POSTPONE(_prAdapter)    \
{    \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n");    \
}
#endif /* CFG_CHIP_RESET_SUPPORT */


/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

u_int8_t kalIsResetting(struct GLUE_INFO *prGlueInfo);
void glSetResettingFlag(struct GLUE_INFO *prGlueInfo, u_int8_t value);

u_int8_t kalGetSimplifyResetFlowFlag(struct GLUE_INFO *prGlueInfo);
void glSetSimplifyResetFlowFlag(struct GLUE_INFO *prGlueInfo, u_int8_t value);

u_int8_t kalIsRstPreventFwOwn(struct GLUE_INFO *prGlueInfo);
void glSetRstPreventFwOwn(struct GLUE_INFO *prGlueInfo, u_int8_t value);

uint64_t glGetRstTIme(struct GLUE_INFO *prGlueInfo);

#if CFG_CHIP_RESET_SUPPORT
void glResetInit(struct GLUE_INFO *prGlueInfo);
void glResetUninit(struct GLUE_INFO *prGlueInfo);
void glReseProbeRemoveDone(struct GLUE_INFO *prGlueInfo, int32_t i4Status,
			   u_int8_t fgIsProbe);

void glSetRstReason(struct ADAPTER *prAdapter,
		    enum _ENUM_CHIP_RESET_REASON_TYPE_T eReason);
int glGetRstReason(struct ADAPTER *prAdapter);
uint32_t glResetSelectActionCe(IN struct ADAPTER *prAdapter);
void glResetTrigger(struct ADAPTER *prAdapter, uint32_t u4RstFlag,
		    const uint8_t *pucFile, uint32_t u4Line);

void glResetInitL0p5Work(struct GLUE_INFO *prGlueInfo);
void glResetWaitL0p5WorkDone(struct GLUE_INFO *prGlueInfo);
u_int8_t glResetReSchL0p5Work(struct GLUE_INFO *prGlueInfo);

#if CFG_CHIP_RESET_KO_SUPPORT
void resetkoNotifyFunc(unsigned int event, void *data);
#endif

#endif /* CFG_CHIP_RESET_SUPPORT */

#endif /* _GL_RST_H */
