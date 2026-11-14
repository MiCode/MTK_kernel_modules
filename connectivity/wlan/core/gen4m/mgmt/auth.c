// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "auth.c"
 *    \brief  This file includes the authentication-related functions.
 *
 *   This file includes the authentication-related functions.
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
struct APPEND_VAR_IE_ENTRY txAuthIETable[] = {
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_CHALLENGE_TEXT), NULL,
	 authAddIEChallengeText},
	{0, authCalculateRSNIELen, authAddRSNIE}, /* Element ID: 48 */
	{(ELEM_HDR_LEN + 1), NULL, authAddMDIE}, /* Element ID: 54 */
	{0, authCalculateFTIELen, authAddFTIE}, /* Element ID: 55 */
#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	{FILS_AUTH_MAX_LEN, 0, filsBuildAuthIE},
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	{0, mldCalculateMlIELen, mldGenerateMlIE}
#endif
};

struct HANDLE_IE_ENTRY rxAuthIETable[] = {
	{ELEM_ID_CHALLENGE_TEXT, 0, authHandleIEChallengeText},
	{ELEM_ID_RSN, 0, authHandleRSNE},
#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	{ELEM_ID_EXTENSION, ELEM_EXT_ID_FILS_NONCE, filsRxAuthNonce},
	{ELEM_ID_EXTENSION, ELEM_EXT_ID_FILS_SESSION, filsRxAuthSession},
	{ELEM_ID_EXTENSION, ELEM_EXT_ID_FILS_WRAPPED_DATA, filsRxAuthWrapped},
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
};

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
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the Authentication frame header and
 *        fixed fields.
 *
 * @param[in] pucBuffer              Pointer to the frame buffer.
 * @param[in] aucPeerMACAddress      Given Peer MAC Address.
 * @param[in] aucMACAddress          Given Our MAC Address.
 * @param[in] u2AuthAlgNum           Authentication Algorithm Number
 * @param[in] u2TransactionSeqNum    Transaction Sequence Number
 * @param[in] u2StatusCode           Status Code
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void
authComposeAuthFrameHeaderAndFF(uint8_t *pucBuffer,
				uint8_t aucPeerMACAddress[],
				uint8_t aucMACAddress[],
				uint16_t u2AuthAlgNum,
				uint16_t u2TransactionSeqNum,
				uint16_t u2StatusCode)
{
	struct WLAN_AUTH_FRAME *prAuthFrame;
	uint16_t u2FrameCtrl;

	prAuthFrame = (struct WLAN_AUTH_FRAME *)pucBuffer;

	/* 4 <1> Compose the frame header of the Authentication frame. */
	/* Fill the Frame Control field. */
	u2FrameCtrl = MAC_FRAME_AUTH;

	/* If this frame is the third frame in the shared key authentication
	 * sequence, it shall be encrypted.
	 */
	if ((u2AuthAlgNum == AUTH_ALGORITHM_NUM_SHARED_KEY)
	    && (u2TransactionSeqNum == AUTH_TRANSACTION_SEQ_3))
		u2FrameCtrl |= MASK_FC_PROTECTED_FRAME;
		/* HW will also detect this bit for applying encryption */
	/* WLAN_SET_FIELD_16(&prAuthFrame->u2FrameCtrl, u2FrameCtrl); */
	prAuthFrame->u2FrameCtrl = u2FrameCtrl;
	/* NOTE(Kevin): Optimized for ARM */

	/* Fill the DA field with Target BSSID. */
	COPY_MAC_ADDR(prAuthFrame->aucDestAddr, aucPeerMACAddress);

	/* Fill the SA field with our MAC Address. */
	COPY_MAC_ADDR(prAuthFrame->aucSrcAddr, aucMACAddress);

	switch (u2TransactionSeqNum) {
	case AUTH_TRANSACTION_SEQ_1:
	case AUTH_TRANSACTION_SEQ_3:

		/* Fill the BSSID field with Target BSSID. */
		COPY_MAC_ADDR(prAuthFrame->aucBSSID, aucPeerMACAddress);
		break;

	case AUTH_TRANSACTION_SEQ_2:
	case AUTH_TRANSACTION_SEQ_4:

		/* Fill the BSSID field with Current BSSID. */
		COPY_MAC_ADDR(prAuthFrame->aucBSSID, aucMACAddress);
		break;

	default:
		ASSERT(0);
	}

	/* Clear the SEQ/FRAG_NO field. */
	prAuthFrame->u2SeqCtrl = 0;

	/* 4 <2> Compose the frame body's fixed field part of
	 *       the Authentication frame.
	 */
	/* Fill the Authentication Algorithm Number field. */
	/* WLAN_SET_FIELD_16(&prAuthFrame->u2AuthAlgNum, u2AuthAlgNum); */
	prAuthFrame->u2AuthAlgNum = u2AuthAlgNum;
	/* NOTE(Kevin): Optimized for ARM */

	/* Fill the Authentication Transaction Sequence Number field. */
	/* WLAN_SET_FIELD_16(&prAuthFrame->u2AuthTransSeqNo,
	 *	u2TransactionSeqNum);
	 */
	prAuthFrame->u2AuthTransSeqNo = u2TransactionSeqNum;
	/* NOTE(Kevin): Optimized for ARM */

	/* Fill the Status Code field. */
	/* WLAN_SET_FIELD_16(&prAuthFrame->u2StatusCode, u2StatusCode); */
	prAuthFrame->u2StatusCode = u2StatusCode;
	/* NOTE(Kevin): Optimized for ARM */
}				/* end of authComposeAuthFrameHeaderAndFF() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will append Challenge Text IE to the Authentication
 *        frame
 *
 * @param[in] prMsduInfo     Pointer to the composed MSDU_INFO_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void authAddIEChallengeText(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo)
{
	struct WLAN_AUTH_FRAME *prAuthFrame;
	struct STA_RECORD *prStaRec;
	uint16_t u2TransactionSeqNum;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prStaRec)
		return;

	/* For Management, frame header and payload are in a continuous
	 * buffer
	 */
	prAuthFrame = (struct WLAN_AUTH_FRAME *)prMsduInfo->prPacket;

	WLAN_GET_FIELD_16(&prAuthFrame->u2AuthTransSeqNo, &u2TransactionSeqNum)

	    /* Only consider SEQ_3 for Challenge Text */
	if ((u2TransactionSeqNum == AUTH_TRANSACTION_SEQ_3) &&
		(prStaRec->ucAuthAlgNum == AUTH_ALGORITHM_NUM_SHARED_KEY)
		&& (prStaRec->prChallengeText != NULL)) {

		COPY_IE(((uintptr_t)(prMsduInfo->prPacket) +
			 prMsduInfo->u2FrameLength),
			(prStaRec->prChallengeText));

		prMsduInfo->u2FrameLength += IE_SIZE(prStaRec->prChallengeText);
	}

	return;

}				/* end of authAddIEChallengeText() */

struct MSDU_INFO *authComposeAuthFrame(struct ADAPTER *prAdapter,
		  struct STA_RECORD *prStaRec,
		  uint8_t ucBssIndex,
		  struct SW_RFB *prFalseAuthSwRfb,
		  uint16_t u2TransactionSeqNum, uint16_t u2StatusCode)
{
	uint8_t *pucReceiveAddr;
	uint8_t *pucTransmitAddr;
	struct MSDU_INFO *prMsduInfo;
	struct BSS_INFO *prBssInfo;
	/*get from input parameter */
	/* ENUM_NETWORK_TYPE_INDEX_T eNetTypeIndex = NETWORK_TYPE_AIS_INDEX; */
	PFN_TX_DONE_HANDLER pfTxDoneHandler = (PFN_TX_DONE_HANDLER) NULL;
	uint16_t u2EstimatedFrameLen;
	uint16_t u2EstimatedExtraIELen;
	uint16_t u2PayloadLen;
	uint16_t ucAuthAlgNum;
	uint32_t i;

	/* 4 <1> Allocate a PKT_INFO_T for Authentication Frame */
	/* Init with MGMT Header Length + Length of Fixed Fields */
	u2EstimatedFrameLen = (MAC_TX_RESERVED_FIELD +
			       WLAN_MAC_MGMT_HEADER_LEN +
			       AUTH_ALGORITHM_NUM_FIELD_LEN +
			       AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN +
			       STATUS_CODE_FIELD_LEN);

	/* + Extra IE Length */
	u2EstimatedExtraIELen = 0;

	for (i = 0; i < ARRAY_SIZE(txAuthIETable); i++) {
		if (txAuthIETable[i].u2EstimatedFixedIELen != 0)
			u2EstimatedExtraIELen +=
				txAuthIETable[i].u2EstimatedFixedIELen;
		else
			u2EstimatedExtraIELen +=
				txAuthIETable[i].pfnCalculateVariableIELen(
					prAdapter, ucBssIndex, prStaRec);
	}

	u2EstimatedFrameLen += u2EstimatedExtraIELen;

	/* Allocate a MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(SAA, WARN, "No PKT_INFO_T for sending Auth Frame.\n");
		return NULL;
	}
	/* 4 <2> Compose Authentication Request frame header and
	 * fixed fields in MSDU_INfO_T.
	 */
	if (prStaRec) {
		prBssInfo =
		    GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
		if (!prBssInfo) {
			DBGLOG(SAA, ERROR, "prBssInfo is null\n");
			return NULL;
		}

		pucTransmitAddr = prBssInfo->aucOwnMacAddr;

		pucReceiveAddr = prStaRec->aucMacAddr;

		ucAuthAlgNum = prStaRec->ucAuthAlgNum;

		switch (u2TransactionSeqNum) {
		case AUTH_TRANSACTION_SEQ_1:
		case AUTH_TRANSACTION_SEQ_3:
			pfTxDoneHandler = saaFsmRunEventTxDone;
			break;

#if CFG_SUPPORT_AAA
		case AUTH_TRANSACTION_SEQ_2:
		case AUTH_TRANSACTION_SEQ_4:
			pfTxDoneHandler = aaaFsmRunEventTxDone;
			break;
#endif
		}

	} else {		/* For Error Status Code */
		struct WLAN_AUTH_FRAME *prFalseAuthFrame;

		prFalseAuthFrame =
		    (struct WLAN_AUTH_FRAME *)prFalseAuthSwRfb->pvHeader;

		pucTransmitAddr = prFalseAuthFrame->aucDestAddr;

		pucReceiveAddr = prFalseAuthFrame->aucSrcAddr;

		ucAuthAlgNum = prFalseAuthFrame->u2AuthAlgNum;

		u2TransactionSeqNum = (prFalseAuthFrame->u2AuthTransSeqNo + 1);
	}

	/* Compose Header and some Fixed Fields */
	authComposeAuthFrameHeaderAndFF((uint8_t *)
					((uintptr_t)(prMsduInfo->prPacket) +
					 MAC_TX_RESERVED_FIELD), pucReceiveAddr,
					pucTransmitAddr, ucAuthAlgNum,
					u2TransactionSeqNum, u2StatusCode);

	u2PayloadLen =
	    (AUTH_ALGORITHM_NUM_FIELD_LEN +
	     AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN + STATUS_CODE_FIELD_LEN);

	/* 4 <3> Update information of MSDU_INFO_T */
	nicTxSetForceRts(prMsduInfo, TRUE);

	TX_SET_MMPDU(prAdapter,
		     prMsduInfo,
		     ucBssIndex,
		     (prStaRec !=
		      NULL) ? (prStaRec->ucIndex) : (STA_REC_INDEX_NOT_FOUND),
		     WLAN_MAC_MGMT_HEADER_LEN,
		     WLAN_MAC_MGMT_HEADER_LEN + u2PayloadLen, pfTxDoneHandler,
		     MSDU_RATE_MODE_AUTO);

	if ((ucAuthAlgNum == AUTH_ALGORITHM_NUM_SHARED_KEY)
	    && (u2TransactionSeqNum == AUTH_TRANSACTION_SEQ_3))
		nicTxConfigPktOption(prMsduInfo, MSDU_OPT_PROTECTED_FRAME,
				     TRUE);
	/* 4 <4> Compose IEs in MSDU_INFO_T */
	for (i = 0; i < ARRAY_SIZE(txAuthIETable); i++) {
		if (txAuthIETable[i].pfnAppendIE)
			txAuthIETable[i].pfnAppendIE(prAdapter, prMsduInfo);
	}

	/* TODO(Kevin):
	 * Also release the unused tail room of the composed MMPDU
	 */

	nicTxConfigPktControlFlag(prMsduInfo,
		MSDU_CONTROL_FLAG_FORCE_TX | MSDU_CONTROL_FLAG_MGNT_2_CMD_QUE,
		TRUE);

	sortMgmtFrameIE(prAdapter, prMsduInfo);

	return prMsduInfo;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send the Authenticiation frame
 *
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 * @param[in] u2TransactionSeqNum    Transaction Sequence Number
 *
 * @retval WLAN_STATUS_RESOURCES No available resource for frame composing.
 * @retval WLAN_STATUS_SUCCESS   Successfully send frame to TX Module
 */
/*----------------------------------------------------------------------------*/
uint32_t
authSendAuthFrame(struct ADAPTER *prAdapter,
		  struct STA_RECORD *prStaRec,
		  uint8_t ucBssIndex,
		  struct SW_RFB *prFalseAuthSwRfb,
		  uint16_t u2TransactionSeqNum, uint16_t u2StatusCode)
{
	struct WLAN_AUTH_FRAME *prAuthFrame;
	struct MSDU_INFO *prMsduInfo;

	prMsduInfo = authComposeAuthFrame(prAdapter, prStaRec, ucBssIndex,
		prFalseAuthSwRfb,
		u2TransactionSeqNum, u2StatusCode);
	if (!prMsduInfo)
		return WLAN_STATUS_RESOURCES;

	prAuthFrame = (struct WLAN_AUTH_FRAME *)
		((uintptr_t)(prMsduInfo->prPacket) + MAC_TX_RESERVED_FIELD);
	DBGLOG(SAA, INFO,
	       "%sTX_AUTH algo=%d auth_seq=%d status=%d seq=%d sn=%d SA=" MACSTR
	       " DA=" MACSTR "\n",
		IS_BSS_INDEX_AIS(prAdapter, prStaRec->ucBssIndex) ?
		"<CONN> " : "",
	       prAuthFrame->u2AuthAlgNum, u2TransactionSeqNum, u2StatusCode,
	       prMsduInfo->ucTxSeqNum, prMsduInfo->u2SwSN,
	       MAC2STR(prAuthFrame->aucSrcAddr),
	       MAC2STR(prAuthFrame->aucDestAddr));

	/* 4 <6> Inform TXM  to send this Authentication frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}				/* end of authSendAuthFrame() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will strictly check the TX Authentication frame
 *        for SAA/AAA event handling.
 *
 * @param[in] prMsduInfo             Pointer of MSDU_INFO_T
 * @param[in] u2TransactionSeqNum    Transaction Sequence Number
 *
 * @retval WLAN_STATUS_FAILURE       This is not the frame we should handle
 *                                   at current state.
 * @retval WLAN_STATUS_SUCCESS       This is the frame we should handle.
 */
/*----------------------------------------------------------------------------*/
uint32_t authCheckTxAuthFrame(struct ADAPTER *prAdapter,
			      struct MSDU_INFO *prMsduInfo,
			      uint16_t u2TransactionSeqNum)
{
	struct WLAN_AUTH_FRAME *prAuthFrame;
	struct STA_RECORD *prStaRec;
	uint16_t u2TxFrameCtrl;
	uint16_t u2TxAuthAlgNum;
	uint16_t u2TxTransactionSeqNum;

	prAuthFrame = (struct WLAN_AUTH_FRAME *)(prMsduInfo->prPacket);
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prStaRec)
		return WLAN_STATUS_INVALID_PACKET;

	/* WLAN_GET_FIELD_16(&prAuthFrame->u2FrameCtrl, &u2TxFrameCtrl) */
	u2TxFrameCtrl = prAuthFrame->u2FrameCtrl;
	/* NOTE(Kevin): Optimized for ARM */
	u2TxFrameCtrl &= MASK_FRAME_TYPE;
	if (u2TxFrameCtrl != MAC_FRAME_AUTH)
		return WLAN_STATUS_FAILURE;

	/* WLAN_GET_FIELD_16(&prAuthFrame->u2AuthAlgNum, &u2TxAuthAlgNum) */
	u2TxAuthAlgNum = prAuthFrame->u2AuthAlgNum;
	/* NOTE(Kevin): Optimized for ARM */
	if (u2TxAuthAlgNum != (uint16_t) (prStaRec->ucAuthAlgNum))
		return WLAN_STATUS_FAILURE;

	/* WLAN_GET_FIELD_16(&prAuthFrame->u2AuthTransSeqNo,
	 *	&u2TxTransactionSeqNum)
	 */
	u2TxTransactionSeqNum = prAuthFrame->u2AuthTransSeqNo;
	/* NOTE(Kevin): Optimized for ARM */
	if (u2TxTransactionSeqNum != u2TransactionSeqNum)
		return WLAN_STATUS_FAILURE;

	return WLAN_STATUS_SUCCESS;

}				/* end of authCheckTxAuthFrame() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check the incoming Auth Frame's Transaction
 *        Sequence Number before delivering it to the corresponding
 *        SAA or AAA Module.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Always not retain authentication frames
 */
/*----------------------------------------------------------------------------*/
uint32_t authCheckRxAuthFrameTransSeq(struct ADAPTER *prAdapter,
				      struct SW_RFB *prSwRfb)
{
	struct WLAN_AUTH_FRAME *prAuthFrame;
	uint16_t u2RxTransactionSeqNum;
	uint16_t u2MinPayloadLen;
	struct STA_RECORD *prStaRec;

	/* 4 <1> locate the Authentication Frame. */
	prAuthFrame = (struct WLAN_AUTH_FRAME *)prSwRfb->pvHeader;

	/* 4 <2> Parse the Header of Authentication Frame. */
	u2MinPayloadLen = (AUTH_ALGORITHM_NUM_FIELD_LEN +
			   AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN +
			   STATUS_CODE_FIELD_LEN);
	if ((prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) < u2MinPayloadLen) {
		DBGLOG(SAA, WARN,
		       "Rx Auth payload: len[%u] < min expected len[%u]\n",
		       (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen),
		       u2MinPayloadLen);
		DBGLOG(SAA, WARN, "=== Dump Rx Auth ===\n");
		DBGLOG_MEM8(SAA, WARN, prAuthFrame, prSwRfb->u2PacketLen);
		return WLAN_STATUS_SUCCESS;
	}

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);

	if (prStaRec &&
		(IS_STA_IN_AIS(prAdapter, prStaRec) ||
		(IS_STA_IN_P2P(prAdapter, prStaRec) &&
		 IS_AP_STA(prStaRec)))) {
		if (prStaRec->eAuthAssocState == SAA_STATE_EXTERNAL_AUTH) {
			DBGLOG(SAA, INFO, "External auth\n");
			saaFsmRunEventRxAuth(prAdapter, prSwRfb);
			return WLAN_STATUS_SUCCESS;
		}
	}

	/* 4 <3> Parse the Fixed Fields of Authentication Frame Body. */
	/* WLAN_GET_FIELD_16(&prAuthFrame->u2AuthTransSeqNo,
	 *	&u2RxTransactionSeqNum);
	 */
	u2RxTransactionSeqNum = prAuthFrame->u2AuthTransSeqNo;
	/* NOTE(Kevin): Optimized for ARM */

	DBGLOG(SAA, LOUD,
		   "Authentication Packet: Auth Trans Seq No = %d\n",
		   u2RxTransactionSeqNum);

	switch (u2RxTransactionSeqNum) {
	case AUTH_TRANSACTION_SEQ_2:
	case AUTH_TRANSACTION_SEQ_4:
#if CFG_SUPPORT_AAA
		if (prStaRec && IS_STA_IN_P2P(prAdapter, prStaRec) &&
			!IS_AP_STA(prStaRec))
			aaaFsmRunEventRxAuth(prAdapter, prSwRfb);
		else
#endif
			saaFsmRunEventRxAuth(prAdapter, prSwRfb);
		break;

	case AUTH_TRANSACTION_SEQ_1:
	case AUTH_TRANSACTION_SEQ_3:
#if CFG_SUPPORT_AAA
		aaaFsmRunEventRxAuth(prAdapter, prSwRfb);
#endif /* CFG_SUPPORT_AAA */
		break;

	default:
		DBGLOG(SAA, WARN,
		       "Strange Authentication Packet: Auth Trans Seq No = %d, Error Status Code = %d\n",
		       u2RxTransactionSeqNum, prAuthFrame->u2StatusCode);
#if CFG_IGNORE_INVALID_AUTH_TSN
		prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
		if (!prStaRec)
			return WLAN_STATUS_SUCCESS;
		switch (prStaRec->eAuthAssocState) {
		case SAA_STATE_SEND_AUTH1:
		case SAA_STATE_WAIT_AUTH2:
		case SAA_STATE_SEND_AUTH3:
		case SAA_STATE_WAIT_AUTH4:
			saaFsmRunEventRxAuth(prAdapter, prSwRfb);
			break;
		default:
			break;
		}
#endif

		break;
	}

	return WLAN_STATUS_SUCCESS;

}				/* end of authCheckRxAuthFrameTransSeq() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the incoming Authentication Frame and
 *        take the status code out.
 *
 * @param[in] prSwRfb                Pointer to SW RFB data structure.
 * @param[in] u2TransactionSeqNum    Transaction Sequence Number
 * @param[out] pu2StatusCode         Pointer to store the Status Code from
 *                                   Authentication.
 *
 * @retval WLAN_STATUS_FAILURE       This is not the frame we should handle
 *                                   at current state.
 * @retval WLAN_STATUS_SUCCESS       This is the frame we should handle.
 */
/*----------------------------------------------------------------------------*/
uint32_t
authCheckRxAuthFrameStatus(struct ADAPTER *prAdapter,
			   struct SW_RFB *prSwRfb,
			   uint16_t u2TransactionSeqNum,
			   uint16_t *pu2StatusCode)
{
	struct STA_RECORD *prStaRec;
	struct WLAN_AUTH_FRAME *prAuthFrame;
	uint16_t u2RxAuthAlgNum;
	uint16_t u2RxTransactionSeqNum;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return WLAN_STATUS_INVALID_PACKET;

	/* 4 <1> locate the Authentication Frame. */
	prAuthFrame = (struct WLAN_AUTH_FRAME *)prSwRfb->pvHeader;

	DBGLOG(SAA, INFO,
		"%sRX_AUTH algo=%d auth_seq=%d status=%d sn=%d SA="
		MACSTR " DA=" MACSTR "\n",
		IS_BSS_INDEX_AIS(prAdapter, prStaRec->ucBssIndex) ?
		"<CONN> " : "",
		prAuthFrame->u2AuthAlgNum,
		prAuthFrame->u2AuthTransSeqNo,
		prAuthFrame->u2StatusCode,
		WLAN_GET_SEQ_SEQ(prAuthFrame->u2SeqCtrl),
		MAC2STR(prAuthFrame->aucSrcAddr),
		MAC2STR(prAuthFrame->aucDestAddr));

	/* 4 <2> Parse the Fixed Fields of Authentication Frame Body. */
	/* WLAN_GET_FIELD_16(&prAuthFrame->u2AuthAlgNum, &u2RxAuthAlgNum); */
	u2RxAuthAlgNum = prAuthFrame->u2AuthAlgNum;
	/* NOTE(Kevin): Optimized for ARM */
	if (u2RxAuthAlgNum != (uint16_t) prStaRec->ucAuthAlgNum) {
		DBGLOG(SAA, WARN,
		       "Discard Auth frame with auth type = %d, current = %d\n",
		       u2RxAuthAlgNum, prStaRec->ucAuthAlgNum);
		*pu2StatusCode = STATUS_CODE_AUTH_ALGORITHM_NOT_SUPPORTED;
		return WLAN_STATUS_SUCCESS;
	}
	/* WLAN_GET_FIELD_16(&prAuthFrame->u2AuthTransSeqNo,
	 *	&u2RxTransactionSeqNum);
	 */
	u2RxTransactionSeqNum = prAuthFrame->u2AuthTransSeqNo;
	/* NOTE(Kevin): Optimized for ARM */
	if (u2RxTransactionSeqNum != u2TransactionSeqNum) {
		if (u2RxAuthAlgNum != AUTH_ALGORITHM_NUM_SAE) {
			DBGLOG(SAA, WARN,
				"Discard Auth frame with Transaction Seq No = %d\n",
				u2RxTransactionSeqNum);
		}
#if (CFG_SUPPORT_CONN_LOG == 1)
		else {
			connLogAuthResp(prAdapter,
				prStaRec,
				prAuthFrame,
				prAuthFrame->u2StatusCode);
		}
#endif
		*pu2StatusCode = STATUS_CODE_AUTH_OUT_OF_SEQ;
		return WLAN_STATUS_FAILURE;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (prAuthFrame->u2StatusCode == STATUS_CODE_SUCCESSFUL &&
	    !mldSanityCheck(prAdapter, prSwRfb->pvHeader,
		prSwRfb->u2PacketLen, prStaRec, prStaRec->ucBssIndex)) {
		DBGLOG(SAA, WARN, "Discard Auth frame with wrong ML IE\n");
		*pu2StatusCode = STATUS_CODE_DENIED_EHT_NOT_SUPPORTED;
		return WLAN_STATUS_FAILURE;
	}
#endif

	/* 4 <3> Get the Status code */
	*pu2StatusCode = prAuthFrame->u2StatusCode;

#if (CFG_SUPPORT_CONN_LOG == 1)
	connLogAuthResp(prAdapter,
		prStaRec,
		prAuthFrame,
		*pu2StatusCode);
#endif

	return WLAN_STATUS_SUCCESS;

}				/* end of authCheckRxAuthFrameStatus() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Challenge Text IE from
 *        the Authentication frame
 *
 * @param[in] prSwRfb                Pointer to SW RFB data structure.
 * @param[in] prIEHdr                Pointer to start address of IE
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t authHandleIEChallengeText(struct ADAPTER *prAdapter,
			       struct SW_RFB *prSwRfb, struct IE_HDR *prIEHdr)
{
	struct WLAN_AUTH_FRAME *prAuthFrame;
	struct STA_RECORD *prStaRec;
	uint16_t u2TransactionSeqNum;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec) {
		DBGLOG(SAA, ERROR, "No starec\n");
		return WLAN_STATUS_FAILURE;
	}
	/* For Management, frame header and payload are in
	 * a continuous buffer
	 */
	prAuthFrame = (struct WLAN_AUTH_FRAME *)prSwRfb->pvHeader;

	/* WLAN_GET_FIELD_16(&prAuthFrame->u2AuthTransSeqNo,
	 *	&u2TransactionSeqNum)
	 */
	u2TransactionSeqNum = prAuthFrame->u2AuthTransSeqNo;
	/* NOTE(Kevin): Optimized for ARM */

	/* Only consider SEQ_2 for Challenge Text */
	if ((u2TransactionSeqNum == AUTH_TRANSACTION_SEQ_2) &&
	    (prStaRec->ucAuthAlgNum == AUTH_ALGORITHM_NUM_SHARED_KEY)) {

		/* Free previous allocated TCM memory */
		if (prStaRec->prChallengeText) {
			cnmMemFree(prAdapter, prStaRec->prChallengeText);
			prStaRec->prChallengeText =
			    (struct IE_CHALLENGE_TEXT *)NULL;
		}
		prStaRec->prChallengeText =
		    cnmMemAlloc(prAdapter, RAM_TYPE_MSG, IE_SIZE(prIEHdr));
		if (prStaRec->prChallengeText == NULL) {
			DBGLOG(SAA, ERROR, "No memory\n");
			return WLAN_STATUS_RESOURCES;
		}

		/* Save the Challenge Text from Auth Seq 2 Frame,
		 * before sending Auth Seq 3 Frame
		 */
		COPY_IE(prStaRec->prChallengeText, prIEHdr);
	}

	return WLAN_STATUS_SUCCESS;
}				/* end of authAddIEChallengeText() */

uint32_t authHandleRSNE(struct ADAPTER *prAdapter,
		    struct SW_RFB *prSwRfb, struct IE_HDR *prIEHdr)
{
	struct STA_RECORD *prStaRec;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec) {
		DBGLOG(SAA, ERROR, "No starec\n");
		return WLAN_STATUS_FAILURE;
	}

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	if (filsRxAuthRSNE(prAdapter, prSwRfb, prIEHdr) !=
		WLAN_STATUS_SUCCESS)
		return WLAN_STATUS_FAILURE;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will parse and process the incoming Authentication
 *        frame.
 *
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 *
 * @retval WLAN_STATUS_SUCCESS   This is the frame we should handle.
 */
/*----------------------------------------------------------------------------*/
uint32_t authProcessRxAuth2_Auth4Frame(struct ADAPTER *prAdapter,
				       struct SW_RFB *prSwRfb)
{
	struct WLAN_AUTH_FRAME *prAuthFrame;
	uint8_t *pucIEsBuffer;
	uint16_t u2IEsLen;
	uint16_t u2Offset;
	uint8_t ucIEID;
	uint8_t ucIEExtID;
	uint32_t i;
	uint32_t u4Status;

	prAuthFrame = (struct WLAN_AUTH_FRAME *)prSwRfb->pvHeader;

	pucIEsBuffer = &prAuthFrame->aucInfoElem[0];
	u2IEsLen = (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
	    (AUTH_ALGORITHM_NUM_FIELD_LEN +
	     AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN + STATUS_CODE_FIELD_LEN);

	IE_FOR_EACH(pucIEsBuffer, u2IEsLen, u2Offset) {
		ucIEID = IE_ID(pucIEsBuffer);
		ucIEExtID = IE_ID_EXT(pucIEsBuffer);

		for (i = 0; i < ARRAY_SIZE(rxAuthIETable); i++) {
			if (ucIEID != rxAuthIETable[i].ucElemID)
				continue;
			if (ucIEID == ELEM_ID_EXTENSION && ucIEExtID !=
				rxAuthIETable[i].ucElemExtID)
				continue;

			if (rxAuthIETable[i].pfnHandleIE != NULL) {
				u4Status = rxAuthIETable[i].pfnHandleIE(
					prAdapter, prSwRfb,
					(struct IE_HDR *)pucIEsBuffer);
				if (u4Status != WLAN_STATUS_SUCCESS)
					return u4Status;
			}
		}
	}
	if (prAuthFrame->u2AuthAlgNum == AUTH_ALGORITHM_NUM_FT) {
		if (prAuthFrame->u2AuthTransSeqNo == AUTH_TRANSACTION_SEQ_4) {
			/* todo: check MIC, if mic error, return
			 * WLAN_STATUS_FAILURE
			 */
		} else if (prAuthFrame->u2AuthTransSeqNo ==
			   AUTH_TRANSACTION_SEQ_2) {
			struct FT_EVENT_PARAMS *prFtParam =
				aisGetFtEventParam(prAdapter,
				secGetBssIdxByRfb(prAdapter,
				prSwRfb));
			prFtParam->pcIe =
			    &prAuthFrame->aucInfoElem[0];
			prFtParam->u2IeLen = u2IEsLen;
		}
	}

	return WLAN_STATUS_SUCCESS;

}				/* end of authProcessRxAuth2_Auth4Frame() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the Deauthentication frame
 *
 * @param[in] pucBuffer              Pointer to the frame buffer.
 * @param[in] aucPeerMACAddress      Given Peer MAC Address.
 * @param[in] aucMACAddress          Given Our MAC Address.
 * @param[in] u2StatusCode           Status Code
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void
authComposeDeauthFrameHeaderAndFF(uint8_t *pucBuffer,
				  uint8_t aucPeerMACAddress[],
				  uint8_t aucMACAddress[],
				  uint8_t aucBssid[],
				  uint16_t u2ReasonCode)
{
	struct WLAN_DEAUTH_FRAME *prDeauthFrame;
	uint16_t u2FrameCtrl;

	prDeauthFrame = (struct WLAN_DEAUTH_FRAME *)pucBuffer;

	/* 4 <1> Compose the frame header of the Deauthentication frame. */
	/* Fill the Frame Control field. */
	u2FrameCtrl = MAC_FRAME_DEAUTH;

	/* WLAN_SET_FIELD_16(&prDeauthFrame->u2FrameCtrl, u2FrameCtrl); */
	prDeauthFrame->u2FrameCtrl = u2FrameCtrl;
	/* NOTE(Kevin): Optimized for ARM */

	/* Fill the DA field with Target BSSID. */
	COPY_MAC_ADDR(prDeauthFrame->aucDestAddr, aucPeerMACAddress);

	/* Fill the SA field with our MAC Address. */
	COPY_MAC_ADDR(prDeauthFrame->aucSrcAddr, aucMACAddress);

	/* Fill the BSSID field with Target BSSID. */
	COPY_MAC_ADDR(prDeauthFrame->aucBSSID, aucBssid);

	/* Clear the SEQ/FRAG_NO field(HW won't overide the FRAG_NO,
	 * so we need to clear it).
	 */
	prDeauthFrame->u2SeqCtrl = 0;

	/* 4 <2> Compose the frame body's fixed field part of
	 *       the Authentication frame.
	 */
	/* Fill the Status Code field. */
	/* WLAN_SET_FIELD_16(&prDeauthFrame->u2ReasonCode, u2ReasonCode); */
	prDeauthFrame->u2ReasonCode = u2ReasonCode;
	/* NOTE(Kevin): Optimized for ARM */
}			/* end of authComposeDeauthFrameHeaderAndFF() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send the Deauthenticiation frame
 *
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] prClassErrSwRfb    Pointer to the SW_RFB_T which is Class Error.
 * @param[in] u2ReasonCode       A reason code to indicate why to leave BSS.
 * @param[in] pfTxDoneHandler    TX Done call back function
 *
 * @retval WLAN_STATUS_RESOURCES No available resource for frame composing.
 * @retval WLAN_STATUS_SUCCESS   Successfully send frame to TX Module
 * @retval WLAN_STATUS_FAILURE   Didn't send Deauth frame for various reasons.
 */
/*----------------------------------------------------------------------------*/
uint32_t
authSendDeauthFrame(struct ADAPTER *prAdapter,
		    struct BSS_INFO *prBssInfo,
		    struct STA_RECORD *prStaRec,
		    struct SW_RFB *prClassErrSwRfb, uint16_t u2ReasonCode,
		    PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	uint8_t *pucReceiveAddr;
	uint8_t *pucTransmitAddr;
	uint8_t *pucBssid = NULL;
	struct MSDU_INFO *prMsduInfo;
	uint16_t u2EstimatedFrameLen;

	struct DEAUTH_INFO *prDeauthInfo;
	OS_SYSTIME rCurrentTime;
	int32_t i4NewEntryIndex, i;
	uint8_t ucStaRecIdx = STA_REC_INDEX_NOT_FOUND;
	uint8_t ucBssIndex = prAdapter->ucSwBssIdNum;
	uint8_t aucBMC[] = BC_MAC_ADDR;

	/* NOTE(Kevin): The best way to reply the Deauth is according to
	 * the incoming data frame
	 */
	/* 4 <1.1> Find the Receiver Address */
	if (prClassErrSwRfb) {
		u_int8_t fgIsAbleToSendDeauth = FALSE;
		uint16_t u2RxFrameCtrl;
		struct WLAN_MAC_HEADER_A4 *prWlanMacHeader = NULL;

		prWlanMacHeader =
		    (struct WLAN_MAC_HEADER_A4 *)prClassErrSwRfb->pvHeader;

		/* WLAN_GET_FIELD_16(&prWlanMacHeader->u2FrameCtrl,
		 *   &u2RxFrameCtrl);
		 */
		u2RxFrameCtrl = prWlanMacHeader->u2FrameCtrl;
		/* NOTE(Kevin): Optimized for ARM */

		/* TODO(Kevin): Currently we won't send Deauth for IBSS node.
		 * How about DLS ?
		 */
		if ((prWlanMacHeader->u2FrameCtrl & MASK_TO_DS_FROM_DS) == 0)
			return WLAN_STATUS_FAILURE;

		DBGLOG(SAA, INFO,
		       "u2FrameCtrl=0x%x, DestAddr=" MACSTR
		       " srcAddr=" MACSTR " BSSID=" MACSTR
		       ", u2SeqCtrl=0x%x\n",
		       prWlanMacHeader->u2FrameCtrl,
		       MAC2STR(prWlanMacHeader->aucAddr1),
		       MAC2STR(prWlanMacHeader->aucAddr2),
		       MAC2STR(prWlanMacHeader->aucAddr3),
		       prWlanMacHeader->u2SeqCtrl);
		/* Check if corresponding BSS is able to send Deauth */
		for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);
			if (!prBssInfo) {
				DBGLOG(SAA, ERROR, "prBssInfo is null\n");
				continue;
			}

			if (IS_NET_ACTIVE(prAdapter, i) &&
			    (EQUAL_MAC_ADDR
			     (prWlanMacHeader->aucAddr1,
			      prBssInfo->aucOwnMacAddr))) {

				fgIsAbleToSendDeauth = TRUE;
				ucBssIndex = (uint8_t) i;
				break;
			}
		}

		if (!fgIsAbleToSendDeauth)
			return WLAN_STATUS_FAILURE;

		pucReceiveAddr = prWlanMacHeader->aucAddr2;
	} else if (prStaRec) {
		prBssInfo =
		    GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
		if (!prBssInfo) {
			DBGLOG(SAA, ERROR, "prBssInfo is null\n");
			return WLAN_STATUS_INVALID_DATA;
		}
		ucStaRecIdx = prStaRec->ucIndex;
		ucBssIndex = prBssInfo->ucBssIndex;

		pucReceiveAddr = prStaRec->aucMacAddr;
	} else if (prBssInfo) {
		ucBssIndex = prBssInfo->ucBssIndex;
		ucStaRecIdx = STA_REC_INDEX_BMCAST;

		pucReceiveAddr = aucBMC;
	} else {
		DBGLOG(SAA, WARN, "Not to send Deauth, invalid data!\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	/* 4 <1.2> Find Transmitter Address and BSSID. */
	pucTransmitAddr = prBssInfo->aucOwnMacAddr;
	pucBssid = prBssInfo->aucBSSID;

	if (ucStaRecIdx != STA_REC_INDEX_BMCAST) {
		/* 4 <2> Check if already send a Deauth frame in
		 * MIN_DEAUTH_INTERVAL_MSEC
		 */
		GET_CURRENT_SYSTIME(&rCurrentTime);

		i4NewEntryIndex = -1;
		for (i = 0; i < MAX_DEAUTH_INFO_COUNT; i++) {
			prDeauthInfo = &(prAdapter->rWifiVar.arDeauthInfo[i]);

			/* For continuously sending Deauth frame, the minimum
			 * interval is MIN_DEAUTH_INTERVAL_MSEC.
			 */
			if (CHECK_FOR_TIMEOUT(rCurrentTime,
					      prDeauthInfo->rLastSendTime,
					      MSEC_TO_SYSTIME
					      (MIN_DEAUTH_INTERVAL_MSEC))) {

				i4NewEntryIndex = i;
			} else if (EQUAL_MAC_ADDR(pucReceiveAddr,
						  prDeauthInfo->aucRxAddr) &&
				   !pfTxDoneHandler) {
				return WLAN_STATUS_FAILURE;
			}
		}

		/* 4 <3> Update information. */
		if (i4NewEntryIndex >= 0) {
			prDeauthInfo =
			    &(prAdapter->
			      rWifiVar.arDeauthInfo[i4NewEntryIndex]);

			COPY_MAC_ADDR(prDeauthInfo->aucRxAddr, pucReceiveAddr);
			prDeauthInfo->rLastSendTime = rCurrentTime;
		} else {
			/* NOTE(Kevin): for the case of AP mode, we may
			 * encounter this case
			 * if deauth all the associated clients.
			 */
			DBGLOG(SAA, WARN, "No unused DEAUTH_INFO_T !\n");
		}
	}
	/* 4 <5> Allocate a PKT_INFO_T for Deauthentication Frame */
	/* Init with MGMT Header Length + Length of Fixed Fields + IE Length */
	u2EstimatedFrameLen =
	    (MAC_TX_RESERVED_FIELD + WLAN_MAC_MGMT_HEADER_LEN +
	     REASON_CODE_FIELD_LEN);

#if CFG_SUPPORT_ASSURANCE
	/* Assurance */
	if (prAdapter->u4DeauthIeFromUpperLength)
		u2EstimatedFrameLen += prAdapter->u4DeauthIeFromUpperLength;
#endif

	/* Allocate a MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(SAA, WARN,
		       "No PKT_INFO_T for sending Deauth Request.\n");
		return WLAN_STATUS_RESOURCES;
	}
	/* 4 <6> compose Deauthentication frame header and some fixed fields */
	authComposeDeauthFrameHeaderAndFF((uint8_t *)
					  ((uintptr_t)(prMsduInfo->prPacket)
					   + MAC_TX_RESERVED_FIELD),
					  pucReceiveAddr, pucTransmitAddr,
					  pucBssid, u2ReasonCode);

#if CFG_SUPPORT_802_11W
	/* AP PMF */
	if (rsnCheckBipKeyInstalled(prAdapter, prStaRec)) {
		uint8_t ucBssIndex = prStaRec->ucBssIndex;
		struct BSS_INFO *prBssInfo =
			GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

		if (!prBssInfo) {
			DBGLOG(SAA, ERROR, "prBssInfo is null\n");
			return WLAN_STATUS_INVALID_DATA;
		}
		/* PMF certification 4.3.3.1, 4.3.3.2 send unprotected
		 * deauth reason 6/7
		 * if (AP mode & not for PMF reply case) OR (STA PMF)
		 */
		if (((prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
		     && (prStaRec->rPmfCfg.fgRxDeauthResp != TRUE) &&
		     prStaRec->fgTransmitKeyExist == TRUE) ||
		    (prBssInfo->eNetworkType == NETWORK_TYPE_AIS)) {
			struct WLAN_DEAUTH_FRAME *prDeauthFrame;

			prDeauthFrame = (struct WLAN_DEAUTH_FRAME *)(uint8_t *)
			    ((uintptr_t)(prMsduInfo->prPacket)
			     + MAC_TX_RESERVED_FIELD);

			prDeauthFrame->u2FrameCtrl |= MASK_FC_PROTECTED_FRAME;
			if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
				aisGetAisFsmInfo(prAdapter, ucBssIndex)
					->encryptedDeauthIsInProcess = TRUE;
			}
			DBGLOG(SAA, INFO,
			       "Reason=%d, DestAddr=" MACSTR
			       " srcAddr=" MACSTR " BSSID=" MACSTR "\n",
			       prDeauthFrame->u2ReasonCode,
			       MAC2STR(prDeauthFrame->aucDestAddr),
			       MAC2STR(prDeauthFrame->aucSrcAddr),
			       MAC2STR(prDeauthFrame->aucBSSID));
		}
	}
#endif
	nicTxSetForceRts(prMsduInfo, TRUE);

	/* 4 <7> Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
		     prMsduInfo,
		     ucBssIndex,
		     ucStaRecIdx,
		     WLAN_MAC_MGMT_HEADER_LEN,
		     WLAN_MAC_MGMT_HEADER_LEN + REASON_CODE_FIELD_LEN,
		     pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

#if CFG_SUPPORT_ASSURANCE
	deauth_build_nonwfa_vend_ie(prAdapter, prMsduInfo);
#endif

#if CFG_SUPPORT_802_11W
	/* AP PMF */
	/* caution: access prStaRec only if true */
	if (rsnCheckBipKeyInstalled(prAdapter, prStaRec) &&
		prStaRec->fgTransmitKeyExist == TRUE) {
		/* 4.3.3.1 send unprotected deauth reason 6/7 */
		if (prStaRec->rPmfCfg.fgRxDeauthResp != TRUE) {
			DBGLOG(RSN, TRACE,
			       "Deauth Set MSDU_OPT_PROTECTED_FRAME\n");
			nicTxConfigPktOption(prMsduInfo,
					     MSDU_OPT_PROTECTED_FRAME, TRUE);
		}

		prStaRec->rPmfCfg.fgRxDeauthResp = FALSE;
	}
#endif
	DBGLOG(SAA, INFO, "ucTxSeqNum=%d ucStaRecIndex=%d u2ReasonCode=%d\n",
	       prMsduInfo->ucTxSeqNum, prMsduInfo->ucStaRecIndex, u2ReasonCode);

#if (CFG_SUPPORT_CONN_LOG == 1)
	connLogDeauth(prAdapter,
		prStaRec,
		prMsduInfo->ucTxSeqNum,
		u2ReasonCode);
#endif

	/* 4 <8> Inform TXM to send this Deauthentication frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}				/* end of authSendDeauthFrame() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will parse and process the incoming Deauthentication
 *        frame if the given BSSID is matched.
 *
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[in] aucBSSID           Given BSSID
 * @param[out] pu2ReasonCode     Pointer to store the Reason Code from
 *                               Deauthentication.
 *
 * @retval WLAN_STATUS_FAILURE   This is not the frame we should handle at
 *                               current state.
 * @retval WLAN_STATUS_SUCCESS   This is the frame we should handle.
 */
/*----------------------------------------------------------------------------*/
uint32_t authProcessRxDeauthFrame(struct SW_RFB *prSwRfb,
				  uint8_t aucBSSID[],
				  uint16_t *pu2ReasonCode)
{
	struct WLAN_DEAUTH_FRAME *prDeauthFrame;
	uint16_t u2RxReasonCode;

	if (!prSwRfb || !aucBSSID || !pu2ReasonCode) {
		DBGLOG(SAA, WARN, "Invalid parameters, ignore pkt!\n");
		return WLAN_STATUS_FAILURE;
	}

	/* 4 <1> locate the Deauthentication Frame. */
	prDeauthFrame = (struct WLAN_DEAUTH_FRAME *)prSwRfb->pvHeader;

	/* 4 <2> Parse the Header of Deauthentication Frame. */
#if 0				/* Kevin: Seems redundant */
	WLAN_GET_FIELD_16(&prDeauthFrame->u2FrameCtrl, &u2RxFrameCtrl)
	    u2RxFrameCtrl &= MASK_FRAME_TYPE;
	if (u2RxFrameCtrl != MAC_FRAME_DEAUTH)
		return WLAN_STATUS_FAILURE;

#endif

	if ((prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) <
	    REASON_CODE_FIELD_LEN) {
		DBGLOG(SAA, WARN, "Invalid Deauth packet length");
		return WLAN_STATUS_FAILURE;
	}

	/* Check if this Deauth Frame is coming from Target BSSID */
	if (UNEQUAL_MAC_ADDR(prDeauthFrame->aucBSSID, aucBSSID)) {
		DBGLOG(SAA, LOUD,
		       "Ignore Deauth Frame from other BSS [" MACSTR "]\n",
		       MAC2STR(prDeauthFrame->aucSrcAddr));
		return WLAN_STATUS_FAILURE;
	}
	/* 4 <3> Parse the Fixed Fields of Deauthentication Frame Body. */
	WLAN_GET_FIELD_16(&prDeauthFrame->u2ReasonCode, &u2RxReasonCode);
	*pu2ReasonCode = u2RxReasonCode;

	return WLAN_STATUS_SUCCESS;

}		/* end of authProcessRxDeauthFrame() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will parse and process the incoming Authentication
 *        frame.
 *
 * @param[in] prSwRfb                Pointer to SW RFB data structure.
 * @param[in] aucExpectedBSSID       Given Expected BSSID.
 * @param[in] u2ExpectedAuthAlgNum   Given Expected Authentication Algorithm
 *                                   Number
 * @param[in] u2ExpectedTransSeqNum  Given Expected Transaction Sequence Number.
 * @param[out] pu2ReturnStatusCode   Return Status Code.
 *
 * @retval WLAN_STATUS_SUCCESS   This is the frame we should handle.
 * @retval WLAN_STATUS_FAILURE   The frame we will ignore.
 */
/*----------------------------------------------------------------------------*/
uint32_t
authProcessRxAuth1Frame(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb,
			uint8_t aucExpectedBSSID[],
			uint16_t u2ExpectedAuthAlgNum,
			uint16_t u2ExpectedTransSeqNum,
			uint16_t *pu2ReturnStatusCode)
{
	struct WLAN_AUTH_FRAME *prAuthFrame;
	uint16_t u2ReturnStatusCode = STATUS_CODE_SUCCESSFUL;

	/* 4 <1> locate the Authentication Frame. */
	prAuthFrame = (struct WLAN_AUTH_FRAME *)prSwRfb->pvHeader;

	/* 4 <2> Check the BSSID */
	if (UNEQUAL_MAC_ADDR(prAuthFrame->aucBSSID, aucExpectedBSSID))
		return WLAN_STATUS_FAILURE;	/* Just Ignore this MMPDU */

	/* 4 <3> Check the SA, which should not be MC/BC */
	if (prAuthFrame->aucSrcAddr[0] & BIT(0)) {
		DBGLOG(P2P, WARN,
		       "Invalid STA MAC with MC/BC bit set: " MACSTR "\n",
		       MAC2STR(prAuthFrame->aucSrcAddr));
		return WLAN_STATUS_FAILURE;
	}

	/* 4 <4> Parse the Fixed Fields of Authentication Frame Body. */
	if (prAuthFrame->u2AuthAlgNum != u2ExpectedAuthAlgNum)
		u2ReturnStatusCode = STATUS_CODE_AUTH_ALGORITHM_NOT_SUPPORTED;

	if (prAuthFrame->u2AuthTransSeqNo != u2ExpectedTransSeqNum)
		u2ReturnStatusCode = STATUS_CODE_AUTH_OUT_OF_SEQ;

	*pu2ReturnStatusCode = u2ReturnStatusCode;

	return WLAN_STATUS_SUCCESS;

}				/* end of authProcessRxAuth1Frame() */

uint32_t
authProcessRxAuthFrame(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb,
			struct BSS_INFO *prBssInfo,
			uint16_t *pu2ReturnStatusCode)
{
	struct WLAN_AUTH_FRAME *prAuthFrame;
	uint16_t u2ReturnStatusCode = STATUS_CODE_SUCCESSFUL;

	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;

	/* 4 <1> locate the Authentication Frame. */
	prAuthFrame = (struct WLAN_AUTH_FRAME *)prSwRfb->pvHeader;

	/* 4 <2> Check the BSSID */
	if (UNEQUAL_MAC_ADDR(prAuthFrame->aucBSSID,
		prBssInfo->aucBSSID))
		return WLAN_STATUS_FAILURE;	/* Just Ignore this MMPDU */

	/* 4 <3> Check the SA, which should not be MC/BC */
	if (prAuthFrame->aucSrcAddr[0] & BIT(0)) {
		DBGLOG(P2P, WARN,
		       "Invalid STA MAC with MC/BC bit set: " MACSTR "\n",
		       MAC2STR(prAuthFrame->aucSrcAddr));
		return WLAN_STATUS_FAILURE;
	}

	/* 4 <4> Parse the Fixed Fields of Authentication Frame Body. */
	switch (prAuthFrame->u2AuthAlgNum) {
	case AUTH_ALGORITHM_NUM_OPEN_SYSTEM:
		if (prAuthFrame->u2AuthTransSeqNo != AUTH_TRANSACTION_SEQ_1)
			u2ReturnStatusCode = STATUS_CODE_AUTH_OUT_OF_SEQ;
		break;

	case AUTH_ALGORITHM_NUM_SAE:
		if (prAuthFrame->u2AuthTransSeqNo != AUTH_TRANSACTION_SEQ_1 &&
			prAuthFrame->u2AuthTransSeqNo != AUTH_TRANSACTION_SEQ_2)
			u2ReturnStatusCode = STATUS_CODE_AUTH_OUT_OF_SEQ;
		break;

#if CFG_SUPPORT_PASN
	case AUTH_ALGORITHM_NUM_PASN:
		if (prAuthFrame->u2AuthTransSeqNo != AUTH_TRANSACTION_SEQ_1 &&
			prAuthFrame->u2AuthTransSeqNo != AUTH_TRANSACTION_SEQ_3)
			u2ReturnStatusCode = STATUS_CODE_AUTH_OUT_OF_SEQ;
		break;
#endif

	default:
		u2ReturnStatusCode = STATUS_CODE_AUTH_ALGORITHM_NOT_SUPPORTED;
		break;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (!mldSanityCheck(prAdapter, prSwRfb->pvHeader,
		prSwRfb->u2PacketLen, NULL, prBssInfo->ucBssIndex)) {
		DBGLOG(AAA, WARN, "Discard Auth frame with wrong ML IE\n");
		return WLAN_STATUS_FAILURE;
	}
#endif

	DBGLOG(AAA, LOUD, "u2ReturnStatusCode = %d\n", u2ReturnStatusCode);

	*pu2ReturnStatusCode = u2ReturnStatusCode;

	return WLAN_STATUS_SUCCESS;

}

/* ToDo: authAddRicIE, authHandleFtIEs, authAddTimeoutIE */

void authAddMDIE(struct ADAPTER *prAdapter,
		 struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucBuffer =
		(uint8_t *)prMsduInfo->prPacket + prMsduInfo->u2FrameLength;
	uint8_t ucBssIdx = prMsduInfo->ucBssIndex;
	struct FT_IES *prFtIEs = aisGetFtIe(prAdapter, ucBssIdx, AIS_FT_R0);

	if (!prFtIEs || !prFtIEs->prMDIE ||
	    !rsnIsFtOverTheAir(prAdapter, ucBssIdx, prMsduInfo->ucStaRecIndex))
		return;
	prMsduInfo->u2FrameLength +=
		5; /* IE size for MD IE is fixed, it is 5 */
	kalMemCopy(pucBuffer, prFtIEs->prMDIE, 5);
}

uint32_t authCalculateFTIELen(struct ADAPTER *prAdapter, uint8_t ucBssIdx,
			     struct STA_RECORD *prStaRec)
{
	/* Use R0 with auth, R1 with assoc */
	struct FT_IES *prFtIEs = aisGetFtIe(prAdapter, ucBssIdx, AIS_FT_R0);

	if (!prFtIEs || !prFtIEs->prFTIE || !prStaRec ||
	    !rsnIsFtOverTheAir(prAdapter, ucBssIdx, prStaRec->ucIndex))
		return 0;
	return IE_SIZE(prFtIEs->prFTIE);
}

void authAddFTIE(struct ADAPTER *prAdapter, struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucBuffer =
		(uint8_t *)prMsduInfo->prPacket + prMsduInfo->u2FrameLength;
	uint32_t ucFtIeSize = 0;
	uint8_t ucBssIdx = prMsduInfo->ucBssIndex;
	/* Use R0 with auth, R1 with assoc */
	struct FT_IES *prFtIEs = aisGetFtIe(prAdapter, ucBssIdx, AIS_FT_R0);

	if (!prFtIEs || !prFtIEs->prFTIE ||
	    !rsnIsFtOverTheAir(prAdapter, ucBssIdx, prMsduInfo->ucStaRecIndex))
		return;
	ucFtIeSize = IE_SIZE(prFtIEs->prFTIE);
	prMsduInfo->u2FrameLength += ucFtIeSize;
	kalMemCopy(pucBuffer, prFtIEs->prFTIE, ucFtIeSize);
}

uint32_t authCalculateRSNIELen(struct ADAPTER *prAdapter, uint8_t ucBssIdx,
			       struct STA_RECORD *prStaRec)
{
	struct FT_IES *prFtIEs = aisGetFtIe(prAdapter, ucBssIdx, AIS_FT_R0);

	if (!prFtIEs || !prFtIEs->prRsnIE || !prStaRec ||
	    !rsnIsFtOverTheAir(prAdapter, ucBssIdx, prStaRec->ucIndex))
		return 0;
	return IE_SIZE(prFtIEs->prRsnIE);
}

void authAddRSNIE(struct ADAPTER *prAdapter,
		  struct MSDU_INFO *prMsduInfo)
{
	struct STA_RECORD *prStaRec;

	prStaRec = cnmGetStaRecByIndex(prAdapter,
		prMsduInfo->ucStaRecIndex);
	if (!prStaRec)
		return;

	if (rsnIsFtOverTheAir(prAdapter, prStaRec->ucBssIndex,
			prStaRec->ucIndex)) {
		authAddRSNIE_impl(prAdapter, prMsduInfo, AIS_FT_R0);
		return;
	}

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	if (rsnIsFilsAuthAlg(prStaRec->ucAuthAlgNum)) {
		rsnGenerateRSNIEImpl(prAdapter, prMsduInfo);
		return;
	}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
}

uint32_t authAddRSNIE_impl(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		uint8_t ucR0R1)
{
	uint8_t *pucBuffer =
		(uint8_t *)prMsduInfo->prPacket + prMsduInfo->u2FrameLength;
	uint32_t ucRSNIeSize = 0;
	uint8_t ucBssIdx = prMsduInfo->ucBssIndex;
	struct FT_IES *prFtIEs = aisGetFtIe(prAdapter, ucBssIdx, ucR0R1);

	if (!prFtIEs || !prFtIEs->prRsnIE ||
	    !rsnIsFtOverTheAir(prAdapter, ucBssIdx, prMsduInfo->ucStaRecIndex))
		return FALSE;

	ucRSNIeSize = IE_SIZE(prFtIEs->prRsnIE);
	prMsduInfo->u2FrameLength += ucRSNIeSize;
	kalMemCopy(pucBuffer, prFtIEs->prRsnIE, ucRSNIeSize);
	DBGDUMP_MEM8(SAA, INFO, "FT: Generate RSNE\n", pucBuffer, ucRSNIeSize);
	return TRUE;
}

#if CFG_SUPPORT_AAA
/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Auth Frame and then return
 *        the status code to AAA to indicate
 *        if need to perform following actions
 *        when the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 *
 * @retval TRUE      Reply the Auth
 * @retval FALSE     Don't reply the Auth
 */
/*---------------------------------------------------------------------------*/
u_int8_t
authFloodingCheck(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		struct SW_RFB *prSwRfb)
{

	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;
	struct WLAN_AUTH_FRAME *prAuthFrame = (struct WLAN_AUTH_FRAME *) NULL;

	DBGLOG(SAA, TRACE, "authFloodingCheck Authentication Frame\n");

	prAuthFrame = (struct WLAN_AUTH_FRAME *) prSwRfb->pvHeader;

	if ((prP2pBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) ||
		(prP2pBssInfo->eIntendOPMode != OP_MODE_NUM)) {
		/* We are not under AP Mode yet. */
		DBGLOG(P2P, WARN,
			"Current OP mode is not under AP mode. (%d)\n",
			prP2pBssInfo->eCurrentOPMode);
		return FALSE;
	}

	prStaRec = cnmGetStaRecByAddress(prAdapter,
		prP2pBssInfo->ucBssIndex, prAuthFrame->aucSrcAddr);

	if (!prStaRec) {
		DBGLOG(SAA, TRACE, "Need reply.\n");
		return TRUE;
	}

	DBGLOG(SAA, WARN, "Auth Flooding Attack, don't reply.\n");
	return FALSE;
}

#endif
