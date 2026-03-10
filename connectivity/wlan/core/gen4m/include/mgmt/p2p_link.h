/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*! \file   "p2p_link.h"
 *  \brief
 */

#ifndef _P2P_LINK_H
#define _P2P_LINK_H

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t p2pLinkProcessRxAuthReqFrame(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct SW_RFB *prSwRfb);

uint32_t p2pLinkProcessRxAssocReqFrame(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct SW_RFB *prSwRfb);

void p2pMldBssInit(struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);

void p2pMldBssUninit(struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);

#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
void p2pLinkInitGCRole(struct ADAPTER *prAdapter);

void p2pLinkUninitGCRole(struct ADAPTER *prAdapter);

uint8_t p2pGetGCBssNum(struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);
#endif

void p2pTargetBssDescResetConnecting(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *fsm);

void p2pClearAllLink(struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);

void p2pDeactivateAllLink(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t fgClearStaRec);

void p2pFillLinkBssDesc(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	struct BSS_DESC_SET *prBssDescSet);

struct BSS_INFO *p2pGetLinkBssInfo(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx);

void p2pGetLinkWmmQueSet(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

void p2pSetLinkBssDesc(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	struct BSS_DESC *prBssDesc,
	uint8_t ucLinkIdx);

struct BSS_DESC *p2pGetLinkBssDesc(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx);

uint8_t p2pGetLinkNum(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);

void p2pSetLinkStaRec(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	struct STA_RECORD *prStaRec,
	uint8_t ucLinkIdx);

struct STA_RECORD *p2pGetLinkStaRec(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx);

struct P2P_CHNL_REQ_INFO *p2pGetChnlReqInfo(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx);

struct P2P_CONNECTION_REQ_INFO *p2pGetConnReqInfo(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx);

void p2pLinkAcquireChJoin(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	struct P2P_CHNL_REQ_INFO *prChnlReq);

struct P2P_ROLE_FSM_INFO *p2pGetDefaultRoleFsmInfo(
	struct ADAPTER *prAdapter,
	enum ENUM_IFTYPE eIftype);

struct BSS_INFO *p2pGetDefaultLinkBssInfo(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

struct STA_RECORD *p2pGetDefaultLinkStaRec(
	struct ADAPTER *prAdapter,
	enum ENUM_IFTYPE eIftype);

uint16_t bssAssignAssocID(struct STA_RECORD *prStaRec);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
void p2pScanFillSecondaryLink(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prBssDescSet);
#endif

void p2pLinkStaRecFree(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct BSS_INFO *prP2pBssInfo);

void p2pRemoveAllBssDesc(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

void p2pRxDeauthNoWtbl(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct SW_RFB *prSwRfb);

u_int8_t p2pNeedAppendP2pIE(
	struct ADAPTER *ad,
	struct BSS_INFO *bss);

u_int8_t p2pNeedSkipProbeResp(
	struct ADAPTER *ad,
	struct BSS_INFO *bss);

#endif /* !_P2P_MLO_H */
