// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "p2p_scan.c"
 *    \brief  This file defines the p2p scan profile and
 *      the processing function of scan result for SCAN Module.
 *
 *    The SCAN Profile selection is part of SCAN MODULE and
 *    responsible for defining SCAN Parameters -
 *    e.g. MIN_CHANNEL_TIME, number of scan channels.
 *    In this file we also define the process of SCAN Result
 *    including adding, searching and removing SCAN record from the list.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */

#include "precomp.h"

#if CFG_ENABLE_WIFI_DIRECT
/*******************************************************************************
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

void
scanP2pProcessBeaconAndProbeResp(struct ADAPTER *prAdapter,
		 struct SW_RFB *prSwRfb,
		 uint32_t *prStatus,
		 struct BSS_DESC *prBssDesc,
		 struct WLAN_BEACON_FRAME *prWlanBeaconFrame)
{
	u_int8_t fgIsBeacon = FALSE;
	u_int8_t fgIsSkipThisBeacon = FALSE;
	u_int8_t fgIsP2pNetRegistered = FALSE;
	u_int8_t fgScanSpecificSSID = FALSE;
	void *prScanRequest = NULL;

	/* Sanity check for p2p net device state */
	GLUE_SPIN_LOCK_DECLARATION();
	GLUE_ACQUIRE_SPIN_LOCK(prAdapter->prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prAdapter->fgIsP2PRegistered &&
		prAdapter->rP2PNetRegState == ENUM_NET_REG_STATE_REGISTERED)
		fgIsP2pNetRegistered = TRUE;
	GLUE_RELEASE_SPIN_LOCK(prAdapter->prGlueInfo, SPIN_LOCK_NET_DEV);

	if (!fgIsP2pNetRegistered)
		return;

	/* Indicate network to kernel for P2P interface when:
	 *  1. This is P2P network
	 *  2. Driver is configured to report all bss networks
	 */
	if (!prBssDesc->fgIsP2PPresent &&
		!prAdapter->p2p_scan_report_all_bss)
		return;

	fgIsBeacon = (prWlanBeaconFrame->u2FrameCtrl & MASK_FRAME_TYPE) ==
			MAC_FRAME_BEACON;

	if (prBssDesc->fgIsConnected && fgIsBeacon) {
		uint32_t u4Idx = 0;
		struct BSS_INFO *prP2pBssInfo =
			(struct BSS_INFO *) NULL;

		for (u4Idx = 0; u4Idx < prAdapter->ucSwBssIdNum; u4Idx++) {
			/* Check BSS for P2P. */
			/* Check BSSID. */
			prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					(uint8_t) u4Idx);

			if ((!prP2pBssInfo) || (!IS_BSS_ACTIVE(prP2pBssInfo)))
				continue;

			if ((prP2pBssInfo->eNetworkType != NETWORK_TYPE_P2P) ||
				(UNEQUAL_MAC_ADDR(prP2pBssInfo->aucBSSID,
					prBssDesc->aucBSSID) ||
				(!EQUAL_SSID(prP2pBssInfo->aucSSID,
					prP2pBssInfo->ucSSIDLen,
					prBssDesc->aucSSID,
					prBssDesc->ucSSIDLen)))) {
				continue;
			}
			/* P2P GC */
			/* Connected */
			if (prP2pBssInfo->eCurrentOPMode !=
				OP_MODE_INFRASTRUCTURE ||
			    prP2pBssInfo->eConnectionState !=
				MEDIA_STATE_CONNECTED)
				continue;

			fgIsSkipThisBeacon = TRUE;
			/* First Time. */
			if (!prP2pBssInfo->ucDTIMPeriod) {
				prP2pBssInfo->ucDTIMPeriod =
					prBssDesc->ucDTIMPeriod;
				nicPmIndicateBssConnected(prAdapter,
					prP2pBssInfo->ucBssIndex);
			}

#if (CFG_P2P2_SUPPORT_CAP_NOTIFICATION == 1)
			if (prP2pBssInfo->fgReSyncCap) {
				struct STA_RECORD *prStarec;

				prP2pBssInfo->fgReSyncCap = FALSE;

				prStarec = prP2pBssInfo->prStaRecOfAP;
				p2pRlmReSyncCapAfterCsa(prAdapter,
							prP2pBssInfo,
							prStarec);
			}
#endif /* CFG_P2P2_SUPPORT_CAP_NOTIFICATION */
		}

	}

	/* Skip report beacon to upper layer if no p2p scan. Note that a p2p
	 * device may scan a specific SSID when it tries to join the GO. The
	 * scan requests may be mixed up with wlan's scan requests. In this
	 * case, we still need to report beacon to supplicant. Otherwise,
	 * supplicant may not be able to find WPS IE and result in inviation
	 * fails.
	 */
	prScanRequest = kalGetP2pDevScanReq(prAdapter->prGlueInfo);
	fgScanSpecificSSID =
		kalGetP2pDevScanSpecificSSID(prAdapter->prGlueInfo);
	if (fgIsBeacon && prScanRequest == NULL && !fgScanSpecificSSID) {
		DBGLOG(P2P, TRACE,
			"Skip beacon, p2pScanRequest NULL, scanSpecificSSID NULL\n");
		fgIsSkipThisBeacon = TRUE;
	}

	if (fgIsBeacon && fgIsSkipThisBeacon) {
		/* Only report Probe Response frame
		 * to supplicant except passive scan.
		 */
		/* Probe response collect
		 * much more information.
		 */
		DBGLOG(P2P, TRACE, "Skip beacon [" MACSTR "][%s][ch %d]\n",
				MAC2STR(prWlanBeaconFrame->aucBSSID),
				prBssDesc->aucSSID,
				prBssDesc->ucChannelNum);
		return;
	}

	do {
		struct RF_CHANNEL_INFO rChannelInfo;

		ASSERT_BREAK((prSwRfb != NULL) && (prBssDesc != NULL));

		rChannelInfo.ucChannelNum = prBssDesc->ucChannelNum;
		rChannelInfo.eBand = prBssDesc->eBand;
		prBssDesc->fgIsP2PReport = TRUE;

		DBGLOG(P2P, TRACE,
			"indicate [" MACSTR "][%s][%s][ch %d][r %d][t %u]\n",
			MAC2STR(prWlanBeaconFrame->aucBSSID),
			fgIsBeacon ? "Beacon" : "Probe Response",
			prBssDesc->aucSSID,
			prBssDesc->ucChannelNum,
			prBssDesc->ucRCPI,
			prBssDesc->rUpdateTime);

		DBGLOG_MEM8(P2P, LOUD, prSwRfb->pvHeader,
				prSwRfb->u2PacketLen);

		kalP2PIndicateBssInfo(prAdapter->prGlueInfo,
				(uint8_t *) prSwRfb->pvHeader,
				(uint32_t) prSwRfb->u2PacketLen,
				&rChannelInfo,
				RCPI_TO_dBm(prBssDesc->ucRCPI));

	} while (FALSE);
}

void scnEventReturnChannel(struct ADAPTER *prAdapter,
		uint8_t ucScnSeqNum)
{

	struct CMD_SCAN_CANCEL rCmdScanCancel = {0};

	/* send cancel message to firmware domain */
	rCmdScanCancel.ucSeqNum = ucScnSeqNum;
	rCmdScanCancel.ucIsExtChannel = (uint8_t) FALSE;

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SCAN_CANCEL,
			    TRUE,
			    FALSE, FALSE, NULL, NULL,
			    sizeof(struct CMD_SCAN_CANCEL),
			    (uint8_t *)&rCmdScanCancel, NULL, 0);
}				/* scnEventReturnChannel */

#if (CFG_SUPPORT_802_11BE_MLO == 1)
static u_int8_t scanP2pNeedTriggerMlScan(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prBssDescSet,
	struct BSS_DESC *prBssDesc)
{
	u_int8_t fgIsMlEnable =
		mldIsMultiLinkEnabled(prAdapter, NETWORK_TYPE_P2P, FALSE);

	/* mlo NOT enabled or peer is non-mlo */
	if (!fgIsMlEnable ||
	    prBssDesc->rMlInfo.fgMldType == MLD_TYPE_ICV_METHOD_V1 ||
	    prBssDesc->rMlInfo.fgValid == FALSE ||
	    prBssDesc->rMlInfo.ucMaxSimuLinks <= 1) {
		DBGLOG(P2P, TRACE,
		       "[ML] do not trigger ML scan enable=%u, type=%u, valid=%u, maxSimuLinks=%u\n",
		       fgIsMlEnable,
		       prBssDesc->rMlInfo.fgMldType,
		       prBssDesc->rMlInfo.fgValid,
		       prBssDesc->rMlInfo.ucMaxSimuLinks);
		return FALSE;
	}

	/*
	 * peer is mld with multi links and
	 * peer's multi links scanned including main link
	 */
	if (prBssDescSet->ucLinkNum > 1 &&
	    prBssDesc->rMlInfo.ucLinkId == 0)
		return FALSE;

	return TRUE;
}
#endif

struct BSS_DESC *scanP2pSearchDesc(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		struct P2P_JOIN_INFO *prJoinInfo)
{
	struct BSS_DESC *prCandidateBssDesc = (struct BSS_DESC *) NULL,
		*prBssDesc = (struct BSS_DESC *) NULL;
	struct LINK *prBssDescList = (struct LINK *) NULL;

	do {
		if ((prAdapter == NULL) || (prConnReqInfo == NULL))
			break;

		prBssDescList = &(prAdapter->rWifiVar.rScanInfo.rBSSDescList);

		DBGLOG(P2P, LOUD,
			"Connecting to BSSID: " MACSTR "\n",
			MAC2STR(prConnReqInfo->aucBssid));
		DBGLOG(P2P, LOUD,
			"Connecting to SSID:%s, length:%d\n",
			HIDE(prConnReqInfo->rSsidStruct.aucSsid),
			prConnReqInfo->rSsidStruct.ucSsidLen);

		LINK_FOR_EACH_ENTRY(prBssDesc, prBssDescList,
			rLinkEntry, struct BSS_DESC) {
			DBGLOG(P2P, LOUD,
				"Checking BSS: " MACSTR "\n",
				MAC2STR(prBssDesc->aucBSSID));

			if (prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE) {
				DBGLOG(P2P, LOUD,
					"Ignore mismatch BSS type.\n");
				continue;
			}

			if (UNEQUAL_MAC_ADDR(prBssDesc->aucBSSID,
				prConnReqInfo->aucBssid)) {
				DBGLOG(P2P, LOUD, "Ignore mismatch BSSID.\n");
				continue;
			}

			/* SSID should be the same?
			 * SSID is vary for each connection. so...
			 */
			if (UNEQUAL_SSID(prConnReqInfo->rSsidStruct.aucSsid,
					 prConnReqInfo->rSsidStruct.ucSsidLen,
					 prBssDesc->aucSSID,
					 prBssDesc->ucSSIDLen)) {

				DBGLOG(P2P, TRACE,
					"Connecting to BSSID: " MACSTR "\n",
					MAC2STR(prConnReqInfo->aucBssid));
				DBGLOG(P2P, TRACE,
					"Connecting to SSID:%s, length:%d\n",
					HIDE(
					  prConnReqInfo->rSsidStruct.aucSsid),
					prConnReqInfo->rSsidStruct.ucSsidLen);
				DBGLOG(P2P, TRACE,
					"Checking SSID:%s, length:%d\n",
					HIDE(prBssDesc->aucSSID),
					prBssDesc->ucSSIDLen);
				DBGLOG(P2P, TRACE,
					"Ignore mismatch SSID, (But BSSID match).\n");
				/* ASSERT(FALSE); *//*let p2p re-scan again */
				continue;
			}

			if (!prBssDesc->fgIsP2PPresent) {
				DBGLOG(P2P, ERROR,
					"SSID, BSSID, BSSTYPE match, but no P2P IE present.\n");
#if CFG_P2P_CONNECT_ALL_BSS
				/* Force return */
#else
				continue;
#endif
			}

			/* Final decision. */
			prCandidateBssDesc = prBssDesc;
			break;
		}

	} while (FALSE);

	if (prJoinInfo) {
		struct BSS_DESC_SET *prBssDescSet = &prJoinInfo->rBssDescSet;

		if (prCandidateBssDesc) {
			/* setup primary link */
			prBssDescSet->ucLinkNum = 1;
			prBssDescSet->aprBssDescW[0] =
				&prBssDescSet->arBssDescWPool[0];
			scanFillBssDescW(prBssDescSet->aprBssDescW[0],
				prCandidateBssDesc);
			prBssDescSet->prMainBssDesc = prCandidateBssDesc;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			p2pScanFillSecondaryLink(prAdapter, prBssInfo,
						 prBssDescSet);
			if (prBssDescSet->ucLinkNum > 1)
				prCandidateBssDesc =
					prBssDescSet->prMainBssDesc;

			if (scanP2pNeedTriggerMlScan(prAdapter,
						     prBssDescSet,
						     prCandidateBssDesc)) {
				DBGLOG(P2P, INFO, "Enable ML probe\n");
				prCandidateBssDesc = NULL;
				kalMemZero(prBssDescSet, sizeof(*prBssDescSet));
				prJoinInfo->fgNeedMlScan = TRUE;
			}
#endif
		} else {
			prBssDescSet->ucLinkNum = 0;
			prBssDescSet->prMainBssDesc = NULL;
		}
	}

	return prCandidateBssDesc;
}				/* scanP2pSearchDesc */
#endif /* CFG_ENABLE_WIFI_DIRECT */
