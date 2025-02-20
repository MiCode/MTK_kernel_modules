// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "wnm.c"
 *    \brief  This file includes the 802.11v default vale and functions.
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

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define WNM_MAX_TOD_ERROR 0
#define WNM_MAX_TOA_ERROR 0
#define MICRO_TO_10NANO(x) ((x)*100)
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
#if CFG_SUPPORT_802_11V_TIMING_MEASUREMENT
static uint8_t ucTimingMeasToken;
#endif

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
#if CFG_SUPPORT_802_11V_TIMING_MEASUREMENT
static uint32_t
wnmRunEventTimgingMeasTxDone(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsduInfo,
			     enum ENUM_TX_RESULT_CODE rTxDoneStatus);

static void wnmComposeTimingMeasFrame(struct ADAPTER *prAdapter,
				      struct STA_RECORD *prStaRec,
				      PFN_TX_DONE_HANDLER pfTxDoneHandler);

static void wnmTimingMeasRequest(struct ADAPTER *prAdapter,
				 struct SW_RFB *prSwRfb);
#endif
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 *
 * \brief This routine is called to process the 802.11v wnm category action
 * frame.
 *
 *
 * \note
 *      Called by: Handle Rx mgmt request
 */
/*----------------------------------------------------------------------------*/
void wnmWNMAction(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct WLAN_ACTION_FRAME *prRxFrame;

	prRxFrame = (struct WLAN_ACTION_FRAME *)prSwRfb->pvHeader;

	DBGLOG(WNM, TRACE, "WNM action frame: %d from " MACSTR "\n",
	       prRxFrame->ucAction, MAC2STR(prRxFrame->aucSrcAddr));

	switch (prRxFrame->ucAction) {
#if CFG_SUPPORT_802_11V_TIMING_MEASUREMENT
	case ACTION_WNM_TIMING_MEASUREMENT_REQUEST:
		wnmTimingMeasRequest(prAdapter, prSwRfb);
		break;
#endif
#if CFG_AP_80211V_SUPPORT
	case ACTION_WNM_BSS_TRANSITION_MANAGEMENT_RSP:
		wnmMulAPAgentRecvBTMResponse(prAdapter, prSwRfb);
		break;
#endif /* CFG_AP_80211V_SUPPORT */
	case ACTION_WNM_BSS_TRANSITION_MANAGEMENT_REQ:
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
#if CFG_SUPPORT_802_11V_BTM_OFFLOAD
		/* btm offload */
		wnmRecvBTMRequest(prAdapter, prSwRfb);
#else
		DBGLOG(RX, INFO,
		       "WNM: action frame %d, try to send to supplicant\n",
		       prRxFrame->ucAction);
		aisFuncValidateRxActionFrame(prAdapter, prSwRfb);
#endif
#endif /* CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT */
		break;
	case ACTION_WNM_NOTIFICATION_REQUEST:
	default:
		DBGLOG(RX, INFO,
		       "WNM: action frame %d, try to send to supplicant\n",
		       prRxFrame->ucAction);
		aisFuncValidateRxActionFrame(prAdapter, prSwRfb);
		break;
	}
}

#if CFG_SUPPORT_802_11V_TIMING_MEASUREMENT
/*----------------------------------------------------------------------------*/
/*!
 *
 * \brief This routine is called to report timing measurement data.
 *
 */
/*----------------------------------------------------------------------------*/
void wnmReportTimingMeas(struct ADAPTER *prAdapter, uint8_t ucStaRecIndex,
			 uint32_t u4ToD, uint32_t u4ToA)
{
	struct STA_RECORD *prStaRec;

	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIndex);

	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return;

	DBGLOG(WNM, TRACE, "WNM: wnmReportTimingMeas: u4ToD %x u4ToA %x", u4ToD,
	       u4ToA);

	if (!prStaRec->rWNMTimingMsmt.ucTrigger)
		return;

	prStaRec->rWNMTimingMsmt.u4ToD = MICRO_TO_10NANO(u4ToD);
	prStaRec->rWNMTimingMsmt.u4ToA = MICRO_TO_10NANO(u4ToA);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle TxDone(TimingMeasurement) Event.
 *
 * @param[in] prAdapter      Pointer to the Adapter structure.
 * @param[in] prMsduInfo     Pointer to the MSDU_INFO_T.
 * @param[in] rTxDoneStatus  Return TX status of the Timing Measurement frame.
 *
 * @retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
static uint32_t
wnmRunEventTimgingMeasTxDone(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsduInfo,
			     enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct STA_RECORD *prStaRec;

	DBGLOG(WNM, LOUD, "WNM: EVENT-TX DONE: Current Time = %ld\n",
	       kalGetTimeTick());

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return WLAN_STATUS_SUCCESS; /* For the case of replying ERROR
					     * STATUS CODE
					     */

	DBGLOG(WNM, TRACE,
	       "WNM: wnmRunEventTimgingMeasTxDone: ucDialog %d ucFollowUp %d u4ToD %x u4ToA %x",
	       prStaRec->rWNMTimingMsmt.ucDialogToken,
	       prStaRec->rWNMTimingMsmt.ucFollowUpDialogToken,
	       prStaRec->rWNMTimingMsmt.u4ToD, prStaRec->rWNMTimingMsmt.u4ToA);

	prStaRec->rWNMTimingMsmt.ucFollowUpDialogToken =
		prStaRec->rWNMTimingMsmt.ucDialogToken;
	prStaRec->rWNMTimingMsmt.ucDialogToken = ++ucTimingMeasToken;

	wnmComposeTimingMeasFrame(prAdapter, prStaRec, NULL);

	return WLAN_STATUS_SUCCESS;

} /* end of wnmRunEventTimgingMeasTxDone() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the Timing Measurement frame.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
static void wnmComposeTimingMeasFrame(struct ADAPTER *prAdapter,
				      struct STA_RECORD *prStaRec,
				      PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct BSS_INFO *prBssInfo;
	struct ACTION_UNPROTECTED_WNM_TIMING_MEAS_FRAME *prTxFrame;
	uint16_t u2PayloadLen;

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex];
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(
		prAdapter, MAC_TX_RESERVED_FIELD + PUBLIC_ACTION_MAX_LEN);

	if (!prMsduInfo)
		return;

	prTxFrame = (struct ACTION_UNPROTECTED_WNM_TIMING_MEAS_FRAME
			     *)((uint32_t)(prMsduInfo->prPacket) +
				MAC_TX_RESERVED_FIELD);

	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	prTxFrame->ucCategory = CATEGORY_UNPROTECTED_WNM_ACTION;
	prTxFrame->ucAction = ACTION_UNPROTECTED_WNM_TIMING_MEASUREMENT;

	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = prStaRec->rWNMTimingMsmt.ucDialogToken;
	prTxFrame->ucFollowUpDialogToken =
		prStaRec->rWNMTimingMsmt.ucFollowUpDialogToken;
	prTxFrame->u4ToD = prStaRec->rWNMTimingMsmt.u4ToD;
	prTxFrame->u4ToA = prStaRec->rWNMTimingMsmt.u4ToA;
	prTxFrame->ucMaxToDErr = WNM_MAX_TOD_ERROR;
	prTxFrame->ucMaxToAErr = WNM_MAX_TOA_ERROR;

	u2PayloadLen = 2 + ACTION_UNPROTECTED_WNM_TIMING_MEAS_LEN;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     WLAN_MAC_MGMT_HEADER_LEN + u2PayloadLen, pfTxDoneHandler,
		     MSDU_RATE_MODE_AUTO);

	DBGLOG(WNM, TRACE,
	       "WNM: wnmComposeTimingMeasFrame: ucDialogToken %d ucFollowUpDialogToken %d u4ToD %x u4ToA %x\n",
	       prTxFrame->ucDialogToken, prTxFrame->ucFollowUpDialogToken,
	       prTxFrame->u4ToD, prTxFrame->u4ToA);

	/* 4 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return;

} /* end of wnmComposeTimingMeasFrame() */

/*----------------------------------------------------------------------------*/
/*!
 *
 * \brief This routine is called to process the 802.11v timing measurement
 * request.
 *
 *
 * \note
 *      Handle Rx mgmt request
 */
/*----------------------------------------------------------------------------*/
static void wnmTimingMeasRequest(struct ADAPTER *prAdapter,
				 struct SW_RFB *prSwRfb)
{
	struct ACTION_WNM_TIMING_MEAS_REQ_FRAME *prRxFrame = NULL;
	struct STA_RECORD *prStaRec;

	prRxFrame =
		(struct ACTION_WNM_TIMING_MEAS_REQ_FRAME *)prSwRfb->pvHeader;
	if (!prRxFrame)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return;

	DBGLOG(WNM, TRACE,
	       "WNM: Received Timing Measuremen Request from " MACSTR "\n",
	       MAC2STR(prStaRec->aucMacAddr));

	/* reset timing msmt */
	prStaRec->rWNMTimingMsmt.fgInitiator = TRUE;
	prStaRec->rWNMTimingMsmt.ucTrigger = prRxFrame->ucTrigger;
	if (!prRxFrame->ucTrigger)
		return;
	prStaRec->rWNMTimingMsmt.ucDialogToken = ++ucTimingMeasToken;
	prStaRec->rWNMTimingMsmt.ucFollowUpDialogToken = 0;
	wnmComposeTimingMeasFrame(prAdapter, prStaRec,
				  wnmRunEventTimgingMeasTxDone);
}

#if WNM_UNIT_TEST
void wnmTimingMeasUnitTest1(struct ADAPTER *prAdapter, uint8_t ucStaRecIndex)
{
	struct STA_RECORD *prStaRec;

	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIndex);
	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return;
	DBGLOG(WNM, INFO,
	       "WNM: Test Timing Measuremen Request from " MACSTR "\n",
	       MAC2STR(prStaRec->aucMacAddr));
	prStaRec->rWNMTimingMsmt.fgInitiator = TRUE;
	prStaRec->rWNMTimingMsmt.ucTrigger = 1;
	prStaRec->rWNMTimingMsmt.ucDialogToken = ++ucTimingMeasToken;
	prStaRec->rWNMTimingMsmt.ucFollowUpDialogToken = 0;
	wnmComposeTimingMeasFrame(prAdapter, prStaRec,
				  wnmRunEventTimgingMeasTxDone);
}
#endif

#endif /* CFG_SUPPORT_802_11V_TIMING_MEASUREMENT */

static uint32_t wnmBTMQueryTxDone(struct ADAPTER *prAdapter,
				  struct MSDU_INFO *prMsduInfo,
				  enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	DBGLOG(WNM, INFO, "BTM: Query Frame Tx Done, Status %d\n",
	       rTxDoneStatus);
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the BTM Query frame.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void wnmSendBTMQueryFrame(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec, uint8_t ucQueryReason)
{
	struct MSDU_INFO *prMsduInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct ACTION_BTM_QUERY_FRAME *prTxFrame = NULL;
	struct BSS_TRANSITION_MGT_PARAM *prBtmParam;
	static uint8_t ucToken = 1;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	prBtmParam = aisGetBTMParam(prAdapter, prStaRec->ucBssIndex);
	prBtmParam->ucQueryDialogToken = ucToken++;
	prBtmParam->fgWaitBtmRequest = TRUE;

	if (!prBssInfo) {
		DBGLOG(WNM, INFO, "BTM: invalid BSS_INFO\n");
		return;
	}

	/* 1 Allocate MSDU Info */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(
		prAdapter, MAC_TX_RESERVED_FIELD + PUBLIC_ACTION_MAX_LEN);
	if (!prMsduInfo)
		return;
	prTxFrame = (struct ACTION_BTM_QUERY_FRAME
			     *)((uintptr_t)(prMsduInfo->prPacket) +
				MAC_TX_RESERVED_FIELD);

	/* 2 Compose The Mac Header. */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;
	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);
	prTxFrame->ucCategory = CATEGORY_WNM_ACTION;
	prTxFrame->ucAction = ACTION_WNM_BSS_TRANSITION_MANAGEMENT_QUERY;

	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = prBtmParam->ucQueryDialogToken;
	prTxFrame->ucQueryReason = ucQueryReason;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     WLAN_MAC_MGMT_HEADER_LEN + 4,
		     wnmBTMQueryTxDone, MSDU_RATE_MODE_AUTO);

	DBGLOG(WNM, INFO, "BTM: Query token %d, reason %d\n",
	       prTxFrame->ucDialogToken, prTxFrame->ucQueryReason);

	/* 5 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

#if (CFG_SUPPORT_REPORT_LOG == 1)
	wnmLogBTMQueryReport(
		prAdapter,
		prStaRec,
		prTxFrame);
#endif
}				/* end of wnmComposeBTMQueryFrame() */

#if CFG_SUPPORT_802_11V_BTM_OFFLOAD

static uint32_t wnmBTMResponseTxDone(struct ADAPTER *prAdapter,
				     struct MSDU_INFO *prMsduInfo,
				     enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	DBGLOG(WNM, INFO, "Bss%d BTM Resp Tx Done Status %d\n",
		prMsduInfo->ucBssIndex, rTxDoneStatus);

#if CFG_EXT_ROAMING_WTC
	wnmWtcCheckDiconnect(prAdapter, prMsduInfo->ucBssIndex);
#endif

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the BTM Response frame.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void wnmSendBTMResponseFrame(struct ADAPTER *adapter,
	struct STA_RECORD *staRec, PFN_TX_DONE_HANDLER pfTxDoneHandler,
	uint8_t dialogToken, uint8_t status, uint8_t reason, uint8_t delay,
	const uint8_t *bssid)
{
	struct MSDU_INFO *prMsduInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct ACTION_BTM_RSP_FRAME *prTxFrame = NULL;
	uint16_t u2PayloadLen = 0;
	struct BSS_TRANSITION_MGT_PARAM *prBtmParam;
	uint8_t *pucOptInfo = NULL;

	if (!staRec) {
		DBGLOG(WNM, INFO, "BTM: No station record found\n");
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(adapter, staRec->ucBssIndex);
	prBtmParam = aisGetBTMParam(adapter, staRec->ucBssIndex);
	prBtmParam->ucStatusCode = status;

	if (!prBssInfo) {
		DBGLOG(WNM, INFO, "BTM: invalid BSS_INFO\n");
		return;
	}

	/* 1 Allocate MSDU Info */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(
		adapter, MAC_TX_RESERVED_FIELD + PUBLIC_ACTION_MAX_LEN);
	if (!prMsduInfo)
		return;
	prTxFrame = (struct ACTION_BTM_RSP_FRAME *)
		((uintptr_t)(prMsduInfo->prPacket) + MAC_TX_RESERVED_FIELD);

	/* 2 Compose The Mac Header. */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, staRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	prTxFrame->ucCategory = CATEGORY_WNM_ACTION;
	prTxFrame->ucAction = ACTION_WNM_BSS_TRANSITION_MANAGEMENT_RSP;

	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = dialogToken;
	prTxFrame->ucStatusCode = status;
	prTxFrame->ucBssTermDelay = delay;
	pucOptInfo = &prTxFrame->aucOptInfo[0];

	if (bssid) {
		COPY_MAC_ADDR(pucOptInfo, bssid);
		pucOptInfo += MAC_ADDR_LEN;
		u2PayloadLen += MAC_ADDR_LEN;
	} else if (status == WNM_BSS_TM_ACCEPT) {
		/*
		 * P802.11-REVmc clarifies that the Target BSSID field is always
		 * present when status code is zero, so use a fake value here if
		 * no BSSID is yet known.
		 */
		COPY_MAC_ADDR(pucOptInfo, "\0\0\0\0\0\0");
		pucOptInfo += MAC_ADDR_LEN;
		u2PayloadLen += MAC_ADDR_LEN;
	}

	/* TODO: add candidates list */

#ifdef CFG_SUPPORT_MBO
	if (status != WNM_BSS_TM_ACCEPT && prBtmParam->fgIsMboPresent) {
		/*
		 * MBO IE requires 6 bytes without the attributes: EID (1),
		 * length (1), OUI (3), OUI type (1).
		 */
		*pucOptInfo++ = ELEM_ID_VENDOR;
		*pucOptInfo++ = 7;
		WLAN_SET_FIELD_BE32(pucOptInfo, MBO_IE_VENDOR_TYPE);
		pucOptInfo += 4;
		*pucOptInfo++ = MBO_ATTR_ID_TRANSITION_REJECT_REASON;
		*pucOptInfo++ = 1;
		*pucOptInfo++ = reason;
		u2PayloadLen += 9;
	}
#endif

#if (CFG_EXT_ROAMING_WTC && CFG_SUPPORT_REPORT_LOG)
	APPEND_WTC_IE(prBtmParam,
		pucOptInfo,
		adapter,
		staRec,
		u2PayloadLen)
#endif

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(adapter, prMsduInfo, staRec->ucBssIndex,
		     staRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     OFFSET_OF(struct ACTION_BTM_RSP_FRAME, aucOptInfo) +
			     u2PayloadLen,
		     pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	nicTxConfigPktControlFlag(prMsduInfo,
			MSDU_CONTROL_FLAG_FORCE_LINK |
			MSDU_CONTROL_FLAG_DIS_MAT,
			TRUE);

	/* 5 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(adapter, prMsduInfo);

	DBGLOG(WNM, INFO,
		"BTM: response token=%d, status=%d, reason=%d, delay=%d, bssid=%p\n",
		dialogToken, status, reason, delay, bssid);

#if (CFG_SUPPORT_REPORT_LOG == 1)
	wnmLogBTMRespReport(
		adapter,
		prMsduInfo,
		dialogToken,
		status,
		reason,
		delay,
		bssid);
#endif
}				/* end of wnmComposeBTMResponseFrame() */

#if CFG_SUPPORT_MBO
void wnmMboIeTransReq(struct ADAPTER *adapter, uint8_t wnmMode,
		const uint8_t *ie, uint16_t len, uint8_t bssIndex)
{
	const uint8_t *pos;
	uint8_t id = 0, elen = 0;
	struct BSS_TRANSITION_MGT_PARAM *btm;
	struct IE_MBO_OCE *mbo;

	pos = kalFindIeMatchMask(ELEM_ID_VENDOR,
		ie, len, "\x50\x6f\x9a\x16",
		4, 2, NULL);

	if (!pos)
		return;

	if (len < (pos - ie))
		goto fail;

	btm = aisGetBTMParam(adapter, bssIndex);
	btm->fgIsMboPresent = TRUE;

	/* MBO IE */
	mbo = (struct IE_MBO_OCE *)pos;
	if (mbo->ucLength + 2 > (len - (pos - ie)))
		goto fail;

	len = mbo->ucLength - 4;
	pos = (uint8_t *)&mbo->aucSubElements[0];

	DBGLOG_MEM8(WNM, INFO, pos, len);

	while (len >= 2) {
		id = *pos++;
		elen = *pos++;
		len -= 2;

		if (elen > len)
			goto fail;

		switch (id) {
		case MBO_ATTR_ID_CELL_DATA_PREF:
			if (elen != 1)
				goto fail;

			DBGLOG(WNM, INFO, "cell preference=%d", *pos);
			break;
		case MBO_ATTR_ID_TRANSITION_REASON:
			if (elen != 1)
				goto fail;

			DBGLOG(WNM, INFO, "transition reason=%d", *pos);
			break;
		case MBO_ATTR_ID_ASSOC_RETRY_DELAY:
			if (elen != 2)
				goto fail;

			if (wnmMode & WNM_BSS_TM_REQ_BSS_TERMINATION_INCLUDED) {
				DBGLOG(WNM, WARN,
					   "Unexpected association retry delay, BSS is terminating");
				goto fail;
			} else if (wnmMode & WNM_BSS_TM_REQ_DISASSOC_IMMINENT) {
				uint16_t u2DisallowSec;
				struct BSS_DESC *bssDesc =
					aisGetTargetBssDesc(adapter, bssIndex);

				WLAN_GET_FIELD_16(pos, &u2DisallowSec);
				aisBssTmpDisallow(adapter, bssDesc,
					u2DisallowSec, 0);
			} else {
				DBGLOG(WNM, WARN,
					"MBO: Association retry delay attribute not in disassoc imminent mode");
			}
			break;
		case MBO_ATTR_ID_AP_CAPA_IND:
		case MBO_ATTR_ID_NON_PREF_CHAN_REPORT:
		case MBO_ATTR_ID_CELL_DATA_CAPA:
		case MBO_ATTR_ID_ASSOC_DISALLOW:
		case MBO_ATTR_ID_TRANSITION_REJECT_REASON:
			DBGLOG(WNM, WARN,
				   "Attribute %d should not be included in BTM Request frame",
				   id);
			break;
		default:
			DBGLOG(WNM, WARN, "Unknown attribute id %u", id);
			return;
		}

		pos += elen;
		len -= elen;
	}

	return;
fail:
	DBGLOG(WNM, WARN, "MBO IE parsing failed (id=%u len=%u left=%hu)",
		   id, elen, len);
}
#endif /* CFG_SUPPORT_MBO */

/*----------------------------------------------------------------------------*/
/*!
 *
 * \brief This routine is called to process the 802.11v BTM request.
 *
 *
 * \note
 *      Handle Rx mgmt request
 */
/*----------------------------------------------------------------------------*/
void wnmRecvBTMRequest(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct ACTION_BTM_REQ_FRAME *prRxFrame = NULL;
	struct BSS_TRANSITION_MGT_PARAM *prBtmParam;
	uint8_t *pucOptInfo = NULL;
	uint8_t ucRequestMode = 0;
	uint16_t u2TmpLen = 0;
	struct MSG_AIS_BSS_TRANSITION *prMsg = NULL;
	uint8_t ucBssIndex = secGetBssIdxByRfb(prAdapter, prSwRfb);
	uint8_t fgNeedResponse = FALSE;
	uint8_t ucStatus = 0;
	struct BSS_DESC *prBssDesc;
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	struct APS_INFO *prApsInfo;
	struct ROAMING_INFO *prRoamingFsmInfo;
	OS_SYSTIME rCurrentTime;

	prRxFrame = (struct ACTION_BTM_REQ_FRAME *) prSwRfb->pvHeader;
	if (!prRxFrame)
		return;
	if (prSwRfb->u2PacketLen <
		OFFSET_OF(struct ACTION_BTM_REQ_FRAME, aucOptInfo)) {
		DBGLOG(WNM, WARN,
		       "BTM: Request frame length is less than a standard BTM frame\n");
		return;
	}

	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);
	prBssDesc = scanSearchBssDescByBssid(prAdapter, prRxFrame->aucBSSID);
	prApsInfo = aisGetApsInfo(prAdapter, ucBssIndex);
	prBtmParam = aisGetBTMParam(prAdapter, ucBssIndex);
	prBtmParam->ucRspBssIndex = ucBssIndex;

	DBGLOG(WNM, INFO,
	       "BTM: Req 0x%x, VInt %d, DiscTimer %d, Token %d\n",
	       prRxFrame->ucRequestMode, prRxFrame->ucValidityInterval,
	       prRxFrame->u2DisassocTimer, prRxFrame->ucDialogToken);

	/* if BTM Request is for broadcast, don't send BTM Response */
	fgNeedResponse = !!(kalMemCmp(prRxFrame->aucDestAddr,
		"\xff\xff\xff\xff\xff\xff", MAC_ADDR_LEN));
	COPY_MAC_ADDR(prBtmParam->aucBSSID, prRxFrame->aucBSSID);
	prBtmParam->ucDialogToken = prRxFrame->ucDialogToken;
	prBtmParam->ucRequestMode = prRxFrame->ucRequestMode;
	prBtmParam->u4ReauthDelay = prBssDesc ?
		prRxFrame->u2DisassocTimer * prBssDesc->u2BeaconInterval :
		prRxFrame->u2DisassocTimer * 100;
	prBtmParam->fgIsMboPresent = FALSE;
#if CFG_EXT_ROAMING_WTC
	wnmWtcRecvBtmReq(prAdapter, ucBssIndex);
#endif
	prBtmParam->fgPendingResponse = fgNeedResponse;
	pucOptInfo = &prRxFrame->aucOptInfo[0];
	ucRequestMode = prBtmParam->ucRequestMode;
	u2TmpLen = OFFSET_OF(struct ACTION_BTM_REQ_FRAME, aucOptInfo);
	if (ucRequestMode & WNM_BSS_TM_REQ_BSS_TERMINATION_INCLUDED) {
		struct SUB_IE_BSS_TERM_DURATION *prBssTermDuration =
			(struct SUB_IE_BSS_TERM_DURATION *)pucOptInfo;

		if (prSwRfb->u2PacketLen <
			u2TmpLen + sizeof(struct SUB_IE_BSS_TERM_DURATION)) {
			DBGLOG(WNM, WARN,
			"BTM: Request frame length is too short\n");
			return;
		}

		prBtmParam->u2TermDuration = prBssTermDuration->u2Duration;
		kalMemCopy(prBtmParam->aucTermTsf,
			   prBssTermDuration->aucTermTsf, 8);
		pucOptInfo += sizeof(*prBssTermDuration);
		u2TmpLen += sizeof(*prBssTermDuration);
	}
	if (ucRequestMode & WNM_BSS_TM_REQ_ESS_DISASSOC_IMMINENT) {
		struct SESSION_INFO_URL *prSessionInfoURL =
			(struct SESSION_INFO_URL *)pucOptInfo;

		if (prSwRfb->u2PacketLen < u2TmpLen +
			OFFSET_OF(struct SESSION_INFO_URL, ucURLLength) + 1 ||
			prSwRfb->u2PacketLen <
			u2TmpLen + prSessionInfoURL->ucURLLength) {
			DBGLOG(WNM, WARN,
		       "BTM: Request frame length is less than a standard BTM frame\n");
			return;
		}

		if (prSessionInfoURL->ucURLLength >
			sizeof(prSessionInfoURL->aucURL)) {
			DBGLOG(WNM, WARN,
		       "BTM: ucURLLength is bigger than aucURL array size\n");
			return;
		}

		kalMemCopy(prBtmParam->aucSessionURL, prSessionInfoURL->aucURL,
			prSessionInfoURL->ucURLLength);
		prBtmParam->ucSessionURLLen = prSessionInfoURL->ucURLLength;
		u2TmpLen += prBtmParam->ucSessionURLLen + 1;
		pucOptInfo += prBtmParam->ucSessionURLLen + 1;
	}

#if CFG_SUPPORT_MBO
	if (prSwRfb->u2PacketLen > u2TmpLen) {
		wnmMboIeTransReq(prAdapter, ucRequestMode, pucOptInfo,
			prSwRfb->u2PacketLen - u2TmpLen, ucBssIndex);
	}
#endif

#if CFG_EXT_ROAMING_WTC
	if (prSwRfb->u2PacketLen > u2TmpLen) {
		if (wnmWtcCheckRejectAp(prAdapter,
			pucOptInfo,
			prSwRfb->u2PacketLen - u2TmpLen,
			ucBssIndex)) {
			ucStatus = WNM_BSS_TM_REJECT_UNSPECIFIED;
			goto send_response;
		}
	}
#endif

#if CFG_SUPPORT_802_11K
	aisResetNeighborApList(prAdapter, ucBssIndex);
#endif

	if (prAdapter->rWifiVar.ucAllowBtmReqMode != 0xff &&
	    !(prAdapter->rWifiVar.ucAllowBtmReqMode & ucRequestMode)) {
		DBGLOG(WNM, INFO,
			"Disallow request mode[%d][%d], reject btm req\n",
			prAdapter->rWifiVar.ucAllowBtmReqMode,
			ucRequestMode);
		ucStatus = WNM_BSS_TM_REJECT_UNSPECIFIED;
		goto send_response;
	}

	/* roaming */
	if (!roamingFsmInDecision(prAdapter, FALSE, ucBssIndex)) {
		DBGLOG(WNM, ERROR,
		    "Bss[%d] Recv btm req but there's ongoing roaming/CSA\n",
		    ucBssIndex);
		ucStatus = WNM_BSS_TM_REJECT_UNSPECIFIED;
		goto send_response;
	}

	if (ucRequestMode & WNM_BSS_TM_REQ_PREF_CAND_LIST_INCLUDED) {
#if CFG_SUPPORT_802_11K
#if (CFG_SUPPORT_REPORT_LOG == 1)
		wnmLogBTMRecvReq(prAdapter,
			ucBssIndex,
			prRxFrame,
			prSwRfb->u2PacketLen - u2TmpLen);
#endif

		if (prSwRfb->u2PacketLen <= u2TmpLen ||
		    !aisCollectNeighborAP(prAdapter, pucOptInfo,
					prSwRfb->u2PacketLen - u2TmpLen,
					prRxFrame->ucValidityInterval,
					ucBssIndex)) {
			DBGLOG(WNM, WARN,
			       "BTM: Candidate Include bit is set, but no candidate list\n");
			ucStatus = WNM_BSS_TM_REJECT_NO_SUITABLE_CANDIDATES;
			goto send_response;

		}
#endif
	}

	if (prBtmParam->fgWaitBtmRequest) {
		prBtmParam->fgWaitBtmRequest = FALSE;
		/* solicited btm already collects neighbor report so send resp*/
		if (prBtmParam->ucQueryDialogToken ==
				prBtmParam->ucDialogToken) {
			DBGLOG(WNM, INFO, "WNM: solicited btm token=%d\n",
				prBtmParam->ucDialogToken);
			ucStatus = WNM_BSS_TM_REJECT_UNSPECIFIED;
			goto send_response;
		}
	}

	if (ais->ucAisIndex != AIS_DEFAULT_INDEX) {
		DBGLOG(WNM, INFO, "WNM: [wlan%d] not support btm roaming\n",
			ais->ucAisIndex);
		ucStatus = WNM_BSS_TM_REJECT_UNSPECIFIED;
		goto send_response;
	}

	if (prAdapter->rWifiVar.u4RejectBtmReqReason) {
		DBGLOG(WNM, INFO, "WNM: reject btm req reason=%d\n",
			prAdapter->rWifiVar.u4RejectBtmReqReason);
		ucStatus = prAdapter->rWifiVar.u4RejectBtmReqReason;
		goto send_response;
	}

	if ((!(ucRequestMode & WNM_BSS_TM_REQ_DISASSOC_IMMINENT) &&
	     (ucRequestMode & WNM_BSS_TM_REQ_ABRIDGED) &&
	     !(ucRequestMode & WNM_BSS_TM_REQ_PREF_CAND_LIST_INCLUDED)) ||
	    ((ucRequestMode & WNM_BSS_TM_REQ_DISASSOC_IMMINENT) &&
	     (ucRequestMode & WNM_BSS_TM_REQ_ABRIDGED) &&
	     !(ucRequestMode & WNM_BSS_TM_REQ_PREF_CAND_LIST_INCLUDED))) {
		DBGLOG(WNM, WARN,
			"WNM: Invalid Frame mode (d,a,p)=(%lu,%lu,%lu)\n",
			ucRequestMode & WNM_BSS_TM_REQ_DISASSOC_IMMINENT,
			ucRequestMode & WNM_BSS_TM_REQ_ABRIDGED,
			ucRequestMode & WNM_BSS_TM_REQ_PREF_CAND_LIST_INCLUDED);
		ucStatus = WNM_BSS_TM_REJECT_UNSPECIFIED;
		goto send_response;
	}

	GET_CURRENT_SYSTIME(&rCurrentTime);
	if (prRoamingFsmInfo->rSkipBtmInfo.rFirstReqTime == 0 ||
	    (prRoamingFsmInfo->fgDisallowBtmRoaming &&
	     CHECK_FOR_TIMEOUT(rCurrentTime,
		prRoamingFsmInfo->rSkipBtmInfo.rFirstReqTime,
		SEC_TO_SYSTIME(prAdapter->rWifiVar.u2DisallowBtmTimeout)))) {
		kalMemZero(&prRoamingFsmInfo->rSkipBtmInfo,
			sizeof(struct ROAMING_SKIP_CONFIG));
		GET_CURRENT_SYSTIME(
			&(prRoamingFsmInfo->rSkipBtmInfo.rFirstReqTime));
		prRoamingFsmInfo->fgDisallowBtmRoaming = FALSE;
	} else if (prRoamingFsmInfo->rSkipBtmInfo.rFirstReqTime != 0 &&
		   prRoamingFsmInfo->fgDisallowBtmRoaming == FALSE &&
		   prRoamingFsmInfo->rSkipBtmInfo.ucConsecutiveCount > 0 &&
		   CHECK_FOR_TIMEOUT(rCurrentTime,
			prRoamingFsmInfo->rSkipBtmInfo.rFirstReqTime,
			SEC_TO_SYSTIME(prAdapter->rWifiVar.
				u2ConsecutiveBtmReqTimeout))) {
		prRoamingFsmInfo->rSkipBtmInfo.ucConsecutiveCount--;
		GET_CURRENT_SYSTIME(
		    &(prRoamingFsmInfo->rSkipBtmInfo.rFirstReqTime));
	} else if (prRoamingFsmInfo->rSkipBtmInfo.ucConsecutiveCount >=
			prAdapter->rWifiVar.ucConsecutiveBtmReqNum) {
		DBGLOG(AIS, ERROR, "Btm req fail - consecutive btm");
		prRoamingFsmInfo->fgDisallowBtmRoaming = TRUE;
		ucStatus = WNM_BSS_TM_REJECT_UNSPECIFIED;

		goto send_response;
	}
	prRoamingFsmInfo->rSkipBtmInfo.ucConsecutiveCount++;

	prMsg = (struct MSG_AIS_BSS_TRANSITION *)cnmMemAlloc(
		prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_AIS_BSS_TRANSITION));
	if (!prMsg) {
		DBGLOG(WNM, WARN, "BTM: Msg Hdr is NULL\n");
		ucStatus = WNM_BSS_TM_REJECT_UNSPECIFIED;
		goto send_response;
	}

	kalMemZero(prMsg, sizeof(*prMsg));
	prMsg->rMsgHdr.eMsgId = MID_WNM_AIS_BSS_TRANSITION;
	prMsg->ucBssIndex = ucBssIndex;
	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *)prMsg,
		    MSG_SEND_METHOD_BUF);

	return;
send_response:
	if (prBtmParam->fgPendingResponse) {
		prBtmParam->fgPendingResponse = false;
		wnmSendBTMResponseFrame(prAdapter,
			aisGetStaRecOfAP(prAdapter, prBtmParam->ucRspBssIndex),
			wnmBTMResponseTxDone,
			prBtmParam->ucDialogToken,
			ucStatus, MBO_TRANSITION_REJECT_REASON_UNSPECIFIED,
			0, NULL);
	}
}

#endif /* CFG_SUPPORT_802_11V_BTM_OFFLOAD */

#if CFG_AP_80211V_SUPPORT
static void wnmMulAPAgentBTMRequestDisassocTimerFunc(
		struct ADAPTER *prAdapter, uintptr_t ulParamPtr)
{
	struct STA_RECORD *prStaRec = NULL;
	struct BSS_INFO *prBssInfo = NULL;

	DBGLOG(WNM, INFO, "BTM: Request Disassociation Imminent\n");
	prStaRec = (struct STA_RECORD *) ulParamPtr;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (prStaRec->fgIsInUse)
		p2pFuncDisconnect(prAdapter,
			prBssInfo, prStaRec,
			TRUE,
			REASON_CODE_DISASSOC_INACTIVITY);
}

static uint32_t wnmMulAPAgentBTMRequestTxDone(struct ADAPTER *prAdapter,
				  struct MSDU_INFO *prMsduInfo,
				  enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct ACTION_BTM_REQ_FRAME *prTxFrame = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct STA_RECORD *prStaRec = NULL;
	int32_t u4DisassocTime = 0;

	DBGLOG(WNM, INFO,
		"BTM: Request Frame Tx Done, Status %d\n", rTxDoneStatus);
	if (rTxDoneStatus != TX_RESULT_SUCCESS)
		return WLAN_STATUS_FAILURE;

	prTxFrame = (struct ACTION_BTM_REQ_FRAME *)
		((uintptr_t)(prMsduInfo->prPacket) +
				MAC_TX_RESERVED_FIELD);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	/* DisassocTimer */
	u4DisassocTime = prTxFrame->u2DisassocTimer;
	if ((prTxFrame->ucRequestMode & BIT(2)) && (u4DisassocTime != 0)) {
		u4DisassocTime *= prBssInfo->u2BeaconInterval;
		if (timerPendingTimer(&prStaRec->rBTMReqDisassocTimer)) {
			cnmTimerStopTimer(prAdapter,
				&prStaRec->rBTMReqDisassocTimer);
			DBGLOG(WNM, INFO, "BTM: update DisassocTimer\n");
		}
		cnmTimerInitTimer(prAdapter, &prStaRec->rBTMReqDisassocTimer,
				(PFN_MGMT_TIMEOUT_FUNC)
				wnmMulAPAgentBTMRequestDisassocTimerFunc,
				(uintptr_t) prStaRec);
		cnmTimerStartTimer(prAdapter,
			&prStaRec->rBTMReqDisassocTimer, u4DisassocTime);
		DBGLOG(WNM, INFO,
			"BTM: disconnect after %d ms\n", u4DisassocTime);
	}

	return WLAN_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the BTM Request frame.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T.
 * @param[in] prSetBtmReqInfo        Pointer to the PARAM_CUSTOM_BTM_REQ_STRUCT.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void wnmMulAPAgentSendBTMRequestFrame(
		struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct PARAM_CUSTOM_BTM_REQ_STRUCT *prSetBtmReqInfo)
{
	struct MSDU_INFO *prMsduInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct ACTION_BTM_REQ_FRAME *prTxFrame = NULL;
	struct IE_NEIGHBOR_REPORT *prNeighborRpt = NULL;
	struct SUB_IE_BSS_CAND_PREFERENCE *prCandidatePrefer = NULL;
	uint16_t u2TxFrameLen = 500;
	uint32_t u4UrlLen = 0;
	uint32_t u4SessionInfoLen = 0;
	uint16_t u2NeighborListLen = 0;
	uint8_t *prOptionInfo = NULL;
	uint8_t ucIndex = 0;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(WNM, INFO, "WNM: prBssInfo rec is NULL\n");
		return;
	}

	if (!prStaRec) {
		DBGLOG(WNM, INFO, "WNM: sta rec is NULL\n");
		return;
	}

	if (!prStaRec) {
		DBGLOG(WNM, INFO, "WNM: sta rec is NULL\n");
		return;
	}

	/* 1 Allocate MSDU Info */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(
		prAdapter, MAC_TX_RESERVED_FIELD + u2TxFrameLen);
	if (!prMsduInfo)
		return;
	prTxFrame = (struct ACTION_BTM_REQ_FRAME *)
		((uintptr_t)(prMsduInfo->prPacket) +
				MAC_TX_RESERVED_FIELD);

	/* 2 Compose The Mac Header. */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;
	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);
	prTxFrame->ucCategory = CATEGORY_WNM_ACTION;
	prTxFrame->ucAction = ACTION_WNM_BSS_TRANSITION_MANAGEMENT_REQ;

	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = wnmGetBtmToken();
	prTxFrame->ucRequestMode = 0;
	prTxFrame->u2DisassocTimer = prSetBtmReqInfo->u2DisassocTimer;
	prTxFrame->ucValidityInterval = prSetBtmReqInfo->ucValidityInterval;

	prOptionInfo = &prTxFrame->aucOptInfo[0];
	if (prSetBtmReqInfo->ucEssImm) {
		prTxFrame->ucRequestMode |= BIT(4);
		u4UrlLen = kalStrLen(prSetBtmReqInfo->aucSessionUrl);
		*prOptionInfo = u4UrlLen;
		prOptionInfo += 1;
		kalMemCopy(prOptionInfo,
			&prSetBtmReqInfo->aucSessionUrl, u4UrlLen);
		u4SessionInfoLen = u4UrlLen + 1;
		prOptionInfo += u4UrlLen;
	}

	for (ucIndex = 0; ucIndex < prSetBtmReqInfo->ucTargetBSSIDCnt;
			ucIndex++) {
		prTxFrame->ucRequestMode |= BIT(0);
		prNeighborRpt = (struct IE_NEIGHBOR_REPORT *) prOptionInfo;
		prNeighborRpt->ucId = ELEM_ID_NEIGHBOR_REPORT;
		prNeighborRpt->ucLength = 6 + 4 + 1 + 1 + 1;
		COPY_MAC_ADDR(prNeighborRpt->aucBSSID,
			prSetBtmReqInfo->ucTargetBSSIDList[ucIndex].mMac);
		prNeighborRpt->u4BSSIDInfo =
			prSetBtmReqInfo->ucTargetBSSIDList[ucIndex].u4BSSIDInfo;
		prNeighborRpt->ucOperClass =
			prSetBtmReqInfo->ucTargetBSSIDList[ucIndex].ucOperClass;
		prNeighborRpt->ucChnlNumber =
			prSetBtmReqInfo->ucTargetBSSIDList[ucIndex].ucChannel;
		prNeighborRpt->ucPhyType =
			prSetBtmReqInfo->ucTargetBSSIDList[ucIndex].ucPhyType;

		/* subelement */
		prCandidatePrefer = (struct SUB_IE_BSS_CAND_PREFERENCE *)
			&prNeighborRpt->aucSubElem[0];
		prCandidatePrefer->ucSubId =
			ELEM_ID_NR_BSS_TRANSITION_CAND_PREF;
		prCandidatePrefer->ucLength = 1;
		prCandidatePrefer->ucPreference =
			prSetBtmReqInfo
			->ucTargetBSSIDList[ucIndex].ucPreference;
		prNeighborRpt->ucLength += (2 + prCandidatePrefer->ucLength);

		prOptionInfo += (2 + prNeighborRpt->ucLength);
		u2NeighborListLen += (2 + prNeighborRpt->ucLength);
	}

	if (prSetBtmReqInfo->ucAbridged)
		prTxFrame->ucRequestMode |= BIT(1);

	if (prTxFrame->u2DisassocTimer != 0)
		prTxFrame->ucRequestMode |= BIT(2);

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     WLAN_MAC_MGMT_HEADER_LEN + 7
		     + u4SessionInfoLen + u2NeighborListLen,
		     wnmMulAPAgentBTMRequestTxDone,
		     MSDU_RATE_MODE_AUTO);

	/* 5 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}				/* end of wnmComposeBTMQueryFrame() */

/*----------------------------------------------------------------------------*/
/*!
 *
 * \brief This routine is called to process the 802.11v BTM request.
 *
 *
 * \note
 *      Handle Rx mgmt request
 */
/*----------------------------------------------------------------------------*/
void wnmMulAPAgentRecvBTMResponse(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb)
{
	struct ACTION_BTM_RSP_FRAME *prRxFrame = NULL;
	uint8_t *pucOptInfo = NULL;
	uint16_t u2TmpLen = 0;
	int32_t i4Ret = 0;
	struct T_MULTI_AP_STA_STEERING_REPORT *prBtmReport = NULL;

	prRxFrame = (struct ACTION_BTM_RSP_FRAME *) prSwRfb->pvHeader;
	if (!prRxFrame)
		return;
	if (prSwRfb->u2PacketLen <
		OFFSET_OF(struct ACTION_BTM_RSP_FRAME, aucOptInfo)) {
		DBGLOG(WNM, WARN,
		       "BTM: Request frame length is less than a standard BTM frame\n");
		return;
	}

	prBtmReport = (struct T_MULTI_AP_STA_STEERING_REPORT *)
			kalMemAlloc(sizeof(
				struct T_MULTI_AP_STA_STEERING_REPORT),
			VIR_MEM_TYPE);
	if (prBtmReport == NULL) {
		DBGLOG(INIT, ERROR, "alloc memory fail\n");
		return;
	}

	kalMemZero(prBtmReport, sizeof(struct T_MULTI_AP_STA_STEERING_REPORT));
	COPY_MAC_ADDR(prBtmReport->mStaMac, prRxFrame->aucSrcAddr);
	COPY_MAC_ADDR(prBtmReport->mBssid, prRxFrame->aucBSSID);

	pucOptInfo = &prRxFrame->aucOptInfo[0];
	u2TmpLen = OFFSET_OF(struct ACTION_BTM_RSP_FRAME, aucOptInfo);

	prBtmReport->u8Status = prRxFrame->ucStatusCode;

	DBGLOG(WNM, INFO,
		"[SAP_Test] u2FrameCtrl = 0x%x\n", prRxFrame->u2FrameCtrl);
	DBGLOG(WNM, INFO,
		"[SAP_Test] u2Duration = %u\n", prRxFrame->u2Duration);
	DBGLOG(WNM, INFO,
		"[SAP_Test] aucDestAddr = " MACSTR "\n",
		MAC2STR(prRxFrame->aucDestAddr));
	DBGLOG(WNM, INFO,
		"[SAP_Test] aucSrcAddr = " MACSTR "\n",
		MAC2STR(prRxFrame->aucSrcAddr));
	DBGLOG(WNM, INFO,
		"[SAP_Test] aucBSSID = " MACSTR "\n",
		MAC2STR(prRxFrame->aucBSSID));
	DBGLOG(WNM, INFO,
		"[SAP_Test] u2SeqCtrl = %u\n", prRxFrame->u2SeqCtrl);
	DBGLOG(WNM, INFO,
		"[SAP_Test] ucCategory = %u\n", prRxFrame->ucCategory);
	DBGLOG(WNM, INFO,
		"[SAP_Test] ucAction = %u\n", prRxFrame->ucAction);
	DBGLOG(WNM, INFO,
		"[SAP_Test] ucDialogToken = %u\n", prRxFrame->ucDialogToken);
	DBGLOG(WNM, INFO,
		"[SAP_Test] ucStatusCode = %u\n", prRxFrame->ucStatusCode);
	DBGLOG(WNM, INFO,
		"[SAP_Test] ucBssTermDelay = %u\n", prRxFrame->ucBssTermDelay);
	if (prSwRfb->u2PacketLen >= u2TmpLen + MAC_ADDR_LEN &&
		prRxFrame->ucStatusCode == BSS_TRANSITION_MGT_STATUS_ACCEPT) {
		COPY_MAC_ADDR(prBtmReport->mDestBssid, pucOptInfo);
		pucOptInfo += MAC_ADDR_LEN;
		u2TmpLen += MAC_ADDR_LEN;
	}
	DBGLOG(WNM, INFO,
			"[SAP_Test] Target BSSID = " MACSTR "\n",
			MAC2STR(prBtmReport->mDestBssid));

	/* BSS Transition Candidate List Entries */
	while (prSwRfb->u2PacketLen > u2TmpLen) {
		DBGLOG_MEM8(WNM, INFO, pucOptInfo, pucOptInfo[1] + 2);
		pucOptInfo += pucOptInfo[1] + 2;
		u2TmpLen += pucOptInfo[1] + 2;
	}

	i4Ret = MulAPAgentMontorSendMsg(
		EV_WLAN_MULTIAP_STEERING_BTM_REPORT,
		prBtmReport, sizeof(*prBtmReport));
	if (i4Ret < 0)
		DBGLOG(AAA, ERROR,
			"EV_WLAN_MULTIAP_STEERING_BTM_REPORT nl send msg failed!\n");

	kalMemFree(prBtmReport,
		VIR_MEM_TYPE,
		sizeof(struct T_MULTI_AP_STA_STEERING_REPORT));
}
#endif /* CFG_AP_80211V_SUPPORT */
