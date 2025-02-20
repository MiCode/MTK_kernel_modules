/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#if CFG_ENABLE_WIFI_DIRECT

#ifndef _P2P_DEV_STATE_H
#define _P2P_DEV_STATE_H

u_int8_t
p2pDevStateInit_IDLE(struct ADAPTER *prAdapter,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_DEV_STATE *peNextState);

void p2pDevStateAbort_IDLE(struct ADAPTER *prAdapter);

u_int8_t
p2pDevStateInit_REQING_CHANNEL(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_DEV_STATE *peNextState);

void
p2pDevStateAbort_REQING_CHANNEL(struct ADAPTER *prAdapter,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_DEV_STATE eNextState);

void
p2pDevStateInit_CHNL_ON_HAND(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

void
p2pDevStateAbort_CHNL_ON_HAND(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_DEV_STATE eNextState);

void p2pDevStateInit_SCAN(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo);

void p2pDevStateAbort_SCAN(struct ADAPTER *prAdapter,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo);

u_int8_t
p2pDevStateInit_OFF_CHNL_TX(struct ADAPTER *prAdapter,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		enum ENUM_P2P_DEV_STATE *peNextState);

void
p2pDevStateAbort_OFF_CHNL_TX(struct ADAPTER *prAdapter,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_DEV_STATE eNextState);

u_int8_t
p2pDevStateInit_LISTEN_OFFLOAD(
		struct ADAPTER *prAdapter,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		struct P2P_LISTEN_OFFLOAD_INFO *pLoInfo,
		enum ENUM_P2P_DEV_STATE *peNextState);

void p2pDevStateAbort_LISTEN_OFFLOAD(
		struct ADAPTER *prAdapter,
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo,
		struct P2P_LISTEN_OFFLOAD_INFO *pLoInfo,
		enum ENUM_P2P_DEV_STATE eNextState);

#endif
#endif
