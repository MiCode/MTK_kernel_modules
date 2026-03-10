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
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/include/wlan_bow.h#1
*/

/*! \file   "wlan_bow.h"
*    \brief This file contains the declairations of 802.11 PAL
*	   command processing routines for
*	   MediaTek Inc. 802.11 Wireless LAN Adapters.
*/


#ifndef _WLAN_BOW_H
#define _WLAN_BOW_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "nic/bow.h"
#include "nic/cmd_buf.h"

#if CFG_ENABLE_BT_OVER_WIFI

extern uint32_t g_arBowRevPalPacketTime[32];

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define BOWCMD_STATUS_SUCCESS       0
#define BOWCMD_STATUS_FAILURE       1
#define BOWCMD_STATUS_UNACCEPTED    2
#define BOWCMD_STATUS_INVALID       3
#define BOWCMD_STATUS_TIMEOUT       4

#define BOW_WILDCARD_SSID               "AMP"
#define BOW_WILDCARD_SSID_LEN       3
#define BOW_SSID_LEN                            21

 /* 0: query, 1: setup, 2: destroy */
#define BOW_QUERY_CMD                   0
#define BOW_SETUP_CMD                   1
#define BOW_DESTROY_CMD               2

#define BOW_INITIATOR                   0
#define BOW_RESPONDER                  1

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct BOW_TABLE {
	uint8_t ucAcquireID;
	u_int8_t fgIsValid;
	enum ENUM_BOW_DEVICE_STATE eState;
	uint8_t aucPeerAddress[6];
	/* UINT_8                      ucRole; */
	/* UINT_8                      ucChannelNum; */
	uint16_t u2Reserved;
};

typedef uint32_t(*PFN_BOW_CMD_HANDLE) (struct ADAPTER *, struct BT_OVER_WIFI_COMMAND *);

struct BOW_CMD {
	uint8_t uCmdID;
	PFN_BOW_CMD_HANDLE pfCmdHandle;
};

struct BOW_EVENT_ACTIVITY_REPORT {
	uint8_t ucReason;
	uint8_t aucReserved;
	uint8_t aucPeerAddress[6];
};

/*
*ucReason:	0: success
*	1: general failure
*	2: too much time (> 2/3 second totally) requested for scheduling.
*	Others: reserved.
*/

struct BOW_EVENT_SYNC_TSF {
	uint64_t u4TsfTime;
	uint32_t u4TsfSysTime;
	uint32_t u4ScoTime;
	uint32_t u4ScoSysTime;
};

struct BOW_ACTIVITY_REPORT_BODY {
	uint32_t u4StartTime;
	uint32_t u4Duration;
	uint32_t u4Periodicity;
};

struct BOW_ACTIVITY_REPORT {
	uint8_t aucPeerAddress[6];
	uint8_t ucScheduleKnown;
	uint8_t ucNumReports;
	struct BOW_ACTIVITY_REPORT_BODY arBowActivityReportBody[MAX_ACTIVITY_REPORT];
};

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
/*--------------------------------------------------------------*/
/* Firmware Command Packer                                      */
/*--------------------------------------------------------------*/
uint32_t
wlanoidSendSetQueryBowCmd(struct ADAPTER *prAdapter,
			  uint8_t ucCID,
			  uint8_t ucBssIdx,
			  u_int8_t fgSetQuery,
			  u_int8_t fgNeedResp,
			  PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
			  PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
			  uint32_t u4SetQueryInfoLen, uint8_t *pucInfoBuffer,
			  uint8_t ucSeqNumber);

/*--------------------------------------------------------------*/
/* Command Dispatcher                                           */
/*--------------------------------------------------------------*/
uint32_t wlanbowHandleCommand(struct ADAPTER *prAdapter,
		struct BT_OVER_WIFI_COMMAND *prCmd);

/*--------------------------------------------------------------*/
/* Routines to handle command                                   */
/*--------------------------------------------------------------*/
uint32_t bowCmdGetMacStatus(struct ADAPTER *prAdapter,
		struct BT_OVER_WIFI_COMMAND *prCmd);

uint32_t bowCmdSetupConnection(struct ADAPTER *prAdapter,
		struct BT_OVER_WIFI_COMMAND *prCmd);

uint32_t bowCmdDestroyConnection(struct ADAPTER *prAdapter,
		struct BT_OVER_WIFI_COMMAND *prCmd);

uint32_t bowCmdSetPTK(struct ADAPTER *prAdapter,
		struct BT_OVER_WIFI_COMMAND *prCmd);

uint32_t bowCmdReadRSSI(struct ADAPTER *prAdapter,
		struct BT_OVER_WIFI_COMMAND *prCmd);

uint32_t bowCmdReadLinkQuality(struct ADAPTER *prAdapter,
		struct BT_OVER_WIFI_COMMAND *prCmd);

uint32_t bowCmdShortRangeMode(struct ADAPTER *prAdapter,
		struct BT_OVER_WIFI_COMMAND *prCmd);

uint32_t bowCmdGetChannelList(struct ADAPTER *prAdapter,
		struct BT_OVER_WIFI_COMMAND *prCmd);

void wlanbowCmdEventSetStatus(struct ADAPTER *prAdapter,
		struct BT_OVER_WIFI_COMMAND *prCmd, uint8_t ucEventBuf);

/*--------------------------------------------------------------*/
/* Callbacks for event indication                               */
/*--------------------------------------------------------------*/
void wlanbowCmdEventSetCommon(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void wlanbowCmdEventLinkConnected(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void wlanbowCmdEventLinkDisconnected(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void wlanbowCmdEventSetSetupConnection(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void wlanbowCmdEventReadLinkQuality(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void wlanbowCmdEventReadRssi(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

uint8_t bowInit(struct ADAPTER *prAdapter);

void bowUninit(struct ADAPTER *prAdapter);

void wlanbowCmdTimeoutHandler(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo);

void bowStopping(struct ADAPTER *prAdapter);

void bowStarting(struct ADAPTER *prAdapter);

void bowAssignSsid(uint8_t *pucSsid, uint8_t *pucSsidLen);

u_int8_t bowValidateProbeReq(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
		uint32_t *pu4ControlFlags);

void bowSendBeacon(struct ADAPTER *prAdapter, uintptr_t ulParamPtr);

void bowResponderScan(struct ADAPTER *prAdapter);

void bowResponderScanDone(struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr);

void bowResponderCancelScan(struct ADAPTER *prAdapter,
		u_int8_t fgIsChannelExtention);

void bowResponderJoin(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc);

void bowFsmRunEventJoinComplete(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void
bowIndicationOfMediaStateToHost(struct ADAPTER *prAdapter,
				enum ENUM_PARAM_MEDIA_STATE eConnectionState,
				u_int8_t fgDelayIndication);

void bowRunEventAAATxFail(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec);

uint32_t bowRunEventAAAComplete(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec);

uint32_t bowRunEventRxDeAuth(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec, struct SW_RFB *prSwRfb);

void bowDisconnectLink(struct ADAPTER *prAdapter, struct MSDU_INFO *prMsduInfo,
		enum ENUM_TX_RESULT_CODE rTxDoneStatus);

u_int8_t bowValidateAssocReq(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
		uint16_t *pu2StatusCode);

u_int8_t
bowValidateAuth(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb, struct STA_RECORD **pprStaRec,
		uint16_t *pu2StatusCode);

void bowRunEventChGrant(struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr);

void bowRequestCh(struct ADAPTER *prAdapter);

void bowReleaseCh(struct ADAPTER *prAdapter);

void bowChGrantedTimeout(struct ADAPTER *prAdapter, uintptr_t ulParamPtr);

u_int8_t bowNotifyAllLinkDisconnected(struct ADAPTER *prAdapter);

u_int8_t bowCheckBowTableIfVaild(struct ADAPTER *prAdapter,
		uint8_t aucPeerAddress[6]);

u_int8_t bowGetBowTableContent(struct ADAPTER *prAdapter, uint8_t ucBowTableIdx,
		struct BOW_TABLE *prBowTable);
u_int8_t
bowGetBowTableEntryByPeerAddress(struct ADAPTER *prAdapter,
		uint8_t aucPeerAddress[6], uint8_t *pucBowTableIdx);

u_int8_t bowGetBowTableFreeEntry(struct ADAPTER *prAdapter,
		uint8_t *pucBowTableIdx);

enum ENUM_BOW_DEVICE_STATE bowGetBowTableState(struct ADAPTER *prAdapter,
		uint8_t aucPeerAddress[6]);

u_int8_t bowSetBowTableState(struct ADAPTER *prAdapter,
		uint8_t aucPeerAddress[6], enum ENUM_BOW_DEVICE_STATE eState);

u_int8_t bowSetBowTableContent(struct ADAPTER *prAdapter, uint8_t ucBowTableIdx,
		struct BOW_TABLE *prBowTable);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif
#endif /* _WLAN_BOW_H */
