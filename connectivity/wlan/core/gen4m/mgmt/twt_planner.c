// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "twt_planner.c"
*   \brief  TWT Planner to determine TWT negotiation policy
*/


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
uint8_t g_IsWfaTestBed;  /* To indocate if WFA test bed */
uint8_t g_IsTwtLogo = 0xFF;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

static uint32_t
_twtPlannerDrvAgrtAdd(struct _TWT_PLANNER_T *prTWTPlanner,
	uint8_t ucBssIdx, uint8_t ucFlowId,
	struct _TWT_PARAMS_T *prTWTParams, uint8_t ucIdx,
	enum _ENUM_TWT_TYPE_T eTwtType)
{
	struct _TWT_AGRT_T *prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[ucIdx]);

	prTWTAgrt->fgValid = TRUE;
	prTWTAgrt->ucBssIdx = ucBssIdx;
	prTWTAgrt->ucFlowId = ucFlowId;
	prTWTAgrt->ucAgrtTblIdx = ucIdx;
	kalMemCopy(&(prTWTAgrt->rTWTAgrt), prTWTParams,
		sizeof(struct _TWT_PARAMS_T));
	prTWTAgrt->eTwtType = eTwtType;

	return WLAN_STATUS_SUCCESS;
}

static uint32_t
_twtPlannerDrvAgrtDel(
	struct _TWT_PLANNER_T *prTWTPlanner, uint8_t ucIdx)
{
	struct _TWT_AGRT_T *prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[ucIdx]);

	kalMemSet(prTWTAgrt, 0, sizeof(struct _TWT_AGRT_T));

	return WLAN_STATUS_SUCCESS;
}

static uint32_t
_twtPlannerDrvAgrtModify(
	struct _TWT_PLANNER_T *prTWTPlanner,
	struct _NEXT_TWT_INFO_T *prNextTWTInfo,
	uint64_t u8CurTsf, uint8_t ucIdx,
	struct _TWT_PARAMS_T *prTWTParams)
{
	struct _TWT_AGRT_T *prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[ucIdx]);

	if (!prNextTWTInfo || !prTWTParams)
		return WLAN_STATUS_FAILURE;

	if (prNextTWTInfo->ucNextTWTSize == NEXT_TWT_SUBFIELD_64_BITS) {
		prTWTAgrt->rTWTAgrt.u8TWT = prNextTWTInfo->u8NextTWT;
	} else if (prNextTWTInfo->ucNextTWTSize == NEXT_TWT_SUBFIELD_32_BITS) {
		prTWTAgrt->rTWTAgrt.u8TWT =
			((u8CurTsf & ((uint64_t)0xFFFFFFFF << 32)) |
			prNextTWTInfo->u8NextTWT);
	} else if (prNextTWTInfo->ucNextTWTSize == NEXT_TWT_SUBFIELD_48_BITS) {
		prTWTAgrt->rTWTAgrt.u8TWT =
			((u8CurTsf & ((uint64_t)0xFFFF << 48)) |
			prNextTWTInfo->u8NextTWT);
	} else {
		/* Zero bit Next TWT is not acceptable */
		return WLAN_STATUS_FAILURE;
	}

	kalMemCopy(prTWTParams, &(prTWTAgrt->rTWTAgrt),
		sizeof(struct _TWT_PARAMS_T));

	return WLAN_STATUS_SUCCESS;
}

static uint32_t
_twtPlannerDrvAgrtGet(
	struct _TWT_PLANNER_T *prTWTPlanner,
	uint8_t ucIdx, struct _TWT_PARAMS_T *prTWTParams)
{
	struct _TWT_AGRT_T *prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[ucIdx]);

	if (!prTWTParams)
		return WLAN_STATUS_FAILURE;

	kalMemCopy(prTWTParams, &(prTWTAgrt->rTWTAgrt),
		sizeof(struct _TWT_PARAMS_T));

	return WLAN_STATUS_SUCCESS;
}

uint8_t
twtPlannerDrvAgrtFind(struct ADAPTER *prAdapter, uint8_t ucBssIdx,
	uint8_t ucFlowId, uint8_t *pucFlowId)
{
	uint8_t i;
	struct _TWT_PLANNER_T *prTWTPlanner = &(prAdapter->rTWTPlanner);
	struct _TWT_AGRT_T *prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[0]);
	uint8_t uCnt = 0;

	for (i = 0; i < TWT_AGRT_MAX_NUM; i++, prTWTAgrt++) {
		if ((prTWTAgrt->fgValid == TRUE) &&
			(prTWTAgrt->ucFlowId == ucFlowId) &&
			(prTWTAgrt->ucBssIdx == ucBssIdx)) {
			*pucFlowId = prTWTAgrt->ucFlowId;
			break;
		}
	}

	if ((i >= TWT_AGRT_MAX_NUM) && (pucFlowId != NULL)) {
		/*
		** This might be the case that we are in WFA logo
		** and QCOM reply TWT flow ID not identical to our
		** requested TWT flow ID
		*/

		/* Check if there exists only 1 valid TWT agrt */
		prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[0]);

		for (i = 0; i < TWT_AGRT_MAX_NUM; i++, prTWTAgrt++) {
			if (prTWTAgrt->fgValid == TRUE)
				uCnt++;
		}

		/* Yes, only 1 valid TWT agrt exists,
		** take its TWT flow ID if the input ucFlowId == 0
		*/
		if ((uCnt == 1) && (ucFlowId == 0)) {
			prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[0]);

			for (i = 0; i < TWT_AGRT_MAX_NUM; i++, prTWTAgrt++) {
				if ((prTWTAgrt->fgValid == TRUE) &&
					(prTWTAgrt->ucBssIdx == ucBssIdx)) {
					*pucFlowId = prTWTAgrt->ucFlowId;

					DBGLOG(TWT_PLANNER, WARN,
						"Gotcha, agrt bss %u flow %u->%u\n",
						ucBssIdx, ucFlowId, *pucFlowId);

					break;
				}
			}
		}
	}

	return i;
}

uint32_t
twtPlannerDrvAgrtAdd(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx, uint8_t ucFlowId,
	struct _TWT_PARAMS_T *prTWTParams, uint8_t *pucIdx,
	enum _ENUM_TWT_TYPE_T eTwtType)
{
	uint8_t ucIdx;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	struct _TWT_PLANNER_T *prTWTPlanner = &(prAdapter->rTWTPlanner);
	struct _TWT_AGRT_T *prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[0]);

	for (ucIdx = 0; ucIdx < TWT_AGRT_MAX_NUM; ucIdx++, prTWTAgrt++) {
		if (prTWTAgrt->fgValid == FALSE)
			break;
	}

	if (ucIdx < TWT_AGRT_MAX_NUM) {
		_twtPlannerDrvAgrtAdd(prTWTPlanner, ucBssIdx,
			ucFlowId, prTWTParams, ucIdx, eTwtType);
		*pucIdx = ucIdx;
		rStatus = WLAN_STATUS_SUCCESS;
	}

	return rStatus;
}

enum _ENUM_TWT_TYPE_T
twtPlannerDrvAgrtGetTwtTypeByIndex(
	struct ADAPTER *prAdapter, uint8_t ucAgrtIdx)
{
	struct _TWT_PLANNER_T *prTWTPlanner = &(prAdapter->rTWTPlanner);
	struct _TWT_AGRT_T *prTWTAgrt = NULL;

	if (ucAgrtIdx >= TWT_AGRT_MAX_NUM) {
		DBGLOG(TWT_PLANNER, ERROR,
			"TWT agrt %u doesn't exist\n", ucAgrtIdx);

		return ENUM_TWT_TYPE_NUM;
	}

	prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[ucAgrtIdx]);

	DBGLOG(TWT_PLANNER, WARN,
		"TWT agrt %u TwtType %d\n", ucAgrtIdx, prTWTAgrt->eTwtType);

	return prTWTAgrt->eTwtType;
}

uint32_t
twtPlannerDrvAgrtModify(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx, uint8_t ucFlowId,
	struct _NEXT_TWT_INFO_T *prNextTWTInfo,
	uint8_t *pucIdx, struct _TWT_PARAMS_T *prTWTParams)
{
	uint8_t ucIdx;
	uint64_t u8CurTsf;
	struct _TWT_PLANNER_T *prTWTPlanner = &(prAdapter->rTWTPlanner);
	uint32_t rStatus;
	uint8_t ucFlowId_real = ucFlowId;

	ucIdx = twtPlannerDrvAgrtFind(
		prAdapter, ucBssIdx, ucFlowId, &ucFlowId_real);
	if (ucIdx >= TWT_AGRT_MAX_NUM) {
		DBGLOG(TWT_PLANNER, ERROR, "Can't find agrt bss %u flow %u\n",
			ucBssIdx, ucFlowId);
		return WLAN_STATUS_FAILURE;
	}

	ucFlowId = ucFlowId_real;

	/* TODO: get current TSF from FW */
	u8CurTsf = 0;

	rStatus = _twtPlannerDrvAgrtModify(prTWTPlanner, prNextTWTInfo,
		u8CurTsf, ucIdx, prTWTParams);
	if (rStatus == WLAN_STATUS_SUCCESS)
		*pucIdx = ucIdx;

	return rStatus;
}

uint32_t
twtPlannerDrvAgrtGet(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx, uint8_t ucFlowId,
	uint8_t *pucIdx, struct _TWT_PARAMS_T *prTWTParams)
{
	uint8_t ucIdx;
	struct _TWT_PLANNER_T *prTWTPlanner = &(prAdapter->rTWTPlanner);
	uint32_t rStatus;
	uint8_t ucFlowId_real = ucFlowId;

	ucIdx = twtPlannerDrvAgrtFind(
		prAdapter, ucBssIdx, ucFlowId, &ucFlowId_real);
	if (ucIdx >= TWT_AGRT_MAX_NUM) {
		DBGLOG(TWT_PLANNER, ERROR, "Can't find agrt bss %u flow %u\n",
			ucBssIdx, ucFlowId);
		return WLAN_STATUS_FAILURE;
	}

	ucFlowId = ucFlowId_real;

	rStatus = _twtPlannerDrvAgrtGet(prTWTPlanner, ucIdx, prTWTParams);
	if (rStatus == WLAN_STATUS_SUCCESS)
		*pucIdx = ucIdx;

	return rStatus;
}

bool
twtPlannerIsDrvAgrtExisting(struct ADAPTER *prAdapter)
{
	bool ret = FALSE;
	uint8_t i;
	struct _TWT_PLANNER_T *prTWTPlanner = &(prAdapter->rTWTPlanner);
	struct _TWT_AGRT_T *prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[0]);

	for (i = 0; i < TWT_AGRT_MAX_NUM; i++, prTWTAgrt++) {
		if (prTWTAgrt->fgValid == TRUE) {
			ret = TRUE;
			break;
		}
	}

	return ret;
}

uint8_t twtPlannerDrvAgrtFindWithTwtType(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	uint8_t ucFlowId,
	uint8_t fgByPassNego,
	enum _ENUM_TWT_TYPE_T eTwtType)
{
	uint8_t i;
	struct _TWT_PLANNER_T *prTWTPlanner = &(prAdapter->rTWTPlanner);
	struct _TWT_AGRT_T *prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[0]);

	for (i = 0; i < TWT_AGRT_MAX_NUM; i++, prTWTAgrt++) {
		if ((prTWTAgrt->fgValid == TRUE) &&
			(prTWTAgrt->ucFlowId == ucFlowId) &&
			(prTWTAgrt->ucBssIdx == ucBssIdx) &&
			(prTWTAgrt->eTwtType == eTwtType) &&
			(prTWTAgrt->rTWTAgrt.fgByPassNego == fgByPassNego))
			break;
	}

	return i;
}

uint8_t twtPlannerDrvAgrtGetFlowID(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	uint8_t fgByPassNego,
	enum _ENUM_TWT_TYPE_T eTwtType)
{
	uint8_t i;
	uint8_t ucFlowID = TWT_INCORRECT_FLOW_ID;
	struct _TWT_PLANNER_T *prTWTPlanner = &(prAdapter->rTWTPlanner);
	struct _TWT_AGRT_T *prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[0]);

	for (i = 0; i < TWT_AGRT_MAX_NUM; i++, prTWTAgrt++) {
		if ((prTWTAgrt->fgValid == TRUE) &&
			(prTWTAgrt->ucBssIdx == ucBssIdx) &&
			(prTWTAgrt->eTwtType == eTwtType) &&
			(prTWTAgrt->rTWTAgrt.fgByPassNego == fgByPassNego)) {
			ucFlowID = prTWTAgrt->ucFlowId;

			break;
		}
	}

	return ucFlowID;
}

void twtPlannerInit(struct _TWT_PLANNER_T *pTWTPlanner)
{
	if (!pTWTPlanner) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid pTWTPlanner\n");

		return;
	}

	kalMemSet(&(pTWTPlanner->arTWTAgrtTbl[0]), 0,
		TWT_AGRT_MAX_NUM * sizeof(struct _TWT_AGRT_T));
}

struct _TWT_FLOW_T *twtPlannerFlowFindById(
	struct STA_RECORD *prStaRec, uint8_t ucFlowId,
	enum _ENUM_TWT_TYPE_T eTwtType)
{
	struct _TWT_FLOW_T *prTWTFlow = NULL;

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return NULL;
	}

	if ((eTwtType == ENUM_TWT_TYPE_RTWT) ||
		(eTwtType == ENUM_TWT_TYPE_BTWT)) {
		if (ucFlowId >= RTWT_MAX_FLOW_NUM) {
			DBGLOG(TWT_PLANNER, ERROR, "Invalid RTWT flow id %d\n",
			ucFlowId);

			return NULL;
		}
	} else {
		if (ucFlowId >= TWT_MAX_FLOW_NUM) {
			DBGLOG(TWT_PLANNER, ERROR, "Invalid TWT flow id %d\n",
				ucFlowId);

			return NULL;
		}
	}

	switch (eTwtType) {
#if (CFG_SUPPORT_BTWT == 1)
	case ENUM_TWT_TYPE_BTWT:
		prTWTFlow = &(prStaRec->arBTWTFlow[ucFlowId]);

		break;
#endif

#if (CFG_SUPPORT_RTWT == 1)
	case ENUM_TWT_TYPE_RTWT:
		prTWTFlow = &(prStaRec->arRTWTFlow[ucFlowId]);

		break;
#endif

	/* ITWT/MLTWT/local emulate */
	case ENUM_TWT_TYPE_DEFAULT:
	case ENUM_TWT_TYPE_ITWT:
	case ENUM_TWT_TYPE_MLTWT:
		prTWTFlow = &(prStaRec->arTWTFlow[ucFlowId]);

		break;

	/* If we reach here, it should be an incorrect TWT type */
	default:
		DBGLOG(TWT_PLANNER, ERROR, "Invalid TWT type %d\n",
			eTwtType);

		break;
	}

	return prTWTFlow;
}

static uint32_t
twtPlannerSendReqStart(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId)
{
	struct _MSG_TWT_REQFSM_START_T *prTWTReqFsmStartMsg;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prTWTReqFsmStartMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_START_T));
	if (prTWTReqFsmStartMsg) {
		prTWTReqFsmStartMsg->rMsgHdr.eMsgId = MID_TWT_REQ_FSM_START;
		prTWTReqFsmStartMsg->prStaRec = prStaRec;
		prTWTReqFsmStartMsg->ucTWTFlowId = ucTWTFlowId;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prTWTReqFsmStartMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

uint32_t twtPlannerSendReqTeardown(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId)
{
	struct _MSG_TWT_REQFSM_TEARDOWN_T *prTWTReqFsmTeardownMsg;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prTWTReqFsmTeardownMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_TEARDOWN_T));
	if (prTWTReqFsmTeardownMsg) {
		prTWTReqFsmTeardownMsg->rMsgHdr.eMsgId =
			MID_TWT_REQ_FSM_TEARDOWN;
		prTWTReqFsmTeardownMsg->prStaRec = prStaRec;
		prTWTReqFsmTeardownMsg->ucTWTFlowId = ucTWTFlowId;
		prTWTReqFsmTeardownMsg->fgTeardownAll = 0;

		mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prTWTReqFsmTeardownMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

uint32_t
twtPlannerSendReqSuspend(struct ADAPTER *prAdapter,
				struct STA_RECORD *prStaRec,
				uint8_t ucTWTFlowId)
{
	struct _MSG_TWT_REQFSM_SUSPEND_T *prTWTReqFsmSuspendMsg;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prTWTReqFsmSuspendMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_SUSPEND_T));
	if (prTWTReqFsmSuspendMsg) {
		prTWTReqFsmSuspendMsg->rMsgHdr.eMsgId =
			MID_TWT_REQ_FSM_SUSPEND;
		prTWTReqFsmSuspendMsg->prStaRec = prStaRec;
		prTWTReqFsmSuspendMsg->ucTWTFlowId = ucTWTFlowId;

		mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prTWTReqFsmSuspendMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

static uint32_t
twtPlannerSendReqResume(struct ADAPTER *prAdapter,
				struct STA_RECORD *prStaRec,
				uint8_t ucTWTFlowId,
				uint64_t u8NextTWT,
				uint8_t ucNextTWTSize)
{
	struct _MSG_TWT_REQFSM_RESUME_T *prTWTReqFsmResumeMsg;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prTWTReqFsmResumeMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_RESUME_T));
	if (prTWTReqFsmResumeMsg) {
		prTWTReqFsmResumeMsg->rMsgHdr.eMsgId =
			MID_TWT_REQ_FSM_RESUME;
		prTWTReqFsmResumeMsg->prStaRec = prStaRec;
		prTWTReqFsmResumeMsg->ucTWTFlowId = ucTWTFlowId;
		prTWTReqFsmResumeMsg->u8NextTWT = u8NextTWT;
		prTWTReqFsmResumeMsg->ucNextTWTSize = ucNextTWTSize;

		DBGLOG(TWT_REQUESTER, WARN,
			"TWT Info Frame 0x%llx 0x%llx\n",
			prTWTReqFsmResumeMsg->u8NextTWT,
			u8NextTWT);

		mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prTWTReqFsmResumeMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

static uint32_t
twtPlannerAddAgrtTbl(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct _TWT_PARAMS_T *prTWTParams,
	uint8_t ucFlowId,
	enum _ENUM_TWT_TYPE_T eTwtType,
	uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	uint8_t ucAgrtTblIdx;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *prTWTAgrtUpdate;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (prBssInfo == NULL) {
		DBGLOG(TWT_PLANNER, ERROR, "No bssinfo to add agrt\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (twtPlannerDrvAgrtFind(
				prAdapter,
				prBssInfo->ucBssIndex,
				ucFlowId, NULL) < TWT_AGRT_MAX_NUM) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Agreement table Bss idx %d Flow ID %d exists\n",
			prBssInfo->ucBssIndex,
			ucFlowId);

		return WLAN_STATUS_FAILURE;
	}

	rWlanStatus = twtPlannerDrvAgrtAdd(prAdapter, prBssInfo->ucBssIndex,
				ucFlowId, prTWTParams, &ucAgrtTblIdx,
				eTwtType);

	if (rWlanStatus) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Agreement table is full\n");
		return WLAN_STATUS_FAILURE;
	}

	prTWTAgrtUpdate = cnmMemAlloc(
		prAdapter,
		RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T));
	if (!prTWTAgrtUpdate) {
		DBGLOG(
		TWT_PLANNER,
		ERROR,
		"Allocate _EXT_CMD_TWT_ARGT_UPDATE_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prTWTAgrtUpdate->ucAgrtTblIdx = ucAgrtTblIdx;
	prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_ADD;
	prTWTAgrtUpdate->ucOwnMacId = prBssInfo->ucOwnMacIndex;
	prTWTAgrtUpdate->ucFlowId = ucFlowId;
	prTWTAgrtUpdate->u2PeerIdGrpId =
			CPU_TO_LE16(prStaRec->ucWlanIndex);
	prTWTAgrtUpdate->ucAgrtSpDuration = prTWTParams->ucMinWakeDur;
	prTWTAgrtUpdate->ucBssIndex = prBssInfo->ucBssIndex;
	prTWTAgrtUpdate->u4AgrtSpStartTsfLow =
		CPU_TO_LE32(prTWTParams->u8TWT & 0xFFFFFFFF);
	prTWTAgrtUpdate->u4AgrtSpStartTsfHigh =
		CPU_TO_LE32((uint32_t)(prTWTParams->u8TWT >> 32));
	prTWTAgrtUpdate->u2AgrtSpWakeIntvlMantissa =
		CPU_TO_LE16(prTWTParams->u2WakeIntvalMantiss);
	prTWTAgrtUpdate->ucAgrtSpWakeIntvlExponent =
		prTWTParams->ucWakeIntvalExponent;
	/* STA role */
	prTWTAgrtUpdate->ucIsRoleAp =
			prTWTParams->fgByPassNego ?
				TWT_ROLE_STA_LOCAL_EMU : TWT_ROLE_STA;

	prTWTAgrtUpdate->ucAgrtParaBitmap =
	((prTWTParams->fgProtect << TWT_AGRT_PARA_BITMAP_PROTECT_OFFSET) |
	((!prTWTParams->fgUnannounced) << TWT_AGRT_PARA_BITMAP_ANNCE_OFFSET) |
	(prTWTParams->fgTrigger << TWT_AGRT_PARA_BITMAP_TRIGGER_OFFSET));

	prTWTAgrtUpdate->ucGrpMemberCnt = 0;

#if (CFG_SUPPORT_RTWT == 1)
	/* RTWT traffic info */
	prTWTAgrtUpdate->ucTrafficInfoPresent =
				prTWTParams->ucTrafficInfoPresent;
	prTWTAgrtUpdate->ucDlUlBmpValid = prTWTParams->ucDlUlBmpValid;
	prTWTAgrtUpdate->ucDlBmp = prTWTParams->ucDlBmp;
	prTWTAgrtUpdate->ucUlBmp = prTWTParams->ucUlBmp;
#endif

#if (CFG_SUPPORT_BTWT == 1)
	if ((ucFlowId == 0) && (prStaRec->arBTWTFlow[ucFlowId].eTwtType
		== ENUM_TWT_TYPE_BTWT)) {
		if (g_IsTwtLogo == 1) {
			prTWTAgrtUpdate->ucReserved_a = 0xAB;
		}
	}
#endif

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
				CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				EXT_CMD_ID_TWT_AGRT_UPDATE,
				TRUE,
				FALSE,
				fgIsOid,
				pfCmdDoneHandler,
				pfCmdTimeoutHandler,
				sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
				(uint8_t *) (prTWTAgrtUpdate),
				NULL, 0);

#if (CFG_TWT_SMART_STA == 1)
	if (g_TwtSmartStaCtrl.fgTwtSmartStaReq == TRUE) {
		g_TwtSmartStaCtrl.fgTwtSmartStaActivated = TRUE;
		g_TwtSmartStaCtrl.ucFlowId = ucFlowId;
		g_TwtSmartStaCtrl.ucBssIndex
			= prBssInfo->ucBssIndex;
		g_TwtSmartStaCtrl.eState
			= TWT_SMART_STA_STATE_SUCCESS;
	}
#endif

	cnmMemFree(prAdapter, prTWTAgrtUpdate);

#if (CFG_SUPPORT_BTWT == 1)
	if (prStaRec->arBTWTFlow[ucFlowId].eTwtType == ENUM_TWT_TYPE_BTWT) {
		prStaRec->arBTWTFlow[ucFlowId].eBtwtState
			= ENUM_BTWT_FLOW_STATE_ACTIVATED;
	}
#endif

	return rWlanStatus;
}

uint32_t
twtPlannerResumeAgrtTbl(struct ADAPTER *prAdapter,
			struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
			uint8_t ucFlowId, uint8_t fgIsOid,
			PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
			PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	uint8_t ucAgrtTblIdx;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *prTWTAgrtUpdate;
	struct _TWT_PARAMS_T rTWTParams;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (prBssInfo == NULL) {
		DBGLOG(TWT_PLANNER, ERROR, "No bssinfo to resume agrt\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	rWlanStatus = twtPlannerDrvAgrtGet(prAdapter, prBssInfo->ucBssIndex,
		ucFlowId, &ucAgrtTblIdx, &rTWTParams);
	if (rWlanStatus) {
		DBGLOG(TWT_PLANNER, ERROR, "No agrt to resume Bss %u flow %u\n",
			prBssInfo->ucBssIndex, ucFlowId);
		return WLAN_STATUS_FAILURE;
	}

	prTWTAgrtUpdate = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T));
	if (!prTWTAgrtUpdate) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Allocate _EXT_CMD_TWT_ARGT_UPDATE_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prTWTAgrtUpdate->ucAgrtTblIdx = ucAgrtTblIdx;
	prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_SUSPEND_RESUME;
	prTWTAgrtUpdate->ucOwnMacId = prBssInfo->ucOwnMacIndex;
	prTWTAgrtUpdate->ucFlowId = ucFlowId;
	prTWTAgrtUpdate->u2PeerIdGrpId = CPU_TO_LE16(prStaRec->ucWlanIndex);
	prTWTAgrtUpdate->ucAgrtSpDuration = rTWTParams.ucMinWakeDur;
	prTWTAgrtUpdate->ucBssIndex = prBssInfo->ucBssIndex;
	prTWTAgrtUpdate->u4AgrtSpStartTsfLow =
		CPU_TO_LE32(rTWTParams.u8TWT & 0xFFFFFFFF);
	prTWTAgrtUpdate->u4AgrtSpStartTsfHigh =
		CPU_TO_LE32((uint32_t)(rTWTParams.u8TWT >> 32));
	prTWTAgrtUpdate->u2AgrtSpWakeIntvlMantissa =
		CPU_TO_LE16(rTWTParams.u2WakeIntvalMantiss);
	prTWTAgrtUpdate->ucAgrtSpWakeIntvlExponent =
		rTWTParams.ucWakeIntvalExponent;
	prTWTAgrtUpdate->ucIsRoleAp = 0;  /* STA role */
	prTWTAgrtUpdate->ucAgrtParaBitmap =
	    ((rTWTParams.fgProtect << TWT_AGRT_PARA_BITMAP_PROTECT_OFFSET) |
	    ((!rTWTParams.fgUnannounced) << TWT_AGRT_PARA_BITMAP_ANNCE_OFFSET) |
	    (rTWTParams.fgTrigger << TWT_AGRT_PARA_BITMAP_TRIGGER_OFFSET));
	prTWTAgrtUpdate->ucGrpMemberCnt = 0;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_TWT_AGRT_UPDATE,
			TRUE,
			FALSE,
			fgIsOid,
			pfCmdDoneHandler,
			pfCmdTimeoutHandler,
			sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
			(uint8_t *) (prTWTAgrtUpdate),
			NULL, 0);

	cnmMemFree(prAdapter, prTWTAgrtUpdate);

	return rWlanStatus;
}

static uint32_t
twtPlannerModifyAgrtTbl(struct ADAPTER *prAdapter,
			struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
			struct _NEXT_TWT_INFO_T *prNextTWTInfo,
			uint8_t ucFlowId, uint8_t fgIsOid,
			PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
			PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	uint8_t ucAgrtTblIdx;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *prTWTAgrtUpdate;
	struct _TWT_PARAMS_T rTWTParams;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (prBssInfo == NULL) {
		DBGLOG(TWT_PLANNER, ERROR, "No bssinfo to modify agrt\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Handle driver agreement table */
	rWlanStatus = twtPlannerDrvAgrtModify(prAdapter, prBssInfo->ucBssIndex,
		ucFlowId, prNextTWTInfo, &ucAgrtTblIdx, &rTWTParams);

	if (rWlanStatus) {
		DBGLOG(TWT_PLANNER, ERROR, "No agrt to modify Bss %u flow %u\n",
			prBssInfo->ucBssIndex, ucFlowId);
		return WLAN_STATUS_FAILURE;
	}

	/* Handle FW agreement table */
	prTWTAgrtUpdate = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T));
	if (!prTWTAgrtUpdate) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Allocate _EXT_CMD_TWT_ARGT_UPDATE_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prTWTAgrtUpdate->ucAgrtTblIdx = ucAgrtTblIdx;
	prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_MODIFY;
	prTWTAgrtUpdate->ucOwnMacId = prBssInfo->ucOwnMacIndex;
	prTWTAgrtUpdate->ucFlowId = ucFlowId;
	prTWTAgrtUpdate->u2PeerIdGrpId =
			CPU_TO_LE16(prStaRec->ucWlanIndex);
	prTWTAgrtUpdate->ucAgrtSpDuration = rTWTParams.ucMinWakeDur;
	prTWTAgrtUpdate->u4AgrtSpStartTsfLow =
		CPU_TO_LE32(rTWTParams.u8TWT & 0xFFFFFFFF);
	prTWTAgrtUpdate->u4AgrtSpStartTsfHigh =
		CPU_TO_LE32((uint32_t)(rTWTParams.u8TWT >> 32));
	prTWTAgrtUpdate->ucGrpMemberCnt = 0;
	prTWTAgrtUpdate->ucBssIndex = prBssInfo->ucBssIndex;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_TWT_AGRT_UPDATE,
			TRUE,
			FALSE,
			fgIsOid,
			pfCmdDoneHandler,
			pfCmdTimeoutHandler,
			sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
			(uint8_t *) (prTWTAgrtUpdate),
			NULL, 0);

	cnmMemFree(prAdapter, prTWTAgrtUpdate);

	return rWlanStatus;
}

static uint32_t
twtPlannerDelAgrtTbl(struct ADAPTER *prAdapter,
			struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
			uint8_t ucFlowId, uint8_t fgIsOid,
			PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
			PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
			uint8_t fgDelDrvEntry)
{
	uint8_t ucAgrtTblIdx;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _TWT_PLANNER_T *prTWTPlanner = &(prAdapter->rTWTPlanner);
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *prTWTAgrtUpdate;
	uint8_t ucFlowId_real = ucFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (prBssInfo == NULL) {
		DBGLOG(TWT_PLANNER, ERROR, "No bssinfo to delete agrt\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Find and delete the agreement entry in the driver */
	ucAgrtTblIdx = twtPlannerDrvAgrtFind(prAdapter,
		prBssInfo->ucBssIndex, ucFlowId, &ucFlowId_real);

	if (ucAgrtTblIdx >= TWT_AGRT_MAX_NUM) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Cannot find the flow %u to be deleted\n", ucFlowId);
		return WLAN_STATUS_FAILURE;

	}

	ucFlowId = ucFlowId_real;

	if (fgDelDrvEntry)
		_twtPlannerDrvAgrtDel(prTWTPlanner, ucAgrtTblIdx);

	/* Send cmd to delete agreement entry in FW */
	prTWTAgrtUpdate = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T));
	if (!prTWTAgrtUpdate) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Alloc _EXT_CMD_TWT_ARGT_UPDATE_T for del FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prTWTAgrtUpdate->ucAgrtTblIdx = ucAgrtTblIdx;
	prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_DELETE;
	prTWTAgrtUpdate->ucOwnMacId =
				prBssInfo->ucOwnMacIndex;
	prTWTAgrtUpdate->ucFlowId = ucFlowId;
	prTWTAgrtUpdate->u2PeerIdGrpId =
		CPU_TO_LE16(prStaRec->ucWlanIndex);
	prTWTAgrtUpdate->ucIsRoleAp = 0;  /* STA role */
	prTWTAgrtUpdate->ucBssIndex = prBssInfo->ucBssIndex;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_TWT_AGRT_UPDATE,
			TRUE,
			FALSE,
			fgIsOid,
			pfCmdDoneHandler,
			pfCmdTimeoutHandler,
			sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
			(uint8_t *) (prTWTAgrtUpdate),
			NULL, 0);

	cnmMemFree(prAdapter, prTWTAgrtUpdate);

	return WLAN_STATUS_SUCCESS;
}

static uint32_t
twtPlannerTeardownAgrtTbl(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec,
			uint8_t fgIsOid,
			PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
			PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *prTWTAgrtUpdate;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Send cmd to teardown this STA in FW */
	prTWTAgrtUpdate = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T));
	if (!prTWTAgrtUpdate) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Alloc _EXT_CMD_TWT_ARGT_UPDATE_T for del FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	/* Don't care about other fields of the cmd */
	prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_TEARDOWN;
	prTWTAgrtUpdate->u2PeerIdGrpId =
		CPU_TO_LE16(prStaRec->ucWlanIndex);
	prTWTAgrtUpdate->ucIsRoleAp = 0;  /* STA role */
	prTWTAgrtUpdate->ucBssIndex = prStaRec->ucBssIndex;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_TWT_AGRT_UPDATE,
			TRUE,
			FALSE,
			fgIsOid,
			pfCmdDoneHandler,
			pfCmdTimeoutHandler,
			sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
			(uint8_t *) (prTWTAgrtUpdate),
			NULL, 0);

	cnmMemFree(prAdapter, prTWTAgrtUpdate);

	return rWlanStatus;
}


static uint32_t
twtPlannerSuspendAgrtTbl(struct ADAPTER *prAdapter,
			struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
			uint8_t ucFlowId, uint8_t fgIsOid,
			PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
			PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	uint8_t ucAgrtTblIdx;
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *prTWTAgrtUpdate;
	struct _TWT_PARAMS_T rTWTParams;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	rWlanStatus = twtPlannerDrvAgrtGet(prAdapter, prBssInfo->ucBssIndex,
		ucFlowId, &ucAgrtTblIdx, &rTWTParams);

	if (rWlanStatus) {
		DBGLOG(TWT_PLANNER, ERROR,
			"No agrt to suspend Bss %u flow %u\n",
			prBssInfo->ucBssIndex, ucFlowId);
		return WLAN_STATUS_FAILURE;
	}

	/* Send cmd to delete agreement entry in FW */
	prTWTAgrtUpdate = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T));

	if (!prTWTAgrtUpdate) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Alloc _EXT_CMD_TWT_ARGT_UPDATE_T for suspend.\n");
		return WLAN_STATUS_FAILURE;
	}

	prTWTAgrtUpdate->ucAgrtTblIdx = ucAgrtTblIdx;
	prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_SUSPEND;
	prTWTAgrtUpdate->ucOwnMacId = prBssInfo->ucOwnMacIndex;
	prTWTAgrtUpdate->ucFlowId = ucFlowId;
	prTWTAgrtUpdate->u2PeerIdGrpId = CPU_TO_LE16(prStaRec->ucWlanIndex);
	prTWTAgrtUpdate->ucIsRoleAp = 0;  /* STA role */
	prTWTAgrtUpdate->ucBssIndex = prBssInfo->ucBssIndex;
	prTWTAgrtUpdate->ucAgrtParaBitmap =
	    ((rTWTParams.fgProtect << TWT_AGRT_PARA_BITMAP_PROTECT_OFFSET) |
	    ((!rTWTParams.fgUnannounced) << TWT_AGRT_PARA_BITMAP_ANNCE_OFFSET) |
	    (rTWTParams.fgTrigger << TWT_AGRT_PARA_BITMAP_TRIGGER_OFFSET));

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_TWT_AGRT_UPDATE,
			TRUE,
			FALSE,
			fgIsOid,
			pfCmdDoneHandler,
			pfCmdTimeoutHandler,
			sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
			(uint8_t *) (prTWTAgrtUpdate),
			NULL, 0);

	cnmMemFree(prAdapter, prTWTAgrtUpdate);

	return rWlanStatus;
}

uint32_t twtPlannerReset(
	struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct STA_RECORD *prStaRec = NULL;
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *prTWTAgrtUpdate = NULL;
	uint8_t i;
	struct _TWT_PLANNER_T *prTWTPlanner = NULL;
	struct _TWT_AGRT_T *prTWTAgrt = NULL;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* If no agrt exits, don't bother resetting */
	if (twtPlannerIsDrvAgrtExisting(prAdapter) == FALSE)
		return rWlanStatus;

	prTWTPlanner = &(prAdapter->rTWTPlanner);
	prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[0]);

	/* For the case we are deauthed, we should terminate all TWTs */
	for (i = 0; i < TWT_AGRT_MAX_NUM; i++, prTWTAgrt++) {
		if (prTWTAgrt->fgValid == FALSE)
			continue;

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
						prTWTAgrt->ucBssIdx);

		/* send cmd to reset FW agreement table */
		if (!prBssInfo) {
			DBGLOG(TWT_PLANNER, ERROR,
					"Invalid prBssInfo\n");

			return WLAN_STATUS_INVALID_DATA;
		}

		prStaRec = prBssInfo->prStaRecOfAP;

		if (!prStaRec) {
			DBGLOG(TWT_PLANNER, ERROR,
				"Invalid prStaRec\n");

			return WLAN_STATUS_INVALID_DATA;
		}

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
		twtReqFsmTeardownTimeoutDeInit(
			prAdapter,
			prStaRec,
			prTWTAgrt->ucFlowId,
			&prTWTAgrt->eTwtType);
#endif

		prTWTAgrtUpdate = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
				sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T));

		if (!prTWTAgrtUpdate) {
			DBGLOG(TWT_PLANNER, ERROR,
				"Alloc _EXT_CMD_TWT_ARGT_UPDATE_T[%d] for reset FAILED.\n",
				i);

			return WLAN_STATUS_FAILURE;
		}

		prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_RESET;
		prTWTAgrtUpdate->u2PeerIdGrpId =
					CPU_TO_LE16(prStaRec->ucWlanIndex);
		prTWTAgrtUpdate->ucIsRoleAp = 0;  /* STA role */
		prTWTAgrtUpdate->ucBssIndex =
						prBssInfo->ucBssIndex;

		rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_TWT_AGRT_UPDATE,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
			(uint8_t *) (prTWTAgrtUpdate),
			NULL, 0);

		cnmMemFree(prAdapter, prTWTAgrtUpdate);
	}

	/* reset driver agreement table */
	memset(&(prAdapter->rTWTPlanner), 0, sizeof(prAdapter->rTWTPlanner));

	/* Enable scan after TWT agrt reset */
	prAdapter->fgEnOnlineScan = TRUE;

#if (CFG_TWT_SMART_STA == 1)
	g_TwtSmartStaCtrl.fgTwtSmartStaActivated = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaReq = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaTeardownReq = FALSE;
	g_TwtSmartStaCtrl.ucBssIndex = 0;
	g_TwtSmartStaCtrl.ucFlowId = 0;
	g_TwtSmartStaCtrl.u4CurTp = 0;
	g_TwtSmartStaCtrl.u4LastTp = 0;
	g_TwtSmartStaCtrl.u4TwtSwitch = 0;
	g_TwtSmartStaCtrl.eState = TWT_SMART_STA_STATE_IDLE;
#endif

	return rWlanStatus;
}

#if (CFG_TWT_STA_DIRECT_TEARDOWN == 1)
void twtPlannerTearingdown(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId,
	uint8_t *p_fgByPassNego)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucAgrtTblIdx;
	struct _TWT_PARAMS_T rTWTParams;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _TWT_FLOW_T *prTWTFlow;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (prBssInfo == NULL) {
		DBGLOG(TWT_PLANNER, ERROR, "No bssinfo to teardown\n");

		return;
	}

	rWlanStatus = twtPlannerDrvAgrtGet(
					prAdapter, prBssInfo->ucBssIndex,
					ucFlowId, &ucAgrtTblIdx, &rTWTParams);

	if (rWlanStatus) {
		DBGLOG(TWT_PLANNER, ERROR,
			"No agrt to suspend Bss %u flow %u\n",
			prBssInfo->ucBssIndex, ucFlowId);

		*p_fgByPassNego = FALSE;

		return;
	}

	*p_fgByPassNego = rTWTParams.fgByPassNego;

#if (CFG_TWT_SMART_STA == 1)
	g_TwtSmartStaCtrl.fgTwtSmartStaActivated = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaReq = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaTeardownReq = FALSE;
	g_TwtSmartStaCtrl.ucBssIndex = 0;
	g_TwtSmartStaCtrl.ucFlowId = 0;
	g_TwtSmartStaCtrl.u4CurTp = 0;
	g_TwtSmartStaCtrl.u4LastTp = 0;
	g_TwtSmartStaCtrl.u4TwtSwitch = 0;
	g_TwtSmartStaCtrl.eState = TWT_SMART_STA_STATE_IDLE;
#endif

	prTWTFlow = &(prStaRec->arTWTFlow[ucFlowId]);

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	if (prTWTFlow->fgIsMLTWT == TRUE) {
		/* MLTWT teardown goes over here */
		mltwtPlannerDelAgrtTbl(
			prAdapter,
			prStaRec,
			ucFlowId);

		return;
	}

	/* i-TWT teardown goes in existing flow */
#endif

	/* Delete driver & FW TWT agreement entry */
	rWlanStatus = twtPlannerDelAgrtTbl(prAdapter,
		prBssInfo, prStaRec, ucFlowId, FALSE,
		NULL, NULL /* handle TWT cmd timeout? */, TRUE);

	/* Teardown FW TWT agreement entry */
	if (rWlanStatus == WLAN_STATUS_SUCCESS)
		twtPlannerTeardownAgrtTbl(prAdapter,
			prStaRec, FALSE, NULL,
			NULL /* handle TWT cmd timeout? */);

	prTWTFlow->eTwtType = ENUM_TWT_TYPE_DEFAULT;
}
#endif

uint64_t twtPlannerAdjustNextTWT(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx, uint8_t ucFlowId,
	uint64_t u8NextTWTOrig)
{
	uint8_t ucAgrtTblIdx;
	struct _TWT_PARAMS_T rTWTParams = {0x0};
	uint64_t u8Diff;
	uint32_t u4WakeIntvl;

	twtPlannerDrvAgrtGet(prAdapter, ucBssIdx, ucFlowId,
		&ucAgrtTblIdx, &rTWTParams);

	u4WakeIntvl = rTWTParams.u2WakeIntvalMantiss <<
		rTWTParams.ucWakeIntvalExponent;
	u8Diff = u8NextTWTOrig - rTWTParams.u8TWT;

	return (rTWTParams.u8TWT +
		(kal_div_u64(u8Diff, u4WakeIntvl) + 1) * u4WakeIntvl);
}

void twtPlannerGetTsfDone(
	struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf)
{
	struct EXT_EVENT_MAC_INFO_T *prEventMacInfo;
	struct _TWT_GET_TSF_CONTEXT_T *prGetTsfCtxt;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct TSF_RESULT_T *prTsfResult;
	uint64_t u8CurTsf = 0;
	uint64_t u8Temp = 0;
	uint64_t u8twt_interval = 0;
	uint64_t u8Mod = 0;
	uint8_t ucNextTWTSize = 0;
	uint64_t u8NextTWT = 0;
	struct _TWT_PARAMS_T *prTWTParams = NULL;
	struct _TWT_FLOW_T *prTWTFlow = NULL;
#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
#endif
#if (CFG_SUPPORT_RTWT == 1)
	uint8_t ucFlowID = TWT_INCORRECT_FLOW_ID;
#endif
#if (CFG_SUPPORT_TWT_STA_CNM == 1)
	enum _ENUM_TWT_TYPE_T eTwtType;
#endif

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prCmdInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	if (!pucEventBuf) {
		DBGLOG(TWT_PLANNER, ERROR, "pucEventBuf is NULL.\n");
		return;
	}

	if (!prCmdInfo->pvInformationBuffer) {
		DBGLOG(TWT_PLANNER, ERROR,
			"prCmdInfo->pvInformationBuffer is NULL.\n");
		return;
	}

	prEventMacInfo = (struct EXT_EVENT_MAC_INFO_T *) (pucEventBuf);
	prGetTsfCtxt = (struct _TWT_GET_TSF_CONTEXT_T *)
		prCmdInfo->pvInformationBuffer;

	if (!prGetTsfCtxt) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prGetTsfCtxt\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prGetTsfCtxt->ucBssIdx);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return;
	}

	prStaRec = prBssInfo->prStaRecOfAP;

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return;
	}

	prTsfResult = &(prEventMacInfo->rMacInfoResult.rTsfResult);
	u8CurTsf = LE32_TO_CPU(prTsfResult->u4TsfBitsLow) |
		(((uint64_t)(LE32_TO_CPU(prTsfResult->u4TsfBitsHigh))) << 32);

	switch (prGetTsfCtxt->ucReason) {
	case TWT_GET_TSF_FOR_ADD_AGRT_BYPASS:
		u8twt_interval = ((u_int64_t)
			(prGetTsfCtxt->rTWTParams.u2WakeIntvalMantiss))
			<< prGetTsfCtxt->rTWTParams.ucWakeIntvalExponent;
		u8Temp = u8CurTsf + u8twt_interval;

		if ((u8twt_interval == 0) ||
			(prGetTsfCtxt->ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
			DBGLOG(TWT_PLANNER, ERROR,
				"Flow ID %d u8twt_interval 0x%x\n",
				prGetTsfCtxt->ucTWTFlowId,
				CPU_TO_LE32(u8twt_interval & 0xFFFFFFFF));

			kalMemFree(prGetTsfCtxt,
				VIR_MEM_TYPE, sizeof(*prGetTsfCtxt));

			return;
		}

		u8Mod = kal_mod64(u8Temp, u8twt_interval);

		prGetTsfCtxt->rTWTParams.u8TWT =
				u8CurTsf + u8twt_interval - u8Mod;

		twtPlannerAddAgrtTbl(prAdapter, prBssInfo,
				prStaRec, &(prGetTsfCtxt->rTWTParams),
				prGetTsfCtxt->ucTWTFlowId,
				ENUM_TWT_TYPE_DEFAULT,
				prGetTsfCtxt->fgIsOid,
				NULL, NULL);

		break;

	case TWT_GET_TSF_FOR_ADD_AGRT:
		prTWTFlow = twtPlannerFlowFindById(
					prStaRec, prGetTsfCtxt->ucTWTFlowId,
					ENUM_TWT_TYPE_ITWT);

#ifdef CFG_SUPPORT_TWT_EXT
		DBGLOG(TWT_PLANNER, WARN,
			"TWT cur TSF: 0x%x 0x%x 0x%x\n",
			CPU_TO_LE32(u8CurTsf & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(u8CurTsf >> 32)),
			prGetTsfCtxt->rTWTParams.u4DesiredWakeTime);
		u8CurTsf += prGetTsfCtxt->rTWTParams.u4DesiredWakeTime;
#endif

		/* To have mantissa alignment::Begin */
		u8twt_interval = ((u_int64_t)
			(prGetTsfCtxt->rTWTParams.u2WakeIntvalMantiss))
			<< prGetTsfCtxt->rTWTParams.ucWakeIntvalExponent;

		u8Temp = u8CurTsf + u8twt_interval;

		DBGLOG(TWT_PLANNER, WARN,
			"u8twt_interval: 0x%x 0x%x u8Temp 0x%x 0x%x\n",
			CPU_TO_LE32(u8twt_interval & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(u8twt_interval >> 32)),
			CPU_TO_LE32(u8Temp & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(u8Temp >> 32)));

		if ((prTWTFlow == NULL) || (u8twt_interval == 0)) {
			DBGLOG(TWT_PLANNER, ERROR,
				"prTWTFlow %d NULL(%s)\nu8twt_interval 0x%x\n",
				prGetTsfCtxt->ucTWTFlowId,
				(prTWTFlow == NULL) ? "Y" : "N",
				CPU_TO_LE32(u8twt_interval & 0xFFFFFFFF));

			kalMemFree(prGetTsfCtxt,
				VIR_MEM_TYPE, sizeof(*prGetTsfCtxt));

			return;
		}

		u8Mod = kal_mod64(u8Temp, u8twt_interval);

		prGetTsfCtxt->rTWTParams.u8TWT =
				u8CurTsf + u8twt_interval - u8Mod;

		DBGLOG(TWT_PLANNER, WARN,
			"TWT cur TSF: 0x%x 0x%x TWT req TSF 0x%x 0x%x\n",
			CPU_TO_LE32(u8CurTsf & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(u8CurTsf >> 32)),
			CPU_TO_LE32(prGetTsfCtxt->rTWTParams.u8TWT &
				0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(prGetTsfCtxt->rTWTParams.u8TWT
				>> 32)));

		DBGLOG(TWT_PLANNER, WARN,
			"u8Mod 0x%x 0x%x\n",
			CPU_TO_LE32(u8Mod & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(u8Mod >> 32)));
		/* To have mantissa alignment::End */

		prTWTParams = &(prTWTFlow->rTWTParams);

		kalMemCopy(prTWTParams, &(prGetTsfCtxt->rTWTParams),
			sizeof(struct _TWT_PARAMS_T));

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
		/*
		 * To de-init CNM abort timeout for get current TSF
		 */
		twtGetCurrentTsfTimeoutDeInit(
			prAdapter,
			prStaRec,
			prGetTsfCtxt->ucReason);

		/*
		 * To setup CNM abort timer in case AP no resp
		 */
		eTwtType = ENUM_TWT_TYPE_ITWT;

		twtReqFsmSetupTimeoutInit(
			prAdapter,
			prStaRec,
			TWT_CNM_STATE_DEFAULT,
			prGetTsfCtxt->ucTWTFlowId,
			&eTwtType);
#endif

		/* Start the process to nego for a new agreement */
		twtPlannerSendReqStart(prAdapter,
			prStaRec, prGetTsfCtxt->ucTWTFlowId);

		break;

	case TWT_GET_TSF_FOR_RESUME_AGRT:
		ucNextTWTSize = prGetTsfCtxt->rNextTWT.ucNextTWTSize;

		/* To have mantissa alignment from TWT wake time::Begin */
		prTWTFlow = twtPlannerFlowFindById(
					prStaRec,
					prGetTsfCtxt->ucTWTFlowId,
					ENUM_TWT_TYPE_ITWT);

		if (prTWTFlow == NULL) {
			DBGLOG(TWT_PLANNER, ERROR, "prTWTFlow is NULL.\n");

			return;
		}

		prTWTParams = &(prTWTFlow->rTWTPeerParams);

		u8twt_interval = ((u_int64_t)(prTWTParams->u2WakeIntvalMantiss))
			<< prTWTParams->ucWakeIntvalExponent;

		u8Temp = u8CurTsf +
			prGetTsfCtxt->rNextTWT.u8NextTWT +
			u8twt_interval - prTWTParams->u8TWT;

		DBGLOG(TWT_PLANNER, WARN,
			"TWT Info Frame[0] TWT resp 0x%x 0x%x u8Temp 0x%x 0x%x\n",
			CPU_TO_LE32(prTWTParams->u8TWT & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(prTWTParams->u8TWT >> 32)),
			CPU_TO_LE32(u8Temp & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(u8Temp >> 32)));

		u8Mod = kal_mod64(u8Temp, u8twt_interval);

		u8NextTWT = u8CurTsf +
			prGetTsfCtxt->rNextTWT.u8NextTWT +
			u8twt_interval - u8Mod;

		DBGLOG(TWT_PLANNER, WARN,
			"TWT Info Frame[1] u8Mod 0x%x 0x%x\n",
				CPU_TO_LE32(u8Mod & 0xFFFFFFFF),
				CPU_TO_LE32((uint32_t)(u8Mod >> 32)));
		/* To have mantissa alignment from TWT wake time::End */

		if (((u8NextTWT & 0xFFFFFFFF00000000) != 0) &&
			(g_IsWfaTestBed == 0))
			ucNextTWTSize = 3;

#if 0
		/*
		** As we are not seeing any Flexible TWT Sched
		** criteria in test plan, temporarily mark it out
		*/
		uint8_t ucNextTWTSize = NEXT_TWT_SUBFIELD_64_BITS;
		uint64_t u8NextTWT = u8CurTsf + TSF_OFFSET_FOR_AGRT_RESUME;

		/* Adjust next TWT if 'Flexible TWT Sched' is not supported */
		if (!HE_IS_MAC_CAP_FLEXIBLE_TWT_SHDL(
			prStaRec->ucHeMacCapInfo)) {
			u8NextTWT = twtPlannerAdjustNextTWT(prAdapter,
					prBssInfo->ucBssIndex,
					prGetTsfCtxt->ucTWTFlowId,
					u8NextTWT);
		}
#endif

		DBGLOG(TWT_REQUESTER, WARN,
		"TWT Info Frame[2] %d Tgt[0x%x 0x%x] Cur[0x%x 0x%x] Input[0x%x 0x%x]\n",
		ucNextTWTSize,
		(uint32_t)(u8NextTWT & 0x00000000FFFFFFFF),
		(uint32_t)((u8NextTWT & 0xFFFFFFFF00000000) >> 32),
		(uint32_t)(u8CurTsf & 0x00000000FFFFFFFF),
		(uint32_t)((u8CurTsf & 0xFFFFFFFF00000000) >> 32),
		(uint32_t)(prGetTsfCtxt->rNextTWT.u8NextTWT &
				0x00000000FFFFFFFF),
		(uint32_t)((prGetTsfCtxt->rNextTWT.u8NextTWT &
				0xFFFFFFFF00000000) >> 32));

		/* Start the process to resume this TWT agreement */
		twtPlannerSendReqResume(prAdapter,
			prStaRec, prGetTsfCtxt->ucTWTFlowId,
			u8NextTWT, ucNextTWTSize);

		break;

#if (CFG_SUPPORT_BTWT == 1)
	case TWT_GET_TSF_FOR_ADD_AGRT_BTWT:
		prTWTFlow = twtPlannerFlowFindById(
					prStaRec, prGetTsfCtxt->ucTWTFlowId,
					ENUM_TWT_TYPE_BTWT);

		if (prTWTFlow == NULL) {
			DBGLOG(TWT_PLANNER, ERROR, "prTWTFlow is NULL.\n");

			return;
		}

		if (prTWTFlow->eBtwtState == ENUM_BTWT_FLOW_STATE_DEFAULT) {
			prTWTParams = &(prTWTFlow->rTWTPeerParams);

			if (prTWTParams != NULL) {
				prTWTParams->u8TWT = u8CurTsf;

				btwtPlannerSendReqStart(prAdapter, prStaRec,
					prGetTsfCtxt->ucTWTFlowId);
			}
		}

		break;
#endif

#if (CFG_SUPPORT_RTWT == 1)
	case TWT_GET_TSF_FOR_ADD_AGRT_RTWT:
		kal_fallthrough;
	case TWT_GET_TSF_FOR_JOIN_AGRT_RTWT:
		prTWTFlow = twtPlannerFlowFindById(
					prStaRec, prGetTsfCtxt->ucTWTFlowId,
					ENUM_TWT_TYPE_RTWT);

		/* To have mantissa alignment::Begin */
		u8twt_interval = ((u_int64_t)
			(prGetTsfCtxt->rTWTParams.u2WakeIntvalMantiss))
			<< prGetTsfCtxt->rTWTParams.ucWakeIntvalExponent;
		u8Temp = u8CurTsf + u8twt_interval;

		DBGLOG(TWT_PLANNER, WARN,
			"u8twt_interval: 0x%x 0x%x u8Temp 0x%x 0x%x\n",
			CPU_TO_LE32(u8twt_interval & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(u8twt_interval >> 32)),
			CPU_TO_LE32(u8Temp & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(u8Temp >> 32)));

		if ((prTWTFlow == NULL) || (u8twt_interval == 0)) {
			DBGLOG(TWT_PLANNER, ERROR,
				"prTWTFlow %d NULL(%s)\nu8twt_interval 0x%x\n",
				prGetTsfCtxt->ucTWTFlowId,
				(prTWTFlow == NULL) ? "Y" : "N",
				CPU_TO_LE32(u8twt_interval & 0xFFFFFFFF));

			kalMemFree(prGetTsfCtxt,
				VIR_MEM_TYPE, sizeof(*prGetTsfCtxt));

			return;
		}

		u8Mod = kal_mod64(u8Temp, u8twt_interval);

		prGetTsfCtxt->rTWTParams.u8TWT =
				u8CurTsf + u8twt_interval - u8Mod;

		DBGLOG(TWT_PLANNER, WARN,
			"TWT cur TSF: 0x%x 0x%x TWT req TSF 0x%x 0x%x\n",
			CPU_TO_LE32(u8CurTsf & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(u8CurTsf >> 32)),
			CPU_TO_LE32(prGetTsfCtxt->rTWTParams.u8TWT &
				0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(prGetTsfCtxt->rTWTParams.u8TWT
				>> 32)));

		DBGLOG(TWT_PLANNER, WARN,
			"u8Mod 0x%x 0x%x\n",
			CPU_TO_LE32(u8Mod & 0xFFFFFFFF),
			CPU_TO_LE32((uint32_t)(u8Mod >> 32)));
		/* To have mantissa alignment::End */

		prTWTParams = &(prTWTFlow->rTWTParams);

		if (prGetTsfCtxt->ucReason == TWT_GET_TSF_FOR_JOIN_AGRT_RTWT) {
			/* WFA asks RTWT setup frame target wake time = 0 */
			prTWTParams->u2TWT = 0;
			prTWTParams->u8TWT = 0;

			/* This is the real thing, not bypass Nego */
			prTWTParams->fgByPassNego = FALSE;

			/*
			 * For join RTWT, copy SET_TWT_PARAMS RTWT traffic info
			 * for later setup purpose, as to the trigger, flow
			 * type, ucMinWakeDur, ucWakeIntvalExponent,
			 * u2WakeIntvalMantiss, all use the settings in beacon
			 * RTWT IE already received
			 */
			prTWTParams->ucTrafficInfoPresent =
				prGetTsfCtxt->rTWTParams.ucTrafficInfoPresent;
			prTWTParams->ucDlUlBmpValid =
				prGetTsfCtxt->rTWTParams.ucDlUlBmpValid;
			prTWTParams->ucDlBmp =
				prGetTsfCtxt->rTWTParams.ucDlBmp;
			prTWTParams->ucUlBmp =
				prGetTsfCtxt->rTWTParams.ucUlBmp;
		} else {
			/*
			 * For new add RTWT, copy all SET_TWT_PARAMS RTWT
			 * parameters for later setup purpose
			 */
			kalMemCopy(prTWTParams, &(prGetTsfCtxt->rTWTParams),
				sizeof(struct _TWT_PARAMS_T));
		}

		/* The current TSF RTWT would align when receive setup resp */
		prTWTParams = &(prTWTFlow->rTWTPeerParams);
		prTWTParams->u8TWT = prGetTsfCtxt->rTWTParams.u8TWT;

		/*
		 * If we are in 11be plugfest STAUT 5.52.1, the WFA test plan
		 * asks non-activated RTWT STA stops TXOP before each active
		 * RTWT service period start.
		 *
		 * As we have setup local bypass TWT in heRlmRecBTWTparams(),
		 * we should teardown it before the real thing!!!
		 */
		if ((g_IsTwtLogo == 1) && (g_IsWfaTestBed == 0) &&
			(IS_FEATURE_ENABLED(
				prAdapter->rWifiVar.ucRTWTStautProtect))) {
			do {
				ucFlowID = twtPlannerDrvAgrtGetFlowID(
						prAdapter,
						prBssInfo->ucBssIndex,
						1,
						ENUM_TWT_TYPE_RTWT);

				if (ucFlowID != TWT_INCORRECT_FLOW_ID)
					rtwtPlannerDelAgrtTbl(
						prAdapter,
						prBssInfo,
						prStaRec,
						ucFlowID);
			} while (ucFlowID != TWT_INCORRECT_FLOW_ID);
		}

		/* Start the process to nego for a new agreement */
		rtwtPlannerSendReqStart(prAdapter,
			prStaRec,
			((prGetTsfCtxt->ucReason ==
				TWT_GET_TSF_FOR_ADD_AGRT_RTWT) ?
			MID_RTWT_REQ_FSM_START : MID_RTWT_REQ_FSM_JOIN),
			prGetTsfCtxt->ucTWTFlowId);

		break;
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	case TWT_GET_TSF_FOR_ADD_AGRT_ML_TWT_ALL_LINKS:
		/*
		* If we reach here, we are preparing setup
		* frame of multi-link TWT with all links sharing
		* the same TWT parameter.
		*/
		prTWTFlow = twtPlannerFlowFindById(
					prStaRec, prGetTsfCtxt->ucTWTFlowId,
					ENUM_TWT_TYPE_MLTWT);

		u8twt_interval = ((u_int64_t)
			(prGetTsfCtxt->rTWTParams.u2WakeIntvalMantiss))
			<< prGetTsfCtxt->rTWTParams.ucWakeIntvalExponent;
		u8Temp = u8CurTsf + u8twt_interval;

		if ((prTWTFlow == NULL) || (u8twt_interval == 0)) {
			DBGLOG(TWT_PLANNER, ERROR,
				"prTWTFlow NULL(%s)\nu8twt_interval 0x%x\n",
				(prTWTFlow == NULL) ? "Y" : "N",
				CPU_TO_LE32(u8twt_interval & 0xFFFFFFFF));

			kalMemFree(prGetTsfCtxt,
				VIR_MEM_TYPE, sizeof(*prGetTsfCtxt));

			return;
		}

		u8Mod = kal_mod64(u8Temp, u8twt_interval);

		prGetTsfCtxt->rTWTParams.u8TWT =
				u8CurTsf + u8twt_interval - u8Mod;

		prTWTParams = &(prTWTFlow->rTWTParams);

		kalMemCopy(prTWTParams, &(prGetTsfCtxt->rTWTParams),
			sizeof(struct _TWT_PARAMS_T));

		/* Start the process to nego for a new agreement */
		mltwtPlannerSendReqStartAllLinks(prAdapter,
			prStaRec, prGetTsfCtxt->ucTWTFlowId);

		break;

	case TWT_GET_TSF_FOR_ADD_AGRT_ML_TWT_ONE_BY_ONE:
		/* Continue to add MLTWT param, no need to nego */
		prTWTFlow = twtPlannerFlowFindById(
					prStaRec, prGetTsfCtxt->ucTWTFlowId,
					ENUM_TWT_TYPE_MLTWT);

		u8twt_interval = ((u_int64_t)
			(prGetTsfCtxt->rTWTParams.u2WakeIntvalMantiss))
			<< prGetTsfCtxt->rTWTParams.ucWakeIntvalExponent;
		u8Temp = u8CurTsf + u8twt_interval;

		if ((prTWTFlow == NULL) || (u8twt_interval == 0)) {
			DBGLOG(TWT_PLANNER, ERROR,
				"prTWTFlow NULL(%s)\nu8twt_interval 0x%x\n",
				(prTWTFlow == NULL) ? "Y" : "N",
				CPU_TO_LE32(u8twt_interval & 0xFFFFFFFF));

			kalMemFree(prGetTsfCtxt,
				VIR_MEM_TYPE, sizeof(*prGetTsfCtxt));

			return;
		}

		prTWTFlow->fgIsMLTWT = TRUE;

		u8Mod = kal_mod64(u8Temp, u8twt_interval);

		prGetTsfCtxt->rTWTParams.u8TWT =
				u8CurTsf + u8twt_interval - u8Mod;

		prTWTParams = &(prTWTFlow->rTWTParams);

		kalMemCopy(prTWTParams, &(prGetTsfCtxt->rTWTParams),
			sizeof(struct _TWT_PARAMS_T));

		/* This is not the final MLTWT param, no need to nego */

		break;

	case TWT_GET_TSF_FOR_END_AGRT_ML_TWT_ONE_BY_ONE:
		/* Final MLTWT param, ready for nego */
		prTWTFlow = twtPlannerFlowFindById(
					prStaRec, prGetTsfCtxt->ucTWTFlowId,
					ENUM_TWT_TYPE_MLTWT);

		u8twt_interval = ((u_int64_t)
			(prGetTsfCtxt->rTWTParams.u2WakeIntvalMantiss))
			<< prGetTsfCtxt->rTWTParams.ucWakeIntvalExponent;
		u8Temp = u8CurTsf + u8twt_interval;

		if ((prTWTFlow == NULL) || (u8twt_interval == 0)) {
			DBGLOG(TWT_PLANNER, ERROR,
				"prTWTFlow NULL(%s)\nu8twt_interval 0x%x\n",
				(prTWTFlow == NULL) ? "Y" : "N",
				CPU_TO_LE32(u8twt_interval & 0xFFFFFFFF));

			kalMemFree(prGetTsfCtxt,
				VIR_MEM_TYPE, sizeof(*prGetTsfCtxt));

			return;
		}

		prTWTFlow->fgIsMLTWT = TRUE;

		u8Mod = kal_mod64(u8Temp, u8twt_interval);

		prGetTsfCtxt->rTWTParams.u8TWT =
				u8CurTsf + u8twt_interval - u8Mod;

		prTWTParams = &(prTWTFlow->rTWTParams);

		kalMemCopy(prTWTParams, &(prGetTsfCtxt->rTWTParams),
			sizeof(struct _TWT_PARAMS_T));

		/*
		* Get the BSS_INFO/STA_REC of MLO setup link,
		* the MLTWT setup frame is nego on the setup link
		*/
		prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

		if (!prMldBssInfo) {
			DBGLOG(REQ, INFO, "MLTWT Invalid MLD_BSS_INFO\n");

				return;
		}

		prBssInfo = mldGetBssInfoByLinkID(
						prAdapter,
						prMldBssInfo,
						0,
						TRUE);

		if (!prBssInfo) {
			DBGLOG(REQ, INFO, "Find no MLTWT setup link\n");

			return;
		}

		prStaRec = prBssInfo->prStaRecOfAP;

		/* We are ready to make MLTWT nego */
		mltwtPlannerSendReqStart(prAdapter,
			prStaRec, prGetTsfCtxt->ucTWTFlowId);

		break;
#endif

	default:
		DBGLOG(TWT_PLANNER, ERROR,
			"Unknown reason to get TSF %u\n",
			prGetTsfCtxt->ucReason);
		break;
	}

	/* free memory */
	kalMemFree(prGetTsfCtxt, VIR_MEM_TYPE, sizeof(*prGetTsfCtxt));
}

uint32_t
twtPlannerGetCurrentTSF(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _EXT_CMD_GET_MAC_INFO_T *prMacInfoCmd;
	struct _EXTRA_ARG_TSF_T *prTsfArg;

	prMacInfoCmd = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_GET_MAC_INFO_T));
	if (!prMacInfoCmd) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Alloc _EXT_CMD_GET_MAC_INFO_T FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prTsfArg = &(prMacInfoCmd->rExtraArgument.rTsfArg);
	prMacInfoCmd->u2MacInfoId = CPU_TO_LE16(MAC_INFO_TYPE_TSF);
	prTsfArg->ucHwBssidIndex = prBssInfo->ucOwnMacIndex;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
					CMD_ID_LAYER_0_EXT_MAGIC_NUM,
					EXT_CMD_ID_GET_MAC_INFO,
					FALSE,
					TRUE,
					FALSE,
					twtPlannerGetTsfDone,
					NULL,
					sizeof(struct _EXT_CMD_GET_MAC_INFO_T),
					(uint8_t *) (prMacInfoCmd),
					pvSetBuffer, u4SetBufferLen);

	cnmMemFree(prAdapter, prMacInfoCmd);

	return rWlanStatus;
}

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
void twtPlannerGetCnmGrantedDone(
	struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf)
{
	struct _TWT_GET_TSF_CONTEXT_T *prGetTsfCtxt;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prCmdInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	if (!prCmdInfo->pvInformationBuffer) {
		DBGLOG(TWT_PLANNER, ERROR,
			"prCmdInfo->pvInformationBuffer is NULL.\n");

		return;
	}

	prGetTsfCtxt = (struct _TWT_GET_TSF_CONTEXT_T *)
		prCmdInfo->pvInformationBuffer;

	if (!prGetTsfCtxt) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prGetTsfCtxt\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prGetTsfCtxt->ucBssIdx);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return;
	}

	prStaRec = prBssInfo->prStaRecOfAP;

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return;
	}

	/*
	 * To setup CNM abort timer in case get current TSF timeout
	 */
	twtGetCurrentTsfTimeoutInit(
		prAdapter,
		prStaRec,
		prGetTsfCtxt->ucReason);

	/* Continue to use the existing prGetTsfCtxt to get current TSF */
	twtPlannerGetCurrentTSF(prAdapter, prBssInfo,
		prGetTsfCtxt, sizeof(*prGetTsfCtxt));
}

static uint32_t
twtPlannerGetCnmGranted(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _EXT_CMD_GET_MAC_INFO_T *prMacInfoCmd;
	struct _EXTRA_ARG_TSF_T *prTsfArg;

	prMacInfoCmd = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_GET_MAC_INFO_T));
	if (!prMacInfoCmd) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Alloc _EXT_CMD_GET_MAC_INFO_T FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prTsfArg = &(prMacInfoCmd->rExtraArgument.rTsfArg);
	prMacInfoCmd->u2MacInfoId = CPU_TO_LE16(MAC_INFO_TYPE_TWT_STA_CNM);
	prTsfArg->ucHwBssidIndex = prBssInfo->ucOwnMacIndex;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
					CMD_ID_LAYER_0_EXT_MAGIC_NUM,
					EXT_CMD_ID_TWT_STA_GET_CNM_GRANTED,
					FALSE,
					TRUE,
					FALSE,
					twtPlannerGetCnmGrantedDone,
					NULL,
					sizeof(struct _EXT_CMD_GET_MAC_INFO_T),
					(uint8_t *) (prMacInfoCmd),
					pvSetBuffer, u4SetBufferLen);

	cnmMemFree(prAdapter, prMacInfoCmd);

	return rWlanStatus;
}

uint32_t twtPlannerAbortCnmGranted(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId, uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	uint8_t ucAgrtTblIdx;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *prTWTAgrtUpdate;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (prBssInfo == NULL) {
		DBGLOG(TWT_PLANNER, ERROR, "No bssinfo to delete agrt\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Use the AGRT tbl idx 0 for CNM abort purpose */
	ucAgrtTblIdx = 0;

	/* Send cmd to delete agreement entry in FW */
	prTWTAgrtUpdate = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T));
	if (!prTWTAgrtUpdate) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Alloc _EXT_CMD_TWT_ARGT_UPDATE_T for del FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prTWTAgrtUpdate->ucAgrtTblIdx = ucAgrtTblIdx;
	prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_CNM_ABORT;
	prTWTAgrtUpdate->ucOwnMacId =
				prBssInfo->ucOwnMacIndex;
	prTWTAgrtUpdate->ucFlowId = ucFlowId;
	prTWTAgrtUpdate->u2PeerIdGrpId =
		CPU_TO_LE16(prStaRec->ucWlanIndex);
	prTWTAgrtUpdate->ucIsRoleAp = 0;  /* STA role */
	prTWTAgrtUpdate->ucBssIndex = prBssInfo->ucBssIndex;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_TWT_AGRT_UPDATE,
			TRUE,
			FALSE,
			fgIsOid,
			pfCmdDoneHandler,
			pfCmdTimeoutHandler,
			sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
			(uint8_t *) (prTWTAgrtUpdate),
			NULL, 0);

	cnmMemFree(prAdapter, prTWTAgrtUpdate);

	return WLAN_STATUS_SUCCESS;
}

void twtGetCurrentTsfTimeoutInit(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _TWT_GET_TSF_REASON ucReason)
{
	struct WIFI_VAR *prWifiVar = NULL;
	struct BSS_INFO *prBssInfo = NULL;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	prWifiVar = &prAdapter->rWifiVar;

	if (!prWifiVar) {
		DBGLOG(TWT_PLANNER, ERROR,
			"invalid prWifiVar\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (prBssInfo == NULL) {
		DBGLOG(TWT_PLANNER, ERROR,
			"invalid BSS_INFO\n");

		return;
	}

	if (ucReason != TWT_GET_TSF_FOR_ADD_AGRT) {
		DBGLOG(TWT_PLANNER, ERROR,
			"invalid ucReason %d\n", ucReason);

		return;
	}

	if (timerPendingTimer(&(prStaRec->rTwtGetCurrentTsfTimeoutTimer)))
		cnmTimerStopTimer(
			prAdapter,
			&(prStaRec->rTwtGetCurrentTsfTimeoutTimer));

	cnmTimerInitTimer(
		prAdapter,
		&(prStaRec->rTwtGetCurrentTsfTimeoutTimer),
		(PFN_MGMT_TIMEOUT_FUNC)twtGetCurrentTsfTimeout,
		(uintptr_t)prBssInfo);

	cnmTimerStartTimer(
		prAdapter,
		&(prStaRec->rTwtGetCurrentTsfTimeoutTimer),
		prWifiVar->u4TwtCnmAbortTimeoutMs);

	DBGLOG(TWT_PLANNER, STATE,
		"TWT get current TSF timeout %d init\n",
		prWifiVar->u4TwtCnmAbortTimeoutMs);
}

void twtGetCurrentTsfTimeoutDeInit(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _TWT_GET_TSF_REASON ucReason)
{
	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"invalid prStaRec\n");

		return;
	}

	if (ucReason != TWT_GET_TSF_FOR_ADD_AGRT) {
		DBGLOG(TWT_PLANNER, ERROR,
			"invalid ucReason %d\n", ucReason);

		return;
	}

	if (timerPendingTimer(&(prStaRec->rTwtGetCurrentTsfTimeoutTimer))) {
		cnmTimerStopTimer(
			prAdapter,
			&(prStaRec->rTwtGetCurrentTsfTimeoutTimer));

		DBGLOG(TWT_PLANNER, STATE,
			"TWT get current TSF timeout de-init\n");
	}
}

void twtGetCurrentTsfTimeout(
	struct ADAPTER *prAdapter,
	uintptr_t ulParamPtr)
{
	struct STA_RECORD *prStaRec = NULL;
	struct BSS_INFO *prBssInfo = NULL;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	prBssInfo = (struct BSS_INFO *)ulParamPtr;

	if (prBssInfo == NULL) {
		DBGLOG(TWT_PLANNER, ERROR,
			"invalid BSS_INFO\n");

		return;
	}

	prStaRec = prBssInfo->prStaRecOfAP;

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"invalid prStaRec\n");

		return;
	}

	DBGLOG(TWT_PLANNER, STATE,
		"TWT get current TSF timeout, abort CNM chnl grant!!\n");

	twtPlannerAbortCnmGranted(
		prAdapter,
		prBssInfo,
		prStaRec,
		0,
		FALSE,
		NULL,
		NULL);
}
#endif  /* #if (CFG_SUPPORT_TWT_STA_CNM == 1) */

void twtPlannerSetParams(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_PARAMS_SET_T *prTWTParamSetMsg;
	struct _TWT_CTRL_T rTWTCtrl, *prTWTCtrl = &rTWTCtrl;
	struct _TWT_GET_TSF_CONTEXT_T *prGetTsfCtxt = NULL;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucBssIdx, ucFlowId;
	uint8_t ucFlowId_real;
	uint8_t ucAgrtIdx = 0;
	enum _ENUM_TWT_TYPE_T eTwtType = ENUM_TWT_TYPE_DEFAULT;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	prTWTParamSetMsg = (struct _MSG_TWT_PARAMS_SET_T *) prMsgHdr;
	kalMemCopy(prTWTCtrl, &prTWTParamSetMsg->rTWTCtrl, sizeof(*prTWTCtrl));

	cnmMemFree(prAdapter, prMsgHdr);

	/* Find the BSS info */
	ucBssIdx = prTWTCtrl->ucBssIdx;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return;
	}

	if (prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE ||
		prBssInfo->eConnectionState != MEDIA_STATE_CONNECTED) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Current op mode %d connection state %d\n",
			prBssInfo->eCurrentOPMode, prBssInfo->eConnectionState);
		return;
	}

	/* Get the STA Record */
	prStaRec = prBssInfo->prStaRecOfAP;

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR, "No AP STA Record\n");
		return;
	}

	/* If bypassing TWT nego, this ctrl param is treated as a TWT agrt*/
	if (IS_TWT_PARAM_ACTION_ADD_BYPASS(prTWTCtrl->ucCtrlAction)) {
		prGetTsfCtxt =
			kalMemAlloc(sizeof(struct _TWT_GET_TSF_CONTEXT_T),
				VIR_MEM_TYPE);
		if (prGetTsfCtxt == NULL) {
			DBGLOG(TWT_PLANNER, ERROR, "mem alloc failed\n");
			return;
		}

		prGetTsfCtxt->ucReason = TWT_GET_TSF_FOR_ADD_AGRT_BYPASS;
		prGetTsfCtxt->ucBssIdx = ucBssIdx;
		prGetTsfCtxt->ucTWTFlowId = prTWTCtrl->ucTWTFlowId;
		prGetTsfCtxt->fgIsOid = FALSE;
		kalMemCopy(&(prGetTsfCtxt->rTWTParams),
				&(prTWTCtrl->rTWTParams),
				sizeof(struct _TWT_PARAMS_T));
		twtPlannerGetCurrentTSF(prAdapter,
			prBssInfo, prGetTsfCtxt, sizeof(*prGetTsfCtxt));

		return;
	}

#if (CFG_SUPPORT_TWT_HOTSPOT_AC == 1)
	/*
	 * Fot CFG_SUPPORT_TWT_HOTSPOT_AC, we are only limited
	 * to operate on MTK internal connectivity, by this native
	 * design, with software AP in the remote MTK platform.
	 */
	if (!IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucTWTRequester)) {
		DBGLOG(TWT_PLANNER, ERROR,
			"User config of TWT req %u\n",
			prAdapter->rWifiVar.ucTWTRequester);

		return;
	}

	if (!HE_IS_MAC_CAP_TWT_RSP(prStaRec->ucHeMacCapInfo) &&
		(prStaRec->ucTWTHospotSupport == FALSE))	{
		DBGLOG(TWT_PLANNER, ERROR,
			"Peer cap 0x%x remote AP of TWT hotspot support %d\n",
			prStaRec->ucHeMacCapInfo[0],
			prStaRec->ucTWTHospotSupport);

		return;
	}
#else
	/* Check if peer has TWT responder capability and local config */
	if ((prTWTCtrl->ucCtrlAction == TWT_PARAM_ACTION_ADD) &&
		(!HE_IS_MAC_CAP_TWT_RSP(prStaRec->ucHeMacCapInfo) ||
		!IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucTWTRequester))) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Peer cap 0x%x user config of TWT req %u\n",
			prStaRec->ucHeMacCapInfo[0],
			prAdapter->rWifiVar.ucTWTRequester);
		return;
	}
#endif

	/* For COEX concern, suppose only 5G is allowed */
	if (!(prAdapter->rWifiVar.ucTWTStaBandBitmap & BIT(prBssInfo->eBand))) {
		DBGLOG(TWT_PLANNER, ERROR,
			"TWT BAND support bitmaps(%u)!=%u\n",
			prAdapter->rWifiVar.ucTWTStaBandBitmap,
			prBssInfo->eBand);
		return;
	}

#if (CFG_SUPPORT_BTWT == 1)
	if ((prTWTCtrl->ucCtrlAction == TWT_PARAM_ACTION_ADD_BTWT) &&
		(!HE_IS_MAC_CAP_BTWT_SUPT(prStaRec->ucHeMacCapInfo) ||
		!IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucBTWTSupport))) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Peer cap 0x%x user config of bTWT req %u\n",
			prStaRec->ucHeMacCapInfo[2],
			prAdapter->rWifiVar.ucBTWTSupport);
		return;
	}
#endif

#if (CFG_SUPPORT_RTWT == 1)
	if ((prTWTCtrl->ucCtrlAction == TWT_PARAM_ACTION_ADD_RTWT) &&
		(!GET_EHT_MAC_CAP_RESTRICTED_TWT(prStaRec->ucEhtMacCapInfo) ||
		!IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRTWTSupport))) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Peer cap 0x%x user config of rTWT req %u\n",
			prStaRec->ucEhtMacCapInfo[0],
			prAdapter->rWifiVar.ucRTWTSupport);
		return;
	}
#endif

	ucFlowId = prTWTCtrl->ucTWTFlowId;

	ucFlowId_real = ucFlowId;

	switch (prTWTCtrl->ucCtrlAction) {
	case TWT_PARAM_ACTION_ADD:
#if (CFG_SUPPORT_BTWT == 1)
		kal_fallthrough;
	case TWT_PARAM_ACTION_ADD_BTWT:
#endif
#if (CFG_SUPPORT_RTWT == 1)
		kal_fallthrough;
	case TWT_PARAM_ACTION_ADD_RTWT:
		kal_fallthrough;
	case TWT_PARAM_ACTION_JOIN_RTWT:
#endif

		if (twtPlannerDrvAgrtFind(
			prAdapter, ucBssIdx,
			ucFlowId, &ucFlowId_real) >= TWT_AGRT_MAX_NUM) {

			prGetTsfCtxt =
				kalMemAlloc(
					sizeof(struct _TWT_GET_TSF_CONTEXT_T),
					VIR_MEM_TYPE);
			if (prGetTsfCtxt == NULL) {
				DBGLOG(TWT_PLANNER, ERROR,
					"mem alloc failed\n");
				return;
			}


			if (prTWTCtrl->ucCtrlAction == TWT_PARAM_ACTION_ADD)
				prGetTsfCtxt->ucReason =
					TWT_GET_TSF_FOR_ADD_AGRT;
#if (CFG_SUPPORT_BTWT == 1)
			else if (prTWTCtrl->ucCtrlAction ==
					TWT_PARAM_ACTION_ADD_BTWT)
				prGetTsfCtxt->ucReason =
					TWT_GET_TSF_FOR_ADD_AGRT_BTWT;
#endif
#if (CFG_SUPPORT_RTWT == 1)
			else if (prTWTCtrl->ucCtrlAction ==
					TWT_PARAM_ACTION_ADD_RTWT)
				prGetTsfCtxt->ucReason =
					TWT_GET_TSF_FOR_ADD_AGRT_RTWT;
			else if (prTWTCtrl->ucCtrlAction ==
					TWT_PARAM_ACTION_JOIN_RTWT)
				prGetTsfCtxt->ucReason =
					TWT_GET_TSF_FOR_JOIN_AGRT_RTWT;
#endif

			prGetTsfCtxt->ucBssIdx = ucBssIdx;
			prGetTsfCtxt->ucTWTFlowId = prTWTCtrl->ucTWTFlowId;
			prGetTsfCtxt->fgIsOid = FALSE;

			kalMemCopy(&(prGetTsfCtxt->rTWTParams),
					&(prTWTCtrl->rTWTParams),
					sizeof(struct _TWT_PARAMS_T));

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
			prGetTsfCtxt->ucTwtStaCnmReason = TWT_STA_CNM_SETUP;

			DBGLOG(TWT_PLANNER, WARN,
				"BSS %u TWT flow %u setup to get CNM granted\n",
				ucBssIdx, ucFlowId);

			twtPlannerGetCnmGranted(prAdapter, prBssInfo,
				prGetTsfCtxt, sizeof(*prGetTsfCtxt));
#else
			prGetTsfCtxt->ucTwtStaCnmReason = TWT_STA_CNM_DEFAULT;

			DBGLOG(TWT_PLANNER, WARN,
				"BSS %u TWT flow %u get current TSF\n",
				ucBssIdx, ucFlowId);

			twtPlannerGetCurrentTSF(prAdapter, prBssInfo,
				prGetTsfCtxt, sizeof(*prGetTsfCtxt));
#endif

			return;
		}

		DBGLOG(TWT_PLANNER, ERROR,
			"BSS %u TWT flow %u already exists\n",
			ucBssIdx, ucFlowId);
		break;

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	case TWT_PARAM_ACTION_ADD_ML_TWT_ALL_LINKS:
		/* MLTWT all links sharing the same TWT parameter */
		if (twtPlannerDrvAgrtFind(
			prAdapter, ucBssIdx,
			ucFlowId, &ucFlowId_real) >= TWT_AGRT_MAX_NUM) {

			prGetTsfCtxt =
				kalMemAlloc(
					sizeof(struct _TWT_GET_TSF_CONTEXT_T),
					VIR_MEM_TYPE);

			if (prGetTsfCtxt == NULL) {
				DBGLOG(TWT_PLANNER, ERROR,
					"mem alloc failed\n");
				return;
			}

			prGetTsfCtxt->ucReason =
				TWT_GET_TSF_FOR_ADD_AGRT_ML_TWT_ALL_LINKS;

			prGetTsfCtxt->ucBssIdx = ucBssIdx;
			prGetTsfCtxt->ucTWTFlowId = prTWTCtrl->ucTWTFlowId;
			prGetTsfCtxt->fgIsOid = FALSE;

			kalMemCopy(&(prGetTsfCtxt->rTWTParams),
					&(prTWTCtrl->rTWTParams),
					sizeof(struct _TWT_PARAMS_T));

			DBGLOG(TWT_PLANNER, WARN,
				"BSS %u ML TWT flow %u get current TSF\n",
				ucBssIdx, ucFlowId);

			twtPlannerGetCurrentTSF(prAdapter, prBssInfo,
				prGetTsfCtxt, sizeof(*prGetTsfCtxt));

			return;
		}

		DBGLOG(TWT_PLANNER, ERROR,
			"BSS %u ML TWT flow %u already exists\n",
			ucBssIdx, ucFlowId);

		break;

	case TWT_PARAM_ACTION_ADD_ML_TWT_ONE_BY_ONE:
		/* MLTWT each link uses distinct TWT parameter */
		if (twtPlannerDrvAgrtFind(
			prAdapter, ucBssIdx,
			ucFlowId, &ucFlowId_real) >= TWT_AGRT_MAX_NUM) {

			prGetTsfCtxt =
				kalMemAlloc(
					sizeof(struct _TWT_GET_TSF_CONTEXT_T),
					VIR_MEM_TYPE);

			if (prGetTsfCtxt == NULL) {
				DBGLOG(TWT_PLANNER, ERROR,
					"mem alloc failed\n");
				return;
			}

			if (prTWTCtrl->ucMLTWT_Param_Last == 0)
				prGetTsfCtxt->ucReason =
					TWT_GET_TSF_FOR_ADD_AGRT_ML_TWT_ONE_BY_ONE;
			else
				prGetTsfCtxt->ucReason =
					TWT_GET_TSF_FOR_END_AGRT_ML_TWT_ONE_BY_ONE;

			prGetTsfCtxt->ucBssIdx = ucBssIdx;
			prGetTsfCtxt->ucTWTFlowId = prTWTCtrl->ucTWTFlowId;
			prGetTsfCtxt->fgIsOid = FALSE;

			kalMemCopy(&(prGetTsfCtxt->rTWTParams),
					&(prTWTCtrl->rTWTParams),
					sizeof(struct _TWT_PARAMS_T));

			DBGLOG(TWT_PLANNER, WARN,
				"BSS %u ML TWT flow %u get current TSF\n",
				ucBssIdx, ucFlowId);

			twtPlannerGetCurrentTSF(prAdapter, prBssInfo,
				prGetTsfCtxt, sizeof(*prGetTsfCtxt));

			return;
		}

		DBGLOG(TWT_PLANNER, ERROR,
			"BSS %u ML TWT flow %u already exists\n",
			ucBssIdx, ucFlowId);

		break;
#endif

	case TWT_PARAM_ACTION_DEL:
		ucAgrtIdx = twtPlannerDrvAgrtFind(
					prAdapter, ucBssIdx, ucFlowId,
					&ucFlowId_real);

		if (ucAgrtIdx >= TWT_AGRT_MAX_NUM) {

			DBGLOG(TWT_PLANNER, ERROR,
				"BSS %u TWT flow %u doesn't exist\n",
				ucBssIdx, ucFlowId);

			return;
		}

		/* Start the process to tear down this TWT agreement */
		ucFlowId = ucFlowId_real;

		eTwtType = twtPlannerDrvAgrtGetTwtTypeByIndex(prAdapter,
								ucAgrtIdx);

		switch (eTwtType) {
#if (CFG_SUPPORT_BTWT == 1)
		case ENUM_TWT_TYPE_BTWT:
			if (prStaRec->arBTWTFlow[ucFlowId].eBtwtState ==
				ENUM_BTWT_FLOW_STATE_ACTIVATED) {
				btwtPlannerSendReqTeardown(prAdapter,
					prStaRec, ucFlowId);
			}

			break;
#endif

#if (CFG_SUPPORT_RTWT == 1)
		case ENUM_TWT_TYPE_RTWT:
			rtwtPlannerSendReqTeardown(
				prAdapter,
				prStaRec, ucFlowId, prTWTCtrl->fgTeardownAll);

			break;
#endif

		case ENUM_TWT_TYPE_ITWT:
		case ENUM_TWT_TYPE_MLTWT:
#if (CFG_SUPPORT_TWT_STA_CNM == 1)
			prGetTsfCtxt = kalMemAlloc(
					sizeof(
					struct _TWT_GET_TSF_CONTEXT_T),
					VIR_MEM_TYPE);

			if (prGetTsfCtxt == NULL) {
				DBGLOG(TWT_PLANNER, ERROR,
					"mem alloc failed\n");

				return;
			}

			prGetTsfCtxt->ucReason =
					TWT_GET_TSF_FOR_CNM_TEARDOWN_GRANTED;
			prGetTsfCtxt->ucBssIdx = ucBssIdx;
			prGetTsfCtxt->ucTWTFlowId =
					prTWTCtrl->ucTWTFlowId;
			prGetTsfCtxt->fgIsOid = FALSE;

			kalMemCopy(&(prGetTsfCtxt->rTWTParams),
					&(prTWTCtrl->rTWTParams),
					sizeof(struct _TWT_PARAMS_T));

			prGetTsfCtxt->ucTwtStaCnmReason =
					TWT_STA_CNM_TEARDOWN;

			DBGLOG(TWT_PLANNER, WARN,
				"BSS %u TWT flow %u teardown to get CNM granted\n",
				ucBssIdx, ucFlowId);

			twtPlannerGetCnmGranted(prAdapter, prBssInfo,
					prGetTsfCtxt, sizeof(*prGetTsfCtxt));
#else
			twtPlannerSendReqTeardown(prAdapter,
				prStaRec, ucFlowId);
#endif
			break;

		case ENUM_TWT_TYPE_DEFAULT:
			/* TWT local emulate goes over here */
			twtPlannerSendReqTeardown(prAdapter,
				prStaRec, ucFlowId);

			break;

		case ENUM_TWT_TYPE_NUM:
		default:
			DBGLOG(TWT_PLANNER, WARN,
				"BSS %u TWT flow %u teardown wrong type %d\n",
				ucBssIdx, ucFlowId,
				eTwtType);

			break;
		}

		break;

	case TWT_PARAM_ACTION_SUSPEND:
		if (twtPlannerDrvAgrtFind(
			prAdapter, ucBssIdx, ucFlowId,
			&ucFlowId_real) < TWT_AGRT_MAX_NUM) {
			/* Start the process to suspend this TWT agreement */
			ucFlowId = ucFlowId_real;

			twtPlannerSendReqSuspend(prAdapter,
				prStaRec, ucFlowId);
		} else {
			DBGLOG(TWT_PLANNER, ERROR,
				"BSS %u TWT flow %u doesn't exist\n",
				ucBssIdx, ucFlowId);
		}

		break;

	case TWT_PARAM_ACTION_RESUME:
		if (twtPlannerDrvAgrtFind(
			prAdapter, ucBssIdx, ucFlowId,
			&ucFlowId_real) < TWT_AGRT_MAX_NUM) {
			prGetTsfCtxt =
				kalMemAlloc(
					sizeof(struct _TWT_GET_TSF_CONTEXT_T),
					VIR_MEM_TYPE);
			if (prGetTsfCtxt == NULL) {
				DBGLOG(TWT_PLANNER, ERROR,
					"mem alloc failed\n");
				return;
			}

			ucFlowId = ucFlowId_real;

			prGetTsfCtxt->ucReason = TWT_GET_TSF_FOR_RESUME_AGRT;
			prGetTsfCtxt->ucBssIdx = ucBssIdx;
			prGetTsfCtxt->ucTWTFlowId = ucFlowId;
			prGetTsfCtxt->fgIsOid = FALSE;
			kalMemCopy(&(prGetTsfCtxt->rNextTWT),
					&(prTWTCtrl->rNextTWT),
					sizeof(struct _NEXT_TWT_INFO_T));
			twtPlannerGetCurrentTSF(prAdapter, prBssInfo,
				prGetTsfCtxt, sizeof(*prGetTsfCtxt));
		} else {
			DBGLOG(TWT_PLANNER, ERROR,
				"BSS %u TWT flow %u doesn't exist\n",
				ucBssIdx, ucFlowId);
		}

		break;

	default:
		DBGLOG(TWT_PLANNER, ERROR,
			"Action %u not supported\n", prTWTCtrl->ucCtrlAction);
		break;
	}
}

void twtPlannerRxNegoResult(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_IND_RESULT_T *prTWTFsmResultMsg;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint8_t ucTWTFlowId;
	struct _TWT_PARAMS_T *prTWTResult, *prTWTParams;
	struct _TWT_FLOW_T *prTWTFlow;
#ifdef CFG_SUPPORT_TWT_EXT
	struct _TWT_PARAMS_T *prTWTSetup;
	uint32_t u4ResultWakeIntvl = 0;
	uint32_t u4SetupWakeIntvl = 0;
	bool fgSetupCheck = FALSE;
#endif
	enum _ENUM_TWT_TYPE_T eTwtType = ENUM_TWT_TYPE_DEFAULT;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	prTWTFsmResultMsg = (struct _MSG_TWT_REQFSM_IND_RESULT_T *) prMsgHdr;
	prStaRec = prTWTFsmResultMsg->prStaRec;
	ucTWTFlowId = prTWTFsmResultMsg->ucTWTFlowId;
	eTwtType = prTWTFsmResultMsg->eTwtType;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_PLANNER, ERROR,
				"Invalid prStaRec\n");

		return;
	}

	cnmMemFree(prAdapter, prMsgHdr);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Rx nego result: invalid STA Type %d\n",
			prStaRec->eStaType);
		return;
	}

	prTWTFlow = twtPlannerFlowFindById(prStaRec, ucTWTFlowId, eTwtType);

	if (prTWTFlow == NULL) {
		DBGLOG(TWT_PLANNER, ERROR,
			"NULL TWT flow %d type %d\n",
			ucTWTFlowId, eTwtType);

		return;
	}

	prTWTResult = &(prTWTFlow->rTWTPeerParams);

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	if (prTWTFlow->fgIsMLTWT == TRUE) {
		/* MLTWT manipulation goes over here */
		mltwtPlannerRxNegoResult(
			prAdapter,
			prStaRec,
			ucTWTFlowId);

#if (CFG_SUPPORT_TWT_STA_CNM == 0)
		/* Disable SCAN during TWT activity */
		prAdapter->fgEnOnlineScan = FALSE;
#endif

		return;
	}

	/* i-TWT/BTWT goes in existing flow */
#endif

	DBGLOG(TWT_PLANNER, STATE, "Rx nego id %d resp=%d\n",
		ucTWTFlowId, prTWTResult->ucSetupCmd);

	switch (prTWTResult->ucSetupCmd) {
	case TWT_SETUP_CMD_ID_ACCEPT:
#ifndef CFG_SUPPORT_TWT_EXT
#if (CFG_SUPPORT_TWT_STA_CNM == 1)
		/*
		 * To setup CNM abort timer to cover TWT
		 * setup agreetment flow by twtPlannerAddAgrtTbl()
		 */
		twtReqFsmSetupTimeoutStateCfg(
			prAdapter,
			prStaRec,
			TWT_CNM_STATE_ADD_AGRT,
			ucTWTFlowId,
			(enum _ENUM_TWT_TYPE_T *)&prTWTFlow->eTwtType);
#endif

		/* Update agreement table */
		twtPlannerAddAgrtTbl(prAdapter, prBssInfo, prStaRec,
			prTWTResult, ucTWTFlowId, eTwtType, FALSE,
			NULL, NULL /* handle TWT cmd timeout? */);

#if (CFG_SUPPORT_TWT_STA_CNM == 0)
		/* Disable SCAN during TWT activity */
		prAdapter->fgEnOnlineScan = FALSE;
#endif

		break;
#else
		prTWTSetup = &(prTWTFlow->rTWTParams);

		u4ResultWakeIntvl = prTWTResult->u2WakeIntvalMantiss <<
			prTWTResult->ucWakeIntvalExponent;
		u4SetupWakeIntvl = prTWTSetup->u2WakeIntvalMantiss <<
			prTWTSetup->ucWakeIntvalExponent;

		if ((u4SetupWakeIntvl == u4ResultWakeIntvl) &&
			(prTWTSetup->ucMinWakeDur ==
			prTWTResult->ucMinWakeDur)) {
			fgSetupCheck = TRUE;
		} else if ((u4ResultWakeIntvl >=
			prTWTSetup->u4WakeIntvalMin) &&
			(u4ResultWakeIntvl <=
			prTWTSetup->u4WakeIntvalMax) &&
			(prTWTResult->ucMinWakeDur >=
			prTWTSetup->ucWakeDurMin) &&
			(prTWTResult->ucMinWakeDur <=
			prTWTSetup->ucWakeDurMax)) {
			fgSetupCheck = TRUE;
		}

		if (fgSetupCheck) {
			twtPlannerAddAgrtTbl(prAdapter, prBssInfo, prStaRec,
				prTWTResult, ucTWTFlowId, eTwtType, FALSE,
				NULL, NULL);

			DBGLOG(TWT_PLANNER, STATE,
				"Rx nego id %d\n",
				ucTWTFlowId);
			/* prAdapter->fgEnOnlineScan = FALSE; */
		} else {
			/*
			 * The TWT parameters suggested by AP
			 * are larger than the upper and lower bounds,
			 */
			DBGLOG(TWT_PLANNER, WARN,
				"No to setup TWT, R:[%d %d] S:[%d %d %d %d]\n",
				u4ResultWakeIntvl,
				prTWTResult->ucMinWakeDur,
				u4SetupWakeIntvl,
				prTWTSetup->ucMinWakeDur,
				prTWTSetup->ucWakeDurMin,
				prTWTSetup->ucWakeDurMax);

			twtPlannerSendReqTeardown(prAdapter,
				prStaRec, ucTWTFlowId);
		}

		break;
#endif

	case TWT_SETUP_CMD_ID_ALTERNATE:
	case TWT_SETUP_CMD_ID_DICTATE:
		/* Use AP's suggestions */
		prTWTParams = &(prTWTFlow->rTWTParams);
		kalMemCopy(prTWTParams,
			prTWTResult, sizeof(struct _TWT_PARAMS_T));
		prTWTParams->ucSetupCmd = TWT_SETUP_CMD_ID_SUGGEST;
		prTWTParams->fgReq = 1;
		twtPlannerSendReqStart(prAdapter, prStaRec, ucTWTFlowId);
		break;

	case TWT_SETUP_CMD_ID_REJECT:
		/* Clear TWT flow in StaRec */
#if (CFG_SUPPORT_TWT_STA_CNM == 1)
		DBGLOG(TWT_PLANNER, WARN,
			"Reject, send TWT teardown frame for sync state!!\n");

		twtSendTeardownFrame(
			prAdapter, prStaRec, ucTWTFlowId,
			twtReqFsmRunEventRejectTxDone);
#else
		twtSendTeardownFrame(
			prAdapter, prStaRec, ucTWTFlowId,
			NULL);
#endif

		break;

	default:
		DBGLOG(TWT_PLANNER, ERROR,
			"Unknown setup command %u\n", prTWTResult->ucSetupCmd);
		break;

	}
}

void twtPlannerSuspendDone(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_IND_RESULT_T *prTWTFsmResultMsg;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint8_t ucTWTFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	prTWTFsmResultMsg = (struct _MSG_TWT_REQFSM_IND_RESULT_T *) prMsgHdr;
	prStaRec = prTWTFsmResultMsg->prStaRec;
	ucTWTFlowId = prTWTFsmResultMsg->ucTWTFlowId;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_PLANNER, ERROR,
				"Invalid prStaRec\n");

		return;
	}

	cnmMemFree(prAdapter, prMsgHdr);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return;
	}

	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Rx suspend result: invalid STA Type %d\n",
			prStaRec->eStaType);
		return;
	}

	twtPlannerSuspendAgrtTbl(prAdapter, prBssInfo, prStaRec,
		ucTWTFlowId, FALSE,
		NULL, NULL /* handle TWT cmd timeout? */);
}

void twtPlannerResumeDone(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_IND_RESULT_T *prTWTFsmResultMsg;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint8_t ucTWTFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	prTWTFsmResultMsg = (struct _MSG_TWT_REQFSM_IND_RESULT_T *) prMsgHdr;
	prStaRec = prTWTFsmResultMsg->prStaRec;
	ucTWTFlowId = prTWTFsmResultMsg->ucTWTFlowId;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_PLANNER, ERROR,
				"Invalid prMsgHdr\n");

		return;
	}

	cnmMemFree(prAdapter, prMsgHdr);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return;
	}

	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Rx suspend result: invalid STA Type %d\n",
			prStaRec->eStaType);
		return;
	}

	/* Resume the FW TWT agreement entry */
	twtPlannerResumeAgrtTbl(prAdapter, prBssInfo, prStaRec,
		ucTWTFlowId, FALSE,
		NULL, NULL /* handle TWT cmd timeout? */);

}

void twtPlannerFillResumeData(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId,
	uint64_t u8NextTWT)
{
	struct _TWT_PLANNER_T *prTWTPlanner = &(prAdapter->rTWTPlanner);
	struct _TWT_AGRT_T *prTWTAgrt;
	struct BSS_INFO *prBssInfo;
	uint8_t ucIdx;
	uint8_t ucFlowId_real = ucFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return;
	}

	ucIdx = twtPlannerDrvAgrtFind(
				prAdapter, prBssInfo->ucBssIndex, ucFlowId,
				&ucFlowId_real);

	if (ucIdx >= TWT_AGRT_MAX_NUM) {
		DBGLOG(TWT_PLANNER, ERROR, "Can't find agrt bss %u flow %u\n",
			prBssInfo->ucBssIndex, ucFlowId);
		return;
	}

	ucFlowId = ucFlowId_real;

	prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[ucIdx]);
	prTWTAgrt->rTWTAgrt.u8TWT = u8NextTWT;

	DBGLOG(TWT_REQUESTER, WARN,
			"TWT Info Frame 0x%llx 0x%llx\n",
			prTWTAgrt->rTWTAgrt.u8TWT, u8NextTWT);
}

void twtPlannerTeardownDone(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_IND_RESULT_T *prTWTFsmResultMsg;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint8_t ucTWTFlowId;
#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	struct _TWT_FLOW_T *prTWTFlow;
#endif

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	prTWTFsmResultMsg = (struct _MSG_TWT_REQFSM_IND_RESULT_T *) prMsgHdr;
	prStaRec = prTWTFsmResultMsg->prStaRec;
	ucTWTFlowId = prTWTFsmResultMsg->ucTWTFlowId;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_PLANNER, ERROR,
				"Invalid prStaRec\n");

		return;
	}

	cnmMemFree(prAdapter, prMsgHdr);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return;
	}

	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Rx teardown result: invalid STA Type %d\n",
			prStaRec->eStaType);
		return;
	}

#if (CFG_TWT_SMART_STA == 1)
	g_TwtSmartStaCtrl.fgTwtSmartStaActivated = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaReq = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaTeardownReq = FALSE;
	g_TwtSmartStaCtrl.ucBssIndex = 0;
	g_TwtSmartStaCtrl.ucFlowId = 0;
	g_TwtSmartStaCtrl.u4CurTp = 0;
	g_TwtSmartStaCtrl.u4LastTp = 0;
	g_TwtSmartStaCtrl.u4TwtSwitch = 0;
	g_TwtSmartStaCtrl.eState = TWT_SMART_STA_STATE_IDLE;
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	prTWTFlow = &(prStaRec->arTWTFlow[ucTWTFlowId]);

	if (prTWTFlow->fgIsMLTWT == TRUE) {
		/* MLTWT teardown goes over here */
		mltwtPlannerDelAgrtTbl(
			prAdapter,
			prStaRec,
			ucTWTFlowId);

#if (CFG_SUPPORT_TWT_STA_CNM == 0)
		/* Enable SCAN after TWT agrt has been tear down */
		prAdapter->fgEnOnlineScan = TRUE;
#endif

		return;
	}

	/* i-TWT teardown goes in existing flow */
#endif

	/* Delete driver & FW TWT agreement entry */
	twtPlannerDelAgrtTbl(prAdapter, prBssInfo, prStaRec,
		ucTWTFlowId, FALSE,
		NULL, NULL /* handle TWT cmd timeout? */, TRUE);

	/* Teardown FW TWT agreement entry */
	twtPlannerTeardownAgrtTbl(prAdapter, prStaRec,
		FALSE, NULL, NULL /* handle TWT cmd timeout? */);

#if (CFG_SUPPORT_TWT_STA_CNM == 0)
	/* Enable SCAN after TWT agrt has been tear down */
	prAdapter->fgEnOnlineScan = TRUE;
#endif
}

void twtPlannerRxInfoFrm(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_IND_INFOFRM_T *prTWTFsmInfoFrmMsg;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint8_t ucTWTFlowId;
	struct _NEXT_TWT_INFO_T rNextTWTInfo;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	prTWTFsmInfoFrmMsg = (struct _MSG_TWT_REQFSM_IND_INFOFRM_T *) prMsgHdr;
	prStaRec = prTWTFsmInfoFrmMsg->prStaRec;
	ucTWTFlowId = prTWTFsmInfoFrmMsg->ucTWTFlowId;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_PLANNER, ERROR,
				"Invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Rx info frame: invalid STA Type %d\n",
			prStaRec->eStaType);

		cnmMemFree(prAdapter, prMsgHdr);

		return;
	}

	kalMemCopy(&(rNextTWTInfo), &(prTWTFsmInfoFrmMsg->rNextTWTInfo),
		sizeof(struct _NEXT_TWT_INFO_T));

	cnmMemFree(prAdapter, prMsgHdr);

	/* Modify the TWT agreement entry */
	twtPlannerModifyAgrtTbl(prAdapter, prBssInfo, prStaRec,
		&rNextTWTInfo, ucTWTFlowId, FALSE,
		NULL, NULL /* handle TWT cmd timeout? */);

}

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
void twtHotspotPlannerSetParams(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_HOTSPOT_PARAMS_SET_T *prTWTParamSetMsg;
	struct _TWT_HOTSPOT_CTRL_T rTWTCtrl, *prTWTCtrl = &rTWTCtrl;
	struct _TWT_GET_TSF_CONTEXT_T *prGetTsfCtxt = NULL;
	struct BSS_INFO *prBssInfo = NULL;

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	prTWTParamSetMsg = (struct _MSG_TWT_HOTSPOT_PARAMS_SET_T *) prMsgHdr;
	kalMemCopy(prTWTCtrl, &prTWTParamSetMsg->rTWTCtrl, sizeof(*prTWTCtrl));

	cnmMemFree(prAdapter, prMsgHdr);

	if (prTWTCtrl->ucIsReject == TRUE) {
		/* It is a reject, no need to
		*  calculate the nearest target tsf
		*/
		DBGLOG(TWT_RESPONDER, ERROR,
				"[TWT_RESP]direct reject\n");

		twtHotspotRespFsmRunEventRxSetup(
			prAdapter,
			(void *)prTWTCtrl);
	} else {
		/* It is an accept, use current tsf for
		*  the nearest target tsf
		*/
		prGetTsfCtxt =
			kalMemAlloc(
				sizeof(
					struct _TWT_GET_TSF_CONTEXT_T),
					VIR_MEM_TYPE);

		if (prGetTsfCtxt == NULL) {
			DBGLOG(TWT_PLANNER, ERROR,
				"[TWT_RESP]mem alloc failed\n");

			return;
		}

		prGetTsfCtxt->ucReason = TWT_GET_TSF_FOR_ADD_AGRT;
		prGetTsfCtxt->ucBssIdx = prTWTCtrl->ucBssIdx;
		prGetTsfCtxt->ucTWTFlowId = prTWTCtrl->ucTWTFlowId;
		prGetTsfCtxt->fgIsOid = FALSE;

		prBssInfo = GET_BSS_INFO_BY_INDEX(
					prAdapter,
					prGetTsfCtxt->ucBssIdx);

		ASSERT(prBssInfo);

		kalMemCopy(
			&(prGetTsfCtxt->rTWTParams),
			&(prTWTCtrl->rTWTParams),
			sizeof(struct _TWT_PARAMS_T));

		DBGLOG(TWT_RESPONDER, ERROR,
				"[TWT_RESP]GetCurrentTSF\n");

		twtHotspotPlannerGetCurrentTSF(
			prAdapter, prBssInfo,
			prGetTsfCtxt, sizeof(*prGetTsfCtxt));
	}
}

void twtHotspotPlannerSetupAgrtToFW(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_HOTSPOT_PARAMS_SET_T *prTWTParamSetMsg;
	struct _TWT_HOTSPOT_CTRL_T rTWTCtrl, *prTWTCtrl = &rTWTCtrl;

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	prTWTParamSetMsg = (struct _MSG_TWT_HOTSPOT_PARAMS_SET_T *) prMsgHdr;
	kalMemCopy(prTWTCtrl, &prTWTParamSetMsg->rTWTCtrl, sizeof(*prTWTCtrl));

	cnmMemFree(prAdapter, prMsgHdr);

	twtHotspotPlannerAddAgrtTbl(
			prAdapter,
			prTWTCtrl->prStaRec,
			prTWTCtrl->ucTWTFlowId,
			FALSE,
			NULL,
			NULL);
}

void twtHotspotPlannerTeardownToFW(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_HOTSPOT_PARAMS_SET_T *prTWTParamSetMsg;
	struct _TWT_HOTSPOT_CTRL_T rTWTCtrl, *prTWTCtrl = &rTWTCtrl;

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	prTWTParamSetMsg = (struct _MSG_TWT_HOTSPOT_PARAMS_SET_T *) prMsgHdr;
	kalMemCopy(prTWTCtrl, &prTWTParamSetMsg->rTWTCtrl, sizeof(*prTWTCtrl));

	cnmMemFree(prAdapter, prMsgHdr);

	twtHotspotPlannerTeardownSta(
			prAdapter,
			prTWTCtrl->prStaRec,
			prTWTCtrl->ucTWTFlowId,
			FALSE,
			NULL,
			NULL);
}

uint32_t twtHotspotPlannerGetCurrentTSF(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _EXT_CMD_GET_MAC_INFO_T *prMacInfoCmd;
	struct _EXTRA_ARG_TSF_T *prTsfArg;

	prMacInfoCmd = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_GET_MAC_INFO_T));

	if (!prMacInfoCmd) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]Alloc _EXT_CMD_GET_MAC_INFO_T FAILED.\n");

		return WLAN_STATUS_FAILURE;
	}

	prTsfArg = &(prMacInfoCmd->rExtraArgument.rTsfArg);
	prMacInfoCmd->u2MacInfoId = CPU_TO_LE16(MAC_INFO_TYPE_TSF);
	prTsfArg->ucHwBssidIndex = prBssInfo->ucOwnMacIndex;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
					CMD_ID_LAYER_0_EXT_MAGIC_NUM,
					EXT_CMD_ID_GET_MAC_INFO,
					FALSE,
					TRUE,
					FALSE,
					twtHotspotPlannerGetTsfDone,
					NULL,
					sizeof(struct _EXT_CMD_GET_MAC_INFO_T),
					(uint8_t *) (prMacInfoCmd),
					pvSetBuffer, u4SetBufferLen);

	return rWlanStatus;
}

void twtHotspotPlannerGetTsfDone(
	struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf)
{
	struct EXT_EVENT_MAC_INFO_T *prEventMacInfo = NULL;
	struct _TWT_GET_TSF_CONTEXT_T *prGetTsfCtxt = NULL;
	struct STA_RECORD *prStaRec = NULL;
	struct TSF_RESULT_T *prTsfResult = NULL;
	uint64_t u8CurTsf = 0;
	struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode = NULL;
	struct _TWT_HOTSPOT_CTRL_T *prTWTHotspotCtrl = NULL;
	struct _TWT_PARAMS_T *prTWTHotspotParam = NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	if (!pucEventBuf) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]pucEventBuf is NULL.\n");

		return;
	}

	if (!prCmdInfo->pvInformationBuffer) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]prCmdInfo->pvInformationBuffer is NULL.\n");

		return;
	}

	prEventMacInfo = (struct EXT_EVENT_MAC_INFO_T *) (pucEventBuf);

	prGetTsfCtxt = (struct _TWT_GET_TSF_CONTEXT_T *)
		prCmdInfo->pvInformationBuffer;

	ASSERT(prGetTsfCtxt);

	twtHotspotGetStaRecByFlowId(
		prAdapter,
		prGetTsfCtxt->ucBssIdx,
		prGetTsfCtxt->ucTWTFlowId,
		&prStaRec);

	if (prStaRec != NULL) {
		prTsfResult = &(prEventMacInfo->rMacInfoResult.rTsfResult);

		u8CurTsf = LE32_TO_CPU(prTsfResult->u4TsfBitsLow) |
			(((uint64_t)(LE32_TO_CPU(prTsfResult->
							u4TsfBitsHigh))) << 32);

		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]GetTSF u8CurTsf=%llx, TsfLow=%x, TsfHigh=%x\n",
			u8CurTsf,
			LE32_TO_CPU(prTsfResult->u4TsfBitsLow),
			LE32_TO_CPU(prTsfResult->u4TsfBitsHigh));

		prTWTHotspotStaNode = prStaRec->prTWTHotspotStaNode;

		prTWTHotspotCtrl = &prStaRec->TWTHotspotCtrl;

		prTWTHotspotParam = &prTWTHotspotCtrl->rTWTParams;

		twtHotspotGetNearestTargetTSF(
			prAdapter, prStaRec, prTWTHotspotStaNode, u8CurTsf);

		prTWTHotspotParam->u8TWT =
			prTWTHotspotStaNode->twt_assigned_tsf;

		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]IsReject=%d SetupCmd=%d\n",
			prTWTHotspotCtrl->ucIsReject,
			prTWTHotspotParam->ucSetupCmd);

		twtHotspotRespFsmRunEventRxSetup(
			prAdapter,
			(void *)prTWTHotspotCtrl);
	}

	/* free memory */
	kalMemFree(prGetTsfCtxt, VIR_MEM_TYPE, sizeof(*prGetTsfCtxt));
}

uint32_t
twtHotspotPlannerAddAgrtTbl(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId,
	uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode = NULL;
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *prTWTAgrtUpdate;

	prTWTHotspotStaNode = prStaRec->prTWTHotspotStaNode;

	if (!prTWTHotspotStaNode) {
		DBGLOG(
			TWT_RESPONDER,
			ERROR,
			"[TWT_RESP]No sta node in sta record for flow id = %d\n",
			ucFlowId);

		return WLAN_STATUS_FAILURE;
	}

	prTWTAgrtUpdate = cnmMemAlloc(
		prAdapter,
		RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T));

	if (!prTWTAgrtUpdate) {
		DBGLOG(
			TWT_RESPONDER,
			ERROR,
			"[TWT_RESP]Allocate _EXT_CMD_TWT_ARGT_UPDATE_T ==> FAILED.\n");

		return WLAN_STATUS_FAILURE;
	}

	if (prTWTHotspotStaNode->flow_id != ucFlowId) {
		DBGLOG(
			TWT_RESPONDER,
			ERROR,
			"[TWT_RESP]Flow id mismatch %d vs %d\n",
			prTWTHotspotStaNode->flow_id, ucFlowId);

		cnmMemFree(prAdapter, prTWTAgrtUpdate);

		return WLAN_STATUS_FAILURE;
	}

	prTWTAgrtUpdate->ucAgrtTblIdx = prTWTHotspotStaNode->agrt_tbl_idx;
	prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_ADD;
	prTWTAgrtUpdate->ucOwnMacId = prTWTHotspotStaNode->own_mac_idx;
	prTWTAgrtUpdate->ucFlowId = prTWTHotspotStaNode->flow_id;
	prTWTAgrtUpdate->u2PeerIdGrpId =
		(prStaRec) ? CPU_TO_LE16(prStaRec->ucWlanIndex) : 1;
	prTWTAgrtUpdate->ucAgrtSpDuration =
		prTWTHotspotStaNode->agrt_sp_duration;
	prTWTAgrtUpdate->ucBssIndex = prTWTHotspotStaNode->bss_idx;
	prTWTAgrtUpdate->u4AgrtSpStartTsfLow =
		CPU_TO_LE32(prTWTHotspotStaNode->twt_assigned_tsf & 0xFFFFFFFF);
	prTWTAgrtUpdate->u4AgrtSpStartTsfHigh =
		CPU_TO_LE32((uint32_t)
		(prTWTHotspotStaNode->twt_assigned_tsf >> 32));
	prTWTAgrtUpdate->u2AgrtSpWakeIntvlMantissa =
		CPU_TO_LE16(prTWTHotspotStaNode->agrt_sp_wake_intvl_mantissa);
	prTWTAgrtUpdate->ucAgrtSpWakeIntvlExponent =
	prTWTHotspotStaNode->agrt_sp_wake_intvl_exponent;
	prTWTAgrtUpdate->ucIsRoleAp = TWT_ROLE_HOTSPOT;  /* TWT hotspot role */
	prTWTAgrtUpdate->ucAgrtParaBitmap = 0;
	prTWTAgrtUpdate->ucGrpMemberCnt = 0;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
		CMD_ID_LAYER_0_EXT_MAGIC_NUM,
		EXT_CMD_ID_TWT_AGRT_UPDATE,
		TRUE,
		FALSE,
		fgIsOid,
		pfCmdDoneHandler,
		pfCmdTimeoutHandler,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
		(uint8_t *) (prTWTAgrtUpdate),
		NULL, 0);

	DBGLOG(
		TWT_RESPONDER,
		ERROR,
		"[TWT_RESP]AgrtTblIdx = %d, OwnMacId = %d, FlowId = %d, PeerIdGrpId = %d, BssIdx = %d, Status=%x\n",
		prTWTAgrtUpdate->ucAgrtTblIdx,
		prTWTAgrtUpdate->ucOwnMacId,
		prTWTAgrtUpdate->ucFlowId,
		prTWTAgrtUpdate->u2PeerIdGrpId,
		prTWTAgrtUpdate->ucBssIndex,
		rWlanStatus);

	DBGLOG(
		TWT_RESPONDER,
		ERROR,
		"[TWT_RESP]SpDuration=%d, TsfLow=%x, TsfHigh=%x, Mantissa=%d, Exp=%d\n",
		prTWTAgrtUpdate->ucAgrtSpDuration,
		prTWTAgrtUpdate->u4AgrtSpStartTsfLow,
		prTWTAgrtUpdate->u4AgrtSpStartTsfHigh,
		prTWTAgrtUpdate->u2AgrtSpWakeIntvlMantissa,
		prTWTAgrtUpdate->ucAgrtSpWakeIntvlExponent);

	cnmMemFree(prAdapter, prTWTAgrtUpdate);

	return rWlanStatus;
}

uint32_t
twtHotspotPlannerTeardownSta(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId,
	uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode = NULL;
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *prTWTAgrtUpdate;

	prTWTHotspotStaNode = prStaRec->prTWTHotspotStaNode;

	if (prTWTHotspotStaNode == NULL) {
		/* TWT hotspot might receive teardown before disconnect */
		DBGLOG(
			TWT_PLANNER,
			ERROR,
			"[TWT_RESP]no sta node for flow id = %d\n",
			ucFlowId);

		twtHotspotRespFsmSteps(
			prAdapter,
			prStaRec,
			TWT_HOTSPOT_RESP_STATE_IDLE_BY_FORCE,
			ucFlowId,
			NULL);

		return WLAN_STATUS_SUCCESS;
	}

	prTWTAgrtUpdate = cnmMemAlloc(
		prAdapter,
		RAM_TYPE_MSG,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T));

	if (!prTWTAgrtUpdate) {
		DBGLOG(
			TWT_PLANNER,
			ERROR,
			"[TWT_RESP]Allocate _EXT_CMD_TWT_ARGT_UPDATE_T ==> FAILED.\n");

		return WLAN_STATUS_FAILURE;
	}

	if (prTWTHotspotStaNode->flow_id != ucFlowId) {
		DBGLOG(
			TWT_PLANNER,
			ERROR,
			"[TWT_RESP]Flow id mismatch %d vs %d\n",
			prTWTHotspotStaNode->flow_id, ucFlowId);

		cnmMemFree(prAdapter, prTWTAgrtUpdate);

		return WLAN_STATUS_FAILURE;
	}

	/* 1. send teardown command to F/W */
	/* 1-1. TWT_AGRT_CTRL_DELETE
	*          - deregister tsf timer & defect RRselection
	*/
	prTWTAgrtUpdate->ucAgrtTblIdx = prTWTHotspotStaNode->agrt_tbl_idx;
	prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_DELETE;
	prTWTAgrtUpdate->ucOwnMacId = prTWTHotspotStaNode->own_mac_idx;
	prTWTAgrtUpdate->ucFlowId = prTWTHotspotStaNode->flow_id;
	prTWTAgrtUpdate->u2PeerIdGrpId = (prStaRec) ?
		CPU_TO_LE16(prStaRec->ucWlanIndex) : 1;
	prTWTAgrtUpdate->ucIsRoleAp = TWT_ROLE_HOTSPOT;  /* TWT hotspot role */
	prTWTAgrtUpdate->ucBssIndex = prTWTHotspotStaNode->bss_idx;

	rWlanStatus = wlanSendSetQueryExtCmd(
		prAdapter,
		CMD_ID_LAYER_0_EXT_MAGIC_NUM,
		EXT_CMD_ID_TWT_AGRT_UPDATE,
		TRUE,
		FALSE,
		fgIsOid,
		pfCmdDoneHandler,
		pfCmdTimeoutHandler,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
		(uint8_t *) (prTWTAgrtUpdate),
		NULL, 0);

	DBGLOG(
		TWT_RESPONDER,
		ERROR,
		"[TWT_RESP]TWT_AGRT_CTRL_DELETE(%d) %d %d %d %d %d\n",
		rWlanStatus,
		prTWTAgrtUpdate->ucAgrtTblIdx,
		prTWTAgrtUpdate->ucBssIndex,
		prTWTAgrtUpdate->ucOwnMacId,
		prTWTAgrtUpdate->ucFlowId,
		prTWTAgrtUpdate->u2PeerIdGrpId);

	/* 1-2. TWT_AGRT_CTRL_TEARDOWN - reset STA to non-TWT STA */
	prTWTAgrtUpdate->ucAgrtCtrlFlag = TWT_AGRT_CTRL_TEARDOWN;

	rWlanStatus = wlanSendSetQueryExtCmd(
		prAdapter,
		CMD_ID_LAYER_0_EXT_MAGIC_NUM,
		EXT_CMD_ID_TWT_AGRT_UPDATE,
		TRUE,
		FALSE,
		fgIsOid,
		pfCmdDoneHandler,
		pfCmdTimeoutHandler,
		sizeof(struct _EXT_CMD_TWT_ARGT_UPDATE_T),
		(uint8_t *) (prTWTAgrtUpdate),
		NULL, 0);

	DBGLOG(
		TWT_RESPONDER,
		ERROR,
		"[TWT_RESP]TWT_AGRT_CTRL_TEARDOWN(%d) %d %d %d %d %d\n",
		rWlanStatus,
		prTWTAgrtUpdate->ucAgrtTblIdx,
		prTWTAgrtUpdate->ucBssIndex,
		prTWTAgrtUpdate->ucOwnMacId,
		prTWTAgrtUpdate->ucFlowId,
		prTWTAgrtUpdate->u2PeerIdGrpId);

	cnmMemFree(prAdapter, prTWTAgrtUpdate);

	/* 2. reset  this Hotsopt's STA station record */
	twtHotspotRespFsmSteps(
		prAdapter,
		prStaRec,
		TWT_HOTSPOT_RESP_STATE_IDLE,
		ucFlowId,
		NULL);

	return rWlanStatus;
}
#endif

#if (CFG_SUPPORT_BTWT == 1)
uint32_t
btwtPlannerSendReqStart(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId)
{
	struct _MSG_TWT_REQFSM_START_T *prTWTReqFsmStartMsg;

	prTWTReqFsmStartMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_START_T));
	if (prTWTReqFsmStartMsg) {
		prTWTReqFsmStartMsg->rMsgHdr.eMsgId = MID_BTWT_REQ_FSM_START;
		prTWTReqFsmStartMsg->prStaRec = prStaRec;
		prTWTReqFsmStartMsg->ucTWTFlowId = ucTWTFlowId;

		prStaRec->arBTWTFlow[ucTWTFlowId].eBtwtState
			= ENUM_BTWT_FLOW_STATE_REQUESTING;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prTWTReqFsmStartMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

uint32_t
btwtPlannerSendReqTeardown(struct ADAPTER *prAdapter,
				struct STA_RECORD *prStaRec,
				uint8_t ucTWTFlowId)
{
	struct _MSG_TWT_REQFSM_TEARDOWN_T *prTWTReqFsmTeardownMsg;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prTWTReqFsmTeardownMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_TEARDOWN_T));
	if (prTWTReqFsmTeardownMsg) {
		prTWTReqFsmTeardownMsg->rMsgHdr.eMsgId =
			MID_BTWT_REQ_FSM_TEARDOWN;
		prTWTReqFsmTeardownMsg->prStaRec = prStaRec;
		prTWTReqFsmTeardownMsg->ucTWTFlowId = ucTWTFlowId;
		prTWTReqFsmTeardownMsg->fgTeardownAll = 0;

		mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prTWTReqFsmTeardownMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

void btwtPlannerTeardownDone(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_IND_RESULT_T *prTWTFsmResultMsg;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	struct _TWT_FLOW_T *prTwtFlow;
	uint8_t ucTWTFlowId, ucIdx;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	prTWTFsmResultMsg = (struct _MSG_TWT_REQFSM_IND_RESULT_T *) prMsgHdr;
	prStaRec = prTWTFsmResultMsg->prStaRec;
	ucTWTFlowId = prTWTFsmResultMsg->ucTWTFlowId;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_PLANNER, ERROR,
				"Invalid prStaRec\n");

		return;
	}

	cnmMemFree(prAdapter, prMsgHdr);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return;
	}

	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Rx teardown result: invalid STA Type %d\n",
			prStaRec->eStaType);
		return;
	}

	if (GET_TWT_TEARDOWN_ALL(ucTWTFlowId) == 0x1) {
		DBGLOG(TWT_PLANNER, ERROR,
			"BTWT teardown %d\n", ucTWTFlowId);

		for (ucIdx = 1; ucIdx < TWT_MAX_FLOW_NUM; ucIdx++) {
			prTwtFlow = &prStaRec->arBTWTFlow[ucIdx];
			if (prTwtFlow->eBtwtState
				== ENUM_BTWT_FLOW_STATE_ACTIVATED) {
				prTwtFlow->eBtwtState
					= ENUM_BTWT_FLOW_STATE_DEFAULT;
				prTwtFlow->eTwtType = ENUM_TWT_TYPE_DEFAULT;

				/* Delete driver & FW TWT agreement entry */
				/* handle TWT cmd timeout? */
				twtPlannerDelAgrtTbl(prAdapter, prBssInfo,
					prStaRec, ucIdx, FALSE,
					NULL, NULL, TRUE);

				/* Teardown FW TWT agreement entry */
				/* handle TWT cmd timeout? */
				twtPlannerTeardownAgrtTbl(prAdapter, prStaRec,
					FALSE, NULL, NULL);
			}
		}
	} else {
		ucIdx = (ucTWTFlowId & TWT_TEARDOWN_FLOW_ID);

		DBGLOG(TWT_PLANNER, ERROR,
			"BTWT teardown1 %d\n", ucIdx);
		/* Delete driver & FW TWT agreement entry */
		twtPlannerDelAgrtTbl(prAdapter, prBssInfo, prStaRec,
			ucIdx, FALSE,
			NULL, NULL /* handle TWT cmd timeout? */, TRUE);

		/* Teardown FW TWT agreement entry */
		twtPlannerTeardownAgrtTbl(prAdapter, prStaRec,
			FALSE, NULL, NULL /* handle TWT cmd timeout? */);

		prStaRec->arBTWTFlow[ucIdx].eBtwtState =
			ENUM_BTWT_FLOW_STATE_DEFAULT;
		prStaRec->arBTWTFlow[ucIdx].eTwtType = ENUM_TWT_TYPE_DEFAULT;
	}
}

uint32_t
btwtPlannerAddAgrtTbl(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct _TWT_PARAMS_T *prTWTParams,
	uint8_t ucFlowId,
	uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	if (!IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucBTWTSupport)) {
		DBGLOG(TWT_PLANNER, ERROR,
			"BTWT support %d\n",
			prAdapter->rWifiVar.ucBTWTSupport);

		return WLAN_STATUS_FAILURE;
	}

	return twtPlannerAddAgrtTbl(
	prAdapter, prBssInfo, prStaRec, prTWTParams,
		ucFlowId, ENUM_TWT_TYPE_BTWT, fgIsOid, pfCmdDoneHandler,
		pfCmdTimeoutHandler);
}

void
btwtPlannerDelAgrtTbl(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId)
{
	DBGLOG(TWT_PLANNER, ERROR,
		"BTWT teardown2 %d\n", ucFlowId);

	/* Delete driver & FW TWT agreement entry */
	twtPlannerDelAgrtTbl(prAdapter, prBssInfo, prStaRec,
		ucFlowId, FALSE,
		NULL, NULL /* handle TWT cmd timeout? */, TRUE);

	/* Teardown FW TWT agreement entry */
	twtPlannerTeardownAgrtTbl(prAdapter, prStaRec,
		FALSE, NULL, NULL /* handle TWT cmd timeout? */);

}

#endif

#if (CFG_SUPPORT_RTWT == 1)
uint32_t rtwtPlannerSendReqStart(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum ENUM_MSG_ID eMsgId,
	uint8_t ucTWTFlowId)
{
	struct _MSG_TWT_REQFSM_START_T *prTWTReqFsmStartMsg;

	prTWTReqFsmStartMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_START_T));
	if (prTWTReqFsmStartMsg) {
		prTWTReqFsmStartMsg->rMsgHdr.eMsgId = eMsgId;
		prTWTReqFsmStartMsg->prStaRec = prStaRec;
		prTWTReqFsmStartMsg->ucTWTFlowId = ucTWTFlowId;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prTWTReqFsmStartMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

uint32_t rtwtPlannerSendReqTeardown(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	u_int8_t fgTeardownAll)
{
	struct _MSG_TWT_REQFSM_TEARDOWN_T *prTWTReqFsmTeardownMsg;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prTWTReqFsmTeardownMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_TEARDOWN_T));
	if (prTWTReqFsmTeardownMsg) {
		prTWTReqFsmTeardownMsg->rMsgHdr.eMsgId =
			MID_RTWT_REQ_FSM_TEARDOWN;
		prTWTReqFsmTeardownMsg->prStaRec = prStaRec;
		prTWTReqFsmTeardownMsg->ucTWTFlowId = ucTWTFlowId;
		prTWTReqFsmTeardownMsg->fgTeardownAll = fgTeardownAll;

		mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prTWTReqFsmTeardownMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

void rtwtPlannerTearingdown(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucAgrtTblIdx;
	struct _TWT_PARAMS_T rTWTParams;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (prBssInfo == NULL) {
		DBGLOG(TWT_PLANNER, ERROR, "No bssinfo to teardown\n");

		return;
	}

	rWlanStatus = twtPlannerDrvAgrtGet(
					prAdapter, prBssInfo->ucBssIndex,
					ucFlowId, &ucAgrtTblIdx, &rTWTParams);

	if (rWlanStatus) {
		DBGLOG(TWT_PLANNER, ERROR,
			"No agrt to suspend Bss %u flow %u\n",
			prBssInfo->ucBssIndex, ucFlowId);

		return;
	}

#if (CFG_TWT_SMART_STA == 1)
	g_TwtSmartStaCtrl.fgTwtSmartStaActivated = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaReq = FALSE;
	g_TwtSmartStaCtrl.fgTwtSmartStaTeardownReq = FALSE;
	g_TwtSmartStaCtrl.ucBssIndex = 0;
	g_TwtSmartStaCtrl.ucFlowId = 0;
	g_TwtSmartStaCtrl.u4CurTp = 0;
	g_TwtSmartStaCtrl.u4LastTp = 0;
	g_TwtSmartStaCtrl.u4TwtSwitch = 0;
	g_TwtSmartStaCtrl.eState = TWT_SMART_STA_STATE_IDLE;
#endif

	/* Delete driver & FW TWT agreement entry */
	rWlanStatus = twtPlannerDelAgrtTbl(prAdapter,
		prBssInfo, prStaRec, ucFlowId, FALSE,
		NULL, NULL /* handle TWT cmd timeout? */, TRUE);

	/* Teardown FW TWT agreement entry */
	if (rWlanStatus == WLAN_STATUS_SUCCESS)
		twtPlannerTeardownAgrtTbl(prAdapter,
			prStaRec, FALSE, NULL,
			NULL /* handle TWT cmd timeout? */);

	prStaRec->arRTWTFlow[ucFlowId].eTwtType = ENUM_TWT_TYPE_DEFAULT;
}

void rtwtPlannerTeardownDone(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_IND_RESULT_T *prTWTFsmResultMsg;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	struct _TWT_FLOW_T *prTwtFlow;
	uint8_t ucTWTFlowId, ucIdx;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	prTWTFsmResultMsg = (struct _MSG_TWT_REQFSM_IND_RESULT_T *) prMsgHdr;
	prStaRec = prTWTFsmResultMsg->prStaRec;
	ucTWTFlowId = prTWTFsmResultMsg->ucTWTFlowId;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_PLANNER, ERROR,
				"Invalid prStaRec\n");

		return;
	}

	cnmMemFree(prAdapter, prMsgHdr);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return;
	}

	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Rx teardown result: invalid STA Type %d\n",
			prStaRec->eStaType);
		return;
	}

	/*
	 * Do we have teardown from AP in RTWT, just like BTWT?
	 * During WFA plugfest, rolling update W.R.T WFA test script
	 */
	if (GET_TWT_TEARDOWN_ALL(ucTWTFlowId) == 0x1) {
		DBGLOG(TWT_PLANNER, ERROR,
			"RTWT teardown %d\n", ucTWTFlowId);

		for (ucIdx = 1; ucIdx < TWT_MAX_FLOW_NUM; ucIdx++) {
			prTwtFlow = &prStaRec->arRTWTFlow[ucIdx];
			if (prTwtFlow->eTwtType
				== ENUM_TWT_TYPE_RTWT) {
				prTwtFlow->eTwtType = ENUM_TWT_TYPE_DEFAULT;

				/* Delete driver & FW TWT agreement entry */
				/* handle TWT cmd timeout? */
				twtPlannerDelAgrtTbl(prAdapter, prBssInfo,
					prStaRec, ucIdx, FALSE,
					NULL, NULL, TRUE);

				/* Teardown FW TWT agreement entry */
				/* handle TWT cmd timeout? */
				twtPlannerTeardownAgrtTbl(prAdapter, prStaRec,
					FALSE, NULL, NULL);
			}
		}
	} else {
		ucIdx = (ucTWTFlowId & TWT_TEARDOWN_FLOW_ID);

		DBGLOG(TWT_PLANNER, ERROR,
			"RTWT teardown1 %d\n", ucIdx);
		/* Delete driver & FW TWT agreement entry */
		twtPlannerDelAgrtTbl(prAdapter, prBssInfo, prStaRec,
			ucIdx, FALSE,
			NULL, NULL /* handle TWT cmd timeout? */, TRUE);

		/* Teardown FW TWT agreement entry */
		twtPlannerTeardownAgrtTbl(prAdapter, prStaRec,
			FALSE, NULL, NULL /* handle TWT cmd timeout? */);

		prStaRec->arRTWTFlow[ucIdx].eTwtType = ENUM_TWT_TYPE_DEFAULT;
	}
}

uint32_t rtwtPlannerAddAgrtTbl(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct _TWT_PARAMS_T *prTWTParams,
	uint8_t ucFlowId,
	uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	if ((!IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucRTWTSupport)) ||
		(!IS_FEATURE_ENABLED(
			prAdapter->rWifiVar.ucRTWTStautProtect))) {
		DBGLOG(TWT_PLANNER, ERROR,
			"RTWT support|protection %d|%d\n",
			prAdapter->rWifiVar.ucRTWTSupport,
			prAdapter->rWifiVar.ucRTWTStautProtect);

		return WLAN_STATUS_FAILURE;
	}

	return twtPlannerAddAgrtTbl(
	prAdapter, prBssInfo, prStaRec, prTWTParams,
		ucFlowId, ENUM_TWT_TYPE_RTWT, fgIsOid, pfCmdDoneHandler,
		pfCmdTimeoutHandler);
}

void rtwtPlannerDelAgrtTbl(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId)
{
	DBGLOG(TWT_PLANNER, ERROR,
		"RTWT teardown2 %d\n", ucFlowId);

	/* Delete driver & FW TWT agreement entry */
	twtPlannerDelAgrtTbl(prAdapter, prBssInfo, prStaRec,
		ucFlowId, FALSE,
		NULL, NULL /* handle TWT cmd timeout? */, TRUE);

	/* Teardown FW TWT agreement entry */
	twtPlannerTeardownAgrtTbl(prAdapter, prStaRec,
		FALSE, NULL, NULL /* handle TWT cmd timeout? */);
}
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
uint32_t mltwtPlannerSendReqStartAllLinks(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId)
{
	struct _MSG_TWT_REQFSM_START_T *prTWTReqFsmStartMsg;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prTWTReqFsmStartMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_START_T));
	if (prTWTReqFsmStartMsg) {
		prTWTReqFsmStartMsg->rMsgHdr.eMsgId = MID_ML_TWT_REQ_FSM_START_ALL_LINKS;
		prTWTReqFsmStartMsg->prStaRec = prStaRec;
		prTWTReqFsmStartMsg->ucTWTFlowId = ucTWTFlowId;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prTWTReqFsmStartMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

uint32_t mltwtPlannerSendReqStart(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId)
{
	struct _MSG_TWT_REQFSM_START_T *prTWTReqFsmStartMsg;

	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prTWTReqFsmStartMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_START_T));
	if (prTWTReqFsmStartMsg) {
		prTWTReqFsmStartMsg->rMsgHdr.eMsgId = MID_ML_TWT_REQ_FSM_START_ONE_BY_ONE;
		prTWTReqFsmStartMsg->prStaRec = prStaRec;
		prTWTReqFsmStartMsg->ucTWTFlowId = ucTWTFlowId;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prTWTReqFsmStartMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

void mltwtPlannerRxNegoResult(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId)
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct LINK *prBssList = NULL;
	struct BSS_INFO *prCurrBssInfo = NULL;
	struct STA_RECORD *prStaRecOfAP = NULL;
	struct _TWT_FLOW_T *prTWTFlow = NULL;
	struct _TWT_PARAMS_T *prTWTResult = NULL;

	/* Get MLD_BSS_INFO in MLO connection */
	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"ML Invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"ML Invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(
					prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"ML Invalid prBssInfo\n");

		return;
	}

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"ML Invalid MLD_BSS_INFO\n");

		return;
	}

	/* Iterate each BSS_INFO in MLD_BSS_INFO */
	prBssList = &prMldBssInfo->rBssList;

	LINK_FOR_EACH_ENTRY(prCurrBssInfo, prBssList,
			rLinkEntryMld,
			struct BSS_INFO) {
		if (!prCurrBssInfo)
			break;

		prStaRecOfAP = prCurrBssInfo->prStaRecOfAP;

		if (!prStaRecOfAP)
			break;

		/* Get RX Nego result in STA_REC */
		prTWTFlow = &(prStaRecOfAP->arTWTFlow[ucTWTFlowId]);

		/* if this STA_REC is not MLTWT, continue */
		if (prTWTFlow->fgIsMLTWT != TRUE)
			continue;

		/*
		* We are MLTWT, proceed with AP response TWT param,
		* add agreement to F/W, only TWT_SETUP_CMD_ID_ACCEPT
		* is manipulated during dev stage, TBD...
		*/
		prTWTResult = &(prTWTFlow->rTWTPeerParams);

		switch (prTWTResult->ucSetupCmd) {
		case TWT_SETUP_CMD_ID_ACCEPT:
			/* Update agreement table */
			twtPlannerAddAgrtTbl(prAdapter, prCurrBssInfo, prStaRecOfAP,
				prTWTResult, ucTWTFlowId, ENUM_TWT_TYPE_MLTWT,
				FALSE, NULL, NULL
				/* handle TWT cmd timeout? */);

			DBGLOG(TWT_PLANNER, STATE,
				"Rx nego id %d link ID %d\n",
				ucTWTFlowId,
				prStaRec->ucLinkIndex);

			/* Disable SCAN during TWT activity */
			prAdapter->fgEnOnlineScan = FALSE;

			break;

		case TWT_SETUP_CMD_ID_ALTERNATE:
		case TWT_SETUP_CMD_ID_DICTATE:
		case TWT_SETUP_CMD_ID_REJECT:
		default:
			DBGLOG(TWT_PLANNER, STATE,
				"MLTWT setup command %u don't care\n",
				prTWTResult->ucSetupCmd);

			break;
		}
	}
}

void mltwtPlannerDelAgrtTbl(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId)
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct LINK *prBssList = NULL;
	struct BSS_INFO *prCurrBssInfo = NULL;
	struct STA_RECORD *prStaRecOfAP = NULL;
	struct _TWT_FLOW_T *prTWTFlow = NULL;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	/* Get MLD_BSS_INFO in MLO connection */
	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"ML Invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_PLANNER, ERROR,
			"ML Invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(
					prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"ML Invalid prBssInfo\n");

		return;
	}

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"ML Invalid MLD_BSS_INFO\n");

		return;
	}

	/* Iterate each BSS_INFO in MLD_BSS_INFO */
	prBssList = &prMldBssInfo->rBssList;

	LINK_FOR_EACH_ENTRY(prCurrBssInfo, prBssList,
			rLinkEntryMld,
			struct BSS_INFO) {
		if (!prCurrBssInfo)
			break;

		prStaRecOfAP = prCurrBssInfo->prStaRecOfAP;

		if (!prStaRecOfAP)
			break;

		/* Get _TWT_FLOW_T in STA_REC */
		prTWTFlow = &(prStaRecOfAP->arTWTFlow[ucTWTFlowId]);

		/* if this STA_REC is not MLTWT, continue */
		if (prTWTFlow->fgIsMLTWT != TRUE)
			continue;

		/* Delete driver & FW TWT agreement entry */
		rWlanStatus = twtPlannerDelAgrtTbl(prAdapter,
			prCurrBssInfo, prStaRecOfAP, ucTWTFlowId, FALSE,
			NULL, NULL /* handle TWT cmd timeout? */, TRUE);

		/* Teardown FW TWT agreement entry */
		if (rWlanStatus == WLAN_STATUS_SUCCESS)
			twtPlannerTeardownAgrtTbl(prAdapter,
				prStaRecOfAP, FALSE, NULL,
				NULL /* handle TWT cmd timeout? */);

		prTWTFlow->fgIsMLTWT = FALSE;
		prTWTFlow->eTwtType = ENUM_TWT_TYPE_DEFAULT;
	}

	/* Enable SCAN after TWT agrt has been tear down */
	prAdapter->fgEnOnlineScan = TRUE;
}

#endif
