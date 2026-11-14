/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  wnm.h
 *    \brief This file contains the IEEE 802.11 family related 802.11v
 *		network management for MediaTek 802.11 Wireless LAN Adapters.
 */


#ifndef _WNM_H
#define _WNM_H

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

/*******************************************************************************
 *                         D A T A   T Y P E S
 *******************************************************************************
 */

struct TIMINGMSMT_PARAM {
	u_int8_t fgInitiator;
	uint8_t ucTrigger;
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t ucFollowUpDialogToken;	/* Follow Up Dialog Token */
	uint32_t u4ToD;		/* Timestamp of Departure [10ns] */
	uint32_t u4ToA;		/* Timestamp of Arrival [10ns] */
};

struct BSS_TRANSITION_MGT_PARAM {
	/* bssidx to response */
	uint8_t ucRspBssIndex;
	/* for Query */
	uint8_t ucQueryDialogToken;
	uint8_t fgWaitBtmRequest;
	/* for Request */
	uint8_t ucDialogToken;
	uint8_t ucRequestMode;
	uint32_t u4ReauthDelay;
	uint16_t u2TermDuration;
	uint8_t aucTermTsf[8];
	uint8_t ucSessionURLLen;
	uint8_t aucSessionURL[255];
	uint8_t ucDisImmiState;
	uint8_t aucBSSID[MAC_ADDR_LEN];
#if CFG_EXT_ROAMING_WTC
	uint8_t ucIsCisco;
#endif
	/* for Respone */
	uint8_t fgPendingResponse:1;
	uint8_t fgIsMboPresent:1;
	u_int8_t fgReserved:6;
	uint8_t ucStatusCode;
#if CFG_EXT_ROAMING_WTC
	uint8_t ucVsieReasonCode;
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
#define WNM_MAX_NEIGHBOR_REPORT 10

/* IEEE 802.11v - BSS Transition Management Request - Request Mode */
#define WNM_BSS_TM_REQ_PREF_CAND_LIST_INCLUDED BIT(0)
#define WNM_BSS_TM_REQ_ABRIDGED BIT(1)
#define WNM_BSS_TM_REQ_DISASSOC_IMMINENT BIT(2)
#define WNM_BSS_TM_REQ_BSS_TERMINATION_INCLUDED BIT(3)
#define WNM_BSS_TM_REQ_ESS_DISASSOC_IMMINENT BIT(4)
#define WNM_BSS_TM_REQ_LINK_REMOVAL_IMMINENT BIT(5)

#define BSS_TRANSITION_MGT_STATUS_ACCEPT			0
#define BSS_TRANSITION_MGT_STATUS_UNSPECIFIED			1
#define BSS_TRANSITION_MGT_STATUS_NEED_SCAN			2
#define BSS_TRANSITION_MGT_STATUS_CAND_NO_CAPACITY		3
#define BSS_TRANSITION_MGT_STATUS_TERM_UNDESIRED		4
#define BSS_TRANSITION_MGT_STATUS_TERM_DELAY_REQUESTED		5
#define BSS_TRANSITION_MGT_STATUS_CAND_LIST_PROVIDED		6
#define BSS_TRANSITION_MGT_STATUS_CAND_NO_CANDIDATES		7
#define BSS_TRANSITION_MGT_STATUS_LEAVING_ESS			8

/* IEEE Std 802.11-2012 - Table 8-253 */
enum BSS_TRANS_MGMT_STATUS_CODE {
	WNM_BSS_TM_ACCEPT = 0,
	WNM_BSS_TM_REJECT_UNSPECIFIED = 1,
	WNM_BSS_TM_REJECT_INSUFFICIENT_BEACON = 2,
	WNM_BSS_TM_REJECT_INSUFFICIENT_CAPABITY = 3,
	WNM_BSS_TM_REJECT_UNDESIRED = 4,
	WNM_BSS_TM_REJECT_DELAY_REQUEST = 5,
	WNM_BSS_TM_REJECT_STA_CANDIDATE_LIST_PROVIDED = 6,
	WNM_BSS_TM_REJECT_NO_SUITABLE_CANDIDATES = 7,
	WNM_BSS_TM_REJECT_LEAVING_ESS = 8
};

/* 802.11v Table 9-176: define Transtion and Transition Query reasons */
#define BSS_TRANSITION_LOAD_BALANCING                   5
#define BSS_TRANSITION_BETTER_AP_FOUND                  6
#define BSS_TRANSITION_LOW_RSSI                         16
#define BSS_TRANSITION_INCLUDE_PREFER_CAND_LIST         19
#define BSS_TRANSITION_LEAVING_ESS                      20

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void wnmWNMAction(struct ADAPTER *prAdapter,
		  struct SW_RFB *prSwRfb);

void wnmReportTimingMeas(struct ADAPTER *prAdapter,
			 uint8_t ucStaRecIndex, uint32_t u4ToD,
			 uint32_t u4ToA);

#if WNM_UNIT_TEST
void wnmTimingMeasUnitTest1(struct ADAPTER *prAdapter,
			    uint8_t ucStaRecIndex);
#endif

void wnmSendBTMQueryFrame(struct ADAPTER *prAdapter,
		 struct STA_RECORD *prStaRec, uint8_t ucQueryReason);

#if CFG_SUPPORT_802_11V_BTM_OFFLOAD
void wnmRecvBTMRequest(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb);

void wnmSendBTMResponseFrame(struct ADAPTER *adapter,
	struct STA_RECORD *staRec, PFN_TX_DONE_HANDLER pfTxDoneHandler,
	uint8_t dialogToken, uint8_t status, uint8_t reason, uint8_t delay,
	const uint8_t *bssid);
#endif /* CFG_SUPPORT_802_11V_BTM_OFFLOAD */

#if CFG_AP_80211V_SUPPORT
void wnmMulAPAgentSendBTMRequestFrame(
			struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec,
			struct PARAM_CUSTOM_BTM_REQ_STRUCT *prSetBtmReqInfo);

void wnmMulAPAgentRecvBTMResponse(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);
#endif /* CFG_AP_80211V_SUPPORT */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _WNM_H */
