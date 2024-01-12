/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
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
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
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
/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/scan_fsm.c#2
*/

/*! \file   "scan_fsm.c"
*    \brief  This file defines the state transition function for SCAN FSM.
*
*    The SCAN FSM is part of SCAN MODULE and responsible for performing basic SCAN
*    behavior as metioned in IEEE 802.11 2007 11.1.3.1 & 11.1.3.2 .
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
#define INFINITE_PNO_INTERVAL 99

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
UINT_8 g_aucScanChannelNum[SCN_SCAN_DONE_PRINT_BUFFER_LENGTH];
UINT_8 g_aucScanChannelIdleTime[SCN_SCAN_DONE_PRINT_BUFFER_LENGTH];
UINT_8 g_aucScanChannelMDRDY[SCN_SCAN_DONE_PRINT_BUFFER_LENGTH];

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static PUINT_8 apucDebugScanState[SCAN_STATE_NUM] = {
	(PUINT_8) DISP_STRING("IDLE"),
	(PUINT_8) DISP_STRING("SCANNING"),
};

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
/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmSteps(IN P_ADAPTER_T prAdapter, IN ENUM_SCAN_STATE_T eNextState)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	P_MSG_HDR_T prMsgHdr;

	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	do {
		DBGLOG(SCN, STATE, "[SCAN]TRANSITION: [%s] -> [%s]\n",
			apucDebugScanState[prScanInfo->eCurrentState], apucDebugScanState[eNextState]);

		/* NOTE(Kevin): This is the only place to change the eCurrentState(except initial) */
		prScanInfo->eCurrentState = eNextState;

		fgIsTransition = (BOOLEAN) FALSE;

		switch (prScanInfo->eCurrentState) {
		case SCAN_STATE_IDLE:
			/* check for pending scanning requests */
			if (!LINK_IS_EMPTY(&(prScanInfo->rPendingMsgList))) {
				/* load next message from pending list as scan parameters */
				LINK_REMOVE_HEAD(&(prScanInfo->rPendingMsgList), prMsgHdr, P_MSG_HDR_T);

				if (prMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ
				    || prMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ
				    || prMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ
				    || prMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ) {
					scnFsmHandleScanMsg(prAdapter, (P_MSG_SCN_SCAN_REQ) prMsgHdr);

					eNextState = SCAN_STATE_SCANNING;
					fgIsTransition = TRUE;
				} else if (prMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ_V2
					   || prMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ_V2
					   || prMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ_V2
					   || prMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ_V2) {
					scnFsmHandleScanMsgV2(prAdapter, (P_MSG_SCN_SCAN_REQ_V2) prMsgHdr);

					eNextState = SCAN_STATE_SCANNING;
					fgIsTransition = TRUE;
				} else {
					/* should not happen */
					ASSERT(0);
				}

				/* switch to next state */
				cnmMemFree(prAdapter, prMsgHdr);
			}
			break;

		case SCAN_STATE_SCANNING:
			if (prScanParam->fgIsScanV2 == FALSE)
				scnSendScanReq(prAdapter);
			else
				scnSendScanReqV2(prAdapter);
			break;

		default:
			ASSERT(0);
			break;

		}
	} while (fgIsTransition);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        Generate CMD_ID_SCAN_REQ command
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnSendScanReq(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	CMD_SCAN_REQ rCmdScanReq;
	UINT_32 i;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	/* send command packet for scan */
	kalMemZero(&rCmdScanReq, sizeof(CMD_SCAN_REQ));

	rCmdScanReq.ucSeqNum = prScanParam->ucSeqNum;
	rCmdScanReq.ucBssIndex = prScanParam->ucBssIndex;
	rCmdScanReq.ucScanType = (UINT_8) prScanParam->eScanType;
	rCmdScanReq.ucSSIDType = prScanParam->ucSSIDType;

	if (prScanParam->ucSSIDNum == 1) {
		COPY_SSID(rCmdScanReq.aucSSID,
			  rCmdScanReq.ucSSIDLength,
			  prScanParam->aucSpecifiedSSID[0], prScanParam->ucSpecifiedSSIDLen[0]);
	}

	rCmdScanReq.ucChannelType = (UINT_8) prScanParam->eScanChannel;

	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		/* P2P would use:
		 * 1. Specified Listen Channel of passive scan for LISTEN state.
		 * 2. Specified Listen Channel of Target Device of active scan for SEARCH state. (Target != NULL)
		 */
		rCmdScanReq.ucChannelListNum = prScanParam->ucChannelListNum;

		for (i = 0; i < rCmdScanReq.ucChannelListNum; i++) {
			rCmdScanReq.arChannelList[i].ucBand = (UINT_8) prScanParam->arChnlInfoList[i].eBand;

			rCmdScanReq.arChannelList[i].ucChannelNum =
			    (UINT_8) prScanParam->arChnlInfoList[i].ucChannelNum;
		}
	}

	rCmdScanReq.u2ChannelDwellTime = prScanParam->u2ChannelDwellTime;
	rCmdScanReq.u2TimeoutValue = prScanParam->u2TimeoutValue;

	if (prScanParam->u2IELen <= MAX_IE_LENGTH)
		rCmdScanReq.u2IELen = prScanParam->u2IELen;
	else
		rCmdScanReq.u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(rCmdScanReq.aucIE, prScanParam->aucIE, sizeof(UINT_8) * rCmdScanReq.u2IELen);

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SCAN_REQ,
			    TRUE,
			    FALSE,
			    FALSE,
			    NULL,
			    NULL,
			    OFFSET_OF(CMD_SCAN_REQ, aucIE) + rCmdScanReq.u2IELen, (PUINT_8) &rCmdScanReq, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        Generate CMD_ID_SCAN_REQ_V2 command
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnSendScanReqV2(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	CMD_SCAN_REQ_V2 rCmdScanReq;
	UINT_32 i;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	/* send command packet for scan */
	kalMemZero(&rCmdScanReq, sizeof(CMD_SCAN_REQ_V2));

	rCmdScanReq.ucSeqNum = prScanParam->ucSeqNum;
	rCmdScanReq.ucBssIndex = prScanParam->ucBssIndex;
	rCmdScanReq.ucScanType = (UINT_8) prScanParam->eScanType;
	rCmdScanReq.ucSSIDType = prScanParam->ucSSIDType;
	rCmdScanReq.ucSSIDNum = prScanParam->ucSSIDNum;

	for (i = 0; i < prScanParam->ucSSIDNum; i++) {
		COPY_SSID(rCmdScanReq.arSSID[i].aucSsid,
			  rCmdScanReq.arSSID[i].u4SsidLen,
			  prScanParam->aucSpecifiedSSID[i], prScanParam->ucSpecifiedSSIDLen[i]);
	}

	rCmdScanReq.u2ProbeDelayTime = (UINT_8) prScanParam->u2ProbeDelayTime;
	rCmdScanReq.ucChannelType = (UINT_8) prScanParam->eScanChannel;

	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		/* P2P would use:
		 * 1. Specified Listen Channel of passive scan for LISTEN state.
		 * 2. Specified Listen Channel of Target Device of active scan for SEARCH state. (Target != NULL)
		 */
		rCmdScanReq.ucChannelListNum = prScanParam->ucChannelListNum;

		for (i = 0; i < rCmdScanReq.ucChannelListNum; i++) {
			rCmdScanReq.arChannelList[i].ucBand = (UINT_8) prScanParam->arChnlInfoList[i].eBand;

			rCmdScanReq.arChannelList[i].ucChannelNum =
			    (UINT_8) prScanParam->arChnlInfoList[i].ucChannelNum;
		}
	}

	rCmdScanReq.u2ChannelDwellTime = prScanParam->u2ChannelDwellTime;
	rCmdScanReq.u2TimeoutValue = prScanParam->u2TimeoutValue;

	if (prScanParam->u2IELen <= MAX_IE_LENGTH)
		rCmdScanReq.u2IELen = prScanParam->u2IELen;
	else
		rCmdScanReq.u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(rCmdScanReq.aucIE, prScanParam->aucIE, sizeof(UINT_8) * rCmdScanReq.u2IELen);

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SCAN_REQ_V2,
			    TRUE,
			    FALSE,
			    FALSE,
			    NULL,
			    NULL,
			    OFFSET_OF(CMD_SCAN_REQ_V2, aucIE) + rCmdScanReq.u2IELen, (PUINT_8) &rCmdScanReq, NULL, 0);

}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmMsgStart(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;

	ASSERT(prMsgHdr);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	if (prScanInfo->eCurrentState == SCAN_STATE_IDLE) {
		if (prMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ
		    || prMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ
		    || prMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ || prMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ) {
			scnFsmHandleScanMsg(prAdapter, (P_MSG_SCN_SCAN_REQ) prMsgHdr);
		} else if (prMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ_V2
			   || prMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ_V2
			   || prMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ_V2
			   || prMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ_V2) {
			scnFsmHandleScanMsgV2(prAdapter, (P_MSG_SCN_SCAN_REQ_V2) prMsgHdr);
		} else {
			/* should not deliver to this function */
			ASSERT(0);
		}

		cnmMemFree(prAdapter, prMsgHdr);
		scnFsmSteps(prAdapter, SCAN_STATE_SCANNING);
	} else {
		LINK_INSERT_TAIL(&prScanInfo->rPendingMsgList, &prMsgHdr->rLinkEntry);
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmMsgAbort(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_SCN_SCAN_CANCEL prScanCancel;
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	CMD_SCAN_CANCEL rCmdScanCancel;

	ASSERT(prMsgHdr);

	prScanCancel = (P_MSG_SCN_SCAN_CANCEL) prMsgHdr;
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	DBGLOG(AIS, STATE, "Abort Scan [%d - [%d %d] - %d]\n",
		prScanInfo->eCurrentState,
		prScanCancel->ucSeqNum, prScanParam->ucSeqNum,
		prScanParam->fgIsObssScan);

	if (prScanInfo->eCurrentState != SCAN_STATE_IDLE) {
		if (prScanCancel->ucSeqNum == prScanParam->ucSeqNum &&
		    prScanCancel->ucBssIndex == prScanParam->ucBssIndex) {
			/* send cancel message to firmware domain */
			rCmdScanCancel.ucSeqNum = prScanParam->ucSeqNum;
			rCmdScanCancel.ucIsExtChannel = (UINT_8) prScanCancel->fgIsChannelExt;

			wlanSendSetQueryCmd(prAdapter,
					    CMD_ID_SCAN_CANCEL,
					    TRUE,
					    FALSE,
					    FALSE,
					    NULL, NULL, sizeof(CMD_SCAN_CANCEL), (PUINT_8) &rCmdScanCancel, NULL, 0);

			/* generate scan-done event for caller */
			scnFsmGenerateScanDoneMsg(prAdapter,
						  prScanParam->ucSeqNum,
						  prScanParam->ucBssIndex, SCAN_STATUS_CANCELLED);

			/* switch to next pending scan */
			scnFsmSteps(prAdapter, SCAN_STATE_IDLE);
		} else if (prScanParam->fgIsObssScan == TRUE) {
			rlmObssAbortScan(prAdapter);
		} else {
			scnFsmRemovePendingMsg(prAdapter, prScanCancel->ucSeqNum, prScanCancel->ucBssIndex);
		}
	}

	cnmMemFree(prAdapter, prMsgHdr);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief            Scan Message Parsing (Legacy)
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmHandleScanMsg(IN P_ADAPTER_T prAdapter, IN P_MSG_SCN_SCAN_REQ prScanReqMsg)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	UINT_32 i;

	ASSERT(prAdapter);
	ASSERT(prScanReqMsg);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	prScanParam->eScanType = prScanReqMsg->eScanType;
	prScanParam->ucBssIndex = prScanReqMsg->ucBssIndex;
	prScanParam->ucSSIDType = prScanReqMsg->ucSSIDType;
	if (prScanParam->ucSSIDType & (SCAN_REQ_SSID_SPECIFIED | SCAN_REQ_SSID_P2P_WILDCARD)) {
		prScanParam->ucSSIDNum = 1;

		COPY_SSID(prScanParam->aucSpecifiedSSID[0],
			  prScanParam->ucSpecifiedSSIDLen[0], prScanReqMsg->aucSSID, prScanReqMsg->ucSSIDLength);

		/* reset SSID length to zero for rest array entries */
		for (i = 1; i < SCN_SSID_MAX_NUM; i++)
			prScanParam->ucSpecifiedSSIDLen[i] = 0;
	} else {
		prScanParam->ucSSIDNum = 0;

		for (i = 0; i < SCN_SSID_MAX_NUM; i++)
			prScanParam->ucSpecifiedSSIDLen[i] = 0;
	}

	prScanParam->u2ProbeDelayTime = 0;
	prScanParam->eScanChannel = prScanReqMsg->eScanChannel;
	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		if (prScanReqMsg->ucChannelListNum <= MAXIMUM_OPERATION_CHANNEL_LIST)
			prScanParam->ucChannelListNum = prScanReqMsg->ucChannelListNum;
		else
			prScanParam->ucChannelListNum = MAXIMUM_OPERATION_CHANNEL_LIST;

		kalMemCopy(prScanParam->arChnlInfoList,
			   prScanReqMsg->arChnlInfoList, sizeof(RF_CHANNEL_INFO_T) * prScanParam->ucChannelListNum);
	}

	if (prScanReqMsg->u2IELen <= MAX_IE_LENGTH)
		prScanParam->u2IELen = prScanReqMsg->u2IELen;
	else
		prScanParam->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(prScanParam->aucIE, prScanReqMsg->aucIE, prScanParam->u2IELen);

	prScanParam->u2ChannelDwellTime = prScanReqMsg->u2ChannelDwellTime;
	prScanParam->u2TimeoutValue = prScanReqMsg->u2TimeoutValue;
	prScanParam->ucSeqNum = prScanReqMsg->ucSeqNum;

	if (prScanReqMsg->rMsgHdr.eMsgId == MID_RLM_SCN_SCAN_REQ)
		prScanParam->fgIsObssScan = TRUE;
	else
		prScanParam->fgIsObssScan = FALSE;

	prScanParam->fgIsScanV2 = FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief            Scan Message Parsing - V2 with multiple SSID support
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmHandleScanMsgV2(IN P_ADAPTER_T prAdapter, IN P_MSG_SCN_SCAN_REQ_V2 prScanReqMsg)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	UINT_32 i;

	ASSERT(prAdapter);
	ASSERT(prScanReqMsg);
	ASSERT(prScanReqMsg->ucSSIDNum <= SCN_SSID_MAX_NUM);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	prScanParam->eScanType = prScanReqMsg->eScanType;
	prScanParam->ucBssIndex = prScanReqMsg->ucBssIndex;
	prScanParam->ucSSIDType = prScanReqMsg->ucSSIDType;
	prScanParam->ucSSIDNum = prScanReqMsg->ucSSIDNum;

	for (i = 0; i < prScanReqMsg->ucSSIDNum; i++) {
		COPY_SSID(prScanParam->aucSpecifiedSSID[i],
			  prScanParam->ucSpecifiedSSIDLen[i],
			  prScanReqMsg->prSsid[i].aucSsid, (UINT_8) prScanReqMsg->prSsid[i].u4SsidLen);
	}

	prScanParam->u2ProbeDelayTime = prScanReqMsg->u2ProbeDelay;
	prScanParam->eScanChannel = prScanReqMsg->eScanChannel;
	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		if (prScanReqMsg->ucChannelListNum <= MAXIMUM_OPERATION_CHANNEL_LIST)
			prScanParam->ucChannelListNum = prScanReqMsg->ucChannelListNum;
		else
			prScanParam->ucChannelListNum = MAXIMUM_OPERATION_CHANNEL_LIST;

		kalMemCopy(prScanParam->arChnlInfoList,
			   prScanReqMsg->arChnlInfoList, sizeof(RF_CHANNEL_INFO_T) * prScanParam->ucChannelListNum);
	}

	if (prScanReqMsg->u2IELen <= MAX_IE_LENGTH)
		prScanParam->u2IELen = prScanReqMsg->u2IELen;
	else
		prScanParam->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(prScanParam->aucIE, prScanReqMsg->aucIE, prScanParam->u2IELen);

	prScanParam->u2ChannelDwellTime = prScanReqMsg->u2ChannelDwellTime;
	prScanParam->u2TimeoutValue = prScanReqMsg->u2TimeoutValue;
	prScanParam->ucSeqNum = prScanReqMsg->ucSeqNum;

	if (prScanReqMsg->rMsgHdr.eMsgId == MID_RLM_SCN_SCAN_REQ)
		prScanParam->fgIsObssScan = TRUE;
	else
		prScanParam->fgIsObssScan = FALSE;

	prScanParam->fgIsScanV2 = TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief            Remove pending scan request
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmRemovePendingMsg(IN P_ADAPTER_T prAdapter, IN UINT_8 ucSeqNum, IN UINT_8 ucBssIndex)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	P_MSG_HDR_T prPendingMsgHdr, prPendingMsgHdrNext, prRemoveMsgHdr = NULL;
	P_LINK_ENTRY_T prRemoveLinkEntry = NULL;
	BOOLEAN fgIsRemovingScan = FALSE;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	/* traverse through rPendingMsgList for removal */
	LINK_FOR_EACH_ENTRY_SAFE(prPendingMsgHdr,
				 prPendingMsgHdrNext, &(prScanInfo->rPendingMsgList), rLinkEntry, MSG_HDR_T) {
		if (prPendingMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ
		    || prPendingMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ
		    || prPendingMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ
		    || prPendingMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ) {
			P_MSG_SCN_SCAN_REQ prScanReqMsg = (P_MSG_SCN_SCAN_REQ) prPendingMsgHdr;

			if (ucSeqNum == prScanReqMsg->ucSeqNum && ucBssIndex == prScanReqMsg->ucBssIndex) {
				prRemoveLinkEntry = &(prScanReqMsg->rMsgHdr.rLinkEntry);
				prRemoveMsgHdr = prPendingMsgHdr;
				fgIsRemovingScan = TRUE;
			}
		} else if (prPendingMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ_V2
			   || prPendingMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ_V2
			   || prPendingMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ_V2
			   || prPendingMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ_V2) {
			P_MSG_SCN_SCAN_REQ_V2 prScanReqMsgV2 = (P_MSG_SCN_SCAN_REQ_V2) prPendingMsgHdr;

			if (ucSeqNum == prScanReqMsgV2->ucSeqNum && ucBssIndex == prScanReqMsgV2->ucBssIndex) {
				prRemoveLinkEntry = &(prScanReqMsgV2->rMsgHdr.rLinkEntry);
				prRemoveMsgHdr = prPendingMsgHdr;
				fgIsRemovingScan = TRUE;
			}
		}

		if (prRemoveLinkEntry) {
			if (fgIsRemovingScan == TRUE) {
				/* generate scan-done event for caller */
				scnFsmGenerateScanDoneMsg(prAdapter, ucSeqNum, ucBssIndex, SCAN_STATUS_CANCELLED);
			}

			/* remove from pending list */
			LINK_REMOVE_KNOWN_ENTRY(&(prScanInfo->rPendingMsgList), prRemoveLinkEntry);
			cnmMemFree(prAdapter, prRemoveMsgHdr);

			break;
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnEventScanDone(IN P_ADAPTER_T prAdapter, IN P_EVENT_SCAN_DONE prScanDone, BOOLEAN fgIsNewVersion)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	UINT_32	u4ChCnt;
	UINT_32	u4PrintfIdx = 0;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	kalMemZero(g_aucScanChannelNum, SCN_SCAN_DONE_PRINT_BUFFER_LENGTH);
	kalMemZero(g_aucScanChannelIdleTime, SCN_SCAN_DONE_PRINT_BUFFER_LENGTH);
	kalMemZero(g_aucScanChannelMDRDY, SCN_SCAN_DONE_PRINT_BUFFER_LENGTH);

	if (fgIsNewVersion) {
		DBGLOG(SCN, INFO,
		       "scnEventScanDone Version%u!size of ScanDone%zu,ucCompleteChanCount[%u],ucCurrentState%u, u4ScanDurBcnCnt[%u]\n",
		       prScanDone->ucScanDoneVersion, sizeof(EVENT_SCAN_DONE), prScanDone->ucCompleteChanCount,
		       prScanDone->ucCurrentState, prScanDone->u4ScanDurBcnCnt);

		if (prScanDone->ucCurrentState != FW_SCAN_STATE_SCAN_DONE) {
			DBGLOG(SCN, INFO,
			       "FW Scan timeout!generate ScanDone event at State%d complete chan count%d ucChannelListNum%d\n",
			       prScanDone->ucCurrentState, prScanDone->ucCompleteChanCount,
			       prScanParam->ucChannelListNum);

		} else {
			DBGLOG(SCN, INFO, " scnEventScanDone at FW_SCAN_STATE_SCAN_DONE state\n");
		}
	} else {
		DBGLOG(SCN, INFO, "Old scnEventScanDone Version\n");
	}

	/* buffer empty channel information */
	if (prScanParam->eScanChannel == SCAN_CHANNEL_FULL || prScanParam->eScanChannel == SCAN_CHANNEL_2G4) {
		if (prScanDone->ucSparseChannelValid) {
			prScanInfo->fgIsSparseChannelValid = TRUE;
			prScanInfo->rSparseChannel.eBand = (ENUM_BAND_T) prScanDone->rSparseChannel.ucBand;
			prScanInfo->rSparseChannel.ucChannelNum = prScanDone->rSparseChannel.ucChannelNum;
			prScanInfo->ucSparseChannelArrayValidNum = prScanDone->ucSparseChannelArrayValidNum;
			DBGLOG(SCN, INFO, "Detected_Channel_Num = %d\n", prScanInfo->ucSparseChannelArrayValidNum);

			for (u4ChCnt = 0; u4ChCnt < prScanInfo->ucSparseChannelArrayValidNum; u4ChCnt++) {
				prScanInfo->aucChannelNum[u4ChCnt] = prScanDone->aucChannelNum[u4ChCnt];
				prScanInfo->au2ChannelIdleTime[u4ChCnt] = prScanDone->au2ChannelIdleTime[u4ChCnt];
				prScanInfo->aucChannelMDRDYCnt[u4ChCnt] = prScanDone->aucChannelMDRDYCnt[u4ChCnt];

				if (u4PrintfIdx % 10 == 0 && u4PrintfIdx != 0) {
					DBGLOG(SCN, INFO, "Channel  : %s\n", g_aucScanChannelNum);
					DBGLOG(SCN, INFO, "IdleTime : %s\n", g_aucScanChannelIdleTime);
					DBGLOG(SCN, INFO, "MdrdyCnt : %s\n", g_aucScanChannelMDRDY);
					DBGLOG(SCN, INFO,
						"==================================================================================\n");
					kalMemZero(g_aucScanChannelNum, SCN_SCAN_DONE_PRINT_BUFFER_LENGTH);
					kalMemZero(g_aucScanChannelIdleTime, SCN_SCAN_DONE_PRINT_BUFFER_LENGTH);
					kalMemZero(g_aucScanChannelMDRDY, SCN_SCAN_DONE_PRINT_BUFFER_LENGTH);
					u4PrintfIdx = 0;
				}
				kalSnprintf(g_aucScanChannelNum + u4PrintfIdx * 7,
					sizeof(g_aucScanChannelNum) - u4PrintfIdx * 7,
					"%7d", prScanInfo->aucChannelNum[u4ChCnt]);
				kalSnprintf(g_aucScanChannelIdleTime + u4PrintfIdx * 7,
					sizeof(g_aucScanChannelIdleTime) - u4PrintfIdx * 7,
					"%7d", prScanInfo->au2ChannelIdleTime[u4ChCnt]);
				kalSnprintf(g_aucScanChannelMDRDY + u4PrintfIdx * 7,
					sizeof(g_aucScanChannelMDRDY) - u4PrintfIdx * 7,
					"%7d", prScanInfo->aucChannelMDRDYCnt[u4ChCnt]);
				u4PrintfIdx++;
			}

			DBGLOG(SCN, INFO, "Channel  : %s\n", g_aucScanChannelNum);
			DBGLOG(SCN, INFO, "IdleTime : %s\n", g_aucScanChannelIdleTime);
			DBGLOG(SCN, INFO, "MdrdyCnt : %s\n", g_aucScanChannelMDRDY);
		} else {
			prScanInfo->fgIsSparseChannelValid = FALSE;
		}
	}

	if (prScanInfo->eCurrentState == SCAN_STATE_SCANNING && prScanDone->ucSeqNum == prScanParam->ucSeqNum) {
		/* generate scan-done event for caller */
		scnFsmGenerateScanDoneMsg(prAdapter, prScanParam->ucSeqNum, prScanParam->ucBssIndex, SCAN_STATUS_DONE);

		/* switch to next pending scan */
		scnFsmSteps(prAdapter, SCAN_STATE_IDLE);
	} else {
		DBGLOG(SCN, INFO, "Unexpected SCAN-DONE event: SeqNum = %d, Current State = %d\n",
		       prScanDone->ucSeqNum, prScanInfo->eCurrentState);
	}
}				/* end of scnEventScanDone */

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID
scnFsmGenerateScanDoneMsg(IN P_ADAPTER_T prAdapter,
			  IN UINT_8 ucSeqNum, IN UINT_8 ucBssIndex, IN ENUM_SCAN_STATUS eScanStatus)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	P_MSG_SCN_SCAN_DONE prScanDoneMsg;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	prScanDoneMsg = (P_MSG_SCN_SCAN_DONE) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_SCN_SCAN_DONE));
	if (!prScanDoneMsg) {
		ASSERT(0);	/* Can't indicate SCAN FSM Complete */
		return;
	}

	if (prScanParam->fgIsObssScan == TRUE) {
		prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_RLM_SCAN_DONE;
	} else {
		switch (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType) {
		case NETWORK_TYPE_AIS:
			prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_AIS_SCAN_DONE;
			break;

		case NETWORK_TYPE_P2P:
			prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_P2P_SCAN_DONE;
			break;

		case NETWORK_TYPE_BOW:
			prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_BOW_SCAN_DONE;
			break;

		default:
			DBGLOG(SCN, LOUD,
			       "Unexpected Network Type: %d\n",
			       GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType);
			ASSERT(0);
			break;
		}
	}

	prScanDoneMsg->ucSeqNum = ucSeqNum;
	prScanDoneMsg->ucBssIndex = ucBssIndex;
	prScanDoneMsg->eScanStatus = eScanStatus;

	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prScanDoneMsg, MSG_SEND_METHOD_BUF);

}				/* end of scnFsmGenerateScanDoneMsg() */

/*----------------------------------------------------------------------------*/
/*!
* \brief        Query for most sparse channel
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnQuerySparseChannel(IN P_ADAPTER_T prAdapter, P_ENUM_BAND_T prSparseBand, PUINT_8 pucSparseChannel)
{
	P_SCAN_INFO_T prScanInfo;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (prScanInfo->fgIsSparseChannelValid == TRUE) {
		if (prSparseBand)
			*prSparseBand = prScanInfo->rSparseChannel.eBand;

		if (pucSparseChannel)
			*pucSparseChannel = prScanInfo->rSparseChannel.ucChannelNum;

		return TRUE;
	} else {
		return FALSE;
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        Event handler for NLO done event
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnEventNloDone(IN P_ADAPTER_T prAdapter, IN P_EVENT_NLO_DONE_T prNloDone)
{
	P_SCAN_INFO_T prScanInfo;
	P_NLO_PARAM_T prNloParam;
	P_SCAN_PARAM_T prScanParam;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prNloParam = &prScanInfo->rNloParam;
	prScanParam = &prNloParam->rScanParam;

	if (prScanInfo->fgNloScanning == TRUE && prNloDone->ucSeqNum == prScanParam->ucSeqNum) {

		DBGLOG(SCN, INFO, "scnEventNloDone reporting to uplayer\n");

		kalSchedScanResults(prAdapter->prGlueInfo);

		if (prNloParam->fgStopAfterIndication == TRUE)
			prScanInfo->fgNloScanning = FALSE;
	} else {
		DBGLOG(SCN, INFO, "Unexpected NLO-DONE event: SeqNum = %d, Current State = %d\n",
		       prNloDone->ucSeqNum, prScanInfo->eCurrentState);
	}
}

#if CFG_SUPPORT_SCHED_SCAN
/*----------------------------------------------------------------------------*/
/*!
* \brief        Event handler for NLO done event
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnEventSchedScanDone(IN P_ADAPTER_T prAdapter,
			IN struct EVENT_SCHED_SCAN_DONE_T *prSchedScanDone)
{
	P_SCAN_INFO_T prScanInfo;
	struct SCHED_SCAN_INFO_T *prSchedScanInfo;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prSchedScanInfo = &prScanInfo->rSchedScanInfo;

	if (prScanInfo->fgIsSchedScanning == TRUE
		&& prSchedScanInfo->ucSeqNum == prSchedScanDone->ucSeqNum) {
		DBGLOG(SCN, INFO, "sched scan done, reporting to uplayer\n");
		kalSchedScanResults(prAdapter->prGlueInfo);
	} else {
		DBGLOG(SCN, WARN,
			"Unexpected SchedScan-DONE event: SeqNum = %d, Current State = %d\n",
			prSchedScanDone->ucSeqNum, prScanInfo->eCurrentState);
	}
}


/*----------------------------------------------------------------------------*/
/*!
* \brief        OID handler for starting scheduled scan
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN
scnFsmSchedScanRequest(IN P_ADAPTER_T prAdapter,
			IN P_PARAM_SCHED_SCAN_REQUEST prRequest)
{
	P_SCAN_INFO_T prScanInfo;
	struct SCHED_SCAN_INFO_T *prSchedScanInfo;
	struct CMD_SCHED_SCAN_REQ_T *prSchedScanCmd = NULL;
	UINT_32 i;
	UINT_16 u2IeLen;
	struct SSID_MATCH_SETS *prMatchSets = NULL;
	P_PARAM_SSID_T prSsid = NULL;
	UINT_32 rStatus;

	ASSERT(prAdapter);

	DBGLOG(SCN, INFO, "scnFsmSchedScanRequest\n");

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prSchedScanInfo = &prScanInfo->rSchedScanInfo;

	if (prScanInfo->fgIsSchedScanning) {
		DBGLOG(SCN, WARN,
			"Sched scan is already running, abandon this req\n");
		return FALSE;
	}

	prScanInfo->fgIsSchedScanning = TRUE;

	/* 0. allocate memory for schedule scan command */
	if (prRequest->u4IELength <= MAX_IE_LENGTH)
		u2IeLen = (UINT_16)prRequest->u4IELength;
	else
		u2IeLen = MAX_IE_LENGTH;

	prSchedScanCmd = (struct CMD_SCHED_SCAN_REQ_T *) cnmMemAlloc(prAdapter,
		RAM_TYPE_BUF, sizeof(struct CMD_SCHED_SCAN_REQ_T) + u2IeLen);
	if (!prSchedScanCmd) {
		DBGLOG(SCN, ERROR, "alloc CMD_SCHED_SCAN_REQ (%zu+%u) fail\n",
			sizeof(struct CMD_SCHED_SCAN_REQ_T), u2IeLen);
		return FALSE;
	}
	kalMemZero(prSchedScanCmd,
		sizeof(struct CMD_SCHED_SCAN_REQ_T) + u2IeLen);
	prMatchSets = &(prSchedScanCmd->auMatchSsid[0]);
	prSsid = &(prSchedScanCmd->auSsid[0]);

	/* 1 Set Sched scan param parameters */
	prSchedScanInfo->ucSeqNum++;
	prSchedScanInfo->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;

	if (!IS_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex)) {
		SET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
		/* sync with firmware */
		nicActivateNetwork(prAdapter,
			prAdapter->prAisBssInfo->ucBssIndex);
	}

	/* 2.1 Prepare command. Set FW struct SSID_MATCH_SETS */
	/* ssid in ssid list will be send in probe request in advance */
	prSchedScanCmd->ucSsidNum = prRequest->u4SsidNum;
	for (i = 0; i < prSchedScanCmd->ucSsidNum; i++) {
		kalMemCopy(&(prSsid[i]), &(prRequest->arSsid[i]),
			sizeof(PARAM_SSID_T));
		DBGLOG(SCN, TRACE, "ssid set(%d) %s\n", i, prSsid[i].aucSsid);
	}

	prSchedScanCmd->ucMatchSsidNum = prRequest->u4MatchSsidNum;
	for (i = 0; i < prSchedScanCmd->ucMatchSsidNum; i++) {
		COPY_SSID(prMatchSets[i].aucSsid, prMatchSets[i].ucSsidLen,
			prRequest->arMatchSsid[i].aucSsid,
			prRequest->arMatchSsid[i].u4SsidLen);
		prMatchSets[i].i4RssiThresold = prRequest->ai4RssiThold[i];
		DBGLOG(SCN, TRACE, "Match set(%d) %s, rssi>%d\n",
				i, prMatchSets[i].aucSsid,
				prMatchSets[i].i4RssiThresold);
	}

	/* 2.2 Prepare command. Set channel */
	if (prRequest->ucChnlNum > 0 &&
		prRequest->ucChnlNum <= MAXIMUM_OPERATION_CHANNEL_LIST) {
		prSchedScanCmd->ucChannelType =
			SCHED_SCAN_CHANNEL_TYPE_SPECIFIED;
		prSchedScanCmd->ucChnlNum = prRequest->ucChnlNum;
		for (i = 0; i < prRequest->ucChnlNum; i++) {
			prSchedScanCmd->aucChannel[i].ucChannelNum =
				prRequest->pucChannels[i];
			prSchedScanCmd->aucChannel[i].ucBand =
				(prSchedScanCmd->aucChannel[i].ucChannelNum <=
				HW_CHNL_NUM_MAX_2G4) ? BAND_2G4 : BAND_5G;
		}
	} else {
		prSchedScanCmd->ucChannelType =
					SCHED_SCAN_CHANNEL_TYPE_DUAL_BAND;
		prSchedScanCmd->ucChnlNum = 0;
	}

	prSchedScanCmd->ucSeqNum = prSchedScanInfo->ucSeqNum;
	prSchedScanCmd->u2IELen = u2IeLen;
	prSchedScanCmd->ucVersion = SCHED_SCAN_CMD_VERSION;
	if (prSchedScanCmd->u2IELen) {
		kalMemCopy(prSchedScanCmd->aucIE, prRequest->pucIE,
				prSchedScanCmd->u2IELen);
	}

	/* Set Multiple Scan Plan here */
	DBGLOG(SCN, TRACE, "set multi scan plan\n");

	prSchedScanCmd->ucMspEntryNum = 0;
	kalMemZero(prSchedScanCmd->au2MspList,
			sizeof(prSchedScanCmd->au2MspList));

	/* if customized the ucCusMspEntryNum, */
	/* update the value and prepare send to fw */
	if (prRequest->ucMspEntryNum != 0) {
		prSchedScanCmd->ucMspEntryNum = prRequest->ucMspEntryNum;
		if (prRequest->ucMspEntryNum == INFINITE_PNO_INTERVAL)
			prSchedScanCmd->au2MspList[0] =
				prRequest->au2MspList[0];
		else {
			kalMemCopy(prSchedScanCmd->au2MspList,
				prRequest->au2MspList,
				sizeof(prSchedScanCmd->au2MspList));
		}
		DBGLOG(SCN, ERROR,
			"ucMspEntryNum is %d and first interval is %d\n",
			prSchedScanCmd->ucMspEntryNum,
			prSchedScanCmd->au2MspList[0]);
	}

	DBGLOG(SCN, INFO,
		"V(%u)seq(%u)sz(%zu)chT(%u)chN(%u)ssid(%u)match(%u)IE(%u=>%u)MSP(%u)\n",
		prSchedScanCmd->ucVersion,
		prSchedScanCmd->ucSeqNum, sizeof(struct CMD_SCHED_SCAN_REQ_T),
		prSchedScanCmd->ucChannelType, prSchedScanCmd->ucChnlNum,
		prSchedScanCmd->ucSsidNum, prSchedScanCmd->ucMatchSsidNum,
		prRequest->u4IELength, prSchedScanCmd->u2IELen,
		prSchedScanCmd->ucMspEntryNum);

	/* 3. send command packet to FW */
	rStatus = wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SET_NLO_REQ,
			TRUE,
			FALSE,
			TRUE,
			nicCmdEventSetCommon,
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_SCHED_SCAN_REQ_T) +
				prSchedScanCmd->u2IELen,
			(PUINT_8) prSchedScanCmd, NULL, 0);

	if (rStatus == WLAN_STATUS_FAILURE) {
		DBGLOG(SCN, ERROR,
			"wlanSendSetQueryCmd CMD_ID_SET_NLO_REQ failed.\n");
		prScanInfo->fgIsSchedScanning = FALSE;
	}
	cnmMemFree(prAdapter, (PVOID) prSchedScanCmd);

	return rStatus == WLAN_STATUS_FAILURE ? FALSE : TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        OID handler for stopping scheduled scan
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnFsmSchedScanStopRequest(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	struct SCHED_SCAN_INFO_T *prSchedScanInfo;
	struct CMD_SCAN_SCHED_CANCEL_T rCmdSchedScanCancel;
	WLAN_STATUS rStatus;

	ASSERT(prAdapter);
	DBGLOG(SCN, INFO, "scnFsmSchedScanStopRequest\n");

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prSchedScanInfo = &prScanInfo->rSchedScanInfo;

	if (prScanInfo->fgIsSchedScanning == FALSE)
		DBGLOG(SCN, WARN,
		"Sched scan is not running, but recv stop req...\n");

	/* send cancel message to firmware domain */
	rCmdSchedScanCancel.ucSeqNum = prSchedScanInfo->ucSeqNum;

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_NLO_CANCEL,
					TRUE, FALSE, TRUE,
					nicCmdEventSetCommon,
					nicOidCmdTimeoutCommon,
					sizeof(struct CMD_SCAN_SCHED_CANCEL_T),
					(PUINT_8) & rCmdSchedScanCancel,
					NULL, 0);

	if (rStatus != WLAN_STATUS_FAILURE) {
		prScanInfo->fgIsSchedScanning = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}
#endif
