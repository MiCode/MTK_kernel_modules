/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _P2P_ROLE_H
#define _P2P_ROLE_H

#define P2P_ROLE_GET_STATISTICS_TIME	    5000

#ifdef CFG_AP_GO_DELAY_CARRIER_ON
#define AP_GO_DELAY_CARRIER_ON_TIMEOUT_MS	50
#endif /* CFG_AP_GO_DELAY_CARRIER_ON */

enum ENUM_BUFFER_TYPE {
	ENUM_FRAME_TYPE_EXTRA_IE_BEACON,
	ENUM_FRAME_TYPE_EXTRA_IE_ASSOC_RSP,
	ENUM_FRAME_TYPE_EXTRA_IE_PROBE_RSP,
	ENUM_FRAME_TYPE_PROBE_RSP_TEMPLATE,
	ENUM_FRAME_TYPE_BEACON_TEMPLATE,
	ENUM_FRAME_IE_NUM
};

enum ENUM_HIDDEN_SSID_TYPE {
	ENUM_HIDDEN_SSID_NONE,
	ENUM_HIDDEN_SSID_ZERO_LEN,
	ENUM_HIDDEN_SSID_ZERO_CONTENT,
	ENUM_HIDDEN_SSID_NUM
};

struct P2P_BEACON_UPDATE_INFO {
	uint8_t *pucBcnHdr;
	uint32_t u4BcnHdrLen;
	uint8_t *pucBcnBody;
	uint32_t u4BcnBodyLen;
};

struct P2P_PROBE_RSP_UPDATE_INFO {
	struct MSDU_INFO *prProbeRspMsduTemplate;
};

struct P2P_ASSOC_RSP_UPDATE_INFO {
	uint8_t *pucAssocRspExtIE;
	uint16_t u2AssocIELen;
};

struct AP_CRYPTO_SETTINGS {
	uint32_t u4WpaVersion;
	uint32_t u4CipherGroup;
	int32_t i4NumOfCiphers;
	uint32_t aucCiphersPairwise[5];
	int32_t i4NumOfAkmSuites;
	uint32_t aucAkmSuites[2];
	u_int8_t fgIsControlPort;
	uint16_t u2ControlPortBE;
	u_int8_t fgIsControlPortEncrypt;
};

/* ////////////////////////// Message ////////////////////////////////// */

struct MSG_P2P_BEACON_UPDATE {
	struct MSG_HDR rMsgHdr;
	uint8_t ucRoleIndex;
	uint32_t u4BcnHdrLen;
	uint32_t u4BcnBodyLen;
	uint32_t u4AssocRespLen;
#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
	uint32_t u4ProbeRespLen;
#endif
	uint8_t *pucBcnHdr;
	uint8_t *pucBcnBody;
	uint8_t *pucAssocRespIE;
#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
	uint8_t *pucProbeRespIE;
#endif
	u_int8_t fgIsWepCipher;
	uint8_t aucBuffer[];	/* Header & Body & Extra IEs are put here. */
};

struct MSG_P2P_MGMT_FRAME_UPDATE {
	struct MSG_HDR rMsgHdr;
	enum ENUM_BUFFER_TYPE eBufferType;
	uint32_t u4BufferLen;
	uint8_t aucBuffer[];
};

struct MSG_P2P_SWITCH_OP_MODE {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	enum ENUM_OP_MODE eOpMode;
	uint8_t ucRoleIdx;
	enum ENUM_IFTYPE eIftype;
};

struct MSG_P2P_MGMT_FRAME_REGISTER {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint16_t u2FrameType;
	u_int8_t fgIsRegister;
};

struct MSG_P2P_CHNL_ABORT {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint64_t u8Cookie;
};

struct MSG_P2P_CONNECTION_REQUEST {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucRoleIdx;
	uint32_t u4ConnFlags;
	struct P2P_SSID_STRUCT rSsid;
	uint8_t aucBssid[MAC_ADDR_LEN];
	uint8_t aucSrcMacAddr[MAC_ADDR_LEN];
	enum ENUM_CHNL_EXT eChnlSco;
	struct RF_CHANNEL_INFO rChannelInfo;
	enum ENUM_PARAM_AUTH_MODE eAuthMode;
	uint32_t u4IELen;
	uint8_t aucIEBuf[];
};

struct MSG_P2P_CONNECTION_ABORT {
	struct MSG_HDR rMsgHdr;	/* Must be the first member. */
	uint8_t ucRoleIdx;
	uint8_t aucTargetID[MAC_ADDR_LEN];
	uint16_t u2ReasonCode;
	u_int8_t fgSendDeauth;
};

struct MSG_P2P_FILS_DISCOVERY_UPDATE {
	uint32_t u4MinInterval;
	uint32_t u4MaxInterval;
	uint32_t u4Length;
	uint8_t *prBuffer;
};

struct MSG_P2P_UNSOL_PROBE_UPDATE {
	uint32_t u4Interval;
	uint32_t u4Length;
	uint8_t *prBuffer;
};

struct MSG_P2P_START_AP {
	struct MSG_HDR rMsgHdr;
	uint32_t u4DtimPeriod;
	uint32_t u4BcnInterval;
	uint8_t aucSsid[32];
	uint16_t u2SsidLen;
	uint8_t ucHiddenSsidType;
	u_int8_t fgIsPrivacy;
	uint8_t ucRoleIdx;
	struct AP_CRYPTO_SETTINGS rEncryptionSettings;
	int32_t i4InactiveTimeout;
	struct MSG_P2P_FILS_DISCOVERY_UPDATE rFilsDiscovery;
	struct MSG_P2P_UNSOL_PROBE_UPDATE rUnsolProbe;
};

struct MSG_P2P_STOP_AP {
	struct MSG_HDR rMsgHdr;
	uint8_t ucRoleIdx;
};

#if (CFG_SUPPORT_DFS_MASTER == 1)
struct MSG_P2P_DFS_CAC {
	struct MSG_HDR rMsgHdr;
	enum ENUM_CHANNEL_WIDTH eChannelWidth;
	uint8_t ucRoleIdx;
};

struct MSG_P2P_RADAR_DETECT {
	struct MSG_HDR rMsgHdr;
	uint8_t ucBssIndex;
};

struct P2P_RADAR_INFO {
	uint8_t u1RddIdx;
	uint8_t u1LongDetected;
	uint8_t u1ConstantPRFDetected;
	uint8_t u1StaggeredPRFDetected;
	uint8_t u1RadarTypeIdx;
	uint8_t u1PeriodicPulseNum;
	uint8_t u1LongPulseNum;
	uint8_t u1HwPulseNum;
	uint8_t u1OutLPN;	 /* Long Pulse Number */
	uint8_t u1OutSPN;	 /* Short Pulse Number */
	uint8_t u1OutCRPN;
	uint8_t u1OutCRPW;	 /* Constant PRF Radar: Pulse Number */
	uint8_t u1OutCRBN;	 /* Constant PRF Radar: Burst Number */
	uint8_t u1OutSTGPN;  /* Staggered PRF radar: Staggered pulse number */
	uint8_t u1OutSTGPW;  /* Staggered PRF radar: maximum pulse width */
	uint8_t u1Reserve;
	uint32_t u4OutPRI_CONST;
	uint32_t u4OutPRI_STG1;
	uint32_t u4OutPRI_STG2;
	uint32_t u4OutPRI_STG3;
	uint32_t u4OutPRIStgDmin;
	/* Staggered PRF radar: min PRI Difference between 1st and 2nd  */
	struct LONG_PULSE_BUFFER arLongPulse[32];
	struct PERIODIC_PULSE_BUFFER arPeriodicPulse[32];
	struct WH_RDD_PULSE_CONTENT arContent[32];
};

#endif

struct MSG_P2P_SET_NEW_CHANNEL {
	struct MSG_HDR rMsgHdr;
	struct RF_CHANNEL_INFO rRfChannelInfo;
	uint8_t ucRoleIdx;
	uint8_t ucBssIndex;
};

struct MSG_P2P_CSA_DONE {
	struct MSG_HDR rMsgHdr;
	uint8_t ucBssIndex;
};

struct MSG_P2P_DEL_IFACE {
	struct MSG_HDR rMsgHdr;
	uint8_t ucRoleIdx;
	enum ENUM_IFTYPE eIftype;
};

struct P2P_STATION_INFO {
	uint32_t u4InactiveTime;
	uint32_t u4RxBytes;	/* TODO: */
	uint32_t u4TxBytes;	/* TODO: */
	uint32_t u4RxPackets;	/* TODO: */
	uint32_t u4TxPackets;	/* TODO: */
	/* TODO: Add more for requirement. */
};

struct MSG_P2P_ADD_MLD_LINK {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucRoleIdx;
	uint8_t ucLinkIdx;
	uint8_t aucMldAddr[MAC_ADDR_LEN];
	uint8_t aucLinkAddr[MAC_ADDR_LEN];
};

struct MSG_P2P_DEL_MLD_LINK {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucRoleIdx;
	uint8_t ucLinkIdx;
};

/* 3  --------------- WFA P2P Attributes Handler prototype --------------- */
typedef uint32_t(*PFN_APPEND_ATTRI_FUNC) (struct ADAPTER *,
		uint8_t, u_int8_t, uint16_t *, uint8_t *, uint16_t);

typedef uint32_t(*PFN_CALCULATE_VAR_ATTRI_LEN_FUNC) (struct ADAPTER *,
		struct STA_RECORD *);

struct APPEND_VAR_ATTRI_ENTRY {
	uint16_t u2EstimatedFixedAttriLen;	/* For fixed length */
	PFN_CALCULATE_VAR_ATTRI_LEN_FUNC pfnCalculateVariableAttriLen;
	PFN_APPEND_ATTRI_FUNC pfnAppendAttri;
};

/* //////////////////////////////////////////////////////////////// */

enum ENUM_P2P_ROLE_STATE {
	P2P_ROLE_STATE_IDLE = 0,
	P2P_ROLE_STATE_SCAN,
	P2P_ROLE_STATE_REQING_CHANNEL,
	P2P_ROLE_STATE_AP_CHNL_DETECTION,
	/* Requesting Channel to Send Specific Frame. */
	P2P_ROLE_STATE_GC_JOIN,
	P2P_ROLE_STATE_OFF_CHNL_TX,
#if (CFG_SUPPORT_DFS_MASTER == 1)
	P2P_ROLE_STATE_DFS_CAC,
#endif
	P2P_ROLE_STATE_SWITCH_CHANNEL,
	P2P_ROLE_STATE_WAIT_FOR_NEXT_REQ_CHNL,
	P2P_ROLE_STATE_NUM
};

enum ENUM_P2P_CONNECTION_TYPE {
	P2P_CONNECTION_TYPE_IDLE = 0,
	P2P_CONNECTION_TYPE_GO,
	P2P_CONNECTION_TYPE_GC,
	P2P_CONNECTION_TYPE_PURE_AP,
	P2P_CONNECTION_TYPE_NUM
};

struct P2P_JOIN_INFO {
	uint8_t ucSeqNumOfReqMsg;
	uint8_t ucAvailableAuthTypes;
	uint32_t u4ConnFlags;
	struct STA_RECORD *prTargetStaRec;
	struct BSS_DESC *prTargetBssDesc;
	uint8_t fgIsJoinSuccess;
	/* For ASSOC Rsp. */
	uint32_t u4BufLength;
	uint8_t aucIEBuf[MAX_IE_LENGTH];
};

/* For STA & AP mode. */
struct P2P_CONNECTION_REQ_INFO {
	enum ENUM_P2P_CONNECTION_TYPE eConnRequest;
	struct P2P_SSID_STRUCT rSsidStruct;
	uint8_t aucBssid[MAC_ADDR_LEN];

	/* AP preferred channel. */
	struct RF_CHANNEL_INFO rChannelInfo;
	enum ENUM_CHNL_EXT eChnlExt;

	/* To record channel bandwidth from CFG80211 */
	enum ENUM_MAX_BANDWIDTH_SETTING eChnlBw;

	/* To record primary channel frequency (MHz) from CFG80211 */
	uint16_t u2PriChnlFreq;

	/* To record Channel Center Frequency Segment 0 (MHz) from CFG80211 */
	uint32_t u4CenterFreq1;

	/* To record Channel Center Frequency Segment 1 (MHz) from CFG80211 */
	uint32_t u4CenterFreq2;

	/* For ASSOC Req. */
	uint32_t u4BufLength;
	uint8_t aucIEBuf[MAX_IE_LENGTH];

	struct MSG_P2P_START_AP rMsgStartAp;
};

#define P2P_ROLE_INDEX_2_ROLE_FSM_INFO(_prAdapter, _RoleIndex) \
	((_prAdapter)->rWifiVar.aprP2pRoleFsmInfo[_RoleIndex])

#define P2P_MAIN_LINK_INDEX (0)
#define P2P_MAIN_ROLE_INDEX (0)

struct P2P_ROLE_FSM_INFO {
	uint8_t ucRoleIndex;

	uint8_t ucBssIndex;

	uint8_t fgIsChannelSelectByAcs;

	/* State related. */
	enum ENUM_P2P_ROLE_STATE eCurrentState;

	/* Channel related. */
	struct P2P_CHNL_REQ_INFO rChnlReqInfo[MLD_LINK_MAX];

	/* Scan related. */
	struct P2P_SCAN_REQ_INFO rScanReqInfo;

	/* Mgmt tx related. */
	struct P2P_MGMT_TX_REQ_INFO rMgmtTxInfo;

	/* Auto channel selection related. */
	struct P2P_ACS_REQ_INFO rAcsReqInfo;

	/* FSM Timer */
	struct TIMER rP2pRoleFsmTimeoutTimer;

#if	CFG_ENABLE_PER_STA_STATISTICS_LOG
	/* Get statistics Timer */
	struct TIMER rP2pRoleFsmGetStatisticsTimer;
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
	struct TIMER rDfsShutDownTimer;
	enum ENUM_MAX_BANDWIDTH_SETTING eDfsChnlBw;
#endif

	struct TIMER rWaitNextReqChnlTimer;

	/* Packet filter for P2P module. */
	uint32_t u4P2pPacketFilter;

	/* GC Join related. */
	struct P2P_JOIN_INFO rJoinInfo;

	/* Connection related. */
	struct P2P_CONNECTION_REQ_INFO rConnReqInfo;

	/* Beacon Information. */
	struct P2P_BEACON_UPDATE_INFO rBeaconUpdateInfo;

	struct P2P_LINK_INFO aprP2pLinkInfo[MLD_LINK_MAX];
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prP2pMldBssInfo;
#endif
};

/*========================= Initial ============================*/

uint8_t p2pRoleFsmInit(struct ADAPTER *prAdapter,
	uint8_t ucRoleIdx, uint8_t ucGroupMldId,
	uint8_t aucIntfMac[]);

u_int8_t p2pRoleFsmNeedMlo(
	struct ADAPTER *prAdapter,
	uint8_t ucRoleIdx);

void p2pRoleFsmUninit(struct ADAPTER *prAdapter, uint8_t ucRoleIdx);

struct BSS_INFO *p2pRoleFsmInitLink(struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t aucMacAddr[],
	uint8_t ucGroupMldId,
	uint8_t ucLinkIdx);

void p2pRoleFsmUninitLink(struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	struct BSS_INFO *prP2pBssInfo);

void p2pRoleFsmDelIface(
	struct ADAPTER *prAdapter,
	uint8_t ucRoleIdx);

void p2pRoleFsmDelIfaceDone(
	struct ADAPTER *prAdapter,
	uint8_t ucRoleIdx);

/*================== Message Event ==================*/

void p2pRoleFsmRunEventAbort(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);

void p2pRoleFsmRunEventPreStartAP(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventStartAP(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventDelIface(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventStopAP(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

#if (CFG_SUPPORT_DFS_MASTER == 1)
void p2pRoleFsmRunEventDfsCac(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventStartCac(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventStopCac(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventRadarDet(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventDfsShutDownTimeout(struct ADAPTER *prAdapter,
		uintptr_t ulParamPtr);
#endif

void p2pRoleFsmRunEventSetNewChannel(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventCsaDone(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventWaitNextReqChnlTimeout(struct ADAPTER *prAdapter,
		uintptr_t ulParamPtr);

void p2pRoleFsmRunEventScanRequest(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void
p2pRoleFsmRunEventScanDone(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);

void p2pRoleFsmRunEventJoinComplete(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventTimeout(struct ADAPTER *prAdapter,
		uintptr_t ulParamPtr);

void p2pRoleFsmDeauthTimeout(struct ADAPTER *prAdapter,
		uintptr_t ulParamPtr);

void p2pRoleFsmRunEventBeaconTimeout(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo);

void p2pRoleFsmRunEventAgingTimeout(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec);

void p2pRoleUpdateACLEntry(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx);

u_int8_t p2pRoleProcessACLInspection(struct ADAPTER *prAdapter,
		uint8_t *pMacAddr, uint8_t ucBssIdx);

uint32_t
p2pRoleFsmRunEventAAACompleteImpl(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct BSS_INFO *prP2pBssInfo);

uint32_t
p2pRoleFsmRunEventAAASuccessImpl(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct BSS_INFO *prP2pBssInfo);

void p2pRoleFsmRunEventAAATxFailImpl(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct BSS_INFO *prP2pBssInfo);

uint32_t
p2pRoleFsmRunEventAAAComplete(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct BSS_INFO *prP2pBssInfo);

uint32_t
p2pRoleFsmRunEventAAASuccess(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct BSS_INFO *prP2pBssInfo,
		struct MSDU_INFO *prMsduInfo);

void p2pRoleFsmRunEventAAATxFail(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct BSS_INFO *prP2pBssInfo);

void p2pRoleFsmRunEventConnectionRequest(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventConnectionAbort(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void
p2pRoleFsmRunEventChnlGrant(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);

uint32_t
p2pRoleFsmRunEventDeauthTxDone(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		enum ENUM_TX_RESULT_CODE rTxDoneStatus);

void p2pRoleFsmRunEventRxDeauthentication(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec, struct SW_RFB *prSwRfb);

void p2pRoleFsmRunEventRxDisassociation(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec, struct SW_RFB *prSwRfb);

/* //////////////////////// TO BE REFINE ///////////////////// */
void p2pRoleFsmRunEventSwitchOPMode(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmReInitBeaconAll(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventBeaconUpdate(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventDissolve(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void
p2pProcessEvent_UpdateNOAParam(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct EVENT_UPDATE_NOA_PARAMS *prEventUpdateNoaParam);

#if	CFG_ENABLE_PER_STA_STATISTICS_LOG
void p2pRoleFsmGetStaStatistics(struct ADAPTER *prAdapter,
		uintptr_t ulParamPtr);
#endif

void p2pRoleFsmNotifyEapolTxStatus(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		enum ENUM_EAPOL_KEY_TYPE_T rEapolKeyType,
		enum ENUM_TX_RESULT_CODE rTxDoneStatus);

void p2pRoleFsmNotifyDhcpDone(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex);

void p2pRoleFsmStateTransition(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		enum ENUM_P2P_ROLE_STATE eNextState);

void p2pRoleFsmStateTransitionImpl(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		uint8_t ucBssIdx,
		enum ENUM_P2P_ROLE_STATE eNextState);

void p2pRoleFsmRunEventMgmtTx(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventTxCancelWait(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventAcs(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pRoleFsmRunEventScanAbort(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx);

#if (CFG_WOW_SUPPORT == 1)
void p2pRoleProcessPreSuspendFlow(struct ADAPTER *prAdapter);
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
void p2pRoleFsmRunEventAddMldLink(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);
void p2pRoleFsmRunEventDelMldLink(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);
#endif

#ifdef CFG_AP_GO_DELAY_CARRIER_ON
void p2pRoleFsmCarrierOnTimeoutHandler(struct ADAPTER *prAdapter,
				       uintptr_t ulParamPtr);

void p2pRoleFsmRunEventApGoStarted(struct ADAPTER *prAdapter,
				   struct MSG_HDR *prMsgHdr);
#endif /* CFG_AP_GO_DELAY_CARRIER_ON */

#endif
