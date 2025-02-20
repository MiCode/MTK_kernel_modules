// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "aa_fsm.c"
 *    \brief  This file defines the FSM for SAA and AAA MODULE.
 *
 *    This file defines the FSM for SAA and AAA MODULE.
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

#if CFG_SUPPORT_RTT
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
struct WIFI_RTT_RESULT {
	struct RTT_RESULT rResult;
	void *pLCI;
	void *pLCR;
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static void rttRequestDoneTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam);
static void rttContRequestTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam);
static void rttFreeAllResults(struct RTT_INFO *prRttInfo);
static void rttUpdateStatus(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct CMD_RTT_REQUEST *prCmd);
static struct RTT_INFO *rttGetInfo(struct ADAPTER *prAdapter);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void rttInit(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	ASSERT(rttInfo);

	rttInfo->fgIsRunning = false;
	rttInfo->fgIsContRunning = false;
	rttInfo->ucSeqNum = 0;

	cnmTimerInitTimer(prAdapter,
		  &rttInfo->rRttDoneTimer,
		  (PFN_MGMT_TIMEOUT_FUNC) rttRequestDoneTimeOut,
		  (uintptr_t)NULL);

	cnmTimerInitTimer(prAdapter,
		  &rttInfo->rRttContTimer,
		  (PFN_MGMT_TIMEOUT_FUNC) rttContRequestTimeOut,
		  (uintptr_t)NULL);

	LINK_INITIALIZE(&rttInfo->rResultList);
}

void rttUninit(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	ASSERT(rttInfo);

	rttFreeAllResults(rttInfo);
	rttUpdateStatus(prAdapter, rttInfo->ucBssIndex, NULL);
}

struct RTT_INFO *rttGetInfo(struct ADAPTER *prAdapter)
{
	if (prAdapter)
		return &(prAdapter->rWifiVar.rRttInfo);
	else
		return NULL;
}

void rttFreeAllResults(struct RTT_INFO *prRttInfo)
{
	struct RTT_RESULT_ENTRY *entry;

	while (!LINK_IS_EMPTY(&prRttInfo->rResultList)) {
		LINK_REMOVE_HEAD(&prRttInfo->rResultList,
			entry, struct RTT_RESULT_ENTRY*);
		kalMemFree(entry, VIR_MEM_TYPE,
			sizeof(struct RTT_RESULT_ENTRY) +
			entry->u2IELen);
	}
}

uint8_t rttIsRunning(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	DBGLOG(RTT, INFO,
		"Running = %d\n",
		rttInfo->fgIsContRunning);

	return rttInfo->fgIsContRunning;
}

void rttUpdateStatus(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct CMD_RTT_REQUEST *prCmd)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	rttInfo->ucBssIndex = ucBssIndex;

	if (prCmd && prCmd->fgEnable) {
		rttInfo->ucSeqNum = prCmd->ucSeqNum;
		rttInfo->fgIsRunning = true;
		rttInfo->fgIsContRunning = true;
		cnmTimerStartTimer(prAdapter,
			&rttInfo->rRttDoneTimer,
			SEC_TO_MSEC(RTT_REQUEST_DONE_TIMEOUT_SEC));
		cnmTimerStartTimer(prAdapter,
			&rttInfo->rRttContTimer,
			SEC_TO_MSEC(RTT_REQUEST_CONT_TIMEOUT_SEC));
	} else {
		rttInfo->fgIsRunning = false;
		cnmTimerStopTimer(prAdapter, &rttInfo->rRttDoneTimer);
	}
}

uint32_t rttSendCmd(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct CMD_RTT_REQUEST *cmd)
{
	uint32_t status;

	status = wlanSendSetQueryCmd(prAdapter,
			CMD_ID_RTT_RANGE_REQUEST,
			TRUE,
			FALSE,
			FALSE,
			nicCmdEventSetCommon,
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_RTT_REQUEST),
			(uint8_t *) cmd, NULL, 0);
	if (status != WLAN_STATUS_FAILURE) {
		/* send cmd to FW successfully */
		rttUpdateStatus(prAdapter, ucBssIndex, cmd);
		return WLAN_STATUS_SUCCESS;
	}

	return WLAN_STATUS_FAILURE;
}

void rttActiveNetwork(struct ADAPTER *prAdapter,
			    uint8_t ucBssIndex, uint8_t active)
{
	if (active) {
		SET_NET_ACTIVE(prAdapter, ucBssIndex);
		nicActivateNetwork(prAdapter, ucBssIndex);
	} else {
		UNSET_NET_ACTIVE(prAdapter, ucBssIndex);
		nicDeactivateNetwork(prAdapter, ucBssIndex);
	}

}

uint8_t rttChannelWidthToCnmChBw(enum WIFI_CHANNEL_WIDTH eChannelWidth)
{
	uint8_t ucCnmBw = CW_20_40MHZ;

	switch (eChannelWidth) {
	case WIFI_CHAN_WIDTH_20:
	case WIFI_CHAN_WIDTH_40:
		ucCnmBw = CW_20_40MHZ;
		break;
	case WIFI_CHAN_WIDTH_80:
		ucCnmBw = CW_80MHZ;
		break;
	case WIFI_CHAN_WIDTH_160:
		ucCnmBw = CW_160MHZ;
		break;
	default:
		ucCnmBw = CW_80MHZ;
		break;
	}

	return ucCnmBw;
}

uint8_t rttBwToBssBw(uint8_t eRttBw)
{
	uint8_t ucBssBw = MAX_BW_20MHZ;

	switch (eRttBw) {
	case WIFI_RTT_BW_5:
	case WIFI_RTT_BW_10:
	case WIFI_RTT_BW_20:
		ucBssBw = MAX_BW_20MHZ;
		break;
	case WIFI_RTT_BW_40:
		ucBssBw = MAX_BW_40MHZ;
		break;
	case WIFI_RTT_BW_80:
		ucBssBw = MAX_BW_80MHZ;
		break;
	case WIFI_RTT_BW_160:
		ucBssBw = MAX_BW_160MHZ;
		break;
	default:
		ucBssBw = MAX_BW_160MHZ;
		break;
	}

	return ucBssBw;
}

uint8_t rttBssBwToRttBw(uint8_t ucBssBw)
{
	uint8_t eRttBw = WIFI_RTT_BW_20;

	switch (ucBssBw) {
	case MAX_BW_20MHZ:
		eRttBw = WIFI_RTT_BW_20;
		break;
	case MAX_BW_40MHZ:
		eRttBw = WIFI_RTT_BW_40;
		break;
	case MAX_BW_80MHZ:
		eRttBw = WIFI_RTT_BW_80;
		break;
	case MAX_BW_160MHZ:
		eRttBw = WIFI_RTT_BW_160;
		break;
	default:
		eRttBw = WIFI_RTT_BW_160;
		break;
	}

	return eRttBw;
}

uint8_t rttGetBssMaxBwByBand(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	enum ENUM_BAND eBand)
{
	uint8_t ucMaxBandwidth = MAX_BW_20MHZ;

	if (IS_BSS_AIS(prBssInfo)) {
		if (eBand == BAND_2G4)
			ucMaxBandwidth = prAdapter->rWifiVar.ucSta2gBandwidth;
		else if (eBand == BAND_5G)
			ucMaxBandwidth = prAdapter->rWifiVar.ucSta5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (eBand == BAND_6G)
			ucMaxBandwidth = prAdapter->rWifiVar.ucSta6gBandwidth;
#endif
		if (ucMaxBandwidth > prAdapter->rWifiVar.ucStaBandwidth)
			ucMaxBandwidth = prAdapter->rWifiVar.ucStaBandwidth;
	} else if (IS_BSS_P2P(prBssInfo)) {
		/* P2P mode */
		if (prBssInfo->eBand == BAND_2G4)
			ucMaxBandwidth = prAdapter->rWifiVar.ucP2p2gBandwidth;
		else if (prBssInfo->eBand == BAND_5G)
			ucMaxBandwidth = prAdapter->rWifiVar.ucP2p5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (prBssInfo->eBand == BAND_6G)
			ucMaxBandwidth = prAdapter->rWifiVar.ucP2p6gBandwidth;
#endif
	}

	return ucMaxBandwidth;
}

uint8_t rttReviseBw(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex,
			struct BSS_DESC *prBssDesc,
			uint8_t eRttBwIn)
{
	uint8_t ucRttBw;
	uint8_t ucMaxBssBw = MAX_BW_20MHZ;
	struct BSS_INFO *prBssInfo;
	uint8_t eRttBwOut = eRttBwIn;

	ucRttBw = rttBwToBssBw(eRttBwIn);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prBssInfo) {
		ucMaxBssBw = rttGetBssMaxBwByBand(prAdapter,
			prBssInfo, prBssDesc->eBand);
	}

	if (ucRttBw > ucMaxBssBw) {
		eRttBwOut = rttBssBwToRttBw(ucMaxBssBw);
		DBGLOG(RTT, INFO,
			"Convert RTT BW from %d to %d\n",
			eRttBwIn, eRttBwOut);
	}

	return eRttBwOut;
}

uint32_t rttAddPeerStaRec(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex,
			struct BSS_DESC *prBssDesc)
{
	struct STA_RECORD *prStaRec, *prStaRecOfAp;
	struct BSS_INFO *prBssInfo;

	prStaRec = cnmGetStaRecByAddress(prAdapter,
				ucBssIndex,
				prBssDesc->aucSrcAddr);

	if (prStaRec == NULL) { /* RTT with un-associated AP */
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
		if (prBssInfo == NULL) {
			DBGLOG(RTT, ERROR,
				"prBssInfo %d is NULL!\n", ucBssIndex);
			return WLAN_STATUS_FAILURE;
		}

		prStaRec = bssCreateStaRecFromBssDesc(prAdapter,
					STA_TYPE_LEGACY_AP,
					ucBssIndex,
					prBssDesc);
		if (prStaRec == NULL)
			return WLAN_STATUS_RESOURCES;

		prStaRec->ucBssIndex = ucBssIndex;

		/* init the prStaRec */
		/* prStaRec will be zero first in cnmStaRecAlloc() */
		COPY_MAC_ADDR(prStaRec->aucMacAddr, prBssDesc->aucSrcAddr);
		prStaRec->u2BSSBasicRateSet = prBssInfo->u2BSSBasicRateSet;
		prStaRec->ucDesiredPhyTypeSet =
			prAdapter->rWifiVar.ucAvailablePhyTypeSet;
		prStaRec->u2DesiredNonHTRateSet =
			prAdapter->rWifiVar.ucAvailablePhyTypeSet;
		prStaRec->u2OperationalRateSet =
			prBssInfo->u2OperationalRateSet;
		prStaRec->ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
		prStaRec->eStaType = STA_TYPE_LEGACY_AP;

		/* align setting with AP */
		prStaRecOfAp = prBssInfo->prStaRecOfAP;
		if (prStaRecOfAp) {
			prStaRec->u2DesiredNonHTRateSet =
				prStaRecOfAp->u2DesiredNonHTRateSet;
		}

		/* Init lowest rate to prevent CCK in 5G band */
		nicTxUpdateStaRecDefaultRate(prAdapter, prStaRec);

		/* Better to change state here, not at TX Done */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);
	}

	if (prStaRec) {
		DBGLOG(RTT, INFO,
			"RTT w/ %s AP " MACSTR "\n",
			prStaRec->ucStaState == STA_STATE_1 ?
			"un-associated" : "associated",
			MAC2STR(prBssDesc->aucSrcAddr));
	} else {
		DBGLOG(RTT, ERROR,
			"Cannot allocate StaRec for " MACSTR "\n",
			MAC2STR(prBssDesc->aucSrcAddr));

		return WLAN_STATUS_RESOURCES;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t rttRemovePeerStaRec(struct ADAPTER *prAdapter)
{
	struct RTT_RESULT_ENTRY *entry;
	struct STA_RECORD *prStaRec;
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo)
		return WLAN_STATUS_FAILURE;

	LINK_FOR_EACH_ENTRY(entry, &rttInfo->rResultList, rLinkEntry,
			    struct RTT_RESULT_ENTRY) {

		if (!entry)
			break;

		prStaRec = cnmGetStaRecByAddress(prAdapter,
				rttInfo->ucBssIndex,
				entry->rResult.aucMacAddr);

		if (prStaRec && prStaRec->ucStaState == STA_STATE_1) {
			/* Free StaRec for un-assoicated AP */
			DBGLOG(RTT, INFO,
				"Free StaRec for un-assoicated AP " MACSTR "\n",
				MAC2STR(entry->rResult.aucMacAddr));

			cnmStaRecFree(prAdapter, prStaRec);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t rttStartRttRequest(struct ADAPTER *prAdapter,
			 struct PARAM_RTT_REQUEST *prRequest,
			 uint8_t ucBssIndex)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct BSS_DESC *bss;
	struct CMD_RTT_REQUEST *cmd;
	uint8_t i, active;
	uint32_t status;
	uint32_t sz = sizeof(struct CMD_RTT_REQUEST);
	enum ENUM_BAND eBand;
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;

	cmd = (struct CMD_RTT_REQUEST *)
			cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sz);
	if (!cmd)
		return WLAN_STATUS_RESOURCES;

	/* Workaround: Abort scan before doing RTT. */
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	if (prScanInfo->eCurrentState == SCAN_STATE_SCANNING)
		aisFsmStateAbort_SCAN(prAdapter, ucBssIndex);

	active = IS_NET_ACTIVE(prAdapter, ucBssIndex);
	if (!active)
		rttActiveNetwork(prAdapter, ucBssIndex, true);

	kalMemZero(cmd, sizeof(struct CMD_RTT_REQUEST));
	cmd->ucSeqNum = rttInfo->ucSeqNum + 1;
	cmd->fgEnable = true;
	cmd->ucConfigNum = 0;

	for (i = 0; i < prRequest->ucConfigNum; i++) {
		struct RTT_CONFIG *rc = &prRequest->arRttConfigs[i];
		struct RTT_CONFIG *tc = NULL;

		/* add to fw cmd if bss exist */
		bss = scanSearchBssDescByBssid(prAdapter, rc->aucAddr);

		if (bss) {
			status = rttAddPeerStaRec(prAdapter, ucBssIndex, bss);
			if (status != WLAN_STATUS_SUCCESS)
				goto fail;

			tc = &cmd->arRttConfigs[cmd->ucConfigNum++];
			COPY_MAC_ADDR(tc->aucAddr, rc->aucAddr);
			tc->eType = rc->eType;
			tc->rChannel = rc->rChannel;
			tc->u2BurstPeriod = rc->u2BurstPeriod;
			tc->u2NumBurstExponent = rc->u2NumBurstExponent;
			tc->u2PreferencePartialTsfTimer =
				rc->u2PreferencePartialTsfTimer;
			tc->ucNumFramesPerBurst = rc->ucNumFramesPerBurst;
			tc->ucNumRetriesPerFtmr = rc->ucNumRetriesPerFtmr;
			tc->ucLciRequest = rc->ucLciRequest;
			tc->ucLcrRequest = rc->ucLcrRequest;
			tc->ucBurstDuration = rc->ucBurstDuration;
			tc->ePreamble = rc->ePreamble;
			tc->eBw = rttReviseBw(prAdapter, ucBssIndex,
				bss, rc->eBw);
			/* internal data for channel request */
			cnmFreqToChnl(tc->rChannel.center_freq,
				&tc->ucPrimaryChannel, &eBand);
			tc->eBand = (uint8_t) eBand;
			tc->eChannelWidth = rttChannelWidthToCnmChBw(
				tc->rChannel.width);
			tc->ucS1 = nicGetS1(prAdapter, tc->eBand,
				tc->ucPrimaryChannel, tc->eChannelWidth);
			tc->ucS2 = nicGetS2(tc->eBand, tc->ucPrimaryChannel,
				tc->eChannelWidth, tc->ucS1);
			tc->ucBssIndex = ucBssIndex;
			tc->eEventType = rc->eEventType;
			tc->ucASAP = rc->ucASAP;
			tc->ucFtmMinDeltaTime = rc->ucFtmMinDeltaTime;
		} else {
			DBGLOG(RTT, ERROR,
				"Bssid " MACSTR " is not in scan result\n",
				MAC2STR(rc->aucAddr));
			cnmMemFree(prAdapter, (void *) cmd);
			return WLAN_STATUS_NOT_ACCEPTED;
		}
	}

	status = rttSendCmd(prAdapter, ucBssIndex, cmd);

fail:
	if (status != WLAN_STATUS_SUCCESS) {
		/* fail to send cmd, restore active network */
		if (!active)
			rttActiveNetwork(prAdapter, ucBssIndex, false);
	}

	cnmMemFree(prAdapter, (void *) cmd);
	DBGLOG(RTT, INFO, "status=%d, seq=%d", status, rttInfo->ucSeqNum);

	return status;
}

uint32_t rttCancelRttRequest(struct ADAPTER *prAdapter,
			 struct PARAM_RTT_REQUEST *prRequest)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct CMD_RTT_REQUEST *cmd;
	uint32_t status;
	uint32_t sz = sizeof(struct CMD_RTT_REQUEST);

	cmd = (struct CMD_RTT_REQUEST *)
			cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sz);
	if (!cmd)
		return WLAN_STATUS_RESOURCES;

	cmd->ucSeqNum = rttInfo->ucSeqNum;
	cmd->fgEnable = false;
	status = rttSendCmd(prAdapter, rttInfo->ucBssIndex, cmd);

	cnmMemFree(prAdapter, (void *) cmd);
	DBGLOG(RTT, INFO, "status=%d, seq=%d", status, rttInfo->ucSeqNum);

	return status;
}

uint32_t rttHandleRttRequest(struct ADAPTER *prAdapter,
			 struct PARAM_RTT_REQUEST *prRequest,
			 uint8_t ucBssIndex)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	uint32_t status;

	ASSERT(rttInfo);
	ASSERT(prRequest);

	rttInfo = &(prAdapter->rWifiVar.rRttInfo);

	if (prRequest->ucConfigNum > CFG_RTT_MAX_CANDIDATES ||
	    prRequest->ucConfigNum <= 0 ||
	    (prRequest->fgEnable && rttInfo->fgIsRunning) ||
	    (!prRequest->fgEnable && !rttInfo->fgIsRunning))
		return WLAN_STATUS_NOT_ACCEPTED;

	if (prRequest->fgEnable) {
		status = rttStartRttRequest(prAdapter, prRequest, ucBssIndex);
	} else {
		rttFreeAllResults(rttInfo);
		status = rttCancelRttRequest(prAdapter, prRequest);
	}
	return status;
}

static void rttRequestDoneTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam)
{
	rttEventDone(prAdapter, NULL);
}

static void rttContRequestTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	rttInfo->fgIsContRunning = FALSE;
}

void rttReportDone(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	void *pvPacket = NULL;
	struct RTT_RESULT_ENTRY *entry;
	uint32_t u4Count = 0, sz = sizeof(struct WIFI_RTT_RESULT);
	uint32_t u4DataLen = 0, complete = TRUE;

	LINK_FOR_EACH_ENTRY(entry, &rttInfo->rResultList, rLinkEntry,
			    struct RTT_RESULT_ENTRY) {
		u4DataLen += sz + entry->u2IELen;
		u4Count++;
	}
	pvPacket = kalProcessRttReportDone(prAdapter->prGlueInfo,
				u4DataLen, u4Count);

	if (!pvPacket)
		complete = FALSE;

	while (!LINK_IS_EMPTY(&rttInfo->rResultList) && complete) {
		uint32_t size;
		struct WIFI_RTT_RESULT *r = NULL;

		LINK_REMOVE_HEAD(&rttInfo->rResultList,
			entry, struct RTT_RESULT_ENTRY*);
		size = sz + entry->u2IELen;
		r = kalMemAlloc(size, VIR_MEM_TYPE);
		if (r) {
			kalMemCopy(&r->rResult, &entry->rResult,
				sizeof(struct RTT_RESULT));
			r->pLCI = NULL;
			r->pLCR = NULL;
			kalMemCopy(r + 1, entry->aucIE, entry->u2IELen);
			if (unlikely(kalNlaPut(pvPacket,
				RTT_ATTRIBUTE_RESULT, size, r) < 0)) {
				DBGLOG(RTT, ERROR, "put result fail");
				complete = FALSE;
			}
		}
		kalMemFree(r, VIR_MEM_TYPE, size);
		kalMemFree(entry, VIR_MEM_TYPE,
			sizeof(struct RTT_RESULT_ENTRY) + entry->u2IELen);
	}

	if (complete)
		kalCfg80211VendorEvent(pvPacket);
	else
		kalKfreeSkb(pvPacket, TRUE);
}

void rttFakeEvent(struct ADAPTER *prAdapter)
{
	struct EVENT_RTT_RESULT fake;
	uint8_t aucMacAddr[MAC_ADDR_LEN] = {0xa0, 0xab, 0x1b, 0x54, 0x65, 0x34};

	COPY_MAC_ADDR(fake.rResult.aucMacAddr, aucMacAddr);
	fake.rResult.u4BurstNum = 1;
	fake.rResult.u4MeasurementNumber  = 1;
	fake.rResult.u4SuccessNumber  = 1;
	fake.rResult.ucNumPerBurstPeer = 2;
	fake.rResult.eStatus = RTT_STATUS_SUCCESS;
	fake.rResult.ucRetryAfterDuration = 0;
	fake.rResult.eType = RTT_TYPE_1_SIDED;
	fake.rResult.i4Rssi = -50;
	fake.rResult.i4RssiSpread = 2;
	fake.rResult.rTxRate.preamble = 0;
	fake.rResult.rTxRate.nss = 1;
	fake.rResult.rTxRate.bw = 0;
	fake.rResult.rTxRate.rateMcsIdx = 2;
	fake.rResult.rTxRate.bitrate = 15;
	fake.rResult.rRxRate.preamble = 0;
	fake.rResult.rRxRate.nss = 1;
	fake.rResult.rRxRate.bw = 0;
	fake.rResult.rRxRate.rateMcsIdx = 2;
	fake.rResult.rRxRate.bitrate = 15;
	fake.rResult.i8Rtt = 5000;
	fake.rResult.i8RttSd = 300;
	fake.rResult.i8RttSpread = 400;
	fake.rResult.i4DistanceMM = 3500;
	fake.rResult.i4DistanceSdMM = 100;
	fake.rResult.i4DistanceSpreadMM =  100;
	fake.rResult.i8Ts = kalGetBootTime();
	fake.rResult.i4BurstDuration = 2;
	fake.rResult.i4NegotiatedBustNum = 4;
	fake.u2IELen = 0;

	rttEventResult(prAdapter, &fake);
}

void rttEventDone(struct ADAPTER *prAdapter,
		      struct EVENT_RTT_DONE *prEvent)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

#if CFG_RTT_TEST_MODE
	rttFakeEvent(prAdapter);
#endif

	if (prEvent == NULL) { /* timeout case */
		DBGLOG(RTT, WARN, "rttRequestDoneTimeOut Seq=%u\n",
		       rttInfo->ucSeqNum);
		rttFreeAllResults(rttInfo);
		rttCancelRttRequest(prAdapter, NULL);
		rttRemovePeerStaRec(prAdapter);
		rttUpdateStatus(prAdapter, rttInfo->ucBssIndex, NULL);
	} else { /* normal rtt done */
		DBGLOG(RTT, INFO,
		       "Event RTT done seq: FW %u, driver %u\n",
		       prEvent->ucSeqNum, rttInfo->ucSeqNum);
		if (prEvent->ucSeqNum == rttInfo->ucSeqNum) {
			rttRemovePeerStaRec(prAdapter);
			rttUpdateStatus(prAdapter, rttInfo->ucBssIndex, NULL);
		}
		else
			return;
	}
	rttReportDone(prAdapter);
}

void rttEventResult(struct ADAPTER *prAdapter,
		      struct EVENT_RTT_RESULT *prEvent)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct RTT_RESULT_ENTRY *entry;
	uint32_t sz = sizeof(struct RTT_RESULT_ENTRY);

	if (!rttInfo->fgIsRunning) {
		DBGLOG(RTT, WARN, "RTT is not running\n");
		return;
	} else if (!prEvent) {
		DBGLOG(RTT, ERROR, "RTT null result\n");
		return;
	}

	DBGLOG(RTT, INFO,
		"RTT result MAC=" MACSTR ", status=%d, range=%d (mm)\n",
		MAC2STR(prEvent->rResult.aucMacAddr),
		prEvent->rResult.eStatus,
		prEvent->rResult.i4DistanceMM);

	entry = kalMemAlloc(sz + prEvent->u2IELen, VIR_MEM_TYPE);
	if (entry) {
		kalMemCopy(&entry->rResult, &prEvent->rResult,
			sizeof(struct RTT_RESULT));
		entry->u2IELen = prEvent->u2IELen;
		kalMemCopy(entry->aucIE, prEvent->aucIE, prEvent->u2IELen);
		LINK_INSERT_TAIL(&rttInfo->rResultList,
			&entry->rLinkEntry);
	}
}
#endif /* CFG_SUPPORT_RTT */

#if 0
void rttFreeResult(struct RTT_INFO *prRttInfo, uint8_t aucBSSID[])
{
	struct RTT_RESULT_ENTRY *entry;
	struct RTT_RESULT_ENTRY *entryNext;

	LINK_FOR_EACH_ENTRY_SAFE(entry, entryNext,
		&prRttInfo->rResultList, rLinkEntry, struct RTT_RESULT_ENTRY) {

		if (EQUAL_MAC_ADDR(entry->rResult.aucMacAddr, aucBSSID)) {
			uint32_t sz = sizeof(struct RTT_RESULT_ENTRY) +
				      entry->rResult.u2IELen;

			LINK_REMOVE_KNOWN_ENTRY(&prRttInfo->rResultList),
				&entry->rLinkEntry);
			kalMemFree(entry, VIR_MEM_TYPE, sz);
		}
	}
}

void rttFreeAllRequests(struct RTT_INFO *prRttInfo)
{
	kalMemFree(prRttInfo->prCmd,
		VIR_MEM_TYPE, sizeof(struct CMD_RTT_REQUEST));
	prRttInfo->prCmd = NULL;
}

void rttFreeRequest(struct RTT_INFO *prRttInfo, uint8_t aucBSSID[])
{
	struct CMD_RTT_REQUEST *cmd = prRttInfo->prCmd;
	uint8_t i;

	if (cmd) {
		for (i = 0; i < cmd->ucConfigNum; i++) {
			uint8_t *bssid = cmd->arRttConfigs[i].aucAddr;

			if (EQUAL_MAC_ADDR(bssid, aucBSSID))
				break;
		}
		if (i < cmd->ucConfigNum) {
			kalMemCopy(&cmd->arRttConfigs[i],
				&cmd->arRttConfigs[i + 1],
				(cmd->ucConfigNum - i - 1) *
					sizeof(struct RTT_CONFIG));
			cmd->ucConfigNum--;
		}
	}
}
#endif

