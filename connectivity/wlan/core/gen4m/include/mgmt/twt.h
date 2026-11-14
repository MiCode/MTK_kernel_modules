/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _TWT_H
#define _TWT_H

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
#define TWT_INCORRECT_FLOW_ID   0xFF
#define TWT_MAX_FLOW_NUM        8
#define RTWT_MAX_FLOW_NUM       32
#define TWT_MAX_WAKE_INTVAL_EXP (TWT_REQ_TYPE_TWT_WAKE_INTVAL_EXP >> \
	TWT_REQ_TYPE_TWT_WAKE_INTVAL_EXP_OFFSET)

#define TWT_ROLE_STA			0
#define TWT_ROLE_AP			1
#define TWT_ROLE_APCLI			2
#define TWT_ROLE_HOTSPOT		3
#define TWT_ROLE_STA_LOCAL_EMU		4

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
#define TWT_HOTSPOT_TSF_ALIGNMENT_EN 1

/* 16TU = 16*1024usec*/
#define TWT_HOTSPOT_TSF_ALIGNMNET_UINT		(16 * 1024)
#endif

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
enum _ENUM_TWT_SMART_STA_STATE_T {
	TWT_SMART_STA_STATE_IDLE = 0,
	TWT_SMART_STA_STATE_REQUESTING = 1,
	TWT_SMART_STA_STATE_SUCCESS = 2,
	TWT_SMART_STA_STATE_FAIL = 3
};

struct _TWT_SMART_STA_T {
	u_int8_t fgTwtSmartStaReq;
	u_int8_t fgTwtSmartStaActivated;
	u_int8_t fgTwtSmartStaTeardownReq;
	uint8_t  ucBssIndex;
	uint8_t  ucFlowId;
	uint32_t u4CurTp;
	uint32_t u4LastTp;
	uint32_t u4Count;
	uint32_t u4TwtSwitch;
	enum _ENUM_TWT_SMART_STA_STATE_T eState;
};

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

/* Macros for setting request type bit fields in TWT IE */
#define SET_TWT_RT_REQUEST(fgReq) \
	(((fgReq) << TWT_REQ_TYPE_TWT_REQUEST_OFFSET) & \
		TWT_REQ_TYPE_TWT_REQUEST)

#define SET_TWT_RT_SETUP_CMD(ucSetupCmd) \
	(((ucSetupCmd) << TWT_REQ_TYPE_TWT_SETUP_COMMAND_OFFSET) & \
		TWT_REQ_TYPE_TWT_SETUP_COMMAND)

#define SET_TWT_RT_TRIGGER(fgTrigger) \
	(((fgTrigger) << TWT_REQ_TYPE_TRIGGER_OFFSET) & TWT_REQ_TYPE_TRIGGER)

#define SET_TWT_RT_FLOW_TYPE(fgUnannounced) \
	(((fgUnannounced) << TWT_REQ_TYPE_FLOWTYPE_OFFSET) & \
		TWT_REQ_TYPE_FLOWTYPE)

#define SET_TWT_RT_FLOW_ID(ucTWTFlowId) \
	(((ucTWTFlowId) << TWT_REQ_TYPE_TWT_FLOW_IDENTIFIER_OFFSET) & \
		TWT_REQ_TYPE_TWT_FLOW_IDENTIFIER)

#define SET_TWT_RT_WAKE_INTVAL_EXP(ucWakeIntvlExponent) \
	(((ucWakeIntvlExponent) << TWT_REQ_TYPE_TWT_WAKE_INTVAL_EXP_OFFSET) & \
		TWT_REQ_TYPE_TWT_WAKE_INTVAL_EXP)

#define SET_TWT_RT_PROTECTION(fgProtect) \
	(((fgProtect) << TWT_REQ_TYPE_TWT_PROTECTION_OFFSET) & \
		TWT_REQ_TYPE_TWT_PROTECTION)

/* Macros for getting request type bit fields in TWT IE */
#define GET_TWT_RT_REQUEST(u2ReqType) \
	(((u2ReqType) & TWT_REQ_TYPE_TWT_REQUEST) >> \
		TWT_REQ_TYPE_TWT_REQUEST_OFFSET)

#define GET_TWT_RT_SETUP_CMD(u2ReqType) \
	(((u2ReqType) & TWT_REQ_TYPE_TWT_SETUP_COMMAND) >> \
		TWT_REQ_TYPE_TWT_SETUP_COMMAND_OFFSET)

#define GET_TWT_RT_TRIGGER(u2ReqType) \
	(((u2ReqType) & TWT_REQ_TYPE_TRIGGER) >> TWT_REQ_TYPE_TRIGGER_OFFSET)

#define GET_TWT_RT_FLOW_TYPE(u2ReqType) \
	(((u2ReqType) & TWT_REQ_TYPE_FLOWTYPE) >> TWT_REQ_TYPE_FLOWTYPE_OFFSET)

#define GET_TWT_RT_FLOW_ID(u2ReqType) \
	(((u2ReqType) & TWT_REQ_TYPE_TWT_FLOW_IDENTIFIER) >> \
		TWT_REQ_TYPE_TWT_FLOW_IDENTIFIER_OFFSET)

#define GET_TWT_RT_WAKE_INTVAL_EXP(u2ReqType) \
	(((u2ReqType) & TWT_REQ_TYPE_TWT_WAKE_INTVAL_EXP) >> \
		TWT_REQ_TYPE_TWT_WAKE_INTVAL_EXP_OFFSET)

#define GET_TWT_RT_PROTECTION(u2ReqType) \
	(((u2ReqType) & TWT_REQ_TYPE_TWT_PROTECTION) >> \
		TWT_REQ_TYPE_TWT_PROTECTION_OFFSET)

/* Macros to set TWT info field */
#define SET_TWT_INFO_FLOW_ID(ucNextTWTCtrl) \
	(((ucNextTWTCtrl) << TWT_INFO_FLOW_ID_OFFSET) & TWT_INFO_FLOW_ID)

#define SET_TWT_INFO_RESP_REQUESTED(ucNextTWTCtrl) \
	(((ucNextTWTCtrl) << TWT_INFO_RESP_REQUESTED_OFFSET) & \
	TWT_INFO_RESP_REQUESTED)

#define SET_TWT_INFO_NEXT_TWT_REQ(ucNextTWTCtrl) \
	(((ucNextTWTCtrl) << TWT_INFO_NEXT_TWT_REQ_OFFSET) & \
	TWT_INFO_NEXT_TWT_REQ)

#define SET_TWT_INFO_NEXT_TWT_SIZE(ucNextTWTCtrl) \
	(((ucNextTWTCtrl) << TWT_INFO_NEXT_TWT_SIZE_OFFSET) & \
	TWT_INFO_NEXT_TWT_SIZE)

#define SET_TWT_INFO_BCAST_RESCHED(ucNextTWTCtrl) \
	(((ucNextTWTCtrl) << TWT_INFO_BCAST_RESCHED_OFFSET) & \
	TWT_INFO_BCAST_RESCHED)

/* Macros to get TWT info field */
#define GET_TWT_INFO_FLOW_ID(ucNextTWTCtrl) \
	(((ucNextTWTCtrl) & TWT_INFO_FLOW_ID) >> TWT_INFO_FLOW_ID_OFFSET)

#define GET_TWT_INFO_RESP_REQUESTED(ucNextTWTCtrl) \
	(((ucNextTWTCtrl) & TWT_INFO_RESP_REQUESTED) >> \
	TWT_INFO_RESP_REQUESTED_OFFSET)

#define GET_TWT_INFO_NEXT_TWT_REQ(ucNextTWTCtrl) \
	(((ucNextTWTCtrl) & TWT_INFO_NEXT_TWT_REQ) >> \
	TWT_INFO_NEXT_TWT_REQ_OFFSET)

#define GET_TWT_INFO_NEXT_TWT_SIZE(ucNextTWTCtrl) \
	(((ucNextTWTCtrl) & TWT_INFO_NEXT_TWT_SIZE) >> \
	TWT_INFO_NEXT_TWT_SIZE_OFFSET)

#define GET_TWT_INFO_BCAST_RESCHED(ucNextTWTCtrl) \
	(((ucNextTWTCtrl) & TWT_INFO_BCAST_RESCHED) >> \
	TWT_INFO_BCAST_RESCHED_OFFSET)

/* Next TWT from the packet should be little endian */
#define GET_48_BITS_NEXT_TWT_FROM_PKT(pMem) \
	((u_int64_t)(*((u_int8_t *)(pMem))) | \
	((u_int64_t)(*(((u_int8_t *)(pMem)) + 1)) << 8) | \
	((u_int64_t)(*(((u_int8_t *)(pMem)) + 2)) << 16) | \
	((u_int64_t)(*(((u_int8_t *)(pMem)) + 3)) << 24) | \
	((u_int64_t)(*(((u_int8_t *)(pMem)) + 4)) << 32) | \
	((u_int64_t)(*(((u_int8_t *)(pMem)) + 5)) << 40))

#define GET_TWT_TEARDOWN_NEGO(ucFlowId) \
	(((ucFlowId) & TWT_TEARDOWN_NEGO) >> TWT_TEARDOWN_NEGO_OFFSET)

#define SET_TWT_TEARDOWN_NEGO(ucFlowId) \
	(((ucFlowId) << TWT_TEARDOWN_NEGO_OFFSET) & TWT_TEARDOWN_NEGO)

#define GET_TWT_TEARDOWN_ALL(ucFlowId) \
	(((ucFlowId) & TWT_TEARDOWN_ALL) >> TWT_TEARDOWN_ALL_OFFSET)

#if (CFG_SUPPORT_BTWT == 1)
#define SET_BTWT_RECOMMENDATION(ucRecomm) \
	(((ucRecomm) << BTWT_REQ_TYPE_RECOMMENDATION_OFFSET) & \
		BTWT_REQ_TYPE_RECOMMENDATION)

#define GET_BTWT_RECOMMENDATION(ucRecomm) \
	(((ucRecomm) & BTWT_REQ_TYPE_RECOMMENDATION) >> \
		BTWT_REQ_TYPE_RECOMMENDATION_OFFSET)

#define SET_BTWT_RESERVED(fgReserved) \
	(((fgReserved) << BTWT_REQ_TYPE_RESERVED_OFFSET) & \
		BTWT_REQ_TYPE_RESERVED)

#define SET_BTWT_CTRL_NEGO(ucNego) \
	(((ucNego) << BTWT_CTRL_NEGOTIATION_OFFSET) & \
	BTWT_CTRL_NEGOTIATION)

#define GET_BTWT_CTRL_NEGO(ucNego) \
	(((ucNego) & BTWT_CTRL_NEGOTIATION) >> \
	BTWT_CTRL_NEGOTIATION_OFFSET)

#define SET_BTWT_ID(ucBrdInfo) \
	(((ucBrdInfo) << BTWT_INFO_BROADCAST_OFFSET) & \
	BTWT_INFO_BROADCAST)

#define GET_BTWT_ID(ucBrdInfo) \
	(((ucBrdInfo) & BTWT_INFO_BROADCAST) >> \
	BTWT_INFO_BROADCAST_OFFSET)

#define SET_BTWT_PERSISTENCE(ucPersistence) \
	(((ucPersistence) << BTWT_INFO_PERSISTENCE_OFFSET) & \
	BTWT_INFO_PERSISTENCE)

#define GET_BTWT_PERSISTENCE(ucPersistence) \
	(((ucPersistence) & BTWT_INFO_PERSISTENCE) >> \
	BTWT_INFO_PERSISTENCE_OFFSET)

#define GET_BTWT_LAST_BCAST(ucLastParm) \
	(((ucLastParm) & BTWT_REQ_TYPE_LAST_BCAST_PARAM) >> \
	BTWT_REQ_TYPE_LAST_BCAST_PARAM_OFFSET)

#endif

#if (CFG_SUPPORT_RTWT == 1)
#define SET_BTWT_RTWT_TRAFFIC_INFO_PRESENT(ucPresent) \
	(((ucPresent) << BTWT_INFO_RTWT_TRAFFIC_INFO_PRESENT_OFFSET) & \
		BTWT_INFO_RTWT_TRAFFIC_INFO_PRESENT)

#define GET_BTWT_RTWT_TRAFFIC_INFO_PRESENT(ucBrdInfo) \
	(((ucBrdInfo) & BTWT_INFO_RTWT_TRAFFIC_INFO_PRESENT) >> \
		BTWT_INFO_RTWT_TRAFFIC_INFO_PRESENT_OFFSET)

#define SET_BTWT_RTWT_SCHEDULE_INFO(u2Schedule) \
	(((u2Schedule) << BTWT_INFO_RTWT_SCHEDULE_INFO_OFFSET) & \
		BTWT_INFO_RTWT_SCHEDULE_INFO)

#define GET_BTWT_RTWT_SCHEDULE_INFO(ucBrdInfo) \
	(((ucBrdInfo) & BTWT_INFO_RTWT_SCHEDULE_INFO) >> \
		BTWT_INFO_RTWT_SCHEDULE_INFO_OFFSET)

#define SET_RTWT_TRAFFIC_INFO_DL_TID_BITMAP_VALID(_Traffic_Info) \
	(((_Traffic_Info) << RTWT_TRAFFIC_INFO_DL_TID_BITMAP_VALID_OFFSET) & \
		RTWT_TRAFFIC_INFO_DL_TID_BITMAP_VALID)
#define GET_RTWT_TRAFFIC_INFO_DL_TID_BITMAP_VALID(_Traffic_Info) \
	(((_Traffic_Info) & RTWT_TRAFFIC_INFO_DL_TID_BITMAP_VALID) >> \
			RTWT_TRAFFIC_INFO_DL_TID_BITMAP_VALID_OFFSET)
#define SET_RTWT_TRAFFIC_INFO_UL_TID_BITMAP_VALID(_Traffic_Info) \
	(((_Traffic_Info) << RTWT_TRAFFIC_INFO_UL_TID_BITMAP_VALID_OFFSET) & \
		RTWT_TRAFFIC_INFO_UL_TID_BITMAP_VALID)
#define GET_RTWT_TRAFFIC_INFO_UL_TID_BITMAP_VALID(_Traffic_Info) \
	(((_Traffic_Info) & RTWT_TRAFFIC_INFO_UL_TID_BITMAP_VALID) >> \
		RTWT_TRAFFIC_INFO_UL_TID_BITMAP_VALID_OFFSET)

#define SET_RTWT_TRAFFIC_INFO_DL_TID_BITMAP(_Traffic_Info) \
	(((_Traffic_Info) << RTWT_TRAFFIC_INFO_DL_TID_BITMAP_OFFSET) & \
		RTWT_TRAFFIC_INFO_DL_TID_BITMAP)
#define GET_RTWT_TRAFFIC_INFO_DL_TID_BITMAP(_Traffic_Info) \
	(((_Traffic_Info) & RTWT_TRAFFIC_INFO_DL_TID_BITMAP) >> \
		RTWT_TRAFFIC_INFO_DL_TID_BITMAP_VALID_OFFSET)
#define SET_RTWT_TRAFFIC_INFO_UL_TID_BITMAP(_Traffic_Info) \
	(((_Traffic_Info) << RTWT_TRAFFIC_INFO_UL_TID_BITMAP_OFFSET) & \
		RTWT_TRAFFIC_INFO_UL_TID_BITMAP)
#define GET_RTWT_TRAFFIC_INFO_UL_TID_BITMAP(_Traffic_Info) \
	(((_Traffic_Info) & RTWT_TRAFFIC_INFO_UL_TID_BITMAP) >> \
		RTWT_TRAFFIC_INFO_UL_TID_BITMAP_OFFSET)
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
#define IE_ML_TWT_LENGTH sizeof(struct IE_ML_TWT_T)

#define ML_TWT_LINK_ID_BITMAP_COUNT 16

#define ML_TWT_CTRL_LINK_ID_BITMAP BIT(6)
#define ML_TWT_CTRL_LINK_ID_BITMAP_OFFSET 6

#define GET_ML_TWT_CTRL_LINK_ID_BITMAP(ucCtrl) \
	(((ucCtrl) & ML_TWT_CTRL_LINK_ID_BITMAP) >> \
	ML_TWT_CTRL_LINK_ID_BITMAP_OFFSET)

#define SET_ML_TWT_CTRL_LINK_ID_BITMAP(ucCtrl) \
	(((ucCtrl) << ML_TWT_CTRL_LINK_ID_BITMAP_OFFSET) & \
	ML_TWT_CTRL_LINK_ID_BITMAP)
#endif

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

void twtProcessS1GAction(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);

uint32_t twtSendSetupFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);

uint32_t twtSendTeardownFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);

uint32_t twtSendInfoFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _NEXT_TWT_INFO_T *prNextTWTInfo,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);

u_int8_t twtGetTxSetupFlowId(
	struct MSDU_INFO *prMsduInfo);

uint8_t twtGetRxSetupFlowId(
	struct _IE_TWT_T *prTWTIE);

u_int8_t twtGetTxTeardownFlowId(
	struct MSDU_INFO *prMsduInfo);

uint8_t twtGetTxInfoFlowId(
	struct MSDU_INFO *prMsduInfo);

static inline u_int8_t twtGetNextTWTByteCnt(u_int8_t ucNextTWTSize)
{
	return (ucNextTWTSize == NEXT_TWT_SUBFIELD_64_BITS) ? 8 :
		((ucNextTWTSize == NEXT_TWT_SUBFIELD_32_BITS) ? 4 :
		((ucNextTWTSize == NEXT_TWT_SUBFIELD_48_BITS) ? 6 : 0));
}

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
void
twtHotspotGetFreeFlowId(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t *p_ucTWTFlowId);

void
twtHotspotReturnFlowId(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId);

void
twtHotspotGetStaRecIndexByFlowId(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	uint8_t ucTWTFlowId,
	uint8_t *p_ucIndex);

void
twtHotspotGetStaRecByFlowId(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	uint8_t ucTWTFlowId,
	struct STA_RECORD **pprStaRec
);

void
twtHotspotGetFreeStaNodeIndex(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t *p_ucIndex);

void
twtHotspotGetFreeStaNode(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct _TWT_HOTSPOT_STA_NODE **pprTWTHotspotStaNode);

void
twtHotspotResetStaNode(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

uint32_t
twtHotspotAlignDuration(
	uint32_t sp_duration,
	uint32_t alignment);

void
twtHotspotGetNearestTargetTSF(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode,
	uint64_t u8CurrentTsf);

uint32_t
twtHotspotSendSetupRespFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	uint8_t ucDialogToken,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);
#endif

#if (CFG_SUPPORT_BTWT == 1)
void btwtFillTWTElement(
	struct _IE_BTWT_T *prTWTBuf,
	uint8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams);

uint32_t btwtSendSetupFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);

uint32_t btwtSendTeardownFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);

uint8_t btwtGetTxSetupFlowId(
	struct MSDU_INFO *prMsduInfo);
#endif

#if (CFG_SUPPORT_RTWT == 1)
void rtwtFillTWTElement(
	struct _IE_RTWT_T *prTWTBuf,
	uint8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	uint8_t ucSetupFrameByteLength);

uint32_t rtwtSendSetupFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);

uint32_t rtwtSendTeardownFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	u_int8_t fgTeardownAll,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);

uint8_t rtwtGetTxSetupFlowId(
	struct MSDU_INFO *prMsduInfo);

void rtwtParseTWTElement(
	struct _IE_RTWT_T *prRTWTIE,
	struct _TWT_PARAMS_T *prTWTParams);
#endif


#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
uint32_t mltwtParseTWTElement(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t *pucIE,
	uint16_t u2IELength);

uint32_t mltwtFillTWTElementAllInOne(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct IE_ML_TWT_T *prMLTWTBuf,
	uint8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams);

uint32_t mltwtSendSetupFrameAllInOne(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);

uint8_t mltwtGetLinkCount(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t ucTWTFlowId);

uint32_t mltwtFillTWTElementPerLinkDistinct(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t *pucIE,
	uint16_t u2IELength,
	uint8_t ucTWTFlowId);

uint32_t mltwtSendSetupFramePerLinkDistinct(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTWTFlowId,
	struct _TWT_PARAMS_T *prTWTParams,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);
#endif
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _TWT_H */
