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
#include "mddp.h"
#include "gl_kal.h"
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define AIS_JOIN_TIMEOUT                    7

#if (CFG_SUPPORT_HE_ER == 1)
#define AP_TX_POWER		  20
#endif

#define AIS_MAIN_LINK_INDEX (0)

/* Support driver triggers roaming */
#if (CFG_EXT_ROAMING == 1)
#define RCPI_DIFF_DRIVER_ROAM			 10 /* 5 dbm */
#define RSSI_BAD_NEED_ROAM                      -70 /* dbm */
#define RSSI_BAD_NEED_ROAM_24G_TO_5G_6G         -10 /* dbm */
#else
#define RCPI_DIFF_DRIVER_ROAM			 20 /* 10 dbm */
#define RSSI_BAD_NEED_ROAM                      -80 /* dbm */
/* In case 2.4G->5G, the trigger rssi is RSSI_BAD_NEED_ROAM_24G_TO_5G
 * In other case(2.4G->2.4G/5G->2.4G/5G->5G), the trigger
 * rssi is RSSI_BAD_NEED_ROAM
 *
 * The reason of using two rssi threshold is that we only
 * want to benifit 2.4G->5G case, and keep original logic in
 * other cases.
 */
#define RSSI_BAD_NEED_ROAM_24G_TO_5G_6G         -40 /* dbm */
#endif

/* When roam to 5G AP, the AP's rcpi should great than
 * RCPI_THRESHOLD_ROAM_2_5G dbm
 */
#define RCPI_THRESHOLD_ROAM_TO_5G_6G  90 /* rssi -65 */

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
static const char * const apucDebugAisState[AIS_STATE_NUM] = {
	"IDLE",
	"SEARCH",
	"SCAN",
	"ONLINE_SCAN",
	"LOOKING_FOR",
	"WAIT_FOR_NEXT_SCAN",
	"REQ_CHANNEL_JOIN",
	"JOIN",
	"JOIN_FAILURE",
	"IBSS_ALONE",
	"IBSS_MERGE",
	"NORMAL_TR",
	"DISCONNECTING",
	"REQ_REMAIN_ON_CHANNEL",
	"REMAIN_ON_CHANNEL",
	"OFF_CHNL_TX",
	"ROAMING",
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void aisFsmRunEventScanDoneTimeOut(struct ADAPTER *prAdapter,
					  uintptr_t ulParam);

static void aisFunClearAllTxReq(struct ADAPTER *prAdapter,
		struct AIS_MGMT_TX_REQ_INFO *prAisMgmtTxInfo);
static void aisFsmRoamingDisconnectPrevAllAP(struct ADAPTER *prAdapter,
				   struct AIS_FSM_INFO *prAisFsmInfo);
static void aisUpdateBssInfoForRoamingAllAP(struct ADAPTER *prAdapter,
				struct AIS_FSM_INFO *prAisFsmInfo,
				struct SW_RFB *prAssocRspSwRfb,
				struct STA_RECORD *prSetupStaRec);
static void aisChangeAllMediaState(struct ADAPTER *prAdapter,
		struct AIS_FSM_INFO *prAisFsmInfo);

static void aisReqJoinChPrivilege(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	uint8_t *ucChTokenId);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
static uint32_t aisScanGenMlScanReq(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg);

static void aisScanAddRlmIE(struct ADAPTER *prAdapter,
	struct MSG_SCN_SCAN_REQ_V2 *prCmdScanReq);
#endif

static void aisScanReqInit(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg);

static void aisScanProcessReqParam(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg,
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest);

static void aisScanProcessReqCh(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg,
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest);

static void aisScanProcessReqExtra(struct ADAPTER *prAdapter,
	struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg,
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest);

static void aisScanResetReq(struct PARAM_SCAN_REQUEST_ADV *prScanRequest);

static uint8_t aisFsmUpdateRsnSetting(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBss, uint8_t ucBssIndex);

static void aisFsmDisconnectedAction(struct ADAPTER *prAdapter,
				     uint8_t ucBssIndex);

static void aisRestoreBandIdx(struct ADAPTER *ad,
	struct BSS_INFO *prBssInfo);

static void aisRestoreAllLink(struct ADAPTER *ad, struct AIS_FSM_INFO *ais);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void aisResetBssTranstionMgtParam(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex)
{
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
	struct BSS_TRANSITION_MGT_PARAM *prBtmParam;
	struct AIS_FSM_INFO *prAisFsmInfo;

	prBtmParam = aisGetBTMParam(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	cnmTimerStopTimer(prAdapter,
			  aisGetBTMDisassocTimer(prAdapter, ucBssIndex));
	kalMemSet(prBtmParam, 0, sizeof(struct BSS_TRANSITION_MGT_PARAM));
#endif
}

#if (CFG_SUPPORT_HE_ER == 1)
uint8_t aisCheckPowerMatchERCondition(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc)
{
	int8_t txpwr = 0;
	int8_t icBeaconRSSI;

	icBeaconRSSI = RCPI_TO_dBm(prBssDesc->ucRCPI);

	if (prAdapter->prGlueInfo->fgNvramAvailable == FALSE)
		txpwr = AP_TX_POWER;
	else
		wlanGetMiniTxPower(prAdapter, prBssDesc->eBand,
				PHY_MODE_OFDM, &txpwr);

	DBGLOG(AIS, INFO, "ER: STA Tx power:%x, AP Tx power:%x, Bcon RSSI:%x\n",
		txpwr, AP_TX_POWER, icBeaconRSSI);

	return ((txpwr - (AP_TX_POWER - icBeaconRSSI)) < -95);
}

bool aisCheckUsingERRate(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc)
{
	bool fgIsStaUseERRate = false;

	if ((prBssDesc->fgIsERSUDisable == 0) &&
		(prBssDesc->ucDCMMaxConRx > 0) &&
		(prBssDesc->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
		|| prBssDesc->eBand == BAND_6G
#endif
		) && (aisCheckPowerMatchERCondition(prAdapter, prBssDesc))) {
		fgIsStaUseERRate = TRUE;
	}

	DBGLOG(AIS, INFO, "ER: ER disable:%x, max rx:%x, band:%x, use ER:%x\n",
		prBssDesc->fgIsERSUDisable, prBssDesc->ucDCMMaxConRx,
		prBssDesc->eBand, fgIsStaUseERRate);

	return fgIsStaUseERRate;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * @brief the function is used to initialize the value of the connection
 *        settings for AIS network
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisInitializeConnectionSettings(struct ADAPTER *prAdapter,
		struct REG_INFO *prRegInfo, uint8_t ucBssIndex)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t aucAnyBSSID[] = BC_BSSID;
	uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	/* Setup default values for operation */
	COPY_MAC_ADDR(prConnSettings->aucMacAddress, aucZeroMacAddr);

	prConnSettings->ucDelayTimeOfDisconnectEvent =
	    AIS_DELAY_TIME_OF_DISCONNECT_SEC;

	COPY_MAC_ADDR(prConnSettings->aucBSSID, aucAnyBSSID);

	prConnSettings->ucSSIDLen = 0;

	prConnSettings->eOPMode = NET_TYPE_INFRA;

	prConnSettings->eConnectionPolicy = CONNECT_BY_SSID_BEST_RSSI;

	if (prRegInfo) {
		prConnSettings->ucAdHocChannelNum = 0;
		prConnSettings->eAdHocBand =
			prRegInfo->u4StartFreq < 5000000 ? BAND_2G4 :
#if (CFG_SUPPORT_WIFI_6G == 1)
			prRegInfo->u4StartFreq > 5950000 ? BAND_6G :
#endif
			BAND_5G;
		prConnSettings->eAdHocMode =
		    (enum ENUM_PARAM_AD_HOC_MODE)(prRegInfo->u4AdhocMode);
	}

	prConnSettings->eAuthMode = AUTH_MODE_OPEN;

	prConnSettings->eEncStatus = ENUM_ENCRYPTION_DISABLED;

	/* MIB attributes */
	prConnSettings->u2BeaconPeriod = DOT11_BEACON_PERIOD_DEFAULT;

	prConnSettings->u2RTSThreshold = DOT11_RTS_THRESHOLD_DEFAULT;

	prConnSettings->u2DesiredNonHTRateSet = RATE_SET_ALL_ABG;

	/* Set U-APSD AC */
	prConnSettings->bmfgApsdEnAc = PM_UAPSD_NONE;

	secInit(prAdapter, ucBssIndex);

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
	prConnSettings->fgSecModeChangeStartTimer = FALSE;
#endif

	prConnSettings->fgIsAdHocQoSEnable = FALSE;
	aisInitializeConnectionRsnInfo(prAdapter, ucBssIndex);

	kalMemZero(&prConnSettings->rFtIeR0,
		sizeof(prConnSettings->rFtIeR0));
	kalMemZero(&prConnSettings->rFtIeR1,
		sizeof(prConnSettings->rFtIeR1));
} /* end of aisFsmInitializeConnectionSettings() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief the function is used to initialize the RsnInfo value of the connection
 *        settings for AIS network
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisInitializeConnectionRsnInfo(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct IEEE_802_11_MIB *prMib;
	uint8_t i;

	prMib = aisGetMib(prAdapter, ucBssIndex);

	/* reset cipher */
	prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_NONE;
	prMib->dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_NONE;
	prMib->dot11RSNAConfigAkm = 0;

	for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
		prMib->dot11RSNAConfigPairwiseCiphersTable
		    [i].dot11RSNAConfigPairwiseCipherEnabled = FALSE;

	/* reset akm */
	for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++)
		prMib->dot11RSNAConfigAuthenticationSuitesTable
		    [i].dot11RSNAConfigAuthenticationSuiteEnabled = FALSE;
} /* end of aisInitializeConnectionRsnInfo() */

#if CFG_SUPPORT_802_11K
uint32_t aisSync11kCapabilities(struct ADAPTER *prAdapter,
				uint8_t ucBssIndex)
{
	struct CMD_SET_RRM_CAPABILITY rCmdRrmCapa;

	kalMemZero(&rCmdRrmCapa, sizeof(rCmdRrmCapa));
	rCmdRrmCapa.ucCmdVer = 0x1;
	rCmdRrmCapa.ucRrmEnable = 1;
	rrmFillRrmCapa(&rCmdRrmCapa.ucCapabilities[0]);
	rCmdRrmCapa.ucBssIndex = ucBssIndex;
	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SET_RRM_CAPABILITY,
			    TRUE,
			    FALSE,
			    FALSE,
			    NULL,
			    nicOidCmdTimeoutCommon,
			    sizeof(struct CMD_SET_RRM_CAPABILITY),
			    (uint8_t *)&rCmdRrmCapa,
			    NULL, 0);
	return WLAN_STATUS_SUCCESS;
}
#endif

void aisInitBssInfo(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	struct BSS_INFO *prAisBssInfo,
	uint8_t ucLinkIdx)
{
	uint8_t i;

	/* 4 <1.2> Initiate PWR STATE */
	SET_NET_PWR_STATE_IDLE(prAdapter, prAisBssInfo->ucBssIndex);

	/* 4 <2> Initiate BSS_INFO_T - common part */
	BSS_INFO_INIT(prAdapter, prAisBssInfo);

	/* override config only affects default ais, which is wlan0 */
	if (!prAdapter->rWifiVar.ucMacAddrOverride ||
	    prAisFsmInfo->ucAisIndex != AIS_DEFAULT_INDEX) {
		uint8_t *source;

		if (ucLinkIdx > 0 && aisGetMainLinkBssInfo(prAisFsmInfo))
			source = aisGetMainLinkBssInfo(prAisFsmInfo)
					->aucOwnMacAddr;
		else
			source = prAdapter->rWifiVar.aucMacAddress[
					prAisFsmInfo->ucAisIndex];
		/* update MAC address */
		nicApplyLinkAddress(prAdapter, source,
			prAisBssInfo->aucOwnMacAddr, ucLinkIdx);
	} else if (ucLinkIdx * 18 + 17 < WLAN_CFG_VALUE_LEN_MAX) {
		/*    link1 addr        link2 addr        link3 addr    */
		/*aa:bb:cc:dd:ee:ff 11:22:33:44:55:66 11:22:33:44:55:77 */
		wlanHwAddrToBin(prAdapter->rWifiVar.aucMacAddrStr +
			ucLinkIdx * 18, prAisBssInfo->aucOwnMacAddr);

		if (kalIsZeroEtherAddr(prAisBssInfo->aucOwnMacAddr)) {
			DBGLOG(AIS, WARN,
				"MacAddr zero, override it by 1st link\n");
			nicApplyLinkAddress(prAdapter,
			    prAdapter->rWifiVar.aucMacAddrStr,
			    prAisBssInfo->aucOwnMacAddr, ucLinkIdx);
		}

		DBGLOG(AIS, INFO, "link: %d, mac: " MACSTR "\n",
			ucLinkIdx, MAC2STR(prAisBssInfo->aucOwnMacAddr));
	}

	/* 4 <3> Initiate BSS_INFO_T - private part */
	/* TODO */
	prAisBssInfo->eBand = BAND_2G4;
	prAisBssInfo->ucPrimaryChannel = 1;
	prAisBssInfo->prStaRecOfAP = (struct STA_RECORD *) NULL;
	prAisBssInfo->ucOpRxNss = prAisBssInfo->ucOpTxNss =
		wlanGetSupportNss(prAdapter, prAisBssInfo->ucBssIndex);
	/* 4 <4> Allocate MSDU_INFO_T for Beacon */
	prAisBssInfo->prBeacon = cnmMgtPktAlloc(prAdapter,
		OFFSET_OF(struct WLAN_BEACON_FRAME,
		aucInfoElem[0]) + MAX_IE_LENGTH);

	if (prAisBssInfo->prBeacon) {
		prAisBssInfo->prBeacon->eSrc = TX_PACKET_MGMT;
		/* NULL STA_REC */
		prAisBssInfo->prBeacon->ucStaRecIndex = 0xFF;
	}

	prAisBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;

	for (i = 0; i < MAX_KEY_NUM; i++) {
		prAisBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
		prAisBssInfo->ucBMCWlanIndexSUsed[i] = FALSE;
		prAisBssInfo->wepkeyUsed[i] = FALSE;
	}

	prAisBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC =
	    (uint8_t) prAdapter->u4UapsdAcBmp;
	prAisBssInfo->rPmProfSetupInfo.ucBmpTriggerAC =
	    (uint8_t) prAdapter->u4UapsdAcBmp;
	prAisBssInfo->rPmProfSetupInfo.ucUapsdSp =
	    (uint8_t) prAdapter->u4MaxSpLen;

	prAisBssInfo->fgFirstArp = FALSE;

	/* For BSS_INFO back trace to AIS FSM. */
	prAisBssInfo->u4PrivateData = prAisFsmInfo->ucAisIndex;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prAisBssInfo->ucLinkIndex = ucLinkIdx;
#endif
	kalAisCsaNotifyWorkInit(prAdapter,
		prAisBssInfo->ucBssIndex);
	LINK_INITIALIZE(&prAisBssInfo->rPmkidCache);
}

struct BSS_INFO *aisAllocBssInfo(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo, uint8_t ucLinkIdx)
{
	struct BSS_INFO *bss = NULL;

	bss = cnmGetBssInfoAndInit(prAdapter, NETWORK_TYPE_AIS, FALSE,
				   INVALID_OMAC_IDX);
	if (!bss) {
		DBGLOG(AIS, ERROR,
			"prAisBssInfo is NULL for link%d\n", ucLinkIdx);
		if (ucLinkIdx == AIS_MAIN_LINK_INDEX)
			ASSERT(0);
	} else {
		prAisFsmInfo->ucLinkNum++;
		aisSetLinkBssInfo(prAisFsmInfo, bss, ucLinkIdx);
		aisInitBssInfo(prAdapter, prAisFsmInfo, bss, ucLinkIdx);
		wlanBindBssIdxToNetInterface(prAdapter->prGlueInfo,
				bss->ucBssIndex,
				wlanGetAisNetDev(prAdapter->prGlueInfo,
					prAisFsmInfo->ucAisIndex)
				);

		DBGLOG(AIS, INFO,
			"[AIS%d] link%d, bss=%d, omac=%d total=%d\n",
			prAisFsmInfo->ucAisIndex, ucLinkIdx,
			bss->ucBssIndex, bss->ucOwnMacIndex,
			prAisFsmInfo->ucLinkNum);
	}

	return bss;
}

void aisFreeBssInfo(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo, uint8_t ucLinkIdx)
{
	struct BSS_INFO *bss = NULL;
	uint8_t fgHalted = kalIsHalted();
	uint8_t ucBssIndex;

	bss = aisGetLinkBssInfo(prAisFsmInfo, ucLinkIdx);
	/* 4 <3> Reset driver-domain BSS-INFO */
	if (!bss)
		return;

	ucBssIndex = bss->ucBssIndex;

	/* Deactivate BSS. */
	UNSET_NET_ACTIVE(prAdapter, ucBssIndex);
	if (!fgHalted)
		nicDeactivateNetwork(prAdapter,
		       NETWORK_ID(ucBssIndex, ucLinkIdx));

	if (bss->prBeacon) {
		cnmMgtPktFree(prAdapter, bss->prBeacon);
		bss->prBeacon = NULL;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldBssUnregister(prAdapter, prAisFsmInfo->prMldBssInfo, bss);
#endif

	wlanBindBssIdxToNetInterface(prAdapter->prGlueInfo,
		ucBssIndex, NULL);
	cnmFreeBssInfo(prAdapter, bss);
	aisSetLinkBssInfo(prAisFsmInfo, NULL, ucLinkIdx);
	aisSetLinkStaRec(prAisFsmInfo, NULL, ucLinkIdx);
	aisSetLinkBssDesc(prAisFsmInfo, NULL, ucLinkIdx);
	prAisFsmInfo->ucLinkNum--;

	DBGLOG(AIS, INFO,
		"[AIS%d] link%d, bss=%d, total=%d\n",
		prAisFsmInfo->ucAisIndex, ucLinkIdx, ucBssIndex,
		prAisFsmInfo->ucLinkNum);
}

void aisFreeAllBssInfo(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo, uint8_t fgUninit)
{
	struct BSS_INFO *bss;
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		bss = aisGetLinkBssInfo(prAisFsmInfo, i);

		if (!bss)
			break;

		/* force to free all bssinfo if uninit ais,
		 * otherwise free bssinfo which is not connected
		 */
		if (fgUninit ||
		   (i != AIS_MAIN_LINK_INDEX &&
		    bss->eConnectionState != MEDIA_STATE_CONNECTED))
			aisFreeBssInfo(prAdapter, prAisFsmInfo, i);
	}
}

void aisFreeIesMem(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t fgUninit)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	struct FT_IES *prFtIEs;

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	if (fgUninit &&
		prConnSettings && prConnSettings->assocIeLen > 0) {
		kalMemFree(prConnSettings->pucAssocIEs, VIR_MEM_TYPE,
			prConnSettings->assocIeLen);
		prConnSettings->assocIeLen = 0;
		prConnSettings->pucAssocIEs = NULL;
	}

	if (prConnSettings && prConnSettings->u4RspIeLength > 0) {
		kalMemFree(prConnSettings->aucRspIe, VIR_MEM_TYPE,
			prConnSettings->u4RspIeLength);
		prConnSettings->u4RspIeLength = 0;
		prConnSettings->aucRspIe = NULL;
	}

	if (prConnSettings && prConnSettings->u4ReqIeLength > 0) {
		kalMemFree(prConnSettings->aucReqIe, VIR_MEM_TYPE,
			prConnSettings->u4ReqIeLength);
		prConnSettings->u4ReqIeLength = 0;
		prConnSettings->aucReqIe = NULL;
	}

	prFtIEs = aisGetFtIe(prAdapter, ucBssIndex, AIS_FT_R0);
	if (prFtIEs && prFtIEs->u4IeLength > 0) {
		kalMemFree(prFtIEs->pucIEBuf, VIR_MEM_TYPE,
			prFtIEs->u4IeLength);
		kalMemZero(prFtIEs, sizeof(*prFtIEs));
	}

	prFtIEs = aisGetFtIe(prAdapter, ucBssIndex, AIS_FT_R1);
	if (prFtIEs && prFtIEs->u4IeLength > 0) {
		kalMemFree(prFtIEs->pucIEBuf,
			VIR_MEM_TYPE,
			prFtIEs->u4IeLength);
		kalMemZero(prFtIEs, sizeof(*prFtIEs));
	}
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
struct MLD_BLOCKLIST_ITEM *aisAddMldBlocklist(struct ADAPTER *prAdapter,
					   struct BSS_DESC *prBssDesc)
{
	struct MLD_BLOCKLIST_ITEM *prEntry = NULL;
	struct LINK_MGMT *prBlockList = &prAdapter->rWifiVar.rMldBlockList;
	struct BSS_DESC *prTmpBssDesc = NULL;
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;

	if (!prBssDesc || !prBssDesc->rMlInfo.fgValid) {
		DBGLOG(AIS, ERROR, "bss descriptor is not valid\n");
		return NULL;
	}
	if (prBssDesc->rMlInfo.prBlock) {
		GET_CURRENT_SYSTIME(&prBssDesc->rMlInfo.prBlock->rAddTime);
		prBssDesc->rMlInfo.prBlock->ucCount++;
		DBGLOG(AIS, INFO, "update blocklist for mld " MACSTR
		       ", count %d\n",
		       MAC2STR(prBssDesc->rMlInfo.aucMldAddr),
		       prBssDesc->rMlInfo.prBlock->ucCount);
		return prBssDesc->rMlInfo.prBlock;
	}

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;
	prEntry = aisQueryMldBlockList(prAdapter, prBssDesc);

	if (prEntry) {
		GET_CURRENT_SYSTIME(&prEntry->rAddTime);
		prBssDesc->rMlInfo.prBlock = prEntry;
		prEntry->ucCount++;
		DBGLOG(AIS, INFO, "update blocklist for mld " MACSTR
		       ", count %d\n",
		       MAC2STR(prBssDesc->rMlInfo.aucMldAddr),
		       prEntry->ucCount);
		return prEntry;
	}
	LINK_MGMT_GET_ENTRY(prBlockList, prEntry, struct MLD_BLOCKLIST_ITEM,
			    VIR_MEM_TYPE);
	if (!prEntry) {
		DBGLOG(AIS, WARN, "No memory to allocate\n");
		return NULL;
	}
	prEntry->ucCount = 1;
	/* Support AP Selection */
	COPY_MAC_ADDR(prEntry->aucMldAddr, prBssDesc->rMlInfo.aucMldAddr);
	GET_CURRENT_SYSTIME(&prEntry->rAddTime);

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prTmpBssDesc, prBSSDescList,
		rLinkEntry, struct BSS_DESC) {

		if (!prTmpBssDesc->rMlInfo.fgValid)
			continue;

		/* mld has affiliated APs */
		if ((EQUAL_MAC_ADDR(prTmpBssDesc->rMlInfo.aucMldAddr,
			prEntry->aucMldAddr))) {
			prTmpBssDesc->rMlInfo.prBlock = prEntry;
		}
	}

	DBGLOG(AIS, INFO, "Add mld " MACSTR " to blocklist\n",
	       MAC2STR(prBssDesc->rMlInfo.aucMldAddr));
	return prEntry;
}

void aisRemoveMldBlockList(struct ADAPTER *prAdapter,
			   struct BSS_DESC *prBssDesc)
{
	struct MLD_BLOCKLIST_ITEM *prEntry = NULL;
	struct BSS_DESC *prTmpBssDesc = NULL;
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;
	prEntry = aisQueryMldBlockList(prAdapter, prBssDesc);
	if (!prEntry)
		return;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prTmpBssDesc, prBSSDescList,
		rLinkEntry, struct BSS_DESC) {

		if (!prTmpBssDesc->rMlInfo.fgValid)
			continue;

		/* mld has affiliated APs */
		if ((EQUAL_MAC_ADDR(prTmpBssDesc->rMlInfo.aucMldAddr,
			prEntry->aucMldAddr))) {
			prTmpBssDesc->rMlInfo.prBlock = NULL;
		}
	}

	LINK_MGMT_RETURN_ENTRY(&prAdapter->rWifiVar.rMldBlockList, prEntry);
	DBGLOG(AIS, INFO, "Remove mld " MACSTR " from blocklist\n",
	       MAC2STR(prBssDesc->rMlInfo.aucMldAddr));
}

struct MLD_BLOCKLIST_ITEM *aisQueryMldBlockList(struct ADAPTER *prAdapter,
					     struct BSS_DESC *prBssDesc)
{
	struct MLD_BLOCKLIST_ITEM *prEntry = NULL;
	struct LINK *prBlockList =
		&prAdapter->rWifiVar.rMldBlockList.rUsingLink;

	if (!prBssDesc)
		return NULL;
	else if (prBssDesc->rMlInfo.prBlock)
		return prBssDesc->rMlInfo.prBlock;

	LINK_FOR_EACH_ENTRY(prEntry, prBlockList, rLinkEntry,
			    struct MLD_BLOCKLIST_ITEM) {
		if (EQUAL_MAC_ADDR(prBssDesc->rMlInfo.aucMldAddr,
				   prEntry->aucMldAddr))
			return prEntry;
	}
	DBGLOG(AIS, LOUD, MACSTR " is not in mld blocklist\n",
	       MAC2STR(prBssDesc->rMlInfo.aucMldAddr));
	return NULL;
}

void aisRemoveTimeoutMldBlocklist(struct ADAPTER *prAdapter)
{
	struct MLD_BLOCKLIST_ITEM *prEntry = NULL;
	struct MLD_BLOCKLIST_ITEM *prNextEntry = NULL;
	struct LINK *prBlockList;
	struct LINK *prFreeList;
	struct BSS_DESC *prBssDesc = NULL;
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;
	OS_SYSTIME rCurrent;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;
	prBlockList = &prAdapter->rWifiVar.rMldBlockList.rUsingLink;
	prFreeList = &prAdapter->rWifiVar.rMldBlockList.rFreeLink;

	GET_CURRENT_SYSTIME(&rCurrent);

	LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prBlockList, rLinkEntry,
				 struct MLD_BLOCKLIST_ITEM) {
		uint16_t sec = AIS_BLOCKLIST_TIMEOUT;

		if (!CHECK_FOR_TIMEOUT(rCurrent, prEntry->rAddTime,
				       SEC_TO_MSEC(sec)))
			continue;

		/* Search BSS Desc from current SCAN result list. */
		LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList,
			rLinkEntry, struct BSS_DESC) {

			if (!prBssDesc->rMlInfo.fgValid)
				continue;

			/* mld has affiliated APs */
			if ((EQUAL_MAC_ADDR(prBssDesc->rMlInfo.aucMldAddr,
				prEntry->aucMldAddr))) {
				prBssDesc->rMlInfo.prBlock = NULL;
			}
		}

		DBGLOG(AIS, INFO,
			"Remove Timeout mld "MACSTR" from blocklist\n",
			MAC2STR(prBssDesc->rMlInfo.aucMldAddr));
		LINK_MGMT_RETURN_ENTRY(&prAdapter->rWifiVar.rMldBlockList,
			prEntry);
	}

	if (prFreeList->u4NumElem > 20) {
		while (!LINK_IS_EMPTY(prFreeList)) {
			LINK_REMOVE_HEAD(prFreeList, prEntry,
				struct MLD_BLOCKLIST_ITEM *);
			kalMemFree(prEntry, VIR_MEM_TYPE,
				sizeof(struct MLD_BLOCKLIST_ITEM));
		}
	}
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief the function is used to initialize the value in AIS_FSM_INFO_T for
 *        AIS FSM operation
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmInit(struct ADAPTER *prAdapter,
		struct REG_INFO *prRegInfo,
		uint8_t ucAisIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo =
		aisFsmGetInstance(prAdapter, ucAisIndex);
	struct BSS_INFO *prBssInfo;
	struct WIFI_VAR *prWifiVar;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	uint8_t aucMldMac[MAC_ADDR_LEN];
#endif
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct AIS_MGMT_TX_REQ_INFO *prMgmtTxReqInfo =
			(struct AIS_MGMT_TX_REQ_INFO *) NULL;
	struct GL_WPA_INFO *prWpaInfo;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) NULL;
	uint8_t ucBssIndex, i;
	struct APS_INFO *prApsInfo;

	if (!prAisFsmInfo) {
		DBGLOG(AIS, ERROR, "prAisFsmInfo is NULL!\n");
		return;
	}

	if (!wlanGetAisNetDev(prAdapter->prGlueInfo, ucAisIndex)) {
		DBGLOG(AIS, INFO, "-> ais(%d) netdev null\n", ucAisIndex);
		return;
	}

	DBGLOG(AIS, INFO, "->aisFsmInit(%d)\n", ucAisIndex);

	/* avoid that the prAisBssInfo is realloc */
	if (aisGetMainLinkBssInfo(prAisFsmInfo) != NULL) {
		DBGLOG(AIS, INFO, "-> realloc(%d)\n", ucAisIndex);
		return;
	}

	prAisFsmInfo->ucAisIndex = ucAisIndex;
	if (ucAisIndex == AIS_DEFAULT_INDEX)
		prAdapter->rWifiVar.prDefaultAisFsmInfo = prAisFsmInfo;

	prWifiVar = &prAdapter->rWifiVar;
	prAisFsmInfo->u4BssIdxBmap = 0;
	prAisFsmInfo->ucLinkNum = 0;
	for (i = 0; i < MAX_BSSID_NUM + 1; i++)
		prAisFsmInfo->arBssId2LinkMap[i] = MLD_LINK_ID_NONE;
	prBssInfo = aisAllocBssInfo(prAdapter, prAisFsmInfo,
		AIS_MAIN_LINK_INDEX);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	COPY_MAC_ADDR(aucMldMac, prBssInfo->aucOwnMacAddr);
	if (IS_FEATURE_DISABLED(prWifiVar->fgMldSyncLinkAddr))
		kalRandomGetBytes((void *)&aucMldMac[3], 3);

	prMldBssInfo = mldBssAlloc(prAdapter, aucMldMac);
	prAisFsmInfo->prMldBssInfo = prMldBssInfo;
	prAisFsmInfo->ucMlProbeSendCount = 0;
	prAisFsmInfo->ucMlProbeEnable = FALSE;

	mldBssRegister(prAdapter, prAisFsmInfo->prMldBssInfo, prBssInfo);
#endif

	/* after aisInitBssInfo, bssinfo is ready */
	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
				kalGetNetDevPriv(
					wlanGetAisNetDev(prAdapter->prGlueInfo,
					ucAisIndex)
				);

	prNetDevPrivate->ucBssIdx = AIS_MAIN_BSS_INDEX(prAdapter, ucAisIndex);

	prNetDevPrivate->ucIsP2p = FALSE;

#if CFG_MTK_MDDP_SUPPORT
	/* support both wlan0 and wlan1 */
	prNetDevPrivate->ucMddpSupport = TRUE;
#else
	prNetDevPrivate->ucMddpSupport = FALSE;
#endif

	aisClearAllLink(prAisFsmInfo);

	/* from this point , bssinfo is ready to use */
	prAisSpecificBssInfo = &prAisFsmInfo->rAisSpecificBssInfo;
	prConnSettings = &prAisFsmInfo->rConnSettings;
	prWpaInfo = &prAisFsmInfo->rWpaInfo;
	prApsInfo = &prAisFsmInfo->rApsInfo;
	ucBssIndex = aisGetMainLinkBssIndex(prAdapter, prAisFsmInfo);

	/* The Init value of u4WpaVersion/u4AuthAlg shall be
	 * DISABLE/OPEN, not zero!
	 */
	/* The Init value of u4CipherGroup/u4CipherPairwise shall be
	 * NONE, not zero!
	 */
	prWpaInfo->u4WpaVersion = IW_AUTH_WPA_VERSION_DISABLED;
	prWpaInfo->u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;
	prWpaInfo->u4CipherGroup = IW_AUTH_CIPHER_NONE;
	prWpaInfo->u4CipherPairwise = IW_AUTH_CIPHER_NONE;

	/* Init number of AP in ESS */
	prApsInfo->u2EssApNum = 0;

	/* kalGetMediaStateIndicated to disconneted */
	prAisFsmInfo->eParamMediaStateIndicated = MEDIA_STATE_DISCONNECTED;

	/* 4 <1> Initiate FSM */
	prAisFsmInfo->ePreviousState = AIS_STATE_IDLE;
	prAisFsmInfo->eCurrentState = AIS_STATE_IDLE;
	prAisFsmInfo->ucReasonOfDisconnect = DISCONNECT_REASON_CODE_RESERVED;

	prAisFsmInfo->ucAvailableAuthTypes = 0;

	prAisFsmInfo->ucSeqNumOfReqMsg = 0;
	prAisFsmInfo->ucSeqNumOfChReq = 0;
	prAisFsmInfo->ucSeqNumOfScanReq = 0;
	prAisFsmInfo->fgIsChannelRequested = FALSE;
	prAisFsmInfo->fgIsChannelGranted = FALSE;
	prAisFsmInfo->fgIsDelIface = FALSE;
	prAisFsmInfo->u4PostponeIndStartTime = 0;
	/* Support AP Selection */
	prAisFsmInfo->ucJoinFailCntAfterScan = 0;
	prAisFsmInfo->ucIsSapCsaPending = FALSE;

	prAisFsmInfo->fgIsScanOidAborted = FALSE;

	prAisFsmInfo->fgIsScanning = FALSE;

	aisInitializeConnectionSettings(prAdapter, prRegInfo, ucBssIndex);

#if CFG_SUPPORT_ROAMING
	/* Roaming Module - intiailization */
	roamingFsmInit(prAdapter, ucBssIndex);
#endif /* CFG_SUPPORT_ROAMING */


	/* 4 <1.1> Initiate FSM - Timer INIT */
	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rBGScanTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventBGSleepTimeOut,
			  (uintptr_t)ucBssIndex);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rIbssAloneTimer,
			  (PFN_MGMT_TIMEOUT_FUNC)
			  aisFsmRunEventIbssAloneTimeOut,
			  (uintptr_t)ucBssIndex);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rScanDoneTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventScanDoneTimeOut,
			  (uintptr_t)ucBssIndex);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rJoinTimeoutTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventJoinTimeout,
			  (uintptr_t)ucBssIndex);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rChannelTimeoutTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventChannelTimeout,
			  (uintptr_t)ucBssIndex);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rDeauthDoneTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventDeauthTimeout,
			  (uintptr_t)ucBssIndex);

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rSecModeChangeTimer,
			  (PFN_MGMT_TIMEOUT_FUNC)
			  aisFsmRunEventSecModeChangeTimeout,
			  (uintptr_t)ucBssIndex);
#endif

#if (CFG_SUPPORT_ML_RECONFIG == 1)
	cnmTimerInitTimer(prAdapter,
		&prAisFsmInfo->rApRemovalTimer,
		(PFN_MGMT_TIMEOUT_FUNC) aisFsmRunApRemovalTimeout,
		(uintptr_t)ucBssIndex);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	prMgmtTxReqInfo = &prAisFsmInfo->rMgmtTxInfo;
	LINK_INITIALIZE(&prMgmtTxReqInfo->rTxReqLink);
	prMgmtTxReqInfo->prMgmtTxMsdu = NULL;

	/* request list initialization */
	LINK_INITIALIZE(&prAisFsmInfo->rPendingReqList);

	kalMemZero(&prAisSpecificBssInfo->arCurEssChnlInfo[0],
		   sizeof(prAisSpecificBssInfo->arCurEssChnlInfo));
	LINK_INITIALIZE(&prAisSpecificBssInfo->rCurEssLink);
	kalMemZero(&prAisSpecificBssInfo->arApHash[0],
		   sizeof(prAisSpecificBssInfo->arApHash));
	/* end Support AP Selection */

	/* 11K, 11V */
	LINK_MGMT_INIT(&prAisSpecificBssInfo->rNeighborApList);
	cnmTimerInitTimer(prAdapter,
			  &prAisSpecificBssInfo->rBTMDisassocTimer,
#if CFG_SUPPORT_ROAMING
			  (PFN_MGMT_TIMEOUT_FUNC) roamingFsmBTMTimeout,
#else
			  NULL,
#endif
			  (uintptr_t) ucBssIndex);


	rrmParamInit(prAdapter, ucBssIndex);
#if CFG_SUPPORT_802_11W
	kal_init_completion(&prAisFsmInfo->rDeauthComp);
	prAisFsmInfo->encryptedDeauthIsInProcess = FALSE;
	prAisSpecificBssInfo->prTargetComebackBssDesc = NULL;
	prAisSpecificBssInfo->fgBipKeyInstalled = FALSE;
	prAisSpecificBssInfo->fgBipGmacKeyInstalled = FALSE;
#endif
	/* AX blocklist*/
	LINK_INITIALIZE(&prAisFsmInfo->rAxBlocklist);
	/* HE HTC blocklist*/
	LINK_INITIALIZE(&prAisFsmInfo->rHeHtcBlocklist);

	/* Customized blocklist */
	LINK_INITIALIZE(&prAisFsmInfo->rCusBlocklist);

	wmmInit(prAdapter, ucBssIndex);

#if CFG_SUPPORT_802_11K
	aisSync11kCapabilities(prAdapter, ucBssIndex);
#endif

	/* keep last, indicate disconnection as default status */
	kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
		WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0, ucBssIndex);

	GET_CURRENT_SYSTIME(
		&prAdapter->rRsnFwDumpTime);

}				/* end of aisFsmInit() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief the function is used to uninitialize the value in AIS_FSM_INFO_T for
 *        AIS FSM operation
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmUninit(struct ADAPTER *prAdapter, uint8_t ucAisIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo =
		aisFsmGetInstance(prAdapter, ucAisIndex);
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	u_int8_t fgHalted = kalIsHalted();
	uint8_t ucBssIndex;

	GLUE_SPIN_LOCK_DECLARATION();

	DBGLOG(AIS, INFO, "->aisFsmUninit(%d)\n", ucAisIndex);

	/* avoid that the prAisBssInfo is double freed */
	if (aisGetMainLinkBssInfo(prAisFsmInfo) == NULL) {
		DBGLOG(AIS, INFO, "-> ais(%d) main bssinfo null\n", ucAisIndex);
		return;
	}

	ucBssIndex = aisGetMainLinkBssIndex(prAdapter, prAisFsmInfo);
	prAisSpecificBssInfo =
		aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

#if CFG_SUPPORT_ROAMING
	/* Roaming Module - unintiailization */
	roamingFsmUninit(prAdapter, ucBssIndex);
#endif /* CFG_SUPPORT_ROAMING */

	/* 4 <1> Stop all timers */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBGScanTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
	if (kalGetGlueScanReq(prAdapter->prGlueInfo) != NULL) {
		/* call aisFsmRunEventScanDoneTimeOut()
		 * to reset scan fsm
		 */
		if (!fgHalted)
			aisFsmRunEventScanDoneTimeOut(prAdapter,
				(uintptr_t)ucBssIndex);

		GLUE_ACQUIRE_SPIN_LOCK(prAdapter->prGlueInfo,
				SPIN_LOCK_NET_DEV);
		kalCfg80211ScanDone(prAdapter->prGlueInfo
				->prScanRequest, TRUE);
		kalClearGlueScanReq(prAdapter->prGlueInfo);
		GLUE_RELEASE_SPIN_LOCK(prAdapter->prGlueInfo,
				SPIN_LOCK_NET_DEV);
	}
	/* For FW assert trigger reset case, stop sched scan */
	if (kalGetGlueSchedScanReq(prAdapter->prGlueInfo) != NULL) {
		if (kalIsResetting())
			kalClearGlueSchedScanReq(prAdapter->prGlueInfo);
	}

	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer);
#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rSecModeChangeTimer);
#endif

#if (CFG_SUPPORT_ML_RECONFIG == 1)
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rApRemovalTimer);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* 4 <2> flush pending request */
	aisFsmFlushRequest(prAdapter, ucBssIndex);

	aisFunClearAllTxReq(prAdapter, &(prAisFsmInfo->rMgmtTxInfo));

#if CFG_SUPPORT_802_11W
	rsnStopSaQuery(prAdapter, ucBssIndex);
#endif

	LINK_MGMT_UNINIT(&prAisSpecificBssInfo->rNeighborApList,
			 struct NEIGHBOR_AP, VIR_MEM_TYPE);

	/* make sure pmkid cached is empty after uninit*/
	rsnFlushPmkid(prAdapter, ucBssIndex);

	/* make sure allocated buffer for IEs is free after uninit*/
	aisFreeIesMem(prAdapter, ucBssIndex, TRUE);

	rrmParamInit(prAdapter, ucBssIndex);
	clearAxBlocklist(prAdapter, ucBssIndex, BLOCKLIST_AX_TO_AC);
	clearAxBlocklist(prAdapter, ucBssIndex, BLOCKLIST_DIS_HE_HTC);

	aisClearCusBlocklist(prAdapter, ucBssIndex, TRUE);

	wmmUnInit(prAdapter, ucBssIndex);

	aisFreeAllBssInfo(prAdapter, prAisFsmInfo, TRUE);
	aisResetBssTranstionMgtParam(prAdapter, ucBssIndex);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldBssFree(prAdapter, prAisFsmInfo->prMldBssInfo);
	prAisFsmInfo->prMldBssInfo = NULL;
#endif
} /* end of aisFsmUninit() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Check if ais is processing beacon timeout
 *
 * @return true if processing
 */
/*----------------------------------------------------------------------------*/
bool aisFsmIsInProcessPostpone(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *fsm = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	struct BSS_INFO *bss = aisGetAisBssInfo(prAdapter, ucBssIndex);
	struct CONNECTION_SETTINGS *set =
		aisGetConnSettings(prAdapter, ucBssIndex);

	return bss->eConnectionState == MEDIA_STATE_DISCONNECTED &&
	    fsm->u4PostponeIndStartTime > 0 &&
	    !CHECK_FOR_TIMEOUT(kalGetTimeTick(), fsm->u4PostponeIndStartTime,
	    SEC_TO_MSEC(set->ucDelayTimeOfDisconnectEvent));
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to check the BSS Desc at scan result
 *             with pre-auth cap at wpa2 mode. If there is no cache entry,
 *             notify the PMKID indication.
 *
 * \param[in] prBss The BSS Desc at scan result
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
struct PMKID_ENTRY *aisSearchPmkidEntry(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec,
			uint8_t ucBssIndex)
{
	struct PMKID_ENTRY *entry = NULL;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct BSS_DESC *prBssDesc = NULL;
	uint8_t	*prBssid = NULL;
	struct PARAM_SSID rSsid = {0};
	uint8_t	*prFilsCacheId = NULL;
	struct CONNECTION_SETTINGS *prConnSettings;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetMainLinkBssInfo(prAisFsmInfo);

	if (!prAisBssInfo) {
		DBGLOG(AIS, ERROR, "prAisBssInfo is NULL!");
		return NULL;
	}
	if (!prStaRec) {
		DBGLOG(AIS, ERROR, "prStaRec is NULL!");
		return NULL;
	}

	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
	prBssid = cnmStaRecAuthAddr(prAdapter, prStaRec);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	if (prBssDesc->u4RsnSelectedAKMSuite == RSN_AKM_SUITE_FILS_SHA256 ||
	    prBssDesc->u4RsnSelectedAKMSuite == RSN_AKM_SUITE_FILS_SHA384) {
		kalMemZero(&rSsid, sizeof(struct PARAM_SSID));
		COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
			prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
		prFilsCacheId = scanGetFilsCacheIdFromBssDesc(prBssDesc);
		prBssid = NULL;
	}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

	entry = rsnSearchPmkidEntryEx(prAdapter,
		prBssid, /* bssid */
		&rSsid, /* SSDID */
		prFilsCacheId, /* cache id */
		prAisBssInfo->ucBssIndex); /* pmksa of main link*/

	/* Do not use PMKID if
	 * 1. it is invalid
	 * 2. auth type is SAE
	 */
	if (entry &&
	    (rsnApInvalidPMK(entry->u2StatusCode, prConnSettings->eAuthMode) ||
	     prStaRec->ucAuthAlgNum == AUTH_ALGORITHM_NUM_SAE)) {
		DBGLOG(RSN, INFO,
			"Do not apply PMKID in RSNIE if invalidPMK or auth type is SAE");
		entry = NULL;
	}

	if (entry && prConnSettings->eAuthMode == AUTH_MODE_WPA3_OWE
		&& bssIsIotAp(prAdapter, prBssDesc,
			      WLAN_IOT_AP_OWE_PMK_REMOVE)) {
		DBGLOG(RSN, INFO,
			"IoT AP: Do not apply PMKID in RSNIE if auth type is OWE");
		entry = NULL;
	}

	return entry;
}

void aisCheckPmkidCache(struct ADAPTER *prAdapter, struct BSS_DESC *prBss,
	uint8_t ucAisIndex)
{
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint32_t u4Bmap;

	/* Skip low rssi AP which won't select to roam */
	if (!prBss || prBss->ucRCPI < RCPI_FOR_DONT_ROAM)
		return;

	prAisFsmInfo = aisFsmGetInstance(prAdapter, ucAisIndex);
	if (!prAisFsmInfo) {
		DBGLOG(AIS, ERROR, "prAisFsmInfo is NULL!\n");
		return;
	}

	prConnSettings = &prAisFsmInfo->rConnSettings;
	prAisBssInfo = aisGetMainLinkBssInfo(prAisFsmInfo);
	u4Bmap = aisGetBssIndexBmap(prAisFsmInfo);

	/* Generate pmkid candidate indications for other APs which are
	 * also belong to the same SSID with the current connected AP or
	 * beacon timeout AP but have no available pmkid.
	 */
	if (prAisBssInfo &&
	    (prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED ||
	    aisFsmIsInProcessPostpone(prAdapter, prAisBssInfo->ucBssIndex)) &&
	    (prConnSettings->eAuthMode == AUTH_MODE_WPA2 ||
	     prConnSettings->eAuthMode == AUTH_MODE_WPA3_OWE ||
	     prConnSettings->eAuthMode == AUTH_MODE_WPA3_SAE) &&
	    EQUAL_SSID(prBss->aucSSID, prBss->ucSSIDLen,
		prConnSettings->aucSSID, prConnSettings->ucSSIDLen) &&
	    !(prBss->fgIsConnected & u4Bmap)) {
		struct PARAM_PMKID_CANDIDATE candidate;
		struct PMKID_ENTRY *entry;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (mldIsMultiLinkFormed(prAdapter, prAisBssInfo->prStaRecOfAP))
			entry = rsnSearchPmkidEntry(prAdapter,
					prBss->rMlInfo.aucMldAddr,
					prAisBssInfo->ucBssIndex);
		else
#endif
			entry = rsnSearchPmkidEntry(prAdapter,
					prBss->aucBSSID,
					prAisBssInfo->ucBssIndex);
		if (entry)
			return;

		COPY_MAC_ADDR(candidate.arBSSID, prBss->aucBSSID);
		candidate.u4Flags = prBss->u2RsnCap & MASK_RSNIE_CAP_PREAUTH;
		rsnGeneratePmkidIndication(prAdapter, &candidate,
			prAisBssInfo->ucBssIndex);

		DBGLOG(RSN, TRACE, "[%d] Generate " MACSTR
			" with preauth %d to pmkid candidate list\n",
			prAisBssInfo->ucBssIndex,
			MAC2STR(prBss->aucBSSID), candidate.u4Flags);
	}
} /* rsnCheckPmkidCache */

#if (CFG_SUPPORT_ML_RECONFIG == 1)
void aisCheckApRemoval(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, uint16_t u2ApRemovalTimer)
{
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	uint32_t u4ApRemovalTime;
	uint32_t u4Margin = prAdapter->rWifiVar.u4ApRemovalMarginMs;
	uint8_t ucBssIndex = prStaRec->ucBssIndex;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	if (prStaRec->fgApRemoval || !u2ApRemovalTimer)
		return;

	u4ApRemovalTime = prAisBssInfo->u2BeaconInterval *
			  u2ApRemovalTimer;

	if (u4ApRemovalTime > u4Margin)
		u4ApRemovalTime -= u4Margin;
	else
		u4ApRemovalTime = 0;

	if (u4ApRemovalTime != 0 &&
	    !timerPendingTimer(&prAisFsmInfo->rApRemovalTimer)) {
		DBGLOG(AIS, INFO,
			"BSS[%d] ap removal %d MS (timer=%d) for " MACSTR "\n",
			ucBssIndex, u4ApRemovalTime, u2ApRemovalTimer,
			MAC2STR(prStaRec->aucMacAddr));

		prStaRec->fgApRemoval = TRUE;
		cnmTimerStartTimer(prAdapter,
			&prAisFsmInfo->rApRemovalTimer,
			u4ApRemovalTime);
	}
}
#endif /* CFG_SUPPORT_ML_RECONFIG */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialization of JOIN STATE
 *
 * @param[in] prBssDesc  The pointer of BSS_DESC_T which is the BSS we will
 *                       try to join with.
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateInit_JOIN(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	struct STA_RECORD **prMainStaRec,
	uint8_t ucLinkIndex)
{
	struct BSS_INFO *prBssInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct STA_RECORD *prStaRec;
	struct MSG_SAA_FSM_START *prJoinReqMsg;
	struct GL_WPA_INFO *prWpaInfo;
	struct FT_EVENT_PARAMS *prFtParam;
#if (CFG_SUPPORT_HE_ER == 1)
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#endif
	struct BSS_DESC *prBssDesc;
	uint8_t ucBssIndex;

	prAisSpecificBssInfo = &prAisFsmInfo->rAisSpecificBssInfo;
	prConnSettings = &prAisFsmInfo->rConnSettings;
	prWpaInfo = &prAisFsmInfo->rWpaInfo;
	prBssDesc = aisGetLinkBssDesc(prAisFsmInfo, ucLinkIndex);
	prBssInfo = aisGetLinkBssInfo(prAisFsmInfo, ucLinkIndex);

	if (!prBssInfo) {
		DBGLOG(AIS, ERROR,
			"aisFsmStateInit_JOIN failed because prAisBssInfo is NULL, return.\n");
		return;
	}

	ucBssIndex = prBssInfo->ucBssIndex;
	prFtParam = aisGetFtEventParam(prAdapter, ucBssIndex);

	/* 4 <1> We are going to connect to this BSS. */
	prBssDesc->fgIsConnecting |= BIT(ucBssIndex);

	/* 4 <2> Setup corresponding STA_RECORD_T */
	prStaRec = bssCreateStaRecFromBssDesc(prAdapter,
					      STA_TYPE_LEGACY_AP,
					      ucBssIndex,
					      prBssDesc);

	if (!prStaRec) {
		DBGLOG(AIS, ERROR,
			"aisFsmStateInit_JOIN failed because prStaRec is NULL, return.\n");
		aisFsmStateAbort_JOIN(prAdapter, ucBssIndex);
		aisFsmSteps(prAdapter, AIS_STATE_JOIN_FAILURE, ucBssIndex);
		return;
	}

	if (*prMainStaRec == NULL)
		*prMainStaRec = prStaRec;

#if CFG_SUPPORT_SCAN_LOG
	scanAbortBeaconRecv(prAdapter,
		ucBssIndex,
		ABORT_CONNECT_STARTS);
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (mldSingleLink(prAdapter, prStaRec, ucBssIndex)) {
		prBssInfo->ucLinkIndex = prBssDesc->rMlInfo.ucLinkIndex;
		mldStarecJoin(prAdapter, prAisFsmInfo->prMldBssInfo,
			*prMainStaRec, prStaRec, prBssDesc);
	}
#endif

	aisSetLinkStaRec(prAisFsmInfo, prStaRec, ucLinkIndex);

	/* 4 <2.1> sync. to firmware domain */
	if (prStaRec->ucStaState == STA_STATE_1)
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);
	/* init to prevent returning status success due to join timeout. */
	prStaRec->u2StatusCode = STATUS_CODE_AUTH_TIMEOUT;

	/* 4 <3> Update ucAvailableAuthTypes which we can choice during SAA */
	if (aisGetMainLinkBssInfo(prAisFsmInfo)->eConnectionState ==
		MEDIA_STATE_DISCONNECTED
		/* not in reconnection */
		&& (!aisFsmIsInProcessPostpone(prAdapter, ucBssIndex)
		|| prAisFsmInfo->ucReasonOfDisconnect ==
			DISCONNECT_REASON_CODE_DEAUTHENTICATED
		|| prAisFsmInfo->ucReasonOfDisconnect ==
			DISCONNECT_REASON_CODE_DISASSOCIATED)) {

		prStaRec->fgIsReAssoc = FALSE;

		switch (prConnSettings->eAuthMode) {
		case AUTH_MODE_OPEN:
		case AUTH_MODE_WPA2_FT:
		case AUTH_MODE_WPA2_FT_PSK:
		case AUTH_MODE_WPA:
		case AUTH_MODE_WPA_PSK:
		case AUTH_MODE_WPA2:
		case AUTH_MODE_WPA2_PSK:
		case AUTH_MODE_WPA_OSEN:
		case AUTH_MODE_WPA3_OWE:
			prAisFsmInfo->ucAvailableAuthTypes =
			    (uint8_t) AUTH_TYPE_OPEN_SYSTEM;
			break;

		case AUTH_MODE_SHARED:
			prAisFsmInfo->ucAvailableAuthTypes =
			    (uint8_t) AUTH_TYPE_SHARED_KEY;
			break;

		case AUTH_MODE_AUTO_SWITCH:
			DBGLOG(AIS, INFO,
			       "JOIN INIT: eAuthMode == AUTH_MODE_AUTO_SWITCH\n");
			prAisFsmInfo->ucAvailableAuthTypes =
			    (uint8_t) (AUTH_TYPE_OPEN_SYSTEM |
				       AUTH_TYPE_SHARED_KEY);
			break;

		case AUTH_MODE_WPA3_SAE:
			if (!aisSearchPmkidEntry(prAdapter,
					prStaRec, ucBssIndex)) {
				prAisFsmInfo->ucAvailableAuthTypes =
					(uint8_t) AUTH_TYPE_SAE;
				DBGLOG(AIS, INFO,
					"JOIN INIT: change AUTH to SAE when PMK not found\n");
			} else {
				prAisFsmInfo->ucAvailableAuthTypes =
					(uint8_t) (AUTH_TYPE_OPEN_SYSTEM |
						   AUTH_TYPE_SAE);
				DBGLOG(AIS, INFO,
					"JOIN INIT: eAuthMode == OPEN | SAE\n");
			}
			break;
#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
		case AUTH_MODE_FILS:
			if (prWpaInfo->u4AuthAlg == IW_AUTH_ALG_FILS_SK) {
				if (aisGetErpKey(prAdapter, ucBssIndex) ||
				    aisSearchPmkidEntry(prAdapter,
					prStaRec, ucBssIndex)) {
					prAisFsmInfo->ucAvailableAuthTypes =
						(uint8_t) AUTH_TYPE_FILS_SK;
					DBGLOG(AIS, INFO,
						"JOIN INIT: eAuthMode == FILS\n");
				} else {
					DBGLOG(AIS, ERROR,
						"JOIN INIT: FILS failed\n");
					return;
				}
			} else {
				prAisFsmInfo->ucAvailableAuthTypes =
					(uint8_t) AUTH_TYPE_OPEN_SYSTEM;
				DBGLOG(AIS, INFO,
					"JOIN INIT: eAuthMode == OPEN\n");
			}
			break;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
		default:
			DBGLOG(AIS, ERROR,
			       "JOIN INIT: Auth Algorithm : %d was not supported by JOIN\n",
			       prConnSettings->eAuthMode);
			/* TODO(Kevin): error handling ? */
			return;
		}

		/* TODO(tyhsu): Assume that Roaming Auth Type
		 * is equal to ConnSettings eAuthMode
		 */
		prAisSpecificBssInfo->ucRoamingAuthTypes =
		    prAisFsmInfo->ucAvailableAuthTypes;

		prStaRec->ucTxAuthAssocRetryLimit = TX_AUTH_ASSOCI_RETRY_LIMIT;

		/* Update Bss info before join */
		prBssInfo->eBand = prBssDesc->eBand;
		prBssInfo->ucPrimaryChannel = prBssDesc->ucChannelNum;

#if (CFG_SUPPORT_HE_ER == 1)
		prStaRec->fgIsExtendedRange = FALSE;

		if (IS_FEATURE_ENABLED(prWifiVar->u4ExtendedRange)) {
			/* check using the ER rate or not */
			prStaRec->fgIsExtendedRange =
			aisCheckUsingERRate(prAdapter, prBssDesc);
		}
#endif
	} else {
		DBGLOG(AIS, LOUD, "JOIN INIT: AUTH TYPE = %d for Roaming\n",
		       prAisSpecificBssInfo->ucRoamingAuthTypes);

		/* We do roaming while the medium is connected */
		prStaRec->fgIsReAssoc = TRUE;

		/* TODO(Kevin): We may call a sub function to
		 * acquire the Roaming Auth Type
		 */
		switch (prConnSettings->eAuthMode) {
		case AUTH_MODE_OPEN:
		case AUTH_MODE_WPA_PSK:
		case AUTH_MODE_WPA2_PSK:
		case AUTH_MODE_WPA3_OWE:
			if (prWpaInfo->u4WpaVersion ==
			    IW_AUTH_WPA_VERSION_DISABLED &&
			    prWpaInfo->u4AuthAlg == IW_AUTH_ALG_FT) {
				prAisFsmInfo->ucAvailableAuthTypes =
					(uint8_t) AUTH_TYPE_FT;
				DBGLOG(AIS, INFO, "FT: Non-RSN FT roaming\n");
			} else {
				/* make sure wpa3 transition mode using open */
				prAisFsmInfo->ucAvailableAuthTypes =
				    (uint8_t) AUTH_TYPE_OPEN_SYSTEM;
			}
			break;
		case AUTH_MODE_WPA2_FT:
		case AUTH_MODE_WPA2_FT_PSK:
			prAisFsmInfo->ucAvailableAuthTypes =
				(uint8_t) AUTH_TYPE_FT |
					  AUTH_TYPE_OPEN_SYSTEM;
			DBGLOG(AIS, INFO,
				"FT: eAuthMode == RSN FT roaming | OPEN\n");

			if (prBssDesc->fgIsFtOverDS) {
				if (prFtParam->eFtDsState ==
				    FT_DS_STATE_AUTHORIZED) {
					prStaRec->ucAuthTranNum =
						AUTH_TRANSACTION_SEQ_2;
				} else {
					prAisFsmInfo->ucAvailableAuthTypes &=
						~AUTH_TYPE_FT;
					DBGLOG(AIS, INFO,
						"FT: ft-over-ds not allowed\n");
				}
			}
			break;
		case AUTH_MODE_WPA3_SAE:
			if (rsnKeyMgmtFT(prBssInfo->u4RsnSelectedAKMSuite)) {
				prAisFsmInfo->ucAvailableAuthTypes =
				       (uint8_t) AUTH_TYPE_FT |
						 AUTH_TYPE_SAE;
				DBGLOG(AIS, INFO,
					"FT: eAuthMode == RSN FT roaming | SAE\n");

				if (prBssDesc->fgIsFtOverDS) {
					if (prFtParam->eFtDsState ==
					    FT_DS_STATE_AUTHORIZED) {
						prStaRec->ucAuthTranNum =
							AUTH_TRANSACTION_SEQ_2;
					} else {
						prAisFsmInfo->
							ucAvailableAuthTypes &=
							~AUTH_TYPE_FT;
						DBGLOG(AIS, INFO,
							"FT: ft-over-ds not allowed\n");
					}
				}
			} else if (!aisSearchPmkidEntry(prAdapter,
					prStaRec, ucBssIndex)) {
				prAisFsmInfo->ucAvailableAuthTypes =
					(uint8_t) AUTH_TYPE_SAE;
				DBGLOG(AIS, INFO,
					"SAE: change AUTH to SAE when roaming but PMK not found\n");
			} else {
				prAisFsmInfo->ucAvailableAuthTypes =
					(uint8_t) (AUTH_TYPE_OPEN_SYSTEM |
						   AUTH_TYPE_SAE);
				DBGLOG(AIS, INFO,
					"SAE: change AUTH to OPEN | SAE when roaming with PMK\n");
			}
			break;
#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
		case AUTH_MODE_FILS:
			if (prWpaInfo->u4AuthAlg == IW_AUTH_ALG_FILS_SK) {
				if (aisGetErpKey(prAdapter, ucBssIndex) ||
				    aisSearchPmkidEntry(prAdapter,
					prStaRec, ucBssIndex)) {
					prAisFsmInfo->ucAvailableAuthTypes =
						(uint8_t) AUTH_TYPE_FILS_SK;
					DBGLOG(AIS, INFO,
						"FILS: roaming eAuthMode == FILS\n");
				} else {
					DBGLOG(AIS, ERROR,
						"FILS: roaming failed\n");
					return;
				}
			} else {
				prAisFsmInfo->ucAvailableAuthTypes =
					(uint8_t) AUTH_TYPE_OPEN_SYSTEM;
				DBGLOG(AIS, INFO,
					"FILS: roaming eAuthMode == OPEN\n");
			}
			break;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
		default:
			prAisFsmInfo->ucAvailableAuthTypes =
			    prAisSpecificBssInfo->ucRoamingAuthTypes;
			break;
		}

		prStaRec->ucTxAuthAssocRetryLimit =
		    TX_AUTH_ASSOCI_RETRY_LIMIT_FOR_ROAMING;
	}

	/* 4 <4> Use an appropriate Authentication Algorithm
	 * Number among the ucAvailableAuthTypes
	 *
	 * Priority of ucAvailableAuthTypes for different AUTH MODE
	 * AUTO_SWITCH : SHARED_KEY -> OPEN
	 * SAE         : OPEN (with PMKID) -> SAE
	 * FT          : FT (with PMKID) -> OPEN
	 * FT-SAE      : FT (with PMKID) -> SAE
	 */
	if (prAisFsmInfo->ucAvailableAuthTypes &
	(uint8_t) AUTH_TYPE_SHARED_KEY) {

		DBGLOG(AIS, LOUD,
		       "JOIN INIT: Try to do Authentication with AuthType == SHARED_KEY.\n");

		prAisFsmInfo->ucAvailableAuthTypes &=
		    ~(uint8_t) AUTH_TYPE_SHARED_KEY;

		prStaRec->ucAuthAlgNum =
		    (uint8_t) AUTH_ALGORITHM_NUM_SHARED_KEY;
#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	} else if (prAisFsmInfo->ucAvailableAuthTypes & (uint8_t)
		   AUTH_TYPE_FILS_SK) {
		DBGLOG(AIS, LOUD,
		       "JOIN INIT: Try to do Authentication with AuthType == FILS_SK.\n");

		prAisFsmInfo->ucAvailableAuthTypes &=
		    ~(uint8_t) AUTH_TYPE_FILS_SK;

		prStaRec->ucAuthAlgNum =
		    (uint8_t) AUTH_ALGORITHM_NUM_FILS_SK;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
	} else if (prAisFsmInfo->ucAvailableAuthTypes & (uint8_t)
		   AUTH_TYPE_FT) {

		DBGLOG(AIS, LOUD,
		       "JOIN INIT: Try to do Authentication with AuthType == FAST_BSS_TRANSITION.\n");

		prAisFsmInfo->ucAvailableAuthTypes &=
		    ~(uint8_t) AUTH_TYPE_FT;

		prStaRec->ucAuthAlgNum =
		    (uint8_t) AUTH_ALGORITHM_NUM_FT;
	} else if (prAisFsmInfo->ucAvailableAuthTypes & (uint8_t)
		   AUTH_TYPE_OPEN_SYSTEM) {

		DBGLOG(AIS, LOUD,
		       "JOIN INIT: Try to do Authentication with AuthType == OPEN_SYSTEM.\n");
		prAisFsmInfo->ucAvailableAuthTypes &=
		    ~(uint8_t) AUTH_TYPE_OPEN_SYSTEM;

		prStaRec->ucAuthAlgNum =
		    (uint8_t) AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
	} else if (prAisFsmInfo->ucAvailableAuthTypes & (uint8_t)
		   AUTH_TYPE_SAE) {
		DBGLOG(AIS, LOUD,
		       "JOIN INIT: Try to do Authentication with AuthType == SAE.\n");

		prAisFsmInfo->ucAvailableAuthTypes &=
		    ~(uint8_t) AUTH_TYPE_SAE;

		prStaRec->ucAuthAlgNum =
		    (uint8_t) AUTH_ALGORITHM_NUM_SAE;
	} else {
		DBGLOG(AIS, ERROR,
		       "JOIN INIT: Unsupported auth type %d\n",
		       prAisFsmInfo->ucAvailableAuthTypes);
		return;
	}

	/* 4 <5> Overwrite Connection Setting for eConnectionPolicy
	 * == ANY (Used by Assoc Req)
	 */

	nicRxClearFrag(prAdapter, prStaRec);

	/* only setup link needs to do SAA */
	if (ucLinkIndex != 0)
		return;

	/* update fgMgmtProtection from main link only */
	aisFsmUpdateRsnSetting(prAdapter, prBssDesc, ucBssIndex);

	if (prBssDesc->ucSSIDLen)
		COPY_SSID(prConnSettings->aucSSID, prConnSettings->ucSSIDLen,
			  prBssDesc->aucSSID, prBssDesc->ucSSIDLen);

	/* 4 <6> Send a Msg to trigger SAA to start JOIN process. */
	prJoinReqMsg =
	    (struct MSG_SAA_FSM_START *)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
						    sizeof(struct
							   MSG_SAA_FSM_START));
	if (!prJoinReqMsg) {
		DBGLOG(AIS, ERROR, "Can't trigger SAA FSM\n");
		return;
	}

	prJoinReqMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_START;
	prJoinReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfReqMsg;
	prJoinReqMsg->prStaRec = prStaRec;

	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *)prJoinReqMsg,
		    MSG_SEND_METHOD_BUF);
}				/* end of aisFsmInit_JOIN() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Retry JOIN for AUTH_MODE_AUTO_SWITCH
 *
 * @param[in] prStaRec       Pointer to the STA_RECORD_T
 *
 * @retval TRUE      We will retry JOIN
 * @retval FALSE     We will not retry JOIN
 */
/*----------------------------------------------------------------------------*/
u_int8_t aisFsmStateInit_RetryJOIN(struct ADAPTER *prAdapter,
	struct STA_RECORD *prMainStaRec, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_SAA_FSM_START *prJoinReqMsg;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct STA_RECORD *prStaRec;
	uint8_t i;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	/* Retry other AuthType if possible */
	if (!prAisFsmInfo->ucAvailableAuthTypes)
		return FALSE;

	if (prMainStaRec->u2StatusCode !=
		STATUS_CODE_AUTH_ALGORITHM_NOT_SUPPORTED &&
	    prMainStaRec->u2StatusCode != STATUS_CODE_AUTH_TIMEOUT &&
	    /* try without invalid PMKID */
	    !rsnApInvalidPMK(prMainStaRec->u2StatusCode,
			     prConnSettings->eAuthMode)) {
		prAisFsmInfo->ucAvailableAuthTypes = 0;
		return FALSE;
	}

	if (prAisFsmInfo->ucAvailableAuthTypes & (uint8_t) AUTH_TYPE_SAE) {
		DBGLOG(AIS, INFO,
		       "RETRY JOIN INIT: Retry Authentication with AuthType == SAE.\n");

		prAisFsmInfo->ucAvailableAuthTypes &=
		    ~(uint8_t) AUTH_TYPE_SAE;

		for (i = 0; i < MLD_LINK_MAX; i++) {
			prStaRec = aisGetLinkStaRec(prAisFsmInfo, i);
			if (prStaRec)
				prStaRec->ucAuthAlgNum =
				    (uint8_t) AUTH_ALGORITHM_NUM_SAE;
		}
	} else if (prAisFsmInfo->ucAvailableAuthTypes &
		   (uint8_t) AUTH_TYPE_OPEN_SYSTEM) {

		DBGLOG(AIS, INFO,
		       "RETRY JOIN INIT: Retry Authentication with AuthType == OPEN_SYSTEM.\n");

		prAisFsmInfo->ucAvailableAuthTypes &=
		    ~(uint8_t) AUTH_TYPE_OPEN_SYSTEM;

		for (i = 0; i < MLD_LINK_MAX; i++) {
			prStaRec = aisGetLinkStaRec(prAisFsmInfo, i);
			if (prStaRec)
				prStaRec->ucAuthAlgNum =
				    (uint8_t) AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
		}
	} else {
		DBGLOG(AIS, ERROR,
		       "RETRY JOIN INIT: Retry Authentication with Unexpected AuthType: %d.\n",
		       prAisFsmInfo->ucAvailableAuthTypes);
		return FALSE;
	}

	/* No more available Auth Types */
	prAisFsmInfo->ucAvailableAuthTypes = 0;

	/* Trigger SAA to start JOIN process. */
	prJoinReqMsg =
	    (struct MSG_SAA_FSM_START *)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
						    sizeof(struct
							   MSG_SAA_FSM_START));
	if (!prJoinReqMsg) {
		DBGLOG(AIS, ERROR, "Can't trigger SAA FSM\n");
		return FALSE;
	}

	prJoinReqMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_START;
	prJoinReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfReqMsg;
	prJoinReqMsg->prStaRec = prMainStaRec;

	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *)prJoinReqMsg,
		    MSG_SEND_METHOD_BUF);

	return TRUE;

}				/* end of aisFsmRetryJOIN() */

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
 * @brief State Initialization of AIS_STATE_IBSS_ALONE
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateInit_IBSS_ALONE(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	/* 4 <1> Check if IBSS was created before ? */
	if (prAisBssInfo->fgIsBeaconActivated) {

	/* 4 <2> Start IBSS Alone Timer for periodic SCAN and then SEARCH */
#if !CFG_SLT_SUPPORT
		cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer,
				   SEC_TO_MSEC(AIS_IBSS_ALONE_TIMEOUT_SEC));
#endif
	}

	aisFsmCreateIBSS(prAdapter, ucBssIndex);
}				/* end of aisFsmStateInit_IBSS_ALONE() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief State Initialization of AIS_STATE_IBSS_MERGE
 *
 * @param[in] prBssDesc  The pointer of BSS_DESC_T which is the IBSS we will
 *                       try to merge with.
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateInit_IBSS_MERGE(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *)NULL;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	/* 4 <1> We will merge with to this BSS immediately. */
	prBssDesc->fgIsConnecting &= ~BIT(ucBssIndex);
	prBssDesc->fgIsConnected |= BIT(ucBssIndex);

	/* 4 <2> Setup corresponding STA_RECORD_T */
	prStaRec = bssCreateStaRecFromBssDesc(prAdapter,
					      STA_TYPE_ADHOC_PEER,
					      prAisBssInfo->ucBssIndex,
					      prBssDesc);

	if (!prStaRec) {
		DBGLOG(AIS, ERROR,
			"aisFsmStateInit_IBSS_MERGE failed because prStaRec is NULL, return.\n");
		return;
	}

	prStaRec->fgIsMerging = TRUE;

	prAisFsmInfo->prTargetStaRec = prStaRec;

	/* 4 <2.1> sync. to firmware domain */
	cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

	/* 4 <3> IBSS-Merge */
	aisFsmMergeIBSS(prAdapter, prStaRec);
}				/* end of aisFsmStateInit_IBSS_MERGE() */

#endif /* CFG_SUPPORT_ADHOC */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process of JOIN Abort
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_JOIN(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_SAA_FSM_ABORT *prJoinAbortMsg;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* 1. Abort JOIN process */
	prJoinAbortMsg =
	    (struct MSG_SAA_FSM_ABORT *)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
						    sizeof(struct
							   MSG_SAA_FSM_ABORT));
	if (!prJoinAbortMsg) {
		DBGLOG(AIS, ERROR, "Can't abort SAA FSM\n");
		return;
	}

	prJoinAbortMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_ABORT;
	prJoinAbortMsg->ucSeqNum = prAisFsmInfo->ucSeqNumOfReqMsg;
	prJoinAbortMsg->prStaRec = aisGetTargetStaRec(prAdapter, ucBssIndex);

	aisTargetBssResetConnected(prAdapter, prAisFsmInfo);
	aisTargetBssResetConnecting(prAdapter, prAisFsmInfo);

	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *)prJoinAbortMsg,
		    MSG_SEND_METHOD_BUF);

	/* 2. Return channel privilege */
	aisFsmReleaseCh(prAdapter, ucBssIndex);

	/* 3.1 stop join timeout timer */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
}				/* end of aisFsmAbortJOIN() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process of SCAN Abort for all ais bss
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_SCAN_All(struct ADAPTER *prAdapter)
{
	u_int8_t ucIdx = 0;

	for (ucIdx = 0; ucIdx < KAL_AIS_NUM; ucIdx++) {
		if (prAdapter->rWifiVar.rScanInfo.eCurrentState
			== SCAN_STATE_SCANNING) {
			aisFsmStateAbort_SCAN(prAdapter, ucIdx);
		}
	}
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Process of SCAN Abort
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_SCAN(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_SCN_SCAN_CANCEL *prScanCancelMsg;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	DBGLOG(AIS, STATE, "[%d] aisFsmStateAbort_SCAN\n",
		ucBssIndex);

	/* Abort JOIN process. */
	prScanCancelMsg =
	    (struct MSG_SCN_SCAN_CANCEL *)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct MSG_SCN_SCAN_CANCEL));
	if (!prScanCancelMsg) {
		DBGLOG(AIS, ERROR, "Can't abort SCN FSM\n");
		return;
	}
	kalMemZero(prScanCancelMsg, sizeof(struct MSG_SCN_SCAN_CANCEL));
	prScanCancelMsg->rMsgHdr.eMsgId = MID_AIS_SCN_SCAN_CANCEL;
	prScanCancelMsg->ucSeqNum = prAisFsmInfo->ucSeqNumOfScanReq;
	prScanCancelMsg->ucBssIndex = ucBssIndex;
	prScanCancelMsg->fgIsChannelExt = FALSE;
	if (prAisFsmInfo->fgIsScanOidAborted) {
		prScanCancelMsg->fgIsOidRequest = TRUE;
		prAisFsmInfo->fgIsScanOidAborted = FALSE;
	}

	/* unbuffered message to guarantee scan is cancelled in sequence */
	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *)prScanCancelMsg,
		    MSG_SEND_METHOD_UNBUF);
}				/* end of aisFsmAbortSCAN() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process of NORMAL_TR Abort
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_NORMAL_TR(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* TODO(Kevin): Do abort other MGMT func */

	/* 1. Release channel to CNM */
	aisFsmReleaseCh(prAdapter, ucBssIndex);

	/* 2.1 stop join timeout timer */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
}				/* end of aisFsmAbortNORMAL_TR() */

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
 * @brief Process of NORMAL_TR Abort
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_IBSS(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_DESC *prBssDesc;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* reset BSS-DESC */
	if (prAisFsmInfo->prTargetStaRec) {
		prBssDesc =
		    scanSearchBssDescByTA(prAdapter,
					  prAisFsmInfo->
					  prTargetStaRec->aucMacAddr);

		if (prBssDesc) {
			prBssDesc->fgIsConnected &= ~BIT(ucBssIndex);
			prBssDesc->fgIsConnecting &= ~BIT(ucBssIndex);
		}
	}
	/* release channel privilege */
	aisFsmReleaseCh(prAdapter, ucBssIndex);
}
#endif /* CFG_SUPPORT_ADHOC */

static u_int8_t
aisState_OFF_CHNL_TX(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_MGMT_TX_REQ_INFO *prMgmtTxInfo =
			(struct AIS_MGMT_TX_REQ_INFO *) NULL;
	struct AIS_OFF_CHNL_TX_REQ_INFO *prOffChnlTxPkt =
			(struct AIS_OFF_CHNL_TX_REQ_INFO *) NULL;
	struct AIS_FSM_INFO *prAisFsmInfo =
		aisGetAisFsmInfo(prAdapter, ucBssIndex);

	prMgmtTxInfo = &prAisFsmInfo->rMgmtTxInfo;

	if (LINK_IS_EMPTY(&(prMgmtTxInfo->rTxReqLink))) {
		cnmTimerStopTimer(prAdapter,
				&prAisFsmInfo->rChannelTimeoutTimer);
		aisFsmReleaseCh(prAdapter, ucBssIndex);
		/* Link is empty, return back to IDLE. */
		return FALSE;
	}

	prOffChnlTxPkt =
		LINK_PEEK_HEAD(&(prMgmtTxInfo->rTxReqLink),
				struct AIS_OFF_CHNL_TX_REQ_INFO,
				rLinkEntry);

	if (prOffChnlTxPkt == NULL) {
		DBGLOG(AIS, ERROR,
			"Fatal Error, Link not empty but get NULL pointer.\n");
		aisFsmReleaseCh(prAdapter, ucBssIndex);
		return FALSE;
	}

	aisFuncTxMgmtFrame(prAdapter,
			prMgmtTxInfo,
			prOffChnlTxPkt->prMgmtTxMsdu,
			prOffChnlTxPkt->u8Cookie,
			ucBssIndex);
	LINK_REMOVE_HEAD(&(prAisFsmInfo->rMgmtTxInfo.rTxReqLink),
			prOffChnlTxPkt,
			struct AIS_OFF_CHNL_TX_REQ_INFO *);
	cnmMemFree(prAdapter, prOffChnlTxPkt);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Remove roaming requests including search and connect
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRemoveRoamingRequest(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	/* clear pending roaming connection request */
	aisFsmClearRequest(prAdapter, AIS_REQUEST_ROAMING_SEARCH, ucBssIndex);
	aisFsmClearRequest(prAdapter, AIS_REQUEST_ROAMING_CONNECT, ucBssIndex);
}

struct BSS_DESC *aisSearchBssDescByScore(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex,
	struct BSS_DESC_SET *set)
{
#if CFG_SUPPORT_ROAMING
	struct ROAMING_INFO *roam = aisGetRoamingInfo(prAdapter, ucBssIndex);
#endif
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* don't use cached scan list for BTO/deauth at 1st trial */
	if (aisFsmIsInProcessPostpone(prAdapter, ucBssIndex) &&
		ais->ucConnTrialCount == 0)
		return NULL;
	else
		return apsSearchBssDescByScore(prAdapter,
#if CFG_SUPPORT_ROAMING
			roam->eReason,
#else
			ROAMING_REASON_POOR_RCPI,
#endif
			ucBssIndex, set);
}

uint8_t aisNeedTargetScan(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *ais = NULL;
	struct BSS_INFO *bss = NULL;
#if CFG_SUPPORT_ROAMING
	struct ROAMING_INFO *roam = NULL;
#endif
	uint8_t discovering = FALSE;
	uint8_t postponing = FALSE;
	uint8_t issued = FALSE;
	uint8_t trial = 0;

	ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	bss = aisGetAisBssInfo(prAdapter, ucBssIndex);
#if CFG_SUPPORT_ROAMING
	roam = aisGetRoamingInfo(prAdapter, ucBssIndex);
	if (!ais || !bss || !roam) {
		DBGLOG(AIS, ERROR,
			"ERR! Null access! ais=%p, bss=%p, roam=%p\n",
			ais, bss, roam);
		return FALSE;
	}
	discovering =
		bss->eConnectionState == MEDIA_STATE_CONNECTED &&
		(roam->eCurrentState == ROAMING_STATE_DISCOVERY ||
		roam->eCurrentState == ROAMING_STATE_ROAM);

	if (discovering) {
		if (roam->rRoamScanParam.ucScanType ==
					ROAMING_SCAN_TYPE_PARTIAL_ONLY)
			return TRUE;
		else if (roam->rRoamScanParam.ucScanType ==
					ROAMING_SCAN_TYPE_FULL_ONLY)
			return FALSE;
	}
#endif
	postponing = aisFsmIsInProcessPostpone(prAdapter, ucBssIndex);
	trial = ais->ucConnTrialCount;

#if CFG_EXT_ROAMING_WTC
	aisWtcNeedPartialScan(
		prAdapter,
		ucBssIndex,
		&issued);
#endif
#if CFG_SUPPORT_NCHO
	issued = ais->fgTargetChnlScanIssued ||
		 prAdapter->rNchoInfo.u4RoamScanControl;
#else
	issued = ais->fgTargetChnlScanIssued;
#endif
#if CFG_SUPPORT_LLW_SCAN
	/* If CRT DATA is 2, prohibit full roaming scan
	 * even if fgTargetChnlScanIssued is FALSE.
	 */
	if (ais->ucLatencyCrtDataMode == 2)
		issued = TRUE;
#endif

	/* For OCE certification, we need to perform full scan in all cases*/
	if (prAdapter->rWifiVar.u4SwTestMode == ENUM_SW_TEST_MODE_SIGMA_OCE)
		return FALSE;

	return (discovering && issued) ||
		(postponing && trial < AIS_ROAMING_CONNECTION_TRIAL_LIMIT);
}

void aisFillBssInfoFromBssDesc(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	struct BSS_DESC_SET *prBssDescSet)
{
	uint8_t i;
	struct BSS_INFO *prMainBss;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct GL_WPA_INFO *prWpaInfo;
	struct BSS_INFO *prAisBssInfo;
	struct BSS_DESC *prBssDesc;
#if CFG_SUPPORT_DBDC
	struct DBDC_DECISION_INFO rDbdcDecisionInfo = {0};
#endif

	prConnSettings = &prAisFsmInfo->rConnSettings;
	prWpaInfo = &prAisFsmInfo->rWpaInfo;
	/* main bss must assign wmm first */
	prMainBss = aisGetMainLinkBssInfo(prAisFsmInfo);
	cnmWmmIndexDecision(prAdapter, prMainBss);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	/* update cap again in case bssinfo is already registered */
	mldBssUpdateCap(prAdapter, prAisFsmInfo->prMldBssInfo, prBssDescSet);
#endif

	for (i = 0; i < MLD_LINK_MAX; i++) {
		prAisBssInfo = aisGetLinkBssInfo(prAisFsmInfo, i);
		prBssDesc = prBssDescSet->aprBssDesc[i];

		/* prBssDesc can be null if roam from mld to legacy */
		aisSetLinkBssDesc(prAisFsmInfo, prBssDesc, i);
		aisSetLinkStaRec(prAisFsmInfo, NULL, i);

		if (!prBssDesc)
			continue;

		if (!prAisBssInfo) {
			prAisBssInfo =
				aisAllocBssInfo(prAdapter, prAisFsmInfo, i);
			if (!prAisBssInfo) {
				aisSetLinkBssDesc(prAisFsmInfo, NULL, i);
				continue;
			}
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			mldBssRegister(prAdapter, prAisFsmInfo->prMldBssInfo,
				       prAisBssInfo);
#endif
		}

		prConnSettings->eAuthMode = prBssDesc->eRsnSelectedAuthMode;
		prWpaInfo->u4WpaVersion = prBssDesc->u4RsnSelectedProto;
		prAisBssInfo->u4RsnSelectedGroupCipher =
			prBssDesc->u4RsnSelectedGroupCipher;
		prAisBssInfo->u4RsnSelectedPairwiseCipher =
			prBssDesc->u4RsnSelectedPairwiseCipher;
		prAisBssInfo->u4RsnSelectedGroupMgmtCipher =
			prBssDesc->u4RsnSelectedGroupMgmtCipher;
		prAisBssInfo->u4RsnSelectedAKMSuite =
			prBssDesc->u4RsnSelectedAKMSuite;
		prAisBssInfo->eBand = prBssDesc->eBand;

		/* backup and reset grant NSS/BW */
		prAisBssInfo->ucBackupGrantTxNss = prAisBssInfo->ucGrantTxNss;
		prAisBssInfo->ucGrantTxNss = 0;
		prAisBssInfo->ucBackupGrantRxNss = prAisBssInfo->ucGrantRxNss;
		prAisBssInfo->ucGrantRxNss = 0;
		prAisBssInfo->ucBackupGrantBW = prAisBssInfo->ucGrantBW;
		prAisBssInfo->ucGrantBW = MAX_BW_UNKNOWN;

#if CFG_SUPPORT_REPLAY_DETECTION
		kalMemZero(&prAisBssInfo->rDetRplyInfo,
			sizeof(struct GL_DETECT_REPLAY_INFO));
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		/* connac3 MLO all bss use the same wmm index as main bss use */
		prAisBssInfo->fgIsWmmInited = TRUE;
		prAisBssInfo->ucWmmQueSet = prMainBss->ucWmmQueSet;
#else
		/* connac2 always assign different wmm index to bssinfo */
		cnmWmmIndexDecision(prAdapter, prAisBssInfo);
#endif

		prAisBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;

#if CFG_SUPPORT_DBDC
		CNM_DBDC_ADD_DECISION_INFO(rDbdcDecisionInfo,
			prAisBssInfo->ucBssIndex,
			prBssDesc->eBand,
			prBssDesc->ucChannelNum,
			prAisBssInfo->ucWmmQueSet);
#endif
		DBGLOG(AIS, INFO, "[%d] mac: " MACSTR ", band: %d, ch: %d, wmm: %d\n",
			i,
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->eBand,
			prBssDesc->ucChannelNum,
			prAisBssInfo->ucWmmQueSet);
	}

#if CFG_SUPPORT_DBDC
	/* DBDC decsion.may change OpNss */
	if (cnmDbdcIsDisabled(prAdapter))
		cnmDbdcPreConnectionEnableDecision(
				prAdapter,
				&rDbdcDecisionInfo);
#endif /*CFG_SUPPORT_DBDC*/
}

uint8_t aisBssDescAllowed(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	struct BSS_DESC_SET *prBssDescSet)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t i, j, match = 0;

	prConnSettings = &prAisFsmInfo->rConnSettings;

	if (prBssDescSet->ucLinkNum == 0)
		return FALSE;

	/* if the connection policy is BSSID/BSSID_HINT, means upper layer
	 * order driver connect to specific AP, we need still do connect
	 */
	if ((prConnSettings->eConnectionPolicy == CONNECT_BY_BSSID &&
	     prBssDescSet->fgIsMatchBssid) ||
	    (prConnSettings->eConnectionPolicy == CONNECT_BY_BSSID_HINT &&
	     prBssDescSet->fgIsMatchBssidHint))
		return TRUE;

	if (prBssDescSet->ucLinkNum != aisGetLinkNum(prAisFsmInfo))
		return TRUE;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_DESC *prBssDesc = aisGetLinkBssDesc(prAisFsmInfo, i);
		struct STA_RECORD *prStaRec = aisGetLinkStaRec(prAisFsmInfo, i);

		if (!prBssDesc || !prStaRec)
			continue;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		/* skip if ap is removed and re-added*/
		if (prStaRec->fgApRemoval)
			continue;
#endif

		for (j = 0; j < prBssDescSet->ucLinkNum; j++) {
			if (prBssDesc == prBssDescSet->aprBssDesc[j]) {
				match++;
				break;
			}
		}
	}

	/* allow when different combination */
	return match != prBssDescSet->ucLinkNum;
}

#if CFG_SUPPORT_ROAMING
enum ENUM_AIS_STATE aisSearchHandleBadBssDesc(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prBssDescSet, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *ais;
	enum ENUM_AIS_STATE state = AIS_STATE_NORMAL_TR;
	struct BSS_DESC *prBssDesc = prBssDescSet->prMainBssDesc;
	struct BSS_TRANSITION_MGT_PARAM *btm;
	struct ROAMING_INFO *roam;
	struct BSS_INFO *aisBssInfo;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	aisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	roam = aisGetRoamingInfo(prAdapter, ucBssIndex);
	btm =  aisGetBTMParam(prAdapter, ucBssIndex);

	if (prBssDesc) {
		DBGLOG(ROAMING, INFO,
			"fgIsConnected=%d, prBssDesc " MACSTR
			", ucBssIndex=%d\n",
			prBssDesc->fgIsConnected,
			MAC2STR(prBssDesc->aucBSSID),
			ucBssIndex);
	}

	roamingFsmRunEventNewCandidate(prAdapter, NULL, ucBssIndex);

	if (roam->eReason == ROAMING_REASON_BTM &&
	    btm->ucDisImmiState == AIS_BTM_DIS_IMMI_STATE_2) {
		ais->u4SleepInterval =
			btm->u4ReauthDelay > prWifiVar->u4BtmDisThreshold ?
			btm->u4ReauthDelay - prWifiVar->u4BtmDisThreshold :
			AIS_BG_SCAN_INTERVAL_MSEC;

		if (ais->u4SleepInterval > prWifiVar->u4BtmTimerThreshold) {
			cnmTimerStopTimer(prAdapter,
				aisGetBTMDisassocTimer(prAdapter, ucBssIndex));

			cnmTimerStartTimer(prAdapter,
				aisGetBTMDisassocTimer(prAdapter, ucBssIndex),
				ais->u4SleepInterval);
		} else {
			ais->fgTargetChnlScanIssued = TRUE;
			state = AIS_STATE_WAIT_FOR_NEXT_SCAN;
			goto skip_roam_fail;
		}
	} else if (roam->eReason == ROAMING_REASON_BTM &&
		   btm->ucDisImmiState == AIS_BTM_DIS_IMMI_STATE_3) {
		if (ais->fgTargetChnlScanIssued) {
			ais->fgTargetChnlScanIssued = FALSE;
			state = AIS_STATE_LOOKING_FOR;
			goto skip_roam_fail;
		}

		DBGLOG(AIS, INFO,
			"[Roaming] No target found in BTM state 3.\n");
	} else if (roam->rRoamScanParam.ucScanCount) {
		if (ais->ucScanTrialCount >=
				roam->rRoamScanParam.ucScanCount) {
			DBGLOG(ROAMING, STATE,
				"Roaming scan retry :%d fail!\n",
				ais->ucScanTrialCount);
			roamingFsmRunEventNewCandidate(prAdapter,
				NULL, ucBssIndex);
			roamingFsmRunEventFail(prAdapter,
				ROAMING_FAIL_REASON_NOCANDIDATE,
				ucBssIndex);

			/* reset retry count */
			ais->ucConnTrialCount = 0;
			state = AIS_STATE_NORMAL_TR;
			goto skip_roam_fail;
		} else {
			DBGLOG(ROAMING, INFO,
				"Didn't reach scan limit %d < %d, try to scan again\n",
				ais->ucScanTrialCount,
				roam->rRoamScanParam.ucScanCount);
			ais->ucScanTrialCount++;
			state = AIS_STATE_LOOKING_FOR;
			goto skip_roam_fail;
		}
	} else if (ais->fgTargetChnlScanIssued) {
		/* if target channel scan has issued, and no
		 * roaming target is found, need to do full scan
		 */
		DBGLOG(AIS, INFO,
		       "[Roaming] No target found, try to full scan again\n");
		ais->fgTargetChnlScanIssued = FALSE;
		state = AIS_STATE_LOOKING_FOR;
		goto skip_roam_fail;
	}

	roamingFsmRunEventFail(prAdapter,
		ROAMING_FAIL_REASON_NOCANDIDATE, ucBssIndex);

skip_roam_fail:
	/* We already associated with it go back to NORMAL_TR */
	return state;
}
#endif /* CFG_SUPPORT_ROAMING */

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint8_t aisSecondLinkAvailable(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MLD_BSS_INFO *prMldBssInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prMldBssInfo = prAisFsmInfo->prMldBssInfo;

#if CFG_SUPPORT_NAN && !CFG_MLO_CONCURRENT_NAN
	if (prAdapter->fgIsNANRegistered)
		return FALSE;
#endif

	return mldBssAllowReconfig(prAdapter, prMldBssInfo);
}

struct MLD_BSS_INFO *aisGetMldBssInfo(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	return aisGetAisFsmInfo(prAdapter, ucBssIndex)->prMldBssInfo;
}

struct MLD_STA_RECORD *aisGetMldStaRec(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct MLD_BSS_INFO *prMldBssInfo =
		aisGetMldBssInfo(prAdapter, ucBssIndex);

	return mldBssGetPeekClient(prAdapter, prMldBssInfo);
}

uint8_t aisNeedMloScan(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prBssDescSet, uint8_t ucBssIndex)
{
	struct BSS_DESC *prBssDesc = prBssDescSet->prMainBssDesc;
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* over retry limit, no need mlo scan */
	if (prAisFsmInfo->ucMlProbeSendCount >=
		prAdapter->rWifiVar.ucMlProbeRetryLimit)
		return FALSE;

	if (!mldIsMultiLinkEnabled(prAdapter, NETWORK_TYPE_AIS, ucBssIndex) ||
	    !aisSecondLinkAvailable(prAdapter, ucBssIndex))
		return FALSE;

	/* already found multi link, no need mlo scan */
	if (prBssDescSet->ucLinkNum != 1)
		return FALSE;

	/* target is not mlo, no need mlo scan */
	if (!prBssDesc->rMlInfo.fgValid ||
	    !prBssDesc->rMlInfo.ucMaxSimuLinks)
		return FALSE;

	return TRUE;
}
#endif

enum ENUM_AIS_STATE aisSearchHandleBssDesc(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prBssDescSet, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	/* 4 <2> We are not under Roaming Condition. */
	if (prAisBssInfo->eConnectionState == MEDIA_STATE_DISCONNECTED) {
		/* 4 <2.a> If we have the matched one */
		if (prBssDescSet->ucLinkNum > 0) {
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			/* If target connected AP has MultiLink
			 * (ucMaxSimuLinks > 0, 0 means only 1 device),
			 * but we only scan one link(ucLinkNum=1), need to send
			 * ML probe request to get completed ML info first.
			 */
			if (aisNeedMloScan(prAdapter,
					prBssDescSet, ucBssIndex)) {
				prAisFsmInfo->ucMlProbeSendCount++;
				prAisFsmInfo->ucMlProbeEnable = TRUE;
				prAisFsmInfo->prMlProbeBssDesc =
					prBssDescSet->aprBssDesc[0];
				return AIS_STATE_LOOKING_FOR;
			}
#endif

			/* Stored the Selected BSS security cipher. or
			 * later asoc req compose IE
			 */

			aisFillBssInfoFromBssDesc(prAdapter,
				prAisFsmInfo, prBssDescSet);

			/* If target connected AP does not have
			 * MultiLink or already scan 2 links, directly
			 * request channel
			 */
			prAisFsmInfo->ucConnTrialCount++;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			prAisFsmInfo->ucMlProbeSendCount = 0;
			prAisFsmInfo->ucMlProbeEnable = FALSE;
			prAisFsmInfo->prMlProbeBssDesc = NULL;
#endif
			return AIS_STATE_REQ_CHANNEL_JOIN;
		} else {
			/* 4 <2.b> If we don't have the matched one */
			if (prAisFsmInfo->rJoinReqTime != 0 &&
			    CHECK_FOR_TIMEOUT(kalGetTimeTick(),
				prAisFsmInfo->rJoinReqTime,
				SEC_TO_SYSTIME(AIS_JOIN_TIMEOUT))) {
				return AIS_STATE_JOIN_FAILURE;
			}
			return aisFsmStateSearchAction(prAdapter, ucBssIndex);
		}
	} else {
#if CFG_SUPPORT_ROAMING
		/* 4 <3> We are under Roaming Condition. */
		if (prAisFsmInfo->ucConnTrialCount >
		    AIS_ROAMING_CONNECTION_TRIAL_LIMIT) {
			DBGLOG(AIS, STATE, "Roaming retry :%d fail!\n",
			       prAisFsmInfo->ucConnTrialCount);
			roamingFsmRunEventNewCandidate(prAdapter,
				NULL, ucBssIndex);
			roamingFsmRunEventFail(prAdapter,
				ROAMING_FAIL_REASON_CONNLIMIT, ucBssIndex);

			/* reset retry count */
			prAisFsmInfo->ucConnTrialCount = 0;
			/* DISCONNECT_REASON_CODE_ROAMING is triggered by
			 * supplicant, must indicate the connection status,
			 */
			if (prAisFsmInfo->ucReasonOfDisconnect ==
			    DISCONNECT_REASON_CODE_ROAMING) {
				aisIndicationOfMediaStateToHost(
					prAdapter,
					MEDIA_STATE_CONNECTED,
					FALSE,
					ucBssIndex);
			}

			return AIS_STATE_NORMAL_TR;
		}

		if (!aisBssDescAllowed(prAdapter,
				prAisFsmInfo, prBssDescSet)) {
			/* roaming triggered by user space */
			if (prAisFsmInfo->ucReasonOfDisconnect ==
				DISCONNECT_REASON_CODE_REASSOCIATION ||
			    prAisFsmInfo->ucReasonOfDisconnect ==
				DISCONNECT_REASON_CODE_ROAMING ||
			    prAisFsmInfo->ucReasonOfDisconnect ==
				DISCONNECT_REASON_CODE_TEST_MODE)
				return aisFsmStateSearchAction(prAdapter,
					ucBssIndex);
			else
				return aisSearchHandleBadBssDesc(prAdapter,
					prBssDescSet, ucBssIndex);

		}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		/* If target connected AP has MultiLink
		 * (ucMaxSimuLinks > 0, 0 means only 1 device),
		 * but we only scan one link(ucLinkNum=1), need to send
		 * ML probe request to get completed ML info first.
		 */
		if (aisNeedMloScan(prAdapter,
				prBssDescSet, ucBssIndex)) {
			prAisFsmInfo->ucMlProbeSendCount++;
			prAisFsmInfo->ucMlProbeEnable = TRUE;
			prAisFsmInfo->prMlProbeBssDesc =
				prBssDescSet->aprBssDesc[0];
			return AIS_STATE_LOOKING_FOR;
		}
#endif

		return AIS_STATE_ROAMING;
#else
		return AIS_STATE_NORMAL_TR;
#endif /* CFG_SUPPORT_ROAMING */
	}
}

#if (CFG_SUPPORT_ANDROID_DUAL_STA == 1)
void aisSendChipConfigCmd(struct ADAPTER *prAdapter, char *aucCmd)
{
	struct CMD_CHIP_CONFIG rCmdChipConfig;

	kalMemZero(&rCmdChipConfig, sizeof(rCmdChipConfig));
	rCmdChipConfig.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
	rCmdChipConfig.u2MsgSize = kalStrnLen(aucCmd, WLAN_CFG_VALUE_LEN_MAX);
	kalStrnCpy(rCmdChipConfig.aucCmd, aucCmd, WLAN_CFG_VALUE_LEN_MAX);

	wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_CHIP_CONFIG,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			sizeof(struct CMD_CHIP_CONFIG),
			(uint8_t *) &rCmdChipConfig,
			NULL,
			0);
}

void aisMultiStaSetQuoteTime(struct ADAPTER *prAdapter, uint8_t fgSetQuoteTime)
{
	uint8_t aucEnableCnmDualSTA[20];

	kalMemZero(aucEnableCnmDualSTA, sizeof(aucEnableCnmDualSTA));
	kalSnprintf(aucEnableCnmDualSTA, sizeof(aucEnableCnmDualSTA),
		"EnableCnmDualSTA %d", fgSetQuoteTime ? 1 : 0);
	aisSendChipConfigCmd(prAdapter, aucEnableCnmDualSTA);

	if (fgSetQuoteTime) {
		uint8_t aucWlanQuoteTime[40];

		kalMemZero(aucWlanQuoteTime, sizeof(aucWlanQuoteTime));
		kalSnprintf(aucWlanQuoteTime, sizeof(aucWlanQuoteTime),
			"MccDualStaAIS0QuotaTimeInUs %d",
			prAdapter->u4MultiStaPrimaryInterface ==
			AIS_DEFAULT_INDEX ? 300000 : 120000);
		aisSendChipConfigCmd(prAdapter, aucWlanQuoteTime);

		kalMemZero(aucWlanQuoteTime, sizeof(aucWlanQuoteTime));
		kalSnprintf(aucWlanQuoteTime, sizeof(aucWlanQuoteTime),
			"MccDualStaAIS1QuotaTimeInUs %d",
			prAdapter->u4MultiStaPrimaryInterface ==
			AIS_SECONDARY_INDEX ? 300000 : 120000);
		aisSendChipConfigCmd(prAdapter, aucWlanQuoteTime);
	}
}

void aisCheckMultiStaStatus(struct ADAPTER *prAdapter,
	enum ENUM_PARAM_MEDIA_STATE eState, uint8_t ucBssIndex)
{
	struct BSS_INFO *prInspectBss = NULL;
	uint8_t ucInspectBssIndex;

	if (ucBssIndex >= KAL_AIS_NUM)
		return;

	switch (eState) {
	case MEDIA_STATE_CONNECTED:
		ucInspectBssIndex = ucBssIndex == AIS_DEFAULT_INDEX ?
			AIS_SECONDARY_INDEX : AIS_DEFAULT_INDEX;
		prInspectBss = aisGetAisBssInfo(prAdapter, ucInspectBssIndex);
		if (!prInspectBss)
			return;

		if (prInspectBss->eConnectionState == MEDIA_STATE_CONNECTED) {
			prAdapter->ucIsMultiStaConnected = TRUE;
			/* Both AIS connected */
			if (prAdapter->u4MultiStaUseCase ==
				WIFI_DUAL_STA_TRANSIENT_PREFER_PRIMARY)
				aisMultiStaSetQuoteTime(prAdapter, TRUE);
		}
		break;
	case MEDIA_STATE_DISCONNECTED:
		if (prAdapter->ucIsMultiStaConnected) {
			prAdapter->ucIsMultiStaConnected = FALSE;
			if (prAdapter->u4MultiStaUseCase ==
				WIFI_DUAL_STA_TRANSIENT_PREFER_PRIMARY)
				aisMultiStaSetQuoteTime(prAdapter, FALSE);
		}
		break;
	default:
		break;
	}
}
#endif

u_int8_t aisScanChannelFixed(struct ADAPTER *prAdapter, enum ENUM_BAND *prBand,
	uint8_t *pucPrimaryChannel, uint8_t ucBssIndex)
{
	struct CONNECTION_SETTINGS *setting;
	struct AIS_FSM_INFO *ais;

	ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	setting = aisGetConnSettings(prAdapter, ucBssIndex);
	if (ais->eCurrentState == AIS_STATE_LOOKING_FOR &&
	    setting->eConnectionPolicy == CONNECT_BY_BSSID &&
	    setting->u4FreqInMHz != 0) {
		*pucPrimaryChannel =
			nicFreq2ChannelNum(setting->u4FreqInMHz * 1000);
		if (*pucPrimaryChannel > 0) {
			if ((setting->u4FreqInMHz >= 2412) &&
				(setting->u4FreqInMHz <= 2484))
				*prBand = BAND_2G4;
			else if ((setting->u4FreqInMHz >= 5180) &&
				(setting->u4FreqInMHz <= 5900))
				*prBand = BAND_5G;
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if ((setting->u4FreqInMHz >= 5955) &&
				(setting->u4FreqInMHz <= 7115))
				*prBand = BAND_6G;
#endif

			DBGLOG(AIS, INFO, "fixed channel %d, band %d\n",
				*pucPrimaryChannel, *prBand);
			return TRUE;
		}
	} else {
		return cnmAisInfraChannelFixed(prAdapter,
				prBand, pucPrimaryChannel);
	}
	return FALSE;
}

static uint8_t aisFsmUpdateRsnSetting(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBss, uint8_t ucBssIndex)
{
	enum ENUM_PARAM_AUTH_MODE eAuthMode;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;

	eAuthMode = aisGetAuthMode(prAdapter, ucBssIndex);
	prAisSpecificBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);

#if CFG_SUPPORT_PASSPOINT
	if (eAuthMode == AUTH_MODE_WPA_OSEN) {
		aisGetConnSettings(prAdapter, ucBssIndex)
			->fgAuthOsenWithRSN = prBss->fgIERSN;
		DBGLOG(AIS, INFO, "OSEN: OSEN=%d, RSN=%d\n",
			prBss->fgIEOsen, prBss->fgIERSN);
	}
#endif


#if CFG_SUPPORT_802_11W
	prAisSpecificBssInfo->fgMgmtProtection = !!prBss->u4RsnSelectedPmf;

	DBGLOG(AIS, INFO,
	       "setting=%d, MgmtProtection = %d\n",
	       kalGetMfpSetting(prAdapter->prGlueInfo, ucBssIndex),
	       prAisSpecificBssInfo->fgMgmtProtection);
#endif

	return TRUE;
}

void aisFsmSyncScanReq(struct ADAPTER *prAdapter,
	struct PARAM_SCAN_REQUEST_ADV *prScanRequestIn,
	uint8_t ucBssIndex)
{
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest =
		aisGetScanReq(prAdapter, ucBssIndex);

	kalMemCopy(prScanRequest, prScanRequestIn,
		sizeof(struct PARAM_SCAN_REQUEST_ADV));
	wlanClearScanningResult(prAdapter, ucBssIndex);
}

enum ENUM_AIS_STATE aisFsmHandleNextReq_NORMAL_TR(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo, uint8_t ucBssIndex)
{
	enum ENUM_AIS_STATE eNextState = prAisFsmInfo->eCurrentState;
	struct AIS_REQ_HDR *prAisReq = NULL;
	struct AIS_SCAN_REQ *prAisScanReq;
	struct AIS_CSA_REQ *prAisCsaReq;

	if (aisFsmIsRequestPending(prAdapter,
		AIS_REQUEST_BTO, TRUE, &prAisReq, ucBssIndex)) {
		aisHandleBeaconTimeout(prAdapter, ucBssIndex, TRUE);
		cnmMemFree(prAdapter, prAisReq);
	} else if (aisFsmIsRequestPending(prAdapter,
		AIS_REQUEST_ROAMING_SEARCH, TRUE, &prAisReq, ucBssIndex)) {
		eNextState = AIS_STATE_LOOKING_FOR;
		cnmMemFree(prAdapter, prAisReq);
	} else if (aisFsmIsRequestPending(prAdapter,
		AIS_REQUEST_ROAMING_CONNECT, TRUE, &prAisReq, ucBssIndex)) {
		eNextState = AIS_STATE_SEARCH;
		cnmMemFree(prAdapter, prAisReq);
	} else if (aisFsmIsRequestPending(prAdapter,
		AIS_REQUEST_SCAN, TRUE, &prAisReq, ucBssIndex)) {
		prAisScanReq = (struct AIS_SCAN_REQ *)prAisReq;
		aisFsmSyncScanReq(prAdapter, &prAisScanReq->rScanRequest,
				  ucBssIndex);
		eNextState = AIS_STATE_ONLINE_SCAN;
		cnmMemFree(prAdapter, prAisReq);
	} else if (aisFsmIsRequestPending(prAdapter,
		AIS_REQUEST_REMAIN_ON_CHANNEL, TRUE, &prAisReq, ucBssIndex)) {
		eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;
		cnmMemFree(prAdapter, prAisReq);
	} else if (aisFsmIsRequestPending(prAdapter,
		AIS_REQUEST_CSA, TRUE, &prAisReq, ucBssIndex)) {
		prAisCsaReq = (struct AIS_CSA_REQ *)prAisReq;
		aisFunSwitchChannelImpl(prAdapter, prAisCsaReq->ucBssIndex);
		cnmMemFree(prAdapter, prAisReq);
	}

	return eNextState;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief The Core FSM engine of AIS(Ad-hoc, Infra STA)
 *
 * @param[in] eNextState Enum value of next AIS STATE
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmSteps(struct ADAPTER *prAdapter,
	enum ENUM_AIS_STATE eNextState, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct MSG_CH_REQ *prMsgChReq;
	struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg;
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest;
	struct AIS_REQ_HDR *prAisReq;
	struct AIS_SCAN_REQ *prAisScanReq;
	u_int8_t fgIsTransition = (u_int8_t) FALSE;
	uint8_t i;
	enum ENUM_AIS_STATE eNewState;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisSpecificBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	do {

		/* Do entering Next State */
		prAisFsmInfo->ePreviousState = prAisFsmInfo->eCurrentState;

		DBGLOG(AIS, STATE, "[AIS%d][%d] TRANSITION: [%s] -> [%s]\n",
			prAisFsmInfo->ucAisIndex, ucBssIndex,
			aisGetFsmState(prAisFsmInfo->eCurrentState),
			aisGetFsmState(eNextState));

		/* NOTE(Kevin): This is the only place to change the
		 * eCurrentState(except initial)
		 */
		prAisFsmInfo->eCurrentState = eNextState;

		fgIsTransition = (u_int8_t) FALSE;

		aisPostponedEventOfDisconnTimeout(prAdapter, ucBssIndex);

		/* Do tasks of the State that we just entered */
		switch (prAisFsmInfo->eCurrentState) {
		/* NOTE(Kevin): we don't have to rearrange the
		 * sequence of following switch case. Instead
		 * I would like to use a common lookup table of array
		 * of function pointer to speed up state search.
		 */
		case AIS_STATE_IDLE:
			/* recycle unused bssinfo */
			aisFreeAllBssInfo(prAdapter, prAisFsmInfo, FALSE);

			prAisReq = aisFsmGetNextRequest(prAdapter, ucBssIndex);
			if (prAisFsmInfo->ePreviousState ==
					AIS_STATE_OFF_CHNL_TX)
				aisFunClearAllTxReq(prAdapter,
						&(prAisFsmInfo->rMgmtTxInfo));

			if (prAisReq)
				DBGLOG(AIS, INFO,
					"eReqType=%d", prAisReq->eReqType);
			else
				DBGLOG(AIS, INFO, "No req anymore");

			if (prAisReq == NULL ||
			    prAisReq->eReqType == AIS_REQUEST_RECONNECT) {
				if (prAisReq != NULL) {
					aisDeactivateAllLink(prAdapter,
							prAisFsmInfo);
#if CFG_SUPPORT_DBDC
					if (cnmDBDCIsReqPeivilegeLock()) {
						DBGLOG(AIS, INFO,
						"DBDC lock: skip activate\n");
					} else
#endif
					{
					    /* sync with firmware */
					    nicActivateNetwork(prAdapter,
						prAisBssInfo->ucBssIndex);

					    SET_NET_PWR_STATE_ACTIVE(prAdapter,
						prAisBssInfo->ucBssIndex);
					}
					eNextState = AIS_STATE_SEARCH;
					fgIsTransition = TRUE;
				} else {
					SET_NET_PWR_STATE_IDLE(prAdapter,
					prAisBssInfo->ucBssIndex);
					/* If sched scan is ongoing, let sched
					 * scan deactivate newtwork when sched
					 * scan done.
					 */
					if (!prAdapter->rWifiVar.rScanInfo.
						fgSchedScanning)
						aisDeactivateAllLink(prAdapter,
								prAisFsmInfo);
				}

				if (prAisReq) {
					/* free the message */
					cnmMemFree(prAdapter, prAisReq);
				}
			} else if (prAisReq->eReqType == AIS_REQUEST_SCAN) {
				prAisScanReq = (struct AIS_SCAN_REQ *)prAisReq;
				aisFsmSyncScanReq(prAdapter,
					&prAisScanReq->rScanRequest,
					prAisBssInfo->ucBssIndex);
				eNextState = AIS_STATE_SCAN;
				fgIsTransition = TRUE;

				/* free the message */
				cnmMemFree(prAdapter, prAisReq);
			} else if (prAisReq->eReqType ==
				   AIS_REQUEST_ROAMING_CONNECT
				   || prAisReq->eReqType ==
				   AIS_REQUEST_ROAMING_SEARCH
				   || prAisReq->eReqType ==
				   AIS_REQUEST_BTO
				   || prAisReq->eReqType ==
				   AIS_REQUEST_CSA) {
				fgIsTransition = TRUE;
				/* ignore */
				/* free the message */
				cnmMemFree(prAdapter, prAisReq);
			} else if (prAisReq->eReqType ==
				   AIS_REQUEST_REMAIN_ON_CHANNEL) {
				eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;
				fgIsTransition = TRUE;

				/* free the message */
				cnmMemFree(prAdapter, prAisReq);
			}

			prAisFsmInfo->u4SleepInterval =
			    AIS_BG_SCAN_INTERVAL_MSEC;

#if (CFG_WOW_SUPPORT == 1)
			if (prAdapter->fgWowLinkDownPendFlag == TRUE) {
				prAdapter->fgWowLinkDownPendFlag = FALSE;
				kalOidComplete(prAdapter->prGlueInfo,
					NULL, 0, WLAN_STATUS_SUCCESS);
			}
#endif

			if (prAdapter->fgSuppSmeLinkDownPend) {
				prAdapter->fgSuppSmeLinkDownPend = FALSE;

				kalOidComplete(prAdapter->prGlueInfo,
					NULL, 0, WLAN_STATUS_SUCCESS);
			}

			break;

		case AIS_STATE_SEARCH: {
			struct BSS_DESC_SET *set =
				aisGetSearchResult(prAdapter, ucBssIndex);

			/* Search for a matched candidate and save
			 * it to prTargetBssDesc.
			 */
			kalMemZero(set, sizeof(struct BSS_DESC_SET));
			if (prAisFsmInfo->ucJoinFailCntAfterScan >=
			    SCN_BSS_JOIN_FAIL_THRESOLD)
				DBGLOG(AIS, STATE,
				       "Failed to connect %s more than 4 times after last scan, scan again\n",
				       prConnSettings->aucSSID);
			else
				aisSearchBssDescByScore(
					prAdapter, ucBssIndex, set);

			eNewState = aisSearchHandleBssDesc(
				prAdapter, set, ucBssIndex);

			if (eNewState != eNextState) {
				eNextState = eNewState;
				fgIsTransition = TRUE;
			}

			break;
		}

		case AIS_STATE_WAIT_FOR_NEXT_SCAN:

			DBGLOG(AIS, LOUD,
			       "SCAN: Idle Begin - Current Time = %u\n",
			       kalGetTimeTick());

			/* Process for pending BTO event */
			if (aisFsmIsRequestPending(prAdapter,
				AIS_REQUEST_BTO, TRUE,
				&prAisReq, ucBssIndex) == TRUE) {
				aisHandleBeaconTimeout(prAdapter,
					ucBssIndex, TRUE);
				fgIsTransition = FALSE;
				break;
			}

			cnmTimerStartTimer(prAdapter,
					   &prAisFsmInfo->rBGScanTimer,
					   prAisFsmInfo->u4SleepInterval);

			SET_NET_PWR_STATE_IDLE(prAdapter,
					       prAisBssInfo->ucBssIndex);
			break;
		case AIS_STATE_SCAN:
		case AIS_STATE_ONLINE_SCAN:
		case AIS_STATE_LOOKING_FOR:
			if (!wlanIsDriverReady(prAdapter->prGlueInfo,
				WLAN_DRV_READY_CHECK_WLAN_ON |
				WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
				DBGLOG(AIS, WARN, "driver is not ready\n");
				return;
			}
			if (!IS_NET_ACTIVE(prAdapter, prAisBssInfo->ucBssIndex))
				/* sync with firmware */
				nicActivateNetwork(prAdapter,
						   prAisBssInfo->ucBssIndex);

			prScanRequest = aisGetScanReq(prAdapter,
				prAisBssInfo->ucBssIndex);
			prScanRequest->fgOobRnrParseEn = TRUE;

			prScanReqMsg =
			    (struct MSG_SCN_SCAN_REQ_V2 *)cnmMemAlloc(prAdapter,
					RAM_TYPE_MSG,
					sizeof(struct MSG_SCN_SCAN_REQ_V2));
			if (!prScanReqMsg) {
				DBGLOG(AIS, ERROR, "Can't trigger SCAN FSM\n");
				return;
			}

			aisScanReqInit(prAdapter, ucBssIndex, prScanReqMsg);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (prAisFsmInfo->ucMlProbeEnable &&
			    !aisScanGenMlScanReq(prAdapter, ucBssIndex,
					prScanReqMsg))
				goto send_msg;
#endif

			aisScanProcessReqParam(prAdapter, ucBssIndex,
				prScanReqMsg, prScanRequest);

			scanInitEssResult(prAdapter);

#if (CFG_SUPPORT_ROAMING_LOG == 1)
			/* LOG: scan start */
			roamingFsmLogScanStart(prAdapter,
				ucBssIndex,
				prScanReqMsg->eScanChannel !=
				SCAN_CHANNEL_SPECIFIED,
				aisGetTargetBssDesc(
				prAdapter, ucBssIndex));
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
send_msg:
			aisScanAddRlmIE(prAdapter, prScanReqMsg);
#endif

			mboxSendMsg(prAdapter, MBOX_ID_0,
				    (struct MSG_HDR *)prScanReqMsg,
				    MSG_SEND_METHOD_BUF);
			/* reset prAisFsmInfo->rScanRequest */
			aisScanResetReq(prScanRequest);

			/* Support AP Selection */
			prAisFsmInfo->ucJoinFailCntAfterScan = 0;
			prAisFsmInfo->fgIsScanning = TRUE;

			break;

#if CFG_SUPPORT_ROAMING
		case AIS_STATE_ROAMING:
			roamingFsmRunEventNewCandidate(prAdapter,
				aisGetSearchResult(prAdapter, ucBssIndex),
				ucBssIndex);
			/* wait until roamingFsm is ready */
			break;
#endif

		case AIS_STATE_REQ_CHANNEL_JOIN:
			aisReqJoinChPrivilege(prAdapter,
				prAisFsmInfo,
				&prAisFsmInfo->ucSeqNumOfChReq);
			break;

		case AIS_STATE_JOIN: {
			struct STA_RECORD *prMainStaRec = NULL;

			for (i = 0; i < MLD_LINK_MAX; i++) {
				struct BSS_INFO *bss = aisGetLinkBssInfo(
					prAisFsmInfo, i);

				if (!bss || !aisGetLinkBssDesc(prAisFsmInfo, i))
					continue;
				/* Renew op trx nss */
				cnmOpModeGetTRxNss(prAdapter,
						   bss->ucBssIndex,
						   &bss->ucOpRxNss,
						   &bss->ucOpTxNss);
				aisFsmStateInit_JOIN(prAdapter,
						prAisFsmInfo,
						&prMainStaRec,
						i);
			}
			break;
		}
		case AIS_STATE_JOIN_FAILURE:
			nicMediaJoinFailure(prAdapter,
					    prAisBssInfo->ucBssIndex,
					    WLAN_STATUS_JOIN_FAILURE);
			aisFsmDisconnectedAction(prAdapter,
				prAisBssInfo->ucBssIndex);
			eNextState = AIS_STATE_IDLE;
			fgIsTransition = TRUE;

			break;

#if CFG_SUPPORT_ADHOC
		case AIS_STATE_IBSS_ALONE:
			aisFsmStateInit_IBSS_ALONE(prAdapter,
				ucBssIndex);
			break;

		case AIS_STATE_IBSS_MERGE:
			aisFsmStateInit_IBSS_MERGE(prAdapter,
				prAisFsmInfo->prTargetBssDesc,
				ucBssIndex);
			break;
#endif /* CFG_SUPPORT_ADHOC */

		case AIS_STATE_NORMAL_TR:
			/* recycle unused bssinfo */
			aisFreeAllBssInfo(prAdapter, prAisFsmInfo, FALSE);

			/* Don't do anything when rJoinTimeoutTimer
			 * is still ticking
			 */
			if (timerPendingTimer(&prAisFsmInfo->rJoinTimeoutTimer))
				break;

			if (prAisFsmInfo->ePreviousState ==
					AIS_STATE_OFF_CHNL_TX) {
				aisFunClearAllTxReq(prAdapter,
						&(prAisFsmInfo->rMgmtTxInfo));
				aisRestoreBandIdx(prAdapter, prAisBssInfo);
			}

			/* for WMM-AC cert 5.2.5 */
			/* after reassoc, update PS flag to FW again */
			if (prAisFsmInfo->ucReasonOfDisconnect ==
				DISCONNECT_REASON_CODE_REASSOCIATION &&
			    prAisFsmInfo->ePreviousState == AIS_STATE_JOIN)
				wmmReSyncPsParamWithFw(prAdapter, ucBssIndex);

			/* reset disconnect reason otherwise
			 * aisSearchHandleBssDesc uses it wrongly
			 */
			prAisFsmInfo->ucReasonOfDisconnect =
				DISCONNECT_REASON_CODE_RESERVED;

			prConnSettings->u2LinkIdBitmap = 0xFFFF;

			eNewState = aisFsmHandleNextReq_NORMAL_TR(prAdapter,
				prAisFsmInfo, ucBssIndex);

			if (eNewState != eNextState) {
				eNextState = eNewState;
				fgIsTransition = TRUE;
			}

			break;

		case AIS_STATE_DISCONNECTING:
			/* send for deauth frame for disconnection */
			authSendDeauthFrame(prAdapter,
					    prAisBssInfo,
					    prAisBssInfo->prStaRecOfAP,
					    (struct SW_RFB *)NULL,
					    prAisBssInfo->u2DeauthReason,
					    aisDeauthXmitComplete);

			prAisBssInfo->u2DeauthReason = REASON_CODE_RESERVED;

			/* If it is scanning or BSS absent, HW may go away from
			 * serving channel, which may cause driver be not able
			 * to TX mgmt frame. So we need to start a longer timer
			 * to wait HW return to serving channel.
			 * We set the time out value to 1 second because
			 * it is long enough to return to serving channel
			 * in most cases, and disconnection delay is seamless
			 * to end-user even time out.
			 */
			cnmTimerStartTimer(prAdapter,
					&prAisFsmInfo->rDeauthDoneTimer,
					(prAisFsmInfo->fgIsScanning ||
					 isNetAbsent(prAdapter, prAisBssInfo)) ?
					1000 : 100);
			break;

		case AIS_STATE_REQ_REMAIN_ON_CHANNEL:
			/* send message to CNM for acquiring channel */
			prMsgChReq =
			    (struct MSG_CH_REQ *)cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG,
				sizeof(struct MSG_CH_REQ));
			if (!prMsgChReq) {
				DBGLOG(AIS, ERROR, "Can't indicate CNM\n");
				return;
			}

			/* release channel */
			aisFsmReleaseCh(prAdapter, ucBssIndex);

			/* zero-ize */
			kalMemZero(prMsgChReq, sizeof(struct MSG_CH_REQ));

			prAisFsmInfo->ucSeqNumOfChReq = cnmIncreaseTokenId(prAdapter);

			/* stop Tx to avoid sending data but FW already change
			 * own mac when band swapped
			 */
			if (prAisBssInfo->prStaRecOfAP) {
				qmSetStaRecTxAllowed(prAdapter,
					   prAisBssInfo->prStaRecOfAP,
					   FALSE);
				DBGLOG(AIS, INFO,
					"[TxMgmt] TxAllowed = FALSE\n");
			}

			/* filling */
			prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
			prMsgChReq->ucBssIndex =
			    prAisBssInfo->ucBssIndex;
			prMsgChReq->ucTokenID = prAisFsmInfo->ucSeqNumOfChReq;
			prMsgChReq->eReqType =
					prAisFsmInfo->rChReqInfo.eReqType;
			prMsgChReq->u4MaxInterval =
			    prAisFsmInfo->rChReqInfo.u4DurationMs;
			prMsgChReq->ucPrimaryChannel =
			    prAisFsmInfo->rChReqInfo.ucChannelNum;
			prMsgChReq->eRfSco = prAisFsmInfo->rChReqInfo.eSco;
			prMsgChReq->eRfBand = prAisFsmInfo->rChReqInfo.eBand;
#if CFG_SUPPORT_DBDC
			prMsgChReq->eDBDCBand = ENUM_BAND_AUTO;
#endif
			mboxSendMsg(prAdapter, MBOX_ID_0,
				    (struct MSG_HDR *)prMsgChReq,
				    MSG_SEND_METHOD_UNBUF);

			prAisFsmInfo->ucChReqNum = 1;
			prAisFsmInfo->fgIsChannelRequested = TRUE;
			prAisFsmInfo->ucBssIndexOfChReq =
				prAisBssInfo->ucBssIndex;

			break;

		case AIS_STATE_REMAIN_ON_CHANNEL:
			if (!IS_NET_ACTIVE(prAdapter, prAisBssInfo->ucBssIndex))
				/* sync with firmware */
				nicActivateNetwork(prAdapter,
						   prAisBssInfo->ucBssIndex);

			break;

		case AIS_STATE_OFF_CHNL_TX:
			if (!IS_NET_ACTIVE(prAdapter, prAisBssInfo->ucBssIndex))
				/* sync with firmware */
				nicActivateNetwork(prAdapter,
					prAisBssInfo->ucBssIndex);

			if (!aisState_OFF_CHNL_TX(prAdapter, ucBssIndex)) {
				if (prAisBssInfo->eConnectionState ==
						MEDIA_STATE_CONNECTED) {
					/* restore txallow status */
					if (prAisBssInfo->prStaRecOfAP) {
						qmSetStaRecTxAllowed(prAdapter,
						     prAisBssInfo->prStaRecOfAP,
						     TRUE);
						DBGLOG(AIS, INFO,
						 "[TxMgmt] TxAllowed = TRUE\n");
					}
					eNextState = AIS_STATE_NORMAL_TR;
				} else {
					eNextState = AIS_STATE_IDLE;
				}
				fgIsTransition = TRUE;
			}
			break;

		default:
			/* Make sure we have handle all STATEs */
			ASSERT(0);
			break;

		}
	} while (fgIsTransition);

	return;

}				/* end of aisFsmSteps() */

enum ENUM_AIS_STATE aisFsmStateSearchAction(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	if (prConnSettings->eOPMode == NET_TYPE_INFRA)
		prAisFsmInfo->ucConnTrialCount++;

	prAisFsmInfo->fgTargetChnlScanIssued = FALSE;

#if CFG_SUPPORT_ADHOC
	if (prConnSettings->eOPMode == NET_TYPE_IBSS ||
		   prConnSettings->eOPMode == NET_TYPE_AUTO_SWITCH ||
		   prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS) {
		prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;
		prAisFsmInfo->prTargetBssDesc = NULL;
		return AIS_STATE_IBSS_ALONE;
	}
#endif /* CFG_SUPPORT_ADHOC */

	return AIS_STATE_LOOKING_FOR;
}

void aisFsmQueryCandidates(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
#if CFG_SUPPORT_802_11K
	struct STA_RECORD *prStaRec;
	struct BSS_DESC *prBssDesc;
	struct BSS_TRANSITION_MGT_PARAM *prBtmParam;

	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
	prStaRec = aisGetStaRecOfAP(prAdapter, ucBssIndex);
	prBtmParam = aisGetBTMParam(prAdapter, ucBssIndex);

#if CFG_SUPPORT_802_11V_BTM_OFFLOAD
	if (prBtmParam->fgPendingResponse) {
		DBGLOG(WNM, WARN, "BTM: don't query when handling\n");
		return;
	}
#endif

	if (prBssDesc && !prBssDesc->fgQueriedCandidates) {
		prBssDesc->fgQueriedCandidates = TRUE;

		aisResetNeighborApList(prAdapter, ucBssIndex);

		if (prBssDesc->aucRrmCap[0] &
		    BIT(RRM_CAP_INFO_NEIGHBOR_REPORT_BIT))
			aisSendNeighborRequest(prAdapter, ucBssIndex);
		else if (prBssDesc->fgSupportBTM)
			wnmSendBTMQueryFrame(prAdapter,
				prStaRec, BSS_TRANSITION_BETTER_AP_FOUND);
	}
#endif
}

uint8_t aisFsmUpdateChannelList(uint8_t channel, enum ENUM_BAND eBand,
	uint8_t *bitmap, uint8_t *count, struct ESS_CHNL_INFO *info)
{
	uint8_t byteNum = 0;
	uint8_t bitNum = 0;

	byteNum = channel / 8;
	bitNum = channel % 8;
#if (CFG_EXT_ROAMING == 1)
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G)
		byteNum + 32;
#endif
#endif
	if (bitmap[byteNum] & BIT(bitNum))
		return 1;
	bitmap[byteNum] |= BIT(bitNum);
	info[*count].ucChannel = channel;
	info[*count].eBand = eBand;
	*count += 1;
	if (*count >= CFG_MAX_NUM_OF_CHNL_INFO)
		return 0;

	return 1;
}

void aisFsmGetCurrentEssChnlList(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct BSS_DESC *prBssDesc = NULL;
	struct LINK *prBSSDescList =
		&prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	struct CONNECTION_SETTINGS *prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);
	struct ESS_CHNL_INFO *prEssChnlInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo;
	struct APS_INFO *prApsInfo = aisGetApsInfo(prAdapter, ucBssIndex);
#if (CFG_EXT_ROAMING == 1)
	uint8_t aucChnlBitMap[64] = {0,};
#else
	uint8_t aucChnlBitMap[30] = {0,};
#endif
	uint8_t aucChnlApNum[234] = {0,};
	uint8_t aucChnlUtil[234] = {0,};
	uint8_t ucChnlCount = 0;
	uint32_t i;
	uint8_t j = 0;
#if CFG_SUPPORT_802_11K
	struct LINK *prNeighborAPLink;
#endif
	struct CFG_SCAN_CHNL *prRoamScnChnl = &prAdapter->rAddRoamScnChnl;
	uint16_t u2ApNum = 0;

	if (!prConnSettings)  {
		log_dbg(SCN, INFO, "No prConnSettings\n");
		return;
	}

	if (prConnSettings->ucSSIDLen == 0) {
		log_dbg(SCN, INFO, "No Ess are expected to connect\n");
		return;
	}

	prAisSpecBssInfo =
		aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	if (!prAisSpecBssInfo) {
		log_dbg(SCN, INFO, "No prAisSpecBssInfo\n");
		return;
	}
	prEssChnlInfo =
		&prAisSpecBssInfo->arCurEssChnlInfo[0];
	if (!prEssChnlInfo) {
		log_dbg(SCN, INFO, "No prEssChnlInfo\n");
		return;
	}

	kalMemZero(prEssChnlInfo, CFG_MAX_NUM_OF_CHNL_INFO *
		sizeof(struct ESS_CHNL_INFO));

	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry,
		struct BSS_DESC) {
		if (prBssDesc->ucChannelNum > 233)
			continue;
		/* Statistic AP num for each channel */
		if (aucChnlApNum[prBssDesc->ucChannelNum] < 255)
			aucChnlApNum[prBssDesc->ucChannelNum]++;
		if (aucChnlUtil[prBssDesc->ucChannelNum] <
			prBssDesc->ucChnlUtilization)
			aucChnlUtil[prBssDesc->ucChannelNum] =
				prBssDesc->ucChnlUtilization;
		if (!EQUAL_SSID(prConnSettings->aucSSID,
			prConnSettings->ucSSIDLen,
			prBssDesc->aucSSID, prBssDesc->ucSSIDLen) ||
			prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE)
			continue;

		u2ApNum++;

#if CFG_SUPPORT_NCHO
		/* scan control is 1: use NCHO channel list only */
		if (prAdapter->rNchoInfo.u4RoamScanControl)
			continue;
#endif
		if (!aisFsmUpdateChannelList(prBssDesc->ucChannelNum,
			prBssDesc->eBand, aucChnlBitMap, &ucChnlCount,
			prEssChnlInfo))
			goto updated;
	}

#if CFG_SUPPORT_NCHO
	if (prAdapter->rNchoInfo.fgNCHOEnabled) {
		struct CFG_NCHO_SCAN_CHNL *ncho;

		if (prAdapter->rNchoInfo.u4RoamScanControl)
			ncho = &prAdapter->rNchoInfo.rRoamScnChnl;
		else
			ncho = &prAdapter->rNchoInfo.rAddRoamScnChnl;

		/* handle user-specefied scan channel info */
		for (i = 0; ucChnlCount < CFG_MAX_NUM_OF_CHNL_INFO &&
			i < ncho->ucChannelListNum; i++) {
			uint8_t chnl;
			enum ENUM_BAND eBand;

			chnl = ncho->arChnlInfoList[i].ucChannelNum;
			eBand = ncho->arChnlInfoList[i].eBand;
			if (!aisFsmUpdateChannelList(chnl, eBand,
			    aucChnlBitMap, &ucChnlCount, prEssChnlInfo))
				goto updated;
		}

		if (prAdapter->rNchoInfo.u4RoamScanControl)
			goto updated;
	}
#endif

#if CFG_SUPPORT_802_11K
	prNeighborAPLink = &prAisSpecBssInfo->rNeighborApList.rUsingLink;
	if (!LINK_IS_EMPTY(prNeighborAPLink)) {
		/* Add channels provided by Neighbor Report to
		 ** channel list for roaming scanning.
		 */
		struct NEIGHBOR_AP *prNeiAP = NULL;
		enum ENUM_BAND eBand;
		uint8_t ucChannel;

		LINK_FOR_EACH_ENTRY(prNeiAP, prNeighborAPLink,
		    rLinkEntry, struct NEIGHBOR_AP) {
			ucChannel = prNeiAP->ucChannel;
			eBand = prNeiAP->eBand;
			if (!rlmDomainIsLegalChannel(
				prAdapter, eBand, ucChannel))
				continue;
			if (!aisFsmUpdateChannelList(ucChannel, eBand,
				aucChnlBitMap, &ucChnlCount, prEssChnlInfo))
				goto updated;
		}
	}
#endif

	/* handle user-specefied scan channel info */
	for (i = 0; ucChnlCount < CFG_MAX_NUM_OF_CHNL_INFO &&
		i < prRoamScnChnl->ucChannelListNum; i++) {
		uint8_t chnl;
		enum ENUM_BAND eBand;

		chnl = prRoamScnChnl->arChnlInfoList[i].ucChannelNum;
		eBand = prRoamScnChnl->arChnlInfoList[i].eBand;
		if (!aisFsmUpdateChannelList(chnl, eBand,
		    aucChnlBitMap, &ucChnlCount, prEssChnlInfo))
			goto updated;
	}

updated:
	prAisSpecBssInfo->ucCurEssChnlInfoNum = ucChnlCount;
	for (j = 0; j < ucChnlCount; j++) {
		uint8_t ucChnl = prEssChnlInfo[j].ucChannel;

		prEssChnlInfo[j].ucApNum = aucChnlApNum[ucChnl];
		prEssChnlInfo[j].ucUtilization = aucChnlUtil[ucChnl];
	}

	prApsInfo->u2EssApNum = u2ApNum;

#if (CFG_SUPPORT_ROAMING_LOG == 1)
	roamingFsmLogScanDone(
		prAdapter,
		ucBssIndex,
		u2ApNum);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventScanDone(struct ADAPTER *prAdapter,
			    struct MSG_HDR *prMsgHdr)
{
	struct MSG_SCN_SCAN_DONE *prScanDoneMsg;
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	uint8_t ucSeqNumOfCompMsg;
	struct CONNECTION_SETTINGS *prConnSettings;
	enum ENUM_SCAN_STATUS eStatus = SCAN_STATUS_DONE;
	struct RADIO_MEASUREMENT_REQ_PARAMS *prRmReq;
	struct BCN_RM_PARAMS *prBcnRmParam;
	uint8_t ucBssIndex = 0;

	prScanDoneMsg = (struct MSG_SCN_SCAN_DONE *)prMsgHdr;
	ucBssIndex = prScanDoneMsg->ucBssIndex;

	DBGLOG(AIS, LOUD, "[%d] EVENT-SCAN DONE: Current Time = %u\n",
		ucBssIndex,
		kalGetTimeTick());

	if (aisGetAisBssInfo(prAdapter, ucBssIndex) == NULL) {
		/* This case occurs when the AIS isn't done, but the wlan0 */
		/* has changed to AP mode. And the prAisBssInfo is freed.  */
		DBGLOG(AIS, WARN, "prAisBssInfo is NULL, and then return\n");
		return;
	}

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prRmReq = aisGetRmReqParam(prAdapter, ucBssIndex);
	prBcnRmParam = &prRmReq->rBcnRmParam;

	ucSeqNumOfCompMsg = prScanDoneMsg->ucSeqNum;
	eStatus = prScanDoneMsg->eScanStatus;
	cnmMemFree(prAdapter, prMsgHdr);

	DBGLOG(AIS, INFO,
	       "ScanDone %u, status(%d) native req(%u)\n",
	       ucSeqNumOfCompMsg, eStatus, prAisFsmInfo->ucSeqNumOfScanReq);

#ifdef CFG_SUPPORT_TWT_EXT
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucTWTRequester))
		twtPlannerCheckResume(prAdapter);
#endif

#if (CFG_SUPPORT_WIFI_6G == 1)
	/* No need to send Uevent if EnOnlyScan6g is enabled */
	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgEnOnlyScan6g))
#endif
		scnFsmNotifyEvent(prAdapter, eStatus, ucBssIndex);

	eNextState = prAisFsmInfo->eCurrentState;

	if (ucSeqNumOfCompMsg != prAisFsmInfo->ucSeqNumOfScanReq) {
		DBGLOG(AIS, WARN,
		       "SEQ NO of AIS SCN DONE MSG is not matched %u %u\n",
		       ucSeqNumOfCompMsg, prAisFsmInfo->ucSeqNumOfScanReq);
	} else {
		kalScanDone(prAdapter->prGlueInfo, ucBssIndex,
			(eStatus == SCAN_STATUS_DONE) ?
			WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE);

		prAisFsmInfo->fgIsScanning = FALSE;
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);
		switch (prAisFsmInfo->eCurrentState) {
		case AIS_STATE_SCAN:
			eNextState = AIS_STATE_IDLE;
#if CFG_SUPPORT_AGPS_ASSIST
			scanReportScanResultToAgps(prAdapter);
#endif
			break;

		case AIS_STATE_ONLINE_SCAN:
			aisFsmGetCurrentEssChnlList(prAdapter, ucBssIndex);

#if CFG_SUPPORT_ROAMING
			eNextState = aisFsmRoamingScanResultsUpdate(prAdapter,
				ucBssIndex);
#else
			eNextState = AIS_STATE_NORMAL_TR;
#endif /* CFG_SUPPORT_ROAMING */
#if CFG_SUPPORT_AGPS_ASSIST
			scanReportScanResultToAgps(prAdapter);
#endif

			aisFsmQueryCandidates(prAdapter, ucBssIndex);
			break;

		case AIS_STATE_LOOKING_FOR:
			aisFsmGetCurrentEssChnlList(prAdapter, ucBssIndex);

#if CFG_SUPPORT_ROAMING
			eNextState = aisFsmRoamingScanResultsUpdate(prAdapter,
				ucBssIndex);
#else
			eNextState = AIS_STATE_SEARCH;
#endif /* CFG_SUPPORT_ROAMING */
			break;

		default:
			break;

		}
	}

	if (prBcnRmParam->eState == RM_ON_GOING) {
#if CFG_SUPPORT_802_11K
		struct LINK *prBSSDescList =
			&prAdapter->rWifiVar.rScanInfo.rBSSDescList;
		struct BSS_DESC *prBssDesc = NULL;
		uint32_t count = 0;
#if (CFG_EXT_ROAMING == 1)
		struct IE_MEASUREMENT_REQ *prCurrReq = prRmReq->prCurrMeasElem;
		struct RM_BCN_REQ *prBcnReq =
			(struct RM_BCN_REQ *)&prCurrReq->aucRequestFields[0];
#endif

		/* collect updated bss for beacon request measurement */
		LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry,
				    struct BSS_DESC)
		{
			if (TIME_BEFORE(prRmReq->rScanStartTime,
				prBssDesc->rUpdateTime)) {
				rrmCollectBeaconReport(
					prAdapter, prBssDesc, ucBssIndex);
				count++;
			}
		}
#if (CFG_EXT_ROAMING == 1)
		if (prBcnReq &&
			prBcnReq->ucMeasurementMode < RM_BCN_REQ_MODE_MAX)
			DBGLOG(RRM, INFO,
				"BCN report (%s) mode req, total: %d\n",
				prBcnReq->ucMeasurementMode ==
				RM_BCN_REQ_ACTIVE_MODE ?
				"Active" : "Passive", count);
#else
		DBGLOG(RRM, INFO, "BCN report Active Mode, total: %d\n", count);
#endif
#if (CFG_SUPPORT_REPORT_LOG == 1)
		rrmRespBeaconReportLog(prAdapter,
			ucBssIndex,
			prCurrReq->ucToken,
			count);
#endif
#endif
		rrmStartNextMeasurement(prAdapter, FALSE, ucBssIndex);
	}

	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState, ucBssIndex);
}				/* end of aisFsmRunEventScanDone() */

void aisFsmAddBlockList(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *ais, uint16_t u2DeauthReason)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *prAisBssInfo = aisGetLinkBssInfo(ais, i);
		struct BSS_DESC *prBss = aisGetLinkBssDesc(ais, i);

		if (prAisBssInfo)
			prAisBssInfo->u2DeauthReason = u2DeauthReason;

		if (prBss) {
			struct AIS_BLOCKLIST_ITEM *blk =
			    aisAddBlocklist(prAdapter, prBss);

			if (blk) {
				blk->u2DeauthReason = u2DeauthReason;
				blk->fgDeauthLastTime = TRUE;
				blk->ucDeauthCount++;
				if (blk->ucDeauthCount > 5)
					blk->ucDeauthCount = 5;

				DBGLOG(AIS, INFO, "update deauth count %d for "
						MACSTR "\n",
						blk->ucDeauthCount,
						MAC2STR(prBss->aucBSSID));
			}
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventAbort(struct ADAPTER *prAdapter,
			 struct MSG_HDR *prMsgHdr)
{
	struct MSG_AIS_ABORT *prAisAbortMsg;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prBssInfo;
	uint8_t ucReasonOfDisconnect;
	u_int8_t fgDelayIndication;
	uint16_t u2DeauthReason;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = 0;

	/* 4 <1> Extract information of Abort Message and then free memory. */
	prAisAbortMsg = (struct MSG_AIS_ABORT *)prMsgHdr;
	ucReasonOfDisconnect = prAisAbortMsg->ucReasonOfDisconnect;
	fgDelayIndication = prAisAbortMsg->fgDelayIndication;
	u2DeauthReason = prAisAbortMsg->u2DeauthReason;

	ucBssIndex = prAisAbortMsg->ucBssIndex;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	ucBssIndex = aisGetMainLinkBssIndex(prAdapter, prAisFsmInfo);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	cnmMemFree(prAdapter, prMsgHdr);

	DBGLOG(AIS, STATE,
	       "[%d] EVENT-ABORT: Current State %s, ucReasonOfDisconnect:%d\n",
	       ucBssIndex,
	       aisGetFsmState(prAisFsmInfo->eCurrentState),
	       ucReasonOfDisconnect);

	/* record join request time */
	GET_CURRENT_SYSTIME(&(prAisFsmInfo->rJoinReqTime));

	if (ucReasonOfDisconnect == DISCONNECT_REASON_CODE_DEAUTHENTICATED ||
	    ucReasonOfDisconnect == DISCONNECT_REASON_CODE_DISASSOCIATED)
		aisFsmAddBlockList(prAdapter, prAisFsmInfo,
			u2DeauthReason);

	/* to support user space triggered roaming */
	if ((ucReasonOfDisconnect == DISCONNECT_REASON_CODE_ROAMING ||
	     ucReasonOfDisconnect == DISCONNECT_REASON_CODE_TEST_MODE) &&
	    prAisFsmInfo->eCurrentState != AIS_STATE_DISCONNECTING) {
#if CFG_SUPPORT_ROAMING
		struct ROAMING_INFO *prRoamingFsmInfo =
			aisGetRoamingInfo(prAdapter, ucBssIndex);
		struct CMD_ROAMING_TRANSIT rRoamingData = {0};
		struct BSS_DESC *prBssDesc =
			aisGetTargetBssDesc(prAdapter, ucBssIndex);

		if (!roamingFsmInDecision(prAdapter, TRUE, ucBssIndex)) {
			DBGLOG(AIS, STATE,
				"Ignore roaming request if unable to roam\n");

			/* DISCONNECT_REASON_CODE_ROAMING is triggered by
			 * supplicant, must indicate the connection status,
			 */
			if (ucReasonOfDisconnect ==
			    DISCONNECT_REASON_CODE_ROAMING)
				aisIndicationOfMediaStateToHost(prAdapter,
					MEDIA_STATE_CONNECTED,
					FALSE,
					ucBssIndex);
			return;
		}

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
		cnmTimerStopTimer(prAdapter,
				  &prAisFsmInfo->rSecModeChangeTimer);
#endif
#if (CFG_SUPPORT_ML_RECONFIG == 1)
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rApRemovalTimer);
#endif /* CFG_SUPPORT_802_11BE_MLO */

		prAisFsmInfo->ucReasonOfDisconnect = ucReasonOfDisconnect;
		rRoamingData.eReason = ROAMING_REASON_UPPER_LAYER_TRIGGER;
		rRoamingData.u2Data = prBssDesc ?
			prBssDesc->ucRCPI : RCPI_FOR_DONT_ROAM;
		rRoamingData.u2RcpiLowThreshold =
			prRoamingFsmInfo->ucThreshold;
		rRoamingData.ucBssidx = ucBssIndex;
		roamingFsmRunEventDiscovery(prAdapter, &rRoamingData);
#endif
		return;
	}
	/* Support AP Selection */
	aisFsmGetCurrentEssChnlList(prAdapter, ucBssIndex);

	aisFsmClearRequest(prAdapter, AIS_REQUEST_RECONNECT, ucBssIndex);
	aisFsmClearRequest(prAdapter, AIS_REQUEST_BTO, ucBssIndex);

	/* for new connection triggered by upper layer,
	 * DISCONNECT_REASON_CODE_ROAMING, DISCONNECT_REASON_CODE_TEST_MODE
	 * are already handled ahead,
	 * DISCONNECT_REASON_CODE_REASSOCIATION is handled in aisFsmStateAbort,
	 * so only add request for DISCONNECT_REASON_CODE_NEW_CONNECTION
	 */
	if (ucReasonOfDisconnect == DISCONNECT_REASON_CODE_NEW_CONNECTION)
		aisFsmInsertRequestToHead(prAdapter,
			AIS_REQUEST_RECONNECT, ucBssIndex);

	if (ucReasonOfDisconnect == DISCONNECT_REASON_CODE_NEW_CONNECTION ||
	    ucReasonOfDisconnect == DISCONNECT_REASON_CODE_LOCALLY)
		prBssInfo->u2DeauthReason = REASON_CODE_DEAUTH_LEAVING_BSS;

	if (prAisFsmInfo->eCurrentState != AIS_STATE_DISCONNECTING ||
	    prAisFsmInfo->fgIsDelIface) {
		/* 4 <3> invoke abort handler */
		aisFsmStateAbort(prAdapter, ucReasonOfDisconnect,
			fgDelayIndication, ucBssIndex);
	}

	/* Change to MEDIA_STATE_TO_BE_INDICATED so
	 * aisIndicationOfMediaStateToHost can report to host when it's really
	 * disconnected or connected.
	 */
	if (ucReasonOfDisconnect == DISCONNECT_REASON_CODE_NEW_CONNECTION)
		prBssInfo->eConnectionStateIndicated =
			MEDIA_STATE_TO_BE_INDICATED;
}				/* end of aisFsmRunEventAbort() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief        This function handles AIS-FSM abort event/command
 *
 * \param[in] prAdapter              Pointer of ADAPTER_T
 *            ucReasonOfDisconnect   Reason for disonnection
 *            fgDelayIndication      Option to delay disconnection indication
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort(struct ADAPTER *prAdapter,
		uint8_t ucReasonOfDisconnect, u_int8_t fgDelayIndication,
		uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	u_int8_t fgIsCheckConnected;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	/* XXX: The wlan0 may has been changed to AP mode. */
	if (prAisBssInfo == NULL)
		return;

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	fgIsCheckConnected = FALSE;

	DBGLOG(AIS, STATE,
		"[%d] aisFsmStateAbort DiscReason[%d], CurState[%d], delayIndi[%d]\n",
		ucBssIndex, ucReasonOfDisconnect,
		prAisFsmInfo->eCurrentState, fgDelayIndication);

#if CFG_SUPPORT_DFS
	aisFunSwitchChannelAbort(prAdapter, prAisFsmInfo, FALSE);
#endif

	/* 4 <1> Save information of Abort Message and then free memory. */
	prAisFsmInfo->ucReasonOfDisconnect = ucReasonOfDisconnect;
	if (prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED &&
	    prAisFsmInfo->eCurrentState != AIS_STATE_DISCONNECTING &&
	    ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION &&
	    ucReasonOfDisconnect != DISCONNECT_REASON_CODE_ROAMING &&
	    ucReasonOfDisconnect != DISCONNECT_REASON_CODE_TEST_MODE)
		wmmNotifyDisconnected(prAdapter, ucBssIndex);

	if (fgDelayIndication) {
		uint8_t join = timerPendingTimer(
				&prAisFsmInfo->rJoinTimeoutTimer);

		if (join) {
			fgDelayIndication = FALSE;
			DBGLOG(AIS, INFO,
				"delay indication not allowed due to join");
		}

#if CFG_ENABLE_WIFI_DIRECT && (CFG_TC10_FEATURE == 1)
		if (cnmP2pIsActive(prAdapter)) {
			fgDelayIndication = FALSE;
			DBGLOG(AIS, INFO,
				"delay indication not allowed due to p2p");
		}
#endif
	}

	/* 4 <2> Abort current job. */
	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_IDLE:
	case AIS_STATE_SEARCH:
	case AIS_STATE_ROAMING:
	case AIS_STATE_JOIN_FAILURE:
		break;

	case AIS_STATE_WAIT_FOR_NEXT_SCAN:
		/* Do cancel timer */
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBGScanTimer);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_ONLINE_SCAN:
		fgIsCheckConnected = TRUE;
		kal_fallthrough;
	case AIS_STATE_SCAN:
		/* Do abort SCAN */
		aisFsmStateAbort_SCAN_All(prAdapter);
		break;
	case AIS_STATE_LOOKING_FOR:
		/* Do abort SCAN */
		aisFsmStateAbort_SCAN_All(prAdapter);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_REQ_CHANNEL_JOIN:
		/* Release channel to CNM */
		aisFsmReleaseCh(prAdapter, ucBssIndex);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_JOIN:
		/* Do abort JOIN */
		aisFsmStateAbort_JOIN(prAdapter, ucBssIndex);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		break;

#if CFG_SUPPORT_ADHOC
	case AIS_STATE_IBSS_ALONE:
	case AIS_STATE_IBSS_MERGE:
		aisFsmStateAbort_IBSS(prAdapter, ucBssIndex);
		break;
#endif /* CFG_SUPPORT_ADHOC */
	case AIS_STATE_NORMAL_TR:
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_DISCONNECTING:
		/* Do abort NORMAL_TR */
		aisFsmStateAbort_NORMAL_TR(prAdapter, ucBssIndex);

		break;

	case AIS_STATE_REQ_REMAIN_ON_CHANNEL:
		/* release channel */
		aisFsmReleaseCh(prAdapter, ucBssIndex);
		break;

	case AIS_STATE_REMAIN_ON_CHANNEL:
	case AIS_STATE_OFF_CHNL_TX:
		fgIsCheckConnected = TRUE;
		/* 1. release channel */
		aisFsmReleaseCh(prAdapter, ucBssIndex);

		/* 2. stop channel timeout timer */
		cnmTimerStopTimer(prAdapter,
				  &prAisFsmInfo->rChannelTimeoutTimer);

		break;

	default:
		break;
	}

#if CFG_SUPPORT_ROAMING
	if (prAisFsmInfo->ucIsStaRoaming) {
		/* Restore all BssInfo and Link settings during roaming */
		aisRestoreAllLink(prAdapter, prAisFsmInfo);
		prAisFsmInfo->ucIsStaRoaming = FALSE;

		DBGLOG(AIS, INFO, "roaming settings restoration is complete\n");
	}
#endif

	if (fgIsCheckConnected
	    && (prAisBssInfo->eConnectionState ==
		MEDIA_STATE_CONNECTED)) {

		/* switch into DISCONNECTING state for sending DEAUTH
		 * if necessary
		 */
		if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
		    (prAisFsmInfo->ucReasonOfDisconnect ==
		     DISCONNECT_REASON_CODE_NEW_CONNECTION ||
#if CFG_RADIO_LOST_DISCONNECT
		     prAisFsmInfo->ucReasonOfDisconnect ==
		     DISCONNECT_REASON_CODE_RADIO_LOST ||
#endif
		     prAisFsmInfo->ucReasonOfDisconnect ==
		     DISCONNECT_REASON_CODE_LOCALLY)
		    && prAisBssInfo->prStaRecOfAP
		    && prAisBssInfo->prStaRecOfAP->fgIsInUse) {
			aisFsmSteps(prAdapter, AIS_STATE_DISCONNECTING,
				ucBssIndex);

			return;
		}
		/* Do abort NORMAL_TR */
		aisFsmStateAbort_NORMAL_TR(prAdapter, ucBssIndex);
	}
	rrmFreeMeasurementResources(prAdapter, ucBssIndex);
	aisFsmDisconnect(prAdapter, fgDelayIndication, ucBssIndex);

	return;

}				/* end of aisFsmStateAbort() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Join Complete Event from SAA FSM
 *        for AIS FSM
 * @param[in] prMsgHdr   Message of Join Complete of SAA FSM.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventJoinComplete(struct ADAPTER *prAdapter,
				struct MSG_HDR *prMsgHdr)
{
	struct MSG_SAA_FSM_COMP *prJoinCompMsg;
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	struct SW_RFB *prAssocRspSwRfb;
	struct STA_RECORD *prStaRec;
	uint8_t ucBssIndex = 0;

	prJoinCompMsg = (struct MSG_SAA_FSM_COMP *)prMsgHdr;
	prAssocRspSwRfb = prJoinCompMsg->prSwRfb;
	prStaRec = prJoinCompMsg->prStaRec;
	if (prStaRec)
		ucBssIndex = prStaRec->ucBssIndex;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	eNextState = prAisFsmInfo->eCurrentState;

	/* Check State and SEQ NUM */
	if (prAisFsmInfo->eCurrentState == AIS_STATE_JOIN) {
		/* Check SEQ NUM */
		if (prJoinCompMsg->ucSeqNum == prAisFsmInfo->ucSeqNumOfReqMsg)
			eNextState =
			    aisFsmJoinCompleteAction(prAdapter, prMsgHdr);
		else {
			eNextState = AIS_STATE_JOIN_FAILURE;
			DBGLOG(AIS, WARN,
			       "SEQ NO of AIS JOIN COMP MSG is not matched.\n");
		}
	}

	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState, ucBssIndex);

	if (prAssocRspSwRfb)
		nicRxReturnRFB(prAdapter, prAssocRspSwRfb);

	cnmMemFree(prAdapter, prMsgHdr);
}				/* end of aisFsmRunEventJoinComplete() */

void aisRestoreBandIdx(struct ADAPTER *ad, struct BSS_INFO *prBssInfo)
{
	if (!prBssInfo)
		return;

	prBssInfo->eHwBandIdx = prBssInfo->eBackupHwBandIdx;
	DBGLOG(AIS, TRACE,
		"ucBssIdx=%d, ucBandIdx=%d\n",
		prBssInfo->ucBssIndex, prBssInfo->eHwBandIdx);
}

void aisRestoreBssInfo(struct ADAPTER *ad, struct BSS_INFO *prBssInfo,
	struct BSS_DESC *prBssDesc, uint8_t ucLinkIndex)
{
	uint8_t ucRfCenterFreqSeg1, ucPrimaryChannel;
	enum ENUM_CHANNEL_WIDTH eRfChannelWidth;
	enum ENUM_CHNL_EXT eRfSco;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct GL_WPA_INFO *prWpaInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
#if CFG_SUPPORT_DBDC
	struct DBDC_DECISION_INFO rDbdcDecisionInfo = {0};
#endif

	if (!prBssInfo || !prBssDesc)
		return;

	prAisSpecificBssInfo = aisGetAisSpecBssInfo(ad, prBssInfo->ucBssIndex);
	prConnSettings = aisGetConnSettings(ad, prBssInfo->ucBssIndex);
	prWpaInfo = aisGetWpaInfo(ad, prBssInfo->ucBssIndex);

#if CFG_SUPPORT_802_11W
	prAisSpecificBssInfo->fgMgmtProtection = !!prBssDesc->u4RsnSelectedPmf;
#endif
	prConnSettings->eAuthMode = prBssDesc->eRsnSelectedAuthMode;
	prWpaInfo->u4WpaVersion = prBssDesc->u4RsnSelectedProto;
	prBssInfo->u4RsnSelectedGroupCipher =
		prBssDesc->u4RsnSelectedGroupCipher;
	prBssInfo->u4RsnSelectedPairwiseCipher =
		prBssDesc->u4RsnSelectedPairwiseCipher;
	prBssInfo->u4RsnSelectedGroupMgmtCipher =
		prBssDesc->u4RsnSelectedGroupMgmtCipher;
	prBssInfo->u4RsnSelectedAKMSuite = prBssDesc->u4RsnSelectedAKMSuite;
	prBssInfo->eBand = prBssDesc->eBand;
	ucPrimaryChannel = prBssDesc->ucChannelNum;
	eRfSco = prBssDesc->eSco;
	eRfChannelWidth = prBssDesc->eChannelWidth;
	ucRfCenterFreqSeg1 = nicGetS1(ad, prBssDesc->eBand, ucPrimaryChannel,
		eRfChannelWidth);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prBssInfo->ucLinkIndex = prBssDesc->rMlInfo.ucLinkIndex;
#endif

	prBssInfo->ucGrantTxNss = prBssInfo->ucBackupGrantTxNss;
	prBssInfo->ucGrantRxNss = prBssInfo->ucBackupGrantRxNss;
	prBssInfo->ucGrantBW = prBssInfo->ucBackupGrantBW;

	rlmReviseMaxBw(ad, prBssInfo->ucBssIndex, &eRfSco, &eRfChannelWidth,
		&ucRfCenterFreqSeg1, &ucPrimaryChannel);

	prBssInfo->ucVhtChannelWidth = eRfChannelWidth;
	prBssInfo->eBssSCO = eRfSco;

	/* update fgMgmtProtection from main link only */
	if (ucLinkIndex == 0)
		aisFsmUpdateRsnSetting(ad, prBssDesc, prBssInfo->ucBssIndex);

#if CFG_SUPPORT_DBDC
	/* DBDC decsion.may change OpNss */
	CNM_DBDC_ADD_DECISION_INFO(rDbdcDecisionInfo,
		prBssInfo->ucBssIndex,
		prBssDesc->eBand,
		prBssDesc->ucChannelNum,
		prBssInfo->ucWmmQueSet);

	cnmDbdcPreConnectionEnableDecision(
		ad,
		&rDbdcDecisionInfo);
#endif /*CFG_SUPPORT_DBDC*/
}

void aisRestoreAllLink(struct ADAPTER *ad, struct AIS_FSM_INFO *ais)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *prAisBssInfo = aisGetLinkBssInfo(ais, i);
		struct STA_RECORD *prStaRec = aisGetLinkStaRec(ais, i);
		struct PARAM_SSID rSsid;
		struct BSS_DESC *prBssDesc = NULL;

		if (!prAisBssInfo)
			continue;

		kalMemZero(&rSsid, sizeof(struct PARAM_SSID));
		COPY_SSID(rSsid.aucSsid,
			  rSsid.u4SsidLen,
			  prAisBssInfo->aucSSID,
			  prAisBssInfo->ucSSIDLen);
		prBssDesc = scanSearchBssDescByBssidAndSsid(ad,
			prAisBssInfo->aucBSSID, TRUE, &rSsid);
		aisSetLinkBssDesc(ais, prBssDesc, i);
		aisSetLinkStaRec(ais, prAisBssInfo->prStaRecOfAP, i);

		aisRestoreBandIdx(ad, prAisBssInfo);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		mldBssUpdateBandIdxBitmap(ad, prAisBssInfo);
#endif

		/* Free STA-REC */
		if (prStaRec && prStaRec != prAisBssInfo->prStaRecOfAP) {
			/* reset to idle to avoid re-entrance by
			 * saaFsmRunEventTxDone if there's pending auth/assoc
			 */
			prStaRec->eAuthAssocState = AA_STATE_IDLE;
			cnmStaRecFree(ad, prStaRec);
		}

		/* free bssinfo if it's not connected */
		if (i != AIS_MAIN_LINK_INDEX &&
		    prAisBssInfo->eConnectionState != MEDIA_STATE_CONNECTED)
			aisFreeBssInfo(ad, ais, i);

		/* roaming but can't find connected bssdesc */
		if (prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
			if (!prBssDesc)
				DBGLOG(AIS, ERROR,
					"Can't find target BssDesc %d\n", i);
			else
				aisRestoreBssInfo(ad, prAisBssInfo,
					prBssDesc, i);
		}
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldBssRestoreCap(ad, ais->prMldBssInfo);
#endif
}

u_int8_t aisHandleTemporaryReject(struct ADAPTER *prAdapter,
			      struct STA_RECORD *prStaRec)
{
#if CFG_SUPPORT_802_11W
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
	uint8_t ucBssIndex = 0;
#if CFG_SUPPORT_ROAMING
	struct ROAMING_INFO *prRoamingInfo;
#endif

	ucBssIndex = prStaRec->ucBssIndex;
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisSpecificBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
#if CFG_SUPPORT_ROAMING
	prRoamingInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);
#endif

	if (prStaRec->u2StatusCode == STATUS_CODE_ASSOC_REJECTED_TEMPORARILY) {
		/* record temporarily rejected AP for SA query */
		prAisSpecificBssInfo->prTargetComebackBssDesc =
			aisGetTargetBssDesc(prAdapter, ucBssIndex);
		/* Extend trial count during Beacon timeout retry*/
		prAisFsmInfo->ucConnTrialCountLimit = 5;
#if CFG_SUPPORT_ROAMING
		prRoamingInfo->eReason = ROAMING_REASON_TEMP_REJECT;
#endif
		DBGLOG(AIS, INFO, "reschedule a comeback timer %u msec\n",
			TU_TO_MSEC(prStaRec->u4assocComeBackTime));
		return true;
	}
	return false;
#else
	return false;
#endif
}

uint8_t aisHandleJoinFailure(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct SW_RFB *prAssocRspSwRfb, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct BSS_DESC *prBssDesc;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct PMKID_ENTRY *prPmkidEntry;
	enum ENUM_AIS_STATE eNextState;
	struct WLAN_ASSOC_RSP_FRAME *prAssocRspFrame = NULL;
	uint16_t u2IELength = 0;
	uint8_t *pucIE = NULL;
	OS_SYSTIME rCurrentTime;
	uint8_t fgTempReject;

	GET_CURRENT_SYSTIME(&rCurrentTime);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
	eNextState = prAisFsmInfo->eCurrentState;

	if (prBssDesc == NULL) {
		DBGLOG(AIS, ERROR, "Can't get bss descriptor\n");
		return AIS_STATE_JOIN_FAILURE;
	}

	if (prAssocRspSwRfb) {
		prAssocRspFrame = (struct WLAN_ASSOC_RSP_FRAME *)
			prAssocRspSwRfb->pvHeader;
		u2IELength = (uint16_t)
			((prAssocRspSwRfb->u2PacketLen -
			prAssocRspSwRfb->u2HeaderLen) -
		       (OFFSET_OF(struct WLAN_ASSOC_RSP_FRAME, aucInfoElem[0]) -
			WLAN_MAC_MGMT_HEADER_LEN));
		pucIE = prAssocRspFrame->aucInfoElem;
	}

	/* 1. Increase Failure Count */
	if (prStaRec->u2StatusCode != STATUS_CODE_ASSOC_REJECTED_TEMPORARILY) {
		prStaRec->ucJoinFailureCount++;
		prBssDesc->ucJoinFailureCount++;
	}

	/* 2. release channel */
	aisFsmReleaseCh(prAdapter, ucBssIndex);

	/* 3.1 stop join timeout timer */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);

	/* Support AP Selection */
	prAisFsmInfo->ucJoinFailCntAfterScan++;

	DBGLOG(AIS, INFO,
	       "ucJoinFailureCount=%d %d, Status=%d Reason=%d, eConnectionState=%d",
	       prStaRec->ucJoinFailureCount,
	       prBssDesc->ucJoinFailureCount,
	       prStaRec->u2StatusCode,
	       prStaRec->u2ReasonCode,
	       prAisBssInfo->eConnectionState);

	COPY_MAC_ADDR(prConnSettings->aucJoinBSSID, prBssDesc->aucBSSID);
	prConnSettings->u2JoinStatus = prStaRec->u2StatusCode;
	GET_CURRENT_SYSTIME(&prBssDesc->rJoinFailTime);

	if (aisGetAuthMode(prAdapter, ucBssIndex) == AUTH_MODE_WPA3_OWE
			&& prStaRec->u2StatusCode ==
			STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED) {
		DBGLOG(AIS, INFO,
		       "OWE DH GROUP AP NOT SUPPORT, no need retry in driver.\n");
		return AIS_STATE_JOIN_FAILURE;
	}

	if (prBssDesc->ucJoinFailureCount >= SCN_BSS_JOIN_FAIL_THRESOLD) {
		aisAddBlocklist(prAdapter, prBssDesc);
		DBGLOG(AIS, INFO,
		       "" MACSTR "join fail %d times,temp disable at time:%u\n",
		       MAC2STR(prBssDesc->aucBSSID),
		       prBssDesc->ucJoinFailureCount,
		       prBssDesc->rJoinFailTime);

	} else if (rsnApOverload(prStaRec->u2StatusCode,
			prStaRec->u2ReasonCode)) {
		aisAddBlocklist(prAdapter, prBssDesc);
		DBGLOG(AIS, INFO,
		       "" MACSTR "overload status=%d reason=%d at time:%u\n",
		       MAC2STR(prBssDesc->aucBSSID),
		       prStaRec->u2StatusCode,
		       prStaRec->u2ReasonCode,
		       prBssDesc->rJoinFailTime);
#if CFG_SUPPORT_MBO
	} else if (pucIE && prStaRec->u2StatusCode ==
			STATUS_CODE_ASSOC_DENIED_POOR_CHANNEL) {
		struct IE_MBO_OCE *mbo = NULL;
		const uint8_t *reject = NULL;

		dumpMemory8(pucIE, u2IELength);

		mbo = (struct IE_MBO_OCE *) kalFindVendorIe(
				VENDOR_IE_TYPE_MBO >> 8,
				VENDOR_OUI_TYPE_MBO,
				pucIE,
				u2IELength);
		if (mbo) {
			int u4lenParam = mbo->ucLength - 4;

			/* The mbo->ucLength is 8 bits value. So that,
			 * its max value is 255. 255 - 4 = 251.
			 * Avoid the overflow check for the signed &
			 * unsigned operation.
			 */
			if ((u4lenParam > 0) && (u4lenParam <= 251))
				reject = kalFindIeMatchMask(
					OCE_ATTR_ID_RSSI_BASED_ASSOC_REJECT,
					mbo->aucSubElements,
					u4lenParam,
					NULL, 0, 0, NULL);
			if (reject && reject[1] == 2) {
				aisBssTmpDisallow(prAdapter, prBssDesc,
				    reject[3],
				    reject[2] + RCPI_TO_dBm(prBssDesc->ucRCPI));
				prBssDesc->ucJoinFailureCount +=
					SCN_BSS_JOIN_FAIL_THRESOLD;
			}
		}
#endif
	}

	prPmkidEntry = aisSearchPmkidEntry(prAdapter, prStaRec, ucBssIndex);
	if (prPmkidEntry)
		prPmkidEntry->u2StatusCode = prStaRec->u2StatusCode;

	if (prBssDesc->prBlock)
		prBssDesc->prBlock->u2AuthStatus = prStaRec->u2StatusCode;

	fgTempReject = aisHandleTemporaryReject(prAdapter, prStaRec);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	/* check whether multi link before restoring all links,
	 * otherwise starec is freed
	 */
	if (!fgTempReject && mldIsMultiLinkFormed(prAdapter, prStaRec))
		aisAddMldBlocklist(prAdapter, prBssDesc);
#endif
	aisTargetBssResetConnecting(prAdapter, prAisFsmInfo);
	aisRestoreAllLink(prAdapter, prAisFsmInfo);

	/* aisRestoreAllLink clears target bssdesc and starec if no connection,
	 * DO NOT use prStaRec or aisGetTargetBssDesc after this point
	 */

	if (fgTempReject ||
	    prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
		/* roaming fail count and time */
		prAdapter->prGlueInfo->u4RoamFailCnt++;
		prAdapter->prGlueInfo->u8RoamFailTime = kalGetTimeTickNs();

#if CFG_SUPPORT_ROAMING
		if (fgTempReject)
			prAisFsmInfo->u4SleepInterval =
				TU_TO_MSEC(prStaRec->u4assocComeBackTime);
		else
			prAisFsmInfo->u4SleepInterval =
				AIS_BG_SCAN_INTERVAL_MSEC;

		eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
#endif /* CFG_SUPPORT_ROAMING */

		if (prAisBssInfo->prStaRecOfAP)
			prAisBssInfo->prStaRecOfAP->fgIsTxAllowed = TRUE;
#if CFG_SUPPORT_ROAMING
		roamingFsmNotifyEvent(prAdapter, ucBssIndex, TRUE, prBssDesc);
		prAisFsmInfo->ucIsStaRoaming = FALSE;
#endif
	} else if (prAisFsmInfo->rJoinReqTime != 0 &&
		CHECK_FOR_TIMEOUT(rCurrentTime, prAisFsmInfo->rJoinReqTime,
		SEC_TO_SYSTIME(AIS_JOIN_TIMEOUT))) {
#if CFG_SUPPORT_WPA3_LOG
		wpa3LogJoinFail(prAdapter,
			prAisBssInfo);
#endif
		/* 4.a temrminate join operation */
		eNextState = AIS_STATE_JOIN_FAILURE;
	} else if (prAisFsmInfo->rJoinReqTime != 0 &&
		prBssDesc->ucJoinFailureCount >= SCN_BSS_JOIN_FAIL_THRESOLD &&
		prConnSettings->u2JoinStatus) {
		/* AP reject STA for
		 * STATUS_CODE_ASSOC_DENIED_AP_OVERLOAD
		 * , or AP block STA
		 */
		eNextState = AIS_STATE_JOIN_FAILURE;
	} else {
		/* 4.b send reconnect request */
		aisFsmInsertRequestToHead(prAdapter,
			AIS_REQUEST_RECONNECT, ucBssIndex);
		eNextState = AIS_STATE_IDLE;
	}

	return eNextState;
}

#if CFG_SUPPORT_ROAMING
void aisFsmEnableRssiMonitor(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo, uint8_t ucEnable)
{
	struct PARAM_RSSI_MONITOR_T rRssi;
	uint32_t rStatus;

	kalMemCopy(&rRssi, &prAisFsmInfo->rRSSIMonitor,
		sizeof(struct PARAM_RSSI_MONITOR_T));

	rRssi.enable = ucEnable;
	if (!ucEnable)
		rRssi.max_rssi_value = rRssi.min_rssi_value = 0;

	rStatus = wlanSendSetQueryCmd(prAdapter,
			   CMD_ID_RSSI_MONITOR,
			   TRUE,
			   FALSE,
			   FALSE,
			   NULL,
			   NULL,
			   sizeof(struct PARAM_RSSI_MONITOR_T),
			   (uint8_t *)&rRssi, NULL, 0);
}
#endif

void aisChangeAllMediaState(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *prAisBssInfo =
			aisGetLinkBssInfo(prAisFsmInfo, i);
		struct STA_RECORD *prStaRec =
			aisGetLinkStaRec(prAisFsmInfo, i);
		struct BSS_DESC *prBssDesc =
			aisGetLinkBssDesc(prAisFsmInfo, i);
		uint8_t ucBssIndex;

		if (!prAisBssInfo)
			continue;

		if (prStaRec &&
		    prStaRec->u2StatusCode != STATUS_CODE_SUCCESSFUL) {
			DBGLOG(AIS, INFO, "Remove link%d status code=%d\n",
				i, prStaRec->u2StatusCode);
			cnmStaRecFree(prAdapter, prStaRec);
			prStaRec = NULL;

			ucBssIndex = prAisBssInfo->ucBssIndex;
			if (prBssDesc) {
				prBssDesc->fgIsConnected &= ~BIT(ucBssIndex);
				prBssDesc->fgIsConnecting &= ~BIT(ucBssIndex);
			}
		}

		kalResetStats(
			wlanGetNetDev(
			prAdapter->prGlueInfo,
			prAisBssInfo->ucBssIndex));

		aisChangeMediaState(prAisBssInfo, prStaRec ?
			MEDIA_STATE_CONNECTED : MEDIA_STATE_DISCONNECTED);

		/* 4 <1.2> Deactivate previous AP's STA_RECORD_T
		 * in Driver if have.
		 */
		if ((prAisBssInfo->prStaRecOfAP) &&
		    (prAisBssInfo->prStaRecOfAP != prStaRec) &&
		    (prAisBssInfo->prStaRecOfAP->fgIsInUse) &&
		    (prAisBssInfo->prStaRecOfAP->ucBssIndex ==
		     prAisBssInfo->ucBssIndex)) {
			cnmStaRecChangeState(prAdapter,
				prAisBssInfo->prStaRecOfAP,
				STA_STATE_1);
			cnmStaRecFree(prAdapter,
				prAisBssInfo->prStaRecOfAP);
			prAisBssInfo->prStaRecOfAP = NULL;
		}

		/* free bssinfo if it has no target starec */
		if (i != AIS_MAIN_LINK_INDEX &&
		    prAisBssInfo->eConnectionState == MEDIA_STATE_DISCONNECTED)
			aisFreeBssInfo(prAdapter, prAisFsmInfo, i);
	}
}

enum ENUM_AIS_STATE aisFsmJoinCompleteAction(struct ADAPTER *prAdapter,
					     struct MSG_HDR *prMsgHdr)
{
	struct MSG_SAA_FSM_COMP *prJoinCompMsg;
	struct AIS_FSM_INFO *prAisFsmInfo;
#if CFG_SUPPORT_ROAMING
	struct ROAMING_INFO *roam;
#endif
	enum ENUM_AIS_STATE eNextState;
	struct STA_RECORD *prStaRec;
	struct SW_RFB *prAssocRspSwRfb;
	struct BSS_INFO *prAisBssInfo;
	struct LINK_SPEED_EX_ *prLq;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = 0;

	prJoinCompMsg = (struct MSG_SAA_FSM_COMP *)prMsgHdr;
	prStaRec = prJoinCompMsg->prStaRec;
	prAssocRspSwRfb = prJoinCompMsg->prSwRfb;

	ucBssIndex = prStaRec->ucBssIndex;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	eNextState = prAisFsmInfo->eCurrentState;
	prLq = &prAdapter->rLinkQuality.rLq[ucBssIndex];
#if CFG_SUPPORT_ROAMING
	roam = aisGetRoamingInfo(prAdapter, ucBssIndex);
#endif

	do {
		/* 4 <1> JOIN was successful */
		if (prJoinCompMsg->rJoinStatus == WLAN_STATUS_SUCCESS) {

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
			prConnSettings->fgSecModeChangeStartTimer = FALSE;
#endif

			/* 1. Reset retry count */
			prAisFsmInfo->ucConnTrialCount = 0;

#if ARP_MONITER_ENABLE
			arpMonResetArpDetect(prAdapter, prStaRec->ucBssIndex);
#endif /* ARP_MONITER_ENABLE */

			aisResetBssTranstionMgtParam(prAdapter, ucBssIndex);

#if CFG_SUPPORT_ROAMING
			prAisFsmInfo->ucIsStaRoaming = FALSE;

			/* if roaming fsm is monitoring old AP, abort it
			 * abort before aisFsmRoamingDisconnectPrevAllAP because
			 * bssinfo is deactivated when roaming 2 -> 1 link
			 */
			if (roam->eCurrentState >= ROAMING_STATE_DECISION)
				roamingFsmRunEventAbort(prAdapter, ucBssIndex);
#endif /* CFG_SUPPORT_ROAMING */

			/* Completion of roaming */
			if (prAisBssInfo->eConnectionState ==
			    MEDIA_STATE_CONNECTED) {
#if CFG_SUPPORT_ROAMING
				roamingFsmNotifyEvent(prAdapter,
					ucBssIndex, FALSE,
					aisGetMainLinkBssDesc(prAisFsmInfo));

#ifdef CFG_SUPPORT_TWT_EXT
				twtPlannerReset(prAdapter, prAisBssInfo);
#endif

				/* 2. Deactivate previous BSS */
				aisFsmRoamingDisconnectPrevAllAP(prAdapter,
							      prAisFsmInfo);

				/* 3. Update bss based on roaming staRec */
				aisUpdateBssInfoForRoamingAllAP(prAdapter,
							     prAisFsmInfo,
							     prAssocRspSwRfb,
							     prStaRec);

				/* 3.1 Update DBDC mode */
#if CFG_SUPPORT_DBDC
				cnmDbdcRuntimeCheckDecision(prAdapter,
							    ucBssIndex,
							    FALSE);
#endif

				/* reset buffered link quality information */
				prLq->fgIsLinkQualityValid = FALSE;
				prLq->fgIsLinkRateValid = FALSE;

				/* 4 <1.6> Indicate Connected Event to Host
				 * immediately.
				 */
				/* Require BSSID, Association ID,
				 * Beacon Interval
				 */
				/* .. from AIS_BSS_INFO_T */
				aisIndicationOfMediaStateToHost(prAdapter,
					MEDIA_STATE_CONNECTED,
					FALSE,
					ucBssIndex);
#endif /* CFG_SUPPORT_ROAMING */
			} else {
#if CFG_SUPPORT_ROAMING
				if (aisFsmIsInProcessPostpone(prAdapter,
					ucBssIndex)) {
					roamingFsmNotifyEvent(
					   prAdapter, ucBssIndex, FALSE,
					   aisGetMainLinkBssDesc(prAisFsmInfo));

					/* Enable rssi monitor */
					if (prAisFsmInfo->rRSSIMonitor.enable)
						aisFsmEnableRssiMonitor(
								prAdapter,
								prAisFsmInfo,
								TRUE);
				}
#endif /* CFG_SUPPORT_ROAMING */

				/* 4 <1.1> Change FW's Media State
				 * immediately.
				 */
				aisChangeAllMediaState(prAdapter, prAisFsmInfo);

				/* For temp solution, need to refine */
				/* 4 <1.4> Update BSS_INFO_T */
				aisUpdateAllBssInfoForJOIN(prAdapter,
					prAisFsmInfo,
					prAssocRspSwRfb,
					prStaRec);

				/* reset buffered link quality information */
				prLq->fgIsLinkQualityValid = FALSE;
				prLq->fgIsLinkRateValid = FALSE;

				/* 4 <1.6> Indicate Connected Event to Host
				 * immediately.
				 */
				/* Require BSSID, Association ID,
				 * Beacon Interval
				 */
				/* .. from AIS_BSS_INFO_T */
				aisIndicationOfMediaStateToHost(prAdapter,
					MEDIA_STATE_CONNECTED,
					FALSE,
					ucBssIndex);

				if (prAdapter->rWifiVar.ucTpTestMode ==
				    ENUM_TP_TEST_MODE_THROUGHPUT)
					nicEnterTPTestMode(prAdapter,
						TEST_MODE_THROUGHPUT);
				else if (prAdapter->rWifiVar.ucTpTestMode ==
					 ENUM_TP_TEST_MODE_SIGMA_AC_N_PMF)
					nicEnterTPTestMode(prAdapter,
					TEST_MODE_SIGMA_AC_N_PMF);
				else if (prAdapter->rWifiVar.ucTpTestMode ==
					 ENUM_TP_TEST_MODE_SIGMA_WMM_PS)
					nicEnterTPTestMode(prAdapter,
						TEST_MODE_SIGMA_WMM_PS);

#if (CFG_SUPPORT_ANDROID_DUAL_STA == 1)
				/* Check dual station status */
				aisCheckMultiStaStatus(prAdapter,
					MEDIA_STATE_CONNECTED, ucBssIndex);
#endif

				rsnAllowCrossAkm(prAdapter, ucBssIndex);
			}

#if CFG_SUPPORT_ROAMING
			if (prConnSettings->eConnectionPolicy !=
							CONNECT_BY_BSSID)
				prConnSettings->eConnectionPolicy =
						CONNECT_BY_SSID_BEST_RSSI;

			/* always start roaming fsm for user space roaming */
			roamingFsmRunEventStart(prAdapter, ucBssIndex);
#endif /* CFG_SUPPORT_ROAMING */

			prAisFsmInfo->rJoinReqTime = 0;

			prAisFsmInfo->ucJoinFailCntAfterScan = 0;

			rlmReqGenerateOMIIE(prAdapter, prAisBssInfo);
			/* 4 <1.7> Set the Next State of AIS FSM */
			eNextState = AIS_STATE_NORMAL_TR;
		}
		/* 4 <2> JOIN was not successful */
		else {
			struct PMKID_ENTRY *prPmkidEntry;

			/* update pmk status before retry */
			prPmkidEntry = aisSearchPmkidEntry(prAdapter,
				prStaRec, ucBssIndex);
			if (prPmkidEntry)
				prPmkidEntry->u2StatusCode =
					prStaRec->u2StatusCode;

			/* 4 <2.1> Redo JOIN process with other Auth Type
			 * if possible
			 */
			if (aisFsmStateInit_RetryJOIN(prAdapter, prStaRec,
				ucBssIndex) == FALSE) {
				eNextState = aisHandleJoinFailure(
					prAdapter, prStaRec,
					prAssocRspSwRfb, ucBssIndex);
			}
		}
	} while (0);
	return eNextState;
}

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Grant Msg of IBSS Create which was
 *        sent by CNM to indicate that channel was changed for creating IBSS.
 *
 * @param[in] prAdapter  Pointer of ADAPTER_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmCreateIBSS(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	do {
		/* Check State */
		if (prAisFsmInfo->eCurrentState == AIS_STATE_IBSS_ALONE)
			aisUpdateBssInfoForCreateIBSS(prAdapter, ucBssIndex);

	} while (FALSE);
}				/* end of aisFsmCreateIBSS() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Grant Msg of IBSS Merge which was
 *        sent by CNM to indicate that channel was changed for merging IBSS.
 *
 * @param[in] prAdapter  Pointer of ADAPTER_T
 * @param[in] prStaRec   Pointer of STA_RECORD_T for merge
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmMergeIBSS(struct ADAPTER *prAdapter,
		     struct STA_RECORD *prStaRec)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	struct BSS_INFO *prAisBssInfo;
	uint8_t ucBssIndex = 0;

	ucBssIndex = prStaRec->ucBssIndex;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	do {

		eNextState = prAisFsmInfo->eCurrentState;

		switch (prAisFsmInfo->eCurrentState) {
		case AIS_STATE_IBSS_MERGE:
			{
				struct BSS_DESC *prBssDesc;

				/* 4 <1.1> Change FW's Media State
				 * immediately.
				 */
				aisChangeMediaState(prAisBssInfo,
					MEDIA_STATE_CONNECTED);

				/* 4 <1.2> Deactivate previous Peers'
				 * STA_RECORD_T in Driver if have.
				 */
				bssInitializeClientList(prAdapter,
							prAisBssInfo);

				/* 4 <1.3> Unmark connection flag of previous
				 * BSS_DESC_T.
				 */
				prBssDesc =
				    scanSearchBssDescByBssid(prAdapter,
					prAisBssInfo->aucBSSID);
				if (prBssDesc != NULL) {
					prBssDesc->fgIsConnecting &=
						~BIT(ucBssIndex);
					prBssDesc->fgIsConnected &=
						~BIT(ucBssIndex);
				}
				/* 4 <1.4> Add Peers' STA_RECORD_T to
				 * Client List
				 */
				bssAddClient(prAdapter, prAisBssInfo, prStaRec);

				/* 4 <1.5> Activate current Peer's STA_RECORD_T
				 * in Driver.
				 */
				cnmStaRecChangeState(prAdapter, prStaRec,
						     STA_STATE_3);
				prStaRec->fgIsMerging = FALSE;

				/* 4 <1.6> Update BSS_INFO_T */
				aisUpdateBssInfoForMergeIBSS(prAdapter,
							     prStaRec);

				/* 4 <1.7> Enable other features */

				/* 4 <1.8> Indicate Connected Event to Host
				 * immediately.
				 */
				aisIndicationOfMediaStateToHost(prAdapter,
					MEDIA_STATE_CONNECTED,
					FALSE,
					ucBssIndex);

				/* 4 <1.9> Set the Next State of AIS FSM */
				eNextState = AIS_STATE_NORMAL_TR;

				/* 4 <1.10> Release channel privilege */
				aisFsmReleaseCh(prAdapter, ucBssIndex);

#if CFG_SLT_SUPPORT
				prAdapter->rWifiVar.rSltInfo.prPseudoStaRec =
				    prStaRec;
#endif
			}
			break;

		default:
			break;
		}

		if (eNextState != prAisFsmInfo->eCurrentState)
			aisFsmSteps(prAdapter, eNextState, ucBssIndex);

	} while (FALSE);
}				/* end of aisFsmMergeIBSS() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Notification of existing IBSS was found
 *        from SCN.
 *
 * @param[in] prMsgHdr   Message of Notification of an IBSS was present.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventFoundIBSSPeer(struct ADAPTER *prAdapter,
				 struct MSG_HDR *prMsgHdr)
{
	struct MSG_AIS_IBSS_PEER_FOUND *prAisIbssPeerFoundMsg;
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prAisBssInfo;
	struct BSS_DESC *prBssDesc;
	u_int8_t fgIsMergeIn;
	uint8_t ucBssIndex = 0;

	prAisIbssPeerFoundMsg = (struct MSG_AIS_IBSS_PEER_FOUND *)prMsgHdr;
	ucBssIndex = prAisIbssPeerFoundMsg->ucBssIndex;
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	prStaRec = prAisIbssPeerFoundMsg->prStaRec;

	fgIsMergeIn = prAisIbssPeerFoundMsg->fgIsMergeIn;

	cnmMemFree(prAdapter, prMsgHdr);

	eNextState = prAisFsmInfo->eCurrentState;
	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_IBSS_ALONE:
		{
			/* 4 <1> An IBSS Peer 'merged in'. */
			if (fgIsMergeIn) {

				/* 4 <1.1> Change FW's Media State
				 * immediately.
				 */
				aisChangeMediaState(prAisBssInfo,
					MEDIA_STATE_CONNECTED);

				/* 4 <1.2> Add Peers' STA_RECORD_T to
				 * Client List
				 */
				bssAddClient(prAdapter, prAisBssInfo, prStaRec);

#if CFG_SLT_SUPPORT
				/* 4 <1.3> Mark connection flag of
				 * BSS_DESC_T.
				 */
				prBssDesc =
				    scanSearchBssDescByTA(prAdapter,
							  prStaRec->aucMacAddr);

				if (prBssDesc != NULL) {
					prBssDesc->fgIsConnecting &=
						~BIT(ucBssIndex);
					prBssDesc->fgIsConnected |=
						BIT(ucBssIndex);
				}

				/* 4 <1.4> Activate current Peer's
				 * STA_RECORD_T in Driver.
				 */
				/* TODO(Kevin): TBD */
				prStaRec->fgIsQoS = TRUE;
#else
				/* 4 <1.3> Mark connection flag
				 * of BSS_DESC_T.
				 */
				prBssDesc =
				    scanSearchBssDescByBssid(prAdapter,
					prAisBssInfo->aucBSSID);

				if (prBssDesc != NULL) {
					prBssDesc->fgIsConnecting &=
						~BIT(ucBssIndex);
					prBssDesc->fgIsConnected |=
						BIT(ucBssIndex);
				}

				/* 4 <1.4> Activate current Peer's STA_RECORD_T
				 * in Driver.
				 */
				/* TODO(Kevin): TBD */
				prStaRec->fgIsQoS = FALSE;

#endif

				cnmStaRecChangeState(prAdapter, prStaRec,
						     STA_STATE_3);
				prStaRec->fgIsMerging = FALSE;

				/* 4 <1.6> sync. to firmware */
				nicUpdateBss(prAdapter,
					     prAisBssInfo->ucBssIndex);

				/* 4 <1.7> Indicate Connected Event to Host
				 * immediately.
				 */
				aisIndicationOfMediaStateToHost(prAdapter,
					MEDIA_STATE_CONNECTED,
					FALSE,
					ucBssIndex);

				/* 4 <1.8> indicate PM for connected */
				nicPmIndicateBssConnected(prAdapter,
					prAisBssInfo->ucBssIndex);

				/* 4 <1.9> Set the Next State of AIS FSM */
				eNextState = AIS_STATE_NORMAL_TR;

				/* 4 <1.10> Release channel privilege */
				aisFsmReleaseCh(prAdapter, ucBssIndex);
			}
			/* 4 <2> We need 'merge out' to this IBSS */
			else {

				/* 4 <2.1> Get corresponding BSS_DESC_T */
				prBssDesc =
				    scanSearchBssDescByTA(prAdapter,
							  prStaRec->aucMacAddr);

				prAisFsmInfo->prTargetBssDesc = prBssDesc;

				/* 4 <2.2> Set the Next State of AIS FSM */
				eNextState = AIS_STATE_IBSS_MERGE;
			}
		}
		break;

	case AIS_STATE_NORMAL_TR:
		{

			/* 4 <3> An IBSS Peer 'merged in'. */
			if (fgIsMergeIn) {

				/* 4 <3.1> Add Peers' STA_RECORD_T to
				 * Client List
				 */
				bssAddClient(prAdapter, prAisBssInfo, prStaRec);

#if CFG_SLT_SUPPORT
				/* 4 <3.2> Activate current Peer's STA_RECORD_T
				 * in Driver.
				 */
				/* TODO(Kevin): TBD */
				prStaRec->fgIsQoS = TRUE;
#else
				/* 4 <3.2> Activate current Peer's STA_RECORD_T
				 * in Driver.
				 */
				/* TODO(Kevin): TBD */
				prStaRec->fgIsQoS = FALSE;
#endif

				cnmStaRecChangeState(prAdapter, prStaRec,
						     STA_STATE_3);
				prStaRec->fgIsMerging = FALSE;

			}
			/* 4 <4> We need 'merge out' to this IBSS */
			else {

				/* 4 <4.1> Get corresponding BSS_DESC_T */
				prBssDesc =
				    scanSearchBssDescByTA(prAdapter,
							  prStaRec->aucMacAddr);

				prAisFsmInfo->prTargetBssDesc = prBssDesc;

				/* 4 <4.2> Set the Next State of AIS FSM */
				eNextState = AIS_STATE_IBSS_MERGE;

			}
		}
		break;

	default:
		break;
	}

	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState, ucBssIndex);
}				/* end of aisFsmRunEventFoundIBSSPeer() */
#endif /* CFG_SUPPORT_ADHOC */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will do necessary procedures when disconnected
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
static void aisFsmDisconnectedAction(struct ADAPTER *prAdapter,
				     uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
#if CFG_SUPPORT_ROAMING
	struct ROAMING_INFO *prRoamingFsmInfo;
#endif
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
#if CFG_SUPPORT_802_11K
	struct BSS_DESC *prBssDesc = NULL;
	struct LINK *prBSSDescList =
		&prAdapter->rWifiVar.rScanInfo.rBSSDescList;
#endif
	struct BSS_INFO *prAisBssInfo;
	struct PARAM_BSSID_EX *prCurrBssid;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prAisSpecificBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
#if CFG_SUPPORT_ROAMING
	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);
#endif
	prCurrBssid = aisGetCurrBssId(prAdapter, ucBssIndex);

	kalMemZero(prAisBssInfo->aucBSSID, MAC_ADDR_LEN);
	kalMemZero(prAisSpecificBssInfo->aucCurrentApAddr, MAC_ADDR_LEN);
	kalMemZero(prCurrBssid, sizeof(*prCurrBssid));
	prAisBssInfo->ucSSIDLen = 0;
	prAisFsmInfo->ucConnTrialCount = 0;
	prAdapter->rAddRoamScnChnl.ucChannelListNum = 0;
	prAisFsmInfo->ucConnTrialCountLimit = 0;

#if CFG_SUPPORT_ROAMING
	prAisFsmInfo->ucIsStaRoaming = FALSE;
	if (prRoamingFsmInfo != NULL)
		prRoamingFsmInfo->eReason = ROAMING_REASON_POOR_RCPI;
#endif
	aisRemoveDeauthBlocklist(prAdapter, TRUE);
	aisRemoveArpNRBlocklist(prAdapter, 0);

	aisClearAllLink(prAisFsmInfo);

	aisRemoveTimeoutBlocklist(prAdapter, 0);

#if CFG_SUPPORT_NCHO
	wlanNchoInit(prAdapter, TRUE);
#endif

#if CFG_SUPPORT_802_11W
	prAisSpecificBssInfo->prTargetComebackBssDesc = NULL;
	rsnStopSaQuery(prAdapter, ucBssIndex);
#endif

#if CFG_SUPPORT_NCHO
	aisFsmNotifyManageChannelList(prAdapter, ucBssIndex);
#endif

	/* reset BTM Params when disconnect */
	aisResetBssTranstionMgtParam(prAdapter, ucBssIndex);

#if CFG_SUPPORT_802_11K
	/* clear query done flag */
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry,
		struct BSS_DESC) {
		prBssDesc->fgQueriedCandidates = FALSE;
	}
#endif

	kalMemZero(&prRoamingFsmInfo->rSkipBtmInfo,
		sizeof(struct ROAMING_SKIP_CONFIG));
	prRoamingFsmInfo->fgDisallowBtmRoaming = FALSE;

#if (CFG_SUPPORT_ML_RECONFIG == 1)
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rApRemovalTimer);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* free allocated memory for assoc IE and FT IE */
	aisFreeIesMem(prAdapter, ucBssIndex, FALSE);

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	if (prConnSettings)
		kalMemZero(&prConnSettings->rErpKey,
			   sizeof(prConnSettings->rErpKey));
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

#if CFG_SUPPORT_SCAN_LOG
	scanAbortBeaconRecv(prAdapter,
		ucBssIndex,
		ABORT_DISCONNECT);
#endif

#if CFG_EXT_SCAN
	if (prAisFsmInfo->ucReasonOfDisconnect ==
		DISCONNECT_REASON_CODE_RADIO_LOST ||
		prAisFsmInfo->ucReasonOfDisconnect ==
		DISCONNECT_REASON_CODE_RADIO_LOST_TX_ERR) {
		scanRemoveBssDescByBssid(prAdapter,
			prAisBssInfo->aucBSSID);

		/* remove from scanning results as well */
		wlanClearBssInScanningResult(prAdapter,
			prAisBssInfo->aucBSSID);
	}
#endif
	prAisFsmInfo->ucIsSapCsaPending = FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate the Media State to HOST
 *
 * @param[in] eConnectionState   Current Media State
 * @param[in] fgDelayIndication  Set TRUE for postponing the Disconnect
 *                               Indication.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void
aisIndicationOfMediaStateToHost(struct ADAPTER *prAdapter,
				enum ENUM_PARAM_MEDIA_STATE eConnectionState,
				u_int8_t fgDelayIndication,
				uint8_t ucBssIndex)
{
	struct EVENT_CONNECTION_STATUS rEventConnStatus;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct STA_RECORD *prStaRec;

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prStaRec = aisGetStaRecOfAP(prAdapter, ucBssIndex);

	/* For indicating the Disconnect Event only if current media state is
	 * disconnected and we didn't do indication yet.
	 */
	DBGLOG(AIS, INFO,
		"[%d] Current state: %d, connection state %d: indicated: %d to %d\n",
		ucBssIndex,
		prAisFsmInfo->eCurrentState,
		prAisBssInfo->eConnectionState,
		prAisBssInfo->eConnectionStateIndicated,
		eConnectionState);

	if (prAisBssInfo->eConnectionState == MEDIA_STATE_DISCONNECTED &&
		/* if receive DEAUTH in JOIN state, report disconnect*/
		!(prAisFsmInfo->ucReasonOfDisconnect ==
		 DISCONNECT_REASON_CODE_DEAUTHENTICATED &&
		 prAisFsmInfo->eCurrentState == AIS_STATE_JOIN)) {
		if (prAisBssInfo->eConnectionStateIndicated ==
						eConnectionState) {
			return;
		}
	}

	if (!fgDelayIndication) {
		/* 4 <0> Cancel Delay Timer */
		prAisFsmInfo->u4PostponeIndStartTime = 0;

		/* 4 <1> Fill EVENT_CONNECTION_STATUS */
		rEventConnStatus.ucMediaStatus = (uint8_t) eConnectionState;

		if (eConnectionState == MEDIA_STATE_CONNECTED) {
#if (CFG_STAINFO_FEATURE == 1)
			prAisFsmInfo->u2ConnectedCount++;
#endif
			rEventConnStatus.ucReasonOfDisconnect =
			    DISCONNECT_REASON_CODE_RESERVED;

			if (prAisBssInfo->eCurrentOPMode ==
			    OP_MODE_INFRASTRUCTURE) {
				rEventConnStatus.ucInfraMode =
				    (uint8_t) NET_TYPE_INFRA;
				rEventConnStatus.u2AID =
				    prAisBssInfo->u2AssocId;
				rEventConnStatus.u2ATIMWindow = 0;
			} else if (prAisBssInfo->eCurrentOPMode ==
				OP_MODE_IBSS) {
				rEventConnStatus.ucInfraMode =
				    (uint8_t) NET_TYPE_IBSS;
				rEventConnStatus.u2AID = 0;
				rEventConnStatus.u2ATIMWindow =
				    prAisBssInfo->u2ATIMWindow;
			} else {
				DBGLOG(AIS, WARN,
					"Invalid operation mode: %d",
					prAisBssInfo->eCurrentOPMode);
				return;
			}

			COPY_SSID(rEventConnStatus.aucSsid,
				  rEventConnStatus.ucSsidLen,
				  prConnSettings->aucSSID,
				  prConnSettings->ucSSIDLen);

			COPY_MAC_ADDR(rEventConnStatus.aucBssid,
			      prAisBssInfo->aucBSSID);

			rEventConnStatus.u2BeaconPeriod =
			    prAisBssInfo->u2BeaconInterval;
			rEventConnStatus.u4FreqInKHz =
			    nicChannelNum2Freq(
					prAisBssInfo->ucPrimaryChannel,
					prAisBssInfo->eBand);
			rEventConnStatus.ucEncryptStatus =
			    prAisBssInfo->u2CapInfo & CAP_INFO_PRIVACY ? 1 : 0;

			switch (prAisBssInfo->ucNonHTBasicPhyType) {
			case PHY_TYPE_HR_DSSS_INDEX:
				rEventConnStatus.ucNetworkType =
				    (uint8_t) PARAM_NETWORK_TYPE_DS;
				break;

			case PHY_TYPE_ERP_INDEX:
				rEventConnStatus.ucNetworkType =
				    (uint8_t) PARAM_NETWORK_TYPE_OFDM24;
				break;

			case PHY_TYPE_OFDM_INDEX:
				rEventConnStatus.ucNetworkType =
				    (uint8_t) PARAM_NETWORK_TYPE_OFDM5;
				break;

			default:
				rEventConnStatus.ucNetworkType =
				    (uint8_t) PARAM_NETWORK_TYPE_DS;
				break;
			}
		} else {
			rEventConnStatus.ucReasonOfDisconnect =
			    prAisFsmInfo->ucReasonOfDisconnect;
		}

#if (CFG_SUPPORT_CONN_LOG == 1)
		connLogDisconnect(prAdapter,
			prAisBssInfo->ucBssIndex);
#endif

		/* 4 <2> Indication */
		nicMediaStateChange(prAdapter,
				    prAisBssInfo->ucBssIndex,
				    &rEventConnStatus);


		prAisBssInfo->eConnectionStateIndicated = eConnectionState;

		if (eConnectionState == MEDIA_STATE_DISCONNECTED) {
#if (CFG_STAINFO_FEATURE == 1)
			prAisFsmInfo->u2ConnectedCount = 0;
#endif
			aisFsmDisconnectedAction(prAdapter, ucBssIndex);
		}
	} else {
		DBGLOG(AIS, INFO,
		       "Postpone the indication of Disconnect for %d seconds\n",
		       prConnSettings->ucDelayTimeOfDisconnectEvent);

		prAisFsmInfo->u4PostponeIndStartTime = kalGetTimeTick();
	}
}				/* end of aisIndicationOfMediaStateToHost() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate an Event of "Media Disconnect" to HOST
 *
 * @param[in] u4Param  Unused timer parameter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisPostponedEventOfDisconnTimeout(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex)
{
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct AIS_FSM_INFO *prAisFsmInfo;
	bool fgFound = TRUE;
	bool fgIsPostponeTimeout;
	enum ENUM_PARAM_CONNECTION_POLICY policy;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	/* firstly, check if we have started postpone indication.
	 ** otherwise, give a chance to do join before indicate to host
	 **/
	if (prAisFsmInfo->u4PostponeIndStartTime == 0)
		return;

	/* if we're in  req channel/join/search state,
	 * don't report disconnect.
	 */
	if (prAisFsmInfo->eCurrentState == AIS_STATE_JOIN ||
	    prAisFsmInfo->eCurrentState == AIS_STATE_SEARCH ||
	    prAisFsmInfo->eCurrentState == AIS_STATE_ROAMING ||
	    prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN)
		return;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	fgIsPostponeTimeout = !aisFsmIsInProcessPostpone(prAdapter, ucBssIndex);
	policy = prConnSettings->eConnectionPolicy;

	/* if we're in join failure state,
	 * report disconnect before report join failure.
	 */
	if (prAisFsmInfo->eCurrentState == AIS_STATE_JOIN_FAILURE)
		fgIsPostponeTimeout = TRUE;

	DBGLOG(AIS, EVENT, "policy %d, timeout %d, trial %d, limit %d\n",
		policy,	fgIsPostponeTimeout, prAisFsmInfo->ucConnTrialCount,
		prAisFsmInfo->ucConnTrialCountLimit);

	/* only retry connect once when beacon timeout */
	if (!fgIsPostponeTimeout && !(prAisFsmInfo->ucConnTrialCount >
			prAisFsmInfo->ucConnTrialCountLimit)) {
		DBGLOG(AIS, INFO,
		       "DelayTimeOfDisconnect, don't report disconnect\n");
		return;
	}

	/* 4 <2> Remove all connection request */
	while (fgFound)
		fgFound = aisFsmClearRequest(prAdapter,
				AIS_REQUEST_RECONNECT, ucBssIndex);
	if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR ||
	    prAisFsmInfo->eCurrentState == AIS_STATE_JOIN_FAILURE)
		prAisFsmInfo->eCurrentState = AIS_STATE_IDLE;

	/* 4 <3> Indicate Disconnected Event to Host immediately. */
	aisIndicationOfMediaStateToHost(prAdapter,
					MEDIA_STATE_DISCONNECTED, FALSE,
					ucBssIndex);
}				/* end of aisPostponedEventOfDisconnTimeout() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will update the contain of BSS_INFO_T for AIS
 *        network once the association was completed.
 *
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 * @param[in] prAssocRspSwRfb        Pointer to SW RFB of ASSOC RESP FRAME.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForJOIN(struct ADAPTER *prAdapter,
			     struct STA_RECORD *prStaRec,
			     struct SW_RFB *prAssocRspSwRfb)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct WLAN_ASSOC_RSP_FRAME *prAssocRspFrame;
	struct BSS_DESC *prBssDesc;
	uint16_t u2IELength;
	const uint8_t *pucIE;
	uint8_t ucBssIndex = 0;
	uint16_t u2RxAssocId;

	ucBssIndex = prStaRec->ucBssIndex;
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prAisSpecBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
	prAssocRspFrame =
	    (struct WLAN_ASSOC_RSP_FRAME *)prAssocRspSwRfb->pvHeader;

	DBGLOG(AIS, INFO,
		"[%d] Update AIS_BSS_INFO_T and apply settings to MAC\n",
		ucBssIndex);

	u2RxAssocId = prAssocRspFrame->u2AssocId;
	if ((u2RxAssocId & BIT(6)) && (u2RxAssocId & BIT(7))
	    && !(u2RxAssocId & BITS(8, 15))) {
		prStaRec->u2AssocId = u2RxAssocId & ~BITS(6, 7);
	} else {
		prStaRec->u2AssocId = u2RxAssocId & ~AID_MSB;
#if CFG_SUPPORT_802_11W
		if (prStaRec->eStaType == STA_TYPE_LEGACY_AP) {
			struct AIS_SPECIFIC_BSS_INFO *prBssSpecInfo;

			prBssSpecInfo =
				aisGetAisSpecBssInfo(prAdapter,
				prStaRec->ucBssIndex);
			prBssSpecInfo->ucSaQueryTimedOut = 0;
		}
#endif
	}

	/* 3 <1> Update BSS_INFO_T from AIS_FSM_INFO_T or User Settings */
	/* 4 <1.1> Setup Operation Mode */
	prAisBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;

	/* Store limitation about 40Mhz bandwidth capability during
	 * association.
	 */
	prAisBssInfo->fg40mBwAllowed = prAisBssInfo->fgAssoc40mBwAllowed;
	prAisBssInfo->fgAssoc40mBwAllowed = FALSE;

	/* 4 <1.2> Setup SSID */
	COPY_SSID(prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen,
		  prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

	/* 4 <1.3> Setup Channel, Band */
	prAisBssInfo->ucPrimaryChannel = prBssDesc->ucChannelNum;
	prAisBssInfo->eBand = prBssDesc->eBand;

	/* 3 <2> Update BSS_INFO_T from STA_RECORD_T */
	/* 4 <2.1> Save current AP's STA_RECORD_T and current AID */
	prAisBssInfo->prStaRecOfAP = prStaRec;
	prAisBssInfo->u2AssocId = prStaRec->u2AssocId;

	/* 4 <2.2> Setup Capability */
	/* Use AP's Cap Info as BSS Cap Info */
	prAisBssInfo->u2CapInfo = prStaRec->u2CapInfo;

	if (prAisBssInfo->u2CapInfo & CAP_INFO_SHORT_PREAMBLE)
		prAisBssInfo->fgIsShortPreambleAllowed = TRUE;
	else
		prAisBssInfo->fgIsShortPreambleAllowed = FALSE;

#if CFG_SUPPORT_TDLS
	prAisBssInfo->fgTdlsIsProhibited = prStaRec->fgTdlsIsProhibited;
	prAisBssInfo->fgTdlsIsChSwProhibited = prStaRec->fgTdlsIsChSwProhibited;
#endif /* CFG_SUPPORT_TDLS */
#if CFG_SUPPORT_TDLS_AUTO
	/* fire the update jiffies */
	prAisBssInfo->ulLastUpdate = kalGetJiffies();
#endif

	/* 4 <2.3> Setup PHY Attributes and Basic Rate Set/Operational
	 * Rate Set
	 */
	prAisBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

	prAisBssInfo->ucNonHTBasicPhyType = prStaRec->ucNonHTBasicPhyType;

	prAisBssInfo->u2OperationalRateSet = prStaRec->u2OperationalRateSet;
	prAisBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

	nicTxUpdateBssDefaultRate(prAisBssInfo);

	/* 3 <3> Update BSS_INFO_T from SW_RFB_T (Association Resp Frame) */
	/* 4 <3.1> Setup BSSID */
	COPY_MAC_ADDR(prAisBssInfo->aucBSSID, prBssDesc->aucBSSID);
	COPY_MAC_ADDR(prAisSpecBssInfo->aucCurrentApAddr,
		      cnmStaRecAuthAddr(prAdapter, prStaRec));

	u2IELength =
	    (uint16_t) ((prAssocRspSwRfb->u2PacketLen -
			 prAssocRspSwRfb->u2HeaderLen) -
			(OFFSET_OF(struct WLAN_ASSOC_RSP_FRAME, aucInfoElem[0])
			 - WLAN_MAC_MGMT_HEADER_LEN));
	pucIE = prAssocRspFrame->aucInfoElem;

	/* 4 <3.2> Parse WMM and setup QBSS flag */
	/* Parse WMM related IEs and configure HW CRs accordingly */
	mqmProcessAssocRsp(prAdapter, prAssocRspSwRfb, pucIE, u2IELength);

	prAisBssInfo->fgIsQBSS = prStaRec->fgIsQoS;

	prBssDesc->fgIsConnecting &= ~BIT(ucBssIndex);
	prBssDesc->fgIsConnected |= BIT(ucBssIndex);
	prBssDesc->ucJoinFailureCount = 0;

	if (prBssDesc->prBlock &&
	    prBssDesc->prBlock->ucDeauthCount == 0 &&
	    prBssDesc->prBlock->fgArpNoResponse == FALSE)
		aisRemoveBlockList(prAdapter, prBssDesc);
	/* 4 <4.1> Setup MIB for current BSS */
	prAisBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;
#if (CFG_SUPPORT_802_11V_MBSSID == 1)
	prAisBssInfo->ucMaxBSSIDIndicator =
				prBssDesc->ucMaxBSSIDIndicator;
	prAisBssInfo->ucMBSSIDIndex = prBssDesc->ucMBSSIDIndex;
#endif

	/* NOTE: Defer ucDTIMPeriod updating to when beacon is received
	 * after connection
	 */
	prAisBssInfo->ucDTIMPeriod = 0;
	prAisBssInfo->fgTIMPresent = TRUE;
	prAisBssInfo->u2ATIMWindow = 0;

	prAisBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_INFRA;

	/* init rekey info */
	GET_CURRENT_SYSTIME(&prAisBssInfo->rRekeyCountResetTime);
	prAisBssInfo->u2RekeyCount = 0;

	/*reset coex related info*/
	prAisBssInfo->eCoexMode = COEX_NONE_BT;

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
	prAisSpecBssInfo->ucRoamSkipTimes = ROAMING_ONE_AP_SKIP_TIMES;
	prAisSpecBssInfo->fgGoodRcpiArea = FALSE;
	prAisSpecBssInfo->fgPoorRcpiArea = FALSE;
#endif

	/* 4 <4.2> Update HT information and set channel */
	/* Record HT related parameters in rStaRec and rBssInfo
	 * Note: it shall be called before nicUpdateBss()
	 */
	rlmProcessAssocRsp(prAdapter, prAssocRspSwRfb, pucIE, u2IELength);
	/* staRec phyTypeSet may be updated if it is different in beacon
	 * and assoc resp. Therefore, should update bssInfo again.
	 */
	prAisBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

	secPostUpdateAddr(prAdapter,
		aisGetAisBssInfo(prAdapter, ucBssIndex));

	/* 4 <4.3> Sync with firmware for BSS-INFO */
	prAisBssInfo->ucBMCWlanIndex = secPrivacySeekForBcEntry(
				prAdapter, prAisBssInfo->ucBssIndex,
				prAisBssInfo->aucOwnMacAddr,
				prStaRec->ucIndex,
				CIPHER_SUITE_NONE, 0xFF);

	nicUpdateBss(prAdapter, ucBssIndex);

	nicUpdateQos(prAdapter, prStaRec);

	/* 4 <4.4> *DEFER OPERATION* nicPmIndicateBssConnected()
	 * will be invoked
	 */
	/* inside scanProcessBeaconAndProbeResp() after 1st beacon
	 * is received
	 */
}				/* end of aisUpdateBssInfoForJOIN() */

void aisUpdateAllBssInfoForJOIN(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	struct SW_RFB *prAssocRspSwRfb,
	struct STA_RECORD *prSetupStaRec)
{
	uint8_t i;

	/* update bssinfo */
	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct STA_RECORD *prStaRec =
			aisGetLinkStaRec(prAisFsmInfo, i);

		if (!prStaRec)
			continue;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (prStaRec == prSetupStaRec) {
			aisUpdateBssInfoForJOIN(prAdapter,
				prStaRec, prAssocRspSwRfb);
		} else {
			struct SW_RFB *prSwRfb = NULL;

			if (prAssocRspSwRfb->u2PacketLen <
			    sizeof(struct WLAN_ASSOC_RSP_FRAME))
				break;

			prSwRfb = mldDupAssocSwRfb(prAdapter,
				prAssocRspSwRfb, prStaRec);

			if (prSwRfb) {
				aisUpdateBssInfoForJOIN(prAdapter,
					prStaRec, prSwRfb);
				nicRxReturnRFB(prAdapter, prSwRfb);
			}
		}
#else
		aisUpdateBssInfoForJOIN(prAdapter,
			prStaRec, prAssocRspSwRfb);
#endif

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
		/* Activate current AP's STA_RECORD_T in Driver. */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
#endif

		/* Update RSSI if necessary */
		nicUpdateRSSI(prAdapter, prStaRec->ucBssIndex,
			      (int8_t) (RCPI_TO_dBm(prStaRec->ucRCPI)), 0);
	}

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 0)
	/* update starec */
	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct STA_RECORD *prStaRec =
			aisGetLinkStaRec(prAisFsmInfo, i);

		if (!prStaRec)
			continue;

		/* Activate current AP's STA_RECORD_T in Driver. */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
	}
#endif
}

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will create an Ad-Hoc network and start sending
 *        Beacon Frames.
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForCreateIBSS(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	if (prAisBssInfo->fgIsBeaconActivated)
		return;

	/* 3 <1> Update BSS_INFO_T per Network Basis */
	/* 4 <1.1> Setup Operation Mode */
	prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

	/* 4 <1.2> Setup SSID */
	COPY_SSID(prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen,
		  prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

	/* 4 <1.3> Clear current AP's STA_RECORD_T and current AID */
	prAisBssInfo->prStaRecOfAP = (struct STA_RECORD *)NULL;
	prAisBssInfo->u2AssocId = 0;

	/* 4 <1.4> Setup Channel, Band and Phy Attributes */
	prAisBssInfo->ucPrimaryChannel = prConnSettings->ucAdHocChannelNum;
	prAisBssInfo->eBand = prConnSettings->eAdHocBand;

	if (prAisBssInfo->eBand == BAND_2G4) {
		/* Depend on eBand */
		prAisBssInfo->ucPhyTypeSet =
		    prAdapter->
		    rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11BGN;
		/* Depend on eCurrentOPMode and ucPhyTypeSet */
		prAisBssInfo->ucConfigAdHocAPMode = AD_HOC_MODE_MIXED_11BG;
	} else {
		/* Depend on eBand */
		prAisBssInfo->ucPhyTypeSet =
		    prAdapter->
		    rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11ANAC;
		/* Depend on eCurrentOPMode and ucPhyTypeSet */
		prAisBssInfo->ucConfigAdHocAPMode = AD_HOC_MODE_11A;
	}

	/* 4 <1.5> Setup MIB for current BSS */
	prAisBssInfo->u2BeaconInterval = prConnSettings->u2BeaconPeriod;
	prAisBssInfo->ucDTIMPeriod = 0;
	prAisBssInfo->u2ATIMWindow = prConnSettings->u2AtimWindow;

	prAisBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_ADHOC;

	if (prConnSettings->eEncStatus == ENUM_ENCRYPTION1_ENABLED ||
	    prConnSettings->eEncStatus == ENUM_ENCRYPTION2_ENABLED ||
	    prConnSettings->eEncStatus == ENUM_ENCRYPTION3_ENABLED ||
	    prConnSettings->eEncStatus == ENUM_ENCRYPTION4_ENABLED) {
		prAisBssInfo->fgIsProtection = TRUE;
	} else {
		prAisBssInfo->fgIsProtection = FALSE;
	}

	/* 3 <2> Update BSS_INFO_T common part */
	ibssInitForAdHoc(prAdapter, prAisBssInfo);
	/* 4 <2.1> Initialize client list */
	bssInitializeClientList(prAdapter, prAisBssInfo);

	/* 3 <3> Set MAC HW */
	/* 4 <3.1> Setup channel and bandwidth */
	rlmBssInitForAPandIbss(prAdapter, prAisBssInfo);

	/* 4 <3.2> use command packets to inform firmware */
	nicUpdateBss(prAdapter, prAisBssInfo->ucBssIndex);

	/* 4 <3.3> enable beaconing */
	bssUpdateBeaconContent(prAdapter, prAisBssInfo->ucBssIndex);

	/* 4 <3.4> Update AdHoc PM parameter */
	nicPmIndicateBssCreated(prAdapter, prAisBssInfo->ucBssIndex);

	/* 3 <4> Set ACTIVE flag. */
	prAisBssInfo->fgIsBeaconActivated = TRUE;
	prAisBssInfo->fgHoldSameBssidForIBSS = TRUE;

	/* 3 <5> Start IBSS Alone Timer */
	cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer,
			   SEC_TO_MSEC(AIS_IBSS_ALONE_TIMEOUT_SEC));
}				/* end of aisCreateIBSS() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will update the contain of BSS_INFO_T for
 *        AIS network once the existing IBSS was found.
 *
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForMergeIBSS(struct ADAPTER *prAdapter,
				  struct STA_RECORD *prStaRec)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_DESC *prBssDesc;
	uint8_t ucBssIndex = 0;

	ucBssIndex = prStaRec->ucBssIndex;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer);

	if (!prAisBssInfo->fgIsBeaconActivated) {

		/* 3 <1> Update BSS_INFO_T per Network Basis */
		/* 4 <1.1> Setup Operation Mode */
		prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

		/* 4 <1.2> Setup SSID */
		COPY_SSID(prAisBssInfo->aucSSID,
			  prAisBssInfo->ucSSIDLen, prConnSettings->aucSSID,
			  prConnSettings->ucSSIDLen);

		/* 4 <1.3> Clear current AP's STA_RECORD_T and current AID */
		prAisBssInfo->prStaRecOfAP = (struct STA_RECORD *)NULL;
		prAisBssInfo->u2AssocId = 0;
	}
	/* 3 <2> Update BSS_INFO_T from STA_RECORD_T */
	/* 4 <2.1> Setup Capability */
	/* Use Peer's Cap Info as IBSS Cap Info */
	prAisBssInfo->u2CapInfo = prStaRec->u2CapInfo;

	if (prAisBssInfo->u2CapInfo & CAP_INFO_SHORT_PREAMBLE) {
		prAisBssInfo->fgIsShortPreambleAllowed = TRUE;
		prAisBssInfo->fgUseShortPreamble = TRUE;
	} else {
		prAisBssInfo->fgIsShortPreambleAllowed = FALSE;
		prAisBssInfo->fgUseShortPreamble = FALSE;
	}

	/* 7.3.1.4 For IBSS, the Short Slot Time subfield shall be set to 0. */
	/* Set to FALSE for AdHoc */
	prAisBssInfo->fgUseShortSlotTime = FALSE;
	prAisBssInfo->u2CapInfo &= ~CAP_INFO_SHORT_SLOT_TIME;

	if (prAisBssInfo->u2CapInfo & CAP_INFO_PRIVACY)
		prAisBssInfo->fgIsProtection = TRUE;
	else
		prAisBssInfo->fgIsProtection = FALSE;

	/* 4 <2.2> Setup PHY Attributes and Basic Rate Set/Operational
	 * Rate Set
	 */
	prAisBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

	prAisBssInfo->ucNonHTBasicPhyType = prStaRec->ucNonHTBasicPhyType;

	prAisBssInfo->u2OperationalRateSet = prStaRec->u2OperationalRateSet;
	prAisBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

	rateGetDataRatesFromRateSet(prAisBssInfo->u2OperationalRateSet,
				    prAisBssInfo->u2BSSBasicRateSet,
				    prAisBssInfo->aucAllSupportedRates,
				    &prAisBssInfo->ucAllSupportedRatesLen);

	/* 3 <3> X Update BSS_INFO_T from SW_RFB_T (Association Resp Frame) */

	/* 3 <4> Update BSS_INFO_T from BSS_DESC_T */
	prBssDesc = scanSearchBssDescByTA(prAdapter, prStaRec->aucMacAddr);
	if (prBssDesc) {
		prBssDesc->fgIsConnecting &= ~BIT(ucBssIndex);
		prBssDesc->fgIsConnected |= BIT(ucBssIndex);

		/* Support AP Selection */
		aisRemoveBlockList(prAdapter, prBssDesc);

		/* 4 <4.1> Setup BSSID */
		COPY_MAC_ADDR(prAisBssInfo->aucBSSID, prBssDesc->aucBSSID);

		/* 4 <4.2> Setup Channel, Band */
		prAisBssInfo->ucPrimaryChannel = prBssDesc->ucChannelNum;
		prAisBssInfo->eBand = prBssDesc->eBand;

		/* 4 <4.3> Setup MIB for current BSS */
		prAisBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;
		prAisBssInfo->ucDTIMPeriod = 0;
		prAisBssInfo->u2ATIMWindow = 0;	/* TBD(Kevin) */

		prAisBssInfo->ucBeaconTimeoutCount =
		    AIS_BEACON_TIMEOUT_COUNT_ADHOC;
	}

	/* 3 <5> Set MAC HW */
	/* 4 <5.1> Find Lowest Basic Rate Index for default TX Rate of MMPDU */
	nicTxUpdateBssDefaultRate(prAisBssInfo);

	/* 4 <5.2> Setup channel and bandwidth */
	rlmBssInitForAPandIbss(prAdapter, prAisBssInfo);

	/* 4 <5.3> use command packets to inform firmware */
	nicUpdateBss(prAdapter, prAisBssInfo->ucBssIndex);

	/* 4 <5.4> enable beaconing */
	bssUpdateBeaconContent(prAdapter, prAisBssInfo->ucBssIndex);

	/* 4 <5.5> Update AdHoc PM parameter */
	nicPmIndicateBssConnected(prAdapter,
				  prAisBssInfo->ucBssIndex);

	/* 3 <6> Set ACTIVE flag. */
	prAisBssInfo->fgIsBeaconActivated = TRUE;
	prAisBssInfo->fgHoldSameBssidForIBSS = TRUE;
}				/* end of aisUpdateBssInfoForMergeIBSS() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Probe Request Frame and then return
 *        result to BSS to indicate if need to send the corresponding
 *         Probe Response Frame if the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[out] pu4ControlFlags   Control flags for replying the Probe Response
 *
 * @retval TRUE      Reply the Probe Response
 * @retval FALSE     Don't reply the Probe Response
 */
/*----------------------------------------------------------------------------*/
u_int8_t aisValidateProbeReq(struct ADAPTER *prAdapter,
			     struct SW_RFB *prSwRfb,
			     uint8_t ucBssIndex,
			     uint32_t *pu4ControlFlags)
{
	struct WLAN_MAC_MGMT_HEADER *prMgtHdr;
	struct BSS_INFO *prBssInfo;
	struct IE_SSID *prIeSsid = (struct IE_SSID *)NULL;
	uint8_t *pucIE;
	uint16_t u2IELength;
	uint16_t u2Offset = 0;
	u_int8_t fgReplyProbeResp = FALSE;

	prBssInfo = aisGetAisBssInfo(prAdapter,
		ucBssIndex);

	/* 4 <1> Parse Probe Req IE and Get IE ptr
	 * (SSID, Supported Rate IE, ...)
	 */
	prMgtHdr = (struct WLAN_MAC_MGMT_HEADER *)prSwRfb->pvHeader;

	u2IELength = prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen;
	pucIE =
	    (uint8_t *) ((uintptr_t)prSwRfb->pvHeader +
			 prSwRfb->u2HeaderLen);

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		if (IE_ID(pucIE) == ELEM_ID_SSID) {
			if ((!prIeSsid) && (IE_LEN(pucIE) <= ELEM_MAX_LEN_SSID))
				prIeSsid = (struct IE_SSID *)pucIE;

			break;
		}
	}			/* end of IE_FOR_EACH */

	/* 4 <2> Check network conditions */

	if (prBssInfo->eCurrentOPMode == OP_MODE_IBSS) {

		if ((prIeSsid) && ((prIeSsid->ucLength ==
			BC_SSID_LEN) ||	/* WILDCARD SSID */
			EQUAL_SSID(prBssInfo->aucSSID,
			prBssInfo->ucSSIDLen,	/* CURRENT SSID */
			prIeSsid->aucSSID,
			prIeSsid->ucLength))) {
			fgReplyProbeResp = TRUE;
		}
	}

	return fgReplyProbeResp;

}				/* end of aisValidateProbeReq() */

#endif /* CFG_SUPPORT_ADHOC */

void aisFsmDisconnectAllBss(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *prAisBssInfo =
			aisGetLinkBssInfo(prAisFsmInfo, i);

		if (!prAisBssInfo)
			continue;

		aisChangeMediaState(prAisBssInfo, MEDIA_STATE_DISCONNECTED);

		/* 4 <4.1> sync. with firmware */
		nicUpdateBss(prAdapter, prAisBssInfo->ucBssIndex);
		prAisBssInfo->prStaRecOfAP = (struct STA_RECORD *)NULL;
		prAisBssInfo->ucLinkIndex = 0;
	}
}

void aisFsmRemoveAllBssDesc(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *prAisBssInfo =
			aisGetLinkBssInfo(prAisFsmInfo, i);

		if (!prAisBssInfo)
			continue;

#if CFG_EXT_SCAN

		scanRemoveConnFlagOfBssDescByBssid(
			prAdapter,
			prAisBssInfo->aucBSSID,
			prAisBssInfo->ucBssIndex);

		if (prAisFsmInfo->ucReasonOfDisconnect ==
			DISCONNECT_REASON_CODE_RADIO_LOST ||
			prAisFsmInfo->ucReasonOfDisconnect ==
			DISCONNECT_REASON_CODE_RADIO_LOST_TX_ERR) {
			struct SCAN_INFO *prScanInfo;
			struct LINK *prBSSDescList;
			struct BSS_DESC *prBssDesc;

			prScanInfo =
				&(prAdapter->rWifiVar.rScanInfo);
			prBSSDescList =
				&prScanInfo->rBSSDescList;
			LINK_FOR_EACH_ENTRY(prBssDesc,
				prBSSDescList,
				rLinkEntry, struct BSS_DESC) {

				if (EQUAL_MAC_ADDR(
					prBssDesc->aucBSSID,
					prAisBssInfo->aucBSSID)) {
					DBGLOG(AIS, INFO,
						""MACSTR" set BTO flag",
						MAC2STR(prBssDesc->aucBSSID));
					prBssDesc->fgIsInBTO = TRUE;
					break;
				}
			}
		}

#else

		if (prAisFsmInfo->ucReasonOfDisconnect ==
			DISCONNECT_REASON_CODE_RADIO_LOST ||
		    prAisFsmInfo->ucReasonOfDisconnect ==
			DISCONNECT_REASON_CODE_RADIO_LOST_TX_ERR) {
			scanRemoveBssDescByBssid(prAdapter,
						 prAisBssInfo->aucBSSID);

			/* remove from scanning results as well */
			wlanClearBssInScanningResult(prAdapter,
						     prAisBssInfo->aucBSSID);
		} else {
			scanRemoveConnFlagOfBssDescByBssid(prAdapter,
			      prAisBssInfo->aucBSSID, prAisBssInfo->ucBssIndex);
		}

#endif
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will modify and update necessary information to firmware
 *        for disconnection handling
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 *
 * @retval None
 */
/*----------------------------------------------------------------------------*/
void aisFsmDisconnect(struct ADAPTER *prAdapter,
		u_int8_t fgDelayIndication, uint8_t ucBssIndex)
{
	struct BSS_INFO *prAisBssInfo;
	uint16_t u2ReasonCode = REASON_CODE_UNSPECIFIED;
	struct AIS_FSM_INFO *prAisFsmInfo = NULL;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

#if (CFG_SUPPORT_TWT == 1)
	twtPlannerReset(prAdapter, prAisBssInfo);
#endif

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
	cnmTimerStopTimer(prAdapter,
		aisGetSecModeChangeTimer(prAdapter, ucBssIndex));
#endif
#if CFG_SUPPORT_DFS
	aisFunSwitchChannelAbort(prAdapter, prAisFsmInfo, TRUE);
#endif
	cnmTimerStopTimer(prAdapter, &prAisBssInfo->rObssScanTimer);

	nicPmIndicateBssAbort(prAdapter, prAisBssInfo->ucBssIndex);

#if CFG_SUPPORT_ADHOC
	if (prAisBssInfo->fgIsBeaconActivated) {
		nicUpdateBeaconIETemplate(prAdapter,
					  IE_UPD_METHOD_DELETE_ALL,
					  prAisBssInfo->ucBssIndex,
					  0, NULL, 0);

		prAisBssInfo->fgIsBeaconActivated = FALSE;
	}
#endif

	rlmBssAborted(prAdapter, prAisBssInfo);

	/* 4 <3> Unset the fgIsConnected flag of BSS_DESC_T and send Deauth
	 * if needed.
	 */
	if (prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {

		{
			if (prAdapter->rWifiVar.ucTpTestMode !=
			    ENUM_TP_TEST_MODE_NORMAL)
				nicEnterTPTestMode(prAdapter, TEST_MODE_NONE);

#if 0
			if (prAdapter->rWifiVar.ucSigmaTestMode)
				nicEnterTPTestMode(prAdapter, TEST_MODE_NONE);
#endif
		}
		/* for NO TIM IE case */
		if (!prAisBssInfo->fgTIMPresent) {
			nicConfigPowerSaveProfile(prAdapter,
						  prAisBssInfo->ucBssIndex,
						  Param_PowerModeFast_PSP,
						  FALSE, PS_CALLER_NO_TIM);
		}

		if (prAisBssInfo->prStaRecOfAP)
			u2ReasonCode = prAisBssInfo->prStaRecOfAP->u2ReasonCode;

		aisFsmRemoveAllBssDesc(prAdapter, prAisFsmInfo);

		if (fgDelayIndication) {
#if CFG_SUPPORT_ROAMING
			struct ROAMING_INFO *roam =
				aisGetRoamingInfo(prAdapter, ucBssIndex);

			/*
			 * There is a chance that roaming failed before
			 * beacon timeout, so reset trial count here to
			 * ensure the new reconnection runs correctly.
			 */
			prAisFsmInfo->ucConnTrialCount = 0;
			GET_CURRENT_SYSTIME(&(prAisFsmInfo->rJoinReqTime));

			switch (prAisFsmInfo->ucReasonOfDisconnect) {
			case DISCONNECT_REASON_CODE_RADIO_LOST:
				roam->eReason = ROAMING_REASON_BEACON_TIMEOUT;
				prAisFsmInfo->ucConnTrialCountLimit = 2;
				break;
			case DISCONNECT_REASON_CODE_RADIO_LOST_TX_ERR:
				roam->eReason = ROAMING_REASON_TX_ERR;
				prAisFsmInfo->ucConnTrialCountLimit = 2;
				break;
			case DISCONNECT_REASON_CODE_DEAUTHENTICATED:
			case DISCONNECT_REASON_CODE_DISASSOCIATED:
				roam->eReason = ROAMING_REASON_SAA_FAIL;
				prAisFsmInfo->ucConnTrialCountLimit = 1;
				break;
			case DISCONNECT_REASON_CODE_REASSOCIATION:
			case DISCONNECT_REASON_CODE_ROAMING:
				roam->eReason =
					ROAMING_REASON_UPPER_LAYER_TRIGGER;
				prAisFsmInfo->ucConnTrialCountLimit = 2;
				break;
			case DISCONNECT_REASON_CODE_BTM:
				roam->eReason =	ROAMING_REASON_BTM;
				prAisFsmInfo->ucConnTrialCountLimit = 2;
				break;
			default:
				DBGLOG(AIS, ERROR, "wrong reason %d",
					prAisFsmInfo->ucReasonOfDisconnect);
			}
			aisFsmClearRequest(prAdapter,
				AIS_REQUEST_RECONNECT, ucBssIndex);
			aisFsmInsertRequest(prAdapter,
				AIS_REQUEST_RECONNECT, ucBssIndex);

			if (prAisBssInfo->eCurrentOPMode != OP_MODE_IBSS)
				prAisBssInfo->fgHoldSameBssidForIBSS = FALSE;
#endif
		} else {
			prAisBssInfo->fgHoldSameBssidForIBSS = FALSE;
		}
	} else {
		prAisBssInfo->fgHoldSameBssidForIBSS = FALSE;
	}

	/* 4 <4> Change Media State immediately. */
	aisFsmDisconnectAllBss(prAdapter, prAisFsmInfo);
	/* aisFsmRemoveAllBssDesc/aisFsmDisconnectAllBss already clear
	 * bssdesc and starec, so clear link info as well
	 */
	aisClearAllLink(prAisFsmInfo);

#if CFG_SUPPORT_ROAMING
	roamingFsmRunEventAbort(prAdapter, ucBssIndex);
	aisFsmRemoveRoamingRequest(prAdapter, ucBssIndex);
#if (CFG_SUPPORT_ROAMING_LOG == 1)
	roamingFsmLogCancel(prAdapter, ucBssIndex);
#endif
#endif /* CFG_SUPPORT_ROAMING */

	/* 4 <6> Indicate Disconnected Event to Host */
	aisIndicationOfMediaStateToHost(prAdapter,
					MEDIA_STATE_DISCONNECTED,
					fgDelayIndication,
					ucBssIndex);

#if (CFG_SUPPORT_ANDROID_DUAL_STA == 1)
	aisCheckMultiStaStatus(prAdapter, MEDIA_STATE_DISCONNECTED, ucBssIndex);
#endif
	/* 4 <7> Trigger AIS FSM */
	aisFsmSteps(prAdapter, AIS_STATE_IDLE, ucBssIndex);
}				/* end of aisFsmDisconnect() */

static void aisFsmRunEventScanDoneTimeOut(struct ADAPTER *prAdapter,
					  uintptr_t ulParam)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = (uint8_t) ulParam;
	struct SCAN_INFO *prScanInfo;

	ASSERT(prAdapter);

/* fos_change begin */
#if CFG_SUPPORT_EXCEPTION_STATISTICS
		prAdapter->total_scandone_timeout_count++;
#endif /* fos_change end */

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	DBGLOG(AIS, STATE,
		"[%d] aisFsmRunEventScanDoneTimeOut Current[%d] Seq=%u\n",
		ucBssIndex,
		prAisFsmInfo->eCurrentState, prAisFsmInfo->ucSeqNumOfScanReq);

	prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;
	kalSetHifDbgEvent(prAdapter->prGlueInfo);
	prScanInfo->fgIsScanTimeout = TRUE;

	/* try to stop scan in CONNSYS */
	aisFsmStateAbort_SCAN(prAdapter, ucBssIndex);

#if CFG_SUPPORT_SCAN_NO_AP_RECOVERY
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucScanNoApRecover))
		scnDoScanTimeoutRecoveryCheck(prAdapter, ucBssIndex);
#endif
}				/* end of aisFsmBGSleepTimeout() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate an Event of "Background Scan Time-Out"
 *        to AIS FSM.
 * @param[in] u4Param  Unused timer parameter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventBGSleepTimeOut(struct ADAPTER *prAdapter,
				  uintptr_t ulParamPtr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	uint8_t ucBssIndex = (uint8_t) ulParamPtr;
#if CFG_SUPPORT_802_11W
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
#endif
#if CFG_SUPPORT_ROAMING
	struct ROAMING_INFO *prRoamingFsmInfo =
			aisGetRoamingInfo(prAdapter, ucBssIndex);
#endif

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

#if CFG_SUPPORT_802_11W
	prAisSpecificBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
#endif

	eNextState = prAisFsmInfo->eCurrentState;

	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_WAIT_FOR_NEXT_SCAN:
		DBGLOG(AIS, LOUD,
			"[%d] EVENT - SCAN TIMER: Idle End - Current Time = %u\n",
			ucBssIndex,
			kalGetTimeTick());

#if CFG_SUPPORT_802_11W
		if (prAisSpecificBssInfo->prTargetComebackBssDesc) {
			eNextState = AIS_STATE_SEARCH;
			prAisSpecificBssInfo->prTargetComebackBssDesc = NULL;
		} else
#endif /* CFG_SUPPORT_802_11W */
		{
			eNextState = AIS_STATE_LOOKING_FOR;
#if CFG_SUPPORT_ROAMING
			if (prRoamingFsmInfo->rRoamScanParam.ucScanCount &&
			    prAisFsmInfo->ucScanTrialCount >=
				prRoamingFsmInfo->rRoamScanParam.ucScanCount) {
				DBGLOG(AIS, STATE,
					"Roaming scan retry :%d fail!\n",
					prAisFsmInfo->ucScanTrialCount);
				roamingFsmRunEventNewCandidate(prAdapter,
					NULL, ucBssIndex);
				roamingFsmRunEventFail(prAdapter,
					ROAMING_FAIL_REASON_NOCANDIDATE,
					ucBssIndex);

				/* reset retry count */
				prAisFsmInfo->ucConnTrialCount = 0;
				eNextState = AIS_STATE_NORMAL_TR;

			} else if (prRoamingFsmInfo->rRoamScanParam.ucScanCount)
				prAisFsmInfo->ucScanTrialCount++;
#endif
		}

		SET_NET_PWR_STATE_ACTIVE(prAdapter,
					 ucBssIndex);

		break;

	default:
		break;
	}

	/* Call aisFsmSteps() when we are going to change AIS STATE */
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState, ucBssIndex);
}				/* end of aisFsmBGSleepTimeout() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate an Event of "IBSS ALONE Time-Out" to
 *        AIS FSM.
 * @param[in] u4Param  Unused timer parameter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventIbssAloneTimeOut(struct ADAPTER *prAdapter,
				    uintptr_t ulParamPtr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	uint8_t ucBssIndex = (uint8_t) ulParamPtr;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	eNextState = prAisFsmInfo->eCurrentState;

	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_IBSS_ALONE:

		/* There is no one participate in our AdHoc during this
		 * TIMEOUT Interval so go back to search for a valid
		 * IBSS again.
		 */

		DBGLOG(AIS, LOUD,
			"[%d] EVENT-IBSS ALONE TIMER: Start pairing\n",
			ucBssIndex);

		/* abort timer */
		aisFsmReleaseCh(prAdapter, ucBssIndex);

		/* Pull back to SEARCH to find candidate again */
		eNextState = AIS_STATE_SEARCH;

		break;

	default:
		break;
	}

	/* Call aisFsmSteps() when we are going to change AIS STATE */
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState, ucBssIndex);
}				/* end of aisIbssAloneTimeOut() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate an Event of "Join Time-Out" to AIS FSM.
 *
 * @param[in] u4Param  Unused timer parameter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventJoinTimeout(struct ADAPTER *prAdapter,
			       uintptr_t ulParamPtr)
{
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	enum ENUM_AIS_STATE eNewState;
	OS_SYSTIME rCurrentTime;
	uint8_t ucBssIndex = (uint8_t) ulParamPtr;
	struct BSS_DESC *prBssDesc;
	struct STA_RECORD *prStaRec;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);

	eNextState = prAisFsmInfo->eCurrentState;

	GET_CURRENT_SYSTIME(&rCurrentTime);

	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_JOIN:
		DBGLOG(AIS, WARN, "EVENT- JOIN TIMEOUT\n");

		prStaRec = aisGetTargetStaRec(prAdapter, ucBssIndex);
		prStaRec->u2StatusCode = STATUS_CODE_AUTH_TIMEOUT;
		eNewState = aisHandleJoinFailure(prAdapter,
				prStaRec,
				NULL, ucBssIndex);
		break;

	case AIS_STATE_NORMAL_TR:
		/* 1. release channel */
		aisFsmReleaseCh(prAdapter, ucBssIndex);

#if CFG_ENABLE_WIFI_DIRECT
		if (prAisFsmInfo->ucIsSapCsaPending == TRUE) {
			ccmChannelSwitchProducer(prAdapter, prAisBssInfo,
						 __func__);
			prAisFsmInfo->ucIsSapCsaPending = FALSE;
		}
#endif

		eNewState = aisFsmHandleNextReq_NORMAL_TR(
			prAdapter, prAisFsmInfo, ucBssIndex);

#if CFG_SUPPORT_LOWLATENCY_MODE
		/* 5. Check if need to set low latency after connected. */
		wlanConnectedForLowLatency(prAdapter, ucBssIndex);
#endif

		break;

	default:
		/* release channel */
		aisFsmReleaseCh(prAdapter, ucBssIndex);
		eNewState = prAisFsmInfo->eCurrentState;
		break;

	}

	/* Call aisFsmSteps() when we are going to change AIS STATE */
	if (eNewState != eNextState)
		aisFsmSteps(prAdapter, eNewState, ucBssIndex);
}				/* end of aisFsmRunEventJoinTimeout() */

void aisFsmRunEventDeauthTimeout(struct ADAPTER *prAdapter,
				 uintptr_t ulParamPtr)
{
	uint8_t ucBssIndex = (uint8_t) ulParamPtr;

	aisDeauthXmitCompleteBss(prAdapter, ucBssIndex, TX_RESULT_LIFE_TIMEOUT);
}

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
void aisFsmRunEventSecModeChangeTimeout(struct ADAPTER *prAdapter,
					uintptr_t ulParamPtr)
{
	uint8_t ucBssIndex = (uint8_t) ulParamPtr;

	DBGLOG(AIS, INFO,
		"[%d] Beacon security mode change timeout, trigger disconnect!\n",
		ucBssIndex);

	aisBssSecurityChanged(prAdapter, ucBssIndex);
}
#endif

#if (CFG_SUPPORT_ML_RECONFIG == 1)
void aisFsmRunApRemovalTimeout(struct ADAPTER *prAdapter,
					uintptr_t ulParamPtr)
{
	uint8_t ucBssIndex = (uint8_t) ulParamPtr;
	struct AIS_FSM_INFO *prAisFsmInfo;
	uint8_t i;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	if (prAdapter->rWifiVar.fgApRemovalByT2LM) {
		struct MLD_STA_RECORD *prMldStaRec = NULL;

		DBGLOG(AIS, INFO,
			"BSS[%d] AP link removed by t2lm!\n",
			ucBssIndex);

		for (i = 0; i < MLD_LINK_MAX; i++) {
			struct STA_RECORD *prStaRec =
				aisGetLinkStaRec(prAisFsmInfo, i);

			if (!prStaRec)
				continue;

			if (prStaRec->fgApRemoval)
				prStaRec->ucULTidBitmap = 0;

			prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);

			DBGLOG(AIS, INFO,
				"StaRec[widx=%d] set t2lm 0x%x!\n",
				prStaRec->ucWlanIndex,
				prStaRec->ucULTidBitmap);
		}

		if (prMldStaRec)
			mldUpdateTidBitmap(prAdapter, prMldStaRec);
	} else {
		DBGLOG(AIS, INFO,
			"BSS[%d] AP link removed, trigger reconnect!\n",
			ucBssIndex);

		aisFsmStateAbort(prAdapter,
			DISCONNECT_REASON_CODE_RADIO_LOST,
			TRUE, ucBssIndex);
	}
}
#endif /* CFG_SUPPORT_ML_RECONFIG */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is used to handle OID_802_11_BSSID_LIST_SCAN
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 * \param[in] prRequestIn  scan request
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
aisFsmScanRequestAdv(struct ADAPTER *prAdapter,
		     struct PARAM_SCAN_REQUEST_ADV *prRequestIn)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest;
	struct AIS_SCAN_REQ *prAisReq;
	uint8_t ucBssIndex = 0;

	if (!prRequestIn) {
		log_dbg(SCN, WARN, "Scan request is NULL\n");
		return;
	}
	ucBssIndex = prRequestIn->ucBssIndex;
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	DBGLOG(SCN, TRACE,
		"[AIS%d][%d] eCurrentState=%d rrm=%d mlo=%d\n",
		prAisFsmInfo->ucAisIndex, ucBssIndex,
		prAisFsmInfo->eCurrentState, prRequestIn->fgIsRrm,
		prRequestIn->fgNeedMloScan);

	if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
		/* 802.1x might not finished yet, pend it for
		 * later handling ..
		 */
		if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
		    timerPendingTimer(&prAisFsmInfo->rJoinTimeoutTimer)) {
			prAisReq = (struct AIS_SCAN_REQ *) cnmMemAlloc(
					prAdapter, RAM_TYPE_MSG,
					sizeof(struct AIS_SCAN_REQ));
			if (!prAisReq) {
				DBGLOG(AIS, ERROR,
					"Can't generate new scan req\n");
				return;
			}
			prAisReq->rReqHdr.eReqType = AIS_REQUEST_SCAN;
			prScanRequest = &prAisReq->rScanRequest;
			kalMemCopy(prScanRequest, prRequestIn,
				   sizeof(struct PARAM_SCAN_REQUEST_ADV));
			aisFsmInsertRequestImpl(prAdapter,
				(struct AIS_REQ_HDR *)prAisReq,
				FALSE, ucBssIndex);
		} else {
			if (prAisFsmInfo->fgIsChannelGranted == TRUE) {
				DBGLOG(SCN, WARN,
				"Scan Request with channel granted for join operation: %d, %d",
				prAisFsmInfo->fgIsChannelGranted,
				prAisFsmInfo->fgIsChannelRequested);
			}

			/* start online scan */
			aisFsmSyncScanReq(prAdapter, prRequestIn, ucBssIndex);
			aisFsmSteps(prAdapter, AIS_STATE_ONLINE_SCAN,
				ucBssIndex);
		}
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE) {
		aisFsmSyncScanReq(prAdapter, prRequestIn, ucBssIndex);
		aisFsmSteps(prAdapter, AIS_STATE_SCAN, ucBssIndex);
	} else {
		prAisReq = (struct AIS_SCAN_REQ *) cnmMemAlloc(prAdapter,
			RAM_TYPE_MSG, sizeof(struct AIS_SCAN_REQ));
		if (!prAisReq) {
			DBGLOG(AIS, ERROR, "Can't generate new scan req\n");
			return;
		}
		prAisReq->rReqHdr.eReqType = AIS_REQUEST_SCAN;
		prScanRequest = &prAisReq->rScanRequest;
		kalMemCopy(prScanRequest, prRequestIn,
			   sizeof(struct PARAM_SCAN_REQUEST_ADV));
		aisFsmInsertRequestImpl(prAdapter,
			(struct AIS_REQ_HDR *)prAisReq, FALSE, ucBssIndex);
	}
}				/* end of aisFsmScanRequestAdv() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is invoked when CNM granted channel privilege
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventChGrant(struct ADAPTER *prAdapter,
			   struct MSG_HDR *prMsgHdr)
{
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct MSG_CH_GRANT *prMsgChGrant;
	uint8_t ucTokenID;
	uint32_t u4GrantInterval;
	uint8_t ucBssIndex = 0;

	prMsgChGrant = (struct MSG_CH_GRANT *)prMsgHdr;

	ucTokenID = prMsgChGrant->ucTokenID;
	u4GrantInterval = prMsgChGrant->u4GrantInterval;
	ucBssIndex = prMsgChGrant->ucBssIndex;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisSpecificBssInfo =
		aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	/* 1. free message */
	cnmMemFree(prAdapter, prMsgHdr);

	if (prAisBssInfo->prStaRecOfAP &&
		prAisBssInfo->fgIsAisSwitchingChnl == TRUE) {
		struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

		/* 2. channel privilege has been approved */
		aisChangeMediaState(prAisBssInfo, MEDIA_STATE_CONNECTED);
		nicUpdateBss(prAdapter, ucBssIndex);

		/* 3. switch to new channel */
		prAisBssInfo->fgIsAisSwitchingChnl = FALSE;
		prAisFsmInfo->fgIsChannelGranted = TRUE;
		aisFsmReleaseCh(prAdapter, ucBssIndex);

		kalIndicateChannelSwitch(
			prAdapter->prGlueInfo,
			prAisBssInfo->eBssSCO,
			prAisBssInfo->ucPrimaryChannel,
			prAisBssInfo->eBand,
			prAisBssInfo->ucBssIndex);

		cnmTimerStartTimer(prAdapter,
				&prAisBssInfo->rCsaDoneTimer,
				SEC_TO_MSEC(prWifiVar->ucCsaDoneTimeout));

#if CFG_SUPPORT_CCM
		ccmChannelSwitchProducer(prAdapter, prAisBssInfo, __func__);
#endif /* CFG_SUPPORT_CCM */
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN
	    && prAisFsmInfo->ucSeqNumOfChReq == ucTokenID) {
		/* 2. channel privilege has been approved */
		prAisFsmInfo->u4ChGrantedInterval = u4GrantInterval;

		/* 3. state transition to join/ibss-alone/ibss-merge */
		/* 3.1 set timeout timer in cases join could not be completed */
		cnmTimerStartTimer(prAdapter,
				   &prAisFsmInfo->rJoinTimeoutTimer,
				   prAisFsmInfo->u4ChGrantedInterval -
				   AIS_JOIN_CH_GRANT_THRESHOLD);
		DBGLOG(AIS, INFO, "Start JOIN Timer!");
		aisFsmSteps(prAdapter, AIS_STATE_JOIN, ucBssIndex);

		prAisFsmInfo->fgIsChannelGranted = TRUE;

#if CFG_SUPPORT_CCM
		ccmPendingCheck(prAdapter, prAisBssInfo, u4GrantInterval);
#endif
	} else if (prAisFsmInfo->eCurrentState ==
		   AIS_STATE_REQ_REMAIN_ON_CHANNEL
		   && prAisFsmInfo->ucSeqNumOfChReq == ucTokenID) {
		/* 2. channel privilege has been approved */
		prAisFsmInfo->u4ChGrantedInterval = u4GrantInterval;

#if CFG_SUPPORT_NCHO
		if (prAdapter->rNchoInfo.fgNCHOEnabled == TRUE &&
		    prAdapter->rNchoInfo.fgIsSendingAF == TRUE &&
		    prAdapter->rNchoInfo.fgChGranted == FALSE) {
			DBGLOG(INIT, TRACE,
			       "NCHO complete rAisChGrntComp trace time is %u\n",
			       kalGetTimeTick());
			prAdapter->rNchoInfo.fgChGranted = TRUE;
			complete(&prAdapter->prGlueInfo->rAisChGrntComp);
		}
#endif

		/*
		 * set timeout timer in cases upper layer
		 * cancel_remain_on_channel never comes
		 */
		if (timerPendingTimer(&prAisFsmInfo->rChannelTimeoutTimer)) {
			cnmTimerStopTimer(prAdapter,
					&prAisFsmInfo->rChannelTimeoutTimer);
		}
		cnmTimerStartTimer(prAdapter,
				&prAisFsmInfo->rChannelTimeoutTimer,
				prAisFsmInfo->u4ChGrantedInterval -
				AIS_JOIN_CH_GRANT_THRESHOLD);

		DBGLOG(AIS, INFO, "Start %s Timer!\n",
			prAisFsmInfo->rChReqInfo.eReqType ==
			CH_REQ_TYPE_OFFCHNL_TX ? "TxMgmt" : "ROC");

		if (prAisFsmInfo->rChReqInfo.eReqType ==
				CH_REQ_TYPE_OFFCHNL_TX) {
			aisFsmSteps(prAdapter, AIS_STATE_OFF_CHNL_TX,
				ucBssIndex);
		} else {
			/* switch to remain_on_channel state */
			aisFsmSteps(prAdapter, AIS_STATE_REMAIN_ON_CHANNEL,
				ucBssIndex);

			/* indicate upper layer for channel ready */
			kalReadyOnChannel(prAdapter->prGlueInfo,
					prAisFsmInfo->rChReqInfo.u8Cookie,
					prAisFsmInfo->rChReqInfo.eBand,
					prAisFsmInfo->rChReqInfo.eSco,
					prAisFsmInfo->rChReqInfo.ucChannelNum,
					prAisFsmInfo->rChReqInfo.u4DurationMs,
					ucBssIndex);
		}

		prAisFsmInfo->fgIsChannelGranted = TRUE;
	} else {		/* mismatched grant */
		/* 2. return channel privilege to CNM immediately */
		/* aisFsmReleaseCh(prAdapter, ucBssIndex); */
		DBGLOG(AIS, WARN, "channel grant token mismatch\n");
	}
}				/* end of aisFsmRunEventChGrant() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to inform CNM that channel privilege
 *           has been released
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmReleaseCh(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_CH_ABORT *prMsgChAbort;
#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct BSS_INFO *prBss = NULL;
#endif


	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	if (prAisFsmInfo->fgIsChannelGranted == TRUE
	    || prAisFsmInfo->fgIsChannelRequested == TRUE) {
		prAisFsmInfo->fgIsChannelRequested = FALSE;
		prAisFsmInfo->fgIsChannelGranted = FALSE;
		/* 1. return channel privilege to CNM immediately */
		prMsgChAbort =
		    (struct MSG_CH_ABORT *)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
						       sizeof(struct
							      MSG_CH_ABORT));
		if (!prMsgChAbort) {
			DBGLOG(AIS, ERROR, "Can't release Channel to CNM\n");
			return;
		}

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
		prBss = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
		prMldBssInfo = mldBssGetByBss(prAdapter, prBss);
#endif

		kalMemZero(prMsgChAbort, sizeof(struct MSG_CH_ABORT));
		prMsgChAbort->rMsgHdr.eMsgId = MID_MNY_CNM_CH_ABORT;
		prMsgChAbort->ucBssIndex = prAisFsmInfo->ucBssIndexOfChReq;
		prMsgChAbort->ucTokenID = prAisFsmInfo->ucSeqNumOfChReq;
		prMsgChAbort->ucExtraChReqNum = prAisFsmInfo->ucChReqNum - 1;
#if CFG_SUPPORT_DBDC
		/* STR/MLSR mode the DBDC band is ENUM_BAND_ALL;
		 * EMLSR/Hybird mode the DBDC band is ENUM_BAND_AUTO
		 */
		if (prMsgChAbort->ucExtraChReqNum >= 1
#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
			&& prMldBssInfo && (
			prMldBssInfo->ucMaxSimuLinks >= 1 ||
			(prMldBssInfo->ucMaxSimuLinks == 0 &&
			prMldBssInfo->ucEmlEnabled  == FALSE &&
			prMldBssInfo->ucHmloEnabled ==  FALSE))
#endif
		)
			prMsgChAbort->eDBDCBand = ENUM_BAND_ALL;
		else
			prMsgChAbort->eDBDCBand = ENUM_BAND_AUTO;
#endif /*CFG_SUPPORT_DBDC */

		DBGLOG(AIS, INFO, "ucBssIndex: %d, ucTokenID: 0x%x, ucExtraChReqNum: %d\n",
			prMsgChAbort->ucBssIndex,
			prMsgChAbort->ucTokenID,
			prMsgChAbort->ucExtraChReqNum);

		mboxSendMsg(prAdapter, MBOX_ID_0,
			    (struct MSG_HDR *)prMsgChAbort,
			    MSG_SEND_METHOD_UNBUF);
	}
}				/* end of aisFsmReleaseCh() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to inform AIS that corresponding beacon has not
 *           been received for a while and probing is not successful
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisBssBeaconTimeout(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)

{
	/* trigger by driver, use dummy reason code */
	aisBssBeaconTimeout_impl(prAdapter, BEACON_TIMEOUT_REASON_NUM,
		DISCONNECT_REASON_CODE_RADIO_LOST, ucBssIndex);
}

void aisBssBeaconTimeout_impl(struct ADAPTER *prAdapter,
	uint8_t ucBcnTimeoutReason, uint8_t ucDisconnectReason,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prAisBssInfo;
	u_int8_t fgDoAbortIndication = FALSE;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_BTO_INFO *prAisBtoInfo;
	uint8_t roam, join;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	ucBssIndex = aisGetMainLinkBssIndex(prAdapter, prAisFsmInfo);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisBtoInfo = &(prAisFsmInfo->rBtoInfo);

	if (prAisBssInfo->eConnectionState != MEDIA_STATE_CONNECTED)
		return;

	/* 4 <1> Diagnose Connection for Beacon Timeout Event */
	if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
		struct STA_RECORD *prStaRec = prAisBssInfo->prStaRecOfAP;

		if (prStaRec)
			fgDoAbortIndication = TRUE;
	} else if (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
		fgDoAbortIndication = TRUE;
	}

	/* 4 <2> invoke abort handler */
	if (fgDoAbortIndication) {
#if CFG_SUPPORT_ROAMING
		roam = roamingFsmCheckIfRoaming(prAdapter, ucBssIndex);
		join = timerPendingTimer(
				&prAisFsmInfo->rJoinTimeoutTimer);

		aisFsmClearRequest(prAdapter, AIS_REQUEST_BTO, ucBssIndex);
		prAisBtoInfo->ucBcnTimeoutReason = ucBcnTimeoutReason;
		prAisBtoInfo->ucDisconnectReason = ucDisconnectReason;

		if (roam || join) {
			struct PARAM_SSID rSsid;

			DBGLOG(AIS, EVENT,
				"Postpone aisBssBeaconTimeout, roam=%d, join=%d",
				roam, join);

			/* record info for postpone handle BTO */
			kalMemZero(&rSsid, sizeof(struct PARAM_SSID));
			COPY_SSID(rSsid.aucSsid,
				  rSsid.u4SsidLen,
				  prAisBssInfo->aucSSID,
				  prAisBssInfo->ucSSIDLen);
			prAisBtoInfo->prBtoBssDesc =
				scanSearchBssDescByBssidAndSsid(prAdapter,
				prAisBssInfo->aucBSSID, TRUE, &rSsid);

			aisFsmInsertRequest(prAdapter,
				AIS_REQUEST_BTO,
				ucBssIndex);
		} else
#endif /* CFG_SUPPORT_ROAMING */
		{
			DBGLOG(AIS, EVENT,
				"aisBssBeaconTimeout, roam=%d, join=%d",
				roam, join);
			aisHandleBeaconTimeout(prAdapter, ucBssIndex, FALSE);
		}
	}
}

void aisHandleBeaconTimeout(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, u_int8_t fgDelayAbortIndication)
{
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_BTO_INFO *prAisBtoInfo;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBtoInfo = &(prAisFsmInfo->rBtoInfo);

	if (fgDelayAbortIndication) {
		struct BSS_DESC *prBssDesc =
			aisGetTargetBssDesc(prAdapter, ucBssIndex);
		struct BSS_DESC *prBtoBssDesc =
			prAisBtoInfo->prBtoBssDesc;

		if (prBtoBssDesc != prBssDesc) {
			DBGLOG(AIS, EVENT,
				"Connect to better AP[" MACSTR
				"], ignore BTO AP[" MACSTR "]\n",
				MAC2STR(prBssDesc->aucBSSID),
				MAC2STR(prBtoBssDesc->aucBSSID));
			return;
		}
	}

	prAisBssInfo->u2DeauthReason =
		REASON_CODE_BEACON_TIMEOUT * 100 +
		prAisBtoInfo->ucBcnTimeoutReason;
	DBGLOG(AIS, EVENT, "aisBssBeaconTimeout\n");

	aisFsmStateAbort(prAdapter,
		prAisBtoInfo->ucDisconnectReason,
		TRUE,
		ucBssIndex);
}				/* end of aisBssBeaconTimeout() */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is to decide if we can roam out by this beacon time
*
* \param[in] prAdapter  Pointer of ADAPTER_T
*
* \return true	if we can roam out
*         false	others
*/
/*----------------------------------------------------------------------------*/
uint8_t aisBeaconTimeoutFilterPolicy(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
#if CFG_SUPPORT_ROAMING
	struct AIS_FSM_INFO *ais;
#if (CFG_EXT_ROAMING == 1)
	enum ENUM_ROAMING_REASON eReason = ROAMING_REASON_NUM;
#else
	enum ENUM_ROAMING_REASON eReason = ROAMING_REASON_TX_ERR;
#endif
	int8_t rssi;

	ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	rssi = prAdapter->rLinkQuality.rLq[ucBssIndex].cRssi;
#if (CFG_EXT_ROAMING == 1)
	if (roamingFsmInDecision(prAdapter, FALSE, ucBssIndex) && rssi > -83)
#else
	if (roamingFsmInDecision(prAdapter, FALSE, ucBssIndex) && rssi > -70)
#endif
	{
		/* Good rssi but beacon timeout happened => PER */
		struct BSS_DESC_SET set = {0};

		apsSearchBssDescByScore(prAdapter, eReason, ucBssIndex, &set);
		if (aisBssDescAllowed(prAdapter, ais, &set)) {
			DBGLOG(AIS, INFO, "Better AP found for BTO\n");
			return TRUE;
		}
	}
#endif
	return FALSE;
}

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
void aisBssSecurityChanged(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{

	aisFsmStateAbort(prAdapter, DISCONNECT_REASON_CODE_DEAUTHENTICATED,
			 FALSE, ucBssIndex);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to inform AIS that corresponding beacon has not
 *           been received for a while and probing is not successful
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisBssLinkDown(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prAisBssInfo;
	u_int8_t fgDoAbortIndication = FALSE;
	struct CONNECTION_SETTINGS *prConnSettings;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	/* 4 <1> Diagnose Connection for Beacon Timeout Event */
	if (prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
		if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
			struct STA_RECORD *prStaRec =
			    prAisBssInfo->prStaRecOfAP;

			if (prStaRec)
				fgDoAbortIndication = TRUE;

		} else if (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
			fgDoAbortIndication = TRUE;
		}
	}
	/* 4 <2> invoke abort handler */
	if (fgDoAbortIndication) {
		DBGLOG(AIS, EVENT, "aisBssLinkDown\n");
		aisFsmStateAbort(prAdapter,
				 DISCONNECT_REASON_CODE_DISASSOCIATED, FALSE,
				 ucBssIndex);
	}

	/* kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
	 * WLAN_STATUS_SCAN_COMPLETE, NULL, 0);
	 */
}				/* end of aisBssLinkDown() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to inform AIS that DEAUTH frame has been
 *           sent and thus state machine could go ahead
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 * \param[in] prMsduInfo Pointer of MSDU_INFO_T for DEAUTH frame
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
aisDeauthXmitCompleteBss(struct ADAPTER *prAdapter,
		      uint8_t ucBssIndex,
		      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	u_int8_t fgIsReset = FALSE;

	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		DBGLOG(AIS, WARN,
		       "Invalid index=%d, DEAUTH frame transmitted without further handling\n",
		       ucBssIndex);
		return WLAN_STATUS_SUCCESS;
	}

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

#if CFG_SUPPORT_802_11W
	/* Notify completion after encrypted deauth frame tx done */
	if (prAisFsmInfo->encryptedDeauthIsInProcess == TRUE) {
		if (!kal_completion_done(&prAisFsmInfo->rDeauthComp)) {
			DBGLOG(AIS, EVENT, "Complete rDeauthComp\n");
			complete(&prAisFsmInfo->rDeauthComp);
		}
	}
	prAisFsmInfo->encryptedDeauthIsInProcess = FALSE;
#endif

	if (rTxDoneStatus == TX_RESULT_SUCCESS)
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rDeauthDoneTimer);

#if CFG_CHIP_RESET_SUPPORT && !CFG_WMT_RESET_API_SUPPORT
	if (prAdapter->chip_info->fgIsSupportL0p5Reset) {
		KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_WFSYS_RESET);
		if (prAdapter->eWfsysResetState == WFSYS_RESET_STATE_REINIT) {
			DBGLOG(AIS, INFO, "during L0.5 reset reinit state\n");

			fgIsReset = TRUE;
		}
		KAL_RELEASE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_WFSYS_RESET);
	}
#endif /* CFG_CHIP_RESET_SUPPORT */

	if (prAisFsmInfo->eCurrentState == AIS_STATE_DISCONNECTING) {
		DBGLOG(AIS, EVENT, "aisDeauthXmitComplete\n");
		if ((rTxDoneStatus != TX_RESULT_DROPPED_IN_DRIVER
		    && rTxDoneStatus != TX_RESULT_QUEUE_CLEARANCE)
			|| fgIsReset)
			aisFsmStateAbort(prAdapter,
					 prAisFsmInfo->ucReasonOfDisconnect,
					 FALSE, ucBssIndex);
	} else {
		DBGLOG(AIS, WARN,
		       "DEAUTH frame transmitted without further handling\n");
	}

	return WLAN_STATUS_SUCCESS;

}				/* end of aisDeauthXmitComplete() */

uint32_t
aisDeauthXmitComplete(struct ADAPTER *prAdapter,
			struct MSDU_INFO *prMsduInfo,
			enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	return aisDeauthXmitCompleteBss(prAdapter,
		prMsduInfo->ucBssIndex, rTxDoneStatus);
}

#if CFG_SUPPORT_ROAMING
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate an Event of "Looking for a candidate
 *         due to weak signal" to AIS FSM.
 * @param[in] u4ReqScan  Requesting Scan or not
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventRoamingDiscovery(struct ADAPTER *prAdapter,
	uint32_t u4ReqScan, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	enum ENUM_AIS_REQUEST_TYPE eAisRequest;
	struct ROAMING_INFO *prRoamingInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prRoamingInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);

	/* TODO: Stop roaming event in FW */
#if CFG_SUPPORT_WFD
#if CFG_ENABLE_WIFI_DIRECT
	if (prRoamingInfo->eReason != ROAMING_REASON_POOR_RCPI) {
		/* Check WFD is running */
		struct WFD_CFG_SETTINGS *prWfdCfgSettings =
		    (struct WFD_CFG_SETTINGS *)NULL;

		prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);
		if ((prWfdCfgSettings->ucWfdEnable != 0)) {
			DBGLOG(AIS, INFO, "WFD is running. Stop roaming.\n");
			roamingFsmRunEventNewCandidate(prAdapter,
				NULL, ucBssIndex);
			roamingFsmRunEventFail(prAdapter,
					       ROAMING_FAIL_REASON_NOCANDIDATE,
					       ucBssIndex);
			return;
		}
	}
#endif
#endif

	/* results are still new */
	if (!u4ReqScan) {
		eAisRequest = AIS_REQUEST_ROAMING_CONNECT;
	} else {
		if (prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN
		    || prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR) {
			eAisRequest = AIS_REQUEST_ROAMING_CONNECT;
		} else {
			eAisRequest = AIS_REQUEST_ROAMING_SEARCH;
			if (prRoamingInfo->rRoamScanParam.ucScanCount)
				prAisFsmInfo->ucScanTrialCount++;
		}
	}

	prAisFsmInfo->fgTargetChnlScanIssued = TRUE;
	if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR
	    && !timerPendingTimer(&prAisFsmInfo->rJoinTimeoutTimer)) {
		if (eAisRequest == AIS_REQUEST_ROAMING_SEARCH) {
			aisFsmSteps(prAdapter, AIS_STATE_LOOKING_FOR,
				ucBssIndex);
		} else
			aisFsmSteps(prAdapter, AIS_STATE_SEARCH,
				ucBssIndex);
	} else {
		aisFsmRemoveRoamingRequest(prAdapter, ucBssIndex);
		aisFsmInsertRequest(prAdapter, eAisRequest, ucBssIndex);
	}
}				/* end of aisFsmRunEventRoamingDiscovery() */

static enum ENUM_AIS_STATE aisSearchHandleReconnect(struct ADAPTER *ad,
	uint8_t ucBssIndex)
{
	uint8_t i, j;
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(ad, ucBssIndex);
	struct CONNECTION_SETTINGS *conn = aisGetConnSettings(ad, ucBssIndex);
	struct BSS_DESC *prMainBssDesc = aisGetMainLinkBssDesc(ais);
	struct ROAMING_INFO *roam = aisGetRoamingInfo(ad, ucBssIndex);

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_DESC *prBssDesc = aisGetLinkBssDesc(ais, i);

		if (!prBssDesc)
			continue;

		for (j = 0; j < MLD_LINK_MAX; j++) {
			struct BSS_INFO *bss = aisGetLinkBssInfo(ais, j);

			if (!bss)
				continue;

			/* same ap, need to reconnect */
			if (ad->rWifiVar.fgRoamByBTO ||
			    EQUAL_MAC_ADDR(bss->aucBSSID,
					   prBssDesc->aucBSSID)) {
				struct MSG_AIS_ABORT *prAisAbortMsg;


				/* set bssid_hint to keep aps result */
				conn->eConnectionPolicy = CONNECT_BY_BSSID_HINT;
				COPY_MAC_ADDR(conn->aucBSSIDHint,
					      prMainBssDesc->aucBSSID);
				conn->u4FreqInMHz = nicChannelNum2Freq(
					prMainBssDesc->ucChannelNum,
					prMainBssDesc->eBand) / 1000;

				prAisAbortMsg = (struct MSG_AIS_ABORT *)
					cnmMemAlloc(ad, RAM_TYPE_MSG,
					sizeof(struct MSG_AIS_ABORT));
				if (!prAisAbortMsg) {
					DBGLOG(REQ, ERROR,
					   "Fail in allocating AisAbortMsg.\n");
					aisFsmStateAbort(ad,
						DISCONNECT_REASON_CODE_LOCALLY,
						FALSE, ucBssIndex);
					return AIS_STATE_ROAMING;
				}
				prAisAbortMsg->rMsgHdr.eMsgId =
					MID_OID_AIS_FSM_JOIN_REQ;

				DBGLOG(AIS, INFO,
					"roaming eReason[%d]\n",
					roam->eReason);
				if (roam->eReason == ROAMING_REASON_BTM)
					prAisAbortMsg->ucReasonOfDisconnect =
						DISCONNECT_REASON_CODE_BTM;
				else
					prAisAbortMsg->ucReasonOfDisconnect =
					  DISCONNECT_REASON_CODE_REASSOCIATION;
				prAisAbortMsg->fgDelayIndication = TRUE;
				prAisAbortMsg->ucBssIndex = ucBssIndex;
				mboxSendMsg(ad, MBOX_ID_0,
					(struct MSG_HDR *) prAisAbortMsg,
					MSG_SEND_METHOD_BUF);

				DBGLOG(AIS, INFO,
					"Force reconnect to the same AP\n");

				/* stay ROAMING and wait for msg executed */
				return AIS_STATE_ROAMING;
			}
		}
	}

	return AIS_STATE_REQ_CHANNEL_JOIN;
}

void aisFsmRunEventRoamingRoam(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *ais;
	struct BSS_DESC_SET *set;
	enum ENUM_AIS_STATE eNewState;

	ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	set = aisGetSearchResult(prAdapter, ucBssIndex);

	if (ais->eCurrentState != AIS_STATE_ROAMING) {
		DBGLOG(AIS, INFO,
			"[AIS%d][%d] curr state=%s not allowed roaming\n",
			ais->ucAisIndex, ucBssIndex,
			aisGetFsmState(ais->eCurrentState));
		return;
	}

	ais->ucConnTrialCount++;
	ais->fgTargetChnlScanIssued = FALSE;
	ais->ucIsStaRoaming = TRUE;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	ais->ucMlProbeSendCount = 0;
	ais->ucMlProbeEnable = FALSE;
	ais->prMlProbeBssDesc = NULL;
#endif

	aisFillBssInfoFromBssDesc(prAdapter, ais, set);

#if CFG_EXT_ROAMING_WTC
	aisWtcSearchHandleBssDesc(
		prAdapter,
		ucBssIndex);
#endif

	eNewState = aisSearchHandleReconnect(
		prAdapter, ucBssIndex);

	if (eNewState != ais->eCurrentState)
		aisFsmSteps(prAdapter, eNewState, ucBssIndex);
}

uint8_t aisCheckNeedDriverRoaming(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
#if (CFG_SUPPORT_DRIVER_ROAMING == 1)
	struct ROAMING_INFO *roam;
	struct AIS_FSM_INFO *ais;
	struct CONNECTION_SETTINGS *setting;
	int8_t rssi;

	roam = aisGetRoamingInfo(prAdapter, ucBssIndex);
	ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	setting = aisGetConnSettings(prAdapter, ucBssIndex);
	rssi = prAdapter->rLinkQuality.rLq[ucBssIndex].cRssi;

	GET_CURRENT_SYSTIME(&roam->rRoamingDiscoveryUpdateTime);

	/*
	 * try to select AP only when roaming is enabled and rssi is bad
	 */
	if (roamingFsmInDecision(prAdapter, FALSE, ucBssIndex) &&
	    ais->eCurrentState == AIS_STATE_ONLINE_SCAN &&
	    CHECK_FOR_TIMEOUT(roam->rRoamingDiscoveryUpdateTime,
		      roam->rRoamingLastDecisionTime,
		      SEC_TO_SYSTIME(prAdapter->rWifiVar.u4InactiveTimeout))) {
		struct BSS_DESC_SET set = {0};
		struct BSS_DESC *target;
		struct BSS_DESC *bss;

		bss = apsSearchBssDescByScore(prAdapter,
			ROAMING_REASON_INACTIVE, ucBssIndex, &set);
		if (bss == NULL)
			return FALSE;

		/* Driver roaming prefer 5g/6g */
		target = aisGetMainLinkBssDesc(ais);

		/* 2.4 -> 5 */
#if (CFG_SUPPORT_WIFI_6G == 1)
		if ((bss->eBand == BAND_5G || bss->eBand == BAND_6G)
#else
		if (bss->eBand == BAND_5G
#endif
			&& target->eBand == BAND_2G4) {
			if (rssi > RSSI_BAD_NEED_ROAM_24G_TO_5G_6G)
				return FALSE;
			if (bss->ucRCPI >= RCPI_THRESHOLD_ROAM_TO_5G_6G ||
			bss->ucRCPI - target->ucRCPI > RCPI_DIFF_DRIVER_ROAM) {
				DBGLOG(AIS, INFO,
					"Driver trigger roaming to A band.\n");
				return TRUE;
			}
		} else {
			if (rssi > RSSI_BAD_NEED_ROAM)
				return FALSE;
			if (bss->ucRCPI - target->ucRCPI >
				RCPI_DIFF_DRIVER_ROAM) {
				DBGLOG(AIS, INFO,
				"Driver trigger roaming for other cases.\n");
				return TRUE;
			}
		}
	}
#endif
	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Update the time of ScanDone for roaming and transit to Roam state.
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
enum ENUM_AIS_STATE aisFsmRoamingScanResultsUpdate(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct ROAMING_INFO *prRoamingFsmInfo;
	enum ENUM_AIS_STATE eNextState;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);
	eNextState = prAisFsmInfo->eCurrentState;

	if (prRoamingFsmInfo->eCurrentState == ROAMING_STATE_DISCOVERY) {
		aisFsmRemoveRoamingRequest(prAdapter, ucBssIndex);
		eNextState = AIS_STATE_SEARCH;
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR) {
		eNextState = AIS_STATE_SEARCH;
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN) {
		eNextState = AIS_STATE_NORMAL_TR;

		if (aisCheckNeedDriverRoaming(prAdapter, ucBssIndex)) {
			struct CMD_ROAMING_TRANSIT rRoamingData = {0};
			struct BSS_DESC *prBssDesc =
				aisGetTargetBssDesc(prAdapter, ucBssIndex);

			rRoamingData.eReason = ROAMING_REASON_INACTIVE;
			rRoamingData.u2Data = prBssDesc->ucRCPI;
			rRoamingData.u2RcpiLowThreshold =
				prRoamingFsmInfo->ucThreshold;
			rRoamingData.ucBssidx = ucBssIndex;
			roamingFsmRunEventDiscovery(prAdapter, &rRoamingData);
		}
	}

	return eNextState;
} /* end of aisFsmRoamingScanResultsUpdate() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will modify and update necessary information to firmware
 *        for disconnection of last AP before switching to roaming bss.
 *
 * @param IN prAdapter          Pointer to the Adapter structure.
 *           prTargetStaRec     Target of StaRec of roaming
 *
 * @retval None
 */
/*----------------------------------------------------------------------------*/
void aisFsmRoamingDisconnectPrevAP(struct ADAPTER *prAdapter,
				   struct BSS_INFO *prAisBssInfo,
				   struct STA_RECORD *prTargetStaRec)
{
	uint8_t ucBssIndex = prAisBssInfo->ucBssIndex;
	struct BSS_DESC *prNewBssDesc = NULL;

	prNewBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);

	if (prAisBssInfo->prStaRecOfAP != prTargetStaRec)
		wmmNotifyDisconnected(prAdapter, ucBssIndex);

	nicPmIndicateBssAbort(prAdapter, prAisBssInfo->ucBssIndex);

	/* Not invoke rlmBssAborted() here to avoid prAisBssInfo->fg40mBwAllowed
	 * to be reset. RLM related parameters will be reset again when handling
	 * association response in rlmProcessAssocRsp(). 20110413
	 */
	/* rlmBssAborted(prAdapter, prAisBssInfo); */

	/* 4 <3> Unset the fgIsConnected flag of BSS_DESC_T and
	 * send Deauth if needed.
	 */
	if (prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
		struct PARAM_SSID rSsid;
		struct BSS_DESC *prBssDesc = NULL;

		kalMemZero(&rSsid, sizeof(struct PARAM_SSID));
		COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen, prAisBssInfo->aucSSID,
			  prAisBssInfo->ucSSIDLen);
		prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
						    prAisBssInfo->aucBSSID,
						    TRUE, &rSsid);
		if (prBssDesc) {
			prBssDesc->fgIsConnected &= ~BIT(ucBssIndex);
			prBssDesc->fgIsConnecting &= ~BIT(ucBssIndex);
		}
	}

	/* 4 <4> Change Media State immediately. */
	aisChangeMediaState(prAisBssInfo,
		prTargetStaRec ?
		MEDIA_STATE_ROAMING_DISC_PREV :
		MEDIA_STATE_DISCONNECTED);

	/* 4 <4.1> sync. with firmware */
	/* Virtial BSSID */
	if (prTargetStaRec)
		prTargetStaRec->ucBssIndex = (prAdapter->ucSwBssIdNum + 1);
	if (prNewBssDesc)
		COPY_MAC_ADDR(prAisBssInfo->aucBSSID, prNewBssDesc->aucBSSID);
	nicUpdateBss(prAdapter, prAisBssInfo->ucBssIndex);

	if (prTargetStaRec) {
		/* if there's no target, postpone removing bc entry to
		 * deactivate otherwise deactivate won't sync with fw because
		 * ucBMCWlanIndex == WTBL_RESERVED_ENTRY
		 */
		secRemoveBssBcEntry(prAdapter, prAisBssInfo, TRUE);
		prTargetStaRec->ucBssIndex = prAisBssInfo->ucBssIndex;
	}
	/* before deactivate previous AP, should move its pending MSDUs
	 ** to the new AP
	 */
	if (prAisBssInfo->prStaRecOfAP) {
		if (prAisBssInfo->prStaRecOfAP != prTargetStaRec &&
		    prAisBssInfo->prStaRecOfAP->fgIsInUse) {
			qmMoveStaTxQueue(prAdapter, prAisBssInfo->prStaRecOfAP,
					 prTargetStaRec);
			cnmStaRecFree(prAdapter, prAisBssInfo->prStaRecOfAP);
			prAisBssInfo->prStaRecOfAP = NULL;
		} else {
			DBGLOG(AIS, WARN, "prStaRecOfAP is in use %d\n",
			       prAisBssInfo->prStaRecOfAP->fgIsInUse);
			/* starec is already freed in nicUpdateBss */
			if (!prAisBssInfo->prStaRecOfAP->fgIsInUse)
				prAisBssInfo->prStaRecOfAP = NULL;
		}
	} else {
		DBGLOG(AIS, WARN,
		       "NULL pointer of prAisBssInfo->prStaRecOfAP\n");
	}
}				/* end of aisFsmRoamingDisconnectPrevAP() */

void aisFsmRoamingDisconnectPrevAllAP(struct ADAPTER *prAdapter,
				   struct AIS_FSM_INFO *prAisFsmInfo)
{
	uint8_t i;

	/* Disable rssi monitor */
	if (prAisFsmInfo->rRSSIMonitor.enable)
		aisFsmEnableRssiMonitor(prAdapter, prAisFsmInfo, FALSE);

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct STA_RECORD *prStaRec =
			aisGetLinkStaRec(prAisFsmInfo, i);
		struct BSS_INFO *prAisBssInfo =
			aisGetLinkBssInfo(prAisFsmInfo, i);

		if (!prAisBssInfo)
			continue;

		if (prStaRec &&
		    prStaRec->u2StatusCode != STATUS_CODE_SUCCESSFUL) {
			DBGLOG(AIS, INFO, "Remove link%d status code=%d\n",
				i, prStaRec->u2StatusCode);
			cnmStaRecFree(prAdapter, prStaRec);
			prStaRec = NULL;
		}

		aisFsmRoamingDisconnectPrevAP(prAdapter,
			prAisBssInfo, prStaRec);

		/* free bssinfo if it has no target starec */
		if (i != AIS_MAIN_LINK_INDEX &&
		    prAisBssInfo->eConnectionState == MEDIA_STATE_DISCONNECTED)
			aisFreeBssInfo(prAdapter, prAisFsmInfo, i);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will update the contain of BSS_INFO_T for AIS
 *         network once the roaming was completed.
 *
 * @param IN prAdapter          Pointer to the Adapter structure.
 *           prStaRec           StaRec of roaming AP
 *           prAssocRspSwRfb
 *
 * @retval None
 */
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForRoamingAP(struct ADAPTER *prAdapter,
				  struct STA_RECORD *prStaRec,
				  struct SW_RFB *prAssocRspSwRfb)
{
	struct BSS_INFO *prAisBssInfo;
	uint8_t ucBssIndex = prStaRec->ucBssIndex;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	/* Change FW's Media State immediately. */
	aisChangeMediaState(prAisBssInfo, MEDIA_STATE_CONNECTED);

	/* Deactivate previous AP's STA_RECORD_T in Driver if have. */
	if ((prAisBssInfo->prStaRecOfAP) &&
	    (prAisBssInfo->prStaRecOfAP != prStaRec)
	    && (prAisBssInfo->prStaRecOfAP->fgIsInUse)) {
		/* before deactivate previous AP, should move its pending MSDUs
		 ** to the new AP
		 */
		qmMoveStaTxQueue(prAdapter,
			prAisBssInfo->prStaRecOfAP, prStaRec);
		/* cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP,
		 ** STA_STATE_1);
		 */
		cnmStaRecFree(prAdapter, prAisBssInfo->prStaRecOfAP);
	}

	/* Update BSS_INFO_T */
	aisUpdateBssInfoForJOIN(prAdapter, prStaRec, prAssocRspSwRfb);

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
	/* Activate current AP's STA_RECORD_T in Driver. */
	cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
#endif
}				/* end of aisFsmRoamingUpdateBss() */

void aisUpdateBssInfoForRoamingAllAP(struct ADAPTER *prAdapter,
				struct AIS_FSM_INFO *prAisFsmInfo,
				struct SW_RFB *prAssocRspSwRfb,
				struct STA_RECORD *prSetupStaRec)
{
	uint8_t i;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct SW_RFB *prSwRfb;
#endif

	/* Enable rssi monitor */
	if (prAisFsmInfo->rRSSIMonitor.enable)
		aisFsmEnableRssiMonitor(prAdapter, prAisFsmInfo, TRUE);

	/* update bssinfo */
	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct STA_RECORD *prStaRec =
			aisGetLinkStaRec(prAisFsmInfo, i);

		if (!prStaRec)
			continue;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (prStaRec == prSetupStaRec) {
			aisUpdateBssInfoForRoamingAP(prAdapter,
				prStaRec, prAssocRspSwRfb);
		} else {
			prSwRfb = mldDupAssocSwRfb(prAdapter,
				prAssocRspSwRfb, prStaRec);
			if (prSwRfb) {
				aisUpdateBssInfoForRoamingAP(prAdapter,
					prStaRec, prSwRfb);
				nicRxReturnRFB(prAdapter, prSwRfb);
			}
		}
#else
		aisUpdateBssInfoForRoamingAP(
			prAdapter, prStaRec, prAssocRspSwRfb);
#endif

	}

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 0)
	/* update starec */
	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct STA_RECORD *prStaRec =
			aisGetLinkStaRec(prAisFsmInfo, i);

		if (!prStaRec)
			continue;

		/* Activate current AP's STA_RECORD_T in Driver. */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
	}
#endif
}

#endif /* CFG_SUPPORT_ROAMING */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Check if there is any pending request and remove it (optional)
 *
 * @param prAdapter
 *        eReqType
 *        bRemove
 *
 * @return TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t aisFsmIsRequestPending(struct ADAPTER *prAdapter,
				enum ENUM_AIS_REQUEST_TYPE eReqType,
				u_int8_t bRemove, struct AIS_REQ_HDR **prReqHdr,
				uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_REQ_HDR *prPendingReqHdr, *prPendingReqHdrNext;
	u_int8_t found = FALSE;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* traverse through pending request list */
	LINK_FOR_EACH_ENTRY_SAFE(prPendingReqHdr,
				 prPendingReqHdrNext,
				 &(prAisFsmInfo->rPendingReqList), rLinkEntry,
				 struct AIS_REQ_HDR) {
		/* check for specified type */
		if (prPendingReqHdr->eReqType == eReqType) {
			found = TRUE;

			/* check if need to remove */
			if (bRemove == TRUE) {
				LINK_REMOVE_KNOWN_ENTRY(&(prAisFsmInfo->
					rPendingReqList),
					&(prPendingReqHdr->rLinkEntry));

				if (prReqHdr)
					*prReqHdr = prPendingReqHdr;
				else
					cnmMemFree(prAdapter, prPendingReqHdr);
				DBGLOG(AIS, INFO, "Remove req=%d\n", eReqType);
			}

			break;
		}
	}

	return found;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Clear any pending request
 *
 * @param prAdapter
 *        eReqType
 *
 * @return TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t aisFsmClearRequest(struct ADAPTER *prAdapter,
			     enum ENUM_AIS_REQUEST_TYPE eReqType,
			     uint8_t ucBssIndex)
{

	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_REQ_HDR *prPendingReqHdr, *prPendingReqHdrNext;
	u_int8_t found = FALSE;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* traverse through pending request list */
	LINK_FOR_EACH_ENTRY_SAFE(prPendingReqHdr,
				 prPendingReqHdrNext,
				 &(prAisFsmInfo->rPendingReqList), rLinkEntry,
				 struct AIS_REQ_HDR) {
		/* check for specified type */
		if (prPendingReqHdr->eReqType == eReqType) {
			found = TRUE;

			LINK_REMOVE_KNOWN_ENTRY(&(prAisFsmInfo->
				rPendingReqList),
				&(prPendingReqHdr->rLinkEntry));

			cnmMemFree(prAdapter, prPendingReqHdr);
			DBGLOG(AIS, INFO, "Remove req=%d\n", eReqType);
		}
	}

	return found;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Clear any pending request
 *
 * @param prAdapter
 *        eReqType
 *
 * @return TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
void aisFsmClearRequestForRrm(struct ADAPTER *prAdapter,
			     uint8_t ucBssIndex)
{

	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_REQ_HDR *prPendingReqHdr, *prPendingReqHdrNext;
	struct AIS_SCAN_REQ *prAisScanReq;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* traverse through pending request list */
	LINK_FOR_EACH_ENTRY_SAFE(prPendingReqHdr,
				 prPendingReqHdrNext,
				 &(prAisFsmInfo->rPendingReqList), rLinkEntry,
				 struct AIS_REQ_HDR) {
		/* check for specified type */
		if (prPendingReqHdr->eReqType == AIS_REQUEST_SCAN) {
			prAisScanReq = (struct AIS_SCAN_REQ *)prPendingReqHdr;
			if (prAisScanReq->rScanRequest.fgIsRrm) {

				LINK_REMOVE_KNOWN_ENTRY(
					&(prAisFsmInfo->rPendingReqList),
					&(prPendingReqHdr->rLinkEntry));

				cnmMemFree(prAdapter, prPendingReqHdr);
				DBGLOG(AIS, INFO, "Remove rrm scan req!\n");
			}
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Get next pending request
 *
 * @param prAdapter
 *
 * @return P_AIS_REQ_HDR_T
 */
/*----------------------------------------------------------------------------*/
struct AIS_REQ_HDR *aisFsmGetNextRequest(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_REQ_HDR *prPendingReqHdr;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	LINK_REMOVE_HEAD(&(prAisFsmInfo->rPendingReqList), prPendingReqHdr,
			 struct AIS_REQ_HDR *);

	return prPendingReqHdr;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Insert a new request
 *
 * @param prAdapter
 *        eReqType
 *
 * @return TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t aisFsmInsertRequest(struct ADAPTER *prAdapter,
			     enum ENUM_AIS_REQUEST_TYPE eReqType,
			     uint8_t ucBssIndex)
{
	struct AIS_REQ_HDR *prAisReq;
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	prAisReq =
	    (struct AIS_REQ_HDR *)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
					      sizeof(struct AIS_REQ_HDR));

	if (!prAisReq) {
		DBGLOG(AIS, ERROR, "Can't generate new message\n");
		return FALSE;
	}

	prAisReq->eReqType = eReqType;

	/* attach request into pending request list */
	LINK_INSERT_TAIL(&prAisFsmInfo->rPendingReqList, &prAisReq->rLinkEntry);

	DBGLOG(AIS, INFO, "eCurrentState=%d, eReqType=%d, u4NumElem=%d\n",
	       prAisFsmInfo->eCurrentState, eReqType,
	       prAisFsmInfo->rPendingReqList.u4NumElem);

	return TRUE;
}

u_int8_t aisFsmInsertRequestImpl(struct ADAPTER *prAdapter,
			     struct AIS_REQ_HDR *prAisReq,
			     uint8_t fgToHead,
			     uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* attach request into pending request list */
	if (fgToHead) {
		LINK_INSERT_HEAD(&prAisFsmInfo->rPendingReqList,
			&prAisReq->rLinkEntry);
	} else {
		LINK_INSERT_TAIL(&prAisFsmInfo->rPendingReqList,
			&prAisReq->rLinkEntry);
	}

	DBGLOG(AIS, INFO,
	       "To %s, eCurrentState=%d, eReqType=%d, u4NumElem=%d\n",
	       fgToHead ? "head" : "tail",
	       prAisFsmInfo->eCurrentState, prAisReq->eReqType,
	       prAisFsmInfo->rPendingReqList.u4NumElem);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Insert a new request to head
 *
 * @param prAdapter
 *        eReqType
 *
 * @return TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t aisFsmInsertRequestToHead(struct ADAPTER *prAdapter,
			     enum ENUM_AIS_REQUEST_TYPE eReqType,
			     uint8_t ucBssIndex)
{
	struct AIS_REQ_HDR *prAisReq;
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	prAisReq =
	    (struct AIS_REQ_HDR *)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
					      sizeof(struct AIS_REQ_HDR));

	if (!prAisReq) {
		DBGLOG(AIS, ERROR, "Can't generate new message\n");
		return FALSE;
	}

	prAisReq->eReqType = eReqType;

	/* attach request into pending request list */
	LINK_INSERT_HEAD(&prAisFsmInfo->rPendingReqList, &prAisReq->rLinkEntry);

	DBGLOG(AIS, INFO, "eCurrentState=%d, eReqType=%d, u4NumElem=%d\n",
	       prAisFsmInfo->eCurrentState, eReqType,
	       prAisFsmInfo->rPendingReqList.u4NumElem);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Flush all pending requests
 *
 * @param prAdapter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmFlushRequest(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct AIS_REQ_HDR *prAisReq;
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	DBGLOG(AIS, INFO, "aisFsmFlushRequest %d\n",
		prAisFsmInfo->rPendingReqList.u4NumElem);

	while ((prAisReq = aisFsmGetNextRequest(prAdapter, ucBssIndex)) != NULL)
		cnmMemFree(prAdapter, prAisReq);
}

void aisFsmRunEventRemainOnChannel(struct ADAPTER *prAdapter,
				   struct MSG_HDR *prMsgHdr)
{
	struct MSG_REMAIN_ON_CHANNEL *prRemainOnChannel;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = 0;

	prRemainOnChannel = (struct MSG_REMAIN_ON_CHANNEL *)prMsgHdr;

	ucBssIndex = prRemainOnChannel->ucBssIdx;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	/* record parameters */
	prAisFsmInfo->rChReqInfo.eBand = prRemainOnChannel->eBand;
	prAisFsmInfo->rChReqInfo.eSco = prRemainOnChannel->eSco;
	prAisFsmInfo->rChReqInfo.ucChannelNum = prRemainOnChannel->ucChannelNum;
	prAisFsmInfo->rChReqInfo.u4DurationMs = prRemainOnChannel->u4DurationMs;
	prAisFsmInfo->rChReqInfo.u8Cookie = prRemainOnChannel->u8Cookie;
	prAisFsmInfo->rChReqInfo.eReqType = prRemainOnChannel->eReqType;

	if ((prAisFsmInfo->eCurrentState == AIS_STATE_IDLE) ||
	    (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR
	     && !timerPendingTimer(&prAisFsmInfo->rJoinTimeoutTimer))) {
		/* transit to next state */
		aisFsmSteps(prAdapter, AIS_STATE_REQ_REMAIN_ON_CHANNEL,
			ucBssIndex);
	} else {
		aisFsmInsertRequest(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL,
			ucBssIndex);
	}

	/* free messages */
	cnmMemFree(prAdapter, prMsgHdr);
}

void aisFsmRunEventCancelRemainOnChannel(struct ADAPTER *prAdapter,
					 struct MSG_HDR *prMsgHdr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct MSG_CANCEL_REMAIN_ON_CHANNEL *prCancelRemainOnChannel;
	u_int8_t rReturn;
	uint8_t ucBssIndex = 0;

	prCancelRemainOnChannel =
	    (struct MSG_CANCEL_REMAIN_ON_CHANNEL *)prMsgHdr;

	ucBssIndex = prCancelRemainOnChannel->ucBssIdx;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	/* 1. Check the cookie first */
	if (prCancelRemainOnChannel->u8Cookie ==
	    prAisFsmInfo->rChReqInfo.u8Cookie) {

		/* 2. release channel privilege/request */
		if (prAisFsmInfo->eCurrentState ==
		    AIS_STATE_REQ_REMAIN_ON_CHANNEL) {
			/* 2.1 elease channel */
			aisFsmReleaseCh(prAdapter, ucBssIndex);
		} else if (prAisFsmInfo->eCurrentState ==
			   AIS_STATE_REMAIN_ON_CHANNEL) {
			/* 2.1 release channel */
			aisFsmReleaseCh(prAdapter, ucBssIndex);

			/* 2.2 stop channel timeout timer */
			cnmTimerStopTimer(prAdapter,
					  &prAisFsmInfo->rChannelTimeoutTimer);
		}

		/* 3. clear pending request of remain_on_channel */
		rReturn = aisFsmClearRequest(prAdapter,
			AIS_REQUEST_REMAIN_ON_CHANNEL, ucBssIndex);

		DBGLOG(AIS, TRACE,
			"rReturn of aisFsmIsRequestPending is %d", rReturn);

		/* 4. decide which state to retreat */
		if (prAisFsmInfo->eCurrentState ==
		    AIS_STATE_REQ_REMAIN_ON_CHANNEL
		    || prAisFsmInfo->eCurrentState ==
		    AIS_STATE_REMAIN_ON_CHANNEL) {
			if (prAisBssInfo->eConnectionState ==
			    MEDIA_STATE_CONNECTED)
				aisFsmSteps(prAdapter, AIS_STATE_NORMAL_TR,
					ucBssIndex);
			else
				aisFsmSteps(prAdapter, AIS_STATE_IDLE,
					ucBssIndex);
		}
	}

	/* 5. free message */
	cnmMemFree(prAdapter, prMsgHdr);
}

static u_int8_t
aisFunChnlReqByOffChnl(struct ADAPTER *prAdapter,
		struct AIS_OFF_CHNL_TX_REQ_INFO *prOffChnlTxReq,
		uint8_t ucBssIndex)
{
	struct MSG_REMAIN_ON_CHANNEL *prMsgChnlReq =
			(struct MSG_REMAIN_ON_CHANNEL *) NULL;

	prMsgChnlReq = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			sizeof(struct MSG_REMAIN_ON_CHANNEL));
	if (prMsgChnlReq == NULL) {
		DBGLOG(AIS, ERROR, "channel request buffer allocate fails.\n");
		return FALSE;
	}

	prMsgChnlReq->u8Cookie = prOffChnlTxReq->u8Cookie;
	prMsgChnlReq->u4DurationMs = prOffChnlTxReq->u4Duration;
	prMsgChnlReq->ucChannelNum = prOffChnlTxReq->rChannelInfo.ucChannelNum;
	prMsgChnlReq->eBand = prOffChnlTxReq->rChannelInfo.eBand;
	prMsgChnlReq->eSco = prOffChnlTxReq->eChnlExt;
	prMsgChnlReq->eReqType = CH_REQ_TYPE_OFFCHNL_TX;

	prMsgChnlReq->ucBssIdx = ucBssIndex;

	aisFsmRunEventRemainOnChannel(prAdapter,
			(struct MSG_HDR *) prMsgChnlReq);
	return TRUE;
}

static u_int8_t
aisFunAddTxReq2Queue(struct ADAPTER *prAdapter,
		struct AIS_MGMT_TX_REQ_INFO *prMgmtTxReqInfo,
		struct MSG_MGMT_TX_REQUEST *prMgmtTxMsg,
		struct AIS_OFF_CHNL_TX_REQ_INFO **pprOffChnlTxReq)
{
	struct AIS_OFF_CHNL_TX_REQ_INFO *prTmpOffChnlTxReq =
			(struct AIS_OFF_CHNL_TX_REQ_INFO *) NULL;

	prTmpOffChnlTxReq = cnmMemAlloc(prAdapter,
			RAM_TYPE_MSG,
			sizeof(struct AIS_OFF_CHNL_TX_REQ_INFO));

	if (prTmpOffChnlTxReq == NULL) {
		DBGLOG(AIS, ERROR, "Allocate TX request buffer fails.\n");
		return FALSE;
	}

	prTmpOffChnlTxReq->u8Cookie = prMgmtTxMsg->u8Cookie;
	prTmpOffChnlTxReq->prMgmtTxMsdu = prMgmtTxMsg->prMgmtMsduInfo;
	prTmpOffChnlTxReq->fgNoneCckRate = prMgmtTxMsg->fgNoneCckRate;
	kalMemCopy(&prTmpOffChnlTxReq->rChannelInfo,
			&prMgmtTxMsg->rChannelInfo,
			sizeof(struct RF_CHANNEL_INFO));
	prTmpOffChnlTxReq->eChnlExt = prMgmtTxMsg->eChnlExt;
	prTmpOffChnlTxReq->fgIsWaitRsp = prMgmtTxMsg->fgIsWaitRsp;
	prTmpOffChnlTxReq->u4Duration = prMgmtTxMsg->u4Duration;

	LINK_INSERT_TAIL(&prMgmtTxReqInfo->rTxReqLink,
			&prTmpOffChnlTxReq->rLinkEntry);

	*pprOffChnlTxReq = prTmpOffChnlTxReq;

	return TRUE;
}

static uint32_t
aisFunHandleOffchnlTxReq(struct ADAPTER *prAdapter,
		struct AIS_FSM_INFO *prAisFsmInfo,
		struct MSG_MGMT_TX_REQUEST *prMgmtTxMsg,
		uint8_t ucBssIndex)
{
	struct AIS_OFF_CHNL_TX_REQ_INFO *prOffChnlTxReq =
			(struct AIS_OFF_CHNL_TX_REQ_INFO *) NULL;
	struct AIS_MGMT_TX_REQ_INFO *prMgmtTxReqInfo =
			(struct AIS_MGMT_TX_REQ_INFO *) NULL;

	prMgmtTxReqInfo = &(prAisFsmInfo->rMgmtTxInfo);

	if (prMgmtTxMsg->u4Duration < MIN_TX_DURATION_TIME_MS)
		prMgmtTxMsg->u4Duration = MIN_TX_DURATION_TIME_MS;

	if (aisFunAddTxReq2Queue(prAdapter, prMgmtTxReqInfo,
			prMgmtTxMsg, &prOffChnlTxReq) == FALSE)
		goto error;

	if (prOffChnlTxReq == NULL)
		goto error;

	if (!aisFunChnlReqByOffChnl(prAdapter, prOffChnlTxReq,
		ucBssIndex))
		goto error;

	return WLAN_STATUS_SUCCESS;
error:
	LINK_REMOVE_KNOWN_ENTRY(
			&(prMgmtTxReqInfo->rTxReqLink),
			&prOffChnlTxReq->rLinkEntry);
	cnmPktFree(prAdapter, prOffChnlTxReq->prMgmtTxMsdu);
	cnmMemFree(prAdapter, prOffChnlTxReq);

	return WLAN_STATUS_RESOURCES;
}

static u_int8_t
aisFunNeedOffchnlTx(struct ADAPTER *prAdapter,
		struct MSG_MGMT_TX_REQUEST *prMgmtTxMsg)
{
	struct BSS_INFO *prAisBssInfo = (struct BSS_INFO *) NULL;
	struct AIS_FSM_INFO *prAisFsmInfo;
	uint8_t ucBssIndex = 0;

	ucBssIndex = prMgmtTxMsg->ucBssIdx;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	if (!prMgmtTxMsg->fgIsOffChannel)
		return FALSE;

	/* tx channel == op channel */
	if (prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED &&
			prAisBssInfo->ucPrimaryChannel ==
				prMgmtTxMsg->rChannelInfo.ucChannelNum)
		return FALSE;

	/* tx channel == roc channel */
	if (prAisFsmInfo->fgIsChannelGranted &&
			prAisFsmInfo->rChReqInfo.ucChannelNum ==
			prMgmtTxMsg->rChannelInfo.ucChannelNum)
		return FALSE;

	DBGLOG(REQ, INFO, "Use offchannel to TX.\n");

	return TRUE;
}

void aisFsmRunEventMgmtFrameTx(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_MGMT_TX_REQUEST *prMgmtTxMsg =
			(struct MSG_MGMT_TX_REQUEST *) NULL;
	struct BSS_INFO *prAisBssInfo;
	uint32_t u4Status;
	uint8_t ucBssIndex = 0;

	if (!prAdapter || !prMsgHdr)
		return;

	prMgmtTxMsg = (struct MSG_MGMT_TX_REQUEST *) prMsgHdr;
	ucBssIndex = prMgmtTxMsg->ucBssIdx;
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	if (prAisFsmInfo == NULL)
		goto exit;

	if (!aisFunNeedOffchnlTx(prAdapter, prMgmtTxMsg)) {
		aisFuncTxMgmtFrame(prAdapter,
				&prAisFsmInfo->rMgmtTxInfo,
				prMgmtTxMsg->prMgmtMsduInfo,
				prMgmtTxMsg->u8Cookie,
				ucBssIndex);
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE ||
		   prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR ||
		   aisFsmIsSwitchChannel(prAdapter, prAisFsmInfo)) {
		u4Status = aisFunHandleOffchnlTxReq(prAdapter,
				prAisFsmInfo,
				prMgmtTxMsg,
				ucBssIndex);
		if (u4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(AIS, WARN, "Handle TX mgmt failed.\n",
				aisGetFsmState(prAisFsmInfo->eCurrentState));
			kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
				  prMgmtTxMsg->u8Cookie,
				  FALSE,
				  prMgmtTxMsg->prMgmtMsduInfo->prPacket,
				  (uint32_t)
				  prMgmtTxMsg->prMgmtMsduInfo->u2FrameLength,
				  ucBssIndex);
		}
	} else {
		DBGLOG(AIS, WARN, "Disable TX mgmt when state=%s.\n",
			aisGetFsmState(prAisFsmInfo->eCurrentState));
		kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
			  prMgmtTxMsg->u8Cookie,
			  FALSE,
			  prMgmtTxMsg->prMgmtMsduInfo->prPacket,
			  (uint32_t)
			  prMgmtTxMsg->prMgmtMsduInfo->u2FrameLength,
			  ucBssIndex);
	}

exit:
	cnmMemFree(prAdapter, prMsgHdr);
}				/* aisFsmRunEventMgmtFrameTx */

void aisFsmRunEventChannelTimeout(struct ADAPTER *prAdapter,
				  uintptr_t ulParamPtr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	uint8_t ucBssIndex = (uint8_t) ulParamPtr;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	if (prAisFsmInfo->eCurrentState == AIS_STATE_REMAIN_ON_CHANNEL) {
		/* 1. release channel */
		aisFsmReleaseCh(prAdapter, ucBssIndex);

		/* 2. stop channel timeout timer */
		cnmTimerStopTimer(prAdapter,
				  &prAisFsmInfo->rChannelTimeoutTimer);

		/* 3. expiration indication to upper layer */
		kalRemainOnChannelExpired(prAdapter->prGlueInfo,
					  prAisFsmInfo->rChReqInfo.u8Cookie,
					  prAisFsmInfo->rChReqInfo.eBand,
					  prAisFsmInfo->rChReqInfo.eSco,
					  prAisFsmInfo->rChReqInfo.ucChannelNum,
					  ucBssIndex);

		/* 4. decide which state to retreat */
		if (prAisBssInfo->eConnectionState ==
		    MEDIA_STATE_CONNECTED)
			aisFsmSteps(prAdapter, AIS_STATE_NORMAL_TR,
				ucBssIndex);
		else
			aisFsmSteps(prAdapter, AIS_STATE_IDLE,
				ucBssIndex);

	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_OFF_CHNL_TX) {
		aisFsmSteps(prAdapter, AIS_STATE_OFF_CHNL_TX,
			ucBssIndex);
	} else {
		DBGLOG(AIS, WARN,
		       "Unexpected remain_on_channel timeout event\n");
		DBGLOG(AIS, STATE, "CURRENT State: [%s]\n",
			aisGetFsmState(prAisFsmInfo->eCurrentState));
	}
}

uint32_t
aisFsmRunEventMgmtFrameTxDone(struct ADAPTER *prAdapter,
			      struct MSDU_INFO *prMsduInfo,
			      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_MGMT_TX_REQ_INFO *prMgmtTxReqInfo =
	    (struct AIS_MGMT_TX_REQ_INFO *)NULL;
	u_int8_t fgIsSuccess = FALSE;
	uint64_t *pu8GlCookie = (uint64_t *) NULL;
	uint8_t ucBssIndex = 0;

	do {
		ucBssIndex = prMsduInfo->ucBssIndex;

		prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
		prMgmtTxReqInfo = &(prAisFsmInfo->rMgmtTxInfo);
		pu8GlCookie =
			(uint64_t *) ((uintptr_t) prMsduInfo->prPacket +
				(uintptr_t) prMsduInfo->u2FrameLength +
				MAC_TX_RESERVED_FIELD);

#if (CFG_SUPPORT_CONN_LOG == 1)
		connLogMgmtTx(prAdapter,
			prMsduInfo,
			rTxDoneStatus);
#endif
#if CFG_SUPPORT_WPA3_LOG
		wpa3LogMgmtTx(prAdapter,
			prMsduInfo,
			rTxDoneStatus);
#endif

		if (rTxDoneStatus != TX_RESULT_SUCCESS) {
			DBGLOG(AIS, ERROR, "Mgmt Frame TX Fail, Status:%d.\n",
			       rTxDoneStatus);
		} else {
			fgIsSuccess = TRUE;
			DBGLOG(AIS, INFO,
				"Mgmt Frame TX Success, cookie: 0x%llx.\n",
				*pu8GlCookie);
#if CFG_SUPPORT_NCHO
			if (prAdapter->rNchoInfo.fgNCHOEnabled == TRUE &&
			    prAdapter->rNchoInfo.fgIsSendingAF == TRUE &&
			    prAdapter->rNchoInfo.fgChGranted == TRUE) {
				prAdapter->rNchoInfo.fgIsSendingAF = FALSE;
				DBGLOG(AIS, TRACE, "NCHO action frame tx done");
			}
#endif
		}

		if (prMgmtTxReqInfo->prMgmtTxMsdu == prMsduInfo) {
			kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
						prMgmtTxReqInfo->u8Cookie,
						fgIsSuccess,
						prMsduInfo->prPacket,
						(uint32_t)
						prMsduInfo->u2FrameLength,
						ucBssIndex);

			prMgmtTxReqInfo->prMgmtTxMsdu = NULL;
		}

	} while (FALSE);

	return WLAN_STATUS_SUCCESS;

}				/* aisFsmRunEventMgmtFrameTxDone */

uint32_t
aisFuncTxMgmtFrame(struct ADAPTER *prAdapter,
		   struct AIS_MGMT_TX_REQ_INFO *prMgmtTxReqInfo,
		   struct MSDU_INFO *prMgmtTxMsdu, uint64_t u8Cookie,
		   uint8_t ucBssIndex)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct MSDU_INFO *prTxMsduInfo = (struct MSDU_INFO *)NULL;
	struct WLAN_MAC_HEADER *prWlanHdr = (struct WLAN_MAC_HEADER *)NULL;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *)NULL;
	uint32_t ucStaRecIdx = STA_REC_INDEX_NOT_FOUND;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct BSS_INFO *prBssInfo;
	struct MLD_STA_RECORD *prMldStarec;
	struct CONNECTION_SETTINGS *prConnSettings = NULL;
	u_int8_t fgConnReqMloSupport = FALSE;
#endif

	do {
		if (prMgmtTxReqInfo->fgIsMgmtTxRequested) {

			/* 1. prMgmtTxReqInfo->prMgmtTxMsdu != NULL */
			/* Packet on driver, not done yet, drop it. */
			prTxMsduInfo = prMgmtTxReqInfo->prMgmtTxMsdu;
			if (prTxMsduInfo != NULL) {

				kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
					  prMgmtTxReqInfo->u8Cookie,
					  FALSE,
					  prTxMsduInfo->prPacket,
					  (uint32_t)
					  prTxMsduInfo->u2FrameLength,
					  ucBssIndex);

				/* Leave it to TX Done handler. */
				/* cnmMgtPktFree(prAdapter, prTxMsduInfo); */
				prMgmtTxReqInfo->prMgmtTxMsdu = NULL;
			}
			/* 2. prMgmtTxReqInfo->prMgmtTxMsdu == NULL */
			/* Packet transmitted, wait tx done. (cookie issue) */
		}

		prWlanHdr =
		    (struct WLAN_MAC_HEADER *)((uintptr_t)
					       prMgmtTxMsdu->prPacket +
					       MAC_TX_RESERVED_FIELD);

		DBGLOG(AIS, INFO,
			"TX Mgmt A1["MACSTR"] A2["MACSTR"] A3["MACSTR"]\n",
			MAC2STR(prWlanHdr->aucAddr1),
			MAC2STR(prWlanHdr->aucAddr2),
			MAC2STR(prWlanHdr->aucAddr3));

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
		if (prConnSettings)
			fgConnReqMloSupport = !!(prConnSettings->u4ConnFlags &
					 CONNECT_REQ_MLO_SUPPORT);
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
		prStaRec = aisGetTargetStaRec(prAdapter, ucBssIndex);
		prMldStarec = mldStarecGetByStarec(prAdapter, prStaRec);
		if (fgConnReqMloSupport && prMldStarec && prBssInfo &&
		    prStaRec) {
			DBGLOG(AIS, INFO,
				"TX Mgmt to DA["MACSTR"] SA["MACSTR"] MLD["
				MACSTR"]\n",
				MAC2STR(prStaRec->aucMacAddr),
				MAC2STR(prBssInfo->aucOwnMacAddr),
				MAC2STR(prMldStarec->aucPeerMldAddr));
			COPY_MAC_ADDR(prWlanHdr->aucAddr1,
				prStaRec->aucMacAddr);
			COPY_MAC_ADDR(prWlanHdr->aucAddr2,
				prBssInfo->aucOwnMacAddr);
			COPY_MAC_ADDR(prWlanHdr->aucAddr3,
				prStaRec->aucMacAddr);
		}
#endif

		prStaRec =
		    cnmGetStaRecByAddress(prAdapter,
					  ucBssIndex,
					  prWlanHdr->aucAddr1);

		if (IS_BMCAST_MAC_ADDR(prWlanHdr->aucAddr1))
			ucStaRecIdx = STA_REC_INDEX_BMCAST;

		if (prStaRec)
			ucStaRecIdx = prStaRec->ucIndex;

		TX_SET_MMPDU(prAdapter,
			     prMgmtTxMsdu,
			     (prStaRec !=
			      NULL) ? (prStaRec->
				       ucBssIndex)
			     : (ucBssIndex),
			     ucStaRecIdx,
			     WLAN_MAC_MGMT_HEADER_LEN,
			     prMgmtTxMsdu->u2FrameLength,
			     aisFsmRunEventMgmtFrameTxDone,
			     MSDU_RATE_MODE_AUTO);
		prMgmtTxReqInfo->u8Cookie = u8Cookie;
		prMgmtTxReqInfo->prMgmtTxMsdu = prMgmtTxMsdu;
		prMgmtTxReqInfo->fgIsMgmtTxRequested = TRUE;

		if (prWlanHdr->u2FrameCtrl == MAC_FRAME_ACTION)
			nicTxSetPktLifeTime(prAdapter, prMgmtTxMsdu,
				AIS_ACTION_FRAME_TX_LIFE_TIME_MS);

		nicTxConfigPktControlFlag(prMgmtTxMsdu,
					  MSDU_CONTROL_FLAG_FORCE_TX, TRUE);

		/* send to TX queue */
		nicTxEnqueueMsdu(prAdapter, prMgmtTxMsdu);

	} while (FALSE);

	return rWlanStatus;
}				/* aisFuncTxMgmtFrame */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Action Frame and indicate to uppoer
 *            layer.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[out] pu4ControlFlags   Control flags for replying the Probe Response
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void aisFuncValidateRxActionFrame(struct ADAPTER *prAdapter,
				  struct SW_RFB *prSwRfb)
{
	struct AIS_FSM_INFO *prAisFsmInfo = (struct AIS_FSM_INFO *)NULL;
	uint8_t ucBssIndex = 0;

	if (prSwRfb->prStaRec)
		ucBssIndex = prSwRfb->prStaRec->ucBssIndex;

	/* CFG_SUPPORT_NAN and CFG_ENABLE_WIFI_DIRECT
	 * consider to bypass AIS RxActionFrame
	 */
	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		DBGLOG(AIS, LOUD,
			"Use default, invalid index = %d\n", ucBssIndex);
		return;
	}

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* All action frames indicate to wpa_supplicant */
	/* Leave the action frame to wpa_supplicant. */
	kalIndicateRxMgmtFrame(prAdapter, prAdapter->prGlueInfo,
		prSwRfb, ucBssIndex, MLD_LINK_ID_NONE);

	return;
}				/* aisFuncValidateRxActionFrame */

/* Support AP Selection */
void aisRefreshFWKBlocklist(struct ADAPTER *prAdapter)
{
	struct AIS_BLOCKLIST_ITEM *prEntry = NULL;
	struct LINK *prBlockList = &prAdapter->rWifiVar.rBlockList.rUsingLink;

	DBGLOG(AIS, INFO,
		"Refresh all the BSSes' fgIsInFWKBlocklist to FALSE\n");

	LINK_FOR_EACH_ENTRY(prEntry, prBlockList, rLinkEntry,
			    struct AIS_BLOCKLIST_ITEM) {
		prEntry->fgIsInFWKBlocklist = FALSE;
	}
}

void aisBssTmpDisallow(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc,
	uint32_t sec, int32_t rssiThreshold)
{
#if CFG_SUPPORT_MBO
	struct AIS_BLOCKLIST_ITEM *blk =
		aisAddBlocklist(prAdapter, prBssDesc);

	if (blk) {
		DBGLOG(AIS, INFO,
			"New delay %d, Original timer %d, rssi threshold %d",
			sec, blk->u2DisallowSec, rssiThreshold);

		blk->fgDisallowed = TRUE;
		if (sec > blk->u2DisallowSec)
			blk->u2DisallowSec = sec;
		blk->i4RssiThreshold = rssiThreshold;
	}
#endif
}

void aisHandleArpNoResponse(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_BLOCKLIST_ITEM *prBlocklistItem;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prBlocklistItem = aisAddBlocklist(
		prAdapter, aisGetMainLinkBssDesc(prAisFsmInfo));

	if (prBlocklistItem)
		prBlocklistItem->fgArpNoResponse = TRUE;

	aisBssBeaconTimeout(prAdapter, ucBssIndex);
}

struct AIS_BLOCKLIST_ITEM *aisAddBlocklist(struct ADAPTER *prAdapter,
					   struct BSS_DESC *prBssDesc)
{
	struct AIS_BLOCKLIST_ITEM *prEntry = NULL;
	struct LINK_MGMT *prBlockList = &prAdapter->rWifiVar.rBlockList;

	if (!prBssDesc) {
		DBGLOG(AIS, ERROR, "bss descriptor is NULL\n");
		return NULL;
	}
	if (prBssDesc->prBlock) {
		GET_CURRENT_SYSTIME(&prBssDesc->prBlock->rAddTime);
		prBssDesc->prBlock->ucCount++;
		if (prBssDesc->prBlock->ucCount > 10)
			prBssDesc->prBlock->ucCount = 10;
		DBGLOG(AIS, INFO, "update blocklist for " MACSTR
		       ", count %d\n",
		       MAC2STR(prBssDesc->aucBSSID),
		       prBssDesc->prBlock->ucCount);
		return prBssDesc->prBlock;
	}

	prEntry = aisQueryBlockList(prAdapter, prBssDesc);

	if (prEntry) {
		GET_CURRENT_SYSTIME(&prEntry->rAddTime);
		prBssDesc->prBlock = prEntry;
		prEntry->ucCount++;
		if (prEntry->ucCount > 10)
			prEntry->ucCount = 10;
		DBGLOG(AIS, INFO, "update blocklist for " MACSTR
		       ", count %d\n",
		       MAC2STR(prBssDesc->aucBSSID), prEntry->ucCount);
		return prEntry;
	}
	LINK_MGMT_GET_ENTRY(prBlockList, prEntry, struct AIS_BLOCKLIST_ITEM,
			    VIR_MEM_TYPE);
	if (!prEntry) {
		DBGLOG(AIS, WARN, "No memory to allocate\n");
		return NULL;
	}
	prEntry->ucCount = 1;
	/* Support AP Selection */
	prEntry->fgIsInFWKBlocklist = FALSE;
	COPY_MAC_ADDR(prEntry->aucBSSID, prBssDesc->aucBSSID);
	COPY_SSID(prEntry->aucSSID, prEntry->ucSSIDLen, prBssDesc->aucSSID,
		  prBssDesc->ucSSIDLen);
	GET_CURRENT_SYSTIME(&prEntry->rAddTime);
	prBssDesc->prBlock = prEntry;

	DBGLOG(AIS, INFO, "Add " MACSTR " to block List\n",
	       MAC2STR(prBssDesc->aucBSSID));
	return prEntry;
}

void aisRemoveBlockList(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc)
{
	struct AIS_BLOCKLIST_ITEM *prEntry = NULL;

	prEntry = aisQueryBlockList(prAdapter, prBssDesc);
	if (!prEntry)
		return;
	LINK_MGMT_RETURN_ENTRY(&prAdapter->rWifiVar.rBlockList, prEntry);
	prBssDesc->prBlock = NULL;
	DBGLOG(AIS, INFO, "Remove " MACSTR " from blocklist\n",
	       MAC2STR(prBssDesc->aucBSSID));
}

struct AIS_BLOCKLIST_ITEM *aisQueryBlockList(struct ADAPTER *prAdapter,
					     struct BSS_DESC *prBssDesc)
{
	struct AIS_BLOCKLIST_ITEM *prEntry = NULL;
	struct LINK *prBlockList = &prAdapter->rWifiVar.rBlockList.rUsingLink;

	if (!prBssDesc)
		return NULL;
	else if (prBssDesc->prBlock)
		return prBssDesc->prBlock;

	LINK_FOR_EACH_ENTRY(prEntry, prBlockList, rLinkEntry,
			    struct AIS_BLOCKLIST_ITEM) {
		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prEntry->aucBSSID) &&
		    EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
			       prEntry->aucSSID, prEntry->ucSSIDLen))
			return prEntry;
	}
	DBGLOG(AIS, TRACE, MACSTR " is not in blocklist\n",
	       MAC2STR(prBssDesc->aucBSSID));
	return NULL;
}

void aisRemoveTimeoutBlocklist(struct ADAPTER *prAdapter, uint16_t u2Sec)
{
	struct AIS_BLOCKLIST_ITEM *prEntry = NULL;
	struct AIS_BLOCKLIST_ITEM *prNextEntry = NULL;
	struct LINK *prBlockList = &prAdapter->rWifiVar.rBlockList.rUsingLink;
	OS_SYSTIME rCurrent;
	struct BSS_DESC *prBssDesc = NULL;
	struct PARAM_SSID rSsid;

	GET_CURRENT_SYSTIME(&rCurrent);

	LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prBlockList, rLinkEntry,
				 struct AIS_BLOCKLIST_ITEM) {
		uint16_t sec = u2Sec;

		if (prEntry->fgIsInFWKBlocklist == TRUE)
			continue;

		/* For deauth blocklist
		 * 1. If u2Sec = 0, set timeout = 0.
		 * 2. Else, set timeout = AIS_BLOCKLIST_TIMEOUT_DEAUTH
		 */
		if (prEntry->ucDeauthCount > 0 && u2Sec != 0)
			sec = AIS_BLOCKLIST_TIMEOUT_DEAUTH;

#if CFG_SUPPORT_MBO
		if (prEntry->fgDisallowed)
			sec = prEntry->u2DisallowSec;
#endif
		if (!CHECK_FOR_TIMEOUT(rCurrent, prEntry->rAddTime,
				       SEC_TO_MSEC(sec)))
			continue;

		kalMemZero(&rSsid, sizeof(struct PARAM_SSID));
		COPY_SSID(rSsid.aucSsid,
			  rSsid.u4SsidLen,
			  prEntry->aucSSID,
			  prEntry->ucSSIDLen);

		prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
						prEntry->aucBSSID,
						TRUE, &rSsid);
		if (prBssDesc) {
			prBssDesc->prBlock = NULL;
			prBssDesc->ucJoinFailureCount = 0;
			DBGLOG(AIS, INFO,
				"Remove Timeout "MACSTR" from blocklist\n",
			       MAC2STR(prBssDesc->aucBSSID));
		}
		LINK_MGMT_RETURN_ENTRY(&prAdapter->rWifiVar.rBlockList,
			prEntry);
	}
}

void aisRemoveDeauthBlocklist(struct ADAPTER *prAdapter,
				u_int8_t fgIsDisconnect)
{
	struct AIS_BLOCKLIST_ITEM *prEntry = NULL;
	struct AIS_BLOCKLIST_ITEM *prNextEntry = NULL;
	struct LINK *prBlockList = &prAdapter->rWifiVar.rBlockList.rUsingLink;
	struct BSS_DESC *prBssDesc = NULL;
	struct PARAM_SSID rSsid;

	LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prBlockList, rLinkEntry,
				 struct AIS_BLOCKLIST_ITEM) {
		if (prEntry->fgIsInFWKBlocklist ||
		    !prEntry->ucDeauthCount)
			continue;

		kalMemZero(&rSsid, sizeof(struct PARAM_SSID));
		COPY_SSID(rSsid.aucSsid,
			  rSsid.u4SsidLen,
			  prEntry->aucSSID,
			  prEntry->ucSSIDLen);

		prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
						prEntry->aucBSSID,
						TRUE, &rSsid);

		if (fgIsDisconnect) {
			if (prBssDesc) {
				prBssDesc->prBlock = NULL;
				prBssDesc->ucJoinFailureCount = 0;
				DBGLOG(AIS, INFO,
				       "Remove deauth "MACSTR
				       " from blocklist\n",
				       MAC2STR(prBssDesc->aucBSSID));
			}
			LINK_MGMT_RETURN_ENTRY(&prAdapter->rWifiVar.rBlockList,
				prEntry);
		} else {
			prEntry->fgDeauthLastTime = FALSE;
			if (prBssDesc) {
				prBssDesc->ucJoinFailureCount = 0;
				DBGLOG(AIS, INFO, "Remove " MACSTR
				  " last deauth flag, deauth count = %d\n",
				  MAC2STR(prBssDesc->aucBSSID),
				  prEntry->ucDeauthCount);
			}
		}
	}
}

void aisRemoveArpNRBlocklist(struct ADAPTER *prAdapter, uint16_t u2Sec)
{
	struct AIS_BLOCKLIST_ITEM *prEntry = NULL;
	struct AIS_BLOCKLIST_ITEM *prNextEntry = NULL;
	struct LINK *prBlockList = &prAdapter->rWifiVar.rBlockList.rUsingLink;
	struct BSS_DESC *prBssDesc = NULL;
	OS_SYSTIME rCurrent;

	GET_CURRENT_SYSTIME(&rCurrent);

	LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prBlockList, rLinkEntry,
				 struct AIS_BLOCKLIST_ITEM) {
		if (prEntry->fgIsInFWKBlocklist ||
		    !prEntry->fgArpNoResponse)
			continue;

		if (!CHECK_FOR_TIMEOUT(rCurrent, prEntry->rAddTime,
				       SEC_TO_MSEC(u2Sec)))
			continue;

		prBssDesc = scanSearchBssDescByBssid(prAdapter,
						     prEntry->aucBSSID);

		if (prBssDesc) {
			prBssDesc->prBlock = NULL;
			prBssDesc->ucJoinFailureCount = 0;
			DBGLOG(AIS, INFO,
			       "Remove ARP NR "MACSTR" from blocklist\n",
			       MAC2STR(prBssDesc->aucBSSID));
		}
		LINK_MGMT_RETURN_ENTRY(&prAdapter->rWifiVar.rBlockList,
			prEntry);
	}
}

#if CFG_SUPPORT_802_11V_BTM_OFFLOAD
void aisFsmRunEventBssTransition(struct ADAPTER *prAdapter,
				 struct MSG_HDR *prMsgHdr)
{
	struct MSG_AIS_BSS_TRANSITION *prMsg =
	    (struct MSG_AIS_BSS_TRANSITION *)prMsgHdr;
	struct AIS_FSM_INFO *prAisFsmInfo =
			(struct AIS_FSM_INFO *) NULL;
	struct BSS_TRANSITION_MGT_PARAM *prBtmParam;
	struct BSS_DESC *prBssDesc;
	struct ROAMING_INFO *prRoamingFsmInfo = NULL;
	struct CMD_ROAMING_TRANSIT rRoamingData;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_STA_RECORD *prMldStarec;
#endif
	uint8_t ucBssIndex = 0;
	uint8_t ucReqMode = 0;

	if (!prMsg) {
		DBGLOG(AIS, WARN, "Msg Header is NULL\n");
		return;
	}

	ucBssIndex = prMsg->ucBssIndex;
	cnmMemFree(prAdapter, prMsgHdr);

	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
	if (!prBssDesc) {
		DBGLOG(AIS, WARN, "prBssDesc is NULL\n");
		return;
	}

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prBtmParam = aisGetBTMParam(prAdapter, ucBssIndex);
	ucReqMode = prBtmParam->ucRequestMode;
	prBtmParam->ucDisImmiState = AIS_BTM_DIS_IMMI_STATE_0;

	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);
	kalMemZero(&rRoamingData, sizeof(struct CMD_ROAMING_TRANSIT));

	/* update cached channel list */
	aisFsmGetCurrentEssChnlList(prAdapter, ucBssIndex);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
#if (CFG_SUPPORT_ML_RECONFIG == 1)
	/* unsolicited btm comes, stop removoal timer and continure to roam */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rApRemovalTimer);
#endif /* CFG_SUPPORT_ML_RECONFIG */

	DBGLOG(AIS, INFO, "DIS_IMMT %d, BSS_TERM %d, T2LM_NEGO %d\n",
		!!(ucReqMode & WNM_BSS_TM_REQ_DISASSOC_IMMINENT),
		!!(ucReqMode & WNM_BSS_TM_REQ_BSS_TERMINATION_INCLUDED),
		(prBssDesc->rMlInfo.u2MldCap & MLD_CAP_TID_TO_LINK_NEGO_MASK) >>
		MLD_CAP_TID_TO_LINK_NEGO_SHIFT);

	prMldStarec = mldStarecGetByStarec(prAdapter,
				aisGetMainLinkStaRec(prAisFsmInfo));

	/* prefer using t2lm for multi link */
	if (prAdapter->rWifiVar.ucT2LMNegotiationSupport &&
	    (prBssDesc->rMlInfo.u2MldCap & MLD_CAP_TID_TO_LINK_NEGO_MASK) &&
	    !(ucReqMode & WNM_BSS_TM_REQ_BSS_TERMINATION_INCLUDED) &&
	    prMldStarec && prMldStarec->rStarecList.u4NumElem > 1) {
		struct NEIGHBOR_AP *prNei;

		prNei = aisGetNeighborAPEntry(prAdapter,
			prBssDesc, ucBssIndex);
		if ((prNei && prNei->ucPreference == 0) ||
		     (ucReqMode & WNM_BSS_TM_REQ_DISASSOC_IMMINENT)) {
#if (CFG_SUPPORT_802_11BE_T2LM == 1)
			int i;
			struct T2LM_INFO *prT2LMParams;
			struct BSS_INFO *prBssInfo;
			uint8_t u2NonPrefLinks;

			u2NonPrefLinks = BIT(prBssDesc->rMlInfo.ucLinkIndex);
			if (prNei && prNei->ucPreference == 0)
				u2NonPrefLinks |= prNei->u2ValidLinks;

			if ((u2NonPrefLinks & prMldStarec->u2ValidLinks) ==
			    prMldStarec->u2ValidLinks) {
				DBGLOG(AIS, WARN,
				     "Skip t2lm all links are NonPref=0x%x mld=0x%x\n",
				     u2NonPrefLinks, prMldStarec->u2ValidLinks);
				goto skip_t2lm;
			}

			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
				prBtmParam->ucRspBssIndex);
			prT2LMParams = (struct T2LM_INFO *)
				kalMemZAlloc(sizeof(struct T2LM_INFO),
				VIR_MEM_TYPE);
			if (!prT2LMParams) {
				DBGLOG(AIS, WARN, "alloc t2lm params failed\n");
				goto skip_t2lm;
			}
			prT2LMParams->ucDirection = 2;
			prT2LMParams->ucDefaultLM = 0;
			prT2LMParams->ucSwitchTimePresent = 0;
			prT2LMParams->ucDurationPresent = 0;
			prT2LMParams->ucLMSize = 1;
			prT2LMParams->ucLMIndicator = 255;
			for (i = 0; i < MAX_NUM_T2LM_TIDS; i++)
				prT2LMParams->au2LMTid[i] =
				    ~u2NonPrefLinks & prMldStarec->u2ValidLinks;

			DBGLOG(AIS, INFO,
			      "Send t2lm for load balance NonPref=0x%x mld=0x%x\n",
			      u2NonPrefLinks, prMldStarec->u2ValidLinks);

			t2lmSend(prAdapter, TID2LINK_REQUEST,
					prBssInfo, prT2LMParams);
			kalMemFree(prT2LMParams, VIR_MEM_TYPE,
				sizeof(struct prT2LMParams));
#else
			goto skip_t2lm;
#endif
			/* per spec, no need to send btm if already send t2lm */
		} else if (prBtmParam->fgPendingResponse) {
			prBtmParam->fgPendingResponse = false;
			wnmSendBTMResponseFrame(prAdapter,
				aisGetStaRecOfAP(prAdapter,
				prBtmParam->ucRspBssIndex),
				NULL,
				prBtmParam->ucDialogToken,
				WNM_BSS_TM_REJECT_UNSPECIFIED,
				MBO_TRANSITION_REJECT_REASON_UNSPECIFIED,
				0, NULL);
		}

		return;
	}
skip_t2lm:
#endif

	if (ucReqMode & WNM_BSS_TM_REQ_DISASSOC_IMMINENT) {
#if CFG_SUPPORT_MBO
		if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgBTMBlocklist))
			aisBssTmpDisallow(prAdapter, prBssDesc,
				MSEC_TO_SEC(prBtmParam->u4ReauthDelay), 0);
#endif

		if (prBtmParam->u4ReauthDelay >
			prAdapter->rWifiVar.u4BtmDisThreshold)
			prBtmParam->ucDisImmiState = AIS_BTM_DIS_IMMI_STATE_1;
		else
			prBtmParam->ucDisImmiState = AIS_BTM_DIS_IMMI_STATE_2;
	} else if (ucReqMode & WNM_BSS_TM_REQ_BSS_TERMINATION_INCLUDED) {
		uint64_t msec = prBtmParam->u2TermDuration * MSEC_PER_MIN;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		/* TODO: consider tsf timer to decide period to disallow AP */
		if (prBssDesc->rMlInfo.fgValid) {
			if (ucReqMode & WNM_BSS_TM_REQ_LINK_REMOVAL_IMMINENT) {
				aisBssTmpDisallow(prAdapter, prBssDesc,
				     MSEC_TO_SEC(msec), 0);
			} else {
				struct BSS_DESC *bss;
				struct LINK *scan_result =
				    &prAdapter->rWifiVar.rScanInfo.rBSSDescList;

				LINK_FOR_EACH_ENTRY(bss, scan_result,
					rLinkEntry, struct BSS_DESC) {
					if (bss->rMlInfo.fgValid &&
					    EQUAL_MAC_ADDR(
						bss->rMlInfo.aucMldAddr,
						prBssDesc->rMlInfo.aucMldAddr))
						aisBssTmpDisallow(prAdapter,
						     bss, MSEC_TO_SEC(msec),
						     0);
				}
			}
		} else

#endif
		{
			aisBssTmpDisallow(prAdapter, prBssDesc,
				MSEC_TO_SEC(msec), 0);
		}

		prBtmParam->ucDisImmiState = AIS_BTM_DIS_IMMI_STATE_1;
	}

	rRoamingData.eReason = ROAMING_REASON_BTM;
#if (CFG_EXT_ROAMING == 1)
	prRoamingFsmInfo->ucRcpi = prBssDesc->ucRCPI;
#endif

	DBGLOG(AIS, INFO, "BTM req roam start, DIS_IMMI_STATE %d\n",
		prBtmParam->ucDisImmiState);
	rRoamingData.u2Data = prBssDesc->ucRCPI;
	rRoamingData.u2RcpiLowThreshold = prRoamingFsmInfo->ucThreshold;
	rRoamingData.ucBssidx = ucBssIndex;
	roamingFsmRunEventDiscovery(prAdapter, &rRoamingData);

	return;
}
#endif /* CFG_SUPPORT_802_11V_BTM_OFFLOAD */

#if CFG_SUPPORT_802_11K
void aisSendNeighborRequest(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct SUB_ELEMENT_LIST *prSSIDIE;
	uint8_t aucBuffer[sizeof(*prSSIDIE) + 31];
	struct BSS_INFO *prBssInfo
		= aisGetAisBssInfo(prAdapter, ucBssIndex);

	kalMemZero(aucBuffer, sizeof(aucBuffer));
	prSSIDIE = (struct SUB_ELEMENT_LIST *)&aucBuffer[0];
	prSSIDIE->rSubIE.ucSubID = ELEM_ID_SSID;
	COPY_SSID(&prSSIDIE->rSubIE.aucOptInfo[0], prSSIDIE->rSubIE.ucLength,
		  prBssInfo->aucSSID, prBssInfo->ucSSIDLen);
	rrmTxNeighborReportRequest(prAdapter, prBssInfo->prStaRecOfAP,
				   prSSIDIE);
}

static u_int8_t aisCandPrefIEIsExist(uint8_t *pucSubIe, uint8_t ucLength)
{
	uint16_t u2Offset = 0;

	IE_FOR_EACH(pucSubIe, ucLength, u2Offset) {
		if (IE_ID(pucSubIe) == ELEM_ID_NR_BSS_TRANSITION_CAND_PREF)
			return TRUE;
	}
	return FALSE;
}

static uint8_t aisGetNeighborApPreference(uint8_t *pucSubIe, uint8_t ucLength)
{
	uint16_t u2Offset = 0;

	IE_FOR_EACH(pucSubIe, ucLength, u2Offset) {
		if (IE_ID(pucSubIe) == ELEM_ID_NR_BSS_TRANSITION_CAND_PREF)
			return pucSubIe[2];
	}
	/* If no preference element is presence, give default value(lowest) 0,
	 */
	/* but it will not be used as a reference. */
	return 0;
}

static uint64_t aisGetBssTermTsf(uint8_t *pucSubIe, uint8_t ucLength)
{
	uint16_t u2Offset = 0;

	IE_FOR_EACH(pucSubIe, ucLength, u2Offset) {
		if (IE_ID(pucSubIe) == ELEM_ID_NR_BSS_TERMINATION_DURATION)
			return *(uint64_t *) &pucSubIe[2];
	}
	/* If no preference element is presence, give default value(lowest) 0 */
	return 0;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint8_t aisCollectNeighborMld(struct ADAPTER *prAdapter,
	struct NEIGHBOR_AP *prNeighborAP, uint8_t *pucSubIe, uint8_t ucLength)
{
	uint16_t u2Offset = 0;
	struct MULTI_LINK_INFO parse, *info = &parse;

	IE_FOR_EACH(pucSubIe, ucLength, u2Offset) {
		if (IE_ID(pucSubIe) == ELEM_ID_NR_BASIC_MULTI_LINK) {
			MLD_PARSE_BASIC_MLIE(info, pucSubIe,
				IE_SIZE(pucSubIe),
				prNeighborAP->aucBssid,
				MAC_FRAME_BEACON);

			if (!info->ucValid)
				return FALSE;

			prNeighborAP->fgIsMld = TRUE;
			COPY_MAC_ADDR(prNeighborAP->aucMldAddr,
				info->aucMldAddr);

			if (!(info->ucMlCtrlPreBmp &
					ML_CTRL_LINK_ID_INFO_PRESENT))
				prNeighborAP->u2ValidLinks = BITS(0, 15);
			else
				prNeighborAP->u2ValidLinks = info->u2ValidLinks;

			return TRUE;
		}
	}

	return FALSE;
}
#endif

struct NEIGHBOR_AP *aisGetNeighborAPEntry(
	struct ADAPTER *prAdapter, struct BSS_DESC *bss, uint8_t ucBssIndex)
{
	struct LINK *prNeighborAPLink =
		&aisGetAisSpecBssInfo(prAdapter, ucBssIndex)
		->rNeighborApList.rUsingLink;
	struct NEIGHBOR_AP *prNeighborAP = NULL;

	LINK_FOR_EACH_ENTRY(prNeighborAP, prNeighborAPLink, rLinkEntry,
			    struct NEIGHBOR_AP)
	{
		if (EQUAL_MAC_ADDR(prNeighborAP->aucBssid, bss->aucBSSID))
			return prNeighborAP;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	LINK_FOR_EACH_ENTRY(prNeighborAP, prNeighborAPLink, rLinkEntry,
			    struct NEIGHBOR_AP)
	{
		if (bss->rMlInfo.fgValid && prNeighborAP->fgIsMld &&
		   EQUAL_MAC_ADDR(prNeighborAP->aucMldAddr,
				  bss->rMlInfo.aucMldAddr) &&
		   (prNeighborAP->u2ValidLinks & BIT(bss->rMlInfo.ucLinkIndex)))
			return prNeighborAP;
	}
#endif

	return NULL;
}

uint32_t aisCollectNeighborAP(struct ADAPTER *prAdapter, uint8_t *pucApBuf,
			  uint16_t u2ApBufLen, uint8_t ucValidInterval,
			  uint8_t ucBssIndex)
{

	struct NEIGHBOR_AP *prNeighborAP = NULL;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo =
	    aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	struct LINK_MGMT *prAPlist = &prAisSpecBssInfo->rNeighborApList;
	struct IE_NEIGHBOR_REPORT *prIe = (struct IE_NEIGHBOR_REPORT *)pucApBuf;
	int16_t c2BufLen;
	uint16_t u2PrefIsZeroCount = 0;
	uint32_t cnt = 0;

	if (!prIe || !u2ApBufLen || u2ApBufLen < prIe->ucLength)
		return 0;
	LINK_MERGE_TO_TAIL(&prAPlist->rFreeLink, &prAPlist->rUsingLink);
	for (c2BufLen = u2ApBufLen; c2BufLen > 0; c2BufLen -= IE_SIZE(prIe),
	     prIe = (struct IE_NEIGHBOR_REPORT *)((uint8_t *) prIe +
						  IE_SIZE(prIe))) {
		uint8_t fgIsMld = FALSE;

		/* BIT0-1: AP reachable, BIT2: same security with current
		 ** setting,
		 ** BIT3: same authenticator with current AP
		 */
		if (prIe->ucId != ELEM_ID_NEIGHBOR_REPORT)
			continue;

		if (prAPlist->rUsingLink.u4NumElem >= WNM_MAX_NEIGHBOR_REPORT)
			break;

		LINK_MGMT_GET_ENTRY(prAPlist, prNeighborAP,
				    struct NEIGHBOR_AP, VIR_MEM_TYPE);
		if (!prNeighborAP)
			break;
		prNeighborAP->fgHT = !!(prIe->u4BSSIDInfo & BIT(11));
		prNeighborAP->fgVht = !!(prIe->u4BSSIDInfo & BIT(12));
		prNeighborAP->fgHe = !!(prIe->u4BSSIDInfo & BIT(14));
		prNeighborAP->fgEht = !!(prIe->u4BSSIDInfo & BIT(21));
		prNeighborAP->fgFromBtm = !!ucValidInterval;
		prNeighborAP->fgRmEnabled = !!(prIe->u4BSSIDInfo & BIT(7));
		prNeighborAP->fgQoS = !!(prIe->u4BSSIDInfo & BIT(5));
		prNeighborAP->fgSameMD = !!(prIe->u4BSSIDInfo & BIT(10));
		prNeighborAP->ucChannel = prIe->ucChnlNumber;
		prNeighborAP->eBand =
#if (CFG_SUPPORT_WIFI_6G == 1)
			IS_6G_OP_CLASS(prIe->ucOperClass) ? BAND_6G :
#endif
			(prNeighborAP->ucChannel <= 14 ? BAND_2G4 : BAND_5G);
		prNeighborAP->fgPrefPresence = aisCandPrefIEIsExist(
			prIe->aucSubElem,
			IE_SIZE(prIe) - OFFSET_OF(struct IE_NEIGHBOR_REPORT,
						   aucSubElem));
		prNeighborAP->ucPreference = aisGetNeighborApPreference(
			prIe->aucSubElem,
			IE_SIZE(prIe) - OFFSET_OF(struct IE_NEIGHBOR_REPORT,
						  aucSubElem));
		prNeighborAP->u8TermTsf = aisGetBssTermTsf(
			prIe->aucSubElem,
			IE_SIZE(prIe) - OFFSET_OF(struct IE_NEIGHBOR_REPORT,
					       aucSubElem));
		COPY_MAC_ADDR(prNeighborAP->aucBssid, prIe->aucBSSID);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		fgIsMld = aisCollectNeighborMld(prAdapter, prNeighborAP,
			prIe->aucSubElem,
			IE_SIZE(prIe) - OFFSET_OF(struct IE_NEIGHBOR_REPORT,
					       aucSubElem));
#endif

		DBGLOG(AIS, INFO,
		       "[%d] Bssid " MACSTR
		       ", PrefPresence %d, Pref %d, Chnl %d, BssidInfo 0x%08x, fgIsMld %d\n",
		       cnt++, MAC2STR(prNeighborAP->aucBssid),
		       prNeighborAP->fgPrefPresence,
		       prNeighborAP->ucPreference, prIe->ucChnlNumber,
		       prIe->u4BSSIDInfo, fgIsMld);

#if CFG_SUPPORT_REPORT_LOG
		wnmLogBTMReqCandiReport(
			prAdapter,
			ucBssIndex,
			prNeighborAP,
			cnt);
#endif

		if (prNeighborAP->fgPrefPresence &&
		    prNeighborAP->ucPreference == 0)
			u2PrefIsZeroCount++;

		if (c2BufLen < IE_SIZE(prIe)) {
			DBGLOG(AIS, WARN, "Truncated neighbor report\n");
			break;
		}
	}
	prAisSpecBssInfo->rNeiApRcvTime = kalGetTimeTick();
	prAisSpecBssInfo->u4NeiApValidInterval =
	    !ucValidInterval
	    ? 0xffffffff
	    : TU_TO_MSEC(ucValidInterval *
			 aisGetAisBssInfo(prAdapter, ucBssIndex)
			 ->u2BeaconInterval);

	if (prAPlist->rUsingLink.u4NumElem > 0 &&
	    prAPlist->rUsingLink.u4NumElem == u2PrefIsZeroCount)
		DBGLOG(AIS, INFO,
		       "The number of valid neighbors is equal to the number of perf value is 0.\n");

	return prAPlist->rUsingLink.u4NumElem;
}

void aisResetNeighborApList(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo =
	    aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	struct LINK_MGMT *prAPlist = &prAisSpecBssInfo->rNeighborApList;

	LINK_MERGE_TO_TAIL(&prAPlist->rFreeLink, &prAPlist->rUsingLink);
	DBGLOG(AIS, INFO, "reset done");
}

uint8_t aisCheckNeighborApValidity(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo =
	    aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	struct LINK_MGMT *prAPlist = &prAisSpecBssInfo->rNeighborApList;
	OS_SYSTIME rCurrent = kalGetTimeTick();

	/* If candidate list is not timeout, just return */
	if (rCurrent <= prAisSpecBssInfo->rNeiApRcvTime ||
	    rCurrent - prAisSpecBssInfo->rNeiApRcvTime <
	    prAisSpecBssInfo->u4NeiApValidInterval) {
		DBGLOG(AIS, TRACE, "valid, Cur %u, Rcv %u, Valid Int %u\n",
			rCurrent, prAisSpecBssInfo->rNeiApRcvTime,
			prAisSpecBssInfo->u4NeiApValidInterval);
		return TRUE;
	}

	if (prAPlist->rUsingLink.u4NumElem > 0) {
		DBGLOG(AIS, INFO, "timeout, Cur %u, Rcv %u, Valid Int %u\n",
			rCurrent, prAisSpecBssInfo->rNeiApRcvTime,
			prAisSpecBssInfo->u4NeiApValidInterval);
		aisResetNeighborApList(prAdapter, ucBssIndex);
	}
	return FALSE;
}

#endif

void aisFsmRunEventCancelTxWait(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr)
{
	struct AIS_FSM_INFO *prAisFsmInfo =
			(struct AIS_FSM_INFO *) NULL;
	struct MSG_CANCEL_TX_WAIT_REQUEST *prCancelTxWaitMsg =
			(struct MSG_CANCEL_TX_WAIT_REQUEST *) NULL;
	struct BSS_INFO *prAisBssInfo = (struct BSS_INFO *) NULL;
	struct AIS_MGMT_TX_REQ_INFO *prMgmtTxInfo =
			(struct AIS_MGMT_TX_REQ_INFO *) NULL;
	struct AIS_OFF_CHNL_TX_REQ_INFO *prOffChnlTxPkt =
			(struct AIS_OFF_CHNL_TX_REQ_INFO *) NULL;
	u_int8_t fgIsCookieFound = FALSE;
	uint8_t ucBssIndex = 0;

	if (prAdapter == NULL || prMsgHdr == NULL)
		goto exit;

	prCancelTxWaitMsg = (struct MSG_CANCEL_TX_WAIT_REQUEST *) prMsgHdr;

	ucBssIndex = prCancelTxWaitMsg->ucBssIdx;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prMgmtTxInfo = &prAisFsmInfo->rMgmtTxInfo;

	if (prAisFsmInfo == NULL ||
		prAisBssInfo == NULL || prMgmtTxInfo == NULL)
		goto exit;

	LINK_FOR_EACH_ENTRY(prOffChnlTxPkt,
			&(prMgmtTxInfo->rTxReqLink),
			rLinkEntry,
			struct AIS_OFF_CHNL_TX_REQ_INFO) {
		if (!prOffChnlTxPkt)
			break;
		if (prOffChnlTxPkt->u8Cookie == prCancelTxWaitMsg->u8Cookie) {
			fgIsCookieFound = TRUE;
			break;
		}
	}

	if (fgIsCookieFound == FALSE && prAisFsmInfo->eCurrentState !=
			AIS_STATE_OFF_CHNL_TX)
		goto exit;

	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer);
	aisFunClearAllTxReq(prAdapter, &(prAisFsmInfo->rMgmtTxInfo));
	aisRestoreBandIdx(prAdapter, prAisBssInfo);
	aisFsmReleaseCh(prAdapter, ucBssIndex);

	if (prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
		/* restore txallow status */
		if (prAisBssInfo->prStaRecOfAP) {
			qmSetStaRecTxAllowed(prAdapter,
				prAisBssInfo->prStaRecOfAP, TRUE);
			DBGLOG(AIS, INFO, "[TxMgmt] TxAllowed = TRUE\n");
		}
		aisFsmSteps(prAdapter, AIS_STATE_NORMAL_TR, ucBssIndex);
	} else {
		aisFsmSteps(prAdapter, AIS_STATE_IDLE, ucBssIndex);
	}

exit:
	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);
}

static void
aisFunClearAllTxReq(struct ADAPTER *prAdapter,
		struct AIS_MGMT_TX_REQ_INFO *prAisMgmtTxInfo)
{
	struct AIS_OFF_CHNL_TX_REQ_INFO *prOffChnlTxPkt =
			(struct AIS_OFF_CHNL_TX_REQ_INFO *) NULL;

	while (!LINK_IS_EMPTY(&(prAisMgmtTxInfo->rTxReqLink))) {
		LINK_REMOVE_HEAD(&(prAisMgmtTxInfo->rTxReqLink),
				prOffChnlTxPkt,
				struct AIS_OFF_CHNL_TX_REQ_INFO *);
		if (!prOffChnlTxPkt)
			continue;
		kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
			prOffChnlTxPkt->u8Cookie,
			FALSE,
			prOffChnlTxPkt->prMgmtTxMsdu->prPacket,
			(uint32_t) prOffChnlTxPkt->prMgmtTxMsdu->u2FrameLength,
			prOffChnlTxPkt->prMgmtTxMsdu->ucBssIndex);
		cnmPktFree(prAdapter, prOffChnlTxPkt->prMgmtTxMsdu);
		cnmMemFree(prAdapter, prOffChnlTxPkt);
	}
}

struct AIS_FSM_INFO *aisGetAisFsmInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		DBGLOG(AIS, WARN,
			"Use default, invalid index=%d caller=%pS\n",
			ucBssIndex, KAL_TRACE);
		return aisGetDefaultAisInfo(prAdapter);
	}

	return aisFsmGetInstance(prAdapter,
		GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->u4PrivateData);
}

struct PARAM_SCAN_REQUEST_ADV *aisGetScanReq(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rScanRequest;
}

struct BSS_DESC_SET *aisGetSearchResult(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rSearchResult;
}

struct AIS_FSM_INFO *aisFsmGetInstance(struct ADAPTER *prAdapter,
	uint8_t ucAisIndex)
{
	if (ucAisIndex < KAL_AIS_NUM)
		return &prAdapter->rWifiVar.rAisFsmInfo[ucAisIndex];
	else
		return NULL;
}

struct AIS_SPECIFIC_BSS_INFO *aisGetAisSpecBssInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rAisSpecificBssInfo;
}

struct BSS_TRANSITION_MGT_PARAM *aisGetBTMParam(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisSpecBssInfo(prAdapter, ucBssIndex)->rBTMParam;
}

struct BSS_INFO *aisGetConnectedBssInfo(
	struct ADAPTER *prAdapter) {

	struct BSS_INFO *prBssInfo;
	uint8_t i;

	if (!prAdapter)
		return NULL;

	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

		if (prBssInfo &&
			IS_BSS_AIS(prBssInfo) &&
			kalGetMediaStateIndicated(
			prAdapter->prGlueInfo,
			prBssInfo->ucBssIndex) ==
			MEDIA_STATE_CONNECTED) {
			return prBssInfo;
		}
	}

	return NULL;
}

struct AIS_FSM_INFO *aisGetDefaultAisInfo(struct ADAPTER *prAdapter)
{
	return prAdapter->rWifiVar.prDefaultAisFsmInfo;
}

struct AIS_LINK_INFO *aisGetDefaultLink(struct ADAPTER *prAdapter)
{
	return  &aisGetDefaultAisInfo(prAdapter)
			->aprLinkInfo[AIS_MAIN_LINK_INDEX];
}

struct BSS_INFO *aisGetDefaultLinkBssInfo(struct ADAPTER *prAdapter)
{
	return	aisGetDefaultLink(prAdapter)->prBssInfo;
}

uint8_t aisGetDefaultLinkBssIndex(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo = aisGetDefaultLinkBssInfo(prAdapter);
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	return	prBssInfo ? prBssInfo->ucBssIndex :
		prWifiVar->ucBssIdStartValue;
}

uint8_t aisGetLinkIndex(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	if (IS_BSS_INDEX_VALID(ucBssIndex) &&
			ais->arBssId2LinkMap[ucBssIndex] != MLD_LINK_ID_NONE)
		return ais->arBssId2LinkMap[ucBssIndex];

	DBGLOG(AIS, WARN,
		"Use default, invalid index=%d caller=%pS\n",
		ucBssIndex, KAL_TRACE);
	return AIS_MAIN_LINK_INDEX;
}

struct STA_RECORD *aisGetDefaultStaRecOfAP(struct ADAPTER *prAdapter)
{
	return	aisGetDefaultLinkBssInfo(prAdapter)->prStaRecOfAP;
}

void aisSetLinkBssInfo(struct AIS_FSM_INFO *prAisFsmInfo,
	struct BSS_INFO *prBssInfo, uint8_t ucLinkIdx)
{
	struct BSS_INFO *ori = prAisFsmInfo->aprLinkInfo[ucLinkIdx].prBssInfo;

	if (ucLinkIdx >= MLD_LINK_MAX)
		return;

	if (ori) {
		prAisFsmInfo->u4BssIdxBmap &= ~BIT(ori->ucBssIndex);
		prAisFsmInfo->arBssId2LinkMap[ori->ucBssIndex] =
			MLD_LINK_ID_NONE;
	}

	prAisFsmInfo->aprLinkInfo[ucLinkIdx].prBssInfo = prBssInfo;

	if (prBssInfo) {
		prAisFsmInfo->u4BssIdxBmap |= BIT(prBssInfo->ucBssIndex);
		prAisFsmInfo->arBssId2LinkMap[prBssInfo->ucBssIndex] =
			ucLinkIdx;
	}
}

struct BSS_INFO *aisGetLinkBssInfo(struct AIS_FSM_INFO *prAisFsmInfo,
	uint8_t ucLinkIdx)
{
	if (ucLinkIdx >= MLD_LINK_MAX || !prAisFsmInfo)
		return NULL;

	return prAisFsmInfo->aprLinkInfo[ucLinkIdx].prBssInfo;
}

uint32_t aisGetBssIndexBmap(struct AIS_FSM_INFO *prAisFsmInfo)
{
	return prAisFsmInfo->u4BssIdxBmap;
}

struct BSS_INFO *aisGetMainLinkBssInfo(struct AIS_FSM_INFO *prAisFsmInfo)
{
	return aisGetLinkBssInfo(prAisFsmInfo, AIS_MAIN_LINK_INDEX);
}

uint8_t aisGetMainLinkBssIndex(struct ADAPTER *prAdapter,
		struct AIS_FSM_INFO *prAisFsmInfo)
{
	struct BSS_INFO *bss = aisGetMainLinkBssInfo(prAisFsmInfo);

	if (bss)
		return bss->ucBssIndex;

	DBGLOG(AIS, WARN,
		"Use default, ais=%p return NULL bss, caller=%pS\n",
		prAisFsmInfo, KAL_TRACE);

	return aisGetDefaultLinkBssIndex(prAdapter);
}

void aisSetLinkBssDesc(struct AIS_FSM_INFO *prAisFsmInfo,
	struct BSS_DESC *prBssDesc, uint8_t ucLinkIdx)
{
	if (ucLinkIdx >= MLD_LINK_MAX)
		return;
	prAisFsmInfo->aprLinkInfo[ucLinkIdx].prTargetBssDesc = prBssDesc;
}

struct BSS_DESC *aisGetLinkBssDesc(struct AIS_FSM_INFO *prAisFsmInfo,
	uint8_t ucLinkIdx)
{
	if (ucLinkIdx >= MLD_LINK_MAX)
		return NULL;

	return prAisFsmInfo->aprLinkInfo[ucLinkIdx].prTargetBssDesc;
}

uint8_t aisGetLinkNum(struct AIS_FSM_INFO *prAisFsmInfo)
{
	uint8_t i, num = 0;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		if (aisGetLinkBssDesc(prAisFsmInfo, i))
			num++;
	}

	return num;
}

struct BSS_DESC *aisGetMainLinkBssDesc(struct AIS_FSM_INFO *prAisFsmInfo)
{
	return aisGetLinkBssDesc(prAisFsmInfo, AIS_MAIN_LINK_INDEX);
}

void aisSetLinkStaRec(struct AIS_FSM_INFO *prAisFsmInfo,
	struct STA_RECORD *prStaRec, uint8_t ucLinkIdx)
{
	if (ucLinkIdx >= MLD_LINK_MAX)
		return;
	prAisFsmInfo->aprLinkInfo[ucLinkIdx].prTargetStaRec = prStaRec;
}

struct STA_RECORD *aisGetLinkStaRec(struct AIS_FSM_INFO *prAisFsmInfo,
	uint8_t ucLinkIdx)
{
	if (ucLinkIdx >= MLD_LINK_MAX)
		return NULL;

	return prAisFsmInfo->aprLinkInfo[ucLinkIdx].prTargetStaRec;
}

struct STA_RECORD *aisGetMainLinkStaRec(struct AIS_FSM_INFO *prAisFsmInfo)
{
	return aisGetLinkStaRec(prAisFsmInfo, AIS_MAIN_LINK_INDEX);
}

void aisClearAllLink(struct AIS_FSM_INFO *prAisFsmInfo)
{
	uint8_t i;

	DBGLOG(AIS, INFO, "Clear BssDesc and StaRec\n");

	for (i = 0; i < MLD_LINK_MAX; i++) {
		prAisFsmInfo->aprLinkInfo[i].prTargetBssDesc = NULL;
		prAisFsmInfo->aprLinkInfo[i].prTargetStaRec = NULL;
	}
}

void aisDeactivateAllLink(struct ADAPTER *prAdapter,
			struct AIS_FSM_INFO *prAisFsmInfo)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *bss = prAisFsmInfo->aprLinkInfo[i].prBssInfo;

		if (bss && IS_NET_ACTIVE(prAdapter, bss->ucBssIndex))
			nicDeactivateNetwork(prAdapter,
				NETWORK_ID(bss->ucBssIndex, i));
	}
}

struct AIS_LINK_INFO *aisGetLink(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	if (IS_BSS_INDEX_VALID(ucBssIndex) &&
			ais->arBssId2LinkMap[ucBssIndex] != MLD_LINK_ID_NONE)
		return &ais->aprLinkInfo[ais->arBssId2LinkMap[ucBssIndex]];

	DBGLOG(AIS, WARN,
		"Use default, invalid index=%d caller=%pS\n",
		ucBssIndex, KAL_TRACE);
	return aisGetDefaultLink(prAdapter);
}

struct BSS_INFO *aisGetAisBssInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return aisGetLink(prAdapter, ucBssIndex)->prBssInfo;
}

struct STA_RECORD *aisGetStaRecOfAP(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return aisGetAisBssInfo(prAdapter, ucBssIndex)->prStaRecOfAP;
}


struct BSS_DESC *aisGetTargetBssDesc(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return aisGetLink(prAdapter, ucBssIndex)->prTargetBssDesc;
}

struct STA_RECORD *aisGetTargetStaRec(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return aisGetLink(prAdapter, ucBssIndex)->prTargetStaRec;
}

void aisTargetBssSetConnected(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *ais)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		if (ais->aprLinkInfo[i].prTargetBssDesc) {
			if (!ais->aprLinkInfo[i].prBssInfo) {
				DBGLOG(AIS, WARN, "link %d missing bssinfo\n",
					i);
				continue;
			}

			ais->aprLinkInfo[i].prTargetBssDesc->fgIsConnected |=
				BIT(ais->aprLinkInfo[i].prBssInfo->ucBssIndex);
		}
	}
}

void aisTargetBssSetConnecting(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *ais)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		if (ais->aprLinkInfo[i].prTargetBssDesc) {
			if (!ais->aprLinkInfo[i].prBssInfo) {
				DBGLOG(AIS, WARN, "link %d missing bssinfo\n",
					i);
				continue;
			}

			ais->aprLinkInfo[i].prTargetBssDesc->fgIsConnecting |=
				BIT(ais->aprLinkInfo[i].prBssInfo->ucBssIndex);
		}
	}
}

void aisTargetBssResetConnected(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *ais)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		if (ais->aprLinkInfo[i].prTargetBssDesc) {
			if (!ais->aprLinkInfo[i].prBssInfo) {
				DBGLOG(AIS, WARN, "link %d missing bssinfo\n",
					i);
				continue;
			}

			ais->aprLinkInfo[i].prTargetBssDesc->fgIsConnected &=
			       ~BIT(ais->aprLinkInfo[i].prBssInfo->ucBssIndex);
		}
	}
}

void aisTargetBssResetConnecting(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *ais)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		if (ais->aprLinkInfo[i].prTargetBssDesc) {
			if (!ais->aprLinkInfo[i].prBssInfo) {
				DBGLOG(AIS, WARN, "link %d missing bssinfo\n",
					i);
				continue;
			}

			ais->aprLinkInfo[i].prTargetBssDesc->fgIsConnecting &=
			       ~BIT(ais->aprLinkInfo[i].prBssInfo->ucBssIndex);
		}
	}
}

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
struct TIMER *aisGetSecModeChangeTimer(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rSecModeChangeTimer;
}
#endif

struct TIMER *aisGetBTMDisassocTimer(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisSpecBssInfo(prAdapter, ucBssIndex)->rBTMDisassocTimer;
}

struct TIMER *aisGetScanDoneTimer(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rScanDoneTimer;
}

enum ENUM_AIS_STATE aisGetCurrState(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return aisGetAisFsmInfo(prAdapter, ucBssIndex)->eCurrentState;
}

struct CONNECTION_SETTINGS *aisGetConnSettings(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rConnSettings;
}

struct GL_WPA_INFO *aisGetWpaInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rWpaInfo;
}

u_int8_t aisGetWapiMode(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return aisGetConnSettings(prAdapter, ucBssIndex)->fgWapiMode;
}

enum ENUM_PARAM_AUTH_MODE aisGetAuthMode(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return aisGetConnSettings(prAdapter, ucBssIndex)->eAuthMode;
}

enum ENUM_PARAM_OP_MODE aisGetOPMode(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return aisGetConnSettings(prAdapter, ucBssIndex)->eOPMode;
}

enum ENUM_WEP_STATUS aisGetEncStatus(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return aisGetConnSettings(prAdapter, ucBssIndex)->eEncStatus;
}

struct IEEE_802_11_MIB *aisGetMib(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rMib;
}

#if CFG_SUPPORT_ROAMING
struct ROAMING_INFO *aisGetRoamingInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rRoamingInfo;
}
#endif

struct APS_INFO *aisGetApsInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rApsInfo;
}

struct PARAM_BSSID_EX *aisGetCurrBssId(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rCurrBssId;
}

uint8_t *aisGetCurrentApAddr(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return aisGetAisSpecBssInfo(prAdapter, ucBssIndex)->aucCurrentApAddr;
}

#if CFG_SUPPORT_PASSPOINT
struct HS20_INFO *aisGetHS20Info(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rHS20Info;
}
#endif

struct RADIO_MEASUREMENT_REQ_PARAMS *aisGetRmReqParam(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rRmReqParams;
}

struct RADIO_MEASUREMENT_REPORT_PARAMS *
	aisGetRmReportParam(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rRmRepParams;
}

struct WMM_INFO *aisGetWMMInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	if (ucBssIndex == 255)
		DBGLOG(AIS, WARN,
			"######## invalid index=%d caller=%pS\n",
			ucBssIndex, KAL_TRACE);
	return &aisGetAisFsmInfo(prAdapter, ucBssIndex)->rWmmInfo;
}

#ifdef CFG_SUPPORT_REPLAY_DETECTION
struct GL_DETECT_REPLAY_INFO *aisGetDetRplyInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex) {

	return &aisGetAisBssInfo(prAdapter, ucBssIndex)->rDetRplyInfo;
}
#endif

struct FT_IES *aisGetFtIe(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucR0R1)
{
	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex))
		return NULL;

	if (ucR0R1 == AIS_FT_R0)
		return &aisGetConnSettings(prAdapter, ucBssIndex)->rFtIeR0;
	else if (ucR0R1 == AIS_FT_R1)
		return &aisGetConnSettings(prAdapter, ucBssIndex)->rFtIeR1;

	return NULL;
}

struct FT_EVENT_PARAMS *aisGetFtEventParam(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	return &aisGetConnSettings(prAdapter, ucBssIndex)->rFtEventParam;
}

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
struct EAP_ERP_KEY *aisGetErpKey(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct EAP_ERP_KEY *erp;

	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex))
		return NULL;

	erp = &aisGetConnSettings(prAdapter, ucBssIndex)->rErpKey;

	return erp->fgValid ? erp : NULL;
}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

const char *aisGetFsmState(enum ENUM_AIS_STATE eCurrentState)
{
	uint32_t u4State = eCurrentState;

	if (u4State < AIS_STATE_NUM)
		return apucDebugAisState[u4State];

	ASSERT(0);
	return (uint8_t *) NULL;
}

u_int8_t addAxBlocklist(struct ADAPTER *prAdapter,
			     uint8_t aucBSSID[], uint8_t ucBssIndex,
			     uint8_t ucType)
{
	struct AX_BLOCKLIST_ITEM *prBlocklistItem;
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	prBlocklistItem =
	    (struct AX_BLOCKLIST_ITEM *)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
					      sizeof(struct AX_BLOCKLIST_ITEM));

	if (!prBlocklistItem) {
		DBGLOG(AIS, ERROR, "Can't generate new message\n");
		return FALSE;
	}

	COPY_MAC_ADDR(prBlocklistItem->aucBSSID, aucBSSID);
	if (ucType == BLOCKLIST_AX_TO_AC) {
		LINK_INSERT_TAIL(&prAisFsmInfo->rAxBlocklist,
			&prBlocklistItem->rLinkEntry);
	} else if (ucType == BLOCKLIST_DIS_HE_HTC) {
		LINK_INSERT_TAIL(&prAisFsmInfo->rHeHtcBlocklist,
			&prBlocklistItem->rLinkEntry);
	} else {
		DBGLOG(AIS, ERROR, "Wrong type %d\n", ucType);
		return FALSE;
	}
	DBGLOG(AIS, INFO, "Add BSSID " MACSTR " into %s blocklist\n",
			MAC2STR(aucBSSID),
			ucType == 0 ? "AX" : "+HTC");

	return TRUE;
}

u_int8_t queryAxBlocklist(struct ADAPTER *prAdapter,
			     uint8_t aucBSSID[], uint8_t ucBssIndex,
			     uint8_t ucType)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct LINK *prBlocklist;
	struct AX_BLOCKLIST_ITEM *prBlocklistItem, *prBlocklistItemNext;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	if (ucType == BLOCKLIST_AX_TO_AC) {
		prBlocklist = &prAisFsmInfo->rAxBlocklist;
	} else if (ucType == BLOCKLIST_DIS_HE_HTC) {
		prBlocklist = &prAisFsmInfo->rHeHtcBlocklist;
	} else {
		DBGLOG(AIS, ERROR, "Wrong type %d\n", ucType);
		return FALSE;
	}

	/* traverse through blocklist */
	LINK_FOR_EACH_ENTRY_SAFE(prBlocklistItem,
				 prBlocklistItemNext,
				 prBlocklist, rLinkEntry,
				 struct AX_BLOCKLIST_ITEM) {
		if (EQUAL_MAC_ADDR(aucBSSID, prBlocklistItem->aucBSSID))
			return TRUE;
	}
		DBGLOG(AIS, INFO,
			"BSSID " MACSTR " is not in %s blocklist!\n",
			MAC2STR(aucBSSID),
			ucType == 0 ? "AX" : "+HTC");
	return FALSE;
}

u_int8_t clearAxBlocklist(struct ADAPTER *prAdapter,
			     uint8_t ucBssIndex,
			     uint8_t ucType)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct LINK *prBlocklist;
	struct AX_BLOCKLIST_ITEM *prBlocklistItem, *prBlocklistItemNext;

	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex))
		return FALSE;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	if (ucType == BLOCKLIST_AX_TO_AC) {
		prBlocklist = &prAisFsmInfo->rAxBlocklist;
	} else if (ucType == BLOCKLIST_DIS_HE_HTC) {
		prBlocklist = &prAisFsmInfo->rHeHtcBlocklist;
	} else {
		DBGLOG(AIS, ERROR, "Wrong type %d\n", ucType);
		return FALSE;
	}

	/* traverse through blocklist */
	LINK_FOR_EACH_ENTRY_SAFE(prBlocklistItem,
				 prBlocklistItemNext,
				 prBlocklist, rLinkEntry,
				 struct AX_BLOCKLIST_ITEM) {
		DBGLOG(AIS, INFO,
			"BSSID " MACSTR " is removed from %s blocklist!\n",
			MAC2STR(prBlocklistItem->aucBSSID),
			ucType == 0 ? "AX" : "+HTC");
		LINK_REMOVE_KNOWN_ENTRY(prBlocklist,
					&prBlocklistItem->rLinkEntry);
		cnmMemFree(prAdapter, prBlocklistItem);
	}
	return TRUE;
}

u_int8_t aisAddCusBlocklist(struct ADAPTER *prAdapter,
	struct PARAM_CUS_BLOCKLIST *prCusBlocklist, uint8_t ucBssIndex)
{
	struct CUS_BLOCKLIST_ITEM *prCusBlkItem;
	struct AIS_FSM_INFO *prAisFsmInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	prCusBlkItem =
	    (struct CUS_BLOCKLIST_ITEM *)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			sizeof(struct CUS_BLOCKLIST_ITEM));

	if (!prCusBlkItem) {
		DBGLOG(AIS, ERROR,
			"Can't alloc new customized blocklist item.\n");
		return FALSE;
	}

	prCusBlkItem->ucType = prCusBlocklist->ucType;

	COPY_SSID(prCusBlkItem->rSSID.aucSsid,
		  prCusBlkItem->rSSID.u4SsidLen,
		  prCusBlocklist->rSSID.aucSsid,
		  prCusBlocklist->rSSID.u4SsidLen);
	COPY_MAC_ADDR(prCusBlkItem->aucBSSID, prCusBlocklist->aucBSSID);
	prCusBlkItem->u4Frequency = prCusBlocklist->u4Frequency;
	if (prCusBlocklist->eBand > BAND_NULL &&
	    prCusBlocklist->eBand < BAND_NUM)
		prCusBlkItem->eBand = prCusBlocklist->eBand;
	else
		prCusBlkItem->eBand = BAND_NULL;

	prCusBlkItem->ucLimitReason = prCusBlocklist->ucLimitReason;
	prCusBlkItem->ucLimitType = prCusBlocklist->ucLimitType;
	prCusBlkItem->u4LimitTimeout = prCusBlocklist->u4LimitTimeout;

	GET_CURRENT_SYSTIME(&prCusBlkItem->rAddTime);

	LINK_INSERT_TAIL(&prAisFsmInfo->rCusBlocklist,
				&prCusBlkItem->rLinkEntry);

	DBGLOG(AIS, INFO, "Add [%s "MACSTR" %d %s] to customized blocklist!\n",
		prCusBlkItem->rSSID.aucSsid,
		MAC2STR(prCusBlkItem->aucBSSID),
		prCusBlkItem->u4Frequency,
		apucBandStr[prCusBlkItem->eBand]);

	return TRUE;
}

u_int8_t aisQueryCusBlocklist(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct BSS_DESC *prBssDesc)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct LINK *prBlocklist;
	struct CUS_BLOCKLIST_ITEM *prBlocklistItem = NULL;

	if (!prBssDesc)
		return FALSE;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prBlocklist = &prAisFsmInfo->rCusBlocklist;

	/* traverse through customized blocklist */
	LINK_FOR_EACH_ENTRY(prBlocklistItem, prBlocklist, rLinkEntry,
			    struct CUS_BLOCKLIST_ITEM) {

		if (prBlocklistItem->ucType == 0)
			continue;

		if (prBlocklistItem->ucType & BIT(CUS_BLOCKLIST_TYPE_SSID) &&
		    UNEQUAL_SSID(prBlocklistItem->rSSID.aucSsid,
				 prBlocklistItem->rSSID.u4SsidLen,
				 prBssDesc->aucSSID,
				 prBssDesc->ucSSIDLen))
			continue;

		if (prBlocklistItem->ucType & BIT(CUS_BLOCKLIST_TYPE_BSSID) &&
		    UNEQUAL_MAC_ADDR(prBlocklistItem->aucBSSID,
				     prBssDesc->aucBSSID))
			continue;

		if (prBlocklistItem->ucType &
				BIT(CUS_BLOCKLIST_TYPE_FREQUENCY) &&
		    nicFreq2ChannelNum(prBlocklistItem->u4Frequency * 1000) !=
					prBssDesc->ucChannelNum)
			continue;

		if (prBlocklistItem->ucType & BIT(CUS_BLOCKLIST_TYPE_BAND) &&
		    prBlocklistItem->eBand != prBssDesc->eBand)
			continue;

		/* If it is the 1st connection scenario */
		if (
#if CFG_SUPPORT_ROAMING
		    !roamingFsmCheckIfRoaming(prAdapter, ucBssIndex) &&
#endif
		    !(prBlocklistItem->ucLimitType &
						BIT(LIMIT_FIRST_CONNECTION)))
			continue;

#if CFG_SUPPORT_ROAMING
		/* If it is the roaming scenario */
		if (roamingFsmCheckIfRoaming(prAdapter, ucBssIndex) &&
		    !(prBlocklistItem->ucLimitType & BIT(LIMIT_ROAMING)))
			continue;
#endif

		DBGLOG(AIS, INFO, MACSTR
			" match [%s "
			MACSTR" %d %s] from customized blocklist!\n",
			MAC2STR(prBssDesc->aucBSSID),
			prBlocklistItem->rSSID.aucSsid,
			MAC2STR(prBlocklistItem->aucBSSID),
			prBlocklistItem->u4Frequency,
			apucBandStr[prBlocklistItem->eBand]);

		return TRUE;
	}

	return FALSE;
}

u_int8_t aisClearCusBlocklist(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, u_int8_t fgRemoveAll)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct LINK *prBlocklist;
	struct CUS_BLOCKLIST_ITEM *prBlocklistItem = NULL,
				  *prBlocklistItemNext = NULL;
	OS_SYSTIME rCurrent;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prBlocklist = &prAisFsmInfo->rCusBlocklist;

	GET_CURRENT_SYSTIME(&rCurrent);

	/* traverse through blocklist */
	LINK_FOR_EACH_ENTRY_SAFE(prBlocklistItem,
				 prBlocklistItemNext,
				 prBlocklist, rLinkEntry,
				 struct CUS_BLOCKLIST_ITEM) {
		if (fgRemoveAll ||
		    CHECK_FOR_TIMEOUT(rCurrent, prBlocklistItem->rAddTime,
				SEC_TO_MSEC(prBlocklistItem->u4LimitTimeout))) {
			DBGLOG(AIS, INFO, "Remove [%s "
				MACSTR" %d %s] from customized blocklist!\n",
				prBlocklistItem->rSSID.aucSsid,
				MAC2STR(prBlocklistItem->aucBSSID),
				prBlocklistItem->u4Frequency,
				apucBandStr[prBlocklistItem->eBand]);

			LINK_REMOVE_KNOWN_ENTRY(prBlocklist,
						&prBlocklistItem->rLinkEntry);
			cnmMemFree(prAdapter, prBlocklistItem);
		}
	}

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Trigger when cfg80211_suspend
 *                1. cancel scan and report scan done event
 *                2. linkdown if wow is disable (not yet)
 *
 * @param prAdapter
 *        eReqType
 *        bRemove
 *
 * @return TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
void aisPreSuspendFlow(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	struct SCAN_INFO *prScanInfo;
#if CFG_WOW_SUPPORT
	struct BSS_INFO *prAisBssInfo = NULL;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_CANCEL_REMAIN_ON_CHANNEL *prMsgChnlAbort;
#endif

	if (prAdapter == NULL)
		return;

	prGlueInfo = prAdapter->prGlueInfo;
	prWifiVar = &prAdapter->rWifiVar;

	if (prGlueInfo == NULL)
		return;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	/* cancel scan */
	aisFsmStateAbort_SCAN(prAdapter,
		prScanInfo->rScanParam.ucBssIndex);

#if CFG_WOW_SUPPORT
	/* 1) wifi cfg "Wow" must be true,
	 * 2) wow is disable
	 * 3) AdvPws is disable
	 * 4) WIfI connected => execute link down flow
	 */
	/* link down AIS */
	prAisBssInfo = aisGetConnectedBssInfo(prAdapter);

	if (!prAisBssInfo)
		return;

	if (IS_FEATURE_ENABLED(prWifiVar->ucWow) &&
		IS_FEATURE_DISABLED(prAdapter->rWowCtrl.fgWowEnable) &&
		IS_FEATURE_DISABLED(prWifiVar->ucAdvPws))
	{
		/* wow off, link down */
		DBGLOG(REQ, STATE, "CFG80211 suspend link down\n");
		aisBssLinkDown(prAdapter, prAisBssInfo->ucBssIndex);
		return;
	}

	/* WOW keep connection case: check AIS state.
	 * must switch backt to working channel for Rx WOW packets in suspend mode.
	 * Off-channel Tx from wpa_suppicant may be sent after resume.
	 */
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter,
		prAisBssInfo->ucBssIndex);

	if ((prAisFsmInfo->eCurrentState == AIS_STATE_REMAIN_ON_CHANNEL) ||
		(prAisFsmInfo->eCurrentState ==
		 AIS_STATE_REQ_REMAIN_ON_CHANNEL)) {
		prMsgChnlAbort =
			cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_CANCEL_REMAIN_ON_CHANNEL));
		if (prMsgChnlAbort == NULL)
			DBGLOG(REQ, ERROR, "ChnlAbort Msg allocate fail!\n");
		else {
			prMsgChnlAbort->rMsgHdr.eMsgId =
				MID_MNY_AIS_CANCEL_REMAIN_ON_CHANNEL;
			prMsgChnlAbort->u8Cookie =
				prAisFsmInfo->rChReqInfo.u8Cookie;
			prMsgChnlAbort->ucBssIdx =
				prAisBssInfo->ucBssIndex;

			mboxSendMsg(prAdapter, MBOX_ID_0,
				(struct MSG_HDR *) prMsgChnlAbort,
				MSG_SEND_METHOD_BUF);
		}
	}
#endif
}

static void aisReqJoinChPrivilege(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	uint8_t *ucChTokenId)
{
	struct MSG_CH_REQ *prMsgChReq = NULL;
	struct MSG_CH_REQ *prSubReq = NULL;
	uint8_t ucReqChNum = 0;
	uint32_t u4MsgSz;
	uint8_t i = 0;
	enum ENUM_CH_REQ_TYPE tmpReqCHType = CH_REQ_TYPE_JOIN;
	enum ENUM_MBMC_BN tmpDBDCBand = ENUM_BAND_ALL;
	struct BSS_INFO *prBss = NULL;
	struct BSS_DESC *prBssDesc = NULL;

	ucReqChNum = aisGetLinkNum(prAisFsmInfo);

	u4MsgSz = sizeof(struct MSG_CH_REQ) * ucReqChNum;
	prMsgChReq = (struct MSG_CH_REQ *)cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG,
		u4MsgSz);
	if (!prMsgChReq) {
		DBGLOG(AIS, ERROR, "Alloc CH req msg failed.\n");
		return;
	}
	kalMemZero(prMsgChReq, u4MsgSz);

	*ucChTokenId = cnmIncreaseTokenId(prAdapter);
	prAisFsmInfo->ucChReqNum = ucReqChNum;
	prAisFsmInfo->fgIsChannelRequested = TRUE;
	prMsgChReq->ucExtraChReqNum = prAisFsmInfo->ucChReqNum - 1;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	tmpReqCHType = mldDecideCnmReqCHType(prAdapter,
			prAisFsmInfo->prMldBssInfo);
#endif /* CFG_SUPPORT_802_11BE_MLO */

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
	if (ucReqChNum >= 2) {
		struct MLD_BSS_INFO *prMldBssInfo = prAisFsmInfo->prMldBssInfo;

		/*need set BAND AUTO in the case of EMLSR/Hybrid*/
		if (prMldBssInfo && prMldBssInfo->ucMaxSimuLinks == 0 &&
			(prMldBssInfo->ucEmlEnabled == TRUE ||
			prMldBssInfo->ucHmloEnabled == TRUE))
			tmpDBDCBand = ENUM_BAND_AUTO;
	}
#endif

	for (i = 0; i < ucReqChNum; i++) {
		prBss = aisGetLinkBssInfo(prAisFsmInfo, i);
		prBssDesc = aisGetLinkBssDesc(prAisFsmInfo, i);

		if (!prBss || !prBssDesc)
			continue;

		/* for secondary link */
		if (!IS_NET_ACTIVE(prAdapter, prBss->ucBssIndex)) {
			/* sync with firmware */
			nicActivateNetwork(prAdapter,
				NETWORK_ID(prBss->ucBssIndex, i));
			SET_NET_PWR_STATE_ACTIVE(prAdapter,
			    prBss->ucBssIndex);
		}

		/* stop Tx due to we need to connect a new AP. even the
		 ** new AP is operating on the same channel with current
		 ** , we still need to stop Tx, because firmware should
		 ** ensure all mgmt and dhcp packets are Tx in time,
		 ** and may cause normal data packets was queued and
		 ** eventually flushed in firmware
		 */
		if (prBss->prStaRecOfAP)
			prBss->prStaRecOfAP->fgIsTxAllowed = FALSE;

		prSubReq = (struct MSG_CH_REQ *)&prMsgChReq[i];

		if (i == 0)
			prAisFsmInfo->ucBssIndexOfChReq = prBss->ucBssIndex;

		prSubReq->ucBssIndex = prBss->ucBssIndex;
#if CFG_SUPPORT_DBDC
		if (ucReqChNum >= 2)
			prSubReq->eDBDCBand = tmpDBDCBand;
		else
			prSubReq->eDBDCBand = ENUM_BAND_AUTO;
#endif
		prSubReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
		prSubReq->ucTokenID = *ucChTokenId;
		prSubReq->eReqType = tmpReqCHType;
		prSubReq->u4MaxInterval = AIS_JOIN_CH_REQUEST_INTERVAL;
		prSubReq->ucPrimaryChannel = prBssDesc->ucChannelNum;
		prSubReq->eRfSco = prBssDesc->eSco;
		prSubReq->eRfBand = prBssDesc->eBand;
		prSubReq->eRfChannelWidth = prBssDesc->eChannelWidth;
		prSubReq->ucRfCenterFreqSeg1 = nicGetS1(prAdapter,
			prSubReq->eRfBand,
			prSubReq->ucPrimaryChannel,
			prSubReq->eRfChannelWidth);
		prSubReq->ucRfCenterFreqSeg2 = 0;

		rlmReviseMaxBw(prAdapter,
			prSubReq->ucBssIndex,
			&prSubReq->eRfSco,
			&prSubReq->eRfChannelWidth,
			&prSubReq->ucRfCenterFreqSeg1,
			&prSubReq->ucPrimaryChannel);
	}

	mboxSendMsg(prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *)prMsgChReq,
		    MSG_SEND_METHOD_UNBUF);
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
static uint32_t aisScanGenMlScanReq(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct BSS_DESC *prBssDesc;
	uint8_t aucIe[MAX_BAND_IE_LENGTH];
	uint32_t u4ScanIELen = 0;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prBssDesc = prAisFsmInfo->prMlProbeBssDesc;

	if (!prBssDesc) {
		DBGLOG(AIS, INFO, "no ml probe target\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	/* Generate ML probe request IE */
	kalMemZero(aucIe, sizeof(aucIe));
	u4ScanIELen = mldFillScanIE(prAdapter, prBssDesc,
		aucIe, sizeof(aucIe), FALSE, prBssDesc->rMlInfo.ucMldId);
	prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;
	prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_WILDCARD;

	/* Not to handle RNR IE in this scan*/
	prScanReqMsg->fgOobRnrParseEn = FALSE;

	/* Assign channel and BSSID */
	prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
	prScanReqMsg->ucChannelListNum = 1;
	prScanReqMsg->arChnlInfoList[0].eBand = prBssDesc->eBand;
	prScanReqMsg->arChnlInfoList[0].ucChannelNum = prBssDesc->ucChannelNum;
	prScanReqMsg->ucBssidMatchCh[0] = prBssDesc->ucChannelNum;
	COPY_MAC_ADDR(prScanReqMsg->aucExtBssid[0], prBssDesc->aucBSSID);

	/* No BssidMatchSsid, set to default value */
	kalMemSet(prScanReqMsg->ucBssidMatchSsidInd, CFG_SCAN_OOB_MAX_NUM,
				sizeof(prScanReqMsg->ucBssidMatchSsidInd));

	/* MaskExtend set to ENUM_SCN_ML_PROBE */
	prScanReqMsg->ucScnFuncMask |= ENUM_SCN_USE_PADDING_AS_BSSID;
	prScanReqMsg->u4ScnFuncMaskExtend |= ENUM_SCN_ML_PROBE;

	/* Copy ML probe request IE */
	kalMemZero(prScanReqMsg->aucIEMl, MAX_BAND_IE_LENGTH);
	if (u4ScanIELen > 0) {
		kalMemCopy(prScanReqMsg->aucIEMl, aucIe, u4ScanIELen);
		prScanReqMsg->u2IELenMl = (uint16_t)u4ScanIELen;
	}

	return WLAN_STATUS_SUCCESS;
}

void aisScanAddRlmIEbyBand(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo, enum ENUM_BAND eBand,
	struct MSG_SCN_SCAN_REQ_V2 *prCmdScanReq)
{
	uint8_t ucBssIndex = prBssInfo->ucBssIndex;
	enum ENUM_BAND eOldBand = prBssInfo->eBand;
	struct MSDU_INFO *msdu = NULL;
	uint32_t len = 0;

	/* change eBand to generate rlm ie */
	prBssInfo->eBand = eBand;

	len = heRlmCalculateHeCapIELen(prAdapter, ucBssIndex, NULL);
	len += ehtRlmCalculateCapIELen(prAdapter, ucBssIndex, NULL);
	if (len > 100)
		goto done;

	msdu = cnmMgtPktAlloc(prAdapter, len);
	if (msdu == NULL)
		goto done;

	msdu->ucBssIndex = ucBssIndex;
	heRlmFillHeCapIE(prAdapter, prBssInfo, msdu);
	ehtRlmFillCapIE(prAdapter, prBssInfo, msdu);

	switch (eBand) {
	case BAND_2G4:
		kalMemCopy(prCmdScanReq->aucIE2G4,
			(uint8_t *) msdu->prPacket, msdu->u2FrameLength);
		prCmdScanReq->u2IELen2G4 = msdu->u2FrameLength;
		break;
	case BAND_5G:
		kalMemCopy(prCmdScanReq->aucIE5G,
			(uint8_t *) msdu->prPacket, msdu->u2FrameLength);
		prCmdScanReq->u2IELen5G = msdu->u2FrameLength;
		break;
#if (CFG_SUPPORT_WIFI_6G == 1)
	case BAND_6G:
		kalMemCopy(prCmdScanReq->aucIE6G,
			(uint8_t *) msdu->prPacket, msdu->u2FrameLength);
		prCmdScanReq->u2IELen6G = msdu->u2FrameLength;
		break;
#endif
	default:
		break;
	}

done:
	prBssInfo->eBand = eOldBand;
	cnmMgtPktFree(prAdapter, msdu);
}

void aisScanAddRlmIE(struct ADAPTER *prAdapter,
	struct MSG_SCN_SCAN_REQ_V2 *prCmdScanReq)
{
	struct BSS_INFO *bss;

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucStaEht))
		return;

	bss = GET_BSS_INFO_BY_INDEX(prAdapter, prCmdScanReq->ucBssIndex);
	if (!bss) {
		DBGLOG(SCN, WARN, "no bssinfo %d\n", prCmdScanReq->ucBssIndex);
		return;
	}

	prCmdScanReq->u2IELen += rlmGenerateMTKChipCapIE(
		prCmdScanReq->aucIE + prCmdScanReq->u2IELen,
		MAX_IE_LENGTH - prCmdScanReq->u2IELen, TRUE,
		MTK_OUI_CHIP_CAP);

	if (prAdapter->rWifiVar.u4SwTestMode ==	ENUM_SW_TEST_MODE_SIGMA_BE) {
		aisScanAddRlmIEbyBand(prAdapter, bss, BAND_2G4, prCmdScanReq);
		aisScanAddRlmIEbyBand(prAdapter, bss, BAND_5G, prCmdScanReq);
#if (CFG_SUPPORT_WIFI_6G == 1)
		aisScanAddRlmIEbyBand(prAdapter, bss, BAND_6G, prCmdScanReq);
#endif
	}
}
#endif

static void aisScanReqInit(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	if (!prConnSettings || !prAisBssInfo || !prAisFsmInfo) {
		DBGLOG(AIS, ERROR,
			"ERR! Null access! ConnSettings=%p, BssInfo=%p, FsmInfo=%p\n",
			prConnSettings, prAisBssInfo, prAisFsmInfo);
		return;
	}

	kalMemZero(prScanReqMsg, sizeof(*prScanReqMsg));
	prScanReqMsg->rMsgHdr.eMsgId = MID_AIS_SCN_SCAN_REQ_V2;
	prScanReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfScanReq;
	prScanReqMsg->ucBssIndex = prAisBssInfo->ucBssIndex;
	COPY_MAC_ADDR(prScanReqMsg->aucBSSID, "\xff\xff\xff\xff\xff\xff");
}

static void aisScanProcessReqParam(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg,
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct RADIO_MEASUREMENT_REQ_PARAMS *prRmReq;
#if CFG_SUPPORT_ROAMING
	struct ROAMING_INFO *prRoamingFsmInfo;

	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);
#endif
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prRmReq = aisGetRmReqParam(prAdapter, ucBssIndex);

	/* record last scan start time for rrm */
	if (prScanRequest->fgIsRrm) {
		GET_CURRENT_SYSTIME(&prRmReq->rScanStartTime);
		prRmReq->rBcnRmParam.eState = RM_ON_GOING;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prScanReqMsg->fgNeedMloScan = prScanRequest->fgNeedMloScan;
#endif

	if (!kalIsZeroEtherAddr(prScanRequest->aucBSSID))
		COPY_MAC_ADDR(prScanReqMsg->aucBSSID, prScanRequest->aucBSSID);

#if CFG_SUPPORT_RDD_TEST_MODE
	prScanReqMsg->eScanType = SCAN_TYPE_PASSIVE_SCAN;
#else
	if (prAisFsmInfo->eCurrentState == AIS_STATE_SCAN
		|| prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN) {
		uint8_t ucScanSSIDNum;
		enum ENUM_SCAN_TYPE eScanType;

		ucScanSSIDNum = prScanRequest->u4SsidNum +
			prScanRequest->ucShortSsidNum;
		eScanType = prScanRequest->ucScanType;

		if (eScanType == SCAN_TYPE_ACTIVE_SCAN && ucScanSSIDNum == 0) {
			prScanReqMsg->eScanType = eScanType;
			prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_WILDCARD;
			prScanReqMsg->ucSSIDNum = 0;
		} else if (eScanType == SCAN_TYPE_PASSIVE_SCAN
				&& ucScanSSIDNum == 0) {
			prScanReqMsg->eScanType = eScanType;
			prScanReqMsg->ucSSIDType = 0;
			prScanReqMsg->ucSSIDNum = 0;
		} else {
			prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;
			if (prScanRequest->ucSSIDType)
				prScanReqMsg->ucSSIDType =
					prScanRequest->ucSSIDType;
			else
				prScanReqMsg->ucSSIDType =
					SCAN_REQ_SSID_SPECIFIED;
			prScanReqMsg->ucShortSSIDNum =
						prScanRequest->ucShortSsidNum;
			prScanReqMsg->ucSSIDNum = prScanRequest->u4SsidNum;
			kalMemCopy(&prScanReqMsg->arSsid, &prScanRequest->rSsid,
				sizeof(prScanRequest->rSsid));
		}
		kalMemCopy(prScanReqMsg->aucExtBssid,
			prScanRequest->aucExtBssid,
			CFG_SCAN_OOB_MAX_NUM * MAC_ADDR_LEN);
		kalMemCopy(prScanReqMsg->aucRandomMac,
			   prScanRequest->aucRandomMac,
			   MAC_ADDR_LEN);
		prScanReqMsg->ucScnFuncMask |= prScanRequest->ucScnFuncMask;
		prScanReqMsg->u4ScnFuncMaskExtend |=
					prScanRequest->u4ScnFuncMaskExtend;
	} else {
		prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

		/* Scan for determined SSID */
		prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_SPECIFIED_ONLY;
		prScanReqMsg->ucSSIDNum = 1;
		COPY_SSID(prScanReqMsg->arSsid[0].aucSsid,
			  prScanReqMsg->arSsid[0].u4SsidLen,
			  prConnSettings->aucSSID,
			  prConnSettings->ucSSIDLen);
#if CFG_SUPPORT_SCAN_RANDOM_MAC
		prScanReqMsg->ucScnFuncMask |= ENUM_SCN_RANDOM_MAC_EN;
#endif
	}
#endif

	/* using default channel dwell time/timeout value */
	prScanReqMsg->u2ProbeDelay = 0;
	prScanReqMsg->u2ChannelDwellTime =
		prScanRequest->u2ChannelDwellTime;
	prScanReqMsg->u2ChannelMinDwellTime =
		prScanRequest->u2ChannelMinDwellTime;
	prScanReqMsg->u2TimeoutValue = 0;

#if CFG_SUPPORT_ROAMING
	/* TODO: Add more variable parameters for different scan modes */
	if (prRoamingFsmInfo->rRoamScanParam.ucScanMode ==
			ROAMING_SCAN_MODE_LOW_LATENCY) {
		prScanReqMsg->u2ChannelDwellTime =
			ROAMING_SCAN_NON_DFS_CH_DWELL_TIME;
		prScanReqMsg->u2ChannelMinDwellTime =
			(prScanReqMsg->u2ChannelDwellTime <
			SCAN_CHANNEL_DWELL_TIME_MIN_MSEC) ?
			prScanReqMsg->u2ChannelDwellTime :
			SCAN_CHANNEL_DWELL_TIME_MIN_MSEC;
	}
#endif

#if CFG_SUPPORT_LLW_SCAN
	/* using customized scan parameters */
	prScanReqMsg->u2ChannelDwellTime =
		prAisFsmInfo->ucNonDfsChDwellTimeMs;
	prScanReqMsg->u2ChannelMinDwellTime =
		(prScanReqMsg->u2ChannelDwellTime <
		SCAN_CHANNEL_DWELL_TIME_MIN_MSEC) ?
		prScanReqMsg->u2ChannelDwellTime :
		SCAN_CHANNEL_DWELL_TIME_MIN_MSEC;

	/* using customized scan parameters */
	prScanReqMsg->u2OpChStayTime =
		prAisFsmInfo->u2OpChStayTimeMs;
	prScanReqMsg->ucDfsChDwellTime =
		prAisFsmInfo->ucDfsChDwellTimeMs;
	prScanReqMsg->ucPerScanChCnt =
		prAisFsmInfo->ucPerScanChannelCnt;
#endif

	/* for 6G OOB scan */
	kalMemCopy(prScanReqMsg->ucBssidMatchCh,
		prScanRequest->ucBssidMatchCh,
		CFG_SCAN_OOB_MAX_NUM);
	kalMemCopy(prScanReqMsg->ucBssidMatchSsidInd,
		prScanRequest->ucBssidMatchSsidInd,
		CFG_SCAN_OOB_MAX_NUM);

	prScanReqMsg->fgOobRnrParseEn = prScanRequest->fgOobRnrParseEn;

	aisScanProcessReqCh(prAdapter, ucBssIndex, prScanReqMsg, prScanRequest);
	aisScanProcessReqExtra(prAdapter, prScanReqMsg, prScanRequest);

	if (prScanRequest->u4IELength > 0) {
		kalMemCopy(prScanReqMsg->aucIE,
			   prScanRequest->aucIEBuf, prScanRequest->u4IELength);
		prScanReqMsg->u2IELen = prScanRequest->u4IELength;
	} else {
#if CFG_SUPPORT_WPS2
		if (prConnSettings->u2WSCIELen > 0) {
			kalMemCopy(prScanReqMsg->aucIE,
				   prConnSettings->aucWSCIE,
				   prConnSettings->u2WSCIELen);
			prScanReqMsg->u2IELen = prConnSettings->u2WSCIELen;

		}
#endif
	}
}

static void aisScanProcessReqCh(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg,
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest)
{
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_BAND eBand = BAND_2G4;
	uint8_t ucChannel = 1;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisSpecificBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* check if tethering is running and need to fix on
	 * specific channel
	 */
	if (aisScanChannelFixed(prAdapter, &eBand, &ucChannel, ucBssIndex)) {
		prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
		prScanReqMsg->ucChannelListNum = 1;
		prScanReqMsg->arChnlInfoList[0].eBand = eBand;
		prScanReqMsg->arChnlInfoList[0].ucChannelNum = ucChannel;
	} else if (aisNeedTargetScan(prAdapter, prAisBssInfo->ucBssIndex)) {
		struct RF_CHANNEL_INFO *prChnlInfo =
					&prScanReqMsg->arChnlInfoList[0];
		uint8_t ucChannelNum = 0;
		uint8_t i = 0;
		uint8_t essChnlNum = prAisSpecificBssInfo->ucCurEssChnlInfoNum;

		for (i = 0; i < essChnlNum; i++) {
			ucChannelNum =
				prAisSpecificBssInfo->
				arCurEssChnlInfo[i].ucChannel;
			prChnlInfo[i].eBand =
				prAisSpecificBssInfo->arCurEssChnlInfo[i].eBand;
			prChnlInfo[i].ucChannelNum = ucChannelNum;
		}
		prScanReqMsg->ucChannelListNum = essChnlNum;
		prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
		prScanReqMsg->fgOobRnrParseEn = FALSE;
		DBGLOG(AIS, INFO,
			   "[Roaming] Target Scan: Total number of scan channel(s)=%d\n",
			   prScanReqMsg->ucChannelListNum);
	} else if (prAdapter->aePreferBand[KAL_NETWORK_TYPE_AIS_INDEX] ==
			BAND_NULL) {
		if (prAdapter->fgEnable5GBand == TRUE)
			prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
		else
			prScanReqMsg->eScanChannel = SCAN_CHANNEL_2G4;
	} else if (prAdapter->aePreferBand[KAL_NETWORK_TYPE_AIS_INDEX] ==
			BAND_2G4) {
		prScanReqMsg->eScanChannel = SCAN_CHANNEL_2G4;
	} else if (prAdapter->aePreferBand[KAL_NETWORK_TYPE_AIS_INDEX] ==
			BAND_5G) {
		prScanReqMsg->eScanChannel = SCAN_CHANNEL_5G;
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prAdapter->aePreferBand[KAL_NETWORK_TYPE_AIS_INDEX] ==
			BAND_6G)
		prScanReqMsg->eScanChannel = SCAN_CHANNEL_6G;
#endif
	else
		prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;

	switch (prScanReqMsg->eScanChannel) {
	case SCAN_CHANNEL_FULL:
	case SCAN_CHANNEL_2G4:
	case SCAN_CHANNEL_5G:
	case SCAN_CHANNEL_6G:
		scanSetRequestChannel(prAdapter,
			prScanRequest->u4ChannelNum,
			prScanRequest->arChannel,
			prScanRequest->u4Flags,
			prAisFsmInfo->eCurrentState ==
			AIS_STATE_ONLINE_SCAN ||
			wlanWfdEnabled(prAdapter),
			prScanReqMsg);
		break;
	default:
		break;
	}
}

static void aisScanProcessReqExtra(struct ADAPTER *prAdapter,
	struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg,
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest)
{
	/* Reduce APP scan's dwell time when scan ch > 5,
	 * prevent it affecting TX/RX performance
	 */
	if ((prScanRequest->u4Flags &
		NL80211_SCAN_FLAG_LOW_SPAN)
		&& prScanReqMsg->ucChannelListNum > 5) {
		prScanReqMsg->u2ChannelDwellTime =
			SCAN_CHANNEL_DWELL_TIME_MSEC_APP;
		prScanReqMsg->u2ChannelMinDwellTime =
			SCAN_CHANNEL_MIN_DWELL_TIME_MSEC_APP;
	}
	if (prAdapter->rWifiVar.u4SwTestMode ==
		ENUM_SW_TEST_MODE_SIGMA_VOICE_ENT &&
		prScanReqMsg->ucChannelListNum == 1) {
		prScanReqMsg->u2ChannelDwellTime =
				SCAN_CHANNEL_DWELL_TIME_VOE;
		prScanReqMsg->u2ChannelMinDwellTime =
				SCAN_CHANNEL_DWELL_TIME_MIN_MSEC;
		DBGLOG(AIS, INFO,
				"[VoE] Adjust dwell time(50ms) for certification\n");
	}
#if CFG_EXT_SCAN
	/* VOE Cert. test set dwell time to 50ms */
	if (prAdapter->rWifiVar.u4SwTestMode ==
		ENUM_SW_TEST_MODE_SIGMA_VOICE_ENT &&
		(aisFsmIsInProcessPostpone(prAdapter,
		prScanReqMsg->ucBssIndex) ||
		kalGetMediaStateIndicated(
		prAdapter->prGlueInfo,
		prScanReqMsg->ucBssIndex) ==
		MEDIA_STATE_CONNECTED)) {
		prScanReqMsg->u2ChannelDwellTime =
				SCAN_CHANNEL_DWELL_TIME_VOE;
		prScanReqMsg->u2ChannelMinDwellTime =
				SCAN_CHANNEL_DWELL_TIME_MIN_MSEC;
	}
#endif

}

static void aisScanResetReq(struct PARAM_SCAN_REQUEST_ADV *prScanRequest)
{
	kalMemZero(prScanRequest, sizeof(struct PARAM_SCAN_REQUEST_ADV));
	prScanRequest->ucScanType = SCAN_TYPE_ACTIVE_SCAN;
}

u_int8_t aisUpdateInterfaceAddr(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	uint8_t aucMacAddr[])
{
	struct BSS_INFO *prAisBssInfo = NULL;
	struct WIFI_VAR *prWifiVar;

	if (!prAdapter || !prAisFsmInfo || !aucMacAddr)
		return FALSE;

	prWifiVar = &prAdapter->rWifiVar;
	prAisBssInfo = aisGetMainLinkBssInfo(prAisFsmInfo);

	if (!prWifiVar || !prAisBssInfo)
		return FALSE;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldBssUpdateMldAddr(prAdapter,
			    mldBssGetByBss(prAdapter, prAisBssInfo),
			    aucMacAddr);
	if (IS_FEATURE_ENABLED(prWifiVar->fgMldSyncLinkAddr))
		nicApplyLinkAddress(prAdapter,
				    aucMacAddr,
				    prAisBssInfo->aucOwnMacAddr,
				    AIS_MAIN_LINK_INDEX);
#else
	nicApplyLinkAddress(prAdapter,
			    aucMacAddr,
			    prAisBssInfo->aucOwnMacAddr,
			    AIS_MAIN_LINK_INDEX);
#endif

	return TRUE;
}

void aisFunFlushTxQueue(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec)
{
	DBGLOG(AIS, TRACE, "STA[%d] Flush TX queue filled at old channel.\n",
			prStaRec->ucIndex);
	if (HAL_IS_TX_DIRECT(prAdapter)) {
		nicTxDirectClearStaAcmQ(prAdapter, prStaRec->ucIndex);
		nicTxDirectClearStaPendQ(prAdapter, prStaRec->ucIndex);
		nicTxDirectClearStaPsQ(prAdapter, prStaRec->ucIndex);
	} else {
		struct MSDU_INFO *prFlushedTxPacketList = NULL;

		prFlushedTxPacketList = qmFlushStaTxQueues(prAdapter,
						prStaRec->ucIndex);
		if (prFlushedTxPacketList)
			wlanProcessQueuedMsduInfo(prAdapter,
					prFlushedTxPacketList);
	}
}

void aisFunSwitchChannel(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_CSA_REQ *prAisReq;

	if (!prBssInfo)
		return;

	if (IS_AIS_CH_SWITCH(prBssInfo)) {
		DBGLOG(AIS, WARN,
			"[%d] Channel switch is pending[%d] or ongoing[%d].\n",
			prBssInfo->ucBssIndex,
			prBssInfo->fgIsAisCsaPending,
			prBssInfo->fgIsAisSwitchingChnl);
		return;
	}

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, prBssInfo->ucBssIndex);
	if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
		aisFunSwitchChannelImpl(prAdapter, prBssInfo->ucBssIndex);
		return;
	}

	prAisReq = (struct AIS_CSA_REQ *) cnmMemAlloc(
						prAdapter, RAM_TYPE_MSG,
						sizeof(struct AIS_CSA_REQ));
	if (!prAisReq) {
		DBGLOG(AIS, ERROR,
			"[%d] Can't generate new csa req\n",
			prBssInfo->ucBssIndex);
		return;
	}

	prAisReq->rReqHdr.eReqType = AIS_REQUEST_CSA;
	prAisReq->ucBssIndex = prBssInfo->ucBssIndex;
	prBssInfo->fgIsAisCsaPending = TRUE;

	aisFsmClearRequest(prAdapter,
			    AIS_REQUEST_CSA,
			    prBssInfo->ucBssIndex);
	aisFsmInsertRequestImpl(prAdapter,
			    (struct AIS_REQ_HDR *)prAisReq,
			    TRUE,
			    prBssInfo->ucBssIndex);
}

void aisFunSwitchChannelImpl(struct ADAPTER *prAdapter,
				uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (!IS_MLD_BSSINFO_MULTI(mldBssGetByBss(prAdapter, prAisBssInfo)))
#endif
	{
		/* Indicate PM abort to sync BSS state with FW */
		nicPmIndicateBssAbort(prAdapter, prAisBssInfo->ucBssIndex);
		prAisBssInfo->ucDTIMPeriod = 0;

		/* Update BSS with temp. disconnect state to FW */
		if (IS_NET_ACTIVE(prAdapter, ucBssIndex))
			nicDeactivateNetworkEx(prAdapter,
				NETWORK_ID(ucBssIndex,
				aisGetLinkIndex(prAdapter, ucBssIndex)),
				FALSE);
		aisChangeMediaState(prAisBssInfo,
			MEDIA_STATE_DISCONNECTED);
		nicUpdateBssEx(prAdapter,
			ucBssIndex,
			FALSE);
	}

	prAisBssInfo->fgIsAisCsaPending = FALSE;
	prAisBssInfo->fgIsAisSwitchingChnl = TRUE;

	/* Release channel if CSA immediately before set authorized */
	aisFsmReleaseCh(prAdapter, ucBssIndex);
	aisReqJoinChPrivilegeForCSA(prAdapter, prAisFsmInfo,
		prAisBssInfo, &prAisFsmInfo->ucSeqNumOfChReq);
}

void aisFunSwitchChannelAbort(struct ADAPTER *ad,
				struct AIS_FSM_INFO *ais, uint8_t fgResetAll)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *prAisBssInfo = aisGetLinkBssInfo(ais, i);
		struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams;
		uint8_t ucBssIndex;

		if (!prAisBssInfo)
			continue;
		prCSAParams = &prAisBssInfo->CSAParams;
		ucBssIndex = prAisBssInfo->ucBssIndex;

		/* CSA is in requet list, clear all  */
		if (prAisBssInfo->fgIsAisCsaPending) {
			aisFsmClearRequest(ad, AIS_REQUEST_CSA, ucBssIndex);
			prAisBssInfo->fgIsAisCsaPending = FALSE;
		}
		/* CSA is waiting channel grant, release it */
		if (prAisBssInfo->fgIsAisSwitchingChnl) {
			prAisBssInfo->fgIsAisSwitchingChnl = FALSE;
			aisFsmReleaseCh(ad, ucBssIndex);
			aisChangeMediaState(prAisBssInfo,
				MEDIA_STATE_CONNECTED);
		}

		if (prCSAParams->fgHasStopTx)
			kalIndicateAllQueueTxAllowed(ad->prGlueInfo,
				ucBssIndex, TRUE);

		if (fgResetAll) {
			cnmTimerStopTimer(ad, &prAisBssInfo->rCsaTimer);
			cnmTimerStopTimer(ad, &prAisBssInfo->rCsaDoneTimer);
		}

		rlmResetCSAParams(prAisBssInfo, TRUE);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will request new channel once reciving CSA request.
 *
 * @param[in] prAisFsmInfo              Pointer to AIS_FSM_INFO
 * @param[in] prBss                     Pointer to AIS BSS_INFO_T
 * @param[in] ucChTokenId               Pointer to token ID
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisReqJoinChPrivilegeForCSA(struct ADAPTER *prAdapter,
				struct AIS_FSM_INFO *prAisFsmInfo,
				struct BSS_INFO *prBss,
				uint8_t *ucChTokenId)
{
	struct MSG_CH_REQ *prMsgChReq = NULL;

	prMsgChReq = (struct MSG_CH_REQ *)cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG,
		sizeof(struct MSG_CH_REQ));
	if (!prMsgChReq) {
		DBGLOG(AIS, ERROR, "Alloc CH req msg failed.\n");
		return;
	}
	kalMemZero(prMsgChReq, sizeof(struct MSG_CH_REQ));

	*ucChTokenId = cnmIncreaseTokenId(prAdapter);
	prAisFsmInfo->ucChReqNum = 1;
	prAisFsmInfo->fgIsChannelRequested = TRUE;
	prAisFsmInfo->ucBssIndexOfChReq = prBss->ucBssIndex;

	prMsgChReq->ucExtraChReqNum = 0;

	prMsgChReq->ucBssIndex = prBss->ucBssIndex;
#if CFG_SUPPORT_DBDC
	prMsgChReq->eDBDCBand = ENUM_BAND_AUTO;
#endif
	prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
	prMsgChReq->ucTokenID = *ucChTokenId;
	prMsgChReq->eReqType = CH_REQ_TYPE_JOIN;
	prMsgChReq->u4MaxInterval = AIS_JOIN_CH_REQUEST_INTERVAL;
	prMsgChReq->ucPrimaryChannel = prBss->ucPrimaryChannel;
	prMsgChReq->eRfSco = prBss->eBssSCO;
	prMsgChReq->eRfBand = prBss->eBand;
	prMsgChReq->eRfChannelWidth = prBss->ucVhtChannelWidth;
	prMsgChReq->ucRfCenterFreqSeg1 = prBss->ucVhtChannelFrequencyS1;
	prMsgChReq->ucRfCenterFreqSeg2 = prBss->ucVhtChannelFrequencyS2;

	mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *)prMsgChReq,
			MSG_SEND_METHOD_UNBUF);
} /* end of aisReqJoinChPrivilegeForCSA() */

u_int8_t aisFsmIsSwitchChannel(struct ADAPTER *ad,
	struct AIS_FSM_INFO *ais)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *prAisBssInfo = aisGetLinkBssInfo(ais, i);

		if (!prAisBssInfo)
			continue;

		if (timerPendingTimer(&prAisBssInfo->rCsaTimer) ||
		    IS_AIS_CH_SWITCH(prAisBssInfo))
			return TRUE;
	}
	return FALSE;
}
