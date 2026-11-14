/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   p2p_fsm.h
 *  \brief  Declaration of functions and finite state machine for P2P Module.
 *
 *  Declaration of functions and finite state machine for P2P Module.
 */


#ifndef _P2P_FSM_H
#define _P2P_FSM_H

void p2pFsmRunEventScanRequest(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pFsmRunEventChGrant(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

#if CFG_SUPPORT_WFD
void p2pFsmRunEventWfdSettingUpdate(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);
#endif

void p2pFsmRunEventScanDone(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pFsmRunEventMgmtFrameTx(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

void p2pFsmRunEventTxCancelWait(struct ADAPTER *prAdapter,
		struct MSG_HDR *prMsgHdr);

struct BSS_DESC *p2pGetTargetBssDesc(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void p2pFsmRunEventCsaDoneTimeOut(struct ADAPTER *prAdapter,
	uintptr_t ulParamPtr);
#if CFG_SAP_RPS_SUPPORT
void p2pFsmRunEventRpsCountdownTimeOut(struct ADAPTER *prAdapter,
	uintptr_t ulParamPtr);
#endif
#endif /* _P2P_FSM_H */
