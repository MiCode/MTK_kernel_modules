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
 * Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/scan.c#4
 */

/*
 * ! \file   "scan.c"
 * \brief  This file defines the scan profile and the processing function of
 *       scan result for SCAN Module.
 *
 * The SCAN Profile selection is part of SCAN MODULE and responsible for defining
 * SCAN Parameters - e.g. MIN_CHANNEL_TIME, number of scan channels.
 * In this file we also define the process of SCAN Result including adding, searching
 * and removing SCAN record from the list.
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

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define REPLICATED_BEACON_TIME_THRESHOLD        (3000)
#define REPLICATED_BEACON_FRESH_PERIOD          (10000)
#define REPLICATED_BEACON_STRENGTH_THRESHOLD    (32)

/*
  * definition for AP selection algorithm
*/
#define BSS_FULL_SCORE                          100
#define CHNL_BSS_NUM_THRESOLD                   100
#define BSS_STA_CNT_THRESOLD                    30
#define SCORE_PER_AP                            1
#define ROAMING_NO_SWING_SCORE_STEP             100
#define SCORE_DIFF_THRESOLD_FOR_LOW_PREFERENCE  100
#define BEST_RSSI                               -48 /* MCS9 at BW 160 requires rssi at least -48dbm */
#define GOOD_RSSI_FOR_HT_VHT                    -64 /* MCS7 at 20BW, MCS5 at 40BW, MCS4 at 80BW, MCS3 at 160BW */
#define MINIMUM_RSSI_2G4                        -94 /* Link speed 1Mbps need at least rssi -94dbm for 2.4G */
#define MINIMUM_RSSI_5G                         -86 /* Link speed 6Mbps need at least rssi -86dbm for 5G */
/* Real Rssi of a Bss may range in current_rssi - 5 dbm  to current_rssi + 5 dbm */
#define RSSI_DIFF_BETWEEN_BSS                   10 /* dbm */
#define LOW_RSSI_FOR_5G_BAND                    -70 /* dbm */

#define WEIGHT_IDX_CHNL_UTIL                    0
#define WEIGHT_IDX_SNR                          0
#define WEIGHT_IDX_RSSI                         2
#define WEIGHT_IDX_SCN_MISS_CNT                 2
#define WEIGHT_IDX_PROBE_RSP                    1
#define WEIGHT_IDX_CLIENT_CNT                   0
#define WEIGHT_IDX_AP_NUM                       0
#define WEIGHT_IDX_5G_BAND                      2
#define WEIGHT_IDX_BAND_WIDTH                   1
#define WEIGHT_IDX_STBC                         1
#define WEIGHT_IDX_DEAUTH_LAST                  1
#define WEIGHT_IDX_BLACK_LIST                   2
#define WEIGHT_IDX_PREFERENCE                   3
#define WEIGHT_IDX_EXCLUDE                      6

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
struct REF_TSF {
	UINT_32 au4Tsf[2];
	OS_SYSTIME rTime;
};
static struct REF_TSF rTsf;
/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static WLAN_STATUS
	__scanProcessBeaconAndProbeResp(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
/*----------------------------------------------------------------------------*/
/*!
* @brief This function is used by SCN to initialize its variables
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID scnInit(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_BSS_DESC_T prBSSDesc;
	P_ROAM_BSS_DESC_T prRoamBSSDesc;
	PUINT_8 pucBSSBuff;
	PUINT_8 pucRoamBSSBuff;
	UINT_32 i;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	pucBSSBuff = &prScanInfo->aucScanBuffer[0];
	pucRoamBSSBuff = &prScanInfo->aucScanRoamBuffer[0];

	DBGLOG(SCN, TRACE, "->scnInit()\n");

	/* 4 <1> Reset STATE and Message List */
	prScanInfo->eCurrentState = SCAN_STATE_IDLE;

	prScanInfo->rLastScanCompletedTime = (OS_SYSTIME) 0;

	LINK_INITIALIZE(&prScanInfo->rPendingMsgList);

	/* 4 <2> Reset link list of BSS_DESC_T */
	kalMemZero((PVOID) pucBSSBuff, SCN_MAX_BUFFER_SIZE);
	kalMemZero((PVOID) pucRoamBSSBuff, SCN_ROAM_MAX_BUFFER_SIZE);

	LINK_INITIALIZE(&prScanInfo->rFreeBSSDescList);
	LINK_INITIALIZE(&prScanInfo->rBSSDescList);
	LINK_INITIALIZE(&prScanInfo->rRoamFreeBSSDescList);
	LINK_INITIALIZE(&prScanInfo->rRoamBSSDescList);

	for (i = 0; i < CFG_MAX_NUM_BSS_LIST; i++) {

		prBSSDesc = (P_BSS_DESC_T) pucBSSBuff;

		LINK_INSERT_TAIL(&prScanInfo->rFreeBSSDescList, &prBSSDesc->rLinkEntry);

		pucBSSBuff += ALIGN_4(sizeof(BSS_DESC_T));
	}
	/* Check if the memory allocation consist with this initialization function */
	ASSERT(((ULONG) pucBSSBuff - (ULONG)&prScanInfo->aucScanBuffer[0]) == SCN_MAX_BUFFER_SIZE);

	for (i = 0; i < CFG_MAX_NUM_ROAM_BSS_LIST; i++) {
		prRoamBSSDesc = (P_ROAM_BSS_DESC_T) pucRoamBSSBuff;

		LINK_INSERT_TAIL(&prScanInfo->rRoamFreeBSSDescList, &prRoamBSSDesc->rLinkEntry);

		pucRoamBSSBuff += ALIGN_4(sizeof(ROAM_BSS_DESC_T));
	}
	ASSERT(((ULONG)pucRoamBSSBuff - (ULONG)&prScanInfo->aucScanRoamBuffer[0]) ==
	       SCN_ROAM_MAX_BUFFER_SIZE);
	/* reset freest channel information */
	prScanInfo->fgIsSparseChannelValid = FALSE;

	/* reset NLO state */
	prScanInfo->fgNloScanning = FALSE;
#if CFG_SUPPORT_SCN_PSCN
	prScanInfo->fgPscnOngoing = FALSE;
	prScanInfo->fgGScnConfigSet = FALSE;
	prScanInfo->fgGScnParamSet = FALSE;
	prScanInfo->prPscnParam = kalMemAlloc(sizeof(CMD_SET_PSCAN_PARAM), VIR_MEM_TYPE);
	if (!(prScanInfo->prPscnParam)) {
		DBGLOG(SCN, ERROR, "Alloc memory for CMD_SET_PSCAN_PARAM fail\n");
		return;
	}
	kalMemZero(prScanInfo->prPscnParam, sizeof(CMD_SET_PSCAN_PARAM));

	prScanInfo->eCurrentPSCNState = PSCN_IDLE;
#endif

#if CFG_SUPPORT_GSCN
	prScanInfo->prGscnFullResult = kalMemAlloc(offsetof(PARAM_WIFI_GSCAN_FULL_RESULT, ie_data)
			+ CFG_IE_BUFFER_SIZE, VIR_MEM_TYPE);
	if (!(prScanInfo->prGscnFullResult)) {
#if CFG_SUPPORT_SCN_PSCN
		kalMemFree(prScanInfo->prPscnParam, VIR_MEM_TYPE, sizeof(CMD_SET_PSCAN_PARAM));
#endif
		DBGLOG(SCN, ERROR, "Alloc memory for PARAM_WIFI_GSCAN_FULL_RESULT fail\n");
		return;
	}
	kalMemZero(prScanInfo->prGscnFullResult,
		offsetof(PARAM_WIFI_GSCAN_FULL_RESULT, ie_data) + CFG_IE_BUFFER_SIZE);
#endif

	cnmTimerInitTimer(prAdapter,
			  &prScanInfo->rScanDoneTimer, (PFN_MGMT_TIMEOUT_FUNC) scnScanDoneTimeout, (ULONG) NULL);
	prScanInfo->ucScanDoneTimeoutCnt = 0;

}				/* end of scnInit() */

VOID scnFreeAllPendingScanRquests(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_MSG_HDR_T prMsgHdr;
	P_MSG_SCN_SCAN_REQ prScanReqMsg;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	/* check for pending scanning requests */
	while (!LINK_IS_EMPTY(&(prScanInfo->rPendingMsgList))) {

		/* load next message from pending list as scan parameters */
		LINK_REMOVE_HEAD(&(prScanInfo->rPendingMsgList), prMsgHdr, P_MSG_HDR_T);
		if (prMsgHdr) {
			prScanReqMsg = (P_MSG_SCN_SCAN_REQ) prMsgHdr;
			DBGLOG(SCN, INFO,
			       "free scan request eMsgId[%d] ucSeqNum [%d] BSSID[%d]!!\n", prMsgHdr->eMsgId,
			       prScanReqMsg->ucSeqNum, prScanReqMsg->ucBssIndex);
			cnmMemFree(prAdapter, prMsgHdr);
		} else {
			/* should not deliver to this function */
			ASSERT(0);
		}
		/* switch to next state */
	}
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is used by SCN to uninitialize its variables
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID scnUninit(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	DBGLOG(SCN, TRACE, "->scnUninit()\n");

	scnFreeAllPendingScanRquests(prAdapter);

	/* 4 <1> Reset STATE and Message List */
	prScanInfo->eCurrentState = SCAN_STATE_IDLE;

	prScanInfo->rLastScanCompletedTime = (OS_SYSTIME) 0;

	/* NOTE(Kevin): Check rPendingMsgList ? */

	/* 4 <2> Reset link list of BSS_DESC_T */
	LINK_INITIALIZE(&prScanInfo->rFreeBSSDescList);
	LINK_INITIALIZE(&prScanInfo->rBSSDescList);
	LINK_INITIALIZE(&prScanInfo->rRoamFreeBSSDescList);
	LINK_INITIALIZE(&prScanInfo->rRoamBSSDescList);
#if CFG_SUPPORT_SCN_PSCN
	kalMemFree(prScanInfo->prPscnParam, VIR_MEM_TYPE, sizeof(CMD_SET_PSCAN_PARAM));

	prScanInfo->eCurrentPSCNState = PSCN_IDLE;
#endif

#if CFG_SUPPORT_GSCN
	kalMemFree(prScanInfo->prGscnFullResult, VIR_MEM_TYPE,
		offsetof(PARAM_WIFI_GSCAN_FULL_RESULT, ie_data) + CFG_IE_BUFFER_SIZE);
#endif

	cnmTimerStopTimer(prAdapter, &prScanInfo->rScanDoneTimer);
}				/* end of scnUninit() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Find the corresponding BSS Descriptor according to given BSSID
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] aucBSSID           Given BSSID.
*
* @return   Pointer to BSS Descriptor, if found. NULL, if not found
*/
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanSearchBssDescByBssid(IN P_ADAPTER_T prAdapter, IN UINT_8 aucBSSID[])
{
	return scanSearchBssDescByBssidAndSsid(prAdapter, aucBSSID, FALSE, NULL);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Find the corresponding BSS Descriptor according to given BSSID
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] aucBSSID           Given BSSID.
* @param[in] fgCheckSsid        Need to check SSID or not. (for multiple SSID with single BSSID cases)
* @param[in] prSsid             Specified SSID
*
* @return   Pointer to BSS Descriptor, if found. NULL, if not found
*/
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T
scanSearchBssDescByBssidAndSsid(IN P_ADAPTER_T prAdapter,
				IN UINT_8 aucBSSID[], IN BOOLEAN fgCheckSsid, IN P_PARAM_SSID_T prSsid)
{
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prBSSDescList;
	P_BSS_DESC_T prBssDesc;
	P_BSS_DESC_T prDstBssDesc = (P_BSS_DESC_T) NULL;

	ASSERT(prAdapter);
	ASSERT(aucBSSID);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prBSSDescList = &prScanInfo->rBSSDescList;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {

		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID)) {
			if (fgCheckSsid == FALSE || prSsid == NULL)
				return prBssDesc;

			if (EQUAL_SSID(prBssDesc->aucSSID,
				       prBssDesc->ucSSIDLen, prSsid->aucSsid, prSsid->u4SsidLen)) {
				return prBssDesc;
			} else if (prDstBssDesc == NULL && prBssDesc->fgIsHiddenSSID == TRUE) {
				prDstBssDesc = prBssDesc;
			} else if (prBssDesc->eBSSType == BSS_TYPE_P2P_DEVICE) {
				/*
				 * 20120206 frog: Equal BSSID but not SSID,
				 * SSID not hidden, SSID must be updated.
				 */
				COPY_SSID(prBssDesc->aucSSID,
					  prBssDesc->ucSSIDLen, prSsid->aucSsid, (UINT_8) (prSsid->u4SsidLen));
				return prBssDesc;
			}
		}
	}

	return prDstBssDesc;
}				/* end of scanSearchBssDescByBssid() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Find the corresponding BSS Descriptor according to given Transmitter Address.
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] aucSrcAddr         Given Source Address(TA).
*
* @return   Pointer to BSS Descriptor, if found. NULL, if not found
*/
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanSearchBssDescByTA(IN P_ADAPTER_T prAdapter, IN UINT_8 aucSrcAddr[])
{
	return scanSearchBssDescByTAAndSsid(prAdapter, aucSrcAddr, FALSE, NULL);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Find the corresponding BSS Descriptor according to given Transmitter Address.
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] aucSrcAddr         Given Source Address(TA).
* @param[in] fgCheckSsid        Need to check SSID or not. (for multiple SSID with single BSSID cases)
* @param[in] prSsid             Specified SSID
*
* @return   Pointer to BSS Descriptor, if found. NULL, if not found
*/
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T
scanSearchBssDescByTAAndSsid(IN P_ADAPTER_T prAdapter,
			     IN UINT_8 aucSrcAddr[], IN BOOLEAN fgCheckSsid, IN P_PARAM_SSID_T prSsid)
{
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prBSSDescList;
	P_BSS_DESC_T prBssDesc;
	P_BSS_DESC_T prDstBssDesc = (P_BSS_DESC_T) NULL;

	ASSERT(prAdapter);
	ASSERT(aucSrcAddr);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prBSSDescList = &prScanInfo->rBSSDescList;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {

		if (EQUAL_MAC_ADDR(prBssDesc->aucSrcAddr, aucSrcAddr)) {
			if (fgCheckSsid == FALSE || prSsid == NULL)
				return prBssDesc;

			if (EQUAL_SSID(prBssDesc->aucSSID,
				       prBssDesc->ucSSIDLen, prSsid->aucSsid, prSsid->u4SsidLen)) {
				return prBssDesc;
			} else if (prDstBssDesc == NULL && prBssDesc->fgIsHiddenSSID == TRUE) {
				prDstBssDesc = prBssDesc;
			}
		}
	}

	return prDstBssDesc;

}				/* end of scanSearchBssDescByTA() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Find the corresponding BSS Descriptor according to
*        given eBSSType, BSSID and Transmitter Address
*
* @param[in] prAdapter  Pointer to the Adapter structure.
* @param[in] eBSSType   BSS Type of incoming Beacon/ProbeResp frame.
* @param[in] aucBSSID   Given BSSID of Beacon/ProbeResp frame.
* @param[in] aucSrcAddr Given source address (TA) of Beacon/ProbeResp frame.
*
* @return   Pointer to BSS Descriptor, if found. NULL, if not found
*/
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T
scanSearchExistingBssDesc(IN P_ADAPTER_T prAdapter,
			  IN ENUM_BSS_TYPE_T eBSSType, IN UINT_8 aucBSSID[], IN UINT_8 aucSrcAddr[])
{
	return scanSearchExistingBssDescWithSsid(prAdapter, eBSSType, aucBSSID, aucSrcAddr, FALSE, NULL);
}

VOID scanRemoveRoamBssDescsByTime(IN P_ADAPTER_T prAdapter, IN UINT_32 u4RemoveTime)
{
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prRoamBSSDescList;
	P_LINK_T prRoamFreeBSSDescList;
	P_ROAM_BSS_DESC_T prRoamBssDesc;
	P_ROAM_BSS_DESC_T prRoamBSSDescNext;
	OS_SYSTIME rCurrentTime;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prRoamBSSDescList = &prScanInfo->rRoamBSSDescList;
	prRoamFreeBSSDescList = &prScanInfo->rRoamFreeBSSDescList;

	GET_CURRENT_SYSTIME(&rCurrentTime);

	LINK_FOR_EACH_ENTRY_SAFE(prRoamBssDesc, prRoamBSSDescNext, prRoamBSSDescList, rLinkEntry,
				 ROAM_BSS_DESC_T) {

		if (CHECK_FOR_TIMEOUT(rCurrentTime, prRoamBssDesc->rUpdateTime,
				      SEC_TO_SYSTIME(u4RemoveTime))) {

			LINK_REMOVE_KNOWN_ENTRY(prRoamBSSDescList, prRoamBssDesc);
			LINK_INSERT_TAIL(prRoamFreeBSSDescList, &prRoamBssDesc->rLinkEntry);
		}
	}
}

P_ROAM_BSS_DESC_T
scanSearchRoamBssDescBySsid(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBssDesc)
{
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prRoamBSSDescList;
	P_ROAM_BSS_DESC_T prRoamBssDesc;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prRoamBSSDescList = &prScanInfo->rRoamBSSDescList;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prRoamBssDesc, prRoamBSSDescList, rLinkEntry, ROAM_BSS_DESC_T) {
		if (EQUAL_SSID(prRoamBssDesc->aucSSID, prRoamBssDesc->ucSSIDLen,
				       prBssDesc->aucSSID, prBssDesc->ucSSIDLen)) {
			return prRoamBssDesc;
		}
	}

	return NULL;

}

P_ROAM_BSS_DESC_T scanAllocateRoamBssDesc(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prRoamFreeBSSDescList;
	P_ROAM_BSS_DESC_T prRoamBssDesc = NULL;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prRoamFreeBSSDescList = &prScanInfo->rRoamFreeBSSDescList;

	LINK_REMOVE_HEAD(prRoamFreeBSSDescList, prRoamBssDesc, P_ROAM_BSS_DESC_T);

	if (prRoamBssDesc) {
		P_LINK_T prRoamBSSDescList;

		kalMemZero(prRoamBssDesc, sizeof(ROAM_BSS_DESC_T));

		prRoamBSSDescList = &prScanInfo->rRoamBSSDescList;

		LINK_INSERT_HEAD(prRoamBSSDescList, &prRoamBssDesc->rLinkEntry);
	}

	return prRoamBssDesc;
}

VOID scanAddToRoamBssDesc(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBssDesc)
{
	P_ROAM_BSS_DESC_T prRoamBssDesc;

	prRoamBssDesc = scanSearchRoamBssDescBySsid(prAdapter, prBssDesc);

	if (prRoamBssDesc == NULL) {
		UINT_32 u4RemoveTime = REMOVE_TIMEOUT_TWO_DAY;

		do {
			prRoamBssDesc = scanAllocateRoamBssDesc(prAdapter);
			if (prRoamBssDesc)
				break;
			scanRemoveRoamBssDescsByTime(prAdapter, u4RemoveTime);
			u4RemoveTime = u4RemoveTime / 2;
		} while (u4RemoveTime > 0);
		if (!prRoamBssDesc) {
			DBGLOG(SCN, ERROR, "Allocate Roam Bss Desc failed\n");
			return;
		}
		COPY_SSID(prRoamBssDesc->aucSSID, prRoamBssDesc->ucSSIDLen,
				prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
	}

	GET_CURRENT_SYSTIME(&prRoamBssDesc->rUpdateTime);
}

VOID scanSearchBssDescOfRoamSsid(IN P_ADAPTER_T prAdapter)
{
#define SSID_ONLY_EXIST_ONE_AP      1    /* If only exist one same ssid AP, avoid unnecessary scan */

	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prBSSDescList;
	P_BSS_DESC_T prBssDesc;
	P_BSS_INFO_T prAisBssInfo;
	UINT_32	u4SameSSIDCount = 0;

	prAisBssInfo = prAdapter->prAisBssInfo;
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;

	if (prAisBssInfo->eConnectionState != PARAM_MEDIA_STATE_CONNECTED)
		return;

	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {
		if (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
				       prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen)) {
			u4SameSSIDCount++;
			if (u4SameSSIDCount > SSID_ONLY_EXIST_ONE_AP) {
				scanAddToRoamBssDesc(prAdapter, prBssDesc);
				break;
			}
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Find the corresponding BSS Descriptor according to
*        given eBSSType, BSSID and Transmitter Address
*
* @param[in] prAdapter  Pointer to the Adapter structure.
* @param[in] eBSSType   BSS Type of incoming Beacon/ProbeResp frame.
* @param[in] aucBSSID   Given BSSID of Beacon/ProbeResp frame.
* @param[in] aucSrcAddr Given source address (TA) of Beacon/ProbeResp frame.
* @param[in] fgCheckSsid Need to check SSID or not. (for multiple SSID with single BSSID cases)
* @param[in] prSsid     Specified SSID
*
* @return   Pointer to BSS Descriptor, if found. NULL, if not found
*/
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T
scanSearchExistingBssDescWithSsid(IN P_ADAPTER_T prAdapter,
				  IN ENUM_BSS_TYPE_T eBSSType,
				  IN UINT_8 aucBSSID[],
				  IN UINT_8 aucSrcAddr[], IN BOOLEAN fgCheckSsid, IN P_PARAM_SSID_T prSsid)
{
	P_SCAN_INFO_T prScanInfo;
	P_BSS_DESC_T prBssDesc, prIBSSBssDesc;

	ASSERT(prAdapter);
	ASSERT(aucSrcAddr);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	switch (eBSSType) {
	case BSS_TYPE_P2P_DEVICE:
		fgCheckSsid = FALSE;
		/* This case need to fall through */
	case BSS_TYPE_INFRASTRUCTURE:
		scanSearchBssDescOfRoamSsid(prAdapter);
		/* This case need to fall through */
	case BSS_TYPE_BOW_DEVICE:
		{
			prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter, aucBSSID, fgCheckSsid, prSsid);

			/* if (eBSSType == prBssDesc->eBSSType) */

			return prBssDesc;
		}

	case BSS_TYPE_IBSS:
		{
			prIBSSBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter, aucBSSID, fgCheckSsid, prSsid);
			prBssDesc = scanSearchBssDescByTAAndSsid(prAdapter, aucSrcAddr, fgCheckSsid, prSsid);

			/* NOTE(Kevin):
			 * Rules to maintain the SCAN Result:
			 * For AdHoc -
			 *    CASE I    We have TA1(BSSID1), but it change its BSSID to BSSID2
			 *              -> Update TA1 entry's BSSID.
			 *    CASE II   We have TA1(BSSID1), and get TA1(BSSID1) again
			 *              -> Update TA1 entry's contain.
			 *    CASE III  We have a SCAN result TA1(BSSID1), and TA2(BSSID2). Sooner or
			 *               later, TA2 merge into TA1, we get TA2(BSSID1)
			 *              -> Remove TA2 first and then replace TA1 entry's TA with TA2,
			 *                 Still have only one entry of BSSID.
			 *    CASE IV   We have a SCAN result TA1(BSSID1), and another TA2 also merge into BSSID1.
			 *              -> Replace TA1 entry's TA with TA2, Still have only one entry.
			 *    CASE V    New IBSS
			 *              -> Add this one to SCAN result.
			 */
			if (prBssDesc) {
				P_LINK_T prBSSDescList;
				P_LINK_T prFreeBSSDescList;

				if ((!prIBSSBssDesc) ||	/* CASE I */
				    (prBssDesc == prIBSSBssDesc)) {	/* CASE II */

					return prBssDesc;
				}
				/* CASE III */
				prBSSDescList = &prScanInfo->rBSSDescList;
				prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

				/* Remove this BSS Desc from the BSS Desc list */
				LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);

				/* Return this BSS Desc to the free BSS Desc list. */
				LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDesc->rLinkEntry);

				return prIBSSBssDesc;
			}

			if (prIBSSBssDesc) {	/* CASE IV */

				return prIBSSBssDesc;
			}
			/* CASE V */
			break;	/* Return NULL; */
		}

	default:
		break;
	}

	return (P_BSS_DESC_T) NULL;

}				/* end of scanSearchExistingBssDesc() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Delete BSS Descriptors from current list according to given Remove Policy.
*
* @param[in] u4RemovePolicy     Remove Policy.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID scanRemoveBssDescsByPolicy(IN P_ADAPTER_T prAdapter, IN UINT_32 u4RemovePolicy)
{
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prBSSDescList;
	P_LINK_T prFreeBSSDescList;
	P_BSS_DESC_T prBssDesc;
	P_LINK_T prEssList = &prAdapter->rWifiVar.rAisSpecificBssInfo.rCurEssLink;

	ASSERT(prAdapter);

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;
	prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

	/* DBGLOG(SCN, TRACE, ("Before Remove - Number Of SCAN Result = %ld\n", */
	/* prBSSDescList->u4NumElem)); */

	if (u4RemovePolicy & SCN_RM_POLICY_TIMEOUT) {
		P_BSS_DESC_T prBSSDescNext;
		OS_SYSTIME rCurrentTime;

		GET_CURRENT_SYSTIME(&rCurrentTime);

		/* Search BSS Desc from current SCAN result list. */
		LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext, prBSSDescList, rLinkEntry, BSS_DESC_T) {

			if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED) &&
			    (prBssDesc->fgIsConnected || prBssDesc->fgIsConnecting)) {
				/* Don't remove the one currently we are connected. */
				continue;
			}

			if (CHECK_FOR_TIMEOUT(rCurrentTime, prBssDesc->rUpdateTime,
					      SEC_TO_SYSTIME(SCN_BSS_DESC_REMOVE_TIMEOUT_SEC))) {

				/*
				 * DBGLOG(SCN, TRACE, ("Remove TIMEOUT BSS DESC(%#x):
				 * MAC: "MACSTR", Current Time = %08lx, Update Time = %08lx\n",
				 */
				/* prBssDesc, MAC2STR(prBssDesc->aucBSSID), rCurrentTime, prBssDesc->rUpdateTime)); */
				if (!prBssDesc->prBlack)
					aisQueryBlackList(prAdapter, prBssDesc);
				if (prBssDesc->prBlack)
					prBssDesc->prBlack->u8DisapperTime = kalGetBootTime();

				/* Remove this BSS Desc from the BSS Desc list */
				LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);
				/* Remove this BSS Desc from the Ess Desc List */
				if (LINK_ENTRY_IS_VALID(&prBssDesc->rLinkEntryEss))
					LINK_REMOVE_KNOWN_ENTRY(prEssList, &prBssDesc->rLinkEntryEss);

				/* Return this BSS Desc to the free BSS Desc list. */
				LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDesc->rLinkEntry);
			}
		}
	} else if (u4RemovePolicy & SCN_RM_POLICY_OLDEST_HIDDEN) {
		P_BSS_DESC_T prBssDescOldest = (P_BSS_DESC_T) NULL;

		/* Search BSS Desc from current SCAN result list. */
		LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {

			if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED) &&
			    (prBssDesc->fgIsConnected || prBssDesc->fgIsConnecting)) {
				/* Don't remove the one currently we are connected. */
				continue;
			}

			if (!prBssDesc->fgIsHiddenSSID)
				continue;

			if (!prBssDescOldest) {	/* 1st element */
				prBssDescOldest = prBssDesc;
				continue;
			}

			if (TIME_BEFORE(prBssDesc->rUpdateTime, prBssDescOldest->rUpdateTime))
				prBssDescOldest = prBssDesc;
		}

		if (prBssDescOldest) {
			/*
			 * DBGLOG(SCN, TRACE,
			 * ("Remove OLDEST HIDDEN BSS DESC(%#x): MAC: "MACSTR", Update Time = %08lx\n",
			 */
			/* prBssDescOldest, MAC2STR(prBssDescOldest->aucBSSID), prBssDescOldest->rUpdateTime)); */
			if (!prBssDescOldest->prBlack)
				aisQueryBlackList(prAdapter, prBssDescOldest);
			if (prBssDescOldest->prBlack)
				prBssDescOldest->prBlack->u8DisapperTime = kalGetBootTime();

			/* Remove this BSS Desc from the BSS Desc list */
			LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDescOldest);
			/* Remove this BSS Desc from the Ess Desc List */
			if (LINK_ENTRY_IS_VALID(&prBssDescOldest->rLinkEntryEss))
				LINK_REMOVE_KNOWN_ENTRY(prEssList, &prBssDescOldest->rLinkEntryEss);

			/* Return this BSS Desc to the free BSS Desc list. */
			LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDescOldest->rLinkEntry);
		}
	} else if (u4RemovePolicy & SCN_RM_POLICY_SMART_WEAKEST) {
		P_BSS_DESC_T prBssDescWeakest = (P_BSS_DESC_T) NULL;
		P_BSS_DESC_T prBssDescWeakestSameSSID = (P_BSS_DESC_T) NULL;
		UINT_32 u4SameSSIDCount = 0;

		/* Search BSS Desc from current SCAN result list. */
		LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {

			if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED) &&
			    (prBssDesc->fgIsConnected || prBssDesc->fgIsConnecting)) {
				/* Don't remove the one currently we are connected. */
				continue;
			}

			if ((!prBssDesc->fgIsHiddenSSID) &&
			    (EQUAL_SSID(prBssDesc->aucSSID,
					prBssDesc->ucSSIDLen, prConnSettings->aucSSID, prConnSettings->ucSSIDLen))) {

				u4SameSSIDCount++;

				if (!prBssDescWeakestSameSSID)
					prBssDescWeakestSameSSID = prBssDesc;
				else if (prBssDesc->ucRCPI < prBssDescWeakestSameSSID->ucRCPI)
					prBssDescWeakestSameSSID = prBssDesc;
				if (u4SameSSIDCount < SCN_BSS_DESC_SAME_SSID_THRESHOLD)
					continue;
			}

			if (!prBssDescWeakest) {	/* 1st element */
				prBssDescWeakest = prBssDesc;
				continue;
			}

			if (prBssDesc->ucRCPI < prBssDescWeakest->ucRCPI)
				prBssDescWeakest = prBssDesc;

		}

		if ((u4SameSSIDCount >= SCN_BSS_DESC_SAME_SSID_THRESHOLD) && (prBssDescWeakestSameSSID))
			prBssDescWeakest = prBssDescWeakestSameSSID;

		if (prBssDescWeakest) {

			/* DBGLOG(SCN, TRACE, ("Remove WEAKEST BSS DESC(%#x): MAC: "MACSTR", Update Time = %08lx\n", */
			/* prBssDescOldest, MAC2STR(prBssDescOldest->aucBSSID), prBssDescOldest->rUpdateTime)); */
			if (!prBssDescWeakest->prBlack)
				aisQueryBlackList(prAdapter, prBssDescWeakest);
			if (prBssDescWeakest->prBlack)
				prBssDescWeakest->prBlack->u8DisapperTime = kalGetBootTime();

			/* Remove this BSS Desc from the BSS Desc list */
			LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDescWeakest);
			/* Remove this BSS Desc from the Ess Desc List */
			if (LINK_ENTRY_IS_VALID(&prBssDescWeakest->rLinkEntryEss))
				LINK_REMOVE_KNOWN_ENTRY(prEssList, &prBssDescWeakest->rLinkEntryEss);
			/* Return this BSS Desc to the free BSS Desc list. */
			LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDescWeakest->rLinkEntry);
		}
	} else if (u4RemovePolicy & SCN_RM_POLICY_ENTIRE) {
		P_BSS_DESC_T prBSSDescNext;
		UINT_64 u8Current = kalGetBootTime();

		LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext, prBSSDescList, rLinkEntry, BSS_DESC_T) {

			if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED) &&
			    (prBssDesc->fgIsConnected || prBssDesc->fgIsConnecting)) {
				/* Don't remove the one currently we are connected. */
				continue;
			}
			if (!prBssDesc->prBlack)
				aisQueryBlackList(prAdapter, prBssDesc);
			if (prBssDesc->prBlack)
				prBssDesc->prBlack->u8DisapperTime = u8Current;

			/* Remove this BSS Desc from the BSS Desc list */
			LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);
			/* Remove this BSS Desc from the Ess Desc List */
			if (LINK_ENTRY_IS_VALID(&prBssDesc->rLinkEntryEss))
				LINK_REMOVE_KNOWN_ENTRY(prEssList, &prBssDesc->rLinkEntryEss);

			/* Return this BSS Desc to the free BSS Desc list. */
			LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDesc->rLinkEntry);
		}

	}

	return;

}				/* end of scanRemoveBssDescsByPolicy() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Delete BSS Descriptors from current list according to given BSSID.
*
* @param[in] prAdapter  Pointer to the Adapter structure.
* @param[in] aucBSSID   Given BSSID.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID scanRemoveBssDescByBssid(IN P_ADAPTER_T prAdapter, IN UINT_8 aucBSSID[])
{
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prBSSDescList;
	P_LINK_T prFreeBSSDescList;
	P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T) NULL;
	P_BSS_DESC_T prBSSDescNext;
	P_LINK_T prEssList = NULL;

	ASSERT(prAdapter);
	ASSERT(aucBSSID);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;
	prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;
	prEssList = &prAdapter->rWifiVar.rAisSpecificBssInfo.rCurEssLink;

	/* Check if such BSS Descriptor exists in a valid list */
	LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext, prBSSDescList, rLinkEntry, BSS_DESC_T) {

		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID)) {
			if (!prBssDesc->prBlack)
				aisQueryBlackList(prAdapter, prBssDesc);
			if (prBssDesc->prBlack)
				prBssDesc->prBlack->u8DisapperTime = kalGetBootTime();
			/* Remove this BSS Desc from the BSS Desc list */
			LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);
			/* Remove this BSS Desc from the Ess Desc List */
			if (LINK_ENTRY_IS_VALID(&prBssDesc->rLinkEntryEss))
				LINK_REMOVE_KNOWN_ENTRY(prEssList, &prBssDesc->rLinkEntryEss);
			/* Return this BSS Desc to the free BSS Desc list. */
			LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDesc->rLinkEntry);

			/* BSSID is not unique, so need to traverse whols link-list */
		}
	}
}				/* end of scanRemoveBssDescByBssid() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Delete BSS Descriptors from current list according to given band configuration
*
* @param[in] prAdapter  Pointer to the Adapter structure.
* @param[in] eBand      Given band
* @param[in] ucBssIndex     AIS - Remove IBSS/Infrastructure BSS
*                           BOW - Remove BOW BSS
*                           P2P - Remove P2P BSS
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID scanRemoveBssDescByBandAndNetwork(IN P_ADAPTER_T prAdapter, IN ENUM_BAND_T eBand, IN UINT_8 ucBssIndex)
{
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prBSSDescList;
	P_LINK_T prFreeBSSDescList;
	P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T) NULL;
	P_BSS_DESC_T prBSSDescNext;
	BOOLEAN fgToRemove;

	ASSERT(prAdapter);
	ASSERT(eBand <= BAND_NUM);
	ASSERT(ucBssIndex <= MAX_BSS_INDEX);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;
	prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

	if (eBand == BAND_NULL)
		return;		/* no need to do anything, keep all scan result */

	/* Check if such BSS Descriptor exists in a valid list */
	LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext, prBSSDescList, rLinkEntry, BSS_DESC_T) {
		fgToRemove = FALSE;

		if (prBssDesc->eBand == eBand) {
			switch (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType) {
			case NETWORK_TYPE_AIS:
				if ((prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE)
				    || (prBssDesc->eBSSType == BSS_TYPE_IBSS)) {
					fgToRemove = TRUE;
				}
				break;

			case NETWORK_TYPE_P2P:
				if (prBssDesc->eBSSType == BSS_TYPE_P2P_DEVICE)
					fgToRemove = TRUE;
				break;

			case NETWORK_TYPE_BOW:
				if (prBssDesc->eBSSType == BSS_TYPE_BOW_DEVICE)
					fgToRemove = TRUE;
				break;

			default:
				ASSERT(0);
				break;
			}
		}

		if (fgToRemove == TRUE) {
			P_LINK_T prEssList = &prAdapter->rWifiVar.rAisSpecificBssInfo.rCurEssLink;

			if (!prBssDesc->prBlack)
				aisQueryBlackList(prAdapter, prBssDesc);
			if (prBssDesc->prBlack)
				prBssDesc->prBlack->u8DisapperTime = kalGetBootTime();

			/* Remove this BSS Desc from the BSS Desc list */
			LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);
			/* Remove this BSS Desc from the Ess Desc List */
			if (LINK_ENTRY_IS_VALID(&prBssDesc->rLinkEntryEss))
				LINK_REMOVE_KNOWN_ENTRY(prEssList, &prBssDesc->rLinkEntryEss);

			/* Return this BSS Desc to the free BSS Desc list. */
			LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDesc->rLinkEntry);
		}
	}

}				/* end of scanRemoveBssDescByBand() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Clear the CONNECTION FLAG of a specified BSS Descriptor.
*
* @param[in] aucBSSID   Given BSSID.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID scanRemoveConnFlagOfBssDescByBssid(IN P_ADAPTER_T prAdapter, IN UINT_8 aucBSSID[])
{
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prBSSDescList;
	P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T) NULL;

	ASSERT(prAdapter);
	ASSERT(aucBSSID);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {

		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID)) {
			prBssDesc->fgIsConnected = FALSE;
			prBssDesc->fgIsConnecting = FALSE;

			/* BSSID is not unique, so need to traverse whols link-list */
		}
	}

	return;

}				/* end of scanRemoveConnectionFlagOfBssDescByBssid() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Allocate new BSS_DESC_T
*
* @param[in] prAdapter          Pointer to the Adapter structure.
*
* @return   Pointer to BSS Descriptor, if has free space. NULL, if has no space.
*/
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanAllocateBssDesc(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prFreeBSSDescList;
	P_BSS_DESC_T prBssDesc;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

	LINK_REMOVE_HEAD(prFreeBSSDescList, prBssDesc, P_BSS_DESC_T);

	if (prBssDesc) {
		P_LINK_T prBSSDescList;

		kalMemZero(prBssDesc, sizeof(BSS_DESC_T));

#if CFG_ENABLE_WIFI_DIRECT
		LINK_INITIALIZE(&(prBssDesc->rP2pDeviceList));
		prBssDesc->fgIsP2PPresent = FALSE;
#endif /* CFG_ENABLE_WIFI_DIRECT */

		prBSSDescList = &prScanInfo->rBSSDescList;

		/* NOTE(Kevin): In current design, this new empty BSS_DESC_T will be
		 * inserted to BSSDescList immediately.
		 */
		LINK_INSERT_TAIL(prBSSDescList, &prBssDesc->rLinkEntry);
	}

	return prBssDesc;

}				/* end of scanAllocateBssDesc() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This API parses Beacon/ProbeResp frame and insert extracted BSS_DESC_T
*        with IEs into prAdapter->rWifiVar.rScanInfo.aucScanBuffer
*
* @param[in] prAdapter      Pointer to the Adapter structure.
* @param[in] prSwRfb        Pointer to the receiving frame buffer.
*
* @return   Pointer to BSS Descriptor
*           NULL if the Beacon/ProbeResp frame is invalid
*/
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanAddToBssDesc(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
	P_BSS_DESC_T prBssDesc = NULL;
	UINT_16 u2CapInfo;
	ENUM_BSS_TYPE_T eBSSType = BSS_TYPE_INFRASTRUCTURE;

	PUINT_8 pucIE;
	UINT_16 u2IELength;
	UINT_16 u2Offset = 0;

	P_WLAN_BEACON_FRAME_T prWlanBeaconFrame = (P_WLAN_BEACON_FRAME_T) NULL;
	P_IE_SSID_T prIeSsid = (P_IE_SSID_T) NULL;
	P_IE_SUPPORTED_RATE_T prIeSupportedRate = (P_IE_SUPPORTED_RATE_T) NULL;
	P_IE_EXT_SUPPORTED_RATE_T prIeExtSupportedRate = (P_IE_EXT_SUPPORTED_RATE_T) NULL;
	UINT_8 ucHwChannelNum = 0;
	UINT_8 ucIeDsChannelNum = 0;
	UINT_8 ucIeHtChannelNum = 0;
	BOOLEAN fgIsValidSsid = FALSE;
	PARAM_SSID_T rSsid;
	UINT_64 u8Timestamp;
	BOOLEAN fgIsNewBssDesc = FALSE;

	UINT_32 i;
	UINT_8 ucSSIDChar;
	UINT_8 ucPowerConstraint = 0;
	P_IE_COUNTRY_T prCountryIE = NULL;
	ENUM_BAND_T eHwBand = HAL_RX_STATUS_GET_RF_BAND(prSwRfb->prRxStatus);
	BOOLEAN fgBandMismatch = FALSE;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prWlanBeaconFrame = (P_WLAN_BEACON_FRAME_T) prSwRfb->pvHeader;

	WLAN_GET_FIELD_16(&prWlanBeaconFrame->u2CapInfo, &u2CapInfo);
	WLAN_GET_FIELD_64(&prWlanBeaconFrame->au4Timestamp[0], &u8Timestamp);

	/* decide BSS type */
	switch (u2CapInfo & CAP_INFO_BSS_TYPE) {
	case CAP_INFO_ESS:
		/* It can also be Group Owner of P2P Group. */
		eBSSType = BSS_TYPE_INFRASTRUCTURE;
		break;

	case CAP_INFO_IBSS:
		eBSSType = BSS_TYPE_IBSS;
		break;
	case 0:
		/*
		 * The P2P Device shall set the ESS bit of the Capabilities field
		 * in the Probe Response fame to 0 and IBSS bit to 0. (3.1.2.1.1)
		 */
		eBSSType = BSS_TYPE_P2P_DEVICE;
		break;

#if CFG_ENABLE_BT_OVER_WIFI
		/* @TODO: add rule to identify BOW beacons */
#endif

	default:
		return NULL;
	}

	/* 4 <1.1> Pre-parse SSID IE */
	pucIE = prWlanBeaconFrame->aucInfoElem;
	u2IELength = (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
	    (UINT_16) OFFSET_OF(WLAN_BEACON_FRAME_BODY_T, aucInfoElem[0]);

	if (u2IELength > CFG_IE_BUFFER_SIZE)
		u2IELength = CFG_IE_BUFFER_SIZE;

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_SSID:
			if (IE_LEN(pucIE) <= ELEM_MAX_LEN_SSID) {
				ucSSIDChar = '\0';

				/* D-Link DWL-900AP+ */
				if (IE_LEN(pucIE) == 0)
					fgIsValidSsid = FALSE;
				/* Cisco AP1230A - (IE_LEN(pucIE) == 1) && (SSID_IE(pucIE)->aucSSID[0] == '\0') */
				/*
				 * Linksys WRK54G/WL520g - (IE_LEN(pucIE) == n) &&
				 * (SSID_IE(pucIE)->aucSSID[0~(n-1)] == '\0')
				 */
				else {
					for (i = 0; i < IE_LEN(pucIE); i++)
						ucSSIDChar |= SSID_IE(pucIE)->aucSSID[i];

					if (ucSSIDChar)
						fgIsValidSsid = TRUE;
				}

				/* Update SSID to BSS Descriptor only if SSID is not hidden. */
				if (fgIsValidSsid == TRUE) {
					COPY_SSID(rSsid.aucSsid,
						  rSsid.u4SsidLen, SSID_IE(pucIE)->aucSSID, SSID_IE(pucIE)->ucLength);
				}
			}
			break;
		case ELEM_ID_DS_PARAM_SET:
			if (IE_LEN(pucIE) == ELEM_MAX_LEN_DS_PARAMETER_SET)
				ucIeDsChannelNum = DS_PARAM_IE(pucIE)->ucCurrChnl;
			break;
		case ELEM_ID_HT_OP:
			if (IE_LEN(pucIE) == (sizeof(IE_HT_OP_T) - 2))
				ucIeHtChannelNum = ((P_IE_HT_OP_T) pucIE)->ucPrimaryChannel;
			break;
		default:
			break;
		}
	}

	if (ucIeDsChannelNum > 0) {
		if (ucIeDsChannelNum <= 14)
			fgBandMismatch = (eHwBand != BAND_2G4);
		else if (ucIeDsChannelNum < 200)
			fgBandMismatch = (eHwBand != BAND_5G);
	} else if (ucIeHtChannelNum > 0) {
		if (ucIeHtChannelNum <= 14)
			fgBandMismatch = (eHwBand != BAND_2G4);
		else if (ucIeHtChannelNum < 200)
			fgBandMismatch = (eHwBand != BAND_5G);
	}

	if (fgBandMismatch) {
		DBGLOG(SCN, INFO, "%pM Band mismatch, HW band %d, DS chnl %d, HT chnl %d\n",
		       prWlanBeaconFrame->aucBSSID, eHwBand, ucIeDsChannelNum, ucIeHtChannelNum);
		return NULL;
	}

	/* 4 <1.2> Replace existing BSS_DESC_T or allocate a new one */
	prBssDesc = scanSearchExistingBssDescWithSsid(prAdapter,
						      eBSSType,
						      (PUINT_8) prWlanBeaconFrame->aucBSSID,
						      (PUINT_8) prWlanBeaconFrame->aucSrcAddr,
						      fgIsValidSsid, fgIsValidSsid == TRUE ? &rSsid : NULL);

	if (prBssDesc == (P_BSS_DESC_T) NULL) {
		fgIsNewBssDesc = TRUE;

		do {
			/* 4 <1.2.1> First trial of allocation */
			prBssDesc = scanAllocateBssDesc(prAdapter);
			if (prBssDesc)
				break;
			/* 4 <1.2.2> Hidden is useless, remove the oldest hidden ssid. (for passive scan) */
			scanRemoveBssDescsByPolicy(prAdapter,
						   (SCN_RM_POLICY_EXCLUDE_CONNECTED |
						    SCN_RM_POLICY_OLDEST_HIDDEN | SCN_RM_POLICY_TIMEOUT));

			/* 4 <1.2.3> Second tail of allocation */
			prBssDesc = scanAllocateBssDesc(prAdapter);
			if (prBssDesc)
				break;
			/* 4 <1.2.4> Remove the weakest one */
			/* If there are more than half of BSS which has the same ssid as connection
			 * setting, remove the weakest one from them.
			 * Else remove the weakest one.
			 */
			scanRemoveBssDescsByPolicy(prAdapter,
						   (SCN_RM_POLICY_EXCLUDE_CONNECTED | SCN_RM_POLICY_SMART_WEAKEST));

			/* 4 <1.2.5> reallocation */
			prBssDesc = scanAllocateBssDesc(prAdapter);
			if (prBssDesc)
				break;
			/* 4 <1.2.6> no space, should not happen */
			/* ASSERT(0); // still no space available ? */
			return NULL;

		} while (FALSE);

	} else {
		OS_SYSTIME rCurrentTime;

		/* WCXRP00000091 */
		/* if the received strength is much weaker than the original one, */
		/* ignore it due to it might be received on the folding frequency */

		GET_CURRENT_SYSTIME(&rCurrentTime);

		ASSERT(prSwRfb->prRxStatusGroup3);

		if (prBssDesc->eBSSType != eBSSType) {
			prBssDesc->eBSSType = eBSSType;
		} else if (HAL_RX_STATUS_GET_CHNL_NUM(prSwRfb->prRxStatus) !=
			   prBssDesc->ucChannelNum
			   && prBssDesc->ucRCPI > HAL_RX_STATUS_GET_RCPI(prSwRfb->prRxStatusGroup3)) {
			/* for signal strength is too much weaker and previous beacon is not stale */
			ASSERT(prSwRfb->prRxStatusGroup3);
			if ((prBssDesc->ucRCPI -
			     HAL_RX_STATUS_GET_RCPI(prSwRfb->prRxStatusGroup3)) >=
			    REPLICATED_BEACON_STRENGTH_THRESHOLD
			    && rCurrentTime - prBssDesc->rUpdateTime <= REPLICATED_BEACON_FRESH_PERIOD) {
			    prBssDesc->u2ReturnLine = __LINE__;
				prBssDesc->rAddBssDescTime = rCurrentTime;
				return prBssDesc;
			}
			/* for received beacons too close in time domain */
			else if (rCurrentTime - prBssDesc->rUpdateTime <= REPLICATED_BEACON_TIME_THRESHOLD) {
				prBssDesc->u2ReturnLine = __LINE__;
				prBssDesc->rAddBssDescTime = rCurrentTime;
				return prBssDesc;
			}
		}

		/* if Timestamp has been reset, re-generate BSS DESC 'cause AP should have reset itself */
		if (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE && u8Timestamp < prBssDesc->u8TimeStamp.QuadPart) {
			BOOLEAN fgIsConnected, fgIsConnecting;

			/* set flag for indicating this is a new BSS-DESC */
			fgIsNewBssDesc = TRUE;

			/* backup 2 flags for APs which reset timestamp unexpectedly */
			fgIsConnected = prBssDesc->fgIsConnected;
			fgIsConnecting = prBssDesc->fgIsConnecting;
			scanRemoveBssDescByBssid(prAdapter, prBssDesc->aucBSSID);

			prBssDesc = scanAllocateBssDesc(prAdapter);
			if (!prBssDesc)
				return NULL;

			/* restore */
			prBssDesc->fgIsConnected = fgIsConnected;
			prBssDesc->fgIsConnecting = fgIsConnecting;
		}
	}
#if 1

	prBssDesc->u2RawLength = prSwRfb->u2PacketLen;
	if (prBssDesc->u2RawLength > CFG_RAW_BUFFER_SIZE)
		prBssDesc->u2RawLength = CFG_RAW_BUFFER_SIZE;

	kalMemCopy(prBssDesc->aucRawBuf, prWlanBeaconFrame, prBssDesc->u2RawLength);
#endif

	/* NOTE: Keep consistency of Scan Record during JOIN process */
	if (fgIsNewBssDesc == FALSE && prBssDesc->fgIsConnecting) {
		prBssDesc->u2ReturnLine = __LINE__;
		return prBssDesc;
	}
	/* 4 <2> Get information from Fixed Fields */
	prBssDesc->eBSSType = eBSSType;	/* Update the latest BSS type information. */

	COPY_MAC_ADDR(prBssDesc->aucSrcAddr, prWlanBeaconFrame->aucSrcAddr);

	COPY_MAC_ADDR(prBssDesc->aucBSSID, prWlanBeaconFrame->aucBSSID);

	prBssDesc->u8TimeStamp.QuadPart = u8Timestamp;

	WLAN_GET_FIELD_16(&prWlanBeaconFrame->u2BeaconInterval, &prBssDesc->u2BeaconInterval);

	prBssDesc->u2CapInfo = u2CapInfo;

	/* 4 <2.1> Retrieve IEs for later parsing */
	u2IELength = (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
	    (UINT_16) OFFSET_OF(WLAN_BEACON_FRAME_BODY_T, aucInfoElem[0]);

	if (u2IELength > CFG_IE_BUFFER_SIZE) {
		u2IELength = CFG_IE_BUFFER_SIZE;
		prBssDesc->fgIsIEOverflow = TRUE;
	} else {
		prBssDesc->fgIsIEOverflow = FALSE;
	}
	prBssDesc->u2IELength = u2IELength;

	if (fgIsValidSsid ||
	    ((prWlanBeaconFrame->u2FrameCtrl & MASK_FRAME_TYPE) == MAC_FRAME_PROBE_RSP))
		kalMemCopy(prBssDesc->aucIEBuf, prWlanBeaconFrame->aucInfoElem, u2IELength);

	/* 4 <2.2> reset prBssDesc variables in case that AP has been reconfigured */
	prBssDesc->fgIsERPPresent = FALSE;
	prBssDesc->fgIsHTPresent = FALSE;
	prBssDesc->fgIsVHTPresent = FALSE;
	prBssDesc->eSco = CHNL_EXT_SCN;
	prBssDesc->fgIEWAPI = FALSE;
	prBssDesc->fgIERSN = FALSE;
	prBssDesc->fgIEWPA = FALSE;
	prBssDesc->eChannelWidth = CW_20_40MHZ;	/*Reset VHT OP IE relative settings */
	prBssDesc->ucCenterFreqS1 = 0;
	prBssDesc->ucCenterFreqS2 = 0;
	prBssDesc->fgExsitBssLoadIE = FALSE;
	prBssDesc->fgMultiAnttenaAndSTBC = FALSE;
	/* Probe response doesn't have TIM IE */
	if ((prWlanBeaconFrame->u2FrameCtrl & MASK_FRAME_TYPE) != MAC_FRAME_PROBE_RSP) {
		prBssDesc->fgDTIMPresent = FALSE;
		prBssDesc->ucDTIMPeriod = 0;
	}
	kalMemZero(prBssDesc->aucRrmCap, sizeof(prBssDesc->aucRrmCap));
	/* 4 <3.1> Full IE parsing on SW_RFB_T */
	pucIE = prWlanBeaconFrame->aucInfoElem;

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {

		switch (IE_ID(pucIE)) {
		case ELEM_ID_SSID:
			if ((!prIeSsid) &&	/* NOTE(Kevin): for Atheros IOT #1 */
			    (IE_LEN(pucIE) <= ELEM_MAX_LEN_SSID)) {
				BOOLEAN fgIsHiddenSSID = FALSE;

				ucSSIDChar = '\0';
				prIeSsid = (P_IE_SSID_T) pucIE;

				/* D-Link DWL-900AP+ */
				if (IE_LEN(pucIE) == 0)
					fgIsHiddenSSID = TRUE;
				/* Cisco AP1230A - (IE_LEN(pucIE) == 1) && (SSID_IE(pucIE)->aucSSID[0] == '\0') */
				/*
				 * Linksys WRK54G/WL520g - (IE_LEN(pucIE) == n) &&
				 * (SSID_IE(pucIE)->aucSSID[0~(n-1)] == '\0')
				 */
				else {
					for (i = 0; i < IE_LEN(pucIE); i++)
						ucSSIDChar |= SSID_IE(pucIE)->aucSSID[i];

					if (!ucSSIDChar)
						fgIsHiddenSSID = TRUE;
				}

				/* Update SSID to BSS Descriptor only if SSID is not hidden. */
				if (!fgIsHiddenSSID) {
					COPY_SSID(prBssDesc->aucSSID,
						  prBssDesc->ucSSIDLen,
						  SSID_IE(pucIE)->aucSSID, SSID_IE(pucIE)->ucLength);
				}

			}
			break;

		case ELEM_ID_SUP_RATES:
			/* NOTE(Kevin): Buffalo WHR-G54S's supported rate set IE exceed 8.
			 * IE_LEN(pucIE) == 12, "1(B), 2(B), 5.5(B), 6(B), 9(B), 11(B),
			 * 12(B), 18(B), 24(B), 36(B), 48(B), 54(B)"
			 */
			/* TP-LINK will set extra and incorrect ie with ELEM_ID_SUP_RATES */
			if ((!prIeSupportedRate) && (IE_LEN(pucIE) <= RATE_NUM_SW))
				prIeSupportedRate = SUP_RATES_IE(pucIE);
			break;

		case ELEM_ID_DS_PARAM_SET:
			if (IE_LEN(pucIE) == ELEM_MAX_LEN_DS_PARAMETER_SET)
				ucIeDsChannelNum = DS_PARAM_IE(pucIE)->ucCurrChnl;
			break;

		case ELEM_ID_TIM:
			if (IE_LEN(pucIE) <= ELEM_MAX_LEN_TIM) {
				prBssDesc->fgDTIMPresent = TRUE;
				prBssDesc->ucDTIMPeriod = TIM_IE(pucIE)->ucDTIMPeriod;
			}
			break;

		case ELEM_ID_IBSS_PARAM_SET:
			if (IE_LEN(pucIE) == ELEM_MAX_LEN_IBSS_PARAMETER_SET)
				prBssDesc->u2ATIMWindow = IBSS_PARAM_IE(pucIE)->u2ATIMWindow;
			break;

		case ELEM_ID_COUNTRY_INFO:
			prCountryIE = (P_IE_COUNTRY_T) pucIE;
			break;

		case ELEM_ID_ERP_INFO:
			if (IE_LEN(pucIE) == ELEM_MAX_LEN_ERP)
				prBssDesc->fgIsERPPresent = TRUE;
			break;

		case ELEM_ID_EXTENDED_SUP_RATES:
			if (!prIeExtSupportedRate)
				prIeExtSupportedRate = EXT_SUP_RATES_IE(pucIE);
			break;

		case ELEM_ID_RSN:
			if (rsnParseRsnIE(prAdapter, RSN_IE(pucIE), &prBssDesc->rRSNInfo)) {
				prBssDesc->fgIERSN = TRUE;
				prBssDesc->u2RsnCap = prBssDesc->rRSNInfo.u2RsnCap;
				if (prAdapter->rWifiVar.rConnSettings.eAuthMode == AUTH_MODE_WPA2)
					rsnCheckPmkidCache(prAdapter, prBssDesc);
			}
			break;

		case ELEM_ID_HT_CAP:
		{
			P_IE_HT_CAP_T prHtCap = (P_IE_HT_CAP_T)pucIE;
			UINT_8 ucSpatial = 0;
			UINT_8 i = 0;

			prBssDesc->fgIsHTPresent = TRUE;

			if (prBssDesc->fgMultiAnttenaAndSTBC)
				break;
			for (; i < 4; i++) {
				if (prHtCap->rSupMcsSet.aucRxMcsBitmask[i] > 0)
					ucSpatial++;
			}
			prBssDesc->fgMultiAnttenaAndSTBC =
				((ucSpatial > 1) && (prHtCap->u2HtCapInfo & HT_CAP_INFO_TX_STBC));
			break;
		}
		case ELEM_ID_HT_OP:
			if (IE_LEN(pucIE) != (sizeof(IE_HT_OP_T) - 2))
				break;

			if ((((P_IE_HT_OP_T) pucIE)->ucInfo1 & HT_OP_INFO1_SCO) != CHNL_EXT_RES) {
				prBssDesc->eSco = (ENUM_CHNL_EXT_T)
				    (((P_IE_HT_OP_T) pucIE)->ucInfo1 & HT_OP_INFO1_SCO);
			}
			ucIeHtChannelNum = ((P_IE_HT_OP_T) pucIE)->ucPrimaryChannel;

			break;
		case ELEM_ID_VHT_CAP:
		{
			P_IE_VHT_CAP_T prVhtCap = (P_IE_VHT_CAP_T)pucIE;
			UINT_16 u2TxMcsSet = prVhtCap->rVhtSupportedMcsSet.u2TxMcsMap;
			UINT_8 ucSpatial = 0;
			UINT_8 i = 0;

			prBssDesc->fgIsVHTPresent = TRUE;

			if (prBssDesc->fgMultiAnttenaAndSTBC)
				break;
			for (; i < 8; i++) {
				if ((u2TxMcsSet & BITS(2*i, 2*i+1)) != 3)
					ucSpatial++;
			}
			prBssDesc->fgMultiAnttenaAndSTBC =
				((ucSpatial > 1) && (prVhtCap->u4VhtCapInfo & VHT_CAP_INFO_TX_STBC));
			break;
		}
		case ELEM_ID_VHT_OP:
			if (IE_LEN(pucIE) != (sizeof(IE_VHT_OP_T) - 2))
				break;

			prBssDesc->eChannelWidth = (ENUM_CHANNEL_WIDTH_T) (((P_IE_VHT_OP_T) pucIE)->ucVhtOperation[0]);
			prBssDesc->ucCenterFreqS1 = (ENUM_CHANNEL_WIDTH_T) (((P_IE_VHT_OP_T) pucIE)->ucVhtOperation[1]);
			prBssDesc->ucCenterFreqS2 = (ENUM_CHANNEL_WIDTH_T) (((P_IE_VHT_OP_T) pucIE)->ucVhtOperation[2]);

			break;
#if CFG_SUPPORT_WAPI
		case ELEM_ID_WAPI:
			if (wapiParseWapiIE(WAPI_IE(pucIE), &prBssDesc->rIEWAPI))
				prBssDesc->fgIEWAPI = TRUE;
			break;
#endif

		case ELEM_ID_BSS_LOAD:
		{
			struct IE_BSS_LOAD *prBssLoad = (struct IE_BSS_LOAD *)pucIE;

			prBssDesc->u2StaCnt = prBssLoad->u2StaCnt;
			prBssDesc->ucChnlUtilization = prBssLoad->ucChnlUtilizaion;
			prBssDesc->u2AvaliableAC = prBssLoad->u2AvailabeAC;
			prBssDesc->fgExsitBssLoadIE = TRUE;
			break;
		}
		case ELEM_ID_VENDOR:	/* ELEM_ID_P2P, ELEM_ID_WMM */
			{
				UINT_8 ucOuiType;
				UINT_16 u2SubTypeVersion;

				if (rsnParseCheckForWFAInfoElem(prAdapter, pucIE, &ucOuiType, &u2SubTypeVersion)) {
					if ((ucOuiType == VENDOR_OUI_TYPE_WPA)
					    && (u2SubTypeVersion == VERSION_WPA)
					    && (rsnParseWpaIE(prAdapter, WPA_IE(pucIE), &prBssDesc->rWPAInfo))) {
						prBssDesc->fgIEWPA = TRUE;
					}
				}
#if CFG_ENABLE_WIFI_DIRECT
				if (prAdapter->fgIsP2PRegistered) {
					if ((p2pFuncParseCheckForP2PInfoElem(prAdapter, pucIE, &ucOuiType))
					    && (ucOuiType == VENDOR_OUI_TYPE_P2P)) {
						prBssDesc->fgIsP2PPresent = TRUE;
					}
				}
#endif /* CFG_ENABLE_WIFI_DIRECT */
			}
			break;
		case ELEM_ID_PWR_CONSTRAINT:
		{
			P_IE_POWER_CONSTRAINT_T prPwrConstraint = (P_IE_POWER_CONSTRAINT_T)pucIE;

			if (IE_LEN(pucIE) != 1)
				break;
			ucPowerConstraint = prPwrConstraint->ucLocalPowerConstraint;
			break;
		}
		case ELEM_ID_RRM_ENABLED_CAP:
			/* RRM Capability IE is always in length 5 bytes */
			kalMemCopy(prBssDesc->aucRrmCap, pucIE + 2, 5);
			break;
			/* no default */
		}
	}

	/* 4 <3.2> Save information from IEs - SSID */
	/* Update Flag of Hidden SSID for used in SEARCH STATE. */

	/* NOTE(Kevin): in current driver, the ucSSIDLen == 0 represent
	 * all cases of hidden SSID.
	 * If the fgIsHiddenSSID == TRUE, it means we didn't get the ProbeResp with
	 * valid SSID.
	 */
	if (prBssDesc->ucSSIDLen == 0)
		prBssDesc->fgIsHiddenSSID = TRUE;
	else
		prBssDesc->fgIsHiddenSSID = FALSE;

	/* 4 <3.3> Check rate information in related IEs. */
	if (prIeSupportedRate || prIeExtSupportedRate) {
		rateGetRateSetFromIEs(prIeSupportedRate,
				      prIeExtSupportedRate,
				      &prBssDesc->u2OperationalRateSet,
				      &prBssDesc->u2BSSBasicRateSet, &prBssDesc->fgIsUnknownBssBasicRate);
	}

	/* 4 <4> Update information from HIF RX Header */
	{
		P_HW_MAC_RX_DESC_T prRxStatus;
		UINT_8 ucRxRCPI;

		prRxStatus = prSwRfb->prRxStatus;
		ASSERT(prRxStatus);

		/* 4 <4.1> Get TSF comparison result */
		prBssDesc->fgIsLargerTSF = HAL_RX_STATUS_GET_TCL(prRxStatus);

		/* 4 <4.2> Get Band information */
		prBssDesc->eBand = eHwBand;

		/* 4 <4.2> Get channel and RCPI information */
		ucHwChannelNum = HAL_RX_STATUS_GET_CHNL_NUM(prRxStatus);

		ASSERT(prSwRfb->prRxStatusGroup3);
		ucRxRCPI = (UINT_8) HAL_RX_STATUS_GET_RCPI(prSwRfb->prRxStatusGroup3);
		if (prBssDesc->eBand == BAND_2G4) {

			/* Update RCPI if in right channel */

			if (ucIeDsChannelNum >= 1 && ucIeDsChannelNum <= 14) {

				/* Receive Beacon/ProbeResp frame from adjacent channel. */
				if ((ucIeDsChannelNum == ucHwChannelNum) || (ucRxRCPI > prBssDesc->ucRCPI))
					prBssDesc->ucRCPI = ucRxRCPI;
				/* trust channel information brought by IE */
				prBssDesc->ucChannelNum = ucIeDsChannelNum;
			} else if (ucIeHtChannelNum >= 1 && ucIeHtChannelNum <= 14) {
				/* Receive Beacon/ProbeResp frame from adjacent channel. */
				if ((ucIeHtChannelNum == ucHwChannelNum) || (ucRxRCPI > prBssDesc->ucRCPI))
					prBssDesc->ucRCPI = ucRxRCPI;
				/* trust channel information brought by IE */
				prBssDesc->ucChannelNum = ucIeHtChannelNum;
			} else {
				prBssDesc->ucRCPI = ucRxRCPI;

				prBssDesc->ucChannelNum = ucHwChannelNum;
			}
		}
		/* 5G Band */
		else {
			if (ucIeHtChannelNum >= 1 && ucIeHtChannelNum < 200) {
				/* Receive Beacon/ProbeResp frame from adjacent channel. */
				if ((ucIeHtChannelNum == ucHwChannelNum) || (ucRxRCPI > prBssDesc->ucRCPI))
					prBssDesc->ucRCPI = ucRxRCPI;
				/* trust channel information brought by IE */
				prBssDesc->ucChannelNum = ucIeHtChannelNum;
			} else {
				/* Always update RCPI */
				prBssDesc->ucRCPI = ucRxRCPI;

				prBssDesc->ucChannelNum = ucHwChannelNum;
			}
		}
	}

	/* 4 <5> Check IE information corret or not */
	if (!rlmDomainIsValidRfSetting(prAdapter, prBssDesc->eBand, prBssDesc->ucChannelNum, prBssDesc->eSco,
				       prBssDesc->eChannelWidth, prBssDesc->ucCenterFreqS1,
				       prBssDesc->ucCenterFreqS2)) {
		/*Dump IE Inforamtion */
		PUINT_8 pucDumpIE;

		pucDumpIE = (PUINT_8) ((ULONG) pucIE - u2IELength);
		DBGLOG(RLM, WARN, "ScanAddToBssDesc IE Information\n");
		DBGLOG(RLM, WARN, "IE Length = %d\n", u2IELength);
		DBGLOG_MEM8(RLM, WARN, pucDumpIE, u2IELength);

		/* Error Handling for Non-predicted IE - Fixed to set 20MHz */
		prBssDesc->eChannelWidth = CW_20_40MHZ;
		prBssDesc->ucCenterFreqS1 = 0;
		prBssDesc->ucCenterFreqS2 = 0;
		prBssDesc->eSco = CHNL_EXT_SCN;
	}
#if CFG_SUPPORT_802_11K
	if (prCountryIE) {
		UINT_8 ucRemainLen = (prCountryIE->ucLength > 3)?(prCountryIE->ucLength - 3):0;
		P_COUNTRY_INFO_SUBBAND_TRIPLET_T prSubBand = &prCountryIE->arCountryStr[0];
		const UINT_8 ucSubBandSize = (UINT_8)sizeof(COUNTRY_INFO_SUBBAND_TRIPLET_T);
		INT_8 cNewPwrLimit = RLM_INVALID_POWER_LIMIT;

		/* Try to find a country subband base on our channel */
		while (ucRemainLen >= ucSubBandSize) {
			if (prSubBand->ucFirstChnlNum < 201 &&
				prBssDesc->ucChannelNum >= prSubBand->ucFirstChnlNum &&
				prBssDesc->ucChannelNum <= (prSubBand->ucFirstChnlNum + prSubBand->ucNumOfChnl - 1))
				break;
			ucRemainLen -= ucSubBandSize;
			prSubBand++;
		}
		/* Found a right country band */
		if (ucRemainLen >= ucSubBandSize) {
			cNewPwrLimit = prSubBand->cMaxTxPwrLv - ucPowerConstraint;
			/* Limit Tx power changed */
			if (prBssDesc->cPowerLimit != cNewPwrLimit) {
				prBssDesc->cPowerLimit = cNewPwrLimit;
				DBGLOG(SCN, TRACE, "Old: TxPwrLimit %d,New: CountryMax %d, Constraint %d\n",
					prBssDesc->cPowerLimit, prSubBand->cMaxTxPwrLv, ucPowerConstraint);
				/* should tell firmware to restrict tx power if connected a BSS */
				if (prBssDesc->fgIsConnected) {
					if (prBssDesc->cPowerLimit != RLM_INVALID_POWER_LIMIT)
						rlmSetMaxTxPwrLimit(prAdapter, prBssDesc->cPowerLimit, 1);
					else
						rlmSetMaxTxPwrLimit(prAdapter, 0, 0);
				}
			}
		} else if (prBssDesc->cPowerLimit != RLM_INVALID_POWER_LIMIT) {
			prBssDesc->cPowerLimit = RLM_INVALID_POWER_LIMIT;
			rlmSetMaxTxPwrLimit(prAdapter, 0, 0);
		}
	} else if (prBssDesc->cPowerLimit != RLM_INVALID_POWER_LIMIT) {
		prBssDesc->cPowerLimit = RLM_INVALID_POWER_LIMIT;
		rlmSetMaxTxPwrLimit(prAdapter, 0, 0);
	}
#endif

	/* 4 <6> PHY type setting */
	prBssDesc->ucPhyTypeSet = 0;

	if (prBssDesc->eBand == BAND_2G4) {
		/* check if support 11n */
		if (prBssDesc->fgIsHTPresent)
			prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_HT;

		/* if not 11n only */
		if (!(prBssDesc->u2BSSBasicRateSet & RATE_SET_BIT_HT_PHY)) {
			/* check if support 11g */
			if ((prBssDesc->u2OperationalRateSet & RATE_SET_OFDM) || prBssDesc->fgIsERPPresent)
				prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_ERP;

			/* if not 11g only */
			if (!(prBssDesc->u2BSSBasicRateSet & RATE_SET_OFDM)) {
				/* check if support 11b */
				if ((prBssDesc->u2OperationalRateSet & RATE_SET_HR_DSSS))
					prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_HR_DSSS;
			}
		}
	} else {		/* (prBssDesc->eBande == BAND_5G) */
		/* check if support 11n */
		if (prBssDesc->fgIsVHTPresent)
			prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;

		if (prBssDesc->fgIsHTPresent)
			prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_HT;

		/* if not 11n only */
		if (!(prBssDesc->u2BSSBasicRateSet & RATE_SET_BIT_HT_PHY)) {
			/* Support 11a definitely */
			prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_OFDM;

			/* ASSERT(!(prBssDesc->u2OperationalRateSet & RATE_SET_HR_DSSS)); */
		}
	}
	/* check if it is a probe response frame */
	if ((prWlanBeaconFrame->u2FrameCtrl & 0x50) == 0x50)
		prBssDesc->fgSeenProbeResp = TRUE;
	/* 4 <7> Update BSS_DESC_T's Last Update TimeStamp. */
	if (fgIsValidSsid ||
	    ((prWlanBeaconFrame->u2FrameCtrl & MASK_FRAME_TYPE) == MAC_FRAME_PROBE_RSP))
		GET_CURRENT_SYSTIME(&prBssDesc->rUpdateTime);

	if (prBssDesc->fgIsConnected) {
		rTsf.rTime = prBssDesc->rUpdateTime;
		kalMemCopy(&rTsf.au4Tsf[0], &prBssDesc->u8TimeStamp, 8);
	}
	prBssDesc->u2ReturnLine = __LINE__;
	return prBssDesc;
}

/* clear all ESS scan result */
VOID scanInitEssResult(P_ADAPTER_T prAdapter)
{
	prAdapter->rWlanInfo.u4ScanResultEssNum = 0;
	kalMemZero(prAdapter->rWlanInfo.arScanResultEss, sizeof(prAdapter->rWlanInfo.arScanResultEss));
}
/*
 * print all ESS into log system once scan done
 * it is useful to log that, otherwise, we have no information to identify if hardware has seen a specific AP,
 * if user complained some AP were not found in scan result list
 */
VOID scanLogEssResult(P_ADAPTER_T prAdapter)
{
#define NUMBER_SSID_PER_LINE 16
	struct ESS_SCAN_RESULT_T *prEssResult = &prAdapter->rWlanInfo.arScanResultEss[0];
	UINT_32 u4ResultNum = prAdapter->rWlanInfo.u4ScanResultEssNum;
	UINT_32 u4Index = 0;

	if (u4ResultNum == 0) {
		DBGLOG(SCN, INFO, "0 Bss is found, %d, %d, %d, %d\n",
			prAdapter->rWlanInfo.u4ScanDbgTimes1, prAdapter->rWlanInfo.u4ScanDbgTimes2,
			prAdapter->rWlanInfo.u4ScanDbgTimes3, prAdapter->rWlanInfo.u4ScanDbgTimes4);
		return;
	}

	DBGLOG(SCN, INFO,
		"Total:%u; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s\n",
		u4ResultNum,
		prEssResult[0].aucSSID, prEssResult[1].aucSSID, prEssResult[2].aucSSID,
		prEssResult[3].aucSSID, prEssResult[4].aucSSID, prEssResult[5].aucSSID,
		prEssResult[6].aucSSID, prEssResult[7].aucSSID, prEssResult[8].aucSSID,
		prEssResult[9].aucSSID, prEssResult[10].aucSSID, prEssResult[11].aucSSID,
		prEssResult[12].aucSSID, prEssResult[13].aucSSID, prEssResult[14].aucSSID,
		prEssResult[15].aucSSID);
	if (u4ResultNum <= NUMBER_SSID_PER_LINE)
		return;
	u4ResultNum -= NUMBER_SSID_PER_LINE;
	prEssResult = &prEssResult[NUMBER_SSID_PER_LINE];
	u4ResultNum = u4ResultNum / NUMBER_SSID_PER_LINE;
	if ((u4ResultNum % NUMBER_SSID_PER_LINE) != 0)
		u4ResultNum++;
	for (; u4Index < u4ResultNum; u4Index++) {
		struct ESS_SCAN_RESULT_T *prEss = &prEssResult[NUMBER_SSID_PER_LINE*u4Index];

		DBGLOG(SCN, INFO,
			"%s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s\n",
			prEss[0].aucSSID,
			prEss[1].aucSSID, prEss[2].aucSSID, prEss[3].aucSSID,
			prEss[4].aucSSID, prEss[5].aucSSID, prEss[6].aucSSID,
			prEss[7].aucSSID, prEss[8].aucSSID, prEss[9].aucSSID,
			prEss[10].aucSSID, prEss[11].aucSSID, prEss[12].aucSSID,
			prEss[13].aucSSID, prEss[14].aucSSID, prEss[15].aucSSID);
	}
}

/*
 * record all Scanned ESS, only one BSS was saved for each ESS, and AP who is hidden ssid was excluded.
 * maximum we only support record 64 ESSes
 */
static VOID scanAddEssResult(P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBssDesc)
{
	struct ESS_SCAN_RESULT_T *prEssResult = &prAdapter->rWlanInfo.arScanResultEss[0];
	UINT_32 u4Index = 0;

	if (prBssDesc->fgIsHiddenSSID)
		return;
	if (prAdapter->rWlanInfo.u4ScanResultEssNum >= CFG_MAX_NUM_BSS_LIST)
		return;
	for (; u4Index < prAdapter->rWlanInfo.u4ScanResultEssNum; u4Index++) {
		if (EQUAL_SSID(prEssResult[u4Index].aucSSID, (UINT_8)prEssResult[u4Index].u2SSIDLen,
			prBssDesc->aucSSID, prBssDesc->ucSSIDLen))
			return;
	}

	COPY_SSID(prEssResult[u4Index].aucSSID, prEssResult[u4Index].u2SSIDLen,
		prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
	COPY_MAC_ADDR(prEssResult[u4Index].aucBSSID, prBssDesc->aucBSSID);
	prAdapter->rWlanInfo.u4ScanResultEssNum++;
}
/*----------------------------------------------------------------------------*/
/*!
* @brief Convert the Beacon or ProbeResp Frame in SW_RFB_T to scan result for query
*
* @param[in] prSwRfb            Pointer to the receiving SW_RFB_T structure.
*
* @retval WLAN_STATUS_SUCCESS   It is a valid Scan Result and been sent to the host.
* @retval WLAN_STATUS_FAILURE   It is not a valid Scan Result.
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS scanAddScanResult(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBssDesc, IN P_SW_RFB_T prSwRfb)
{
	P_SCAN_INFO_T prScanInfo;
	UINT_8 aucRatesEx[PARAM_MAX_LEN_RATES_EX];
	P_WLAN_BEACON_FRAME_T prWlanBeaconFrame;
	PARAM_MAC_ADDRESS rMacAddr;
	PARAM_SSID_T rSsid;
	ENUM_PARAM_NETWORK_TYPE_T eNetworkType;
	PARAM_802_11_CONFIG_T rConfiguration;
	ENUM_PARAM_OP_MODE_T eOpMode;
	UINT_8 ucRateLen = 0;
	UINT_32 i;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (prBssDesc->eBand == BAND_2G4) {
		if ((prBssDesc->u2OperationalRateSet & RATE_SET_OFDM)
		    || prBssDesc->fgIsERPPresent) {
			eNetworkType = PARAM_NETWORK_TYPE_OFDM24;
		} else {
			eNetworkType = PARAM_NETWORK_TYPE_DS;
		}
	} else {
		ASSERT(prBssDesc->eBand == BAND_5G);
		eNetworkType = PARAM_NETWORK_TYPE_OFDM5;
	}

	if (prBssDesc->eBSSType == BSS_TYPE_P2P_DEVICE) {
		/* NOTE(Kevin): Not supported by WZC(TBD) */
		DBGLOG(SCN, INFO, "Bss Desc type is P2P\n");
		return WLAN_STATUS_FAILURE;
	}

	prWlanBeaconFrame = (P_WLAN_BEACON_FRAME_T) prSwRfb->pvHeader;
	COPY_MAC_ADDR(rMacAddr, prWlanBeaconFrame->aucBSSID);
	COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen, prBssDesc->aucSSID, prBssDesc->ucSSIDLen);

	rConfiguration.u4Length = sizeof(PARAM_802_11_CONFIG_T);
	rConfiguration.u4BeaconPeriod = (UINT_32) prWlanBeaconFrame->u2BeaconInterval;
	rConfiguration.u4ATIMWindow = prBssDesc->u2ATIMWindow;
	rConfiguration.u4DSConfig = nicChannelNum2Freq(prBssDesc->ucChannelNum);
	rConfiguration.rFHConfig.u4Length = sizeof(PARAM_802_11_CONFIG_FH_T);

	rateGetDataRatesFromRateSet(prBssDesc->u2OperationalRateSet, 0, aucRatesEx, &ucRateLen);

	/* NOTE(Kevin): Set unused entries, if any, at the end of the array to 0.
	 * from OID_802_11_BSSID_LIST
	 */
	for (i = ucRateLen; i < ARRAY_SIZE(aucRatesEx); i++)
		aucRatesEx[i] = 0;

	switch (prBssDesc->eBSSType) {
	case BSS_TYPE_IBSS:
		eOpMode = NET_TYPE_IBSS;
		break;

	case BSS_TYPE_INFRASTRUCTURE:
	case BSS_TYPE_P2P_DEVICE:
	case BSS_TYPE_BOW_DEVICE:
	default:
		eOpMode = NET_TYPE_INFRA;
		break;
	}

	DBGLOG(SCN, TRACE, "ind %s %d %d\n", prBssDesc->aucSSID, prBssDesc->ucChannelNum, prBssDesc->ucRCPI);

	if (prAdapter->rWifiVar.rScanInfo.fgNloScanning &&
				test_bit(SUSPEND_FLAG_CLEAR_WHEN_RESUME, &prAdapter->ulSuspendFlag)) {
		UINT_8 i = 0;
		P_BSS_DESC_T *pprPendBssDesc = &prScanInfo->rNloParam.aprPendingBssDescToInd[0];

		for (; i < SCN_SSID_MATCH_MAX_NUM; i++) {
			if (pprPendBssDesc[i])
				continue;
			DBGLOG(SCN, INFO,
				"indicate bss[%pM] before wiphy resume, need to indicate again after wiphy resume\n",
				prBssDesc->aucBSSID);
			pprPendBssDesc[i] = prBssDesc;
			break;
		}
	}
	scanAddEssResult(prAdapter, prBssDesc);

	kalIndicateBssInfo(prAdapter->prGlueInfo,
		   (PUINT_8) prSwRfb->pvHeader,
		   prSwRfb->u2PacketLen, prBssDesc->ucChannelNum, RCPI_TO_dBm(prBssDesc->ucRCPI));
	prBssDesc->fgRptCfg80211 = TRUE;
	nicAddScanResult(prAdapter,
			 rMacAddr,
			 &rSsid,
			 prWlanBeaconFrame->u2CapInfo,
			 RCPI_TO_dBm(prBssDesc->ucRCPI),
			 eNetworkType,
			 &rConfiguration,
			 eOpMode,
			 aucRatesEx,
			 prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen,
			 (PUINT_8) ((ULONG) (prSwRfb->pvHeader) + WLAN_MAC_MGMT_HEADER_LEN));

	return WLAN_STATUS_SUCCESS;

}				/* end of scanAddScanResult() */

BOOLEAN scanCheckBssIsLegal(IN P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
	BOOLEAN fgAddToScanResult = FALSE;
	ENUM_BAND_T eBand;
	UINT_8 ucChannel;

	ASSERT(prAdapter);
	/* check the channel is in the legal doamin */
	if (rlmDomainIsLegalChannel(prAdapter, prBssDesc->eBand, prBssDesc->ucChannelNum) == TRUE) {
		/* check ucChannelNum/eBand for adjacement channel filtering */
		if (cnmAisInfraChannelFixed(prAdapter, &eBand, &ucChannel) == TRUE &&
		    (eBand != prBssDesc->eBand || ucChannel != prBssDesc->ucChannelNum)) {
			fgAddToScanResult = FALSE;
		} else {
			fgAddToScanResult = TRUE;
		}
	}

	return fgAddToScanResult;

}

static UINT_8 scanGetChannel(P_HW_MAC_RX_DESC_T prRxStatus, PUINT_8 pucIE, UINT_16 u2IELen)
{
	UINT_8 ucDsChannel = 0;
	UINT_8 ucHtChannel = 0;
	UINT_8 ucHwChannel = HAL_RX_STATUS_GET_CHNL_NUM(prRxStatus);
	UINT_16 u2Offset = 0;
	ENUM_BAND_T eBand = HAL_RX_STATUS_GET_RF_BAND(prRxStatus);

	IE_FOR_EACH(pucIE, u2IELen, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_DS_PARAM_SET:
			if (IE_LEN(pucIE) == ELEM_MAX_LEN_DS_PARAMETER_SET)
				ucDsChannel = DS_PARAM_IE(pucIE)->ucCurrChnl;
			break;
		case ELEM_ID_HT_OP:
			if (IE_LEN(pucIE) == (sizeof(IE_HT_OP_T) - 2))
				ucHtChannel = ((P_IE_HT_OP_T) pucIE)->ucPrimaryChannel;
			break;
		}
	}
	DBGLOG(SCN, INFO, "band %d, hw channel %d, ds %d, ht %d\n", eBand, ucHwChannel, ucDsChannel, ucHtChannel);
	if (eBand == BAND_2G4) {
		if (ucDsChannel >= 1 && ucDsChannel <= 14)
			return ucDsChannel;
		return (ucHtChannel >= 1 && ucHtChannel <= 14) ? ucHtChannel:ucHwChannel;
	}
	return (ucHtChannel >= 1 && ucHtChannel < 200) ? ucHtChannel:ucHwChannel;
}
/*----------------------------------------------------------------------------*/
/*!
* @brief Parse channel number to array index.
*
* @param[in] u4ChannelNum            channel number.
*
* @retval index           array index
*/
/*----------------------------------------------------------------------------*/
UINT_8 nicChannelNum2Index(IN UINT_8 ucChannelNum)
{
	UINT_8 ucindex;

	/*Full2Partial*/
	if (ucChannelNum >= 1 && ucChannelNum <= 14)
		/*1---14*/
		ucindex = ucChannelNum;
	else if (ucChannelNum >= 36 && ucChannelNum <= 64)
		/*15---22*/
		ucindex = 6 + (ucChannelNum >> 2);
	else if (ucChannelNum >= 100 && ucChannelNum <= 144)
		/*23---34*/
		ucindex = (ucChannelNum >> 2) - 2;
	else if (ucChannelNum >= 149 && ucChannelNum <= 165) {
		/*35---39*/
		ucChannelNum = ucChannelNum - 1;
		ucindex = (ucChannelNum >> 2) - 2;
	} else
		ucindex = 0;

	return ucindex;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Parse the content of given Beacon or ProbeResp Frame.
*
* @param[in] prSwRfb            Pointer to the receiving SW_RFB_T structure.
*
* @retval WLAN_STATUS_SUCCESS           if not report this SW_RFB_T to host
* @retval WLAN_STATUS_PENDING           if report this SW_RFB_T to host as scan result
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS scanProcessBeaconAndProbeResp(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
	struct RADIO_MEASUREMENT_REQ_PARAMS *prRmReq = &prAdapter->rWifiVar.rRmReqParams;
	PUCHAR cp;
	/* if beacon request measurement is on-going,  collect Beacon Report */
	if (prRmReq->rBcnRmParam.eState == RM_ON_GOING) {
		struct RM_BEACON_REPORT_PARAMS rRepParams;
		P_WLAN_BEACON_FRAME_T prWlanBeacon = (P_WLAN_BEACON_FRAME_T)prSwRfb->pvHeader;
		UINT_16 u2IELen = prSwRfb->u2PacketLen - OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem);

		kalMemZero(&rRepParams, sizeof(rRepParams));
		if (u2IELen > CFG_IE_BUFFER_SIZE)
			u2IELen = CFG_IE_BUFFER_SIZE;

		/* if this is a one antenna only device, the antenna id is always 1. 7.3.2.40 */
		rRepParams.ucAntennaID = 1;
		rRepParams.ucChannel = scanGetChannel(prSwRfb->prRxStatus, prWlanBeacon->aucInfoElem, u2IELen);
		rRepParams.ucRCPI = (UINT_8)HAL_RX_STATUS_GET_RCPI(prSwRfb->prRxStatusGroup3);
		rRepParams.ucRSNI = 255; /* 255 means RSNI not available. see 7.3.2.41 */
		rRepParams.ucFrameInfo = 0;
		DBGLOG_MEM8(SW4, INFO, (PUINT_8) prWlanBeacon, OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem));
		cp = (PUCHAR)prWlanBeacon + OFFSET_OF(WLAN_BEACON_FRAME_T, au4Timestamp[0]);
		kalMemCopy(&rRepParams.aucBcnFixedField, cp,
			sizeof(rRepParams.aucBcnFixedField)); /*all 12 bytes fixedfield are copied*/
		scanCollectBeaconReport(prAdapter, prWlanBeacon->aucInfoElem, u2IELen,
			prWlanBeacon->aucBSSID, &rRepParams);
		return WLAN_STATUS_SUCCESS;
	}
	return __scanProcessBeaconAndProbeResp(prAdapter, prSwRfb);
}
#if 0 /* 802.11k potential code */
static BOOLEAN
	scanBeaconReportCheckChannel(P_RM_BCN_REQ_T prBcnReq, UINT_8 ucBcnReqLen, UINT_8 ucChannel)
{
	PUINT_8 pucSubIE = &prBcnReq->aucSubElements[0];

	if (prBcnReq->ucChannel == ucChannel)
		return TRUE;
	ucBcnReqLen -= OFFSET_OF(RM_BCN_REQ_T, aucSubElements);
	while (ucBcnReqLen > 0) {
		UINT_8 ucNumChannels = 0;
		UINT_16 u2IeSize = IE_SIZE(pucSubIE);

		if (pucSubIE[0] != ELEM_ID_AP_CHANNEL_REPORT) {
			ucBcnReqLen -= u2IeSize;
			pucSubIE += u2IeSize;
			continue;
		}
		ucNumChannels = u2IeSize - 1;
		while (ucNumChannels >= 3) {
			if (ucChannel == pucSubIE[ucNumChannels])
				return TRUE;
		}
		ucBcnReqLen -= u2IeSize;
		pucSubIE += u2IeSize;
	}
	return FALSE;
}
#endif

VOID scanCollectBeaconReport(IN P_ADAPTER_T prAdapter, PUINT_8 pucIEBuf,
	UINT_16 u2IELength, PUINT_8 pucBssid, struct RM_BEACON_REPORT_PARAMS *prRepParams)
{
#define BEACON_FIXED_FIELD_LENGTH 12
	struct RADIO_MEASUREMENT_REQ_PARAMS *prRmReq = &prAdapter->rWifiVar.rRmReqParams;
	struct RADIO_MEASUREMENT_REPORT_PARAMS *prRmRep = &prAdapter->rWifiVar.rRmRepParams;
	P_RM_BCN_REQ_T prBcnReq = (P_RM_BCN_REQ_T)&prRmReq->prCurrMeasElem->aucRequestFields[0];
	P_IE_MEASUREMENT_REPORT_T prMeasReport = NULL;
	/* Variables to process Beacon IE */
	PUINT_8 pucIE;
	UINT_16 u2Offset = 0;

	/* Variables to collect report */
	UINT_8 ucRSSI = 0;
	PUINT_8 pucSubIE = NULL;
	UINT_8 ucCondition = 0;
	UINT_8 ucRefValue = 0;
	UINT_8 ucReportDetail = 0;
	PUINT_8 pucReportIeIds = NULL;
	UINT_8 ucReportIeIdsLen = 0;
	struct RM_BCN_REPORT *prBcnReport = NULL;
	UINT_8 ucBcnReportLen = 0;
	struct RM_MEASURE_REPORT_ENTRY *prReportEntry = NULL;
	UINT_16 u2RemainLen = 0;
	BOOLEAN fgValidChannel = FALSE;
	UINT_16 u2IeSize = 0;

	if (!EQUAL_MAC_ADDR(prBcnReq->aucBssid, "\xff\xff\xff\xff\xff\xff") &&
		!EQUAL_MAC_ADDR(prBcnReq->aucBssid, pucBssid)) {
		DBGLOG(SCN, INFO, "bssid mismatch, req %pM, actual %pM\n", prBcnReq->aucBssid, pucBssid);
		return;
	}

	pucIE = pucIEBuf;
	/* Step1: parsing Beacon Request sub element field to get Report controlling information */
	/* if match the channel that is in fixed field, no need to check AP channel report */
	if (prBcnReq->ucChannel == prRepParams->ucChannel)
		fgValidChannel = TRUE;

	u2RemainLen = prRmReq->prCurrMeasElem->ucLength - 3 - OFFSET_OF(RM_BCN_REQ_T, aucSubElements);
	pucSubIE = &prBcnReq->aucSubElements[0];
	while (u2RemainLen > 0) {
		u2IeSize = IE_SIZE(pucSubIE);
		if (u2IeSize > u2RemainLen)
			break;
		switch (pucSubIE[0]) {
		case 0: /* checking if SSID is matched */
			/* length of sub-element ssid is 0 or first byte is 0, means wildcard ssid matching */
			if (!IE_LEN(pucSubIE) || !pucSubIE[2])
				break;
			IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
				if (IE_ID(pucIE) == ELEM_ID_SSID)
					break;
			}
			if (EQUAL_SSID(&pucIE[2], pucIE[1], &pucSubIE[2], pucSubIE[1]))
				break;
			DBGLOG(SCN, TRACE,
				"SSID mismatch, req(len %d, %s), bcn(id %d, len %d, %s)\n",
				pucSubIE[1], &pucSubIE[2], pucIE[0], pucIE[1], &pucIE[2]);
			return; /* don't match SSID, don't report it */
		case 1: /* Beacon Reporting Information */
			ucCondition = pucSubIE[3];
			ucRefValue = pucSubIE[4];
			break;
		case 2: /* Reporting Detail Element */
			ucReportDetail = pucSubIE[2];
			break;
		case 10: /* Request Elements */
		{
			struct IE_REQUEST_T *prIe = (struct IE_REQUEST_T *)pucSubIE;

			pucReportIeIds = prIe->aucReqIds;
			ucReportIeIdsLen = prIe->ucLength;
			break;
		}
		case 51: /* AP CHANNEL REPORT Element */
		{
			UINT_8 ucNumChannels = 3; /* channel info is starting with the fourth byte */

			if (fgValidChannel)
				break;
			/* try to match with AP channel report */
			while (ucNumChannels < u2IeSize) {
				if (prRepParams->ucChannel == pucSubIE[ucNumChannels]) {
					fgValidChannel = TRUE;
					break;
				}
				ucNumChannels++;
			}
		}
		}
		u2RemainLen -= u2IeSize;
		pucSubIE += u2IeSize;
	}
	if (!fgValidChannel && prBcnReq->ucChannel > 0 && prBcnReq->ucChannel < 255) {
		DBGLOG(SCN, INFO, "channel %d, valid %d\n", prBcnReq->ucChannel, fgValidChannel);
		return;
	}

	/* Step2: check report condition */
	ucRSSI = RCPI_TO_dBm(prRepParams->ucRCPI);
	switch (ucCondition) {
	case 1:
		if (ucRSSI <= ucRefValue/2)
			return;
		break;
	case 2:
		if (ucRSSI >= ucRefValue/2)
			return;
		break;
	case 3:
		break;
	}
	/* Step3: Compose Beacon Report in a temp buffer */
	/* search in saved reported link, check if we have saved a report for this AP */
	LINK_FOR_EACH_ENTRY(prReportEntry, &prRmRep->rReportLink, rLinkEntry, struct RM_MEASURE_REPORT_ENTRY) {
		P_IE_MEASUREMENT_REPORT_T prReportElem = (P_IE_MEASUREMENT_REPORT_T)prReportEntry->aucMeasReport;

		prBcnReport = (struct RM_BCN_REPORT *)prReportElem->aucReportFields;
		if (EQUAL_MAC_ADDR(prBcnReport->aucBSSID, pucBssid))
			break;
		prBcnReport = NULL;
	}
	if (!prBcnReport) {/* not found a entry in collected report link */
		LINK_REMOVE_HEAD(&prRmRep->rFreeReportLink, prReportEntry, struct RM_MEASURE_REPORT_ENTRY *);
		if (!prReportEntry) {/* not found a entry in free report link */
			prReportEntry = kalMemAlloc(sizeof(*prReportEntry), VIR_MEM_TYPE);
			if (!prReportEntry)/* no memory to allocate in OS */ {
				DBGLOG(SCN, ERROR, "Alloc Measurement Report Entry failed, No Memory\n");
				return;
			}
		}
		DBGLOG(SCN, INFO, "allocate entry for Bss %pM, total entry %u\n",
			pucBssid, prRmRep->rReportLink.u4NumElem);
		LINK_INSERT_TAIL(&prRmRep->rReportLink, &prReportEntry->rLinkEntry);
	}
	kalMemZero(prReportEntry->aucMeasReport, sizeof(prReportEntry->aucMeasReport));
	prMeasReport = (P_IE_MEASUREMENT_REPORT_T)prReportEntry->aucMeasReport;
	prBcnReport = (struct RM_BCN_REPORT *)prMeasReport->aucReportFields;
	/* Fixed length field */
	prBcnReport->ucRegulatoryClass = prBcnReq->ucRegulatoryClass;
	prBcnReport->ucChannel = prRepParams->ucChannel;
	prBcnReport->u2Duration = prBcnReq->u2Duration;
	/* ucReportInfo: Bit 0 is the type of frame, 0 means beacon/probe response, bit 1~7 means phy type */
	prBcnReport->ucReportInfo = prRepParams->ucFrameInfo;
	prBcnReport->ucRCPI = ucRSSI;
	prBcnReport->ucRSNI = prRepParams->ucRSNI;/* ToDo: no RSNI is supported now */
	COPY_MAC_ADDR(prBcnReport->aucBSSID, pucBssid);
	prBcnReport->ucAntennaID = prRepParams->ucAntennaID; /* only one Antenna now */
	{
		OS_SYSTIME rCurrent;
		UINT_64 u8Tsf = *(PUINT_64)&rTsf.au4Tsf[0];

		GET_CURRENT_SYSTIME(&rCurrent);
		if (prRmReq->rStartTime >= rTsf.rTime)
			u8Tsf += prRmReq->rStartTime - rTsf.rTime;
		else
			u8Tsf += rTsf.rTime - prRmReq->rStartTime;
		kalMemCopy(prBcnReport->aucStartTime, &u8Tsf, 8); /* ToDo: start time is not supported now */
		u8Tsf = *(PUINT_64)&rTsf.au4Tsf[0] + rCurrent - rTsf.rTime;
		kalMemCopy(prBcnReport->aucParentTSF, &u8Tsf, 4); /* low part of TSF */
	}
	ucBcnReportLen = 0;
	/* Optional Subelement Field */
	/* all fixed length fields and IEs in Request Sub Elements should be reported */
	if (ucReportDetail == 1 && ucReportIeIdsLen > 0) {
		pucSubIE = &prBcnReport->aucOptElem[2];
		kalMemCopy(pucSubIE, prRepParams->aucBcnFixedField, BEACON_FIXED_FIELD_LENGTH);
		pucSubIE += BEACON_FIXED_FIELD_LENGTH;
		ucBcnReportLen += BEACON_FIXED_FIELD_LENGTH;
		pucIE = pucIEBuf;
		IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
			UINT_16 i = 0;
			UINT_8 ucIncludedIESize = 0;

			for (; i < ucReportIeIdsLen; i++)
				if (pucIE[0] == pucReportIeIds[i]) {
					ucIncludedIESize = IE_SIZE(pucIE);
					break;
				}
			/* the length of sub-element should less than 225,
			** and the included IE should be a complete one
			*/
			if (ucBcnReportLen + ucIncludedIESize > RM_BCN_REPORT_SUB_ELEM_MAX_LENGTH)
				break;
			if (ucIncludedIESize == 0)
				continue;
			ucBcnReportLen += ucIncludedIESize;
			kalMemCopy(pucSubIE, pucIE, ucIncludedIESize);
			pucSubIE += ucIncludedIESize;
		}
		prBcnReport->aucOptElem[0] = 1; /* sub-element id for reported frame body */
		prBcnReport->aucOptElem[1] = ucBcnReportLen; /* length of the sub-element */
		ucBcnReportLen += 2;
	} else if (ucReportDetail == 2) {/* all fixed length fields and IEs should be reported */
		ucBcnReportLen += BEACON_FIXED_FIELD_LENGTH;
		pucIE = pucIEBuf;
		/* the length of sub-element should less than 225, and the included IE should be a complete one */
		IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
			if (ucBcnReportLen + IE_SIZE(pucIE) > RM_BCN_REPORT_SUB_ELEM_MAX_LENGTH)
				break;
			ucBcnReportLen += IE_SIZE(pucIE);
		}
		prBcnReport->aucOptElem[0] = 1; /* sub-element id for reported frame body */
		prBcnReport->aucOptElem[1] = ucBcnReportLen; /* length of the sub-element */
		pucIE = &prBcnReport->aucOptElem[2];
		kalMemCopy(pucIE, prRepParams->aucBcnFixedField, BEACON_FIXED_FIELD_LENGTH);
		pucIE += BEACON_FIXED_FIELD_LENGTH;
		kalMemCopy(pucIE, pucIEBuf, ucBcnReportLen - BEACON_FIXED_FIELD_LENGTH);
		ucBcnReportLen += 2;
	}
	ucBcnReportLen += OFFSET_OF(struct RM_BCN_REPORT, aucOptElem);
	/* Step4: fill in basic content of Measurement report IE */
	prMeasReport->ucId = ELEM_ID_MEASUREMENT_REPORT;
	prMeasReport->ucToken = prRmReq->prCurrMeasElem->ucToken;
	prMeasReport->ucMeasurementType = ELEM_RM_TYPE_BEACON_REPORT;
	prMeasReport->ucReportMode = 0;
	prMeasReport->ucLength = 3 + ucBcnReportLen;
	DBGLOG(SCN, INFO, "Bss %pM, ReportDeail %d, IncludeIE Num %d, chnl %d\n",
		pucBssid, ucReportDetail, ucReportIeIdsLen, prRepParams->ucChannel);
}

static WLAN_STATUS __scanProcessBeaconAndProbeResp(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
	P_SCAN_INFO_T prScanInfo;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T) NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	P_BSS_INFO_T prAisBssInfo;
	P_WLAN_BEACON_FRAME_T prWlanBeaconFrame = (P_WLAN_BEACON_FRAME_T) NULL;
#if CFG_SLT_SUPPORT
	P_SLT_INFO_T prSltInfo = (P_SLT_INFO_T) NULL;
#endif

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	/* 4 <0> Ignore invalid Beacon Frame */
	if ((prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) <
	    (TIMESTAMP_FIELD_LEN + BEACON_INTERVAL_FIELD_LEN + CAP_INFO_FIELD_LEN)) {
		DBGLOG(SCN, ERROR, "Ignore invalid Beacon Frame\n");
		return rStatus;
	}
#if CFG_SLT_SUPPORT
	prSltInfo = &prAdapter->rWifiVar.rSltInfo;

	if (prSltInfo->fgIsDUT) {
		DBGLOG(P2P, INFO, "\n\rBCN: RX\n");
		prSltInfo->u4BeaconReceiveCnt++;
		return WLAN_STATUS_SUCCESS;
	} else {
		return WLAN_STATUS_SUCCESS;
	}
#endif

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prWlanBeaconFrame = (P_WLAN_BEACON_FRAME_T) prSwRfb->pvHeader;

	/* 4 <1> Parse and add into BSS_DESC_T */
	prBssDesc = scanAddToBssDesc(prAdapter, prSwRfb);

	prAdapter->rWlanInfo.u4ScanDbgTimes1++;
	if (prBssDesc) {
		/*Full2Partial at here, we should save channel info*/
		{
			UINT_8	ucindex;

			ucindex = nicChannelNum2Index(prBssDesc->ucChannelNum);
			if (prAdapter->prGlueInfo->arChannelScanInfo[ucindex].cScanThreshold < FULL2PARTIALTHRESHOLD)
				DBGLOG(SCN, TRACE, "Full2Partial ucChannelNum=%d, ucindex=%d, cScanThreshold=%d\n",
					prBssDesc->ucChannelNum, ucindex,
					prAdapter->prGlueInfo->arChannelScanInfo[ucindex].cScanThreshold);

			/*prAdapter->prGlueInfo->ucChannelListNum++;*/
			/*prAdapter->prGlueInfo->aucChannelNum[ucindex] = 1;*/
			prAdapter->prGlueInfo->arChannelScanInfo[ucindex].ucScanFlag = 0;
			prAdapter->prGlueInfo->arChannelScanInfo[ucindex].cScanThreshold = FULL2PARTIALTHRESHOLD;

		}

		/* 4 <1.1> Beacon Change Detection for Connected BSS */
		if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED &&
		    ((prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE && prConnSettings->eOPMode != NET_TYPE_IBSS)
		     || (prBssDesc->eBSSType == BSS_TYPE_IBSS && prConnSettings->eOPMode != NET_TYPE_INFRA))
		    && EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prAisBssInfo->aucBSSID)
		    && EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen, prAisBssInfo->aucSSID,
				  prAisBssInfo->ucSSIDLen)) {
			BOOLEAN fgNeedDisconnect = FALSE;

#if CFG_SUPPORT_BEACON_CHANGE_DETECTION
			/* <1.1.2> check if supported rate differs */
			if (prAisBssInfo->u2OperationalRateSet != prBssDesc->u2OperationalRateSet)
				fgNeedDisconnect = TRUE;
#endif
#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
			if (rsnCheckSecurityModeChanged(prAdapter, prAisBssInfo, prBssDesc)
#if CFG_SUPPORT_WAPI
				|| (prAdapter->rWifiVar.rConnSettings.fgWapiMode == TRUE &&
					!wapiPerformPolicySelection(prAdapter, prBssDesc))
#endif
				) {
				DBGLOG(SCN, INFO, "Beacon security mode change detected\n");
				DBGLOG_MEM8(SCN, INFO, prSwRfb->pvHeader, prSwRfb->u2PacketLen);
				fgNeedDisconnect = FALSE;
				if (!prConnSettings->fgSecModeChangeStartTimer) {
					cnmTimerStartTimer(prAdapter,
							&prAdapter->rWifiVar.rAisFsmInfo.rSecModeChangeTimer,
							SEC_TO_MSEC(3));
					prConnSettings->fgSecModeChangeStartTimer = TRUE;
				}
			} else {
				if (prConnSettings->fgSecModeChangeStartTimer) {
					cnmTimerStopTimer(prAdapter,
							&prAdapter->rWifiVar.rAisFsmInfo.rSecModeChangeTimer);
					prConnSettings->fgSecModeChangeStartTimer = FALSE;
				}
			}
#endif
#if CFG_SUPPORT_BEACON_CHANGE_DETECTION
			/* <1.1.3> beacon content change detected, disconnect immediately */
			if (fgNeedDisconnect == TRUE)
				aisBssBeaconTimeout(prAdapter);
#endif
		}
		/* 4 <1.1> Update AIS_BSS_INFO */
		if (((prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE && prConnSettings->eOPMode != NET_TYPE_IBSS)
		     || (prBssDesc->eBSSType == BSS_TYPE_IBSS && prConnSettings->eOPMode != NET_TYPE_INFRA))) {
			if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {

				/*
				 * _not_ checking prBssDesc->fgIsConnected anymore,
				 * due to Linksys AP uses " " as hidden SSID, and would have different BSS descriptor
				 */
				DBGLOG(SCN, TRACE, "DTIMPeriod[%d] Present[%d] BSSID[%pM]\n",
				       prAisBssInfo->ucDTIMPeriod,
				       prAisBssInfo->fgDTIMPresent,
				       prBssDesc->aucBSSID);
				if ((!prAisBssInfo->ucDTIMPeriod && prAisBssInfo->fgDTIMPresent) &&
				     EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prAisBssInfo->aucBSSID) &&
				     (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) &&
				     ((prWlanBeaconFrame->u2FrameCtrl & MASK_FRAME_TYPE) == MAC_FRAME_BEACON)) {
					PARAM_POWER_MODE ePwrMode = Param_PowerModeFast_PSP;

					prAisBssInfo->ucDTIMPeriod = prBssDesc->ucDTIMPeriod;
					prAisBssInfo->fgDTIMPresent = prBssDesc->fgDTIMPresent;
					/* To Handle No TIM IE information case */
					if (!prAisBssInfo->fgDTIMPresent) {
						ePwrMode = Param_PowerModeCAM;
						DBGLOG(SCN, WARN, "DTIM absence, set to CAM mode!\n");
					}
					nicPowerSaveInfoMap(prAdapter, prAisBssInfo->ucBssIndex,
							    ePwrMode, PS_CALLER_NO_TIM);
					nicConfigPowerSaveProfile(prAdapter, prAisBssInfo->ucBssIndex, ePwrMode, FALSE);
					/* To Handle No TIM IE information case */

					/* sync with firmware for beacon information */
					nicPmIndicateBssConnected(prAdapter, prAisBssInfo->ucBssIndex);
				}
			}
#if CFG_SUPPORT_ADHOC
			if (EQUAL_SSID(prBssDesc->aucSSID,
				       prBssDesc->ucSSIDLen,
				       prConnSettings->aucSSID,
				       prConnSettings->ucSSIDLen) &&
			    (prBssDesc->eBSSType == BSS_TYPE_IBSS) && (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS)) {

				ibssProcessMatchedBeacon(prAdapter, prAisBssInfo, prBssDesc,
							 (UINT_8) HAL_RX_STATUS_GET_RCPI(prSwRfb->prRxStatusGroup3));
			}
#endif /* CFG_SUPPORT_ADHOC */
		}

		rlmProcessBcn(prAdapter,
			      prSwRfb,
			      ((P_WLAN_BEACON_FRAME_T) (prSwRfb->pvHeader))->aucInfoElem,
			      (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
			      (UINT_16) (OFFSET_OF(WLAN_BEACON_FRAME_BODY_T, aucInfoElem[0])));

		mqmProcessBcn(prAdapter,
			      prSwRfb,
			      ((P_WLAN_BEACON_FRAME_T) (prSwRfb->pvHeader))->aucInfoElem,
			      (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
			      (UINT_16) (OFFSET_OF(WLAN_BEACON_FRAME_BODY_T, aucInfoElem[0])));

		prAdapter->rWlanInfo.u4ScanDbgTimes2++;
		/* 4 <3> Send SW_RFB_T to HIF when we perform SCAN for HOST */
		if (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE || prBssDesc->eBSSType == BSS_TYPE_IBSS) {
			/* for AIS, send to host */
			prAdapter->rWlanInfo.u4ScanDbgTimes3++;
			if (prConnSettings->fgIsScanReqIssued || prAdapter->rWifiVar.rScanInfo.fgNloScanning
#if CFG_SUPPORT_SCN_PSCN
			|| prAdapter->rWifiVar.rScanInfo.fgPscnOngoing
#endif
			) {
				BOOLEAN fgAddToScanResult;

				fgAddToScanResult = scanCheckBssIsLegal(prAdapter, prBssDesc);

				prAdapter->rWlanInfo.u4ScanDbgTimes4++;
				if (fgAddToScanResult == TRUE)
					rStatus = scanAddScanResult(prAdapter, prBssDesc, prSwRfb);
			}
		}
#if CFG_ENABLE_WIFI_DIRECT
		if (prAdapter->fgIsP2PRegistered)
			scanP2pProcessBeaconAndProbeResp(prAdapter, prSwRfb, &rStatus, prBssDesc, prWlanBeaconFrame);
#endif
	}

	return rStatus;

}				/* end of scanProcessBeaconAndProbeResp() */

VOID scanReportBss2Cfg80211(IN P_ADAPTER_T prAdapter, IN ENUM_BSS_TYPE_T eBSSType, IN P_BSS_DESC_T prSpecificBssDesc)
{
	P_LINK_T prBSSDescList = NULL;
	P_BSS_DESC_T prBssDesc = NULL;
	RF_CHANNEL_INFO_T rChannelInfo;

	ASSERT(prAdapter);

	DBGLOG(SCN, TRACE, "scanReportBss2Cfg80211\n");
	if (prSpecificBssDesc) {
		/* Check BSSID is legal channel */
		if (!scanCheckBssIsLegal(prAdapter, prSpecificBssDesc)) {
			DBGLOG(SCN, TRACE, "Remove specific SSID[%s] on channel %d\n",
			       prSpecificBssDesc->aucSSID, prSpecificBssDesc->ucChannelNum);
			return;
		}

		DBGLOG(SCN, TRACE, "Report specific SSID[%s]\n", prSpecificBssDesc->aucSSID);

		if (eBSSType == BSS_TYPE_INFRASTRUCTURE) {
			kalIndicateBssInfo(prAdapter->prGlueInfo,
					   (PUINT_8) prSpecificBssDesc->aucRawBuf,
					   prSpecificBssDesc->u2RawLength,
					   prSpecificBssDesc->ucChannelNum,
					   RCPI_TO_dBm(prSpecificBssDesc->ucRCPI));
		} else {
			rChannelInfo.ucChannelNum = prSpecificBssDesc->ucChannelNum;
			rChannelInfo.eBand = prSpecificBssDesc->eBand;
			kalP2PIndicateBssInfo(prAdapter->prGlueInfo,
					      (PUINT_8) prSpecificBssDesc->aucRawBuf,
					      prSpecificBssDesc->u2RawLength,
					      &rChannelInfo,
					      RCPI_TO_dBm(prSpecificBssDesc->ucRCPI));
		}

#if CFG_ENABLE_WIFI_DIRECT
		prSpecificBssDesc->fgIsP2PReport = FALSE;
#endif

	} else {
		/* Search BSS Desc from current SCAN result list. */
		prBSSDescList = &(prAdapter->rWifiVar.rScanInfo.rBSSDescList);

		LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {

			/* Check BSSID is legal channel */
			if (!scanCheckBssIsLegal(prAdapter, prBssDesc)) {
				DBGLOG(SCN, TRACE, "Remove SSID[%s] on channel %d\n",
				       prBssDesc->aucSSID, prBssDesc->ucChannelNum);
				continue;
			}

			if ((prBssDesc->eBSSType == eBSSType)
#if CFG_ENABLE_WIFI_DIRECT
			    || ((eBSSType == BSS_TYPE_P2P_DEVICE) && (prBssDesc->fgIsP2PReport == TRUE))
#endif
			    ) {

				DBGLOG(SCN, TRACE, "Report SSID[%s]\n", prBssDesc->aucSSID);

				if (eBSSType == BSS_TYPE_INFRASTRUCTURE) {
					if (prBssDesc->u2RawLength != 0)
						kalIndicateBssInfo(prAdapter->prGlueInfo,
								   (PUINT_8) prBssDesc->aucRawBuf,
								   prBssDesc->u2RawLength,
								   prBssDesc->ucChannelNum,
								   RCPI_TO_dBm(prBssDesc->ucRCPI));

					if (prBssDesc->u2RawLength != 0) {
						kalMemZero(prBssDesc->aucRawBuf, CFG_RAW_BUFFER_SIZE);
						prBssDesc->u2RawLength = 0;
#if CFG_ENABLE_WIFI_DIRECT
						prBssDesc->fgIsP2PReport = FALSE;
#endif
					}
				} else {
#if CFG_ENABLE_WIFI_DIRECT
					if (prBssDesc->fgIsP2PReport == TRUE) {
#endif
						rChannelInfo.ucChannelNum = prBssDesc->ucChannelNum;
						rChannelInfo.eBand = prBssDesc->eBand;

						kalP2PIndicateBssInfo(prAdapter->prGlueInfo,
								      (PUINT_8) prBssDesc->aucRawBuf,
								      prBssDesc->u2RawLength,
								      &rChannelInfo,
								      RCPI_TO_dBm(prBssDesc->ucRCPI));

						/* Do not clear it then we can pass the bss in Specific report */
						/* kalMemZero(prBssDesc->aucRawBuf,CFG_RAW_BUFFER_SIZE); */

						/* The BSS entry will not be cleared after scan done.
						 * So if we dont receive the BSS in next scan, we cannot
						 * pass it. We use u2RawLength for the purpose.
						 */
						/* prBssDesc->u2RawLength=0; */
#if CFG_ENABLE_WIFI_DIRECT
						prBssDesc->fgIsP2PReport = FALSE;
					}
#endif
				}
			}
		}
	}

}

#if CFG_SUPPORT_PASSPOINT
/*----------------------------------------------------------------------------*/
/*!
* @brief Find the corresponding BSS Descriptor according to given BSSID
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] aucBSSID           Given BSSID.
* @param[in] fgCheckSsid        Need to check SSID or not. (for multiple SSID with single BSSID cases)
* @param[in] prSsid             Specified SSID
*
* @return   Pointer to BSS Descriptor, if found. NULL, if not found
*/
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanSearchBssDescByBssidAndLatestUpdateTime(IN P_ADAPTER_T prAdapter, IN UINT_8 aucBSSID[])
{
	P_SCAN_INFO_T prScanInfo;
	P_LINK_T prBSSDescList;
	P_BSS_DESC_T prBssDesc;
	P_BSS_DESC_T prDstBssDesc = (P_BSS_DESC_T) NULL;
	OS_SYSTIME rLatestUpdateTime = 0;

	ASSERT(prAdapter);
	ASSERT(aucBSSID);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prBSSDescList = &prScanInfo->rBSSDescList;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {

		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID)) {
			if (!rLatestUpdateTime || CHECK_FOR_EXPIRATION(prBssDesc->rUpdateTime, rLatestUpdateTime)) {
				prDstBssDesc = prBssDesc;
				COPY_SYSTIME(rLatestUpdateTime, prBssDesc->rUpdateTime);
			}
		}
	}

	return prDstBssDesc;

}				/* end of scanSearchBssDescByBssid() */

#endif /* CFG_SUPPORT_PASSPOINT */

#if CFG_SUPPORT_AGPS_ASSIST
VOID scanReportScanResultToAgps(P_ADAPTER_T prAdapter)
{
	P_LINK_T prBSSDescList = &prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	P_BSS_DESC_T prBssDesc = NULL;
	P_SCAN_INFO_T prScanInfo = &prAdapter->rWifiVar.rScanInfo;
	UINT_8 ucIndex = 0;
	P_AGPS_AP_LIST_T prAgpsApList = kalMemAlloc(sizeof(AGPS_AP_LIST_T), VIR_MEM_TYPE);
	P_AGPS_AP_INFO_T prAgpsInfo = &prAgpsApList->arApInfo[0];

	if (prAgpsApList == NULL)
		return;

	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {
		if (prBssDesc->rUpdateTime < prScanInfo->rLastScanCompletedTime)
			continue;
		COPY_MAC_ADDR(prAgpsInfo->aucBSSID, prBssDesc->aucBSSID);
		prAgpsInfo->ePhyType = AGPS_PHY_G;
		prAgpsInfo->u2Channel = prBssDesc->ucChannelNum;
		prAgpsInfo->i2ApRssi = RCPI_TO_dBm(prBssDesc->ucRCPI);
		prAgpsInfo++;
		ucIndex++;
		if (ucIndex == SCN_AGPS_AP_LIST_MAX_NUM)
			break;
	}
	prAgpsApList->ucNum = ucIndex;
	GET_CURRENT_SYSTIME(&prScanInfo->rLastScanCompletedTime);
	/* DBGLOG(SCN, INFO, ("num of scan list:%d\n", ucIndex)); */
	kalIndicateAgpsNotify(prAdapter, AGPS_EVENT_WLAN_AP_LIST, (PUINT_8) prAgpsApList, sizeof(AGPS_AP_LIST_T));
	kalMemFree(prAgpsApList, VIR_MEM_TYPE, sizeof(AGPS_AP_LIST_T));
}
#endif /* CFG_SUPPORT_AGPS_ASSIST */

VOID scanGetCurrentEssChnlList(P_ADAPTER_T prAdapter)
{
	P_BSS_DESC_T prBssDesc = NULL;
	P_LINK_T prBSSDescList = &prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	P_CONNECTION_SETTINGS_T prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	struct ESS_CHNL_INFO *prEssChnlInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo.arCurEssChnlInfo[0];
	P_LINK_T prCurEssLink = &prAdapter->rWifiVar.rAisSpecificBssInfo.rCurEssLink;
	UINT_8 aucChnlBitMap[30] = {0,};
	UINT_8 aucChnlApNum[215] = {0,};
	UINT_8 aucChnlUtil[215] = {0,};
	UINT_8 ucByteNum = 0;
	UINT_8 ucBitNum = 0;
	UINT_8 ucChnlCount = 0;
	UINT_8 j = 0;
	/*UINT_8 i = 0;*/

	if (prConnSettings->ucSSIDLen == 0) {
		DBGLOG(SCN, INFO, "No Ess are expected to connect\n");
		return;
	}
	kalMemZero(prEssChnlInfo, CFG_MAX_NUM_OF_CHNL_INFO * sizeof(struct ESS_CHNL_INFO));
	while (!LINK_IS_EMPTY(prCurEssLink)) {
		prBssDesc = LINK_PEEK_HEAD(prCurEssLink, BSS_DESC_T, rLinkEntryEss);
		LINK_REMOVE_KNOWN_ENTRY(prCurEssLink, &prBssDesc->rLinkEntryEss);
	}
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T) {
		if (prBssDesc->ucChannelNum > 214)
			continue;
		/* Statistic AP num for each channel */
		if (aucChnlApNum[prBssDesc->ucChannelNum] < 255)
			aucChnlApNum[prBssDesc->ucChannelNum]++;
		if (aucChnlUtil[prBssDesc->ucChannelNum] < prBssDesc->ucChnlUtilization)
			aucChnlUtil[prBssDesc->ucChannelNum] = prBssDesc->ucChnlUtilization;
		if (!EQUAL_SSID(prConnSettings->aucSSID, prConnSettings->ucSSIDLen,
			prBssDesc->aucSSID, prBssDesc->ucSSIDLen))
			continue;
		/* Record same BSS list */
		LINK_INSERT_HEAD(prCurEssLink, &prBssDesc->rLinkEntryEss);
		ucByteNum = prBssDesc->ucChannelNum / 8;
		ucBitNum = prBssDesc->ucChannelNum % 8;
		if (aucChnlBitMap[ucByteNum] & BIT(ucBitNum))
			continue;
		aucChnlBitMap[ucByteNum] |= BIT(ucBitNum);
		prEssChnlInfo[ucChnlCount].ucChannel = prBssDesc->ucChannelNum;
		ucChnlCount++;
		if (ucChnlCount >= CFG_MAX_NUM_OF_CHNL_INFO)
			break;
	}
	prAdapter->rWifiVar.rAisSpecificBssInfo.ucCurEssChnlInfoNum = ucChnlCount;
	for (j = 0; j < ucChnlCount; j++) {
		UINT_8 ucChnl = prEssChnlInfo[j].ucChannel;

		prEssChnlInfo[j].ucApNum = aucChnlApNum[ucChnl];
		prEssChnlInfo[j].ucUtilization = aucChnlUtil[ucChnl];
	}
#if 0
	/* Sort according to AP number */
	for (j = 0; j < ucChnlCount; j++) {
		for (i = j + 1; i < ucChnlCount; i++)
			if (prEssChnlInfo[j].ucApNum > prEssChnlInfo[i].ucApNum) {
				struct ESS_CHNL_INFO rTemp = prEssChnlInfo[j];

				prEssChnlInfo[j] = prEssChnlInfo[i];
				prEssChnlInfo[i] = rTemp;
			}
	}
#endif
	DBGLOG(SCN, INFO, "Find %s in %d BSSes, result %d\n",
		prConnSettings->aucSSID, prBSSDescList->u4NumElem, prCurEssLink->u4NumElem);
}

#define CALCULATE_SCORE_BY_PROBE_RSP(prBssDesc) \
	(WEIGHT_IDX_PROBE_RSP * (prBssDesc->fgSeenProbeResp ? BSS_FULL_SCORE : 0))

#define CALCULATE_SCORE_BY_MISS_CNT(prAdapter, prBssDesc) \
	(WEIGHT_IDX_SCN_MISS_CNT * \
	(prAdapter->rWifiVar.rAisFsmInfo.u4ScanUpdateIdx - prBssDesc->u4UpdateIdx > 3 ? 0 : \
	(BSS_FULL_SCORE - (prAdapter->rWifiVar.rAisFsmInfo.u4ScanUpdateIdx - prBssDesc->u4UpdateIdx) * 25)))

#define CALCULATE_SCORE_BY_BAND(prAdapter, prBssDesc, cRssi) \
	(WEIGHT_IDX_5G_BAND * \
	((prBssDesc->eBand == BAND_5G && prAdapter->fgEnable5GBand && cRssi > -70) ? BSS_FULL_SCORE : 0))

#define CALCULATE_SCORE_BY_STBC(prAdapter, prBssDesc) \
	(WEIGHT_IDX_STBC * \
	((prBssDesc->fgMultiAnttenaAndSTBC && prAdapter->rWifiVar.ucRxStbc) ? BSS_FULL_SCORE:0))

#define CALCULATE_SCORE_BY_DEAUTH(prBssDesc) \
	(WEIGHT_IDX_DEAUTH_LAST * (prBssDesc->fgDeauthLastTime ? 0:BSS_FULL_SCORE))

#if 0/* we don't take it into account now */
/* Channel Utilization: weight index will be */
static UINT_16 scanCalculateScoreByChnlInfo(
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo, UINT_8 ucChannel)
{
	struct ESS_CHNL_INFO *prEssChnlInfo = &prAisSpecificBssInfo->arCurEssChnlInfo[0];
	UINT_8 i = 0;
	UINT_16 u2Score = 0;

	for (; i < prAisSpecificBssInfo->ucCurEssChnlInfoNum; i++) {
		if (ucChannel == prEssChnlInfo[i].ucChannel) {
#if 0	/* currently, we don't take channel utilization into account */
			/* the channel utilization max value is 255. great utilization means little weight value.
			 * the step of weight value is 2.6
			*/
			u2Score = WEIGHT_IDX_CHNL_UTIL *
				(BSS_FULL_SCORE - (prEssChnlInfo[i].ucUtilization * 10 / 26));
#endif
			/* if AP num on this channel is greater than 100, the weight will be 0.
			 * otherwise, the weight value decrease 1 if AP number increase 1
			*/
			if (prEssChnlInfo[i].ucApNum <= CHNL_BSS_NUM_THRESOLD)
				u2Score += WEIGHT_IDX_AP_NUM *
					(BSS_FULL_SCORE - prEssChnlInfo[i].ucApNum * SCORE_PER_AP);
			DBGLOG(SCN, INFO, "channel %d\n", ucChannel);
			break;
		}
	}
	return u2Score;
}
#endif
static UINT_16 scanCalculateScoreByBandwidth(P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
	UINT_16 u2Score = 0;
	ENUM_CHANNEL_WIDTH_T eChannelWidth = prBssDesc->eChannelWidth;
	UINT_8 ucSta5GBW = prAdapter->rWifiVar.ucSta5gBandwidth;
	UINT_8 ucSta2GBW = prAdapter->rWifiVar.ucSta2gBandwidth;
	UINT_8 ucStaBW = prAdapter->rWifiVar.ucStaBandwidth;

	if (prBssDesc->fgIsVHTPresent && prAdapter->fgEnable5GBand) {
		if (ucSta5GBW > ucStaBW)
			ucSta5GBW = ucStaBW;
		switch (ucSta5GBW) {
		case MAX_BW_20MHZ:
		case MAX_BW_40MHZ:
			eChannelWidth = CW_20_40MHZ;
			break;
		case MAX_BW_80MHZ:
			eChannelWidth = CW_80MHZ;
			break;
		}
		switch (eChannelWidth) {
		case CW_20_40MHZ:
			u2Score = 60;
			break;
		case CW_80MHZ:
			u2Score = 80;
			break;
		case CW_160MHZ:
		case CW_80P80MHZ:
			u2Score = BSS_FULL_SCORE;
			break;
		}
	} else if (prBssDesc->fgIsHTPresent) {
		if (prBssDesc->eBand == BAND_2G4) {
			if (ucSta2GBW > ucStaBW)
				ucSta2GBW = ucStaBW;
			u2Score = (prBssDesc->eSco == 0 || ucSta2GBW == MAX_BW_20MHZ) ? 40:60;
		} else if (prBssDesc->eBand == BAND_5G) {
			if (ucSta5GBW > ucStaBW)
				ucSta5GBW = ucStaBW;
			u2Score = (prBssDesc->eSco == 0 || ucSta5GBW == MAX_BW_20MHZ) ? 40:60;
		}
	} else if (prBssDesc->u2BSSBasicRateSet & RATE_SET_OFDM)
		u2Score = 20;
	else
		u2Score = 10;
	/* DBGLOG(SCN, INFO, "eCW %d, vht %d, ht %d, eband %d, esco %d, u2Score %d\n",
	  * eChannelWidth, prBssDesc->fgIsVHTPresent,
	  * prBssDesc->fgIsHTPresent, prBssDesc->eBand, prBssDesc->eSco, u2Score);
	  */
	return u2Score * WEIGHT_IDX_BAND_WIDTH;
}

static UINT_16 scanCalculateScoreByClientCnt(P_BSS_DESC_T prBssDesc)
{
	UINT_16 u2Score = 0;
	UINT_16 u2StaCnt = 0;

	if (!prBssDesc->fgExsitBssLoadIE || prBssDesc->u2StaCnt > BSS_STA_CNT_THRESOLD) {
		DBGLOG(SCN, TRACE, "exist bss load %d, sta cnt %d\n",
			prBssDesc->fgExsitBssLoadIE, prBssDesc->u2StaCnt);
		return 0;
	}
	u2StaCnt = prBssDesc->u2StaCnt;
	if (u2StaCnt < 9)
		u2Score = BSS_FULL_SCORE - u2StaCnt;
	else if (u2StaCnt < 17)
		u2Score = BSS_FULL_SCORE - 2 * u2StaCnt + 8;
	else if (u2StaCnt < 31)
		u2Score = BSS_FULL_SCORE - 3 * u2StaCnt + 24;
	else if (u2StaCnt < 39)
		u2Score = BSS_FULL_SCORE - 4 * u2StaCnt + 54;
	else
		u2Score = 0;
	return u2Score * WEIGHT_IDX_CLIENT_CNT;
}

static UINT_16 scanCalculateScoreBySnrRssi(P_BSS_DESC_T prBssDesc)
{
	UINT_16 u2Score = 0;
	INT_8 cRssi = RCPI_TO_dBm(prBssDesc->ucRCPI);

	/*UINT_8 ucSNR = prBssDesc->ucSNR;*/
	DBGLOG(SCN, TRACE, "cRSSI %d\n", cRssi);
	if (cRssi >= BEST_RSSI)
		u2Score = 100;
	else
		u2Score = (cRssi + 98) * 2;
	u2Score *= WEIGHT_IDX_RSSI;

	/* TODO: we don't know the valid value for SNR, so don't take it into account */
	return u2Score;
}

static BOOLEAN scanNeedReplaceCandidate(P_ADAPTER_T prAdapter, P_BSS_DESC_T prCandBss,
					P_BSS_DESC_T prCurrBss, UINT_16 u2CandScore, UINT_16 u2CurrScore,
					INT_16 i2CandPref, INT_16 i2CurrPref)
{
	INT_8 cCandRssi = RCPI_TO_dBm(prCandBss->ucRCPI);
	INT_8 cCurrRssi = RCPI_TO_dBm(prCurrBss->ucRCPI);
	UINT_32 u4UpdateIdx = prAdapter->rWifiVar.rAisFsmInfo.u4ScanUpdateIdx;
	UINT_16 u2CandMiss = u4UpdateIdx - prCandBss->u4UpdateIdx;
	UINT_16 u2CurrMiss = u4UpdateIdx - prCurrBss->u4UpdateIdx;

	/* 1. No need check score case */
	/* 1.1 Scan missing count of CurrBss is too more, but Candidate is suitable, don't replace */
	if (u2CurrMiss > 2 && u2CurrMiss > u2CandMiss) {
		DBGLOG(SCN, WARN, "Scan Miss count of CurrBss > 2, and Candidate <= 2\n");
		return FALSE;
	}
	/* 1.2 Scan missing count of Candidate is too more, but CurrBss is suitable, replace */
	if (u2CandMiss > 2 && u2CandMiss > u2CurrMiss) {
		DBGLOG(SCN, WARN, "Scan Miss count of Candidate > 2, and CurrBss <= 2\n");
		return TRUE;
	}
	/* 1.3 Hard connecting RSSI check */
	if ((prCurrBss->eBand == BAND_5G && cCurrRssi < MINIMUM_RSSI_5G) ||
		(prCurrBss->eBand == BAND_2G4 && cCurrRssi < MINIMUM_RSSI_2G4))
		return FALSE;
	else if ((prCandBss->eBand == BAND_5G && cCandRssi < MINIMUM_RSSI_5G) ||
		(prCandBss->eBand == BAND_2G4 && cCandRssi < MINIMUM_RSSI_2G4))
		return TRUE;

	/* 1.4 RSSI of Current Bss is lower than Candidate, don't replace
	** If the lower Rssi is greater than -59dbm, then no need check the difference
	** Otherwise, if difference is greater than 10dbm, select the good RSSI
	*/
	if ((cCandRssi - cCurrRssi >= RSSI_DIFF_BETWEEN_BSS) &&
		cCurrRssi < (GOOD_RSSI_FOR_HT_VHT + 5))
		return FALSE;
	/* 1.5 RSSI of Candidate Bss is lower than Current, replace */
	if ((cCurrRssi - cCandRssi >= RSSI_DIFF_BETWEEN_BSS) &&
		cCandRssi < (GOOD_RSSI_FOR_HT_VHT + 5))
		return TRUE;

	/* 1.6 prefer to select 2.4G Bss if Rssi of a 5G band BSS is lower than -70dbm */
	if (prCandBss->eBand != prCurrBss->eBand) {
		if (prCandBss->eBand == BAND_5G) {
			if (cCandRssi < LOW_RSSI_FOR_5G_BAND)
				return TRUE;
		} else if (cCurrRssi < LOW_RSSI_FOR_5G_BAND)
			return FALSE;
	}
	/* 2. Check Score */
	/* 2.1 Apply influence of preference */
	if (i2CandPref > i2CurrPref) {
		if (i2CurrPref < 0)
			u2CandScore += BSS_FULL_SCORE * WEIGHT_IDX_EXCLUDE;
		else
			u2CandScore += BSS_FULL_SCORE * WEIGHT_IDX_PREFERENCE;
	} else if (i2CurrPref > i2CandPref) {
		if (i2CandPref < 0)
			u2CurrScore += BSS_FULL_SCORE * WEIGHT_IDX_EXCLUDE;
		else
			u2CurrScore += BSS_FULL_SCORE * WEIGHT_IDX_PREFERENCE;
	}
	/* 2.2 Cases that no need to replace candidate */
	if (prCandBss->fgIsConnected) {
		if ((u2CandScore + ROAMING_NO_SWING_SCORE_STEP) >= u2CurrScore)
			return FALSE;
	} else if (prCurrBss->fgIsConnected) {
		if (u2CandScore >= (u2CurrScore + ROAMING_NO_SWING_SCORE_STEP))
			return FALSE;
	} else if (u2CandScore >= u2CurrScore)
		return FALSE;
	/* 2.3 other cases, replace candidate */
	return TRUE;
}

static BOOLEAN scanSanityCheckBssDesc(P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
	ENUM_BAND_T eBand = BAND_2G4;
	UINT_8 ucChannel = 0;

	if (!(prBssDesc->ucPhyTypeSet & (prAdapter->rWifiVar.ucAvailablePhyTypeSet))) {
		DBGLOG(SCN, WARN, "BSS %pM, Ignore unsupported ucPhyTypeSet = %x\n",
			prBssDesc->aucBSSID, prBssDesc->ucPhyTypeSet);
		return FALSE;
	}
	if (prBssDesc->fgIsUnknownBssBasicRate) {
		DBGLOG(SCN, WARN, "BSS %pM, fgIsUnknownBssBasicRate\n", prBssDesc->aucBSSID);
		return FALSE;
	}
	if (cnmAisInfraChannelFixed(prAdapter, &eBand, &ucChannel) == TRUE &&
		(eBand != prBssDesc->eBand || ucChannel != prBssDesc->ucChannelNum)) {
		DBGLOG(SCN, INFO, "BSS %pM, Band %d channel %d, fix channel required band %d channel %d\n",
			prBssDesc->aucBSSID, prBssDesc->eBand, prBssDesc->ucChannelNum, eBand, ucChannel);
		return FALSE;
	}
	if (!rlmDomainIsLegalChannel(prAdapter, prBssDesc->eBand, prBssDesc->ucChannelNum)) {
		DBGLOG(SCN, WARN, "BSS %pM, Band %d channel %d is not legal\n",
			prBssDesc->aucBSSID, prBssDesc->eBand, prBssDesc->ucChannelNum);
		return FALSE;
	}
#if CFG_SUPPORT_RN
	if (prAdapter->prAisBssInfo->fgDisConnReassoc == FALSE)
#endif
		if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), prBssDesc->rUpdateTime,
					SEC_TO_SYSTIME(SCN_BSS_DESC_STALE_SEC))) {
			DBGLOG(SCN, WARN, "BSS %pM rUpdateTime=[%u,%u] timeout, ret line @ %d\n",
				prBssDesc->aucBSSID, prBssDesc->rUpdateTime,
				prBssDesc->rAddBssDescTime, prBssDesc->u2ReturnLine);
			return FALSE;
		}
#if CFG_SUPPORT_WAPI
	if (prAdapter->rWifiVar.rConnSettings.fgWapiMode) {
		if (!wapiPerformPolicySelection(prAdapter, prBssDesc))
			return FALSE;
	} else
#endif
	if (!rsnPerformPolicySelection(prAdapter, prBssDesc))
		return FALSE;
	if (prAdapter->rWifiVar.rAisSpecificBssInfo.fgCounterMeasure) {
		DBGLOG(SCN, WARN, "BSS %pM, Skip while at counter measure period!!!\n", prBssDesc->aucBSSID);
		return FALSE;
	}
	return TRUE;
}

static UINT_8 scanGenNeighborApReport(P_BSS_DESC_T prBssDesc, PUINT_8 pucBuf, UINT_8 ucPreference)
{
	struct IE_NEIGHBOR_REPORT_T *prReport = (struct IE_NEIGHBOR_REPORT_T *)pucBuf;

	prReport->ucId = ELEM_ID_NEIGHBOR_REPORT;
	prReport->ucLength = sizeof(*prReport) - 2 + 3;
	COPY_MAC_ADDR(prReport->aucBSSID, prBssDesc->aucBSSID);
	/* AP Reachability is unknown BITS(0,1) == 2
	** Security is same with current AP, BIT(2) is set
	** key scope is unknown, BIT(3) is not set
	*/
	prReport->u4BSSIDInfo = 6;
	prReport->ucChnlNumber = prBssDesc->ucChannelNum;
	prReport->ucOperClass = 0; /* Todo: fill in right operation class */
	/* Fill in dot11PHYType. FHSS(1), DSSS(2), IR base band(3), OFDM(5), ERP(6), HT(7)
	** See EEE Std 802.11-2012, Annex C, MIB Detail
	*/
	if (prBssDesc->ucPhyTypeSet & (PHY_TYPE_BIT_HT | PHY_TYPE_BIT_VHT))
		prReport->ucPhyType = 7;
	else if (prBssDesc->ucPhyTypeSet & PHY_TYPE_BIT_ERP)
		prReport->ucPhyType = 6;
	else if (prBssDesc->ucPhyTypeSet & PHY_TYPE_BIT_OFDM)
		prReport->ucPhyType = 4;
	else if (prBssDesc->ucPhyTypeSet & PHY_TYPE_BIT_HR_DSSS)
		prReport->ucPhyType = 5;
	else
		prReport->ucPhyType = 2;

	prReport->aucSubElem[0] = 3; /* Preference sub element id is 3 */
	prReport->aucSubElem[1] = 1; /* Length of sub element is 1 */
	prReport->aucSubElem[2] = ucPreference; /* Preference value */
	return IE_SIZE(prReport);
}

struct NEIGHBOR_AP_T *scanGetNeighborAPEntry(
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo, PUINT_8 pucBssid)
{
	P_LINK_T prNeighborApLink = &prAisSpecificBssInfo->rNeighborApList.rUsingLink;
	struct NEIGHBOR_AP_T *prNeighborAP = NULL;

	LINK_FOR_EACH_ENTRY(prNeighborAP, prNeighborApLink, rLinkEntry, struct NEIGHBOR_AP_T) {
		if (EQUAL_MAC_ADDR(prNeighborAP->aucBssid, pucBssid))
			return prNeighborAP;
	}
	return NULL;
}

/*****
* Bss Characteristics to be taken into account when calculate Score:
* Channel Loading Group:
* 1. Client Count (in BSS Load IE).
* 2. AP number on the Channel.
*
* RF Group:
* 1. Channel utilization.
* 2. SNR.
* 3. RSSI.
*
* Misc Group:
* 1. Deauth Last time.
* 2. Scan Missing Count.
* 3. Has probe response in scan result.
*
* Capability Group:
* 1. Prefer 5G band.
* 2. Bandwidth.
* 3. STBC and Multi Anttena.
*/
P_BSS_DESC_T scanSearchBssDescByScoreForAis(P_ADAPTER_T prAdapter)
{
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo = NULL;
	P_LINK_T prEssLink = NULL;
	P_CONNECTION_SETTINGS_T prConnSettings = NULL;
	P_BSS_DESC_T prBssDesc = NULL;
	P_BSS_DESC_T prCandBssDesc = NULL;
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
	struct NEIGHBOR_AP_T *prNeighborAP = NULL;
	struct BSS_TRANSITION_MGT_PARAM_T *prBtmParam = NULL;
	INT_8 cConnectedRssi = -128;
#endif
	UINT_16 u2ScoreBand = 0;
	UINT_16 u2ScoreStaCnt = 0;
	UINT_16 u2ScoreProbeRsp = 0;
	UINT_16 u2ScoreScanMiss = 0;
	UINT_16 u2ScoreBandwidth = 0;
	UINT_16 u2ScoreSTBC = 0;
	UINT_16 u2ScoreDeauth = 0;
	UINT_16 u2ScoreSnrRssi = 0;
	UINT_16 u2ScoreTotal = 0;
	UINT_16 u2CandBssScore = 0;
	UINT_16 u2BlackListScore = 0;
	BOOLEAN fgSearchBlackList = FALSE;
	INT_8 cRssi = -128;
	INT_16 i2CandPreference = 0;
	INT_16 i2Preference = 0;
#if 0 /* Don't send our neighbor ap list currently */
	UINT_8 ucPrioBssNum = 0;
	static P_BSS_DESC_T aprPrioBss[CFG_MAX_NUM_BSS_LIST];
#endif

	if (!prAdapter) {
		DBGLOG(SCN, ERROR, "prAdapter is NULL!\n");
		return NULL;
	}
	prAisSpecificBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prEssLink = &prAisSpecificBssInfo->rCurEssLink;

	aisRemoveTimeoutBlacklist(prAdapter);

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
	prBtmParam = &prAisSpecificBssInfo->rBTMParam;
#if 0	/* Don't send our neighbor ap list currently */
	kalMemZero(&aprPrioBss[0], sizeof(aprPrioBss));
#endif
	if (prBtmParam->fgUnsolicitedReq) {
		LINK_FOR_EACH_ENTRY(prBssDesc, prEssLink, rLinkEntryEss, BSS_DESC_T) {
			if (prBssDesc->fgIsConnected) {
				cConnectedRssi = RCPI_TO_dBm(prBssDesc->ucRCPI);
				DBGLOG(SCN, INFO, "RSSI of Connected AP(%pM) is %d\n",
				       prBssDesc->aucBSSID, cConnectedRssi);
				break;
			}
		}
		prBssDesc = NULL;
	}
#endif

try_again:
	LINK_FOR_EACH_ENTRY(prBssDesc, prEssLink, rLinkEntryEss, BSS_DESC_T) {
		if (prConnSettings->eConnectionPolicy == CONNECT_BY_BSSID) {
			if (!EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prConnSettings->aucBSSID))
				continue;
			if (!scanSanityCheckBssDesc(prAdapter, prBssDesc))
				continue;
			DBGLOG(SCN, INFO,
			       "Selected %pM base on bssid, when find %s, %pM in %d BSSes\n",
			       prBssDesc->aucBSSID, prConnSettings->aucSSID,
			       prConnSettings->aucBSSID, prEssLink->u4NumElem);
			return prBssDesc;
		} else if (!fgSearchBlackList) {
			prBssDesc->prBlack = aisQueryBlackList(prAdapter, prBssDesc);
			if (prBssDesc->prBlack) {
				if (prBssDesc->prBlack->blackListSource & AIS_BLACK_LIST_FROM_FWK)
					DBGLOG(SCN, INFO, "%s(%pM) is in FWK blacklist, skip it\n",
					       prBssDesc->aucSSID, prBssDesc->aucBSSID);
				continue;
			}
		} else if (!prBssDesc->prBlack)
			continue;
		else {
			/* never search FWK blacklist even if we are trying blacklist */
			if (prBssDesc->prBlack->blackListSource	& AIS_BLACK_LIST_FROM_FWK) {
				DBGLOG(SCN, INFO, "Although trying blacklist, %s(%pM) is in FWK blacklist, skip it\n",
				       prBssDesc->aucSSID, prBssDesc->aucBSSID);
				continue;
			}
			u2BlackListScore = WEIGHT_IDX_BLACK_LIST *
				aisCalculateBlackListScore(prAdapter, prBssDesc);
		}

		cRssi = RCPI_TO_dBm(prBssDesc->ucRCPI);
		DBGLOG(SCN, TRACE, "cRSSI %d, %pM\n", cRssi, prBssDesc->aucBSSID);

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
		prNeighborAP = scanGetNeighborAPEntry(prAisSpecificBssInfo, prBssDesc->aucBSSID);

		/* if Bss is to terminate in 10 minutes, don't select it */
		if (prNeighborAP && prNeighborAP->u8TermTsf > 0 &&
		    (prNeighborAP->u8TermTsf <= prBssDesc->u8TimeStamp.QuadPart ||
		    prNeighborAP->u8TermTsf - prBssDesc->u8TimeStamp.QuadPart < 10 * 60 * USEC_PER_SEC))
			continue;
#endif

		if (!scanSanityCheckBssDesc(prAdapter, prBssDesc))
			continue;

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
		if (prBtmParam->fgUnsolicitedReq) {
			/* Don't select BSSes that are not in candidate list and
			** abridged bit is set or preference is 0
			*/
			if (!prNeighborAP) {
				if (prBtmParam->ucRequestMode & BTM_REQ_MODE_ABRIDGED)
					continue;
				i2Preference = 0;
			} else {
				i2Preference = prNeighborAP->ucPreference;
				if (!i2Preference) {
					DBGLOG(SCN, WARN, "Bss %pM is excluded by Serving AP\n",
					       prBssDesc->aucBSSID);
					continue;
				}
			}
			/* ucCandPreference is initialized to 0, so if ucPreference > ucCandPreference,
			** means: if prCanBssDesc is NULL, then prBssDesc can be selected as
			** candidate base on preference
			** if prCandBssDesc is not NULL, then prBssDesc can be also replace candidate
			** directly base on preference
			*/
			DBGLOG(SCN, INFO, "Neighbor %d, Pref %d, rssi %d, bssid %pM, CandPref %d\n",
			       !!prNeighborAP, i2Preference, cRssi, prBssDesc->aucBSSID, i2CandPreference);
			if (i2Preference > i2CandPreference) {
				/* If cRssi is better than GOOD_RSS_FOR_HT_VHT, no need to consider RSSI,
				** because no matter the RSSI is in which value, data rate is similar.
				** Otherwise, if cRssi + 5dbm is less than the Rssi of connected bss, by pass it.
				*/
				if (cRssi < GOOD_RSSI_FOR_HT_VHT && cRssi + 5 < cConnectedRssi) {
					DBGLOG(SCN, INFO,
					       "Bypass due to rssi(%d dbm) weaker than connected bss(%d dbm)\n",
					       cRssi, cConnectedRssi);
					continue;
				}
				prCandBssDesc = prBssDesc;
				i2CandPreference = i2Preference;
				continue;
			} else if (i2CandPreference > 0 && i2Preference < i2CandPreference)
				continue;
			/* If all Bss that're in candidate list have preference 0, or preference is equal,
			** fallback to traditional ap selection algorithm
			*/
		} else if (prNeighborAP)
			i2Preference = !prNeighborAP->ucPreference ? -1 : prNeighborAP->ucPreference;
		else
			i2Preference = 0;
#endif

		u2ScoreBandwidth = scanCalculateScoreByBandwidth(prAdapter, prBssDesc);
		u2ScoreStaCnt = scanCalculateScoreByClientCnt(prBssDesc);
		u2ScoreSTBC = CALCULATE_SCORE_BY_STBC(prAdapter, prBssDesc);
#if 0	/* currently, we don't take these factors into account */
		u2ScoreChnlInfo = scanCalculateScoreByChnlInfo(prAisSpecificBssInfo, prBssDesc->ucChannelNum);
#endif
		u2ScoreSnrRssi = scanCalculateScoreBySnrRssi(prBssDesc);
		u2ScoreDeauth = CALCULATE_SCORE_BY_DEAUTH(prBssDesc);
		u2ScoreProbeRsp = CALCULATE_SCORE_BY_PROBE_RSP(prBssDesc);
		u2ScoreScanMiss = CALCULATE_SCORE_BY_MISS_CNT(prAdapter, prBssDesc);
		u2ScoreBand = CALCULATE_SCORE_BY_BAND(prAdapter, prBssDesc, cRssi);
		u2ScoreTotal = u2ScoreBandwidth + u2ScoreDeauth + u2ScoreProbeRsp +
			u2ScoreScanMiss + u2ScoreSnrRssi + u2ScoreStaCnt + u2ScoreSTBC + u2ScoreBand + u2BlackListScore;
		DBGLOG(SCN, INFO,
		"%pM Score,Total %d:BW[%d],PRE[%d],DE[%d],PR[%d],SM[%d],SC[%d],SR[%d,%d],ST[%d],BD[%d],Connected %d\n",
			prBssDesc->aucBSSID, u2ScoreTotal, u2ScoreBandwidth, i2Preference, u2ScoreDeauth,
			u2ScoreProbeRsp, u2ScoreScanMiss, u2ScoreStaCnt, u2ScoreSnrRssi,
			RCPI_TO_dBm(prBssDesc->ucRCPI), u2ScoreSTBC, u2ScoreBand, prBssDesc->fgIsConnected);

		if (!prCandBssDesc ||
		    scanNeedReplaceCandidate(prAdapter, prCandBssDesc, prBssDesc, u2CandBssScore, u2ScoreTotal,
					     i2CandPreference, i2Preference)) {
			prCandBssDesc = prBssDesc;
			u2CandBssScore = u2ScoreTotal;
			i2CandPreference = i2Preference;
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT && 0 /* Don't send our neighbor ap list currently */
			aprPrioBss[ucPrioBssNum++] = prBssDesc;
#endif
		}
	} /* end of LINK_FOR_EACH */

	if (prConnSettings->eConnectionPolicy == CONNECT_BY_BSSID) {
		DBGLOG(SCN, INFO, "Selected None base on bssid %pM in %d BSSes\n",
		       prConnSettings->aucBSSID, prEssLink->u4NumElem);
		return NULL;
	}

	if (prCandBssDesc) {
		DBGLOG(SCN, INFO,
		       "Selected %pM, Score %d when find %s, %pM in %d BSSes, blacklist %d\n",
		       prCandBssDesc->aucBSSID, u2CandBssScore, prConnSettings->aucSSID,
		       prConnSettings->aucBSSID, prEssLink->u4NumElem, fgSearchBlackList);
		return prCandBssDesc;
	}
	/* if No Candidate BSS is found, try BSSes which are in blacklist */
	if (!fgSearchBlackList && prEssLink->u4NumElem > 0) {
		fgSearchBlackList = TRUE;
		DBGLOG(SCN, TRACE, "No Bss is found, Try blacklist\n");
		goto try_again;
	}
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT && 0 /* Don't send our neighbor ap list currently */
	/* Prepare our neighbor AP list if need to send back Bss Transition Mgmt response frame */
	if (prAisSpecificBssInfo->rBTMParam.fgPendingResponse && !prAdapter->rWifiVar.rAisFsmInfo.fgAdjChnlScanIssued) {
		PUINT_8 pucNeighborAPBuf = NULL;
		INT_16 i = -1;

		kalMemFree(prAisSpecificBssInfo->rBTMParam.pucOurNeighborBss, VIR_MEM_TYPE,
			prAisSpecificBssInfo->rBTMParam.u2OurNeighborBssLen);
		prAisSpecificBssInfo->rBTMParam.u2OurNeighborBssLen =
			ucPrioBssNum * (sizeof(struct IE_NEIGHBOR_REPORT_T) + 3);
		pucNeighborAPBuf = kalMemAlloc(prAisSpecificBssInfo->rBTMParam.u2OurNeighborBssLen, VIR_MEM_TYPE);
		prAisSpecificBssInfo->rBTMParam.pucOurNeighborBss = pucNeighborAPBuf;
		if (pucNeighborAPBuf) {
			kalMemZero(pucNeighborAPBuf, prAisSpecificBssInfo->rBTMParam.u2OurNeighborBssLen);
			i = (INT_16)ucPrioBssNum - 1;
			for (; i >= 0; i--)
				pucNeighborAPBuf +=
				scanGenNeighborApReport(aprPrioBss[i], pucNeighborAPBuf, 1);
		}
	}
#endif
	DBGLOG(SCN, INFO, "Selected None when find %s, %pM in %d BSSes, blacklist %d\n",
	       prConnSettings->aucSSID, prConnSettings->aucBSSID,
	       prEssLink->u4NumElem, fgSearchBlackList);

	return NULL;
}

VOID scanCollectNeighborBss(P_ADAPTER_T prAdapter)
{
	P_WIFI_VAR_T prWifiVar = &prAdapter->rWifiVar;
	struct BSS_TRANSITION_MGT_PARAM_T *prBtmParam = &prWifiVar->rAisSpecificBssInfo.rBTMParam;
	PUINT_8 pucNeighborBssBuf = prBtmParam->pucOurNeighborBss;
	P_BSS_DESC_T prConnectedBssDesc = prWifiVar->rAisFsmInfo.prTargetBssDesc;
	P_BSS_DESC_T prBssDesc = NULL;
	P_LINK_T prEssLink = NULL;

	if (!prAdapter->prAisBssInfo || prAdapter->prAisBssInfo->eConnectionState !=
		PARAM_MEDIA_STATE_CONNECTED || !prConnectedBssDesc)
		return;
	kalMemFree(pucNeighborBssBuf, VIR_MEM_TYPE, prBtmParam->u2OurNeighborBssLen);
	prBtmParam->pucOurNeighborBss = NULL;
	prEssLink = &prWifiVar->rAisSpecificBssInfo.rCurEssLink;
	prBtmParam->u2OurNeighborBssLen = prEssLink->u4NumElem * sizeof(struct IE_NEIGHBOR_REPORT_T);
	pucNeighborBssBuf = kalMemAlloc(prBtmParam->u2OurNeighborBssLen, VIR_MEM_TYPE);
	if (!pucNeighborBssBuf) {
		DBGLOG(SCN, ERROR, "No memory to collect neighbor Bss\n");
		return;
	}
	prBtmParam->pucOurNeighborBss = pucNeighborBssBuf;
	LINK_FOR_EACH_ENTRY(prBssDesc, prEssLink, rLinkEntryEss, BSS_DESC_T) {
		if (prBssDesc == prConnectedBssDesc)
			continue;
		pucNeighborBssBuf += scanGenNeighborApReport(prBssDesc, pucNeighborBssBuf, 0);
	}
}

