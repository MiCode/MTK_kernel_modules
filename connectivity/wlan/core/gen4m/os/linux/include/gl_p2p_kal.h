/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_p2p_kal.h
 *    \brief  Declaration of KAL functions for Wi-Fi Direct support
 *	    - kal*() which is provided by GLUE Layer.
 *
 *    Any definitions in this file will be shared among GLUE Layer
 *    and internal Driver Stack.
 */


#ifndef _GL_P2P_KAL_H
#define _GL_P2P_KAL_H

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "config.h"
#include "gl_typedef.h"
#include "gl_os.h"
#include "wlan_lib.h"
#include "wlan_oid.h"
#include "wlan_p2p.h"
#include "gl_kal.h"
#include "gl_wext_priv.h"
#include "gl_p2p_ioctl.h"
#include "nic/p2p.h"

#if DBG
extern int allocatedMemSize;
#endif

u_int8_t kalP2pFuncGetChannelType(enum ENUM_CHNL_EXT rChnlSco,
		enum nl80211_channel_type *channel_type);

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/* Service Discovery */
void kalP2PIndicateSDRequest(struct GLUE_INFO *prGlueInfo,
		uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN],
		uint8_t ucSeqNum);

void kalP2PIndicateSDResponse(struct GLUE_INFO *prGlueInfo,
		uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN],
		uint8_t ucSeqNum);

void kalP2PIndicateTXDone(struct GLUE_INFO *prGlueInfo,
		uint8_t ucSeqNum, uint8_t ucStatus);

/*------------------------------------------------------------------------*/
/* Wi-Fi Direct handling                                                      */
/*------------------------------------------------------------------------*/
/*ENUM_PARAM_MEDIA_STATE_T kalP2PGetState(P_GLUE_INFO_T prGlueInfo);*/

/*VOID
 *kalP2PSetState(P_GLUE_INFO_T prGlueInfo,
 *		ENUM_PARAM_MEDIA_STATE_T eState,
 *		PARAM_MAC_ADDRESS rPeerAddr,
 *		UINT_8 ucRole);
 */

void
kalP2PUpdateAssocInfo(struct GLUE_INFO *prGlueInfo,
		uint8_t *pucFrameBody,
		uint32_t u4FrameBodyLen,
		u_int8_t fgReassocRequest,
		uint8_t ucBssIndex);

/*UINT_32 kalP2PGetFreqInKHz(P_GLUE_INFO_T prGlueInfo);*/

int32_t mtk_Netdev_To_DevIdx(struct GLUE_INFO *prGlueInfo,
		void *pvNdev, uint8_t *pucDevIdx);
int32_t mtk_Netdev_To_RoleIdx(struct GLUE_INFO *prGlueInfo,
		void *pvNdev,
		uint8_t *pucRoleIdx);

uint8_t kalP2PGetRole(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIdx);

#if 1
void kalP2PSetRole(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRole,
		uint8_t ucRoleIdx);

#else
void
kalP2PSetRole(struct GLUE_INFO *prGlueInfo,
		uint8_t ucResult,
		uint8_t *pucSSID,
		uint8_t ucSSIDLen,
		uint8_t ucRole);
#endif

void kalP2PSetCipher(struct GLUE_INFO *prGlueInfo,
		uint32_t u4Cipher,
		uint8_t ucRoleIdx);

u_int8_t kalP2PGetCipher(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIdx);

u_int8_t kalP2PGetWepCipher(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIdx);

u_int8_t kalP2PGetTkipCipher(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIdx);

u_int8_t kalP2PGetCcmpCipher(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIdx);

void kalP2PSetWscMode(struct GLUE_INFO *prGlueInfo,
		uint8_t ucWscMode);

uint8_t kalP2PGetWscMode(struct GLUE_INFO *prGlueInfo);

uint16_t kalP2PCalWSC_IELen(struct GLUE_INFO *prGlueInfo,
		uint8_t ucType,
		uint8_t ucRoleIdx);

void kalP2PGenWSC_IE(struct GLUE_INFO *prGlueInfo,
		uint8_t ucType,
		uint8_t *pucBuffer,
		uint8_t ucRoleIdx);

void kalP2PUpdateWSC_IE(struct GLUE_INFO *prGlueInfo,
		uint8_t ucType,
		uint8_t *pucBuffer,
		uint16_t u2BufferLength,
		uint8_t ucRoleIdx);

uint16_t kalP2PCalP2P_IELen(struct GLUE_INFO *prGlueInfo,
		uint8_t ucIndex,
		uint8_t ucRoleIdx);

void kalP2PGenP2P_IE(struct GLUE_INFO *prGlueInfo,
		uint8_t ucIndex,
		uint8_t *pucBuffer,
		uint8_t ucRoleIdx);

void kalP2PTxCarrierOn(struct GLUE_INFO *prGlueInfo,
		struct BSS_INFO *prBssInfo);

u_int8_t kalP2PIsTxCarrierOn(struct GLUE_INFO *prGlueInfo,
		struct BSS_INFO *prBssInfo);

void kalP2PUpdateP2P_IE(struct GLUE_INFO *prGlueInfo,
		uint8_t ucIndex,
		uint8_t *pucBuffer,
		uint16_t u2BufferLength,
		uint8_t ucRoleIdx);

u_int8_t kalP2PIndicateFound(struct GLUE_INFO *prGlueInfo);

void kalP2PIndicateConnReq(struct GLUE_INFO *prGlueInfo,
		uint8_t *pucDevName,
		int32_t u4NameLength,
		uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN],
		uint8_t ucDevType,	/* 0: P2P Device / 1: GC / 2: GO */
		int32_t i4ConfigMethod,
		int32_t i4ActiveConfigMethod);

/*VOID kalP2PInvitationStatus(P_GLUE_INFO_T prGlueInfo,
 *		UINT_32 u4InvStatus);
 */

void
kalP2PInvitationIndication(struct GLUE_INFO *prGlueInfo,
		struct P2P_DEVICE_DESC *prP2pDevDesc,
		uint8_t *pucSsid,
		uint8_t ucSsidLen,
		uint8_t ucOperatingChnl,
		uint8_t ucInvitationType,
		uint8_t *pucGroupBssid);

struct net_device *kalP2PGetDevHdlr(struct GLUE_INFO *prGlueInfo);

void
kalGetChnlList(struct GLUE_INFO *prGlueInfo,
		enum ENUM_BAND eSpecificBand,
		uint8_t ucMaxChannelNum,
		uint8_t *pucNumOfChannel,
		struct RF_CHANNEL_INFO *paucChannelList);

#if CFG_SUPPORT_ANTI_PIRACY
void kalP2PIndicateSecCheckRsp(struct GLUE_INFO *prGlueInfo,
		uint8_t *pucRsp,
		uint16_t u2RspLen);
#endif

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

void
kalP2PIndicateChannelReady(struct GLUE_INFO *prGlueInfo,
		uint64_t u8SeqNum,
		uint32_t u4ChannelNum,
		enum ENUM_BAND eBand,
		enum ENUM_CHNL_EXT eSco,
		uint32_t u4Duration);

void kalP2PIndicateScanDone(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex,
		u_int8_t fgIsAbort);

void
kalP2PIndicateBssInfo(struct GLUE_INFO *prGlueInfo,
		uint8_t *pucFrameBuf,
		uint32_t u4BufLen,
		struct RF_CHANNEL_INFO *prChannelInfo,
		int32_t i4SignalStrength);

void
kalP2PIndicateRxMgmtFrame(struct ADAPTER *prAdapter,
		struct GLUE_INFO *prGlueInfo,
		struct SW_RFB *prSwRfb,
		u_int8_t fgIsDevInterface,
		uint8_t ucRoleIdx,
		uint32_t u4LinkId);

void kalP2PIndicateMgmtTxStatus(struct GLUE_INFO *prGlueInfo,
		struct MSDU_INFO *prMsduInfo,
		u_int8_t fgIsAck);

void
kalP2PIndicateChannelExpired(struct GLUE_INFO *prGlueInfo,
		uint64_t u8SeqNum,
		uint32_t u4ChannelNum,
		enum ENUM_BAND eBand,
		enum ENUM_CHNL_EXT eSco);

void
kalP2PGCIndicateConnectionStatus(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex,
		struct P2P_CONNECTION_REQ_INFO *prP2pConnInfo,
		uint8_t *pucRxIEBuf,
		uint16_t u2RxIELen,
		uint16_t u2StatusReason,
		uint32_t eStatus);

void
kalP2PGOStationUpdate(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex,
		struct STA_RECORD *prCliStaRec,
		u_int8_t fgIsNew);

#if (CFG_SUPPORT_DFS_MASTER == 1)
void
kalP2PRddDetectUpdate(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex);

void
kalP2PCacFinishedUpdate(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex);
void
kalP2PCacStartedUpdate(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex);
#endif

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER

u_int8_t kalP2PSetBlockList(struct GLUE_INFO *prGlueInfo,
		uint8_t rbssid[PARAM_MAC_ADDR_LEN],
		u_int8_t fgIsblock,
		uint8_t ucRoleIndex);

u_int8_t kalP2PResetBlockList(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex);

#if CFG_AP_80211KVR_INTERFACE
void kalP2PCatBlockList(struct GLUE_INFO *prGlueInfo,
		bool flag);
#endif

u_int8_t kalP2PCmpBlockList(struct GLUE_INFO *prGlueInfo,
		uint8_t rbssid[PARAM_MAC_ADDR_LEN],
		uint8_t ucRoleIndex);

void kalP2PSetMaxClients(struct GLUE_INFO *prGlueInfo,
		uint32_t u4MaxClient,
		uint8_t ucRoleIndex);

u_int8_t kalP2PMaxClients(struct GLUE_INFO *prGlueInfo,
		uint32_t u4NumClient,
		uint8_t ucRoleIndex);

uint8_t kalP2PGetMaxClients(struct GLUE_INFO *prGlueInfo, uint8_t ucRoleIndex);
#endif

void kalP2pUnlinkBss(struct GLUE_INFO *prGlueInfo, uint8_t aucBSSID[]);

void kalP2pIndicateQueuedMgmtFrame(struct GLUE_INFO *prGlueInfo,
		struct P2P_QUEUED_ACTION_FRAME *prFrame);

void kalP2pIndicateAcsResult(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex,
		int8_t icLinkId,
		enum ENUM_BAND eBand,
		uint8_t ucPrimaryCh,
		uint8_t ucSecondCh,
		uint8_t ucSeg0Ch,
		uint8_t ucSeg1Ch,
		enum ENUM_MAX_BANDWIDTH_SETTING eChnlBw,
		enum P2P_VENDOR_ACS_HW_MODE eHwMode,
		uint16_t u2PunctBitmap,
		enum ENUM_CHNL_EXT eSco);

void kalP2pIndicateListenOffloadEvent(
	struct GLUE_INFO *prGlueInfo,
	uint32_t event);

#if (CFG_SUPPORT_DFS_MASTER == 1)
void kalP2pPreStartRdd(
		struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIdx,
		uint32_t ucPrimaryCh,
		enum ENUM_BAND eBand);

void kalP2pIndicateRadarEvent(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex,
		uint32_t event,
		uint32_t freq);
#endif

void kalP2pNotifyStopApComplete(struct ADAPTER *prAdapter,
		uint8_t ucRoleIndex);

void kalP2pNotifyDisconnComplete(struct ADAPTER *prAdapter,
		uint8_t ucRoleIndex);

void kalP2pNotifyDelStaComplete(struct ADAPTER *prAdapter,
		uint8_t ucRoleIndex);

u_int8_t kalP2pIsStoppingAp(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

void kalP2pIndicateChnlSwitchStarted(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct RF_CHANNEL_INFO *prRfChnlInfo,
	uint8_t ucCsaCount,
	u_int8_t fgQuiet,
	u_int8_t fgLockHeld);

void kalP2pChnlSwitchStartNotifyWork(struct work_struct *work);
void kalP2pCsaNotifyWorkInit(struct BSS_INFO *prBssInfo);

#if (CFG_SUPPORT_DFS_MASTER == 1)
int32_t kalP2pFuncPreStartRdd(
	struct GLUE_INFO *prGlueInfo,
	uint8_t ucRoleIdx,
	struct cfg80211_chan_def *chandef,
	unsigned int cac_time_ms);
#endif

void kalP2pClearCsaChan(
	struct GL_P2P_INFO *prGlueP2pInfo);

void kalSetP2pRoleMac(
		struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t ucRoleIdx);

void kalSetP2pDevMac(
		struct GLUE_INFO *prGlueInfo,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t ucRoleIdx);

void *kalGetP2pNetHdl(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Idx, u_int8_t fgIsRole);

#if CFG_AP_80211KVR_INTERFACE
int32_t kalGetMulAPIfIdx(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Idx, uint32_t *pu4IfIndex);
#endif

void *kalGetP2pDevScanReq(struct GLUE_INFO *prGlueInfo);
u_int8_t kalGetP2pDevScanSpecificSSID(struct GLUE_INFO *prGlueInfo);

#if CFG_SUPPORT_IDC_RIL_BRIDGE_NOTIFY
void kalIdcRegisterRilNotifier(struct GLUE_INFO *prGlueInfo);
void kalIdcUnregisterRilNotifier(struct GLUE_INFO *prGlueInfo);
void kalIdcGetRilInfo(void);
#endif
#if CFG_SUPPORT_IDC_RIL_BRIDGE
void kalSetRilBridgeChannelInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucRat,
	uint32_t u4Band,
	uint32_t u4Channel);
#endif

void kalP2pStopApInterface(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

void kalP2pCuRptUevent(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand, uint8_t ucPrimaryCh,
	uint8_t ucChnlLoad);

#endif /* _GL_P2P_KAL_H */
