// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "hem_mbox.c"
 *    \brief
 *
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

#if (CFG_SUPPORT_802_11AX == 1)
#include "he_rlm.h"
#endif

extern void kalSendUeventHandler(struct ADAPTER *prAdapter,
				 struct MSG_HDR *prMsgHdr);

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
#if DBG
static const char * const apucDebugMsg[] = {
	"MID_MNY_CNM_CH_REQ",
	"MID_MNY_CNM_CH_ABORT",
	"MID_CNM_AIS_CH_GRANT",
	"MID_CNM_P2P_CH_GRANT",

#if (CFG_SUPPORT_DFS_MASTER == 1)
	"MID_CNM_P2P_RADAR_DETECT",
#endif
	"MID_CNM_P2P_CSA_DONE",
	"MID_AIS_SCN_SCAN_REQ",
	"MID_AIS_SCN_SCAN_REQ_V2",
	"MID_AIS_SCN_SCAN_CANCEL",
	"MID_P2P_SCN_SCAN_REQ",
	"MID_P2P_SCN_SCAN_REQ_V2",
	"MID_P2P_SCN_SCAN_CANCEL",
	"MID_RLM_SCN_SCAN_REQ",
	"MID_RLM_SCN_SCAN_REQ_V2",
	"MID_RLM_SCN_SCAN_CANCEL",
	"MID_SCN_AIS_SCAN_DONE",
	"MID_SCN_P2P_SCAN_DONE",
	"MID_SCN_RLM_SCAN_DONE",

	"MID_OID_AIS_FSM_JOIN_REQ",
	"MID_OID_AIS_FSM_ABORT",
	"MID_AIS_SAA_FSM_START",
	"MID_OID_SAA_FSM_CONTINUE",
	"MID_OID_SAA_FSM_EXTERNAL_AUTH",
	"MID_AIS_SAA_FSM_ABORT",
	"MID_SAA_AIS_JOIN_COMPLETE",

#if CFG_ENABLE_WIFI_DIRECT
	"MID_P2P_SAA_FSM_START",
	"MID_P2P_SAA_FSM_ABORT",
	"MID_SAA_P2P_JOIN_COMPLETE",

	"MID_MNY_P2P_FUN_SWITCH",
	"MID_MNY_P2P_DEVICE_DISCOVERY",
	"MID_MNY_P2P_CONNECTION_REQ",
	"MID_MNY_P2P_CONNECTION_ABORT",
	"MID_MNY_P2P_BEACON_UPDATE",
	"MID_MNY_P2P_BEACON_REINIT",
	"MID_MNY_P2P_STOP_AP",
	"MID_MNY_P2P_CHNL_REQ",
	"MID_MNY_P2P_CHNL_ABORT",
	"MID_MNY_P2P_MGMT_TX",
	"MID_MNY_P2P_MGMT_TX_CANCEL_WAIT",
	"MID_MNY_P2P_MGMT_FRAME_REGISTER",
	"MID_MNY_P2P_START_AP",
	"MID_MNY_P2P_DEL_IFACE",
#if (CFG_SUPPORT_DFS_MASTER == 1)
	"MID_MNY_P2P_DFS_CAC",
	"MID_MNY_P2P_START_CAC",
	"MID_MNY_P2P_STOP_CAC",
#endif
	"MID_MNY_P2P_SET_NEW_CHANNEL",
#if CFG_SUPPORT_WFD
	"MID_MNY_P2P_WFD_CFG_UPDATE",
#endif
	"MID_MNY_P2P_UPDATE_DEV_BSS",
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	"MID_MNY_P2P_ADD_MLD_LINK",
	"MID_MNY_P2P_DEL_MLD_LINK",
#endif
#endif

	"MID_SAA_AIS_FSM_ABORT",
	"MID_MNY_AIS_REMAIN_ON_CHANNEL",
	"MID_MNY_AIS_CANCEL_REMAIN_ON_CHANNEL",
	"MID_MNY_AIS_MGMT_TX",
	"MID_MNY_AIS_MGMT_TX_CANCEL_WAIT",
#if CFG_SUPPORT_802_11V_BTM_OFFLOAD
	"MID_WNM_AIS_BSS_TRANSITION",
#endif
	"MID_OID_WMM_TSPEC_OPERATE",
	"MID_RRM_REQ_SCHEDULE",
#if CFG_SUPPORT_NCHO
	"MID_MNY_AIS_NCHO_ACTION_FRAME",
#endif
	"MID_MNY_P2P_ACS",

#if (CFG_SUPPORT_TWT == 1)
	"MID_TWT_REQ_FSM_START",
	"MID_TWT_REQ_FSM_TEARDOWN",
	"MID_TWT_REQ_FSM_SUSPEND",
	"MID_TWT_REQ_FSM_RESUME",
	"MID_TWT_REQ_IND_RESULT",
	"MID_TWT_REQ_IND_SUSPEND_DONE",
	"MID_TWT_REQ_IND_RESUME_DONE",
	"MID_TWT_REQ_IND_TEARDOWN_DONE",
	"MID_TWT_REQ_IND_INFOFRM",
	"MID_TWT_PARAMS_SET",
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	"MID_SMPS_ACTION_SET",
#endif
#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	"MID_TWT_RESP_PARAMS_SET",
	"MID_TWT_RESP_SETUP_AGRT_TO_FW",
	"MID_TWT_RESP_TEARDOWN_TO_FW",
#endif
#if (CFG_SUPPORT_BTWT == 1)
	"MID_BTWT_REQ_FSM_START",
	"MID_BTWT_REQ_FSM_TEARDOWN",
	"MID_BTWT_REQ_IND_TEARDOWN_DONE",
#endif
#if (CFG_SUPPORT_RTWT == 1)
	"MID_RTWT_REQ_FSM_START",
	"MID_RTWT_REQ_FSM_JOIN",
	"MID_RTWT_REQ_FSM_TEARDOWN",
	"MID_RTWT_REQ_IND_TEARDOWN_DONE",
#endif
#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	"MID_ML_TWT_REQ_FSM_START_ALL_LINKS",
	"MID_ML_TWT_REQ_FSM_START_ONE_BY_ONE",
#endif
#if (CFG_SUPPORT_NAN == 1)
	"MID_CNM_NAN_CH_GRANT",
#endif
	"MID_MNY_P2P_LISTEN_OFFLOAD_START",
	"MID_MNY_P2P_LISTEN_OFFLOAD_STOP",
#if ARP_MONITER_ENABLE
#if CFG_QM_ARP_MONITOR_MSG
	"MID_QM_ARP_MONITOR",
#endif /* CFG_QM_ARP_MONITOR_MSG */
#endif /* ARP_MONITER_ENABLE */
	"MID_RSN_FW_DUMP",
	"MID_RSN_MIC_FAIL",
	"MID_UEVENT_REQ",
#if CFG_SUPPORT_TDLS_AUTO
	"MID_TDLS_AUTO",
#endif
#ifdef CFG_AP_GO_DELAY_CARRIER_ON
	"MID_MNY_P2P_NOTIFY_APGO_STARTED",
#endif /* CFG_AP_GO_DELAY_CARRIER_ON */
#if CFG_ENABLE_WIFI_DIRECT
	"MID_MNY_P2P_UPDATE_WMM_PARAMS",
#if (CFG_SUPPORT_SAP_BCN_CRI_UPD == 1)
	"MID_MNY_P2P_BCN_CRI_UPD",
#endif /* CFG_SUPPORT_SAP_BCN_CRI_UPD */
#if (CFG_P2P2_SUPPORT_GC_REQ_CSA == 1)
	"MID_MNY_P2P_GC_CSA_REQ",
#endif /* CFG_P2P2_SUPPORT_GC_REQ_CSA */
#endif /* CFG_ENABLE_WIFI_DIRECT */
#if CFG_FLUSH_TX_PENDING_PKT
	"MID_FLUSH_TX_PENDING_Q",
#endif
};

/*lint -restore */
#endif /* DBG */

/* This message entry will be re-ordered based on the message ID order
 * by invoking mboxInitMsgMap()
 */
static struct MSG_HNDL_ENTRY arMsgMapTable[] = {
	{MID_MNY_CNM_CH_REQ, cnmChMngrRequestPrivilege},
	{MID_MNY_CNM_CH_ABORT, cnmChMngrAbortPrivilege},
	{MID_CNM_AIS_CH_GRANT, aisFsmRunEventChGrant},
#if CFG_ENABLE_WIFI_DIRECT
	/*set in gl_p2p_init.c */
	{MID_CNM_P2P_CH_GRANT, p2pFsmRunEventChGrant},
#else
	{MID_CNM_P2P_CH_GRANT, mboxDummy},
#endif
#if (CFG_SUPPORT_DFS_MASTER == 1)
	{MID_CNM_P2P_RADAR_DETECT, p2pRoleFsmRunEventRadarDet},
#endif
#if CFG_ENABLE_WIFI_DIRECT
	{MID_CNM_P2P_CSA_DONE, p2pRoleFsmRunEventCsaDone},
#endif

	/*--------------------------------------------------*/
	/* SCN Module Mailbox Messages                      */
	/*--------------------------------------------------*/
	{MID_AIS_SCN_SCAN_REQ, scnFsmMsgStart},
	{MID_AIS_SCN_SCAN_REQ_V2, scnFsmMsgStart},
	{MID_AIS_SCN_SCAN_CANCEL, scnFsmMsgAbort},
	{MID_P2P_SCN_SCAN_REQ, scnFsmMsgStart},
	{MID_P2P_SCN_SCAN_REQ_V2, scnFsmMsgStart},
	{MID_P2P_SCN_SCAN_CANCEL, scnFsmMsgAbort},
	{MID_RLM_SCN_SCAN_REQ, scnFsmMsgStart},
	{MID_RLM_SCN_SCAN_REQ_V2, scnFsmMsgStart},
	{MID_RLM_SCN_SCAN_CANCEL, scnFsmMsgAbort},
	{MID_SCN_AIS_SCAN_DONE, aisFsmRunEventScanDone},
#if CFG_ENABLE_WIFI_DIRECT
	/*set in gl_p2p_init.c */
	{MID_SCN_P2P_SCAN_DONE, p2pFsmRunEventScanDone},
#else
	{MID_SCN_P2P_SCAN_DONE, mboxDummy},
#endif

	{MID_SCN_RLM_SCAN_DONE, rlmObssScanDone},

	/*--------------------------------------------------*/
	/* AIS Module Mailbox Messages                      */
	/*--------------------------------------------------*/
	{MID_OID_AIS_FSM_JOIN_REQ, aisFsmRunEventAbort},
	{MID_OID_AIS_FSM_ABORT, aisFsmRunEventAbort},
	{MID_AIS_SAA_FSM_START, saaFsmRunEventStart},
	{MID_OID_SAA_FSM_CONTINUE, saaFsmRunEventFTContinue},
	{MID_OID_SAA_FSM_EXTERNAL_AUTH, saaFsmRunEventExternalAuthDone},
	{MID_AIS_SAA_FSM_ABORT, saaFsmRunEventAbort},
	{MID_SAA_AIS_JOIN_COMPLETE, aisFsmRunEventJoinComplete},

#if CFG_ENABLE_WIFI_DIRECT	/*set in gl_p2p_init.c */
	{MID_P2P_SAA_FSM_START, saaFsmRunEventStart},
	{MID_P2P_SAA_FSM_ABORT, saaFsmRunEventAbort},
	{MID_SAA_P2P_JOIN_COMPLETE, p2pRoleFsmRunEventJoinComplete},	/* V */

	{MID_MNY_P2P_FUN_SWITCH, p2pRoleFsmRunEventSwitchOPMode},
	{MID_MNY_P2P_DEVICE_DISCOVERY, p2pFsmRunEventScanRequest},	/* V */
	{MID_MNY_P2P_CONNECTION_REQ, p2pRoleFsmRunEventConnectionRequest},
	{MID_MNY_P2P_CONNECTION_ABORT, p2pRoleFsmRunEventConnectionAbort},
#if CFG_ENABLE_WIFI_DIRECT
	{MID_MNY_P2P_BEACON_UPDATE, p2pRoleFsmRunEventBeaconUpdate},
	{MID_MNY_P2P_BEACON_REINIT, p2pRoleFsmReInitBeaconAll},
	{MID_MNY_P2P_STOP_AP, p2pRoleFsmRunEventStopAP},
#endif
	{MID_MNY_P2P_CHNL_REQ, p2pDevFsmRunEventChannelRequest},	/* V */
	{MID_MNY_P2P_CHNL_ABORT, p2pDevFsmRunEventChannelAbort},	/* V */
	{MID_MNY_P2P_MGMT_TX, p2pFsmRunEventMgmtFrameTx},	/* V */
	{MID_MNY_P2P_MGMT_TX_CANCEL_WAIT, p2pFsmRunEventTxCancelWait},
	{MID_MNY_P2P_MGMT_FRAME_REGISTER,
		p2pDevFsmRunEventMgmtFrameRegister},
#if CFG_ENABLE_WIFI_DIRECT
	{MID_MNY_P2P_START_AP, p2pRoleFsmRunEventPreStartAP},
#endif
	{MID_MNY_P2P_DEL_IFACE, p2pRoleFsmRunEventDelIface},
	{MID_MNY_P2P_SAP_CHANGE_BW, p2pRoleFsmRunEventChangeBw},
#if (CFG_SUPPORT_DFS_MASTER == 1)
	{MID_MNY_P2P_DFS_CAC, p2pRoleFsmRunEventDfsCac},
	{MID_MNY_P2P_START_CAC, p2pRoleFsmRunEventStartCac},
	{MID_MNY_P2P_STOP_CAC, p2pRoleFsmRunEventStopCac},
#endif
	{MID_MNY_P2P_SET_NEW_CHANNEL, p2pRoleFsmRunEventSetNewChannel},
#if CFG_SUPPORT_WFD
	{MID_MNY_P2P_WFD_CFG_UPDATE, p2pFsmRunEventWfdSettingUpdate},
#endif
	{MID_MNY_P2P_UPDATE_DEV_BSS, p2pDevFsmRunEventUpdateDevBss},
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	{MID_MNY_P2P_ADD_MLD_LINK, p2pRoleFsmRunEventAddMldLink},
	{MID_MNY_P2P_DEL_MLD_LINK, p2pRoleFsmRunEventDelMldLink},
#endif
#endif

	{MID_SAA_AIS_FSM_ABORT, aisFsmRunEventAbort},
	{MID_MNY_AIS_REMAIN_ON_CHANNEL,
		aisFsmRunEventRemainOnChannel},
	{MID_MNY_AIS_CANCEL_REMAIN_ON_CHANNEL,
		aisFsmRunEventCancelRemainOnChannel},
	{MID_MNY_AIS_MGMT_TX, aisFsmRunEventMgmtFrameTx},
	{MID_MNY_AIS_MGMT_TX_CANCEL_WAIT, aisFsmRunEventCancelTxWait},
#if CFG_SUPPORT_802_11V_BTM_OFFLOAD
	{MID_WNM_AIS_BSS_TRANSITION, aisFsmRunEventBssTransition},
#endif
	{MID_OID_WMM_TSPEC_OPERATE, wmmRunEventTSOperate},
	{MID_RRM_REQ_SCHEDULE, rrmRunEventProcessNextRm},
#if CFG_SUPPORT_NCHO
	{MID_MNY_AIS_NCHO_ACTION_FRAME,
		aisFsmRunEventNchoActionFrameTx},
#endif
#if CFG_ENABLE_WIFI_DIRECT
	{MID_MNY_P2P_ACS, p2pRoleFsmRunEventAcs},
#endif
#if (CFG_SUPPORT_TWT == 1)
	{MID_TWT_REQ_FSM_START, twtReqFsmRunEventStart},
	{MID_TWT_REQ_FSM_TEARDOWN, twtReqFsmRunEventTeardown},
	{MID_TWT_REQ_FSM_SUSPEND, twtReqFsmRunEventSuspend},
	{MID_TWT_REQ_FSM_RESUME, twtReqFsmRunEventResume},
	{MID_TWT_REQ_IND_RESULT, twtPlannerRxNegoResult},
	{MID_TWT_REQ_IND_SUSPEND_DONE, twtPlannerSuspendDone},
	{MID_TWT_REQ_IND_RESUME_DONE, twtPlannerResumeDone},
	{MID_TWT_REQ_IND_TEARDOWN_DONE, twtPlannerTeardownDone},
	{MID_TWT_REQ_IND_INFOFRM, twtPlannerRxInfoFrm},
	{MID_TWT_PARAMS_SET, twtPlannerSetParams},
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	{MID_SMPS_ACTION_SET, heRlmProcessSMPSAction},
#endif
#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	{MID_TWT_RESP_PARAMS_SET, twtHotspotPlannerSetParams},
	{MID_TWT_RESP_SETUP_AGRT_TO_FW, twtHotspotPlannerSetupAgrtToFW},
	{MID_TWT_RESP_TEARDOWN_TO_FW, twtHotspotPlannerTeardownToFW},
#endif

#if (CFG_SUPPORT_BTWT == 1)
	{MID_BTWT_REQ_FSM_START, btwtReqFsmRunEventStart},
	{MID_BTWT_REQ_FSM_TEARDOWN, btwtReqFsmRunEventTeardown},
	{MID_BTWT_REQ_IND_TEARDOWN_DONE, btwtPlannerTeardownDone},
#endif

#if (CFG_SUPPORT_RTWT == 1)
	{MID_RTWT_REQ_FSM_START, rtwtReqFsmRunEventStart},
	{MID_RTWT_REQ_FSM_JOIN, rtwtReqFsmRunEventStart},
	{MID_RTWT_REQ_FSM_TEARDOWN, rtwtReqFsmRunEventTeardown},
	{MID_RTWT_REQ_IND_TEARDOWN_DONE, rtwtPlannerTeardownDone},
#endif

#if (CFG_SUPPORT_802_11BE_ML_TWT == 1)
	{MID_ML_TWT_REQ_FSM_START_ALL_LINKS, mltwtReqFsmRunEventStartAllLinks},
	{MID_ML_TWT_REQ_FSM_START_ONE_BY_ONE, mltwtReqFsmRunEventStart},
#endif

#if (CFG_SUPPORT_NAN == 1)
	{MID_CNM_NAN_CH_GRANT, nanDevSendEnableRequest},
#endif

#if CFG_ENABLE_WIFI_DIRECT
	{MID_MNY_P2P_LISTEN_OFFLOAD_START, p2pDevFsmListenOffloadStart},
	{MID_MNY_P2P_LISTEN_OFFLOAD_STOP, p2pDevFsmListenOffloadStop},
#endif
#if ARP_MONITER_ENABLE
#if CFG_QM_ARP_MONITOR_MSG
	{MID_QM_ARP_MONITOR, arpMonHandleMsg},
#endif /* CFG_QM_ARP_MONITOR_MSG */
#endif /* ARP_MONITER_ENABLE */
	{MID_RSN_FW_DUMP, rsnTriggerDumpWTBL},
	{MID_RSN_MIC_FAIL, rsnMicErrorHandleMsg},

#if CFG_SUPPORT_PASN
	{MID_PASN_RESP, pasnHandlePasnResp},
	{MID_PASN_SECURE_RANGING_CTX, pasnHandleSecureRangingCtx},
#endif

	{MID_UEVENT_REQ, kalSendUeventHandler},
#if CFG_SUPPORT_TDLS_AUTO
	{MID_TDLS_AUTO, TdlsAuto},
#endif
#ifdef CFG_AP_GO_DELAY_CARRIER_ON
	{MID_MNY_P2P_NOTIFY_APGO_STARTED, p2pRoleFsmRunEventApGoStarted},
#endif /* CFG_AP_GO_DELAY_CARRIER_ON */
#if CFG_ENABLE_WIFI_DIRECT
	{MID_MNY_P2P_UPDATE_WMM_PARAMS, p2pRoleFsmRunEventUpdateWmmParams},
#if (CFG_SUPPORT_SAP_BCN_CRI_UPD == 1)
	{MID_MNY_P2P_BCN_CRI_UPD, p2pRoleFsmRunEventBcnCriUpd},
#endif /* CFG_SUPPORT_SAP_BCN_CRI_UPD */
#if (CFG_P2P2_SUPPORT_GC_REQ_CSA == 1)
	{MID_MNY_P2P_GC_CSA_REQ, p2pRoleFsmRunEventGcCsaReq},
#endif /* CFG_P2P2_SUPPORT_GC_REQ_CSA */
#endif /* CFG_ENABLE_WIFI_DIRECT */
#if CFG_FLUSH_TX_PENDING_PKT
	{MID_FLUSH_TX_PENDING_Q, nicFreePendingTxMsduHandleMsg},
#endif
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#if DBG
#define MBOX_HNDL_MSG(prAdapter, prMsg) do { \
	if (prMsg->eMsgId >= 0 && prMsg->eMsgId < MID_TOTAL_NUM) { \
		ASSERT(arMsgMapTable[prMsg->eMsgId].pfMsgHndl); \
		if (arMsgMapTable[prMsg->eMsgId].pfMsgHndl) { \
			DBGLOG(CNM, LOUD, \
			"DO MSG [%d: %s]\n", \
			prMsg->eMsgId, apucDebugMsg[prMsg->eMsgId]); \
			arMsgMapTable[prMsg->eMsgId].pfMsgHndl( \
				prAdapter, prMsg); \
		} \
		else { \
		    DBGLOG(CNM, ERROR, "NULL fptr for MSG [%d]\n", \
			prMsg->eMsgId); \
		    cnmMemFree(prAdapter, prMsg); \
		} \
	} \
	else { \
		DBGLOG(CNM, ERROR, "Invalid MSG ID [%d]\n", prMsg->eMsgId); \
		cnmMemFree(prAdapter, prMsg); \
	} \
} while (0)
#else
#define MBOX_HNDL_MSG(prAdapter, prMsg) do { \
	if (prMsg->eMsgId >= 0 && prMsg->eMsgId < MID_TOTAL_NUM) { \
		ASSERT(arMsgMapTable[prMsg->eMsgId].pfMsgHndl); \
		if (arMsgMapTable[prMsg->eMsgId].pfMsgHndl) { \
			DBGLOG(CNM, LOUD, "DO MSG [%d]\n", prMsg->eMsgId); \
			arMsgMapTable[prMsg->eMsgId].pfMsgHndl( \
				prAdapter, prMsg); \
		} \
		else { \
		    DBGLOG(CNM, ERROR, "NULL fptr for MSG [%d]\n", \
			prMsg->eMsgId); \
		    cnmMemFree(prAdapter, prMsg); \
		} \
	} \
	else { \
		DBGLOG(CNM, ERROR, "Invalid MSG ID [%d]\n", prMsg->eMsgId); \
		cnmMemFree(prAdapter, prMsg); \
	} \
} while (0)
#endif
/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mboxInitMsgMap(void)
{
	uint32_t i, idx;
	struct MSG_HNDL_ENTRY rTempEntry;

	ASSERT(ARRAY_SIZE(arMsgMapTable) == MID_TOTAL_NUM);

	for (i = 0; i < MID_TOTAL_NUM; i++) {
		if (arMsgMapTable[i].eMsgId == (enum ENUM_MSG_ID) i)
			continue;
		for (idx = i + 1; idx < MID_TOTAL_NUM; idx++) {
			if (arMsgMapTable[idx].eMsgId == (enum ENUM_MSG_ID) i)
				break;
		}
		ASSERT(idx < MID_TOTAL_NUM);
		if (idx >= MID_TOTAL_NUM)
			continue;

		/* Swap target entry and current entry */
		rTempEntry.eMsgId = arMsgMapTable[idx].eMsgId;
		rTempEntry.pfMsgHndl = arMsgMapTable[idx].pfMsgHndl;

		arMsgMapTable[idx].eMsgId = arMsgMapTable[i].eMsgId;
		arMsgMapTable[idx].pfMsgHndl = arMsgMapTable[i].pfMsgHndl;

		arMsgMapTable[i].eMsgId = rTempEntry.eMsgId;
		arMsgMapTable[i].pfMsgHndl = rTempEntry.pfMsgHndl;
	}

	/* Verify the correctness of final message map */
	for (i = 0; i < MID_TOTAL_NUM; i++) {
		ASSERT(arMsgMapTable[i].eMsgId == (enum ENUM_MSG_ID) i);
		while (arMsgMapTable[i].eMsgId != (enum ENUM_MSG_ID) i)
			;
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mboxSetup(struct ADAPTER *prAdapter,
	       enum ENUM_MBOX_ID eMboxId)
{
	struct MBOX *prMbox;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	if (eMboxId >= MBOX_ID_TOTAL_NUM) {
		DBGLOG(CNM, ERROR, "eMboxId %d is invalid\n", eMboxId);
		return;
	}

	prMbox = &(prAdapter->arMbox[eMboxId]);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_MAILBOX);
	LINK_INITIALIZE(&prMbox->rLinkHead);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_MAILBOX);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
mboxSendMsg(struct ADAPTER *prAdapter,
	    enum ENUM_MBOX_ID eMboxId, struct MSG_HDR *prMsg,
	    enum EUNM_MSG_SEND_METHOD eMethod)
{
	struct MBOX *prMbox;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prMsg);
	if (!prMsg) {
		DBGLOG(CNM, ERROR, "prMsg is NULL\n");
		return;
	}
	if (eMboxId >= MBOX_ID_TOTAL_NUM) {
		DBGLOG(CNM, ERROR, "eMboxId %d is invalid\n", eMboxId);
		return;
	}

	ASSERT(prAdapter);
	if (!prAdapter) {
		DBGLOG(CNM, ERROR, "prAdapter is NULL\n");
		return;
	}

	prMbox = &(prAdapter->arMbox[eMboxId]);

	switch (eMethod) {
	case MSG_SEND_METHOD_BUF:
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_MAILBOX);
		LINK_INSERT_TAIL(&prMbox->rLinkHead, &prMsg->rLinkEntry);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_MAILBOX);

		/* to wake up main service thread */
		GLUE_SET_EVENT(prAdapter->prGlueInfo);

		break;

	case MSG_SEND_METHOD_UNBUF:
		MBOX_HNDL_MSG(prAdapter, prMsg);
		break;

	default:
		ASSERT(0);
		break;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mboxRcvAllMsg(struct ADAPTER *prAdapter,
		   enum ENUM_MBOX_ID eMboxId)
{
	struct MBOX *prMbox;
	struct MSG_HDR *prMsg;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	if (eMboxId >= MBOX_ID_TOTAL_NUM) {
		DBGLOG(CNM, ERROR, "eMboxId %d is invalid\n", eMboxId);
		return;
	}

	prMbox = &(prAdapter->arMbox[eMboxId]);

	while (!LINK_IS_EMPTY(&prMbox->rLinkHead)) {
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_MAILBOX);
		LINK_REMOVE_HEAD(&prMbox->rLinkHead, prMsg,
				 struct MSG_HDR *);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_MAILBOX);

		ASSERT(prMsg);
		if (!prMsg) {
			DBGLOG(CNM, ERROR, "prMsg is NULL\n");
			continue;
		}
#ifdef UT_TEST_MODE
		if (testMBoxRcv(prAdapter, prMsg))
#endif
		MBOX_HNDL_MSG(prAdapter, prMsg);
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mboxInitialize(struct ADAPTER *prAdapter)
{
	uint32_t i;

	ASSERT(prAdapter);

	/* Initialize Mailbox */
	mboxInitMsgMap();

	/* Setup/initialize each mailbox */
	for (i = 0; i < MBOX_ID_TOTAL_NUM; i++)
		mboxSetup(prAdapter, i);

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mboxDestroy(struct ADAPTER *prAdapter)
{
	struct MBOX *prMbox;
	struct MSG_HDR *prMsg;
	uint8_t i;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	for (i = 0; i < MBOX_ID_TOTAL_NUM; i++) {
		prMbox = &(prAdapter->arMbox[i]);

		while (!LINK_IS_EMPTY(&prMbox->rLinkHead)) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_MAILBOX);
			LINK_REMOVE_HEAD(&prMbox->rLinkHead, prMsg,
					 struct MSG_HDR *);
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_MAILBOX);

			ASSERT(prMsg);
			cnmMemFree(prAdapter, prMsg);
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This is dummy function to prevent empty arMsgMapTable[]
 *        for compiling.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mboxDummy(struct ADAPTER *prAdapter,
	       struct MSG_HDR *prMsgHdr)
{
	ASSERT(prAdapter);

	cnmMemFree(prAdapter, prMsgHdr);
}
