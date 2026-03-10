/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*
 * Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/cnm.c#2
 */

/*! \file   "cnm.c"
 *    \brief  Module of Concurrent Network Management
 *
 *    Module of Concurrent Network Management
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "wsys_cmd_handler_fw.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#if CFG_SUPPORT_DBDC
#define DBDC_ENABLE_GUARD_TIME		(4*1000)	/* ms */
#define DBDC_DISABLE_GUARD_TIME		(1*1000)	/* ms */
#define DBDC_DISABLE_COUNTDOWN_TIME	(2*1000)	/* ms */
#define DBDC_WMM_TX_QUOTA		    (0x90)
#endif /* CFG_SUPPORT_DBDC */

#if CFG_SUPPORT_IDC_CH_SWITCH
#define IDC_CSA_GUARD_TIME			(60)	/* 60 Sec */
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
#define RDD_OP_CHG_NO_ACT           (0)
#define RDD_OP_CHG_STOP_CAC         (1)
#endif

#define CNM_WMM_QUOTA_RETRIGGER_TIME_MS (200)	/* ms */
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
#if CFG_SUPPORT_DBDC
enum ENUM_DBDC_GUARD_TIMER_T {
	ENUM_DBDC_GUARD_TIMER_NONE,

	/* Prevent switch too quick */
	ENUM_DBDC_GUARD_TIMER_SWITCH_GUARD_TIME,

	/* Prevent continuously trigger by reconnection */
	ENUM_DBDC_GUARD_TIMER_DISABLE_COUNT_DOWN,

	ENUM_DBDC_GUARD_TIMER_NUM
};

enum ENUM_DBDC_FSM_STATE_T {
	ENUM_DBDC_FSM_STATE_DISABLE_IDLE,
	ENUM_DBDC_FSM_STATE_WAIT_PROTOCOL_ENABLE,
	ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE,
	ENUM_DBDC_FSM_STATE_ENABLE_GUARD,
	ENUM_DBDC_FSM_STATE_ENABLE_IDLE,
	ENUM_DBDC_FSM_STATE_WAIT_HW_DISABLE,
	ENUM_DBDC_FSM_STATE_DISABLE_GUARD,
	ENUM_DBDC_FSM_STATE_WAIT_PROTOCOL_DISABLE,
	ENUM_DBDC_FSM_STATE_NUM
};

enum ENUM_OPMODE_STATE_T {
	ENUM_OPMODE_STATE_DONE,
	ENUM_OPMODE_STATE_FAIL,
	ENUM_OPMODE_STATE_WAIT,
	ENUM_OPMODE_STATE_NUM
};

struct DBDC_INFO_T {
	enum ENUM_DBDC_FSM_STATE_T eDbdcFsmCurrState;
	enum ENUM_DBDC_FSM_STATE_T eDbdcFsmPrevState;
	enum ENUM_DBDC_FSM_STATE_T eDbdcFsmNextState;

	struct TIMER rDbdcGuardTimer;
	enum ENUM_DBDC_GUARD_TIMER_T eDdbcGuardTimerType;

	uint8_t fgReqPrivelegeLock;
	struct LINK rPendingMsgList;

	bool fgDbdcDisableOpmodeChangeDone;
	enum ENUM_OPMODE_STATE_T eBssOpModeState[BSSID_NUM];

	/* Set DBDC setting for incoming network */
	uint8_t ucPrimaryChannel;
	uint8_t ucWmmQueIdx;
	enum ENUM_BAND	eRfBand;

	/* Used for iwpriv to force enable DBDC*/
	bool fgHasSentCmd;
	bool fgCmdEn;

	/* Used to queue enter/leave A+G event */
	bool fgPostpondEnterAG;
	bool fgPostpondLeaveAG;

	/* For debug */
	OS_SYSTIME rPeivilegeLockTime;

	/* Used to indicated current support DBDCAAMode or not */
	bool fgIsDBDCAAMode;
	uint8_t ucBssIdx;
	u_int8_t fgIsDBDCEnByP2pLis;
};

enum ENUM_DBDC_FSM_EVENT_T {
	DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG,
	DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG,
	DBDC_FSM_EVENT_SWITCH_GUARD_TIME_TO,
	DBDC_FSM_EVENT_DISABLE_COUNT_DOWN_TO,
	DBDC_FSM_EVENT_ACTION_FRAME_ALL_SUCCESS,
	DBDC_FSM_EVENT_ACTION_FRAME_SOME_FAIL,
	DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE,
	DBDC_FSM_EVENT_ERR,
	DBDC_FSM_EVENT_NUM
};

enum ENUM_DBDC_PROTOCOL_STATUS_T {
	ENUM_DBDC_PROTOCOL_STATUS_DONE_SUCCESS = 0,
	ENUM_DBDC_PROTOCOL_STATUS_DONE_FAIL,
	ENUM_DBDC_PROTOCOL_STATUS_WAIT,
	ENUM_DBDC_PROTOCOL_STATUS_NUM
};

typedef void (*DBDC_ENTRY_FUNC)(struct ADAPTER *);
typedef void (*DBDC_EVENT_HNDL_FUNC)(struct ADAPTER *,
				     enum ENUM_DBDC_FSM_EVENT_T);
typedef void (*DBDC_EXIT_FUNC)(struct ADAPTER *);

struct DBDC_FSM_T {
	DBDC_ENTRY_FUNC pfEntryFunc;
	DBDC_EVENT_HNDL_FUNC pfEventHandlerFunc;
	DBDC_EXIT_FUNC pfExitFunc;
};
#endif /*CFG_SUPPORT_DBDC*/

enum ENUM_CNM_OPMODE_REQ_STATUS {
	CNM_OPMODE_REQ_STATUS_SUCCESS,
	CNM_OPMODE_REQ_STATUS_INVALID_PARAM,
	CNM_OPMODE_REQ_STATUS_RUNNING,
	CNM_OPMODE_REQ_STATUS_DEFER,
	CNM_OPMODE_REQ_STATUS_NUM
};

struct CNM_OPMODE_BSS_REQ {
	bool fgEnable;
	bool fgNewRequest;
	uint8_t ucOpRxNss;
	uint8_t ucOpTxNss;
};

struct CNM_OPMODE_BSS_RUNNING_REQ {
	/* Initiator */
	enum ENUM_CNM_OPMODE_REQ_T eReqIdx;
	/* Highest prioirty req */
	enum ENUM_CNM_OPMODE_REQ_T eRunReq;
	bool fgIsRunning;
	uint8_t ucOpRxNss;
	uint8_t ucOpTxNss;
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

enum ENUM_CNM_MODE {
	ENUM_CNM_MODE_MCC = 0,
	ENUM_CNM_MODE_SCC = 1,
	ENUM_CNM_MODE_MBMC = 2,
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if CFG_SUPPORT_IDC_CH_SWITCH
struct EVENT_LTE_SAFE_CHN g_rLteSafeChInfo;
#endif

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#if CFG_SUPPORT_DBDC
#define DBDC_SET_GUARD_TIME(_prAdapter, _u4TimeoutMs) { \
	cnmTimerStartTimer(_prAdapter, \
		&g_rDbdcInfo.rDbdcGuardTimer, \
		_u4TimeoutMs); \
	g_rDbdcInfo.eDdbcGuardTimerType = \
		ENUM_DBDC_GUARD_TIMER_SWITCH_GUARD_TIME; \
}

#define DBDC_SET_DISABLE_COUNTDOWN(_prAdapter) { \
	cnmTimerStartTimer(_prAdapter, \
		&g_rDbdcInfo.rDbdcGuardTimer, \
		DBDC_DISABLE_COUNTDOWN_TIME); \
	g_rDbdcInfo.eDdbcGuardTimerType = \
		ENUM_DBDC_GUARD_TIMER_DISABLE_COUNT_DOWN; \
	}

#define DBDC_FSM_MSG_WRONG_EVT(_eEvent) \
	log_dbg(CNM, WARN, \
		"[DBDC] Should not reveice evt %u during state %u\n", \
		_eEvent, \
		g_rDbdcInfo.eDbdcFsmCurrState)

#define DBDC_FSM_MSG_ERROR_EVT(_eEvent) \
	log_dbg(CNM, ERROR, "[DBDC] Reveice evt %u during state %u\n", \
		_eEvent, \
		g_rDbdcInfo.eDbdcFsmCurrState)

#define USE_DBDC_CAPABILITY() \
	((g_rDbdcInfo.eDbdcFsmCurrState \
		== ENUM_DBDC_FSM_STATE_WAIT_PROTOCOL_ENABLE || \
	g_rDbdcInfo.eDbdcFsmCurrState \
		== ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE || \
	g_rDbdcInfo.eDbdcFsmCurrState \
		== ENUM_DBDC_FSM_STATE_ENABLE_GUARD || \
	g_rDbdcInfo.eDbdcFsmCurrState \
		== ENUM_DBDC_FSM_STATE_ENABLE_IDLE)?TRUE:FALSE)

#define DBDC_SET_WMMBAND_FW_AUTO_BY_CHNL(_ucCh, _ucWmmQIdx, _eBand, _ucBId) \
	{ \
		g_rDbdcInfo.ucPrimaryChannel = (_ucCh);\
		g_rDbdcInfo.ucWmmQueIdx = (_ucWmmQIdx);\
		g_rDbdcInfo.eRfBand = (_eBand);\
		g_rDbdcInfo.ucBssIdx = (_ucBId);\
	}


#define DBDC_SET_WMMBAND_FW_AUTO_DEFAULT() \
	{ \
		g_rDbdcInfo.ucPrimaryChannel = 0; \
		g_rDbdcInfo.ucWmmQueIdx = 0;\
		g_rDbdcInfo.ucBssIdx = 0;\
		g_rDbdcInfo.eRfBand = BAND_NULL;\
	}

#define DBDC_UPDATE_CMD_WMMBAND_FW_AUTO(_prCmdBody) \
	{ \
		(_prCmdBody)->ucPrimaryChannel = g_rDbdcInfo.ucPrimaryChannel; \
		(_prCmdBody)->ucWmmQueIdx = g_rDbdcInfo.ucWmmQueIdx; \
		(_prCmdBody)->ucRfBand = g_rDbdcInfo.eRfBand; \
		DBDC_SET_WMMBAND_FW_AUTO_DEFAULT(); \
	}

#endif

#if CFG_SUPPORT_NAN
uint8_t g_ucNanWmmQueIdx;
#endif
/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static enum ENUM_CNM_OPMODE_REQ_STATUS
cnmOpModeSetTRxNss(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	enum ENUM_CNM_OPMODE_REQ_T eNewReq,
	bool fgEnable,
	uint8_t ucOpRxNss,
	uint8_t ucOpTxNss
);

static void
cnmWmmQuotaCallback(
	struct ADAPTER *prAdapter,
	uintptr_t plParamPtr
);


#if CFG_SUPPORT_DBDC
static void
cnmDbdcFsmEntryFunc_DISABLE_IDLE(
	struct ADAPTER *prAdapter
);

static void
cnmDbdcFsmEntryFunc_WAIT_PROTOCOL_ENABLE(
	struct ADAPTER *prAdapter
);

static void
cnmDbdcFsmEntryFunc_WAIT_HW_ENABLE(
	struct ADAPTER *prAdapter
);

static void
cnmDbdcFsmEntryFunc_ENABLE_GUARD(
	struct ADAPTER *prAdapter
);

static void
cnmDbdcFsmEntryFunc_WAIT_HW_DISABLE(
	struct ADAPTER *prAdapter
);

static void
cnmDbdcFsmEntryFunc_ENABLE_IDLE(
	struct ADAPTER *prAdapter
);

static void
cnmDbdcFsmEntryFunc_DISABLE_GUARD(
	struct ADAPTER *prAdapter
);

static void
cnmDbdcFsmEventHandler_DISABLE_IDLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T eEvent
);

static void
cnmDbdcFsmEventHandler_WAIT_PROTOCOL_ENABLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T eEvent
);

static void
cnmDbdcFsmEventHandler_WAIT_HW_ENABLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T eEvent
);

static void
cnmDbdcFsmEventHandler_ENABLE_GUARD(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T eEvent
);

static void
cnmDbdcFsmEventHandler_ENABLE_IDLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T eEvent
);

static void
cnmDbdcFsmEventHandler_WAIT_HW_DISABLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T eEvent
);

static void
cnmDbdcFsmEventHandler_DISABLE_GUARD(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T eEvent
);

static void
cnmDbdcFsmEventHandler_WAIT_PROTOCOL_DISABLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T eEvent
);

static void
cnmDbdcFsmExitFunc_WAIT_HW_ENABLE(
	struct ADAPTER *prAdapter
);

static void
cnmDbdcFsmExitFunc_WAIT_HW_DISABLE(
	struct ADAPTER *prAdapter
);

static void
cnmDbdcOpModeChangeDoneCallback(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	bool fgSuccess
);

static void
cnmWmmQuotaSetMaxQuota(
	struct ADAPTER *prAdapter,
	uint8_t ucWmmIndex,
	enum ENUM_CNM_WMM_QUOTA_REQ_T eNewReq,
	bool fgEnable,
	uint32_t u4ReqQuota
);

/*******************************************************************************
 *                           P R I V A T E   D A T A 2
 *******************************************************************************
 */
static struct DBDC_FSM_T arDdbcFsmActionTable[] = {
	/* ENUM_DBDC_FSM_STATE_DISABLE_IDLE */
	{
		cnmDbdcFsmEntryFunc_DISABLE_IDLE,
		cnmDbdcFsmEventHandler_DISABLE_IDLE,
		NULL
	},

	/* ENUM_DBDC_FSM_STATE_WAIT_PROTOCOL_ENABLE */
	{
		cnmDbdcFsmEntryFunc_WAIT_PROTOCOL_ENABLE,
		cnmDbdcFsmEventHandler_WAIT_PROTOCOL_ENABLE,
		NULL
	},

	/* ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE */
	{
		cnmDbdcFsmEntryFunc_WAIT_HW_ENABLE,
		cnmDbdcFsmEventHandler_WAIT_HW_ENABLE,
		cnmDbdcFsmExitFunc_WAIT_HW_ENABLE
	},

	/* ENUM_DBDC_FSM_STATE_ENABLE_GUARD */
	{
		cnmDbdcFsmEntryFunc_ENABLE_GUARD,
		cnmDbdcFsmEventHandler_ENABLE_GUARD,
		NULL
	},

	/* ENUM_DBDC_FSM_STATE_ENABLE_IDLE */
	{
		cnmDbdcFsmEntryFunc_ENABLE_IDLE,
		cnmDbdcFsmEventHandler_ENABLE_IDLE,
		NULL
	},

	/* ENUM_DBDC_FSM_STATE_WAIT_HW_DISABLE */
	{
		cnmDbdcFsmEntryFunc_WAIT_HW_DISABLE,
		cnmDbdcFsmEventHandler_WAIT_HW_DISABLE,
		cnmDbdcFsmExitFunc_WAIT_HW_DISABLE
	},

	/* ENUM_DBDC_FSM_STATE_DISABLE_GUARD */
	{
		cnmDbdcFsmEntryFunc_DISABLE_GUARD,
		cnmDbdcFsmEventHandler_DISABLE_GUARD,
		NULL
	},

	/* ENUM_DBDC_FSM_STATE_WAIT_PROTOCOL_DISABLE */
	{
		NULL,
		cnmDbdcFsmEventHandler_WAIT_PROTOCOL_DISABLE,
		NULL
	},
};

static struct DBDC_INFO_T g_rDbdcInfo;
#endif

#if CFG_SUPPORT_IDC_CH_SWITCH
OS_SYSTIME g_rLastCsaSysTime;
#endif

static struct CNM_OPMODE_BSS_CONTROL_T g_arBssOpControl[MAX_BSSID_NUM];
static uint8_t *apucCnmOpModeReq[CNM_OPMODE_REQ_MAX_CAP+1] = {
	(uint8_t *) DISP_STRING("ANT Ctrl"),
	(uint8_t *) DISP_STRING("DBDC"),
	(uint8_t *) DISP_STRING("DBDC Scan"),
	(uint8_t *) DISP_STRING("COEX"),
	(uint8_t *) DISP_STRING("SmartGear"),
	(uint8_t *) DISP_STRING("User"),
	(uint8_t *) DISP_STRING("SmartGear_1T2R"),
	(uint8_t *) DISP_STRING("ANT Ctrl_1T2R"),
	(uint8_t *) DISP_STRING("CoAnt"),
	(uint8_t *) DISP_STRING("RDD"),
	(uint8_t *) DISP_STRING("N/A"),
	(uint8_t *) DISP_STRING("MAX_CAP")
};

static uint8_t *apucCnmOpModeReqStatus[CNM_OPMODE_REQ_STATUS_NUM+1] = {
	(uint8_t *) DISP_STRING("Success"),
	(uint8_t *) DISP_STRING("Invalid"),
	(uint8_t *) DISP_STRING("Running"),
	(uint8_t *) DISP_STRING("Defer"),
	(uint8_t *) DISP_STRING("N/A")
};

static struct CNM_WMM_QUOTA_CONTROL_T g_arWmmQuotaControl[MAX_BSSID_NUM];
static uint8_t *apucCnmWmmQuotaReq[CNM_WMM_REQ_DEFAULT+1] = {
	(uint8_t *) DISP_STRING("DBDC"),
	(uint8_t *) DISP_STRING("N/A"),
	(uint8_t *) DISP_STRING("Default")
};

/*******************************************************************************
 *                                 M A C R O S 2
 *******************************************************************************
 */
#define DBDC_FSM_EVENT_HANDLER(_prAdapter, _event) { \
	if (g_rDbdcInfo.eDbdcFsmCurrState < 0 || \
		g_rDbdcInfo.eDbdcFsmCurrState >= ENUM_DBDC_FSM_STATE_NUM) { \
		log_dbg(CNM, WARN, \
		"[DBDC] eDbdcFsmCurrState %d is invalid!\n", \
		g_rDbdcInfo.eDbdcFsmCurrState); \
		return; \
	} \
	arDdbcFsmActionTable[g_rDbdcInfo.eDbdcFsmCurrState] \
	.pfEventHandlerFunc(_prAdapter, _event); \
	}

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to initialize variables in CNM_INFO_T.
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmInit(struct ADAPTER *prAdapter)
{
	struct CNM_OPMODE_BSS_CONTROL_T *prBssOpCtrl;
	struct CNM_WMM_QUOTA_CONTROL_T *prWmmQuotaCtrl;
	enum ENUM_CNM_OPMODE_REQ_T eReqIdx;
	enum ENUM_CNM_WMM_QUOTA_REQ_T eReqIdxWmm;
	uint8_t ucBssIndex, ucWmmIndex;

	ASSERT(prAdapter);

	if (prAdapter->ucHwBssIdNum > MAX_BSSID_NUM) {
		/* Unexpected! out of bounds access may happen... */
		DBGLOG(CNM, WARN,
			"HwBssNum(%d) > MAX_BSSID_NUM !!!\n",
			prAdapter->ucHwBssIdNum);
	}

	for (ucBssIndex = 0; ucBssIndex < prAdapter->ucHwBssIdNum;
		ucBssIndex++) {
		prBssOpCtrl = &(g_arBssOpControl[ucBssIndex]);
		prBssOpCtrl->rRunning.fgIsRunning = false;
		for (eReqIdx = CNM_OPMODE_REQ_START;
				eReqIdx < CNM_OPMODE_REQ_NUM; eReqIdx++)
			prBssOpCtrl->arReqPool[eReqIdx].fgEnable = false;
	}

	if (prAdapter->ucHwBssIdNum > MAX_BSSID_NUM ||
		prAdapter->ucWmmSetNum > MAX_BSSID_NUM) {
		/* Unexpected! out of bounds access may happen... */
		DBGLOG(CNM, WARN,
			"HwBssNum(%d)WmmNum(%d) > MAX_BSSID_NUM !!!\n",
			prAdapter->ucHwBssIdNum,
			prAdapter->ucWmmSetNum);
		ASSERT(0);
	}

	for (ucWmmIndex = 0; ucWmmIndex < prAdapter->ucWmmSetNum;
		ucWmmIndex++) {
		prWmmQuotaCtrl = &(g_arWmmQuotaControl[ucWmmIndex]);
		prWmmQuotaCtrl->rRunning.fgIsRunning = false;
		cnmTimerInitTimer(prAdapter,
			&(prWmmQuotaCtrl->rTimer),
			(PFN_MGMT_TIMEOUT_FUNC)
			cnmWmmQuotaCallback,
			(uintptr_t)
			ucWmmIndex);
		for (eReqIdxWmm = CNM_WMM_REQ_DBDC;
				eReqIdxWmm < CNM_WMM_REQ_NUM; eReqIdxWmm++)
			prWmmQuotaCtrl->arReqPool[eReqIdxWmm].fgEnable = false;
	}

#if CFG_SUPPORT_IDC_CH_SWITCH
	g_rLastCsaSysTime = 0;
#endif
}	/* end of cnmInit()*/

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to initialize variables in CNM_INFO_T.
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmUninit(struct ADAPTER *prAdapter)
{
	struct CNM_WMM_QUOTA_CONTROL_T *prWmmQuotaCtrl;
	uint8_t ucWmmIndex;
#if CFG_SUPPORT_DBDC
	cnmTimerStopTimer(prAdapter,
		&g_rDbdcInfo.rDbdcGuardTimer);
#endif
	for (ucWmmIndex = 0; ucWmmIndex < prAdapter->ucWmmSetNum;
		ucWmmIndex++) {
		prWmmQuotaCtrl = &(g_arWmmQuotaControl[ucWmmIndex]);
		cnmTimerStopTimer(prAdapter, &(prWmmQuotaCtrl->rTimer));
	}
}	/* end of cnmUninit()*/

/*----------------------------------------------------------------------------*/
/*!
 * @brief Before handle the message from other module, it need to obtain
 *        the Channel privilege from Channel Manager
 *
 * @param[in] prMsgHdr   The message need to be handled.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmChMngrRequestPrivilege(struct ADAPTER
			       *prAdapter,
			       struct MSG_HDR *prMsgHdr)
{
	struct MSG_CH_REQ *prMsgChReq;
	struct CMD_CH_PRIVILEGE *prCmdBody;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	uint32_t rStatus;
#if CFG_SUPPORT_DBDC
	OS_SYSTIME rChReqQueueTime;
#endif

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	prMsgChReq = (struct MSG_CH_REQ *)prMsgHdr;

#if CFG_SUPPORT_DBDC
	if (cnmDBDCIsReqPeivilegeLock()) {
		LINK_INSERT_TAIL(&g_rDbdcInfo.rPendingMsgList,
				 &prMsgHdr->rLinkEntry);
		log_dbg(CNM, INFO,
		       "[DBDC] ChReq: queued Token %u REQ\n",
		       prMsgChReq->ucTokenID);

		/* Trigger EE dump if PeivilegeLock was held for more than 5s */
		rChReqQueueTime = kalGetTimeTick();
		if ((g_rDbdcInfo.rPeivilegeLockTime != 0) &&
			(rChReqQueueTime > g_rDbdcInfo.rPeivilegeLockTime) &&
			((rChReqQueueTime -
				g_rDbdcInfo.rPeivilegeLockTime) > 5000)) {
			log_dbg(CNM, WARN,
				"[DBDC] ChReq: long peivilege lock at %d, %d\n",
				g_rDbdcInfo.rPeivilegeLockTime,
				rChReqQueueTime);
			GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_REQ_CHL_FAIL);
		}
		return;
	}
#endif

	prCmdBody = (struct CMD_CH_PRIVILEGE *)
		    cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
				sizeof(struct CMD_CH_PRIVILEGE));
	ASSERT(prCmdBody);

	/* To do: exception handle */
	if (!prCmdBody) {
		log_dbg(CNM, ERROR,
		       "ChReq: fail to get buf (net=%d, token=%d)\n",
		       prMsgChReq->ucBssIndex, prMsgChReq->ucTokenID);

		cnmMemFree(prAdapter, prMsgHdr);
		return;
	}

	/* Activate network if it's not activated yet */
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsgChReq->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(CNM, ERROR,
			"Invalid bss idx: %d\n",
			prMsgChReq->ucBssIndex);
		return;
	}

	if (!IS_BSS_ACTIVE(prBssInfo))
		nicActivateNetworkEx(prAdapter, prBssInfo->ucBssIndex, FALSE);

	log_dbg(CNM, INFO,
	       "ChReq net=%d token=%d b=%d c=%d s=%d w(vht)=%d s1=%d s2=%d d=%d t=%d\n",
	       prMsgChReq->ucBssIndex, prMsgChReq->ucTokenID,
	       prMsgChReq->eRfBand, prMsgChReq->ucPrimaryChannel,
	       prMsgChReq->eRfSco, prMsgChReq->eRfChannelWidth,
	       prMsgChReq->ucRfCenterFreqSeg1,
	       prMsgChReq->ucRfCenterFreqSeg2,
	       prMsgChReq->u4MaxInterval,
	       prMsgChReq->eReqType);

	prCmdBody->ucBssIndex = prMsgChReq->ucBssIndex;
	prCmdBody->ucTokenID = prMsgChReq->ucTokenID;
	prCmdBody->ucAction = CMD_CH_ACTION_REQ;	/* Request */
	prCmdBody->ucPrimaryChannel =
		prMsgChReq->ucPrimaryChannel;
	prCmdBody->ucRfSco = (uint8_t)prMsgChReq->eRfSco;
	prCmdBody->ucRfBand = (uint8_t)prMsgChReq->eRfBand;
	prCmdBody->ucRfChannelWidth = (uint8_t)
				      prMsgChReq->eRfChannelWidth;
	prCmdBody->ucRfCenterFreqSeg1 = (uint8_t)
					prMsgChReq->ucRfCenterFreqSeg1;
	prCmdBody->ucRfCenterFreqSeg2 = (uint8_t)
					prMsgChReq->ucRfCenterFreqSeg2;
	prCmdBody->ucReqType = (uint8_t)prMsgChReq->eReqType;
	prCmdBody->ucDBDCBand = (uint8_t)prMsgChReq->eDBDCBand;
	prCmdBody->aucReserved = 0;
	prCmdBody->u4MaxInterval = prMsgChReq->u4MaxInterval;
	prCmdBody->aucReserved2[0] = 0;
	prCmdBody->aucReserved2[1] = 0;
	prCmdBody->aucReserved2[2] = 0;
	prCmdBody->aucReserved2[3] = 0;
	prCmdBody->aucReserved2[4] = 0;
	prCmdBody->aucReserved2[5] = 0;
	prCmdBody->aucReserved2[6] = 0;
	prCmdBody->aucReserved2[7] = 0;

	ASSERT(prCmdBody->ucBssIndex <=
	       prAdapter->ucHwBssIdNum);

	/* For monkey testing 20110901 */
	if (prCmdBody->ucBssIndex > prAdapter->ucHwBssIdNum)
		log_dbg(CNM, ERROR,
		       "CNM: ChReq with wrong netIdx=%d\n\n",
		       prCmdBody->ucBssIndex);

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				      CMD_ID_CH_PRIVILEGE,	/* ucCID */
				      TRUE,	/* fgSetQuery */
				      FALSE,	/* fgNeedResp */
				      FALSE,	/* fgIsOid */
				      NULL,	/* pfCmdDoneHandler */
				      NULL,	/* pfCmdTimeoutHandler */

				      /* u4SetQueryInfoLen */
				      sizeof(struct CMD_CH_PRIVILEGE),

				      /* pucInfoBuffer */
				      (uint8_t *)prCmdBody,

				      prMsgHdr,	/* pvSetQueryBuffer */
				      0	/* u4SetQueryBufferLen */
				     );

	/* ASSERT(rStatus == WLAN_STATUS_PENDING); */

	cnmMemFree(prAdapter, prCmdBody);
	cnmMemFree(prAdapter, prMsgHdr);
}	/* end of cnmChMngrRequestPrivilege()*/

/*----------------------------------------------------------------------------*/
/*!
 * @brief Before deliver the message to other module, it need to release
 *        the Channel privilege to Channel Manager.
 *
 * @param[in] prMsgHdr   The message need to be delivered
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmChMngrAbortPrivilege(struct ADAPTER *prAdapter,
			     struct MSG_HDR *prMsgHdr)
{
	struct MSG_CH_ABORT *prMsgChAbort;
	struct CMD_CH_PRIVILEGE *prCmdBody;
	uint32_t rStatus;
#if CFG_SISO_SW_DEVELOP
	struct BSS_INFO *prBssInfo;
#endif
#if CFG_SUPPORT_DBDC
	struct LINK_ENTRY *prLinkEntry_pendingMsg;
	struct MSG_CH_REQ *prPendingMsg;
#endif

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	prMsgChAbort = (struct MSG_CH_ABORT *)prMsgHdr;

#if CFG_SUPPORT_DBDC
	if (cnmDBDCIsReqPeivilegeLock()) {
		LINK_FOR_EACH(prLinkEntry_pendingMsg,
			      &g_rDbdcInfo.rPendingMsgList) {
			prPendingMsg = (struct MSG_CH_REQ *)
				       LINK_ENTRY(prLinkEntry_pendingMsg,
						struct MSG_HDR, rLinkEntry);

			/* Find matched request and check
			 * if it is being served.
			 */
			if ((prPendingMsg->ucTokenID ==
				prMsgChAbort->ucTokenID) &&
				(prPendingMsg->ucBssIndex ==
				prMsgChAbort->ucBssIndex)) {
				LINK_REMOVE_KNOWN_ENTRY(
					&g_rDbdcInfo.rPendingMsgList,
					&prPendingMsg->rMsgHdr.rLinkEntry);

				log_dbg(CNM, INFO, "[DBDC] ChAbort: remove Token %u REQ)\n",
					prPendingMsg->ucTokenID);

				cnmMemFree(prAdapter, prPendingMsg);
				cnmMemFree(prAdapter, prMsgHdr);

				return;
			}
		}
	}
#endif

	prCmdBody = (struct CMD_CH_PRIVILEGE *)
		    cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
				sizeof(struct CMD_CH_PRIVILEGE));
	if (!prCmdBody) {
		log_dbg(CNM, ERROR,
		       "ChAbort: fail to get buf (net=%d, token=%d)\n",
		       prMsgChAbort->ucBssIndex, prMsgChAbort->ucTokenID);

		cnmMemFree(prAdapter, prMsgHdr);
		return;
	}

	prCmdBody->ucBssIndex = prMsgChAbort->ucBssIndex;
	prCmdBody->ucTokenID = prMsgChAbort->ucTokenID;
	prCmdBody->ucAction = CMD_CH_ACTION_ABORT;	/* Abort */
	prCmdBody->ucDBDCBand = (uint8_t)
				prMsgChAbort->eDBDCBand;

	log_dbg(CNM, INFO, "ChAbort net=%d token=%d dbdc=%u\n",
	       prCmdBody->ucBssIndex, prCmdBody->ucTokenID,
	       prCmdBody->ucDBDCBand);

	ASSERT(prCmdBody->ucBssIndex <=
	       prAdapter->ucHwBssIdNum);

	/* For monkey testing 20110901 */
	if (prCmdBody->ucBssIndex > prAdapter->ucHwBssIdNum)
		log_dbg(CNM, ERROR,
		       "CNM: ChAbort with wrong netIdx=%d\n\n",
		       prCmdBody->ucBssIndex);

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				      CMD_ID_CH_PRIVILEGE,	/* ucCID */
				      TRUE,	/* fgSetQuery */
				      FALSE,	/* fgNeedResp */
				      FALSE,	/* fgIsOid */
				      NULL,	/* pfCmdDoneHandler */
				      NULL,	/* pfCmdTimeoutHandler */

				      /* u4SetQueryInfoLen */
				      sizeof(struct CMD_CH_PRIVILEGE),

				      /* pucInfoBuffer */
				      (uint8_t *)prCmdBody,

				      prMsgHdr,	/* pvSetQueryBuffer */
				      0	/* u4SetQueryBufferLen */
				     );

	/* ASSERT(rStatus == WLAN_STATUS_PENDING); */

#if CFG_SISO_SW_DEVELOP
	prBssInfo =
		prAdapter->aprBssInfo[prMsgChAbort->ucBssIndex];
	/* Driver clear granted CH in BSS info */
	prBssInfo->fgIsGranted = FALSE;
	prBssInfo->eBandGranted = BAND_NULL;
	prBssInfo->ucPrimaryChannelGranted = 0;
#endif

	cnmMemFree(prAdapter, prCmdBody);
	cnmMemFree(prAdapter, prMsgHdr);
}				/* end of cnmChMngrAbortPrivilege()*/

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmChMngrHandleChEvent(struct ADAPTER *prAdapter,
			    struct WIFI_EVENT *prEvent)
{
	struct EVENT_CH_PRIVILEGE *prEventBody;
	struct MSG_CH_GRANT *prChResp;
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);
	ASSERT(prEvent);

	prEventBody = (struct EVENT_CH_PRIVILEGE *)(
			      prEvent->aucBuffer);
	prChResp = (struct MSG_CH_GRANT *)
		   cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			       sizeof(struct MSG_CH_GRANT));
	if (!prChResp) {
		log_dbg(CNM, ERROR,
		       "ChGrant: fail to get buf (net=%d, token=%d)\n",
		       prEventBody->ucBssIndex, prEventBody->ucTokenID);

		return;
	}

	log_dbg(CNM, INFO,
	       "ChGrant net=%d band=%d token=%d ch=%d bw=%d sco=%d s1=%d s2=%d, u4GrantInterval=%d\n",
	       prEventBody->ucBssIndex,
	       prEventBody->ucDBDCBand,
	       prEventBody->ucTokenID,
	       prEventBody->ucPrimaryChannel,
	       prEventBody->ucRfChannelWidth,
	       prEventBody->ucRfSco,
	       prEventBody->ucRfCenterFreqSeg1,
	       prEventBody->ucRfCenterFreqSeg2,
	       prEventBody->u4GrantInterval);

	ASSERT(prEventBody->ucBssIndex <=
	       prAdapter->ucHwBssIdNum);
	ASSERT(prEventBody->ucStatus == EVENT_CH_STATUS_GRANT);

	if (prEventBody->ucBssIndex >= ARRAY_SIZE(prAdapter->aprBssInfo)) {
		DBGLOG(CNM, ERROR, "ucBssIndex out of range %u!\n",
			prEventBody->ucBssIndex);
		return;
	}

	prBssInfo =
		prAdapter->aprBssInfo[prEventBody->ucBssIndex];

	if (!prBssInfo) {
		DBGLOG(CNM, ERROR, "prBssInfo is NULL\n");
		return;
	}

	/* Decide message ID based on network and response status */
	if (IS_BSS_AIS(prBssInfo))
		prChResp->rMsgHdr.eMsgId = MID_CNM_AIS_CH_GRANT;
#if CFG_ENABLE_WIFI_DIRECT
	else if (prAdapter->fgIsP2PRegistered
		 && IS_BSS_P2P(prBssInfo))
		prChResp->rMsgHdr.eMsgId = MID_CNM_P2P_CH_GRANT;
#endif
#if CFG_ENABLE_BT_OVER_WIFI
	else if (IS_BSS_BOW(prBssInfo))
		prChResp->rMsgHdr.eMsgId = MID_CNM_BOW_CH_GRANT;
#endif
#if CFG_SUPPORT_NAN
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_NAN)
		prChResp->rMsgHdr.eMsgId = MID_CNM_NAN_CH_GRANT;
#endif
	else {
		cnmMemFree(prAdapter, prChResp);
		return;
	}

	prChResp->ucBssIndex = prEventBody->ucBssIndex;
	prChResp->ucTokenID = prEventBody->ucTokenID;
	prChResp->ucPrimaryChannel =
		prEventBody->ucPrimaryChannel;
	prChResp->eRfSco = (enum ENUM_CHNL_EXT)
			   prEventBody->ucRfSco;
	prChResp->eRfBand = (enum ENUM_BAND)
			    prEventBody->ucRfBand;
	prChResp->eRfChannelWidth = (enum ENUM_CHANNEL_WIDTH)
				    prEventBody->ucRfChannelWidth;
	prChResp->ucRfCenterFreqSeg1 =
		prEventBody->ucRfCenterFreqSeg1;
	prChResp->ucRfCenterFreqSeg2 =
		prEventBody->ucRfCenterFreqSeg2;
	prChResp->eReqType = (enum ENUM_CH_REQ_TYPE)
			     prEventBody->ucReqType;
	prChResp->eDBDCBand = (enum ENUM_MBMC_BN)
			      prEventBody->ucDBDCBand;
	prChResp->u4GrantInterval =
		prEventBody->u4GrantInterval;

	mboxSendMsg(prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *)prChResp, MSG_SEND_METHOD_BUF);
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
void cnmRadarDetectEvent(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent)
{
	struct EVENT_RDD_REPORT *prEventBody;
	struct BSS_INFO *prBssInfo;
	struct MSG_P2P_RADAR_DETECT *prP2pRddDetMsg;
	uint8_t ucBssIndex;

	prEventBody = (struct EVENT_RDD_REPORT *)(
			      prEvent->aucBuffer);

	prP2pRddDetMsg = (struct MSG_P2P_RADAR_DETECT *)
			 cnmMemAlloc(prAdapter,
				     RAM_TYPE_MSG, sizeof(*prP2pRddDetMsg));

	if (!prP2pRddDetMsg) {
		log_dbg(CNM, ERROR,
		       "cnmMemAlloc for prP2pRddDetMsg failed!\n");
		return;
	}

	prP2pRddDetMsg->rMsgHdr.eMsgId =
		MID_CNM_P2P_RADAR_DETECT;

	for (ucBssIndex = 0; ucBssIndex < MAX_BSSID_NUM; ucBssIndex++) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
						  ucBssIndex);

		if (prBssInfo && prBssInfo->fgIsDfsActive) {
			prP2pRddDetMsg->ucBssIndex = ucBssIndex;
			break;
		}
	}

	log_dbg(CNM, INFO,
		"cnmRadarDetectEvent (%d).\n",
		prP2pRddDetMsg->ucBssIndex);

	p2pFuncSetDfsState(DFS_STATE_DETECTED);

	p2pFuncRadarInfoInit();

	g_rP2pRadarInfo.u1RddIdx =
		prEventBody->u1RddIdx;
	g_rP2pRadarInfo.u1LongDetected =
		prEventBody->u1LongDetected;
	g_rP2pRadarInfo.u1ConstantPRFDetected =
		prEventBody->u1ConstantPRFDetected;
	g_rP2pRadarInfo.u1StaggeredPRFDetected =
		prEventBody->u1StaggeredPRFDetected;
	g_rP2pRadarInfo.u1RadarTypeIdx =
		prEventBody->u1RadarTypeIdx;
	g_rP2pRadarInfo.u1PeriodicPulseNum =
		prEventBody->u1PeriodicPulseNum;
	g_rP2pRadarInfo.u1LongPulseNum =
		prEventBody->u1LongPulseNum;
	g_rP2pRadarInfo.u1HwPulseNum =
		prEventBody->u1HwPulseNum;
	g_rP2pRadarInfo.u1OutLPN =
		prEventBody->u1OutLPN;
	g_rP2pRadarInfo.u1OutSPN =
		prEventBody->u1OutSPN;
	g_rP2pRadarInfo.u1OutCRPN =
		prEventBody->u1OutCRPN;
	g_rP2pRadarInfo.u1OutCRPW =
		prEventBody->u1OutCRPW;
	g_rP2pRadarInfo.u1OutCRBN =
		prEventBody->u1OutCRBN;
	g_rP2pRadarInfo.u1OutSTGPN =
		prEventBody->u1OutSTGPN;
	g_rP2pRadarInfo.u1OutSTGPW =
		prEventBody->u1OutSTGPW;
	g_rP2pRadarInfo.u1Reserve =
		prEventBody->u1Reserve;
	g_rP2pRadarInfo.u4OutPRI_CONST =
		prEventBody->u4OutPRI_CONST;
	g_rP2pRadarInfo.u4OutPRI_STG1 =
		prEventBody->u4OutPRI_STG1;
	g_rP2pRadarInfo.u4OutPRI_STG2 =
		prEventBody->u4OutPRI_STG2;
	g_rP2pRadarInfo.u4OutPRI_STG3 =
		prEventBody->u4OutPRI_STG3;
	g_rP2pRadarInfo.u4OutPRIStgDmin =
		prEventBody->u4OutPRIStgDmin;
	kalMemCopy(&g_rP2pRadarInfo.arLongPulse[0],
		   &prEventBody->arLongPulse[0],
		   prEventBody->u1LongPulseNum * sizeof(struct
				   LONG_PULSE_BUFFER));
	kalMemCopy(&g_rP2pRadarInfo.arPeriodicPulse[0],
		   &prEventBody->arPeriodicPulse[0],
		   prEventBody->u1PeriodicPulseNum * sizeof(struct
				   PERIODIC_PULSE_BUFFER));
	kalMemCopy(&g_rP2pRadarInfo.arContent[0],
		   &prEventBody->arContent[0],
		   prEventBody->u1HwPulseNum * sizeof(struct
				   WH_RDD_PULSE_CONTENT));

	mboxSendMsg(prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *)prP2pRddDetMsg, MSG_SEND_METHOD_BUF);
}

void cnmCsaDoneEvent(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent)
{
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
	uint8_t ucBssIndex;

	DBGLOG(CNM, INFO, "cnmCsaDoneEvent.\n");
	if (prAdapter->rWifiVar.fgCsaInProgress == FALSE) {
		DBGLOG(CNM, WARN, "Receive duplicate cnmCsaDoneEvent.\n");
		return;
	}
	ucBssIndex = p2pFuncGetCsaBssIndex();
	if (!IS_BSS_INDEX_VALID(ucBssIndex)) {
		log_dbg(CNM, ERROR, "Csa bss is invalid!\n");
		return;
	}
	prP2pBssInfo = prAdapter->aprBssInfo[ucBssIndex];

	/* Clean up CSA variable */
	prAdapter->rWifiVar.fgCsaInProgress = FALSE;
	prAdapter->rWifiVar.ucChannelSwitchMode = 0;
	prAdapter->rWifiVar.ucNewOperatingClass = 0;
	prAdapter->rWifiVar.ucNewChannelNumber = 0;
	prAdapter->rWifiVar.ucChannelSwitchCount = 0;
	prAdapter->rWifiVar.ucSecondaryOffset = 0;
	prAdapter->rWifiVar.ucNewChannelWidth = 0;
	prAdapter->rWifiVar.ucNewChannelS1 = 0;
	prAdapter->rWifiVar.ucNewChannelS2 = 0;
	if (!prP2pBssInfo ||
		prP2pBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE)
		return;
	p2pFunChnlSwitchNotifyDone(prAdapter);
}
#endif

#define CFG_SUPPORT_IDC_CROSS_BAND_SWITCH   1

#if CFG_SUPPORT_IDC_CH_SWITCH
uint8_t cnmIsSafeCh(struct BSS_INFO *prBssInfo)
{
	enum ENUM_BAND eBand;
	uint8_t ucChannel;
	uint32_t u4Safe2G = 0,
		u4Safe5G_1 = 0,
		u4Safe5G_2 = 0,
		u4Safe6G = 0;

	if (!prBssInfo)
		return FALSE;

	if (g_rLteSafeChInfo.u4Flags & BIT(0)) {
		u4Safe2G = g_rLteSafeChInfo
			.rLteSafeChn.au4SafeChannelBitmask[0];
		u4Safe5G_1 = g_rLteSafeChInfo
			.rLteSafeChn.au4SafeChannelBitmask[1];
		u4Safe5G_2 = g_rLteSafeChInfo
			.rLteSafeChn.au4SafeChannelBitmask[2];
		u4Safe6G = g_rLteSafeChInfo
			.rLteSafeChn.au4SafeChannelBitmask[3];
	}

	eBand = prBssInfo->eBand;
	ucChannel = prBssInfo->ucPrimaryChannel;

	if (eBand == BAND_2G4) {
		if (u4Safe2G & BIT(ucChannel))
			return TRUE;
	} else if (eBand == BAND_5G &&
		ucChannel >= 36 && ucChannel <= 144) {
		if (u4Safe5G_1 & BIT((ucChannel - 36) / 4))
			return TRUE;
	} else if (eBand == BAND_5G &&
		ucChannel >= 149 && ucChannel <= 181) {
		if (u4Safe5G_2 & BIT((ucChannel - 149) / 4))
			return TRUE;
#if (CFG_SUPPORT_WIFI_6G == 1)
	} else if (eBand == BAND_6G &&
		ucChannel >= 7 && ucChannel <= 215) {
		if (u4Safe6G & BIT((ucChannel - 7) / 16))
			return TRUE;
#endif
	}

	return FALSE;
}

uint8_t cnmDecideSapNewChannel(
	struct GLUE_INFO *prGlueInfo,
	struct BSS_INFO *prBssInfo)
{
	uint8_t ucSwitchMode;
	uint32_t u4LteSafeChnBitMask_2G  = 0, u4LteSafeChnBitMask_5G_1 = 0,
		u4LteSafeChnBitMask_5G_2 = 0, u4LteSafeChnBitMask_6G = 0;
	uint8_t ucCurrentChannel = 0;

	if (!prGlueInfo || !prBssInfo) {
		DBGLOG(P2P, ERROR, "prGlueInfo or prBssInfo is NULL\n");
		return -EFAULT;
	}

	ucCurrentChannel = prBssInfo->ucPrimaryChannel;

	ASSERT(ucCurrentChannel);

	if (prBssInfo->eBand == BAND_2G4)
		ucSwitchMode = CH_SWITCH_2G;
	else if (prBssInfo->eBand == BAND_5G)
		ucSwitchMode = CH_SWITCH_5G;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prBssInfo->eBand == BAND_6G)
		ucSwitchMode = CH_SWITCH_6G;
#endif
	else {
		DBGLOG(P2P, WARN, "Bss has invalid band\n");
		return -EFAULT;
	}
	/*
	*  Get LTE safe channels
	*/
	if (g_rLteSafeChInfo.u4Flags & BIT(0)) {
		u4LteSafeChnBitMask_2G = g_rLteSafeChInfo
			.rLteSafeChn.au4SafeChannelBitmask[0];
		u4LteSafeChnBitMask_5G_1 = g_rLteSafeChInfo
			.rLteSafeChn.au4SafeChannelBitmask[1];
		u4LteSafeChnBitMask_5G_2 = g_rLteSafeChInfo
			.rLteSafeChn.au4SafeChannelBitmask[2];
		u4LteSafeChnBitMask_6G = g_rLteSafeChInfo
			.rLteSafeChn.au4SafeChannelBitmask[3];
	}

	if (ucSwitchMode == CH_SWITCH_2G) {
		if (!(u4LteSafeChnBitMask_2G & BITS(1, 14))) {
		DBGLOG(P2P, WARN,
			"FW report 2.4G all channels unsafe!?\n");
#if CFG_SUPPORT_IDC_CROSS_BAND_SWITCH
		/* Choose 5G non-RDD Channel */
		if ((u4LteSafeChnBitMask_5G_1 || u4LteSafeChnBitMask_5G_2)
			&& prGlueInfo->prAdapter->rWifiVar
			.fgCrossBandSwitchEn) {
			ucSwitchMode = CH_SWITCH_5G;
			DBGLOG(P2P, WARN,
				"Switch to 5G channel instead\n");
		} else {
			/* not to switch channel*/
			return 0;
		}
#endif
		}
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (ucSwitchMode == CH_SWITCH_6G) {
		if (!(u4LteSafeChnBitMask_6G & BITS(0, 13))) {
			DBGLOG(P2P, WARN,
				"FW report 6G all channels unsafe!?\n");
			/* not to switch channel*/
			return 0;
		}
	}
#endif
	else { /*ucSwitchMode == CH_SWITCH_5G*/
		if ((!(u4LteSafeChnBitMask_5G_1 & BITS(0, 27))) &&
			(!(u4LteSafeChnBitMask_5G_2 & BITS(0, 8)))) {
		DBGLOG(P2P, WARN,
			"FW report 5G all channels unsafe!?\n");
#if CFG_SUPPORT_IDC_CROSS_BAND_SWITCH
		/* Choose 2.4G non-RDD Channel */
		if (u4LteSafeChnBitMask_2G
			&& prGlueInfo->prAdapter->rWifiVar
			.fgCrossBandSwitchEn) {
			ucSwitchMode = CH_SWITCH_2G;
			DBGLOG(P2P, WARN,
				"Switch to 2.4G channel instead\n");
		} else {
			/* not to switch channel*/
			return 0;
		}
#endif
		}
	}

	return p2pFunGetAcsBestCh(prGlueInfo->prAdapter,
			prBssInfo->eBand,
			rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo),
			u4LteSafeChnBitMask_2G,
			u4LteSafeChnBitMask_5G_1,
			u4LteSafeChnBitMask_5G_2,
			u4LteSafeChnBitMask_6G);
}

uint8_t cnmIdcCsaReq(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucCh, uint8_t ucRoleIdx)
{
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t ucBssIdx = 0;
	struct RF_CHANNEL_INFO rRfChnlInfo;

	ASSERT(ucCh);

	if (p2pFuncRoleToBssIdx(
		prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS)
		return -1;

	DBGLOG(REQ, INFO,
		"[CSA]RoleIdx=%d, Band=%d, CH=%d, BssIdx=%d\n",
		ucRoleIdx, eBand, ucCh, ucBssIdx);

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	if (prBssInfo->eBand != eBand ||
		prBssInfo->ucPrimaryChannel != ucCh) {
		rlmGetChnlInfoForCSA(prAdapter,
			eBand, ucCh, ucBssIdx, &rRfChnlInfo);

		DBGLOG(REQ, INFO,
		"[CSA]CH=%d,Band=%d,BW=%d,PriFreq=%d,S1Freq=%d\n",
			rRfChnlInfo.ucChannelNum,
			rRfChnlInfo.eBand,
			rRfChnlInfo.ucChnlBw,
			rRfChnlInfo.u2PriChnlFreq,
			rRfChnlInfo.u4CenterFreq1);

		p2pFuncSetChannel(prAdapter, ucRoleIdx, &rRfChnlInfo);

		cnmSapChannelSwitchReq(prAdapter, &rRfChnlInfo, ucRoleIdx);

		/* Record Last Channel Switch Time */
		GET_CURRENT_SYSTIME(&g_rLastCsaSysTime);

		return 0; /* Return Success */

	} else {
		DBGLOG(CNM, INFO,
			"[CSA]Req CH = cur Band=%d, CH:%d, Stop Req\n",
			prBssInfo->eBand,
			prBssInfo->ucPrimaryChannel);
		return -1;
	}
}

void cnmSetIdcBssIdx(struct ADAPTER *prAdapter, uint8_t hwBssIdx)
{
	g_rLteSafeChInfo.aucReserved[0] = hwBssIdx;
}

uint8_t cnmGetIdcBssIdx(struct ADAPTER *prAdapter)
{
	return g_rLteSafeChInfo.aucReserved[0];
}

void cnmIdcDetectHandler(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent)
{
	struct EVENT_LTE_SAFE_CHN *prEventBody;
	uint8_t ucIdx;
	OS_SYSTIME rCurrentTime = 0;
	bool fgCsaCoolDown = FALSE;
	uint8_t ucColdDownTime = 0;
	struct WIFI_VAR *prWifiVar =
		(struct WIFI_VAR *)NULL;
#if CFG_TC10_FEATURE
	struct BSS_INFO *prBssInfo;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo;
#endif

#if CFG_TC10_FEATURE
	prBssInfo = cnmGetSapBssInfo(prAdapter);
	if (!prBssInfo) {
		DBGLOG(CNM, WARN,
			"[CSA]SoftAp Not Exist\n");
		return;
	}
	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
		prBssInfo->u4PrivateData);
	if (!prP2pRoleFsmInfo) {
		DBGLOG(CNM, WARN,
			"[CSA]SoftAp fsm Not Exist\n");
		return;
	} else if (!prP2pRoleFsmInfo->fgIsChannelSelectByAcs) {
		DBGLOG(P2P, INFO, "Do not switch channel since not ACS\n");
		return;
	}
#endif

	prEventBody = (struct EVENT_LTE_SAFE_CHN *)(
		prEvent->aucBuffer);

	g_rLteSafeChInfo.ucVersion = prEventBody->ucVersion;
	g_rLteSafeChInfo.u4Flags = prEventBody->u4Flags;

	/* Statistics from FW is valid */
	if (prEventBody->u4Flags & BIT(0)) {
		for (ucIdx = 0;
			ucIdx < ENUM_SAFE_CH_MASK_MAX_NUM;
				ucIdx++) {
			g_rLteSafeChInfo.rLteSafeChn.
				au4SafeChannelBitmask[ucIdx]
				= prEventBody->rLteSafeChn.
				au4SafeChannelBitmask[ucIdx];

			DBGLOG(P2P, INFO,
				"[CSA]LTE safe channels[%d]=0x%08x\n",
				ucIdx,
				prEventBody->rLteSafeChn.
				au4SafeChannelBitmask[ucIdx]);
		}
	}

	if (g_rLteSafeChInfo.ucVersion == 2)
		goto SKIP_COOL_DOWN;

	prWifiVar = &prAdapter->rWifiVar;
	if (prWifiVar->ucChannelSwtichColdownTime)
		ucColdDownTime = prWifiVar->ucChannelSwtichColdownTime;
	else
		ucColdDownTime = IDC_CSA_GUARD_TIME;


	/* Only allow to switch channel once each minute*/
	GET_CURRENT_SYSTIME(&rCurrentTime);
	if ((CHECK_FOR_TIMEOUT(rCurrentTime,
			g_rLastCsaSysTime,
			SEC_TO_SYSTIME(ucColdDownTime)))
			|| (g_rLastCsaSysTime == 0)) {
		fgCsaCoolDown = TRUE;
	}

	if (!fgCsaCoolDown) {
		DBGLOG(CNM, INFO,
			"[CSA]CsaCoolDown not Finish yet,rCurrentTime=%d,g_rLastCsaSysTime=%d,IDC_CSA_GUARD_TIME=%d\n",
			rCurrentTime,
			g_rLastCsaSysTime,
			SEC_TO_SYSTIME(ucColdDownTime));
		return;
	}

SKIP_COOL_DOWN:
	cnmSetIdcBssIdx(prAdapter, 0);
	cnmIdcSwitchSapChannel(prAdapter);
}

void cnmIdcSwitchSapChannel(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;
	uint8_t ucNewChannel = 0;

	if (!prAdapter)
		return;

	for (i = cnmGetIdcBssIdx(prAdapter); i < prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

		if (prBssInfo &&
			IS_BSS_P2P(prBssInfo) &&
			p2pFuncIsAPMode(
			prAdapter->rWifiVar.prP2PConnSettings
			[prBssInfo->u4PrivateData]) &&
			IS_NET_PWR_STATE_ACTIVE(
			prAdapter,
			prBssInfo->ucBssIndex)) {
			if (cnmIsSafeCh(prBssInfo))
				continue;
			ucNewChannel = cnmDecideSapNewChannel(
				prAdapter->prGlueInfo,
				prBssInfo);
			if (ucNewChannel) {
				cnmIdcCsaReq(prAdapter,
					prBssInfo->eBand,
					ucNewChannel,
					prBssInfo->u4PrivateData);
				DBGLOG(CNM, INFO,
					"IDC Version %d, Bss=%d, NewCH=%d\n",
					g_rLteSafeChInfo.ucVersion,
					prBssInfo->ucBssIndex,
					ucNewChannel);
				break;
			}
		}
	}
	cnmSetIdcBssIdx(prAdapter, i);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is invoked for P2P or BOW networks
 *
 * @param (none)
 *
 * @return TRUE: suggest to adopt the returned preferred channel
 *         FALSE: No suggestion. Caller should adopt its preference
 */
/*----------------------------------------------------------------------------*/
u_int8_t
cnmPreferredChannel(struct ADAPTER *prAdapter,
		    enum ENUM_BAND *prBand, uint8_t *pucPrimaryChannel,
		    enum ENUM_CHNL_EXT *prBssSCO)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;

	ASSERT(prAdapter);
	ASSERT(prBand);
	ASSERT(pucPrimaryChannel);
	ASSERT(prBssSCO);

	for (i = 0; i < prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		if (prBssInfo) {
			if (IS_BSS_AIS(prBssInfo)
			    && RLM_NET_PARAM_VALID(prBssInfo)) {
				*prBand = prBssInfo->eBand;
				*pucPrimaryChannel
					= prBssInfo->ucPrimaryChannel;
				*prBssSCO = prBssInfo->eBssSCO;

				return TRUE;
			}
		}
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return TRUE: available channel is limited to return value
 *         FALSE: no limited
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmAisInfraChannelFixed(struct ADAPTER
				 *prAdapter,
				 enum ENUM_BAND *prBand,
				 uint8_t *pucPrimaryChannel)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;
	struct WIFI_VAR *prWifiVar;

	ASSERT(prAdapter);

	prWifiVar = &prAdapter->rWifiVar;

	if (prWifiVar->u4ScanCtrl &
	    SCN_CTRL_DEFAULT_SCAN_CTRL) {
		/* log_dbg(CNM, INFO, "ByPass AIS channel Fix check\n");*/
		return FALSE;
	}

	for (i = 0; i < prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

#if 0
		log_dbg(INIT, INFO,
		       "%s BSS[%u] active[%u] netType[%u]\n",
		       __func__, i, prBssInfo->fgIsNetActive,
		       prBssInfo->eNetworkType);
#endif

		if (!IS_NET_ACTIVE(prAdapter, i))
			continue;

#if CFG_ENABLE_WIFI_DIRECT
		if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P
			&& !cnmSapIsConcurrent(prAdapter)) {
			u_int8_t fgFixedChannel =
				p2pFuncIsAPMode(
					prAdapter->rWifiVar.prP2PConnSettings[
						prBssInfo->u4PrivateData]);

			if (fgFixedChannel) {

				*prBand = prBssInfo->eBand;
				*pucPrimaryChannel
					= prBssInfo->ucPrimaryChannel;

				return TRUE;

			}
		}
#endif

#if CFG_ENABLE_BT_OVER_WIFI && CFG_BOW_LIMIT_AIS_CHNL
		if (prBssInfo->eNetworkType == NETWORK_TYPE_BOW) {
			*prBand = prBssInfo->eBand;
			*pucPrimaryChannel = prBssInfo->ucPrimaryChannel;

			return TRUE;
		}
#endif

	}

	return FALSE;
}

#if CFG_SUPPORT_CHNL_CONFLICT_REVISE
u_int8_t cnmAisDetectP2PChannel(struct ADAPTER
				*prAdapter,
				enum ENUM_BAND *prBand,
				uint8_t *pucPrimaryChannel)
{
	uint8_t i = 0;
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);

#if CFG_ENABLE_WIFI_DIRECT
	for (; i < prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];
		if (prBssInfo->eNetworkType != NETWORK_TYPE_P2P)
			continue;
		if (prBssInfo->eConnectionState ==
		    MEDIA_STATE_CONNECTED ||
		    (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT
		     && prBssInfo->eIntendOPMode == OP_MODE_NUM)) {
			*prBand = prBssInfo->eBand;
			*pucPrimaryChannel = prBssInfo->ucPrimaryChannel;
			return TRUE;
		}
	}
#endif
	return FALSE;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmAisInfraConnectNotify(struct ADAPTER *prAdapter)
{
#if CFG_ENABLE_BT_OVER_WIFI
	struct BSS_INFO *prBssInfo, *prAisBssInfo, *prBowBssInfo;
	uint8_t i;

	ASSERT(prAdapter);

	prAisBssInfo = NULL;
	prBowBssInfo = NULL;

	for (i = 0; i < prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

		if (prBssInfo && IS_BSS_ACTIVE(prBssInfo)) {
			if (IS_BSS_AIS(prBssInfo))
				prAisBssInfo = prBssInfo;
			else if (IS_BSS_BOW(prBssInfo))
				prBowBssInfo = prBssInfo;
		}
	}

	if (prAisBssInfo && prBowBssInfo
	    && RLM_NET_PARAM_VALID(prAisBssInfo)
	    && RLM_NET_PARAM_VALID(prBowBssInfo)) {
		if (prAisBssInfo->eBand != prBowBssInfo->eBand ||
		    prAisBssInfo->ucPrimaryChannel !=
		    prBowBssInfo->ucPrimaryChannel) {

			/* Notify BOW to do deactivation */
			bowNotifyAllLinkDisconnected(prAdapter);
		}
	}
#endif
#if (CFG_SUPPORT_NAN == 1) && (CFG_NAN_SCHEDULER_VERSION == 1)
	if (nanSchedUpdateNonNanTimelineByAis(prAdapter)
		== WLAN_STATUS_SUCCESS)
		nanSchedSyncNonNanChnlToNan(prAdapter);
#endif

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return TRUE: permitted
 *         FALSE: Not permitted
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmAisIbssIsPermitted(struct ADAPTER
			       *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;

	ASSERT(prAdapter);

	/* P2P device network shall be included */
	for (i = 0; i <= prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

		if (prBssInfo && IS_BSS_ACTIVE(prBssInfo)
		    && !IS_BSS_AIS(prBssInfo))
			return FALSE;
	}

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return TRUE: permitted
 *         FALSE: Not permitted
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmP2PIsPermitted(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;
	u_int8_t fgBowIsActive;

	ASSERT(prAdapter);

	fgBowIsActive = FALSE;

	for (i = 0; i < prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

		if (prBssInfo && IS_BSS_ACTIVE(prBssInfo)) {
			if (prBssInfo->eCurrentOPMode == OP_MODE_IBSS)
				return FALSE;
			else if (IS_BSS_BOW(prBssInfo))
				fgBowIsActive = TRUE;
		}
	}

#if CFG_ENABLE_BT_OVER_WIFI
	if (fgBowIsActive) {
		/* Notify BOW to do deactivation */
		bowNotifyAllLinkDisconnected(prAdapter);
	}
#endif

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return TRUE: permitted
 *         FALSE: Not permitted
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmBowIsPermitted(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;

	ASSERT(prAdapter);

	/* P2P device network shall be included */
	for (i = 0; i <= prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

		if (prBssInfo && IS_BSS_ACTIVE(prBssInfo) &&
		    (IS_BSS_P2P(prBssInfo)
		     || prBssInfo->eCurrentOPMode == OP_MODE_IBSS)) {
			return FALSE;
		}
	}

	return TRUE;
}



static uint8_t cnmGetAPBwPermitted(struct ADAPTER
				   *prAdapter, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucAPBandwidth = MAX_BW_160MHZ;
	struct BSS_DESC *prBssDesc = NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *)NULL;
	uint8_t i = 0;
	uint8_t ucOffset = (MAX_BW_80MHZ - CW_80MHZ);


	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(INIT, ERROR,
			"Invalid bss idx: %d\n",
			ucBssIndex);
		return MAX_BW_20MHZ;
	}

	if (IS_BSS_AIS(prBssInfo)) {
		/*AIS station mode*/
		prBssDesc
			= aisGetTargetBssDesc(prAdapter, ucBssIndex);
	} else if (IS_BSS_P2P(prBssInfo)) {
		/* P2P mode */

		for (i = 0 ; i < BSS_P2P_NUM; i++) {

			if (!prAdapter->rWifiVar.aprP2pRoleFsmInfo[i])
				continue;

			if (prAdapter->rWifiVar.aprP2pRoleFsmInfo[i]->ucBssIndex
			    ==
			    ucBssIndex)
				break;

		}

		if (i >= BSS_P2P_NUM) {
			prP2pRoleFsmInfo = NULL;
		} else {
			prP2pRoleFsmInfo =
				prAdapter->rWifiVar.aprP2pRoleFsmInfo[i];

			/*only GC need to consider GO's BW*/
			if (!p2pFuncIsAPMode(
					prAdapter->rWifiVar.prP2PConnSettings[
						prBssInfo->u4PrivateData])) {
				prBssDesc = prP2pRoleFsmInfo->rJoinInfo
					.prTargetBssDesc;
			}

		}


	}

	if (prBssDesc) {
		if (prBssDesc->eChannelWidth == CW_20_40MHZ) {
			if ((prBssDesc->eSco == CHNL_EXT_SCA)
			    || (prBssDesc->eSco == CHNL_EXT_SCB))
				ucAPBandwidth = MAX_BW_40MHZ;
			else
				ucAPBandwidth = MAX_BW_20MHZ;
		} else {
			ucAPBandwidth = prBssDesc->eChannelWidth + ucOffset;
		}

	}

	return ucAPBandwidth;
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return TRUE: permitted
 *         FALSE: Not permitted
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmBss40mBwPermitted(struct ADAPTER *prAdapter,
			      uint8_t ucBssIndex)
{
	ASSERT(prAdapter);

	/* Note: To support real-time decision instead of current
	 *       activated-time, the STA roaming case shall be considered
	 *       about synchronization problem. Another variable
	 *       fgAssoc40mBwAllowed is added to represent HT capability
	 *       when association
	 */

	/* Decide max bandwidth by feature option */
	if (cnmGetBssMaxBw(prAdapter,
			   ucBssIndex) < MAX_BW_40MHZ)
		return FALSE;

	/*check AP or GO capbility for Station or GC */
	if (cnmGetAPBwPermitted(prAdapter,
				ucBssIndex) < MAX_BW_40MHZ)
		return FALSE;
#if 0
	/* Decide max by other BSS */
	for (i = 0; i < prAdapter->ucHwBssIdNum; i++) {
		if (i != ucBssIndex) {
			prBssInfo = prAdapter->aprBssInfo[i];

			if (prBssInfo && IS_BSS_ACTIVE(prBssInfo) &&
			    (prBssInfo->fg40mBwAllowed
			     || prBssInfo->fgAssoc40mBwAllowed))
				return FALSE;
		}
	}
#endif

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return TRUE: permitted
 *         FALSE: Not permitted
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmBss80mBwPermitted(struct ADAPTER *prAdapter,
			      uint8_t ucBssIndex)
{
	ASSERT(prAdapter);

	/* Note: To support real-time decision instead of current
	 *       activated-time, the STA roaming case shall be considered
	 *       about synchronization problem. Another variable
	 *       fgAssoc40mBwAllowed is added to represent HT capability
	 *       when association
	 */

	/* Check 40Mhz first */
	if (!cnmBss40mBwPermitted(prAdapter, ucBssIndex))
		return FALSE;

	/* Decide max bandwidth by feature option */
	if (cnmGetBssMaxBw(prAdapter,
			   ucBssIndex) < MAX_BW_80MHZ)
		return FALSE;

	/*check AP or GO capbility for Station or GC */
	if (cnmGetAPBwPermitted(prAdapter,
				ucBssIndex) < MAX_BW_80MHZ)
		return FALSE;

	return TRUE;
}

uint8_t cnmGetBssMaxBw(struct ADAPTER *prAdapter,
		       uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucMaxBandwidth = MAX_BW_80_80_MHZ; /*chip capability*/
	struct BSS_DESC *prBssDesc = NULL;
	enum ENUM_BAND eBand = BAND_NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo =
		(struct P2P_CONNECTION_REQ_INFO *) NULL;
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	uint8_t ucChannelBw = MAX_BW_80_80_MHZ;
#endif
	uint8_t ucRoleIndex = 0;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(CNM, ERROR, "Invalid bss idx: %d\n", ucBssIndex);
		return MAX_BW_20MHZ;
	}

	if (IS_BSS_AIS(prBssInfo)) {
		/* STA mode */

		/* should check Bss_info could be used or not
		 * the info might not be trustable before state3
		 */

		prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
		if (prBssDesc)
			eBand = prBssDesc->eBand;
		else
			eBand = prBssInfo->eBand;


		ASSERT(eBand != BAND_NULL);

		if (eBand == BAND_2G4)
			ucMaxBandwidth = prAdapter->rWifiVar.ucSta2gBandwidth;
		else if (eBand == BAND_5G)
			ucMaxBandwidth = prAdapter->rWifiVar.ucSta5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (eBand == BAND_6G)
			ucMaxBandwidth = prAdapter->rWifiVar.ucSta6gBandwidth;
#endif

		if (prAdapter->rWifiVar.u2CountryCode == COUNTRY_CODE_ID) {
			if (eBand == BAND_2G4 &&
					ucMaxBandwidth > MAX_BW_20MHZ) {
				ucMaxBandwidth = MAX_BW_20MHZ;
				log_dbg(CNM, INFO,
					"Apply customized bandwidth for Indonesia: 2.4G MaxBW20\n");
			} else if (eBand == BAND_5G &&
					ucMaxBandwidth > MAX_BW_80MHZ) {
				ucMaxBandwidth = MAX_BW_80MHZ;
				log_dbg(CNM, INFO,
					"Apply customized bandwidth for Indonesia: 5G MaxBW80\n");
			}
		}

		if (ucMaxBandwidth > prAdapter->rWifiVar.ucStaBandwidth)
			ucMaxBandwidth = prAdapter->rWifiVar.ucStaBandwidth;
	} else if (IS_BSS_P2P(prBssInfo)) {
		prP2pRoleFsmInfo = p2pFuncGetRoleByBssIdx(prAdapter,
				   ucBssIndex);
		if (!prAdapter->rWifiVar.ucApChnlDefFromCfg
		    && prP2pRoleFsmInfo
		    && prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
			prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
			ucMaxBandwidth = prP2pConnReqInfo->eChnlBw;
		} else {
			/* AP mode */
			if (p2pFuncIsAPMode(
					prAdapter->rWifiVar.prP2PConnSettings[
						prBssInfo->u4PrivateData])) {
				if (prBssInfo->eBand == BAND_2G4)
					ucMaxBandwidth = prAdapter->rWifiVar
						.ucAp2gBandwidth;
				else if (prBssInfo->eBand == BAND_5G)
					ucMaxBandwidth = prAdapter->rWifiVar
						.ucAp5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
				else if (prBssInfo->eBand == BAND_6G)
					ucMaxBandwidth = prAdapter->rWifiVar
						.ucAp6gBandwidth;
#endif

				if (ucMaxBandwidth
					> prAdapter->rWifiVar.ucApBandwidth)
					ucMaxBandwidth = prAdapter->rWifiVar
						.ucApBandwidth;
			}
			/* P2P mode */
			else {
				if (prBssInfo->eBand == BAND_2G4)
					ucMaxBandwidth = prAdapter->rWifiVar
						.ucP2p2gBandwidth;
				else if (prBssInfo->eBand == BAND_5G)
					ucMaxBandwidth = prAdapter->rWifiVar
						.ucP2p5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
				else if (prBssInfo->eBand == BAND_6G)
					ucMaxBandwidth = prAdapter->rWifiVar
						.ucP2p6gBandwidth;
#endif
			}

		}
		ucRoleIndex = prBssInfo->u4PrivateData;

		if (p2pFuncGetDfsState() == DFS_STATE_DETECTED)
			ucMaxBandwidth = prAdapter->rWifiVar
				.prP2pSpecificBssInfo[ucRoleIndex]
				->ucRddBw;

	}

#if CFG_SUPPORT_NAN
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_NAN) {
		if (prBssInfo->eBand == BAND_2G4)
			ucMaxBandwidth = prAdapter->rWifiVar
					.ucNan2gBandwidth;
		else if (prBssInfo->eBand == BAND_5G)
			ucMaxBandwidth = prAdapter->rWifiVar
					.ucNan5gBandwidth;
	}
#endif

#if (CFG_SUPPORT_SINGLE_SKU == 1)
	if (IS_BSS_AIS(prBssInfo) && prBssDesc) {
		ucChannelBw = rlmDomainGetChannelBw(prBssDesc->eBand,
			prBssDesc->ucChannelNum);
	} else {
		ucChannelBw = rlmDomainGetChannelBw(prBssInfo->eBand,
			prBssInfo->ucPrimaryChannel);
	}
	if (ucMaxBandwidth > ucChannelBw)
		ucMaxBandwidth = ucChannelBw;
#endif
	if (IS_BSS_AIS(prBssInfo) && prBssDesc) {
		DBGLOG(CNM, TRACE, "pCH=%d, BW=%d\n",
			prBssDesc->ucChannelNum, ucMaxBandwidth);
	} else {
		DBGLOG(CNM, TRACE, "pCH=%d, BW=%d\n",
			prBssInfo->ucPrimaryChannel, ucMaxBandwidth);
	}

	return ucMaxBandwidth;
}

uint8_t cnmGetBssBandBw(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	enum ENUM_BAND eBand)
{
	uint8_t ucMaxBandwidth = MAX_BW_20MHZ;

	if (IS_BSS_AIS(prBssInfo)) {
		if (eBand == BAND_2G4)
			ucMaxBandwidth = prAdapter->rWifiVar.ucSta2gBandwidth;
		else if (eBand == BAND_5G)
			ucMaxBandwidth = prAdapter->rWifiVar.ucSta5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (eBand == BAND_6G)
			ucMaxBandwidth = prAdapter->rWifiVar.ucSta6gBandwidth;
#endif
		if (ucMaxBandwidth > prAdapter->rWifiVar.ucStaBandwidth)
			ucMaxBandwidth = prAdapter->rWifiVar.ucStaBandwidth;
	} else if (IS_BSS_P2P(prBssInfo)) {
		/* AP mode */
		if (p2pFuncIsAPMode(
				prAdapter->rWifiVar.prP2PConnSettings[
					prBssInfo->u4PrivateData])) {
			if (prBssInfo->eBand == BAND_2G4)
				ucMaxBandwidth = prAdapter->rWifiVar
					.ucAp2gBandwidth;
			else if (prBssInfo->eBand == BAND_5G) {
				ucMaxBandwidth = prAdapter->rWifiVar
					.ucAp5gBandwidth;
				/* Use platform capability */
				if (prAdapter->rWifiVar.u4SwTestMode ==
					ENUM_SW_TEST_MODE_SIGMA_AX_AP)
					ucMaxBandwidth = prAdapter->rWifiVar
						.ucApBandwidth;
			}
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (prBssInfo->eBand == BAND_6G)
				ucMaxBandwidth = prAdapter->rWifiVar
					.ucAp6gBandwidth;
#endif
			if (ucMaxBandwidth
				> prAdapter->rWifiVar.ucApBandwidth)
				ucMaxBandwidth = prAdapter->rWifiVar
					.ucApBandwidth;
		}
		/* P2P mode */
		else {
			if (prBssInfo->eBand == BAND_2G4)
				ucMaxBandwidth = prAdapter->rWifiVar
					.ucP2p2gBandwidth;
			else if (prBssInfo->eBand == BAND_5G)
				ucMaxBandwidth = prAdapter->rWifiVar
					.ucP2p5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (prBssInfo->eBand == BAND_6G)
				ucMaxBandwidth = prAdapter->rWifiVar
					.ucP2p6gBandwidth;
#endif
		}
	}
#if CFG_SUPPORT_NAN
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_NAN) {
		if (prBssInfo->eBand == BAND_2G4)
			ucMaxBandwidth = prAdapter->rWifiVar
				.ucNan2gBandwidth;
		else if (prBssInfo->eBand == BAND_5G)
			ucMaxBandwidth = prAdapter->rWifiVar
				.ucNan5gBandwidth;
	}
#endif

	return ucMaxBandwidth;
}

uint8_t cnmGetBssMaxBwToChnlBW(struct ADAPTER
			       *prAdapter,
			       uint8_t ucBssIndex)
{
	uint8_t ucMaxBandwidth = cnmGetBssMaxBw(prAdapter,
						ucBssIndex);
	return ucMaxBandwidth == MAX_BW_20MHZ ? ucMaxBandwidth :
	       (ucMaxBandwidth - 1);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief    Search available HW ID and BSS_INFO structure and initialize
 *           these parameters, i.e., fgIsNetActive, ucBssIndex, eNetworkType
 *           and ucOwnMacIndex
 *
 * @param (none)
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
struct BSS_INFO *cnmGetBssInfoAndInit(struct ADAPTER *prAdapter,
				      enum ENUM_NETWORK_TYPE eNetworkType,
				      u_int8_t fgIsP2pDevice)
{
	struct WIFI_VAR *prWifiVar;
	struct BSS_INFO *prBssInfo = NULL, *prOutBssInfo = NULL;
	uint8_t i, ucBssIndex, ucOwnMacIdx = 0;

	ASSERT(prAdapter);

	prWifiVar = &prAdapter->rWifiVar;
	/*specific case for p2p device scan*/
	if (eNetworkType == NETWORK_TYPE_P2P && fgIsP2pDevice) {
		prBssInfo =
			prAdapter->aprBssInfo[prAdapter->ucP2PDevBssIdx];

		prBssInfo->fgIsInUse = TRUE;
		prBssInfo->ucBssIndex = prAdapter->ucP2PDevBssIdx;
		prBssInfo->eNetworkType = eNetworkType;
		prBssInfo->ucOwnMacIndex = prAdapter->ucHwBssIdNum;
		prBssInfo->u4TxStopTh = prWifiVar->u4NetifStopTh;
		prBssInfo->u4TxStartTh = prWifiVar->u4NetifStartTh;
#if (CFG_SUPPORT_802_11BE_MLO == 1) || defined(CFG_SUPPORT_UNIFIED_COMMAND)
		prBssInfo->ucOwnMldId = prBssInfo->ucBssIndex +
			MAT_OWN_MLD_ID_BASE;
		prBssInfo->ucGroupMldId = MLD_GROUP_NONE;
		prBssInfo->ucLinkIndex = 0;
#endif
		/* initialize wlan id and status for keys */
		prBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;
		prBssInfo->wepkeyWlanIdx = WTBL_RESERVED_ENTRY;
		for (i = 0; i < MAX_KEY_NUM; i++) {
			prBssInfo->ucBMCWlanIndexSUsed[i] = FALSE;
			prBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
			prBssInfo->wepkeyUsed[i] = FALSE;
		}

		return prBssInfo;
	}

	/* Find available HW set  with the order 1,2,..*/
	do {
		for (ucBssIndex = 0;
		     ucBssIndex < prAdapter->ucHwBssIdNum;
		     ucBssIndex++) {
			prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

			if (prBssInfo && prBssInfo->fgIsInUse
			    && ucOwnMacIdx == prBssInfo->ucOwnMacIndex)
				break;
		}

		if (ucBssIndex >= prAdapter->ucHwBssIdNum) {
			/* No hit the ucOwnMacIndex could be
			 * assigned to this new bss
			 */
			goto omac_choosed;
		}
	} while (++ucOwnMacIdx < prAdapter->ucHwBssIdNum);

	if (ucOwnMacIdx >= prAdapter->ucHwBssIdNum)
		return NULL;

omac_choosed:

	/* Find available BSS_INFO */
	for (ucBssIndex = 0;
	     ucBssIndex < prAdapter->ucHwBssIdNum;
	     ucBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

		if (prBssInfo && !prBssInfo->fgIsInUse) {
			prBssInfo->fgIsInUse = TRUE;
			prBssInfo->eNetworkType = eNetworkType;
			prBssInfo->ucBssIndex = ucBssIndex;
			prBssInfo->ucOwnMacIndex = ucOwnMacIdx;
			prBssInfo->eHwBandIdx = ENUM_BAND_AUTO;
			prBssInfo->eBackupHwBandIdx = ENUM_BAND_AUTO;
#if (CFG_SUPPORT_802_11BE_MLO == 1) || defined(CFG_SUPPORT_UNIFIED_COMMAND)
			prBssInfo->ucOwnMldId = ucBssIndex +
				MAT_OWN_MLD_ID_BASE;
			prBssInfo->ucGroupMldId = MLD_GROUP_NONE;
			prBssInfo->ucLinkIndex = 0;
#endif
#if (CFG_HW_WMM_BY_BSS == 1)
			prBssInfo->ucWmmQueSet = DEFAULT_HW_WMM_INDEX;
			prBssInfo->fgIsWmmInited = FALSE;
#endif
			/* initialize wlan id and status for keys */
			prBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;
			prBssInfo->wepkeyWlanIdx = WTBL_RESERVED_ENTRY;
			prBssInfo->fgBcDefaultKeyExist = FALSE;
			prBssInfo->ucBcDefaultKeyIdx = 0xff;
			prBssInfo->u4TxStopTh = prWifiVar->u4NetifStopTh;
			prBssInfo->u4TxStartTh = prWifiVar->u4NetifStartTh;
			for (i = 0; i < MAX_KEY_NUM; i++) {
				prBssInfo->ucBMCWlanIndexSUsed[i] = FALSE;
				prBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
				prBssInfo->wepkeyUsed[i] = FALSE;
			}
#if CFG_SUPPORT_DFS
			cnmTimerInitTimer(prAdapter,
				&prBssInfo->rCsaTimer,
				(PFN_MGMT_TIMEOUT_FUNC) rlmCsaTimeout,
				(uintptr_t)ucBssIndex);
			rlmResetCSAParams(prBssInfo);
			prBssInfo->fgHasStopTx = FALSE;
#endif
			prBssInfo->u4PowerSaveFlag = 0;
			prBssInfo->ePwrMode = Param_PowerModeCAM;

			kalMemZero(prBssInfo->aucBSSID, MAC_ADDR_LEN);

			log_dbg(CNM, INFO, "bss=%d,type=%d,omac=%d\n",
				prBssInfo->ucBssIndex,
				prBssInfo->eNetworkType,
				prBssInfo->ucOwnMacIndex);

			prOutBssInfo = prBssInfo;
			break;
		}
	}

	return prOutBssInfo;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief    Search available HW ID and BSS_INFO structure and initialize
 *           these parameters, i.e., ucBssIndex, eNetworkType and ucOwnMacIndex
 *
 * @param (none)
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
void cnmFreeBssInfo(struct ADAPTER *prAdapter,
		    struct BSS_INFO *prBssInfo)
{
	ASSERT(prAdapter);
	ASSERT(prBssInfo);

#if CFG_SUPPORT_DFS
	cnmTimerStopTimer(prAdapter, &prBssInfo->rCsaTimer);
#endif

	prBssInfo->fgIsInUse = FALSE;
}

#if CFG_SUPPORT_DBDC
/*----------------------------------------------------------------------------*/
/*!
 * @brief    Init DBDC
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmInitDbdcSetting(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_DBDC_DOWNGRADE_NSS == 1)
	struct CNM_OPMODE_BSS_REQ *prOpModeReq;
#endif
	uint8_t ucBssLoopIndex;

	DBDC_SET_WMMBAND_FW_AUTO_DEFAULT();
	g_rDbdcInfo.fgHasSentCmd = FALSE;
	g_rDbdcInfo.fgPostpondEnterAG = FALSE;
	g_rDbdcInfo.fgPostpondLeaveAG = FALSE;
	g_rDbdcInfo.rPeivilegeLockTime = 0;

	/* Parameter decision */
	switch (prAdapter->rWifiVar.eDbdcMode) {
	case ENUM_DBDC_MODE_DISABLED:
		cnmUpdateDbdcSetting(prAdapter, FALSE);
		break;

	case ENUM_DBDC_MODE_DYNAMIC:
		g_rDbdcInfo.eDbdcFsmCurrState =
			ENUM_DBDC_FSM_STATE_DISABLE_IDLE;
		g_rDbdcInfo.eDbdcFsmPrevState =
			ENUM_DBDC_FSM_STATE_DISABLE_IDLE;
		g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_DISABLE_IDLE;

		cnmTimerInitTimer(prAdapter,
			&g_rDbdcInfo.rDbdcGuardTimer,
			(PFN_MGMT_TIMEOUT_FUNC)cnmDbdcGuardTimerCallback,
			(uintptr_t) NULL);

		g_rDbdcInfo.eDdbcGuardTimerType =
			ENUM_DBDC_GUARD_TIMER_NONE;
		g_rDbdcInfo.fgReqPrivelegeLock = FALSE;
		LINK_INITIALIZE(&g_rDbdcInfo.rPendingMsgList);
		g_rDbdcInfo.fgDbdcDisableOpmodeChangeDone = TRUE;

		for (ucBssLoopIndex = 0;
		     ucBssLoopIndex < prAdapter->ucHwBssIdNum;
		     ucBssLoopIndex++)
			g_rDbdcInfo.eBssOpModeState[ucBssLoopIndex] =
				ENUM_OPMODE_STATE_DONE;

		cnmUpdateDbdcSetting(prAdapter, FALSE);
		break;

	case ENUM_DBDC_MODE_STATIC:
#if (CFG_SUPPORT_DBDC_DOWNGRADE_NSS == 1)
		for (ucBssLoopIndex = 0;
		    ucBssLoopIndex < prAdapter->ucHwBssIdNum;
		    ucBssLoopIndex++) {
			prOpModeReq =
				&(g_arBssOpControl[ucBssLoopIndex].
				arReqPool[CNM_OPMODE_REQ_DBDC]);
			prOpModeReq->fgEnable = TRUE;

			prOpModeReq->ucOpRxNss = 1;
			prOpModeReq->ucOpTxNss = 1;
		}
#endif
		cnmUpdateDbdcSetting(prAdapter, TRUE);

		g_rDbdcInfo.eDbdcFsmCurrState =
			ENUM_DBDC_FSM_STATE_ENABLE_IDLE;
		g_rDbdcInfo.eDbdcFsmPrevState =
			ENUM_DBDC_FSM_STATE_ENABLE_IDLE;
		g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_ENABLE_IDLE;
		prAdapter->rWifiVar.fgDbDcModeEn = g_rDbdcInfo.fgCmdEn;

		/* Just resue dynamic DBDC FSM handler. */
		cnmDbdcFsmEntryFunc_ENABLE_IDLE(prAdapter);
		break;

	default:
		log_dbg(CNM, ERROR, "[DBDC]Incorrect DBDC mode %u\n",
		       prAdapter->rWifiVar.eDbdcMode);
		break;
	}
}

#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_WIFI_DBDC6G == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief    Check A(5G)+A(6G) DBDC concurrent Condition
 *
 * @param (5G Channel Frequency)
 * @param (6G Channel Frequency)
 *
 * @return TRUE: A(5G)+A(6G) use DBDC concurrent , FALSE: Not concurrent
 */
/*----------------------------------------------------------------------------*/
static u_int8_t cnmDbdcDecideIsAAConcurrent(
	struct ADAPTER *prAdapter,
	u_int8_t uc5gCH,
	u_int8_t uc6gCH)
{
	u_int16_t ucFrequency5G, ucFrequency6G;
	u_int8_t fgAAConcurrent = FALSE;
	uint16_t u2MiminmunFrequency =
		prAdapter->rWifiFemCfg.u2WifiDBDCAwithAMinimumFrqInterval;

	if (prAdapter->rWifiFemCfg.u2WifiDBDCAwithA == FALSE)
		return FALSE;

	/*5G Channel start form 5000, EX: CH36 = 5180, CH132 = 5660*/
	ucFrequency5G = uc5gCH * 5 + 5000;

	/*6G Channel start form 5955,  EX: CH1 = 5955, CH5=5975*/
	ucFrequency6G = (uc6gCH - 1) * 5 + 5955;

	/* Check A+A can be DBDC or not */
	if ((ucFrequency6G - ucFrequency5G) >= u2MiminmunFrequency) {
		log_dbg(CNM, INFO, "[%s]decide AA DBDC = True\n", __func__);
		fgAAConcurrent = TRUE;
	}

	log_dbg(CNM, INFO, "5CH[%u]F[%u],6CH[%u]F[%u],MinF[%u],AA[%d]\n",
			uc5gCH, ucFrequency5G,
			uc6gCH, ucFrequency6G,
			u2MiminmunFrequency,
			fgAAConcurrent);

	return fgAAConcurrent;
}

#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief    Check DBDC A+G / A+A  Condition
 * refactor cnmDbdcIsAGConcurrent rename to cnmDbdcIsConcurrent
 *
 * @param (none)
 *
 * @return TRUE: DBDC A+G or A+A, FALSE: NOT for DBDC
 */
/*----------------------------------------------------------------------------*/
static u_int8_t cnmDbdcIsConcurrent(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eRfBand_Connecting,
	uint8_t ucPrimaryCHConnecting)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex;
	uint8_t ucBandCount[BAND_NUM] = {0};
	u_int8_t fgDBDCConcurrent = FALSE;
	enum ENUM_BAND eBssBand[MAX_BSSID_NUM + 1] = {BAND_NULL};
	enum ENUM_BAND eBandBss;
#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_WIFI_DBDC6G == 1)
	uint8_t ucBssPrimaryCH[MAX_BSSID_NUM + 1] = {0};
	uint8_t ucPrimaryChBss;
	uint8_t uc5gCH = 0, uc6gCH = 0;
#endif
#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
	uint8_t ucBssNum = prAdapter->ucHwBssIdNum + 1;
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo =
			prAdapter->rWifiVar.prP2pDevFsmInfo;
#else
	uint8_t ucBssNum = prAdapter->ucHwBssIdNum;
#endif
	u_int8_t fgDbdcP2pListening = FALSE;

#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
	if (prAdapter->fgPowerForceOneNss) {
		log_dbg(CNM, INFO, "[DBDC] disable DBDC by power");
		return FALSE;
	}
#endif

	if (eRfBand_Connecting > BAND_NULL && eRfBand_Connecting < BAND_NUM) {
		ucBandCount[eRfBand_Connecting]++;
#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_WIFI_DBDC6G == 1)
		if (eRfBand_Connecting == BAND_5G)
			uc5gCH = ucPrimaryCHConnecting;
		else if (eRfBand_Connecting == BAND_6G)
			uc6gCH = ucPrimaryCHConnecting;
#endif
	}

	for (ucBssIndex = 0;
			ucBssIndex < ucBssNum; ucBssIndex++) {

		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)

		/* dbdc decision use fgIsP2pListening to decide
		 * if it should check p2p dev BssInfo or not.
		 */

		if (prP2pDevFsmInfo &&
			prAdapter->rWifiVar.ucDbdcP2pLisEn)
			fgDbdcP2pListening =
				prP2pDevFsmInfo->fgIsP2pListening;

		if ((ucBssIndex != prAdapter->ucP2PDevBssIdx
			&& IS_BSS_NOT_ALIVE(prAdapter, prBssInfo)) ||
			(ucBssIndex == prAdapter->ucP2PDevBssIdx
			&& !fgDbdcP2pListening)
			)
			continue;
#else
		if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo))
			continue;
#endif

#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)

		if ((ucBssIndex == prAdapter->ucP2PDevBssIdx) &&
			prP2pDevFsmInfo) {
			eBandBss = prP2pDevFsmInfo->eReqBand;
#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_WIFI_DBDC6G == 1)
			ucPrimaryChBss = prP2pDevFsmInfo->ucReqChannelNum;
#endif
		} else
#endif
		{
			eBandBss = prBssInfo->eBand;
#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_WIFI_DBDC6G == 1)
			ucPrimaryChBss = prBssInfo->ucPrimaryChannel;
#endif
		}

		if (eBandBss > BAND_NULL && eBandBss < BAND_NUM) {
			eBssBand[ucBssIndex] = eBandBss;
			ucBandCount[eBandBss]++;
#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_WIFI_DBDC6G == 1)
			ucBssPrimaryCH[ucBssIndex] = ucPrimaryChBss;
			if (eBandBss == BAND_5G &&
				ucPrimaryChBss > uc5gCH)
				uc5gCH = ucPrimaryChBss;
			if (eBandBss == BAND_6G &&
				(uc6gCH == 0 || ucPrimaryChBss < uc6gCH))
				uc6gCH = ucPrimaryChBss;
#endif
		}
	}

#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_WIFI_DBDC6G == 1)
	g_rDbdcInfo.fgIsDBDCAAMode = 0;
#endif

	/* DBDC decision */
	if (ucBandCount[BAND_2G4] > 0) {
		/* 2.4G + 5G / 6G => enable DBDC */
		/* 2.4G + 5G + 6G => enable DBDC */
		if (ucBandCount[BAND_5G] > 0
#if (CFG_SUPPORT_WIFI_6G == 1)
				|| ucBandCount[BAND_6G] > 0
#endif
		   )
			fgDBDCConcurrent = TRUE;
		else /* 2.4G only */
			fgDBDCConcurrent = FALSE;
	} else {
#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_WIFI_DBDC6G == 1)
		/* Check DBDC A+A when HW support */
		if (ucBandCount[BAND_5G] > 0 && uc5gCH > 0 &&
		    ucBandCount[BAND_6G] > 0 && uc6gCH > 0 &&
		    cnmDbdcDecideIsAAConcurrent(prAdapter, uc5gCH, uc6gCH)) {
			fgDBDCConcurrent = TRUE;
			g_rDbdcInfo.fgIsDBDCAAMode = 1;
		} else {
			fgDBDCConcurrent = FALSE;
		}
#else
		/* 5G / 6G => disable DBDC */
		/* 5G + 6G => Do not supportf A+A, disable DBDC, */
		fgDBDCConcurrent = FALSE;
#endif
	}

	log_dbg(CNM, INFO, "[DBDC] %d BSS (P2P Listen = %u), Band[%u.%u.%u.%u.%u][New Band = %u, CH = %u], enable = %u\n",
			ucBssNum,
			fgDbdcP2pListening,
			eBssBand[BSSID_0],
			eBssBand[BSSID_1],
			eBssBand[BSSID_2],
			eBssBand[BSSID_3],
			eBssBand[MAX_BSSID_NUM],
			eRfBand_Connecting,
			ucPrimaryCHConnecting,
			fgDBDCConcurrent);

#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_WIFI_DBDC6G == 1)
	log_dbg(CNM, INFO, "[DBDC] CH[%u.%u.%u.%u.%u], 5G MAX = %u, 6G min = %u, AAMode[%u]\n",
			ucBssPrimaryCH[BSSID_0],
			ucBssPrimaryCH[BSSID_1],
			ucBssPrimaryCH[BSSID_2],
			ucBssPrimaryCH[BSSID_3],
			ucBssPrimaryCH[MAX_BSSID_NUM],
			uc5gCH,
			uc6gCH,
			g_rDbdcInfo.fgIsDBDCAAMode);
#endif

	return fgDBDCConcurrent;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to check MCC
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval current network is MCC mode
 */

bool cnmIsMccMode(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	uint32_t u4Idx;
	uint8_t ucLast2GChNum = 0, ucLast5GChNum = 0;
	bool fgIs2GMcc = false, fgIs5GMcc = false;

	ASSERT(prAdapter);

	for (u4Idx = 0; u4Idx < MAX_BSSID_NUM; u4Idx++) {
		prBssInfo = prAdapter->aprBssInfo[u4Idx];

		if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo))
			continue;

		if (prBssInfo->eBand == BAND_2G4) {
			if (ucLast2GChNum != 0 &&
			    ucLast2GChNum != prBssInfo->ucPrimaryChannel)
				fgIs2GMcc = true;
			ucLast2GChNum = prBssInfo->ucPrimaryChannel;
		} else if (prBssInfo->eBand == BAND_5G) {
			if (ucLast5GChNum != 0 &&
			    ucLast5GChNum != prBssInfo->ucPrimaryChannel)
				fgIs5GMcc = true;
			ucLast5GChNum = prBssInfo->ucPrimaryChannel;
		}
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (prBssInfo->eBand == BAND_6G) {
			/* Use the same handler as 5G channel */
			if (ucLast5GChNum != 0 &&
			    ucLast5GChNum != prBssInfo->ucPrimaryChannel)
				fgIs5GMcc = true;
			ucLast5GChNum = prBssInfo->ucPrimaryChannel;
		}
#endif
	}

	if (fgIs2GMcc || fgIs5GMcc)
		return true;

	return !prAdapter->rWifiVar.fgDbDcModeEn &&
		(ucLast2GChNum != 0 && ucLast5GChNum != 0);
}

uint8_t cnmGetDbdcNss(struct ADAPTER *prAdapter, uint8_t ucBssIndex,
		u_int8_t fgDbdcEn)
{
#if (CFG_SUPPORT_DBDC_DOWNGRADE_NSS == 1)
	if (fgDbdcEn)
		return 1;
#endif

	return wlanGetSupportNss(prAdapter, ucBssIndex);
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief    MT6632 HW capability will change between BW160+NSS2 and BW80+NSS1
 *
 * @param (none)
 *
 * @return TRUE: WAIT/WAIT FAIL/Done Success/Done Fail
 */
/*----------------------------------------------------------------------------*/
static enum ENUM_DBDC_PROTOCOL_STATUS_T cnmDbdcOpmodeChangeAndWait(
	struct ADAPTER *prAdapter,
	u_int8_t fgDbdcEn)
{
	uint8_t ucBssIndex;
	uint8_t ucTRxNss;
	struct BSS_INFO *prBssInfo;
	enum ENUM_CNM_OPMODE_REQ_STATUS eStatus;
	enum ENUM_DBDC_PROTOCOL_STATUS_T eRetVar =
		ENUM_DBDC_PROTOCOL_STATUS_DONE_SUCCESS;

#define IS_BSS_CLIENT(_prBssInfo) \
(_prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE)

	/* Always there are only up to 4 (BSSID_NUM) connected BSS. */
	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucHwBssIdNum && ucBssIndex < BSSID_NUM;
		ucBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
		ucTRxNss = cnmGetDbdcNss(prAdapter, ucBssIndex, fgDbdcEn);

		if (IS_BSS_ALIVE(prAdapter, prBssInfo)) {
			eStatus = cnmOpModeSetTRxNss(prAdapter,
					ucBssIndex,
					CNM_OPMODE_REQ_DBDC,
					fgDbdcEn,
					ucTRxNss, /* [DBDC] RxNss = TxNss */
					ucTRxNss);

			log_dbg(CNM, INFO, "[DBDC] BSS index[%u] to TRxNSS %u Mode:%s, status %u\n",
				ucBssIndex,
				ucTRxNss,
				IS_BSS_CLIENT(prBssInfo) ? "Client" : "Master",
				eStatus);

			switch (eStatus) {
			case CNM_OPMODE_REQ_STATUS_RUNNING:
			case CNM_OPMODE_REQ_STATUS_DEFER:
				g_rDbdcInfo.fgDbdcDisableOpmodeChangeDone
					= FALSE;
				g_rDbdcInfo.eBssOpModeState[ucBssIndex]
					= ENUM_OPMODE_STATE_WAIT;
				eRetVar = ENUM_DBDC_PROTOCOL_STATUS_WAIT;

				break;

			case CNM_OPMODE_REQ_STATUS_SUCCESS:
				g_rDbdcInfo.eBssOpModeState[ucBssIndex]
					= ENUM_OPMODE_STATE_DONE;
				break;

			case CNM_OPMODE_REQ_STATUS_INVALID_PARAM:
				g_rDbdcInfo.eBssOpModeState[ucBssIndex]
					= ENUM_OPMODE_STATE_FAIL;

#define __SUCCESS__	ENUM_DBDC_PROTOCOL_STATUS_DONE_SUCCESS
#define __FAIL__	ENUM_DBDC_PROTOCOL_STATUS_DONE_FAIL
				if (eRetVar == __SUCCESS__)
					eRetVar = __FAIL__;
#undef __SUCCESS__
#undef __FAIL__

				break;

			default:
				ASSERT(0);
				break;
			}
		} else {
			/* When DBDC is enabled, we limit all BSSes' OpTRxNss.
			 * Use the same API to update control table for
			 * inactive BSS.
			 */
			cnmOpModeSetTRxNss(prAdapter,
					ucBssIndex,
					CNM_OPMODE_REQ_DBDC,
					fgDbdcEn,
					ucTRxNss, /* [DBDC] RxNss = TxNss */
					ucTRxNss);
			g_rDbdcInfo.eBssOpModeState[ucBssIndex]
				= ENUM_OPMODE_STATE_DONE;
		}
	}

	return eRetVar;
}


void cnmDbdcOpModeChangeDoneCallback(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	bool fgSuccess)
{
	uint8_t ucBssLoopIndex;
	bool fgIsAllActionFrameSuccess = true;

	if (fgSuccess)
		g_rDbdcInfo.eBssOpModeState[ucBssIndex] =
			ENUM_OPMODE_STATE_DONE;
	else
		g_rDbdcInfo.eBssOpModeState[ucBssIndex] =
			ENUM_OPMODE_STATE_FAIL;

	log_dbg(CNM, INFO, "[DBDC] OPMODE STATE [%u/%u/%u/%u]\n",
	       g_rDbdcInfo.eBssOpModeState[BSSID_0],
	       g_rDbdcInfo.eBssOpModeState[BSSID_1],
	       g_rDbdcInfo.eBssOpModeState[BSSID_2],
	       g_rDbdcInfo.eBssOpModeState[BSSID_3]);

	for (ucBssLoopIndex = 0;
	     ucBssLoopIndex < prAdapter->ucHwBssIdNum;
	     ucBssLoopIndex++) {

		if (g_rDbdcInfo.eBssOpModeState[ucBssLoopIndex] ==
		    ENUM_OPMODE_STATE_WAIT)
			return;

		if (g_rDbdcInfo.eBssOpModeState[ucBssLoopIndex] ==
		    ENUM_OPMODE_STATE_FAIL &&
		    fgIsAllActionFrameSuccess == true) {
			/* Some OP mode change FAIL */
			fgIsAllActionFrameSuccess = false;
		}
	}

	if (!g_rDbdcInfo.fgDbdcDisableOpmodeChangeDone) {
		g_rDbdcInfo.fgDbdcDisableOpmodeChangeDone = true;

		if (fgIsAllActionFrameSuccess) {
			DBDC_FSM_EVENT_HANDLER(prAdapter,
				DBDC_FSM_EVENT_ACTION_FRAME_ALL_SUCCESS);
		} else {
			DBDC_FSM_EVENT_HANDLER(prAdapter,
				DBDC_FSM_EVENT_ACTION_FRAME_SOME_FAIL);
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief    Send DBDC Enable/Disable command to FW
 *
 * @param (none)
 *
 * @return (uint32_t)
 */
/*----------------------------------------------------------------------------*/
uint32_t cnmUpdateDbdcSetting(struct ADAPTER *prAdapter,
			  u_int8_t fgDbdcEn)
{
	struct CMD_DBDC_SETTING rDbdcSetting;
	struct CMD_DBDC_SETTING *prCmdBody;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo =
				prAdapter->rWifiVar.prP2pDevFsmInfo;
	u_int8_t fgIsP2pListening = FALSE;
#endif

	log_dbg(CNM, INFO, "[DBDC] %s\n",
	       fgDbdcEn ? "Enable" : "Disable");

	/* Send event to FW */
	prCmdBody = (struct CMD_DBDC_SETTING *)&rDbdcSetting;

	kalMemZero(prCmdBody, sizeof(struct CMD_DBDC_SETTING));

	prCmdBody->ucDbdcEn = fgDbdcEn;

		/* Parameter decision */
#if (CFG_HW_WMM_BY_BSS == 1)
	if (fgDbdcEn) {
		u_int8_t ucWmmSetBitmapPerBSS;
		struct BSS_INFO *prBssInfo;
		u_int8_t ucBssIndex;
		/*
		 * As DBDC enabled, for BSS use 2.4g Band, assign related
		 * WmmGroupSet bitmask to 1.
		 * This is used to indicate the WmmGroupSet is associated
		 * to Band#1 (otherwise, use for band#0)
		 */
		for (ucBssIndex = 0; ucBssIndex < prAdapter->ucHwBssIdNum;
			ucBssIndex++) {
			prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

			if (!prBssInfo || prBssInfo->fgIsInUse == FALSE)
				continue;

			if (prBssInfo->eBand == BAND_2G4) {
				ucWmmSetBitmapPerBSS = prBssInfo->ucWmmQueSet;
				prCmdBody->ucWmmBandBitmap |=
					BIT(ucWmmSetBitmapPerBSS);
			}
		}
		/* For P2P Device, we force it to use WMM3 */
		prBssInfo = prAdapter->aprBssInfo[P2P_DEV_BSS_INDEX];
		if (prBssInfo->eBand == BAND_2G4)
			prCmdBody->ucWmmBandBitmap |= BIT(MAX_HW_WMM_INDEX);
	}
#else
	if (fgDbdcEn)
		prCmdBody->ucWmmBandBitmap |= BIT(DBDC_2G_WMM_INDEX);
#endif

#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
	if (fgDbdcEn) {
		if (prP2pDevFsmInfo
			&& prP2pDevFsmInfo->fgIsP2pListening) {
			fgIsP2pListening = prP2pDevFsmInfo->fgIsP2pListening;
			prP2pDevFsmInfo->fgIsP2pListening = FALSE;
			/* p2p dev req, just compare all active bss*/
			if (g_rDbdcInfo.ucBssIdx == prAdapter->ucP2PDevBssIdx) {
				if (!cnmDbdcIsConcurrent(prAdapter,
					BAND_NULL,
					0))
					g_rDbdcInfo.fgIsDBDCEnByP2pLis = TRUE;
			} else {
			/*
			 * non p2p dev req, use req band/ch compare
			 * to all active bss
			 */
				if (!cnmDbdcIsConcurrent(prAdapter,
					g_rDbdcInfo.eRfBand,
					g_rDbdcInfo.ucPrimaryChannel))
				g_rDbdcInfo.fgIsDBDCEnByP2pLis = TRUE;
			}
			prP2pDevFsmInfo->fgIsP2pListening =
							fgIsP2pListening;
		}
	} else {
		g_rDbdcInfo.fgIsDBDCEnByP2pLis = FALSE;
	}

	log_dbg(CNM, ERROR, "fgDbdcEn=%d, fgIsDBDCEnByP2pLis=%d\n",
		fgDbdcEn, g_rDbdcInfo.fgIsDBDCEnByP2pLis);
#endif

	/* FW uses ucWmmBandBitmap from driver if it does not support ver 1*/
	prCmdBody->ucCmdVer = 0x1;
	prCmdBody->u2CmdLen = sizeof(struct CMD_DBDC_SETTING);
	DBDC_UPDATE_CMD_WMMBAND_FW_AUTO(prCmdBody);

	if (g_rDbdcInfo.fgHasSentCmd == TRUE)
		log_dbg(CNM, WARN, "Not event came back for DBDC\n");

	g_rDbdcInfo.fgHasSentCmd = TRUE;
	g_rDbdcInfo.fgCmdEn = fgDbdcEn;

	/* Set DBDC A+A Mode to FW */
	if (g_rDbdcInfo.fgIsDBDCAAMode == TRUE)
		prCmdBody->ucDBDCAAMode = 1;

	log_dbg(CNM, WARN, "fgDbdcEn=%d, ucDBDCAAMode=%d\n",
		g_rDbdcInfo.fgCmdEn, prCmdBody->ucDBDCAAMode);

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				      CMD_ID_SET_DBDC_PARMS,	/* ucCID */
				      TRUE, /* fgSetQuery */
				      FALSE,	/* fgNeedResp */
				      FALSE,	/* fgIsOid */
				      NULL, /* pfCmdDoneHandler */
				      NULL, /* pfCmdTimeoutHandler */

				      /* u4SetQueryInfoLen */
				      sizeof(struct CMD_DBDC_SETTING),

				      /* pucInfoBuffer */
				      (uint8_t *)prCmdBody,

				      NULL, /* pvSetQueryBuffer */
				      0 /* u4SetQueryBufferLen */);

	if (rStatus != WLAN_STATUS_PENDING)
		DBGLOG(CNM, WARN,
			"cnmUpdateDbdcSetting set cmd fail %d\n", rStatus);

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief DBDC FSM Entry
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static void
cnmDbdcFsmSteps(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_STATE_T   eNextState,
	enum ENUM_DBDC_FSM_EVENT_T   eEvent)
{
	/* Do entering Next State and do its initial function. */
	g_rDbdcInfo.eDbdcFsmPrevState = g_rDbdcInfo.eDbdcFsmCurrState;
	g_rDbdcInfo.eDbdcFsmCurrState = eNextState;
	g_rDbdcInfo.eDbdcFsmNextState = eNextState;

	log_dbg(CNM, INFO, "[DBDC] event %d state %d->%d\n",
	       eEvent,
	       g_rDbdcInfo.eDbdcFsmPrevState,
	       g_rDbdcInfo.eDbdcFsmCurrState);

	if (g_rDbdcInfo.eDbdcFsmPrevState < 0 ||
		g_rDbdcInfo.eDbdcFsmPrevState >=
		sizeof(arDdbcFsmActionTable) / sizeof(struct DBDC_FSM_T)) {
		log_dbg(CNM, INFO, "Invalid state[%d]\n",
			g_rDbdcInfo.eDbdcFsmPrevState);
		return;
	}
	if (g_rDbdcInfo.eDbdcFsmPrevState != g_rDbdcInfo.eDbdcFsmCurrState) {
		/* state change, call exit function of previous state */
		if (arDdbcFsmActionTable[g_rDbdcInfo.eDbdcFsmPrevState]
			.pfExitFunc) {
			arDdbcFsmActionTable[g_rDbdcInfo.eDbdcFsmPrevState]
				.pfExitFunc(prAdapter);
		}

		/* state change, call entry function of current state */
		if (arDdbcFsmActionTable[g_rDbdcInfo.eDbdcFsmCurrState]
			.pfEntryFunc) {
			arDdbcFsmActionTable[g_rDbdcInfo.eDbdcFsmCurrState]
				.pfEntryFunc(prAdapter);
		}
	}
}

u_int8_t
cnmDBDCIsReqPeivilegeLock(void)
{
	return g_rDbdcInfo.fgReqPrivelegeLock;
}

static void
cnmDBDCFsmActionReqPeivilegeLock(void)
{
	g_rDbdcInfo.fgReqPrivelegeLock = TRUE;
	g_rDbdcInfo.rPeivilegeLockTime = kalGetTimeTick();
	log_dbg(CNM, INFO, "[DBDC] ReqPrivelege Lock!!\n");
}

static void
cnmDBDCFsmActionReqPeivilegeUnLock(struct ADAPTER *prAdapter)
{
	struct MSG_CH_REQ *prPendingMsg;
	struct MSG_HDR *prMsgHdr;

	g_rDbdcInfo.fgReqPrivelegeLock = FALSE;
	g_rDbdcInfo.rPeivilegeLockTime = 0;
	log_dbg(CNM, INFO, "[DBDC] ReqPrivelege Unlock!!\n");

	while (!LINK_IS_EMPTY(&g_rDbdcInfo.rPendingMsgList)) {

		LINK_REMOVE_HEAD(&g_rDbdcInfo.rPendingMsgList, prMsgHdr,
				 struct MSG_HDR *);

		if (prMsgHdr) {
			prPendingMsg = (struct MSG_CH_REQ *)prMsgHdr;

			log_dbg(CNM, INFO, "[DBDC] ChReq: send queued REQ of Token %u\n",
				prPendingMsg->ucTokenID);

			cnmChMngrRequestPrivilege(prAdapter,
						  &prPendingMsg->rMsgHdr);
		} else {
			ASSERT(0);
		}
	}
}

static void
cnmDbdcFsmEntryFunc_DISABLE_IDLE(struct ADAPTER *prAdapter)
{
	uint8_t ucWmmIndex;
	uint8_t ucBssIndex;
	struct CNM_OPMODE_BSS_CONTROL_T *prBssOpCtrl;

	if (cnmDBDCIsReqPeivilegeLock()) {
		cnmDBDCFsmActionReqPeivilegeUnLock(prAdapter);
	}

	for (ucBssIndex = 0; ucBssIndex < prAdapter->ucHwBssIdNum;
		ucBssIndex++) {
		prBssOpCtrl = &(g_arBssOpControl[ucBssIndex]);
		prBssOpCtrl->rRunning.fgIsRunning = false;
		prBssOpCtrl->arReqPool[CNM_OPMODE_REQ_DBDC].fgEnable = false;
	}

	for (ucWmmIndex = 0; ucWmmIndex < prAdapter->ucWmmSetNum;
		ucWmmIndex++) {
		cnmWmmQuotaSetMaxQuota(
			prAdapter,
			ucWmmIndex,
			CNM_WMM_REQ_DBDC,
			false,
			0 /* don't care */);
	}
}

static void
cnmDbdcFsmEntryFunc_WAIT_PROTOCOL_ENABLE(struct ADAPTER *prAdapter)
{
	if (!cnmDBDCIsReqPeivilegeLock())
		cnmDBDCFsmActionReqPeivilegeLock();
}

static void
cnmDbdcFsmEntryFunc_WAIT_HW_ENABLE(struct ADAPTER *prAdapter)
{
	uint32_t rStatus;

	if (!cnmDBDCIsReqPeivilegeLock())
		cnmDBDCFsmActionReqPeivilegeLock();

	rStatus = cnmUpdateDbdcSetting(prAdapter, TRUE);

	if (rStatus != WLAN_STATUS_PENDING) {
		cnmDBDCFsmActionReqPeivilegeUnLock(prAdapter);
		DBDC_FSM_EVENT_HANDLER(prAdapter,
			DBDC_FSM_EVENT_ERR);
	}
}

static void
cnmDbdcFsmEntryFunc_ENABLE_GUARD(struct ADAPTER *prAdapter)
{
	if (timerPendingTimer(&g_rDbdcInfo.rDbdcGuardTimer)) {
		log_dbg(CNM, WARN,
		       "[DBDC] Guard Timer type %u should not exist, stop it\n",
		       g_rDbdcInfo.eDdbcGuardTimerType);
		cnmTimerStopTimer(prAdapter,
				  &g_rDbdcInfo.rDbdcGuardTimer);
		g_rDbdcInfo.eDdbcGuardTimerType =
			ENUM_DBDC_GUARD_TIMER_NONE;
	}
	DBDC_SET_GUARD_TIME(prAdapter, DBDC_ENABLE_GUARD_TIME);
}

static void
cnmDbdcFsmEntryFunc_ENABLE_IDLE(
	struct ADAPTER *prAdapter
)
{
	uint8_t ucWmmIndex;
	uint32_t u4ReqQuota = DBDC_WMM_TX_QUOTA;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;

	for (ucWmmIndex = 0; ucWmmIndex < prAdapter->ucWmmSetNum;
		ucWmmIndex++) {

		if (prChipInfo->dmashdlQuotaDecision) {
			u4ReqQuota =
				prChipInfo->dmashdlQuotaDecision(
					prAdapter,
					ucWmmIndex);
		}

		cnmWmmQuotaSetMaxQuota(
			prAdapter,
			ucWmmIndex,
			CNM_WMM_REQ_DBDC,
			true,
			u4ReqQuota);
	}
}


static void
cnmDbdcFsmEntryFunc_WAIT_HW_DISABLE(struct ADAPTER *prAdapter)
{
	uint32_t rStatus;

#if (CFG_SUPPORT_DBDC_NO_BLOCKING_OPMODE)
	if (!cnmDBDCIsReqPeivilegeLock())
		cnmDBDCFsmActionReqPeivilegeLock();
#endif

	rStatus = cnmUpdateDbdcSetting(prAdapter, FALSE);

	if (rStatus != WLAN_STATUS_PENDING) {
		cnmDBDCFsmActionReqPeivilegeUnLock(prAdapter);
		DBDC_FSM_EVENT_HANDLER(prAdapter,
			DBDC_FSM_EVENT_ERR);
	}
}

static void
cnmDbdcFsmEntryFunc_DISABLE_GUARD(struct ADAPTER *prAdapter)
{
	/* Do nothing if we will enter A+G immediately */
	if (g_rDbdcInfo.fgPostpondEnterAG)
		return;

	if (timerPendingTimer(&g_rDbdcInfo.rDbdcGuardTimer)) {
		log_dbg(CNM, WARN,
		       "[DBDC] Guard Timer type %u should not exist, stop it\n",
		       g_rDbdcInfo.eDdbcGuardTimerType);
		cnmTimerStopTimer(prAdapter,
				  &g_rDbdcInfo.rDbdcGuardTimer);
		g_rDbdcInfo.eDdbcGuardTimerType =
			ENUM_DBDC_GUARD_TIMER_NONE;
	}
	DBDC_SET_GUARD_TIME(prAdapter, DBDC_DISABLE_GUARD_TIME);

	cnmDbdcOpmodeChangeAndWait(prAdapter, FALSE);
}

static void
cnmDbdcFsmEventHandler_DISABLE_IDLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T	eEvent)
{
	switch (eEvent) {
	case DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG:
		/* Do Nothing */
		break;

	case DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG:
		/* Enable DBDC */
		switch (cnmDbdcOpmodeChangeAndWait(prAdapter, TRUE)) {
		case ENUM_DBDC_PROTOCOL_STATUS_WAIT:
			g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_WAIT_PROTOCOL_ENABLE;
			break;

		case ENUM_DBDC_PROTOCOL_STATUS_DONE_SUCCESS:
			g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE;
			break;

		case ENUM_DBDC_PROTOCOL_STATUS_DONE_FAIL:
#if (CFG_SUPPORT_DBDC_NO_BLOCKING_OPMODE)
			log_dbg(CNM, WARN,
				"[DBDC] OPMode Fail, ForceEn at state %d\n",
				g_rDbdcInfo.eDbdcFsmCurrState);
			g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE;
			break;
#endif

		default:
			break;
		}
		break;

	case DBDC_FSM_EVENT_SWITCH_GUARD_TIME_TO:
	case DBDC_FSM_EVENT_DISABLE_COUNT_DOWN_TO:
	case DBDC_FSM_EVENT_ACTION_FRAME_ALL_SUCCESS:
	case DBDC_FSM_EVENT_ACTION_FRAME_SOME_FAIL:
	case DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE:
		/* ABNORMAL CASE*/
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;
	default:
		/* WRONG EVENT */
		DBDC_FSM_MSG_ERROR_EVT(eEvent);
		break;
	}

	cnmDbdcFsmSteps(prAdapter, g_rDbdcInfo.eDbdcFsmNextState, eEvent);
}

static void
cnmDbdcFsmEventHandler_WAIT_PROTOCOL_ENABLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T   eEvent)
{
	switch (eEvent) {
	case DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG:
		/* Stop Enabling DBDC */
		g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_DISABLE_IDLE;
		break;

	case DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG:
		/* IGNORE */
		break;

	case DBDC_FSM_EVENT_SWITCH_GUARD_TIME_TO:
	case DBDC_FSM_EVENT_DISABLE_COUNT_DOWN_TO:
		/* ABNORMAL CASE*/
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

	case DBDC_FSM_EVENT_ACTION_FRAME_ALL_SUCCESS:
		g_rDbdcInfo.eDbdcFsmNextState =
		ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE;
		break;

	case DBDC_FSM_EVENT_ACTION_FRAME_SOME_FAIL:
#if (CFG_SUPPORT_DBDC_NO_BLOCKING_OPMODE)
		g_rDbdcInfo.eDbdcFsmNextState =
		ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE;
		log_dbg(CNM, WARN,
			"[DBDC] OPMode Fail, ForceEn at state %d\n",
			g_rDbdcInfo.eDbdcFsmCurrState);
#else
		/* Not recover anything. Stop Enable DBDC */
		g_rDbdcInfo.eDbdcFsmNextState =
		ENUM_DBDC_FSM_STATE_DISABLE_IDLE;
#endif

		break;

	case DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE:
		/* ABNORMAL CASE*/
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

	default:
		/* WRONG EVENT */
		DBDC_FSM_MSG_ERROR_EVT(eEvent);
		break;
	}

	cnmDbdcFsmSteps(prAdapter, g_rDbdcInfo.eDbdcFsmNextState, eEvent);
}

static void
cnmDbdcFsmEventHandler_WAIT_HW_ENABLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T   eEvent)
{
	/* Prepare to Enable DBDC */

	switch (eEvent) {
	case DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG:
		g_rDbdcInfo.fgPostpondLeaveAG = TRUE;
		break;

	case DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG:
		g_rDbdcInfo.fgPostpondLeaveAG = FALSE;
		break;

	case DBDC_FSM_EVENT_SWITCH_GUARD_TIME_TO:
	case DBDC_FSM_EVENT_DISABLE_COUNT_DOWN_TO:
	case DBDC_FSM_EVENT_ACTION_FRAME_ALL_SUCCESS:
	case DBDC_FSM_EVENT_ACTION_FRAME_SOME_FAIL:
		/* ABNORMAL CASE*/
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

	case DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE:
		g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_ENABLE_GUARD;
		break;

	case DBDC_FSM_EVENT_ERR:
		g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_DISABLE_IDLE;
		g_rDbdcInfo.fgPostpondLeaveAG = FALSE;
		break;

	default:
		/* WRONG EVENT */
		DBDC_FSM_MSG_ERROR_EVT(eEvent);
		break;
	}

	cnmDbdcFsmSteps(prAdapter, g_rDbdcInfo.eDbdcFsmNextState, eEvent);

	/* Leave A+G immediately */
	if (eEvent == DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE &&
		g_rDbdcInfo.fgPostpondLeaveAG) {
		DBDC_FSM_EVENT_HANDLER(prAdapter,
			DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG);

		g_rDbdcInfo.fgPostpondLeaveAG = FALSE;
	}
}


static void
cnmDbdcFsmEventHandler_ENABLE_GUARD(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T   eEvent)
{
	switch (eEvent) {
	case DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG:
		/* stop guard timer */
		if (timerPendingTimer(&g_rDbdcInfo.rDbdcGuardTimer)) {
			log_dbg(CNM, WARN, "[DBDC] Stop Guard Timer type %u\n",
				g_rDbdcInfo.eDdbcGuardTimerType);
			cnmTimerStopTimer(prAdapter,
					  &g_rDbdcInfo.rDbdcGuardTimer);
			g_rDbdcInfo.eDdbcGuardTimerType =
			ENUM_DBDC_GUARD_TIMER_NONE;
		}
		/* directly enter HW disable state */
		if (!cnmDbdcIsConcurrent(prAdapter, BAND_NULL, 0))
			g_rDbdcInfo.eDbdcFsmNextState =
				ENUM_DBDC_FSM_STATE_WAIT_HW_DISABLE;
		break;

	case DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG:
		/* IGNORE */
		break;

	case DBDC_FSM_EVENT_SWITCH_GUARD_TIME_TO:
		/* Exit DBDC if non A+G */
		if (!cnmDbdcIsConcurrent(prAdapter, BAND_NULL, 0)) {
			g_rDbdcInfo.eDbdcFsmNextState =
				ENUM_DBDC_FSM_STATE_WAIT_HW_DISABLE;
		} else {
			g_rDbdcInfo.eDbdcFsmNextState =
				ENUM_DBDC_FSM_STATE_ENABLE_IDLE;
		}
		break;

	case DBDC_FSM_EVENT_DISABLE_COUNT_DOWN_TO:
	case DBDC_FSM_EVENT_ACTION_FRAME_ALL_SUCCESS:
	case DBDC_FSM_EVENT_ACTION_FRAME_SOME_FAIL:
	case DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE:
		/* ABNORMAL CASE*/
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

	default:
		/* WRONG EVENT */
		DBDC_FSM_MSG_ERROR_EVT(eEvent);
		break;
	}

	cnmDbdcFsmSteps(prAdapter, g_rDbdcInfo.eDbdcFsmNextState, eEvent);
}

static void
cnmDbdcFsmEventHandler_ENABLE_IDLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T   eEvent)
{
	switch (eEvent) {
	case DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG:
		/* stop guard timer */
		if (timerPendingTimer(&g_rDbdcInfo.rDbdcGuardTimer)) {
			log_dbg(CNM, WARN, "[DBDC] Guard Timer type %u should not exist, stop it\n",
				g_rDbdcInfo.eDdbcGuardTimerType);
			cnmTimerStopTimer(prAdapter,
					  &g_rDbdcInfo.rDbdcGuardTimer);
			g_rDbdcInfo.eDdbcGuardTimerType =
			ENUM_DBDC_GUARD_TIMER_NONE;
		}
		/* directly enter HW disable state */
		if (!cnmDbdcIsConcurrent(prAdapter, BAND_NULL, 0))
			g_rDbdcInfo.eDbdcFsmNextState =
				ENUM_DBDC_FSM_STATE_WAIT_HW_DISABLE;
		break;

	case DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG:
		/* cancel DBDC disable countdown if exist */
		if (timerPendingTimer(&g_rDbdcInfo.rDbdcGuardTimer) &&
		    g_rDbdcInfo.eDdbcGuardTimerType ==
		    ENUM_DBDC_GUARD_TIMER_DISABLE_COUNT_DOWN) {
			cnmTimerStopTimer(prAdapter,
					  &g_rDbdcInfo.rDbdcGuardTimer);
		}
		break;

	case DBDC_FSM_EVENT_SWITCH_GUARD_TIME_TO:
		/* ABNORMAL CASE*/
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

	case DBDC_FSM_EVENT_DISABLE_COUNT_DOWN_TO:
		if (!cnmDbdcIsConcurrent(prAdapter, BAND_NULL, 0))
			g_rDbdcInfo.eDbdcFsmNextState =
				ENUM_DBDC_FSM_STATE_WAIT_HW_DISABLE;
		break;

	case DBDC_FSM_EVENT_ACTION_FRAME_ALL_SUCCESS:
	case DBDC_FSM_EVENT_ACTION_FRAME_SOME_FAIL:
	case DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE:
		/* ABNORMAL CASE*/
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

	default:
		/* WRONG EVENT */
		DBDC_FSM_MSG_ERROR_EVT(eEvent);
		break;
	}

	cnmDbdcFsmSteps(prAdapter, g_rDbdcInfo.eDbdcFsmNextState, eEvent);
}

static void
cnmDbdcFsmEventHandler_WAIT_HW_DISABLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T   eEvent)
{
	switch (eEvent) {
	case DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG:
		g_rDbdcInfo.fgPostpondEnterAG = FALSE;
		break;

	case DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG:
		g_rDbdcInfo.fgPostpondEnterAG = TRUE;
		break;

	case DBDC_FSM_EVENT_SWITCH_GUARD_TIME_TO:
	case DBDC_FSM_EVENT_DISABLE_COUNT_DOWN_TO:
	case DBDC_FSM_EVENT_ACTION_FRAME_ALL_SUCCESS:
	case DBDC_FSM_EVENT_ACTION_FRAME_SOME_FAIL:
		/* ABNORMAL CASE*/
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

	case DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE:
		g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_DISABLE_GUARD;
		break;

	case DBDC_FSM_EVENT_ERR:
		g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_ENABLE_IDLE;
		g_rDbdcInfo.fgPostpondEnterAG = FALSE;
		break;

	default:
		/* WRONG EVENT */
		DBDC_FSM_MSG_ERROR_EVT(eEvent);
		break;
	}

	cnmDbdcFsmSteps(prAdapter, g_rDbdcInfo.eDbdcFsmNextState, eEvent);

	/* Enter A+G immediately */
	if (eEvent == DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE &&
		g_rDbdcInfo.fgPostpondEnterAG) {
		DBDC_FSM_EVENT_HANDLER(prAdapter,
			DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG);

		g_rDbdcInfo.fgPostpondEnterAG = FALSE;
	}
}

static void
cnmDbdcFsmEventHandler_DISABLE_GUARD(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T   eEvent)
{
	switch (eEvent) {
	case DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG:
		/* IGNORE */
		break;

	case DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG:
		/* Enable DBDC */
		switch (cnmDbdcOpmodeChangeAndWait(prAdapter, TRUE)) {
		case ENUM_DBDC_PROTOCOL_STATUS_WAIT:
			g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_WAIT_PROTOCOL_ENABLE;
			break;

		case ENUM_DBDC_PROTOCOL_STATUS_DONE_SUCCESS:
			g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE;
			break;

		case ENUM_DBDC_PROTOCOL_STATUS_DONE_FAIL:
#if (CFG_SUPPORT_DBDC_NO_BLOCKING_OPMODE)
			log_dbg(CNM, WARN,
				"[DBDC] OPMode Fail, ForceEn at state %d\n",
				g_rDbdcInfo.eDbdcFsmCurrState);
			g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE;
			break;
#endif

		default:
			break;
		}
		break;

	case DBDC_FSM_EVENT_SWITCH_GUARD_TIME_TO:

#define __PRO_ENABLE__	ENUM_DBDC_FSM_STATE_WAIT_PROTOCOL_ENABLE
#define __PRO_DISABLE__	ENUM_DBDC_FSM_STATE_WAIT_PROTOCOL_DISABLE
#define __HW_ENABLE__	ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE
#define __DISABLE__	ENUM_DBDC_FSM_STATE_DISABLE_IDLE
#define __STAT_WAIT__	ENUM_DBDC_PROTOCOL_STATUS_WAIT

		if (g_rDbdcInfo.fgDbdcDisableOpmodeChangeDone) {
			if (cnmDbdcIsConcurrent(prAdapter, BAND_NULL, 0)) {
				switch (cnmDbdcOpmodeChangeAndWait(
					prAdapter, TRUE)) {
				case ENUM_DBDC_PROTOCOL_STATUS_WAIT:
					g_rDbdcInfo.eDbdcFsmNextState =
						__PRO_ENABLE__;
					break;
				case ENUM_DBDC_PROTOCOL_STATUS_DONE_SUCCESS:
					g_rDbdcInfo.eDbdcFsmNextState =
						__HW_ENABLE__;
					break;
				case ENUM_DBDC_PROTOCOL_STATUS_DONE_FAIL:
#if (CFG_SUPPORT_DBDC_NO_BLOCKING_OPMODE)
					g_rDbdcInfo.eDbdcFsmNextState =
						__HW_ENABLE__;
					log_dbg(CNM, WARN,
						"[DBDC] OPMode Fail, ForceEn at state %d\n",
						g_rDbdcInfo.eDbdcFsmCurrState);
#else
					if (cnmDbdcOpmodeChangeAndWait(
						prAdapter, FALSE)
						== __STAT_WAIT__)
						g_rDbdcInfo.eDbdcFsmNextState =
							__PRO_DISABLE__;
					else
						g_rDbdcInfo.eDbdcFsmNextState =
							__DISABLE__;
#endif
					break;
				default:
					break;
				}
			} else {
				g_rDbdcInfo.eDbdcFsmNextState =
					__DISABLE__;
			}
		} else {
			g_rDbdcInfo.eDbdcFsmNextState =
				__PRO_DISABLE__;
		}

#undef __PRO_ENABLE__
#undef __PRO_DISABLE__
#undef __HW_ENABLE__
#undef __DISABLE__
#undef __STAT_WAIT__

		break;

	case DBDC_FSM_EVENT_DISABLE_COUNT_DOWN_TO:
		/* ABNORMAL CASE */
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

	case DBDC_FSM_EVENT_ACTION_FRAME_ALL_SUCCESS:
	case DBDC_FSM_EVENT_ACTION_FRAME_SOME_FAIL:
		/* Do nothing */
		break;

	case DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE:
		/* ABNORMAL CASE */
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

	default:
		/* WRONG EVENT */
		DBDC_FSM_MSG_ERROR_EVT(eEvent);
		break;
	}

	cnmDbdcFsmSteps(prAdapter, g_rDbdcInfo.eDbdcFsmNextState, eEvent);
}

static void
cnmDbdcFsmEventHandler_WAIT_PROTOCOL_DISABLE(
	struct ADAPTER *prAdapter,
	enum ENUM_DBDC_FSM_EVENT_T   eEvent)
{
	/* Prepare to Enable DBDC */

	switch (eEvent) {
	case DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG:
		/* Return to idle state to prevent getting stuck */
		g_rDbdcInfo.eDbdcFsmNextState =
			ENUM_DBDC_FSM_STATE_DISABLE_IDLE;
		break;
	case DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG:
		/* IGNORE */
		break;

	case DBDC_FSM_EVENT_SWITCH_GUARD_TIME_TO:
	case DBDC_FSM_EVENT_DISABLE_COUNT_DOWN_TO:
		/* ABNORMAL CASE*/
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

#define __PRO_ENABLE__	ENUM_DBDC_FSM_STATE_WAIT_PROTOCOL_ENABLE

	case DBDC_FSM_EVENT_ACTION_FRAME_ALL_SUCCESS:
	case DBDC_FSM_EVENT_ACTION_FRAME_SOME_FAIL:
		if (cnmDbdcIsConcurrent(prAdapter, BAND_NULL, 0)) {
			switch (cnmDbdcOpmodeChangeAndWait(prAdapter, TRUE)) {
			case ENUM_DBDC_PROTOCOL_STATUS_WAIT:
				g_rDbdcInfo.eDbdcFsmNextState =
					__PRO_ENABLE__;
				break;
			case ENUM_DBDC_PROTOCOL_STATUS_DONE_SUCCESS:
				g_rDbdcInfo.eDbdcFsmNextState =
					ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE;
				break;
			case ENUM_DBDC_PROTOCOL_STATUS_DONE_FAIL:
#if (CFG_SUPPORT_DBDC_NO_BLOCKING_OPMODE)
				g_rDbdcInfo.eDbdcFsmNextState =
					ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE;
				log_dbg(CNM, WARN,
					"[DBDC] OPMode Fail, ForceEn at state %d\n",
					g_rDbdcInfo.eDbdcFsmCurrState);
#else
				g_rDbdcInfo.eDbdcFsmNextState =
					ENUM_DBDC_FSM_STATE_DISABLE_IDLE;
#endif
				break;
			default:
				break;
			}
		} else
			g_rDbdcInfo.eDbdcFsmNextState =
				ENUM_DBDC_FSM_STATE_DISABLE_IDLE;
		break;

#undef __PRO_ENABLE__

	case DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE:
		/* ABNORMAL CASE*/
		DBDC_FSM_MSG_WRONG_EVT(eEvent);
		break;

	default:
		/* WRONG EVENT */
		DBDC_FSM_MSG_ERROR_EVT(eEvent);
		break;
	}

	cnmDbdcFsmSteps(prAdapter, g_rDbdcInfo.eDbdcFsmNextState, eEvent);
}

static void
cnmDbdcFsmExitFunc_WAIT_HW_ENABLE(
	struct ADAPTER *prAdapter)
{
	cnmDBDCFsmActionReqPeivilegeUnLock(prAdapter);
}

static void
cnmDbdcFsmExitFunc_WAIT_HW_DISABLE(
	struct ADAPTER *prAdapter)
{
	/* Do not release privilege lock if we will enter A+G immediately */
	if (!g_rDbdcInfo.fgPostpondEnterAG)
		cnmDBDCFsmActionReqPeivilegeUnLock(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief check whether DBDC is disabled
 *        to avoid STA disable DBDC during roaming, which may cause DBDC->MCC,
 *        check DBDC status before cnmDbdcPreConnectionEnableDecision
 *
 * @param ADAPTER
 *
 * @return boolean
 */
/*----------------------------------------------------------------------------*/
bool cnmDbdcIsDisabled(struct ADAPTER *prAdapter)
{
	if (prAdapter->rWifiVar.fgDbDcModeEn == FALSE)
		return TRUE;

	if (g_rDbdcInfo.fgHasSentCmd == TRUE &&
		g_rDbdcInfo.fgCmdEn == FALSE)
		return TRUE;

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Get maximum bandwidth capability with considering DBDC mode
 *
 * @param (none)
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
uint8_t cnmGetDbdcBwCapability(struct ADAPTER
			       *prAdapter,
			       uint8_t ucBssIndex)
{
	uint8_t ucMaxBw = MAX_BW_20MHZ;

	ucMaxBw = cnmGetBssMaxBw(prAdapter, ucBssIndex);

#if (CFG_SUPPORT_DBDC_DOWNGRADE_BW == 1)
	/* Can't use BW160 when DBDC enabled */
	if (USE_DBDC_CAPABILITY() && (ucMaxBw >= MAX_BW_160MHZ))
		ucMaxBw = MAX_BW_80MHZ;
#endif

	return ucMaxBw;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief    Run-time check if DBDC Need enable or update guard time.
 *           The WmmQ is set to the correct DBDC band before connetcting.
 *           It could make sure the TxPath is correct after connected.
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmDbdcPreConnectionEnableDecision(
	struct ADAPTER *prAdapter,
	uint8_t		ucChangedBssIndex,
	enum ENUM_BAND	eRfBand,
	uint8_t ucPrimaryChannel,
	uint8_t ucWmmQueIdx)
{
	log_dbg(CNM, INFO, "[DBDC] BSS %u Rf %u", ucChangedBssIndex, eRfBand);

	if (prAdapter->rWifiVar.eDbdcMode != ENUM_DBDC_MODE_DYNAMIC &&
		(prAdapter->rWifiVar.eDbdcMode != ENUM_DBDC_MODE_STATIC)) {
		log_dbg(CNM, INFO, "[DBDC Debug] DBDC Mode %u Return",
		       prAdapter->rWifiVar.eDbdcMode);
		return;
	}

	if (prAdapter->rWifiVar.eDbdcMode == ENUM_DBDC_MODE_STATIC &&
		prAdapter->rWifiVar.fgDbDcModeEn) {
		if (timerPendingTimer(&g_rDbdcInfo.rDbdcGuardTimer) &&
		    g_rDbdcInfo.eDdbcGuardTimerType ==
		    ENUM_DBDC_GUARD_TIMER_SWITCH_GUARD_TIME) {
			/* update timer for connection retry */
			log_dbg(CNM, INFO, "[DBDC] DBDC guard time extend\n");
			cnmTimerStopTimer(prAdapter,
					  &g_rDbdcInfo.rDbdcGuardTimer);
			cnmTimerStartTimer(prAdapter,
					   &g_rDbdcInfo.rDbdcGuardTimer,
					   DBDC_ENABLE_GUARD_TIME);
		}
		/* The DBDC is already ON, so renew WMM band information only */
		DBDC_SET_WMMBAND_FW_AUTO_BY_CHNL(ucPrimaryChannel,
			ucWmmQueIdx, eRfBand, ucChangedBssIndex);
		cnmUpdateDbdcSetting(prAdapter, TRUE);
		return;
	}

	if (timerPendingTimer(&g_rDbdcInfo.rDbdcGuardTimer) &&
		g_rDbdcInfo.eDdbcGuardTimerType
		== ENUM_DBDC_GUARD_TIMER_SWITCH_GUARD_TIME) {
		log_dbg(CNM, INFO, "[DBDC Debug] Guard Time Check");

		if ((cnmDbdcIsConcurrent(prAdapter, eRfBand,
				ucPrimaryChannel)
			&& !prAdapter->rWifiVar.fgDbDcModeEn) ||
			(!cnmDbdcIsConcurrent(prAdapter, eRfBand,
				ucPrimaryChannel)
			&& prAdapter->rWifiVar.fgDbDcModeEn)) {
			/* cancel Guard Time and change DBDC mode */
			cnmTimerStopTimer(prAdapter,
				&g_rDbdcInfo.rDbdcGuardTimer);
			g_rDbdcInfo.eDdbcGuardTimerType =
				ENUM_DBDC_GUARD_TIMER_NONE;
		} else {
			log_dbg(CNM, INFO, "[DBDC Debug] Guard Time extend Return");
			cnmTimerStopTimer(prAdapter,
					  &g_rDbdcInfo.rDbdcGuardTimer);
			cnmTimerStartTimer(prAdapter,
					   &g_rDbdcInfo.rDbdcGuardTimer,
					   DBDC_ENABLE_GUARD_TIME);
			return;
		}
	}

	if (eRfBand == BAND_NULL) {
		log_dbg(CNM, INFO, "[DBDC Debug] Wrong RF band Return");
		return;
	}

	if (cnmDbdcIsConcurrent(prAdapter, eRfBand, ucPrimaryChannel)) {
		DBDC_SET_WMMBAND_FW_AUTO_BY_CHNL(ucPrimaryChannel,
			ucWmmQueIdx, eRfBand, ucChangedBssIndex);
		DBDC_FSM_EVENT_HANDLER(prAdapter,
			DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG);
	} else {
		DBDC_FSM_EVENT_HANDLER(prAdapter,
			DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief    Run-time check if we need enable/disable DBDC or update guard time.
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmDbdcRuntimeCheckDecision(struct ADAPTER
			    *prAdapter,
			    uint8_t ucChangedBssIndex,
			    u_int8_t ucForceLeaveEnGuard)
{
	bool fgIsAgConcurrent;
#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo =
				prAdapter->rWifiVar.prP2pDevFsmInfo;
	u_int8_t fgIsP2pListening = FALSE;
#endif

	log_dbg(CNM, INFO, "[DBDC Debug] BSS %u",
	       ucChangedBssIndex);

	/* Only allow runtime switch for dynamic DBDC */
	if (prAdapter->rWifiVar.eDbdcMode !=
	    ENUM_DBDC_MODE_DYNAMIC) {
		log_dbg(CNM, INFO, "[DBDC Debug] DBDC Mode %u Return",
		       prAdapter->rWifiVar.eDbdcMode);
		return;
	}

	/* AGConcurrent status sync with DBDC satus. Do nothing. */
	fgIsAgConcurrent = cnmDbdcIsConcurrent(prAdapter, BAND_NULL, 0);
	if (fgIsAgConcurrent ==
		prAdapter->rWifiVar.fgDbDcModeEn) {
#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
		if (fgIsAgConcurrent && prP2pDevFsmInfo) {
			log_dbg(CNM, INFO,
				"[DBDC Debug] DBDC %u EnByP2pLis %u",
				prAdapter->rWifiVar.fgDbDcModeEn,
				g_rDbdcInfo.fgIsDBDCEnByP2pLis);

			if (prP2pDevFsmInfo->fgIsP2pListening) {
				if (ucChangedBssIndex !=
					prAdapter->ucP2PDevBssIdx) {
					fgIsP2pListening =
					prP2pDevFsmInfo->
							fgIsP2pListening;
					prP2pDevFsmInfo->
						fgIsP2pListening = FALSE;

					if (!cnmDbdcIsConcurrent(prAdapter,
						BAND_NULL,
						0)) {
						g_rDbdcInfo.fgIsDBDCEnByP2pLis =
								TRUE;
					}

					prP2pDevFsmInfo->
					fgIsP2pListening = fgIsP2pListening;
				}
			} else {
				g_rDbdcInfo.fgIsDBDCEnByP2pLis =
								FALSE;
			}

			log_dbg(CNM, INFO, "[DBDC] En %u p2plis %u EnP2pLisTo %u",
					prAdapter->rWifiVar.fgDbDcModeEn,
					prP2pDevFsmInfo->fgIsP2pListening,
					g_rDbdcInfo.fgIsDBDCEnByP2pLis
			);
		}
#endif
		return;
	}

	/* Only need to extend in DISABLE_GUARD for connection retry.
	 * If AGConcurrent status changes in ENABLE_GUARD, the FSM
	 * will go through DISABLE_GUARD state. It could make sure
	 * the interval of successive OPChange is larger than 4 sec
	 * (DBDC_ENABLE_GUARD_TIME).
	 */
	if (timerPendingTimer(&g_rDbdcInfo.rDbdcGuardTimer) &&
		g_rDbdcInfo.eDdbcGuardTimerType ==
		ENUM_DBDC_GUARD_TIMER_SWITCH_GUARD_TIME) {

		if (g_rDbdcInfo.eDbdcFsmCurrState ==
		ENUM_DBDC_FSM_STATE_DISABLE_GUARD) {
			log_dbg(CNM, INFO,
				"[DBDC] DBDC guard time extend, state %d\n",
				g_rDbdcInfo.eDbdcFsmCurrState);
			cnmTimerStopTimer(prAdapter,
					  &g_rDbdcInfo.rDbdcGuardTimer);
			cnmTimerStartTimer(prAdapter,
					   &g_rDbdcInfo.rDbdcGuardTimer,
					   DBDC_ENABLE_GUARD_TIME);
		} else if (g_rDbdcInfo.eDbdcFsmCurrState ==
					ENUM_DBDC_FSM_STATE_ENABLE_GUARD &&
					ucForceLeaveEnGuard) {
			log_dbg(CNM, INFO, "[DBDC] Abort EnGuard Time, state %d, type %d\n",
				g_rDbdcInfo.eDbdcFsmCurrState,
				g_rDbdcInfo.eDdbcGuardTimerType);
			/* cancel Guard Time and change DBDC mode */
			cnmTimerStopTimer(prAdapter,
				&g_rDbdcInfo.rDbdcGuardTimer);
			g_rDbdcInfo.eDdbcGuardTimerType =
				ENUM_DBDC_GUARD_TIMER_NONE;
			goto dbdc_check;
		} else {
			log_dbg(CNM, INFO,
				"[DBDC] DBDC guard time, state %d\n",
				g_rDbdcInfo.eDbdcFsmCurrState);
		}

		return;
	}

	/* After COUNT_DOWN timeout in ENABLE_IDLE state, FSM will check
	 * AGConcurrent status agin.
	 */
	if (timerPendingTimer(&g_rDbdcInfo.rDbdcGuardTimer) &&
	    g_rDbdcInfo.eDdbcGuardTimerType ==
	    ENUM_DBDC_GUARD_TIMER_DISABLE_COUNT_DOWN) {
		log_dbg(CNM, INFO,
		       "[DBDC Debug] Disable Countdown Return, state %d\n",
		       g_rDbdcInfo.eDbdcFsmCurrState);
		return;
	}
dbdc_check:
	if (cnmDbdcIsConcurrent(prAdapter, BAND_NULL, 0)) {
		DBDC_FSM_EVENT_HANDLER(prAdapter,
				       DBDC_FSM_EVENT_BSS_CONNECTING_ENTER_AG);
	} else
		DBDC_FSM_EVENT_HANDLER(prAdapter,
				       DBDC_FSM_EVENT_BSS_DISCONNECT_LEAVE_AG);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief    DBDC Guard Time/Countdown Callback
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmDbdcGuardTimerCallback(struct ADAPTER
			       *prAdapter,
			       uintptr_t plParamPtr)
{
	log_dbg(CNM, INFO, "[DBDC Debug] Timer %u",
	       g_rDbdcInfo.eDdbcGuardTimerType);

	if (prAdapter->rWifiVar.eDbdcMode !=
	    ENUM_DBDC_MODE_DYNAMIC) {
		log_dbg(CNM, INFO, "[DBDC Debug] DBDC Mode %u Return",
		       prAdapter->rWifiVar.eDbdcMode);
		return;
	}

	if (g_rDbdcInfo.eDdbcGuardTimerType ==
	    ENUM_DBDC_GUARD_TIMER_SWITCH_GUARD_TIME) {

		g_rDbdcInfo.eDdbcGuardTimerType =
			ENUM_DBDC_GUARD_TIMER_NONE;
		DBDC_FSM_EVENT_HANDLER(prAdapter,
				       DBDC_FSM_EVENT_SWITCH_GUARD_TIME_TO);

	} else if (g_rDbdcInfo.eDdbcGuardTimerType ==
		   ENUM_DBDC_GUARD_TIMER_DISABLE_COUNT_DOWN) {

		g_rDbdcInfo.eDdbcGuardTimerType =
			ENUM_DBDC_GUARD_TIMER_NONE;
		DBDC_FSM_EVENT_HANDLER(prAdapter,
				       DBDC_FSM_EVENT_DISABLE_COUNT_DOWN_TO);

	} else
		log_dbg(CNM, ERROR, "[DBDC] WRONG DBDC TO TYPE %u\n",
		       g_rDbdcInfo.eDdbcGuardTimerType);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief    DBDC HW Switch done event
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmDbdcEventHwSwitchDone(struct ADAPTER
			      *prAdapter,
			      struct WIFI_EVENT *prEvent)
{
	u_int8_t fgDbdcEn;

	/* Check DBDC state by FSM */
	if (g_rDbdcInfo.eDbdcFsmCurrState ==
	    ENUM_DBDC_FSM_STATE_WAIT_HW_ENABLE) {
		fgDbdcEn = true;
		g_rDbdcInfo.fgHasSentCmd = false;
	} else if (g_rDbdcInfo.eDbdcFsmCurrState ==
		   ENUM_DBDC_FSM_STATE_WAIT_HW_DISABLE) {
		fgDbdcEn = false;
		g_rDbdcInfo.fgHasSentCmd = false;
	} else if (g_rDbdcInfo.fgHasSentCmd == true) {
		/* The "set_dbdc" test cmd may confuse original FSM.
		 * Besides, we do not config TxQuota for the testing cmd.
		 */
		log_dbg(CNM, INFO,
				"[DBDC] switch event from cmd happen in state %u\n",
				g_rDbdcInfo.eDbdcFsmCurrState);
		g_rDbdcInfo.fgHasSentCmd = FALSE;
		prAdapter->rWifiVar.fgDbDcModeEn = g_rDbdcInfo.fgCmdEn;
		return;
	} else {
		log_dbg(CNM, ERROR,
		       "[DBDC] switch event happen in state %u\n",
		       g_rDbdcInfo.eDbdcFsmCurrState);
		return;
	}

	/* Change DBDC state */
	prAdapter->rWifiVar.fgDbDcModeEn = fgDbdcEn;
	DBDC_FSM_EVENT_HANDLER(prAdapter,
			       DBDC_FSM_EVENT_DBDC_HW_SWITCH_DONE);
}

#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
u_int8_t cnmDbdcIsP2pListenDbdcEn(void)
{
	return g_rDbdcInfo.fgIsDBDCEnByP2pLis;
}
#endif
#endif /*CFG_SUPPORT_DBDC*/


enum ENUM_CNM_NETWORK_TYPE_T cnmGetBssNetworkType(
	struct BSS_INFO *prBssInfo)
{
	if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS)
		return ENUM_CNM_NETWORK_TYPE_AIS;
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
		if (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE)
			return ENUM_CNM_NETWORK_TYPE_P2P_GC;
		else if (prBssInfo->eCurrentOPMode ==
			 OP_MODE_ACCESS_POINT)
			return ENUM_CNM_NETWORK_TYPE_P2P_GO;
	}
	return ENUM_CNM_NETWORK_TYPE_OTHER;
}

u_int8_t cnmSapIsConcurrent(struct ADAPTER *prAdapter)
{
	if (prAdapter)
		return (prAdapter->u4P2pMode != RUNNING_P2P_MODE);

	return TRUE;
}

u_int8_t cnmSapIsActive(struct ADAPTER *prAdapter)
{
	return (cnmGetSapBssInfo(prAdapter) != NULL);
}

struct BSS_INFO *cnmGetSapBssInfo(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;

	if (!prAdapter)
		return NULL;

	for (i = 0; i < KAL_P2P_NUM; i++) {
		prBssInfo = prAdapter->aprSapBssInfo[i];

		if (prBssInfo)
			return prBssInfo;
	}

	return NULL;

}

struct BSS_INFO *
cnmGetOtherSapBssInfo(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prSapBssInfo)
{
	struct BSS_INFO *prBssInfo;

	uint8_t i;

	if (!prAdapter)
		return NULL;

	for (i = 0; i < prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];
		if ((prSapBssInfo != prBssInfo) &&
			IS_BSS_P2P(prBssInfo) &&
			p2pFuncIsAPMode(
			prAdapter->rWifiVar.prP2PConnSettings
			[prBssInfo->u4PrivateData]) &&
			IS_NET_PWR_STATE_ACTIVE(
			prAdapter,
			prBssInfo->ucBssIndex)) {
			DBGLOG(P2P, INFO,
				"Get other sap (role%d)\n",
				prSapBssInfo->u4PrivateData);
			return prBssInfo;
		}
	}

	return NULL;
}

uint8_t cnmSapChannelSwitchReq(struct ADAPTER *prAdapter,
	struct RF_CHANNEL_INFO *prRfChannelInfo,
	uint8_t ucRoleIdx)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_P2P_INFO *prGlueP2pInfo = NULL;
	struct MSG_P2P_SET_NEW_CHANNEL *prP2pSetNewChannelMsg =
		(struct MSG_P2P_SET_NEW_CHANNEL *) NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo =
		(struct P2P_CONNECTION_REQ_INFO *) NULL;
	uint8_t ucBssIdx = 0;

	DBGLOG(P2P, INFO,
		"role(%d) c=%d b=%d opw=%d\n",
		ucRoleIdx,
		prRfChannelInfo->ucChannelNum,
		prRfChannelInfo->eBand,
		prRfChannelInfo->ucChnlBw);

	/* Free chandef buffer */
	if (!prGlueInfo) {
		DBGLOG(P2P, WARN, "glue info is not active\n");
		goto error;
	}
	prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];
	if (!prGlueP2pInfo) {
		DBGLOG(P2P, WARN, "p2p glue info is not active\n");
		goto error;
	}
	prGlueP2pInfo->fgChannelSwitchReq = true;

	/* Fill conn info */
	prP2pRoleFsmInfo =
		P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx);
	if (!prP2pRoleFsmInfo)
		goto error;
	prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
	if (!prP2pConnReqInfo)
		goto error;

	prP2pConnReqInfo->rChannelInfo.ucChannelNum =
		prRfChannelInfo->ucChannelNum;
	prP2pConnReqInfo->rChannelInfo.eBand =
		prRfChannelInfo->eBand;
	prP2pConnReqInfo->eChnlBw =
		prRfChannelInfo->ucChnlBw;

	p2pFuncSetDfsState(DFS_STATE_INACTIVE);

	if (p2pFuncRoleToBssIdx(
		prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS) {
		DBGLOG(P2P, WARN, "Incorrect role index");
		goto error;
	}

	/* Set new channel */
	prP2pSetNewChannelMsg = (struct MSG_P2P_SET_NEW_CHANNEL *)
		cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG, sizeof(*prP2pSetNewChannelMsg));
	if (prP2pSetNewChannelMsg == NULL) {
		DBGLOG(P2P, WARN,
			"prP2pSetNewChannelMsg alloc fail\n");
		goto error;
	}

	prP2pSetNewChannelMsg->rMsgHdr.eMsgId =
		MID_MNY_P2P_SET_NEW_CHANNEL;

	memcpy(&prP2pSetNewChannelMsg->rRfChannelInfo,
		prRfChannelInfo, sizeof(struct RF_CHANNEL_INFO));

	prP2pSetNewChannelMsg->ucRoleIdx = ucRoleIdx;
	prP2pSetNewChannelMsg->ucBssIndex = ucBssIdx;
	p2pFuncSetCsaBssIndex(ucBssIdx);
	mboxSendMsg(prAdapter,
		MBOX_ID_0,
		(struct MSG_HDR *) prP2pSetNewChannelMsg,
		MSG_SEND_METHOD_BUF);

	kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);

	prGlueInfo->prP2PInfo[ucRoleIdx]->eChnlSwitchPolicy =
		p2pFunDetermineChnlSwitchPolicy(prAdapter, ucBssIdx,
			prRfChannelInfo);

	p2pFunNotifyChnlSwitch(prAdapter, ucBssIdx,
		prGlueInfo->prP2PInfo[ucRoleIdx]->eChnlSwitchPolicy,
		prRfChannelInfo);

	return 0;

error:

	return -1;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief    Search available HW WMM index.
*
* @param (none)
*
* @return
*/
/*----------------------------------------------------------------------------*/
void cnmWmmIndexDecision(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	u_int8_t ucWmmIdx;

	if (!prAdapter || !prBssInfo) {
		DBGLOG(CNM, ERROR, "Set WMM fail\n");
		return;
	}

	if (prBssInfo->fgIsWmmInited)
		return;

	ucWmmIdx = prBssInfo->ucBssIndex;

	if (prAdapter->ucHwWmmEnBit & BIT(ucWmmIdx))
		DBGLOG(CNM, WARN, "Duplicated WMM%d found\n", ucWmmIdx);

	prAdapter->ucHwWmmEnBit |= BIT(ucWmmIdx);
	prBssInfo->fgIsWmmInited = TRUE;

	if (ucWmmIdx > MAX_HW_WMM_INDEX) {
		DBGLOG(CNM, ERROR, "Invalid WMM%d found, fallback to WMM%d\n",
			ucWmmIdx,
			ucWmmIdx % HW_WMM_NUM);
		ucWmmIdx %= HW_WMM_NUM;
	}

	prBssInfo->ucWmmQueSet = ucWmmIdx;

#elif (CFG_HW_WMM_BY_BSS == 1)
	uint8_t ucWmmIndex = 0;

	if (!prAdapter || !prBssInfo || !prBssInfo->fgIsInUse) {
		DBGLOG(CNM, ERROR, "Set WMM fail\n");
		return;
	}

	if (prBssInfo->fgIsWmmInited)
		return;

#if CFG_SUPPORT_NAN
	if (prBssInfo->eNetworkType == NETWORK_TYPE_NAN) {
		ucWmmIndex = HW_WMM_NUM;
		if (prBssInfo->eBand == BAND_2G4) {
			if ((prAdapter->ucHwWmmEnBit & BIT(0))
				|| (prAdapter->ucHwWmmEnBit & BIT(2))) {
				if (!(prAdapter->ucHwWmmEnBit & BIT(1))
				&& !(prAdapter->ucHwWmmEnBit & BIT(3))) {
					g_ucNanWmmQueIdx = 3;
					ucWmmIndex = 3;
				}
			} else {
				g_ucNanWmmQueIdx = 2;
				ucWmmIndex = 2;
			}
		}
		if (prBssInfo->eBand == BAND_5G) {
			if (g_ucNanWmmQueIdx == 2)
				ucWmmIndex = 0;
			if (g_ucNanWmmQueIdx == 3)
				ucWmmIndex = 1;
		}
		if (ucWmmIndex != HW_WMM_NUM) {
			prAdapter->ucHwWmmEnBit |= BIT(ucWmmIndex);
			prBssInfo->fgIsWmmInited = TRUE;
			prBssInfo->ucWmmQueSet = ucWmmIndex;

			DBGLOG(CNM, INFO, "NAN bss%d assign ucWmmIndex: %d\n",
				prBssInfo->ucBssIndex, ucWmmIndex);
			return;
		}
	}
#endif
	for (ucWmmIndex = 0; ucWmmIndex < HW_WMM_NUM; ucWmmIndex++) {
		if (!(prAdapter->ucHwWmmEnBit & BIT(ucWmmIndex))) {
			prAdapter->ucHwWmmEnBit |= BIT(ucWmmIndex);
			prBssInfo->fgIsWmmInited = TRUE;
			prBssInfo->ucWmmQueSet = ucWmmIndex;

			DBGLOG(CNM, INFO, "Bss%d assign ucWmmIndex: %d\n",
				prBssInfo->ucBssIndex, ucWmmIndex);
			return;
		}
	}
#else
	if (!prAdapter || !prBssInfo) {
		DBGLOG(CNM, ERROR, "Set WMM fail\n");
		return;
	}

	/* Follow the same rule with cnmUpdateDbdcSetting */
	if (prBssInfo->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
		|| prBssInfo->eBand == BAND_6G
#endif
	)
		prBssInfo->ucWmmQueSet = DBDC_5G_WMM_INDEX;
	else
		prBssInfo->ucWmmQueSet = (prAdapter->rWifiVar.eDbdcMode ==
			 ENUM_DBDC_MODE_DISABLED) ?
			DBDC_5G_WMM_INDEX : DBDC_2G_WMM_INDEX;
#endif
}
/*----------------------------------------------------------------------------*/
/*!
* @brief    Free BSS HW WMM index.
*
* @param (none)
*
* @return None
*/
/*----------------------------------------------------------------------------*/
void cnmFreeWmmIndex(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	DBGLOG(CNM, INFO, "[Free] ucWmmQueSet: %d\n", prBssInfo->ucWmmQueSet);

#if (CFG_HW_WMM_BY_BSS == 1)
	prAdapter->ucHwWmmEnBit &= (~BIT(prBssInfo->ucWmmQueSet));
#endif
	prBssInfo->ucWmmQueSet = DEFAULT_HW_WMM_INDEX;
	prBssInfo->fgIsWmmInited = FALSE;
}

enum ENUM_CNM_OPMODE_REQ_T
cnmOpModeMapEvtReason(
	enum ENUM_EVENT_OPMODE_CHANGE_REASON eEvt
)
{
	enum ENUM_CNM_OPMODE_REQ_T eReqIdx;

	switch (eEvt) {
	case EVENT_OPMODE_CHANGE_REASON_DBDC:
		eReqIdx = CNM_OPMODE_REQ_DBDC;
		break;
	case EVENT_OPMODE_CHANGE_REASON_COANT:
		eReqIdx = CNM_OPMODE_REQ_COANT;
		break;
	case EVENT_OPMODE_CHANGE_REASON_DBDC_SCAN:
		eReqIdx = CNM_OPMODE_REQ_DBDC_SCAN;
		break;
	case EVENT_OPMODE_CHANGE_REASON_SMARTGEAR:
		eReqIdx = CNM_OPMODE_REQ_SMARTGEAR;
		break;
	case EVENT_OPMODE_CHANGE_REASON_SMARTGEAR_1T2R:
		eReqIdx = CNM_OPMODE_REQ_SMARTGEAR_1T2R;
		break;
	case EVENT_OPMODE_CHANGE_REASON_COEX:
		eReqIdx = CNM_OPMODE_REQ_COEX;
		break;
	case EVENT_OPMODE_CHANGE_REASON_ANT_CTRL:
		eReqIdx = CNM_OPMODE_REQ_ANT_CTRL;
		break;
	case EVENT_OPMODE_CHANGE_REASON_ANT_CTRL_1T2R:
		eReqIdx = CNM_OPMODE_REQ_ANT_CTRL_1T2R;
		break;
	case EVENT_OPMODE_CHANGE_REASON_USER_CONFIG:
		eReqIdx = CNM_OPMODE_REQ_USER_CONFIG;
		break;
	case EVENT_OPMODE_CHANGE_REASON_RDD:
		eReqIdx = CNM_OPMODE_REQ_RDD_OPCHNG;
		break;
	default:
		eReqIdx = CNM_OPMODE_REQ_NUM;
		break;
	}
	return eReqIdx;
}

void cnmOpModeDump(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex
)
{
	struct BSS_INFO *prBssInfo;

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	DBGLOG(CNM, INFO,
		"BSS[%d] DumpOpMode Tx(Cur:%x,Run:%x), Rx(Cur:%x,Run:%x)\n",
		ucBssIndex,
		prBssInfo->ucOpTxNss,
		prBssInfo->fgIsOpChangeTxNss ?
			prBssInfo->ucOpChangeTxNss : 0xFF,
		prBssInfo->ucOpRxNss,
		prBssInfo->fgIsOpChangeRxNss ?
			prBssInfo->ucOpChangeRxNss : 0xFF);
}

void cnmOpModeCallbackDispatcher(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	bool fgSuccess)
{
	struct CNM_OPMODE_BSS_CONTROL_T *prBssOpCtrl;
	struct CNM_OPMODE_BSS_REQ *prReq;
	enum ENUM_CNM_OPMODE_REQ_T eReqIdx;

	ASSERT(prAdapter);
	if (ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(CNM, WARN,
			"CbOpMode, invalid,B[%d]\n",
			ucBssIndex);
		return;
	}

	/* Step 1. Run callback function */
	prBssOpCtrl = &g_arBssOpControl[ucBssIndex];
	if (prBssOpCtrl->rRunning.eReqIdx < 0
		|| prBssOpCtrl->rRunning.eRunReq < 0
		|| prBssOpCtrl->rRunning.eReqIdx > CNM_OPMODE_REQ_MAX_CAP
		|| prBssOpCtrl->rRunning.eRunReq > CNM_OPMODE_REQ_MAX_CAP) {
		DBGLOG(CNM, WARN,
			"CbOpMode, invalid,ReqIdx[%d] eRunReq [%d]\n",
			prBssOpCtrl->rRunning.eReqIdx,
			prBssOpCtrl->rRunning.eRunReq);
		return;
	}
	if (!prBssOpCtrl->rRunning.fgIsRunning) {
#if CFG_SUPPORT_DBDC
		/* GO/AP run cb immediately. */
		DBGLOG(CNM, INFO,
			"CbOpMode, BSS[%d] none running, OpModeState[%d]\n",
			ucBssIndex,
			g_rDbdcInfo.eBssOpModeState[ucBssIndex]);
		/* We have to callback op mode change done.
		 * Otherwise, DBDC state machine won't continue.
		 */
		if (g_rDbdcInfo.eBssOpModeState[ucBssIndex] ==
			ENUM_OPMODE_STATE_WAIT) {
			cnmDbdcOpModeChangeDoneCallback(
				prAdapter, ucBssIndex, fgSuccess);
		}
#endif
	} else {
		switch (prBssOpCtrl->rRunning.eReqIdx) {
#if CFG_SUPPORT_DBDC
		case CNM_OPMODE_REQ_DBDC:
			cnmDbdcOpModeChangeDoneCallback(
				prAdapter, ucBssIndex, fgSuccess);
			break;
#endif
		default:
			break;
		}
		DBGLOG(CNM, INFO,
			"CbOpMode,%s,Run,%s,T:%u,R:%u,%s\n",
			apucCnmOpModeReq[prBssOpCtrl->rRunning.eReqIdx],
			apucCnmOpModeReq[prBssOpCtrl->rRunning.eRunReq],
			prBssOpCtrl->rRunning.ucOpTxNss,
			prBssOpCtrl->rRunning.ucOpRxNss,
			fgSuccess ? "OK" : "FAIL");
	}
	prBssOpCtrl->rRunning.fgIsRunning = false;

	/* Step 2. Check pending request */
	for (eReqIdx = CNM_OPMODE_REQ_START;
		eReqIdx < CNM_OPMODE_REQ_NUM;
		eReqIdx++) {
		prReq = &(prBssOpCtrl->arReqPool[eReqIdx]);
		if (prReq->fgNewRequest)
			break;
	}

	if (eReqIdx != CNM_OPMODE_REQ_NUM) {
		DBGLOG(CNM, INFO,
			"CbOpMode,ReTrigger:%s,En,%u,Tx:%u,Rx:%u\n",
			apucCnmOpModeReq[eReqIdx],
			prReq->fgEnable,
			prReq->ucOpTxNss,
			prReq->ucOpRxNss);
		cnmOpModeSetTRxNss(
			prAdapter,
			ucBssIndex,
			eReqIdx,
			prReq->fgEnable,
			prReq->ucOpRxNss,
			prReq->ucOpTxNss);
	}
}

static enum ENUM_CNM_OPMODE_REQ_T
cnmOpModeReqDispatcher(
	struct CNM_OPMODE_BSS_CONTROL_T *prBssOpCtrl
)
{
	struct CNM_OPMODE_BSS_REQ *prReq;
	enum ENUM_CNM_OPMODE_REQ_T eReqIdx;
	enum ENUM_CNM_OPMODE_REQ_T eReqFinal = CNM_OPMODE_REQ_MAX_CAP;

	if (prBssOpCtrl->rRunning.fgIsRunning) {
		if (prBssOpCtrl->rRunning.eReqIdx >= 0 &&
				prBssOpCtrl->rRunning.eReqIdx <=
				CNM_OPMODE_REQ_MAX_CAP &&
				prBssOpCtrl->rRunning.eRunReq >= 0 &&
				prBssOpCtrl->rRunning.eRunReq <=
				CNM_OPMODE_REQ_MAX_CAP)
			DBGLOG(CNM, INFO,
			"OpMode %s (Tx:%d,Rx:%d) is running %s, defer new request\n",
			apucCnmOpModeReq[prBssOpCtrl->rRunning.eReqIdx],
			prBssOpCtrl->rRunning.ucOpTxNss,
			prBssOpCtrl->rRunning.ucOpRxNss,
			apucCnmOpModeReq[prBssOpCtrl->rRunning.eRunReq]
			);
		return CNM_OPMODE_REQ_NUM;
	}

	for (eReqIdx = CNM_OPMODE_REQ_START;
		  eReqIdx < CNM_OPMODE_REQ_NUM; eReqIdx++) {
		prReq = &(prBssOpCtrl->arReqPool[eReqIdx]);
		prReq->fgNewRequest = false;
		if (prReq->fgEnable && eReqIdx < eReqFinal)
			eReqFinal = eReqIdx;
	}

	return eReqFinal;
}

uint8_t cnmOpModeGetMaxBw(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	uint8_t ucOpMaxBw = MAX_BW_UNKNOWN;
	uint8_t ucS1 = 0;

	if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
#if CFG_SUPPORT_P2P_ECSA
		uint8_t ucRoleIndex = prBssInfo->u4PrivateData;
#endif

#if CFG_SUPPORT_DBDC
		ucOpMaxBw = cnmGetDbdcBwCapability(prAdapter,
				prBssInfo->ucBssIndex);
#else
		ucOpMaxBw = cnmGetBssMaxBw(prAdapter,
				prBssInfo->ucBssIndex);
#endif

#if CFG_SUPPORT_P2P_ECSA
		/* ECSA overwrite */
		if (prAdapter->rWifiVar
			.prP2pSpecificBssInfo[ucRoleIndex]
			->fgEcsa) {
			uint8_t bw = prAdapter->rWifiVar
				.prP2pSpecificBssInfo[ucRoleIndex]
				->ucEcsaBw;

			if (bw < ucOpMaxBw) {
				ucOpMaxBw = bw;
				DBGLOG(P2P, INFO,
					"ECSA overwrite bw %d\n", bw);
			}
		}
#endif

		if (ucOpMaxBw >= MAX_BW_80MHZ) {
			/* Verify if there is valid S1 */
			ucS1 = nicGetS1(prBssInfo->eBand,
				prBssInfo->ucPrimaryChannel,
				rlmGetVhtOpBwByBssOpBw(ucOpMaxBw));

			/* Try if there is valid S1 for BW160 if we failed to
			 * get S1 for BW320.
			 */
			if (ucS1 == 0 &&
			   (ucOpMaxBw == MAX_BW_320_1MHZ ||
			    ucOpMaxBw == MAX_BW_320_2MHZ)) {
				ucS1 = nicGetS1(prBssInfo->eBand,
					prBssInfo->ucPrimaryChannel,
					rlmGetVhtOpBwByBssOpBw(MAX_BW_160MHZ));

				if (ucS1) /* Fallback to BW80 */
					ucOpMaxBw = MAX_BW_160MHZ;
			}

			/* Try if there is valid S1 for BW80 if we failed to
			 * get S1 for BW160.
			 */
			if (ucS1 == 0 && ucOpMaxBw == MAX_BW_160MHZ) {
				ucS1 = nicGetS1(prBssInfo->eBand,
					prBssInfo->ucPrimaryChannel,
					rlmGetVhtOpBwByBssOpBw(MAX_BW_80MHZ));

				if (ucS1) /* Fallback to BW80 */
					ucOpMaxBw = MAX_BW_80MHZ;
			}

			if (ucS1 == 0) { /* Invalid S1 */
				DBGLOG(CNM, INFO,
					"fallback to BW20, BssIdx[%d], CH[%d], MaxBw[%d]\n",
					prBssInfo->ucBssIndex,
					prBssInfo->ucPrimaryChannel,
					ucOpMaxBw);

				ucOpMaxBw = MAX_BW_20MHZ;
			}
		}
	} else { /* STA, GC */
		ucOpMaxBw = rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo);
	}

	return ucOpMaxBw;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Set the operating TRx Nss.
 *        If failed to change OpRxNss, the OpTxNss will not change.
 *        If the BSS is not alive, just update to control table.
 *
 * @param prAdapter
 * @param ucBssIndex
 * @param eNewReq
 * @param fgEnable
 * @param ucOpRxNss
 * @param ucOpTxNss
 *
 * @return ENUM_CNM_OPMODE_REQ_STATUS
 */
/*----------------------------------------------------------------------------*/
enum ENUM_CNM_OPMODE_REQ_STATUS
cnmOpModeSetTRxNss(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	enum ENUM_CNM_OPMODE_REQ_T eNewReq,
	bool fgEnable,
	uint8_t ucOpRxNss,
	uint8_t ucOpTxNss
)
{
	struct BSS_INFO *prBssInfo;
	struct CNM_OPMODE_BSS_CONTROL_T *prBssOpCtrl;
	struct CNM_OPMODE_BSS_REQ *prReq;
	enum ENUM_OP_CHANGE_STATUS_T eRlmStatus;
	enum ENUM_CNM_OPMODE_REQ_STATUS eStatus
		= CNM_OPMODE_REQ_STATUS_SUCCESS;
	uint8_t ucOpRxNssFinal, ucOpTxNssFinal, ucOpBwFinal;
	enum ENUM_CNM_OPMODE_REQ_T eRunReq;
	uint8_t ucSendAct = TRUE;
	uint8_t ucRoleIndex = 0;

	ASSERT(prAdapter);
	if (ucBssIndex > prAdapter->ucHwBssIdNum ||
		ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(CNM, WARN, "SetOpMode invalid BSS[%d]\n", ucBssIndex);
		return CNM_OPMODE_REQ_STATUS_INVALID_PARAM;
	}

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	prBssOpCtrl = &g_arBssOpControl[ucBssIndex];
	prReq = &(prBssOpCtrl->arReqPool[eNewReq]);

	/* Step 1 Update req pool */
	prReq->fgEnable = fgEnable;
	prReq->fgNewRequest = true;
	prReq->ucOpRxNss = ucOpRxNss;
	prReq->ucOpTxNss = ucOpTxNss;

	/* Step 2 Select the highest priority req */
	eRunReq = cnmOpModeReqDispatcher(prBssOpCtrl);
	if (eRunReq == CNM_OPMODE_REQ_NUM) {
		return CNM_OPMODE_REQ_STATUS_DEFER;
	} else if (eRunReq == CNM_OPMODE_REQ_MAX_CAP) {
		ucOpRxNssFinal = ucOpTxNssFinal =
		wlanGetSupportNss(prAdapter, ucBssIndex);
	} else  {
		prReq = &prBssOpCtrl->arReqPool[eRunReq];
		ucOpRxNssFinal = prReq->ucOpRxNss;
		ucOpTxNssFinal = prReq->ucOpTxNss;
	}

	if (IS_BSS_ALIVE(prAdapter, prBssInfo)) {
		/* Step 3. Special rule for BW change (DBDC)
		 * We only bound OpBw @ BW80 for DBDC.
		 * This function colud not restore to current peer's
		 * OpBw. It's fine because below reasons(2018/08):
		 *   1) No DBDC project supports BW160 or NW80+80.
		 *   2) No feature wants to change OpBw.
		 *
		 * If you want to change OpBw in the future, please
		 * make sure you can restore to current peer's OpBw.
		 */
		ucOpBwFinal = cnmOpModeGetMaxBw(prAdapter, prBssInfo);
#if (CFG_SUPPORT_DBDC_DOWNGRADE_BW == 1)
		if ((eRunReq == CNM_OPMODE_REQ_DBDC ||
			eRunReq == CNM_OPMODE_REQ_DBDC_SCAN) &&
			ucOpBwFinal > MAX_BW_80MHZ) {
			DBGLOG(CNM, INFO,
				"SetOpMode Bss[%d] %s override BW %d to MAX_BW_80MHZ\n",
				ucBssIndex,
				apucCnmOpModeReq[eRunReq],
				ucOpBwFinal);
			ucOpBwFinal = MAX_BW_80MHZ;
		}
#endif
		if (eRunReq == CNM_OPMODE_REQ_RDD_OPCHNG) {
			ucRoleIndex = prBssInfo->u4PrivateData;
			ucOpBwFinal = prAdapter->rWifiVar
			.prP2pSpecificBssInfo[ucRoleIndex]
			->ucRddBw;
		}
		/* When DBDC is off, we should rollback STA's bandwidth
		 * as peer's bandwidth capability.
		 */
		if (eNewReq == CNM_OPMODE_REQ_DBDC && !fgEnable) {
			if (prBssInfo->eCurrentOPMode ==
				OP_MODE_INFRASTRUCTURE) {
				ucOpBwFinal =
					rlmGetBssOpBwByOwnAndPeerCapability(
						prAdapter, prBssInfo);
				DBGLOG(CNM, INFO,
					"SetOpMode Bss[%d] %s %s override BW to %d\n",
					ucBssIndex,
					apucCnmOpModeReq[eNewReq],
					fgEnable ? "En" : "Dis",
					ucOpBwFinal);
			}
		}

		if (eNewReq == CNM_OPMODE_REQ_RDD_OPCHNG &&
			IS_BSS_APGO(prBssInfo))
			ucOpBwFinal = prAdapter->rWifiVar
				.prP2pSpecificBssInfo[prBssInfo->u4PrivateData]
				->ucRddBw;

		/* Step 4. Execute OpMode change function for alive BSS */
		if (eNewReq == CNM_OPMODE_REQ_SMARTGEAR_1T2R ||
			eNewReq == CNM_OPMODE_REQ_ANT_CTRL_1T2R)
			ucSendAct = FALSE;

		eRlmStatus = rlmChangeOperationMode(prAdapter,
					ucBssIndex,
					ucOpBwFinal,
					ucOpRxNssFinal,
					ucOpTxNssFinal,
					ucSendAct,
					cnmOpModeCallbackDispatcher
		);

		switch (eRlmStatus) {
		case OP_CHANGE_STATUS_VALID_NO_CHANGE:
		case OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_DONE:
			eStatus = CNM_OPMODE_REQ_STATUS_SUCCESS;
			break;
		case OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_WAIT:
			eStatus = CNM_OPMODE_REQ_STATUS_RUNNING;
			prBssOpCtrl->rRunning.fgIsRunning = true;
			prBssOpCtrl->rRunning.eReqIdx = eNewReq;
			prBssOpCtrl->rRunning.eRunReq = eRunReq;
			prBssOpCtrl->rRunning.ucOpTxNss = ucOpTxNssFinal;
			prBssOpCtrl->rRunning.ucOpRxNss = ucOpRxNssFinal;
#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
			if (prAdapter->fgANTCtrl)
				prAdapter->ucANTCtrlPendingCount++;
#endif
			break;
		case OP_CHANGE_STATUS_INVALID:
		default:
			eStatus = CNM_OPMODE_REQ_STATUS_INVALID_PARAM;
#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
			/* cannot complete ANT control */
			if (prAdapter->fgANTCtrl)
				prAdapter->ucANTCtrlPendingCount++;
#endif
			break;
		}
	}

	/* Step 5. Dump result */
	if (eRunReq >= 0 && eRunReq <= CNM_OPMODE_REQ_MAX_CAP
		&& eNewReq >= 0 && eNewReq <= CNM_OPMODE_REQ_MAX_CAP)
		DBGLOG(CNM, INFO,
		"SetOpMode Bss[%d] alive[%d] NewReq:%s %s RunReq:%s,%s\n",
		ucBssIndex, IS_BSS_ALIVE(prAdapter, prBssInfo),
		apucCnmOpModeReq[eNewReq],
		fgEnable ? "En" : "Dis",
		apucCnmOpModeReq[eRunReq],
		apucCnmOpModeReqStatus[eStatus]);
	cnmOpModeDump(prAdapter, ucBssIndex);

	return eStatus;
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief Get the operation TRx Nss.
 *        If DBDC is goning to enable or already enabled, return 1.
 *        Else return MaxCapability.
 *
 * @param prAdapter
 * @param ucBssIndex
 * @param pucOpRxNss
 * @param pucOpTxNss
 *
 * @return ucOpTRxNss
 */
/*----------------------------------------------------------------------------*/
void cnmOpModeGetTRxNss(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t *pucOpRxNss,
	uint8_t *pucOpTxNss)
{
	struct CNM_OPMODE_BSS_CONTROL_T *prBssOpCtrl;
	struct CNM_OPMODE_BSS_REQ *prReq;
	enum ENUM_CNM_OPMODE_REQ_T eReqIdx;
	enum ENUM_CNM_OPMODE_REQ_T eCurrMaxIdx = CNM_OPMODE_REQ_MAX_CAP;
	uint8_t ucOpRxNss, ucOpTxNss;

	if (pucOpRxNss == NULL || pucOpTxNss == NULL ||
		ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(CNM, WARN,
			"GetOpMode invalid param B[%d]\n",
			ucBssIndex);
		return;
	}

	ucOpRxNss = ucOpTxNss = wlanGetSupportNss(prAdapter, ucBssIndex);
	prBssOpCtrl = &g_arBssOpControl[ucBssIndex];

	*pucOpTxNss = ucOpTxNss;
	*pucOpRxNss = ucOpRxNss;

	if (prBssOpCtrl->rRunning.fgIsRunning) {
		eCurrMaxIdx = prBssOpCtrl->rRunning.eRunReq;
		*pucOpTxNss = prBssOpCtrl->rRunning.ucOpTxNss;
		*pucOpRxNss = prBssOpCtrl->rRunning.ucOpRxNss;
		if (eCurrMaxIdx >= 0 && eCurrMaxIdx <= CNM_OPMODE_REQ_MAX_CAP
			&& prBssOpCtrl->rRunning.eReqIdx >= 0
			&& prBssOpCtrl->rRunning.eReqIdx
				<= CNM_OPMODE_REQ_MAX_CAP)
			DBGLOG(CNM, INFO,
			"GetOpMode,use running %s from %s\n",
			apucCnmOpModeReq[eCurrMaxIdx],
			apucCnmOpModeReq[prBssOpCtrl->rRunning.eReqIdx]);
	} else {
		for (eReqIdx = CNM_OPMODE_REQ_START;
			eReqIdx < CNM_OPMODE_REQ_NUM;
			eReqIdx++) {
			prReq = &(prBssOpCtrl->arReqPool[eReqIdx]);
			if (prReq->fgEnable && !prReq->fgNewRequest) {
				eCurrMaxIdx = eReqIdx;
				*pucOpTxNss = (ucOpTxNss > prReq->ucOpTxNss) ?
					prReq->ucOpTxNss : ucOpTxNss;
				*pucOpRxNss = (ucOpRxNss > prReq->ucOpRxNss) ?
					prReq->ucOpRxNss : ucOpRxNss;
				break;
			}
		}
	}
	if (eCurrMaxIdx >= 0 && eCurrMaxIdx <= CNM_OPMODE_REQ_MAX_CAP)
		DBGLOG(CNM, INFO,
			"GetOpMode BSS[%u](%s) T:%d R:%u\n",
			ucBssIndex, apucCnmOpModeReq[eCurrMaxIdx],
			*pucOpTxNss, *pucOpRxNss);
}

#if CFG_SUPPORT_SMART_GEAR
/*----------------------------------------------------------------------------*/
/*!
 * @brief Handle Smart Gear Status Change event from FW.
 *
 * @param prAdapter
 * @param prEvent
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
void cnmEventSGStatus(
	struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
#if CFG_SUPPORT_DATA_STALL
	struct EVENT_SMART_GEAT_STATE *prSGState;
	enum ENUM_VENDOR_DRIVER_EVENT eEvent;

	ASSERT(prAdapter);
	prSGState = (struct EVENT_SMART_GEAT_STATE *) (prEvent->aucBuffer);

	if (prSGState->fgIsEnable == 0x01) {
		if (prSGState->u4StateIdx == 0x00)
			eEvent = EVENT_SG_1T1R;
		else
			eEvent = EVENT_SG_2T2R;
	} else if (prSGState->fgIsEnable == 0x00) {
		eEvent = EVENT_SG_DISABLE;
	} else {
		;/* Not correction value, juste reture;*/
		return;
	}

	DBGLOG(CNM,  INFO,
			"[SG]cnmEventSGStatus,%u,%u,%u\n",
			prSGState->fgIsEnable, prSGState->u4StateIdx, eEvent);
	KAL_REPORT_ERROR_EVENT(prAdapter,
			eEvent, (uint16_t)sizeof(u_int8_t),
			0,
			TRUE);
#endif /* CFG_SUPPORT_DATA_STALL */
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Event handler for EVENT_ID_OPMODE_CHANGE
 *
 * @param prAdapter
 * @param prEvent
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
void cnmOpmodeEventHandler(
	struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_OPMODE_CHANGE *prEvtOpMode;
	enum ENUM_CNM_OPMODE_REQ_T eReqIdx;
	uint8_t ucBssIndex;

	ASSERT(prAdapter);
	prEvtOpMode = (struct EVENT_OPMODE_CHANGE *)
		(prEvent->aucBuffer);

#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
	/* only notify FW for ANT control */
	if ((prEvtOpMode->ucEnable) &&
	    (prEvtOpMode->ucReason == EVENT_OPMODE_CHANGE_REASON_ANT_CTRL)) {
		prAdapter->fgANTCtrl = true;
		prAdapter->ucANTCtrlReason = prEvtOpMode->ucReason;
		prAdapter->ucANTCtrlPendingCount = 0;
	}
#endif

	eReqIdx = cnmOpModeMapEvtReason(
		(enum ENUM_EVENT_OPMODE_CHANGE_REASON)
		prEvtOpMode->ucReason);

	if (eReqIdx >= CNM_OPMODE_REQ_NUM) {
		DBGLOG(CNM, WARN,
			"EvtOpMode,WrongReaosn,%u,Evt,%u,igonre\n",
			eReqIdx, prEvtOpMode->ucReason);
		return;
	}

	DBGLOG(CNM, INFO,
		"EvtOpMode, Req:%s BssBitmap:0x%x, En:%u T:%u R:%u\n",
		apucCnmOpModeReq[eReqIdx],
		prEvtOpMode->ucBssBitmap,
		prEvtOpMode->ucEnable,
		prEvtOpMode->ucOpTxNss,
		prEvtOpMode->ucOpRxNss);

	for (ucBssIndex = 0;
		 ucBssIndex < prAdapter->ucHwBssIdNum;
		 ucBssIndex++) {
		if (prEvtOpMode->ucBssBitmap & BIT(ucBssIndex)) {
			cnmOpModeSetTRxNss(
				prAdapter,
				ucBssIndex,
				eReqIdx,
				prEvtOpMode->ucEnable,
				prEvtOpMode->ucOpRxNss,
				prEvtOpMode->ucOpTxNss
			);
		}
	}

#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
	/* notify FW if no active BSS or no pending action frame */
	if (prAdapter->fgANTCtrl) {
		DBGLOG(CNM, INFO,
			"ANT control = Enable: %d, reason: %d, pending count = %d\n",
			prAdapter->fgANTCtrl, prAdapter->ucANTCtrlReason,
			prAdapter->ucANTCtrlPendingCount);
		if (prAdapter->ucANTCtrlPendingCount == 0)
			rlmSyncAntCtrl(prAdapter,
				prEvtOpMode->ucOpTxNss, prEvtOpMode->ucOpRxNss);
	}
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Event handler for EVENT_ID_OPMODE_CHANGE
 *
 * @param prAdapter
 * @param prEvent
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
void cnmRddOpmodeEventHandler(
	struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_RDD_OPMODE_CHANGE *prRddEvtOpMode;

	struct WIFI_EVENT *pEventOpMode = NULL;
	struct EVENT_OPMODE_CHANGE *prEventBody;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo =
		(struct P2P_CONNECTION_REQ_INFO *) NULL;
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex;
	uint8_t ucRoleIndex;

	if (!prAdapter) {
		DBGLOG(P2P, ERROR,
					"[RDD]NULL prAdapter\n");
		return;
	}

	prRddEvtOpMode = (struct EVENT_RDD_OPMODE_CHANGE *)
		(prEvent->aucBuffer);

	for (ucBssIndex = 0;
		 ucBssIndex < prAdapter->ucHwBssIdNum;
		 ucBssIndex++) {
		if (prRddEvtOpMode->ucBssBitmap & BIT(ucBssIndex))
			break;
	}

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	if (prBssInfo) {
		prP2pRoleFsmInfo =
			P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
			prBssInfo->u4PrivateData);
		ucRoleIndex = prBssInfo->u4PrivateData;
	}
	if (prP2pRoleFsmInfo)
		prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);

	if (prP2pConnReqInfo) {
		DBGLOG(P2P, TRACE,
			"[RDD]Current CH=%d,Band=%d,BW=%d\n",
			prP2pConnReqInfo->rChannelInfo.ucChannelNum,
			prP2pConnReqInfo->rChannelInfo.eBand,
			prP2pConnReqInfo->rChannelInfo.ucChnlBw);
	} else
		return;

	DBGLOG(P2P, TRACE,
		"[RDD]event CH=%d,TxNss=%d,BW=%d\n",
		prRddEvtOpMode->ucPriChannel,
		prRddEvtOpMode->ucOpTxNss,
		prRddEvtOpMode->ucChBw);

	if (prRddEvtOpMode->ucChBw !=
		prP2pConnReqInfo->rChannelInfo.ucChnlBw ||
		prRddEvtOpMode->ucOpRxNss !=
		prBssInfo->ucOpRxNss ||
		prRddEvtOpMode->ucOpTxNss !=
		prBssInfo->ucOpTxNss ||
		prRddEvtOpMode->ucAction ==
		RDD_OP_CHG_STOP_CAC) {
		pEventOpMode =
			(struct WIFI_EVENT *)
			kalMemAlloc(sizeof(struct WIFI_EVENT)+
			sizeof(struct EVENT_OPMODE_CHANGE), VIR_MEM_TYPE);
		if (!pEventOpMode) {
			DBGLOG(P2P, TRACE,
				"pEventOpMode is null\n");
			return;
		}
		prEventBody =
			(struct EVENT_OPMODE_CHANGE *)
			&(pEventOpMode->aucBuffer[0]);
		prEventBody->ucEvtVer = 0;
		prEventBody->u2EvtLen = prRddEvtOpMode->u2EvtLen;
		prEventBody->ucBssBitmap = prRddEvtOpMode->ucBssBitmap;
		prEventBody->ucEnable = prRddEvtOpMode->ucEnable;
		prEventBody->ucOpTxNss = prRddEvtOpMode->ucOpTxNss;
		prEventBody->ucOpRxNss = prRddEvtOpMode->ucOpRxNss;
		prEventBody->ucReason = EVENT_OPMODE_CHANGE_REASON_RDD;
		prAdapter->rWifiVar.prP2pSpecificBssInfo[ucRoleIndex]
			->ucRddBw = prRddEvtOpMode->ucChBw;
	}

	if (p2pFuncGetDfsState() == DFS_STATE_CHECKING &&
		prRddEvtOpMode->ucAction == RDD_OP_CHG_STOP_CAC) {
		struct MSG_P2P_RADAR_DETECT *prP2pRddDetMsg;

		prP2pRddDetMsg = (struct MSG_P2P_RADAR_DETECT *)
			cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, sizeof(*prP2pRddDetMsg));
		if (!prP2pRddDetMsg) {
			DBGLOG(P2P, TRACE,
				"prP2pRddDetMsg is null\n");
			if (pEventOpMode) {
				kalMemFree(pEventOpMode,
					VIR_MEM_TYPE, sizeof(struct WIFI_EVENT)+
					sizeof(struct EVENT_OPMODE_CHANGE));
			}
			return;
		}

		prP2pRddDetMsg->rMsgHdr.eMsgId =
		MID_CNM_P2P_RADAR_DETECT;
		prP2pRddDetMsg->ucBssIndex = ucBssIndex;
		prAdapter->rWifiVar.prP2pSpecificBssInfo[ucRoleIndex]
			->fgIsRddOpchng = TRUE;
		prAdapter->rWifiVar.prP2pSpecificBssInfo[ucRoleIndex]
			->ucRddCh = prRddEvtOpMode->ucPriChannel;

		prAdapter->rWifiVar
			.prP2pSpecificBssInfo[ucRoleIndex]
			->prRddPostOpchng = pEventOpMode;

		p2pRoleFsmRunEventRadarDet(prAdapter,
		(struct MSG_HDR *) prP2pRddDetMsg);
	} else if (p2pFuncGetDfsState() != DFS_STATE_CHECKING &&
		prAdapter->rWifiVar.prP2pSpecificBssInfo[ucRoleIndex] &&
		prRddEvtOpMode->ucPriChannel !=
		prP2pConnReqInfo->rChannelInfo.ucChannelNum) {
		struct RF_CHANNEL_INFO rfChannelInfo;

		prAdapter->rWifiVar.prP2pSpecificBssInfo[ucRoleIndex]
			->fgIsRddOpchng = TRUE;
		rfChannelInfo = prP2pConnReqInfo->rChannelInfo;
		rfChannelInfo.ucChannelNum = prRddEvtOpMode->ucPriChannel;
		if (prRddEvtOpMode->ucPriChannel < 14)
			rfChannelInfo.eBand = BAND_2G4;
		else
			rfChannelInfo.eBand = BAND_5G;
		rfChannelInfo.ucChnlBw = prRddEvtOpMode->ucChBw;
		cnmSapChannelSwitchReq(prAdapter,
		&rfChannelInfo,
		ucRoleIndex);
		kalP2PTxCarrierOn(prAdapter->prGlueInfo,
			prBssInfo);
		prAdapter->rWifiVar.prP2pSpecificBssInfo[ucRoleIndex]
			->prRddPostOpchng = pEventOpMode;
	} else if (pEventOpMode) {
		if (p2pFuncGetDfsState() != DFS_STATE_CHECKING)
			cnmOpmodeEventHandler(prAdapter, pEventOpMode);
		kalMemFree(pEventOpMode,
			VIR_MEM_TYPE, sizeof(struct WIFI_EVENT)+
			sizeof(struct EVENT_OPMODE_CHANGE));
	}
}

enum ENUM_CNM_WMM_QUOTA_REQ_T
cnmWmmQuotaReqDispatcher(
	struct CNM_WMM_QUOTA_CONTROL_T *prWmmQuotaCtrl
)
{
	struct CNM_WMM_QUOTA_REQ *prReq;
	enum ENUM_CNM_WMM_QUOTA_REQ_T eReqIdx;
	enum ENUM_CNM_WMM_QUOTA_REQ_T eReqFinal = CNM_WMM_REQ_DEFAULT;

	if (prWmmQuotaCtrl->rRunning.fgIsRunning) {
		if (prWmmQuotaCtrl->rRunning.eReqIdx >= 0 &&
			prWmmQuotaCtrl->rRunning.eReqIdx
				<= CNM_WMM_REQ_DEFAULT &&
			prWmmQuotaCtrl->rRunning.eRunReq >= 0 &&
			prWmmQuotaCtrl->rRunning.eRunReq
				<= CNM_WMM_REQ_DEFAULT)
			DBGLOG(CNM, WARN,
			"WmmQuota,PreReq,%s,RunningReq,%s\n",
			apucCnmWmmQuotaReq[prWmmQuotaCtrl->rRunning.eReqIdx],
			apucCnmWmmQuotaReq[prWmmQuotaCtrl->rRunning.eRunReq]);
	}

	for (eReqIdx = CNM_WMM_REQ_DBDC;
		  eReqIdx < CNM_WMM_REQ_NUM; eReqIdx++) {
		prReq = &(prWmmQuotaCtrl->arReqPool[eReqIdx]);
		if (prReq->fgEnable && eReqIdx < eReqFinal)
			eReqFinal = eReqIdx;
	}
	return eReqFinal;
}

void
cnmWmmQuotaCallback(
	struct ADAPTER *prAdapter,
	uintptr_t plParamPtr
)
{
	struct CNM_WMM_QUOTA_CONTROL_T *prWmmQuotaCtrl;
	bool fgRun;
	uint8_t ucWmmIndex;

	KAL_SPIN_LOCK_DECLARATION();

	ucWmmIndex = (uint8_t)plParamPtr;
	prWmmQuotaCtrl = &(g_arWmmQuotaControl[ucWmmIndex]);

	if (!prWmmQuotaCtrl->rRunning.fgIsRunning) {
		DBGLOG(CNM, WARN,
			"WmmQuotaCb,%d,None runnig\n",
			ucWmmIndex);
		return;
	}

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_UPDATE_WMM_QUOTA);
	fgRun = prAdapter->rWmmQuotaReqCS[ucWmmIndex].fgRun;
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_UPDATE_WMM_QUOTA);

	if (fgRun) {
		if (prWmmQuotaCtrl->rRunning.eReqIdx >= 0 &&
			prWmmQuotaCtrl->rRunning.eReqIdx <=
			CNM_WMM_REQ_DEFAULT &&
			prWmmQuotaCtrl->rRunning.eRunReq >= 0 &&
			prWmmQuotaCtrl->rRunning.eRunReq <=
			CNM_WMM_REQ_DEFAULT)
			DBGLOG(CNM, TRACE,
			"WmmQuotaCb,%d,Req,%s,Run,%s,Quota,%u,WakeUpHIF\n",
			ucWmmIndex,
			apucCnmWmmQuotaReq[prWmmQuotaCtrl->rRunning.eReqIdx],
			apucCnmWmmQuotaReq[prWmmQuotaCtrl->rRunning.eRunReq],
			prWmmQuotaCtrl->rRunning.u4ReqQuota
			);
		kalSetWmmUpdateEvent(prAdapter->prGlueInfo);
		if (!timerPendingTimer(&(prWmmQuotaCtrl->rTimer))) {
			cnmTimerStartTimer(
				prAdapter, &(prWmmQuotaCtrl->rTimer),
				CNM_WMM_QUOTA_RETRIGGER_TIME_MS);
		}
	} else {
		prWmmQuotaCtrl->rRunning.fgIsRunning = false;
		if (prWmmQuotaCtrl->rRunning.eReqIdx >= 0 &&
			prWmmQuotaCtrl->rRunning.eReqIdx <=
			CNM_WMM_REQ_DEFAULT &&
			prWmmQuotaCtrl->rRunning.eRunReq >= 0 &&
			prWmmQuotaCtrl->rRunning.eRunReq <=
			CNM_WMM_REQ_DEFAULT)
			DBGLOG(CNM, TRACE,
			"WmmQuotaCb,%u,%s,Run,%s,Quota,%u,Finish\n",
			ucWmmIndex,
			apucCnmWmmQuotaReq[prWmmQuotaCtrl->rRunning.eReqIdx],
			apucCnmWmmQuotaReq[prWmmQuotaCtrl->rRunning.eRunReq],
			prWmmQuotaCtrl->rRunning.u4ReqQuota);
	}
}

#if CFG_SUPPORT_DBDC
void cnmWmmQuotaSetMaxQuota(
	struct ADAPTER *prAdapter,
	uint8_t ucWmmIndex,
	enum ENUM_CNM_WMM_QUOTA_REQ_T eNewReq,
	bool fgEnable,
	uint32_t u4ReqQuota
)
{
	struct CNM_WMM_QUOTA_CONTROL_T *prWmmQuotaCtrl;
	enum ENUM_CNM_WMM_QUOTA_REQ_T eRunReq;
	uint32_t u4QuotaFinal = 0;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	prWmmQuotaCtrl = &(g_arWmmQuotaControl[ucWmmIndex]);

	if (eNewReq < 0 || eNewReq >= CNM_WMM_REQ_NUM) {
		DBGLOG(CNM, WARN, "Invalid eNewReq Idx %d!\n", eNewReq);
		return;
	}
	prWmmQuotaCtrl->arReqPool[eNewReq].fgEnable = fgEnable;
	prWmmQuotaCtrl->arReqPool[eNewReq].u4ReqQuota = u4ReqQuota;

	eRunReq = cnmWmmQuotaReqDispatcher(prWmmQuotaCtrl);
	if (eRunReq < 0 || eRunReq > CNM_WMM_REQ_DEFAULT
		|| eNewReq < 0 || eNewReq > CNM_WMM_REQ_DEFAULT) {
		DBGLOG(CNM, WARN, "Invalid req Idx %d!\n", eRunReq);
		return;
	} else if (eRunReq == CNM_WMM_REQ_DEFAULT) {
		/* unlimit */
		u4QuotaFinal = -1;
	} else {
		if (eRunReq < CNM_WMM_REQ_NUM)
			u4QuotaFinal = prWmmQuotaCtrl->
				arReqPool[eRunReq].u4ReqQuota;
	}

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_UPDATE_WMM_QUOTA);
	prAdapter->rWmmQuotaReqCS[ucWmmIndex].u4Quota = u4QuotaFinal;
	prAdapter->rWmmQuotaReqCS[ucWmmIndex].fgRun = true;
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_UPDATE_WMM_QUOTA);

	prWmmQuotaCtrl->rRunning.fgIsRunning = true;
	prWmmQuotaCtrl->rRunning.eReqIdx = eNewReq;
	prWmmQuotaCtrl->rRunning.eRunReq = eRunReq;
	prWmmQuotaCtrl->rRunning.u4ReqQuota = u4QuotaFinal;
	DBGLOG(CNM, TRACE,
			"SetWmmQuota,%u,%s %s,Run,%s,Quota,0x%x\n",
			ucWmmIndex,
			apucCnmWmmQuotaReq[prWmmQuotaCtrl->rRunning.eReqIdx],
			fgEnable ? "En" : "Dis",
			apucCnmWmmQuotaReq[prWmmQuotaCtrl->rRunning.eRunReq],
			prWmmQuotaCtrl->rRunning.u4ReqQuota);

	cnmWmmQuotaCallback(prAdapter, ucWmmIndex);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief check if p2p is active
 *
 * @param prAdapter
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
u_int8_t cnmP2pIsActive(struct ADAPTER *prAdapter)
{
	uint8_t ret;

	ret = (cnmGetP2pBssInfo(prAdapter) != NULL);
	DBGLOG(CNM, TRACE, "P2p is %s\n", ret ? "ACTIVE" : "INACTIVE");
	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief get p2p bss info
 *
 * @param prAdapter
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
struct BSS_INFO *cnmGetP2pBssInfo(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;

	if (!prAdapter)
		return NULL;

	for (i = 0; i < prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

		if (prBssInfo &&
		    IS_BSS_P2P(prBssInfo) &&
		    !p2pFuncIsAPMode(
		    prAdapter->rWifiVar.prP2PConnSettings
		    [prBssInfo->u4PrivateData]) &&
		    IS_BSS_ALIVE(prAdapter, prBssInfo))
			return prBssInfo;
	}

	return NULL;
}

enum ENUM_BAND_80211 cnmGet80211Band(enum ENUM_BAND eBand)
{
	enum ENUM_BAND_80211 eBand80211 = BAND_80211_NUM;

	switch (eBand) {
	case BAND_2G4:
		eBand80211 = BAND_80211_G;
		break;
	case BAND_5G:
#if (CFG_SUPPORT_WIFI_6G == 1)
	case BAND_6G:
#endif
		eBand80211 = BAND_80211_A;
		break;
	default:
		break;
	}

	return eBand80211;
}

#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief for customize
 *
 * @param level
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
bool isNeedBecomeOneNss(int level)
{
	return level > 2;
}

bool isNeedForceOneNss(int level)
{
	return level > 3;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief for control power level
 *
 * @param prAdapter
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
int cnmPowerControl(
	struct ADAPTER *prAdapter,
	uint8_t level)
{
	struct BSS_INFO *prSta0BssInfo;
	struct BSS_INFO *prSta1BssInfo;

	prAdapter->fgPowerForceOneNss = FALSE;
	prAdapter->fgPowerNeedDisconnect = FALSE;

	if (isNeedBecomeOneNss(level))
		prAdapter->fgPowerForceOneNss = TRUE;

	if (prAdapter->rWifiVar.eDbdcMode == ENUM_DBDC_MODE_DYNAMIC &&
		isNeedForceOneNss(level))
		prAdapter->fgPowerNeedDisconnect = TRUE;

	DBGLOG(CNM, INFO, "ForceOneNss=%d, NeedDisconnect=%d, dbdc=%d",
		prAdapter->fgPowerForceOneNss,
		prAdapter->fgPowerNeedDisconnect,
		prAdapter->rWifiVar.fgDbDcModeEn);

	/* DBDC enabled need to disconnect STA */
	if (prAdapter->rWifiVar.eDbdcMode == ENUM_DBDC_MODE_DYNAMIC &&
	    prAdapter->rWifiVar.fgDbDcModeEn && prAdapter->fgPowerForceOneNss) {
		/* check if dual sta */
		prSta0BssInfo = aisGetAisBssInfo(prAdapter, 0);
		prSta1BssInfo = aisGetAisBssInfo(prAdapter, 1);
		if (prSta1BssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
			prSta1BssInfo->u2DeauthReason =
				REASON_CODE_DISASSOC_LEAVING_BSS;
			aisFsmStateAbort(prAdapter,
				DISCONNECT_REASON_CODE_DISASSOCIATED,
				FALSE, 1);
		} else if (prSta0BssInfo->eConnectionState ==
				MEDIA_STATE_CONNECTED) {
			prSta0BssInfo->u2DeauthReason =
				REASON_CODE_DISASSOC_LEAVING_BSS;
			aisFsmStateAbort(prAdapter,
				DISCONNECT_REASON_CODE_DISASSOCIATED,
				FALSE, 0);
		}
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief for control power level error handling
 *
 * @param prAdapter, prBssInfo
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
void cnmPowerControlErrorHandling(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	DBGLOG(CNM, INFO, "eNetworkType=%d", prBssInfo->eNetworkType);
	switch (prBssInfo->eNetworkType) {
	case NETWORK_TYPE_AIS:
		prBssInfo->u2DeauthReason = REASON_CODE_DISASSOC_LEAVING_BSS;
		aisFsmStateAbort(prAdapter,
			DISCONNECT_REASON_CODE_DISASSOCIATED,
			FALSE, prBssInfo->ucBssIndex);
		break;
	case NETWORK_TYPE_P2P:
		p2pFuncDisconnect(prAdapter,
			prBssInfo,
			prBssInfo->prStaRecOfAP,
			TRUE,
			REASON_CODE_OP_MODE_CHANGE_FAIL,
			TRUE);
		break;
	default:
		break;
	}
}
#endif

#if CFG_WOW_SUPPORT
/*----------------------------------------------------------------------------*/
/*!
* @brief stop pending Join timer if suspend during AIS join.
*
* @param (prAdapter)
*
* @return None
*/
/*----------------------------------------------------------------------------*/
void cnmStopPendingJoinTimerForSuspend(struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	uint8_t i;

	if (prAdapter == NULL)
		return;


	for (i = 0; i < KAL_AIS_NUM; i++) {
		prAisFsmInfo = aisFsmGetInstance(prAdapter, i);

		/* Timer 1: rJoinTimeoutTimer
		 * Driver couldn't get any CH_GRANT event of CH_REQ after resume
		 * Because pending AIS join timer should do CH_ABORT to FW.
		 * Without CH_ABORT cmd, FW CNM's FSM would keep in GRANT stage.
		 * FW's CNM couldn't service any other CH_REQ in GRANT stage.
		 * As a result, checking the timer in suspend flow.
		 */
		if (prAisFsmInfo &&
		    timerPendingTimer(&prAisFsmInfo->rJoinTimeoutTimer)) {
			DBGLOG(CNM, STATE, "[AIS] pending rJoinTimeoutTimer\n");
			cnmTimerStopTimer(prAdapter,
				&prAisFsmInfo->rJoinTimeoutTimer);
			/* Release Channel */
			aisFsmReleaseCh(prAdapter,
			       aisGetMainLinkBssIndex(prAdapter, prAisFsmInfo));
		}


	}
}
#endif

uint8_t cnmIncreaseTokenId(struct ADAPTER *prAdapter)
{
	return ++prAdapter->ucCnmTokenID;
}

enum ENUM_CNM_MODE cnmGetMode(
	struct ADAPTER *prAdapter,
	uint8_t ucPreferBssIdx,
	uint8_t ucPreferChannel,
	enum ENUM_BAND ePreferBand)
{
	struct BSS_INFO *prBssInfo;
	uint32_t u4Idx;
	uint8_t ucLast2GChNum = 0, ucLast5GChNum = 0;
	bool fgIs2GMcc = false, fgIs5GMcc = false;

	for (u4Idx = 0; u4Idx < MAX_BSSID_NUM; u4Idx++) {
		uint8_t ucChannel;
		enum ENUM_BAND eBand;

		prBssInfo = prAdapter->aprBssInfo[u4Idx];

		if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo))
			continue;

		if ((ucPreferBssIdx == u4Idx) &&
			(!ucPreferChannel) &&
			(ePreferBand != BAND_NULL)) {
			eBand = ePreferBand;
			ucChannel = ucPreferChannel;
		} else {
			eBand = prBssInfo->eBand;
			ucChannel = prBssInfo->ucPrimaryChannel;
		}

		if (eBand == BAND_2G4) {
			if (ucLast2GChNum != 0 &&
			    ucLast2GChNum != ucChannel)
				fgIs2GMcc = true;
			ucLast2GChNum = ucChannel;
		} else if (eBand == BAND_5G) {
			if (ucLast5GChNum != 0 &&
			    ucLast5GChNum != ucChannel)
				fgIs5GMcc = true;
			ucLast5GChNum = ucChannel;
		}
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (eBand == BAND_6G) {
			/* Use the same handler as 5G channel */
			if (ucLast5GChNum != 0 &&
			    ucLast5GChNum != ucChannel)
				fgIs5GMcc = true;
			ucLast5GChNum = ucChannel;
		}
#endif
	}

	if (fgIs2GMcc || fgIs5GMcc)
		return ENUM_CNM_MODE_MCC;
	else if (ucLast2GChNum && ucLast5GChNum &&
		!prAdapter->rWifiVar.fgDbDcModeEn)
		return ENUM_CNM_MODE_MCC;
	else if (ucLast2GChNum && ucLast5GChNum)
		return ENUM_CNM_MODE_MBMC;
	else
		return ENUM_CNM_MODE_SCC;
}

void _cnmOwnGcCsaCmd(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	uint8_t ucChannel,
	enum ENUM_BAND eBand)
{
	struct CMD_SET_GC_CSA_STRUCT *prCmd;

	if (!prAdapter ||
		IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgP2pGcCsa))
		return;

	prCmd = (struct CMD_SET_GC_CSA_STRUCT *)
		cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(*prCmd));

	if (!prCmd) {
		DBGLOG(P2P, ERROR,
			"cnmMemAlloc for prCmd failed!\n");
		return;
	}

	DBGLOG(P2P, INFO,
		"Bss(%d) c=%d b=%d\n",
		ucBssIdx,
		ucChannel,
		eBand);

	prCmd->ucBssIdx = ucBssIdx;
	prCmd->ucChannel = ucChannel;
	prCmd->ucband = (uint8_t) eBand;

	wlanSendSetQueryCmd(prAdapter,
		CMD_ID_SET_P2P_GC_CSA,
		TRUE,
		FALSE,
		FALSE,
		NULL,
		NULL,
		sizeof(*prCmd),
		(uint8_t *) prCmd, NULL, 0);

	cnmMemFree(prAdapter, prCmd);
}

uint8_t cnmOwnGcCsaReq(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucCh,
	uint8_t ucRoleIdx)
{
	struct MSG_P2P_SET_NEW_CHANNEL *prMsg =
		(struct MSG_P2P_SET_NEW_CHANNEL *) NULL;
	uint8_t ucBssIdx = 0;

	DBGLOG(P2P, TRACE,
		"role(%d) c=%d b=%d\n",
		ucRoleIdx,
		ucCh,
		eBand);

	if (!prAdapter) {
		DBGLOG(P2P, WARN, "ad is not active\n");
		goto error;
	} else if (p2pFuncRoleToBssIdx(
		prAdapter, ucRoleIdx, &ucBssIdx) !=
		WLAN_STATUS_SUCCESS) {
		DBGLOG(P2P, WARN, "Incorrect role index");
		goto error;
	}

	/* Set new channel */
	prMsg = (struct MSG_P2P_SET_NEW_CHANNEL *)
		cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG, sizeof(*prMsg));
	if (prMsg == NULL) {
		DBGLOG(P2P, WARN,
			"prP2pSetNewChannelMsg alloc fail\n");
		goto error;
	}

	prMsg->rMsgHdr.eMsgId = MID_MNY_P2P_GC_CSA;
	prMsg->rRfChannelInfo.eBand = eBand;
	prMsg->rRfChannelInfo.ucChannelNum = ucCh;
	prMsg->ucRoleIdx = ucRoleIdx;
	prMsg->ucBssIndex = ucBssIdx;

	mboxSendMsg(prAdapter,
		MBOX_ID_0,
		(struct MSG_HDR *) prMsg,
		MSG_SEND_METHOD_BUF);

	return 0;

error:

	return -1;
}

void cnmOwnGcCsaHandler(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
	struct MSG_P2P_SET_NEW_CHANNEL *prMsg;
	struct RF_CHANNEL_INFO *prRfChannelInfo;
	struct STA_RECORD *prStaRec;
	enum ENUM_CNM_MODE eNextState;

	prMsg = (struct MSG_P2P_SET_NEW_CHANNEL *) prMsgHdr;
	prRfChannelInfo = &prMsg->rRfChannelInfo;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prMsg->ucBssIndex);
	if (!prP2pBssInfo ||
		!IS_BSS_GC(prP2pBssInfo) ||
		kalP2pIsStoppingAp(prAdapter,
		prP2pBssInfo)) {
		DBGLOG(P2P, ERROR,
			"BSS%d is disabled.\n",
			prMsg->ucBssIndex);
		goto error;
	}

	prStaRec = prP2pBssInfo->prStaRecOfAP;
	if (!prStaRec ||
		!prStaRec->ucGcCsaSupported) {
		DBGLOG(P2P, INFO,
			"BSS%d, peer GcCsa capa is disabled.\n",
			prP2pBssInfo->ucBssIndex);
		goto error;
	}
	eNextState = cnmGetMode(prAdapter,
		prMsg->ucBssIndex,
		prRfChannelInfo->ucChannelNum,
		prRfChannelInfo->eBand);

	if (eNextState == ENUM_CNM_MODE_MCC) {
		DBGLOG(CNM, INFO,
			"[CSA] GC to MCC, reject own GcCsa\n");
	} else {
		_cnmOwnGcCsaCmd(prAdapter,
			prMsg->ucBssIndex,
			prRfChannelInfo->ucChannelNum,
			prRfChannelInfo->eBand);
	}

error:
	cnmMemFree(prAdapter, prMsgHdr);
}

void cnmPeerGcCsaHandler(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_GC_CSA_T *prCsaEvent;
	enum ENUM_CNM_MODE eCurrState;
	enum ENUM_CNM_MODE eNextState;
	struct BSS_INFO *prBssInfo = NULL;

	DBGLOG(CNM, TRACE, "[CSA] CSA Req from GC\n");

	if (!prAdapter || !prEvent)
		return;

	/* parse Req channel from GC */
	prCsaEvent = (struct EVENT_GC_CSA_T *)
		&prEvent->aucBuffer;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prCsaEvent->ucBssIndex);
	if (!prBssInfo ||
		!IS_BSS_APGO(prBssInfo) ||
		kalP2pIsStoppingAp(prAdapter,
		prBssInfo)) {
		DBGLOG(P2P, ERROR,
			"BSS%d is disabled.\n",
			prCsaEvent->ucBssIndex);
		return;
	}

	eCurrState = cnmGetMode(prAdapter,
		0, 0, BAND_NULL);
	eNextState = cnmGetMode(prAdapter,
		prCsaEvent->ucBssIndex,
		prCsaEvent->ucChannel,
		(enum ENUM_BAND) prCsaEvent->ucBand);

	DBGLOG(CNM, INFO,
		"[CSA] %d->%d, Go: %d, %d, GcCsa: %d, %d\n",
		eCurrState,
		eNextState,
		prCsaEvent->ucBand,
		prCsaEvent->ucChannel,
		prBssInfo->eBand,
		prBssInfo->ucPrimaryChannel);

	if (eCurrState != ENUM_CNM_MODE_MCC &&
		eNextState == ENUM_CNM_MODE_MCC) {
		DBGLOG(CNM, INFO,
			"[CSA] GO step to MCC, reject peer GcCsa\n");
	} else {
		cnmIdcCsaReq(prAdapter,
			prCsaEvent->ucBand,
			prCsaEvent->ucChannel,
			prBssInfo->u4PrivateData);
	}
}

