/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
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
#if CFG_SUPPORT_PASSPOINT
#include "hs20.h"
#endif /* CFG_SUPPORT_PASSPOINT */
#include "gl_os.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define MIN_TX_DURATION_TIME_MS 100

#if CFG_SUPPORT_CSI
#define CSI_RING_SIZE 1000
#define CSI_MAX_DATA_COUNT 256
#define CSI_MAX_RSVD1_COUNT 10

#define CSI_H_max_Index 4		/* for 2x2 support*/
#define CSI_MAX_RSVD2_COUNT 10
#define Max_Stream_Bytes 3000

#if CFG_SUPPORT_CSI_NF
#define CSI_DCTONE_NUM 3
#define CSI_NOISE_TH 20
#define CSI_NOISE_CNT_TH 50

#define CSI_DBW20_RATIO_TH 2
#define CSI_DBW40_RATIO_TH 2
#endif
#endif

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
	ENUM_SW_TEST_MODE_SIGMA_VOICE_ENT = 0x14
};

struct ESS_SCAN_RESULT_T {
	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint16_t u2SSIDLen;
	uint8_t aucSSID[PARAM_MAX_LEN_SSID];
};

struct WLAN_INFO {
	struct PARAM_BSSID_EX rCurrBssId[KAL_AIS_NUM];

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
	struct CMD_LINK_ATTRIB eLinkAttr;
/* CMD_PS_PROFILE_T         ePowerSaveMode; */
	struct CMD_PS_PROFILE arPowerSaveMode[MAX_BSSID_NUM];

	/* Support power save flag for the caller */
	uint32_t u4PowerSaveFlag[MAX_BSSID_NUM];

	/* trigger parameter */
	enum ENUM_RSSI_TRIGGER_TYPE eRssiTriggerType;
	int32_t rRssiTriggerValue;

	/* Privacy Filter */
	enum ENUM_PARAM_PRIVACY_FILTER ePrivacyFilter;

	/* RTS Threshold */
	uint32_t eRtsThreshold;

	/* Network Type */
	uint8_t ucNetworkType[KAL_AIS_NUM];

	/* Network Type In Use */
	uint8_t ucNetworkTypeInUse;

	/* Force enable/disable power save mode*/
	u_int8_t fgEnSpecPwrMgt;

};

/* Session for CONNECTION SETTINGS */
struct CONNECTION_SETTINGS {

	uint8_t aucMacAddress[MAC_ADDR_LEN];

	uint8_t ucDelayTimeOfDisconnectEvent;

	u_int8_t fgIsConnByBssidIssued;
	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint8_t aucBSSIDHint[MAC_ADDR_LEN];

	u_int8_t fgIsConnReqIssued;
	u_int8_t fgIsDisconnectedByNonRequest;
	enum ENUM_RECONNECT_LEVEL_T eReConnectLevel;

	uint8_t ucSSIDLen;
	uint8_t aucSSID[ELEM_MAX_LEN_SSID];

	enum ENUM_PARAM_OP_MODE eOPMode;

	enum ENUM_PARAM_CONNECTION_POLICY eConnectionPolicy;

	enum ENUM_PARAM_AD_HOC_MODE eAdHocMode;

	enum ENUM_PARAM_AUTH_MODE eAuthMode;

	enum ENUM_WEP_STATUS eEncStatus;

	u_int8_t fgIsScanReqIssued;

	/* MIB attributes */
	uint16_t u2BeaconPeriod;

	uint16_t u2RTSThreshold;	/* User desired setting */

	uint16_t u2DesiredNonHTRateSet;	/* User desired setting */

	uint8_t ucAdHocChannelNum;	/* For AdHoc */

	enum ENUM_BAND eAdHocBand;	/* For AdHoc */

	uint32_t u4FreqInKHz;	/* Center frequency */

	/* ATIM windows using for IBSS power saving function */
	uint16_t u2AtimWindow;

	/* Features */
	u_int8_t fgIsEnableRoaming;

	u_int8_t fgIsAdHocQoSEnable;

	enum ENUM_PARAM_PHY_CONFIG eDesiredPhyConfig;

#if CFG_SUPPORT_802_11D
	u_int8_t fgMultiDomainCapabilityEnabled;
#endif				/* CFG_SUPPORT_802_11D */

#if 1				/* CFG_SUPPORT_WAPI */
	u_int8_t fgWapiMode;
	uint32_t u4WapiSelectedGroupCipher;
	uint32_t u4WapiSelectedPairwiseCipher;
	uint32_t u4WapiSelectedAKMSuite;
	uint8_t aucWapiAssocInfoIEs[42];
	uint16_t u2WapiAssocInfoIESz;
#endif
	uint8_t aucWSCAssocInfoIE[200];	/*for Assoc req */
	uint16_t u2WSCAssocInfoIELen;

	/* for cfg80211 connected indication */
	uint32_t u4RspIeLength;
	uint8_t aucRspIe[CFG_CFG80211_IE_BUF_LEN];

	uint32_t u4ReqIeLength;
	uint8_t aucReqIe[CFG_CFG80211_IE_BUF_LEN];

	u_int8_t fgWpsActive;
	uint8_t aucWSCIE[GLUE_INFO_WSCIE_LENGTH];	/*for probe req */
	uint16_t u2WSCIELen;

	/*
	 * Buffer to hold non-wfa vendor specific IEs set
	 * from wpa_supplicant. This is used in sending
	 * Association Request in AIS mode.
	 */
	uint16_t non_wfa_vendor_ie_len;
	uint8_t non_wfa_vendor_ie_buf[NON_WFA_VENDOR_IE_MAX_LEN];

	/* 11R */
	struct FT_IES rFtIeForTx;
	struct cfg80211_ft_event_params rFtEventParam;

	/* CR1486, CR1640 */
	/* for WPS, disable the privacy check for AP selection policy */
	u_int8_t fgPrivacyCheckDisable;

	/* b0~3: trigger-en AC0~3. b4~7: delivery-en AC0~3 */
	uint8_t bmfgApsdEnAc;

	/* for RSN info store, when upper layer set rsn info */
	struct RSN_INFO rRsnInfo;

	u_int8_t fgSecModeChangeStartTimer;
#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
	struct cfg80211_bss *bss;

	u_int8_t fgIsConnInitialized;

	u_int8_t fgIsSendAssoc;

	u_int8_t ucAuthDataLen;
	u_int8_t aucAuthData[AUTH_DATA_MAX_LEN];
		/* Additional elements for Authentication frame,
		* starts with transaction sequence number field
		*/
		/* Temp assign a fixed large length AUTH_DATA_MAX_LEN */
	u_int8_t ucChannelNum;
	uint8_t  ucRoleIdx;
#endif
#if CFG_SUPPORT_OWE
	    /* for OWE info store, when upper layer set rsn info */
	    struct OWE_INFO rOweInfo;
#endif
#if CFG_SUPPORT_WPA3_H2E
	struct RSNXE rRsnXE;
#endif
};

struct BSS_INFO {

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

	u_int8_t fgIsInUse;	/* For CNM to assign BSS_INFO */
	u_int8_t fgIsNetActive;	/* TRUE if this network has been activated */

	uint8_t ucBssIndex;	/* BSS_INFO_T index */

	uint8_t ucReasonOfDisconnect;	/* Used by media state indication */

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

#if CFG_ENABLE_GTK_FRAME_FILTER
	struct IPV4_NETWORK_ADDRESS_LIST *prIpV4NetAddrList;
#endif
	uint16_t u2DeauthReason;

#if CFG_SUPPORT_TDLS
	u_int8_t fgTdlsIsProhibited;
	u_int8_t fgTdlsIsChSwProhibited;
#endif

	/*link layer statistics */
	struct WIFI_WMM_AC_STAT arLinkStatistics[WMM_AC_INDEX_NUM];

	uint32_t u4CoexPhyRateLimit;

	u_int8_t fgIsGranted;
	enum ENUM_BAND eBandGranted;
	uint8_t ucPrimaryChannelGranted;
	struct PARAM_CUSTOM_ACL rACL;

#if CFG_SUPPORT_802_11W
	/* AP PMF */
	struct AP_PMF_CFG rApPmfCfg;
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
	struct _DL_LIST twt_sch_link; /* twt sch-link */
	struct _TWT_HOTSPOT_STA_NODE arTWTSta[TWT_MAX_FLOW_NUM];
#endif

#if (CFG_SUPPORT_P2P_CSA == 1)
	struct TIMER rCsaTimer;
	struct SWITCH_CH_AND_BAND_PARAMS CSAParams;
	uint8_t ucVhtChannelWidthBeforeCsa;
	uint8_t fgHasStopTx;
	u_int8_t fgIsSwitchingChnl;
#endif
};

/* Support AP Selection */
struct ESS_CHNL_INFO {
	uint8_t ucChannel;
	uint8_t ucUtilization;
	uint8_t ucApNum;
};
/* end Support AP Selection */

struct NEIGHBOR_AP_T {
	struct LINK_ENTRY rLinkEntry;
	uint8_t aucBssid[MAC_ADDR_LEN];
	u_int8_t fgHT:1;
	u_int8_t fgSameMD:1;
	u_int8_t fgRmEnabled:1;
	u_int8_t fgFromBtm:1;
	u_int8_t fgQoS:1;
	uint8_t ucReserved:3;
	u_int8_t fgPrefPresence;
	uint8_t ucPreference;
	uint8_t ucChannel;
	uint64_t u8TermTsf;
};

struct AIS_SPECIFIC_BSS_INFO {
	/* This value indicate the roaming type used in AIS_JOIN */
	uint8_t ucRoamingAuthTypes;

	u_int8_t fgIsIBSSActive;

	/*! \brief Global flag to let arbiter stay at standby
	 *  and not connect to any network
	 */
	u_int8_t fgCounterMeasure;

#if 0
	u_int8_t fgWepWapiBcKeyExist;	/* WEP WAPI BC key exist flag */
	uint8_t ucWepWapiBcWlanIndex;	/* WEP WAPI BC wlan index */

	/* RSN BC key exist flag, map to key id 0, 1, 2, 3 */
	u_int8_t fgRsnBcKeyExist[4];
	/* RSN BC wlan index, map to key id 0, 1, 2, 3 */
	uint8_t ucRsnBcWlanIndex[4];
#endif

	/* While Do CounterMeasure procedure,
	 * check the EAPoL Error report have send out
	 */
	u_int8_t fgCheckEAPoLTxDone;

	uint32_t u4RsnaLastMICFailTime;

	/* By the flow chart of 802.11i,
	 *  wait 60 sec before associating to same AP
	 *  or roaming to a new AP
	 *  or sending data in IBSS,
	 *  keep a timer for handle the 60 sec counterMeasure
	 */
	struct TIMER rRsnaBlockTrafficTimer;
	struct TIMER rRsnaEAPoLReportTimeoutTimer;

	/* For Keep the Tx/Rx Mic key for TKIP SW Calculate Mic */
	/* This is only one for AIS/AP */
	uint8_t aucTxMicKey[8];
	uint8_t aucRxMicKey[8];

	/* Buffer for WPA2 PMKID */
	/* The PMKID cache lifetime is expire by media_disconnect_indication */
	struct LINK rPmkidCache;
#if CFG_SUPPORT_802_11W
	u_int8_t fgMgmtProtection;
	uint32_t u4SaQueryStart;
	uint32_t u4SaQueryCount;
	uint8_t ucSaQueryTimedOut;
	uint8_t *pucSaQueryTransId;
	struct TIMER rSaQueryTimer;
	u_int8_t fgBipKeyInstalled;
#endif
	uint8_t ucKeyAlgorithmId;

	/* Support AP Selection */
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
	uint8_t	ucRoamSkipTimes;
	u_int8_t fgGoodRcpiArea;
	u_int8_t fgPoorRcpiArea;
#endif
	struct ESS_CHNL_INFO arCurEssChnlInfo[CFG_MAX_NUM_OF_CHNL_INFO];
	uint8_t ucCurEssChnlInfoNum;
	struct LINK rCurEssLink;
	/* end Support AP Selection */

	struct BSS_TRANSITION_MGT_PARAM_T rBTMParam;
	struct LINK_MGMT  rNeighborApList;
	OS_SYSTIME rNeiApRcvTime;
	uint32_t u4NeiApValidInterval;
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
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t uc6GBandwidthMode;	/* 20/40M or 20M only *//* Not used */
#endif
	/* Support AP Selection */
	struct LINK_MGMT rBlackList;
#if CFG_SUPPORT_MBO
	struct PARAM_BSS_DISALLOWED_LIST rBssDisallowedList;
#endif
	enum ENUM_PARAM_PHY_CONFIG eDesiredPhyConfig;
	/* Common connection settings end */

	struct CONNECTION_SETTINGS rConnSettings[KAL_AIS_NUM];

	struct SCAN_INFO rScanInfo;

#if CFG_SUPPORT_ROAMING
	struct ROAMING_INFO rRoamingInfo[KAL_AIS_NUM];
#endif				/* CFG_SUPPORT_ROAMING */

	struct AIS_FSM_INFO rAisFsmInfo[KAL_AIS_NUM];

	enum ENUM_PWR_STATE aePwrState[MAX_BSSID_NUM + 1];

	struct BSS_INFO arBssInfoPool[MAX_BSSID_NUM];

	struct BSS_INFO rP2pDevInfo;

	struct AIS_SPECIFIC_BSS_INFO rAisSpecificBssInfo[KAL_AIS_NUM];

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
	uint8_t aucMacAddress[MAC_ADDR_LEN];
	uint8_t aucMacAddress1[MAC_ADDR_LEN];
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

#if CFG_SUPPORT_PASSPOINT
	struct HS20_INFO rHS20Info[KAL_AIS_NUM];
#endif				/* CFG_SUPPORT_PASSPOINT */
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
#endif
	uint8_t ucApHt;
	uint8_t ucApVht;
	uint8_t ucP2pGoHt;
	uint8_t ucP2pGoVht;
	uint8_t ucP2pGcHt;
	uint8_t ucP2pGcVht;
	uint8_t ucDisP2pPs;

	/* NIC capability from FW event*/
	uint8_t ucHwNotSupportAC;
	uint8_t ucHwNotSupportAX;

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
	uint8_t ucHeOMCtrl;
#endif
#if (CFG_SUPPORT_TWT == 1)
	uint8_t ucTWTRequester;
	uint8_t ucTWTResponder;
	uint8_t ucTWTStaBandBitmap;
#endif
#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	uint8_t ucTWTHotSpotSupport;
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

	uint8_t ucApSco;
	uint8_t ucP2pGoSco;

	uint8_t ucStaBandwidth;
	uint8_t ucSta5gBandwidth;
	uint8_t ucSta2gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucSta6gBandwidth;
#endif
	uint8_t ucApBandwidth;
	uint8_t ucAp2gBandwidth;
	uint8_t ucAp5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucAp6gBandwidth;
#endif
	uint8_t ucP2p5gBandwidth;
	uint8_t ucP2p2gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucP2p6gBandwidth;
#endif

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
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucAp6gNSS; /* Less or euqal than ucNss */
#endif
	uint8_t ucAp5gNSS; /* Less or euqal than ucNss */
	uint8_t ucAp2gNSS; /* Less or euqal than ucNss */
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucGo6gNSS; /* Less or euqal than ucNss */
#endif
	uint8_t ucGo5gNSS; /* Less or euqal than ucNss */
	uint8_t ucGo2gNSS; /* Less or euqal than ucNss */

	uint8_t ucRxMaxMpduLen;
	uint32_t u4TxMaxAmsduInAmpduLen;

	uint8_t ucTxBaSize;
	uint8_t ucRxHtBaSize;
	uint8_t ucRxVhtBaSize;
#if (CFG_SUPPORT_802_11AX == 1)
	uint16_t u2RxHeBaSize;
	uint16_t u2TxHeBaSize;
#endif

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
	u_int8_t fgCsaInProgress;
	uint8_t ucChannelSwitchMode;
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
	uint8_t ucEapolOffload; /* eapol offload when active mode / wow mode */
	uint8_t ucMobileLikeSuspend; /* allow mobile like suspend */
	/* set wow detect type, default 1 (magic packet) */
	/* bit 0, magic packet */
	/* bit 1, any */
	/* bit 2, disconnect */
	/* bit 3, GTK rekey failure */
	/* bit 4, beacon lost */
	uint8_t ucWowDetectType;

	uint8_t u4SwTestMode;
	uint8_t	ucCtrlFlagAssertPath;
	uint8_t	ucCtrlFlagDebugLevel;
	uint32_t u4WakeLockRxTimeout;
	uint32_t u4WakeLockThreadWakeup;
	uint32_t u4RegP2pIfAtProbe; /* register p2p interface during probe */
	/* p2p group interface use the same mac addr as p2p device interface */
	uint8_t ucP2pShareMacAddr;
	uint8_t ucSmartRTS;

	uint32_t u4UapsdAcBmp;
	uint32_t u4MaxSpLen;
	/* 0: enable online scan, non-zero: disable online scan */
	uint32_t fgDisOnlineScan;
	uint32_t fgDisBcnLostDetection;
	uint32_t fgDisRoaming;		/* 0:enable roaming 1:disable */
	uint32_t u4AisRoamingNumber;
	uint32_t fgEnArpFilter;

	uint8_t	uDeQuePercentEnable;
	uint32_t	u4DeQuePercentVHT80Nss1;
	uint32_t	u4DeQuePercentVHT40Nss1;
	uint32_t	u4DeQuePercentVHT20Nss1;
	uint32_t	u4DeQuePercentHT40Nss1;
	uint32_t	u4DeQuePercentHT20Nss1;

	uint32_t u4PerfMonUpdatePeriod;
	uint32_t u4PerfMonTpTh[PERF_MON_TP_MAX_THRESHOLD];
	uint32_t	u4BoostCpuTh;

	u_int8_t fgTdlsBufferSTASleep; /* Support TDLS 5.5.4.2 optional case */
	u_int8_t fgChipResetRecover;
	enum PARAM_POWER_MODE ePowerMode;

	u_int8_t fgNvramCheckEn; /* nvram checking in scan result*/

	u_int8_t fgEnableSerL0;
	enum ENUM_FEATURE_OPTION_IN_SER eEnableSerL0p5;
	enum ENUM_FEATURE_OPTION_IN_SER eEnableSerL1;

#if CFG_SUPPORT_SPE_IDX_CONTROL
	u_int8_t ucSpeIdxCtrl;	/* 0: WF0, 1: WF1, 2: duplicate */
#if CFG_SUPPORT_COEX_NON_COTX
	u_int8_t ucSpeIdxCtrl2g; /* 0:WF0, 1:WF1, 2: both WF0/1 only in 2G*/
	u_int8_t fgCoexNonCoTx; /* 0(default):allow co-Tx,1:disallow co-Tx*/
#endif
#endif

#if CFG_SUPPORT_LOWLATENCY_MODE
	uint8_t ucLowLatencyModeScan;
	uint8_t ucLowLatencyModeReOrder;
	uint8_t ucLowLatencyModePower;
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */
#if CFG_SUPPORT_IDC_CH_SWITCH
	uint8_t ucChannelSwtichColdownTime;
	uint8_t fgCrossBandSwitchEn;
#endif
#if CFG_SUPPORT_PERF_IND
	u_int8_t fgPerfIndicatorEn;
#endif

	/* 11K */
	struct RADIO_MEASUREMENT_REQ_PARAMS rRmReqParams[KAL_AIS_NUM];
	struct RADIO_MEASUREMENT_REPORT_PARAMS rRmRepParams[KAL_AIS_NUM];

	/* WMMAC */
	struct WMM_INFO rWmmInfo[KAL_AIS_NUM];

	/* Tx Msdu Queue method */
	uint8_t ucTxMsduQueue;

	uint32_t u4MTU; /* net device maximum transmission unit */
#if CFG_SUPPORT_RX_GRO
	uint32_t ucGROFlushTimeout; /* Flush packet timeout (ms) */
	uint32_t ucGROEnableTput; /* Threshold of enable GRO Tput */
#endif
	uint32_t ucTputThresholdMbps;

#if CFG_SUPPORT_IOT_AP_BLACKLIST
	uint8_t fgEnDefaultIotApRule;
#endif
	uint8_t ucMsduReportTimeout;

#if CFG_SUPPORT_DATA_STALL
	uint32_t u4PerHighThreshole;
	uint32_t u4TxLowRateThreshole;
	uint32_t u4RxLowRateThreshole;
	uint32_t u4ReportEventInterval;
	uint32_t u4TrafficThreshold;
#endif

	uint8_t fgReuseRSNIE;
#if CFG_SUPPORT_HE_ER
	uint8_t u4ExtendedRange;
	u_int8_t fgErTx;
	u_int8_t fgErRx;
#endif

#if (CFG_SUPPORT_P2P_CSA == 1)
	uint8_t ucP2pCsaCount;
#endif

#if (CFG_SUPPORT_P2PGO_ACS == 1)
	uint8_t ucP2pGoACS;
#endif

#if CFG_SUPPORT_TPENHANCE_MODE
	uint8_t ucTpEnhanceEnable;
	uint8_t ucTpEnhancePktNum;
	uint32_t u4TpEnhanceInterval; /* in us */
	int8_t cTpEnhanceRSSI;
	uint32_t u4TpEnhanceThreshold;
#endif /* CFG_SUPPORT_TPENHANCE_MODE */

#if (CFG_COALESCING_INTERRUPT == 1)
	uint16_t u2CoalescingIntMaxPk;
	uint16_t u2CoalescingIntMaxTime;
	uint16_t u2CoalescingIntuFilterMask;
	u_int8_t fgCoalescingIntEn;
	uint32_t u4PerfMonTpCoalescingIntTh;
#endif

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
	int32_t i4Ed2GNonEU;
	int32_t i4Ed5GNonEU;
	int32_t i4Ed2GEU;
	int32_t i4Ed5GEU;
#endif

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
	uint8_t ucNanBandwidth;
	uint8_t fgEnNanVHT;
	uint8_t ucNanFtmBw;
	uint8_t ucNanDiscBcnInterval;
	uint8_t ucNanCommittedDw;
	unsigned char fgNoPmf;
	uint8_t fgNanIsSigma;
#endif
#if CFG_SUPPORT_WAC
	uint16_t u2WACIELen;
	uint8_t aucWACIECache[ELEM_MAX_LEN_WAC_INFO];
	bool fgEnableWACIE;
#endif
#if CFG_SUPPORT_ONE_TIME_CAL
	uint8_t fgOneTimeCalEnable;
#endif
	uint32_t u4P2pGoWmmParamAC0;
	uint32_t u4P2pGoWmmParamAC1;
	uint32_t u4P2pGoWmmParamAC2;
	uint32_t u4P2pGoWmmParamAC3;
};

/* cnm_timer module */
struct ROOT_TIMER {
	struct LINK rLinkHead;
	OS_SYSTIME rNextExpiredSysTime;
	KAL_WAKE_LOCK_T *prWakeLock;
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

#if CFG_SUPPORT_NCHO
enum _ENUM_NCHO_ITEM_SET_TYPE_T {
	ITEM_SET_TYPE_NUM,
	ITEM_SET_TYPE_STR
};

enum _ENUM_NCHO_BAND_T {
	NCHO_BAND_AUTO = 0,
	NCHO_BAND_5G,
	NCHO_BAND_2G4,
	NCHO_BAND_NUM
};

enum _ENUM_NCHO_DFS_SCN_MODE_T {
	NCHO_DFS_SCN_DISABLE = 0,
	NCHO_DFS_SCN_ENABLE1,
	NCHO_DFS_SCN_ENABLE2,
	NCHO_DFS_SCN_NUM
};

struct _CFG_NCHO_RE_ASSOC_T {
	/*!< SSID length in bytes. Zero length is broadcast(any) SSID */
	uint32_t u4SsidLen;
	uint8_t aucSsid[ELEM_MAX_LEN_SSID];
	uint8_t aucBssid[MAC_ADDR_LEN];
	uint32_t u4CenterFreq;
};

struct _CFG_NCHO_SCAN_CHNL_T {
	uint8_t ucChannelListNum;
	struct RF_CHANNEL_INFO arChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];
};

struct _NCHO_ACTION_FRAME_PARAMS_T {
	uint8_t aucBssid[MAC_ADDR_LEN];
	int32_t i4channel;
	int32_t i4DwellTime;
	int32_t i4len;
	uint8_t aucData[520];
};

struct _NCHO_AF_INFO_T {
	uint8_t *aucBssid;
	int32_t i4channel;
	int32_t i4DwellTime;
	int32_t i4len;
	uint8_t *pucData;
};

struct _NCHO_INFO_T {
	u_int8_t fgECHOEnabled;
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
	enum _ENUM_NCHO_BAND_T eBand;
	enum _ENUM_NCHO_DFS_SCN_MODE_T eDFSScnMode;
	uint32_t u4RoamScanControl;
	struct _CFG_NCHO_SCAN_CHNL_T rRoamScnChnl;
	struct _NCHO_ACTION_FRAME_PARAMS_T rParamActionFrame;
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
struct PERF_MONITOR_T {
	struct TIMER rPerfMonTimer;
	unsigned long ulPerfMonFlag;
	unsigned long ulLastTxBytes[BSS_DEFAULT_NUM];
	unsigned long ulLastRxBytes[BSS_DEFAULT_NUM];
	uint64_t ulThroughput; /* in bps */
	unsigned long ulTxTp[BSS_DEFAULT_NUM]; /* in Bps */
	unsigned long ulRxTp[BSS_DEFAULT_NUM]; /* in Bps */
	uint32_t u4UpdatePeriod; /* in ms */
	uint32_t u4TarPerfLevel;
	uint32_t u4CurrPerfLevel;
	uint32_t u4UsedCnt;
	unsigned long ulTotalTxSuccessCount;
	unsigned long ulTotalTxFailCount;
};

#if CFG_SUPPORT_CSI
/*
 * CSI_DATA_T is used for representing
 * the CSI and other useful * information
 * for application usage
 */
struct CSI_DATA_T {
	uint8_t ucFwVer;
	uint8_t ucBw;
	bool bIsCck;
	uint16_t u2DataCount;
	int16_t ac2IData[CSI_MAX_DATA_COUNT];
	int16_t ac2QData[CSI_MAX_DATA_COUNT];
	uint8_t ucDbdcIdx;
	int8_t cRssi;
	uint8_t ucSNR;
	uint64_t u8TimeStamp;
	uint8_t ucDataBw;
	uint8_t ucPrimaryChIdx;
	uint8_t aucTA[MAC_ADDR_LEN];
	uint32_t u4ExtraInfo;
	uint8_t ucRxMode;
	uint16_t u2RxRate;
	int32_t ai4Rsvd1[CSI_MAX_RSVD1_COUNT];
	int32_t au4Rsvd2[CSI_MAX_RSVD2_COUNT];
	uint8_t ucRsvd1Cnt;
	uint8_t ucRsvd2Cnt;
	int32_t i4Rsvd3;
	uint8_t ucRsvd4;
	uint32_t Antenna_pattern;
	uint32_t u4TRxIdx;
};

/*
 * CSI_INFO_T is used to store the CSI
 * settings and CSI event data
 */
struct CSI_INFO_T {
	/* Variables for manipulate the CSI data in g_aucProcBuf */
	bool bIncomplete;
	int32_t u4CopiedDataSize;
	int32_t u4RemainingDataSize;
	wait_queue_head_t waitq;
	/* Variable for recording the CSI function config */
	uint8_t ucMode;
	uint8_t ucValue1[CSI_CONFIG_ITEM_NUM];
	uint8_t ucValue2[CSI_CONFIG_ITEM_NUM];
	/* Variable for manipulating the CSI ring buffer */
	struct CSI_DATA_T arCSIBuffer[CSI_RING_SIZE];
	uint32_t u4CSIBufferHead;
	uint32_t u4CSIBufferTail;
	uint32_t u4CSIBufferUsed;
	int16_t ai2TempIData[CSI_MAX_DATA_COUNT];
	int16_t ai2TempQData[CSI_MAX_DATA_COUNT];
	/*for usr to get the specific H(jw), 0 for all */
	uint16_t Matrix_Get_Bit;
	uint8_t byte_stream[Max_Stream_Bytes];/*send bytes to proc interfacel */
};
#endif

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
	struct BSS_INFO *prAisBssInfo[KAL_AIS_NUM];
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
#if (CFG_TX_DYN_CMD_SUPPORT == 1)
	struct QUE rDynCmdQ;
	uint32_t u4DynCmdAllocCnt;	/* Debug only */
#endif

	/* Element for RX PATH */
	struct RX_CTRL rRxCtrl;

	/* Timer for restarting RFB setup procedure */
	struct TIMER rPacketDelaySetupTimer;

	uint32_t u4IntStatus;

	enum ENUM_ACPI_STATE rAcpiState;

	u_int8_t fgIsIntEnable;
	u_int8_t fgIsIntEnableWithLPOwnSet;

	u_int8_t fgIsFwOwn;
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
	/* check if an empty MsduInfo is available */
	struct timer_list rTxDirectSkbTimer;
	/* check if HIF port is ready to accept a new Msdu */
	struct timer_list rTxDirectHifTimer;

	struct sk_buff_head rTxDirectSkbQueue;
	struct QUE rTxDirectHifQueue[TX_PORT_NUM];

#if (CFG_SUPPORT_TX_TSO_SW == 1)
	struct sk_buff_head rTsoQueue;
	uint32_t u4TsoQueueCnt;
#endif

	struct QUE rStaPsQueue[CFG_STA_REC_NUM];
	uint32_t u4StaPsBitmap;
	struct QUE rBssAbsentQueue[MAX_BSSID_NUM + 1];
	uint32_t u4BssAbsentBitmap;
	struct QUE rStaPendQueue[CFG_STA_REC_NUM];
	uint32_t u4StaPendBitmap;
	/* TX Direct related : END */

	struct QUE rPendingCmdQueue;

#if CFG_SUPPORT_MULTITHREAD
	struct QUE rTxCmdQueue;
	struct QUE rTxCmdDoneQueue;
#if CFG_REDIRECT_OID_SUPPORT
	struct QUE rOidQueue;
#endif
#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
#if CFG_SUPPORT_CFG80211_QUEUE
	struct QUE rCfg80211Queue;
#endif
#endif
#if CFG_FIX_2_TX_PORT
	struct QUE rTxP0Queue;
	struct QUE rTxP1Queue;
#else
	struct QUE rTxPQueue[TX_PORT_NUM];
#endif
	struct QUE rRxQueue;
	struct QUE rTxDataDoneQueue;
#endif

	struct GLUE_INFO *prGlueInfo;

	uint8_t ucCmdSeqNum;
	uint8_t ucTxSeqNum;
	uint8_t aucPidPool[WTBL_SIZE];

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

#if CFG_SUPPORT_MSP
	struct EVENT_WLAN_INFO rEventWlanInfo;
#endif

	struct LINK_QUALITY rLinkQuality;
	OS_SYSTIME rLinkQualityUpdateTime;
	u_int8_t fgIsLinkQualityValid;
	OS_SYSTIME rLinkRateUpdateTime;
	u_int8_t fgIsLinkRateValid;

	/* WIFI_VAR_T */
	struct WIFI_VAR rWifiVar;

	/* MTK WLAN NIC driver IEEE 802.11 MIB */
	struct IEEE_802_11_MIB rMib[KAL_AIS_NUM];

	/* Mailboxs for inter-module communication */
	struct MBOX arMbox[MBOX_ID_TOTAL_NUM];

	/* Timers for OID Pending Handling */
	struct TIMER rOidTimeoutTimer;
	uint8_t ucOidTimeoutCount;

	/* Root Timer for cnm_timer module */
	struct ROOT_TIMER rRootTimer;

#if CFG_CHIP_RESET_SUPPORT
	enum ENUM_WFSYS_RESET_STATE_TYPE_T eWfsysResetState;
	spinlock_t rWfsysResetLock;
	u_int8_t fgWfsysResetPostpone;
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

	struct CNM_INFO rCnmInfo;

	uint32_t u4PowerMode;

	uint32_t u4CtiaPowerMode;
	u_int8_t fgEnCtiaPowerMode;

	/* Bitmap is defined as #define KEEP_FULL_PWR_{FEATURE}_BIT
	 *  in wlan_lib.h
	 * Each feature controls KeepFullPwr(CMD_ID_KEEP_FULL_PWR) should
	 * register bitmap to ensure low power during suspend.
	 */
	uint32_t u4IsKeepFullPwrBitmap;

	uint32_t fgEnArpFilter;

	uint32_t u4UapsdAcBmp;

	uint32_t u4MaxSpLen;

	uint32_t u4PsCurrentMeasureEn;

	u_int8_t fgInCtiaMode;

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
#endif

#if CFG_M0VE_BA_TO_DRIVER
	struct TIMER rMqmIdleRxBaDetectionTimer;
	uint32_t u4FlagBitmap;
#endif
#if CFG_ASSERT_DUMP
	struct TIMER rN9CorDumpTimer;
	struct TIMER rCr4CorDumpTimer;
	u_int8_t fgN9CorDumpFileOpend;
	u_int8_t fgCr4CorDumpFileOpend;
	u_int8_t fgN9AssertDumpOngoing;
	u_int8_t fgCr4AssertDumpOngoing;
	u_int8_t fgKeepPrintCoreDump;
#endif
#if CFG_SUPPORT_ONE_TIME_CAL
	u_int8_t fgOneTimeCalOngoing;
	struct ONE_TIME_CAL_FILE rOneTimeCalFile;
	uint8_t aucOneTimeCalFileBuf[100*1024];
	struct TIMER rOneTimeCalTimer;
#endif
#if CFG_SUPPORT_ICS
	u_int8_t fgIcsDumpFileOpend;
	u_int8_t fgIcsDumpOngoing;
	uint16_t u2IcsSeqNo;
#endif
	/* Tx resource information */
	u_int8_t fgIsNicTxReousrceValid;
	struct tx_resource_info nicTxReousrce;

    /* Efuse Start and End address */
	uint32_t u4EfuseStartAddress;
	uint32_t u4EfuseEndAddress;

	/* COEX feature */
	uint32_t u4FddMode;

#if CFG_WOW_SUPPORT
	struct WOW_CTRL	rWowCtrl;
	u_int8_t fgWowLinkDownPendFlag;
#if CFG_SUPPORT_MDNS_OFFLOAD
	struct MDNS_INFO_T rMdnsInfo;
	uint8_t mdns_offload_enable;
	uint8_t mdns_wake_flag;
#endif
#endif

#if CFG_SUPPORT_WOW_EINT
	struct WOWLAN_DEV_NODE rWowlanDevNode;
#endif
#if CFG_SUPPORT_WOW_EINT_KEYEVENT_WAKEUP
	struct input_dev *prWowInputDev;
#endif

#if CFG_SUPPORT_NCHO			/*  NCHO information */
	struct _NCHO_INFO_T rNchoInfo;
#endif

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

#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
	u_int8_t fgSuppSmeLinkDownPend;
#endif

	/* SER related info */
	uint8_t ucSerState;
	u_int8_t fgIsHifNotReady;
	uint32_t u4HifNotReadyTick;

#if (CFG_HW_WMM_BY_BSS == 1)
	uint8_t ucHwWmmEnBit;
#endif
	unsigned long ulSuspendFlag;
	struct WIFI_FEM_CFG rWifiFemCfg;

	/* Smar Gear */
	uint8_t ucSmarGearSupportSisoOnly;
	uint8_t ucSmartGearWfPathSupport;

	struct PERF_MONITOR_T rPerMonitor;
	struct ICAP_INFO_T rIcapInfo;
	struct RECAL_INFO_T rReCalInfo;

	/* Support change QM RX BA entry miss timeout (unit: ms) dynamically */
	uint32_t u4QmRxBaMissTimeout;

#if CFG_SUPPORT_LOWLATENCY_MODE
	u_int8_t fgEnLowLatencyMode;
	u_int8_t fgEnCfg80211Scan;
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
#endif /* CFG_SUPPORT_802_11AX == 1 */
#if (CFG_SUPPORT_TWT == 1)
	struct _TWT_PLANNER_T rTWTPlanner;
#endif

#if CFG_SUPPORT_WIFI_SYSDVT
	struct AUTOMATION_DVT *auto_dvt;
	uint16_t u2TxTest;
	uint16_t u2TxTestCount;
	uint8_t  ucTxTestUP;
#endif /* CFG_SUPPORT_WIFI_SYSDVT */

	bool fgEnHifDbgInfo;
	uint32_t u4HifDbgFlag;
	uint32_t u4HifChkFlag;
	uint32_t u4TxHangFlag;
	uint32_t u4NoMoreRfb;

	/* Only for PCIE DmaSchdl usage so far. */
	struct {
		bool fgRun;
		uint32_t u4Quota;
	} rWmmQuotaReqCS[BSS_DEFAULT_NUM];

#if CFG_SUPPORT_OSHARE
	bool fgEnOshareMode;
#endif

	struct WLAN_DEBUG_INFO rDebugInfo;
#if CFG_SUPPORT_IOT_AP_BLACKLIST
	struct WLAN_IOT_AP_RULE_T rIotApRule[CFG_IOT_AP_RULE_MAX_CNT];
#endif

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	uint32_t u4LastLinkQuality;
	uint32_t u4LinkQualityCounter;
	struct WIFI_LINK_QUALITY_INFO rLinkQualityInfo;
	struct PARAM_GET_STA_STATISTICS rQueryStaStatistics;
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
	int8_t cArpNoResponseIdx;

#if (CFG_SUPPORT_WIFI_RNR == 1)
	struct LINK rNeighborAPInfoList;
#endif

#if WLAN_INCLUDE_SYS
	u_int8_t fgEnDbgPowerMode;
#endif

#if CFG_SUPPORT_ICS
	u_int8_t fgEnTmacICS;
	u_int8_t fgEnRmacICS;
#endif

#if CFG_SUPPORT_PHY_ICS
	u_int8_t fgEnPhyICS;
#endif

#if (CFG_SUPPORT_WFDMA_REALLOC == 1)
	u_int8_t fgIsSupportWfdmaRealloc;
	u_int8_t fgWfdmaReallocDone;
	u_int8_t fgOidWfdmaReallocPend;
	uint32_t u4ReallocCmdTotalResource;
#endif /* CFG_SUPPORT_WFDMA_REALLOC */
	uint8_t CurNoResSeqID;
#if (CFG_SUPPORT_MAILBOX_ACK == 1)
	u_int8_t fgIsSupportMailBoxDriverOwnAck;
#endif

#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
	u_int8_t fgIsNeedDlPatch;
#endif

#if defined(_HIF_SDIO)
	u_int8_t fgGetMailBoxRWAck;
	/* For check mailbox r/w access */
	u_int8_t fgMBAccessFail;
#endif

#if CFG_SUPPORT_CSI
	struct CSI_INFO_T rCSIInfo;
#endif

#if (CFG_WIFI_GET_MCS_INFO == 1)
	struct TIMER rRxMcsInfoTimer;
	u_int8_t fgIsMcsInfoValid;
#endif
#if CFG_SUPPORT_EXCEPTION_STATISTICS
	uint32_t total_beacon_timeout_count;
	uint32_t beacon_timeout_count[BEACON_TIMEOUT_DUE_2_NUM];
	uint32_t total_tx_done_fail_count;
	uint32_t tx_done_fail_count[TX_RESULT_NUM];
	uint32_t total_deauth_rx_count;
	uint32_t deauth_rx_count[REASON_CODE_BEACON_TIMEOUT + 1];
	uint32_t total_scandone_timeout_count;
	uint32_t total_mgmtTX_timeout_count;
	uint32_t total_mgmtRX_timeout_count;
#endif
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
	((_prBssInfo)->eNetworkType == NETWORK_TYPE_AIS)

#define IS_BSS_INDEX_AIS(_prAdapter, _BssIndex) \
	(_BssIndex < KAL_AIS_NUM)

#define IS_BSS_P2P(_prBssInfo) \
	((_prBssInfo)->eNetworkType == NETWORK_TYPE_P2P)

#define IS_BSS_BOW(_prBssInfo) \
	((_prBssInfo)->eNetworkType == NETWORK_TYPE_BOW)

#define IS_BSS_APGO(_prBssInfo) \
	(IS_BSS_P2P(_prBssInfo) && \
	(_prBssInfo)->eCurrentOPMode == OP_MODE_ACCESS_POINT)

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
	(_prBssInfo)->ucReasonOfDisconnect = DISCONNECT_REASON_CODE_RESERVED; \
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
