/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
#include "precomp.h"

void
p2pRoleStateInit_IDLE(IN struct ADAPTER *prAdapter,
		IN struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		IN struct BSS_INFO *prP2pBssInfo)
{
	cnmTimerStartTimer(prAdapter,
		&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer),
		P2P_AP_CHNL_HOLD_TIME_MS);
}				/* p2pRoleStateInit_IDLE */

void
p2pRoleStateAbort_IDLE(IN struct ADAPTER *prAdapter,
		IN struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		IN struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo)
{

	/* AP mode channel hold time. */
	if (prP2pChnlReqInfo->fgIsChannelRequested)
		p2pFuncReleaseCh(prAdapter,
			prP2pRoleFsmInfo->ucBssIndex,
			prP2pChnlReqInfo);

	DBGLOG(P2P, TRACE, "stop role idle timer.\n");
	cnmTimerStopTimer(prAdapter,
		&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer));
}				/* p2pRoleStateAbort_IDLE */

void p2pRoleStateInit_SCAN(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct P2P_SCAN_REQ_INFO *prScanReqInfo)
{
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo =
		(struct P2P_DEV_FSM_INFO *) NULL;
	struct P2P_SCAN_REQ_INFO *prDevScanReqInfo = NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prScanReqInfo != NULL));

		prScanReqInfo->fgIsScanRequest = TRUE;
		if (prScanReqInfo->u4BufLength == 0) {
			/* If we let u4BufLength be zero,
			 * scan module will copy the IE buf from ScanParam
			 * Sometime this content is from AIS,
			 * so we need copy it from P2Pdev
			 */
			prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;
			if (prP2pDevFsmInfo) {
				prDevScanReqInfo =
					&(prP2pDevFsmInfo->rScanReqInfo);
				if (prDevScanReqInfo->u4BufLength != 0) {
					/* IE Buffer */
					kalMemCopy(prScanReqInfo->aucIEBuf,
						prDevScanReqInfo->aucIEBuf,
						prDevScanReqInfo->u4BufLength);
					prScanReqInfo->u4BufLength =
						prDevScanReqInfo->u4BufLength;
					DBGLOG(P2P, TRACE,
						"p2pRoleStateInit_SCAN Copy p2p IE from P2P dev\n");
				}
			} else
				DBGLOG(P2P, ERROR, "No prP2pDevFsmInfo ptr\n");
		}
		p2pFuncRequestScan(prAdapter, ucBssIndex, prScanReqInfo);
	} while (FALSE);
}				/* p2pRoleStateInit_SCAN */

void p2pRoleStateAbort_SCAN(IN struct ADAPTER *prAdapter,
		IN struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo)
{
	struct P2P_SCAN_REQ_INFO *prScanInfo =
		(struct P2P_SCAN_REQ_INFO *) NULL;

	do {
		prScanInfo = &prP2pRoleFsmInfo->rScanReqInfo;

		p2pFuncCancelScan(prAdapter,
			prP2pRoleFsmInfo->ucBssIndex,
			prScanInfo);

		/* TODO: May need indicate port index to upper layer. */
		kalP2PIndicateScanDone(prAdapter->prGlueInfo,
			prP2pRoleFsmInfo->ucRoleIndex,
			prScanInfo->fgIsAbort);

	} while (FALSE);
}				/* p2pRoleStateAbort_SCAN */

void
p2pRoleStateInit_REQING_CHANNEL(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL));

		p2pFuncAcquireCh(prAdapter, ucBssIdx, prChnlReqInfo);

	} while (FALSE);
}				/* p2pRoleStateInit_REQING_CHANNEL */

void
p2pRoleStateAbort_REQING_CHANNEL(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pRoleBssInfo,
		IN struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		IN enum ENUM_P2P_ROLE_STATE eNextState)
{

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prP2pRoleBssInfo != NULL)
			&& (prP2pRoleFsmInfo != NULL));

		if (eNextState == P2P_ROLE_STATE_IDLE) {
			if (prP2pRoleBssInfo->eIntendOPMode
				== OP_MODE_ACCESS_POINT) {
				struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo =
					&(prP2pRoleFsmInfo->rChnlReqInfo);

				if (IS_NET_PWR_STATE_ACTIVE(prAdapter,
					prP2pRoleFsmInfo->ucBssIndex))
					p2pFuncStartGO(prAdapter,
						prP2pRoleBssInfo,
					&(prP2pRoleFsmInfo->rConnReqInfo),
					&(prP2pRoleFsmInfo->rChnlReqInfo));
				else if (prP2pChnlReqInfo->fgIsChannelRequested)
					p2pFuncReleaseCh(prAdapter,
						prP2pRoleFsmInfo->ucBssIndex,
						prP2pChnlReqInfo);
			} else {
				p2pFuncReleaseCh(prAdapter,
					prP2pRoleFsmInfo->ucBssIndex,
					&(prP2pRoleFsmInfo->rChnlReqInfo));
			}
		}
	} while (FALSE);
}				/* p2pRoleStateAbort_REQING_CHANNEL */

void
p2pRoleStateInit_AP_CHNL_DETECTION(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct P2P_SCAN_REQ_INFO *prScanReqInfo,
		IN struct P2P_CONNECTION_REQ_INFO *prConnReqInfo)
{
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t ucPreferedChnl = 0;
	enum ENUM_BAND eBand = BAND_NULL;
	enum ENUM_CHNL_EXT eSco = CHNL_EXT_SCN;

	do {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
		ASSERT_BREAK((prAdapter != NULL) && (prScanReqInfo != NULL)
			     && (prConnReqInfo != NULL) && (prBssInfo != NULL));

		prP2pSpecificBssInfo =
			prAdapter->rWifiVar
				.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

		if ((cnmPreferredChannel(prAdapter,
					 &eBand,
					 &ucPreferedChnl,
					 &eSco) == FALSE)
			&& (prConnReqInfo->rChannelInfo.ucChannelNum == 0)) {

			/* Sparse channel detection. */
			prP2pSpecificBssInfo->ucPreferredChannel = 0;

			prScanReqInfo->eScanType = SCAN_TYPE_PASSIVE_SCAN;
			/* 50ms for passive channel load detection */
			prScanReqInfo->u2PassiveDewellTime = 50;
		} else {
			/* Active scan to shorten scan time. */
			prScanReqInfo->eScanType = SCAN_TYPE_ACTIVE_SCAN;
			prScanReqInfo->u2PassiveDewellTime = 0;

			if (prConnReqInfo->rChannelInfo.ucChannelNum != 0) {
				prP2pSpecificBssInfo->ucPreferredChannel =
					prConnReqInfo->rChannelInfo
						.ucChannelNum;
				prP2pSpecificBssInfo->eRfBand =
					prConnReqInfo->rChannelInfo
						.eBand;
				prP2pSpecificBssInfo->eRfSco = CHNL_EXT_SCN;
			} else {
				prP2pSpecificBssInfo->ucPreferredChannel =
					ucPreferedChnl;
				prP2pSpecificBssInfo->eRfBand = eBand;
				prP2pSpecificBssInfo->eRfSco = eSco;
			}

		}

		/* TODO: See if channel set to include 5G or only 2.4G */
		prScanReqInfo->eChannelSet = SCAN_CHANNEL_2G4;

		prScanReqInfo->fgIsAbort = TRUE;
		prScanReqInfo->fgIsScanRequest = TRUE;
		prScanReqInfo->ucNumChannelList = 0;
		prScanReqInfo->u4BufLength = 0;
		prScanReqInfo->ucSsidNum = 1;
		prScanReqInfo->arSsidStruct[0].ucSsidLen = 0;

		p2pFuncRequestScan(prAdapter, ucBssIndex, prScanReqInfo);

	} while (FALSE);

	return;

}				/* p2pRoleStateInit_AP_CHNL_DETECTION */

void
p2pRoleStateAbort_AP_CHNL_DETECTION(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		IN struct P2P_SCAN_REQ_INFO *prP2pScanReqInfo,
		IN enum ENUM_P2P_ROLE_STATE eNextState)
{
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	struct BSS_INFO *prBssInfo = NULL;

	do {
		if (eNextState == P2P_ROLE_STATE_REQING_CHANNEL) {
			prBssInfo =
				GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
			prP2pSpecificBssInfo =
				prAdapter->rWifiVar
				.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

			if (prP2pSpecificBssInfo->ucPreferredChannel == 0) {
				if (scnQuerySparseChannel(prAdapter,
					&prP2pSpecificBssInfo
						->eRfBand,
					&prP2pSpecificBssInfo
						->ucPreferredChannel)) {
					prP2pSpecificBssInfo->eRfSco =
						CHNL_EXT_SCN;
				} else {
					DBGLOG(P2P, ERROR,
						"Sparse Channel Error, use default settings\n");
					/* Sparse channel false. */
					prP2pSpecificBssInfo->ucPreferredChannel
						= P2P_DEFAULT_LISTEN_CHANNEL;
					prP2pSpecificBssInfo->eRfBand
						= BAND_2G4;
					prP2pSpecificBssInfo->eRfSco
						= CHNL_EXT_SCN;
				}
			}

			prChnlReqInfo->u8Cookie = 0;
			prChnlReqInfo->ucReqChnlNum =
				prP2pSpecificBssInfo->ucPreferredChannel;
			prChnlReqInfo->eBand = prP2pSpecificBssInfo->eRfBand;
			prChnlReqInfo->eChnlSco = prP2pSpecificBssInfo->eRfSco;
			prChnlReqInfo->u4MaxInterval = P2P_AP_CHNL_HOLD_TIME_MS;
			prChnlReqInfo->eChnlReqType = CH_REQ_TYPE_GO_START_BSS;

			prChnlReqInfo->eChannelWidth = CW_20_40MHZ;
			prChnlReqInfo->ucCenterFreqS1 = 0;
			prChnlReqInfo->ucCenterFreqS2 = 0;

		} else {
			p2pFuncCancelScan(prAdapter,
				ucBssIndex,
				prP2pScanReqInfo);
		}
	} while (FALSE);
}

void
p2pRoleStateInit_GC_JOIN(IN struct ADAPTER *prAdapter,
		IN struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	/* P_MSG_JOIN_REQ_T prJoinReqMsg = (P_MSG_JOIN_REQ_T)NULL; */
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prP2pRoleFsmInfo != NULL)
			&& (prChnlReqInfo != NULL));

		prP2pBssInfo =
			GET_BSS_INFO_BY_INDEX(prAdapter,
				prP2pRoleFsmInfo->ucBssIndex);

		/* Setup a join timer. */
		DBGLOG(P2P, TRACE, "Start a join init timer\n");
		cnmTimerStartTimer(prAdapter,
			&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer),
			(prChnlReqInfo->u4MaxInterval
				- AIS_JOIN_CH_GRANT_THRESHOLD));

		p2pFuncGCJoin(prAdapter,
			prP2pBssInfo,
			&(prP2pRoleFsmInfo->rJoinInfo));

	} while (FALSE);
}				/* p2pRoleStateInit_GC_JOIN */

void
p2pRoleStateAbort_GC_JOIN(IN struct ADAPTER *prAdapter,
		IN struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		IN struct P2P_JOIN_INFO *prJoinInfo,
		IN enum ENUM_P2P_ROLE_STATE eNextState)
{
	do {

		if (prJoinInfo->fgIsJoinComplete == FALSE) {
			struct MSG_SAA_FSM_ABORT *prJoinAbortMsg =
				(struct MSG_SAA_FSM_ABORT *) NULL;
			struct BSS_DESC *prBssDesc =
				(struct BSS_DESC *) NULL;

			prJoinAbortMsg =
				(struct MSG_SAA_FSM_ABORT *) cnmMemAlloc(
					prAdapter,
					RAM_TYPE_MSG,
					sizeof(struct MSG_SAA_FSM_ABORT));
			if (!prJoinAbortMsg) {
				DBGLOG(P2P, TRACE,
					"Fail to allocate join abort message buffer\n");
				ASSERT(FALSE);
				return;
			}

			prJoinAbortMsg->rMsgHdr.eMsgId = MID_P2P_SAA_FSM_ABORT;
			prJoinAbortMsg->ucSeqNum = prJoinInfo->ucSeqNumOfReqMsg;
			prJoinAbortMsg->prStaRec = prJoinInfo->prTargetStaRec;

			/* Reset the flag to clear target BSS state */
			prBssDesc = prJoinInfo->prTargetBssDesc;
			if (prBssDesc != NULL) {
				prBssDesc->fgIsConnecting = FALSE;
			}

			mboxSendMsg(prAdapter,
				MBOX_ID_0,
				(struct MSG_HDR *) prJoinAbortMsg,
				MSG_SEND_METHOD_BUF);

		}

		/* Stop Join Timer. */
		cnmTimerStopTimer(prAdapter,
			&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer));

		/* Release channel requested. */
		p2pFuncReleaseCh(prAdapter,
			prP2pRoleFsmInfo->ucBssIndex,
			&(prP2pRoleFsmInfo->rChnlReqInfo));

	} while (FALSE);
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
void
p2pRoleStateInit_DFS_CAC(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL));

		p2pFuncAcquireCh(prAdapter, ucBssIdx, prChnlReqInfo);
	} while (FALSE);
}				/* p2pRoleStateInit_DFS_CAC */

void
p2pRoleStateAbort_DFS_CAC(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pRoleBssInfo,
		IN struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		IN enum ENUM_P2P_ROLE_STATE eNextState)
{
	do {
		cnmTimerStopTimer(prAdapter,
			&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer));

		p2pFuncReleaseCh(prAdapter,
			prP2pRoleFsmInfo->ucBssIndex,
			&(prP2pRoleFsmInfo->rChnlReqInfo));

	} while (FALSE);
}				/* p2pRoleStateAbort_DFS_CAC */

void
p2pRoleStateInit_SWITCH_CHANNEL(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
#if (CFG_SUPPORT_P2P_CSA == 1)
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

	if (!prBssInfo)
		return;
#endif

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL));

#if (CFG_SUPPORT_P2P_CSA == 1)
		prBssInfo->fgIsSwitchingChnl = TRUE;
#endif

		p2pFuncAcquireCh(prAdapter, ucBssIdx, prChnlReqInfo);
	} while (FALSE);
}				/* p2pRoleStateInit_SWITCH_CHANNEL */

void
p2pRoleStateAbort_SWITCH_CHANNEL(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pRoleBssInfo,
		IN struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		IN enum ENUM_P2P_ROLE_STATE eNextState)
{
	do {
		p2pFuncReleaseCh(prAdapter,
			prP2pRoleFsmInfo->ucBssIndex,
			&(prP2pRoleFsmInfo->rChnlReqInfo));
	} while (FALSE);
}				/* p2pRoleStateAbort_SWITCH_CHANNEL */
#endif

void
p2pRoleStatePrepare_To_REQING_CHANNEL_STATE(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo,
		IN struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		OUT struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	enum ENUM_BAND eBandBackup;
	uint8_t ucChannelBackup;
	enum ENUM_CHNL_EXT eSCOBackup;
	uint8_t ucRfBw;

	do {
		/* P2P BSS info is for temporarily use
		 * Request a 80MHz channel before starting AP/GO
		 * to prevent from STA/GC connected too early (before CH abort)
		 * Therefore, STA/GC Rate will drop during DHCP exchange packets
		 */

		/* Previous issue:
		 * Always request 20MHz channel,
		 * but carry 40MHz HT cap/80MHz VHT cap,
		 * then if GC/STA connected before CH abort,
		 * GO/AP cannot listen to GC/STA's 40MHz/80MHz packets.
		 */

		eBandBackup = prBssInfo->eBand;
		ucChannelBackup = prBssInfo->ucPrimaryChannel;
		eSCOBackup = prBssInfo->eBssSCO;

		prBssInfo->ucPrimaryChannel =
			prConnReqInfo->rChannelInfo.ucChannelNum;
		prBssInfo->eBand = prConnReqInfo->rChannelInfo.eBand;

		prBssInfo->eBssSCO = rlmGetScoForAP(prAdapter, prBssInfo);

		ASSERT_BREAK((prAdapter != NULL)
			&& (prConnReqInfo != NULL)
			&& (prChnlReqInfo != NULL));

		prChnlReqInfo->u8Cookie = 0;
		prChnlReqInfo->ucReqChnlNum =
			prConnReqInfo->rChannelInfo.ucChannelNum;
		prChnlReqInfo->eBand = prConnReqInfo->rChannelInfo.eBand;
		prChnlReqInfo->eChnlSco = prBssInfo->eBssSCO;
		prChnlReqInfo->u4MaxInterval = P2P_AP_CHNL_HOLD_TIME_MS;
		prChnlReqInfo->eChnlReqType = CH_REQ_TYPE_GO_START_BSS;

		if (prBssInfo->eBand == BAND_5G) {
			/* Decide RF BW by own OP BW */
#if CFG_SUPPORT_DBDC
			ucRfBw = cnmGetDbdcBwCapability(prAdapter,
				prBssInfo->ucBssIndex);
#else
			ucRfBw = cnmGetBssMaxBw(prAdapter,
				prBssInfo->ucBssIndex);
#endif
			/* Revise to VHT OP BW */
			ucRfBw = rlmGetVhtOpBwByBssOpBw(ucRfBw);
			prChnlReqInfo->eChannelWidth = ucRfBw;
		} else
			prChnlReqInfo->eChannelWidth = CW_20_40MHZ;

		/* TODO: BW80+80 support */
		prChnlReqInfo->ucCenterFreqS1 =
			nicGetVhtS1(prBssInfo->ucPrimaryChannel,
				prChnlReqInfo->eChannelWidth);
		prChnlReqInfo->ucCenterFreqS2 = 0;

		/* If the S1 is invalid, force to change bandwidth */
		if ((prBssInfo->eBand == BAND_5G) &&
			(prChnlReqInfo->ucCenterFreqS1 == 0))
			prChnlReqInfo->eChannelWidth =
				VHT_OP_CHANNEL_WIDTH_20_40;

		DBGLOG(P2P, TRACE,
			"p2pRoleStatePrepare_To_REQING_CHANNEL_STATE\n");

		/* Reset */
		prBssInfo->ucPrimaryChannel = ucChannelBackup;
		prBssInfo->eBand = eBandBackup;
		prBssInfo->eBssSCO = eSCOBackup;
	} while (FALSE);
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
void
p2pRoleStatePrepare_To_DFS_CAC_STATE(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo,
		IN enum ENUM_CHANNEL_WIDTH rChannelWidth,
		IN struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		OUT struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	enum ENUM_BAND eBandBackup;
	uint8_t ucChannelBackup;
	enum ENUM_CHNL_EXT eSCOBackup;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;

	do {

		eBandBackup = prBssInfo->eBand;
		ucChannelBackup = prBssInfo->ucPrimaryChannel;
		eSCOBackup = prBssInfo->eBssSCO;

		prBssInfo->ucPrimaryChannel =
			prConnReqInfo->rChannelInfo.ucChannelNum;
		prBssInfo->eBand = prConnReqInfo->rChannelInfo.eBand;

		prBssInfo->eBssSCO = rlmGetScoForAP(prAdapter, prBssInfo);

		prP2pRoleFsmInfo =
			P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
				prBssInfo->u4PrivateData);

		ASSERT_BREAK((prAdapter != NULL)
			&& (prConnReqInfo != NULL)
			&& (prChnlReqInfo != NULL));

		prChnlReqInfo->u8Cookie = 0;
		prChnlReqInfo->ucReqChnlNum =
			prConnReqInfo->rChannelInfo.ucChannelNum;
		prChnlReqInfo->eBand = prConnReqInfo->rChannelInfo.eBand;
		prChnlReqInfo->eChnlSco = prBssInfo->eBssSCO;
		prChnlReqInfo->u4MaxInterval =
			prAdapter->prGlueInfo
				->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]
				->cac_time_ms;
		prChnlReqInfo->eChnlReqType = CH_REQ_TYPE_DFS_CAC;

		prBssInfo->ucVhtChannelWidth =
			cnmGetBssMaxBwToChnlBW(prAdapter,
				prBssInfo->ucBssIndex);
		prChnlReqInfo->eChannelWidth = prBssInfo->ucVhtChannelWidth;

		if (prChnlReqInfo->eChannelWidth
			== VHT_OP_CHANNEL_WIDTH_80P80) {
			/* TODO: BW80+80 support */
			log_dbg(RLM, WARN, "BW80+80 not support. Fallback  to VHT_OP_CHANNEL_WIDTH_20_40\n");
			prChnlReqInfo->eChannelWidth =
				VHT_OP_CHANNEL_WIDTH_20_40;
			prChnlReqInfo->ucCenterFreqS1 = 0;
			prChnlReqInfo->ucCenterFreqS2 = 0;
		} else {
			prChnlReqInfo->ucCenterFreqS1 =
				rlmGetVhtS1ForAP(prAdapter, prBssInfo);
			prChnlReqInfo->ucCenterFreqS2 = 0;
		}

		DBGLOG(P2P, TRACE,
			"p2pRoleStatePrepare_To_REQING_CHANNEL_STATE\n");

		/* Reset */
		prBssInfo->ucPrimaryChannel = ucChannelBackup;
		prBssInfo->eBand = eBandBackup;
		prBssInfo->eBssSCO = eSCOBackup;
	} while (FALSE);
}
#endif

u_int8_t
p2pRoleStateInit_OFF_CHNL_TX(IN struct ADAPTER *prAdapter,
		IN struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		IN struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		OUT enum ENUM_P2P_ROLE_STATE *peNextState)
{
	struct P2P_OFF_CHNL_TX_REQ_INFO *prOffChnlTxPkt =
			(struct P2P_OFF_CHNL_TX_REQ_INFO *) NULL;

	if (prAdapter == NULL || prP2pMgmtTxInfo == NULL || peNextState == NULL)
		return FALSE;

	if (LINK_IS_EMPTY(&(prP2pMgmtTxInfo->rTxReqLink))) {
		p2pFuncReleaseCh(prAdapter,
			prP2pRoleFsmInfo->ucBssIndex,
			prChnlReqInfo);
		/* Link is empty, return back to IDLE. */
		*peNextState = P2P_ROLE_STATE_IDLE;
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
		prChnlReqInfo->eChannelWidth =
				prOffChnlTxPkt->rChannelInfo.ucChnlBw;
		prChnlReqInfo->ucCenterFreqS1 =
				prOffChnlTxPkt->rChannelInfo.u4CenterFreq1;
		prChnlReqInfo->ucCenterFreqS2 =
				prOffChnlTxPkt->rChannelInfo.u4CenterFreq2;

		p2pFuncAcquireCh(prAdapter,
				prP2pRoleFsmInfo->ucBssIndex,
				prChnlReqInfo);
	} else {
		cnmTimerStartTimer(prAdapter,
			&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer),
			prOffChnlTxPkt->u4Duration);
		p2pFuncTxMgmtFrame(prAdapter,
				prOffChnlTxPkt->ucBssIndex,
				prOffChnlTxPkt->prMgmtTxMsdu,
				prOffChnlTxPkt->fgNoneCckRate);

		LINK_REMOVE_HEAD(&(prP2pMgmtTxInfo->rTxReqLink),
				prOffChnlTxPkt,
				struct P2P_OFF_CHNL_TX_REQ_INFO *);
		cnmMemFree(prAdapter, prOffChnlTxPkt);
	}

	return FALSE;
}

void
p2pRoleStateAbort_OFF_CHNL_TX(IN struct ADAPTER *prAdapter,
		IN struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		IN struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		IN enum ENUM_P2P_ROLE_STATE eNextState)
{
	cnmTimerStopTimer(prAdapter,
			&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer));

	if (eNextState == P2P_ROLE_STATE_OFF_CHNL_TX)
		return;

	p2pFunClearAllTxReq(prAdapter, prP2pMgmtTxInfo);
	p2pFuncReleaseCh(prAdapter,
		prP2pRoleFsmInfo->ucBssIndex,
		prChnlReqInfo);
}
