/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   ais_fsm.h
 *  \brief  Declaration of functions and finite state machine for AIS Module.
 *
 *  Declaration of functions and finite state machine for AIS Module.
 */


#ifndef _AIS_FSM_H
#define _AIS_FSM_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define AIS_DEFAULT_INDEX (0)
#define AIS_SECONDARY_INDEX (1)

#define AIS_BG_SCAN_INTERVAL_MSEC           1000  /* MSEC */

#define AIS_DELAY_TIME_OF_DISCONNECT_SEC    5	/* 10 */

#define AIS_BEACON_TIMEOUT_COUNT_ADHOC      30
#define AIS_BEACON_TIMEOUT_COUNT_INFRA      10
#define AIS_BEACON_TIMEOUT_GUARD_TIME_SEC   1	/* Second */

#define AIS_BEACON_MAX_TIMEOUT_TU           100
#define AIS_BEACON_MIN_TIMEOUT_TU           5
#define AIS_BEACON_MAX_TIMEOUT_VALID        TRUE
#define AIS_BEACON_MIN_TIMEOUT_VALID        TRUE

#define AIS_BMC_MAX_TIMEOUT_TU              100
#define AIS_BMC_MIN_TIMEOUT_TU              5
#define AIS_BMC_MAX_TIMEOUT_VALID           TRUE
#define AIS_BMC_MIN_TIMEOUT_VALID           TRUE

#define AIS_JOIN_CH_GRANT_THRESHOLD         10
#if CFG_MTK_FPGA_PLATFORM
#define AIS_SCN_DONE_TIMEOUT_SEC            30 /* 30 for 2.4G + 5G */	/* 5 */
#define AIS_JOIN_CH_REQUEST_INTERVAL        40000
#define AIS_ACTION_FRAME_TX_LIFE_TIME_MS    0 /* 0 for no limit */
#else
#define AIS_SCN_DONE_TIMEOUT_SEC            15 /* 15 for 2.4G + 5G */	/* 5 */
#define AIS_JOIN_CH_REQUEST_INTERVAL        4000
#define AIS_ACTION_FRAME_TX_LIFE_TIME_MS    100
#endif
#define AIS_CSA_CH_REQUEST_INTERVAL         100

/* Support AP Selection*/
#define AIS_BLOCKLIST_TIMEOUT               15 /* seconds */
#if (CFG_SUPPORT_802_11BE_MLO == 1)
#define AIS_MLD_BLOCKLIST_TIMEOUT	    15 /* seconds */
#endif

#define AP_HASH_SIZE	256	/* Size of hash tab must be power of 2. */

#define AIS_CONNECTION_TRIAL_LIMIT	    10
#define AIS_BSS_TRIAL_LIMIT		    2
#define AIS_MLD_TRIAL_LIMIT		    1
#define AIS_TEMPORARY_REJECT_LIMIT	    1

#define AIS_MAIN_BSS_INDEX(_adapter, _ais_idx) \
	aisGetMainLinkBssIndex(_adapter, aisFsmGetInstance(_adapter, _ais_idx))

#define AIS_MAIN_BSS_INFO(_adapter, _ais_idx) \
	aisGetMainLinkBssInfo(aisFsmGetInstance(_adapter, _ais_idx))

#define AIS_MAIN_STA_REC(_adapter, _ais_idx) \
	aisGetMainLinkStaRec(aisFsmGetInstance(_adapter, _ais_idx))

#define AIS_LINK_NUM(_adapter, _ais_idx) \
	aisGetLinkNum(aisFsmGetInstance(_adapter, _ais_idx))

#define AIS_INDEX(_adapter, _bss_idx) \
	aisGetAisFsmInfo(_adapter, _bss_idx)->ucAisIndex

#define IS_AIS_CONN_BSSDESC(_ais_info, _bssdesc) \
	(!!(_bssdesc->fgIsConnected & aisGetBssIndexBmap(_ais_info)))

#if CFG_SUPPORT_ROAMING
#define IS_AIS_ROAMING(_adapter, _bss_idx) \
	(IS_BSS_INDEX_AIS(_adapter, _bss_idx) && \
	aisGetAisFsmInfo(_adapter, _bss_idx)->ucIsStaRoaming)
#else
#define IS_AIS_ROAMING(_adapter, _bss_idx) (FALSE)
#endif

#if CFG_SUPPORT_DFS
#define IS_AIS_CH_SWITCH(_bss_info) \
	(IS_BSS_AIS(_bss_info) && \
	(_bss_info->fgIsAisCsaPending || _bss_info->fgIsAisSwitchingChnl))
#else
#define IS_AIS_CH_SWITCH(_bss_info) (FALSE)
#endif

#define IS_AIS_OFF_CHNL(_adapter, _bss_idx) \
	((IS_BSS_INDEX_AIS(_adapter, _bss_idx)) && \
	((aisGetAisFsmInfo(_adapter, _bss_idx)->eCurrentState == \
	AIS_STATE_REQ_REMAIN_ON_CHANNEL) || \
	(aisGetAisFsmInfo(_adapter, _bss_idx)->eCurrentState == \
	AIS_STATE_REMAIN_ON_CHANNEL) || \
	(aisGetAisFsmInfo(_adapter, _bss_idx)->eCurrentState == \
	AIS_STATE_OFF_CHNL_TX)))

#if (CFG_EXT_ROAMING == 1)
#define RCPI_FOR_DONT_ROAM		    54 /*-83dbm*/
#else
#define RCPI_FOR_DONT_ROAM		    60 /*-80dbm*/
#endif

#define RCPI_FOR_DONT_BTO                   68 /*-76dbm*/

#define AIS_BTM_DIS_IMMI_THRESHOLD	    60000 /* MSEC */
#define AIS_BTM_DIS_IMMI_STATE_0	    0
#define AIS_BTM_DIS_IMMI_STATE_1	    1
#define AIS_BTM_DIS_IMMI_STATE_2	    2
#define AIS_BTM_DIS_IMMI_STATE_3	    3

#define AIS_FT_R0		0
#define AIS_FT_R1		1

#define AIS_MAX_QUERIED_BSSID_NUM	    5

#define AIS_DEFAULT_AGING_PERIOD	30 /* second */
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
enum ENUM_AIS_STATE {
	AIS_STATE_IDLE = 0,
	AIS_STATE_SEARCH,
	AIS_STATE_SCAN,
	AIS_STATE_ONLINE_SCAN,
	AIS_STATE_LOOKING_FOR,
	AIS_STATE_WAIT_FOR_NEXT_SCAN,
	AIS_STATE_REQ_CHANNEL_JOIN,
	AIS_STATE_JOIN,
	AIS_STATE_JOIN_FAILURE,
	AIS_STATE_NORMAL_TR,
	AIS_STATE_DISCONNECTING,
	AIS_STATE_REQ_REMAIN_ON_CHANNEL,
	AIS_STATE_REMAIN_ON_CHANNEL,
	AIS_STATE_OFF_CHNL_TX,
	AIS_STATE_ROAMING,
	AIS_STATE_LINK_RECONFIG,
	AIS_STATE_NUM
};

/* disconnect reason */
enum ENUM_DISCONNECT_REASON_CODE {
	DISCONNECT_REASON_CODE_RESERVED,
	DISCONNECT_REASON_CODE_RADIO_LOST,
	DISCONNECT_REASON_CODE_DEAUTHENTICATED,
	DISCONNECT_REASON_CODE_DISASSOCIATED,
	DISCONNECT_REASON_CODE_NEW_CONNECTION,
	DISCONNECT_REASON_CODE_REASSOCIATION,
	DISCONNECT_REASON_CODE_ROAMING,
	DISCONNECT_REASON_CODE_CHIPRESET,
	DISCONNECT_REASON_CODE_LOCALLY,
	DISCONNECT_REASON_CODE_RECONFIG_IFACE,
	DISCONNECT_REASON_CODE_TEST_MODE,
	DISCONNECT_REASON_CODE_NUM,
};

static const char * const apucDiscReasonStr[DISCONNECT_REASON_CODE_NUM + 1] = {
	"RESERVED",
	"RADIO_LOST",
	"DEAUTHENTICATED",
	"DISASSOCIATED",
	"NEW_CONNECTION",
	"REASSOCIATION",
	"ROAMING",
	"CHIPRESET",
	"LOCALLY",
	"DEL_IFACE",
	"TEST_MODE",
	"NULL"
};

struct MSG_AIS_ABORT {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucReasonOfDisconnect;
	uint8_t fgDelayIndication;
	uint8_t ucBssIndex;
	/* Reason that been Deauth/Disassoc, valid when ucReasonOfDisconnect is
	 * DISCONNECT_REASON_CODE_DEAUTHENTICATED or
	 * DISCONNECT_REASON_CODE_DISASSOCIATED
	 */
	uint16_t u2DeauthReason;
};

enum ENUM_AIS_REQUEST_TYPE {
	AIS_REQUEST_SCAN,
	AIS_REQUEST_RECONNECT,
	AIS_REQUEST_ROAMING_SEARCH,
	AIS_REQUEST_ROAMING_CONNECT,
	AIS_REQUEST_REMAIN_ON_CHANNEL,
	AIS_REQUEST_BTO,
	AIS_REQUEST_CSA,
	AIS_REQUEST_LOOKING_FOR,
	AIS_REQUEST_LINK_RECONFIG,
	AIS_REQUEST_NUM
};

struct AIS_REQ_HDR {
	struct LINK_ENTRY rLinkEntry;
	enum ENUM_AIS_REQUEST_TYPE eReqType;
};

struct AIS_SCAN_REQ {
	struct AIS_REQ_HDR rReqHdr;
	struct PARAM_SCAN_REQUEST_ADV rScanRequest;
};

struct AIS_CSA_REQ {
	struct AIS_REQ_HDR rReqHdr;
	u_int8_t ucBssIndex;
};

struct AIS_REQ_CHNL_INFO {
	enum ENUM_BAND eBand;
	enum ENUM_CHNL_EXT eSco;
	uint8_t ucChannelNum;
	uint32_t u4DurationMs;
	uint64_t u8Cookie;
	enum ENUM_CH_REQ_TYPE eReqType;
};

struct AIS_MGMT_TX_REQ_INFO {
	struct LINK rTxReqLink;
	u_int8_t fgIsMgmtTxRequested;
	struct MSDU_INFO *prMgmtTxMsdu;
	uint64_t u8Cookie;
};

struct ESS_CHNL_INFO {
	uint8_t ucChannel;
	uint8_t ucUtilization;
	uint8_t ucApNum;
	enum ENUM_BAND eBand;
};

struct AIS_SPECIFIC_BSS_INFO {
	/* This value indicate the roaming type used in AIS_JOIN */
	uint8_t ucRoamingAuthTypes;

	/* current ap field for reassociation */
	uint8_t aucCurrentApAddr[MAC_ADDR_LEN];

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
	uint8_t aucSaQueryBSSID[MAC_ADDR_LEN];
	uint8_t *pucSaQueryTransId;
	struct TIMER rSaQueryTimer;
	u_int8_t fgBipKeyInstalled;
	struct BSS_DESC *aprTargetComebackBssDesc[MLD_LINK_MAX];
	uint8_t aucIPN[6];
	uint8_t aucIGTK[32];
	u_int8_t fgBipGmacKeyInstalled;
#endif
	uint8_t ucKeyAlgorithmId;

#if (CFG_SUPPORT_RSNO == 1)
	enum ENUM_RSN_OVERRIDE eRsnSelectedRSNOverride;
#endif /* CFG_SUPPORT_RSNO */

	/* Support AP Selection */
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
	uint8_t ucRoamSkipTimes;
	u_int8_t fgGoodRcpiArea;
	u_int8_t fgPoorRcpiArea;
#endif
	struct ESS_CHNL_INFO arCurEssChnlInfo[CFG_MAX_NUM_OF_CHNL_INFO];
	uint8_t ucCurEssChnlInfoNum;
	uint8_t aucCurEssChnlBitMap[64];

	struct LINK rCurEssLink;
	struct AP_COLLECTION *arApHash[AP_HASH_SIZE];

	/* end Support AP Selection */

	struct BSS_TRANSITION_MGT_PARAM rBTMParam;
	struct LINK_MGMT rNeighborApList;
	OS_SYSTIME rNeiApRcvTime;
	uint32_t u4NeiApValidInterval;
	uint8_t aucQueriedBssid[AIS_MAX_QUERIED_BSSID_NUM][MAC_ADDR_LEN];
	uint8_t ucQueriedBssidIdx;

#if CFG_SUPPORT_ASSURANCE
	uint8_t fgRoamingReasonEnable;
	uint8_t fgBcnReptErrReasonEnable;
#endif
};

/* Session for CONNECTION SETTINGS */
struct CONNECTION_SETTINGS {

	uint8_t aucMacAddress[MAC_ADDR_LEN];

	uint8_t ucDelayTimeOfDisconnectEvent;

	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint8_t aucBSSIDHint[MAC_ADDR_LEN];

	uint8_t ucSSIDLen;
	uint8_t aucSSID[ELEM_MAX_LEN_SSID];

	enum ENUM_PARAM_OP_MODE eOPMode;

	enum ENUM_PARAM_CONNECTION_POLICY eConnectionPolicy;

	enum ENUM_PARAM_AD_HOC_MODE eAdHocMode;

	enum ENUM_PARAM_AUTH_MODE eAuthMode;

	enum ENUM_WEP_STATUS eEncStatus;

	/* MIB attributes */
	uint16_t u2BeaconPeriod;

	uint16_t u2RTSThreshold;	/* User desired setting */

	uint16_t u2DesiredNonHTRateSet;	/* User desired setting */

	uint8_t ucAdHocChannelNum;	/* For AdHoc */

	enum ENUM_BAND eAdHocBand;	/* For AdHoc */

	uint32_t u4FreqInMHz;	/* Center frequency */

	/* ATIM windows using for IBSS power saving function */
	uint16_t u2AtimWindow;

	enum ENUM_PARAM_PHY_CONFIG eDesiredPhyConfig;

#if CFG_SUPPORT_802_11D
	u_int8_t fgMultiDomainCapabilityEnabled;
#endif				/* CFG_SUPPORT_802_11D */

#if 1				/* CFG_SUPPORT_WAPI */
	u_int8_t fgWapiMode;
	uint32_t u4WapiSelectedGroupCipher;
	uint32_t u4WapiSelectedPairwiseCipher;
	uint32_t u4WapiSelectedAKMSuite;
#endif

	/* for cfg80211 connected indication */
	uint32_t u4RspIeLength;
	uint8_t *aucRspIe;

	uint32_t u4ReqIeLength;
	uint8_t *aucReqIe;

	u_int8_t fgWpsActive;
	uint8_t aucWSCIE[GLUE_INFO_WSCIE_LENGTH];	/*for probe req */
	uint16_t u2WSCIELen;

	uint8_t aucJoinBSSID[MAC_ADDR_LEN];
	uint16_t u2JoinStatus;
	uint32_t u4ConnFlags;

	/*
	 * Buffer to hold non-wfa vendor specific IEs set
	 * from wpa_supplicant. This is used in sending
	 * Association Request in AIS mode.
	 */
	uint16_t non_wfa_vendor_ie_len;
	uint8_t non_wfa_vendor_ie_buf[NON_WFA_VENDOR_IE_MAX_LEN];

	/* 11R */
	struct FT_IES rFtIeR0;
	struct FT_IES rFtIeR1;
	struct FT_EVENT_PARAMS rFtEventParam;

	/* CR1486, CR1640 */
	/* for WPS, disable the privacy check for AP selection policy */
	u_int8_t fgPrivacyCheckDisable;

	/* b0~3: trigger-en AC0~3. b4~7: delivery-en AC0~3 */
	uint8_t bmfgApsdEnAc;

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	struct EAP_ERP_KEY rErpKey;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
	u_int8_t fgSecModeChangeStartTimer;
#endif

	uint8_t *pucAssocIEs;
	size_t assocIeLen;
	u_int8_t fgAuthOsenWithRSN;
	uint16_t u2LinkIdBitmap;
	u_int8_t fgDisableBTM;
};

struct AIS_LINK_INFO {
	struct BSS_INFO *prBssInfo;
	struct BSS_DESC *prTargetBssDesc;
	struct STA_RECORD *prTargetStaRec;
};

/* Support AP Selection */
struct AIS_BLOCKLIST_ITEM {
	struct LINK_ENTRY rLinkEntry;

	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint16_t u2DeauthReason;
	uint16_t u2AuthStatus;
	uint8_t ucCount;
	uint8_t ucDeauthCount;
	uint8_t ucSSIDLen;
	uint8_t aucSSID[32];
	OS_SYSTIME rAddTime;
	u_int8_t fgDeauthLastTime;
	u_int8_t fgIsInFWKBlocklist;
#if CFG_SUPPORT_MBO
	uint8_t fgDisallowed;
	uint16_t u2DisallowSec;
	int32_t i4RssiThreshold;
#endif
};

#if (CFG_SUPPORT_802_11BE_MLO == 1)
struct MLD_BLOCKLIST_ITEM {
	struct LINK_ENTRY rLinkEntry;
	uint8_t aucMldAddr[MAC_ADDR_LEN];
	uint8_t aucCount[MLO_LINK_PLAN_NUM];
	uint32_t u4BlockBmap; /* bmap for blocked link plan */
	OS_SYSTIME rAddTime;
};

#if (CFG_SUPPORT_ML_RECONFIG == 1)
enum ENUM_MLRC_ACTION {
	MLRC_ACTION_EMPTY,		/* no bssdesc */
	MLRC_ACTION_NONE,		/* has bssdesc but none to do */
	MLRC_ACTION_ADD_DEPAUSE,	/* add by depause */
	MLRC_ACTION_DELETE_PAUSE,	/* delete by pause */
	MLRC_ACTION_NUM,
};

extern const uint8_t *apucMlrcActionStr[MLRC_ACTION_NUM + 1];

enum ENUM_MLRC_STATE {
	MLRC_STATE_IDLE,
	MLRC_STATE_NEGO,
	MLRC_STATE_NEGO_FAIL,
	MLRC_STATE_RECONFIG,
	MLRC_STATE_NUM,
};

extern const uint8_t *apucMlrcStateStr[MLRC_STATE_NUM + 1];

enum ENUM_MLRC_MODE {
	MLRC_MODE_DEL_AND_ADD,
	MLRC_MODE_ADD_ONLY,
	MLRC_MODE_DEL_ONLY,
	MLRC_MODE_ADD_THEN_DEL,
	MLRC_MODE_DEL_THEN_ADD,
	MLRC_MODE_NUM,
};

#define MLRC_OP_PENDING_DELAY		5000

struct MLRC_INFO {
	struct TIMER rApRemovalTimer;
	struct TIMER rPendingMlrcOpTimer;
	enum ENUM_MLRC_STATE eMlrcState;
	enum ENUM_MLRC_ACTION aeMlrcAction[MLD_LINK_MAX];
	struct BSS_DESC *aprBssDesc[MLD_LINK_MAX];
	struct SW_RFB *prLRResponseSwRfb;
	uint8_t ucDialogToken;
	uint8_t fgHasPendingAction;
};

#endif /* CFG_SUPPORT_ML_RECONFIG */
#endif /* CFG_SUPPORT_802_11BE_MLO */

struct AX_BLOCKLIST_ITEM {
	struct LINK_ENTRY rLinkEntry;
	uint8_t aucBSSID[MAC_ADDR_LEN];
};

struct CUS_BLOCKLIST_ITEM {
	struct LINK_ENTRY rLinkEntry;

	uint8_t ucType;

	struct PARAM_SSID rSSID;
	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint32_t u4Frequency;
	enum ENUM_BAND eBand;

	uint8_t ucLimitReason;
	uint8_t ucLimitType;
	uint32_t u4LimitTimeout;
	OS_SYSTIME rAddTime;
};

struct AIS_BTO_INFO {
	struct BSS_DESC *prBtoBssDesc;
	uint8_t ucBcnTimeoutReason;
	uint8_t ucDisconnectReason;
	uint8_t fgTryRecover;
};

struct AIS_FSM_INFO {
	enum ENUM_AIS_STATE ePreviousState;
	enum ENUM_AIS_STATE eCurrentState;

	u_int8_t ucAisIndex;

	u_int8_t fgIsScanning;
	OS_SYSTIME rScanDoneTime;

	u_int8_t fgIsChannelRequested;
	u_int8_t fgIsChannelGranted;

	u_int8_t fgIsReconfigIface;
	u_int8_t fgIsDisassocPend;

	uint8_t ucChReqNum;
	enum ENUM_MBMC_BN eChReqDbdcBand;

	uint8_t ucAvailableAuthTypes; /* Used for AUTH_MODE_AUTO_SWITCH */

	uint8_t ucLinkNum;
	uint8_t arBssId2LinkMap[MAX_BSSID_NUM + 1];
	uint32_t u4BssIdxBmap;
	struct AIS_LINK_INFO aprLinkInfo[MLD_LINK_MAX];

	struct CONNECTION_SETTINGS rConnSettings;

#if CFG_SUPPORT_ROAMING
	struct ROAMING_INFO rRoamingInfo;
#endif	/* CFG_SUPPORT_ROAMING */

	struct APS_INFO rApsInfo;

	struct AIS_SPECIFIC_BSS_INFO rAisSpecificBssInfo;

	uint32_t u4SleepInterval;

	struct TIMER rBGScanTimer;

	uint32_t u4PostponeIndStartTime;

	struct TIMER rJoinTimeoutTimer;

	struct TIMER rChannelTimeoutTimer;

	struct TIMER rScanDoneTimer;

	struct TIMER rDeauthDoneTimer;

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
	struct TIMER rSecModeChangeTimer;
#endif

	uint8_t ucSeqNumOfReqMsg;
	uint8_t ucSeqNumOfChReq;
	uint8_t ucBssIndexOfChReq;
	uint8_t ucSeqNumOfScanReq;

	uint32_t u4ChGrantedInterval;

	uint8_t ucScanTrialCount;
	uint8_t ucConnTrialCount;

	struct PARAM_SCAN_REQUEST_ADV rScanRequest;
	struct BSS_DESC_SET rSearchResult;

	u_int8_t fgIsScanOidAborted;

	/* Pending Request List */
	struct LINK rPendingReqList;

	/* Join Request Timestamp */
	OS_SYSTIME rJoinReqTime;

	/* for cfg80211 REMAIN_ON_CHANNEL support */
	struct AIS_REQ_CHNL_INFO rChReqInfo;

	/* Mgmt tx related. */
	struct AIS_MGMT_TX_REQ_INFO rMgmtTxInfo;

	/* Packet filter for AIS module. */
	uint32_t u4AisPacketFilter;

	/* 11K */
	struct RADIO_MEASUREMENT_REQ_PARAMS rRmReqParams;
	struct RADIO_MEASUREMENT_REPORT_PARAMS rRmRepParams;

	/* WMMAC */
	struct WMM_INFO rWmmInfo;

#if CFG_SUPPORT_PASSPOINT
	struct HS20_INFO rHS20Info;
#endif	/* CFG_SUPPORT_PASSPOINT */

#if CFG_SUPPORT_802_11W
	/* STA PMF: for encrypted deauth frame */
	kal_completion rDeauthComp;
	u_int8_t encryptedDeauthIsInProcess;
#endif

	/* MTK WLAN NIC driver IEEE 802.11 MIB */
	struct IEEE_802_11_MIB rMib;

	struct PARAM_BSSID_EX rCurrBssId;

	/*! \brief wext wpa related information */
	struct GL_WPA_INFO rWpaInfo;

	/* Indicated media state */
	enum ENUM_PARAM_MEDIA_STATE eParamMediaStateIndicated;

	uint8_t ucReasonOfDisconnect;	/* Used by media state indication */

	/* Scan target channel when device roaming */
	uint8_t fgTargetChnlScanIssued;
#if CFG_SUPPORT_ROAMING
	uint8_t ucIsStaRoaming;
#endif
#if CFG_SUPPORT_LLW_SCAN
	/* scan parameters */
	uint16_t u2OpChStayTimeMs;
	uint8_t ucNonDfsChDwellTimeMs;
	uint8_t ucDfsChDwellTimeMs;
	uint8_t ucPerScanChannelCnt;
	uint8_t ucLatencyMode;
#endif
	struct LINK rAxBlocklist;
	struct LINK rHeHtcBlocklist;

	struct LINK rCusBlocklist;

	/* rssi monitor */
	struct PARAM_RSSI_MONITOR_T rRSSIMonitor;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo;
	uint8_t ucMlProbeSendCount;
	uint8_t ucMlProbeEnable;
	struct BSS_DESC *prMlProbeBssDesc;
#if (CFG_SUPPORT_ML_RECONFIG == 1)
	struct MLRC_INFO rMlrcInfo;
#endif /* CFG_SUPPORT_ML_RECONFIG */
#endif

	struct AIS_BTO_INFO rBtoInfo;

#if (CFG_EXT_FEATURE == 1)
	struct AIS_EXT_INFO rAisExtInfo;
#endif
	uint8_t ucIsSapCsaPending;
};

struct AIS_OFF_CHNL_TX_REQ_INFO {
	struct LINK_ENTRY rLinkEntry;
	struct MSDU_INFO *prMgmtTxMsdu;
	u_int8_t fgNoneCckRate;
	struct RF_CHANNEL_INFO rChannelInfo;	/* Off channel TX. */
	enum ENUM_CHNL_EXT eChnlExt;
	/* See if driver should keep at the same channel. */
	u_int8_t fgIsWaitRsp;
	uint64_t u8Cookie; /* cookie used to match with supplicant */
	uint32_t u4Duration; /* wait time for tx request */
};

enum WNM_AIS_BSS_TRANSITION {
	BSS_TRANSITION_NO_MORE_ACTION,
	BSS_TRANSITION_REQ_ROAMING,
	BSS_TRANSITION_DISASSOC,
	BSS_TRANSITION_MAX_NUM
};
struct MSG_AIS_BSS_TRANSITION {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucBssIndex;
};

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
#define aisChangeMediaState(_prAisBssInfo, _eNewMediaState) \
	(_prAisBssInfo->eConnectionState = (_eNewMediaState))

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void aisInitializeConnectionSettings(struct ADAPTER
		*prAdapter, struct REG_INFO *prRegInfo,
		uint8_t ucBssIndex);

void aisInitializeConnectionRsnInfo(struct ADAPTER
		*prAdapter, uint8_t ucBssIndex);

void aisFsmInit(struct ADAPTER *prAdapter,
		struct REG_INFO *prRegInfo,
		uint8_t ucAisIndex);

void aisFsmUninit(struct ADAPTER *prAdapter, uint8_t ucAisIndex);

bool aisFsmIsInProcessPostpone(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

bool aisFsmIsInBeaconTimeout(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

uint32_t aisFsmStateInit_JOIN(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo, struct STA_RECORD **prMainStaRec,
	uint8_t ucLinkIndex);

u_int8_t aisFsmStateInit_RetryJOIN(struct ADAPTER
				   *prAdapter, struct STA_RECORD *prStaRec,
				   uint8_t ucBssIndex);

void aisFsmStateAbort(struct ADAPTER *prAdapter,
		      uint8_t ucReasonOfDisconnect, u_int8_t fgDelayIndication,
		      uint8_t ucBssIndex);

void aisFsmStateAbort_JOIN(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

void aisFsmStateAbort_SCAN_All(struct ADAPTER *prAdapter);

void aisFsmStateAbort_SCAN(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

void aisFsmStateAbort_NORMAL_TR(struct ADAPTER
				*prAdapter, uint8_t ucBssIndex);

void aisFsmSteps(struct ADAPTER *prAdapter,
		 enum ENUM_AIS_STATE eNextState, uint8_t ucBssIndex);

void aisFsmGetCurrentEssChnlList(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void aisFsmIndicateToResetFT(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/* Mailbox Message Handling                                                   */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventScanDone(struct ADAPTER *prAdapter,
			    struct MSG_HDR *prMsgHdr);

enum ENUM_AIS_STATE aisFsmScanResultsUpdate(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex);

void aisFsmRunEventAbort(struct ADAPTER *prAdapter,
			 struct MSG_HDR *prMsgHdr);

void aisFsmRunEventJoinComplete(struct ADAPTER
				*prAdapter, struct MSG_HDR *prMsgHdr);

enum ENUM_AIS_STATE aisFsmJoinCompleteAction(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr);

void aisFsmAuthorizedAction(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

void aisFsmRunEventRemainOnChannel(struct ADAPTER
				   *prAdapter, struct MSG_HDR *prMsgHdr);

void aisFsmRunEventCancelRemainOnChannel(struct ADAPTER
		*prAdapter, struct MSG_HDR *prMsgHdr);

/*----------------------------------------------------------------------------*/
/* Handling of Incoming Mailbox Message from CNM                              */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventChGrant(struct ADAPTER *prAdapter,
			   struct MSG_HDR *prMsgHdr);

/*----------------------------------------------------------------------------*/
/* Generating Outgoing Mailbox Message to CNM                                 */
/*----------------------------------------------------------------------------*/
void aisFsmReleaseCh(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/* Event Indication                                                           */
/*----------------------------------------------------------------------------*/
void
aisIndicationOfMediaStateToHost(struct ADAPTER
				*prAdapter,
				enum ENUM_PARAM_MEDIA_STATE eConnectionState,
				u_int8_t fgDelayIndication,
				uint8_t ucBssIndex);

void aisPostponedEventOfDisconnTimeout(struct ADAPTER *prAdapter,
				uint8_t ucBssIndex);

void aisUpdateAllBssInfoForJOIN(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	struct SW_RFB *prAssocRspSwRfb,
	struct STA_RECORD *prSetupStaRec);

void aisUpdateBssInfoForJOIN(struct ADAPTER *prAdapter,
			     struct STA_RECORD *prStaRec,
			     struct SW_RFB *prAssocRspSwRfb);

enum ENUM_AIS_STATE aisSearchHandleBssDesc(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prBssDescSet, uint8_t ucBssIndex);

uint32_t
aisFsmRunEventMgmtFrameTxDone(struct ADAPTER *prAdapter,
			      struct MSDU_INFO *prMsduInfo,
			      enum ENUM_TX_RESULT_CODE rTxDoneStatus);

/*----------------------------------------------------------------------------*/
/* Disconnection Handling                                                     */
/*----------------------------------------------------------------------------*/
void aisFsmDisconnect(struct ADAPTER *prAdapter,
		      u_int8_t fgDelayIndication,
		      uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/* Event Handling                                                             */
/*----------------------------------------------------------------------------*/
void aisBssBeaconTimeout(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void aisBssBeaconTimeout_impl(struct ADAPTER *prAdapter,
	uint8_t ucBcnTimeoutReason, uint8_t fgTryRecover, uint8_t ucBssIndex);

uint8_t aisBeaconTimeoutFilterPolicy(struct ADAPTER *prAdapter,
	uint8_t ucLinkDtThreshold, uint8_t ucBssIndex);

void aisBssLinkDown(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void aisHandleBeaconTimeout(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
void aisBssSecurityChanged(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
#endif

uint32_t
aisDeauthXmitComplete(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo,
		      enum ENUM_TX_RESULT_CODE rTxDoneStatus);
uint32_t
aisDeauthXmitCompleteBss(struct ADAPTER *prAdapter,
		      uint8_t ucBssIndex,
		      enum ENUM_TX_RESULT_CODE rTxDoneStatus);

#if CFG_SUPPORT_ROAMING
void aisFsmRunEventRoamingDiscovery(
	struct ADAPTER *prAdapter,
	uint32_t u4ReqScan,
	uint8_t ucBssIndex);

void aisFsmRunEventRoamingRoam(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

void aisFsmRoamingDisconnectPrevAP(struct ADAPTER *prAdapter,
				   struct BSS_INFO *prAisBssInfo,
				   struct STA_RECORD *prTargetStaRec);

void aisUpdateBssInfoForRoamingAP(struct ADAPTER
				  *prAdapter, struct STA_RECORD *prStaRec,
				  struct SW_RFB *prAssocRspSwRfb);
#endif /*CFG_SUPPORT_ROAMING */

/*----------------------------------------------------------------------------*/
/* Timeout Handling                                                           */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventBGSleepTimeOut(struct ADAPTER
				  *prAdapter, uintptr_t ulParamPtr);

void aisFsmRunEventJoinTimeout(struct ADAPTER *prAdapter,
			       uintptr_t ulParamPtr);

void aisFsmRunEventChannelTimeout(struct ADAPTER
				  *prAdapter, uintptr_t ulParamPtr);

void aisFsmRunEventDeauthTimeout(struct ADAPTER
				 *prAdapter, uintptr_t ulParamPtr);

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
void aisFsmRunEventSecModeChangeTimeout(struct ADAPTER
					*prAdapter, uintptr_t ulParamPtr);
#endif

/*----------------------------------------------------------------------------*/
/* OID/IOCTL Handling                                                         */
/*----------------------------------------------------------------------------*/

void
aisFsmScanRequestAdv(struct ADAPTER *prAdapter,
		struct PARAM_SCAN_REQUEST_ADV *prRequestIn);

/*----------------------------------------------------------------------------*/
/* Internal State Checking                                                    */
/*----------------------------------------------------------------------------*/
u_int8_t aisFsmIsRequestPending(struct ADAPTER *prAdapter,
				enum ENUM_AIS_REQUEST_TYPE eReqType,
				u_int8_t bRemove, struct AIS_REQ_HDR **prReqHdr,
				uint8_t ucBssIndex);

void aisFsmRemoveRoamingRequest(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex);

struct AIS_REQ_HDR *aisFsmGetNextRequest(struct ADAPTER *prAdapter,
				uint8_t ucBssIndex);

u_int8_t aisFsmInsertRequest(struct ADAPTER *prAdapter,
			     enum ENUM_AIS_REQUEST_TYPE eReqType,
			     uint8_t ucBssIndex);

u_int8_t aisFsmInsertRequestImpl(struct ADAPTER *prAdapter,
			     struct AIS_REQ_HDR *prAisReq,
			     uint8_t fgToHead,
			     uint8_t ucBssIndex);

u_int8_t aisFsmInsertRequestToHead(struct ADAPTER *prAdapter,
			     enum ENUM_AIS_REQUEST_TYPE eReqType,
			     uint8_t ucBssIndex);

u_int8_t aisFsmClearRequest(struct ADAPTER *prAdapter,
			     enum ENUM_AIS_REQUEST_TYPE eReqType,
			     uint8_t ucBssIndex);
void aisFsmClearRequestForRrm(struct ADAPTER *prAdapter,
			     uint8_t ucBssIndex);

void aisFsmFlushRequest(struct ADAPTER *prAdapter,
				uint8_t ucBssIndex);

uint32_t
aisFuncTxMgmtFrame(struct ADAPTER *prAdapter,
		   struct AIS_MGMT_TX_REQ_INFO *prMgmtTxReqInfo,
		   struct MSDU_INFO *prMgmtTxMsdu, uint64_t u8Cookie,
		   uint8_t ucBssIndex);

void aisFsmRunEventMgmtFrameTx(struct ADAPTER *prAdapter,
				struct MSG_HDR *prMsgHdr);

void aisFuncValidateRxActionFrame(struct ADAPTER *prAdapter,
				struct SW_RFB *prSwRfb);

void aisFsmRunEventBssTransition(struct ADAPTER *prAdapter,
				struct MSG_HDR *prMsgHdr);

void aisFsmRunEventCancelTxWait(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

enum ENUM_AIS_STATE aisFsmStateSearchAction(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex,
	struct BSS_DESC_SET *prBssDescSet);
#if defined(CFG_TEST_MGMT_FSM) && (CFG_TEST_MGMT_FSM != 0)
void aisTest(void);
#endif /* CFG_TEST_MGMT_FSM */

/* Support AP Selection */
void aisRefreshFWKBlocklist(struct ADAPTER *prAdapter);
struct AIS_BLOCKLIST_ITEM *aisAddBlocklist(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc);
void aisRemoveBlockList(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc);
void aisRemoveTimeoutBlocklist(struct ADAPTER *prAdapter, uint16_t u2Sec);
void aisRemoveDeauthBlocklist(struct ADAPTER *prAdapter,
	u_int8_t fgRemoveAll, uint16_t u2Sec);
struct AIS_BLOCKLIST_ITEM *aisQueryBlockList(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc);
void aisBssTmpDisallow(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc,
	uint32_t sec, int32_t rssiThreshold);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
struct MLD_BLOCKLIST_ITEM *aisAddMldBlocklist(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prBssDescSet);
void aisRemoveMldBlockList(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc);
void aisRemoveTimeoutMldBlocklist(struct ADAPTER *prAdapter, uint16_t u2Sec);
struct MLD_BLOCKLIST_ITEM *aisQueryMldBlockList(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc);
uint16_t aisGetNeighborMldAPPrefLinks(
	struct ADAPTER *prAdapter, struct BSS_DESC *bss, uint8_t ucBssIndex);
#endif

/* Support 11K */
#if CFG_SUPPORT_802_11K
struct NEIGHBOR_AP *aisGetNeighborAPEntry(
	struct ADAPTER *prAdapter, struct BSS_DESC *bss, uint8_t ucBssIndex);
uint32_t aisCollectNeighborAP(struct ADAPTER *prAdapter, uint8_t *pucApBuf,
			  uint16_t u2ApBufLen, uint8_t ucValidInterval,
			  uint8_t ucBssIndex);
void aisResetNeighborApList(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
uint8_t aisCheckNeighborApValidity(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
#endif
/* end Support 11K */

void aisCheckPmkidCache(struct ADAPTER *prAdapter,
			struct BSS_DESC *prBss,
			uint8_t ucAisIndex);

struct PMKID_ENTRY *aisSearchPmkidEntry(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec, uint8_t ucBssIndex);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
void aisCheckApRemoval(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, uint16_t u2ApRemovalTimer);
#endif /* CFG_SUPPORT_802_11BE_MLO */

/*----------------------------------------------------------------------------*/
/* CSA Handler                                                                */
/*----------------------------------------------------------------------------*/
void aisFunFlushTxQueue(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);
void aisFunSwitchChannel(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);
void aisFunSwitchChannelImpl(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
void aisFunSwitchChannelAbort(struct ADAPTER *ad,
	struct AIS_FSM_INFO *ais, uint8_t fgResetAll);
void aisReqJoinChPrivilegeForCSA(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	struct BSS_INFO *prBss,
	uint8_t *ucChTokenId);
u_int8_t aisFsmIsSwitchChannel(struct ADAPTER *ad,
	struct AIS_FSM_INFO *ais);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

struct AIS_FSM_INFO *aisGetAisFsmInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct PARAM_SCAN_REQUEST_ADV *aisGetScanReq(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct BSS_DESC_SET *aisGetSearchResult(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct AIS_SPECIFIC_BSS_INFO *aisGetAisSpecBssInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct BSS_TRANSITION_MGT_PARAM *aisGetBTMParam(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

#if (CFG_SUPPORT_ML_RECONFIG == 1)
void aisResetMlrcInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct MLRC_INFO *aisGetMlrcInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

enum ENUM_MLRC_STATE aisGetMlrcState(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void aisSetMlrcState(
	struct ADAPTER *prAdapter,
	enum ENUM_MLRC_STATE eMlrcState,
	uint8_t ucBssIndex);

void aisFsmMlReconfigUpdateState(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
#endif

struct BSS_INFO *aisGetConnectedBssInfo(
	struct ADAPTER *prAdapter);

struct BSS_INFO *aisGetAisBssInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct STA_RECORD *aisGetStaRecOfAP(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct BSS_DESC *aisGetTargetBssDesc(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct STA_RECORD *aisGetTargetStaRec(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

int8_t aisGetTargetRssi(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct AIS_FSM_INFO *aisFsmGetInstance(
	struct ADAPTER *prAdapter, uint8_t ucAisIndex);
struct AIS_FSM_INFO *aisGetDefaultAisInfo(struct ADAPTER *prAdapter);
struct AIS_LINK_INFO *aisGetDefaultLink(struct ADAPTER *prAdapter);
struct BSS_INFO *aisGetDefaultLinkBssInfo(struct ADAPTER *prAdapter);
uint8_t aisGetLinkIndex(struct ADAPTER *prAdapter, uint8_t ucBssIndex);
uint8_t aisGetDefaultLinkBssIndex(struct ADAPTER *prAdapter);
struct STA_RECORD *aisGetDefaultStaRecOfAP(struct ADAPTER *prAdapter);
struct AIS_LINK_INFO *aisGetLink(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
void aisSetLinkBssInfo(struct AIS_FSM_INFO *prAisFsmInfo,
	struct BSS_INFO *prBssInfo, uint8_t ucLinkIdx);
struct BSS_INFO *aisGetLinkBssInfo(struct AIS_FSM_INFO *prAisFsmInfo,
	uint8_t ucLinkIdx);
uint32_t aisGetBssIndexBmap(struct AIS_FSM_INFO *prAisFsmInfo);
struct BSS_INFO *aisGetMainLinkBssInfo(struct AIS_FSM_INFO *prAisFsmInfo);
uint8_t aisGetMainLinkBssIndex(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo);
void aisSetLinkBssDesc(struct AIS_FSM_INFO *prAisFsmInfo,
	struct BSS_DESC *prBssDesc, uint8_t ucLinkIdx);
struct BSS_DESC *aisGetLinkBssDesc(struct AIS_FSM_INFO *prAisFsmInfo,
	uint8_t ucLinkIdx);
uint8_t aisGetLinkNum(struct AIS_FSM_INFO *prAisFsmInfo);
struct BSS_DESC *aisGetMainLinkBssDesc(struct AIS_FSM_INFO *prAisFsmInfo);
struct BSS_DESC *aisGetHighBandLinkBssDesc(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo);
void aisSetLinkStaRec(struct AIS_FSM_INFO *prAisFsmInfo,
	 struct STA_RECORD *prStaRec, uint8_t ucLinkIdx);
struct STA_RECORD *aisGetLinkStaRec(struct AIS_FSM_INFO *prAisFsmInfo,
	uint8_t ucLinkIdx);
struct STA_RECORD *aisGetMainLinkStaRec(struct AIS_FSM_INFO *prAisFsmInfo);
void aisClearAllLink(struct AIS_FSM_INFO *prAisFsmInfo);
void aisDeactivateAllLink(struct ADAPTER *prAdapter,
			struct AIS_FSM_INFO *prAisFsmInfo);

void aisConfigPowerSaveProfileAllLink(struct ADAPTER *prAdapter,
	enum PARAM_POWER_MODE ePwrMode, enum POWER_SAVE_CALLER ucCaller,
	uint8_t ucBssIndex);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint8_t aisSecondLinkAvailable(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

struct MLD_BSS_INFO *aisGetMldBssInfo(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex);

struct MLD_STA_RECORD *aisGetMldStaRec(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex);
#endif

void aisTargetBssSetConnected(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *ais);

void aisTargetBssSetConnecting(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *ais);

void aisTargetBssResetConnected(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *ais);

void aisTargetBssResetConnecting(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *ais);

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
struct TIMER *aisGetSecModeChangeTimer(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
#endif

struct TIMER *aisGetScanDoneTimer(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

enum ENUM_AIS_STATE aisGetCurrState(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct CONNECTION_SETTINGS *
	aisGetConnSettings(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct GL_WPA_INFO *aisGetWpaInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

u_int8_t aisGetWapiMode(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

enum ENUM_PARAM_AUTH_MODE aisGetAuthMode(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

enum ENUM_PARAM_OP_MODE aisGetOPMode(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

enum ENUM_WEP_STATUS aisGetEncStatus(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct IEEE_802_11_MIB *aisGetMib(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

#if CFG_SUPPORT_ROAMING
struct ROAMING_INFO *aisGetRoamingInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct ROAMING_REPORT_INFO *aisGetRoamingReport(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
#endif

struct APS_INFO *aisGetApsInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct PARAM_BSSID_EX *aisGetCurrBssId(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

uint8_t *aisGetCurrentApAddr(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

#if CFG_SUPPORT_PASSPOINT
struct HS20_INFO *aisGetHS20Info(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
#endif

struct RADIO_MEASUREMENT_REQ_PARAMS *aisGetRmReqParam(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct RADIO_MEASUREMENT_REPORT_PARAMS *
	aisGetRmReportParam(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

struct WMM_INFO *
	aisGetWMMInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

#ifdef CFG_SUPPORT_REPLAY_DETECTION
struct GL_DETECT_REPLAY_INFO *
	aisGetDetRplyInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
#endif

const char *aisGetFsmState(enum ENUM_AIS_STATE);

const char *aisGetFsmReqType(enum ENUM_AIS_REQUEST_TYPE);

const char *aisGetDiscReason(enum ENUM_DISCONNECT_REASON_CODE);

struct FT_IES *
	aisGetFtIe(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucR0R1);

struct FT_EVENT_PARAMS *
	aisGetFtEventParam(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
struct EAP_ERP_KEY *aisGetErpKey(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

u_int8_t addAxBlocklist(struct ADAPTER *prAdapter,
	uint8_t aucBSSID[],
	uint8_t ucBssIndex,
	uint8_t ucType);

u_int8_t queryAxBlocklist(struct ADAPTER *prAdapter,
	uint8_t aucBSSID[],
	uint8_t ucBssIndex,
	uint8_t ucType);

u_int8_t clearAxBlocklist(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucType);

u_int8_t aisAddCusBlocklist(struct ADAPTER *prAdapter,
	struct PARAM_CUS_BLOCKLIST *prCusBlocklist,
	uint8_t ucBssIndex);

u_int8_t aisQueryCusBlocklist(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct BSS_DESC *prBssDesc);

u_int8_t aisClearCusBlocklist(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, u_int8_t fgRemoveAll);

void aisPreSuspendFlow(
	struct ADAPTER *prAdapter);

#if (CFG_SUPPORT_ANDROID_DUAL_STA == 1)
void aisMultiStaSetQuoteTime(
	struct ADAPTER *prAdapter,
	uint8_t fgSetQuoteTime);
#endif

uint8_t aisNeedTargetScan(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

u_int8_t aisUpdateInterfaceAddr(struct ADAPTER *prAdapter,
	struct AIS_FSM_INFO *prAisFsmInfo,
	uint8_t aucMacAddr[]);

void aisFsmStartJoinTimer(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint32_t u4TimeoutMs);

void aisFsmStopJoinTimer(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

#if (CFG_SUPPORT_ICS_STA == 1)
void aisFsmIcsLogControl(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
#endif /* CFG_SUPPORT_ICS_STA */

uint32_t aisSendChipConfigCmd(struct ADAPTER *prAdapter,
	uint8_t *aucCmd, uint8_t fgIsOid);

#endif /* _AIS_FSM_H */
