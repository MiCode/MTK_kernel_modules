// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file epcs.c
 *    \brief This file includes IEEE802.11be TDLS support.
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

#if (CFG_SUPPORT_802_11BE_MLO == 1)
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
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
PFN_TX_DONE_HANDLER epcsTxDoneHandler[PROTECTED_EHT_ACTION_NUM] = {
	[EPCS_ENABLE_REQUEST] = epcsReqTxDoneCb,
	[EPCS_ENABLE_RESPONSE] = epcsRspTxDoneCb,
	[EPCS_TEARDOWN] = epcsTeardownTxDoneCb,
};

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will handle TxDone Event of EPCS action frames.
 *
 * @param[in] prAdapter      Pointer to the Adapter structure.
 * @param[in] prMsduInfo     Pointer to the MSDU_INFO_T.
 * @param[in] rTxDoneStatus  Return TX status of the EPCS action frame.
 *
 * @retval WLAN_STATUS_SUCCESS
 */
/*---------------------------------------------------------------------------*/
uint32_t epcsReqTxDoneCb(struct ADAPTER *prAdapter,
	      struct MSDU_INFO *prMsduInfo,
	      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct EVENT_TX_DONE *prTxDone;
	struct STA_RECORD *prStaRec;
	struct MLD_STA_RECORD *prMldStaRec;

	prTxDone = prMsduInfo->prTxDone;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	if (!prMldStaRec)
		return WLAN_STATUS_FAILURE;

	DBGLOG(TX, DEBUG,
		"EPCS TX DONE, WIDX:PID:SN[%u:%u:%d] Status[%u], SeqNo: %d\n",
		prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
		prTxDone ? prTxDone->u2SequenceNumber : -1,
		rTxDoneStatus,
		prMsduInfo->ucTxSeqNum);

	if (rTxDoneStatus != TX_RESULT_SUCCESS)
		prMldStaRec->fgEPCS = 0;

	return WLAN_STATUS_SUCCESS;
}

uint32_t epcsRspTxDoneCb(struct ADAPTER *prAdapter,
	      struct MSDU_INFO *prMsduInfo,
	      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct EVENT_TX_DONE *prTxDone;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct MLD_STA_RECORD *prMldStaRec;
	struct MLD_BSS_INFO *prMldBssInfo;

	prTxDone = prMsduInfo->prTxDone;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	if (!prMldStaRec || !prMldBssInfo)
		return WLAN_STATUS_FAILURE;

	DBGLOG(TX, DEBUG,
		"EPCS TX DONE, WIDX:PID:SN[%u:%u:%d] Status[%u], SeqNo: %d\n",
		prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
		prTxDone ? prTxDone->u2SequenceNumber : -1,
		rTxDoneStatus,
		prMsduInfo->ucTxSeqNum);
	cnmTimerStopTimer(prAdapter, &prMldStaRec->rEpcsTimer);

	if (rTxDoneStatus != TX_RESULT_SUCCESS &&
			prMldStaRec->ucEpcsRspRetryCnt > EPCS_RETRY_LIMIT) {
		prMldStaRec->fgEPCS = 0;
		prMldStaRec->ucEpcsRspRetryCnt = 0;
		/* Restore BSS (MU)Edca param parsed in Req from AP */
		epcsMldMUnEdcaBackupRestore(prMldBssInfo, FALSE);
	} else if (rTxDoneStatus != TX_RESULT_SUCCESS) {
		prMldStaRec->ucEpcsRspRetryCnt++;
		epcsSend(prAdapter, EPCS_ENABLE_RESPONSE, prBssInfo);
	} else {
		prMldStaRec->fgEPCS = 1;
		/* Update (MU)EDCA param parsed in Req from AP */
		epcsMldMUnEdcaUpdate(prAdapter, prMldBssInfo);
		epcsMldStaRecUpdate(prAdapter, prMldBssInfo);
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t epcsTeardownTxDoneCb(struct ADAPTER *prAdapter,
	      struct MSDU_INFO *prMsduInfo,
	      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct EVENT_TX_DONE *prTxDone;
	struct STA_RECORD *prStaRec;
	struct MLD_STA_RECORD *prMldStaRec;

	prTxDone = prMsduInfo->prTxDone;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	if (!prMldStaRec)
		return WLAN_STATUS_FAILURE;

	DBGLOG(TX, DEBUG,
		"EPCS TX DONE, WIDX:PID:SN[%u:%u:%d] Status[%u], SeqNo: %d\n",
		prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
		prTxDone ? prTxDone->u2SequenceNumber : -1,
		rTxDoneStatus,
		prMsduInfo->ucTxSeqNum);

	if (rTxDoneStatus != TX_RESULT_SUCCESS)
		prMldStaRec->fgEPCS = 0;

	return WLAN_STATUS_SUCCESS;
}

void epcsTimeout(struct ADAPTER *prAdapter, uintptr_t ulParamPtr)
{
	struct MLD_STA_RECORD *prMldStaRec;
	struct MLD_BSS_INFO *prMldBssInfo;

	prMldStaRec = (struct MLD_STA_RECORD *) ulParamPtr;
	prMldBssInfo = mldBssGetByIdx(prAdapter, prMldStaRec->ucGroupMldId);
	if (!prMldBssInfo)
		return;

	if (prMldStaRec->fgEPCS) {
		epcsMldMUnEdcaBackupRestore(prMldBssInfo, FALSE);
		epcsMldMUnEdcaUpdate(prAdapter, prMldBssInfo);
	}
	prMldStaRec->fgEPCS = 0;
	epcsMldStaRecUpdate(prAdapter, prMldBssInfo);
}

void epcsComposeReq(struct MSDU_INFO *prMsduInfo, uint8_t ucDialogToken)
{
	struct ACTION_EPCS_REQ_FRAME *prTxFrame;

	prTxFrame = prMsduInfo->prPacket;
	prTxFrame->ucDialogToken = ucDialogToken;
}

void epcsComposeRsp(struct MSDU_INFO *prMsduInfo, uint8_t ucDialogToken,
		uint16_t u2StatusCode)
{
	struct ACTION_EPCS_RSP_FRAME *prTxFrame;

	prTxFrame = prMsduInfo->prPacket;
	prTxFrame->ucDialogToken = ucDialogToken;
	WLAN_SET_FIELD_16(&prTxFrame->u2StatusCode, u2StatusCode);
}
/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will enqueue EPCS action frame for TX.
 *
 * @param     prAdapter      Pointer to the Adapter structure.
 * @param     eAction        EPCS type requested to send
 *                           valid action code: EPCS_ENABLE_REQUEST [3]
 *                                              EPCS_ENABLE_RESPONSE [4]
 *                                              EPCS_TEARDOWN [5]
 *
 * @retval WLAN_STATUS_SUCCESS
 */
/*---------------------------------------------------------------------------*/
uint32_t epcsSend(struct ADAPTER *prAdapter, enum PROTECTED_EHT_ACTION eAction,
		struct BSS_INFO *prBssInfo)
{
	struct MSDU_INFO *prMsduInfo;
	struct WLAN_ACTION_FRAME *prTxFrame;
	struct STA_RECORD *prStaRec;
	struct MLD_STA_RECORD *prMldStaRec;
	struct MLD_BSS_INFO *prMldBssInfo;
	struct BSS_DESC *prBssDesc;
	uint16_t u2EstimatedFrameLen;
	uint16_t u2FrameLen = 0;


	if (!prBssInfo || !prAdapter || !IS_BSS_AIS(prBssInfo))
		return WLAN_STATUS_FAILURE;

	prBssDesc = aisGetTargetBssDesc(prAdapter, prBssInfo->ucBssIndex);
	prStaRec = aisGetStaRecOfAP(prAdapter, prBssInfo->ucBssIndex);
	if (!prBssDesc || !prStaRec)
		return WLAN_STATUS_FAILURE;

	if (!prBssDesc->fgEpcsCap) {
		DBGLOG(TX, ERROR,
			"AP does not support EPCS\n");
		return WLAN_STATUS_FAILURE;
	}

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldStaRec || !prMldBssInfo) {
		DBGLOG(TX, ERROR,
			"prMldStaRec or prMldBssInfo equal to NULL\n");
		return WLAN_STATUS_FAILURE;
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

	switch (eAction) {
	case EPCS_ENABLE_REQUEST:
		prMldStaRec->ucEpcsTxDialogToken++;
		epcsComposeReq(prMsduInfo, prMldStaRec->ucEpcsTxDialogToken);
		u2FrameLen += OFFSET_OF(struct ACTION_EPCS_REQ_FRAME,
					aucMultiLink);
		break;
	case EPCS_ENABLE_RESPONSE:
		epcsComposeRsp(prMsduInfo, prMldStaRec->ucEpcsRxDialogToken,
				STATUS_CODE_SUCCESSFUL);
		u2FrameLen += OFFSET_OF(struct ACTION_EPCS_RSP_FRAME,
				aucMultiLink);
		prMldStaRec->fgEPCS = 1;
		cnmTimerStartTimer(prAdapter, &prMldStaRec->rEpcsTimer,
				EPCS_REQ_TX_TIMEOUT);
		break;
	case EPCS_TEARDOWN:
		u2FrameLen += OFFSET_OF(struct ACTION_EPCS_RSP_FRAME,
				ucDialogToken);
		/* Restore BSS (MU)Edca param */
		if (prMldStaRec->fgEPCS) {
			epcsMldMUnEdcaBackupRestore(prMldBssInfo, FALSE);
			epcsMldMUnEdcaUpdate(prAdapter, prMldBssInfo);
		}
		prMldStaRec->fgEPCS = 0;
		epcsMldStaRecUpdate(prAdapter, prMldBssInfo);
		break;
	default:
		DBGLOG(TX, ERROR, "action invalid %u\n", eAction);
		return WLAN_STATUS_FAILURE;
	}

	nicTxSetMngPacket(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
			  prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
			  u2FrameLen, epcsTxDoneHandler[eAction],
			  MSDU_RATE_MODE_AUTO);

	DBGLOG(TX, TRACE, "[EPCS] Frame content:");
	DBGLOG_MEM8(TX, TRACE, prMsduInfo->prPacket, u2FrameLen);
	DBGLOG(TX, TRACE, "[EPCS] !=======================================!\n");

	/* Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

static bool epcsIsRspValid(struct ADAPTER *prAdapter,
			struct MLD_STA_RECORD *prMldStaRec,
			struct BSS_DESC *prBssDesc, uint8_t ucRxToken)
{
	/* check if Rx token matches latest Tx EPCS request */
	if (prMldStaRec->ucEpcsTxDialogToken == ucRxToken)
		return TRUE;

	/* check Unsolicated EPCS capability */
	if (!prAdapter->rWifiVar.fgEnUEpcs || !prBssDesc->fgUEpcsCap)
		return FALSE;

	/* the only valid token id of unsolicated EPCS resp is 0 */
	if (ucRxToken == 0)
		return TRUE;

	return FALSE;

}

void epcsProcessRsp(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
		struct MLD_STA_RECORD *prMldStaRec,
		struct MLD_BSS_INFO *prMldBssInfo, struct BSS_DESC *prBssDesc)
{
	const uint8_t *ml;
	struct ACTION_EPCS_RSP_FRAME *prRxFrame;
	uint8_t *pucIE;
	uint16_t u2IELength;
	struct MULTI_LINK_INFO rMlInfo;
	struct MULTI_LINK_INFO *prMlInfo = &rMlInfo;

	if (!prAdapter || !prSwRfb || !prMldStaRec || !prMldBssInfo
	    || !prBssDesc)
		return;

	prRxFrame = (struct ACTION_EPCS_RSP_FRAME *) prSwRfb->pvHeader;
	DBGLOG(RX, TRACE, "[EPCS] Frame content:");
	DBGLOG_MEM8(RX, TRACE, prRxFrame, prSwRfb->u2PacketLen);
	DBGLOG(RX, TRACE, "[EPCS] !=======================================!\n");

	DBGLOG(RX, INFO, "status %u, uEpcs %d, Rx Dia %d, Adapter Dia %d",
	       prRxFrame->u2StatusCode, prAdapter->rWifiVar.fgEnUEpcs,
	       prRxFrame->ucDialogToken, prMldStaRec->ucEpcsTxDialogToken);

	if (prRxFrame->u2StatusCode != STATUS_CODE_SUCCESSFUL)
		return;
	if (!epcsIsRspValid(prAdapter, prMldStaRec, prBssDesc,
		prRxFrame->ucDialogToken))
		return;

	/* Process priority access multi-link IE */
	pucIE = (uint8_t *) &prRxFrame->aucMultiLink;
	u2IELength = prSwRfb->u2PacketLen -
		OFFSET_OF(struct ACTION_EPCS_REQ_FRAME, aucMultiLink);

	ml = mldFindMlIE(pucIE, u2IELength,
			ML_CTRL_TYPE_PRIORITY_ACCESS);
	if (!ml) {
		DBGLOG(RX, DEBUG, "No ML Priority Access IE\n");
		return;
	}

	/* (MU)EDCA backup */
	epcsMldMUnEdcaBackupRestore(prMldBssInfo, TRUE);

	mldParsePriorityAccessMlIE(prAdapter, prMlInfo, prSwRfb, ml,
			u2IELength, __func__);
	prMldStaRec->fgEPCS = 1;

	/* (MU)EDCA update */
	epcsMldMUnEdcaUpdate(prAdapter, prMldBssInfo);
	epcsMldStaRecUpdate(prAdapter, prMldBssInfo);
}

uint32_t epcsProcessReq(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
		struct MLD_STA_RECORD *prMldStaRec,
		struct MLD_BSS_INFO *prMldBssInfo)
{
	const uint8_t *ml;
	struct ACTION_EPCS_REQ_FRAME *prRxFrame;
	uint8_t *pucIE;
	uint16_t u2IELength;
	struct MULTI_LINK_INFO rMlInfo;
	struct MULTI_LINK_INFO *prMlInfo = &rMlInfo;

	if (!prAdapter || !prSwRfb || !prMldStaRec || !prMldBssInfo)
		return WLAN_STATUS_FAILURE;

	prRxFrame = (struct ACTION_EPCS_REQ_FRAME *) prSwRfb->pvHeader;

	prMldStaRec->ucEpcsRxDialogToken = prRxFrame->ucDialogToken;

	/* Process priority access multi-link IE */
	pucIE = (uint8_t *) &prRxFrame->aucMultiLink;
	u2IELength = prSwRfb->u2PacketLen -
		OFFSET_OF(struct ACTION_EPCS_REQ_FRAME, aucMultiLink);
	ml = mldFindMlIE(pucIE, u2IELength,
			ML_CTRL_TYPE_PRIORITY_ACCESS);
	if (!ml) {
		DBGLOG(RX, DEBUG, "No ML Priority Access IE\n");
		return WLAN_STATUS_FAILURE;
	}

	/* (MU)EDCA backup */
	epcsMldMUnEdcaBackupRestore(prMldBssInfo, TRUE);

	mldParsePriorityAccessMlIE(prAdapter, prMlInfo, prSwRfb, ml,
			u2IELength, __func__);
	prMldStaRec->fgEPCS = 1;

	return WLAN_STATUS_SUCCESS;
}

void epcsMldMUnEdcaBackupRestore(struct MLD_BSS_INFO *prMldBssInfo,
		u_int8_t fgBackup)
{
	struct BSS_INFO *bss;

	LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList, rLinkEntryMld,
			struct BSS_INFO) {
		epcsMUnEdcaBackupRestore(bss, fgBackup);
	}

}

void epcsMldMUnEdcaUpdate(struct ADAPTER *prAdapter,
		struct MLD_BSS_INFO *prMldBssInfo)
{
	struct BSS_INFO *bss;

	LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList, rLinkEntryMld,
			struct BSS_INFO) {
		nicQmUpdateWmmParms(prAdapter, bss->ucBssIndex);
		nicQmUpdateMUEdcaParams(prAdapter, bss->ucBssIndex);
	}

}
void epcsMldStaRecUpdate(struct ADAPTER *prAdapter,
		struct MLD_BSS_INFO *prMldBssInfo)
{
	struct BSS_INFO *bss;

	LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList, rLinkEntryMld,
			struct BSS_INFO) {
		if (!bss) {
			DBGLOG(QM, ERROR, "bss is NULL\n");
			break;
		} else if (!IS_BSS_AIS(bss)) {
			DBGLOG(QM, ERROR,
				"[%u] bss is not AIS\n", bss->ucBssIndex);
			break;
		}
		cnmStaSendUpdateCmd(prAdapter, bss->prStaRecOfAP, NULL, FALSE);
	}

}

void epcsMUnEdcaBackupRestore(struct BSS_INFO *prBssInfo, u_int8_t fgBackup)
{
	enum ENUM_WMM_ACI eAci;
	struct AC_QUE_PARMS *prAcQueParams;

	if (!prBssInfo)
		return;

	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
		prAcQueParams = &prBssInfo->arACQueParms[eAci];
		DBGLOG(QM, DEBUG,
			"BSS[%u]: eAci[%d] ACM[%d] Aifsn[%d] CWmin/max[%d/%d] TxopLimit[%d]\n",
			prBssInfo->ucBssIndex, eAci,
			prAcQueParams->ucIsACMSet,
			prAcQueParams->u2Aifsn, prAcQueParams->u2CWmin,
			prAcQueParams->u2CWmax,
			prAcQueParams->u2TxopLimit);
		prAcQueParams = &prBssInfo->arBackupACQueParms[eAci];
		DBGLOG(QM, DEBUG,
			"BSS[%u]: backup eAci[%d] ACM[%d] Aifsn[%d] CWmin/max[%d/%d] TxopLimit[%d]\n",
			prBssInfo->ucBssIndex, eAci,
			prAcQueParams->ucIsACMSet,
			prAcQueParams->u2Aifsn, prAcQueParams->u2CWmin,
			prAcQueParams->u2CWmax,
			prAcQueParams->u2TxopLimit);
	}

	if (fgBackup) {
		prBssInfo->ucBackupWmmParamSetCount =
				prBssInfo->ucWmmParamSetCount;
		kalMemCopy(prBssInfo->arBackupACQueParms,
				prBssInfo->arACQueParms,
				sizeof(prBssInfo->arACQueParms));
		prBssInfo->ucBackupMUEdcaUpdateCnt =
			prBssInfo->ucMUEdcaUpdateCnt;
		kalMemCopy(prBssInfo->arBackupMUEdcaParams,
				prBssInfo->arMUEdcaParams,
				sizeof(prBssInfo->arMUEdcaParams));
	} else {
		prBssInfo->ucWmmParamSetCount =
				prBssInfo->ucBackupWmmParamSetCount;
		kalMemCopy(prBssInfo->arACQueParms,
				prBssInfo->arBackupACQueParms,
				sizeof(prBssInfo->arACQueParms));
		prBssInfo->ucMUEdcaUpdateCnt =
			prBssInfo->ucBackupMUEdcaUpdateCnt;
		kalMemCopy(prBssInfo->arMUEdcaParams,
				prBssInfo->arBackupMUEdcaParams,
				sizeof(prBssInfo->arMUEdcaParams));
	}

	DBGLOG(QM, DEBUG, "SWAPPED, fgBackup [%d]\n", fgBackup);

	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
		prAcQueParams = &prBssInfo->arACQueParms[eAci];
		DBGLOG(QM, DEBUG,
			"BSS[%u]: eAci[%d] ACM[%d] Aifsn[%d] CWmin/max[%d/%d] TxopLimit[%d]\n",
			prBssInfo->ucBssIndex, eAci,
			prAcQueParams->ucIsACMSet,
			prAcQueParams->u2Aifsn, prAcQueParams->u2CWmin,
			prAcQueParams->u2CWmax,
			prAcQueParams->u2TxopLimit);
		prAcQueParams = &prBssInfo->arBackupACQueParms[eAci];
		DBGLOG(QM, DEBUG,
			"BSS[%u]: backup eAci[%d] ACM[%d] Aifsn[%d] CWmin/max[%d/%d] TxopLimit[%d]\n",
			prBssInfo->ucBssIndex, eAci,
			prAcQueParams->ucIsACMSet,
			prAcQueParams->u2Aifsn, prAcQueParams->u2CWmin,
			prAcQueParams->u2CWmax,
			prAcQueParams->u2TxopLimit);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to process the EPCS action frame.
 *        Called by: Handle Rx mgmt request
 * @param     prAdapter      Pointer to the Adapter structure.
 * @param     prSwRfb        Pointer to processing action frame
 *                           valid action code: EPCS_ENABLE_REQUEST [3]
 *                                              EPCS_ENABLE_RESPONSE [4]
 *                                              EPCS_TEARDOWN [5]
 */
/*----------------------------------------------------------------------------*/
void epcsProcessAction(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct WLAN_ACTION_FRAME *prRxFrame;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct MLD_STA_RECORD *prMldStaRec;
	struct MLD_BSS_INFO *prMldBssInfo;
	struct BSS_DESC *prBssDesc;
	struct WIFI_VAR *prWifiVar;
	uint32_t rStatus;

	if (!prAdapter || !prSwRfb)
		return;

	prWifiVar = &prAdapter->rWifiVar;
	if (IS_FEATURE_DISABLED(prWifiVar->fgEnEpcs))
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo || !IS_BSS_AIS(prBssInfo))
		return;

	prBssDesc = aisGetTargetBssDesc(prAdapter, prBssInfo->ucBssIndex);
	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldStaRec || !prMldBssInfo || !prBssDesc) {
		DBGLOG(RX, ERROR,
			"prMldStaRec, prMldBssInfo, or prBssDesc equal to NULL\n"
		);
		return;
	}

	prRxFrame = (struct WLAN_ACTION_FRAME *)prSwRfb->pvHeader;

	DBGLOG(RX, DEBUG, "Received EPCS action:%u\n", prRxFrame->ucAction);

	switch (prRxFrame->ucAction) {
	case EPCS_ENABLE_REQUEST:
		if (!prBssDesc->fgEpcsCap)
			break;
		rStatus = epcsProcessReq(prAdapter, prSwRfb,
				prMldStaRec, prMldBssInfo);

		if (rStatus == WLAN_STATUS_SUCCESS) {
			/* Send response */
			prMldStaRec->ucEpcsRspRetryCnt = 0;
			epcsSend(prAdapter, EPCS_ENABLE_RESPONSE, prBssInfo);
		}
		break;
	case EPCS_ENABLE_RESPONSE:
		epcsProcessRsp(prAdapter, prSwRfb, prMldStaRec, prMldBssInfo,
			prBssDesc);
		break;
	case EPCS_TEARDOWN:
		/* Restore BSS (MU)Edca param */
		if (prMldStaRec->fgEPCS) {
			epcsMldMUnEdcaBackupRestore(prMldBssInfo, FALSE);
			epcsMldMUnEdcaUpdate(prAdapter, prMldBssInfo);
		}
		prMldStaRec->fgEPCS = 0;
		epcsMldStaRecUpdate(prAdapter, prMldBssInfo);
		break;
	default:
		DBGLOG(RX, ERROR, "Action unexpected %u\n",
				prRxFrame->ucAction);
		break;
	}
}

#endif /* CFG_SUPPORT_80_11BE_EPCS */
#endif /* CFG_SUPPORT_802_11BE_MLO */
