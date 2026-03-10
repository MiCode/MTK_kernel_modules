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
 ** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/ais_fsm.c#4
 */

/*! \file   "aa_fsm.c"
 *    \brief  This file defines the FSM for SAA and AAA MODULE.
 *
 *    This file defines the FSM for SAA and AAA MODULE.
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

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
struct WIFI_RTT_RESULT {
	struct RTT_RESULT rResult;
	void *pLCI;
	void *pLCR;
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static void rttRequestDoneTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam);
static void rttFreeAllResults(struct RTT_INFO *prRttInfo);
static void rttUpdateStatus(struct ADAPTER *prAdapter,
	struct CMD_RTT_REQUEST *prCmd);
static struct RTT_INFO *rttGetInfo(struct ADAPTER *prAdapter);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void rttInit(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	ASSERT(rttInfo);

	rttInfo->fgIsRunning = false;
	rttInfo->ucSeqNum = 0;

	cnmTimerInitTimer(prAdapter,
		  &rttInfo->rRttDoneTimer,
		  (PFN_MGMT_TIMEOUT_FUNC) rttRequestDoneTimeOut,
		  (uintptr_t)NULL);

	LINK_INITIALIZE(&rttInfo->rResultList);

}

void rttUninit(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	ASSERT(rttInfo);

	rttFreeAllResults(rttInfo);
	rttUpdateStatus(prAdapter, NULL);
}

struct RTT_INFO *rttGetInfo(struct ADAPTER *prAdapter)
{
	if (prAdapter)
		return &(prAdapter->rWifiVar.rRttInfo);
	else
		return NULL;
}

void rttFreeAllResults(struct RTT_INFO *prRttInfo)
{
	struct RTT_RESULT_ENTRY *entry;

	while (!LINK_IS_EMPTY(&prRttInfo->rResultList)) {
		LINK_REMOVE_HEAD(&prRttInfo->rResultList,
			entry, struct RTT_RESULT_ENTRY*);
		kalMemFree(entry, VIR_MEM_TYPE,
			sizeof(struct RTT_RESULT_ENTRY) +
			entry->u2IELen);
	}
}

void rttUpdateStatus(struct ADAPTER *prAdapter,
	struct CMD_RTT_REQUEST *prCmd)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

	if (prCmd && prCmd->fgEnable) {
		rttInfo->ucSeqNum = prCmd->ucSeqNum;
		rttInfo->fgIsRunning = true;
		cnmTimerStartTimer(prAdapter,
				   &rttInfo->rRttDoneTimer,
				   SEC_TO_MSEC(RTT_REQUEST_DONE_TIMEOUT_SEC));
	} else {
		rttInfo->fgIsRunning = false;
		cnmTimerStopTimer(prAdapter, &rttInfo->rRttDoneTimer);
	}
}

uint32_t rttSendCmd(struct ADAPTER *prAdapter,
	struct CMD_RTT_REQUEST *cmd)
{
	uint32_t status;

	status = wlanSendSetQueryCmd(prAdapter,
			CMD_ID_RTT_RANGE_REQUEST,
			TRUE,
			FALSE,
			FALSE,
			nicCmdEventSetCommon,
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_RTT_REQUEST),
			(uint8_t *) cmd, NULL, 0);
	if (status != WLAN_STATUS_FAILURE) {
		/* send cmd to FW successfully */
		rttUpdateStatus(prAdapter, cmd);
		return WLAN_STATUS_SUCCESS;
	}

	return WLAN_STATUS_FAILURE;
}

void rttActiveNetwork(struct ADAPTER *prAdapter,
			    uint8_t ucBssIndex, uint8_t active)
{
	if (active) {
		SET_NET_ACTIVE(prAdapter, ucBssIndex);
		nicActivateNetwork(prAdapter, ucBssIndex);
	} else {
		UNSET_NET_ACTIVE(prAdapter, ucBssIndex);
		nicDeactivateNetwork(prAdapter, ucBssIndex);
	}

}

uint32_t rttStartRttRequest(struct ADAPTER *prAdapter,
			 struct PARAM_RTT_REQUEST *prRequest,
			 uint8_t ucBssIndex)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct BSS_DESC *bss;
	struct CMD_RTT_REQUEST *cmd;
	uint8_t i, active;
	uint32_t status;
	uint32_t sz = sizeof(struct CMD_RTT_REQUEST);


	cmd = (struct CMD_RTT_REQUEST *)
			cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sz);
	if (!cmd)
		return WLAN_STATUS_RESOURCES;

	active = IS_NET_ACTIVE(prAdapter, ucBssIndex);
	if (!active)
		rttActiveNetwork(prAdapter, ucBssIndex, true);

	kalMemZero(cmd, sizeof(struct CMD_RTT_REQUEST));
	cmd->ucSeqNum = rttInfo->ucSeqNum + 1;
	cmd->fgEnable = true;
	cmd->ucConfigNum = 0;

	for (i = 0; i < prRequest->ucConfigNum; i++) {
		struct RTT_CONFIG *rc = &prRequest->arRttConfigs[i];
		struct RTT_CONFIG *tc = NULL;

		/* add to fw cmd if bss exist */
		bss = scanSearchBssDescByBssid(prAdapter, rc->aucAddr);

		if (bss) {
			tc = &cmd->arRttConfigs[cmd->ucConfigNum++];
			COPY_MAC_ADDR(tc->aucAddr, rc->aucAddr);
			tc->eType = rc->eType;
			tc->rChannel = rc->rChannel;
			tc->ucBurstPeriod = rc->ucBurstPeriod;
			tc->ucNumFramesPerBurst = rc->ucNumFramesPerBurst;
			tc->ucNumRetriesPerFtmr = rc->ucNumRetriesPerFtmr;
			tc->ucLciRequest = rc->ucLciRequest;
			tc->ucLcrRequest = rc->ucLcrRequest;
			tc->ucBurstDuration = rc->ucBurstDuration;
			tc->ePreamble = rc->ePreamble;
			tc->eBw = rc->eBw;
			/* TODO: set TSF Time */
		} else {
			DBGLOG(RTT, ERROR,
				"Bssid " MACSTR " is not in scan result\n",
				MAC2STR(rc->aucAddr));
			cnmMemFree(prAdapter, (void *) cmd);
			return WLAN_STATUS_NOT_ACCEPTED;
		}
	}

	status = rttSendCmd(prAdapter, cmd);
	if (status != WLAN_STATUS_SUCCESS) {
		/* fail to send cmd, restore active network */
		if (!active)
			rttActiveNetwork(prAdapter, ucBssIndex, false);
	}

	cnmMemFree(prAdapter, (void *) cmd);
	DBGLOG(RTT, INFO, "status=%d, seq=%d", status, rttInfo->ucSeqNum);

	return status;
}

uint32_t rttCancelRttRequest(struct ADAPTER *prAdapter,
			 struct PARAM_RTT_REQUEST *prRequest)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct CMD_RTT_REQUEST *cmd;
	uint32_t status;
	uint32_t sz = sizeof(struct CMD_RTT_REQUEST);

	cmd = (struct CMD_RTT_REQUEST *)
			cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sz);
	if (!cmd)
		return WLAN_STATUS_RESOURCES;

	cmd->ucSeqNum = rttInfo->ucSeqNum;
	cmd->fgEnable = false;
	status = rttSendCmd(prAdapter, cmd);

	cnmMemFree(prAdapter, (void *) cmd);
	DBGLOG(RTT, INFO, "status=%d, seq=%d", status, rttInfo->ucSeqNum);

	return status;
}

uint32_t rttHandleRttRequest(struct ADAPTER *prAdapter,
			 struct PARAM_RTT_REQUEST *prRequest,
			 uint8_t ucBssIndex)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	uint32_t status;

	ASSERT(rttInfo);
	ASSERT(prRequest);

	rttInfo = &(prAdapter->rWifiVar.rRttInfo);

	if (prRequest->ucConfigNum > CFG_RTT_MAX_CANDIDATES ||
	    prRequest->ucConfigNum <= 0 ||
	    (prRequest->fgEnable && rttInfo->fgIsRunning) ||
	    (!prRequest->fgEnable && !rttInfo->fgIsRunning))
		return WLAN_STATUS_NOT_ACCEPTED;

	if (prRequest->fgEnable) {
		status = rttStartRttRequest(prAdapter, prRequest, ucBssIndex);
	} else {
		rttFreeAllResults(rttInfo);
		status = rttCancelRttRequest(prAdapter, prRequest);
	}
	return status;
}

static void rttRequestDoneTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam)
{
	rttEventDone(prAdapter, NULL);
}

void rttReportDone(struct ADAPTER *prAdapter)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	void *pvPacket = NULL;
	struct RTT_RESULT_ENTRY *entry;
	uint32_t u4Count = 0, sz = sizeof(struct WIFI_RTT_RESULT);
	uint32_t u4DataLen = 0, complete = TRUE;

	LINK_FOR_EACH_ENTRY(entry, &rttInfo->rResultList, rLinkEntry,
			    struct RTT_RESULT_ENTRY) {
		u4DataLen += sz + entry->u2IELen;
		u4Count++;
	}
	pvPacket = kalProcessRttReportDone(prAdapter->prGlueInfo,
				u4DataLen, u4Count);

	if (!pvPacket)
		complete = FALSE;

	while (!LINK_IS_EMPTY(&rttInfo->rResultList)) {
		uint32_t size;
		struct WIFI_RTT_RESULT *r = NULL;

		LINK_REMOVE_HEAD(&rttInfo->rResultList,
			entry, struct RTT_RESULT_ENTRY*);
		size = sz + entry->u2IELen;
		r = kalMemAlloc(size, VIR_MEM_TYPE);
		if (r) {
			kalMemCopy(&r->rResult, &entry->rResult,
				sizeof(struct RTT_RESULT));
			r->pLCI = NULL;
			r->pLCR = NULL;
			kalMemCopy(r + 1, entry->aucIE, entry->u2IELen);
			if (unlikely(kalNlaPut(pvPacket,
				RTT_ATTRIBUTE_RESULT, size, r) < 0)) {
				DBGLOG(RTT, ERROR, "put result fail");
				complete = FALSE;
			}
		}
		kalMemFree(r, VIR_MEM_TYPE, size);
		kalMemFree(entry, VIR_MEM_TYPE,
			sizeof(struct RTT_RESULT_ENTRY) + entry->u2IELen);
	}

	if (complete)
		kalCfg80211VendorEvent(pvPacket);
	else
		kalKfreeSkb(pvPacket, TRUE);
}

void rttFakeEvent(struct ADAPTER *prAdapter)
{
	struct EVENT_RTT_RESULT fake;
	uint8_t aucMacAddr[MAC_ADDR_LEN] = {0xa0, 0xab, 0x1b, 0x54, 0x65, 0x34};

	COPY_MAC_ADDR(fake.rResult.aucMacAddr, aucMacAddr);
	fake.rResult.u4BurstNum = 1;
	fake.rResult.u4MeasurementNumber  = 1;
	fake.rResult.u4SuccessNumber  = 1;
	fake.rResult.ucNumPerBurstPeer = 2;
	fake.rResult.eStatus = RTT_STATUS_SUCCESS;
	fake.rResult.ucRetryAfterDuration = 0;
	fake.rResult.eType = RTT_TYPE_1_SIDED;
	fake.rResult.i4Rssi = -50;
	fake.rResult.i4RssiSpread = 2;
	fake.rResult.rTxRate.preamble = 0;
	fake.rResult.rTxRate.nss = 1;
	fake.rResult.rTxRate.bw = 0;
	fake.rResult.rTxRate.rateMcsIdx = 2;
	fake.rResult.rTxRate.bitrate = 15;
	fake.rResult.rRxRate.preamble = 0;
	fake.rResult.rRxRate.nss = 1;
	fake.rResult.rRxRate.bw = 0;
	fake.rResult.rRxRate.rateMcsIdx = 2;
	fake.rResult.rRxRate.bitrate = 15;
	fake.rResult.i8Rtt = 5000;
	fake.rResult.i8RttSd = 300;
	fake.rResult.i8RttSpread = 400;
	fake.rResult.i4DistanceMM = 3500;
	fake.rResult.i4DistanceSdMM = 100;
	fake.rResult.i4DistanceSpreadMM =  100;
	fake.rResult.i8Ts = kalGetBootTime();
	fake.rResult.i4BurstDuration = 2;
	fake.rResult.i4NegotiatedBustNum = 4;
	fake.u2IELen = 0;

	rttEventResult(prAdapter, &fake);
}

void rttEventDone(struct ADAPTER *prAdapter,
		      struct EVENT_RTT_DONE *prEvent)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);

#if CFG_RTT_TEST_MODE
	rttFakeEvent(prAdapter);
#endif

	if (prEvent == NULL) { /* timeout case */
		DBGLOG(RTT, WARN, "rttRequestDoneTimeOut Seq=%u\n",
		       rttInfo->ucSeqNum);
		rttFreeAllResults(rttInfo);
		rttCancelRttRequest(prAdapter, NULL);
	} else { /* normal rtt done */
		DBGLOG(RTT, INFO,
		       "Event RTT done seq: FW %u, driver %u\n",
		       prEvent->ucSeqNum, rttInfo->ucSeqNum);
		if (prEvent->ucSeqNum == rttInfo->ucSeqNum)
			rttUpdateStatus(prAdapter, NULL);
		else
			return;
	}
	rttReportDone(prAdapter);
}

void rttEventResult(struct ADAPTER *prAdapter,
		      struct EVENT_RTT_RESULT *prEvent)
{
	struct RTT_INFO *rttInfo = rttGetInfo(prAdapter);
	struct RTT_RESULT_ENTRY *entry;
	uint32_t sz = sizeof(struct RTT_RESULT_ENTRY);

	if (!rttInfo->fgIsRunning) {
		DBGLOG(RTT, WARN, "RTT is not running\n");
		return;
	} else if (!prEvent) {
		DBGLOG(RTT, ERROR, "RTT null result\n");
		return;
	}

	DBGLOG(RTT, INFO, "RTT result MAC=" MACSTR " status=%d\n",
		MAC2STR(prEvent->rResult.aucMacAddr), prEvent->rResult.eStatus);
	entry = kalMemAlloc(sz + prEvent->u2IELen, VIR_MEM_TYPE);
	if (entry) {
		kalMemCopy(&entry->rResult, &prEvent->rResult,
			sizeof(struct RTT_RESULT));
		entry->u2IELen = prEvent->u2IELen;
		kalMemCopy(entry->aucIE, prEvent->aucIE, prEvent->u2IELen);
		LINK_INSERT_TAIL(&rttInfo->rResultList,
			&entry->rLinkEntry);
	}
}

#if 0
void rttFreeResult(struct RTT_INFO *prRttInfo, uint8_t aucBSSID[])
{
	struct RTT_RESULT_ENTRY *entry;
	struct RTT_RESULT_ENTRY *entryNext;

	LINK_FOR_EACH_ENTRY_SAFE(entry, entryNext,
		&prRttInfo->rResultList, rLinkEntry, struct RTT_RESULT_ENTRY) {

		if (EQUAL_MAC_ADDR(entry->rResult.aucMacAddr, aucBSSID)) {
			uint32_t sz = sizeof(struct RTT_RESULT_ENTRY) +
				      entry->rResult.u2IELen;

			LINK_REMOVE_KNOWN_ENTRY(&prRttInfo->rResultList),
				&entry->rLinkEntry);
			kalMemFree(entry, VIR_MEM_TYPE, sz);
		}
	}
}

void rttFreeAllRequests(struct RTT_INFO *prRttInfo)
{
	kalMemFree(prRttInfo->prCmd,
		VIR_MEM_TYPE, sizeof(struct CMD_RTT_REQUEST));
	prRttInfo->prCmd = NULL;
}

void rttFreeRequest(struct RTT_INFO *prRttInfo, uint8_t aucBSSID[])
{
	struct CMD_RTT_REQUEST *cmd = prRttInfo->prCmd;
	uint8_t i;

	if (cmd) {
		for (i = 0; i < cmd->ucConfigNum; i++) {
			uint8_t *bssid = cmd->arRttConfigs[i].aucAddr;

			if (EQUAL_MAC_ADDR(bssid, aucBSSID))
				break;
		}
		if (i < cmd->ucConfigNum) {
			kalMemCopy(&cmd->arRttConfigs[i],
				&cmd->arRttConfigs[i + 1],
				(cmd->ucConfigNum - i - 1) *
					sizeof(struct RTT_CONFIG));
			cmd->ucConfigNum--;
		}
	}
}
#endif

