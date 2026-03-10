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
#ifndef _P2P_FUNC_H
#define _P2P_FUNC_H

#define P2P_OFF_CHNL_TX_DEFAULT_TIME_MS                      1000

#if (CFG_SUPPORT_DFS_MASTER == 1)
extern struct P2P_RADAR_INFO g_rP2pRadarInfo;

enum _ENUM_DFS_STATE_T {
	DFS_STATE_INACTIVE = 0,
	DFS_STATE_CHECKING,
	DFS_STATE_ACTIVE,
	DFS_STATE_DETECTED,
	DFS_STATE_NUM
};
#endif

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */
#define ATTRI_ID(_fp)       (((struct P2P_ATTRIBUTE *)_fp)->ucId)
#define ATTRI_LEN(_fp)      \
(((uint16_t)((uint8_t *)&((struct P2P_ATTRIBUTE *)_fp)->u2Length)[0]) | \
((uint16_t)((uint8_t *)&((struct P2P_ATTRIBUTE *)_fp)->u2Length)[1] << 8))

#define ATTRI_SIZE(_fp)     (P2P_ATTRI_HDR_LEN + ATTRI_LEN(_fp))

#define P2P_ATTRI_FOR_EACH(_pucAttriBuf, _u2AttriBufLen, _u2Offset) \
	for ((_u2Offset) = 0; ((_u2Offset) < (_u2AttriBufLen)); \
	     (_u2Offset) += ATTRI_SIZE(_pucAttriBuf), \
	     ((_pucAttriBuf) += ATTRI_SIZE(_pucAttriBuf)))

#define P2P_IE(_fp)          ((struct IE_P2P *)_fp)

#define WSC_ATTRI_ID(_fp)     \
(((uint16_t)((uint8_t *)&((struct WSC_ATTRIBUTE *)_fp)->u2Id)[0] << 8) | \
((uint16_t)((uint8_t *)&((struct WSC_ATTRIBUTE *)_fp)->u2Id)[1]))

#define WSC_ATTRI_LEN(_fp)      \
(((uint16_t)((uint8_t *)&((struct WSC_ATTRIBUTE *)_fp)->u2Length)[0] << 8) | \
((uint16_t)((uint8_t *)&((struct WSC_ATTRIBUTE *)_fp)->u2Length)[1]))

#define WSC_ATTRI_SIZE(_fp)     (WSC_ATTRI_HDR_LEN + WSC_ATTRI_LEN(_fp))

#define WSC_ATTRI_FOR_EACH(_pucAttriBuf, _u2AttriBufLen, _u2Offset) \
	for ((_u2Offset) = 0; ((_u2Offset) < (_u2AttriBufLen)); \
	     (_u2Offset) += WSC_ATTRI_SIZE(_pucAttriBuf), \
	     ((_pucAttriBuf) += WSC_ATTRI_SIZE(_pucAttriBuf)))

#define WSC_IE(_fp)          ((struct IE_P2P *)_fp)

#define WFD_ATTRI_SIZE(_fp)     (P2P_ATTRI_HDR_LEN + WSC_ATTRI_LEN(_fp))

#define WFD_ATTRI_FOR_EACH(_pucAttriBuf, _u2AttriBufLen, _u2Offset) \
	for ((_u2Offset) = 0; ((_u2Offset) < (_u2AttriBufLen)); \
	     (_u2Offset) += WFD_ATTRI_SIZE(_pucAttriBuf), \
	     ((_pucAttriBuf) += WFD_ATTRI_SIZE(_pucAttriBuf)))

/******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

void p2pFuncRequestScan(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo);

void p2pFuncCancelScan(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo);

void
p2pFuncUpdateBssInfoForJOIN(struct ADAPTER *prAdapter,
		struct BSS_DESC *prBssDesc,
		struct STA_RECORD *prStaRec,
		struct BSS_INFO *prP2pBssInfo,
		struct SW_RFB *prAssocRspSwRfb);

void
p2pFuncAddPendingMgmtLinkEntry(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx, uint64_t u8Cookie);

void
p2pFuncRemovePendingMgmtLinkEntry(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx, uint64_t u8Cookie);

void p2pFuncAcquireCh(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

void
p2pFuncDisconnect(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		struct STA_RECORD *prStaRec,
		u_int8_t fgSendDeauth,
		uint16_t u2ReasonCode,
		u_int8_t fgIsLocallyGenerated);

struct BSS_INFO *p2pFuncBSSIDFindBssInfo(struct ADAPTER *prAdapter,
		uint8_t *pucBSSID);

void p2pFuncGCJoin(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_JOIN_INFO *prP2pJoinInfo);

void p2pFuncStopComplete(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo);

void
p2pFuncStartGO(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo,
		struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo);

void p2pFuncStopGO(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo);

uint32_t p2pFuncRoleToBssIdx(struct ADAPTER *prAdapter,
		uint8_t ucRoleIdx,
		uint8_t *pucBssIdx);

struct P2P_ROLE_FSM_INFO *p2pFuncGetRoleByBssIdx(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex);

void
p2pFuncSwitchOPMode(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_OP_MODE eOpMode,
		u_int8_t fgSyncToFW);

void p2pFuncReleaseCh(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

#if (CFG_SUPPORT_DFS_MASTER == 1)
void p2pFuncSetDfsChannelAvailable(struct ADAPTER *prAdapter,
		uint8_t ucChannel, uint8_t ucAvailable);

void p2pFuncChannelListFiltering(struct ADAPTER *prAdapter,
		uint16_t ucFilteredCh, uint8_t ucFilteredBw,
		uint8_t pucNumOfChannel,
		struct RF_CHANNEL_INFO *paucChannelList,
		uint8_t *pucOutNumOfChannel,
		struct RF_CHANNEL_INFO *paucOutChannelList);

void p2pFuncStartRdd(struct ADAPTER *prAdapter, uint8_t ucBssIdx);

void p2pFuncStopRdd(struct ADAPTER *prAdapter, uint8_t ucBssIdx);

void p2pFuncCsaUpdateGcStaRec(struct BSS_INFO *prBssInfo);

void p2pFuncDfsSwitchCh(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo);

u_int8_t p2pFuncCheckWeatherRadarBand(
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

int32_t p2pFuncSetDriverCacTime(uint32_t u4CacTime);

void p2pFuncEnableManualCac(void);

uint32_t p2pFuncGetDriverCacTime(void);

u_int8_t p2pFuncIsManualCac(void);

void p2pFuncRadarInfoInit(void);

void p2pFuncShowRadarInfo(struct ADAPTER *prAdapter, uint8_t ucBssIdx);

void p2pFuncGetRadarInfo(struct P2P_RADAR_INFO *prP2pRadarInfo);

uint8_t *p2pFuncJpW53RadarType(void);

uint8_t *p2pFuncJpW56RadarType(void);

void p2pFuncSetRadarDetectMode(uint8_t ucRadarDetectMode);

uint8_t p2pFuncGetRadarDetectMode(void);

void p2pFuncSetDfsState(uint8_t ucDfsState);

uint8_t p2pFuncGetDfsState(void);

uint8_t *p2pFuncShowDfsState(void);

uint8_t p2pFuncGetCsaBssIndex(void);

void p2pFuncSetCsaBssIndex(uint8_t ucBssIdx);

void p2pFuncRecordCacStartBootTime(void);

uint32_t p2pFuncGetCacRemainingTime(void);
#endif

void p2pFuncSetChannel(struct ADAPTER *prAdapter,
		uint8_t ucRoleIdx,
		struct RF_CHANNEL_INFO *prRfChannelInfo);

u_int8_t p2pFuncRetryJOIN(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct P2P_JOIN_INFO *prJoinInfo);

uint32_t
p2pFuncTxMgmtFrame(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct MSDU_INFO *prMgmtTxMsdu,
		u_int8_t fgNonCckRate);

uint32_t
p2pFuncBeaconUpdate(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		struct P2P_BEACON_UPDATE_INFO *prBcnUpdateInfo,
		uint8_t *pucNewBcnHdr,
		uint32_t u4NewHdrLen,
		uint8_t *pucNewBcnBody,
		uint32_t u4NewBodyLen);

uint32_t
p2pFuncAssocRespUpdate(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t *AssocRespIE,
		uint32_t u4AssocRespLen);

#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
uint32_t
p2pFuncProbeRespUpdate(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t *ProbeRespIE, uint32_t u4ProbeRespLen,
		enum ENUM_IE_UPD_METHOD eMethod);
#endif

u_int8_t
p2pFuncValidateAuth(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, struct SW_RFB *prSwRfb,
		struct STA_RECORD **pprStaRec, uint16_t *pu2StatusCode);

u_int8_t p2pFuncValidateAssocReq(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb, uint16_t *pu2StatusCode);

void p2pFuncResetStaRecStatus(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec);

void
p2pFuncInitConnectionSettings(struct ADAPTER *prAdapter,
		struct P2P_CONNECTION_SETTINGS *prP2PConnSettings,
		u_int8_t fgIsApMode);

u_int8_t p2pFuncParseCheckForP2PInfoElem(struct ADAPTER *prAdapter,
		uint8_t *pucBuf, uint8_t *pucOuiType);

u_int8_t p2pFuncParseCheckForTKIPInfoElem(uint8_t *pucBuf);

u_int8_t
p2pFuncValidateProbeReq(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb, uint32_t *pu4ControlFlags,
		u_int8_t fgIsDevInterface, uint8_t ucRoleIdx);

void p2pFuncValidateRxActionFrame(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		u_int8_t fgIsDevInterface, uint8_t ucRoleIdx);

u_int8_t p2pFuncIsAPMode(struct P2P_CONNECTION_SETTINGS *prP2pConnSettings);

u_int8_t p2pFuncIsDualAPMode(struct ADAPTER *prAdapter);

u_int8_t p2pFuncIsDualGOMode(struct ADAPTER *prAdapter);

void
p2pFuncParseBeaconContent(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t *pucIEInfo, uint32_t u4IELen);

struct BSS_DESC *
p2pFuncKeepOnConnection(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo);

void p2pFuncStoreAssocRspIEBuffer(struct ADAPTER *prAdapter,
		struct P2P_JOIN_INFO *prP2pJoinInfo,
		struct SW_RFB *prSwRfb);

void
p2pFuncMgmtFrameRegister(struct ADAPTER *prAdapter,
		uint16_t u2FrameType,
		u_int8_t fgIsRegistered,
		uint32_t *pu4P2pPacketFilter);

void p2pFuncUpdateMgmtFrameRegister(struct ADAPTER *prAdapter,
		uint32_t u4OsFilter);

void p2pFuncGetStationInfo(struct ADAPTER *prAdapter,
		uint8_t *pucMacAddr,
		struct P2P_STATION_INFO *prStaInfo);

struct MSDU_INFO *p2pFuncProcessP2pProbeRsp(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx, uint8_t fgNonTxLink, uint8_t fgHide,
	struct WLAN_BEACON_FRAME *prProbeRspFrame);

void
p2pFuncProcessP2pProbeRspAction(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMgmtTxMsdu,
		uint8_t ucBssIdx);

#if 0 /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0) */
uint32_t
p2pFuncCalculateExtra_IELenForBeacon(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void p2pFuncGenerateExtra_IEForBeacon(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);

#else
uint32_t p2pFuncCalculateP2p_IELenForBeacon(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void p2pFuncGenerateP2p_IEForBeacon(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);

uint32_t p2pFuncCalculateWSC_IELenForBeacon(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void p2pFuncGenerateWSC_IEForBeacon(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);
#endif
uint32_t
p2pFuncCalculateP2p_IELenForAssocRsp(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void p2pFuncGenerateP2p_IEForAssocRsp(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);

uint32_t
p2pFuncCalculateP2P_IELen(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec,
		struct APPEND_VAR_ATTRI_ENTRY arAppendAttriTable[],
		uint32_t u4AttriTableSize);

void
p2pFuncGenerateP2P_IE(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		u_int8_t fgIsAssocFrame,
		uint16_t *pu2Offset,
		uint8_t *pucBuf,
		uint16_t u2BufSize,
		struct APPEND_VAR_ATTRI_ENTRY arAppendAttriTable[],
		uint32_t u4AttriTableSize);

uint32_t
p2pFuncAppendAttriStatusForAssocRsp(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		u_int8_t fgIsAssocFrame,
		uint16_t *pu2Offset,
		uint8_t *pucBuf,
		uint16_t u2BufSize);

uint32_t
p2pFuncAppendAttriExtListenTiming(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		u_int8_t fgIsAssocFrame,
		uint16_t *pu2Offset,
		uint8_t *pucBuf,
		uint16_t u2BufSize);

void
p2pFuncDissolve(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		u_int8_t fgSendDeauth,
		uint16_t u2ReasonCode,
		u_int8_t fgIsLocallyGenerated);

struct IE_HDR *
p2pFuncGetSpecIE(struct ADAPTER *prAdapter,
		uint8_t *pucIEBuf,
		uint16_t u2BufferLen,
		uint8_t ucElemID,
		u_int8_t *pfgIsMore);

struct P2P_ATTRIBUTE *
p2pFuncGetSpecAttri(struct ADAPTER *prAdapter,
		uint8_t ucOuiType,
		uint8_t *pucIEBuf,
		uint16_t u2BufferLen,
		uint8_t ucAttriID);

uint32_t p2pCalculateWSCIELen(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec);

void p2pGenerateWSCIE(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);

#if CFG_SUPPORT_WFD
uint32_t p2pCalculateWFDIELen(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec);

void p2pGenerateWFDIE(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);
#endif

uint32_t p2pCalculateP2PIELen(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec);

void p2pGenerateP2PIE(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);

#if CFG_SUPPORT_CUSTOM_VENDOR_IE
uint32_t p2pCalculateVendorIELen(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec);

void p2pGenerateVendorIE(struct ADAPTER *p2pGenerateVendorIE,
		struct MSDU_INFO *prMsduInfo);
#endif

#if CFG_SUPPORT_WFD
uint32_t wfdFuncCalculateWfdIELenForAssocRsp(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void wfdFuncGenerateWfdIEForAssocRsp(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);
#endif

uint32_t p2pFuncCalculateP2P_IE_NoA(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx, struct STA_RECORD *prStaRec);

void p2pFuncGenerateP2P_IE_NoA(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);

void p2pFunCleanQueuedMgmtFrame(struct ADAPTER *prAdapter,
		struct P2P_QUEUED_ACTION_FRAME *prFrame);

void p2pFuncSwitchGcChannel(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo);

void p2pFuncSwitchSapChannel(struct ADAPTER *prAdapter);

uint32_t p2pFunGetPreferredFreqList(struct ADAPTER *prAdapter,
		enum ENUM_IFTYPE eIftype, uint32_t *freq_list,
		uint32_t *num_freq_list, u_int8_t fgIsSkipDfs);

enum ENUM_P2P_CONNECT_STATE
p2pFuncGetP2pActionFrameType(struct MSDU_INFO *prMgmtMsdu);

u_int8_t
p2pFuncCheckOnRocChnl(struct RF_CHANNEL_INFO *prTxChnl,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

u_int8_t
p2pFuncNeedWaitRsp(struct ADAPTER *prAdapter,
		enum ENUM_P2P_CONNECT_STATE eConnState);

u_int8_t
p2pFuncNeedForceSleep(struct ADAPTER *prAdapter);

void
p2pFunClearAllTxReq(struct ADAPTER *prAdapter,
		struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo);

uint8_t p2pFunGetAcsBestCh(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand,
		enum ENUM_MAX_BANDWIDTH_SETTING eChnlBw,
		uint32_t u4LteSafeChnMask_2G,
		uint32_t u4LteSafeChnMask_5G_1,
		uint32_t u4LteSafeChnMask_5G_2,
		uint32_t u4LteSafeChnMask_6G);
#if (CFG_SUPPORT_P2PGO_ACS == 1)

void p2pFunGetAcsBestChList(struct ADAPTER *prAdapter,
		uint8_t eBand,
		enum ENUM_MAX_BANDWIDTH_SETTING eChnlBw,
		uint32_t u4LteSafeChnMask_2G,
		uint32_t u4LteSafeChnMask_5G_1,
		uint32_t u4LteSafeChnMask_5G_2,
		uint32_t u4LteSafeChnMask_6G,
		uint8_t *pucSortChannelNumber,
		struct RF_CHANNEL_INFO *paucSortChannelList,
		u_int8_t fgNoDfs);
#endif
void p2pFunProcessAcsReport(struct ADAPTER *prAdapter,
		uint8_t ucRoleIndex,
		struct PARAM_GET_CHN_INFO *prLteSafeChnInfo,
		struct P2P_ACS_REQ_INFO *prAcsReqInfo);

void p2pFunIndicateAcsResult(struct GLUE_INFO *prGlueInfo,
		struct P2P_ACS_REQ_INFO *prAcsReqInfo);

void p2pFunCalAcsChnScores(struct ADAPTER *prAdapter);

uint8_t p2pFuncIsCsaBlockScan(struct ADAPTER *prAdapter);

enum ENUM_CHNL_SWITCH_POLICY
p2pFunDetermineChnlSwitchPolicy(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct RF_CHANNEL_INFO *prNewChannelInfo);

void
p2pFunNotifyChnlSwitch(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		enum ENUM_CHNL_SWITCH_POLICY ePolicy,
		struct RF_CHANNEL_INFO *prNewChannelInfo);

void
p2pFunChnlSwitchNotifyDone(struct ADAPTER *prAdapter);

uint8_t p2pFuncIsBufferableMMPDU(struct ADAPTER *prAdapter,
		enum ENUM_P2P_CONNECT_STATE eConnState,
		struct MSDU_INFO *prMgmtTxMsdu);

void p2pFuncSetAclPolicy(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	enum ENUM_PARAM_CUSTOM_ACL_POLICY ePolicy,
	uint8_t aucAddr[]);

#endif

#if CFG_AP_80211KVR_INTERFACE
void p2pFunMulAPAgentBssStatusNotification(
		struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);
#endif

struct BSS_INFO *p2pGetAisBssByBand(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand);

struct BSS_INFO *p2pGetAisConnectedBss(
	struct ADAPTER *prAdapter);

