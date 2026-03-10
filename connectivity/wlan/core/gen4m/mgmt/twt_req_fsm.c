/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2017 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*! \file   "twt_req_fsm.c"
*   \brief  FSM for TWT Requesting STA negotiation
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

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

static uint8_t *apucDebugTWTReqState[TWT_REQ_STATE_NUM] = {
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_IDLE"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_REQTX"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_WAIT_RSP"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_SUSPENDING"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_SUSPENDED"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_RESUMING"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_TEARING_DOWN"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_RX_TEARDOWN"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_RX_INFOFRM"),
#if (CFG_SUPPORT_BTWT == 1)
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_REQTX_BTWT"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_TEARING_DOWN_BTWT"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_RX_TEARDOWN_BTWT"),
#endif
#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_REQTX_ML_TWT_ALL_LINKS"),
	(uint8_t *) DISP_STRING("TWT_REQ_STATE_REQTX_ML_TWT_ONE_BY_ONE"),
#endif
};

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
static uint8_t *apucDebugTWTRespState[TWT_HOTSPOT_RESP_STATE_NUM] = {
	(uint8_t *) DISP_STRING("TWT_HOTSPOT_RESP_STATE_IDLE"),
	(uint8_t *) DISP_STRING("TWT_HOTSPOT_RESP_STATE_RECEIVE_SETUP"),
	(uint8_t *) DISP_STRING("TWT_HOTSPOT_RESP_STATE_SETUP_RESPONSE"),
	(uint8_t *) DISP_STRING("TWT_HOTSPOT_RESP_STATE_RECEIVE_TEARDOWN"),
	(uint8_t *) DISP_STRING("TWT_HOTSPOT_RESP_STATE_SEND_TEARDOWN_TO_STA"),
	(uint8_t *) DISP_STRING("TWT_HOTSPOT_RESP_STATE_DISCONNECT"),
	(uint8_t *) DISP_STRING("TWT_HOTSPOT_RESP_STATE_IDLE_BY_FORCE"),
};
#endif

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static uint32_t
twtReqFsmSendEvent(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	enum ENUM_MSG_ID eMsgId);

static uint32_t
twtReqFsmSendEventRxInfoFrm(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	struct _NEXT_TWT_INFO_T *prNextTWTInfo);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* @brief The Core FSM engine of TWT Requester Module.
*
* @param[in] prStaRec           Pointer to the STA_RECORD_T
* @param[in] eNextState         The value of Next State
* @param[in] prRetainedSwRfb     SW_RFB_T for JOIN Success
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void
twtReqFsmSteps(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _ENUM_TWT_REQUESTER_STATE_T eNextState,
	uint8_t ucTWTFlowId,
	void *pParam)
{
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	enum _ENUM_TWT_REQUESTER_STATE_T ePreState;
	uint8_t fgIsTransition;
#if (CFG_TWT_STA_DIRECT_TEARDOWN == 1)
	uint8_t fgByPassNego = FALSE;
#endif

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prStaRec\n");

		return;
	}

	do {
		if (prStaRec->aeTWTReqState >= TWT_REQ_STATE_NUM ||
			eNextState >= TWT_REQ_STATE_NUM) {
			DBGLOG(TWT_RESPONDER, ERROR,
				"Invalid stat eNextState[%d]\n", eNextState);
			return;
		}

		DBGLOG(TWT_REQUESTER, STATE,
		"[TWT_REQ]BSS %d Flow %d TRANSITION: [%s] -> [%s]\n",
		prStaRec->ucBssIndex,
		ucTWTFlowId,
		apucDebugTWTReqState[prStaRec->aeTWTReqState],
		apucDebugTWTReqState[eNextState]);

		ePreState = prStaRec->aeTWTReqState;

		prStaRec->aeTWTReqState = eNextState;
		fgIsTransition = (uint8_t) FALSE;

		switch (prStaRec->aeTWTReqState) {
		case TWT_REQ_STATE_IDLE:
			/* Notify TWT Planner of the negotiation result */
			if (ePreState == TWT_REQ_STATE_WAIT_RSP) {
				twtReqFsmSendEvent(prAdapter, prStaRec,
					ucTWTFlowId, MID_TWT_REQ_IND_RESULT);
				/* TODO: how to handle failures */
			} else if (ePreState == TWT_REQ_STATE_TEARING_DOWN) {
#if (CFG_TWT_STA_DIRECT_TEARDOWN == 1)
				/* Enable SCAN after TWT agrt has been tear down */
				prAdapter->fgEnOnlineScan = TRUE;
#else
				twtReqFsmSendEvent(prAdapter, prStaRec,
					ucTWTFlowId,
					MID_TWT_REQ_IND_TEARDOWN_DONE);
#endif
			} else if (ePreState == TWT_REQ_STATE_RESUMING) {
				/* At the end of resuming */
			}
#if (CFG_SUPPORT_BTWT == 1)
			else if (ePreState == TWT_REQ_STATE_TEARING_DOWN_BTWT) {
				twtReqFsmSendEvent(prAdapter, prStaRec,
					ucTWTFlowId,
					MID_BTWT_REQ_IND_TEARDOWN_DONE);
			}
#endif
			break;

		case TWT_REQ_STATE_REQTX:
		{
			struct _TWT_PARAMS_T *prTWTParams =
				(struct _TWT_PARAMS_T *)pParam;

			if (!prTWTParams) {
				DBGLOG(TWT_REQUESTER, ERROR,
					"invalid prTWTParams\n");

				return;
			}

			rStatus = twtSendSetupFrame(
				prAdapter, prStaRec, ucTWTFlowId,
				prTWTParams, twtReqFsmRunEventTxDone);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				eNextState = TWT_REQ_STATE_IDLE;
				fgIsTransition = TRUE;
			}
			break;
		}

		case TWT_REQ_STATE_WAIT_RSP:
			break;

		case TWT_REQ_STATE_TEARING_DOWN:
#if (CFG_TWT_STA_DIRECT_TEARDOWN == 1)
			twtPlannerTearingdown(
				prAdapter, prStaRec,
				ucTWTFlowId, &fgByPassNego);

			/*
			* Bypass nego, no need to send
			* teardown frame!!
			*/
			if (fgByPassNego) {
				eNextState = TWT_REQ_STATE_IDLE;
				fgIsTransition = TRUE;

				DBGLOG(TWT_REQUESTER, STATE,
					"[TWT_REQ]BSS %d Flow %d bypass nego\n",
					prStaRec->ucBssIndex,
					ucTWTFlowId);
			} else {
#endif

				rStatus = twtSendTeardownFrame(
					prAdapter, prStaRec, ucTWTFlowId,
					twtReqFsmRunEventTxDone);

				if (rStatus != WLAN_STATUS_SUCCESS) {
					eNextState = TWT_REQ_STATE_IDLE;
					fgIsTransition = TRUE;
				}

#if (CFG_TWT_STA_DIRECT_TEARDOWN == 1)
			}
#endif
			break;

		case TWT_REQ_STATE_SUSPENDING:
		{
			struct _NEXT_TWT_INFO_T rNextTWTInfo = {0};

			rStatus = twtSendInfoFrame(
				prAdapter, prStaRec, ucTWTFlowId, &rNextTWTInfo,
				twtReqFsmRunEventTxDone);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				eNextState = TWT_REQ_STATE_IDLE;
				fgIsTransition = TRUE;
			}
			break;
		}

		case TWT_REQ_STATE_RESUMING:
		{
			struct _NEXT_TWT_INFO_T *prNextTWTInfo =
				(struct _NEXT_TWT_INFO_T *)pParam;

			twtPlannerFillResumeData(
				prAdapter,
				prStaRec,
				ucTWTFlowId,
				prNextTWTInfo->u8NextTWT);

/*
* for TWT inf frame blocked outside TWT service period,
* inform F/W in advance to sending TWT inf frame.
*/
			twtPlannerResumeAgrtTbl(
				prAdapter,
				GET_BSS_INFO_BY_INDEX(prAdapter,
					prStaRec->ucBssIndex),
				prStaRec,
				ucTWTFlowId,
				FALSE,
				NULL, NULL /* handle TWT cmd timeout? */);

			rStatus = twtSendInfoFrame(
				prAdapter, prStaRec, ucTWTFlowId, prNextTWTInfo,
				twtReqFsmRunEventTxDone);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				eNextState = TWT_REQ_STATE_IDLE;
				fgIsTransition = TRUE;
			}
			break;
		}

		case TWT_REQ_STATE_SUSPENDED:
			twtReqFsmSendEvent(prAdapter, prStaRec,
				ucTWTFlowId, MID_TWT_REQ_IND_SUSPEND_DONE);
			break;

		case TWT_REQ_STATE_RX_TEARDOWN:
			twtReqFsmSendEvent(prAdapter, prStaRec,
				ucTWTFlowId, MID_TWT_REQ_IND_TEARDOWN_DONE);
			break;

		case TWT_REQ_STATE_RX_INFOFRM:
		{
			struct _NEXT_TWT_INFO_T *prNextTWTInfo =
				(struct _NEXT_TWT_INFO_T *)pParam;
			twtReqFsmSendEventRxInfoFrm(prAdapter, prStaRec,
				ucTWTFlowId, prNextTWTInfo);
			break;
		}
#if (CFG_SUPPORT_BTWT == 1)
		case TWT_REQ_STATE_REQTX_BTWT:
		{
			struct _TWT_PARAMS_T *prTWTParams =
				(struct _TWT_PARAMS_T *)pParam;

			if (!prTWTParams) {
				DBGLOG(TWT_REQUESTER, ERROR,
					"invalid prTWTParams\n");

				return;
			}

			rStatus = btwtSendSetupFrame(
				prAdapter, prStaRec, ucTWTFlowId,
				prTWTParams, twtReqFsmRunEventTxDone);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				eNextState = TWT_REQ_STATE_IDLE;
				fgIsTransition = TRUE;
			}
			break;
		}

		case TWT_REQ_STATE_TEARING_DOWN_BTWT:
		{
			rStatus = btwtSendTeardownFrame(
				prAdapter, prStaRec, ucTWTFlowId,
				twtReqFsmRunEventTxDone);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				eNextState = TWT_REQ_STATE_IDLE;
				fgIsTransition = TRUE;
			}
			break;
		}

		case TWT_REQ_STATE_RX_TEARDOWN_BTWT:
		{
			twtReqFsmSendEvent(prAdapter, prStaRec,
				ucTWTFlowId, MID_BTWT_REQ_IND_TEARDOWN_DONE);
			break;
		}
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
		case TWT_REQ_STATE_REQTX_ML_TWT_ALL_LINKS:
		{
			struct _TWT_PARAMS_T *prTWTParams =
				(struct _TWT_PARAMS_T *)pParam;

			if (!prTWTParams) {
				DBGLOG(TWT_REQUESTER, ERROR,
					"invalid prTWTParams\n");

				return;
			}

			rStatus = mltwtSendSetupFrameAllInOne(
				prAdapter, prStaRec, ucTWTFlowId,
				prTWTParams, twtReqFsmRunEventTxDone);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				eNextState = TWT_REQ_STATE_IDLE;
				fgIsTransition = TRUE;
			}

			break;
		}

		case TWT_REQ_STATE_REQTX_ML_TWT_ONE_BY_ONE:
		{
			struct _TWT_PARAMS_T *prTWTParams =
				(struct _TWT_PARAMS_T *)pParam;

			if (!prTWTParams) {
				DBGLOG(TWT_REQUESTER, ERROR,
					"invalid prTWTParams\n");

				return;
			}

			rStatus = mltwtSendSetupFramePerLinkDistinct(
				prAdapter, prStaRec, ucTWTFlowId,
				prTWTParams, twtReqFsmRunEventTxDone);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				eNextState = TWT_REQ_STATE_IDLE;
				fgIsTransition = TRUE;
			}

			break;
		}
#endif

		default:
			DBGLOG(TWT_REQUESTER, ERROR,
				"Unknown TWT_REQUESTER STATE\n");

			break;
		}

	} while (fgIsTransition);
}

static uint32_t
twtReqFsmSendEvent(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	enum ENUM_MSG_ID eMsgId)
{
	struct _MSG_TWT_REQFSM_IND_RESULT_T *prTWTFsmResultMsg;

	prTWTFsmResultMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_IND_RESULT_T));
	if (prTWTFsmResultMsg) {
		prTWTFsmResultMsg->rMsgHdr.eMsgId = eMsgId;
		prTWTFsmResultMsg->prStaRec = prStaRec;
		prTWTFsmResultMsg->ucTWTFlowId = ucTWTFlowId;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prTWTFsmResultMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

static uint32_t
twtReqFsmSendEventRxInfoFrm(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	struct _NEXT_TWT_INFO_T *prNextTWTInfo)
{
	struct _MSG_TWT_REQFSM_IND_INFOFRM_T *prTWTFsmInfoFrmMsg;

	prTWTFsmInfoFrmMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct _MSG_TWT_REQFSM_IND_INFOFRM_T));
	if (prTWTFsmInfoFrmMsg) {
		prTWTFsmInfoFrmMsg->rMsgHdr.eMsgId = MID_TWT_REQ_IND_INFOFRM;
		prTWTFsmInfoFrmMsg->prStaRec = prStaRec;
		prTWTFsmInfoFrmMsg->ucTWTFlowId = ucTWTFlowId;
		kalMemCopy(&(prTWTFsmInfoFrmMsg->rNextTWTInfo), prNextTWTInfo,
			sizeof(struct _NEXT_TWT_INFO_T));

		mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prTWTFsmInfoFrmMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Start Event to TWT FSM.
*
* @param[in] prMsgHdr   Message of Request for a particular STA.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void twtReqFsmRunEventStart(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_START_T *prTWTReqFsmStartMsg;
	struct STA_RECORD *prStaRec;
	struct _TWT_PARAMS_T *prTWTParams;
	uint8_t ucTWTFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prMsgHdr\n");

		return;
	}

	prTWTReqFsmStartMsg = (struct _MSG_TWT_REQFSM_START_T *) prMsgHdr;
	prStaRec = prTWTReqFsmStartMsg->prStaRec;
	ucTWTFlowId = prTWTReqFsmStartMsg->ucTWTFlowId;
	prTWTParams = &(prStaRec->arTWTFlow[ucTWTFlowId].rTWTParams);

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);
		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prStaRec\n");

		return;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTParams\n");

		return;
	}

	DBGLOG(TWT_REQUESTER, LOUD,
		"EVENT-START: TWT Requester FSM %d\n", ucTWTFlowId);

	cnmMemFree(prAdapter, prMsgHdr);

	/* Validation of TWT Requester Start Event */
	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"EVENT-START: Invalid Type %d\n",
			prStaRec->eStaType);

		/* TODO: Notify TWT Planner */

		return;
	}

	twtReqFsmSteps(prAdapter, prStaRec,
		TWT_REQ_STATE_REQTX, ucTWTFlowId, prTWTParams);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Teardown Event to TWT FSM.
*
* @param[in] prMsgHdr   Message of Request for a particular STA.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void twtReqFsmRunEventTeardown(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_TEARDOWN_T *prTWTReqFsmTeardownMsg;
	struct STA_RECORD *prStaRec;
	uint8_t ucTWTFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return;
	}


	prTWTReqFsmTeardownMsg = (struct _MSG_TWT_REQFSM_TEARDOWN_T *) prMsgHdr;
	prStaRec = prTWTReqFsmTeardownMsg->prStaRec;
	ucTWTFlowId = prTWTReqFsmTeardownMsg->ucTWTFlowId;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);
		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	DBGLOG(TWT_REQUESTER, LOUD, "EVENT-TEARDOWN: TWT Requester FSM %d\n",
		ucTWTFlowId);

	cnmMemFree(prAdapter, prMsgHdr);

	/* Validation of TWT Requester Teardown Event */
	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_REQUESTER, ERROR, "Invalid STA Type %d\n",
			prStaRec->eStaType);

		/* TODO: Notify TWT Planner */

		return;
	}

	twtReqFsmSteps(prAdapter, prStaRec, TWT_REQ_STATE_TEARING_DOWN,
		ucTWTFlowId, NULL);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Suspend Event to TWT FSM.
*
* @param[in] prMsgHdr   Message of Request for a particular STA.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void twtReqFsmRunEventSuspend(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_SUSPEND_T *prTWTReqFsmSuspendMsg;
	struct STA_RECORD *prStaRec;
	uint8_t ucTWTFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prMsgHdr\n");

		return;
	}

	prTWTReqFsmSuspendMsg = (struct _MSG_TWT_REQFSM_SUSPEND_T *) prMsgHdr;
	prStaRec = prTWTReqFsmSuspendMsg->prStaRec;
	ucTWTFlowId = prTWTReqFsmSuspendMsg->ucTWTFlowId;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_REQUESTER, ERROR,
				"invalid prStaRec\n");

		return;
	}

	DBGLOG(TWT_REQUESTER, LOUD, "EVENT-SUSPEND: TWT Requester FSM %d\n",
		ucTWTFlowId);

	cnmMemFree(prAdapter, prMsgHdr);

	/* Validation of TWT Requester Suspend Event */
	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_REQUESTER, ERROR, "Invalid STA Type %d\n",
			prStaRec->eStaType);

		/* TODO: Notify TWT Planner */

		return;
	}

	twtReqFsmSteps(prAdapter, prStaRec, TWT_REQ_STATE_SUSPENDING,
		ucTWTFlowId, NULL);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Suspend Event to TWT FSM.
*
* @param[in] prMsgHdr   Message of Request for a particular STA.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void twtReqFsmRunEventResume(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_RESUME_T *prTWTReqFsmResumeMsg;
	struct STA_RECORD *prStaRec;
	uint8_t ucTWTFlowId;
	struct _NEXT_TWT_INFO_T rNextTWTInfo;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prMsgHdr\n");

		return;
	}

	prTWTReqFsmResumeMsg = (struct _MSG_TWT_REQFSM_RESUME_T *) prMsgHdr;
	prStaRec = prTWTReqFsmResumeMsg->prStaRec;
	ucTWTFlowId = prTWTReqFsmResumeMsg->ucTWTFlowId;
	rNextTWTInfo.u8NextTWT = prTWTReqFsmResumeMsg->u8NextTWT;
	rNextTWTInfo.ucNextTWTSize = prTWTReqFsmResumeMsg->ucNextTWTSize;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_REQUESTER, ERROR,
				"invalid prStaRec\n");

		return;
	}

	DBGLOG(TWT_REQUESTER, LOUD, "EVENT-RESUME: TWT Requester FSM %d\n",
		ucTWTFlowId);

	DBGLOG(TWT_REQUESTER, WARN,
		"TWT Info Frame 0x%x 0x%x\n",
			rNextTWTInfo.u8NextTWT,
			prTWTReqFsmResumeMsg->u8NextTWT);

	cnmMemFree(prAdapter, prMsgHdr);

	/* Validation of TWT Requester Teardown Event */
	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_REQUESTER, ERROR, "Invalid STA Type %d\n",
			prStaRec->eStaType);

		/* TODO: Notify TWT Planner */
		return;
	}

	twtReqFsmSteps(prAdapter, prStaRec, TWT_REQ_STATE_RESUMING,
		ucTWTFlowId, (void *)&rNextTWTInfo);
}

uint32_t
twtReqFsmRunEventTxDone(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct STA_RECORD *prStaRec;
	enum _ENUM_TWT_REQUESTER_STATE_T eNextState;
	uint8_t ucTWTFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prMsduInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"EVENT-TXDONE: No valid STA Record\n");

		return WLAN_STATUS_INVALID_PACKET;
	}

	if (rTxDoneStatus)
		DBGLOG(TWT_REQUESTER, INFO,
			"EVENT-TX DONE [status: %d][seq: %d]: Current Time = %d\n",
		   rTxDoneStatus, prMsduInfo->ucTxSeqNum, kalGetTimeTick());

	/* Next state is set to current state
	 *by default and check Tx done status to transition if possible
	 */
	eNextState = prStaRec->aeTWTReqState;

	switch (prStaRec->aeTWTReqState) {
	case TWT_REQ_STATE_REQTX:

		if (rTxDoneStatus == TX_RESULT_SUCCESS)
			eNextState = TWT_REQ_STATE_WAIT_RSP;
		else
			eNextState = TWT_REQ_STATE_IDLE;

		ucTWTFlowId = twtGetTxSetupFlowId(prMsduInfo);

		if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
			(ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
			DBGLOG(TWT_REQUESTER, ERROR,
				"TWT_INCORRECT_FLOW_ID %d\n",
				ucTWTFlowId);

			return WLAN_STATUS_INVALID_DATA;
		}

		twtReqFsmSteps(prAdapter,
			prStaRec, eNextState, ucTWTFlowId, NULL);

		break;

	case TWT_REQ_STATE_TEARING_DOWN:

		/*
		* if (rTxDoneStatus == TX_RESULT_SUCCESS)
		* Even it is a failure TWT teardown case,
		* the state machine goes to TWT_REQ_STATE_IDLE
		*/
		eNextState = TWT_REQ_STATE_IDLE;

		ucTWTFlowId = twtGetTxTeardownFlowId(prMsduInfo);

		if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
			(ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
			DBGLOG(TWT_REQUESTER, ERROR,
				"TWT_INCORRECT_FLOW_ID %d\n",
				ucTWTFlowId);

			return WLAN_STATUS_INVALID_DATA;
		}

		twtReqFsmSteps(prAdapter, prStaRec, eNextState,
			ucTWTFlowId, NULL);

		break;

	case TWT_REQ_STATE_SUSPENDING:
		if (rTxDoneStatus == TX_RESULT_SUCCESS) {
			eNextState = TWT_REQ_STATE_SUSPENDED;
			ucTWTFlowId = twtGetTxInfoFlowId(prMsduInfo);

			if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
				(ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
				DBGLOG(TWT_REQUESTER, ERROR,
					"TWT_INCORRECT_FLOW_ID %d\n",
					ucTWTFlowId);

				return WLAN_STATUS_INVALID_DATA;
			}

			twtReqFsmSteps(prAdapter, prStaRec, eNextState,
				ucTWTFlowId, NULL);
		}

		break;

	case TWT_REQ_STATE_RESUMING:
		if (rTxDoneStatus == TX_RESULT_SUCCESS) {
			eNextState = TWT_REQ_STATE_IDLE;
			ucTWTFlowId = twtGetTxInfoFlowId(prMsduInfo);

			if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
				(ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
				DBGLOG(TWT_REQUESTER, ERROR,
					"TWT_INCORRECT_FLOW_ID %d\n",
					ucTWTFlowId);

				return WLAN_STATUS_INVALID_DATA;
			}

			twtReqFsmSteps(prAdapter, prStaRec, eNextState,
				ucTWTFlowId, NULL);
		}

		break;

#if (CFG_SUPPORT_BTWT == 1)
	case TWT_REQ_STATE_REQTX_BTWT:
		if (rTxDoneStatus == TX_RESULT_SUCCESS)
			eNextState = TWT_REQ_STATE_WAIT_RSP;
		else
			eNextState = TWT_REQ_STATE_IDLE;

		ucTWTFlowId = btwtGetTxSetupFlowId(prMsduInfo);

		if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
			(ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
			DBGLOG(TWT_REQUESTER, ERROR,
				"TWT_INCORRECT_FLOW_ID %d\n",
				ucTWTFlowId);

			return WLAN_STATUS_INVALID_DATA;
		}

		twtReqFsmSteps(prAdapter,
			prStaRec, eNextState, ucTWTFlowId, NULL);

		DBGLOG(TWT_REQUESTER, INFO,
		"EVENT-TX DONE flowID= %d\n", ucTWTFlowId);

		break;

	case TWT_REQ_STATE_TEARING_DOWN_BTWT:
		if (rTxDoneStatus == TX_RESULT_SUCCESS)
			eNextState = TWT_REQ_STATE_IDLE;

		ucTWTFlowId = twtGetTxTeardownFlowId(prMsduInfo);

		if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
			(ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
			DBGLOG(TWT_REQUESTER, ERROR,
				"TWT_INCORRECT_FLOW_ID %d\n",
				ucTWTFlowId);

			return WLAN_STATUS_INVALID_DATA;
		}

		twtReqFsmSteps(prAdapter, prStaRec, eNextState,
			ucTWTFlowId, NULL);

		break;
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	case TWT_REQ_STATE_REQTX_ML_TWT_ALL_LINKS:
	case TWT_REQ_STATE_REQTX_ML_TWT_ONE_BY_ONE:
		if (rTxDoneStatus == TX_RESULT_SUCCESS)
			eNextState = TWT_REQ_STATE_WAIT_RSP;
		else
			eNextState = TWT_REQ_STATE_IDLE;

		ucTWTFlowId = twtGetTxSetupFlowId(prMsduInfo);

		if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
			(ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
			DBGLOG(TWT_REQUESTER, ERROR,
				"TWT_INCORRECT_FLOW_ID %d\n",
				ucTWTFlowId);

			return WLAN_STATUS_INVALID_DATA;
		}

		twtReqFsmSteps(prAdapter,
			prStaRec, eNextState, ucTWTFlowId, NULL);

		/*
		 * For MLTWT to follow current TWT setup normal
		 * operation flow, it needs to sync all link's
		 * aeTWTReqState in STA_REC of BSS_INFO in MLD.
		 *
		 * Because twtReqFsmSteps() proceeds acccording
		 * to prStaRec->aeTWTReqState.
		 *
		 * As to TWT teardown, once the STA_REC finishes
		 * TWT setup, prStaRec->aeTWTReqState would just
		 * back to TWT_REQ_STATE_IDLE, teardown would be
		 * fine beginning from TWT_REQ_STATE_IDLE in the
		 * unit of STA_REC of BSS_INFO in MLD.
		 */
		mltwtReqFsmSync(
			prAdapter,
			prStaRec,
			eNextState,
			ucTWTFlowId);

		break;
#endif

	default:
		break;		/* Ignore other cases */
	}

	return WLAN_STATUS_SUCCESS;
}

void twtReqFsmRunEventRxSetup(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId)
{
	if (!IS_AP_STA(prStaRec))
		return;

	switch (prStaRec->aeTWTReqState) {
	case TWT_REQ_STATE_WAIT_RSP:
		/* transition to the IDLE state */
		twtReqFsmSteps(prAdapter,
			prStaRec, TWT_REQ_STATE_IDLE, ucTWTFlowId, NULL);
		break;

	default:
		break;		/* Ignore other cases */
	}
}

void twtReqFsmRunEventRxTeardown(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId)
{
	if (!IS_AP_STA(prStaRec))
		return;

	switch (prStaRec->aeTWTReqState) {
	case TWT_REQ_STATE_IDLE:
#if (CFG_SUPPORT_BTWT == 1)
		if (GET_TWT_TEARDOWN_NEGO(ucTWTFlowId) == 3) {
			twtReqFsmSteps(prAdapter, prStaRec,
				TWT_REQ_STATE_RX_TEARDOWN_BTWT,
				ucTWTFlowId, NULL);
		} else {
#endif
			twtReqFsmSteps(prAdapter, prStaRec,
				TWT_REQ_STATE_RX_TEARDOWN,
				ucTWTFlowId, NULL);
#if (CFG_SUPPORT_BTWT == 1)
		}
#endif

		break;

	default:
		break;		/* Ignore other cases */
	}
}

void twtReqFsmRunEventRxInfoFrm(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	struct _NEXT_TWT_INFO_T *prNextTWTInfo)
{
	if (!IS_AP_STA(prStaRec))
		return;

	switch (prStaRec->aeTWTReqState) {
	case TWT_REQ_STATE_IDLE:
		/* transition to the RX Info frame state */
		twtReqFsmSteps(prAdapter, prStaRec, TWT_REQ_STATE_RX_INFOFRM,
			ucTWTFlowId, prNextTWTInfo);
		break;

	default:
		break;		/* Ignore other cases */
	}
}

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
uint32_t
twtHotspotRespFsmSendEvent(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	enum ENUM_MSG_ID eMsgId)
{
	struct _MSG_TWT_HOTSPOT_PARAMS_SET_T *prTWTHotspotParamSetMsg = NULL;

	prTWTHotspotParamSetMsg =
		cnmMemAlloc(
			prAdapter,
			RAM_TYPE_MSG,
			sizeof(struct _MSG_TWT_HOTSPOT_PARAMS_SET_T));

	if (prTWTHotspotParamSetMsg) {
		prTWTHotspotParamSetMsg->rMsgHdr.eMsgId = eMsgId;
		prTWTHotspotParamSetMsg->rTWTCtrl.prStaRec = prStaRec;
		prTWTHotspotParamSetMsg->rTWTCtrl.ucTWTFlowId = ucTWTFlowId;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prTWTHotspotParamSetMsg,
			MSG_SEND_METHOD_BUF);
	} else
		return WLAN_STATUS_RESOURCES;

	return WLAN_STATUS_SUCCESS;
}

void
twtHotspotRespFsmSteps(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _ENUM_TWT_HOTSPOT_RESPONDER_STATE_T eNextState,
	uint8_t ucTWTFlowId,
	void *pParam)
{
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	enum _ENUM_TWT_HOTSPOT_RESPONDER_STATE_T ePreState;
	uint8_t fgIsTransition;
	struct BSS_INFO *prBssInfo;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	ASSERT(prBssInfo);
	ASSERT(prAdapter);
	ASSERT(prStaRec);

	do {
		if (prBssInfo->aeTWTRespState < 0 ||
			prBssInfo->aeTWTRespState >=
			TWT_HOTSPOT_RESP_STATE_NUM ||
			eNextState < 0 || eNextState >=
			TWT_HOTSPOT_RESP_STATE_NUM) {
			DBGLOG(TWT_RESPONDER, ERROR,
				"Invalid stat eNextState[%d]\n", eNextState);
			return;
		}
		DBGLOG(TWT_RESPONDER, ERROR,
		"[TWT_RESP] Flow %d TRANSITION: [%s] -> [%s]\n",
		ucTWTFlowId,
		apucDebugTWTRespState[prBssInfo->aeTWTRespState],
		apucDebugTWTRespState[eNextState]);

		ePreState = prBssInfo->aeTWTRespState;

		prBssInfo->aeTWTRespState = eNextState;
		fgIsTransition = (uint8_t) FALSE;

		switch (prBssInfo->aeTWTRespState) {
		case TWT_HOTSPOT_RESP_STATE_IDLE:
			if (ePreState ==
				TWT_HOTSPOT_RESP_STATE_SETUP_RESPONSE) {
				if (prStaRec->TWTHotspotCtrl.ucIsReject
					== FALSE) {
					/* Send TWT agrt EXT_CMD to F/W */
					twtHotspotRespFsmSendEvent(
						prAdapter,
						prStaRec,
						ucTWTFlowId,
						MID_TWT_RESP_SETUP_AGRT_TO_FW);
				}
			} else if (
				(ePreState ==
				TWT_HOTSPOT_RESP_STATE_RECEIVE_TEARDOWN) ||
				(ePreState ==
				TWT_HOTSPOT_RESP_STATE_DISCONNECT)) {
				/* Reset the TWT hotspot station node */
				twtHotspotResetStaNode(
					prAdapter,
					prStaRec);

				/* Return the flow ID */
				twtHotspotReturnFlowId(
					prAdapter,
					prStaRec,
					ucTWTFlowId);
			}

			break;

		case TWT_HOTSPOT_RESP_STATE_RECEIVE_SETUP:
			/* We just receive STA's TWT setup request */
			eNextState = TWT_HOTSPOT_RESP_STATE_SETUP_RESPONSE;

			fgIsTransition = TRUE;

			break;

		case TWT_HOTSPOT_RESP_STATE_SETUP_RESPONSE:
		{
			/* Send the TWT setup response frame */
			struct _TWT_HOTSPOT_CTRL_T *pTWTHotspotCtrl =
				(struct _TWT_HOTSPOT_CTRL_T *) pParam;
			struct _TWT_PARAMS_T *prTWTParams =
				(struct _TWT_PARAMS_T *)
				(&pTWTHotspotCtrl->rTWTParams);

			ASSERT(prTWTParams);

			rStatus = twtHotspotSendSetupRespFrame(
				prAdapter, prStaRec, ucTWTFlowId,
				pTWTHotspotCtrl->ucDialogToken,
				prTWTParams, twtHotspotRespFsmRunEventTxDone);

			if (rStatus != WLAN_STATUS_SUCCESS) {
				eNextState = TWT_HOTSPOT_RESP_STATE_IDLE;

				fgIsTransition = TRUE;
			}

			break;
		}

		case TWT_HOTSPOT_RESP_STATE_RECEIVE_TEARDOWN:
		{
			/* TWT hotspot receives STA's teardown frame */
			twtHotspotRespFsmSendEvent(
				prAdapter,
				prStaRec,
				ucTWTFlowId,
				MID_TWT_RESP_TEARDOWN_TO_FW);

			break;
		}

		case TWT_HOTSPOT_RESP_STATE_SEND_TEARDOWN_TO_STA:
			break;

		case TWT_HOTSPOT_RESP_STATE_DISCONNECT:
		{
			/* TWT hotspot disconnect with this STA */
			twtHotspotRespFsmSendEvent(
				prAdapter,
				prStaRec,
				ucTWTFlowId,
				MID_TWT_RESP_TEARDOWN_TO_FW);

			break;
		}

		case TWT_HOTSPOT_RESP_STATE_IDLE_BY_FORCE:
		{
			fgIsTransition = (uint8_t) TRUE;

			eNextState = TWT_HOTSPOT_RESP_STATE_IDLE;

			break;
		}

		default:
			DBGLOG(TWT_RESPONDER, ERROR,
				"[TWT_RESP]Unknown TWT_RESPONDER STATE\n");
			ASSERT(0);

			break;
		}
	} while (fgIsTransition);
}

void
twtHotspotRespFsmRunEventRxSetup(
	struct ADAPTER *prAdapter,
	void *pParam)
{
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	struct _TWT_HOTSPOT_CTRL_T *prTWTCtrl;

	prTWTCtrl = (struct _TWT_HOTSPOT_CTRL_T *)pParam;

	prStaRec = prTWTCtrl->prStaRec;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	ASSERT(prBssInfo);

	if (p2pFuncIsAPMode(prAdapter->rWifiVar
		.prP2PConnSettings[prBssInfo->u4PrivateData])) {
		switch (prBssInfo->aeTWTRespState) {
		case TWT_HOTSPOT_RESP_STATE_IDLE:
			/* transition to the TWT hotspot receive setup state */
			twtHotspotRespFsmSteps(
				prAdapter,
				prStaRec,
				TWT_HOTSPOT_RESP_STATE_RECEIVE_SETUP,
				prTWTCtrl->ucTWTFlowId,
				(void *)prTWTCtrl);

			break;

		default:
			break;
		}
	}
}

u_int32_t
twtHotspotRespFsmRunEventTxDone(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct STA_RECORD *prStaRec;
	enum _ENUM_TWT_HOTSPOT_RESPONDER_STATE_T eNextState;
	uint8_t ucTWTFlowId;
	struct BSS_INFO *prBssInfo;

	ASSERT(prMsduInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prStaRec) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]EVENT-TXDONE: No valid STA Record\n");
		return WLAN_STATUS_INVALID_PACKET;
	}

	if (rTxDoneStatus)
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]EVENT-TX DONE [status: %d][seq: %d]: Current Time = %d\n",
		   rTxDoneStatus, prMsduInfo->ucTxSeqNum, kalGetTimeTick());

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	ASSERT(prBssInfo);

	/* Next state is set to current state
	 *by default and check Tx done status to transition if possible
	 */
	eNextState = prBssInfo->aeTWTRespState;

	switch (prBssInfo->aeTWTRespState) {
	case TWT_HOTSPOT_RESP_STATE_SETUP_RESPONSE:
		eNextState = TWT_HOTSPOT_RESP_STATE_IDLE;

		ucTWTFlowId = twtGetTxSetupFlowId(prMsduInfo);

		twtHotspotRespFsmSteps(prAdapter,
			prStaRec, eNextState, ucTWTFlowId, NULL);

		break;

	default:
		break;		/* Ignore other cases */
	}

	return WLAN_STATUS_SUCCESS;
}
#endif

#if (CFG_SUPPORT_BTWT == 1)
void btwtReqFsmRunEventStart(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_START_T *prTWTReqFsmStartMsg;
	struct STA_RECORD *prStaRec;
	struct _TWT_PARAMS_T *prTWTParams;
	uint8_t ucTWTFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prMsgHdr\n");

		return;
	}

	prTWTReqFsmStartMsg = (struct _MSG_TWT_REQFSM_START_T *) prMsgHdr;
	prStaRec = prTWTReqFsmStartMsg->prStaRec;
	ucTWTFlowId = prTWTReqFsmStartMsg->ucTWTFlowId;
	prTWTParams = &(prStaRec->arTWTFlow[ucTWTFlowId].rTWTParams);

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_REQUESTER, ERROR,
				"invalid prStaRec\n");

		return;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTParams\n");

		return;
	}

	DBGLOG(TWT_REQUESTER, LOUD,
		"EVENT-START: BTWT Requester FSM %d\n", ucTWTFlowId);

	cnmMemFree(prAdapter, prMsgHdr);

	/* Validation of TWT Requester Start Event */
	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"EVENT-START: Invalid Type %d\n",
			prStaRec->eStaType);

		/* TODO: Notify TWT Planner */

		return;
	}

	twtReqFsmSteps(prAdapter, prStaRec,
		TWT_REQ_STATE_REQTX_BTWT, ucTWTFlowId, prTWTParams);
}

void btwtReqFsmRunEventTeardown(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_TEARDOWN_T *prTWTReqFsmTeardownMsg;
	struct STA_RECORD *prStaRec;
	uint8_t ucTWTFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prMsgHdr\n");

		return;
	}

	prTWTReqFsmTeardownMsg = (struct _MSG_TWT_REQFSM_TEARDOWN_T *) prMsgHdr;
	prStaRec = prTWTReqFsmTeardownMsg->prStaRec;
	ucTWTFlowId = prTWTReqFsmTeardownMsg->ucTWTFlowId;

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_REQUESTER, ERROR,
				"ML invalid prStaRec\n");

		return;
	}

	DBGLOG(TWT_REQUESTER, LOUD, "EVENT-TEARDOWN: BTWT Requester FSM %d\n",
		ucTWTFlowId);

	cnmMemFree(prAdapter, prMsgHdr);

	/* Validation of TWT Requester Teardown Event */
	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_REQUESTER, ERROR, "Invalid STA Type %d\n",
			prStaRec->eStaType);

		/* TODO: Notify TWT Planner */

		return;
	}

	twtReqFsmSteps(prAdapter, prStaRec, TWT_REQ_STATE_TEARING_DOWN_BTWT,
		ucTWTFlowId, NULL);
}
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
void mltwtReqFsmRunEventStartAllLinks(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_START_T *prTWTReqFsmStartMsg;
	struct STA_RECORD *prStaRec;
	struct _TWT_PARAMS_T *prTWTParams;
	uint8_t ucTWTFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prMsgHdr\n");

		return;
	}

	prTWTReqFsmStartMsg = (struct _MSG_TWT_REQFSM_START_T *) prMsgHdr;
	prStaRec = prTWTReqFsmStartMsg->prStaRec;
	ucTWTFlowId = prTWTReqFsmStartMsg->ucTWTFlowId;
	prTWTParams = &(prStaRec->arTWTFlow[ucTWTFlowId].rTWTParams);

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_REQUESTER, ERROR,
				"ML invalid prStaRec\n");

		return;
	}


	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prTWTParams\n");

		return;
	}

	DBGLOG(TWT_REQUESTER, LOUD,
		"EVENT-START: ML TWT Requester FSM %d\n", ucTWTFlowId);

	cnmMemFree(prAdapter, prMsgHdr);

	/* Validation of TWT Requester Start Event */
	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"EVENT-START: Invalid Type %d\n",
			prStaRec->eStaType);

		/* TODO: Notify TWT Planner */
		return;
	}

	twtReqFsmSteps(prAdapter, prStaRec,
		TWT_REQ_STATE_REQTX_ML_TWT_ALL_LINKS,
		ucTWTFlowId,
		prTWTParams);
}

void mltwtReqFsmRunEventStart(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _MSG_TWT_REQFSM_START_T *prTWTReqFsmStartMsg;
	struct STA_RECORD *prStaRec;
	struct _TWT_PARAMS_T *prTWTParams;
	uint8_t ucTWTFlowId;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prAdapter\n");

		return;
	}

	if (!prMsgHdr) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prMsgHdr\n");

		return;
	}

	prTWTReqFsmStartMsg = (struct _MSG_TWT_REQFSM_START_T *) prMsgHdr;
	prStaRec = prTWTReqFsmStartMsg->prStaRec;
	ucTWTFlowId = prTWTReqFsmStartMsg->ucTWTFlowId;
	prTWTParams = &(prStaRec->arTWTFlow[ucTWTFlowId].rTWTParams);

	if ((!prStaRec) || (prStaRec->fgIsInUse == FALSE)) {
		cnmMemFree(prAdapter, prMsgHdr);

		if (!prStaRec)
			DBGLOG(TWT_REQUESTER, ERROR,
				"ML invalid prStaRec\n");

		return;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prTWTParams\n");

		return;
	}

	DBGLOG(TWT_REQUESTER, LOUD,
		"EVENT-START: ML TWT Requester FSM %d\n", ucTWTFlowId);

	cnmMemFree(prAdapter, prMsgHdr);

	/* Validation of TWT Requester Start Event */
	if (!IS_AP_STA(prStaRec)) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"EVENT-START: Invalid Type %d\n",
			prStaRec->eStaType);

		/* TODO: Notify TWT Planner */
		return;
	}

	twtReqFsmSteps(prAdapter, prStaRec,
		TWT_REQ_STATE_REQTX_ML_TWT_ONE_BY_ONE,
		ucTWTFlowId,
		prTWTParams);

}

void mltwtReqFsmRunEventRxSetup(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t *pucIE,
	uint16_t u2IELength)
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct STA_RECORD *prStaRecOfAP = NULL;
	struct IE_ML_TWT_T *prMLTWTIE = NULL;
	uint16_t u2Offset;
	uint8_t ucLinkID;
	uint8_t ucTWTFlowId;
	uint16_t u2LinkIdBitMap;

	/* Get MLD_BSS_INFO in MLO connection */
	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prStaRec\n");

		return;
	}

	if (!IS_AP_STA(prStaRec))
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(
					prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prBssInfo\n");

		return;
	}

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid MLD_BSS_INFO\n");

		return;
	}

	IE_FOR_EACH(pucIE, u2IELength, u2Offset)
	{
		DBGLOG(TWT_REQUESTER, WARN,
			"u2IELength %d u2Offset %d\n",
			u2IELength, u2Offset);

		if (IE_ID(pucIE) == ELEM_ID_TWT) {
			prMLTWTIE = (struct IE_ML_TWT_T *)pucIE;

			ucTWTFlowId = twtGetRxSetupFlowId(
					(struct _IE_TWT_T *)prMLTWTIE);

			if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
				(ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
				DBGLOG(TWT_REQUESTER, ERROR,
					"TWT_INCORRECT_FLOW_ID %d\n",
					ucTWTFlowId);

				return;
			}

			u2LinkIdBitMap = prMLTWTIE->u2LinkIdBitmap;

			/* Iterate each link ID */
			for (ucLinkID = 0;
				ucLinkID < ML_TWT_LINK_ID_BITMAP_COUNT;
				ucLinkID++) {
				if ((u2LinkIdBitMap & BIT(ucLinkID))
					== BIT(ucLinkID)) {
					/* Get the BSS INFO of ucLinkID */
					prBssInfo = mldGetBssInfoByLinkID(
								prAdapter,
								prMldBssInfo,
								ucLinkID,
								TRUE);

					if (!prBssInfo) {
						DBGLOG(TWT_REQUESTER, ERROR,
						"MLTWT %d no BSS_INFO of link ID %d\n",
						ucTWTFlowId,
						ucLinkID);

						return;
					}

					prStaRecOfAP = prBssInfo->prStaRecOfAP;

					if (!prStaRecOfAP) {
						DBGLOG(TWT_REQUESTER, ERROR,
						"TWT Flow %d no STA_REC of link ID %d\n",
						ucTWTFlowId,
						ucLinkID);

						return;
					}

					/*
					 * This supposed to be the setup link's
					 * STA_REC of BSS_INFO, only need to
					 * monitor the TWT_REQ_STATE of setup
					 * link would be quiet sufficient, No!
					 * The whole STA_REC of BSS_INFOs in
					 * the MLD must all be synced!!!
					 *
					 * Because the twtReqFsmSteps()
					 * proceeds inaccordance to
					 * TWT_REQ_STATE
					 */
					if (prStaRecOfAP->aeTWTReqState
						!= TWT_REQ_STATE_WAIT_RSP) {
						DBGLOG(TWT_REQUESTER, ERROR,
						"MLTWT RX Setup: BSS %d Flow %d invalid req state %d\n",
						prBssInfo->ucBssIndex,
						ucTWTFlowId,
						prStaRecOfAP->aeTWTReqState);

						return;
					}

					/*
					 * STA_REC of this MLO link transition
					 * to the IDLE state
					 */
					twtReqFsmSteps(prAdapter,
						prStaRecOfAP,
						TWT_REQ_STATE_IDLE,
						ucTWTFlowId, NULL);
				}
			}
		}
	} /* end IE_FOR_EACH(pucIE, u2IELength, u2Offset) */
}

void mltwtReqFsmSync(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _ENUM_TWT_REQUESTER_STATE_T eNextState,
	u_int8_t ucTWTFlowId)
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct LINK *prBssList = NULL;
	struct BSS_INFO *prCurrBssInfo = NULL;
	struct STA_RECORD *prStaRecOfAP = NULL;

	/* Get MLD_BSS_INFO in MLO connection */
	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(
					prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid prBssInfo\n");

		return;
	}


	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML invalid MLD_BSS_INFO\n");

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

		DBGLOG(TWT_REQUESTER, STATE,
			"[MLTWT_STA_SYNC]BSS %d Flow %d: [%s] -> [%s]\n",
			prStaRecOfAP->ucBssIndex,
			ucTWTFlowId,
			apucDebugTWTReqState[prStaRecOfAP->aeTWTReqState],
			apucDebugTWTReqState[eNextState]);

		prStaRecOfAP->aeTWTReqState = eNextState;
	}
}
#endif
