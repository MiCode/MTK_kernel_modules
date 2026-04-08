/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _TWT_REQ_FSM_H
#define _TWT_REQ_FSM_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

enum _ENUM_TWT_REQUESTER_STATE_T {
	TWT_REQ_STATE_IDLE = 0,
	TWT_REQ_STATE_REQTX,
	TWT_REQ_STATE_WAIT_RSP,
	TWT_REQ_STATE_SUSPENDING,
	TWT_REQ_STATE_SUSPENDED,
	TWT_REQ_STATE_RESUMING,
	TWT_REQ_STATE_TEARING_DOWN,
	TWT_REQ_STATE_RX_TEARDOWN,
	TWT_REQ_STATE_RX_INFOFRM,
#if (CFG_SUPPORT_BTWT == 1)
	TWT_REQ_STATE_REQTX_BTWT,
	TWT_REQ_STATE_TEARING_DOWN_BTWT,
	TWT_REQ_STATE_RX_TEARDOWN_BTWT,
#endif
#if (CFG_SUPPORT_RTWT == 1)
	TWT_REQ_STATE_REQTX_RTWT,
	TWT_REQ_STATE_TEARING_DOWN_RTWT,
	TWT_REQ_STATE_RX_TEARDOWN_RTWT,
#endif
#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	TWT_REQ_STATE_REQTX_ML_TWT_ALL_LINKS,
	TWT_REQ_STATE_REQTX_ML_TWT_ONE_BY_ONE,
#endif
	TWT_REQ_STATE_NUM
};

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
enum _ENUM_TWT_HOTSPOT_RESPONDER_STATE_T {
	TWT_HOTSPOT_RESP_STATE_IDLE = 0,
	TWT_HOTSPOT_RESP_STATE_RECEIVE_SETUP,
	TWT_HOTSPOT_RESP_STATE_SETUP_RESPONSE,
	TWT_HOTSPOT_RESP_STATE_RECEIVE_TEARDOWN,
	TWT_HOTSPOT_RESP_STATE_SEND_TEARDOWN_TO_STA,
	TWT_HOTSPOT_RESP_STATE_DISCONNECT,
	TWT_HOTSPOT_RESP_STATE_IDLE_BY_FORCE,
	TWT_HOTSPOT_RESP_STATE_NUM
};
#endif

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
enum _ENUM_TWT_CNM_STATE_T {
	TWT_CNM_STATE_DEFAULT = 0,
	TWT_CNM_STATE_WAIT_RESP,
	TWT_CNM_STATE_ADD_AGRT,
	TWT_CNM_STATE_ABORT
};
#endif
/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

u_int32_t
twtReqFsmRunEventTxDone(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus);

void twtReqFsmRunEventRxSetup(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	enum _ENUM_TWT_TYPE_T eTwtType);

void twtReqFsmRunEventRxTeardown(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId);

void twtReqFsmRunEventRxInfoFrm(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _NEXT_TWT_INFO_T *prNextTWTInfo);

void twtReqFsmRunEventStart(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void twtReqFsmRunEventTeardown(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void twtReqFsmRunEventSuspend(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void twtReqFsmRunEventResume(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
uint32_t
twtHotspotRespFsmSendEvent(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	enum ENUM_MSG_ID eMsgId);

void
twtHotspotRespFsmSteps(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _ENUM_TWT_HOTSPOT_RESPONDER_STATE_T eNextState,
	uint8_t ucTWTFlowId,
	void *pParam);

void
twtHotspotRespFsmRunEventRxSetup(
	struct ADAPTER *prAdapter,
	void *pParam);

u_int32_t
twtHotspotRespFsmRunEventTxDone(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus);
#endif

#if (CFG_SUPPORT_BTWT == 1)
void btwtReqFsmRunEventStart(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void btwtReqFsmRunEventTeardown(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);
#endif

#if (CFG_SUPPORT_RTWT == 1)
void rtwtReqFsmRunEventStart(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void rtwtReqFsmRunEventTeardown(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
void mltwtReqFsmRunEventStartAllLinks(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void mltwtReqFsmRunEventStart(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void mltwtReqFsmRunEventRxSetup(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t *pucIE,
	uint16_t u2IELength);

void mltwtReqFsmSync(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _ENUM_TWT_REQUESTER_STATE_T eNextState,
	u_int8_t ucTWTFlowId);
#endif

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
u_int32_t
twtReqFsmRunEventRejectTxDone(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus);

void twtReqFsmSetupTimeoutInit(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _ENUM_TWT_CNM_STATE_T eCurState,
	u_int8_t ucTWTFlowId,
	enum _ENUM_TWT_TYPE_T *preTwtType);

void twtReqFsmSetupTimeoutDeInit(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	enum _ENUM_TWT_TYPE_T *preTwtType);

void twtReqFsmSetupTimeoutStateCfg(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _ENUM_TWT_CNM_STATE_T eCurState,
	u_int8_t ucTWTFlowId,
	enum _ENUM_TWT_TYPE_T *preTwtType);

void twtReqFsmSetupEventTimeout(
	struct ADAPTER *prAdapter,
	uintptr_t ulParamPtr);

void twtReqFsmTeardownTimeoutInit(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	enum _ENUM_TWT_TYPE_T *preTwtType);

void twtReqFsmTeardownTimeoutDeInit(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	enum _ENUM_TWT_TYPE_T *preTwtType);

void twtReqFsmTeardownEventTimeout(
	struct ADAPTER *prAdapter,
	uintptr_t ulParamPtr);
#endif /* #if (CFG_SUPPORT_TWT_STA_CNM == 1) */

#endif /* _TWT_REQ_FSM_H */
