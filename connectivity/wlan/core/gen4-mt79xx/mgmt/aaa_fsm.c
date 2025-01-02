/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/aaa_fsm.c#3 $
 */

/*! \file   "aaa_fsm.c"
 *    \brief  This file defines the FSM for AAA MODULE.
 *
 *    This file defines the FSM for AAA MODULE.
 */

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "precomp.h"

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */
#if 0
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send Event to AIS/BOW/P2P
 *
 * @param[in] rJoinStatus        To indicate JOIN success or failure.
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] prSwRfb            Pointer to the SW_RFB_T

 * @return none
 */
/*----------------------------------------------------------------------------*/
uint32_t aaaFsmSendEventJoinComplete(uint32_t rJoinStatus,
		struct STA_RECORD *prStaRec, struct SW_RFB *prSwRfb)
{
	P_MSG_SAA_JOIN_COMP_T prJoinCompMsg;

	ASSERT(prStaRec);

	prJoinCompMsg = cnmMemAlloc(RAM_TYPE_TCM, sizeof(MSG_SAA_JOIN_COMP_T));
	if (!prJoinCompMsg)
		return WLAN_STATUS_RESOURCES;

	if (IS_STA_IN_AIS(prStaRec))
		prJoinCompMsg->rMsgHdr.eMsgId = MID_SAA_AIS_JOIN_COMPLETE;
	else if (IS_STA_IN_P2P(prStaRec))
		prJoinCompMsg->rMsgHdr.eMsgId = MID_SAA_P2P_JOIN_COMPLETE;
	else if (IS_STA_IN_BOW(prStaRec))
		prJoinCompMsg->rMsgHdr.eMsgId = MID_SAA_BOW_JOIN_COMPLETE;
	else
		ASSERT(0);

	prJoinCompMsg->rJoinStatus = rJoinStatus;
	prJoinCompMsg->prStaRec = prStaRec;
	prJoinCompMsg->prSwRfb = prSwRfb;

	mboxSendMsg(MBOX_ID_0,
		(struct MSG_HDR *) prJoinCompMsg,
		MSG_SEND_METHOD_BUF);

	return WLAN_STATUS_SUCCESS;

}				/* end of saaFsmSendEventJoinComplete() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Start Event to AAA FSM.
 *
 * @param[in] prMsgHdr   Message of Join Request for a particular STA.
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void aaaFsmRunEventStart(IN struct MSG_HDR *prMsgHdr)
{
	P_MSG_SAA_JOIN_REQ_T prJoinReqMsg;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prAisBssInfo;

	ASSERT(prMsgHdr);

	prJoinReqMsg = (P_MSG_SAA_JOIN_REQ_T) prMsgHdr;
	prStaRec = prJoinReqMsg->prStaRec;

	ASSERT(prStaRec);

	DBGLOG(SAA, LOUD, "EVENT-START: Trigger SAA FSM\n");

	cnmMemFree(prMsgHdr);

	/* 4 <1> Validation of SAA Start Event */
	if (!IS_AP_STA(prStaRec->eStaType)) {

		DBGLOG(SAA, ERROR,
			"EVENT-START: STA Type - %d was not supported.\n",
			prStaRec->eStaType);

		/* Ignore the return value because don't care the prSwRfb */
		saaFsmSendEventJoinComplete(WLAN_STATUS_FAILURE,
			prStaRec, NULL);

		return;
	}
	/* 4 <2> The previous JOIN process is not completed ? */
	if (prStaRec->eAuthAssocState != AA_STATE_IDLE) {
		DBGLOG(SAA, ERROR, "EVENT-START: Reentry of SAA Module.\n");
		prStaRec->eAuthAssocState = AA_STATE_IDLE;
	}
	/* 4 <3> Reset Status Code and Time */
	/* Update Station Record - Status/Reason Code */
	prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;

	/* Update the record join time. */
	GET_CURRENT_SYSTIME(&prStaRec->rLastJoinTime);

	prStaRec->ucTxAuthAssocRetryCount = 0;

	if (prStaRec->prChallengeText) {
		cnmMemFree(prStaRec->prChallengeText);
		prStaRec->prChallengeText = (struct IE_CHALLENGE_TEXT *) NULL;
	}

	cnmTimerStopTimer(&prStaRec->rTxReqDoneOrRxRespTimer);

	prStaRec->ucStaState = STA_STATE_1;

	/* Trigger SAA MODULE */
	saaFsmSteps(prStaRec, SAA_STATE_SEND_AUTH1, (struct SW_RFB *) NULL);
}				/* end of saaFsmRunEventStart() */
#endif

#if CFG_SUPPORT_AAA

void aaaFsmRunEventTxReqTimeOut(IN struct ADAPTER *prAdapter,
		IN unsigned long plParamPtr)
{
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) plParamPtr;
	struct BSS_INFO *prBssInfo;

	if (!prStaRec)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	DBGLOG(AAA, LOUD,
		"EVENT-TIMER: TX REQ TIMEOUT, Current Time = %d\n",
		kalGetTimeTick());

	/* Trigger statistics log if Auth/Assoc Tx timeout */
	wlanTriggerStatsLog(prAdapter, prAdapter->rWifiVar.u4StatsLogDuration);

	switch (prStaRec->eAuthAssocState) {
	case AAA_STATE_SEND_AUTH2:
		DBGLOG(AAA, ERROR,
			       "LOST EVENT ,Auth Tx done disappear timeout");

		prStaRec->eAuthAssocState = AA_STATE_IDLE;

		/* NOTE(Kevin): Change to STATE_1 */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

#if CFG_ENABLE_WIFI_DIRECT
			if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
				p2pRoleFsmRunEventAAATxFail(prAdapter,
					prStaRec, prBssInfo);
#endif /* CFG_ENABLE_WIFI_DIRECT */
		break;
#if 0
	/*state 2 to state 3 only check Assoc_req valid, no need for time out
	 *the fail case already handle at aaaFsmRunEventRxAssoc
	 */
	case AAA_STATE_SEND_ASSOC2:
		DBGLOG(AAA, ERROR,
			       "LOST EVENT ,Assoc Tx done disappear for (%d)Ms\n",
			TU_TO_MSEC(TX_AUTHENTICATION_RESPONSE_TIMEOUT_TU));


		prStaRec->eAuthAssocState = AAA_STATE_SEND_AUTH2;

		/* NOTE(Kevin): Change to STATE_2 */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);

#if CFG_ENABLE_WIFI_DIRECT
		if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
			p2pRoleFsmRunEventAAATxFail(prAdapter,
				prStaRec, prBssInfo);
#endif /* CFG_ENABLE_WIFI_DIRECT */
		break;
#endif

	default:
		return;
	}


}				/* end of saaFsmRunEventTxReqTimeOut() */





/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will process the Rx Auth Request Frame and then
 *        trigger AAA FSM.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void aaaFsmRunEventRxAuth(IN struct ADAPTER *prAdapter,
		IN struct SW_RFB *prSwRfb)
{
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;
	uint16_t u2StatusCode;
	u_int8_t fgReplyAuth = FALSE;
	struct WLAN_AUTH_FRAME *prAuthFrame = (struct WLAN_AUTH_FRAME *) NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	ASSERT(prAdapter);

	do {
		prAuthFrame = (struct WLAN_AUTH_FRAME *) prSwRfb->pvHeader;

		DBGLOG(AAA, INFO,
			"SA: " MACSTR ", bssid: " MACSTR ", %d %d sta: %d\n",
			MAC2STR(prAuthFrame->aucSrcAddr),
			MAC2STR(prAuthFrame->aucBSSID),
#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
			prAuthFrame->aucAuthData[0],
#else
			prAuthFrame->u2AuthTransSeqNo,
#endif
			prAuthFrame->u2AuthAlgNum,
			prSwRfb->ucStaRecIdx);

#if CFG_ENABLE_WIFI_DIRECT
		prBssInfo = p2pFuncBSSIDFindBssInfo(prAdapter,
			prAuthFrame->aucBSSID);

		/* 4 <1> Check P2P network conditions */

		/* if (prBssInfo && prAdapter->fgIsP2PRegistered) */
		/* modify coding sytle to reduce indent */

		if (!prAdapter->fgIsP2PRegistered)
			goto bow_proc;

		if (prBssInfo && prBssInfo->fgIsNetActive) {

			/* 4 <1.1> Validate Auth Frame
			 * by Auth Algorithm/Transation Seq
			 */
			if (WLAN_STATUS_SUCCESS ==
				authProcessRxAuthFrame(prAdapter,
					prSwRfb,
					prBssInfo,
					&u2StatusCode)) {

				if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
					DBGLOG(AAA, TRACE,
						"process RxAuth status success\n");
					/* 4 <1.2> Validate Auth Frame
					 * for Network Specific Conditions
					 */
					fgReplyAuth = p2pFuncValidateAuth(
						prAdapter,
						prBssInfo,
						prSwRfb,
						&prStaRec,
						&u2StatusCode);

#if CFG_SUPPORT_802_11W
					/* AP PMF, if PMF connection,
					 * ignore Rx auth
					 */
					/* Certification 4.3.3.4 */
					if (rsnCheckBipKeyInstalled(prAdapter,
						prStaRec)) {
						DBGLOG(AAA, INFO,
							"Drop RxAuth\n");
						return;
					}
#endif
				} else if (authFloodingCheck(
						prAdapter,
						prBssInfo,
						prSwRfb) == FALSE) {
					return;
				} else {
					fgReplyAuth = TRUE;
				}
				break;
			}
		}
#endif /* CFG_ENABLE_WIFI_DIRECT */

bow_proc:

		/* 4 <2> Check BOW network conditions */
#if CFG_ENABLE_BT_OVER_WIFI
		{
			struct BOW_FSM_INFO *prBowFsmInfo =
				(struct BOW_FSM_INFO *) NULL;

			prBowFsmInfo = &(prAdapter->rWifiVar.rBowFsmInfo);
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
				prBowFsmInfo->ucBssIndex);

			if ((prBssInfo->fgIsNetActive)
				&& (prBssInfo->eCurrentOPMode == OP_MODE_BOW)) {

				/* 4 <2.1> Validate Auth Frame
				 * by Auth Algorithm/Transation Seq
				 */
				/* Check if for this BSSID */
				if (WLAN_STATUS_SUCCESS ==
				    authProcessRxAuth1Frame(prAdapter,
					    prSwRfb,
					    prBssInfo->aucBSSID,
					    AUTH_ALGORITHM_NUM_OPEN_SYSTEM,
					    AUTH_TRANSACTION_SEQ_1,
					    &u2StatusCode)) {

					if (u2StatusCode
						== STATUS_CODE_SUCCESSFUL) {

						/* 4 <2.2> Validate Auth Frame
						 * for Network Specific
						 * Conditions
						 */
						fgReplyAuth =
						    bowValidateAuth(prAdapter,
							prSwRfb,
							&prStaRec,
							&u2StatusCode);

					} else {

						fgReplyAuth = TRUE;
					}
					/* TODO(Kevin): Allocate a STA_RECORD_T
					 * for new client
					 */
					break;
				}
			}
		}
#endif /* CFG_ENABLE_BT_OVER_WIFI */

		return;
	} while (FALSE);

	if (prStaRec) {
		/* update RCPI */
		ASSERT(prSwRfb->prRxStatusGroup3);
		prStaRec->ucRCPI = nicRxGetRcpiValueFromRxv(
			prAdapter,
			RCPI_MODE_MAX,
			prSwRfb);
	}
	/* 4 <3> Update STA_RECORD_T and
	 * reply Auth_2(Response to Auth_1) Frame
	 */
	if (fgReplyAuth) {

		if (prStaRec) {

			if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
				if (prStaRec->eAuthAssocState
					!= AA_STATE_IDLE) {

					DBGLOG(AAA, WARN,
						"Previous AuthAssocState (%d) != IDLE.\n",
						prStaRec->eAuthAssocState);
				}

				prStaRec->eAuthAssocState =
					AAA_STATE_SEND_AUTH2;
			} else {
				prStaRec->eAuthAssocState = AA_STATE_IDLE;

				/* NOTE(Kevin): Change to STATE_1 */
				cnmStaRecChangeState(prAdapter,
					prStaRec, STA_STATE_1);
			}

			/* Update the record join time. */
			GET_CURRENT_SYSTIME(&prStaRec->rUpdateTime);

			/* Update Station Record - Status/Reason Code */
			prStaRec->u2StatusCode = u2StatusCode;

			prStaRec->ucAuthAlgNum = prAuthFrame->u2AuthAlgNum;
		} else {
			/* NOTE(Kevin): We should have STA_RECORD_T
			 * if the status code was successful
			 */
			ASSERT(!(u2StatusCode == STATUS_CODE_SUCCESSFUL));
			return;
		}

		if (prBssInfo->u4RsnSelectedAKMSuite ==
			RSN_AKM_SUITE_SAE) {
			kalP2PIndicateRxMgmtFrame(
				prAdapter,
				prAdapter->prGlueInfo,
				prSwRfb,
				FALSE,
				(uint8_t)prBssInfo->u4PrivateData);
			DBGLOG(AAA, INFO, "Forward RxAuth\n");
			if (prStaRec && prStaRec->fgIsInUse) {
				cnmTimerStopTimer(prAdapter,
					&prStaRec->rTxReqDoneOrRxRespTimer);
				/*ToDo:Init Timer to check get
				 * Auth Txdone avoid sta_rec not clear
				 */
				cnmTimerInitTimer(prAdapter,
					&prStaRec->rTxReqDoneOrRxRespTimer,
					(PFN_MGMT_TIMEOUT_FUNC)
					aaaFsmRunEventTxReqTimeOut,
					(unsigned long) prStaRec);

				cnmTimerStartTimer(prAdapter,
					&prStaRec->rTxReqDoneOrRxRespTimer,
					TU_TO_MSEC(
					DOT11_RSNA_SAE_RETRANS_PERIOD_TU));
			}
			return;
		}

		/* NOTE: Ignore the return status for AAA */
		/* 4 <4> Reply  Auth */
		rStatus = authSendAuthFrame(prAdapter,
					prStaRec,
					prBssInfo->ucBssIndex,
					prSwRfb,
					AUTH_TRANSACTION_SEQ_2,
					u2StatusCode);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(AAA, WARN, "Send Auth Fail!\n");
			return;
		}


		/*sta_rec might be removed
		 * when client list full, skip timer setting
		 */
		/*
		 * check if prStaRec valid as authSendAuthFrame may free
		 * StaRec when TX resource is not enough
		 */
		if (prStaRec && prStaRec->fgIsInUse) {
			cnmTimerStopTimer(prAdapter,
				&prStaRec->rTxReqDoneOrRxRespTimer);
			/*ToDo:Init Timer to check get
			 * Auth Txdone avoid sta_rec not clear
			 */
			cnmTimerInitTimer(prAdapter,
				&prStaRec->rTxReqDoneOrRxRespTimer,
				(PFN_MGMT_TIMEOUT_FUNC)
				aaaFsmRunEventTxReqTimeOut,
				(unsigned long) prStaRec);

			cnmTimerStartTimer(prAdapter,
				&prStaRec->rTxReqDoneOrRxRespTimer,
				TU_TO_MSEC(
					TX_AUTHENTICATION_RESPONSE_TIMEOUT_TU));
		}



	} else if (prStaRec)
		cnmStaRecFree(prAdapter, prStaRec);
}				/* end of aaaFsmRunEventRxAuth() */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will process
 *    the Rx (Re)Association Request Frame and then
 *    trigger AAA FSM.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS           Always return success
 */
/*---------------------------------------------------------------------------*/
uint32_t aaaFsmRunEventRxAssoc(IN struct ADAPTER *prAdapter,
		IN struct SW_RFB *prSwRfb)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;
	uint16_t u2StatusCode = STATUS_CODE_RESERVED;
	u_int8_t fgReplyAssocResp = FALSE;
	u_int8_t fgSendSAQ = FALSE;
	struct WLAN_ASSOC_REQ_FRAME *prAssocReqFrame =
			(struct WLAN_ASSOC_REQ_FRAME *) NULL;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	do {
		prAssocReqFrame =
			(struct WLAN_ASSOC_REQ_FRAME *) prSwRfb->pvHeader;

		/* 4 <1> Check if we have the STA_RECORD_T
		 * for incoming Assoc Req
		 */
		prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);

		/* No Wtbl handling */
		if (!prStaRec) {
			secHandleNoWtbl(prAdapter, prSwRfb);
			prStaRec = prSwRfb->prStaRec;
		}

		/* We should have the corresponding Sta Record. */
		if ((!prStaRec) || (!prStaRec->fgIsInUse)) {
			/* Not to reply association response
			 * with failure code due to lack of STA_REC
			 */
			DBGLOG(AAA, TRACE,
				"get sta fail, wlan idx: %d, sta index: %d\n",
				prSwRfb->ucWlanIdx,
				prSwRfb->ucStaRecIdx);
			break;
		}

		DBGLOG(AAA, INFO,
			"SA: " MACSTR ", bssid: " MACSTR ", sta idx: %d\n",
			MAC2STR(prAssocReqFrame->aucSrcAddr),
			MAC2STR(prAssocReqFrame->aucBSSID),
			prSwRfb->ucStaRecIdx);

		if (!IS_CLIENT_STA(prStaRec)) {
			DBGLOG(AAA, ERROR,
				"error sta type, skip process rx assoc\n");
			cnmDumpStaRec(prAdapter, prSwRfb->ucStaRecIdx);
			break;
		}

		DBGLOG(AAA, TRACE,
			"RxAssoc enter ucStaState:%d, eAuthassocState:%d\n",
			prStaRec->ucStaState, prStaRec->eAuthAssocState);

		if (prStaRec->ucStaState == STA_STATE_3) {
			/* Do Reassocation */
		} else if ((prStaRec->ucStaState == STA_STATE_2) &&
			(prStaRec->eAuthAssocState == AAA_STATE_SEND_AUTH2)) {
			/* Normal case */
		} else {
			DBGLOG(AAA, WARN,
				"Previous AuthAssocState (%d) != SEND_AUTH2.\n",
				prStaRec->eAuthAssocState);

			/* Maybe Auth Response TX fail,
			 * but actually it success.
			 */
			cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);
		}

		/* update RCPI */
		ASSERT(prSwRfb->prRxStatusGroup3);
		prStaRec->ucRCPI =
			nicRxGetRcpiValueFromRxv(
				prAdapter, RCPI_MODE_MAX, prSwRfb);

		/* 4 <2> Check P2P network conditions */
#if CFG_ENABLE_WIFI_DIRECT
		if ((prAdapter->fgIsP2PRegistered)
			&& (IS_STA_IN_P2P(prStaRec))) {

			prBssInfo =
				GET_BSS_INFO_BY_INDEX(prAdapter,
					prStaRec->ucBssIndex);

			if (prBssInfo->fgIsNetActive) {

				/* 4 <2.1> Validate Assoc Req Frame and
				 * get Status Code
				 */
				/* Check if for this BSSID */
				if (WLAN_STATUS_SUCCESS ==
				    assocProcessRxAssocReqFrame(prAdapter,
						prSwRfb, &u2StatusCode)) {

					if (u2StatusCode
						== STATUS_CODE_SUCCESSFUL) {
						/* 4 <2.2>
						 * Validate Assoc Req Frame
						 * for Network Specific
						 * Conditions
						 */
						fgReplyAssocResp =
						    p2pFuncValidateAssocReq(
								prAdapter,
								prSwRfb,
								(uint16_t *)
								&u2StatusCode);
					} else {
						fgReplyAssocResp = TRUE;
					}

					break;
				}
			}
		}
#endif /* CFG_ENABLE_WIFI_DIRECT */

		/* 4 <3> Check BOW network conditions */
#if CFG_ENABLE_BT_OVER_WIFI
		if (IS_STA_BOW_TYPE(prStaRec)) {

			prBssInfo =
				GET_BSS_INFO_BY_INDEX(prAdapter,
					prStaRec->ucBssIndex);

			if ((prBssInfo->fgIsNetActive)
				&& (prBssInfo->eCurrentOPMode == OP_MODE_BOW)) {

				/* 4 <3.1> Validate Auth Frame
				 * by Auth Algorithm/Transation Seq
				 */
				/* Check if for this BSSID */
				if (WLAN_STATUS_SUCCESS ==
				    assocProcessRxAssocReqFrame(prAdapter,
					prSwRfb, &u2StatusCode)) {

					if (u2StatusCode
						== STATUS_CODE_SUCCESSFUL) {

						/* 4 <3.2>
						 * Validate Auth Frame for
						 * Network Specific Conditions
						 */
						fgReplyAssocResp =
						    bowValidateAssocReq(
								prAdapter,
								prSwRfb,
								&u2StatusCode);
					} else {

						fgReplyAssocResp = TRUE;
					}

					/* TODO(Kevin):
					 * Allocate a STA_RECORD_T
					 * for new client
					 */
					break;
				}
			}
		}
#endif /* CFG_ENABLE_BT_OVER_WIFI */

		return WLAN_STATUS_SUCCESS;	/* To release the SW_RFB_T */
	} while (FALSE);

	/* 4 <4> Update STA_RECORD_T and reply Assoc Resp Frame */
	if (fgReplyAssocResp) {
		uint16_t u2IELength;
		uint8_t *pucIE;

		cnmTimerStopTimer(prAdapter,
			&prStaRec->rTxReqDoneOrRxRespTimer);

		if ((((struct WLAN_ASSOC_REQ_FRAME *)
			(prSwRfb->pvHeader))->u2FrameCtrl & MASK_FRAME_TYPE) ==
		    MAC_FRAME_REASSOC_REQ) {

			u2IELength = prSwRfb->u2PacketLen -
			    (uint16_t)
			    OFFSET_OF(struct WLAN_REASSOC_REQ_FRAME,
			    aucInfoElem[0]);

			pucIE = ((struct WLAN_REASSOC_REQ_FRAME *)
				(prSwRfb->pvHeader))->aucInfoElem;
		} else {
			u2IELength = prSwRfb->u2PacketLen -
				(uint16_t)
				OFFSET_OF(struct WLAN_ASSOC_REQ_FRAME,
				aucInfoElem[0]);

			pucIE = ((struct WLAN_ASSOC_REQ_FRAME *)
				(prSwRfb->pvHeader))->aucInfoElem;
		}

		rlmProcessAssocReq(prAdapter, prSwRfb, pucIE, u2IELength);

		/* 4 <4.1> Assign Association ID */
		if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {

#if CFG_ENABLE_WIFI_DIRECT
			if ((prAdapter->fgIsP2PRegistered)
				&& (IS_STA_IN_P2P(prStaRec))) {
				prBssInfo =
					GET_BSS_INFO_BY_INDEX(prAdapter,
						prStaRec->ucBssIndex);
				if (p2pRoleFsmRunEventAAAComplete(prAdapter,
					prStaRec, prBssInfo)
					== WLAN_STATUS_SUCCESS) {
					prStaRec->u2AssocId =
						bssAssignAssocID(prStaRec);
					/* prStaRec->eAuthAssocState
					 * = AA_STATE_IDLE;
					 */
					/* NOTE(Kevin): for TX done */
					prStaRec->eAuthAssocState =
						AAA_STATE_SEND_ASSOC2;
					/* NOTE(Kevin):
					 * Method A:
					 * Change to STATE_3
					 * before handle TX Done
					 */
					/* cnmStaRecChangeState(prAdapter,
					 * prStaRec, STA_STATE_3);
					 */
				} else {
					/* Client List FULL. */
					u2StatusCode = STATUS_CODE_REQ_DECLINED;

					/* Invalid Association ID */
					prStaRec->u2AssocId = 0;

					/* If(Re)association fail,
					 * remove sta record and
					 * use class error to handle sta
					 */
					prStaRec->eAuthAssocState =
						AA_STATE_IDLE;

					/* NOTE(Kevin):
					 * Better to change state here,
					 * not at TX Done
					 */
					cnmStaRecChangeState(prAdapter,
						prStaRec, STA_STATE_2);
				}
			}
#endif

#if CFG_ENABLE_BT_OVER_WIFI
			if ((IS_STA_BOW_TYPE(prStaRec))) {
				/* if (bowRunEventAAAComplete(prAdapter,
				 * prStaRec) == WLAN_STATUS_SUCCESS) {
				 */
				prStaRec->u2AssocId =
					bssAssignAssocID(prStaRec);

				/* NOTE(Kevin): for TX done */
				prStaRec->eAuthAssocState =
					AAA_STATE_SEND_ASSOC2;

				/* NOTE(Kevin):
				 * Method A: Change to STATE_3
				 * before handle TX Done
				 */
				/* cnmStaRecChangeState(prAdapter,
				 * prStaRec, STA_STATE_3);
				 */
			}
#endif
		} else {

#if CFG_SUPPORT_802_11W
			/* AP PMF */
			/* don't change state,
			 * just send assoc resp
			 * (NO need TX done, TIE + code30)
			 * and then SAQ
			 */
			if (u2StatusCode
				== STATUS_CODE_ASSOC_REJECTED_TEMPORARILY) {
				DBGLOG(AAA, INFO, "AP send SAQ\n");
				fgSendSAQ = TRUE;
			} else
#endif
			{
				/* Invalid Association ID */
				prStaRec->u2AssocId = 0;

				/* If (Re)association fail, remove sta record
				 * and use class error to handle sta
				 */
				prStaRec->eAuthAssocState = AA_STATE_IDLE;
				/* Remove from client list if it was previously
				 * associated
				 */
				if ((prStaRec->ucStaState > STA_STATE_1) &&
				     prAdapter->fgIsP2PRegistered &&
				     (IS_STA_IN_P2P(prStaRec))) {
					struct BSS_INFO *prBssInfo = NULL;

					prBssInfo = GET_BSS_INFO_BY_INDEX(
					    prAdapter,
					    prStaRec->ucBssIndex);
					if (prBssInfo) {
						DBGLOG(AAA, INFO,
						    "Remove client\n");
						bssRemoveClient(
						    prAdapter,
						    prBssInfo,
						    prStaRec);
					}
				}

				/* NOTE(Kevin):
				 * Better to change state here, not at TX Done
				 */
				cnmStaRecChangeState(prAdapter,
					prStaRec, STA_STATE_2);
			}
		}

		/* Update the record join time. */
		GET_CURRENT_SYSTIME(&prStaRec->rUpdateTime);

		/* Update Station Record - Status/Reason Code */
		prStaRec->u2StatusCode = u2StatusCode;

		/* NOTE: Ignore the return status for AAA */
		/* 4 <4.2> Reply  Assoc Resp */
		assocSendReAssocRespFrame(prAdapter, prStaRec);

#if CFG_SUPPORT_802_11W
		/* AP PMF */
		if (fgSendSAQ) {
			/* if PMF connection, and return code 30, send SAQ */
			rsnApStartSaQuery(prAdapter, prStaRec);
		}
#endif

	}

	return WLAN_STATUS_SUCCESS;

}				/* end of aaaFsmRunEventRxAssoc() */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will handle TxDone(Auth2/AssocReq) Event of AAA FSM.
 *
 * @param[in] prAdapter      Pointer to the Adapter structure.
 * @param[in] prMsduInfo     Pointer to the MSDU_INFO_T.
 * @param[in] rTxDoneStatus  Return TX status of the Auth1/Auth3/AssocReq frame.
 *
 * @retval WLAN_STATUS_SUCCESS
 */
/*---------------------------------------------------------------------------*/
uint32_t
aaaFsmRunEventTxDone(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo,
		IN enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);


	DBGLOG(AAA, INFO,
		"EVENT-TX DONE [status: %d][seq: %d]: Current Time = %d\n",
		rTxDoneStatus,
		prMsduInfo->ucTxSeqNum,
		kalGetTimeTick());

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	/* For the case of replying ERROR STATUS CODE */
	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return WLAN_STATUS_SUCCESS;

	ASSERT(prStaRec->ucBssIndex <= prAdapter->ucHwBssIdNum);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	DBGLOG(AAA, TRACE, "TxDone ucStaState:%d, eAuthAssocState:%d\n",
		prStaRec->ucStaState, prStaRec->eAuthAssocState);

	/* Trigger statistics log if Auth/Assoc Tx failed */
	if (rTxDoneStatus != TX_RESULT_SUCCESS)
		wlanTriggerStatsLog(prAdapter,
			prAdapter->rWifiVar.u4StatsLogDuration);

	switch (prStaRec->eAuthAssocState) {
	case AAA_STATE_SEND_AUTH2:
		/* Strictly check the outgoing frame is matched
		 * with current AA STATE
		 */
		if (authCheckTxAuthFrame(prAdapter,
			prMsduInfo, AUTH_TRANSACTION_SEQ_2)
			!= WLAN_STATUS_SUCCESS)
			break;

		cnmTimerStopTimer(prAdapter,
			&prStaRec->rTxReqDoneOrRxRespTimer);

		if (prStaRec->u2StatusCode == STATUS_CODE_SUCCESSFUL) {
			if (rTxDoneStatus == TX_RESULT_SUCCESS) {

				/* NOTE(Kevin):
				 * Change to STATE_2 at TX Done
				 */
				cnmStaRecChangeState(prAdapter,
					prStaRec, STA_STATE_2);
				/* Error handle if can not
				 * complete the ASSOC flow
				 */
				cnmTimerStartTimer(prAdapter,
					&prStaRec->rTxReqDoneOrRxRespTimer,
					TU_TO_MSEC(TX_ASSOCIATE_TIMEOUT_TU));
			} else {

				prStaRec->eAuthAssocState =
					AA_STATE_IDLE;

				/* NOTE(Kevin): Change to STATE_1 */
				cnmStaRecChangeState(prAdapter,
					prStaRec, STA_STATE_1);

#if CFG_ENABLE_WIFI_DIRECT
				if (prBssInfo->eNetworkType
					== NETWORK_TYPE_P2P)
					p2pRoleFsmRunEventAAATxFail(
						prAdapter,
						prStaRec, prBssInfo);
#endif /* CFG_ENABLE_WIFI_DIRECT */
#if CFG_ENABLE_BT_OVER_WIFI
				if (IS_STA_BOW_TYPE(prStaRec))
					bowRunEventAAATxFail(prAdapter,
						prStaRec);

#endif /* CFG_ENABLE_BT_OVER_WIFI */
#if CFG_AP_80211KVR_INTERFACE
				aaaMulAPAgentStaEventNotify(prStaRec,
					prBssInfo->aucBSSID,
					prStaRec->fgIsInUse);
#endif /* CFG_AP_80211KVR_INTERFACE */
			}

		}
		/* NOTE(Kevin): Ignore the TX Done Event of
		 * Auth Frame with Error Status Code
		 */

		break;

	case AAA_STATE_SEND_ASSOC2:
		{
			/* Strictly check the outgoing frame is matched
			 * with current SAA STATE
			 */
			if (assocCheckTxReAssocRespFrame(prAdapter,
				prMsduInfo) != WLAN_STATUS_SUCCESS)
				break;

			if (prStaRec->u2StatusCode == STATUS_CODE_SUCCESSFUL) {
				if (rTxDoneStatus == TX_RESULT_SUCCESS) {

					prStaRec->eAuthAssocState =
						AA_STATE_IDLE;

					/* NOTE(Kevin):
					 * Change to STATE_3 at TX Done
					 */
#if CFG_ENABLE_WIFI_DIRECT
					if (prBssInfo->eNetworkType
						== NETWORK_TYPE_P2P)
						p2pRoleFsmRunEventAAASuccess(
							prAdapter,
							prStaRec,
							prBssInfo);
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if CFG_ENABLE_BT_OVER_WIFI

					if (IS_STA_BOW_TYPE(prStaRec))
						bowRunEventAAAComplete(
							prAdapter,
							prStaRec);

#endif /* CFG_ENABLE_BT_OVER_WIFI */

				} else {

					prStaRec->eAuthAssocState =
						AAA_STATE_SEND_AUTH2;

					/* NOTE(Kevin): Change to STATE_2 */
					cnmStaRecChangeState(prAdapter,
						prStaRec, STA_STATE_2);

#if CFG_ENABLE_WIFI_DIRECT
					if (prBssInfo->eNetworkType
						== NETWORK_TYPE_P2P)
						p2pRoleFsmRunEventAAATxFail(
							prAdapter,
							prStaRec,
							prBssInfo);
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if CFG_ENABLE_BT_OVER_WIFI
					if (IS_STA_BOW_TYPE(prStaRec))
						bowRunEventAAATxFail(prAdapter,
							prStaRec);

#endif /* CFG_ENABLE_BT_OVER_WIFI */

				}
			}
			/* NOTE(Kevin): Ignore the TX Done Event of
			 * Auth Frame with Error Status Code
			 */
		}
		break;

	case AA_STATE_IDLE:
		/* 2013-08-27 frog:  Do nothing.
		 * Somtimes we may send Assoc Resp twice.
		 * (Rx Assoc Req before the first Assoc TX Done)
		 * The AssocState is changed to IDLE after first TX done.
		 * Free station record when IDLE is seriously wrong.
		 */
		/* 2017-01-12 Do nothing only when STA is in state 3 */
		/* Free the StaRec if found any unexpected status */
		if (prStaRec->ucStaState != STA_STATE_3)
			cnmStaRecFree(prAdapter, prStaRec);
		break;

	default:
		break;		/* Ignore other cases */
	}

	DBGLOG(AAA, TRACE, "TxDone end ucStaState:%d, eAuthAssocState:%d\n",
		prStaRec->ucStaState, prStaRec->eAuthAssocState);

	return WLAN_STATUS_SUCCESS;

}				/* end of aaaFsmRunEventTxDone() */

#if CFG_AP_80211KVR_INTERFACE
#if CFG_SUPPORT_TRAFFIC_REPORT && CFG_WIFI_SUPPORT_NOISE_HISTOGRAM
uint32_t aaaMulAPAgentChanNoiseControl(
	struct GLUE_INFO *prGlueInfo,
	bool fgIsChanNoiseEnable)
{
	uint8_t ucBand = ENUM_BAND_0;
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	struct CMD_RLM_AIRTIME_MON *cmd_traffic = NULL;
	struct CMD_NOISE_HISTOGRAM_REPORT *cmd_noise = NULL;

	/* traffic report and noise histogram */
	cmd_traffic =
		(struct CMD_RLM_AIRTIME_MON *)
		kalMemAlloc(sizeof(*cmd_traffic), VIR_MEM_TYPE);
	if (!cmd_traffic)
		goto error;
	cmd_noise =
		(struct CMD_NOISE_HISTOGRAM_REPORT *)
		kalMemAlloc(sizeof(*cmd_noise), VIR_MEM_TYPE);
	if (!cmd_noise)
		goto error;

	/* set traffic report enable/disable */
	memset(cmd_traffic, 0, sizeof(*cmd_traffic));
	cmd_traffic->u2Type = CMD_GET_REPORT_TYPE;
	cmd_traffic->u2Len = sizeof(*cmd_traffic);
	cmd_traffic->ucBand = ucBand;

	if (fgIsChanNoiseEnable) {
		prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap |=
			KEEP_FULL_PWR_TRAFFIC_REPORT_BIT;
		cmd_traffic->ucAction = CMD_GET_REPORT_ENABLE;
	} else {
		prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap &=
			~KEEP_FULL_PWR_TRAFFIC_REPORT_BIT;
		cmd_traffic->ucAction = CMD_GET_REPORT_DISABLE;
	}
	cmd_traffic->u2Type |= CMD_ADV_CONTROL_SET;

	rStatus = kalIoctl(prGlueInfo,
				wlanoidAdvCtrl,
				cmd_traffic,
				sizeof(*cmd_traffic),
				TRUE, TRUE, TRUE,
				&u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%x)\n", rStatus);
		goto error;
	}

	/* set noise histogram enable/disable */
	memset(cmd_noise, 0, sizeof(*cmd_noise));
	cmd_noise->u2Type = CMD_NOISE_HISTOGRAM_TYPE;
	cmd_noise->u2Len = sizeof(*cmd_noise);

	if (fgIsChanNoiseEnable) {
		prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap |=
			KEEP_FULL_PWR_NOISE_HISTOGRAM_BIT;
		cmd_noise->ucAction = CMD_NOISE_HISTOGRAM_ENABLE;
	} else {
		prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap &=
			~KEEP_FULL_PWR_NOISE_HISTOGRAM_BIT;
		cmd_noise->ucAction = CMD_NOISE_HISTOGRAM_DISABLE;
	}
	cmd_noise->u2Type |= CMD_ADV_CONTROL_SET;

	rStatus = kalIoctl(prGlueInfo,
				wlanoidAdvCtrl,
				cmd_noise,
				sizeof(*cmd_noise),
				TRUE, TRUE, TRUE,
				&u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%x)\n", rStatus);
		goto error;
	}

	rStatus = WLAN_STATUS_SUCCESS;
error:
	if (cmd_traffic)
		kalMemFree(cmd_traffic, VIR_MEM_TYPE, sizeof(*cmd_traffic));
	if (cmd_noise)
		kalMemFree(cmd_noise, VIR_MEM_TYPE, sizeof(*cmd_noise));

	return rStatus;

}

void aaaMulAPAgentChanNoiseInitWorkHandler(
	struct work_struct *work)
{
	struct GLUE_INFO *prGlueInfo;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	prGlueInfo = ENTRY_OF(work, struct GLUE_INFO, rChanNoiseControlWork);

	/* Disable traffic report and noise histogram */
	rStatus = aaaMulAPAgentChanNoiseControl(prGlueInfo, FALSE);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(AAA, ERROR,
			"Disable traffic report and nose histogram fail\n");
		goto error;
	}

	/* Enable traffic report and noise histogram */
	rStatus = aaaMulAPAgentChanNoiseControl(prGlueInfo, TRUE);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(AAA, ERROR,
			"Enable traffic report and nose histogram fail\n");
		goto error;
	}

error:
	if (!rStatus)
		schedule_delayed_work(&prGlueInfo->rChanNoiseGetInfoWork,
			MSEC_TO_JIFFIES(SAP_CHAN_NOISE_GET_INFO_PERIOD));
	else
		schedule_delayed_work(&prGlueInfo->rChanNoiseControlWork, 0);
}

void  aaaMulAPAgentChanNoiseCollectionWorkHandler(
	struct work_struct *work)
{
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4BufLen = 0;
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	uint32_t u4SampleDuration = 0;
	uint32_t u4NoiseTotalCnt = 0;
	uint8_t ucBand = ENUM_BAND_0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct BSS_INFO *prBssInfo = NULL;
	struct CMD_RLM_AIRTIME_MON *cmd_traffic = NULL;
	struct CMD_NOISE_HISTOGRAM_REPORT *cmd_noise = NULL;
	struct T_MULTI_AP_BSS_METRICS_RESP *sBssMetricsResp = NULL;
	int32_t i4Ret = 0;

	prGlueInfo = ENTRY_OF(work, struct GLUE_INFO, rChanNoiseGetInfoWork);

	cmd_traffic = (struct CMD_RLM_AIRTIME_MON *)
		kalMemAlloc(sizeof(*cmd_traffic), VIR_MEM_TYPE);
	if (!cmd_traffic)
		goto error;
	cmd_noise = (struct CMD_NOISE_HISTOGRAM_REPORT *)
		kalMemAlloc(sizeof(*cmd_noise), VIR_MEM_TYPE);
	if (!cmd_noise)
		goto error;

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo,
			prGlueInfo->prP2PInfo[1]->prDevHandler,
			&ucRoleIdx) != 0)
		goto error;
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
			ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS)
		goto error;
	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		goto error;
	}

	/* get traffic report */
	memset(cmd_traffic, 0, sizeof(*cmd_traffic));
	cmd_traffic->u2Type = CMD_GET_REPORT_TYPE;
	cmd_traffic->u2Len = sizeof(*cmd_traffic);
	cmd_traffic->ucBand = ucBand;
	cmd_traffic->ucAction = CMD_GET_REPORT_GET;

	rStatus = kalIoctl(prGlueInfo,
		wlanoidAdvCtrl,
		cmd_traffic,
		sizeof(*cmd_traffic),
		TRUE, TRUE, TRUE,
		&u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%x)\n", rStatus);
		goto error;
	}

	u4SampleDuration = cmd_traffic->u4FetchEd - cmd_traffic->u4FetchSt;
	if (!u4SampleDuration)
		u4SampleDuration = 255;

	prBssInfo->u4ChanUtil = cmd_traffic->u4ChBusy
		/ (u4SampleDuration / 255);

	DBGLOG(AAA, INFO,
		"[Traffic Report] u4ChBusy = %d\n", cmd_traffic->u4ChBusy);
	DBGLOG(AAA, INFO,
		"[Traffic Report] Duration = %d\n",
		cmd_traffic->u4FetchEd - cmd_traffic->u4FetchSt);

	/* get noise histogram */
	memset(cmd_noise, 0, sizeof(*cmd_noise));
	cmd_noise->u2Type = CMD_NOISE_HISTOGRAM_TYPE;
	cmd_noise->u2Len = sizeof(*cmd_noise);
	cmd_noise->ucAction = CMD_NOISE_HISTOGRAM_GET;

	rStatus = kalIoctl(prGlueInfo,
		wlanoidAdvCtrl,
		cmd_noise,
		sizeof(*cmd_noise),
		TRUE, TRUE, TRUE,
		&u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%x)\n", rStatus);
		goto error;
	}

	u4NoiseTotalCnt =
		cmd_noise->u4IPI0 + cmd_noise->u4IPI1
		+ cmd_noise->u4IPI2 + cmd_noise->u4IPI3
		+ cmd_noise->u4IPI4 + cmd_noise->u4IPI5
		+ cmd_noise->u4IPI6 + cmd_noise->u4IPI7
		+ cmd_noise->u4IPI8 + cmd_noise->u4IPI9
		+ cmd_noise->u4IPI10;
	if (!u4NoiseTotalCnt)
		u4NoiseTotalCnt = 1;

	prBssInfo->i4NoiseHistogram =
		((cmd_noise->u4IPI0 * (95) + cmd_noise->u4IPI1 * (90) +
		cmd_noise->u4IPI2 * (87) + cmd_noise->u4IPI3 * (84) +
		cmd_noise->u4IPI4 * (81) + cmd_noise->u4IPI5 * (77) +
		cmd_noise->u4IPI6 * (72) + cmd_noise->u4IPI7 * (67) +
		cmd_noise->u4IPI8 * (62) + cmd_noise->u4IPI9 * (57) +
		cmd_noise->u4IPI10 * (55)) / u4NoiseTotalCnt) * (-1);

	DBGLOG(AAA, INFO,
		"[Noise Histogram] u4NoiseTotalCnt  = %d\n", u4NoiseTotalCnt);
	DBGLOG(AAA, INFO,
		"[Noise Histogram] i4NoiseHistogram  = %d\n",
		prBssInfo->i4NoiseHistogram);

	sBssMetricsResp = (struct T_MULTI_AP_BSS_METRICS_RESP *)
			kalMemAlloc(sizeof(struct T_MULTI_AP_BSS_METRICS_RESP),
			VIR_MEM_TYPE);
	if (sBssMetricsResp == NULL) {
		DBGLOG(INIT, ERROR, "alloc memory fail\n");
		goto error;
	}

	/* 1. BSS Measurement */
	/* Interface Index */
	i4Ret = sscanf(prGlueInfo->prP2PInfo[1]->prDevHandler->name,
		"ap%u", &sBssMetricsResp->uIfIndex);
	if (i4Ret != 1)
		DBGLOG(P2P, WARN, "read sap index fail: %d\n", i4Ret);

	COPY_MAC_ADDR(sBssMetricsResp->mBssid, prBssInfo->aucBSSID);
	sBssMetricsResp->u8Channel = prBssInfo->ucPrimaryChannel;
	sBssMetricsResp->u16AssocStaNum =
		bssGetClientCount(prGlueInfo->prAdapter, prBssInfo);
	sBssMetricsResp->u8ChanUtil = prBssInfo->u4ChanUtil;
	sBssMetricsResp->iChanNoise = prBssInfo->i4NoiseHistogram;

	DBGLOG(REQ, INFO,
		"[SAP_Test] uIfIndex = %u\n", sBssMetricsResp->uIfIndex);
	DBGLOG(REQ, INFO,
		"[SAP_Test] mBssid = " MACSTR "\n",
		MAC2STR(sBssMetricsResp->mBssid));
	DBGLOG(REQ, INFO,
		"[SAP_Test] u8Channel = %d\n", sBssMetricsResp->u8Channel);
	DBGLOG(REQ, INFO,
		"[SAP_Test] u16AssocStaNum = %d\n",
		sBssMetricsResp->u16AssocStaNum);
	DBGLOG(REQ, INFO,
		"[SAP_Test] u8ChanUtil = %d\n", sBssMetricsResp->u8ChanUtil);
	DBGLOG(REQ, INFO,
		"[SAP_Test] iChanNoise = %d\n", sBssMetricsResp->iChanNoise);

	i4Ret = MulAPAgentMontorSendMsg(EV_WLAN_MULTIAP_BSS_METRICS_RESPONSE,
		sBssMetricsResp, sizeof(*sBssMetricsResp));
	if (i4Ret < 0)
		DBGLOG(AAA, ERROR,
			"EV_WLAN_MULTIAP_BSS_METRICS_RESPONSE nl send msg failed!\n");

	kalMemFree(sBssMetricsResp, VIR_MEM_TYPE, sizeof(*sBssMetricsResp));

error:
	if (cmd_traffic)
		kalMemFree(cmd_traffic, VIR_MEM_TYPE, sizeof(*cmd_traffic));
	if (cmd_noise)
		kalMemFree(cmd_noise, VIR_MEM_TYPE, sizeof(*cmd_noise));

	aaaMulAPAgentChanNoiseControl(prGlueInfo, FALSE);
}
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT */

void aaaMulAPAgentStaEventNotify(
	IN struct STA_RECORD *prStaRec, IN unsigned char *pucAddr,
	IN unsigned char fgIsConnected)
{
	int32_t i4Ret = 0;
	struct T_MULTI_AP_STA_EVENT_NOTIFY *prStaEventNotify;

	/* STA Event notification */
	/* Multi-AP_Specification_v1.0 17.2.19 */
	if (prStaRec->u2AssocReqIeLen
		+ OFFSET_OF(struct T_MULTI_AP_STA_EVENT_NOTIFY, u8Cap)
		> sizeof(struct T_MULTI_AP_STA_EVENT_NOTIFY)) {
		DBGLOG(AAA, ERROR,
			"IE too large %d %d\n",
			prStaRec->u2AssocReqIeLen,
			sizeof(struct T_MULTI_AP_STA_EVENT_NOTIFY));
		return;
	}

	prStaEventNotify = kalMemAlloc(sizeof(*prStaEventNotify), VIR_MEM_TYPE);
	if (!prStaEventNotify) {
		DBGLOG(AAA, ERROR, "mem alloc fail\n");
		return;
	}

	COPY_MAC_ADDR(prStaEventNotify->mStaMac, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prStaEventNotify->mBssid, pucAddr);
	prStaEventNotify->u8Status = fgIsConnected;
	prStaEventNotify->uCapLen = 1 + 2 + 2 + prStaRec->u2AssocReqIeLen;
	prStaEventNotify->u8Cap[0] = 0; /* 0:Success 1:Fail */
	WLAN_SET_FIELD_16(prStaEventNotify->u8Cap + 1, prStaRec->u2CapInfo);
	WLAN_SET_FIELD_16(prStaEventNotify->u8Cap + 3,
		prStaRec->u2ListenInterval);
	kalMemCopy(prStaEventNotify->u8Cap + 5,
		prStaRec->pucAssocReqIe,
		prStaRec->u2AssocReqIeLen);

	DBGLOG(AAA, INFO,
		"[SAP_Test] mStaMac=" MACSTR "\n",
		MAC2STR(prStaEventNotify->mStaMac));
	DBGLOG(AAA, INFO,
		"[SAP_Test] mBssid=" MACSTR "\n",
		MAC2STR(prStaEventNotify->mBssid));
	DBGLOG(AAA, INFO,
		"[SAP_Test] u8Status=%d\n", prStaEventNotify->u8Status);
	DBGLOG(AAA, INFO,
		"[SAP_Test] uCapLen=%d\n", prStaEventNotify->uCapLen);
	DBGLOG_MEM8(AAA, INFO,
		prStaEventNotify,
		offsetof(struct T_MULTI_AP_STA_EVENT_NOTIFY, u8Cap) + 5);
	DBGLOG_MEM8(AAA, INFO,
		prStaEventNotify->u8Cap, prStaEventNotify->uCapLen);

	i4Ret = MulAPAgentMontorSendMsg(EV_WLAN_MULTIAP_STA_TOPOLOGY_NOTIFY,
		prStaEventNotify, sizeof(*prStaEventNotify));
	if (i4Ret < 0)
		DBGLOG(AAA, ERROR,
			"EV_WLAN_MULTIAP_STA_TOPOLOGY_NOTIFY nl send msg failed!\n");

	kalMemFree(prStaEventNotify, VIR_MEM_TYPE, sizeof(*prStaEventNotify));
}

void aaaMulAPAgentUnassocStaMeasureTimeout(
	IN struct ADAPTER *prAdapter, unsigned long ulParamPtr)
{
	int32_t i4Ret = 0;
	uint8_t ucIndex = 0;
	struct BSS_INFO *prBssInfo = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;
	struct T_MULTI_AP_STA_UNASSOC_METRICS_RESP
		*sStaUnAssocMetricsResp = NULL;

	prGlueInfo = prAdapter->prGlueInfo;

	prBssInfo = (struct BSS_INFO *) ulParamPtr;
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		return;
	}

	sStaUnAssocMetricsResp =
		kalMemAlloc(sizeof(*sStaUnAssocMetricsResp), VIR_MEM_TYPE);
	if (!sStaUnAssocMetricsResp) {
		DBGLOG(INIT, ERROR, "mem alloc fail\n");
		return;
	}
	kalMemZero(sStaUnAssocMetricsResp,
		sizeof(struct T_MULTI_AP_STA_UNASSOC_METRICS_RESP));

	/* Interface Index */
	i4Ret = sscanf(prGlueInfo->prP2PInfo[1]->prDevHandler->name,
		"ap%u", &sStaUnAssocMetricsResp->uIfIndex);
	if (i4Ret != 1)
		DBGLOG(P2P, WARN, "read sap index fail: %d\n", i4Ret);

	COPY_MAC_ADDR(sStaUnAssocMetricsResp->mBssid, prBssInfo->aucBSSID);
	kalMemCopy(sStaUnAssocMetricsResp->tMetrics,
		prBssInfo->arUnAssocSTA,
		sizeof(struct T_MULTI_AP_STA_UNASSOC_METRICS)
		* SAP_UNASSOC_METRICS_STA_MAX);

	for (ucIndex = 0; ucIndex < SAP_UNASSOC_METRICS_STA_MAX; ucIndex++) {
		if (EQUAL_MAC_ADDR(aucZeroMacAddr,
			sStaUnAssocMetricsResp->tMetrics[ucIndex].mStaMac))
			continue;

		sStaUnAssocMetricsResp->u8StaNum++;
		if (sStaUnAssocMetricsResp->tMetrics[ucIndex].uTime != 0)
			sStaUnAssocMetricsResp->tMetrics[ucIndex].uTime =
				kalGetTimeTick()
				- sStaUnAssocMetricsResp
				->tMetrics[ucIndex].uTime;
	}

	for (ucIndex = 0; ucIndex < SAP_UNASSOC_METRICS_STA_MAX; ucIndex++) {
		DBGLOG(REQ, INFO,
			"[SAP_Test] [Report] arUnAssocSTA[%d]="MACSTR
			",time=%u, RSSI=%d, ch=%d\n",
			ucIndex,
			MAC2STR(sStaUnAssocMetricsResp
				->tMetrics[ucIndex].mStaMac),
			sStaUnAssocMetricsResp->tMetrics[ucIndex].uTime,
			sStaUnAssocMetricsResp->tMetrics[ucIndex].iRssi,
			sStaUnAssocMetricsResp->tMetrics[ucIndex].u8Channel);
	}
	DBGLOG(REQ, INFO,
		"[SAP_Test] uIfIndex = %u\n",
		sStaUnAssocMetricsResp->uIfIndex);
	DBGLOG(REQ, INFO,
		"[SAP_Test] mBssid = " MACSTR "\n",
		MAC2STR(sStaUnAssocMetricsResp->mBssid));
	DBGLOG(REQ, INFO,
		"[SAP_Test] u8StaNum = %u\n",
		sStaUnAssocMetricsResp->u8StaNum);

	i4Ret = MulAPAgentMontorSendMsg(
		EV_WLAN_MULTIAP_UNASSOC_STA_METRICS_RESPONSE,
		sStaUnAssocMetricsResp, sizeof(*sStaUnAssocMetricsResp));
	if (i4Ret < 0)
		DBGLOG(AAA, ERROR,
			"EV_WLAN_MULTIAP_UNASSOC_STA_METRICS_RESPONSE nl send msg failed!\n");

	kalMemFree(sStaUnAssocMetricsResp,
		VIR_MEM_TYPE,
		sizeof(struct T_MULTI_AP_STA_UNASSOC_METRICS_RESP));
}
#endif
#endif /* CFG_SUPPORT_AAA */

#if 0
/* TODO(Kevin): for abort event, just reset the STA_RECORD_T. */
/*---------------------------------------------------------------------------*/
/*!
 * \brief This function will send ABORT Event to JOIN FSM.
 *
 * \param[in] prAdapter  Pointer to the Adapter structure.
 *
 * \return none
 */
/*---------------------------------------------------------------------------*/
void saaFsmRunEventAbort(IN struct MSG_HDR *prMsgHdr)
{
	P_JOIN_INFO_T prJoinInfo;
	struct STA_RECORD *prStaRec;

	DEBUGFUNC("joinFsmRunEventAbort");

	ASSERT(prAdapter);
	prJoinInfo = &prAdapter->rJoinInfo;

	DBGLOG(JOIN, EVENT, "JOIN EVENT: ABORT\n");

	/* NOTE(Kevin): when reach here,
	 * the ARB_STATE should be in ARB_STATE_JOIN.
	 */
	ASSERT(prJoinInfo->prBssDesc);

	/* 4 <1> Update Flags and Elements of JOIN Module. */
	/* Reset Send Auth/(Re)Assoc Frame Count */
	prJoinInfo->ucTxAuthAssocRetryCount = 0;

	/* Cancel all JOIN relative Timer */
	ARB_CANCEL_TIMER(prAdapter, prJoinInfo->rTxRequestTimer);

	ARB_CANCEL_TIMER(prAdapter, prJoinInfo->rRxResponseTimer);

	ARB_CANCEL_TIMER(prAdapter, prJoinInfo->rJoinTimer);

	/* 4 <2> Update the associated
	 * STA_RECORD_T during JOIN.
	 */
	/* Get a Station Record if possible, TA == BSSID for AP */
	prStaRec = staRecGetStaRecordByAddr(prAdapter,
		prJoinInfo->prBssDesc->aucBSSID);
	if (prStaRec)
		prStaRec->ucStaState = STA_STATE_1;
		/* Update Station Record - Class 1 Flag */
#if DBG
	else
		ASSERT(0);
		/* Shouldn't happened, because we already
		 * add this STA_RECORD_T at JOIN_STATE_INIT
		 */

#endif /* DBG */

	/* 4 <3> Pull back to IDLE. */
	joinFsmSteps(prAdapter, JOIN_STATE_IDLE);

	/* 4 <4> If we are in Roaming, recover the settings of previous BSS. */
	/* NOTE: JOIN FAIL -
	 * Restore original setting from current struct BSS_INFO.
	 */
	if (prAdapter->eConnectionState == MEDIA_STATE_CONNECTED)
		joinAdoptParametersFromCurrentBss(prAdapter);
}				/* end of joinFsmRunEventAbort() */
#endif

/* TODO(Kevin): following code will be modified and move to AIS FSM */
#if 0
/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will send Join Timeout Event to JOIN FSM.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 *
 * \retval WLAN_STATUS_FAILURE   Fail because of Join Timeout
 */
/*----------------------------------------------------------------------------*/
uint32_t joinFsmRunEventJoinTimeOut(IN struct ADAPTER *prAdapter)
{
	P_JOIN_INFO_T prJoinInfo;
	struct STA_RECORD *prStaRec;

	DEBUGFUNC("joinFsmRunEventJoinTimeOut");

	ASSERT(prAdapter);
	prJoinInfo = &prAdapter->rJoinInfo;

	DBGLOG(JOIN, EVENT, "JOIN EVENT: JOIN TIMEOUT\n");

	/* Get a Station Record if possible, TA == BSSID for AP */
	prStaRec = staRecGetStaRecordByAddr(prAdapter,
		prJoinInfo->prBssDesc->aucBSSID);

	/* We have renew this Sta Record when in JOIN_STATE_INIT */
	ASSERT(prStaRec);

	/* Record the Status Code of Authentication Request */
	prStaRec->u2StatusCode = STATUS_CODE_JOIN_TIMEOUT;

	/* Increase Failure Count */
	prStaRec->ucJoinFailureCount++;

	/* Reset Send Auth/(Re)Assoc Frame Count */
	prJoinInfo->ucTxAuthAssocRetryCount = 0;

	/* Cancel other JOIN relative Timer */
	ARB_CANCEL_TIMER(prAdapter, prJoinInfo->rTxRequestTimer);

	ARB_CANCEL_TIMER(prAdapter, prJoinInfo->rRxResponseTimer);

	/* Restore original setting from current BSS_INFO_T */
	if (prAdapter->eConnectionState == MEDIA_STATE_CONNECTED)
		joinAdoptParametersFromCurrentBss(prAdapter);

	/* Pull back to IDLE */
	joinFsmSteps(prAdapter, JOIN_STATE_IDLE);

	return WLAN_STATUS_FAILURE;

}				/* end of joinFsmRunEventJoinTimeOut() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will adopt the parameters from Peer BSS.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void joinAdoptParametersFromPeerBss(IN struct ADAPTER *prAdapter)
{
	P_JOIN_INFO_T prJoinInfo;
	struct BSS_DESC *prBssDesc;

	DEBUGFUNC("joinAdoptParametersFromPeerBss");

	ASSERT(prAdapter);
	prJoinInfo = &prAdapter->rJoinInfo;
	prBssDesc = prJoinInfo->prBssDesc;

	/* 4 <1> Adopt Peer BSS' PHY TYPE */
	prAdapter->eCurrentPhyType = prBssDesc->ePhyType;

	DBGLOG(JOIN, INFO, "Target BSS[%s]'s PhyType = %s\n",
	       prBssDesc->aucSSID,
	       (prBssDesc->ePhyType == PHY_TYPE_ERP_INDEX) ? "ERP" : "HR_DSSS");

	/* 4 <2> Adopt Peer BSS' Frequency(Band/Channel) */
	DBGLOG(JOIN, INFO,
		"Target BSS's Channel = %d, Band = %d\n",
		prBssDesc->ucChannelNum, prBssDesc->eBand);

	nicSwitchChannel(prAdapter,
		prBssDesc->eBand, prBssDesc->ucChannelNum, 10);

	prJoinInfo->fgIsParameterAdopted = TRUE;
}				/* end of joinAdoptParametersFromPeerBss() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will adopt the parameters from current associated BSS.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void joinAdoptParametersFromCurrentBss(IN struct ADAPTER *prAdapter)
{
	/* P_JOIN_INFO_T prJoinInfo = &prAdapter->rJoinInfo; */
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);
	prBssInfo = &prAdapter->rBssInfo;

	/* 4 <1> Adopt current BSS' PHY TYPE */
	prAdapter->eCurrentPhyType = prBssInfo->ePhyType;

	/* 4 <2> Adopt current BSS' Frequency(Band/Channel) */
	DBGLOG(JOIN, INFO,
		"Current BSS's Channel = %d, Band = %d\n",
		prBssInfo->ucChnl, prBssInfo->eBand);

	nicSwitchChannel(prAdapter, prBssInfo->eBand, prBssInfo->ucChnl, 10);
}				/* end of joinAdoptParametersFromCurrentBss() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will update all the SW variables and
 *        HW MCR registers after
 *        the association with target BSS.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void joinComplete(IN struct ADAPTER *prAdapter)
{
	P_JOIN_INFO_T prJoinInfo;
	struct BSS_DESC *prBssDesc;
	P_PEER_BSS_INFO_T prPeerBssInfo;
	struct BSS_INFO *prBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct STA_RECORD *prStaRec;
	struct TX_CTRL *prTxCtrl;
#if CFG_SUPPORT_802_11D
	struct IE_COUNTRY *prIECountry;
#endif

	DEBUGFUNC("joinComplete");

	ASSERT(prAdapter);
	prJoinInfo = &prAdapter->rJoinInfo;
	prBssDesc = prJoinInfo->prBssDesc;
	prPeerBssInfo = &prAdapter->rPeerBssInfo;
	prBssInfo = &prAdapter->rBssInfo;
	prConnSettings = &prAdapter->rConnSettings;
	prTxCtrl = &prAdapter->rTxCtrl;

/* 4 <1> Update Connecting & Connected Flag of BSS_DESC_T. */
	/* Remove previous AP's Connection Flags if have */
	scanRemoveConnectionFlagOfBssDescByBssid(prAdapter,
		prBssInfo->aucBSSID);

	prBssDesc->fgIsConnected = TRUE;	/* Mask as Connected */

	if (prBssDesc->fgIsHiddenSSID) {
		/* NOTE(Kevin): This is for the case of Passive Scan
		 * and the target BSS didn't
		 * broadcast SSID on its Beacon Frame.
		 */
		COPY_SSID(prBssDesc->aucSSID,
			  prBssDesc->ucSSIDLen,
			  prAdapter->rConnSettings.aucSSID,
			  prAdapter->rConnSettings.ucSSIDLen);

		if (prBssDesc->ucSSIDLen)
			prBssDesc->fgIsHiddenSSID = FALSE;

#if DBG
		else
			ASSERT(0);

#endif /* DBG */

		DBGLOG(JOIN, INFO,
			"Hidden SSID! - Update SSID : %s\n",
			prBssDesc->aucSSID);
	}

/* 4 <2> Update BSS_INFO_T from BSS_DESC_T */
	/* 4 <2.A> PHY Type */
	prBssInfo->ePhyType = prBssDesc->ePhyType;

	/* 4 <2.B> BSS Type */
	prBssInfo->eBSSType = BSS_TYPE_INFRASTRUCTURE;

	/* 4 <2.C> BSSID */
	COPY_MAC_ADDR(prBssInfo->aucBSSID, prBssDesc->aucBSSID);

	DBGLOG(JOIN, INFO,
		"JOIN to BSSID: [" MACSTR "]\n",
		MAC2STR(prBssDesc->aucBSSID));

	/* 4 <2.D> SSID */
	COPY_SSID(prBssInfo->aucSSID,
		prBssInfo->ucSSIDLen,
		prBssDesc->aucSSID,
		prBssDesc->ucSSIDLen);

	/* 4 <2.E> Channel / Band information. */
	prBssInfo->eBand = prBssDesc->eBand;
	prBssInfo->ucChnl = prBssDesc->ucChannelNum;

	/* 4 <2.F> RSN/WPA information. */
	secFsmRunEventStart(prAdapter);
	prBssInfo->u4RsnSelectedPairwiseCipher =
		prBssDesc->u4RsnSelectedPairwiseCipher;
	prBssInfo->u4RsnSelectedGroupCipher =
		prBssDesc->u4RsnSelectedGroupCipher;
	prBssInfo->u4RsnSelectedAKMSuite =
		prBssDesc->u4RsnSelectedAKMSuite;

	if (secRsnKeyHandshakeEnabled())
		prBssInfo->fgIsWPAorWPA2Enabled = TRUE;
	else
		prBssInfo->fgIsWPAorWPA2Enabled = FALSE;

	/* 4 <2.G> Beacon interval. */
	prBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;

	/* 4 <2.H> DTIM period. */
	prBssInfo->ucDtimPeriod = prBssDesc->ucDTIMPeriod;

	/* 4 <2.I> ERP Information */
	/* Our BSS's PHY_TYPE is ERP now. */
	if ((prBssInfo->ePhyType == PHY_TYPE_ERP_INDEX) &&
	    (prBssDesc->fgIsERPPresent)) {

		prBssInfo->fgIsERPPresent = TRUE;
		/* Save the ERP for later check */
		prBssInfo->ucERP = prBssDesc->ucERP;
	} else {
		/* Some AP, may send ProbeResp without ERP IE.
		 * Thus prBssDesc->fgIsERPPresent is FALSE.
		 */
		prBssInfo->fgIsERPPresent = FALSE;
		prBssInfo->ucERP = 0;
	}

#if CFG_SUPPORT_802_11D
	/* 4 <2.J> Country inforamtion of the associated AP */
	if (prConnSettings->fgMultiDomainCapabilityEnabled) {
		struct DOMAIN_INFO_ENTRY rDomainInfo;

		if (domainGetDomainInfoByScanResult(prAdapter, &rDomainInfo)) {
			if (prBssDesc->prIECountry) {
				prIECountry = prBssDesc->prIECountry;

				domainParseCountryInfoElem(prIECountry,
					&prBssInfo->rDomainInfo);

				/* use the domain get from the BSS info */
				prBssInfo->fgIsCountryInfoPresent = TRUE;
				nicSetupOpChnlList(prAdapter,
					prBssInfo->rDomainInfo.u2CountryCode,
					FALSE);
			} else {
				/* use the domain get from the scan result */
				prBssInfo->fgIsCountryInfoPresent = TRUE;
				nicSetupOpChnlList(prAdapter,
					rDomainInfo.u2CountryCode, FALSE);
			}
		}
	}
#endif

	/* 4 <2.K> Signal Power of the associated AP */
	prBssInfo->rRcpi = prBssDesc->rRcpi;
	prBssInfo->rRssi = RCPI_TO_dBm(prBssInfo->rRcpi);
	GET_CURRENT_SYSTIME(&prBssInfo->rRssiLastUpdateTime);

	/* 4 <2.L> Capability Field of the associated AP */
	prBssInfo->u2CapInfo = prBssDesc->u2CapInfo;

	DBGLOG(JOIN, INFO,
		"prBssInfo-> fgIsERPPresent = %d, ucERP = %02x, rRcpi = %d, rRssi = %ld\n",
		prBssInfo->fgIsERPPresent,
		prBssInfo->ucERP,
		prBssInfo->rRcpi,
		prBssInfo->rRssi);

/* 4 <3> Update BSS_INFO_T from PEER_BSS_INFO_T & NIC RATE FUNC */
	/* 4 <3.A> Association ID */
	prBssInfo->u2AssocId = prPeerBssInfo->u2AssocId;

	/* 4 <3.B> WMM Information */
	if (prAdapter->fgIsEnableWMM
		&& (prPeerBssInfo->rWmmInfo.ucWmmFlag & WMM_FLAG_SUPPORT_WMM)) {

		prBssInfo->fgIsWmmAssoc = TRUE;
		prTxCtrl->rTxQForVoipAccess = TXQ_AC3;

		qosWmmInfoInit(&prBssInfo->rWmmInfo,
			(prBssInfo->ePhyType == PHY_TYPE_HR_DSSS_INDEX)
			? TRUE : FALSE);

		if (prPeerBssInfo->rWmmInfo.ucWmmFlag
			& WMM_FLAG_AC_PARAM_PRESENT) {
			kalMemCopy(&prBssInfo->rWmmInfo,
				&prPeerBssInfo->rWmmInfo, sizeof(WMM_INFO_T));
		} else {
			kalMemCopy(&prBssInfo->rWmmInfo,
				&prPeerBssInfo->rWmmInfo,
				sizeof(WMM_INFO_T) -
				sizeof(prPeerBssInfo->rWmmInfo.arWmmAcParams));
		}
	} else {
		prBssInfo->fgIsWmmAssoc = FALSE;
		prTxCtrl->rTxQForVoipAccess = TXQ_AC1;

		kalMemZero(&prBssInfo->rWmmInfo, sizeof(WMM_INFO_T));
	}

	/* 4 <3.C> Operational Rate Set & BSS Basic Rate Set */
	prBssInfo->u2OperationalRateSet = prPeerBssInfo->u2OperationalRateSet;
	prBssInfo->u2BSSBasicRateSet = prPeerBssInfo->u2BSSBasicRateSet;

	/* 4 <3.D> Short Preamble */
	if (prBssInfo->fgIsERPPresent) {

/* NOTE(Kevin 2007/12/24): Truth Table.
 *Short Preamble Bit in
 *<AssocReq> <AssocResp w/i ERP> <BARKER(Long)> Final Driver Setting(Short)
 *TRUE        FALSE              FALSE          FALSE(shouldn't have such case,
 *                                                  use the AssocResp)
 *TRUE        FALSE              TRUE           FALSE
 *FALSE       FALSE              FALSE          FALSE(shouldn't have such case,
 *                                              use the AssocResp)
 *FALSE       FALSE              TRUE           FALSE
 *TRUE        TRUE               FALSE          TRUE(follow ERP)
 *TRUE        TRUE               TRUE           FALSE(follow ERP)
 *FALSE       TRUE               FALSE          FALSE(shouldn't have such case,
 *                                              and we should set to FALSE)
 *FALSE       TRUE               TRUE           FALSE(we should set to FALSE)
 */
		if ((prPeerBssInfo->fgIsShortPreambleAllowed) &&
		    ((prConnSettings->ePreambleType == PREAMBLE_TYPE_SHORT) ||
		     /* Short Preamble Option Enable is TRUE */
		     ((prConnSettings->ePreambleType == PREAMBLE_TYPE_AUTO)
		      && (prBssDesc->u2CapInfo & CAP_INFO_SHORT_PREAMBLE)))) {

			prBssInfo->fgIsShortPreambleAllowed = TRUE;

			if (prBssInfo->ucERP & ERP_INFO_BARKER_PREAMBLE_MODE)
				prBssInfo->fgUseShortPreamble = FALSE;
			else
				prBssInfo->fgUseShortPreamble = TRUE;

		} else {
			prBssInfo->fgIsShortPreambleAllowed = FALSE;
			prBssInfo->fgUseShortPreamble = FALSE;
		}
	} else {
	/* NOTE(Kevin 2007/12/24): Truth Table.
	 * Short Preamble Bit in
	 * <AssocReq> <AssocResp w/o ERP>     Final Driver Setting(Short)
	 * TRUE        FALSE                  FALSE
	 * FALSE       FALSE                  FALSE
	 * TRUE        TRUE                   TRUE
	 * FALSE       TRUE(status success)   TRUE
	 * --> Honor the result of prPeerBssInfo.
	 */

		prBssInfo->fgIsShortPreambleAllowed =
			prBssInfo->fgUseShortPreamble =
		    prPeerBssInfo->fgIsShortPreambleAllowed;
	}

	DBGLOG(JOIN, INFO,
		"prBssInfo->fgIsShortPreambleAllowed = %d, prBssInfo->fgUseShortPreamble = %d\n",
		prBssInfo->fgIsShortPreambleAllowed,
		prBssInfo->fgUseShortPreamble);

	/* 4 <3.E> Short Slot Time */
	/* AP support Short Slot Time */
	prBssInfo->fgUseShortSlotTime =
		prPeerBssInfo->fgUseShortSlotTime;

	DBGLOG(JOIN, INFO,
		"prBssInfo->fgUseShortSlotTime = %d\n",
		prBssInfo->fgUseShortSlotTime);

	nicSetSlotTime(prAdapter,
		prBssInfo->ePhyType,
		((prConnSettings->fgIsShortSlotTimeOptionEnable &&
		prBssInfo->fgUseShortSlotTime) ? TRUE : FALSE));

	/* 4 <3.F> Update Tx Rate for Control Frame */
	bssUpdateTxRateForControlFrame(prAdapter);

	/* 4 <3.G> Save the available Auth Types
	 * during Roaming (Design for Fast BSS Transition).
	 */
	/* if (prAdapter->fgIsEnableRoaming) */
	/* NOTE(Kevin): Always prepare info for roaming */
	{

		if (prJoinInfo->ucCurrAuthAlgNum
			== AUTH_ALGORITHM_NUM_OPEN_SYSTEM)
			prJoinInfo->ucRoamingAuthTypes |= AUTH_TYPE_OPEN_SYSTEM;
		else if (prJoinInfo->ucCurrAuthAlgNum
			== AUTH_ALGORITHM_NUM_SHARED_KEY)
			prJoinInfo->ucRoamingAuthTypes |= AUTH_TYPE_SHARED_KEY;

		prBssInfo->ucRoamingAuthTypes = prJoinInfo->ucRoamingAuthTypes;

		/* Set the stable time of the associated BSS.
		 * We won't do roaming decision
		 * during the stable time.
		 */
		SET_EXPIRATION_TIME(prBssInfo->rRoamingStableExpirationTime,
				    SEC_TO_SYSTIME(ROAMING_STABLE_TIMEOUT_SEC));
	}

	/* 4 <3.H> Update Parameter for TX Fragmentation Threshold */
#if CFG_TX_FRAGMENT
	txFragInfoUpdate(prAdapter);
#endif /* CFG_TX_FRAGMENT */

/* 4 <4> Update STA_RECORD_T */
	/* Get a Station Record if possible */
	prStaRec = staRecGetStaRecordByAddr(prAdapter, prBssDesc->aucBSSID);

	if (prStaRec) {
		uint16_t u2OperationalRateSet, u2DesiredRateSet;

		/* 4 <4.A> Desired Rate Set */
		u2OperationalRateSet =
			(rPhyAttributes
			[prBssInfo->ePhyType].u2SupportedRateSet &
			prBssInfo->u2OperationalRateSet);

		u2DesiredRateSet =
			(u2OperationalRateSet
			& prConnSettings->u2DesiredRateSet);
		if (u2DesiredRateSet) {
			prStaRec->u2DesiredRateSet = u2DesiredRateSet;
		} else {
			/* For Error Handling - The Desired Rate Set is
			 * not covered in Operational Rate Set.
			 */
			prStaRec->u2DesiredRateSet = u2OperationalRateSet;
		}

		/* Try to set the best initial rate for this entry */
		if (!rateGetBestInitialRateIndex(prStaRec->u2DesiredRateSet,
			prStaRec->rRcpi, &prStaRec->ucCurrRate1Index)) {

			if (!rateGetLowestRateIndexFromRateSet(
				prStaRec->u2DesiredRateSet,
				&prStaRec->ucCurrRate1Index))
				ASSERT(0);
		}

		DBGLOG(JOIN, INFO,
			"prStaRec->ucCurrRate1Index = %d\n",
			prStaRec->ucCurrRate1Index);

		/* 4 <4.B> Preamble Mode */
		prStaRec->fgIsShortPreambleOptionEnable =
			prBssInfo->fgUseShortPreamble;

		/* 4 <4.C> QoS Flag */
		prStaRec->fgIsQoS = prBssInfo->fgIsWmmAssoc;
	}
#if DBG
	else
		ASSERT(0);

#endif /* DBG */

/* 4 <5> Update NIC */
	/* 4 <5.A> Update BSSID & Operation Mode */
	nicSetupBSS(prAdapter, prBssInfo);

	/* 4 <5.B> Update WLAN Table. */
	if (nicSetHwBySta(prAdapter, prStaRec) == FALSE)
		ASSERT(FALSE);

	/* 4 <5.C> Update Desired Rate Set for BT. */
#if CFG_TX_FRAGMENT
	if (prConnSettings->fgIsEnableTxAutoFragmentForBT)
		txRateSetInitForBT(prAdapter, prStaRec);

#endif /* CFG_TX_FRAGMENT */

	/* 4 <5.D> TX AC Parameter and TX/RX Queue Control */
	if (prBssInfo->fgIsWmmAssoc) {

#if CFG_TX_AGGREGATE_HW_FIFO
		nicTxAggregateTXQ(prAdapter, FALSE);
#endif /* CFG_TX_AGGREGATE_HW_FIFO */

		qosUpdateWMMParametersAndAssignAllowedACI(prAdapter,
			&prBssInfo->rWmmInfo);
	} else {

#if CFG_TX_AGGREGATE_HW_FIFO
		nicTxAggregateTXQ(prAdapter, TRUE);
#endif /* CFG_TX_AGGREGATE_HW_FIFO */

		nicTxNonQoSAssignDefaultAdmittedTXQ(prAdapter);

		nicTxNonQoSUpdateTXQParameters(prAdapter,
			prBssInfo->ePhyType);
	}

#if CFG_TX_STOP_WRITE_TX_FIFO_UNTIL_JOIN
	{
		prTxCtrl->fgBlockTxDuringJoin = FALSE;

#if !CFG_TX_AGGREGATE_HW_FIFO	/* TX FIFO AGGREGATE already do flush once */
		nicTxFlushStopQueues(prAdapter,
			(uint8_t) TXQ_DATA_MASK, (uint8_t) NULL);
#endif /* CFG_TX_AGGREGATE_HW_FIFO */

		nicTxRetransmitOfSendWaitQue(prAdapter);

		if (prTxCtrl->fgIsPacketInOsSendQueue)
			nicTxRetransmitOfOsSendQue(prAdapter);

#if CFG_SDIO_TX_ENHANCE
		halTxLeftClusteredMpdu(prAdapter);
#endif /* CFG_SDIO_TX_ENHANCE */

	}
#endif /* CFG_TX_STOP_WRITE_TX_FIFO_UNTIL_JOIN */

/* 4 <6> Setup CONNECTION flag. */
	prAdapter->eConnectionState = MEDIA_STATE_CONNECTED;
	prAdapter->eConnectionStateIndicated = MEDIA_STATE_CONNECTED;

	if (prJoinInfo->fgIsReAssoc)
		prAdapter->fgBypassPortCtrlForRoaming = TRUE;
	else
		prAdapter->fgBypassPortCtrlForRoaming = FALSE;

	kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
		WLAN_STATUS_MEDIA_CONNECT, (void *) NULL, 0);
}				/* end of joinComplete() */
#endif
