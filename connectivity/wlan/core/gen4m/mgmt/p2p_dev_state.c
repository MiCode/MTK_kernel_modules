// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "precomp.h"

#if CFG_ENABLE_WIFI_DIRECT

u_int8_t
p2pDevStateInit_IDLE(struct ADAPTER *prAdapter,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_DEV_STATE *peNextState)
{
	u_int8_t fgIsTransition = FALSE, fgIsShareInterface = TRUE;
	uint32_t u4Idx = 0;
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo;
	struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo;
	void *prRoleHandler = NULL;
	void *prDevHandler = NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prChnlReqInfo) && (peNextState != NULL));

		if (!LINK_IS_EMPTY(&(prChnlReqInfo->rP2pChnlReqLink))) {
			fgIsTransition = TRUE;
			*peNextState = P2P_DEV_STATE_REQING_CHANNEL;
			break;
		}

		prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;
		prP2pMgmtTxInfo = prP2pDevFsmInfo != NULL ?
				&(prP2pDevFsmInfo->rMgmtTxInfo) : NULL;
		if (prP2pDevFsmInfo && prP2pMgmtTxInfo && !LINK_IS_EMPTY(
				&(prP2pMgmtTxInfo->rTxReqLink))) {
			fgIsTransition = TRUE;
			*peNextState = P2P_DEV_STATE_OFF_CHNL_TX;
			break;
		}

		/* Check the interface shared by P2P_DEV and P2P_ROLE or not? */
		/* If not shared, we shall let BSSID4 alive
		 * to receive PROVISION REQUEST from GC
		 */
		prGlueInfo = prAdapter->prGlueInfo;
		if (prGlueInfo) {
			for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++) {
				prRoleHandler = kalGetP2pNetHdl(prGlueInfo,
								u4Idx, TRUE);
				prDevHandler = kalGetP2pNetHdl(prGlueInfo,
								u4Idx, FALSE);
				if ((prRoleHandler != NULL) &&
				(prRoleHandler != prDevHandler) &&
				!p2pFuncIsAPMode(prAdapter, u4Idx)) {
					fgIsShareInterface = FALSE;
					break;
				}
			}
		}
		/************************* End *************************/

		if (fgIsShareInterface ||
			  prP2pDevFsmInfo->ucGoStartedBitmap == 0)
			/* Stay in IDLE state. */
			nicDeactivateNetwork(prAdapter,
				prAdapter->ucP2PDevBssIdx);
		else
			DBGLOG(P2P, INFO,
				"Keep P2P Device alive. isShareIface[%u] GoStartedBitmap[0x%x]\n",
				fgIsShareInterface,
				prP2pDevFsmInfo->ucGoStartedBitmap);

#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
		if (prP2pDevFsmInfo && prP2pDevFsmInfo->fgIsP2pListening
			&& prAdapter->rWifiVar.ucDbdcP2pLisEn) {
			if (prAdapter->rWifiVar.u4DbdcP2pLisSwDelayTime) {
				cnmTimerStopTimer(prAdapter,
				&(prP2pDevFsmInfo->rP2pListenDbdcTimer));

				cnmTimerStartTimer(prAdapter,
				&(prP2pDevFsmInfo->rP2pListenDbdcTimer),
				prAdapter->rWifiVar.u4DbdcP2pLisSwDelayTime);
			} else {
				prP2pDevFsmInfo->fgIsP2pListening = FALSE;
				cnmDbdcRuntimeCheckDecision(prAdapter,
					prAdapter->ucP2PDevBssIdx, FALSE);
			}
		}
#endif
	} while (FALSE);

	return fgIsTransition;
}				/* p2pDevStateInit_IDLE */

void p2pDevStateAbort_IDLE(struct ADAPTER *prAdapter)
{
	/* Currently Aobrt IDLE do nothing. */
}				/* p2pDevStateAbort_IDLE */

u_int8_t
p2pDevStateInit_REQING_CHANNEL(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
	enum ENUM_P2P_DEV_STATE *peNextState)
{
	u_int8_t fgIsTransition = FALSE;
	struct MSG_P2P_CHNL_REQUEST *prP2pMsgChnlReq =
		(struct MSG_P2P_CHNL_REQUEST *) NULL;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo =
				(struct P2P_DEV_FSM_INFO *) NULL;
#endif

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prChnlReqInfo != NULL) && (peNextState != NULL));

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
		if (!prBssInfo)
			break;
		if (LINK_IS_EMPTY(&(prChnlReqInfo->rP2pChnlReqLink))) {
			/* NO Channel Request Pending. */
			DBGLOG(P2P, ERROR,
				"NO Pending Channel Request, but enter Req Channel State\n");
			fgIsTransition = TRUE;
			*peNextState = P2P_DEV_STATE_IDLE;
			break;
		}

		LINK_REMOVE_HEAD(&(prChnlReqInfo->rP2pChnlReqLink),
			prP2pMsgChnlReq, struct MSG_P2P_CHNL_REQUEST *);

		if (prP2pMsgChnlReq == NULL) {
			ASSERT(FALSE);
			break;
		}

		if (prBssInfo->fgIsWmmInited == FALSE)
			prBssInfo->ucWmmQueSet = MAX_HW_WMM_INDEX;
		prBssInfo->eBand = prP2pMsgChnlReq->rChannelInfo.eBand;
#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
		prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;
		DBGLOG(P2P, INFO,
			"ucDbdcP2pLisEn %u P2pLisSwDelayTime %u\n"
			, prAdapter->rWifiVar.ucDbdcP2pLisEn
			, prAdapter->rWifiVar.u4DbdcP2pLisSwDelayTime);

		if (prP2pDevFsmInfo &&
			prAdapter->rWifiVar.ucDbdcP2pLisEn) {
#if CFG_SUPPORT_DBDC
			struct DBDC_DECISION_INFO rDbdcDecisionInfo = {0};
#endif
			prP2pDevFsmInfo->fgIsP2pListening = TRUE;
			prP2pDevFsmInfo->ucReqChannelNum =
				prP2pMsgChnlReq->rChannelInfo.ucChannelNum;
			prP2pDevFsmInfo->eReqBand =
				prP2pMsgChnlReq->rChannelInfo.eBand;

			cnmTimerStopTimer(prAdapter,
				&(prP2pDevFsmInfo->rP2pListenDbdcTimer));
#if CFG_SUPPORT_DBDC
			CNM_DBDC_ADD_DECISION_INFO(rDbdcDecisionInfo,
				prBssInfo->ucBssIndex,
				prP2pMsgChnlReq->rChannelInfo.eBand,
				prP2pMsgChnlReq->rChannelInfo.ucChannelNum,
				prBssInfo->ucWmmQueSet);

			cnmDbdcPreConnectionEnableDecision(
				prAdapter,
				&rDbdcDecisionInfo);
#endif
		}
#endif
		prChnlReqInfo->u4MaxInterval = prP2pMsgChnlReq->u4Duration;
		prChnlReqInfo->ucReqChnlNum =
			prP2pMsgChnlReq->rChannelInfo.ucChannelNum;
		prChnlReqInfo->eChnlSco = prP2pMsgChnlReq->eChnlSco;
		prChnlReqInfo->eBand = prP2pMsgChnlReq->rChannelInfo.eBand;
		prChnlReqInfo->u8Cookie = prP2pMsgChnlReq->u8Cookie;
		prChnlReqInfo->eChnlReqType = prP2pMsgChnlReq->eChnlReqType;
		prChnlReqInfo->eChannelWidth = prBssInfo->ucVhtChannelWidth;
		prChnlReqInfo->ucCenterFreqS1 =
			prBssInfo->ucVhtChannelFrequencyS1;
		prChnlReqInfo->ucCenterFreqS2 =
			prBssInfo->ucVhtChannelFrequencyS2;

		p2pFuncAcquireCh(prAdapter, ucBssIdx, prChnlReqInfo);
	} while (FALSE);

	if (prP2pMsgChnlReq)
		cnmMemFree(prAdapter, prP2pMsgChnlReq);

	return fgIsTransition;
}				/* p2pDevStateInit_REQING_CHANNEL */

void
p2pDevStateAbort_REQING_CHANNEL(struct ADAPTER *prAdapter,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_DEV_STATE eNextState)
{
	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prChnlReqInfo != NULL)
			&& (eNextState < P2P_DEV_STATE_NUM));

		switch (eNextState) {
		case P2P_DEV_STATE_IDLE:
			/* Channel abort case. */
			p2pFuncReleaseCh(prAdapter,
				prAdapter->ucP2PDevBssIdx, prChnlReqInfo);
			break;
		case P2P_DEV_STATE_CHNL_ON_HAND:
			/* Channel on hand case. */
			break;
		case P2P_DEV_STATE_OFF_CHNL_TX:
			/* OffChannel TX case. */
			break;
		case P2P_DEV_STATE_LISTEN_OFFLOAD:
			/* Listen offload case. */
			break;
		default:
			/* Un-expected state transition. */
			DBGLOG(P2P, ERROR,
				"Unexpected State Transition(eNextState=%d)\n",
				eNextState);
			ASSERT(FALSE);
			break;
		}
	} while (FALSE);
}				/* p2pDevStateAbort_REQING_CHANNEL */

void
p2pDevStateInit_CHNL_ON_HAND(struct ADAPTER *prAdapter,
		 struct BSS_INFO *prP2pBssInfo,
		 struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		 struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	do {
		uint32_t u4TimeoutMs = 0;

		ASSERT_BREAK((prAdapter != NULL)
			&& (prP2pDevFsmInfo != NULL)
			&& (prChnlReqInfo != NULL));

		ASSERT(prChnlReqInfo->eChnlReqType == CH_REQ_TYPE_ROC);

		prChnlReqInfo->ucOriChnlNum = prP2pBssInfo->ucPrimaryChannel;
		prChnlReqInfo->eOriBand = prP2pBssInfo->eBand;
		prChnlReqInfo->eOriChnlSco = prP2pBssInfo->eBssSCO;

		prP2pBssInfo->ucPrimaryChannel = prChnlReqInfo->ucReqChnlNum;
		prP2pBssInfo->eBand = prChnlReqInfo->eBand;
		prP2pBssInfo->eBssSCO = prChnlReqInfo->eChnlSco;

		if (prAdapter->prP2pInfo->ucExtendChanFlag)
			u4TimeoutMs = P2P_DEV_EXTEND_CHAN_TIME;
		else
			u4TimeoutMs = prChnlReqInfo->u4MaxInterval;

		log_dbg(P2P, TRACE,
			"Start channel on hand timer, Cookie: 0x%llx, Interval: %d\n",
			prChnlReqInfo->u8Cookie, u4TimeoutMs);

		cnmTimerStartTimer(prAdapter,
			&(prP2pDevFsmInfo->rP2pFsmTimeoutTimer),
			u4TimeoutMs);

		/* Do NOT report channel ready event again for extension case */
		if (!prAdapter->prP2pInfo->ucExtendChanFlag) {
			kalP2PIndicateChannelReady(prAdapter->prGlueInfo,
					prChnlReqInfo->u8Cookie,
					prChnlReqInfo->ucReqChnlNum,
					prChnlReqInfo->eBand,
					prChnlReqInfo->eChnlSco,
					prChnlReqInfo->u4MaxInterval);
			if (prP2pDevFsmInfo->rQueuedActionFrame.u2Length > 0) {
				kalP2pIndicateQueuedMgmtFrame(
					prAdapter->prGlueInfo,
					&prP2pDevFsmInfo->rQueuedActionFrame);
				p2pFunCleanQueuedMgmtFrame(prAdapter,
					&prP2pDevFsmInfo->rQueuedActionFrame);
			}
		}
	} while (FALSE);
}				/* p2pDevStateInit_CHNL_ON_HAND */

void
p2pDevStateAbort_CHNL_ON_HAND(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_DEV_STATE eNextState)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) || (prChnlReqInfo != NULL));

		cnmTimerStopTimer(prAdapter,
			&(prP2pDevFsmInfo->rP2pFsmTimeoutTimer));

		prP2pBssInfo->ucPrimaryChannel = prChnlReqInfo->ucOriChnlNum;
		prP2pBssInfo->eBand = prChnlReqInfo->eOriBand;
		prP2pBssInfo->eBssSCO = prChnlReqInfo->eOriChnlSco;

		if (eNextState != P2P_DEV_STATE_CHNL_ON_HAND) {
			kalP2PIndicateChannelExpired(prAdapter->prGlueInfo,
					     prChnlReqInfo->u8Cookie,
					     prChnlReqInfo->ucReqChnlNum,
					     prChnlReqInfo->eBand,
					     prChnlReqInfo->eChnlSco);

			p2pFuncReleaseCh(prAdapter,
				prP2pDevFsmInfo->ucBssIndex, prChnlReqInfo);
			p2pFunCleanQueuedMgmtFrame(prAdapter,
					&prP2pDevFsmInfo->rQueuedActionFrame);
		}
	} while (FALSE);
}				/* p2pDevStateAbort_CHNL_ON_HAND */

void p2pDevStateInit_SCAN(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prScanReqInfo != NULL));

		prScanReqInfo->fgIsScanRequest = TRUE;

		p2pFuncRequestScan(prAdapter, ucBssIndex, prScanReqInfo);
	} while (FALSE);
}				/* p2pDevStateInit_CHNL_ON_HAND */

void p2pDevStateAbort_SCAN(struct ADAPTER *prAdapter,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo)
{
	struct P2P_SCAN_REQ_INFO *prScanInfo =
		(struct P2P_SCAN_REQ_INFO *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prP2pDevFsmInfo != NULL));

		prScanInfo = &(prP2pDevFsmInfo->rScanReqInfo);

		p2pFuncCancelScan(prAdapter,
			prP2pDevFsmInfo->ucBssIndex,
			prScanInfo);

		kalP2PIndicateScanDone(prAdapter->prGlueInfo,
			0xFF,
			prScanInfo->fgIsAbort);
	} while (FALSE);
}				/* p2pDevStateAbort_CHNL_ON_HAND */

u_int8_t
p2pDevStateInit_OFF_CHNL_TX(struct ADAPTER *prAdapter,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		enum ENUM_P2P_DEV_STATE *peNextState)
{
	struct P2P_OFF_CHNL_TX_REQ_INFO *prOffChnlTxPkt =
		(struct P2P_OFF_CHNL_TX_REQ_INFO *) NULL;

	if (prAdapter == NULL || prP2pMgmtTxInfo == NULL || peNextState == NULL)
		return FALSE;

	if (LINK_IS_EMPTY(&(prP2pMgmtTxInfo->rTxReqLink))) {
		p2pFuncReleaseCh(prAdapter,
				prAdapter->ucP2PDevBssIdx,
				prChnlReqInfo);
		/* Link is empty, return back to IDLE. */
		*peNextState = P2P_DEV_STATE_IDLE;
		return TRUE;
	}

	prOffChnlTxPkt =
		LINK_PEEK_HEAD(&(prP2pMgmtTxInfo->rTxReqLink),
				struct P2P_OFF_CHNL_TX_REQ_INFO,
				rLinkEntry);

	if (prOffChnlTxPkt == NULL) {
		DBGLOG(P2P, ERROR,
			"Fatal Error, Link not empty but get NULL pointer.\n");
		ASSERT(FALSE);
		return FALSE;
	}

	if (!p2pFuncCheckOnRocChnl(&(prOffChnlTxPkt->rChannelInfo),
			prChnlReqInfo)) {
		DBGLOG(P2P, WARN,
			"req channel(%d) != TX channel(%d), request chnl again",
			prChnlReqInfo->ucReqChnlNum,
			prOffChnlTxPkt->rChannelInfo.ucChannelNum);

		prChnlReqInfo->u8Cookie = prOffChnlTxPkt->u8Cookie;
		prChnlReqInfo->eChnlReqType = CH_REQ_TYPE_OFFCHNL_TX;
		prChnlReqInfo->eBand = prOffChnlTxPkt->rChannelInfo.eBand;
		prChnlReqInfo->ucReqChnlNum =
				prOffChnlTxPkt->rChannelInfo.ucChannelNum;
		prChnlReqInfo->eChnlSco = prOffChnlTxPkt->eChnlExt;
		prChnlReqInfo->u4MaxInterval = prOffChnlTxPkt->u4Duration;

		p2pFuncAcquireCh(prAdapter,
				prP2pDevFsmInfo->ucBssIndex,
				prChnlReqInfo);
	} else {
		cnmTimerStartTimer(prAdapter,
				&(prP2pDevFsmInfo->rP2pFsmTimeoutTimer),
				prOffChnlTxPkt->u4Duration);
		p2pFuncTxMgmtFrame(prAdapter,
				prP2pDevFsmInfo->ucBssIndex,
				prOffChnlTxPkt->prMgmtTxMsdu,
				prOffChnlTxPkt->fgNoneCckRate);

		LINK_REMOVE_HEAD(&(prP2pMgmtTxInfo->rTxReqLink),
				prOffChnlTxPkt,
				struct P2P_OFF_CHNL_TX_REQ_INFO *);
		cnmMemFree(prAdapter, prOffChnlTxPkt);
	}

	return FALSE;
}				/* p2pDevSateInit_OFF_CHNL_TX */

void
p2pDevStateAbort_OFF_CHNL_TX(struct ADAPTER *prAdapter,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_DEV_STATE eNextState)
{
	cnmTimerStopTimer(prAdapter, &(prP2pDevFsmInfo->rP2pFsmTimeoutTimer));

	if (eNextState == P2P_DEV_STATE_OFF_CHNL_TX)
		return;

	p2pFunClearAllTxReq(prAdapter, prP2pMgmtTxInfo);
	p2pFuncReleaseCh(prAdapter,
			prAdapter->ucP2PDevBssIdx,
			prChnlReqInfo);
}				/* p2pDevSateAbort_OFF_CHNL_TX */

void p2pComposeLoProbeRsp(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct WLAN_BEACON_FRAME rProbeRspFrame;
	struct WLAN_BEACON_FRAME *prFrame;
	struct GL_P2P_INFO *prP2PInfo =
		(struct GL_P2P_INFO *) NULL;
	struct MSDU_INFO *prNewMgmtTxMsdu =
		(struct MSDU_INFO *) NULL;
	struct MSDU_INFO *prMgmtTxMsdu =
		(struct MSDU_INFO *) NULL;
	struct BSS_INFO *prBssInfo =
		(struct BSS_INFO *) NULL;
	struct P2P_DEV_FSM_INFO *fsm =
		(struct P2P_DEV_FSM_INFO *) NULL;
	struct P2P_LISTEN_OFFLOAD_INFO *pLoInfo =
		(struct P2P_LISTEN_OFFLOAD_INFO *) NULL;
#ifdef CFG_SUPPORT_PRE_WIFI7
	uint8_t fgHide = TRUE;
#else
	uint8_t fgHide = FALSE;
#endif

	if (!prAdapter)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(P2P, INFO,
			"Bss is not active\n");
		return;
	}

	fsm = prAdapter->rWifiVar.prP2pDevFsmInfo;
	if (!fsm)
		return;
	pLoInfo = &fsm->rLoInfo;

	prMgmtTxMsdu = cnmMgtPktAlloc(prAdapter,
		(int32_t) (pLoInfo->u2IELen + sizeof(uint64_t)
		+ MAC_TX_RESERVED_FIELD));
	if (prMgmtTxMsdu == NULL) {
		DBGLOG(P2P, ERROR, "Allocate TX packet fails.\n");
		return;
	}

	prMgmtTxMsdu->prPacket = pLoInfo->aucIE;
	prMgmtTxMsdu->u2FrameLength = pLoInfo->u2IELen;

	prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[
		prBssInfo->u4PrivateData];

	DBGLOG(P2P, TRACE,
		"Dump probe response content from supplicant.\n");

	DBGLOG_MEM8(P2P, TRACE,
		prMgmtTxMsdu->prPacket,
		prMgmtTxMsdu->u2FrameLength);

	p2pFuncProcessP2pProbeRspAction(prAdapter,
		prMgmtTxMsdu, ucBssIndex);

	/* backup header before free packet from supplicant */
	kalMemCopy(&rProbeRspFrame,
		(uint8_t *)((uintptr_t)prMgmtTxMsdu->prPacket +
		MAC_TX_RESERVED_FIELD),
		sizeof(rProbeRspFrame));

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (prP2PInfo->u2MlIELen != 0)
		fgHide = FALSE;
#endif

	/* compose p2p probe rsp frame */
	prNewMgmtTxMsdu = p2pFuncProcessP2pProbeRsp(prAdapter, ucBssIndex,
						    FALSE, fgHide,
						    &rProbeRspFrame);

	if (prNewMgmtTxMsdu) {
		cnmMgtPktFree(prAdapter, prMgmtTxMsdu);
		prMgmtTxMsdu = prNewMgmtTxMsdu;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		/* temp solution, supplicant only build ml
		 * common info for ml probe resp, so we have to
		 * fill complete per-sta profile when ml ie len
		 * is not 0.
		 */
		mldGenerateProbeRspIE(prAdapter, prMgmtTxMsdu,
			ucBssIndex, &rProbeRspFrame,
			p2pFuncProcessP2pProbeRsp);
#endif
	}

	prFrame = (struct WLAN_BEACON_FRAME *)prMgmtTxMsdu->prPacket;

	DBGLOG(P2P, TRACE,
		"Dump probe response content to FW.\n");
	DBGLOG_MEM8(P2P, TRACE,
		prMgmtTxMsdu->prPacket,
		prMgmtTxMsdu->u2FrameLength);

	pLoInfo->u2IELen =
		prMgmtTxMsdu->u2FrameLength -
		OFFSET_OF(struct WLAN_BEACON_FRAME,
		aucInfoElem) + 2;
	kalMemCopy(pLoInfo->aucIE,
		(uint8_t *) (&prFrame->u2CapInfo),
		pLoInfo->u2IELen);

	DBGLOG(P2P, TRACE,
		"Dump probe response shift content to FW.\n");
	DBGLOG_MEM8(P2P, TRACE,
		pLoInfo->aucIE,
		pLoInfo->u2IELen);

	cnmMgtPktFree(prAdapter, prMgmtTxMsdu);
}

u_int8_t
p2pDevStateInit_LISTEN_OFFLOAD(
	struct ADAPTER *prAdapter,
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
	struct P2P_LISTEN_OFFLOAD_INFO *pLoInfo)
{
	struct CMD_SET_P2P_LO_START_STRUCT *prCmd;
	struct BSS_INFO *bss = (struct BSS_INFO *) NULL;
	uint16_t u2CmdBufLen = 0;

	do {
		if (!prAdapter)
			return FALSE;

		bss = GET_BSS_INFO_BY_INDEX(prAdapter,
			pLoInfo->ucBssIndex);
		if (!bss) {
			DBGLOG(P2P, INFO,
				"Bss is not active\n");
			return FALSE;
		}

		u2CmdBufLen =
			sizeof(struct CMD_SET_P2P_LO_START_STRUCT) +
			pLoInfo->u2IELen;

		prCmd = (struct CMD_SET_P2P_LO_START_STRUCT *)
			cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			u2CmdBufLen);
		if (!prCmd) {
			DBGLOG(P2P, ERROR,
				"cnmMemAlloc for prCmd failed!\n");
			return FALSE;
		}

		prCmd->ucBssIndex = pLoInfo->ucBssIndex;
		prCmd->u2ListenPrimaryCh =
			nicFreq2ChannelNum(pLoInfo->u4Freq * 1000);
		prCmd->u2Period = pLoInfo->u4Period;
		prCmd->u2Interval = pLoInfo->u4Interval;
		prCmd->u2Count = pLoInfo->u4Count;

		/* listen channel */
		bss->ucPrimaryChannel =
			prCmd->u2ListenPrimaryCh;

		p2pComposeLoProbeRsp(prAdapter,
			prAdapter->ucP2PDevBssIdx);

		kalMemCopy(prCmd->aucIE,
			pLoInfo->aucIE,
			pLoInfo->u2IELen);
		prCmd->u4IELen = pLoInfo->u2IELen;

		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SET_P2P_LO_START,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			u2CmdBufLen,
			(uint8_t *) prCmd,
			NULL,
			0);

		cnmMemFree(prAdapter, prCmd);
	} while (FALSE);

	return FALSE;
}

void p2pDevStateAbort_LISTEN_OFFLOAD(
	struct ADAPTER *prAdapter,
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
	struct P2P_LISTEN_OFFLOAD_INFO *pLoInfo,
	enum ENUM_P2P_DEV_STATE eNextState)
{
	struct CMD_SET_P2P_LO_STOP_STRUCT *prCmd;
	uint16_t u2CmdBufLen = 0;

	do {
		if (!prAdapter)
			break;

		u2CmdBufLen =
			sizeof(struct CMD_SET_P2P_LO_STOP_STRUCT);

		prCmd = (struct CMD_SET_P2P_LO_STOP_STRUCT *)
			cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			u2CmdBufLen);
		if (!prCmd) {
			DBGLOG(P2P, ERROR,
				"cnmMemAlloc for prCmd failed!\n");
			break;
		}

		prCmd->ucBssIndex = pLoInfo->ucBssIndex;

		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SET_P2P_LO_STOP,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			u2CmdBufLen,
			(uint8_t *) prCmd,
			NULL,
			0);

		cnmMemFree(prAdapter, prCmd);
	} while (FALSE);
}
#endif /* CFG_ENABLE_WIFI_DIRECT */
