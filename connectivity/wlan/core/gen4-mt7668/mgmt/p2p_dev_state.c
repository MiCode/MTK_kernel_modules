// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "precomp.h"

BOOLEAN
p2pDevStateInit_IDLE(IN P_ADAPTER_T prAdapter,
		     IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo, OUT P_ENUM_P2P_DEV_STATE_T peNextState)
{
	BOOLEAN fgIsTransition = FALSE, fgIsShareInterface = TRUE;
	UINT_32 u4Idx = 0;
	P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo) && (peNextState != NULL));

		if (!LINK_IS_EMPTY(&(prChnlReqInfo->rP2pChnlReqLink))) {
			fgIsTransition = TRUE;
			*peNextState = P2P_DEV_STATE_REQING_CHANNEL;
			break;
		}

		/* Check the interface shared by P2P_DEV and P2P_ROLE or not? */
		/* If not shared, we shall let BSSID4 alive to receive PROVISION REQUEST from GC */
		prGlueInfo = prAdapter->prGlueInfo;
		if (prGlueInfo) {
			for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++) {
				if ((prGlueInfo->prP2PInfo[u4Idx] != NULL) &&
					(prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler != NULL) &&
					(prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler !=
					prGlueInfo->prP2PInfo[u4Idx]->prDevHandler)) {
					fgIsShareInterface = FALSE;
					break;
				}
			}
		}
		/************************* End *************************/

		if (fgIsShareInterface) {
			/* Stay in IDLE state. */
			UNSET_NET_ACTIVE(prAdapter, P2P_DEV_BSS_INDEX);
			nicDeactivateNetwork(prAdapter, P2P_DEV_BSS_INDEX);
		}
	} while (FALSE);

	return fgIsTransition;
}				/* p2pDevStateInit_IDLE */

VOID p2pDevStateAbort_IDLE(IN P_ADAPTER_T prAdapter)
{
	/* Currently Aobrt IDLE do nothing. */
}				/* p2pDevStateAbort_IDLE */

BOOLEAN
p2pDevStateInit_REQING_CHANNEL(IN P_ADAPTER_T prAdapter,
			       IN UINT_8 ucBssIdx,
			       IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo, OUT P_ENUM_P2P_DEV_STATE_T peNextState)
{
	BOOLEAN fgIsTransition = FALSE;
	P_MSG_P2P_CHNL_REQUEST_T prP2pMsgChnlReq = (P_MSG_P2P_CHNL_REQUEST_T) NULL;
	P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T) NULL;
#if CFG_SUPPORT_DBDC
	CNM_DBDC_CAP_T rDbdcCap;
#endif /*CFG_SUPPORT_DBDC*/

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL) && (peNextState != NULL));

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

		if (LINK_IS_EMPTY(&(prChnlReqInfo->rP2pChnlReqLink))) {
			/* NO Channel Request Pending. */
			DBGLOG(P2P, ERROR, "NO Pending Channel Request, but enter Req Channel State\n");
			fgIsTransition = TRUE;
			*peNextState = P2P_DEV_STATE_IDLE;
			break;
		}

		LINK_REMOVE_HEAD(&(prChnlReqInfo->rP2pChnlReqLink), prP2pMsgChnlReq, P_MSG_P2P_CHNL_REQUEST_T);

		if (!prP2pMsgChnlReq)
			break;

#if (CFG_HW_WMM_BY_BSS == 1)
		if (prBssInfo->fgIsWmmInited == FALSE)
			prBssInfo->ucWmmQueSet = MAX_HW_WMM_INDEX;
#endif
#if CFG_SUPPORT_DBDC
		cnmGetDbdcCapability(prAdapter,
			prBssInfo->ucBssIndex,
			prP2pMsgChnlReq->rChannelInfo.eBand,
			prP2pMsgChnlReq->rChannelInfo.ucChannelNum,
			wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex),
			&rDbdcCap);

		prBssInfo->eDBDCBand = ENUM_BAND_AUTO;
		prBssInfo->ucNss = rDbdcCap.ucNss;
#if (CFG_HW_WMM_BY_BSS == 0)
		prBssInfo->ucWmmQueSet = rDbdcCap.ucWmmSetIndex;
#endif
#endif /*CFG_SUPPORT_DBDC*/
		prChnlReqInfo->u4MaxInterval = prP2pMsgChnlReq->u4Duration;
		prChnlReqInfo->ucReqChnlNum = prP2pMsgChnlReq->rChannelInfo.ucChannelNum;
		prChnlReqInfo->eChnlSco = prP2pMsgChnlReq->eChnlSco;
		prChnlReqInfo->eBand = prP2pMsgChnlReq->rChannelInfo.eBand;
		prChnlReqInfo->u8Cookie = prP2pMsgChnlReq->u8Cookie;
		prChnlReqInfo->eChnlReqType = prP2pMsgChnlReq->eChnlReqType;
		prChnlReqInfo->eChannelWidth = prBssInfo->ucVhtChannelWidth;
		prChnlReqInfo->ucCenterFreqS1 = prBssInfo->ucVhtChannelFrequencyS1;
		prChnlReqInfo->ucCenterFreqS2 = prBssInfo->ucVhtChannelFrequencyS2;

		p2pFuncAcquireCh(prAdapter, ucBssIdx, prChnlReqInfo);
	} while (FALSE);

	if (prP2pMsgChnlReq)
		cnmMemFree(prAdapter, prP2pMsgChnlReq);

	return fgIsTransition;
}				/* p2pDevStateInit_REQING_CHANNEL */

VOID
p2pDevStateAbort_REQING_CHANNEL(IN P_ADAPTER_T prAdapter,
				IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo, IN ENUM_P2P_DEV_STATE_T eNextState)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL) && (eNextState < P2P_DEV_STATE_NUM));

		switch (eNextState) {
		case P2P_DEV_STATE_IDLE:
			/* Channel abort case. */
			p2pFuncReleaseCh(prAdapter, P2P_DEV_BSS_INDEX, prChnlReqInfo);
			break;
		case P2P_DEV_STATE_CHNL_ON_HAND:
			/* Channel on hand case. */
			break;
		default:
			/* Un-expected state transition. */
			DBGLOG(P2P, ERROR, "Unexpected State Transition(eNextState=%d)\n", eNextState);
			ASSERT(FALSE);
			break;
		}
	} while (FALSE);
}				/* p2pDevStateAbort_REQING_CHANNEL */

VOID
p2pDevStateInit_CHNL_ON_HAND(IN P_ADAPTER_T prAdapter,
			     IN P_BSS_INFO_T prP2pBssInfo,
			     IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo, IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prP2pDevFsmInfo != NULL) && (prChnlReqInfo != NULL));

		ASSERT(prChnlReqInfo->eChnlReqType == CH_REQ_TYPE_P2P_LISTEN);

		prChnlReqInfo->ucOriChnlNum = prP2pBssInfo->ucPrimaryChannel;
		prChnlReqInfo->eOriBand = prP2pBssInfo->eBand;
		prChnlReqInfo->eOriChnlSco = prP2pBssInfo->eBssSCO;

		prP2pBssInfo->ucPrimaryChannel = prChnlReqInfo->ucReqChnlNum;
		prP2pBssInfo->eBand = prChnlReqInfo->eBand;
		prP2pBssInfo->eBssSCO = prChnlReqInfo->eChnlSco;

		cnmTimerStartTimer(prAdapter, &(prP2pDevFsmInfo->rP2pFsmTimeoutTimer), prChnlReqInfo->u4MaxInterval);

		kalP2PIndicateChannelReady(prAdapter->prGlueInfo,
					   prChnlReqInfo->u8Cookie,
					   prChnlReqInfo->ucReqChnlNum,
					   prChnlReqInfo->eBand, prChnlReqInfo->eChnlSco, prChnlReqInfo->u4MaxInterval);
	} while (FALSE);
}				/* p2pDevStateInit_CHNL_ON_HAND */

VOID
p2pDevStateAbort_CHNL_ON_HAND(IN P_ADAPTER_T prAdapter,
			      IN P_BSS_INFO_T prP2pBssInfo,
			      IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo, IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) || (prChnlReqInfo != NULL));

		cnmTimerStopTimer(prAdapter, &(prP2pDevFsmInfo->rP2pFsmTimeoutTimer));

		prP2pBssInfo->ucPrimaryChannel = prChnlReqInfo->ucOriChnlNum;
		prP2pBssInfo->eBand = prChnlReqInfo->eOriBand;
		prP2pBssInfo->eBssSCO = prChnlReqInfo->eOriChnlSco;

		kalP2PIndicateChannelExpired(prAdapter->prGlueInfo,
					     prChnlReqInfo->u8Cookie,
					     prChnlReqInfo->ucReqChnlNum,
					     prChnlReqInfo->eBand, prChnlReqInfo->eChnlSco);

		p2pFuncReleaseCh(prAdapter, prP2pDevFsmInfo->ucBssIndex, prChnlReqInfo);
	} while (FALSE);
}				/* p2pDevStateAbort_CHNL_ON_HAND */

VOID p2pDevStateInit_SCAN(IN P_ADAPTER_T prAdapter, IN UINT_8 ucBssIndex, IN P_P2P_SCAN_REQ_INFO_T prScanReqInfo)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prScanReqInfo != NULL));

		prScanReqInfo->fgIsScanRequest = TRUE;

		p2pFuncRequestScan(prAdapter, ucBssIndex, prScanReqInfo);
	} while (FALSE);
}				/* p2pDevStateInit_CHNL_ON_HAND */

VOID p2pDevStateAbort_SCAN(IN P_ADAPTER_T prAdapter, IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo)
{
	P_P2P_SCAN_REQ_INFO_T prScanInfo = (P_P2P_SCAN_REQ_INFO_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prP2pDevFsmInfo != NULL));

		prScanInfo = &(prP2pDevFsmInfo->rScanReqInfo);

		p2pFuncCancelScan(prAdapter, prP2pDevFsmInfo->ucBssIndex, prScanInfo);

		kalP2PIndicateScanDone(prAdapter->prGlueInfo, 0xFF, prScanInfo->fgIsAbort);
	} while (FALSE);
}				/* p2pDevStateAbort_CHNL_ON_HAND */

BOOLEAN
p2pDevStateInit_OFF_CHNL_TX(IN P_ADAPTER_T prAdapter,
			    IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo,
			    IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo,
			    IN P_P2P_MGMT_TX_REQ_INFO_T prP2pMgmtTxInfo, OUT P_ENUM_P2P_DEV_STATE_T peNextState)
{
	P_P2P_OFF_CHNL_TX_REQ_INFO_T prP2pOffChnlTxPkt = (P_P2P_OFF_CHNL_TX_REQ_INFO_T) NULL;
	BOOLEAN fgIsTransition = FALSE;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prP2pMgmtTxInfo != NULL)
			     && (peNextState != NULL));

		if (!LINK_IS_EMPTY(&(prP2pMgmtTxInfo->rP2pTxReqLink))) {
			prP2pOffChnlTxPkt =
			    LINK_PEEK_HEAD(&(prP2pMgmtTxInfo->rP2pTxReqLink), P2P_OFF_CHNL_TX_REQ_INFO_T, rLinkEntry);

			if (prP2pOffChnlTxPkt == NULL) {
				DBGLOG(P2P, ERROR, "Fetal Error, Link not empty but get NULL pointer.\n");
				ASSERT(FALSE);
				break;
			}

			if (prChnlReqInfo->ucReqChnlNum != prP2pOffChnlTxPkt->rChannelInfo.ucChannelNum) {
				prChnlReqInfo->ucReqChnlNum = prP2pOffChnlTxPkt->rChannelInfo.ucChannelNum;
				prChnlReqInfo->eChnlSco = prP2pOffChnlTxPkt->eChnlExt;
				prChnlReqInfo->eBand = prP2pOffChnlTxPkt->rChannelInfo.eBand;
				prChnlReqInfo->u8Cookie = 0;
				prChnlReqInfo->eChannelWidth = CW_20_40MHZ;
				prChnlReqInfo->ucCenterFreqS1 = 0;
				prChnlReqInfo->ucCenterFreqS2 = 0;
				ASSERT(prChnlReqInfo->eChnlReqType == CH_REQ_TYPE_OFFCHNL_TX);

				p2pFuncAcquireCh(prAdapter, prP2pDevFsmInfo->ucBssIndex, prChnlReqInfo);
			} else {
				LINK_REMOVE_HEAD(&(prP2pMgmtTxInfo->rP2pTxReqLink),
						 prP2pOffChnlTxPkt, P_P2P_OFF_CHNL_TX_REQ_INFO_T);

				p2pFuncTxMgmtFrame(prAdapter,
						   prP2pDevFsmInfo->ucBssIndex,
						   prP2pOffChnlTxPkt->prMgmtTxMsdu, prP2pOffChnlTxPkt->fgNoneCckRate);

				prP2pMgmtTxInfo->prMgmtTxMsdu = prP2pOffChnlTxPkt->prMgmtTxMsdu;
				prP2pMgmtTxInfo->fgIsWaitRsp = prP2pOffChnlTxPkt->fgIsWaitRsp;
			}
		} else {
			/* Link is empty, return back to IDLE. */
			*peNextState = P2P_DEV_STATE_IDLE;
			fgIsTransition = TRUE;
		}
	} while (FALSE);

	return fgIsTransition;
}				/* p2pDevSateInit_OFF_CHNL_TX */

VOID
p2pDevStateAbort_OFF_CHNL_TX(IN P_ADAPTER_T prAdapter,
			     IN P_P2P_MGMT_TX_REQ_INFO_T prP2pMgmtTxInfo,
			     IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo, IN ENUM_P2P_DEV_STATE_T eNextState)
{
	P_P2P_OFF_CHNL_TX_REQ_INFO_T prP2pOffChnlTxPkt = (P_P2P_OFF_CHNL_TX_REQ_INFO_T) NULL;

	if (eNextState != P2P_DEV_STATE_OFF_CHNL_TX) {
		while (!LINK_IS_EMPTY(&(prP2pMgmtTxInfo->rP2pTxReqLink))) {
			LINK_REMOVE_HEAD(&(prP2pMgmtTxInfo->rP2pTxReqLink),
					 prP2pOffChnlTxPkt, P_P2P_OFF_CHNL_TX_REQ_INFO_T);

			if (prP2pOffChnlTxPkt)
				kalP2PIndicateMgmtTxStatus(prAdapter->prGlueInfo,
						   prP2pOffChnlTxPkt->prMgmtTxMsdu, FALSE);
			else
				DBGLOG(P2P, INFO, "No packet for indicating Tx status!\n");
		}

		p2pFuncReleaseCh(prAdapter, P2P_DEV_BSS_INDEX, prChnlReqInfo);
	}
}				/* p2pDevSateAbort_OFF_CHNL_TX */
