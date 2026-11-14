// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "twt.c"
*   \brief  Functions for processing TWT related elements and frames.
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

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static void twtFillTWTElement(
	struct _IE_TWT_T *prTWTBuf,
	uint8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams);

static void twtParseTWTElement(
	struct _IE_TWT_T *prTWTIE,
	struct _TWT_PARAMS_T *prTWTParams);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
/*----------------------------------------------------------------------------*/
/*!
* \brief Send TWT Setup frame (S1G action frame)
*
* \param[in] prAdapter ADAPTER structure
*            prStaRec station record structure
*
* \return none
*/
/*----------------------------------------------------------------------------*/
uint32_t twtSendSetupFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _ACTION_TWT_SETUP_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	struct _IE_TWT_T *prTWTBuf;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTParams\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD
		+ sizeof(struct _ACTION_TWT_SETUP_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)
			cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, WARN,
			"No MSDU_INFO_T for sending TWT Setup Frame.\n");
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
	prTxFrame->ucCategory = CATEGORY_S1G_ACTION;
	prTxFrame->ucAction = ACTION_S1G_TWT_SETUP;

	prTWTBuf = &(prTxFrame->rTWT);
	twtFillTWTElement(prTWTBuf, ucTWTFlowId, prTWTParams);

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			WLAN_MAC_MGMT_HEADER_LEN,
			sizeof(struct _ACTION_TWT_SETUP_FRAME),
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
			MSDU_CONTROL_FLAG_FORCE_LINK |
			MSDU_CONTROL_FLAG_DIS_MAT,
			TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* Enqueue the frame to send this action frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

uint32_t twtSendTeardownFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _ACTION_TWT_TEARDOWN_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
		sizeof(struct _ACTION_TWT_TEARDOWN_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *) cnmMgtPktAlloc(
		prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, WARN,
			"No MSDU_INFO_T for sending TWT Teardown Frame.\n");
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
	prTxFrame->ucCategory = CATEGORY_S1G_ACTION;
	prTxFrame->ucAction = ACTION_S1G_TWT_TEARDOWN;
	prTxFrame->ucTWTFlow = ucTWTFlowId;

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			WLAN_MAC_MGMT_HEADER_LEN,
			sizeof(struct _ACTION_TWT_TEARDOWN_FRAME),
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
			MSDU_CONTROL_FLAG_FORCE_LINK |
			MSDU_CONTROL_FLAG_DIS_MAT,
			TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* Enqueue the frame to send this action frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

uint32_t twtSendInfoFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	struct _NEXT_TWT_INFO_T *prNextTWTInfo,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _ACTION_TWT_INFO_FRAME *prTxFrame;
	uint32_t u4Pos =
		OFFSET_OF(struct _ACTION_TWT_INFO_FRAME, aucNextTWT[0]);
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
		sizeof(struct _ACTION_TWT_INFO_FRAME) +
		twtGetNextTWTByteCnt(prNextTWTInfo->ucNextTWTSize);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *) cnmMgtPktAlloc(
		prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, WARN,
			"No MSDU_INFO_T for sending TWT Info Frame.\n");
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
	prTxFrame->ucCategory = CATEGORY_S1G_ACTION;
	prTxFrame->ucAction = ACTION_S1G_TWT_INFORMATION;
	prTxFrame->ucNextTWTCtrl = (ucTWTFlowId & TWT_INFO_FLOW_ID) |
		((prNextTWTInfo->ucNextTWTSize  <<
		TWT_INFO_NEXT_TWT_SIZE_OFFSET) & TWT_INFO_NEXT_TWT_SIZE);

	switch (prNextTWTInfo->ucNextTWTSize) {
	case NEXT_TWT_SUBFIELD_64_BITS:
	{
		uint64_t *pu8NextTWT =
			(uint64_t *)(((uint8_t *)prTxFrame) + u4Pos);
		*pu8NextTWT = CPU_TO_LE64(prNextTWTInfo->u8NextTWT);
		break;
	}

	case NEXT_TWT_SUBFIELD_32_BITS:
	{
		uint32_t *pu4NextTWT =
			(uint32_t *)(((uint8_t *)prTxFrame) + u4Pos);
		*pu4NextTWT = CPU_TO_LE32(
			(uint32_t)(prNextTWTInfo->u8NextTWT & 0xFFFFFFFF));

		DBGLOG(TWT_REQUESTER, WARN,
			"TWT Info Frame 0x%x 0x%llx\n",
			*pu4NextTWT, prNextTWTInfo->u8NextTWT);

		break;
	}

	case NEXT_TWT_SUBFIELD_48_BITS:
	{
		uint8_t *pucMem = ((uint8_t *)prTxFrame) + u4Pos;
		/* little endian placement */
		*pucMem = prNextTWTInfo->u8NextTWT & 0xFF;
		*(pucMem + 1) = (prNextTWTInfo->u8NextTWT >> 8) & 0xFF;
		*(pucMem + 2) = (prNextTWTInfo->u8NextTWT >> 16) & 0xFF;
		*(pucMem + 3) = (prNextTWTInfo->u8NextTWT >> 24) & 0xFF;
		*(pucMem + 4) = (prNextTWTInfo->u8NextTWT >> 32) & 0xFF;
		*(pucMem + 5) = (prNextTWTInfo->u8NextTWT >> 40) & 0xFF;
		break;
	}

	default:
		break;
	}

	DBGLOG(TWT_REQUESTER, WARN,
			"TWT Info Frame %u %zu %u 0x%x 0x%x 0x%llx\n",
			u2EstimatedFrameLen,
			sizeof(struct _ACTION_TWT_INFO_FRAME),
			twtGetNextTWTByteCnt(
				prNextTWTInfo->ucNextTWTSize),
			prTxFrame->ucNextTWTCtrl,
			twtGetNextTWTByteCnt(
				prNextTWTInfo->ucNextTWTSize) == 0 ?
			0 : *((uint32_t *)(((uint8_t *)prTxFrame) + u4Pos)),
			prNextTWTInfo->u8NextTWT);

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			WLAN_MAC_MGMT_HEADER_LEN,
			(sizeof(struct _ACTION_TWT_INFO_FRAME) +
				twtGetNextTWTByteCnt(
					prNextTWTInfo->ucNextTWTSize)),
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
			MSDU_CONTROL_FLAG_FORCE_LINK |
			MSDU_CONTROL_FLAG_DIS_MAT,
			TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* Enqueue the frame to send this action frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

static void twtFillTWTElement(
	struct _IE_TWT_T *prTWTBuf,
	uint8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams)
{
	if (!prTWTBuf) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTBuf\n");

		return;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTParams\n");

		return;
	}

	/* Add TWT element */
	prTWTBuf->ucId = ELEM_ID_TWT;
	prTWTBuf->ucLength = sizeof(struct _IE_TWT_T) - ELEM_HDR_LEN;

	/* Request Type */
	prTWTBuf->u2ReqType |= SET_TWT_RT_REQUEST(prTWTParams->fgReq) |
		SET_TWT_RT_SETUP_CMD(prTWTParams->ucSetupCmd) |
		SET_TWT_RT_TRIGGER(prTWTParams->fgTrigger) |
		TWT_REQ_TYPE_IMPLICIT_LAST_BCAST_PARAM |
		SET_TWT_RT_FLOW_TYPE(prTWTParams->fgUnannounced) |
		SET_TWT_RT_FLOW_ID(ucTWTFlowId) |
		SET_TWT_RT_WAKE_INTVAL_EXP(prTWTParams->ucWakeIntvalExponent) |
		SET_TWT_RT_PROTECTION(prTWTParams->fgProtect);

	prTWTBuf->u8TWT = CPU_TO_LE64(prTWTParams->u8TWT);
	prTWTBuf->ucMinWakeDur = prTWTParams->ucMinWakeDur;
	prTWTBuf->u2WakeIntvalMantiss =
		CPU_TO_LE16(prTWTParams->u2WakeIntvalMantiss);
}

#if (CFG_SUPPORT_BTWT == 1)
static void btwtParseTWTElement(
	struct _IE_BTWT_T *prBTWTIE,
	struct _TWT_PARAMS_T *prTWTParams)
{
	uint16_t u2ReqType;
	uint64_t u8TargetWakeTime;
	uint64_t u8twt_interval = 0;
	uint64_t u8Mod = 0;
	uint64_t u8Temp;

	if (!prBTWTIE) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prBTWTIE\n");

		return;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTParams\n");

		return;
	}

	if (prBTWTIE->u2WakeIntvalMantiss == 0)	{
		/* For coverity check */
		return;
	}

	u2ReqType = LE16_TO_CPU(prBTWTIE->u2ReqType);
	prTWTParams->fgReq = GET_TWT_RT_REQUEST(u2ReqType);
	prTWTParams->ucSetupCmd = GET_TWT_RT_SETUP_CMD(u2ReqType);
	prTWTParams->fgTrigger = GET_TWT_RT_TRIGGER(u2ReqType);
	prTWTParams->fgUnannounced = GET_TWT_RT_FLOW_TYPE(u2ReqType);
	prTWTParams->ucWakeIntvalExponent =
		GET_TWT_RT_WAKE_INTVAL_EXP(u2ReqType);

	prTWTParams->ucMinWakeDur = prBTWTIE->ucMinWakeDur;
	prTWTParams->u2WakeIntvalMantiss =
		LE16_TO_CPU(prBTWTIE->u2WakeIntvalMantiss);

	DBGLOG(TWT_REQUESTER, WARN,
		"BTWT cur TSF %x %x\nAP resp TWT %x\n",
		(uint32_t)(prTWTParams->u8TWT & 0x00000000FFFFFFFF),
		(uint32_t)((prTWTParams->u8TWT & 0xFFFFFFFF00000000) >> 32),
		prBTWTIE->u2TWT);

	u8TargetWakeTime = prTWTParams->u8TWT;

	/* see Draft P802.11ax_D7.0.pdf, p421 */
	u8TargetWakeTime &= 0xFFFFFFFFFFFFFC00;
	u8TargetWakeTime |= (uint32_t)(prBTWTIE->u2TWT << 10);

	DBGLOG(TWT_REQUESTER, WARN,
		"BTWT cur TSF sync with AP %x %x\n",
		(uint32_t)(u8TargetWakeTime & 0x00000000FFFFFFFF),
		(uint32_t)((u8TargetWakeTime & 0xFFFFFFFF00000000) >> 32));

	u8twt_interval = (((u_int64_t)prTWTParams->u2WakeIntvalMantiss)
		<< prTWTParams->ucWakeIntvalExponent);
	u8Temp = u8TargetWakeTime + u8twt_interval;
	u8Mod = kal_mod64(u8Temp, u8twt_interval);
	prTWTParams->u8TWT = u8TargetWakeTime + (u8twt_interval - u8Mod);

	DBGLOG(TWT_REQUESTER, WARN,
		"BTWT twt %x %x\n",
		(uint32_t)(prTWTParams->u8TWT & 0x00000000FFFFFFFF),
		(uint32_t)((prTWTParams->u8TWT & 0xFFFFFFFF00000000) >> 32));
}

uint8_t btwtGetTxSetupFlowId(
	struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucFlowId;
	struct _ACTION_BTWT_SETUP_FRAME *prTxFrame;

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prMsduInfo\n");

		return TWT_INCORRECT_FLOW_ID;
	}

	prTxFrame = (struct _ACTION_BTWT_SETUP_FRAME *)(prMsduInfo->prPacket);
	ucFlowId = GET_BTWT_ID(prTxFrame->rTWT.u2BTWTInfo);

	return ucFlowId;
}
#endif

static void twtParseTWTElement(
	struct _IE_TWT_T *prTWTIE,
	struct _TWT_PARAMS_T *prTWTParams)
{
	uint16_t u2ReqType;

	u2ReqType = LE16_TO_CPU(prTWTIE->u2ReqType);

	prTWTParams->fgReq = GET_TWT_RT_REQUEST(u2ReqType);
	prTWTParams->ucSetupCmd = GET_TWT_RT_SETUP_CMD(u2ReqType);
	prTWTParams->fgTrigger = GET_TWT_RT_TRIGGER(u2ReqType);
	prTWTParams->fgUnannounced = GET_TWT_RT_FLOW_TYPE(u2ReqType);

	prTWTParams->ucWakeIntvalExponent =
		GET_TWT_RT_WAKE_INTVAL_EXP(u2ReqType);

	prTWTParams->fgProtect = GET_TWT_RT_PROTECTION(u2ReqType);
	prTWTParams->u8TWT = LE64_TO_CPU(prTWTIE->u8TWT);
	prTWTParams->ucMinWakeDur = prTWTIE->ucMinWakeDur;

	prTWTParams->u2WakeIntvalMantiss =
		LE16_TO_CPU(prTWTIE->u2WakeIntvalMantiss);
}

uint8_t twtGetTxSetupFlowId(
	struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucFlowId;
	struct _ACTION_TWT_SETUP_FRAME *prTxFrame;

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prMsduInfo\n");

		return TWT_INCORRECT_FLOW_ID;
	}

	prTxFrame = (struct _ACTION_TWT_SETUP_FRAME *)(prMsduInfo->prPacket);
	ucFlowId = GET_TWT_RT_FLOW_ID(prTxFrame->rTWT.u2ReqType);

	return ucFlowId;
}

uint8_t twtGetTxTeardownFlowId(
	struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucFlowId;
	struct _ACTION_TWT_TEARDOWN_FRAME *prTxFrame;

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prMsduInfo\n");

		return TWT_INCORRECT_FLOW_ID;
	}

	prTxFrame = (struct _ACTION_TWT_TEARDOWN_FRAME *)(prMsduInfo->prPacket);
	ucFlowId = (prTxFrame->ucTWTFlow & TWT_TEARDOWN_FLOW_ID);

	return ucFlowId;
}

uint8_t twtGetTxInfoFlowId(
	struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucFlowId;
	struct _ACTION_TWT_INFO_FRAME *prTxFrame;

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prMsduInfo\n");

		return TWT_INCORRECT_FLOW_ID;
	}

	prTxFrame = (struct _ACTION_TWT_INFO_FRAME *)(prMsduInfo->prPacket);
	ucFlowId = GET_TWT_INFO_FLOW_ID(prTxFrame->ucNextTWTCtrl);

	return ucFlowId;
}

uint8_t twtGetRxSetupFlowId(
	struct _IE_TWT_T *prTWTIE)
{
	uint16_t u2ReqType;

	if (!prTWTIE) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTIE\n");

		return TWT_INCORRECT_FLOW_ID;
	}

	u2ReqType = LE16_TO_CPU(prTWTIE->u2ReqType);

	return GET_TWT_RT_FLOW_ID(u2ReqType);
}

void twtProcessS1GAction(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct WLAN_ACTION_FRAME *prRxFrame;
	struct _ACTION_TWT_SETUP_FRAME *prRxSetupFrame = NULL;
	struct _ACTION_TWT_TEARDOWN_FRAME *prRxTeardownFrame = NULL;
	struct _ACTION_TWT_INFO_FRAME *prRxInfoFrame = NULL;
	struct STA_RECORD *prStaRec;
	struct RX_DESC_OPS_T *prRxDescOps;

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	uint8_t ucTWTHotspotRejectSetupReq = 0;
	struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode = NULL;
	struct _MSG_TWT_HOTSPOT_PARAMS_SET_T *prTWTHotspotParamSetMsg = NULL;
	struct _TWT_HOTSPOT_CTRL_T *prTWTHotspotCtrl = NULL;
	struct _TWT_PARAMS_T *prTWTHotspotParam = NULL;
#endif

	uint8_t ucTWTFlowId = 0;
	uint32_t u4Offset;
	uint16_t u2ReqType;
#if (CFG_SUPPORT_BTWT == 1)
	uint8_t ucRecommendation = 0;
#endif
	struct _TWT_FLOW_T *prTWTFlow = NULL;
	enum _ENUM_TWT_TYPE_T eTwtType = ENUM_TWT_TYPE_DEFAULT;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return;
	}

	if (!prSwRfb) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prSwRfb\n");

		return;
	}

	prRxDescOps = prAdapter->chip_info->prRxDescOps;

	if (!prRxDescOps->nic_rxd_get_rx_byte_count) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid nic_rxd_get_rx_byte_count\n");

		return;
	}

	if (!prRxDescOps->nic_rxd_get_pkt_type) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid nic_rxd_get_pkt_type\n");

		return;
	}

	if (!prRxDescOps->nic_rxd_get_wlan_idx) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid nic_rxd_get_wlan_idx\n");

		return;
	}

	prRxFrame = (struct WLAN_ACTION_FRAME *)prSwRfb->pvHeader;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, WARN,
		"Received an S1G Action: wlanIdx[%d] w/o corresponding staRec\n"
		, prRxDescOps->nic_rxd_get_wlan_idx(prSwRfb->prRxStatus));
		return;
	}

	switch (prRxFrame->ucAction) {
	case ACTION_S1G_TWT_SETUP:
		prRxSetupFrame =
			(struct _ACTION_TWT_SETUP_FRAME *) prSwRfb->pvHeader;
#if (CFG_SUPPORT_BTWT == 1)
		if (prStaRec->ucStaState != STA_STATE_3 ||
			prSwRfb->u2PacketLen <
				sizeof(struct _ACTION_BTWT_SETUP_FRAME)) {
			DBGLOG(TWT_REQUESTER, WARN,
				"improper TWT Setupframe %d\n",
				prStaRec->ucStaState);
			return;
		}
#else
		if (prStaRec->ucStaState != STA_STATE_3 ||
			prSwRfb->u2PacketLen <
				sizeof(struct _ACTION_TWT_SETUP_FRAME)) {
			DBGLOG(TWT_REQUESTER, WARN,
				"Received improper TWT Setup frame\n");
			return;
		}
#endif

/* TWT hotspot code segment::Begin
 * For TWT hotspot, we handle req type only, leave the
 * existing code flow unchanged for the ease of maintain.
 */
#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
		u2ReqType = prRxSetupFrame->rTWT.u2ReqType;

		if ((u2ReqType & TWT_REQ_TYPE_TWT_REQUEST)) {
			/* TWT hotspot receives STA's TWT setup request */
			/* Check if TWT hotspot is enabled */
			if (IS_FEATURE_ENABLED(
				prAdapter->rWifiVar.ucTWTHotSpotSupport)) {

				if ((u2ReqType & TWT_REQ_TYPE_TRIGGER) ||
				(!(u2ReqType & TWT_REQ_TYPE_FLOWTYPE))) {
				/* Accepts only non-trigger & unannounce */
				DBGLOG(TWT_REQUESTER, ERROR,
					"Reject TWT Setup params tr[%lu] ua[%lu]\n",
					(u2ReqType & TWT_REQ_TYPE_TRIGGER),
					(u2ReqType & TWT_REQ_TYPE_FLOWTYPE));

					ucTWTHotspotRejectSetupReq = TRUE;
				}

				if (ucTWTHotspotRejectSetupReq == FALSE) {
					/* Get free flow id */
					twtHotspotGetFreeFlowId(
						prAdapter,
						prStaRec,
						&ucTWTFlowId);

					if (ucTWTFlowId !=
						TWT_HOTSPOT_NO_MORE_FLOW_ID) {
					/* Accept, next to
					* send TWT setup resp frame
					*/
						ucTWTHotspotRejectSetupReq
							= FALSE;

						prStaRec->ucTWTFlowId
							= ucTWTFlowId;
					} else {
						/* No more flow ID avail,
						*  just reject
						*/
						ucTWTHotspotRejectSetupReq
							= TRUE;
					}
				}
			} else {
				/* TWT hotspot not enabled, just reject */
				ucTWTHotspotRejectSetupReq = TRUE;
			}

			if (ucTWTHotspotRejectSetupReq == FALSE) {
				twtHotspotGetFreeStaNode(
					prAdapter,
					prStaRec,
					&prTWTHotspotStaNode);

				if (prTWTHotspotStaNode == NULL) {
					/* No sta node avail,
					* just reject
					*/
					ucTWTHotspotRejectSetupReq
						= TRUE;
				} else {
					prTWTHotspotStaNode->flow_id
						= ucTWTFlowId;
					prTWTHotspotStaNode->prStaRec
						= prStaRec;
				}

				DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]IsReject=%d\n",
					ucTWTHotspotRejectSetupReq);
			}

			/* If this is a reject, prTWTHotspotStaNode = NULL */
			prStaRec->prTWTHotspotStaNode
				= prTWTHotspotStaNode;

			prTWTHotspotParamSetMsg = cnmMemAlloc(
				prAdapter, RAM_TYPE_MSG,
				sizeof(struct _MSG_TWT_HOTSPOT_PARAMS_SET_T));

			if (prTWTHotspotParamSetMsg) {
				prTWTHotspotCtrl =
					&prTWTHotspotParamSetMsg->rTWTCtrl;
				prTWTHotspotCtrl->ucBssIdx
					= prStaRec->ucBssIndex;
				prTWTHotspotCtrl->ucCtrlAction
					= TWT_PARAM_ACTION_ADD;
				prTWTHotspotCtrl->ucTWTFlowId = ucTWTFlowId;
				prTWTHotspotCtrl->ucIsReject
					= ucTWTHotspotRejectSetupReq;
				prTWTHotspotCtrl->ucDialogToken =
					prRxSetupFrame->ucDialogToken;
				prTWTHotspotCtrl->prStaRec = prStaRec;

				DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]B=%d lA=%d F=%d RJ=%d Tkn=%d\n",
					prTWTHotspotCtrl->ucBssIdx,
					prTWTHotspotCtrl->ucCtrlAction,
					prTWTHotspotCtrl->ucTWTFlowId,
					prTWTHotspotCtrl->ucIsReject,
					prTWTHotspotCtrl->ucDialogToken);

				prTWTHotspotParam =
					&prTWTHotspotCtrl->rTWTParams;
				prTWTHotspotParam->fgReq = FALSE;
				prTWTHotspotParam->fgTrigger =
					(u2ReqType & TWT_REQ_TYPE_TRIGGER) ?
						TRUE : FALSE;
				prTWTHotspotParam->fgProtect =
					(u2ReqType &
					TWT_REQ_TYPE_TWT_PROTECTION) ?
						TRUE : FALSE;
				prTWTHotspotParam->fgUnannounced =
					(u2ReqType & TWT_REQ_TYPE_FLOWTYPE) ?
						TRUE : FALSE;

				/* 1st stage Accept(Reject)
				* is determined over here
				*/
				prTWTHotspotParam->ucSetupCmd =
					(ucTWTHotspotRejectSetupReq) ?
						TWT_SETUP_CMD_ID_REJECT :
						TWT_SETUP_CMD_ID_ACCEPT;

				prTWTHotspotParam->ucMinWakeDur =
					prRxSetupFrame->rTWT.ucMinWakeDur;
				prTWTHotspotParam->ucWakeIntvalExponent =
					(u2ReqType &
					TWT_REQ_TYPE_TWT_WAKE_INTVAL_EXP) >>
					TWT_REQ_TYPE_TWT_WAKE_INTVAL_EXP_OFFSET;
				prTWTHotspotParam->u2WakeIntvalMantiss =
				prRxSetupFrame->rTWT.u2WakeIntvalMantiss;
				prTWTHotspotParam->u8TWT =
					prRxSetupFrame->rTWT.u8TWT;

				kalMemCopy(
					&prStaRec->TWTHotspotCtrl,
					prTWTHotspotCtrl,
					sizeof(struct _TWT_HOTSPOT_CTRL_T));

				if (ucTWTHotspotRejectSetupReq == FALSE) {
					/* Prepare to get
					*  the nearest target tsf
					*/
					prTWTHotspotStaNode->agrt_sp_duration =
						prTWTHotspotParam->
							ucMinWakeDur;
					prTWTHotspotStaNode->
						agrt_sp_wake_intvl_exponent =
						prTWTHotspotParam->
							ucWakeIntvalExponent;
					prTWTHotspotStaNode->
						agrt_sp_wake_intvl_mantissa =
						prRxSetupFrame->
						rTWT.u2WakeIntvalMantiss;
					prTWTHotspotStaNode->
						agrt_sp_start_tsf =
							prTWTHotspotParam->
								u8TWT;
					prTWTHotspotStaNode->
						schedule_sp_start_tsf = 0;
				}

				prTWTHotspotParamSetMsg->rMsgHdr.eMsgId =
					MID_TWT_RESP_PARAMS_SET;

				mboxSendMsg(prAdapter, MBOX_ID_0,
				(struct MSG_HDR *) prTWTHotspotParamSetMsg,
				MSG_SEND_METHOD_BUF);
			}

			return;
		}
#endif
/* TWT hotspot code segment::End
 * For TWT hotspot, we handle req type only, leave the
 * existing code flow unchanged for the ease of maintain.
 */

/* TWT STA code segment::Begin
 * i-TWT/BTWT/ML-TWT handling goes over here.
 */
#if (CFG_SUPPORT_BTWT == 1)
		if (GET_BTWT_CTRL_NEGO(
			((struct _IE_BTWT_T *)&(prRxSetupFrame->rTWT))->ucCtrl)
			== 3) {
			/* BTWT setup resp */
			ucTWTFlowId = GET_BTWT_ID(
			((struct _IE_BTWT_T *)&(prRxSetupFrame->rTWT))
				->u2BTWTInfo);

			DBGLOG(RLM, WARN, "BTWT params here\n");

			if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
				(ucTWTFlowId >= RTWT_MAX_FLOW_NUM)) {
				DBGLOG(TWT_REQUESTER, ERROR,
					"TWT_INCORRECT_FLOW_ID %d\n",
					ucTWTFlowId);

				return;
			}

			DBGLOG(TWT_REQUESTER, WARN,
				"Rx BTWT params: %x %x %x %x %x\n",
		((struct _IE_BTWT_T *)&(prRxSetupFrame->rTWT))->u2ReqType,
		((struct _IE_BTWT_T *)&(prRxSetupFrame->rTWT))->u2TWT,
		((struct _IE_BTWT_T *)&(prRxSetupFrame->rTWT))->ucMinWakeDur,
		((struct _IE_BTWT_T *)&
			(prRxSetupFrame->rTWT))->u2WakeIntvalMantiss,
		((struct _IE_BTWT_T *)&(prRxSetupFrame->rTWT))->u2BTWTInfo);

			ucRecommendation = GET_BTWT_RECOMMENDATION(
				((struct _IE_BTWT_T *)
				&(prRxSetupFrame->rTWT))->u2ReqType);

			prTWTFlow = twtPlannerFlowFindById(
					prStaRec, ucTWTFlowId,
					(ucRecommendation == 0) ?
						ENUM_TWT_TYPE_BTWT
						: ENUM_TWT_TYPE_RTWT);

			if (prTWTFlow == NULL) {
				DBGLOG(TWT_REQUESTER, ERROR,
					"NULL TWT flow %d recom %d\n",
					ucTWTFlowId, ucRecommendation);

				return;
			}

			if (ucRecommendation == 0) {
				btwtParseTWTElement(
					((struct _IE_BTWT_T *)
						&(prRxSetupFrame->rTWT)),
					&(prTWTFlow->rTWTPeerParams));

				prTWTFlow->eTwtType = ENUM_TWT_TYPE_BTWT;

				eTwtType = ENUM_TWT_TYPE_BTWT;
			}
#if (CFG_SUPPORT_RTWT == 1)
			else if (ucRecommendation == 4) {
				rtwtParseTWTElement(
					((struct _IE_RTWT_T *)
						&(prRxSetupFrame->rTWT)),
					&(prTWTFlow->rTWTPeerParams));

				prTWTFlow->eTwtType = ENUM_TWT_TYPE_RTWT;

				eTwtType = ENUM_TWT_TYPE_RTWT;
			}
#endif
		} else {
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
			if (GET_ML_TWT_CTRL_LINK_ID_BITMAP(
				((struct IE_ML_TWT_T *)&(prRxSetupFrame->rTWT))->ucCtrl)
				== 1) {
				/* We receive multi-link TWT response frame */
				/* Parse TWT element in the unit of link ID bitmap */
				/* Iterate all link IDs related STA_REC in MLD_GRP */
				if (mltwtParseTWTElement(
					prAdapter,
					prStaRec,
					(uint8_t *)&(prRxSetupFrame->rTWT),
					prSwRfb->u2PacketLen -
					sizeof(
					struct _ACTION_ML_TWT_SETUP_FRAME_PER_LINK_DISTINCT)
					) != WLAN_STATUS_SUCCESS) {
					DBGLOG(TWT_REQUESTER, ERROR,
						"MLTWT setup resp\n");

					return;
				}
			} else {
#endif

				/* Parse TWT element */
				ucTWTFlowId =
					twtGetRxSetupFlowId(
						&(prRxSetupFrame->rTWT));

				if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
					(ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
					DBGLOG(TWT_REQUESTER, ERROR,
						"TWT_INCORRECT_FLOW_ID %d\n",
						ucTWTFlowId);

					return;
				}

				prTWTFlow = twtPlannerFlowFindById(
						prStaRec, ucTWTFlowId,
						ENUM_TWT_TYPE_ITWT);

				if (prTWTFlow == NULL) {
					DBGLOG(TWT_REQUESTER, ERROR,
						"NULL TWT flow %d type %d\n",
						ucTWTFlowId,
						ENUM_TWT_TYPE_ITWT);

					return;
				}

				twtParseTWTElement(
					&(prRxSetupFrame->rTWT),
					&(prTWTFlow->rTWTPeerParams));

				prTWTFlow->eTwtType = ENUM_TWT_TYPE_ITWT;

				eTwtType = ENUM_TWT_TYPE_ITWT;

#if (CFG_SUPPORT_BTWT == 1)
			}
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
		}
#endif

		/* Notify TWT Requester FSM upon reception of a TWT response */
		u2ReqType = prRxSetupFrame->rTWT.u2ReqType;
		if (!(u2ReqType & TWT_REQ_TYPE_TWT_REQUEST)) {
#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
			if (GET_ML_TWT_CTRL_LINK_ID_BITMAP(
				((struct IE_ML_TWT_T *)
				&(prRxSetupFrame->rTWT))->ucCtrl)
				== 1) {
				/* For ML-TWT multiple TWT flow ID handling  */
				mltwtReqFsmRunEventRxSetup(
					prAdapter,
					prStaRec,
					(uint8_t *)&(prRxSetupFrame->rTWT),
					prSwRfb->u2PacketLen -
					sizeof(
		struct _ACTION_ML_TWT_SETUP_FRAME_PER_LINK_DISTINCT));
			} else {
			/* For i-TWT/BTWT/RTWT single TWT flow ID handling */
#endif
			twtReqFsmRunEventRxSetup(prAdapter,
				prSwRfb, prStaRec, ucTWTFlowId,
				eTwtType);
#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
			}
#endif

		}
/* TWT STA code segment::End
 * i-TWT/BTWT/ML-TWT handling goes over here.
 */

		break;

	case ACTION_S1G_TWT_TEARDOWN:
		prRxTeardownFrame = (struct _ACTION_TWT_TEARDOWN_FRAME *)
			prSwRfb->pvHeader;
		if (prStaRec->ucStaState != STA_STATE_3 ||
			prSwRfb->u2PacketLen <
				sizeof(struct _ACTION_TWT_TEARDOWN_FRAME)) {
			DBGLOG(TWT_REQUESTER, WARN,
				"Received improper TWT Teardown frame\n");
			return;
		}

		ucTWTFlowId = prRxTeardownFrame->ucTWTFlow;

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
		prTWTHotspotStaNode = prStaRec->prTWTHotspotStaNode;

		if (prTWTHotspotStaNode != NULL) {
			/* if we're over here, this is Hotspot's
			 * STA's teardown request
			 */
			if (prTWTHotspotStaNode->flow_id == ucTWTFlowId) {
				/* Proceed the teardown operation for
				 * this Hotsopt's STA:
				 * 1. send teardown command to F/W
				 * 2. reset	Hotsopt's STA station record
				 */
				twtHotspotRespFsmSteps(
					prAdapter,
					prStaRec,
					TWT_HOTSPOT_RESP_STATE_RECEIVE_TEARDOWN,
					ucTWTFlowId,
					NULL);
			} else {
				/* incorrect flow id from sta */
				DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]Received TWT Teardown flow id = %d\n",
					ucTWTFlowId);

				return;
			}
		}		else		{
			/* We are TWT STA */
			/* Notify TWT Requester FSM */
			twtReqFsmRunEventRxTeardown(
				prAdapter, prSwRfb, prStaRec,
				ucTWTFlowId);
		}
#else
		/* Notify TWT Requester FSM */
		twtReqFsmRunEventRxTeardown(
			prAdapter, prSwRfb, prStaRec, ucTWTFlowId);
#endif

		break;

	case ACTION_S1G_TWT_INFORMATION:
	{
		uint8_t ucNextTWTSize = 0;
		uint8_t *pucMem;
		struct _NEXT_TWT_INFO_T rNextTWTInfo;

		prRxInfoFrame = (struct _ACTION_TWT_INFO_FRAME *)
			prSwRfb->pvHeader;
		if (prStaRec->ucStaState != STA_STATE_3 ||
			prSwRfb->u2PacketLen <
				sizeof(struct _ACTION_TWT_INFO_FRAME)) {
			DBGLOG(TWT_REQUESTER, WARN,
				"Received improper TWT Info frame\n");
			return;
		}

		ucTWTFlowId = GET_TWT_INFO_FLOW_ID(
			prRxInfoFrame->ucNextTWTCtrl);
		ucNextTWTSize = GET_TWT_INFO_NEXT_TWT_SIZE(
			prRxInfoFrame->ucNextTWTCtrl);

		u4Offset = OFFSET_OF(struct _ACTION_TWT_INFO_FRAME,
			aucNextTWT[0]);
		pucMem = ((uint8_t *)prRxInfoFrame) + u4Offset;

		if (ucNextTWTSize == NEXT_TWT_SUBFIELD_64_BITS &&
			prSwRfb->u2PacketLen >=
			(sizeof(struct _ACTION_TWT_INFO_FRAME) + 8)) {
			rNextTWTInfo.u8NextTWT =
				LE64_TO_CPU(*((uint64_t *)pucMem));
		} else if (ucNextTWTSize == NEXT_TWT_SUBFIELD_32_BITS &&
			prSwRfb->u2PacketLen >=
			(sizeof(struct _ACTION_TWT_INFO_FRAME) + 4)) {
			rNextTWTInfo.u8NextTWT =
				LE32_TO_CPU(*((uint32_t *)pucMem));
		} else if (ucNextTWTSize == NEXT_TWT_SUBFIELD_48_BITS &&
			prSwRfb->u2PacketLen >=
			(sizeof(struct _ACTION_TWT_INFO_FRAME) + 6)) {
			rNextTWTInfo.u8NextTWT =
				GET_48_BITS_NEXT_TWT_FROM_PKT(pucMem);
		} else {
			DBGLOG(TWT_REQUESTER, WARN,
				"TWT Info frame with imcorrect size\n");
			return;
		}

		rNextTWTInfo.ucNextTWTSize = ucNextTWTSize;

		/* Notify TWT Requester FSM */
		twtReqFsmRunEventRxInfoFrm(
			prAdapter, prSwRfb, prStaRec, ucTWTFlowId,
			&rNextTWTInfo);

		break;
	}
	default:
		break;
	}
}

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
void
twtHotspotGetFreeFlowId(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t *p_ucTWTFlowId)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i = 0;

	if (!prAdapter) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prBssInfo\n");

		return;
	}

	if (IS_BSS_AP(prAdapter, prBssInfo)) {
		for (i = 0; i < TWT_MAX_FLOW_NUM; i++) {
			if ((prBssInfo->twt_flow_id_bitmap & (1 << i)) == 0) {
				*p_ucTWTFlowId = i;

				prBssInfo->twt_flow_id_bitmap |= (1 << i);

				DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]BSS_INFO[%d] hotspot TWT flow id bitmap[%x]\n",
					prBssInfo->ucBssIndex,
					prBssInfo->twt_flow_id_bitmap);

				return;
			}
		}

		*p_ucTWTFlowId = TWT_HOTSPOT_NO_MORE_FLOW_ID;

		DBGLOG(TWT_RESPONDER, ERROR,
				"[TWT_RESP]Hotspot no more TWT connectivity\n");
	}
}

void
twtHotspotReturnFlowId(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i = 0;

	if (!prAdapter) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prBssInfo\n");

		return;
	}

	if (IS_BSS_AP(prAdapter, prBssInfo)) {
		i = prBssInfo->twt_flow_id_bitmap;
		prBssInfo->twt_flow_id_bitmap &= (~(1 << ucTWTFlowId));

		DBGLOG(TWT_RESPONDER, ERROR,
				"[TWT_RESP]BSS_INFO[%d] TWT flow id bitmap [%x]->[%x]\n",
				prBssInfo->ucBssIndex,
				i,
				prBssInfo->twt_flow_id_bitmap);
	}
}

void
twtHotspotGetStaRecIndexByFlowId(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	uint8_t ucTWTFlowId,
	uint8_t *p_ucIndex)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i = 0;

	if (!prAdapter) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prAdapter\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

	if (!prBssInfo) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prBssInfo\n");

		return;
	}

	if (IS_BSS_AP(prAdapter, prBssInfo)) {
		for (i = 0; i < TWT_MAX_FLOW_NUM; i++) {
			if ((prBssInfo->arTWTSta[i].used == 1) &&
				(prBssInfo->arTWTSta[i].flow_id ==
					ucTWTFlowId)) {
				*p_ucIndex = i;

				return;
			}
		}

		*p_ucIndex = 0xFF;
	}
}

void
twtHotspotGetStaRecByFlowId(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	uint8_t ucTWTFlowId,
	struct STA_RECORD **pprStaRec
)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i = 0;

	if (!prAdapter) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prAdapter\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

	if (!prBssInfo) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prBssInfo\n");

		return;
	}

	if (IS_BSS_AP(prAdapter, prBssInfo)) {
		for (i = 0; i < TWT_MAX_FLOW_NUM; i++) {
			if ((prBssInfo->arTWTSta[i].used == 1) &&
				(prBssInfo->arTWTSta[i].flow_id ==
					ucTWTFlowId)) {
				*pprStaRec = prBssInfo->arTWTSta[i].prStaRec;

				return;
			}
		}

		*pprStaRec = NULL;
	}
}

void
twtHotspotGetFreeStaNodeIndex(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t *p_ucIndex)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i = 0;

	if (!prAdapter) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prBssInfo\n");

		return;
	}

	if (IS_BSS_AP(prAdapter, prBssInfo)) {
		for (i = 0; i < TWT_MAX_FLOW_NUM; i++) {
			if (prBssInfo->arTWTSta[i].used == 0) {
				prBssInfo->arTWTSta[i].used = 1;

				*p_ucIndex = i;

				DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]BSS_INFO[%d] hotspot TWT sta node[%d]\n",
					prBssInfo->ucBssIndex,
					i);

				return;
			}
		}

		*p_ucIndex = TWT_HOTSPOT_NO_MORE_FLOW_ID;

		DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]Hotspot no more TWT sta nodes\n");
	}
}

void
twtHotspotGetFreeStaNode(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct _TWT_HOTSPOT_STA_NODE **pprTWTHotspotStaNode)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i = 0;

	if (!prAdapter) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prBssInfo\n");

		return;
	}

	if (IS_BSS_AP(prAdapter, prBssInfo)) {
		for (i = 0; i < TWT_MAX_FLOW_NUM; i++) {
			if (prBssInfo->arTWTSta[i].used == 0) {
				prBssInfo->arTWTSta[i].used = 1;

/* Caller:
*       struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode = NULL;
*   Callee:
*       pprTWTHotspotStaNode => &prTWTHotspotStaNode
*       *pprTWTHotspotStaNode => prTWTHotspotStaNode
*/

				prBssInfo->arTWTSta[i].own_mac_idx =
					prBssInfo->ucOwnMacIndex;
				prBssInfo->arTWTSta[i].peer_id_grp_id =
					CPU_TO_LE16(prStaRec->ucWlanIndex);
				prBssInfo->arTWTSta[i].bss_idx =
					prBssInfo->ucBssIndex;

				*pprTWTHotspotStaNode = &prBssInfo->arTWTSta[i];

				DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]BSS_INFO[%d] hotspot TWT sta node[%d]\n",
					prBssInfo->ucBssIndex,
					i);

				return;
			}
		}

		*pprTWTHotspotStaNode = NULL;

		DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]Hotspot no more TWT sta nodes\n");
	}
}

void
twtHotspotResetStaNode(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prBssInfo;
	struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode = NULL;
	struct LINK *p_twt_sch_link = NULL;

	if (!prAdapter) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prStaRec\n");

		return;
	}


	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prBssInfo\n");

		return;
	}

	p_twt_sch_link = &prBssInfo->twt_sch_link;

	prTWTHotspotStaNode = prStaRec->prTWTHotspotStaNode;

	if (!prTWTHotspotStaNode) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prTWTHotspotStaNode\n");

		return;
	}


	if (IS_BSS_AP(prAdapter, prBssInfo)) {
		/* Reset hotspot sta node */
		prTWTHotspotStaNode->used = 0;

		prTWTHotspotStaNode->own_mac_idx = 0;
		prTWTHotspotStaNode->flow_id = 0;
		prTWTHotspotStaNode->peer_id_grp_id = 0;
		prTWTHotspotStaNode->agrt_sp_duration = 0;
		prTWTHotspotStaNode->bss_idx = 0;

		prTWTHotspotStaNode->schedule_sp_start_tsf = 0;
		prTWTHotspotStaNode->twt_assigned_tsf = 0;
		prTWTHotspotStaNode->agrt_sp_start_tsf = 0;
		prTWTHotspotStaNode->agrt_sp_wake_intvl_mantissa = 0;
		prTWTHotspotStaNode->agrt_sp_wake_intvl_exponent = 0;
		prTWTHotspotStaNode->prStaRec = NULL;

		/* Remove this hotspot sta node from schedule list */
		LINK_REMOVE_KNOWN_ENTRY(
			p_twt_sch_link,
			&prTWTHotspotStaNode->list_entry);

		/* Finally, set station record's hotspot sta node to NULL */
		prStaRec->prTWTHotspotStaNode = NULL;
	}
}

uint32_t
twtHotspotAlignDuration(
	uint32_t sp_duration,
	uint32_t alignment)
{
	uint32_t sp_duration_alignment = 0;
	uint32_t m = sp_duration % alignment;

#if (TWT_HOTSPOT_TSF_ALIGNMENT_EN == 1)
	sp_duration_alignment = sp_duration + (alignment - m);
#else
	sp_duration_alignment = sp_duration;
#endif /* TWT_TSF_ALIGNMENT_EN */

	return sp_duration_alignment;
}

void
twtHotspotGetNearestTargetTSF(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode,
	uint64_t u8CurrentTsf)
{
	struct BSS_INFO *prBssInfo;
	uint16_t sp_duration = 0;
	uint64_t u8twt_interval = 0;
	uint64_t u8Temp = 0;
	uint64_t u8Mod = 0;
	u_int8_t bFound = FALSE;
	struct LINK *p_twt_sch_link = NULL;
	struct _TWT_HOTSPOT_STA_NODE *curr_twt_node = NULL;
	struct _TWT_HOTSPOT_STA_NODE *next_twt_node = NULL;
	struct _TWT_HOTSPOT_STA_NODE *temp_twt_node = NULL;
	struct _TWT_HOTSPOT_STA_NODE *head_twt_node = NULL;
	struct _TWT_HOTSPOT_STA_NODE *tail_twt_node = NULL;

	if (!prAdapter) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prAdapter\n");

		return;
	}

	if (!prStaRec) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prStaRec\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(
				prAdapter,
				prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prBssInfo\n");

		return;
	}

	if (prTWTHotspotStaNode == NULL) {
		DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]NULL node\n");
		return;
	}
	if (IS_BSS_AP(prAdapter, prBssInfo)) {
		p_twt_sch_link = &prBssInfo->twt_sch_link;

		/* Build the whole schedule from
		* BssInfo's TWT schedule link
		*/
		sp_duration = prTWTHotspotStaNode->
						agrt_sp_duration << 8;

		if (LINK_IS_EMPTY(p_twt_sch_link)) {
			/* insert as the 1st node */
			prTWTHotspotStaNode->schedule_sp_start_tsf = 0;

			LINK_INSERT_TAIL(
				p_twt_sch_link,
				&prTWTHotspotStaNode->list_entry);
		} else if (p_twt_sch_link->u4NumElem == 1) {
			curr_twt_node = LINK_PEEK_HEAD(
				p_twt_sch_link,
				struct _TWT_HOTSPOT_STA_NODE,
				list_entry);
			if (curr_twt_node == NULL) {
				DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]can't find curr_twt_node\n");
				return;
			}
			if (curr_twt_node->schedule_sp_start_tsf
				>= sp_duration) {
				/* insert before 1st node */
				/* curr_twt_node is 1st node */
				prTWTHotspotStaNode->schedule_sp_start_tsf = 0;

				LINK_INSERT_BEFORE(
					p_twt_sch_link,
					&curr_twt_node->list_entry,
					&prTWTHotspotStaNode->list_entry);
			} else {
				/* insert after 1st node */
				/* curr_twt_node is 1st node */
				prTWTHotspotStaNode->schedule_sp_start_tsf =
					curr_twt_node->schedule_sp_start_tsf +
					twtHotspotAlignDuration(
					(curr_twt_node->agrt_sp_duration) << 8,
					TWT_HOTSPOT_TSF_ALIGNMNET_UINT);

				LINK_INSERT_AFTER(
					p_twt_sch_link,
					&curr_twt_node->list_entry,
					&prTWTHotspotStaNode->list_entry);
			}
		} else {
			/* insert at proper place */
			head_twt_node = LINK_PEEK_HEAD(
				p_twt_sch_link,
				struct _TWT_HOTSPOT_STA_NODE,
				list_entry);

			LINK_FOR_EACH_ENTRY_SAFE(
				temp_twt_node,
				next_twt_node,
				p_twt_sch_link,
				list_entry,
				struct _TWT_HOTSPOT_STA_NODE) {
				curr_twt_node = temp_twt_node;

				/* space check before 1st node */
				if ((curr_twt_node == head_twt_node) &&
					(curr_twt_node->
						schedule_sp_start_tsf >=
						sp_duration)) {
					/* insert before head */
					prTWTHotspotStaNode->
						schedule_sp_start_tsf = 0;

					LINK_INSERT_BEFORE(
						p_twt_sch_link,
						&curr_twt_node->list_entry,
						&prTWTHotspotStaNode->
							list_entry);

					bFound = TRUE;

					break;
				}

				/* space check after 1st node
				* if current node is not the last node
				*/
				tail_twt_node = LINK_PEEK_TAIL(
					p_twt_sch_link,
					struct _TWT_HOTSPOT_STA_NODE,
					list_entry);

				if ((curr_twt_node != tail_twt_node) &&
					(next_twt_node->
						schedule_sp_start_tsf -
						(curr_twt_node->
						schedule_sp_start_tsf +
						twtHotspotAlignDuration(
						curr_twt_node->
							agrt_sp_duration << 8,
						TWT_HOTSPOT_TSF_ALIGNMNET_UINT))
							>= sp_duration)) {
					prTWTHotspotStaNode->
						schedule_sp_start_tsf =
					curr_twt_node->
						schedule_sp_start_tsf +
					twtHotspotAlignDuration(
						curr_twt_node->agrt_sp_duration
						<< 8,
						TWT_HOTSPOT_TSF_ALIGNMNET_UINT);

					LINK_INSERT_AFTER(
						p_twt_sch_link,
						&curr_twt_node->list_entry,
						&prTWTHotspotStaNode->
							list_entry);

					bFound = TRUE;

					break;
				}
			}

			/* insert as the tail node */
			if (!bFound) {
				prTWTHotspotStaNode->schedule_sp_start_tsf =
					curr_twt_node->schedule_sp_start_tsf +
					twtHotspotAlignDuration(
					curr_twt_node->agrt_sp_duration << 8,
					TWT_HOTSPOT_TSF_ALIGNMNET_UINT);

				LINK_INSERT_TAIL(
					p_twt_sch_link,
					&prTWTHotspotStaNode->list_entry);
			}
		}

		/* Determine the real timestamp for this TWT activity */
		u8twt_interval = ((u_int64_t)(prTWTHotspotStaNode->
			agrt_sp_wake_intvl_mantissa)) <<
			prTWTHotspotStaNode->agrt_sp_wake_intvl_exponent;

		u8Temp = u8CurrentTsf -
			prTWTHotspotStaNode->schedule_sp_start_tsf;

		u8Mod = kal_mod64(u8Temp, u8twt_interval);

		prTWTHotspotStaNode->twt_assigned_tsf =
						u8CurrentTsf +
						(u8twt_interval - u8Mod) +
						u8twt_interval * 10;

		DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]twt_assigned_tsf=%llx\n",
					prTWTHotspotStaNode->twt_assigned_tsf);
		DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]u8Temp=%llx\n",
					u8Temp);
		DBGLOG(TWT_RESPONDER, ERROR,
					"[TWT_RESP]u8Mod=%llx\n",
					u8Mod);
	}
}

uint32_t
twtHotspotSendSetupRespFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	uint8_t ucDialogToken,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _ACTION_TWT_SETUP_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	struct _IE_TWT_T *prTWTBuf;

	if (!prAdapter) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prTWTParams\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD
		+ sizeof(struct _ACTION_TWT_SETUP_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)
			cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(TWT_RESPONDER, ERROR,
			"[TWT_RESP]No MSDU_INFO_T for sending TWT Setup Resp Frame.\n");

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
	prTxFrame->ucCategory = CATEGORY_S1G_ACTION;
	prTxFrame->ucAction = ACTION_S1G_TWT_SETUP;

	/* Must be the same as in the STA's setup req frame*/
	prTxFrame->ucDialogToken = ucDialogToken;

	prTWTBuf = &(prTxFrame->rTWT);
	twtFillTWTElement(prTWTBuf, ucTWTFlowId, prTWTParams);

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			WLAN_MAC_MGMT_HEADER_LEN,
			sizeof(struct _ACTION_TWT_SETUP_FRAME),
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	/* Enqueue the frame to send this action frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}
#endif


#if (CFG_SUPPORT_BTWT == 1)
void btwtFillTWTElement(
	struct _IE_BTWT_T *prTWTBuf,
	uint8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams)
{
	if (!prTWTBuf) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTBuf\n");

		return;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTParams\n");

		return;
	}

	/* Add TWT element */
	prTWTBuf->ucId = ELEM_ID_TWT;
	prTWTBuf->ucLength = sizeof(struct _IE_BTWT_T) - ELEM_HDR_LEN;

	/* Request Type */
	prTWTBuf->u2ReqType |= SET_TWT_RT_REQUEST(prTWTParams->fgReq) |
		SET_TWT_RT_SETUP_CMD(prTWTParams->ucSetupCmd) |
		SET_TWT_RT_TRIGGER(prTWTParams->fgTrigger) |
		BTWT_REQ_TYPE_LAST_BCAST_PARAM |
		SET_TWT_RT_FLOW_TYPE(prTWTParams->fgUnannounced) |
		SET_BTWT_RECOMMENDATION(0) |
		SET_TWT_RT_WAKE_INTVAL_EXP(prTWTParams->ucWakeIntvalExponent) |
		SET_BTWT_RESERVED(0);

	prTWTBuf->ucCtrl |= SET_BTWT_CTRL_NEGO(3);
	prTWTBuf->u2BTWTInfo |= SET_BTWT_ID(ucTWTFlowId);
	prTWTBuf->u2TWT =
		CPU_TO_LE16((uint16_t)((prTWTParams->u8TWT >> 10) & 0xFFFF));
	prTWTBuf->ucMinWakeDur = prTWTParams->ucMinWakeDur;
	prTWTBuf->u2WakeIntvalMantiss =
		CPU_TO_LE16(prTWTParams->u2WakeIntvalMantiss);
	DBGLOG(TWT_REQUESTER, WARN,
		"btwtSendSetupFrame frame %x, %x, %x, %x, %x, %x, %x, %x\n",
		prTWTBuf->ucId,
		prTWTBuf->ucLength,
		prTWTBuf->ucCtrl,
		prTWTBuf->u2ReqType,
		prTWTBuf->u2TWT,
		prTWTBuf->ucMinWakeDur,
		prTWTBuf->u2WakeIntvalMantiss,
		prTWTBuf->u2BTWTInfo);

}

uint32_t btwtSendSetupFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _ACTION_BTWT_SETUP_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	struct _IE_BTWT_T *prBTWTBuf;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTParams\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(TWT_REQUESTER, WARN,
		"btwtSendSetupFrame begin\n");

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD
		+ sizeof(struct _ACTION_BTWT_SETUP_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)
			cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, WARN,
			"No MSDU_INFO_T for sending TWT Setup Frame.\n");
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
	prTxFrame->ucCategory = CATEGORY_S1G_ACTION;
	prTxFrame->ucAction = ACTION_S1G_TWT_SETUP;

	prBTWTBuf = &(prTxFrame->rTWT);
	btwtFillTWTElement(prBTWTBuf, ucTWTFlowId, prTWTParams);
	DBGLOG(TWT_REQUESTER, WARN,
		"btwtSendSetupFrame flowid %d\n", ucTWTFlowId);

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			WLAN_MAC_MGMT_HEADER_LEN,
			sizeof(struct _ACTION_BTWT_SETUP_FRAME),
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
			MSDU_CONTROL_FLAG_FORCE_LINK |
			MSDU_CONTROL_FLAG_DIS_MAT,
			TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* Enqueue the frame to send this action frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
	DBGLOG(TWT_REQUESTER, WARN,
		"btwtSendSetupFrame end\n");

	return WLAN_STATUS_SUCCESS;
}

uint32_t btwtSendTeardownFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _ACTION_TWT_TEARDOWN_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
		sizeof(struct _ACTION_TWT_TEARDOWN_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *) cnmMgtPktAlloc(
		prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, WARN,
			"No MSDU_INFO_T for sending TWT Teardown Frame.\n");
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
	prTxFrame->ucCategory = CATEGORY_S1G_ACTION;
	prTxFrame->ucAction = ACTION_S1G_TWT_TEARDOWN;
	prTxFrame->ucTWTFlow = ucTWTFlowId;
	prTxFrame->ucTWTFlow |= SET_TWT_TEARDOWN_NEGO(3);

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			WLAN_MAC_MGMT_HEADER_LEN,
			sizeof(struct _ACTION_TWT_TEARDOWN_FRAME),
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
			MSDU_CONTROL_FLAG_FORCE_LINK |
			MSDU_CONTROL_FLAG_DIS_MAT,
			TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* Enqueue the frame to send this action frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;

}
#endif

#if (CFG_SUPPORT_RTWT == 1)
void rtwtFillTWTElement(
	struct _IE_RTWT_T *prTWTBuf,
	uint8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	uint8_t ucSetupFrameByteLength)
{
	if (!prTWTBuf) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTBuf\n");

		return;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTParams\n");

		return;
	}

	if (ucSetupFrameByteLength <= ELEM_HDR_LEN) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid setup frame length %d\n",
			ucSetupFrameByteLength);

		return;
	}

	/* Add TWT element */
	prTWTBuf->ucId = ELEM_ID_TWT;
	prTWTBuf->ucLength = ucSetupFrameByteLength - ELEM_HDR_LEN;

	/* Request Type */
	prTWTBuf->u2ReqType |= SET_TWT_RT_REQUEST(prTWTParams->fgReq) |
		SET_TWT_RT_SETUP_CMD(prTWTParams->ucSetupCmd) |
		SET_TWT_RT_TRIGGER(prTWTParams->fgTrigger) |
		BTWT_REQ_TYPE_LAST_BCAST_PARAM |
		SET_TWT_RT_FLOW_TYPE(prTWTParams->fgUnannounced) |
		SET_BTWT_RECOMMENDATION(4) |
		SET_TWT_RT_WAKE_INTVAL_EXP(prTWTParams->ucWakeIntvalExponent) |
		SET_BTWT_RESERVED(0);

	/* Control field */
	prTWTBuf->ucCtrl |= SET_BTWT_CTRL_NEGO(3);

	/* BTWT Info field */
	prTWTBuf->u2BTWTInfo |= SET_BTWT_ID(ucTWTFlowId) |
				SET_BTWT_PERSISTENCE(255);

	if (prTWTParams->ucTrafficInfoPresent != 0) {
		prTWTBuf->u2BTWTInfo |=	SET_BTWT_RTWT_TRAFFIC_INFO_PRESENT(1);

		/* RTWT traffic info field, 0~3 bytes */
		prTWTBuf->uc_arRTWTTrafficInfo[0] = prTWTParams->ucDlUlBmpValid;

		DBGLOG(TWT_REQUESTER, WARN,
			"rtwt Setup Frame traffic info[0] %x\n",
			prTWTBuf->uc_arRTWTTrafficInfo[0]);

		if (prTWTParams->ucDlUlBmpValid != 0) {
			prTWTBuf->uc_arRTWTTrafficInfo[1] =
						prTWTParams->ucDlBmp;
			prTWTBuf->uc_arRTWTTrafficInfo[2] =
						prTWTParams->ucUlBmp;

			DBGLOG(TWT_REQUESTER, WARN,
				"rtwt Setup Frame traffic info[1|2] %x %x\n",
				prTWTBuf->uc_arRTWTTrafficInfo[1],
				prTWTBuf->uc_arRTWTTrafficInfo[2]);
		}
	}

	prTWTBuf->u2TWT = prTWTParams->u2TWT;
	prTWTBuf->ucMinWakeDur = prTWTParams->ucMinWakeDur;
	prTWTBuf->u2WakeIntvalMantiss =
		CPU_TO_LE16(prTWTParams->u2WakeIntvalMantiss);
	DBGLOG(TWT_REQUESTER, WARN,
		"rtwt Setup Frame %x, %x, %x, %x, %x, %x, %x, %x\n",
		prTWTBuf->ucId,
		prTWTBuf->ucLength,
		prTWTBuf->ucCtrl,
		prTWTBuf->u2ReqType,
		prTWTBuf->u2TWT,
		prTWTBuf->ucMinWakeDur,
		prTWTBuf->u2WakeIntvalMantiss,
		prTWTBuf->u2BTWTInfo);
}

uint32_t rtwtSendSetupFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _ACTION_RTWT_SETUP_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	struct _IE_RTWT_T *prRTWTBuf;
	uint8_t ucSetupFrameByteLength;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTParams\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (prTWTParams->ucTrafficInfoPresent == 0)
		ucSetupFrameByteLength =
			sizeof(struct _ACTION_RTWT_SETUP_FRAME);
	else {
		if (prTWTParams->ucDlUlBmpValid == 0)
			ucSetupFrameByteLength =
				sizeof(struct _ACTION_RTWT_SETUP_FRAME) +
				RTWT_TRAFFIC_INFO_DL_UL_BMP_VALID_BYTE_LENGTH;
		else
			ucSetupFrameByteLength =
				sizeof(struct _ACTION_RTWT_SETUP_FRAME) +
				RTWT_TRAFFIC_INFO_DL_UL_BMP_VALID_BYTE_LENGTH +
				RTWT_TRAFFIC_INFO_DL_TID_BMP_BYTE_LENGTH +
				RTWT_TRAFFIC_INFO_UL_TID_BMP_BYTE_LENGTH;
	}

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD + ucSetupFrameByteLength;

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)
			cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, WARN,
			"No MSDU_INFO_T for sending TWT Setup Frame.\n");
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
	prTxFrame->ucCategory = CATEGORY_S1G_ACTION;
	prTxFrame->ucAction = ACTION_S1G_TWT_SETUP;

	prRTWTBuf = &(prTxFrame->rTWT);
	rtwtFillTWTElement(prRTWTBuf,
			ucTWTFlowId, prTWTParams,
			ucSetupFrameByteLength);

	DBGLOG(TWT_REQUESTER, WARN,
		"RTWT flowid %d\n", ucTWTFlowId);

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			WLAN_MAC_MGMT_HEADER_LEN,
			ucSetupFrameByteLength,
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
			MSDU_CONTROL_FLAG_FORCE_LINK |
			MSDU_CONTROL_FLAG_DIS_MAT,
			TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* Enqueue the frame to send this action frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

uint32_t rtwtSendTeardownFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	u_int8_t fgTeardownAll,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _ACTION_TWT_TEARDOWN_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
		sizeof(struct _ACTION_TWT_TEARDOWN_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *) cnmMgtPktAlloc(
		prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, WARN,
			"No MSDU_INFO_T for sending TWT Teardown Frame.\n");
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
	prTxFrame->ucCategory = CATEGORY_S1G_ACTION;
	prTxFrame->ucAction = ACTION_S1G_TWT_TEARDOWN;
	prTxFrame->ucTWTFlow = ucTWTFlowId;
	prTxFrame->ucTWTFlow |= SET_TWT_TEARDOWN_NEGO(3);

	if (fgTeardownAll != 0)
		prTxFrame->ucTWTFlow |= TWT_TEARDOWN_ALL;

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			WLAN_MAC_MGMT_HEADER_LEN,
			sizeof(struct _ACTION_TWT_TEARDOWN_FRAME),
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
			MSDU_CONTROL_FLAG_FORCE_LINK |
			MSDU_CONTROL_FLAG_DIS_MAT,
			TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* Enqueue the frame to send this action frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

uint8_t rtwtGetTxSetupFlowId(
	struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucFlowId;
	struct _ACTION_RTWT_SETUP_FRAME *prTxFrame;

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prMsduInfo\n");

		return TWT_INCORRECT_FLOW_ID;
	}

	prTxFrame = (struct _ACTION_RTWT_SETUP_FRAME *)(prMsduInfo->prPacket);
	ucFlowId = GET_BTWT_ID(prTxFrame->rTWT.u2BTWTInfo);

	return ucFlowId;
}

void rtwtParseTWTElement(
	struct _IE_RTWT_T *prRTWTIE,
	struct _TWT_PARAMS_T *prTWTParams)
{
	uint16_t u2ReqType;
	uint64_t u8TargetWakeTime;
	uint64_t u8twt_interval = 0;
	uint64_t u8Mod = 0;
	uint64_t u8Temp;

	if (!prRTWTIE) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prBTWTIE\n");

		return;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"invalid prTWTParams\n");

		return;
	}

	if (prRTWTIE->u2WakeIntvalMantiss == 0)	{
		/* For coverity check */
		return;
	}

	u2ReqType = LE16_TO_CPU(prRTWTIE->u2ReqType);
	prTWTParams->fgReq = GET_TWT_RT_REQUEST(u2ReqType);
	prTWTParams->ucSetupCmd = GET_TWT_RT_SETUP_CMD(u2ReqType);
	prTWTParams->fgTrigger = GET_TWT_RT_TRIGGER(u2ReqType);
	prTWTParams->fgUnannounced = GET_TWT_RT_FLOW_TYPE(u2ReqType);
	prTWTParams->ucWakeIntvalExponent =
		GET_TWT_RT_WAKE_INTVAL_EXP(u2ReqType);

	prTWTParams->ucMinWakeDur = prRTWTIE->ucMinWakeDur;
	prTWTParams->u2WakeIntvalMantiss =
		LE16_TO_CPU(prRTWTIE->u2WakeIntvalMantiss);

	DBGLOG(TWT_REQUESTER, WARN,
		"RTWT cur TSF %x %x\nAP resp TWT %x\n",
		(uint32_t)(prTWTParams->u8TWT & 0x00000000FFFFFFFF),
		(uint32_t)((prTWTParams->u8TWT & 0xFFFFFFFF00000000) >> 32),
		prRTWTIE->u2TWT);

	u8TargetWakeTime = prTWTParams->u8TWT;

	/* see Draft P802.11ax_D7.0.pdf, p421 */
	u8TargetWakeTime &= 0xFFFFFFFFFFFFFC00;
	u8TargetWakeTime |= (uint32_t)(prRTWTIE->u2TWT << 10);

	DBGLOG(TWT_REQUESTER, WARN,
		"RTWT cur TSF sync with AP %x %x\n",
		(uint32_t)(u8TargetWakeTime & 0x00000000FFFFFFFF),
		(uint32_t)((u8TargetWakeTime & 0xFFFFFFFF00000000) >> 32));

	u8twt_interval = (((u_int64_t)prTWTParams->u2WakeIntvalMantiss)
		<< prTWTParams->ucWakeIntvalExponent);
	u8Temp = u8TargetWakeTime + u8twt_interval;
	u8Mod = kal_mod64(u8Temp, u8twt_interval);
	prTWTParams->u8TWT = u8TargetWakeTime + (u8twt_interval - u8Mod);

	DBGLOG(TWT_REQUESTER, WARN,
		"RTWT twt %x %x\n",
		(uint32_t)(prTWTParams->u8TWT & 0x00000000FFFFFFFF),
		(uint32_t)((prTWTParams->u8TWT & 0xFFFFFFFF00000000) >> 32));

	/* Evaluate if the RTWT setup resp frame contains traffic info */
	if (GET_BTWT_RTWT_TRAFFIC_INFO_PRESENT(prRTWTIE->u2BTWTInfo) != 0) {
		prTWTParams->ucTrafficInfoPresent = 1;
		prTWTParams->ucDlUlBmpValid = prRTWTIE->uc_arRTWTTrafficInfo[0];
		prTWTParams->ucDlBmp = prRTWTIE->uc_arRTWTTrafficInfo[1];
		prTWTParams->ucUlBmp = prRTWTIE->uc_arRTWTTrafficInfo[2];
	} else {
		prTWTParams->ucTrafficInfoPresent = 0;
		prTWTParams->ucDlUlBmpValid = 0;
		prTWTParams->ucDlBmp = 0;
		prTWTParams->ucUlBmp = 0;
	}

	DBGLOG(TWT_REQUESTER, WARN,
		"RTWT traffic info(%x) %x %x %x\n",
		prTWTParams->ucTrafficInfoPresent,
		prTWTParams->ucDlUlBmpValid,
		prTWTParams->ucDlBmp,
		prTWTParams->ucUlBmp);
}
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
uint32_t mltwtParseTWTElement(
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
			"MLTWT invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(
					prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"ML Invalid MLD_BSS_INFO\n");

		return WLAN_STATUS_FAILURE;
	}

	IE_FOR_EACH(pucIE, u2IELength, u2Offset)
	{
		DBGLOG(TWT_REQUESTER, WARN,
			"u2IELength %d u2Offset %d\n",
			u2IELength, u2Offset);

	switch (IE_ID(pucIE))
	{
	case ELEM_ID_TWT:
		prMLTWTIE = (struct IE_ML_TWT_T *)pucIE;

		ucTWTFlowId = twtGetRxSetupFlowId(
					(struct _IE_TWT_T *)prMLTWTIE);

		if ((ucTWTFlowId == TWT_INCORRECT_FLOW_ID) ||
			(ucTWTFlowId >= TWT_MAX_FLOW_NUM)) {
			DBGLOG(TWT_REQUESTER, ERROR,
				"TWT_INCORRECT_FLOW_ID %d\n",
				ucTWTFlowId);

			return WLAN_STATUS_INVALID_DATA;
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

					return WLAN_STATUS_FAILURE;
				}

				prStaRecOfAP = prBssInfo->prStaRecOfAP;

				if (!prStaRecOfAP) {
					DBGLOG(TWT_REQUESTER, ERROR,
						"TWT Flow %d no STA_REC of link ID %d\n",
						ucTWTFlowId,
						ucLinkID);

					return WLAN_STATUS_FAILURE;
				}

				twtParseTWTElement(
					(struct _IE_TWT_T *)pucIE,
					&(prStaRecOfAP->arTWTFlow[ucTWTFlowId]
					.rTWTPeerParams));

				prStaRecOfAP->arTWTFlow[ucTWTFlowId].fgIsMLTWT = TRUE;

				prStaRecOfAP->arTWTFlow[ucTWTFlowId].eTwtType =
					ENUM_TWT_TYPE_MLTWT;
			}
		}

		break;

	default:
		break;
	}
	}

#if 0
    if (kal_mod64(u2IELength, IE_ML_TWT_LENGTH) == 0) {
		/*
		* The frame contains multiple complete TWT elements,
		* ex.
		* {TWT Element[0]|TWT Element[1]|TWT Element[2]}
		* , where TWT Element = {tag|length|CTRL|TWT param}
		*/
    } else {
		/*
		* The frame contains TWT element combinatory with
		* multiple TWT parameters,
		* ex.
		* {tag|length|CTRL|TWT param[0]|TWT param[1]|TWT param[2]}
		*/
    }
#endif

	return WLAN_STATUS_SUCCESS;
}

/*
* All MLTWT links sharing the same TWT parameter
*/
uint32_t mltwtFillTWTElementAllInOne(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct IE_ML_TWT_T *prMLTWTBuf,
	uint8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams)
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct LINK *prBssList = NULL;
	struct BSS_INFO *prCurrBssInfo = NULL;
	struct STA_RECORD *prStaRecOfAP = NULL;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prMLTWTBuf) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prMLTWTBuf\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prTWTParams\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Add TWT element */
	prMLTWTBuf->ucId = ELEM_ID_TWT;
	prMLTWTBuf->ucLength = sizeof(struct IE_ML_TWT_T) - ELEM_HDR_LEN;

	/* Request Type */
	prMLTWTBuf->u2ReqType |= SET_TWT_RT_REQUEST(prTWTParams->fgReq) |
		SET_TWT_RT_SETUP_CMD(prTWTParams->ucSetupCmd) |
		SET_TWT_RT_TRIGGER(prTWTParams->fgTrigger) |
		TWT_REQ_TYPE_IMPLICIT_LAST_BCAST_PARAM |
		SET_TWT_RT_FLOW_TYPE(prTWTParams->fgUnannounced) |
		SET_TWT_RT_FLOW_ID(ucTWTFlowId) |
		SET_TWT_RT_WAKE_INTVAL_EXP(prTWTParams->ucWakeIntvalExponent) |
		SET_TWT_RT_PROTECTION(prTWTParams->fgProtect);

	prMLTWTBuf->u8TWT = CPU_TO_LE64(prTWTParams->u8TWT);
	prMLTWTBuf->ucMinWakeDur = prTWTParams->ucMinWakeDur;
	prMLTWTBuf->u2WakeIntvalMantiss =
		CPU_TO_LE16(prTWTParams->u2WakeIntvalMantiss);

	/* CTRL link ID bitmap manipulation */
	prMLTWTBuf->ucCtrl |= SET_ML_TWT_CTRL_LINK_ID_BITMAP(1);

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR, "ML Invalid MLD_BSS_INFO\n");

		return WLAN_STATUS_FAILURE;
	}

	prBssList = &prMldBssInfo->rBssList;

	LINK_FOR_EACH_ENTRY(prCurrBssInfo, prBssList,
			rLinkEntryMld,
			struct BSS_INFO) {
		if (!prCurrBssInfo)
			break;

		prStaRecOfAP = prCurrBssInfo->prStaRecOfAP;

		if (!prStaRecOfAP)
			break;

		prMLTWTBuf->u2LinkIdBitmap |= BIT(prStaRecOfAP->ucLinkId);
	}

	DBGLOG(TWT_REQUESTER, WARN,
		"Link ID bitmap %x\n", prMLTWTBuf->u2LinkIdBitmap);

	return WLAN_STATUS_SUCCESS;
}

/*
* All MLTWT links sharing the same TWT parameter
*/
uint32_t mltwtSendSetupFrameAllInOne(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _ACTION_ML_TWT_SETUP_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	struct IE_ML_TWT_T *prMLTWTBuf;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prTWTParams\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD
		+ sizeof(struct _ACTION_ML_TWT_SETUP_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)
			cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, WARN,
			"No MSDU_INFO_T for sending MLTWT Setup Frame.\n");
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
	prTxFrame->ucCategory = CATEGORY_S1G_ACTION;
	prTxFrame->ucAction = ACTION_S1G_TWT_SETUP;

	prMLTWTBuf = &(prTxFrame->rTWT);

	if (mltwtFillTWTElementAllInOne(
		prAdapter, prBssInfo,
		prMLTWTBuf, ucTWTFlowId,
		prTWTParams) == WLAN_STATUS_FAILURE) {
		DBGLOG(TWT_REQUESTER, ERROR, "MLTWT setup frame error\n");

		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(TWT_REQUESTER, WARN,
		"MLTWT flowid %d\n", ucTWTFlowId);

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			WLAN_MAC_MGMT_HEADER_LEN,
			sizeof(struct _ACTION_ML_TWT_SETUP_FRAME),
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	/* Enqueue the frame to send this action frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

uint8_t mltwtGetLinkCount(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t ucTWTFlowId)
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct LINK *prBssList = NULL;
	struct BSS_INFO *prCurrBssInfo = NULL;
	struct STA_RECORD *prStaRecOfAP = NULL;
	uint8_t ucMloLinkCount = 0;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prAdapter\n");

		return 0;
	}

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prBssInfo\n");

		return 0;
	}

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR, "ML Invalid MLD_BSS_INFO\n");

		return 0;
	}

	prBssList = &prMldBssInfo->rBssList;

	LINK_FOR_EACH_ENTRY(prCurrBssInfo, prBssList,
			rLinkEntryMld,
			struct BSS_INFO) {
		if (!prCurrBssInfo)
			break;

		prStaRecOfAP = prCurrBssInfo->prStaRecOfAP;

		if (!prStaRecOfAP)
			break;

		if (prStaRecOfAP->arTWTFlow[ucTWTFlowId].fgIsMLTWT == TRUE)
			ucMloLinkCount++;
	}

	return ucMloLinkCount;
}

/*
* MLTWT per link distinct TWT parameter
*/
uint32_t mltwtFillTWTElementPerLinkDistinct(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t *pucIE,
	uint16_t u2IELength,
	uint8_t ucTWTFlowId)
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct LINK *prBssList = NULL;
	struct BSS_INFO *prCurrBssInfo = NULL;
	struct STA_RECORD *prStaRecOfAP = NULL;
	uint16_t u2Offset = 0;
	struct _TWT_FLOW_T *prTWTFlow = NULL;
	struct _TWT_PARAMS_T *prTWTParams = NULL;
	struct IE_ML_TWT_T *prMLTWTBuf = NULL;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!pucIE) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid pucIE\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR, "ML Invalid MLD_BSS_INFO\n");

		return WLAN_STATUS_FAILURE;
	}

	prBssList = &prMldBssInfo->rBssList;

	LINK_FOR_EACH_ENTRY(prCurrBssInfo, prBssList,
			rLinkEntryMld,
			struct BSS_INFO) {
		if (!prCurrBssInfo)
			break;

		prStaRecOfAP = prCurrBssInfo->prStaRecOfAP;

		if (!prStaRecOfAP)
			break;

		prTWTFlow = &(prStaRecOfAP->arTWTFlow[ucTWTFlowId]);

		if ((prTWTFlow->fgIsMLTWT == TRUE)
			&& (u2Offset < u2IELength))
		{
			prMLTWTBuf = (struct IE_ML_TWT_T *)(pucIE + u2Offset);

			prTWTParams = &(prTWTFlow->rTWTParams);

			/* Add TWT element */
			prMLTWTBuf->ucId = ELEM_ID_TWT;
			prMLTWTBuf->ucLength = sizeof(struct IE_ML_TWT_T) - ELEM_HDR_LEN;

			/* CTRL link ID bitmap manipulation */
			prMLTWTBuf->ucCtrl |= SET_ML_TWT_CTRL_LINK_ID_BITMAP(1);

			/* Request Type */
			prMLTWTBuf->u2ReqType |= SET_TWT_RT_REQUEST(prTWTParams->fgReq) |
				SET_TWT_RT_SETUP_CMD(prTWTParams->ucSetupCmd) |
				SET_TWT_RT_TRIGGER(prTWTParams->fgTrigger) |
				TWT_REQ_TYPE_IMPLICIT_LAST_BCAST_PARAM |
				SET_TWT_RT_FLOW_TYPE(prTWTParams->fgUnannounced) |
				SET_TWT_RT_FLOW_ID(ucTWTFlowId) |
				SET_TWT_RT_WAKE_INTVAL_EXP(prTWTParams->ucWakeIntvalExponent) |
				SET_TWT_RT_PROTECTION(prTWTParams->fgProtect);

			prMLTWTBuf->u8TWT = CPU_TO_LE64(prTWTParams->u8TWT);
			prMLTWTBuf->ucMinWakeDur = prTWTParams->ucMinWakeDur;
			prMLTWTBuf->u2WakeIntvalMantiss =
				CPU_TO_LE16(prTWTParams->u2WakeIntvalMantiss);

			/* Link ID bitmap manipulation */
			prMLTWTBuf->u2LinkIdBitmap =
				BIT(prStaRecOfAP->ucLinkId);

			/* Accumutate offset in the unit of (struct IE_ML_TWT_T) */
			u2Offset += sizeof(struct IE_ML_TWT_T);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*
* MLTWT per link distinct TWT parameter
*/
uint32_t mltwtSendSetupFramePerLinkDistinct(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _ACTION_ML_TWT_SETUP_FRAME_PER_LINK_DISTINCT *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	uint8_t ucMloLinkCount = 0;

	if (!prAdapter) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prAdapter\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prStaRec) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prStaRec\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prTWTParams) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prTWTParams\n");

		return WLAN_STATUS_INVALID_DATA;
	}


	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT invalid prBssInfo\n");

		return WLAN_STATUS_INVALID_DATA;
	}

	ucMloLinkCount = mltwtGetLinkCount(
						prAdapter,
						prBssInfo,
						ucTWTFlowId);

	if (ucMloLinkCount == 0) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"MLTWT %d no link.\n",
			ucTWTFlowId);

		return WLAN_STATUS_FAILURE;
	}

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD
		+ OFFSET_OF(
			struct _ACTION_ML_TWT_SETUP_FRAME_PER_LINK_DISTINCT,
			rTWT[0])
		+ (ucMloLinkCount * sizeof(struct IE_ML_TWT_T));

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)
			cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(TWT_REQUESTER, ERROR,
			"No MSDU_INFO_T for sending MLTWT Setup Frame.\n");

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
	prTxFrame->ucCategory = CATEGORY_S1G_ACTION;
	prTxFrame->ucAction = ACTION_S1G_TWT_SETUP;

	/* Use the 1st struct _IE_ML_TWT_T fo rmanipulation */
	if (mltwtFillTWTElementPerLinkDistinct(
			prAdapter, prBssInfo, prTxFrame->rTWT,
			(ucMloLinkCount * sizeof(struct IE_ML_TWT_T)),
			ucTWTFlowId) == WLAN_STATUS_FAILURE) {
		DBGLOG(TWT_REQUESTER, ERROR, "MLTWT setup frame error\n");

		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(TWT_REQUESTER, WARN,
		"MLTWT flowid %d\n", ucTWTFlowId);

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			WLAN_MAC_MGMT_HEADER_LEN,
			u2EstimatedFrameLen,
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	/* Enqueue the frame to send this action frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}
#endif
