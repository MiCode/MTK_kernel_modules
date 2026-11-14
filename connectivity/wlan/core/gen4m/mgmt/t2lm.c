// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file t2lm.c
 *    \brief This file includes IEEE802.11be T2LM support.
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

#if (CFG_SUPPORT_802_11BE_T2LM == 1)
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

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static const char * const apucDebugT2LMState[T2LM_STATE_NUM] = {
	"T2LM_IDLE",
	"T2LM_ADV_SWITCH",
	"T2LM_ADV_DURATION",
	"T2LM_REQ_PENDING",
	"T2LM_REQ_SWITCH",
	"T2LM_REQ_DURATION",
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
PFN_TX_DONE_HANDLER t2lmTxDoneHandler[PROTECTED_EHT_ACTION_NUM] = {
	[TID2LINK_REQUEST] = t2lmReqTxDoneCb,
	[TID2LINK_RESPONSE] = t2lmRspTxDoneCb,
	[TID2LINK_TEARDOWN] = t2lmTeardownTxDoneCb,
};

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will handle TxDone Event of T2LM action frames.
 *
 * @param[in] prAdapter      Pointer to the Adapter structure.
 * @param[in] prMsduInfo     Pointer to the MSDU_INFO_T.
 * @param[in] rTxDoneStatus  Return TX status of the T2LM action frame.
 *
 * @retval WLAN_STATUS_SUCCESS
 */
/*---------------------------------------------------------------------------*/
void t2lmFsmSteps(struct ADAPTER *prAdapter,
		struct MLD_STA_RECORD *prMldStaRec,
		enum ENUM_T2LM_STATE eNextState)
{
	enum ENUM_T2LM_STATE ePreviousState;
	struct T2LM_INFO *prT2LMParams;
	uint32_t u4T2LMState;

	if (!prMldStaRec)
		return;

	prT2LMParams = &prMldStaRec->rT2LMParams;
	u4T2LMState = prMldStaRec->eT2LMState;

	if (u4T2LMState == eNextState)
		return;

	cnmTimerStopTimer(prAdapter, &prMldStaRec->rT2LMFsmTimer);

	if ((u4T2LMState < T2LM_STATE_NUM)
			&& ((uint32_t) eNextState < T2LM_STATE_NUM)) {
		DBGLOG(ML, INFO, "[T2LM]TRANSITION: [%s] -> [%s]\n",
			apucDebugT2LMState[u4T2LMState],
			apucDebugT2LMState[(uint32_t)eNextState]);
	}

	ePreviousState = prMldStaRec->eT2LMState;

	prMldStaRec->eT2LMState = eNextState;

	switch (prMldStaRec->eT2LMState) {
	case T2LM_STATE_IDLE:
		t2lmReset(prAdapter, prMldStaRec);
		break;
	case T2LM_STATE_ADV_SWITCH:
		DBGLOG(ML, INFO,
			"MLD_STA[idx=%d, grpMldId=%d] start t2lm adv switch delay = %d MS(switch=0x%x)\n",
			prMldStaRec->ucIdx, prMldStaRec->ucGroupMldId,
			prT2LMParams->u4SwitchDelayMs,
			prT2LMParams->u2MappingSwitchTime);

		cnmTimerStopTimer(prAdapter,
			&prMldStaRec->rT2LMTimer);
		cnmTimerStartTimer(prAdapter,
			&prMldStaRec->rT2LMTimer,
			prT2LMParams->u4SwitchDelayMs);
		break;
	case T2LM_STATE_ADV_DURATION:
		DBGLOG(ML, INFO,
			"MLD_STA[idx=%d, grpMldId=%d] start t2lm adv duration = %dMS(expected=0x%x)\n",
			prMldStaRec->ucIdx, prMldStaRec->ucGroupMldId,
			prT2LMParams->u4T2lmDurationMs,
			prT2LMParams->u4ExpectedDuration);

		t2lmMldStaRecUpdate(prAdapter, prMldStaRec, TRUE);
		cnmTimerStopTimer(prAdapter,
			&prMldStaRec->rT2LMTimer);
		if (prT2LMParams->u4T2lmDurationMs != 0)
			cnmTimerStartTimer(prAdapter,
				&prMldStaRec->rT2LMTimer,
				prT2LMParams->u4T2lmDurationMs);
		break;
	case T2LM_STATE_REQ_PENDING:
		t2lmMldStaRecUpdate(prAdapter, prMldStaRec, FALSE);
		prMldStaRec->eT2LMNextState = T2LM_STATE_IDLE;
		cnmTimerStartTimer(prAdapter, &prMldStaRec->rT2LMFsmTimer,
			T2LM_REQ_TX_TIMEOUT);
		break;
	case T2LM_STATE_REQ_SWITCH:
		DBGLOG(ML, TRACE,
			"MLD_STA[idx=%d, grpMldId=%d] start t2lm req switch = %dMS(expected=0x%x)\n",
			prMldStaRec->ucIdx, prMldStaRec->ucGroupMldId,
			prT2LMParams->u4SwitchDelayMs,
			prT2LMParams->u2MappingSwitchTime);
		cnmTimerStopTimer(prAdapter, &prMldStaRec->rT2LMTimer);
		cnmTimerStartTimer(prAdapter, &prMldStaRec->rT2LMTimer,
			prT2LMParams->u4SwitchDelayMs);
		break;
	case T2LM_STATE_REQ_DURATION:
		DBGLOG(ML, INFO,
			"MLD_STA[idx=%d, grpMldId=%d] start t2lm req duration = %dMS(expected=0x%x)\n",
			prMldStaRec->ucIdx, prMldStaRec->ucGroupMldId,
			prT2LMParams->u4T2lmDurationMs,
			prT2LMParams->u4ExpectedDuration);

		t2lmMldStaRecUpdate(prAdapter, prMldStaRec, TRUE);

		cnmTimerStopTimer(prAdapter,
			&prMldStaRec->rT2LMTimer);
		if (prT2LMParams->u4T2lmDurationMs != 0)
			cnmTimerStartTimer(prAdapter,
				&prMldStaRec->rT2LMTimer,
				prT2LMParams->u4T2lmDurationMs);
		break;
	default:
		DBGLOG(ML, ERROR, "Unknown T2LM STATE\n");
		break;
	}
}

uint32_t t2lmReqTxDoneCb(struct ADAPTER *prAdapter,
	      struct MSDU_INFO *prMsduInfo,
	      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct EVENT_TX_DONE *prTxDone;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	struct MLD_STA_RECORD *prMldStaRec;
	struct ACTION_T2LM_REQ_FRAME *prTxFrame;

	if (!prMsduInfo)
		return WLAN_STATUS_FAILURE;

	if (!prMsduInfo->prPacket)
		return WLAN_STATUS_FAILURE;

	prTxFrame = prMsduInfo->prPacket;
	prTxDone = prMsduInfo->prTxDone;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	if (!prMldStaRec)
		return WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;

	DBGLOG(TX, DEBUG,
		"T2LM TX DONE, BN:WIDX:PID:SN[%u:%u:%u:%d] Status[%u], SeqNo: %d\n",
		prBssInfo->eBand, prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
		prTxDone ? prTxDone->u2SequenceNumber : -1,
		rTxDoneStatus,
		prMsduInfo->ucTxSeqNum);
#if (CFG_SUPPORT_MLD_LOG == 1) && (CFG_SUPPORT_802_11BE_MLO == 1)
	mldLogT2LMReq(prAdapter, prBssInfo, prStaRec,
		prTxFrame->ucDialogToken, prMsduInfo->u2HwSeqNum,
		rTxDoneStatus);
#endif

	if (rTxDoneStatus != TX_RESULT_SUCCESS &&
			prAdapter->ucT2LMReqRetryCnt >
				prAdapter->rWifiVar.u4T2LMRetryLimit) {
		prAdapter->ucT2LMReqRetryCnt = 0;
		return WLAN_STATUS_FAILURE;
	} else if (rTxDoneStatus != TX_RESULT_SUCCESS) {
		prAdapter->ucT2LMReqRetryCnt++;
		t2lmSend(prAdapter, TID2LINK_REQUEST,
				prStaRec, &prMldStaRec->rT2LMParams);
	} else {
		t2lmFsmSteps(prAdapter, prMldStaRec, T2LM_STATE_REQ_PENDING);
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t t2lmRspTxDoneCb(struct ADAPTER *prAdapter,
	      struct MSDU_INFO *prMsduInfo,
	      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct EVENT_TX_DONE *prTxDone;
	struct STA_RECORD *prStaRec;
	struct MLD_STA_RECORD *prMldStaRec;
	struct BSS_INFO *prBssInfo;
	struct ACTION_T2LM_RSP_FRAME *prTxFrame;

	if (!prMsduInfo)
		return WLAN_STATUS_FAILURE;

	if (!prMsduInfo->prPacket)
		return WLAN_STATUS_FAILURE;

	prTxFrame = prMsduInfo->prPacket;
	prTxDone = prMsduInfo->prTxDone;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	if (!prMldStaRec)
		return WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;

	DBGLOG(TX, DEBUG,
		"T2LM TX DONE, BN:WIDX:PID:SN[%u:%u:%u:%d] Status[%u], SeqNo: %d\n",
		prBssInfo->eBand, prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
		prTxDone ? prTxDone->u2SequenceNumber : -1,
		rTxDoneStatus,
		prMsduInfo->ucTxSeqNum);
#if (CFG_SUPPORT_MLD_LOG == 1) && (CFG_SUPPORT_802_11BE_MLO == 1)
	mldLogT2LMResp(prAdapter,
		prBssInfo,
		prStaRec,
		prTxFrame->ucDialogToken,
		rTxDoneStatus);
#endif

	if (rTxDoneStatus == TX_RESULT_SUCCESS &&
	    prMldStaRec->eT2LMState == T2LM_STATE_REQ_PENDING) {
		if (prMldStaRec->rT2LMParams.u4SwitchDelayMs == 0)
			t2lmFsmSteps(prAdapter, prMldStaRec,
					T2LM_STATE_ADV_DURATION);
		else
			t2lmFsmSteps(prAdapter, prMldStaRec,
					T2LM_STATE_ADV_SWITCH);
	} else {
		t2lmFsmSteps(prAdapter, prMldStaRec, T2LM_STATE_IDLE);
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t t2lmTeardownTxDoneCb(struct ADAPTER *prAdapter,
	      struct MSDU_INFO *prMsduInfo,
	      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct EVENT_TX_DONE *prTxDone;
	struct STA_RECORD *prStaRec;
	struct MLD_STA_RECORD *prMldStaRec;
	struct BSS_INFO *prBssInfo;

	prTxDone = prMsduInfo->prTxDone;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	if (!prMldStaRec)
		return WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;

	DBGLOG(TX, DEBUG,
		"T2LM TX DONE, BN:WIDX:PID:SN[%u:%u:%u:%d] Status[%u], SeqNo: %d\n",
		prBssInfo->eBand, prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
		prTxDone ? prTxDone->u2SequenceNumber : -1,
		rTxDoneStatus,
		prMsduInfo->ucTxSeqNum);
#if (CFG_SUPPORT_MLD_LOG == 1) && (CFG_SUPPORT_802_11BE_MLO == 1)
	mldLogT2LMTeardown(prAdapter,
		prBssInfo,
		prStaRec,
		rTxDoneStatus);
#endif

	return WLAN_STATUS_SUCCESS;
}

void t2lmParseT2LMIE(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, const uint8_t *pucIE)
{
	struct MLD_STA_RECORD *prMldStaRec;
	struct STA_RECORD *prCurrStarec;
	struct LINK *prStarecList;
	struct T2LM_INFO *prT2LMParams;
	struct IE_TID_TO_LINK_MAPPING *prT2LM;
	const uint8_t *pos, *tid_profile_pos;
	uint8_t ucULTidBitmap, ucDLTidBitmap;
	uint8_t ucLMPresenceIndication = 0;
	uint8_t fgValidLinkSet = FALSE;
	uint8_t fgSwitchTimeChanged = FALSE;
	uint32_t u4Margin = prAdapter->rWifiVar.u4T2LMMarginMs;
	uint8_t ucDisabledNum = 0;
	int i;

	if (prAdapter->rWifiVar.ucT2LMNegotiationSupport == T2LM_NO_SUPPORT) {
		DBGLOG(ML, WARN,
			"Parse T2LM IE Fail, NegotiationSupport set to 0\n");
		return;
	} else if (prStaRec->ucStaState != STA_STATE_3) {
		DBGLOG(ML, ERROR,
			"Parse T2LM IE Fail, prStaRec is not in state 3\n");
		return;
	}

	DBGLOG_MEM8(ML, LOUD, pucIE, IE_LEN(pucIE));

	prT2LM = (struct IE_TID_TO_LINK_MAPPING *) pucIE;
	pos = prT2LM->ucOptCtrl;

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	if (!prMldStaRec) {
		DBGLOG(ML, ERROR,
			"No Mld Starec, should not have T2LM ie\n");
		return;
	}

	prT2LMParams = &prMldStaRec->rT2LMParams;

	/* T2LM control bits[0, 1] is direction
	 * 0: Downlink, 1: Uplink, 2:Downlink and uplink
	 */
	if (BE_IS_T2LM_CTRL_DIRECTION(prT2LM->ucCtrl) < 3) {
		prT2LMParams->ucDirection =
			BE_IS_T2LM_CTRL_DIRECTION(prT2LM->ucCtrl);
		DBGLOG(ML, LOUD, "direction = %d\n",
			prT2LMParams->ucDirection);
	} else {
		DBGLOG(ML, ERROR,
			"Invalid Direction value in T2LM ie\n");
		return;
	}

	/* T2LM control bit[2] is default link mapping */
	if (BE_IS_T2LM_CTRL_DEFAULT_LINK(prT2LM->ucCtrl) == 0) {
		ucLMPresenceIndication = *pos;
		DBGLOG(ML, LOUD, "link mapping presence indicator = %d\n",
			ucLMPresenceIndication);
		pos += 1;
	}

	/* T2LM control bit[3] is mapping switch time present */
	if (BE_IS_T2LM_CTRL_SWITCH_TIME(prT2LM->ucCtrl)) {
		uint16_t u2SwitchTimeTUs, u2tsf1025TUs;
		uint64_t u8tsf;

		WLAN_GET_FIELD_16(pos, &u2SwitchTimeTUs);
		WLAN_GET_FIELD_64(prStaRec->au4Timestamp, &u8tsf);

		if (u2SwitchTimeTUs !=
		    prMldStaRec->rT2LMParams.u2MappingSwitchTime) {
			fgSwitchTimeChanged = TRUE;
			prMldStaRec->rT2LMParams.u2MappingSwitchTime =
				u2SwitchTimeTUs;
		}

		prT2LMParams->u2MappingSwitchTime = u2SwitchTimeTUs;
		u2tsf1025TUs = (u8tsf & BITS(10, 25)) >> 10;
		if (u2SwitchTimeTUs > u2tsf1025TUs) {
			prT2LMParams->u4SwitchDelayMs =
				TU_TO_MSEC(u2SwitchTimeTUs - u2tsf1025TUs);
			if (prT2LMParams->u4SwitchDelayMs > u4Margin)
				prT2LMParams->u4SwitchDelayMs -= u4Margin;
		}

		DBGLOG(ML, LOUD,
			"widx[%d] switch delay = %d MS(switch=0x%x, margin=%d, tsf=%lu, tsf_10_25=%d)\n",
			prStaRec->ucWlanIndex,
			prT2LMParams->u4SwitchDelayMs,
			u2SwitchTimeTUs, u4Margin, u8tsf, u2tsf1025TUs);
		pos += 2;
	} else {
		prT2LMParams->u4SwitchDelayMs = 0;
		DBGLOG(ML, LOUD,
			"SwitchTime = 0, T2LM is establised currently\n");
	}

	/* T2LM control bit[4] is expected duration present */
	if (BE_IS_T2LM_CTRL_DURATION(prT2LM->ucCtrl)) {
		uint32_t u4DurationTUs;

		WLAN_GET_FIELD_24(pos, &u4DurationTUs);
		prT2LMParams->u4ExpectedDuration = u4DurationTUs;
		prT2LMParams->u4T2lmDurationMs =
			TU_TO_MSEC(u4DurationTUs) + 2 * u4Margin;

		DBGLOG(ML, LOUD,
			"widx[%d] t2lm duration = %dMS(expected=0x%x, margin=%d)\n",
			prStaRec->ucWlanIndex, prT2LMParams->u4T2lmDurationMs,
			u4DurationTUs, u4Margin);
		pos += 3;
	} else {
		prT2LMParams->u4T2lmDurationMs = 0;
		prT2LMParams->u4ExpectedDuration = 0;
	}

	tid_profile_pos = pos;
	prStarecList = &prMldStaRec->rStarecList;
	LINK_FOR_EACH_ENTRY(prCurrStarec, prStarecList, rLinkEntryMld,
		    struct STA_RECORD) {
		ucDLTidBitmap = prCurrStarec->ucDLTidBitmap;
		ucULTidBitmap = prCurrStarec->ucULTidBitmap;

		if (prCurrStarec->ucLinkId > 14) {
			DBGLOG(ML, ERROR,
				"Linkid = %d, sta idx %d is invalid\n",
				prCurrStarec->ucLinkId,
				prCurrStarec->ucIndex);
			continue;
		}

		/* T2LM control bit[5] is link mapping size */
		if (BE_IS_T2LM_CTRL_DEFAULT_LINK(prT2LM->ucCtrl) == 0
			&& BE_IS_T2LM_CTRL_LM_SIZE(prT2LM->ucCtrl)) {
			uint8_t ucTidLinkMapping;

			/*Link Mapping of Tid n, size is 1 octet*/
			for (i = 0; i < MAX_NUM_T2LM_TIDS; i++) {
				if (ucLMPresenceIndication & BIT(i)) {
					ucTidLinkMapping = *pos;

					if (ucTidLinkMapping > 0)
						fgValidLinkSet = TRUE;

					if (ucTidLinkMapping
						& BIT(prCurrStarec
							->ucLinkId)) {
						ucDLTidBitmap |= BIT(i);
						ucULTidBitmap |= BIT(i);
					} else {
						ucDLTidBitmap &= ~(BIT(i));
						ucULTidBitmap &= ~(BIT(i));
					}
					pos += 1;
				}
			}
		} else if (BE_IS_T2LM_CTRL_DEFAULT_LINK(prT2LM->ucCtrl) == 0
			&& (BE_IS_T2LM_CTRL_LM_SIZE(prT2LM->ucCtrl) == 0)) {
			uint16_t u2TidLinkMapping;

			/*Link Mapping of Tid n, size is 2 octet*/
			for (i = 0; i < MAX_NUM_T2LM_TIDS; i++) {
				if (ucLMPresenceIndication & BIT(i)) {
					WLAN_GET_FIELD_16(pos,
						&u2TidLinkMapping);

					if (u2TidLinkMapping > 0)
						fgValidLinkSet = TRUE;

					if (u2TidLinkMapping
						& BIT(prCurrStarec
							->ucLinkId)) {
						ucDLTidBitmap |= BIT(i);
						ucULTidBitmap |= BIT(i);
					} else {
						ucDLTidBitmap &= ~(BIT(i));
						ucULTidBitmap &= ~(BIT(i));
					}
					pos += 2;
				}
			}
		}

		DBGLOG(ML, TRACE,
			"Linkid = %d, ucTidBitmap(UL:DL) = 0x%02x:0x%02x\n",
			prCurrStarec->ucLinkId,
			ucDLTidBitmap,
			ucULTidBitmap);

		switch (prT2LMParams->ucDirection) {
		case T2LM_DIRECTION_DL:
			prCurrStarec->ucPendingDLTidBitmap = ucDLTidBitmap;
			break;
		case T2LM_DIRECTION_UL:
			prCurrStarec->ucPendingULTidBitmap = ucULTidBitmap;
			break;
		case T2LM_DIRECTION_DL_UL:
			prCurrStarec->ucPendingDLTidBitmap = ucDLTidBitmap;
			prCurrStarec->ucPendingULTidBitmap = ucULTidBitmap;
			break;
		default:
			break;
		}

		if (!ucDLTidBitmap && !ucULTidBitmap)
			ucDisabledNum++;

		pos = tid_profile_pos;
	}

	if (!fgValidLinkSet) {
		DBGLOG(ML, WARN,
			"Link set is invalid, no Tids are mapped to link\n");
		return;
	}

	/* all links are disabled, must leave current mld */
	if (ucDisabledNum == prStarecList->u4NumElem) {
		DBGLOG(ML, INFO,
			"Bss%d trigger BTO, mldstarec[" MACSTR
			"] all links disabled\n",
			prStaRec->ucBssIndex,
			MAC2STR(prMldStaRec->aucPeerMldAddr));
		aisBssBeaconTimeout(prAdapter, prStaRec->ucBssIndex);
		return;
	}

	if (prMldStaRec->eT2LMState == T2LM_STATE_REQ_PENDING) {
		t2lmMldStaRecBackup(prAdapter, prMldStaRec, prT2LMParams);
	} else {
		if (prT2LMParams->u4SwitchDelayMs == 0) {
			t2lmFsmSteps(prAdapter, prMldStaRec,
					T2LM_STATE_ADV_DURATION);
		} else if (fgSwitchTimeChanged) {
			t2lmFsmSteps(prAdapter, prMldStaRec,
					T2LM_STATE_ADV_SWITCH);
		}
	}
}

void t2lmTimeout(struct ADAPTER *prAdapter, uintptr_t ulParamPtr)
{
	struct MLD_STA_RECORD *prMldStaRec;

	prMldStaRec = (struct MLD_STA_RECORD *) ulParamPtr;

	if (!prMldStaRec->fgIsInUse) {
		DBGLOG(ML, WARN, "mld starec is not in use\n",
			prMldStaRec->eT2LMState);
		return;
	}

	if (prMldStaRec->eT2LMState == T2LM_STATE_ADV_DURATION
		|| prMldStaRec->eT2LMState == T2LM_STATE_REQ_DURATION) {
		t2lmFsmSteps(prAdapter, prMldStaRec, T2LM_STATE_IDLE);
	} else {
		if (prMldStaRec->eT2LMState == T2LM_STATE_ADV_SWITCH) {
			t2lmFsmSteps(prAdapter, prMldStaRec,
				T2LM_STATE_ADV_DURATION);
		} else if (prMldStaRec->eT2LMState == T2LM_STATE_REQ_SWITCH) {
			t2lmFsmSteps(prAdapter, prMldStaRec,
				T2LM_STATE_REQ_DURATION);
		} else {
			DBGLOG(ML, WARN, "T2LM state:[%d]\n",
					prMldStaRec->eT2LMState);
		}
	}
}

void t2lmFsmTimeout(struct ADAPTER *prAdapter, uintptr_t ulParamPtr)
{
	struct MLD_STA_RECORD *prMldStaRec;

	prMldStaRec = (struct MLD_STA_RECORD *) ulParamPtr;

	if (!prMldStaRec->fgIsInUse) {
		DBGLOG(ML, WARN, "mld starec is not in use\n",
			prMldStaRec->eT2LMState);
		return;
	}

	t2lmFsmSteps(prAdapter, prMldStaRec, prMldStaRec->eT2LMNextState);
}


void t2lmFillT2LMIE(uint8_t *prT2LMBuf,
	struct T2LM_INFO *prT2LMParams)
{
	struct IE_TID_TO_LINK_MAPPING *prT2LM;
	uint8_t *pos;
	int i;

	if (!prT2LMBuf) {
		DBGLOG(TX, ERROR,
			"invalid prT2LMBuf\n");
		return;
	}

	if (!prT2LMParams) {
		DBGLOG(TX, ERROR,
			"invalid prT2LMParams\n");
		return;
	}

	prT2LM = (struct IE_TID_TO_LINK_MAPPING *) prT2LMBuf;
	pos = prT2LM->ucOptCtrl;

	prT2LM->ucId = ELEM_ID_RESERVED;
	prT2LM->ucExtId = ELEM_EXT_ID_TID2LNK_MAP;
	if (prT2LMParams->ucDirection)
		prT2LM->ucCtrl |= prT2LMParams->ucDirection &
				T2LM_CTRL_DIRECTION;

	if (prT2LMParams->ucDefaultLM)
		prT2LM->ucCtrl |= prT2LMParams->ucDefaultLM <<
				T2LM_CTRL_DEFAULT_LINK_MAPPING_SHIFT;

	if (prT2LMParams->ucSwitchTimePresent)
		prT2LM->ucCtrl |= prT2LMParams->ucSwitchTimePresent <<
				T2LM_CTRL_MAPPING_SWITCH_TIME_PRESENT_SHIFT;

	if (prT2LMParams->ucDurationPresent)
		prT2LM->ucCtrl |= prT2LMParams->ucDurationPresent <<
				T2LM_CTRL_EXPECTED_DURATION_PRESENT_SHIFT;

	if (prT2LMParams->ucLMSize)
		prT2LM->ucCtrl |= prT2LMParams->ucLMSize <<
				T2LM_CTRL_LINK_MAPPING_SIZE_SHIFT;

	if (prT2LMParams->ucLMIndicator != 0) {
		DBGLOG(TX, LOUD, "ucLMIndicator : %d, pos: %p",
			prT2LMParams->ucLMIndicator, pos);
		*pos++ = prT2LMParams->ucLMIndicator;
	}

	if (prT2LMParams->ucSwitchTimePresent == 1) {
		DBGLOG(TX, LOUD, "u2SwitchTime : %d",
			prT2LMParams->u2MappingSwitchTime);
		WLAN_SET_FIELD_16(pos, prT2LMParams->u2MappingSwitchTime);
		pos += 2;
	}

	if (prT2LMParams->ucDurationPresent == 1) {
		DBGLOG(TX, LOUD, "u4Duration : %d",
			prT2LMParams->u4ExpectedDuration);
		WLAN_SET_FIELD_24(pos, prT2LMParams->u4ExpectedDuration);
		pos += 3;
	}

	if (prT2LMParams->ucDefaultLM == 0
		&& prT2LMParams->ucLMSize == 1) {
		for (i = 0; i < MAX_NUM_T2LM_TIDS; i++) {
			if (prT2LMParams->ucLMIndicator & BIT(i)) {
				*pos++ = (uint8_t) prT2LMParams->au2LMTid[i];
				DBGLOG(TX, LOUD, "au2LMTid : %d",
					prT2LMParams->au2LMTid[i]);
			}
		}
	} else if (prT2LMParams->ucDefaultLM == 0
		&& prT2LMParams->ucLMSize == 0) {
		for (i = 0; i < MAX_NUM_T2LM_TIDS; i++) {
			if (prT2LMParams->ucLMIndicator & BIT(i)) {
				WLAN_SET_FIELD_16(pos,
					prT2LMParams->au2LMTid[i]);
				pos += 2;
				DBGLOG(TX, LOUD, "au2LMTid : %d",
					prT2LMParams->au2LMTid[i]);
			}
		}
	}

	prT2LM->ucLength = pos - prT2LMBuf - ELEM_HDR_LEN;
	DBGLOG(TX, TRACE, "[F] Frame content:");
	DBGLOG_MEM8(TX, TRACE, prT2LM, IE_SIZE(prT2LM));
}

void t2lmComposeReq(struct MSDU_INFO *prMsduInfo, uint8_t ucDialogToken,
		struct T2LM_INFO *prT2LMParams)
{
	struct ACTION_T2LM_REQ_FRAME *prTxFrame;
	uint8_t *prT2LMBuf;

	prTxFrame = prMsduInfo->prPacket;
	prTxFrame->ucDialogToken = ucDialogToken;
	prT2LMBuf = &prTxFrame->aucT2LM[0];
	t2lmFillT2LMIE(prT2LMBuf, prT2LMParams);
}

void t2lmComposeRsp(struct MSDU_INFO *prMsduInfo, uint8_t ucDialogToken,
		uint8_t ucStatusCode, struct T2LM_INFO *prT2LMParams)
{
	struct ACTION_T2LM_RSP_FRAME *prTxFrame;
	uint8_t *prT2LMBuf;

	prTxFrame = prMsduInfo->prPacket;
	prTxFrame->ucDialogToken = ucDialogToken;
	prTxFrame->ucStatusCode = ucStatusCode;
	prT2LMBuf = &prTxFrame->aucT2LM[0];
}

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will enqueue T2LM action frame for TX.
 *
 * @param     prAdapter      Pointer to the Adapter structure.
 * @param     eAction        T2LM type requested to send
 *                           valid action code: TID2LINK_REQUEST [0]
 *                                              TID2LINK_RESPONSE [1]
 *                                              TID2LINK_TEARDOWN [2]
 *
 * @retval WLAN_STATUS_SUCCESS
 */
/*---------------------------------------------------------------------------*/
uint32_t t2lmSend(struct ADAPTER *prAdapter, enum PROTECTED_EHT_ACTION eAction,
		struct STA_RECORD *prStaRec, struct T2LM_INFO *prT2LMParams)
{
	struct MSDU_INFO *prMsduInfo;
	struct WLAN_ACTION_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	struct MLD_STA_RECORD *prMldStaRec;
	struct MLD_BSS_INFO *prMldBssInfo;
	uint16_t u2EstimatedFrameLen;
	uint16_t u2FrameLen = 0;

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;

	if (prAdapter->rWifiVar.ucT2LMNegotiationSupport == T2LM_NO_SUPPORT) {
		DBGLOG(ML, ERROR,
			"T2LM is disabled, NegotiationSupport set to 0\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldStaRec || !prMldBssInfo) {
		DBGLOG(TX, ERROR,
			"prMldStaRec or prMldBssInfo equal to NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!prT2LMParams) {
		DBGLOG(TX, ERROR, "invalid prT2LMParams\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD + PUBLIC_ACTION_MAX_LEN;

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
							u2EstimatedFrameLen);
	if (!prMsduInfo) {
		DBGLOG(TX, ERROR, "Alloc packet failed for MSDU\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);
	prTxFrame = prMsduInfo->prPacket;

	/* Fill frame ctrl */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;
	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	/* Compose the frame body's frame */
	prTxFrame->ucCategory = CATEGORY_PROTECTED_EHT_ACTION;
	prTxFrame->ucAction = eAction;

	DBGLOG(TX, TRACE, "Send T2LM action: %u\n", eAction);

	switch (eAction) {
	case TID2LINK_REQUEST:
		if (prMldStaRec->eT2LMState == T2LM_STATE_REQ_PENDING) {
			DBGLOG(TX, WARN,
				"Current state is [REQ_PENDING], skip T2LM Request.\n");
			break;
		}
		prAdapter->ucT2LMTxDialogToken++;
		t2lmComposeReq(prMsduInfo, prAdapter->ucT2LMTxDialogToken,
				prT2LMParams);
		t2lmMldStaRecBackup(prAdapter, prMldStaRec, prT2LMParams);
		u2FrameLen += OFFSET_OF(struct ACTION_T2LM_REQ_FRAME, aucT2LM);
		u2FrameLen += IE_SIZE(&((struct ACTION_T2LM_REQ_FRAME *)
				prMsduInfo->prPacket)->aucT2LM[0]);
		break;
	case TID2LINK_RESPONSE:
		t2lmComposeRsp(prMsduInfo, prAdapter->ucT2LMRxDialogToken,
				STATUS_CODE_SUCCESSFUL, prT2LMParams);
		u2FrameLen += OFFSET_OF(struct ACTION_T2LM_RSP_FRAME, aucT2LM);
		break;
	case TID2LINK_TEARDOWN:
		/* Teardone frame doesn't need to compose */
		u2FrameLen += sizeof(struct ACTION_T2LM_TEARDOWN_FRAME);
		break;
	default:
		DBGLOG(TX, ERROR, "action invalid %u\n", eAction);
		cnmMgtPktFree(prAdapter, prMsduInfo);
		return WLAN_STATUS_FAILURE;
	}

	nicTxSetMngPacket(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
			  prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
			  u2FrameLen, t2lmTxDoneHandler[eAction],
			  MSDU_RATE_MODE_AUTO);

	/* Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

uint32_t t2lmProcessReq(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
		struct MLD_STA_RECORD *prMldStaRec,
		struct MLD_BSS_INFO *prMldBssInfo)
{
	struct ACTION_T2LM_REQ_FRAME *prRxFrame;
	uint8_t *pucIE;
	uint16_t u2IELength, u2Offset;

	if (!prAdapter || !prSwRfb || !prMldStaRec || !prMldBssInfo)
		return WLAN_STATUS_FAILURE;

	prRxFrame = (struct ACTION_T2LM_REQ_FRAME *) prSwRfb->pvHeader;

	prAdapter->ucT2LMRxDialogToken = prRxFrame->ucDialogToken;

	/* Process priority access multi-link IE */
	pucIE = (uint8_t *) &prRxFrame->aucT2LM[0];
	u2IELength = prSwRfb->u2PacketLen -
		OFFSET_OF(struct ACTION_T2LM_REQ_FRAME, aucT2LM);
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		if (IE_ID(pucIE) == ELEM_ID_RESERVED &&
		    IE_ID_EXT(pucIE) == ELEM_EXT_ID_TID2LNK_MAP) {
			DBGLOG(RX, LOUD, "[T2LM] Req Frame\n");
			t2lmParseT2LMIE(prAdapter, prSwRfb->prStaRec, pucIE);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t t2lmProcessRsp(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
		struct MLD_STA_RECORD *prMldStaRec,
		struct MLD_BSS_INFO *prMldBssInfo)
{
	struct T2LM_INFO *prT2LMParams;
	struct ACTION_T2LM_RSP_FRAME *prRxFrame;
	uint8_t *pucIE;
	uint16_t u2IELength, u2Offset;

	if (!prAdapter || !prSwRfb || !prMldStaRec || !prMldBssInfo)
		return WLAN_STATUS_FAILURE;

	prT2LMParams = &prMldStaRec->rT2LMParams;

	prRxFrame = (struct ACTION_T2LM_RSP_FRAME *) prSwRfb->pvHeader;
	DBGLOG(RX, TRACE, "[T2LM] Frame content:");
	DBGLOG_MEM8(RX, TRACE, prRxFrame, prSwRfb->u2PacketLen);
	DBGLOG(RX, TRACE, "[T2LM] !=======================================!\n");

	if (prRxFrame->ucStatusCode != STATUS_CODE_SUCCESSFUL ||
		prRxFrame->ucDialogToken != prAdapter->ucT2LMTxDialogToken)
		return WLAN_STATUS_FAILURE;

	prAdapter->ucT2LMRxDialogToken = prRxFrame->ucDialogToken;

	/* Process priority access multi-link IE */
	pucIE = (uint8_t *) &prRxFrame->aucT2LM[0];
	u2IELength = prSwRfb->u2PacketLen -
		OFFSET_OF(struct ACTION_T2LM_RSP_FRAME, aucT2LM);

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		if (IE_ID(pucIE) == ELEM_ID_RESERVED &&
			IE_ID_EXT(pucIE) == ELEM_EXT_ID_TID2LNK_MAP) {
			DBGLOG(RX, LOUD, "[T2LM] Rsp Frame\n");
		}
	}

	if (prMldStaRec->eT2LMState == T2LM_STATE_REQ_PENDING) {
		cnmTimerStopTimer(prAdapter, &prMldStaRec->rT2LMFsmTimer);
		if (prT2LMParams->u4SwitchDelayMs == 0) {
			t2lmFsmSteps(prAdapter, prMldStaRec,
				T2LM_STATE_REQ_DURATION);
		} else {
			t2lmFsmSteps(prAdapter, prMldStaRec,
				T2LM_STATE_REQ_SWITCH);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t t2lmProcessTeardown(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		struct MLD_STA_RECORD *prMldStaRec,
		struct MLD_BSS_INFO *prMldBssInfo)
{
	struct ACTION_T2LM_TEARDOWN_FRAME *prRxFrame;

	if (!prAdapter || !prSwRfb || !prMldStaRec || !prMldBssInfo)
		return WLAN_STATUS_FAILURE;

	if (prMldStaRec->eT2LMState == T2LM_STATE_ADV_DURATION
		|| prMldStaRec->eT2LMState == T2LM_STATE_ADV_SWITCH) {
		DBGLOG(ML, WARN, "T2LM state:[%s], teardown fail\n",
				apucDebugT2LMState[prMldStaRec->eT2LMState]);
		return WLAN_STATUS_FAILURE;
	}

	prRxFrame = (struct ACTION_T2LM_TEARDOWN_FRAME *) prSwRfb->pvHeader;

	t2lmFsmSteps(prAdapter, prMldStaRec, T2LM_STATE_IDLE);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to process the T2LM action frame.
 *        Called by: Handle Rx mgmt request
 * @param     prAdapter      Pointer to the Adapter structure.
 * @param     prSwRfb        Pointer to processing action frame
 *                           valid action code: TID2LINK_REQUEST [0]
 *                                              TID2LINK_RESPONSE [1]
 *                                              TID2LINK_TEARDOWN [2]
 */
/*----------------------------------------------------------------------------*/
void t2lmProcessAction(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct WLAN_ACTION_FRAME *prRxFrame;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct MLD_STA_RECORD *prMldStaRec;
	struct MLD_BSS_INFO *prMldBssInfo;
	struct T2LM_INFO *prT2LMParams;
	uint32_t rStatus;

	if (!prAdapter || !prSwRfb)
		return;

	if (prAdapter->rWifiVar.ucT2LMNegotiationSupport == T2LM_NO_SUPPORT) {
		DBGLOG(ML, ERROR,
			"T2LM is disabled, NegotiationSupport set to 0\n");
		return;
	}

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return;

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldStaRec || !prMldBssInfo) {
		DBGLOG(RX, ERROR,
			"prMldStaRec or prMldBssInfo equal to NULL\n");
		return;
	}

	prRxFrame = (struct WLAN_ACTION_FRAME *)prSwRfb->pvHeader;

	DBGLOG(RX, DEBUG, "Received T2LM action:%u\n", prRxFrame->ucAction);

	prT2LMParams = kalMemZAlloc(sizeof(*prT2LMParams), VIR_MEM_TYPE);
	if (prT2LMParams == NULL)
		return;

	switch (prRxFrame->ucAction) {
	case TID2LINK_REQUEST:
		if (prMldStaRec->eT2LMState == T2LM_STATE_REQ_PENDING) {
			DBGLOG(RX, WARN,
				"Received T2LM [REQ], T2LM state:[PENDING], Skip REQ\n");
			break;
		}

		t2lmFsmSteps(prAdapter, prMldStaRec, T2LM_STATE_REQ_PENDING);
		rStatus = t2lmProcessReq(prAdapter, prSwRfb,
				prMldStaRec, prMldBssInfo);
		if (rStatus == WLAN_STATUS_SUCCESS) {
			prAdapter->ucT2LMReqRetryCnt = 0;
			t2lmSend(prAdapter, TID2LINK_RESPONSE,
				prStaRec, prT2LMParams);
		} else {
			t2lmFsmSteps(prAdapter, prMldStaRec,
				T2LM_STATE_IDLE);
		}
		break;
	case TID2LINK_RESPONSE:
		rStatus = t2lmProcessRsp(prAdapter, prSwRfb,
				prMldStaRec, prMldBssInfo);
		break;
	case TID2LINK_TEARDOWN:
		rStatus = t2lmProcessTeardown(prAdapter, prSwRfb,
				prMldStaRec, prMldBssInfo);
		break;
	default:
		DBGLOG(RX, ERROR, "Action unexpected %u\n",
				prRxFrame->ucAction);
		break;
	}
	kalMemFree(prT2LMParams, VIR_MEM_TYPE, sizeof(*prT2LMParams));
}

void t2lmMldStaRecBackup(struct ADAPTER *prAdapter,
		struct MLD_STA_RECORD *prMldStaRec,
		struct T2LM_INFO *prT2LMParams)
{
	struct STA_RECORD *prCurrStarec;
	struct LINK *prStarecList;
	uint8_t ucTidBitmap;
	int i;

	kalMemCopy(&prMldStaRec->rT2LMParams, prT2LMParams,
			sizeof(struct T2LM_INFO));

	prStarecList = &prMldStaRec->rStarecList;

	LINK_FOR_EACH_ENTRY(prCurrStarec, prStarecList, rLinkEntryMld,
		    struct STA_RECORD) {
		ucTidBitmap = 0xff;

		if (prCurrStarec->ucLinkId > 14) {
			DBGLOG(TX, ERROR,
				"Linkid = %d, sta idx %d is invalid\n",
				prCurrStarec->ucLinkId,
				prCurrStarec->ucIndex);
			continue;
		}

		if (prT2LMParams->ucDefaultLM == 0
			&& prT2LMParams->ucLMSize == 1) {
			/*Link Mapping of Tid n, size is 1 octet*/
			for (i = 0; i < MAX_NUM_T2LM_TIDS; i++) {
				if (prT2LMParams->ucLMIndicator & BIT(i)) {
					if (prT2LMParams->au2LMTid[i]
						& BIT(prCurrStarec
							->ucLinkId))
						ucTidBitmap |= BIT(i);
					else
						ucTidBitmap &= ~(BIT(i));
				}
			}
		} else if (prT2LMParams->ucDefaultLM == 0
			&& prT2LMParams->ucLMSize == 0) {
			/*Link Mapping of Tid n, size is 2 octet*/
			for (i = 0; i < MAX_NUM_T2LM_TIDS; i++) {
				if (prT2LMParams->ucLMIndicator & BIT(i)) {
					if (prT2LMParams->au2LMTid[i]
						& BIT(prCurrStarec
							->ucLinkId))
						ucTidBitmap |= BIT(i);
					else
						ucTidBitmap &= ~(BIT(i));
				}
			}
		}

		DBGLOG(TX, TRACE, "Linkid = %d, ucTidBitmap = 0x%02x\n",
			prCurrStarec->ucLinkId,
			ucTidBitmap);
		switch (prMldStaRec->rT2LMParams.ucDirection) {
		case T2LM_DIRECTION_DL:
			prCurrStarec->ucPendingDLTidBitmap = ucTidBitmap;
			break;
		case T2LM_DIRECTION_UL:
			prCurrStarec->ucPendingULTidBitmap = ucTidBitmap;
			break;
		case T2LM_DIRECTION_DL_UL:
			prCurrStarec->ucPendingDLTidBitmap = ucTidBitmap;
			prCurrStarec->ucPendingULTidBitmap = ucTidBitmap;
			break;
		default:
			break;
		}
	}
}

void t2lmMldStaRecUpdate(struct ADAPTER *prAdapter,
		struct MLD_STA_RECORD *prMldStaRec, u_int8_t fgSendcmd)
{
	struct STA_RECORD *prCurrStarec;
	struct LINK *prStarecList;

	prStarecList = &prMldStaRec->rStarecList;

	LINK_FOR_EACH_ENTRY(prCurrStarec, prStarecList,
		rLinkEntryMld, struct STA_RECORD) {
		switch (prMldStaRec->rT2LMParams.ucDirection) {
		case T2LM_DIRECTION_DL:
			prCurrStarec->ucDLTidBitmap = prCurrStarec
				->ucPendingDLTidBitmap;
			break;
		case T2LM_DIRECTION_UL:
			prCurrStarec->ucULTidBitmap = prCurrStarec
				->ucPendingULTidBitmap;
			break;
		case T2LM_DIRECTION_DL_UL:
			prCurrStarec->ucDLTidBitmap = prCurrStarec
				->ucPendingDLTidBitmap;
			prCurrStarec->ucULTidBitmap = prCurrStarec
				->ucPendingULTidBitmap;
			break;
		default:
			break;
		}
	}

	if (fgSendcmd)
		mldUpdateTidBitmap(prAdapter, prMldStaRec);
}

void t2lmReset(struct ADAPTER *prAdapter,
		struct MLD_STA_RECORD *prMldStaRec)
{
	struct STA_RECORD *prCurrStarec;
	struct T2LM_INFO *prT2LMParams;
	struct LINK *prStarecList;

	if (!prMldStaRec)
		return;

	prT2LMParams = &prMldStaRec->rT2LMParams;
	kalMemZero(prT2LMParams, sizeof(struct T2LM_INFO));

	prStarecList = &prMldStaRec->rStarecList;
	LINK_FOR_EACH_ENTRY(prCurrStarec, prStarecList,
		rLinkEntryMld, struct STA_RECORD) {
		prCurrStarec->ucULTidBitmap = 0xff;
		prCurrStarec->ucDLTidBitmap = 0xff;
	}
	mldUpdateTidBitmap(prAdapter, prMldStaRec);

	cnmTimerStopTimer(prAdapter, &prMldStaRec->rT2LMTimer);
}

#endif /* CFG_SUPPORT_802_11BE_T2LM */
