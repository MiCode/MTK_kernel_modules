/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#if CFG_ENABLE_WIFI_DIRECT

#ifndef _P2P_ROLE_STATE_H
#define _P2P_ROLE_STATE_H

void
p2pRoleStateInit_IDLE(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct BSS_INFO *prP2pBssInfo);

void
p2pRoleStateAbort_IDLE(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo);

void p2pRoleStateInit_SCAN(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo);

void p2pRoleStateAbort_SCAN(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo);

void
p2pRoleStateInit_REQING_CHANNEL(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

void
p2pRoleStateAbort_REQING_CHANNEL(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pRoleBssInfo,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		enum ENUM_P2P_ROLE_STATE eNextState);

void
p2pRoleStateInit_AP_CHNL_DETECTION(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo,
		struct P2P_CONNECTION_REQ_INFO *prConnReqInfo);

void
p2pRoleStateAbort_AP_CHNL_DETECTION(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		struct P2P_SCAN_REQ_INFO *prP2pScanReqInfo,
		enum ENUM_P2P_ROLE_STATE eNextState);

void
p2pRoleStateInit_GC_JOIN(struct ADAPTER *prAdapter,
			 struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
			 struct P2P_JOIN_INFO *prJoinInfo);

void
p2pRoleStateAbort_GC_JOIN(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_JOIN_INFO *prJoinInfo,
		enum ENUM_P2P_ROLE_STATE eNextState);

#if (CFG_SUPPORT_DFS_MASTER == 1)
void
p2pRoleStateInit_DFS_CAC(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

void
p2pRoleStateAbort_DFS_CAC(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pRoleBssInfo,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		enum ENUM_P2P_ROLE_STATE eNextState);

void
p2pRoleStateInit_SWITCH_CHANNEL(struct ADAPTER *prAdapter,
				struct P2P_CSA_REQ_INFO *prCsaReqInfo,
				struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

void
p2pRoleStateAbort_SWITCH_CHANNEL(struct ADAPTER *prAdapter,
				 struct P2P_CSA_REQ_INFO *prCsaReqInfo,
				 struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

void
p2pRoleStatePrepare_To_DFS_CAC_STATE(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		enum ENUM_CHANNEL_WIDTH rChannelWidth,
		struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

#endif

void
p2pRoleStatePrepare_To_REQING_CHANNEL_STATE(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo);

u_int8_t
p2pRoleStateInit_OFF_CHNL_TX(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		enum ENUM_P2P_ROLE_STATE *peNextState);

void
p2pRoleStateAbort_OFF_CHNL_TX(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		enum ENUM_P2P_ROLE_STATE eNextState);

#endif

#endif /* CFG_ENABLE_WIFI_DIRECT */
