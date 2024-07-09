/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: include/mgmt/hem_mbox.h
 */

/*! \file   hem_mbox.h
 *    \brief
 *
 */

#ifndef _HEM_MBOX_H
#define _HEM_MBOX_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
/* Message IDs */
enum ENUM_MSG_ID {
	/* MANY notify CNM to obtain channel privilege */
	MID_MNY_CNM_CH_REQ,
	/* MANY notify CNM to abort/release channel privilege */
	MID_MNY_CNM_CH_ABORT,

	/* CNM notify AIS for indicating channel granted */
	MID_CNM_AIS_CH_GRANT,
	/* CNM notify P2P for indicating channel granted */
	MID_CNM_P2P_CH_GRANT,
	/* CNM notify BOW for indicating channel granted */
	MID_CNM_BOW_CH_GRANT,

#if (CFG_SUPPORT_DFS_MASTER == 1)
	MID_CNM_P2P_RADAR_DETECT,
	MID_CNM_P2P_CSA_DONE,
#endif
	/*--------------------------------------------------*/
	/* SCN Module Mailbox Messages                      */
	/*--------------------------------------------------*/
	/* AIS notify SCN for starting scan */
	MID_AIS_SCN_SCAN_REQ,
	/* AIS notify SCN for starting scan with multiple SSID support */
	MID_AIS_SCN_SCAN_REQ_V2,
	/* AIS notify SCN for cancelling scan */
	MID_AIS_SCN_SCAN_CANCEL,
	/* P2P notify SCN for starting scan */
	MID_P2P_SCN_SCAN_REQ,
	/* P2P notify SCN for starting scan with multiple SSID support */
	MID_P2P_SCN_SCAN_REQ_V2,
	/* P2P notify SCN for cancelling scan */
	MID_P2P_SCN_SCAN_CANCEL,
	/* BOW notify SCN for starting scan */
	MID_BOW_SCN_SCAN_REQ,
	/* BOW notify SCN for starting scan with multiple SSID support */
	MID_BOW_SCN_SCAN_REQ_V2,
	/* BOW notify SCN for cancelling scan */
	MID_BOW_SCN_SCAN_CANCEL,
	/* RLM notify SCN for starting scan (OBSS-SCAN) */
	MID_RLM_SCN_SCAN_REQ,
	/* RLM notify SCN for starting scan (OBSS-SCAN)
	 * with multiple SSID support
	 */
	MID_RLM_SCN_SCAN_REQ_V2,
	/* RLM notify SCN for cancelling scan (OBSS-SCAN) */
	MID_RLM_SCN_SCAN_CANCEL,
	/* SCN notify AIS for scan completion */
	MID_SCN_AIS_SCAN_DONE,
	/* SCN notify P2P for scan completion */
	MID_SCN_P2P_SCAN_DONE,
	/* SCN notify BOW for scan completion */
	MID_SCN_BOW_SCAN_DONE,
	/* SCN notify RLM for scan completion (OBSS-SCAN) */
	MID_SCN_RLM_SCAN_DONE,

	/*--------------------------------------------------*/
	/* AIS Module Mailbox Messages                      */
	/*--------------------------------------------------*/
	/* OID/IOCTL notify AIS for join */
	MID_OID_AIS_FSM_JOIN_REQ,
	/* OID/IOCTL notify AIS for abort */
	MID_OID_AIS_FSM_ABORT,
	/* AIS notify SAA for Starting authentication/association fsm */
	MID_AIS_SAA_FSM_START,
	/* OID notify SAA to continue to do authentication/association fsm for
	** FT
	*/
	MID_OID_SAA_FSM_CONTINUE,
	/* AIS notify SAA for Aborting authentication/association fsm */
	MID_AIS_SAA_FSM_ABORT,
	/* SAA notify AIS for indicating join complete */
	MID_SAA_AIS_JOIN_COMPLETE,

#if CFG_ENABLE_BT_OVER_WIFI
	/*--------------------------------------------------*/
	/* BOW Module Mailbox Messages                      */
	/*--------------------------------------------------*/
	/* BOW notify SAA for Starting authentication/association fsm */
	MID_BOW_SAA_FSM_START,
	/* BOW notify SAA for Aborting authentication/association fsm */
	MID_BOW_SAA_FSM_ABORT,
	/* SAA notify BOW for indicating join complete */
	MID_SAA_BOW_JOIN_COMPLETE,
#endif

#if CFG_ENABLE_WIFI_DIRECT
	/*--------------------------------------------------*/
	/* P2P Module Mailbox Messages                      */
	/*--------------------------------------------------*/
	/* P2P notify SAA for Starting authentication/association fsm */
	MID_P2P_SAA_FSM_START,
	/* P2P notify SAA for Aborting authentication/association fsm */
	MID_P2P_SAA_FSM_ABORT,
	/* SAA notify P2P for indicating join complete */
	MID_SAA_P2P_JOIN_COMPLETE,

	MID_MNY_P2P_FUN_SWITCH,	/* Enable P2P FSM. */
	/* Start device discovery. */
	MID_MNY_P2P_DEVICE_DISCOVERY,
	/* Connection request. */
	MID_MNY_P2P_CONNECTION_REQ,
	/* Abort connection request, P2P FSM return to IDLE. */
	MID_MNY_P2P_CONNECTION_ABORT,
	MID_MNY_P2P_BEACON_UPDATE,
	MID_MNY_P2P_STOP_AP,
	MID_MNY_P2P_CHNL_REQ,
	MID_MNY_P2P_CHNL_ABORT,
	MID_MNY_P2P_MGMT_TX,
	MID_MNY_P2P_MGMT_TX_CANCEL_WAIT,
	MID_MNY_P2P_GROUP_DISSOLVE,
	MID_MNY_P2P_MGMT_FRAME_REGISTER,
	MID_MNY_P2P_NET_DEV_REGISTER,
	MID_MNY_P2P_START_AP,
	MID_MNY_P2P_DEL_IFACE,
	MID_MNY_P2P_MGMT_FRAME_UPDATE,
#if (CFG_SUPPORT_DFS_MASTER == 1)
	MID_MNY_P2P_DFS_CAC,
	MID_MNY_P2P_SET_NEW_CHANNEL,
#endif
#if CFG_SUPPORT_WFD
	MID_MNY_P2P_WFD_CFG_UPDATE,
#endif
	MID_MNY_P2P_ACTIVE_BSS,
#endif

#if CFG_SUPPORT_ADHOC
	/* SCN notify AIS that an IBSS Peer has been found
	 * and can merge into
	 */
	MID_SCN_AIS_FOUND_IBSS,
#endif				/* CFG_SUPPORT_ADHOC */

	/* SAA notify AIS for indicating deauthentication/disassociation */
	MID_SAA_AIS_FSM_ABORT,

	/*--------------------------------------------------*/
	/* AIS MGMT-TX Support                              */
	/*--------------------------------------------------*/
	MID_MNY_AIS_REMAIN_ON_CHANNEL,
	MID_MNY_AIS_CANCEL_REMAIN_ON_CHANNEL,
	MID_MNY_AIS_MGMT_TX,
	MID_MNY_AIS_MGMT_TX_CANCEL_WAIT,
	MID_WNM_AIS_BSS_TRANSITION,
	MID_OID_WMM_TSPEC_OPERATE,
	MID_RRM_REQ_SCHEDULE,
#if CFG_SUPPORT_NCHO
	MID_MNY_AIS_NCHO_ACTION_FRAME,
#endif
	MID_MNY_P2P_ACS,
#if (CFG_SUPPORT_TWT == 1)
	/*--------------------------------------------------*/
	/* TWT Requester Support                            */
	/*--------------------------------------------------*/
	MID_TWT_REQ_FSM_START,
	MID_TWT_REQ_FSM_TEARDOWN,
	MID_TWT_REQ_FSM_SUSPEND,
	MID_TWT_REQ_FSM_RESUME,
	MID_TWT_REQ_IND_RESULT,
	MID_TWT_REQ_IND_SUSPEND_DONE,
	MID_TWT_REQ_IND_RESUME_DONE,
	MID_TWT_REQ_IND_TEARDOWN_DONE,
	MID_TWT_REQ_IND_INFOFRM,
	MID_TWT_PARAMS_SET,
#endif
#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	MID_TWT_RESP_PARAMS_SET,
	MID_TWT_RESP_SETUP_AGRT_TO_FW,
	MID_TWT_RESP_TEARDOWN_TO_FW,
#endif
#if (CFG_SUPPORT_NAN == 1)
	MID_CNM_NAN_CH_GRANT,
#endif
	MID_TOTAL_NUM
};

/* Message header of inter-components */
struct MSG_HDR {
	struct LINK_ENTRY rLinkEntry;
	enum ENUM_MSG_ID eMsgId;
};

typedef void(*PFN_MSG_HNDL_FUNC) (struct ADAPTER *,
				  struct MSG_HDR *);

struct MSG_HNDL_ENTRY {
	enum ENUM_MSG_ID eMsgId;
	PFN_MSG_HNDL_FUNC pfMsgHndl;
};

enum EUNM_MSG_SEND_METHOD {
	/* Message is put in the queue and will be */
	MSG_SEND_METHOD_BUF = 0,
	/*executed when mailbox is checked. */
	/* The handler function is called immediately */
	MSG_SEND_METHOD_UNBUF
	/* in the same context of the sender */
};

enum ENUM_MBOX_ID {
	MBOX_ID_0 = 0,
	MBOX_ID_TOTAL_NUM
};

/* Define Mailbox structure */
struct MBOX {
	struct LINK rLinkHead;
};

struct MSG_SAA_FSM_START {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucSeqNum;
	struct STA_RECORD *prStaRec;
};

struct MSG_SAA_FSM_COMP {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucSeqNum;
	uint32_t rJoinStatus;
	struct STA_RECORD *prStaRec;
	struct SW_RFB *prSwRfb;
};

struct MSG_SAA_FSM_ABORT {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucSeqNum;
	struct STA_RECORD *prStaRec;
};

struct MSG_CONNECTION_ABORT {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucNetTypeIndex;
};

struct MSG_REMAIN_ON_CHANNEL {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	enum ENUM_BAND eBand;
	enum ENUM_CHNL_EXT eSco;
	uint8_t ucChannelNum;
	uint32_t u4DurationMs;
	uint64_t u8Cookie;
	enum ENUM_CH_REQ_TYPE eReqType;
	uint8_t ucBssIdx;
};

struct MSG_CANCEL_REMAIN_ON_CHANNEL {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint64_t u8Cookie;
	uint8_t ucBssIdx;
};

struct MSG_MGMT_TX_REQUEST {
	struct MSG_HDR rMsgHdr;
	uint8_t ucBssIdx;
	struct MSDU_INFO *prMgmtMsduInfo;
	uint64_t u8Cookie;	/* For indication. */
	u_int8_t fgNoneCckRate;
	u_int8_t fgIsOffChannel;
	struct RF_CHANNEL_INFO rChannelInfo;
	enum ENUM_CHNL_EXT eChnlExt;
	u_int8_t fgIsWaitRsp;
	uint32_t u4Duration;
};

#if (CFG_SUPPORT_TWT == 1)
struct _MSG_TWT_REQFSM_START_T {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	struct STA_RECORD *prStaRec;
	u_int8_t ucTWTFlowId;
};

struct _MSG_TWT_REQFSM_IND_RESULT_T {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	struct STA_RECORD *prStaRec;
	u_int8_t ucTWTFlowId;
};

struct _MSG_TWT_REQFSM_TEARDOWN_T {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	struct STA_RECORD *prStaRec;
	u_int8_t ucTWTFlowId;
};

struct _MSG_TWT_REQFSM_SUSPEND_T {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	struct STA_RECORD *prStaRec;
	u_int8_t ucTWTFlowId;
};

struct _MSG_TWT_REQFSM_RESUME_T {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	struct STA_RECORD *prStaRec;
	u_int8_t ucTWTFlowId;
	u_int8_t ucNextTWTSize;
	u_int64_t u8NextTWT;
};

struct _MSG_TWT_REQFSM_IND_INFOFRM_T {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	struct STA_RECORD *prStaRec;
	u_int8_t ucTWTFlowId;
	struct _NEXT_TWT_INFO_T rNextTWTInfo;
};

struct _MSG_TWT_PARAMS_SET_T {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	struct _TWT_CTRL_T rTWTCtrl;
};

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
struct _MSG_TWT_HOTSPOT_PARAMS_SET_T {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	struct _TWT_HOTSPOT_CTRL_T rTWTCtrl;
};
#endif

#endif

struct MSG_CANCEL_TX_WAIT_REQUEST {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint64_t u8Cookie;
	uint8_t ucBssIdx;
};

struct MSG_SAA_FT_CONTINUE {
	struct MSG_HDR rMsgHdr;
	struct STA_RECORD *prStaRec;
	/* if fgFTRicRequest is TRUE, then will do FT Resource
	** Request Protocol
	*/
	u_int8_t fgFTRicRequest;
};

/* specific message data types */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
void mboxSetup(IN struct ADAPTER *prAdapter,
	       IN enum ENUM_MBOX_ID eMboxId);

void
mboxSendMsg(IN struct ADAPTER *prAdapter,
	    IN enum ENUM_MBOX_ID eMboxId, IN struct MSG_HDR *prMsg,
	    IN enum EUNM_MSG_SEND_METHOD eMethod);

void mboxRcvAllMsg(IN struct ADAPTER *prAdapter,
		   IN enum ENUM_MBOX_ID eMboxId);

void mboxInitialize(IN struct ADAPTER *prAdapter);

void mboxDestroy(IN struct ADAPTER *prAdapter);

void mboxDummy(IN struct ADAPTER *prAdapter,
	       struct MSG_HDR *prMsgHdr);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _HEM_MBOX_H */
