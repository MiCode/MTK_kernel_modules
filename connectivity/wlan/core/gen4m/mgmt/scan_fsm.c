// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "scan_fsm.c"
 *    \brief  This file defines the state transition function for SCAN FSM.
 *
 *    The SCAN FSM is part of SCAN MODULE and responsible for performing basic
 *    SCAN behavior as metioned in IEEE 802.11 2007 11.1.3.1 & 11.1.3.2.
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
static const char * const apucDebugScanState[SCAN_STATE_NUM] = {
	"IDLE",
	"SCANNING",
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#if (CFG_SUPPORT_WIFI_6G == 1)
#define SCN_GET_EBAND_BY_CH_NUM(_ucChNum) \
	((_ucChNum <= HW_CHNL_NUM_MAX_2G4) ? BAND_2G4 : \
	(_ucChNum > HW_CHNL_NUM_MAX_5G) ? BAND_6G : \
	BAND_5G)
#else
#define SCN_GET_EBAND_BY_CH_NUM(_ucChNum) \
	((_ucChNum <= HW_CHNL_NUM_MAX_2G4) ? BAND_2G4 :	BAND_5G)
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void scnFsmSteps(struct ADAPTER *prAdapter,
	enum ENUM_SCAN_STATE eNextState)
{
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	struct MSG_HDR *prMsgHdr;

	u_int8_t fgIsTransition = (u_int8_t) FALSE;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	do {
		if ((uint32_t)prScanInfo->eCurrentState < SCAN_STATE_NUM &&
			(uint32_t)eNextState < SCAN_STATE_NUM) {
			log_dbg(SCN, STATE, "[SCAN]TRANSITION: [%s] -> [%s]\n",
			apucDebugScanState[
				(uint32_t) prScanInfo->eCurrentState],
			apucDebugScanState[
				(uint32_t) eNextState]);
		}
		/* NOTE(Kevin): This is the only place to change the
		 * eCurrentState(except initial)
		 */
		prScanInfo->eCurrentState = eNextState;

		fgIsTransition = (u_int8_t) FALSE;

		switch (prScanInfo->eCurrentState) {
		case SCAN_STATE_IDLE:
			prScanParam->fgOobRnrParseEn = FALSE;
			/* check for pending scanning requests */
			if (!LINK_IS_EMPTY(&(prScanInfo->rPendingMsgList))) {
				/* load next message from pending list as
				 * scan parameters
				 */
				LINK_REMOVE_HEAD(&(prScanInfo->rPendingMsgList),
					prMsgHdr, struct MSG_HDR *);

#define __MSG_ID__ prMsgHdr->eMsgId
				if (__MSG_ID__ == MID_AIS_SCN_SCAN_REQ
					|| __MSG_ID__ == MID_P2P_SCN_SCAN_REQ
					|| __MSG_ID__ == MID_RLM_SCN_SCAN_REQ) {
					scnFsmHandleScanMsg(prAdapter,
						(struct MSG_SCN_SCAN_REQ *)
						 prMsgHdr);

					eNextState = SCAN_STATE_SCANNING;
					fgIsTransition = TRUE;
				} else if (__MSG_ID__ == MID_AIS_SCN_SCAN_REQ_V2
					|| __MSG_ID__ == MID_P2P_SCN_SCAN_REQ_V2
					|| __MSG_ID__ == MID_RLM_SCN_SCAN_REQ_V2
					) {
					scnFsmHandleScanMsgV2(prAdapter,
						(struct MSG_SCN_SCAN_REQ_V2 *)
						 prMsgHdr);

					eNextState = SCAN_STATE_SCANNING;
					fgIsTransition = TRUE;
				} else {
					/* should not happen */
					ASSERT(0);
				}
#undef __MSG_ID__

				/* switch to next state */
				cnmMemFree(prAdapter, prMsgHdr);
			}
			break;

		case SCAN_STATE_SCANNING:
			if (scnDoAllowScanCheck(prAdapter)) {
				/* Support AP Selection */
				prScanInfo->u4ScanUpdateIdx++;
				if (prScanParam->fgIsScanV2 == FALSE)
					scnSendScanReq(prAdapter);
				else
					scnSendScanReqV2(prAdapter);
			} else {
				scnFsmGenerateScanDoneMsg(prAdapter,
					prScanParam->eMsgId,
					prScanParam->ucSeqNum,
					prScanParam->ucBssIndex,
					SCAN_STATUS_CANCELLED,
					MSG_SEND_METHOD_BUF);

				/* switch to next pending scan */
				eNextState = SCAN_STATE_IDLE;
				fgIsTransition = TRUE;
			}
			break;

		default:
			ASSERT(0);
			break;

		}
	} while (fgIsTransition);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief        Generate CMD_ID_SCAN_REQ command
 *
 * Because CMD_ID_SCAN_REQ is deprecated,
 * wrap this command to CMD_ID_SCAN_REQ_V2
 *
 * \param[in] prAdapter   adapter
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void scnSendScanReq(struct ADAPTER *prAdapter)
{
	log_dbg(SCN, WARN,
		"CMD_ID_SCAN_REQ is deprecated, use CMD_ID_SCAN_REQ_V2\n");
	scnSendScanReqV2(prAdapter);
}

void scanAddPerBandIE(struct ADAPTER *prAdapter,
	struct SCAN_PARAM *prScanParam,
	struct CMD_SCAN_REQ_V2 *prCmdScanReq)
{
#if defined(CFG_SUPPORT_UNIFIED_COMMAND) && (CFG_SUPPORT_802_11BE_MLO == 1)
	uint16_t len = prScanParam->u2IELen;

	len += prScanParam->u2IELenMl;
	len += prScanParam->u2IELen2G4 + prScanParam->u2IELen5G;
#if (CFG_SUPPORT_WIFI_6G == 1)
	len += prScanParam->u2IELen6G;
#endif

	/* FW share scan IE with per band IE, so ensure space is enough here */
	if (len > MAX_IE_LENGTH) {
		log_dbg(SCN, WARN, "no space for per band IE (len=%d)\n", len);
		return;
	}

	if (prScanParam->u2IELenMl > 0 &&
	    prScanParam->u2IELenMl <= MAX_BAND_IE_LENGTH) {
		prCmdScanReq->u2IELenMl = prScanParam->u2IELenMl;
		kalMemCopy(prCmdScanReq->aucIEMl,
			prScanParam->aucIEMl, prScanParam->u2IELenMl);
	}
	if (prScanParam->u2IELen2G4 > 0 &&
	    prScanParam->u2IELen2G4 <= MAX_BAND_IE_LENGTH) {
		prCmdScanReq->u2IELen2G4 = prScanParam->u2IELen2G4;
		kalMemCopy(prCmdScanReq->aucIE2G4,
			prScanParam->aucIE2G4, prScanParam->u2IELen2G4);
	}
	if (prScanParam->u2IELen5G > 0 &&
	    prScanParam->u2IELen5G <= MAX_BAND_IE_LENGTH) {
		prCmdScanReq->u2IELen5G = prScanParam->u2IELen5G;
		kalMemCopy(prCmdScanReq->aucIE5G,
			prScanParam->aucIE5G, prScanParam->u2IELen5G);
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prScanParam->u2IELen6G > 0 &&
	    prScanParam->u2IELen6G <= MAX_BAND_IE_LENGTH) {
		prCmdScanReq->u2IELen6G = prScanParam->u2IELen6G;
		kalMemCopy(prCmdScanReq->aucIE6G,
			prScanParam->aucIE6G, prScanParam->u2IELen6G);
	}
#endif
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief        Generate CMD_ID_SCAN_REQ_V2 command
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void scnSendScanReqV2(struct ADAPTER *prAdapter)
{
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	/* CMD_SCAN_REQ_V2 rCmdScanReq; */
	struct CMD_SCAN_REQ_V2 *prCmdScanReq;
	uint32_t i, j;
	uint8_t ucChannel;
	uint8_t ucBand;
	struct CHANNEL_INFO *prCnlInfo;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &(prScanInfo->rScanParam);

	prCmdScanReq = kalMemAlloc(
		sizeof(struct CMD_SCAN_REQ_V2), VIR_MEM_TYPE);
	if (!prCmdScanReq) {
		log_dbg(SCN, ERROR, "alloc CmdScanReq V2 fail\n");
		return;
	}

#if CFG_SUPPORT_SCAN_LOG
	scanAbortBeaconRecv(prAdapter,
		prScanParam->ucBssIndex,
		ABORT_SCAN_STARTS);
#endif

	/* send command packet for scan */
	kalMemZero(prCmdScanReq, sizeof(struct CMD_SCAN_REQ_V2));
	/* Modify channelList number from 32 to 54 */
	if (prScanParam->ucScnFuncMask & ENUM_SCN_USE_PADDING_AS_BSSID ||
		prScanParam->u4ScnFuncMaskExtend & ENUM_SCN_ML_PROBE) {
		kalMemCopy(prCmdScanReq->aucExtBSSID,
			prScanParam->aucBSSID,
			CFG_SCAN_OOB_MAX_NUM * MAC_ADDR_LEN);
	} else {
		COPY_MAC_ADDR(prCmdScanReq->aucBSSID,
		&prScanParam->aucBSSID[0][0]);
	}
	if (!EQUAL_MAC_ADDR(prCmdScanReq->aucBSSID, "\x00\x00\x00\x00\x00\x00"))
		DBGLOG(SCN, INFO, "Include BSSID "MACSTR" in probe request\n",
			MAC2STR(prCmdScanReq->aucBSSID));

	prCmdScanReq->ucSeqNum = prScanParam->ucSeqNum;
	prCmdScanReq->ucBssIndex = prScanParam->ucBssIndex;
	prCmdScanReq->ucScanType = (uint8_t) prScanParam->eScanType;
	prCmdScanReq->ucSSIDType = prScanParam->ucSSIDType;
	prCmdScanReq->auVersion[0] = 1;
	prCmdScanReq->ucScnFuncMask |= prScanParam->ucScnFuncMask;
	prCmdScanReq->u4ScnFuncMaskExtend |= prScanParam->u4ScnFuncMaskExtend;
	prCmdScanReq->ucScnSourceMask = ENUM_SCN_NORMAL;

#if (CONFIG_BAND_NUM > 2)
	if (scnCheckIsOnlyAbandP2PConnected(prAdapter))
		prCmdScanReq->u4ScnFuncMaskExtend |= ENUM_SCN_SILENT_SCAN;
#endif

	/* for 6G OOB scan */
	kalMemCopy(prCmdScanReq->ucBssidMatchCh, prScanParam->ucBssidMatchCh,
			CFG_SCAN_OOB_MAX_NUM);
	kalMemCopy(prCmdScanReq->ucBssidMatchSsidInd,
		prScanParam->ucBssidMatchSsidInd, CFG_SCAN_OOB_MAX_NUM);

	if (kalIsValidMacAddr(prScanParam->aucRandomMac)) {
		prCmdScanReq->ucScnFuncMask |= (ENUM_SCN_RANDOM_MAC_EN |
						ENUM_SCN_RANDOM_SN_EN);
		kalMemCopy(prCmdScanReq->aucRandomMac,
			prScanParam->aucRandomMac, MAC_ADDR_LEN);
	}
	if (prAdapter->rWifiVar.eDbdcMode == ENUM_DBDC_MODE_DISABLED
#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
		|| (prAdapter->rWifiVar.eDbdcMode == ENUM_DBDC_MODE_DYNAMIC &&
		    prAdapter->fgPowerForceOneNss)
#endif
		)
		prCmdScanReq->ucScnFuncMask |= ENUM_SCN_DBDC_SCAN_DIS;

	/* Set SSID to scan request */
	if (prScanParam->ucSSIDNum <= SCAN_CMD_SSID_NUM) {
		prCmdScanReq->ucSSIDNum = prScanParam->ucSSIDNum;
		prCmdScanReq->ucSSIDExtNum = 0;
	} else if (prScanParam->ucSSIDNum <= CFG_SCAN_SSID_MAX_NUM) {
		prCmdScanReq->ucSSIDNum = SCAN_CMD_SSID_NUM;
		prCmdScanReq->ucSSIDExtNum = prScanParam->ucSSIDNum
			- SCAN_CMD_SSID_NUM;
	} else {
		log_dbg(SCN, WARN, "Too many SSID %u\n",
					prScanParam->ucSSIDNum);
		prCmdScanReq->ucSSIDNum = SCAN_CMD_SSID_NUM;
		prCmdScanReq->ucSSIDExtNum = SCAN_CMD_EXT_SSID_NUM;
	}

	for (i = 0; i < prCmdScanReq->ucSSIDNum; i++) {
		COPY_SSID(prCmdScanReq->arSSID[i].aucSsid,
			prCmdScanReq->arSSID[i].u4SsidLen,
			prScanParam->aucSpecifiedSSID[i],
			prScanParam->ucSpecifiedSSIDLen[i]);
		log_dbg(SCN, TRACE,
			"Ssid=%s, SsidLen=%d\n",
			HIDE(prCmdScanReq->arSSID[i].aucSsid),
			prCmdScanReq->arSSID[i].u4SsidLen);
	}
	for (i = 0; i < prCmdScanReq->ucSSIDExtNum; i++) {
		COPY_SSID(prCmdScanReq->arSSIDExtend[i].aucSsid,
			prCmdScanReq->arSSIDExtend[i].u4SsidLen,
			prScanParam->aucSpecifiedSSID
			[prCmdScanReq->ucSSIDNum+i],
			prScanParam->ucSpecifiedSSIDLen
			[prCmdScanReq->ucSSIDNum+i]);
		log_dbg(SCN, TRACE,
			"Ssid=%s, SsidLen=%d\n",
			prCmdScanReq->arSSIDExtend[i].aucSsid,
			prCmdScanReq->arSSIDExtend[i].u4SsidLen);
	}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	/* Set Short SSID to scan request */
	kalMemCopy(prCmdScanReq->ucBssidMatchShortSsidInd,
		prScanParam->ucBssidMatchShortSsidInd, CFG_SCAN_OOB_MAX_NUM);

	prCmdScanReq->ucShortSSIDNum = prScanParam->ucShortSSIDNum;
	for (i = 0; i < prCmdScanReq->ucShortSSIDNum; i++) {
		kalMemCopy(prCmdScanReq->aucShortSSID[i],
			prScanParam->aucShortSSID[i],
			MAX_SHORT_SSID_LEN);
	}
#endif

	prCmdScanReq->u2ProbeDelayTime
		= (uint8_t) prScanParam->u2ProbeDelayTime;
	prCmdScanReq->ucChannelType
		= (uint8_t) prScanParam->eScanChannel;

	/* Set channel info to scan request */
	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		for (i = 0, j = 0; i < prScanParam->ucChannelListNum; i++) {
			ucBand = (uint8_t) prScanParam->arChnlInfoList[i].eBand;
			ucChannel = prScanParam->arChnlInfoList[i].ucChannelNum;
			/* remove DFS channel */
			if (prAdapter->rWifiVar.rScanInfo.fgSkipDFS &&
				rlmDomainIsDfsChnls(prAdapter, ucChannel))
				continue;

			if (j < SCAN_CMD_CHNL_NUM) {
				prCnlInfo = &(prCmdScanReq->arChannelList[j]);
				prCnlInfo->ucBand = ucBand;
				prCnlInfo->ucChannelNum = ucChannel;
				prCmdScanReq->ucChannelListNum = ++j;
			} else if (j < MAXIMUM_OPERATION_CHANNEL_LIST) {
				prCnlInfo = &(prCmdScanReq->
				   arChannelListExtend[j - SCAN_CMD_CHNL_NUM]);
				prCnlInfo->ucBand = ucBand;
				prCnlInfo->ucChannelNum = ucChannel;
				prCmdScanReq->ucChannelListExtNum =
					(++j - SCAN_CMD_CHNL_NUM);
			} else {
				log_dbg(SCN, WARN, "Too many Channel %u\n",
					prScanParam->ucChannelListNum);
				prCmdScanReq->ucChannelListNum = 0;
				prCmdScanReq->ucChannelListExtNum = 0;
				prCmdScanReq->ucChannelType = SCAN_CHANNEL_FULL;
				break;
			}
		}
	}

	prCmdScanReq->u2ChannelDwellTime = prScanParam->u2ChannelDwellTime;
	prCmdScanReq->u2ChannelMinDwellTime =
		prScanParam->u2ChannelMinDwellTime;
	prCmdScanReq->u2TimeoutValue = prScanParam->u2TimeoutValue;

#if CFG_SUPPORT_LLW_SCAN

	if (prScanParam->u2LLWChannelDwellTime != 0) {
		prCmdScanReq->u2ChannelDwellTime =
			prScanParam->u2LLWChannelDwellTime;
		prCmdScanReq->u2ChannelMinDwellTime =
			prScanParam->u2LLWChannelMinDwellTime;
	}
	prCmdScanReq->u2OpChStayTimeMs = prScanParam->u2OpChStayTime;
	prCmdScanReq->ucDfsChDwellTimeMs = prScanParam->ucDfsChDwellTime;
	prCmdScanReq->ucPerScanChannelCnt = prScanParam->ucPerScanChCnt;
#endif

	/* If ProbeDelayTime bigger than MinDwellTime,
	 * reset ProbeDelayTime to 0
	 */
	if (prCmdScanReq->u2ProbeDelayTime >
	    prCmdScanReq->u2ChannelMinDwellTime)
		prCmdScanReq->u2ProbeDelayTime = 0;

	/* OCE certification handling */
	scanHandleOceIE(prScanParam, prCmdScanReq);

	/* enable split scan when (not in roam) && (WFD || 1s TRX pkt > 30) */
	if (scnEnableSplitScan(prAdapter, prScanParam->ucBssIndex,
				prCmdScanReq)) {
		prCmdScanReq->ucScnFuncMask |= ENUM_SCN_SPLIT_SCAN_EN;
		/* if WFD enable, not do dbdc scan and reduce dwell time to
		 * enhance latency
		 */
		if (wlanWfdEnabled(prAdapter)) {
			prCmdScanReq->ucScnFuncMask |= ENUM_SCN_DBDC_SCAN_DIS;
			prCmdScanReq->u2ChannelDwellTime =
				SCAN_CHANNEL_DWELL_TIME_MIN_MSEC;
			prCmdScanReq->u2ChannelMinDwellTime =
				SCAN_CHANNEL_DWELL_TIME_MIN_MSEC;
		}
	}

	if (prScanParam->u2IELen <= MAX_IE_LENGTH)
		prCmdScanReq->u2IELen = prScanParam->u2IELen;
	else
		prCmdScanReq->u2IELen = MAX_IE_LENGTH;

	if (prCmdScanReq->u2IELen)
		kalMemCopy(prCmdScanReq->aucIE, prScanParam->aucIE,
			sizeof(uint8_t) * prCmdScanReq->u2IELen);

	scanAddPerBandIE(prAdapter, prScanParam, prCmdScanReq);

	log_dbg(SCN, TRACE, "ScanReqV2: used[%d]free[%d] ScanType=%d,BSS=%u,SSIDType=%d,Num=%u,Ext=%u,ChannelType=%d,Num=%d,Ext=%u,Seq=%u,Ver=%u,Dw=%u,Min=%u,IELen=%d,Func=(0x%X,0x%X),Mac="
		MACSTR ",BSSID:"MACSTR"\n",
		prScanInfo->rBSSDescList.u4NumElem,
		prScanInfo->rFreeBSSDescList.u4NumElem,
		prCmdScanReq->ucScanType,
		prCmdScanReq->ucBssIndex,
		prCmdScanReq->ucSSIDType,
		prCmdScanReq->ucSSIDNum,
		prCmdScanReq->ucSSIDExtNum,
		prCmdScanReq->ucChannelType,
		prCmdScanReq->ucChannelListNum,
		prCmdScanReq->ucChannelListExtNum,
		prCmdScanReq->ucSeqNum, prCmdScanReq->auVersion[0],
		prCmdScanReq->u2ChannelDwellTime,
		prCmdScanReq->u2ChannelMinDwellTime,
		prCmdScanReq->u2IELen,
		prCmdScanReq->ucScnFuncMask,
		prCmdScanReq->u4ScnFuncMaskExtend,
		MAC2STR(prCmdScanReq->aucRandomMac),
		MAC2STR(prCmdScanReq->aucBSSID));

	scanLogCacheFlushAll(prAdapter, &(prScanInfo->rScanLogCache),
		LOG_SCAN_REQ_D2F);
	scanReqLog(prCmdScanReq);

	wlanSendSetQueryCmd(prAdapter,
		CMD_ID_SCAN_REQ_V2,
		TRUE,
		FALSE,
		FALSE,
		NULL,
		NULL,
		sizeof(struct CMD_SCAN_REQ_V2),
		(uint8_t *)prCmdScanReq, NULL, 0);
	log_dbg(SCN, TRACE, "Send %zu bytes\n", sizeof(struct CMD_SCAN_REQ_V2));

	GET_CURRENT_SYSTIME(&prScanInfo->rLastScanStartTime);

	kalMemFree(prCmdScanReq, VIR_MEM_TYPE, sizeof(struct CMD_SCAN_REQ_V2));

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void scnFsmMsgStart(struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;

	ASSERT(prMsgHdr);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	if (prScanInfo->eCurrentState == SCAN_STATE_IDLE) {
		if (prMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ
			|| prMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ
			|| prMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ) {
			scnFsmHandleScanMsg(prAdapter,
				(struct MSG_SCN_SCAN_REQ *) prMsgHdr);
		} else if (prMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ_V2
			|| prMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ_V2
			|| prMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ_V2) {
			scnFsmHandleScanMsgV2(prAdapter,
				(struct MSG_SCN_SCAN_REQ_V2 *) prMsgHdr);
		} else {
			/* should not deliver to this function */
			ASSERT(0);
		}

		cnmMemFree(prAdapter, prMsgHdr);
		scnFsmSteps(prAdapter, SCAN_STATE_SCANNING);
	} else {
		LINK_INSERT_TAIL(&prScanInfo->rPendingMsgList,
			&prMsgHdr->rLinkEntry);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void scnFsmMsgAbort(struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct MSG_SCN_SCAN_CANCEL *prScanCancel;
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	struct CMD_SCAN_CANCEL rCmdScanCancel;
	ASSERT(prMsgHdr);

	prScanCancel = (struct MSG_SCN_SCAN_CANCEL *) prMsgHdr;
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;
	kalMemZero(&rCmdScanCancel, sizeof(rCmdScanCancel));

	if (prScanInfo->eCurrentState != SCAN_STATE_IDLE) {
#if (CFG_SUPPORT_WIFI_RNR == 1)
		while (!LINK_IS_EMPTY(&prScanInfo->rNeighborAPInfoList)) {
			struct NEIGHBOR_AP_INFO *prNeighborAPInfo;

			LINK_REMOVE_HEAD(&prScanInfo->rNeighborAPInfoList,
				prNeighborAPInfo, struct NEIGHBOR_AP_INFO *);

			cnmMemFree(prAdapter, prNeighborAPInfo);
		}
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		while (!LINK_IS_EMPTY(&prScanInfo->rMldAPInfoList)) {
			struct MLD_AP_INFO *prMldAPInfo;

			LINK_REMOVE_HEAD(&prScanInfo->rMldAPInfoList,
				prMldAPInfo, struct MLD_AP_INFO *);
			cnmMemFree(prAdapter, prMldAPInfo);
		}
#endif

		if (prScanCancel->ucSeqNum == prScanParam->ucSeqNum &&
			prScanCancel->ucBssIndex == prScanParam->ucBssIndex) {
			enum ENUM_SCAN_STATUS eStatus = SCAN_STATUS_DONE;

			/* send cancel message to firmware domain */
			rCmdScanCancel.ucSeqNum = prScanParam->ucSeqNum;
			rCmdScanCancel.ucIsExtChannel
				= (uint8_t) prScanCancel->fgIsChannelExt;

			scanlog_dbg(LOG_SCAN_ABORT_REQ_D2F, INFO, "Scan Abort#%u to Q: isExtCh=%u",
				rCmdScanCancel.ucSeqNum,
				rCmdScanCancel.ucIsExtChannel);
#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			wlanSendSetQueryCmdHelper(
#else
			wlanSendSetQueryCmdAdv(
#endif
				prAdapter, CMD_ID_SCAN_CANCEL,
				0, TRUE, FALSE, FALSE, NULL, NULL,
				sizeof(struct CMD_SCAN_CANCEL),
				(uint8_t *) &rCmdScanCancel, NULL, 0,
				CMD_SEND_METHOD_REQ_RESOURCE);
#else
			wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SCAN_CANCEL,
				TRUE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				sizeof(struct CMD_SCAN_CANCEL),
				(uint8_t *) &rCmdScanCancel,
				NULL,
				0);
#endif /* CFG_MLO_CONCURRENT_SINGLE_PHY == 1 */

			/* Full2Partial: ignore this statistics */
			if (prScanInfo->fgIsScanForFull2Partial) {
				prScanInfo->fgIsScanForFull2Partial = FALSE;
				prScanInfo->u4LastFullScanTime = 0;
				log_dbg(SCN, INFO,
					"Full2Partial: scan canceled(%u)\n",
					prScanParam->ucSeqNum);
			}

			/* generate scan-done event for caller */
			if (prScanCancel->fgIsOidRequest)
				eStatus = SCAN_STATUS_CANCELLED;
			else
				eStatus = SCAN_STATUS_DONE;
			scnFsmGenerateScanDoneMsg(prAdapter,
				prScanParam->eMsgId,
				prScanParam->ucSeqNum,
				prScanParam->ucBssIndex,
				eStatus,
				MSG_SEND_METHOD_UNBUF);

			/* switch to next pending scan */
			scnFsmSteps(prAdapter, SCAN_STATE_IDLE);
		} else {
			scnFsmRemovePendingMsg(prAdapter,
				prScanCancel->ucSeqNum,
				prScanCancel->ucBssIndex);
		}
	} else {
		scnFsmRemovePendingMsg(prAdapter,
			prScanCancel->ucSeqNum,
			prScanCancel->ucBssIndex);
	}

	cnmMemFree(prAdapter, prMsgHdr);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Scan Message Parsing (Legacy)
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void scnFsmHandleScanMsg(struct ADAPTER *prAdapter,
	struct MSG_SCN_SCAN_REQ *prScanReqMsg)
{
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	uint32_t i;

	ASSERT(prAdapter);
	ASSERT(prScanReqMsg);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	kalMemZero(prScanParam, sizeof(*prScanParam));
	prScanParam->eScanType = prScanReqMsg->eScanType;
	prScanParam->ucBssIndex = prScanReqMsg->ucBssIndex;
	prScanParam->ucSSIDType = prScanReqMsg->ucSSIDType;
	if (prScanParam->ucSSIDType
		& (SCAN_REQ_SSID_SPECIFIED | SCAN_REQ_SSID_P2P_WILDCARD)) {
		prScanParam->ucSSIDNum = 1;

		COPY_SSID(prScanParam->aucSpecifiedSSID[0],
			prScanParam->ucSpecifiedSSIDLen[0],
			prScanReqMsg->aucSSID, prScanReqMsg->ucSSIDLength);

		/* reset SSID length to zero for rest array entries */
		for (i = 1; i < CFG_SCAN_SSID_MAX_NUM; i++)
			prScanParam->ucSpecifiedSSIDLen[i] = 0;
	} else {
		prScanParam->ucSSIDNum = 0;

		for (i = 0; i < CFG_SCAN_SSID_MAX_NUM; i++)
			prScanParam->ucSpecifiedSSIDLen[i] = 0;
	}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	/* No BssidMatchShortSsid, set to default value */
	prScanParam->ucShortSSIDNum = 0;
	kalMemSet(prScanParam->ucBssidMatchShortSsidInd,
		CFG_SCAN_OOB_MAX_NUM,
		sizeof(prScanParam->ucBssidMatchShortSsidInd));
#endif

	prScanParam->u2ProbeDelayTime = 0;
	prScanParam->eScanChannel = prScanReqMsg->eScanChannel;
	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		if (prScanReqMsg->ucChannelListNum
			<= MAXIMUM_OPERATION_CHANNEL_LIST) {
			prScanParam->ucChannelListNum
				= prScanReqMsg->ucChannelListNum;
		} else {
			prScanParam->ucChannelListNum
				= MAXIMUM_OPERATION_CHANNEL_LIST;
		}

		kalMemCopy(prScanParam->arChnlInfoList,
			prScanReqMsg->arChnlInfoList,
			sizeof(struct RF_CHANNEL_INFO)
				* prScanParam->ucChannelListNum);
	}

	if (prScanReqMsg->u2IELen <= MAX_IE_LENGTH)
		prScanParam->u2IELen = prScanReqMsg->u2IELen;
	else
		prScanParam->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen) {
		kalMemCopy(prScanParam->aucIE,
			prScanReqMsg->aucIE, prScanParam->u2IELen);
	}

	prScanParam->u2ChannelDwellTime = prScanReqMsg->u2ChannelDwellTime;
	prScanParam->u2TimeoutValue = prScanReqMsg->u2TimeoutValue;
	prScanParam->ucSeqNum = prScanReqMsg->ucSeqNum;
	prScanParam->eMsgId = prScanReqMsg->rMsgHdr.eMsgId;
	prScanParam->fgIsScanV2 = FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Scan Message Parsing - V2 with multiple SSID support
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void scnFsmHandleScanMsgV2(struct ADAPTER *prAdapter,
	struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg)
{
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	uint32_t i;
#if CFG_MTK_FPGA_PLATFORM
	uint8_t ch_list[] = {1, 6, 11,
			  36};
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ch_list_6g[] = {37, 1, 31};
#endif
#endif /* CFG_MTK_FPGA_PLATFORM */

	ASSERT(prAdapter);
	ASSERT(prScanReqMsg);
	ASSERT(prScanReqMsg->ucSSIDNum <= CFG_SCAN_SSID_MAX_NUM);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	kalMemZero(prScanParam, sizeof(*prScanParam));
	prScanParam->eScanType = prScanReqMsg->eScanType;
	prScanParam->ucBssIndex = prScanReqMsg->ucBssIndex;
	prScanParam->ucSSIDType = prScanReqMsg->ucSSIDType;
	prScanParam->ucSSIDNum = prScanReqMsg->ucSSIDNum;
	prScanParam->ucScnFuncMask |= prScanReqMsg->ucScnFuncMask;
	prScanParam->u4ScnFuncMaskExtend |= prScanReqMsg->u4ScnFuncMaskExtend;

	kalMemCopy(prScanParam->aucRandomMac, prScanReqMsg->aucRandomMac,
		MAC_ADDR_LEN);
	/* for 6G OOB scan */
	kalMemCopy(prScanParam->ucBssidMatchCh, prScanReqMsg->ucBssidMatchCh,
			CFG_SCAN_OOB_MAX_NUM);
	kalMemCopy(prScanParam->ucBssidMatchSsidInd,
		prScanReqMsg->ucBssidMatchSsidInd, CFG_SCAN_OOB_MAX_NUM);
	prScanParam->fgOobRnrParseEn = prScanReqMsg->fgOobRnrParseEn;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	/* No BssidMatchShortSsid, set to default value */
	prScanParam->ucShortSSIDNum = 0;
	kalMemSet(prScanParam->ucBssidMatchShortSsidInd,
		CFG_SCAN_OOB_MAX_NUM,
		sizeof(prScanParam->ucBssidMatchShortSsidInd));
#endif

	if ((prScanParam->ucSSIDType & SCAN_REQ_SSID_SPECIFIED_ONLY) &&
		((prScanReqMsg->ucScnFuncMask &
		ENUM_SCN_USE_PADDING_AS_BSSID) == 0)) {
		prScanParam->ucSSIDNum = 1;
		kalMemZero(prScanParam->ucSpecifiedSSIDLen,
			   sizeof(prScanParam->ucSpecifiedSSIDLen));
		COPY_SSID(prScanParam->aucSpecifiedSSID[0],
			  prScanParam->ucSpecifiedSSIDLen[0],
			  &prScanReqMsg->arSsid[0].aucSsid[0],
			  prScanReqMsg->arSsid[0].u4SsidLen);
	} else {
		for (i = 0; i < prScanReqMsg->ucSSIDNum &&
			    i < CFG_SCAN_SSID_MAX_NUM; i++) {
			COPY_SSID(prScanParam->aucSpecifiedSSID[i],
				  prScanParam->ucSpecifiedSSIDLen[i],
				  prScanReqMsg->arSsid[i].aucSsid,
				  (uint8_t) prScanReqMsg->arSsid[i].u4SsidLen);
		}
	}

	prScanParam->u2ProbeDelayTime = prScanReqMsg->u2ProbeDelay;
	prScanParam->eScanChannel = prScanReqMsg->eScanChannel;
	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		if (prScanReqMsg->ucChannelListNum
			<= MAXIMUM_OPERATION_CHANNEL_LIST) {
			prScanParam->ucChannelListNum
				= prScanReqMsg->ucChannelListNum;
		} else {
			prScanParam->ucChannelListNum
				= MAXIMUM_OPERATION_CHANNEL_LIST;
		}

		kalMemCopy(prScanParam->arChnlInfoList,
			prScanReqMsg->arChnlInfoList,
			sizeof(struct RF_CHANNEL_INFO)
				* prScanParam->ucChannelListNum);
	}

	if (prScanReqMsg->u2IELen <= MAX_IE_LENGTH)
		prScanParam->u2IELen = prScanReqMsg->u2IELen;
	else
		prScanParam->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen) {
		kalMemCopy(prScanParam->aucIE,
			prScanReqMsg->aucIE, prScanParam->u2IELen);
	}

	prScanParam->u2ChannelDwellTime = prScanReqMsg->u2ChannelDwellTime;
	prScanParam->u2ChannelMinDwellTime =
		prScanReqMsg->u2ChannelMinDwellTime;
	prScanParam->u2TimeoutValue = prScanReqMsg->u2TimeoutValue;
#if CFG_SUPPORT_LLW_SCAN
	prScanParam->u2OpChStayTime = prScanReqMsg->u2OpChStayTime;
	prScanParam->ucDfsChDwellTime = prScanReqMsg->ucDfsChDwellTime;
	prScanParam->ucPerScanChCnt = prScanReqMsg->ucPerScanChCnt;
	prScanParam->u2LLWChannelDwellTime =
		prScanReqMsg->u2LLWChannelDwellTime;
	prScanParam->u2LLWChannelMinDwellTime =
		prScanReqMsg->u2LLWChannelMinDwellTime;
#endif
	prScanParam->ucSeqNum = prScanReqMsg->ucSeqNum;
	prScanParam->eMsgId = prScanReqMsg->rMsgHdr.eMsgId;
	prScanParam->fgIsScanV2 = TRUE;

	kalMemCopy(prScanParam->aucBSSID,
		prScanReqMsg->aucExtBssid,
		CFG_SCAN_OOB_MAX_NUM * MAC_ADDR_LEN);

#if CFG_MTK_FPGA_PLATFORM
	prScanParam->ucChannelListNum = ARRAY_SIZE(ch_list);
	for (i = 0; i < ARRAY_SIZE(ch_list); i++) {
		prScanParam->arChnlInfoList[i].ucChannelNum
			= ch_list[i];
		prScanParam->arChnlInfoList[i].eBand = ch_list[i] <=
			HW_CHNL_NUM_MAX_2G4 ? BAND_2G4 : BAND_5G;
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	prScanParam->ucChannelListNum += ARRAY_SIZE(ch_list_6g);
	for (i = 0; i < ARRAY_SIZE(ch_list_6g); i++) {
		prScanParam->arChnlInfoList[i +
			ARRAY_SIZE(ch_list)].ucChannelNum = ch_list_6g[i];
		prScanParam->arChnlInfoList[i +
			ARRAY_SIZE(ch_list)].eBand = BAND_6G;
	}
#endif
	prScanParam->eScanChannel = SCAN_CHANNEL_SPECIFIED;
#endif /* CFG_MTK_FPGA_PLATFORM */

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prScanParam->fgCollectMldAP = prScanReqMsg->fgNeedMloScan;

	if (prScanReqMsg->u2IELenMl > 0 &&
	    prScanParam->u2IELenMl <= MAX_BAND_IE_LENGTH) {
		prScanParam->u2IELenMl = prScanReqMsg->u2IELenMl;
		kalMemCopy(prScanParam->aucIEMl,
			prScanReqMsg->aucIEMl, prScanParam->u2IELenMl);
	}
	if (prScanReqMsg->u2IELen2G4 > 0 &&
	    prScanParam->u2IELen2G4 <= MAX_BAND_IE_LENGTH) {
		prScanParam->u2IELen2G4 = prScanReqMsg->u2IELen2G4;
		kalMemCopy(prScanParam->aucIE2G4,
			prScanReqMsg->aucIE2G4, prScanParam->u2IELen2G4);
	}
	if (prScanReqMsg->u2IELen5G > 0 &&
	    prScanParam->u2IELen5G <= MAX_BAND_IE_LENGTH) {
		prScanParam->u2IELen5G = prScanReqMsg->u2IELen5G;
		kalMemCopy(prScanParam->aucIE5G,
			prScanReqMsg->aucIE5G, prScanParam->u2IELen5G);
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prScanReqMsg->u2IELen6G > 0 &&
	    prScanParam->u2IELen6G <= MAX_BAND_IE_LENGTH) {
		prScanParam->u2IELen6G = prScanReqMsg->u2IELen6G;
		kalMemCopy(prScanParam->aucIE6G,
			prScanReqMsg->aucIE6G, prScanParam->u2IELen6G);
	}
#endif
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Remove pending scan request
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void scnFsmRemovePendingMsg(struct ADAPTER *prAdapter, uint8_t ucSeqNum,
	uint8_t ucBssIndex)
{
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	struct MSG_HDR *prPendingMsgHdr = NULL;
	struct MSG_HDR *prPendingMsgHdrNext = NULL;
	struct MSG_HDR *prRemoveMsgHdr = NULL;
	struct LINK_ENTRY *prRemoveLinkEntry = NULL;
	u_int8_t fgIsRemovingScan = FALSE;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	/* traverse through rPendingMsgList for removal */
	LINK_FOR_EACH_ENTRY_SAFE(prPendingMsgHdr,
		prPendingMsgHdrNext, &(prScanInfo->rPendingMsgList),
		rLinkEntry, struct MSG_HDR) {

#define __MSG_ID__ prPendingMsgHdr->eMsgId
		if (__MSG_ID__ == MID_AIS_SCN_SCAN_REQ
		    || __MSG_ID__ == MID_P2P_SCN_SCAN_REQ
		    || __MSG_ID__ == MID_RLM_SCN_SCAN_REQ) {
			struct MSG_SCN_SCAN_REQ *prScanReqMsg
				= (struct MSG_SCN_SCAN_REQ *)
					prPendingMsgHdr;

			if (ucSeqNum == prScanReqMsg->ucSeqNum
				&& ucBssIndex == prScanReqMsg->ucBssIndex) {
				prRemoveLinkEntry
					= &(prScanReqMsg->rMsgHdr.rLinkEntry);
				prRemoveMsgHdr = prPendingMsgHdr;
				fgIsRemovingScan = TRUE;
			}
		} else if (__MSG_ID__ == MID_AIS_SCN_SCAN_REQ_V2
			   || __MSG_ID__ == MID_P2P_SCN_SCAN_REQ_V2
			   || __MSG_ID__ == MID_RLM_SCN_SCAN_REQ_V2) {
			struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsgV2
				= (struct MSG_SCN_SCAN_REQ_V2 *)
					prPendingMsgHdr;

			if (ucSeqNum == prScanReqMsgV2->ucSeqNum
				&& ucBssIndex == prScanReqMsgV2->ucBssIndex) {
				prRemoveLinkEntry
					= &(prScanReqMsgV2->rMsgHdr.rLinkEntry);
				prRemoveMsgHdr = prPendingMsgHdr;
				fgIsRemovingScan = TRUE;
			}
		}
#undef __MSG_ID__

		if (prRemoveLinkEntry) {
			if (fgIsRemovingScan == TRUE) {
				/* generate scan-done event for caller */
				scnFsmGenerateScanDoneMsg(prAdapter,
					prPendingMsgHdr->eMsgId, ucSeqNum,
					ucBssIndex, SCAN_STATUS_CANCELLED,
					MSG_SEND_METHOD_UNBUF);
			}

			/* remove from pending list */
			LINK_REMOVE_KNOWN_ENTRY(&(prScanInfo->rPendingMsgList),
				prRemoveLinkEntry);
			cnmMemFree(prAdapter, prRemoveMsgHdr);

			break;
		}
	}
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint8_t scnNeedMloScan(struct ADAPTER *prAdapter, uint8_t ucSeqNum)
{
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	struct MLD_AP_INFO *prMldAPInfo, *prMldAPInfoNext;
	struct BSS_DESC *prBssDesc;
	uint16_t i;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	/* add to mld ap list for mlo scan later */
	if (prScanParam->fgCollectMldAP) {
		struct LINK *prScanResult = &prScanInfo->rBSSDescList;

		/* reset mld ap list */
		LINK_INITIALIZE(&prScanInfo->rMldAPInfoList);
		prScanParam->fgCollectMldAP = FALSE;

		LINK_FOR_EACH_ENTRY(prBssDesc, prScanResult, rLinkEntry,
			struct BSS_DESC) {
			if (!prBssDesc->rMlInfo.fgValid)
				continue;

			for (i = 0; i < prScanParam->ucChannelListNum; i++) {
				if (prScanParam->arChnlInfoList[i].eBand
					== prBssDesc->eBand &&
				    prScanParam->arChnlInfoList[i].ucChannelNum
					== prBssDesc->ucChannelNum)
					break;
			}
			/* skip ap which is not in chnl list */
			if (i == prScanParam->ucChannelListNum)
				continue;

			prMldAPInfo =
			    (struct MLD_AP_INFO *)cnmMemAlloc(prAdapter,
			    RAM_TYPE_BUF, sizeof(struct MLD_AP_INFO));
			if (!prMldAPInfo)
				goto cleanup;

			kalMemZero(prMldAPInfo, sizeof(struct MLD_AP_INFO));
			prMldAPInfo->prBssDesc = prBssDesc;

			LINK_INSERT_TAIL(
				&prScanInfo->rMldAPInfoList,
				&prMldAPInfo->rLinkEntry);
			DBGLOG(ML, INFO,
				"Push MldAddr="MACSTR",BSS="MACSTR
				",LinkID=%d,MaxSimu=%d,EmlCap=0x%x,MldCap=0x%x,MldType=%d,Total=%d\n",
				MAC2STR(prBssDesc->rMlInfo.aucMldAddr),
				MAC2STR(prBssDesc->aucBSSID),
				prBssDesc->rMlInfo.ucLinkId,
				prBssDesc->rMlInfo.ucMaxSimuLinks,
				prBssDesc->rMlInfo.u2EmlCap,
				prBssDesc->rMlInfo.u2MldCap,
				prBssDesc->rMlInfo.fgMldType,
				prScanInfo->rMldAPInfoList.u4NumElem);
		}
	}

	/* traverse through pending request list */
	LINK_FOR_EACH_ENTRY_SAFE(prMldAPInfo,
				 prMldAPInfoNext,
				 &prScanInfo->rMldAPInfoList, rLinkEntry,
				 struct MLD_AP_INFO) {
		uint8_t aucIe[MAX_BAND_IE_LENGTH] = {0};
		uint32_t u4ScanIELen = 0;
		struct AIS_FSM_INFO *prAisFsmInfo;

		LINK_REMOVE_KNOWN_ENTRY(&prScanInfo->rMldAPInfoList,
			&prMldAPInfo->rLinkEntry);
		prBssDesc = prMldAPInfo->prBssDesc;
		cnmMemFree(prAdapter, prMldAPInfo);

		DBGLOG(ML, INFO,
			"Pop MldAddr="MACSTR",BSS="MACSTR
			",LinkID=%d,MaxSimu=%d,EmlCap=0x%x,MldCap=0x%x,MldType=%d,Total=%d\n",
			MAC2STR(prBssDesc->rMlInfo.aucMldAddr),
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->rMlInfo.ucLinkId,
			prBssDesc->rMlInfo.ucMaxSimuLinks,
			prBssDesc->rMlInfo.u2EmlCap,
			prBssDesc->rMlInfo.u2MldCap,
			prBssDesc->rMlInfo.fgMldType,
			prScanInfo->rMldAPInfoList.u4NumElem);

		u4ScanIELen = mldFillScanIE(prAdapter,
			prBssDesc, aucIe, sizeof(aucIe), FALSE,
			prBssDesc->rMlInfo.ucMldId);
		if (u4ScanIELen == 0)
			continue;

		prScanParam->ucSeqNum = ucSeqNum;

		prScanParam->eScanType = SCAN_TYPE_ACTIVE_SCAN;
		prScanParam->ucSSIDType = SCAN_REQ_SSID_SPECIFIED_ONLY;

		/* Not to handle RNR IE in this scan*/
		prScanParam->fgOobRnrParseEn = FALSE;

		/* Assign channel and BSSID */
		prScanParam->eScanChannel = SCAN_CHANNEL_SPECIFIED;
		prScanParam->ucChannelListNum = 1;
		prScanParam->arChnlInfoList[0].eBand =
			prBssDesc->eBand;
		prScanParam->arChnlInfoList[0].ucChannelNum =
			prBssDesc->ucChannelNum;
		prScanParam->ucBssidMatchCh[0] =
			prBssDesc->ucChannelNum;
		COPY_MAC_ADDR(prScanParam->aucBSSID[0],
			prBssDesc->aucBSSID);

		/* No BssidMatchSsid, set to default value */
		kalMemSet(prScanParam->ucBssidMatchSsidInd,
			CFG_SCAN_OOB_MAX_NUM,
			sizeof(prScanParam->ucBssidMatchSsidInd));

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		/* No BssidMatchShortSsid, set to default value */
		prScanParam->ucShortSSIDNum = 0;
		kalMemSet(prScanParam->ucBssidMatchShortSsidInd,
			CFG_SCAN_OOB_MAX_NUM,
			sizeof(prScanParam->ucBssidMatchShortSsidInd));
#endif

		/* MaskExtend set to ENUM_SCN_ML_PROBE */
		prScanParam->ucScnFuncMask |= ENUM_SCN_USE_PADDING_AS_BSSID;
		prScanParam->u4ScnFuncMaskExtend |= ENUM_SCN_ML_PROBE;

		/* Copy ML probe request IE */
		kalMemCopy(prScanParam->aucIEMl, aucIe, u4ScanIELen);
		prScanParam->u2IELenMl = (uint16_t)u4ScanIELen;

		/* Restart ScanDone timer to avoid mlo scan
		 * causing scan timeout
		 */
		prAisFsmInfo = aisGetAisFsmInfo(prAdapter,
				prScanParam->ucBssIndex);
		cnmTimerStopTimer(prAdapter,
				&prAisFsmInfo->rScanDoneTimer);
		cnmTimerStartTimer(prAdapter,
				&prAisFsmInfo->rScanDoneTimer,
				SEC_TO_MSEC(AIS_SCN_DONE_TIMEOUT_SEC));

		/* go for next scan */
		scnFsmSteps(prAdapter, SCAN_STATE_SCANNING);
		return TRUE;
	}

cleanup:
	while (!LINK_IS_EMPTY(&prScanInfo->rMldAPInfoList)) {
		LINK_REMOVE_HEAD(&prScanInfo->rMldAPInfoList,
			prMldAPInfo, struct MLD_AP_INFO *);
		cnmMemFree(prAdapter, prMldAPInfo);
	}
	return FALSE;
}
#endif

#if (CFG_SUPPORT_WIFI_RNR == 1)
void scnSetRnrScanParam(struct SCAN_PARAM *prScanParam,
	struct NEIGHBOR_AP_PARAM *prNeighborParam)
{
	uint8_t fgRnrChnlScan, ucBssidCnt = 0;
	uint16_t i;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint8_t aucNullAddr[] = NULL_MAC_ADDR;
	uint16_t j;
#endif

	fgRnrChnlScan = prNeighborParam->ucSSIDType &
		(SCAN_REQ_SSID_SPECIFIED | SCAN_REQ_SSID_SPECIFIED_ONLY);

	if (fgRnrChnlScan) {
		/* rnr chl scan can copy param directly */
		prScanParam->ucSSIDType = prNeighborParam->ucSSIDType;
		prScanParam->eScanChannel = prNeighborParam->eScanChannel;
		prScanParam->ucScnFuncMask = prNeighborParam->ucScnFuncMask;

		prScanParam->ucChannelListNum =
			prNeighborParam->ucChannelListNum;
		prScanParam->ucSSIDNum = prNeighborParam->ucSSIDNum;
		prScanParam->ucShortSSIDNum = 0;

		kalMemCopy(prScanParam->arChnlInfoList,
			prNeighborParam->arChnlInfoList,
			sizeof(prScanParam->arChnlInfoList));

		kalMemCopy(prScanParam->ucSpecifiedSSIDLen,
			prNeighborParam->ucSpecifiedSSIDLen,
			sizeof(prScanParam->ucSpecifiedSSIDLen));

		kalMemCopy(prScanParam->aucSpecifiedSSID,
			prNeighborParam->aucSpecifiedSSID,
			sizeof(prScanParam->aucSpecifiedSSID));

		/* reset to indicate this param is handled */
		prNeighborParam->ucChannelListNum = 0;
	} else {
		uint8_t ucRnrChNum;
		enum ENUM_BAND eBand;

		ucRnrChNum = prNeighborParam->arChnlInfoList[0].ucChannelNum;
		eBand = prNeighborParam->arChnlInfoList[0].eBand;

		prScanParam->ucSSIDType = prNeighborParam->ucSSIDType;
		prScanParam->eScanChannel = prNeighborParam->eScanChannel;
		prScanParam->ucScnFuncMask = prNeighborParam->ucScnFuncMask;

		/* don't add same chnl to the newest nbr ap param */
		for (i = 0; i < prScanParam->ucChannelListNum; i++) {
			if (ucRnrChNum == prScanParam
				->arChnlInfoList[i].ucChannelNum &&
			    eBand == prScanParam
				->arChnlInfoList[i].eBand)
				break;
		}

		if (i == prScanParam->ucChannelListNum) {
			struct RF_CHANNEL_INFO *prRfChnlInfo;

			prRfChnlInfo = &prScanParam->arChnlInfoList[
				prScanParam->ucChannelListNum];
			prScanParam->ucChannelListNum++;
			prRfChnlInfo->eBand = eBand;
			prRfChnlInfo->ucChannelNum = ucRnrChNum;
		}

		for (i = prNeighborParam->ucBssidStartIdx;
			i < prNeighborParam->ucBssidNum &&
			prScanParam->ucBssidNum <
				ARRAY_SIZE(prScanParam->aucBSSID); i++) {
			uint8_t *pucShortSSID;

			ucBssidCnt = prScanParam->ucBssidNum;
			pucShortSSID = prNeighborParam->aucShortSSID[i];

			COPY_MAC_ADDR(prScanParam->aucBSSID[ucBssidCnt],
				prNeighborParam->aucBSSID[i]);
			prScanParam->ucBssidMatchCh[ucBssidCnt] = ucRnrChNum;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			if (kalMemCmp(pucShortSSID, aucNullAddr,
				       MAX_SHORT_SSID_LEN) != 0) {
				for (j = 0; j < prScanParam->ucShortSSIDNum;
						j++) {
					if (kalMemCmp(pucShortSSID,
						prScanParam->aucShortSSID[j],
						MAX_SHORT_SSID_LEN) == 0)
						break;
				}

				if (j == prScanParam->ucShortSSIDNum) {
					kalMemCopy(
						prScanParam->aucShortSSID[j],
						pucShortSSID,
						MAX_SHORT_SSID_LEN);
					prScanParam->ucShortSSIDNum++;
					prScanParam->ucBssidMatchShortSsidInd[
						ucBssidCnt] = j;
				}
			}
#endif

			prScanParam->ucBssidNum++;
			prNeighborParam->ucBssidStartIdx++;
		}
	}
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void scnEventScanDone(struct ADAPTER *prAdapter,
	struct EVENT_SCAN_DONE *prScanDone, u_int8_t fgIsNewVersion)
{
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	KAL_SPIN_LOCK_DECLARATION();

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;
	prScanInfo->ucScanDoneVersion = prScanDone->ucScanDoneVersion;

	if (fgIsNewVersion) {
		scanlog_dbg(LOG_SCAN_DONE_F2D, INFO, "Version(%u)!size of ScanDone%zu,used[%d]free[%d],ucCompleteChanCount[%u],ucCurrentState%u, u4ScanDurBcnCnt[%u],Seq[%u]\n",
			prScanDone->ucScanDoneVersion,
			sizeof(struct EVENT_SCAN_DONE),
			prScanInfo->rBSSDescList.u4NumElem,
			prScanInfo->rFreeBSSDescList.u4NumElem,
			prScanDone->ucCompleteChanCount,
			prScanDone->ucCurrentState,
			prScanDone->u4ScanDurBcnCnt,
			prScanDone->ucSeqNum);

		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_FW);
		scanLogCacheFlushBSS(&(prScanInfo->rScanLogCache.rBSSListFW),
			LOG_SCAN_DONE_F2D);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_FW);

		if (prScanDone->ucCurrentState != FW_SCAN_STATE_SCAN_DONE) {
			log_dbg(SCN, INFO, "FW Scan timeout!generate ScanDone event at State%d complete chan count%d ucChannelListNum%d\n",
				prScanDone->ucCurrentState,
				prScanDone->ucCompleteChanCount,
				prScanParam->ucChannelListNum);

		} else {
			log_dbg(SCN, TRACE, "at FW_SCAN_STATE_SCAN_DONE state\n");
		}
	} else {
		scanlog_dbg(LOG_SCAN_DONE_F2D, INFO, "Old Version\n");
	}

	/* buffer empty channel information */
	if (prScanDone->ucSparseChannelValid) {
		scnFsmDumpScanDoneInfo(prAdapter, prScanDone);
	} else {
		prScanInfo->fgIsSparseChannelValid = FALSE;
	}
#if (CFG_SUPPORT_ROAMING_LOG == 1)
	roamingFsmLogScanBuffer(prAdapter, prScanParam->ucBssIndex);
#endif

	/* Full2Partial */
	if (prScanInfo->fgIsScanForFull2Partial &&
		prScanInfo->ucFull2PartialSeq == prScanDone->ucSeqNum) {
		uint32_t *pu4BitMap = &(prScanInfo->au4ChannelBitMap[0]);
#if (CFG_SUPPORT_WIFI_6G == 1)
		log_dbg(SCN, INFO,
			"Full2Partial(%u):%08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\n",
			scanCountBits(prScanInfo->au4ChannelBitMap,
			sizeof(prScanInfo->au4ChannelBitMap)),
			pu4BitMap[7], pu4BitMap[6], pu4BitMap[5], pu4BitMap[4],
			pu4BitMap[3], pu4BitMap[2], pu4BitMap[1], pu4BitMap[0],
			pu4BitMap[15], pu4BitMap[14], pu4BitMap[13],
			pu4BitMap[12], pu4BitMap[11], pu4BitMap[10],
			pu4BitMap[9], pu4BitMap[8]);
#else
		log_dbg(SCN, INFO,
			"Full2Partial(%u):%08X %08X %08X %08X %08X %08X %08X %08X\n",
			scanCountBits(prScanInfo->au4ChannelBitMap,
			sizeof(prScanInfo->au4ChannelBitMap)),
			pu4BitMap[7], pu4BitMap[6], pu4BitMap[5], pu4BitMap[4],
			pu4BitMap[3], pu4BitMap[2], pu4BitMap[1], pu4BitMap[0]);
#endif

		prScanInfo->fgIsScanForFull2Partial = FALSE;
	}

	if (prScanInfo->eCurrentState == SCAN_STATE_SCANNING
		&& prScanDone->ucSeqNum == prScanParam->ucSeqNum) {
#if (CFG_SUPPORT_WIFI_RNR == 1)
		uint8_t fgNeedRnrScan = FALSE;
#endif

		/* remove before next scan to check current scan req */
		scanRemoveBssDescsByPolicy(prAdapter,
			SCN_RM_POLICY_EXCLUDE_CONNECTED |
			SCN_RM_POLICY_EXCLUDE_SPECIFIC_SSID |
			SCN_RM_POLICY_TIMEOUT |
			SCN_RM_POLICY_MISS_COUNT,
			NULL);

#if (CFG_SUPPORT_WIFI_RNR == 1)
		prScanParam->fgOobRnrParseEn = FALSE;

		if (!LINK_IS_EMPTY(&prScanInfo->rNeighborAPInfoList)) {
			fgNeedRnrScan = TRUE;

			prScanParam->eScanType = SCAN_TYPE_ACTIVE_SCAN;
			prScanParam->ucSeqNum = prScanDone->ucSeqNum;
			prScanParam->ucBssidNum = 0;
			prScanParam->ucChannelListNum = 0;
			prScanParam->ucShortSSIDNum = 0;
			prScanParam->u2ChannelDwellTime =
				SCAN_CHANNEL_DWELL_TIME_MSEC_APP;
			prScanParam->u2ChannelMinDwellTime =
				SCAN_CHANNEL_MIN_DWELL_TIME_MSEC_APP;
			prScanParam->u4ScnFuncMaskExtend |=
				ENUM_SCN_RNR_SCAN;

			/* Init value = CFG_SCAN_OOB_MAX_NUM, if init value = 0
			 * will let FW confuse to match SSID ind 0.
			 */
			kalMemSet(prScanParam->ucBssidMatchSsidInd,
				CFG_SCAN_OOB_MAX_NUM,
				sizeof(prScanParam->ucBssidMatchSsidInd));
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			kalMemSet(prScanParam->ucBssidMatchShortSsidInd,
			      CFG_SCAN_OOB_MAX_NUM,
			      sizeof(prScanParam->ucBssidMatchShortSsidInd));
#endif
		}

		while (!LINK_IS_EMPTY(&prScanInfo->rNeighborAPInfoList)) {
			struct NEIGHBOR_AP_INFO *prNeighborAPInfo;
			struct NEIGHBOR_AP_PARAM *prNeighborParam;

			LINK_REMOVE_HEAD(&prScanInfo->rNeighborAPInfoList,
			prNeighborAPInfo, struct NEIGHBOR_AP_INFO *);

			prNeighborParam = &prNeighborAPInfo->rNeighborParam;
			scnSetRnrScanParam(prScanParam, prNeighborParam);

			/* rnr chnl scan handle param one by one */
			if (prNeighborParam->ucChannelListNum == 0) {
				cnmMemFree(prAdapter, prNeighborAPInfo);
				break;
			}

			if (prNeighborParam->ucBssidStartIdx <
			    prNeighborParam->ucBssidNum) {
				LINK_INSERT_HEAD(
					&prScanInfo->rNeighborAPInfoList,
					&prNeighborAPInfo->rLinkEntry);
			} else {
				cnmMemFree(prAdapter, prNeighborAPInfo);
			}

			/* trigger rnr bssid scan when full */
			if (prScanParam->ucBssidNum == CFG_SCAN_OOB_MAX_NUM)
				break;
		}

		if (fgNeedRnrScan) {
			/* Restart ScanDone timer to avoid RNR scan
			 * causing scan timeout
			 */
			if (IS_BSS_INDEX_AIS(prAdapter,
						prScanParam->ucBssIndex)) {
				struct AIS_FSM_INFO *prAisFsmInfo;

				prAisFsmInfo = aisGetAisFsmInfo(prAdapter,
					prScanParam->ucBssIndex);
#if CFG_SUPPORT_LLW_SCAN
				/* using customized scan parameters */
				prScanParam->u2ChannelDwellTime =
					prAisFsmInfo->ucNonDfsChDwellTimeMs;

				prScanParam->u2ChannelMinDwellTime =
					(prScanParam->u2ChannelDwellTime <
					SCAN_CHANNEL_DWELL_TIME_MIN_MSEC) ?
					prScanParam->u2ChannelDwellTime :
					SCAN_CHANNEL_DWELL_TIME_MIN_MSEC;

				prScanParam->u2OpChStayTime =
					prAisFsmInfo->u2OpChStayTimeMs;

				prScanParam->ucDfsChDwellTime =
					prAisFsmInfo->ucDfsChDwellTimeMs;

				prScanParam->ucPerScanChCnt =
					prAisFsmInfo->ucPerScanChannelCnt;
#endif
				cnmTimerStopTimer(prAdapter,
					&prAisFsmInfo->rScanDoneTimer);
				cnmTimerStartTimer(prAdapter,
					&prAisFsmInfo->rScanDoneTimer,
					SEC_TO_MSEC(AIS_SCN_DONE_TIMEOUT_SEC));
			}
			/* go for next scan */
			scnFsmSteps(prAdapter, SCAN_STATE_SCANNING);
			return;
		}
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (scnNeedMloScan(prAdapter, prScanDone->ucSeqNum))
			return;
#endif

		/* generate scan-done event for caller */
		scnFsmGenerateScanDoneMsg(prAdapter,
			prScanParam->eMsgId, prScanParam->ucSeqNum,
			prScanParam->ucBssIndex, SCAN_STATUS_DONE,
			MSG_SEND_METHOD_BUF);

		/* switch to next pending scan */
		scnFsmSteps(prAdapter, SCAN_STATE_IDLE);
	} else {
		log_dbg(SCN, INFO, "Unexpected SCAN-DONE event: SeqNum = %d, Current State = %d\n",
			prScanDone->ucSeqNum,
			prScanInfo->eCurrentState);
	}
	prScanInfo->fgWifiOnFirstScan = FALSE;
#if CFG_SUPPORT_SCAN_NO_AP_RECOVERY
	/* SCAN NO AP RECOVERY is only for AIS and not OOB scan,
	 * FW report scan done, reset ScnTimeoutTimes and reset count to 0
	 */
	prScanInfo->ucScnTimeoutTimes = 0;
	prScanInfo->ucScnTimeoutSubsysResetCnt = 0;

#if CFG_EXT_SCAN
	if (IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucScanNoApRecover) &&
		IS_BSS_INDEX_AIS(prAdapter, prScanParam->ucBssIndex) &&
		prScanInfo->fgIsSparseChannelValid &&
		(prScanParam->eMsgId == MID_AIS_SCN_SCAN_REQ ||
		prScanParam->eMsgId == MID_AIS_SCN_SCAN_REQ_V2)) {
		scnDoZeroChRecoveryCheck(prAdapter, prScanInfo);
	}
#endif /* CFG_EXT_SCAN */

#endif
}	/* end of scnEventScanDone */


/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
scnFsmDumpScanDoneInfo(struct ADAPTER *prAdapter,
	struct EVENT_SCAN_DONE *prScanDone)
{
	uint8_t ucScanChNum = 0;
	uint8_t ucChCnt = 0;
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	enum ENUM_BAND eBand = BAND_NULL;
	char strbuf[SCN_SCAN_DONE_PRINT_BUFFER_LENGTH];

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	prScanInfo->fgIsSparseChannelValid = TRUE;
	prScanInfo->rSparseChannel.eBand
		= (enum ENUM_BAND) prScanDone->rSparseChannel.ucBand;
	prScanInfo->rSparseChannel.ucChannelNum
		= prScanDone->rSparseChannel.ucChannelNum;
	ucScanChNum = prScanInfo->ucSparseChannelArrayValidNum
		= prScanDone->ucSparseChannelArrayValidNum;

	for (ucChCnt = 0; ucChCnt < ucScanChNum; ucChCnt++) {
		prScanInfo->au2ChannelIdleTime[ucChCnt]
			= prScanDone->au2ChannelIdleTime[ucChCnt];
		prScanInfo->aucChannelMDRDYCnt[ucChCnt]
			= prScanDone->aucChannelMDRDYCnt[ucChCnt];
		prScanInfo->aucChannelBAndPCnt[ucChCnt]
			= prScanDone->aucChannelBAndPCnt[ucChCnt];
		prScanInfo->au2ChannelScanTime[ucChCnt]
			= prScanDone->au2ChannelScanTime[ucChCnt];
		prScanInfo->aucChannelUtilVal[ucChCnt]
			= prScanDone->aucChannelUtilVal[ucChCnt];
		eBand =
		SCN_GET_EBAND_BY_CH_NUM(prScanDone->aucChannelNum[ucChCnt]);
		prScanInfo->aeChannelBand[ucChCnt] = eBand;
		prScanInfo->aucChannelNum[ucChCnt] =
			nicRxdChNumTranslate(eBand,
			prScanDone->aucChannelNum[ucChCnt]);
		scanFillChnlInfo(prAdapter, eBand,
			prScanInfo->aucChannelNum[ucChCnt],
			prScanDone->au2ChannelIdleTime[ucChCnt],
			prScanDone->au2ChannelScanTime[ucChCnt],
			prScanDone->aucChannelUtilVal[ucChCnt]);
	}
#if CFG_SUPPORT_SCAN_NO_AP_RECOVERY
#if CFG_EXT_SCAN
	if (ucScanChNum == 0)
		prScanInfo->ucScnZeroChannelCnt++;
	else {
		prScanInfo->ucScnZeroChannelCnt = 0;
		prScanInfo->ucScnZeroChSubsysResetCnt = 0;
	}
#endif
#endif

	if (prAdapter->rWifiVar.u2CountryCode) {
		log_dbg(SCN, INFO,
			"Country Code = %c%c, Detected_Channel_Num = %d\n",
			((prAdapter->rWifiVar.u2CountryCode
				& 0xff00) >> 8),
			(prAdapter->rWifiVar.u2CountryCode
				& 0x00ff), ucScanChNum);
	} else {
		log_dbg(SCN, INFO,
			"Country Code is NULL, Detected_Channel_Num = %d\n",
			ucScanChNum);
	}

#define print_info_ch(_Mod, _Clz, _Fmt, var) \
	do { \
		uint16_t u2Written = 0; \
		uint16_t u2TotalLen = SCN_SCAN_DONE_PRINT_BUFFER_LENGTH; \
		for (ucChCnt = 0; ucChCnt < ucScanChNum; ucChCnt++) { \
			u2Written += kalSnprintf(strbuf + u2Written, \
				u2TotalLen - u2Written, "%6d", \
				prScanInfo->var[ucChCnt]); \
		} \
		log_dbg(_Mod, _Clz, _Fmt, strbuf); \
	} while (0)

#define print_info(_Mod, _Clz, _Fmt, var) \
	do { \
		uint16_t u2Written = 0; \
		uint16_t u2TotalLen = SCN_SCAN_DONE_PRINT_BUFFER_LENGTH; \
		for (ucChCnt = 0; ucChCnt < ucScanChNum; ucChCnt++) { \
			u2Written += kalSnprintf(strbuf + u2Written, \
				u2TotalLen - u2Written, "%6d", \
				prScanInfo->var[ucChCnt]); \
		} \
		log_dbg(_Mod, _Clz, _Fmt, strbuf); \
	} while (0)

	/* If FW scan channel count more than Driver request,
	*  means this scan done event might have something wrong,
	*  not to print it to avoid unexpected issue
	*/
	if ((ucScanChNum > prScanParam->ucChannelListNum) &&
		(prScanParam->ucChannelListNum != 0)) {
		log_dbg(SCN, INFO,
			"Driver request %d ch, but FW scan %d ch!\n",
			prScanParam->ucChannelListNum,
			ucScanChNum);
		return;
	}

	print_info_ch(SCN, INFO, "Channel  : %s\n", aucChannelNum);
	print_info(SCN, INFO, "IdleTime : %s\n", au2ChannelIdleTime);
	print_info(SCN, INFO, "MdrdyCnt : %s\n", aucChannelMDRDYCnt);
	print_info(SCN, INFO, "BAndPCnt : %s\n", aucChannelBAndPCnt);
	print_info(SCN, LOUD, "ScanTime : %s\n", au2ChannelScanTime);
	if (prScanDone->ucScanDoneVersion >= SCAN_DONE_VERSION_SUPPORT_CU)
		print_info(SCN, INFO, "CU Value : %s\n", aucChannelUtilVal);

#undef	print_scan_info
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
scnFsmGenerateScanDoneMsg(struct ADAPTER *prAdapter,
	enum ENUM_MSG_ID eMsgId, uint8_t ucSeqNum, uint8_t ucBssIndex,
	enum ENUM_SCAN_STATUS eScanStatus, enum EUNM_MSG_SEND_METHOD eMethod)
{
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	struct MSG_SCN_SCAN_DONE *prScanDoneMsg;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	prScanDoneMsg = (struct MSG_SCN_SCAN_DONE *) cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG, sizeof(struct MSG_SCN_SCAN_DONE));
	if (!prScanDoneMsg) {
		ASSERT(0);	/* Can't indicate SCAN FSM Complete */
		return;
	}

	switch (eMsgId) {
	case MID_AIS_SCN_SCAN_REQ:
	case MID_AIS_SCN_SCAN_REQ_V2:
		prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_AIS_SCAN_DONE;
		break;
	case MID_P2P_SCN_SCAN_REQ:
	case MID_P2P_SCN_SCAN_REQ_V2:
		prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_P2P_SCAN_DONE;
		break;
	case MID_RLM_SCN_SCAN_REQ:
	case MID_RLM_SCN_SCAN_REQ_V2:
		prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_RLM_SCAN_DONE;
		break;
	default:
		log_dbg(SCN, ERROR, "Unexpected eMsgId: %d\n", eMsgId);
		cnmMemFree(prAdapter, prScanDoneMsg);
		return;
	}

	prScanDoneMsg->ucSeqNum = ucSeqNum;
	prScanDoneMsg->ucBssIndex = ucBssIndex;
	prScanDoneMsg->eScanStatus = eScanStatus;

	mboxSendMsg(prAdapter, MBOX_ID_0,
		(struct MSG_HDR *) prScanDoneMsg, eMethod);

}	/* end of scnFsmGenerateScanDoneMsg() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief        Query for most sparse channel
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
u_int8_t scnQuerySparseChannel(struct ADAPTER *prAdapter,
	enum ENUM_BAND *prSparseBand, uint8_t *pucSparseChannel)
{
	struct SCAN_INFO *prScanInfo;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (prScanInfo->fgIsSparseChannelValid == TRUE) {
		if (prSparseBand)
			*prSparseBand = prScanInfo->rSparseChannel.eBand;

		if (pucSparseChannel) {
			*pucSparseChannel
				= prScanInfo->rSparseChannel.ucChannelNum;
		}

		return TRUE;
	} else {
		return FALSE;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief        Event handler for schedule scan done event
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void scnEventSchedScanDone(struct ADAPTER *prAdapter,
	struct EVENT_SCHED_SCAN_DONE *prSchedScanDone)
{
	struct SCAN_INFO *prScanInfo;
	struct SCHED_SCAN_PARAM *prSchedScanParam;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prSchedScanParam = &prScanInfo->rSchedScanParam;

	if (prScanInfo->fgSchedScanning == TRUE) {
		scanlog_dbg(LOG_SCHED_SCAN_DONE_F2D, INFO, "scnEventSchedScanDone seq %u\n",
			prSchedScanDone->ucSeqNum);

		kalSchedScanResults(prAdapter->prGlueInfo);
	} else {
		scanlog_dbg(LOG_SCHED_SCAN_DONE_F2D, INFO, "Unexpected SCHEDSCANDONE event: Seq = %u, Current State = %d\n",
			prSchedScanDone->ucSeqNum, prScanInfo->eCurrentState);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief        Function to check spilit scan enable or not
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
bool scnEnableSplitScan(struct ADAPTER *prAdapter, uint8_t ucBssIndex,
				struct CMD_SCAN_REQ_V2 *prCmdScanReq)
{
	uint8_t ucWfdEn = FALSE, ucTrxPktEn = FALSE, ucRoamingEn = FALSE;
	struct PERF_MONITOR *prPerMonitor;
	struct BSS_INFO *prBssInfo = NULL;
	struct AIS_FSM_INFO *prAisFsmInfo;
	unsigned long ulTrxPacketsDiffTotal = 0;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	prPerMonitor = &prAdapter->rPerMonitor;

	if (!prBssInfo)
		return FALSE;
	/* Enable condition 1: WFD case*/
	ucWfdEn = wlanWfdEnabled(prAdapter);

	/* Enable condition 2: (TX + RX) packets in last 1s > 30,
	 * exclude P2P device because prPerMonitor not include P2P device
	 */
	if (ucBssIndex < MAX_BSSID_NUM && IS_BSS_ACTIVE(prBssInfo)) {
		ulTrxPacketsDiffTotal +=
			(prPerMonitor->ulTxPacketsDiffLastSec[ucBssIndex] +
			prPerMonitor->ulRxPacketsDiffLastSec[ucBssIndex]);

		if (ulTrxPacketsDiffTotal > SCAN_SPLIT_PACKETS_THRESHOLD) {
			log_dbg(SCN, TRACE, "SplitScan: TRXPacket=%ld",
				ulTrxPacketsDiffTotal);
			ucTrxPktEn = TRUE;
		}
	}
	/* Enable Pre-condition: not in roaming, avoid roaming scan too long */
	if (IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
		if (prAisFsmInfo &&
		    prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED &&
		    prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR) {
			ucRoamingEn = TRUE;
			prCmdScanReq->ucScnSourceMask =
					ENUM_SCN_ROMAING;
		}
	}
	log_dbg(SCN, TRACE, "SplitScan: Roam(%d),WFD(%d),TRX(%d)",
				ucRoamingEn, ucWfdEn, ucTrxPktEn);
	/* Enable split scan when (not in roam) & (WFD or TRX packet > 30) */
	if ((!ucRoamingEn) && (ucWfdEn || ucTrxPktEn))
		return TRUE;
	else
		return FALSE;
}

#if CFG_SUPPORT_SCHED_SCAN
/*----------------------------------------------------------------------------*/
/*!
 * \brief        handler for starting schedule scan
 *
 * \param[in]
 *
 * \return       TRUE if send sched scan successfully. FALSE otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t
scnFsmSchedScanRequest(struct ADAPTER *prAdapter,
	struct PARAM_SCHED_SCAN_REQUEST *prRequest)
{
	struct SCAN_INFO *prScanInfo;
	struct SCHED_SCAN_PARAM *prSchedScanParam;
	struct CMD_SCHED_SCAN_REQ *prSchedScanCmd = NULL;
	struct SSID_MATCH_SETS *prMatchSets = NULL;
	struct PARAM_SSID *prSsid = NULL;
	uint32_t i;
	uint16_t u2IeLen;
	enum ENUM_BAND ePreferedChnl = BAND_NULL;
	struct BSS_INFO *prAisBssInfo;
	uint8_t active;

	ASSERT(prAdapter);
	ASSERT(prRequest);
	ASSERT(prRequest->u4SsidNum <= CFG_SCAN_HIDDEN_SSID_MAX_NUM);
	ASSERT(prRequest->u4MatchSsidNum <= CFG_SCAN_SSID_MATCH_MAX_NUM);
	log_dbg(SCN, TRACE, "scnFsmSchedScanRequest\n");

	prAisBssInfo = aisGetAisBssInfo(prAdapter,
		prRequest->ucBssIndex);
	if (prAisBssInfo == NULL) {
		log_dbg(SCN, WARN, "prAisBssInfo is NULL\n");
		return FALSE;
	}

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prSchedScanParam = &prScanInfo->rSchedScanParam;

	if (prScanInfo->fgSchedScanning) {
		log_dbg(SCN, WARN, "prScanInfo->fgSchedScanning = TRUE already scanning\n");

		return FALSE;
	}

	/* 0. allocate memory for schedule scan command */
	if (prRequest->u4IELength <= MAX_IE_LENGTH)
		u2IeLen = (uint16_t)prRequest->u4IELength;
	else
		u2IeLen = MAX_IE_LENGTH;

	prSchedScanCmd = (struct CMD_SCHED_SCAN_REQ *) cnmMemAlloc(prAdapter,
		RAM_TYPE_BUF, sizeof(struct CMD_SCHED_SCAN_REQ) + u2IeLen);
	if (!prSchedScanCmd) {
		log_dbg(SCN, ERROR, "alloc CMD_SCHED_SCAN_REQ (%zu+%u) fail\n",
			sizeof(struct CMD_SCHED_SCAN_REQ), u2IeLen);
		return FALSE;
	}
	kalMemZero(prSchedScanCmd, sizeof(struct CMD_SCHED_SCAN_REQ) + u2IeLen);
	prMatchSets = &(prSchedScanCmd->auMatchSsid[0]);
	prSsid = &(prSchedScanCmd->auSsid[0]);

	/* 1 Set Sched scan param parameters */
	prSchedScanParam->ucSeqNum++;
	prSchedScanParam->ucBssIndex = prAisBssInfo->ucBssIndex;
	prSchedScanParam->fgStopAfterIndication = FALSE;

	prSchedScanCmd->ucBssIndex = prSchedScanParam->ucBssIndex;
	active = IS_NET_ACTIVE(prAdapter, prAisBssInfo->ucBssIndex);
	if (!active) {
		/* sync with firmware */
		nicActivateNetwork(prAdapter, prAisBssInfo->ucBssIndex);
	}
	/* 2.1 Prepare command. Set FW struct SSID_MATCH_SETS */
	/* ssid in ssid list will be send in probe request in advance */
	prSchedScanCmd->ucSsidNum = prRequest->u4SsidNum;
	for (i = 0; i < prSchedScanCmd->ucSsidNum &&
		    i < CFG_SCAN_HIDDEN_SSID_MAX_NUM; i++) {
		kalMemCopy(&(prSsid[i]), &(prRequest->arSsid[i]),
			sizeof(struct PARAM_SSID));
		log_dbg(SCN, TRACE, "ssid set(%d) %s\n", i,
			HIDE(prSsid[i].aucSsid));
	}

	prSchedScanCmd->ucMatchSsidNum = prRequest->u4MatchSsidNum;
	for (i = 0; i < prSchedScanCmd->ucMatchSsidNum &&
		    i < CFG_SCAN_SSID_MATCH_MAX_NUM; i++) {
		COPY_SSID(prMatchSets[i].aucSsid, prMatchSets[i].ucSsidLen,
			prRequest->arMatchSsid[i].aucSsid,
			prRequest->arMatchSsid[i].u4SsidLen);
		prMatchSets[i].i4RssiThresold = prRequest->ai4RssiThold[i];
		log_dbg(SCN, TRACE, "Match set(%d) %s, rssi>%d\n",
				i, prMatchSets[i].aucSsid,
				prMatchSets[i].i4RssiThresold);
	}

	/* 2.2 Prepare command. Set channel */

	ePreferedChnl
		= prAdapter->aePreferBand[NETWORK_TYPE_AIS];
	if (ePreferedChnl == BAND_2G4) {
		prSchedScanCmd->ucChannelType =
			SCAN_CHANNEL_2G4;
		prSchedScanCmd->ucChnlNum = 0;
	} else if (ePreferedChnl == BAND_5G) {
		prSchedScanCmd->ucChannelType =
			SCAN_CHANNEL_5G;
		prSchedScanCmd->ucChnlNum = 0;
#if (CFG_SUPPORT_WIFI_6G == 1)
	} else if (ePreferedChnl == BAND_6G) {
		prSchedScanCmd->ucChannelType =
			SCAN_CHANNEL_6G;
		prSchedScanCmd->ucChnlNum = 0;
#endif
	} else if (prRequest->ucChnlNum > 0 &&
		prRequest->ucChnlNum <= MAXIMUM_OPERATION_CHANNEL_LIST) {
		prSchedScanCmd->ucChannelType =
			SCAN_CHANNEL_SPECIFIED;
		prSchedScanCmd->ucChnlNum = prRequest->ucChnlNum;
		for (i = 0; i < prRequest->ucChnlNum; i++) {
			prSchedScanCmd->aucChannel[i].ucChannelNum =
				prRequest->aucChannel[i].ucChannelNum;
			prSchedScanCmd->aucChannel[i].ucBand =
				prRequest->aucChannel[i].ucBand;
		}
	} else {
		prSchedScanCmd->ucChnlNum = 0;
		prSchedScanCmd->ucChannelType =
			SCAN_CHANNEL_FULL;
	}

	prSchedScanCmd->ucSeqNum = prSchedScanParam->ucSeqNum;
	prSchedScanCmd->fgStopAfterIndication =
		prSchedScanParam->fgStopAfterIndication;
	prSchedScanCmd->u2IELen = u2IeLen;
	prSchedScanCmd->ucVersion = SCHED_SCAN_CMD_VERSION;
	if (prSchedScanCmd->u2IELen) {
		kalMemCopy(prSchedScanCmd->aucIE, prRequest->pucIE,
				prSchedScanCmd->u2IELen);
	}

	prSchedScanCmd->ucScnFuncMask |= prRequest->ucScnFuncMask;

	if (kalIsValidMacAddr(prRequest->aucRandomMac)) {
		prSchedScanCmd->ucScnFuncMask |=
			(ENUM_SCN_RANDOM_MAC_EN | ENUM_SCN_RANDOM_SN_EN);
		kalMemCopy(prSchedScanCmd->aucRandomMac,
			prRequest->aucRandomMac, MAC_ADDR_LEN);
	}

	scnSetSchedScanPlan(prAdapter, prSchedScanCmd,
				prRequest->u2ScanInterval);

	log_dbg(SCN, INFO, "V(%u)seq(%u)sz(%zu)chT(%u)chN(%u)ssid(%u)match(%u)IE(%u=>%u)MSP(%u),Intv(%d),Func(0x%X)\n",
		prSchedScanCmd->ucVersion,
		prSchedScanCmd->ucSeqNum, sizeof(struct CMD_SCHED_SCAN_REQ),
		prSchedScanCmd->ucChannelType, prSchedScanCmd->ucChnlNum,
		prSchedScanCmd->ucSsidNum, prSchedScanCmd->ucMatchSsidNum,
		prRequest->u4IELength, prSchedScanCmd->u2IELen,
		prSchedScanCmd->ucMspEntryNum,
		prRequest->u2ScanInterval,
		prSchedScanCmd->ucScnFuncMask);

	/* 3. send command packet to FW */
	do {
		if (!scnFsmSchedScanSetCmd(prAdapter, prSchedScanCmd)) {
			log_dbg(SCN, TRACE, "scnFsmSchedScanSetCmd failed\n");
			break;
		}
		if (!scnFsmSchedScanSetAction(prAdapter,
				SCHED_SCAN_ACT_ENABLE)) {
			log_dbg(SCN, TRACE, "scnFsmSchedScanSetAction failed\n");
			break;
		}
		prScanInfo->fgSchedScanning = TRUE;
	} while (0);

	if (!prScanInfo->fgSchedScanning && !active)
		nicDeactivateNetwork(prAdapter, prAisBssInfo->ucBssIndex);

	cnmMemFree(prAdapter, (void *) prSchedScanCmd);

	return prScanInfo->fgSchedScanning;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief         handler for stopping scheduled scan
 *
 * \param[in]
 *
 * \return        TRUE if send stop command successfully. FALSE otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t scnFsmSchedScanStopRequest(struct ADAPTER *prAdapter)
{
	uint8_t ucBssIndex = 0;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct SCAN_INFO *prScanInfo;

	ASSERT(prAdapter);

	ucBssIndex =
		prAdapter->rWifiVar.rScanInfo.rSchedScanParam.ucBssIndex;
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (!scnFsmSchedScanSetAction(prAdapter, SCHED_SCAN_ACT_DISABLE)) {
		log_dbg(SCN, TRACE, "scnFsmSchedScanSetAction failed\n");
		return FALSE;
	}

	prAdapter->rWifiVar.rScanInfo.fgSchedScanning = FALSE;

	/* Deactivate network when not connected and not in normal scan */
	if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE &&
		prScanInfo->eCurrentState == SCAN_STATE_IDLE)
		nicDeactivateNetwork(prAdapter, ucBssIndex);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief handler for setting schedule scan action
 * \param prAdapter       adapter
 * \param ucSchedScanAct  schedule scan action. set enable/disable to FW
 *
 * \return TRUE if send query command successfully. FALSE otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t
scnFsmSchedScanSetAction(struct ADAPTER *prAdapter,
		 enum ENUM_SCHED_SCAN_ACT ucSchedScanAct)
{
	struct CMD_SET_SCHED_SCAN_ENABLE rCmdSchedScanAction;
	uint32_t rStatus;

	ASSERT(prAdapter);

	kalMemZero(&rCmdSchedScanAction,
		sizeof(struct CMD_SET_SCHED_SCAN_ENABLE));

	/* 0:enable, 1:disable */
	rCmdSchedScanAction.ucSchedScanAct = ucSchedScanAct;

	if (ucSchedScanAct == SCHED_SCAN_ACT_ENABLE) {
		scanlog_dbg(LOG_SCHED_SCAN_REQ_START_D2F, INFO, "sched scan action = %d\n",
			rCmdSchedScanAction.ucSchedScanAct);
	} else {
		scanlog_dbg(LOG_SCHED_SCAN_REQ_STOP_D2F, INFO, "sched scan action = %d\n",
			rCmdSchedScanAction.ucSchedScanAct);
	}

	rStatus = wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SET_SCAN_SCHED_ENABLE,
			    TRUE,
			    FALSE,
			    FALSE,
			    nicCmdEventSetCommon,
			    nicOidCmdTimeoutCommon,
			    sizeof(struct CMD_SET_SCHED_SCAN_ENABLE),
			    (uint8_t *)&rCmdSchedScanAction, NULL, 0);

	return (rStatus != WLAN_STATUS_FAILURE) ? TRUE : FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief                 handler for setting schedule scan command
 * \param prAdapter       adapter
 * \param prSchedScanCmd  schedule scan command
 *
 * \return                TRUE if send query command successfully.
 *                        FAIL otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t
scnFsmSchedScanSetCmd(struct ADAPTER *prAdapter,
		struct CMD_SCHED_SCAN_REQ *prSchedScanCmd)
{
	uint16_t u2IeSize = 0;
	uint32_t rStatus;

	ASSERT(prAdapter);

	log_dbg(SCN, TRACE, "--> %s()\n", __func__);

	if (prSchedScanCmd)
		u2IeSize = prSchedScanCmd->u2IELen;
	rStatus = wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SET_SCAN_SCHED_REQ,
			    TRUE,
			    FALSE,
			    FALSE,
			    nicCmdEventSetCommon,
			    nicOidCmdTimeoutCommon,
			    sizeof(struct CMD_SCHED_SCAN_REQ) + u2IeSize,
			    (uint8_t *) prSchedScanCmd, NULL, 0);

	return (rStatus != WLAN_STATUS_FAILURE) ? TRUE : FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief                 Set schedule scan multiple scan plan (scan interval)
 * \param prAdapter       adapter
 * \param prSchedScanCmd  schedule scan command request
 *
 * \return                void
 */
/*----------------------------------------------------------------------------*/
void
scnSetSchedScanPlan(struct ADAPTER *prAdapter,
		struct CMD_SCHED_SCAN_REQ *prSchedScanCmd,
		uint16_t u2ScanInterval)
{
	/* Set Multiple Scan Plan here */
	log_dbg(SCN, TRACE, "--> %s()\n", __func__);

	ASSERT(prAdapter);


	if (u2ScanInterval != 0) {
		uint8_t i;

		prSchedScanCmd->ucMspEntryNum = 10;
		for (i = 0; i < 10; i++)
			prSchedScanCmd->au2MspList[i] = u2ScanInterval;
	} else {
		prSchedScanCmd->ucMspEntryNum = 0;
		kalMemZero(prSchedScanCmd->au2MspList,
				sizeof(prSchedScanCmd->au2MspList));
	}
}

#endif /* CFG_SUPPORT_SCHED_SCAN */

/*----------------------------------------------------------------------------*/
/*!
 * \brief                 Check if we can allow conduct a scan process now
 * \param prAdapter       adapter
 *
 * \return                TRUE if we can conduct scan;
 *                        FALSE if we should not start a scan.
 */
/*----------------------------------------------------------------------------*/
u_int8_t
scnDoAllowScanCheck(struct ADAPTER *prAdapter)
{
#if CFG_ENABLE_CSA_BLOCK_SCAN
	if (p2pFuncIsCsaBlockScan(prAdapter)) {
		log_dbg(SCN, INFO, "Abort Scan due to CSA blocking.\n");
		return FALSE;
	}
#endif

#if CFG_SUPPORT_RTT
	if (rttIsRunning(prAdapter)) {
		log_dbg(SCN, INFO, "Abort Scan due to RTT blocking.\n");
		return FALSE;
	}
#endif
	return TRUE;
}

#if CFG_SUPPORT_SCAN_NO_AP_RECOVERY
/*----------------------------------------------------------------------------*/
/*!
 * \brief                 Check scan result Mdrdy is all zero or not
 * \param aucChannelMDRDYCnt       Channel Mdrdy result
 * \param ucChNum	  Scan channel count
 *
 * \return                void
 */
/*----------------------------------------------------------------------------*/
void
scnDoZeroMdrdyRecoveryCheck(struct ADAPTER *prAdapter,
		struct EVENT_SCAN_DONE *prScanDone,
		struct SCAN_INFO *prScanInfo, uint8_t ucBssIndex)
{
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint8_t i, fgRecovery = TRUE;
	uint8_t fgZeroMdrdy = TRUE, fgZeroBeaconProbeReq = TRUE;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* Check scan channel count > 5, too few channel might false alarm */
	if (!prScanInfo->fgIsSparseChannelValid ||
		prScanDone->ucSparseChannelArrayValidNum <= 5)
		fgRecovery = FALSE;

	/* exclude OOB scan */
	if (prScanInfo->rScanParam.ucScnFuncMask
		& ENUM_SCN_USE_PADDING_AS_BSSID)
		fgRecovery = FALSE;

	/* Not to do recovery for roaming scan */
	if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR)
		fgRecovery = FALSE;

	if (!fgRecovery)
		return;

	for (i = 0; i < prScanDone->ucSparseChannelArrayValidNum; i++) {
		if (prScanDone->aucChannelMDRDYCnt[i] > 0)
			fgZeroMdrdy = FALSE;
		if (prScanDone->aucChannelBAndPCnt[i] > 0)
			fgZeroBeaconProbeReq = FALSE;
	}

	/* Abnormal: total Mdrdy=0 or beacon+ProbReq=0 case */
	if (fgZeroMdrdy || fgZeroBeaconProbeReq) {
		prScanInfo->ucScnZeroMdrdyTimes++;

		log_dbg(SCN, WARN, "ScanRecover: Mdrdy(%d),BandP(%d), Count(%d), Conn(%d), SER(%d), Reset(%d)",
			fgZeroMdrdy, fgZeroBeaconProbeReq,
			prScanInfo->ucScnZeroMdrdyTimes,
			prAisBssInfo->eConnectionState,
			prScanInfo->ucScnZeroMdrdySerCnt,
			prScanInfo->ucScnZeroMdrdySubsysResetCnt);

		/* Do L1 SER if continuous abnormal count >= 3,
		 * if first L1 SER not useful, will do twice
		 */
		if (prScanInfo->ucScnZeroMdrdyTimes >= prWifiVar->
			ucScanNoApRecoverTh)
			if (prScanInfo->ucScnZeroMdrdySerCnt < 2) {
				prScanInfo->ucScnZeroMdrdySerCnt++;
				wlanoidSerExtCmd(prAdapter,
						SER_ACTION_SET_ENABLE_MASK,
						(SER_ENABLE_TRACKING |
						SER_ENABLE_L1_RECOVER |
						SER_ENABLE_L2_RECOVER |
						SER_ENABLE_L3_RX_ABORT |
						SER_ENABLE_L3_TX_ABORT |
						SER_ENABLE_L3_TX_DISABLE |
						SER_ENABLE_L3_BF_RECOVER), 0);
				wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
					SER_SET_L1_RECOVER, 0);
			}
			/* TODO:
			 * If still abnormal after do twice L1 SER
			 * (ucScnZeroMdrdySerCnt >= 2)
			 * do subsys reset if no connection.
			 */
	}
	/* Normal: Mdrdy>0 and beacon+ProbReq>0 case */
	else {
		prScanInfo->ucScnZeroMdrdyTimes = 0;
		prScanInfo->ucScnZeroMdrdySerCnt = 0;
		prScanInfo->ucScnZeroMdrdySubsysResetCnt = 0;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief                 Check scan result Mdrdy is all zero or not
 * \param aucChannelMDRDYCnt       Channel Mdrdy result
 * \param ucChNum	  Scan channel count
 *
 * \return                void
 */
/*----------------------------------------------------------------------------*/
void
scnDoScanTimeoutRecoveryCheck(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex)
{
	struct BSS_INFO *prAisBssInfo;
	struct SCAN_INFO *prScanInfo;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prScanInfo->ucScnTimeoutTimes++;

	log_dbg(SCN, WARN, "ScanRecover:Conn(%d), ScnTimeoutCount(%d), Reset(%d)",
		prAisBssInfo->eConnectionState,
		prScanInfo->ucScnTimeoutTimes,
		prScanInfo->ucScnTimeoutSubsysResetCnt);

	/* If scanDoneTimeout count > 3 and no connection, do subsys reset */
	if (prScanInfo->ucScnTimeoutTimes >= prWifiVar->ucScanNoApRecoverTh) {
		if (prScanInfo->ucScnTimeoutSubsysResetCnt < 1 &&
		   prAisBssInfo->eConnectionState == MEDIA_STATE_DISCONNECTED) {
			prScanInfo->ucScnTimeoutSubsysResetCnt++;
			GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_SCAN_RECOVERY);
		}
	}
}

#if CFG_EXT_SCAN
void
scnDoZeroChRecoveryCheck(struct ADAPTER *prAdapter,
		struct SCAN_INFO *prScanInfo)
{
	log_dbg(SCN, WARN,
		"ScanRecover: ScnZeroChCount(%d), ResetCount(%d)\n",
		prScanInfo->ucScnZeroChannelCnt,
		prScanInfo->ucScnZeroChSubsysResetCnt);

	/* If scanDoneTimeout count > 3, do subsys reset */
	if (prScanInfo->ucScnZeroChannelCnt > 3 &&
		prScanInfo->ucScnZeroChSubsysResetCnt < 1) {
		prScanInfo->ucScnZeroChSubsysResetCnt++;
		GL_DEFAULT_RESET_TRIGGER(prAdapter,
			RST_SCAN_RECOVERY);
	}
}
#endif
#endif

enum ENUM_SCN_DONE_REASON {
	SCN_DONE_OK = 0,
	SCN_DONE_TIMEOUT,
	SCN_DONE_DRIVER_ABORT,
	SCN_DONE_NUM
};

void
scnFsmNotifyEvent(struct ADAPTER *prAdapter,
		enum ENUM_SCAN_STATUS eStatus,
		uint8_t ucBssIndex)
{
	struct SCAN_INFO *prScanInfo;
	char uEvent[300], strbuf[200] = "N/A";
	uint8_t fgIsScanNormal = TRUE, fgIsDbdcScan = TRUE;
	uint8_t i, ucReasonInd, ucWritten = 0, fgAnyConnection = FALSE;
	uint8_t ucTotalLen = 200;
	static const char * const apucScnReason[3] = {
		"OK",
		"TIMEOUT",
		"DRIVER ABORT",
	};
	uint32_t u4ScanTime = 0;

	/*
	 * Status: NORMAL, ABNORMAL (scan timeout or driver abort scan both
	 * classify to ABNORMAL)
	 * DBDC: ENABLE, DISABLE
	 * Reason: TIMEOUT, DRIVER ABORT (if status NORMAL, print OK)
	 * Time: in ms
	 * Channel: (divide by space) ex: 1 2 3 4 5 6
	 *
	 * eStatus = SCAN_STATUS_DONE --> normal scan done OR scan timeout!
	 * eStatus = SCAN_STATUS_CANCELLED --> driver abort
	 */
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (prScanInfo->fgIsScanTimeout || eStatus == SCAN_STATUS_CANCELLED) {
		fgIsScanNormal = FALSE;
		ucReasonInd = prScanInfo->fgIsScanTimeout ? SCN_DONE_TIMEOUT :
				SCN_DONE_DRIVER_ABORT;
		prScanInfo->fgIsScanTimeout = FALSE;
	}
	/* Currently FW only do DBDC scan when no connection, but this condition
	 * might changed.
	 */
	for (i = 0; i < KAL_AIS_NUM; i++) {
		if (IS_BSS_ALIVE(prAdapter, prAdapter->aprBssInfo[i])) {
			fgAnyConnection	= TRUE;
			break;
		}
	}
	if (prAdapter->rWifiVar.eDbdcMode == ENUM_DBDC_MODE_DISABLED
		|| fgAnyConnection)
		fgIsDbdcScan = FALSE;

	if (fgIsScanNormal) {
		u4ScanTime = USEC_TO_MSEC(SYSTIME_TO_USEC(kalGetTimeTick()
					- prScanInfo->rLastScanStartTime));
		ucReasonInd = SCN_DONE_OK;
		for (i = 0; i < prScanInfo->ucSparseChannelArrayValidNum; i++) {
			ucWritten += kalSnprintf(strbuf + ucWritten,
					ucTotalLen - ucWritten, "%d ",
					prScanInfo->aucChannelNum[i]);
		}
	}
	kalSnprintf(uEvent, sizeof(uEvent),
		"Scan=Status:%s,DBDC:%s,Time:%d,Channel:%s,Reason:%s",
		(fgIsScanNormal ? "NORMAL" : "ABNORMAL"),
		(fgIsDbdcScan ? "ENABLE" : "DISABLE"),
		u4ScanTime,
		strbuf,
		apucScnReason[ucReasonInd]);

	DBGLOG(SCN, LOUD, "request uevent:%s\n", uEvent);
	/* Only send Uevent if BSS is AIS */
	if (IS_BSS_INDEX_AIS(prAdapter, ucBssIndex))
		kalSendUevent(prAdapter, uEvent);
}
