/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "scan.h"
 *    \brief
 *
 */


#ifndef _SCAN_H
#define _SCAN_H

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
/*! Maximum buffer size of SCAN list */
#define SCN_MAX_BUFFER_SIZE \
	(CFG_MAX_NUM_BSS_LIST * ALIGN_4(sizeof(struct BSS_DESC)))

/* Remove SCAN result except the connected one. */
#define SCN_RM_POLICY_EXCLUDE_CONNECTED		BIT(0)

/* Remove the timeout one */
#define SCN_RM_POLICY_TIMEOUT			BIT(1)

/* Remove the oldest one with hidden ssid */
#define SCN_RM_POLICY_OLDEST_HIDDEN		BIT(2)

/* If there are more than half BSS which has the same ssid as connection
 * setting, remove the weakest one from them Else remove the weakest one.
 */
#define SCN_RM_POLICY_SMART_WEAKEST		BIT(3)

/* Remove entire SCAN result */
#define SCN_RM_POLICY_ENTIRE			BIT(4)

/* Remove SCAN result except the specific one. */
#define SCN_RM_POLICY_EXCLUDE_SPECIFIC_SSID	BIT(5)

/* Remove SCAN result over missed counts. */
#define SCN_RM_POLICY_MISS_COUNT		BIT(6)

#define SCN_BSS_DESC_MISS_COUNT_LIMIT		2

#define SCN_BSS_DESC_STALE_SEC KAL_SCN_BSS_DESC_STALE_SEC

/* For WFD scan need about 15s. */
#define SCN_BSS_DESC_STALE_SEC_WFD		30

#define SCN_PROBE_DELAY_MSEC			0

#define SCN_ADHOC_BSS_DESC_TIMEOUT_SEC		5 /* Second. */
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_WFD
 /* Second. For WFD scan timeout. */
#define SCN_ADHOC_BSS_DESC_TIMEOUT_SEC_WFD	20
#endif
#endif

#define SCAN_DONE_DIFFERENCE				3
#define SCAN_DONE_VERSION_SUPPORT_CU			4

/* Full2Partial */
/* Define a full scan as scan channel number larger than this number */
#define SCAN_FULL2PARTIAL_CHANNEL_NUM           (25)
#if (CFG_SUPPORT_WIFI_6G == 1)
#define SCAN_CHANNEL_BITMAP_ARRAY_LEN           (8 + 8)
#else
#define SCAN_CHANNEL_BITMAP_ARRAY_LEN           (8)
#endif
#define BITS_OF_UINT                            (32)
#define BITS_OF_BYTE                            (8)

/* dwell time setting, should align FW setting */
#define SCAN_CHANNEL_DWELL_TIME_MIN_MSEC        (42)
#define SCAN_SPLIT_PACKETS_THRESHOLD		(30)

/* dwell time setting, reduce APP trigger scan dwell time to 20 */
#define SCAN_CHANNEL_MIN_DWELL_TIME_MSEC_APP	(20)
#define SCAN_CHANNEL_DWELL_TIME_MSEC_APP	(40)

/* dwell time setting for OCE certification */
#define SCAN_CHANNEL_DWELL_TIME_OCE         (42 + 8)
/* dwell time setting for VOE certification */
#define SCAN_CHANNEL_DWELL_TIME_VOE         (42 + 8)

#if (CFG_SUPPORT_WIFI_RNR == 1)
#define SCAN_TBTT_INFO_SET_OFFSET		(4)
#endif

#if CFG_SUPPORT_LLW_SCAN
#define SCAN_CUST_DWELL_GAP_FOR_MAX_AND_MIN (10)
#define SCAN_CUST_LLW_DWELLTIME_THRESHOLD (40)
#endif

/*----------------------------------------------------------------------------*/
/* MSG_SCN_SCAN_REQ                                                           */
/*----------------------------------------------------------------------------*/
#define SCAN_REQ_SSID_WILDCARD			BIT(0)
#define SCAN_REQ_SSID_P2P_WILDCARD		BIT(1)
#define SCAN_REQ_SSID_SPECIFIED						\
	BIT(2) /* two probe req will be sent, wildcard and specified */
#define SCAN_REQ_SSID_SPECIFIED_ONLY					\
	BIT(3) /* only a specified ssid probe request will be sent */

/*----------------------------------------------------------------------------*/
/* Support Multiple SSID SCAN                                                 */
/*----------------------------------------------------------------------------*/
#define SCN_SSID_MAX_NUM			CFG_SCAN_SSID_MAX_NUM
#define SCN_SSID_MATCH_MAX_NUM			CFG_SCAN_SSID_MATCH_MAX_NUM

#if CFG_SUPPORT_BATCH_SCAN
/*----------------------------------------------------------------------------*/
/* SCAN_BATCH_REQ                                                             */
/*----------------------------------------------------------------------------*/
#define SCAN_BATCH_REQ_START			BIT(0)
#define SCAN_BATCH_REQ_STOP			BIT(1)
#define SCAN_BATCH_REQ_RESULT			BIT(2)
#endif

#define SCN_CTRL_SCAN_CHANNEL_LISTEN_TIME_ENABLE	BIT(1)
#define SCN_CTRL_IGNORE_AIS_FIX_CHANNEL			BIT(1)
#define SCN_CTRL_ENABLE					BIT(0)

#define SCN_CTRL_DEFAULT_SCAN_CTRL		SCN_CTRL_IGNORE_AIS_FIX_CHANNEL

#define SCN_SCAN_DONE_PRINT_BUFFER_LENGTH	500

#define SCN_SCAN_OOB_PRINT_BUFFER_LENGTH	500

#define SCN_2G_BAND_CHNL_SLOT_NUM	14

#if (CFG_SUPPORT_UNII4 == 1)
#define SCN_5G_BAND_CHNL_SLOT_NUM	28
#else
#define SCN_5G_BAND_CHNL_SLOT_NUM	25
#endif

#define SCN_6G_BAND_CHNL_SLOT_NUM	59

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

enum ENUM_SCAN_TYPE {
	SCAN_TYPE_PASSIVE_SCAN = 0,
	SCAN_TYPE_ACTIVE_SCAN,
	SCAN_TYPE_NUM
};

enum ENUM_SCAN_STATE {
	SCAN_STATE_IDLE = 0,
	SCAN_STATE_SCANNING,
	SCAN_STATE_NUM
};

enum ENUM_FW_SCAN_STATE {
	FW_SCAN_STATE_IDLE = 0,			/* 0 */
	FW_SCAN_STATE_SCAN_START,		/* 1 */
	FW_SCAN_STATE_REQ_CHANNEL,		/* 2 */
	FW_SCAN_STATE_SET_CHANNEL,		/* 3 */
	FW_SCAN_STATE_DELAYED_ACTIVE_PROB_REQ,	/* 4 */
	FW_SCAN_STATE_ACTIVE_PROB_REQ,		/* 5 */
	FW_SCAN_STATE_LISTEN,			/* 6 */
	FW_SCAN_STATE_SCAN_DONE,		/* 7 */
	FW_SCAN_STATE_NLO_START,		/* 8 */
	FW_SCAN_STATE_NLO_HIT_CHECK,		/* 9 */
	FW_SCAN_STATE_NLO_STOP,			/* 10 */
	FW_SCAN_STATE_BATCH_START,		/* 11 */
	FW_SCAN_STATE_BATCH_CHECK,		/* 12 */
	FW_SCAN_STATE_BATCH_STOP,		/* 13 */
	FW_SCAN_STATE_NUM			/* 14 */
};

enum ENUM_SCAN_CHANNEL {
	SCAN_CHANNEL_FULL = 0,
	SCAN_CHANNEL_2G4 = 1,
	SCAN_CHANNEL_5G = 2,
	SCAN_CHANNEL_P2P_SOCIAL = 3,
	SCAN_CHANNEL_SPECIFIED = 4,
	SCAN_CHANNEL_5G_NO_DFS = 5,
	SCAN_CHANNEL_5G_DFS_ONLY = 6,
	SCAN_CHANNEL_FULL_NO_DFS = 7,
	SCAN_CHANNEL_6G = 8,
	SCAN_CHANNEL_NUM
};

static const char * const apucScanTypeStr[SCAN_TYPE_NUM] = {
	"PASSIVE",
	"ACTIVE",
};

static const char * const apucScanChnlStr[SCAN_CHANNEL_NUM] = {
	"FULL",
	"2G4",
	"5G",
	"P2P_SOCIAL",
	"SPECIFIED",
	"5G_NO_DFS",
	"5G_DFS_ONLY",
	"FULL_NO_DFS",
	"6G",
};

struct MSG_SCN_FSM {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint32_t u4Dummy;
};

enum ENUM_SCHED_SCAN_ACT {
	SCHED_SCAN_ACT_ENABLE = 0,
	SCHED_SCAN_ACT_DISABLE,
};

#define SCAN_LOG_PREFIX_MAX_LEN		(16)
#define SCAN_LOG_MSG_MAX_LEN		(500)
#define SCAN_LOG_BUFF_SIZE		(200)
#define SCAN_LOG_DYN_ALLOC_MEM		(0)

enum ENUM_SCAN_LOG_PREFIX {
	/* Scan */
	LOG_SCAN_REQ_K2D = 0,		/* 0 */
	LOG_SCAN_REQ_D2F,
	LOG_SCAN_RESULT_F2D,
	LOG_SCAN_RESULT_D2K,
	LOG_SCAN_DONE_F2D,
	LOG_SCAN_DONE_D2K,		/* 5 */

	/* Sched scan */
	LOG_SCHED_SCAN_REQ_START_K2D,
	LOG_SCHED_SCAN_REQ_START_D2F,
	LOG_SCHED_SCAN_REQ_STOP_K2D,
	LOG_SCHED_SCAN_REQ_STOP_D2F,
	LOG_SCHED_SCAN_DONE_F2D,	/* 10 */
	LOG_SCHED_SCAN_DONE_D2K,

	/* Scan abort */
	LOG_SCAN_ABORT_REQ_K2D,
	LOG_SCAN_ABORT_REQ_D2F,
	LOG_SCAN_ABORT_DONE_D2K,

	/* Driver only */
	LOG_SCAN_D2D,

	/* Last one */
	LOG_SCAN_MAX
};

/* IEEE Std 802.11 2020 Table 9-283 */
enum ESP_TRAFFIC_AC {
	ESP_AC_BK = 0,
	ESP_AC_BE = 1,
	ESP_AC_VI = 2,
	ESP_AC_VO = 3,
	ESP_AC_NUM = 4,
};

#if (CFG_SUPPORT_802_11BE_MLO == 1)
struct ML_INFO {
	uint8_t fgValid;
	uint8_t aucMldAddr[MAC_ADDR_LEN];
	uint8_t ucLinkId;
	uint8_t ucMldId;
	uint16_t u2ValidLinks;
	uint8_t ucMaxSimuLinks;
	uint8_t fgMlrcOp;
	uint16_t u2EmlCap;
	uint16_t u2MldCap;
	uint16_t u2ExtMldCap;
	uint16_t u2DisabledLinks;
	uint16_t u2ApRemovalTimer;
	uint8_t fgMldType;
	struct MLD_BLOCKLIST_ITEM *prBlock;
	uint8_t fgIsEmlsrPermittedAP;
};
#endif

/*----------------------------------------------------------------------------*/
/* BSS Descriptors                                                            */
/*----------------------------------------------------------------------------*/
struct BSS_DESC {
	struct LINK_ENTRY rLinkEntry;
	/* Support AP Selection*/
	struct LINK_ENTRY rLinkEntryEss[KAL_AIS_NUM];

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	/* For MLO, the MldAddr is to save common MLD MAC address */
	struct ML_INFO rMlInfo;
#endif

	uint8_t fgIsInUse; /* Indicate if this entry is in use or not */

	/* flag used to check whether this rfb is generated by driver */
	uint8_t fgDriverGen;

	uint8_t aucBSSID[MAC_ADDR_LEN];

	/* For IBSS, the SrcAddr is different from BSSID */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];

	/* If we are going to connect to this BSS (JOIN or ROAMING to another
	 * BSS), don't remove this record from BSS List.
	 * Is a Bitmap, Bit0: BSS0, Bit1: Bss1
	 */
	u_int8_t fgIsConnecting;

	/* If we have connected to this BSS (NORMAL_TR), don't removed
	 * this record from BSS list.
	 * Is a Bitmap, Bit0: BSS0, Bit1: Bss1
	 */
	u_int8_t fgIsConnected;

	/* If we are in beacon timeout procedure, don't removed
	 * this record from BSS list to keep its channel. And we should not
	 * take the BSS as connection candidate for AP selection.
	 */
	u_int8_t fgIsInBTO;

	/* When this flag is TRUE, means the SSID of this
	 * BSS is not known yet.
	 */
	u_int8_t fgIsHiddenSSID;

	uint8_t ucSSIDLen;
	uint8_t aucSSID[ELEM_MAX_LEN_SSID];

	OS_SYSTIME rUpdateTime;

	enum ENUM_BSS_TYPE eBSSType;

	uint16_t u2CapInfo;

	uint16_t u2BeaconInterval;
	uint16_t u2ATIMWindow;

	uint16_t u2OperationalRateSet;
	uint16_t u2BSSBasicRateSet;
	u_int8_t fgIsUnknownBssBasicRate;

	u_int8_t fgIsERPPresent;
	u_int8_t fgIsHTPresent;
	u_int8_t fgIsVHTPresent;
#if (CFG_SUPPORT_802_11AX == 1)
	u_int8_t fgIsHEPresent;
	uint8_t ucHePhyCapInfo[HE_PHY_CAP_BYTE_NUM];
	uint8_t ucBssColorInfo;
#if (CFG_SUPPORT_WIFI_6G == 1)
	u_int8_t fgIsHE6GPresent;
	u_int8_t fgIsCoHostedBssPresent;
	u_int8_t He6gRegInfo;
#endif
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	u_int8_t fgIsEHTPresent;
	union {
		uint8_t ucEpcsPriAccess;
		struct {
		/* 0b'01 EPCS
		 * 0b'10, Unsolicited EPCS
		 */
			uint8_t fgEpcsCap: 1;
			uint8_t fgUEpcsCap: 1;
		};
	};
	uint8_t ucEhtPhyCapInfo[EHT_PHY_CAP_BYTE_NUM];
#endif

#if (CFG_SUPPORT_802_11V_MBSSID == 1)
	/* Max BSSID indicator. Range from 1 to 8.
	* 0 means MBSSID function is disabled
	*/
	u_int8_t ucMaxBSSIDIndicator;
	/* MBSSID index which DUT connected for this BSS.
	* 0 means DUT connect to transmitted BSSID
	*/
	u_int8_t ucMBSSIDIndex;
#endif

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	u_int8_t ucIsFilsSkSupport;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

	uint8_t ucPhyTypeSet;	/* Available PHY Type Set of this BSS */

	/* record from bcn or probe response */
	uint8_t ucVhtCapNumSoundingDimensions;

	uint8_t ucChannelNum;

	/* Record bandwidth for association process. Some AP will
	 * send association resp by 40MHz BW
	 */
	enum ENUM_CHNL_EXT eSco;

	enum ENUM_CHANNEL_WIDTH eChannelWidth;	/* VHT, HE operation ie */
	uint8_t ucCenterFreqS1;
	uint8_t ucCenterFreqS2;
	uint8_t ucCenterFreqS3;
	enum ENUM_BAND eBand;

	uint8_t ucDTIMPeriod;
	u_int8_t fgTIMPresent;

	/* This BSS's TimeStamp is larger than us(TCL == 1 in RX_STATUS_T) */
	u_int8_t fgIsLargerTSF;

	uint8_t ucRCPI;
	uint8_t aucRCPIAnt[MAX_ANTENNA_NUM];

	uint8_t fgIsUapsdSupported;

	/*! \brief The srbiter Search State will matched the scan result,
	 *   and saved the selected cipher and akm, and report the score,
	 *   for arbiter join state, join module will carry this target BSS
	 *   to rsn generate ie function, for gen wpa/rsn ie
	 */
	uint32_t u4RsnSelectedGroupCipher;
	uint32_t u4RsnSelectedPairwiseCipher;
	uint32_t u4RsnSelectedGroupMgmtCipher;
	uint32_t u4RsnSelectedAKMSuite;
	uint32_t u4RsnSelectedProto;
	uint32_t u4RsnSelectedPmf;
	enum ENUM_PARAM_AUTH_MODE eRsnSelectedAuthMode;

	uint16_t u2RsnCap;
	uint16_t u2RsnxCap;

	struct RSN_INFO rRSNInfo;
	struct RSN_INFO rWPAInfo;
	struct RSNX_INFO rRSNXInfo;
#if 1	/* CFG_SUPPORT_WAPI */
	struct WAPI_INFO rIEWAPI;
	u_int8_t fgIEWAPI;
#endif
	u_int8_t fgIERSN;
	u_int8_t fgIEWPA;
	u_int8_t fgIEOsen;
	u_int8_t fgIERSNX;
	u_int8_t fgIEOWETM;

#if (CFG_SUPPORT_RSNO == 1)
	enum ENUM_RSN_OVERRIDE eRsnSelectedRSNOverride;

	struct RSN_INFO rRSNOInfo;
	struct RSN_INFO rRSNO2Info;
	struct RSNX_INFO rRSNXOInfo;
	u_int8_t fgIERSNO;
	u_int8_t fgIERSNO2;
	u_int8_t fgIERSNXO;

	uint16_t u2RsnoCap;
	uint16_t u2Rsno2Cap;
	uint16_t u2RsnxoCap;
#endif /* CFG_SUPPORT_RSNO */

#if CFG_ENABLE_WIFI_DIRECT
	u_int8_t fgIsP2PPresent;
	u_int8_t fgIsP2PReport;	/* TRUE: report to upper layer */
	struct P2P_DEVICE_DESC *prP2pDesc;
	struct LINK rP2pDeviceList;

	/* For
	 *    1. P2P Capability.
	 *    2. P2P Device ID. ( in aucSrcAddr[] )
	 *    3. NOA   (TODO:)
	 *    4. Extend Listen Timing. (Probe Rsp)  (TODO:)
	 *    5. P2P Device Info. (Probe Rsp)
	 *    6. P2P Group Info. (Probe Rsp)
	 */
#endif

	/* the beacon doesn't advertise the FT AKM but will
	 * use FT when supported clients connect
	 */
	uint8_t ucIsAdaptive11r;
	uint8_t fgIsFtOverDS;
	u_int8_t fgSupportBTM; /* Indicates whether to support BTM */

	/* for Cistco CCX AP that supports single PMK
	 * don't use PMKID thus always use auth SAE
	 */
	uint8_t ucIsCiscoCCXIePresent;

	uint16_t u2RawLength;		/* The byte count of aucRawBuf[] */
	uint16_t u2IELength;		/* The byte count of aucIEBuf[] */

	/* Place u8TimeStamp before aucIEBuf[1] to force DW align */
	union ULARGE_INTEGER u8TimeStamp;

	uint8_t aucRawBuf[CFG_RAW_BUFFER_SIZE];
	uint8_t *pucIeBuf;
	OS_SYSTIME rJoinFailTime;

	/* Support AP Selection */
	struct AIS_BLOCKLIST_ITEM *prBlock;
	uint16_t u2Score;
	uint32_t u4Tput;
	uint8_t fgPicked;
	uint8_t fgIsMatchBssid;
	uint8_t fgIsMatchBssidHint;

#if CFG_SUPPORT_802_11K
	struct NEIGHBOR_AP *prNeighbor;
	uint8_t fgQueriedCandidates;
#endif
	uint16_t u2CurrCountryCode;
	uint8_t fgIsDisallowed;
	uint8_t fgExistEspIE;
	uint32_t u4EspInfo[ESP_AC_NUM];
	uint8_t fgExistEspOutIE;
	uint8_t ucEspOutInfo[ESP_AC_NUM];
	uint8_t fgIsRWMValid;
	uint16_t u2DownlinkAvailCap;
	uint16_t u2UplinkAvailCap;
	u_int8_t fgExistTxPwr;
	int8_t cTransmitPwr;
	uint16_t u2StaCnt;
	uint16_t u2AvaliableAC; /* Available Admission Capacity */
	uint8_t ucJoinFailureCount;
	uint8_t ucTempRejectCount;
	uint8_t ucChnlUtilization;
	uint8_t ucSNR;
	u_int8_t fgSeenProbeResp;
	u_int8_t fgExistBssLoadIE;
	u_int8_t fgMultiAnttenaAndSTBC;
	u_int8_t fgIsMCC;
	uint32_t u4UpdateIdx;
	uint8_t ucScanMissCount;
	uint8_t fgIotApActionValid;
	uint8_t ucIotVer;
	uint64_t u8IotApAct;
	uint16_t u2MaximumMpdu;
	/* end Support AP Selection */
	int8_t cPowerLimit;
	uint8_t aucRrmCap[5];
#if CFG_SUPPORT_HE_ER
	uint8_t fgIsERSUDisable;
	uint8_t ucDCMMaxConRx;
#endif
#if CFG_SUPPORT_MLR
	uint8_t ucMlrType;
	uint8_t ucMlrLength;
	uint8_t ucMlrSupportBitmap;
#endif
#if (CFG_SUPPORT_TX_PWR_ENV == 1)
	uint8_t fgIsTxPwrEnvPresent;
	uint8_t ucTxPwrEnvPwrLmtNum;
	int8_t aicTxPwrEnvMaxTxPwr[TX_PWR_ENV_MAX_TXPWR_BW_NUM];
	uint8_t fgExtSpecMgmtCap;
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrMode;
#endif
};

struct SCAN_PARAM {	/* Used by SCAN FSM */
	/* Active or Passive */
	enum ENUM_SCAN_TYPE eScanType;

	/* Network Type */
	uint8_t ucBssIndex;

	/* Specified SSID Type */
	uint8_t ucSSIDType;
	uint8_t ucSSIDNum;
	uint8_t ucShortSSIDNum;

	/* Length of Specified SSID */
	uint8_t ucSpecifiedSSIDLen[CFG_SCAN_SSID_MAX_NUM];

	/* Specified SSID */
	uint8_t aucSpecifiedSSID[CFG_SCAN_SSID_MAX_NUM][ELEM_MAX_LEN_SSID];

#if CFG_ENABLE_WIFI_DIRECT
	u_int8_t fgFindSpecificDev;	/* P2P: Discovery Protocol */
	uint8_t aucDiscoverDevAddr[MAC_ADDR_LEN];
	u_int8_t fgIsDevType;
	struct P2P_DEVICE_TYPE rDiscoverDevType;

	/* TODO: Find Specific Device Type. */
#endif	/* CFG_ENABLE_WIFI_DIRECT */

	uint16_t u2ChannelDwellTime;
	uint16_t u2ChannelMinDwellTime;
	uint16_t u2TimeoutValue;
#if CFG_SUPPORT_LLW_SCAN
	uint16_t u2OpChStayTime;
	uint8_t ucDfsChDwellTime;
	uint8_t ucPerScanChCnt;
	uint16_t u2LLWChannelDwellTime;
	uint16_t u2LLWChannelMinDwellTime;
#endif

	uint8_t ucBssidNum;
	uint8_t aucBSSID[CFG_SCAN_OOB_MAX_NUM][MAC_ADDR_LEN];

	enum ENUM_MSG_ID eMsgId;
	u_int8_t fgIsScanV2;

	/* Run time flags */
	uint16_t u2ProbeDelayTime;

	/* channel information */
	enum ENUM_SCAN_CHANNEL eScanChannel;
	uint8_t ucChannelListNum;
	struct RF_CHANNEL_INFO arChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];

	/* random mac */
	uint8_t ucScnFuncMask;
	uint32_t u4ScnFuncMaskExtend;
	uint8_t aucRandomMac[MAC_ADDR_LEN];

	/* Feedback information */
	uint8_t ucSeqNum;

	/* For OOB discovery*/
	uint8_t ucBssidMatchCh[CFG_SCAN_OOB_MAX_NUM];
	uint8_t ucBssidMatchSsidInd[CFG_SCAN_OOB_MAX_NUM];
	u_int8_t fgOobRnrParseEn;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	/* Short SSID */
	uint8_t aucShortSSID[CFG_SCAN_OOB_MAX_NUM][MAX_SHORT_SSID_LEN];
	uint8_t ucBssidMatchShortSsidInd[CFG_SCAN_OOB_MAX_NUM];
#endif

	/* Information Element */
	uint16_t u2IELen;
	uint8_t aucIE[MAX_IE_LENGTH];

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	uint8_t fgCollectMldAP;
	uint16_t u2IELenMl;
	uint8_t aucIEMl[MAX_BAND_IE_LENGTH];
	uint16_t u2IELen2G4;
	uint8_t aucIE2G4[MAX_BAND_IE_LENGTH];
	uint16_t u2IELen5G;
	uint8_t aucIE5G[MAX_BAND_IE_LENGTH];
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint16_t u2IELen6G;
	uint8_t aucIE6G[MAX_BAND_IE_LENGTH];
#endif
#endif
};

struct SCHED_SCAN_PARAM {	/* Used by SCAN FSM */
	uint8_t ucSeqNum;
	uint8_t ucBssIndex;              /* Network Type */
	u_int8_t fgStopAfterIndication;  /* always FALSE */
	uint8_t ucMatchSSIDNum;          /* Match SSID */
	struct BSS_DESC *aprPendingBssDescToInd[SCN_SSID_MATCH_MAX_NUM];
};

struct SCAN_LOG_ELEM_BSS {
	struct LINK_ENTRY rLinkEntry;

	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;
};

struct SCAN_LOG_CACHE {
	struct LINK rBSSListFW;
	struct LINK rBSSListCFG;

	struct SCAN_LOG_ELEM_BSS arBSSListBufFW[SCAN_LOG_BUFF_SIZE];
	struct SCAN_LOG_ELEM_BSS arBSSListBufCFG[SCAN_LOG_BUFF_SIZE];
};

struct CHNL_IDLE_SLOT {
	uint16_t au2ChIdleTime2G[SCN_2G_BAND_CHNL_SLOT_NUM];
	uint16_t au2ChIdleTime5G[SCN_5G_BAND_CHNL_SLOT_NUM];
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint16_t au2ChIdleTime6G[SCN_6G_BAND_CHNL_SLOT_NUM];
#endif
};

struct CHNL_DWELL_TIME {
	uint16_t au2ChDwellTime2G[SCN_2G_BAND_CHNL_SLOT_NUM];
	uint16_t au2ChDwellTime5G[SCN_5G_BAND_CHNL_SLOT_NUM];
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint16_t au2ChDwellTime6G[SCN_6G_BAND_CHNL_SLOT_NUM];
#endif
};

struct CHNL_UTIL_VAL {
	uint16_t au2ChUtilVal2G[SCN_2G_BAND_CHNL_SLOT_NUM];
	uint16_t au2ChUtilVal5G[SCN_5G_BAND_CHNL_SLOT_NUM];
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint16_t au2ChUtilVal6G[SCN_6G_BAND_CHNL_SLOT_NUM];
#endif
};

struct SCAN_INFO {
	/* Store the STATE variable of SCAN FSM */
	enum ENUM_SCAN_STATE eCurrentState;

	OS_SYSTIME rLastScanCompletedTime;

	struct SCAN_PARAM rScanParam;
	struct SCHED_SCAN_PARAM rSchedScanParam;

	uint32_t u4NumOfBssDesc;

	uint8_t ucScanDoneVersion;

	uint8_t aucScanBuffer[SCN_MAX_BUFFER_SIZE];

	struct LINK rBSSDescList;

	struct LINK rFreeBSSDescList;

	struct LINK rPendingMsgList;

#if CFG_SUPPORT_802_11BE_MLO
	struct LINK rMldAPInfoList;
#endif

	/* Sparse Channel Detection */
	u_int8_t fgIsSparseChannelValid;
	struct RF_CHANNEL_INFO rSparseChannel;

	/* Sched scan state tracking */
	u_int8_t fgSchedScanning;

	/* Full2Partial */
	OS_SYSTIME u4LastFullScanTime;
	u_int8_t fgIsScanForFull2Partial;
	u_int8_t ucFull2PartialSeq;
	uint32_t au4ChannelBitMap[SCAN_CHANNEL_BITMAP_ARRAY_LEN];

	/*channel idle count # Mike */
	uint8_t	 ucSparseChannelArrayValidNum;
	uint8_t	 aucReserved[3];
	uint8_t	 aucChannelNum[64];
	uint16_t	au2ChannelIdleTime[64];
	/* Mdrdy Count in each Channel  */
	uint8_t	 aucChannelMDRDYCnt[64];
	/* Beacon and Probe Response Count in each Channel */
	uint8_t	 aucChannelBAndPCnt[64];
	uint16_t	au2ChannelScanTime[64];
	/* CU value in each Channel  */
	uint8_t	 aucChannelUtilVal[64];
	/* eBand infor for differing the 2g4/6g */
	enum ENUM_BAND aeChannelBand[64];

	/* Support AP Selection */
	uint32_t u4ScanUpdateIdx;
	/* Scan log cache */
	struct SCAN_LOG_CACHE rScanLogCache;

	struct CHNL_IDLE_SLOT rSlotInfo;

	struct CHNL_DWELL_TIME rDwellInfo;

	struct CHNL_UTIL_VAL rChUtilVal;

#if CFG_SUPPORT_SCAN_NO_AP_RECOVERY
	uint8_t		ucScnZeroMdrdyTimes;
	uint8_t		ucScnZeroMdrdySerCnt;
	uint8_t		ucScnZeroMdrdySubsysResetCnt;
	uint8_t		ucScnTimeoutTimes;
	uint8_t		ucScnTimeoutSubsysResetCnt;
#if CFG_EXT_SCAN
	uint8_t		ucScnZeroChannelCnt;
	uint8_t		ucScnZeroChSubsysResetCnt;
#endif
#endif
	/*Skip DFS channel scan or not */
	u_int8_t	fgSkipDFS;
	uint8_t		fgIsScanTimeout;
	OS_SYSTIME rLastScanStartTime;

#if (CFG_SUPPORT_WIFI_RNR == 1)
	struct LINK rNeighborAPInfoList;
#endif

#if CFG_SUPPORT_SCAN_LOG
	uint8_t fgBcnReport;
#endif
	uint8_t fgWifiOnFirstScan;
};

/* Incoming Mailbox Messages */
struct MSG_SCN_SCAN_REQ {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucSeqNum;
	uint8_t ucBssIndex;
	enum ENUM_SCAN_TYPE eScanType;

	/* BIT(0) wildcard / BIT(1) P2P-wildcard / BIT(2) specific */
	uint8_t ucSSIDType;

	uint8_t ucSSIDLength;
	uint8_t aucSSID[PARAM_MAX_LEN_SSID];
	uint16_t u2ChannelDwellTime;	/* ms unit */
	uint16_t u2TimeoutValue;	/* ms unit */
	enum ENUM_SCAN_CHANNEL eScanChannel;
	uint8_t ucChannelListNum;
	struct RF_CHANNEL_INFO arChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];
	uint16_t u2IELen;
	uint8_t aucIE[MAX_IE_LENGTH];
};

struct MSG_SCN_SCAN_REQ_V2 {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucSeqNum;
	uint8_t ucBssIndex;
	enum ENUM_SCAN_TYPE eScanType;

	/* BIT(0) wildcard / BIT(1) P2P-wildcard / BIT(2) specific */
	uint8_t ucSSIDType;

	uint8_t ucSSIDNum;
	struct PARAM_SSID arSsid[CFG_SCAN_SSID_MAX_NUM];
	uint16_t u2ProbeDelay;
	uint16_t u2ChannelDwellTime;	/* In TU. 1024us. */
	uint16_t u2ChannelMinDwellTime;	/* In TU. 1024us. */
	uint16_t u2TimeoutValue;	/* ms unit */
#if CFG_SUPPORT_LLW_SCAN
	uint16_t u2OpChStayTime;	/* ms unit */
	uint8_t ucDfsChDwellTime;	/* ms unit */
	uint8_t ucPerScanChCnt;
	uint16_t u2LLWChannelDwellTime;
	uint16_t u2LLWChannelMinDwellTime;
#endif
	uint8_t aucBSSID[MAC_ADDR_LEN];
	enum ENUM_SCAN_CHANNEL eScanChannel;
	uint8_t ucChannelListNum;
	struct RF_CHANNEL_INFO arChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];
	uint8_t ucScnFuncMask;
	uint32_t u4ScnFuncMaskExtend;
	uint8_t aucRandomMac[MAC_ADDR_LEN];	/* random mac */

	/* pass from PARAM_SCAN_REQUEST_ADV.aucBssid */
	uint8_t aucExtBssid[CFG_SCAN_OOB_MAX_NUM][MAC_ADDR_LEN];
	uint8_t ucShortSSIDNum;
	/* For OOB discovery*/
	uint8_t ucBssidMatchCh[CFG_SCAN_OOB_MAX_NUM];
	uint8_t ucBssidMatchSsidInd[CFG_SCAN_OOB_MAX_NUM];
	u_int8_t fgOobRnrParseEn;

	uint16_t u2IELen;
	uint8_t aucIE[MAX_IE_LENGTH];

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	uint8_t fgNeedMloScan;
	uint16_t u2IELenMl;
	uint8_t aucIEMl[MAX_BAND_IE_LENGTH];
	uint16_t u2IELen2G4;
	uint8_t	aucIE2G4[MAX_BAND_IE_LENGTH];
	uint16_t u2IELen5G;
	uint8_t	aucIE5G[MAX_BAND_IE_LENGTH];
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint16_t u2IELen6G;
	uint8_t	aucIE6G[MAX_BAND_IE_LENGTH];
#endif
#endif
};

struct MSG_SCN_SCAN_CANCEL {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucSeqNum;
	uint8_t ucBssIndex;
	u_int8_t fgIsChannelExt;
	u_int8_t fgIsOidRequest;
};

/* Outgoing Mailbox Messages */
enum ENUM_SCAN_STATUS {
	SCAN_STATUS_DONE = 0,
	SCAN_STATUS_CANCELLED,
	SCAN_STATUS_FAIL,
	SCAN_STATUS_BUSY,
	SCAN_STATUS_NUM
};

struct MSG_SCN_SCAN_DONE {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucSeqNum;
	uint8_t ucBssIndex;
	enum ENUM_SCAN_STATUS eScanStatus;
};

#if (CFG_SUPPORT_WIFI_RNR == 1)

#define MAXIMUM_RNR_BSSID_LIST		16

struct NEIGHBOR_AP_PARAM {
	/* Specified SSID Type */
	uint8_t ucSSIDType;
	uint8_t ucSSIDNum;
	uint8_t ucShortSSIDNum;

	/* Length of Specified SSID */
	uint8_t ucSpecifiedSSIDLen[CFG_SCAN_SSID_MAX_NUM];

	/* Specified SSID */
	uint8_t aucSpecifiedSSID[CFG_SCAN_SSID_MAX_NUM][ELEM_MAX_LEN_SSID];

	uint8_t ucBssidNum;
	uint8_t aucBSSID[MAXIMUM_RNR_BSSID_LIST][MAC_ADDR_LEN];
	uint8_t ucBssidStartIdx;

	/* channel information */
	enum ENUM_SCAN_CHANNEL eScanChannel;
	uint8_t ucChannelListNum;
	struct RF_CHANNEL_INFO arChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];

	/* random mac */
	uint8_t ucScnFuncMask;
	uint8_t aucRandomMac[MAC_ADDR_LEN];

	/* short SSID */
	uint8_t aucShortSSID[MAXIMUM_RNR_BSSID_LIST][MAX_SHORT_SSID_LEN];
};

struct NEIGHBOR_AP_INFO {
	struct LINK_ENTRY rLinkEntry;
	struct NEIGHBOR_AP_PARAM rNeighborParam;
};
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
struct MLD_AP_INFO {
	struct LINK_ENTRY rLinkEntry;
	struct BSS_DESC *prBssDesc;
};
#endif

/* BSS_DESC_WRAPPER */
struct BSS_DESC_W {
	struct BSS_DESC *prBssDesc;
	enum ENUM_BAND eBand;
	enum ENUM_CHANNEL_WIDTH eChannelWidth;
	uint8_t ucLinkId;
	uint8_t fgUnSyncOm;
	uint8_t fgApRemoval;
};

struct BSS_DESC_SET {
	struct BSS_DESC *prMainBssDesc;
	uint8_t ucLinkNum; /* must smaller than MLD_LINK_MAX */
	uint8_t ucRfBandBmap;
	uint16_t u2ValidLinks;
	uint8_t fgIsMatchBssid;
	uint8_t fgIsMatchBssidHint;
	uint8_t fgIsAllLinkInBlockList;
	uint8_t fgIsAllLinkConnected;
	enum ENUM_MLO_MODE eMloMode;
	uint8_t ucMaxSimuLinks;
	struct BSS_DESC_W *aprBssDescW[MLD_LINK_MAX];
	struct BSS_DESC_W arBssDescWPool[MLD_LINK_MAX];
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
extern const char aucScanLogPrefix[][SCAN_LOG_PREFIX_MAX_LEN];

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#if DBG_DISABLE_ALL_LOG
#define scanlog_dbg(prefix, _Clz, _Fmt, ...)
#else /* DBG_DISABLE_ALL_LOG */
#define scanlog_dbg(prefix, _Clz, _Fmt, ...) \
	do { \
		if ((au2DebugModule[DBG_SCN_IDX] & \
			DBG_CLASS_##_Clz) == 0) \
			break; \
		LOG_FUNC("[%u]SCANLOG:(SCN " #_Clz ") %s " _Fmt, \
			KAL_GET_CURRENT_THREAD_ID(), \
			aucScanLogPrefix[prefix], ##__VA_ARGS__); \
	} while (0)
#endif /* DBG_DISABLE_ALL_LOG */

#define IS_2G_OP_CLASS(_opClass)	((_opClass >= 81) && (_opClass <= 84))
#define IS_5G_OP_CLASS(_opClass)	((_opClass >= 115) && (_opClass <= 130))
#define IS_5G_LOW_ONLY_OP_CLASS(_opClass)	\
	((_opClass >= 115) && (_opClass <= 117))
#define IS_5G_HIGH_ONLY_OP_CLASS(_opClass)	\
	((_opClass >= 124) && (_opClass <= 127))
#define IS_6G_OP_CLASS(_opClass)	((_opClass >= 131) && (_opClass <= 137))
#define IS_EHT_OP_CLASS(_opClass)	(_opClass == 137)
#define IS_6G_PSC_CHANNEL(_ch)		(((_ch - 5) % 16) == 0)

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/* Routines in scan.c                                                         */
/*----------------------------------------------------------------------------*/
void scnFreeAllPendingScanRquests(struct ADAPTER *prAdapter);

void scnInit(struct ADAPTER *prAdapter);

void scnUninit(struct ADAPTER *prAdapter);

/* Scan utilities */
uint32_t scanCountBits(uint32_t bitMap[], uint32_t bitMapSize);

void scanSetRequestChannel(struct ADAPTER *prAdapter,
		uint32_t u4ScanChannelNum,
		struct RF_CHANNEL_INFO arChannel[],
		uint32_t u4ScanFlags,
		uint8_t fgIsOnlineScan,
		struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg);

/* BSS-DESC Search */
struct BSS_DESC *scanSearchBssDescByBssid(struct ADAPTER *prAdapter,
					  uint8_t aucBSSID[]);


struct BSS_DESC *
scanSearchBssDescByBssidAndChnl(struct ADAPTER *prAdapter,
	uint8_t aucBSSID[], enum ENUM_BAND eBand, uint8_t ucChannel);

struct BSS_DESC *
scanSearchBssDescByBssidAndSsid(struct ADAPTER *prAdapter,
				uint8_t aucBSSID[],
				u_int8_t fgCheckSsid,
				struct PARAM_SSID *prSsid);

struct BSS_DESC *scanSearchBssDescByTA(struct ADAPTER *prAdapter,
				       uint8_t aucSrcAddr[]);

struct BSS_DESC *
scanSearchBssDescByTAAndSsid(struct ADAPTER *prAdapter,
			     uint8_t aucSrcAddr[],
			     u_int8_t fgCheckSsid,
			     struct PARAM_SSID *prSsid);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
struct BSS_DESC *
scanSearchBssDescByLinkIdMldAddrSsid(struct ADAPTER *prAdapter,
				  uint8_t ucLinkId,
				  uint8_t aucMldAddr[],
				  u_int8_t fgCheckSsid,
				  struct PARAM_SSID *prSsid);

uint8_t
scanSearchBssDescCountByMldAddrSsid(struct ADAPTER *prAdapter,
				  uint8_t aucMldAddr[],
				  u_int8_t fgCheckSsid,
				  struct PARAM_SSID *prSsid);
#endif

/* BSS-DESC Search - Alternative */
struct BSS_DESC *
scanSearchExistingBssDesc(struct ADAPTER *prAdapter,
			  enum ENUM_BSS_TYPE eBSSType,
			  uint8_t aucBSSID[],
			  uint8_t aucSrcAddr[]);

struct BSS_DESC *
scanSearchExistingBssDescWithSsid(struct ADAPTER *prAdapter,
				  enum ENUM_BSS_TYPE eBSSType,
				  uint8_t aucBSSID[],
				  uint8_t aucSrcAddr[],
				  u_int8_t fgCheckSsid,
				  struct PARAM_SSID *prSsid);

/* BSS-DESC Allocation */
struct BSS_DESC *scanAllocateBssDesc(struct ADAPTER *prAdapter);

/* BSS-DESC Removal */
void scanRemoveBssDescsByPolicy(struct ADAPTER *prAdapter,
				uint32_t u4RemovePolicy,
				struct SW_RFB *prSwRfb);

void scanRemoveBssDescByBssid(struct ADAPTER *prAdapter,
			      uint8_t aucBSSID[]);

void scanRemoveBssDescByBandAndNetwork(
				struct ADAPTER *prAdapter,
				enum ENUM_BAND eBand,
				uint8_t ucBssIndex);

/* BSS-DESC State Change */
void scanRemoveConnFlagOfBssDescByBssid(struct ADAPTER *prAdapter,
					uint8_t aucBSSID[],
					uint8_t ucBssIndex);

/* BSS-DESC Insertion - ALTERNATIVE */
struct BSS_DESC *scanAddToBssDesc(struct ADAPTER *prAdapter,
				  struct SW_RFB *prSwRfb,
				  uint8_t fgHasMLElement);

uint32_t scanProcessBeaconAndProbeResp(struct ADAPTER *prAdapter,
				       struct SW_RFB *prSWRfb);

void
scanBuildProbeReqFrameCommonIEs(struct MSDU_INFO *prMsduInfo,
				uint8_t *pucDesiredSsid,
				uint32_t u4DesiredSsidLen,
				uint16_t u2SupportedRateSet);

uint32_t scanSendProbeReqFrames(struct ADAPTER *prAdapter,
				struct SCAN_PARAM *prScanParam);

void scanUpdateBssDescForSearch(struct ADAPTER *prAdapter,
				struct BSS_DESC *prBssDesc);

uint32_t scanAddScanResult(struct ADAPTER *prAdapter,
			   struct BSS_DESC *prBssDesc,
			   struct SW_RFB *prSwRfb);

void scanReportBss2Cfg80211(struct ADAPTER *prAdapter,
			    enum ENUM_BSS_TYPE eBSSType,
			    struct BSS_DESC *SpecificprBssDesc);

bool scnEnableSplitScan(struct ADAPTER *prAdapter,
				uint8_t ucBssIndex,
				struct CMD_SCAN_REQ_V2 *prCmdScanReq);

/*----------------------------------------------------------------------------*/
/* Routines in scan_fsm.c                                                     */
/*----------------------------------------------------------------------------*/
void scnFsmSteps(struct ADAPTER *prAdapter,
		 enum ENUM_SCAN_STATE eNextState);

/*----------------------------------------------------------------------------*/
/* Command Routines                                                           */
/*----------------------------------------------------------------------------*/
void scnSendScanReq(struct ADAPTER *prAdapter);

void scnSendScanReqV2(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* RX Event Handling                                                          */
/*----------------------------------------------------------------------------*/
void scnEventScanDone(struct ADAPTER *prAdapter,
		      struct EVENT_SCAN_DONE *prScanDone,
		      u_int8_t fgIsNewVersion);

void scnEventSchedScanDone(struct ADAPTER *prAdapter,
		     struct EVENT_SCHED_SCAN_DONE *prSchedScanDone);

/*----------------------------------------------------------------------------*/
/* Mailbox Message Handling                                                   */
/*----------------------------------------------------------------------------*/
void scnFsmMsgStart(struct ADAPTER *prAdapter,
		    struct MSG_HDR *prMsgHdr);

void scnFsmMsgAbort(struct ADAPTER *prAdapter,
		    struct MSG_HDR *prMsgHdr);

void scnFsmHandleScanMsg(struct ADAPTER *prAdapter,
			 struct MSG_SCN_SCAN_REQ *prScanReqMsg);

void scnFsmHandleScanMsgV2(struct ADAPTER *prAdapter,
			   struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg);

void scnFsmRemovePendingMsg(struct ADAPTER *prAdapter,
			    uint8_t ucSeqNum,
			    uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/* Mailbox Message Generation                                                 */
/*----------------------------------------------------------------------------*/
void
scnFsmGenerateScanDoneMsg(struct ADAPTER *prAdapter,
			  enum ENUM_MSG_ID eMsgId,
			  uint8_t ucSeqNum,
			  uint8_t ucBssIndex,
			  enum ENUM_SCAN_STATUS eScanStatus,
			  enum EUNM_MSG_SEND_METHOD eMethod);

/*----------------------------------------------------------------------------*/
/* Query for sparse channel                                                   */
/*----------------------------------------------------------------------------*/
u_int8_t scnQuerySparseChannel(struct ADAPTER *prAdapter,
			       enum ENUM_BAND *prSparseBand,
			       uint8_t *pucSparseChannel);

/*----------------------------------------------------------------------------*/
/* OID/IOCTL Handling                                                         */
/*----------------------------------------------------------------------------*/
#if CFG_SUPPORT_PASSPOINT
struct BSS_DESC *scanSearchBssDescByBssidAndLatestUpdateTime(
						struct ADAPTER *prAdapter,
						uint8_t aucBSSID[]);
#endif /* CFG_SUPPORT_PASSPOINT */

#if CFG_SUPPORT_SCHED_SCAN
u_int8_t scnFsmSchedScanRequest(struct ADAPTER *prAdapter,
			struct PARAM_SCHED_SCAN_REQUEST *prSchedScanRequest);

u_int8_t scnFsmSchedScanStopRequest(struct ADAPTER *prAdapter);

u_int8_t scnFsmSchedScanSetAction(struct ADAPTER *prAdapter,
			enum ENUM_SCHED_SCAN_ACT ucSchedScanAct);

u_int8_t scnFsmSchedScanSetCmd(struct ADAPTER *prAdapter,
			struct CMD_SCHED_SCAN_REQ *prSchedScanCmd);

void scnSetSchedScanPlan(struct ADAPTER *prAdapter,
			struct CMD_SCHED_SCAN_REQ *prSchedScanCmd,
			uint16_t u2ScanInterval);

#endif /* CFG_SUPPORT_SCHED_SCAN */

u_int8_t scnDoAllowScanCheck(struct ADAPTER *prAdapter);

#if CFG_SUPPORT_SCAN_NO_AP_RECOVERY
void scnDoZeroMdrdyRecoveryCheck(struct ADAPTER *prAdapter,
			struct EVENT_SCAN_DONE *prScanDone,
			struct SCAN_INFO *prScanInfo, uint8_t ucBssIndex);
void scnDoScanTimeoutRecoveryCheck(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex);
#if CFG_EXT_SCAN
void scnDoZeroChRecoveryCheck(struct ADAPTER *prAdapter,
			struct SCAN_INFO *prScanInfo);
#endif
#endif

void scnFsmNotifyEvent(struct ADAPTER *prAdapter,
			enum ENUM_SCAN_STATUS eStatus,
			uint8_t ucBssIndex);
void scanLogEssResult(struct ADAPTER *prAdapter);
void scanInitEssResult(struct ADAPTER *prAdapter);
#if CFG_SUPPORT_SCAN_CACHE_RESULT
/*----------------------------------------------------------------------------*/
/* Routines in scan_cache.c                                                   */
/*----------------------------------------------------------------------------*/
u_int8_t isScanCacheDone(struct GL_SCAN_CACHE_INFO *prScanCache);
#endif /* CFG_SUPPORT_SCAN_CACHE_RESULT */

void scanReqLog(struct CMD_SCAN_REQ_V2 *prCmdScanReq);
void scanResultLog(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb);
void scanLogCacheAddBSS(struct LINK *prList,
	struct SCAN_LOG_ELEM_BSS *prListBuf,
	enum ENUM_SCAN_LOG_PREFIX prefix,
	uint8_t bssId[], uint16_t seq);
void scanLogCacheFlushBSS(struct LINK *prList,
			enum ENUM_SCAN_LOG_PREFIX prefix);
void scanLogCacheFlushAll(struct ADAPTER *prAdapter,
	struct SCAN_LOG_CACHE *prScanLogCache,
	enum ENUM_SCAN_LOG_PREFIX prefix);
uint8_t *scanLogSSIDType(uint8_t ucSSIDType);

void scanFillChnlInfo(struct ADAPTER *ad, enum ENUM_BAND eBand,
	uint8_t ucChNum, uint16_t u2IdleTime, uint16_t u2DwellTime,
	uint8_t ucChUtilVal);

uint16_t scanGetChnlIdleSlot(struct ADAPTER *ad, enum ENUM_BAND eBand,
	uint8_t ucChNum);

uint16_t scanGetChnlDwellTime(struct ADAPTER *ad, enum ENUM_BAND eBand,
	uint8_t ucChNum);

uint8_t scanGetChnlUtilVal(struct ADAPTER *ad, enum ENUM_BAND eBand,
	uint8_t ucChNum);

void scanRemoveBssDescFromList(struct ADAPTER *prAdapter,
			       struct LINK *prBSSDescList,
			       struct BSS_DESC *prBssDesc);
void scanInsertBssDescToList(struct LINK *prBSSDescList,
			     struct BSS_DESC *prBssDesc,
			     u_int8_t init);
void scanResetBssDesc(struct ADAPTER *prAdapter,
		      struct BSS_DESC *prBssDesc);

void scanFillBssDescW(struct BSS_DESC_W *prBssDescW,
	struct BSS_DESC *prBssDesc);

/* Check if VHT IE filled in Epigram IE */
void scanCheckEpigramVhtIE(uint8_t *pucBuf, struct BSS_DESC *prBssDesc);
void scanParseVHTCapIE(uint8_t *pucIE, struct BSS_DESC *prBssDesc);
void scanParseVHTOpIE(uint8_t *pucIE, struct BSS_DESC *prBssDesc);

void scanCheckAdaptive11rIE(uint8_t *pucBuf, struct BSS_DESC *prBssDesc);
void scanCheckCiscoCCXIE(uint8_t *pucBuf, struct BSS_DESC *prBssDesc);
void scanParseCheckMTKOuiIE(struct ADAPTER *prAdapter,
	uint8_t *pucIE, struct BSS_DESC *prBssDesc,
	enum ENUM_BAND eHwBand, uint16_t u2FrameCtrl);
void scanParseWMMIE(struct ADAPTER *prAdapter,
	uint8_t *pucIE, struct BSS_DESC *prBssDesc);

void scanHandleOceIE(struct SCAN_PARAM *prScanParam,
	struct CMD_SCAN_REQ_V2 *prCmdScanReq);

uint8_t	*scanGetFilsCacheIdFromBssDesc(struct BSS_DESC *bss);

void scnFsmDumpScanDoneInfo(struct ADAPTER *prAdapter,
	struct EVENT_SCAN_DONE *prScanDone);

#if (CFG_SUPPORT_WIFI_6G == 1)
void scanParseHEOpIE(uint8_t *pucIE, struct BSS_DESC *prBssDesc,
	enum ENUM_BAND eHwBand);
#endif

#if (CFG_SUPPORT_802_11BE == 1)
void scanParseEhtCapIE(uint8_t *pucIE, struct BSS_DESC *prBssDesc);
void scanParseEhtOpIE(uint8_t *pucIE, struct BSS_DESC *prBssDesc,
	enum ENUM_BAND eHwBand);
#endif

enum ENUM_BAND scanOpClassToBand(uint8_t ucOpClass);

void updateLinkStatsApRec(struct ADAPTER *prAdapter,
		struct BSS_DESC *prBssDesc);

const char *SSID2STR(const uint8_t *ssid, uint8_t ssid_len);

#if (CONFIG_BAND_NUM > 2)
uint8_t scnCheckIsOnlyAbandP2PConnected(struct ADAPTER *prAdapter);
#endif

#endif /* _SCAN_H */
