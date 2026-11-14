/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   aa_fsm.h
 *    \brief  Declaration of functions and finite state machine for
 *							SAA/AAA Module.
 *
 *    Declaration of functions and finite state machine for SAA/AAA Module.
 */

#ifndef _AA_FSM_H
#define _AA_FSM_H

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
/* Retry interval for retransmiting authentication-request MMPDU. */
#define TX_AUTHENTICATION_RETRY_TIMEOUT_TU          100	/* TU. */

/* Retry interval for retransmiting association-request MMPDU. */
#define TX_ASSOCIATION_RETRY_TIMEOUT_TU             100	/* TU. */

/* Retry interval for external authentication */
#define EXTERNAL_AUTHENTICATION_TIMEOUT_TU	1500	/* TU */

/* Wait for a response to a transmitted authentication-request MMPDU. */
#define DOT11_AUTHENTICATION_RESPONSE_TIMEOUT_TU    512/* TU. */
#if CFG_MTK_FPGA_PLATFORM
/* Wait for a response to a transmitted association-request MMPDU. */
#define DOT11_ASSOCIATION_RESPONSE_TIMEOUT_TU       5120/* TU. */

/* Wait for a response to a transmitted SAE authentication MMPDU. */
/* 2000 msec is the default value on 802.11-REVmd-D0.5 */
#define DOT11_RSNA_SAE_RETRANS_PERIOD_TU            5120
#else
/* Wait for a response to a transmitted association-request MMPDU. */
#define DOT11_ASSOCIATION_RESPONSE_TIMEOUT_TU       512	/* TU. */

/* Wait for a response to a transmitted SAE authentication MMPDU. */
/* 2000 msec is the default value on 802.11-REVmd-D0.5 */
#define DOT11_RSNA_SAE_RETRANS_PERIOD_TU            2000
#endif

/* The maximum time to wait for JOIN process complete. */
/* Beacon Interval, 20 * 100TU = 2 sec. */
#define JOIN_FAILURE_TIMEOUT_BEACON_INTERVAL        20

/* Retry interval for next JOIN request. */
#define JOIN_RETRY_INTERVAL_SEC                     10	/* Seconds */

/* Maximum Retry Count for accept a JOIN request. */
#define JOIN_MAX_RETRY_FAILURE_COUNT                2	/* Times */

#if CFG_MTK_FPGA_PLATFORM
#define TX_AUTHENTICATION_RESPONSE_TIMEOUT_TU        5120 /* TU. */
#else
#define TX_AUTHENTICATION_RESPONSE_TIMEOUT_TU        512 /* TU. */
#endif
#define TX_ASSOCIATE_TIMEOUT_TU        2048 /* TU. */
#define JOIN_MAX_RETRY_OVERLOAD_RN		    1	/* Times */

#if CFG_AP_80211KVR_INTERFACE
#define SAP_CHAN_NOISE_GET_INFO_PERIOD              5000 /* ms */
#endif

#define TX_ACTION_RETRY_TIMEOUT_TU                  1000 /* TU. */
#define TX_ACTION_RESPONSE_TIMEOUT_TU               1000 /* TU. */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
enum ENUM_AA_STATE {
	AA_STATE_IDLE = 0,
	SAA_STATE_SEND_AUTH1,
	SAA_STATE_WAIT_AUTH2,
	SAA_STATE_SEND_AUTH3,
	SAA_STATE_WAIT_AUTH4,
	SAA_STATE_EXTERNAL_AUTH,
	SAA_STATE_SEND_ASSOC1,
	SAA_STATE_WAIT_ASSOC2,
	AAA_STATE_SEND_AUTH2,
	AAA_STATE_SEND_AUTH4,	/* We may not use,			     */
				/* because P2P GO didn't support WEP and 11r */
	AAA_STATE_SEND_ASSOC2,
	AA_STATE_RESOURCE,	/* A state for debugging the case of	     */
				/* out of msg buffer.			     */
	AA_STATE_NUM
};

enum ENUM_AA_FRM_TYPE {
	FRM_DISASSOC = 0,
	FRM_DEAUTH
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

/*----------------------------------------------------------------------------*/
/* Routines in saa_fsm.c                                                      */
/*----------------------------------------------------------------------------*/
void
saaFsmSteps(struct ADAPTER *prAdapter,
	    struct STA_RECORD *prStaRec,
	    enum ENUM_AA_STATE eNextState,
	    struct SW_RFB *prRetainedSwRfb);

uint32_t
saaFsmSendEventJoinComplete(struct ADAPTER *prAdapter,
		enum ENUM_AA_STATE ePreviousState, uint32_t rJoinStatus,
		struct STA_RECORD *prStaRec, struct SW_RFB *prSwRfb);

void saaFsmRunEventStart(struct ADAPTER *prAdapter,
			 struct MSG_HDR *prMsgHdr);

uint32_t
saaFsmRunEventTxDone(struct ADAPTER *prAdapter,
		     struct MSDU_INFO *prMsduInfo,
		     enum ENUM_TX_RESULT_CODE rTxDoneStatus);

void saaFsmRunEventTxReqTimeOut(struct ADAPTER
				*prAdapter, uintptr_t plParamPtr);

void saaFsmRunEventRxRespTimeOut(struct ADAPTER
				 *prAdapter, uintptr_t ulParamPtr);

void saaFsmRunEventRxAuth(struct ADAPTER *prAdapter,
			  struct SW_RFB *prSwRfb);

uint32_t saaFsmRunEventRxAssoc(struct ADAPTER *prAdapter,
			       struct SW_RFB *prSwRfb);

uint32_t saaFsmRunEventRxDeauth(struct ADAPTER
				*prAdapter, struct SW_RFB *prSwRfb);

uint32_t saaFsmRunEventRxDisassoc(struct ADAPTER
				  *prAdapter, struct SW_RFB *prSwRfb);

void saaFsmRunEventAbort(struct ADAPTER *prAdapter,
			 struct MSG_HDR *prMsgHdr);

void saaFsmStateAbort(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec,
			struct SW_RFB *prRetainedSwRfb);

void saaChkDeauthfrmParamHandler(struct ADAPTER
				 *prAdapter, struct SW_RFB *prSwRfb,
				 struct STA_RECORD *prStaRec);

void
saaChkDisassocfrmParamHandler(struct ADAPTER *prAdapter,
			      struct WLAN_DISASSOC_FRAME *prDisassocFrame,
			      struct STA_RECORD *prStaRec,
			      struct SW_RFB *prSwRfb);

void
saaSendDisconnectMsgHandler(struct ADAPTER *prAdapter,
			    struct STA_RECORD *prStaRec,
			    struct BSS_INFO *prAisBssInfo,
			    enum ENUM_AA_FRM_TYPE eFrmType);

void saaFsmRunEventFTContinue(struct ADAPTER *prAdapter,
			      struct MSG_HDR *prMsgHdr);

void saaFsmRunEventExternalAuthDone(struct ADAPTER *prAdapter,
				    struct MSG_HDR *prMsgHdr);

/*----------------------------------------------------------------------------*/
/* Routines in aaa_fsm.c                                                      */
/*----------------------------------------------------------------------------*/
void aaaFsmRunEventRxAuth(struct ADAPTER *prAdapter,
			  struct SW_RFB *prSwRfb);

uint32_t aaaFsmRunEventRxAssoc(struct ADAPTER *prAdapter,
			       struct SW_RFB *prSwRfb);

uint32_t
aaaFsmRunEventTxDone(struct ADAPTER *prAdapter,
		     struct MSDU_INFO *prMsduInfo,
		     enum ENUM_TX_RESULT_CODE rTxDoneStatus);

#if CFG_AP_80211KVR_INTERFACE
void aaaMulAPAgentChanNoiseInitWorkHandler(
	struct work_struct *work);

void aaaMulAPAgentChanNoiseCollectionWorkHandler(
	struct work_struct *work);

void aaaMulAPAgentStaEventNotify(struct STA_RECORD *prStaRec,
	unsigned char *pucAddr, unsigned char fgIsConnected);

void aaaMulAPAgentUnassocStaMeasureTimeout(
	struct ADAPTER *prAdapter, uintptr_t ulParamPtr);
#endif
#endif /* _AA_FSM_H */
