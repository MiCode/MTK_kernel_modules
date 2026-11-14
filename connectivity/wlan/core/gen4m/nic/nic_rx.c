// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   nic_rx.c
 *    \brief  Functions that provide many rx-related functions
 *
 *    This file includes the functions used to process RFB and dispatch RFBs to
 *    the appropriate related rx functions for protocols.
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
#include "que_mgt.h"
#include "wnm.h"
#if CFG_SUPPORT_NAN
#include "nan_data_engine.h"
#endif
#include "radiotap.h"
#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
#include "gl_ics.h"
#endif

#if CFG_SUPPORT_RX_PAGE_POOL
#include "hif_pdma.h"
#endif

#if CFG_SUPPORT_CSI
#include "gl_csi.h"
#endif

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

#if CFG_MGMT_FRAME_HANDLING
static PROCESS_RX_MGT_FUNCTION
apfnProcessRxMgtFrame[MAX_NUM_OF_FC_SUBTYPES] = {
#if CFG_SUPPORT_AAA
	aaaFsmRunEventRxAssoc,	/* subtype 0000: Association request */
#else
	NULL,			/* subtype 0000: Association request */
#endif /* CFG_SUPPORT_AAA */
	saaFsmRunEventRxAssoc,	/* subtype 0001: Association response */
#if CFG_SUPPORT_AAA
	aaaFsmRunEventRxAssoc,	/* subtype 0010: Reassociation request */
#else
	NULL,			/* subtype 0010: Reassociation request */
#endif /* CFG_SUPPORT_AAA */
	saaFsmRunEventRxAssoc,	/* subtype 0011: Reassociation response */
#if CFG_ENABLE_WIFI_DIRECT
	bssProcessProbeRequest,	/* subtype 0100: Probe request */
#else
	NULL,			/* subtype 0100: Probe request */
#endif /* CFG_ENABLE_WIFI_DIRECT */
	scanProcessBeaconAndProbeResp,	/* subtype 0101: Probe response */
	NULL,			/* subtype 0110: reserved */
	NULL,			/* subtype 0111: reserved */
	scanProcessBeaconAndProbeResp,	/* subtype 1000: Beacon */
	NULL,			/* subtype 1001: ATIM */
	saaFsmRunEventRxDisassoc,	/* subtype 1010: Disassociation */
	authCheckRxAuthFrameTransSeq,	/* subtype 1011: Authentication */
	saaFsmRunEventRxDeauth,	/* subtype 1100: Deauthentication */
	nicRxProcessActionFrame,	/* subtype 1101: Action */
	NULL,			/* subtype 1110: reserved */
	NULL			/* subtype 1111: reserved */
};
#endif

struct RX_EVENT_HANDLER arEventTable[] = {
	{EVENT_ID_RX_ADDBA,	qmHandleEventRxAddBa},
#if CFG_SUPPORT_DBDC
	{EVENT_ID_DBDC_SWITCH_DONE, cnmDbdcEventHwSwitchDone},
#endif
	{EVENT_ID_RX_DELBA,	qmHandleEventRxDelBa},
	{EVENT_ID_LINK_QUALITY, nicEventLinkQuality},
	{EVENT_ID_LAYER_0_EXT_MAGIC_NUM, nicEventLayer0ExtMagic},
	{EVENT_ID_MIC_ERR_INFO,	nicEventMicErrorInfo},
	{EVENT_ID_SCAN_DONE, nicEventScanDone},
	{EVENT_ID_SCHED_SCAN_DONE, nicEventSchedScanDone},
	{EVENT_ID_TX_DONE, nicTxProcessTxDoneEvent},
	{EVENT_ID_SLEEPY_INFO, nicEventSleepyNotify},
	{EVENT_ID_STATISTICS, nicEventStatistics},
	{EVENT_ID_TPUT_INFO, nicEventTputFactorHandler},
	{EVENT_ID_WTBL_INFO, nicEventWlanInfo},
	{EVENT_ID_MIB_INFO, nicEventMibInfo},
#if (CFG_WIFI_GET_MCS_INFO == 1)
	{EVENT_ID_TX_MCS_INFO, nicEventTxMcsInfo},
#endif
	{EVENT_ID_CH_PRIVILEGE, cnmChMngrHandleChEvent},
	{EVENT_ID_BSS_ABSENCE_PRESENCE, qmHandleEventBssAbsencePresence},
#if CFG_ENABLE_WIFI_DIRECT
	{EVENT_ID_STA_CHANGE_PS_MODE, qmHandleEventStaChangePsMode},
	{EVENT_ID_STA_UPDATE_FREE_QUOTA, qmHandleEventStaUpdateFreeQuota},
#endif
	{EVENT_ID_BSS_BEACON_TIMEOUT, nicEventBeaconTimeout},
#if CFG_ENABLE_WIFI_DIRECT
	{EVENT_ID_UPDATE_NOA_PARAMS, nicEventUpdateNoaParams},
	{EVENT_ID_STA_AGING_TIMEOUT, nicEventStaAgingTimeout},
	{EVENT_ID_AP_OBSS_STATUS, nicEventApObssStatus},
#endif
	{EVENT_ID_ROAMING_STATUS, nicEventRoamingStatus},
	{EVENT_ID_SEND_DEAUTH, nicEventSendDeauth},
	{EVENT_ID_UPDATE_RDD_STATUS, nicEventUpdateRddStatus},
	{EVENT_ID_UPDATE_BWCS_STATUS, nicEventUpdateBwcsStatus},
	{EVENT_ID_UPDATE_BCM_DEBUG, nicEventUpdateBcmDebug},
	{EVENT_ID_ADD_PKEY_DONE, nicEventAddPkeyDone},
	{EVENT_ID_DEBUG_MSG, nicEventDebugMsg},
#if CFG_SUPPORT_TDLS
	{EVENT_ID_TDLS, nicEventTdls},
#endif
#if (CFG_SUPPORT_HE_ER == 1)
	{EVENT_ID_BSS_ER_TX_MODE, bssProcessErTxModeEvent},
#endif

#if CFG_SUPPORT_LLS
	{EVENT_ID_STATS_LLS, nicEventStatsLinkStats},
#endif
	{EVENT_ID_RSSI_MONITOR, nicEventRssiMonitor},
#if CFG_SUPPORT_FW_DROP_SSN
	{EVENT_ID_FW_DROP_SSN, nicEventFwDropSSN},
#endif /* CFG_SUPPORT_FW_DROP_SSN */
	{EVENT_ID_DUMP_MEM, nicEventDumpMem},
#if CFG_CE_ASSERT_DUMP
	{EVENT_ID_ASSERT_DUMP, nicEventAssertDump},
#endif
#if CFG_SUPPORT_BAR_DELAY_INDICATION
	{EVENT_ID_DELAY_BAR, nicEventHandleDelayBar},
#endif
	{EVENT_ID_HIF_CTRL, nicEventHifCtrl},
	{EVENT_ID_RDD_SEND_PULSE, nicEventRddSendPulse},
#if (CFG_SUPPORT_DFS_MASTER == 1)
	{EVENT_ID_RDD_REPORT, cnmRadarDetectEvent},
#endif
#if CFG_SUPPORT_IDC_CH_SWITCH
	{EVENT_ID_LTE_IDC_REPORT, cnmIdcDetectHandler},
#endif
#if CFG_ENABLE_WIFI_DIRECT
	{EVENT_ID_CSA_DONE, cnmCsaDoneEvent},
#endif
#if CFG_ENABLE_WIFI_DIRECT
	{EVENT_ID_P2P_LO_STOP, p2pDevListenOffloadStopHandler},
#endif
	{EVENT_ID_UPDATE_COEX_PHYRATE, nicEventUpdateCoexPhyrate},
	{EVENT_ID_UPDATE_COEX_STATUS, nicEventUpdateCoexStatus},
	{EVENT_ID_TX_ADDBA, qmHandleEventTxAddBa},
	{EVENT_ID_GET_CNM, nicEventCnmInfo},
#if CFG_SUPPORT_SMART_GEAR
	{EVENT_ID_SG_STATUS, cnmEventSGStatus},
#endif
	{EVENT_ID_COEX_CTRL, nicEventCoexCtrl},
#if (CFG_WOW_SUPPORT == 1)
	{EVENT_ID_WOW_WAKEUP_REASON, nicEventWowWakeUpReason},
#endif
	{EVENT_ID_OPMODE_CHANGE, cnmOpmodeEventHandler},
#if (CFG_SUPPORT_DFS_MASTER == 1)
	{EVENT_ID_RDD_OPMODE_CHANGE, cnmRddOpmodeEventHandler},
#endif
#if CFG_FAST_PATH_SUPPORT
	{EVENT_ID_FAST_PATH, fpEventHandler},
#endif
#if CFG_SUPPORT_NAN
	{EVENT_ID_NAN_EXT_EVENT, nicNanEventDispatcher},
#endif
#if CFG_SUPPORT_CSI
	{EVENT_ID_CSI_DATA, nicEventCSIData},
#endif
#if CFG_SUPPORT_802_PP_DSCB
	{EVENT_ID_STATIC_PP_DSCB, nicEventUpdateStaticPPDscb},
#endif
	{EVENT_ID_REPORT_U_EVENT, nicEventReportUEvent},
#if (CFG_COALESCING_INTERRUPT == 1)
	{EVENT_ID_PF_CF_COALESCING_INT_DONE, nicEventCoalescingIntDone},
#endif
#if CFG_SUPPORT_RTT
	{EVENT_ID_RTT_RESULT, nicEventRttResult},
	{EVENT_ID_RTT_DONE, nicEventRttDone},
#endif
#if (CFG_VOLT_INFO == 1)
	{EVEN_ID_GET_VOLT_INFO, nicEventGetVnf},
#endif
#if CFG_SUPPORT_MLR
	{EVENT_ID_MLR_FSM_UPDATE, mlrEventMlrFsmUpdateHandler},
#endif
#if CFG_SUPPORT_WIFI_POWER_METRICS
	{EVENT_ID_POWER_METRICS, nicEventPowerMetricsStatGetInfo},
#endif
#if (CFG_SURVEY_DUMP_FULL_CHANNEL == 1)
	{EVENT_ID_CHANNEL_TIME, nicEventChannelTime}
#endif

#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
	{ EVENT_ID_MDNS_RECORD, nicEventMdnsStats},
#endif
#endif
#if (CFG_HW_DETECT_REPORT == 1)
	{EVENT_ID_HW_DETECT_REPROT, nicEventHwDetectReport},
#endif /* CFG_HW_DETECT_REPORT */
};

uint32_t arEventTableSize = ARRAY_SIZE(arEventTable);

static const struct ACTION_FRAME_SIZE_MAP arActionFrameReservedLen[] = {
	{(uint16_t)(CATEGORY_QOS_ACTION | ACTION_QOS_MAP_CONFIGURE << 8),
	 sizeof(struct _ACTION_QOS_MAP_CONFIGURE_FRAME)},
	{(uint16_t)(CATEGORY_PUBLIC_ACTION | ACTION_PUBLIC_20_40_COEXIST << 8),
	 OFFSET_OF(struct ACTION_20_40_COEXIST_FRAME, rChnlReport)},
	{(uint16_t)
	 (CATEGORY_PUBLIC_ACTION | ACTION_PUBLIC_EX_CH_SW_ANNOUNCEMENT << 8),
	 sizeof(struct ACTION_EX_CHANNEL_SWITCH_FRAME)},
	{(uint16_t)
	 (CATEGORY_PUBLIC_ACTION | ACTION_PUBLIC_VENDOR_SPECIFIC << 8),
	 sizeof(struct WLAN_PUBLIC_VENDOR_ACTION_FRAME)},
	{(uint16_t)(CATEGORY_HT_ACTION | ACTION_HT_NOTIFY_CHANNEL_WIDTH << 8),
	 sizeof(struct ACTION_NOTIFY_CHNL_WIDTH_FRAME)},
	{(uint16_t)(CATEGORY_HT_ACTION | ACTION_HT_SM_POWER_SAVE << 8),
	 sizeof(struct ACTION_SM_POWER_SAVE_FRAME)},
	{(uint16_t)(CATEGORY_SA_QUERY_ACTION | ACTION_SA_QUERY_REQUEST << 8),
	 sizeof(struct ACTION_SA_QUERY_FRAME)},
	{(uint16_t)
	 (CATEGORY_WNM_ACTION | ACTION_WNM_TIMING_MEASUREMENT_REQUEST << 8),
	 sizeof(struct ACTION_WNM_TIMING_MEAS_REQ_FRAME)},
	{(uint16_t)(CATEGORY_FT_ACTION | ACTION_FT_REQUEST << 8),
	 sizeof(struct ACTION_FT_REQ_ACTION_FRAME)},
	{(uint16_t)(CATEGORY_FT_ACTION | ACTION_FT_RESPONSE << 8),
	 sizeof(struct ACTION_FT_RESP_ACTION_FRAME)},
	{(uint16_t)(CATEGORY_SPEC_MGT | ACTION_MEASUREMENT_REQ << 8),
	 sizeof(struct ACTION_SM_REQ_FRAME)},
	{(uint16_t)(CATEGORY_SPEC_MGT | ACTION_MEASUREMENT_REPORT << 8),
	 sizeof(struct ACTION_SM_REQ_FRAME)},
	{(uint16_t)(CATEGORY_SPEC_MGT | ACTION_TPC_REQ << 8),
	 sizeof(struct ACTION_SM_REQ_FRAME)},
	{(uint16_t)(CATEGORY_SPEC_MGT | ACTION_TPC_REPORT << 8),
	 sizeof(struct ACTION_SM_REQ_FRAME)},
	{(uint16_t)(CATEGORY_SPEC_MGT | ACTION_CHNL_SWITCH << 8),
	 sizeof(struct ACTION_SM_REQ_FRAME)},
	{(uint16_t)
	 (CATEGORY_VHT_ACTION | ACTION_OPERATING_MODE_NOTIFICATION << 8),
	 sizeof(struct ACTION_OP_MODE_NOTIFICATION_FRAME)},
#if (CFG_SUPPORT_TWT == 1)
#if (CFG_SUPPORT_BTWT == 1)
	{(uint16_t)(CATEGORY_S1G_ACTION | ACTION_S1G_TWT_SETUP << 8),
	sizeof(struct _ACTION_BTWT_SETUP_FRAME)},
#else
	{(uint16_t)(CATEGORY_S1G_ACTION | ACTION_S1G_TWT_SETUP << 8),
	sizeof(struct _ACTION_TWT_SETUP_FRAME)},
#endif
	{(uint16_t)(CATEGORY_S1G_ACTION | ACTION_S1G_TWT_TEARDOWN << 8),
	 sizeof(struct _ACTION_TWT_TEARDOWN_FRAME)},
	{(uint16_t)(CATEGORY_S1G_ACTION | ACTION_S1G_TWT_INFORMATION << 8),
	 sizeof(struct _ACTION_TWT_INFO_FRAME)},
#endif
	{(uint16_t)(CATEGORY_RM_ACTION | RM_ACTION_RM_REQUEST << 8),
	 sizeof(struct ACTION_RM_REQ_FRAME)},
	{(uint16_t)(CATEGORY_RM_ACTION | RM_ACTION_REIGHBOR_RESPONSE << 8),
	 sizeof(struct ACTION_NEIGHBOR_REPORT_FRAME)},
	{(uint16_t)(CATEGORY_WME_MGT_NOTIFICATION | ACTION_ADDTS_RSP << 8),
	 sizeof(struct WMM_ACTION_TSPEC_FRAME)},
	{(uint16_t)(CATEGORY_WME_MGT_NOTIFICATION | ACTION_DELTS << 8),
	 sizeof(struct WMM_ACTION_TSPEC_FRAME)},
#if CFG_SUPPORT_NAN
	{(uint16_t)(CATEGORY_PROTECTED_DUAL_OF_PUBLIC_ACTION |
		ACTION_PUBLIC_VENDOR_SPECIFIC << 8),
	 sizeof(struct _NAN_ACTION_FRAME_T)},
#endif
#if CFG_FAST_PATH_SUPPORT
	{(uint16_t)(CATEGORY_ROBUST_AV_STREAMING_ACTION | ACTION_MSCS_RSP << 8),
	sizeof(struct ACTION_MSCS_RSP_FRAME)},
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
#if (CFG_SUPPORT_802_11BE_T2LM == 1)
	{(uint16_t)(CATEGORY_PROTECTED_EHT_ACTION | TID2LINK_REQUEST << 8),
	sizeof(struct ACTION_T2LM_REQ_FRAME)},
	{(uint16_t)(CATEGORY_PROTECTED_EHT_ACTION | TID2LINK_RESPONSE << 8),
	sizeof(struct ACTION_T2LM_RSP_FRAME)},
	{(uint16_t)(CATEGORY_PROTECTED_EHT_ACTION | TID2LINK_TEARDOWN << 8),
	sizeof(struct ACTION_T2LM_TEARDOWN_FRAME)},
#endif
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	{(uint16_t)(CATEGORY_PROTECTED_EHT_ACTION | EPCS_ENABLE_REQUEST << 8),
	sizeof(struct ACTION_EPCS_REQ_FRAME)},
	{(uint16_t)(CATEGORY_PROTECTED_EHT_ACTION | EPCS_ENABLE_RESPONSE << 8),
	sizeof(struct ACTION_EPCS_RSP_FRAME)},
#endif
#endif
#if CFG_FAST_PATH_SUPPORT
	{(uint16_t)(CATEGORY_VENDOR_SPECIFIC_PROTECTED_ACTION),
	sizeof(struct ACTION_VENDOR_SPEC_PROTECTED_FRAME)},
#endif
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

#if CFG_DYNAMIC_RFB_ADJUSTMENT
static void nicRxReturnUnUseRFB(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialize the RFBs
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxInitialize(struct ADAPTER *prAdapter)
{
	struct RX_CTRL *prRxCtrl;
	struct SW_RFB *prSwRfb = (struct SW_RFB *) NULL;
	uint32_t i;

	ASSERT(prAdapter);
	prRxCtrl = &prAdapter->rRxCtrl;

#if CFG_SUPPORT_RX_PAGE_POOL
	kalCreatePagePool(prAdapter->prGlueInfo);
#endif /* CFG_SUPPORT_RX_PAGE_POOL */

	/* 4 <0> Clear allocated memory. */
	kalMemZero(prRxCtrl->prRxCached,
		   sizeof(struct SW_RFB[CFG_RX_MAX_PKT_NUM]));

	/* 4 <1> Initialize the RFB lists */
	QUEUE_INITIALIZE(&prRxCtrl->rFreeSwRfbList);
	QUEUE_INITIALIZE(&prRxCtrl->rReceivedRfbList);
	QUEUE_INITIALIZE(&prRxCtrl->rIndicatedRfbList);
#if CFG_DYNAMIC_RFB_ADJUSTMENT
	QUEUE_INITIALIZE(&prRxCtrl->rUnUseRfbList);
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */

#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
	kalSetPagePoolPageNum(CFG_RX_MAX_PKT_NUM - nicRxGetUnUseCnt(prAdapter));
#endif /* CFG_SUPPORT_DYNAMIC_PAGE_POOL */
	for (i = 0; i < CFG_RX_MAX_PKT_NUM; i++) {
		prSwRfb = &prRxCtrl->prRxCached[i];
		prRxCtrl->aprSwRfbPool[i] = prSwRfb;
#if CFG_RFB_TRACK
		RX_RFB_TRACK_INIT(prAdapter, prSwRfb, i);
#endif /* CFG_RFB_TRACK */
		if (RX_GET_UNUSE_RFB_CNT(prRxCtrl) ==
		    nicRxGetUnUseCnt(prAdapter) &&
		    nicRxSetupRFB(prAdapter, prSwRfb)) {
			DBGLOG(RX, ERROR,
			       "nicRxInitialize failed: Cannot allocate packet buffer for SwRfb!\n");
			return;
		}
		nicRxReturnRFB(prAdapter, prSwRfb);
	}

	if (RX_GET_FREE_RFB_CNT(prRxCtrl) !=
		(CFG_RX_MAX_PKT_NUM - nicRxGetUnUseCnt(prAdapter)))
		ASSERT_NOMEM();

	/* 4 <2> Clear all RX counters */
	RX_RESET_ALL_CNTS(prRxCtrl);
	RX_RESET_ALL_PKT_CNTS(prRxCtrl);
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	RX_RRO_RESET_ALL_CNTS(prRxCtrl);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	prRxCtrl->pucRxCoalescingBufPtr =
		prAdapter->pucCoalescingBufCached;
}				/* end of nicRxInitialize() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Uninitialize the RFBs
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxUninitialize(struct ADAPTER *prAdapter)
{
	struct RX_CTRL *prRxCtrl;
	struct SW_RFB *prSwRfb = (struct SW_RFB *) NULL;
	uint32_t i;

	ASSERT(prAdapter);
	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);

	nicRxFlush(prAdapter);

	for (i = 0; i < CFG_RX_MAX_PKT_NUM; i++) {
		prSwRfb = prRxCtrl->aprSwRfbPool[i];
		if (prSwRfb) {
			if (prSwRfb->pvPacket)
				kalPacketFree(prAdapter->prGlueInfo,
				prSwRfb->pvPacket);
			prSwRfb->pvPacket = NULL;
		}
	}

#if CFG_SUPPORT_RX_PAGE_POOL
	kalReleasePagePool(prAdapter->prGlueInfo);
#endif /* CFG_SUPPORT_RX_PAGE_POOL */
}				/* end of nicRxUninitialize() */

void nicRxFillSSN(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	if (prSwRfb->fgHdrTran) {
		if (!(prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4)))
			return;

		prSwRfb->u2SSN = HAL_RX_STATUS_GET_SEQFrag_NUM(
			prSwRfb->prRxStatusGroup4) >> RX_STATUS_SEQ_NUM_OFFSET;
	} else {
		struct WLAN_MAC_HEADER *prWlanHeader;

		prWlanHeader = (struct WLAN_MAC_HEADER *) prSwRfb->pvHeader;
		if (!prWlanHeader)
			return;

		prSwRfb->u2SSN = prWlanHeader->u2SeqCtrl
					>> MASK_SC_SEQ_NUM_OFFSET;
	}
}

void nicRxFillEthTypeLen(struct SW_RFB *prSwRfb)
{
	struct ETH_FRAME *prEth;

	if (prSwRfb->fgDataFrame == FALSE ||
		prSwRfb->u2PacketLen <= ETHER_HEADER_LEN)
		return;

	prEth = prSwRfb->pvHeader;
	if (!prEth)
		return;

	prSwRfb->u2EthTypeLen = NTOHS(prEth->u2TypeLen);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Fill RFB
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb   specify the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxFillRFB(struct ADAPTER *prAdapter,
		  struct SW_RFB *prSwRfb)
{
	struct RX_DESC_OPS_T *prRxDescOps = prAdapter->chip_info->prRxDescOps;

	if (prRxDescOps->nic_rxd_fill_rfb)
		prRxDescOps->nic_rxd_fill_rfb(prAdapter, prSwRfb);
	else
		DBGLOG(RX, ERROR,
			"%s:: no nic_rxd_fill_rfb??\n",
			__func__);

	nicRxFillSSN(prAdapter, prSwRfb);
	nicRxFillEthTypeLen(prSwRfb);
}

/**
 * nicRxProcessRxv() - function to parse RXV for rate information
 * @prAdapter: pointer to adapter
 * @prSwRfb: RFB of received frame
 *
 * If parsed data will be saved in
 * prAdapter->arStaRec[prSwRfb->ucStaRecIdx].u4RxV[*], then can be used
 * for calling wlanGetRxRate().
 */
void nicRxProcessRxv(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb)
{
#if (CFG_SUPPORT_MSP == 1)
	struct mt66xx_chip_info *prChipInfo;
	void *pvPacket;
	uint8_t *pucEthDestAddr;
	struct WIFI_VAR *prWifiVar;

	prChipInfo = prAdapter->chip_info;

	if (!prChipInfo || !prChipInfo->asicRxProcessRxvforMSP)
		return;

	/* ignore non-data frame */
	if (!prSwRfb->fgDataFrame)
		return;

	pucEthDestAddr = prSwRfb->pvHeader;
	if (!pucEthDestAddr)
		return;

	pvPacket = prSwRfb->pvPacket;
	if (!pvPacket)
		return;

	/* Ignore BMC pkt */
	if (prSwRfb->fgIsBC || prSwRfb->fgIsMC ||
		IS_BMCAST_MAC_ADDR(pucEthDestAddr))
		return;

	/* Ignore filtered pkt, such as ARP */
	prWifiVar = &prAdapter->rWifiVar;
	if (GLUE_IS_PKT_FLAG_SET(pvPacket) & prWifiVar->u4RxRateProtoFilterMask
		&& wlanIsRxLowRate(prAdapter, prSwRfb)) {

		DBGLOG(RX, TEMP, "u4RxRateProtoFilterMask:%u, proto:%u\n",
			prWifiVar->u4RxRateProtoFilterMask,
			GLUE_IS_PKT_FLAG_SET(pvPacket));
		return;
	}

	prChipInfo->asicRxProcessRxvforMSP(prAdapter, prSwRfb);
#endif /* CFG_SUPPORT_MSP == 1 */

/* fos_change begin */
#if CFG_SUPPORT_STAT_STATISTICS
	nicRxGetNoiseLevelAndLastRate(prAdapter, prSwRfb);
#endif /* fos_change end */

#if CFG_SUPPORT_PERF_IND
	nicRxPerfIndProcessRXV(prAdapter, prSwRfb,
		GLUE_GET_PKT_BSS_IDX(prSwRfb->pvPacket));
#endif
}

#if CFG_TCP_IP_CHKSUM_OFFLOAD || CFG_TCP_IP_CHKSUM_OFFLOAD_NDIS_60
/*----------------------------------------------------------------------------*/
/*!
 * @brief Fill checksum status in RFB
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 * @param u4TcpUdpIpCksStatus specify the Checksum status
 *
 * @return (none)
 *
 * Set values in prSwRfb->aeCSUM for IPv4, IPv6, TCP, and UDP,
 * with CSUM_RES_NONE, CSUM_RES_SUCCESS, or CSUM_RES_FAILED.
 */
/*----------------------------------------------------------------------------*/
void nicRxFillChksumStatus(struct ADAPTER *prAdapter,
			   struct SW_RFB *prSwRfb)
{
	struct RX_CSO_REPORT_T *rReport;
	uint32_t u4TcpUdpIpCksStatus;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	u4TcpUdpIpCksStatus = prSwRfb->u4TcpUdpIpCksStatus;
	rReport = (struct RX_CSO_REPORT_T *) &u4TcpUdpIpCksStatus;
	DBGLOG_LIMITED(RX, LOUD,
	       "RX_IPV4_STATUS=%d, RX_TCP_STATUS=%d, RX_UDP_STATUS=%d\n",
	       rReport->u4IpV4CksStatus, rReport->u4TcpCksStatus,
	       rReport->u4UdpCksStatus);
	DBGLOG_LIMITED(RX, LOUD,
		"RX_IPV4_TYPE=%d, RX_IPV6_TYPE=%d, RX_TCP_TYPE=%d, RX_UDP_TYPE=%d\n",
	  rReport->u4IpV4CksType, rReport->u4IpV6CksType,
	  rReport->u4TcpCksType, rReport->u4UdpCksType);

	if (prAdapter->u4CSUMFlags == CSUM_NOT_SUPPORTED)
		return;

	/**
	 * In nicRxSetupRFB(), prSwRfb->aeCSUM are all zeroed with the SW_RFB,
	 * i.e., CSUM_RES_NONE, by kalMemZero.
	 */

	if (u4TcpUdpIpCksStatus & RX_CS_FLAG_NOT_DONE ||
	    (u4TcpUdpIpCksStatus & RX_CS_TYPE_TCP &&
	     u4TcpUdpIpCksStatus & RX_CS_TYPE_UDP))
		return;

	if (u4TcpUdpIpCksStatus & RX_CS_TYPE_IPv4) {
		prSwRfb->aeCSUM[CSUM_TYPE_IPV4] = CSUM_RES_SUCCESS;
		if (unlikely(u4TcpUdpIpCksStatus & RX_CS_STATUS_IP))
			prSwRfb->aeCSUM[CSUM_TYPE_IPV4] = CSUM_RES_FAILED;
	} else if (u4TcpUdpIpCksStatus & RX_CS_TYPE_IPv6) {
		/* No IP layer checksum for IPv6, always success. */
		prSwRfb->aeCSUM[CSUM_TYPE_IPV6] = CSUM_RES_SUCCESS;
	}

	if (u4TcpUdpIpCksStatus & RX_CS_TYPE_TCP) {
		prSwRfb->aeCSUM[CSUM_TYPE_TCP] = CSUM_RES_SUCCESS;
		if (unlikely(u4TcpUdpIpCksStatus & RX_CS_STATUS_TCP))
			prSwRfb->aeCSUM[CSUM_TYPE_TCP] = CSUM_RES_FAILED;
	} else if (u4TcpUdpIpCksStatus & RX_CS_TYPE_UDP) {
		prSwRfb->aeCSUM[CSUM_TYPE_UDP] = CSUM_RES_SUCCESS;
		if (unlikely(u4TcpUdpIpCksStatus & RX_CS_STATUS_UDP))
			prSwRfb->aeCSUM[CSUM_TYPE_UDP] = CSUM_RES_FAILED;
	}
}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

/*----------------------------------------------------------------------------*/
/*!
 * \brief nicRxClearFrag() is used to clean all fragments in the fragment cache.
 *
 * \param[in] prAdapter       pointer to the Adapter handler
 * \param[in] prStaRec        The fragment cache is stored under station record.
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxClearFrag(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec)
{
	int i, j;
	struct FRAG_INFO *prFragInfo;

	for (i = 0; i < TID_NUM; i++) {
		for (j = 0; j < MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS; j++) {
			prFragInfo = &prStaRec->rFragInfo[i][j];

			if (prFragInfo->pr1stFrag) {
				nicRxReturnRFB(prAdapter,
					prFragInfo->pr1stFrag);
				prFragInfo->pr1stFrag = (struct SW_RFB *)NULL;
			}
		}
	}

	TRACE_FUNC(RX, TRACE, "%s\n");
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief rxDefragMPDU() is used to defragment the incoming packets.
 *
 * \param[in] prSWRfb        The RFB which is being processed.
 * \param[in] UINT_16     u2FrameCtrl
 *
 * \retval NOT NULL  Receive the last fragment data
 * \retval NULL      Receive the fragment packet which is not the last
 */
/*----------------------------------------------------------------------------*/
struct SW_RFB *nicRxDefragMPDU(struct ADAPTER *prAdapter,
	struct SW_RFB *prSWRfb, struct QUE *prReturnedQue)
{

	struct SW_RFB *prOutputSwRfb = (struct SW_RFB *) NULL;
	struct RX_CTRL *prRxCtrl;
	struct FRAG_INFO *prFragInfo;
	uint32_t i = 0, j;
	uint16_t u2SeqCtrl, u2FrameCtrl;
	uint16_t u2SeqNo;
	uint8_t ucFragNo;
	u_int8_t fgFirst = FALSE;
	u_int8_t fgLast = FALSE;
	OS_SYSTIME rCurrentTime;
	struct WLAN_MAC_HEADER *prWlanHeader = NULL;
	struct WLAN_MAC_HEADER_QOS *prWlanHeaderQos = NULL;
	struct WLAN_MAC_HEADER_A4_QOS *prWlanHeaderA4Qos = NULL;
	void *prRxStatus = NULL;
	struct HW_MAC_RX_STS_GROUP_4 *prRxStatusGroup4 = NULL;
	struct STA_RECORD *prStaRec;
	uint8_t ucTid = 0;
#if CFG_SUPPORT_FRAG_AGG_VALIDATION
	uint8_t ucSecMode = CIPHER_SUITE_NONE;
	uint64_t u8PN;
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

	ASSERT(prSWRfb);

	prRxCtrl = &prAdapter->rRxCtrl;
	prStaRec = prSWRfb->prStaRec;

	prRxStatus = prSWRfb->prRxStatus;
	ASSERT(prRxStatus);

	if (prSWRfb->fgHdrTran == FALSE) {
		prWlanHeader = (struct WLAN_MAC_HEADER *)
					prSWRfb->pvHeader;
		prWlanHeaderQos = (struct WLAN_MAC_HEADER_QOS *)
					prSWRfb->pvHeader;
		prWlanHeaderA4Qos = (struct WLAN_MAC_HEADER_A4_QOS *)
					prSWRfb->pvHeader;
		u2FrameCtrl = prWlanHeader->u2FrameCtrl;
		if (RXM_IS_QOS_DATA_FRAME(u2FrameCtrl)) {
			if (RXM_IS_FROM_DS_TO_DS(u2FrameCtrl)) {
				ucTid = (prWlanHeaderA4Qos->
					u2QosCtrl & MASK_QC_TID);
			} else {
				ucTid = (prWlanHeaderQos->
					u2QosCtrl & MASK_QC_TID);
			}
		} else
			ucTid = TID_NUM;
	} else {
		prRxStatusGroup4 = prSWRfb->prRxStatusGroup4;
		prSWRfb->u2SequenceControl = HAL_RX_STATUS_GET_SEQFrag_NUM(
						     prRxStatusGroup4);
		u2FrameCtrl = HAL_RX_STATUS_GET_FRAME_CTL_FIELD(
				      prRxStatusGroup4);
		if (RXM_IS_QOS_DATA_FRAME(u2FrameCtrl))
			ucTid = prSWRfb->ucTid;
		else
			ucTid = TID_NUM;
	}
	u2SeqCtrl = prSWRfb->u2SequenceControl;
	u2SeqNo = u2SeqCtrl >> MASK_SC_SEQ_NUM_OFFSET;
	ucFragNo = (uint8_t) (u2SeqCtrl & MASK_SC_FRAG_NUM);
	prSWRfb->u2FrameCtrl = u2FrameCtrl;
	prSWRfb->ucTid = ucTid;

#if (CFG_SUPPORT_BALANCE_MLRV2 == 1)
	MLR_DBGLOG(prAdapter, RX, DEBUG,
		   "SN:%d FragNo:%d MoreFrag:%04x FC:%04x SQ:%04x fgHdrTran:%d\n",
		   u2SeqNo,
		   ucFragNo,
		   (uint16_t) (u2FrameCtrl & MASK_FC_MORE_FRAG),
		   u2FrameCtrl,
		   u2SeqCtrl,
		   prSWRfb->fgHdrTran);
#endif

	if (!(u2FrameCtrl & MASK_FC_MORE_FRAG)) {
		/* The last fragment frame */
		if (ucFragNo) {
			DBGLOG(RX, LOUD,
			       "FC %04x M %04x SQ %04x\n", u2FrameCtrl,
			       (uint16_t) (u2FrameCtrl & MASK_FC_MORE_FRAG),
			       u2SeqCtrl);
			fgLast = TRUE;
		}
		/* Non-fragment frame */
		else
			return prSWRfb;
	}
	/* The fragment frame except the last one */
	else {
		if (ucFragNo == 0) {
			DBGLOG(RX, LOUD,
			       "FC %04x M %04x SQ %04x\n", u2FrameCtrl,
			       (uint16_t) (u2FrameCtrl & MASK_FC_MORE_FRAG),
			       u2SeqCtrl);
			fgFirst = TRUE;
		} else {
			DBGLOG(RX, LOUD,
			       "FC %04x M %04x SQ %04x\n", u2FrameCtrl,
			       (uint16_t) (u2FrameCtrl & MASK_FC_MORE_FRAG),
			       u2SeqCtrl);
		}
	}

	GET_CURRENT_SYSTIME(&rCurrentTime);

	/* check cipher suite to set if we need to get PN */
#if CFG_SUPPORT_FRAG_AGG_VALIDATION
	if (prSWRfb->ucSecMode == CIPHER_SUITE_TKIP
		|| prSWRfb->ucSecMode == CIPHER_SUITE_TKIP_WO_MIC
		|| prSWRfb->ucSecMode == CIPHER_SUITE_CCMP
		|| prSWRfb->ucSecMode == CIPHER_SUITE_CCMP_W_CCX
		|| prSWRfb->ucSecMode == CIPHER_SUITE_CCMP_256
		|| prSWRfb->ucSecMode == CIPHER_SUITE_GCMP_128
		|| prSWRfb->ucSecMode == CIPHER_SUITE_GCMP_256) {
		ucSecMode = prSWRfb->ucSecMode;
		if (!qmRxPNtoU64(prSWRfb->prRxStatusGroup1->aucPN,
			CCMPTSCPNNUM, &u8PN)) {
			DBGLOG(QM, ERROR, "PN2U64 failed\n");
			/* should not enter here, just fallback */
			ucSecMode = CIPHER_SUITE_NONE;
		}
	}
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */


	for (j = 0; j < MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS; j++) {
		prFragInfo = &prStaRec->rFragInfo[prSWRfb->ucTid][j];
		if (prFragInfo->pr1stFrag) {
			/* I. If the receive timer for the MSDU or MMPDU that
			 * is stored in the fragments queue exceeds
			 * dot11MaxReceiveLifetime, we discard the uncompleted
			 * fragments.
			 * II. If we didn't receive the last MPDU for a period,
			 * we use this function for remove frames.
			 */
			if (CHECK_FOR_EXPIRATION(rCurrentTime,
				prFragInfo->rReceiveLifetimeLimit)) {
				prFragInfo->pr1stFrag->eDst =
					RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(prReturnedQue,
						prFragInfo->pr1stFrag);

				prFragInfo->pr1stFrag = (struct SW_RFB *) NULL;
			}
		}
	}

	for (i = 0; i < MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS; i++) {
		prFragInfo = &prStaRec->rFragInfo[prSWRfb->ucTid][i];

		if (fgFirst) {	/* looking for timed-out frag buffer */

			if (prFragInfo->pr1stFrag == (struct SW_RFB *)
			    NULL)	/* find a free frag buffer */
				break;
		} else {
			/* looking for a buffer with desired next seqctrl */

			if (prFragInfo->pr1stFrag == (struct SW_RFB *) NULL)
				continue;

			if (RXM_IS_QOS_DATA_FRAME(u2FrameCtrl)) {
				if (RXM_IS_QOS_DATA_FRAME(
					prFragInfo->pr1stFrag->u2FrameCtrl)) {
					if (u2SeqNo == prFragInfo->u2SeqNo
#if CFG_SUPPORT_FRAG_AGG_VALIDATION
					    && ucSecMode
						== prFragInfo->ucSecMode
#else
					    && ucFragNo
						== prFragInfo->ucNextFragNo
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */
					   )
						break;
				}
			} else {
				if (!RXM_IS_QOS_DATA_FRAME(
					prFragInfo->pr1stFrag->u2FrameCtrl)) {
					if (u2SeqNo == prFragInfo->u2SeqNo
#if CFG_SUPPORT_FRAG_AGG_VALIDATION
					    && ucSecMode
						== prFragInfo->ucSecMode
#else
					    && ucFragNo
						== prFragInfo->ucNextFragNo
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */
					   )
						break;
				}
			}
		}
	}

	if (i >= MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS) {

		/* Can't find a proper struct FRAG_INFO.
		 * I. 1st Fragment MPDU, all of the FragInfo are exhausted
		 * II. 2nd ~ (n-1)th Fragment MPDU, can't find the right
		 * FragInfo for defragment.
		 * Because we won't process fragment frame outside this
		 * function, so we should free it right away.
		 */
		nicRxReturnRFB(prAdapter, prSWRfb);

		return (struct SW_RFB *) NULL;
	}

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
	if (prFragInfo->pr1stFrag != (struct SW_RFB *) NULL) {
		/* check if the FragNo is cont. */
		if (ucFragNo != prFragInfo->ucNextFragNo
			|| ((ucSecMode != CIPHER_SUITE_NONE)
				&& (u8PN != prFragInfo->u8NextPN))
			) {
			DBGLOG(RX, DEBUG, "non-cont FragNo or PN, drop it.");

			DBGLOG(RX, DEBUG,
				"SN:%04x NxFragN:%02x FragN:%02x\n",
				prFragInfo->u2SeqNo,
				prFragInfo->ucNextFragNo,
				ucFragNo);

			if (ucSecMode != CIPHER_SUITE_NONE)
				DBGLOG(RX, DEBUG,
					"SN:%04x NxPN:%llx PN:%llx\n",
					prFragInfo->u2SeqNo,
					prFragInfo->u8NextPN,
					u8PN);

			/* discard fragments if FragNo is non-cont. */
			nicRxReturnRFB(prAdapter, prFragInfo->pr1stFrag);
			prFragInfo->pr1stFrag = (struct SW_RFB *) NULL;

			nicRxReturnRFB(prAdapter, prSWRfb);
			return (struct SW_RFB *) NULL;
		}
	}
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

	ASSERT(prFragInfo);

	/* retrieve Rx payload */
	prSWRfb->pucPayload = (uint8_t *) ((
		(uintptr_t) prSWRfb->pvHeader) +
		prSWRfb->u2HeaderLen);
	prSWRfb->u2PayloadLength =
		(uint16_t) (prSWRfb->u2RxByteCount - ((
		uintptr_t) prSWRfb->pucPayload -
		(uintptr_t) prRxStatus));

	if (fgFirst) {
		DBGLOG(RX, LOUD, "rxDefragMPDU first\n");

		SET_EXPIRATION_TIME(prFragInfo->rReceiveLifetimeLimit,
			TU_TO_SYSTIME(
			DOT11_RECEIVE_LIFETIME_TU_DEFAULT));

		prFragInfo->pr1stFrag = prSWRfb;

		prFragInfo->pucNextFragStart =
			(uint8_t *) prSWRfb->pucRecvBuff +
			prSWRfb->u2RxByteCount;

		prFragInfo->u2SeqNo = u2SeqNo;
		prFragInfo->ucNextFragNo = ucFragNo + 1; /* should be 1 */

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
		prFragInfo->ucSecMode = ucSecMode;
		if (prFragInfo->ucSecMode != CIPHER_SUITE_NONE)
			prFragInfo->u8NextPN = u8PN + 1;
		else
			prFragInfo->u8NextPN = 0;
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

		DBGLOG(RX, LOUD,
			"First: SeqCtrl:%04x, SN:%04x, NxFragN = %02x\n",
			u2SeqCtrl, prFragInfo->u2SeqNo,
			prFragInfo->ucNextFragNo);

		/* prSWRfb->fgFragmented = TRUE; */
		/* whsu: todo for checksum */
	} else {
		prFragInfo->pr1stFrag->u2RxByteCount +=
			prSWRfb->u2PayloadLength;

		if (prFragInfo->pr1stFrag->u2RxByteCount >
		    CFG_RX_MAX_PKT_SIZE) {

			prFragInfo->pr1stFrag->eDst = RX_PKT_DESTINATION_NULL;
			QUEUE_INSERT_TAIL(prReturnedQue, prFragInfo->pr1stFrag);

			prFragInfo->pr1stFrag = (struct SW_RFB *) NULL;

			nicRxReturnRFB(prAdapter, prSWRfb);
			DBGLOG(RX, LOUD,
				"Defrag: dropped due length > CFG_RX_MAX_PKT_SIZE\n");
		} else {
			kalMemCopy(prFragInfo->pucNextFragStart,
				prSWRfb->pucPayload,
				prSWRfb->u2PayloadLength);
			/* [6630] update rx byte count and packet length */
			prFragInfo->pr1stFrag->u2PacketLen +=
				prSWRfb->u2PayloadLength;
			prFragInfo->pr1stFrag->u2PayloadLength +=
				prSWRfb->u2PayloadLength;

			if (fgLast) {	/* The last one, free the buffer */
				DBGLOG(RX, LOUD, "Defrag: finished\n");

				prOutputSwRfb = prFragInfo->pr1stFrag;

				prFragInfo->pr1stFrag = (struct SW_RFB *) NULL;
			} else {
				DBGLOG(RX, LOUD, "Defrag: mid fraged\n");

				prFragInfo->pucNextFragStart +=
					prSWRfb->u2PayloadLength;

				prFragInfo->ucNextFragNo++;

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
				if (prFragInfo->ucSecMode
					!= CIPHER_SUITE_NONE)
					prFragInfo->u8NextPN++;
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

			}

			nicRxReturnRFB(prAdapter, prSWRfb);
		}
	}

	/* DBGLOG_MEM8(RXM, INFO, */
	/* prFragInfo->pr1stFrag->pucPayload, */
	/* prFragInfo->pr1stFrag->u2PayloadLength); */

	return prOutputSwRfb;
}				/* end of rxmDefragMPDU() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Do duplicate detection
 *
 * @param prSwRfb Pointer to the RX packet
 *
 * @return TRUE: a duplicate, FALSE: not a duplicate
 */
/*----------------------------------------------------------------------------*/
u_int8_t nicRxIsDuplicateFrame(struct SW_RFB
			       *prSwRfb)
{

	/* Non-QoS Unicast Data or Unicast MMPDU: SC Cache #4;
	 *   QoS Unicast Data: SC Cache #0~3;
	 *   Broadcast/Multicast: RetryBit == 0
	 */
	uint32_t u4SeqCtrlCacheIdx;
	uint16_t u2SequenceControl, u2FrameCtrl;
	u_int8_t fgIsDuplicate = FALSE, fgIsAmsduSubframe = FALSE;
	struct WLAN_MAC_HEADER *prWlanHeader = NULL;
	void *prRxStatus = NULL;
	struct HW_MAC_RX_STS_GROUP_4 *prRxStatusGroup4 = NULL;

	ASSERT(prSwRfb);

	/* Situations in which the STC_REC is missing include:
	 *   (1) Probe Request (2) (Re)Association Request
	 *   (3) IBSS data frames (4) Probe Response
	 */
	if (!prSwRfb->prStaRec)
		return FALSE;

	prRxStatus = prSwRfb->prRxStatus;
	ASSERT(prRxStatus);

	fgIsAmsduSubframe = prSwRfb->ucPayloadFormat;
	if (prSwRfb->fgHdrTran == FALSE) {
		prWlanHeader = (struct WLAN_MAC_HEADER *) prSwRfb->pvHeader;
		u2SequenceControl = prSwRfb->u2SequenceControl;
		u2FrameCtrl = prWlanHeader->u2FrameCtrl;
	} else {
		prRxStatusGroup4 = prSwRfb->prRxStatusGroup4;
		u2SequenceControl = HAL_RX_STATUS_GET_SEQFrag_NUM(
					    prRxStatusGroup4);
		u2FrameCtrl = HAL_RX_STATUS_GET_FRAME_CTL_FIELD(
				      prRxStatusGroup4);
	}
	prSwRfb->u2SequenceControl = u2SequenceControl;

	/* Case 1: Unicast QoS data */
	if (RXM_IS_QOS_DATA_FRAME(
		    u2FrameCtrl)) {
		/* WLAN header shall exist when doing duplicate detection */
		if (prSwRfb->ucTid < CFG_RX_MAX_BA_TID_NUM &&
			prSwRfb->prStaRec->
			aprRxReorderParamRefTbl[prSwRfb->ucTid]) {

			/* QoS data with an RX BA agreement
			 *  Case 1: The packet is not an AMPDU subframe,
			 *          so the RetryBit may be set to 1 (TBC).
			 *  Case 2: The RX BA agreement was just established.
			 *          Some enqueued packets may not be sent with
			 *          aggregation.
			 */

			DBGLOG(RX, LOUD, "RX: SC=0x%X (BA Entry present)\n",
			       u2SequenceControl);

			/* Update the SN cache in order to ensure the
			 * correctness of duplicate removal in case the
			 * BA agreement is deleted
			 */
			prSwRfb->prStaRec->au2CachedSeqCtrl[prSwRfb->ucTid] =
				u2SequenceControl;

			/* debug */
#if 0
			DBGLOG(RXM, LOUD,
			       "RXM: SC= 0x%X (Cache[%d] updated) with BA\n",
			       u2SequenceControl, prSwRfb->ucTID);

			if (g_prMqm->arRxBaTable[
				prSwRfb->prStaRec->aucRxBaTable[prSwRfb->ucTID]]
				.ucStatus == BA_ENTRY_STATUS_DELETING) {
				DBGLOG(RXM, LOUD,
					"RXM: SC= 0x%X (Cache[%d] updated) with DELETING BA\n",
				  u2SequenceControl, prSwRfb->ucTID);
			}
#endif

			/* HW scoreboard shall take care Case 1.
			 * Let the layer layer handle Case 2.
			 */
			return FALSE;	/* Not a duplicate */
		}

		if (prSwRfb->prStaRec->ucDesiredPhyTypeSet &
		    (PHY_TYPE_BIT_HT | PHY_TYPE_BIT_VHT)) {
			u4SeqCtrlCacheIdx = prSwRfb->ucTid;
#if (CFG_SUPPORT_802_11AX == 1)
			} else if (prSwRfb->prStaRec->ucDesiredPhyTypeSet &
				   PHY_TYPE_BIT_HE) {
				u4SeqCtrlCacheIdx = prSwRfb->ucTid;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
			} else if (prSwRfb->prStaRec->ucDesiredPhyTypeSet &
				   PHY_TYPE_BIT_EHT) {
				u4SeqCtrlCacheIdx = prSwRfb->ucTid;
#endif
		} else {
			if (prSwRfb->ucTid < 8) {	/* UP = 0~7 */
				u4SeqCtrlCacheIdx = aucTid2ACI[prSwRfb->ucTid];
			} else {
				DBGLOG(RX, WARN,
				       "RXM: (Warning) Unknown QoS Data with TID=%d\n",
				       prSwRfb->ucTid);
				/* Ignore duplicate frame check */
				return FALSE;
			}
		}
	}
	/* Case 2: Unicast non-QoS data or MMPDUs */
	else
		u4SeqCtrlCacheIdx = TID_NUM;

	/* If this is a retransmission */
	if (u2FrameCtrl & MASK_FC_RETRY) {
		if (u2SequenceControl !=
		    prSwRfb->prStaRec->au2CachedSeqCtrl[u4SeqCtrlCacheIdx]) {
			prSwRfb->prStaRec->au2CachedSeqCtrl[u4SeqCtrlCacheIdx] =
				u2SequenceControl;
			if (fgIsAmsduSubframe ==
					RX_PAYLOAD_FORMAT_FIRST_SUB_AMSDU)
				prSwRfb->prStaRec->
					afgIsIgnoreAmsduDuplicate[
					u4SeqCtrlCacheIdx] = TRUE;
			DBGLOG(RX, LOUD, "RXM: SC= 0x%x (Cache[%u] updated)\n",
			       u2SequenceControl, u4SeqCtrlCacheIdx);
		} else {
			/* A duplicate. */
			if (prSwRfb->prStaRec->
				afgIsIgnoreAmsduDuplicate[u4SeqCtrlCacheIdx]) {
				if (fgIsAmsduSubframe ==
					RX_PAYLOAD_FORMAT_LAST_SUB_AMSDU)
					prSwRfb->prStaRec->
					afgIsIgnoreAmsduDuplicate[
					u4SeqCtrlCacheIdx] = FALSE;
			} else {
				fgIsDuplicate = TRUE;
				DBGLOG(RX, LOUD,
					"RXM: SC= 0x%x (Cache[%u] duplicate)\n",
				  u2SequenceControl, u4SeqCtrlCacheIdx);
			}
		}
	}

	/* Not a retransmission */
	else {

		prSwRfb->prStaRec->au2CachedSeqCtrl[u4SeqCtrlCacheIdx] =
			u2SequenceControl;
		prSwRfb->prStaRec->afgIsIgnoreAmsduDuplicate[u4SeqCtrlCacheIdx]
			= FALSE;

		DBGLOG(RX, LOUD, "RXM: SC= 0x%x (Cache[%u] updated)\n",
		       u2SequenceControl, u4SeqCtrlCacheIdx);
	}

	return fgIsDuplicate;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process packet doesn't need to do buffer reordering
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessPktWithoutReorder(struct ADAPTER
				   *prAdapter, struct SW_RFB *prSwRfb)
{
	struct RX_CTRL *prRxCtrl;
	struct TX_CTRL *prTxCtrl;
	u_int8_t fgIsRetained = FALSE;
	uint32_t u4CurrentRxBufferCount;
	/* P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL; */

	/* DBGLOG(RX, TRACE, ("\n")); */

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);

	prTxCtrl = &prAdapter->rTxCtrl;
	ASSERT(prTxCtrl);

	u4CurrentRxBufferCount = RX_GET_FREE_RFB_CNT(prRxCtrl);
	/* QM USED = $A, AVAILABLE COUNT = $B, INDICATED TO OS = $C
	 * TOTAL = $A + $B + $C
	 *
	 * Case #1 (Retain)
	 * -------------------------------------------------------
	 * $A + $B < THRESHOLD := $A + $B + $C < THRESHOLD + $C
	 * := $TOTAL - THRESHOLD < $C
	 * => $C used too much, retain
	 *
	 * Case #2 (Non-Retain)
	 * -------------------------------------------------------
	 * $A + $B > THRESHOLD := $A + $B + $C > THRESHOLD + $C
	 * := $TOTAL - THRESHOLD > $C
	 * => still available for $C to use
	 *
	 */

#if (CONFIG_SUPPORT_OS_IND_RETAINED == 1)
	fgIsRetained = (((u4CurrentRxBufferCount +
			  qmGetRxReorderQueuedBufferCount(prAdapter) +
			  prTxCtrl->i4PendingFwdFrameCount) <
			 CFG_RX_RETAINED_PKT_THRESHOLD) ? TRUE : FALSE);
#else
	fgIsRetained = FALSE;
#endif

	/* DBGLOG(RX, DEBUG, ("fgIsRetained = %d\n", fgIsRetained)); */
#if CFG_ENABLE_PER_STA_STATISTICS && CFG_ENABLE_PKT_LIFETIME_PROFILE
#if CFG_SUPPORT_WFD
	if (prSwRfb->prStaRec
	    && (prAdapter->rWifiVar.rWfdConfigureSettings.ucWfdEnable >
		0))
		prSwRfb->prStaRec->u4TotalRxPktsNumber++;
#endif
#endif


#if CFG_AP_80211KVR_INTERFACE
	if (prSwRfb->prStaRec) {
		prSwRfb->prStaRec->u8TotalRxBytes += prSwRfb->u2PacketLen;
		prSwRfb->prStaRec->u8TotalRxPkts++;
	}
#endif

	if (kalProcessRxPacket(prAdapter->prGlueInfo,
			       prSwRfb->pvPacket,
			       prSwRfb->pvHeader,
			       (uint32_t) prSwRfb->u2PacketLen,
			       prSwRfb->aeCSUM) != WLAN_STATUS_SUCCESS) {
		DBGLOG(RX, ERROR,
		       "kalProcessRxPacket return value != WLAN_STATUS_SUCCESS\n");
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_DROP_TOTAL_COUNT);
		nicRxReturnRFB(prAdapter, prSwRfb);
		return;
	}

#if CFG_SUPPORT_WED_PROXY
	/* Add info to SKB headroom after SKB reset */
	wedHwRxInfoWrapper(prSwRfb);
#endif

#if CFG_SUPPORT_MULTITHREAD
	if (HAL_IS_RX_DIRECT(prAdapter)) {
		kalRxIndicateOnePkt(prAdapter->prGlueInfo, prSwRfb->pvPacket);
		if (fgIsRetained)
			RX_ADD_CNT(prRxCtrl, RX_DATA_RETAINED_COUNT, 1);
	} else {
		KAL_SPIN_LOCK_DECLARATION();

		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_TO_OS_QUE);
		QUEUE_INSERT_TAIL(&(prAdapter->rRxQueue),
				  GLUE_GET_PKT_QUEUE_ENTRY(prSwRfb->pvPacket));
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_TO_OS_QUE);

		prRxCtrl->ucNumIndPacket++;
		kalSetTxEvent2Rx(prAdapter->prGlueInfo);
	}
#else
#if defined(_HIF_USB)
	if (HAL_IS_RX_DIRECT(prAdapter)) {
		kalRxIndicateOnePkt(prAdapter->prGlueInfo, prSwRfb->pvPacket);
		RX_ADD_CNT(prRxCtrl, RX_DATA_INDICATION_COUNT, 1);
		if (fgIsRetained)
			RX_ADD_CNT(prRxCtrl, RX_DATA_RETAINED_COUNT, 1);
	}
#endif
	prRxCtrl->apvIndPacket[prRxCtrl->ucNumIndPacket] =
		prSwRfb->pvPacket;
	prRxCtrl->ucNumIndPacket++;
#endif

#if (CONFIG_SUPPORT_OS_IND_RETAINED == 1)
	if (fgIsRetained) {
		prRxCtrl->apvRetainedPacket[prRxCtrl->ucNumRetainedPacket] =
			prSwRfb->pvPacket;
		prRxCtrl->ucNumRetainedPacket++;
	} else
#endif
		prSwRfb->pvPacket = NULL;

#if (CFG_SUPPORT_RETURN_TASK == 1)
	/* Move SKB allocation to another context to reduce RX latency,
	 * only if SKB is NULL.
	 */
	if (!prSwRfb->pvPacket) {
		nicRxReturnRFB(prAdapter, prSwRfb);
		kal_tasklet_hi_schedule(&prAdapter->prGlueInfo->rRxRfbRetTask);
		return;
	}
#elif CFG_SUPPORT_RETURN_WORK
	if (!prSwRfb->pvPacket) {
		nicRxReturnRFB(prAdapter, prSwRfb);
#if !CFG_SUPPORT_SKB_ALLOC_WORK
		/* SkbAllocWork call it later in kalSkbAllocWorkDone */
		kalRxRfbReturnWorkSchedule(prAdapter->prGlueInfo);
#endif /* !CFG_SUPPORT_SKB_ALLOC_WORK */
		return;
	}
#endif

	/* Return RFB */
	if (nicRxSetupRFB(prAdapter, prSwRfb)) {
		DBGLOG(RX, WARN,
		       "Cannot allocate packet buffer for SwRfb!\n");
		if (!timerPendingTimer(
			    &prAdapter->rPacketDelaySetupTimer)) {
			DBGLOG(RX, WARN,
				"Start ReturnIndicatedRfb Timer (%ums)\n",
			  RX_RETURN_INDICATED_RFB_TIMEOUT_MSEC);
			cnmTimerStartTimer(prAdapter,
				&prAdapter->rPacketDelaySetupTimer,
					RX_RETURN_INDICATED_RFB_TIMEOUT_MSEC);
		}
	}
	nicRxReturnRFB(prAdapter, prSwRfb);
}

u_int8_t nicRxCheckForwardPktResource(
	struct ADAPTER *prAdapter, uint32_t ucTid)
{
	struct TX_CTRL *prTxCtrl;
	uint8_t i, uTxQidx;

	prTxCtrl = &prAdapter->rTxCtrl;
	uTxQidx = aucACI2TxQIdx[aucTid2ACI[ucTid]];

	/* If the resource used more than half, we could control WMM resource
	 * by limit every AC queue.
	 */
	for (i = uTxQidx+1; i < WMM_AC_INDEX_NUM; i++) {
		if (GLUE_GET_REF_CNT(prTxCtrl
			->i4PendingFwdFrameWMMCount[uTxQidx]) >=
			GLUE_GET_REF_CNT(prTxCtrl
			->i4PendingFwdFrameWMMCount[i]) &&
			GLUE_GET_REF_CNT(prTxCtrl
			->i4PendingFwdFrameWMMCount[i]) > 0 &&
			GLUE_GET_REF_CNT(prTxCtrl
			->i4PendingFwdFrameCount) > prAdapter
			->rQM.u4MaxForwardBufferCount)
			return FALSE;
	}
	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process forwarding data packet
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessForwardPkt(struct ADAPTER *prAdapter,
			    struct SW_RFB *prSwRfb)
{
	struct MSDU_INFO *prMsduInfo, *prRetMsduInfoList;
	struct TX_CTRL *prTxCtrl;
	struct RX_CTRL *prRxCtrl;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prTxCtrl = &prAdapter->rTxCtrl;
	prRxCtrl = &prAdapter->rRxCtrl;

	if (prSwRfb->ucTid >= TX_DESC_TID_NUM) {
		DBGLOG_LIMITED(RX, WARN,
		       "Wrong forward packet: tid:%d\n", prSwRfb->ucTid);
		prSwRfb->ucTid = 0;
	}

	if (!nicRxCheckForwardPktResource(prAdapter, prSwRfb->ucTid)) {
		nicRxReturnRFB(prAdapter, prSwRfb);
		return;
	}

	RX_INC_CNT(&prAdapter->rRxCtrl, RX_DATA_FORWARD_COUNT);

	DBGLOG_LIMITED(RX, TRACE, "to forward packet: %d,%d,%d,%d,%d\n",
		GLUE_GET_REF_CNT(prTxCtrl->i4PendingFwdFrameWMMCount[0]),
		GLUE_GET_REF_CNT(prTxCtrl->i4PendingFwdFrameWMMCount[1]),
		GLUE_GET_REF_CNT(prTxCtrl->i4PendingFwdFrameWMMCount[2]),
		GLUE_GET_REF_CNT(prTxCtrl->i4PendingFwdFrameWMMCount[3]),
		GLUE_GET_REF_CNT(prTxCtrl->i4PendingFwdFrameCount));

	prMsduInfo = cnmPktAlloc(prAdapter, 0);

	if (prMsduInfo &&
	    kalProcessRxPacket(prAdapter->prGlueInfo,
			prSwRfb->pvPacket,
			prSwRfb->pvHeader,
			(uint32_t) prSwRfb->u2PacketLen,
			prSwRfb->aeCSUM) == WLAN_STATUS_SUCCESS) {
		uint8_t ucTmpTid = 0;
		/* parsing forward frame */
		wlanProcessTxFrame(prAdapter, (void *) (prSwRfb->pvPacket));
		/* pack into MSDU_INFO_T */
		nicTxFillMsduInfo(prAdapter, prMsduInfo,
				  (void *) (prSwRfb->pvPacket));

		prMsduInfo->eSrc = TX_PACKET_FORWARDING;
		prMsduInfo->ucBssIndex = secGetBssIdxByWlanIdx(prAdapter,
					 prSwRfb->ucWlanIdx);
		prMsduInfo->ucUserPriority = prSwRfb->ucTid;

		/* release RX buffer (to rIndicatedRfbList) */
		prSwRfb->pvPacket = NULL;
		ucTmpTid = prSwRfb->ucTid;
		nicRxReturnRFB(prAdapter, prSwRfb);

		/* Handle if prMsduInfo out of bss index range*/
		if (prMsduInfo->ucBssIndex > MAX_BSSID_NUM) {
			DBGLOG(QM, DEBUG,
			    "Invalid bssidx:%u\n", prMsduInfo->ucBssIndex);
			if (prMsduInfo->pfTxDoneHandler != NULL)
				prMsduInfo->pfTxDoneHandler(prAdapter,
						prMsduInfo,
						TX_RESULT_DROPPED_IN_DRIVER);
			nicTxReturnMsduInfo(prAdapter, prMsduInfo);
			return;
		}

		/* increase forward frame counter */
		GLUE_INC_REF_CNT(prTxCtrl->i4PendingFwdFrameCount);

		/* add resource control for WMM forward packet */
		GLUE_INC_REF_CNT(prTxCtrl
			->i4PendingFwdFrameWMMCount[
			aucACI2TxQIdx[aucTid2ACI[ucTmpTid]]]);

		/* send into TX queue */
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);
		prRetMsduInfoList = qmEnqueueTxPackets(prAdapter,
						       prMsduInfo);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);

		if (prRetMsduInfoList !=
		    NULL) {	/* TX queue refuses queuing the packet */
			nicTxFreeMsduInfoPacket(prAdapter, prRetMsduInfoList);
			nicTxReturnMsduInfo(prAdapter, prRetMsduInfoList);
		}
		/* indicate service thread for sending */
		if (prTxCtrl->i4PendingFwdFrameCount > 0)
			kalSetEvent(prAdapter->prGlueInfo);
	} else {		/* no TX resource */
		DBGLOG(QM, DEBUG, "No Tx MSDU_INFO for forwarding frames\n");
		nicRxReturnRFB(prAdapter, prSwRfb);
		if (prMsduInfo)
			nicTxReturnMsduInfo(prAdapter, prMsduInfo);
	}
}

static struct SW_RFB *nicRxDuplicateBmcPkt(struct ADAPTER *prAdapter,
					   struct SW_RFB *prSwRfb)
{
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;
	struct SW_RFB *prSwRfbDuplicated;

	KAL_SPIN_LOCK_DECLARATION();

	if (RX_GET_FREE_RFB_CNT(prRxCtrl) < /* Reserved for others */
	    CFG_RX_MAX_PKT_NUM - (CFG_NUM_OF_QM_RX_PKT_NUM - 16)) {
		DBGLOG(RX, WARN,
		      "Stop to forward BMC packet due to less free Sw Rfb %u\n",
		      RX_GET_FREE_RFB_CNT(prRxCtrl));
		return NULL;
	}

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
	QUEUE_REMOVE_HEAD(&prRxCtrl->rFreeSwRfbList,
			  prSwRfbDuplicated, struct SW_RFB *);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

	if (!prSwRfbDuplicated)
		return NULL;

	if (kalDuplicateSwRfbSanity(prSwRfbDuplicated) !=
	    WLAN_STATUS_SUCCESS) {
		nicRxReturnRFB(prAdapter, prSwRfbDuplicated);
		RX_INC_CNT(prRxCtrl, RX_POINTER_ERR_DROP_COUNT);
		RX_INC_CNT(prRxCtrl, RX_DROP_TOTAL_COUNT);
		return NULL;
	}

	kalMemCopy(prSwRfbDuplicated->pucRecvBuff,
		   prSwRfb->pucRecvBuff,
		   ALIGN_4(prSwRfb->u2RxByteCount +
			   HIF_RX_HW_APPENDED_LEN));

	prSwRfbDuplicated->ucPacketType = RX_PKT_TYPE_RX_DATA;
	prSwRfbDuplicated->ucStaRecIdx = prSwRfb->ucStaRecIdx;

	nicRxFillRFB(prAdapter, prSwRfbDuplicated);
	GLUE_COPY_PRIV_DATA(prSwRfbDuplicated->pvPacket,
			    prSwRfb->pvPacket);

	prSwRfbDuplicated->eDst = RX_PKT_DESTINATION_FORWARD;

	return prSwRfbDuplicated;
}

static void nicRxDuplicateBmcPkts(struct ADAPTER *prAdapter,
				  struct SW_RFB *prSwRfb,
				  struct SW_RFB *aprSwRfbs[], uint8_t *pucNum)
{
	struct SW_RFB *prSwRfbDuplicated;
#if (CFG_SUPPORT_802_11BE_MLO == 1) && (CFG_SUPPORT_MLO_GRP_FRAME_XMIT == 1)
	struct MLD_BSS_INFO *prMldBss;
	struct LINK *prBssList;
	struct BSS_INFO *prBssInfo;
	uint16_t u2Sn;
#endif /* CFG_SUPPORT_802_11BE_MLO */

	*pucNum = 0;

#if (CFG_SUPPORT_802_11BE_MLO == 1) && (CFG_SUPPORT_MLO_GRP_FRAME_XMIT == 1)
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		GLUE_GET_PKT_BSS_IDX(prSwRfb->pvPacket));
	prMldBss = mldBssGetByBss(prAdapter, prBssInfo);
	if (!prBssInfo || !prMldBss || !IS_BSS_AP(prAdapter, prBssInfo) ||
	    !RLM_NET_IS_11BE(prBssInfo))
		goto legacy;

	prBssList = &prMldBss->rBssList;
	u2Sn = mldBssGetGrpFrameSn(prAdapter, prMldBss);
	LINK_FOR_EACH_ENTRY(prBssInfo, prBssList, rLinkEntryMld,
			    struct BSS_INFO) {
		prSwRfbDuplicated = nicRxDuplicateBmcPkt(prAdapter, prSwRfb);
		if (!prSwRfbDuplicated)
			break;

		GLUE_SET_PKT_BSS_IDX(prSwRfbDuplicated->pvPacket,
				     prBssInfo->ucBssIndex);
		GLUE_SET_PKT_SN(prSwRfbDuplicated->pvPacket, u2Sn);
		prSwRfbDuplicated->ucWlanIdx = prBssInfo->ucBMCWlanIndex;

		aprSwRfbs[(*pucNum)++] = prSwRfbDuplicated;
	}

	return;

legacy:
#endif /* CFG_SUPPORT_802_11BE_MLO */

	prSwRfbDuplicated = nicRxDuplicateBmcPkt(prAdapter, prSwRfb);
	if (!prSwRfbDuplicated)
		return;

	aprSwRfbs[(*pucNum)++] = prSwRfbDuplicated;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process broadcast data packet for both host and forwarding
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessGOBroadcastPkt(struct ADAPTER
				*prAdapter, struct SW_RFB *prSwRfb)
{
	struct SW_RFB *aprSwRfbs[MLD_LINK_MAX];
	uint8_t ucIdx, ucNum = 0;

	_Static_assert(CFG_NUM_OF_QM_RX_PKT_NUM >= 16,
			"CFG_NUM_OF_QM_RX_PKT_NUM too small");

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	kalMemZero(aprSwRfbs, sizeof(aprSwRfbs));
	nicRxDuplicateBmcPkts(prAdapter, prSwRfb, aprSwRfbs, &ucNum);
	for (ucIdx = 0; ucIdx < ucNum; ucIdx++)
		nicRxProcessForwardPkt(prAdapter, aprSwRfbs[ucIdx]);

	/* 3. Indicate to host */
	prSwRfb->eDst = RX_PKT_DESTINATION_HOST;
	nicRxProcessPktWithoutReorder(prAdapter, prSwRfb);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process & Parsing RXV for traffic indicator
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
#if CFG_SUPPORT_PERF_IND
void nicRxPerfIndProcessRXV(struct ADAPTER *prAdapter,
			       struct SW_RFB *prSwRfb,
			       uint8_t ucBssIndex)
{
	struct mt66xx_chip_info *prChipInfo;

	prChipInfo = prAdapter->chip_info;
	if (!prChipInfo || !prChipInfo->asicRxPerfIndProcessRXV)
		return;

	prChipInfo->asicRxPerfIndProcessRXV(prAdapter, prSwRfb, ucBssIndex);
	/* else { */
		/* print too much, remove for system perfomance */
		/* DBGLOG(RX, ERROR, "%s: no asicRxPerfIndProcessRXV ??\n", */
		/* __func__); */
	/* } */
}
#endif

static void nicRxSendDeauthPacket(struct ADAPTER *prAdapter,
		uint16_t u2FrameCtrl,
		uint8_t *pucSrcAddr,
		uint8_t *pucDestAddr,
		uint8_t *pucBssid)
{
	struct SW_RFB rSwRfb;
	struct WLAN_MAC_HEADER rWlanHeader;
	uint32_t u4Status;

	if (!prAdapter || !pucSrcAddr || !pucDestAddr || !pucBssid)
		return;

	kalMemZero(&rSwRfb, sizeof(rSwRfb));
	kalMemZero(&rWlanHeader, sizeof(rWlanHeader));

	rWlanHeader.u2FrameCtrl = u2FrameCtrl;
	COPY_MAC_ADDR(rWlanHeader.aucAddr1, pucSrcAddr);
	COPY_MAC_ADDR(rWlanHeader.aucAddr2, pucDestAddr);
	COPY_MAC_ADDR(rWlanHeader.aucAddr3, pucBssid);
	rSwRfb.pvHeader = &rWlanHeader;

	u4Status = authSendDeauthFrame(prAdapter,
		NULL,
		NULL,
		&rSwRfb,
		REASON_CODE_CLASS_3_ERR,
		NULL);
	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(NIC, WARN, "u4Status: %d\n", u4Status);
}

static void nicRxProcessDropPacket(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb)
{
	struct WLAN_MAC_HEADER *prWlanHeader = NULL;
	uint8_t ucBssIndex = 0;
	uint16_t u2FrameCtrl;

	if (!prAdapter || !prSwRfb)
		return;

	prWlanHeader = (struct WLAN_MAC_HEADER *) prSwRfb->pvHeader;

	if (!prWlanHeader)
		return;

	u2FrameCtrl = prWlanHeader->u2FrameCtrl;
	DBGLOG(RX, TEMP,
		"TA: " MACSTR " RA: " MACSTR " bssid: " MACSTR " fc: 0x%x\n",
		MAC2STR(prWlanHeader->aucAddr2),
		MAC2STR(prWlanHeader->aucAddr1),
		MAC2STR(prWlanHeader->aucAddr3),
		u2FrameCtrl);

	if ((u2FrameCtrl & (MASK_FC_FROM_DS | MASK_FC_TO_DS)) == 0)
		return;

	for (ucBssIndex = 0; ucBssIndex < prAdapter->ucSwBssIdNum;
			ucBssIndex++) {
		struct BSS_INFO *prBssInfo;
		u_int8_t fgSendDeauth = FALSE;

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
		if (!prBssInfo)
			continue;
		if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo))
			continue;
		switch (prBssInfo->eNetworkType) {
		case NETWORK_TYPE_P2P:
			if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT &&
				EQUAL_MAC_ADDR(prWlanHeader->aucAddr3,
					prBssInfo->aucOwnMacAddr))
				fgSendDeauth = TRUE;
			break;
		default:
			break;
		}

		if (fgSendDeauth)
			nicRxSendDeauthPacket(prAdapter,
				u2FrameCtrl,
				prWlanHeader->aucAddr1,
				prWlanHeader->aucAddr2,
				prWlanHeader->aucAddr3);
	}
}
/* fos_change begin */
#if CFG_SUPPORT_STAT_STATISTICS
void nicRxGetNoiseLevelAndLastRate(struct ADAPTER *prAdapter,
			       struct SW_RFB *prSwRfb)
{
	struct STA_RECORD *prStaRec;
	uint8_t noise_level = 0;
	uint8_t ucRxRate;
	uint8_t ucRxMode;
	uint8_t ucMcs;
	uint8_t ucFrMode;
	uint8_t ucShortGI;

	if (prAdapter == NULL || prSwRfb == NULL)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (prStaRec == NULL)
		return;

	noise_level = ((prSwRfb->prRxStatusGroup3->u4RxVector[5] &
		RX_VT_NF0_MASK) >> 1);

	if (noise_level == 0) {
		DBGLOG(RX, TRACE, "Invalid noise level\n");
	} else if (prStaRec->ucNoise_avg) {
		prStaRec->ucNoise_avg = (((prStaRec->ucNoise_avg << 3) -
			  prStaRec->ucNoise_avg) >> 3) + (noise_level >> 3);
	} else {
		prStaRec->ucNoise_avg = noise_level;
	}

	DBGLOG(RX, TRACE, "Noise_level avg:%d latest:%d\n",
		prStaRec->ucNoise_avg, noise_level);

	wlanGetRxRateByBssid(prAdapter->prGlueInfo,
				GLUE_GET_PKT_BSS_IDX(prRetSwRfb->pvPacket),
				&prStaRec->u4LastPhyRate, NULL, NULL);
}
#endif /* fos_change end */

#if CFG_QUEUE_RX_IF_CONN_NOT_READY
void nicRxEnqueuePendingQueue(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, struct SW_RFB *prSwRfb)
{
	uint8_t ucBssIndex;

	if (!prAdapter || !prStaRec || !prSwRfb)
		return;

	DBGLOG_LIMITED(RX, TRACE,
		"StaRecId[%u] BssId[%u] RxAllowed:%u prSwRfb:%p PktType:0x%02x\n",
		prStaRec->ucIndex,
		prStaRec->ucBssIndex,
		prStaRec->fgIsRxAllowed,
		prSwRfb,
		GLUE_IS_PKT_FLAG_SET(prSwRfb->pvPacket));

	ucBssIndex = prStaRec->ucBssIndex;
	RX_PENDING_INC_BSS_CNT(&prAdapter->rRxCtrl, ucBssIndex);

#if CFG_RFB_TRACK
	RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb, RFB_TRACK_RX_PENDING);
#endif /* CFG_RFB_TRACK */

	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_RX_PENDING);
	QUEUE_INSERT_TAIL(&prAdapter->rRxPendingQueue, prSwRfb);
	KAL_RELEASE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_RX_PENDING);
}

void nicRxDequeuePendingQueue(struct ADAPTER *prAdapter)
{
	struct QUE rSrcQ, rDstQ;
	struct QUE *prSrcQ = &rSrcQ;
	struct QUE *prDstQ = &rDstQ;
	struct SW_RFB *prSwRfb;
	struct STA_RECORD *prStaRec;

	if (!prAdapter)
		return;

	if (QUEUE_IS_EMPTY(&prAdapter->rRxPendingQueue))
		return;

	QUEUE_INITIALIZE(prSrcQ);
	QUEUE_INITIALIZE(prDstQ);

	NIC_RX_DEQUEUE_MOVE_ALL(prAdapter, prSrcQ,
		&prAdapter->rRxPendingQueue,
		SPIN_LOCK_RX_PENDING, RFB_TRACK_RX_PENDING);

	while (QUEUE_IS_NOT_EMPTY(prSrcQ)) {
		QUEUE_REMOVE_HEAD(prSrcQ, prSwRfb, struct SW_RFB *);
		if (prSwRfb == NULL)
			break;

		/*
		 * If AP does not reply assoc resp,
		 * AIS will disconnect and indirectly call this function,
		 * so we need to drop it if prStaRec is not valid to
		 * prevent SwRfb leakage.
		 */
		prStaRec = cnmGetStaRecByIndex(prAdapter,
			prSwRfb->ucStaRecIdx);
		if (!prStaRec) {
			DBGLOG_LIMITED(RX, TRACE,
				"StaRecId[%u] prStaRec NULL\n",
				prSwRfb->ucStaRecIdx);
			nicRxReturnRFB(prAdapter, prSwRfb);
			continue;
		}

		DBGLOG_LIMITED(RX, TRACE,
			"StaRecId[%u] BssId[%u] RxAllowed:%u prSwRfb:%p PktFlag:0x%02x\n",
			prStaRec->ucIndex,
			prStaRec->ucBssIndex,
			prStaRec->fgIsRxAllowed,
			prSwRfb,
			GLUE_IS_PKT_FLAG_SET(prSwRfb->pvPacket));

		/* Rx is not allowed, cannot dequeue the rx pkt */
		if (!prStaRec->fgIsRxAllowed) {
			QUEUE_INSERT_TAIL(prDstQ, prSwRfb);
			continue;
		}

		RX_PENDING_DEC_BSS_CNT(&prAdapter->rRxCtrl,
			prStaRec->ucBssIndex);

		/* Rx is allowed, so indicate to host */
		nicRxProcessPktWithoutReorder(prAdapter, prSwRfb);
	}

	if (QUEUE_IS_EMPTY(prDstQ))
		return;

	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_RX_PENDING);
	QUEUE_CONCATENATE_QUEUES_HEAD(&prAdapter->rRxPendingQueue, prDstQ);
	KAL_RELEASE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_RX_PENDING);
}
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */

uint32_t nicRxProcessPacketToHost(struct ADAPTER *prAdapter,
	struct SW_RFB *prRetSwRfb)
{
	struct RX_CTRL *prRxCtrl;
	struct STA_RECORD *prStaRec;
	uint8_t ucBssIndex;
	struct BSS_INFO *prBssInfo;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	OS_SYSTIME rCurrentTime;

	prRxCtrl = &prAdapter->rRxCtrl;
	prStaRec = cnmGetStaRecByIndex(prAdapter,
			prRetSwRfb->ucStaRecIdx);
	if (!prStaRec)
		goto end;

	/* store it in local variable to prevent timing issue */
	ucBssIndex = prStaRec->ucBssIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		goto end;

	GET_BOOT_SYSTIME(&rCurrentTime);

#if ARP_MONITER_ENABLE
	arpMonProcessRxPacket(prAdapter, prBssInfo, prRetSwRfb, rCurrentTime);
#endif /* ARP_MONITER_ENABLE */

	if (ucBssIndex < MAX_BSSID_NUM)
		prRxCtrl->rLastRxTime[ucBssIndex] = rCurrentTime;

	secCheckRxEapolPacketEncryption(prAdapter, prRetSwRfb, prStaRec);

#if CFG_QUEUE_RX_IF_CONN_NOT_READY
	if (!prStaRec->fgIsRxAllowed) {
		/* Rx is not Allowed to indicate to host */
		u4Status = WLAN_STATUS_PENDING;
		nicRxEnqueuePendingQueue(prAdapter, prStaRec, prRetSwRfb);
	}
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */

end:
	return u4Status;
}

void nicRxIndicatePackets(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfbListHead)
{
	uint32_t ret;
	struct RX_CTRL *prRxCtrl;
	struct SW_RFB *prRetSwRfb, *prNextSwRfb;

	prRxCtrl = &prAdapter->rRxCtrl;
	prRetSwRfb = prSwRfbListHead;

	while (prRetSwRfb) {
		/**
		 * Collect RXV information,
		 * prAdapter->arStaRec[i].u4RxV[*] updated.
		 * wlanGetRxRate() can get new rate values
		 */
		if (prRetSwRfb->eDst != RX_PKT_DESTINATION_NULL)
			nicRxProcessRxv(prAdapter, prRetSwRfb);

		/* save next first */
		prNextSwRfb = QUEUE_GET_NEXT_ENTRY(prRetSwRfb);

		switch (prRetSwRfb->eDst) {
		case RX_PKT_DESTINATION_HOST:
			ret = nicRxProcessPacketToHost(prAdapter,
					prRetSwRfb);
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_SUPPORT_CONNAC2X == 1)
			/* Not handle non-CONNAC2X case */
			if (RXV_AUTODVT_DNABLED(prAdapter) &&
				(prRetSwRfb->ucGroupVLD &
				BIT(RX_GROUP_VLD_3)) &&
				(prRetSwRfb->ucGroupVLD &
				BIT(RX_GROUP_VLD_5))) {
				connac2x_rxv_correct_test(
					prAdapter, prRetSwRfb);
			}
#endif
#endif /* CFG_SUPPORT_WIFI_SYSDVT */
			if (ret == WLAN_STATUS_SUCCESS)
				nicRxProcessPktWithoutReorder(prAdapter,
					prRetSwRfb);
			break;

		case RX_PKT_DESTINATION_FORWARD:
			nicRxProcessForwardPkt(
				prAdapter, prRetSwRfb);
			break;

		case RX_PKT_DESTINATION_HOST_WITH_FORWARD:
			nicRxProcessGOBroadcastPkt(prAdapter,
				prRetSwRfb);
			break;

		case RX_PKT_DESTINATION_NULL:
			nicRxReturnRFB(prAdapter, prRetSwRfb);
			RX_INC_CNT(prRxCtrl,
				RX_DST_NULL_DROP_COUNT);
			RX_INC_CNT(prRxCtrl,
				RX_DROP_TOTAL_COUNT);
			break;

		default:
			break;
		}

		prRetSwRfb = prNextSwRfb;
	}
#if CFG_SUPPORT_RX_GRO
#if (CFG_SUPPORT_RX_NAPI == 0)
	kal_gro_flush_queue(prAdapter->prGlueInfo);
#endif /* CFG_SUPPORT_RX_NAPI == 0 */
#endif /* CFG_SUPPORT_RX_GRO */
}

void nicRxEnqueueRfbMainToNapi(struct ADAPTER *ad, struct QUE *prQue)
{
	KAL_ACQUIRE_SPIN_LOCK_BH(ad, SPIN_LOCK_RX_TO_NAPI);
	QUEUE_CONCATENATE_QUEUES(&ad->rRxMainToNapiQue, prQue);
	KAL_RELEASE_SPIN_LOCK_BH(ad, SPIN_LOCK_RX_TO_NAPI);
}

void nicRxIndicateRfbMainToNapi(struct ADAPTER *ad)
{
	struct QUE rQue;
	struct QUE *prQue = &rQue;

	if (!ad)
		return;

	if (QUEUE_IS_EMPTY(&ad->rRxMainToNapiQue))
		return;

	QUEUE_INITIALIZE(prQue);

	NIC_RX_DEQUEUE_MOVE_ALL(ad, prQue, &ad->rRxMainToNapiQue,
		SPIN_LOCK_RX_TO_NAPI, RFB_TRACK_MAIN_TO_NAPI);

	if (QUEUE_IS_EMPTY(prQue))
		return;

	QUEUE_ENTRY_SET_NEXT(QUEUE_GET_TAIL(prQue), NULL);
	nicRxIndicatePackets(ad, QUEUE_GET_HEAD(prQue));
}

void nicRxParseDropPkt(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	DBGLOG_LIMITED(RX, DEBUG,
		"SwRfb:[0x%p:0x%p] PktLen:[%u] BMC:[%u:%u] SecMode:[%u] WlanId:[%u:%u]\n",
		prSwRfb, prSwRfb->pvPacket,
		prSwRfb->u2PacketLen,
		prSwRfb->fgIsBC, prSwRfb->fgIsMC,
		prSwRfb->ucSecMode,
		prSwRfb->ucWlanIdx, prSwRfb->ucStaRecIdx
	);

	StatsRxPktInfoDisplay(prAdapter, prSwRfb);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process HIF data packet
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessDataPacket(struct ADAPTER *prAdapter,
			    struct SW_RFB *prSwRfb)
{
	struct RX_CTRL *prRxCtrl;
	struct SW_RFB *prRetSwRfb;
	struct HW_MAC_RX_DESC *prRxStatus;

	u_int8_t fgDrop;
	uint8_t ucBssIndex = 0;
	struct mt66xx_chip_info *prChipInfo;
	struct RX_DESC_OPS_T *prRxDescOps;

	/* DBGLOG(INIT, TRACE, ("\n")); */

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	if (prAdapter->prGlueInfo->fgIsEnableMon) {
		radiotapFillRadiotap(prAdapter, prSwRfb);
		return;
	}
#endif

#if CFG_RFB_TRACK
	RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb, RFB_TRACK_DATA);
#endif /* CFG_RFB_TRACK */

	nicRxFillRFB(prAdapter, prSwRfb);

	fgDrop = FALSE;

	prRxCtrl = &prAdapter->rRxCtrl;
	prChipInfo = prAdapter->chip_info;
	prRxDescOps = prChipInfo->prRxDescOps;
	prRxStatus = (struct HW_MAC_RX_DESC *)prSwRfb->prRxStatus;

	/* Check AMPDU_nERR_Bitmap */
	prSwRfb->fgDataFrame = TRUE;
	prSwRfb->fgFragFrame = FALSE;
	prSwRfb->fgReorderBuffer = FALSE;

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
	prSwRfb->fgIsFirstSubAMSDULLCMS = FALSE;
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

#if CFG_WIFI_SW_CIPHER_MISMATCH
	if (prSwRfb->prStaRec &&
	    prSwRfb->prStaRec->fgTransmitKeyExist &&
	    prSwRfb->prStaRec->ucStaState == STA_STATE_3 &&
	    prSwRfb->fgIsBC == FALSE &&
	    prSwRfb->fgIsMC == FALSE &&
	    !prSwRfb->fgIsCipherMS) {
		uint16_t u2FrameCtrl = 0;

		if (prSwRfb->fgHdrTran == FALSE) {
			u2FrameCtrl = ((struct WLAN_MAC_HEADER *)
				prSwRfb->pvHeader)->u2FrameCtrl;
			prSwRfb->fgIsCipherMS =
				!RXM_IS_PROTECTED_FRAME(u2FrameCtrl);
		} else if (prSwRfb->prRxStatusGroup4) {
			u2FrameCtrl = HAL_RX_STATUS_GET_FRAME_CTL_FIELD(
					      prSwRfb->prRxStatusGroup4);
			prSwRfb->fgIsCipherMS =
				!RXM_IS_PROTECTED_FRAME(u2FrameCtrl);
		}
	}
#endif

	if (prRxDescOps->nic_rxd_sanity_check)
		fgDrop = prRxDescOps->nic_rxd_sanity_check(
			prAdapter, prSwRfb);
	else {
		DBGLOG(RX, ERROR,
			"%s:: no nic_rxd_sanity_check??\n", __func__);
		fgDrop = TRUE;
	}

	if (fgDrop && prRxStatus->ucWlanIdx >= WTBL_SIZE &&
			HAL_RX_STATUS_IS_LLC_MIS(prRxStatus))
		nicRxProcessDropPacket(prAdapter, prSwRfb);

#if CFG_TCP_IP_CHKSUM_OFFLOAD || CFG_TCP_IP_CHKSUM_OFFLOAD_NDIS_60
	if (prAdapter->fgIsSupportCsumOffload && fgDrop == FALSE)
		nicRxFillChksumStatus(prAdapter, prSwRfb);
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

	/* if(secCheckClassError(prAdapter, prSwRfb, prStaRec) == TRUE && */
	if (prAdapter->fgTestMode == FALSE && fgDrop == FALSE) {
		ucBssIndex = secGetBssIdxByWlanIdx(prAdapter,
						   prSwRfb->ucWlanIdx);
		GLUE_SET_PKT_BSS_IDX(prSwRfb->pvPacket, ucBssIndex);

		if (IS_BSS_INDEX_AIS(prAdapter, ucBssIndex) &&
			prSwRfb->u2EthTypeLen == ETH_P_1X)
			qmCheckRxEAPOLM3(prAdapter, prSwRfb, ucBssIndex);

#if CFG_FAST_PATH_SUPPORT
		if (
#if CFG_SUPPORT_LOWLATENCY_MODE
			prAdapter->fgEnLowLatencyMode &&
#endif
			prAdapter->rWifiVar.ucSupportProtocol != 0)
			mscsHandleRxPacket(prAdapter, prSwRfb);
#endif

#if ((CFG_SUPPORT_802_11AX == 1) && (CFG_SUPPORT_WIFI_SYSDVT == 1))
		if (fgEfuseCtrlAxOn == 1) {
		if (prAdapter->fgEnShowHETrigger) {
			uint16_t u2TxFrameCtrl;

			u2TxFrameCtrl = (*(uint8_t *) (prSwRfb->pvHeader) &
				 MASK_FRAME_TYPE);
			if (RXM_IS_TRIGGER_FRAME(u2TxFrameCtrl)) {
				DBGLOG(NIC, STATE,
					"\n%s: HE Trigger --------------\n",
					__func__);
				dumpMemory8((uint8_t *)prSwRfb->prRxStatus,
					prSwRfb->u2RxByteCount);
				DBGLOG(NIC, STATE,
					"%s: HE Trigger end --------------\n",
					__func__);
				nicRxReturnRFB(prAdapter, prSwRfb);
				return;
			}
		}
		}
#endif /* CFG_SUPPORT_802_11AX == 1 */

		prRetSwRfb = qmHandleRxPackets(prAdapter, prSwRfb);
		if (prRetSwRfb != NULL)
			nicRxIndicatePackets(prAdapter, prRetSwRfb);
	} else {
		nicRxReturnRFB(prAdapter, prSwRfb);
		RX_INC_CNT(prRxCtrl, RX_CLASS_ERR_DROP_COUNT);
		RX_INC_CNT(prRxCtrl, RX_DROP_TOTAL_COUNT);
	}
}

void nicRxProcessEventPacket(struct ADAPTER *prAdapter,
			     struct SW_RFB *prSwRfb)
{
	struct mt66xx_chip_info *prChipInfo;
	struct CMD_INFO *prCmdInfo;
	struct WIFI_EVENT *prEvent;
	uint32_t u4Idx, u4Size;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	prChipInfo = prAdapter->chip_info;
	prEvent = (struct WIFI_EVENT *)
			(prSwRfb->pucRecvBuff + prChipInfo->rxd_size);

	if (prEvent->u2PacketLength > RX_GET_PACKET_MAX_SIZE(prAdapter)
		|| prEvent->u2PacketLength < sizeof(struct WIFI_EVENT)) {
		DBGLOG(NIC, ERROR,
			"Invalid RX event: ID[0x%02X] SEQ[%u] LEN[%u]\n",
			prEvent->ucEID, prEvent->ucSeqNum,
			prEvent->u2PacketLength);
		nicRxReturnRFB(prAdapter, prSwRfb);
		return;
	}

	if (prEvent->ucEID != EVENT_ID_DEBUG_MSG
	    && prEvent->ucEID != EVENT_ID_ASSERT_DUMP) {
		DBGLOG(NIC, TRACE,
			"RX EVENT: ID[0x%02X] SEQ[%u] LEN[%u]\n",
			prEvent->ucEID, prEvent->ucSeqNum,
			prEvent->u2PacketLength);
	}
#if (CFG_SUPPORT_STATISTICS == 1)
	wlanWakeLogEvent(prAdapter->prGlueInfo, prEvent->ucEID);
#endif
	/* Event handler table */
	u4Size = ARRAY_SIZE(arEventTable);

	for (u4Idx = 0; u4Idx < u4Size; u4Idx++) {
		if (prEvent->ucEID == arEventTable[u4Idx].eEID) {
			arEventTable[u4Idx].pfnHandler(prAdapter, prEvent);
			break;
		}
	}

	/* Event cannot be found in event handler table, use default action */
	if (u4Idx >= u4Size) {
		DBGLOG(RX, TRACE, "Not static config event: id=0x%02X, seq=%u",
				prEvent->ucEID, prEvent->ucSeqNum);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
						prEvent->ucSeqNum);

		if (prCmdInfo != NULL) {
			if (unlikely(prEvent->ucEID ==
					EVENT_ID_INIT_EVENT_CMD_RESULT) &&
					prCmdInfo->fgIsOid) {
				/*
				 * This event ID will be returned if CMD is not
				 * handled by FW. Here skip invoking
				 * pfCmdDoneHandler since the call back cannot
				 * distinguish the calling conditions.
				 * The callback accessing the event buffer is
				 * dangerous.
				 */
				DBGLOG(RX, DEBUG, "FW not support cmd 0x%02X",
						prCmdInfo->ucCID);
				kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
						0, WLAN_STATUS_FAILURE);
			} else {
				if (prCmdInfo->pfCmdDoneHandler)
					prCmdInfo->pfCmdDoneHandler(
						prAdapter, prCmdInfo,
						prEvent->aucBuffer);
				else if (prCmdInfo->fgIsOid)
					kalOidComplete(
						prAdapter->prGlueInfo,
						prCmdInfo,
						0,
						WLAN_STATUS_SUCCESS);
			}

			/* return prCmdInfo */
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		} else {
			DBGLOG(RX, TRACE,
				"UNHANDLED RX EVENT: ID[0x%02X] SEQ[%u] LEN[%u]\n",
			  prEvent->ucEID, prEvent->ucSeqNum,
			  prEvent->u2PacketLength);
		}
	}

	/* Reset Chip NoAck flag */
	if (prAdapter->fgIsChipNoAck) {
		DBGLOG_LIMITED(RX, WARN,
		       "Got response from chip, clear NoAck flag!\n");
		KAL_WARN_ON(TRUE);
	}
	prAdapter->ucOidTimeoutCount = 0;
	prAdapter->fgIsChipNoAck = FALSE;

	nicRxReturnRFB(prAdapter, prSwRfb);
}

static void nicDumpRxMgmtPacketHex(struct ADAPTER *prAdapter,
				   struct SW_RFB *prSwRfb)
{
	struct WLAN_MAC_HEADER *pHeader;
	uint16_t u2MgmtHexDump;
	uint8_t ucSubtype;

	if (!prAdapter || !prSwRfb || !prSwRfb->pvHeader)
		return;

	pHeader = prSwRfb->pvHeader;
	ucSubtype = pHeader->b4SubType;

	u2MgmtHexDump = prAdapter->rWifiVar.u2MgmtHexDump;

	if (nicNeedDumpActionFrame(pHeader, prSwRfb->u2PacketLen) ||
	    BIT(pHeader->b4SubType) & u2MgmtHexDump) {
		if (prSwRfb->u2PacketLen == 0) {
			DBGLOG(RX, ERROR, "Invalid u2PacketLen");
			return;
		}

		DBGDUMP_HEX(RX, INFO, "Dump RX MGMT Frame:",
			    pHeader, prSwRfb->u2PacketLen);
		DBGLOG(RX, INFO, "Dump RX MGMT Frame End subtype=%u (%u)",
		       pHeader->b4SubType, prSwRfb->u2PacketLen);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief nicRxProcessMgmtPacket is used to dispatch management frames
 *        to corresponding modules
 *
 * @param prAdapter Pointer to the Adapter structure.
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessMgmtPacket(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct GLUE_INFO *prGlueInfo;
	struct WLAN_MAC_HEADER *pHeader;
	uint8_t ucSubtype;
#if CFG_SUPPORT_802_11W
	/* BOOL   fgMfgDrop = FALSE; */
#endif
#if CFG_WIFI_SW_CIPHER_MISMATCH
	struct WLAN_MAC_HEADER *prWlanHeader = NULL;
#endif
	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	nicRxFillRFB(prAdapter, prSwRfb);

	if (!prSwRfb->pvHeader || !prSwRfb->pvPacket) {
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_NULL_PACKET_COUNT);
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_DROP_TOTAL_COUNT);
		nicRxReturnRFB(prAdapter, prSwRfb);
		return;
	}
	pHeader = prSwRfb->pvHeader;

	if (prSwRfb->u2HeaderLen < sizeof(struct WLAN_MAC_HEADER)
		|| prSwRfb->u2PacketLen < prSwRfb->u2HeaderLen
		|| prSwRfb->u2PacketLen > RX_GET_PACKET_MAX_SIZE(prAdapter)) {
		DBGLOG(RX, WARN,
			"Mgmt packet length check fail! length[H,P]:%u,%u\n",
			prSwRfb->u2HeaderLen, prSwRfb->u2PacketLen);
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_DROP_TOTAL_COUNT);
		nicRxReturnRFB(prAdapter, prSwRfb);
		return;
	}

#if CFG_WIFI_SW_CIPHER_MISMATCH
	prWlanHeader = (struct WLAN_MAC_HEADER *) prSwRfb->pvHeader;
#endif
	ucSubtype = pHeader->b4SubType;

#if CFG_SUPPORT_802_11W
	if (prSwRfb->fgIcvErr) {
		if (prSwRfb->ucSecMode == CIPHER_SUITE_BIP ||
		    prSwRfb->ucSecMode == CIPHER_SUITE_BIP_GMAC_256)
			DBGLOG(RSN, INFO, "[MFP] RX with BIP ICV ERROR\n");
		else
			DBGLOG(RSN, INFO, "[MFP] RX with ICV ERROR\n");

		nicRxReturnRFB(prAdapter, prSwRfb);
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_DROP_TOTAL_COUNT);
		return;
	}
#endif

#if CFG_WIFI_SW_CIPHER_MISMATCH
	if ((rsnCheckBipKeyInstalled(prAdapter, prSwRfb->prStaRec))
		&& (prSwRfb->prStaRec->ucStaState == STA_STATE_3)
		&& (!(prWlanHeader->u2FrameCtrl & MASK_FC_PROTECTED_FRAME))
		&& (prSwRfb->fgIsBC == FALSE)
		&& (prSwRfb->fgIsMC == FALSE)) {
		prSwRfb->fgIsCipherMS = TRUE;
	}
#endif

#if CFG_SUPPORT_SW_BIP_GMAC
	/* BIP-GMAC checking for BMC mgmt frame (deauth/disassoc) */
	if (rsnCheckBipGmacKeyInstall(prAdapter, prSwRfb->prStaRec)
		&& (prSwRfb->fgIsBC || prSwRfb->fgIsMC) &&
		(ucSubtype == 10 || ucSubtype == 12)) {
		/* HW doesn't support BIP-GMAC, will set fgIsCipherMS, driver
		 * should reset this flag and let rsnCheckBipGmac do the check
		 */
		prSwRfb->fgIsCipherMS = FALSE;
		if (rsnCheckBipGmac(prAdapter, prSwRfb) != TRUE) {
			prSwRfb->fgIsCipherMS = TRUE;
			DBGLOG(RX, WARN,
				"BIP-GMAC integrity check fail! Drop it\n");
			nicRxReturnRFB(prAdapter, prSwRfb);
			return;
		}
	}
#endif

	if (prAdapter->fgTestMode == FALSE) {
#if CFG_MGMT_FRAME_HANDLING
		prGlueInfo = prAdapter->prGlueInfo;
		if ((prGlueInfo == NULL) || (prGlueInfo->u4ReadyFlag == 0)) {
			DBGLOG(RX, WARN,
			   "Bypass this mgmt frame without wlanProbe done\n");
		} else if (apfnProcessRxMgtFrame[ucSubtype]) {
			if (prSwRfb->pvHeader) {
				nicDumpRxMgmtPacketHex(prAdapter, prSwRfb);
			} else {
				DBGLOG(RX, ERROR, "Invalid pvHeader\n");
			}

			switch (apfnProcessRxMgtFrame[ucSubtype] (prAdapter,
					prSwRfb)) {
			case WLAN_STATUS_PENDING:
				return;
			case WLAN_STATUS_SUCCESS:
			case WLAN_STATUS_FAILURE:
				break;

			default:
				DBGLOG(RX, WARN,
				       "Unexpected MMPDU(0x%02X) returned with abnormal status\n",
				       ucSubtype);
				break;
			}
		}
#endif
	}

	nicRxReturnRFB(prAdapter, prSwRfb);
}

void nicRxProcessMsduReport(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	halRxProcessMsduReport(prAdapter, prSwRfb);
}

void nicRxProcessRxReport(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct HW_MAC_RX_REPORT *prRxRpt;
	uint32_t *prRxv = NULL;
	uint32_t u4RxvOfst, u4Idx;
	uint16_t u2RxByteCntHw, u2RxByteCntSw, u2PRXVCnt;
	uint8_t ucDataType;
	struct SW_RX_RPT_BLK_RXV *prRxRptBlkRxv = NULL;

	ASSERT(prAdapter);
	ASSERT(prAdapter->prGlueInfo);

	prRxRpt = (struct HW_MAC_RX_REPORT *)prSwRfb->pucRecvBuff;
	u2RxByteCntHw = RX_RPT_GET_RX_BYTE_COUNT(prRxRpt);
	u2RxByteCntSw = RX_RPT_HDR_LEN + RX_RPT_USER_INFO_LEN;
	u2PRXVCnt = RX_RPT_GET_RXV_PRXV_BYTE_COUNT(prRxRpt);
	u4RxvOfst = (RX_RPT_HDR_LEN + RX_RPT_USER_INFO_LEN
		+ RX_RPT_BLK_HDR_LEN) << 2;

	/* Sanity check */
	if (RX_RPT_GET_RXV_BLK_EXIST(prRxRpt))
		u2RxByteCntSw += RX_RPT_BLK_HDR_LEN;
	if (RX_RPT_GET_RXV_TYPE_CRXV1_VLD(prRxRpt))
		u2RxByteCntSw += RX_RPT_BLK_CRXV1_LEN;
	if (RX_RPT_GET_RXV_TYPE_PRXV1_VLD(prRxRpt))
		u2RxByteCntSw += RX_RPT_BLK_PRXV1_LEN;
	if (RX_RPT_GET_RXV_TYPE_PRXV2_VLD(prRxRpt))
		u2RxByteCntSw += RX_RPT_BLK_PRXV2_LEN;
	if (RX_RPT_GET_RXV_TYPE_CRXV2_VLD(prRxRpt))
		u2RxByteCntSw += RX_RPT_BLK_CRXV2_LEN;

	if (u2RxByteCntHw != (u2RxByteCntSw << 2)) {
		DBGLOG(RX, ERROR, "Expect %d bytes but real %d bytes !!\n",
			(u2RxByteCntSw << 2), u2RxByteCntHw);
		return;
	}

	prSwRfb->ucStaRecIdx = secGetStaIdxByWlanIdx(prAdapter,
		(uint8_t) RX_RPT_GET_WLAN_ID(prRxRpt));

	if (prSwRfb->ucStaRecIdx >= CFG_STA_REC_NUM)
		return;

	/* Only check data frame */
	ucDataType = (uint8_t) RX_RPT_GET_FRAME_TYPE(prRxRpt);
	if (!RX_RPT_IS_DATA_FRAME(ucDataType))
		return;

	prRxRptBlkRxv = (struct SW_RX_RPT_BLK_RXV *)kalMemAlloc(
			sizeof(struct SW_RX_RPT_BLK_RXV), VIR_MEM_TYPE);
	if (!prRxRptBlkRxv) {
		DBGLOG(RX, ERROR, "Allocate prRxRptBlkRxv failed!\n");
		return;
	}

	if (RX_RPT_GET_RXV_BLK_EXIST(prRxRpt)) {
		if (RX_RPT_GET_RXV_TYPE_CRXV1_VLD(prRxRpt)) {
			prRxv = (uint32_t *)((uint8_t *)prRxRpt + u4RxvOfst);
			for (u4Idx = 0; u4Idx < RX_RPT_BLK_CRXV1_LEN; u4Idx++) {
				prRxRptBlkRxv->u4CRxv1[u4Idx] =
					*(prRxv + u4Idx);
			}

			u4RxvOfst += (RX_RPT_BLK_CRXV1_LEN << 2);
		}
		if (RX_RPT_GET_RXV_TYPE_PRXV1_VLD(prRxRpt)) {
			prRxv = (uint32_t *)((uint8_t *)prRxRpt + u4RxvOfst);
			for (u4Idx = 0; u4Idx < RX_RPT_BLK_PRXV1_LEN; u4Idx++)
				prRxRptBlkRxv->u4PRxv1[u4Idx] =
					*(prRxv + u4Idx);

			u4RxvOfst += (RX_RPT_BLK_PRXV1_LEN << 2);
		}
		if (RX_RPT_GET_RXV_TYPE_PRXV2_VLD(prRxRpt)) {
			prRxv = (uint32_t *)((uint8_t *)prRxRpt + u4RxvOfst);
			for (u4Idx = 0; u4Idx < RX_RPT_BLK_PRXV2_LEN; u4Idx++)
				prRxRptBlkRxv->u4PRxv2[u4Idx] =
					*(prRxv + u4Idx);

			u4RxvOfst += (RX_RPT_BLK_PRXV2_LEN << 2);
		}
		if (RX_RPT_GET_RXV_TYPE_CRXV2_VLD(prRxRpt)) {
			prRxv = (uint32_t *)((uint8_t *)prRxRpt + u4RxvOfst);
			for (u4Idx = 0; u4Idx < RX_RPT_BLK_CRXV2_LEN; u4Idx++)
				prRxRptBlkRxv->u4CRxv2[u4Idx] =
					*(prRxv + u4Idx);

			u4RxvOfst += (RX_RPT_BLK_CRXV2_LEN << 2);
		}
	}

	if (prRxRptBlkRxv)
		kalMemFree(prRxRptBlkRxv, VIR_MEM_TYPE,
			sizeof(struct SW_RX_RPT_BLK_RXV));
}

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
static void nicRxCheckWakeupReason(struct ADAPTER *prAdapter,
				   struct SW_RFB *prSwRfb)
{
	struct RX_DESC_OPS_T *prRxDescOps;

	prRxDescOps = prAdapter->chip_info->prRxDescOps;
	if (prRxDescOps->nic_rxd_check_wakeup_reason)
		prRxDescOps->nic_rxd_check_wakeup_reason(prAdapter, prSwRfb);
	else
		DBGLOG(RX, ERROR,
			"%s:: no nic_rxd_check_wakeup_reason??\n",
			__func__);
}
#endif /* CFG_SUPPORT_WAKEUP_REASON_DEBUG */

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
#if CFG_SUPPORT_ICS_TIMESYNC
static void nicRxWriteIcsTimeSync(struct ADAPTER *prAdapter,
	uint8_t *pucRecvBuff)
{
	struct ICS_BIN_TIMESYNC_HDR *prIcsTimeSyncHeader;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	ssize_t ret;

	if ((prAdapter->u2IcsSeqNo % prWifiVar->u4IcsTimeSyncCnt) != 0)
		return;

	/* prepare ICS header */
	prIcsTimeSyncHeader = (struct ICS_BIN_TIMESYNC_HDR *)pucRecvBuff;
	prIcsTimeSyncHeader->u4MagicNum = ICS_BIN_LOG_MAGIC_NUM;
	prIcsTimeSyncHeader->ucVer = 1;
	prIcsTimeSyncHeader->ucRsv = 0;
	prIcsTimeSyncHeader->u4Timestamp = 0;
	prIcsTimeSyncHeader->u2MsgID = RX_PKT_TYPE_SW_TIMESYNC;
	prIcsTimeSyncHeader->u2Length = sizeof(prIcsTimeSyncHeader->u8Time);

	prIcsTimeSyncHeader->u2SeqNo = prAdapter->u2IcsSeqNo++;
	prIcsTimeSyncHeader->u8Time = kalGetUIntRealTime();

	ret = kalIcsWrite(prAdapter->prGlueInfo,
		pucRecvBuff,
		sizeof(struct ICS_BIN_TIMESYNC_HDR));
	if (ret != sizeof(struct ICS_BIN_TIMESYNC_HDR)) {
		DBGLOG_LIMITED(NIC, DEBUG,
			"timesync dropped written:%ld rxByteCount:%d\n",
			ret, prIcsTimeSyncHeader->u2Length);
		RX_INC_CNT(&prAdapter->rRxCtrl,
			RX_ICS_DROP_COUNT);
	}
}
#endif /* CFG_SUPPORT_ICS_TIMESYNC */

static void nicRxProcessIcsLog(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct ICS_AGG_HEADER *prIcsAggHeader;
	struct ICS_BIN_LOG_HDR *prIcsBinLogHeader;
	uint32_t u4Size = 0;
	uint8_t *pucBuf;
	ssize_t ret;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prIcsAggHeader = (struct ICS_AGG_HEADER *)prSwRfb->prRxStatus;
	if (prIcsAggHeader->rxByteCount > ETHER_MAX_PKT_SZ) {
		DBGLOG_LIMITED(NIC, DEBUG, "rxByteCount too large[%u]\n",
			       prIcsAggHeader->rxByteCount);
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_ICS_DROP_COUNT);
		return;
	}

	u4Size = prIcsAggHeader->rxByteCount + sizeof(
			struct ICS_BIN_LOG_HDR);
	pucBuf = kalMemAlloc(u4Size, VIR_MEM_TYPE);
	if (!pucBuf) {
		DBGLOG_LIMITED(NIC, DEBUG, "pucBuf NULL\n");
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_ICS_DROP_COUNT);
		return;
	}

#if CFG_SUPPORT_ICS_TIMESYNC
	/* generate time sync ICS frame */
	nicRxWriteIcsTimeSync(prAdapter, pucBuf);
#endif /* CFG_SUPPORT_ICS_TIMESYNC */

	/* prepare ICS header */
	prIcsBinLogHeader = (struct ICS_BIN_LOG_HDR *)pucBuf;
	prIcsBinLogHeader->u4MagicNum = ICS_BIN_LOG_MAGIC_NUM;
	prIcsBinLogHeader->ucVer = 1;
	prIcsBinLogHeader->ucRsv = 0;
	prIcsBinLogHeader->u4Timestamp = 0;
	prIcsBinLogHeader->u2MsgID = RX_PKT_TYPE_ICS;
	prIcsBinLogHeader->u2Length = prIcsAggHeader->rxByteCount;

	prIcsBinLogHeader->u2SeqNo = prAdapter->u2IcsSeqNo++;

	/* prepare ICS frame */
	kalMemCopy(pucBuf + sizeof(struct ICS_BIN_LOG_HDR),
			prIcsAggHeader, prIcsAggHeader->rxByteCount);

	/* write to ring, ret: written */
	ret = kalIcsWrite(prAdapter->prGlueInfo, pucBuf, u4Size);
	if (ret != u4Size) {
		DBGLOG_LIMITED(NIC, DEBUG,
			"dropped written:%zd rxByteCount:%u\n",
			ret, prIcsAggHeader->rxByteCount);
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_ICS_DROP_COUNT);
	}

	kalMemFree(pucBuf, VIR_MEM_TYPE, u4Size);
}
#endif /* #if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1)) */

void nicRxProcessPacketType(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct RX_CTRL *prRxCtrl;
	struct mt66xx_chip_info *prChipInfo;

	prRxCtrl = &prAdapter->rRxCtrl;
	prChipInfo = prAdapter->chip_info;

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
	if (kalIsWakeupByWlan(prAdapter))
		nicRxCheckWakeupReason(prAdapter, prSwRfb);
#endif

	switch (prSwRfb->ucPacketType) {
	case RX_PKT_TYPE_RX_DATA:
		if (HAL_IS_RX_DIRECT(prAdapter)) {
			KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter,
				SPIN_LOCK_RX_DIRECT);
			nicRxProcessDataPacket(
				prAdapter,
				prSwRfb);
			KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
				SPIN_LOCK_RX_DIRECT);
		} else {
			nicRxProcessDataPacket(
				prAdapter,
				prSwRfb);
		}
		break;

	case RX_PKT_TYPE_SW_DEFINED:
		/* HIF_RX_PKT_TYPE_EVENT */
		if ((NIC_RX_GET_U2_SW_PKT_TYPE(
			prSwRfb->prRxStatus) &
		     prChipInfo->u2RxSwPktBitMap) ==
		    prChipInfo->u2RxSwPktEvent) {
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			if (IS_UNI_EVENT(prSwRfb->pucRecvBuff +
					prChipInfo->rxd_size))
				nicRxProcessUniEventPacket(prAdapter, prSwRfb);
			else
#endif
				nicRxProcessEventPacket(prAdapter, prSwRfb);
		}
		/* case HIF_RX_PKT_TYPE_MANAGEMENT: */
		else if ((NIC_RX_GET_U2_SW_PKT_TYPE(
				prSwRfb->prRxStatus)
			& prChipInfo->u2RxSwPktBitMap)
			== prChipInfo->u2RxSwPktFrame){

			/* OFLD pkts should go data flow
			 * 1: EAPOL
			 * 2: ARP / NS
			 * 3: TDLS
			 */
			RX_STATUS_GET(
				prChipInfo->prRxDescOps,
				prSwRfb->ucOFLD,
				get_ofld,
				prSwRfb->prRxStatus);
			RX_STATUS_GET(
				prChipInfo->prRxDescOps,
				prSwRfb->fgHdrTran,
				get_HdrTrans,
				prSwRfb->prRxStatus);
			if ((prSwRfb->ucOFLD) || (prSwRfb->fgHdrTran)) {
				if (HAL_IS_RX_DIRECT(prAdapter)) {
					KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter,
						SPIN_LOCK_RX_DIRECT);
					nicRxProcessDataPacket(
							prAdapter, prSwRfb);
					KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
						SPIN_LOCK_RX_DIRECT);
				} else {
					nicRxProcessDataPacket(
							prAdapter, prSwRfb);
				}
			}
			else
				nicRxProcessMgmtPacket(
				prAdapter, prSwRfb);
		} else {
			DBGLOG(RX, ERROR,
				"u2PktTYpe(0x%04X) is OUT OF DEF.!!!\n",
			  NIC_RX_GET_U2_SW_PKT_TYPE(
				prSwRfb->prRxStatus));
			DBGLOG_MEM8(RX, ERROR,
				prSwRfb->pucRecvBuff,
				prSwRfb->u2RxByteCount);

			/*ASSERT(0);*/
			nicRxReturnRFB(prAdapter,
				prSwRfb);
			RX_INC_CNT(prRxCtrl,
				RX_TYPE_ERR_DROP_COUNT);
			RX_INC_CNT(prRxCtrl,
				RX_DROP_TOTAL_COUNT);

		}
		break;

	case RX_PKT_TYPE_MSDU_REPORT:
		nicRxProcessMsduReport(prAdapter,
			prSwRfb);
		nicRxReturnRFB(prAdapter, prSwRfb);
		break;

	case RX_PKT_TYPE_RX_REPORT:
		nicRxProcessRxReport(prAdapter, prSwRfb);
		nicRxReturnRFB(prAdapter, prSwRfb);
		break;

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
	case RX_PKT_TYPE_ICS:
		if ((prAdapter->fgEnTmacICS
			|| prAdapter->fgEnRmacICS)
				|| prAdapter->rWifiVar.fgDynamicIcs)
			nicRxProcessIcsLog(prAdapter, prSwRfb);
		RX_INC_CNT(prRxCtrl, RX_ICS_LOG_COUNT);
		nicRxReturnRFB(prAdapter, prSwRfb);
		break;

#if (CFG_SUPPORT_PHY_ICS == 1)
	case RX_PKT_TYPE_PHY_ICS:
		if (prAdapter->fgEnPhyICS == TRUE) {
			nicRxProcessIcsLog(prAdapter, prSwRfb);
			/* DBGLOG(RX, ERROR, "ucPacketType = %d\n", */
				/* prSwRfb->ucPacketType); */
		}
		nicRxReturnRFB(prAdapter, prSwRfb);
		break;
#endif /* #if CFG_SUPPORT_PHY_ICS */
#endif /* CFG_SUPPORT_ICS */


	/* case HIF_RX_PKT_TYPE_TX_LOOPBACK: */
	/* case HIF_RX_PKT_TYPE_MANAGEMENT: */
	case RX_PKT_TYPE_TX_STATUS:
	case RX_PKT_TYPE_RX_VECTOR:
	case RX_PKT_TYPE_TM_REPORT:
	default:
		DBGLOG(RX, ERROR, "ucPacketType = %d\n",
		       prSwRfb->ucPacketType);
		DBGLOG_MEM32(RX, ERROR, prSwRfb->prRxStatus,
			     prChipInfo->rxd_size);
		nicRxReturnRFB(prAdapter, prSwRfb);
		RX_INC_CNT(prRxCtrl,
			RX_TYPE_ERR_DROP_COUNT);
		RX_INC_CNT(prRxCtrl,
			RX_DROP_TOTAL_COUNT);
		break;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief nicProcessRFBs is used to process RFBs in the rReceivedRFBList queue.
 *
 * @param prAdapter Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessRFBs(struct ADAPTER *prAdapter)
{
	struct RX_CTRL *prRxCtrl;
	struct SW_RFB *prSwRfb = (struct SW_RFB *) NULL;
	struct QUE rTempRfbList;
	struct QUE *prTempRfbList = &rTempRfbList;
	uint32_t u4RxLoopCount, u4Tick;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);

	prRxCtrl->ucNumIndPacket = 0;
	prRxCtrl->ucNumRetainedPacket = 0;
	u4RxLoopCount = prAdapter->rWifiVar.u4TxRxLoopCount;
	u4Tick = kalGetTimeTick();

	QUEUE_INITIALIZE(prTempRfbList);

	while (u4RxLoopCount--) {
		while (QUEUE_IS_NOT_EMPTY(&prRxCtrl->rReceivedRfbList)) {

			/* check process RFB timeout */
			if ((kalGetTimeTick() - u4Tick) > RX_PROCESS_TIMEOUT) {
				DBGLOG(RX, WARN,
					"Process RFBs timeout, pending count: %u\n",
					RX_GET_RECEIVED_RFB_CNT(prRxCtrl));
				kalSetRxProcessEvent(prAdapter->prGlueInfo);
				break;
			}

			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
			QUEUE_MOVE_ALL(prTempRfbList,
				&prRxCtrl->rReceivedRfbList);
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);

			while (QUEUE_IS_NOT_EMPTY(prTempRfbList)) {
				QUEUE_REMOVE_HEAD(prTempRfbList,
					prSwRfb, struct SW_RFB *);

				if (!prSwRfb)
					break;
#if CFG_RFB_TRACK
				RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
					RFB_TRACK_MAIN);
#endif /* CFG_RFB_TRACK */

				/* Too many leading tabs -
				 * consider code refactoring
				 */
				nicRxProcessPacketType(prAdapter, prSwRfb);
			}

			if (prRxCtrl->ucNumIndPacket > 0) {
				RX_ADD_CNT(prRxCtrl, RX_DATA_RETAINED_COUNT,
					   prRxCtrl->ucNumRetainedPacket);
#if !CFG_SUPPORT_MULTITHREAD
#if CFG_NATIVE_802_11
				kalRxIndicatePkts(prAdapter->prGlueInfo,
					(uint32_t) prRxCtrl->ucNumIndPacket,
					(uint32_t)
						prRxCtrl->ucNumRetainedPacket);
#else
				kalRxIndicatePkts(prAdapter->prGlueInfo,
				  prRxCtrl->apvIndPacket,
				  (uint32_t) prRxCtrl->ucNumIndPacket);
#endif
#endif
				kalPerMonStart(prAdapter->prGlueInfo);
			}
		}
	}
}				/* end of nicRxProcessRFBs() */

void *__nicRxPacketAlloc(struct GLUE_INFO *pr, uint8_t **ppucData,
	int32_t i4Idx)
{
#if CFG_SUPPORT_RX_PAGE_POOL
	return kalAllocRxSkbFromPp(pr, ppucData, i4Idx);
#else
	return kalPacketAlloc(pr, CFG_RX_MAX_MPDU_SIZE, FALSE, ppucData);
#endif /* CFG_SUPPORT_RX_PAGE_POOL */
}

static void *nicRxPacketAlloc(struct GLUE_INFO *pr, uint8_t **ppucData)
{
#if CFG_SUPPORT_SKB_ALLOC_WORK
	void *pvPacket = NULL;

	kalSkbAllocDeqSkb(pr, &pvPacket, ppucData);
	if (!pvPacket) {
		pvPacket = kalAllocRxSkbFromPp(
			pr, ppucData, PAGE_POOL_LAST_IDX);
	}

	return pvPacket;
#else
	return __nicRxPacketAlloc(pr, ppucData, -1);
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Setup a RFB and allocate the os packet to the RFB
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param prSwRfb        Pointer to the RFB
 *
 * @retval WLAN_STATUS_SUCCESS
 * @retval WLAN_STATUS_RESOURCES
 */
/*----------------------------------------------------------------------------*/
static uint32_t __nicRxSetupRFB(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	void *pvPacket;
	uint8_t *pucRecvBuff = NULL;
#if CFG_RFB_TRACK
	uint32_t u4RfbTrackId;
#endif /* CFG_RFB_TRACK */

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

#if CFG_RFB_TRACK
	/* store rfb track id before memzero */
	u4RfbTrackId = prSwRfb->u4RfbTrackId;
#endif /* CFG_RFB_TRACK */
	if (!prSwRfb->pvPacket) {
		kalMemZero(prSwRfb, sizeof(struct SW_RFB));
		pvPacket = nicRxPacketAlloc(prAdapter->prGlueInfo,
						&pucRecvBuff);
		if (pvPacket == NULL)
			return WLAN_STATUS_RESOURCES;

		prSwRfb->pvPacket = pvPacket;
		prSwRfb->pucRecvBuff = (void *) pucRecvBuff;
	} else {
		kalMemZero(((uint8_t *) prSwRfb + OFFSET_OF(struct SW_RFB,
				prRxStatus)),
			   (sizeof(struct SW_RFB) - OFFSET_OF(struct SW_RFB,
					   prRxStatus)));
	}

	prSwRfb->prRxStatus = prSwRfb->pucRecvBuff;
#if CFG_RFB_TRACK
	prSwRfb->u4RfbTrackId = u4RfbTrackId;
#endif /* CFG_RFB_TRACK */

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicRxSetupRFB(struct ADAPTER *prAdapter,
		       struct SW_RFB *prSwRfb)
{
#if CFG_DYNAMIC_RFB_ADJUSTMENT
	struct RX_CTRL *prRxCtrl;

	prRxCtrl = &prAdapter->rRxCtrl;
	if (RX_GET_UNUSE_RFB_CNT(prRxCtrl) < nicRxGetUnUseCnt(prAdapter))
		return WLAN_STATUS_RESOURCES;
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */

	return __nicRxSetupRFB(prAdapter, prSwRfb);
}

void nicRxConcatRxQue(struct ADAPTER *prAdapter,
	struct QUE *prQue, uint8_t ucTrackState, uint8_t *fileAndLine)
{
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;
#if CFG_RFB_TRACK
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct SW_RFB *prSwRfb = NULL;
#endif /* CFG_RFB_TRACK */

	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
#if CFG_RFB_TRACK
	if (IS_FEATURE_ENABLED(prWifiVar->fgRfbTrackEn)) {
		while (QUEUE_IS_NOT_EMPTY(prQue)) {
			QUEUE_REMOVE_HEAD(prQue, prSwRfb, struct SW_RFB *);
			if (!prSwRfb)
				break;
			__RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
				ucTrackState, fileAndLine);
			QUEUE_INSERT_TAIL(&prRxCtrl->rReceivedRfbList,
				&prSwRfb->rQueEntry);
		}
	} else
#endif /* CFG_RFB_TRACK */
	{
		if (QUEUE_IS_NOT_EMPTY(prQue))
			QUEUE_CONCATENATE_QUEUES(
				&prRxCtrl->rReceivedRfbList,
				prQue);
	}
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
}

void nicRxConcatFreeQue(struct ADAPTER *prAdapter,
	struct QUE *prQue, uint8_t ucTrackState, uint8_t *fileAndLine)
{
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;
#if CFG_RFB_TRACK
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct SW_RFB *prSwRfb = NULL;
#endif /* CFG_RFB_TRACK */

	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
#if CFG_RFB_TRACK
	if (IS_FEATURE_ENABLED(prWifiVar->fgRfbTrackEn)) {
		while (QUEUE_IS_NOT_EMPTY(prQue)) {
			QUEUE_REMOVE_HEAD(prQue, prSwRfb, struct SW_RFB *);
			if (!prSwRfb)
				break;
			__RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
				ucTrackState, fileAndLine);
			QUEUE_INSERT_TAIL(&prRxCtrl->rFreeSwRfbList,
				&prSwRfb->rQueEntry);
		}
	} else
#endif /* CFG_RFB_TRACK */
	{
		if (QUEUE_IS_NOT_EMPTY(prQue))
			QUEUE_CONCATENATE_QUEUES(
				&prRxCtrl->rFreeSwRfbList,
				prQue);
	}
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
}

void nicRxDequeueFreeQue(struct ADAPTER *prAdapter, uint32_t u4Num,
	struct QUE *prQue, uint8_t ucTrackState, uint8_t *fileAndLine)
{
	uint32_t i;
	struct RX_CTRL *prRxCtrl;
	struct SW_RFB *prSwRfb = NULL;

	KAL_SPIN_LOCK_DECLARATION();

	QUEUE_INITIALIZE(prQue);
	prRxCtrl = &prAdapter->rRxCtrl;

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
	for (i = 0; i < u4Num; i++) {
		QUEUE_REMOVE_HEAD(&prRxCtrl->rFreeSwRfbList,
			prSwRfb, struct SW_RFB *);
		if (!prSwRfb)
			break;
#if CFG_RFB_TRACK
		__RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
			ucTrackState, fileAndLine);
#endif /* CFG_RFB_TRACK */
		QUEUE_INSERT_TAIL(prQue, &prSwRfb->rQueEntry);
	}
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
}

void nicRxQueueMoveAll(struct ADAPTER *prAdapter,
	struct QUE *prDstQue, struct QUE *prSrcQue,
	enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
	uint8_t ucTrackState, uint8_t *fileAndLine)
{
#if CFG_RFB_TRACK
	struct SW_RFB *prSwRfb = NULL;
#endif /* CFG_RFB_TRACK */

	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, rLockCategory);
#if CFG_RFB_TRACK
	while (QUEUE_IS_NOT_EMPTY(prSrcQue)) {
		QUEUE_REMOVE_HEAD(prSrcQue, prSwRfb, struct SW_RFB *);
		if (!prSwRfb)
			break;
		__RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
			ucTrackState, fileAndLine);
		QUEUE_INSERT_TAIL(prDstQue, &prSwRfb->rQueEntry);
	}
#else
	QUEUE_MOVE_ALL(prDstQue, prSrcQue);
#endif /* CFG_RFB_TRACK */
	KAL_RELEASE_SPIN_LOCK(prAdapter, rLockCategory);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to acquire a RFB from free swrfb list
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param num          num of swrfb to acquire
 *
 * @return swrfb
 */
/*----------------------------------------------------------------------------*/
struct SW_RFB *nicRxAcquireRFB(struct ADAPTER *prAdapter, uint16_t num,
	uint8_t ucTrackState, uint8_t *fileAndLine)
{
	uint16_t i;
	struct QUE tmp, *que = &tmp;
	struct SW_RFB *rfb = NULL;
	struct RX_CTRL *ctrl;
	uint32_t u4Status;

	ctrl = &prAdapter->rRxCtrl;

	QUEUE_INITIALIZE(que);

	nicRxDequeueFreeQue(prAdapter, num, que,
		ucTrackState, fileAndLine);

	if (likely(que->u4NumElem == num))
		return QUEUE_GET_HEAD(que);

	DBGLOG_LIMITED(RX, WARN,
		"No More RFB caller=%pS\n", KAL_TRACE);

	NIC_RX_CONCAT_FREE_QUE(prAdapter, que);

	/* Fallback, allocate from spared */
	QUEUE_INITIALIZE(que);
	for (i = 0; i < num; i++) {
		rfb = kalMemAlloc(sizeof(struct SW_RFB), VIR_MEM_TYPE);
		if (unlikely(!rfb)) {
			DBGLOG_LIMITED(RX, WARN,
				"No RFB from spared caller=%pS\n", KAL_TRACE);
			goto error;
		}
		u4Status = nicRxSetupRFB(prAdapter, rfb);
		if (unlikely(u4Status != WLAN_STATUS_SUCCESS)) {
			kalMemFree(rfb, VIR_MEM_TYPE, sizeof(struct SW_RFB));
			goto error;
		}
		QUEUE_INSERT_TAIL(que, &rfb->rQueEntry);
	}

	return QUEUE_GET_HEAD(que);

error:
	/* The flow shall never reach here */
	if (i > 0) {
		do {
			QUEUE_REMOVE_HEAD(que, rfb, struct SW_RFB *);
			nicRxReturnRFB(prAdapter, rfb);
		} while (rfb);
	}
	return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to add a new received rfb
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param rfb          received swrfb, it could be a list of swrfb
 *
 * @return
 */
/*----------------------------------------------------------------------------*/

void nicRxReceiveRFB(struct ADAPTER *prAdapter, struct SW_RFB *rfb)
{
	struct SW_RFB *next = NULL;
	struct RX_CTRL *ctrl;

	KAL_SPIN_LOCK_DECLARATION();

	if (!rfb)
		return;

	ctrl = &prAdapter->rRxCtrl;
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);

	while(rfb) {
		next = QUEUE_GET_NEXT_ENTRY(rfb);
		QUEUE_INSERT_TAIL(&ctrl->rReceivedRfbList, &rfb->rQueEntry);
		rfb = next;
	}

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to copy swrfb data to another
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param prDst          destination
 * @param prSrc          source
 *
 * @return status
 */
/*----------------------------------------------------------------------------*/

uint32_t nicRxCopyRFB(struct ADAPTER *prAdapter,
		       struct SW_RFB *prDst, struct SW_RFB *prSrc)
{
	kalMemCopy(prDst->pucRecvBuff, prSrc->pucRecvBuff,
	       ALIGN_4(prSrc->u2RxByteCount + HIF_RX_HW_APPENDED_LEN));
	prDst->ucPacketType = prSrc->ucPacketType;
	nicRxFillRFB(prAdapter, prDst);
	GLUE_COPY_PRIV_DATA(prDst->pvPacket, prSrc->pvPacket);

	return WLAN_STATUS_SUCCESS;
}

u_int8_t isRfbFromSpared(struct RX_CTRL *prRxCtrl, struct SW_RFB *prSwRfb)
{
	return prSwRfb < prRxCtrl->prRxCached ||
		prSwRfb > prRxCtrl->prRxCached + CFG_RX_MAX_PKT_NUM;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to put a RFB back onto the "RFB with Buffer"
 *        list or "RFB without buffer" list according to pvPacket.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param prSwRfb          Pointer to the RFB
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void __nicRxReturnRFB(struct ADAPTER *prAdapter,
		    struct SW_RFB *prSwRfb)
{
	struct RX_CTRL *prRxCtrl;
	struct QUE_ENTRY *prQueEntry;
	struct GLUE_INFO *prGlueInfo;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	if (!prSwRfb)
		return;

	prRxCtrl = &prAdapter->rRxCtrl;
	prQueEntry = &prSwRfb->rQueEntry;
	prGlueInfo = prAdapter->prGlueInfo;

	ASSERT(prQueEntry);

	/* The processing on this RFB is done,
	 * so put it back on the tail of our list
	 */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

	if (prSwRfb->pvPacket) {
		/* QUEUE_INSERT_TAIL */
		QUEUE_INSERT_TAIL(&prRxCtrl->rFreeSwRfbList, prQueEntry);
#if CFG_RFB_TRACK
		RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb, RFB_TRACK_FREE);
#endif /* CFG_RFB_TRACK */

#if !CFG_SUPPORT_SKB_ALLOC_WORK
		/* SkbAllocWork call it later in wlanReturnPacketDelaySetup */
		if (prAdapter->ulNoMoreRfb != 0) {
			DBGLOG_LIMITED(RX, DEBUG,
				"Free rfb and set IntEvent!!!!!\n");
			kalSetDrvIntEvent(prGlueInfo);
		}
#endif /* !CFG_SUPPORT_SKB_ALLOC_WORK */
	} else {
		/* QUEUE_INSERT_TAIL */
		QUEUE_INSERT_TAIL(&prRxCtrl->rIndicatedRfbList, prQueEntry);
#if CFG_RFB_TRACK
		RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb, RFB_TRACK_INDICATED);
#endif /* CFG_RFB_TRACK */
	}
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

	/* Trigger Rx if there are free SwRfb */
	if (halIsPendingRx(prAdapter)
	    && (RX_GET_FREE_RFB_CNT(prRxCtrl) > 0))
		kalSetIntEvent(prGlueInfo);
} /* end of __nicRxReturnRFB() */

void nicRxReturnRFB(struct ADAPTER *prAdapter,
		    struct SW_RFB *prSwRfb)
{
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;

	if (!prSwRfb)
		return;

#if CFG_SUPPORT_WED_PROXY
	wedHwRxInfoFree(prSwRfb);
#endif

	if (isRfbFromSpared(prRxCtrl, prSwRfb)) {
		if (prSwRfb->pvPacket)
			kalPacketFree(prAdapter->prGlueInfo,
				prSwRfb->pvPacket);
		kalMemFree(prSwRfb, VIR_MEM_TYPE, sizeof(struct SW_RFB));
		return;
	}

#if CFG_SUPPORT_RX_PAGE_POOL
	if (prSwRfb->pvPacket)
		kalSkbReuseCheck(prSwRfb);
#endif /* CFG_SUPPORT_RX_PAGE_POOL */

#if CFG_DYNAMIC_RFB_ADJUSTMENT
	if (RX_GET_UNUSE_RFB_CNT(prRxCtrl) < nicRxGetUnUseCnt(prAdapter)) {
		nicRxReturnUnUseRFB(prAdapter, prSwRfb);
		return;
	}
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */

	__nicRxReturnRFB(prAdapter, prSwRfb);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process rx interrupt. When the rx
 *        Interrupt is asserted, it means there are frames in queue.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicProcessRxInterrupt(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);
	prAdapter->prGlueInfo->IsrRxCnt++;

	if (halIsHifStateSuspend(prAdapter)) {
		DBGLOG(RX, WARN, "suspend RX INT\n");
	}

	/* SER break point */
	if (nicSerIsRxStop(prAdapter)) {
		/* Skip following Rx handling */
		return;
	}

	halProcessRxInterrupt(prAdapter);

#if CFG_SUPPORT_MULTITHREAD
	kalSetRxProcessEvent(prAdapter->prGlueInfo);
#else
	nicRxProcessRFBs(prAdapter);
#endif

	return;

}				/* end of nicProcessRxInterrupt() */

#if CFG_TCP_IP_CHKSUM_OFFLOAD
/*----------------------------------------------------------------------------*/
/*!
 * @brief Used to update IP/TCP/UDP checksum statistics of RX Module.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 * @param aeCSUM     The array of checksum result.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxUpdateCSUMStatistics(struct ADAPTER *prAdapter,
		const enum ENUM_CSUM_RESULT aeCSUM[])
{
	struct RX_CTRL *prRxCtrl;

	ASSERT(prAdapter);
	ASSERT(aeCSUM);

	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);

	if ((aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_SUCCESS)
	    || (aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_SUCCESS)) {
		/* count success num */
		RX_INC_CNT(prRxCtrl, RX_CSUM_IP_SUCCESS_COUNT);
	} else if ((aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_FAILED)
		   || (aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_FAILED)) {
		RX_INC_CNT(prRxCtrl, RX_CSUM_IP_FAILED_COUNT);
	} else if ((aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_NONE)
		   && (aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_NONE)) {
		RX_INC_CNT(prRxCtrl, RX_CSUM_UNKNOWN_L3_PKT_COUNT);
	} else {
		ASSERT(0);
	}

	if (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_SUCCESS) {
		/* count success num */
		RX_INC_CNT(prRxCtrl, RX_CSUM_TCP_SUCCESS_COUNT);
	} else if (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_FAILED) {
		RX_INC_CNT(prRxCtrl, RX_CSUM_TCP_FAILED_COUNT);
	} else if (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_SUCCESS) {
		RX_INC_CNT(prRxCtrl, RX_CSUM_UDP_SUCCESS_COUNT);
	} else if (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_FAILED) {
		RX_INC_CNT(prRxCtrl, RX_CSUM_UDP_FAILED_COUNT);
	} else if ((aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_NONE)
		   && (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_NONE)) {
		RX_INC_CNT(prRxCtrl, RX_CSUM_UNKNOWN_L4_PKT_COUNT);
	} else {
		ASSERT(0);
	}

}				/* end of nicRxUpdateCSUMStatistics() */
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to query current status of RX Module.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param pucBuffer      Pointer to the message buffer.
 * @param pu4Count      Pointer to the buffer of message length count.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxQueryStatus(struct ADAPTER *prAdapter,
		      uint8_t *pucBuffer, uint32_t *pu4Count)
{
	struct RX_CTRL *prRxCtrl;
	uint8_t *pucCurrBuf = pucBuffer;
	uint32_t u4CurrCount;

	ASSERT(prAdapter);
	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);
	ASSERT(pu4Count);

#define SPRINTF_RX_QSTATUS(arg) \
	{ \
		u4CurrCount = \
			kalScnprintf(pucCurrBuf, *pu4Count, PRINTF_ARG arg); \
		pucCurrBuf += (uint8_t)u4CurrCount; \
		*pu4Count -= u4CurrCount; \
	}


	SPRINTF_RX_QSTATUS(("\n\nRX CTRL STATUS:"));
	SPRINTF_RX_QSTATUS(("\n==============="));
	SPRINTF_RX_QSTATUS(("\nFREE RFB w/i BUF LIST :%9u",
		RX_GET_FREE_RFB_CNT(prRxCtrl)));
	SPRINTF_RX_QSTATUS(("\nFREE RFB w/o BUF LIST :%9u",
		RX_GET_INDICATED_RFB_CNT(prRxCtrl)));
	SPRINTF_RX_QSTATUS(("\nRECEIVED RFB LIST     :%9u",
		RX_GET_RECEIVED_RFB_CNT(prRxCtrl)));

	SPRINTF_RX_QSTATUS(("\n\n"));

	/* *pu4Count = (UINT_32)((UINT_32)pucCurrBuf - (UINT_32)pucBuffer); */

}				/* end of nicRxQueryStatus() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Clear RX related counters
 *
 * @param prAdapter Pointer of Adapter Data Structure
 *
 * @return - (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxClearStatistics(struct ADAPTER *prAdapter)
{
	struct RX_CTRL *prRxCtrl;

	ASSERT(prAdapter);
	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);

	RX_RESET_ALL_CNTS(prRxCtrl);

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to query current statistics of RX Module.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param pucBuffer      Pointer to the message buffer.
 * @param pu4Count       Pointer to the buffer of message length count.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxQueryStatistics(struct ADAPTER *prAdapter,
			  uint8_t *pucBuffer, uint32_t *pu4Count)
{
	struct RX_CTRL *prRxCtrl;
	uint8_t *pucCurrBuf = pucBuffer;
	uint32_t u4CurrCount;

	ASSERT(prAdapter);
	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);
	ASSERT(pu4Count);

#define SPRINTF_RX_COUNTER(eCounter) \
	{ \
		u4CurrCount = kalScnprintf(pucCurrBuf, *pu4Count, \
			"%-30s : %u\n", #eCounter, \
			(uint32_t)prRxCtrl->au8Statistics[eCounter]); \
		pucCurrBuf += (uint8_t)u4CurrCount; \
		*pu4Count -= u4CurrCount; \
	}

	SPRINTF_RX_COUNTER(RX_MPDU_TOTAL_COUNT);
	SPRINTF_RX_COUNTER(RX_SIZE_ERR_DROP_COUNT);
	SPRINTF_RX_COUNTER(RX_DATA_INDICATION_COUNT);
	SPRINTF_RX_COUNTER(RX_DATA_RETURNED_COUNT);
	SPRINTF_RX_COUNTER(RX_DATA_RETAINED_COUNT);

#if CFG_TCP_IP_CHKSUM_OFFLOAD || CFG_TCP_IP_CHKSUM_OFFLOAD_NDIS_60
	SPRINTF_RX_COUNTER(RX_CSUM_TCP_FAILED_COUNT);
	SPRINTF_RX_COUNTER(RX_CSUM_UDP_FAILED_COUNT);
	SPRINTF_RX_COUNTER(RX_CSUM_IP_FAILED_COUNT);
	SPRINTF_RX_COUNTER(RX_CSUM_TCP_SUCCESS_COUNT);
	SPRINTF_RX_COUNTER(RX_CSUM_UDP_SUCCESS_COUNT);
	SPRINTF_RX_COUNTER(RX_CSUM_IP_SUCCESS_COUNT);
	SPRINTF_RX_COUNTER(RX_CSUM_UNKNOWN_L4_PKT_COUNT);
	SPRINTF_RX_COUNTER(RX_CSUM_UNKNOWN_L3_PKT_COUNT);
	SPRINTF_RX_COUNTER(RX_IP_V6_PKT_CCOUNT);
#endif

	/* *pu4Count = (UINT_32)(pucCurrBuf - pucBuffer); */

	nicRxClearStatistics(prAdapter);

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Use to distinguish waiting pre-cal or not
 *
 * @param prAdapter pointer to the Adapter handler
 * @param pucRspBuffer pointer to the Response buffer
 *
 * @retval WLAN_STATUS_SUCCESS: Response packet has been read
 * @retval WLAN_STATUS_FAILURE: Read Response packet timeout or error occurred
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t
nicRxWaitResponse(struct ADAPTER *prAdapter,
		  uint8_t ucPortIdx, uint8_t *pucRspBuffer,
		  uint32_t u4MaxRespBufferLen, uint32_t *pu4Length) {
	return nicRxWaitResponseByWaitingInterval(
				prAdapter, ucPortIdx,
				pucRspBuffer, u4MaxRespBufferLen,
				pu4Length, CFG_DEFAULT_SLEEP_WAITING_INTERVAL,
				CFG_DEFAULT_RX_RESPONSE_TIMEOUT);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Read the Response data from data port
 *
 * @param prAdapter pointer to the Adapter handler
 * @param pucRspBuffer pointer to the Response buffer
 *
 * @retval WLAN_STATUS_SUCCESS: Response packet has been read
 * @retval WLAN_STATUS_FAILURE: Read Response packet timeout or error occurred
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t
nicRxWaitResponseByWaitingInterval(struct ADAPTER *prAdapter,
		  uint8_t ucPortIdx, uint8_t *pucRspBuffer,
		  uint32_t u4MaxRespBufferLen, uint32_t *pu4Length,
		  uint32_t u4WaitingInterval, uint32_t u4TimeoutValue) {
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_EVENT *prEvent;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
#if (CFG_SUPPORT_DEBUG_SOP == 1)
	struct CHIP_DBG_OPS *prChipDbg = (struct CHIP_DBG_OPS *) NULL;
#endif

	if (prAdapter == NULL) {
		DBGLOG(INIT, WARN, "prAdapter is NULL\n");

		return WLAN_STATUS_FAILURE;
	}

	prChipInfo = prAdapter->chip_info;

	u4Status = halRxWaitResponse(prAdapter, ucPortIdx,
					pucRspBuffer, u4MaxRespBufferLen,
					pu4Length, u4WaitingInterval,
					u4TimeoutValue);
	if (u4Status == WLAN_STATUS_SUCCESS) {
		prEvent = (struct WIFI_EVENT *)
			(pucRspBuffer + prChipInfo->rxd_size);

		DBGLOG(INIT, TRACE,
		       "RX EVENT: ID[0x%02X] SEQ[%u] LEN[%u] VER[%d]\n",
		       prEvent->ucEID, prEvent->ucSeqNum,
		       prEvent->u2PacketLength, prEvent->ucEventVersion);
		DBGLOG_MEM8(RX, TRACE, pucRspBuffer,
			prChipInfo->rxd_size + prEvent->u2PacketLength);
	} else {
		prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;
		halPrintHifDbgInfo(prAdapter);
		DBGLOG(RX, ERROR, "halRxWaitResponse fail!status %X\n",
		       u4Status);
#if (CFG_SUPPORT_DEBUG_SOP == 1)
		prChipDbg = prAdapter->chip_info->prDebugOps;
		prChipDbg->show_debug_sop_info(prAdapter, SLAVENORESP);
#endif
	}

	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Set filter to enable Promiscuous Mode
 *
 * @param prAdapter          Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxEnablePromiscuousMode(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

}				/* end of nicRxEnablePromiscuousMode() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Set filter to disable Promiscuous Mode
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxDisablePromiscuousMode(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

}				/* end of nicRxDisablePromiscuousMode() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief this function flushes all packets queued in reordering module
 *
 * @param prAdapter              Pointer to the Adapter structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Flushed successfully
 */
/*----------------------------------------------------------------------------*/
uint32_t nicRxFlush(struct ADAPTER *prAdapter)
{
	struct SW_RFB *prSwRfb;

	ASSERT(prAdapter);
	prSwRfb = qmFlushRxQueues(prAdapter);
	if (prSwRfb != NULL) {
		do {
			struct SW_RFB *prNextSwRfb;

			/* save next first */
			prNextSwRfb = QUEUE_GET_NEXT_ENTRY(prSwRfb);

			/* free */
			nicRxReturnRFB(prAdapter, prSwRfb);

			prSwRfb = prNextSwRfb;
		} while (prSwRfb);
	}

	return WLAN_STATUS_SUCCESS;
}

uint8_t nicIsActionFrameValid(struct SW_RFB *prSwRfb)
{
	struct WLAN_ACTION_FRAME *prActFrame;
	uint16_t u2ActionIndex = 0, u2ExpectedLen = 0;
	uint32_t u4Idx;

	if (prSwRfb->u2PacketLen < sizeof(struct WLAN_ACTION_FRAME))
		return FALSE;
	prActFrame = (struct WLAN_ACTION_FRAME *) prSwRfb->pvHeader;

	DBGLOG(RSN, TRACE, "Action frame category=%d action=%d\n",
	       prActFrame->ucCategory, prActFrame->ucAction);

	u2ActionIndex = prActFrame->ucCategory | prActFrame->ucAction << 8;
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(arActionFrameReservedLen); u4Idx++) {
		if (u2ActionIndex == arActionFrameReservedLen[u4Idx].u2Index) {
			u2ExpectedLen = (uint16_t)
				arActionFrameReservedLen[u4Idx].len;
			DBGLOG(RSN, LOUD,
				"Found expected len of incoming action frame:%d\n",
				u2ExpectedLen);
			break;
		}
	}
	if (u2ExpectedLen != 0 && prSwRfb->u2PacketLen < u2ExpectedLen) {
		DBGLOG(RSN, INFO,
			"Received an abnormal action frame: packet len/expected len:%d/%d\n",
			prSwRfb->u2PacketLen, u2ExpectedLen);
		return FALSE;
	}
	return TRUE;
}

#if CFG_SUPPORT_NAN
uint32_t nicRxNANPMFCheck(struct ADAPTER *prAdapter,
		 struct BSS_INFO *prBssInfo, struct SW_RFB *prSwRfb)
{
	struct _NAN_ACTION_FRAME_T *prActionFrame = NULL;

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error!\n");
		return WLAN_STATUS_FAILURE;
	}

	prActionFrame = (struct _NAN_ACTION_FRAME_T *)prSwRfb->pvHeader;

	if (prAdapter->rWifiVar.fgNoPmf)
		return WLAN_STATUS_SUCCESS;

	if (prBssInfo != NULL) {
		if (prBssInfo->eNetworkType == NETWORK_TYPE_NAN) {
			if (prSwRfb->prStaRec->fgIsTxKeyReady == TRUE) {
				/* NAN Todo: Not HW_MAC_RX_DESC here */
#if (CFG_SUPPORT_CONNAC3X == 1)
				if (
				HAL_MAC_CONNAC3X_RX_STATUS_IS_CIPHER_MISMATCH(
				(struct HW_MAC_CONNAC3X_RX_DESC *)prSwRfb
						->prRxStatus) == TRUE) {
#elif (CFG_SUPPORT_CONNAC2X == 1)
				if (HAL_MAC_CONNAC2X_RX_STATUS_IS_CIPHER_MISMATCH(
					(struct HW_MAC_CONNAC2X_RX_DESC *)prSwRfb
							->prRxStatus) == TRUE) {
#else
				if (HAL_RX_STATUS_IS_CIPHER_MISMATCH(
					(struct HW_MAC_RX_DESC *)prSwRfb
							->prRxStatus) == TRUE) {
#endif
					DBGLOG(NAN, DEBUG,
					       "[PMF] Rx NON-PROTECT NAF, StaIdx:%d, Wtbl:%d\n",
					       prSwRfb->prStaRec->ucIndex,
					       prSwRfb->ucWlanIdx);
					DBGLOG(NAN, DEBUG,
					       "Src=>%02x:%02x:%02x:%02x:%02x:%02x, OUISubtype:%d\n",
					       prActionFrame->aucSrcAddr[0],
					       prActionFrame->aucSrcAddr[1],
					       prActionFrame->aucSrcAddr[2],
					       prActionFrame->aucSrcAddr[3],
					       prActionFrame->aucSrcAddr[4],
					       prActionFrame->aucSrcAddr[5],
					       prActionFrame->ucOUISubtype);
					return WLAN_STATUS_FAILURE;
				}
			}
		}
	}
	return WLAN_STATUS_SUCCESS;
}


static uint32_t (* const nanActionFrameHandler[NAN_ACTION_NUM])
		(struct ADAPTER *, struct SW_RFB *) = {
	[NAN_ACTION_RANGING_REQUEST] = nanRangingRequestRx,
	[NAN_ACTION_RANGING_RESPONSE] = nanRangingResponseRx,
	[NAN_ACTION_RANGING_TERMINATION] = nanRangingTerminationRx,
	[NAN_ACTION_RANGING_REPORT] = nanRangingReportRx,

	[NAN_ACTION_DATA_PATH_REQUEST] = nanNdpProcessDataRequest,
	[NAN_ACTION_DATA_PATH_RESPONSE] = nanNdpProcessDataResponse,
	[NAN_ACTION_DATA_PATH_CONFIRM] = nanNdpProcessDataConfirm,
	[NAN_ACTION_DATA_PATH_KEY_INSTALLMENT] = nanNdpProcessDataKeyInstall,
	[NAN_ACTION_DATA_PATH_TERMINATION] = nanNdpProcessDataTermination,

	[NAN_ACTION_SCHEDULE_REQUEST] = nanNdlProcessScheduleRequest,
	[NAN_ACTION_SCHEDULE_RESPONSE] = nanNdlProcessScheduleResponse,
	[NAN_ACTION_SCHEDULE_CONFIRM] = nanNdlProcessScheduleConfirm,
	[NAN_ACTION_SCHEDULE_UPDATE_NOTIFICATION] =
		nanNdlProcessScheduleUpdateNotification,
	[NAN_ACTION_RESERVED] = nanExtProcessRsvdFrame,

};

uint32_t nicRxProcessNanPubActionFrame(struct ADAPTER *prAdapter,
			      struct SW_RFB *prSwRfb)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_ACTION_FRAME_T *prActionFrame = NULL;
	uint8_t ucOuiType;
	uint8_t ucOuiSubtype;
	struct BSS_INFO *prBssInfo = NULL;

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error!\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(NAN, LOUD, "NAN RX ACTION FRAME PROCESSING\n");
	prActionFrame = (struct _NAN_ACTION_FRAME_T *)prSwRfb->pvHeader;
	if (!IS_WFA_SPECIFIC_OUI(prActionFrame->aucOUI))
		return WLAN_STATUS_INVALID_DATA;

	ucOuiType = prActionFrame->ucOUItype;

	if (prSwRfb->prStaRec && (ucOuiType == VENDOR_OUI_TYPE_NAN_NAF)) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(
			prAdapter, prSwRfb->prStaRec->ucBssIndex);
		if (nicRxNANPMFCheck(prAdapter, prBssInfo, prSwRfb) ==
		    WLAN_STATUS_FAILURE)
			return WLAN_STATUS_FAILURE;
	}

	if (ucOuiType == VENDOR_OUI_TYPE_NAN_NAF ||
	    ucOuiType == VENDOR_OUI_TYPE_NAN_SDF) {
		ucOuiSubtype = prActionFrame->ucOUISubtype;
		DBGLOG(NAN, INFO,
		       "Rx NAN Pub Action, StaIdx:%d, Wtbl:%d, Key:%d, OUISubtype:%d(%s), Src: "
		       MACSTR " Dest: " MACSTR "\n",
		       prSwRfb->ucStaRecIdx, prSwRfb->ucWlanIdx,
		       (prSwRfb->prStaRec ? prSwRfb->prStaRec->fgIsTxKeyReady
					  : 0),
		       ucOuiSubtype,
		       nanActionFrameOuiString(ucOuiSubtype),
		       MAC2STR(prActionFrame->aucSrcAddr),
		       MAC2STR(prActionFrame->aucDestAddr));

		/* NAN_CHK_PNT log message */
		nanLogRx(nanActionFrameOuiString(ucOuiSubtype),
			 prActionFrame->aucSrcAddr);

		/* NAN_CHK_PNT log message */
		if (UNEQUAL_MAC_ADDR(prActionFrame->aucClusterID,
		    nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0)
		    ->aucClusterId)) {
			switch (ucOuiSubtype) {
			case NAN_ACTION_DATA_PATH_REQUEST:
				nanLogFailRxReqStr("cluster_mismatch");
				break;
			case NAN_ACTION_DATA_PATH_RESPONSE:
				nanLogFailRxRespStr("cluster_mismatch");
				break;
			case NAN_ACTION_DATA_PATH_CONFIRM:
				nanLogFailRxConfmStr("cluster_mismatch");
				break;
			case NAN_ACTION_DATA_PATH_KEY_INSTALLMENT:
				nanLogFailRxKeyInstlStr("cluster_mismatch");
				break;
			default:
				break;
			}
		}

		if (ucOuiSubtype < NAN_ACTION_NUM)
			rWlanStatus =
				nanActionFrameHandler[ucOuiSubtype](prAdapter,
								    prSwRfb);
	}
	return rWlanStatus;
}
#endif

static u_int8_t nicIsUnprotectedRobustActionFrame(struct ADAPTER *prAdapter,
						struct SW_RFB *prSwRfb)
{
#if CFG_SUPPORT_802_11W
	u_int8_t fgRobustAction;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo;
	struct WLAN_ACTION_FRAME *prActFrame = prSwRfb->pvHeader;
	struct BSS_INFO *prBssInfo = NULL;

	fgRobustAction = secIsRobustActionFrame(prAdapter, prSwRfb->pvHeader);

	if (!fgRobustAction)
		return FALSE;
	if (!prSwRfb->prStaRec)
		return FALSE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					prSwRfb->prStaRec->ucBssIndex);
	if (prBssInfo && prBssInfo->eNetworkType != NETWORK_TYPE_AIS)
		return FALSE;

	prAisSpecBssInfo = aisGetAisSpecBssInfo(prAdapter,
					prSwRfb->prStaRec->ucBssIndex);

	if (!prAisSpecBssInfo->fgMgmtProtection)
		return FALSE;

	if (prActFrame->u2FrameCtrl & MASK_FC_PROTECTED_FRAME)
		return FALSE;

#if CFG_WIFI_SW_CIPHER_MISMATCH
	if (!prSwRfb->fgIsCipherMS)
#else
	if (prSwRfb->ucSecMode != CIPHER_SUITE_CCMP)
#endif
		return FALSE;

	return TRUE;
#else
	return FALSE;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
uint32_t nicRxProcessActionFrame(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb)
{
	struct WLAN_ACTION_FRAME *prActFrame;
	struct BSS_INFO *prBssInfo = NULL;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	DBGLOG(RSN, TRACE, "[Rx] nicRxProcessActionFrame\n");

	if (!nicIsActionFrameValid(prSwRfb))
		return WLAN_STATUS_INVALID_PACKET;

	prActFrame = (struct WLAN_ACTION_FRAME *)prSwRfb->pvHeader;

	if (nicIsUnprotectedRobustActionFrame(prAdapter, prSwRfb)) {
		DBGLOG(RSN, INFO,
		       "[MFP] Not handle and drop un-protected robust action frame %x %x!!\n",
		       prSwRfb->ucWlanIdx, prSwRfb->ucSecMode);
		return WLAN_STATUS_INVALID_PACKET;
	}

	DBGLOG(RSN, INFO, "[Rx]RobustAction %x %x\n",
		       prSwRfb->ucWlanIdx, prSwRfb->ucSecMode);

	if (prSwRfb->prStaRec)
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prSwRfb->prStaRec->ucBssIndex);

	switch (prActFrame->ucCategory) {
	case CATEGORY_QOS_ACTION:
		DBGLOG(RX, DEBUG, "received dscp action frame: %d\n",
		       __LINE__);
		handleQosMapConf(prAdapter, prSwRfb);
		break;

	case CATEGORY_PUBLIC_ACTION:
		aisFuncValidateRxActionFrame(prAdapter, prSwRfb);
		rlmProcessPublicAction(prAdapter, prSwRfb);
#if CFG_ENABLE_WIFI_DIRECT
		if (prAdapter->fgIsP2PRegistered) {
			if (prBssInfo)
				p2pFuncValidateRxActionFrame(prAdapter, prSwRfb,
					(prBssInfo->ucBssIndex ==
					prAdapter->ucP2PDevBssIdx),
					(uint8_t) prBssInfo->u4PrivateData);
			else
				p2pFuncValidateRxActionFrame(prAdapter,
					prSwRfb, TRUE, 0);
		}
#endif
#if CFG_SUPPORT_NAN
		if (prAdapter->fgIsNANRegistered)
			nicRxProcessNanPubActionFrame(prAdapter, prSwRfb);
#endif

#if CFG_SUPPORT_TDLS_AUTO
		TdlsProcessPublicAction(prAdapter, prSwRfb);
#endif

#if CFG_SUPPORT_RTT_RSTA
		if (rttIsSupport(prAdapter))
			rttProcessPublicAction(prAdapter, prSwRfb);
#endif
		break;

	case CATEGORY_FT_ACTION:
		DBGLOG(RX, DEBUG, "received ft action frame\n");
#if CFG_SUPPORT_ROAMING
		roamingFsmRunEventRxFtAction(prAdapter, prSwRfb);
#endif
		break;

	case CATEGORY_HT_ACTION:
		rlmProcessHtAction(prAdapter, prSwRfb);
		break;

	case CATEGORY_VENDOR_SPECIFIC_PROTECTED_ACTION:
#if CFG_FAST_PATH_SUPPORT
		fpProcessVendorSpecProtectedFrame(prAdapter, prSwRfb);
#endif
		aisFuncValidateRxActionFrame(prAdapter, prSwRfb);
		break;

	case CATEGORY_VENDOR_SPECIFIC_ACTION:
#if CFG_ENABLE_WIFI_DIRECT
		if (prAdapter->fgIsP2PRegistered) {
			if (prBssInfo)
				p2pFuncValidateRxActionFrame(prAdapter, prSwRfb,
					(prBssInfo->ucBssIndex ==
					prAdapter->ucP2PDevBssIdx),
					(uint8_t) prBssInfo->u4PrivateData);
			else
				p2pFuncValidateRxActionFrame(prAdapter,
					prSwRfb, TRUE, 0);
		}
#endif
		if (prBssInfo && prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
#if CFG_SUPPORT_NCHO
			if (prAdapter->rNchoInfo.fgNCHOEnabled == TRUE
			    && prAdapter->rNchoInfo.u4WesMode == TRUE) {
				aisFuncValidateRxActionFrame(prAdapter,
					prSwRfb);
				DBGLOG(INIT, DEBUG,
				       "NCHO CATEGORY_VENDOR_SPECIFIC_ACTION\n");
			}
#endif
			if (prAdapter->ucEnVendorSpecifiedRpt)
				aisFuncValidateRxActionFrame(prAdapter,
					prSwRfb);
		}
		break;

#if CFG_SUPPORT_802_11W
	case CATEGORY_SA_QUERY_ACTION: {
		if (prSwRfb->prStaRec) {
			ASSERT(prBssInfo);
			if ((prBssInfo->eNetworkType == NETWORK_TYPE_AIS) &&
				aisGetAisSpecBssInfo(prAdapter,
				prSwRfb->prStaRec->ucBssIndex)->
			    fgMgmtProtection /* Use MFP */) {
				/* MFP test plan 5.3.3.4 */
				rsnSaQueryAction(prAdapter, prSwRfb);
#if CFG_ENABLE_WIFI_DIRECT
			} else if ((prBssInfo->eNetworkType ==
					NETWORK_TYPE_P2P) &&
				  (prBssInfo->eCurrentOPMode ==
					OP_MODE_ACCESS_POINT)) {
				/* AP PMF */
				DBGLOG(RSN, INFO,
					"[Rx] nicRx AP PMF SAQ action\n");
				if (rsnCheckBipKeyInstalled(prAdapter,
						prSwRfb->prStaRec)) {
					/* MFP test plan 4.3.3.4 */
					rsnApSaQueryAction(prAdapter, prSwRfb);
				}
#endif
			}
		}
	}
	break;
#endif

	case CATEGORY_WNM_ACTION: {
		if (prSwRfb->prStaRec && prBssInfo &&
			prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
			DBGLOG(RX, DEBUG, "WNM action frame: %d\n", __LINE__);
			wnmWNMAction(prAdapter, prSwRfb);
		} else if (prSwRfb->prStaRec && prBssInfo &&
			   prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
			p2pRlmProcessWnmActionFrame(prAdapter, prSwRfb);
		} else {
			DBGLOG(RX, DEBUG,
				"WNM action frame:%d, do nothing!\n", __LINE__);
		}
	}
	break;

#if CFG_SUPPORT_DFS
	case CATEGORY_SPEC_MGT: {
		if (prAdapter->fgEnable5GBand) {
			DBGLOG(RLM, INFO,
			       "[CSA]nicRxProcessActionFrame\n");
			rlmProcessSpecMgtAction(prAdapter, prSwRfb);
		}
	}
	break;
#endif

#if CFG_SUPPORT_802_11AC
	case CATEGORY_VHT_ACTION:
		rlmProcessVhtAction(prAdapter, prSwRfb);
		break;
#endif

#if (CFG_SUPPORT_TWT == 1)
	case CATEGORY_S1G_ACTION:
		twtProcessS1GAction(prAdapter, prSwRfb);
		break;
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	case CATEGORY_PROTECTED_EHT_ACTION:
		switch (prActFrame->ucAction) {
		case TID2LINK_REQUEST:
		case TID2LINK_RESPONSE:
		case TID2LINK_TEARDOWN:
#if (CFG_SUPPORT_802_11BE_T2LM == 1)
			t2lmProcessAction(prAdapter, prSwRfb);
#endif
			break;
		case EPCS_ENABLE_REQUEST:
		case EPCS_ENABLE_RESPONSE:
		case EPCS_TEARDOWN:
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
			epcsProcessAction(prAdapter, prSwRfb);
#endif
			break;
		case ACTION_LR_RESPONSE:
#if CFG_SUPPORT_ML_RECONFIG
			roamingFsmRunEventRxLRAction(prAdapter, prSwRfb);
			u4Status = WLAN_STATUS_PENDING;
#endif /* CFG_SUPPORT_ML_RECONFIG */
			break;
		default:
			break;
		}

		break;
#endif

#if CFG_SUPPORT_802_11K
	case CATEGORY_RM_ACTION:
		switch (prActFrame->ucAction) {
#if CFG_AP_80211K_SUPPORT
		case RM_ACTION_RM_REPORT:
			rlmProcessRadioMeasurementResponse(
				prAdapter, prSwRfb);
			break;
#endif /* CFG_AP_80211K_SUPPORT */

		case RM_ACTION_RM_REQUEST:
#if CFG_SUPPORT_RM_BEACON_REPORT_BY_SUPPLICANT
			/* handle RM beacon request by supplicant */
			if (prSwRfb->prStaRec &&
				IS_BSS_INDEX_AIS(prAdapter,
					prSwRfb->prStaRec->ucBssIndex))
				aisFuncValidateRxActionFrame(prAdapter,
					prSwRfb);
#else
			rrmProcessRadioMeasurementRequest(prAdapter, prSwRfb);
#endif
			break;

		case RM_ACTION_REIGHBOR_RESPONSE:
			rrmProcessNeighborReportResonse(prAdapter, prActFrame,
							prSwRfb);
			break;
		}
		break;
#endif

	case CATEGORY_WME_MGT_NOTIFICATION:
		wmmParseQosAction(prAdapter, prSwRfb);
		break;

	case CATEGORY_PROTECTED_DUAL_OF_PUBLIC_ACTION:
		aisFuncValidateRxActionFrame(prAdapter, prSwRfb);
#if CFG_SUPPORT_NAN
		if (prAdapter->fgIsNANRegistered)
			nicRxProcessNanPubActionFrame(prAdapter, prSwRfb);
#endif
		break;

	case CATEGORY_ROBUST_AV_STREAMING_ACTION:
#if CFG_FAST_PATH_SUPPORT
		mscsProcessRobustAVStreaming(prAdapter, prSwRfb);
#endif
		aisFuncValidateRxActionFrame(prAdapter, prSwRfb);
		break;

	default:
		break;
	}			/* end of switch case */

	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
void nicRxGetAllRcpiValueFromRxv(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	uint8_t *aucRxRCPIAnt)
{
	struct mt66xx_chip_info *prChipInfo;
	uint8_t i = 0, j = 0;

	prChipInfo = prAdapter->chip_info;
	if (prChipInfo->asicRxGetRcpiValueFromRxv)
		for (i = ANTENNA_WF0, j = RCPI_MODE_WF0;
				i < MAX_ANTENNA_NUM; i++, j++)
			aucRxRCPIAnt[i] =
			prChipInfo->asicRxGetRcpiValueFromRxv(j, prSwRfb);
	else
		DBGLOG(RX, ERROR, "No asicRxGetRcpiValueFromRxv!!!\n");
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
uint8_t nicRxGetRcpiValueFromRxv(
	struct ADAPTER *prAdapter,
	uint8_t ucRcpiMode,
	struct SW_RFB *prSwRfb)
{
	struct mt66xx_chip_info *prChipInfo;

	prChipInfo = prAdapter->chip_info;
	if (prChipInfo->asicRxGetRcpiValueFromRxv)
		return prChipInfo->asicRxGetRcpiValueFromRxv(
				ucRcpiMode, prSwRfb);
	else {
		DBGLOG(RX, ERROR, "%s: no asicRxGetRcpiValueFromRxv ??\n",
			__func__);
		return 0;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
uint8_t nicRxGetRxModeValueFromRxv(struct ADAPTER *prAdapter,
				struct SW_RFB *prSwRfb)
{
	struct mt66xx_chip_info *prChipInfo;

	prChipInfo = prAdapter->chip_info;
	if (prChipInfo->asicRxGetRxModeValueFromRxv)
		return prChipInfo->asicRxGetRxModeValueFromRxv(prSwRfb);
	DBGLOG(RX, ERROR, "no asicRxGetRxModeValueFromRxv ??\n");
	return 0xFF;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
int32_t nicRxGetLastRxRssi(struct ADAPTER *prAdapter, char *pcCommand,
				 int i4TotalLen, uint8_t ucWlanIdx)
{
	int32_t i4RSSI0 = 0, i4RSSI1 = 0, i4RSSI2 = 0, i4RSSI3 = 0;
	int32_t i4BytesWritten = 0;
	uint32_t u4RxV3 = 0;
	uint8_t ucStaIdx = 0;
	struct CHIP_DBG_OPS *prChipDbg;

	if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIdx, &ucStaIdx) ==
	    WLAN_STATUS_SUCCESS) {
		u4RxV3 = prAdapter->arStaRec[ucStaIdx].au4RxV[3];
		DBGLOG(REQ, LOUD, "****** RX Vector3 = 0x%08x ******\n",
		       u4RxV3);
	} else {
		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s", "Last RX RSSI", " = NOT SUPPORT");
		return i4BytesWritten;
	}

	if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIdx, &ucStaIdx)
		    != WLAN_STATUS_SUCCESS) {
		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s", "Last RX Rate", " = NOT SUPPORT");
		return i4BytesWritten;
	}

	prChipDbg = prAdapter->chip_info->prDebugOps;

	if (prChipDbg && prChipDbg->show_rx_rssi_info) {
		i4BytesWritten = prChipDbg->show_rx_rssi_info(
				prAdapter,
				pcCommand,
				i4TotalLen,
				ucStaIdx);
		return i4BytesWritten;
	}

	i4RSSI0 = RCPI_TO_dBm((u4RxV3 & RX_VT_RCPI0_MASK) >>
			      RX_VT_RCPI0_OFFSET);
	i4RSSI1 = RCPI_TO_dBm((u4RxV3 & RX_VT_RCPI1_MASK) >>
			      RX_VT_RCPI1_OFFSET);

	if (prAdapter->rWifiVar.ucNSS > 2) {
		i4RSSI2 = RCPI_TO_dBm((u4RxV3 & RX_VT_RCPI2_MASK) >>
				      RX_VT_RCPI2_OFFSET);
		i4RSSI3 = RCPI_TO_dBm((u4RxV3 & RX_VT_RCPI3_MASK) >>
				      RX_VT_RCPI3_OFFSET);

		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "%-20s%s%d %d %d %d\n",
			"Last RX Data RSSI", " = ",
			i4RSSI0, i4RSSI1, i4RSSI2, i4RSSI3);
	} else
		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "%-20s%s%d %d\n",
			"Last RX Data RSSI", " = ", i4RSSI0, i4RSSI1);

	return i4BytesWritten;
}

int32_t nicRxGetDataLastRxRssi(struct ADAPTER *prAdapter,
				int32_t *ai4RSSI, uint8_t ucWlanIdx)
{
	uint32_t u4RxV3 = 0;
	uint8_t ucStaIdx = 0;

	if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIdx, &ucStaIdx) ==
	    WLAN_STATUS_SUCCESS) {
		u4RxV3 = prAdapter->arStaRec[ucStaIdx].au4RxV[3];
		DBGLOG(REQ, LOUD, "****** RX Vector3 = 0x%08x ******\n",
		       u4RxV3);
	} else {
		return -1;
	}

	ai4RSSI[0] = RCPI_TO_dBm((u4RxV3 & RX_VT_RCPI0_MASK) >>
			      RX_VT_RCPI0_OFFSET);
	ai4RSSI[1] = RCPI_TO_dBm((u4RxV3 & RX_VT_RCPI1_MASK) >>
			      RX_VT_RCPI1_OFFSET);

	if (prAdapter->rWifiVar.ucNSS > 2) {
		ai4RSSI[2] = RCPI_TO_dBm((u4RxV3 & RX_VT_RCPI2_MASK) >>
				      RX_VT_RCPI2_OFFSET);
		ai4RSSI[3] = RCPI_TO_dBm((u4RxV3 & RX_VT_RCPI3_MASK) >>
				      RX_VT_RCPI3_OFFSET);
	}
	return 0;
}

int32_t nicRxGetDataLastRxAntRcpi(struct ADAPTER *prAdapter,
				uint8_t ucWlanIdx, uint8_t ucBssIndex)
{
	uint32_t u4RxV3 = 0;
	uint8_t ucStaIdx = 0;

	if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIdx, &ucStaIdx) ==
	    WLAN_STATUS_SUCCESS) {
		u4RxV3 = prAdapter->arStaRec[ucStaIdx].au4RxV[3];
		DBGLOG(REQ, LOUD, "****** RX Vector3 = 0x%08x ******\n",
		       u4RxV3);
	} else {
		return -1;
	}

	prAdapter->aucDataRcpiAnt[ucBssIndex][0] =
		(u4RxV3 & RX_VT_RCPI0_MASK) >> RX_VT_RCPI0_OFFSET;
	prAdapter->aucDataRcpiAnt[ucBssIndex][1] =
		(u4RxV3 & RX_VT_RCPI1_MASK) >> RX_VT_RCPI1_OFFSET;

	return 0;
}

/**
 * Lookup wlan index by matching band index.
 */
uint8_t getWlanIdxByBandAndBssid(struct ADAPTER *prAdapter,
	uint8_t ucHwBandIdx, uint8_t ucHwBssIdx, uint8_t ucWlanIdx)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	return mldGetWlanIdxByBandAndBssid(prAdapter,
		ucHwBandIdx, ucHwBssIdx, ucWlanIdx);
#else
	return ucWlanIdx;
#endif
}

/**
 * HW RX setting MLD_ID
 * if (is_QoS_frame)
 *     if (TID is even):
 *         MLD_ID = primary_MLD_ID
 *     else:
 *         MLD_ID = secondary_MLD_ID
 * else: (management frame goes here, need to distinguish by band)
 *     MLD_ID = primary_MLD_ID
 */
uint8_t getPrimaryWlanIdx(struct ADAPTER *prAdapter,
		uint8_t ucTid, uint8_t ucWlanIdx)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	return mldGetPrimaryWlanIdx(prAdapter, ucWlanIdx);
#else
	return ucWlanIdx;
#endif
}

/**
 * For MLO, it should read prSwRfb->ucHwBandIdx to match the RX link.
 */
static void updateLinkStatsMpduAc(struct ADAPTER *prAdapter,
				struct SW_RFB *prSwRfb)
{
#if CFG_SUPPORT_LLS
	static const uint8_t Tid2LinkStatsAc[] = {
		STATS_LLS_WIFI_AC_BE,
		STATS_LLS_WIFI_AC_BK,
		STATS_LLS_WIFI_AC_BK,
		STATS_LLS_WIFI_AC_BE,
		STATS_LLS_WIFI_AC_VI,
		STATS_LLS_WIFI_AC_VI,
		STATS_LLS_WIFI_AC_VO,
		STATS_LLS_WIFI_AC_VO,
	};
	uint8_t ac = Tid2LinkStatsAc[(uint8_t)(prSwRfb->ucTid & 0x7U)];
	uint8_t ucBssIdx = GLUE_GET_PKT_BSS_IDX(prSwRfb->pvPacket);
	struct BSS_INFO *prBssInfo;

	if (!IS_RX_MPDU_BEGIN(prSwRfb->ucPayloadFormat))
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo)
		return;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (prBssInfo->eHwBandIdx != prSwRfb->ucHwBandIdx) {
		uint8_t ucHwBandIdx = prSwRfb->ucHwBandIdx;
		uint8_t i;

		/* find the BSS by matching the band index */
		/* TODO: performance? */
		for (i = 0; i < MAX_BSSID_NUM; i++) {
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);
			if (prBssInfo &&
			    prBssInfo->eHwBandIdx == ucHwBandIdx)
				break;
		}

		if (!prBssInfo)
			return;
	}
#endif
	prBssInfo->u4RxMpduAc[ac]++;
#endif
}

#if CFG_RFB_TRACK
static const char * const apucRfbTrackStatusStr[RFB_TRACK_STATUS_NUM] = {
	[RFB_TRACK_INIT] = "INIT",
	[RFB_TRACK_UNUSE] = "UNUSE",
	[RFB_TRACK_FREE] = "FREE",
	[RFB_TRACK_HIF] = "HIF",
	[RFB_TRACK_RX] = "RX",
	[RFB_TRACK_MAIN] = "MAIN",
	[RFB_TRACK_FIFO] = "FIFO",
	[RFB_TRACK_NAPI] = "NAPI",
	[RFB_TRACK_MAIN_TO_NAPI] = "MAIN_TO_NAPI",
	[RFB_TRACK_DATA] = "DATA",
	[RFB_TRACK_REORDERING_IN] = "REORDERING_IN",
	[RFB_TRACK_REORDERING_OUT] = "REORDERING_OUT",
	[RFB_TRACK_INDICATED] = "INDICATED",
	[RFB_TRACK_PACKET_SETUP] = "PACKET_SETUP",
	[RFB_TRACK_ADJUST_UNUSE] = "ADJUST_UNUSE",
	[RFB_TRACK_MLO] = "MLO",
	[RFB_TRACK_FW_DROP_SSN] = "FW_DROP_SSN",
#if CFG_QUEUE_RX_IF_CONN_NOT_READY
	[RFB_TRACK_RX_PENDING] = "RX_PENDING",
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */
	[RFB_TRACK_FAIL] = "FAIL",
};

void nicRxRfbTrackInit(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, uint32_t i, uint8_t *fileAndLine)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;
	struct RFB_TRACK *prRfbTrack;

	if (IS_FEATURE_DISABLED(prWifiVar->fgRfbTrackEn))
		return;

	if (i >= CFG_RX_MAX_PKT_NUM) {
		DBGLOG(NIC, ERROR, "Invalid index[%u] file:%s\n",
			i, fileAndLine);
		RFB_TRACK_INC_CNT(prRxCtrl, RFB_TRACK_FAIL);
		return;
	}

	prRfbTrack = &(prRxCtrl->rRfbTrack[i]);
	prRfbTrack->prSwRfb = prSwRfb;
	prRfbTrack->ucTrackState = RFB_TRACK_INIT;
	prRfbTrack->pucFileAndLine = fileAndLine;
	GET_CURRENT_SYSTIME(&prRfbTrack->rTrackTime);
	RFB_TRACK_INC_CNT(prRxCtrl, RFB_TRACK_INIT);

	prSwRfb->u4RfbTrackId = i;

	DBGLOG(NIC, TEMP,
		"prSwRfb[%p:%p] TrackId[%u] State[%s] Line[%s] Time[%u]\n",
		prSwRfb,
		prRfbTrack->prSwRfb, prSwRfb->u4RfbTrackId,
		apucRfbTrackStatusStr[prRfbTrack->ucTrackState],
		prRfbTrack->pucFileAndLine,
		prRfbTrack->rTrackTime);
}

void nicRxRfbTrackUpdate(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, uint8_t ucTrackState,
	uint8_t *fileAndLine)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;
	struct RFB_TRACK *prRfbTrack;

	if (IS_FEATURE_DISABLED(prWifiVar->fgRfbTrackEn))
		return;

	if (ucTrackState >= RFB_TRACK_STATUS_NUM) {
		DBGLOG(NIC, ERROR, "Invalid TrackId[%u] file:%s\n",
			ucTrackState, fileAndLine);
		RFB_TRACK_INC_CNT(prRxCtrl, RFB_TRACK_FAIL);
		return;
	}

	if (prSwRfb->u4RfbTrackId >= CFG_RX_MAX_PKT_NUM) {
		DBGLOG(NIC, ERROR, "Invalid index[%u] file:%s\n",
			prSwRfb->u4RfbTrackId, fileAndLine);
		RFB_TRACK_INC_CNT(prRxCtrl, RFB_TRACK_FAIL);
		return;
	}

	prRfbTrack = &(prRxCtrl->rRfbTrack[prSwRfb->u4RfbTrackId]);
	if (prRfbTrack->prSwRfb != prSwRfb) {
		DBGLOG(NIC, ERROR,
			"TrackId[%d] Invalid pointer[%p,%p] file:%s\n",
			prSwRfb->u4RfbTrackId,
			prRfbTrack->prSwRfb, prSwRfb, fileAndLine);
		RFB_TRACK_INC_CNT(prRxCtrl, RFB_TRACK_FAIL);
		return;
	}

	/* Decrease from original group */
	RFB_TRACK_DEC_CNT(prRxCtrl, prRfbTrack->ucTrackState);
	prRfbTrack->ucTrackState = ucTrackState;
	prRfbTrack->pucFileAndLine = fileAndLine;
	GET_BOOT_SYSTIME(&prRfbTrack->rTrackTime);
	RFB_TRACK_INC_CNT(prRxCtrl, ucTrackState);

	DBGLOG(NIC, TEMP,
		"prSwRfb[%p] TrackId[%u] State[%s] Line[%s] Time[%u]\n",
		prSwRfb,
		prSwRfb->u4RfbTrackId,
		prRfbTrack->ucTrackState >= ARRAY_SIZE(apucRfbTrackStatusStr) ?
			"" : apucRfbTrackStatusStr[prRfbTrack->ucTrackState],
		prRfbTrack->pucFileAndLine,
		prRfbTrack->rTrackTime);
}

void nicRxRfbTrackCheck(struct ADAPTER *prAdapter)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;
	static OS_SYSTIME last;
	OS_SYSTIME now;
	uint32_t i = 0;
	struct RFB_TRACK *prRfbTrack;

	if (IS_FEATURE_DISABLED(prWifiVar->fgRfbTrackEn))
		return;

	GET_BOOT_SYSTIME(&now);

	if (!CHECK_FOR_TIMEOUT(now, last,
		SEC_TO_SYSTIME(prWifiVar->u4RfbTrackInterval)))
		return;

	for (i = 0 ; i < CFG_RX_MAX_PKT_NUM; i++) {
		OS_SYSTIME rTrackTime;

		prRfbTrack = &(prRxCtrl->rRfbTrack[i]);
		rTrackTime = prRfbTrack->rTrackTime;

		/* no need to check rfb in free rfb list */
		if (prRfbTrack->ucTrackState == RFB_TRACK_FREE ||
			prRfbTrack->ucTrackState == RFB_TRACK_UNUSE)
			continue;

		/* rfb track time is change, skip this rfb check */
		if (rTrackTime > now)
			continue;

		if (!CHECK_FOR_TIMEOUT(now, rTrackTime,
			SEC_TO_SYSTIME(prWifiVar->u4RfbTrackTimeout)))
			continue;

		DBGLOG(NIC, DEBUG,
			"prSwRfb[%p] TrackId[%u] State[%s] Line[%s] Time[%u] Diff[%u ms]\n",
			prRfbTrack->prSwRfb,
			i,
			apucRfbTrackStatusStr[prRfbTrack->ucTrackState],
			prRfbTrack->pucFileAndLine,
			rTrackTime,
			now - rTrackTime
			);
	}

	last = now;
}
#endif /* CFG_RFB_TRACK */

#if CFG_DYNAMIC_RFB_ADJUSTMENT
static void nicRxReturnUnUseRFB(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct RX_CTRL *prRxCtrl;

	KAL_SPIN_LOCK_DECLARATION();

	if (!prSwRfb)
		return;

	/* release skb when rfb unuse */
	if (prSwRfb->pvPacket) {
		kalPacketFree(prAdapter->prGlueInfo, prSwRfb->pvPacket);
		prSwRfb->pvPacket = NULL;
	}

	/* enqueue into unuse rfb list */
	prRxCtrl = &prAdapter->rRxCtrl;
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
	QUEUE_INSERT_TAIL(&prRxCtrl->rUnUseRfbList, &prSwRfb->rQueEntry);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

#if CFG_RFB_TRACK
	RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb, RFB_TRACK_UNUSE);
#endif /* CFG_RFB_TRACK */
}

void nicRxAdjustUnUseRFB(struct ADAPTER *prAdapter)
{
	struct RX_CTRL *prRxCtrl;
	struct SW_RFB *prSwRfb;

	KAL_SPIN_LOCK_DECLARATION();

	prRxCtrl = &prAdapter->rRxCtrl;
	if (RX_GET_UNUSE_RFB_CNT(prRxCtrl) == nicRxGetUnUseCnt(prAdapter))
		return;

	if (RX_GET_UNUSE_RFB_CNT(prRxCtrl) < nicRxGetUnUseCnt(prAdapter)) {
		uint32_t u4Cnt[2] = {0};

		/* unuse rfb list is not full */
		/* dequeue indicated rfb list first */
		while (RX_GET_UNUSE_RFB_CNT(prRxCtrl) <
			nicRxGetUnUseCnt(prAdapter)) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
			QUEUE_REMOVE_HEAD(&prRxCtrl->rIndicatedRfbList,
				prSwRfb, struct SW_RFB *);
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

			if (!prSwRfb)
				break;
#if CFG_RFB_TRACK
			RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
				RFB_TRACK_ADJUST_UNUSE);
#endif /* CFG_RFB_TRACK */
			nicRxReturnRFB(prAdapter, prSwRfb);
			u4Cnt[0]++;
		}

		/* dequeue free rfb list */
		while (RX_GET_UNUSE_RFB_CNT(prRxCtrl) <
			nicRxGetUnUseCnt(prAdapter)) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
			QUEUE_REMOVE_HEAD(&prRxCtrl->rFreeSwRfbList,
				prSwRfb, struct SW_RFB *);
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

			if (!prSwRfb)
				break;
#if CFG_RFB_TRACK
			RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
				RFB_TRACK_ADJUST_UNUSE);
#endif /* CFG_RFB_TRACK */
			nicRxReturnRFB(prAdapter, prSwRfb);
			u4Cnt[1]++;
		}

		DBGLOG(NIC, DEBUG,
			"Move rfb[%u,%u] to unuse rfb list.\n",
			u4Cnt[0], u4Cnt[1]);
	} else {
		uint32_t u4Cnt = 0;

		/* unuse rfb list is full, need to dequeue from it */
		while (RX_GET_UNUSE_RFB_CNT(prRxCtrl) >
			nicRxGetUnUseCnt(prAdapter)) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
			QUEUE_REMOVE_HEAD(&prRxCtrl->rUnUseRfbList,
				prSwRfb, struct SW_RFB *);
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

			if (!prSwRfb)
				break;
#if CFG_RFB_TRACK
			RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
				RFB_TRACK_ADJUST_UNUSE);
#endif /* CFG_RFB_TRACK */
			nicRxReturnRFB(prAdapter, prSwRfb);
			u4Cnt++;
		}

		DBGLOG(NIC, DEBUG,
			"Move unuse rfb[%u] to indicated rfb list.\n",
			u4Cnt);

		wlanReturnPacketDelaySetupTimeout(prAdapter, (uintptr_t)NULL);
	}
}

void nicAcquireDynamicRfbLock(struct ADAPTER *prAdapter)
{
#if !CFG_SUPPORT_RX_WORK || CFG_SUPPORT_HIF_RX_NAPI
	if (HAL_IS_TX_DIRECT(prAdapter) || HAL_IS_RX_DIRECT(prAdapter))
		spin_lock_bh(&prAdapter->prGlueInfo->rSpinLock[
				SPIN_LOCK_DYNAMIC_RFB]);
	else
#endif
		KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_DYNAMIC_RFB);
}

void nicReleaseDynamicRfbLock(struct ADAPTER *prAdapter)
{
#if !CFG_SUPPORT_RX_WORK || CFG_SUPPORT_HIF_RX_NAPI
	if (HAL_IS_TX_DIRECT(prAdapter) || HAL_IS_RX_DIRECT(prAdapter))
		spin_unlock_bh(&prAdapter->prGlueInfo->rSpinLock[
				SPIN_LOCK_DYNAMIC_RFB]);
	else
#endif
		KAL_RELEASE_MUTEX(prAdapter, MUTEX_DYNAMIC_RFB);
}

u_int8_t nicRxSetRfbCntByLevel(struct ADAPTER *prAdapter, uint32_t u4Lv)
{
	uint32_t u4RfbCnt;
	u_int8_t fgRet = TRUE;

	if (u4Lv >= PERF_MON_RFB_MAX_THRESHOLD)
		u4Lv = PERF_MON_RFB_MAX_THRESHOLD - 1;

	nicAcquireDynamicRfbLock(prAdapter);

	if (prAdapter->u4RfbUnUseCntLv == u4Lv) {
		fgRet = FALSE;
		goto unlock;
	}

	prAdapter->u4RfbUnUseCntLv = u4Lv;
	u4RfbCnt = prAdapter->rWifiVar.u4RfbUnUseCnt[u4Lv];
	nicRxSetUnUseCnt(prAdapter, u4RfbCnt, TRUE);

	prAdapter->ulUpdateRxRfbCntPeriod = jiffies +
		prAdapter->rWifiVar.u4PerfMonUpdatePeriod * HZ / 1000;
unlock:
	nicReleaseDynamicRfbLock(prAdapter);
	return fgRet;
}

u_int8_t nicRxIncRfbCnt(struct ADAPTER *prAdapter)
{
	uint32_t u4Lv, u4RfbCnt;
	u_int8_t fgRet = TRUE;

	nicAcquireDynamicRfbLock(prAdapter);

	if ((prAdapter->u4RfbUnUseCntLv + 1) == PERF_MON_RFB_MAX_THRESHOLD) {
		fgRet = FALSE;
		goto unlock;
	}

	prAdapter->u4RfbUnUseCntLv++;
	u4Lv = prAdapter->u4RfbUnUseCntLv;
	u4RfbCnt = prAdapter->rWifiVar.u4RfbUnUseCnt[u4Lv];
	nicRxSetUnUseCnt(prAdapter, u4RfbCnt, TRUE);

	prAdapter->ulUpdateRxRfbCntPeriod = jiffies +
		prAdapter->rWifiVar.u4PerfMonUpdatePeriod * HZ / 1000;

unlock:
	nicReleaseDynamicRfbLock(prAdapter);
	return fgRet;
}

u_int8_t nicRxDecRfbCnt(struct ADAPTER *prAdapter)
{
	uint32_t u4Lv, u4RfbCnt;
	u_int8_t fgRet = TRUE;

	nicAcquireDynamicRfbLock(prAdapter);

	if (prAdapter->u4RfbUnUseCntLv == 0) {
		fgRet = FALSE;
		goto unlock;
	}

	if (time_before(jiffies, prAdapter->ulUpdateRxRfbCntPeriod)) {
		fgRet = FALSE;
		goto unlock;
	}

	prAdapter->u4RfbUnUseCntLv--;
	u4Lv = prAdapter->u4RfbUnUseCntLv;
	u4RfbCnt = prAdapter->rWifiVar.u4RfbUnUseCnt[u4Lv];
	nicRxSetUnUseCnt(prAdapter, u4RfbCnt, TRUE);

	prAdapter->ulUpdateRxRfbCntPeriod = jiffies +
		prAdapter->rWifiVar.u4PerfMonUpdatePeriod * HZ / 1000;

unlock:
	nicReleaseDynamicRfbLock(prAdapter);
	return fgRet;
}
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */

uint32_t nicRxGetUnUseCnt(struct ADAPTER *prAdapter)
{
#if CFG_DYNAMIC_RFB_ADJUSTMENT
	return prAdapter->u4RfbUnUseCnt;
#else /* CFG_DYNAMIC_RFB_ADJUSTMENT */
	return 0;
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */
}

void nicRxSetUnUseCnt(struct ADAPTER *prAdapter,
	uint32_t u4UnUseCnt, u_int8_t fgAdjustNow)
{
#if CFG_DYNAMIC_RFB_ADJUSTMENT
	if (prAdapter->u4RfbUnUseCnt == u4UnUseCnt)
		return;

	DBGLOG(NIC, DEBUG, "u4RfbUnUseCnt:[%u->%u]\n",
	       prAdapter->u4RfbUnUseCnt, u4UnUseCnt);
	prAdapter->u4RfbUnUseCnt = u4UnUseCnt;

	if (fgAdjustNow)
		nicRxAdjustUnUseRFB(prAdapter);
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */
}

void nicRxProcessRxvLinkStats(struct ADAPTER *prAdapter,
	struct SW_RFB *prRetSwRfb, uint32_t *pu4RxV)
{
#if CFG_SUPPORT_LLS
	struct CHIP_DBG_OPS *prChipDbg;

	prChipDbg = prAdapter->chip_info->prDebugOps;
	if (prChipDbg && prChipDbg->get_rx_link_stats)
		prChipDbg->get_rx_link_stats(prAdapter, prRetSwRfb, pu4RxV);

	updateLinkStatsMpduAc(prAdapter, prRetSwRfb);
#endif
}

uint16_t nicRxGetFrameControl(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	uint16_t u2FrameCtrl = 0;
	struct RX_DESC_OPS_T *prRxDescOps;
	void *prRxStatus = prSwRfb->prRxStatus;
	struct WLAN_MAC_HEADER *prWlanHeader;
	uint8_t isHdrTans = FALSE;

	if (likely(prAdapter->chip_info && prAdapter->chip_info->prRxDescOps)) {
		prRxDescOps = prAdapter->chip_info->prRxDescOps;
		if (prRxDescOps->nic_rxd_get_HdrTrans)
			isHdrTans =
				prRxDescOps->nic_rxd_get_HdrTrans(prRxStatus);
	}

	if (isHdrTans) {
		if (prSwRfb->prRxStatusGroup4)
			u2FrameCtrl = prSwRfb->prRxStatusGroup4->u2FrameCtl;
		else
			DBGLOG(RX, ERROR, "Header trans w/o Group4\n");
	} else {
		prWlanHeader = prSwRfb->pvHeader;
		u2FrameCtrl = prWlanHeader->u2FrameCtrl;
	}

	return u2FrameCtrl;
}

uint32_t nicRxGetReorderCnt(struct ADAPTER *prAdapter)
{
	uint32_t u4Cnt = 0;
	uint32_t i = 0;

	for (i = 0; i < MAX_BSSID_NUM; i++)
		u4Cnt += REORDERING_GET_BSS_CNT(&prAdapter->rRxCtrl, i);

	return u4Cnt;
}

