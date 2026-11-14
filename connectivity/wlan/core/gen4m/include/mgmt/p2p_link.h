/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "p2p_link.h"
 *  \brief
 */

#ifndef _P2P_LINK_H
#define _P2P_LINK_H

#if (CFG_SUPPORT_802_11BE_MLO == 1)
extern uint8_t gucRemainMldBssLinkNum;

uint32_t p2pLinkProcessRxAuthReqFrame(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct SW_RFB *prSwRfb);

uint32_t p2pLinkProcessRxAssocReqFrame(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct SW_RFB *prSwRfb,
	uint16_t *pu2StatusCode);

struct MLD_BSS_INFO *p2pMldBssInit(struct ADAPTER *prAdapter,
	const uint8_t aucMldMacAddr[],
	u_int8_t fgIsApMode);

void p2pMldBssUninit(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldbss);

void p2pLinkInitGcOtherLinks(struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkNum);

void p2pLinkUninitGcOtherLinks(struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);

uint32_t p2pLinkGet2ndLinkFreqByCfg(struct ADAPTER *prAdapter,
	u_int8_t fgIsApMode,
	enum ENUM_BAND eMainLinkBand, uint32_t u4MainLinkFreq,
	uint32_t *u4PreferFreq);

void p2pLinkGet2ndLinkFreq(struct ADAPTER *prAdapter,
	u_int8_t fgIsApMode,
	enum ENUM_BAND eMainLinkBand, uint32_t u4MainLinkFreq,
	uint32_t u4PeerFreq, uint32_t *u4PreferFreq);
#endif

void p2pTargetBssDescResetConnecting(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *fsm);

void p2pClearAllLink(struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);

void p2pDeactivateAllLink(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t fgClearStaRec);

void p2pSetLinkBssInfo(struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx,
	struct BSS_INFO *prBssInfo);

struct BSS_INFO *p2pGetLinkBssInfo(struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx);

void p2pGetLinkWmmQueSet(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

struct BSS_DESC *p2pGetLinkBssDesc(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx);

void p2pSetLinkStaRec(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	struct STA_RECORD *prStaRec,
	uint8_t ucLinkIdx);

struct STA_RECORD *p2pGetLinkStaRec(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx);

void p2pLinkAcquireChJoin(struct ADAPTER *prAdapter,
			  struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
			  struct P2P_CHNL_REQ_INFO *prChnlReq,
			  struct P2P_JOIN_INFO *prJoinInfo);

struct P2P_ROLE_FSM_INFO *p2pGetDefaultRoleFsmInfo(
	struct ADAPTER *prAdapter,
	enum ENUM_IFTYPE eIftype);

struct BSS_INFO *p2pGetDefaultLinkBssInfo(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

uint16_t bssAssignAssocID(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
void p2pScanFillSecondaryLink(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prBssInfo,
			      struct BSS_DESC_SET *prBssDescSet);
#endif

void p2pLinkStaRecFree(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct BSS_INFO *prP2pBssInfo);

void p2pRemoveAllBssDesc(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

u_int8_t p2pNeedAppendP2pIE(
	struct ADAPTER *ad,
	struct BSS_INFO *bss);

u_int8_t p2pNeedSkipProbeResp(
	struct ADAPTER *ad,
	struct BSS_INFO *bss);

#endif /* !_P2P_MLO_H */
