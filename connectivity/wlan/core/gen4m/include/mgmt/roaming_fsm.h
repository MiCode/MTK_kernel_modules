/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "roaming_fsm.h"
 *    \brief  This file defines the FSM for Roaming MODULE.
 *
 *    This file defines the FSM for Roaming MODULE.
 */


#ifndef _ROAMING_FSM_H
#define _ROAMING_FSM_H

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
/* Roaming Discovery interval, SCAN result need to be updated */
#if (CFG_EXT_ROAMING == 1)
#define ROAMING_DISCOVER_TIMEOUT_SEC		0	/* Seconds. */
#else
#define ROAMING_DISCOVER_TIMEOUT_SEC		10	/* Seconds. */
#endif
#define ROAMING_INACTIVE_TIMEOUT_SEC		10	/* Seconds. */
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
#define ROAMING_ONE_AP_SKIP_TIMES		3
#endif
#define ROAMING_BTM_DELTA			0	/* % */

#define ROAMING_RECOVER_RLM_SYNC		0
#define ROAMING_RECOVER_BSS_UPDATE		1

#define ROAMING_SCAN_NON_DFS_CH_DWELL_TIME	(20)	/* Ms */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

enum ENUM_ROAMING_FAIL_REASON {
	ROAMING_FAIL_REASON_CONNLIMIT = 0,
	ROAMING_FAIL_REASON_NOCANDIDATE,
	ROAMING_FAIL_REASON_NUM
};

/* events of roaming between driver and firmware */
enum ENUM_ROAMING_EVENT {
	ROAMING_EVENT_START = 0,
	ROAMING_EVENT_DISCOVERY,
	ROAMING_EVENT_ROAM,
	ROAMING_EVENT_FAIL,
	ROAMING_EVENT_ABORT,
	ROAMING_EVENT_THRESHOLD_UPDATE,
	ROAMING_EVENT_NUM
};

struct CMD_ROAMING_TRANSIT {
	uint16_t u2Event;
	uint16_t u2Data;
	uint16_t u2RcpiLowThreshold;
	uint8_t ucIsSupport11B;
	uint8_t ucBssidx;
	enum ENUM_ROAMING_REASON eReason;
	uint32_t u4RoamingTriggerTime; /*sec in mcu*/
	uint16_t u2RcpiHighThreshold;
	uint8_t aucReserved2[6];
};

struct CMD_ROAMING_CTRL {
	uint8_t fgEnable;
	uint8_t ucRcpiAdjustStep;
	uint16_t u2RcpiLowThr;
	uint8_t ucRoamingRetryLimit;
	uint8_t ucRoamingStableTimeout;
	uint8_t aucReserved[2];
};

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
struct CMD_ROAMING_SKIP_ONE_AP {
	uint8_t	  fgIsRoamingSkipOneAP;
	uint8_t	  aucReserved[3];
	uint8_t	  aucReserved2[8];
};
#endif

enum ENUM_ROAMING_STATE {
	ROAMING_STATE_IDLE = 0,
	ROAMING_STATE_DECISION,
	ROAMING_STATE_DISCOVERY,
	ROAMING_STATE_ROAM,
	ROAMING_STATE_HANDLE_NEW_CANDIDATE,
	ROAMING_STATE_SEND_WNM_RESP,
	ROAMING_STATE_SEND_FT_REQUEST,
	ROAMING_STATE_WAIT_FT_RESPONSE,
	ROAMING_STATE_NUM
};

struct ROAMING_EVENT_INFO {
	uint8_t ucStatus;
	uint8_t aucPrevBssid[MAC_ADDR_LEN];
	uint8_t aucCurrBssid[MAC_ADDR_LEN];
	uint8_t ucPrevChannel;
	uint8_t ucCurrChannel;
	uint8_t ucPrevRcpi;
	uint8_t ucCurrRcpi;
	uint8_t ucBw;
	uint16_t u2ApLoading;
	uint8_t ucSupportStbc;
};

#if (CFG_EXT_ROAMING == 1)
enum ENUM_ROAMING_SCAN_SORUCE {
	ROAMING_SCAN_INVALID = 0,
	ROAMING_SCAN_FORCE_FULL,       /* Initial connection */
	ROAMING_SCAN_INACTIVE_TIMER,   /* Inactive Timer */
	ROAMING_SCAN_SINGLE_TIMER,     /* Scan Timer-1 */
	ROAMING_SCAN_PERIODIC_TIMER,   /* Scan Timer-2 */
	ROAMING_SCAN_NUM
};

struct ROAMING_SCAN_CADENCE {
	struct TIMER rScanTimer;
	uint8_t fgIsInitialConn;
	uint8_t ucScanSource;
	uint8_t ucFullScanCount;
	uint32_t u4ScanScheduleSec;
};
#endif

enum ENUM_ROAMING_SCAN_TYPE {
	ROAMING_SCAN_TYPE_NORMAL = 0,
	ROAMING_SCAN_TYPE_PARTIAL_ONLY,     /* Only perform partial scan */
	ROAMING_SCAN_TYPE_FULL_ONLY,        /* Only perform full scan */
	ROAMING_SCAN_TYPE_NUM
};

enum ENUM_ROAMING_SCAN_MODE {
	ROAMING_SCAN_MODE_NORMAL = 0,
	ROAMING_SCAN_MODE_LOW_LATENCY,
	ROAMING_SCAN_MODE_NUM
};

struct ROAMING_SCAN_PARAMETER {
	uint8_t ucScanType;
	uint8_t ucScanCount;
	uint8_t ucScanMode;
};

struct ROAMING_SKIP_CONFIG {
	uint8_t ucConsecutiveCount;
	OS_SYSTIME rFirstReqTime;
};

struct ROAMING_INFO {
	enum ENUM_ROAMING_STATE eCurrentState;

	/* sync with FW: NUM(init) -> START -> FAIL -> ABORT */
	enum ENUM_ROAMING_EVENT eCurrentEvent[MAX_BSSID_NUM];
	uint32_t u4BssIdxBmap;

	OS_SYSTIME rRoamingDiscoveryUpdateTime;
#if CFG_SUPPORT_DRIVER_ROAMING
	OS_SYSTIME rRoamingLastDecisionTime;
#endif

	enum ENUM_ROAMING_REASON eReason;
	uint8_t ucPER;
	uint8_t ucRcpi;
	uint8_t ucThreshold;
	struct ROAMING_EVENT_INFO rEventInfo;
#if (CFG_EXT_ROAMING == 1)
	struct ROAMING_SCAN_CADENCE rScanCadence;
#endif
	struct ROAMING_SKIP_CONFIG rSkipBtmInfo;
	u_int8_t fgDisallowBtmRoaming;

	struct TIMER rTxReqDoneRxRespTimer;
	struct BSS_DESC_SET *prRoamTarget;
	uint8_t ucTxActionRetryCount;

	struct ROAMING_SCAN_PARAMETER rRoamScanParam;
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
void roamingFsmInit(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void roamingFsmUninit(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void roamingFsmSendCmd(struct ADAPTER *prAdapter,
	struct CMD_ROAMING_TRANSIT *prTransit);

void roamingFsmSendStartCmd(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void roamingFsmSendAbortCmd(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void roamingFsmScanResultsUpdate(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void roamingFsmSteps(struct ADAPTER *prAdapter,
	enum ENUM_ROAMING_STATE eNextState,
	uint8_t ucBssIndex);

void roamingFsmRunEventStart(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void roamingFsmRunEventDiscovery(struct ADAPTER *prAdapter,
	struct CMD_ROAMING_TRANSIT *prTransit);

void roamingFsmRunEventFail(struct ADAPTER *prAdapter,
	uint8_t ucReason,
	uint8_t ucBssIndex);

void roamingFsmRunEventAbort(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void roamingFsmRunEventNewCandidate(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prRoamTarget,
	uint8_t ucBssIndex);

void roamingFsmNotifyEvent(struct ADAPTER *adapter, uint8_t bssIndex,
	uint8_t ucFail, struct BSS_DESC *prBssDesc);

uint32_t roamingFsmProcessEvent(struct ADAPTER *prAdapter,
	struct CMD_ROAMING_TRANSIT *prTransit);

void roamingFsmSetRecoverBitmap(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t ucScenario);

void roamingFsmDoRecover(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

uint8_t roamingFsmInDecision(struct ADAPTER *prAdapter,
	u_int8_t fgIgnorePolicy, uint8_t ucBssIndex);

void roamingFsmRunEventRxFtAction(struct ADAPTER *prAdapter,
			  struct SW_RFB *prSwRfb);

void roamingFsmTxReqDoneOrRxRespTimeout(
	struct ADAPTER *prAdapter, uintptr_t ulParam);

u_int8_t roamingFsmCheckIfRoaming(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void roamingFsmBTMTimeout(struct ADAPTER *prAdapter,
	uintptr_t ulParamPtr);
#endif /* _ROAMING_FSM_H */
