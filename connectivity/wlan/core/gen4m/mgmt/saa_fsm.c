// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "saa_fsm.c"
 *    \brief  This file defines the FSM for SAA MODULE.
 *
 *    This file defines the FSM for SAA MODULE.
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
static const char * const apucDebugAAState[AA_STATE_NUM] = {
	"AA_IDLE",
	"SAA_SEND_AUTH1",
	"SAA_WAIT_AUTH2",
	"SAA_SEND_AUTH3",
	"SAA_WAIT_AUTH4",
	"SAA_EXTERNAL_AUTH",
	"SAA_SEND_ASSOC1",
	"SAA_WAIT_ASSOC2",
	"AAA_SEND_AUTH2",
	"AAA_SEND_AUTH4",
	"AAA_SEND_ASSOC2",
	"AA_RESOURCE",
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

/*----------------------------------------------------------------------------*/
/*!
 * @brief The Core FSM engine of SAA Module.
 *
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] eNextState         The value of Next State
 * @param[in] prRetainedSwRfb    Pointer to the retained SW_RFB_T for JOIN
 *                               Success
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void
saaFsmSteps(struct ADAPTER *prAdapter,
	    struct STA_RECORD *prStaRec, enum ENUM_AA_STATE eNextState,
	    struct SW_RFB *prRetainedSwRfb)
{
	uint32_t rStatus;
	enum ENUM_AA_STATE ePreviousState;
	u_int8_t fgIsTransition;
	uint32_t u4AuthAssocState;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	if (!prStaRec)
		return;
	else if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON)) {
		DBGLOG(SAA, WARN, "saaFsmStateAbort, driver is not ready\n");
		saaFsmStateAbort(prAdapter, prStaRec, prRetainedSwRfb);
		return;
	}

	do {
		u4AuthAssocState = prStaRec->eAuthAssocState;
		if ((u4AuthAssocState < AA_STATE_NUM)
				&& ((uint32_t) eNextState < AA_STATE_NUM)) {
			DBGLOG(SAA, STATE, "[SAA]TRANSITION: [%s] -> [%s]\n",
				apucDebugAAState[u4AuthAssocState],
				apucDebugAAState[(uint32_t)eNextState]);
		}

		ePreviousState = prStaRec->eAuthAssocState;

		/* NOTE(Kevin): This is the only place to change the
		 * eAuthAssocState(except initial)
		 */
		prStaRec->eAuthAssocState = eNextState;

		fgIsTransition = (u_int8_t) FALSE;
		switch (prStaRec->eAuthAssocState) {
		case AA_STATE_IDLE:
			DBGLOG(SAA, TRACE,
				"authAlgNum %d, AuthTranNum %d\n",
				prStaRec->ucAuthAlgNum,
				prStaRec->ucAuthTranNum);
			if (prStaRec->ucAuthAlgNum == AUTH_ALGORITHM_NUM_FT &&
			    prStaRec->ucAuthTranNum == AUTH_TRANSACTION_SEQ_2 &&
			    prStaRec->ucStaState == STA_STATE_1) {
				struct PARAM_STATUS_INDICATION rStatus = {
				.eStatusType =
				ENUM_STATUS_TYPE_FT_AUTH_STATUS};
				struct FT_EVENT_PARAMS *prFtParam =
				aisGetFtEventParam(prAdapter,
				prStaRec->ucBssIndex);
				prFtParam->prTargetAp = prStaRec;
				/* now, we don't support RIC first */
				prFtParam->pcRicIes = NULL;
				prFtParam->u2RicIesLen = 0;
				DBGLOG(SAA, INFO,
					"[%d] FT: notify supplicant to update FT IEs\n",
					prStaRec->ucBssIndex);
				kalIndicateStatusAndComplete(
					prAdapter->prGlueInfo,
					WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
					&rStatus, sizeof(rStatus),
					prStaRec->ucBssIndex);
				break;
				/* wait supplicant update ft ies and then
				 * continue to send assoc 1
				 */
			}

			/* Only trigger this event once */
			if (ePreviousState != prStaRec->eAuthAssocState) {
				uint32_t status =
					prStaRec->u2StatusCode ==
					STATUS_CODE_SUCCESSFUL &&
					prRetainedSwRfb ?
					WLAN_STATUS_SUCCESS :
					WLAN_STATUS_FAILURE;
				uint32_t result = saaFsmSendEventJoinComplete(
						prAdapter, status, prStaRec,
						prRetainedSwRfb);

				if (result != WLAN_STATUS_SUCCESS) {
					if (status == WLAN_STATUS_SUCCESS ||
					    result == WLAN_STATUS_RESOURCES) {
						eNextState = AA_STATE_RESOURCE;
						fgIsTransition = TRUE;
					}
				}
			}

			/* Free allocated TCM memory */
			if (prStaRec->prChallengeText) {
				cnmMemFree(prAdapter,
					   prStaRec->prChallengeText);
				prStaRec->prChallengeText =
					(struct IE_CHALLENGE_TEXT *) NULL;
			}

			break;

		case SAA_STATE_SEND_AUTH1:

			/* Do tasks in INIT STATE */
			if (prStaRec->ucTxAuthAssocRetryCount >=
					prStaRec->ucTxAuthAssocRetryLimit) {

				/* Record the Status Code of Auth Request */
				prStaRec->u2StatusCode =
						STATUS_CODE_AUTH_TIMEOUT;

				eNextState = AA_STATE_IDLE;
				fgIsTransition = TRUE;
			} else {
				prStaRec->ucTxAuthAssocRetryCount++;
				prStaRec->ucAuthTranNum =
					AUTH_TRANSACTION_SEQ_1;
				/* Update Station Record - Class 1 Flag */
				if (prStaRec->ucStaState != STA_STATE_1)
					cnmStaRecChangeState(prAdapter,
						     prStaRec, STA_STATE_1);

				rStatus = authSendAuthFrame(prAdapter,
						      prStaRec,
						      prStaRec->ucBssIndex,
						      NULL,
						      AUTH_TRANSACTION_SEQ_1,
						      STATUS_CODE_RESERVED);

				if (rStatus != WLAN_STATUS_SUCCESS) {
					cnmTimerInitTimer(prAdapter,
					   &prStaRec->rTxReqDoneOrRxRespTimer,
					   (PFN_MGMT_TIMEOUT_FUNC)
					   saaFsmRunEventTxReqTimeOut,
					   (uintptr_t) prStaRec);

					cnmTimerStartTimer(prAdapter,
					   &prStaRec->rTxReqDoneOrRxRespTimer,
					   TU_TO_MSEC(
					   TX_AUTHENTICATION_RETRY_TIMEOUT_TU));
				}
			}

			break;

		case SAA_STATE_WAIT_AUTH2:
			break;

		case SAA_STATE_SEND_AUTH3:

			/* Do tasks in INIT STATE */
			if (prStaRec->ucTxAuthAssocRetryCount >=
			    prStaRec->ucTxAuthAssocRetryLimit) {

				/* Record the Status Code of Auth Request */
				prStaRec->u2StatusCode =
						STATUS_CODE_AUTH_TIMEOUT;

				eNextState = AA_STATE_IDLE;
				fgIsTransition = TRUE;
			} else {
				prStaRec->ucTxAuthAssocRetryCount++;
				prStaRec->ucAuthTranNum =
					AUTH_TRANSACTION_SEQ_3;

				rStatus = authSendAuthFrame(prAdapter,
						      prStaRec,
						      prStaRec->ucBssIndex,
						      NULL,
						      AUTH_TRANSACTION_SEQ_3,
						      STATUS_CODE_RESERVED);

				if (rStatus != WLAN_STATUS_SUCCESS) {
					cnmTimerInitTimer(prAdapter,
					   &prStaRec->rTxReqDoneOrRxRespTimer,
					   (PFN_MGMT_TIMEOUT_FUNC)
					   saaFsmRunEventTxReqTimeOut,
					   (uintptr_t) prStaRec);

					cnmTimerStartTimer(prAdapter,
					   &prStaRec->rTxReqDoneOrRxRespTimer,
					   TU_TO_MSEC(
					   TX_AUTHENTICATION_RETRY_TIMEOUT_TU));
				}
			}

			break;

		case SAA_STATE_WAIT_AUTH4:
			break;

#if CFG_SUPPORT_WPA3
		case SAA_STATE_EXTERNAL_AUTH:
			kalExternalAuthRequest(
				prAdapter->prGlueInfo,
				prStaRec);
			break;
#endif /* CFG_SUPPORT_WPA3 */

		case SAA_STATE_SEND_ASSOC1:
			/* Do tasks in INIT STATE */
			if (prStaRec->ucTxAuthAssocRetryCount >=
			    prStaRec->ucTxAuthAssocRetryLimit) {

				/* Record the Status Code of Auth Request */
				prStaRec->u2StatusCode =
						STATUS_CODE_ASSOC_TIMEOUT;

				eNextState = AA_STATE_IDLE;
				fgIsTransition = TRUE;
			} else {
				prStaRec->ucTxAuthAssocRetryCount++;

				rStatus = assocSendReAssocReqFrame(prAdapter,
								   prStaRec);
				if (rStatus != WLAN_STATUS_SUCCESS) {
					cnmTimerInitTimer(prAdapter,
					    &prStaRec->rTxReqDoneOrRxRespTimer,
					    (PFN_MGMT_TIMEOUT_FUNC)
					    saaFsmRunEventTxReqTimeOut,
					    (uintptr_t) prStaRec);

					cnmTimerStartTimer(prAdapter,
					    &prStaRec->rTxReqDoneOrRxRespTimer,
					    TU_TO_MSEC(
					    TX_ASSOCIATION_RETRY_TIMEOUT_TU));
				}
			}

			break;

		case SAA_STATE_WAIT_ASSOC2:
			break;

		case AA_STATE_RESOURCE:
			/* TODO(Kevin) Can setup a timer and send
			 * message later
			 */
			break;

		default:
			DBGLOG(SAA, ERROR, "Unknown AA STATE\n");
			ASSERT(0);
			break;
		}

	} while (fgIsTransition);

	return;

}				/* end of saaFsmSteps() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send Event to AIS/BOW/P2P
 *
 * @param[in] rJoinStatus        To indicate JOIN success or failure.
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] prSwRfb            Pointer to the SW_RFB_T

 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t
saaFsmSendEventJoinComplete(struct ADAPTER *prAdapter,
			    uint32_t rJoinStatus,
			    struct STA_RECORD *prStaRec,
			    struct SW_RFB *prSwRfb)
{
	if (!prStaRec) {
		DBGLOG(SAA, ERROR, "[%s]prStaRec is NULL\n", __func__);
		return WLAN_STATUS_INVALID_PACKET;
	}
	if (!prAdapter) {
		DBGLOG(SAA, ERROR, "[%s]prAdapter is NULL\n", __func__);
		return WLAN_STATUS_INVALID_PACKET;
	}
	if (prStaRec->ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(NIC, ERROR, "ucBssIndex out of range!\n");
		return WLAN_STATUS_FAILURE;
	}

	/* For wlan0 (AP) + p2p0, don't check the prAisBssInfo for the P2P. */
#if CFG_ENABLE_WIFI_DIRECT
	if ((prAdapter->fgIsP2PRegistered) &&
	    (IS_STA_IN_P2P(prAdapter, prStaRec))) {
		struct MSG_SAA_FSM_COMP *prSaaFsmCompMsg;

		prSaaFsmCompMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
					      sizeof(struct MSG_SAA_FSM_COMP));
		if (!prSaaFsmCompMsg)
			return WLAN_STATUS_RESOURCES;

		if (rJoinStatus == WLAN_STATUS_SUCCESS)
			prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;

		prSaaFsmCompMsg->rMsgHdr.eMsgId = MID_SAA_P2P_JOIN_COMPLETE;
		prSaaFsmCompMsg->ucSeqNum = prStaRec->ucAuthAssocReqSeqNum;
		prSaaFsmCompMsg->rJoinStatus = rJoinStatus;
		prSaaFsmCompMsg->prStaRec = prStaRec;
		prSaaFsmCompMsg->prSwRfb = prSwRfb;

		/* NOTE(Kevin): Set to UNBUF for immediately JOIN complete */
		mboxSendMsg(prAdapter, MBOX_ID_0,
			    (struct MSG_HDR *) prSaaFsmCompMsg,
			    MSG_SEND_METHOD_UNBUF);

		return WLAN_STATUS_SUCCESS;
	}
#endif /* CFG_ENABLE_WIFI_DIRECT */

	if (IS_STA_IN_AIS(prAdapter, prStaRec)) {
		struct MSG_SAA_FSM_COMP *prSaaFsmCompMsg;

		prSaaFsmCompMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
					      sizeof(struct MSG_SAA_FSM_COMP));
		if (!prSaaFsmCompMsg)
			return WLAN_STATUS_RESOURCES;

		if (rJoinStatus == WLAN_STATUS_SUCCESS)
			prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;

		prSaaFsmCompMsg->rMsgHdr.eMsgId = MID_SAA_AIS_JOIN_COMPLETE;
		prSaaFsmCompMsg->ucSeqNum = prStaRec->ucAuthAssocReqSeqNum;
		prSaaFsmCompMsg->rJoinStatus = rJoinStatus;
		prSaaFsmCompMsg->prStaRec = prStaRec;
		prSaaFsmCompMsg->prSwRfb = prSwRfb;

		/* NOTE(Kevin): Set to UNBUF for immediately JOIN complete */
		mboxSendMsg(prAdapter, MBOX_ID_0,
			    (struct MSG_HDR *) prSaaFsmCompMsg,
			    MSG_SEND_METHOD_UNBUF);

		return WLAN_STATUS_SUCCESS;
	}
#if CFG_ENABLE_BT_OVER_WIFI
	else if (IS_STA_BOW_TYPE(prStaRec)) {
		/* @TODO: BOW handler */

		struct MSG_SAA_FSM_COMP *prSaaFsmCompMsg;

		prSaaFsmCompMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
					      sizeof(struct MSG_SAA_FSM_COMP));
		if (!prSaaFsmCompMsg)
			return WLAN_STATUS_RESOURCES;

		prSaaFsmCompMsg->rMsgHdr.eMsgId = MID_SAA_BOW_JOIN_COMPLETE;
		prSaaFsmCompMsg->ucSeqNum = prStaRec->ucAuthAssocReqSeqNum;
		prSaaFsmCompMsg->rJoinStatus = rJoinStatus;
		prSaaFsmCompMsg->prStaRec = prStaRec;
		prSaaFsmCompMsg->prSwRfb = prSwRfb;

		/* NOTE(Kevin): Set to UNBUF for immediately JOIN complete */
		mboxSendMsg(prAdapter, MBOX_ID_0,
			    (struct MSG_HDR *) prSaaFsmCompMsg,
			    MSG_SEND_METHOD_UNBUF);

		return WLAN_STATUS_SUCCESS;
	}
#endif
	else {
		DBGLOG(SAA, ERROR, "Invalid case in %s.\n", __func__);
		return WLAN_STATUS_FAILURE;
	}

}				/* end of saaFsmSendEventJoinComplete() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Start Event to SAA FSM.
 *
 * @param[in] prMsgHdr   Message of Join Request for a particular STA.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventStart(struct ADAPTER *prAdapter,
			 struct MSG_HDR *prMsgHdr)
{
	struct MSG_SAA_FSM_START *prSaaFsmStartMsg;
	struct STA_RECORD *prStaRec;

	prSaaFsmStartMsg = (struct MSG_SAA_FSM_START *) prMsgHdr;
	prStaRec = prSaaFsmStartMsg->prStaRec;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);
		return;
	}

	DBGLOG(SAA, LOUD, "EVENT-START: Trigger SAA FSM.\n");

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldStarecSetSetupIdx(prAdapter, prStaRec);
#if (CFG_MLD_INFO_PRESETUP == 1)
	/* Update MAT and WTBL's MLD info before AUTH */
	mldSetupMlInfo(prAdapter, prStaRec);
#endif /* CFG_MLD_INFO_PRESETUP */
#endif

	/* record sequence number of request message */
	prStaRec->ucAuthAssocReqSeqNum = prSaaFsmStartMsg->ucSeqNum;

	cnmMemFree(prAdapter, prMsgHdr);

	/* 4 <1> Validation of SAA Start Event */
	if (!IS_AP_STA(prStaRec)) {

		DBGLOG(SAA, ERROR,
		       "EVENT-START: STA Type - %d was not supported.\n",
		       prStaRec->eStaType);

		/* Ignore the return value because don't care the prSwRfb */
		saaFsmSendEventJoinComplete(prAdapter, WLAN_STATUS_FAILURE,
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
	prStaRec->u2StatusCode = STATUS_CODE_UNSPECIFIED_FAILURE;

	/* Update the record join time. */
	GET_CURRENT_SYSTIME(&prStaRec->rLastJoinTime);

	prStaRec->ucTxAuthAssocRetryCount = 0;

	if (prStaRec->prChallengeText) {
		cnmMemFree(prAdapter, prStaRec->prChallengeText);
		prStaRec->prChallengeText = (struct IE_CHALLENGE_TEXT *) NULL;
	}

	cnmTimerStopTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer);

	/* 4 <4> Init the sec fsm */
	/* secFsmInit(prAdapter, prStaRec); */

	/* 4 <5> Reset the STA STATE */
	/* Update Station Record - Class 1 Flag */
	/* NOTE(Kevin): Moved to AIS FSM for Reconnect issue -
	 * We won't deactivate the same struct STA_RECORD and then activate it
	 * again for the case of reconnection.
	 */
	/* cnmStaRecChangeState(prStaRec, STA_STATE_1); */

	/* 4 <7> Trigger SAA FSM */
	if (prStaRec->ucStaState == STA_STATE_1) {
#if CFG_SUPPORT_WPA3_LOG
		wpa3LogSaaStart(prAdapter, prStaRec);
#endif
		if (prStaRec->ucAuthAlgNum == AUTH_ALGORITHM_NUM_SAE)
			saaFsmSteps(prAdapter, prStaRec,
				    SAA_STATE_EXTERNAL_AUTH,
				    (struct SW_RFB *) NULL);
		else if (prStaRec->ucAuthAlgNum == AUTH_ALGORITHM_NUM_FT &&
			 prStaRec->ucAuthTranNum == AUTH_TRANSACTION_SEQ_2) {
			saaFsmSteps(prAdapter, prStaRec,
				    AA_STATE_IDLE,
				    (struct SW_RFB *) NULL);
		} else
			saaFsmSteps(prAdapter, prStaRec,
				    SAA_STATE_SEND_AUTH1,
				    (struct SW_RFB *) NULL);
	} else if (prStaRec->ucStaState == STA_STATE_2 ||
		 prStaRec->ucStaState == STA_STATE_3)
		saaFsmSteps(prAdapter, prStaRec,
			    SAA_STATE_SEND_ASSOC1, (struct SW_RFB *) NULL);
}				/* end of saaFsmRunEventStart() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Continue Event to SAA FSM.
 *
 * @param[in] prMsgHdr   Message of Join Request for a particular STA.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventFTContinue(struct ADAPTER *prAdapter,
			      struct MSG_HDR *prMsgHdr)
{
	struct MSG_SAA_FT_CONTINUE *prSaaFsmMsg = NULL;
	struct STA_RECORD *prStaRec;
	u_int8_t fgFtRicRequest = FALSE;

	prSaaFsmMsg = (struct MSG_SAA_FT_CONTINUE *)prMsgHdr;
	prStaRec = prSaaFsmMsg->prStaRec;
	fgFtRicRequest = prSaaFsmMsg->fgFTRicRequest;
	cnmMemFree(prAdapter, prMsgHdr);
	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		DBGLOG(SAA, ERROR, "No Sta Record or it is not in use\n");
		return;
	}
	if (prStaRec->eAuthAssocState != AA_STATE_IDLE) {
		DBGLOG(SAA, ERROR,
		       "FT: Wrong SAA FSM state %d to continue auth/assoc\n",
		       prStaRec->eAuthAssocState);
		return;
	}
	DBGLOG(SAA, TRACE, "Continue to do auth/assoc\n");
	if (fgFtRicRequest)
		saaFsmSteps(prAdapter, prStaRec, SAA_STATE_SEND_AUTH3,
			    (struct SW_RFB *)NULL);
	else {
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);
		saaFsmSteps(prAdapter, prStaRec, SAA_STATE_SEND_ASSOC1,
			    (struct SW_RFB *)NULL);
	}
}				/* end of saaFsmRunEventFTContinue() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle TxDone(Auth1/Auth3/AssocReq) Event of SAA
 *        FSM.
 *
 * @param[in] prMsduInfo     Pointer to the MSDU_INFO_T.
 * @param[in] rTxDoneStatus  Return TX status of the Auth1/Auth3/AssocReq frame.
 *
 * @retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
saaFsmRunEventTxDone(struct ADAPTER *prAdapter,
		     struct MSDU_INFO *prMsduInfo,
		     enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{

	struct STA_RECORD *prStaRec;
	enum ENUM_AA_STATE eNextState;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prStaRec)
		return WLAN_STATUS_INVALID_PACKET;

	if (rTxDoneStatus)
		DBGLOG(SAA, INFO,
		       "EVENT-TX DONE [status: %d][seq: %d]: Current Time = %d\n",
		       rTxDoneStatus, prMsduInfo->ucTxSeqNum, kalGetTimeTick());

	eNextState = prStaRec->eAuthAssocState;

	switch (prStaRec->eAuthAssocState) {
	case SAA_STATE_SEND_AUTH1:
		{
			/* Strictly check the outgoing frame is matched with
			 * current AA STATE
			 */
			if (authCheckTxAuthFrame(prAdapter, prMsduInfo,
						 AUTH_TRANSACTION_SEQ_1) !=
						 WLAN_STATUS_SUCCESS)
				break;
#if (CFG_SUPPORT_CONN_LOG == 1)
			connLogAuthReq(prAdapter,
				prMsduInfo,
				rTxDoneStatus);
#endif

			if (rTxDoneStatus == TX_RESULT_SUCCESS) {
				eNextState = SAA_STATE_WAIT_AUTH2;

				cnmTimerStopTimer(prAdapter,
					&prStaRec->rTxReqDoneOrRxRespTimer);

				cnmTimerInitTimer(prAdapter,
				    &prStaRec->rTxReqDoneOrRxRespTimer,
				    (PFN_MGMT_TIMEOUT_FUNC)
				    saaFsmRunEventRxRespTimeOut,
				    (uintptr_t) prStaRec);

				cnmTimerStartTimer(prAdapter,
				    &prStaRec->rTxReqDoneOrRxRespTimer,
				    TU_TO_MSEC(
				    DOT11_AUTHENTICATION_RESPONSE_TIMEOUT_TU));
			}

#if CFG_SUPPORT_WPA3_LOG
			wpa3LogSaaTx(prAdapter,
				prMsduInfo,
				rTxDoneStatus);
#endif

			/* if TX was successful, change to next state.
			 * if TX was failed, do retry if possible.
			 */
			saaFsmSteps(prAdapter, prStaRec, eNextState,
				    (struct SW_RFB *) NULL);
		}
		break;

	case SAA_STATE_SEND_AUTH3:
		{
			/* Strictly check the outgoing frame is matched with
			 * current JOIN STATE
			 */
			if (authCheckTxAuthFrame(prAdapter, prMsduInfo,
						 AUTH_TRANSACTION_SEQ_3) !=
						 WLAN_STATUS_SUCCESS)
				break;

			if (rTxDoneStatus == TX_RESULT_SUCCESS) {
				eNextState = SAA_STATE_WAIT_AUTH4;

				cnmTimerStopTimer(prAdapter,
				      &prStaRec->rTxReqDoneOrRxRespTimer);

				cnmTimerInitTimer(prAdapter,
				      &prStaRec->rTxReqDoneOrRxRespTimer,
				      (PFN_MGMT_TIMEOUT_FUNC)
				      saaFsmRunEventRxRespTimeOut,
				      (uintptr_t) prStaRec);

				cnmTimerStartTimer(prAdapter,
				    &prStaRec->rTxReqDoneOrRxRespTimer,
				    TU_TO_MSEC(
				    DOT11_AUTHENTICATION_RESPONSE_TIMEOUT_TU));
			}

			/* if TX was successful, change to next state.
			 * if TX was failed, do retry if possible.
			 */
			saaFsmSteps(prAdapter, prStaRec, eNextState,
				    (struct SW_RFB *) NULL);
		}
		break;

	case SAA_STATE_SEND_ASSOC1:
		{
#if (CFG_SUPPORT_CONN_LOG == 1)
			connLogAssocReq(prAdapter,
				prMsduInfo,
				rTxDoneStatus);
#endif

			/* Strictly check the outgoing frame is matched with
			 * current SAA STATE
			 */
			if (assocCheckTxReAssocReqFrame(prAdapter, prMsduInfo)
				!= WLAN_STATUS_SUCCESS)
				break;

			if (rTxDoneStatus == TX_RESULT_SUCCESS) {
				eNextState = SAA_STATE_WAIT_ASSOC2;

				cnmTimerStopTimer(prAdapter,
						  &prStaRec->
						  rTxReqDoneOrRxRespTimer);

				cnmTimerInitTimer(prAdapter,
				      &prStaRec->rTxReqDoneOrRxRespTimer,
				      (PFN_MGMT_TIMEOUT_FUNC)
					saaFsmRunEventRxRespTimeOut,
				      (uintptr_t) prStaRec);

				cnmTimerStartTimer(prAdapter,
				      &(prStaRec->rTxReqDoneOrRxRespTimer),
				      TU_TO_MSEC(
				      DOT11_ASSOCIATION_RESPONSE_TIMEOUT_TU));
			}

			/* if TX was successful, change to next state.
			 * if TX was failed, do retry if possible.
			 */
			saaFsmSteps(prAdapter, prStaRec, eNextState,
				    (struct SW_RFB *) NULL);
		}
		break;

	default:
		break;		/* Ignore other cases */
	}

	return WLAN_STATUS_SUCCESS;

}				/* end of saaFsmRunEventTxDone() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send Tx Request Timeout Event to SAA FSM.
 *
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventTxReqTimeOut(struct ADAPTER *prAdapter,
				uintptr_t plParamPtr)
{
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) plParamPtr;

	if (!prStaRec)
		return;

	DBGLOG(SAA, LOUD, "EVENT-TIMER: TX REQ TIMEOUT, Current Time = %d\n",
	       kalGetTimeTick());
/* fos_change begin */
#if CFG_SUPPORT_EXCEPTION_STATISTICS
		prAdapter->total_mgmtTX_timeout_count++;
#endif /* fos_change end */

	switch (prStaRec->eAuthAssocState) {
	case SAA_STATE_SEND_AUTH1:
	case SAA_STATE_SEND_AUTH3:
	case SAA_STATE_SEND_ASSOC1:
		saaFsmSteps(prAdapter, prStaRec,
			    prStaRec->eAuthAssocState, (struct SW_RFB *) NULL);
		break;

	default:
		return;
	}
}				/* end of saaFsmRunEventTxReqTimeOut() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send Rx Response Timeout Event to SAA FSM.
 *
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventRxRespTimeOut(struct ADAPTER *prAdapter,
				 uintptr_t ulParamPtr)
{
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) ulParamPtr;
	enum ENUM_AA_STATE eNextState;

	DBGLOG(SAA, LOUD, "EVENT-TIMER: RX RESP TIMEOUT, Current Time = %d\n",
	       kalGetTimeTick());

	if (!prStaRec)
		return;

/* fos_change begin */
#if CFG_SUPPORT_EXCEPTION_STATISTICS
		prAdapter->total_mgmtRX_timeout_count++;
#endif /* fos_change end */

	eNextState = prStaRec->eAuthAssocState;

	switch (prStaRec->eAuthAssocState) {
	case SAA_STATE_WAIT_AUTH2:
		/* Record the Status Code of Authentication Request */
		prStaRec->u2StatusCode = STATUS_CODE_AUTH_TIMEOUT;
#if CFG_SUPPORT_WPA3_LOG
		wpa3LogAuthTimeout(prAdapter,
			prStaRec);
#endif
		/* Pull back to earlier state to do retry */
		eNextState = SAA_STATE_SEND_AUTH1;
		break;

	case SAA_STATE_WAIT_AUTH4:
		/* Record the Status Code of Authentication Request */
		prStaRec->u2StatusCode = STATUS_CODE_AUTH_TIMEOUT;

		/* Pull back to earlier state to do retry */
		eNextState = SAA_STATE_SEND_AUTH3;
		break;

	case SAA_STATE_WAIT_ASSOC2:
		/* Record the Status Code of Authentication Request */
		prStaRec->u2StatusCode = STATUS_CODE_ASSOC_TIMEOUT;
#if CFG_SUPPORT_WPA3_LOG
		wpa3LogAssocTimeout(prAdapter,
			prStaRec);
#endif
		/* Pull back to earlier state to do retry */
		eNextState = SAA_STATE_SEND_ASSOC1;
		break;

	default:
		break;		/* Ignore other cases */
	}

	if (eNextState != prStaRec->eAuthAssocState)
		saaFsmSteps(prAdapter, prStaRec, eNextState,
			    (struct SW_RFB *) NULL);
}				/* end of saaFsmRunEventRxRespTimeOut() */

struct STA_RECORD *saaFsmFindStaRec(struct ADAPTER *prAdapter,
		struct WLAN_MAC_MGMT_HEADER *mgmt)
{
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	uint8_t ucBssIdx = 0;

	do {
		for (ucBssIdx = 0;
			ucBssIdx < prAdapter->ucSwBssIdNum; ucBssIdx++) {
			if (!IS_NET_ACTIVE(prAdapter, ucBssIdx))
				continue;

			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

			if (prBssInfo &&
				EQUAL_MAC_ADDR(prBssInfo->aucOwnMacAddr,
				mgmt->aucDestAddr))
				break;

			prBssInfo = NULL;
		}

	} while (FALSE);

	if (!prBssInfo)
		return NULL;

	return cnmGetStaRecByAddress(prAdapter,
		prBssInfo->ucBssIndex, mgmt->aucBSSID);
}				/* p2pFuncBSSIDFindBssInfo */


/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will process the Rx Auth Response Frame and then
 *        trigger SAA FSM.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventRxAuth(struct ADAPTER *prAdapter,
			  struct SW_RFB *prSwRfb)
{
	struct STA_RECORD *prStaRec;
	uint16_t u2StatusCode;
	enum ENUM_AA_STATE eNextState;
	uint8_t ucWlanIdx;
	uint8_t ucStaRecIdx;

	ucStaRecIdx = prSwRfb->ucStaRecIdx;
	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIdx);
	ucWlanIdx = prSwRfb->ucWlanIdx;

	/* We should have the corresponding Sta Record. */
	if (!prStaRec) {
		struct WLAN_MAC_MGMT_HEADER *mgmt =
			(struct WLAN_MAC_MGMT_HEADER *)prSwRfb->pvHeader;

		DBGLOG(SAA, WARN,
			"Received a AuthResp: DA[" MACSTR "] bssid[" MACSTR
			"] wlanIdx[%d] staRecIdx[%d] w/o corresponding staRec\n",
			MAC2STR(mgmt->aucDestAddr),
			MAC2STR(mgmt->aucBSSID),
			ucWlanIdx,
			ucStaRecIdx);
#if (CFG_WLAN_CONNAC3_DEV == 1)
 		prStaRec = saaFsmFindStaRec(prAdapter, mgmt);
		if (!prStaRec) {
			DBGLOG(SAA, WARN, "StaRec not found\n");
			return;
		}

		DBGLOG(SAA, WARN,
			"StaRec=%d, widx=%d, IS_AP_STA=%d, State=%d, found\n",
			prStaRec->ucIndex, prStaRec->ucWlanIndex,
			!!IS_AP_STA(prStaRec), prStaRec->eAuthAssocState);

		prSwRfb->ucStaRecIdx = prStaRec->ucIndex;
		prSwRfb->ucWlanIdx = prStaRec->ucWlanIndex;
#else
		return;
#endif
	}

	if (!IS_AP_STA(prStaRec))
		return;

	switch (prStaRec->eAuthAssocState) {
	case SAA_STATE_SEND_AUTH1:
	case SAA_STATE_WAIT_AUTH2:
		/* Check if the incoming frame is what we are waiting for */
		if (authCheckRxAuthFrameStatus(prAdapter,
					       prSwRfb,
					       AUTH_TRANSACTION_SEQ_2,
					       &u2StatusCode) ==
					       WLAN_STATUS_SUCCESS) {

			cnmTimerStopTimer(prAdapter,
					  &prStaRec->rTxReqDoneOrRxRespTimer);

			/* Record the Status Code of Authentication Request */
			prStaRec->u2StatusCode = u2StatusCode;

			if (u2StatusCode == STATUS_CODE_SUCCESSFUL &&
			    authProcessRxAuth2_Auth4Frame(prAdapter, prSwRfb) ==
				WLAN_STATUS_SUCCESS) {

				prStaRec->ucAuthTranNum =
					AUTH_TRANSACTION_SEQ_2;
				/* after received Auth2 for FT, should indicate
				 * to supplicant
				 * and wait response from supplicant
				 */
				if (prStaRec->ucAuthAlgNum ==
				    AUTH_ALGORITHM_NUM_FT) {
					eNextState = AA_STATE_IDLE;
				} else if (prStaRec->ucAuthAlgNum ==
					 AUTH_ALGORITHM_NUM_SHARED_KEY) {
					eNextState = SAA_STATE_SEND_AUTH3;
#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
				} else if (prStaRec->ucAuthAlgNum ==
					AUTH_ALGORITHM_NUM_FILS_SK) {
					if (filsProcessAuth(prAdapter, prStaRec)
					    != WLAN_STATUS_SUCCESS) {
						DBGLOG(SAA, INFO,
							"FILS Authentication response processing failed");
						eNextState = AA_STATE_IDLE;
					} else {
						/* Update StaRec - Class 2 */
						cnmStaRecChangeState(
							prAdapter, prStaRec,
							STA_STATE_2);
						eNextState =
							SAA_STATE_SEND_ASSOC1;
					}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
				} else {
					/* Update Station Record - Class 2 */
					cnmStaRecChangeState(prAdapter,
							     prStaRec,
							     STA_STATE_2);

					eNextState = SAA_STATE_SEND_ASSOC1;
				}
			} else {
				DBGLOG(SAA, INFO,
				       "Auth Req was %s by [" MACSTR
				       "], Status Code = %d\n",
					u2StatusCode != STATUS_CODE_SUCCESSFUL ?
					"rejected" : "invalid",
				       MAC2STR(prStaRec->aucMacAddr),
				       u2StatusCode);
				eNextState = AA_STATE_IDLE;
			}

			/* Reset Send Auth/(Re)Assoc Frame Count */
			prStaRec->ucTxAuthAssocRetryCount = 0;

			saaFsmSteps(prAdapter, prStaRec, eNextState,
				    (struct SW_RFB *) NULL);
		}
		break;

	case SAA_STATE_SEND_AUTH3:
	case SAA_STATE_WAIT_AUTH4:
		/* Check if the incoming frame is what we are waiting for */
		if (authCheckRxAuthFrameStatus(prAdapter,
					       prSwRfb,
					       AUTH_TRANSACTION_SEQ_4,
					       &u2StatusCode) ==
					       WLAN_STATUS_SUCCESS) {

			cnmTimerStopTimer(prAdapter,
					  &prStaRec->rTxReqDoneOrRxRespTimer);

			/* Record the Status Code of Authentication Request */
			prStaRec->u2StatusCode = u2StatusCode;

			if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {

				/* Add for 802.11r handling */
				uint32_t rStatus =
					authProcessRxAuth2_Auth4Frame(prAdapter,
								      prSwRfb);

				prStaRec->ucAuthTranNum =
					AUTH_TRANSACTION_SEQ_4;
				/* if Auth4 check is failed(check mic in Auth
				 * ack frame), should disconnect
				 */
				if (prStaRec->ucAuthAlgNum ==
					AUTH_ALGORITHM_NUM_FT &&
				    rStatus != WLAN_STATUS_SUCCESS) {
					DBGLOG(SAA, INFO,
					      "Rx Auth4 fail [" MACSTR
					      "], status %d, maybe MIC error\n",
					      MAC2STR(prStaRec->aucMacAddr),
					      u2StatusCode);
					/* Reset Send Auth/(Re)Assoc Frame Count
					 */
					prStaRec->ucTxAuthAssocRetryCount = 0;
					saaFsmSteps(prAdapter, prStaRec,
						    AA_STATE_IDLE,
						    (struct SW_RFB *)NULL);
					break;
				}

				/* Update Station Record - Class 2 Flag */
				cnmStaRecChangeState(prAdapter,
						     prStaRec, STA_STATE_2);

				eNextState = SAA_STATE_SEND_ASSOC1;
			} else {
				DBGLOG(SAA, INFO,
				       "Auth Req was rejected by [" MACSTR
				       "], Status Code = %d\n",
				       MAC2STR(prStaRec->aucMacAddr),
				       u2StatusCode);

				eNextState = AA_STATE_IDLE;
			}

			/* Reset Send Auth/(Re)Assoc Frame Count */
			prStaRec->ucTxAuthAssocRetryCount = 0;

			saaFsmSteps(prAdapter, prStaRec,
				    eNextState, (struct SW_RFB *) NULL);
		}
		break;

	case SAA_STATE_EXTERNAL_AUTH:
	{
		uint32_t u4LinkId = MLD_LINK_ID_NONE;

		if (authCheckRxAuthFrameStatus(prAdapter,
					       prSwRfb,
					       AUTH_TRANSACTION_SEQ_2,
					       &u2StatusCode) ==
					       WLAN_STATUS_SUCCESS) {
			if (u2StatusCode == WLAN_STATUS_SAE_HASH_TO_ELEMENT)
				DBGLOG(SAA, INFO,
					"SAE auth uses the hash-to-element method, instead of looping\n");
			else if (u2StatusCode != STATUS_CODE_SUCCESSFUL) {
				DBGLOG(SAA, INFO,
				       "Auth Req was rejected by [" MACSTR
				       "], Status Code = %d\n",
				       MAC2STR(prStaRec->aucMacAddr),
				       u2StatusCode);
				/* Record join fail status code only*/
				prStaRec->u2StatusCode = u2StatusCode;
			}
		}
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		u4LinkId = (uint32_t)prStaRec->ucLinkIndex;
#endif
		kalIndicateRxMgmtFrame(prAdapter, prAdapter->prGlueInfo,
				prSwRfb, prStaRec->ucBssIndex, u4LinkId);
	}
		break;

	default:
		break;		/* Ignore other cases */
	}
}				/* end of saaFsmRunEventRxAuth() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will process the Rx (Re)Association Response Frame and
 *        then trigger SAA FSM.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS           if the status code was not success
 * @retval WLAN_STATUS_BUFFER_RETAINED   if the status code was success
 */
/*----------------------------------------------------------------------------*/
uint32_t saaFsmRunEventRxAssoc(struct ADAPTER *prAdapter,
			       struct SW_RFB *prSwRfb)
{
	struct STA_RECORD *prStaRec;
	uint16_t u2StatusCode;
	enum ENUM_AA_STATE eNextState;
	struct SW_RFB *prRetainedSwRfb = (struct SW_RFB *) NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucWlanIdx;
	struct WLAN_MAC_MGMT_HEADER *prWlanMgmtHdr;
	uint8_t ucStaRecIdx;

	ucStaRecIdx = prSwRfb->ucStaRecIdx;
	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIdx);
	ucWlanIdx = prSwRfb->ucWlanIdx;

	/* We should have the corresponding Sta Record. */
	if (!prStaRec) {
		struct WLAN_MAC_MGMT_HEADER *mgmt =
			(struct WLAN_MAC_MGMT_HEADER *)prSwRfb->pvHeader;

		DBGLOG(SAA, WARN,
			"Received a AssocResp: DA[" MACSTR "] bssid[" MACSTR
			"] wlanIdx[%d] staRecIdx[%d] w/o corresponding staRec\n",
			MAC2STR(mgmt->aucDestAddr),
			MAC2STR(mgmt->aucBSSID),
			ucWlanIdx,
			ucStaRecIdx);
#if (CFG_WLAN_CONNAC3_DEV == 1)
		prStaRec = saaFsmFindStaRec(prAdapter, mgmt);
		if (!prStaRec) {
			DBGLOG(SAA, WARN, "StaRec not found\n");
			return rStatus;
		}
		prSwRfb->ucStaRecIdx = prStaRec->ucIndex;
		prSwRfb->ucWlanIdx = prStaRec->ucWlanIndex;
#else
		return rStatus;
#endif
	}

	if (!IS_AP_STA(prStaRec))
		return rStatus;

	switch (prStaRec->eAuthAssocState) {
	case SAA_STATE_SEND_ASSOC1:
	case SAA_STATE_WAIT_ASSOC2:
		/* TRUE if the incoming frame is what we are waiting for */
		if (assocCheckRxReAssocRspFrameStatus(prAdapter,
		    prSwRfb, &u2StatusCode) == WLAN_STATUS_SUCCESS) {

			cnmTimerStopTimer(prAdapter,
					  &prStaRec->rTxReqDoneOrRxRespTimer);

			/* Record the Status Code of Authentication Request */
			prStaRec->u2StatusCode = u2StatusCode;
			prRetainedSwRfb = prSwRfb;
			rStatus = WLAN_STATUS_PENDING;

			if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {

				/* Update Station Record - Class 3 Flag */
				/* NOTE(Kevin): Moved to AIS FSM for roaming
				 * issue - We should deactivate the struct
				 * STA_RECORD of previous AP before activate
				 * new one in Driver.
				 */
				/* cnmStaRecChangeState(prStaRec,
				 * STA_STATE_3);
				 */

				/* Clear history. */
				prStaRec->ucJoinFailureCount = 0;
			} else {
				cnmStaRecChangeState(prAdapter, prStaRec,
						STA_STATE_1);
				DBGLOG(SAA, INFO,
				       "Assoc Req was rejected by [" MACSTR
				       "], Status Code = %d\n",
				       MAC2STR(prStaRec->aucMacAddr),
				       u2StatusCode);
			}
			/* Assoc Resp's BSSID doesn't match target, ignore */
			prWlanMgmtHdr = (struct WLAN_MAC_MGMT_HEADER *)
					(prSwRfb->pvHeader);
			if (!EQUAL_MAC_ADDR(prWlanMgmtHdr->aucBSSID,
					    prStaRec->aucMacAddr)) {
				DBGLOG(SAA, ERROR, "Unknown BSSID\n");
				rStatus = WLAN_STATUS_FAILURE;
				prRetainedSwRfb = NULL;
				if (u2StatusCode == STATUS_CODE_SUCCESSFUL)
					u2StatusCode = prStaRec->u2StatusCode =
						STATUS_CODE_UNSPECIFIED_FAILURE;
			}

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
			if (u2StatusCode == STATUS_CODE_SUCCESSFUL &&
			    filsProcessAssocResp(prAdapter, prSwRfb)) {
				DBGLOG(SAA, WARN,
					"Fils process assoc resp failed\n");
				rStatus = WLAN_STATUS_FAILURE;
				prRetainedSwRfb = NULL;
				if (u2StatusCode == STATUS_CODE_SUCCESSFUL)
					prStaRec->u2StatusCode =
						STATUS_CODE_UNSPECIFIED_FAILURE;
			}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (u2StatusCode == STATUS_CODE_SUCCESSFUL &&
			    !mldSanityCheck(prAdapter, prSwRfb->pvHeader,
				prSwRfb->u2PacketLen, prStaRec,
				prStaRec->ucBssIndex)) {
				DBGLOG(SAA, WARN,
					"Discard Assoc frame with wrong ML IE\n");
				rStatus = WLAN_STATUS_FAILURE;
				prRetainedSwRfb = NULL;
				if (u2StatusCode == STATUS_CODE_SUCCESSFUL)
					u2StatusCode = prStaRec->u2StatusCode =
						STATUS_CODE_UNSPECIFIED_FAILURE;
			}
#endif

			/* Reset Send Auth/(Re)Assoc Frame Count */
			prStaRec->ucTxAuthAssocRetryCount = 0;

			/* update RCPI */
			ASSERT(prSwRfb->prRxStatusGroup3);
			prStaRec->ucRCPI =
				nicRxGetRcpiValueFromRxv(
					prAdapter, RCPI_MODE_MAX, prSwRfb);

			eNextState = AA_STATE_IDLE;

			saaFsmSteps(prAdapter, prStaRec,
				    eNextState, prRetainedSwRfb);
		}
		break;

	default:
		break;		/* Ignore other cases */
	}

	return rStatus;

}				/* end of saaFsmRunEventRxAssoc() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the incoming Deauth Frame when State2.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Always not retain deauthentication frames
 */
/*----------------------------------------------------------------------------*/
uint32_t saaFsmStaState2HandleRxDeauth(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec)
{
	uint16_t u2StatusCode = STATUS_CODE_ASSOC_PREV_AUTH_INVALID;

	if (!prStaRec ||
	    prStaRec->ucStaState != STA_STATE_2 ||
	    prStaRec->u2ReasonCode != REASON_CODE_PREV_AUTH_INVALID)
		return WLAN_STATUS_INVALID_DATA;

	prStaRec->u2StatusCode = u2StatusCode;
#if CFG_STAINFO_FEATURE
	prAdapter->u2ConnRejectStatus = u2StatusCode;
#endif

	/* Reset Send Auth/(Re)Assoc Frame Count */
	prStaRec->ucTxAuthAssocRetryCount = 0;

	saaFsmSteps(prAdapter, prStaRec, AA_STATE_IDLE, (struct SW_RFB *) NULL);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check the incoming Deauth Frame.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Always not retain deauthentication frames
 */
/*----------------------------------------------------------------------------*/
uint32_t saaFsmRunEventRxDeauth(struct ADAPTER *prAdapter,
				struct SW_RFB *prSwRfb)
{
	struct STA_RECORD *prStaRec;
	struct WLAN_DEAUTH_FRAME *prDeauthFrame;
	uint8_t ucWlanIdx;
	uint8_t ucStaRecIdx;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	prDeauthFrame = (struct WLAN_DEAUTH_FRAME *) prSwRfb->pvHeader;
	ucWlanIdx = prSwRfb->ucWlanIdx;
	ucStaRecIdx = prSwRfb->ucStaRecIdx;

	DBGLOG(SAA, INFO, "Rx Deauth frame ,DA[" MACSTR "] SA[" MACSTR
	       "] BSSID[" MACSTR "] ReasonCode[0x%x]\n",
	       MAC2STR(prDeauthFrame->aucDestAddr),
	       MAC2STR(prDeauthFrame->aucSrcAddr),
	       MAC2STR(prDeauthFrame->aucBSSID), prDeauthFrame->u2ReasonCode);

#if CFG_ENABLE_WIFI_DIRECT
	if (!prStaRec) {
		secHandleNoWtbl(prAdapter, prSwRfb);
		prStaRec = cnmGetStaRecByIndex(prAdapter,
					       prSwRfb->ucStaRecIdx);
	}
#endif

	if (!prStaRec) {
		DBGLOG(SAA, WARN,
		       "Received a Deauth: wlanIdx[%d] staRecIdx[%d] w/o corresponding staRec\n",
		       ucWlanIdx, ucStaRecIdx);
		goto exit;
	}

	if (IS_STA_IN_AIS(prAdapter, prStaRec)) {
		struct BSS_INFO *prAisBssInfo;
		struct BSS_DESC *prBssDesc;
		uint8_t ucBssIndex = prStaRec->ucBssIndex;

		prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
		prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);

		if (!prAisBssInfo || !IS_AP_STA(prStaRec))
			goto exit;

		if ((UNEQUAL_MAC_ADDR(prStaRec->aucMacAddr,
			prDeauthFrame->aucSrcAddr)) ||
		    (prAisBssInfo && IS_UCAST_MAC_ADDR(
			prDeauthFrame->aucDestAddr) &&
		     UNEQUAL_MAC_ADDR(prAisBssInfo->aucOwnMacAddr,
			prDeauthFrame->aucDestAddr))) {
			DBGLOG(SAA, WARN,
				"Received a Deauth SA[" MACSTR
				"] DA[" MACSTR "] unmatch Peer[" MACSTR
				"] Own[" MACSTR "]\n",
				MAC2STR(prDeauthFrame->aucSrcAddr),
				MAC2STR(prDeauthFrame->aucDestAddr),
				MAC2STR(prStaRec->aucMacAddr),
				MAC2STR(prAisBssInfo->aucOwnMacAddr));
			goto exit;
		}

		/* if state != CONNECTED, don't do disconnect again */
		if (prStaRec->ucStaState != STA_STATE_2 &&
		    kalGetMediaStateIndicated(prAdapter->prGlueInfo,
			ucBssIndex) != MEDIA_STATE_CONNECTED)
			goto exit;

		if (prStaRec->ucStaState > STA_STATE_1) {
#if (CFG_SUPPORT_CONN_LOG == 1)
			connLogRxDeauth(prAdapter,
				prStaRec,
				prDeauthFrame,
				prBssDesc);
#endif

			/* Check if this is the AP we are associated
			 * or associating with
			 */
			if (authProcessRxDeauthFrame(prSwRfb,
				    prStaRec->aucMacAddr,
				    &prStaRec->u2ReasonCode)
					    == WLAN_STATUS_SUCCESS) {

#if CFG_SUPPORT_802_11W
				struct AIS_SPECIFIC_BSS_INFO
						*prAisSpecBssInfo;

				prAisSpecBssInfo =
					aisGetAisSpecBssInfo(prAdapter,
					ucBssIndex);

				DBGLOG(RSN, INFO,
				       "QM RX MGT: Deauth frame, P=%d Sec=%d CM=%d BC=%d fc=%02x wlanIdx=%d, txAllowed=%d\n",
				       prAisSpecBssInfo->
					fgMgmtProtection, (uint8_t)
					prSwRfb->ucSecMode,
					prSwRfb->fgIsCipherMS,
					IS_BMCAST_MAC_ADDR
					(prDeauthFrame->aucDestAddr),
					prDeauthFrame->u2FrameCtrl,
					ucWlanIdx,
					prStaRec->fgIsTxAllowed);

				if (IS_STA_IN_AIS(prAdapter, prStaRec) &&
				    prStaRec->fgIsTxAllowed &&
				    prAisSpecBssInfo->fgMgmtProtection
				    && IS_INCORRECT_SEC_RX_FRAME(
					prSwRfb,
					prDeauthFrame->aucDestAddr,
					prDeauthFrame->u2FrameCtrl)
				    /* HAL_RX_STATUS_GET_SEC_MODE
				     * (prSwRfb->prRxStatus) !=
				     * CIPHER_SUITE_BIP
				     */
				    ) {
					saaChkDeauthfrmParamHandler(
						prAdapter, prSwRfb,
						prStaRec);
					return WLAN_STATUS_SUCCESS;
				}
#endif
				if (saaFsmStaState2HandleRxDeauth(prAdapter,
					prStaRec) == WLAN_STATUS_SUCCESS)
					goto exit;

				saaSendDisconnectMsgHandler(prAdapter,
				      prStaRec,
				      prAisBssInfo,
				      FRM_DEAUTH);
			}
		}
	}
#if CFG_ENABLE_WIFI_DIRECT
	else if (prAdapter->fgIsP2PRegistered &&
		 IS_STA_IN_P2P(prAdapter, prStaRec)) {
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		struct MLD_STA_RECORD *mld_starec;
#endif

#if CFG_AP_80211KVR_INTERFACE
		aaaMulAPAgentStaEventNotify(prStaRec,
			prDeauthFrame->aucBSSID, FALSE);
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		mld_starec = mldStarecGetByStarec(prAdapter, prStaRec);
		if (mld_starec) {
			uint8_t mldsta_idx = mld_starec->ucIdx;
			uint8_t i = 0;

			for (i = 0; i < CFG_STA_REC_NUM; i++) {
				struct STA_RECORD *sta;
				struct BSS_INFO *bss;

				sta = cnmGetStaRecByIndex(prAdapter, i);
				if (!sta ||
				    sta->ucMldStaIndex != mldsta_idx)
					continue;

				bss = GET_BSS_INFO_BY_INDEX(prAdapter,
							    sta->ucBssIndex);
				if (!bss) {
					DBGLOG(SAA, INFO,
						"bss is null (%u)\n",
						sta->ucBssIndex);
					continue;
				}

				prSwRfb->ucStaRecIdx = sta->ucIndex;
				prSwRfb->ucWlanIdx = sta->ucWlanIndex;
				prSwRfb->fgDriverGen = sta != prStaRec;
				COPY_MAC_ADDR(prDeauthFrame->aucDestAddr,
					      bss->aucOwnMacAddr);
				COPY_MAC_ADDR(prDeauthFrame->aucSrcAddr,
					      sta->aucMacAddr);
				COPY_MAC_ADDR(prDeauthFrame->aucBSSID,
					      bss->aucBSSID);

				p2pRoleFsmRunEventRxDeauthentication(prAdapter,
								     sta,
								     prSwRfb);
			}
		} else
#endif
		{
			p2pRoleFsmRunEventRxDeauthentication(prAdapter,
							     prStaRec,
							     prSwRfb);
		}
	}
#endif
#if CFG_ENABLE_BT_OVER_WIFI
	else if (IS_STA_BOW_TYPE(prStaRec))
		bowRunEventRxDeAuth(prAdapter, prStaRec, prSwRfb);
#endif
#if CFG_SUPPORT_NAN
	else if (IS_STA_NAN_TYPE(prStaRec)) {
		DBGLOG(SAA, WARN,
		       "Received a Deauth: wlanIdx[%d] from NAN network\n",
		       ucWlanIdx);
	}
#endif
	else {
		DBGLOG(SAA, WARN, "No handler.\n");
		ASSERT(0);
	}

exit:
	return WLAN_STATUS_SUCCESS;
}				/* end of saaFsmRunEventRxDeauth() */

/* for AOSP */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check param of deauth frame and reson code for
 *        deauth.
 *
 * @param[in]
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/

void saaChkDeauthfrmParamHandler(struct ADAPTER *prAdapter,
				 struct SW_RFB *prSwRfb,
				 struct STA_RECORD *prStaRec)
{
	struct WLAN_DEAUTH_FRAME *prDeauthFrame;
	struct AIS_SPECIFIC_BSS_INFO *prBssSpecInfo;

	prBssSpecInfo = aisGetAisSpecBssInfo(prAdapter, prStaRec->ucBssIndex);

	do {
		prDeauthFrame = (struct WLAN_DEAUTH_FRAME *) prSwRfb->pvHeader;
		if (!IS_BMCAST_MAC_ADDR(prDeauthFrame->aucDestAddr)) {
			/* MFP test plan 5.3.3.5 */
			DBGLOG(RSN, INFO,
				"[%d] QM RX MGT: rsnStartSaQuery\n",
				prStaRec->ucBssIndex);

			COPY_MAC_ADDR(prBssSpecInfo->aucSaQueryBSSID,
					prDeauthFrame->aucBSSID);
			rsnStartSaQuery(prAdapter, prStaRec->ucBssIndex);
		} else {
			DBGLOG(RSN, INFO, "RXM: Drop unprotected Mgmt frame\n");
			DBGLOG(RSN, INFO,
			"RXM:(MACRX Done)RX(ucSecMode=0x%x)(ucWlanIdx=0x%x)\n",
			prSwRfb->ucSecMode, prSwRfb->ucWlanIdx);
		}
	} while (0);
}

/* for AOSP */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check and send disconnect message to AIS module
 *
 * @param[in]
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
void
saaSendDisconnectMsgHandler(struct ADAPTER *prAdapter,
			    struct STA_RECORD *prStaRec,
			    struct BSS_INFO *prAisBssInfo,
			    enum ENUM_AA_FRM_TYPE eFrmType)
{
	if (prStaRec->ucStaState == STA_STATE_3) {
		struct MSG_AIS_ABORT *prAisAbortMsg;
		u_int8_t fgDelayIndication = TRUE;
		struct AIS_FSM_INFO *prAisFsmInfo;

		prAisFsmInfo = aisGetAisFsmInfo(prAdapter,
			prStaRec->ucBssIndex);

		if (!prAisFsmInfo) {
			DBGLOG(SAA, WARN, "prAisFsmInfo[%d] is NULL\n",
					  prStaRec->ucBssIndex);
			return;
		}

#if CFG_SUPPORT_NCHO
		/* Disconnect directly under NCHO mode */
		if (prAdapter && prAdapter->rNchoInfo.fgNCHOEnabled) {
			DBGLOG(RSN, INFO,
				"Disconnect directly due to NCHO enabled\n");
			fgDelayIndication = FALSE;
		}
#endif

		if (timerPendingTimer(&prAisFsmInfo->rJoinTimeoutTimer))
			cnmTimerStopTimer(prAdapter,
				&prAisFsmInfo->rJoinTimeoutTimer);

		/* NOTE(Kevin): Change state immediately to
		 * avoid starvation of MSG buffer because of too
		 * many deauth frames before changing the STA
		 * state.
		 */
		cnmStaRecChangeState(prAdapter, prStaRec,
			eFrmType == FRM_DEAUTH ? STA_STATE_1 : STA_STATE_2);

		prAisAbortMsg =
		    (struct MSG_AIS_ABORT *)
		    cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG,
				sizeof(struct MSG_AIS_ABORT));
		if (!prAisAbortMsg)
			return;

		prAisAbortMsg->rMsgHdr.eMsgId =	MID_SAA_AIS_FSM_ABORT;
		prAisAbortMsg->ucReasonOfDisconnect = eFrmType == FRM_DEAUTH ?
				DISCONNECT_REASON_CODE_DEAUTHENTICATED :
				DISCONNECT_REASON_CODE_DISASSOCIATED;
		prAisAbortMsg->fgDelayIndication = fgDelayIndication;
		prAisAbortMsg->ucBssIndex = prStaRec->ucBssIndex;
		prAisAbortMsg->u2DeauthReason = prStaRec->u2ReasonCode;
		mboxSendMsg(prAdapter, MBOX_ID_0,
			    (struct MSG_HDR *) prAisAbortMsg,
			    MSG_SEND_METHOD_BUF);
	}
	if (prAisBssInfo)
		prAisBssInfo->u2DeauthReason = prStaRec->u2ReasonCode;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check the incoming Disassociation Frame.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Always not retain disassociation frames
 */
/*----------------------------------------------------------------------------*/
uint32_t saaFsmRunEventRxDisassoc(struct ADAPTER *prAdapter,
				  struct SW_RFB *prSwRfb)
{
	struct STA_RECORD *prStaRec;
	struct WLAN_DISASSOC_FRAME *prDisassocFrame;
	uint8_t ucWlanIdx;
	uint8_t ucStaRecIdx;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	prDisassocFrame = (struct WLAN_DISASSOC_FRAME *) prSwRfb->pvHeader;
	ucWlanIdx = prSwRfb->ucWlanIdx;
	ucStaRecIdx = prSwRfb->ucStaRecIdx;

	DBGLOG(SAA, INFO,
	       "Rx Disassoc frame from SA[" MACSTR "] BSSID[" MACSTR
	       "] DA[" MACSTR "] ReasonCode[0x%x]\n",
	       MAC2STR(prDisassocFrame->aucSrcAddr),
	       MAC2STR(prDisassocFrame->aucBSSID),
	       MAC2STR(prDisassocFrame->aucDestAddr),
	       prDisassocFrame->u2ReasonCode);

	/* We should have the corresponding Sta Record. */
	if (!prStaRec) {
		DBGLOG(SAA, WARN,
		       "Received a DisAssoc: wlanIdx[%d] staRecIdx[%d] w/o corresponding staRec\n",
		       ucWlanIdx, ucStaRecIdx);
		goto exit;
	}

	if (IS_STA_IN_AIS(prAdapter, prStaRec)) {
		struct BSS_INFO *prAisBssInfo;
		struct AIS_FSM_INFO *prAisFsmInfo;
		struct BSS_DESC *prBssDesc;
		uint8_t ucBssIndex = 0;

		if (!IS_AP_STA(prStaRec))
			goto exit;

		ucBssIndex = prStaRec->ucBssIndex;

		prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
		prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
		prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);

		if (prBssDesc && UNEQUAL_MAC_ADDR(prBssDesc->aucBSSID,
			prDisassocFrame->aucSrcAddr)) {
			DBGLOG(SAA, WARN,
				"Received a DisAssoc[" MACSTR
				"] unmatch target[" MACSTR "]\n",
				MAC2STR(prDisassocFrame->aucSrcAddr),
				MAC2STR(prBssDesc->aucBSSID));
			goto exit;
		}

		if (prStaRec->ucStaState > STA_STATE_1) {
#if (CFG_SUPPORT_CONN_LOG == 1)
			connLogRxDeassoc(prAdapter,
				prStaRec,
				prDisassocFrame,
				prBssDesc);
#endif

			/* Check if this is the AP we are associated
			 * or associating with
			 */
			if (assocProcessRxDisassocFrame(prAdapter,
			    prSwRfb, prStaRec->aucMacAddr, &prStaRec->
			    u2ReasonCode) == WLAN_STATUS_SUCCESS) {

#if CFG_SUPPORT_802_11W
				struct AIS_SPECIFIC_BSS_INFO
						*prAisSpecBssInfo;

				prAisSpecBssInfo =
					aisGetAisSpecBssInfo(prAdapter,
					ucBssIndex);

				DBGLOG(RSN, INFO,
				       "QM RX MGT: Disassoc frame, P=%d Sec=%d CM=%d BC=%d fc=%02x wlanIdx=%d txAllowed=%d\n",
				       prAisSpecBssInfo->
					fgMgmtProtection, (uint8_t)
					prSwRfb->ucSecMode,
					prSwRfb->fgIsCipherMS,
					IS_BMCAST_MAC_ADDR
					(prDisassocFrame->aucDestAddr),
					prDisassocFrame->u2FrameCtrl,
					ucWlanIdx,
					prStaRec->fgIsTxAllowed);

				if (IS_STA_IN_AIS(prAdapter, prStaRec) &&
				    prStaRec->fgIsTxAllowed &&
				    prAisSpecBssInfo->fgMgmtProtection
				    && IS_INCORRECT_SEC_RX_FRAME(
					prSwRfb,
					prDisassocFrame->aucDestAddr,
					prDisassocFrame->u2FrameCtrl)
				    /* HAL_RX_STATUS_GET_SEC_MODE(
				     * prSwRfb->prRxStatus) !=
				     * CIPHER_SUITE_CCMP
				     */
				    ) {
					/* prDisassocFrame =
					 * (P_WLAN_DISASSOC_FRAME_T)
					 * prSwRfb->pvHeader;
					 */
					saaChkDisassocfrmParamHandler(
					      prAdapter,
					      prDisassocFrame, prStaRec,
					      prSwRfb);
					return WLAN_STATUS_SUCCESS;
				}
#endif
				/* fos_change begin */
#if CFG_SUPPORT_EXCEPTION_STATISTICS
				prAdapter->total_deauth_rx_count++;
				if (prStaRec->u2ReasonCode <=
					REASON_CODE_BEACON_TIMEOUT)
					prAdapter->deauth_rx_count
					[prStaRec->u2ReasonCode]++;
#endif /* fos_change end */
				saaSendDisconnectMsgHandler(prAdapter,
				      prStaRec,
				      prAisBssInfo,
				      FRM_DISASSOC);
			}
		}
	}
#if CFG_ENABLE_WIFI_DIRECT
	else if (prAdapter->fgIsP2PRegistered &&
		 (IS_STA_IN_P2P(prAdapter, prStaRec))) {
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		struct MLD_STA_RECORD *mld_starec;
#endif

#if CFG_AP_80211KVR_INTERFACE
		aaaMulAPAgentStaEventNotify(prStaRec,
			prDisassocFrame->aucBSSID, FALSE);
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		mld_starec = mldStarecGetByStarec(prAdapter, prStaRec);
		if (mld_starec) {
			uint8_t mldsta_idx = mld_starec->ucIdx;
			uint8_t i = 0;

			for (i = 0; i < CFG_STA_REC_NUM; i++) {
				struct STA_RECORD *sta;
				struct BSS_INFO *bss;

				sta = cnmGetStaRecByIndex(prAdapter, i);
				if (!sta ||
				    sta->ucMldStaIndex != mldsta_idx)
					continue;

				bss = GET_BSS_INFO_BY_INDEX(prAdapter,
							    sta->ucBssIndex);
				if (!bss) {
					DBGLOG(SAA, INFO,
						"bss is null (%u)\n",
						sta->ucBssIndex);
					continue;
				}

				prSwRfb->ucStaRecIdx = sta->ucIndex;
				prSwRfb->ucWlanIdx = sta->ucWlanIndex;
				prSwRfb->fgDriverGen = sta != prStaRec;
				COPY_MAC_ADDR(prDisassocFrame->aucDestAddr,
					      bss->aucOwnMacAddr);
				COPY_MAC_ADDR(prDisassocFrame->aucSrcAddr,
					      sta->aucMacAddr);
				COPY_MAC_ADDR(prDisassocFrame->aucBSSID,
					      bss->aucBSSID);

				p2pRoleFsmRunEventRxDisassociation(prAdapter,
								   sta,
								   prSwRfb);
			}
		} else
#endif
		{
			p2pRoleFsmRunEventRxDisassociation(prAdapter,
							   prStaRec,
							   prSwRfb);
		}
	}
#endif
#if CFG_ENABLE_BT_OVER_WIFI
	else if (IS_STA_BOW_TYPE(prStaRec)) {
		/* ToDo:: nothing */
		/* TODO(Kevin) */
	}
#endif
	else {
		DBGLOG(SAA, WARN, "No handler.\n");
		ASSERT(0);
	}

exit:
	return WLAN_STATUS_SUCCESS;
}				/* end of saaFsmRunEventRxDisassoc() */

/* for AOSP */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check param of Disassoc frame and reson code for
 *       Disassoc.
 *
 * @param[in]
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/

void
saaChkDisassocfrmParamHandler(struct ADAPTER *prAdapter,
			      struct WLAN_DISASSOC_FRAME *prDisassocFrame,
			      struct STA_RECORD *prStaRec,
			      struct SW_RFB *prSwRfb)
{
	struct AIS_SPECIFIC_BSS_INFO *prBssSpecInfo;

	prBssSpecInfo = aisGetAisSpecBssInfo(prAdapter, prStaRec->ucBssIndex);

	if (!IS_BMCAST_MAC_ADDR(prDisassocFrame->aucDestAddr)) {
		/* MFP test plan 5.3.3.5 */
		DBGLOG(RSN, INFO,
			"[%d] QM RX MGT: rsnStartSaQuery\n",
			prStaRec->ucBssIndex);

		COPY_MAC_ADDR(prBssSpecInfo->aucSaQueryBSSID,
					prDisassocFrame->aucBSSID);
		rsnStartSaQuery(prAdapter, prStaRec->ucBssIndex);
	} else {
		DBGLOG(RSN, INFO, "RXM: Drop unprotected Mgmt frame\n");
		DBGLOG(RSN, INFO,
			"RXM:(MACRX Done)RX(ucSecMode=0x%x)(ucWlanIdx=0x%x)\n",
			prSwRfb->ucSecMode, prSwRfb->ucWlanIdx);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Abort Event to SAA FSM.
 *
 * @param[in] prMsgHdr   Message of Abort Request for a particular STA.
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventAbort(struct ADAPTER *prAdapter,
			 struct MSG_HDR *prMsgHdr)
{
	struct MSG_SAA_FSM_ABORT *prSaaFsmAbortMsg;
	struct STA_RECORD *prStaRec;

	prSaaFsmAbortMsg = (struct MSG_SAA_FSM_ABORT *) prMsgHdr;
	prStaRec = prSaaFsmAbortMsg->prStaRec;

	if (!prStaRec) {
		cnmMemFree(prAdapter, prMsgHdr);
		return;
	}

	DBGLOG(SAA, LOUD, "EVENT-ABORT: Stop SAA FSM.\n");

	cnmMemFree(prAdapter, prMsgHdr);

	saaFsmStateAbort(prAdapter, prStaRec, (struct SW_RFB *) NULL);

	if (prStaRec->eAuthAssocState != AA_STATE_IDLE) {
		if (prStaRec->eAuthAssocState < AA_STATE_NUM) {
			DBGLOG(SAA, LOUD,
			"EVENT-ABORT: Previous Auth/Assoc State == %s.\n",
			apucDebugAAState[prStaRec->eAuthAssocState]);
		}
	}
#if 0
	/* For the Auth/Assoc State to IDLE */
	prStaRec->eAuthAssocState = AA_STATE_IDLE;
#else
	/* Free this StaRec */
	cnmStaRecFree(prAdapter, prStaRec);
#endif
}				/* end of saaFsmRunEventAbort() */

void saaFsmStateAbort(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec,
			struct SW_RFB *prRetainedSwRfb)
{
	if (prStaRec) {
		/* Free allocated TCM memory */
		if (prStaRec->prChallengeText) {
			cnmMemFree(prAdapter, prStaRec->prChallengeText);
			prStaRec->prChallengeText =
				(struct IE_CHALLENGE_TEXT *) NULL;
		}

		/* Reset Send Auth/(Re)Assoc Frame Count */
		prStaRec->ucTxAuthAssocRetryCount = 0;

		/* Cancel JOIN relative Timer */
		cnmTimerStopTimer(prAdapter,
				&prStaRec->rTxReqDoneOrRxRespTimer);
	}

	if (prRetainedSwRfb)
		nicRxReturnRFB(prAdapter, prRetainedSwRfb);
}

void saaFsmRunEventExternalAuthDone(struct ADAPTER *prAdapter,
				    struct MSG_HDR *prMsgHdr)
{
	struct MSG_SAA_EXTERNAL_AUTH_DONE *prSaaFsmMsg = NULL;
	struct STA_RECORD *prStaRec;
	uint16_t status;

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	prSaaFsmMsg = (struct MSG_SAA_EXTERNAL_AUTH_DONE *)prMsgHdr;
	prStaRec = prSaaFsmMsg->prStaRec;
	status = prSaaFsmMsg->status;

	cnmMemFree(prAdapter, prMsgHdr);

#if CFG_SUPPORT_WPA3_LOG
	wpa3LogExternalAuth(prAdapter,
		prStaRec,
		status);
#endif

	if (prStaRec->eAuthAssocState != SAA_STATE_EXTERNAL_AUTH) {
		DBGLOG(SAA, WARN,
		       "Receive External Auth DONE at wrong state\n");
	} else if (status != WLAN_STATUS_SUCCESS) {
		prStaRec->u2StatusCode = status;
		saaFsmSteps(prAdapter, prStaRec, AA_STATE_IDLE,
			    (struct SW_RFB *)NULL);
	} else {
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);
		saaFsmSteps(prAdapter, prStaRec, SAA_STATE_SEND_ASSOC1,
			    (struct SW_RFB *)NULL);
	}
}				/* end of saaFsmRunEventExternalAuthDone() */

