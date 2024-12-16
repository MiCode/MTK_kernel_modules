/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
** Id:
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
/* Roaming Discovery interval, SCAN result need to be updated */
#define ROAMING_DISCOVERY_TIMEOUT_SEC               5	/* Seconds. */
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
#define ROAMING_ONE_AP_SKIP_TIMES		3
#endif

/* #define ROAMING_NO_SWING_RCPI_STEP                  5 //rcpi */
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
typedef enum _ENUM_ROAMING_FAIL_REASON_T {
	ROAMING_FAIL_REASON_CONNLIMIT = 0,
	ROAMING_FAIL_REASON_NOCANDIDATE,
	ROAMING_FAIL_REASON_NUM
} ENUM_ROAMING_FAIL_REASON_T;

/* events of roaming between driver and firmware */
typedef enum _ENUM_ROAMING_EVENT_T {
	ROAMING_EVENT_START = 0,
	ROAMING_EVENT_DISCOVERY,
	ROAMING_EVENT_ROAM,
	ROAMING_EVENT_FAIL,
	ROAMING_EVENT_ABORT,
	ROAMING_EVENT_NUM
} ENUM_ROAMING_EVENT_T;

typedef enum _ENUM_ROAMING_REASON_T {
	ROAMING_REASON_POOR_RCPI = 0,
	ROAMING_REASON_TX_ERR, /*Lowest rate, high PER*/
	ROAMING_REASON_RETRY,
	ROAMING_REASON_NUM
} ENUM_ROAMING_REASON_T;

typedef struct _CMD_ROAMING_TRANSIT_T {
	UINT_16	u2Event;
	UINT_16	u2Data;
	UINT_16	u2RcpiLowThreshold;
	UINT_8	ucIsSupport11B;
	UINT_8	aucReserved[1];
	ENUM_ROAMING_REASON_T	eReason;
	UINT_32	u4RoamingTriggerTime; /*sec in mcu*/
	UINT_8 aucReserved2[8];
} CMD_ROAMING_TRANSIT_T, *P_CMD_ROAMING_TRANSIT_T;


typedef struct _CMD_ROAMING_CTRL_T {
	UINT_8 fgEnable;
	UINT_8 ucRcpiAdjustStep;
	UINT_16 u2RcpiLowThr;
	UINT_8 ucRoamingRetryLimit;
	UINT_8 ucRoamingStableTimeout;
	UINT_8 aucReserved[2];
} CMD_ROAMING_CTRL_T, *P_CMD_ROAMING_CTRL_T;

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
typedef struct _CMD_ROAMING_SKIP_ONE_AP_T {
	  UINT_8	  fgIsRoamingSkipOneAP;
	  UINT_8	  aucReserved[3];
	  UINT_8	  aucReserved2[8];
} CMD_ROAMING_SKIP_ONE_AP_T, *P_CMD_ROAMING_SKIP_ONE_AP_T;
#endif

 /**/ typedef enum _ENUM_ROAMING_STATE_T {
	ROAMING_STATE_IDLE = 0,
	ROAMING_STATE_DECISION,
	ROAMING_STATE_DISCOVERY,
	ROAMING_STATE_ROAM,
	ROAMING_STATE_NUM
} ENUM_ROAMING_STATE_T;

typedef struct _ROAMING_INFO_T {
	BOOLEAN fgIsEnableRoaming;

	ENUM_ROAMING_STATE_T eCurrentState;

	OS_SYSTIME rRoamingDiscoveryUpdateTime;
} ROAMING_INFO_T, *P_ROAMING_INFO_T;

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

#if CFG_SUPPORT_ROAMING
#define IS_ROAMING_ACTIVE(prAdapter) \
	(prAdapter->rWifiVar.rRoamingInfo.eCurrentState == ROAMING_STATE_ROAM)
#else
#define IS_ROAMING_ACTIVE(prAdapter) FALSE
#endif /* CFG_SUPPORT_ROAMING */

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
VOID roamingFsmInit(IN P_ADAPTER_T prAdapter);

VOID roamingFsmUninit(IN P_ADAPTER_T prAdapter);

VOID roamingFsmSendCmd(IN P_ADAPTER_T prAdapter, IN P_CMD_ROAMING_TRANSIT_T prTransit);

VOID roamingFsmScanResultsUpdate(IN P_ADAPTER_T prAdapter);

VOID roamingFsmSteps(IN P_ADAPTER_T prAdapter, IN ENUM_ROAMING_STATE_T eNextState);

VOID roamingFsmRunEventStart(IN P_ADAPTER_T prAdapter);

VOID roamingFsmRunEventDiscovery(IN P_ADAPTER_T prAdapter, IN P_CMD_ROAMING_TRANSIT_T prTransit);

VOID roamingFsmRunEventRoam(IN P_ADAPTER_T prAdapter);

VOID roamingFsmRunEventFail(IN P_ADAPTER_T prAdapter, IN UINT_32 u4Reason);

VOID roamingFsmRunEventAbort(IN P_ADAPTER_T prAdapter);

WLAN_STATUS roamingFsmProcessEvent(IN P_ADAPTER_T prAdapter, IN P_CMD_ROAMING_TRANSIT_T prTransit);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _ROAMING_FSM_H */
