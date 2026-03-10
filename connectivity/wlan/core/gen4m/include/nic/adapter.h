/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*! \file   adapter.h
 *  \brief  Definition of internal data structure for driver manipulation.
 *
 *  In this file we define the internal data structure - ADAPTER_T which stands
 *  for MiniPort ADAPTER(From Windows point of view) or stands for
 *  Network ADAPTER.
 */

#ifndef _ADAPTER_H
#define _ADAPTER_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_os.h"
#if (CFG_SUPPORT_POWER_THROTTLING == 1)
#include "thrm.h"
#endif
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define MIN_TX_DURATION_TIME_MS 100

#define SET_HE_MCS_MAP 1
#define SET_EHT_BW20_MCS_MAP 2
#define SET_EHT_BW80_MCS_MAP 4

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
enum {
	ENUM_SW_TEST_MODE_NONE = 0,
	ENUM_SW_TEST_MODE_SIGMA_AC = 0x1,
	ENUM_SW_TEST_MODE_SIGMA_WFD = 0x2,
	ENUM_SW_TEST_MODE_CTIA = 0x3,
	ENUM_SW_TEST_MODE_SIGMA_TDLS = 0x4,
	ENUM_SW_TEST_MODE_SIGMA_P2P = 0x5,
	ENUM_SW_TEST_MODE_SIGMA_N = 0x6,
	ENUM_SW_TEST_MODE_SIGMA_HS20_R1 = 0x7,
	ENUM_SW_TEST_MODE_SIGMA_HS20_R2 = 0x8,
	ENUM_SW_TEST_MODE_SIGMA_PMF = 0x9,
	ENUM_SW_TEST_MODE_SIGMA_WMMPS = 0xA,
	ENUM_SW_TEST_MODE_SIGMA_AC_R2 = 0xB,
	ENUM_SW_TEST_MODE_SIGMA_NAN = 0xC,
	ENUM_SW_TEST_MODE_SIGMA_AC_AP = 0xD,
	ENUM_SW_TEST_MODE_SIGMA_N_AP = 0xE,
	ENUM_SW_TEST_MODE_SIGMA_WFDS = 0xF,
	ENUM_SW_TEST_MODE_SIGMA_WFD_R2 = 0x10,
	ENUM_SW_TEST_MODE_SIGMA_LOCATION = 0x11,
	ENUM_SW_TEST_MODE_SIGMA_TIMING_MANAGEMENT = 0x12,
	ENUM_SW_TEST_MODE_SIGMA_WMMAC = 0x13,
	ENUM_SW_TEST_MODE_SIGMA_VOICE_ENT = 0x14,
	ENUM_SW_TEST_MODE_SIGMA_AX = 0x15,
	ENUM_SW_TEST_MODE_SIGMA_AX_AP = 0x16,
	ENUM_SW_TEST_MODE_SIGMA_OCE = 0x17,
	ENUM_SW_TEST_MODE_SIGMA_BE = 0x18,
	ENUM_SW_TEST_MODE_SIGMA_WPA3 = 0x19,
	ENUM_SW_TEST_MODE_NUM
};

struct ESS_SCAN_RESULT_T {
	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint16_t u2SSIDLen;
	uint8_t aucSSID[PARAM_MAX_LEN_SSID];
};

struct WLAN_INFO {
	/* Scan Result */
	struct PARAM_BSSID_EX arScanResult[CFG_MAX_NUM_BSS_LIST];
	uint8_t *apucScanResultIEs[CFG_MAX_NUM_BSS_LIST];
	uint32_t u4ScanResultNum;

	struct ESS_SCAN_RESULT_T arScanResultEss[CFG_MAX_NUM_BSS_LIST];
	uint32_t u4ScanResultEssNum;
	uint32_t u4ScanDbgTimes1;
	uint32_t u4ScanDbgTimes2;
	uint32_t u4ScanDbgTimes3;
	uint32_t u4ScanDbgTimes4;

	/* IE pool for Scanning Result */
	uint8_t aucScanIEBuf[CFG_MAX_COMMON_IE_BUF_LEN];
	uint32_t u4ScanIEBufferUsage;

	OS_SYSTIME u4SysTime;

	/* connection parameter (for Ad-Hoc) */
	uint16_t u2BeaconPeriod;
	uint16_t u2AtimWindow;

	uint8_t eDesiredRates[PARAM_MAX_LEN_RATES];

	/* trigger parameter */
	enum ENUM_RSSI_TRIGGER_TYPE eRssiTriggerType;
	int32_t rRssiTriggerValue;

	/* Privacy Filter */
	enum ENUM_PARAM_PRIVACY_FILTER ePrivacyFilter;

	/* RTS Threshold */
	uint32_t eRtsThreshold;

	/* Network Type */
	uint8_t ucNetworkType[MAX_BSSID_NUM];

	/* Network Type In Use */
	uint8_t ucNetworkTypeInUse;

	/* Force enable/disable power save mode*/
	u_int8_t fgEnSpecPwrMgt;

};

struct BSS_INFO {
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct LINK_ENTRY rLinkEntryMld;
#endif
	enum ENUM_NETWORK_TYPE eNetworkType;

	/* Private data parameter for each NETWORK type usage. */
	uint32_t u4PrivateData;
	/* P2P network type has 3 network interface to distinguish. */

	/* Connected Flag used in AIS_NORMAL_TR */
	enum ENUM_PARAM_MEDIA_STATE eConnectionState;
	/* The Media State that report to HOST */
	enum ENUM_PARAM_MEDIA_STATE eConnectionStateIndicated;

	/* Current Operation Mode - Infra/IBSS */
	enum ENUM_OP_MODE eCurrentOPMode;
#if CFG_ENABLE_WIFI_DIRECT
	enum ENUM_OP_MODE eIntendOPMode;
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
	u_int8_t fgIsDfsActive;
#endif

	u_int8_t fgIsSwitchingChnl;
	u_int8_t fgIsInUse;	/* For CNM to assign BSS_INFO */
	u_int8_t fgIsNetActive;	/* TRUE if this network has been activated */

	uint8_t ucBssIndex;	/* BSS_INFO_T index */

	uint8_t ucSSIDLen;	/* Length of SSID */

#if CFG_ENABLE_WIFI_DIRECT
	/* For Hidden SSID usage. */
	enum ENUM_HIDDEN_SSID_TYPE eHiddenSsidType;
#endif

	uint8_t aucSSID[ELEM_MAX_LEN_SSID];	/* SSID used in this BSS */

	uint8_t aucBSSID[MAC_ADDR_LEN];	/* The BSSID of the associated BSS */

	/* Owned MAC Address used in this BSS */
	uint8_t aucOwnMacAddr[MAC_ADDR_LEN];

	uint8_t ucOwnMacIndex;	/* Owned MAC index used in this BSS */

	/* For Infra Mode, and valid only if
	 * eConnectionState == MEDIA_STATE_CONNECTED
	 */
	struct STA_RECORD *prStaRecOfAP;
	/* For IBSS/AP Mode, all known STAs in current BSS */
	struct LINK rStaRecOfClientList;

	/* For open Mode, BC/MC Tx wlan index, For STA, BC/MC Rx wlan index */
	uint8_t ucBMCWlanIndex;

	/* For AP Mode, BC/MC Tx wlan index, For STA, BC/MC Rx wlan index */
	uint8_t ucBMCWlanIndexS[MAX_KEY_NUM];
	uint8_t ucBMCWlanIndexSUsed[MAX_KEY_NUM];

	u_int8_t fgBcDefaultKeyExist;	/* Bc Transmit key exist or not */
	/* Bc default key idx, for STA, the Rx just set,
	 * for AP, the tx key id
	 */
	uint8_t ucBcDefaultKeyIdx;

	uint8_t wepkeyUsed[MAX_KEY_NUM];
	uint8_t wepkeyWlanIdx;	/* wlan index of the wep key */

	uint16_t u2CapInfo;	/* Change Detection */

	uint16_t u2BeaconInterval;	/* The Beacon Interval of this BSS */

	uint16_t u2ATIMWindow;	/* For IBSS Mode */

	/* For Infra Mode, it is the Assoc ID assigned by AP. */
	uint16_t u2AssocId;

	uint8_t ucDTIMPeriod;	/* For Infra/AP Mode */
	u_int8_t fgTIMPresent;

	/* For AP Mode, it is the DTIM value we should carried in
	 * the Beacon of next TBTT.
	 */
	uint8_t ucDTIMCount;

	/* Available PHY Type Set of this peer
	 * (This is deduced from received struct BSS_DESC)
	 */
	uint8_t ucPhyTypeSet;

	/* The Basic PHY Type Index, used to setup Phy Capability */
	uint8_t ucNonHTBasicPhyType;
	/* The configuration of AdHoc/AP Mode. e.g. 11g or 11b */
	uint8_t ucConfigAdHocAPMode;
	u_int8_t fgIsWepCipherGroup;

	/* For Infra/AP Mode, it is a threshold of Beacon Lost Count to
	 *  confirm connection was lost
	 */
	uint8_t ucBeaconTimeoutCount;

	/* For IBSS Mode, to keep use same BSSID
	 * to extend the life cycle of an IBSS
	 */
	u_int8_t fgHoldSameBssidForIBSS;
	/* For AP/IBSS Mode, it is used to indicate that Beacon is sending */
	u_int8_t fgIsBeaconActivated;

	struct MSDU_INFO *prBeacon;	/* For AP/IBSS Mode - Beacon Frame */

	/* For IBSS Mode - To indicate that we can reply ProbeResp Frame.
	 * In current TBTT interval
	 */
	u_int8_t fgIsIBSSMaster;

	/* From Capability Info. of AssocResp Frame
	 * AND of Beacon/ProbeResp Frame
	 */
	u_int8_t fgIsShortPreambleAllowed;
	/* Short Preamble is enabled in current BSS. */
	u_int8_t fgUseShortPreamble;
	/* Short Slot Time is enabled in current BSS. */
	u_int8_t fgUseShortSlotTime;

	/* Operational Rate Set of current BSS */
	uint16_t u2OperationalRateSet;
	uint16_t u2BSSBasicRateSet;	/* Basic Rate Set of current BSS */

	/* Used for composing Beacon Frame in AdHoc or AP Mode */
	uint8_t ucAllSupportedRatesLen;
	uint8_t aucAllSupportedRates[RATE_NUM_SW];
	/* TODO(Kevin): Number of associated clients */
	uint8_t ucAssocClientCnt;

	u_int8_t fgIsProtection;
	/* For Infra/AP/IBSS Mode, it is used to indicate if we support WMM in
	 * current BSS.
	 */
	u_int8_t fgIsQBSS;
	u_int8_t fgIsNetAbsent;	/* TRUE: BSS is absent, FALSE: BSS is present */
	OS_SYSTIME tmLastPresent;
	uint32_t u4PresentTime; /* in ms */

	/* Stop/Start Subqueue threshold for BSS */
	uint32_t u4TxStopTh;
	uint32_t u4TxStartTh;
	u_int8_t fgIs11B;

	uint32_t u4RsnSelectedGroupCipher;
	uint32_t u4RsnSelectedPairwiseCipher;
	uint32_t u4RsnSelectedAKMSuite;
	uint16_t u2RsnSelectedCapInfo;

	/*-------------------------------------------------------------------*/
	/* Operation mode change notification                                */
	/*-------------------------------------------------------------------*/
	/*Need to change OpMode channel width*/
	u_int8_t fgIsOpChangeChannelWidth;
	/* The OpMode channel width that we want to change to*/
	/* 0:20MHz, 1:40MHz, 2:80MHz, 3:160MHz 4:80+80MHz*/
	uint8_t ucOpChangeChannelWidth;

	/* Need to change OpMode RxNss */
	uint8_t fgIsOpChangeRxNss;
	/* The OpMode RxNss that we want to change to */
	uint8_t ucOpChangeRxNss;

	/* Need to change OpMode TxNss */
	uint8_t fgIsOpChangeTxNss;
	/* The OpMode TxNss that we want to change to */
	uint8_t ucOpChangeTxNss;

	PFN_OPMODE_NOTIFY_DONE_FUNC pfOpChangeHandler;

	uint8_t aucOpModeChangeState[OP_NOTIFY_TYPE_NUM];

	uint8_t aucOpModeChangeRetryCnt[OP_NOTIFY_TYPE_NUM];


	/*-------------------------------------------------------------------*/
	/* Power Management related information                              */
	/*-------------------------------------------------------------------*/
	struct PM_PROFILE_SETUP_INFO rPmProfSetupInfo;

	/* Support power save flag for the caller */
	uint32_t u4PowerSaveFlag;
	enum PARAM_POWER_MODE ePwrMode;

	/*-------------------------------------------------------------------*/
	/* WMM/QoS related information                                       */
	/*-------------------------------------------------------------------*/
	/* Used to detect the change of EDCA parameters. For AP mode,
	 * the value is used in WMM IE
	 */
	uint8_t ucWmmParamSetCount;

	struct AC_QUE_PARMS arACQueParms[WMM_AC_INDEX_NUM];
	/* For AP mode, broadcast the CWminLog2 */
	uint8_t aucCWminLog2ForBcast[WMM_AC_INDEX_NUM];
	/* For AP mode, broadcast the CWmaxLog2 */
	uint8_t aucCWmaxLog2ForBcast[WMM_AC_INDEX_NUM];
	/* For AP mode, broadcast the value */
	struct AC_QUE_PARMS arACQueParmsForBcast[WMM_AC_INDEX_NUM];
	uint8_t ucWmmQueSet;
#if (CFG_SUPPORT_802_11AX == 1)
	uint8_t ucMUEdcaUpdateCnt;
	/*
	 *  Store MU EDCA params for each ACs in BSS info
	 *  Use the same format as the update cmd for memory copy
	 */
	struct _CMD_MU_EDCA_PARAMS_T arMUEdcaParams[WMM_AC_INDEX_NUM];

	/* Spatial Reuse Parameter Set for the BSS */
	uint8_t ucSRControl;
	uint8_t ucNonSRGObssPdMaxOffset;
	uint8_t ucSRGObssPdMinOffset;
	uint8_t ucSRGObssPdMaxOffset;
	uint64_t u8SRGBSSColorBitmap;
	uint64_t u8SRGPartialBSSIDBitmap;
#if (CFG_SUPPORT_WIFI_6G == 1)
	u_int8_t fgIsHE6GPresent;
	u_int8_t fgIsCoHostedBssPresent;
#endif
#endif

#if (CFG_HW_WMM_BY_BSS == 1)
	u_int8_t fgIsWmmInited;
#endif

	/*-------------------------------------------------------------------*/
	/* 802.11n HT operation IE when (prStaRec->ucPhyTypeSet              */
	/* & PHY_TYPE_BIT_HT) is true. They have the same definition with    */
	/* fields of information element (CM)                                */
	/*-------------------------------------------------------------------*/
	enum ENUM_BAND eBand;
	uint8_t ucPrimaryChannel;
	uint8_t ucHtOpInfo1;
	uint16_t u2HtOpInfo2;
	uint16_t u2HtOpInfo3;
	uint8_t ucOpRxNss; /* Own OP RxNss */
	uint8_t ucOpTxNss; /* Own OP TxNss */

	/*-------------------------------------------------------------------*/
	/* 802.11ac VHT operation IE when (prStaRec->ucPhyTypeSet            */
	/* & PHY_TYPE_BIT_VHT) is true. They have the same definition with   */
	/* fields of information element (EASON)                             */
	/*-------------------------------------------------------------------*/
	/*-------------------------------------------------------------------*/
	/* Note that FW will use ucVhtChannelWidth, ucVhtChannelFrequencyS1  */
	/* and ucVhtChannelFrequencyS2 as general RLM parameters regardless  */
	/* of VHT, HE or EHT. Hence, driver shall update these 3 parameters  */
	/* by reference to the spec of VHT IE even in 6G channels that shall */
	/* not use VHT IE.                                                   */
	/*-------------------------------------------------------------------*/
#if 1				/* CFG_SUPPORT_802_11AC */
	uint8_t ucVhtChannelWidth;
	uint8_t ucVhtChannelFrequencyS1;
	uint8_t ucVhtChannelFrequencyS2;
	uint16_t u2VhtBasicMcsSet;
#endif

#if (CFG_SUPPORT_802_11AX == 1)
	uint8_t ucHeOpParams[HE_OP_BYTE_NUM];
	uint8_t ucBssColorInfo;
	uint16_t u2HeBasicMcsSet;
#if (CFG_SUPPORT_WIFI_6G == 1)
	struct _6G_OPER_INFOR_T r6gOperInfor;
#endif
#endif

#if (CFG_SUPPORT_802_11BE == 1)
	uint8_t  ucEhtOpParams;
	uint32_t u4BasicEhtMcsNssSet;
	uint8_t  fgIsEhtOpPresent;
	uint8_t  fgIsEhtDscbPresent;
	uint8_t  ucEhtCtrl;
	uint8_t  ucEhtCcfs0;
	uint8_t  ucEhtCcfs1;
	uint16_t u2EhtDisSubChanBitmap;
	struct EHT_OP_INFO rEhtOpInfo;
	struct EHT_DSCP_INFO rEhtDscpInfo;
#endif
#if (CFG_SUPPORT_802_11V_MBSSID == 1)
	uint8_t ucMaxBSSIDIndicator;
	uint8_t ucMBSSIDIndex;
#endif

	/*-------------------------------------------------------------------*/
	/* Required protection modes (CM)                                    */
	/*-------------------------------------------------------------------*/
	u_int8_t fgErpProtectMode;
	enum ENUM_HT_PROTECT_MODE eHtProtectMode;
	enum ENUM_GF_MODE eGfOperationMode;
	enum ENUM_RIFS_MODE eRifsOperationMode;

	u_int8_t fgObssErpProtectMode;	/* GO only */
	enum ENUM_HT_PROTECT_MODE eObssHtProtectMode;	/* GO only */
	enum ENUM_GF_MODE eObssGfOperationMode;	/* GO only */
	u_int8_t fgObssRifsOperationMode;	/* GO only */

	/*-------------------------------------------------------------------*/
	/* OBSS to decide if 20/40M bandwidth is permitted.                  */
	/* The first member indicates the following channel list length.     */
	/*-------------------------------------------------------------------*/
	u_int8_t fgAssoc40mBwAllowed;
	u_int8_t fg40mBwAllowed;
	enum ENUM_CHNL_EXT eBssSCO;	/* Real setting for HW
					 * 20/40M AP mode will always set 40M,
					 * but its OP IE can be changed.
					 */
	uint8_t auc2G_20mReqChnlList[CHNL_LIST_SZ_2G + 1];
	uint8_t auc2G_NonHtChnlList[CHNL_LIST_SZ_2G + 1];
	uint8_t auc2G_PriChnlList[CHNL_LIST_SZ_2G + 1];
	uint8_t auc2G_SecChnlList[CHNL_LIST_SZ_2G + 1];

	uint8_t auc5G_20mReqChnlList[CHNL_LIST_SZ_5G + 1];
	uint8_t auc5G_NonHtChnlList[CHNL_LIST_SZ_5G + 1];
	uint8_t auc5G_PriChnlList[CHNL_LIST_SZ_5G + 1];
	uint8_t auc5G_SecChnlList[CHNL_LIST_SZ_5G + 1];

	/*-------------------------------------------------------------------*/
	/* Scan related information                                          */
	/*-------------------------------------------------------------------*/
	/* Set scanning MAC OUI */
	u_int8_t fgIsScanOuiSet;
	uint8_t ucScanOui[MAC_OUI_LEN];

	struct TIMER rObssScanTimer;
	uint16_t u2ObssScanInterval;	/* in unit of sec */

	u_int8_t fgObssActionForcedTo20M;	/* GO only */
	u_int8_t fgObssBeaconForcedTo20M;	/* GO only */

	/*--------------------------------------------------------------------*/
	/* HW Related Fields (Kevin)                                          */
	/*--------------------------------------------------------------------*/
	/* The default rate code copied to MAC TX Desc */
	uint16_t u2HwDefaultFixedRateCode;
	uint16_t u2HwLPWakeupGuardTimeUsec;

	uint8_t ucBssFreeQuota;	/* The value is updated from FW  */

	uint16_t u2DeauthReason;

#if CFG_SUPPORT_TDLS
	u_int8_t fgTdlsIsProhibited;
	u_int8_t fgTdlsIsChSwProhibited;
#endif
#if CFG_SUPPORT_TDLS_P2P_AUTO
	struct sta_tdls_info *prTdlsHash[STA_TDLS_HASH_SIZE + 1];
	uint64_t ulLastUpdate;
	int32_t i4TdlsLastRx;
	int32_t i4TdlsLastTx;
	struct TIMER rTdlsStateTimer;
#endif

	/*link layer statistics */
	struct WIFI_WMM_AC_STAT arLinkStatistics[WMM_AC_INDEX_NUM];

	uint32_t u4CoexPhyRateLimit;
	enum ENUM_COEX_MODE eCoexMode;

	u_int8_t fgIsGranted;
	enum ENUM_BAND eBandGranted;
	uint8_t ucPrimaryChannelGranted;
	struct PARAM_CUSTOM_ACL rACL;
#if CFG_SUPPORT_802_11W
	/* AP PMF */
	struct AP_PMF_CFG rApPmfCfg;
	uint8_t fgBipKeyInstalled;
#endif

#if CFG_AP_80211KVR_INTERFACE
	uint32_t u4ChanUtil;
	int32_t i4NoiseHistogram;
	struct T_MULTI_AP_STA_UNASSOC_METRICS
		arUnAssocSTA[SAP_UNASSOC_METRICS_STA_MAX];
	struct TIMER rUnassocStaMeasureTimer;
#endif

#if (CFG_SUPPORT_HE_ER == 1)
	uint8_t ucErMode;
#endif

	uint8_t ucCountryIELen;
	uint8_t aucCountryStr[3];
	uint8_t aucSubbandTriplet[253];
	enum ENUM_IFTYPE eIftype;

#if ((CFG_SUPPORT_TWT == 1) && (CFG_SUPPORT_TWT_HOTSPOT == 1))
	enum _ENUM_TWT_HOTSPOT_RESPONDER_STATE_T aeTWTRespState;
	uint8_t twt_flow_id_bitmap;
	struct LINK twt_sch_link; /* twt sch-link */
	struct _TWT_HOTSPOT_STA_NODE arTWTSta[TWT_MAX_FLOW_NUM];
#endif

	/* Buffer for WPA2 PMKID */
	/* The PMKID cache lifetime is expire by media_disconnect_indication */
	struct LINK rPmkidCache;

	enum ENUM_MBMC_BN eHwBandIdx;
	enum ENUM_MBMC_BN eBackupHwBandIdx; /* backup to restore hw band */
	/* TODO other code block  use this in uni_cmd_event.c w/o define */
#if (CFG_SUPPORT_802_11BE_MLO == 1) || defined(CFG_SUPPORT_UNIFIED_COMMAND)
	uint8_t ucGroupMldId;
	uint8_t ucOwnMldId;
	uint8_t ucLinkIndex;
#endif

	uint16_t u2MaxIdlePeriod;
	uint8_t ucIdleOptions;

#if CFG_SUPPORT_DFS
	struct TIMER rCsaTimer;
	struct SWITCH_CH_AND_BAND_PARAMS CSAParams;
	uint8_t fgHasStopTx;
	uint8_t ucVhtChannelWidthBeforeCsa;
	uint8_t fgIsAisSwitchingChnl;
#endif

	u_int8_t fgEnableH2E;

#if CFG_FAST_PATH_SUPPORT
	struct FAST_PATH_INFO rFastPathInfo;
#endif


#if CFG_SUPPORT_LLS
	uint32_t u4RxMpduAc[STATS_LLS_WIFI_AC_MAX];
#endif
};

#if (CFG_SUPPORT_802_11BE_MLO == 1)
struct MLD_BSS_INFO {
	struct LINK rBssList;
	struct LINK rMldStaRecOfClientList;

	u_int8_t fgIsInUse;
	uint8_t ucOmacIdx;
	uint8_t ucGroupMldId;
	uint8_t ucOmRemapIdx;
	uint8_t aucOwnMldAddr[MAC_ADDR_LEN];
	uint8_t ucBssBitmap;
	uint8_t ucHwBandBitmap; /* BIT(i), i = prBssInfo->eBandIdx */
};
#endif

struct NEIGHBOR_AP {
	struct LINK_ENTRY rLinkEntry;
	uint8_t aucBssid[MAC_ADDR_LEN];
	uint8_t fgHT:1;
	uint8_t fgSameMD:1;
	uint8_t fgRmEnabled:1;
	uint8_t fgFromBtm:1;
	uint8_t fgQoS:1;
	uint8_t fgHe:1;
	uint8_t fgVht:1;
	uint8_t fgEht:1;
	u_int8_t fgPrefPresence;
	uint8_t ucPreference;
	uint8_t ucChannel;
	uint64_t u8TermTsf;
	enum ENUM_BAND eBand;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	uint8_t fgIsMld;
	uint8_t aucMldAddr[MAC_ADDR_LEN];
	uint16_t u2ValidLinks; /* bitmap of valid MLO link IDs */
#endif
};

struct BOW_SPECIFIC_BSS_INFO {
	uint16_t u2Reserved;	/* Reserved for Data Type Check */
};

#if CFG_SUPPORT_NAN
struct _NAN_SPECIFIC_BSS_INFO_T {
	uint8_t ucBssIndex;
	uint16_t u2Reserved; /* Reserved for Data Type Check */
	uint32_t u4ModuleUsed;
	uint8_t aucClusterId[MAC_ADDR_LEN];
	struct _NAN_ATTR_MASTER_INDICATION_T rMasterIndAttr;

	/* struct NAN_CRB_NEGO_CTRL_T rNanSchNegoCtrl;
	 * struct NAN_PEER_SCHEDULE_RECORD_T
	 * arNanPeerSchedRecord[NAN_MAX_CONN_CFG];
	 * struct NAN_TIMELINE_MGMT_T rNanTimelineMgmt;
	 * struct NAN_SCHEDULER_T rNanScheduler;
	 */
};
#endif

#if CFG_SLT_SUPPORT
struct SLT_INFO {

	struct BSS_DESC *prPseudoBssDesc;
	uint16_t u2SiteID;
	uint8_t ucChannel2G4;
	uint8_t ucChannel5G;
	u_int8_t fgIsDUT;
	uint32_t u4BeaconReceiveCnt;
	/* ///////Deprecated///////// */
	struct STA_RECORD *prPseudoStaRec;
};
#endif

struct WLAN_TABLE {
	uint8_t ucUsed;
	uint8_t ucBssIndex;
	uint8_t ucKeyId;
	uint8_t ucPairwise;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint8_t ucStaIndex;
};

/* Major member variables for WiFi FW operation.
 * Variables within this region will be ready for
 * access after WIFI function is enabled.
 */
struct WIFI_VAR {
	u_int8_t fgIsRadioOff;

	u_int8_t fgIsEnterD3ReqIssued;

	u_int8_t fgDebugCmdResp;

	/* Common connection settings start */
	/* Used for AP mode for desired channel and bandwidth */
	uint16_t u2CountryCode;
	uint8_t uc2G4BandwidthMode;	/* 20/40M or 20M only *//* Not used */
	uint8_t uc5GBandwidthMode;	/* 20/40M or 20M only *//* Not used */
	uint8_t uc6GBandwidthMode;	/* 20/40M or 20M only *//* Not used */
	/* Support AP Selection */
	struct LINK_MGMT rBlackList;
#if CFG_SUPPORT_MBO
	struct PARAM_BSS_DISALLOWED_LIST rBssDisallowedList;
#endif
	enum ENUM_PARAM_PHY_CONFIG eDesiredPhyConfig;
	/* Common connection settings end */

	struct SCAN_INFO rScanInfo;
	struct ROAMING_IDLE_INFO rRoamSlotInfo;

	struct AIS_FSM_INFO rAisFsmInfo[KAL_AIS_NUM];
	struct AIS_FSM_INFO *prDefaultAisFsmInfo;

	struct RTT_INFO rRttInfo;

	enum ENUM_PWR_STATE aePwrState[MAX_BSSID_NUM + 1];

	struct BSS_INFO arBssInfoPool[MAX_BSSID_NUM];

	struct BSS_INFO rP2pDevInfo;

#if CFG_SUPPORT_NAN
	struct _NAN_SPECIFIC_BSS_INFO_T
		*aprNanSpecificBssInfo[NAN_BSS_INDEX_NUM];
#endif

#if CFG_ENABLE_WIFI_DIRECT
	struct P2P_CONNECTION_SETTINGS *prP2PConnSettings[BSS_P2P_NUM];

	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo[BSS_P2P_NUM];

/* P_P2P_FSM_INFO_T prP2pFsmInfo; */

	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo;

	/* Currently we only support 2 p2p interface. */
	struct P2P_ROLE_FSM_INFO *aprP2pRoleFsmInfo[BSS_P2P_NUM];

#if	CFG_ENABLE_PER_STA_STATISTICS_LOG
	struct PARAM_GET_STA_STATISTICS *prP2pQueryStaStatistics[BSS_P2P_NUM];
#endif

#endif				/* CFG_ENABLE_WIFI_DIRECT */

#if CFG_ENABLE_BT_OVER_WIFI
	struct BOW_SPECIFIC_BSS_INFO rBowSpecificBssInfo;
	struct BOW_FSM_INFO rBowFsmInfo;
#endif				/* CFG_ENABLE_BT_OVER_WIFI */

	struct WLAN_TABLE arWtbl[WTBL_SIZE];

	struct DEAUTH_INFO arDeauthInfo[MAX_DEAUTH_INFO_COUNT];

	/* Current Wi-Fi Settings and Flags */
	uint8_t aucPermanentAddress[MAC_ADDR_LEN];
	uint8_t aucMacAddress[KAL_AIS_NUM][MAC_ADDR_LEN];
	uint8_t aucDeviceAddress[MAC_ADDR_LEN];
	uint8_t aucInterfaceAddress[KAL_P2P_NUM][MAC_ADDR_LEN];

	uint8_t ucAvailablePhyTypeSet;

	/* Basic Phy Type used by SCN according
	 * to the set of Available PHY Types
	 */
	enum ENUM_PHY_TYPE_INDEX eNonHTBasicPhyType2G4;

	enum ENUM_PARAM_PREAMBLE_TYPE ePreambleType;
	enum ENUM_REGISTRY_FIXED_RATE eRateSetting;

	u_int8_t fgIsShortSlotTimeOptionEnable;
	/* User desired setting, but will honor the capability of AP */

	u_int8_t fgEnableJoinToHiddenSSID;
	u_int8_t fgSupportWZCDisassociation;

#if CFG_SUPPORT_WFD
	struct WFD_CFG_SETTINGS rWfdConfigureSettings;
#endif

#if CFG_SLT_SUPPORT
	struct SLT_INFO rSltInfo;
#endif

	uint8_t aucMediatekOuiIE[64];
	uint16_t u2MediatekOuiIELen;

	/* Feature Options */
	uint8_t ucQoS;

	uint8_t ucStaHt;
	uint8_t ucStaVht;
#if (CFG_SUPPORT_802_11AX == 1)
	uint8_t ucStaHe;
	uint8_t ucApHe;
	uint8_t ucP2pGoHe;
	uint8_t ucP2pGcHe;
	uint8_t ucVcoreBoostEnable;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	uint8_t ucStaEht;
	uint8_t ucApEht;
	uint8_t ucP2pGoEht;
	uint8_t ucP2pGcEht;
	uint8_t ucEhtAmsduInAmpduTx;
	uint8_t ucEhtAmsduInAmpduRx;
	uint8_t ucStaEhtBfee;
	uint8_t ucEhtOMCtrl;
	uint8_t ucStaEht242ToneRUWt20M;
	uint8_t ucEhtNDP4xLTF3dot2usGI;
	uint8_t ucEhtSUBfer;
	uint8_t ucEhtSUBfee;
	uint8_t ucEhtBfeeSSLeEq80m;
	uint8_t ucEhtBfee160m;
	uint8_t ucEhtBfee320m;
	uint8_t	ucEhtNG16SUFeedback;
	uint8_t	ucEhtNG16MUFeedback;
	uint8_t ucEhtCodebook75MuFeedback;
	uint8_t	ucEhtTrigedSUBFFeedback;
	uint8_t	ucEhtTrigedMUBFPartialBWFeedback;
	uint8_t ucEhtTrigedCQIFeedback;
	uint8_t ucEhtPartialBwDLMUMIMO;
	uint8_t ucEhtMUPPDU4xEHTLTFdot8usGI;
	uint8_t ucEhtNonTrigedCQIFeedback;
	uint8_t ucEhtTx1024QAM4096QAMLe242ToneRU;
	uint8_t ucEhtRx1024QAM4096QAMLe242ToneRU;
	uint8_t ucEhtCommonNominalPktPadding;
	uint8_t ucEhtMaxLTFNum;
	uint8_t ucEhtMCS15;
	uint8_t ucEhtDup6G;
	uint8_t ucEht20MRxNDPWiderBW;
	uint8_t ucPresetLinkId;
	uint8_t ucMldLinkMax;
	uint8_t ucStaMldLinkMax;
	uint8_t ucP2pMldLinkMax;
	uint8_t ucApMldMainLinkIdx;
	uint8_t ucStaMldMainLinkIdx;
	uint8_t ucStaPreferMldAddr;
	uint8_t ucEnableMlo;
	uint8_t ucMaxSimultaneousLinks;
	uint8_t aucMloP2pPreferFreq[WLAN_CFG_VALUE_LEN_MAX];
	uint8_t ucMlProbeRetryLimit;
	struct LINK_MGMT rMldBlockList;
	uint8_t ucMldRetryCount;
	uint32_t u4AisEHTNumber;
#endif
	uint8_t ucApHt;
	uint8_t ucApVht;
	uint8_t ucP2pGoHt;
	uint8_t ucP2pGoVht;
	uint8_t ucP2pGcHt;
	uint8_t ucP2pGcVht;

	/* NIC capability from FW event*/
	uint8_t ucHwNotSupportAC;

	uint8_t ucAmpduTx;
	uint8_t ucAmpduRx;
	uint8_t ucAmsduInAmpduTx;
	uint8_t ucAmsduInAmpduRx;
	uint8_t ucHtAmsduInAmpduTx;
	uint8_t ucHtAmsduInAmpduRx;
	uint8_t ucVhtAmsduInAmpduTx;
	uint8_t ucVhtAmsduInAmpduRx;
#if (CFG_SUPPORT_802_11AX == 1)
	uint8_t ucHeAmsduInAmpduTx;
	uint8_t ucHeAmsduInAmpduRx;
	uint8_t ucHeCertForceAmsdu;
	uint8_t ucTrigMacPadDur;
	uint8_t fgEnableSR;
	uint8_t ucStaHeBfee;
	uint8_t ucStaHeSuBfer;
	uint8_t ucMaxAmpduLenExp;
	uint8_t ucHeOMCtrl;
	uint8_t ucRxCtrlToMutiBss;
	uint8_t ucStaHePpRx;
	uint8_t ucHeDynamicSMPS;
	uint8_t ucHeHTC;
#endif
	uint8_t ucBtmCap;
#if (CFG_SUPPORT_TWT == 1)
	uint8_t ucTWTRequester;
	uint8_t ucTWTResponder;
	uint8_t ucTWTStaBandBitmap;
#endif
#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	uint8_t ucTWTHotSpotSupport;
#endif
#if (CFG_TWT_SMART_STA == 1)
	uint8_t ucTWTSmartSta;
#endif
#if (CFG_SUPPORT_BTWT == 1)
	uint8_t ucBTWTSupport;
#endif
	uint8_t ucTspec;
	uint8_t ucUapsd;
	uint8_t ucStaUapsd;
	uint8_t ucApUapsd;
	uint8_t ucP2pUapsd;

	uint8_t ucTxShortGI;
	uint8_t ucRxShortGI;
	uint8_t ucTxLdpc;
	uint8_t ucRxLdpc;
	uint8_t ucTxStbc;
	uint8_t ucRxStbc;
	uint8_t ucRxStbcNss;
	uint8_t ucTxGf;
	uint8_t ucRxGf;

	uint8_t ucMCS32;

	uint8_t ucTxopPsTx;
	uint8_t ucSigTaRts;
	uint8_t ucDynBwRts;

	uint8_t ucStaHtBfee;
	uint8_t ucStaVhtBfee;
	uint8_t ucStaHtBfer;
	uint8_t ucStaVhtBfer;
	uint8_t ucStaVhtMuBfee;
	uint8_t fgForceSTSNum;

	uint8_t ucDataTxDone;
	uint8_t ucDataTxRateMode;
	uint32_t u4DataTxRateCode;

	uint8_t ucApWpsMode;
	uint8_t ucApChannel;
	uint16_t u2ApFreq;
	uint8_t ucApAcsChannel[3];

	uint8_t ucApSco;
	uint8_t ucP2pGoSco;

	uint8_t ucStaBandwidth;
	uint8_t ucSta5gBandwidth;
	uint8_t ucSta2gBandwidth;
	uint8_t ucSta6gBandwidth;
	uint8_t ucApBandwidth;
	uint8_t ucAp2gBandwidth;
	uint8_t ucAp5gBandwidth;
	uint8_t ucAp6gBandwidth;
	uint8_t ucP2p5gBandwidth;
	uint8_t ucP2p2gBandwidth;
	uint8_t ucP2p6gBandwidth;

	/* If enable, AP channel bandwidth Channel
	 * Center Frequency Segment 0/1
	 */
	/* and secondary channel offset will align wifi.cfg */
	/* Otherwise align cfg80211 */
	uint8_t ucApChnlDefFromCfg;

	/*
	 * According TGn/TGac 4.2.44, AP should not connect
	 * with TKIP client with HT/VHT capabilities. We leave
	 * a wifi.cfg item for user to decide whether to
	 * enable HT/VHT capabilities in that case
	 */
	uint8_t ucApAllowHtVhtTkip;

	uint8_t ucNSS;
	uint8_t fgSta1NSS; /* Less or euqal than ucNss */
	uint8_t ucAp6gNSS; /* Less or euqal than ucNss */
	uint8_t ucAp5gNSS; /* Less or euqal than ucNss */
	uint8_t ucAp2gNSS; /* Less or euqal than ucNss */
	uint8_t ucGo6gNSS; /* Less or euqal than ucNss */
	uint8_t ucGo5gNSS; /* Less or euqal than ucNss */
	uint8_t ucGo2gNSS; /* Less or euqal than ucNss */

	uint8_t ucRxMaxMpduLen;
	uint8_t ucRxQuotaInfoEn;
	uint32_t u4HtTxMaxAmsduInAmpduLen;
	uint32_t u4VhtTxMaxAmsduInAmpduLen;
	uint32_t u4TxMaxAmsduInAmpduLen;

	uint8_t ucTxBaSize;
	uint8_t ucRxHtBaSize;
	uint8_t ucRxVhtBaSize;
#if (CFG_SUPPORT_802_11AX == 1)
	uint16_t u2RxHeBaSize;
	uint16_t u2TxHeBaSize;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	uint16_t u2RxEhtBaSize;
	uint16_t u2TxEhtBaSize;
#endif
	u_int8_t fgMoveWinOnMissingLast;
	uint16_t u2BaExtSize;
	uint32_t u4BaVerboseLogging;

	uint8_t ucThreadScheduling;
	uint8_t ucThreadPriority;
	int8_t cThreadNice;

	uint8_t ucTcRestrict;
	uint32_t u4MaxTxDeQLimit;
	uint8_t ucAlwaysResetUsedRes;

	uint32_t u4NetifStopTh;
	uint32_t u4NetifStartTh;
	struct PARAM_GET_CHN_INFO rChnLoadInfo;

#if CFG_SUPPORT_MTK_SYNERGY
	uint8_t ucMtkOui;
	uint32_t u4MtkOuiCap;
	uint8_t aucMtkFeature[4];
	u_int8_t ucGbandProbe256QAM;
#endif
#if CFG_SUPPORT_VHT_IE_IN_2G
	uint8_t ucVhtIeIn2g;
#endif
	uint8_t fgCsaInProgress;
	uint8_t fgCsaWaitTrigger;
	uint8_t ucChannelSwitchMode;
	uint8_t ucNewOperatingClass;
	uint8_t ucNewChannelNumber;
	uint8_t ucChannelSwitchCount;
	uint8_t ucSecondaryOffset;
	uint8_t ucNewChannelWidth;
	uint8_t ucNewChannelS1;
	uint8_t ucNewChannelS2;

	uint32_t u4HifIstLoopCount;
	uint32_t u4Rx2OsLoopCount;
	uint32_t u4HifTxloopCount;
	uint32_t u4TxRxLoopCount;
	uint32_t u4TxFromOsLoopCount;
	uint32_t u4TxIntThCount;

	uint32_t au4TcPageCount[TC_NUM];
	uint32_t au4TcPageCountPle[TC_NUM];
	uint8_t ucExtraTxDone;
	uint8_t ucTxDbg;

	uint8_t ucCmdRsvResource;
	uint32_t u4MgmtQueueDelayTimeout;

	uint32_t u4StatsLogTimeout;
	uint32_t u4StatsLogDuration;
	uint8_t ucDhcpTxDone;
	uint8_t ucArpTxDone;

	uint8_t ucMacAddrOverride;
	uint8_t aucMacAddrStr[WLAN_CFG_VALUE_LEN_MAX];

	uint8_t ucCtiaMode;
	uint8_t ucTpTestMode;
	uint8_t ucSigmaTestMode;
	enum ENUM_CNM_DBDC_MODE eDbdcMode;
	u_int8_t fgDbDcModeEn;
#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
	uint8_t ucDbdcP2pLisEn;
	uint32_t u4DbdcP2pLisSwDelayTime;
#endif
	uint8_t u4ScanCtrl;
	uint8_t ucScanChannelListenTime;

#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
	uint8_t ucEfuseBufferModeCal;
#endif
	uint8_t ucCalTimingCtrl;
	uint8_t ucWow;
	uint8_t ucOffload;
	uint8_t ucAdvPws; /* enable LP multiple DTIM function, default enable */
	uint8_t ucWowOnMdtim; /* multiple DTIM if WOW enable, default 1 */
	uint8_t ucWowOffMdtim; /* multiple DTIM if WOW disable, default 3 */
	uint8_t ucEapolSuspendOffload; /* Suspend mode eapol offload, def:0 */
	/* set wow detect type, default 1 (magic packet) */
	uint8_t ucWowDetectType;
	/* set wow detect type extension, default 1 (port) */
	uint16_t u2WowDetectTypeExt;
	uint32_t u4TxHangFullDumpMode;
	uint8_t ucMobileLikeSuspend; /* allow mobile like suspend */
#if (CFG_WFD_SCC_BALANCE_SUPPORT == 1)
	uint32_t u4WfdSccBalanceEnable;
	uint32_t u4WfdSccBalanceMode; /* 0: auto mode, 1: force mode */
	int32_t i4BssCount[MAX_BSSID_NUM];
#endif
	uint32_t u4DrvOwnMode; /* 0: default, 1: delay 10ms */
	uint8_t u4SwTestMode;
	uint8_t	ucCtrlFlagAssertPath;
	uint8_t	ucCtrlFlagDebugLevel;
	uint32_t u4WakeLockRxTimeout;
	uint32_t u4WakeLockThreadWakeup;
	uint32_t u4RegP2pIfAtProbe; /* register p2p interface during probe */
	uint8_t ucRegP2pMode;
	/* p2p group interface use the same mac addr as p2p device interface */
	uint8_t ucP2pShareMacAddr;
	uint8_t ucSmartRTS;

	uint32_t u4UapsdAcBmp;
	uint32_t u4MaxSpLen;
	uint32_t u4P2pUapsdAcBmp;
	uint32_t u4P2pMaxSpLen;
	/* 0: enable online scan, non-zero: disable online scan */
	uint32_t fgDisOnlineScan;
	uint32_t fgDisBcnLostDetection;
	uint32_t fgDisAgingLostDetection;
	uint32_t fgDisRoaming;		/* 0:enable roaming 1:disable */
	uint32_t u4AisRoamingNumber;
	uint32_t fgEnArpFilter;

	uint8_t	uDeQuePercentEnable;
	uint32_t u4DeQuePercentVHT80Nss1;
	uint32_t u4DeQuePercentVHT40Nss1;
	uint32_t u4DeQuePercentVHT20Nss1;
	uint32_t u4DeQuePercentHT40Nss1;
	uint32_t u4DeQuePercentHT20Nss1;

	uint32_t u4PerfMonUpdatePeriod;
	uint32_t u4PerfMonTpTh[PERF_MON_TP_MAX_THRESHOLD];
#if CFG_DYNAMIC_RFB_ADJUSTMENT
	uint32_t u4RfbBoostTpTh[PERF_MON_RFB_MAX_THRESHOLD];
	uint32_t u4RfbInUseCnt[PERF_MON_RFB_MAX_THRESHOLD];
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */
#if CFG_SUPPORT_MCC_BOOST_CPU
	uint32_t u4MccBoostTputLvTh;
	uint32_t u4MccBoostPresentTime;
#endif /* CFG_SUPPORT_MCC_BOOST_CPU */
	u_int8_t fgBoostCpuEn;
	uint32_t u4BoostCpuTh;
#if CFG_SUPPORT_LITTLE_CPU_BOOST
	uint32_t u4BoostLittleCpuTh;
#endif /* CFG_SUPPORT_LITTLE_CPU_BOOST */
	uint32_t au4CpuBoostMinFreq;
	uint8_t fgIsBoostCpuThAdjustable;

	uint32_t u4PerfMonPendingTh;
	uint32_t u4PerfMonUsedTh;

	u_int8_t fgTdlsBufferSTASleep; /* Support TDLS 5.5.4.2 optional case */
	u_int8_t fgTdlsDisable;
	u_int8_t fgChipResetRecover;
	enum PARAM_POWER_MODE ePowerMode;

	u_int8_t fgNvramCheckEn; /* nvram checking in scan result*/

	uint8_t fgRstRecover;
	u_int8_t fgEnableSerL0;
	enum ENUM_FEATURE_OPTION_IN_SER eEnableSerL0p5;
	enum ENUM_FEATURE_OPTION_IN_SER eEnableSerL1;

#if CFG_SUPPORT_SPE_IDX_CONTROL
	u_int8_t ucSpeIdxCtrl;	/* 0: WF0, 1: WF1, 2: duplicate */
#endif

#if CFG_SUPPORT_LOWLATENCY_MODE
	uint8_t ucLowLatencyModeScan;
	uint8_t ucLowLatencyModeReOrder;
	uint8_t ucLowLatencyModePower;
	uint8_t ucLowLatencyPacketPriority;
	uint8_t ucLowLatencyCmdData;
	uint8_t ucLowLatencyCmdDataAllPacket;
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */
#if CFG_SUPPORT_IDC_CH_SWITCH
	uint8_t ucChannelSwtichColdownTime;
	uint8_t fgCrossBandSwitchEn;
#endif
#if CFG_SUPPORT_PERF_IND
	u_int8_t fgPerfIndicatorEn;
#endif

	u_int8_t fgSwRxReordering;
	u_int8_t fgFlushRxReordering;
	u_int8_t fgRxIcvErrDbg;
	union {
		uint32_t u4TxRxDescDump;
		struct {
			uint32_t fgDumpTxD: 1;        /* 0x01 */
			uint32_t fgDumpTxDmad: 1;     /* 0x02 */
			uint32_t fgDumpTxP: 1;        /* 0x04 */
			uint32_t fgDumpReserved: 1;

			uint32_t fgDumpRxD: 1;        /* 0x10 */
			uint32_t fgDumpRxDmad: 1;     /* 0x20 */
			uint32_t fgDumpRxDsegment: 1; /* 0x40 */
			uint32_t fgDumpRxEvt: 1;      /* 0x80 */
		};
	};
	uint32_t u4BaShortMissTimeoutMs;
	uint32_t u4BaIotApMissTimeoutMs;
	uint32_t u4BaMissTimeoutMs;

	/* Tx Msdu Queue method */
	uint8_t ucTxMsduQueue;
	uint8_t ucTxMsduQueueInit;
	uint32_t u4TxHifRes;

	uint32_t u4MTU; /* net device maximum transmission unit */
#if CFG_SUPPORT_RX_GRO
	uint32_t ucGROFlushTimeout; /* Flush packet timeout (ms) */
	uint32_t ucGROEnableTput; /* Threshold of enable GRO Tput */
#endif /* CFG_SUPPORT_RX_GRO */
#if CFG_SUPPORT_DISABLE_DATA_DDONE_INTR
	uint32_t u4TputThresholdMbps;
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR */

#if CFG_SUPPORT_IOT_AP_BLACKLIST
	uint8_t fgEnDefaultIotApRule;
#endif
	uint8_t ucMsduReportTimeout;
	uint8_t ucMsduReportTimeoutSerTime;

#if CFG_SUPPORT_DATA_STALL
	uint32_t u4PerHighThreshole;
	uint32_t u4TxLowRateThreshole;
	uint32_t u4RxLowRateThreshole;
	uint32_t u4ReportEventInterval;
	uint32_t u4TrafficThreshold;
#endif

#if CFG_SUPPORT_HE_ER
	uint8_t u4ExtendedRange;
	u_int8_t fgErTx;
	u_int8_t fgErRx;
	u_int8_t fgErSuRx;
#endif
#if (CFG_SUPPORT_BSS_MAX_IDLE_PERIOD == 1)
	u_int8_t fgBssMaxIdle;
	uint16_t u2BssMaxIdlePeriod;
#endif
#if CFG_SUPPORT_SMART_GEAR
	uint8_t ucSGCfg;
	uint8_t ucSG24GFavorANT;
	uint8_t ucSG5GFavorANT;
#endif
#if (CFG_SUPPORT_P2PGO_ACS == 1)
	uint8_t ucP2pGoACS;
#endif
	uint8_t fgReuseRSNIE;

	uint32_t u4DiscoverTimeout;
	uint32_t u4InactiveTimeout;
	uint8_t ucDisallowBand2G;
	uint8_t ucDisallowBand5G;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucDisallowBand6G;
#endif
	uint32_t u4BtmDelta;
	uint32_t u4BtmDisTimerThreshold;
#if ARP_MONITER_ENABLE
	uint32_t uArpMonitorNumber;
	uint32_t uArpMonitorRxPktNum;
	uint8_t uArpMonitorCriticalThres;
	uint8_t ucArpMonitorUseRule; /* 0:old rule, 1:new rule */
#endif /* ARP_MONITER_ENABLE */
#if CFG_RFB_TRACK
	u_int8_t fgRfbTrackEn;
	uint32_t u4RfbTrackInterval;
	uint32_t u4RfbTrackTimeout;
#endif /* CFG_RFB_TRACK */
#if CFG_SUPPORT_SCAN_NO_AP_RECOVERY
	uint8_t ucScanNoApRecover;
	uint8_t ucScanNoApRecoverTh;
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

#if (CFG_COALESCING_INTERRUPT == 1)
	uint16_t u2CoalescingIntMaxPk;
	uint16_t u2CoalescingIntMaxTime;
	uint16_t u2CoalescingIntuFilterMask;
	u_int8_t fgCoalescingIntEn;
	uint32_t u4PerfMonTpCoalescingIntTh;
#endif
#if CFG_MTK_MDDP_SUPPORT
	u_int8_t fgMddpSupport;
#endif

	uint8_t fgSapCheckPmkidInDriver;
	uint8_t fgSapChannelSwitchPolicy;
	uint8_t fgSapConcurrencyPolicy;
	uint8_t fgSapAuthPolicy;
	uint8_t fgSapOverwriteAcsChnlBw;
	uint8_t fgSapAddTPEIE;
	uint8_t fgSapOffload;
	uint8_t fgSapSkipObss;
	uint8_t fgP2pGcCsa;
	uint8_t fgSkipP2pIe;
	uint8_t fgSkipP2pProbeResp;
	uint8_t ucDfsRegion;
	uint32_t u4ByPassCacTime;
	uint32_t u4CC2Region;
#if CFG_SUPPORT_TDLS_P2P_AUTO
	uint32_t u4TdlsP2pAuto;
#endif
	uint32_t u4ApChnlHoldTime;
	uint32_t u4P2pChnlHoldTime;
	uint32_t u4ProbeRspRetryLimit;
	uint8_t fgAllowSameBandDualSta;
	uint8_t ucApForceSleep;

#if CFG_SUPPORT_NAN
	uint8_t ucMasterPref;
	uint8_t ucConfig5gChannel;
	uint8_t ucChannel5gVal;
	uint8_t ucAisQuotaVal;	 /* Unit: NAN slot */
	uint8_t ucDftNdlQuotaVal;      /* Unit: NAN slot */
	uint8_t ucDftRangQuotaVal;     /* Unit: NAN slot */
	uint8_t ucDftQuotaStartOffset; /* Unit: NAN slot */
	uint8_t ucDftNdcStartOffset;
	uint8_t ucNanFixChnl;
	unsigned char fgEnableNDPE;
	uint8_t ucDftNdlQosQuotaVal;    /* Unit: NAN slot */
	uint16_t u2DftNdlQosLatencyVal; /* Unit: NAN slot */
	uint8_t fgEnNanVHT;
	uint8_t ucNanFtmBw;
	uint8_t ucNanDiscBcnInterval;
	uint8_t ucNanCommittedDw;
	unsigned char fgNoPmf;
	uint8_t ucNan2gBandwidth;
	uint8_t ucNan5gBandwidth;
	uint8_t ucNdlFlowCtrlVer;
#endif

#if CFG_SUPPORT_TPENHANCE_MODE
	uint8_t ucTpEnhanceEnable;
	uint8_t ucTpEnhancePktNum;
	uint32_t u4TpEnhanceInterval; /* in us */
	int8_t cTpEnhanceRSSI;
	uint32_t u4TpEnhanceThreshold;
#endif /* CFG_SUPPORT_TPENHANCE_MODE */

#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
	uint32_t u4BackoffLevel;
#endif

#if CFG_SUPPORT_LLS
	u_int8_t fgLinkStatsDump;
	enum LLS_QUERY_MODE ucLinkStatsQueryMode;
#endif /* CFG_SUPPORT_LLS */

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	uint8_t ucAbnWakeupDetectEn;
	uint8_t ucAbnWakeupPktThld;
	uint8_t ucAbnWakeupDetectIntv;
#endif

#if (CFG_SUPPORT_APF == 1)
	uint8_t ucApfEnable;
#endif

	/* rx rate filter */
	uint32_t u4RxRateProtoFilterMask;

#if CFG_SUPPORT_BAR_DELAY_INDICATION
	u_int8_t fgBARDelayIndicationEn;
#endif /* CFG_SUPPORT_BAR_DELAY_INDICATION */
	uint32_t u4MultiStaPrimaryQuoteTime;
	uint32_t u4MultiStaSecondaryQuoteTime;
#if CFG_SUPPORT_LIMITED_PKT_PID
	uint32_t u4PktPIDTimeout;
#endif /* CFG_SUPPORT_LIMITED_PKT_PID */
#if CFG_SUPPORT_ICS_TIMESYNC
	uint32_t u4IcsTimeSyncCnt;
#endif /* CFG_SUPPORT_ICS_TIMESYNC */
#if (CFG_SUPPORT_WIFI_6G == 1)
	/* Only scan all 6g channels, including PSC and non-PSC */
	u_int8_t fgEnOnlyScan6g;
#endif
	uint8_t ucCsaDeauthClient;

#define LATENCY_STATS_MAX_SLOTS 5
#define INVALID_TX_DELAY 0xFFFFFFFF
#if CFG_SUPPORT_TX_LATENCY_STATS
	u_int8_t fgPacketLatencyLog;
	u_int8_t fgTxLatencyKeepCounting;
	u_int8_t fgTxLatencyPerBss;
	uint32_t u4MsduStatsUpdateInterval; /* in ms */
	uint32_t u4ContinuousTxFailThreshold;

	uint32_t au4AirTxDelayMax[LATENCY_STATS_MAX_SLOTS]; /* in ms */
	uint32_t au4MacTxDelayMax[LATENCY_STATS_MAX_SLOTS]; /* in ms */
	uint32_t au4DriverTxDelayMax[LATENCY_STATS_MAX_SLOTS]; /* in ms */
	uint32_t au4ConnsysTxDelayMax[LATENCY_STATS_MAX_SLOTS]; /* in ms */
	uint32_t au4ConnsysTxFailDelayMax[LATENCY_STATS_MAX_SLOTS]; /* in ms */
#endif /* CFG_SUPPORT_TX_LATENCY_STATS */

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
	int32_t i4Ed2GNonEU;
	int32_t i4Ed5GNonEU;
	int32_t i4Ed2GEU;
	int32_t i4Ed5GEU;
#endif

#if CFG_MTK_FPGA_PLATFORM
	uint32_t u4FpgaSpeedFactor;
#endif
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	u_int8_t fgEnableMawd;
	u_int8_t fgEnableMawdTx;
	u_int8_t fgEnableSdo;
	u_int8_t fgEnableRro;
	u_int8_t fgEnableRro2Md;
	u_int8_t fgEnableRroPreFillRxRing;
	u_int8_t fgEnableRroDbg;
	u_int8_t fgEnableRroAdvDump;
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	u_int8_t fgIcmpTxs;

	uint8_t ucUdpTspecUp;
	uint8_t ucTcpTspecUp;
	uint32_t u4UdpDelayBound;
	uint32_t u4TcpDelayBound;
	uint8_t ucDataRate;
	uint8_t ucSupportProtocol; /* 0:UDP, 1:TCP, 2:BOTH */
	uint8_t ucCheckBeacon;
	uint8_t ucEnableFastPath;
	uint8_t ucFastPathAllPacket;
#if (CFG_TX_HIF_PORT_QUEUE == 1)
	uint8_t ucEnableTxHifPortQ;
#endif
#if (CFG_VOLT_INFO == 1)
	uint8_t fgVnfEn;
	uint32_t u4VnfDebTimes;
	uint32_t u4VnfDebInterval;
	uint32_t u4VnfDelta;
#endif /* CFG_VOLT_INFO */

#if CFG_SUPPORT_MLR
	u_int8_t fgEnForceTxFrag;
	uint16_t u2TxFragThr;
	/* 0:split size by average, 1:fixed split size */
	uint16_t u2TxFragSplitSize;
	uint8_t ucTxMlrRateRcpiThr;
	u_int8_t fgEnTxFragDebug;
	u_int8_t fgEnTxFragTxDone;
	u_int8_t ucErrPos;
#endif

#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
	uint32_t u4TxDataDelayTimeout;
	uint32_t u4TxDataDelayCnt;
#endif /* CFG_SUPPORT_TX_DATA_DELAY == 1 */

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	int32_t i4ThrmCtrlTemp;
	int32_t i4ThrmRadioOffTemp;
	uint8_t aucThrmLvTxDuty[6];
#endif

#if CFG_SUPPORT_PCIE_ASPM
	/* 0: Keep L0, 1: enable PCIE enter L1.2 */
	u_int8_t fgPcieEnableL1ss;
#endif

#if CFG_SUPPORT_PCIE_GEN_SWITCH
	uint32_t u4PcieGenSwitchTputThr;
	uint32_t u4PcieGenSwitchJudgeTime;
#endif

	u_int8_t fgEnWfdmaNoMmioRead;
#if CFG_MTK_WIFI_EN_SW_EMI_READ
	u_int8_t fgEnSwEmiRead;
#endif

	uint32_t u4PrdcIntTime;
	u_int8_t fgEnDlyInt;
	uint32_t u4DlyIntTime;
	uint32_t u4DlyIntCnt;

#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
	uint32_t u4PagePoolMinCnt;
	uint32_t u4PagePoolMaxCnt;
#endif

	/* 0:BW20, 1:BW40, 2:BW80, 3:BW160 4:BW8080, 5: BW320 */
	uint32_t u4PhyMaxBandwidth;

	uint32_t u4PmkRefreshThreshold;

	u_int8_t fgEnP2pPref6g;
	u_int8_t fgP2pPrefSkipDfs;
};

/* cnm_timer module */
struct ROOT_TIMER {
	struct LINK rLinkHead;
	OS_SYSTIME rNextExpiredSysTime;
	KAL_WAKE_LOCK_T *rWakeLock;
	u_int8_t fgWakeLocked;
};

/* FW/DRV/NVRAM version information */
struct WIFI_VER_INFO {

	/* NVRAM or Registry */
	uint16_t u2Part1CfgOwnVersion;
	uint16_t u2Part1CfgPeerVersion;
	uint16_t u2Part2CfgOwnVersion;
	uint16_t u2Part2CfgPeerVersion;

	/* Firmware */
	uint8_t aucReleaseManifest[256];

	/* N9 SW */
	uint16_t u2FwProductID;
	uint16_t u2FwOwnVersion;
	uint16_t u2FwPeerVersion;
	uint8_t ucFwBuildNumber;
	uint8_t aucFwBranchInfo[4];
	uint8_t aucFwDateCode[16];

	struct TAILER_COMMON_FORMAT_T rCommonTailer;
	struct TAILER_REGION_FORMAT_T rRegionTailers[MAX_FWDL_SECTION_NUM];

	/* N9 tailer */
	struct TAILER_FORMAT_T rN9tailer[N9_FWDL_SECTION_NUM];

	/* CR4 tailer */
	struct TAILER_FORMAT_T rCR4tailer[CR4_FWDL_SECTION_NUM];
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
	/* N9 Compressed tailer */
	struct TAILER_FORMAT_T_2 rN9Compressedtailer;
	/* CR4 tailer */
	struct TAILER_FORMAT_T_2 rCR4Compressedtailer;
	u_int8_t fgIsN9CompressedFW;
	u_int8_t fgIsCR4CompressedFW;
#endif
	/* Patch header */
	struct PATCH_FORMAT_T rPatchHeader;
	u_int8_t fgPatchIsDlByDrv;
};

#if CFG_ENABLE_WIFI_DIRECT
/*
 * p2p function pointer structure
 */

struct P2P_FUNCTION_LINKER {
	P2P_REMOVE prP2pRemove;

	P2P_GENERATE_P2P_IE prP2pGenerateWSC_IEForBeacon;

	P2P_NET_REGISTER prP2pNetRegister;
	P2P_NET_UNREGISTER prP2pNetUnregister;
	/* All IEs generated from supplicant. */
	P2P_CALCULATE_P2P_IE_LEN prP2pCalculateP2p_IELenForAssocReq;
	/* All IEs generated from supplicant. */
	P2P_GENERATE_P2P_IE prP2pGenerateP2p_IEForAssocReq;
};

#endif

struct CFG_SCAN_CHNL {
	uint8_t ucChannelListNum;
	struct RF_CHANNEL_INFO arChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];
};

#if CFG_SUPPORT_NCHO
enum ENUM_NCHO_ITEM_SET_TYPE {
	ITEM_SET_TYPE_NUM,
	ITEM_SET_TYPE_STR
};

enum ENUM_NCHO_BAND {
	NCHO_BAND_AUTO = 0,
	NCHO_BAND_5G,
	NCHO_BAND_2G4,
	NCHO_BAND_NUM
};

enum ENUM_NCHO_DFS_SCN_MODE {
	NCHO_DFS_SCN_DISABLE = 0,
	NCHO_DFS_SCN_ENABLE1,
	NCHO_DFS_SCN_ENABLE2,
	NCHO_DFS_SCN_NUM
};

struct CFG_NCHO_RE_ASSOC {
	/*!< SSID length in bytes. Zero length is broadcast(any) SSID */
	uint32_t u4SsidLen;
	uint8_t aucSsid[ELEM_MAX_LEN_SSID];
	uint8_t aucBssid[MAC_ADDR_LEN];
	uint32_t u4CenterFreq;
};

#define CFG_NCHO_SCAN_CHNL CFG_SCAN_CHNL

struct NCHO_ACTION_FRAME_PARAMS {
	uint8_t aucBssid[MAC_ADDR_LEN];
	int32_t i4channel;
	int32_t i4DwellTime;
	int32_t i4len;
	uint8_t aucData[520];
};

struct NCHO_AF_INFO {
	uint8_t *aucBssid;
	int32_t i4channel;
	int32_t i4DwellTime;
	int32_t i4len;
	uint8_t *pucData;
};

struct NCHO_INFO {
	u_int8_t fgNCHOEnabled;
	u_int8_t fgChGranted;
	u_int8_t fgIsSendingAF;
	int32_t i4RoamTrigger;		/* db */
	int32_t i4RoamDelta;		/* db */
	uint32_t u4RoamScanPeriod;	/* ms */
	uint32_t u4ScanChannelTime;	/* ms */
	uint32_t u4ScanHomeTime;		/* ms */
	uint32_t u4ScanHomeawayTime;	/* ms */
	uint32_t u4ScanNProbes;
	uint32_t u4WesMode;
	enum ENUM_NCHO_BAND eBand;
	enum ENUM_NCHO_DFS_SCN_MODE eDFSScnMode;
	uint32_t u4RoamScanControl;
	struct CFG_NCHO_SCAN_CHNL rRoamScnChnl;
	struct CFG_NCHO_SCAN_CHNL rAddRoamScnChnl;
	struct NCHO_ACTION_FRAME_PARAMS rParamActionFrame;
};
#endif

struct WIFI_FEM_CFG {
	/* WiFi FEM path */
	uint16_t u2WifiPath;
	uint16_t u2WifiPath6G;
	/* Is support DBDC A+A Mode (5+6) */
	uint16_t u2WifiDBDCAwithA;
	/* Minimum Frequency Interval to support A+A */
	uint16_t u2WifiDBDCAwithAMinimumFrqInterval;
	/* Reserved  */
	uint32_t au4Reserved[3];
};
/*
 * State Machine:
 * -->STOP: No Tx/Rx traffic
 * -->DISABLE: Screen is off
 * -->RUNNING: Screen is on && Tx/Rx traffic is active
 */
struct PERF_MONITOR {
	struct TIMER rPerfMonTimer;
	OS_SYSTIME rLastUpdateTime;
	unsigned long ulPerfMonFlag;
	unsigned long ulLastTxBytes[MAX_BSSID_NUM];
	unsigned long ulLastRxBytes[MAX_BSSID_NUM];
	unsigned long ulLastTxPackets[MAX_BSSID_NUM];
	unsigned long ulLastRxPackets[MAX_BSSID_NUM];
	unsigned long ulTxPacketsDiffLastSec[MAX_BSSID_NUM];
	unsigned long ulRxPacketsDiffLastSec[MAX_BSSID_NUM];
	uint64_t ulThroughput; /* in bps */
	/* ulThroughputInPPS = pps * ETHER_MAX_PKT_SZ */
	uint64_t ulThroughputInPPS; /* in pps */
	unsigned long ulTxTp[MAX_BSSID_NUM]; /* in Bps */
	unsigned long ulRxTp[MAX_BSSID_NUM]; /* in Bps */
	uint32_t u4UpdatePeriod; /* in ms */
	uint32_t u4BoostPerfLevel;
	uint32_t u4TarPerfLevel;
	uint32_t u4CurrPerfLevel;
	uint32_t u4UsedCnt;
	unsigned long ulTotalTxSuccessCount;
	unsigned long ulTotalTxFailCount;
	uint32_t u4TriggerCnt;
	uint32_t u4RunCnt;
	u_int8_t fgIdle; /* set as true when no tx/rx on last sec */
};

struct HIF_STATS {
	unsigned long ulUpdatePeriod; /* in ms */
	uint32_t u4MsiIsrCount[32];
	uint32_t u4HwIsrCount;
	uint32_t u4SwIsrCount;
	uint32_t u4IsrNotIndCount;
	uint32_t u4CmdInCount; /* cmd from main_thread to hif_thread */
	uint32_t u4CmdTxCount; /* cmd from hif_thread to DMA */
	uint32_t u4CmdTxdoneCount; /* cmd from DMA to consys */
	uint32_t u4DataInCount; /* data from main_thread to hif_thread */
	uint32_t u4DataTxCount; /* data from hif_thread to DMA */
	uint32_t u4DataTxdoneCount; /* data from DMA to consys */
	uint32_t u4DataMsduRptCount; /* data from consys to air */
	uint32_t u4EventRxCount; /* event from DMA to hif_thread */
	uint32_t u4DataRxCount; /* data from DMA to hif_thread */
	uint32_t u4TxDataRegCnt;
	uint32_t u4RxDataRegCnt;
};

struct OID_HANDLER_RECORD {
	uint8_t aucName[100];
};

/**
 * struct TX_LATENCY_STATS - TX latency statistics counters
 * @au4DriverLatency: Counter distribution of TX delay in Driver
 * @au4ConnsysLatency: Counter distribution of TX delay in Connsys
 * @au4MacLatency: Counter distribution of TX delay logged in MSDU report
 * @au4FailConnsysLatency: Counter distribution of TX Failed delay in Connsys
 * @u4TxFail: Number of TX failed count
 */
struct TX_LATENCY_STATS {
	uint32_t au4DriverLatency[BSSID_NUM][LATENCY_STATS_MAX_SLOTS];
	uint32_t au4ConnsysLatency[BSSID_NUM][LATENCY_STATS_MAX_SLOTS];
	uint32_t au4MacLatency[BSSID_NUM][LATENCY_STATS_MAX_SLOTS];
	uint32_t au4AirLatency[BSSID_NUM][LATENCY_STATS_MAX_SLOTS];
	uint32_t au4FailConnsysLatency[BSSID_NUM][LATENCY_STATS_MAX_SLOTS];
	uint32_t u4TxFail;
};

/**
 * struct TX_LATENCY_REPORT_STATS - TX latency for reporting
 * @rCounting: Continuous counting counters of each TX delay metrics
 * @rReported: Reported counters of each TX delay metrics
 * @u4ContinuousTxFail: Continuous TX fail monitor abnormal TX fail cases
 * @fgTxLatencyEnabled: A on/off switch controlling the reporting mechanism
 */
struct TX_LATENCY_REPORT_STATS {
	struct TX_LATENCY_STATS rCounting;
	struct TX_LATENCY_STATS rReported;
#if (CFG_WFD_SCC_BALANCE_SUPPORT == 1)
	struct TX_LATENCY_STATS rReported4SccB;
	struct TX_LATENCY_STATS rDiff;
#endif
	uint32_t u4ContinuousTxFail;
	u_int8_t fgTxLatencyEnabled;
};

struct HOST_SUSPEND_NOTIFY_INFO {
	enum ENUM_HOST_SUSPEND_ADDR_TYPE eType;
	uint32_t u4SetAddr;
	uint32_t u4ClrAddr;
	uint32_t u4Mask;
	uint32_t u4Shift;
};

#if CFG_SUPPORT_MSP
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
struct MIB_STATS {
	uint64_t u4RxFcsErrCnt;
	uint64_t u4RxFifoFullCnt;
	uint64_t u4RxMpduCnt;
	uint64_t u4RxAMPDUCnt;
	uint64_t u4RxTotalByte;
	uint64_t u4RxValidAMPDUSF;
	uint64_t u4RxValidByte;
	uint64_t u4ChannelIdleCnt;
	uint64_t u4RxVectorDropCnt;
	uint64_t u4DelimiterFailedCnt;
	uint64_t u4RxVectorMismatchCnt;
	uint64_t u4MdrdyCnt;
	uint64_t u4CCKMdrdyCnt;
	uint64_t u4OFDMLGMixMdrdy;
	uint64_t u4OFDMGreenMdrdy;
	uint64_t u4PFDropCnt;
	uint64_t u4RxLenMismatchCnt;
	uint64_t u4PCcaTime;
	uint64_t u4SCcaTime;
	uint64_t u4CcaNavTx;
	uint64_t u4PEDTime;
	uint64_t u4BeaconTxCnt;
	uint64_t u4Tx20MHzCnt;
	uint64_t u4Tx40MHzCnt;
	uint64_t u4Tx80MHzCnt;
	uint64_t u4Tx160MHzCnt;
	uint64_t u4Tx320MHzCnt;
	uint64_t u8ObssAirTime;
	uint64_t u8NonWifiAirTime;
	uint64_t u8TxDurCnt;
	uint64_t u8RxDurCnt;
	uint64_t u8BACnt;
	uint64_t u4Mac2PHYTxTime;
	uint64_t u4MacRxFcsOkCnt;
	uint64_t u4RxOutOfRangeCnt;
	uint64_t au4RtsTxCnt[BSSID_NUM];
	uint64_t au4RtsRetryCnt[BSSID_NUM];
	uint64_t au4BaMissedCnt[BSSID_NUM];
	uint64_t au4AckFailedCnt[BSSID_NUM];
	uint64_t au4FrameRetryCnt[BSSID_NUM];
	uint64_t au4FrameRetry2Cnt[BSSID_NUM];
	uint64_t au4FrameRetry3Cnt[BSSID_NUM];
	uint64_t au4TxDdlmtRng[5];
	uint64_t au4TxCnt[BSSID_NUM];
	uint64_t au4TxData[BSSID_NUM];
	uint64_t au4TxByte[BSSID_NUM];
	uint64_t au4RxOk[BSSID_NUM];
	uint64_t au4RxData[BSSID_NUM];
	uint64_t au4RxByte[BSSID_NUM];
	uint64_t au4MbssTxOk[16];
	uint64_t au4MbssTxByte[16];
	uint64_t au4MbssRxOk[16];
	uint64_t au4MbssRxByte[16];
	uint64_t u4TxAmpdu;
	uint64_t u4TxAmpduMpdu;
	uint64_t u4TxAmpduAcked;
};
#endif
#endif

/**
 * @u4Delay: accumulated delay for counting average (ms)
 * @u4DelayNum: accumulated TX count for counting average
 * @u4DelayLimit: delay limit triggering indication (ms)
 * @fgTxDelayOverLimitReportInterval: interval for counting average report (ms)
 * @eTxDelayOverLimitStatsType: choice of report types, average or immediate
 * @fgTxDelayOverLimitReportEnabled: switching the report on/off
 * @fgReported: mark if the error has been reported in that interval
 */
struct TX_DELAY_OVER_LIMIT_REPORT_STATS {
	uint32_t u4Delay[MAX_TX_OVER_LIMIT_TYPE];
	uint32_t u4DelayNum[MAX_TX_OVER_LIMIT_TYPE];
	uint32_t u4DelayLimit[MAX_TX_OVER_LIMIT_TYPE];
	uint32_t fgTxDelayOverLimitReportInterval; /* if REPORT_AVERAGE */
	enum ENUM_TX_OVER_LIMIT_STATS_TYPE eTxDelayOverLimitStatsType;
	bool fgTxDelayOverLimitReportEnabled;
	bool fgReported[MAX_TX_OVER_LIMIT_TYPE];
};

/*
 * Major ADAPTER structure
 * Major data structure for driver operation
 */
struct ADAPTER {
	struct mt66xx_chip_info *chip_info;
	uint8_t ucRevID;
	u_int8_t fgIsReadRevID;

	uint16_t u2NicOpChnlNum;

	u_int8_t fgIsEnableWMM;
	/* This flag is used to indicate that WMM is enable in current BSS */
	u_int8_t fgIsWmmAssoc;

	uint32_t u4OsPacketFilter;	/* packet filter used by OS */
	u_int8_t fgAllMulicastFilter;	/* mDNS filter used by OS */

	struct BSS_INFO *aprBssInfo[MAX_BSSID_NUM + 1];
	struct BSS_INFO *aprSapBssInfo[KAL_P2P_NUM];

	uint8_t ucHwBssIdNum;
	uint8_t ucWmmSetNum;
	uint8_t ucWtblEntryNum;
	uint8_t ucTxDefaultWlanIndex;
	uint8_t ucP2PDevBssIdx;

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	/* Does FW support Checksum Offload feature */
	u_int8_t fgIsSupportCsumOffload;
	uint32_t u4CSUMFlags;
#endif				/* CFG_TCP_IP_CHKSUM_OFFLOAD */

	enum ENUM_BAND aePreferBand[NETWORK_TYPE_NUM];

	/* ADAPTER flags */
	uint32_t u4Flags;
	uint32_t u4HwFlags;

	u_int8_t fgIsRadioOff;

	u_int8_t fgIsEnterD3ReqIssued;

	uint8_t aucMacAddress[MAC_ADDR_LEN];

	/* Current selection basing on the set of Available PHY Types */
	enum ENUM_PHY_TYPE_INDEX eCurrentPhyType;

	uint32_t u4CoalescingBufCachedSize;
	uint8_t *pucCoalescingBufCached;

	/* Buffer for CMD_INFO_T, Mgt packet and mailbox message */
	struct BUF_INFO rMgtBufInfo;
	struct BUF_INFO rMsgBufInfo;
	uint8_t *pucMgtBufCached;
	uint32_t u4MgtBufCachedSize;
	uint8_t aucMsgBuf[MSG_BUFFER_SIZE];
#if CFG_DBG_MGT_BUF
	uint32_t u4MemAllocDynamicCount;	/* Debug only */
	uint32_t u4MemFreeDynamicCount;	/* Debug only */
#endif

	struct STA_RECORD arStaRec[CFG_STA_REC_NUM];

	/* Element for TX PATH */
	struct TX_CTRL rTxCtrl;
	struct QUE rFreeCmdList;
	struct CMD_INFO arHifCmdDesc[CFG_TX_MAX_CMD_PKT_NUM];

	/* Element for RX PATH */
	struct RX_CTRL rRxCtrl;

	/* bitmap for hif adjust control */
	uint8_t ucAdjustCtrlBitmap;

	/* Timer for restarting RFB setup procedure */
	struct TIMER rPacketDelaySetupTimer;

	uint32_t u4IntStatus;

	enum ENUM_ACPI_STATE rAcpiState;

	uint32_t fgIsIntEnable;
	u_int8_t fgIsIntEnableWithLPOwnSet;

	u_int8_t fgIsFwOwn;
	u_int8_t fgIsWiFiOnDrvOwn;
	u_int8_t fgWiFiInSleepyState;

	/* Set by callback to make sure WOW done before system suspend */
	u_int8_t fgSetPfCapabilityDone;
	u_int8_t fgSetWowDone;

	OS_SYSTIME rLastOwnFailedLogTime;
	uint32_t u4OwnFailedCount;
	uint32_t u4OwnFailedLogCount;

	uint32_t u4PwrCtrlBlockCnt;

	/* TX Direct related : BEGIN */
	u_int8_t fgTxDirectInited;

	#define TX_DIRECT_CHECK_INTERVAL	(1000 * HZ / USEC_PER_SEC)

	struct QUE rTxDirectHifQueue[TC_NUM];
	struct QUE rStaPsQueue[CFG_STA_REC_NUM];
	uint32_t u4StaPsBitmap;
	struct QUE rBssAbsentQueue[MAX_BSSID_NUM + 1];
	uint32_t u4BssAbsentTxBufferBitmap;
	struct QUE rStaPendQueue[CFG_STA_REC_NUM];
	uint32_t u4StaPendBitmap;
#if CFG_SUPPORT_SOFT_ACM
	struct QUE rStaAcmQueue[CFG_STA_REC_NUM][AC_NUM];
	int32_t i4StaAcmQueueCnt[CFG_STA_REC_NUM];
	uint32_t u4StaAcmBitmap;
#endif /* CFG_SUPPORT_SOFT_ACM */
	/* TX Direct related : END */
#if (CFG_TX_MGMT_BY_DATA_Q == 1)
	struct QUE rMgmtDirectTxQueue;
#endif /* CFG_TX_MGMT_BY_DATA_Q == 1 */

	struct QUE rPendingCmdQueue;

#if CFG_SUPPORT_MULTITHREAD
	struct QUE rTxCmdQueue;
	struct QUE rTxCmdDoneQueue;
#if CFG_FIX_2_TX_PORT
	struct QUE rTxP0Queue;
	struct QUE rTxP1Queue;
#else
	struct QUE rTxPQueue[MAX_BSSID_NUM][TC_NUM];
#if (CFG_TX_HIF_PORT_QUEUE == 1)
	struct QUE rTxHifPQueue[MAX_BSSID_NUM][TC_NUM];
#endif
#endif
	struct QUE rRxQueue;
	struct QUE rTxDataDoneQueue;
#endif
	/* Rx queue that queue rx packets before ASSOC */
	struct QUE rRxPendingQueue;

	struct GLUE_INFO *prGlueInfo;

	uint8_t ucCmdSeqNum;
	uint8_t ucTxSeqNum;
#if CFG_SUPPORT_SEPARATE_TXS_PID_POOL
	uint8_t aucPidPool[WTBL_SIZE][TX_PACKET_TYPE_NUM];
#else
	uint8_t aucPidPool[WTBL_SIZE];
#endif

#if CFG_SUPPORT_LIMITED_PKT_PID
	/* last timestamp of pkt with txdone */
	uint32_t u4PktPIDTime[WTBL_SIZE][ENUM_PKT_FLAG_NUM];
#endif /* CFG_SUPPORT_LIMITED_PKT_PID */

#if 1				/* CFG_SUPPORT_WAPI */
	u_int8_t fgUseWapi;
#endif

	/* RF Test flags */
	u_int8_t fgTestMode;
	u_int8_t fgIcapMode;

	/* WLAN Info for DRIVER_CORE OID query */
	struct WLAN_INFO rWlanInfo;

#if CFG_SUPPORT_NAN
	enum ENUM_NET_REG_STATE rNanNetRegState;
	unsigned char fgIsNANRegistered;
	unsigned char fgIsNANfromHAL;
	bool fgIsNanSendRequestToCnm;
	uint8_t ucNanReqTokenId;
	uint8_t ucNanPubNum;
	uint8_t ucNanSubNum;

	/* Container for Data Engine */
	struct _NAN_DATA_PATH_INFO_T rDataPathInfo;

	/* Container for Ranging Engine */
	struct _NAN_RANGING_INFO_T rRangingInfo;
#endif

#if CFG_ENABLE_WIFI_DIRECT
	uint8_t u4P2pMode;
	u_int8_t fgIsP2PRegistered;
	/* flag to report all networks in p2p scan */
	u_int8_t p2p_scan_report_all_bss;
	enum ENUM_NET_REG_STATE rP2PNetRegState;
	enum ENUM_P2P_REG_STATE rP2PRegState;
	/* BOOLEAN             fgIsWlanLaunched; */
	struct P2P_INFO *prP2pInfo;
#if CFG_SUPPORT_P2P_RSSI_QUERY
	OS_SYSTIME rP2pLinkQualityUpdateTime;
	u_int8_t fgIsP2pLinkQualityValid;
	struct LINK_QUALITY rP2pLinkQuality;
#endif
#endif

	/* Online Scan Option */
	u_int8_t fgEnOnlineScan;

	/* Online Scan Option */
	u_int8_t fgDisBcnLostDetection;

	/* MAC address */
	uint8_t rMyMacAddr[PARAM_MAC_ADDR_LEN];

	/* Wake-up Event for WOL */
	uint32_t u4WakeupEventEnable;

	/* Event Buffering */
	struct EVENT_STATISTICS rStatStruct;
	OS_SYSTIME rStatUpdateTime;
	u_int8_t fgIsStatValid;

#if CFG_SUPPORT_LLS
	uint8_t ucLinkStatsBssNum;
	struct HAL_LLS_FULL_REPORT rLinkStatsDestBuffer;
	struct HAL_LLS_FW_REPORT *pucLinkStatsSrcBufferAddr;
	struct LinkStatsBuffer {
		uint8_t *pucLinkStatsBuffer;
		uint8_t *pucLinkStatsBufferEnd;
	} rLinkStatsCache[MAX_BSSID_NUM];
	/* Store in LLS order */
	uint32_t *pu4TxTimePerLevels;
	uint32_t u4TxTimePerLevelsSize; /* 256 * 4bytes (uint32_t) * 2 bands */
	struct STATS_LLS_PEER_AP_REC rPeerApRec[KAL_AIS_NUM];
#endif

#if CFG_SUPPORT_MSP
	struct EVENT_WLAN_INFO rEventWlanInfo;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	struct MIB_STATS rMibStats[ENUM_BAND_NUM];
#endif
#endif

	struct PARAM_LINK_SPEED_EX rLinkQuality;

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
	OS_SYSTIME rAllStatsUpdateTime;
#endif

	/* WIFI_VAR_T */
	struct WIFI_VAR rWifiVar;

	/* Mailboxs for inter-module communication */
	struct MBOX arMbox[MBOX_ID_TOTAL_NUM];

	/* Timers for OID Pending Handling */
	struct TIMER rOidTimeoutTimer;
	uint8_t ucOidTimeoutCount;

	/* Root Timer for cnm_timer module */
	struct ROOT_TIMER rRootTimer;

#if CFG_CHIP_RESET_SUPPORT
	enum ENUM_WFSYS_RESET_STATE_TYPE_T eWfsysResetState;
	u_int8_t fgIsCfgSuspend;
	/* Records how many times L0.5 reset is triggered. */
	uint16_t u2WfsysResetCnt;
#endif
	u_int8_t fgIsChipNoAck;
	u_int8_t fgIsChipAssert;
	u_int8_t fgIsCmdAllocFail;
	uint32_t u4CmdAllocStartFailTime;

	/* RLM maintenance */
	enum ENUM_CHNL_EXT eRfSco;
	enum ENUM_SYS_PROTECT_MODE eSysProtectMode;
	enum ENUM_GF_MODE eSysHtGfMode;
	enum ENUM_RIFS_MODE eSysTxRifsMode;
	enum ENUM_SYS_PCO_PHASE eSysPcoPhase;

	struct DOMAIN_INFO_ENTRY *prDomainInfo;

	/* QM */
	struct QUE_MGT rQM;

	uint32_t u4PowerMode;

	uint32_t u4CtiaPowerMode;
	u_int8_t fgEnCtiaPowerMode;

	uint32_t fgEnArpFilter;

	uint32_t u4UapsdAcBmp;
	uint32_t u4MaxSpLen;
	uint32_t u4P2pUapsdAcBmp;
	uint32_t u4P2pMaxSpLen;

	uint32_t u4PsCurrentMeasureEn;

	/* Version Information */
	struct WIFI_VER_INFO rVerInfo;

	/* 5GHz support (from F/W) */
	u_int8_t fgIsHw5GBandDisabled;
	u_int8_t fgEnable5GBand;
	u_int8_t fgIsEepromUsed;
	u_int8_t fgIsEfuseValid;
	u_int8_t fgIsEmbbededMacAddrValid;

#if (CFG_SUPPORT_WIFI_6G == 1)
	u_int8_t fgIsHwSupport6G;
#endif

#if CFG_SUPPORT_ANT_SWAP
	u_int8_t fgIsSupportAntSwp;
#endif
#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
	u_int8_t fgIsPowerLimitTableValid;
#endif

	/* Packet Forwarding Tracking */
	int32_t i4PendingFwdFrameCount;

#if CFG_SUPPORT_RDD_TEST_MODE
	uint8_t ucRddStatus;
#endif

	u_int8_t fgDisStaAgingTimeoutDetection;

	uint32_t u4FwCompileFlag0;
	uint32_t u4FwCompileFlag1;
	uint32_t u4FwFeatureFlag0;
	uint32_t u4FwFeatureFlag1;

#if CFG_SUPPORT_CFG_FILE
	struct WLAN_CFG *prWlanCfg;
	struct WLAN_CFG rWlanCfg;

	struct WLAN_CFG_REC *prWlanCfgRec;
	struct WLAN_CFG_REC rWlanCfgRec;

	struct WLAN_CFG *prWlanCfgEm;
	struct WLAN_CFG rWlanCfgEm;
#endif

#if CFG_M0VE_BA_TO_DRIVER
	struct TIMER rMqmIdleRxBaDetectionTimer;
	uint32_t u4FlagBitmap;
#endif
#if (CFG_CE_ASSERT_DUMP == 1)
	struct TIMER rN9CorDumpTimer;
	u_int8_t fgN9AssertDumpOngoing;
	u_int8_t fgKeepPrintCoreDump;
#endif
	/* Tx resource information */
	u_int8_t fgIsNicTxReousrceValid;
	struct tx_resource_info nicTxReousrce;

    /* Efuse Start and End address */
	uint32_t u4EfuseStartAddress;
	uint32_t u4EfuseEndAddress;

	/* COEX feature */
	uint32_t u4FddMode;

	struct HOST_SUSPEND_NOTIFY_INFO rHostSuspendInfo;

	/* Casan load type */
	uint32_t u4CasanLoadType;

#if CFG_WOW_SUPPORT
	struct WOW_CTRL	rWowCtrl;
	u_int8_t fgWowLinkDownPendFlag;
#if CFG_SUPPORT_MDNS_OFFLOAD
	struct MDNS_INFO_T rMdnsInfo;
	u_int8_t mdns_offload_enable;
	uint8_t mdns_wake_flag;
#endif

#endif
#if CFG_SUPPORT_WOW_EINT_KEYEVENT_WAKEUP
	struct input_dev *prWowInputDev;
#endif

#if CFG_SUPPORT_WOW_EINT
	struct WOWLAN_DEV_NODE rWowlanDevNode;
#endif

#if CFG_SUPPORT_NCHO			/*  NCHO information */
	struct NCHO_INFO rNchoInfo;
#endif
	struct CFG_SCAN_CHNL rAddRoamScnChnl;

/*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
	uint8_t aucEepromVaule[16]; /* HQA CMD for Efuse Block size contents */
	uint32_t u4FreeBlockNum;
	uint32_t u4TotalBlockNum;
	uint32_t u4GetTxPower;
/*#endif*/
	u_int8_t fgIsCr4FwDownloaded;
	u_int8_t fgIsFwDownloaded;
	u_int8_t fgIsSupportBufferBinSize16Byte;
	u_int8_t fgIsSupportDelayCal;
	u_int8_t fgIsSupportGetFreeEfuseBlockCount;
	u_int8_t fgIsSupportQAAccessEfuse;
	u_int8_t fgIsSupportPowerOnSendBufferModeCMD;
	u_int8_t fgIsSupportGetTxPower;
	u_int8_t fgIsEnableLpdvt;

	u_int8_t fgSuppSmeLinkDownPend;

	/* SER related info */
	uint8_t ucSerState;

#if (CFG_HW_WMM_BY_BSS == 1)
	uint8_t ucHwWmmEnBit;
#endif
	unsigned long ulSuspendFlag;
	struct WIFI_FEM_CFG rWifiFemCfg;

	/* Smar Gear */
	uint8_t ucSmarGearSupportSisoOnly;
	uint8_t ucSmartGearWfPathSupport;

	struct PERF_MONITOR rPerMonitor;
#if CFG_SUPPORT_MCC_BOOST_CPU
	u_int8_t fgMccBoost;
#endif /* CFG_SUPPORT_MCC_BOOST_CPU */

	/* ICAP */
	u_int8_t ucICapDone;
	struct ICAP_INFO_T rIcapInfo;
	struct RBIST_DUMP_IQ_T QAICapInfo;
	struct EXT_EVENT_RBIST_DUMP_DATA_T IcapDumpEvent;
	struct RECAL_INFO_T rReCalInfo;

	/* Support change QM RX BA entry miss timeout (unit: ms) dynamically */
	uint32_t u4QmRxBaMissTimeout;

#if CFG_SUPPORT_LOWLATENCY_MODE
	u_int8_t fgEnLowLatencyMode;
	u_int8_t fgEnCfg80211Scan;
	u_int8_t fgEnTxDupDetect;
	u_int8_t fgTxDupCertificate;
	OS_SYSTIME tmTxDataInterval;
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

#if (CFG_SUPPORT_802_11AX == 1)
	struct __HE_CFG_INFO_T rHeCfg;
	uint8_t ucMcsMapSetFromSigma;
	u_int8_t fgMcsMapBeenSet;
	u_int8_t fgMuEdcaOverride;
	uint32_t u4MBACount;
	uint32_t u4HeHtcOM;
	uint8_t  fgEnShowHETrigger;
	uint8_t fgTxPPDU;
	uint8_t ucVcoreBoost;
#endif /* CFG_SUPPORT_802_11AX == 1 */
#if (CFG_SUPPORT_802_11BE == 1)
	uint8_t fgEhtHtcOM;
	uint32_t u4EhtMcsMap20MHzSetFromSigma;
	uint32_t u4EhtMcsMap80MHzSetFromSigma;
#endif
#if (CFG_SUPPORT_TWT == 1)
	struct _TWT_PLANNER_T rTWTPlanner;
#endif

#if CFG_SUPPORT_WIFI_SYSDVT
	struct AUTOMATION_DVT *auto_dvt;
	uint16_t u2TxTest;
	uint16_t u2TxTestCount;
	uint8_t  ucTxTestUP;
#endif /* CFG_SUPPORT_WIFI_SYSDVT */

	uint32_t u4HifDbgFlag;
	uint32_t u4HifChkFlag;
	uint32_t u4HifDbgMod;
	uint32_t u4HifDbgBss;
	uint32_t u4HifDbgReason;
	uint32_t u4HifTxHangDumpBitmap;
	uint32_t u4HifTxHangDumpIdx;
	uint32_t u4HifTxHangDumpNum;
	unsigned long ulNoMoreRfb;
#if CFG_DYNAMIC_RFB_ADJUSTMENT
	uint32_t u4RfbInUseCnt;
	uint32_t u4RfbInUseCntLv;
	unsigned long ulUpdateRxRfbCntPeriod;
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */

	/* Only for PCIE DmaSchdl usage so far. */
	struct {
		bool fgRun;
		uint32_t u4Quota;
	} rWmmQuotaReqCS[MAX_BSSID_NUM];

	/* TX HIF Control falgs */
	uint32_t au4TxHifResCtl[TC_NUM];
	uint32_t u4TxHifResCtlIdx;
	uint32_t u4TxHifResCtlNum;

#if CFG_SUPPORT_OSHARE
	bool fgEnOshareMode;
#endif

	bool fgMddpActivated;
	uint8_t ucMddpBssIndex;
#if defined(_HIF_PCIE)
#if CFG_SUPPORT_PCIE_ASPM
	uint32_t u4MddpPCIeL12SeqNum;
#endif
#endif

	struct WLAN_DEBUG_INFO rDebugInfo;
#if CFG_SUPPORT_IOT_AP_BLACKLIST
	struct WLAN_IOT_AP_RULE_T rIotApRule[CFG_IOT_AP_RULE_MAX_CNT];
#endif

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	uint32_t u4LastLinkQuality;
	uint32_t u4LinkQualityCounter;
	struct WIFI_LINK_QUALITY_INFO rLinkQualityInfo;
#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
	struct PARAM_GET_STA_STATISTICS rQueryStaStatistics[MAX_BSSID_NUM];
#else
	struct PARAM_GET_STA_STATISTICS rQueryStaStatistics;
#endif
	struct PARAM_802_11_STATISTICS_STRUCT rStat;
	uint32_t u4BufLen;
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
	/* dynamic tx power control */
	struct LINK rTxPwr_DefaultList;
	struct LINK rTxPwr_DynamicList;
#endif

#if CFG_DBG_MGT_BUF
	struct LINK rMemTrackLink;
#endif

#if CFG_SUPPORT_DATA_STALL
	OS_SYSTIME tmReportinterval;
#endif

#if CFG_SUPPORT_BIGDATA_PIP
	OS_SYSTIME tmDataPipReportinterval;
#endif

	u_int8_t fgEnDbgPowerMode;

	struct HIF_STATS rHifStats;

	struct TX_LATENCY_REPORT_STATS rMsduReportStats;

	struct TX_DELAY_OVER_LIMIT_REPORT_STATS rTxDelayOverLimitStats;

	unsigned int u4FWLastUpdateTime;

	u_int8_t fgSetLogOnOff;
	u_int8_t fgSetLogLevel;

/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
	struct WAKEUP_STATISTIC arWakeupStatistic[WAKEUP_TYPE_NUM];
	uint32_t wake_event_count[EVENT_ID_END];
#endif
#if CFG_SUPPORT_EXCEPTION_STATISTICS
	uint32_t total_beacon_timeout_count;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint32_t beacon_timeout_count[UNI_ENUM_BCN_TIMEOUT_REASON_MAX_NUM];
#else
	uint32_t beacon_timeout_count[BEACON_TIMEOUT_REASON_NUM];
#endif
	uint32_t total_tx_done_fail_count;
	uint32_t tx_done_fail_count[TX_RESULT_NUM];
	uint32_t total_deauth_rx_count;
	uint32_t deauth_rx_count[REASON_CODE_BEACON_TIMEOUT + 1];
	uint32_t total_scandone_timeout_count;
	uint32_t total_mgmtTX_timeout_count;
	uint32_t total_mgmtRX_timeout_count;
#endif

	struct OID_HANDLER_RECORD arPrevWaitHdlrRec[OID_HDLR_REC_NUM];
	struct OID_HANDLER_RECORD arPrevCompHdlrRec[OID_HDLR_REC_NUM];
	uint32_t u4WaitRecIdx;
	uint32_t u4CompRecIdx;
	const char *fw_flavor;

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
	u_int8_t fgEnTmacICS;
	u_int8_t fgEnRmacICS;
	uint16_t u2IcsSeqNo;
#endif /* CFG_SUPPORT_ICS */

#if CFG_SUPPORT_PHY_ICS
	u_int8_t fgEnPhyICS;
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	struct LINK rPwrLevelHandlerList;
	uint32_t u4PwrLevel;
	struct conn_pwr_event_max_temp rTempInfo;
	struct THRM_PROT_CFG_CONTEXT rThrmProtCfg;
	OS_SYSTIME rPwrLevelStatUpdateTime;
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
	bool fgPowerForceOneNss;
	bool fgPowerNeedDisconnect;
	bool fgANTCtrl;
	u_int8_t ucANTCtrlReason;
	u_int8_t ucANTCtrlPendingCount;
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO aprMldBssInfo[MAX_BSSID_NUM];
	struct MLD_STA_RECORD aprMldStarec[CFG_STA_REC_NUM];
	uint8_t ucBssAbsentBitmap;
	uint32_t u4StaInPSBitmap;
#endif
	uint8_t ucCnmTokenID;
#if (CFG_SUPPORT_AVOID_DESENSE == 1)
	bool fgIsNeedAvoidDesenseFreq;
#endif

	bool fgForceDualStaInMCCMode;
	uint8_t ucIsMultiStaConnected;
	uint32_t u4MultiStaPrimaryInterface;
	uint32_t u4MultiStaUseCase;

	bool fgIsPostponeTxEAPOLM3;

#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
	u_int8_t fgIsNeedDlPatch;
#endif
	uint8_t CurNoResSeqID;
#if defined(_HIF_SDIO)
	u_int8_t fgGetMailBoxRWAck;
	/* For check mailbox r/w access */
	u_int8_t fgMBAccessFail;
#endif
#if (CFG_WIFI_GET_MCS_INFO == 1)
	struct TIMER rRxMcsInfoTimer;
	u_int8_t fgIsMcsInfoValid;
#endif

#if CFG_SUPPORT_TDLS
	uint32_t u4TdlsLinkCount;
#endif /* CFG_SUPPORT_TDLS */

#if CFG_FAST_PATH_SUPPORT
	struct MSCS_CAP_FAST_PATH rFastPathCap;
#endif
	uint8_t ucEnVendorSpecifiedRpt;

#if CFG_SUPPORT_MLR
	uint8_t ucMlrIsSupport;
	uint8_t ucMlrVersion;
	uint32_t u4MlrSupportBitmap;
#endif

#if CFG_SUPPORT_PKT_OFLD
	struct ABNORMAL_WAKEUP_STATISTIC rAbnormalWakeupStat;
	u_int8_t ucRxDataMode;
#endif

	struct QUE rTimeoutRxBaEntry; /* wait for Timeout flush by NAPI */
	struct QUE rFlushRxBaEntry; /* wait for BA Delete flush by NAPI */

	uint32_t u4LongestPending; /* longest pending token in seconds */
};				/* end of _ADAPTER_T */

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

#define SUSPEND_FLAG_FOR_WAKEUP_REASON (0)
#define SUSPEND_FLAG_CLEAR_WHEN_RESUME (1)

/* Macros for argument _BssIndex */
#define IS_NET_ACTIVE(_prAdapter, _BssIndex) \
	((_prAdapter)->aprBssInfo[(_BssIndex)]->fgIsNetActive)

/* Macros for argument _prBssInfo */
#define IS_BSS_ACTIVE(_prBssInfo)     ((_prBssInfo)->fgIsNetActive)

#define IS_BSS_AIS(_prBssInfo) \
	((_prBssInfo)->eNetworkType == NETWORK_TYPE_AIS && \
	 (_prBssInfo)->fgIsInUse)

#define IS_BSS_INDEX_AIS(_prAdapter, _BssIndex) \
	(GET_BSS_INFO_BY_INDEX(_prAdapter, _BssIndex) && \
	IS_BSS_AIS(GET_BSS_INFO_BY_INDEX(_prAdapter, _BssIndex)))

#define IS_BSS_P2P(_prBssInfo) \
	((_prBssInfo)->eNetworkType == NETWORK_TYPE_P2P)

#define IS_BSS_INDEX_P2P(_prAdapter, _BssIndex) \
	(GET_BSS_INFO_BY_INDEX(_prAdapter, _BssIndex) && \
	IS_BSS_P2P(GET_BSS_INFO_BY_INDEX(_prAdapter, _BssIndex)))

#define IS_BSS_BOW(_prBssInfo) \
	((_prBssInfo)->eNetworkType == NETWORK_TYPE_BOW)

#define IS_BSS_APGO(_prBssInfo) \
	(IS_BSS_P2P(_prBssInfo) && \
	(_prBssInfo)->eCurrentOPMode == OP_MODE_ACCESS_POINT)

#define IS_BSS_GC(_prBssInfo) \
	(IS_BSS_P2P(_prBssInfo) && \
	(_prBssInfo)->eCurrentOPMode == OP_MODE_INFRASTRUCTURE)

#define SET_NET_ACTIVE(_prAdapter, _BssIndex) \
	{(_prAdapter)->aprBssInfo[(_BssIndex)]->fgIsNetActive = TRUE; }

#define UNSET_NET_ACTIVE(_prAdapter, _BssIndex) \
	{(_prAdapter)->aprBssInfo[(_BssIndex)]->fgIsNetActive = FALSE; }

#define BSS_INFO_INIT(_prAdapter, _prBssInfo) \
{   uint8_t _aucZeroMacAddr[] = NULL_MAC_ADDR; \
	\
	(_prBssInfo)->eConnectionState = MEDIA_STATE_DISCONNECTED; \
	(_prBssInfo)->eConnectionStateIndicated = \
		MEDIA_STATE_DISCONNECTED; \
	(_prBssInfo)->eCurrentOPMode = OP_MODE_INFRASTRUCTURE; \
	COPY_MAC_ADDR((_prBssInfo)->aucBSSID, _aucZeroMacAddr); \
	LINK_INITIALIZE(&((_prBssInfo)->rStaRecOfClientList)); \
	(_prBssInfo)->fgIsBeaconActivated = FALSE; \
	(_prBssInfo)->u2HwDefaultFixedRateCode = RATE_CCK_1M_LONG; \
}

/*----------------------------------------------------------------------------*/
/* Macros for Power State                                                     */
/*----------------------------------------------------------------------------*/
#define SET_NET_PWR_STATE_IDLE(_prAdapter, _BssIndex) \
	{_prAdapter->rWifiVar.aePwrState[(_BssIndex)] = PWR_STATE_IDLE; }

#define SET_NET_PWR_STATE_ACTIVE(_prAdapter, _BssIndex) \
	{_prAdapter->rWifiVar.aePwrState[(_BssIndex)] = PWR_STATE_ACTIVE; }

#define SET_NET_PWR_STATE_PS(_prAdapter, _BssIndex) \
	{_prAdapter->rWifiVar.aePwrState[(_BssIndex)] = PWR_STATE_PS; }

#define IS_NET_PWR_STATE_ACTIVE(_prAdapter, _BssIndex) \
	(_prAdapter->rWifiVar.aePwrState[(_BssIndex)] == PWR_STATE_ACTIVE)

#define IS_NET_PWR_STATE_IDLE(_prAdapter, _BssIndex) \
	(_prAdapter->rWifiVar.aePwrState[(_BssIndex)] == PWR_STATE_IDLE)

#define IS_SCN_PWR_STATE_ACTIVE(_prAdapter) \
	(_prAdapter->rWifiVar.rScanInfo.eScanPwrState == SCAN_PWR_STATE_ACTIVE)

#define IS_SCN_PWR_STATE_IDLE(_prAdapter) \
	(_prAdapter->rWifiVar.rScanInfo.eScanPwrState == SCAN_PWR_STATE_IDLE)

#define IS_WIFI_2G4_WF0_SUPPORT(_prAdapter) \
	((_prAdapter)->rWifiFemCfg.u2WifiPath & WLAN_FLAG_2G4_WF0)

#define IS_WIFI_5G_WF0_SUPPORT(_prAdapter) \
	((_prAdapter)->rWifiFemCfg.u2WifiPath & WLAN_FLAG_5G_WF0)

#define IS_WIFI_2G4_WF1_SUPPORT(_prAdapter) \
	((_prAdapter)->rWifiFemCfg.u2WifiPath & WLAN_FLAG_2G4_WF1)

#define IS_WIFI_5G_WF1_SUPPORT(_prAdapter) \
	((_prAdapter)->rWifiFemCfg.u2WifiPath & WLAN_FLAG_5G_WF1)

#define IS_WIFI_6G_WF0_SUPPORT(_prAdapter) \
	((_prAdapter)->rWifiFemCfg.u2WifiPath6G & WLAN_FLAG_6G_WF0)

#define IS_WIFI_6G_WF1_SUPPORT(_prAdapter) \
	((_prAdapter)->rWifiFemCfg.u2WifiPath6G & WLAN_FLAG_6G_WF1)

#define IS_WIFI_2G4_SISO(_prAdapter) \
	((IS_WIFI_2G4_WF0_SUPPORT(_prAdapter) && \
	!(IS_WIFI_2G4_WF1_SUPPORT(_prAdapter))) || \
	(IS_WIFI_2G4_WF1_SUPPORT(_prAdapter) && \
	!(IS_WIFI_2G4_WF0_SUPPORT(_prAdapter))))

#define IS_WIFI_5G_SISO(_prAdapter) \
	((IS_WIFI_5G_WF0_SUPPORT(_prAdapter) && \
	!(IS_WIFI_5G_WF1_SUPPORT(_prAdapter))) || \
	(IS_WIFI_5G_WF1_SUPPORT(_prAdapter) && \
	!(IS_WIFI_5G_WF0_SUPPORT(_prAdapter))))

#define IS_WIFI_6G_SISO(_prAdapter) \
	((IS_WIFI_6G_WF0_SUPPORT(_prAdapter) && \
	!(IS_WIFI_6G_WF1_SUPPORT(_prAdapter))) || \
	(IS_WIFI_6G_WF1_SUPPORT(_prAdapter) && \
	!(IS_WIFI_6G_WF0_SUPPORT(_prAdapter))))

#define IS_WIFI_SMART_GEAR_SUPPORT_WF0_SISO(_prAdapter) \
	((_prAdapter)->ucSmarGearSupportSisoOnly && \
	((_prAdapter)->ucSmartGearWfPathSupport == BIT(ANTENNA_WF0)))

#define IS_WIFI_SMART_GEAR_SUPPORT_WF1_SISO(_prAdapter) \
	((_prAdapter)->ucSmarGearSupportSisoOnly && \
	((_prAdapter)->ucSmartGearWfPathSupport == BIT(ANTENNA_WF1)))

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _ADAPTER_H */

