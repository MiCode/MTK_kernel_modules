/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "cnm_mem.h"
 *    \brief  In this file we define the structure of the control unit of
 *    packet buffer and MGT/MSG Memory Buffer.
 */


#ifndef _CNM_MEM_H
#define _CNM_MEM_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "qosmap.h"
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#ifndef POWER_OF_2
#define POWER_OF_2(n)				BIT(n)
#endif

/* Size of a basic management buffer block in power of 2 */
/* 7 to the power of 2 = 128 */
#define MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2	7

/* 5 to the power of 2 = 32 */
#define MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2	5

/* Size of a basic management buffer block */
#define MGT_BUF_BLOCK_SIZE	POWER_OF_2(MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2)
#define MSG_BUF_BLOCK_SIZE	POWER_OF_2(MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2)

/* Total size of (n) basic management buffer blocks */
#define MGT_BUF_BLOCKS_SIZE(n) \
	((uint32_t)(n) << MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2)
#define MSG_BUF_BLOCKS_SIZE(n) \
	((uint32_t)(n) << MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2)

/* Number of management buffer block */
#define MAX_NUM_OF_BUF_BLOCKS			32	/* Range: 1~32 */

/* Size of overall management frame buffer */
#define MGT_BUFFER_SIZE		(MAX_NUM_OF_BUF_BLOCKS * MGT_BUF_BLOCK_SIZE)
#define MSG_BUFFER_SIZE		(MAX_NUM_OF_BUF_BLOCKS * MSG_BUF_BLOCK_SIZE)

#define ANY_BSS_INDEX			0xFF

/* STA_REC related definitions */
#define STA_REC_INDEX_BMCAST		0xFF
#define STA_REC_INDEX_NOT_FOUND		0xFE

/* Number of SW queues in each STA_REC: AC0~AC4 */
#define STA_WAIT_QUEUE_NUM		5

/* Number of SC caches in each STA_REC: AC0~AC4 */
#define SC_CACHE_INDEX_NUM		5

/* P2P related definitions */
#if CFG_ENABLE_WIFI_DIRECT
/* Moved from p2p_fsm.h */
#define WPS_ATTRI_MAX_LEN_DEVICE_NAME		32	/* 0x1011 */

/* NOTE(Kevin): Shall <= 16 */
#define P2P_GC_MAX_CACHED_SEC_DEV_TYPE_COUNT	8
#endif

/* Define the argument of cnmStaFreeAllStaByNetwork when all station records
 * will be free. No one will be free
 */
#define STA_REC_EXCLUDE_NONE		CFG_STA_REC_NUM

#define MLD_LINK_INDEX_NOT_FOUND	0xFF

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
/* Use 32 bits to represent buffur bitmap in BUF_INFO, i.e., the variable
 * rFreeBlocksBitmap in BUF_INFO structure.
 */
#if (MAX_NUM_OF_BUF_BLOCKS != 32)
#error > #define MAX_NUM_OF_MGT_BUF_BLOCKS should be 32 !
#endif /* MAX_NUM_OF_MGT_BUF_BLOCKS */

/* Control variable of TX management memory pool */
struct BUF_INFO {
	uint8_t *pucBuf;

#if CFG_DBG_MGT_BUF
	uint32_t u4AllocCount;
	uint32_t u4FreeCount;
	uint32_t u4AllocNullCount;
#endif	/* CFG_DBG_MGT_BUF */

	uint32_t rFreeBlocksBitmap;
	uint8_t aucAllocatedBlockNum[MAX_NUM_OF_BUF_BLOCKS];
};

/* Wi-Fi divides RAM into three types
 * MSG:     Mailbox message (Small size)
 * BUF:     HW DMA buffers (HIF/MAC)
 */
enum ENUM_RAM_TYPE {
	RAM_TYPE_MSG = 0,
	RAM_TYPE_BUF
};

enum ENUM_BUFFER_SOURCE {
	BUFFER_SOURCE_HIF_TX0 = 0,
	BUFFER_SOURCE_HIF_TX1,
	BUFFER_SOURCE_MAC_RX,
	BUFFER_SOURCE_MNG,
	BUFFER_SOURCE_BCN,
	BUFFER_SOURCE_NUM
};

enum ENUM_SEC_STATE {
	SEC_STATE_INIT,
	SEC_STATE_INITIATOR_PORT_BLOCKED,
	SEC_STATE_RESPONDER_PORT_BLOCKED,
	SEC_STATE_CHECK_OK,
	SEC_STATE_SEND_EAPOL,
	SEC_STATE_SEND_DEAUTH,
	SEC_STATE_COUNTERMEASURE,
	SEC_STATE_NUM
};

struct TSPEC_ENTRY {
	uint8_t ucStatus;
	uint8_t ucToken;	/* Dialog Token in ADDTS_REQ or ADDTS_RSP */
	uint16_t u2MediumTime;
	uint32_t u4TsInfo;
	/* PARAM_QOS_TS_INFO rParamTsInfo; */
	/* Add other retained QoS parameters below */
};

#if 0
struct SEC_INFO {

	enum ENUM_SEC_STATE ePreviousState;
	enum ENUM_SEC_STATE eCurrentState;

	u_int8_t fg2nd1xSend;
	u_int8_t fgKeyStored;

	uint8_t aucStoredKey[64];

	u_int8_t fgAllowOnly1x;
};
#endif

#define MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS	3

#define UPDATE_BSS_RSSI_INTERVAL_SEC		3	/* Seconds */

/* Fragment information structure */
struct FRAG_INFO {
	uint16_t u2SeqNo;
	uint8_t ucNextFragNo;
#if CFG_SUPPORT_FRAG_AGG_VALIDATION
	uint8_t ucSecMode;
	uint64_t u8NextPN;
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */
	uint8_t *pucNextFragStart;
	struct SW_RFB *pr1stFrag;

	/* The receive time of 1st fragment */
	OS_SYSTIME rReceiveLifetimeLimit;
};

#if CFG_SUPPORT_802_11W
/* AP PMF */
struct AP_PMF_CFG {
	u_int8_t fgMfpc;
	u_int8_t fgMfpr;
	u_int8_t fgSha256;
	u_int8_t fgAPApplyPmfReq;
	u_int8_t fgBipKeyInstalled;
};

struct STA_PMF_CFG {
	u_int8_t fgMfpc;
	u_int8_t fgMfpr;
	u_int8_t fgSha256;
	u_int8_t fgSaeRequireMfp;
	u_int8_t fgApplyPmf;
	u_int8_t fgBipKeyInstalled;

	/* for certification 4.3.3.1, 4.3.3.2 TX unprotected deauth */
	u_int8_t fgRxDeauthResp;

	/* For PMF SA query TX request retry a timer */
	/* record the start time of 1st SAQ request */
	uint32_t u4SAQueryStart;

	uint32_t u4SAQueryCount;
	uint8_t ucSAQueryTimedOut;	/* retry more than 1000ms */
	struct TIMER rSAQueryTimer;
	uint16_t u2TransactionID;
};
#endif

/* Define a structure identical to cfg80211_qos_map in linux kernel redundantly
 * for independent from underlying OS
 */
#define QOS_MAP_MAX_EX 21
struct DSCP_EXCEPTION {
	uint8_t dscp;
	uint8_t up;
};

struct DSCP_RANGE {
	uint8_t low;
	uint8_t high;
};

struct QOS_MAP {
	uint8_t ucDscpExNum; /* 0..21 */
	struct DSCP_EXCEPTION arDscpException[QOS_MAP_MAX_EX];
	struct DSCP_RANGE arDscpRange[WMM_UP_INDEX_NUM];
};

/* Define STA record structure */
struct STA_RECORD {
	struct LINK_ENTRY rLinkEntry;
	struct LINK_ENTRY rLinkEntryMld;
	uint8_t ucIndex;	/* Not modify it except initializing */
	uint8_t ucWlanIndex;	/* WLAN table index */

#if 0 /* TODO: Remove this */
	/* The BSS STA Rx WLAN index, IBSS Rx BC WLAN table
	 * index, work at IBSS Open and WEP
	 */
	uint8_t ucBMCWlanIndex;
#endif

	u_int8_t fgIsInUse;	/* Indicate if this entry is in use or not */
	uint8_t aucMacAddr[MAC_ADDR_LEN];	/* MAC address */

	/* SAA/AAA */
	/* Store STATE Value used in SAA/AAA */
	enum ENUM_AA_STATE eAuthAssocState;

	uint8_t ucAuthAssocReqSeqNum;

	/* Indicate the role of this STA in the network (for example, P2P GO) */
	enum ENUM_STA_TYPE eStaType;

	uint8_t ucBssIndex;	/* BSS_INFO_I index */

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	uint8_t ucMldStaIndex;	/* MLD_STAREC index */
	uint8_t ucLinkIndex;
	/*
	 * the tid-to-link bitmap,  BIT0 for TID0, BIT1 for TID1...
	 *     1'b1: supoort transmission for the TID in this link
	 *     1'b0: NOT support transmission for the TID in this link
	 */
	uint8_t ucULTidBitmap;
	uint8_t ucDLTidBitmap;
	uint8_t ucPendingULTidBitmap;
	uint8_t ucPendingDLTidBitmap;
	uint8_t aucMldAddr[MAC_ADDR_LEN];
	uint8_t fgApRemoval;
#endif

	uint8_t ucStaState;	/* STATE_1,2,3 */

	/* Available PHY Type Set of this peer (may deduced from
	 * received struct BSS_DESC)
	 */
	uint8_t ucPhyTypeSet;

	/* record from bcn or probe response */
	uint8_t ucVhtCapNumSoundingDimensions;

	/* The match result by AND operation of peer's PhyTypeSet and ours. */
	uint8_t ucDesiredPhyTypeSet;

	/* A flag to indicate a Basic Phy Type which is used to generate some
	 * Phy Attribute IE (e.g. capability, MIB) during association.
	 */
	u_int8_t fgHasBasicPhyType;

	/* The Basic Phy Type chosen among the ucDesiredPhyTypeSet. */
	uint8_t ucNonHTBasicPhyType;

	uint16_t u2HwDefaultFixedRateCode;

	/* For Infra Mode, to store Capability Info. from Association Resp(SAA).
	 * For AP Mode, to store Capability Info. from Association Req(AAA).
	 */
	uint16_t u2CapInfo;

	/* For Infra Mode, to store AID from Association Resp(SAA).
	 * For AP Mode, to store the Assigned AID(AAA).
	 */
	uint16_t u2AssocId;

	uint16_t u2ListenInterval;	/* Listen Interval from STA(AAA) */

	/* Our Current Desired Rate Set after match
	 * with STA's Operational Rate Set
	 */
	uint16_t u2DesiredNonHTRateSet;

	uint16_t u2OperationalRateSet;	/* Operational Rate Set of peer BSS */
	uint16_t u2BSSBasicRateSet;	/* Basic Rate Set of peer BSS */

	/* For IBSS Mode, to indicate that Merge is ongoing */
	u_int8_t fgIsMerging;

	/* For Infra/AP Mode, to diagnose the Connection with this peer
	 * by sending ProbeReq/Null frame
	 */
	u_int8_t fgDiagnoseConnection;

	/*----------------------------------------------------------------------
	 * 802.11n HT capabilities when (prStaRec->ucPhyTypeSet &
	 * PHY_TYPE_BIT_HT) is true. They have the same definition with fields
	 * of information element
	 *----------------------------------------------------------------------
	 */
	uint8_t ucMcsSet;	/* MCS0~7 rate set of peer BSS */
	u_int8_t fgSupMcs32;	/* MCS32 is supported by peer BSS */
	uint8_t aucRxMcsBitmask[SUP_MCS_RX_BITMASK_OCTET_NUM];
	uint16_t u2RxHighestSupportedRate;
	uint32_t u4TxRateInfo;
	uint16_t u2HtCapInfo;	/* HT cap info field by HT cap IE */
	uint8_t ucAmpduParam;	/* Field A-MPDU Parameters in HT cap IE */
	uint16_t u2HtExtendedCap;	/* HT extended cap field by HT cap IE */

	/* TX beamforming cap field by HT cap IE */
	uint32_t u4TxBeamformingCap;

	uint8_t ucAselCap;	/* ASEL cap field by HT cap IE */

#if 1	/* CFG_SUPPORT_802_11AC */
	/*----------------------------------------------------------------------
	 * 802.11ac  VHT capabilities when (prStaRec->ucPhyTypeSet &
	 * PHY_TYPE_BIT_VHT) is true. They have the same definition with fields
	 * of information element
	 *----------------------------------------------------------------------
	 */
	uint32_t u4VhtCapInfo;
	uint16_t u2VhtRxMcsMap;
	uint16_t u2VhtRxMcsMapAssoc;
	uint16_t u2VhtRxHighestSupportedDataRate;
	uint16_t u2VhtTxMcsMap;
	uint16_t u2VhtTxHighestSupportedDataRate;
	uint8_t ucVhtOpMode;
#endif
	uint8_t ucOpModeInOpNotificationIE;

#if (CFG_SUPPORT_802_11AX == 1)
	/*--------------------------------------------------------------------*/
	/* HE capability if (prStaRec->ucPhyTypeSet & PHY_TYPE_BIT_HE) is set */
	/* They have the same definition with fields of information element   */
	/*--------------------------------------------------------------------*/
	uint8_t ucHeMacCapInfo[HE_MAC_CAP_BYTE_NUM];
	uint8_t ucHePhyCapInfo[HE_PHY_CAP_BYTE_NUM];

	uint16_t u2HeRxMcsMapBW80;
	uint16_t u2HeRxMcsMapBW80Assoc;
	uint16_t u2HeTxMcsMapBW80;
	uint16_t u2HeRxMcsMapBW160;
	uint16_t u2HeRxMcsMapBW160Assoc;
	uint16_t u2HeTxMcsMapBW160;
	uint16_t u2HeRxMcsMapBW80P80;
	uint16_t u2HeRxMcsMapBW80P80Assoc;
	uint16_t u2HeTxMcsMapBW80P80;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	/*--------------------------------------------------------------------*/
	/* EHT capability if (prStaRec->ucPhyTypeSet &                        */
	/*  PHY_TYPE_BIT_EHT) is set                                          */
	/* They have the same definition with fields of information element   */
	/*--------------------------------------------------------------------*/
	uint8_t ucEhtMacCapInfo[EHT_MAC_CAP_BYTE_NUM];
	uint8_t ucEhtPhyCapInfo[EHT_PHY_CAP_BYTE_NUM];
	uint8_t ucEhtPhyCapInfoExt[EHT_PHY_CAP_BYTE_NUM];
	uint8_t aucMcsMap20MHzSta[4];
	uint8_t aucMcsMap80MHz[3];
	uint8_t aucMcsMap160MHz[3];
	uint8_t aucMcsMap320MHz[3];
#endif

#if (CFG_SUPPORT_WIFI_6G == 1)
	/* HE 6 GHz Band Capabilities */
	uint16_t u2He6gBandCapInfo;
#endif

	/*----------------------------------------------------------------------
	 * 802.11ac  HT operation info when (prStaRec->ucPhyTypeSet &
	 * PHY_TYPE_BIT_HT) is true. They have the same definition with fields
	 * of information element
	 *----------------------------------------------------------------------
	 */
	uint8_t ucHtPeerOpInfo1; /* Backup peer HT OP Info */
	uint16_t u2HtPeerOpInfo2; /* Backup peer HT OP Info */

	/*----------------------------------------------------------------------
	 * 802.11ac  VHT operation info when (prStaRec->ucPhyTypeSet &
	 * PHY_TYPE_BIT_VHT) is true. They have the same definition with fields
	 * of information element
	 *----------------------------------------------------------------------
	 */
	/* Backup peer VHT Op Info */
	uint8_t ucVhtOpChannelWidth;
	uint8_t ucVhtOpChannelFrequencyS1;
	uint8_t ucVhtOpChannelFrequencyS2;


	uint8_t ucRCPI;		/* RCPI of peer */

	/* Target BSS's DTIM Period, we use this value for
	 * setup Listen Interval
	 * TODO(Kevin): TBD
	 */
	uint8_t ucDTIMPeriod;

	/* For Infra/AP Mode, the Auth Algorithm Num used
	 * in Authentication(SAA/AAA)
	 */
	uint8_t ucAuthAlgNum;
	uint8_t ucAuthTranNum; /* For Infra/AP Mode, the Auth Transaction Number
				  */

	/* For Infra/AP Mode, to indicate ReAssoc Frame was in used(SAA/AAA) */
	u_int8_t fgIsReAssoc;

	/* For Infra Mode, the Retry Count of TX Auth/Assod Frame(SAA) */
	uint8_t ucTxAuthAssocRetryCount;

	/* For Infra Mode, the Retry Limit of TX Auth/Assod Frame(SAA) */
	uint8_t ucTxAuthAssocRetryLimit;

	uint16_t u2StatusCode;	/* Status of Auth/Assoc Req */
	uint16_t u2ReasonCode;	/* Reason that been Deauth/Disassoc */
	u_int8_t fgIsLocallyGenerated;

	/* Point to an allocated buffer for storing Challenge */
	/* Text for Shared Key Authentication */
	struct IE_CHALLENGE_TEXT *prChallengeText;

	/* For Infra Mode, a timer used to send a timeout event
	 * while waiting for TX request done or RX response.
	 */
	struct TIMER rTxReqDoneOrRxRespTimer;

	/* For Infra Mode, a timer used to avoid the Deauth frame
	 * not be sent
	 */
	struct TIMER rDeauthTxDoneTimer;

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	struct FILS_INFO *prFilsInfo;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

	/*----------------------------------------------------------------------
	 * Power Management related fields (for STA/ AP/ P2P/ BOW power saving
	 * mode)
	 *----------------------------------------------------------------------
	 */
	/* For Infra Mode, to indicate that outgoing frame need
	 * toggle the Pwr Mgt Bit in its Frame Control Field.
	 */
	u_int8_t fgSetPwrMgtBit;

	/* For AP Mode, to indicate the client PS state(PM).
	 * TRUE: In PS Mode; FALSE: In Active Mode.
	 */
	u_int8_t fgIsInPS;

	/* For Infra Mode, to indicate we've sent a PS POLL to AP
	 * and start the PS_POLL Service Period(LP)
	 */
	u_int8_t fgIsInPsPollSP;

	/* For Infra Mode, to indicate we've sent a Trigger Frame
	 * to AP and start the Delivery Service Period(LP)
	 */
	u_int8_t fgIsInTriggerSP;

	uint8_t ucBmpDeliveryAC;	/* 0: AC0, 1: AC1, 2: AC2, 3: AC3 */

	uint8_t ucBmpTriggerAC;		/* 0: AC0, 1: AC1, 2: AC2, 3: AC3 */

	uint8_t ucUapsdSp;		/* Max SP length */

	/*--------------------------------------------------------------------*/

	u_int8_t fgIsRtsEnabled;

	/* (4) System Timestamp of Successful TX and RX */
	uint32_t rUpdateTime;

	/* (4) System Timestamp of latest JOIN process */
	uint32_t rLastJoinTime;

	uint8_t ucJoinFailureCount;	/* Retry Count of JOIN process */

	/* For TXM to defer pkt forwarding to MAC TX DMA */
	struct LINK arStaWaitQueue[STA_WAIT_QUEUE_NUM];

	/* Duplicate removal for HT STA on a
	 * per-TID basis ("+1" is for MMPDU and non-QoS)
	 */
	uint16_t au2CachedSeqCtrl[TID_NUM + 1];

	u_int8_t afgIsIgnoreAmsduDuplicate[TID_NUM + 1];

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
	uint16_t au2AmsduInvalidSN[TID_NUM + 1];
	u_int8_t afgIsAmsduInvalid[TID_NUM + 1];
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

#if 0
	/* RXM */
	struct RX_BA_ENTRY *aprRxBaTable[TID_NUM];

	/* TXM */
	P_TX_BA_ENTRY_T aprTxBaTable[TID_NUM];
#endif

	struct FRAG_INFO rFragInfo[TID_NUM + 1][
			MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS];

#if 0 /* TODO: Remove this */
	struct SEC_INFO rSecInfo; /* The security state machine */
#endif

#if CFG_SUPPORT_ADHOC
	/* Ad-hoc RSN Rx BC key exist flag, only reserved two
	 * entry for each peer
	 */
	u_int8_t fgAdhocRsnBcKeyExist[2];

	/* Ad-hoc RSN Rx BC wlan index */
	uint8_t ucAdhocRsnBcWlanIndex[2];
#endif

	u_int8_t fgPortBlock;	/* The 802.1x Port Control flag */

	u_int8_t fgTransmitKeyExist;	/* Unicast key exist for this STA */

	u_int8_t fgTxAmpduEn;	/* Enable TX AMPDU for this Peer */
	u_int8_t fgRxAmpduEn;	/* Enable RX AMPDU for this Peer */

	uint8_t *pucAssocReqIe;
	uint16_t u2AssocReqIeLen;

	uint8_t *pucAssocRespIe;
	uint16_t u2AssocRespIeLen;

	/* link layer satatistics */
	struct WIFI_WMM_AC_STAT arLinkStatistics[WMM_AC_INDEX_NUM];

	/*----------------------------------------------------------------------
	 * WMM/QoS related fields
	 *----------------------------------------------------------------------
	 */
	/* If the STA is associated as a QSTA or QAP (for TX/RX) */
	u_int8_t fgIsQoS;

	/* If the peer supports WMM, set to TRUE (for association) */
	u_int8_t fgIsWmmSupported;

	/* Set according to the scan result (for association) */
	u_int8_t fgIsUapsdSupported;

	u_int8_t afgAcmRequired[ACI_NUM];

#if (CFG_SUPPORT_802_11AX == 1)
	/* If the peer supports MU EDCA, set to TRUE (for association)*/
	u_int8_t fgIsMuEdcaSupported;
#endif

	/*----------------------------------------------------------------------
	 * P2P related fields
	 *----------------------------------------------------------------------
	 */
#if CFG_ENABLE_WIFI_DIRECT
	uint8_t u2DevNameLen;
	uint8_t aucDevName[WPS_ATTRI_MAX_LEN_DEVICE_NAME];

	uint8_t aucDevAddr[MAC_ADDR_LEN];	/* P2P Device Address */

	uint16_t u2ConfigMethods;

	uint8_t ucDeviceCap;

	uint8_t ucSecondaryDevTypeCount;

	struct DEVICE_TYPE rPrimaryDevTypeBE;

	struct DEVICE_TYPE arSecondaryDevTypeBE[
		P2P_GC_MAX_CACHED_SEC_DEV_TYPE_COUNT];
#endif	/* CFG_SUPPORT_P2P */

	/*----------------------------------------------------------------------
	 * QM related fields
	 *----------------------------------------------------------------------
	 */
	/* Per Sta flow controal. Valid when fgIsInPS is TRUE.
	 * Change it for per Queue flow control
	 */
	uint8_t ucFreeQuota;

#if 0 /* TODO: Remove this */
	/* used in future */
	uint8_t aucFreeQuotaPerQueue[NUM_OF_PER_STA_TX_QUEUES];
#endif
	uint8_t ucFreeQuotaForDelivery;
	uint8_t ucFreeQuotaForNonDelivery;

	/*----------------------------------------------------------------------
	 * TXM related fields
	 *----------------------------------------------------------------------
	 */
	void *aprTxDescTemplate[TX_DESC_TID_NUM];

#if CFG_ENABLE_PKT_LIFETIME_PROFILE && CFG_ENABLE_PER_STA_STATISTICS
	uint32_t u4TotalTxPktsNumber;
	uint32_t u4TotalTxPktsTime;
	uint32_t u4TotalTxPktsHifTxTime;

	uint32_t u4TotalRxPktsNumber;
	uint32_t u4MaxTxPktsTime;
	uint32_t u4MaxTxPktsHifTime;

	uint32_t u4ThresholdCounter;
	uint32_t u4EnqueueCounter;
	uint32_t u4DeqeueuCounter;
#endif
#if CFG_AP_80211KVR_INTERFACE
	uint64_t u8TotalTxBytes;
	uint64_t u8TotalRxBytes;
	uint64_t u8TotalRxPkts;
	uint64_t u8GetDataRateTime;
#endif
	/* When this STA_REC called qmActivateStaRec, set to TRUE. */
	u_int8_t fgIsValid;

	/* TX key is ready */
	u_int8_t fgIsTxKeyReady;

	/* When the STA is connected or TX key is ready */
	u_int8_t fgIsTxAllowed;

	/* Per-STA Queues: [0] AC0, [1] AC1, [2] AC2, [3] AC3 */
	struct QUE arTxQueue[NUM_OF_PER_STA_TX_QUEUES];

	/* Per-STA Pending Queues: [0] AC0, [1] AC1, [2] AC2, [3] AC3 */
	/* This queue is for Tx packet in protected BSS before key is set */
	struct QUE arPendingTxQueue[NUM_OF_PER_STA_TX_QUEUES];

	/* Tx packet target queue pointer. Select between arTxQueue &
	 * arPendingTxQueue
	 */
	struct QUE *aprTargetQueue[NUM_OF_PER_STA_TX_QUEUES];

#if CFG_QUEUE_RX_IF_CONN_NOT_READY
	/* When the STA is ready to indicate packet to host */
	u_int8_t fgIsRxAllowed;
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */

	/* Reorder Parameter reference table */
	struct RX_BA_ENTRY *aprRxReorderParamRefTbl[CFG_RX_MAX_BA_TID_NUM];

#if CFG_SUPPORT_802_11V_TIMING_MEASUREMENT
	struct TIMINGMSMT_PARAM rWNMTimingMsmt;
#endif
	uint8_t ucTrafficDataType;	/* 0: auto 1: data 2: video 3: voice */

	/* 0: auto 1:Force enable 2: Force disable 3: enable by peer */
	uint8_t ucTxGfMode;

	/* 0: auto 1:Force enable 2: Force disable 3: enable by peer */
	uint8_t ucTxSgiMode;

	/* 0: auto 1:Force enable 2: Force disable 3: enable by peer */
	uint8_t ucTxStbcMode;

	uint32_t u4FixedPhyRate;
	uint16_t u2MaxLinkSpeed;	/* unit is 0.5 Mbps */
	uint16_t u2MinLinkSpeed;
	uint32_t u4Flags;	/* reserved for MTK Synergies */
#if CFG_SUPPORT_RXSMM_ALLOWLIST
	u_int8_t fgRxsmmEnable;	/* AllowList for RxSMM enable */
#endif

#if CFG_SUPPORT_TDLS
	u_int8_t fgTdlsIsProhibited;	/* TRUE: AP prohibits TDLS links */

	/* TRUE: AP prohibits TDLS chan switch */
	u_int8_t fgTdlsIsChSwProhibited;

	u_int8_t flgTdlsIsInitiator;	/* TRUE: the peer is the initiator */

	/* temp to queue HT capability element */
	struct IE_HT_CAP rTdlsHtCap;

	struct PARAM_KEY rTdlsKeyTemp;	/* temp to queue the key information */
	uint8_t ucTdlsIndex;
#endif	/* CFG_SUPPORT_TDLS */
#if CFG_SUPPORT_TX_BF
	struct TXBF_PFMU_STA_INFO rTxBfPfmuStaInfo;
#endif
#if CFG_SUPPORT_MSP
	uint32_t au4RxV[RXV_NUM];
#endif
	uint8_t ucSmDialogToken;	/* Spectrum Mngt Dialog Token */
	uint8_t ucSmMsmtRequestMode;	/* Measurement Request Mode */
	uint8_t ucSmMsmtToken;		/* Measurement Request Token */

	/* Element for AMSDU */
	uint8_t ucAmsduEnBitmap;	/* Tid bit mask of AMSDU enable */
	uint8_t ucMaxMpduCount;
	uint32_t u4MaxMpduLen;
	uint32_t u4MinMpduLen;
#if CFG_SUPPORT_802_11W
	/* AP PMF */
	struct STA_PMF_CFG rPmfCfg;
	/* STA PMF */
	uint32_t u4assocComeBackTime;
#endif
#if CFG_AP_80211K_SUPPORT
	uint16_t u2BcnReqRepetition;
#endif
#if CFG_AP_80211V_SUPPORT
	struct TIMER rBTMReqDisassocTimer;
#endif /* CFG_AP_80211V_SUPPORT */

#if QOS_MAP_LEGACY_DSCP_TABLE
	uint8_t qosMapTable[64]; /* a legacy QoS Map for DSCP to TID */
#endif
	struct QOS_MAP rQosMap; /* a structure for cfg80211_classify8021d */

#if (CFG_SUPPORT_TWT == 1)
	/* TWT Requester state */
	enum _ENUM_TWT_REQUESTER_STATE_T aeTWTReqState;
	struct _TWT_FLOW_T arTWTFlow[TWT_MAX_FLOW_NUM];
#if (CFG_SUPPORT_BTWT == 1)
	struct _TWT_FLOW_T arBTWTFlow[RTWT_MAX_FLOW_NUM];
#endif
#if (CFG_SUPPORT_RTWT == 1)
	struct _TWT_FLOW_T arRTWTFlow[RTWT_MAX_FLOW_NUM];
#endif

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	u_int8_t ucTWTHospotSupport;
	u_int8_t ucTWTFlowId;
	struct _TWT_HOTSPOT_CTRL_T TWTHotspotCtrl;
	struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode;
#endif

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
	/* Get current TSF timer */
	struct TIMER rTwtGetCurrentTsfTimeoutTimer;
	/* FSM Wait Resp Timer */
	struct TIMER rTwtFsmWaitRespTimeoutTimer;
	/* FSM Teardown Timer */
	struct TIMER rTwtFsmTeardownTimeoutTimer;
#endif
#endif
	uint32_t au4Timestamp[2];
#if (CFG_SUPPORT_802_11AX == 1)
	struct HE_A_CTRL_OM_T arHeACtrlOm;
#endif

#if CFG_STAINFO_FEATURE
	u_int8_t fgSupportProxyARP;
	u_int8_t fgSupportTFS;
	u_int8_t fgSupportWNMSleep;
	u_int8_t fgSupportTIMBcast;
	u_int8_t fgSupportDMS;
	u_int8_t ucSupportedBand;
#endif
	uint32_t u4SupportedOpClassBits;
	uint16_t u2SupportedChnlBits_2g;
	uint32_t u4SupportedChnlBits_5g_0;
	uint16_t u2SupportedChnlBits_5g_1;

	/*
	 * Flag used to record the connected status of upper layer.
	 * Indicate connected status only when disconnected, and only
	 * indicate disconnected status only when connected.
	 */
	u_int8_t fgIsConnected;
#if CFG_SUPPORT_HE_ER
	u_int8_t fgIsExtendedRange;
#endif
/* fos_change begin*/
#if CFG_SUPPORT_STAT_STATISTICS
	uint32_t u4LastPhyRate;
	uint8_t ucNoise_avg;
#endif /* fos_change end*/
#if CFG_SUPPORT_NAN
	OS_SYSTIME rNanExpiredSendTime;
	unsigned char fgNanSendTimeExpired;
	atomic_t NanRefCount;
#endif

#if CFG_SUPPORT_LLS
	/* Store data in format from RXV in reduced size to serve Link Stats
	 * report format defined in STATS_LLS_WIFI_RATE
	 *
	 * preamble   :3;   0:OFDM, 1:CCK, 2:HT 3:VHT 4:HE, in separate array
	 * nss        :1;   0:1x1, 1:2x2
	 * bw         :2;   0:20MHz, 1:40Mhz, 2:80Mhz, 3:160Mhz
	 * rateMcsIdx :4;   CCK: [2, 4, 11, 22]
	 *                  OFDM:  [12, 18, 24, 36, 48, 72, 96, 108];
	 *                  HT/VHT/HE it would be mcs index
	 */
	struct {
		uint32_t u4RxMpduOFDM[1]
			[STATS_LLS_MAX_OFDM_BW_NUM][STATS_LLS_OFDM_NUM];
		uint32_t u4RxMpduCCK[1]
			[STATS_LLS_MAX_CCK_BW_NUM][STATS_LLS_CCK_NUM];
		uint32_t u4RxMpduHT[1]
			[STATS_LLS_MAX_HT_BW_NUM][STATS_LLS_HT_NUM];
		uint32_t u4RxMpduVHT[STATS_LLS_MAX_NSS_NUM]
			[STATS_LLS_MAX_VHT_BW_NUM][STATS_LLS_VHT_NUM];
		uint32_t u4RxMpduHE[STATS_LLS_MAX_NSS_NUM]
			[STATS_LLS_MAX_HE_BW_NUM][STATS_LLS_HE_NUM];
		uint32_t u4RxMpduEHT[STATS_LLS_MAX_NSS_NUM]
			[STATS_LLS_MAX_EHT_BW_NUM][STATS_LLS_EHT_NUM];
	};
#endif
#if CFG_SUPPORT_STA_INFO
	uint32_t u4RxBmcCnt;
	uint32_t u4RxRetryCnt;
#endif
#if (CFG_WIFI_GET_MCS_INFO == 1)
	uint32_t au4RxV0[MCS_INFO_SAMPLE_CNT];
	uint32_t au4RxV1[MCS_INFO_SAMPLE_CNT];
	uint32_t au4RxV2[MCS_INFO_SAMPLE_CNT];
#endif

#if CFG_SUPPORT_MLR
	/* Peer MLR capability */
	uint8_t ucMlrSupportBitmap;
	/* Peer MLR status */
	uint8_t ucMlrMode;
	uint8_t ucMlrState;
	u_int8_t fgEnableTxFrag;
#endif
	u_int8_t fgIsMscsSupported;
	struct LINK rMscsMonitorList;
	struct LINK rMscsTcpMonitorList;
	u_int8_t ucGcCsaSupported;
	u_int8_t fgIsEapEncrypt;

	u_int8_t fgEcsaCapable;

	u_int8_t fgIsSupportCsa;
};

#if (CFG_SUPPORT_802_11BE_MLO == 1)
struct MLD_STA_RECORD {
	struct LINK_ENTRY rLinkEntry;
	uint8_t fgIsInUse;
	uint8_t ucIdx;
	uint8_t ucGroupMldId; /* id from mld bss */
	uint8_t aucPeerMldAddr[MAC_ADDR_LEN];
	uint16_t u2PrimaryMldId;
	uint16_t u2SecondMldId;
	uint16_t u2SetupWlanId;
	uint8_t fgEPCS;
	uint8_t fgMldType;
	uint8_t aucStrBitmap[UNI_MLD_LINK_MAX];
	uint16_t u2EmlCap;
	uint16_t u2MldCap;
	uint8_t ucEmlEnabled;
	uint8_t ucMaxSimuLinks;
	struct LINK rStarecList;
	uint64_t aucRxPktCnt[ENUM_BAND_NUM];
	uint32_t u4StaBitmap;
	uint32_t u4ActiveStaBitmap;
#if (CFG_SINGLE_BAND_MLSR_56 == 1)
	uint8_t fgIsSbMlsr; /* single band MLSR 5+6 */
#endif /* CFG_SINGLE_BAND_MLSR_56 */
	uint16_t u2ValidLinks; /* bitmap of valid MLO link IDs */
	struct TIMER rEpcsTimer;
#if (CFG_SUPPORT_802_11BE_T2LM == 1)
	enum ENUM_T2LM_STATE eT2LMState;
	struct TIMER rT2LMTimer;
	struct T2LM_INFO rT2LMParams;
#endif
};
#endif

enum ENUM_STA_REC_CMD_ACTION {
	STA_REC_CMD_ACTION_STA = 0,
	STA_REC_CMD_ACTION_BSS = 1,
	STA_REC_CMD_ACTION_BSS_EXCLUDE_STA = 2
};

#if CFG_SUPPORT_TDLS

/* TDLS FSM */
struct CMD_PEER_ADD {

	uint8_t aucPeerMac[6];
	enum ENUM_STA_TYPE eStaType;
	uint8_t ucBssIdx;
};

struct CMD_PEER_UPDATE_HT_CAP_MCS_INFO {
	uint8_t arRxMask[SUP_MCS_RX_BITMASK_OCTET_NUM];
	uint16_t u2RxHighest;
	uint8_t ucTxParams;
	uint8_t Reserved[3];
};

struct CMD_PEER_UPDATE_VHT_CAP_MCS_INFO {
	uint16_t u2RxMcsMap;
	uint16_t u2RxHighest;
	uint16_t u2TxMcsMap;
	uint16_t u2TxHighest;
};

struct CMD_PEER_UPDATE_HT_CAP {
	uint16_t u2CapInfo;
	uint8_t ucAmpduParamsInfo;

	/* 16 bytes MCS information */
	struct CMD_PEER_UPDATE_HT_CAP_MCS_INFO rMCS;

	uint16_t u2ExtHtCapInfo;
	uint32_t u4TxBfCapInfo;
	uint8_t ucAntennaSelInfo;
};

struct CMD_PEER_UPDATE_VHT_CAP {
	uint32_t u4CapInfo;
	/* 16 bytes MCS information */
	struct CMD_PEER_UPDATE_VHT_CAP_MCS_INFO rVMCS;

};

struct CMD_PEER_UPDATE_HE_CAP {
#if CFG_SUPPORT_TDLS_11AX
	uint8_t ucHeMacCapInfo[HE_MAC_CAP_BYTE_NUM];
	uint8_t ucHePhyCapInfo[HE_PHY_CAP_BYTE_NUM];
	uint16_t u2HeRxMcsMapBW80;
	uint16_t u2HeRxMcsMapBW80Assoc;
	uint16_t u2HeTxMcsMapBW80;
	uint16_t u2HeRxMcsMapBW160;
	uint16_t u2HeRxMcsMapBW160Assoc;
	uint16_t u2HeTxMcsMapBW160;
	uint16_t u2HeRxMcsMapBW80P80;
	uint16_t u2HeRxMcsMapBW80P80Assoc;
	uint16_t u2HeTxMcsMapBW80P80;
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint16_t u2He6gBandCapInfo;
#endif
	uint8_t ucReserved;
};

struct CMD_PEER_UPDATE_EHT_CAP {
#if CFG_SUPPORT_TDLS_11BE
	uint8_t ucEhtMacCapInfo[EHT_MAC_CAP_BYTE_NUM];
	uint8_t ucEhtPhyCapInfo[EHT_PHY_CAP_BYTE_NUM];
	uint8_t ucEhtPhyCapInfoExt[EHT_PHY_CAP_BYTE_NUM];
	uint8_t aucMcsMap20MHzSta[4];
	uint8_t aucMcsMap80MHz[3];
	uint8_t aucMcsMap160MHz[3];
	uint8_t aucMcsMap320MHz[3];
#endif
	uint8_t ucReserved;
};

struct CMD_PEER_UPDATE {

	uint8_t aucPeerMac[6];

#define CMD_PEER_UPDATE_SUP_CHAN_MAX			50
	uint8_t aucSupChan[CMD_PEER_UPDATE_SUP_CHAN_MAX];

	uint16_t u2StatusCode;

#define CMD_PEER_UPDATE_SUP_RATE_MAX			50
	uint8_t aucSupRate[CMD_PEER_UPDATE_SUP_RATE_MAX];
	uint16_t u2SupRateLen;

	uint8_t UapsdBitmap;
	uint8_t UapsdMaxSp;	/* MAX_SP */

	uint16_t u2Capability;
#define CMD_PEER_UPDATE_EXT_CAP_MAXLEN			5
	uint8_t aucExtCap[CMD_PEER_UPDATE_EXT_CAP_MAXLEN];
	uint16_t u2ExtCapLen;

	struct CMD_PEER_UPDATE_HT_CAP rHtCap;
	struct CMD_PEER_UPDATE_VHT_CAP rVHtCap;
	struct CMD_PEER_UPDATE_HE_CAP rHeCap;
	struct CMD_PEER_UPDATE_EHT_CAP rEhtCap;
	u_int8_t fgIsSupHt;
	u_int8_t fgIsSupVht;
	u_int8_t fgIsSupHe;
	u_int8_t fgIsSupEht;
	enum ENUM_STA_TYPE eStaType;
	uint8_t ucBssIdx;

	/* TODO */
	/* So far, TDLS only a few of the parameters, the rest will be added
	 * in the future requiements
	 */
	/* kernel 3.10 station paramenters */
#if 0
	struct station_parameters {
	   const u8 *supported_rates;
	   struct net_device *vlan;
	   u32 sta_flags_mask, sta_flags_set;
	   u32 sta_modify_mask;
	   int listen_interval;
	   u16 aid;
	   u8 supported_rates_len;
	   u8 plink_action;
	   u8 plink_state;
	   const struct ieee80211_ht_cap *ht_capa;
	   const struct ieee80211_vht_cap *vht_capa;
	   u8 uapsd_queues;
	   u8 max_sp;
	   enum nl80211_mesh_power_mode local_pm;
	   u16 capability;
	   const u8 *ext_capab;
	   u8 ext_capab_len;
	   const u8 *supported_channels;
	   u8 supported_channels_len;
	   const u8 *supported_oper_classes;
	   u8 supported_oper_classes_len;
	   };
#endif

};

#endif

#if CFG_DBG_MGT_BUF
/**
 * struct MEM_TRACK - A structure for tracking dynamic allocated memory
 *
 * DO NOT attempt to reorder the structure to eliminate the slop, which might
 * break the byte alignment assumption of aucData.
 *
 * @rLinkEntry: link entry linked all the allocated memory
 * @ucCmdId: Command ID
 * @ucWhere: log the processed location of the memory block
 *  0x10: the CmdId enqueue to rCmdQueue and is waiting for main_thread handling
 *  0x11: the CmdId drop in driver
 *  0x12: the CmdId drop in driver
 *  0x13: the CmdId queue back to rCmdQueue
 *  0x14: the CmdId can't enqueue to TxCmdQueue due to card removal
 *  0x15: the CmdId can't enqueue to TxCmdQueue due to out of resource
 *  0x20: the CmdId is in TxCmdQueue and is waiting for main_thread handling
 *  0x30: the CmdId needs to send to FW via HIF
 *  0x40: the CmdId enqueues to TxCmdDone queue
 *  0x50: the CmdId is sent to WFDMA by HIF
 *  0x60: the CmdId is in PendingCmdQuene and already report to module
 * @pucFileAndLine: the caller information of the memory block
 * @aucData: returned memory pointer to the caller
 */
struct MEM_TRACK {
	struct LINK_ENTRY rLinkEntry;
	uint8_t ucCmdId;
	uint8_t ucWhere; /* followed by slop, but it doesn't matter */
	uint8_t *pucFileAndLine; /* A pointer here forces aucData aligned */
	uint8_t aucData[];
};
#endif
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
#define STRL(x) #x
#define STRLINE(x) STRL(x)

#if CFG_DBG_MGT_BUF
#define cnmMgtPktAlloc(_prAdapter, _u4Length) \
	cnmPktAllocWrapper((_prAdapter), (_u4Length), (uint8_t *)__func__)

#define cnmMgtPktFree(_prAdapter, _prMsduInfo) \
	cnmPktFreeWrapper((_prAdapter), (_prMsduInfo), (uint8_t *)__func__)

#define cnmMemAlloc(_prAdapter, eRameType, u4Length) \
	cnmMemAllocX(_prAdapter, eRameType, u4Length, \
		__FILE__ ":" STRLINE(__LINE__))

#define IS_FROM_BUF(_prAdapter, pucInfoBuffer) \
	(((uint8_t *)(pucInfoBuffer) >= \
		(uint8_t *)_prAdapter->rMgtBufInfo.pucBuf) && \
	((uint8_t *)(pucInfoBuffer) < \
		(uint8_t *)_prAdapter->rMgtBufInfo.pucBuf + MGT_BUFFER_SIZE))

#define cnmPktAlloc(_prAdapter, u4Length) \
	cnmPktAllocX(_prAdapter, u4Length, \
		__FILE__ ":" STRLINE(__LINE__))
#else
#define cnmMgtPktAlloc cnmPktAlloc
#define cnmMgtPktFree cnmPktFree
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

struct MSDU_INFO *cnmPktAllocWrapper(struct ADAPTER *prAdapter,
	uint32_t u4Length, uint8_t *pucStr);

void cnmPktFreeWrapper(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, uint8_t *pucStr);

#if CFG_DBG_MGT_BUF
struct MSDU_INFO *cnmPktAllocX(struct ADAPTER *prAdapter,
	uint32_t u4Length, uint8_t *fileAndLine);
#else
struct MSDU_INFO *cnmPktAlloc(struct ADAPTER *prAdapter,
	uint32_t u4Length);
#endif
void cnmPktFree(struct ADAPTER *prAdapter, struct MSDU_INFO *prMsduInfo);

void cnmMemInit(struct ADAPTER *prAdapter);

#if CFG_DBG_MGT_BUF
void *cnmMemAllocX(struct ADAPTER *prAdapter,
	enum ENUM_RAM_TYPE eRamType, uint32_t u4Length,
	uint8_t *fileAndLine);
#else
void *cnmMemAlloc(struct ADAPTER *prAdapter, enum ENUM_RAM_TYPE eRamType,
	uint32_t u4Length);
#endif

void cnmMemFree(struct ADAPTER *prAdapter, void *pvMemory);

void cnmStaRecInit(struct ADAPTER *prAdapter);

struct STA_RECORD *
cnmStaRecAlloc(struct ADAPTER *prAdapter, enum ENUM_STA_TYPE eStaType,
	uint8_t ucBssIndex, uint8_t *pucMacAddr);

void cnmStaRecFree(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

void cnmStaFreeAllStaByNetwork(struct ADAPTER *prAdapter, uint8_t ucBssIndex,
	uint8_t ucStaRecIndexExcluded);

struct STA_RECORD *cnmGetStaRecByIndex(struct ADAPTER *prAdapter,
	uint8_t ucIndex);

struct STA_RECORD *cnmGetStaRecByWlanIndex(struct ADAPTER *prAdapter,
	uint8_t ucWlanIndex);

struct STA_RECORD *cnmGetStaRecByIndexWithoutInUseCheck(
	struct ADAPTER *prAdapter,
	uint8_t ucIndex);

struct STA_RECORD *cnmGetStaRecByAddress(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, const uint8_t aucPeerMACAddress[]);

void cnmStaRecChangeState(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, uint8_t ucNewState);

uint8_t *cnmStaRecAuthAddr(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

int cnmShowBssInfo(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo,
	char *pcCommand, int i4TotalLen);
int cnmShowStaRec(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	char *pcCommand, int i4TotalLen);

void cnmDumpBssInfo(struct ADAPTER *prAdapter, uint8_t ucBssIdx);
void cnmDumpStaRec(struct ADAPTER *prAdapter, uint8_t ucStaRecIdx);

uint32_t cnmDumpMemoryStatus(struct ADAPTER *prAdapter, uint8_t *pucBuf,
	uint32_t u4Max);

#if CFG_SUPPORT_TDLS
uint32_t			/* TDLS_STATUS */
cnmPeerAdd(struct ADAPTER *prAdapter, void *pvSetBuffer,
	uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen);

uint32_t			/* TDLS_STATUS */
cnmPeerUpdate(struct ADAPTER *prAdapter, void *pvSetBuffer,
	uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen);

struct STA_RECORD *cnmGetTdlsPeerByAddress(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t aucPeerMACAddress[]);
#endif

#if CFG_SUPPORT_TX_BF
void cnmStaSendUpdateCmd(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	 struct TXBF_PFMU_STA_INFO *prTxBfPfmuStaInfo, u_int8_t fgNeedResp);
#else
void cnmStaSendUpdateCmd(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	 void *prTxBfPfmuStaInfo, u_int8_t fgNeedResp);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#ifndef _lint
/* Kevin: we don't have to call following function to inspect the data
 * structure. It will check automatically while at compile time.
 * We'll need this for porting driver to different RTOS.
 */
static __KAL_INLINE__ void cnmMemDataTypeCheck(void)
{
#if 0
	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, rLinkEntry)
			== 0);

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, rLinkEntry)
			== OFFSET_OF(struct SW_RFB, rLinkEntry));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, pucBuffer)
			== OFFSET_OF(struct SW_RFB, pucBuffer));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, ucBufferSource)
			== OFFSET_OF(struct SW_RFB, ucBufferSource));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, pucMacHeader)
			== OFFSET_OF(struct SW_RFB, pucMacHeader));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, ucMacHeaderLength)
			== OFFSET_OF(struct SW_RFB, ucMacHeaderLength));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, pucPayload)
			== OFFSET_OF(struct SW_RFB, pucPayload));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, u2PayloadLength)
			 == OFFSET_OF(struct SW_RFB, u2PayloadLength));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, prStaRec)
			== OFFSET_OF(struct SW_RFB, prStaRec));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, ucNetworkTypeIndex)
			== OFFSET_OF(struct SW_RFB, ucNetworkTypeIndex));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, ucTID)
			== OFFSET_OF(struct SW_RFB, ucTID));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, fgIs802_11Frame)
			== OFFSET_OF(struct SW_RFB, fgIs802_11Frame));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, ucControlFlag)
			== OFFSET_OF(struct SW_RFB, ucControlFlag));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, rArrivalTime)
			== OFFSET_OF(struct SW_RFB, rArrivalTime));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, ucTC)
			== OFFSET_OF(struct SW_RFB, ucTC));

#if CFG_PROFILE_BUFFER_TRACING
	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, eActivity[0])
			== OFFSET_OF(struct SW_RFB, eActivity[0]));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, rActivityTime[0])
			== OFFSET_OF(struct SW_RFB, rActivityTime[0]));
#endif

#if DBG && CFG_BUFFER_FREE_CHK
	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSDU_INFO, fgBufferInSource)
			== OFFSET_OF(struct SW_RFB, fgBufferInSource));
#endif

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct STA_RECORD, rLinkEntry)
			== 0);

	return;
#endif
}
#endif /* _lint */

#endif /* _CNM_MEM_H */
