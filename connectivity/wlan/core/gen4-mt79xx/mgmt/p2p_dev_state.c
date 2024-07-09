/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
#include "precomp.h"

u_int8_t
p2pDevStateInit_IDLE(IN struct ADAPTER *prAdapter,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		OUT enum ENUM_P2P_DEV_STATE *peNextState)
{
	u_int8_t fgIsTransition = FALSE, fgIsShareInterface = TRUE;
	uint32_t u4Idx = 0;
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo;
	struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo;

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
				if ((prGlueInfo->prP2PInfo[u4Idx] != NULL) &&
				(prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler
				!= NULL) &&
				(prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler
				!=
				prGlueInfo->prP2PInfo[u4Idx]->prDevHandler)) {
					fgIsShareInterface = FALSE;
					break;
				}
			}
		}
		/************************* End *************************/

		if (fgIsShareInterface) {
			/* Stay in IDLE state. */
			UNSET_NET_ACTIVE(prAdapter, prAdapter->ucP2PDevBssIdx);
			nicDeactivateNetwork(prAdapter,
				prAdapter->ucP2PDevBssIdx);
		}
#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
		if (prP2pDevFsmInfo && prP2pDevFsmInfo->fgIsP2pListening
			&& prAdapter->rWifiVar.ucDbdcP2pLisEn) {
			if (prAdapter->rWifiVar.u4DbdcP2pLisSwDelayTime) {
				cnmTimerStopTimer(prAdapter,
					&(prP2pDevFsmInfo->rP2pListenDbdcTimer));

				cnmTimerStartTimer(prAdapter,
					&(prP2pDevFsmInfo->rP2pListenDbdcTimer),
					prAdapter->rWifiVar.u4DbdcP2pLisSwDelayTime);
			}
			else {
				prP2pDevFsmInfo->fgIsP2pListening = FALSE;
				cnmDbdcRuntimeCheckDecision(prAdapter,
						prAdapter->ucP2PDevBssIdx, FALSE);
			}
		}
#endif
	} while (FALSE);

	return fgIsTransition;
}				/* p2pDevStateInit_IDLE */

void p2pDevStateAbort_IDLE(IN struct ADAPTER *prAdapter)
{
	/* Currently Aobrt IDLE do nothing. */
}				/* p2pDevStateAbort_IDLE */

u_int8_t
p2pDevStateInit_REQING_CHANNEL(IN struct ADAPTER *prAdapter,
	IN uint8_t ucBssIdx,
	IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
	OUT enum ENUM_P2P_DEV_STATE *peNextState)
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
			prP2pDevFsmInfo->fgIsP2pListening = TRUE;
			prP2pDevFsmInfo->ucReqChannelNum =
				prP2pMsgChnlReq->rChannelInfo.ucChannelNum;
			prP2pDevFsmInfo->eReqBand =
				prP2pMsgChnlReq->rChannelInfo.eBand;

			cnmTimerStopTimer(prAdapter,
				&(prP2pDevFsmInfo->rP2pListenDbdcTimer));

			cnmDbdcPreConnectionEnableDecision(
							prAdapter,
							prBssInfo->ucBssIndex,
							prP2pMsgChnlReq->rChannelInfo.eBand,
							prP2pMsgChnlReq->rChannelInfo.ucChannelNum,
							prBssInfo->ucWmmQueSet);
		}
#endif
		cnmOpModeGetTRxNss(
			prAdapter, prBssInfo->ucBssIndex,
			&prBssInfo->ucOpRxNss, &prBssInfo->ucOpTxNss);
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
p2pDevStateAbort_REQING_CHANNEL(IN struct ADAPTER *prAdapter,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		IN enum ENUM_P2P_DEV_STATE eNextState)
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
p2pDevStateInit_CHNL_ON_HAND(IN struct ADAPTER *prAdapter,
		 IN struct BSS_INFO *prP2pBssInfo,
		 IN struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		 IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
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

		log_dbg(P2P, INFO,
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
p2pDevStateAbort_CHNL_ON_HAND(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		IN enum ENUM_P2P_DEV_STATE eNextState)
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

void p2pDevStateInit_SCAN(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct P2P_SCAN_REQ_INFO *prScanReqInfo)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prScanReqInfo != NULL));

		prScanReqInfo->fgIsScanRequest = TRUE;

		p2pFuncRequestScan(prAdapter, ucBssIndex, prScanReqInfo);
	} while (FALSE);
}				/* p2pDevStateInit_CHNL_ON_HAND */

void p2pDevStateAbort_SCAN(IN struct ADAPTER *prAdapter,
		IN struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo)
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
p2pDevStateInit_OFF_CHNL_TX(IN struct ADAPTER *prAdapter,
		IN struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		IN struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		OUT enum ENUM_P2P_DEV_STATE *peNextState)
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
p2pDevStateAbort_OFF_CHNL_TX(IN struct ADAPTER *prAdapter,
		IN struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		IN struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		IN enum ENUM_P2P_DEV_STATE eNextState)
{
	cnmTimerStopTimer(prAdapter, &(prP2pDevFsmInfo->rP2pFsmTimeoutTimer));

	if (eNextState == P2P_DEV_STATE_OFF_CHNL_TX)
		return;

	p2pFunClearAllTxReq(prAdapter, prP2pMgmtTxInfo);
	p2pFuncReleaseCh(prAdapter,
			prAdapter->ucP2PDevBssIdx,
			prChnlReqInfo);
}				/* p2pDevSateAbort_OFF_CHNL_TX */
