/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   rsn.h
 *  \brief  The wpa/rsn related define, macro and structure are described here.
 */

#ifndef _RTT_H
#define _RTT_H

#if CFG_SUPPORT_RTT
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#if CFG_SUPPORT_PASN
#include "pasn.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define RTT_REQUEST_DONE_TIMEOUT_SEC 4
#define RTT_REQUEST_CONT_TIMEOUT_SEC 2

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* Ranging status */
enum ENUM_RTT_STATUS {
	RTT_STATUS_SUCCESS       = 0,
	/* general failure status */
	RTT_STATUS_FAILURE       = 1,
	/* target STA does not respond to request */
	RTT_STATUS_FAIL_NO_RSP   = 2,
	/* request rejected. Applies to 2-sided RTT only */
	RTT_STATUS_FAIL_REJECTED = 3,
	RTT_STATUS_FAIL_NOT_SCHEDULED_YET  = 4,
	/* timing measurement times out */
	RTT_STATUS_FAIL_TM_TIMEOUT         = 5,
	/* Target on different channel, cannot range */
	RTT_STATUS_FAIL_AP_ON_DIFF_CHANNEL = 6,
	/* ranging not supported */
	RTT_STATUS_FAIL_NO_CAPABILITY  = 7,
	/* request aborted for unknown reason */
	RTT_STATUS_ABORTED             = 8,
	/* Invalid T1-T4 timestamp */
	RTT_STATUS_FAIL_INVALID_TS     = 9,
	/* 11mc protocol failed */
	RTT_STATUS_FAIL_PROTOCOL       = 10,
	/* request could not be scheduled */
	RTT_STATUS_FAIL_SCHEDULE       = 11,
	/* responder cannot collaborate at time of request */
	RTT_STATUS_FAIL_BUSY_TRY_LATER = 12,
	/* bad request args */
	RTT_STATUS_INVALID_REQ         = 13,
	/* WiFi not enabled */
	RTT_STATUS_NO_WIFI             = 14,
	/* Responder overrides param info, cannot range with new params */
	RTT_STATUS_FAIL_FTM_PARAM_OVERRIDE = 15,
	/* Negotiation failure */
	RTT_STATUS_NAN_RANGING_PROTOCOL_FAILURE = 16,
	/* concurrency not supported (NDP+RTT) */
	RTT_STATUS_NAN_RANGING_CONCURRENCY_NOT_SUPPORTED = 17,
};

/* RTT peer type */
enum ENUM_RTT_PEER_TYPE {
	RTT_PEER_AP         = 0x1,
	RTT_PEER_STA        = 0x2,
	RTT_PEER_P2P_GO     = 0x3,
	RTT_PEER_P2P_CLIENT = 0x4,
	RTT_PEER_NAN_RSTA     = 0x5,
	RTT_PEER_NAN_ISTA    = 0x6
};

/* RTT Measurement Bandwidth */
enum ENUM_WIFI_RTT_BW {
	WIFI_RTT_BW_5   = 0x01,
	WIFI_RTT_BW_10  = 0x02,
	WIFI_RTT_BW_20  = 0x04,
	WIFI_RTT_BW_40  = 0x08,
	WIFI_RTT_BW_80  = 0x10,
	WIFI_RTT_BW_160 = 0x20,
	WIFI_RTT_BW_320 = 0x40
};

/* RTT Measurement Preamble */
enum ENUM_WIFI_RTT_PREAMBLE {
	WIFI_RTT_PREAMBLE_LEGACY = 0x1,
	WIFI_RTT_PREAMBLE_HT     = 0x2,
	WIFI_RTT_PREAMBLE_VHT    = 0x4,
	WIFI_RTT_PREAMBLE_HE     = 0x8,
	WIFI_RTT_PREAMBLE_EHT    = 0x10
};

/* RTT Type */
enum ENUM_RTT_TYPE {
	RTT_TYPE_1_SIDED = 0x1,
	RTT_TYPE_2_SIDED = 0x2,
	RTT_TYPE_2_SIDED_11MC = RTT_TYPE_2_SIDED,
	RTT_TYPE_2_SIDED_11AZ_NTB = 0x3
};

/* RTT state */
enum ENUM_RTT_STATE {
	RTT_STATE_IDLE,
	RTT_STATE_RTT_START,
	RTT_STATE_RTT_DONE,
#if CFG_SUPPORT_PASN
	RTT_STATE_PASN,
#endif
	RTT_STATE_NUM
};

struct PARAM_RTT_REQUEST {
	uint8_t fgEnable;
	uint8_t ucConfigNum;
	struct RTT_CONFIG arRttConfigs[CFG_RTT_MAX_CANDIDATES];
};

struct RTT_RESULT_ENTRY {
	struct LINK_ENTRY rLinkEntry;
	struct RTT_RESULT rResult;
	uint16_t u2IELen;
	/* Keep it last */
	uint8_t aucIE[];
};

struct RTT_CLIENT_ENTRY {
	struct LINK_ENTRY rLinkEntry;
	enum ENUM_STA_TYPE eStaType;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint8_t ucSeqNum;
};

struct RTT_INFO {
	uint8_t ucBssIndex;
	uint8_t fgIsRunning;
	uint8_t fgIsContRunning;
	uint8_t ucSeqNum;
	uint8_t ucState; /* ENUM_RTT_STATE */
	enum ENUM_RTT_PEER_TYPE eRttPeerType;
	struct LINK rResultList;
	struct LINK rClientList;
	struct TIMER rRttDoneTimer;
	struct TIMER rRttContTimer; /* Continuous RTT requests */
	struct RTT_CAPABILITIES rRttCapabilities;
#if CFG_SUPPORT_PASN
	struct CMD_RTT_REQUEST *prRttReq;
	uint8_t ucNumPeers;
	struct PASN_PEER arPeer[PASN_MAX_PEERS];
	uint8_t ucOldBssColorInfo; /* Original bss color in BSSInfo */
	uint8_t ucOldPhyTypSet; /* Original PhyTypeSet in BSSInfo */
#endif
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

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void rttInit(struct ADAPTER *prAdapter);

uint8_t rttBssBwToRttBw(uint8_t ucBssBw);

void rttUninit(struct ADAPTER *prAdapter);

uint8_t rttIsRunning(struct ADAPTER *prAdapter);

uint8_t rttIsSupport(struct ADAPTER *prAdapter);

uint32_t rttHandleRttRequest(struct ADAPTER *prAdapter,
	struct PARAM_RTT_REQUEST *prRequest,
	uint8_t ucBssIndex);

uint32_t rttHandleDeauth(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

void rttEventDone(struct ADAPTER *prAdapter,
		      struct EVENT_RTT_DONE *prEvent);

void rttEventResult(struct ADAPTER *prAdapter,
		      struct EVENT_RTT_RESULT *prEvent);

uint8_t rttBssBwToRttBw(uint8_t ucBssBw);

uint8_t rttBwToBssBw(uint8_t eRttBw);

void rttProcessPublicAction(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb);

uint8_t rttBlockScan(struct ADAPTER *prAdapter);

#if CFG_SUPPORT_PASN
uint32_t rttDoPasn(struct ADAPTER *prAdapter,
			struct PARAM_RTT_REQUEST *prRequest,
			uint8_t ucBssIndex);

uint32_t rttCancelPasn(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex);

uint32_t rttDeleteSecureCtx(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec,
			uint8_t ucBssIndex);
#endif /* CFG_SUPPORT_PASN */
#endif /* CFG_SUPPORT_RTT */
#endif /* _RTT_H */
