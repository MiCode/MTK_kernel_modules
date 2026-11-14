/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "t2lm.h"
 *    \brief This file contains the internal used in T2LM modules
 *	 for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */

#ifndef _T2LM_H
#define _T2LM_H
#if (CFG_SUPPORT_802_11BE_T2LM == 1)

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define BE_IS_T2LM_CTRL_DIRECTION(_u2ctrl) \
		(_u2ctrl & T2LM_CTRL_DIRECTION)

#define BE_IS_T2LM_CTRL_DEFAULT_LINK(_u2ctrl) \
		(_u2ctrl & T2LM_CTRL_DEFAULT_LINK_MAPPING)

#define BE_IS_T2LM_CTRL_SWITCH_TIME(_u2ctrl) \
		(_u2ctrl & T2LM_CTRL_MAPPING_SWITCH_TIME_PRESENT)

#define BE_IS_T2LM_CTRL_DURATION(_u2ctrl) \
		(_u2ctrl & T2LM_CTRL_EXPECTED_DURATION_PRESENT)

#define BE_IS_T2LM_CTRL_LM_SIZE(_u2ctrl) \
		(_u2ctrl & T2LM_CTRL_LINK_MAPPING_SIZE)

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define T2LM_REQ_TX_TIMEOUT 3000

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct MLD_STA_RECORD;

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

enum ENUM_T2LM_STATE {
	T2LM_STATE_IDLE = 0,
	T2LM_STATE_ADV_SWITCH,
	T2LM_STATE_ADV_DURATION,
	T2LM_STATE_REQ_PENDING,
	T2LM_STATE_REQ_SWITCH,
	T2LM_STATE_REQ_DURATION,
	T2LM_STATE_NUM
};

struct T2LM_INFO {
	uint8_t ucDirection;
	uint8_t ucDefaultLM;
	uint8_t ucSwitchTimePresent;
	uint8_t ucDurationPresent;
	uint8_t ucLMSize;
	uint8_t ucLMIndicator;
	uint16_t u2MappingSwitchTime;
	uint32_t u4ExpectedDuration;
	uint16_t au2LMTid[MAX_NUM_T2LM_TIDS];

	/* for timer */
	uint32_t u4SwitchDelayMs;
	uint32_t u4T2lmDurationMs;
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define SET_T2LM_CTRL_DIRECTION(_u2ctrl) \
	(_u2ctrl |= T2LM_CTRL_DIRECTION)

#define SET_T2LM_CTRL_DEFAULT_LINK(_u2ctrl) \
	(_u2ctrl |= T2LM_CTRL_DEFAULT_LINK_MAPPING)

#define SET_T2LM_CTRL_SWITCH_TIME(_u2ctrl) \
	(_u2ctrl |= T2LM_CTRL_MAPPING_SWITCH_TIME_PRESENT)

#define SET_T2LM_CTRL_DURATION(_u2ctrl) \
	(_u2ctrl |= T2LM_CTRL_EXPECTED_DURATION_PRESENT)

#define SET_T2LM_CTRL_LM_SIZE(_u2ctrl) \
	(_u2ctrl |= T2LM_CTRL_LINK_MAPPING_SIZE)


/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
void t2lmFsmSteps(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStaRec,
	enum ENUM_T2LM_STATE eNextState);

uint32_t t2lmReqTxDoneCb(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t t2lmRspTxDoneCb(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t t2lmTeardownTxDoneCb(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t t2lmSend(struct ADAPTER *prAdapter, enum PROTECTED_EHT_ACTION eAction,
	struct STA_RECORD *prStaRec, struct T2LM_INFO *prT2LMParams);

void t2lmParseT2LMIE(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, const uint8_t *pucIE);

void t2lmTimeout(struct ADAPTER *prAdapter, uintptr_t ulParamPtr);

void t2lmFsmTimeout(struct ADAPTER *prAdapter, uintptr_t ulParamPtr);

void t2lmProcessAction(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb);

void t2lmComposeReq(struct MSDU_INFO *prMsduInfo, uint8_t ucDialogToken,
	struct T2LM_INFO *prT2LMParams);

void t2lmComposeRsp(struct MSDU_INFO *prMsduInfo, uint8_t ucDialogToken,
	uint8_t ucStatusCode, struct T2LM_INFO *prT2LMParams);

void t2lmMldStaRecBackup(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStaRec, struct T2LM_INFO *prT2LMParams);

void t2lmMldStaRecUpdate(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStaRec, u_int8_t fgSendcmd);

void t2lmReset(struct ADAPTER *prAdapter, struct MLD_STA_RECORD *prMldStaRec);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* CFG_SUPPORT_802_11BE_T2LM */
#endif /* _T2LM_H */

