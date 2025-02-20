// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "roaming_fsm.c"
 *    \brief  This file defines the FSM for Roaming MODULE.
 *
 *    This file defines the FSM for Roaming MODULE.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

#if CFG_SUPPORT_ROAMING
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

#define TX_ACTION_RETRY_LIMIT                  2

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static const char * const apucDebugRoamingState[ROAMING_STATE_NUM] = {
	"IDLE",
	"DECISION",
	"DISCOVERY",
	"ROAM",
	"HANDLE_NEW_CANDIDATE",
	"SEND_WNM_RESP",
	"SEND_FT_REQUEST",
	"WAIT_FT_RESPONSE",
};

static const char * const apucEvent[ROAMING_EVENT_NUM + 1] = {
	"START",
	"DISCOVERY",
	"ROAM",
	"FAIL",
	"ABORT",
	"THRESHOLD_UPDATE",
	"NUM",
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

uint32_t roamingFsmCheckTxFtActionFrame(struct ADAPTER *prAdapter,
			struct MSDU_INFO *prMsduInfo)
{
	struct ACTION_FT_REQ_ACTION_FRAME *prTxFrame;
	struct STA_RECORD *prStaRec;
	uint16_t u2TxFrameCtrl;

	prTxFrame = (struct ACTION_FT_REQ_ACTION_FRAME *)(prMsduInfo->prPacket);
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prStaRec)
		return WLAN_STATUS_INVALID_PACKET;

	u2TxFrameCtrl = prTxFrame->u2FrameCtrl;
	u2TxFrameCtrl &= MASK_FRAME_TYPE;
	if (u2TxFrameCtrl != MAC_FRAME_ACTION ||
	    prTxFrame->ucCategory != CATEGORY_FT_ACTION ||
	    prTxFrame->ucAction != ACTION_FT_REQUEST) {
		DBGLOG(WNM, INFO,
			"Check fail ctrl=0x%x category=%d action=%d\n",
			u2TxFrameCtrl, prTxFrame->ucCategory,
			prTxFrame->ucAction);
		return WLAN_STATUS_FAILURE;
	}

	return WLAN_STATUS_SUCCESS;
}

static uint32_t roamingFtActionTxDone(struct ADAPTER *prAdapter,
				     struct MSDU_INFO *prMsduInfo,
				     enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct ROAMING_INFO *prRoam;
	uint8_t ucBssIndex;
	enum ENUM_ROAMING_STATE eNextState;

	DBGLOG(WNM, INFO, "FT action Tx Done Status %d\n", rTxDoneStatus);
	ucBssIndex = prMsduInfo->ucBssIndex;
	prRoam = aisGetRoamingInfo(prAdapter, ucBssIndex);
	eNextState = prRoam->eCurrentState;

	switch (prRoam->eCurrentState) {
	case ROAMING_STATE_SEND_FT_REQUEST:
		if (roamingFsmCheckTxFtActionFrame(prAdapter, prMsduInfo) !=
					 WLAN_STATUS_SUCCESS)
			break;

		if (rTxDoneStatus == TX_RESULT_SUCCESS) {
			eNextState = ROAMING_STATE_WAIT_FT_RESPONSE;
			cnmTimerStopTimer(prAdapter,
				&prRoam->rTxReqDoneRxRespTimer);
			cnmTimerStartTimer(prAdapter,
			    &prRoam->rTxReqDoneRxRespTimer,
			    TU_TO_MSEC(
			    TX_ACTION_RESPONSE_TIMEOUT_TU));
		}

		/* if TX was successful, change to next state.
		 * if TX was failed, do retry if possible.
		 */
		roamingFsmSteps(prAdapter, eNextState, ucBssIndex);
		break;
	default:
		break;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t roamingFsmSendFtActionFrame(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, struct BSS_DESC_SET *prRoamTarget)
{
	struct MSDU_INFO *prMsduInfo;
	struct BSS_INFO *prBssInfo;
	struct ACTION_FT_REQ_ACTION_FRAME *prTxFrame;
	struct FT_IES *prFtIEs;
	uint8_t *pos;
	uint16_t u2RsnLen = 0, u2FTLen = 0, u2MDLen = 0;
	struct BSS_DESC *prBssDesc = prRoamTarget->prMainBssDesc;
	const uint8_t *aucTargetApAddr;
	const uint8_t *aucStaAddr;

	if (!prStaRec) {
		DBGLOG(ROAMING, INFO, "FT: No station record found\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(ROAMING, INFO,
			"FT: invalid BSS_INFO %d\n", prStaRec->ucBssIndex);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	aucStaAddr = prBssInfo->aucOwnMacAddr;
	aucTargetApAddr = prBssDesc->aucBSSID;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	/* set to FTR addr */
	if (mldIsSingleLinkEnabled(prAdapter, NETWORK_TYPE_AIS,
		prStaRec->ucBssIndex) && prBssDesc->rMlInfo.fgValid)
		aucTargetApAddr = prBssDesc->rMlInfo.aucMldAddr;
#endif

	prFtIEs = aisGetFtIe(prAdapter, prStaRec->ucBssIndex, AIS_FT_R0);

	if (!prFtIEs) {
		DBGLOG(ROAMING, INFO,
			"FT: Bss%d No FT info\n", prStaRec->ucBssIndex);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (prFtIEs->prRsnIE)
		u2RsnLen = IE_SIZE(prFtIEs->prRsnIE);
	if (prFtIEs->prMDIE)
		u2MDLen = IE_SIZE(prFtIEs->prMDIE);
	if (prFtIEs->prFTIE)
		u2FTLen = IE_SIZE(prFtIEs->prFTIE);

	/* 1 Allocate MSDU Info, reserved 255 for ML IE */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
		MAC_TX_RESERVED_FIELD +	u2RsnLen + u2FTLen + u2MDLen + 255);
	if (!prMsduInfo)
		return WLAN_STATUS_RESOURCES;

	prTxFrame = (struct ACTION_FT_REQ_ACTION_FRAME *)
		((uintptr_t)(prMsduInfo->prPacket) + MAC_TX_RESERVED_FIELD);
	pos = (uint8_t *)prTxFrame;

	/* 2 Compose The Mac Header. */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	prTxFrame->ucCategory = CATEGORY_FT_ACTION;
	prTxFrame->ucAction = ACTION_FT_REQUEST;

	COPY_MAC_ADDR(prTxFrame->aucStaAddr, aucStaAddr);
	COPY_MAC_ADDR(prTxFrame->aucTargetApAddr, aucTargetApAddr);

	pos += sizeof(struct ACTION_FT_REQ_ACTION_FRAME);

	if (prFtIEs->prRsnIE) {
		kalMemCopy(pos, (uint8_t *)prFtIEs->prRsnIE, u2RsnLen);
		pos += u2RsnLen;
	}

	if (prFtIEs->prMDIE) {
		kalMemCopy(pos, (uint8_t *)prFtIEs->prMDIE, u2MDLen);
		pos += u2MDLen;
	}

	if (prFtIEs->prFTIE) {
		kalMemCopy(pos, (uint8_t *)prFtIEs->prFTIE, u2FTLen);
		pos += u2FTLen;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (mldIsSingleLinkEnabled(prAdapter, NETWORK_TYPE_AIS,
		prStaRec->ucBssIndex) && prBssDesc->rMlInfo.fgValid) {
		struct IE_MULTI_LINK_CONTROL *common;
		struct MLD_BSS_INFO *mld_bssinfo;

		common = (struct IE_MULTI_LINK_CONTROL *) pos;
		common->ucId = ELEM_ID_RESERVED;
		common->ucLength = 10;
		common->ucExtId = ELEM_EXT_ID_MLD;

		BE_SET_ML_CTRL_TYPE(common->u2Ctrl, ML_CTRL_TYPE_BASIC);

		/* filling common info field*/
		pos = common->aucCommonInfo;
		*pos++ = 7; /* common info length */

		mld_bssinfo = mldBssGetByBss(prAdapter, prBssInfo);
		if (mld_bssinfo) {
			COPY_MAC_ADDR(pos, mld_bssinfo->aucOwnMldAddr);
			pos += MAC_ADDR_LEN;
		}
	}
#endif

	DBGDUMP_MEM8(ROAMING, INFO, "FT request\n",
		prMsduInfo->prPacket, pos - (uint8_t *)prTxFrame);

	nicTxSetPktLifeTime(prAdapter, prMsduInfo,
		AIS_ACTION_FRAME_TX_LIFE_TIME_MS);
	nicTxSetPktRetryLimit(prMsduInfo, TX_DESC_TX_COUNT_NO_LIMIT);
	nicTxSetForceRts(prMsduInfo, TRUE);

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     pos - (uint8_t *)prTxFrame,
		     roamingFtActionTxDone, MSDU_RATE_MODE_AUTO);

	nicTxConfigPktControlFlag(prMsduInfo, MSDU_CONTROL_FLAG_FORCE_TX, TRUE);

	/* 5 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

uint32_t roamingFsmCheckRxFtActionFrameStatus(struct ADAPTER *prAdapter,
			   struct SW_RFB *prSwRfb, uint16_t *pu2StatusCode)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct ACTION_FT_RESP_ACTION_FRAME *prRxFrame;
	struct ROAMING_INFO *prRoam;
	struct BSS_DESC *prBssDesc;
	const uint8_t *aucTargetApAddr;
	const uint8_t *aucStaAddr;
	uint8_t ucBssIndex;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return WLAN_STATUS_INVALID_PACKET;

	ucBssIndex = prStaRec->ucBssIndex;
	prBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prRoam = aisGetRoamingInfo(prAdapter, ucBssIndex);

	if (!prRoam->prRoamTarget || !prRoam->prRoamTarget->prMainBssDesc)
		return WLAN_STATUS_INVALID_PACKET;

	prBssDesc = prRoam->prRoamTarget->prMainBssDesc;
	aucStaAddr = prBssInfo->aucOwnMacAddr;
	aucTargetApAddr = prBssDesc->aucBSSID;
	prRxFrame = (struct ACTION_FT_RESP_ACTION_FRAME *)prSwRfb->pvHeader;

	if (prRxFrame->ucCategory != CATEGORY_FT_ACTION ||
	    prRxFrame->ucAction != ACTION_FT_RESPONSE) {
		DBGLOG(ROAMING, WARN,
		       "FT: Discard frame(cat=%d, action=%d)\n",
		       prRxFrame->ucCategory, prRxFrame->ucAction);
		*pu2StatusCode = STATUS_CODE_AUTH_ALGORITHM_NOT_SUPPORTED;
		return WLAN_STATUS_SUCCESS;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	/* set to FTR addr */
	if (mldIsSingleLinkEnabled(prAdapter, NETWORK_TYPE_AIS, ucBssIndex) &&
	    prBssDesc->rMlInfo.fgValid) {
		struct MULTI_LINK_INFO parse, *info = &parse;
		const uint8_t *ml;

		aucTargetApAddr = prBssDesc->rMlInfo.aucMldAddr;

		kalMemSet(info, 0, sizeof(*info));
		/* look for ml ie from frame payload */
		ml = mldFindMlIE(prRxFrame->aucInfoElem,
			prSwRfb->u2PacketLen -
			sizeof(struct ACTION_FT_RESP_ACTION_FRAME),
			ML_CTRL_TYPE_BASIC);
		if (ml)
			MLD_PARSE_BASIC_MLIE(info, ml, IE_SIZE(ml),
				prBssDesc->aucBSSID, MAC_FRAME_AUTH);

		if (!info->ucValid ||
		    UNEQUAL_MAC_ADDR(info->aucMldAddr, aucTargetApAddr)) {
			DBGLOG(ROAMING, WARN,
				"FT: AP wrong ML ie (addr=" MACSTR
				", expect=" MACSTR " valid=%d, num=%d)\n",
				MAC2STR(info->aucMldAddr),
				MAC2STR(aucTargetApAddr),
				info->ucValid,
				info->ucProfNum);
			*pu2StatusCode = STATUS_CODE_UNSPECIFIED_FAILURE;
			return WLAN_STATUS_SUCCESS;
		}
	}
#endif

	if (UNEQUAL_MAC_ADDR(prRxFrame->aucStaAddr, aucStaAddr)) {
		DBGLOG(ROAMING, WARN,
		       "FT: wrong sta addr " MACSTR " expected " MACSTR "\n",
		       MAC2STR(prRxFrame->aucStaAddr), MAC2STR(aucStaAddr));
		*pu2StatusCode = STATUS_CODE_UNSPECIFIED_FAILURE;
		return WLAN_STATUS_SUCCESS;
	}

	if (UNEQUAL_MAC_ADDR(prRxFrame->aucTargetApAddr, aucTargetApAddr)) {
		DBGLOG(ROAMING, WARN,
		       "FT: wrong ap addr " MACSTR " expected " MACSTR "\n",
		       MAC2STR(prRxFrame->aucTargetApAddr),
		       MAC2STR(aucTargetApAddr));
		*pu2StatusCode = STATUS_CODE_UNSPECIFIED_FAILURE;
		return WLAN_STATUS_SUCCESS;
	}

	*pu2StatusCode = prRxFrame->u2StatusCode;

	return WLAN_STATUS_SUCCESS;
}

uint32_t roamingFsmProcessRxFtResponse(struct ADAPTER *prAdapter,
				       struct SW_RFB *prSwRfb)
{
	struct ACTION_FT_RESP_ACTION_FRAME *prRxFrame;
	struct FT_EVENT_PARAMS *prFtParam = aisGetFtEventParam(prAdapter,
		secGetBssIdxByRfb(prAdapter, prSwRfb));
	uint16_t u2IEsLen;

	prRxFrame = (struct ACTION_FT_RESP_ACTION_FRAME *)prSwRfb->pvHeader;
	u2IEsLen = (prSwRfb->u2PacketLen -
		sizeof(struct ACTION_FT_RESP_ACTION_FRAME));

	prFtParam->pcIe = &prRxFrame->aucInfoElem[0];
	prFtParam->u2IeLen = u2IEsLen;

	return WLAN_STATUS_SUCCESS;
}

void roamingFsmTxReqDoneOrRxRespTimeout(
	struct ADAPTER *prAdapter, uintptr_t ulParam)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	uint8_t ucBssIndex = (uint8_t) ulParam;
	struct ROAMING_INFO *prRoamingFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);

	switch (prRoamingFsmInfo->eCurrentState) {
	case ROAMING_STATE_SEND_WNM_RESP:
		roamingFsmSteps(prAdapter, ROAMING_STATE_HANDLE_NEW_CANDIDATE,
			ucBssIndex);
		break;

	case ROAMING_STATE_SEND_FT_REQUEST:
	case ROAMING_STATE_WAIT_FT_RESPONSE:
		roamingFsmSteps(prAdapter, ROAMING_STATE_SEND_FT_REQUEST,
			ucBssIndex);
		break;
	default:
		break;
	}
}

void roamingFsmRunEventRxFtAction(struct ADAPTER *prAdapter,
			  struct SW_RFB *prSwRfb)
{
	struct ROAMING_INFO *prRoamingFsmInfo;
	struct ACTION_FT_RESP_ACTION_FRAME *prRxFrame;
	uint16_t u2StatusCode = 0;
	uint8_t ucBssIndex;
	struct FT_EVENT_PARAMS *prFtParam;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	DBGDUMP_MEM8(ROAMING, INFO, "FT response\n",
		prSwRfb->pvHeader, prSwRfb->u2PacketLen);

	prRxFrame = (struct ACTION_FT_RESP_ACTION_FRAME *)prSwRfb->pvHeader;
	ucBssIndex =  secGetBssIdxByRfb(prAdapter, prSwRfb);
	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);
	prFtParam = aisGetFtEventParam(prAdapter, ucBssIndex);

	switch (prRoamingFsmInfo->eCurrentState) {
	case ROAMING_STATE_SEND_FT_REQUEST:
	case ROAMING_STATE_WAIT_FT_RESPONSE:
		/* Check if the incoming frame is what we are waiting for */
		if (roamingFsmCheckRxFtActionFrameStatus(prAdapter, prSwRfb,
			&u2StatusCode) != WLAN_STATUS_SUCCESS)
			break;

		if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
			prFtParam->eFtDsState = FT_DS_STATE_AUTHORIZED;
			roamingFsmProcessRxFtResponse(prAdapter, prSwRfb);
		} else {
			prFtParam->eFtDsState = FT_DS_STATE_FAIL;
			DBGLOG(SAA, INFO,
			       "FT action was rejected by [" MACSTR
			       "], Status Code = %d\n",
			       MAC2STR(prRxFrame->aucSrcAddr),
			       u2StatusCode);
		}

		cnmTimerStopTimer(prAdapter,
				&prRoamingFsmInfo->rTxReqDoneRxRespTimer);

		prFtParam->u2FtDsStatusCode = u2StatusCode;

		/* Reset Send Auth/(Re)Assoc Frame Count */
		prRoamingFsmInfo->ucTxActionRetryCount = 0;

		roamingFsmSteps(prAdapter,
			ROAMING_STATE_HANDLE_NEW_CANDIDATE,
			ucBssIndex);
		break;
	default:
		break;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialize the value in ROAMING_FSM_INFO_T for ROAMING FSM operation
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void roamingFsmInit(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct ROAMING_INFO *prRoamingFsmInfo;
	uint8_t i;

	DBGLOG(ROAMING, LOUD,
	       "[%d]->Init: Current Time = %u\n",
	       ucBssIndex,
	       kalGetTimeTick());

	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);

	/* 4 <1> Initiate FSM */
	prRoamingFsmInfo->eCurrentState = ROAMING_STATE_IDLE;
	prRoamingFsmInfo->rRoamingDiscoveryUpdateTime = 0;
	kalMemZero(&prRoamingFsmInfo->rSkipBtmInfo,
			sizeof(struct ROAMING_SKIP_CONFIG));
	prRoamingFsmInfo->fgDisallowBtmRoaming = FALSE;
	prRoamingFsmInfo->u4BssIdxBmap = 0;
	prRoamingFsmInfo->rRoamScanParam.ucScanType = ROAMING_SCAN_TYPE_NORMAL;
	prRoamingFsmInfo->rRoamScanParam.ucScanCount = 0;
	prRoamingFsmInfo->rRoamScanParam.ucScanMode = ROAMING_SCAN_MODE_NORMAL;

	for (i = 0; i < MAX_BSSID_NUM; i++)
		prRoamingFsmInfo->eCurrentEvent[i] = ROAMING_EVENT_NUM;

	cnmTimerInitTimer(prAdapter,
		  &prRoamingFsmInfo->rTxReqDoneRxRespTimer,
		  (PFN_MGMT_TIMEOUT_FUNC) roamingFsmTxReqDoneOrRxRespTimeout,
		  (uintptr_t)ucBssIndex);
}				/* end of roamingFsmInit() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Uninitialize the value in AIS_FSM_INFO_T for AIS FSM operation
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void roamingFsmUninit(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct ROAMING_INFO *prRoamingFsmInfo;

	DBGLOG(ROAMING, LOUD,
	       "[%d]->Uninit: Current Time = %u\n",
	       ucBssIndex,
	       kalGetTimeTick());

	prRoamingFsmInfo =
		aisGetRoamingInfo(prAdapter, ucBssIndex);

	prRoamingFsmInfo->eCurrentState = ROAMING_STATE_IDLE;
} /* end of roamingFsmUninit() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Send commands to firmware
 *
 * @param [IN P_ADAPTER_T]       prAdapter
 *        [IN P_ROAMING_PARAM_T] prParam
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void roamingFsmSendCmd(struct ADAPTER *prAdapter,
	struct CMD_ROAMING_TRANSIT *prTransit)
{
	uint32_t rStatus;
	uint8_t ucBssIndex = prTransit->ucBssidx;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct ROAMING_INFO *prRoamingFsmInfo;

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);

	/* Check Roaming Conditions */
	if (prConnSettings->eConnectionPolicy == CONNECT_BY_BSSID ||
	    prAdapter->rWifiVar.fgDisRoaming) {
		DBGLOG(ROAMING, INFO,
			"Ignore [%d]->Send(%s): Policy=%d, DisRoam=%d Current Time = %u\n",
			ucBssIndex, apucEvent[prTransit->u2Event],
			prConnSettings->eConnectionPolicy,
			prAdapter->rWifiVar.fgDisRoaming,
			kalGetTimeTick());
		return;
	}

	if (prRoamingFsmInfo->eCurrentEvent[ucBssIndex] == prTransit->u2Event) {
		DBGLOG(ROAMING, INFO,
			"Ignore [%d](%s)->Send(%s): Current Time = %u\n",
			ucBssIndex,
			apucEvent[prRoamingFsmInfo->eCurrentEvent[ucBssIndex]],
			apucEvent[prTransit->u2Event],
			kalGetTimeTick());
		return;
	}

	DBGLOG(ROAMING, INFO,
		"[%d](%s)->Send(%s): Current Time = %u\n",
		ucBssIndex,
		apucEvent[prRoamingFsmInfo->eCurrentEvent[ucBssIndex]],
		apucEvent[prTransit->u2Event],
		kalGetTimeTick());

	prRoamingFsmInfo->eCurrentEvent[ucBssIndex] = prTransit->u2Event;

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				      CMD_ID_ROAMING_TRANSIT,	/* ucCID */
				      TRUE,	/* fgSetQuery */
				      FALSE,	/* fgNeedResp */
				      FALSE,	/* fgIsOid */
				      NULL,	/* pfCmdDoneHandler */
				      NULL,	/* pfCmdTimeoutHandler */
				      sizeof(struct CMD_ROAMING_TRANSIT),
				      /* u4SetQueryInfoLen */
				      (uint8_t *)prTransit, /* pucInfoBuffer */
				      NULL,	/* pvSetQueryBuffer */
				      0	/* u4SetQueryBufferLen */
				     );

	/* ASSERT(rStatus == WLAN_STATUS_PENDING); */
}				/* end of roamingFsmSendCmd() */

void roamingFsmSendStartCmd(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct ROAMING_INFO *prRoamingFsmInfo;
	uint8_t i;

	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);

	/* start to monitor all links */
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		struct CMD_ROAMING_TRANSIT rTransit = {0};

		if (prRoamingFsmInfo->u4BssIdxBmap & BIT(i)) {
			rTransit.u2Event = ROAMING_EVENT_START;
			rTransit.u2Data = i;
			rTransit.ucBssidx = i;
			roamingFsmSendCmd(prAdapter,
				(struct CMD_ROAMING_TRANSIT *) &rTransit);
		}
	}
}

void roamingFsmSendAbortCmd(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct ROAMING_INFO *prRoamingFsmInfo;
	uint8_t i;

	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);

	/* abort all started links */
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		struct CMD_ROAMING_TRANSIT rTransit = {0};

		if (prRoamingFsmInfo->u4BssIdxBmap & BIT(i)) {
			rTransit.u2Event = ROAMING_EVENT_ABORT;
			rTransit.ucBssidx = i;
			roamingFsmSendCmd(prAdapter,
				(struct CMD_ROAMING_TRANSIT *) &rTransit);
		}
	}
}

/*----------------------------------------------------------------------------*/

/*
 * @brief Check if need to do scan for roaming
 *
 * @param[out] fgIsNeedScan Set to TRUE if need to scan since
 *		there is roaming candidate in current scan result
 *		or skip roaming times > limit times
 * @return
 */
/*----------------------------------------------------------------------------*/
static u_int8_t roamingFsmIsNeedScan(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_SPECIFIC_BSS_INFO *asbi = NULL;
	struct LINK *prEssLink = NULL;
	u_int8_t fgIsNeedScan = TRUE;

	asbi = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	if (asbi == NULL) {
		DBGLOG(ROAMING, WARN, "ais specific bss info is NULL\n");
		return TRUE;
	}

	prEssLink = &asbi->rCurEssLink;

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
	/*
	 * Start skip roaming scan mechanism if only one ESSID AP
	 */
	if (prEssLink->u4NumElem == 1) {
		struct BSS_DESC *prBssDesc;

		/* Get current BssDesc */
		prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
		if (prBssDesc) {
			DBGLOG(ROAMING, INFO,
				"roamingFsmSteps: RCPI:%d RoamSkipTimes:%d\n",
				prBssDesc->ucRCPI, asbi->ucRoamSkipTimes);
			if (prBssDesc->ucRCPI > 90) {
				/* Set parameters related to Good Area */
				asbi->ucRoamSkipTimes = 3;
				asbi->fgGoodRcpiArea = TRUE;
				asbi->fgPoorRcpiArea = FALSE;
			} else {
				if (asbi->fgGoodRcpiArea) {
					asbi->ucRoamSkipTimes--;
				} else if (prBssDesc->ucRCPI > 67) {
					/*Set parameters related to Poor Area*/
					if (!asbi->fgPoorRcpiArea) {
						asbi->ucRoamSkipTimes = 2;
						asbi->fgPoorRcpiArea = TRUE;
						asbi->fgGoodRcpiArea = FALSE;
					} else {
						asbi->ucRoamSkipTimes--;
					}
				} else {
					asbi->fgPoorRcpiArea = FALSE;
					asbi->fgGoodRcpiArea = FALSE;
					asbi->ucRoamSkipTimes--;
				}
			}

			if (asbi->ucRoamSkipTimes == 0) {
				asbi->ucRoamSkipTimes = 3;
				asbi->fgPoorRcpiArea = FALSE;
				asbi->fgGoodRcpiArea = FALSE;
				DBGLOG(ROAMING, INFO, "Need Scan\n");
			} else {
				struct CMD_ROAMING_SKIP_ONE_AP cmd = {0};

				cmd.fgIsRoamingSkipOneAP = 1;

				wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SET_ROAMING_SKIP,
				    TRUE,
				    FALSE,
				    FALSE, NULL, NULL,
				    sizeof(struct CMD_ROAMING_SKIP_ONE_AP),
				    (uint8_t *)&cmd, NULL, 0);

				fgIsNeedScan = FALSE;
			}
		} else {
			DBGLOG(ROAMING, WARN, "Target BssDesc is NULL\n");
		}
	}
#endif

#if CFG_ENABLE_WIFI_DIRECT
	if (cnmP2pIsActive(prAdapter))
		fgIsNeedScan = FALSE;
#endif

	return fgIsNeedScan;
}

static uint32_t roamingFsmBTMResponseTxDone(struct ADAPTER *prAdapter,
				     struct MSDU_INFO *prMsduInfo,
				     enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	uint8_t ucBssIndex = 0;
	struct ROAMING_INFO *prRoamingFsmInfo;

	DBGLOG(ROAMING, INFO, "BTM Resp Tx Done Status %d\n", rTxDoneStatus);
	ucBssIndex = prMsduInfo->ucBssIndex;
	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);
	cnmTimerStopTimer(prAdapter,
		&prRoamingFsmInfo->rTxReqDoneRxRespTimer);
	roamingFsmTxReqDoneOrRxRespTimeout(prAdapter, ucBssIndex);
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief The Core FSM engine of ROAMING for AIS Infra.
 *
 * @param [IN P_ADAPTER_T]          prAdapter
 *        [ENUM_ROAMING_STATE_T] eNextState Enum value of next AIS STATE
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void roamingFsmSteps(struct ADAPTER *prAdapter,
	enum ENUM_ROAMING_STATE eNextState,
	uint8_t ucBssIndex)
{
	struct ROAMING_INFO *prRoam;
	enum ENUM_ROAMING_STATE ePreviousState;
	u_int8_t fgIsTransition = (u_int8_t) FALSE;
	u_int32_t u4ScnResultsTimeout = prAdapter->rWifiVar.u4DiscoverTimeout;
	struct BSS_TRANSITION_MGT_PARAM *prBtmParam;
	struct FT_EVENT_PARAMS *prFtParam;
	struct BSS_INFO *prBssInfo;
	uint32_t rStatus;

	prRoam = aisGetRoamingInfo(prAdapter, ucBssIndex);
	prBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prBtmParam = aisGetBTMParam(prAdapter, ucBssIndex);
	prFtParam = aisGetFtEventParam(prAdapter, ucBssIndex);

	do {
		if (prRoam->eCurrentState < 0 ||
		    prRoam->eCurrentState >= ROAMING_STATE_NUM ||
		    eNextState < 0 || eNextState >= ROAMING_STATE_NUM) {
			DBGLOG(ROAMING, STATE, "Invalid stat eNextState[%d]\n",
				eNextState);
			return;
		}
		/* Do entering Next State */
		DBGLOG(ROAMING, STATE,
		       "[ROAMING%d] TRANSITION: [%s] -> [%s]\n",
		       ucBssIndex,
		       apucDebugRoamingState[prRoam->eCurrentState],
		       apucDebugRoamingState[eNextState]);

		/* NOTE(Kevin): This is the only place to
		 *    change the eCurrentState(except initial)
		 */
		ePreviousState = prRoam->eCurrentState;
		prRoam->eCurrentState = eNextState;

		fgIsTransition = (u_int8_t) FALSE;

		/* Do tasks of the State that we just entered */
		switch (prRoam->eCurrentState) {
		case ROAMING_STATE_IDLE:
			prRoam->prRoamTarget = NULL;
			prRoam->rRoamScanParam.ucScanType =
					ROAMING_SCAN_TYPE_NORMAL;
			prRoam->rRoamScanParam.ucScanCount = 0;
			prRoam->rRoamScanParam.ucScanMode =
					ROAMING_SCAN_MODE_NORMAL;
			prFtParam->eFtDsState = FT_DS_STATE_IDLE;

			break;
		case ROAMING_STATE_DECISION:
#if CFG_SUPPORT_DRIVER_ROAMING
			GET_CURRENT_SYSTIME(
				&prRoam->rRoamingLastDecisionTime);
#endif
			prRoam->eReason = ROAMING_REASON_POOR_RCPI;
			prRoam->prRoamTarget = NULL;
			prRoam->rRoamScanParam.ucScanType =
					ROAMING_SCAN_TYPE_NORMAL;
			prRoam->rRoamScanParam.ucScanCount = 0;
			prRoam->rRoamScanParam.ucScanMode =
					ROAMING_SCAN_MODE_NORMAL;
			prFtParam->eFtDsState = FT_DS_STATE_IDLE;

			break;

		case ROAMING_STATE_DISCOVERY: {
			OS_SYSTIME rCurrentTime;
			u_int8_t fgIsNeedScan = FALSE;

#if CFG_SUPPORT_NCHO
			if (prAdapter->rNchoInfo.fgNCHOEnabled == TRUE)
				u4ScnResultsTimeout = 0;
#endif

			GET_CURRENT_SYSTIME(&rCurrentTime);
			if (CHECK_FOR_TIMEOUT(rCurrentTime,
			      prRoam->rRoamingDiscoveryUpdateTime,
			      SEC_TO_SYSTIME(u4ScnResultsTimeout))) {
				DBGLOG(ROAMING, LOUD,
					"roamingFsmSteps: DiscoveryUpdateTime Timeout\n");

				fgIsNeedScan = roamingFsmIsNeedScan(prAdapter,
								ucBssIndex);
			}
			aisFsmRunEventRoamingDiscovery(
				prAdapter, fgIsNeedScan, ucBssIndex);
		}
		break;

		case ROAMING_STATE_ROAM:
			aisFsmRunEventRoamingRoam(prAdapter, ucBssIndex);
			break;

		case ROAMING_STATE_HANDLE_NEW_CANDIDATE: {
			prRoam->ucTxActionRetryCount = 0;
#if CFG_SUPPORT_802_11V_BTM_OFFLOAD
			if (prBtmParam->fgPendingResponse) {
				eNextState = ROAMING_STATE_SEND_WNM_RESP;
				fgIsTransition = TRUE;
				break;
			}
#endif /* CFG_SUPPORT_802_11V_BTM_OFFLOAD */

			if (prRoam->prRoamTarget &&
			    prRoam->prRoamTarget->prMainBssDesc->fgIsFtOverDS &&
			    prFtParam->eFtDsState == FT_DS_STATE_IDLE) {
				eNextState = ROAMING_STATE_SEND_FT_REQUEST;
				fgIsTransition = TRUE;
				break;
			}

			eNextState = ROAMING_STATE_ROAM;
			fgIsTransition = TRUE;
		}
			break;

#if CFG_SUPPORT_802_11V_BTM_OFFLOAD
		case ROAMING_STATE_SEND_WNM_RESP: {
			struct BSS_INFO *prAisBssInfo =
				aisGetAisBssInfo(prAdapter,
				prBtmParam->ucRspBssIndex);
			struct BSS_DESC_SET *prRoamTarget =
				prRoam->prRoamTarget;

			/* reply btm resp to the link sending btm req but
			 * keep bssid the same as setup link's bssid
			 */
			prBtmParam->fgPendingResponse = FALSE;
			if (prRoamTarget) {
				struct BSS_DESC *prBssDesc =
					prRoamTarget->prMainBssDesc;
				const uint8_t *aucTargetApAddr;

				aucTargetApAddr = prBssDesc->aucBSSID;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
				/* set to FTR addr */
				if (mldIsSingleLinkEnabled(prAdapter,
					NETWORK_TYPE_AIS, ucBssIndex) &&
				    prBssDesc->rMlInfo.fgValid)
					aucTargetApAddr =
						prBssDesc->rMlInfo.aucMldAddr;
#endif

				wnmSendBTMResponseFrame(
				      prAdapter,
				      prAisBssInfo->prStaRecOfAP,
				      roamingFsmBTMResponseTxDone,
				      prBtmParam->ucDialogToken,
				      WNM_BSS_TM_ACCEPT,
				      MBO_TRANSITION_REJECT_REASON_UNSPECIFIED,
				      0,
				      aucTargetApAddr);
				cnmTimerStopTimer(prAdapter,
					&prRoam->rTxReqDoneRxRespTimer);
				cnmTimerStartTimer(prAdapter,
					&prRoam->rTxReqDoneRxRespTimer, 1000);

				DBGLOG(ROAMING, INFO,
					"Wait for sending btm resp done\n");
			} else {
				wnmSendBTMResponseFrame(
				      prAdapter,
				      prAisBssInfo->prStaRecOfAP,
				      NULL,
				      prBtmParam->ucDialogToken,
				      WNM_BSS_TM_REJECT_NO_SUITABLE_CANDIDATES,
#if CFG_EXT_ROAMING_WTC
				      wnmWtcGetRejectStatus(prAdapter,
					ucBssIndex),
#else
					MBO_TRANSITION_REJECT_REASON_RSSI,
#endif
				      0,
				      NULL);
				eNextState = ROAMING_STATE_HANDLE_NEW_CANDIDATE;
				fgIsTransition = TRUE;
			}
		}
			break;
#endif /* CFG_SUPPORT_802_11V_BTM_OFFLOAD */

		case ROAMING_STATE_SEND_FT_REQUEST: {
			prFtParam->eFtDsState = FT_DS_STATE_ONGOING;

			/* Do tasks in INIT STATE */
			if (prRoam->ucTxActionRetryCount >=
					TX_ACTION_RETRY_LIMIT) {
				prFtParam->eFtDsState = FT_DS_STATE_FAIL;
				eNextState = ROAMING_STATE_HANDLE_NEW_CANDIDATE;
				fgIsTransition = TRUE;
			} else {
				prRoam->ucTxActionRetryCount++;

				rStatus = roamingFsmSendFtActionFrame(
					prAdapter, prBssInfo->prStaRecOfAP,
					prRoam->prRoamTarget);

				if (rStatus == WLAN_STATUS_SUCCESS) {
					cnmTimerStopTimer(prAdapter,
					   &prRoam->rTxReqDoneRxRespTimer);
					cnmTimerStartTimer(prAdapter,
					   &prRoam->rTxReqDoneRxRespTimer,
					   TU_TO_MSEC(
					   TX_ACTION_RETRY_TIMEOUT_TU));
				} else {
					prFtParam->eFtDsState =
						FT_DS_STATE_FAIL;
					eNextState =
					     ROAMING_STATE_HANDLE_NEW_CANDIDATE;
					fgIsTransition = TRUE;
				}
			}
		}
			break;

		case ROAMING_STATE_WAIT_FT_RESPONSE: {
		}
			break;

		default:
			ASSERT(0); /* Make sure we have handle all STATEs */
		}
	} while (fgIsTransition);

	return;

}				/* end of roamingFsmSteps() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Transit to Decision state after join completion
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void roamingFsmRunEventStart(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct ROAMING_INFO *prRoamingFsmInfo;
	enum ENUM_ROAMING_STATE eNextState;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	uint8_t i;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	if (prAisBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
		return;

	DBGLOG(ROAMING, EVENT,
	       "[%d] EVENT-ROAMING START: Current Time = %u\n",
	       ucBssIndex,
	       kalGetTimeTick());

	if (prRoamingFsmInfo->eCurrentState != ROAMING_STATE_IDLE)
		return;

	/* start to monitor all links */
	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *bss = aisGetLinkBssInfo(prAisFsmInfo, i);

		if (!bss ||
		     bss->eConnectionState != MEDIA_STATE_CONNECTED)
			continue;

		if (prRoamingFsmInfo->u4BssIdxBmap & BIT(bss->ucBssIndex))
			DBGLOG(ROAMING, WARN,
				"Bss%d start roaming fsm again\n",
				bss->ucBssIndex);

		prRoamingFsmInfo->u4BssIdxBmap |= BIT(bss->ucBssIndex);
	}

	eNextState = ROAMING_STATE_DECISION;
	if (eNextState != prRoamingFsmInfo->eCurrentState) {
		roamingFsmSendStartCmd(prAdapter, ucBssIndex);

		/* Step to next state */
		roamingFsmSteps(prAdapter, eNextState, ucBssIndex);
	}
}				/* end of roamingFsmRunEventStart() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Transit to Discovery state when deciding to find a candidate
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void roamingFsmRunEventDiscovery(struct ADAPTER *prAdapter,
	struct CMD_ROAMING_TRANSIT *prTransit)
{
	struct ROAMING_INFO *prRoamingFsmInfo;
	enum ENUM_ROAMING_STATE eNextState;
	uint8_t ucBssIndex = prTransit->ucBssidx;

	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);

	DBGLOG(ROAMING, EVENT,
	       "[%d] EVENT-ROAMING DISCOVERY: Current Time = %u\n",
	       ucBssIndex,
	       kalGetTimeTick());

	/* DECISION -> DISCOVERY */
	/* Errors as IDLE, DISCOVERY, ROAM -> DISCOVERY */
	if (prRoamingFsmInfo->eCurrentState != ROAMING_STATE_DECISION) {
		DBGLOG(ROAMING, INFO,
			"Current State = %d, Ignore discovery\n",
			prRoamingFsmInfo->eCurrentState);
		return;
	}

	eNextState = ROAMING_STATE_DISCOVERY;
	/* DECISION -> DISCOVERY */
	if (eNextState != prRoamingFsmInfo->eCurrentState) {
		struct BSS_INFO *prAisBssInfo;
		struct BSS_DESC *prBssDesc;
		struct BSS_DESC *prBssDescTarget;
		uint8_t arBssid[PARAM_MAC_ADDR_LEN];
		struct PARAM_SSID rSsid;
		struct AIS_FSM_INFO *prAisFsmInfo;
		struct CONNECTION_SETTINGS *prConnSettings;

		kalMemZero(&rSsid, sizeof(struct PARAM_SSID));
		prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
		prConnSettings =
			aisGetConnSettings(prAdapter, ucBssIndex);

		/* sync. rcpi with firmware */
		prAisBssInfo =
			&(prAdapter->rWifiVar.arBssInfoPool[NETWORK_TYPE_AIS]);
		prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
		if (prBssDesc) {
			COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
				  prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
			COPY_MAC_ADDR(arBssid, prBssDesc->aucBSSID);
		} else  {
			COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
				  prConnSettings->aucSSID,
				  prConnSettings->ucSSIDLen);
			COPY_MAC_ADDR(arBssid, prConnSettings->aucBSSID);
		}

		prRoamingFsmInfo->ucRcpi = (uint8_t)(prTransit->u2Data & 0xff);
		prRoamingFsmInfo->ucThreshold =	prTransit->u2RcpiLowThreshold;

		prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
				arBssid, TRUE, &rSsid);
		if (prBssDesc) {
			prBssDesc->ucRCPI = prRoamingFsmInfo->ucRcpi;
			DBGLOG(ROAMING, INFO, "RCPI %u(%d)\n",
			     prBssDesc->ucRCPI, RCPI_TO_dBm(prBssDesc->ucRCPI));
		}

		prBssDescTarget = aisGetTargetBssDesc(prAdapter, ucBssIndex);
		if (prBssDescTarget && prBssDescTarget != prBssDesc) {
			prBssDescTarget->ucRCPI = prRoamingFsmInfo->ucRcpi;
			DBGLOG(ROAMING, WARN, "update target bss\n");
		}

		/* Save roaming reason code and PER value for AP selection */
		prRoamingFsmInfo->eReason = prTransit->eReason;
		if (prTransit->eReason == ROAMING_REASON_TX_ERR) {
			prRoamingFsmInfo->ucPER =
				(prTransit->u2Data >> 8) & 0xff;
			DBGLOG(ROAMING, INFO, "ucPER %u\n",
				prRoamingFsmInfo->ucPER);
		} else {
			prRoamingFsmInfo->ucPER = 0;
		}

#if CFG_SUPPORT_NCHO
		if (prRoamingFsmInfo->eReason == ROAMING_REASON_RETRY)
			DBGLOG(ROAMING, INFO,
				"NCHO enable=%d,trigger=%d,delta=%d,period=%d\n",
				prAdapter->rNchoInfo.fgNCHOEnabled,
				prAdapter->rNchoInfo.i4RoamTrigger,
				prAdapter->rNchoInfo.i4RoamDelta,
				prAdapter->rNchoInfo.u4RoamScanPeriod);
#endif

		roamingFsmSteps(prAdapter, eNextState, ucBssIndex);
	}
}				/* end of roamingFsmRunEventDiscovery() */

void roamingFsmNotifyEvent(
	struct ADAPTER *adapter, uint8_t bssIndex, uint8_t ucFail,
	struct BSS_DESC *prBssDesc)
{
	struct ROAMING_INFO *roam = aisGetRoamingInfo(adapter, bssIndex);
	struct ROAMING_EVENT_INFO *prEventInfo = &roam->rEventInfo;
	struct BSS_INFO *prAisBssInfo = aisGetAisBssInfo(adapter, bssIndex);
	char uevent[300];

	/* Check if bss index valid to pass coverity */
	if (bssIndex >= ARRAY_SIZE(adapter->rLinkQuality.rLq)) {
		DBGLOG(ROAMING, WARN, "invalid bss idx %u, caller=%pS\n",
				bssIndex, KAL_TRACE);
		return;
	}

	COPY_MAC_ADDR(roam->rEventInfo.aucPrevBssid, prAisBssInfo->aucBSSID);
	COPY_MAC_ADDR(roam->rEventInfo.aucCurrBssid, prBssDesc->aucBSSID);
	roam->rEventInfo.ucPrevChannel = prAisBssInfo->ucPrimaryChannel;
	roam->rEventInfo.ucCurrChannel = prBssDesc->ucChannelNum;
	roam->rEventInfo.ucBw = (uint8_t) prBssDesc->eBand;
	roam->rEventInfo.u2ApLoading = prBssDesc->u2StaCnt;
	roam->rEventInfo.ucSupportStbc = prBssDesc->fgMultiAnttenaAndSTBC;
	roam->rEventInfo.ucSupportStbc = prBssDesc->fgMultiAnttenaAndSTBC;
	roam->rEventInfo.ucPrevRcpi =
		dBm_TO_RCPI(adapter->rLinkQuality.rLq[bssIndex].cRssi);
	roam->rEventInfo.ucCurrRcpi = prBssDesc->ucRCPI;

	kalSnprintf(uevent, sizeof(uevent),
		"roam=Status:%s,BSSID:" MACSTR "/" MACSTR
		",Reason:%d,Chann:%d/%d,RCPI:%d/%d,BW:%d,STBC:%s\n",
		(ucFail == TRUE ? "FAIL" : "SUCCESS"),
		MAC2STR(prEventInfo->aucPrevBssid),
		MAC2STR(prEventInfo->aucCurrBssid), (uint8_t) roam->eReason,
		prEventInfo->ucPrevChannel, prEventInfo->ucCurrChannel,
		prEventInfo->ucPrevRcpi, prEventInfo->ucCurrRcpi,
		prEventInfo->ucBw,
		(prEventInfo->ucSupportStbc == TRUE ? "TRUE" : " FALSE"));
	kalSendUevent(adapter, uevent);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Transit to Decision state as being failed to find out any candidate
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void roamingFsmRunEventFail(struct ADAPTER *prAdapter,
	uint8_t ucReason, uint8_t ucBssIndex)
{
	struct ROAMING_INFO *prRoamingFsmInfo;
	enum ENUM_ROAMING_STATE eNextState;
	struct CMD_ROAMING_TRANSIT rTransit;

	prRoamingFsmInfo =
		aisGetRoamingInfo(prAdapter, ucBssIndex);
	kalMemZero(&rTransit, sizeof(struct CMD_ROAMING_TRANSIT));

	DBGLOG(ROAMING, STATE,
	       "[%d] EVENT-ROAMING FAIL: reason %x Current Time = %u\n",
	       ucBssIndex,
	       ucReason, kalGetTimeTick());

	if (prRoamingFsmInfo->eCurrentState == ROAMING_STATE_IDLE) {
		DBGLOG(ROAMING, INFO,
			"Current State = %d, Ignore fail\n",
			prRoamingFsmInfo->eCurrentState);
		return;
	}

	eNextState = ROAMING_STATE_DECISION;
	/* ROAM -> DECISION */
	if (eNextState != prRoamingFsmInfo->eCurrentState) {
		rTransit.u2Event = ROAMING_EVENT_FAIL;
		rTransit.u2Data = (uint16_t) (ucReason & 0xffff);
		rTransit.ucBssidx = ucBssIndex;
		roamingFsmSendCmd(prAdapter,
			(struct CMD_ROAMING_TRANSIT *) &rTransit);

		/* Step to next state */
		roamingFsmSteps(prAdapter, eNextState, ucBssIndex);
	}
}				/* end of roamingFsmRunEventFail() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Transit to Idle state as beging aborted by other moduels, AIS
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void roamingFsmRunEventAbort(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct ROAMING_INFO *prRoamingFsmInfo;
	enum ENUM_ROAMING_STATE eNextState;
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);

	DBGLOG(ROAMING, EVENT,
	       "[%d] EVENT-ROAMING ABORT: Current Time = %u\n",
	       ucBssIndex,
	       kalGetTimeTick());

	eNextState = ROAMING_STATE_IDLE;
	/* IDLE, DECISION, DISCOVERY, ROAM -> IDLE */
	if (eNextState != prRoamingFsmInfo->eCurrentState) {
		roamingFsmSendAbortCmd(prAdapter, ucBssIndex);

		/* Step to next state */
		roamingFsmSteps(prAdapter, eNextState, ucBssIndex);
	}

	/* abort all started links */
	prRoamingFsmInfo->u4BssIdxBmap = 0;
}				/* end of roamingFsmRunEventAbort() */

void roamingFsmRunEventNewCandidate(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prRoamTarget,
	uint8_t ucBssIndex)
{
	struct ROAMING_INFO *prRoam;
	struct FT_EVENT_PARAMS *prFt;
	struct BSS_TRANSITION_MGT_PARAM *prBtm;
	uint8_t ucBtmReqMode = 0;

	prRoam = aisGetRoamingInfo(prAdapter, ucBssIndex);
	prFt = aisGetFtEventParam(prAdapter, ucBssIndex);
	prBtm =  aisGetBTMParam(prAdapter, ucBssIndex);
	ucBtmReqMode = prBtm->ucRequestMode;

	DBGLOG(ROAMING, EVENT,
	       "[%d] EVENT-NEW CANDIDATE: Current Time = %u\n",
	       ucBssIndex,
	       kalGetTimeTick());

	/* accept new candiate when
	 * DISCOVERY: new fw/driver roaming event
	 * DECISION: userspace roaming
	 * ROAM: roaming retry
	 */
	if (prRoam->eCurrentState != ROAMING_STATE_DISCOVERY &&
	    prRoam->eCurrentState != ROAMING_STATE_DECISION &&
	    prRoam->eCurrentState != ROAMING_STATE_ROAM) {
		DBGLOG(ROAMING, INFO,
			"Current State = %d, Ignore new candidate\n",
			prRoam->eCurrentState);
		return;
	}

	if (prRoamTarget && !prRoamTarget->prMainBssDesc) {
		DBGLOG(ROAMING, WARN, "Invalid new candidate\n");
		return;
	}

	if (prRoam->eReason == ROAMING_REASON_BTM &&
	    (ucBtmReqMode & (WNM_BSS_TM_REQ_DISASSOC_IMMINENT |
			     WNM_BSS_TM_REQ_BSS_TERMINATION_INCLUDED))) {
		uint8_t ucDisImmiState = prBtm->ucDisImmiState;

		if (prBtm->ucDisImmiState == AIS_BTM_DIS_IMMI_STATE_1)
			prBtm->ucDisImmiState = AIS_BTM_DIS_IMMI_STATE_2;
		else if (prBtm->ucDisImmiState == AIS_BTM_DIS_IMMI_STATE_2)
			prBtm->ucDisImmiState = AIS_BTM_DIS_IMMI_STATE_3;

		DBGLOG(ROAMING, INFO, "DIS_IMMI_STATE %d -> %d\n",
			ucDisImmiState, prBtm->ucDisImmiState);
	}

	prRoam->prRoamTarget = prRoamTarget;
	prFt->eFtDsState = FT_DS_STATE_IDLE;
	cnmTimerStopTimer(prAdapter, &prRoam->rTxReqDoneRxRespTimer);

	roamingFsmSteps(prAdapter,
		ROAMING_STATE_HANDLE_NEW_CANDIDATE, ucBssIndex);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process events from firmware
 *
 * @param [IN P_ADAPTER_T]       prAdapter
 *        [IN P_ROAMING_PARAM_T] prParam
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
uint32_t roamingFsmProcessEvent(struct ADAPTER *prAdapter,
	struct CMD_ROAMING_TRANSIT *prTransit)
{
	uint8_t ucBssIndex = prTransit->ucBssidx;

	if (ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(ROAMING, ERROR, "ucBssIndex [%d] out of range!\n",
			ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(ROAMING, LOUD,
	       "[%d] ROAMING Process Events: Current Time = %u\n",
	       ucBssIndex,
	       kalGetTimeTick());

	if (prTransit->u2Event == ROAMING_EVENT_DISCOVERY) {
		struct CMD_ROAMING_TRANSIT rTransit = {0};

		DBGLOG(ROAMING, INFO,
			"ROAMING_EVENT_DISCOVERY Data[%u] RCPI[%u(%d)] PER[%u] Thr[%u(%d)] Reason[%d] Time[%u]\n",
			prTransit->u2Data,
			(prTransit->u2Data) & 0xff,      /* L[8], RCPI */
			RCPI_TO_dBm((prTransit->u2Data) & 0xff),
			(prTransit->u2Data >> 8) & 0xff, /* H[8], PER */
			prTransit->u2RcpiLowThreshold,
			RCPI_TO_dBm(prTransit->u2RcpiLowThreshold),
			prTransit->eReason,
			prTransit->u4RoamingTriggerTime);

		rTransit.u2Event = ROAMING_EVENT_ROAM;
		rTransit.ucBssidx = ucBssIndex;
		roamingFsmSendCmd(prAdapter,
			(struct CMD_ROAMING_TRANSIT *) &rTransit);

		/* fail when roaming is ongoing or during CSA*/
		if (!roamingFsmInDecision(prAdapter, FALSE, ucBssIndex)) {
			DBGLOG(ROAMING, EVENT,
				"There's ongoing roaming/CSA - ignore bssidx:%d\n",
				ucBssIndex);

			rTransit.u2Event = ROAMING_EVENT_FAIL;
			rTransit.u2Data = ROAMING_FAIL_REASON_NOCANDIDATE;
			rTransit.ucBssidx = ucBssIndex;
			roamingFsmSendCmd(prAdapter,
				(struct CMD_ROAMING_TRANSIT *) &rTransit);

			return WLAN_STATUS_SUCCESS;
		}

		roamingFsmRunEventDiscovery(prAdapter, prTransit);
	} else if (prTransit->u2Event == ROAMING_EVENT_THRESHOLD_UPDATE) {
		DBGLOG(ROAMING, INFO,
			"ROAMING_EVENT_THRESHOLD_UPDATE RCPI H[%d(%d)] L[%d(%d)]\n",
			prTransit->u2RcpiHighThreshold,
			RCPI_TO_dBm(prTransit->u2RcpiHighThreshold),
			prTransit->u2RcpiLowThreshold,
			RCPI_TO_dBm(prTransit->u2RcpiLowThreshold));
	}

	return WLAN_STATUS_SUCCESS;
}

uint8_t roamingFsmInDecision(struct ADAPTER *prAdapter,
	u_int8_t fgIgnorePolicy, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *ais;
	struct ROAMING_INFO *roam;
	enum ENUM_PARAM_CONNECTION_POLICY policy;
	struct CONNECTION_SETTINGS *setting;
#if CFG_SUPPORT_DFS
	struct BSS_INFO *prBssInfo;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
#endif
	roam = aisGetRoamingInfo(prAdapter, ucBssIndex);
	setting = aisGetConnSettings(prAdapter, ucBssIndex);
	ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	policy = setting->eConnectionPolicy;

	return IS_BSS_INDEX_AIS(prAdapter, ucBssIndex) &&
	       roam->eCurrentState == ROAMING_STATE_DECISION &&
#if CFG_SUPPORT_DFS
	       !aisFsmIsSwitchChannel(prAdapter, ais) &&
#endif
	       !prAdapter->rWifiVar.fgDisRoaming &&
	       (policy != CONNECT_BY_BSSID || fgIgnorePolicy);
}

u_int8_t roamingFsmCheckIfRoaming(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct ROAMING_INFO *prRoamingFsmInfo;

	if (IS_BSS_INDEX_AIS(prAdapter, ucBssIndex) == FALSE)
		return FALSE;

	prBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);

	DBGLOG(INIT, INFO,
		"BSSID[%d] C[%d] R[%d] A[%d] IsInPostpone[%d]\n",
		ucBssIndex,
		prBssInfo->eConnectionState,
		prRoamingFsmInfo->eCurrentState,
		prAisFsmInfo->eCurrentState,
		aisFsmIsInProcessPostpone(prAdapter, ucBssIndex));

	/* check if processing BTO */
	if (aisFsmIsInProcessPostpone(prAdapter, ucBssIndex))
		return TRUE;

	/* check if under roaming state */
	if ((prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) &&
		((prRoamingFsmInfo->eCurrentState == ROAMING_STATE_ROAM ||
		prRoamingFsmInfo->eCurrentState == ROAMING_STATE_DISCOVERY)
		|| (prAisFsmInfo->eCurrentState == AIS_STATE_JOIN ||
		prAisFsmInfo->eCurrentState == AIS_STATE_SEARCH ||
		prAisFsmInfo->eCurrentState == AIS_STATE_ROAMING ||
		prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN)))
		return TRUE;

	return FALSE;
}

void roamingFsmBTMTimeout(struct ADAPTER *prAdapter,
				uintptr_t ulParamPtr)
{
	uint8_t ucBssIndex = (uint8_t) ulParamPtr;
	struct CMD_ROAMING_TRANSIT rRoamingData = {0};
	struct AIS_FSM_INFO *prAisFsmInfo =
		aisGetAisFsmInfo(prAdapter, ucBssIndex);
	struct BSS_TRANSITION_MGT_PARAM *prBtmParam =
		aisGetBTMParam(prAdapter, ucBssIndex);
	struct BSS_DESC *prBssDesc =
		scanSearchBssDescByBssid(prAdapter, prBtmParam->aucBSSID);

	if (prBssDesc &&
	    prBssDesc->fgIsConnected & aisGetBssIndexBmap(prAisFsmInfo)) {
		DBGLOG(ROAMING, INFO, "[%d] BTM DiassocTimer Timeout\n",
				      ucBssIndex);

		rRoamingData.eReason = ROAMING_REASON_BTM;
		rRoamingData.u2Data = prBssDesc->ucRCPI;
		rRoamingData.ucBssidx = ucBssIndex;
		prAisFsmInfo->fgTargetChnlScanIssued = TRUE;
		roamingFsmRunEventDiscovery(prAdapter, &rRoamingData);
	} else {
		DBGLOG(ROAMING, ERROR, "[%d] Invalid BssDesc\n", ucBssIndex);
	}
}
#endif
