/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
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

void p2pFuncRequestScan(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct P2P_SCAN_REQ_INFO *prScanReqInfo);

void p2pFuncCancelScan(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct P2P_SCAN_REQ_INFO *prScanReqInfo);

void
p2pFuncUpdateBssInfoForJOIN(IN struct ADAPTER *prAdapter,
		IN struct BSS_DESC *prBssDesc,
		IN struct STA_RECORD *prStaRec,
		IN struct BSS_INFO *prP2pBssInfo,
		IN struct SW_RFB *prAssocRspSwRfb);

void p2pFuncAcquireCh(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

void
p2pFuncDisconnect(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN struct STA_RECORD *prStaRec,
		IN u_int8_t fgSendDeauth,
		IN uint16_t u2ReasonCode);

struct BSS_INFO *p2pFuncBSSIDFindBssInfo(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucBSSID);

void p2pFuncGCJoin(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN struct P2P_JOIN_INFO *prP2pJoinInfo);

void p2pFuncStopComplete(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo);

void
p2pFuncStartGO(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo,
		IN struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo,
		IN struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo);

void p2pFuncStopGO(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo);

uint32_t p2pFuncRoleToBssIdx(IN struct ADAPTER *prAdapter,
		IN uint8_t ucRoleIdx,
		OUT uint8_t *pucBssIdx);

struct P2P_ROLE_FSM_INFO *p2pFuncGetRoleByBssIdx(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex);

void
p2pFuncSwitchOPMode(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN enum ENUM_OP_MODE eOpMode,
		IN u_int8_t fgSyncToFW);

void p2pFuncReleaseCh(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

#if (CFG_SUPPORT_DFS_MASTER == 1)
void p2pFuncStartRdd(IN struct ADAPTER *prAdapter, IN uint8_t ucBssIdx);

void p2pFuncStopRdd(IN struct ADAPTER *prAdapter, IN uint8_t ucBssIdx);

void p2pFuncDfsSwitchCh(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo,
		IN struct P2P_CHNL_REQ_INFO rP2pChnlReqInfo);

u_int8_t p2pFuncCheckWeatherRadarBand(
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

int32_t p2pFuncSetDriverCacTime(IN uint32_t u4CacTime);

void p2pFuncEnableManualCac(void);

uint32_t p2pFuncGetDriverCacTime(void);

u_int8_t p2pFuncIsManualCac(void);

void p2pFuncRadarInfoInit(void);

void p2pFuncShowRadarInfo(IN struct ADAPTER *prAdapter, IN uint8_t ucBssIdx);

void p2pFuncGetRadarInfo(IN struct P2P_RADAR_INFO *prP2pRadarInfo);

uint8_t *p2pFuncJpW53RadarType(void);

uint8_t *p2pFuncJpW56RadarType(void);

void p2pFuncSetRadarDetectMode(IN uint8_t ucRadarDetectMode);

uint8_t p2pFuncGetRadarDetectMode(void);

void p2pFuncSetDfsState(IN uint8_t ucDfsState);

uint8_t p2pFuncGetDfsState(void);

uint8_t *p2pFuncShowDfsState(void);

void p2pFuncRecordCacStartBootTime(void);

uint32_t p2pFuncGetCacRemainingTime(void);
#endif

void p2pFuncSetChannel(IN struct ADAPTER *prAdapter,
		IN uint8_t ucRoleIdx,
		IN struct RF_CHANNEL_INFO *prRfChannelInfo);

u_int8_t p2pFuncRetryJOIN(IN struct ADAPTER *prAdapter,
		IN struct STA_RECORD *prStaRec,
		IN struct P2P_JOIN_INFO *prJoinInfo);

uint32_t
p2pFuncTxMgmtFrame(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct MSDU_INFO *prMgmtTxMsdu,
		IN u_int8_t fgNonCckRate);

uint32_t
p2pFuncBeaconUpdate(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN struct P2P_BEACON_UPDATE_INFO *prBcnUpdateInfo,
		IN uint8_t *pucNewBcnHdr,
		IN uint32_t u4NewHdrLen,
		IN uint8_t *pucNewBcnBody,
		IN uint32_t u4NewBodyLen);

uint32_t
p2pFuncAssocRespUpdate(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN uint8_t *AssocRespIE,
		IN uint32_t u4AssocRespLen);

#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
uint32_t
p2pFuncProbeRespUpdate(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN uint8_t *ProbeRespIE, IN uint32_t u4ProbeRespLen);
#endif

u_int8_t
p2pFuncValidateAuth(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo, IN struct SW_RFB *prSwRfb,
		IN struct STA_RECORD **pprStaRec, OUT uint16_t *pu2StatusCode);

u_int8_t p2pFuncValidateAssocReq(IN struct ADAPTER *prAdapter,
		IN struct SW_RFB *prSwRfb, OUT uint16_t *pu2StatusCode);

void p2pFuncResetStaRecStatus(IN struct ADAPTER *prAdapter,
		IN struct STA_RECORD *prStaRec);

void
p2pFuncInitConnectionSettings(IN struct ADAPTER *prAdapter,
		IN struct P2P_CONNECTION_SETTINGS *prP2PConnSettings,
		IN u_int8_t fgIsApMode);

u_int8_t p2pFuncParseCheckForP2PInfoElem(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucBuf, OUT uint8_t *pucOuiType);

u_int8_t p2pFuncParseCheckForTKIPInfoElem(IN uint8_t *pucBuf);

u_int8_t
p2pFuncValidateProbeReq(IN struct ADAPTER *prAdapter,
		IN struct SW_RFB *prSwRfb, OUT uint32_t *pu4ControlFlags,
		IN u_int8_t fgIsDevInterface, IN uint8_t ucRoleIdx);

void p2pFuncValidateRxActionFrame(IN struct ADAPTER *prAdapter,
		IN struct SW_RFB *prSwRfb,
		IN u_int8_t fgIsDevInterface, IN uint8_t ucRoleIdx);

u_int8_t p2pFuncIsAPMode(IN struct P2P_CONNECTION_SETTINGS *prP2pConnSettings);

void
p2pFuncParseBeaconContent(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN uint8_t *pucIEInfo, IN uint32_t u4IELen);

struct BSS_DESC *
p2pFuncKeepOnConnection(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo,
		IN struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		IN struct P2P_SCAN_REQ_INFO *prScanReqInfo);

void p2pFuncStoreAssocRspIEBuffer(IN struct ADAPTER *prAdapter,
		IN struct P2P_JOIN_INFO *prP2pJoinInfo,
		IN struct SW_RFB *prSwRfb);

void
p2pFuncMgmtFrameRegister(IN struct ADAPTER *prAdapter,
		IN uint16_t u2FrameType,
		IN u_int8_t fgIsRegistered,
		OUT uint32_t *pu4P2pPacketFilter);

void p2pFuncUpdateMgmtFrameRegister(IN struct ADAPTER *prAdapter,
		IN uint32_t u4OsFilter);

void p2pFuncGetStationInfo(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucMacAddr,
		OUT struct P2P_STATION_INFO *prStaInfo);

struct MSDU_INFO *p2pFuncProcessP2pProbeRsp(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx, IN struct MSDU_INFO *prMgmtTxMsdu);

#if 0 /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0) */
uint32_t
p2pFuncCalculateExtra_IELenForBeacon(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex, IN struct STA_RECORD *prStaRec);

void p2pFuncGenerateExtra_IEForBeacon(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo);

#else
uint32_t p2pFuncCalculateP2p_IELenForBeacon(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex, IN struct STA_RECORD *prStaRec);

void p2pFuncGenerateP2p_IEForBeacon(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo);

uint32_t p2pFuncCalculateWSC_IELenForBeacon(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex, IN struct STA_RECORD *prStaRec);

void p2pFuncGenerateWSC_IEForBeacon(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo);
#endif
uint32_t
p2pFuncCalculateP2p_IELenForAssocRsp(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex, IN struct STA_RECORD *prStaRec);

void p2pFuncGenerateP2p_IEForAssocRsp(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo);

uint32_t
p2pFuncCalculateP2P_IELen(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct STA_RECORD *prStaRec,
		IN struct APPEND_VAR_ATTRI_ENTRY arAppendAttriTable[],
		IN uint32_t u4AttriTableSize);

void
p2pFuncGenerateP2P_IE(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN u_int8_t fgIsAssocFrame,
		IN uint16_t *pu2Offset,
		IN uint8_t *pucBuf,
		IN uint16_t u2BufSize,
		IN struct APPEND_VAR_ATTRI_ENTRY arAppendAttriTable[],
		IN uint32_t u4AttriTableSize);

uint32_t
p2pFuncAppendAttriStatusForAssocRsp(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN u_int8_t fgIsAssocFrame,
		IN uint16_t *pu2Offset,
		IN uint8_t *pucBuf,
		IN uint16_t u2BufSize);

uint32_t
p2pFuncAppendAttriExtListenTiming(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN u_int8_t fgIsAssocFrame,
		IN uint16_t *pu2Offset,
		IN uint8_t *pucBuf,
		IN uint16_t u2BufSize);

void
p2pFuncDissolve(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN u_int8_t fgSendDeauth,
		IN uint16_t u2ReasonCode);

struct IE_HDR *
p2pFuncGetSpecIE(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucIEBuf,
		IN uint16_t u2BufferLen,
		IN uint8_t ucElemID,
		IN u_int8_t *pfgIsMore);

struct P2P_ATTRIBUTE *
p2pFuncGetSpecAttri(IN struct ADAPTER *prAdapter,
		IN uint8_t ucOuiType,
		IN uint8_t *pucIEBuf,
		IN uint16_t u2BufferLen,
		IN uint8_t ucAttriID);

uint32_t wfdFuncCalculateWfdIELenForAssocRsp(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex, IN struct STA_RECORD *prStaRec);

void wfdFuncGenerateWfdIEForAssocRsp(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo);

uint32_t p2pFuncCalculateP2P_IE_NoA(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx, IN struct STA_RECORD *prStaRec);

void p2pFuncGenerateP2P_IE_NoA(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo);

void p2pFunCleanQueuedMgmtFrame(IN struct ADAPTER *prAdapter,
		IN struct P2P_QUEUED_ACTION_FRAME *prFrame);

void p2pFuncSwitchSapChannel(IN struct ADAPTER *prAdapter);

uint32_t p2pFunGetPreferredFreqList(IN struct ADAPTER *prAdapter,
		IN enum ENUM_IFTYPE eIftype, OUT uint32_t *freq_list,
		OUT uint32_t *num_freq_list);

enum ENUM_P2P_CONNECT_STATE
p2pFuncGetP2pActionFrameType(IN struct MSDU_INFO *prMgmtMsdu);

u_int8_t
p2pFuncCheckOnRocChnl(IN struct RF_CHANNEL_INFO *prTxChnl,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

u_int8_t
p2pFuncNeedWaitRsp(IN struct ADAPTER *prAdapter,
		IN enum ENUM_P2P_CONNECT_STATE eConnState);

void
p2pFunClearAllTxReq(IN struct ADAPTER *prAdapter,
		IN struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo);

uint8_t p2pFunGetAcsBestCh(IN struct ADAPTER *prAdapter,
		IN enum ENUM_BAND eBand,
		IN enum ENUM_MAX_BANDWIDTH_SETTING eChnlBw,
		IN uint32_t u4LteSafeChnMask_2G,
		IN uint32_t u4LteSafeChnMask_5G_1,
		IN uint32_t u4LteSafeChnMask_5G_2);

#if (CFG_SUPPORT_P2PGO_ACS == 1 ||\
	CFG_SUPPORT_P2P_CSA_ACS == 1)
void p2pFunGetAcsBestChList(IN struct ADAPTER *prAdapter,
		IN uint8_t eBand,
		IN enum ENUM_MAX_BANDWIDTH_SETTING eChnlBw,
		IN uint32_t u4LteSafeChnMask_2G,
		IN uint32_t u4LteSafeChnMask_5G_1,
		IN uint32_t u4LteSafeChnMask_5G_2,
		OUT uint8_t *pucSortChannelNumber,
		OUT struct RF_CHANNEL_INFO *paucSortChannelList);
#endif

void p2pFunProcessAcsReport(IN struct ADAPTER *prAdapter,
		IN uint8_t ucRoleIndex,
		IN struct PARAM_GET_CHN_INFO *prLteSafeChnInfo,
		IN struct P2P_ACS_REQ_INFO *prAcsReqInfo);

void p2pFunIndicateAcsResult(IN struct GLUE_INFO *prGlueInfo,
		IN struct P2P_ACS_REQ_INFO *prAcsReqInfo);

void p2pFunCalAcsChnScores(IN struct ADAPTER *prAdapter);

enum ENUM_CHNL_SWITCH_POLICY
p2pFunDetermineChnlSwitchPolicy(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		IN struct RF_CHANNEL_INFO *prNewChannelInfo);

void
p2pFunNotifyChnlSwitch(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		enum ENUM_CHNL_SWITCH_POLICY ePolicy,
		IN struct RF_CHANNEL_INFO *prNewChannelInfo);

#if(CFG_SUPPORT_DFS_MASTER == 1)
void
p2pFunChnlSwitchNotifyDone(IN struct ADAPTER *prAdapter);
#endif

uint8_t p2pFuncIsBufferableMMPDU(IN struct ADAPTER *prAdapter,
		IN enum ENUM_P2P_CONNECT_STATE eConnState,
		IN struct MSDU_INFO *prMgmtTxMsdu);

#endif

#if (CFG_SUPPORT_P2P_CSA_ACS == 1)
void p2pFuncSetCsaChnlScanReq(IN struct ADAPTER *prAdapter);
#endif

#if CFG_AP_80211KVR_INTERFACE
void p2pFunMulAPAgentBssStatusNotification(
		IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo);
#endif

#if (CFG_SUPPORT_P2P_CSA == 1)
void p2pFuncSwitchGcChannel(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo);
#endif

#if CFG_POWER_OFF_CTRL_SUPPORT
void p2pFuncPreReboot(void);
#endif
