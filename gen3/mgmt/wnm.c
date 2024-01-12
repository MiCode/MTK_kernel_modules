/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

/*
** Id: //Department/DaVinci/TRUNK/MT6620_5931_WiFi_Driver/mgmt/wnm.c#1
*/

/*
 * ! \file   "wnm.c"
 *  \brief  This file includes the 802.11v default vale and functions.
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

#define WNM_MAX_TOD_ERROR  0
#define WNM_MAX_TOA_ERROR  0
#define MICRO_TO_10NANO(x) ((x)*100)
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
#if CFG_SUPPORT_802_11V_TIMING_MEASUREMENT
static UINT_8 ucTimingMeasToken;
#endif
static UINT_8 ucBtmMgtToken = 1;

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
#if CFG_SUPPORT_802_11V_TIMING_MEASUREMENT
static WLAN_STATUS
wnmRunEventTimgingMeasTxDone(IN P_ADAPTER_T prAdapter,
			     IN P_MSDU_INFO_T prMsduInfo, IN ENUM_TX_RESULT_CODE_T rTxDoneStatus);

static VOID
wnmComposeTimingMeasFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec, IN PFN_TX_DONE_HANDLER pfTxDoneHandler);

static VOID wnmTimingMeasRequest(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);
#endif
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
*
* \brief This routine is called to process the 802.11v wnm category action frame.
*
*
* \note
*      Called by: Handle Rx mgmt request
*/
/*----------------------------------------------------------------------------*/
VOID wnmWNMAction(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
	P_WLAN_ACTION_FRAME prRxFrame;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prRxFrame = (P_WLAN_ACTION_FRAME) prSwRfb->pvHeader;
	DBGLOG(WNM, TRACE, "WNM action frame: %d from %pM\n", prRxFrame->ucAction, prRxFrame->aucSrcAddr);
	switch (prRxFrame->ucAction) {
#if CFG_SUPPORT_802_11V_TIMING_MEASUREMENT
	case ACTION_WNM_TIMING_MEASUREMENT_REQUEST:
		wnmTimingMeasRequest(prAdapter, prSwRfb);
		break;
#endif
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
	case ACTION_WNM_BSS_TRANSITION_MANAGEMENT_REQ:
		wnmRecvBTMRequest(prAdapter, prSwRfb);
		break;
#endif
	default:
		DBGLOG(WNM, TRACE, "Unsupported WNM action frame: %d\n", prRxFrame->ucAction);
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
VOID wnmReportTimingMeas(IN P_ADAPTER_T prAdapter, IN UINT_8 ucStaRecIndex, IN UINT_32 u4ToD, IN UINT_32 u4ToA)
{
	P_STA_RECORD_T prStaRec;

	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIndex);

	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return;

	DBGLOG(WNM, TRACE, "wnmReportTimingMeas: u4ToD %x u4ToA %x", u4ToD, u4ToA);

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
static WLAN_STATUS
wnmRunEventTimgingMeasTxDone(IN P_ADAPTER_T prAdapter,
			     IN P_MSDU_INFO_T prMsduInfo, IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
	P_STA_RECORD_T prStaRec;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	DBGLOG(WNM, LOUD, "EVENT-TX DONE: Current Time = %ld\n", kalGetTimeTick());

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return WLAN_STATUS_SUCCESS;	/* For the case of replying ERROR STATUS CODE */

	DBGLOG(WNM, TRACE,
	       "wnmRunEventTimgingMeasTxDone: ucDialog %d ucFollowUp %d u4ToD %x u4ToA %x",
		prStaRec->rWNMTimingMsmt.ucDialogToken,
		prStaRec->rWNMTimingMsmt.ucFollowUpDialogToken, prStaRec->rWNMTimingMsmt.u4ToD,
		prStaRec->rWNMTimingMsmt.u4ToA);

	prStaRec->rWNMTimingMsmt.ucFollowUpDialogToken = prStaRec->rWNMTimingMsmt.ucDialogToken;
	prStaRec->rWNMTimingMsmt.ucDialogToken = ++ucTimingMeasToken;

	wnmComposeTimingMeasFrame(prAdapter, prStaRec, NULL);

	return WLAN_STATUS_SUCCESS;

}				/* end of wnmRunEventTimgingMeasTxDone() */

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
static VOID
wnmComposeTimingMeasFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec, IN PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	P_MSDU_INFO_T prMsduInfo;
	P_BSS_INFO_T prBssInfo;
	P_ACTION_UNPROTECTED_WNM_TIMING_MEAS_FRAME prTxFrame;
	UINT_16 u2PayloadLen;

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex];
	ASSERT(prBssInfo);

	prMsduInfo = (P_MSDU_INFO_T) cnmMgtPktAlloc(prAdapter, MAC_TX_RESERVED_FIELD + PUBLIC_ACTION_MAX_LEN);

	if (!prMsduInfo)
		return;

	prTxFrame = (P_ACTION_UNPROTECTED_WNM_TIMING_MEAS_FRAME)
	    ((UINT_32) (prMsduInfo->prPacket) + MAC_TX_RESERVED_FIELD);

	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	prTxFrame->ucCategory = CATEGORY_UNPROTECTED_WNM_ACTION;
	prTxFrame->ucAction = ACTION_UNPROTECTED_WNM_TIMING_MEASUREMENT;

	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = prStaRec->rWNMTimingMsmt.ucDialogToken;
	prTxFrame->ucFollowUpDialogToken = prStaRec->rWNMTimingMsmt.ucFollowUpDialogToken;
	prTxFrame->u4ToD = prStaRec->rWNMTimingMsmt.u4ToD;
	prTxFrame->u4ToA = prStaRec->rWNMTimingMsmt.u4ToA;
	prTxFrame->ucMaxToDErr = WNM_MAX_TOD_ERROR;
	prTxFrame->ucMaxToAErr = WNM_MAX_TOA_ERROR;

	u2PayloadLen = 2 + ACTION_UNPROTECTED_WNM_TIMING_MEAS_LEN;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
		     prMsduInfo,
		     prStaRec->ucBssIndex,
		     prStaRec->ucIndex,
		     WLAN_MAC_MGMT_HEADER_LEN,
		     WLAN_MAC_MGMT_HEADER_LEN + u2PayloadLen, pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	DBGLOG(WNM, TRACE,
	       "wnmComposeTimingMeasFrame: ucDialogToken %d ucFollowUpDialogToken %d u4ToD %x u4ToA %x\n",
		prTxFrame->ucDialogToken, prTxFrame->ucFollowUpDialogToken, prTxFrame->u4ToD, prTxFrame->u4ToA);

	/* 4 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return;

}				/* end of wnmComposeTimingMeasFrame() */

/*----------------------------------------------------------------------------*/
/*!
*
* \brief This routine is called to process the 802.11v timing measurement request.
*
*
* \note
*      Handle Rx mgmt request
*/
/*----------------------------------------------------------------------------*/
static VOID wnmTimingMeasRequest(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
	P_ACTION_WNM_TIMING_MEAS_REQ_FRAME prRxFrame = NULL;
	P_STA_RECORD_T prStaRec;

	prRxFrame = (P_ACTION_WNM_TIMING_MEAS_REQ_FRAME) prSwRfb->pvHeader;
	if (!prRxFrame)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return;

	DBGLOG(WNM, TRACE, "IEEE 802.11: Received Timing Measuremen Request from "
			    MACSTR "\n", MAC2STR(prStaRec->aucMacAddr));

	/* reset timing msmt */
	prStaRec->rWNMTimingMsmt.fgInitiator = TRUE;
	prStaRec->rWNMTimingMsmt.ucTrigger = prRxFrame->ucTrigger;
	if (!prRxFrame->ucTrigger)
		return;

	prStaRec->rWNMTimingMsmt.ucDialogToken = ++ucTimingMeasToken;
	prStaRec->rWNMTimingMsmt.ucFollowUpDialogToken = 0;

	wnmComposeTimingMeasFrame(prAdapter, prStaRec, wnmRunEventTimgingMeasTxDone);
}

#if WNM_UNIT_TEST
VOID wnmTimingMeasUnitTest1(P_ADAPTER_T prAdapter, UINT_8 ucStaRecIndex)
{
	P_STA_RECORD_T prStaRec;

	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIndex);
	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return;

	DBGLOG(WNM, INFO, "IEEE 802.11v: Test Timing Measuremen Request from "
			   MACSTR "\n", MAC2STR(prStaRec->aucMacAddr));

	prStaRec->rWNMTimingMsmt.fgInitiator = TRUE;
	prStaRec->rWNMTimingMsmt.ucTrigger = 1;

	prStaRec->rWNMTimingMsmt.ucDialogToken = ++ucTimingMeasToken;
	prStaRec->rWNMTimingMsmt.ucFollowUpDialogToken = 0;

	wnmComposeTimingMeasFrame(prAdapter, prStaRec, wnmRunEventTimgingMeasTxDone);
}
#endif

#endif /* CFG_SUPPORT_802_11V_TIMING_MEASUREMENT */

UINT_8 wnmGetBtmToken(VOID)
{
	return ucBtmMgtToken++;
}

static WLAN_STATUS
wnmBTMQueryTxDone(IN P_ADAPTER_T prAdapter,
		  IN P_MSDU_INFO_T prMsduInfo, IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
	DBGLOG(WNM, INFO, "BTM Query Frame Tx Done, Status %d\n", rTxDoneStatus);
	return WLAN_STATUS_SUCCESS;
}

static WLAN_STATUS
wnmBTMResponseTxDone(IN P_ADAPTER_T prAdapter,
		     IN P_MSDU_INFO_T prMsduInfo, IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
	struct BSS_TRANSITION_MGT_PARAM_T *prBtm =
		&prAdapter->rWifiVar.rAisSpecificBssInfo.rBTMParam;
	P_AIS_FSM_INFO_T prAisFsmInfo = &prAdapter->rWifiVar.rAisFsmInfo;

	DBGLOG(WNM, INFO, "BTM Response Frame Tx Done Status %d\n", rTxDoneStatus);
	if (prBtm->fgPendingResponse && prAisFsmInfo->eCurrentState == AIS_STATE_SEARCH) {
		prBtm->fgPendingResponse = FALSE;
		aisFsmSteps(prAdapter, AIS_STATE_REQ_CHANNEL_JOIN);
	}
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will compose the Bss Transition Management Response frame.
*
* @param[in] prAdapter              Pointer to the Adapter structure.
* @param[in] prStaRec               Pointer to the STA_RECORD_T.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID
wnmSendBTMResponseFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
	P_MSDU_INFO_T prMsduInfo = NULL;
	P_BSS_INFO_T prBssInfo = NULL;
	struct ACTION_BTM_RSP_FRAME_T *prTxFrame = NULL;
	UINT_16 u2PayloadLen = 0;
	struct BSS_TRANSITION_MGT_PARAM_T *prBtmParam =
					&prAdapter->rWifiVar.rAisSpecificBssInfo.rBTMParam;
	PUINT_8 pucOptInfo = NULL;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	ASSERT(prBssInfo);

	/* 1 Allocate MSDU Info */
	prMsduInfo = (P_MSDU_INFO_T) cnmMgtPktAlloc(prAdapter, MAC_TX_RESERVED_FIELD + PUBLIC_ACTION_MAX_LEN);
	if (!prMsduInfo)
		return;
	prTxFrame = (struct ACTION_BTM_RSP_FRAME_T *)
	    ((ULONG) (prMsduInfo->prPacket) + MAC_TX_RESERVED_FIELD);

	/* 2 Compose The Mac Header. */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	prTxFrame->ucCategory = CATEGORY_WNM_ACTION;
	prTxFrame->ucAction = ACTION_WNM_BSS_TRANSITION_MANAGEMENT_RSP;

	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = prBtmParam->ucDialogToken;
	prBtmParam->ucDialogToken = 0; /* reset dialog token */
	prTxFrame->ucStatusCode = prBtmParam->ucStatusCode;
	prTxFrame->ucBssTermDelay = prBtmParam->ucTermDelay;
	pucOptInfo = &prTxFrame->aucOptInfo[0];
	if (prBtmParam->ucStatusCode == BSS_TRANSITION_MGT_STATUS_ACCEPT) {
		COPY_MAC_ADDR(pucOptInfo, prBtmParam->aucTargetBssid);
		pucOptInfo += MAC_ADDR_LEN;
		u2PayloadLen += MAC_ADDR_LEN;
	}
	if (prBtmParam->u2OurNeighborBssLen > 0) {
		kalMemCopy(pucOptInfo, prBtmParam->pucOurNeighborBss, prBtmParam->u2OurNeighborBssLen);
		kalMemFree(prBtmParam->pucOurNeighborBss, VIR_MEM_TYPE, prBtmParam->u2OurNeighborBssLen);
		prBtmParam->u2OurNeighborBssLen = 0;
		u2PayloadLen += prBtmParam->u2OurNeighborBssLen;
	}
	glNotifyDrvStatus(SND_BTM_RSP, (PVOID)prBtmParam);

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
		     prMsduInfo,
		     prStaRec->ucBssIndex,
		     prStaRec->ucIndex,
		     WLAN_MAC_MGMT_HEADER_LEN,
		     OFFSET_OF(struct ACTION_BTM_RSP_FRAME_T, aucOptInfo) + u2PayloadLen,
		     wnmBTMResponseTxDone, MSDU_RATE_MODE_AUTO);

	/* 5 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}				/* end of wnmComposeBTMResponseFrame() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will compose the Bss Transition Management Query frame.
*
* @param[in] prAdapter              Pointer to the Adapter structure.
* @param[in] prStaRec               Pointer to the STA_RECORD_T.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID
wnmSendBTMQueryFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
	P_MSDU_INFO_T prMsduInfo = NULL;
	P_BSS_INFO_T prBssInfo = NULL;
	struct ACTION_BTM_QUERY_FRAME_T *prTxFrame = NULL;
	struct BSS_TRANSITION_MGT_PARAM_T *prBtmParam =
				&prAdapter->rWifiVar.rAisSpecificBssInfo.rBTMParam;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	ASSERT(prBssInfo);

	/* 1 Allocate MSDU Info */
	prMsduInfo = (P_MSDU_INFO_T) cnmMgtPktAlloc(prAdapter, MAC_TX_RESERVED_FIELD + PUBLIC_ACTION_MAX_LEN);
	if (!prMsduInfo)
		return;
	prTxFrame = (struct ACTION_BTM_QUERY_FRAME_T *)
	    ((ULONG) (prMsduInfo->prPacket) + MAC_TX_RESERVED_FIELD);

	/* 2 Compose The Mac Header. */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;
	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);
	prTxFrame->ucCategory = CATEGORY_WNM_ACTION;
	prTxFrame->ucAction = ACTION_WNM_BSS_TRANSITION_MANAGEMENT_QUERY;

	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = prBtmParam->ucDialogToken;
	prTxFrame->ucQueryReason = prBtmParam->ucQueryReason;
	if (prBtmParam->u2OurNeighborBssLen > 0) {
		kalMemCopy(prTxFrame->pucNeighborBss, prBtmParam->pucOurNeighborBss, prBtmParam->u2OurNeighborBssLen);
		kalMemFree(prBtmParam->pucOurNeighborBss, VIR_MEM_TYPE, prBtmParam->u2OurNeighborBssLen);
		prBtmParam->u2OurNeighborBssLen = 0;
	}

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
		     prMsduInfo,
		     prStaRec->ucBssIndex,
		     prStaRec->ucIndex,
		     WLAN_MAC_MGMT_HEADER_LEN,
		     WLAN_MAC_MGMT_HEADER_LEN + 4 + prBtmParam->u2OurNeighborBssLen,
		     wnmBTMQueryTxDone, MSDU_RATE_MODE_AUTO);

	/* 5 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}				/* end of wnmComposeBTMQueryFrame() */

/*----------------------------------------------------------------------------*/
/*!
*
* \brief This routine is called to process the 802.11v Bss Transition Management request.
*
*
* \note
*      Handle Rx mgmt request
*/
/*----------------------------------------------------------------------------*/
VOID wnmRecvBTMRequest(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
	struct ACTION_BTM_REQ_FRAME_T *prRxFrame = NULL;
	struct BSS_TRANSITION_MGT_PARAM_T *prBtmParam =
			&prAdapter->rWifiVar.rAisSpecificBssInfo.rBTMParam;
	PUINT_8 pucOptInfo = NULL;
	UINT_8 ucRequestMode = 0;
	UINT_16 u2TmpLen = 0;
	struct MSG_AIS_BSS_TRANSITION_T *prMsg = NULL;
	enum WNM_AIS_BSS_TRANSITION eTransType = BSS_TRANSITION_NO_MORE_ACTION;

	prRxFrame = (struct ACTION_BTM_REQ_FRAME_T *) prSwRfb->pvHeader;
	if (!prRxFrame)
		return;
	if (prSwRfb->u2PacketLen < OFFSET_OF(struct ACTION_BTM_REQ_FRAME_T, aucOptInfo)) {
		DBGLOG(WNM, WARN, "BTM request frame length is less than a standard BTM frame\n");
		return;
	}
	prMsg = (struct MSG_AIS_BSS_TRANSITION_T *)
			cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_AIS_BSS_TRANSITION_T));
	if (!prMsg) {
		DBGLOG(WNM, WARN, "Msg Hdr is NULL\n");
		return;
	}
	kalMemZero(prMsg, sizeof(*prMsg));
	prBtmParam->ucRequestMode = prRxFrame->ucRequestMode;
	prBtmParam->u2DisassocTimer = prRxFrame->u2DisassocTimer;
	prMsg->ucToken = prRxFrame->ucDialogToken;
	pucOptInfo = &prRxFrame->aucOptInfo[0];
	ucRequestMode = prBtmParam->ucRequestMode;
	u2TmpLen = OFFSET_OF(struct ACTION_BTM_REQ_FRAME_T, aucOptInfo);
	if (ucRequestMode & BTM_REQ_MODE_DISC_IMM)
		eTransType = BSS_TRANSITION_REQ_ROAMING;
	if (ucRequestMode & BTM_REQ_MODE_BSS_TERM_INCLUDE) {
		struct SUB_IE_BSS_TERM_DURATION_T *prBssTermDuration =
				(struct SUB_IE_BSS_TERM_DURATION_T *)pucOptInfo;

		prBtmParam->u2TermDuration = prBssTermDuration->u2Duration;
		kalMemCopy(prBtmParam->aucTermTsf, prBssTermDuration->aucTermTsf, 8);
		pucOptInfo += sizeof(*prBssTermDuration);
		u2TmpLen += sizeof(*prBssTermDuration);
		eTransType = BSS_TRANSITION_REQ_ROAMING;
	}
	if (ucRequestMode & BTM_REQ_MODE_ESS_DISC_IMM) {
		kalMemCopy(prBtmParam->aucSessionURL, &pucOptInfo[1], pucOptInfo[0]);
		prBtmParam->ucSessionURLLen = pucOptInfo[0];
		u2TmpLen += pucOptInfo[0];
		pucOptInfo += pucOptInfo[0] + 1;
		eTransType = BSS_TRANSITION_DISASSOC;
	}
	if (ucRequestMode & BTM_REQ_MODE_CAND_INCLUDED_BIT) {
		if (!(ucRequestMode & BTM_REQ_MODE_ESS_DISC_IMM))
			eTransType = BSS_TRANSITION_REQ_ROAMING;
		if (prSwRfb->u2PacketLen > u2TmpLen) {
			prMsg->u2CandListLen = prSwRfb->u2PacketLen - u2TmpLen;
			prMsg->pucCandList = pucOptInfo;
			prMsg->ucValidityInterval = prRxFrame->ucValidityInterval;
		} else
			DBGLOG(WNM, WARN, "Candidate Include bit is set, but no candidate list\n");
	}

	DBGLOG(WNM, INFO, "BTM param: Req %d, VInt %d, DiscTimer %d, Token %d, TransType %d\n",
		   prBtmParam->ucRequestMode, prRxFrame->ucValidityInterval, prBtmParam->u2DisassocTimer,
		   prMsg->ucToken, eTransType);

	prMsg->eTransitionType = eTransType;
	prMsg->rMsgHdr.eMsgId = MID_WNM_AIS_BSS_TRANSITION;
	/* if Bss Transition Mgmt Request is dest for broadcast, don't send Btm Response */
	if (kalMemCmp(prRxFrame->aucDestAddr, "\xff\xff\xff\xff\xff\xff", MAC_ADDR_LEN))
		prMsg->fgNeedResponse = TRUE;
	else
		prMsg->fgNeedResponse = FALSE;
	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prMsg, MSG_SEND_METHOD_BUF);
}

