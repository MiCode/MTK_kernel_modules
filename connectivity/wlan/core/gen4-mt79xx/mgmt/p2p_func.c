/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */

#include "precomp.h"

struct APPEND_VAR_ATTRI_ENTRY txAssocRspAttributesTable[] = {
	{(P2P_ATTRI_HDR_LEN + P2P_ATTRI_MAX_LEN_STATUS), NULL,
		p2pFuncAppendAttriStatusForAssocRsp}
	/* 0 *//* Status */
	, {(P2P_ATTRI_HDR_LEN + P2P_ATTRI_MAX_LEN_EXT_LISTEN_TIMING), NULL,
		p2pFuncAppendAttriExtListenTiming}	/* 8 */
};

/* XXX: The IEs in the txProbeRspIETable should be in the ID order
 *	ref : standard "Probe Response frame body" table
 */
struct APPEND_VAR_IE_ENTRY txProbeRspIETable[] = {
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_ERP), NULL,
	 rlmRspGenerateErpIE},				/* 42 */
	{(ELEM_HDR_LEN + (RATE_NUM_SW - ELEM_MAX_LEN_SUP_RATES)), NULL,
	 bssGenerateExtSuppRate_IE},			/* 50 */
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_RSN), NULL,
	 rsnGenerateRSNIE},				/* 48 */
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP), NULL,
	 rlmRspGenerateHtCapIE},			/* 45 */
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_HT_OP), NULL,
	 rlmRspGenerateHtOpIE},				/* 61 */
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_OBSS_SCAN), NULL,
	 rlmRspGenerateObssScanIE},			/* 74 */
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP), NULL,
	 rlmRspGenerateExtCapIE},			/* 127 */
#if CFG_SUPPORT_802_11AC
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP), NULL,
	 rlmRspGenerateVhtCapIE},			/* 191 */
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP), NULL,
	 rlmRspGenerateVhtOpIE},			/* 192 */
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP_MODE_NOTIFICATION), NULL,
	 rlmRspGenerateVhtOpNotificationIE},		/* 199 */
#endif
#if CFG_SUPPORT_802_11AX
	{0, heRlmCalculateHeCapIELen,
	 heRlmRspGenerateHeCapIE},			/* 255, EXT 35 */
	{0, heRlmCalculateHeOpIELen,
	 heRlmRspGenerateHeOpIE},			/* 255, EXT 36 */
#endif
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
	 rsnGenerateWpaNoneIE},				/* 221 */
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_WMM_PARAM), NULL,
	 mqmGenerateWmmParamIE},			/* 221 */
#if CFG_SUPPORT_MTK_SYNERGY
	{(ELEM_HDR_LEN + ELEM_MIN_LEN_MTK_OUI), NULL,
	 rlmGenerateMTKOuiIE},				/* 221 */
#endif
	{(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
	 rsnGenerateWPAIE},				/* 221 */
};

#if (CFG_SUPPORT_DFS_MASTER == 1)
u_int8_t g_fgManualCac = FALSE;
uint32_t g_u4DriverCacTime;
uint32_t g_u4CacStartBootTime;
uint8_t g_ucRadarDetectMode = FALSE;
struct P2P_RADAR_INFO g_rP2pRadarInfo;
uint8_t g_ucDfsState = DFS_STATE_INACTIVE;
static uint8_t *apucDfsState[DFS_STATE_NUM] = {
	(uint8_t *) DISP_STRING("DFS_STATE_INACTIVE"),
	(uint8_t *) DISP_STRING("DFS_STATE_CHECKING"),
	(uint8_t *) DISP_STRING("DFS_STATE_ACTIVE"),
	(uint8_t *) DISP_STRING("DFS_STATE_DETECTED")
};

uint8_t *apucW53RadarType[3] = {
	(uint8_t *) DISP_STRING("Unknown Type"),
	(uint8_t *) DISP_STRING("Type 1 (short pulse)"),
	(uint8_t *) DISP_STRING("Type 2 (short pulse)")
};
uint8_t *apucW56RadarType[12] = {
	(uint8_t *) DISP_STRING("Unknown Type"),
	(uint8_t *) DISP_STRING("Type 1 (short pulse)"),
	(uint8_t *) DISP_STRING("Type 2 (short pulse)"),
	(uint8_t *) DISP_STRING("Type 3 (short pulse)"),
	(uint8_t *) DISP_STRING("Type 4 (short pulse)"),
	(uint8_t *) DISP_STRING("Type 5 (short pulse)"),
	(uint8_t *) DISP_STRING("Type 6 (short pulse)"),
	(uint8_t *) DISP_STRING("Type 7 (long pulse)"),
	(uint8_t *) DISP_STRING("Type 8 (short pulse)"),
	(uint8_t *) DISP_STRING("Type 4 or Type 5 or Type 6 (short pulse)"),
	(uint8_t *) DISP_STRING("Type 5 or Type 6 or Type 8 (short pulse)"),
	(uint8_t *) DISP_STRING("Type 5 or Type 6 (short pulse)")
};
#endif

static void
p2pFuncParseBeaconVenderId(IN struct ADAPTER *prAdapter, IN uint8_t *pucIE,
		IN struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo,
		IN uint8_t ucRoleIndex);
#if 0
static void
p2pFuncGetAttriListAction(IN struct ADAPTER *prAdapter,
		IN struct IE_P2P *prIe,
		IN uint8_t ucOuiType,
		OUT uint8_t **pucAttriListStart,
		OUT uint16_t *u2AttriListLen,
		OUT u_int8_t *fgIsAllocMem,
		OUT u_int8_t *fgBackupAttributes,
		OUT uint16_t *u2BufferSize);
#endif

static void
p2pFuncProcessP2pProbeRspAction(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucIEBuf, IN uint8_t ucElemIdType,
		OUT uint8_t *ucBssIdx, OUT struct BSS_INFO **prP2pBssInfo,
		OUT u_int8_t *fgIsWSCIE,
		OUT u_int8_t *fgIsP2PIE,
		OUT u_int8_t *fgIsWFDIE,
		OUT u_int8_t *fgIsVenderIE);

static void
p2pFuncGetSpecAttriAction(IN struct IE_P2P *prP2pIE,
		IN uint8_t ucOuiType,
		IN uint8_t ucAttriID,
		OUT struct P2P_ATTRIBUTE **prTargetAttri);

/*---------------------------------------------------------------------------*/
/*!
 * @brief Function for requesting scan.
 *           There is an option to do ACTIVE or PASSIVE scan.
 *
 * @param eScanType - Specify the scan type of the scan request.
 *                                It can be an ACTIVE/PASSIVE
 *                                  Scan.
 *              eChannelSet - Specify the preferred channel set.
 *                              A FULL scan would request a legacy
 *                              full channel normal scan.(usually ACTIVE).
 *                              A P2P_SOCIAL scan would scan
 *                              1+6+11 channels.(usually ACTIVE)
 *                              A SPECIFIC scan would
 *                              only 1/6/11 channels scan.
 *                              (Passive Listen/Specific Search)
 *               ucChannelNum - A specific channel number.
 *                                       (Only when channel is specified)
 *               eBand - A specific band. (Only when channel is specified)
 *
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void p2pFuncRequestScan(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct P2P_SCAN_REQ_INFO *prScanReqInfo)
{
	struct MSG_SCN_SCAN_REQ_V2 *prScanReqV2 =
		(struct MSG_SCN_SCAN_REQ_V2 *) NULL;

#ifdef CFG_SUPPORT_BEAM_PLUS
	/*NFC Beam + Indication */
	struct P2P_FSM_INFO *prP2pFsmInfo = (struct P2P_FSM_INFO *) NULL;
#endif

	DEBUGFUNC("p2pFuncRequestScan()");

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prScanReqInfo != NULL));

		if (prScanReqInfo->eChannelSet == SCAN_CHANNEL_SPECIFIED) {
			ASSERT_BREAK(prScanReqInfo->ucNumChannelList > 0);
			DBGLOG(P2P, LOUD,
				"P2P Scan Request Channel:%d\n",
				prScanReqInfo->arScanChannelList
					[0].ucChannelNum);
		}

		prScanReqV2 = (struct MSG_SCN_SCAN_REQ_V2 *)
		    cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
				(sizeof(struct MSG_SCN_SCAN_REQ_V2) +
				(sizeof(struct PARAM_SSID) *
				prScanReqInfo->ucSsidNum)));
		if (!prScanReqV2) {
			DBGLOG(P2P, ERROR,
				"p2pFuncRequestScan: Memory allocation fail, can not send SCAN MSG to scan module\n");
			break;
		}

		prScanReqV2->rMsgHdr.eMsgId = MID_P2P_SCN_SCAN_REQ_V2;
		prScanReqV2->ucSeqNum = ++prScanReqInfo->ucSeqNumOfScnMsg;
		prScanReqV2->ucBssIndex = ucBssIndex;
		prScanReqV2->eScanType = prScanReqInfo->eScanType;
		prScanReqV2->eScanChannel = prScanReqInfo->eChannelSet;
		prScanReqV2->u2IELen = 0;
		prScanReqV2->prSsid = (struct PARAM_SSID *)
			((unsigned long) prScanReqV2 +
			sizeof(struct MSG_SCN_SCAN_REQ_V2));

		/* Copy IE for Probe Request. */
		kalMemCopy(prScanReqV2->aucIE,
			prScanReqInfo->aucIEBuf, prScanReqInfo->u4BufLength);
		prScanReqV2->u2IELen = (uint16_t) prScanReqInfo->u4BufLength;

		prScanReqV2->u2ChannelDwellTime =
			prScanReqInfo->u2PassiveDewellTime;
		prScanReqV2->u2ChannelMinDwellTime =
			SCAN_CHANNEL_DWELL_TIME_MIN_MSEC;
		COPY_MAC_ADDR(prScanReqV2->aucBSSID,
			      "\xff\xff\xff\xff\xff\xff");

		prScanReqV2->u2TimeoutValue = 0;
		prScanReqV2->u2ProbeDelay = 0;

		switch (prScanReqInfo->eChannelSet) {
		case SCAN_CHANNEL_SPECIFIED:
			{
				uint32_t u4Idx = 0;
				struct RF_CHANNEL_INFO *prDomainInfo =
				    (struct RF_CHANNEL_INFO *)
				    prScanReqInfo->arScanChannelList;

				if (prScanReqInfo->ucNumChannelList
					> MAXIMUM_OPERATION_CHANNEL_LIST)
					prScanReqInfo->ucNumChannelList =
					MAXIMUM_OPERATION_CHANNEL_LIST;

				for (u4Idx = 0;
					u4Idx < prScanReqInfo->ucNumChannelList;
					u4Idx++) {
					prScanReqV2->arChnlInfoList
						[u4Idx].ucChannelNum =
						prDomainInfo->ucChannelNum;
					prScanReqV2->arChnlInfoList
						[u4Idx].eBand =
						prDomainInfo->eBand;
					prDomainInfo++;
				}

				prScanReqV2->ucChannelListNum =
					prScanReqInfo->ucNumChannelList;
			}
			kal_fallthrough;
		case SCAN_CHANNEL_FULL:
			kal_fallthrough;
		case SCAN_CHANNEL_2G4:
			kal_fallthrough;
		case SCAN_CHANNEL_P2P_SOCIAL:
			{
				/* UINT_8 aucP2pSsid[] = P2P_WILDCARD_SSID; */
				struct PARAM_SSID *prParamSsid =
					(struct PARAM_SSID *) NULL;

				prParamSsid = prScanReqV2->prSsid;

				for (prScanReqV2->ucSSIDNum = 0;
					prScanReqV2->ucSSIDNum
					< prScanReqInfo->ucSsidNum;
					prScanReqV2->ucSSIDNum++) {

					COPY_SSID(prParamSsid->aucSsid,
						prParamSsid->u4SsidLen,
						prScanReqInfo->arSsidStruct
							[prScanReqV2->ucSSIDNum]
							.aucSsid,
						prScanReqInfo->arSsidStruct
							[prScanReqV2->ucSSIDNum]
							.ucSsidLen);

					prParamSsid++;
				}

				/* For compatible. (in FW?) need to check. */
				if (prScanReqV2->ucSSIDNum == 0)
					prScanReqV2->ucSSIDType =
						SCAN_REQ_SSID_P2P_WILDCARD;
				else
					prScanReqV2->ucSSIDType =
						SCAN_REQ_SSID_SPECIFIED;
			}
			break;
		default:
			/* Currently there is no other scan channel set. */
			ASSERT(FALSE);
			break;
		}

		prScanReqInfo->fgIsScanRequest = TRUE;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prScanReqV2,
			MSG_SEND_METHOD_BUF);

	} while (FALSE);
}				/* p2pFuncRequestScan */

void p2pFuncCancelScan(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct P2P_SCAN_REQ_INFO *prScanInfo)
{
	struct MSG_SCN_SCAN_CANCEL *prScanCancelMsg =
		(struct MSG_SCN_SCAN_CANCEL *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prScanInfo != NULL));

		if (!prScanInfo->fgIsScanRequest)
			break;

		DBGLOG(P2P, TRACE, "P2P Cancel Scan\n");

		prScanCancelMsg = (struct MSG_SCN_SCAN_CANCEL *)
			cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_SCN_SCAN_CANCEL));
		if (!prScanCancelMsg) {
			/* Buffer not enough,
			 * can not cancel scan request.
			 */
			DBGLOG(P2P, TRACE,
				"Buffer not enough, can not cancel scan.\n");
			break;
		}
		kalMemZero(prScanCancelMsg,
			sizeof(struct MSG_SCN_SCAN_CANCEL));

		prScanCancelMsg->rMsgHdr.eMsgId =
			MID_P2P_SCN_SCAN_CANCEL;
		prScanCancelMsg->ucBssIndex = ucBssIndex;
		prScanCancelMsg->ucSeqNum =
			prScanInfo->ucSeqNumOfScnMsg++;
		prScanCancelMsg->fgIsChannelExt = FALSE;
		prScanInfo->fgIsScanRequest = FALSE;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prScanCancelMsg,
			MSG_SEND_METHOD_BUF);
	} while (FALSE);
}				/* p2pFuncCancelScan */

void p2pFuncGCJoin(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN struct P2P_JOIN_INFO *prP2pJoinInfo)
{
	struct MSG_SAA_FSM_START *prJoinReqMsg =
		(struct MSG_SAA_FSM_START *) NULL;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;
	struct BSS_DESC *prBssDesc = (struct BSS_DESC *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prP2pBssInfo != NULL)
			&& (prP2pJoinInfo != NULL));

		prBssDesc = prP2pJoinInfo->prTargetBssDesc;
		if ((prBssDesc) == NULL) {
			DBGLOG(P2P, ERROR,
				"p2pFuncGCJoin: NO Target BSS Descriptor\n");
			ASSERT(FALSE);
			break;
		}
#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
		if (prAdapter->rWifiVar.ucDbdcP2pLisEn) {
			/* Prevent wrong p2p conn nss during
			 * DBDC sw case enable -> disable.
			 * If conn happened in WAIT_HW_DISABLE state
			 * Nss could be set to 1, because op mode change
			 * is done after DBDC disable sw done.
			 */
			cnmOpModeGetTRxNss(
				prAdapter, prP2pBssInfo->ucBssIndex,
				&prP2pBssInfo->ucOpRxNss, 
				&prP2pBssInfo->ucOpTxNss);
		}
#endif
		if (prBssDesc->ucSSIDLen) {
			COPY_SSID(prP2pBssInfo->aucSSID,
				prP2pBssInfo->ucSSIDLen,
				prBssDesc->aucSSID,
				prBssDesc->ucSSIDLen);
		}

		/* 2 <1> We are goin to connect to this BSS */
		prBssDesc->fgIsConnecting = TRUE;

		/* 2 <2> Setup corresponding STA_RECORD_T */
		prStaRec = bssCreateStaRecFromBssDesc(prAdapter,
#if CFG_P2P_CONNECT_ALL_BSS
			(STA_TYPE_P2P_GO),
#else
			(prBssDesc->fgIsP2PPresent
			? (STA_TYPE_P2P_GO)
			: (STA_TYPE_LEGACY_AP)),
#endif
			prP2pBssInfo->ucBssIndex, prBssDesc);

		if (prStaRec == NULL) {
			DBGLOG(P2P, TRACE, "Create station record fail\n");
			ASSERT(FALSE);
			break;
		}

		prP2pJoinInfo->prTargetStaRec = prStaRec;
		prP2pJoinInfo->fgIsJoinComplete = FALSE;
		prP2pJoinInfo->u4BufLength = 0;

		/* 2 <2.1> Sync. to FW domain */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

		if (prP2pBssInfo->eConnectionState
			== MEDIA_STATE_DISCONNECTED) {
			prStaRec->fgIsReAssoc = FALSE;
			prP2pJoinInfo->ucAvailableAuthTypes =
				(uint8_t) AUTH_TYPE_OPEN_SYSTEM;
			prStaRec->ucTxAuthAssocRetryLimit =
				TX_AUTH_ASSOCI_RETRY_LIMIT;
		} else {
			DBGLOG(P2P, ERROR,
				"JOIN INIT: Join Request when connected.\n");
			break;
		}

		/* 2 <4> Use an appropriate Authentication Algorithm Number
		 * among the ucAvailableAuthTypes.
		 */
		if (prP2pJoinInfo->ucAvailableAuthTypes
			& (uint8_t) AUTH_TYPE_OPEN_SYSTEM) {

			DBGLOG(P2P, TRACE,
				"JOIN INIT: Try to do Authentication with AuthType == OPEN_SYSTEM.\n");

			prP2pJoinInfo->ucAvailableAuthTypes &=
				~(uint8_t) AUTH_TYPE_OPEN_SYSTEM;

			prStaRec->ucAuthAlgNum =
				(uint8_t) AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
		} else {
			DBGLOG(P2P, ERROR,
				"JOIN INIT: ucAvailableAuthTypes Error.\n");
			ASSERT(FALSE);
			break;
		}

		/* 4 <5> Overwrite Connection Setting
		 * for eConnectionPolicy == ANY (Used by Assoc Req)
		 */

		/* 2 <5> Backup desired channel. */

		/* 2 <6> Send a Msg to trigger SAA to start JOIN process. */
		prJoinReqMsg = (struct MSG_SAA_FSM_START *)
			cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, sizeof(struct MSG_SAA_FSM_START));

		if (!prJoinReqMsg) {
			DBGLOG(P2P, TRACE, "Allocation Join Message Fail\n");
			return;
		}

		prJoinReqMsg->rMsgHdr.eMsgId = MID_P2P_SAA_FSM_START;
		prJoinReqMsg->ucSeqNum = ++prP2pJoinInfo->ucSeqNumOfReqMsg;
		prJoinReqMsg->prStaRec = prStaRec;

		/* TODO: Consider fragmentation info in station record. */

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prJoinReqMsg,
			MSG_SEND_METHOD_BUF);

	} while (FALSE);

}				/* p2pFuncGCJoin */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will update the contain of BSS_INFO_T
 *        for AIS network once the association was completed.
 *
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 * @param[in] prAssocRspSwRfb        Pointer to SW RFB of ASSOC RESP FRAME.
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void
p2pFuncUpdateBssInfoForJOIN(IN struct ADAPTER *prAdapter,
		IN struct BSS_DESC *prBssDesc,
		IN struct STA_RECORD *prStaRec,
		IN struct BSS_INFO *prP2pBssInfo,
		IN struct SW_RFB *prAssocRspSwRfb)
{
	struct WLAN_ASSOC_RSP_FRAME *prAssocRspFrame =
		(struct WLAN_ASSOC_RSP_FRAME *) NULL;
	uint16_t u2IELength;
	uint8_t *pucIE;

	DEBUGFUNC("p2pUpdateBssInfoForJOIN()");

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prStaRec != NULL)
			&& (prP2pBssInfo != NULL)
			&& (prAssocRspSwRfb != NULL));

		prAssocRspFrame = (struct WLAN_ASSOC_RSP_FRAME *)
			prAssocRspSwRfb->pvHeader;

		if (prBssDesc == NULL) {
			/* Target BSS NULL. */
			DBGLOG(P2P, TRACE, "Target BSS NULL\n");
			break;
		}

		DBGLOG(P2P, INFO,
			"Update P2P_BSS_INFO_T and apply settings to MAC\n");

		/* 3 <1> Update BSS_INFO_T from AIS_FSM_INFO_T
		 * or User Settings
		 */
		/* 4 <1.1> Setup Operation Mode */
		ASSERT_BREAK(prP2pBssInfo->eCurrentOPMode
			== OP_MODE_INFRASTRUCTURE);

		/* 4 <1.2> Setup SSID */
		COPY_SSID(prP2pBssInfo->aucSSID,
			prP2pBssInfo->ucSSIDLen,
			prBssDesc->aucSSID,
			prBssDesc->ucSSIDLen);

		/* 4 <1.3> Setup Channel, Band */
		prP2pBssInfo->ucPrimaryChannel = prBssDesc->ucChannelNum;
		prP2pBssInfo->eBand = prBssDesc->eBand;

		/* 3 <2> Update BSS_INFO_T from STA_RECORD_T */
		/* 4 <2.1> Save current AP's STA_RECORD_T and current AID */
		prP2pBssInfo->prStaRecOfAP = prStaRec;
		prP2pBssInfo->u2AssocId = prStaRec->u2AssocId;

		/* 4 <2.2> Setup Capability */
		/* Use AP's Cap Info as BSS Cap Info */
		prP2pBssInfo->u2CapInfo = prStaRec->u2CapInfo;

		if (prP2pBssInfo->u2CapInfo & CAP_INFO_SHORT_PREAMBLE)
			prP2pBssInfo->fgIsShortPreambleAllowed = TRUE;
		else
			prP2pBssInfo->fgIsShortPreambleAllowed = FALSE;

		/* 4 <2.3> Setup PHY Attributes and
		 * Basic Rate Set/Operational Rate Set
		 */
		prP2pBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

		prP2pBssInfo->ucNonHTBasicPhyType =
			prStaRec->ucNonHTBasicPhyType;

		prP2pBssInfo->u2OperationalRateSet =
			prStaRec->u2OperationalRateSet;
		prP2pBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

		nicTxUpdateBssDefaultRate(prP2pBssInfo);

		/* 3 <3> Update BSS_INFO_T from SW_RFB_T
		 * (Association Resp Frame)
		 */
		/* 4 <3.1> Setup BSSID */
		if (UNEQUAL_MAC_ADDR(prBssDesc->aucBSSID,
				prAssocRspFrame->aucBSSID)) {
			DBGLOG(P2P, WARN, "Assoc BSSID " MACSTR "\n",
				MAC2STR(prAssocRspFrame->aucBSSID));
			COPY_MAC_ADDR(prP2pBssInfo->aucBSSID,
				prBssDesc->aucBSSID);
		} else
			COPY_MAC_ADDR(prP2pBssInfo->aucBSSID,
				prAssocRspFrame->aucBSSID);

		u2IELength =
		    (uint16_t) ((prAssocRspSwRfb->u2PacketLen -
		    prAssocRspSwRfb->u2HeaderLen) -
			(OFFSET_OF(struct WLAN_ASSOC_RSP_FRAME,
			aucInfoElem[0]) -
			WLAN_MAC_MGMT_HEADER_LEN));

		pucIE = prAssocRspFrame->aucInfoElem;

		/* 4 <3.2> Parse WMM and setup QBSS flag */
		/* Parse WMM related IEs and configure HW CRs accordingly */
		mqmProcessAssocRsp(prAdapter,
			prAssocRspSwRfb, pucIE, u2IELength);

		prP2pBssInfo->fgIsQBSS = prStaRec->fgIsQoS;

		/* 3 <4> Update BSS_INFO_T from BSS_DESC_T */

		prBssDesc->fgIsConnecting = FALSE;
		prBssDesc->fgIsConnected = TRUE;

		/* 4 <4.1> Setup MIB for current BSS */
		prP2pBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;
		/* NOTE: Defer ucDTIMPeriod updating to
		 * when beacon is received after connection
		 */
		prP2pBssInfo->ucDTIMPeriod = 0;
		prP2pBssInfo->u2ATIMWindow = 0;

		prP2pBssInfo->ucBeaconTimeoutCount =
			AIS_BEACON_TIMEOUT_COUNT_INFRA;

		/* 4 <4.2> Update HT information and set channel */
		/* Record HT related parameters in rStaRec and rBssInfo
		 * Note: it shall be called before nicUpdateBss()
		 */
		rlmProcessAssocRsp(prAdapter,
			prAssocRspSwRfb, pucIE, u2IELength);

		/* 4 <4.3> Sync with firmware for BSS-INFO */
		nicUpdateBss(prAdapter, prP2pBssInfo->ucBssIndex);

		/* 4 <4.4> *DEFER OPERATION*
		 * nicPmIndicateBssConnected() will be invoked
		 * inside scanProcessBeaconAndProbeResp()
		 * after 1st beacon is received
		 */

	} while (FALSE);
}				/* end of p2pUpdateBssInfoForJOIN() */

uint32_t
p2pFunMgmtFrameTxDone(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo,
		IN enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	u_int8_t fgIsSuccess = FALSE;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

		if (rTxDoneStatus != TX_RESULT_SUCCESS) {
			DBGLOG(P2P, TRACE,
				"Mgmt Frame TX Fail, Status:%d.\n",
				rTxDoneStatus);
		} else {
			fgIsSuccess = TRUE;
			DBGLOG(P2P, TRACE, "Mgmt Frame TX Done.\n");
		}

		kalP2PIndicateMgmtTxStatus(prAdapter->prGlueInfo,
			prMsduInfo, fgIsSuccess);

	} while (FALSE);

	return WLAN_STATUS_SUCCESS;

}				/* p2pFunMgmtFrameTxDone */

const char *
p2pActionFrameToString(enum ENUM_P2P_ACTION_FRAME_TYPE eP2pAction)
{
	switch (eP2pAction) {
	case P2P_GO_NEG_REQ:
		return "GO_NEG_REQ";
	case P2P_GO_NEG_RESP:
		return "GO_NEG_RESP";
	case P2P_GO_NEG_CONF:
		return "GO_NEG_CONF";
	case P2P_INVITATION_REQ:
		return "INVITATION_REQ";
	case P2P_INVITATION_RESP:
		return "INVITATION_RESP";
	case P2P_DEV_DISC_REQ:
		return "DEV_DISC_REQ";
	case P2P_DEV_DISC_RESP:
		return "DEV_DISC_RESP";
	case P2P_PROV_DISC_REQ:
		return "PROV_DISC_REQ";
	case P2P_PROV_DISC_RESP:
		return "PROV_DISC_RESP";
	}

	return "UNKNOWN P2P Public Action";
}

const char *
paToString(int32_t u4PaAction)
{
	switch (u4PaAction) {
	case WLAN_PA_20_40_BSS_COEX:
		return "PA_20_40_BSS_COEX";
	case WLAN_PA_VENDOR_SPECIFIC:
		return "PA_VENDOR_SPECIFIC";
	case WLAN_PA_GAS_INITIAL_REQ:
		return "PA_GAS_INITIAL_REQ";
	case WLAN_PA_GAS_INITIAL_RESP:
		return "PA_GAS_INITIAL_RESP";
	case WLAN_PA_GAS_COMEBACK_REQ:
		return "PA_GAS_COMEBACK_REQ";
	case WLAN_PA_GAS_COMEBACK_RESP:
		return "PA_GAS_COMEBACK_RESP";
	case WLAN_TDLS_DISCOVERY_RESPONSE:
		return "TDLS_DISCOVERY_RESPONSE";
	}

	return "UNKNOWN Public Action";
}


const char *
actionToString(int32_t u4WlanAction)
{
	switch (u4WlanAction) {
	case WLAN_ACTION_SPECTRUM_MGMT:
		return "SPECTRUM_MGMT";
	case WLAN_ACTION_QOS:
		return "QOS";
	case WLAN_ACTION_DLS:
		return "DLS";
	case WLAN_ACTION_BLOCK_ACK:
		return "BLOCK_ACK";
	case WLAN_ACTION_PUBLIC:
		return "PUBLIC";
	case WLAN_ACTION_RADIO_MEASUREMENT:
		return "RADIO_MEASUREMENT";
	case WLAN_ACTION_FT:
		return "FT";
	case WLAN_ACTION_HT:
		return "HT";
	case WLAN_ACTION_SA_QUERY:
		return "SA_QUERY";
	case WLAN_ACTION_PROTECTED_DUAL:
		return "PROTECTED_DUAL";
	case WLAN_ACTION_WNM:
		return "WNM";
	case WLAN_ACTION_UNPROTECTED_WNM:
		return "UNPROTECTED_WNM";
	case WLAN_ACTION_TDLS:
		return "TDLS";
	case WLAN_ACTION_SELF_PROTECTED:
		return "SELF_PROTECTED";
	case WLAN_ACTION_WMM:
		return "WMM";
	case WLAN_ACTION_VENDOR_SPECIFIC:
		return "VENDOR_SPECIFIC";
	}

	return "UNKNOWN Action Frame";
}


enum ENUM_P2P_CONNECT_STATE
p2pFuncTagActionActionP2PFrame(IN struct MSDU_INFO *prMgmtTxMsdu,
		IN struct WLAN_ACTION_FRAME *prActFrame,
		IN uint8_t ucP2pAction, IN uint64_t u8Cookie)
{
	DBGLOG(P2P, INFO,
		"Found P2P_%s, SA: " MACSTR
			" - DA: " MACSTR ", cookie: 0x%llx, SeqNO: %d\n",
		p2pActionFrameToString(ucP2pAction),
		MAC2STR(prActFrame->aucSrcAddr),
		MAC2STR(prActFrame->aucDestAddr),
		u8Cookie,
		prMgmtTxMsdu->ucTxSeqNum);
	return ucP2pAction + 1;
}

enum ENUM_P2P_CONNECT_STATE
p2pFuncTagActionActionFrame(IN struct MSDU_INFO *prMgmtTxMsdu,
		IN struct WLAN_ACTION_FRAME *prActFrame,
		IN uint8_t ucAction, IN uint64_t u8Cookie)
{
	uint8_t *pucVendor = NULL;
	enum ENUM_P2P_CONNECT_STATE eCNNState = P2P_CNN_NORMAL;

	DBGLOG(P2P, INFO,
		"Found WLAN_%s, SA: " MACSTR
			" - DA: " MACSTR ", cookie: 0x%llx, SeqNo: %d\n",
		paToString(ucAction),
		MAC2STR(prActFrame->aucSrcAddr),
		MAC2STR(prActFrame->aucDestAddr),
		u8Cookie,
		prMgmtTxMsdu->ucTxSeqNum);

	if (ucAction != WLAN_PA_VENDOR_SPECIFIC)
		return P2P_CNN_NORMAL;

	pucVendor = (uint8_t *)prActFrame + 26;
	if (*(pucVendor + 0) == 0x50 &&
	    *(pucVendor + 1) == 0x6f &&
	    *(pucVendor + 2) == 0x9a) {
		if (*(pucVendor + 3) == 0x09)
			/* found p2p IE */
			eCNNState = p2pFuncTagActionActionP2PFrame(prMgmtTxMsdu,
				prActFrame, *(pucVendor + 4), u8Cookie);
		else if (*(pucVendor + 3) == 0x0a)
			/* found WFD IE */
			DBGLOG(P2P, INFO, "Found WFD IE, SA: " MACSTR
					" - DA: " MACSTR "\n",
				MAC2STR(prActFrame->aucSrcAddr),
				MAC2STR(prActFrame->aucDestAddr));
		else
			DBGLOG(P2P, INFO,
				"Found Other vendor 0x%x, SA: " MACSTR
					" - DA: " MACSTR "\n",
				*(pucVendor + 3),
				MAC2STR(prActFrame->aucSrcAddr),
				MAC2STR(prActFrame->aucDestAddr));
	}
	return eCNNState;
}

enum ENUM_P2P_CONNECT_STATE
p2pFuncTagActionCategoryFrame(IN struct MSDU_INFO *prMgmtTxMsdu,
		struct WLAN_ACTION_FRAME *prActFrame,
		IN uint8_t ucCategory,
		IN uint64_t u8Cookie)
{
	uint8_t ucAction = 0;
	enum ENUM_P2P_CONNECT_STATE eCNNState = P2P_CNN_NORMAL;

	DBGLOG(P2P, TRACE,
		"Found WLAN_ACTION_%s, SA: " MACSTR
			" BSSID: " MACSTR
			" DA: " MACSTR ", u8Cookie: 0x%llx, SeqNO: %d\n",
		actionToString(ucCategory),
		MAC2STR(prActFrame->aucSrcAddr),
		MAC2STR(prActFrame->aucBSSID),
		MAC2STR(prActFrame->aucDestAddr),
		u8Cookie,
		prMgmtTxMsdu->ucTxSeqNum);

	if (ucCategory == WLAN_ACTION_PUBLIC) {
		ucAction = prActFrame->ucAction;
		eCNNState = p2pFuncTagActionActionFrame(prMgmtTxMsdu,
			prActFrame, ucAction, u8Cookie);
	}
	return eCNNState;
}

void p2pProcessActionResponse(IN struct ADAPTER *prAdapter,
		enum ENUM_P2P_ACTION_FRAME_TYPE eType)
{
	u_int8_t fgIdle = FALSE;

	if (!prAdapter || !prAdapter->prP2pInfo)
		return;

	switch (prAdapter->prP2pInfo->eConnState) {
	case P2P_CNN_GO_NEG_REQ:
		if (eType == P2P_GO_NEG_RESP)
			fgIdle = TRUE;
		break;
	case P2P_CNN_GO_NEG_RESP:
		if (eType == P2P_GO_NEG_CONF || eType == P2P_GO_NEG_REQ)
			fgIdle = TRUE;
		break;
	case P2P_CNN_INVITATION_REQ:
		if (eType == P2P_INVITATION_RESP)
			fgIdle = TRUE;
		break;
	case P2P_CNN_DEV_DISC_REQ:
		if (eType == P2P_DEV_DISC_RESP)
			fgIdle = TRUE;
		break;
	case P2P_CNN_PROV_DISC_REQ:
		if (eType == P2P_PROV_DISC_RESP)
			fgIdle = TRUE;
		break;
	default:
		break;
	}

	DBGLOG(P2P, INFO, "eConnState: %d, eType: %d\n",
			prAdapter->prP2pInfo->eConnState, eType);

	if (fgIdle)
		prAdapter->prP2pInfo->eConnState = P2P_CNN_NORMAL;
}

/*
 * used to debug p2p mgmt frame:
 * GO Nego Req
 * GO Nego Res
 * GO Nego Confirm
 * GO Invite Req
 * GO Invite Res
 * Device Discoverability Req
 * Device Discoverability Res
 * Provision Discovery Req
 * Provision Discovery Res
 */
enum ENUM_P2P_CONNECT_STATE
p2pFuncTagMgmtFrame(IN struct MSDU_INFO *prMgmtTxMsdu,
		IN uint64_t u8Cookie)
{
	/* P_MSDU_INFO_T prTxMsduInfo = (P_MSDU_INFO_T)NULL; */
	struct WLAN_MAC_HEADER *prWlanHdr = (struct WLAN_MAC_HEADER *) NULL;
	struct WLAN_BEACON_FRAME *prProbRspHdr =
		(struct WLAN_BEACON_FRAME *)NULL;
	uint16_t u2TxFrameCtrl;
	struct WLAN_ACTION_FRAME *prActFrame;
	uint8_t ucCategory;
	enum ENUM_P2P_CONNECT_STATE eCNNState = P2P_CNN_NORMAL;

	prWlanHdr = (struct WLAN_MAC_HEADER *)
		((unsigned long) prMgmtTxMsdu->prPacket +
		MAC_TX_RESERVED_FIELD);
	/*
	 * mgmt frame MASK_FC_TYPE = 0
	 * use MASK_FRAME_TYPE is oK for frame type/subtype judge
	 */
	u2TxFrameCtrl = prWlanHdr->u2FrameCtrl & MASK_FRAME_TYPE;

	switch (u2TxFrameCtrl) {
	case MAC_FRAME_PROBE_RSP:
		prProbRspHdr = (struct WLAN_BEACON_FRAME *) prWlanHdr;
		DBGLOG(P2P, INFO,
			"TX Probe Response, SA: " MACSTR
				" BSSID: " MACSTR
				" DA: " MACSTR ", cookie: 0x%llx, seqNo: %d\n",
			MAC2STR(prProbRspHdr->aucSrcAddr),
			MAC2STR(prProbRspHdr->aucBSSID),
			MAC2STR(prProbRspHdr->aucDestAddr),
			u8Cookie,
			prMgmtTxMsdu->ucTxSeqNum);

		break;

	case MAC_FRAME_ACTION:
		prActFrame = (struct WLAN_ACTION_FRAME *)prWlanHdr;
		ucCategory = prActFrame->ucCategory;
		eCNNState = p2pFuncTagActionCategoryFrame(prMgmtTxMsdu,
			prActFrame, ucCategory, u8Cookie);

		break;
	default:
		DBGLOG(P2P, INFO,
			"Untagged frame type: 0x%x, A1: " MACSTR
				", A2: " MACSTR
				", A3: " MACSTR " seqNo: %d\n",
			u2TxFrameCtrl,
			MAC2STR(prWlanHdr->aucAddr1),
			MAC2STR(prWlanHdr->aucAddr2),
			MAC2STR(prWlanHdr->aucAddr3),
			prMgmtTxMsdu->ucTxSeqNum);
		break;
	}
	return eCNNState;
}

uint32_t
p2pFuncTxMgmtFrame(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct MSDU_INFO *prMgmtTxMsdu,
		IN u_int8_t fgNonCckRate)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	/* P_MSDU_INFO_T prTxMsduInfo = (P_MSDU_INFO_T)NULL; */
	struct WLAN_MAC_HEADER *prWlanHdr = (struct WLAN_MAC_HEADER *) NULL;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;
	uint8_t ucRetryLimit = 30;	/* TX_DESC_TX_COUNT_NO_LIMIT; */
	u_int8_t fgDrop = FALSE;
	struct BSS_INFO *prBssInfo;
	uint64_t *pu8GlCookie = (uint64_t *) NULL;
	uint64_t u8GlCookie;
	enum ENUM_P2P_CONNECT_STATE eConnState = P2P_CNN_NORMAL;

	do {
		ASSERT_BREAK(prAdapter != NULL);

		/* Drop this frame if BSS inactive */
		if (!IS_NET_ACTIVE(prAdapter, ucBssIndex)) {
			p2pDevFsmRunEventMgmtFrameTxDone(prAdapter,
				prMgmtTxMsdu, TX_RESULT_INACTIVE_BSS);
			cnmMgtPktFree(prAdapter, prMgmtTxMsdu);
			break;
		}
		pu8GlCookie =
			(uint64_t *) ((unsigned long) prMgmtTxMsdu->prPacket +
				(unsigned long) prMgmtTxMsdu->u2FrameLength +
				MAC_TX_RESERVED_FIELD);

		u8GlCookie = *pu8GlCookie;

		prWlanHdr = (struct WLAN_MAC_HEADER *)
			((unsigned long) prMgmtTxMsdu->prPacket +
			MAC_TX_RESERVED_FIELD);
		prStaRec = cnmGetStaRecByAddress(prAdapter,
			ucBssIndex, prWlanHdr->aucAddr1);
		/* prMgmtTxMsdu->ucBssIndex = ucBssIndex; */

		switch (prWlanHdr->u2FrameCtrl & MASK_FRAME_TYPE) {
		case MAC_FRAME_PROBE_RSP:
			DBGLOG(P2P, TRACE, "TX Probe Resposne Frame\n");
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
				ucBssIndex);
			if ((!nicTxIsMgmtResourceEnough(prAdapter))
				|| (prBssInfo->fgIsNetAbsent)) {
				DBGLOG(P2P, INFO,
					"Drop Tx probe response due to resource issue\n");
				fgDrop = TRUE;

				break;
			}
			prMgmtTxMsdu->ucStaRecIndex =
			    (prStaRec != NULL)
			    ? (prStaRec->ucIndex) : (STA_REC_INDEX_NOT_FOUND);
			/* Modifiy Lie time to 100 mS due
			 * to the STA only wait 30-50mS
			 */
			/* and AP do not need send it after STA left */
			nicTxSetPktLifeTime(prMgmtTxMsdu, 100);
			prMgmtTxMsdu = p2pFuncProcessP2pProbeRsp(prAdapter,
				ucBssIndex, prMgmtTxMsdu);

			/*
			 * Not check prMsduInfo sanity
			 * as p2pFuncProcessP2pProbeRsp will always
			 * return a MsduInfo
			 */
			pu8GlCookie =
				(uint64_t *) ((unsigned long)
					prMgmtTxMsdu->prPacket +
					(unsigned long)
					prMgmtTxMsdu->u2FrameLength +
					MAC_TX_RESERVED_FIELD);
			/* Restore cookie as it will be corrupted
			 * in p2pFuncProcessP2pProbeRsp
			 */
			*pu8GlCookie = u8GlCookie;
			ucRetryLimit = 6;
			break;
		default:
			prMgmtTxMsdu->ucBssIndex = ucBssIndex;
			break;
		}

		if (fgDrop) {
			/* Drop this frame */
			p2pDevFsmRunEventMgmtFrameTxDone(prAdapter,
				prMgmtTxMsdu, TX_RESULT_DROPPED_IN_DRIVER);
			cnmMgtPktFree(prAdapter, prMgmtTxMsdu);

			break;
		}

		TX_SET_MMPDU(prAdapter,
			prMgmtTxMsdu,
			prMgmtTxMsdu->ucBssIndex,
			(prStaRec != NULL)
			? (prStaRec->ucIndex) : (STA_REC_INDEX_NOT_FOUND),
			WLAN_MAC_MGMT_HEADER_LEN,
			prMgmtTxMsdu->u2FrameLength,
			p2pDevFsmRunEventMgmtFrameTxDone,
			MSDU_RATE_MODE_AUTO);

		nicTxSetPktRetryLimit(prMgmtTxMsdu, ucRetryLimit);

		eConnState = p2pFuncTagMgmtFrame(prMgmtTxMsdu, u8GlCookie);

		if (p2pFuncNeedWaitRsp(prAdapter,
				prAdapter->prP2pInfo->eConnState))
			prAdapter->prP2pInfo->eConnState = eConnState;

		/* Bufferable MMPDUs are suggested to be queued */
		/* when GC is sleeping according to SPEC, */
		/* instead of being sent to ALTX Q. */

		/* GO discoverability REQ needs to be sent to GC */
		/* when GC is awake due to P2P-6.1.10 cert fail */

		if (!p2pFuncIsBufferableMMPDU(prAdapter,
				eConnState, prMgmtTxMsdu))
			nicTxConfigPktControlFlag(prMgmtTxMsdu,
				MSDU_CONTROL_FLAG_FORCE_TX, TRUE);

		nicTxEnqueueMsdu(prAdapter, prMgmtTxMsdu);


	} while (FALSE);

	return rWlanStatus;
}				/* p2pFuncTxMgmtFrame */

void p2pFuncStopComplete(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL));

		DBGLOG(P2P, TRACE, "p2pFuncStopComplete\n");

		/* GO: It would stop Beacon TX.
		 * GC: Stop all BSS related PS function.
		 */
		nicPmIndicateBssAbort(prAdapter, prP2pBssInfo->ucBssIndex);
		/* Reset RLM related field of BSSINFO. */
		rlmBssAborted(prAdapter, prP2pBssInfo);

		UNSET_NET_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex);
		nicDeactivateNetwork(prAdapter, prP2pBssInfo->ucBssIndex);
		/* Release CNM channel */
		nicUpdateBss(prAdapter, prP2pBssInfo->ucBssIndex);

		/* Reset current OPMode */
		prP2pBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;

		/* Point StaRecOfAP to NULL when GC role stop Complete */
		prP2pBssInfo->prStaRecOfAP = NULL;

		kalP2pNotifyStopApComplete(prAdapter,
				prP2pBssInfo->u4PrivateData);
	} while (FALSE);

}				/* p2pFuncStopComplete */

void
p2pFuncParseWmmParam(struct AC_QUE_PARMS *prACQueParms,
		uint8_t ucAC,
		uint32_t u4Param)
{
	uint8_t ucEn =
		(u4Param & BIT(0));
	uint8_t ucAifs =
		(u4Param & BITS(4, 7)) >> 4;
	uint8_t ucCWmin =
		(u4Param & BITS(8, 11)) >> 8;
	uint8_t ucCWmax =
		(u4Param & BITS(12, 15)) >> 12;
	uint16_t u2Txop =
		(u4Param & BITS(16, 31)) >> 16;

	if (!ucEn)
		return;

	prACQueParms[ucAC].u2Aifsn = ucAifs;
	prACQueParms[ucAC].u2CWmin = (1 << ucCWmin) - 1;
	prACQueParms[ucAC].u2CWmax = (1 << ucCWmax) - 1;
	prACQueParms[ucAC].u2TxopLimit = u2Txop;

	DBGLOG(WMM, TRACE,
		"AC:%d AIFS:0x%X, CWmin:0x%X, CWmax;0x%X, TXOP:0x%X\n",
		ucAC,
		prACQueParms[ucAC].u2Aifsn,
		prACQueParms[ucAC].u2CWmin,
		prACQueParms[ucAC].u2CWmax,
		prACQueParms[ucAC].u2TxopLimit);
}

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will start a P2P Group Owner and send Beacon Frames.
 *
 * @param (none)
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void
p2pFuncStartGO(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo,
		IN struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo,
		IN struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo)
{
#if (CFG_SUPPORT_DFS_MASTER == 1)
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;
#endif

#ifdef CFG_SUPPORT_P2P_OPEN_SECURITY
	uint8_t fgIsOpenP2P = TRUE;
#else
	uint8_t fgIsOpenP2P = FALSE;
#endif

#ifdef CFG_SUPPORT_P2P_GO_11B_RATE
	uint8_t fgIs11bRate = TRUE;
#else
	uint8_t fgIs11bRate = FALSE;
#endif

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prBssInfo != NULL));

		if (prBssInfo->ucBssIndex >= prAdapter->ucHwBssIdNum) {
			DBGLOG(P2P, ERROR,
				"P2P BSS exceed the number of P2P interface number.");
			ASSERT(FALSE);
			break;
		}

		DBGLOG(P2P, TRACE, "p2pFuncStartGO:\n");

#if CFG_AP_80211KVR_INTERFACE
#if CFG_SUPPORT_TRAFFIC_REPORT && CFG_WIFI_SUPPORT_NOISE_HISTOGRAM
		INIT_DELAYED_WORK(
			&(prAdapter->prGlueInfo->rChanNoiseControlWork),
			aaaMulAPAgentChanNoiseInitWorkHandler);
		INIT_DELAYED_WORK(
			&(prAdapter->prGlueInfo->rChanNoiseGetInfoWork),
			aaaMulAPAgentChanNoiseCollectionWorkHandler);
#endif
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
		prCmdRddOnOffCtrl = (struct CMD_RDD_ON_OFF_CTRL *)
			cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			sizeof(*prCmdRddOnOffCtrl));

		if (!prCmdRddOnOffCtrl) {
			break;
		}

		prCmdRddOnOffCtrl->ucDfsCtrl = RDD_START_TXQ;

		/*
		 * FIX ME: Mobile driver can't get correct band.
		 * There is only 5G in DFS channel,
		 * which is on band_0. So it assigned to ENUM_BAND_0
		 * as temp solution.
		 * Remember to fix it when driver could get
		 * the correct band from firmware.
		 */
		prCmdRddOnOffCtrl->ucRddIdx = ENUM_BAND_0;

		DBGLOG(P2P, INFO,
			"p2pFuncStartGO: Start TXQ - DFS ctrl: %.d\n",
			prCmdRddOnOffCtrl->ucDfsCtrl);

		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_RDD_ON_OFF_CTRL,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(*prCmdRddOnOffCtrl),
			(uint8_t *) prCmdRddOnOffCtrl, NULL, 0);

		cnmMemFree(prAdapter, prCmdRddOnOffCtrl);
#endif

		/* Re-start AP mode.  */
		p2pFuncSwitchOPMode(prAdapter,
			prBssInfo, prBssInfo->eIntendOPMode, FALSE);

		prBssInfo->eIntendOPMode = OP_MODE_NUM;

		/* 4 <1.1> Assign SSID */
		COPY_SSID(prBssInfo->aucSSID,
			prBssInfo->ucSSIDLen,
			prP2pConnReqInfo->rSsidStruct.aucSsid,
			prP2pConnReqInfo->rSsidStruct.ucSsidLen);

		DBGLOG(P2P, TRACE, "GO SSID:%s\n", prBssInfo->aucSSID);

		/* 4 <1.2> Clear current AP's STA_RECORD_T and current AID */
		prBssInfo->prStaRecOfAP = (struct STA_RECORD *) NULL;
		prBssInfo->u2AssocId = 0;

		/* 4 <1.3> Setup Channel, Band and Phy Attributes */
		prBssInfo->ucPrimaryChannel = prP2pChnlReqInfo->ucReqChnlNum;
		prBssInfo->eBand = prP2pChnlReqInfo->eBand;
		prBssInfo->eBssSCO = prP2pChnlReqInfo->eChnlSco;

		DBGLOG(P2P, TRACE,
			"GO Channel:%d\n",
			prBssInfo->ucPrimaryChannel);

		if (prBssInfo->eBand == BAND_5G) {
			/* Depend on eBand */
			prBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11A;
			/* Depend on eCurrentOPMode and ucPhyTypeSet */
			prBssInfo->ucConfigAdHocAPMode = AP_MODE_11A;
		} else if ((prP2pConnReqInfo->eConnRequest
			== P2P_CONNECTION_TYPE_PURE_AP) || fgIs11bRate) {
			prBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11BG;
			/* Depend on eCurrentOPMode and ucPhyTypeSet */
			prBssInfo->ucConfigAdHocAPMode = AP_MODE_MIXED_11BG;
		} else {
			ASSERT(prP2pConnReqInfo->eConnRequest
				== P2P_CONNECTION_TYPE_GO);
			prBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11G;
			/* Depend on eCurrentOPMode and ucPhyTypeSet */
			prBssInfo->ucConfigAdHocAPMode = AP_MODE_11G_P2P;
		}

		/* Overwrite BSS PHY type set by Feature Options */
		bssDetermineApBssInfoPhyTypeSet(prAdapter,
			(prP2pConnReqInfo->eConnRequest ==
			P2P_CONNECTION_TYPE_PURE_AP) ? TRUE : FALSE, prBssInfo);

		DBGLOG(P2P, TRACE, "Phy type: 0x%x\n", prBssInfo->ucPhyTypeSet);

		prBssInfo->ucNonHTBasicPhyType = (uint8_t)
			rNonHTApModeAttributes
				[prBssInfo->ucConfigAdHocAPMode]
					.ePhyTypeIndex;
		prBssInfo->u2BSSBasicRateSet =
			rNonHTApModeAttributes
				[prBssInfo->ucConfigAdHocAPMode]
					.u2BSSBasicRateSet;
		prBssInfo->u2OperationalRateSet =
			rNonHTPhyAttributes
				[prBssInfo->ucNonHTBasicPhyType]
					.u2SupportedRateSet;

		if (prBssInfo->ucAllSupportedRatesLen == 0) {
			rateGetDataRatesFromRateSet(
				prBssInfo->u2OperationalRateSet,
			    prBssInfo->u2BSSBasicRateSet,
			    prBssInfo->aucAllSupportedRates,
			    &prBssInfo->ucAllSupportedRatesLen);
		}
		/* 4 <1.5> Setup MIB for current BSS */
		prBssInfo->u2ATIMWindow = 0;
		prBssInfo->ucBeaconTimeoutCount = 0;

		/* 3 <2> Update BSS_INFO_T common part */
#if CFG_SUPPORT_AAA
		prBssInfo->fgIsProtection = FALSE;
		if ((prP2pConnReqInfo->eConnRequest == P2P_CONNECTION_TYPE_GO)
			&& (!fgIsOpenP2P)) {
			/* Always enable protection at P2P GO */
			prBssInfo->fgIsProtection = TRUE;
		} else {
			if (!fgIsOpenP2P)
				ASSERT(prP2pConnReqInfo->eConnRequest
					== P2P_CONNECTION_TYPE_PURE_AP);
			if (kalP2PGetCipher(prAdapter->prGlueInfo,
				(uint8_t) prBssInfo->u4PrivateData))
				prBssInfo->fgIsProtection = TRUE;
		}

		bssInitForAP(prAdapter, prBssInfo, TRUE);

#if 0
		if (prBssInfo->ucBMCWlanIndex >= WTBL_SIZE) {
			prBssInfo->ucBMCWlanIndex =
			    secPrivacySeekForBcEntry(prAdapter,
					prBssInfo->ucBssIndex,
					prBssInfo->aucBSSID, 0xff,
					CIPHER_SUITE_NONE, 0xff);
		}
#endif
		p2pFuncParseWmmParam(prBssInfo->arACQueParms, 0,
			prAdapter->rWifiVar.u4P2pGoWmmParamAC0);
		p2pFuncParseWmmParam(prBssInfo->arACQueParms, 1,
			prAdapter->rWifiVar.u4P2pGoWmmParamAC1);
		p2pFuncParseWmmParam(prBssInfo->arACQueParms, 2,
			prAdapter->rWifiVar.u4P2pGoWmmParamAC2);
		p2pFuncParseWmmParam(prBssInfo->arACQueParms, 3,
			prAdapter->rWifiVar.u4P2pGoWmmParamAC3);

		nicQmUpdateWmmParms(prAdapter, prBssInfo->ucBssIndex);
#endif /* CFG_SUPPORT_AAA */

		/* 3 <3> Set MAC HW */
		/* 4 <3.1> Setup channel and bandwidth */
		rlmBssInitForAPandIbss(prAdapter, prBssInfo);

		/* 4 <3.2> Reset HW TSF Update Mode and Beacon Mode */
		nicUpdateBss(prAdapter, prBssInfo->ucBssIndex);

		/* 4 <3.3> Update Beacon again
		 * for network phy type confirmed.
		 */
		bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);

		/* 4 <3.4> Setup BSSID */
		nicPmIndicateBssCreated(prAdapter, prBssInfo->ucBssIndex);
#if CFG_AP_80211KVR_INTERFACE
		/* 7. BSS status notification */
		p2pFunMulAPAgentBssStatusNotification(prAdapter,
			prBssInfo);
#endif /* CFG_AP_80211KVR_INTERFACE */

	} while (FALSE);
}				/* p2pFuncStartGO() */

void p2pFuncStopGO(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo)
{
	uint32_t u4ClientCount = 0;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL));

		DBGLOG(P2P, TRACE, "p2pFuncStopGO\n");

#if CFG_AP_80211KVR_INTERFACE
		cancel_delayed_work_sync(
			&prAdapter->prGlueInfo->rChanNoiseControlWork);
		cancel_delayed_work_sync(
			&prAdapter->prGlueInfo->rChanNoiseGetInfoWork);
#endif
		u4ClientCount = bssGetClientCount(prAdapter, prP2pBssInfo);

		if ((prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
		    && (prP2pBssInfo->eIntendOPMode == OP_MODE_NUM)) {
			/* AP is created, Beacon Updated. */
			p2pFuncDissolve(prAdapter,
				prP2pBssInfo, TRUE,
				REASON_CODE_DEAUTH_LEAVING_BSS);
			prP2pBssInfo->eIntendOPMode = OP_MODE_P2P_DEVICE;
		}

		/* Do not Deactivate Network if any Client existed,
		 * we'll deactive it after Deauth Tx done
		 */
		if (u4ClientCount == 0) {
			DBGLOG(P2P, INFO,
				"No client! Deactive GO immediately.\n");
			p2pChangeMediaState(prAdapter,
				prP2pBssInfo, MEDIA_STATE_DISCONNECTED);
			p2pFuncStopComplete(prAdapter, prP2pBssInfo);
		}

	} while (FALSE);

}				/* p2pFuncStopGO */

uint32_t p2pFuncRoleToBssIdx(IN struct ADAPTER *prAdapter,
		IN uint8_t ucRoleIdx, OUT uint8_t *pucBssIdx)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (pucBssIdx != NULL));

		if (ucRoleIdx >= BSS_P2P_NUM) {
			rWlanStatus = WLAN_STATUS_FAILURE;
			break;
		}
		if (!prAdapter->rWifiVar.aprP2pRoleFsmInfo[ucRoleIdx]) {
			DBGLOG(P2P, WARN,
				"%s, invalid aprP2pRoleFsmInfo, ignore\n",
				__func__);
			rWlanStatus = WLAN_STATUS_FAILURE;
		} else
			*pucBssIdx = prAdapter->rWifiVar
				.aprP2pRoleFsmInfo[ucRoleIdx]->ucBssIndex;

	} while (FALSE);

	return rWlanStatus;
}				/* p2pFuncRoleToBssIdx */

struct P2P_ROLE_FSM_INFO *p2pFuncGetRoleByBssIdx(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex)
{
	int32_t i = 0;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *)NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL));

		for (i = 0 ; i < BSS_P2P_NUM; i++) {
			if (!prAdapter->rWifiVar.aprP2pRoleFsmInfo[i])
				continue;

			if (prAdapter->rWifiVar.aprP2pRoleFsmInfo[i]->ucBssIndex
				== ucBssIndex)
				break;
		}
		if (i < BSS_P2P_NUM)
			prP2pRoleFsmInfo =
				prAdapter->rWifiVar.aprP2pRoleFsmInfo[i];

	} while (FALSE);

	return prP2pRoleFsmInfo;
}


void
p2pFuncSwitchOPMode(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN enum ENUM_OP_MODE eOpMode,
		IN u_int8_t fgSyncToFW)
{
	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prP2pBssInfo != NULL)
			&& (eOpMode < OP_MODE_NUM));

		if (prP2pBssInfo->eCurrentOPMode != eOpMode) {
			DBGLOG(P2P, TRACE,
				"p2pFuncSwitchOPMode: Switch to from %d, to %d.\n",
				prP2pBssInfo->eCurrentOPMode, eOpMode);

			switch (prP2pBssInfo->eCurrentOPMode) {
			case OP_MODE_ACCESS_POINT:
				/* p2pFuncDissolve will be done
				 * in p2pFuncStopGO().
				 */
				/* p2pFuncDissolve(prAdapter,
				 * prP2pBssInfo, TRUE,
				 * REASON_CODE_DEAUTH_LEAVING_BSS);
				 */
				if (prP2pBssInfo->eIntendOPMode
					!= OP_MODE_P2P_DEVICE) {
					p2pFuncStopGO(prAdapter, prP2pBssInfo);

					SET_NET_PWR_STATE_IDLE(prAdapter,
						prP2pBssInfo->ucBssIndex);
				}
				break;
			default:
				break;
			}

			prP2pBssInfo->eIntendOPMode = eOpMode;

			/* The state is in disconnecting and
			 * can not change any BSS status
			 */
			if (IS_NET_PWR_STATE_IDLE(prAdapter,
				prP2pBssInfo->ucBssIndex) &&
				IS_NET_ACTIVE(prAdapter,
				prP2pBssInfo->ucBssIndex)) {
				DBGLOG(P2P, TRACE,
					"under deauth procedure, Quit.\n");
				break;
			}

			prP2pBssInfo->eCurrentOPMode = eOpMode;
			switch (eOpMode) {
			case OP_MODE_INFRASTRUCTURE:
				DBGLOG(P2P, TRACE,
					"p2pFuncSwitchOPMode: Switch to Client.\n");
				kal_fallthrough;
			case OP_MODE_ACCESS_POINT:
				/* Change interface address. */
				if (eOpMode == OP_MODE_ACCESS_POINT) {
					DBGLOG(P2P, TRACE,
						"p2pFuncSwitchOPMode: Switch to AP.\n");
					prP2pBssInfo->ucSSIDLen = 0;
				}

#if CFG_DUAL_P2PLIKE_INTERFACE
				/*avoid ap1 Bss  have diff A2 & A3, */
				/*ToDo :  fix for P2P case*/

#else
				COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr,
					prAdapter->rWifiVar
						.aucInterfaceAddress[
						prP2pBssInfo->u4PrivateData]);
				COPY_MAC_ADDR(prP2pBssInfo->aucBSSID,
					prAdapter->rWifiVar
						.aucInterfaceAddress[
						prP2pBssInfo->u4PrivateData]);
#endif
				break;
			case OP_MODE_P2P_DEVICE:
				{
					/* Change device address. */
					DBGLOG(P2P, TRACE,
						"p2pFuncSwitchOPMode: Switch back to P2P Device.\n");

					p2pChangeMediaState(prAdapter,
						prP2pBssInfo,
						MEDIA_STATE_DISCONNECTED);

					COPY_MAC_ADDR(
						prP2pBssInfo->aucOwnMacAddr,
						prAdapter->rWifiVar
							.aucDeviceAddress);
					COPY_MAC_ADDR(
						prP2pBssInfo->aucBSSID,
						prAdapter->rWifiVar
							.aucDeviceAddress);

				}
				break;
			default:
				ASSERT(FALSE);
				break;
			}

			if (1) {
				struct P2P_DISCONNECT_INFO rP2PDisInfo;
				memset(&rP2PDisInfo, 0,
					sizeof(struct P2P_DISCONNECT_INFO));

				rP2PDisInfo.ucRole = 2;
				wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_P2P_ABORT,
				    TRUE,
				    FALSE,
				    FALSE,
				    NULL,
				    NULL,
				    sizeof(struct P2P_DISCONNECT_INFO),
				    (uint8_t *) &rP2PDisInfo, NULL, 0);
			}

			DBGLOG(P2P, TRACE,
				"The device address is changed to " MACSTR "\n",
				MAC2STR(prP2pBssInfo->aucOwnMacAddr));
			DBGLOG(P2P, TRACE,
				"The BSSID is changed to " MACSTR "\n",
				MAC2STR(prP2pBssInfo->aucBSSID));

			/* Update BSS INFO to FW. */
			if ((fgSyncToFW) && (eOpMode != OP_MODE_ACCESS_POINT))
				nicUpdateBss(prAdapter,
					prP2pBssInfo->ucBssIndex);
		} else if (prP2pBssInfo->eCurrentOPMode == eOpMode &&
				eOpMode == OP_MODE_INFRASTRUCTURE) {
			/*
			 * Sometimes the interface is changed from P2P_CLIENT
			 * to STATION, but GC's connection flow is still in
			 * processing. We must force stop previous connection
			 * request to avoid unexpected behavior.
			 */
			struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
					(struct P2P_ROLE_FSM_INFO *) NULL;
			struct P2P_CONNECTION_REQ_INFO *prConnReqInfo =
					(struct P2P_CONNECTION_REQ_INFO *) NULL;

			prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
					prAdapter, prP2pBssInfo->u4PrivateData);
			if (prP2pRoleFsmInfo == NULL)
				break;

			prConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
			if (prConnReqInfo == NULL)
				break;

			if (prConnReqInfo->eConnRequest ==
					P2P_CONNECTION_TYPE_GC) {
				log_dbg(P2P, INFO, "Force stop connection request since mode switch.\n");
				prConnReqInfo->eConnRequest =
						P2P_CONNECTION_TYPE_IDLE;
				p2pRoleFsmRunEventAbort(prAdapter,
						prP2pRoleFsmInfo);
			}
		}

	} while (FALSE);
}				/* p2pFuncSwitchOPMode */

/*---------------------------------------------------------------------------*/
/*!
 * \brief    This function is to inform CNM that channel privilege
 *           has been released
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*---------------------------------------------------------------------------*/
void p2pFuncReleaseCh(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	struct MSG_CH_ABORT *prMsgChRelease = (struct MSG_CH_ABORT *) NULL;

	DEBUGFUNC("p2pFuncReleaseCh()");

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL));

		if (!prChnlReqInfo->fgIsChannelRequested)
			break;
		DBGLOG(P2P, TRACE, "P2P Release Channel\n");
		prChnlReqInfo->fgIsChannelRequested = FALSE;

		/* 1. return channel privilege to CNM immediately */
		prMsgChRelease = (struct MSG_CH_ABORT *)
			cnmMemAlloc(prAdapter,
			RAM_TYPE_MSG, sizeof(struct MSG_CH_ABORT));
		if (!prMsgChRelease) {
			break;
		}

		prMsgChRelease->rMsgHdr.eMsgId = MID_MNY_CNM_CH_ABORT;
		prMsgChRelease->ucBssIndex = ucBssIdx;
		prMsgChRelease->ucTokenID = prChnlReqInfo->ucSeqNumOfChReq++;
#if CFG_SUPPORT_DBDC
		prMsgChRelease->eDBDCBand = ENUM_BAND_AUTO;

		DBGLOG(P2P, INFO,
			"p2pFuncReleaseCh: P2P abort channel on band %u.\n",
			prMsgChRelease->eDBDCBand);
#endif /*CFG_SUPPORT_DBDC*/
		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgChRelease,
			MSG_SEND_METHOD_BUF);

	} while (FALSE);
}				/* p2pFuncReleaseCh */

/*---------------------------------------------------------------------------*/
/*!
 * @brief Process of CHANNEL_REQ_JOIN Initial. Enter CHANNEL_REQ_JOIN State.
 *
 * @param (none)
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void p2pFuncAcquireCh(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx, IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	struct MSG_CH_REQ *prMsgChReq = (struct MSG_CH_REQ *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL));

		p2pFuncReleaseCh(prAdapter, ucBssIdx, prChnlReqInfo);

		/* send message to CNM for acquiring channel */
		prMsgChReq = (struct MSG_CH_REQ *)
				cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, sizeof(struct MSG_CH_REQ));

		if (!prMsgChReq) {
			/* Can't indicate CNM for channel acquiring */
			break;
		}

		prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
		prMsgChReq->ucBssIndex = ucBssIdx;
		prMsgChReq->ucTokenID = ++prChnlReqInfo->ucSeqNumOfChReq;
		prMsgChReq->eReqType = prChnlReqInfo->eChnlReqType;
		prMsgChReq->u4MaxInterval = prChnlReqInfo->u4MaxInterval;
		prMsgChReq->ucPrimaryChannel = prChnlReqInfo->ucReqChnlNum;
		prMsgChReq->eRfSco = prChnlReqInfo->eChnlSco;
		prMsgChReq->eRfBand = prChnlReqInfo->eBand;
		prMsgChReq->eRfChannelWidth = prChnlReqInfo->eChannelWidth;
		prMsgChReq->ucRfCenterFreqSeg1 = prChnlReqInfo->ucCenterFreqS1;
		prMsgChReq->ucRfCenterFreqSeg2 = prChnlReqInfo->ucCenterFreqS2;
#if CFG_SUPPORT_DBDC
		prMsgChReq->eDBDCBand = ENUM_BAND_AUTO;

		DBGLOG(P2P, INFO,
		   "p2pFuncAcquireCh: P2P Request channel on band %u, tokenID: %d, cookie: 0x%llx.\n",
		   prMsgChReq->eDBDCBand,
		   prMsgChReq->ucTokenID,
		   prChnlReqInfo->u8Cookie);

#endif /*CFG_SUPPORT_DBDC*/
		/* Channel request join BSSID. */

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgChReq,
			MSG_SEND_METHOD_BUF);

		prChnlReqInfo->fgIsChannelRequested = TRUE;

	} while (FALSE);
}				/* p2pFuncAcquireCh */

#if (CFG_SUPPORT_DFS_MASTER == 1)
void p2pFuncStartRdd(IN struct ADAPTER *prAdapter, IN uint8_t ucBssIdx)
{
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	uint8_t ucReqChnlNum;

	DEBUGFUNC("p2pFuncStartRdd()");


	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
			prAdapter->aprBssInfo[ucBssIdx]->u4PrivateData);

	ucReqChnlNum = prP2pRoleFsmInfo->rChnlReqInfo.ucReqChnlNum;

	prCmdRddOnOffCtrl = (struct CMD_RDD_ON_OFF_CTRL *)
		cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(*prCmdRddOnOffCtrl));

	if (!prCmdRddOnOffCtrl) {
		DBGLOG(P2P, ERROR,
			"cnmMemAlloc for prCmdRddOnOffCtrl failed!\n");
		return;
	}

	prCmdRddOnOffCtrl->ucDfsCtrl = RDD_START;

	/*
	 * FIX ME: Mobile driver can't get correct band.
	 * There is only 5G in DFS channel,
	 * which is on band_0. So it assigned to ENUM_BAND_0 as temp solution.
	 * Remember to fix it when driver could get
	 * the correct band from firmware.
	 */
	prCmdRddOnOffCtrl->ucRddIdx = ENUM_BAND_0;

#if (CFG_SUPPORT_SINGLE_SKU == 1)
	switch(rlmDomainGetDfsRegion()){
	case NL80211_DFS_FCC:
		prCmdRddOnOffCtrl->ucSetVal = ENUM_RDM_FCC;
		break;
	case NL80211_DFS_ETSI:
		prCmdRddOnOffCtrl->ucSetVal = ENUM_RDM_CE;
		break;
	case NL80211_DFS_JP:
		prCmdRddOnOffCtrl->ucSetVal = ENUM_RDM_JAP;
		break;
	default:
		DBGLOG(P2P, ERROR,"rlmDomainGetDfsRegion is NL80211_DFS_UNSET!\n");
		break;
	}

	if (rlmDomainIsSameCountryCode("KR", 2))
		prCmdRddOnOffCtrl->ucSetVal = ENUM_RDM_KR;
#endif

	if (prCmdRddOnOffCtrl->ucRddIdx)
		prCmdRddOnOffCtrl->ucRddRxSel = RDD_IN_SEL_1;
	else
		prCmdRddOnOffCtrl->ucRddRxSel = RDD_IN_SEL_0;

	DBGLOG(P2P, INFO,
		"p2pFuncStartRdd: Start Radar detection - DFS ctrl: %d, RDD index: %d\n",
		prCmdRddOnOffCtrl->ucDfsCtrl, prCmdRddOnOffCtrl->ucRddIdx);

	wlanSendSetQueryCmd(prAdapter,
		CMD_ID_RDD_ON_OFF_CTRL,
		TRUE,
		FALSE,
		FALSE,
		NULL,
		NULL,
		sizeof(*prCmdRddOnOffCtrl),
		(uint8_t *) prCmdRddOnOffCtrl, NULL, 0);

	cnmMemFree(prAdapter, prCmdRddOnOffCtrl);
}				/* p2pFuncStartRdd */

void p2pFuncStopRdd(IN struct ADAPTER *prAdapter, IN uint8_t ucBssIdx)
{
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;

	DEBUGFUNC("p2pFuncStopRdd()");

	prCmdRddOnOffCtrl = (struct CMD_RDD_ON_OFF_CTRL *)
		cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(*prCmdRddOnOffCtrl));

	if (!prCmdRddOnOffCtrl) {
		DBGLOG(P2P, ERROR,
				"cnmMemAlloc for prCmdRddOnOffCtrl failed!\n");
		return;
	}

	prCmdRddOnOffCtrl->ucDfsCtrl = RDD_STOP;

	/*
	 * FIX ME: Mobile driver can't get correct band.
	 * There is only 5G in DFS channel,
	 * which is on band_0. So it assigned to ENUM_BAND_0 as temp solution.
	 * Remember to fix it when driver could get
	 * the correct band from firmware.
	 */
	prCmdRddOnOffCtrl->ucRddIdx = ENUM_BAND_0;

	if (prCmdRddOnOffCtrl->ucRddIdx)
		prCmdRddOnOffCtrl->ucRddRxSel = RDD_IN_SEL_1;
	else
		prCmdRddOnOffCtrl->ucRddRxSel = RDD_IN_SEL_0;

	DBGLOG(P2P, INFO,
		"p2pFuncStopRdd: Stop Radar detection - DFS ctrl: %d, RDD index: %d\n",
		prCmdRddOnOffCtrl->ucDfsCtrl, prCmdRddOnOffCtrl->ucRddIdx);

	wlanSendSetQueryCmd(prAdapter,
		CMD_ID_RDD_ON_OFF_CTRL,
		TRUE,
		FALSE,
		FALSE,
		NULL,
		NULL,
		sizeof(*prCmdRddOnOffCtrl),
		(uint8_t *) prCmdRddOnOffCtrl, NULL, 0);

	cnmMemFree(prAdapter, prCmdRddOnOffCtrl);

}				/* p2pFuncStopRdd */


void p2pFuncDfsSwitchCh(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo,
		IN struct P2P_CHNL_REQ_INFO rP2pChnlReqInfo)
{

	struct GLUE_INFO *prGlueInfo;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;
	uint8_t role_idx = 0;
#if (CFG_SUPPORT_P2P_CSA == 1)
	u_int8_t fgIsPureAp = TRUE;
#endif

	DEBUGFUNC("p2pFuncDfsSwitchCh()");

	if (!prBssInfo) {
		DBGLOG(P2P, ERROR, "prBssInfo shouldn't be NULL!\n");
		return;
	}

	/*  Setup Channel, Band */
	prBssInfo->ucPrimaryChannel = rP2pChnlReqInfo.ucReqChnlNum;
	prBssInfo->eBand = rP2pChnlReqInfo.eBand;
	prBssInfo->eBssSCO = rP2pChnlReqInfo.eChnlSco;

/* To Support Cross Band Channel Swtich */
#if CFG_SUPPORT_IDC_CH_SWITCH
	if (prBssInfo->eBand == BAND_5G) {
		/* Depend on eBand */
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11A;
		/* Depend on eCurrentOPMode and ucPhyTypeSet */
		prBssInfo->ucConfigAdHocAPMode = AP_MODE_11A;
	} else { /* Only SAP mode should enter this function */
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11BG;
		/* Depend on eCurrentOPMode and ucPhyTypeSet */
		prBssInfo->ucConfigAdHocAPMode = AP_MODE_MIXED_11BG;
	}

#if (CFG_SUPPORT_P2P_CSA == 1)
	fgIsPureAp = p2pFuncIsAPMode(
			prAdapter->rWifiVar.prP2PConnSettings
			[prBssInfo->u4PrivateData]);
#endif

	/* Overwrite BSS PHY type set by Feature Options */
	bssDetermineApBssInfoPhyTypeSet(prAdapter,
		TRUE, prBssInfo);

	prBssInfo->ucNonHTBasicPhyType = (uint8_t)
		rNonHTApModeAttributes
			[prBssInfo->ucConfigAdHocAPMode]
				.ePhyTypeIndex;
	prBssInfo->u2BSSBasicRateSet =
		rNonHTApModeAttributes
			[prBssInfo->ucConfigAdHocAPMode]
				.u2BSSBasicRateSet;
	prBssInfo->u2OperationalRateSet =
		rNonHTPhyAttributes
			[prBssInfo->ucNonHTBasicPhyType]
				.u2SupportedRateSet;
	kalMemZero(prBssInfo->aucAllSupportedRates, RATE_NUM_SW);
	rateGetDataRatesFromRateSet(
		prBssInfo->u2OperationalRateSet,
		prBssInfo->u2BSSBasicRateSet,
		prBssInfo->aucAllSupportedRates,
		&prBssInfo->ucAllSupportedRatesLen);
#endif

	/* Setup channel and bandwidth */
	rlmBssInitForAPandIbss(prAdapter, prBssInfo);

#if (CFG_SUPPORT_P2P_CSA == 1)
	/* Update Beacon to FW. Note that we have to set Op mode Rx
	 * flag to TRUE in order to update VHT OP Notification IE.
	 * Otherwise, clients will not be able to process VHT OP
	 * Notification IE and assume wrong Rx NSS.
	 */
	prBssInfo->fgIsOpChangeRxNss = TRUE;

	if (fgIsPureAp)
		bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);
	else if (rlmUpdateParamsForAP(prAdapter, prBssInfo, FALSE) == FALSE)
		bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);

	prBssInfo->fgIsOpChangeRxNss = FALSE;
#else
	/* Update Beacon again for network phy type confirmed. */
	bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);
#endif


	/* Reset HW TSF Update Mode and Beacon Mode */
	nicUpdateBss(prAdapter, prBssInfo->ucBssIndex);

	nicPmIndicateBssCreated(prAdapter, prBssInfo->ucBssIndex);

	prCmdRddOnOffCtrl = (struct CMD_RDD_ON_OFF_CTRL *)
		cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(*prCmdRddOnOffCtrl));

	if (!prCmdRddOnOffCtrl) {
		DBGLOG(P2P, ERROR,
			"cnmMemAlloc for prCmdRddOnOffCtrl failed!\n");
		return;
	}

	prCmdRddOnOffCtrl->ucDfsCtrl = RDD_START_TXQ;

	/*
	 * FIX ME: Mobile driver can't get correct band.
	 * There is only 5G in DFS channel,
	 * which is on band_0. So it assigned to ENUM_BAND_0
	 * as temp solution.
	 * Remember to fix it when driver could get
	 * the correct band from firmware.
	 */
	prCmdRddOnOffCtrl->ucRddIdx = ENUM_BAND_0;

	DBGLOG(P2P, INFO,
		"p2pFuncDfsSwitchCh: Start TXQ - DFS ctrl: %.d\n",
		prCmdRddOnOffCtrl->ucDfsCtrl);

	wlanSendSetQueryCmd(prAdapter,
		CMD_ID_RDD_ON_OFF_CTRL,
		TRUE,
		FALSE,
		FALSE,
		NULL,
		NULL,
		sizeof(*prCmdRddOnOffCtrl),
		(uint8_t *) prCmdRddOnOffCtrl,
		NULL, 0);

	cnmMemFree(prAdapter, prCmdRddOnOffCtrl);

	prP2pRoleFsmInfo =
		P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
			prBssInfo->u4PrivateData);

	prGlueInfo = prAdapter->prGlueInfo;
	role_idx = prP2pRoleFsmInfo->ucRoleIndex;

#if CFG_SUPPORT_SAP_DFS_CHANNEL
	wlanUpdateDfsChannelTable(prGlueInfo,
		role_idx,
		prBssInfo->ucPrimaryChannel,
		prBssInfo->ucVhtChannelWidth,
		prBssInfo->eBssSCO,
		nicChannelNum2Freq(
			prBssInfo->ucVhtChannelFrequencyS1,
			prBssInfo->eBand) / 1000,
		prBssInfo->eBand);
#endif

	kalP2pIndicateChnlSwitch(prAdapter, prBssInfo);

	/* Down the flag */
	prAdapter->rWifiVar.ucChannelSwitchMode = 0;
#if CFG_SUPPORT_DBDC
	/* Check DBDC status */
	cnmDbdcRuntimeCheckDecision(prAdapter,
						prBssInfo->ucBssIndex,
						FALSE);

#endif
} /* p2pFuncDfsSwitchCh */

u_int8_t p2pFuncCheckWeatherRadarBand(
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	uint8_t ucReqChnlNum;
	uint8_t ucCenterFreqS1;
	enum ENUM_CHANNEL_WIDTH eChannelWidth;
	enum ENUM_CHNL_EXT eChnlSco;


	ucReqChnlNum = prChnlReqInfo->ucReqChnlNum;
	ucCenterFreqS1 = prChnlReqInfo->ucCenterFreqS1;
	eChannelWidth = prChnlReqInfo->eChannelWidth;
	eChnlSco = prChnlReqInfo->eChnlSco;

#if (CFG_SUPPORT_SINGLE_SKU == 1)
	if (rlmDomainGetDfsRegion() == NL80211_DFS_ETSI) {
		if (eChannelWidth == VHT_OP_CHANNEL_WIDTH_80) {
			if (ucCenterFreqS1 >= 120 && ucCenterFreqS1 <= 128)
				return TRUE;
		} else {
			if ((ucReqChnlNum >= 120 && ucReqChnlNum <= 128))
				return TRUE;
			else if (ucReqChnlNum == 116
				&& eChnlSco == CHNL_EXT_SCA)
				return TRUE; /* ch116, 120 BW40 */
		}
	}
#endif

	return FALSE;
}

int32_t p2pFuncSetDriverCacTime(IN uint32_t u4CacTime)
{
	uint32_t i4Status = WLAN_STATUS_SUCCESS;

	g_u4DriverCacTime = u4CacTime;

	DBGLOG(P2P, INFO,
		"p2pFuncSetDriverCacTime: g_u4ManualCacTime = %dsec\n",
		g_u4DriverCacTime);

	return i4Status;
}

void p2pFuncEnableManualCac(void)
{
	g_fgManualCac = TRUE;
}

uint32_t p2pFuncGetDriverCacTime(void)
{
	return g_u4DriverCacTime;
}

u_int8_t p2pFuncIsManualCac(void)
{
	return g_fgManualCac;
}

void p2pFuncRadarInfoInit(void)
{
	kalMemZero(&g_rP2pRadarInfo, sizeof(g_rP2pRadarInfo));
}
/*
void p2pFuncShowRadarInfo(IN struct ADAPTER *prAdapter, IN uint8_t ucBssIdx)
{
	uint8_t ucCnt = 0;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	uint8_t ucReqChnlNum;

	if (g_rP2pRadarInfo.ucRadarReportMode == 1) {

		prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
				prAdapter->aprBssInfo[ucBssIdx]->u4PrivateData);

		ucReqChnlNum = prP2pRoleFsmInfo->rChnlReqInfo.ucReqChnlNum;

		DBGLOG(P2P, INFO, "-----Radar Detected Event-----\n");
		DBGLOG(P2P, INFO,
			"Radar detected in DBDC band%d\n",
				g_rP2pRadarInfo.ucRddIdx);

#if (CFG_SUPPORT_SINGLE_SKU == 1)
		switch (rlmDomainGetDfsRegion()) {
		case NL80211_DFS_FCC:
			DBGLOG(P2P, INFO, "Regulation domain: FCC\n");
			break;
		case NL80211_DFS_ETSI:
			DBGLOG(P2P, INFO, "Regulation domain: ETSI\n");
			break;
		case NL80211_DFS_JP:
			DBGLOG(P2P, INFO, "Regulation domain: JP\n");

			if (ucReqChnlNum >= 52 && ucReqChnlNum <= 64)
				DBGLOG(P2P, INFO,
					"Radar type: W53 - %s\n",
					p2pFuncJpW53RadarType());
			else if (ucReqChnlNum >= 100 && ucReqChnlNum <= 140)
				DBGLOG(P2P, INFO,
					"Radar type: W56 - %s\n",
					p2pFuncJpW56RadarType());
			break;
		default:
			break;
		}
#endif

		DBGLOG(P2P, INFO, "Radar Content:\n");

		DBGLOG(P2P, INFO, "start time    pulse width    PRI\n");

		if (g_rP2pRadarInfo.ucPeriodicDetected) {
			DBGLOG(P2P, INFO, "%-10d    %-11d    -\n"
				, g_rP2pRadarInfo.arPpbContent
					[ucCnt].u4PeriodicStartTime
				, g_rP2pRadarInfo.arPpbContent
					[ucCnt].u2PeriodicPulseWidth);

			for (ucCnt = 1;
				ucCnt < g_rP2pRadarInfo.ucPPBNum; ucCnt++) {
				DBGLOG(P2P, INFO, "%-10d    %-11d    %d\n"
					, g_rP2pRadarInfo.arPpbContent
						[ucCnt].u4PeriodicStartTime
					, g_rP2pRadarInfo.arPpbContent
						[ucCnt].u2PeriodicPulseWidth
					, (g_rP2pRadarInfo.arPpbContent
						[ucCnt].u4PeriodicStartTime
					- g_rP2pRadarInfo.arPpbContent
						[ucCnt-1].u4PeriodicStartTime)
						* 2 / 5);
			}
		} else if (g_rP2pRadarInfo.ucLongDetected) {
			DBGLOG(P2P, INFO, "%-10d    %-11d    -\n"
				, g_rP2pRadarInfo.arLpbContent
					[ucCnt].u4LongStartTime
				, g_rP2pRadarInfo.arLpbContent
					[ucCnt].u2LongPulseWidth);

			for (ucCnt = 1;
				ucCnt < g_rP2pRadarInfo.ucLPBNum; ucCnt++) {
				DBGLOG(P2P, INFO, "%-10d    %-11d    %d\n"
					, g_rP2pRadarInfo.arLpbContent
						[ucCnt].u4LongStartTime
					, g_rP2pRadarInfo.arLpbContent
						[ucCnt].u2LongPulseWidth
					, (g_rP2pRadarInfo.arLpbContent
						[ucCnt].u4LongStartTime
					- g_rP2pRadarInfo.arLpbContent
						[ucCnt-1].u4LongStartTime)
						* 2 / 5);
			}
		}
	}
}
*/

void p2pFuncGetRadarInfo(IN struct P2P_RADAR_INFO *prP2pRadarInfo)
{
	kalMemCopy(prP2pRadarInfo, &g_rP2pRadarInfo, sizeof(*prP2pRadarInfo));
}

/*
uint8_t *p2pFuncJpW53RadarType(void)
{
	uint32_t u4Type1Diff;
	uint32_t u4Type2Diff;

	if (g_rP2pRadarInfo.u4PRI1stUs >= 1428)
		u4Type1Diff = g_rP2pRadarInfo.u4PRI1stUs - 1428;
	else
		u4Type1Diff = 1428 - g_rP2pRadarInfo.u4PRI1stUs;

	if (g_rP2pRadarInfo.u4PRI1stUs >= 3846)
		u4Type2Diff = g_rP2pRadarInfo.u4PRI1stUs - 3846;
	else
		u4Type2Diff = 3846 - g_rP2pRadarInfo.u4PRI1stUs;

	if (u4Type1Diff < u4Type2Diff)
		return apucW53RadarType[1];
	else
		return apucW53RadarType[2];
}

uint8_t *p2pFuncJpW56RadarType(void)
{
	uint32_t u4Type1Diff;
	uint32_t u4Type2Diff;

	if (g_rP2pRadarInfo.ucLongDetected)
		return apucW56RadarType[7];

	if (g_rP2pRadarInfo.u4PRI1stUs >= 3980
		&& g_rP2pRadarInfo.u4PRI1stUs <= 4020)
		return apucW56RadarType[3];

	if (g_rP2pRadarInfo.u4PRI1stUs >= 1368
		&& g_rP2pRadarInfo.u4PRI1stUs <= 1448) {

		if (g_rP2pRadarInfo.u4PRI1stUs >= 1388)
			u4Type1Diff = g_rP2pRadarInfo.u4PRI1stUs - 1388;
		else
			u4Type1Diff = 1388 - g_rP2pRadarInfo.u4PRI1stUs;

		if (g_rP2pRadarInfo.u4PRI1stUs >= 1428)
			u4Type2Diff = g_rP2pRadarInfo.u4PRI1stUs - 1428;
		else
			u4Type2Diff = 1428 - g_rP2pRadarInfo.u4PRI1stUs;

		if (u4Type1Diff < u4Type2Diff)
			return apucW56RadarType[1];
		else
			return apucW56RadarType[2];

	}

	if (g_rP2pRadarInfo.u4PRI1stUs >= 130
		&& g_rP2pRadarInfo.u4PRI1stUs < 200)
		return apucW56RadarType[4];

	if (g_rP2pRadarInfo.u4PRI1stUs >= 200
		&& g_rP2pRadarInfo.u4PRI1stUs <= 520) {

		if (g_rP2pRadarInfo.u4PRI1stUs <= 230)
		return apucW56RadarType[9];

		if (g_rP2pRadarInfo.u4PRI1stUs >= 323
			&& g_rP2pRadarInfo.u4PRI1stUs <= 343)
			return apucW56RadarType[10];

		return apucW56RadarType[11];
	}

	return apucW56RadarType[0];
}
*/

void p2pFuncSetRadarDetectMode(IN uint8_t ucRadarDetectMode)
{
	g_ucRadarDetectMode = ucRadarDetectMode;

	DBGLOG(P2P, INFO,
		"p2pFuncSetRadarDetectMode: g_ucRadarDetectMode: %d\n",
		g_ucRadarDetectMode);
}

uint8_t p2pFuncGetRadarDetectMode(void)
{
	return g_ucRadarDetectMode;
}

void p2pFuncSetDfsState(IN uint8_t ucDfsState)
{
	DBGLOG(P2P, INFO,
		"[DFS_STATE] TRANSITION: [%s] -> [%s]\n",
		apucDfsState[g_ucDfsState], apucDfsState[ucDfsState]);

	g_ucDfsState = ucDfsState;
}

uint8_t p2pFuncGetDfsState(void)
{
	return g_ucDfsState;
}

uint8_t *p2pFuncShowDfsState(void)
{
	return apucDfsState[g_ucDfsState];
}

void p2pFuncRecordCacStartBootTime(void)
{
	g_u4CacStartBootTime = kalGetBootTime();
}

uint32_t p2pFuncGetCacRemainingTime(void)
{
	uint32_t u4CurrentBootTime;
	uint32_t u4CacRemainingTime;

	u4CurrentBootTime = kalGetBootTime();

	u4CacRemainingTime = g_u4DriverCacTime -
		(u4CurrentBootTime - g_u4CacStartBootTime)/1000000;

	return u4CacRemainingTime;
}
#endif

#if 0
uint32_t
p2pFuncBeaconUpdate(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucBcnHdr,
		IN uint32_t u4HdrLen,
		IN uint8_t *pucBcnBody,
		IN uint32_t u4BodyLen,
		IN uint32_t u4DtimPeriod,
		IN uint32_t u4BcnInterval)
{
	uint32_t rResultStatus = WLAN_STATUS_INVALID_DATA;
	struct WLAN_BEACON_FRAME *prBcnFrame =
		(struct WLAN_BEACON_FRAME *) NULL;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
	struct MSDU_INFO *prBcnMsduInfo = (struct MSDU_INFO *) NULL;
	uint8_t *pucTIMBody = (uint8_t *) NULL;
	uint16_t u2FrameLength = 0, uint16_t u2OldBodyLen = 0;
	uint8_t aucIEBuf[MAX_IE_LENGTH];

	do {
		ASSERT_BREAK(prAdapter != NULL);

		prP2pBssInfo =
			&(prAdapter->rWifiVar
			.arBssInfo[NETWORK_TYPE_P2P_INDEX]);
		prBcnMsduInfo = prP2pBssInfo->prBeacon;
		ASSERT_BREAK(prBcnMsduInfo != NULL);

		/* TODO: Find TIM IE pointer. */
		prBcnFrame = prBcnMsduInfo->prPacket;

		ASSERT_BREAK(prBcnFrame != NULL);

		do {
			/* Ori header. */
			uint16_t u2IELength = 0, u2Offset = 0;
			uint8_t *pucIEBuf = prBcnFrame->aucInfoElem;

			u2IELength = prBcnMsduInfo->u2FrameLength -
				prBcnMsduInfo->ucMacHeaderLength;

			IE_FOR_EACH(pucIEBuf, u2IELength, u2Offset) {
				if ((IE_ID(pucIEBuf) == ELEM_ID_TIM)
					|| ((IE_ID(pucIEBuf)
					> ELEM_ID_IBSS_PARAM_SET))) {
					pucTIMBody = pucIEBuf;
					break;
				}
				u2FrameLength += IE_SIZE(pucIEBuf);
			}

			if (pucTIMBody == NULL)
				pucTIMBody = pucIEBuf;

			/* Body not change. */
			u2OldBodyLen = (uint16_t) ((uint32_t) pucTIMBody -
				(uint32_t) prBcnFrame->aucInfoElem);
			/* Move body. */
			kalMemCmp(aucIEBuf, pucTIMBody, u2OldBodyLen);
		} while (FALSE);
		if (pucBcnHdr) {
			kalMemCopy(prBcnMsduInfo->prPacket,
				pucBcnHdr, u4HdrLen);
			pucTIMBody = (uint8_t *)
				((uint32_t) prBcnMsduInfo->prPacket + u4HdrLen);
			prBcnMsduInfo->ucMacHeaderLength =
			    (WLAN_MAC_MGMT_HEADER_LEN +
			     (TIMESTAMP_FIELD_LEN +
			     BEACON_INTERVAL_FIELD_LEN + CAP_INFO_FIELD_LEN));
			/* Header + Partial Body. */
			u2FrameLength = u4HdrLen;
		} else {
			/* Header not change. */
			u2FrameLength += prBcnMsduInfo->ucMacHeaderLength;
		}

		if (pucBcnBody) {
			kalMemCopy(pucTIMBody, pucBcnBody, u4BodyLen);
			u2FrameLength += (uint16_t) u4BodyLen;
		} else {
			kalMemCopy(pucTIMBody, aucIEBuf, u2OldBodyLen);
			u2FrameLength += u2OldBodyLen;
		}

		/* Frame Length */
		prBcnMsduInfo->u2FrameLength = u2FrameLength;
		prBcnMsduInfo->fgIs802_11 = TRUE;
		prBcnMsduInfo->ucNetworkType = NETWORK_TYPE_P2P_INDEX;
		prP2pBssInfo->u2BeaconInterval = (uint16_t) u4BcnInterval;
		prP2pBssInfo->ucDTIMPeriod = (uint8_t) u4DtimPeriod;
		prP2pBssInfo->u2CapInfo = prBcnFrame->u2CapInfo;
		prBcnMsduInfo->ucPacketType = 3;
		rResultStatus = nicUpdateBeaconIETemplate(prAdapter,
			IE_UPD_METHOD_UPDATE_ALL,
			NETWORK_TYPE_P2P_INDEX,
			prP2pBssInfo->u2CapInfo,
			(uint8_t *) prBcnFrame->aucInfoElem,
			prBcnMsduInfo->u2FrameLength -
			OFFSET_OF(struct WLAN_BEACON_FRAME, aucInfoElem));
		if (prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
			/* AP is created, Beacon Update. */
			nicPmIndicateBssAbort(prAdapter,
				NETWORK_TYPE_P2P_INDEX);
			nicPmIndicateBssCreated(prAdapter,
				NETWORK_TYPE_P2P_INDEX);
		}

	} while (FALSE);
	return rResultStatus;
}				/* p2pFuncBeaconUpdate */

#else
uint32_t
p2pFuncBeaconUpdate(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN struct P2P_BEACON_UPDATE_INFO *prBcnUpdateInfo,
		IN uint8_t *pucNewBcnHdr,
		IN uint32_t u4NewHdrLen,
		IN uint8_t *pucNewBcnBody,
		IN uint32_t u4NewBodyLen)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct WLAN_BEACON_FRAME *prBcnFrame =
		(struct WLAN_BEACON_FRAME *) NULL;
	struct MSDU_INFO *prBcnMsduInfo = (struct MSDU_INFO *) NULL;
	uint8_t *pucIEBuf = (uint8_t *) NULL;
	uint8_t aucIEBuf[MAX_IE_LENGTH];

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prP2pBssInfo != NULL)
			&& (prBcnUpdateInfo != NULL));

		prBcnMsduInfo = prP2pBssInfo->prBeacon;

#if DBG
		if (prBcnUpdateInfo->pucBcnHdr != NULL) {
			ASSERT((uint32_t) prBcnUpdateInfo->pucBcnHdr ==
				((uint32_t) prBcnMsduInfo->prPacket +
				MAC_TX_RESERVED_FIELD));
		}

		if (prBcnUpdateInfo->pucBcnBody != NULL) {
			ASSERT((uint32_t) prBcnUpdateInfo->pucBcnBody ==
				((uint32_t) prBcnUpdateInfo->pucBcnHdr +
				(uint32_t) prBcnUpdateInfo->u4BcnHdrLen));
		}
#endif
		prBcnFrame = (struct WLAN_BEACON_FRAME *)
			((unsigned long) prBcnMsduInfo->prPacket +
			MAC_TX_RESERVED_FIELD);

		if (!pucNewBcnBody) {
			/* Old body. */
			pucNewBcnBody = prBcnUpdateInfo->pucBcnBody;
			ASSERT(u4NewBodyLen == 0);
			u4NewBodyLen = prBcnUpdateInfo->u4BcnBodyLen;
		} else {
			prBcnUpdateInfo->u4BcnBodyLen = u4NewBodyLen;
		}

		/* Temp buffer body part. */
		kalMemCopy(aucIEBuf, pucNewBcnBody, u4NewBodyLen);

		if (pucNewBcnHdr) {
			kalMemCopy(prBcnFrame, pucNewBcnHdr, u4NewHdrLen);
			prBcnUpdateInfo->pucBcnHdr = (uint8_t *) prBcnFrame;
			prBcnUpdateInfo->u4BcnHdrLen = u4NewHdrLen;
		}

		pucIEBuf = (uint8_t *)
			((unsigned long) prBcnUpdateInfo->pucBcnHdr +
			(unsigned long) prBcnUpdateInfo->u4BcnHdrLen);
		kalMemCopy(pucIEBuf, aucIEBuf, u4NewBodyLen);
		prBcnUpdateInfo->pucBcnBody = pucIEBuf;

		/* Frame Length */
		prBcnMsduInfo->u2FrameLength = (uint16_t)
			(prBcnUpdateInfo->u4BcnHdrLen +
			prBcnUpdateInfo->u4BcnBodyLen);

		prBcnMsduInfo->ucPacketType = TX_PACKET_TYPE_MGMT;
		prBcnMsduInfo->fgIs802_11 = TRUE;
		prBcnMsduInfo->ucBssIndex = prP2pBssInfo->ucBssIndex;

		/* Update BSS INFO related information. */
		COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr,
			prBcnFrame->aucSrcAddr);
		COPY_MAC_ADDR(prP2pBssInfo->aucBSSID, prBcnFrame->aucBSSID);
		prP2pBssInfo->u2CapInfo = prBcnFrame->u2CapInfo;

		p2pFuncParseBeaconContent(prAdapter,
			prP2pBssInfo,
			(uint8_t *) prBcnFrame->aucInfoElem,
			(prBcnMsduInfo->u2FrameLength -
			OFFSET_OF(struct WLAN_BEACON_FRAME, aucInfoElem)));

#if 1
		/* bssUpdateBeaconContent(prAdapter, NETWORK_TYPE_P2P_INDEX); */
#else
		nicUpdateBeaconIETemplate(prAdapter,
			IE_UPD_METHOD_UPDATE_ALL,
			NETWORK_TYPE_P2P_INDEX,
			prBcnFrame->u2CapInfo,
			(uint8_t *) prBcnFrame->aucInfoElem,
			(prBcnMsduInfo->u2FrameLength -
			OFFSET_OF(struct WLAN_BEACON_FRAME, aucInfoElem)));
#endif
	} while (FALSE);

	return rWlanStatus;
}				/* p2pFuncBeaconUpdate */

/*---------------------------------------------------------------------------*/
/*!
 * \brief    This function is to update extra IEs (ex: WPS) for assoc resp.
 *           Caller should sanity check the params.
 *
 * \param[in] prAdapter      Pointer of ADAPTER_T
 * \param[in] prP2pBssInfo   Pointer to BSS_INFO_T structure
 * \param[in] AssocRespIE    Pointer to extra IEs for assoc resp
 * \param[in] u4AssocRespLen Length of extra IEs for assoc resp
 *
 * \return WLAN_STATUS
 */
/*---------------------------------------------------------------------------*/

uint32_t
p2pFuncAssocRespUpdate(IN struct ADAPTER *prAdapter,
		    IN struct BSS_INFO *prP2pBssInfo,
		    IN uint8_t *AssocRespIE, IN uint32_t u4AssocRespLen)
{
	uint8_t ucOuiType = 0;
	uint16_t u2SubTypeVersion = 0;

	if (!rsnParseCheckForWFAInfoElem(prAdapter,
		AssocRespIE, &ucOuiType, &u2SubTypeVersion))
		return WLAN_STATUS_FAILURE;

	if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
		kalP2PUpdateWSC_IE(prAdapter->prGlueInfo, 3,
			(uint8_t *)AssocRespIE, IE_SIZE(AssocRespIE),
			(uint8_t) (prP2pBssInfo->u4PrivateData));
	}

	return WLAN_STATUS_SUCCESS;
}

#endif

#if 0
/* TODO: We do not apply IE in deauth frame set from upper layer now. */
uint32_t
p2pFuncDeauth(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucPeerMacAddr,
		IN uint16_t u2ReasonCode,
		IN uint8_t *pucIEBuf,
		IN uint16_t u2IELen,
		IN u_int8_t fgSendDeauth)
{
	uint32_t rWlanStatus = WLAN_STATUS_FAILURE;
	struct STA_RECORD *prCliStaRec = (struct STA_RECORD *) NULL;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
	u_int8_t fgIsStaFound = FALSE;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (pucPeerMacAddr != NULL));

		prP2pBssInfo =
			&(prAdapter->rWifiVar
			.arBssInfo[NETWORK_TYPE_P2P_INDEX]);

		prCliStaRec = cnmGetStaRecByAddress(prAdapter,
			NETWORK_TYPE_P2P_INDEX, pucPeerMacAddr);

		switch (prP2pBssInfo->eCurrentOPMode) {
		case OP_MODE_ACCESS_POINT:
			{
				struct LINK *prStaRecOfClientList =
					(struct LINK *) NULL;
				struct LINK_ENTRY *prLinkEntry =
					(struct LINK_ENTRY *) NULL;

				prStaRecOfClientList =
					&(prP2pBssInfo->rStaRecOfClientList);

				LINK_FOR_EACH(prLinkEntry,
					prStaRecOfClientList) {
					if ((uint32_t) prCliStaRec
						== (uint32_t) prLinkEntry) {
						LINK_REMOVE_KNOWN_ENTRY(
						prStaRecOfClientList,
						&prCliStaRec->rLinkEntry);
						fgIsStaFound = TRUE;
						break;
					}
				}

			}
			break;
		case OP_MODE_INFRASTRUCTURE:
			ASSERT(prCliStaRec == prP2pBssInfo->prStaRecOfAP);
			if (prCliStaRec != prP2pBssInfo->prStaRecOfAP)
				break;
			prP2pBssInfo->prStaRecOfAP = NULL;
			fgIsStaFound = TRUE;
			break;
		default:
			break;
		}

		if (fgIsStaFound)
			p2pFuncDisconnect(prAdapter,
				prCliStaRec, fgSendDeauth, u2ReasonCode);

		rWlanStatus = WLAN_STATUS_SUCCESS;
	} while (FALSE);

	return rWlanStatus;
}				/* p2pFuncDeauth */

/* TODO: We do not apply IE in disassoc frame set from upper layer now. */
uint32_t
p2pFuncDisassoc(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucPeerMacAddr,
		IN uint16_t u2ReasonCode,
		IN uint8_t *pucIEBuf,
		IN uint16_t u2IELen,
		IN u_int8_t fgSendDisassoc)
{
	uint32_t rWlanStatus = WLAN_STATUS_FAILURE;
	struct STA_RECORD *prCliStaRec = (struct STA_RECORD *) NULL;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
	u_int8_t fgIsStaFound = FALSE;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (pucPeerMacAddr != NULL));

		prP2pBssInfo =
			&(prAdapter->rWifiVar
				.arBssInfo[NETWORK_TYPE_P2P_INDEX]);

		prCliStaRec = cnmGetStaRecByAddress(prAdapter,
			NETWORK_TYPE_P2P_INDEX, pucPeerMacAddr);

		switch (prP2pBssInfo->eCurrentOPMode) {
		case OP_MODE_ACCESS_POINT:
			{
				struct LINK *prStaRecOfClientList =
					(struct LINK *) NULL;
				struct LINK_ENTRY *prLinkEntry =
					(struct LINK_ENTRY *) NULL;

				prStaRecOfClientList =
					&(prP2pBssInfo->rStaRecOfClientList);

				LINK_FOR_EACH(prLinkEntry,
					prStaRecOfClientList) {
					if ((uint32_t) prCliStaRec
						== (uint32_t) prLinkEntry) {
						LINK_REMOVE_KNOWN_ENTRY(
						prStaRecOfClientList,
						&prCliStaRec->rLinkEntry);
						fgIsStaFound = TRUE;
						/* p2pFuncDisconnect(prAdapter,
						 * prCliStaRec,
						 */
						/* fgSendDisassoc,
						 * u2ReasonCode);
						 */
						break;
					}
				}

			}
			break;
		case OP_MODE_INFRASTRUCTURE:
			ASSERT(prCliStaRec == prP2pBssInfo->prStaRecOfAP);
			if (prCliStaRec != prP2pBssInfo->prStaRecOfAP)
				break;
			/* p2pFuncDisconnect(prAdapter,
			 * prCliStaRec, fgSendDisassoc, u2ReasonCode);
			 */
			prP2pBssInfo->prStaRecOfAP = NULL;
			fgIsStaFound = TRUE;
			break;
		default:
			break;
		}

		if (fgIsStaFound) {

			p2pFuncDisconnect(prAdapter,
				prCliStaRec, fgSendDisassoc, u2ReasonCode);
			/* 20120830 moved into p2pFuncDisconnect(). */
			/* cnmStaRecFree(prAdapter, prCliStaRec); */

		}

		rWlanStatus = WLAN_STATUS_SUCCESS;
	} while (FALSE);

	return rWlanStatus;
}				/* p2pFuncDisassoc */

#endif

#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
uint32_t
p2pFuncProbeRespUpdate(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN uint8_t *ProbeRespIE, IN uint32_t u4ProbeRespLen)

{
	struct MSDU_INFO *prMsduInfo = (struct MSDU_INFO *) NULL;
	uint32_t u4IeArraySize = 0, u4Idx = 0;
	uint8_t *pucP2pIe = (uint8_t *) NULL;
	uint8_t *pucWpsIe = (uint8_t *) NULL;
	uint8_t *pucWfdIe = (uint8_t *) NULL;

	if (prP2pBssInfo == NULL)
		return WLAN_STATUS_FAILURE;

	/* reuse beacon MsduInfo */
	prMsduInfo = prP2pBssInfo->prBeacon;

	/* beacon prMsduInfo will be NULLify
	 * once BSS deactivated, so skip if it is
	 */
	if (!prMsduInfo)
		return WLAN_STATUS_SUCCESS;

	if (!ProbeRespIE) {
		DBGLOG(BSS, INFO,
			"change beacon: has no extra probe response IEs\n");
		return WLAN_STATUS_SUCCESS;
	}
	if (p2pFuncIsAPMode(
		prAdapter->rWifiVar
			.prP2PConnSettings[prP2pBssInfo->u4PrivateData])) {
		DBGLOG(BSS, INFO,
			"change beacon: pure Ap mode do not add extra probe response IEs\n");
		return WLAN_STATUS_SUCCESS;
	}
	prMsduInfo->u2FrameLength = 0;

	bssBuildBeaconProbeRespFrameCommonIEs(prMsduInfo,
		prP2pBssInfo, ProbeRespIE);

	u4IeArraySize =
		sizeof(txProbeRspIETable) / sizeof(struct APPEND_VAR_IE_ENTRY);

	for (u4Idx = 0; u4Idx < u4IeArraySize; u4Idx++) {
		if (txProbeRspIETable[u4Idx].pfnAppendIE)
			txProbeRspIETable[u4Idx]
				.pfnAppendIE(prAdapter, prMsduInfo);
	}

	/* process probe response IE from supplicant */
	pucP2pIe = (uint8_t *) cfg80211_find_vendor_ie(WLAN_OUI_WFA,
			WLAN_OUI_TYPE_WFA_P2P,
			ProbeRespIE,
			u4ProbeRespLen);

	pucWfdIe = (uint8_t *) cfg80211_find_vendor_ie(WLAN_OUI_WFA,
			WLAN_OUI_TYPE_WFA_P2P + 1,
			ProbeRespIE,
			u4ProbeRespLen);

	pucWpsIe = (uint8_t *) cfg80211_find_vendor_ie(WLAN_OUI_MICROSOFT,
			WLAN_OUI_TYPE_MICROSOFT_WPS,
			ProbeRespIE,
			u4ProbeRespLen);

	if (pucP2pIe) {
		kalMemCopy(prMsduInfo->prPacket + prMsduInfo->u2FrameLength,
				pucP2pIe, IE_SIZE(pucP2pIe));
		prMsduInfo->u2FrameLength += IE_SIZE(pucP2pIe);
	}

	if (pucWfdIe) {
		kalMemCopy(prMsduInfo->prPacket + prMsduInfo->u2FrameLength,
				pucWfdIe, IE_SIZE(pucWfdIe));
		prMsduInfo->u2FrameLength += IE_SIZE(pucWfdIe);
	}

	if (pucWpsIe) {
		kalMemCopy(prMsduInfo->prPacket + prMsduInfo->u2FrameLength,
				pucWpsIe, IE_SIZE(pucWpsIe));
		prMsduInfo->u2FrameLength += IE_SIZE(pucWpsIe);
	}

	DBGLOG(BSS, INFO,
		"update probe response for bss index: %d, IE len: %d\n",
		prP2pBssInfo->ucBssIndex, prMsduInfo->u2FrameLength);
	/* dumpMemory8(prMsduInfo->prPacket, prMsduInfo->u2FrameLength); */

	return nicUpdateBeaconIETemplate(prAdapter,
					IE_UPD_METHOD_UPDATE_PROBE_RSP,
					prP2pBssInfo->ucBssIndex,
					prP2pBssInfo->u2CapInfo,
					prMsduInfo->prPacket,
					prMsduInfo->u2FrameLength);
}
#endif

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function is called to dissolve from group or one group.
 *           (Would not change P2P FSM.)
 *              1. GC: Disconnect from AP. (Send Deauth)
 *              2. GO: Disconnect all STA
 *
 * @param[in] prAdapter   Pointer to the adapter structure.
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void
p2pFuncDissolve(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN u_int8_t fgSendDeauth,
		IN uint16_t u2ReasonCode)
{
	struct STA_RECORD *prCurrStaRec, *prStaRecNext;
	struct LINK *prClientList;

	DEBUGFUNC("p2pFuncDissolve()");

	do {

		ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL));

		switch (prP2pBssInfo->eCurrentOPMode) {
		case OP_MODE_INFRASTRUCTURE:
			/* Reset station record status. */
			if (prP2pBssInfo->prStaRecOfAP) {
#if CFG_WPS_DISCONNECT || (KERNEL_VERSION(4, 4, 0) <= CFG80211_VERSION_CODE)
				kalP2PGCIndicateConnectionStatus(
					prAdapter->prGlueInfo,
					(uint8_t) prP2pBssInfo->u4PrivateData,
					NULL, NULL, 0,
					REASON_CODE_DEAUTH_LEAVING_BSS,
					WLAN_STATUS_MEDIA_DISCONNECT);
#else
				kalP2PGCIndicateConnectionStatus(
					prAdapter->prGlueInfo,
					(uint8_t) prP2pBssInfo->u4PrivateData,
					NULL, NULL, 0,
					REASON_CODE_DEAUTH_LEAVING_BSS);
#endif

				/* 2012/02/14 frog:
				 * After formation before join group,
				 * prStaRecOfAP is NULL.
				 */
				p2pFuncDisconnect(prAdapter,
					prP2pBssInfo,
					prP2pBssInfo->prStaRecOfAP,
					fgSendDeauth,
					u2ReasonCode);
			}

			/* Fix possible KE when RX Beacon &
			 * call nicPmIndicateBssConnected().
			 * hit prStaRecOfAP == NULL.
			 */
			p2pChangeMediaState(prAdapter,
				prP2pBssInfo,
				MEDIA_STATE_DISCONNECTED);

			prP2pBssInfo->prStaRecOfAP = NULL;

			break;
		case OP_MODE_ACCESS_POINT:
			/* Under AP mode, we would net
			 * send deauthentication frame to each STA.
			 * We only stop the Beacon & let all stations timeout.
			 */
			/* Send deauth. */
			authSendDeauthFrame(prAdapter,
			    prP2pBssInfo,
			    NULL, (struct SW_RFB *) NULL,
			    u2ReasonCode, (PFN_TX_DONE_HANDLER) NULL);

			prClientList = &prP2pBssInfo->rStaRecOfClientList;

			/* This case may let LINK_FOR_EACH_ENTRY_SAFE crash */
			if (prClientList == NULL)
				break;
			LINK_FOR_EACH_ENTRY_SAFE(prCurrStaRec, prStaRecNext,
				prClientList, rLinkEntry, struct STA_RECORD) {
				ASSERT(prCurrStaRec);
				p2pFuncDisconnect(prAdapter,
					prP2pBssInfo, prCurrStaRec,
					TRUE, u2ReasonCode);
			}
			break;
		default:
			return;	/* 20110420 -- alreay in Device Mode. */
		}

		/* Make the deauth frame send to FW ASAP. */
#if !CFG_SUPPORT_MULTITHREAD
		wlanAcquirePowerControl(prAdapter);
#endif
		wlanProcessCommandQueue(prAdapter,
			&prAdapter->prGlueInfo->rCmdQueue);
#if !CFG_SUPPORT_MULTITHREAD
		wlanReleasePowerControl(prAdapter);
#endif

		/* Change Connection Status. */
		/* 20161025, can not set DISCONNECTED if clientcount > 0 */
		/*p2pChangeMediaState(prAdapter,
		 * prP2pBssInfo, MEDIA_STATE_DISCONNECTED);
		 */

	} while (FALSE);
}				/* p2pFuncDissolve */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function is called to dissolve from group or one group.
 *           (Would not change P2P FSM.)
 *              1. GC: Disconnect from AP. (Send Deauth)
 *               2. GO: Disconnect all STA
 *
 * @param[in] prAdapter   Pointer to the adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void
p2pFuncDisconnect(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN struct STA_RECORD *prStaRec,
		IN u_int8_t fgSendDeauth, IN uint16_t u2ReasonCode)
{
	enum ENUM_PARAM_MEDIA_STATE eOriMediaStatus;
#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode = NULL;
#endif

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prStaRec != NULL) && (prP2pBssInfo != NULL));

		ASSERT_BREAK(prP2pBssInfo->eNetworkType == NETWORK_TYPE_P2P);

		ASSERT_BREAK(prP2pBssInfo->ucBssIndex
			< prAdapter->ucP2PDevBssIdx);

		eOriMediaStatus = prP2pBssInfo->eConnectionState;

		/* Indicate disconnect. */
		if (prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
			struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
			    P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
					prP2pBssInfo->u4PrivateData);

#if CFG_TC10_FEATURE
			/* Store last sta deauth reason */
			prP2pBssInfo->u2DeauthReason = u2ReasonCode;
#endif
			kalP2PGOStationUpdate(prAdapter->prGlueInfo,
				prP2pRoleFsmInfo->ucRoleIndex, prStaRec, FALSE);

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
			prTWTHotspotStaNode = prStaRec->prTWTHotspotStaNode;

			if (prTWTHotspotStaNode != NULL) {
				/* Teardown this STA*/
				/* 1. send teardown command to F/W */
				/* 2. reset this STA station record */
				twtHotspotRespFsmSteps(
					prAdapter,
					prStaRec,
					TWT_HOTSPOT_RESP_STATE_DISCONNECT,
					prTWTHotspotStaNode->flow_id,
					NULL);
			}
#endif
		} else {
			struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
			    P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
					prP2pBssInfo->u4PrivateData);

			prP2pRoleFsmInfo->rJoinInfo.prTargetBssDesc = NULL;

			scanRemoveConnFlagOfBssDescByBssid(prAdapter,
				prP2pBssInfo->aucBSSID);
		}

		DBGLOG(P2P, INFO,
			"p2pFuncDisconnect(): BssMode: %d, reason: %d, SendDeauth %s\n",
			prP2pBssInfo->eCurrentOPMode, u2ReasonCode,
			fgSendDeauth == TRUE ? "TRUE" : "FALSE");

		if (fgSendDeauth) {
			/* Send deauth. */
			authSendDeauthFrame(prAdapter,
			    prP2pBssInfo,
			    prStaRec,
			    (struct SW_RFB *) NULL,
			    u2ReasonCode,
			    (PFN_TX_DONE_HANDLER)
			    p2pRoleFsmRunEventDeauthTxDone);

			/* Make the deauth frame send to FW ASAP. */
#if !CFG_SUPPORT_MULTITHREAD
			wlanAcquirePowerControl(prAdapter);
#endif
			wlanProcessCommandQueue(prAdapter,
				&prAdapter->prGlueInfo->rCmdQueue);
#if !CFG_SUPPORT_MULTITHREAD
			wlanReleasePowerControl(prAdapter);
#endif
		} else {
			/* Change station state. */
			cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

			/* Reset Station Record Status. */
			p2pFuncResetStaRecStatus(prAdapter, prStaRec);

			cnmStaRecFree(prAdapter, prStaRec);

			if ((prP2pBssInfo->eCurrentOPMode
				!= OP_MODE_ACCESS_POINT) ||
			    (bssGetClientCount(prAdapter, prP2pBssInfo) == 0)) {
				DBGLOG(P2P, TRACE,
					"No More Client, Media Status DISCONNECTED\n");
				p2pChangeMediaState(prAdapter,
					prP2pBssInfo,
					MEDIA_STATE_DISCONNECTED);
			}

			if (eOriMediaStatus != prP2pBssInfo->eConnectionState) {
				/* Update Disconnected state to FW. */
				nicUpdateBss(prAdapter,
					prP2pBssInfo->ucBssIndex);
			}

		}
	} while (FALSE);

	return;

}				/* p2pFuncDisconnect */

void p2pFuncSetChannel(IN struct ADAPTER *prAdapter,
		IN uint8_t ucRoleIdx,
		IN struct RF_CHANNEL_INFO *prRfChannelInfo)
{
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo =
		(struct P2P_CONNECTION_REQ_INFO *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prRfChannelInfo != NULL));

		prP2pRoleFsmInfo =
			P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx);
		if (!prP2pRoleFsmInfo)
			break;
		prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);

		prP2pConnReqInfo->rChannelInfo.ucChannelNum =
			prRfChannelInfo->ucChannelNum;
		prP2pConnReqInfo->rChannelInfo.eBand = prRfChannelInfo->eBand;
		prP2pConnReqInfo->eChnlBw = prRfChannelInfo->ucChnlBw;
		prP2pConnReqInfo->u2PriChnlFreq =
			prRfChannelInfo->u2PriChnlFreq;
		prP2pConnReqInfo->u4CenterFreq1 =
			prRfChannelInfo->u4CenterFreq1;
		prP2pConnReqInfo->u4CenterFreq2 =
			prRfChannelInfo->u4CenterFreq2;
#if CFG_AP_80211KVR_INTERFACE
		/* Update TX-pwr as soon as channel changed */
		{
			struct PARAM_CUSTOM_GET_TX_POWER rGetTxPower;
			struct BSS_INFO *prP2pRoleBssInfo =
				(struct BSS_INFO *) NULL;
			uint32_t rStatus = WLAN_STATUS_SUCCESS;
			uint32_t u4BufLen = 0;

			prP2pRoleBssInfo =
				GET_BSS_INFO_BY_INDEX(prAdapter,
				prP2pRoleFsmInfo->ucBssIndex);
			kalMemZero(&rGetTxPower,
				sizeof(struct PARAM_CUSTOM_GET_TX_POWER));
			rGetTxPower.ucCenterChannel =
				prP2pRoleBssInfo->ucPrimaryChannel;
			rGetTxPower.ucBand = prP2pRoleBssInfo->eBand;
			rGetTxPower.ucDbdcIdx = ENUM_BAND_0;

			rStatus = kalIoctl(prAdapter->prGlueInfo,
				wlanoidQueryGetTxPower,
				&rGetTxPower,
				sizeof(struct PARAM_CUSTOM_GET_TX_POWER),
				TRUE, TRUE, TRUE, &u4BufLen);

			if (rStatus != WLAN_STATUS_SUCCESS)
				DBGLOG(OID, ERROR,
					"ERR: Get TxPower fail (%x)\r\n",
					rStatus);
		}
#endif /* CFG_AP_80211KVR_INTERFACE */

	} while (FALSE);
}				/* p2pFuncSetChannel */

/*---------------------------------------------------------------------------*/
/*!
 * @brief Retry JOIN for AUTH_MODE_AUTO_SWITCH
 *
 * @param[in] prStaRec       Pointer to the STA_RECORD_T
 *
 * @retval TRUE      We will retry JOIN
 * @retval FALSE     We will not retry JOIN
 */
/*---------------------------------------------------------------------------*/
u_int8_t p2pFuncRetryJOIN(IN struct ADAPTER *prAdapter,
		IN struct STA_RECORD *prStaRec,
		IN struct P2P_JOIN_INFO *prJoinInfo)
{
	struct MSG_SAA_FSM_START *prJoinReqMsg =
		(struct MSG_SAA_FSM_START *) NULL;
	u_int8_t fgRetValue = FALSE;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prStaRec != NULL)
			&& (prJoinInfo != NULL));

		/* Retry other AuthType if possible */
		if (!prJoinInfo->ucAvailableAuthTypes)
			break;

		if (prJoinInfo->ucAvailableAuthTypes
			& (uint8_t) AUTH_TYPE_SHARED_KEY) {

			DBGLOG(P2P, INFO,
				"RETRY JOIN INIT: Retry Authentication with AuthType == SHARED_KEY.\n");

			prJoinInfo->ucAvailableAuthTypes &=
				~(uint8_t) AUTH_TYPE_SHARED_KEY;

			prStaRec->ucAuthAlgNum =
				(uint8_t) AUTH_ALGORITHM_NUM_SHARED_KEY;
		} else {
			DBGLOG(P2P, ERROR,
				"RETRY JOIN INIT: Retry Authentication with Unexpected AuthType.\n");
			ASSERT(0);
			break;
		}

		/* No more available Auth Types */
		prJoinInfo->ucAvailableAuthTypes = 0;

		/* Trigger SAA to start JOIN process. */
		prJoinReqMsg = (struct MSG_SAA_FSM_START *)
			cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			sizeof(struct MSG_SAA_FSM_START));
		if (!prJoinReqMsg) {
			break;
		}

		prJoinReqMsg->rMsgHdr.eMsgId = MID_P2P_SAA_FSM_START;
		prJoinReqMsg->ucSeqNum = ++prJoinInfo->ucSeqNumOfReqMsg;
		prJoinReqMsg->prStaRec = prStaRec;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prJoinReqMsg,
			MSG_SEND_METHOD_BUF);

		fgRetValue = TRUE;
	} while (FALSE);

	return fgRetValue;

}				/* end of p2pFuncRetryJOIN() */

struct BSS_INFO *p2pFuncBSSIDFindBssInfo(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucBSSID)
{
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	uint8_t ucBssIdx = 0;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (pucBSSID != NULL));

		for (ucBssIdx = 0;
			ucBssIdx < prAdapter->ucHwBssIdNum; ucBssIdx++) {
			if (!IS_NET_ACTIVE(prAdapter, ucBssIdx))
				continue;

			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

			if (EQUAL_MAC_ADDR(prBssInfo->aucBSSID, pucBSSID)
				&& IS_BSS_P2P(prBssInfo))
				break;

			prBssInfo = NULL;
		}

	} while (FALSE);

	return prBssInfo;
}				/* p2pFuncBSSIDFindBssInfo */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Auth Frame and then return
 *        the status code to AAA to indicate
 *        if need to perform following actions
 *        when the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[in] pprStaRec          Pointer to pointer of STA_RECORD_T structure.
 * @param[out] pu2StatusCode     The Status Code of Validation Result
 *
 * @retval TRUE      Reply the Auth
 * @retval FALSE     Don't reply the Auth
 */
/*---------------------------------------------------------------------------*/
u_int8_t
p2pFuncValidateAuth(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN struct SW_RFB *prSwRfb,
		IN struct STA_RECORD **pprStaRec,
		OUT uint16_t *pu2StatusCode)
{
	u_int8_t fgPmfConn = FALSE;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;
	struct WLAN_AUTH_FRAME *prAuthFrame = (struct WLAN_AUTH_FRAME *) NULL;

	DBGLOG(P2P, TRACE, "p2pValidate Authentication Frame\n");


	/* P2P 3.2.8 */
	*pu2StatusCode = STATUS_CODE_REQ_DECLINED;
	prAuthFrame = (struct WLAN_AUTH_FRAME *) prSwRfb->pvHeader;

	if ((prP2pBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) ||
	    (prP2pBssInfo->eIntendOPMode != OP_MODE_NUM)) {
		/* We are not under AP Mode yet. */
		DBGLOG(P2P, WARN,
			"Current OP mode is not under AP mode. (%d)\n",
			prP2pBssInfo->eCurrentOPMode);
		return FALSE;
	}

	prStaRec = cnmGetStaRecByAddress(prAdapter,
		prP2pBssInfo->ucBssIndex, prAuthFrame->aucSrcAddr);

	if (!prStaRec) {
		prStaRec = cnmStaRecAlloc(prAdapter, STA_TYPE_P2P_GC,
			prP2pBssInfo->ucBssIndex,
			prAuthFrame->aucSrcAddr);

		/* TODO(Kevin): Error handling of allocation of
		 * struct STA_RECORD for
		 * exhausted case and do removal of unused struct STA_RECORD.
		 */
		/* Sent a message event to clean un-used STA_RECORD_T. */
		/* ASSERT(prStaRec); */
		if (!prStaRec) {
			DBGLOG(P2P, WARN,
				"StaRec Full. (%d)\n", CFG_STA_REC_NUM);
			return TRUE;
		}

		prSwRfb->ucStaRecIdx = prStaRec->ucIndex;

		prStaRec->u2BSSBasicRateSet = prP2pBssInfo->u2BSSBasicRateSet;

		prStaRec->u2DesiredNonHTRateSet = RATE_SET_ERP_P2P;

		prStaRec->u2OperationalRateSet = RATE_SET_ERP_P2P;
		prStaRec->ucPhyTypeSet = PHY_TYPE_SET_802_11GN;

		/* Update default Tx rate */
		nicTxUpdateStaRecDefaultRate(prAdapter, prStaRec);

		/* NOTE(Kevin): Better to change state here, not at TX Done */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);
	} else {
#if CFG_SUPPORT_802_11W
		/* AP PMF. if PMF connection, do not reset state & FSM */
		fgPmfConn = rsnCheckBipKeyInstalled(prAdapter, prStaRec);
		if (fgPmfConn) {
			DBGLOG(P2P, WARN, "PMF Connction, return false\n");
			return FALSE;
		}
#endif

		prSwRfb->ucStaRecIdx = prStaRec->ucIndex;

		if ((prStaRec->ucStaState > STA_STATE_1)
			&& (IS_STA_IN_P2P(prStaRec))) {

			cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

			p2pFuncResetStaRecStatus(prAdapter, prStaRec);

			bssRemoveClient(prAdapter, prP2pBssInfo, prStaRec);

			p2pFuncDisconnect(prAdapter,
				prP2pBssInfo, prStaRec, FALSE,
				REASON_CODE_DISASSOC_INACTIVITY);
		}

	}

	if (bssGetClientCount(prAdapter, prP2pBssInfo)
		>= P2P_MAXIMUM_CLIENT_COUNT
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
		|| kalP2PMaxClients(prAdapter->prGlueInfo,
		bssGetClientCount(prAdapter, prP2pBssInfo),
		(uint8_t) prP2pBssInfo->u4PrivateData)
#endif
	) {
		/* GROUP limit full. */
		/* P2P 3.2.8 */
		DBGLOG(P2P, WARN,
			"Group Limit Full. (%d)\n",
			bssGetClientCount(prAdapter, prP2pBssInfo));
		cnmStaRecFree(prAdapter, prStaRec);
		*pu2StatusCode = STATUS_CODE_ASSOC_DENIED_AP_OVERLOAD;
		return TRUE;
	}
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
	else {
		/* Hotspot Blacklist */
		if (kalP2PCmpBlackList(prAdapter->prGlueInfo,
			prAuthFrame->aucSrcAddr,
			(uint8_t) prP2pBssInfo->u4PrivateData)
			|| !p2pRoleProcessACLInspection(prAdapter,
			prStaRec->aucMacAddr, prP2pBssInfo->ucBssIndex)) {
			DBGLOG(P2P, WARN, "in black list.\n");
			cnmStaRecFree(prAdapter, prStaRec);
			*pu2StatusCode
				= STATUS_CODE_ASSOC_DENIED_OUTSIDE_STANDARD;
			return FALSE;
		}

	}
#endif
	/* prStaRec->eStaType = STA_TYPE_INFRA_CLIENT; */
	prStaRec->eStaType = STA_TYPE_P2P_GC;

	/* Update Station Record - Status/Reason Code */
	prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;

	prStaRec->ucJoinFailureCount = 0;

	*pprStaRec = prStaRec;

	*pu2StatusCode = STATUS_CODE_SUCCESSFUL;


	return TRUE;

}				/* p2pFuncValidateAuth */

void p2pFuncResetStaRecStatus(IN struct ADAPTER *prAdapter,
		IN struct STA_RECORD *prStaRec)
{
	do {
		if ((prAdapter == NULL) || (prStaRec == NULL)) {
			break;
		}

		prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;
		prStaRec->u2ReasonCode = REASON_CODE_RESERVED;
		prStaRec->ucJoinFailureCount = 0;
		prStaRec->fgTransmitKeyExist = FALSE;

		prStaRec->fgSetPwrMgtBit = FALSE;

	} while (FALSE);
}				/* p2pFuncResetStaRecStatus */

/*---------------------------------------------------------------------------*/
/*!
 * @brief The function is used to initialize the value
 *           of the connection settings for P2P network
 *
 * @param (none)
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void
p2pFuncInitConnectionSettings(IN struct ADAPTER *prAdapter,
		IN struct P2P_CONNECTION_SETTINGS *prP2PConnSettings,
		IN u_int8_t fgIsApMode)
{
	struct WIFI_VAR *prWifiVar = NULL;

	ASSERT(prP2PConnSettings);

	prWifiVar = &(prAdapter->rWifiVar);
	ASSERT(prWifiVar);

	prP2PConnSettings->fgIsApMode = fgIsApMode;

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
	prP2PConnSettings->fgIsWPSMode = prWifiVar->ucApWpsMode;
#endif
}				/* p2pFuncInitConnectionSettings */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Assoc Req Frame and then return
 *        the status code to AAA to indicate if need
 *        to perform following actions
 *        when the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[out] pu2StatusCode     The Status Code of Validation Result
 *
 * @retval TRUE      Reply the Assoc Resp
 * @retval FALSE     Don't reply the Assoc Resp
 */
/*---------------------------------------------------------------------------*/
u_int8_t p2pFuncValidateAssocReq(IN struct ADAPTER *prAdapter,
		IN struct SW_RFB *prSwRfb,
		OUT uint16_t *pu2StatusCode)
{
	u_int8_t fgReplyAssocResp = TRUE;
	struct WLAN_ASSOC_REQ_FRAME *prAssocReqFrame =
		(struct WLAN_ASSOC_REQ_FRAME *) NULL;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;

	/* TODO(Kevin): Call P2P functions to check ..
	 *  2. Check we can accept connection from thsi peer
	 *  a. If we are in PROVISION state,
	 *      only accept the peer we do the GO formation previously.
	 *  b. If we are in OPERATION state, only accept
	 *      the other peer when P2P_GROUP_LIMIT is 0.
	 *  3. Check Black List here.
	 */

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prSwRfb != NULL) && (pu2StatusCode != NULL));

		*pu2StatusCode = STATUS_CODE_REQ_DECLINED;
		prAssocReqFrame =
			(struct WLAN_ASSOC_REQ_FRAME *) prSwRfb->pvHeader;

		prP2pBssInfo =
			p2pFuncBSSIDFindBssInfo(prAdapter,
				prAssocReqFrame->aucBSSID);

		if (prP2pBssInfo == NULL) {
			DBGLOG(P2P, ERROR,
				"RX ASSOC frame without BSS active / BSSID match\n");
			break;
		}

		prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);

		if (prStaRec == NULL) {
			/* Station record should be ready
			 * while RX AUTH frame.
			 */
			fgReplyAssocResp = FALSE;
			break;
		}
		ASSERT(prSwRfb->prRxStatusGroup3);
		prStaRec->ucRCPI =
			nicRxGetRcpiValueFromRxv(
				prAdapter, RCPI_MODE_MAX, prSwRfb);

		prStaRec->u2DesiredNonHTRateSet &=
			prP2pBssInfo->u2OperationalRateSet;
		prStaRec->ucDesiredPhyTypeSet =
			prStaRec->ucPhyTypeSet & prP2pBssInfo->ucPhyTypeSet;

		if (prStaRec->ucDesiredPhyTypeSet == 0) {
			/* The station only support 11B rate. */
			*pu2StatusCode =
				STATUS_CODE_ASSOC_DENIED_RATE_NOT_SUPPORTED;
			break;
		}

		*pu2StatusCode = STATUS_CODE_SUCCESSFUL;

	} while (FALSE);

	return fgReplyAssocResp;

}				/* p2pFuncValidateAssocReq */

/*---------------------------------------------------------------------------*/
/*!
* @brief This function is used to check the TKIP IE
*
*
* @return none
*/
/*----------------------------------------------------------------------------*/
u_int8_t p2pFuncParseCheckForTKIPInfoElem(IN uint8_t *pucBuf)
{
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA;
	struct WPA_INFO_ELEM *prWpaIE = (struct WPA_INFO_ELEM *) NULL;
	uint32_t u4GroupKeyCipher = 0;

	if (pucBuf == NULL)
		return FALSE;

	prWpaIE = (struct WPA_INFO_ELEM *) pucBuf;

	if (prWpaIE->ucLength <= ELEM_MIN_LEN_WFA_OUI_TYPE_SUBTYPE)
		return FALSE;

	if (kalMemCmp(prWpaIE->aucOui, aucWfaOui, sizeof(aucWfaOui)))
		return FALSE;

	WLAN_GET_FIELD_32(&prWpaIE->u4GroupKeyCipherSuite, &u4GroupKeyCipher);

	if (prWpaIE->ucOuiType == VENDOR_OUI_TYPE_WPA &&
		u4GroupKeyCipher == WPA_CIPHER_SUITE_TKIP)
		return TRUE;
	else
		return FALSE;
}				/* p2pFuncParseCheckForP2PInfoElem */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function is used to check the P2P IE
 *
 *
 * @return none
 */
/*---------------------------------------------------------------------------*/
u_int8_t p2pFuncParseCheckForP2PInfoElem(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucBuf, OUT uint8_t *pucOuiType)
{
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;
	struct IE_WFA *prWfaIE = (struct IE_WFA *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (pucBuf != NULL) && (pucOuiType != NULL));

		prWfaIE = (struct IE_WFA *) pucBuf;

		if (IE_LEN(pucBuf) <= ELEM_MIN_LEN_WFA_OUI_TYPE_SUBTYPE) {
			break;
		} else if (prWfaIE->aucOui[0] != aucWfaOui[0] ||
			   prWfaIE->aucOui[1] != aucWfaOui[1] ||
			   prWfaIE->aucOui[2] != aucWfaOui[2]) {
			break;
		}

		*pucOuiType = prWfaIE->ucOuiType;

		return TRUE;
	} while (FALSE);

	return FALSE;
}				/* p2pFuncParseCheckForP2PInfoElem */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Probe Request Frame and then return
 *        result to BSS to indicate if need to send
 *        the corresponding Probe Response Frame
 *        if the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[out] pu4ControlFlags   Control flags for replying the Probe Response
 *
 * @retval TRUE      Reply the Probe Response
 * @retval FALSE     Don't reply the Probe Response
 */
/*---------------------------------------------------------------------------*/
u_int8_t
p2pFuncValidateProbeReq(IN struct ADAPTER *prAdapter,
		IN struct SW_RFB *prSwRfb,
		OUT uint32_t *pu4ControlFlags,
		IN u_int8_t fgIsDevInterface,
		IN uint8_t ucRoleIdx)
{
	u_int8_t fgIsReplyProbeRsp = FALSE;
	u_int8_t fgApplyp2PDevFilter = FALSE;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;

	DEBUGFUNC("p2pFuncValidateProbeReq");

	do {

		ASSERT_BREAK((prAdapter != NULL) && (prSwRfb != NULL));

		prP2pRoleFsmInfo =
			prAdapter->rWifiVar.aprP2pRoleFsmInfo[ucRoleIdx];

		/* Process both cases that with amd without add p2p interface */
		if (fgIsDevInterface)
			fgApplyp2PDevFilter = TRUE;
		else {
			if (prAdapter->prGlueInfo->prP2PInfo[0]->prDevHandler ==
				prAdapter->prGlueInfo->prP2PInfo
					[ucRoleIdx]->aprRoleHandler)
				fgApplyp2PDevFilter = TRUE;
			else
				fgApplyp2PDevFilter = FALSE;
		}
		/* TODO: */
		if ((fgApplyp2PDevFilter &&
			(prAdapter->u4OsPacketFilter
			& PARAM_PACKET_FILTER_PROBE_REQ))
			|| (!fgApplyp2PDevFilter &&
			(prP2pRoleFsmInfo->u4P2pPacketFilter
			& PARAM_PACKET_FILTER_PROBE_REQ))) {
			/* Leave the probe response to p2p_supplicant. */
			kalP2PIndicateRxMgmtFrame(prAdapter,
				prAdapter->prGlueInfo,
				prSwRfb, fgIsDevInterface, ucRoleIdx);
		}

	} while (FALSE);

	return fgIsReplyProbeRsp;

}				/* end of p2pFuncValidateProbeReq() */

static void
p2pFunAbortOngoingScan(IN struct ADAPTER *prAdapter)
{
	struct SCAN_INFO *prScanInfo;

	if (!prAdapter)
		return;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (!prScanInfo || (prScanInfo->eCurrentState != SCAN_STATE_SCANNING))
		return;

	if (IS_BSS_INDEX_AIS(prAdapter,
		prScanInfo->rScanParam.ucBssIndex))
		aisFsmStateAbort_SCAN(prAdapter,
			prScanInfo->rScanParam.ucBssIndex);
}

static void p2pFunBufferP2pActionFrame(IN struct ADAPTER *prAdapter,
		IN struct SW_RFB *prSwRfb,
		IN uint8_t ucRoleIdx)
{
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo =
			(struct P2P_DEV_FSM_INFO *) NULL;
	struct P2P_QUEUED_ACTION_FRAME *prFrame;

	prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;

	if (prP2pDevFsmInfo == NULL)
		return;

	prFrame = &prP2pDevFsmInfo->rQueuedActionFrame;

	if (prFrame->u2Length > 0) {
		DBGLOG(P2P, WARN, "p2p action frames are pending, drop it.\n");
		return;
	}

	DBGLOG(P2P, INFO, "Buffer the p2p action frame.\n");
	prFrame->ucRoleIdx = ucRoleIdx;
	prFrame->u4Freq = nicChannelNum2Freq(
		prSwRfb->ucChnlNum, BAND_NULL) / 1000;
	prFrame->u2Length = prSwRfb->u2PacketLen;
	prFrame->prHeader = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
			prSwRfb->u2PacketLen);
	if (prFrame->prHeader == NULL) {
		DBGLOG(P2P, WARN, "Allocate buffer fail.\n");
		p2pFunCleanQueuedMgmtFrame(prAdapter, prFrame);
		return;
	}
	kalMemCopy(prFrame->prHeader, prSwRfb->pvHeader, prSwRfb->u2PacketLen);
}

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Probe Request Frame and then return
 *        result to BSS to indicate if need to send
 *        the corresponding Probe Response
 *        Frame if the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[out] pu4ControlFlags   Control flags for replying the Probe Response
 *
 * @retval TRUE      Reply the Probe Response
 * @retval FALSE     Don't reply the Probe Response
 */
/*--------------------------------------------------------------------------*/
void p2pFuncValidateRxActionFrame(IN struct ADAPTER *prAdapter,
		IN struct SW_RFB *prSwRfb, IN u_int8_t fgIsDevInterface,
					IN uint8_t ucRoleIdx)
{
	struct WLAN_ACTION_FRAME *prActFrame;
	struct WLAN_PUBLIC_VENDOR_ACTION_FRAME *prActPubVenFrame;
	uint32_t u4OUI;
	u_int8_t fgBufferFrame = FALSE;

	DEBUGFUNC("p2pFuncValidateRxActionFrame");

	if (prAdapter == NULL || prSwRfb == NULL) {
		DBGLOG(P2P, ERROR, "Invalid parameter.\n");
		return;
	}
	prActFrame = (struct WLAN_ACTION_FRAME *) prSwRfb->pvHeader;

	switch (prActFrame->ucCategory) {
	case CATEGORY_PUBLIC_ACTION:
		if (prActFrame->ucAction != 0x9 ||
				prSwRfb->u2PacketLen <
				sizeof(struct WLAN_PUBLIC_VENDOR_ACTION_FRAME))
			break;
		WLAN_GET_FIELD_BE32(prActFrame->ucActionDetails, &u4OUI);
		DBGLOG(P2P, TRACE, "Action: oui: 0x%x\n", u4OUI);
		if (u4OUI != P2P_IE_VENDOR_TYPE)
			break;

		prActPubVenFrame =
				(struct WLAN_PUBLIC_VENDOR_ACTION_FRAME *)
				prActFrame;
		p2pProcessActionResponse(prAdapter,
				prActPubVenFrame->ucPubSubType);
		if (prActPubVenFrame->ucPubSubType == P2P_GO_NEG_REQ)
			p2pFunAbortOngoingScan(prAdapter);
		if (fgIsDevInterface) {
			p2pDevFsmNotifyP2pRx(prAdapter,
					prActPubVenFrame->ucPubSubType,
					&fgBufferFrame);
		}
		/* Fall through */
	default:
		break;
	}

	if (fgBufferFrame) {
		p2pFunBufferP2pActionFrame(prAdapter,
				prSwRfb,
				ucRoleIdx);
		return;
	}

	if (prAdapter->u4OsPacketFilter
		& PARAM_PACKET_FILTER_ACTION_FRAME) {
		/* Leave the Action frame to p2p_supplicant. */
		kalP2PIndicateRxMgmtFrame(prAdapter,
			prAdapter->prGlueInfo,
			prSwRfb, fgIsDevInterface, ucRoleIdx);
	} else {
		DBGLOG(P2P, INFO,
			"do not indicate action frame as filter closed\n");
	}

	return;

}				/* p2pFuncValidateRxMgmtFrame */

u_int8_t p2pFuncIsAPMode(IN struct P2P_CONNECTION_SETTINGS *prP2pConnSettings)
{
	if (prP2pConnSettings) {
		if (prP2pConnSettings->fgIsWPSMode == 1)
			return FALSE;
		return prP2pConnSettings->fgIsApMode;
	} else {
		return FALSE;
	}
}

/* p2pFuncIsAPMode */

void
p2pFuncParseBeaconContent(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN uint8_t *pucIEInfo, IN uint32_t u4IELen)
{
	uint8_t *pucIE = (uint8_t *) NULL;
	uint16_t u2Offset = 0;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	uint8_t i = 0;
	struct RSN_INFO rRsnIe;

	kalMemZero(&rRsnIe, sizeof(struct RSN_INFO));

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL));

		if (u4IELen == 0)
			break;

		prP2pSpecificBssInfo =
			prAdapter->rWifiVar.prP2pSpecificBssInfo
				[prP2pBssInfo->u4PrivateData];
		prP2pSpecificBssInfo->u2AttributeLen = 0;
		prP2pSpecificBssInfo->u2WpaIeLen = 0;
		prP2pSpecificBssInfo->u2RsnIeLen = 0;

		ASSERT_BREAK(pucIEInfo != NULL);

		pucIE = pucIEInfo;

		if (prP2pBssInfo->u2CapInfo & CAP_INFO_PRIVACY)
			kalP2PSetCipher(prAdapter->prGlueInfo,
				IW_AUTH_CIPHER_WEP40,
				(uint8_t) prP2pBssInfo->u4PrivateData);
		else
			kalP2PSetCipher(prAdapter->prGlueInfo,
				IW_AUTH_CIPHER_NONE,
				(uint8_t) prP2pBssInfo->u4PrivateData);

		prP2pBssInfo->ucCountryIELen = 0;

		IE_FOR_EACH(pucIE, u4IELen, u2Offset) {
			switch (IE_ID(pucIE)) {
			case ELEM_ID_SSID:	/* 0 *//* V *//* Done */
				{

				/* DBGLOG(P2P, TRACE, ("SSID update\n")); */
				/* SSID is saved when start AP/GO */
				/* SSID IE set in beacon from supplicant
				 * will not always be
				 * the true since hidden SSID case
				 */
#if CFG_SUPPORT_HIDDEN_SW_AP
					COPY_SSID(
					prP2pBssInfo->aucSSID,
					prP2pBssInfo->ucSSIDLen,
					SSID_IE(pucIE)->aucSSID,
					SSID_IE(pucIE)->ucLength);

					COPY_SSID(
					prP2pSpecificBssInfo->aucGroupSsid,
					prP2pSpecificBssInfo->u2GroupSsidLen,
					SSID_IE(pucIE)->aucSSID,
					SSID_IE(pucIE)->ucLength);
#endif

				}
				break;
			case ELEM_ID_SUP_RATES:	/* 1 *//* V *//* Done */
#ifndef CFG_SUPPORT_P2P_GO_KEEP_RATE_SETTING
				{
					DBGLOG(P2P, TRACE, "Support Rate IE\n");
					if ((SUP_RATES_IE(pucIE)->ucLength)
						> ELEM_MAX_LEN_SUP_RATES)
						SUP_RATES_IE(pucIE)->ucLength =
							ELEM_MAX_LEN_SUP_RATES;
					kalMemCopy(
					prP2pBssInfo->aucAllSupportedRates,
					SUP_RATES_IE(pucIE)->aucSupportedRates,
					SUP_RATES_IE(pucIE)->ucLength);
					prP2pBssInfo->ucAllSupportedRatesLen =
					SUP_RATES_IE(pucIE)->ucLength;
					DBGLOG_MEM8(P2P, TRACE,
					SUP_RATES_IE(pucIE)->aucSupportedRates,
					SUP_RATES_IE(pucIE)->ucLength);
				}
#endif
				break;
			case ELEM_ID_DS_PARAM_SET:	/* 3 *//* V *//* Done */
				{
					DBGLOG(P2P, TRACE,
						"DS PARAM IE: %d.\n",
						DS_PARAM_IE(pucIE)->ucCurrChnl);

					/* prP2pBssInfo->ucPrimaryChannel =
					 * DS_PARAM_IE(pucIE)->ucCurrChnl;
					 */

					/* prP2pBssInfo->eBand = BAND_2G4; */
				}
				break;
			case ELEM_ID_TIM:	/* 5 *//* V */
				TIM_IE(pucIE)->ucDTIMPeriod =
					prP2pBssInfo->ucDTIMPeriod;
				DBGLOG(P2P, TRACE,
					"TIM IE, Len:%d, DTIM:%d\n",
					IE_LEN(pucIE),
					TIM_IE(pucIE)->ucDTIMPeriod);
				break;
#if CFG_SUPPORT_802_11D
			case ELEM_ID_COUNTRY_INFO: /* 7 */
				if (COUNTRY_IE(pucIE)->ucLength
					>= ELEM_MIN_LEN_COUNTRY_INFO) {
					uint8_t ucTripletLen, ucMaxTripletLen;

					prP2pBssInfo->ucCountryIELen =
						COUNTRY_IE(pucIE)->ucLength;
					ucTripletLen =
						COUNTRY_IE(pucIE)->ucLength - 3;
					ucMaxTripletLen =
						sizeof(prP2pBssInfo
							->aucSubbandTriplet) /
							sizeof(uint8_t);

					if (ucTripletLen > ucMaxTripletLen) {
						DBGLOG(P2P, ERROR,
							"COUNTRY IE length is unexpected !!\n");
						ucTripletLen = ucMaxTripletLen;
					}

					kalMemCopy(
					prP2pBssInfo->aucCountryStr,
					COUNTRY_IE(pucIE)->aucCountryStr, 3);
					kalMemCopy(
					prP2pBssInfo->aucSubbandTriplet,
					COUNTRY_IE(pucIE)->arCountryStr,
					ucTripletLen);
				}
				break;
#endif
			case ELEM_ID_ERP_INFO:	/* 42 *//* V */
				{
#if 1
					/* This IE would dynamic change due to
					 * FW detection change is required.
					 */
					DBGLOG(P2P, TRACE,
						"ERP IE will be over write by driver\n");
					DBGLOG(P2P, TRACE,
						"    ucERP: %x.\n",
						ERP_INFO_IE(pucIE)->ucERP);

					prP2pBssInfo->ucPhyTypeSet |=
							PHY_TYPE_SET_802_11G;
#else
					/* This IE would dynamic change due to
					 * FW detection change is required.
					 */
					DBGLOG(P2P, TRACE, "ERP IE.\n");

					prP2pBssInfo->ucPhyTypeSet |=
						PHY_TYPE_SET_802_11GN;

					ASSERT(prP2pBssInfo->eBand == BAND_2G4);

					prP2pBssInfo->fgObssErpProtectMode =
					    ((ERP_INFO_IE(pucIE)->ucERP
					    & ERP_INFO_USE_PROTECTION)
					    ? TRUE : FALSE);

					prP2pBssInfo->fgErpProtectMode =
					    ((ERP_INFO_IE(pucIE)->ucERP
					    & (ERP_INFO_USE_PROTECTION |
						ERP_INFO_NON_ERP_PRESENT))
						? TRUE : FALSE);
#endif

				}
				break;
			case ELEM_ID_HT_CAP:	/* 45 *//* V */
				{
#if 1
					DBGLOG(P2P, TRACE,
						"HT CAP IE would be overwritten by driver\n");

					DBGLOG(P2P, TRACE,
						"HT Cap Info:%x, AMPDU Param:%x\n",
						HT_CAP_IE(pucIE)->u2HtCapInfo,
						HT_CAP_IE(pucIE)->ucAmpduParam);

					DBGLOG(P2P, TRACE,
						"HT Extended Cap:%x, TX Beamforming Cap:%x, Ant Selection Cap:%x\n",
						HT_CAP_IE(pucIE)
							->u2HtExtendedCap,
						HT_CAP_IE(pucIE)
							->u4TxBeamformingCap,
						HT_CAP_IE(pucIE)->ucAselCap);

					prP2pBssInfo->ucPhyTypeSet |=
							PHY_TYPE_SET_802_11N;
#else
					prP2pBssInfo->ucPhyTypeSet |=
						PHY_TYPE_SET_802_11N;

					/* u2HtCapInfo */
					if ((HT_CAP_IE(pucIE)->u2HtCapInfo &
						(HT_CAP_INFO_SUP_CHNL_WIDTH |
						HT_CAP_INFO_SHORT_GI_40M |
						HT_CAP_INFO_DSSS_CCK_IN_40M))
						== 0) {
						prP2pBssInfo
							->fgAssoc40mBwAllowed =
								FALSE;
					} else {
						prP2pBssInfo
							->fgAssoc40mBwAllowed =
								TRUE;
					}

					if ((HT_CAP_IE(pucIE)->u2HtCapInfo &
						(HT_CAP_INFO_SHORT_GI_20M |
						HT_CAP_INFO_SHORT_GI_40M))
						== 0) {
						prAdapter->rWifiVar
							.rConnSettings
							.fgRxShortGIDisabled =
								TRUE;
					} else {
						prAdapter->rWifiVar
							.rConnSettings
							.fgRxShortGIDisabled =
								FALSE;
					}

					/* ucAmpduParam */
					DBGLOG(P2P, TRACE,
						"AMPDU setting from supplicant:0x%x, & default value:0x%x\n",
						(uint8_t)
						HT_CAP_IE(pucIE)->ucAmpduParam,
						(uint8_t)
						AMPDU_PARAM_DEFAULT_VAL);

					/* rSupMcsSet */
					/* Can do nothing.
					 * the field is default value
					 * from other configuration.
					 */
					/* HT_CAP_IE(pucIE)->rSupMcsSet; */

					/* u2HtExtendedCap */
					ASSERT(
					HT_CAP_IE(pucIE)->u2HtExtendedCap ==
				    (HT_EXT_CAP_DEFAULT_VAL &
					~(HT_EXT_CAP_PCO |
					HT_EXT_CAP_PCO_TRANS_TIME_NONE)));

					/* u4TxBeamformingCap */
					ASSERT(
					HT_CAP_IE(pucIE)->u4TxBeamformingCap
					== TX_BEAMFORMING_CAP_DEFAULT_VAL);

					/* ucAselCap */
					ASSERT(
					HT_CAP_IE(pucIE)->ucAselCap
					== ASEL_CAP_DEFAULT_VAL);
#endif
				}
				break;
			case ELEM_ID_RSN:	/* 48 *//* V */

				DBGLOG(P2P, TRACE, "RSN IE\n");
				kalP2PSetCipher(prAdapter->prGlueInfo,
					IW_AUTH_CIPHER_CCMP,
					(uint8_t) prP2pBssInfo->u4PrivateData);
				if (IE_LEN(pucIE) > ELEM_MAX_LEN_RSN) {
					DBGLOG(P2P, ERROR,
						"RSN IE length is unexpected !!\n");
					return;
				}
				kalMemCopy(prP2pSpecificBssInfo->aucRsnIeBuffer,
					pucIE, IE_SIZE(pucIE));
				prP2pSpecificBssInfo->u2RsnIeLen
					= IE_SIZE(pucIE);
				if (rsnParseRsnIE(prAdapter,
					RSN_IE(pucIE), &rRsnIe)) {
					prP2pBssInfo->u4RsnSelectedGroupCipher =
						RSN_CIPHER_SUITE_CCMP;
					prP2pBssInfo
						->u4RsnSelectedPairwiseCipher =
							RSN_CIPHER_SUITE_CCMP;
					prP2pBssInfo->u4RsnSelectedAKMSuite =
						RSN_AKM_SUITE_PSK;
					prP2pBssInfo->u2RsnSelectedCapInfo =
						rRsnIe.u2RsnCap;
					DBGLOG(RSN, TRACE,
						"RsnIe CAP:0x%x\n",
						rRsnIe.u2RsnCap);
				}

#if CFG_SUPPORT_802_11W
				/* AP PMF */
				prP2pBssInfo->rApPmfCfg.fgMfpc =
					(rRsnIe.u2RsnCap
						& ELEM_WPA_CAP_MFPC) ? 1 : 0;
				prP2pBssInfo->rApPmfCfg.fgMfpr =
					(rRsnIe.u2RsnCap
						& ELEM_WPA_CAP_MFPR) ? 1 : 0;
				prP2pSpecificBssInfo->u4KeyMgtSuiteCount
					= (rRsnIe.u4AuthKeyMgtSuiteCount
					< P2P_MAX_AKM_SUITES)
					? rRsnIe.u4AuthKeyMgtSuiteCount
					: P2P_MAX_AKM_SUITES;
				for (i = 0;
					i < rRsnIe.u4AuthKeyMgtSuiteCount;
					i++) {
					if ((rRsnIe.au4AuthKeyMgtSuite[i]
					== RSN_AKM_SUITE_PSK_SHA256) ||
					(rRsnIe.au4AuthKeyMgtSuite[i]
					== RSN_AKM_SUITE_802_1X_SHA256)) {
						DBGLOG(RSN, INFO,
							"SHA256 support\n");
						/* over-write
						 * u4RsnSelectedAKMSuite
						 * by SHA256 AKM
						 */
						prP2pBssInfo
						->u4RsnSelectedAKMSuite
						= rRsnIe.au4AuthKeyMgtSuite[i];
						prP2pBssInfo
						->rApPmfCfg.fgSha256
						= TRUE;
						break;
					} else if (rRsnIe.au4AuthKeyMgtSuite[i]
					== RSN_AKM_SUITE_SAE)
						prP2pBssInfo
						->u4RsnSelectedAKMSuite
						= rRsnIe.au4AuthKeyMgtSuite[i];

					if (i < P2P_MAX_AKM_SUITES) {
						prP2pSpecificBssInfo
						->au4KeyMgtSuite[i]
						= rRsnIe.au4AuthKeyMgtSuite[i];
					}
				}
				DBGLOG(RSN, ERROR,
					"bcn mfpc:%d, mfpr:%d, sha256:%d, 0x%04x\n",
					prP2pBssInfo->rApPmfCfg.fgMfpc,
					prP2pBssInfo->rApPmfCfg.fgMfpr,
					prP2pBssInfo->rApPmfCfg.fgSha256,
					prP2pBssInfo->u4RsnSelectedAKMSuite);
#endif

				break;
			case ELEM_ID_EXTENDED_SUP_RATES:	/* 50 *//* V */
#ifndef CFG_SUPPORT_P2P_GO_KEEP_RATE_SETTING
				/* ELEM_ID_SUP_RATES should be placed
				 * before ELEM_ID_EXTENDED_SUP_RATES.
				 */
				if ((EXT_SUP_RATES_IE(pucIE)->ucLength)
					> ELEM_MAX_LEN_SUP_RATES)
					EXT_SUP_RATES_IE(pucIE)->ucLength =
						ELEM_MAX_LEN_SUP_RATES;

				DBGLOG(P2P, TRACE, "Ex Support Rate IE\n");
				kalMemCopy(&
					(prP2pBssInfo->aucAllSupportedRates
					[prP2pBssInfo->ucAllSupportedRatesLen]),
					EXT_SUP_RATES_IE(pucIE)
						->aucExtSupportedRates,
					EXT_SUP_RATES_IE(pucIE)->ucLength);

				DBGLOG_MEM8(P2P, TRACE,
					EXT_SUP_RATES_IE(pucIE)
					->aucExtSupportedRates,
					EXT_SUP_RATES_IE(pucIE)
					->ucLength);

				prP2pBssInfo->ucAllSupportedRatesLen +=
					EXT_SUP_RATES_IE(pucIE)->ucLength;
#endif
				break;
			case ELEM_ID_HT_OP:
				/* 61 *//* V *//* TODO: */
				{
#if 1
					DBGLOG(P2P, TRACE,
						"HT OP IE would be overwritten by driver\n");

					DBGLOG(P2P, TRACE,
						"    Primary Channel: %x, Info1: %x, Info2: %x, Info3: %x\n",
						HT_OP_IE(pucIE)
							->ucPrimaryChannel,
						HT_OP_IE(pucIE)->ucInfo1,
						HT_OP_IE(pucIE)->u2Info2,
						HT_OP_IE(pucIE)->u2Info3);

					prP2pBssInfo->ucPhyTypeSet |=
							PHY_TYPE_SET_802_11N;
#else
					uint16_t u2Info2 = 0;

					prP2pBssInfo->ucPhyTypeSet |=
						PHY_TYPE_SET_802_11N;

					DBGLOG(P2P, TRACE, "HT OP IE\n");

					/* ucPrimaryChannel. */
					ASSERT(
					HT_OP_IE(pucIE)->ucPrimaryChannel
					== prP2pBssInfo->ucPrimaryChannel);

					/* ucInfo1 */
					prP2pBssInfo->ucHtOpInfo1 =
						HT_OP_IE(pucIE)->ucInfo1;

					/* u2Info2 */
					u2Info2 = HT_OP_IE(pucIE)->u2Info2;

					if (u2Info2
					& HT_OP_INFO2_NON_GF_HT_STA_PRESENT) {
					ASSERT(
					prP2pBssInfo->eGfOperationMode
					!= GF_MODE_NORMAL);
					u2Info2 &=
					~HT_OP_INFO2_NON_GF_HT_STA_PRESENT;
					}

					if (u2Info2
					& HT_OP_INFO2_OBSS_NON_HT_STA_PRESENT) {
					prP2pBssInfo->eObssHtProtectMode =
					HT_PROTECT_MODE_NON_MEMBER;
					u2Info2 &=
					~HT_OP_INFO2_OBSS_NON_HT_STA_PRESENT;
					}

					switch (u2Info2
						& HT_OP_INFO2_HT_PROTECTION) {
					case HT_PROTECT_MODE_NON_HT:
						prP2pBssInfo->eHtProtectMode =
							HT_PROTECT_MODE_NON_HT;
						break;
					case HT_PROTECT_MODE_NON_MEMBER:
						prP2pBssInfo->eHtProtectMode =
							HT_PROTECT_MODE_NONE;
						prP2pBssInfo
						->eObssHtProtectMode =
						HT_PROTECT_MODE_NON_MEMBER;
						break;
					default:
						prP2pBssInfo->eHtProtectMode =
						HT_OP_IE(pucIE)->u2Info2;
						break;
					}

					/* u2Info3 */
					prP2pBssInfo->u2HtOpInfo3 =
						HT_OP_IE(pucIE)->u2Info3;

					/* aucBasicMcsSet */
					DBGLOG_MEM8(P2P, TRACE,
					HT_OP_IE(pucIE)->aucBasicMcsSet, 16);
#endif
				}
				break;
			case ELEM_ID_OBSS_SCAN_PARAMS:	/* 74 *//* V */
				{
					DBGLOG(P2P, TRACE,
						"ELEM_ID_OBSS_SCAN_PARAMS IE would be replaced by driver\n");
				}
				break;
			case ELEM_ID_EXTENDED_CAP:	/* 127 *//* V */
				{
					DBGLOG(P2P, TRACE,
						"ELEM_ID_EXTENDED_CAP IE would be replaced by driver\n");
				}
				break;
			case ELEM_ID_VENDOR:	/* 221 *//* V */
				DBGLOG(P2P, TRACE, "Vender Specific IE\n");
				{
					p2pFuncParseBeaconVenderId(prAdapter,
						pucIE, prP2pSpecificBssInfo,
						(uint8_t)
						prP2pBssInfo->u4PrivateData);
					/* TODO: Store other Vender IE
					 * except for WMM Param.
					 */
				}
				break;
			case ELEM_ID_VHT_CAP:
				prP2pBssInfo->ucPhyTypeSet |=
						PHY_TYPE_SET_802_11AC;
				break;
			case ELEM_ID_VHT_OP:
				prP2pBssInfo->ucPhyTypeSet |=
						PHY_TYPE_SET_802_11AC;
				break;
#if (CFG_SUPPORT_802_11AX == 1)
			case ELEM_ID_RESERVED:
				if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_CAP)
					prP2pBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11AX;
				break;
#endif
			default:
				DBGLOG(P2P, TRACE,
					"Unprocessed element ID:%d\n",
					IE_ID(pucIE));
				break;
			}
		}

	} while (FALSE);
}				/* p2pFuncParseBeaconContent */

/* Code refactoring for AOSP */
static void
p2pFuncParseBeaconVenderId(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucIE,
		IN struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo,
		IN uint8_t ucRoleIndex)
{
	do {
		uint8_t ucOuiType;
		uint16_t u2SubTypeVersion;

		if (rsnParseCheckForWFAInfoElem(
			prAdapter, pucIE, &ucOuiType, &u2SubTypeVersion)) {
			if ((ucOuiType == VENDOR_OUI_TYPE_WPA)
				&& (u2SubTypeVersion == VERSION_WPA)) {
				if (!kalP2PGetCcmpCipher(prAdapter->prGlueInfo,
					ucRoleIndex))
					kalP2PSetCipher(prAdapter->prGlueInfo,
						IW_AUTH_CIPHER_TKIP,
						ucRoleIndex);
				if (IE_LEN(pucIE) > ELEM_MAX_LEN_WPA) {
					DBGLOG(P2P, ERROR,
						"WPA IE length is unexpected !!\n");
					return;
				}
				kalMemCopy(
					prP2pSpecificBssInfo
						->aucWpaIeBuffer,
					pucIE, IE_SIZE(pucIE));
				prP2pSpecificBssInfo->u2WpaIeLen =
					IE_SIZE(pucIE);
				DBGLOG(P2P, TRACE, "WPA IE in supplicant\n");
			} else if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
				kalP2PUpdateWSC_IE(prAdapter->prGlueInfo,
					0, pucIE, IE_SIZE(pucIE), ucRoleIndex);
				DBGLOG(P2P, TRACE, "WPS IE in supplicant\n");
			} else if (ucOuiType == VENDOR_OUI_TYPE_WMM) {
				DBGLOG(P2P, TRACE, "WMM IE in supplicant\n");
			}
			/* WMM here. */
		} else if (p2pFuncParseCheckForP2PInfoElem(
			prAdapter, pucIE, &ucOuiType)) {
			/* TODO Store the whole P2P IE & generate later. */
			/* Be aware that there may be one or more P2P IE. */
			if (ucOuiType == VENDOR_OUI_TYPE_P2P) {
				kalMemCopy(&prP2pSpecificBssInfo
					->aucAttributesCache
					[prP2pSpecificBssInfo->u2AttributeLen],
					pucIE, IE_SIZE(pucIE));
				prP2pSpecificBssInfo->u2AttributeLen +=
					IE_SIZE(pucIE);
				DBGLOG(P2P, TRACE, "P2P IE in supplicant\n");
			} else if (ucOuiType == VENDOR_OUI_TYPE_WFD) {

				kalMemCopy(&prP2pSpecificBssInfo
					->aucAttributesCache
					[prP2pSpecificBssInfo->u2AttributeLen],
					pucIE, IE_SIZE(pucIE));

				prP2pSpecificBssInfo->u2AttributeLen +=
					IE_SIZE(pucIE);
			} else {
				DBGLOG(P2P, TRACE,
					"Unknown 50-6F-9A-%d IE.\n",
					ucOuiType);
			}
		} else {
			kalMemCopy(&prP2pSpecificBssInfo->aucAttributesCache
				[prP2pSpecificBssInfo->u2AttributeLen],
				pucIE, IE_SIZE(pucIE));

			prP2pSpecificBssInfo->u2AttributeLen +=
				IE_SIZE(pucIE);
			DBGLOG(P2P, TRACE,
				"Driver unprocessed Vender Specific IE\n");
		}
	} while (0);
}

struct BSS_DESC *
p2pFuncKeepOnConnection(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo,
		IN struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo,
		IN struct P2P_SCAN_REQ_INFO *prScanReqInfo)
{
	struct BSS_DESC *prTargetBss = (struct BSS_DESC *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prBssInfo != NULL) &&
			(prConnReqInfo != NULL) && (prChnlReqInfo != NULL) &&
			(prScanReqInfo != NULL));

		if (prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
			break;
		/* Update connection request information. */
		ASSERT(prConnReqInfo->eConnRequest == P2P_CONNECTION_TYPE_GC);

		/* Find BSS Descriptor first. */
		prTargetBss = scanP2pSearchDesc(prAdapter, prConnReqInfo);

		if (prTargetBss == NULL) {
			/* Update scan parameter... to scan target device. */
			/* TODO: Need refine. */
			prScanReqInfo->ucNumChannelList = 1;
			prScanReqInfo->eScanType = SCAN_TYPE_ACTIVE_SCAN;
			prScanReqInfo->eChannelSet = SCAN_CHANNEL_FULL;
			/* Prevent other P2P ID in IE. */
			prScanReqInfo->u4BufLength = 0;
			prScanReqInfo->fgIsAbort = TRUE;
		} else {
			prChnlReqInfo->u8Cookie = 0;
			prChnlReqInfo->ucReqChnlNum = prTargetBss->ucChannelNum;
			prChnlReqInfo->eBand = prTargetBss->eBand;
			prChnlReqInfo->eChnlSco = prTargetBss->eSco;
			prChnlReqInfo->u4MaxInterval =
				AIS_JOIN_CH_REQUEST_INTERVAL;
			prChnlReqInfo->eChnlReqType = CH_REQ_TYPE_JOIN;

			prChnlReqInfo->eChannelWidth =
				prTargetBss->eChannelWidth;
			prChnlReqInfo->ucCenterFreqS1 =
				prTargetBss->ucCenterFreqS1;
			prChnlReqInfo->ucCenterFreqS2 =
				prTargetBss->ucCenterFreqS2;
		}

	} while (FALSE);

	return prTargetBss;
}				/* p2pFuncKeepOnConnection */

/* Currently Only for ASSOC Response Frame. */
void p2pFuncStoreAssocRspIEBuffer(IN struct ADAPTER *prAdapter,
		IN struct P2P_JOIN_INFO *prP2pJoinInfo,
		IN struct SW_RFB *prSwRfb)
{
	struct WLAN_ASSOC_RSP_FRAME *prAssocRspFrame =
		(struct WLAN_ASSOC_RSP_FRAME *) NULL;
	int16_t i2IELen = 0;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prP2pJoinInfo != NULL) && (prSwRfb != NULL));

		prAssocRspFrame =
			(struct WLAN_ASSOC_RSP_FRAME *) prSwRfb->pvHeader;

		if (prAssocRspFrame->u2FrameCtrl != MAC_FRAME_ASSOC_RSP)
			break;

		i2IELen = prSwRfb->u2PacketLen -
			(WLAN_MAC_HEADER_LEN +
			CAP_INFO_FIELD_LEN +
			STATUS_CODE_FIELD_LEN + AID_FIELD_LEN);

		if (i2IELen <= 0)
			break;

		prP2pJoinInfo->u4BufLength = (uint32_t) i2IELen;

		kalMemCopy(prP2pJoinInfo->aucIEBuf,
			prAssocRspFrame->aucInfoElem,
			prP2pJoinInfo->u4BufLength);

	} while (FALSE);
}				/* p2pFuncStoreAssocRspIEBuffer */

/*---------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set Packet Filter.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer
 *                                     that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_NOT_SUPPORTED
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*---------------------------------------------------------------------------*/
void
p2pFuncMgmtFrameRegister(IN struct ADAPTER *prAdapter,
		IN uint16_t u2FrameType,
		IN u_int8_t fgIsRegistered,
		OUT uint32_t *pu4P2pPacketFilter)
{
	uint32_t u4NewPacketFilter = 0;
	struct CMD_RX_PACKET_FILTER rSetRxPacketFilter;

	DEBUGFUNC("p2pFuncMgmtFrameRegister");

	do {
		ASSERT_BREAK(prAdapter != NULL);

		if (pu4P2pPacketFilter)
			u4NewPacketFilter = *pu4P2pPacketFilter;

		switch (u2FrameType) {
		case MAC_FRAME_PROBE_REQ:
			if (fgIsRegistered) {
				u4NewPacketFilter |=
					PARAM_PACKET_FILTER_PROBE_REQ;
				DBGLOG(P2P, TRACE,
					"Open packet filer probe request\n");
			} else {
				u4NewPacketFilter &=
					~PARAM_PACKET_FILTER_PROBE_REQ;
				DBGLOG(P2P, TRACE,
					"Close packet filer probe request\n");
			}
			break;
		case MAC_FRAME_ACTION:
			if (fgIsRegistered) {
				u4NewPacketFilter |=
					PARAM_PACKET_FILTER_ACTION_FRAME;
				DBGLOG(P2P, TRACE,
					"Open packet filer action frame.\n");
			} else {
				u4NewPacketFilter &=
					~PARAM_PACKET_FILTER_ACTION_FRAME;
				DBGLOG(P2P, TRACE,
					"Close packet filer action frame.\n");
			}
			break;
		default:
			DBGLOG(P2P, TRACE,
				"Ask frog to add code for mgmt:%x\n",
				u2FrameType);
			break;
		}

		if (pu4P2pPacketFilter)
			*pu4P2pPacketFilter = u4NewPacketFilter;

		/* u4NewPacketFilter |= prAdapter->u4OsPacketFilter; */

		prAdapter->u4OsPacketFilter &= ~PARAM_PACKET_FILTER_P2P_MASK;
		prAdapter->u4OsPacketFilter |= u4NewPacketFilter;

		DBGLOG(P2P, TRACE,
			"P2P Set PACKET filter:0x%x\n",
			prAdapter->u4OsPacketFilter);

		rSetRxPacketFilter.u4RxPacketFilter =
			prAdapter->u4OsPacketFilter;
		wlanoidSetPacketFilter(prAdapter,
			&rSetRxPacketFilter,
			FALSE,
			&u4NewPacketFilter,
			sizeof(u4NewPacketFilter));

	} while (FALSE);
}				/* p2pFuncMgmtFrameRegister */

void p2pFuncUpdateMgmtFrameRegister(IN struct ADAPTER *prAdapter,
		IN uint32_t u4OsFilter)
{
	struct CMD_RX_PACKET_FILTER rSetRxPacketFilter;

	do {

		/* TODO: Filter need to be done. */
		/* prAdapter->rWifiVar
		 * .prP2pFsmInfo->u4P2pPacketFilter = u4OsFilter;
		 */

		if ((prAdapter->u4OsPacketFilter
			& PARAM_PACKET_FILTER_P2P_MASK) ^ u4OsFilter) {

			prAdapter->u4OsPacketFilter &=
				~PARAM_PACKET_FILTER_P2P_MASK;

			prAdapter->u4OsPacketFilter |=
				(u4OsFilter & PARAM_PACKET_FILTER_P2P_MASK);

			rSetRxPacketFilter.u4RxPacketFilter =
				prAdapter->u4OsPacketFilter;
			wlanoidSetPacketFilter(prAdapter,
				&rSetRxPacketFilter,
				FALSE,
				&u4OsFilter,
				sizeof(u4OsFilter));
			DBGLOG(P2P, TRACE,
				"P2P Set PACKET filter:0x%x\n",
				prAdapter->u4OsPacketFilter);
		}

	} while (FALSE);
}				/* p2pFuncUpdateMgmtFrameRegister */

void p2pFuncGetStationInfo(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucMacAddr,
		OUT struct P2P_STATION_INFO *prStaInfo)
{

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (pucMacAddr != NULL) && (prStaInfo != NULL));

		prStaInfo->u4InactiveTime = 0;
		prStaInfo->u4RxBytes = 0;
		prStaInfo->u4TxBytes = 0;
		prStaInfo->u4RxPackets = 0;
		prStaInfo->u4TxPackets = 0;
		/* TODO: */

	} while (FALSE);
}				/* p2pFuncGetStationInfo */

#if 0
u_int8_t
p2pFuncGetAttriList(IN struct ADAPTER *prAdapter,
		IN uint8_t ucOuiType,
		IN uint8_t *pucIE,
		IN uint16_t u2IELength,
		OUT uint8_t **ppucAttriList,
		OUT uint16_t *pu2AttriListLen)
{
	u_int8_t fgIsAllocMem = FALSE;
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;
	uint16_t u2Offset = 0;
	struct IE_P2P *prIe = (struct IE_P2P *) NULL;
	uint8_t *pucAttriListStart = (uint8_t *) NULL;
	uint16_t u2AttriListLen = 0, u2BufferSize;
	u_int8_t fgBackupAttributes = FALSE;

	u2BufferSize = 0;

	do {
		ASSERT_BREAK((prAdapter != NULL) &&
			(pucIE != NULL) &&
			(u2IELength != 0) &&
			(ppucAttriList != NULL) &&
			(pu2AttriListLen != NULL));

		if (ppucAttriList)
			*ppucAttriList = NULL;
		if (pu2AttriListLen)
			*pu2AttriListLen = 0;

		if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
			aucWfaOui[0] = 0x00;
			aucWfaOui[1] = 0x50;
			aucWfaOui[2] = 0xF2;
		} else if ((ucOuiType != VENDOR_OUI_TYPE_P2P)
#if CFG_SUPPORT_WFD
			   && (ucOuiType != VENDOR_OUI_TYPE_WFD)
#endif
		    ) {
			DBGLOG(P2P, INFO,
				"Not supported OUI Type to parsing 0x%x\n",
				ucOuiType);
			break;
		}

		IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
			if (IE_ID(pucIE) == ELEM_ID_VENDOR) {
				prIe = (struct IE_P2P *) pucIE;

				if (prIe->ucLength <= P2P_OUI_TYPE_LEN)
					continue;

				if ((prIe->aucOui[0] == aucWfaOui[0]) &&
				    (prIe->aucOui[1] == aucWfaOui[1]) &&
				    (prIe->aucOui[2] == aucWfaOui[2]) &&
				    (ucOuiType == prIe->ucOuiType)) {
					p2pFuncGetAttriListAction(prAdapter,
						prIe, ucOuiType,
						&pucAttriListStart,
						&u2AttriListLen,
						&fgIsAllocMem,
						&fgBackupAttributes,
						&u2BufferSize);
				}	/* prIe->aucOui */
			}	/* ELEM_ID_VENDOR */
		}		/* IE_FOR_EACH */

	} while (FALSE);

	if (pucAttriListStart) {
		uint8_t *pucAttribute = pucAttriListStart;

		DBGLOG(P2P, LOUD, "Checking Attribute Length.\n");
		if (ucOuiType == VENDOR_OUI_TYPE_P2P) {
			P2P_ATTRI_FOR_EACH(pucAttribute,
				u2AttriListLen, u2Offset);
		} else if (ucOuiType == VENDOR_OUI_TYPE_WFD) {
			/* Todo:: Nothing */
		} else if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
			/* Big Endian: WSC, WFD. */
			WSC_ATTRI_FOR_EACH(pucAttribute,
				u2AttriListLen, u2Offset) {
				DBGLOG(P2P, LOUD,
					"Attribute ID:%d, Length:%d.\n",
					WSC_ATTRI_ID(pucAttribute),
					WSC_ATTRI_LEN(pucAttribute));
			}
		} else {
		}

		ASSERT(u2Offset == u2AttriListLen);

		*ppucAttriList = pucAttriListStart;
		*pu2AttriListLen = u2AttriListLen;

	} else {
		*ppucAttriList = (uint8_t *) NULL;
		*pu2AttriListLen = 0;
	}

	return fgIsAllocMem;
}				/* p2pFuncGetAttriList */

/* Code refactoring for AOSP */
static void
p2pFuncGetAttriListAction(IN struct ADAPTER *prAdapter,
		IN struct IE_P2P *prIe,
		IN uint8_t ucOuiType,
		OUT uint8_t **pucAttriListStart,
		OUT uint16_t *u2AttriListLen,
		OUT u_int8_t *fgIsAllocMem,
		OUT u_int8_t *fgBackupAttributes,
		OUT uint16_t *u2BufferSize)
{
	do {
		if (!(*pucAttriListStart)) {
			*pucAttriListStart = &prIe->aucP2PAttributes[0];
			if (prIe->ucLength > P2P_OUI_TYPE_LEN)
				*u2AttriListLen =
					(uint16_t)
					(prIe->ucLength - P2P_OUI_TYPE_LEN);
			else
				ASSERT(FALSE);
		} else {
			/* More than 2 attributes. */
			uint16_t u2CopyLen;

			if (*fgBackupAttributes == FALSE) {
				struct P2P_SPECIFIC_BSS_INFO
					*prP2pSpecificBssInfo =
				    prAdapter->rWifiVar.prP2pSpecificBssInfo;

				*fgBackupAttributes = TRUE;
				if (ucOuiType == VENDOR_OUI_TYPE_P2P) {
					kalMemCopy(&prP2pSpecificBssInfo
						->aucAttributesCache[0],
					*pucAttriListStart, *u2AttriListLen);

					*pucAttriListStart =
						&prP2pSpecificBssInfo
						->aucAttributesCache[0];
					*u2BufferSize =
						P2P_MAXIMUM_ATTRIBUTE_LEN;
				} else if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
					kalMemCopy(&prP2pSpecificBssInfo
						->aucWscAttributesCache[0],
					*pucAttriListStart, *u2AttriListLen);
					*pucAttriListStart =
						&prP2pSpecificBssInfo
						->aucWscAttributesCache[0];
					*u2BufferSize =
					WPS_MAXIMUM_ATTRIBUTES_CACHE_SIZE;
				}
#if CFG_SUPPORT_WFD
				else if (ucOuiType == VENDOR_OUI_TYPE_WFD) {
					uint8_t *pucTmpBuf = (uint8_t *) NULL;

					pucTmpBuf = (uint8_t *) kalMemAlloc
					    (WPS_MAXIMUM_ATTRIBUTES_CACHE_SIZE,
					    VIR_MEM_TYPE);

					if (pucTmpBuf != NULL) {
						*fgIsAllocMem = TRUE;
					} else {
						/* Can't alloca memory
						 * for WFD IE relocate.
						 */
						ASSERT(FALSE);
						break;
					}

					kalMemCopy(pucTmpBuf,
						*pucAttriListStart,
						*u2AttriListLen);
					*pucAttriListStart = pucTmpBuf;
					*u2BufferSize =
					WPS_MAXIMUM_ATTRIBUTES_CACHE_SIZE;
				}
#endif
				else
					*fgBackupAttributes = FALSE;
			}
			u2CopyLen =
				(uint16_t) (prIe->ucLength - P2P_OUI_TYPE_LEN);

			if (((*u2AttriListLen) + u2CopyLen) > (*u2BufferSize)) {
				u2CopyLen = (*u2BufferSize) - (*u2AttriListLen);
				DBGLOG(P2P, WARN,
					"Length of received P2P attributes > maximum cache size.\n");
			}

			if (u2CopyLen) {
				kalMemCopy((uint8_t *)
					((unsigned long) (*pucAttriListStart) +
					(unsigned long) (*u2AttriListLen)),
					&prIe->aucP2PAttributes[0], u2CopyLen);
				*u2AttriListLen += u2CopyLen;
			}

		}
	} while (0);
}
#endif

struct MSDU_INFO *p2pFuncProcessP2pProbeRsp(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx, IN struct MSDU_INFO *prMgmtTxMsdu)
{
	struct MSDU_INFO *prRetMsduInfo = prMgmtTxMsdu;
	struct WLAN_BEACON_FRAME *prProbeRspFrame =
		(struct WLAN_BEACON_FRAME *) NULL;
	uint8_t *pucIEBuf = (uint8_t *) NULL;
	uint16_t u2Offset = 0, u2IELength = 0, u2ProbeRspHdrLen = 0;
	u_int8_t fgIsWSCIE = FALSE;
	u_int8_t fgIsWFDIE = FALSE;
	u_int8_t fgIsVenderIE = FALSE;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
	uint16_t u2EstimateSize = 0, u2EstimatedExtraIELen = 0;
	uint32_t u4IeArraySize = 0, u4Idx = 0;
	u_int8_t u4P2PIEIdx = 0;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMgmtTxMsdu != NULL));

		prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

		/* 3 Make sure this is probe response frame. */
		prProbeRspFrame = (struct WLAN_BEACON_FRAME *)
			((unsigned long) prMgmtTxMsdu->prPacket +
			MAC_TX_RESERVED_FIELD);
		ASSERT_BREAK((prProbeRspFrame->u2FrameCtrl & MASK_FRAME_TYPE)
			== MAC_FRAME_PROBE_RSP);

		/* 3 Get the importent P2P IE. */
		u2ProbeRspHdrLen =
		    (WLAN_MAC_MGMT_HEADER_LEN +
		    TIMESTAMP_FIELD_LEN +
		    BEACON_INTERVAL_FIELD_LEN +
		    CAP_INFO_FIELD_LEN);
		pucIEBuf = prProbeRspFrame->aucInfoElem;
		u2IELength = prMgmtTxMsdu->u2FrameLength - u2ProbeRspHdrLen;

#if CFG_SUPPORT_WFD
		/* Reset in each time ?? */
		prAdapter->prGlueInfo
			->prP2PInfo[prP2pBssInfo->u4PrivateData]
			->u2WFDIELen = 0;
#endif
#if CFG_SUPPORT_CUSTOM_VENDOR_IE
		prAdapter->prGlueInfo
			->prP2PInfo[prP2pBssInfo->u4PrivateData]
			->u2VenderIELen = 0;
#endif


		IE_FOR_EACH(pucIEBuf, u2IELength, u2Offset) {
			switch (IE_ID(pucIEBuf)) {
			case ELEM_ID_SSID:
				{
					p2pFuncProcessP2pProbeRspAction(
						prAdapter,
						pucIEBuf, ELEM_ID_SSID,
						&ucBssIdx,
						&prP2pBssInfo,
						&fgIsWSCIE,
						&u4P2PIEIdx,
						&fgIsWFDIE,
						&fgIsVenderIE);
				}
				break;
			case ELEM_ID_VENDOR:
				{
					p2pFuncProcessP2pProbeRspAction(
						prAdapter,
						pucIEBuf, ELEM_ID_VENDOR,
						&ucBssIdx,
						&prP2pBssInfo,
						&fgIsWSCIE,
						&u4P2PIEIdx,
						&fgIsWFDIE,
						&fgIsVenderIE);
				}
				break;
			default:
				break;
			}

		}

		/* 3 Check the total size & current frame. */
		u2EstimateSize = WLAN_MAC_MGMT_HEADER_LEN +
		    TIMESTAMP_FIELD_LEN +
		    BEACON_INTERVAL_FIELD_LEN +
		    CAP_INFO_FIELD_LEN +
		    (ELEM_HDR_LEN + ELEM_MAX_LEN_SSID) +
		    (ELEM_HDR_LEN + ELEM_MAX_LEN_SUP_RATES) +
		    (ELEM_HDR_LEN + ELEM_MAX_LEN_DS_PARAMETER_SET);

		u2EstimatedExtraIELen = 0;

		u4IeArraySize =
			sizeof(txProbeRspIETable) /
			sizeof(struct APPEND_VAR_IE_ENTRY);
		for (u4Idx = 0; u4Idx < u4IeArraySize; u4Idx++) {
			if (txProbeRspIETable[u4Idx].u2EstimatedFixedIELen) {
				u2EstimatedExtraIELen +=
					txProbeRspIETable[u4Idx]
					.u2EstimatedFixedIELen;
			}

			else {
				ASSERT(txProbeRspIETable[u4Idx]
					.pfnCalculateVariableIELen);

				u2EstimatedExtraIELen +=
					(uint16_t) (
						txProbeRspIETable[u4Idx]
						.pfnCalculateVariableIELen(
							prAdapter,
							ucBssIdx, NULL));
			}

		}

		if (fgIsWSCIE)
			u2EstimatedExtraIELen +=
				kalP2PCalWSC_IELen(prAdapter->prGlueInfo, 2,
					(uint8_t) prP2pBssInfo->u4PrivateData);

		if (u4P2PIEIdx > 0) {
			for (u4Idx = 0; u4Idx < u4P2PIEIdx; u4Idx++)
				u2EstimatedExtraIELen +=
					kalP2PCalP2P_IELen(
						prAdapter->prGlueInfo,
						u4Idx,
						(uint8_t)
						prP2pBssInfo->u4PrivateData);
			u2EstimatedExtraIELen +=
				p2pFuncCalculateP2P_IE_NoA(prAdapter,
					ucBssIdx, NULL);
		}
#if CFG_SUPPORT_WFD
		ASSERT(sizeof(prAdapter->prGlueInfo
			->prP2PInfo[prP2pBssInfo->u4PrivateData]->aucWFDIE)
			>=
		prAdapter->prGlueInfo
		->prP2PInfo[prP2pBssInfo->u4PrivateData]->u2WFDIELen);

		if (fgIsWFDIE)
			u2EstimatedExtraIELen +=
				prAdapter->prGlueInfo
					->prP2PInfo[prP2pBssInfo->u4PrivateData]
					->u2WFDIELen;
#endif
#if CFG_SUPPORT_CUSTOM_VENDOR_IE
		if (fgIsVenderIE)
			u2EstimatedExtraIELen +=
				prAdapter->prGlueInfo
					->prP2PInfo[prP2pBssInfo->u4PrivateData]
					->u2VenderIELen;
#endif

		u2EstimateSize += u2EstimatedExtraIELen;
		if ((u2EstimateSize) > (prRetMsduInfo->u2FrameLength)) {
			/* add sizeof(UINT_64) for Cookie */
			prRetMsduInfo = cnmMgtPktAlloc(prAdapter,
				u2EstimateSize + sizeof(uint64_t));

			if (prRetMsduInfo == NULL) {
				DBGLOG(P2P, WARN,
					"No packet for sending new probe response, use original one\n");
				prRetMsduInfo = prMgmtTxMsdu;
				break;
			}

		}

		prRetMsduInfo->ucBssIndex = ucBssIdx;

		/* 3 Compose / Re-compose probe response frame. */
		bssComposeBeaconProbeRespFrameHeaderAndFF((uint8_t *)
			((unsigned long) (prRetMsduInfo->prPacket) +
			MAC_TX_RESERVED_FIELD),
			prProbeRspFrame->aucDestAddr,
			prProbeRspFrame->aucSrcAddr,
			prProbeRspFrame->aucBSSID,
			prProbeRspFrame->u2BeaconInterval,
			prProbeRspFrame->u2CapInfo);

		prRetMsduInfo->u2FrameLength =
		    (WLAN_MAC_MGMT_HEADER_LEN +
		    TIMESTAMP_FIELD_LEN +
		    BEACON_INTERVAL_FIELD_LEN + CAP_INFO_FIELD_LEN);

		bssBuildBeaconProbeRespFrameCommonIEs(prRetMsduInfo,
			prP2pBssInfo, prProbeRspFrame->aucDestAddr);

		prRetMsduInfo->ucStaRecIndex = prMgmtTxMsdu->ucStaRecIndex;

		for (u4Idx = 0; u4Idx < u4IeArraySize; u4Idx++) {
			if (txProbeRspIETable[u4Idx].pfnAppendIE)
				txProbeRspIETable[u4Idx]
					.pfnAppendIE(prAdapter, prRetMsduInfo);

		}

		if (fgIsWSCIE) {
			kalP2PGenWSC_IE(prAdapter->prGlueInfo,
				2,
				(uint8_t *)
				((unsigned long) prRetMsduInfo->prPacket +
				(unsigned long) prRetMsduInfo->u2FrameLength),
				(uint8_t) prP2pBssInfo->u4PrivateData);

			prRetMsduInfo->u2FrameLength += (uint16_t)
				kalP2PCalWSC_IELen(prAdapter->prGlueInfo,
				2,
				(uint8_t) prP2pBssInfo->u4PrivateData);
		}

		if (u4P2PIEIdx > 0) {
			for (u4Idx = 0; u4Idx < u4P2PIEIdx; u4Idx++) {
				kalP2PGenP2P_IE(prAdapter->prGlueInfo,
					u4Idx,
					(uint8_t *)
					((unsigned long)
					prRetMsduInfo->prPacket +
					(unsigned long)
					prRetMsduInfo->u2FrameLength),
					(uint8_t) prP2pBssInfo->u4PrivateData);

				prRetMsduInfo->u2FrameLength +=
					(uint16_t)
					kalP2PCalP2P_IELen(
					prAdapter->prGlueInfo,
					u4Idx,
					(uint8_t) prP2pBssInfo->u4PrivateData);
			}
			p2pFuncGenerateP2P_IE_NoA(prAdapter, prRetMsduInfo);

		}
#if CFG_SUPPORT_WFD
		if (fgIsWFDIE > 0) {
			ASSERT(prAdapter->prGlueInfo
				->prP2PInfo[prP2pBssInfo->u4PrivateData]
				->u2WFDIELen > 0);
			kalMemCopy((uint8_t *)
				((unsigned long) prRetMsduInfo->prPacket +
				(unsigned long) prRetMsduInfo->u2FrameLength),
				prAdapter->prGlueInfo->prP2PInfo
					[prP2pBssInfo->u4PrivateData]->aucWFDIE,
				prAdapter->prGlueInfo->prP2PInfo
					[prP2pBssInfo->u4PrivateData]
					->u2WFDIELen);
			prRetMsduInfo->u2FrameLength +=
			(uint16_t) prAdapter->prGlueInfo
				->prP2PInfo[prP2pBssInfo->u4PrivateData]
				->u2WFDIELen;

		}
#endif /* CFG_SUPPORT_WFD */
#if CFG_SUPPORT_CUSTOM_VENDOR_IE
		if (fgIsVenderIE &&
			prAdapter->prGlueInfo
			->prP2PInfo[prP2pBssInfo->u4PrivateData]
			->u2VenderIELen > 0) {
			kalMemCopy((uint8_t *)
				((unsigned long) prRetMsduInfo->prPacket +
				(unsigned long) prRetMsduInfo->u2FrameLength),
				prAdapter->prGlueInfo->prP2PInfo
					[prP2pBssInfo->u4PrivateData]
					->aucVenderIE,
				prAdapter->prGlueInfo->prP2PInfo
					[prP2pBssInfo->u4PrivateData]
					->u2VenderIELen);
			prRetMsduInfo->u2FrameLength +=
			(uint16_t) prAdapter->prGlueInfo
				->prP2PInfo[prP2pBssInfo->u4PrivateData]
				->u2VenderIELen;
		}
#endif

	} while (FALSE);

	if (prRetMsduInfo != prMgmtTxMsdu)
		cnmMgtPktFree(prAdapter, prMgmtTxMsdu);

	return prRetMsduInfo;
}				/* p2pFuncProcessP2pProbeRsp */

/* Code refactoring for AOSP */
static void
p2pFuncProcessP2pProbeRspAction(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucIEBuf,
		IN uint8_t ucElemIdType,
		OUT uint8_t *ucBssIdx,
		OUT struct BSS_INFO **prP2pBssInfo,
		OUT u_int8_t *fgIsWSCIE,
		OUT u_int8_t *u4P2PIEIdx,
		OUT u_int8_t *fgIsWFDIE,
		OUT u_int8_t *fgIsVenderIE)
{
	uint8_t ucOuiType = 0;
	uint16_t u2SubTypeVersion = 0;

	switch (ucElemIdType) {
	case ELEM_ID_SSID:
		{
			if (SSID_IE(pucIEBuf)->ucLength > 7) {
				for ((*ucBssIdx) = 0;
					(*ucBssIdx) < prAdapter->ucHwBssIdNum;
					(*ucBssIdx)++) {
					*prP2pBssInfo =
						GET_BSS_INFO_BY_INDEX(
							prAdapter, *ucBssIdx);
					if (!(*prP2pBssInfo))
						continue;
					if (EQUAL_SSID(
						(*prP2pBssInfo)->aucSSID,
						(*prP2pBssInfo)->ucSSIDLen,
						SSID_IE(pucIEBuf)->aucSSID,
						SSID_IE(pucIEBuf)->ucLength)) {
						break;
					}
				}
				if ((*ucBssIdx) == prAdapter->ucP2PDevBssIdx)
					*prP2pBssInfo =
						GET_BSS_INFO_BY_INDEX(
							prAdapter, *ucBssIdx);
			} else {
				*prP2pBssInfo =
					GET_BSS_INFO_BY_INDEX(
						prAdapter,
						prAdapter->ucP2PDevBssIdx);
				COPY_SSID(
					(*prP2pBssInfo)->aucSSID,
					(*prP2pBssInfo)->ucSSIDLen,
					SSID_IE(pucIEBuf)->aucSSID,
					SSID_IE(pucIEBuf)->ucLength);

			}
		}
		break;
	case ELEM_ID_VENDOR:
		if (rsnParseCheckForWFAInfoElem(prAdapter,
			pucIEBuf, &ucOuiType, &u2SubTypeVersion)) {
			if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
				kalP2PUpdateWSC_IE(prAdapter->prGlueInfo,
					2, pucIEBuf,
					IE_SIZE(pucIEBuf),
					(uint8_t)
					((struct BSS_INFO *)*prP2pBssInfo)
					->u4PrivateData);
				*fgIsWSCIE = TRUE;
			}

		} else if (p2pFuncParseCheckForP2PInfoElem(prAdapter,
			pucIEBuf, &ucOuiType)) {
			if (ucOuiType == VENDOR_OUI_TYPE_P2P) {
				/* 2 Note(frog): I use WSC IE buffer
				 * for Probe Request
				 * to store the P2P IE for Probe Response.
				 */
				if (*u4P2PIEIdx < MAX_P2P_IE_SIZE) {
					kalP2PUpdateP2P_IE(
						prAdapter->prGlueInfo,
						*u4P2PIEIdx,
						pucIEBuf,
						IE_SIZE(pucIEBuf),
						(uint8_t)
						((struct BSS_INFO *)
						*prP2pBssInfo)
						->u4PrivateData);
					*u4P2PIEIdx = *u4P2PIEIdx + 1;
				} else
					DBGLOG(P2P, WARN,
						"Too much P2P IE for ProbeResp, skip update\n");
			}
#if CFG_SUPPORT_WFD
			else if (ucOuiType == VENDOR_OUI_TYPE_WFD) {
				DBGLOG(P2P, INFO,
				       "WFD IE is found in probe resp (supp). Len %u\n",
				       IE_SIZE(pucIEBuf));
				if ((sizeof(
				prAdapter->prGlueInfo
				->prP2PInfo
				[((struct BSS_INFO *)*prP2pBssInfo)
				->u4PrivateData]
				->aucWFDIE)
				>=
				(prAdapter->prGlueInfo
				->prP2PInfo
				[((struct BSS_INFO *)*prP2pBssInfo)
				->u4PrivateData]->u2WFDIELen +
				IE_SIZE(pucIEBuf)))) {
					*fgIsWFDIE = TRUE;
					kalMemCopy(prAdapter->prGlueInfo
						->prP2PInfo
						[((struct BSS_INFO *)
						*prP2pBssInfo)
						->u4PrivateData]->aucWFDIE,
						pucIEBuf, IE_SIZE(pucIEBuf));
					prAdapter->prGlueInfo
						->prP2PInfo
						[((struct BSS_INFO *)
						*prP2pBssInfo)
						->u4PrivateData]->u2WFDIELen +=
						IE_SIZE(pucIEBuf);
				}
			}	/*  VENDOR_OUI_TYPE_WFD */
#endif
		} else {
			DBGLOG(P2P, INFO,
			       "Other vender IE is found in probe resp (supp). Len %u\n",
			       IE_SIZE(pucIEBuf));
#if CFG_SUPPORT_CUSTOM_VENDOR_IE
			if ((prAdapter->prGlueInfo->prP2PInfo
				[((struct BSS_INFO *)*prP2pBssInfo)
				->u4PrivateData]->u2VenderIELen
				+ IE_SIZE(pucIEBuf)) < 1024) {
				*fgIsVenderIE = TRUE;
				kalMemCopy(prAdapter->prGlueInfo
					->prP2PInfo
					[((struct BSS_INFO *)
					*prP2pBssInfo)
					->u4PrivateData]->aucVenderIE +
					prAdapter->prGlueInfo
					->prP2PInfo
					[((struct BSS_INFO *)
					*prP2pBssInfo)
					->u4PrivateData]->u2VenderIELen,
					pucIEBuf, IE_SIZE(pucIEBuf));
				prAdapter->prGlueInfo
					->prP2PInfo
					[((struct BSS_INFO *)
					*prP2pBssInfo)
					->u4PrivateData]->u2VenderIELen +=
					IE_SIZE(pucIEBuf);
			}
#endif
		}
		break;
	default:
		break;
	}
}

#if 0 /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0) */
uint32_t
p2pFuncCalculateExtra_IELenForBeacon(IN struct ADAPTER *prAdapter,
		IN ENUM_NETWORK_TYPE_INDEX_T eNetTypeIndex,
		IN struct STA_RECORD *prStaRec)
{

	struct P2P_SPECIFIC_BSS_INFO *prP2pSpeBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	uint32_t u4IELen = 0;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (eNetTypeIndex == NETWORK_TYPE_P2P_INDEX));

		if (p2pFuncIsAPMode(prAdapter->rWifiVar.prP2pFsmInfo))
			break;

		prP2pSpeBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo;

		u4IELen = prP2pSpeBssInfo->u2IELenForBCN;

	} while (FALSE);

	return u4IELen;
}				/* p2pFuncCalculateP2p_IELenForBeacon */

void p2pFuncGenerateExtra_IEForBeacon(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo)
{
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpeBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	uint8_t *pucIEBuf = (uint8_t *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

		prP2pSpeBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo;

		if (p2pFuncIsAPMode(prAdapter->rWifiVar.prP2pFsmInfo))
			break;

		pucIEBuf = (uint8_t *) ((uint32_t) prMsduInfo->prPacket +
			(uint32_t) prMsduInfo->u2FrameLength);

		kalMemCopy(pucIEBuf,
			prP2pSpeBssInfo->aucBeaconIECache,
			prP2pSpeBssInfo->u2IELenForBCN);

		prMsduInfo->u2FrameLength += prP2pSpeBssInfo->u2IELenForBCN;

	} while (FALSE);
}				/* p2pFuncGenerateExtra_IEForBeacon */

#else
uint32_t p2pFuncCalculateP2p_IELenForBeacon(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx, IN struct STA_RECORD *prStaRec)
{
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpeBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	uint32_t u4IELen = 0;
	struct BSS_INFO *prBssInfo;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (ucBssIdx < prAdapter->ucHwBssIdNum));

		prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

		if (!prAdapter->fgIsP2PRegistered)
			break;

		if (p2pFuncIsAPMode(
			prAdapter->rWifiVar.prP2PConnSettings
			[prBssInfo->u4PrivateData]))
			break;

		prP2pSpeBssInfo =
			prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prBssInfo->u4PrivateData];

		u4IELen = prP2pSpeBssInfo->u2AttributeLen;

	} while (FALSE);

	return u4IELen;
}				/* p2pFuncCalculateP2p_IELenForBeacon */

void p2pFuncGenerateP2p_IEForBeacon(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo)
{
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpeBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	uint8_t *pucIEBuf = (uint8_t *) NULL;
	struct BSS_INFO *prBssInfo;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

		if (!prAdapter->fgIsP2PRegistered)
			break;

		prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];

		prP2pSpeBssInfo =
			prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prBssInfo->u4PrivateData];

		if (p2pFuncIsAPMode(
			prAdapter->rWifiVar.prP2PConnSettings
			[prBssInfo->u4PrivateData]))
			break;

		pucIEBuf = (uint8_t *) ((unsigned long) prMsduInfo->prPacket +
			(unsigned long) prMsduInfo->u2FrameLength);

		kalMemCopy(pucIEBuf,
			prP2pSpeBssInfo->aucAttributesCache,
			prP2pSpeBssInfo->u2AttributeLen);

		prMsduInfo->u2FrameLength += prP2pSpeBssInfo->u2AttributeLen;

	} while (FALSE);
}				/* p2pFuncGenerateP2p_IEForBeacon */

uint32_t p2pFuncCalculateWSC_IELenForBeacon(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx, IN struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

	if (prP2pBssInfo->eNetworkType != NETWORK_TYPE_P2P)
		return 0;

	return kalP2PCalWSC_IELen(prAdapter->prGlueInfo,
		0, (uint8_t) prP2pBssInfo->u4PrivateData);
}				/* p2pFuncCalculateP2p_IELenForBeacon */

void p2pFuncGenerateWSC_IEForBeacon(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucBuffer;
	uint16_t u2IELen = 0;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);

	if (prP2pBssInfo->eNetworkType != NETWORK_TYPE_P2P)
		return;

	u2IELen = (uint16_t) kalP2PCalWSC_IELen(prAdapter->prGlueInfo,
		0, (uint8_t) prP2pBssInfo->u4PrivateData);

	pucBuffer = (uint8_t *) ((unsigned long) prMsduInfo->prPacket +
		(unsigned long) prMsduInfo->u2FrameLength);

	ASSERT(pucBuffer);

	/* TODO: Check P2P FSM State. */
	kalP2PGenWSC_IE(prAdapter->prGlueInfo,
		0, pucBuffer, (uint8_t) prP2pBssInfo->u4PrivateData);

	prMsduInfo->u2FrameLength += u2IELen;
}				/* p2pFuncGenerateP2p_IEForBeacon */

#endif
/*---------------------------------------------------------------------------*/
/*!
 * @brief This function is used to calculate P2P IE length for Beacon frame.
 *
 * @param[in] eNetTypeIndex      Specify which network
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 *
 * @return The length of P2P IE added
 */
/*---------------------------------------------------------------------------*/
uint32_t p2pFuncCalculateP2p_IELenForAssocRsp(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex, IN struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (prBssInfo->eNetworkType != NETWORK_TYPE_P2P)
		return 0;

	return p2pFuncCalculateP2P_IELen(prAdapter,
		ucBssIndex,
		prStaRec,
		txAssocRspAttributesTable,
		sizeof(txAssocRspAttributesTable) /
		sizeof(struct APPEND_VAR_ATTRI_ENTRY));

}				/* p2pFuncCalculateP2p_IELenForAssocRsp */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function is used to generate P2P IE for Beacon frame.
 *
 * @param[in] prMsduInfo             Pointer to the composed MSDU_INFO_T.
 *
 * @return none
 */
/*---------------------------------------------------------------------------*/
void p2pFuncGenerateP2p_IEForAssocRsp(IN struct ADAPTER *prAdapter,
	IN struct MSDU_INFO *prMsduInfo)
{
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;


	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prStaRec) {
		DBGLOG(P2P, ERROR, "prStaRec of ucStaRecIndex %d is NULL!\n",
			prMsduInfo->ucStaRecIndex);
		return;
	}

	if (IS_STA_IN_P2P(prStaRec)) {
		DBGLOG(P2P, TRACE, "Generate NULL P2P IE for Assoc Rsp.\n");

		p2pFuncGenerateP2P_IE(prAdapter,
			prMsduInfo->ucBssIndex,
			TRUE,
			&prMsduInfo->u2FrameLength,
			prMsduInfo->prPacket,
			1500,
			txAssocRspAttributesTable,
			sizeof(txAssocRspAttributesTable) /
			sizeof(struct APPEND_VAR_ATTRI_ENTRY));
	} else {

		DBGLOG(P2P, TRACE, "Legacy device, no P2P IE.\n");
	}
}				/* p2pFuncGenerateP2p_IEForAssocRsp */

uint32_t
p2pFuncCalculateP2P_IELen(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct STA_RECORD *prStaRec,
		IN struct APPEND_VAR_ATTRI_ENTRY arAppendAttriTable[],
		IN uint32_t u4AttriTableSize)
{

	uint32_t u4OverallAttriLen, u4Dummy;
	uint16_t u2EstimatedFixedAttriLen;
	uint32_t i;

	/* Overall length of all Attributes */
	u4OverallAttriLen = 0;

	for (i = 0; i < u4AttriTableSize; i++) {
		u2EstimatedFixedAttriLen =
			arAppendAttriTable[i].u2EstimatedFixedAttriLen;

		if (u2EstimatedFixedAttriLen) {
			u4OverallAttriLen += u2EstimatedFixedAttriLen;
		} else {
			ASSERT(arAppendAttriTable[i]
				.pfnCalculateVariableAttriLen);

			u4OverallAttriLen += arAppendAttriTable[i]
				.pfnCalculateVariableAttriLen
					(prAdapter, prStaRec);
		}
	}

	u4Dummy = u4OverallAttriLen;
	u4OverallAttriLen += P2P_IE_OUI_HDR;

	for (; (u4Dummy > P2P_MAXIMUM_ATTRIBUTE_LEN);) {
		u4OverallAttriLen += P2P_IE_OUI_HDR;
		u4Dummy -= P2P_MAXIMUM_ATTRIBUTE_LEN;
	}

	return u4OverallAttriLen;
}				/* p2pFuncCalculateP2P_IELen */

void
p2pFuncGenerateP2P_IE(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN u_int8_t fgIsAssocFrame,
		IN uint16_t *pu2Offset,
		IN uint8_t *pucBuf,
		IN uint16_t u2BufSize,
		IN struct APPEND_VAR_ATTRI_ENTRY arAppendAttriTable[],
		IN uint32_t u4AttriTableSize)
{
	uint8_t *pucBuffer = (uint8_t *) NULL;
	struct IE_P2P *prIeP2P = (struct IE_P2P *) NULL;
	uint32_t u4OverallAttriLen;
	uint32_t u4AttriLen;
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;
	uint8_t aucTempBuffer[P2P_MAXIMUM_ATTRIBUTE_LEN];
	uint32_t i;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (pucBuf != NULL));

		pucBuffer = (uint8_t *) ((unsigned long) pucBuf + (*pu2Offset));

		ASSERT_BREAK(pucBuffer != NULL);

		/* Check buffer length is still enough. */
		ASSERT_BREAK((u2BufSize - (*pu2Offset)) >= P2P_IE_OUI_HDR);

		prIeP2P = (struct IE_P2P *) pucBuffer;

		prIeP2P->ucId = ELEM_ID_P2P;

		prIeP2P->aucOui[0] = aucWfaOui[0];
		prIeP2P->aucOui[1] = aucWfaOui[1];
		prIeP2P->aucOui[2] = aucWfaOui[2];
		prIeP2P->ucOuiType = VENDOR_OUI_TYPE_P2P;

		(*pu2Offset) += P2P_IE_OUI_HDR;

		/* Overall length of all Attributes */
		u4OverallAttriLen = 0;

		for (i = 0; i < u4AttriTableSize; i++) {

			if (arAppendAttriTable[i].pfnAppendAttri) {
				u4AttriLen =
					arAppendAttriTable[i]
						.pfnAppendAttri(prAdapter,
						ucBssIndex, fgIsAssocFrame,
						pu2Offset, pucBuf, u2BufSize);

				u4OverallAttriLen += u4AttriLen;

				if (u4OverallAttriLen
					> P2P_MAXIMUM_ATTRIBUTE_LEN) {
					u4OverallAttriLen -=
						P2P_MAXIMUM_ATTRIBUTE_LEN;

					prIeP2P->ucLength =
						(VENDOR_OUI_TYPE_LEN +
						P2P_MAXIMUM_ATTRIBUTE_LEN);

					pucBuffer = (uint8_t *)
						((unsigned long)
						prIeP2P +
						(VENDOR_OUI_TYPE_LEN +
						P2P_MAXIMUM_ATTRIBUTE_LEN));

					prIeP2P = (struct IE_P2P *)
						((unsigned long) prIeP2P +
						(ELEM_HDR_LEN +
						(VENDOR_OUI_TYPE_LEN +
						P2P_MAXIMUM_ATTRIBUTE_LEN)));

					kalMemCopy(aucTempBuffer,
						pucBuffer, u4OverallAttriLen);

					prIeP2P->ucId = ELEM_ID_P2P;

					prIeP2P->aucOui[0] = aucWfaOui[0];
					prIeP2P->aucOui[1] = aucWfaOui[1];
					prIeP2P->aucOui[2] = aucWfaOui[2];
					prIeP2P->ucOuiType =
						VENDOR_OUI_TYPE_P2P;

					kalMemCopy(prIeP2P->aucP2PAttributes,
						aucTempBuffer,
						u4OverallAttriLen);

					(*pu2Offset) += P2P_IE_OUI_HDR;
				}

			}

		}

		prIeP2P->ucLength =
			(uint8_t) (VENDOR_OUI_TYPE_LEN + u4OverallAttriLen);

	} while (FALSE);
}				/* p2pFuncGenerateP2P_IE */

uint32_t
p2pFuncAppendAttriStatusForAssocRsp(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN u_int8_t fgIsAssocFrame,
		IN uint16_t *pu2Offset,
		IN uint8_t *pucBuf,
		IN uint16_t u2BufSize)
{
	uint8_t *pucBuffer;
	struct P2P_ATTRI_STATUS *prAttriStatus;
	uint32_t u4AttriLen = 0;

	ASSERT(prAdapter);
	ASSERT(pucBuf);

	if (fgIsAssocFrame)
		return u4AttriLen;
	/* TODO: For assoc request P2P IE check in driver &
	 * return status in P2P IE.
	 */

	pucBuffer = (uint8_t *)
		((unsigned long) pucBuf +
		(unsigned long) (*pu2Offset));

	ASSERT(pucBuffer);
	prAttriStatus = (struct P2P_ATTRI_STATUS *) pucBuffer;

	ASSERT(u2BufSize >= ((*pu2Offset) + (uint16_t) u4AttriLen));

	prAttriStatus->ucId = P2P_ATTRI_ID_STATUS;
	WLAN_SET_FIELD_16(&prAttriStatus->u2Length, P2P_ATTRI_MAX_LEN_STATUS);

	prAttriStatus->ucStatusCode = P2P_STATUS_SUCCESS;

	u4AttriLen = (P2P_ATTRI_HDR_LEN + P2P_ATTRI_MAX_LEN_STATUS);

	(*pu2Offset) += (uint16_t) u4AttriLen;

	return u4AttriLen;
}				/* p2pFuncAppendAttriStatusForAssocRsp */

uint32_t
p2pFuncAppendAttriExtListenTiming(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN u_int8_t fgIsAssocFrame,
		IN uint16_t *pu2Offset,
		IN uint8_t *pucBuf,
		IN uint16_t u2BufSize)
{
	uint32_t u4AttriLen = 0;
	struct P2P_ATTRI_EXT_LISTEN_TIMING *prP2pExtListenTiming =
		(struct P2P_ATTRI_EXT_LISTEN_TIMING *) NULL;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	uint8_t *pucBuffer = NULL;
	struct BSS_INFO *prBssInfo = NULL;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	ASSERT(prAdapter);
	ASSERT(pucBuf);
	ASSERT(prBssInfo);

	if (fgIsAssocFrame)
		return u4AttriLen;
	/* TODO: For extend listen timing. */

	prP2pSpecificBssInfo =
		prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prBssInfo->u4PrivateData];

	u4AttriLen = (P2P_ATTRI_HDR_LEN + P2P_ATTRI_MAX_LEN_EXT_LISTEN_TIMING);

	ASSERT(u2BufSize >= ((*pu2Offset) + (uint16_t) u4AttriLen));

	pucBuffer = (uint8_t *)
		((unsigned long) pucBuf +
		(unsigned long) (*pu2Offset));

	ASSERT(pucBuffer);

	prP2pExtListenTiming = (struct P2P_ATTRI_EXT_LISTEN_TIMING *) pucBuffer;

	prP2pExtListenTiming->ucId = P2P_ATTRI_ID_EXT_LISTEN_TIMING;
	WLAN_SET_FIELD_16(&prP2pExtListenTiming->u2Length,
		P2P_ATTRI_MAX_LEN_EXT_LISTEN_TIMING);
	WLAN_SET_FIELD_16(&prP2pExtListenTiming->u2AvailInterval,
		prP2pSpecificBssInfo->u2AvailabilityInterval);
	WLAN_SET_FIELD_16(&prP2pExtListenTiming->u2AvailPeriod,
		prP2pSpecificBssInfo->u2AvailabilityPeriod);

	(*pu2Offset) += (uint16_t) u4AttriLen;

	return u4AttriLen;
}				/* p2pFuncAppendAttriExtListenTiming */

struct IE_HDR *
p2pFuncGetSpecIE(IN struct ADAPTER *prAdapter,
		IN uint8_t *pucIEBuf,
		IN uint16_t u2BufferLen,
		IN uint8_t ucElemID,
		IN u_int8_t *pfgIsMore)
{
	struct IE_HDR *prTargetIE = (struct IE_HDR *) NULL;
	uint8_t *pucIE = (uint8_t *) NULL;
	uint16_t u2Offset = 0;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			     && (pucIEBuf != NULL));

		pucIE = pucIEBuf;

		if (pfgIsMore)
			*pfgIsMore = FALSE;

		IE_FOR_EACH(pucIE, u2BufferLen, u2Offset) {
			if (IE_ID(pucIE) == ucElemID) {
				if ((prTargetIE) && (pfgIsMore)) {

					*pfgIsMore = TRUE;
					break;
				}
				prTargetIE = (struct IE_HDR *) pucIE;

				if (pfgIsMore == NULL)
					break;

			}
		}

	} while (FALSE);

	return prTargetIE;
}				/* p2pFuncGetSpecIE */

struct P2P_ATTRIBUTE *
p2pFuncGetSpecAttri(IN struct ADAPTER *prAdapter,
		IN uint8_t ucOuiType,
		IN uint8_t *pucIEBuf,
		IN uint16_t u2BufferLen,
		IN uint8_t ucAttriID)
{
	struct IE_P2P *prP2pIE = (struct IE_P2P *) NULL;
	struct P2P_ATTRIBUTE *prTargetAttri = (struct P2P_ATTRIBUTE *) NULL;
	u_int8_t fgIsMore = FALSE;
	uint8_t *pucIE = (uint8_t *) NULL;
	uint16_t u2BufferLenLeft = 0;

	DBGLOG(P2P, INFO,
		"Check AssocReq Oui type %u attri %u for len %u\n",
		ucOuiType, ucAttriID, u2BufferLen);

	do {
		ASSERT_BREAK((prAdapter != NULL)
			     && (pucIEBuf != NULL));

		u2BufferLenLeft = u2BufferLen;
		pucIE = pucIEBuf;

		do {
			fgIsMore = FALSE;
			prP2pIE = (struct IE_P2P *) p2pFuncGetSpecIE(prAdapter,
				pucIE, u2BufferLenLeft,
				ELEM_ID_VENDOR, &fgIsMore);
			if (prP2pIE) {
				ASSERT((unsigned long) prP2pIE
					>= (unsigned long) pucIE);
				u2BufferLenLeft = u2BufferLen -
					(uint16_t) (((unsigned long) prP2pIE) -
					((unsigned long) pucIEBuf));

				DBGLOG(P2P, INFO,
					"Find vendor id %u len %u oui %u more %u LeftLen %u\n",
					IE_ID(prP2pIE), IE_LEN(prP2pIE),
					prP2pIE->ucOuiType, fgIsMore,
					u2BufferLenLeft);

				if (IE_LEN(prP2pIE) > P2P_OUI_TYPE_LEN)
					p2pFuncGetSpecAttriAction(prP2pIE,
						ucOuiType, ucAttriID,
						&prTargetAttri);
				/* P2P_OUI_TYPE_LEN */
				pucIE = (uint8_t *)
					(((unsigned long) prP2pIE) +
					IE_SIZE(prP2pIE));
			}
			/* prP2pIE */
		} while (prP2pIE && fgIsMore && u2BufferLenLeft);

	} while (FALSE);

	return prTargetAttri;
}

/* p2pFuncGetSpecAttri */

/* Code refactoring for AOSP */
static void
p2pFuncGetSpecAttriAction(IN struct IE_P2P *prP2pIE,
		IN uint8_t ucOuiType,
		IN uint8_t ucAttriID,
		OUT struct P2P_ATTRIBUTE **prTargetAttri)
{
	uint8_t *pucAttri = (uint8_t *) NULL;
	uint16_t u2OffsetAttri = 0;
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;

	if (prP2pIE->ucOuiType == ucOuiType) {
		switch (ucOuiType) {
		case VENDOR_OUI_TYPE_WPS:
			aucWfaOui[0] = 0x00;
			aucWfaOui[1] = 0x50;
			aucWfaOui[2] = 0xF2;
			break;
		case VENDOR_OUI_TYPE_P2P:
			break;
		case VENDOR_OUI_TYPE_WPA:
		case VENDOR_OUI_TYPE_WMM:
		case VENDOR_OUI_TYPE_WFD:
		default:
			break;
		}

		if ((prP2pIE->aucOui[0] == aucWfaOui[0]) &&
		    (prP2pIE->aucOui[1] == aucWfaOui[1]) &&
		    (prP2pIE->aucOui[2] == aucWfaOui[2])) {

			u2OffsetAttri = 0;
			pucAttri = prP2pIE->aucP2PAttributes;

			if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
				WSC_ATTRI_FOR_EACH(pucAttri,
					(IE_LEN(prP2pIE) - P2P_IE_OUI_HDR),
					u2OffsetAttri) {
					if (WSC_ATTRI_ID(pucAttri)
						== ucAttriID) {
						*prTargetAttri =
							(struct P2P_ATTRIBUTE *)
							pucAttri;
						break;
					}

				}

			} else if (ucOuiType == VENDOR_OUI_TYPE_P2P) {
				P2P_ATTRI_FOR_EACH(pucAttri,
					(IE_LEN(prP2pIE) - P2P_IE_OUI_HDR),
					u2OffsetAttri) {
					if (ATTRI_ID(pucAttri)
						== ucAttriID) {
						*prTargetAttri =
							(struct P2P_ATTRIBUTE *)
							pucAttri;
						break;
					}
				}

			}
#if CFG_SUPPORT_WFD
			else if (ucOuiType == VENDOR_OUI_TYPE_WFD) {
				WFD_ATTRI_FOR_EACH(pucAttri,
					(IE_LEN(prP2pIE) - P2P_IE_OUI_HDR),
					u2OffsetAttri) {
					if (ATTRI_ID(pucAttri)
						== (uint8_t) ucAttriID) {
						*prTargetAttri =
							(struct P2P_ATTRIBUTE *)
							pucAttri;
						break;
					}
				}
			}
#endif
			else {
				/* Todo:: Nothing */
				/* Possible or else. */
			}
		}
	}			/* ucOuiType */
}

uint32_t
p2pFuncGenerateBeaconProbeRsp(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo,
		IN struct MSDU_INFO *prMsduInfo,
		IN u_int8_t fgIsProbeRsp)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct WLAN_BEACON_FRAME *prBcnFrame =
		(struct WLAN_BEACON_FRAME *) NULL;
	/* P_APPEND_VAR_IE_ENTRY_T prAppendIeTable =
	 * (P_APPEND_VAR_IE_ENTRY_T)NULL;
	 */

	do {

		ASSERT_BREAK((prAdapter != NULL)
			&& (prBssInfo != NULL)
			&& (prMsduInfo != NULL));

		/* txBcnIETable */

		/* txProbeRspIETable */

		prBcnFrame = (struct WLAN_BEACON_FRAME *) prMsduInfo->prPacket;

		return nicUpdateBeaconIETemplate(prAdapter,
			IE_UPD_METHOD_UPDATE_ALL,
			prBssInfo->ucBssIndex,
			prBssInfo->u2CapInfo,
			(uint8_t *) prBcnFrame->aucInfoElem,
			prMsduInfo->u2FrameLength -
			OFFSET_OF(struct WLAN_BEACON_FRAME, aucInfoElem));

	} while (FALSE);

	return rWlanStatus;
}				/* p2pFuncGenerateBeaconProbeRsp */

uint32_t
p2pFuncComposeBeaconProbeRspTemplate(IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prP2pBssInfo,
		IN uint8_t *pucBcnBuffer,
		IN uint32_t u4BcnBufLen,
		IN u_int8_t fgIsProbeRsp,
		IN struct P2P_PROBE_RSP_UPDATE_INFO *prP2pProbeRspInfo,
		IN u_int8_t fgSynToFW)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct MSDU_INFO *prMsduInfo = (struct MSDU_INFO *) NULL;
	struct WLAN_MAC_HEADER *prWlanBcnFrame =
		(struct WLAN_MAC_HEADER *) NULL;

	uint8_t *pucBuffer = (uint8_t *) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (pucBcnBuffer != NULL)
			&& (prP2pBssInfo != NULL));

		prWlanBcnFrame = (struct WLAN_MAC_HEADER *) pucBcnBuffer;

		if ((prWlanBcnFrame->u2FrameCtrl != MAC_FRAME_BEACON)
			&& (!fgIsProbeRsp)) {
			rWlanStatus = WLAN_STATUS_INVALID_DATA;
			break;
		}

		else if (prWlanBcnFrame->u2FrameCtrl != MAC_FRAME_PROBE_RSP) {
			rWlanStatus = WLAN_STATUS_INVALID_DATA;
			break;
		}

		if (fgIsProbeRsp) {
			ASSERT_BREAK(prP2pProbeRspInfo != NULL);

			if (!prP2pProbeRspInfo->prProbeRspMsduTemplate)
				cnmMgtPktFree(prAdapter,
				prP2pProbeRspInfo->prProbeRspMsduTemplate);

			prP2pProbeRspInfo->prProbeRspMsduTemplate =
				cnmMgtPktAlloc(prAdapter, u4BcnBufLen);

			prMsduInfo = prP2pProbeRspInfo->prProbeRspMsduTemplate;

			if (prMsduInfo == NULL) {
				rWlanStatus = WLAN_STATUS_FAILURE;
				break;
			}

			prMsduInfo->eSrc = TX_PACKET_MGMT;
			prMsduInfo->ucStaRecIndex = 0xFF;
			prMsduInfo->ucBssIndex = prP2pBssInfo->ucBssIndex;

		} else {
			prMsduInfo = prP2pBssInfo->prBeacon;

			if (prMsduInfo == NULL) {
				rWlanStatus = WLAN_STATUS_FAILURE;
				break;
			}

			if (u4BcnBufLen >
				(OFFSET_OF(struct WLAN_BEACON_FRAME,
				aucInfoElem[0]) + MAX_IE_LENGTH)) {
				/* Unexpected error, buffer overflow. */
				ASSERT(FALSE);
				break;
			}

		}

		pucBuffer = (uint8_t *)
			((unsigned long) (prMsduInfo->prPacket) +
			MAC_TX_RESERVED_FIELD);

		kalMemCopy(pucBuffer, pucBcnBuffer, u4BcnBufLen);

		prMsduInfo->fgIs802_11 = TRUE;
		prMsduInfo->u2FrameLength = (uint16_t) u4BcnBufLen;

		if (fgSynToFW)
			rWlanStatus = p2pFuncGenerateBeaconProbeRsp(prAdapter,
				prP2pBssInfo, prMsduInfo, fgIsProbeRsp);

	} while (FALSE);

	return rWlanStatus;

}				/* p2pFuncComposeBeaconTemplate */

uint32_t wfdFuncCalculateWfdIELenForAssocRsp(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		IN struct STA_RECORD *prStaRec)
{

#if CFG_SUPPORT_WFD_COMPOSE_IE
	uint16_t u2EstimatedExtraIELen = 0;
	struct WFD_CFG_SETTINGS *prWfdCfgSettings =
		(struct WFD_CFG_SETTINGS *) NULL;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (prBssInfo->eNetworkType != NETWORK_TYPE_P2P)
		return 0;

	prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);

	if (IS_STA_P2P_TYPE(prStaRec) && (prWfdCfgSettings->ucWfdEnable > 0)) {

		u2EstimatedExtraIELen =
			prAdapter->prGlueInfo->prP2PInfo[0]->u2WFDIELen;
		ASSERT(u2EstimatedExtraIELen < 128);
	}
	return u2EstimatedExtraIELen;

#else
	return 0;
#endif
}				/* wfdFuncCalculateWfdIELenForAssocRsp */

void wfdFuncGenerateWfdIEForAssocRsp(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo)
{

#if CFG_SUPPORT_WFD_COMPOSE_IE
	struct WFD_CFG_SETTINGS *prWfdCfgSettings =
		(struct WFD_CFG_SETTINGS *) NULL;
	struct STA_RECORD *prStaRec;
	uint16_t u2EstimatedExtraIELen;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);

	prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);

	do {
		ASSERT_BREAK((prMsduInfo != NULL) && (prAdapter != NULL));

		prStaRec = cnmGetStaRecByIndex(prAdapter,
			prMsduInfo->ucStaRecIndex);

		if (prStaRec) {
			if (IS_STA_P2P_TYPE(prStaRec)) {

				if (prWfdCfgSettings->ucWfdEnable > 0) {
					u2EstimatedExtraIELen =
						prAdapter->prGlueInfo
						->prP2PInfo[prP2pBssInfo
						->u4PrivateData]->u2WFDIELen;
					if (u2EstimatedExtraIELen > 0) {
						ASSERT(
							u2EstimatedExtraIELen
							< 128);
						ASSERT(sizeof
							(prAdapter->prGlueInfo
							->prP2PInfo
							[prP2pBssInfo
							->u4PrivateData]
							->aucWFDIE) >=
							prAdapter->prGlueInfo
							->prP2PInfo[prP2pBssInfo
							->u4PrivateData]
							->u2WFDIELen);
						kalMemCopy(
							(prMsduInfo->prPacket +
							prMsduInfo
							->u2FrameLength),
							prAdapter->prGlueInfo
							->prP2PInfo
							[prP2pBssInfo
							->u4PrivateData]
							->aucWFDIE,
							u2EstimatedExtraIELen);
						prMsduInfo->u2FrameLength +=
							u2EstimatedExtraIELen;
					}
				}
			}	/* IS_STA_P2P_TYPE */
		} else {
		}
	} while (FALSE);

	return;
#else

	return;
#endif
}				/* wfdFuncGenerateWfdIEForAssocRsp */

void
p2pFuncComposeNoaAttribute(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex,
		OUT uint8_t *aucNoaAttrArray,
		OUT uint32_t *pu4Len)
{
	struct BSS_INFO *prBssInfo = NULL;
	struct P2P_ATTRI_NOA *prNoaAttr = NULL;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo = NULL;
	struct NOA_DESCRIPTOR *prNoaDesc = NULL;
	uint32_t u4NumOfNoaDesc = 0;
	uint32_t i = 0;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	prP2pSpecificBssInfo =
		prAdapter->rWifiVar
			.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

	prNoaAttr = (struct P2P_ATTRI_NOA *) aucNoaAttrArray;

	prNoaAttr->ucId = P2P_ATTRI_ID_NOTICE_OF_ABSENCE;
	prNoaAttr->ucIndex = prP2pSpecificBssInfo->ucNoAIndex;

	if (prP2pSpecificBssInfo->fgEnableOppPS) {
		prNoaAttr->ucCTWOppPSParam =
			P2P_CTW_OPPPS_PARAM_OPPPS_FIELD |
		    (prP2pSpecificBssInfo->u2CTWindow &
		    P2P_CTW_OPPPS_PARAM_CTWINDOW_MASK);
	} else {
		prNoaAttr->ucCTWOppPSParam = 0;
	}

	for (i = 0; i < prP2pSpecificBssInfo->ucNoATimingCount; i++) {
		if (prP2pSpecificBssInfo->arNoATiming[i].fgIsInUse) {
			prNoaDesc = (struct NOA_DESCRIPTOR *)
				&prNoaAttr->aucNoADesc
				[i * sizeof(struct NOA_DESCRIPTOR)];

			prNoaDesc->ucCountType =
				prP2pSpecificBssInfo->arNoATiming[i].ucCount;
			prNoaDesc->u4Duration =
				prP2pSpecificBssInfo->arNoATiming[i].u4Duration;
			prNoaDesc->u4Interval =
				prP2pSpecificBssInfo->arNoATiming[i].u4Interval;
			prNoaDesc->u4StartTime =
				prP2pSpecificBssInfo->arNoATiming[i]
					.u4StartTime;

			u4NumOfNoaDesc++;
		}
	}

	/* include "index" + "OppPs Params" + "NOA descriptors" */
	prNoaAttr->u2Length = 2 +
		u4NumOfNoaDesc * sizeof(struct NOA_DESCRIPTOR);

	/* include "Attribute ID" + "Length" + "index" +
	 * "OppPs Params" + "NOA descriptors"
	 */
	*pu4Len = P2P_ATTRI_HDR_LEN + prNoaAttr->u2Length;
}

uint32_t p2pFuncCalculateP2P_IE_NoA(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		IN struct STA_RECORD *prStaRec)
{
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo = NULL;
	uint8_t ucIdx;
	uint32_t u4NumOfNoaDesc = 0;
	struct BSS_INFO *prBssInfo;

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	if (p2pFuncIsAPMode(
		prAdapter->rWifiVar.prP2PConnSettings
		[prBssInfo->u4PrivateData]))
		return 0;

	prP2pSpecificBssInfo =
		prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prBssInfo->u4PrivateData];

	for (ucIdx = 0;
		ucIdx < prP2pSpecificBssInfo->ucNoATimingCount; ucIdx++) {
		if (prP2pSpecificBssInfo->arNoATiming[ucIdx].fgIsInUse)
			u4NumOfNoaDesc++;
	}

	/* include "index" + "OppPs Params" + "NOA descriptors" */
	/* include "Attribute ID" + "Length" + "index" +
	 * "OppPs Params" + "NOA descriptors"
	 */
	return P2P_ATTRI_HDR_LEN + 2 +
		(u4NumOfNoaDesc * sizeof(struct NOA_DESCRIPTOR));
}

void p2pFuncGenerateP2P_IE_NoA(IN struct ADAPTER *prAdapter,
		IN struct MSDU_INFO *prMsduInfo)
{
	struct IE_P2P *prIeP2P;
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;
	uint32_t u4AttributeLen;
	struct BSS_INFO *prBssInfo;

	prBssInfo =
		prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];

	if (p2pFuncIsAPMode(
		prAdapter->rWifiVar.prP2PConnSettings
		[prBssInfo->u4PrivateData]))
		return;

	prIeP2P = (struct IE_P2P *)
		((unsigned long) prMsduInfo->prPacket +
		(uint32_t) prMsduInfo->u2FrameLength);

	prIeP2P->ucId = ELEM_ID_P2P;
	prIeP2P->aucOui[0] = aucWfaOui[0];
	prIeP2P->aucOui[1] = aucWfaOui[1];
	prIeP2P->aucOui[2] = aucWfaOui[2];
	prIeP2P->ucOuiType = VENDOR_OUI_TYPE_P2P;

	/* Compose NoA attribute */
	p2pFuncComposeNoaAttribute(prAdapter,
		prMsduInfo->ucBssIndex,
		prIeP2P->aucP2PAttributes,
		&u4AttributeLen);

	prIeP2P->ucLength = VENDOR_OUI_TYPE_LEN + u4AttributeLen;

	prMsduInfo->u2FrameLength += (ELEM_HDR_LEN + prIeP2P->ucLength);

}

void p2pFunCleanQueuedMgmtFrame(IN struct ADAPTER *prAdapter,
		IN struct P2P_QUEUED_ACTION_FRAME *prFrame)
{
	if (prAdapter == NULL || prFrame == NULL || prFrame->u2Length == 0 ||
			prFrame->prHeader == NULL)
		return;

	DBGLOG(P2P, INFO, "Clean queued p2p action frame.\n");

	prFrame->ucRoleIdx = 0;
	prFrame->u4Freq = 0;
	prFrame->u2Length = 0;
	cnmMemFree(prAdapter, prFrame->prHeader);
	prFrame->prHeader = NULL;
}

void p2pFuncSwitchSapChannel(
		IN struct ADAPTER *prAdapter)
{
	u_int8_t fgEnable = FALSE;
	u_int8_t fgDbDcModeEn = FALSE;
	u_int8_t fgIsSapDfs = FALSE;
	struct BSS_INFO *prP2pBssInfo =
		(struct BSS_INFO *) NULL;
	struct BSS_INFO *prAisBssInfo =
		(struct BSS_INFO *) NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	uint8_t ucStaChannelNum = 0;
	uint8_t ucSapChannelNum = 0;
	enum ENUM_BAND eStaBand = BAND_NULL;
	enum ENUM_BAND eSapBand = BAND_NULL;

#if CFG_SUPPORT_DFS_MASTER && CFG_SUPPORT_IDC_CH_SWITCH
	fgEnable = TRUE;
#endif

	if (!prAdapter
		|| !cnmSapIsConcurrent(prAdapter)
		|| !fgEnable) {
		DBGLOG(P2P, WARN, "Not support concurrent STA + SAP\n");
		goto exit;
	}

	prAisBssInfo = aisGetConnectedBssInfo(prAdapter);
	if (!prAisBssInfo) {
		ucStaChannelNum = 0;
#if CFG_SUPPORT_SAP_DFS_CHANNEL
		/* restore DFS channels table */
		wlanUpdateDfsChannelTable(prAdapter->prGlueInfo,
			-1, /* p2p role index */
			0, /* primary channel */
			0, /* bandwidth */
			0, /* sco */
			0, /* center frequency */
			0 /* eBand */);
#endif
	} else {
		/* Get current channel info */
		ucStaChannelNum = prAisBssInfo->ucPrimaryChannel;
		eStaBand = prAisBssInfo->eBand;
#if CFG_SUPPORT_SAP_DFS_CHANNEL
		/* restore DFS channels table */
		wlanUpdateDfsChannelTable(prAdapter->prGlueInfo,
			-1, /* p2p role index */
			ucStaChannelNum, /* primary channel */
			0, /* bandwidth */
			0, /* sco */
			0, /* center frequency */
			0 /* eBand */);
#endif
		if (eStaBand != BAND_2G4 && eStaBand != BAND_5G) {
			DBGLOG(P2P, WARN, "STA has invalid band\n");
			goto exit;
		}
	}

	/* Assume only one sap bss info */
	prP2pBssInfo = cnmGetSapBssInfo(prAdapter);
	if (!prP2pBssInfo) {
		DBGLOG(P2P, WARN, "SAP is not active\n");
		goto exit;
	}
	prP2pRoleFsmInfo =
		P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
			prP2pBssInfo->u4PrivateData);
	if (!prP2pRoleFsmInfo) {
		DBGLOG(P2P, WARN, "SAP is not active\n");
		goto exit;
	}

	ucSapChannelNum = prP2pBssInfo->ucPrimaryChannel;
	eSapBand = prP2pBssInfo->eBand;
	if (eSapBand != BAND_2G4 && eSapBand != BAND_5G) {
		DBGLOG(P2P, WARN, "SAP has invalid band\n");
		goto exit;
	}

	if (eSapBand == BAND_5G)
		fgIsSapDfs = rlmDomainIsLegalDfsChannel(prAdapter,
			eSapBand, ucSapChannelNum);

	/* STA is not connected */
	if (ucStaChannelNum == 0) {
		if (fgIsSapDfs) {
			/* Choose one 5G channel */
			ucStaChannelNum = 36;
			eStaBand = BAND_5G;
			DBGLOG(P2P, INFO, "[SCC] Choose a channel\n");
		} else {
			DBGLOG(P2P, WARN, "STA is not connected\n");
			goto exit;
		}
	}

	fgDbDcModeEn = prAdapter->rWifiVar.fgDbDcModeEn;

	/* Check channel no */
	if (ucStaChannelNum == ucSapChannelNum) {
		/* Do nothing, i.e. SCC */
		DBGLOG(P2P, INFO, "[SCC] Keep StaCH(%d)\n", ucStaChannelNum);
		goto exit;
	} else if (fgDbDcModeEn == TRUE
		&& (eStaBand != eSapBand) && !fgIsSapDfs) {
		/* Do nothing, i.e. DBDC */
		DBGLOG(P2P, INFO,
			"[DBDC] Keep StaCH(%d), SapCH(%d)(dfs: %u)\n",
			ucStaChannelNum, ucSapChannelNum, fgIsSapDfs);
		goto exit;
	} else {
		/* Otherwise, switch to STA channel, i.e. SCC */

		struct RF_CHANNEL_INFO rRfChnlInfo;

		/* Use sta ch info to do sap ch switch */
		rRfChnlInfo.ucChannelNum = ucStaChannelNum;
		rRfChnlInfo.eBand = eStaBand;
		rRfChnlInfo.ucChnlBw =
			rlmGetBssOpBwByVhtAndHtOpInfo(prP2pBssInfo);

		DBGLOG(P2P, INFO,
			"[SCC] StaCH(%d), SapCH(%d)(dfs: %u)\n",
			ucStaChannelNum, ucSapChannelNum, fgIsSapDfs);

#if(CFG_SUPPORT_DFS_MASTER == 1)
		cnmSapChannelSwitchReq(prAdapter,
			&rRfChnlInfo,
			prP2pBssInfo->u4PrivateData);
#endif
	}

exit:

	DBGLOG(P2P, TRACE, "Check done\n");
	/* return; */
}

uint32_t
p2pFunGetPreferredFreqList(IN struct ADAPTER *prAdapter,
		IN enum ENUM_IFTYPE eIftype, OUT uint32_t *freq_list,
		OUT uint32_t *num_freq_list)
{
	struct BSS_INFO *prAisBssInfo;
	uint8_t ucNumOfChannel;
	uint32_t i;
	struct RF_CHANNEL_INFO *aucChannelList;
	enum ENUM_BAND eBandPrefer;

#if (CFG_SUPPORT_P2PGO_ACS == 1)
	uint8_t eBandSel;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#endif

	prAisBssInfo = aisGetConnectedBssInfo(prAdapter);

	aucChannelList = (struct RF_CHANNEL_INFO *) kalMemAlloc(
			sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM,
			VIR_MEM_TYPE);
	if (!aucChannelList) {
		DBGLOG(P2P, ERROR,
			"Allocate buffer for channel list fail\n");
		return -ENOMEM;
	}
	kalMemZero(aucChannelList,
			sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM);

	DBGLOG(P2P, INFO, "iftype: %d\n", eIftype);

	if (!prAisBssInfo) {
		/* Prefer 5G if STA is NOT connected */
		eBandPrefer = BAND_5G;

#if (CFG_SUPPORT_P2PGO_ACS == 1)
		eBandSel = BIT(BAND_5G);

		if (prWifiVar->ucP2pGoACS == FEATURE_ENABLED) {
			p2pFunGetAcsBestChList(prAdapter,
					eBandSel, MAX_BW_20MHZ, BITS(0, 31),
					BITS(0, 31), BITS(0, 31),
					&ucNumOfChannel, aucChannelList);

		} else
#endif
		{
			rlmDomainGetChnlList(prAdapter, eBandPrefer, TRUE,
				MAX_CHN_NUM, &ucNumOfChannel, aucChannelList);
		}
		DBGLOG(P2P, INFO,
			"ucNumOfChannel: %d\n",
			ucNumOfChannel);
		for (i = 0; i < ucNumOfChannel; i++) {
			freq_list[i] = nicChannelNum2Freq(
				aucChannelList[i].ucChannelNum,
				aucChannelList[i].eBand) / 1000;
			(*num_freq_list)++;
		}
	} else if (prAdapter->rWifiVar.eDbdcMode ==
		ENUM_DBDC_MODE_DISABLED) {
		/* DBDC disabled */
		DBGLOG(P2P, INFO,
			"Prefer SCC, STA operating channel: %d, conn state: %d",
			prAisBssInfo->ucPrimaryChannel,
			prAisBssInfo->eConnectionState);
		freq_list[0] = nicChannelNum2Freq(
			prAisBssInfo->ucPrimaryChannel,
			prAisBssInfo->eBand) / 1000;
		(*num_freq_list)++;
	} else {
		/* DBDC enabled */
		DBGLOG(P2P, INFO,
			"STA operating channel: %d, band: %d, conn state: %d",
			prAisBssInfo->ucPrimaryChannel,
			prAisBssInfo->eBand,
			prAisBssInfo->eConnectionState);

		eBandPrefer = BAND_5G;

		/* Prefer 5G if STA is connected at 2.4G */
		if (prAisBssInfo->eBand == BAND_2G4) {
#if (CFG_SUPPORT_P2PGO_ACS == 1)
			eBandSel = BIT(BAND_5G);

			if (prWifiVar->ucP2pGoACS == FEATURE_ENABLED) {
				p2pFunGetAcsBestChList(prAdapter,
					eBandSel, MAX_BW_20MHZ,
					BITS(0, 31), BITS(0, 31),
					BITS(0, 31),
					&ucNumOfChannel,
					aucChannelList);
			} else
#endif
			{
				rlmDomainGetChnlList(prAdapter, eBandPrefer,
					TRUE,
					MAX_CHN_NUM,
					&ucNumOfChannel,
					aucChannelList);
			}

			for (i = 0; i < ucNumOfChannel; i++) {
				freq_list[i] = nicChannelNum2Freq(
					aucChannelList[i].ucChannelNum,
					aucChannelList[i].eBand) / 1000;
				(*num_freq_list)++;
			}

			/* Add SCC channel */
			freq_list[i] = nicChannelNum2Freq(
				prAisBssInfo->ucPrimaryChannel,
				prAisBssInfo->eBand) / 1000;
				(*num_freq_list)++;
		} else {
			/* Prefer SCC at 5G or
			* DBDC at 2G if STA is connected at 5G band
			*/

			/* Add SCC channel */
			freq_list[0] = nicChannelNum2Freq(
				prAisBssInfo->ucPrimaryChannel,
				prAisBssInfo->eBand) / 1000;
			(*num_freq_list)++;

			/* Add 2G channel */
#if (CFG_SUPPORT_P2PGO_ACS == 1)
			if (prWifiVar->ucP2pGoACS == FEATURE_ENABLED) {
				p2pFunGetAcsBestChList(prAdapter,
					BIT(BAND_2G4), MAX_BW_20MHZ,
					BITS(0, 31), BITS(0, 31),
					BITS(0, 31),
					&ucNumOfChannel, aucChannelList);
			} else
#endif
			{
				rlmDomainGetChnlList(prAdapter, BAND_2G4, TRUE,
					MAX_CHN_NUM,
					&ucNumOfChannel,
					aucChannelList);
			}

			for (i = 0; i < ucNumOfChannel; i++) {
				freq_list[i + 1] = nicChannelNum2Freq(
					aucChannelList[i].ucChannelNum,
					aucChannelList[i].eBand) / 1000;
				(*num_freq_list)++;
			}
		}
	}

	kalMemFree(aucChannelList, VIR_MEM_TYPE,
			sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM);

	return WLAN_STATUS_SUCCESS;
}

enum ENUM_P2P_CONNECT_STATE
p2pFuncGetP2pActionFrameType(IN struct MSDU_INFO *prMgmtMsdu)
{
	struct WLAN_MAC_HEADER *prWlanHdr = (struct WLAN_MAC_HEADER *) NULL;
	struct WLAN_ACTION_FRAME *prActFrame;
	uint8_t *pucVendor = NULL;

	prWlanHdr = (struct WLAN_MAC_HEADER *)
			((unsigned long) prMgmtMsdu->prPacket +
					MAC_TX_RESERVED_FIELD);
	if ((prWlanHdr->u2FrameCtrl & MASK_FRAME_TYPE) != MAC_FRAME_ACTION)
		return P2P_CNN_NORMAL;
	prActFrame = (struct WLAN_ACTION_FRAME *) prWlanHdr;
	if (prActFrame->ucCategory != WLAN_ACTION_PUBLIC)
		return P2P_CNN_NORMAL;
	if (prActFrame->ucAction != WLAN_PA_VENDOR_SPECIFIC)
		return P2P_CNN_NORMAL;
	pucVendor = (uint8_t *) prActFrame + 26;
	if (*(pucVendor + 0) == 0x50 &&
			*(pucVendor + 1) == 0x6f &&
			*(pucVendor + 2) == 0x9a &&
			*(pucVendor + 3) == 0x09)
		return ((uint8_t) *(pucVendor + 4)) + 1;
	else
		return P2P_CNN_NORMAL;
}

u_int8_t
p2pFuncCheckOnRocChnl(IN struct RF_CHANNEL_INFO *prTxChnl,
		IN struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	if (prTxChnl == NULL || prChnlReqInfo == NULL)
		return FALSE;

	if (prTxChnl->ucChannelNum == prChnlReqInfo->ucReqChnlNum &&
			prChnlReqInfo->fgIsChannelRequested)
		return TRUE;

	return FALSE;
}

u_int8_t
p2pFuncNeedWaitRsp(IN struct ADAPTER *prAdapter,
		IN enum ENUM_P2P_CONNECT_STATE eConnState)
{
	switch (eConnState) {
	case P2P_CNN_GO_NEG_REQ:
	case P2P_CNN_GO_NEG_RESP:
	case P2P_CNN_INVITATION_REQ:
	case P2P_CNN_INVITATION_RESP:
	case P2P_CNN_DEV_DISC_REQ:
	case P2P_CNN_PROV_DISC_REQ:
		return TRUE;
	default:
		return FALSE;
	}
}

void
p2pFunClearAllTxReq(IN struct ADAPTER *prAdapter,
		IN struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo)
{
	struct P2P_OFF_CHNL_TX_REQ_INFO *prOffChnlTxPkt =
			(struct P2P_OFF_CHNL_TX_REQ_INFO *) NULL;

	while (!LINK_IS_EMPTY(&(prP2pMgmtTxInfo->rTxReqLink))) {
		LINK_REMOVE_HEAD(&(prP2pMgmtTxInfo->rTxReqLink),
				prOffChnlTxPkt,
				struct P2P_OFF_CHNL_TX_REQ_INFO *);
		if (!prOffChnlTxPkt)
			continue;
		kalP2PIndicateMgmtTxStatus(
				prAdapter->prGlueInfo,
				prOffChnlTxPkt->prMgmtTxMsdu,
				FALSE);
		cnmPktFree(prAdapter, prOffChnlTxPkt->prMgmtTxMsdu);
		cnmMemFree(prAdapter, prOffChnlTxPkt);
	}
}

uint8_t p2pFunGetAcsBestCh(IN struct ADAPTER *prAdapter,
		IN enum ENUM_BAND eBand,
		IN enum ENUM_MAX_BANDWIDTH_SETTING eChnlBw,
		IN uint32_t u4LteSafeChnMask_2G,
		IN uint32_t u4LteSafeChnMask_5G_1,
		IN uint32_t u4LteSafeChnMask_5G_2)
{
	struct RF_CHANNEL_INFO aucChannelList[MAX_PER_BAND_CHN_NUM];
	uint8_t ucNumOfChannel;
	struct PARAM_GET_CHN_INFO *prGetChnLoad;
	uint8_t i;
	struct PARAM_PREFER_CHN_INFO rPreferChannel;

	/* reset */
	rPreferChannel.ucChannel = 0;
	rPreferChannel.u4Dirtiness = 0xFFFFFFFF;

	kalMemZero(aucChannelList,
			sizeof(struct RF_CHANNEL_INFO) * MAX_PER_BAND_CHN_NUM);

	rlmDomainGetChnlList(prAdapter, eBand, TRUE, MAX_PER_BAND_CHN_NUM,
			&ucNumOfChannel, aucChannelList);

	/*
	 * 2. Calculate each channel's dirty score
	 */
	prGetChnLoad = &(prAdapter->rWifiVar.rChnLoadInfo);

	DBGLOG(P2P, INFO, "acs chnl mask=[0x%08x][0x%08x][0x%08x]\n",
			u4LteSafeChnMask_2G,
			u4LteSafeChnMask_5G_1,
			u4LteSafeChnMask_5G_2);

	for (i = 0; i < ucNumOfChannel; i++) {
		uint8_t ucIdx;

		ucIdx = wlanGetChannelIndex(aucChannelList[i].eBand,
				aucChannelList[i].ucChannelNum);
		if (ucIdx >= MAX_PER_BAND_CHN_NUM)
			continue;

		DBGLOG(P2P, TRACE, "idx: %u, ch: %u, d: %d\n",
				ucIdx,
				aucChannelList[i].ucChannelNum,
				prGetChnLoad->rEachChnLoad[ucIdx].u4Dirtiness);

		if (aucChannelList[i].ucChannelNum <= 14) {
			if (!(u4LteSafeChnMask_2G & BIT(
					aucChannelList[i].ucChannelNum)))
				continue;
		} else if ((aucChannelList[i].ucChannelNum >= 36) &&
				(aucChannelList[i].ucChannelNum <= 144)) {
			if (!(u4LteSafeChnMask_5G_1 & BIT(
				(aucChannelList[i].ucChannelNum - 36) / 4)))
				continue;
		} else if ((aucChannelList[i].ucChannelNum >= 149) &&
				(aucChannelList[i].ucChannelNum <= 181)) {
			if (!(u4LteSafeChnMask_5G_2 & BIT(
				(aucChannelList[i].ucChannelNum - 149) / 4)))
				continue;
		}

		if (eBand == BAND_5G && eChnlBw >= MAX_BW_80MHZ &&
				nicGetVhtS1(aucChannelList[i].ucChannelNum,
					VHT_OP_CHANNEL_WIDTH_80) == 0)
			continue;

		if (rPreferChannel.u4Dirtiness >
				prGetChnLoad->rEachChnLoad[ucIdx].u4Dirtiness) {
			rPreferChannel.ucChannel =
				prGetChnLoad->rEachChnLoad[ucIdx].ucChannel;
			rPreferChannel.u4Dirtiness =
				prGetChnLoad->rEachChnLoad[ucIdx].u4Dirtiness;
		}
	}

	return rPreferChannel.ucChannel;
}

#if (CFG_SUPPORT_P2PGO_ACS == 1 ||\
	CFG_SUPPORT_P2P_CSA_ACS == 1)
void p2pFunGetAcsBestChList(IN struct ADAPTER *prAdapter,
		IN uint8_t eBandSel,
		IN enum ENUM_MAX_BANDWIDTH_SETTING eChnlBw,
		IN uint32_t u4LteSafeChnMask_2G,
		IN uint32_t u4LteSafeChnMask_5G_1,
		IN uint32_t u4LteSafeChnMask_5G_2,
		OUT uint8_t *pucSortChannelNumber,
		OUT struct RF_CHANNEL_INFO *prSortChannelList)
{
	struct PARAM_GET_CHN_INFO *prChnLoad;
	struct PARAM_CHN_RANK_INFO *prChnRank;
	uint8_t i, ucInUsedCHNumber = 0, ucBandIdx = BAND_2G4;

	/*
	 * 1. Sort all channels (which are support by current domain)
	*/
	wlanSortChannel(prAdapter, CHNL_SORT_POLICY_BY_CH_DOMAIN);
	/*
	 * 2. Calculate each channel's dirty score
	*/
	prChnLoad = &(prAdapter->rWifiVar.rChnLoadInfo);

	/*
	 * 3. Skip some un-safe channels
	*/
	for (i = 0; i < MAX_CHN_NUM; i++) {
		prChnRank = &(prChnLoad->rChnRankList[i]);

		DBGLOG(P2P, TRACE, "idx:%d band:%d ch:%u d:0x%x\n",
				i,
				prChnRank->eBand,
				prChnRank->ucChannel,
				prChnRank->u4Dirtiness);
		if (prChnRank->u4Dirtiness == 0xFFFFFFFF)
			continue;

		if (!(eBandSel & BIT(prChnRank->eBand)))
			continue;

		if (prChnRank->eBand == BAND_2G4 &&
				prChnRank->ucChannel <= 14) {
			ucBandIdx = BAND_2G4;
			if (!(u4LteSafeChnMask_2G & BIT(prChnRank->ucChannel)))
				continue;
		} else if (prChnRank->eBand == BAND_5G &&
				prChnRank->ucChannel >= 36 &&
				prChnRank->ucChannel <= 144) {
			ucBandIdx = BAND_5G;
			if (!(u4LteSafeChnMask_5G_1 &
				BIT((prChnRank->ucChannel - 36) / 4)))
				continue;
		} else if (prChnRank->eBand == BAND_5G &&
				prChnRank->ucChannel >= 149 &&
				prChnRank->ucChannel <= 181) {
			ucBandIdx = BAND_5G;
			if (!(u4LteSafeChnMask_5G_2 &
				BIT((prChnRank->ucChannel - 149) / 4)))
				continue;
		}

		if (ucBandIdx == BAND_5G && eChnlBw >= MAX_BW_80MHZ &&
				nicGetVhtS1(prChnRank->ucChannel,
					VHT_OP_CHANNEL_WIDTH_80) == 0)
			continue;

		(prSortChannelList+ucInUsedCHNumber)->ucChannelNum =
			prChnRank->ucChannel;
		(prSortChannelList+ucInUsedCHNumber)->eBand =
			ucBandIdx;

		ucInUsedCHNumber++;
	}
	/*
	 * 4. Dump the Result
	*/
	*pucSortChannelNumber = ucInUsedCHNumber;
	for (i = 0; i < ucInUsedCHNumber; i++) {
		DBGLOG(P2P, TRACE, "ACS idx=%d, band[%d] ch[%d]\n",
				i,
				(prSortChannelList+i)->eBand,
				(prSortChannelList+i)->ucChannelNum);
	}
}
#endif

void p2pFunProcessAcsReport(IN struct ADAPTER *prAdapter,
		IN uint8_t ucRoleIndex,
		IN struct PARAM_GET_CHN_INFO *prLteSafeChnInfo,
		IN struct P2P_ACS_REQ_INFO *prAcsReqInfo)
{
	enum ENUM_BAND eBand;
	uint32_t u4LteSafeChnMask_2G;

	if (!prAdapter || !prAcsReqInfo)
		return;

	if (prAcsReqInfo->eHwMode == P2P_VENDOR_ACS_HW_MODE_11B ||
			prAcsReqInfo->eHwMode == P2P_VENDOR_ACS_HW_MODE_11G)
		eBand = BAND_2G4;
	else
		eBand = BAND_5G;

	if (prLteSafeChnInfo && (eBand == BAND_2G4)) {
		struct LTE_SAFE_CHN_INFO *prLteSafeChnList;
		struct RF_CHANNEL_INFO aucChannelList[MAX_2G_BAND_CHN_NUM];
		uint8_t ucNumOfChannel;
		uint8_t i;
		u_int8_t fgIsMaskValid = FALSE;

		kalMemZero(aucChannelList,
			sizeof(struct RF_CHANNEL_INFO) * MAX_2G_BAND_CHN_NUM);

		rlmDomainGetChnlList(prAdapter, eBand, TRUE,
			MAX_2G_BAND_CHN_NUM, &ucNumOfChannel, aucChannelList);

		prLteSafeChnList = &prLteSafeChnInfo->rLteSafeChnList;
		u4LteSafeChnMask_2G = prLteSafeChnList->au4SafeChannelBitmask[
			ENUM_SAFE_CH_MASK_BAND_2G4];

#if CFG_TC1_FEATURE
		/* Restrict 2.4G band channel selection range
		 * to 1/6/11 per customer's request
		 */
		u4LteSafeChnMask_2G &= 0x0842;
#elif CFG_TC10_FEATURE
		/* Restrict 2.4G band channel selection range
		 * to 1~11 per customer's request
		 */
		u4LteSafeChnMask_2G &= 0x0FFE;
#endif
		prAcsReqInfo->u4LteSafeChnMask_2G &= u4LteSafeChnMask_2G;
		for (i = 0; i < ucNumOfChannel; i++) {
			if ((prAcsReqInfo->u4LteSafeChnMask_2G &
					BIT(aucChannelList[i].ucChannelNum)))
				fgIsMaskValid = TRUE;
		}
		if (!fgIsMaskValid) {
			DBGLOG(P2P, WARN,
				"All mask invalid, mark all as valid\n");
			prAcsReqInfo->u4LteSafeChnMask_2G = BITS(1, 14);
		}
	} else if (prLteSafeChnInfo && (eBand == BAND_5G)) {
		/* Add support for 5G FW mask */
		struct LTE_SAFE_CHN_INFO *prLteSafeChnList =
			&prLteSafeChnInfo->rLteSafeChnList;
		uint32_t u4LteSafeChnMask_5G_1 =
			prLteSafeChnList->au4SafeChannelBitmask
			[ENUM_SAFE_CH_MASK_BAND_5G_0];
		uint32_t u4LteSafeChnMask_5G_2 =
			prLteSafeChnList->au4SafeChannelBitmask
			[ENUM_SAFE_CH_MASK_BAND_5G_1];

		prAcsReqInfo->u4LteSafeChnMask_5G_1 &= u4LteSafeChnMask_5G_1;
		prAcsReqInfo->u4LteSafeChnMask_5G_1 &= u4LteSafeChnMask_5G_2;
	}

	prAcsReqInfo->ucPrimaryCh = p2pFunGetAcsBestCh(prAdapter,
			eBand,
			prAcsReqInfo->eChnlBw,
			prAcsReqInfo->u4LteSafeChnMask_2G,
			prAcsReqInfo->u4LteSafeChnMask_5G_1,
			prAcsReqInfo->u4LteSafeChnMask_5G_2);

	p2pFunIndicateAcsResult(prAdapter->prGlueInfo,
			prAcsReqInfo);
}

enum ENUM_CHNL_EXT p2pFunGetSco(IN struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand, uint8_t ucPrimaryCh) {
	enum ENUM_CHNL_EXT eSCO = CHNL_EXT_SCN;
	uint8_t ucSecondChannel;

	if (eBand == BAND_2G4) {
		if (ucPrimaryCh != 14)
			eSCO = (ucPrimaryCh > 7) ? CHNL_EXT_SCB : CHNL_EXT_SCA;
	} else {
		if (regd_is_single_sku_en()) {
			if (rlmDomainIsLegalChannel(prAdapter,
					eBand,
					ucPrimaryCh))
				eSCO = rlmSelectSecondaryChannelType(prAdapter,
						eBand,
						ucPrimaryCh);
		} else {
			struct DOMAIN_INFO_ENTRY *prDomainInfo =
					rlmDomainGetDomainInfo(prAdapter);
			struct DOMAIN_SUBBAND_INFO *prSubband;
			uint8_t i, j;

			for (i = 0; i < MAX_SUBBAND_NUM; i++) {
				prSubband = &prDomainInfo->rSubBand[i];
				if (prSubband->ucBand != eBand)
					continue;
				for (j = 0; j < prSubband->ucNumChannels; j++) {
					if ((prSubband->ucFirstChannelNum +
						j * prSubband->ucChannelSpan) ==
						ucPrimaryCh) {
						eSCO = (j & 1) ?
							CHNL_EXT_SCB :
							CHNL_EXT_SCA;
						break;
					}
				}

				if (j < prSubband->ucNumChannels)
					break;	/* Found */
			}
		}
	}
	/* Check if it is boundary channel
	 * and 40MHz BW is permitted
	*/
	if (eSCO != CHNL_EXT_SCN) {
		ucSecondChannel = (eSCO == CHNL_EXT_SCA)
			? (ucPrimaryCh + CHNL_SPAN_20)
			: (ucPrimaryCh - CHNL_SPAN_20);

		if (!rlmDomainIsLegalChannel(prAdapter,
			eBand,
			ucSecondChannel))
			eSCO = CHNL_EXT_SCN;
	}
	return eSCO;
}

uint8_t p2pFunGetSecCh(IN struct ADAPTER *prAdapter,
		IN enum ENUM_BAND eBand,
		IN enum ENUM_CHNL_EXT eSCO,
		IN uint8_t ucPrimaryCh)
{
	uint8_t ucSecondCh;

	if (eSCO == CHNL_EXT_SCN)
		return 0;

	if (eSCO == CHNL_EXT_SCA)
		ucSecondCh = ucPrimaryCh + CHNL_SPAN_20;
	else
		ucSecondCh = ucPrimaryCh - CHNL_SPAN_20;

	if (!rlmDomainIsLegalChannel(prAdapter, eBand, ucSecondCh))
		ucSecondCh = 0;

	return ucSecondCh;
}

void p2pFunIndicateAcsResult(IN struct GLUE_INFO *prGlueInfo,
		IN struct P2P_ACS_REQ_INFO *prAcsReqInfo)
{
	uint8_t ucVhtBw = VHT_OP_CHANNEL_WIDTH_20_40;

	if (prAcsReqInfo->ucPrimaryCh == 0) {
		if (prAcsReqInfo->eHwMode == P2P_VENDOR_ACS_HW_MODE_11B ||
				prAcsReqInfo->eHwMode ==
					P2P_VENDOR_ACS_HW_MODE_11G) {
			prAcsReqInfo->ucPrimaryCh = AP_DEFAULT_CHANNEL_2G;
		} else {
			prAcsReqInfo->ucPrimaryCh = AP_DEFAULT_CHANNEL_5G;
		}
		DBGLOG(P2P, WARN, "No chosed channel, use default channel %d\n",
				prAcsReqInfo->ucPrimaryCh);
	}

	if (prAcsReqInfo->eChnlBw > MAX_BW_20MHZ) {
		enum ENUM_BAND eBand;
		enum ENUM_CHNL_EXT eSCO;

		eBand = prAcsReqInfo->ucPrimaryCh <= 14 ? BAND_2G4 : BAND_5G;
		eSCO = p2pFunGetSco(prGlueInfo->prAdapter,
				eBand,
				prAcsReqInfo->ucPrimaryCh);

		prAcsReqInfo->ucSecondCh = p2pFunGetSecCh(
				prGlueInfo->prAdapter,
				eBand,
				eSCO,
				prAcsReqInfo->ucPrimaryCh);
	}

	switch (prAcsReqInfo->eChnlBw) {
	case MAX_BW_20MHZ:
	case MAX_BW_40MHZ:
		ucVhtBw = VHT_OP_CHANNEL_WIDTH_20_40;
		break;
	case MAX_BW_80MHZ:
		ucVhtBw = VHT_OP_CHANNEL_WIDTH_80;
		break;
	case MAX_BW_160MHZ:
		ucVhtBw = VHT_OP_CHANNEL_WIDTH_160;
		break;
	case MAX_BW_80_80_MHZ:
		ucVhtBw = VHT_OP_CHANNEL_WIDTH_80P80;
		break;
	default:
		ucVhtBw = VHT_OP_CHANNEL_WIDTH_20_40;
		break;
	}
	prAcsReqInfo->ucCenterFreqS1 = nicGetVhtS1(
			prAcsReqInfo->ucPrimaryCh,
			ucVhtBw);

	if (prAcsReqInfo->eChnlBw != VHT_OP_CHANNEL_WIDTH_80P80)
		prAcsReqInfo->ucCenterFreqS2 = 0;
	else
		DBGLOG(P2P, ERROR, "Not support 80+80 bw.\n");

	prAcsReqInfo->fgIsProcessing = FALSE;
	kalP2pIndicateAcsResult(prGlueInfo,
			prAcsReqInfo->ucRoleIdx,
			prAcsReqInfo->ucPrimaryCh,
			prAcsReqInfo->ucSecondCh,
			prAcsReqInfo->ucCenterFreqS1,
			prAcsReqInfo->ucCenterFreqS2,
			prAcsReqInfo->eChnlBw);
}

void p2pFunCalAcsChnScores(IN struct ADAPTER *prAdapter)
{
	struct BSS_DESC *prBssDesc = NULL;
	struct PARAM_GET_CHN_INFO *prChnLoadInfo;
	struct LINK *prBSSDescList = NULL;
	struct SCAN_INFO *prgScanInfo = NULL;

	if (!prAdapter)
		return;

	prgScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prgScanInfo->rBSSDescList;
	prChnLoadInfo = &prAdapter->rWifiVar.rChnLoadInfo;

	/* Clear old ACS data (APNum, Dirtiness, ...)
	 * and initialize the ch number
	 */
	kalMemZero(&(prAdapter->rWifiVar.rChnLoadInfo),
		sizeof(prAdapter->rWifiVar.rChnLoadInfo));
	wlanInitChnLoadInfoChannelList(prAdapter);

	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList,
			rLinkEntry, struct BSS_DESC) {
		uint8_t ucIdx = wlanGetChannelIndex(
				prBssDesc->eBand,
				prBssDesc->ucChannelNum);

		if (ucIdx >= MAX_CHN_NUM)
			continue;
		prChnLoadInfo->rEachChnLoad[ucIdx].u2APNum++;
	}

	wlanCalculateAllChannelDirtiness(prAdapter);
	wlanSortChannel(prAdapter, CHNL_SORT_POLICY_ALL_CN);
}

enum ENUM_CHNL_SWITCH_POLICY
p2pFunDetermineChnlSwitchPolicy(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		IN struct RF_CHANNEL_INFO *prNewChannelInfo)
{
	enum ENUM_CHNL_SWITCH_POLICY ePolicy = CHNL_SWITCH_POLICY_CSA;

	struct BSS_INFO *prBssInfo;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

#if (CFG_SUPPORT_P2P_CSA == 1)
	if (!prBssInfo || IS_BSS_APGO(prBssInfo))
		return ePolicy;
#endif

#if CFG_SEND_DEAUTH_DURING_CHNL_SWITCH
	/* Send deauth frame to clients:
	 * 1. Cross band
	 * 2. BW > 20MHz
	 */
	if (prNewChannelInfo->eBand == BAND_5G ||
			(prBssInfo && prBssInfo->eBand == BAND_5G &&
				prNewChannelInfo->eBand == BAND_2G4))
		ePolicy = CHNL_SWITCH_POLICY_DEAUTH;
#endif

	return ePolicy;
}

void
p2pFunNotifyChnlSwitch(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIdx,
		enum ENUM_CHNL_SWITCH_POLICY ePolicy,
		IN struct RF_CHANNEL_INFO *prNewChannelInfo)
{
	struct BSS_INFO *prBssInfo;
	struct LINK *prClientList;
	struct STA_RECORD *prCurrStaRec;

	DBGLOG(P2P, INFO, "bss index: %d, policy: %d\n", ucBssIdx, ePolicy);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	prClientList = &prBssInfo->rStaRecOfClientList;

	switch (ePolicy) {
	case CHNL_SWITCH_POLICY_DEAUTH:
		if (prClientList && prClientList->u4NumElem > 0) {
			LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList,
					rLinkEntry, struct STA_RECORD) {
				struct TIMER *prTimer;

				if (!prCurrStaRec)
					break;

				prTimer = &(prCurrStaRec->rDeauthTxDoneTimer);

				p2pFuncDisconnect(prAdapter, prBssInfo,
						prCurrStaRec, TRUE,
						REASON_CODE_DEAUTH_LEAVING_BSS);

				if (!timerPendingTimer(prTimer)) {
					cnmTimerInitTimer(prAdapter,
						prTimer,
						(PFN_MGMT_TIMEOUT_FUNC)
						p2pRoleFsmDeauthTimeout,
						(unsigned long) prCurrStaRec);
					cnmTimerStartTimer(prAdapter,
						prTimer,
						P2P_DEAUTH_TIMEOUT_TIME_MS);
				}
			}
			/* wait for deauth TX done & switch channel */
		} else {
#if(CFG_SUPPORT_DFS_MASTER == 1)
			p2pFunChnlSwitchNotifyDone(prAdapter);
#endif
		}
		break;
	case CHNL_SWITCH_POLICY_CSA:
		/* Set CSA IE */
		prAdapter->rWifiVar.ucChannelSwitchMode = 1;
		prAdapter->rWifiVar.ucNewChannelNumber =
			prNewChannelInfo->ucChannelNum;
#if (CFG_SUPPORT_P2P_CSA == 1)
		prAdapter->rWifiVar.ucChannelSwitchCount =
			prAdapter->rWifiVar.ucP2pCsaCount;
		prAdapter->rWifiVar.ucSecondaryOffset =
			rlmGetScoByChnInfo(prAdapter, prNewChannelInfo);
		prAdapter->rWifiVar.ucNewChannelWidth =
			rlmGetVhtOpBwByBssOpBw(prNewChannelInfo->ucChnlBw);
		prAdapter->rWifiVar.ucNewChannelS1 =
			nicFreq2ChannelNum(
				prNewChannelInfo->u4CenterFreq1 * 1000);
		prAdapter->rWifiVar.ucNewChannelS2 = 0;
#else
		prAdapter->rWifiVar.ucChannelSwitchCount = 5;
#endif

		/* Send Action Frame */
		rlmSendChannelSwitchFrame(prAdapter, prBssInfo->ucBssIndex);

		/* To prevent race condition, we have to set CSA flags
		 * after all CSA parameters are updated. In this way,
		 * we can guarantee that CSA IE will be and only be
		 * reported once in the beacon.
		 */
		prAdapter->rWifiVar.fgCsaInProgress = TRUE;

		/* Update Beacon */
		bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);
		break;
	default:
		DBGLOG(P2P, WARN, "invalid policy for channel switch: %d\n",
			ePolicy);
		break;
	}
}

#if(CFG_SUPPORT_DFS_MASTER == 1)
void
p2pFunChnlSwitchNotifyDone(IN struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	struct MSG_P2P_CSA_DONE *prP2pCsaDoneMsg;

	if (!prAdapter)
		return;

#if (CFG_SUPPORT_P2P_CSA == 1)
	/* check if P2P BSS existed
	* note that we have either P2P or SAP now
	* flow might be changed if P2P/SAP existed
	* at the same time
	*/
	prBssInfo = cnmGetP2pBssInfo(prAdapter);

	if (!prBssInfo)
#endif
	{
		prBssInfo = cnmGetSapBssInfo(prAdapter);
	}

	if (!prBssInfo)
		return;

	prP2pCsaDoneMsg = (struct MSG_P2P_CSA_DONE *) cnmMemAlloc(prAdapter,
			RAM_TYPE_MSG,
			sizeof(*prP2pCsaDoneMsg));

	if (!prP2pCsaDoneMsg) {
		log_dbg(CNM, ERROR, "allocate for prP2pCsaDoneMsg failed!\n");
		return;
	}

	DBGLOG(CNM, INFO, "ucBssIndex = %d\n", prBssInfo->ucBssIndex);

	prP2pCsaDoneMsg->rMsgHdr.eMsgId = MID_CNM_P2P_CSA_DONE;
	prP2pCsaDoneMsg->ucBssIndex = prBssInfo->ucBssIndex;
	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prP2pCsaDoneMsg,
			MSG_SEND_METHOD_BUF);
}
#endif

uint8_t p2pFuncIsBufferableMMPDU(IN struct ADAPTER *prAdapter,
		IN enum ENUM_P2P_CONNECT_STATE eConnState,
		IN struct MSDU_INFO *prMgmtTxMsdu)
{
	struct WLAN_MAC_HEADER *prWlanHdr = (struct WLAN_MAC_HEADER *) NULL;
	uint16_t u2TxFrameCtrl;
	uint8_t fgIsBufferableMMPDU = FALSE;

	prWlanHdr = (struct WLAN_MAC_HEADER *)
		((unsigned long) prMgmtTxMsdu->prPacket +
		MAC_TX_RESERVED_FIELD);

	if (!prWlanHdr) {
		DBGLOG(P2P, ERROR, "prWlanHdr is NULL\n");
		return FALSE;
	}
	u2TxFrameCtrl = prWlanHdr->u2FrameCtrl & MASK_FRAME_TYPE;

	switch (u2TxFrameCtrl) {
	case MAC_FRAME_ACTION:
		switch (eConnState) {
		case P2P_CNN_GO_NEG_REQ:
		case P2P_CNN_GO_NEG_RESP:
		case P2P_CNN_GO_NEG_CONF:
		case P2P_CNN_INVITATION_REQ:
		case P2P_CNN_INVITATION_RESP:
		case P2P_CNN_DEV_DISC_RESP:
		case P2P_CNN_PROV_DISC_REQ:
		case P2P_CNN_PROV_DISC_RESP:
			fgIsBufferableMMPDU = FALSE;
			break;
		default:
			fgIsBufferableMMPDU = TRUE;
			break;
		}
		break;
	case MAC_FRAME_DISASSOC:
	case MAC_FRAME_DEAUTH:
		fgIsBufferableMMPDU = TRUE;
		break;
	default:
		fgIsBufferableMMPDU = FALSE;
		break;
	}
	DBGLOG(P2P, TRACE, "fgIsBufferableMMPDU = %u\n", fgIsBufferableMMPDU);
	return fgIsBufferableMMPDU;
}

#if (CFG_SUPPORT_P2P_CSA_ACS == 1)
void p2pFuncSetCsaChnlScanReq(IN struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prP2pBssInfo = NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo;
	struct MSG_P2P_SCAN_REQUEST *prP2pScanReqMsg;
	struct P2P_SCAN_REQ_INFO *prScanReqInfo;

	uint32_t u4MsgSize = 0;
	uint8_t ucNumOf5gChannel = 0;

	if (!prAdapter) {
		DBGLOG(P2P, WARN, "prAdapter is NULL!\n");
		return;
	}

	prP2pBssInfo = cnmGetP2pBssInfo(prAdapter);

	if (!prP2pBssInfo) {
		DBGLOG(P2P, WARN, "prP2pBssInfo is NULL!\n");
		return;
	}

	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
					prP2pBssInfo->u4PrivateData);

	if (!prP2pRoleFsmInfo) {
		DBGLOG(P2P, WARN, "prP2pRoleFsmInfo is NULL!\n");
		return;
	}

	prScanReqInfo = &(prP2pRoleFsmInfo->rScanReqInfo);
	if (prScanReqInfo->fgIsScanRequest == TRUE) {
		DBGLOG(P2P, WARN, "P2P SCAN is going on, ignore CSA\n");
		return;
	}

	u4MsgSize = sizeof(struct MSG_P2P_SCAN_REQUEST) + (
				MAX_5G_BAND_CHN_NUM *
				sizeof(struct RF_CHANNEL_INFO));

	prP2pScanReqMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG, u4MsgSize);
	if (prP2pScanReqMsg == NULL) {
		DBGLOG(P2P, ERROR, "alloc scan req. fail\n");
		return;
	}

	kalMemSet(prP2pScanReqMsg, 0, u4MsgSize);
	prP2pScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;
	prP2pScanReqMsg->ucBssIdx = prP2pRoleFsmInfo->ucBssIndex;
	prP2pScanReqMsg->i4SsidNum = 0;
	prP2pScanReqMsg->u4IELen = 0;
	prP2pScanReqMsg->eScanReason = SCAN_REASON_CSA;

	/* get non-DFS channel list */
	rlmDomainGetChnlList(prAdapter, BAND_5G, TRUE,
				 MAX_5G_BAND_CHN_NUM, &ucNumOf5gChannel,
				 prP2pScanReqMsg->arChannelListInfo);

	prP2pScanReqMsg->u4NumChannel = (uint32_t)ucNumOf5gChannel;

	p2pRoleFsmRunEventScanRequest(prAdapter,
			(struct MSG_HDR *) prP2pScanReqMsg);

}
#endif

#if CFG_AP_80211KVR_INTERFACE
void p2pFunMulAPAgentBssStatusNotification(
		IN struct ADAPTER *prAdapter,
		IN struct BSS_INFO *prBssInfo)
{
	struct T_MULTI_AP_BSS_STATUS_REPORT *prBssReport;
	struct PARAM_CUSTOM_GET_TX_POWER rGetTxPower;
	bool fgSGIEnable = false;
	uint8_t ucMaxBw = 0;
	uint8_t ucNss = 0;
	uint8_t ucNssLoop = 0;
	uint8_t ucOffset = 0;
	uint8_t ucMcsMap = 0;
	uint32_t u4BufLen = 0;
	int32_t i4Ret = 0;

	fgSGIEnable = IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxShortGI);
	ucMaxBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);
	ucNss = wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);

	prBssReport = kalMemAlloc(sizeof(*prBssReport), VIR_MEM_TYPE);
	if (!prBssReport) {
		DBGLOG(AAA, ERROR, "mem alloc fail\n");
		return;
	}

	kalMemZero(prBssReport, sizeof(*prBssReport));
	/* Interface Index */
	i4Ret = sscanf(prAdapter->prGlueInfo->prP2PInfo[1]->prDevHandler->name,
		"ap%u", &prBssReport->uIfIndex);
	if (i4Ret != 1)
		DBGLOG(P2P, WARN, "read sap index fail: %d\n", i4Ret);

	/* Bssid */
	COPY_MAC_ADDR(prBssReport->mBssid, prBssInfo->aucBSSID);
	/* Status */
	prBssReport->uStatus = prBssInfo->fgIsInUse;
	/* Channel */
	prBssReport->u8Channel = prBssInfo->ucPrimaryChannel;
	/* Operation Class Table E-4 in IEEE802.11-2016. */
	if (prBssInfo->ucPrimaryChannel < 14)
		prBssReport->u8OperClass = 81;
	else if (prBssInfo->ucPrimaryChannel >= 36
		&& prBssInfo->ucPrimaryChannel <= 48)
		prBssReport->u8OperClass = 115;
	else if (prBssInfo->ucPrimaryChannel >= 52
		&& prBssInfo->ucPrimaryChannel <= 64)
		prBssReport->u8OperClass = 118;
	else if (prBssInfo->ucPrimaryChannel >= 149
		&& prBssInfo->ucPrimaryChannel <= 161)
		prBssReport->u8OperClass = 124;
	else
		DBGLOG(P2P, WARN,
			"unknown CH %d for op-class\n",
			prBssInfo->ucPrimaryChannel);
	/* TX Power */
	/*
	 * THIS/CMD are both running in main_thread.
	 * Cannot get result by this CMD
	 */
	kalMemZero(&rGetTxPower, sizeof(struct PARAM_CUSTOM_GET_TX_POWER));
	rGetTxPower.ucCenterChannel = prBssInfo->ucPrimaryChannel;
	rGetTxPower.ucBand = prBssInfo->eBand;
	rGetTxPower.ucDbdcIdx = ENUM_BAND_0;
	wlanoidQueryGetTxPower(prAdapter,
		&rGetTxPower,
		sizeof(struct PARAM_CUSTOM_GET_TX_POWER), &u4BufLen);
	prBssReport->u8Txpower = prAdapter->u4GetTxPower/2;
	/* Band */
	prBssReport->uBand = prBssInfo->eBand;
	/* HT Capability */
	if (RLM_NET_IS_11N(prBssInfo)) {
		prBssReport->uHtCap = (
				(ucNss << SAP_HTCAP_TXSTREAMNUM_OFFSET) |
				(ucNss << SAP_HTCAP_RXSTREAMNUM_OFFSET) |
				(fgSGIEnable << SAP_HTCAP_SGIFOR20M_OFFSET) |
				((prBssInfo->fgAssoc40mBwAllowed
					? fgSGIEnable : 0)
				<< SAP_HTCAP_SGIFOR40M_OFFSET) |
				((prBssInfo->fgAssoc40mBwAllowed ? 1 : 0)
				<< SAP_HTCAP_HTFOR40M_OFFSET)
		);
	}
	/* VHT Capability */
	if (RLM_NET_IS_11AC(prBssInfo)) {
		for (ucNssLoop = 1; ucNssLoop <= 8; ucNssLoop++) {
			ucOffset = (ucNssLoop - 1) * 2;
			ucMcsMap = (ucNssLoop <=
				ucNss ? VHT_CAP_INFO_MCS_MAP_MCS9
				: VHT_CAP_INFO_MCS_NOT_SUPPORTED);
			prBssReport->u16VhtTxMcs |=
				(ucMcsMap << ucOffset);
			prBssReport->u16VhtRxMcs |=
				(ucMcsMap << ucOffset);
		}
		prBssReport->u16VhtCap = (
				(ucNss << SAP_VHTCAP_TXSTREAMNUM_OFFSET) |
				(ucNss << SAP_VHTCAP_RXSTREAMNUM_OFFSET) |
				((ucMaxBw >= MAX_BW_80MHZ ? fgSGIEnable : 0)
					<< SAP_VHTCAP_SGIFOR80M_OFFSET) |
				((ucMaxBw >= MAX_BW_160MHZ ? fgSGIEnable : 0)
					<< SAP_VHTCAP_SGIFOR160M_OFFSET) |
				((ucMaxBw == MAX_BW_80_80_MHZ)
					<< SAP_VHTCAP_VHTFORDUAL80M_OFFSET) |
				((ucMaxBw == MAX_BW_160MHZ)
					<< SAP_VHTCAP_VHTFOR160M_OFFSET) |
				(IS_FEATURE_ENABLED(
					prAdapter->rWifiVar.ucStaVhtBfer)
					<< SAP_VHTCAP_SUBEAMFORMER_OFFSET) |
				(0 << SAP_VHTCAP_MUBEAMFORMER_OFFSET)
		);
	}
	/* HE Capability (not support) */
	prBssReport->u8HeMcsNum = 0;
	kalMemZero(&prBssReport->u8HeMcs, 16);
	kalMemZero(&prBssReport->u16HeCap, sizeof(uint16_t));

	DBGLOG(P2P, INFO,
		"[SAP_Test] uIfIndex=%d\n", prBssReport->uIfIndex);
	DBGLOG(P2P, INFO,
		"[SAP_Test] mBssid=" MACSTR "\n", MAC2STR(prBssReport->mBssid));
	DBGLOG(P2P, INFO,
		"[SAP_Test] uStatus=%d\n", prBssReport->uStatus);
	DBGLOG(P2P, INFO,
		"[SAP_Test] u8Channel=%d\n", prBssReport->u8Channel);
	DBGLOG(P2P, INFO,
		"[SAP_Test] u8OperClass=%d\n", prBssReport->u8OperClass);
	DBGLOG(P2P, INFO,
		"[SAP_Test] u8Txpower=%d\n", prBssReport->u8Txpower);
	DBGLOG(P2P, INFO,
		"[SAP_Test] uBand=%d\n", prBssReport->uBand);
	DBGLOG(P2P, INFO,
		"[SAP_Test] uHtCap=0x%x\n", prBssReport->uHtCap);
	DBGLOG(P2P, INFO,
		"[SAP_Test] u16VhtTxMcs=0x%x\n", prBssReport->u16VhtTxMcs);
	DBGLOG(P2P, INFO,
		"[SAP_Test] u16VhtRxMcs=0x%x\n", prBssReport->u16VhtRxMcs);
	DBGLOG(P2P, INFO,
		"[SAP_Test] u16VhtCap=0x%x\n", prBssReport->u16VhtCap);
	DBGLOG(P2P, INFO,
		"[SAP_Test] u8HeMcsNum=%d\n", prBssReport->u8HeMcsNum);
	DBGLOG_MEM8(P2P, WARN, prBssReport->u8HeMcs, 16);
	DBGLOG(P2P, INFO,
		"[SAP_Test] u16HeCap=0x%x\n", prBssReport->u16HeCap);

	i4Ret = MulAPAgentMontorSendMsg(
		EV_WLAN_MULTIAP_BSS_STATUS_REPORT,
		prBssReport, sizeof(*prBssReport));
	if (i4Ret < 0)
		DBGLOG(AAA, ERROR,
			"EV_WLAN_MULTIAP_BSS_STATUS_REPORT nl send msg failed!\n");

	kalMemFree(prBssReport, VIR_MEM_TYPE, sizeof(*prBssReport));
}
#endif /* CFG_AP_80211KVR_INTERFACE */

#if (CFG_SUPPORT_P2P_CSA == 1)
void p2pFuncSwitchGcChannel(
		struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo)
{
	u_int8_t fgEnable = FALSE;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct P2P_CHNL_REQ_INFO *prChnlReqInfo =
		(struct P2P_CHNL_REQ_INFO *) NULL;
	struct GL_P2P_INFO *prGlueP2pInfo =
		(struct GL_P2P_INFO *) NULL;
	struct RF_CHANNEL_INFO rRfChnlInfo;
	uint8_t role_idx = 0;

#if CFG_SUPPORT_DFS_MASTER
	fgEnable = TRUE;
#endif

	if (!prAdapter || !fgEnable) {
		DBGLOG(P2P, TRACE, "Not support DFS function\n");
		return;
	}

	role_idx = prP2pBssInfo->u4PrivateData;

	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, role_idx);
	if (!prP2pRoleFsmInfo) {
		DBGLOG(P2P, WARN, "GC is not active\n");
		return;
	}

	prChnlReqInfo = &prP2pRoleFsmInfo->rChnlReqInfo;

	if (prChnlReqInfo->ucReqChnlNum == prP2pBssInfo->ucPrimaryChannel) {
		DBGLOG(P2P, WARN, "same channel, no need to switch channel\n");
		return;
	}

	/* Free chandef buffer */
	prGlueP2pInfo = prAdapter->prGlueInfo->prP2PInfo[role_idx];
	if (!prGlueP2pInfo) {
		DBGLOG(P2P, WARN, "p2p glue info is not active\n");
		return;
	}
	if (prGlueP2pInfo->chandef != NULL) {
		if (prGlueP2pInfo->chandef->chan) {
			cnmMemFree(prAdapter,
			    prGlueP2pInfo->chandef->chan);
			prGlueP2pInfo->chandef->chan = NULL;
		}
		cnmMemFree(prAdapter,
			prGlueP2pInfo->chandef);
		prGlueP2pInfo->chandef = NULL;
	}

	/* Sameband CSA */
	/* Update channel parameters & channel request info */
	rlmGetChnlInfoForCSA(prAdapter,
			prP2pBssInfo->eBand,
			prP2pBssInfo->ucPrimaryChannel,
			prP2pBssInfo->ucBssIndex,
			&rRfChnlInfo);

	p2pFuncSetChannel(prAdapter, role_idx, &rRfChnlInfo);

	rlmBssUpdateChannelParams(prAdapter, prP2pBssInfo);

	DBGLOG(P2P, INFO,
		"SCO=%d H1=%d H2=%d H3=%d BW=%d S1=%d S2=%d CH=%d Band=%d TxN=%d RxN=%d\n",
		prP2pBssInfo->eBssSCO,
		prP2pBssInfo->ucHtOpInfo1,
		prP2pBssInfo->u2HtOpInfo2,
		prP2pBssInfo->u2HtOpInfo3,
		prP2pBssInfo->ucVhtChannelWidth,
		prP2pBssInfo->ucVhtChannelFrequencyS1,
		prP2pBssInfo->ucVhtChannelFrequencyS2,
		prP2pBssInfo->ucPrimaryChannel,
		prP2pBssInfo->eBand,
		prP2pBssInfo->ucOpTxNss,
		prP2pBssInfo->ucOpRxNss);

	prChnlReqInfo->ucReqChnlNum = prP2pBssInfo->ucPrimaryChannel;
	prChnlReqInfo->eBand = prP2pBssInfo->eBand;
	prChnlReqInfo->eChnlSco = prP2pBssInfo->eBssSCO;
	prChnlReqInfo->eChannelWidth = prP2pBssInfo->ucVhtChannelWidth;
	prChnlReqInfo->ucCenterFreqS1 = prP2pBssInfo->ucVhtChannelFrequencyS1;
	prChnlReqInfo->ucCenterFreqS2 = prP2pBssInfo->ucVhtChannelFrequencyS2;

	p2pRoleFsmStateTransition(prAdapter,
		prP2pRoleFsmInfo,
		P2P_ROLE_STATE_SWITCH_CHANNEL);
}
#endif /* CFG_SUPPORT_P2P_CSA */

#if CFG_POWER_OFF_CTRL_SUPPORT
void p2pFuncPreReboot(void)
{
	struct wiphy *wiphy = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	struct MSG_P2P_CONNECTION_ABORT *prDisconnMsg = NULL;
	struct BSS_INFO *prBssInfo;
	uint8_t aucBCAddr[] = BC_MAC_ADDR;
	uint8_t ucRoleIdx;
	uint8_t ucBssIdx;

	wiphy = wlanGetWiphy();
	prGlueInfo = (struct GLUE_INFO *) wiphy_priv(wiphy);
	prAdapter = prGlueInfo->prAdapter;

	for (ucRoleIdx = 0; ucRoleIdx < KAL_P2P_NUM; ucRoleIdx++) {
		ucBssIdx = prAdapter->rWifiVar
			.aprP2pRoleFsmInfo[ucRoleIdx]->ucBssIndex;
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
		if (IS_BSS_P2P(prBssInfo) &&
			prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
			prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
			break;
		}
	}

	if (ucRoleIdx == KAL_P2P_NUM)
		return;

	prDisconnMsg =
		(struct MSG_P2P_CONNECTION_ABORT *)
		cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
		sizeof(struct MSG_P2P_CONNECTION_ABORT));

	if (prDisconnMsg == NULL)
		return;

	prDisconnMsg->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_ABORT;
	prDisconnMsg->ucRoleIdx = ucRoleIdx;
	prDisconnMsg->u2ReasonCode = REASON_CODE_DEAUTH_LEAVING_BSS;
	prDisconnMsg->fgSendDeauth = TRUE;
	COPY_MAC_ADDR(prDisconnMsg->aucTargetID, aucBCAddr);

	DBGLOG(P2P, WARN, "Disconnect pre reboot\n");

	mboxSendMsg(prGlueInfo->prAdapter,
		MBOX_ID_0,
		(struct MSG_HDR *) prDisconnMsg,
		MSG_SEND_METHOD_BUF);
}
#endif
