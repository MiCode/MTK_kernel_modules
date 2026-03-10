/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/ais_fsm.c#4
*/

/*
 * ! \file   "aa_fsm.c"
 *   \brief  This file defines the FSM for SAA and AAA MODULE.
 *
 *   This file defines the FSM for SAA and AAA MODULE.
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
#ifdef FW_CFG_SUPPORT
#ifdef CFG_SUPPORT_COEX_IOT_AP
#include "fwcfg.h"
#endif
#endif

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define AIS_ROAMING_CONNECTION_TRIAL_LIMIT  2
#define AIS_JOIN_TIMEOUT                    7

#define CTIA_MAGIC_SSID                     "no_use_ctia_ssid"	/* "ctia_test_only_*#*#3646633#*#*" */
#define CTIA_MAGIC_SSID_LEN			16				/* strlen(CTIA_MAGIC_SSID) */

#define AIS_FSM_STATE_SEARCH_ACTION_PHASE_0	0
#define AIS_FSM_STATE_SEARCH_ACTION_PHASE_1	1
#define AIS_FSM_STATE_SEARCH_ACTION_PHASE_2	2

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
#if DBG
/*lint -save -e64 Type mismatch */
static PUINT_8 apucDebugAisState[AIS_STATE_NUM] = {
	(PUINT_8) DISP_STRING("AIS_STATE_IDLE"),
	(PUINT_8) DISP_STRING("AIS_STATE_SEARCH"),
	(PUINT_8) DISP_STRING("AIS_STATE_SCAN"),
	(PUINT_8) DISP_STRING("AIS_STATE_ONLINE_SCAN"),
	(PUINT_8) DISP_STRING("AIS_STATE_LOOKING_FOR"),
	(PUINT_8) DISP_STRING("AIS_STATE_WAIT_FOR_NEXT_SCAN"),
	(PUINT_8) DISP_STRING("AIS_STATE_REQ_CHANNEL_JOIN"),
	(PUINT_8) DISP_STRING("AIS_STATE_JOIN"),
	(PUINT_8) DISP_STRING("AIS_STATE_JOIN_FAILURE"),
	(PUINT_8) DISP_STRING("AIS_STATE_IBSS_ALONE"),
	(PUINT_8) DISP_STRING("AIS_STATE_IBSS_MERGE"),
	(PUINT_8) DISP_STRING("AIS_STATE_NORMAL_TR"),
	(PUINT_8) DISP_STRING("AIS_STATE_DISCONNECTING"),
	(PUINT_8) DISP_STRING("AIS_STATE_REQ_REMAIN_ON_CHANNEL"),
	(PUINT_8) DISP_STRING("AIS_STATE_REMAIN_ON_CHANNEL")
};

/*lint -restore */
#endif /* DBG */

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static VOID aisFsmSetOkcTimeout(IN P_ADAPTER_T prAdapter, ULONG ulParam);
static VOID aisFsmRunEventScanDoneTimeOut(IN P_ADAPTER_T prAdapter, ULONG ulParam);
static VOID aisRemoveOldestBcnTimeout(P_AIS_FSM_INFO_T prAisFsmInfo);
static VOID aisRemoveDisappearedBlacklist(P_ADAPTER_T prAdapter);
#if defined(MT6631)
static VOID aisPreScan(P_ADAPTER_T prAdapter, struct _MSG_SCN_SCAN_REQ_V3_T *prMsg);
#else
static VOID aisPreScan(P_ADAPTER_T prAdapter, P_MSG_SCN_SCAN_REQ_V2 prMsg);
#endif
static VOID aisPostScan(P_ADAPTER_T prAdapter);
static VOID aisFsmMayRequstPmkId(P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc);
static VOID aisCheckNeighborApValidity(IN P_ADAPTER_T prAdapter);
static VOID aisTxBtmResponseTimeout(IN P_ADAPTER_T prAdapter, ULONG ulParam);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
static VOID aisResetBssTranstionMgtParam(P_AIS_SPECIFIC_BSS_INFO_T prSpecificBssInfo)
{
	struct BSS_TRANSITION_MGT_PARAM_T *prBtmParam = &prSpecificBssInfo->rBTMParam;

#if !CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
	return;
#endif
	if (prBtmParam->u2OurNeighborBssLen > 0) {
		kalMemFree(prBtmParam->pucOurNeighborBss, VIR_MEM_TYPE, prBtmParam->u2OurNeighborBssLen);
		prBtmParam->u2OurNeighborBssLen = 0;
	}
	kalMemZero(prBtmParam, sizeof(*prBtmParam));
}

/*----------------------------------------------------------------------------*/
/*!
* @brief the function is used to initialize the value of the connection settings for
*        AIS network
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisInitializeConnectionSettings(IN P_ADAPTER_T prAdapter, IN P_REG_INFO_T prRegInfo)
{
	P_CONNECTION_SETTINGS_T prConnSettings;
	UINT_8 aucAnyBSSID[] = BC_BSSID;
	UINT_8 aucZeroMacAddr[] = NULL_MAC_ADDR;
	int i = 0;

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	/* Setup default values for operation */
	COPY_MAC_ADDR(prConnSettings->aucMacAddress, aucZeroMacAddr);

	prConnSettings->ucDelayTimeOfDisconnectEvent = AIS_DELAY_TIME_OF_DISCONNECT_SEC;
	/*For 11n 5.2.16*/
	if (prAdapter->rWifiVar.ucCert11nMode == 1)
		prConnSettings->ucDelayTimeOfDisconnectEvent = 10;


	COPY_MAC_ADDR(prConnSettings->aucBSSID, aucAnyBSSID);
	prConnSettings->fgIsConnByBssidIssued = FALSE;

	prConnSettings->eReConnectLevel = RECONNECT_LEVEL_MIN;
	prConnSettings->fgIsConnReqIssued = FALSE;
	prConnSettings->fgIsDisconnectedByNonRequest = FALSE;

	prConnSettings->ucSSIDLen = 0;

	prConnSettings->eOPMode = NET_TYPE_INFRA;

	prConnSettings->eConnectionPolicy = CONNECT_BY_SSID_BEST_RSSI;

	if (prRegInfo) {
		prConnSettings->ucAdHocChannelNum = (UINT_8) nicFreq2ChannelNum(prRegInfo->u4StartFreq);
		prConnSettings->eAdHocBand = prRegInfo->u4StartFreq < 5000000 ? BAND_2G4 : BAND_5G;
		prConnSettings->eAdHocMode = (ENUM_PARAM_AD_HOC_MODE_T) (prRegInfo->u4AdhocMode);
	}

	prConnSettings->eAuthMode = AUTH_MODE_OPEN;

	prConnSettings->eEncStatus = ENUM_ENCRYPTION_DISABLED;

	prConnSettings->fgIsScanReqIssued = FALSE;

	/* MIB attributes */
	prConnSettings->u2BeaconPeriod = DOT11_BEACON_PERIOD_DEFAULT;

	prConnSettings->u2RTSThreshold = DOT11_RTS_THRESHOLD_DEFAULT;

	prConnSettings->u2DesiredNonHTRateSet = RATE_SET_ALL_ABG;

	/* prConnSettings->u4FreqInKHz; *//* Center frequency */

	/* Set U-APSD AC */
	prConnSettings->bmfgApsdEnAc = PM_UAPSD_NONE;

	secInit(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* Features */
	prConnSettings->fgIsEnableRoaming = FALSE;

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
	prConnSettings->fgSecModeChangeStartTimer = FALSE;
#endif

#if CFG_SUPPORT_ROAMING
	if (prRegInfo)
		prConnSettings->fgIsEnableRoaming = ((prRegInfo->fgDisRoaming > 0) ? (FALSE) : (TRUE));
	/*override disable/enable roaming settings by cfg*/
	if (prAdapter->rWifiVar.fgDisRoaming)
		prConnSettings->fgIsEnableRoaming = FALSE;
	else
		prConnSettings->fgIsEnableRoaming = TRUE;
#endif /* CFG_SUPPORT_ROAMING */

	prConnSettings->fgIsAdHocQoSEnable = FALSE;

#if CFG_SUPPORT_802_11AC
	prConnSettings->eDesiredPhyConfig = PHY_CONFIG_802_11ABGNAC;
#else
	prConnSettings->eDesiredPhyConfig = PHY_CONFIG_802_11ABGN;
#endif

	/* Set default bandwidth modes */
	prConnSettings->uc2G4BandwidthMode = CONFIG_BW_20M;
	prConnSettings->uc5GBandwidthMode = CONFIG_BW_20_40M;

	prConnSettings->rRsnInfo.ucElemId = 0x30;
	prConnSettings->rRsnInfo.u2Version = 0x0001;
	prConnSettings->rRsnInfo.u4GroupKeyCipherSuite = 0;
	prConnSettings->rRsnInfo.u4PairwiseKeyCipherSuiteCount = 0;
	for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
		prConnSettings->rRsnInfo.au4PairwiseKeyCipherSuite[i] = 0;
	prConnSettings->rRsnInfo.u4AuthKeyMgtSuiteCount = 0;
	for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++)
		prConnSettings->rRsnInfo.au4AuthKeyMgtSuite[i] = 0;
	prConnSettings->rRsnInfo.u2RsnCap = 0;
	prConnSettings->rRsnInfo.fgRsnCapPresent = FALSE;

}				/* end of aisFsmInitializeConnectionSettings() */

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
VOID aisFsmInit(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_INFO_T prAisBssInfo;
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo;

	DBGLOG(AIS, TRACE, "->aisFsmInit()\n");

	prAdapter->prAisBssInfo = prAisBssInfo = cnmGetBssInfoAndInit(prAdapter, NETWORK_TYPE_AIS, FALSE);
	ASSERT(prAisBssInfo);

	/* update MAC address */
	COPY_MAC_ADDR(prAdapter->prAisBssInfo->aucOwnMacAddr, prAdapter->rMyMacAddr);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisSpecificBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);

	/* 4 <1> Initiate FSM */
	prAisFsmInfo->ePreviousState = AIS_STATE_IDLE;
	prAisFsmInfo->eCurrentState = AIS_STATE_IDLE;

	prAisFsmInfo->ucAvailableAuthTypes = 0;

	prAisFsmInfo->prTargetBssDesc = (P_BSS_DESC_T) NULL;

	prAisFsmInfo->ucSeqNumOfReqMsg = 0;
	prAisFsmInfo->ucSeqNumOfChReq = 0;
	prAisFsmInfo->ucSeqNumOfScanReq = 0;

	prAisFsmInfo->fgIsInfraChannelFinished = TRUE;
#if CFG_SUPPORT_ROAMING
	prAisFsmInfo->fgIsRoamingScanPending = FALSE;
#endif /* CFG_SUPPORT_ROAMING */
	prAisFsmInfo->fgIsChannelRequested = FALSE;
	prAisFsmInfo->fgIsChannelGranted = FALSE;
	prAisFsmInfo->u4PostponeIndStartTime = 0;
	prAisFsmInfo->ucJoinFailCntAfterScan = 0;
	prAisFsmInfo->u4ScanUpdateIdx = 0;
#if CFG_SUPPORT_ABORT_SCAN
	AisFsmSetScanState(prAdapter, FALSE);
#endif

	/* 4 <1.1> Initiate FSM - Timer INIT */
	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rBGScanTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventBGSleepTimeOut, (ULONG) NULL);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rIbssAloneTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventIbssAloneTimeOut, (ULONG) NULL);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rScanDoneTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventScanDoneTimeOut, (ULONG) NULL);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rJoinTimeoutTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventJoinTimeout, (ULONG) NULL);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rDeauthDoneTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventDeauthTimeout, (ULONG) NULL);

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rSecModeChangeTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventSecModeChangeTimeout, (ULONG) NULL);
#endif
	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rWaitOkcPMKTimer,
			  (PFN_MGMT_TIMEOUT_FUNC)aisFsmSetOkcTimeout, (ULONG) NULL);

	cnmTimerInitTimer(prAdapter,
			  &prAisSpecificBssInfo->rBtmResponseTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisTxBtmResponseTimeout, (ULONG) NULL);
	cnmTimerInitTimer(prAdapter,
			&prAisFsmInfo->rChannelTimeoutTimer,
			(PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventChannelTimeout, (ULONG) NULL);

	/* 4 <1.2> Initiate PWR STATE */
	SET_NET_PWR_STATE_IDLE(prAdapter, prAisBssInfo->ucBssIndex);

	/* 4 <2> Initiate BSS_INFO_T - common part */
	BSS_INFO_INIT(prAdapter, prAisBssInfo);
	COPY_MAC_ADDR(prAisBssInfo->aucOwnMacAddr, prAdapter->rWifiVar.aucMacAddress);

	/* 4 <3> Initiate BSS_INFO_T - private part */
	/* TODO */
	prAisBssInfo->eBand = BAND_2G4;
	prAisBssInfo->ucPrimaryChannel = 1;
	prAisBssInfo->prStaRecOfAP = (P_STA_RECORD_T) NULL;

	/* 4 <4> Allocate MSDU_INFO_T for Beacon */
	prAisBssInfo->prBeacon = cnmMgtPktAlloc(prAdapter,
						OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem[0]) + MAX_IE_LENGTH);

	if (prAisBssInfo->prBeacon) {
		prAisBssInfo->prBeacon->eSrc = TX_PACKET_MGMT;
		prAisBssInfo->prBeacon->ucStaRecIndex = 0xFF;	/* NULL STA_REC */
	} else {
		ASSERT(0);
	}
	/* secGetBmcWlanIndex(prAdapter, NETWORK_TYPE_AIS, prAisBssInfo->ucBssIndex); */
	prAisBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;

#if 0
	prAisBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC = PM_UAPSD_ALL;
	prAisBssInfo->rPmProfSetupInfo.ucBmpTriggerAC = PM_UAPSD_ALL;
	prAisBssInfo->rPmProfSetupInfo.ucUapsdSp = WMM_MAX_SP_LENGTH_2;
#else
	if (prAdapter->u4UapsdAcBmp == 0) {
		prAdapter->u4UapsdAcBmp = CFG_INIT_UAPSD_AC_BMP;
		/* ASSERT(prAdapter->u4UapsdAcBmp); */
	}
	prAisBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC = (UINT_8) prAdapter->u4UapsdAcBmp;
	prAisBssInfo->rPmProfSetupInfo.ucBmpTriggerAC = (UINT_8) prAdapter->u4UapsdAcBmp;
	prAisBssInfo->rPmProfSetupInfo.ucUapsdSp = (UINT_8) prAdapter->u4MaxSpLen;
#endif

	/* request list initialization */
	LINK_INITIALIZE(&prAisFsmInfo->rPendingReqList);

	LINK_MGMT_INIT(&prAdapter->rWifiVar.rConnSettings.rBlackList);
	LINK_MGMT_INIT(&prAisFsmInfo->rBcnTimeout);
	LINK_MGMT_INIT(&prAisSpecificBssInfo->rNeighborApList);
	kalMemZero(&prAisSpecificBssInfo->arCurEssChnlInfo[0], sizeof(prAisSpecificBssInfo->arCurEssChnlInfo));
	LINK_INITIALIZE(&prAisSpecificBssInfo->rCurEssLink);
	kalMemZero(&prAisSpecificBssInfo->rBTMParam, sizeof(prAisSpecificBssInfo->rBTMParam));
	/* DBGPRINTF("[2] ucBmpDeliveryAC:0x%x, ucBmpTriggerAC:0x%x, ucUapsdSp:0x%x", */
	/* prAisBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC, */
	/* prAisBssInfo->rPmProfSetupInfo.ucBmpTriggerAC, */
	/* prAisBssInfo->rPmProfSetupInfo.ucUapsdSp); */

	/* Bind NetDev & BssInfo */
	/* wlanBindBssIdxToNetInterface(prAdapter->prGlueInfo, NET_DEV_WLAN_IDX, prAisBssInfo->ucBssIndex); */

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
VOID aisFsmUninit(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_INFO_T prAisBssInfo;
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo;

	DBGLOG(AIS, TRACE, "->aisFsmUninit()\n");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisSpecificBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);

	/* 4 <1> Stop all timers */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBGScanTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);
#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rSecModeChangeTimer);
#endif
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rWaitOkcPMKTimer);
	cnmTimerStopTimer(prAdapter, &prAisSpecificBssInfo->rBtmResponseTimer);

	/* 4 <2> flush pending request */
	aisFsmFlushRequest(prAdapter);
	aisResetBssTranstionMgtParam(prAisSpecificBssInfo);

	/* 4 <3> Reset driver-domain BSS-INFO */
	if ((prAisBssInfo != NULL) && (prAisBssInfo->prBeacon != NULL)) {
		cnmMgtPktFree(prAdapter, prAisBssInfo->prBeacon);
		prAisBssInfo->prBeacon = NULL;
	}
#if CFG_SUPPORT_802_11W
	rsnStopSaQuery(prAdapter);
#endif

	if (prAisBssInfo) {
		cnmFreeBssInfo(prAdapter, prAisBssInfo);
		prAdapter->prAisBssInfo = NULL;
	}
	LINK_MGMT_UNINIT(&prAdapter->rWifiVar.rConnSettings.rBlackList, struct AIS_BLACKLIST_ITEM, VIR_MEM_TYPE);
	LINK_MGMT_UNINIT(&prAisFsmInfo->rBcnTimeout, struct AIS_BEACON_TIMEOUT_BSS, VIR_MEM_TYPE);
	LINK_MGMT_UNINIT(&prAisSpecificBssInfo->rNeighborApList, struct NEIGHBOR_AP_T, VIR_MEM_TYPE);
}				/* end of aisFsmUninit() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Initialization of JOIN STATE
*
* @param[in] prBssDesc  The pointer of BSS_DESC_T which is the BSS we will try to join with.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmStateInit_JOIN(IN P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_INFO_T prAisBssInfo;
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_STA_RECORD_T prStaRec;
	P_MSG_JOIN_REQ_T prJoinReqMsg;

	DEBUGFUNC("aisFsmStateInit_JOIN()");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisSpecificBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	ASSERT(prBssDesc);

	/* 4 <1> We are going to connect to this BSS. */
	prBssDesc->fgIsConnecting = TRUE;

	/* 4 <2> Setup corresponding STA_RECORD_T */
	prStaRec = bssCreateStaRecFromBssDesc(prAdapter,
					      STA_TYPE_LEGACY_AP, prAdapter->prAisBssInfo->ucBssIndex, prBssDesc);

	prAisFsmInfo->prTargetStaRec = prStaRec;

	/* 4 <2.1> sync. to firmware domain */
	if (prStaRec->ucStaState == STA_STATE_1)
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

	/* 4 <3> Update ucAvailableAuthTypes which we can choice during SAA */
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED) {

		prStaRec->fgIsReAssoc = FALSE;

		switch (prConnSettings->eAuthMode) {
		case AUTH_MODE_NON_RSN_FT: /* FT initial mobility doamin association always use Open AA */
		case AUTH_MODE_WPA2_FT:
		case AUTH_MODE_WPA2_FT_PSK:
		case AUTH_MODE_OPEN:	/* Note: Omit break here. */
		case AUTH_MODE_WPA:
		case AUTH_MODE_WPA_PSK:
		case AUTH_MODE_WPA2:
		case AUTH_MODE_WPA2_PSK:
		case AUTH_MODE_WPA3_OWE:
			prAisFsmInfo->ucAvailableAuthTypes = (UINT_8) AUTH_TYPE_OPEN_SYSTEM;
			break;

		case AUTH_MODE_SHARED:
			prAisFsmInfo->ucAvailableAuthTypes = (UINT_8) AUTH_TYPE_SHARED_KEY;
			break;

		case AUTH_MODE_AUTO_SWITCH:
			DBGLOG(AIS, LOUD, "JOIN INIT: eAuthMode == AUTH_MODE_AUTO_SWITCH\n");
			prAisFsmInfo->ucAvailableAuthTypes = (UINT_8) (AUTH_TYPE_OPEN_SYSTEM | AUTH_TYPE_SHARED_KEY);
			break;
		case AUTH_MODE_WPA3_SAE:
			DBGLOG(AIS, LOUD, "JOIN INIT: eAuthMode == AUTH_MODE_WPA3_SAE\n");
			prAisFsmInfo->ucAvailableAuthTypes = (UINT_8) AUTH_TYPE_SAE;
			break;
		default:
			ASSERT(!(prConnSettings->eAuthMode == AUTH_MODE_WPA_NONE));
			DBGLOG(AIS, ERROR,
			       "JOIN INIT: Auth Algorithm : %d was not supported by JOIN\n",
				prConnSettings->eAuthMode);
			/* TODO(Kevin): error handling ? */
			return;
		}

		/* TODO(tyhsu): Assume that Roaming Auth Type is equal to ConnSettings eAuthMode */
		prAisSpecificBssInfo->ucRoamingAuthTypes = prAisFsmInfo->ucAvailableAuthTypes;

		prStaRec->ucTxAuthAssocRetryLimit = TX_AUTH_ASSOCI_RETRY_LIMIT;
		/* reset Bss Transition Management Params when do first connection */
		aisResetBssTranstionMgtParam(prAisSpecificBssInfo);
	} else {
		ASSERT(prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE);

		DBGLOG(AIS, LOUD, "JOIN INIT: AUTH TYPE = %d for Roaming\n",
				   prAisSpecificBssInfo->ucRoamingAuthTypes);

		prStaRec->fgIsReAssoc = TRUE;	/* We do roaming while the medium is connected */

		/* TODO(Kevin): We may call a sub function to acquire the Roaming Auth Type */
		switch (prConnSettings->eAuthMode) {
		case AUTH_MODE_WPA2_FT:
		case AUTH_MODE_WPA2_FT_PSK:
		case AUTH_MODE_NON_RSN_FT:
			prAisFsmInfo->ucAvailableAuthTypes = (UINT_8) AUTH_TYPE_FAST_BSS_TRANSITION;
			break;
		case AUTH_MODE_WPA3_SAE:
			prAisFsmInfo->ucAvailableAuthTypes = (UINT_8) AUTH_TYPE_SAE;
			break;
		default:
			prAisFsmInfo->ucAvailableAuthTypes = prAisSpecificBssInfo->ucRoamingAuthTypes;
			break;
		}

		prStaRec->ucTxAuthAssocRetryLimit = TX_AUTH_ASSOCI_RETRY_LIMIT_FOR_ROAMING;
	}

	/* 4 <4> Use an appropriate Authentication Algorithm Number among the ucAvailableAuthTypes */
	if (prAisFsmInfo->ucAvailableAuthTypes & (UINT_8) AUTH_TYPE_SHARED_KEY) {

		DBGLOG(AIS, LOUD, "JOIN INIT: Try to do Authentication with AuthType == SHARED_KEY.\n");

		prAisFsmInfo->ucAvailableAuthTypes &= ~(UINT_8) AUTH_TYPE_SHARED_KEY;

		prStaRec->ucAuthAlgNum = (UINT_8) AUTH_ALGORITHM_NUM_SHARED_KEY;
	} else if (prAisFsmInfo->ucAvailableAuthTypes & (UINT_8) AUTH_TYPE_OPEN_SYSTEM) {

		DBGLOG(AIS, LOUD, "JOIN INIT: Try to do Authentication with AuthType == OPEN_SYSTEM.\n");
		prAisFsmInfo->ucAvailableAuthTypes &= ~(UINT_8) AUTH_TYPE_OPEN_SYSTEM;

		prStaRec->ucAuthAlgNum = (UINT_8) AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
	} else if (prAisFsmInfo->ucAvailableAuthTypes & (UINT_8) AUTH_TYPE_FAST_BSS_TRANSITION) {

		DBGLOG(AIS, LOUD, "JOIN INIT: Try to do Authentication with AuthType == FAST_BSS_TRANSITION.\n");

		prAisFsmInfo->ucAvailableAuthTypes &= ~(UINT_8) AUTH_TYPE_FAST_BSS_TRANSITION;

		prStaRec->ucAuthAlgNum = (UINT_8) AUTH_ALGORITHM_NUM_FAST_BSS_TRANSITION;
	} else if (prAisFsmInfo->ucAvailableAuthTypes & (UINT_8) AUTH_TYPE_SAE) {
		DBGLOG(AIS, LOUD, "JOIN INIT: Try to do Authentication with AuthType == SAE.\n");
		prAisFsmInfo->ucAvailableAuthTypes &= ~(UINT_8) AUTH_TYPE_SAE;
		prStaRec->ucAuthAlgNum = (UINT_8) AUTH_ALGORITHM_NUM_SAE;
	} else {
		ASSERT(0);
	}

	/* 4 <5> Overwrite Connection Setting for eConnectionPolicy == ANY (Used by Assoc Req) */
	if (prBssDesc->ucSSIDLen)
		COPY_SSID(prConnSettings->aucSSID, prConnSettings->ucSSIDLen, prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
	/* 4 <6> Send a Msg to trigger SAA to start JOIN process. */
	prJoinReqMsg = (P_MSG_JOIN_REQ_T) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_JOIN_REQ_T));
	if (!prJoinReqMsg) {

		ASSERT(0);	/* Can't trigger SAA FSM */
		return;
	}

	prJoinReqMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_START;
	prJoinReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfReqMsg;
	prJoinReqMsg->prStaRec = prStaRec;

	if (1) {
		int j;
		P_FRAG_INFO_T prFragInfo;

		for (j = 0; j < MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS; j++) {
			prFragInfo = &prStaRec->rFragInfo[j];

			if (prFragInfo->pr1stFrag) {
				/* nicRxReturnRFB(prAdapter, prFragInfo->pr1stFrag); */
				prFragInfo->pr1stFrag = (P_SW_RFB_T) NULL;
			}
		}
	}
#if CFG_SUPPORT_802_11K
	rlmSetMaxTxPwrLimit(prAdapter, (prBssDesc->cPowerLimit != RLM_INVALID_POWER_LIMIT) ?
	prBssDesc->cPowerLimit:RLM_MAX_TX_PWR, 1);
#endif

	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prJoinReqMsg, MSG_SEND_METHOD_BUF);

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
BOOLEAN aisFsmStateInit_RetryJOIN(IN P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_MSG_JOIN_REQ_T prJoinReqMsg;

	DEBUGFUNC("aisFsmStateInit_RetryJOIN()");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* Retry other AuthType if possible */
	if (!prAisFsmInfo->ucAvailableAuthTypes)
		return FALSE;

	if (prStaRec->u2StatusCode != STATUS_CODE_AUTH_ALGORITHM_NOT_SUPPORTED) {
		prAisFsmInfo->ucAvailableAuthTypes = 0;
		return FALSE;
	}

	if (prAisFsmInfo->ucAvailableAuthTypes & (UINT_8) AUTH_TYPE_OPEN_SYSTEM) {
		DBGLOG(AIS, INFO, "RETRY JOIN INIT: Retry Authentication with AuthType == OPEN_SYSTEM.\n");

		prAisFsmInfo->ucAvailableAuthTypes &= ~(UINT_8) AUTH_TYPE_OPEN_SYSTEM;

		prStaRec->ucAuthAlgNum = (UINT_8) AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
	} else {
		DBGLOG(AIS, ERROR, "RETRY JOIN INIT: Retry Authentication with Unexpected AuthType.\n");
		ASSERT(0);
	}

	prAisFsmInfo->ucAvailableAuthTypes = 0;	/* No more available Auth Types */

	/* Trigger SAA to start JOIN process. */
	prJoinReqMsg = (P_MSG_JOIN_REQ_T) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_JOIN_REQ_T));
	if (!prJoinReqMsg) {

		ASSERT(0);	/* Can't trigger SAA FSM */
		return FALSE;
	}

	prJoinReqMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_START;
	prJoinReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfReqMsg;
	prJoinReqMsg->prStaRec = prStaRec;

	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prJoinReqMsg, MSG_SEND_METHOD_BUF);

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
VOID aisFsmStateInit_IBSS_ALONE(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_INFO_T prAisBssInfo;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prAisBssInfo = prAdapter->prAisBssInfo;

	/* 4 <1> Check if IBSS was created before ? */
	if (prAisBssInfo->fgIsBeaconActivated) {

		/* 4 <2> Start IBSS Alone Timer for periodic SCAN and then SEARCH */
#if !CFG_SLT_SUPPORT
		cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer, SEC_TO_MSEC(AIS_IBSS_ALONE_TIMEOUT_SEC));
#endif
	}

	aisFsmCreateIBSS(prAdapter);

}				/* end of aisFsmStateInit_IBSS_ALONE() */

/*----------------------------------------------------------------------------*/
/*!
* @brief State Initialization of AIS_STATE_IBSS_MERGE
*
* @param[in] prBssDesc  The pointer of BSS_DESC_T which is the IBSS we will try to merge with.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmStateInit_IBSS_MERGE(IN P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_INFO_T prAisBssInfo;
	P_STA_RECORD_T prStaRec = (P_STA_RECORD_T) NULL;

	ASSERT(prBssDesc);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prAisBssInfo = prAdapter->prAisBssInfo;

	/* 4 <1> We will merge with to this BSS immediately. */
	prBssDesc->fgIsConnecting = FALSE;
	prBssDesc->fgIsConnected = TRUE;

	/* 4 <2> Setup corresponding STA_RECORD_T */
	prStaRec = bssCreateStaRecFromBssDesc(prAdapter,
					      STA_TYPE_ADHOC_PEER, prAdapter->prAisBssInfo->ucBssIndex, prBssDesc);

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
VOID aisFsmStateAbort_JOIN(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_MSG_JOIN_ABORT_T prJoinAbortMsg;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* 1. Abort JOIN process */
	prJoinAbortMsg = (P_MSG_JOIN_ABORT_T) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_JOIN_ABORT_T));
	if (!prJoinAbortMsg) {

		ASSERT(0);	/* Can't abort SAA FSM */
		return;
	}

	prJoinAbortMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_ABORT;
	prJoinAbortMsg->ucSeqNum = prAisFsmInfo->ucSeqNumOfReqMsg;
	prJoinAbortMsg->prStaRec = prAisFsmInfo->prTargetStaRec;

	prAisFsmInfo->prTargetBssDesc->fgIsConnected = FALSE;
	prAisFsmInfo->prTargetBssDesc->fgIsConnecting = FALSE;

	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prJoinAbortMsg, MSG_SEND_METHOD_BUF);

	/* 2. Return channel privilege */
	aisFsmReleaseCh(prAdapter);

	/* 3.1 stop join timeout timer */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);

	/* 3.2 reset local variable */
	prAisFsmInfo->fgIsInfraChannelFinished = TRUE;

}				/* end of aisFsmAbortJOIN() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Process of SCAN Abort
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmStateAbort_SCAN(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_MSG_SCN_SCAN_CANCEL prScanCancelMsg;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	DBGLOG(AIS, STATE, "aisFsmStateAbort_SCAN\n");

	/* Abort JOIN process. */
	prScanCancelMsg = (P_MSG_SCN_SCAN_CANCEL) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_SCN_SCAN_CANCEL));
	if (!prScanCancelMsg) {

		ASSERT(0);	/* Can't abort SCN FSM */
		return;
	}

	prScanCancelMsg->rMsgHdr.eMsgId = MID_AIS_SCN_SCAN_CANCEL;
	prScanCancelMsg->ucSeqNum = prAisFsmInfo->ucSeqNumOfScanReq;
	prScanCancelMsg->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
	prScanCancelMsg->fgIsChannelExt = FALSE;

	/* unbuffered message to guarantee scan is cancelled in sequence */
	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prScanCancelMsg, MSG_SEND_METHOD_UNBUF);

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
VOID aisFsmStateAbort_NORMAL_TR(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* TODO(Kevin): Do abort other MGMT func */

	/* 1. Release channel to CNM */
	aisFsmReleaseCh(prAdapter);

	/* 2.1 stop join timeout timer */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);

	/* 2.2 reset local variable */
	prAisFsmInfo->fgIsInfraChannelFinished = TRUE;
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
VOID aisFsmStateAbort_IBSS(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_DESC_T prBssDesc;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* reset BSS-DESC */
	if (prAisFsmInfo->prTargetStaRec) {
		prBssDesc = scanSearchBssDescByTA(prAdapter, prAisFsmInfo->prTargetStaRec->aucMacAddr);

		if (prBssDesc) {
			prBssDesc->fgIsConnected = FALSE;
			prBssDesc->fgIsConnecting = FALSE;
		}
	}
	/* release channel privilege */
	aisFsmReleaseCh(prAdapter);

}
#endif /* CFG_SUPPORT_ADHOC */

/*----------------------------------------------------------------------------*/
/*!
* @brief Change array index to channel number.
*
* @param[in] ucIndex            array index.
*
* @retval ucChannelNum           ucChannelNum
*/
/*----------------------------------------------------------------------------*/
UINT_8 aisIndex2ChannelNum(IN UINT_8 ucIndex)
{
	UINT_8 ucChannel;

	/*Full2Partial*/
	if (ucIndex >= 1 && ucIndex <= 14)
		/*1---14*/
		ucChannel = ucIndex;
		/*1---14*/
	else if (ucIndex >= 15 && ucIndex <= 22)
		/*15---22*/
		ucChannel = (ucIndex - 6) << 2;
		/*36---64*/
	else if (ucIndex >= 23 && ucIndex <= 34)
		/*23---34*/
		ucChannel = (ucIndex + 2) << 2;
		/*100---144*/
	else if (ucIndex >= 35 && ucIndex <= 39) {
		/*35---39*/
		ucIndex = ucIndex + 2;
		ucChannel = (ucIndex << 2) + 1;
		/*149---164*/
	} else {
		/*error*/
		ucChannel = 0;
	}
	return ucChannel;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Full2Partial Process full scan channel info
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/

VOID aisGetAndSetScanChannel(IN P_ADAPTER_T prAdapter)
{
	P_PARTIAL_SCAN_INFO prPartialScanChannel = NULL;
	struct GL_Channel_scan_info	*prChannelScanInfo;
	UINT_8	ucChannelNum;
	int i = 1;
	UINT_8 t = 0;
	/*Full2Partial*/

	/*at here set channel info*/
	prPartialScanChannel = &(prAdapter->prGlueInfo->rFullScanApChannel);
	kalMemSet(prPartialScanChannel, 0, sizeof(PARTIAL_SCAN_INFO));

	prChannelScanInfo = prAdapter->prGlueInfo->arChannelScanInfo;
	prChannelScanInfo++;
	while (i < FULL_SCAN_MAX_CHANNEL_NUM) {
		if (prChannelScanInfo->cScanThreshold > 0) {
			ucChannelNum = aisIndex2ChannelNum(i);
			DBGLOG(AIS, TRACE, "Full2Partial i=%d, channel value=%d\n", i, ucChannelNum);
			if ((ucChannelNum != 0) && (t < MAXIMUM_OPERATION_CHANNEL_LIST)) {
				if ((ucChannelNum >= 1) && (ucChannelNum <= 14))
					prPartialScanChannel->arChnlInfoList[t].eBand = BAND_2G4;
				else
					prPartialScanChannel->arChnlInfoList[t].eBand = BAND_5G;

				prPartialScanChannel->arChnlInfoList[t].ucChannelNum = ucChannelNum;
				t++;

				if (t >= MAXIMUM_OPERATION_CHANNEL_LIST) {
					DBGLOG(AIS, INFO, "Full2Partial num %d great %d max channel number\n",
						t, MAXIMUM_OPERATION_CHANNEL_LIST);
					break;
				}
			}
		}
		i++;
		prChannelScanInfo = prChannelScanInfo + 1;
	}
	DBGLOG(AIS, INFO, "Full2Partial channel num=%d\n", t);
	if (t > 0) {
		prPartialScanChannel->ucChannelListNum = t;
		prAdapter->prGlueInfo->pucFullScan2PartialChannel = (PUINT_8)prPartialScanChannel;
	}
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
VOID aisFsmSteps(IN P_ADAPTER_T prAdapter, ENUM_AIS_STATE_T eNextState)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_INFO_T prAisBssInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_DESC_T prBssDesc;
	P_MSG_CH_REQ_T prMsgChReq;
#if defined(MT6631)
	struct _MSG_SCN_SCAN_REQ_V3_T *prScanReqMsg;
#else
	P_MSG_SCN_SCAN_REQ_V2 prScanReqMsg;
#endif
	P_AIS_REQ_HDR_T prAisReq;
	ENUM_BAND_T eBand;
	UINT_8 ucChannel;
	UINT_16 u2ScanIELen;
	UINT_8 ucWFDEnable = 0;
	struct BSS_TRANSITION_MGT_PARAM_T *prBtmParam = &prAdapter->rWifiVar.rAisSpecificBssInfo.rBTMParam;
	P_LINK_T prNeighborAPLink = &prAdapter->rWifiVar.rAisSpecificBssInfo.rNeighborApList.rUsingLink;
	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;
	BOOLEAN fgAdjChnlScanIssued = FALSE;
	UINT_32 u4ScnReqMsgLen = 0;

	DEBUGFUNC("aisFsmSteps()");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	do {

		/* Do entering Next State */
		prAisFsmInfo->ePreviousState = prAisFsmInfo->eCurrentState;

#if DBG
		DBGLOG(AIS, STATE, "TRANSITION: [%s] -> [%s]\n",
				    apucDebugAisState[prAisFsmInfo->eCurrentState], apucDebugAisState[eNextState]);
#else
		DBGLOG(AIS, STATE, "[%d] TRANSITION: [%d] -> [%d]\n",
				    DBG_AIS_IDX, prAisFsmInfo->eCurrentState, eNextState);
#endif
		/* NOTE(Kevin): This is the only place to change the eCurrentState(except initial) */
		prAisFsmInfo->eCurrentState = eNextState;

		fgIsTransition = (BOOLEAN) FALSE;

		aisCheckPostponedDisconnTimeout(prAdapter, prAisFsmInfo);

		/* Do tasks of the State that we just entered */
		switch (prAisFsmInfo->eCurrentState) {
			/* NOTE(Kevin): we don't have to rearrange the sequence of following
			 * switch case. Instead I would like to use a common lookup table of array
			 * of function pointer to speed up state search.
			 */
		case AIS_STATE_IDLE:

			prAisReq = aisFsmGetNextRequest(prAdapter);
			cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);

			if (prAisReq)
				DBGLOG(AIS, TRACE, "eReqType=%d, fgIsConnReqIssued=%d, DisByNonRequest=%d\n",
					prAisReq->eReqType, prConnSettings->fgIsConnReqIssued,
					prConnSettings->fgIsDisconnectedByNonRequest);
			if (prAisReq == NULL || prAisReq->eReqType == AIS_REQUEST_RECONNECT) {

				if (IS_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex)) {
					UNSET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
					DBGLOG(AIS, TRACE, "DEACTIVATE AIS from ACTIVE to Disable\n");
					/* sync with firmware */
					nicDeactivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
				} else {
					DBGLOG(AIS, INFO, "fgIsNetActive %d\n",
					prAdapter->aprBssInfo[prAdapter->prAisBssInfo->ucBssIndex]->fgIsNetActive);
				}

				if (prConnSettings->fgIsConnReqIssued == TRUE &&
				    prConnSettings->fgIsDisconnectedByNonRequest == FALSE) {

					prAisFsmInfo->fgTryScan = TRUE;
					if (!IS_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex)) {
						SET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
						/* sync with firmware */
						nicActivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
					}
					SET_NET_PWR_STATE_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
					prAisBssInfo->fgIsNetRequestInActive = FALSE;

					/* reset trial count */
					prAisFsmInfo->ucConnTrialCount = 0;

					eNextState = AIS_STATE_COLLECT_ESS_INFO;
					fgIsTransition = TRUE;
				} else {
					SET_NET_PWR_STATE_IDLE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

					/* sync with firmware */
#if CFG_SUPPORT_PNO
					prAisBssInfo->fgIsNetRequestInActive = TRUE;
					if (prAisBssInfo->fgIsPNOEnable) {
						SET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
						/* sync with firmware */
						nicActivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

						DBGLOG(BSS, INFO,
						"[BSSidx][Network]=%d PNOEnable&&OP_MODE_INFRASTRUCTURE,KEEP ACTIVE\n",
							prAisBssInfo->ucBssIndex);
					}
#endif
#if CFG_SUPPORT_ABORT_SCAN
					AisFsmHandlePendingScan(prAdapter, prAisReq, &eNextState, &fgIsTransition);
#else
					/* check for other pending request */
					if (prAisReq && (aisFsmIsRequestPending
							 (prAdapter, AIS_REQUEST_SCAN, TRUE) == TRUE)) {
						wlanClearScanningResult(prAdapter);
						eNextState = AIS_STATE_SCAN;
						fgIsTransition = TRUE;
					}
#endif
				}

				if (prAisReq) {
					/* free the message */
					cnmMemFree(prAdapter, prAisReq);
				}
			} else if (prAisReq->eReqType == AIS_REQUEST_SCAN) {
#if CFG_SUPPORT_ROAMING
				prAisFsmInfo->fgIsRoamingScanPending = FALSE;
#endif /* CFG_SUPPORT_ROAMING */
				wlanClearScanningResult(prAdapter);

				eNextState = AIS_STATE_SCAN;
				fgIsTransition = TRUE;

				/* free the message */
				cnmMemFree(prAdapter, prAisReq);
			} else if (prAisReq->eReqType == AIS_REQUEST_ROAMING_CONNECT
				   || prAisReq->eReqType == AIS_REQUEST_ROAMING_SEARCH) {
				/* ignore */
				/* free the message */
				cnmMemFree(prAdapter, prAisReq);
			} else if (prAisReq->eReqType == AIS_REQUEST_REMAIN_ON_CHANNEL) {
				eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;
				fgIsTransition = TRUE;

				/* free the message */
				cnmMemFree(prAdapter, prAisReq);
			}

			prAisFsmInfo->u4SleepInterval = AIS_BG_SCAN_INTERVAL_MIN_SEC;

			break;

		case AIS_STATE_SEARCH:
			/* 4 <1> Search for a matched candidate and save it to prTargetBssDesc.
			 *
			 * ATTENTION: anyone can't leave this case without changing the state,
			 * except BTM, otherwise, may cause BtmResponseTimer's handler run worngly
			 */
#if CFG_SLT_SUPPORT
			prBssDesc = prAdapter->rWifiVar.rSltInfo.prPseudoBssDesc;
#else
			aisCheckNeighborApValidity(prAdapter);
			if (prAisFsmInfo->ucJoinFailCntAfterScan >= 5) {
				prBssDesc = NULL;
				DBGLOG(AIS, WARN,
					"Failed to connect %s more than 5 times after last scan, scan again\n",
					prConnSettings->aucSSID);
			} else
				prBssDesc = scanSearchBssDescByScoreForAis(prAdapter);
#endif
			/* 4 <2> We are not under Roaming Condition. */
			if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED) {
				/* 4 <2.a> If we don't have the matched one */
				if (!prBssDesc) {
					/* increase connection trial count for infrastructure connection */
					if (prConnSettings->eOPMode == NET_TYPE_INFRA)
						prAisFsmInfo->ucConnTrialCount++;
#if CFG_SUPPORT_RN
					if (prAisBssInfo->fgDisConnReassoc == TRUE) {
						eNextState = AIS_STATE_JOIN_FAILURE;
						fgIsTransition = TRUE;
						break;
					}
#endif
					if (prAisFsmInfo->rJoinReqTime != 0 &&
						CHECK_FOR_TIMEOUT(kalGetTimeTick(),
								  prAisFsmInfo->rJoinReqTime,
								  SEC_TO_SYSTIME(AIS_JOIN_TIMEOUT))) {
						eNextState = AIS_STATE_JOIN_FAILURE;
						prAisFsmInfo->prTargetBssDesc = NULL;
						fgIsTransition = TRUE;
						break;
					}

					/* 4 <A> Try to SCAN */
					if (prAisFsmInfo->fgTryScan) {
						eNextState = AIS_STATE_LOOKING_FOR;

						fgIsTransition = TRUE;
					}
					/* 4 <B> We've do SCAN already, now wait in some STATE. */
					else {
						eNextState = aisFsmStateSearchAction(prAdapter,
									AIS_FSM_STATE_SEARCH_ACTION_PHASE_0);
						fgIsTransition = TRUE;
					}
					break;
				}
				/* 4 <2.b> If we have the matched one */
				/* 4 <A> Stored the Selected BSS security cipher. */
				/* or later asoc req compose IE */
				prAisBssInfo->u4RsnSelectedGroupCipher = prBssDesc->u4RsnSelectedGroupCipher;
				prAisBssInfo->u4RsnSelectedPairwiseCipher =
				    prBssDesc->u4RsnSelectedPairwiseCipher;
				prAisBssInfo->u4RsnSelectedAKMSuite = prBssDesc->u4RsnSelectedAKMSuite;

				/* 4 <B> Do STATE transition and update current Operation Mode. */
				if (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE) {
					prAisBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;

					/* Record the target BSS_DESC_T for next STATE. */
					prAisFsmInfo->prTargetBssDesc = prBssDesc;

					/* Transit to channel acquire */
					eNextState = AIS_STATE_REQ_CHANNEL_JOIN;
					fgIsTransition = TRUE;

					/* increase connection trial count */
					prAisFsmInfo->ucConnTrialCount++;

					/* If fgOkcPmksaReady is true, means this's not the first time that
					** we connect to this ESS. For example, we have disconnected with
					** this ESS, and then need to reconnect again.
					*/
					if (prConnSettings->fgOkcPmksaReady)
						aisFsmMayRequstPmkId(prAdapter, prBssDesc);
				}
#if CFG_SUPPORT_ADHOC
				else if (prBssDesc->eBSSType == BSS_TYPE_IBSS) {

					prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

					/* Record the target BSS_DESC_T for next STATE. */
					prAisFsmInfo->prTargetBssDesc = prBssDesc;

					eNextState = AIS_STATE_IBSS_MERGE;
					fgIsTransition = TRUE;
				}
#endif /* CFG_SUPPORT_ADHOC */
				else {
					ASSERT(0);
					eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
					fgIsTransition = TRUE;
				}
				break;
			}
			/* 4 <3> We are under Roaming Condition. */
			if (prAisFsmInfo->ucConnTrialCount > AIS_ROAMING_CONNECTION_TRIAL_LIMIT) {
#if CFG_SUPPORT_ROAMING
				roamingFsmRunEventFail(prAdapter, ROAMING_FAIL_REASON_CONNLIMIT);
#endif /* CFG_SUPPORT_ROAMING */
				/* reset retry count */
				prAisFsmInfo->ucConnTrialCount = 0;

				/* abort connection trial */
				if (prConnSettings->eReConnectLevel < RECONNECT_LEVEL_BEACON_TIMEOUT) {
					prConnSettings->eReConnectLevel = RECONNECT_LEVEL_ROAMING_FAIL;
					prConnSettings->fgIsConnReqIssued = FALSE;
				} else {
					DBGLOG(AIS, INFO,
					       "Do not set fgIsConnReqIssued, level is %d\n",
					       prConnSettings->eReConnectLevel);
				}

				eNextState = AIS_STATE_NORMAL_TR;
				fgIsTransition = TRUE;

				break;
			}
			/* 4 <3.a> This BSS_DESC_T is our AP. */
			/* NOTE(Kevin 2008/05/16): Following cases will go back to NORMAL_TR.
			 * CASE I: During Roaming, APP(WZC/NDISTEST) change the connection
			 *         settings. That make we can NOT match the original AP, so the
			 *         prBssDesc is NULL.
			 * CASE II: The same reason as CASE I. Because APP change the
			 *          eOPMode to other network type in connection setting
			 *          (e.g. NET_TYPE_IBSS), so the BssDesc become the IBSS node.
			 * (For CASE I/II, before WZC/NDISTEST set the OID_SSID, it will change
			 * other parameters in connection setting first. So if we do roaming
			 * at the same time, it will hit these cases.)
			 *
			 * CASE III: Normal case, we can't find other candidate to roam
			 * out, so only the current AP will be matched.
			 *
			 * CASE VI: Timestamp of the current AP might be reset
			 */
			if (prAisBssInfo->ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION &&
				((!prBssDesc) ||	/* CASE I */
				(prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE) ||	/* CASE II */
				(prBssDesc->fgIsConnected) ||	/* CASE III */
				(EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prAisBssInfo->aucBSSID))) /* CASE VI */) {
				if (prBssDesc) {
					DBGLOG(ROAMING, INFO,
					"fgIsConnected=%d, prBssDesc->BSSID %pM, prAisBssInfo->BSSID %pM\n",
					prBssDesc->fgIsConnected, prBssDesc->aucBSSID, prAisBssInfo->aucBSSID);
				}
				if (!prBtmParam->fgUnsolicitedReq && prAisFsmInfo->fgAdjChnlScanIssued) {
					/* if BTM request is solicited and adjacent channel scan has issued,
					** and no roaming target is found, need to do full scan again
					*/
					eNextState = AIS_STATE_LOOKING_FOR;
					fgIsTransition = TRUE;
					break;
				}
#if DBG
				if ((prBssDesc) && (prBssDesc->fgIsConnected))
					ASSERT(EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prAisBssInfo->aucBSSID));
#endif /* DBG */
				/* We already associated with it, go back to NORMAL_TR */
				/* TODO(Kevin): Roaming Fail */
#if CFG_SUPPORT_ROAMING
				roamingFsmRunEventFail(prAdapter, ROAMING_FAIL_REASON_NOCANDIDATE);
#endif /* CFG_SUPPORT_ROAMING */
				/* The case it is a unsolicited btm request or left channel scan has been issued,
				** but still no candidate is found.
				*/
				if (prBtmParam->fgPendingResponse) {
					prBtmParam->ucStatusCode = BSS_TRANSITION_MGT_STATUS_CAND_LIST_PROVIDED;
					prBtmParam->ucTermDelay = 0;
					kalMemZero(prBtmParam->aucTargetBssid, MAC_ADDR_LEN);
					prBtmParam->fgPendingResponse = FALSE;
					wnmSendBTMResponseFrame(prAdapter, prAdapter->prAisBssInfo->prStaRecOfAP);
				}
				/* Retreat to NORMAL_TR state */
				eNextState = AIS_STATE_NORMAL_TR;
				fgIsTransition = TRUE;
				break;
			}
			/* 4 <3.b> Try to roam out for JOIN this BSS_DESC_T. */
			if (!prBssDesc) {
				fgIsTransition = TRUE;
				eNextState = aisFsmStateSearchAction(prAdapter,
						     AIS_FSM_STATE_SEARCH_ACTION_PHASE_1);
				break;
			}
			aisFsmStateSearchAction(prAdapter, AIS_FSM_STATE_SEARCH_ACTION_PHASE_2);
			/* 4 <A> Record the target BSS_DESC_T for next STATE. */
			prAisFsmInfo->prTargetBssDesc = prBssDesc;

			/* tyhsu: increase connection trial count */
			prAisFsmInfo->ucConnTrialCount++;

			/* Reach here representations we're ready to roaming,
			** fgOkcEnabled is used here other than fgOkcPmksaReady.
			** The reason is to consider this is the first time we connected
			** to the ESS, and supplicant didn't set pmkid till now.
			*/
			if (prConnSettings->fgOkcEnabled)
				aisFsmMayRequstPmkId(prAdapter, prBssDesc);

			/* Candidate is found, tell serving BSS. Wait at most 1 second to start roaming */
			if (prBtmParam->fgPendingResponse) {
				prBtmParam->ucStatusCode = BSS_TRANSITION_MGT_STATUS_ACCEPT;
				prBtmParam->ucTermDelay = 0;
				COPY_MAC_ADDR(prBtmParam->aucTargetBssid, prBssDesc->aucBSSID);
				wnmSendBTMResponseFrame(prAdapter, prAdapter->prAisBssInfo->prStaRecOfAP);
				cnmTimerStartTimer(prAdapter,
						&prAdapter->rWifiVar.rAisSpecificBssInfo.rBtmResponseTimer, 1000);
				fgIsTransition = FALSE;
				break;
			}
			/* Transit to channel acquire */
			eNextState = AIS_STATE_REQ_CHANNEL_JOIN;
			fgIsTransition = TRUE;
			break;

		case AIS_STATE_WAIT_FOR_NEXT_SCAN:

			DBGLOG(AIS, LOUD, "SCAN: Idle Begin - Current Time = %u\n", kalGetTimeTick());

			cnmTimerStartTimer(prAdapter,
					   &prAisFsmInfo->rBGScanTimer, SEC_TO_MSEC(prAisFsmInfo->u4SleepInterval));

			SET_NET_PWR_STATE_IDLE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

			if (prAisFsmInfo->u4SleepInterval < AIS_BG_SCAN_INTERVAL_MAX_SEC)
				prAisFsmInfo->u4SleepInterval <<= 1;

			break;

		case AIS_STATE_SCAN:
		case AIS_STATE_ONLINE_SCAN:
		case AIS_STATE_LOOKING_FOR:

			if (!IS_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex)) {
				SET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

				/* sync with firmware */
				nicActivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
				prAisBssInfo->fgIsNetRequestInActive = FALSE;
			}

			/* IE length decision */
			if (prAisFsmInfo->u4ScanIELength > 0) {
				u2ScanIELen = (UINT_16) prAisFsmInfo->u4ScanIELength;
			} else {
#if CFG_SUPPORT_WPS2
				u2ScanIELen = prAdapter->prGlueInfo->u2WSCIELen;
#else
				u2ScanIELen = 0;
#endif
			}
#if defined(MT6631)
			u4ScnReqMsgLen = OFFSET_OF(struct _MSG_SCN_SCAN_REQ_V3_T, aucIE) + u2ScanIELen;
			prScanReqMsg = (struct _MSG_SCN_SCAN_REQ_V3_T *) cnmMemAlloc(prAdapter,
							   RAM_TYPE_MSG,
							   u4ScnReqMsgLen);

#else
			u4ScnReqMsgLen = OFFSET_OF(MSG_SCN_SCAN_REQ_V2, aucIE) + u2ScanIELen;
			prScanReqMsg = (P_MSG_SCN_SCAN_REQ_V2) cnmMemAlloc(prAdapter,
							   RAM_TYPE_MSG,
							   u4ScnReqMsgLen);

#endif

			if (!prScanReqMsg) {
				ASSERT(0);	/* Can't trigger SCAN FSM */
				return;
			}
			kalMemZero(prScanReqMsg, u4ScnReqMsgLen);
#if defined(MT6631)
			prScanReqMsg->rMsgHdr.eMsgId = MID_AIS_SCN_SCAN_REQ_V3;
#else
			prScanReqMsg->rMsgHdr.eMsgId = MID_AIS_SCN_SCAN_REQ_V2;
#endif
			prScanReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfScanReq;
			prScanReqMsg->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
			if (rlmFillScanMsg(prAdapter, (struct _MSG_SCN_SCAN_REQ_V3_T *)prScanReqMsg)) {
				mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prScanReqMsg, MSG_SEND_METHOD_BUF);
				break;
			}

			COPY_MAC_ADDR(prScanReqMsg->aucBSSID, "\xff\xff\xff\xff\xff\xff");

#if CFG_SUPPORT_RDD_TEST_MODE
			prScanReqMsg->eScanType = SCAN_TYPE_PASSIVE_SCAN;
#else
			if (prAisFsmInfo->eCurrentState == AIS_STATE_SCAN
			    || prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN) {
				if (prAisFsmInfo->ucScanSSIDNum == 0) {
#if CFG_SUPPORT_AIS_PASSIVE_SCAN
					prScanReqMsg->eScanType = SCAN_TYPE_PASSIVE_SCAN;

					prScanReqMsg->ucSSIDType = 0;
					prScanReqMsg->ucSSIDNum = 0;
#else
					prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

					prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_WILDCARD;
					prScanReqMsg->ucSSIDNum = 0;
#endif
				} else if (prAisFsmInfo->ucScanSSIDNum == 1
					   && prAisFsmInfo->arScanSSID[0].u4SsidLen == 0) {
					prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

					prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_WILDCARD;
					prScanReqMsg->ucSSIDNum = 0;
				} else {
					prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

					prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_SPECIFIED;
					prScanReqMsg->ucSSIDNum = prAisFsmInfo->ucScanSSIDNum;
					prScanReqMsg->prSsid = prAisFsmInfo->arScanSSID;
				}
#if defined(MT6631)
				kalMemCopy(prScanReqMsg->aucRandomMac, &(prAisFsmInfo->rScanRandMacAddr.aucRandomMac),
							MAC_ADDR_LEN);
				prScanReqMsg->ucScnFuncMask = prAisFsmInfo->rScanRandMacAddr.ucScnFuncMask;
#endif
			} else {
				prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

				COPY_SSID(prAisFsmInfo->rRoamingSSID.aucSsid,
					  prAisFsmInfo->rRoamingSSID.u4SsidLen,
					  prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

				/* Scan for determined SSID */
				prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_SPECIFIED;
				prScanReqMsg->ucSSIDNum = 1;
				prScanReqMsg->prSsid = &(prAisFsmInfo->rRoamingSSID);
			}
#endif

			/* using default channel dwell time/timeout value */
			prScanReqMsg->u2ProbeDelay = 0;
			prScanReqMsg->u2TimeoutValue = 0;
			prScanReqMsg->u2ChannelDwellTime = 0;
			prScanReqMsg->u2MinChannelDwellTime = 0;
			prScanReqMsg->ucChannelListNum = 0;
			if (prAisFsmInfo->fgAdjChnlScanIssued) {
				fgAdjChnlScanIssued = TRUE;
				prAisFsmInfo->fgAdjChnlScanIssued = FALSE;
			} else
				aisCheckNeighborApValidity(prAdapter);
			/* Collect scan channel or excluded scan channel */
			if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR && !LINK_IS_EMPTY(prNeighborAPLink)) {
				struct NEIGHBOR_AP_T *prNeiAP = NULL;
				P_RF_CHANNEL_INFO_T prChnlInfo = &prScanReqMsg->arChnlInfoList[0];
				UINT_8 ucChnlNum = 0;
				UINT_8 i = 0;

				/* Add the connection channel */
				if (prAisFsmInfo->prTargetBssDesc) {
					ucChannel = prAisFsmInfo->prTargetBssDesc->ucChannelNum;
					eBand = ucChannel <= 14 ? BAND_2G4:BAND_5G;
					if (rlmDomainIsLegalChannel(prAdapter, eBand, ucChannel) &&
						ucChnlNum != MAXIMUM_OPERATION_CHANNEL_LIST) {
						prChnlInfo[i].eBand = eBand;
						prChnlInfo[i].ucChannelNum = ucChannel;
						ucChnlNum++;
					}
				}
				LINK_FOR_EACH_ENTRY(prNeiAP, prNeighborAPLink, rLinkEntry, struct NEIGHBOR_AP_T) {
					ucChannel = prNeiAP->ucChannel;
					eBand = ucChannel <= 14 ? BAND_2G4:BAND_5G;
					if (!rlmDomainIsLegalChannel(prAdapter, eBand, ucChannel))
						continue;
					for (i = 0; i < ucChnlNum && ucChannel != prChnlInfo[i].ucChannelNum; i++)
						;
					if (i != ucChnlNum)
						continue;
					prChnlInfo[ucChnlNum].eBand = eBand;
					prChnlInfo[ucChnlNum].ucChannelNum = ucChannel;
					ucChnlNum++;
					if (ucChnlNum == MAXIMUM_OPERATION_CHANNEL_LIST)
						break;
				}
				prScanReqMsg->ucChannelListNum = ucChnlNum;
			}
			/* check if tethering is running and need to fix on specific channel */
			if (cnmAisInfraChannelFixed(prAdapter, &eBand, &ucChannel) == TRUE) {
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
				prScanReqMsg->ucChannelListNum = 1;
				prScanReqMsg->arChnlInfoList[0].eBand = eBand;
				prScanReqMsg->arChnlInfoList[0].ucChannelNum = ucChannel;
			} else if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR &&
				   !fgAdjChnlScanIssued && prScanReqMsg->ucChannelListNum > 0) {
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
				prAisFsmInfo->fgAdjChnlScanIssued = TRUE;
				/* Less than 10 channels, increase dwell time to find more APs */
				if (prScanReqMsg->ucChannelListNum < 10)
					prScanReqMsg->u2ChannelDwellTime = 100;
			} else if ((prAisFsmInfo->eCurrentState != AIS_STATE_LOOKING_FOR) &&
				(prAdapter->prGlueInfo != NULL) &&
				(prAdapter->prGlueInfo->pucScanChannel != NULL)) {
				/* handle partial scan channel info */
				P_PARTIAL_SCAN_INFO channel_t;
				UINT_32	u4size;

				channel_t = (P_PARTIAL_SCAN_INFO)prAdapter->prGlueInfo->pucScanChannel;

				/* set partial scan */
				prScanReqMsg->ucChannelListNum = channel_t->ucChannelListNum;
				u4size = sizeof(channel_t->arChnlInfoList);

				DBGLOG(AIS, TRACE,
					"Partial Scan: ucChannelListNum=%d, total size=%d\n",
					prScanReqMsg->ucChannelListNum, u4size);

				kalMemCopy(&(prScanReqMsg->arChnlInfoList), &(channel_t->arChnlInfoList),
					u4size);

				/* clear prGlueInfo partial scan info */
				kalMemSet(channel_t, 0, sizeof(PARTIAL_SCAN_INFO));
				prAdapter->prGlueInfo->pucScanChannel = NULL;
				DBGLOG(AIS, TRACE, "partial scan clear and reset pucScanChannel\n");


				/* set scan channel type for partial scan */
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
			} else if (prAdapter->aePreferBand[prAdapter->prAisBssInfo->ucBssIndex] == BAND_NULL) {
				if (prAdapter->fgEnable5GBand == TRUE)
					prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
				else
					prScanReqMsg->eScanChannel = SCAN_CHANNEL_2G4;
			} else if (prAdapter->aePreferBand[prAdapter->prAisBssInfo->ucBssIndex]
					== BAND_2G4) {
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_2G4;
			} else if (prAdapter->aePreferBand[prAdapter->prAisBssInfo->ucBssIndex]
					== BAND_5G) {
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_5G;
			} else {
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
				ASSERT(0);
			}

#if CFG_SUPPORT_WFD
#if CFG_ENABLE_WIFI_DIRECT

			{
				/* Check WFD is running */
				P_WFD_CFG_SETTINGS_T prWfdCfgSettings = (P_WFD_CFG_SETTINGS_T) NULL;

				prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);
				ucWFDEnable = prWfdCfgSettings->ucWfdEnable;
				DBGLOG(AIS, INFO, "Full2Partial ucWFDEnable=%u\n", ucWFDEnable);
			}
#endif
#endif

			/*Full2Partial at here, chech sould update full scan to partial scan or not*/
			if (((prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN) ||
				((ucWFDEnable) && (prAisFsmInfo->eCurrentState != AIS_STATE_LOOKING_FOR)))
				&& (prScanReqMsg->eScanChannel == SCAN_CHANNEL_FULL) &&
				(prScanReqMsg->ucSSIDType == SCAN_REQ_SSID_WILDCARD) &&
				(prScanReqMsg->ucSSIDNum == 0)) {
				/*this is a full scan*/
				OS_SYSTIME rCurrentTime;
				P_PARTIAL_SCAN_INFO channel_t;
				P_GLUE_INFO_T pGlinfo;
				UINT_32 u4size;

				DBGLOG(AIS, TRACE, "Full2Partial eScanChannel = %d, ucSSIDNum=%d\n",
					prScanReqMsg->eScanChannel, prScanReqMsg->ucSSIDNum);
				pGlinfo = prAdapter->prGlueInfo;
				GET_CURRENT_SYSTIME(&rCurrentTime);
				DBGLOG(AIS, TRACE, "Full2Partial LastFullST= %u,CurrentT=%u\n",
					pGlinfo->u4LastFullScanTime, rCurrentTime);
				if ((!ucWFDEnable) && ((pGlinfo->u4LastFullScanTime == 0) ||
					(CHECK_FOR_TIMEOUT(rCurrentTime, pGlinfo->u4LastFullScanTime,
						SEC_TO_SYSTIME(UPDATE_FULL_TO_PARTIAL_SCAN_TIMEOUT))))) {
					/*first full scan during connected*/
					/*or time over 60s from last full scan*/
					DBGLOG(AIS, INFO, "Full2Partial not update full scan\n");
					pGlinfo->u4LastFullScanTime = rCurrentTime;
				} else {
					DBGLOG(AIS, INFO, "Full2Partial update full scan to partial scan\n");

					/*at here, we should update full scan to partial scan*/
					pGlinfo->pucFullScan2PartialChannel = NULL;
					aisGetAndSetScanChannel(prAdapter);

					if (pGlinfo->pucFullScan2PartialChannel != NULL) {
						PUINT_8 pChanneltmp;
						/* update full scan to partial scan */
						pChanneltmp = pGlinfo->pucFullScan2PartialChannel;
						channel_t = (P_PARTIAL_SCAN_INFO)pChanneltmp;

						/* set partial scan */
						prScanReqMsg->ucChannelListNum = channel_t->ucChannelListNum;
						u4size = sizeof(channel_t->arChnlInfoList);

						DBGLOG(AIS, TRACE, "Full2Partial ChList=%d,u4size=%u\n",
							channel_t->ucChannelListNum, u4size);

						kalMemCopy(&(prScanReqMsg->arChnlInfoList),
							&(channel_t->arChnlInfoList), u4size);
						/* set scan channel type for partial scan */
						prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
					}
				}
			}

			if (prAisFsmInfo->u4ScanIELength > 0) {
				kalMemCopy(prScanReqMsg->aucIE, prAisFsmInfo->aucScanIEBuf,
					   prAisFsmInfo->u4ScanIELength);
			} else {
#if CFG_SUPPORT_WPS2
				if (prAdapter->prGlueInfo->u2WSCIELen > 0) {
					kalMemCopy(prScanReqMsg->aucIE,
						   &prAdapter->prGlueInfo->aucWSCIE, prAdapter->prGlueInfo->u2WSCIELen);
				}
			}
#endif

			prScanReqMsg->u2IELen = u2ScanIELen;

			mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prScanReqMsg, MSG_SEND_METHOD_BUF);
			prAisFsmInfo->fgTryScan = FALSE;	/* Will enable background sleep for infrastructure */
			prAisFsmInfo->ucJoinFailCntAfterScan = 0;
			kalMemZero(&(prAisFsmInfo->rScanRandMacAddr),
						sizeof(struct _PARAM_SCAN_RANDOM_MAC_ADDR_T));
#if CFG_SUPPORT_ABORT_SCAN
			AisFsmSetScanState(prAdapter, TRUE);
#endif
			if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR &&
			    prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED)
				glNotifyDrvStatus(ROAMING_SCAN_START, prScanReqMsg);
			aisPreScan(prAdapter, prScanReqMsg);
			break;

		case AIS_STATE_REQ_CHANNEL_JOIN:
			/* stop Tx due to we need to connect a new AP. even the new
			** AP is operating on the same channel with current, we still
			** need to stop Tx, because firmware should ensure all mgmt and
			** dhcp packets are Tx in time, and may cause normal data
			** packets was queued and eventually flushed in firmware
			*/
			if (prAisBssInfo->prStaRecOfAP &&
			    prAisBssInfo->ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION)
				prAdapter->prAisBssInfo->prStaRecOfAP->fgIsTxAllowed = FALSE;
			prAisFsmInfo->fgAdjChnlScanIssued = FALSE;
			/* send message to CNM for acquiring channel */
			prMsgChReq = (P_MSG_CH_REQ_T) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_CH_REQ_T));
			if (!prMsgChReq) {
				ASSERT(0);	/* Can't indicate CNM for channel acquiring */
				return;
			}

			if (prAisFsmInfo->prTargetBssDesc == NULL)
				break;

			prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
			prMsgChReq->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
			prMsgChReq->ucTokenID = ++prAisFsmInfo->ucSeqNumOfChReq;
			prMsgChReq->eReqType = CH_REQ_TYPE_JOIN;
			prMsgChReq->u4MaxInterval = AIS_JOIN_CH_REQUEST_INTERVAL;
			prMsgChReq->ucPrimaryChannel = prAisFsmInfo->prTargetBssDesc->ucChannelNum;
			prMsgChReq->eRfSco = prAisFsmInfo->prTargetBssDesc->eSco;
			prMsgChReq->eRfBand = prAisFsmInfo->prTargetBssDesc->eBand;

			/* To do: check if 80/160MHz bandwidth is needed here */
			prMsgChReq->eRfChannelWidth = prAisFsmInfo->prTargetBssDesc->eChannelWidth;
			prMsgChReq->ucRfCenterFreqSeg1 = prAisFsmInfo->prTargetBssDesc->ucCenterFreqS1;
			prMsgChReq->ucRfCenterFreqSeg2 = prAisFsmInfo->prTargetBssDesc->ucCenterFreqS2;

			mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prMsgChReq, MSG_SEND_METHOD_BUF);

			prAisFsmInfo->fgIsChannelRequested = TRUE;
			break;

		case AIS_STATE_JOIN:
			aisFsmStateInit_JOIN(prAdapter, prAisFsmInfo->prTargetBssDesc);
			break;

		case AIS_STATE_JOIN_FAILURE:
			prAdapter->rWifiVar.rConnSettings.eReConnectLevel = RECONNECT_LEVEL_MIN;
			prConnSettings->fgIsDisconnectedByNonRequest = TRUE;
			prConnSettings->ucSSIDLen = 0;
#if CFG_SUPPORT_REPORT_MISC
			if (prAdapter->rReportMiscSet.eQueryNum == REPORT_AUTHASSOC_START) {
				wlanSendSetQueryCmd(prAdapter, CMD_ID_GET_REPORT_MISC,
						    FALSE,
						    TRUE,
						    FALSE,
						    nicCmdEventReportMisc,
						    NULL,
						    0,
						    NULL,
						    NULL,
						    0);
				prAdapter->rReportMiscSet.eQueryNum = REPORT_AUTHASSOC_END;
			}
#endif
#if CFG_SUPPORT_RN
			if (prAisBssInfo->fgDisConnReassoc == TRUE) {
				nicMediaJoinFailure(prAdapter, prAdapter->prAisBssInfo->ucBssIndex,
							WLAN_STATUS_MEDIA_DISCONNECT);
				prAisBssInfo->fgDisConnReassoc = FALSE;
			} else
#endif
			{
				nicMediaJoinFailure(prAdapter, prAdapter->prAisBssInfo->ucBssIndex,
					WLAN_STATUS_JOIN_FAILURE);
			}
			eNextState = AIS_STATE_IDLE;
			fgIsTransition = TRUE;

			break;

#if CFG_SUPPORT_ADHOC
		case AIS_STATE_IBSS_ALONE:
			aisFsmStateInit_IBSS_ALONE(prAdapter);
			break;

		case AIS_STATE_IBSS_MERGE:
			aisFsmStateInit_IBSS_MERGE(prAdapter, prAisFsmInfo->prTargetBssDesc);
			break;
#endif /* CFG_SUPPORT_ADHOC */

		case AIS_STATE_NORMAL_TR:
			if (prAisFsmInfo->fgIsInfraChannelFinished == FALSE) {
				/* Don't do anything when rJoinTimeoutTimer is still ticking */
#if CFG_SUPPORT_ABORT_SCAN
				DBGLOG(AIS, INFO, "Don't do anything when rJoinTimeoutTimer is still ticking");
				if (prAdapter->prGlueInfo &&
				    prAdapter->prGlueInfo->prScanRequest == NULL &&
				    aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, TRUE) == TRUE)
					DBGLOG(AIS, INFO, "Pending scan request, removed\n");
#endif
			} else {
				/* 1. Process for pending roaming scan */
				if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, TRUE) == TRUE) {
					eNextState = AIS_STATE_LOOKING_FOR;
					fgIsTransition = TRUE;
				}
				/* 2. Process for pending roaming connect */
				else if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, TRUE) == TRUE) {
					eNextState = AIS_STATE_COLLECT_ESS_INFO;
					fgIsTransition = TRUE;
				}
				/* 3. Process for pending scan */
				else if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, TRUE) == TRUE) {
#if CFG_SUPPORT_ABORT_SCAN
					if (prAdapter->fgAbortScan) {
						DBGLOG(INIT, INFO,
						       "Remove pending scan, no need insert new scan req\n");
						AisFsmGenerateScanDoneMsg(prAdapter, SCAN_STATUS_CANCELLED);
						prAdapter->fgAbortScan = FALSE;
						eNextState = AIS_STATE_NORMAL_TR;
					} else {
						wlanClearScanningResult(prAdapter);
						eNextState = AIS_STATE_ONLINE_SCAN;
					}
#else
					wlanClearScanningResult(prAdapter);
					eNextState = AIS_STATE_ONLINE_SCAN;
#endif
					fgIsTransition = TRUE;
				}
				/* 4. Process for pending remain on channel*/
				else if (aisFsmIsRequestPending
					(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL, TRUE) == TRUE) {
					eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;
					fgIsTransition = TRUE;
				}
			}

			break;

		case AIS_STATE_DISCONNECTING:
			/* send for deauth frame for disconnection */
			authSendDeauthFrame(prAdapter,
					    prAisBssInfo,
					    prAisBssInfo->prStaRecOfAP,
					    (P_SW_RFB_T) NULL, REASON_CODE_DEAUTH_LEAVING_BSS, aisDeauthXmitComplete);
			/* If it is scanning or BSS absent, HW may go away from serving channel, which may cause driver
			** be not able to TX mgmt frame. So we need to start a longer timer to wait HW return to serving
			** channel. We set the time out value to 1 second because it is long enough to return to serving
			** channel in most cases, and disconnection delay is seamless to end-user even time out.
			*/
			cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rDeauthDoneTimer,
					   (prAisFsmInfo->fgIsScanning || prAisBssInfo->fgIsNetAbsent) ? 1000 : 100);
			break;

		case AIS_STATE_REQ_REMAIN_ON_CHANNEL:
			/* send message to CNM for acquiring channel */
			prMsgChReq = (P_MSG_CH_REQ_T) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_CH_REQ_T));
			if (!prMsgChReq) {
				ASSERT(0);	/* Can't indicate CNM for channel acquiring */
				return;
			}

			/* release channel */
			aisFsmReleaseCh(prAdapter);

			/* zero-ize */
			kalMemZero(prMsgChReq, sizeof(MSG_CH_REQ_T));

			/* filling */
			prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
			prMsgChReq->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
			prMsgChReq->ucTokenID = ++prAisFsmInfo->ucSeqNumOfChReq;
			prMsgChReq->eReqType = CH_REQ_TYPE_JOIN;
			prMsgChReq->u4MaxInterval = prAisFsmInfo->rChReqInfo.u4DurationMs;
			prMsgChReq->ucPrimaryChannel = prAisFsmInfo->rChReqInfo.ucChannelNum;
			prMsgChReq->eRfSco = prAisFsmInfo->rChReqInfo.eSco;
			prMsgChReq->eRfBand = prAisFsmInfo->rChReqInfo.eBand;

			mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prMsgChReq, MSG_SEND_METHOD_BUF);

			prAisFsmInfo->fgIsChannelRequested = TRUE;

			break;

		case AIS_STATE_REMAIN_ON_CHANNEL:
			if (!IS_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex)) {
				SET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

				/* sync with firmware */
				nicActivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
			}
			prAisBssInfo->fgIsNetRequestInActive = FALSE;
			break;
		case AIS_STATE_COLLECT_ESS_INFO:
		{
#if 0 /* disable channel utilization now */
			UINT_8 i = 0;
			P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;
			struct MSG_REQ_CH_UTIL *prMsgReqChUtil = NULL;

			/* don't request channel utilization if user asked to connect a specific bss */
			if (prConnSettings->eConnectionPolicy == CONNECT_BY_BSSID) {
				eNextState = AIS_STATE_SEARCH;
				fgIsTransition = TRUE;
				break;
			}
			prMsgReqChUtil = (struct MSG_REQ_CH_UTIL *)
				cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_REQ_CH_UTIL));
			if (!prMsgReqChUtil) {
				DBGLOG(AIS, ERROR, "No memory!");
				return;
			}
			kalMemZero(prMsgReqChUtil, sizeof(*prMsgReqChUtil));
			prMsgReqChUtil->rMsgHdr.eMsgId = MID_MNY_CNM_REQ_CH_UTIL;
			prMsgReqChUtil->u2ReturnMID = MID_CNM_AIS_RSP_CH_UTIL;
			prMsgReqChUtil->u2Duration = 100; /* 100ms */
			prMsgReqChUtil->ucChnlNum = prAisSpecBssInfo->ucCurEssChnlInfoNum;
			for (; i < prMsgReqChUtil->ucChnlNum && i < sizeof(prMsgReqChUtil->aucChnlList); i++)
				prMsgReqChUtil->aucChnlList[i] = prAisSpecBssInfo->arCurEssChnlInfo[i].ucChannel;

			mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prMsgReqChUtil, MSG_SEND_METHOD_BUF);
#else
			eNextState = AIS_STATE_SEARCH;
			fgIsTransition = TRUE;
#endif
			break;
		}
		default:
			ASSERT(0);	/* Make sure we have handle all STATEs */
			break;

		}
	} while (fgIsTransition);

	return;

}				/* end of aisFsmSteps() */

enum _ENUM_AIS_STATE_T aisFsmStateSearchAction(IN struct _ADAPTER_T *prAdapter, UINT_8 ucPhase)
{
	struct _CONNECTION_SETTINGS_T *prConnSettings;
	struct _BSS_INFO_T *prAisBssInfo;
	struct _AIS_FSM_INFO_T *prAisFsmInfo;
#if CFG_SLT_SUPPORT
	struct _BSS_DESC_T *prBssDesc;
#endif
	enum _ENUM_AIS_STATE_T eState = AIS_STATE_IDLE;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

#if CFG_SLT_SUPPORT
	prBssDesc = prAdapter->rWifiVar.rSltInfo.prPseudoBssDesc;
#endif

	if (ucPhase == AIS_FSM_STATE_SEARCH_ACTION_PHASE_0) {
		if (prConnSettings->eOPMode == NET_TYPE_INFRA) {

			/* issue reconnect request, */
			/*and retreat to idle state for scheduling */
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);
			eState = AIS_STATE_IDLE;
		}
#if CFG_SUPPORT_ADHOC
		else if ((prConnSettings->eOPMode == NET_TYPE_IBSS)
			 || (prConnSettings->eOPMode == NET_TYPE_AUTO_SWITCH)
			 || (prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS)) {
			prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;
			prAisFsmInfo->prTargetBssDesc = NULL;
			eState = AIS_STATE_IBSS_ALONE;
		}
#endif /* CFG_SUPPORT_ADHOC */
		else {
			ASSERT(0);
			eState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
		}
	} else if (ucPhase == AIS_FSM_STATE_SEARCH_ACTION_PHASE_1) {
		/* increase connection trial count for infrastructure connection */
		if (prConnSettings->eOPMode == NET_TYPE_INFRA)
			prAisFsmInfo->ucConnTrialCount++;
		/* 4 <A> Try to SCAN */
		if (prAisFsmInfo->fgTryScan)
			eState = AIS_STATE_LOOKING_FOR;

		/* 4 <B> We've do SCAN already, now wait in some STATE. */
		else {
			if (prConnSettings->eOPMode == NET_TYPE_INFRA) {

				/* issue reconnect request, and */
				/* retreat to idle state for scheduling */
				aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);

				eState = AIS_STATE_IDLE;
			}
#if CFG_SUPPORT_ADHOC
			else if ((prConnSettings->eOPMode == NET_TYPE_IBSS)
				 || (prConnSettings->eOPMode == NET_TYPE_AUTO_SWITCH)
				 || (prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS)) {

				prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;
				prAisFsmInfo->prTargetBssDesc = NULL;

				eState = AIS_STATE_IBSS_ALONE;
			}
#endif /* CFG_SUPPORT_ADHOC */
			else {
				ASSERT(0);
				eState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
			}
		}
	} else {
#if DBG
		if (prAisBssInfo->ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION)
			ASSERT(UNEQUAL_MAC_ADDR(prBssDesc->aucBSSID, prAisBssInfo->aucBSSID));
#endif /* DBG */
	}
	return eState;
}
#if 0
/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmSetChannelInfo(IN P_ADAPTER_T prAdapter, IN P_MSG_SCN_SCAN_REQ_V2 ScanReqMsg,
			IN ENUM_AIS_STATE_T CurrentState)

{
	/*get scan channel infro from prAdapter->prGlueInfo->prScanRequest*/
	struct cfg80211_scan_request *scan_req_t = NULL;
	struct ieee80211_channel *channel_tmp = NULL;
	int i = 0;
	int j = 0;
	UINT_8 channel_num = 0;
	UINT_8 channel_counts = 0;
	UINT_8 BssIndex_t = 0;

	if ((prAdapter == NULL) || (ScanReqMsg == NULL))
		return;
	if ((CurrentState == AIS_STATE_SCAN) || (CurrentState == AIS_STATE_ONLINE_SCAN)) {
		if (prAdapter->prGlueInfo->prScanRequest != NULL) {
			scan_req_t = prAdapter->prGlueInfo->prScanRequest;
			if ((scan_req_t != NULL) && (scan_req_t->n_channels != 0) &&
				(scan_req_t->channels != NULL)) {
				channel_counts = scan_req_t->n_channels;
				DBGLOG(AIS, TRACE, "channel_counts=%d\n", channel_counts);

				while (j < channel_counts) {
					channel_tmp = scan_req_t->channels[j];
					if (channel_tmp == NULL)
						break;

					DBGLOG(AIS, TRACE, "set channel band=%d\n", channel_tmp->band);
					if (channel_tmp->band >= IEEE80211_BAND_60GHZ) {
						j++;
						continue;
					}
					if (i >= MAXIMUM_OPERATION_CHANNEL_LIST)
						break;
					if (channel_tmp->band == IEEE80211_BAND_2GHZ)
						ScanReqMsg->arChnlInfoList[i].eBand = BAND_2G4;
					else if (channel_tmp->band == IEEE80211_BAND_5GHZ)
						ScanReqMsg->arChnlInfoList[i].eBand = BAND_5G;

					DBGLOG(AIS, TRACE, "set channel channel_rer =%d\n",
						channel_tmp->center_freq);

					channel_num = (UINT_8)nicFreq2ChannelNum(
						channel_tmp->center_freq * 1000);

					DBGLOG(AIS, TRACE, "set channel channel_num=%d\n",
						channel_num);
					ScanReqMsg->arChnlInfoList[i].ucChannelNum = channel_num;

					j++;
					i++;
				}
			}

		}
	}

	DBGLOG(AIS, INFO, "set channel i=%d\n", i);
	if (i > 0) {
		ScanReqMsg->ucChannelListNum = i;
		ScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;

		return;
	}

	BssIndex_t = prAdapter->prAisBssInfo->ucBssIndex;
	if (prAdapter->aePreferBand[BssIndex_t] == BAND_NULL) {
		if (prAdapter->fgEnable5GBand == TRUE)
			ScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
		else
			ScanReqMsg->eScanChannel = SCAN_CHANNEL_2G4;
		} else if (prAdapter->aePreferBand[BssIndex_t]
				== BAND_2G4) {
			ScanReqMsg->eScanChannel = SCAN_CHANNEL_2G4;
		} else if (prAdapter->aePreferBand[BssIndex_t]
				== BAND_5G) {
			ScanReqMsg->eScanChannel = SCAN_CHANNEL_5G;
		} else {
			ScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
			ASSERT(0);
		}

}
#endif

#ifdef CFG_SUPPORT_ABORT_SCAN
VOID AisFsmHandlePendingScan(IN P_ADAPTER_T prAdapter,
			     IN P_AIS_REQ_HDR_T prAisReq,
			     OUT ENUM_AIS_STATE_T *peNextState,
			     OUT PBOOLEAN pfgIsTransition)
{
	if (!prAisReq ||
	    (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, FALSE) == FALSE))
		return;

	if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, TRUE) == FALSE)
		DBGLOG(AIS, WARN, "Not find AIS_REQUEST_SCAN from pending req list\n");
	if (prAdapter->fgAbortScan) {
		DBGLOG(AIS, INFO,
		       "Remove pending scan, no need insert new scan req\n");
		AisFsmGenerateScanDoneMsg(prAdapter, SCAN_STATUS_CANCELLED);
		prAdapter->fgAbortScan = FALSE;
		*peNextState = AIS_STATE_IDLE;
	} else {
		wlanClearScanningResult(prAdapter);
		*peNextState = AIS_STATE_SCAN;
	}
	*pfgIsTransition = TRUE;
}


VOID AisFsmGenerateScanDoneMsg(IN P_ADAPTER_T prAdapter, IN ENUM_SCAN_STATUS eScanStatus)
{
	P_MSG_SCN_SCAN_DONE prScanDoneMsg;
	P_AIS_FSM_INFO_T prAisFsmInfo;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	prScanDoneMsg = (P_MSG_SCN_SCAN_DONE) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_SCN_SCAN_DONE));
	if (!prScanDoneMsg) {
		ASSERT(0);	/* Can't indicate SCAN FSM Complete */
		return;
	}

	prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_AIS_SCAN_DONE;
	prScanDoneMsg->ucSeqNum = prAisFsmInfo->ucSeqNumOfScanReq;
	prScanDoneMsg->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
	prScanDoneMsg->eScanStatus = eScanStatus;

	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prScanDoneMsg, MSG_SEND_METHOD_BUF);
}				/* end of AisFsmGenerateScanDoneMsg() */

BOOLEAN AisFsmGetScanState(IN P_ADAPTER_T prAdapter)
{

	P_AIS_FSM_INFO_T prAisFsmInfo;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	return prAisFsmInfo->fgIsScanning;
}

VOID AisFsmSetScanState(IN P_ADAPTER_T prAdapter, IN BOOLEAN bFlag)
{

	P_AIS_FSM_INFO_T prAisFsmInfo;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisFsmInfo->fgIsScanning = bFlag;
}

#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmRunEventScanDone(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_SCN_SCAN_DONE prScanDoneMsg;
	P_AIS_FSM_INFO_T prAisFsmInfo;
	ENUM_AIS_STATE_T eNextState;
	UINT_8 ucSeqNumOfCompMsg;
	P_CONNECTION_SETTINGS_T prConnSettings;
	struct RADIO_MEASUREMENT_REQ_PARAMS *prRmReq = &prAdapter->rWifiVar.rRmReqParams;
	struct BCN_RM_PARAMS *prBcnRmParam = &prRmReq->rBcnRmParam;
	struct cfg80211_scan_request *prScanRequest;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
#if CFG_SUPPORT_ABORT_SCAN
	ENUM_SCAN_STATUS eScanStatus = SCAN_STATUS_DONE;
#endif

	DEBUGFUNC("aisFsmRunEventScanDone()");

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	DBGLOG(AIS, LOUD, "EVENT-SCAN DONE: Current Time = %u\n", kalGetTimeTick());

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prScanRequest = prAdapter->prGlueInfo->prScanRequest;
	prScanDoneMsg = (P_MSG_SCN_SCAN_DONE) prMsgHdr;
	ASSERT(prScanDoneMsg->ucBssIndex == prAdapter->prAisBssInfo->ucBssIndex);

	ucSeqNumOfCompMsg = prScanDoneMsg->ucSeqNum;
#if CFG_SUPPORT_ABORT_SCAN
	eScanStatus = prScanDoneMsg->eScanStatus;
#endif
	cnmMemFree(prAdapter, prMsgHdr);

	DBGLOG(AIS, INFO, "AIS[%u] ScanDone[SN:%u status:%d] ScanReq[SN:%u Req:%u]\n",
	       prAisFsmInfo->eCurrentState,
	       ucSeqNumOfCompMsg, eScanStatus,
	       prAisFsmInfo->ucSeqNumOfScanReq,
	       prScanRequest ? TRUE : FALSE);

	eNextState = prAisFsmInfo->eCurrentState;

	if (ucSeqNumOfCompMsg != prAisFsmInfo->ucSeqNumOfScanReq) {
		DBGLOG(AIS, WARN, "SEQ NO of AIS SCN DONE MSG is not matched(%d:%d).\n",
			ucSeqNumOfCompMsg, prAisFsmInfo->ucSeqNumOfScanReq);
	} else {
#if CFG_SUPPORT_ABORT_SCAN
		if (eScanStatus == SCAN_STATUS_CANCELLED)
			rStatus = WLAN_STATUS_ABORT_SCAN;
		AisFsmSetScanState(prAdapter, FALSE);
#endif
		/*Need to clear driver scanning state*/
		prConnSettings->fgIsScanReqIssued = FALSE;
		prAisFsmInfo->u4ScanIELength = 0;

		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);
		switch (prAisFsmInfo->eCurrentState) {
		case AIS_STATE_SCAN:
			aisPostScan(prAdapter);
			kalScanDone(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX, rStatus);
			eNextState = AIS_STATE_IDLE;
#if CFG_SUPPORT_AGPS_ASSIST
			scanReportScanResultToAgps(prAdapter);
#endif
			break;

		case AIS_STATE_ONLINE_SCAN:
			aisPostScan(prAdapter);
			kalScanDone(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX, rStatus);
#if CFG_SUPPORT_ROAMING
			eNextState = aisFsmRoamingScanResultsUpdate(prAdapter);
#else
			eNextState = AIS_STATE_NORMAL_TR;
#endif /* CFG_SUPPORT_ROAMING */
#if CFG_SUPPORT_AGPS_ASSIST
			scanReportScanResultToAgps(prAdapter);
#endif
			scanGetCurrentEssChnlList(prAdapter);
			break;

		case AIS_STATE_LOOKING_FOR:
			if (prScanRequest)
				kalScanDone(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX, rStatus);
#if CFG_SUPPORT_ROAMING
			if (prAdapter->rWifiVar.rRoamingInfo.eCurrentState == ROAMING_STATE_DISCOVERY)
				glNotifyDrvStatus(ROAMING_SCAN_DONE, NULL);
			eNextState = aisFsmRoamingScanResultsUpdate(prAdapter);
#else
			eNextState = AIS_STATE_COLLECT_ESS_INFO;
#endif /* CFG_SUPPORT_ROAMING */
			aisPostScan(prAdapter);
			scanGetCurrentEssChnlList(prAdapter);
			break;

		default:
#if CFG_SUPPORT_ABORT_SCAN
			if (rStatus == WLAN_STATUS_ABORT_SCAN || prScanRequest) {
				DBGLOG(AIS, INFO,
					"AIS default state[%d] previous state [%d] rStatus [%d]\n",
					prAisFsmInfo->eCurrentState,
					prAisFsmInfo->ePreviousState,
					rStatus);
				aisPostScan(prAdapter);
				kalScanDone(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX, rStatus);
#if CFG_SUPPORT_ROAMING
				eNextState = aisFsmRoamingScanResultsUpdate(prAdapter);
#else
#endif /* CFG_SUPPORT_ROAMING */
#if CFG_SUPPORT_AGPS_ASSIST
				scanReportScanResultToAgps(prAdapter);
#endif
			} else
				DBGLOG(AIS, WARN,
				       "Wrong AIS state %d and the previous state is %d!\n",
				       prAisFsmInfo->eCurrentState,
				       prAisFsmInfo->ePreviousState);
#else
			DBGLOG(AIS, WARN, "Wrong AIS state %d\n", prAisFsmInfo->eCurrentState);
#endif
			break;
		}
	}
	aisRemoveOldestBcnTimeout(prAisFsmInfo);
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);
	if (prBcnRmParam->eState == RM_NO_REQUEST)
		return;
	/* normal mode scan done, and beacon measurement is pending, schedule to do measurement */
	if (prBcnRmParam->eState == RM_WAITING) {
		rlmDoBeaconMeasurement(prAdapter, 0);
	} else if (prBcnRmParam->rNormalScan.fgExist) {/* pending normal scan here, should schedule it on time */
		struct NORMAL_SCAN_PARAMS *prParam = &prBcnRmParam->rNormalScan;

		DBGLOG(AIS, INFO, "Schedule normal scan after a beacon measurement done\n");
		prBcnRmParam->eState = RM_WAITING;
		prBcnRmParam->rNormalScan.fgExist = FALSE;
		cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer,
			SEC_TO_MSEC(AIS_SCN_DONE_TIMEOUT_SEC));
		aisFsmScanRequestAdv(prAdapter, prParam->ucSsidNum, prParam->arSSID,
					prParam->aucScanIEBuf, prParam->u4IELen, prParam->fgFull2Partial,
					&(prParam->rScanRandMacAddr));
	} else /* Radio Measurement is on-going, schedule to next Measurement Element */
		rlmStartNextMeasurement(prAdapter, FALSE);

}				/* end of aisFsmRunEventScanDone() */

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmRunEventAbort(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_AIS_ABORT_T prAisAbortMsg;
	P_AIS_FSM_INFO_T prAisFsmInfo;
	UINT_8 ucReasonOfDisconnect;
	BOOLEAN fgDelayIndication;
	P_CONNECTION_SETTINGS_T prConnSettings;

	DEBUGFUNC("aisFsmRunEventAbort()");

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	/* 4 <1> Extract information of Abort Message and then free memory. */
	prAisAbortMsg = (P_MSG_AIS_ABORT_T) prMsgHdr;
	ucReasonOfDisconnect = prAisAbortMsg->ucReasonOfDisconnect;
	fgDelayIndication = prAisAbortMsg->fgDelayIndication;

	cnmMemFree(prAdapter, prMsgHdr);

	DBGLOG(AIS, STATE, "EVENT-ABORT: Current State %d, ucReasonOfDisconnect:%d\n",
		prAisFsmInfo->eCurrentState, ucReasonOfDisconnect);

	/* record join request time */
	GET_CURRENT_SYSTIME(&(prAisFsmInfo->rJoinReqTime));

	/* 4 <2> clear previous pending connection request and insert new one */
	if (ucReasonOfDisconnect == DISCONNECT_REASON_CODE_DEAUTHENTICATED ||
		ucReasonOfDisconnect == DISCONNECT_REASON_CODE_DISASSOCIATED) {
		P_STA_RECORD_T prSta = prAisFsmInfo->prTargetStaRec;
		P_BSS_DESC_T prBss = prAisFsmInfo->prTargetBssDesc;

		if (prSta && prBss && prSta->u2ReasonCode == REASON_CODE_DISASSOC_AP_OVERLOAD) {
			struct AIS_BLACKLIST_ITEM *prBlackList = aisAddBlacklist(prAdapter, prBss,
										AIS_BLACK_LIST_FROM_DRIVER);

			if (prBlackList)
				prBlackList->u2DeauthReason = prSta->u2ReasonCode;
			prBss->fgDeauthLastTime = TRUE;
		}
		prConnSettings->fgIsDisconnectedByNonRequest = TRUE;
	} else {
		prConnSettings->fgIsDisconnectedByNonRequest = FALSE;
	}

	/* to support user space triggered roaming */
	if (ucReasonOfDisconnect == DISCONNECT_REASON_CODE_ROAMING &&
	    prAisFsmInfo->eCurrentState != AIS_STATE_DISCONNECTING) {

		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rSecModeChangeTimer);
		if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
			/* 1. release channel */
			aisFsmReleaseCh(prAdapter);
			/* 2.1 stop join timeout timer */
			cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
			/* 2.2 reset local variable */
			prAisFsmInfo->fgIsInfraChannelFinished = TRUE;
			aisFsmSteps(prAdapter, AIS_STATE_COLLECT_ESS_INFO);
		} else {
			aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, TRUE);
			aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, TRUE);
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_ROAMING_CONNECT);
		}
		return;
	}

	scanGetCurrentEssChnlList(prAdapter);
	aisFsmIsRequestPending(prAdapter, AIS_REQUEST_RECONNECT, TRUE);
	aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);
#if CFG_SUPPORT_ABORT_SCAN
	if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, TRUE) == TRUE) {
		if (prAdapter->fgAbortScan) {
			DBGLOG(INIT, INFO, "Remove pending scan, no need insert new scan req\n");
			AisFsmGenerateScanDoneMsg(prAdapter, SCAN_STATUS_CANCELLED);
			prAdapter->fgAbortScan = FALSE;
		} else {
			DBGLOG(INIT, INFO, "Remove pending scan and insert new scan req\n");
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
		}
	}
#endif

	if (prAisFsmInfo->eCurrentState != AIS_STATE_DISCONNECTING) {
		if (ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION) {
			/* 4 <3> invoke abort handler */
			aisFsmStateAbort(prAdapter, ucReasonOfDisconnect, fgDelayIndication);
		} else {
			/* 1. release channel */
			aisFsmReleaseCh(prAdapter);
			/* 2.1 stop join timeout timer */
			cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
			/* 2.2 reset local variable */
			prAisFsmInfo->fgIsInfraChannelFinished = TRUE;

			prAdapter->prAisBssInfo->ucReasonOfDisconnect = ucReasonOfDisconnect;
			aisFsmSteps(prAdapter, AIS_STATE_IDLE);
		}
	}
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
VOID aisFsmStateAbort(IN P_ADAPTER_T prAdapter, UINT_8 ucReasonOfDisconnect, BOOLEAN fgDelayIndication)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_INFO_T prAisBssInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;
	BOOLEAN fgIsCheckConnected;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	fgIsCheckConnected = FALSE;

	/* 4 <1> Save information of Abort Message and then free memory. */
	prAisBssInfo->ucReasonOfDisconnect = ucReasonOfDisconnect;
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED &&
		prAisFsmInfo->eCurrentState != AIS_STATE_DISCONNECTING &&
		ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION &&
		ucReasonOfDisconnect != DISCONNECT_REASON_CODE_ROAMING)
		wmmNotifyDisconnected(prAdapter);

	/* 4 <2> Abort current job. */
	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_IDLE:
	case AIS_STATE_SEARCH:
	case AIS_STATE_JOIN_FAILURE:
		break;
	case AIS_STATE_WAIT_FOR_NEXT_SCAN:
		/* Do cancel timer */
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBGScanTimer);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_SCAN:
#if CFG_SUPPORT_ABORT_SCAN
		if (prAdapter->prGlueInfo->ucAbortScanCnt < ABORT_SCAN_COUNT &&
		    prAdapter->prGlueInfo->u4LastNormalScanTime != 0 &&
		    !(CHECK_FOR_TIMEOUT(kalGetTimeTick(),
					prAdapter->prGlueInfo->u4LastNormalScanTime,
					SEC_TO_SYSTIME(ABORT_SCAN_INTERVAl)))) {
			DBGLOG(AIS, INFO,
				"Last normal before %d s (%lld, %lld)\n",
				ABORT_SCAN_INTERVAl, kalGetTimeTick(),
				prAdapter->prGlueInfo->u4LastNormalScanTime);
			/* Do abort SCAN */
			aisFsmStateAbort_SCAN(prAdapter);

			/* queue for later handling */
			if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, FALSE) == FALSE)
				aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
		} else {
			DBGLOG(AIS, INFO,
				"Last normal scan time(%lld, %lld)\n",
				kalGetTimeTick(),
				prAdapter->prGlueInfo->u4LastNormalScanTime);
		}
#else
		/* Do abort SCAN */
		aisFsmStateAbort_SCAN(prAdapter);

		/* queue for later handling */
		if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, FALSE) == FALSE)
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);

#endif
		break;

	case AIS_STATE_LOOKING_FOR:
#if CFG_SUPPORT_ABORT_SCAN
		if (ucReasonOfDisconnect != DISCONNECT_REASON_CODE_NEW_CONNECTION &&
		    prAdapter->prGlueInfo->ucAbortScanCnt < ABORT_SCAN_COUNT &&
		    prAdapter->prGlueInfo->u4LastNormalScanTime != 0 &&
		    !(CHECK_FOR_TIMEOUT(kalGetTimeTick(),
					prAdapter->prGlueInfo->u4LastNormalScanTime,
					SEC_TO_SYSTIME(ABORT_SCAN_INTERVAl)))) {
			DBGLOG(AIS, INFO,
				"Last normal before %d s (%lld,%lld)\n",
				ABORT_SCAN_INTERVAl, kalGetTimeTick(),
				prAdapter->prGlueInfo->u4LastNormalScanTime);
			/* Do abort SCAN */
			aisFsmStateAbort_SCAN(prAdapter);
		} else {
			DBGLOG(AIS, INFO,
				"Last normal scan time(%lld, %lld)\n",
				kalGetTimeTick(),
				prAdapter->prGlueInfo->u4LastNormalScanTime);
		}
#else
		/* Do abort SCAN */
		aisFsmStateAbort_SCAN(prAdapter);
#endif

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_REQ_CHANNEL_JOIN:
		/* Release channel to CNM */
		aisFsmReleaseCh(prAdapter);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rWaitOkcPMKTimer);
		break;

	case AIS_STATE_JOIN:
		/* Do abort JOIN */
		aisFsmStateAbort_JOIN(prAdapter);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		break;

#if CFG_SUPPORT_ADHOC
	case AIS_STATE_IBSS_ALONE:
	case AIS_STATE_IBSS_MERGE:
		aisFsmStateAbort_IBSS(prAdapter);
		break;
#endif /* CFG_SUPPORT_ADHOC */

	case AIS_STATE_ONLINE_SCAN:
		/* Do abort SCAN */
#if CFG_SUPPORT_ABORT_SCAN
		if (prAdapter->prGlueInfo->ucAbortScanCnt < ABORT_SCAN_COUNT &&
		    prAdapter->prGlueInfo->u4LastNormalScanTime != 0 &&
		    !(CHECK_FOR_TIMEOUT(kalGetTimeTick(),
					prAdapter->prGlueInfo->u4LastNormalScanTime,
					SEC_TO_SYSTIME(ABORT_SCAN_INTERVAl)))) {
			DBGLOG(AIS, INFO,
				"Last normal before %d s (%lld, %lld)\n",
				ABORT_SCAN_INTERVAl, kalGetTimeTick(),
				prAdapter->prGlueInfo->u4LastNormalScanTime);
			aisFsmStateAbort_SCAN(prAdapter);

			/* queue for later handling */
			if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, FALSE) == FALSE)
				aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);

		} else {
			DBGLOG(AIS, INFO,
				"Last normal scan time(%lld, %lld)\n",
				kalGetTimeTick(),
				prAdapter->prGlueInfo->u4LastNormalScanTime);
		}
#else
		aisFsmStateAbort_SCAN(prAdapter);

		/* queue for later handling */
		if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, FALSE) == FALSE)
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);

#endif
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_NORMAL_TR:
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_DISCONNECTING:
		/* Do abort NORMAL_TR */
		aisFsmStateAbort_NORMAL_TR(prAdapter);

		break;

	case AIS_STATE_REQ_REMAIN_ON_CHANNEL:
		/* release channel */
		aisFsmReleaseCh(prAdapter);
		break;

	case AIS_STATE_REMAIN_ON_CHANNEL:
		/* 1. release channel */
		aisFsmReleaseCh(prAdapter);

		/* 2. stop channel timeout timer */
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer);

		break;

	default:
		break;
	}

	if (fgIsCheckConnected && (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED)) {

		/* switch into DISCONNECTING state for sending DEAUTH if necessary */
		if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
		    prAisBssInfo->ucReasonOfDisconnect == DISCONNECT_REASON_CODE_NEW_CONNECTION &&
		    prAisBssInfo->prStaRecOfAP && prAisBssInfo->prStaRecOfAP->fgIsInUse) {
			aisFsmSteps(prAdapter, AIS_STATE_DISCONNECTING);

			return;
		}

		/* Do abort NORMAL_TR */
		aisFsmStateAbort_NORMAL_TR(prAdapter);
	}
	rlmCancelRadioMeasurement(prAdapter);
	aisFsmDisconnect(prAdapter, fgDelayIndication);

}				/* end of aisFsmStateAbort() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Join Complete Event from SAA FSM for AIS FSM
*
* @param[in] prMsgHdr   Message of Join Complete of SAA FSM.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmRunEventJoinComplete(IN struct _ADAPTER_T *prAdapter, IN struct _MSG_HDR_T *prMsgHdr)
{
	struct _MSG_SAA_FSM_COMP_T *prJoinCompMsg;
	struct _AIS_FSM_INFO_T *prAisFsmInfo;
	enum _ENUM_AIS_STATE_T eNextState;
	struct _SW_RFB_T *prAssocRspSwRfb;

	DEBUGFUNC("aisFsmRunEventJoinComplete()");
	ASSERT(prMsgHdr);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prJoinCompMsg = (struct _MSG_SAA_FSM_COMP_T *)prMsgHdr;
	prAssocRspSwRfb = prJoinCompMsg->prSwRfb;

	eNextState = prAisFsmInfo->eCurrentState;

	/* Check State and SEQ NUM */
	if (prAisFsmInfo->eCurrentState == AIS_STATE_JOIN) {
		/* Check SEQ NUM */
		if (prJoinCompMsg->ucSeqNum == prAisFsmInfo->ucSeqNumOfReqMsg)
			eNextState = aisFsmJoinCompleteAction(prAdapter, prMsgHdr);
		/* try to remove timeout blacklist item */
		aisRemoveDisappearedBlacklist(prAdapter);
#if DBG
		else
			DBGLOG(AIS, WARN, "SEQ NO of AIS JOIN COMP MSG is not matched.\n");
#endif /* DBG */
	}
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);

	if (prAssocRspSwRfb)
		nicRxReturnRFB(prAdapter, prAssocRspSwRfb);

	cnmMemFree(prAdapter, prMsgHdr);

}				/* end of aisFsmRunEventJoinComplete() */

enum _ENUM_AIS_STATE_T aisFsmJoinCompleteAction(IN struct _ADAPTER_T *prAdapter, IN struct _MSG_HDR_T *prMsgHdr)
{
	struct _MSG_SAA_FSM_COMP_T *prJoinCompMsg;
	struct _AIS_FSM_INFO_T *prAisFsmInfo;
	enum _ENUM_AIS_STATE_T eNextState;
	struct _STA_RECORD_T *prStaRec;
	struct _SW_RFB_T *prAssocRspSwRfb;
	struct _BSS_INFO_T *prAisBssInfo;
	OS_SYSTIME rCurrentTime;
	P_CONNECTION_SETTINGS_T prConnSettings;
	UINT_8 aucP2pSsid[] = CTIA_MAGIC_SSID;

	DEBUGFUNC("aisFsmJoinCompleteAction()");

	ASSERT(prMsgHdr);

	GET_CURRENT_SYSTIME(&rCurrentTime);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prJoinCompMsg = (struct _MSG_SAA_FSM_COMP_T *)prMsgHdr;
	prStaRec = prJoinCompMsg->prStaRec;
	prAssocRspSwRfb = prJoinCompMsg->prSwRfb;
	prAisBssInfo = prAdapter->prAisBssInfo;
	eNextState = prAisFsmInfo->eCurrentState;
	prConnSettings = &prAdapter->rWifiVar.rConnSettings;

	do {
		/* 4 <1> JOIN was successful */
		if (prJoinCompMsg->rJoinStatus == WLAN_STATUS_SUCCESS) {

#if CFG_SUPPORT_RN
			GET_CURRENT_SYSTIME(&prAisBssInfo->rConnTime);
			prAisBssInfo->fgDisConnReassoc = FALSE;
#endif
#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
			prAdapter->rWifiVar.rConnSettings.fgSecModeChangeStartTimer = FALSE;
#endif
			/* 1. Reset retry count */
			prAisFsmInfo->ucConnTrialCount = 0;
			prAdapter->rWifiVar.rConnSettings.eReConnectLevel = RECONNECT_LEVEL_MIN;

			/* Completion of roaming */
			if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {

#if CFG_SUPPORT_ROAMING
				/* 2. Deactivate previous BSS */
				aisFsmRoamingDisconnectPrevAP(prAdapter, prStaRec);

				/* 3. Update bss based on roaming staRec */
				aisUpdateBssInfoForRoamingAP(prAdapter, prStaRec, prAssocRspSwRfb);
#endif /* CFG_SUPPORT_ROAMING */
			} else {
				kalResetStats(prAdapter->prGlueInfo->prDevHandler);

				/* 4 <1.1> Change FW's Media State immediately. */
				aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

				/* 4 <1.2> Deactivate previous AP's STA_RECORD_T in Driver if have. */
				if ((prAisBssInfo->prStaRecOfAP) &&
				    (prAisBssInfo->prStaRecOfAP != prStaRec) &&
				    (prAisBssInfo->prStaRecOfAP->fgIsInUse) &&
				    (prAisBssInfo->prStaRecOfAP->ucBssIndex ==
				     prAisBssInfo->ucBssIndex)) {

					cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP, STA_STATE_1);
					cnmStaRecFree(prAdapter, prAisBssInfo->prStaRecOfAP);
				}

				/* For temp solution, need to refine */
				/* 4 <1.4> Update BSS_INFO_T */
				aisUpdateBssInfoForJOIN(prAdapter, prStaRec, prAssocRspSwRfb);

				/* 4 <1.3> Activate current AP's STA_RECORD_T in Driver. */
				cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);

				/* 4 <1.5> Update RSSI if necessary */
				nicUpdateRSSI(prAdapter,
					      prAdapter->prAisBssInfo->ucBssIndex,
					      (INT_8) (RCPI_TO_dBm(prStaRec->ucRCPI)), 0);

				/* 4 <1.6> Indicate Connected Event to Host immediately. */
				/* Require BSSID, Association ID, Beacon Interval */
				/* .. from AIS_BSS_INFO_T */
				aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_CONNECTED, FALSE);

				if (EQUAL_SSID
				    (aucP2pSsid, CTIA_MAGIC_SSID_LEN, prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen)) {
					nicEnterCtiaMode(prAdapter, TRUE, FALSE);
				}
			}

#if CFG_SUPPORT_ROAMING
			/* if user space roaming is enabled, we should disable driver/fw roaming */
			if ((prAdapter->rWifiVar.rConnSettings.eConnectionPolicy != CONNECT_BY_BSSID)
				&& prAdapter->rWifiVar.rRoamingInfo.fgDrvRoamingAllow)
				roamingFsmRunEventStart(prAdapter);
#endif /* CFG_SUPPORT_ROAMING */
			if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, FALSE) == FALSE)
				prAisFsmInfo->rJoinReqTime = 0;
#if CFG_SUPPORT_802_11K
			if (prAisFsmInfo->prTargetBssDesc->aucRrmCap[0] & BIT(RRM_CAP_INFO_NEIGHBOR_REPORT_BIT))
				aisSendNeighborRequest(prAdapter);
#endif
			prAisFsmInfo->prTargetBssDesc->fgDeauthLastTime = FALSE;
			prAisFsmInfo->ucJoinFailCntAfterScan = 0;
			prAisBssInfo->u2DeauthReason = REASON_CODE_RESERVED;
			glNotifyDrvStatus(CONNECT_AP, (PVOID)prStaRec->aucMacAddr);
#if (defined FW_CFG_SUPPORT) && (defined CFG_SUPPORT_COEX_IOT_AP)
			wlanFWCfgForIotAP(prAdapter, prAisFsmInfo->prTargetBssDesc->aucBSSID);
#endif
			/* 4 <1.7> Set the Next State of AIS FSM */
			eNextState = AIS_STATE_NORMAL_TR;
		}
		/* 4 <2> JOIN was not successful */
		else {
			/* 4 <2.1> Redo JOIN process with other Auth Type if possible */
			if (aisFsmStateInit_RetryJOIN(prAdapter, prStaRec) == FALSE) {
				struct _BSS_DESC_T *prBssDesc;

				/* 1. Increase Failure Count */
				prStaRec->ucJoinFailureCount++;

				/* 2. release channel */
				aisFsmReleaseCh(prAdapter);

				/* 3.1 stop join timeout timer */
				cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);

				/* 3.2 reset local variable */
				prAisFsmInfo->fgIsInfraChannelFinished = TRUE;
				prAisFsmInfo->ucJoinFailCntAfterScan++;

				prBssDesc = prAisFsmInfo->prTargetBssDesc;

				if (prBssDesc == NULL)
					break;

				DBGLOG(AIS, TRACE,
				"ucJoinFailureCount=%d %d, Status=%d Reason=%d, eConnectionState=%d, DisReassoc=%d",
					prStaRec->ucJoinFailureCount, prBssDesc->ucJoinFailureCount,
					prStaRec->u2StatusCode, prStaRec->u2ReasonCode,
					prAisBssInfo->eConnectionState, prAisBssInfo->fgDisConnReassoc);

				/* ASSERT(prBssDesc); */
				/* ASSERT(prBssDesc->fgIsConnecting); */
				glNotifyDrvStatus(JOIN_FAIL, (PVOID)prStaRec);
				prBssDesc->u2JoinStatus = prStaRec->u2StatusCode;
				prBssDesc->ucJoinFailureCount++;
				if (prBssDesc->ucJoinFailureCount >= AIS_BSS_ADD_BLACKLIST_THRESHOLD) {
					aisAddBlacklist(prAdapter, prBssDesc, AIS_BLACK_LIST_FROM_DRIVER);

					GET_CURRENT_SYSTIME(&prBssDesc->rJoinFailTime);
					DBGLOG(AIS, WARN,
					       "Bss " MACSTR " join fail %d times, temp disable it at time: %u\n",
						MAC2STR(prBssDesc->aucBSSID),
						prBssDesc->ucJoinFailureCount,
						prBssDesc->rJoinFailTime);
				}

				if (prBssDesc->prBlack)
					prBssDesc->prBlack->u2AuthStatus = prStaRec->u2StatusCode;

				if (prBssDesc)
					prBssDesc->fgIsConnecting = FALSE;

				/* 3.3 Free STA-REC */
				if (prStaRec != prAisBssInfo->prStaRecOfAP)
					cnmStaRecFree(prAdapter, prStaRec);

				if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
					PARAM_SSID_T rSsid;
#if CFG_SUPPORT_ROAMING
					eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
#endif /* CFG_SUPPORT_ROAMING */
					if (prAisBssInfo->prStaRecOfAP)
						prAisBssInfo->prStaRecOfAP->fgIsTxAllowed = TRUE;
					if (prConnSettings->eConnectionPolicy == CONNECT_BY_BSSID &&
					    prBssDesc->u2JoinStatus) {
						UINT_32 u4InfoBufLen = 0;

						/* For framework roaming case, if authentication is rejected,
						** need to make driver disconnecting because wpa_supplicant
						** will enter disconnected state in this case, otherwise, connection
						** state between driver and supplicant will be not synchronized.
						*/
						wlanoidSetDisassociate(prAdapter, NULL, 0, &u4InfoBufLen);
						eNextState = prAisFsmInfo->eCurrentState;
						break;
					}
					COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
						  prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen);
					prAisFsmInfo->prTargetBssDesc =
						scanSearchBssDescByBssidAndSsid(prAdapter,
						prAisBssInfo->aucBSSID, TRUE, &rSsid);
					prAisFsmInfo->prTargetStaRec = prAisBssInfo->prStaRecOfAP;
					ASSERT(prAisFsmInfo->prTargetBssDesc);
					if (!prAisFsmInfo->prTargetBssDesc)
						DBGLOG(AIS, ERROR, "Can't retrieve target bss descriptor\n");
#if CFG_SUPPORT_RN
				} else if (prAisBssInfo->fgDisConnReassoc == TRUE) {
					eNextState = AIS_STATE_JOIN_FAILURE;
#endif
				} else if (prAisFsmInfo->rJoinReqTime != 0 &&
					CHECK_FOR_TIMEOUT(rCurrentTime, prAisFsmInfo->rJoinReqTime,
							  SEC_TO_SYSTIME(AIS_JOIN_TIMEOUT))) {
					/* 4.a temrminate join operation */
					eNextState = AIS_STATE_JOIN_FAILURE;
					prBssDesc->u2JoinStatus = STATUS_CODE_AUTH_TIMEOUT;
				} else if (prAisFsmInfo->rJoinReqTime != 0
					&& prBssDesc->ucJoinFailureCount >= SCN_BSS_JOIN_FAIL_THRESHOLD
					&& prBssDesc->u2JoinStatus) {
					/* AP reject STA for STATUS_CODE_ASSOC_DENIED_AP_OVERLOAD, or AP block STA */
					eNextState = AIS_STATE_JOIN_FAILURE;
				} else {
					/* 4.b send reconnect request */
					aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);

					eNextState = AIS_STATE_IDLE;
				}
			}
		}
	} while (0);
	return eNextState;
}

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Grant Msg of IBSS Create which was sent by
*        CNM to indicate that channel was changed for creating IBSS.
*
* @param[in] prAdapter  Pointer of ADAPTER_T
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmCreateIBSS(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	do {
		/* Check State */
		if (prAisFsmInfo->eCurrentState == AIS_STATE_IBSS_ALONE)
			aisUpdateBssInfoForCreateIBSS(prAdapter);

	} while (FALSE);

}				/* end of aisFsmCreateIBSS() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Grant Msg of IBSS Merge which was sent by
*        CNM to indicate that channel was changed for merging IBSS.
*
* @param[in] prAdapter  Pointer of ADAPTER_T
* @param[in] prStaRec   Pointer of STA_RECORD_T for merge
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmMergeIBSS(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	ENUM_AIS_STATE_T eNextState;
	P_BSS_INFO_T prAisBssInfo;

	ASSERT(prAdapter);
	ASSERT(prStaRec);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;

	do {

		eNextState = prAisFsmInfo->eCurrentState;

		switch (prAisFsmInfo->eCurrentState) {
		case AIS_STATE_IBSS_MERGE:
			{
				P_BSS_DESC_T prBssDesc;

				/* 4 <1.1> Change FW's Media State immediately. */
				aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

				/* 4 <1.2> Deactivate previous Peers' STA_RECORD_T in Driver if have. */
				bssInitializeClientList(prAdapter, prAisBssInfo);

				/* 4 <1.3> Unmark connection flag of previous BSS_DESC_T. */
				prBssDesc = scanSearchBssDescByBssid(prAdapter, prAisBssInfo->aucBSSID);
				if (prBssDesc != NULL) {
					prBssDesc->fgIsConnecting = FALSE;
					prBssDesc->fgIsConnected = FALSE;
				}
				/* 4 <1.4> Add Peers' STA_RECORD_T to Client List */
				bssAddClient(prAdapter, prAisBssInfo, prStaRec);

				/* 4 <1.5> Activate current Peer's STA_RECORD_T in Driver. */
				cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
				prStaRec->fgIsMerging = FALSE;

				/* 4 <1.6> Update BSS_INFO_T */
				aisUpdateBssInfoForMergeIBSS(prAdapter, prStaRec);

				/* 4 <1.7> Enable other features */

				/* 4 <1.8> Indicate Connected Event to Host immediately. */
				aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_CONNECTED, FALSE);

				/* 4 <1.9> Set the Next State of AIS FSM */
				eNextState = AIS_STATE_NORMAL_TR;

				/* 4 <1.10> Release channel privilege */
				aisFsmReleaseCh(prAdapter);

#if CFG_SLT_SUPPORT
				prAdapter->rWifiVar.rSltInfo.prPseudoStaRec = prStaRec;
#endif
			}
			break;

		default:
			break;
		}

		if (eNextState != prAisFsmInfo->eCurrentState)
			aisFsmSteps(prAdapter, eNextState);

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
VOID aisFsmRunEventFoundIBSSPeer(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_AIS_IBSS_PEER_FOUND_T prAisIbssPeerFoundMsg;
	P_AIS_FSM_INFO_T prAisFsmInfo;
	ENUM_AIS_STATE_T eNextState;
	P_STA_RECORD_T prStaRec;
	P_BSS_INFO_T prAisBssInfo;
	P_BSS_DESC_T prBssDesc;
	BOOLEAN fgIsMergeIn;

	ASSERT(prMsgHdr);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;

	prAisIbssPeerFoundMsg = (P_MSG_AIS_IBSS_PEER_FOUND_T) prMsgHdr;

	ASSERT(prAisIbssPeerFoundMsg->ucBssIndex == prAdapter->prAisBssInfo->ucBssIndex);

	prStaRec = prAisIbssPeerFoundMsg->prStaRec;
	ASSERT(prStaRec);

	fgIsMergeIn = prAisIbssPeerFoundMsg->fgIsMergeIn;

	cnmMemFree(prAdapter, prMsgHdr);

	eNextState = prAisFsmInfo->eCurrentState;
	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_IBSS_ALONE:
		{
			/* 4 <1> An IBSS Peer 'merged in'. */
			if (fgIsMergeIn) {

				/* 4 <1.1> Change FW's Media State immediately. */
				aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

				/* 4 <1.2> Add Peers' STA_RECORD_T to Client List */
				bssAddClient(prAdapter, prAisBssInfo, prStaRec);

#if CFG_SLT_SUPPORT
				/* 4 <1.3> Mark connection flag of BSS_DESC_T. */
				prBssDesc = scanSearchBssDescByTA(prAdapter, prStaRec->aucMacAddr);

				if (prBssDesc != NULL) {
					prBssDesc->fgIsConnecting = FALSE;
					prBssDesc->fgIsConnected = TRUE;
				} else {
					ASSERT(0);	/* Should be able to find a BSS_DESC_T here. */
				}

				/* 4 <1.4> Activate current Peer's STA_RECORD_T in Driver. */
				prStaRec->fgIsQoS = TRUE;	/* TODO(Kevin): TBD */
#else
				/* 4 <1.3> Mark connection flag of BSS_DESC_T. */
				prBssDesc = scanSearchBssDescByBssid(prAdapter, prAisBssInfo->aucBSSID);

				if (prBssDesc != NULL) {
					prBssDesc->fgIsConnecting = FALSE;
					prBssDesc->fgIsConnected = TRUE;
				} else {
					ASSERT(0);	/* Should be able to find a BSS_DESC_T here. */
				}

				/* 4 <1.4> Activate current Peer's STA_RECORD_T in Driver. */
				prStaRec->fgIsQoS = FALSE;	/* TODO(Kevin): TBD */

#endif

				cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
				prStaRec->fgIsMerging = FALSE;

				/* 4 <1.6> sync. to firmware */
				nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

				/* 4 <1.7> Indicate Connected Event to Host immediately. */
				aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_CONNECTED, FALSE);

				/* 4 <1.8> indicate PM for connected */
				nicPmIndicateBssConnected(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

				/* 4 <1.9> Set the Next State of AIS FSM */
				eNextState = AIS_STATE_NORMAL_TR;

				/* 4 <1.10> Release channel privilege */
				aisFsmReleaseCh(prAdapter);
			}
			/* 4 <2> We need 'merge out' to this IBSS */
			else {

				/* 4 <2.1> Get corresponding BSS_DESC_T */
				prBssDesc = scanSearchBssDescByTA(prAdapter, prStaRec->aucMacAddr);

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

				/* 4 <3.1> Add Peers' STA_RECORD_T to Client List */
				bssAddClient(prAdapter, prAisBssInfo, prStaRec);

#if CFG_SLT_SUPPORT
				/* 4 <3.2> Activate current Peer's STA_RECORD_T in Driver. */
				prStaRec->fgIsQoS = TRUE;	/* TODO(Kevin): TBD */
#else
				/* 4 <3.2> Activate current Peer's STA_RECORD_T in Driver. */
				prStaRec->fgIsQoS = FALSE;	/* TODO(Kevin): TBD */
#endif

				cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
				prStaRec->fgIsMerging = FALSE;

			}
			/* 4 <4> We need 'merge out' to this IBSS */
			else {

				/* 4 <4.1> Get corresponding BSS_DESC_T */
				prBssDesc = scanSearchBssDescByTA(prAdapter, prStaRec->aucMacAddr);

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
		aisFsmSteps(prAdapter, eNextState);

}				/* end of aisFsmRunEventFoundIBSSPeer() */
#endif /* CFG_SUPPORT_ADHOC */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will indicate the Media State to HOST
*
* @param[in] eConnectionState   Current Media State
* @param[in] fgDelayIndication  Set TRUE for postponing the Disconnect Indication.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID
aisIndicationOfMediaStateToHost(IN P_ADAPTER_T prAdapter,
				ENUM_PARAM_MEDIA_STATE_T eConnectionState, BOOLEAN fgDelayIndication)
{
	EVENT_CONNECTION_STATUS rEventConnStatus;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_INFO_T prAisBssInfo;
	P_AIS_FSM_INFO_T prAisFsmInfo;

	DEBUGFUNC("aisIndicationOfMediaStateToHost()");

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* NOTE(Kevin): Move following line to aisChangeMediaState() macro per CM's request. */
	/* prAisBssInfo->eConnectionState = eConnectionState; */

	/* For indicating the Disconnect Event only if current media state is
	 * disconnected and we didn't do indication yet.
	 */
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED &&
		prAisFsmInfo->eCurrentState != AIS_STATE_JOIN) {

		if (prAisBssInfo->eConnectionStateIndicated == eConnectionState)
			return;
	}

	if (!fgDelayIndication) {
		/* 4 <0> Cancel Delay Timer */
		prAisFsmInfo->u4PostponeIndStartTime = 0;

		/* 4 <1> Fill EVENT_CONNECTION_STATUS */
		rEventConnStatus.ucMediaStatus = (UINT_8) eConnectionState;

		if (eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
			rEventConnStatus.ucReasonOfDisconnect = DISCONNECT_REASON_CODE_RESERVED;

			if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
				rEventConnStatus.ucInfraMode = (UINT_8) NET_TYPE_INFRA;
				rEventConnStatus.u2AID = prAisBssInfo->u2AssocId;
				rEventConnStatus.u2ATIMWindow = 0;
			} else if (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
				rEventConnStatus.ucInfraMode = (UINT_8) NET_TYPE_IBSS;
				rEventConnStatus.u2AID = 0;
				rEventConnStatus.u2ATIMWindow = prAisBssInfo->u2ATIMWindow;
			} else {
				ASSERT(0);
			}

			COPY_SSID(rEventConnStatus.aucSsid,
				  rEventConnStatus.ucSsidLen, prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

			COPY_MAC_ADDR(rEventConnStatus.aucBssid, prAisBssInfo->aucBSSID);

			rEventConnStatus.u2BeaconPeriod = prAisBssInfo->u2BeaconInterval;
			rEventConnStatus.u4FreqInKHz = nicChannelNum2Freq(prAisBssInfo->ucPrimaryChannel);

			switch (prAisBssInfo->ucNonHTBasicPhyType) {
			case PHY_TYPE_HR_DSSS_INDEX:
				rEventConnStatus.ucNetworkType = (UINT_8) PARAM_NETWORK_TYPE_DS;
				break;

			case PHY_TYPE_ERP_INDEX:
				rEventConnStatus.ucNetworkType = (UINT_8) PARAM_NETWORK_TYPE_OFDM24;
				break;

			case PHY_TYPE_OFDM_INDEX:
				rEventConnStatus.ucNetworkType = (UINT_8) PARAM_NETWORK_TYPE_OFDM5;
				break;

			default:
				ASSERT(0);
				rEventConnStatus.ucNetworkType = (UINT_8) PARAM_NETWORK_TYPE_DS;
				break;
			}
		} else {
			/* Clear the pmkid cache while media disconnect */
			secClearPmkid(prAdapter);
			rEventConnStatus.ucReasonOfDisconnect = prAisBssInfo->ucReasonOfDisconnect;
		}

		/* 4 <2> Indication */
		nicMediaStateChange(prAdapter, prAdapter->prAisBssInfo->ucBssIndex, &rEventConnStatus);
		prAisBssInfo->eConnectionStateIndicated = eConnectionState;
		if (eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED) {
			if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
				aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_DISCONNECTED);
				/* 4 <4.1> sync. with firmware */
				nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
			}
			prAisFsmInfo->prTargetBssDesc = NULL;
			prAisFsmInfo->prTargetStaRec = NULL;
		}
	} else {
		/* NOTE: Only delay the Indication of Disconnect Event */
		ASSERT(eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED);

#if CFG_SUPPORT_RN
		if (prAisBssInfo->fgDisConnReassoc) {
			DBGLOG(AIS, INFO, "Reassoc the AP once beacause of receive deauth/deassoc\n");
		} else
#endif
		{
			DBGLOG(AIS, INFO, "Postpone the indication of Disconnect for %d seconds\n",
					prConnSettings->ucDelayTimeOfDisconnectEvent);

			prAisFsmInfo->u4PostponeIndStartTime = kalGetTimeTick();
		}
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
VOID aisCheckPostponedDisconnTimeout(IN P_ADAPTER_T prAdapter, P_AIS_FSM_INFO_T prAisFsmInfo)
{
	P_BSS_INFO_T prAisBssInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;
	BOOLEAN fgFound = TRUE;
	BOOLEAN fgIsPostponeTimeout = FALSE;
	BOOLEAN fgIsBeaconTimeout = FALSE;

	/*
	 * firstly, check if we have started postpone indication.
	 * otherwise, give a chance to do join before indicate to host
	 */
	if (prAisFsmInfo->u4PostponeIndStartTime == 0)
		return;

	/* if we're in	req channel/join/search state, don't report disconnect. */
	if (prAisFsmInfo->eCurrentState == AIS_STATE_JOIN ||
		prAisFsmInfo->eCurrentState == AIS_STATE_SEARCH ||
		prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN ||
		prAisFsmInfo->eCurrentState == AIS_STATE_COLLECT_ESS_INFO)
		return;

	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	fgIsPostponeTimeout = CHECK_FOR_TIMEOUT(kalGetTimeTick(), prAisFsmInfo->u4PostponeIndStartTime,
						SEC_TO_MSEC(prConnSettings->ucDelayTimeOfDisconnectEvent));
	fgIsBeaconTimeout = prAisBssInfo->ucReasonOfDisconnect == DISCONNECT_REASON_CODE_RADIO_LOST;
#if CFG_SUPPORT_RN
	fgIsBeaconTimeout &= !prAisBssInfo->fgDisConnReassoc;
#endif

	/* only retry connect two times when beacon timeout */
	if (!fgIsPostponeTimeout && !(fgIsBeaconTimeout && prAisFsmInfo->ucConnTrialCount > 1)) {
		DBGLOG(AIS, INFO, "DelayTimeOfDisconnect, don't report disconnect\n");
		return;
	}

	/* 4 <1> Deactivate previous AP's STA_RECORD_T in Driver if have. */
	if (prAisBssInfo->prStaRecOfAP) {
		/* cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP, STA_STATE_1); */

		prAisBssInfo->prStaRecOfAP = (P_STA_RECORD_T) NULL;
	}
	/* 4 <2> Remove all pending connection request */
	while (fgFound)
		fgFound = aisFsmIsRequestPending(prAdapter, AIS_REQUEST_RECONNECT, TRUE);
	if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR)
		prAisFsmInfo->eCurrentState = AIS_STATE_IDLE;
	prConnSettings->fgIsDisconnectedByNonRequest = TRUE;
	prAisBssInfo->u2DeauthReason = 100 * REASON_CODE_BEACON_TIMEOUT + prAisBssInfo->u2DeauthReason;
	prConnSettings->ucSSIDLen = 0;
	/* 4 <3> Indicate Disconnected Event to Host immediately. */
	aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_DISCONNECTED, FALSE);

}				/* end of aisPostponedEventOfDisconnTimeout() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will update the contain of BSS_INFO_T for AIS network once
*        the association was completed.
*
* @param[in] prStaRec               Pointer to the STA_RECORD_T
* @param[in] prAssocRspSwRfb        Pointer to SW RFB of ASSOC RESP FRAME.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisUpdateBssInfoForJOIN(IN P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec, P_SW_RFB_T prAssocRspSwRfb)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_INFO_T prAisBssInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_WLAN_ASSOC_RSP_FRAME_T prAssocRspFrame;
	P_BSS_DESC_T prBssDesc;
	UINT_16 u2IELength;
	PUINT_8 pucIE;

	DEBUGFUNC("aisUpdateBssInfoForJOIN()");

	ASSERT(prStaRec);
	ASSERT(prAssocRspSwRfb);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prAssocRspFrame = (P_WLAN_ASSOC_RSP_FRAME_T) prAssocRspSwRfb->pvHeader;

	DBGLOG(AIS, TRACE, "Update AIS_BSS_INFO_T and apply settings to MAC\n");

	/* 3 <1> Update BSS_INFO_T from AIS_FSM_INFO_T or User Settings */
	/* 4 <1.1> Setup Operation Mode */
	prAisBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;

	/* 4 <1.2> Setup SSID */
	COPY_SSID(prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen, prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

	/* 4 <1.3> Setup Channel, Band */
	prAisBssInfo->ucPrimaryChannel = prAisFsmInfo->prTargetBssDesc->ucChannelNum;
	prAisBssInfo->eBand = prAisFsmInfo->prTargetBssDesc->eBand;

	/* 3 <2> Update BSS_INFO_T from STA_RECORD_T */
	/* 4 <2.1> Save current AP's STA_RECORD_T and current AID */
	prAisBssInfo->prStaRecOfAP = prStaRec;
	prAisBssInfo->u2AssocId = prStaRec->u2AssocId;

	/* 4 <2.2> Setup Capability */
	prAisBssInfo->u2CapInfo = prStaRec->u2CapInfo;	/* Use AP's Cap Info as BSS Cap Info */

	if (prAisBssInfo->u2CapInfo & CAP_INFO_SHORT_PREAMBLE)
		prAisBssInfo->fgIsShortPreambleAllowed = TRUE;
	else
		prAisBssInfo->fgIsShortPreambleAllowed = FALSE;

#if CFG_SUPPORT_TDLS
	prAisBssInfo->fgTdlsIsProhibited = prStaRec->fgTdlsIsProhibited;
	prAisBssInfo->fgTdlsIsChSwProhibited = prStaRec->fgTdlsIsChSwProhibited;
#endif /* CFG_SUPPORT_TDLS */

	/* 4 <2.3> Setup PHY Attributes and Basic Rate Set/Operational Rate Set */
	prAisBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

	prAisBssInfo->ucNonHTBasicPhyType = prStaRec->ucNonHTBasicPhyType;

	prAisBssInfo->u2OperationalRateSet = prStaRec->u2OperationalRateSet;
	prAisBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

	nicTxUpdateBssDefaultRate(prAisBssInfo);

	/* 3 <3> Update BSS_INFO_T from SW_RFB_T (Association Resp Frame) */
	/* 4 <3.1> Setup BSSID */
	COPY_MAC_ADDR(prAisBssInfo->aucBSSID, prAssocRspFrame->aucBSSID);

	u2IELength = (UINT_16) ((prAssocRspSwRfb->u2PacketLen - prAssocRspSwRfb->u2HeaderLen) -
				(OFFSET_OF(WLAN_ASSOC_RSP_FRAME_T, aucInfoElem[0]) - WLAN_MAC_MGMT_HEADER_LEN));
	pucIE = prAssocRspFrame->aucInfoElem;

	/* 4 <3.2> Parse WMM and setup QBSS flag */
	/* Parse WMM related IEs and configure HW CRs accordingly */
	mqmProcessAssocRsp(prAdapter, prAssocRspSwRfb, pucIE, u2IELength);

	prAisBssInfo->fgIsQBSS = prStaRec->fgIsQoS;

	/* 3 <4> Update BSS_INFO_T from BSS_DESC_T */
	prBssDesc = prAisFsmInfo->prTargetBssDesc;
	if (prBssDesc) {
		prBssDesc->fgIsConnecting = FALSE;
		prBssDesc->fgIsConnected = TRUE;
		aisRemoveBlackList(prAdapter, prBssDesc, AIS_BLACK_LIST_FROM_DRIVER);
		prBssDesc->ucJoinFailureCount = 0;
		/* 4 <4.1> Setup MIB for current BSS */
		prAisBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;
	} else {
		/* should never happen */
		ASSERT(0);
	}

	/* NOTE: Defer ucDTIMPeriod updating to when beacon is received after connection */
	prAisBssInfo->ucDTIMPeriod = 0;
	prAisBssInfo->fgDTIMPresent = TRUE;
	prAisBssInfo->u2ATIMWindow = 0;

	prAisBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_INFRA;
	prAisBssInfo->ucRoamSkipTimes = 3;
	prAisBssInfo->fgGoodRcpiArea = FALSE;
	prAisBssInfo->fgPoorRcpiArea = FALSE;

	/* 4 <4.2> Update HT information and set channel */
	/* Record HT related parameters in rStaRec and rBssInfo
	 * Note: it shall be called before nicUpdateBss()
	 */
	rlmProcessAssocRsp(prAdapter, prAssocRspSwRfb, pucIE, u2IELength);

	/* 4 <4.3> Sync with firmware for BSS-INFO */
	nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 4 <4.4> *DEFER OPERATION* nicPmIndicateBssConnected() will be invoked */
	/* inside scanProcessBeaconAndProbeResp() after 1st beacon is received */

}				/* end of aisUpdateBssInfoForJOIN() */

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
* @brief This function will create an Ad-Hoc network and start sending Beacon Frames.
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisUpdateBssInfoForCreateIBSS(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_INFO_T prAisBssInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	if (prAisBssInfo->fgIsBeaconActivated)
		return;

	/* 3 <1> Update BSS_INFO_T per Network Basis */
	/* 4 <1.1> Setup Operation Mode */
	prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

	/* 4 <1.2> Setup SSID */
	COPY_SSID(prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen, prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

	/* 4 <1.3> Clear current AP's STA_RECORD_T and current AID */
	prAisBssInfo->prStaRecOfAP = (P_STA_RECORD_T) NULL;
	prAisBssInfo->u2AssocId = 0;

	/* 4 <1.4> Setup Channel, Band and Phy Attributes */
	prAisBssInfo->ucPrimaryChannel = prConnSettings->ucAdHocChannelNum;
	prAisBssInfo->eBand = prConnSettings->eAdHocBand;

	if (prAisBssInfo->eBand == BAND_2G4) {
		/* Depend on eBand */
		prAisBssInfo->ucPhyTypeSet = prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11BGN;
		/* Depend on eCurrentOPMode and ucPhyTypeSet */
		prAisBssInfo->ucConfigAdHocAPMode = AD_HOC_MODE_MIXED_11BG;
	} else {
		/* Depend on eBand */
		prAisBssInfo->ucPhyTypeSet = prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11ANAC;
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
	    prConnSettings->eEncStatus == ENUM_ENCRYPTION3_ENABLED) {
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
	nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 4 <3.3> enable beaconing */
	bssUpdateBeaconContent(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 4 <3.4> Update AdHoc PM parameter */
	nicPmIndicateBssCreated(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 3 <4> Set ACTIVE flag. */
	prAisBssInfo->fgIsBeaconActivated = TRUE;
	prAisBssInfo->fgHoldSameBssidForIBSS = TRUE;

	/* 3 <5> Start IBSS Alone Timer */
	cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer, SEC_TO_MSEC(AIS_IBSS_ALONE_TIMEOUT_SEC));

	return;

}				/* end of aisCreateIBSS() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will update the contain of BSS_INFO_T for AIS network once
*        the existing IBSS was found.
*
* @param[in] prStaRec               Pointer to the STA_RECORD_T
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisUpdateBssInfoForMergeIBSS(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_INFO_T prAisBssInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_DESC_T prBssDesc;
	/* UINT_16 u2IELength; */
	/* PUINT_8 pucIE; */

	ASSERT(prStaRec);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer);

	if (!prAisBssInfo->fgIsBeaconActivated) {

		/* 3 <1> Update BSS_INFO_T per Network Basis */
		/* 4 <1.1> Setup Operation Mode */
		prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

		/* 4 <1.2> Setup SSID */
		COPY_SSID(prAisBssInfo->aucSSID,
			  prAisBssInfo->ucSSIDLen, prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

		/* 4 <1.3> Clear current AP's STA_RECORD_T and current AID */
		prAisBssInfo->prStaRecOfAP = (P_STA_RECORD_T) NULL;
		prAisBssInfo->u2AssocId = 0;
	}
	/* 3 <2> Update BSS_INFO_T from STA_RECORD_T */
	/* 4 <2.1> Setup Capability */
	prAisBssInfo->u2CapInfo = prStaRec->u2CapInfo;	/* Use Peer's Cap Info as IBSS Cap Info */

	if (prAisBssInfo->u2CapInfo & CAP_INFO_SHORT_PREAMBLE) {
		prAisBssInfo->fgIsShortPreambleAllowed = TRUE;
		prAisBssInfo->fgUseShortPreamble = TRUE;
	} else {
		prAisBssInfo->fgIsShortPreambleAllowed = FALSE;
		prAisBssInfo->fgUseShortPreamble = FALSE;
	}

	/* 7.3.1.4 For IBSS, the Short Slot Time subfield shall be set to 0. */
	prAisBssInfo->fgUseShortSlotTime = FALSE;	/* Set to FALSE for AdHoc */
	prAisBssInfo->u2CapInfo &= ~CAP_INFO_SHORT_SLOT_TIME;

	if (prAisBssInfo->u2CapInfo & CAP_INFO_PRIVACY)
		prAisBssInfo->fgIsProtection = TRUE;
	else
		prAisBssInfo->fgIsProtection = FALSE;

	/* 4 <2.2> Setup PHY Attributes and Basic Rate Set/Operational Rate Set */
	prAisBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

	prAisBssInfo->ucNonHTBasicPhyType = prStaRec->ucNonHTBasicPhyType;

	prAisBssInfo->u2OperationalRateSet = prStaRec->u2OperationalRateSet;
	prAisBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

	rateGetDataRatesFromRateSet(prAisBssInfo->u2OperationalRateSet,
				    prAisBssInfo->u2BSSBasicRateSet,
				    prAisBssInfo->aucAllSupportedRates, &prAisBssInfo->ucAllSupportedRatesLen);

	/* 3 <3> X Update BSS_INFO_T from SW_RFB_T (Association Resp Frame) */

	/* 3 <4> Update BSS_INFO_T from BSS_DESC_T */
	prBssDesc = scanSearchBssDescByTA(prAdapter, prStaRec->aucMacAddr);
	if (prBssDesc) {
		prBssDesc->fgIsConnecting = FALSE;
		prBssDesc->fgIsConnected = TRUE;

		/* 4 <4.1> Setup BSSID */
		COPY_MAC_ADDR(prAisBssInfo->aucBSSID, prBssDesc->aucBSSID);

		/* 4 <4.2> Setup Channel, Band */
		prAisBssInfo->ucPrimaryChannel = prBssDesc->ucChannelNum;
		prAisBssInfo->eBand = prBssDesc->eBand;

		/* 4 <4.3> Setup MIB for current BSS */
		prAisBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;
		prAisBssInfo->ucDTIMPeriod = 0;
		prAisBssInfo->u2ATIMWindow = 0;	/* TBD(Kevin) */

		prAisBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_ADHOC;
	} else {
		/* should never happen */
		ASSERT(0);
	}

	/* 3 <5> Set MAC HW */
	/* 4 <5.1> Find Lowest Basic Rate Index for default TX Rate of MMPDU */
	nicTxUpdateBssDefaultRate(prAisBssInfo);

	/* 4 <5.2> Setup channel and bandwidth */
	rlmBssInitForAPandIbss(prAdapter, prAisBssInfo);

	/* 4 <5.3> use command packets to inform firmware */
	nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 4 <5.4> enable beaconing */
	bssUpdateBeaconContent(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 4 <5.5> Update AdHoc PM parameter */
	nicPmIndicateBssConnected(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 3 <6> Set ACTIVE flag. */
	prAisBssInfo->fgIsBeaconActivated = TRUE;
	prAisBssInfo->fgHoldSameBssidForIBSS = TRUE;

}				/* end of aisUpdateBssInfoForMergeIBSS() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will validate the Rx Probe Request Frame and then return
*        result to BSS to indicate if need to send the corresponding Probe Response
*        Frame if the specified conditions were matched.
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] prSwRfb            Pointer to SW RFB data structure.
* @param[out] pu4ControlFlags   Control flags for replying the Probe Response
*
* @retval TRUE      Reply the Probe Response
* @retval FALSE     Don't reply the Probe Response
*/
/*----------------------------------------------------------------------------*/
BOOLEAN aisValidateProbeReq(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb, OUT PUINT_32 pu4ControlFlags)
{
	P_WLAN_MAC_MGMT_HEADER_T prMgtHdr;
	P_BSS_INFO_T prBssInfo;
	P_IE_SSID_T prIeSsid = (P_IE_SSID_T) NULL;
	PUINT_8 pucIE;
	UINT_16 u2IELength;
	UINT_16 u2Offset = 0;
	BOOLEAN fgReplyProbeResp = FALSE;

	ASSERT(prSwRfb);
	ASSERT(pu4ControlFlags);

	prBssInfo = prAdapter->prAisBssInfo;

	/* 4 <1> Parse Probe Req IE and Get IE ptr (SSID, Supported Rate IE, ...) */
	prMgtHdr = (P_WLAN_MAC_MGMT_HEADER_T) prSwRfb->pvHeader;

	u2IELength = prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen;
	pucIE = (PUINT_8) ((ULONG) prSwRfb->pvHeader + prSwRfb->u2HeaderLen);

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		if (IE_ID(pucIE) == ELEM_ID_SSID) {
			if ((!prIeSsid) && (IE_LEN(pucIE) <= ELEM_MAX_LEN_SSID))
				prIeSsid = (P_IE_SSID_T) pucIE;

			break;
		}
	}			/* end of IE_FOR_EACH */

	/* 4 <2> Check network conditions */

	if (prBssInfo->eCurrentOPMode == OP_MODE_IBSS) {

		if ((prIeSsid) && ((prIeSsid->ucLength == BC_SSID_LEN) ||	/* WILDCARD SSID */
				   EQUAL_SSID(prBssInfo->aucSSID, prBssInfo->ucSSIDLen,	/* CURRENT SSID */
					      prIeSsid->aucSSID, prIeSsid->ucLength))) {
			fgReplyProbeResp = TRUE;
		}
	}

	return fgReplyProbeResp;

}				/* end of aisValidateProbeReq() */

#endif /* CFG_SUPPORT_ADHOC */

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
VOID aisFsmDisconnect(IN P_ADAPTER_T prAdapter, IN BOOLEAN fgDelayIndication)
{
	P_BSS_INFO_T prAisBssInfo;
	UINT_8 aucP2pSsid[] = CTIA_MAGIC_SSID;

	ASSERT(prAdapter);

	prAisBssInfo = prAdapter->prAisBssInfo;

	cnmTimerStopTimer(prAdapter, &prAdapter->rWifiVar.rAisFsmInfo.rSecModeChangeTimer);
	nicPmIndicateBssAbort(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

#if CFG_SUPPORT_ADHOC
	if (prAisBssInfo->fgIsBeaconActivated) {
		nicUpdateBeaconIETemplate(prAdapter,
					  IE_UPD_METHOD_DELETE_ALL, prAdapter->prAisBssInfo->ucBssIndex, 0, NULL, 0);

		prAisBssInfo->fgIsBeaconActivated = FALSE;
	}
#endif

	rlmBssAborted(prAdapter, prAisBssInfo);

	/* 4 <3> Unset the fgIsConnected flag of BSS_DESC_T and send Deauth if needed. */
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {

		/* add for ctia mode */
		{

			if (EQUAL_SSID(aucP2pSsid, CTIA_MAGIC_SSID_LEN, prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen))
				nicEnterCtiaMode(prAdapter, FALSE, FALSE);
		}
		/* for NO TIM IE case */
		if (!prAisBssInfo->fgDTIMPresent) {
			nicPowerSaveInfoMap(prAdapter, prAisBssInfo->ucBssIndex,
					    Param_PowerModeFast_PSP, PS_CALLER_NO_TIM);
			nicConfigPowerSaveProfile(prAdapter,
						  prAisBssInfo->ucBssIndex,
						  Param_PowerModeFast_PSP, FALSE);
		}

		prAdapter->rWifiVar.rAisFsmInfo.prTargetBssDesc->fgIsConnected = FALSE;
		prAdapter->rWifiVar.rAisFsmInfo.prTargetBssDesc->fgIsConnecting = FALSE;
		if (prAisBssInfo->ucReasonOfDisconnect == DISCONNECT_REASON_CODE_RADIO_LOST) {
#if CFG_SUPPORT_RN
			if (prAisBssInfo->fgDisConnReassoc == FALSE)
#endif
				{
					scanRemoveBssDescByBssid(prAdapter, prAisBssInfo->aucBSSID);
					/* remove from scanning results as well */
					wlanClearBssInScanningResult(prAdapter, prAisBssInfo->aucBSSID);
				}

			/* trials for re-association */
			if (fgDelayIndication) {
				aisFsmIsRequestPending(prAdapter, AIS_REQUEST_RECONNECT, TRUE);
				aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);
			}
		}

		if (fgDelayIndication) {
			if (prAisBssInfo->eCurrentOPMode != OP_MODE_IBSS)
				prAisBssInfo->fgHoldSameBssidForIBSS = FALSE;
		} else {
			prAisBssInfo->fgHoldSameBssidForIBSS = FALSE;
		}
	} else {
		prAisBssInfo->fgHoldSameBssidForIBSS = FALSE;
	}

	/* 4 <4> Change Media State immediately. */
	if (prAisBssInfo->ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION) {
		aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_DISCONNECTED);

		/* 4 <4.1> sync. with firmware */
		nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
	}

	if (!fgDelayIndication) {
		if (prAisBssInfo->ucReasonOfDisconnect != DISCONNECT_REASON_CODE_NEW_CONNECTION)
			prAdapter->rWifiVar.rConnSettings.ucSSIDLen = 0;
		/* 4 <5> Deactivate previous AP's STA_RECORD_T or all Clients in Driver if have. */
		if (prAisBssInfo->prStaRecOfAP) {
			/* cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP, STA_STATE_1); */
			prAisBssInfo->prStaRecOfAP = (P_STA_RECORD_T) NULL;
		}
	}
#if CFG_SUPPORT_ROAMING
	roamingFsmRunEventAbort(prAdapter);

	/* clear pending roaming connection request */
	aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, TRUE);
	aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, TRUE);
#endif /* CFG_SUPPORT_ROAMING */

	/* 4 <6> Indicate Disconnected Event to Host */
	aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_DISCONNECTED, fgDelayIndication);

	/* 4 <7> Trigger AIS FSM */
	aisFsmSteps(prAdapter, AIS_STATE_IDLE);

}				/* end of aisFsmDisconnect() */

static VOID aisFsmRunEventScanDoneTimeOut(IN P_ADAPTER_T prAdapter, ULONG ulParam)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	ENUM_AIS_STATE_T eNextState;
	P_CONNECTION_SETTINGS_T prConnSettings;

	DEBUGFUNC("aisFsmRunEventScanDoneTimeOut()");

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	DBGLOG(AIS, STATE, "aisFsmRunEventScanDoneTimeOut Current[%d]\n", prAisFsmInfo->eCurrentState);

	/* report all scanned frames to upper layer to avoid scanned frame is timeout */
	/* must be put before kalScanDone */
/* scanReportBss2Cfg80211(prAdapter,BSS_TYPE_INFRASTRUCTURE,NULL); */

	prConnSettings->fgIsScanReqIssued = FALSE;
	kalScanDone(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX, WLAN_STATUS_SUCCESS);
	eNextState = prAisFsmInfo->eCurrentState;

	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_SCAN:
		prAisFsmInfo->u4ScanIELength = 0;
		eNextState = AIS_STATE_IDLE;
		break;
	case AIS_STATE_ONLINE_SCAN:
		/* reset scan IE buffer */
		prAisFsmInfo->u4ScanIELength = 0;
#if CFG_SUPPORT_ROAMING
		eNextState = aisFsmRoamingScanResultsUpdate(prAdapter);
#else
		eNextState = AIS_STATE_NORMAL_TR;
#endif /* CFG_SUPPORT_ROAMING */
		scanGetCurrentEssChnlList(prAdapter);
		break;
	default:
		break;
	}

	/* try to stop scan in CONNSYS */
	aisFsmStateAbort_SCAN(prAdapter);
	aisRemoveOldestBcnTimeout(prAisFsmInfo);

/* wlanQueryDebugCode(prAdapter); */ /* display current SCAN FSM in FW, debug use */

	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);

}				/* end of aisFsmBGSleepTimeout() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will indicate an Event of "Background Scan Time-Out" to AIS FSM.
*
* @param[in] u4Param  Unused timer parameter
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmRunEventBGSleepTimeOut(IN P_ADAPTER_T prAdapter, ULONG ulParamPtr)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	ENUM_AIS_STATE_T eNextState;

	DEBUGFUNC("aisFsmRunEventBGSleepTimeOut()");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	eNextState = prAisFsmInfo->eCurrentState;

	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_WAIT_FOR_NEXT_SCAN:
		DBGLOG(AIS, LOUD, "EVENT - SCAN TIMER: Idle End - Current Time = %u\n", kalGetTimeTick());

		eNextState = AIS_STATE_LOOKING_FOR;

		SET_NET_PWR_STATE_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

		break;

	default:
		break;
	}

	/* Call aisFsmSteps() when we are going to change AIS STATE */
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);

}				/* end of aisFsmBGSleepTimeout() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will indicate an Event of "IBSS ALONE Time-Out" to AIS FSM.
*
* @param[in] u4Param  Unused timer parameter
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmRunEventIbssAloneTimeOut(IN P_ADAPTER_T prAdapter, ULONG ulParamPtr)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	ENUM_AIS_STATE_T eNextState;

	DEBUGFUNC("aisFsmRunEventIbssAloneTimeOut()");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	eNextState = prAisFsmInfo->eCurrentState;

	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_IBSS_ALONE:

		/* There is no one participate in our AdHoc during this TIMEOUT Interval
		 * so go back to search for a valid IBSS again.
		 */

		DBGLOG(AIS, LOUD, "EVENT-IBSS ALONE TIMER: Start pairing\n");

		prAisFsmInfo->fgTryScan = TRUE;

		/* abort timer */
		aisFsmReleaseCh(prAdapter);

		/* Pull back to SEARCH to find candidate again */
		eNextState = AIS_STATE_SEARCH;

		break;

	default:
		break;
	}

	/* Call aisFsmSteps() when we are going to change AIS STATE */
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);

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
VOID aisFsmRunEventJoinTimeout(IN P_ADAPTER_T prAdapter, ULONG ulParamPtr)
{
	P_BSS_INFO_T prAisBssInfo;
	P_AIS_FSM_INFO_T prAisFsmInfo;
	ENUM_AIS_STATE_T eNextState;
	OS_SYSTIME rCurrentTime;

	DEBUGFUNC("aisFsmRunEventJoinTimeout()");

	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	eNextState = prAisFsmInfo->eCurrentState;

	GET_CURRENT_SYSTIME(&rCurrentTime);

	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_JOIN:
		DBGLOG(AIS, WARN, "EVENT- JOIN TIMEOUT\n");

		/* 1. Do abort JOIN */
		aisFsmStateAbort_JOIN(prAdapter);

		/* 2. Increase Join Failure Count */
		aisAddBlacklist(prAdapter, prAisFsmInfo->prTargetBssDesc, AIS_BLACK_LIST_FROM_DRIVER);

		prAisFsmInfo->prTargetBssDesc->ucJoinFailureCount++;

		if (prAisFsmInfo->prTargetBssDesc->ucJoinFailureCount < JOIN_MAX_RETRY_FAILURE_COUNT) {
			/* 3.1 Retreat to AIS_STATE_SEARCH state for next try */
			eNextState = AIS_STATE_SEARCH;
		} else if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
			/* roaming cases */
			/* 3.2 Retreat to AIS_STATE_WAIT_FOR_NEXT_SCAN state for next try */
			eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
		} else if (prAisFsmInfo->rJoinReqTime != 0 &&
			!CHECK_FOR_TIMEOUT(rCurrentTime, prAisFsmInfo->rJoinReqTime,
					   SEC_TO_SYSTIME(AIS_JOIN_TIMEOUT))) {
			/* 3.3 Retreat to AIS_STATE_WAIT_FOR_NEXT_SCAN state for next try */
			eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
		} else {
			/* 3.4 Retreat to AIS_STATE_JOIN_FAILURE to terminate join operation */
			eNextState = AIS_STATE_JOIN_FAILURE;
			prAisFsmInfo->prTargetBssDesc->u2JoinStatus = STATUS_CODE_AUTH_TIMEOUT;
		}

		break;

	case AIS_STATE_NORMAL_TR:
		/* 1. release channel */
		aisFsmReleaseCh(prAdapter);
		prAisFsmInfo->fgIsInfraChannelFinished = TRUE;

		/* 2. process if there is pending scan */
		if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, TRUE) == TRUE) {
			wlanClearScanningResult(prAdapter);
			eNextState = AIS_STATE_ONLINE_SCAN;
		}
		/* 3. Process for pending roaming scan */
		else if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, TRUE) == TRUE)
			eNextState = AIS_STATE_LOOKING_FOR;
		/* 4. Process for pending roaming scan */
		else if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, TRUE) == TRUE)
			eNextState = AIS_STATE_COLLECT_ESS_INFO;
		else if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL, TRUE) == TRUE)
			eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;

		break;

	default:
		/* release channel */
		aisFsmReleaseCh(prAdapter);
		break;

	}

	/* Call aisFsmSteps() when we are going to change AIS STATE */
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);

}				/* end of aisFsmRunEventJoinTimeout() */

VOID aisFsmRunEventDeauthTimeout(IN P_ADAPTER_T prAdapter, ULONG ulParamPtr)
{
	aisDeauthXmitComplete(prAdapter, NULL, TX_RESULT_LIFE_TIMEOUT);
}

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
VOID aisFsmRunEventSecModeChangeTimeout(IN P_ADAPTER_T prAdapter, ULONG ulParamPtr)
{
	DBGLOG(AIS, INFO, "Beacon security mode change timeout, trigger disconnect!\n");
	aisBssSecurityChanged(prAdapter);
}
#endif
#if defined(CFG_TEST_MGMT_FSM) && (CFG_TEST_MGMT_FSM != 0)
/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID aisTest(VOID)
{
	P_MSG_AIS_ABORT_T prAisAbortMsg;
	P_CONNECTION_SETTINGS_T prConnSettings;
	UINT_8 aucSSID[] = "pci-11n";
	UINT_8 ucSSIDLen = 7;

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	/* Set Connection Request Issued Flag */
	prConnSettings->fgIsConnReqIssued = TRUE;
	prConnSettings->ucSSIDLen = ucSSIDLen;
	kalMemCopy(prConnSettings->aucSSID, aucSSID, ucSSIDLen);

	prAisAbortMsg = (P_MSG_AIS_ABORT_T) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_AIS_ABORT_T));
	if (!prAisAbortMsg) {

		ASSERT(0);	/* Can't trigger SCAN FSM */
		return;
	}

	prAisAbortMsg->rMsgHdr.eMsgId = MID_HEM_AIS_FSM_ABORT;

	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prAisAbortMsg, MSG_SEND_METHOD_BUF);

	wifi_send_msg(INDX_WIFI, MSG_ID_WIFI_IST, 0);

}
#endif /* CFG_TEST_MGMT_FSM */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is used to handle OID_802_11_BSSID_LIST_SCAN
*
* \param[in] prAdapter  Pointer of ADAPTER_T
* \param[in] prSsid     Pointer of SSID_T if specified
* \param[in] pucIe      Pointer to buffer of extra information elements to be attached
* \param[in] u4IeLength Length of information elements
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmScanRequest(IN P_ADAPTER_T prAdapter, IN P_PARAM_SSID_T prSsid, IN PUINT_8 pucIe, IN UINT_32 u4IeLength)
{
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_INFO_T prAisBssInfo;
	P_AIS_FSM_INFO_T prAisFsmInfo;

	DEBUGFUNC("aisFsmScanRequest()");

	ASSERT(prAdapter);
	ASSERT(u4IeLength <= MAX_IE_LENGTH);

	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	DBGLOG(AIS, TRACE, "eCurrentState=%d, fgIsScanReqIssued=%d\n",
		prAisFsmInfo->eCurrentState, prConnSettings->fgIsScanReqIssued);
	if (!prConnSettings->fgIsScanReqIssued) {
		prConnSettings->fgIsScanReqIssued = TRUE;
		scanInitEssResult(prAdapter);

		if (prSsid == NULL) {
			prAisFsmInfo->ucScanSSIDNum = 0;
		} else {
			prAisFsmInfo->ucScanSSIDNum = 1;

			COPY_SSID(prAisFsmInfo->arScanSSID[0].aucSsid,
				  prAisFsmInfo->arScanSSID[0].u4SsidLen, prSsid->aucSsid, prSsid->u4SsidLen);
		}

		if (u4IeLength > 0 && u4IeLength <= MAX_IE_LENGTH) {
			prAisFsmInfo->u4ScanIELength = u4IeLength;
			kalMemCopy(prAisFsmInfo->aucScanIEBuf, pucIe, u4IeLength);
		} else {
			prAisFsmInfo->u4ScanIELength = 0;
		}

		if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
			if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE
			    && prAisFsmInfo->fgIsInfraChannelFinished == FALSE) {
				/* 802.1x might not finished yet, pend it for later handling .. */
				aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
			} else {
				if (prAisFsmInfo->fgIsChannelGranted == TRUE) {
					DBGLOG(AIS, WARN,
					       "Scan Request with channel granted for join operation: %d, %d",
						prAisFsmInfo->fgIsChannelGranted, prAisFsmInfo->fgIsChannelRequested);
				}

				/* start online scan */
				wlanClearScanningResult(prAdapter);
				aisFsmSteps(prAdapter, AIS_STATE_ONLINE_SCAN);
			}
		} else if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE) {
			wlanClearScanningResult(prAdapter);
			aisFsmSteps(prAdapter, AIS_STATE_SCAN);
		} else {
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
		}
	} else {
		DBGLOG(AIS, WARN, "Scan Request dropped. (state: %d)\n", prAisFsmInfo->eCurrentState);
	}

}	/* end of aisFsmScanRequest() */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is used to handle OID_802_11_BSSID_LIST_SCAN
*
* \param[in] prAdapter  Pointer of ADAPTER_T
* \param[in] ucSsidNum  Number of SSID
* \param[in] prSsid     Pointer to the array of SSID_T if specified
* \param[in] pucIe      Pointer to buffer of extra information elements to be attached
* \param[in] u4IeLength Length of information elements
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID
aisFsmScanRequestAdv(IN P_ADAPTER_T prAdapter, IN UINT_8 ucSsidNum, IN P_PARAM_SSID_T prSsid, IN PUINT_8 pucIe,
	IN UINT_32 u4IeLength,  IN UINT_8 ucSetChannel, IN struct _PARAM_SCAN_RANDOM_MAC_ADDR_T *prScanRandMacAddr)
{
	UINT_32 i;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_INFO_T prAisBssInfo;
	P_AIS_FSM_INFO_T prAisFsmInfo;

	DEBUGFUNC("aisFsmScanRequestAdv()");

	ASSERT(prAdapter);
	ASSERT(ucSsidNum <= SCN_SSID_MAX_NUM);
	ASSERT(u4IeLength <= MAX_IE_LENGTH);
	ASSERT(prScanRandMacAddr);

	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	DBGLOG(AIS, TRACE, "eCurrentState=%d, fgIsScanReqIssued=%d\n",
			prAisFsmInfo->eCurrentState, prConnSettings->fgIsScanReqIssued);

	if (!prConnSettings->fgIsScanReqIssued) {
		prConnSettings->fgIsScanReqIssued = TRUE;
		scanInitEssResult(prAdapter);

		if (ucSsidNum == 0) {
			prAisFsmInfo->ucScanSSIDNum = 0;
		} else {
			prAisFsmInfo->ucScanSSIDNum = ucSsidNum;

			for (i = 0; i < ucSsidNum; i++) {
				COPY_SSID(prAisFsmInfo->arScanSSID[i].aucSsid,
					  prAisFsmInfo->arScanSSID[i].u4SsidLen,
					  prSsid[i].aucSsid, prSsid[i].u4SsidLen);
			}
		}

		if (u4IeLength > 0 && u4IeLength <= MAX_IE_LENGTH) {
			prAisFsmInfo->u4ScanIELength = u4IeLength;
			kalMemCopy(prAisFsmInfo->aucScanIEBuf, pucIe, u4IeLength);
		} else {
			prAisFsmInfo->u4ScanIELength = 0;
		}

		/* save and set partial scan info */
		if (ucSetChannel) {
			prAdapter->prGlueInfo->pucScanChannel = (PUINT_8)&(prAdapter->prGlueInfo->rScanChannelInfo);
			DBGLOG(AIS, TRACE, "partial scan set pucScanChannel\n");
		}
		kalMemCopy(&(prAisFsmInfo->rScanRandMacAddr), prScanRandMacAddr,
			sizeof(struct _PARAM_SCAN_RANDOM_MAC_ADDR_T));
		if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
			if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE
			    && prAisFsmInfo->fgIsInfraChannelFinished == FALSE) {
				/* 802.1x might not finished yet, pend it for later handling .. */
				aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
			} else {
				if (prAisFsmInfo->fgIsChannelGranted == TRUE) {
					DBGLOG(AIS, WARN,
					       "Scan Request with channel granted for join operation: %d, %d",
						prAisFsmInfo->fgIsChannelGranted, prAisFsmInfo->fgIsChannelRequested);
				}

				/* start online scan */
				wlanClearScanningResult(prAdapter);
				aisFsmSteps(prAdapter, AIS_STATE_ONLINE_SCAN);
			}
		} else if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE) {
			wlanClearScanningResult(prAdapter);
			aisFsmSteps(prAdapter, AIS_STATE_SCAN);
		} else {
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
		}
	} else if (prAdapter->rWifiVar.rRmReqParams.rBcnRmParam.eState == RM_ON_GOING) {
		struct NORMAL_SCAN_PARAMS *prNormalScan = &prAdapter->rWifiVar.rRmReqParams.rBcnRmParam.rNormalScan;

		prNormalScan->fgExist = TRUE;
		if (ucSsidNum == 0) {
			prNormalScan->ucSsidNum = 0;
		} else {
			prNormalScan->ucSsidNum = ucSsidNum;

			for (i = 0; i < ucSsidNum; i++) {
				COPY_SSID(prNormalScan->arSSID[i].aucSsid,
					  prNormalScan->arSSID[i].u4SsidLen,
					  prSsid[i].aucSsid, prSsid[i].u4SsidLen);
			}
		}

		if (u4IeLength > 0 && u4IeLength <= MAX_IE_LENGTH) {
			prNormalScan->u4IELen = u4IeLength;
			kalMemCopy(prNormalScan->aucScanIEBuf, pucIe, u4IeLength);
		} else {
			prNormalScan->u4IELen = 0;
		}
		prNormalScan->fgFull2Partial = ucSetChannel;
		kalMemCopy(&(prNormalScan->rScanRandMacAddr), prScanRandMacAddr,
			sizeof(struct _PARAM_SCAN_RANDOM_MAC_ADDR_T));
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);
		DBGLOG(AIS, INFO, "Buffer normal scan while Beacon request measurement\n");
	} else {
		if (ucSetChannel) {
			/*need reset pucScanChannel and memset rScanChannelInfo*/
			kalMemSet(&(prAdapter->prGlueInfo->rScanChannelInfo), 0, sizeof(PARTIAL_SCAN_INFO));
			prAdapter->prGlueInfo->pucScanChannel = NULL;
		}

		DBGLOG(AIS, WARN, "Scan Request dropped. (state: %d)\n", prAisFsmInfo->eCurrentState);
	}

} /* end of aisFsmScanRequestAdv() */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is invoked when CNM granted channel privilege
*
* \param[in] prAdapter  Pointer of ADAPTER_T
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmRunEventChGrant(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_BSS_INFO_T prAisBssInfo;
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_MSG_CH_GRANT_T prMsgChGrant;
	UINT_8 ucTokenID;
	UINT_32 u4GrantInterval;
	UINT_32 u4Entry = 0;

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prMsgChGrant = (P_MSG_CH_GRANT_T) prMsgHdr;

	ucTokenID = prMsgChGrant->ucTokenID;
	u4GrantInterval = prMsgChGrant->u4GrantInterval;

	/* 1. free message */
	cnmMemFree(prAdapter, prMsgHdr);

	if (prAisFsmInfo->ucSeqNumOfChReq != ucTokenID) { /* mismatched grant */
		DBGLOG(AIS, WARN, "Ignore token(Cur %d, Grant %d) mismatched channel grant\n",
		       prAisFsmInfo->ucSeqNumOfChReq, ucTokenID);
		return;
	}
	if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN) {
		/* 2. channel privilege has been approved */
		prAisFsmInfo->u4ChGrantedInterval = u4GrantInterval;

		/* 3. state transition to join/ibss-alone/ibss-merge */
		/* 3.1 set timeout timer in cases join could not be completed */
		cnmTimerStartTimer(prAdapter,
				   &prAisFsmInfo->rJoinTimeoutTimer,
				   prAisFsmInfo->u4ChGrantedInterval - AIS_JOIN_CH_GRANT_THRESHOLD);
		/* 3.2 set local variable to indicate join timer is ticking */
		prAisFsmInfo->fgIsInfraChannelFinished = FALSE;

		/* 3.3 switch to join state */
		/* Three cases can switch to join state:
		** 1. There's an available PMKSA in wpa_supplicant
		** 2. found okc pmkid entry for this BSS
		** 3. current state is disconnected. In this case, supplicant may not get a valid pmksa,
		** so no pmkid will be passed to driver, so we no need to wait pmkid anyway.
		*/
		if (!prAdapter->rWifiVar.rConnSettings.fgOkcPmksaReady ||
			(rsnSearchPmkidEntry(prAdapter, prAisFsmInfo->prTargetBssDesc->aucBSSID, &u4Entry) &&
			prAdapter->rWifiVar.rAisSpecificBssInfo.arPmkidCache[u4Entry].fgPmkidExist))
			aisFsmSteps(prAdapter, AIS_STATE_JOIN);

		prAisFsmInfo->fgIsChannelGranted = TRUE;
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_REMAIN_ON_CHANNEL) {
		/* 2. channel privilege has been approved */
		prAisFsmInfo->u4ChGrantedInterval = u4GrantInterval;

		/* 3.1 set timeout timer in cases upper layer cancel_remain_on_channel never comes */
		cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer, prAisFsmInfo->u4ChGrantedInterval);

		/* 3.2 switch to remain_on_channel state */
		aisFsmSteps(prAdapter, AIS_STATE_REMAIN_ON_CHANNEL);

		/* 3.3. indicate upper layer for channel ready */
		kalReadyOnChannel(prAdapter->prGlueInfo,
				  prAisFsmInfo->rChReqInfo.u8Cookie,
				  prAisFsmInfo->rChReqInfo.eBand,
				  prAisFsmInfo->rChReqInfo.eSco,
				  prAisFsmInfo->rChReqInfo.ucChannelNum, prAisFsmInfo->rChReqInfo.u4DurationMs);

		prAisFsmInfo->fgIsChannelGranted = TRUE;
	} else {
		/* 2. return channel privilege to CNM immediately */
		aisFsmReleaseCh(prAdapter);
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
VOID aisFsmReleaseCh(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_MSG_CH_ABORT_T prMsgChAbort;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	if (prAisFsmInfo->fgIsChannelGranted == TRUE || prAisFsmInfo->fgIsChannelRequested == TRUE) {

		prAisFsmInfo->fgIsChannelRequested = FALSE;
		prAisFsmInfo->fgIsChannelGranted = FALSE;

		/* 1. return channel privilege to CNM immediately */
		prMsgChAbort = (P_MSG_CH_ABORT_T) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_CH_ABORT_T));
		if (!prMsgChAbort) {
			ASSERT(0);	/* Can't release Channel to CNM */
			return;
		}

		prMsgChAbort->rMsgHdr.eMsgId = MID_MNY_CNM_CH_ABORT;
		prMsgChAbort->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
		prMsgChAbort->ucTokenID = prAisFsmInfo->ucSeqNumOfChReq;

		mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prMsgChAbort, MSG_SEND_METHOD_BUF);
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
VOID aisBssBeaconTimeout(IN P_ADAPTER_T prAdapter)
{
	P_BSS_INFO_T prAisBssInfo;
	BOOLEAN fgDoAbortIndication = FALSE;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_AIS_FSM_INFO_T prAisFsmInfo;

	ASSERT(prAdapter);

	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	glNotifyDrvStatus(BEACON_TIMEOUT, (PVOID)prAisBssInfo->aucBSSID);
	/* 4 <1> Diagnose Connection for Beacon Timeout Event */
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
		if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
			P_STA_RECORD_T prStaRec = prAisBssInfo->prStaRecOfAP;

			if (prStaRec)
				fgDoAbortIndication = TRUE;

		} else if (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
			fgDoAbortIndication = TRUE;
		}
	}
	/* 4 <2> invoke abort handler */
	if (fgDoAbortIndication && (prAdapter->rWifiVar.rAisFsmInfo.u4PostponeIndStartTime == 0)) {
		prConnSettings->fgIsDisconnectedByNonRequest = FALSE;
		if (prConnSettings->eReConnectLevel < RECONNECT_LEVEL_USER_SET) {
			prConnSettings->eReConnectLevel = RECONNECT_LEVEL_BEACON_TIMEOUT;
			prConnSettings->fgIsConnReqIssued = TRUE;
		}
		prAisFsmInfo->ucConnTrialCount = 0;
		aisFsmStateAbort(prAdapter, DISCONNECT_REASON_CODE_RADIO_LOST, TRUE);
	}

}				/* end of aisBssBeaconTimeout() */

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
VOID aisBssSecurityChanged(P_ADAPTER_T prAdapter)
{
	prAdapter->rWifiVar.rConnSettings.fgIsDisconnectedByNonRequest = TRUE;
	aisFsmStateAbort(prAdapter, DISCONNECT_REASON_CODE_DEAUTHENTICATED, FALSE);
}
#endif

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
WLAN_STATUS
aisDeauthXmitComplete(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo, IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	if (rTxDoneStatus == TX_RESULT_SUCCESS)
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rDeauthDoneTimer);

	if (prAisFsmInfo->eCurrentState == AIS_STATE_DISCONNECTING) {
		if (rTxDoneStatus != TX_RESULT_DROPPED_IN_DRIVER && rTxDoneStatus != TX_RESULT_QUEUE_CLEARANCE)
			aisFsmStateAbort(prAdapter, DISCONNECT_REASON_CODE_NEW_CONNECTION, FALSE);
	} else {
		DBGLOG(AIS, WARN, "DEAUTH frame transmitted without further handling");
	}

	return WLAN_STATUS_SUCCESS;

}				/* end of aisDeauthXmitComplete() */

#if CFG_SUPPORT_ROAMING
/*----------------------------------------------------------------------------*/
/*!
* @brief This function will indicate an Event of "Looking for a candidate due to weak signal" to AIS FSM.
*
* @param[in] u4ReqScan  Requesting Scan or not
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aisFsmRunEventRoamingDiscovery(IN P_ADAPTER_T prAdapter, UINT_32 u4ReqScan)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;
	ENUM_AIS_REQUEST_TYPE_T eAisRequest = AIS_REQUEST_NUM;

	DBGLOG(AIS, LOUD, "aisFsmRunEventRoamingDiscovery()\n");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	/* search candidates by best rssi */
	prConnSettings->eConnectionPolicy = CONNECT_BY_SSID_BEST_RSSI;

	/* TODO: Stop roaming event in FW */
#if CFG_SUPPORT_WFD
#if CFG_ENABLE_WIFI_DIRECT
	{
		/* Check WFD is running */
		P_WFD_CFG_SETTINGS_T prWfdCfgSettings = (P_WFD_CFG_SETTINGS_T) NULL;

		prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);
		if ((prWfdCfgSettings->ucWfdEnable != 0)) {
			DBGLOG(ROAMING, INFO, "WFD is running. Stop roaming[WfdEnable:%u]\n",
				prWfdCfgSettings->ucWfdEnable);
			roamingFsmRunEventRoam(prAdapter);
			roamingFsmRunEventFail(prAdapter, ROAMING_FAIL_REASON_NOCANDIDATE);
			return;
		}
	}
#endif
#endif

	/* results are still new */
	if (!u4ReqScan) {
		roamingFsmRunEventRoam(prAdapter);
		eAisRequest = AIS_REQUEST_ROAMING_CONNECT;
	} else if (prAisFsmInfo->eCurrentState != AIS_STATE_ONLINE_SCAN
		   && prAisFsmInfo->eCurrentState != AIS_STATE_LOOKING_FOR)
		eAisRequest = AIS_REQUEST_ROAMING_SEARCH;

	if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR && prAisFsmInfo->fgIsInfraChannelFinished == TRUE) {
		if (eAisRequest == AIS_REQUEST_ROAMING_SEARCH)
			aisFsmSteps(prAdapter, AIS_STATE_LOOKING_FOR);
		else
			aisFsmSteps(prAdapter, AIS_STATE_COLLECT_ESS_INFO);
	} else {
		aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, TRUE);
		aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, TRUE);
		if (eAisRequest != AIS_REQUEST_NUM)
			aisFsmInsertRequest(prAdapter, eAisRequest);
	}
}				/* end of aisFsmRunEventRoamingDiscovery() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Update the time of ScanDone for roaming and transit to Roam state.
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
ENUM_AIS_STATE_T aisFsmRoamingScanResultsUpdate(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_ROAMING_INFO_T prRoamingFsmInfo;
	ENUM_AIS_STATE_T eNextState;

	DBGLOG(AIS, LOUD, "->aisFsmRoamingScanResultsUpdate()\n");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prRoamingFsmInfo = (P_ROAMING_INFO_T) &(prAdapter->rWifiVar.rRoamingInfo);

	roamingFsmScanResultsUpdate(prAdapter);

	eNextState = prAisFsmInfo->eCurrentState;
	if (prRoamingFsmInfo->eCurrentState == ROAMING_STATE_DISCOVERY) {
		roamingFsmRunEventRoam(prAdapter);
		eNextState = AIS_STATE_COLLECT_ESS_INFO;
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR) {
		eNextState = AIS_STATE_COLLECT_ESS_INFO;
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN) {
		eNextState = AIS_STATE_NORMAL_TR;
	}

	return eNextState;
}				/* end of aisFsmRoamingScanResultsUpdate() */

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
VOID aisFsmRoamingDisconnectPrevAP(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prTargetStaRec)
{
	P_BSS_INFO_T prAisBssInfo;

	DBGLOG(AIS, LOUD, "aisFsmRoamingDisconnectPrevAP()");

	ASSERT(prAdapter);

	prAisBssInfo = prAdapter->prAisBssInfo;
	if (prAisBssInfo->prStaRecOfAP != prTargetStaRec)
		wmmNotifyDisconnected(prAdapter);

	nicPmIndicateBssAbort(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/*
	 * Not invoke rlmBssAborted() here to avoid prAisBssInfo->fg40mBwAllowed
	 * to be reset. RLM related parameters will be reset again when handling
	 * association response in rlmProcessAssocRsp(). 20110413
	 */
	/* rlmBssAborted(prAdapter, prAisBssInfo); */

	/* 4 <3> Unset the fgIsConnected flag of BSS_DESC_T and send Deauth if needed. */
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
		PARAM_SSID_T rSsid;
		P_BSS_DESC_T prBssDesc = NULL;

		COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen, prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen);
		prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter, prAisBssInfo->aucBSSID, TRUE, &rSsid);
		if (prBssDesc) {
			prBssDesc->fgIsConnected = FALSE;
			prBssDesc->fgIsConnecting = FALSE;
		}
	}
	/* 4 <4> Change Media State immediately. */
	aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_DISCONNECT_PREV);

	/* 4 <4.1> sync. with firmware */
	prTargetStaRec->ucBssIndex = (MAX_BSS_INDEX + 1);	/* Virtial BSSID */
	nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
	prTargetStaRec->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
	/* before deactivate previous AP, should move its pending MSDUs to the new AP */
	if (prAisBssInfo->prStaRecOfAP)
		if (prAisBssInfo->prStaRecOfAP != prTargetStaRec && prAisBssInfo->prStaRecOfAP->fgIsInUse) {
			qmMoveStaTxQueue(prAisBssInfo->prStaRecOfAP, prTargetStaRec);
			/* Currently, firmware just drop all previous AP's data packets, so driver no need to handle
			** waiting tx done status packets
			*/
#if 0
			nicTxHandleRoamingDone(prAdapter, prAisBssInfo->prStaRecOfAP, prTargetStaRec);
#endif
			cnmStaRecFree(prAdapter, prAisBssInfo->prStaRecOfAP);
		} else
			DBGLOG(AIS, WARN, "prStaRecOfAP is in use %d\n", prAisBssInfo->prStaRecOfAP->fgIsInUse);
	else
		DBGLOG(AIS, WARN, "NULL pointer of prAisBssInfo->prStaRecOfAP\n");
}				/* end of aisFsmRoamingDisconnectPrevAP() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will update the contain of BSS_INFO_T for AIS network once
*        the roaming was completed.
*
* @param IN prAdapter          Pointer to the Adapter structure.
*           prStaRec           StaRec of roaming AP
*           prAssocRspSwRfb
*
* @retval None
*/
/*----------------------------------------------------------------------------*/
VOID aisUpdateBssInfoForRoamingAP(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec, IN P_SW_RFB_T prAssocRspSwRfb)
{
	P_BSS_INFO_T prAisBssInfo;

	DBGLOG(AIS, LOUD, "aisUpdateBssInfoForRoamingAP()");

	ASSERT(prAdapter);

	prAisBssInfo = prAdapter->prAisBssInfo;

	/* 4 <1.1> Change FW's Media State immediately. */
	aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

	/* 4 <1.2> Deactivate previous AP's STA_RECORD_T in Driver if have. */
	if ((prAisBssInfo->prStaRecOfAP) &&
	    (prAisBssInfo->prStaRecOfAP != prStaRec) && (prAisBssInfo->prStaRecOfAP->fgIsInUse)) {
	    /* before deactivate previous AP, should move its pending MSDUs to the new AP */
		qmMoveStaTxQueue(prAisBssInfo->prStaRecOfAP, prStaRec);
		/* cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP, STA_STATE_1); */
		cnmStaRecFree(prAdapter, prAisBssInfo->prStaRecOfAP);
	}

	/* 4 <1.4> Update BSS_INFO_T */
	aisUpdateBssInfoForJOIN(prAdapter, prStaRec, prAssocRspSwRfb);

	/* 4 <1.3> Activate current AP's STA_RECORD_T in Driver. */
	cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);

	/* 4 <1.6> Indicate Connected Event to Host immediately. */
	/* Require BSSID, Association ID, Beacon Interval.. from AIS_BSS_INFO_T */
	aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_CONNECTED, FALSE);

}				/* end of aisFsmRoamingUpdateBss() */

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
BOOLEAN aisFsmIsRequestPending(IN P_ADAPTER_T prAdapter, IN ENUM_AIS_REQUEST_TYPE_T eReqType, IN BOOLEAN bRemove)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_AIS_REQ_HDR_T prPendingReqHdr, prPendingReqHdrNext;

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* traverse through pending request list */
	LINK_FOR_EACH_ENTRY_SAFE(prPendingReqHdr,
				 prPendingReqHdrNext, &(prAisFsmInfo->rPendingReqList), rLinkEntry, AIS_REQ_HDR_T) {
		/* check for specified type */
		if (prPendingReqHdr->eReqType == eReqType) {
			/* check if need to remove */
			if (bRemove == TRUE) {
				LINK_REMOVE_KNOWN_ENTRY(&(prAisFsmInfo->rPendingReqList),
							&(prPendingReqHdr->rLinkEntry));
				if (eReqType == AIS_REQUEST_SCAN) {
					if (prPendingReqHdr->pucChannelInfo != NULL) {
						DBGLOG(AIS, INFO, "partial scan req pu8ChannelInfo no NULL\n");
						prAdapter->prGlueInfo->pucScanChannel = prPendingReqHdr->pucChannelInfo;
						prPendingReqHdr->pucChannelInfo = NULL;
						}
					}

				cnmMemFree(prAdapter, prPendingReqHdr);
			}

			return TRUE;
		}
	}

	return FALSE;
}

struct _AIS_FSM_INFO_T *aisGetAisFsmInfo(
	IN struct _ADAPTER_T *prAdapter,
	IN uint8_t ucBssIndex) {

	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		DBGLOG(AIS, WARN,
			"Use default, invalid index = %d\n", ucBssIndex);
		ucBssIndex = 0;
	}

	DBGLOG(AIS, LOUD, "ucBssIndex = %d\n", ucBssIndex);

	return &prAdapter->rWifiVar.rAisFsmInfo;
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
P_AIS_REQ_HDR_T aisFsmGetNextRequest(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_AIS_REQ_HDR_T prPendingReqHdr;

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	LINK_REMOVE_HEAD(&(prAisFsmInfo->rPendingReqList), prPendingReqHdr, P_AIS_REQ_HDR_T);
	/* save partial scan puScanChannel info to prGlueInfo */
	if ((prPendingReqHdr != NULL) && (prAdapter->prGlueInfo != NULL)) {
		if (prAdapter->prGlueInfo->pucScanChannel != NULL)
			DBGLOG(INIT, TRACE, "prGlueInfo error pucScanChannel=%p",
					prAdapter->prGlueInfo->pucScanChannel);

		if (prPendingReqHdr->pucChannelInfo != NULL) {
			prAdapter->prGlueInfo->pucScanChannel = prPendingReqHdr->pucChannelInfo;
			prPendingReqHdr->pucChannelInfo = NULL;
		}
	}

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
BOOLEAN aisFsmInsertRequest(IN P_ADAPTER_T prAdapter, IN ENUM_AIS_REQUEST_TYPE_T eReqType)
{
	P_AIS_REQ_HDR_T prAisReq;
	P_AIS_FSM_INFO_T prAisFsmInfo;

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	prAisReq = (P_AIS_REQ_HDR_T) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(AIS_REQ_HDR_T));

	if (!prAisReq) {
		ASSERT(0);	/* Can't generate new message */
		return FALSE;
	}

	prAisReq->eReqType = eReqType;
	prAisReq->pucChannelInfo = NULL;
	/* save partial scan pucScanChannel info to pending scan */
	if ((prAdapter->prGlueInfo != NULL) &&
		(prAdapter->prGlueInfo->pucScanChannel != NULL)) {
		prAisReq->pucChannelInfo = prAdapter->prGlueInfo->pucScanChannel;
		prAdapter->prGlueInfo->pucScanChannel = NULL;
	}

	/* attach request into pending request list */
	LINK_INSERT_TAIL(&prAisFsmInfo->rPendingReqList, &prAisReq->rLinkEntry);

	DBGLOG(AIS, INFO, "aisFsmInsertRequest: eCurrentState=%d, eReqType=%d, u4NumElem=%d\n",
		prAisFsmInfo->eCurrentState, eReqType, prAisFsmInfo->rPendingReqList.u4NumElem);

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
VOID aisFsmFlushRequest(IN P_ADAPTER_T prAdapter)
{
	P_AIS_REQ_HDR_T prAisReq;

	ASSERT(prAdapter);

	while ((prAisReq = aisFsmGetNextRequest(prAdapter)) != NULL) {
		/* for partional scan, if channel infor exist, free channel info */
		if (prAisReq->pucChannelInfo != NULL)
			kalMemSet(prAisReq->pucChannelInfo, 0, sizeof(PARTIAL_SCAN_INFO));

		cnmMemFree(prAdapter, prAisReq);
	}

}

VOID aisFsmRunEventRemainOnChannel(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_REMAIN_ON_CHANNEL_T prRemainOnChannel;
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;

	DEBUGFUNC("aisFsmRunEventRemainOnChannel()");

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	prRemainOnChannel = (P_MSG_REMAIN_ON_CHANNEL_T) prMsgHdr;

	/* record parameters */
	prAisFsmInfo->rChReqInfo.eBand = prRemainOnChannel->eBand;
	prAisFsmInfo->rChReqInfo.eSco = prRemainOnChannel->eSco;
	prAisFsmInfo->rChReqInfo.ucChannelNum = prRemainOnChannel->ucChannelNum;
	prAisFsmInfo->rChReqInfo.u4DurationMs = prRemainOnChannel->u4DurationMs;
	prAisFsmInfo->rChReqInfo.u8Cookie = prRemainOnChannel->u8Cookie;

	if ((prAisFsmInfo->eCurrentState == AIS_STATE_IDLE) || (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR
		&& prAisFsmInfo->fgIsInfraChannelFinished == TRUE)) {
		/* transit to next state */
		aisFsmSteps(prAdapter, AIS_STATE_REQ_REMAIN_ON_CHANNEL);
	} else {
		aisFsmInsertRequest(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL);
	}

	/* free messages */
	cnmMemFree(prAdapter, prMsgHdr);

}

VOID aisFsmRunEventCancelRemainOnChannel(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_INFO_T prAisBssInfo;
	P_MSG_CANCEL_REMAIN_ON_CHANNEL_T prCancelRemainOnChannel;

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;

	prCancelRemainOnChannel = (P_MSG_CANCEL_REMAIN_ON_CHANNEL_T) prMsgHdr;

	/* 1. Check the cookie first */
	if (prCancelRemainOnChannel->u8Cookie == prAisFsmInfo->rChReqInfo.u8Cookie) {

		/* 2. release channel privilege/request */
		if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_REMAIN_ON_CHANNEL) {
			/* 2.1 elease channel */
			aisFsmReleaseCh(prAdapter);
		} else if (prAisFsmInfo->eCurrentState == AIS_STATE_REMAIN_ON_CHANNEL) {
			/* 2.1 release channel */
			aisFsmReleaseCh(prAdapter);

			/* 2.2 stop channel timeout timer */
			cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer);
		}

		/* 3. clear pending request of remain_on_channel */
		aisFsmIsRequestPending(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL, TRUE);

		/* 4. decide which state to retreat */
		if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_REMAIN_ON_CHANNEL ||
		    prAisFsmInfo->eCurrentState == AIS_STATE_REMAIN_ON_CHANNEL) {
			if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED)
				aisFsmSteps(prAdapter, AIS_STATE_NORMAL_TR);
			else
				aisFsmSteps(prAdapter, AIS_STATE_IDLE);
		}
	}

	/* 5. free message */
	cnmMemFree(prAdapter, prMsgHdr);

}

VOID aisFsmRunEventMgmtFrameTx(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_MSG_MGMT_TX_REQUEST_T prMgmtTxMsg = (P_MSG_MGMT_TX_REQUEST_T) NULL;

	do {
		ASSERT((prAdapter != NULL) && (prMsgHdr != NULL));

		prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

		if (prAisFsmInfo == NULL)
			break;

		prMgmtTxMsg = (P_MSG_MGMT_TX_REQUEST_T) prMsgHdr;

		aisFuncTxMgmtFrame(prAdapter,
				   &prAisFsmInfo->rMgmtTxInfo, prMgmtTxMsg->prMgmtMsduInfo, prMgmtTxMsg->u8Cookie);

	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* aisFsmRunEventMgmtFrameTx */

VOID aisFsmRunEventChannelTimeout(IN P_ADAPTER_T prAdapter, ULONG ulParamPtr)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_BSS_INFO_T prAisBssInfo;

	DEBUGFUNC("aisFsmRunEventRemainOnChannel()");

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;

	if (prAisFsmInfo->eCurrentState == AIS_STATE_REMAIN_ON_CHANNEL) {
		/* 1. release channel */
		aisFsmReleaseCh(prAdapter);

		/* 2. stop channel timeout timer */
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer);

		/* 3. expiration indication to upper layer */
		kalRemainOnChannelExpired(prAdapter->prGlueInfo,
					  prAisFsmInfo->rChReqInfo.u8Cookie,
					  prAisFsmInfo->rChReqInfo.eBand,
					  prAisFsmInfo->rChReqInfo.eSco, prAisFsmInfo->rChReqInfo.ucChannelNum);

		/* 4. decide which state to retreat */
		if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED)
			aisFsmSteps(prAdapter, AIS_STATE_NORMAL_TR);
		else
			aisFsmSteps(prAdapter, AIS_STATE_IDLE);

	} else {
		DBGLOG(AIS, WARN, "Unexpected remain_on_channel timeout event\n");
#if DBG
		DBGLOG(AIS, STATE, "CURRENT State: [%s]\n", apucDebugAisState[prAisFsmInfo->eCurrentState]);
#else
		DBGLOG(AIS, STATE, "[%d] CURRENT State: [%d]\n", DBG_AIS_IDX, prAisFsmInfo->eCurrentState);
#endif
	}

}

WLAN_STATUS
aisFsmRunEventMgmtFrameTxDone(IN P_ADAPTER_T prAdapter,
			      IN P_MSDU_INFO_T prMsduInfo, IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;
	P_AIS_MGMT_TX_REQ_INFO_T prMgmtTxReqInfo = (P_AIS_MGMT_TX_REQ_INFO_T) NULL;
	BOOLEAN fgIsSuccess = FALSE;

	do {
		ASSERT((prAdapter != NULL) && (prMsduInfo != NULL));

		prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
		prMgmtTxReqInfo = &(prAisFsmInfo->rMgmtTxInfo);

		if (rTxDoneStatus == TX_RESULT_SUCCESS)
			fgIsSuccess = TRUE;

		if (prMgmtTxReqInfo->prMgmtTxMsdu == prMsduInfo) {
			kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
						prMgmtTxReqInfo->u8Cookie,
						fgIsSuccess, prMsduInfo->prPacket, (UINT_32) prMsduInfo->u2FrameLength);

			prMgmtTxReqInfo->prMgmtTxMsdu = NULL;
		}

	} while (FALSE);

	return WLAN_STATUS_SUCCESS;

}				/* aisFsmRunEventMgmtFrameTxDone */

WLAN_STATUS
aisFuncTxMgmtFrame(IN P_ADAPTER_T prAdapter,
		   IN P_AIS_MGMT_TX_REQ_INFO_T prMgmtTxReqInfo, IN P_MSDU_INFO_T prMgmtTxMsdu, IN UINT_64 u8Cookie)
{
	WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;
	P_MSDU_INFO_T prTxMsduInfo = (P_MSDU_INFO_T) NULL;
	P_WLAN_MAC_HEADER_T prWlanHdr = (P_WLAN_MAC_HEADER_T) NULL;
	P_STA_RECORD_T prStaRec = (P_STA_RECORD_T) NULL;

	do {
		ASSERT((prAdapter != NULL) && (prMgmtTxReqInfo != NULL));

		if (prMgmtTxReqInfo->fgIsMgmtTxRequested) {

			/* 1. prMgmtTxReqInfo->prMgmtTxMsdu != NULL */
			/* Packet on driver, not done yet, drop it. */
			prTxMsduInfo = prMgmtTxReqInfo->prMgmtTxMsdu;
			if (prTxMsduInfo != NULL) {

				kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
							prMgmtTxReqInfo->u8Cookie,
							FALSE,
							prTxMsduInfo->prPacket, (UINT_32) prTxMsduInfo->u2FrameLength);

				/* Leave it to TX Done handler. */
				/* cnmMgtPktFree(prAdapter, prTxMsduInfo); */
				prMgmtTxReqInfo->prMgmtTxMsdu = NULL;
			}
			/* 2. prMgmtTxReqInfo->prMgmtTxMsdu == NULL */
			/* Packet transmitted, wait tx done. (cookie issue) */
		}

		ASSERT(prMgmtTxReqInfo->prMgmtTxMsdu == NULL);

		prWlanHdr = (P_WLAN_MAC_HEADER_T) ((ULONG) prMgmtTxMsdu->prPacket + MAC_TX_RESERVED_FIELD);
		prStaRec = cnmGetStaRecByAddress(prAdapter, prAdapter->prAisBssInfo->ucBssIndex, prWlanHdr->aucAddr1);

		TX_SET_MMPDU(prAdapter,
			     prMgmtTxMsdu,
			     (prStaRec !=
			      NULL) ? (prStaRec->ucBssIndex) : (prAdapter->prAisBssInfo->ucBssIndex),
			     (prStaRec != NULL) ? (prStaRec->ucIndex) : (STA_REC_INDEX_NOT_FOUND),
			     WLAN_MAC_MGMT_HEADER_LEN, prMgmtTxMsdu->u2FrameLength,
			     aisFsmRunEventMgmtFrameTxDone, MSDU_RATE_MODE_AUTO);
		prMgmtTxReqInfo->u8Cookie = u8Cookie;
		prMgmtTxReqInfo->prMgmtTxMsdu = prMgmtTxMsdu;
		prMgmtTxReqInfo->fgIsMgmtTxRequested = TRUE;

		nicTxConfigPktControlFlag(prMgmtTxMsdu, MSDU_CONTROL_FLAG_FORCE_TX, TRUE);

		/* send to TX queue */
		nicTxEnqueueMsdu(prAdapter, prMgmtTxMsdu);

	} while (FALSE);

	return rWlanStatus;
}				/* aisFuncTxMgmtFrame */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will validate the Rx Action Frame and indicate to uppoer layer
*           if the specified conditions were matched.
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] prSwRfb            Pointer to SW RFB data structure.
* @param[out] pu4ControlFlags   Control flags for replying the Probe Response
*
* @retval none
*/
/*----------------------------------------------------------------------------*/
VOID aisFuncValidateRxActionFrame(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
	P_AIS_FSM_INFO_T prAisFsmInfo = (P_AIS_FSM_INFO_T) NULL;

	DEBUGFUNC("aisFuncValidateRxActionFrame");

	do {

		ASSERT((prAdapter != NULL) && (prSwRfb != NULL));

		prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

		if (1 /* prAisFsmInfo->u4AisPacketFilter & PARAM_PACKET_FILTER_ACTION_FRAME */) {
			/* Leave the action frame to wpa_supplicant. */
			kalIndicateRxMgmtFrame(prAdapter->prGlueInfo, prSwRfb);
		}

	} while (FALSE);

	return;

}				/* aisFuncValidateRxActionFrame */

VOID aisRemoveBlacklistBySource(P_ADAPTER_T prAdapter, enum _BLACK_LIST_SOURCE source)
{
	P_CONNECTION_SETTINGS_T prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	struct AIS_BLACKLIST_ITEM *prNextEntry = NULL;
	P_LINK_T prFreeList = &prConnSettings->rBlackList.rFreeLink;
	P_LINK_T prBlackList = &prConnSettings->rBlackList.rUsingLink;

	LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prBlackList, rLinkEntry, struct AIS_BLACKLIST_ITEM) {
		prEntry->blackListSource &= ~source;
		if (!prEntry->blackListSource) {
			LINK_REMOVE_KNOWN_ENTRY(prBlackList, &prEntry->rLinkEntry);
			LINK_INSERT_HEAD(prFreeList, &prEntry->rLinkEntry);
		}
	}
}

struct AIS_BLACKLIST_ITEM *
aisAddBlacklist(P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc, enum _BLACK_LIST_SOURCE source)
{
	P_CONNECTION_SETTINGS_T prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	struct LINK_MGMT *prBlackList = &prConnSettings->rBlackList;

	if (!prBssDesc) {
		DBGLOG(AIS, ERROR, "bss descriptor is NULL\n");
		return NULL;
	}
	if (prBssDesc->prBlack) {
		GET_CURRENT_SYSTIME(&prBssDesc->prBlack->rAddTime);
		prBssDesc->prBlack->ucCount++;
		prBssDesc->prBlack->blackListSource |= source;
		DBGLOG(AIS, INFO, "update blacklist for %pM, count %d\n",
			prBssDesc->aucBSSID, prBssDesc->prBlack->ucCount);
		return prBssDesc->prBlack;
	}

	prEntry = aisQueryBlackList(prAdapter, prBssDesc);

	if (prEntry) {
		GET_CURRENT_SYSTIME(&prEntry->rAddTime);
		prBssDesc->prBlack = prEntry;
		prEntry->ucCount++;
		prEntry->blackListSource |= source;
		DBGLOG(AIS, INFO, "update blacklist for %pM, count %d\n",
			prBssDesc->aucBSSID, prEntry->ucCount);
		return prEntry;
	}
	LINK_MGMT_GET_ENTRY(prBlackList, prEntry, struct AIS_BLACKLIST_ITEM, VIR_MEM_TYPE);
	if (!prEntry) {
		DBGLOG(AIS, WARN, "No memory to allocate\n");
		return NULL;
	}
	prEntry->ucCount = 1;
	prEntry->blackListSource |= source;
	COPY_MAC_ADDR(prEntry->aucBSSID, prBssDesc->aucBSSID);
	COPY_SSID(prEntry->aucSSID, prEntry->ucSSIDLen, prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
	GET_CURRENT_SYSTIME(&prEntry->rAddTime);
	prBssDesc->prBlack = prEntry;

	DBGLOG(AIS, INFO, "Add %pM to black List\n", prBssDesc->aucBSSID);
	return prEntry;
}

struct AIS_BLACKLIST_ITEM *
aisAddBlacklistByBssid(P_ADAPTER_T prAdapter, UINT_8 aucBSSID[], enum _BLACK_LIST_SOURCE source)
{
	P_CONNECTION_SETTINGS_T prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	P_LINK_T prFreeList = &prConnSettings->rBlackList.rFreeLink;
	P_LINK_T prBlackList = &prConnSettings->rBlackList.rUsingLink;

	prEntry = aisQueryBlackListByBssid(prAdapter, aucBSSID);

	if (prEntry) {
		GET_CURRENT_SYSTIME(&prEntry->rAddTime);
		prEntry->ucCount++;
		prEntry->blackListSource |= source;
		DBGLOG(AIS, INFO, "update blacklist for %pM, count %d\n",
			aucBSSID, prEntry->ucCount);
		return prEntry;
	}

	LINK_REMOVE_HEAD(prFreeList, prEntry, struct AIS_BLACKLIST_ITEM *);
	if (!prEntry)
		prEntry = kalMemAlloc(sizeof(struct AIS_BLACKLIST_ITEM), VIR_MEM_TYPE);
	if (!prEntry) {
		DBGLOG(AIS, WARN, "No memory to allocate\n");
		return NULL;
	}
	kalMemZero(prEntry, sizeof(*prEntry));
	prEntry->ucCount = 1;
	prEntry->blackListSource |= source;
	COPY_MAC_ADDR(prEntry->aucBSSID, aucBSSID);
	COPY_SSID(prEntry->aucSSID, prEntry->ucSSIDLen, prConnSettings->aucSSID, prConnSettings->ucSSIDLen);
	GET_CURRENT_SYSTIME(&prEntry->rAddTime);
	LINK_INSERT_HEAD(prBlackList, &prEntry->rLinkEntry);

	DBGLOG(AIS, INFO, "Add %pM to black List\n", aucBSSID);
	return prEntry;
}
VOID aisRemoveBlackList(P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc, enum _BLACK_LIST_SOURCE source)
{
	P_CONNECTION_SETTINGS_T prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;

	prEntry = aisQueryBlackList(prAdapter, prBssDesc);
	if (!prEntry)
		return;
	prEntry->blackListSource &= ~source;

	if (!prEntry->blackListSource) {
		LINK_MGMT_RETURN_ENTRY(&prConnSettings->rBlackList, prEntry);
		prBssDesc->prBlack = NULL;
		DBGLOG(AIS, INFO, "Remove %pM from blacklist\n", prBssDesc->aucBSSID);
	}
}

struct AIS_BLACKLIST_ITEM *
aisQueryBlackList(P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
	P_CONNECTION_SETTINGS_T prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	P_LINK_T prBlackList = &prConnSettings->rBlackList.rUsingLink;

	if (!prBssDesc)
		return NULL;
	else if (prBssDesc->prBlack)
		return prBssDesc->prBlack;

	LINK_FOR_EACH_ENTRY(prEntry, prBlackList, rLinkEntry, struct AIS_BLACKLIST_ITEM) {
		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prEntry->aucBSSID) &&
			EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
			prEntry->aucSSID, prEntry->ucSSIDLen)) {
			prBssDesc->prBlack = prEntry;
			return prEntry;
		}
	}
	DBGLOG(AIS, TRACE, "%pM is not in blacklist\n", prBssDesc->aucBSSID);
	return NULL;
}

struct AIS_BLACKLIST_ITEM *
aisQueryBlackListByBssid(P_ADAPTER_T prAdapter, UINT_8 aucBSSID[])
{
	P_CONNECTION_SETTINGS_T prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	P_LINK_T prBlackList = &prConnSettings->rBlackList.rUsingLink;

	LINK_FOR_EACH_ENTRY(prEntry, prBlackList, rLinkEntry, struct AIS_BLACKLIST_ITEM) {
		if (EQUAL_MAC_ADDR(aucBSSID, prEntry->aucBSSID) &&
			EQUAL_SSID(prConnSettings->aucSSID, prConnSettings->ucSSIDLen,
			prEntry->aucSSID, prEntry->ucSSIDLen))
			return prEntry;
	}
	DBGLOG(AIS, TRACE, "%pM is not in blacklist\n", aucBSSID);
	return NULL;
}

VOID aisRemoveTimeoutBlacklist(P_ADAPTER_T prAdapter)
{
	P_CONNECTION_SETTINGS_T prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	struct AIS_BLACKLIST_ITEM *prNextEntry = NULL;
	P_LINK_T prBlackList = &prConnSettings->rBlackList.rUsingLink;
	OS_SYSTIME rCurrent;

	GET_CURRENT_SYSTIME(&rCurrent);

	LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prBlackList, rLinkEntry, struct AIS_BLACKLIST_ITEM) {
		if (prEntry->blackListSource & AIS_BLACK_LIST_FROM_FWK)
			continue;
		if (!CHECK_FOR_TIMEOUT(rCurrent, prEntry->rAddTime, SEC_TO_MSEC(AIS_BLACKLIST_TIMEOUT)))
			continue;
		LINK_MGMT_RETURN_ENTRY(&prConnSettings->rBlackList, prEntry);
	}
}

static VOID aisRemoveDisappearedBlacklist(P_ADAPTER_T prAdapter)
{
	P_CONNECTION_SETTINGS_T prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	struct AIS_BLACKLIST_ITEM *prNextEntry = NULL;
	P_LINK_T prBlackList = &prConnSettings->rBlackList.rUsingLink;
	P_BSS_DESC_T prBssDesc = NULL;
	P_LINK_T prBSSDescList = &prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	UINT_64 u8Current = kalGetBootTime();
	BOOLEAN fgDisappeared = TRUE;

	LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prBlackList, rLinkEntry, struct AIS_BLACKLIST_ITEM) {
		fgDisappeared = TRUE;
		LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {
			if (prBssDesc->prBlack == prEntry || (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prEntry->aucBSSID) &&
				EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
				prEntry->aucSSID, prEntry->ucSSIDLen))) {
				fgDisappeared = FALSE;
				break;
			}
		}
		if (!fgDisappeared || (u8Current - prEntry->u8DisapperTime) < 600 * USEC_PER_SEC)
			continue;
		DBGLOG(AIS, INFO, "Remove disappeared blacklist %s %pM\n",
			prEntry->aucSSID, prEntry->aucBSSID);
		LINK_MGMT_RETURN_ENTRY(&prConnSettings->rBlackList, prEntry);
	}
}

BOOLEAN aisApOverload(struct AIS_BLACKLIST_ITEM *prBlack)
{
	switch (prBlack->u2AuthStatus) {
	case STATUS_CODE_ASSOC_DENIED_AP_OVERLOAD:
	case STATUS_CODE_ASSOC_DENIED_BANDWIDTH:
		return TRUE;
	}
	switch (prBlack->u2DeauthReason) {
	case REASON_CODE_DISASSOC_LACK_OF_BANDWIDTH:
	case REASON_CODE_DISASSOC_AP_OVERLOAD:
		return TRUE;
	}
	return FALSE;
}

UINT_16 aisCalculateBlackListScore(P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
	if (!prBssDesc->prBlack)
		prBssDesc->prBlack = aisQueryBlackList(prAdapter, prBssDesc);

	if (!prBssDesc->prBlack)
		return 100;
	else if (aisApOverload(prBssDesc->prBlack) || prBssDesc->prBlack->ucCount >= 10)
		return 0;
	return 100 - prBssDesc->prBlack->ucCount * 10;
}

VOID aisRecordBeaconTimeout(P_ADAPTER_T prAdapter, P_BSS_INFO_T prAisBssInfo)
{
	P_AIS_FSM_INFO_T prAisFsmInfo = &prAdapter->rWifiVar.rAisFsmInfo;
	struct LINK_MGMT *prBcnTimeout = &prAisFsmInfo->rBcnTimeout;
	struct AIS_BEACON_TIMEOUT_BSS *prEntry = NULL;

	LINK_MGMT_GET_ENTRY(prBcnTimeout, prEntry, struct AIS_BEACON_TIMEOUT_BSS, VIR_MEM_TYPE);
	if (!prEntry) {
		DBGLOG(CNM, WARN, "No memory to allocate\n");
		return;
	}
	COPY_MAC_ADDR(prEntry->aucBSSID, prAisBssInfo->aucBSSID);
	COPY_SSID(prEntry->aucSSID, prEntry->ucSSIDLen,
			prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen);

	if (prAisFsmInfo->prTargetBssDesc)
		prEntry->u8Tsf = prAisFsmInfo->prTargetBssDesc->u8TimeStamp.QuadPart;
	else
		prEntry->u8Tsf = 0x0ULL;
	prEntry->u8AddTime = kalGetBootTime();
}

VOID aisRemoveBeaconTimeoutEntry(P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
#if 0 /* wave2-feature*/
	UINT_64 u8Tsf = 0;
	P_AIS_FSM_INFO_T prAisFsmInfo = &prAdapter->rWifiVar.rAisFsmInfo;
	struct LINK_MGMT *prBcnTimeout = &prAisFsmInfo->rBcnTimeout;
	struct AIS_BEACON_TIMEOUT_BSS *prEntry = NULL;

	LINK_FOR_EACH_ENTRY(prEntry, &prBcnTimeout->rUsingLink,
		rLinkEntry, struct AIS_BEACON_TIMEOUT_BSS) {
		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prEntry->aucBSSID) &&
			EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
			prEntry->aucSSID, prEntry->ucSSIDLen)) {
			u8Tsf = prBssDesc->u8TimeStamp.QuadPart;
			if (u8Tsf < prEntry->u8Tsf)
				DBGLOG(AIS, INFO, "%pM %s may reboot %llu seconds ago\n",
					prBssDesc->aucBSSID, prBssDesc->aucSSID, u8Tsf/USEC_PER_SEC);
			else
				DBGLOG(AIS, INFO, "%pM %s\n", prBssDesc->aucBSSID, prBssDesc->aucSSID);
			LINK_REMOVE_KNOWN_ENTRY(&prBcnTimeout->rUsingLink, prEntry);
			LINK_INSERT_HEAD(&prBcnTimeout->rFreeLink, &prEntry->rLinkEntry);
			return;
		}
	}
#endif
}

static VOID aisRemoveOldestBcnTimeout(P_AIS_FSM_INFO_T prAisFsmInfo)
{
#if 0 /* wave2-feature*/
	struct AIS_BEACON_TIMEOUT_BSS *prEntry = NULL;
	P_LINK_T prLink = &prAisFsmInfo->rBcnTimeout.rUsingLink;
	UINT_64 u8Current = kalGetBootTime();

	while (TRUE) {
		prEntry = LINK_PEEK_HEAD(prLink, struct AIS_BEACON_TIMEOUT_BSS, rLinkEntry);
		if (!prEntry || (u8Current - prEntry->u8AddTime < CFG_BSS_DISAPPEAR_THRESOLD * USEC_PER_SEC))
			break;
		DBGLOG(AIS, INFO, "%pM %s has disappeard about %llu seconds\n",
			prEntry->aucBSSID, prEntry->aucSSID, (u8Current - prEntry->u8AddTime)/USEC_PER_SEC);
		LINK_REMOVE_HEAD(prLink, prEntry, struct AIS_BEACON_TIMEOUT_BSS *);
	}
#endif
}

VOID aisFsmRunEventBssTransition(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	struct MSG_AIS_BSS_TRANSITION_T *prMsg = (struct MSG_AIS_BSS_TRANSITION_T *)prMsgHdr;
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;
	struct BSS_TRANSITION_MGT_PARAM_T *prBtmParam = &prAisSpecificBssInfo->rBTMParam;
	enum WNM_AIS_BSS_TRANSITION eTransType = BSS_TRANSITION_MAX_NUM;
	P_BSS_DESC_T prBssDesc = prAdapter->rWifiVar.rAisFsmInfo.prTargetBssDesc;
	BOOLEAN fgNeedBtmResponse = FALSE;
	UINT_8 ucStatus = BSS_TRANSITION_MGT_STATUS_UNSPECIFIED;
	UINT_8 ucRcvToken = 0;
	static UINT_8 aucChnlList[MAXIMUM_OPERATION_CHANNEL_LIST];

	if (!prMsg) {
		DBGLOG(AIS, WARN, "Msg Header is NULL\n");
		return;
	}
	eTransType = prMsg->eTransitionType;
	fgNeedBtmResponse = prMsg->fgNeedResponse;
	ucRcvToken = prMsg->ucToken;

	DBGLOG(AIS, INFO, "Transition Type: %d\n", eTransType);
	aisCollectNeighborAP(prAdapter, prMsg->pucCandList, prMsg->u2CandListLen, prMsg->ucValidityInterval);
	cnmMemFree(prAdapter, prMsgHdr);
	/* Solicited BTM request: the case we're waiting btm request after send btm query before roaming scan */
	if (prBtmParam->ucDialogToken == ucRcvToken) {
		prBtmParam->fgPendingResponse = fgNeedBtmResponse;
		prBtmParam->fgUnsolicitedReq = FALSE;

		switch (prAdapter->rWifiVar.rRoamingInfo.eCurrentState) {
		case ROAMING_STATE_REQ_CAND_LIST:
			glNotifyDrvStatus(SOL_BTM_REQ, (PVOID)prAisSpecificBssInfo);
			roamingFsmSteps(prAdapter, ROAMING_STATE_DISCOVERY);
			return;
		case ROAMING_STATE_DISCOVERY:
			/* this case need to fall through */
		case ROAMING_STATE_ROAM:
			ucStatus = BSS_TRANSITION_MGT_STATUS_UNSPECIFIED;
			goto send_response;
		default:
			/* not solicited btm request, but dialog token matches occasionally. */
			break;
		}
	}
	glNotifyDrvStatus(UNSOL_BTM_REQ, (PVOID)prAisSpecificBssInfo);
	prBtmParam->fgUnsolicitedReq = TRUE;
	/* Unsolicited BTM request */
	switch (eTransType) {
	case BSS_TRANSITION_DISASSOC:
		ucStatus = BSS_TRANSITION_MGT_STATUS_ACCEPT;
		break;
	case BSS_TRANSITION_REQ_ROAMING:
	{
		struct NEIGHBOR_AP_T *prNeiAP = NULL;
		P_LINK_T prUsingLink = &prAisSpecificBssInfo->rNeighborApList.rUsingLink;
		UINT_8 i = 0;
		UINT_8 ucChannel = 0;
		UINT_8 ucChnlCnt = 0;
		UINT_16 u2LeftTime = 0;

		if (!prBssDesc) {
			DBGLOG(AIS, ERROR, "Target Bss Desc is NULL\n");
			break;
		}
		prBtmParam->fgPendingResponse = fgNeedBtmResponse;
		kalMemZero(aucChnlList, sizeof(aucChnlList));
		LINK_FOR_EACH_ENTRY(prNeiAP, prUsingLink, rLinkEntry, struct NEIGHBOR_AP_T) {
			ucChannel = prNeiAP->ucChannel;
			for (i = 0; i < ucChnlCnt && ucChannel != aucChnlList[i]; i++)
				;
			if (i == ucChnlCnt)
				ucChnlCnt++;
		}
		/* reserve 1 second for association */
		u2LeftTime = prBtmParam->u2DisassocTimer * prBssDesc->u2BeaconInterval - 1000;
		/* check if left time is enough to do partial scan, if not enought, reject directly */
		if (u2LeftTime < ucChnlCnt * prBssDesc->u2BeaconInterval) {
			ucStatus = BSS_TRANSITION_MGT_STATUS_UNSPECIFIED;
			goto send_response;
		}
		roamingFsmSteps(prAdapter, ROAMING_STATE_DISCOVERY);
		return;
	}
	default:
		ucStatus = BSS_TRANSITION_MGT_STATUS_ACCEPT;
		break;
	}
send_response:
	if (fgNeedBtmResponse && prAdapter->prAisBssInfo && prAdapter->prAisBssInfo->prStaRecOfAP) {
		prBtmParam->ucStatusCode = ucStatus;
		prBtmParam->ucTermDelay = 0;
		kalMemZero(prBtmParam->aucTargetBssid, MAC_ADDR_LEN);
		prBtmParam->u2OurNeighborBssLen = 0;
		prBtmParam->fgPendingResponse = FALSE;
		wnmSendBTMResponseFrame(prAdapter, prAdapter->prAisBssInfo->prStaRecOfAP);
	}
}

#if CFG_SUPPORT_802_11K
VOID aisSendNeighborRequest(P_ADAPTER_T prAdapter)
{
	struct SUB_ELEMENT_LIST *prSSIDIE;
	UINT_8 aucBuffer[sizeof(*prSSIDIE) + 31];
	P_BSS_INFO_T prBssInfo = prAdapter->prAisBssInfo;

	kalMemZero(aucBuffer, sizeof(aucBuffer));
	prSSIDIE = (struct SUB_ELEMENT_LIST *)&aucBuffer[0];
	prSSIDIE->rSubIE.ucSubID = ELEM_ID_SSID;
	COPY_SSID(&prSSIDIE->rSubIE.aucOptInfo[0], prSSIDIE->rSubIE.ucLength,
		prBssInfo->aucSSID, prBssInfo->ucSSIDLen);
	rlmTxNeighborReportRequest(prAdapter, prBssInfo->prStaRecOfAP, prSSIDIE);
}
#endif

VOID aisRunEventChnlUtilRsp(P_ADAPTER_T prAdapter, P_MSG_HDR_T prMsgHdr)
{
	struct MSG_CH_UTIL_RSP *prChUtilRsp = (struct MSG_CH_UTIL_RSP *)prMsgHdr;
	struct ESS_CHNL_INFO *prEssChnlInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo.arCurEssChnlInfo[0];
	PUINT_8 pucChnlList = NULL;
	PUINT_8 pucUtilization = NULL;
	UINT_8 i = 0;
	UINT_8 j = 0;

	if (!prChUtilRsp)
		return;
	if (prAdapter->rWifiVar.rAisFsmInfo.eCurrentState != AIS_STATE_COLLECT_ESS_INFO) {
		cnmMemFree(prAdapter, prChUtilRsp);
		return;
	}
	pucChnlList = prChUtilRsp->aucChnlList;
	pucUtilization = prChUtilRsp->aucChUtil;
	for (i = 0; i < prChUtilRsp->ucChnlNum; i++) {
		DBGLOG(AIS, INFO, "channel %d, utilization %d\n", pucChnlList[i], pucUtilization[i]);
		for (j = 0; j < prAdapter->rWifiVar.rAisSpecificBssInfo.ucCurEssChnlInfoNum; j++) {
			if (prEssChnlInfo[j].ucChannel != pucChnlList[i])
				continue;
			if (prEssChnlInfo[j].ucUtilization >= pucUtilization[i])
				continue;
			prEssChnlInfo[j].ucUtilization = pucUtilization[i];
			break;
		}
	}
	cnmMemFree(prAdapter, prChUtilRsp);
	aisFsmSteps(prAdapter, AIS_STATE_SEARCH);
}

static OS_SYSTIME rScanStartTime;
/* Update scan update index for each BSS descriptor if it was seen.
** And call aisRemoveBeaconTimeoutEntry
*/
static VOID aisPostScan(P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo = &prAdapter->rWifiVar.rAisFsmInfo;
	P_LINK_T prLink = &prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	P_BSS_DESC_T prBssDesc = NULL;
	BOOLEAN fgNeedYellow = FALSE;

	prAisFsmInfo->u4ScanUpdateIdx++;
	DBGLOG(AIS, TEMP, "scan update index %u\n", prAisFsmInfo->u4ScanUpdateIdx);
	LINK_FOR_EACH_ENTRY(prBssDesc, prLink, rLinkEntry, BSS_DESC_T) {
		if (prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE)
			continue;
		if (prBssDesc->fgRptCfg80211 && prBssDesc->rUpdateTime + 10000 < rScanStartTime &&
			prBssDesc->ucSSIDLen > 0 && prBssDesc->aucSSID[0] > 0) {
			fgNeedYellow = TRUE;
			DBGLOG(AIS, INFO, "%pM %s scan update mismatch, return line %d, Up@ %u, Add@ %u\n",
				prBssDesc->aucBSSID, prBssDesc->aucSSID, prBssDesc->u2ReturnLine,
				prBssDesc->rUpdateTime, prBssDesc->rAddBssDescTime);
		}
		if (!prBssDesc->u2RawLength) {
			DBGLOG(AIS, TEMP, "Bss %pM, chnl %d not seen, updateIdx %u\n",
			prBssDesc->aucBSSID, prBssDesc->ucChannelNum, prBssDesc->u4UpdateIdx);
			continue;
		}
		prBssDesc->u4UpdateIdx = prAisFsmInfo->u4ScanUpdateIdx;
		aisRemoveBeaconTimeoutEntry(prAdapter, prBssDesc);
	}
	if (fgNeedYellow)
		DBGLOG(SCN, ERROR, "Scan debug, scan mismatch\n");
}

/* Call this function before AIS scan
** It will undate scan update index on Bss whose channel is not included in the scan request
** Additionally, will reset fgSeeProbeResp to false if Bss whose channel is included in the scan request
*/
#if defined(MT6631)
static VOID aisPreScan(P_ADAPTER_T prAdapter, struct _MSG_SCN_SCAN_REQ_V3_T *prMsg)
#else
static VOID aisPreScan(P_ADAPTER_T prAdapter, P_MSG_SCN_SCAN_REQ_V2 prMsg)
#endif
{
	P_LINK_T prLink = &prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	P_BSS_DESC_T prBssDesc = NULL;
	BOOLEAN fgResetRawBuff = (prAdapter->rWifiVar.rAisFsmInfo.eCurrentState == AIS_STATE_SCAN ||
		prAdapter->rWifiVar.rAisFsmInfo.eCurrentState == AIS_STATE_ONLINE_SCAN);

	DBGLOG(AIS, TEMP, "scan channel %d\n", prMsg->eScanChannel);
	if (prMsg->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		UINT_8 i = prMsg->ucChannelListNum;
		P_RF_CHANNEL_INFO_T prch = &prMsg->arChnlInfoList[0];

		while (i >= 16) {
			DBGLOG(AIS, TEMP, "chnl: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			prch[0].ucChannelNum, prch[1].ucChannelNum, prch[2].ucChannelNum, prch[3].ucChannelNum,
			prch[4].ucChannelNum, prch[5].ucChannelNum, prch[6].ucChannelNum, prch[7].ucChannelNum,
			prch[8].ucChannelNum, prch[9].ucChannelNum, prch[10].ucChannelNum, prch[11].ucChannelNum,
			prch[12].ucChannelNum, prch[13].ucChannelNum, prch[14].ucChannelNum, prch[15].ucChannelNum);
			i -= 16;
			prch = &prch[16];
		}
		while (i >= 8) {
			DBGLOG(AIS, TEMP, "chnl: %d,%d,%d,%d,%d,%d,%d,%d\n", prch[0].ucChannelNum,
			prch[1].ucChannelNum, prch[2].ucChannelNum, prch[3].ucChannelNum, prch[4].ucChannelNum,
			prch[5].ucChannelNum, prch[6].ucChannelNum, prch[7].ucChannelNum);
			i -= 8;
			prch = &prch[8];
		}
		while (i >= 4) {
			DBGLOG(AIS, TEMP, "chnl: %d,%d,%d,%d\n", prch[0].ucChannelNum,
			prch[1].ucChannelNum, prch[2].ucChannelNum, prch[3].ucChannelNum);
			i -= 4;
			prch = &prch[4];
		}
		while (i > 0) {
			DBGLOG(AIS, TEMP, "chnl: %d\n", prch[0].ucChannelNum);
			i--;
			prch++;
		}
	}
	LINK_FOR_EACH_ENTRY(prBssDesc, prLink, rLinkEntry, BSS_DESC_T) {
		if (prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE)
			continue;
		prBssDesc->fgRptCfg80211 = FALSE;
		prBssDesc->u2ReturnLine = 0;
		if (fgResetRawBuff && prBssDesc->u2RawLength) {
			kalMemZero(prBssDesc->aucRawBuf, CFG_RAW_BUFFER_SIZE);
			prBssDesc->u2RawLength = 0;
		}
		if (prMsg->eScanChannel == SCAN_CHANNEL_FULL)
			prBssDesc->fgSeenProbeResp = FALSE;
		else if (prMsg->eScanChannel == SCAN_CHANNEL_2G4) {
			if (prBssDesc->eBand == BAND_5G)
				prBssDesc->u4UpdateIdx++;
			else
				prBssDesc->fgSeenProbeResp = FALSE;
		} else if (prMsg->eScanChannel == SCAN_CHANNEL_5G) {
			if (prBssDesc->eBand == BAND_2G4)
				prBssDesc->u4UpdateIdx++;
			else
				prBssDesc->fgSeenProbeResp = FALSE;
		} else if (prMsg->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
			UINT_8 i = 0;

			for (; i < prMsg->ucChannelListNum; i++) {
				if (prBssDesc->ucChannelNum == prMsg->arChnlInfoList[i].ucChannelNum)
					break;
			}
			if (i == prMsg->ucChannelListNum)
				prBssDesc->u4UpdateIdx++;
			else
				prBssDesc->fgSeenProbeResp = FALSE;
		}
		DBGLOG(AIS, TEMP, "Bss %pM, chnl %d, updateIdx %u\n",
			prBssDesc->aucBSSID, prBssDesc->ucChannelNum, prBssDesc->u4UpdateIdx);
	}
	GET_CURRENT_SYSTIME(&rScanStartTime);
	DBGLOG(AIS, INFO, "scan start at %u\n", rScanStartTime);
}

VOID aisFsmRunEventSetOkcPmk(IN P_ADAPTER_T prAdapter)
{
	P_AIS_FSM_INFO_T prAisFsmInfo = &prAdapter->rWifiVar.rAisFsmInfo;

	prAdapter->rWifiVar.rConnSettings.fgOkcPmksaReady = TRUE;
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rWaitOkcPMKTimer);
	if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN &&
		prAisFsmInfo->fgIsChannelGranted)
		aisFsmSteps(prAdapter, AIS_STATE_JOIN);
}

static VOID aisFsmSetOkcTimeout(IN P_ADAPTER_T prAdapter, ULONG ulParam)
{
	P_AIS_FSM_INFO_T prAisFsmInfo = &prAdapter->rWifiVar.rAisFsmInfo;

	DBGLOG(AIS, WARN, "Wait OKC PMKID timeout\n");
	if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN &&
		prAisFsmInfo->fgIsChannelGranted)
		aisFsmSteps(prAdapter, AIS_STATE_JOIN);
}

static VOID aisFsmMayRequstPmkId(P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
	/* if okc is enabled and no pmkid entry was found, should ask supplicant to generate one */
	UINT_8 aucBuf[sizeof(PARAM_PMKID_CANDIDATE_LIST_T) + sizeof(PARAM_STATUS_INDICATION_T)];
	P_PARAM_STATUS_INDICATION_T prStatusEvent = (P_PARAM_STATUS_INDICATION_T)aucBuf;
	P_PARAM_PMKID_CANDIDATE_LIST_T prPmkidCandicate =
		(P_PARAM_PMKID_CANDIDATE_LIST_T)(prStatusEvent+1);
	UINT_32 u4Entry = 0;
	P_AIS_FSM_INFO_T prAisFsmInfo = &prAdapter->rWifiVar.rAisFsmInfo;

	if (rsnSearchPmkidEntry(prAdapter, prBssDesc->aucBSSID, &u4Entry) &&
		prAdapter->rWifiVar.rAisSpecificBssInfo.arPmkidCache[u4Entry].fgPmkidExist)
		return;

	DBGLOG(AIS, INFO, "No PMK for %pM, try to generate a OKC PMK\n", prBssDesc->aucBSSID);
	prStatusEvent->eStatusType = ENUM_STATUS_TYPE_CANDIDATE_LIST;
	prPmkidCandicate->u4Version = 1;
	prPmkidCandicate->u4NumCandidates = 1;
	prPmkidCandicate->arCandidateList[0].u4Flags = 0; /* don't request preauth */
	COPY_MAC_ADDR(prPmkidCandicate->arCandidateList[0].arBSSID, prBssDesc->aucBSSID);
	kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
		 WLAN_STATUS_MEDIA_SPECIFIC_INDICATION, (PVOID)aucBuf, sizeof(aucBuf));
	cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rWaitOkcPMKTimer, AIS_WAIT_OKC_PMKID_SEC);
}

static UINT_8 aisGetNeighborApPreference(PUINT_8 pucSubIe, UINT_8 ucLength)
{
	UINT_16 u2Offset = 0;

	IE_FOR_EACH(pucSubIe, ucLength, u2Offset) {
		if (IE_ID(pucSubIe) == 3)
			return pucSubIe[2];
	}
	/* If no preference element is presence, give default value(lowest) 0 */
	return 0;
}

static UINT_64 aisGetBssTermTsf(PUINT_8 pucSubIe, UINT_8 ucLength)
{
	UINT_16 u2Offset = 0;

	IE_FOR_EACH(pucSubIe, ucLength, u2Offset) {
		if (IE_ID(pucSubIe) == 4)
			return *(UINT_64 *)&pucSubIe[2];
	}
	/* If no preference element is presence, give default value(lowest) 0 */
	return 0;
}


VOID aisCollectNeighborAP(P_ADAPTER_T prAdapter, PUINT_8 pucApBuf,
			  UINT_16 u2ApBufLen, UINT_8 ucValidInterval) {
	struct NEIGHBOR_AP_T *prNeighborAP = NULL;
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;
	struct LINK_MGMT *prAPlist = &prAisSpecBssInfo->rNeighborApList;
	struct IE_NEIGHBOR_REPORT_T *prIe = (struct IE_NEIGHBOR_REPORT_T *)pucApBuf;
	UINT_16 u2BufLen;

	if (!prIe || !u2ApBufLen || u2ApBufLen < prIe->ucLength)
		return;
	LINK_MERGE_TO_TAIL(&prAPlist->rFreeLink, &prAPlist->rUsingLink);
	for (u2BufLen = u2ApBufLen; u2BufLen > 0; u2BufLen -= IE_SIZE(prIe),
		prIe = (struct IE_NEIGHBOR_REPORT_T *)((PUINT_8)prIe + IE_SIZE(prIe))) {
		/* BIT0-1: AP reachable, BIT2: same security with current setting,
		** BIT3: same authenticator with current AP
		*/
		if (prIe->ucId != ELEM_ID_NEIGHBOR_REPORT || (prIe->u4BSSIDInfo & 0x7) != 0x7)
			continue;
		LINK_MGMT_GET_ENTRY(prAPlist, prNeighborAP, struct NEIGHBOR_AP_T, VIR_MEM_TYPE);
		if (!prNeighborAP)
			break;
		prNeighborAP->fgHT = !!(prIe->u4BSSIDInfo & BIT(11));
		prNeighborAP->fgFromBtm = !!ucValidInterval;
		prNeighborAP->fgRmEnabled = !!(prIe->u4BSSIDInfo & BIT(7));
		prNeighborAP->fgQoS = !!(prIe->u4BSSIDInfo & BIT(5));
		prNeighborAP->fgSameMD = !!(prIe->u4BSSIDInfo & BIT(10));
		prNeighborAP->ucChannel = prIe->ucChnlNumber;
		prNeighborAP->ucPreference = aisGetNeighborApPreference(prIe->aucSubElem, IE_SIZE(prIe) -
			OFFSET_OF(struct IE_NEIGHBOR_REPORT_T, aucSubElem));
		prNeighborAP->u8TermTsf = aisGetBssTermTsf(prIe->aucSubElem, IE_SIZE(prIe) -
			OFFSET_OF(struct IE_NEIGHBOR_REPORT_T, aucSubElem));
		COPY_MAC_ADDR(prNeighborAP->aucBssid, prIe->aucBSSID);
		DBGLOG(SCN, INFO, "Bssid %pM, Pref %d, Chnl %d, BssidInfo 0x%08x\n", prNeighborAP->aucBssid,
			prNeighborAP->ucPreference, prIe->ucChnlNumber, prIe->u4BSSIDInfo);
		/* No need to save neighbor ap list with decendant preference
		** for (prTemp = LINK_ENTRY(prAPlist->rUsingLink.prNext, struct NEIGHBOR_AP_T, rLinkEntry);
		**	prTemp != prNeighborAP;
		**	prTemp = LINK_ENTRY(prTemp->rLinkEntry.prNext, struct NEIGHBOR_AP_T, rLinkEntry)) {
		**	if (prTemp->ucPreference < prNeighborAP->ucPreference) {
		**		__linkDel(prNeighborAP->rLinkEntry.prPrev, prNeighborAP->rLinkEntry.prNext);
		**		__linkAdd(&prNeighborAP->rLinkEntry, prTemp->rLinkEntry.prPrev, &prTemp->rLinkEntry);
		**		break;
		**	}
		** }
		*/
		if (u2BufLen < IE_SIZE(prIe)) {
			DBGLOG(AIS, WARN, "Truncated neighbor report\n");
			break;
		}
	}
	prAisSpecBssInfo->rNeiApRcvTime = kalGetTimeTick();
	prAisSpecBssInfo->u4NeiApValidInterval = !ucValidInterval ? 0xffffffff :
		TU_TO_MSEC(ucValidInterval * prAdapter->prAisBssInfo->u2BeaconInterval);
}

/* Check if neighbor AP list is still valid, called when AIS state is search or scan/online scan/looking for
** If invalid, free all neighbor ap list
*/
static VOID aisCheckNeighborApValidity(IN P_ADAPTER_T prAdapter)
{
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;
	struct LINK_MGMT *prAPlist = &prAisSpecBssInfo->rNeighborApList;
	OS_SYSTIME rCurrentTime = kalGetTimeTick();

	/* If candidate list is not timeout, just return */
	if (rCurrentTime <= prAisSpecBssInfo->rNeiApRcvTime ||
	    rCurrentTime - prAisSpecBssInfo->rNeiApRcvTime < prAisSpecBssInfo->u4NeiApValidInterval)
		return;

	DBGLOG(AIS, INFO, "timeout, Cur %u, Rcv at %u, Valid Interval %u\n", rCurrentTime,
	       prAisSpecBssInfo->rNeiApRcvTime, prAisSpecBssInfo->u4NeiApValidInterval);
	LINK_MERGE_TO_TAIL(&prAPlist->rFreeLink, &prAPlist->rUsingLink);
}

static VOID aisTxBtmResponseTimeout(IN P_ADAPTER_T prAdapter, ULONG ulParam)
{
	P_AIS_FSM_INFO_T prAisFsmInfo = &prAdapter->rWifiVar.rAisFsmInfo;

	prAdapter->rWifiVar.rAisSpecificBssInfo.rBTMParam.fgPendingResponse = FALSE;

	/* This timer will not be stopped. And after timeout, if not AIS_STATE_SEARCH,
	 * some event/cmd may occurs, just do nothing
	 */
	if (prAisFsmInfo->eCurrentState == AIS_STATE_SEARCH)
		aisFsmSteps(prAdapter, AIS_STATE_REQ_CHANNEL_JOIN);
}
