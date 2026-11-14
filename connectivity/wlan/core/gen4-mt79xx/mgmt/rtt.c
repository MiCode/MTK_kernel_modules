// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
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

#if (CFG_SUPPORT_RTT == 1)
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define RTT_DEBUG 0

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
static struct RTT_BIAS_TABLE *rttGetBias(struct ADAPTER *prAdapter);

static void rttRequestDoneTimeOut(struct ADAPTER *prAdapter,
			unsigned long ulParam);
static void rttContRequestTimeOut(struct ADAPTER *prAdapter,
			unsigned long ulParam);
static void rttFreeAllResults(struct RTT_INFO *prRttInfo);
static void rttFreeAllClients(struct RTT_INFO *prRttInfo);
static void rttUpdateStatus(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex, struct CMD_RTT_REQUEST *prCmd);
static void rttActiveNetwork(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex, uint8_t active);
static uint32_t rttRemoveStaRec(struct ADAPTER *prAdapter);
static uint32_t rttAddPeerStaRec(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex, struct BSS_DESC *prBssDesc);
static uint32_t rttRemovePeerStaRec(struct ADAPTER *prAdapter);

static uint32_t rttInitBiasTable(struct ADAPTER *prAdapter);

#if (CFG_SUPPORT_RTT_RSTA == 1)
static u_int8_t rttGetRSTABssIndex(struct ADAPTER *prAdapter,
			uint8_t *pucDestAddr);
static u_int8_t rttIsRSTAActive(struct ADAPTER *prAdapter);
static enum WIFI_CHANNEL_WIDTH
rttFtmChannelWidth(uint8_t ucFormatAndBandwidth);
static enum ENUM_WIFI_RTT_PREAMBLE
rttFtmPreamble(uint8_t ucFormatAndBandwidth);
static uint32_t rttAddClientStaRec(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex, uint8_t *pucClientMacAddr);
static uint32_t rttRemoveClientStaRec(struct ADAPTER *prAdapter);
#endif /* CFG_SUPPORT_RTT_RSTA */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#if (RTT_DEBUG == 1)
/* Dump info For RTT Bias debug */
void dumpRTTCalInfo(struct RTT_CAL_INFO *info)
{
	int i = 0;

	DBGLOG(RTT, INFO, "RTT_CAL_INFO:\n");
	DBGLOG(RTT, INFO, "  Version : %u\n", info->ucVersion);
	DBGLOG(RTT, INFO, "  Scale   : %u\n", info->ucScale);
	DBGLOG(RTT, INFO, "  WF0 Enable: %u\n", info->ucWF0Enable);
	DBGLOG(RTT, INFO, "  WF1 Enable: %u\n", info->ucWF1Enable);
	DBGLOG(RTT, INFO, "  WF2 Enable: %u\n", info->ucWF2Enable);
	DBGLOG(RTT, INFO, "  WF3 Enable: %u\n", info->ucWF3Enable);
	DBGLOG(RTT, INFO, "  Reserved Bits : %u\n", info->ucWFxReserved);

	DBGLOG(RTT, INFO, "  Reserved Bytes: ");
	for (i = 0; i < sizeof(info->ucReserved); ++i)
		DBGLOG(RTT, INFO, "%02X ", info->ucReserved[i]);
	DBGLOG(RTT, INFO, "\n");
}

void dumpRTTBiasTable2G(struct RTT_BIAS_TABLE_2G *table)
{
	DBGLOG(RTT, INFO, "  2G Bias Table:\n");
	DBGLOG(RTT, INFO, "    CBW20_DBW20: %u\n", table->u4CBW20DBW20);
	DBGLOG(RTT, INFO, "    CBW40_DBW20: %u\n", table->u4CBW40DBW20);
	DBGLOG(RTT, INFO, "    CBW40_DBW40: %u\n", table->u4CBW40DBW40);
}

void dumpRTTBiasTable56G(struct RTT_BIAS_TABLE_56G *table)
{
	DBGLOG(RTT, INFO, "  56G Bias Table:\n");
	DBGLOG(RTT, INFO, "    CBW20_DBW20: %u\n", table->u4CBW20DBW20);
	DBGLOG(RTT, INFO, "    CBW40_DBW20: %u\n", table->u4CBW40DBW20);
	DBGLOG(RTT, INFO, "    CBW40_DBW40: %u\n", table->u4CBW40DBW40);
	DBGLOG(RTT, INFO, "    CBW80_DBW20: %u\n", table->u4CBW80DBW20);
	DBGLOG(RTT, INFO, "    CBW80_DBW40: %u\n", table->u4CBW80DBW40);
	DBGLOG(RTT, INFO, "    CBW80_DBW80: %u\n", table->u4CBW80DBW80);
	DBGLOG(RTT, INFO, "    CBW160_DBW20: %u\n", table->u4CBW160DBW20);
	DBGLOG(RTT, INFO, "    CBW160_DBW40: %u\n", table->u4CBW160DBW40);
	DBGLOG(RTT, INFO, "    CBW160_DBW80: %u\n", table->u4CBW160DBW80);
	DBGLOG(RTT, INFO, "    CBW160_DBW160: %u\n", table->u4CBW160DBW160);
}
#endif /* RTT_DEBUG */

static struct RTT_BIAS_TABLE *rttGetBias(struct ADAPTER *prAdapter)
{
	if (prAdapter)
		return &(prAdapter->rWifiVar.rRttBiasInfo);
	else
		return NULL;
}

static void rttRequestDoneTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return;
	}
	/* Stop RTT since timeout */
	if (rttInfo->ucState == RTT_STATE_START)
		rttEventDone(prAdapter, NULL);
}

static void rttContRequestTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return;
	}

	/* No continuous RTT requests */
	rttInfo->fgIsContRunning = FALSE;
}

static void rttFreeAllResults(struct RTT_INFO *prRttInfo)
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

static void rttFreeAllClients(struct RTT_INFO *prRttInfo)
{
	struct RTT_CLIENT_ENTRY *entry;

	while (!LINK_IS_EMPTY(&prRttInfo->rClientList)) {
		LINK_REMOVE_HEAD(&prRttInfo->rClientList,
			entry, struct RTT_CLIENT_ENTRY*);
		kalMemFree(entry, VIR_MEM_TYPE,
			sizeof(struct RTT_CLIENT_ENTRY));
	}
}


static void rttUpdateStatus(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct CMD_RTT_REQUEST *prCmd)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return;
	}

	rttInfo->ucBssIndex = ucBssIndex;

	if (prCmd && prCmd->fgEnable) { /* Request Start RTT */
		rttInfo->eRttPeerType = prCmd->arRttConfigs[0].ePeer;
		rttInfo->ucSeqNum = prCmd->ucSeqNum;
		rttInfo->fgIsRunning = true;
		rttInfo->fgIsContRunning = true;
		rttInfo->ucState = RTT_STATE_START;
		cnmTimerStartTimer(prAdapter,
			&rttInfo->rRttDoneTimer,
			SEC_TO_MSEC(RTT_REQUEST_DONE_TIMEOUT_SEC));
		cnmTimerStartTimer(prAdapter,
			&rttInfo->rRttContTimer,
			SEC_TO_MSEC(RTT_REQUEST_CONT_TIMEOUT_SEC));
	} else {
		/* Stop RTT: Cancel Request, Event Done or Timeout */
		rttInfo->fgIsRunning = false;
		rttInfo->ucState = RTT_STATE_IDLE;
		cnmTimerStopTimer(prAdapter, &rttInfo->rRttDoneTimer);
		rttRemoveStaRec(prAdapter);
		rttFreeAllResults(rttInfo);
		rttActiveNetwork(prAdapter, ucBssIndex, false);
	}
}

static void rttActiveNetwork(struct ADAPTER *prAdapter,
			    uint8_t ucBssIndex, uint8_t active)
{
	struct BSS_INFO *prBssInfo;
	struct SCAN_INFO *prScanInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!IS_BSS_AIS(prBssInfo))
		return;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	if (!prScanInfo || !prAisFsmInfo) {
		DBGLOG(RTT, ERROR, "prScanInfo or prAisFsmInfo is NULL\n");
		return;
	}

	if (active) {
		DBGLOG(RTT, INFO, "RTT activate network\n");
		SET_NET_ACTIVE(prAdapter, ucBssIndex);
		nicActivateNetwork(prAdapter, ucBssIndex);
	} else if (
		prBssInfo->eConnectionState == MEDIA_STATE_DISCONNECTED &&
		prScanInfo->eCurrentState == SCAN_STATE_IDLE &&
		prAisFsmInfo->eCurrentState == AIS_STATE_IDLE  &&
		IS_NET_ACTIVE(prAdapter, ucBssIndex)) {
		DBGLOG(RTT, INFO, "RTT deactivate network\n");
		UNSET_NET_ACTIVE(prAdapter, ucBssIndex);
		nicDeactivateNetwork(prAdapter, ucBssIndex);
	}
}

static uint32_t rttRemoveStaRec(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo)
		return WLAN_STATUS_FAILURE;

	switch (rttInfo->eRttPeerType) {
	case RTT_PEER_AP:
		rttRemovePeerStaRec(prAdapter);
		break;
#if (CFG_SUPPORT_RTT_RSTA == 1)
	case RTT_PEER_STA:
		rttRemoveClientStaRec(prAdapter);
		break;
#endif /* CFG_SUPPORT_RTT_RSTA */
	default:
		DBGLOG(RTT, WARN, "invalid RttPeerType=%u\n",
			rttInfo->eRttPeerType);
		break;
	}

	return WLAN_STATUS_SUCCESS;
}

/* For RTT_PEER_AP, add StaRec if peer is un-assoicated */
static uint32_t rttAddPeerStaRec(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex,
			struct BSS_DESC *prBssDesc)
{
	struct STA_RECORD *prStaRec, *prStaRecOfAp;
	struct BSS_INFO *prBssInfo;

	prStaRec = cnmGetStaRecByAddress(prAdapter,
				ucBssIndex,
				prBssDesc->aucSrcAddr);

	/* Create Tmp StaRec if RTT with un-associated AP */
	if (prStaRec == NULL) {
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
		if (prStaRec == NULL) {
			DBGLOG(RTT, ERROR, "Create StaRec fail\n");
			return WLAN_STATUS_RESOURCES;
		}

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
		/* Update StaRec to State1 */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);
	}

	if (prStaRec) {
		DBGLOG(RTT, INFO,
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

/* For RTT_PEER_AP, remove un-assoicated peer StaRec */
static uint32_t rttRemovePeerStaRec(struct ADAPTER *prAdapter)
{
	struct RTT_RESULT_ENTRY *entry;
	struct STA_RECORD *prStaRec, *prStaRecOfAp;
	struct BSS_INFO *prBssInfo;
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL!\n");
		return WLAN_STATUS_FAILURE;
	}

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
					prStaRecOfAp->aucMacAddr)) {
				/* assoicated AP, do not free it */
				continue;
			} else {
				/* Free StaRec for un-assoicated AP */
				DBGLOG(RTT, INFO,
				"Free StaRec for un-assoic AP " MACSTR "\n",
				MAC2STR(entry->rResult.aucMacAddr));

				cnmStaRecFree(prAdapter, prStaRec);
			}
		}
	}

	return WLAN_STATUS_SUCCESS;
}

static uint32_t rttInitBiasTable(struct ADAPTER *prAdapter)
{
	struct RTT_BIAS_TABLE *rttBiasTable = rttGetBias(prAdapter);
	uint8_t *pucConfigBuf;
	uint32_t u4ConfigReadLen;
	uint32_t u4TableSize;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
#if (RTT_DEBUG == 1)
	int i = 0;
#endif /* RTT_DEBUG */

	u4TableSize = sizeof(struct RTT_BIAS_TABLE);
	if (!rttBiasTable) {
		DBGLOG(RTT, ERROR, "rttBiasTable is NULL");
		return WLAN_STATUS_FAILURE;
	}
	pucConfigBuf = (uint8_t *) kalMemAlloc(
			u4TableSize, VIR_MEM_TYPE);
	u4ConfigReadLen = 0;
	/* Initialize the RTT Bias Table to 0 */
	kalMemZero(rttBiasTable, sizeof(struct RTT_BIAS_TABLE));
	if (!pucConfigBuf) {
		DBGLOG(RTT, ERROR, "pucConfigBuf is NULL");
		return WLAN_STATUS_FAILURE;
	}

	kalMemZero(pucConfigBuf, u4TableSize);

	DBGLOG(RTT, INFO,
		"Read RTT Bias value file: %s\n", RTT_BIAS_TABLE_FILE);
	if (kalRequestFirmware(RTT_BIAS_TABLE_FILE, pucConfigBuf,
		   u4TableSize, &u4ConfigReadLen,
		   prAdapter->prGlueInfo->prDev) == 0) {
		/* Nothing */
	} else if (kalReadToFile("/data/misc/wifi/" RTT_BIAS_TABLE_FILE,
		   pucConfigBuf, u4TableSize,
		   &u4ConfigReadLen) == 0) {
		/* Nothing */
	} else if (kalReadToFile("/storage/sdcard0/" RTT_BIAS_TABLE_FILE,
		   pucConfigBuf, u4TableSize,
		   &u4ConfigReadLen) == 0) {
		/* Nothing */
	}

	if (u4ConfigReadLen != u4TableSize) {
		DBGLOG(RTT, WARN, "Read length mismatch: expected %u, got %u\n",
			u4TableSize, u4ConfigReadLen);
		u4Status = WLAN_STATUS_FAILURE;
	} else if (pucConfigBuf[0] != '\0' && u4ConfigReadLen > 0) {
		/* Parse RTT Bias Data */
		kalMemCopy(rttBiasTable, pucConfigBuf, u4TableSize);
	} else {
		DBGLOG(RTT, ERROR, "RTT Bias table file is empty");
		u4Status = WLAN_STATUS_FAILURE;
	}

	kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4TableSize);

#if (RTT_DEBUG == 1)
	/* Dump for Bias value debug */
	DBGLOG(RTT, INFO, "Dump RTT Bias Table\n");
	dumpMemory32((uint32_t *)rttBiasTable, u4TableSize);
	dumpRTTCalInfo(&rttBiasTable->rttCalInfo);
	for (i = 0; i < 14; i++) {
		DBGLOG(RTT, INFO, "2G CH%d\n", i+1);
		dumpRTTBiasTable2G(&rttBiasTable->rttBias2G[i]);
	}
	for (i = 0; i < 3; i++) {
		DBGLOG(RTT, INFO, "5G Group%d\n", i);
		dumpRTTBiasTable56G(&rttBiasTable->rttBias5G[i]);
	}
	DBGLOG(RTT, INFO, "6G Group\n");
	dumpRTTBiasTable56G(&rttBiasTable->rttBias6G);
#endif /* RTT_DEBUG */

	return u4Status;
}

#if (CFG_SUPPORT_RTT_RSTA == 1)
static u_int8_t rttGetRSTABssIndex(struct ADAPTER *prAdapter,
					uint8_t *pucDestAddr)
{
	uint8_t ucBssIndex = 0;
	struct BSS_INFO *prBssInfo = NULL;

	if (!prAdapter)
		return FALSE;

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucHwBssIdNum;
		ucBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
		if (prBssInfo &&
			IS_BSS_AP(prAdapter, prBssInfo) &&
			IS_BSS_ACTIVE(prBssInfo) &&
			EQUAL_MAC_ADDR(pucDestAddr, prBssInfo->aucOwnMacAddr)) {
			break;
		}
	}

	return ucBssIndex;
}

static u_int8_t rttIsRSTAActive(struct ADAPTER *prAdapter)
{
	uint8_t ucBssIndex = 0;
	uint8_t fgIsAPActive = FALSE;
	struct BSS_INFO *prBssInfo = NULL;

	if (!prAdapter)
		return FALSE;

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucHwBssIdNum;
		ucBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
		if (prBssInfo &&
			IS_BSS_AP(prAdapter, prBssInfo) &&
			IS_BSS_ACTIVE(prBssInfo)) {
			fgIsAPActive = TRUE;
			break;
		}
	}

	return fgIsAPActive;
}

static enum WIFI_CHANNEL_WIDTH
rttFtmChannelWidth(uint8_t ucFormatAndBandwidth)
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

static enum ENUM_WIFI_RTT_PREAMBLE
rttFtmPreamble(uint8_t ucFormatAndBandwidth)
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

/* For RTT_PEER_STA, add StaRec if peer is un-assoicated */
static uint32_t rttAddClientStaRec(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex, uint8_t *pucClientMacAddr)
{
	struct STA_RECORD *prStaRec;
	enum ENUM_STA_TYPE eStaType = STA_TYPE_LEGACY_CLIENT;
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct RTT_CLIENT_ENTRY *entry;
	uint32_t sz = sizeof(struct RTT_CLIENT_ENTRY);

	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL!\n");
		return WLAN_STATUS_FAILURE;
	}

	prStaRec = cnmGetStaRecByAddress(prAdapter,
			ucBssIndex,
			pucClientMacAddr);

	if (prStaRec == NULL) { /* RTT with new client */
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
		DBGLOG(RTT, INFO,
			"RTT w/ %s client " MACSTR "\n",
			prStaRec->ucStaState == STA_STATE_1 ?
			"un-associated" : "associated",
			MAC2STR(pucClientMacAddr));
	}

	return WLAN_STATUS_SUCCESS;
}

/* For RTT_PEER_STA, remove un-assoicated peer StaRec */
static uint32_t rttRemoveClientStaRec(struct ADAPTER *prAdapter)
{
	struct RTT_CLIENT_ENTRY *entry;
	struct RTT_CLIENT_ENTRY *entryNext;
	struct STA_RECORD *prStaRec;
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL!\n");
		return WLAN_STATUS_FAILURE;
	}

	LINK_FOR_EACH_ENTRY_SAFE(entry, entryNext, &rttInfo->rClientList,
		rLinkEntry, struct RTT_CLIENT_ENTRY) {

		if (!entry)
			continue;

		prStaRec = cnmGetStaRecByAddress(prAdapter,
				rttInfo->ucBssIndex,
				entry->aucMacAddr);

		if (prStaRec && prStaRec->ucStaState == STA_STATE_1 &&
			entry->ucSeqNum == rttInfo->ucSeqNum) {
			/* Free StaRec for un-associated Client */
			DBGLOG(RTT, INFO,
				"Free StaRec for un-assoc client " MACSTR "\n",
				MAC2STR(entry->aucMacAddr));

			cnmStaRecFree(prAdapter, prStaRec);
			/* Remove entry from client list */
			LINK_REMOVE_KNOWN_ENTRY(&rttInfo->rClientList,
				&entry->rLinkEntry);
			kalMemFree(entry, VIR_MEM_TYPE,
				sizeof(struct RTT_CLIENT_ENTRY));
		} else {
			DBGLOG(RTT, WARN,
				"Unknown StaRec " MACSTR ", Seq(%u, %u)\n",
				MAC2STR(entry->aucMacAddr),
				entry->ucSeqNum, rttInfo->ucSeqNum);
		}
	}

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_RTT_RSTA */

struct RTT_INFO *rttGetInfo(struct ADAPTER *prAdapter)
{
	if (prAdapter)
		return &(prAdapter->rWifiVar.rRttInfo);
	else
		return NULL;
}

/* Initialize RTT_INFO */
void rttInit(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return;
	}

	rttInfo->ucBssIndex = AIS_DEFAULT_INDEX;
	rttInfo->fgIsRunning = false;
	rttInfo->fgIsContRunning = false;
	rttInfo->ucSeqNum = 0;
	rttInfo->ucState = RTT_STATE_IDLE;
	rttInfo->eRttPeerType = RTT_PEER_AP;

	cnmTimerInitTimer(prAdapter,
		  &rttInfo->rRttDoneTimer,
		  (PFN_MGMT_TIMEOUT_FUNC) rttRequestDoneTimeOut,
		  (uintptr_t)NULL, TIMER_WAKELOCK_AUTO);
	cnmTimerInitTimer(prAdapter,
		  &rttInfo->rRttContTimer,
		  (PFN_MGMT_TIMEOUT_FUNC) rttContRequestTimeOut,
		  (uintptr_t)NULL, TIMER_WAKELOCK_AUTO);

	LINK_INITIALIZE(&rttInfo->rResultList);
	LINK_INITIALIZE(&rttInfo->rClientList);
	if (rttInitBiasTable(prAdapter) != WLAN_STATUS_SUCCESS)
		DBGLOG(RTT, ERROR, "Init Bias Table Fail\n");
}

void rttUninit(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return;
	}

	rttFreeAllResults(rttInfo);
	rttFreeAllClients(rttInfo);
	rttUpdateStatus(prAdapter, rttInfo->ucBssIndex, NULL);
}

uint8_t rttIsRunning(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return FALSE;
	}
	if (rttInfo->fgIsContRunning)
		DBGLOG(RTT, INFO, "RTT is Running\n");

	return rttInfo->fgIsContRunning;
}

/* Transfer BW type */
uint8_t rttBwToBssBw(uint8_t ucRttBw)
{
	uint8_t ucBssBw = WIFI_CHAN_WIDTH_20;

	switch (ucRttBw) {
	case WIFI_RTT_BW_5:
	case WIFI_RTT_BW_10:
	case WIFI_RTT_BW_20:
		ucBssBw = WIFI_CHAN_WIDTH_20;
		break;
	case WIFI_RTT_BW_40:
		ucBssBw = WIFI_CHAN_WIDTH_40;
		break;
	case WIFI_RTT_BW_80:
		ucBssBw = WIFI_CHAN_WIDTH_80;
		break;
	case WIFI_RTT_BW_160:
		ucBssBw = WIFI_CHAN_WIDTH_160;
		break;
	default:
		ucBssBw = WIFI_CHAN_WIDTH_20;
		break;
	}

	return ucBssBw;
}

uint8_t rttBssBwToRttBw(uint8_t ucBssBw)
{
	uint8_t ucRttBw = WIFI_RTT_BW_20;

	switch (ucBssBw) {
	case WIFI_CHAN_WIDTH_20:
		ucRttBw = WIFI_RTT_BW_20;
		break;
	case WIFI_CHAN_WIDTH_40:
		ucRttBw = WIFI_RTT_BW_40;
		break;
	case WIFI_CHAN_WIDTH_80:
		ucRttBw = WIFI_RTT_BW_80;
		break;
	case WIFI_CHAN_WIDTH_160:
		ucRttBw = WIFI_RTT_BW_160;
		break;
	default:
		ucRttBw = WIFI_RTT_BW_20;
		break;
	}

	return ucRttBw;
}

uint8_t rttMaxBwToRttBw(uint8_t ucMaxBw)
{
	uint8_t ucRttBw = WIFI_RTT_BW_20;

	switch (ucMaxBw) {
	case MAX_BW_20MHZ:
		ucRttBw = WIFI_RTT_BW_20;
		break;
	case MAX_BW_40MHZ:
		ucRttBw = WIFI_RTT_BW_40;
		break;
	case MAX_BW_80MHZ:
		ucRttBw = WIFI_RTT_BW_80;
		break;
	case MAX_BW_160MHZ:
		ucRttBw = WIFI_RTT_BW_160;
		break;
	default:
		ucRttBw = WIFI_RTT_BW_20;
		break;
	}

	return ucRttBw;
}

uint8_t rttRttBwToMaxBw(uint8_t ucRttBw)
{
	uint8_t ucMaxBw = MAX_BW_20MHZ;

	switch (ucRttBw) {
	case WIFI_RTT_BW_5:
	case WIFI_RTT_BW_10:
	case WIFI_RTT_BW_20:
		ucMaxBw = MAX_BW_20MHZ;
		break;
	case WIFI_RTT_BW_40:
		ucMaxBw = MAX_BW_40MHZ;
		break;
	case WIFI_RTT_BW_80:
		ucMaxBw = MAX_BW_80MHZ;
		break;
	case WIFI_RTT_BW_160:
		ucMaxBw = MAX_BW_160MHZ;
		break;
	default:
		ucMaxBw = MAX_BW_20MHZ;
		break;
	}

	return ucMaxBw;
}

uint8_t rttReviseBw(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex,
			struct BSS_DESC *prBssDesc,
			uint8_t ucRttBwIn)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucMaxBssBw = MAX_BW_20MHZ;
	uint8_t ucRttMaxBw = WIFI_RTT_BW_20;
	uint8_t ucRttBwOut = ucRttBwIn;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prBssInfo && prBssDesc) {
		ucMaxBssBw = cnmGetBssBandBw(prAdapter,
			prBssInfo, prBssDesc->eBand);
		ucRttMaxBw = rttMaxBwToRttBw(ucMaxBssBw);

		if (ucRttBwIn > ucRttMaxBw) {
			ucRttBwOut = ucRttMaxBw;
			DBGLOG(RTT, INFO,
				"Convert RTT BW from %d to %d\n",
				ucRttBwIn, ucRttBwOut);
		}
	}

	return ucRttBwOut;
}

/* Handle RTT Request */
uint32_t rttSendCmd(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct CMD_RTT_REQUEST *cmd)
{
	uint32_t status;
	uint32_t u4CmdBufLen = 0;
	uint8_t i;
	struct CMD_LOC_REQ *prLocReq = NULL;
	struct LOC_CMD_RANGE_REQUEST_MC_ISTA *prRttReqInfo = NULL;

	if (!cmd) {
		DBGLOG(RTT, ERROR, "CMD is NULL\n");
		return WLAN_STATUS_FAILURE;
	}
	u4CmdBufLen = sizeof(struct CMD_LOC_REQ);
	if (cmd->arRttConfigs[0].eType == RTT_TYPE_2_SIDED_11MC) {
		if (cmd->arRttConfigs[0].ePeer == RTT_PEER_AP) {
			u4CmdBufLen +=
				sizeof(struct LOC_CMD_RANGE_REQUEST_MC_ISTA);
#if (CFG_SUPPORT_RTT_RSTA == 1)
		} else if (cmd->arRttConfigs[0].ePeer == RTT_PEER_STA) {
			u4CmdBufLen +=
				sizeof(struct LOC_CMD_RANGE_REQUEST_MC_RSTA);
#endif /* CFG_SUPPORT_RTT_RSTA */
		} else {
			DBGLOG(RTT, ERROR, "Not support P2P or NAN Ranging.\n");
			return WLAN_STATUS_INVALID_DATA;
		}
	} else {
		DBGLOG(RTT, ERROR, "Not support AZ or One Sized Ranging.\n");
		return WLAN_STATUS_FAILURE;
	}

	prLocReq = (struct CMD_LOC_REQ *)
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufLen);
	if (prLocReq == NULL) {
		DBGLOG(RTT, ERROR, "Request prLocReq Mem fail.\n");
		return WLAN_STATUS_FAILURE;
	}
	prRttReqInfo =
		(struct LOC_CMD_RANGE_REQUEST_MC_ISTA *) prLocReq->aucBody;

	if (cmd->arRttConfigs[0].eType == RTT_TYPE_2_SIDED_11MC) {
		if (cmd->arRttConfigs[0].ePeer == RTT_PEER_AP)
			prLocReq->ucSubCmdId = CMD_LOC_RANGE_MC_ISTA;
		else if (cmd->arRttConfigs[0].ePeer == RTT_PEER_STA)
			prLocReq->ucSubCmdId = CMD_LOC_RANGE_MC_RSTA;
	}
	prRttReqInfo->ucSeqNum = cmd->ucSeqNum;
	prRttReqInfo->fgEnable = cmd->fgEnable;
	prRttReqInfo->ucConfigNum = cmd->ucConfigNum;

	for (i = 0; i < cmd->ucConfigNum; i++) {
		kalMemCopy(&prRttReqInfo->arRttConfigs[i],
			&cmd->arRttConfigs[i],
			sizeof(struct RTT_CONFIG));
	}

	status = wlanSendSetQueryCmd(prAdapter,
			CMD_ID_LOCATION,
			TRUE,
			FALSE,
			FALSE,
			nicCmdEventSetCommon,
			nicOidCmdTimeoutCommon,
			u4CmdBufLen,
			(uint8_t *) prLocReq, NULL, 0);

	cnmMemFree(prAdapter, prLocReq);

	if (status != WLAN_STATUS_FAILURE) {
		/* send cmd to FW successfully, update status */
		rttUpdateStatus(prAdapter, ucBssIndex, cmd);
		return WLAN_STATUS_SUCCESS;
	}

	return WLAN_STATUS_FAILURE;
}

uint32_t rttStartRttRequest(struct ADAPTER *prAdapter,
			 struct PARAM_RTT_REQUEST *prRequest,
			 uint8_t ucBssIndex)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct RTT_BIAS_TABLE *rttBiasTable = rttGetBias(prAdapter);
	struct BSS_DESC *bss = NULL;
	struct CMD_RTT_REQUEST *cmd;
	uint8_t i, active;
	uint8_t ucMaxBw;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint32_t sz = sizeof(struct CMD_RTT_REQUEST);
	enum ENUM_BAND eBand;
	enum ENUM_CHNL_EXT eSco;
	struct SCAN_INFO *prScanInfo;
	uint8_t ucChannelGroup;
	struct RTT_BIAS_TABLE_2G *table_2G;
	struct RTT_BIAS_TABLE_56G *table_56G;

	cmd = (struct CMD_RTT_REQUEST *)
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sz);
	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return WLAN_STATUS_FAILURE;
	}
	if (!cmd)
		return WLAN_STATUS_RESOURCES;

	/* Workaround: Abort scan before doing RTT */
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (prScanInfo->eCurrentState == SCAN_STATE_SCANNING) {
		struct AIS_FSM_INFO *prAisFsmInfo =
			aisGetAisFsmInfo(prAdapter, ucBssIndex);

		prAisFsmInfo->fgIsScanOidAborted = TRUE;
		aisFsmStateAbort_SCAN(prAdapter, ucBssIndex);
		DBGLOG(RTT, INFO, "Abort Scan due to RTT is requested\n");
	}

#if (CFG_SUPPORT_SCHED_SCAN == 1)
	/* check if there is any pending sched_scan not yet finished */
	if ((prScanInfo->fgSchedScanning) &&
		(prAdapter->prGlueInfo->prSchedScanRequest != NULL)) {
		uint32_t rStatus, u4BufLen;

		rStatus = kalIoctl(prAdapter->prGlueInfo,
				wlanoidSetStopSchedScan, NULL, 0,
				FALSE, TRUE, TRUE, &u4BufLen);
		if (rStatus != WLAN_STATUS_FAILURE) {
			kalSchedScanStopped(prAdapter->prGlueInfo, TRUE);
			DBGLOG(RTT, INFO,
				"Abort SchedScan due to RTT is requested\n");
		}
	}
#endif /* CFG_SUPPORT_SCHED_SCAN */

	active = IS_NET_ACTIVE(prAdapter, ucBssIndex);
	if (!active)
		rttActiveNetwork(prAdapter, ucBssIndex, true);

	/* Create RTT Request to start ranging */
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
#if (CFG_SUPPORT_RTT_RSTA == 1)
		} else if (rc->ePeer == RTT_PEER_STA) {
			rttAddClientStaRec(prAdapter, ucBssIndex, rc->aucAddr);
#endif /* CFG_SUPPORT_RTT_RSTA */
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
		tc->ucNumRetriesPerRttFrame = rc->ucNumRetriesPerRttFrame;
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
		tc->eChannelWidth = cnmChannelWidthToCnmChBw(
			tc->rChannel.width);
		ucMaxBw = rttRttBwToMaxBw(rc->eBw);
		eSco = nicGetSco(prAdapter, tc->eBand, tc->ucPrimaryChannel);
		tc->ucS1 = nicGetS1(tc->eBand,
			tc->ucPrimaryChannel, eSco, ucMaxBw);
		tc->ucS2 = nicGetS2(tc->eBand, tc->ucPrimaryChannel,
			rlmGetBssOpBwByChannelWidth(eSco, tc->eChannelWidth));
		tc->ucBssIndex = ucBssIndex;
		tc->eEventType = rc->eEventType;
		tc->ucASAP = rc->ucASAP;
		tc->ucFtmMinDeltaTime = rc->ucFtmMinDeltaTime;
		tc->u4BiasPsValueBW20 = 0;
		tc->u4BiasPsValueBW40 = 0;
		tc->u4BiasPsValueBW80 = 0;
		tc->u4BiasPsValueBW160 = 0;

		if (tc->eBand == BAND_2G4) {
			table_2G = &rttBiasTable->
				rttBias2G[tc->ucPrimaryChannel-1];
			if (tc->ucPrimaryChannel >= 1
				&& tc->ucPrimaryChannel <= 14) {
				if (tc->eBw == WIFI_RTT_BW_20) {
					tc->u4BiasPsValueBW20 =
						table_2G->u4CBW20DBW20;
				} else if (tc->eBw == WIFI_RTT_BW_40) {
					tc->u4BiasPsValueBW20 =
						table_2G->u4CBW40DBW20;
					tc->u4BiasPsValueBW40 =
						table_2G->u4CBW40DBW40;
				}
			} else {
				DBGLOG(RTT, INFO,
		"Request CH%d BW%u, u4BiasPsValue BW20/40/80/160 = %u/%u/%u/%u",
		tc->ucPrimaryChannel, tc->eBw,
		tc->u4BiasPsValueBW20, tc->u4BiasPsValueBW40,
		tc->u4BiasPsValueBW80, tc->u4BiasPsValueBW160);
			}
		} else if (tc->eBand == BAND_5G) {
			if (tc->ucPrimaryChannel >= 36
				&& tc->ucPrimaryChannel < 100)
				ucChannelGroup = RTT_BIAS_5G_GROUP1;
			else if (tc->ucPrimaryChannel >= 100
				&& tc->ucPrimaryChannel < 149)
				ucChannelGroup = RTT_BIAS_5G_GROUP2;
			else
				ucChannelGroup = RTT_BIAS_5G_GROUP3;
			table_56G = &rttBiasTable->
				rttBias5G[ucChannelGroup];
			if (tc->eBw == WIFI_RTT_BW_20) {
				tc->u4BiasPsValueBW20 =
					table_56G->u4CBW20DBW20;
			} else if (tc->eBw == WIFI_RTT_BW_40) {
				tc->u4BiasPsValueBW20 =
					table_56G->u4CBW40DBW20;
				tc->u4BiasPsValueBW40 =
					table_56G->u4CBW40DBW40;
			} else if (tc->eBw == WIFI_RTT_BW_80) {
				tc->u4BiasPsValueBW20 =
					table_56G->u4CBW80DBW20;
				tc->u4BiasPsValueBW40 =
					table_56G->u4CBW80DBW40;
				tc->u4BiasPsValueBW80 =
					table_56G->u4CBW80DBW80;
			} else if (tc->eBw == WIFI_RTT_BW_160) {
				tc->u4BiasPsValueBW20 =
					table_56G->u4CBW160DBW20;
				tc->u4BiasPsValueBW40 =
					table_56G->u4CBW160DBW40;
				tc->u4BiasPsValueBW80 =
					table_56G->u4CBW160DBW80;
				tc->u4BiasPsValueBW160 =
					table_56G->u4CBW160DBW160;
			}
		}
		DBGLOG(RTT, INFO,
		"Request CH%d BW%u, u4BiasPsValue BW20/40/80/160 = %u/%u/%u/%u",
		tc->ucPrimaryChannel, tc->eBw,
		tc->u4BiasPsValueBW20, tc->u4BiasPsValueBW40,
		tc->u4BiasPsValueBW80, tc->u4BiasPsValueBW160);
	}
	status = rttSendCmd(prAdapter, ucBssIndex, cmd);

fail:
	if (status != WLAN_STATUS_SUCCESS) {
		DBGLOG(RTT, ERROR,
		"Fail to send cmd, restore active network");
		if (!active)
			rttActiveNetwork(prAdapter, ucBssIndex, false);
		/* RTT TODO: status to Idle if send cmd fail? */
	}

	cnmMemFree(prAdapter, (void *) cmd);
	DBGLOG(RTT, INFO,
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
	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return WLAN_STATUS_FAILURE;
	}
	if (!cmd)
		return WLAN_STATUS_RESOURCES;

	cmd->ucSeqNum = rttInfo->ucSeqNum;
	cmd->fgEnable = false;
	status = rttSendCmd(prAdapter, rttInfo->ucBssIndex, cmd);
	cnmMemFree(prAdapter, (void *) cmd);
	DBGLOG(RTT, INFO, "RTT Cancel status=%d, seq=%d",
				status, rttInfo->ucSeqNum);

	return status;
}

uint32_t rttHandleRttRequest(struct ADAPTER *prAdapter,
			 struct PARAM_RTT_REQUEST *prRequest,
			 uint8_t ucBssIndex)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	uint32_t status;

	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return WLAN_STATUS_FAILURE;
	}
	if (!prRequest) {
		DBGLOG(RTT, ERROR, "prRequest is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

#if (CFG_SUPPORT_CSI == 1)
	if (prRequest->fgEnable && wlanCSIIsRunning(prAdapter)) {
		DBGLOG(RTT, WARN,
		       "CSI is running, can not start RTT\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}
#endif /* CFG_SUPPORT_CSI */

	if ((prRequest->ucConfigNum > RTT_MAX_CANDIDATES) ||
	    (prRequest->fgEnable && prRequest->ucConfigNum == 0) ||
	    (prRequest->fgEnable && rttInfo->fgIsRunning) ||
	    (!prRequest->fgEnable && !rttInfo->fgIsRunning)) {
		DBGLOG(RTT, ERROR,
		       "RTT Request Enable:%d, configNum:%d, Status Running:%d, Request not accepted\n",
		       prRequest->fgEnable, prRequest->ucConfigNum,
		       rttInfo->fgIsRunning);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (prRequest->fgEnable)
		/* Start RTT */
		status = rttStartRttRequest(prAdapter, prRequest, ucBssIndex);
	else
		/* Stop RTT */
		status = rttCancelRttRequest(prAdapter, prRequest);
	return status;
}

/* Handle RTT Event */
void rttEventGetCapabilities(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen = 0;
	struct EVENT_LOC_RESP *prLocResp = NULL;
	struct LOC_EVENT_CAPABILITIES *prLocCapa = NULL;
	struct RTT_CAPABILITIES *prCapaBuf =
		(struct RTT_CAPABILITIES *)prCmdInfo->pvInformationBuffer;

	prLocResp = (struct EVENT_LOC_RESP *)pucEventBuf;
	if (!prLocResp) {
		DBGLOG(RTT, ERROR, "prLocResp error!\n");
		return;
	}
	if (prLocResp->ucSubEventId != EVENT_LOC_CAPA) {
		DBGLOG(RTT, ERROR, "RTT EVENT_LOC_CAPA ERROR\n");
		return;
	}

	DBGLOG(RTT, INFO, "Handle RTT EVENT_LOC_CAPA Event\n");

	prLocCapa = (struct LOC_EVENT_CAPABILITIES *) prLocResp->aucBody;

	if (prCmdInfo->fgIsOid) {
		u4QueryInfoLen = sizeof(struct RTT_CAPABILITIES);
		if (prCmdInfo->u2InfoBufLen >= u4QueryInfoLen) {
			prCapaBuf->fgRttOneSidedSupported =
			  prLocCapa->u2LocInitSupported & BIT(0) ? TRUE : FALSE;
			prCapaBuf->fgRttFtmSupported =
			  prLocCapa->u2LocInitSupported & BIT(1) ? TRUE : FALSE;
			prCapaBuf->fgLciSupported = prLocCapa->ucLciSupport;
			prCapaBuf->fgLcrSupported = prLocCapa->ucLcrSupport;
			prCapaBuf->ucPreambleSupport =
			  (uint8_t)(prLocCapa->u2PreambleSupport);
			prCapaBuf->ucBwSupport =
			  (uint8_t)(prLocCapa->u2BwSupport);
			prCapaBuf->fgResponderSupported =
			  prLocCapa->u2LocResSupported & BIT(1) ? TRUE : FALSE;
			prCapaBuf->fgMcVersion = 80;

			DBGLOG(RTT, INFO,
				"FW LOC LocInitSupported=%hhu, LocResSupported=%hhu, lci=%hhu, lcr=%hhu, preamble=%hhu, bw=%hhu, AZbw=%hhu, minDeltatimePerPacket=%llu",
				prLocCapa->u2LocInitSupported,
				prLocCapa->u2LocResSupported,
				prLocCapa->ucLciSupport,
				prLocCapa->ucLcrSupport,
				prLocCapa->u2PreambleSupport,
				prLocCapa->u2BwSupport,
				prLocCapa->u2AzBwSupport,
				prLocCapa->u4MinDeltaTimePerPacket);
		} else {
			DBGLOG(RTT, ERROR,
				"invalid CMD buffer, length=%d",
				prCmdInfo->u2InfoBufLen);
		}

		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
			       prCmdInfo->u2InfoBufLen, WLAN_STATUS_SUCCESS);
	}
}

void rttEventResult(struct ADAPTER *prAdapter,
	struct EVENT_RTT_RESULT *prEvent)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct RTT_RESULT_ENTRY *entry;
	uint32_t sz;

	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return;
	}
	if (!rttInfo->fgIsRunning) {
		DBGLOG(RTT, WARN, "RTT is not running\n");
		return;
	} else if (!prEvent) {
		DBGLOG(RTT, ERROR, "RTT null result\n");
		return;
	}

	sz = sizeof(struct RTT_RESULT_ENTRY) + prEvent->u2IELen;

	DBGLOG(RTT, INFO,
		"RTT result MAC=" MACSTR ", status=%d, range=%d (mm)\n",
		MAC2STR(prEvent->rResult.aucMacAddr),
		prEvent->rResult.eStatus,
		prEvent->rResult.i4DistanceMM);

	/* Add result into Result list */
	entry = kalMemAlloc(sz, VIR_MEM_TYPE);
	if (entry) {
		kalMemCopy(&entry->rResult, &prEvent->rResult,
			sizeof(struct RTT_RESULT));
		entry->u2IELen = prEvent->u2IELen;
		kalMemCopy(entry->aucIE, prEvent->aucIE, prEvent->u2IELen);
		LINK_INSERT_TAIL(&rttInfo->rResultList,
			&entry->rLinkEntry);
	}
}

void rttEventDone(struct ADAPTER *prAdapter,
	struct EVENT_RTT_DONE *prEvent)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	DBGLOG(RTT, INFO, "RTT Event Done, Peer Type: %d\n",
				rttInfo->eRttPeerType);
	if (!rttInfo) {
		DBGLOG(RTT, ERROR, "rttInfo is NULL\n");
		return;
	}

	if (prEvent == NULL) { /* timeout case */
		DBGLOG(RTT, WARN, "rttRequestDoneTimeOut Seq=%u\n",
			rttInfo->ucSeqNum);
		rttCancelRttRequest(prAdapter, NULL);
	} else { /* Normal rtt done */
		DBGLOG(RTT, INFO,
			"Event RTT done seq: FW %u, Driver %u\n",
			prEvent->ucSeqNum, rttInfo->ucSeqNum);
		if (prEvent->ucSeqNum != rttInfo->ucSeqNum) {
			DBGLOG(RTT, WARN,
			"RTT Event FW/Driver Seq mismatch\n");
			return;
		}
	}

	switch (rttInfo->eRttPeerType) {
	case RTT_PEER_AP:
		kalProcessRttReportDone(prAdapter);
		rttInfo->ucState = RTT_STATE_DONE;
		break;
	case RTT_PEER_STA:
	default:
		/* Do nothing */
		break;
	}
	rttUpdateStatus(prAdapter, rttInfo->ucBssIndex, NULL);
}

#if (CFG_SUPPORT_RTT_RSTA == 1)
uint32_t rttProcessFTM(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		struct WLAN_ACTION_FRAME *prActFrame,
		struct FTM_INFO_ELEM *prFtmInfoElem)
{
	struct PARAM_RTT_REQUEST *rttReq = NULL;
	enum WIFI_CHANNEL_WIDTH channelWidth;
	enum ENUM_BAND eBand = 0;
	struct RX_DESC_OPS_T *prRxDescOps;
	uint8_t ucApBssIndex = 0;

	ucApBssIndex = rttGetRSTABssIndex(prAdapter, prActFrame->aucDestAddr);
	if (ucApBssIndex == prAdapter->ucHwBssIdNum) {
		DBGLOG(RTT, ERROR, "Invalid AP BssId.\n");
		return WLAN_STATUS_FAILURE;
	}
	rttReq = kalMemAlloc(sizeof(struct PARAM_RTT_REQUEST), VIR_MEM_TYPE);
	if (!rttReq) {
		DBGLOG(RTT, ERROR, "fail to alloc memory for rttReq.\n");
		return WLAN_STATUS_FAILURE;
	}
	kalMemZero(rttReq, sizeof(struct PARAM_RTT_REQUEST));
	rttReq->fgEnable = true;
	rttReq->ucConfigNum = 1;
	channelWidth = rttFtmChannelWidth(prFtmInfoElem->ucFormatAndBandwidth);

	COPY_MAC_ADDR(rttReq->arRttConfigs[0].aucAddr, prActFrame->aucSrcAddr);
	rttReq->arRttConfigs[0].eType = RTT_TYPE_2_SIDED_11MC;
	rttReq->arRttConfigs[0].ePeer = RTT_PEER_STA;
	rttReq->arRttConfigs[0].rChannel.width = channelWidth;
	prRxDescOps = prAdapter->chip_info->prRxDescOps;
	RX_STATUS_GET(prRxDescOps, eBand, get_rf_band, prSwRfb->prRxStatus);
	rttReq->arRttConfigs[0].rChannel.center_freq =
		nicChannelNum2Freq(prSwRfb->ucChnlNum, eBand) / 1000;
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

	rttHandleRttRequest(prAdapter, rttReq, ucApBssIndex);

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

	u2IELength = prSwRfb->u2PacketLen - (uint16_t)OFFSET_OF(
		struct ACTION_FTM_REQUEST_FRAME, aucInfoElem[0]);
	prRxFrame = (struct ACTION_FTM_REQUEST_FRAME *) prSwRfb->pvHeader;
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
			DBGLOG(RTT, INFO, "Receive IFTMR\n");
			rttProcessFTMR(prAdapter, prSwRfb);
		}
		break;
	default:
		break;
	}
}
#endif /* CFG_SUPPORT_RTT_RSTA */

#endif /* CFG_SUPPORT_RTT */
