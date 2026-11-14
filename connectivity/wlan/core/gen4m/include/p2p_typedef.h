/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   p2p_typedef.h
 *    \brief  Declaration of data type and return values of
 *    internal protocol stack.
 *
 *    In this file we declare the data type and return values
 *    which will be exported to all MGMT Protocol Stack.
 */

#ifndef _P2P_TYPEDEF_H
#define _P2P_TYPEDEF_H

#if CFG_ENABLE_WIFI_DIRECT

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */

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

/*
 * type definition of pointer to p2p structure
 */
/* typedef struct GL_P2P_INFO   GL_P2P_INFO_T, *P_GL_P2P_INFO_T; */
struct P2P_DEV_INFO;	/* declare P2P_INFO_T */

struct P2P_FSM_INFO;	/* declare P2P_FSM_INFO_T */

struct P2P_DEV_FSM_INFO;	/* declare P2P_DEV_FSM_INFO_T */

struct P2P_ROLE_FSM_INFO;	/* declare P2P_ROLE_FSM_INFO_T */

struct P2P_CONNECTION_SETTINGS;	/* declare P2P_CONNECTION_SETTINGS_T */

/* Type definition for function pointer to p2p function*/
typedef u_int8_t(*P2P_LAUNCH) (struct GLUE_INFO *prGlueInfo);

typedef u_int8_t(*P2P_REMOVE) (struct GLUE_INFO *prGlueInfo,
					  u_int8_t fgIsWlanLaunched);

typedef u_int8_t(*KAL_P2P_GET_CIPHER) (struct GLUE_INFO *prGlueInfo);

typedef u_int8_t(*KAL_P2P_GET_TKIP_CIPHER) (struct GLUE_INFO *prGlueInfo);

typedef u_int8_t(*KAL_P2P_GET_CCMP_CIPHER) (struct GLUE_INFO *prGlueInfo);

typedef u_int8_t(*KAL_P2P_GET_WSC_MODE) (struct GLUE_INFO *prGlueInfo);

typedef void(*KAL_P2P_SET_MULTICAST_WORK_ITEM) (struct GLUE_INFO *prGlueInfo);

typedef void(*P2P_NET_REGISTER) (struct GLUE_INFO *prGlueInfo);

typedef void(*P2P_NET_UNREGISTER) (struct GLUE_INFO *prGlueInfo);

typedef void(*KAL_P2P_UPDATE_ASSOC_INFO) (struct GLUE_INFO *prGlueInfo,
		uint8_t *pucFrameBody,
		uint32_t u4FrameBodyLen,
		u_int8_t fgReassocRequest);

typedef u_int8_t(*P2P_VALIDATE_AUTH) (struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		struct STA_RECORD **pprStaRec,
		uint16_t *pu2StatusCode);

typedef u_int8_t(*P2P_VALIDATE_ASSOC_REQ) (struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		uint16_t *pu4ControlFlags);

typedef void(*P2P_RUN_EVENT_AAA_TX_FAIL) (struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec);

typedef u_int8_t(*P2P_PARSE_CHECK_FOR_P2P_INFO_ELEM) (
		struct ADAPTER *prAdapter,
		uint8_t *pucBuf,
		uint8_t *pucOuiType);

typedef uint32_t(*P2P_RUN_EVENT_AAA_COMPLETE) (struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec);

typedef void(*P2P_PROCESS_EVENT_UPDATE_NOA_PARAM) (
		struct ADAPTER *prAdapter,
		uint8_t ucNetTypeIndex,
		struct EVENT_UPDATE_NOA_PARAMS *prEventUpdateNoaParam);

typedef void(*SCAN_P2P_PROCESS_BEACON_AND_PROBE_RESP) (
		struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		uint32_t *prStatus,
		struct BSS_DESC *prBssDesc,
		struct WLAN_BEACON_FRAME *prWlanBeaconFrame);

typedef void(*P2P_RX_PUBLIC_ACTION_FRAME) (struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb);

typedef void(*RLM_RSP_GENERATE_OBSS_SCAN_IE) (struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);

typedef void(*RLM_UPDATE_BW_BY_CH_LIST_FOR_AP) (struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);

typedef void(*RLM_PROCESS_PUBLIC_ACTION) (struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb);

typedef void(*RLM_PROCESS_HT_ACTION) (struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb);

typedef void(*RLM_UPDATE_PARAMS_FOR_AP) (struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, u_int8_t fgUpdateBeacon);

typedef void(*RLM_HANDLE_OBSS_STATUS_EVENT_PKT) (struct ADAPTER *prAdapter,
		struct EVENT_AP_OBSS_STATUS *prObssStatus);

typedef u_int8_t(*P2P_FUNC_VALIDATE_PROBE_REQ) (struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		uint32_t *pu4ControlFlags);

typedef void(*RLM_BSS_INIT_FOR_AP) (struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);

typedef uint32_t(*P2P_GET_PROB_RSP_IE_TABLE_SIZE) (void);

typedef uint8_t *(*P2P_BUILD_REASSOC_REQ_FRAME_COMMON_IES) (
		struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo, uint8_t *pucBuffer);

typedef void(*P2P_FUNC_DISCONNECT) (struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		u_int8_t fgSendDeauth, uint16_t u2ReasonCode);

typedef void(*P2P_FSM_RUN_EVENT_RX_DEAUTH) (struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct SW_RFB *prSwRfb);

typedef void(*P2P_FSM_RUN_EVENT_RX_DISASSOC) (struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct SW_RFB *prSwRfb);

typedef u_int8_t(*P2P_FUN_IS_AP_MODE) (struct P2P_FSM_INFO *prP2pFsmInfo);

typedef void(*P2P_FSM_RUN_EVENT_BEACON_TIMEOUT) (struct ADAPTER *prAdapter);

typedef void(*P2P_FUNC_STORE_ASSOC_RSP_IE_BUFFER) (
		struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb);

typedef void(*P2P_GENERATE_P2P_IE) (struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);

typedef uint32_t(*P2P_CALCULATE_P2P_IE_LEN) (struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

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

#endif /*CFG_ENABLE_WIFI_DIRECT */

#endif /* _P2P_TYPEDEF_H */
