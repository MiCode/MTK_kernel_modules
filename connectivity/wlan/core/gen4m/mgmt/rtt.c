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
#define REVERSE_BYTES(num) \
	(((num >> 24) & 0xFF)		| \
	 ((num >> 8) & 0xFF00)		| \
	 ((num << 8) & 0xFF0000)	| \
	 ((num << 24) & 0xFF000000))

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static void rttRequestDoneTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam);
static void rttContRequestTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam);
static void rttFreeAllResults(struct RTT_INFO *prRttInfo);
static void rttFreeAllClients(struct RTT_INFO *prRttInfo);
static void rttUpdateStatus(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct CMD_RTT_REQUEST *prCmd);
static struct RTT_INFO *rttGetInfo(struct ADAPTER *prAdapter);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
/* Block scan activity when RTT is continueously running. Note that this flag
 * should only be enabled for testing purpose. Enable this flag in MP version
 * will result in disconnection because roaming will be blocked due to the scan
 * activity being disabled.
 */
uint8_t rttBlockScan(struct ADAPTER *prAdapter)
{
	return prAdapter->rWifiVar.ucRttBlockScan;
}

void rttInit(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	ASSERT(rttInfo);

	rttInfo->fgIsRunning = false;
	rttInfo->fgIsContRunning = false;
	rttInfo->ucSeqNum = 0;
	rttInfo->ucState = RTT_STATE_IDLE;
#if CFG_SUPPORT_PASN
	rttInfo->prRttReq = NULL;
	rttInfo->ucNumPeers = 0;
#endif

	cnmTimerInitTimer(prAdapter,
		  &rttInfo->rRttDoneTimer,
		  rttRequestDoneTimeOut,
		  (uintptr_t)NULL);

	cnmTimerInitTimer(prAdapter,
		  &rttInfo->rRttContTimer,
		  rttContRequestTimeOut,
		  (uintptr_t)NULL);

	LINK_INITIALIZE(&rttInfo->rResultList);
	LINK_INITIALIZE(&rttInfo->rClientList);

	/* Get RTT capability from FW */
	kalMemZero(&rttInfo->rRttCapabilities,
		sizeof(struct RTT_CAPABILITIES));

	wlanSendSetQueryCmd(prAdapter,
			CMD_ID_RTT_GET_CAPABILITIES,
			FALSE,
			TRUE,
			FALSE,
			nicCmdEventRttCapabilities,
			nicOidCmdTimeoutCommon,
			0, NULL,
			(void *)&rttInfo->rRttCapabilities,
			sizeof(struct RTT_CAPABILITIES));
}

void rttUninit(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	ASSERT(rttInfo);

	rttFreeAllResults(rttInfo);
	rttFreeAllClients(rttInfo);
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

void rttFreeAllClients(struct RTT_INFO *prRttInfo)
{
	struct RTT_CLIENT_ENTRY *entry;

	while (!LINK_IS_EMPTY(&prRttInfo->rClientList)) {
		LINK_REMOVE_HEAD(&prRttInfo->rClientList,
			entry, struct RTT_CLIENT_ENTRY*);
		kalMemFree(entry, VIR_MEM_TYPE,
			sizeof(struct RTT_CLIENT_ENTRY));
	}
}

uint8_t rttIsRunning(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	DBGLOG(RTT, LOUD,
		"Running = %d, ContRunning = %d, Block scan = %d\n",
		rttInfo->fgIsRunning,
		rttInfo->fgIsContRunning,
		rttBlockScan(prAdapter));

	if (rttBlockScan(prAdapter))
		return rttInfo->fgIsContRunning;
	else
		return FALSE;
}

uint8_t rttIsSupport(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	return rttInfo->rRttCapabilities.fgRttFtmSupported;
}

void rttUpdateStatus(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct CMD_RTT_REQUEST *prCmd)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	rttInfo->ucBssIndex = ucBssIndex;

	if (prCmd && prCmd->fgEnable) {
		rttInfo->eRttPeerType = prCmd->arRttConfigs[0].ePeer;
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
		rttInfo->ucState = RTT_STATE_IDLE;
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
	if (active)
		nicActivateNetwork(prAdapter, ucBssIndex);
	else
		nicDeactivateNetwork(prAdapter, ucBssIndex);
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
		ucMaxBandwidth = p2pFuncGetMaxBw(prAdapter, prBssInfo->eBand,
						 IS_BSS_AP(prAdapter,
							   prBssInfo));
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
	if (prBssInfo && prBssDesc) {
		ucMaxBssBw = rttGetBssMaxBwByBand(prAdapter,
			prBssInfo, prBssDesc->eBand);

		if (ucRttBw > ucMaxBssBw) {
			eRttBwOut = rttBssBwToRttBw(ucMaxBssBw);
			DBGLOG(RTT, DEBUG,
				"Convert RTT BW from %d to %d\n",
				eRttBwIn, eRttBwOut);
		}
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

		prStaRec->eStaSubtype = STA_SUBTYPE_RTT;
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
		/* In case BSS is not connected with any AP. There is no
		 * PhyTypeSet in BSSInfo. Hence, we should not overwrite
		 * PhyTypeSet in StaRec which is obtained from BSS desc.
		 */
		//prStaRec->ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
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
		DBGLOG(RTT, DEBUG,
			"RTT w/ %s AP " MACSTR ", StaRecIdx=%d, WlanIdx=%d\n",
			prStaRec->ucStaState == STA_STATE_1 ?
			"un-associated" : "associated",
			MAC2STR(prBssDesc->aucSrcAddr),
			prStaRec->ucIndex,
			prStaRec->ucWlanIndex);
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
	struct STA_RECORD *prStaRec, *prStaRecOfAp;
	struct BSS_INFO *prBssInfo;
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo)
		return WLAN_STATUS_FAILURE;

#if CFG_SUPPORT_PASN
	if (rttInfo->ucNumPeers > 0)
		rttCancelPasn(prAdapter, rttInfo->ucBssIndex);
#endif

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, rttInfo->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(RTT, ERROR,
			"prBssInfo %d is NULL!\n", rttInfo->ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	prStaRecOfAp = prBssInfo->prStaRecOfAP;

	LINK_FOR_EACH_ENTRY(entry, &rttInfo->rResultList, rLinkEntry,
			    struct RTT_RESULT_ENTRY) {

		if (!entry)
			break;

		prStaRec = cnmGetStaRecByAddress(prAdapter,
				rttInfo->ucBssIndex,
				entry->rResult.aucMacAddr);

		if (prStaRec) {
			if (prStaRecOfAp &&
				EQUAL_MAC_ADDR(prStaRec->aucMacAddr,
					prStaRecOfAp->aucMacAddr))
				continue;
			else { /* Free StaRec for un-assoicated AP */
				DBGLOG(RTT, DEBUG,
					"Free StaRec for AP " MACSTR "\n",
					MAC2STR(entry->rResult.aucMacAddr));

				cnmStaRecFree(prAdapter, prStaRec);
			}
		}
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t rttAddClientStaRec(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex,
			uint8_t *pucClientMacAddr)
{
	struct STA_RECORD *prStaRec;
	enum ENUM_STA_TYPE eStaType = STA_TYPE_LEGACY_CLIENT;
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct RTT_CLIENT_ENTRY *entry;
	uint32_t sz = sizeof(struct RTT_CLIENT_ENTRY);

#if CFG_SUPPORT_NAN
	if (IS_BSS_INDEX_NAN(prAdapter, ucBssIndex))
		eStaType = STA_TYPE_NAN;
#endif

	prStaRec = cnmGetStaRecByAddress(prAdapter,
			ucBssIndex,
			pucClientMacAddr);

	if (!prStaRec) { /* RTT with new client */
		prStaRec = cnmStaRecAlloc(prAdapter,
			eStaType,
			ucBssIndex,
			pucClientMacAddr);

		if (!prStaRec) {
			DBGLOG(RTT, ERROR,
				"Cannot allocate StaRec for " MACSTR "\n",
				MAC2STR(pucClientMacAddr));
			return WLAN_STATUS_RESOURCES;
		}

		prStaRec->u2BSSBasicRateSet = BASIC_RATE_SET_ERP;
		prStaRec->u2DesiredNonHTRateSet = RATE_SET_ERP;
		prStaRec->u2OperationalRateSet = RATE_SET_ERP;
		prStaRec->ucPhyTypeSet = PHY_TYPE_SET_802_11AC;

		/* Update default Tx rate */
		nicTxUpdateStaRecDefaultRate(prAdapter, prStaRec);

		/* NOTE(Kevin): Better to change state here, not at TX Done */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

		/* Add to client list */
		entry = kalMemAlloc(sz, VIR_MEM_TYPE);
		if (entry) {
			entry->eStaType = eStaType;
			entry->ucSeqNum = rttInfo->ucSeqNum + 1;
			COPY_MAC_ADDR(entry->aucMacAddr, pucClientMacAddr);
			LINK_INSERT_TAIL(&rttInfo->rClientList,
				&entry->rLinkEntry);
		}
	}

	if (prStaRec) {
		DBGLOG(RTT, DEBUG,
			"RTT w/ %s client " MACSTR "\n",
			prStaRec->ucStaState == STA_STATE_1 ?
			"un-associated" : "associated",
			MAC2STR(pucClientMacAddr));
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t rttRemoveClientStaRec(struct ADAPTER *prAdapter)
{
	struct RTT_CLIENT_ENTRY *entry;
	struct RTT_CLIENT_ENTRY *entryNext;
	struct STA_RECORD *prStaRec;
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo)
		return WLAN_STATUS_FAILURE;

	LINK_FOR_EACH_ENTRY_SAFE(entry, entryNext, &rttInfo->rClientList,
		rLinkEntry, struct RTT_CLIENT_ENTRY) {

		prStaRec = cnmGetStaRecByAddress(prAdapter,
				rttInfo->ucBssIndex,
				entry->aucMacAddr);

		if (prStaRec && prStaRec->ucStaState == STA_STATE_1 &&
			entry->ucSeqNum == rttInfo->ucSeqNum) {
			/* Free StaRec for un-associated Client */
			DBGLOG(RTT, DEBUG,
				"Free StaRec for un-assoc client " MACSTR "\n",
				MAC2STR(entry->aucMacAddr));

			cnmStaRecFree(prAdapter, prStaRec);

			/* Remove entry from client list */
			LINK_REMOVE_KNOWN_ENTRY(&rttInfo->rClientList,
				&entry->rLinkEntry);
			kalMemFree(entry, VIR_MEM_TYPE,
				sizeof(struct RTT_CLIENT_ENTRY));
		} else {
			DBGLOG(RTT, DEBUG,
				"Unknown StaRec " MACSTR ", Seq(%u, %u)\n",
				MAC2STR(entry->aucMacAddr),
				entry->ucSeqNum, rttInfo->ucSeqNum);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_PASN
void rttPasnDoneCallback(struct ADAPTER *prAdapter,
		enum PASN_STATUS ePasnStatus,
		struct PASN_RESP_EVENT *prPasnDoneEvt,
		void *pvUserData)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	uint32_t status;
	struct PASN_PEER *prPasnPeer;
	struct STA_RECORD *prStaRec;
	struct BSS_DESC *prBssDesc;
	struct BSS_INFO *prBssInfo;
	uint8_t i;

	ASSERT(rttInfo);

	if (ePasnStatus != PASN_STATUS_SUCCESS)
		goto pasn_failed;

	for (i = 0; i < prPasnDoneEvt->ucNumPeers; i++) {
		prPasnPeer = &prPasnDoneEvt->arPeer[i];

		if (prPasnPeer->eStatus != PASN_STATUS_SUCCESS) {
			DBGLOG(RTT, ERROR,
				"PASN failed, peer idx=%d\n", i);
			goto pasn_failed;
		}

		/* Update BSS color */
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			rttInfo->ucBssIndex);

		prBssDesc = scanSearchBssDescByBssid(prAdapter,
			prPasnPeer->aucPeerAddr);

		if (prBssInfo && prBssDesc) {
			/* Update BSS color & PhyTypeSet */
			rttInfo->ucOldBssColorInfo = prBssInfo->ucBssColorInfo;
			rttInfo->ucOldPhyTypSet = prBssInfo->ucPhyTypeSet;

			/*
			 * bit[7]   : disabled = 0
			 * bit[6]   : partial
			 * bit[5:0] : new color
			 */
			prBssInfo->ucBssColorInfo =
				(prBssDesc->ucBssColorInfo &
					HE_OP_BSSCOLOR_PARTIAL_BSS_COLOR) |
				(prBssDesc->ucBssColorInfo &
					HE_OP_BSSCOLOR_BSS_COLOR_MASK);

			prBssInfo->ucPhyTypeSet = prBssDesc->ucPhyTypeSet;

			DBGLOG(RTT, DEBUG,
				"Update BSSInfo, PhyTypeSet: 0x%x -> 0x%x, BssColor: 0x%x -> 0x%x\n",
				rttInfo->ucOldPhyTypSet,
				prBssInfo->ucPhyTypeSet,
				rttInfo->ucOldBssColorInfo,
				prBssInfo->ucBssColorInfo);

			nicUpdateBssEx(prAdapter, rttInfo->ucBssIndex, FALSE);
		}

		/* Change to STA_STATE_3 for NDP frames exchange */
		prStaRec = cnmGetStaRecByAddress(prAdapter,
			rttInfo->ucBssIndex,
			prPasnPeer->aucPeerAddr);

		if (prStaRec) {
			cnmStaRecChangeState(prAdapter,
				prStaRec, STA_STATE_3);
		}
	}

	DBGLOG(RTT, DEBUG,
		"PASN done success, peers=%d\n",
		prPasnDoneEvt->ucNumPeers);

	if (rttInfo->ucState == RTT_STATE_PASN) {
		/* Continuous sending RTT request to FW */
		status = rttSendCmd(prAdapter,
				rttInfo->ucBssIndex,
				rttInfo->prRttReq);

		cnmMemFree(prAdapter, (void *) rttInfo->prRttReq);
		DBGLOG(RTT, DEBUG,
			"status=%d, seq=%d",
			status, rttInfo->ucSeqNum);
		rttInfo->ucState = RTT_STATE_RTT_START;
	}

	return;

pasn_failed:
	/* PASN failed */
	DBGLOG(RTT, ERROR,
		"PASN done failed\n", prPasnDoneEvt->ucNumPeers);

	cnmMemFree(prAdapter, (void *) rttInfo->prRttReq);
	rttEventDone(prAdapter, NULL);
}

uint32_t rttFillParamKey(struct PARAM_KEY *param, uint8_t *bssid,
	uint8_t *key, uint16_t key_len, uint8_t *seq, uint16_t seq_len,
	uint8_t pairwise, uint32_t key_index, uint32_t cipher, uint8_t bssidx)
{
	uint8_t aucBCAddr[] = BC_MAC_ADDR;

	kalMemZero(param, sizeof(struct PARAM_KEY));

	param->ucBssIdx = bssidx;
	param->u4KeyIndex = key_index;

	if (pairwise && bssid) {
		param->u4KeyIndex |= BIT(31);
		param->u4KeyIndex |= BIT(30);
		COPY_MAC_ADDR(param->arBSSID, bssid);
	} else {		/* Group key */
		COPY_MAC_ADDR(param->arBSSID, aucBCAddr);
	}

	switch (cipher) {
	case RSN_CIPHER_SUITE_WEP40:
		param->ucCipher = CIPHER_SUITE_WEP40;
		break;
	case RSN_CIPHER_SUITE_WEP104:
		param->ucCipher = CIPHER_SUITE_WEP104;
		break;
	case RSN_CIPHER_SUITE_TKIP:
		param->ucCipher = CIPHER_SUITE_TKIP;
		break;
	case RSN_CIPHER_SUITE_CCMP:
		param->ucCipher = CIPHER_SUITE_CCMP;
		break;
	case RSN_CIPHER_SUITE_GCMP:
		param->ucCipher = CIPHER_SUITE_GCMP_128;
		break;
	case RSN_CIPHER_SUITE_GCMP_256:
		param->ucCipher = CIPHER_SUITE_GCMP_256;
		break;
	case RSN_CIPHER_SUITE_AES_128_CMAC:
		param->ucCipher = CIPHER_SUITE_BIP_CMAC_128;
		break;
	case RSN_CIPHER_SUITE_BIP_GMAC_256:
		param->ucCipher = CIPHER_SUITE_BIP_GMAC_256;
		break;
	default:
		DBGLOG(RTT, WARN, "invalid cipher (0x%x)\n", cipher);
		return WLAN_STATUS_FAILURE;
	}

	if (key) {
		if (key_len > sizeof(param->aucKeyMaterial)) {
			DBGLOG(RTT, WARN, "key too long %d\n", key_len);
			return WLAN_STATUS_RESOURCES;
		}

		kalMemCopy(param->aucKeyMaterial, key, key_len);

		if (param->ucCipher == CIPHER_SUITE_TKIP) {
			uint8_t tmp1[8], tmp2[8];

			kalMemCopy(tmp1, &key[16], 8);
			kalMemCopy(tmp2, &key[24], 8);
			kalMemCopy(&param->aucKeyMaterial[16], tmp2, 8);
			kalMemCopy(&param->aucKeyMaterial[24], tmp1, 8);
		}
	}

	param->u4KeyLength = key_len;
	param->u4Length = OFFSET_OF(struct PARAM_KEY, aucKeyMaterial) +
			  param->u4KeyLength;

	DBGLOG(RTT, DEBUG,
		"keyidx=0x%x, keylen=%d, bssid="MACSTR", bssidx=%d\n",
		param->u4KeyIndex,
		param->u4KeyLength,
		MAC2STR(param->arBSSID),
		param->ucBssIdx);

	return WLAN_STATUS_SUCCESS;
}

void rttRemovePTK(struct ADAPTER *ad, struct RTT_CONFIG *rttcfg)
{
	struct PARAM_REMOVE_KEY param;
	uint8_t keyidx = 0;
	uint32_t len = 0;

	param.u4Length = sizeof(struct PARAM_REMOVE_KEY);
	param.u4KeyIndex = keyidx;
	param.ucBssIdx = rttcfg->ucBssIndex;
	COPY_MAC_ADDR(param.arBSSID, rttcfg->aucAddr);

	DBGLOG(RTT, DEBUG,
		"Bss%d BSSID[" MACSTR "] remove key %d\n",
		rttcfg->ucBssIndex, MAC2STR(param.arBSSID), keyidx);

	wlanSetRemoveKey(ad,
		(void *)&param,
		sizeof(struct PARAM_REMOVE_KEY),
		&len, FALSE);
}

uint32_t rttInstallPTK(struct ADAPTER *ad, struct RTT_CONFIG *rttcfg)
{
	struct PARAM_KEY param;
	uint8_t keyidx = 0;
	uint8_t pairwise = TRUE;
	uint32_t len = 0;
	uint32_t u4Cipher;

	/* Reverse the bytes in cipher suite to align
	 * with the definitions in driver.
	 */
	u4Cipher = REVERSE_BYTES(rttcfg->u4Cipher);

	if (rttFillParamKey(&param,
		rttcfg->aucAddr,
		rttcfg->aucTk,
		rttcfg->ucTkLen,
		NULL,
		0,
		pairwise,
		keyidx,
		u4Cipher,
		rttcfg->ucBssIndex)) {
		DBGLOG(RTT, ERROR,
			"BSSID[" MACSTR "] fill key failed\n",
			MAC2STR(rttcfg->aucAddr));

		return WLAN_STATUS_FAILURE;
	}

	wlanSetAddKey(ad, &param, sizeof(param), &len, FALSE);

	return WLAN_STATUS_SUCCESS;
}

uint32_t rttRemoveLtfKeyseed(struct ADAPTER *ad, struct RTT_CONFIG *rttcfg)
{
	struct CMD_RTT_INSTALL_LTF_KEYSEED param;
	struct STA_RECORD *prStaRec;
	uint32_t status;

	prStaRec = cnmGetStaRecByAddress(ad,
		rttcfg->ucBssIndex, rttcfg->aucAddr);

	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	kalMemZero(&param, sizeof(struct CMD_RTT_INSTALL_LTF_KEYSEED));

	param.ucAddRemove = 0; /* remove */
	param.u2WlanIdx = (uint16_t)prStaRec->ucWlanIndex;

	status = wlanSendSetQueryCmd(ad,
			CMD_ID_RTT_INSTALL_LTF_KEYSEED,
			TRUE,
			FALSE,
			FALSE,
			nicCmdEventSetCommon,
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_RTT_INSTALL_LTF_KEYSEED),
			(uint8_t *) &param, NULL, 0);

	return status;
}

uint32_t rttInstallLtfKeyseed(struct ADAPTER *ad, struct RTT_CONFIG *rttcfg)
{
	struct CMD_RTT_INSTALL_LTF_KEYSEED param;
	struct STA_RECORD *prStaRec;
	uint32_t status;

	prStaRec = cnmGetStaRecByAddress(ad,
		rttcfg->ucBssIndex, rttcfg->aucAddr);

	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	kalMemZero(&param, sizeof(struct CMD_RTT_INSTALL_LTF_KEYSEED));

	param.ucAddRemove = 1; /* add */
	param.u2WlanIdx = (uint16_t)prStaRec->ucWlanIndex;
	param.ucLtfKeyseedLen = rttcfg->ucLtfKeyseedLen;
	kalMemCopy(param.aucLtfKeyseed, rttcfg->aucLtfKeyseed,
		rttcfg->ucLtfKeyseedLen);

	status = wlanSendSetQueryCmd(ad,
			CMD_ID_RTT_INSTALL_LTF_KEYSEED,
			TRUE,
			FALSE,
			FALSE,
			nicCmdEventSetCommon,
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_RTT_INSTALL_LTF_KEYSEED),
			(uint8_t *) &param, NULL, 0);

	return status;
}

void rttRangingCtxCallback(struct ADAPTER *prAdapter,
		struct PASN_SECURE_RANGING_CTX *prCtx,
		void *pvUserData)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct RTT_CONFIG *tc = NULL, *cc = NULL;
	uint8_t i;

	ASSERT(rttInfo);

	DBGLOG(RTT, DEBUG,
		"Secure ranging ctx, "MACSTR", TkLen=%d, LtfKeyseedLen=%d\n",
		MAC2STR(prCtx->aucPeerAddr),
		prCtx->ucTkLen, prCtx->ucLtfKeyseedLen);

	/* Find target config */
	for (i = 0; i < rttInfo->prRttReq->ucConfigNum; i++) {
		cc = &rttInfo->prRttReq->arRttConfigs[i];

		if (EQUAL_MAC_ADDR(cc->aucAddr, prCtx->aucPeerAddr)) {
			tc = cc;
			break;
		}
	}

	if (!tc) {
		DBGLOG(RTT, ERROR, "Cannot find target RTT config\n");
		return;
	}

	/* Update secure ranging context */
	if (prCtx->u4Action == QCA_WLAN_VENDOR_SECURE_RANGING_CTX_ACTION_ADD) {
		tc->u4Cipher = prCtx->u4Cipher;
		tc->u4ShaType = prCtx->u4ShaType;
		tc->ucTkLen = prCtx->ucTkLen;
		tc->ucLtfKeyseedLen = prCtx->ucLtfKeyseedLen;

		kalMemCopy(tc->aucTk, prCtx->aucTk,
			prCtx->ucTkLen);
		kalMemCopy(tc->aucLtfKeyseed, prCtx->aucLtfKeyseed,
			prCtx->ucLtfKeyseedLen);

		/* Install keys */
		rttInstallPTK(prAdapter, tc);
		rttInstallLtfKeyseed(prAdapter, tc);
	} else { /* QCA_WLAN_VENDOR_SECURE_RANGING_CTX_ACTION_DELETE */
		tc->ucTkLen = 0;
		tc->ucLtfKeyseedLen = 0;

		/* Delete keys */
		rttRemovePTK(prAdapter, tc);
		rttRemoveLtfKeyseed(prAdapter, tc);
	}
}

uint32_t rttDoPasn(struct ADAPTER *prAdapter,
			struct PARAM_RTT_REQUEST *prRequest,
			uint8_t ucBssIndex)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct STA_RECORD *prStaRec, *prStaRecOfAp;
	struct BSS_DESC *prBssDesc;
	struct BSS_INFO *prBssInfo;
	struct PASN_AUTH rPasnReq;
	struct PASN_PEER *prPeer;
	uint8_t i;
	uint32_t status = WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(RTT, ERROR,
			"prBssInfo %d is NULL!\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	prStaRecOfAp = prBssInfo->prStaRecOfAP;

	memset(&rPasnReq, 0, sizeof(struct PASN_AUTH));

	for (i = 0; i < prRequest->ucConfigNum; i++) {
		struct RTT_CONFIG *rc = &prRequest->arRttConfigs[i];

		prBssDesc = scanSearchBssDescByBssid(prAdapter,
			rc->aucAddr);
		prStaRec = cnmGetStaRecByAddress(prAdapter, ucBssIndex,
			prBssDesc->aucSrcAddr);

		if (!prBssDesc || !prStaRec)
			continue;

		if (prStaRecOfAp &&
			EQUAL_MAC_ADDR(prStaRec->aucMacAddr,
				prStaRecOfAp->aucMacAddr))
			continue;
		else if (rc->eType == RTT_TYPE_2_SIDED_11AZ_NTB) {
			prPeer = &rPasnReq.arPeer[rPasnReq.ucNumPeers];

			COPY_MAC_ADDR(prPeer->aucOwnAddr,
				prBssInfo->aucOwnMacAddr);
			COPY_MAC_ADDR(prPeer->aucPeerAddr,
				prStaRec->aucMacAddr);
			prPeer->ucLtfKeyseedRequired = TRUE;

			// External auth mode
			prStaRec->eAuthAssocState = SAA_STATE_EXTERNAL_AUTH;
			prStaRec->ucAuthAlgNum =
				(uint8_t) AUTH_ALGORITHM_NUM_PASN;

			rPasnReq.ucNumPeers++;
			status = WLAN_STATUS_SUCCESS;
		}
	}

	if (status == WLAN_STATUS_SUCCESS) {
		/* Update peer info */
		rttInfo->ucNumPeers = rPasnReq.ucNumPeers;
		kalMemCopy(rttInfo->arPeer, rPasnReq.arPeer,
			sizeof(struct PASN_PEER) * PASN_MAX_PEERS);

		/* Indicate PASN request */
		rPasnReq.eAction =
			(enum PASN_ACTION)QCA_WLAN_VENDOR_PASN_ACTION_AUTH;

		pasnHandlePasnRequest(prAdapter, ucBssIndex, &rPasnReq,
			rttPasnDoneCallback, rttRangingCtxCallback, NULL);
	} else {
		/* Clean up peer number */
		rttInfo->ucNumPeers = 0;
	}

	return status;
}

uint32_t rttCancelPasn(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct STA_RECORD *prStaRec;
	struct BSS_DESC *prBssDesc;
	struct BSS_INFO *prBssInfo;
	struct PASN_AUTH rPasnReq;
	struct PASN_PEER *prPeer, *prPeerRec;
	uint8_t i;

	if (rttInfo->ucNumPeers == 0) {
		DBGLOG(RTT, ERROR, "No PASN peer!\n");
		return WLAN_STATUS_FAILURE;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(RTT, ERROR,
			"prBssInfo %d is NULL!\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	memset(&rPasnReq, 0, sizeof(struct PASN_AUTH));

	for (i = 0; i < rttInfo->ucNumPeers; i++) {
		prPeerRec = &rttInfo->arPeer[i];

		prBssDesc = scanSearchBssDescByBssid(prAdapter,
			prPeerRec->aucPeerAddr);
		prStaRec = cnmGetStaRecByAddress(prAdapter, ucBssIndex,
			prBssDesc->aucSrcAddr);

		if (!prBssDesc || !prStaRec)
			continue;

		prPeer = &rPasnReq.arPeer[rPasnReq.ucNumPeers];

		COPY_MAC_ADDR(prPeer->aucOwnAddr,
			prBssInfo->aucOwnMacAddr);
		COPY_MAC_ADDR(prPeer->aucPeerAddr,
			prStaRec->aucMacAddr);

		rPasnReq.ucNumPeers++;

		/* Send deauth */
		authSendDeauthFrame(prAdapter, prBssInfo, prStaRec,
			NULL, REASON_CODE_PREV_AUTH_INVALID, NULL);
	}

	rPasnReq.eAction = (enum PASN_ACTION)
		QCA_WLAN_VENDOR_PASN_ACTION_DELETE_SECURE_RANGING_CONTEXT;

	pasnHandlePasnRequest(prAdapter, ucBssIndex, &rPasnReq,
		NULL, NULL, NULL);

	/* Clean up peer number to prevent doing again */
	rttInfo->ucNumPeers = 0;

	/* Restore BSS Color & PhyTypeSet */
	DBGLOG(RTT, DEBUG,
		"Restore BSSInfo, PhyTypeSet: 0x%x -> 0x%x, BssColor: 0x%x -> 0x%x\n",
		prBssInfo->ucPhyTypeSet, rttInfo->ucOldPhyTypSet,
		prBssInfo->ucBssColorInfo, rttInfo->ucOldBssColorInfo);

	prBssInfo->ucBssColorInfo = rttInfo->ucOldBssColorInfo;
	prBssInfo->ucPhyTypeSet = rttInfo->ucOldPhyTypSet;

	nicUpdateBssEx(prAdapter, rttInfo->ucBssIndex, FALSE);

	return WLAN_STATUS_SUCCESS;
}

uint32_t rttDeleteSecureCtx(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec,
			uint8_t ucBssIndex)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct BSS_INFO *prBssInfo;
	struct PASN_AUTH rPasnReq;
	struct PASN_PEER *prPeer, *prPeerRec;
	struct RTT_CONFIG *tc = NULL, *cc = NULL;
	const uint8_t aucNULLAddr[] = NULL_MAC_ADDR;
	uint8_t i;

	if (rttInfo->ucNumPeers == 0) {
		DBGLOG(RTT, ERROR, "No PASN peer!\n");
		return WLAN_STATUS_FAILURE;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(RTT, ERROR,
			"prBssInfo %d is NULL!\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	memset(&rPasnReq, 0, sizeof(struct PASN_AUTH));

	for (i = 0; i < rttInfo->ucNumPeers; i++) {
		prPeerRec = &rttInfo->arPeer[i];

		if (EQUAL_MAC_ADDR(prPeerRec->aucPeerAddr,
			prStaRec->aucMacAddr)) {

			prPeer = &rPasnReq.arPeer[rPasnReq.ucNumPeers];

			COPY_MAC_ADDR(prPeer->aucOwnAddr,
				prBssInfo->aucOwnMacAddr);
			COPY_MAC_ADDR(prPeer->aucPeerAddr,
				prStaRec->aucMacAddr);

			rPasnReq.ucNumPeers++;

			/* Clean up Peer Record in rttInfo */
			COPY_MAC_ADDR(prPeerRec->aucPeerAddr,
				aucNULLAddr);

			break;
		}
	}

	rttInfo->ucNumPeers -= rPasnReq.ucNumPeers;

	rPasnReq.eAction = (enum PASN_ACTION)
		QCA_WLAN_VENDOR_PASN_ACTION_DELETE_SECURE_RANGING_CONTEXT;

	pasnHandlePasnRequest(prAdapter, ucBssIndex, &rPasnReq,
		NULL, NULL, NULL);

	/* Find target config */
	for (i = 0; i < rttInfo->prRttReq->ucConfigNum; i++) {
		cc = &rttInfo->prRttReq->arRttConfigs[i];

		if (EQUAL_MAC_ADDR(cc->aucAddr, prStaRec->aucMacAddr)) {
			tc = cc;
			break;
		}
	}

	if (tc) {
		/* Delete keys */
		rttRemovePTK(prAdapter, tc);
		rttRemoveLtfKeyseed(prAdapter, tc);
	}

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_PASN */

void rttUpdateChannelParams(struct ADAPTER *prAdpater,
	struct WIFI_CHANNEL_INFO *rChannel,
	struct BSS_DESC *bss_desc)
{
	uint32_t center_freq = 0;
	uint32_t center_freq0 = 0;
	uint32_t center_freq1 = 0;

	center_freq = nicChannelNum2Freq(
		bss_desc->ucChannelNum,
		bss_desc->eBand) / 1000;
	center_freq0 = bss_desc->ucCenterFreqS1;
	center_freq1 = bss_desc->ucCenterFreqS2;

	DBGLOG(RTT, DEBUG,
		"Update channel param, freq: %d -> %d, S1: %d -> %d, S2: %d -> %d",
		rChannel->center_freq, center_freq,
		rChannel->center_freq0, center_freq0,
		rChannel->center_freq1, center_freq1);

	rChannel->center_freq = center_freq;
	rChannel->center_freq0 = center_freq0;
	rChannel->center_freq1 = center_freq1;
}

uint32_t rttStartRttRequest(struct ADAPTER *prAdapter,
			 struct PARAM_RTT_REQUEST *prRequest,
			 uint8_t ucBssIndex)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct BSS_DESC *bss = NULL;
	struct CMD_RTT_REQUEST *cmd;
	uint8_t i, active;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint32_t sz = sizeof(struct CMD_RTT_REQUEST);
	enum ENUM_BAND eBand;
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;

	if (!IS_BSS_INDEX_VALID(ucBssIndex)) {
		DBGLOG(RTT, ERROR,
			"Invalid BSS index = %d\n",
			ucBssIndex);
		return WLAN_STATUS_INVALID_DATA;
	}

	cmd = (struct CMD_RTT_REQUEST *)
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sz);

	if (!cmd)
		return WLAN_STATUS_RESOURCES;

	/* Workaround: Abort scan before doing RTT. */
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	if (rttBlockScan(prAdapter) &&
		prScanInfo->eCurrentState == SCAN_STATE_SCANNING) {
		aisFsmStateAbort_SCAN(prAdapter, ucBssIndex);
	}

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

		if (rc->ePeer == RTT_PEER_AP) {
			bss = scanSearchBssDescByBssid(prAdapter, rc->aucAddr);

			if (bss) {
				status = rttAddPeerStaRec(prAdapter,
					ucBssIndex, bss);
				if (status != WLAN_STATUS_SUCCESS)
					goto fail;
			} else {
				DBGLOG(RTT, ERROR,
					"" MACSTR " is not in scan result\n",
					MAC2STR(rc->aucAddr));
				status =  WLAN_STATUS_FAILURE;
				goto fail;
			}
		} else if ((rc->ePeer == RTT_PEER_STA) ||
			(rc->ePeer == RTT_PEER_NAN_RSTA) ||
			(rc->ePeer == RTT_PEER_NAN_ISTA)) {
			rttAddClientStaRec(prAdapter, ucBssIndex, rc->aucAddr);
		}

		tc = &cmd->arRttConfigs[cmd->ucConfigNum++];
		COPY_MAC_ADDR(tc->aucAddr, rc->aucAddr);
		tc->eType = rc->eType;
		tc->ePeer = rc->ePeer;
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
		tc->ucS1 = nicGetS1(tc->eBand, tc->ucPrimaryChannel,
			nicGetSco(prAdapter, tc->eBand, tc->ucPrimaryChannel),
			rttBwToBssBw(rc->eBw));
		tc->ucS2 = nicGetS2(tc->eBand, tc->ucPrimaryChannel,
			tc->eChannelWidth);
		tc->ucBssIndex = ucBssIndex;
		tc->eEventType = rc->eEventType;
		tc->ucASAP = rc->ucASAP;
		tc->ucFtmMinDeltaTime = rc->ucFtmMinDeltaTime;

		/* 11az config */
		tc->u8NtbMinMeasurementTime = rc->u8NtbMinMeasurementTime;
		tc->u8NtbMaxMeasurementTime = rc->u8NtbMaxMeasurementTime;
		tc->ucI2rLmrFeedback = rc->ucI2rLmrFeedback;
		tc->ucImmeR2iFeedback = rc->ucImmeR2iFeedback;
		tc->ucImmeI2rFeedback = rc->ucImmeI2rFeedback;
		tc->ucForceReplyI2rLmr = rc->ucForceReplyI2rLmr;
	}

#if CFG_SUPPORT_PASN
	status = rttDoPasn(prAdapter, prRequest, ucBssIndex);

	if (status == WLAN_STATUS_SUCCESS) {
		/* keep RTT request until PASN is done */
		rttInfo->prRttReq = cmd;
		rttInfo->ucState = RTT_STATE_PASN;

		rttUpdateStatus(prAdapter, ucBssIndex, cmd);
	} else /* No need to do PASN */
#endif
	{
		status = rttSendCmd(prAdapter, ucBssIndex, cmd);

		if (status == WLAN_STATUS_SUCCESS) {
			rttInfo->ucState = RTT_STATE_RTT_START;
			cnmMemFree(prAdapter, (void *) cmd);
		}
	}

fail:
	if (status != WLAN_STATUS_SUCCESS) {
		/* fail to send cmd, restore active network */
		if (!active)
			rttActiveNetwork(prAdapter, ucBssIndex, false);

		cnmMemFree(prAdapter, (void *) cmd);
	}

	DBGLOG(RTT, DEBUG,
		"bssIndex=%d, status=%d, seq=%d",
		ucBssIndex, status, rttInfo->ucSeqNum);

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
	DBGLOG(RTT, DEBUG, "status=%d, seq=%d", status, rttInfo->ucSeqNum);
	rttInfo->ucState = RTT_STATE_IDLE;

	return status;
}

uint32_t rttHandleDeauth(struct ADAPTER *prAdapter,
			 struct STA_RECORD *prStaRec)
{
#if CFG_SUPPORT_PASN
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	ASSERT(rttInfo);

	rttDeleteSecureCtx(prAdapter,
		prStaRec,
		rttInfo->ucBssIndex);
#endif

	return WLAN_STATUS_SUCCESS;
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
	    (prRequest->fgEnable && prRequest->ucConfigNum == 0) ||
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
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (rttInfo->ucState == RTT_STATE_RTT_START
#if CFG_SUPPORT_PASN
		|| rttInfo->ucState == RTT_STATE_PASN
#endif
	)
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

uint32_t rttRemoveStaRec(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo)
		return WLAN_STATUS_FAILURE;

	switch (rttInfo->eRttPeerType) {
	case RTT_PEER_AP:
		rttRemovePeerStaRec(prAdapter);
		break;
	case RTT_PEER_STA:
	case RTT_PEER_NAN_RSTA:
	case RTT_PEER_NAN_ISTA:
		rttRemoveClientStaRec(prAdapter);
		break;
	default:
		DBGLOG(RTT, WARN, "invalid RttPeerType=%u\n",
			rttInfo->eRttPeerType);
		break;
	}

	return WLAN_STATUS_SUCCESS;
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
		rttRemoveStaRec(prAdapter);
		rttUpdateStatus(prAdapter, rttInfo->ucBssIndex, NULL);
	} else { /* normal rtt done */
		DBGLOG(RTT, DEBUG,
			"Event RTT done seq: FW %u, driver %u\n",
			prEvent->ucSeqNum, rttInfo->ucSeqNum);
		if (prEvent->ucSeqNum == rttInfo->ucSeqNum) {
			rttRemoveStaRec(prAdapter);
			rttUpdateStatus(prAdapter, rttInfo->ucBssIndex, NULL);
		}
		else
			return;
	}

	switch (rttInfo->eRttPeerType) {
	case RTT_PEER_NAN_RSTA:
	case RTT_PEER_AP:
		rttReportDone(prAdapter);
		rttInfo->ucState = RTT_STATE_RTT_DONE;
		break;
	case RTT_PEER_STA:
	case RTT_PEER_NAN_ISTA:
	default:
		/* Do nothing */
		break;
	}
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

	DBGLOG(RTT, DEBUG,
		"RTT result MAC=" MACSTR ", status=%d, range=%d (mm)\n",
		MAC2STR(prEvent->rResult.aucMacAddr),
		prEvent->rResult.eStatus,
		prEvent->rResult.i4DistanceMM);

	entry = kalMemAlloc(sz + prEvent->u2IELen, VIR_MEM_TYPE);
	if (entry) {
		kalMemCopy(&entry->rResult, &prEvent->rResult,
			sizeof(struct RTT_RESULT));
#if (CFG_SUPPORT_NAN == 1)
		if (IS_BSS_INDEX_NAN(prAdapter, rttInfo->ucBssIndex)) {
			struct _NAN_RANGING_INSTANCE_T *prRanging = NULL;

			prRanging = nanRangingInstanceSearchByMac(
					prAdapter,
					entry->rResult.aucMacAddr);
			if (prRanging) {
				kalMemCopy(&prRanging->ranging_ctrl
					.rRangingRttResult,
					&entry->rResult,
					sizeof(struct RTT_RESULT));
				nanRangingReportDiscResult(
					prAdapter,
					entry->rResult.aucMacAddr);
			}
		}
#endif /* CFG_SUPPOR_NAN */
		entry->u2IELen = prEvent->u2IELen;
		kalMemCopy(entry->aucIE, prEvent->aucIE, prEvent->u2IELen);
		LINK_INSERT_TAIL(&rttInfo->rResultList,
			&entry->rLinkEntry);
	}

	rttInfo->ucState = RTT_STATE_IDLE;
}
#endif /* CFG_SUPPORT_RTT */

#if CFG_SUPPORT_RTT_RSTA
u_int8_t rttGetRSTABssIndex(struct ADAPTER *prAdapter,
	uint8_t *pucDestAddr)
{
	uint8_t ucBssIndex = 0;
	struct BSS_INFO *prBssInfo = NULL;
	uint16_t u2RangingBand = 0;

	if (!prAdapter)
		return FALSE;

	u2RangingBand =
		prAdapter->rWifiVar.u2NanRangingBand;

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucSwBssIdNum;
		ucBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
		if (prBssInfo &&
			(IS_BSS_APGO(prBssInfo) ||
			IS_BSS_NAN(prBssInfo)) &&
			IS_BSS_ACTIVE(prBssInfo) &&
			EQUAL_MAC_ADDR(pucDestAddr,
			prBssInfo->aucOwnMacAddr)) {
			/* Select ranging band */
			if (IS_BSS_NAN(prBssInfo) &&
				(u2RangingBand > 2) &&
				(prBssInfo->eBand < BAND_5G))
				continue;

			return ucBssIndex;
		}
	}

	return 0;
}

u_int8_t rttIsRSTAActive(struct ADAPTER *prAdapter)
{
	uint8_t ucBssIndex = 0;
	uint8_t fgIsAPActive = FALSE;
	struct BSS_INFO *prBssInfo = NULL;

	if (!prAdapter)
		return FALSE;

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucSwBssIdNum;
		ucBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
		if (prBssInfo &&
			(IS_BSS_APGO(prBssInfo) ||
			IS_BSS_NAN(prBssInfo)) &&
			IS_BSS_ACTIVE(prBssInfo)) {
			fgIsAPActive = TRUE;
			break;
		}
	}

	return fgIsAPActive;
}

enum WIFI_CHANNEL_WIDTH rttFtmChannelWidth(uint8_t ucFormatAndBandwidth)
{
	enum WIFI_CHANNEL_WIDTH eChannelWidth = WIFI_CHAN_WIDTH_80;

	switch (ucFormatAndBandwidth) {
	case FTM_FORMAT_BW_HT_MIXED_BW20:
	case FTM_FORMAT_BW_VHT_BW20:
		eChannelWidth = WIFI_CHAN_WIDTH_20;
		break;
	case FTM_FORMAT_BW_HT_MIXED_BW40:
	case FTM_FORMAT_BW_VHT_BW40:
		eChannelWidth = WIFI_CHAN_WIDTH_40;
		break;
	case FTM_FORMAT_BW_VHT_BW160:
		eChannelWidth = WIFI_CHAN_WIDTH_160;
		break;
	case FTM_FORMAT_BW_VHT_BW80:
	default:
		eChannelWidth = WIFI_CHAN_WIDTH_80;
		break;
	}

	return eChannelWidth;
}

enum ENUM_WIFI_RTT_PREAMBLE rttFtmPreamble(uint8_t ucFormatAndBandwidth)
{
	enum ENUM_WIFI_RTT_PREAMBLE ePreamble = WIFI_RTT_PREAMBLE_VHT;

	switch (ucFormatAndBandwidth) {
	case FTM_FORMAT_BW_HT_MIXED_BW20:
	case FTM_FORMAT_BW_HT_MIXED_BW40:
		ePreamble = WIFI_RTT_PREAMBLE_HT;
		break;
	case FTM_FORMAT_BW_VHT_BW20:
	case FTM_FORMAT_BW_VHT_BW40:
	case FTM_FORMAT_BW_VHT_BW80:
	case FTM_FORMAT_BW_VHT_BW160:
	default:
		ePreamble = WIFI_RTT_PREAMBLE_VHT;
		break;
	}

	return ePreamble;
}

uint32_t rttProcessFTM(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		struct WLAN_ACTION_FRAME *prActFrame,
		struct FTM_INFO_ELEM *prFtmInfoElem)
{
	struct PARAM_RTT_REQUEST *rttReq = NULL;
	enum WIFI_CHANNEL_WIDTH channelWidth;
	uint8_t ucBssIndex = 0;

	rttReq = kalMemAlloc(sizeof(struct PARAM_RTT_REQUEST), VIR_MEM_TYPE);
	if (!rttReq) {
		DBGLOG(RTT, ERROR, "fail to alloc memory for rttReq.\n");
		return -1;
	}

	ucBssIndex = rttGetRSTABssIndex(prAdapter,
		prActFrame->aucDestAddr);

	kalMemZero(rttReq, sizeof(struct PARAM_RTT_REQUEST));
	rttReq->fgEnable = true;
	rttReq->ucConfigNum = 1;
	channelWidth = rttFtmChannelWidth(prFtmInfoElem->ucFormatAndBandwidth);

	COPY_MAC_ADDR(rttReq->arRttConfigs[0].aucAddr, prActFrame->aucSrcAddr);
	rttReq->arRttConfigs[0].eType = RTT_TYPE_2_SIDED_11MC;
	if (IS_BSS_INDEX_NAN(prAdapter, ucBssIndex)) {
		rttReq->arRttConfigs[0].ePeer = RTT_PEER_NAN_ISTA;
		if (prAdapter->rWifiVar.u2NanRangingBand > 2)
			channelWidth = WIFI_CHAN_WIDTH_80;
	} else
		rttReq->arRttConfigs[0].ePeer = RTT_PEER_STA;
	rttReq->arRttConfigs[0].rChannel.width = channelWidth;
	rttReq->arRttConfigs[0].rChannel.center_freq =
		nicChannelNum2Freq(prSwRfb->ucChnlNum, prSwRfb->eRfBand) / 1000;
	rttReq->arRttConfigs[0].rChannel.center_freq0 = 0;
	rttReq->arRttConfigs[0].rChannel.center_freq1 = 0;
	rttReq->arRttConfigs[0].u2BurstPeriod = 0;
	rttReq->arRttConfigs[0].u2NumBurstExponent =
		prFtmInfoElem->ucNumberOfBurstsExponent;
	rttReq->arRttConfigs[0].u2PreferencePartialTsfTimer = 0;
	rttReq->arRttConfigs[0].ucNumFramesPerBurst =
		prFtmInfoElem->ucFtmPerBurst;
	rttReq->arRttConfigs[0].ucNumRetriesPerRttFrame = 3;
	rttReq->arRttConfigs[0].ucNumRetriesPerFtmr = 0;
	rttReq->arRttConfigs[0].ucLciRequest = 0;
	rttReq->arRttConfigs[0].ucLcrRequest = 0;
	rttReq->arRttConfigs[0].ucBurstDuration =
		prFtmInfoElem->ucBurstDuration;
	rttReq->arRttConfigs[0].ePreamble =
		rttFtmPreamble(prFtmInfoElem->ucFormatAndBandwidth);
	rttReq->arRttConfigs[0].eBw = rttBssBwToRttBw(channelWidth);
	rttReq->arRttConfigs[0].ucASAP = 1;
	rttReq->arRttConfigs[0].ucFtmMinDeltaTime =
		prFtmInfoElem->ucMinDeltaFtm;

	rttHandleRttRequest(prAdapter, rttReq,
		ucBssIndex);

	kalMemFree(rttReq, VIR_MEM_TYPE, sizeof(struct PARAM_RTT_REQUEST));

	return WLAN_STATUS_SUCCESS;
}

void rttProcessFTMR(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb)
{
	uint8_t *pucIE;
	uint16_t u2IELength;
	uint16_t u2Offset = 0;
	struct FTM_INFO_ELEM *prFtmInfoElem;
	struct WLAN_ACTION_FRAME *prActFrame;
	struct ACTION_FTM_REQUEST_FRAME *prRxFrame;

	prActFrame = (struct WLAN_ACTION_FRAME *) prSwRfb->pvHeader;
	prRxFrame = (struct ACTION_FTM_REQUEST_FRAME *) prSwRfb->pvHeader;

	u2IELength = prSwRfb->u2PacketLen - (uint16_t)OFFSET_OF(
		struct ACTION_FTM_REQUEST_FRAME, aucInfoElem[0]);
	pucIE = prRxFrame->aucInfoElem;

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_FINE_TIMING_MEASUREMENT:
			if (IE_LEN(pucIE) !=
				(sizeof(struct FTM_INFO_ELEM) - 2)) {
				DBGLOG(RTT, ERROR, "Invalid IE length\n");
				break;
			}

			prFtmInfoElem = (struct FTM_INFO_ELEM *)pucIE;
			rttProcessFTM(prAdapter, prSwRfb, prActFrame,
				prFtmInfoElem);
			break;

		default:
			break;
		}
	}
}

void rttProcessPublicAction(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb)
{
	struct WLAN_ACTION_FRAME *prActFrame = NULL;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prActFrame = (struct WLAN_ACTION_FRAME *) prSwRfb->pvHeader;

	switch (prActFrame->ucAction) {
	case ACTION_PUBLIC_FINE_TIMING_MEASUREMENT_REQUEST:
		if (rttIsRSTAActive(prAdapter)) {
			DBGLOG(RTT, DEBUG, "Receive IFTMR\n");
			rttProcessFTMR(prAdapter, prSwRfb);
		}
		break;
	default:
		break;
	}
}
#endif /* CFG_SUPPORT_RTT_RSTA  */

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

