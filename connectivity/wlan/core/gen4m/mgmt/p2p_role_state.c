// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "precomp.h"

#if CFG_ENABLE_WIFI_DIRECT

void
p2pRoleStateInit_IDLE(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct BSS_INFO *prP2pBssInfo)
{
	cnmTimerStartTimer(prAdapter,
		&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer),
		IS_BSS_AP(prAdapter, prP2pBssInfo)
		? prAdapter->rWifiVar.u4ApChnlHoldTime
		: prAdapter->rWifiVar.u4P2pChnlHoldTime);
}				/* p2pRoleStateInit_IDLE */

void
p2pRoleStateAbort_IDLE(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo)
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

void p2pRoleStateInit_SCAN(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo)
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

void p2pRoleStateAbort_SCAN(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo)
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
p2pRoleStateInit_REQING_CHANNEL(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL));

		if (prChnlReqInfo->eChnlReqType == CH_REQ_TYPE_JOIN) {
			struct P2P_JOIN_INFO *prJoinInfo;

			prJoinInfo = &prP2pRoleFsmInfo->rJoinInfo;
			p2pLinkAcquireChJoin(prAdapter, prP2pRoleFsmInfo,
					     prChnlReqInfo, prJoinInfo);
		} else {
			p2pFuncAcquireCh(prAdapter, ucBssIdx, prChnlReqInfo);
		}

	} while (FALSE);
}				/* p2pRoleStateInit_REQING_CHANNEL */

void
p2pRoleStateAbort_REQING_CHANNEL(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pRoleBssInfo,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		enum ENUM_P2P_ROLE_STATE eNextState)
{
	u_int8_t fgIsStartGO = FALSE;
	uint8_t ucRoleIdx;
	uint8_t ucBssIdx;
	struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo;

	if (!prAdapter || !prP2pRoleBssInfo || !prP2pRoleFsmInfo) {
		DBGLOG(P2P, ERROR, "Null ptr\n");
		return;
	}

	ucRoleIdx = prP2pRoleFsmInfo->ucRoleIndex;
	ucBssIdx = prP2pRoleFsmInfo->ucBssIndex;
	prP2pChnlReqInfo = &prP2pRoleFsmInfo->rChnlReqInfo;

	if (eNextState == P2P_ROLE_STATE_IDLE) {
		if (prP2pRoleBssInfo->eIntendOPMode == OP_MODE_ACCESS_POINT) {
			if (IS_NET_PWR_STATE_ACTIVE(prAdapter, ucBssIdx)) {
				p2pFuncStartGO(prAdapter, prP2pRoleBssInfo,
					       &prP2pRoleFsmInfo->rConnReqInfo,
					       prP2pChnlReqInfo);
				fgIsStartGO = TRUE;
			} else if (prP2pChnlReqInfo->fgIsChannelRequested) {
				p2pFuncReleaseCh(prAdapter, ucBssIdx,
						 prP2pChnlReqInfo);
			}
		} else {
			p2pFuncReleaseCh(prAdapter, ucBssIdx, prP2pChnlReqInfo);
		}
	} else if (eNextState == P2P_ROLE_STATE_SCAN) {
		/* Abort channel anyway */
		p2pFuncReleaseCh(prAdapter, ucBssIdx, prP2pChnlReqInfo);
	}

#ifndef CFG_AP_GO_DELAY_CARRIER_ON
	if (fgIsStartGO && IS_BSS_AP(prAdapter, prP2pRoleBssInfo))
		p2pFuncNotifySapStarted(prAdapter, ucBssIdx);
#endif /* CFG_AP_GO_DELAY_CARRIER_ON */
}				/* p2pRoleStateAbort_REQING_CHANNEL */

void
p2pRoleStateInit_AP_CHNL_DETECTION(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo,
		struct P2P_CONNECTION_REQ_INFO *prConnReqInfo)
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
		if (!prBssInfo)
			return;
		prP2pSpecificBssInfo =
			prAdapter->rWifiVar
				.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];
		if (!prP2pSpecificBssInfo)
			return;

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
p2pRoleStateAbort_AP_CHNL_DETECTION(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		struct P2P_SCAN_REQ_INFO *prP2pScanReqInfo,
		enum ENUM_P2P_ROLE_STATE eNextState)
{
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	struct BSS_INFO *prBssInfo = NULL;

	do {
		if (eNextState == P2P_ROLE_STATE_REQING_CHANNEL) {
			prBssInfo =
				GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
			if (!prBssInfo)
				break;
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

			if (IS_BSS_AP(prAdapter, prBssInfo))
				prChnlReqInfo->u4MaxInterval =
					prAdapter->rWifiVar.u4ApChnlHoldTime;
			else
				prChnlReqInfo->u4MaxInterval =
					prAdapter->rWifiVar.u4P2pChnlHoldTime;

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
p2pRoleStateInit_GC_JOIN(struct ADAPTER *prAdapter,
			 struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
			 struct P2P_JOIN_INFO *prJoinInfo)
{
	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prP2pRoleFsmInfo != NULL)
			&& (prJoinInfo != NULL));

		/* Setup a join timer. */
		DBGLOG(P2P, TRACE, "Start a join init timer\n");
		cnmTimerStartTimer(prAdapter,
			&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer),
			(P2P_GC_JOIN_CH_REQUEST_INTERVAL
				- P2P_GC_JOIN_CH_GRANT_THRESHOLD));

		p2pFuncGCJoin(prAdapter, prP2pRoleFsmInfo, prJoinInfo);
	} while (FALSE);
}				/* p2pRoleStateInit_GC_JOIN */

void
p2pRoleStateAbort_GC_JOIN(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_JOIN_INFO *prJoinInfo,
		enum ENUM_P2P_ROLE_STATE eNextState)
{
	struct BSS_INFO *prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					    prP2pRoleFsmInfo->ucBssIndex);
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo;

	if (!prBssInfo) {
		DBGLOG(P2P, ERROR, "No BssInfo found");
		return;
	}

	prP2pSpecificBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo[
					prBssInfo->u4PrivateData];

	if (prJoinInfo->fgIsJoinSuccess != TRUE) {
		uint8_t i;

		/* Reset the flag to clear target BSS state */
		p2pTargetBssDescResetConnecting(prAdapter,
			prP2pRoleFsmInfo);

		for (i = 0; i < MLD_LINK_MAX; i++) {
			struct BSS_INFO *bss;
			struct STA_RECORD *sta;

			sta = p2pGetLinkStaRec(prP2pRoleFsmInfo, i);
			if (!sta)
				continue;

			bss = GET_BSS_INFO_BY_INDEX(prAdapter,
						    sta->ucBssIndex);
			if (!bss)
				continue;

			p2pLinkStaRecFree(prAdapter, sta, bss);
			p2pSetLinkStaRec(prP2pRoleFsmInfo, NULL, i);
		}
	}

	/* Stop Join Timer. */
	cnmTimerStopTimer(prAdapter,
		&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer));

	/* Release channel requested. */
	p2pFuncReleaseCh(prAdapter,
		prP2pRoleFsmInfo->ucBssIndex,
		&(prP2pRoleFsmInfo->rChnlReqInfo));

	prP2pRoleFsmInfo->rJoinInfo.prTargetStaRec = NULL;

#if CFG_SUPPORT_CCM
	if (prJoinInfo->fgIsJoinSuccess == TRUE &&
	    prP2pSpecificBssInfo->fgIsGcEapolDone)
		ccmChannelSwitchProducer(prAdapter, prBssInfo, __func__);
#endif /* CFG_SUPPORT_CCM */
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
void
p2pRoleStateInit_DFS_CAC(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL));

		p2pFuncAcquireCh(prAdapter, ucBssIdx, prChnlReqInfo);
	} while (FALSE);
}				/* p2pRoleStateInit_DFS_CAC */

void
p2pRoleStateAbort_DFS_CAC(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pRoleBssInfo,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		enum ENUM_P2P_ROLE_STATE eNextState)
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
p2pRoleStateInit_SWITCH_CHANNEL(struct ADAPTER *prAdapter,
				struct P2P_CSA_REQ_INFO *prCsaReqInfo,
				struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL));

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
						  prCsaReqInfo->ucBssIdx);
		if (!prBssInfo)
			break;

		prBssInfo->fgIsSwitchingChnl = TRUE;
		p2pFuncAcquireCh(prAdapter, prCsaReqInfo->ucBssIdx,
				 prChnlReqInfo);
	} while (FALSE);
}				/* p2pRoleStateInit_SWITCH_CHANNEL */

void
p2pRoleStateAbort_SWITCH_CHANNEL(struct ADAPTER *prAdapter,
				 struct P2P_CSA_REQ_INFO *prCsaReqInfo,
				 struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	p2pFuncReleaseCh(prAdapter, prCsaReqInfo->ucBssIdx, prChnlReqInfo);
}				/* p2pRoleStateAbort_SWITCH_CHANNEL */
#endif

void
p2pRoleStatePrepare_To_REQING_CHANNEL_STATE(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
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

		if (IS_BSS_AP(prAdapter, prBssInfo))
			prChnlReqInfo->u4MaxInterval =
			prAdapter->rWifiVar.u4ApChnlHoldTime;
		else
			prChnlReqInfo->u4MaxInterval =
			prAdapter->rWifiVar.u4P2pChnlHoldTime;

		prChnlReqInfo->eChnlReqType = CH_REQ_TYPE_GO_START_BSS;

		if (prBssInfo->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
			|| prBssInfo->eBand == BAND_6G
#endif
		) {
			/* Decide RF BW by own OP BW */
			ucRfBw = cnmOpModeGetMaxBw(prAdapter,
				prBssInfo);
			/* Revise to VHT OP BW */
			ucRfBw = rlmGetVhtOpBwByBssOpBw(ucRfBw);
			prChnlReqInfo->eChannelWidth = ucRfBw;
		} else
			prChnlReqInfo->eChannelWidth = CW_20_40MHZ;

		/* TODO: BW80+80 support */
		prChnlReqInfo->ucCenterFreqS1 = nicGetS1(
			prBssInfo->eBand,
			prBssInfo->ucPrimaryChannel,
			prChnlReqInfo->eChnlSco,
			rlmGetBssOpBwByChannelWidth(prChnlReqInfo->eChnlSco,
				prChnlReqInfo->eChannelWidth));
		prChnlReqInfo->ucCenterFreqS2 = 0;

		/* If the S1 is invalid, force to change bandwidth */
		if ((prBssInfo->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
			|| prBssInfo->eBand == BAND_6G
#endif
			) && (prChnlReqInfo->ucCenterFreqS1 == 0))
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
p2pRoleStatePrepare_To_DFS_CAC_STATE(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		enum ENUM_CHANNEL_WIDTH rChannelWidth,
		struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	enum ENUM_BAND eBandBackup;
	uint8_t ucChannelBackup;
	enum ENUM_CHNL_EXT eSCOBackup;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	uint8_t ucRfBw;
	uint32_t u4CacTimeMs;

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
		prChnlReqInfo->eChnlReqType = CH_REQ_TYPE_DFS_CAC;

		prBssInfo->ucVhtChannelWidth =
			cnmGetBssMaxBwToChnlBW(prAdapter,
				prBssInfo->ucBssIndex);
		prChnlReqInfo->eChannelWidth = prBssInfo->ucVhtChannelWidth;

		if (p2pFuncGetRadarDetectMode() != DFS_DETECT_MODE_NORMAL)
			ucRfBw = prP2pRoleFsmInfo->eDfsChnlBw;
		else
			/* Decide RF BW by own OP BW */
			ucRfBw = cnmGetDbdcBwCapability(prAdapter,
				prBssInfo->ucBssIndex);

		if (p2pFuncIsDualAPMode(prAdapter) &&
			(ucRfBw >= MAX_BW_160MHZ))
			ucRfBw = MAX_BW_80MHZ;

		/* Revise to VHT OP BW */
		ucRfBw = rlmGetVhtOpBwByBssOpBw(ucRfBw);
		prChnlReqInfo->eChannelWidth =
			(enum ENUM_CHANNEL_WIDTH) ucRfBw;

			/* TODO: BW80+80 support */
		prChnlReqInfo->ucCenterFreqS1 = nicGetS1(
			prBssInfo->eBand,
			prBssInfo->ucPrimaryChannel,
			prChnlReqInfo->eChnlSco,
			rlmGetBssOpBwByChannelWidth(prChnlReqInfo->eChnlSco,
				prChnlReqInfo->eChannelWidth));
		prChnlReqInfo->ucCenterFreqS2 = 0;

		/* If the S1 is invalid, force to change bandwidth */
		if (prChnlReqInfo->ucCenterFreqS1 == 0)
			prChnlReqInfo->eChannelWidth =
				VHT_OP_CHANNEL_WIDTH_20_40;

		/* Update CAC time */
		u4CacTimeMs = prAdapter->prGlueInfo
				->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]
				->cac_time_ms;

		if (p2pFuncCheckWeatherRadarBand(prAdapter, prChnlReqInfo))
			u4CacTimeMs = P2P_AP_CAC_WEATHER_CHNL_HOLD_TIME_MS;

		if (p2pFuncIsManualCac())
			u4CacTimeMs = p2pFuncGetDriverCacTime() * 1000;
		else
			p2pFuncSetDriverCacTime(u4CacTimeMs/1000);

		prChnlReqInfo->u4MaxInterval = u4CacTimeMs;

		DBGLOG(P2P, TRACE,
			"p2pRoleStatePrepare_To_DFS_CAC_STATE\n");

		/* Reset */
		prBssInfo->ucPrimaryChannel = ucChannelBackup;
		prBssInfo->eBand = eBandBackup;
		prBssInfo->eBssSCO = eSCOBackup;
	} while (FALSE);
}
#endif

u_int8_t
p2pRoleStateInit_OFF_CHNL_TX(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		enum ENUM_P2P_ROLE_STATE *peNextState)
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
p2pRoleStateAbort_OFF_CHNL_TX(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_ROLE_STATE eNextState)
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
#endif /* CFG_ENABLE_WIFI_DIRECT */
