/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "cnm.h"
 *    \brief
 */


#ifndef _CNM_H
#define _CNM_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "nic_cmd_event.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/** @def DBDC_5G_WMM_INDEX
 * Define constant for DBDC 5G WMM index
 */
#define DBDC_5G_WMM_INDEX	0

/** @def DBDC_2G_WMM_INDEX
 * Define constant for DBDC 2G WMM index
 */
#define DBDC_2G_WMM_INDEX	1

/** @def HW_WMM_NUM
 * Define macro to access hardware WMM number from adapter structure
 */
#define HW_WMM_NUM		(prAdapter->ucWmmSetNum)

/** @def MAX_HW_WMM_INDEX
 * Define constant for maximum hardware WMM index
 */
#define MAX_HW_WMM_INDEX	(HW_WMM_NUM - 1)

/** @def DEFAULT_HW_WMM_INDEX
 * Define constant for default hardware WMM index
 */
#define DEFAULT_HW_WMM_INDEX	MAX_HW_WMM_INDEX

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

#if (CFG_SUPPORT_IDC_CH_SWITCH == 1)
/** @struct ENUM_CH_SWITCH_TYPE
 *  (IDC) Enum representing different channel switch types
 */
enum ENUM_CH_SWITCH_TYPE {
	/** Represents 2G channel switch type */
	CH_SWITCH_2G, /* Default */

	/** Represents 5G channel switch type */
	CH_SWITCH_5G,

	/** Represents 6G channel switch type */
	CH_SWITCH_6G,

	/** Number of channel switch types */
	CH_SWITCH_NUM
};
#endif

/** @struct MSG_CH_REQ
 *  (MSG) This struct represents a message for channel request.
 */
struct MSG_CH_REQ {
	/** Header structure for the message */
	struct MSG_HDR rMsgHdr;	/* Must be the first member */

	/** Index of the BSS that requested this channel */
	uint8_t ucBssIndex;

	/** Token ID of the channel request */
	uint8_t ucTokenID;

	/** Primary channel number */
	uint8_t ucPrimaryChannel;

	/** Enum for RF Scheduling Coefficient */
	enum ENUM_CHNL_EXT eRfSco;

	/** Enum for RF Band */
	enum ENUM_BAND eRfBand;

	/** Enum for RF Channel Width */
	/* To support 80/160MHz bandwidth */
	enum ENUM_CHANNEL_WIDTH eRfChannelWidth;

	/** Center frequency segment 1 for 80/160MHz bandwidth */
	uint8_t ucRfCenterFreqSeg1;	/* To support 80/160MHz bandwidth */

	/** Center frequency segment 2 for 80/160MHz bandwidth */
	uint8_t ucRfCenterFreqSeg2;	/* To support 80/160MHz bandwidth */

	/** Enum for RF Channel Width from AP*/
	enum ENUM_CHANNEL_WIDTH eRfChannelWidthFromAP;

	/** Center frequency segment 1 from AP for 80/160MHz bandwidth */
	uint8_t ucRfCenterFreqSeg1FromAP;

	/** Center frequency segment 2 from AP for 80/160MHz bandwidth */
	uint8_t ucRfCenterFreqSeg2FromAP;

	/** Enum for Channel Request Type */
	enum ENUM_CH_REQ_TYPE eReqType;

	/** Maximum interval in milliseconds */
	uint32_t u4MaxInterval;	/* In unit of ms */

	/** Enum for Dual Band Dual Concurrent Band */
	enum ENUM_MBMC_BN eDBDCBand;

	/** Number of extra channel requests */
	uint8_t ucExtraChReqNum;

	/** Flexible array member for variable-length buffer */
	uint8_t aucBuffer[];
};

/** @struct MSG_CH_ABORT
 *  (MSG) This struct represents a message for channel abort.
 */
struct MSG_CH_ABORT {
	/** Header of the message, must be the first member */
	struct MSG_HDR rMsgHdr;	/* Must be the first member */

	/** Identifier for the BSS */
	uint8_t ucBssIndex;

	/** Token ID for the message */
	uint8_t ucTokenID;

	/** Enumeration representing the DBDC band */
	enum ENUM_MBMC_BN eDBDCBand;

	/** Number of extra channel requests */
	uint8_t ucExtraChReqNum;
};

/** @struct MSG_CH_GRANT
 *  (MSG) This struct represents a message for channel grant.
 */
struct MSG_CH_GRANT {
	/** Header structure for the message */
	struct MSG_HDR rMsgHdr;	/* Must be the first member */

	/** Index of the BSS */
	uint8_t ucBssIndex;

	/** Token ID of the granted channel */
	uint8_t ucTokenID;

	/** Channel number of the granted channel */
	uint8_t ucPrimaryChannel;

	/** Enum for RF Scheduling Coefficient  */
	enum ENUM_CHNL_EXT eRfSco;

	/** Enum for RF Band */
	enum ENUM_BAND eRfBand;

	/** Enum for RF Channel Width */
    /* To support 80/160MHz bandwidth */
	enum ENUM_CHANNEL_WIDTH eRfChannelWidth;

	/** RF Center Frequency Segment 1 */
	uint8_t ucRfCenterFreqSeg1;	/* To support 80/160MHz bandwidth */

	/** RF Center Frequency Segment 2 */
	uint8_t ucRfCenterFreqSeg2;	/* To support 80/160MHz bandwidth */

	/** The type of request */
	enum ENUM_CH_REQ_TYPE eReqType;

	/** Bandwidth of the channel */
	uint32_t u4GrantInterval;	/* In unit of ms */

	/** Enum for Dual Band Dual Concurrent Band */
	enum ENUM_MBMC_BN eDBDCBand;
};

/** @struct MSG_CH_REOCVER
 *  (MSG) This structure to represent message receiver information
 */
struct MSG_CH_REOCVER {
	/** Header for the message, must be the first member */
	struct MSG_HDR rMsgHdr;	/* Must be the first member */

	/** BSS index of the message receiver */
	uint8_t ucBssIndex;

	/** Token ID of the message receiver */
	uint8_t ucTokenID;

	/** Primary channel used */
	uint8_t ucPrimaryChannel;

	/** RF SCO type */
	enum ENUM_CHNL_EXT eRfSco;

	/** RF band type */
	enum ENUM_BAND eRfBand;

	/** RF channel width (80/160MHz) */
    /* To support 80/160MHz bandwidth */
	enum ENUM_CHANNEL_WIDTH eRfChannelWidth;

	/** Center frequency for segment 1 */
	uint8_t ucRfCenterFreqSeg1;	/* To support 80/160MHz bandwidth */

	/** Center frequency for segment 2 */
	uint8_t ucRfCenterFreqSeg2;	/* To support 80/160MHz bandwidth */

	/** Channel request type */
	enum ENUM_CH_REQ_TYPE eReqType;
};

/** @struct CNM_INFO
 * @brief Definition of structure CNM_INFO
 */
struct CNM_INFO {
	/** Flag indicating if channel is granted */
	u_int8_t fgChGranted;

	/** Index of the BSS */
	uint8_t ucBssIndex;

	/** Band of the channel */
	uint8_t ucTokenID;
};

#if CFG_ENABLE_WIFI_DIRECT
/* Moved from p2p_fsm.h */
/** @struct DEVICE_TYPE
 * @brief Define a struct named DEVICE_TYPE with packed attribute at the front
 */
__KAL_ATTRIB_PACKED_FRONT__
struct DEVICE_TYPE {
	/** 2-byte unsigned integer to represent the category ID */
	uint16_t u2CategoryId;

	/** Array of 4 bytes to store the Organizationally Unique Identifier */
	uint8_t aucOui[4];

	/** 2-byte unsigned integer to represent the subcategory ID */
	uint16_t u2SubCategoryId;
} __KAL_ATTRIB_PACKED__;
#endif

/** @enum ENUM_CNM_DBDC_MODE
 *  (DBDC) Enum representing different CNM DBDC modes
 */
enum ENUM_CNM_DBDC_MODE {
	/** A or G traffic separate by WMM,
	 *  but both TRX on band 0, CANNOT enable DBDC
	 */
	ENUM_DBDC_MODE_DISABLED,

	/** A/G traffic separate by WMM,
	 *  WMM0 or WMM1 TRX on band 0 or band 1, CANNOT disable DBDC
	 */
	ENUM_DBDC_MODE_STATIC,

	/** Automatically enable or disable DBDC,
	 *  setting just like static or disable mode
	 */
	ENUM_DBDC_MODE_DYNAMIC,

	/** Total number of CNM DBDC modes */
	ENUM_DBDC_MODE_NUM
};

#if CFG_SUPPORT_DBDC
/** @enum ENUM_CNM_DBDC_SWITCH_MECHANISM
 *  (DBDC) Enum representing different CNM DBDC switch mechanisms
 *  When DBDC available in dynamic DBDC
 */
enum ENUM_CNM_DBDC_SWITCH_MECHANISM {
	/** Switch to DBDC when available (less latency) */
	ENUM_DBDC_SWITCH_MECHANISM_LATENCY_MODE,

	/** Switch to DBDC when DBDC T-put > MCC T-put */
	ENUM_DBDC_SWITCH_MECHANISM_THROUGHPUT_MODE,

	/** Total number of CNM DBDC switch mechanisms */
	ENUM_DBDC_SWITCH_MECHANISM_NUM
};
#endif	/* CFG_SUPPORT_DBDC */

/** @enum ENUM_CNM_NETWORK_TYPE_T
 *  (CNM) Enum representing different CNM network types
 */
enum ENUM_CNM_NETWORK_TYPE_T {
	/** Represents an unspecified network type */
	ENUM_CNM_NETWORK_TYPE_OTHER,

	/** Represents an AIS (Ad-hoc Infrastructure Search) network type */
	ENUM_CNM_NETWORK_TYPE_AIS,

	/** Represents a P2P (Peer-to-Peer) Group Client network type */
	ENUM_CNM_NETWORK_TYPE_P2P_GC,

	/** Represents a P2P (Peer-to-Peer) Group Owner network type */
	ENUM_CNM_NETWORK_TYPE_P2P_GO,

	/** Represents a NAN (Neighborhood Awareness Networking) network type */
	ENUM_CNM_NETWORK_TYPE_NAN,

	/** Represents the total number of CNM network types */
	ENUM_CNM_NETWORK_TYPE_NUM
};

/** @enum ENUM_CNM_OPMODE_REQ_T
 *  (CNM) Enum representing different CNM operation mode
 *  requests for priority order
 */
enum ENUM_CNM_OPMODE_REQ_T {
	CNM_OPMODE_REQ_START      = 0,

	/** Antenna control request */
	CNM_OPMODE_REQ_ANT_CTRL   = 0,

	/** Hardware constraint capability request */
	CNM_OPMODE_REQ_HW_CONSTRIAN_CAP   = 1,

	/** DBDC (Dual Band Dual Concurrent) decision request */
	CNM_OPMODE_REQ_DBDC       = 2,

	/** DBDC scan request */
	CNM_OPMODE_REQ_DBDC_SCAN  = 3,

	/** Coexistence decision request */
	CNM_OPMODE_REQ_COEX       = 4,

	/** Tx antenna control */
	CNM_OPMODE_REQ_TX_ANT_CTRL = 5,

	/** Smartgear decision request */
	CNM_OPMODE_REQ_SMARTGEAR  = 6,

	/** User configuration request */
	CNM_OPMODE_REQ_USER_CONFIG     = 7,

	/** Smartgear 1T2R decision request */
	CNM_OPMODE_REQ_SMARTGEAR_1T2R  = 8,

	/** Antenna control 1T2R request */
	CNM_OPMODE_REQ_ANT_CTRL_1T2R   = 9,

	/** CO-Ant request */
	CNM_OPMODE_REQ_COANT      = 10,

	/** RDD (Radar Detection and DFS) operation change request */
	CNM_OPMODE_REQ_RDD_OPCHNG = 11,

	/** User configuration request */
	CNM_OPMODE_REQ_USER_CONFIG_BW	  = 12,

	/** Total number of CNM operation mode requests */
	CNM_OPMODE_REQ_NUM        = 13,

	/** Maximum capability of opmode request (just for coding) */
	CNM_OPMODE_REQ_MAX_CAP    = 14


};

struct CNM_CHIP_CAP_INFO {
	uint8_t aucMaxBw2g[CONFIG_BAND_NUM];
	uint8_t aucMaxBw5g[CONFIG_BAND_NUM];
	uint8_t aucMaxBw6g[CONFIG_BAND_NUM];
	/* indicate which hw bn can be allocate by rf band,
	 * Bit(0): Bn0, Bit(1): Bn1, Bit(2): Bn2.
	 */
	uint8_t ucValidHwBn2g;
	uint8_t ucValidHwBn5g;
	uint8_t ucValidHwBn6g;
	uint8_t aucMaxNss[CONFIG_BAND_NUM];
	uint8_t aucMaxMcs[CONFIG_BAND_NUM];
};

struct CNM_OPMODE_BSS_REQ {
	bool fgEnable;
	bool fgNewRequest;
	uint8_t ucOpRxNss;
	uint8_t ucOpTxNss;
	uint8_t ucBandWidth; /* ENUM_MAX_BANDWIDTH_SETTING */
};

struct CNM_OPMODE_BSS_RUNNING_REQ {
	/* Initiator */
	enum ENUM_CNM_OPMODE_REQ_T eReqIdx;
	/* Highest prioirty req */
	enum ENUM_CNM_OPMODE_REQ_T eRunReq;
	bool fgIsRunning;
	uint8_t ucOpRxNss;
	uint8_t ucOpTxNss;
	uint8_t ucBandWidth; /* ENUM_MAX_BANDWIDTH_SETTING */
};

struct CNM_OPMODE_BSS_CONTROL_T {
	struct CNM_OPMODE_BSS_RUNNING_REQ
		rRunning;
	struct CNM_OPMODE_BSS_REQ
		arReqPool[CNM_OPMODE_REQ_NUM];
};

enum ENUM_CNM_WMM_QUOTA_REQ_T {
	CNM_WMM_REQ_DBDC    = 0,
	CNM_WMM_REQ_NUM     = 1,
	CNM_WMM_REQ_DEFAULT = 2 /* just for coding */
};

struct CNM_WMM_QUOTA_REQ {
	bool fgEnable;
	uint32_t u4ReqQuota;
};

struct CNM_WMM_QUOTA_RUNNING_REQ {
	/* Initiator */
	enum ENUM_CNM_WMM_QUOTA_REQ_T eReqIdx;
	/* Highest prioirty req */
	enum ENUM_CNM_WMM_QUOTA_REQ_T eRunReq;
	bool fgIsRunning;
	uint32_t u4ReqQuota;
};

struct CNM_WMM_QUOTA_CONTROL_T {
	struct CNM_WMM_QUOTA_RUNNING_REQ
		rRunning;
	struct CNM_WMM_QUOTA_REQ
		arReqPool[CNM_WMM_REQ_NUM];
	struct TIMER rTimer;
};

#if (CFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL == 1)
enum ENUM_SHR_ANT_GRANT {
	SHR_ANT_GRANT_TO_WIFI = 0x0,
	SHR_ANT_GRANT_TO_BT = 0x1,
};

enum ENUM_DBDC_SETTING_REASON {
	DBDC_SETTING_REASON_NULL,
	DBDC_SETTING_REASON_SWCH_SHR_ANT_TO_BT_DIRECTLY,
	DBDC_SETTING_REASON_SWCH_SHR_ANT_TO_BT_AFTER_DBDC,
	DBDC_SETTING_REASON_NUM
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
/* True if our TxNss > 1 && peer support 2ss rate && peer no Rx limit. */
#if (CFG_SUPPORT_WIFI_6G == 1)
/** @def IS_CONNECTION_NSS2
 * Macro to check if the connection meets specific requirements for NSS2.
 * Conditions:
 * 1. BSSInfo's operating transmit spatial streams (ucOpTxNss) > 1,
 *    StaRec's 2nd index of RX MCS Bitmask != 0x00,
 *    HT capability info contains SM power save bit.
 *
 * OR
 * 2. BSSInfo's operating transmit spatial streams (ucOpTxNss) > 1,
 *    Bits 2 and 3 of VHT RX MCS Map are not [0, 1],
 *    RX NSS value in VHT operation mode > 0.
 *
 * OR
 * 3. BSSInfo's operating transmit spatial streams (ucOpTxNss) > 1,
 *    Band is 6GHz,
 *    Bits 2 and 3 of HE RX MCS Map for 80MHz bandwidth are not [0, 1],
 *    HE 6GHz capabilities info contains SM power save bit.
 */
#define IS_CONNECTION_NSS2(prBssInfo, prStaRec) \
	((((prBssInfo)->ucOpTxNss > 1) && \
	((prStaRec)->aucRxMcsBitmask[1] != 0x00) \
	&& (((prStaRec)->u2HtCapInfo & HT_CAP_INFO_SM_POWER_SAVE) != 0)) || \
	(((prBssInfo)->ucOpTxNss > 1) && ((((prStaRec)->u2VhtRxMcsMap \
	& BITS(2, 3)) >> 2) != BITS(0, 1)) && ((((prStaRec)->ucVhtOpMode \
	& VHT_OP_MODE_RX_NSS) >> VHT_OP_MODE_RX_NSS_OFFSET) > 0)) || \
	(((prBssInfo)->ucOpTxNss > 1) \
	&& ((prBssInfo)->eBand == BAND_6G) \
	&& ((((prStaRec)->u2HeRxMcsMapBW80 & BITS(2, 3)) >> 2) != BITS(0, 1)) \
	&& (((prStaRec)->u2He6gBandCapInfo \
	& HE_6G_CAP_INFO_SM_POWER_SAVE) != 0)))

#else
/** @def IS_CONNECTION_NSS2
 * Macro to check if the connection meets specific requirements for NSS2.
 * Conditions:
 * 1. BSSInfo's operating transmit spatial streams (ucOpTxNss) > 1,
 *    StaRec's 2nd index of RX MCS Bitmask != 0x00,
 *    HT capability info contains SM power save bit.
 *
 * OR
 * 2. BSSInfo's operating transmit spatial streams (ucOpTxNss) > 1,
 *    Bits 2 and 3 of VHT RX MCS Map are not [0, 1],
 *    RX NSS value in VHT operation mode > 0.
 */
#define IS_CONNECTION_NSS2(prBssInfo, prStaRec) \
	((((prBssInfo)->ucOpTxNss > 1) && \
	((prStaRec)->aucRxMcsBitmask[1] != 0x00) \
	&& (((prStaRec)->u2HtCapInfo & HT_CAP_INFO_SM_POWER_SAVE) != 0)) || \
	(((prBssInfo)->ucOpTxNss > 1) && ((((prStaRec)->u2VhtRxMcsMap \
	& BITS(2, 3)) >> 2) != BITS(0, 1)) && ((((prStaRec)->ucVhtOpMode \
	& VHT_OP_MODE_RX_NSS) >> VHT_OP_MODE_RX_NSS_OFFSET) > 0)))
#endif

/** @def CNM_DBDC_ADD_DECISION_INFO
 * This macro is used to add decision information to the CNM database.
 * It takes in several parameters and assigns them to the specified fields
 * in the rDeciInfo structure.
 * Parameters:
 *   - rDeciInfo: the decision information structure
 *     to which the data will be added
 *   - _ucBId: the BSS index value to be assigned
 *   - _eBand: the RF band value to be assigned
 *   - _ucCh: the primary channel value to be assigned
 *   - _ucWmmQId: the WMM queue index value to be assigned
 */
#define CNM_DBDC_ADD_DECISION_INFO(rDeciInfo, \
		_ucBId, _eBand, _ucCh, _ucWmmQId) {\
	rDeciInfo.dbdcElem[rDeciInfo.ucLinkNum].ucBssIndex = _ucBId;\
	rDeciInfo.dbdcElem[rDeciInfo.ucLinkNum].eRfBand = _eBand;\
	rDeciInfo.dbdcElem[rDeciInfo.ucLinkNum].ucPrimaryChannel = _ucCh;\
	rDeciInfo.dbdcElem[rDeciInfo.ucLinkNum].ucWmmQueIndex = _ucWmmQId;\
	rDeciInfo.ucLinkNum++;\
}

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to initialize CNM functionality
 *
 * @param[in] prAdapter
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmInit(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to uninitialized CNM functionality
 *
 * @param[in] prAdapter
 *
 * @return void
 */
void cnmUninit(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Before handle the message from other module, it need to obtain
 *        the Channel privilege from Channel Manager
 *
 * @param[in] prAdapter
 * @param[in] prMsgHdr   The message need to be handled.
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmChMngrRequestPrivilege(struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Before deliver the message to other module, it need to release
 *        the Channel privilege to Channel Manager.
 *
 * @param[in] prAdapter
 * @param[in] prMsgHdr   The message need to be delivered
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmChMngrAbortPrivilege(struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Event handler for @ref EVENT_ID_CH_PRIVILEGE
 *
 * @param[in] prAdapter
 * @param[in] prEvent
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmChMngrHandleChEvent(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);

#if (CFG_SUPPORT_DFS_MASTER == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief Event handler for @ref EVENT_ID_RDD_REPORT(Strong correlated with P2P)
 *
 *
 * @param[in] prAdapter
 * @param[in] prEvent
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmRadarDetectEvent(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);
#endif

#if (CFG_SUPPORT_IDC_CH_SWITCH == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief Used to do CSA request
 *
 * @param[in] prAdapter
 * @param[in] eBand
 * @param[in] ucCh
 * @param[in] ucRoleIdx
 *
 * @return 0: success, -1: fail
 */
/*----------------------------------------------------------------------------*/
uint8_t cnmIdcCsaReq(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ch_num, uint8_t ucMode, uint8_t ucBw, uint8_t ucRoleIdx);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Used to switche SAP channel based on certain conditions
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmIdcSwitchSapChannel(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Handles events to detect LTE safe channels and switch SAP channel
 * if necessary. @ref EVENT_ID_LTE_IDC_REPORT
 *
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 * @param[in] prEvent	Pointer to the WIFI_EVENT structure
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmIdcDetectHandler(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);
#endif

#if CFG_ENABLE_WIFI_DIRECT
/*----------------------------------------------------------------------------*/
/*!
 * @brief Handles events after Channel Switch Announcement (CSA) is completed
 * @ref EVENT_ID_CSA_DONE (Strong correlated with P2P)
 *
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 * @param[in] prEvent	Pointer to the WIFI_EVENT structure
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmCsaDoneEvent(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Resets CSA-related parameters in the ADAPTER structure.
 *
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 * @param[in] prBssInfo	Pointer to the BSS_INFO structure
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmCsaResetParams(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Requests a channel switch for the SAP based on provided RF Ch info.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 * @param[in] prRfChannelInfo	Pointer to the RF_CHANNEL_INFO
 * @param[in] ucRoleIdx	Role index for the channel switch
 *
 * @return 0 on successful message send, -1 on error
 */
/*----------------------------------------------------------------------------*/
uint8_t cnmSapChannelSwitchReq(struct ADAPTER *prAdapter,
	struct RF_CHANNEL_INFO *prRfChannelInfo,
	uint8_t ucRoleIdx, uint8_t ucMode);
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Finds the preferred channel for the given adapter and parameters.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 * @param[in] prBand	Pointer to the ENUM_BAND to store the preferred band
 * @param[in] pucPrimaryChannel	Pointer to store the preferred primary channel
 * @param[in] prBssSCO	Pointer to store the BSS secondary/extension CH offset
 *
 * @return TRUE: suggest to adopt the returned preferred channel
 *         FALSE: No suggestion. Caller should adopt its preference
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmPreferredChannel(struct ADAPTER *prAdapter, enum ENUM_BAND *prBand,
	uint8_t *pucPrimaryChannel, enum ENUM_CHNL_EXT *prBssSCO);

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Checks for the presence of an AIS fixed channel in the adapter.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 * @param[in] prBand	Pointer to  store the band of the fixed channel
 * @param[in] pucPrimaryChannel	Pointer to store the primary channel
 *            number of the fixed channel
 *
 * @return 0 on successful message send, -1 on error
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmAisInfraChannelFixed(struct ADAPTER *prAdapter,
	enum ENUM_BAND *prBand, uint8_t *pucPrimaryChannel);

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Checks if P2P operation is permitted for the given adapter.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 *
 * @return TRUE if P2P operation is permitted, FALSE otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmP2PIsPermitted(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Checks if 40MHz bandwidth is permitted for the given BSS index.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 * @param[in] ucBssIndex	The index of the BSS
 *
 * @return TRUE if 40MHz bandwidth is permitted, FALSE otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmBss40mBwPermitted(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Checks if 80MHz bandwidth is permitted for the given BSS index.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 * @param[in] ucBssIndex	The index of the BSS
 *
 * @return TRUE if 80MHz bandwidth is permitted, FALSE otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmBss80mBwPermitted(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Gets the maximum bandwidth for a specific BSS in the given adapter.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 * @param[in] ucBssIndex	The index of the BSS
 *
 * @return The maximum bandwidth for the BSS
 */
/*----------------------------------------------------------------------------*/
uint8_t cnmGetBssMaxBw(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Get the maximum bandwidth corresponding to a channel width.
 * This function calculates the maximum bandwidth based on the input BSS index
 * and checks if it is equal to 20MHz.
 * If it is, the function returns the maximum bandwidth (i.e., 20MHz),
 * otherwise it returns the maximum bandwidth minus 1.
 *
 * @param[in] prAdapter Pointer to the ADAPTER structure
 * @param[in] ucBssIndex Index of the BSS (Basic Service Set)
 *
 * @return Maximum bandwidth corresponding to the channel width
 */
/*----------------------------------------------------------------------------*/
uint8_t cnmGetBssMaxBwToChnlBW(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/*!
 * @brief    This function is used to retrieve and initialize
 *           a BSS_INFO structure based on the provided parameters,
 *           i.e., fgIsNetActive, ucBssIndex, eNetworkType
 *           and ucOwnMacIndex
 *
 * @param[in] prAdapter Pointer to the ADAPTER structure
 * @param[in] eNetworkType enum of network type
 * @param[in] fgIsP2pDevice determine whether is P2P device or not
 *
 * @return BSS_INFO
 */
/*----------------------------------------------------------------------------*/
struct BSS_INFO *cnmGetBssInfoAndInit(struct ADAPTER *prAdapter,
	enum ENUM_NETWORK_TYPE eNetworkType, u_int8_t fgIsP2pDevice,
	uint8_t ucTargetOwnMacIdx);

/*----------------------------------------------------------------------------*/
/*!
 * @brief    Frees the BSS information structure associated
 *           with a given wireless network.
 *
 * @param[in] prAdapter Pointer to the adapter structure
 * @param[in/out] prBssInfo Pointer to the BSS information structure to be freed
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmFreeBssInfo(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo);

#if CFG_SUPPORT_CHNL_CONFLICT_REVISE
/*----------------------------------------------------------------------------*/
/*!
 * @brief   This function used to find the operating channel
 *          and band of P2P device
 *
 * @param[in]   prAdapter	Pointer to the adapter structure
 * @param[in/out]   prBand	Pointer to the enum variable for band type
 * @param[in/out]   pucPrimaryChannel	Pointer to the primary CH
 *
 * @return TRUE: found the operating channel and band of P2P device
 *         FALSE: not found
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmAisDetectP2PChannel(struct ADAPTER *prAdapter,
	enum ENUM_BAND *prBand, uint8_t *pucPrimaryChannel);
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief	Function to make a decision about WMM index
 *
 * @param[in/out]	prAdapter	Pointer to the adapter structure
 * @param[in/out]	prBssInfo	Pointer to the BSS info structure
 *
 * @return	void
 */
/*----------------------------------------------------------------------------*/
void cnmWmmIndexDecision(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

/*----------------------------------------------------------------------------*/
/*!
 * @brief	Frees the WMM index associated with a BSS
 *          by resetting the hardware WMM enable bit,
 *          setting the WMM queue set to default,
 *          and indicating that WMM initialization is not performed.
 *
 * @param[in/out]	prAdapter	Pointer to the adapter structure
 * @param[in/out]	prBssInfo	Pointer to the BSS info structure
 *
 * @return	void
 */
/*----------------------------------------------------------------------------*/
void cnmFreeWmmIndex(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Get the capability of the bandwidth for a specific BSS index
 *        with considering DBDC mode
 *		  This function retrieves the bandwidth capability
 *        for a BSS with a given index.
 *
 * @param[in] prAdapter Pointer to the adapter structure.
 * @param[in] ucBssIndex Index of the BSS.
 *
 * @return The bandwidth capability of the BSS.
 */
/*----------------------------------------------------------------------------*/
uint8_t cnmGetDbdcBwCapability(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex
);

#if CFG_SUPPORT_DBDC
/*----------------------------------------------------------------------------*/
/*!
 * @brief Initializes the DBDC (Dual Band Dual Concurrent) setting
 *			This function initializes the DBDC setting
 *			based on the given adapters DBDC mode.
 *			It sets various parameters and updates the DBDC setting.
 *
 * @param[in/out] prAdapter Pointer to the adapter structure.
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmInitDbdcSetting(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief	Updates the DBDC (Dual Band Dual Concurrent) setting
 *			for the WLAN adapter.
 *
 * @param[in]   prAdapter	Pointer to the WLAN adapter structure
 * @param[in]   fgDbdcEn	Flag indicating whether to
 *							enable or disable DBDC
 *
 * @return	WLAN_STATUS_SUCCESS if the operation is successful,
 *			otherwise an error code
 */
/*----------------------------------------------------------------------------*/
uint32_t cnmUpdateDbdcSetting(
	struct ADAPTER *prAdapter,
	u_int8_t fgDbdcEn);

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function checks if the CNM DBDC mode is disabled
 *        based on the provided adapter.
 *        check whether DBDC is disabled
 *        to avoid STA disable DBDC during roaming, which may cause DBDC->MCC,
 *        check DBDC status before cnmDbdcPreConnectionEnableDecision
 *
 * @param[in]   prAdapter	Pointer to the ADAPTER structure.
 *
 * @return  TRUE if CNM DbDc mode is deemed disabled, FALSE otherwise.
 */
/*----------------------------------------------------------------------------*/
bool cnmDbdcIsDisabled(struct ADAPTER *prAdapter);

#if (CFG_SUPPORT_DBDC_SUSPEND_FLOW == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function checks if the CNM DBDC FSM is DISABLE_IDLE or
 *        ENABLE_IDLE based on the provided adapter.
 *        check whether DBDC is idle
 *        to avoid DBDC FSM confusion when suspend
 *
 * @param[in]   prAdapter       Pointer to the ADAPTER structure.
 *
 * @return  TRUE if CNM DbDc FSM is DISABLE_IDLE or ENABLE_IDLE,
 * FALSE otherwise.
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmDbdcFsmIsIdle(struct ADAPTER *prAdapter);
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief	This function enables or disables the pre-connection
 *          decision for DBDC (Dual Band Dual Concurrent) mode.
 *			It updates WMM band information, DBDC settings,
 *          and manages guard timers based on the decision info
 *          and adapter state.
 *			Run-time check if DBDC Need enable or update guard time.
 *          The WmmQ is set to the correct DBDC band before connetcting.
 *          It could make sure the TxPath is correct after connected.
 *
 * @param[in]	prAdapter	Pointer to the adapter structure
 * @param[in/out]	prDbdcDecisionInfo	Pointer to the decision info
 *
 * @return  void
 */
/*----------------------------------------------------------------------------*/
void cnmDbdcPreConnectionEnableDecision(
	struct ADAPTER *prAdapter,
	struct DBDC_DECISION_INFO *prDbdcDecisionInfo);

/*----------------------------------------------------------------------------*/
/*!
 * @brief	This function performs a runtime check decision
 *          based on various conditions.
 *          It updates DBDC related settings and timers accordingly
 *
 * @param[in]	prAdapter	Pointer to the adapter structure
 * @param[in]	ucChangedBssIndex	Index of the changed BSS
 * @param[in]	ucForceLeaveEnGuard	Flag to force leave the guard
 *
 * @return  void
 */
/*----------------------------------------------------------------------------*/
void cnmDbdcRuntimeCheckDecision(struct ADAPTER *prAdapter,
	uint8_t ucChangedBssIndex,
	u_int8_t ucForceLeaveEnGuard);

#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief	Function to get the status of whether DBDC is enabled for
 *          P2P listening
 *
 * @param[in]	prAdapter	Pointer to the adapter structure
 *
 * @return  1 if DBDC is enabled by P2P Listen DBDC, 0 otherwise.
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmDbdcIsP2pListenDbdcEn(struct ADAPTER *prAdapter);
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief	Callback function for CNM DBDC guard timer/Countdown.
 *
 * @param[in]	prAdapter	Pointer to the adapter structure
 * @param[in/out]	plParamPtr	Parameter pointer
 *
 * @return  void
 */
/*----------------------------------------------------------------------------*/
void cnmDbdcGuardTimerCallback(struct ADAPTER *prAdapter,
	uintptr_t plParamPtr);

/*----------------------------------------------------------------------------*/
/*!
 * @brief	Function to handle DBDC hardware switch completion event.
 *
 * @param[in/out]	prAdapter	Pointer to the adapter structure
 * @param[in]	prEvent	Pointer to the Wi-Fi event structure
 *
 * @return  void
 */
/*----------------------------------------------------------------------------*/
void cnmDbdcEventHwSwitchDone(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);

#if (CFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL == 1)

/*----------------------------------------------------------------------------*/
/*!
 * @brief	Function to set Antenna to BT or WF.
 *
 * @param[in/out]	prAdapter	Pointer to the adapter structure
 * @param[in]	prShrAntSwchEvent	Select Antenna grant to which radio
 *
 * @return  void
 */
/*----------------------------------------------------------------------------*/
void cnmUpdateSharedAntennaSwitch(struct ADAPTER *prAdapter,
	struct UNI_EVENT_SHR_ANT_SWCH_UPDATE_T *prShrAntSwchEvent);

#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief	Function to check if DBDC privilege lock is requested.
 *
 * @param[in]	void
 *
 * @return  True if DBDC privilege lock is requested, otherwise false
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmDBDCIsReqPeivilegeLock(struct ADAPTER *prAdapter);
#if (CFG_SUPPORT_DBDC_SUSPEND_FLOW == 1)
void cnmDbdcPreResumeFlow(struct ADAPTER *prAdapter);
void cnmDbdcPreSuspendFlow(struct ADAPTER *prAdapter);
#endif /*CFG_SUPPORT_DBDC_SUSPEND_FLOW*/
#endif /*CFG_SUPPORT_DBDC*/

/*----------------------------------------------------------------------------*/
/*!
 * @brief	Function to get the network type from BSS information.
 *
 * @param[in]	prBssInfo	Pointer to the BSS information structure
 *
 * @return  The corresponding @ref ENUM_CNM_NETWORK_TYPE_T
 *          based on the BSS network type
 */
/*----------------------------------------------------------------------------*/
enum ENUM_CNM_NETWORK_TYPE_T cnmGetBssNetworkType(struct BSS_INFO *prBssInfo);

#if CFG_ENABLE_WIFI_DIRECT
/*----------------------------------------------------------------------------*/
/*!
 * @brief Checks if the specified adapter of SAP is active
 *        by verifying whether BSSinformation for the SAP is available or not.
 *
 * @param[in] prAdapter	pointer to the ADAPTER representing the adapter
 *
 * @return 1 if SAP is active, 0 otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmSapIsActive(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function checks if the given adapter is running
 *        in concurrent mode.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER structure
 *
 * @return TRUE if the adapter is running in concurrent mode,
 *         otherwise returns FALSE.
 *
 * @note
 * The function first checks if the pointer prAdapter is not NULL.
 * Then, it checks if the u4P2pMode field of the ADAPTER structure
 * is not equal to @ref RUNNING_P2P_MODE.
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmSapIsConcurrent(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Function to get the BSS Information of a SAP.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER struct containing adapter
 *
 * @return Pointer to the BSS_INFO struct of the SAP, or NULL if not found.
 */
/*----------------------------------------------------------------------------*/
struct BSS_INFO *cnmGetSapBssInfo(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Function to get BSS Information of other SAPs.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER struct containing adapter
 * @param[in] prNonSapBssInfo	Pointer to the BSS_INFO of the current SAP.
 *
 * @return Pointer to the BSS_INFO struct of another SAP, or NULL if not found
 */
/*----------------------------------------------------------------------------*/
struct BSS_INFO *
cnmGetOtherSapBssInfo(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prSapBssInfo);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Retrieve alive BSS info for SAP from the given adapter.
 *
 * @param[in] prAdapter	prAdapter Pointer to the adapter structure
 * @param[in] prNonSapBssInfo	pointer of pointer to BSS_INFO array
 *                              to store SAP BSS info.
 *
 * @return The number of alive SAP BSS info found and stored in prSapBssInfo.
 */
/*----------------------------------------------------------------------------*/
uint8_t
cnmGetAliveSapBssInfo(
	struct ADAPTER *prAdapter,
	struct BSS_INFO **prNonSapBssInfo);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Function to get information about non-SAP alive BSS from the adapter.
 *
 * @param[in] prAdapter	prAdapter Pointer to the adapter structure
 * @param[in] prNonSapBssInfo	Pointer to store non-SAP BSS information
 *
 * @return Number of alive non-Sap BSS found
 */
/*----------------------------------------------------------------------------*/
uint8_t
cnmGetAliveNonSapBssInfo(
	struct ADAPTER *prAdapter,
	struct BSS_INFO **prNonSapBssInfo);
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Function to get the operating mode TRx NSS values for a specific BSS.
 *
 * @param[in] prAdapter	prAdapter Pointer to the adapter structure
 * @param[in] ucBssIndex	ucBssIndex Index of the BSS
 * @param[in/out] pucOpRxNss	Pointer to store the operating mode Rx NSS
 * @param[in/out] pucOpTxNss	Pointer to store the operating mode Tx NSS
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmOpModeGetTRxNss(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t *pucOpRxNss,
	uint8_t *pucOpTxNss
);

#if CFG_SUPPORT_SMART_GEAR
/*----------------------------------------------------------------------------*/
/*!
 * @brief Handle Smart Antenna Status Change Event for SmartGear
 * This function processes the Smart Antenna status change event and reports it.
 *
 * @param[in] prAdapter	Pointer to the adapter structure
 * @param[in] prEvent	Pointer to the Wi-Fi event structure
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmEventSGStatus(
	struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent
);
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Handles event related to operation mode changes in Wi-Fi adapter.
 *
 * @param[in] prAdapter	Pointer to the Wi-Fi adapter structure
 * @param[in] prEvent	Pointer to the Wi-Fi event structure
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmOpmodeEventHandler(
	struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent
);

#if (CFG_SUPPORT_DFS_MASTER == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief Event handler for RDD operational mode change event
 *        for @ref EVENT_ID_OPMODE_CHANGE
 *
 * @param[in] prAdapter	Pointer to the adapter structure
 * @param[in] prEvent	Pointer to the Wi-Fi event structure
 *
 * @return	vode
 */
/*----------------------------------------------------------------------------*/
void cnmRddOpmodeEventHandler(
	struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent
);
#endif

#if CFG_ENABLE_WIFI_DIRECT
/*----------------------------------------------------------------------------*/
/*!
 * @brief Check if the P2P feature is active for the provided adapter.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER struct representing the adapter.
 *
 * @return	1 if P2P is active, 0 otherwise.
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmP2pIsActive(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Function to retrieve P2P BSS information from the given adapter.
 *
 * @param[in] prAdapter	Pointer to the ADAPTER containing
 *
 * @return	Pointer to the BSS_INFO of P2P BSS, or NULL if not found
 */
/*----------------------------------------------------------------------------*/
struct BSS_INFO *cnmGetP2pBssInfo(struct ADAPTER *prAdapter);
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Check if the adapter is in Multi-Channel Concurrence mode
 *
 * @param[in] prAdapter	Pointer to the adapter structure
 *
 * @retval	true if MCC mode, false otherwise
 */
/*----------------------------------------------------------------------------*/
bool cnmIsMccMode(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Convert frequency to channel and band information
 *
 * @param[in] u4Freq	Frequency input
 * @param[out] ucChannel	Channel output
 * @param[out] eBand	Band output
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmFreqToChnl(uint32_t u4Freq, u8 *ucChannel, enum ENUM_BAND *eBand);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Get the band based on the input frequency.
 *
 * @param[in] u4Freq	The frequency for which to determine the band.
 *
 * @return The band corresponding to the input frequency.
 */
/*----------------------------------------------------------------------------*/
enum ENUM_BAND cnmGetBandByFreq(uint32_t u4Freq);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Maps generic band enumeration to 802.11 band enumeration.
 * This function converts a generic band enum to its corresponding
 * 802.11 band enum.
 * It handles the mapping for 2.4GHz, 5GHz, and optional 6GHz bands.
 *
 * @param[in] eBand	The input generic band enum.
 *
 * @return The mapped 802.11 band enum.
 */
/*----------------------------------------------------------------------------*/
enum ENUM_BAND_80211 cnmGet80211Band(enum ENUM_BAND eBand);

#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief Control the power settings for the adapter based on
 * the specified level
 *
 * @param[in] prAdapter	Pointer to the adapter structure
 * @param[in] level	Power level to be set
 *
 * @return	0 if successful
 */
/*----------------------------------------------------------------------------*/
int cnmPowerControl(struct ADAPTER *prAdapter, uint8_t level);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Function to handle power control errors based on network type
 *
 * @param prAdapter	Pointer to the adapter structure
 * @param prBssInfo	Pointer to the BSS information structure
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmPowerControlErrorHandling(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo
);
#endif

#if (CFG_SUPPORT_DBDC == 1 && CFG_UPDATE_STATIC_DBDC_QUOTA == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief Function to update static DBDC quota based on WMM settings
 * This function updates the static DBDC quota based on WMM settings
 * for a given adapter.
 *
 * @param[in] prAdapter Pointer to the ADAPTER struct containing
 * chip information and BSS info
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmUpdateStaticDbdcQuota(
	struct ADAPTER *prAdapter);

#endif

#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function determines the maximum quota hardware band based on
 * WMM index and returns it.
 *
 * @param[in] prAdapter Pointer to the ADAPTER struct containing
 * chip information and BSS info
 * @param[in] ucWmmIndex WMM index used for filtering
 * @param[in/out] eBand Pointer to store the selected band
 * @param[in/out] fgIsMldMulti Pointer to a flag indicating
 * if MLD is multi or not
 *
 * @return The target hardware band based on the input parameters
 */
/*----------------------------------------------------------------------------*/
enum ENUM_MBMC_BN cnmGetMaxQuotaHwBandByWmmIndex(
	struct ADAPTER *prAdapter, uint8_t ucWmmIndex,
	enum ENUM_BAND *eBand, u_int8_t *fgIsMldMulti);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Function to update dynamic maximum quota for each WMM index
 *
 * @param[in] prAdapter pointer to the ADAPTER structure whose token ID
 * will be incremented
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void cnmCtrlDynamicMaxQuota(struct ADAPTER *prAdapter);

#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#ifndef _lint
/* We don't have to call following function to inspect the data structure.
 * It will check automatically while at compile time.
 * We'll need this to guarantee the same member order in different structures
 * to simply handling effort in some functions.
 */
static __KAL_INLINE__ void cnmMsgDataTypeCheck(void)
{
	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, rMsgHdr)
			== 0);

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, rMsgHdr)
			== OFFSET_OF(struct MSG_CH_REOCVER, rMsgHdr));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, ucBssIndex)
			== OFFSET_OF(struct MSG_CH_REOCVER, ucBssIndex));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, ucTokenID)
			== OFFSET_OF(struct MSG_CH_REOCVER, ucTokenID));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, ucPrimaryChannel)
			== OFFSET_OF(struct MSG_CH_REOCVER, ucPrimaryChannel));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, eRfSco)
			== OFFSET_OF(struct MSG_CH_REOCVER, eRfSco));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, eRfBand)
			== OFFSET_OF(struct MSG_CH_REOCVER, eRfBand));

	DATA_STRUCT_INSPECTING_ASSERT(
		OFFSET_OF(struct MSG_CH_GRANT, eReqType)
			== OFFSET_OF(struct MSG_CH_REOCVER, eReqType));
}
#endif /* _lint */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to increase the token ID
 * It increments the tokenId field by 1 and returns the updated value.
 *
 * @param[in] prAdapter pointer to the ADAPTER structure whose token ID
 * will be incremented
 *
 * @return The updated token ID after incrementing.
 */
/*----------------------------------------------------------------------------*/
uint8_t cnmIncreaseTokenId(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * @brief Get the maximum bandwidth supported by the current operation mode
 * and BSS information
 *
 * @param[in] prAdapter	Pointer to the adapter structure.
 * @param[in] prBssinfo	Pointer to the BSS information structure.
 *
 * @return Maximum bandwidth supported
 */
/*----------------------------------------------------------------------------*/
uint8_t cnmOpModeGetMaxBw(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);
#endif /* _CNM_H */
