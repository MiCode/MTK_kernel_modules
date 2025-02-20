/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _P2P_FUNC_H
#define _P2P_FUNC_H

#define P2P_OFF_CHNL_TX_DEFAULT_TIME_MS                      1000
#define GO_CSA_ACTION_FRAME_LIFE_TIME_MARGIN_MS		     50
#define GO_CSA_ACTION_FRAME_MINIMUM_LIFE_TIME_MS	     100

#if (CONFIG_BAND_NUM > 2)
#define AA_HW_BAND_NUM	CONFIG_BAND_NUM
#else
#define AA_HW_BAND_NUM	3
#endif

enum ENUM_AA_HW_BAND {
	AA_HW_BAND_0 = 0,
	AA_HW_BAND_1,
	AA_HW_BAND_2,
	AA_HW_BAND_3,
};

enum ENUM_CSA_STATUS {
	CSA_STATUS_SUCCESS = 0,
	CSA_STATUS_DFS_NOT_SUP,
	CSA_STATUS_NON_PSC_NOT_SUP,
	CSA_STATUS_NON_SAE_NOT_SUP,
	CSA_STATUS_PEER_NOT_SUP_CSA,
	CSA_STATUS_PEER_NOT_SUP_CH
};

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

enum ENUM_P2P_MGMT_TX_TYPE {
	P2P_MGMT_OFF_CH_TX = 0,
	P2P_MGMT_REMAIN_ON_CH_TX,
	P2P_MGMT_TX_TYPE_NUM
};

enum ENUM_P2P_CH_FILTER_TYPE {
	P2P_CROSS_BAND_STA_SCC_FILTER,
	P2P_REMOVE_DFS_CH_FILTER,
	P2P_BT_DESENSE_CH_FILTER,
	P2P_SKIP_BT_DESENSE_FILTER,
	P2P_ALIVE_BSS_SYNC_FILTER,
	P2P_RFBAND_CHECK_FILTER,
	P2P_DUAL_AP_CH_FILTER,
	P2P_DUAL_A_BAND_FILTER,
	P2P_USER_PREF_CH_FILTER,
	P2P_SET_DEFAULT_CH_FILTER,
	P2P_MAX_CH_FILTER_NUM
};

enum ENUM_P2P_FILTER_SCENARIO_TYPE {
	P2P_DEFAULT_SCENARIO,
	P2P_BT_COEX_SCENARIO,
	P2P_STA_SCC_ONLY_SCENARIO,
	P2P_MCC_SINGLE_SAP_SCENARIO,
	P2P_MCC_DUAL_SAP_SCENARIO,
	P2P_MAX_FILTER_SCENARIO_NUM
};

enum ENUM_P2P_FORCE_TRX_CONFIG {
	P2P_FORCE_TRX_CONFIG_NONE = 0,
	P2P_FORCE_TRX_CONFIG_MCS7,
	P2P_FORCE_TRX_CONFIG_MCS9
};

struct CCM_AA_FOBIDEN_REGION_UNIT;

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
	struct MSG_MGMT_TX_REQUEST *prMgmtTxMsg);

void
p2pFuncRemovePendingMgmtLinkEntry(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx, uint64_t u8Cookie);

uint32_t
p2pFuncIsPendingTxMgmtNeedWait(struct ADAPTER *prAdapter, uint8_t ucRoleIndex,
	enum ENUM_P2P_MGMT_TX_TYPE eP2pMgmtTxType);

void p2pFuncAcquireCh(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

void p2pFuncSetApNss(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		uint8_t ucOpTxNss,
		uint8_t ucOpRxNss);

void p2pFuncSetForceTrxConfig(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		uint8_t ucScenarioConfig);

uint8_t
p2pFuncGetForceTrxConfig(struct ADAPTER *prAdapter);

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
	uint8_t ucAvailable, uint8_t ucChannel,
	enum ENUM_MAX_BANDWIDTH_SETTING eBw);

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

void p2pFuncDisableManualCac(void);

uint32_t p2pFuncGetDriverCacTime(void);

u_int8_t p2pFuncIsManualCac(void);

void p2pFuncRadarInfoInit(void);

void p2pFuncShowRadarInfo(struct ADAPTER *prAdapter, uint8_t ucBssIdx);

void p2pFuncGetRadarInfo(struct P2P_RADAR_INFO *prP2pRadarInfo);

uint8_t *p2pFuncJpW53RadarType(void);

uint8_t *p2pFuncJpW56RadarType(void);

void p2pFuncSetRadarDetectMode(uint8_t ucRadarDetectMode);

uint8_t p2pFuncGetRadarDetectMode(void);

void p2pFuncAddRadarDetectCnt(void);

void p2pFuncRadarDetectCntUevent(struct ADAPTER *prAdapter);

void p2pFuncRadarDetectDoneUevent(struct ADAPTER *prAdapter);

void p2pFuncResetRadarDetectCnt(void);

uint8_t p2pFuncGetRadarDetectCnt(void);

void p2pFuncSetDfsState(uint8_t ucDfsState);

uint8_t p2pFuncGetDfsState(void);

const char *p2pFuncShowDfsState(void);

void p2pFuncRecordCacStartBootTime(void);

uint32_t p2pFuncGetCacRemainingTime(void);
#endif
uint8_t p2pFuncGetCsaBssIndex(void);

void p2pFuncSetCsaBssIndex(uint8_t ucBssIdx);

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

void p2pFuncParseMTKOuiInfoElem(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		uint8_t *pucIE);
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

u_int8_t p2pFuncIsDualAPActive(struct ADAPTER *prAdapter);

void
p2pFuncParseBeaconContent(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t aucBSSID[],
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

void p2pCrossBandStaSccFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario);

void p2pRemoveDfsChFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario);

void p2pUserPrefChFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario);

void p2pMccAliveBssSyncFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario);

void p2pSetDefaultFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario);

void p2pBtDesenseChFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario);

void p2pDualABandFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario);

void p2pRfBandCheckFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario);

void p2pDualApChFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario);

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

void p2pFuncSwitchChannelHelper(struct ADAPTER *prAdapter);

void p2pFuncSwitchChannel(struct ADAPTER *prAdapter,
			  struct BSS_INFO *prTargetBss, const char *pucSrcFunc);

u_int8_t p2pFuncSwitchGoChannel(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo,
			    uint32_t u4TargetCh,
			    enum ENUM_MBMC_BN eTargetHwBandIdx,
			    enum ENUM_BAND eTargetBand);

bool p2pFuncSwitchSapChannel(struct ADAPTER *prAdapter,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario);

void p2pFuncNotifySapStarted(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx);

#if CFG_SUPPORT_CCM
uint8_t p2pFuncSapFilteredChListGen(
		struct ADAPTER *prAdapter,
		struct RF_CHANNEL_INFO *prChnlList,
		uint8_t *prForbiddenListLen,
		struct CCM_AA_FOBIDEN_REGION_UNIT *prRegionOutput,
		uint16_t *prTargetBw);
#endif

uint8_t
p2pFunGetTopPreferFreqByBand(struct ADAPTER *prAdapter,
			     enum ENUM_BAND eBandPrefer,
			     uint8_t ucTopPreferNum, uint32_t *pu4Freq);

uint8_t p2pFuncGetFreqAllowList(struct ADAPTER *prAdapter,
			      uint32_t *pau4AllowFreqList);

uint8_t p2pFuncGetAllAcsFreqList(struct ADAPTER *prAdapter,
					uint32_t *ucChnlNum,
					struct RF_CHANNEL_INFO *arChnlList,
					uint32_t *pau4SafeFreqList);

uint8_t p2pFuncAppendPrefFreq(struct BSS_INFO **prBssList,
	uint8_t ucNumOfAliveBss, uint32_t *prFreqList);

#if CFG_SUPPORT_CCM
uint32_t p2pFuncAppendAaFreq(struct ADAPTER *prAdapter,
			     struct BSS_INFO *prBssInfo,
			     uint32_t *apu4FreqList);
#endif

uint32_t p2pFunGetPreferredFreqList(struct ADAPTER *prAdapter,
		enum ENUM_IFTYPE eIftype, uint32_t *pau4FreqList,
		uint32_t *pu4FreqListNum, uint32_t *pau4FreqAllowList,
		uint8_t ucAllowFreqNum, u_int8_t fgIsSkipDfs);

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
		uint32_t u4LteSafeChnMask_2G,
		uint32_t u4LteSafeChnMask_5G_1,
		uint32_t u4LteSafeChnMask_5G_2,
		uint32_t u4LteSafeChnMask_6G,
		uint8_t *pucSortChannelNumber,
		struct RF_CHANNEL_INFO *paucSortChannelList);
#endif
void p2pFunProcessAcsReport(struct ADAPTER *prAdapter,
		uint8_t ucRoleIndex,
		struct P2P_ACS_REQ_INFO *prAcsReqInfo);

void p2pFunIndicateAcsResult(struct GLUE_INFO *prGlueInfo,
		struct P2P_ACS_REQ_INFO *prAcsReqInfo);

void p2pFunCalAcsChnScores(struct ADAPTER *prAdapter);
#if CFG_ENABLE_CSA_BLOCK_SCAN
uint8_t p2pFuncIsCsaBlockScan(struct ADAPTER *prAdapter);
#endif
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

uint32_t p2pFuncStoreFilsInfo(struct ADAPTER *prAdapter,
	struct MSG_P2P_FILS_DISCOVERY_UPDATE *prMsg,
	uint8_t ucBssIndex);

void p2pFuncClearFilsInfo(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

uint32_t p2pFuncStoreUnsolProbeInfo(struct ADAPTER *prAdapter,
	struct MSG_P2P_UNSOL_PROBE_UPDATE *prMsg,
	uint8_t ucBssIndex);

void p2pFuncClearUnsolProbeInfo(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

uint32_t p2pFuncCalculateP2p_IELenForOwe(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void p2pFuncGenerateP2p_IEForOwe(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);

typedef void(*PFN_P2P_CH_CANDIDATE_FILETER_FUNC) (struct ADAPTER *,
		uint8_t *,
		struct P2P_CH_SWITCH_CANDIDATE *,
		struct BSS_INFO *,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE);

struct P2P_CH_CANDIDATE_FILETER_ENTRY {
		enum ENUM_P2P_CH_FILTER_TYPE eP2pChFilterType;
		PFN_P2P_CH_CANDIDATE_FILETER_FUNC pfnChCandFilter;
};
u_int8_t p2pFuncIsLteSafeChnl(enum ENUM_BAND eBand, uint8_t ucChnlNum,
				 uint32_t *pau4SafeChnl);

#if CFG_SUPPORT_CCM
u_int8_t p2pFuncIsPreferWfdAa(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prCsaBss);
#endif

enum ENUM_CSA_STATUS p2pFuncIsCsaAllowed(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo,
			    uint32_t u4TargetCh,
			    enum ENUM_BAND eTargetBand);
