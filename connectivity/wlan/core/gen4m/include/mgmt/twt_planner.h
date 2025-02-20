/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _TWT_PLANNER_H
#define _TWT_PLANNER_H

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
#if (CFG_SUPPORT_TWT_STA_CNM == 1)
#define TWT_CNM_GRANT_DEFAULT_INTERVAL_MS 256
#endif
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
#if (CFG_SUPPORT_BTWT == 1)
enum _ENUM_BTWT_FLOW_STATE_T {
	ENUM_BTWT_FLOW_STATE_DEFAULT = 0,
	ENUM_BTWT_FLOW_STATE_REQUESTING,
	ENUM_BTWT_FLOW_STATE_ACTIVATED,
};
#endif

struct _TWT_FLOW_T {
	struct _TWT_PARAMS_T rTWTParams;
	struct _TWT_PARAMS_T rTWTPeerParams;
	u_int64_t u8NextTWT;
#if (CFG_SUPPORT_BTWT == 1)
	enum _ENUM_BTWT_FLOW_STATE_T eBtwtState;
#endif
#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	uint8_t fgIsMLTWT;
#endif
	enum _ENUM_TWT_TYPE_T eTwtType;
#if (CFG_SUPPORT_TWT_STA_CNM == 1)
	enum _ENUM_TWT_CNM_STATE_T eTwtCnmState;
	struct STA_RECORD *prOwnStaRec;
	u_int8_t ucTWTFlowId;
#endif
};

struct _TWT_AGRT_T {
	u_int8_t fgValid;
	u_int8_t ucAgrtTblIdx;
	u_int8_t ucBssIdx;
	u_int8_t ucFlowId;
	struct _TWT_PARAMS_T rTWTAgrt;
	enum _ENUM_TWT_TYPE_T eTwtType;
};

struct _TWT_PLANNER_T {
	struct _TWT_AGRT_T arTWTAgrtTbl[TWT_AGRT_MAX_NUM];
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

#define TSF_OFFSET_FOR_EMU	   (1 * 1000 * 1000)	/* after 1 sec */
#define TSF_OFFSET_FOR_AGRT_ADD	   (5 * 1000 * 1000)	/* after 5 sec */
#define TSF_OFFSET_FOR_AGRT_RESUME (5 * 1000 * 1000)	/* after 5 sec */

/* Definitions for action control of TWT params */
enum {
	TWT_PARAM_ACTION_NONE = 0,
	TWT_PARAM_ACTION_ADD_BYPASS = 1, /* bypass nego & add an agrt */
	TWT_PARAM_ACTION_DEL_BYPASS = 2, /* bypass proto & del an agrt */
	TWT_PARAM_ACTION_MOD_BYPASS = 3, /* bypass proto & modify an agrt */
	TWT_PARAM_ACTION_ADD = 4,
	TWT_PARAM_ACTION_DEL = 5,
	TWT_PARAM_ACTION_SUSPEND = 6,
	TWT_PARAM_ACTION_RESUME = 7,
	TWT_PARAM_ACTION_TESTBED_CONFIG = 8,
	TWT_PARAM_ACTION_ADD_BTWT = 9,
	TWT_PARAM_ACTION_ENABLE_ITWT = 10,
	TWT_PARAM_ACTION_ENABLE_BTWT = 11,
	TWT_PARAM_ACTION_ENABLE_INF_FRAME = 12,
	TWT_PARAM_ACTION_ADD_ML_TWT_ALL_LINKS = 13,
	TWT_PARAM_ACTION_ADD_ML_TWT_ONE_BY_ONE = 14,
	TWT_PARAM_ACTION_ADD_RTWT = 15,
	TWT_PARAM_ACTION_JOIN_RTWT = 16,
	TWT_PARAM_ACTION_ENABLE_RTWT = 17,
	TWT_PARAM_ACTION_MAX
};

#define IS_TWT_PARAM_ACTION_ADD_BYPASS(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_ADD_BYPASS)
#define IS_TWT_PARAM_ACTION_DEL_BYPASS(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_DEL_BYPASS)
#define IS_TWT_PARAM_ACTION_MOD_BYPASS(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_MOD_BYPASS)
#define IS_TWT_PARAM_ACTION_ADD(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_ADD)
#define IS_TWT_PARAM_ACTION_DEL(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_DEL)
#define IS_TWT_PARAM_ACTION_SUSPEND(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_SUSPEND)
#define IS_TWT_PARAM_ACTION_RESUME(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_RESUME)
#define IS_TWT_PARAM_ACTION_TESTBED_CONFIG(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_TESTBED_CONFIG)
#define IS_TWT_PARAM_ACTION_ADD_BTWT(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_ADD_BTWT)
#define IS_TWT_PARAM_ACTION_ENABLE_ITWT(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_ENABLE_ITWT)
#define IS_TWT_PARAM_ACTION_ENABLE_BTWT(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_ENABLE_BTWT)
#define IS_TWT_PARAM_ACTION_ENABLE_INF_FRAME(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_ENABLE_INF_FRAME)
#define IS_TWT_PARAM_ACTION_ADD_ML_TWT_ALL_LINKS(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_ADD_ML_TWT_ALL_LINKS)
#define IS_TWT_PARAM_ACTION_ADD_ML_TWT_ONE_BY_ONE(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_ADD_ML_TWT_ONE_BY_ONE)
#define IS_TWT_PARAM_ACTION_ADD_RTWT(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_ADD_RTWT)
#define IS_TWT_PARAM_ACTION_JOIN_RTWT(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_JOIN_RTWT)
#define IS_TWT_PARAM_ACTION_ENABLE_RTWT(ucCtrlAction) \
	((ucCtrlAction) == TWT_PARAM_ACTION_ENABLE_RTWT)

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void twtPlannerSetParams(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

uint32_t twtPlannerSendReqTeardown(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId);

uint32_t twtPlannerReset(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

#if (CFG_TWT_STA_DIRECT_TEARDOWN == 1)
void twtPlannerTearingdown(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId,
	uint8_t *p_fgByPassNego);
#endif

void twtPlannerRxNegoResult(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void twtPlannerSuspendDone(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void twtPlannerTeardownDone(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void twtPlannerResumeDone(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void twtPlannerFillResumeData(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId,
	uint64_t u8NextTWT);

uint32_t
twtPlannerResumeAgrtTbl(struct ADAPTER *prAdapter,
			struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
			uint8_t ucFlowId, uint8_t fgIsOid,
			PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
			PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler);

void twtPlannerRxInfoFrm(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void twtPlannerGetTsfDone(
	struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf);

struct _TWT_FLOW_T *twtPlannerFlowFindById(
	struct STA_RECORD *prStaRec, uint8_t ucFlowId,
	enum _ENUM_TWT_TYPE_T eTwtType);

enum _ENUM_TWT_TYPE_T
twtPlannerDrvAgrtGetTwtTypeByIndex(
	struct ADAPTER *prAdapter, uint8_t ucAgrtIdx);

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
void twtPlannerGetCnmGrantedDone(
	struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf);

uint32_t twtPlannerAbortCnmGranted(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId, uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler);

void twtGetCurrentTsfTimeoutInit(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _TWT_GET_TSF_REASON ucReason);

void twtGetCurrentTsfTimeoutDeInit(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum _TWT_GET_TSF_REASON ucReason);

void twtGetCurrentTsfTimeout(
	struct ADAPTER *prAdapter,
	uintptr_t ulParamPtr);
#endif

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
void twtHotspotPlannerSetParams(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void twtHotspotPlannerSetupAgrtToFW(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void twtHotspotPlannerTeardownToFW(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

uint32_t twtHotspotPlannerGetCurrentTSF(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen);

void twtHotspotPlannerGetTsfDone(
	struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf);

uint32_t
twtHotspotPlannerAddAgrtTbl(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId,
	uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler);

uint32_t
twtHotspotPlannerTeardownSta(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId,
	uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler);
#endif

#if (CFG_SUPPORT_BTWT == 1)
uint32_t btwtPlannerSendReqStart(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId);

uint32_t btwtPlannerSendReqTeardown(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId);

void btwtPlannerTeardownDone(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

uint32_t btwtPlannerAddAgrtTbl(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct _TWT_PARAMS_T *prTWTParams,
	uint8_t ucFlowId,
	uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler);

void btwtPlannerDelAgrtTbl(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId);
#endif

#if (CFG_SUPPORT_RTWT == 1)
uint32_t rtwtPlannerSendReqStart(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	enum ENUM_MSG_ID eMsgId,
	uint8_t ucTWTFlowId);

uint32_t rtwtPlannerSendReqTeardown(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId,
	u_int8_t fgTeardownAll);

void rtwtPlannerTearingdown(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId);

void rtwtPlannerTeardownDone(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

uint32_t rtwtPlannerAddAgrtTbl(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct _TWT_PARAMS_T *prTWTParams,
	uint8_t ucFlowId,
	uint8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler);

void rtwtPlannerDelAgrtTbl(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	uint8_t ucFlowId);
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
uint32_t mltwtPlannerSendReqStartAllLinks(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId);

uint32_t mltwtPlannerSendReqStart(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId);

void mltwtPlannerRxNegoResult(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId);

void mltwtPlannerDelAgrtTbl(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId);
#endif

uint32_t
twtPlannerSendReqSuspend(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTWTFlowId);

uint8_t
twtPlannerDrvAgrtFind(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	uint8_t ucFlowId,
	uint8_t *pucFlowId);

uint32_t
twtPlannerGetCurrentTSF(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen);

uint8_t twtPlannerDrvAgrtFindWithTwtType(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	uint8_t ucFlowId,
	uint8_t fgByPassNego,
	enum _ENUM_TWT_TYPE_T eTwtType);

uint8_t twtPlannerDrvAgrtGetFlowID(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	uint8_t fgByPassNego,
	enum _ENUM_TWT_TYPE_T eTwtType);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _TWT_PLANNER_H */
