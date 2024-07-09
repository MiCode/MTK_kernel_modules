/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 ** Id: //Department/DaVinci/TRUNK/WiFi_P2P_Driver/mgmt/p2p_fsm.c#61
 */

/*! \file   "p2p_fsm.c"
 *  \brief  This file defines the FSM for P2P Module.
 *
 *  This file defines the FSM for P2P Module.
 */


/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "precomp.h"

#if CFG_ENABLE_WIFI_DIRECT

/******************************************************************************
 *                              C O N S T A N T S
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
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

void p2pFsmRunEventScanRequest(IN struct ADAPTER *prAdapter,
		IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_P2P_SCAN_REQUEST *prP2pScanReqMsg =
		(struct MSG_P2P_SCAN_REQUEST *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));
		if ((prAdapter == NULL) || (prMsgHdr == NULL))
			break;

		prP2pScanReqMsg = (struct MSG_P2P_SCAN_REQUEST *) prMsgHdr;

		prAdapter->prP2pInfo->eConnState = P2P_CNN_NORMAL;

		if (prP2pScanReqMsg->ucBssIdx == prAdapter->ucP2PDevBssIdx)
			p2pDevFsmRunEventScanRequest(prAdapter, prMsgHdr);
		else
			p2pRoleFsmRunEventScanRequest(prAdapter, prMsgHdr);

		prMsgHdr = NULL;
		/* Both p2pDevFsmRunEventScanRequest and
		 * p2pRoleFsmRunEventScanRequest
		 * free prMsgHdr before return,
		 * so prMsgHdr is needed to be NULL.
		 */
	} while (FALSE);

	if (prMsgHdr != NULL)
		cnmMemFree(prAdapter, prMsgHdr);
}				/* p2pDevFsmRunEventScanRequest */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is call when channel is granted
 *             by CNM module from FW.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void p2pFsmRunEventChGrant(IN struct ADAPTER *prAdapter,
		IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_CH_GRANT *prMsgChGrant = (struct MSG_CH_GRANT *) NULL;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
#if (CFG_SUPPORT_AUTO_SCC == 1)
	struct AIS_FSM_INFO *prAisFsmInfo;
#endif

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

		prMsgChGrant = (struct MSG_CH_GRANT *) prMsgHdr;

		prP2pBssInfo =
			GET_BSS_INFO_BY_INDEX(prAdapter,
				prMsgChGrant->ucBssIndex);

		prAdapter->prP2pInfo->eConnState = P2P_CNN_NORMAL;
		prAdapter->prP2pInfo->ucExtendChanFlag = 0;

		DBGLOG(P2P, TRACE, "P2P Run Event Channel Grant\n");

#if CFG_SISO_SW_DEVELOP
		/* Driver record granted CH in BSS info */
		prP2pBssInfo->fgIsGranted = TRUE;
		prP2pBssInfo->eBandGranted = prMsgChGrant->eRfBand;
		prP2pBssInfo->ucPrimaryChannelGranted =
			prMsgChGrant->ucPrimaryChannel;
#endif

		switch (prP2pBssInfo->eCurrentOPMode) {
		case OP_MODE_P2P_DEVICE:
			ASSERT(prP2pBssInfo->ucBssIndex
				== prAdapter->ucP2PDevBssIdx);

			p2pDevFsmRunEventChnlGrant(prAdapter,
				prMsgHdr,
				prAdapter->rWifiVar.prP2pDevFsmInfo);
			break;
		case OP_MODE_INFRASTRUCTURE:
		case OP_MODE_ACCESS_POINT:
			ASSERT(prP2pBssInfo->ucBssIndex
				< prAdapter->ucP2PDevBssIdx);

			p2pRoleFsmRunEventChnlGrant(prAdapter, prMsgHdr,
				P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
					prP2pBssInfo->u4PrivateData));
#if (CFG_SUPPORT_AUTO_SCC == 1)
			/* Keep Going when switch to new channel  */
			prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
			if (!prAisFsmInfo)
				break;
			if (prAisFsmInfo->eCurrentState ==
					AIS_STATE_GO_SYNC_CHANNEL ||
				prAisFsmInfo->ePreviousState ==
					AIS_STATE_GO_SYNC_CHANNEL) {
				/* Update AIS Steps*/
				aisFsmSteps(prAdapter,
					AIS_STATE_GO_SYNC_CH_DONE);
				/* Stop the Go sync channel timer  */
				cnmTimerStopTimer(prAdapter,
					&prAisFsmInfo->rGoSyncChannelTimer);
			}
#endif
			break;
		default:
			ASSERT(FALSE);
			break;
		}
	} while (FALSE);
}				/* p2pFsmRunEventChGrant */

void p2pFsmRunEventNetDeviceRegister(IN struct ADAPTER *prAdapter,
		IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_P2P_NETDEV_REGISTER *prNetDevRegisterMsg =
		(struct MSG_P2P_NETDEV_REGISTER *) NULL;

	DBGLOG(P2P, TRACE, "p2pFsmRunEventNetDeviceRegister\n");

	prNetDevRegisterMsg = (struct MSG_P2P_NETDEV_REGISTER *) prMsgHdr;

	if (prNetDevRegisterMsg->fgIsEnable) {
		p2pSetMode((prNetDevRegisterMsg->ucMode == 1) ? TRUE : FALSE);
		if (p2pLaunch(prAdapter->prGlueInfo))
			ASSERT(prAdapter->fgIsP2PRegistered);
	} else {
		if (prAdapter->fgIsP2PRegistered)
			p2pRemove(prAdapter->prGlueInfo);
	}

	cnmMemFree(prAdapter, prMsgHdr);
}				/* p2pFsmRunEventNetDeviceRegister */

void p2pFsmRunEventUpdateMgmtFrame(IN struct ADAPTER *prAdapter,
		IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_P2P_MGMT_FRAME_UPDATE *prP2pMgmtFrameUpdateMsg =
		(struct MSG_P2P_MGMT_FRAME_UPDATE *) NULL;

	DBGLOG(P2P, TRACE, "p2pFsmRunEventUpdateMgmtFrame\n");

	prP2pMgmtFrameUpdateMsg = (struct MSG_P2P_MGMT_FRAME_UPDATE *) prMsgHdr;

	switch (prP2pMgmtFrameUpdateMsg->eBufferType) {
	case ENUM_FRAME_TYPE_EXTRA_IE_BEACON:
		break;
	case ENUM_FRAME_TYPE_EXTRA_IE_ASSOC_RSP:
		break;
	case ENUM_FRAME_TYPE_EXTRA_IE_PROBE_RSP:
		break;
	case ENUM_FRAME_TYPE_PROBE_RSP_TEMPLATE:
		break;
	case ENUM_FRAME_TYPE_BEACON_TEMPLATE:
		break;
	default:
		break;
	}

	cnmMemFree(prAdapter, prMsgHdr);
}				/* p2pFsmRunEventUpdateMgmtFrame */

#if CFG_SUPPORT_WFD
void p2pFsmRunEventWfdSettingUpdate(IN struct ADAPTER *prAdapter,
		IN struct MSG_HDR *prMsgHdr)
{
	struct WFD_CFG_SETTINGS *prWfdCfgSettings =
		(struct WFD_CFG_SETTINGS *) NULL;
	struct MSG_WFD_CONFIG_SETTINGS_CHANGED *prMsgWfdCfgSettings =
		(struct MSG_WFD_CONFIG_SETTINGS_CHANGED *) NULL;
	uint32_t i;

	/* WLAN_STATUS rStatus =  WLAN_STATUS_SUCCESS; */

	DBGLOG(P2P, INFO, "p2pFsmRunEventWfdSettingUpdate\n");

	do {
		ASSERT_BREAK((prAdapter != NULL));

		if (prMsgHdr != NULL) {
			prMsgWfdCfgSettings =
				(struct MSG_WFD_CONFIG_SETTINGS_CHANGED *)
					prMsgHdr;
			prWfdCfgSettings =
				prMsgWfdCfgSettings->prWfdCfgSettings;
		} else {
			prWfdCfgSettings =
				&prAdapter->rWifiVar.rWfdConfigureSettings;
		}

		DBGLOG(P2P, INFO,
				"WFD Enalbe %x info %x state %x flag %x adv %x\n",
				prWfdCfgSettings->ucWfdEnable,
				prWfdCfgSettings->u2WfdDevInfo,
				(uint32_t) prWfdCfgSettings->u4WfdState,
				(uint32_t) prWfdCfgSettings->u4WfdFlag,
				(uint32_t) prWfdCfgSettings->u4WfdAdvancedFlag);

		if (prWfdCfgSettings->ucWfdEnable == 0)
			for (i = 0; i < KAL_P2P_NUM; i++) {
				if (prAdapter->prGlueInfo->prP2PInfo[i])
					prAdapter->prGlueInfo->prP2PInfo[i]
						->u2WFDIELen = 0;
			}

#if CFG_ENABLE_PER_STA_STATISTICS_LOG
		if (prAdapter->rWifiVar.aprP2pRoleFsmInfo[0]) {
			/* Assume role 0 */
			struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
				(struct P2P_ROLE_FSM_INFO *)
				prAdapter->rWifiVar.aprP2pRoleFsmInfo[0];

			if (prWfdCfgSettings->ucWfdEnable == 1)
				cnmTimerStartTimer(prAdapter,
					&(prP2pRoleFsmInfo
					->rP2pRoleFsmGetStatisticsTimer),
					(3 * P2P_ROLE_GET_STATISTICS_TIME));
			else
				cnmTimerStopTimer(prAdapter,
					&prP2pRoleFsmInfo
					->rP2pRoleFsmGetStatisticsTimer);
		}
#endif

	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);
}

/* p2pFsmRunEventWfdSettingUpdate */

#endif /* CFG_SUPPORT_WFD */


/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is used to handle scan done event
 *             during Device Discovery.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void p2pFsmRunEventScanDone(IN struct ADAPTER *prAdapter,
		IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_SCN_SCAN_DONE *prScanDoneMsg =
		(struct MSG_SCN_SCAN_DONE *) NULL;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;

	prScanDoneMsg = (struct MSG_SCN_SCAN_DONE *) prMsgHdr;

	prP2pBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, prScanDoneMsg->ucBssIndex);

	if (prAdapter->fgIsP2PRegistered == FALSE) {
		DBGLOG(P2P, TRACE,
			"P2P BSS Info is removed, break p2pFsmRunEventScanDone\n");

		cnmMemFree(prAdapter, prMsgHdr);
		return;
	}

	DBGLOG(P2P, TRACE, "P2P Scan Done Event\n");

	switch (prP2pBssInfo->eCurrentOPMode) {
	case OP_MODE_P2P_DEVICE:
		ASSERT(prP2pBssInfo->ucBssIndex == prAdapter->ucP2PDevBssIdx);
		p2pDevFsmRunEventScanDone(prAdapter,
			prMsgHdr,
			prAdapter->rWifiVar.prP2pDevFsmInfo);
		break;
	case OP_MODE_INFRASTRUCTURE:
	case OP_MODE_ACCESS_POINT:
		ASSERT(prP2pBssInfo->ucBssIndex < prAdapter->ucP2PDevBssIdx);
		p2pRoleFsmRunEventScanDone(prAdapter, prMsgHdr,
			P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
				prP2pBssInfo->u4PrivateData));
		break;
	default:
		ASSERT(FALSE);
		break;
	}
}				/* p2pFsmRunEventScanDone */



#endif /* CFG_ENABLE_WIFI_DIRECT */
