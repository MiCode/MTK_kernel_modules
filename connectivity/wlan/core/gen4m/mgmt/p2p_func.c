// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "precomp.h"
#include "gl_kal.h"

#if CFG_ENABLE_WIFI_DIRECT

#define IS_6G_PSC_CHANNEL(_ch) \
	(((_ch - 5) % 16) == 0)

struct APPEND_VAR_ATTRI_ENTRY txAssocRspAttributesTable[] = {
	{(P2P_ATTRI_HDR_LEN + P2P_ATTRI_MAX_LEN_STATUS), NULL,
		p2pFuncAppendAttriStatusForAssocRsp}
	/* 0 *//* Status */
	, {(P2P_ATTRI_HDR_LEN + P2P_ATTRI_MAX_LEN_EXT_LISTEN_TIMING), NULL,
		p2pFuncAppendAttriExtListenTiming}	/* 8 */
};

struct APPEND_VAR_IE_ENTRY txProbeRspIETable[] = {
	{(ELEM_HDR_LEN + (RATE_NUM_SW - ELEM_MAX_LEN_SUP_RATES)), NULL,
			bssGenerateExtSuppRate_IE}	/* 50 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_ERP), NULL,
			rlmRspGenerateErpIE}	/* 42 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP), NULL,
			rlmRspGenerateHtCapIE}	/* 45 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_HT_OP), NULL,
			rlmRspGenerateHtOpIE}	/* 61 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_TPE), NULL,
			rlmGeneratePwrConstraintIE}	/* 32 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_RSN), NULL,
			rsnGenerateRSNIE}	/* 48 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_OBSS_SCAN), NULL,
			rlmRspGenerateObssScanIE}	/* 74 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP), NULL,
			rlmRspGenerateExtCapIE}	/* 127 */
#if CFG_SUPPORT_802_11AC
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP), NULL,
			rlmRspGenerateVhtCapIE}	/*191 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP), NULL,
			rlmRspGenerateVhtOpIE}	/*192 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_TPE), NULL,
			rlmGenerateVhtTPEIE}	/* 195 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP_MODE_NOTIFICATION), NULL,
			rlmRspGenerateVhtOpNotificationIE}	/*199 */
#endif
#if CFG_SUPPORT_802_11AX
	, {0, heRlmCalculateHeCapIELen,
			heRlmRspGenerateHeCapIE}    /* 255, EXT 35 */
	, {0, heRlmCalculateHeOpIELen,
			heRlmRspGenerateHeOpIE}     /* 255, EXT 36 */
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	, {0, mldCalculateRnrIELen, mldGenerateRnrIE}
	, {0, mldCalculateMlIELen, mldGenerateMlIE}
	, {0, ehtRlmCalculateCapIELen, ehtRlmRspGenerateCapIE}
	, {0, ehtRlmCalculateOpIELen, ehtRlmRspGenerateOpIE}
#endif
	/* contiguous vendor ie will be treat as one ie when sorting */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
			rsnGenerateWpaNoneIE}	/* 221 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WMM_PARAM), NULL,
			mqmGenerateWmmParamIE}	/* 221 */
#if CFG_SUPPORT_MTK_SYNERGY
	, {0, rlmCalculateMTKOuiIELen, rlmGenerateMTKOuiIE} /* 221 */
#endif
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
			rsnGenerateWPAIE}	/* 221 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
	   rsnGenerateOWEIE} /* 221 */
	, {0, p2pCalculateWSCIELen, p2pGenerateWSCIE} /* 221 */
#if CFG_SUPPORT_WFD
	, {0, p2pCalculateWFDIELen, p2pGenerateWFDIE} /* 221 */
#endif
	, {0, p2pCalculateP2PIELen, p2pGenerateP2PIE} /* 221 */
#if CFG_SUPPORT_CUSTOM_VENDOR_IE
	, {0, p2pCalculateVendorIELen, p2pGenerateVendorIE} /* 221 */
#endif
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_RSN), NULL,
			rsnGenerateRSNXIE}	/* 244 */
};

struct P2P_CH_CANDIDATE_FILETER_ENTRY p2pSccOnlyChCandFilterTable[] = {
	{P2P_CROSS_BAND_STA_SCC_FILTER, p2pCrossBandStaSccFilter}
};

struct P2P_CH_CANDIDATE_FILETER_ENTRY p2pSingleApMccFilterTable[] = {
	{P2P_DUAL_A_BAND_FILTER, p2pDualABandFilter},
	{P2P_RFBAND_CHECK_FILTER, p2pRfBandCheckFilter},
	{P2P_ALIVE_BSS_SYNC_FILTER, p2pMccAliveBssSyncFilter},
	{P2P_REMOVE_DFS_CH_FILTER, p2pRemoveDfsChFilter},
	{P2P_USER_PREF_CH_FILTER, p2pUserPrefChFilter},
	{P2P_SET_DEFAULT_CH_FILTER, p2pSetDefaultFilter}
};

struct P2P_CH_CANDIDATE_FILETER_ENTRY p2pBtCoexChCandFilterTable[] = {
	{P2P_BT_DESENSE_CH_FILTER, p2pBtDesenseChFilter}
};

struct P2P_CH_CANDIDATE_FILETER_ENTRY p2pDualApCandFilterTable[] = {
	{P2P_DUAL_AP_CH_FILTER, p2pDualApChFilter}
};

uint8_t g_ucBssIdx;

#if (CFG_SUPPORT_DFS_MASTER == 1)
u_int8_t g_fgManualCac = FALSE;
uint32_t g_u4DriverCacTime;
uint32_t g_u4CacStartBootTime;
uint8_t g_ucRadarDetectMode = FALSE;
uint8_t g_ucRadarDetectCnt;
struct P2P_RADAR_INFO g_rP2pRadarInfo;
uint8_t g_ucDfsState = DFS_STATE_INACTIVE;

static const char * const apucDfsState[DFS_STATE_NUM] = {
	"DFS_STATE_INACTIVE",
	"DFS_STATE_CHECKING",
	"DFS_STATE_ACTIVE",
	"DFS_STATE_DETECTED",
};

static const char * const apucW53RadarType[3] = {
	"Unknown Type",
	"Type 1 (short pulse)",
	"Type 2 (short pulse)"
};

static const char * const apucW56RadarType[12] = {
	"Unknown Type",
	"Type 1 (short pulse)",
	"Type 2 (short pulse)",
	"Type 3 (short pulse)",
	"Type 4 (short pulse)",
	"Type 5 (short pulse)",
	"Type 6 (short pulse)",
	"Type 7 (long pulse)",
	"Type 8 (short pulse)",
	"Type 4 or Type 5 or Type 6 (short pulse)",
	"Type 5 or Type 6 or Type 8 (short pulse)",
	"Type 5 or Type 6 (short pulse)",
};
#endif

static void
p2pFuncParseBeaconVenderId(struct ADAPTER *prAdapter, uint8_t *pucIE,
		struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo,
		uint8_t ucRoleIndex);
#if 0
static void
p2pFuncGetAttriListAction(struct ADAPTER *prAdapter,
		struct IE_P2P *prIe,
		uint8_t ucOuiType,
		uint8_t **pucAttriListStart,
		uint16_t *u2AttriListLen,
		u_int8_t *fgIsAllocMem,
		u_int8_t *fgBackupAttributes,
		uint16_t *u2BufferSize);
#endif

static void
p2pFuncGetSpecAttriAction(struct IE_P2P *prP2pIE,
		uint8_t ucOuiType,
		uint8_t ucAttriID,
		struct P2P_ATTRIBUTE **prTargetAttri);

static void
p2pFunAbortOngoingScan(struct ADAPTER *prAdapter);

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
void p2pFuncRequestScan(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo)
{
	struct MSG_SCN_SCAN_REQ_V2 *prScanReqV2 =
		(struct MSG_SCN_SCAN_REQ_V2 *) NULL;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	const uint8_t *ml;
#endif

#ifdef CFG_SUPPORT_BEAM_PLUS
	/*NFC Beam + Indication */
	struct P2P_FSM_INFO *prP2pFsmInfo = (struct P2P_FSM_INFO *) NULL;
#endif

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
				sizeof(struct MSG_SCN_SCAN_REQ_V2));
		if (!prScanReqV2) {
			DBGLOG(P2P, ERROR,
				"p2pFuncRequestScan: Memory allocation fail, can not send SCAN MSG to scan module\n");
			break;
		}

		kalMemZero(prScanReqV2,	sizeof(struct MSG_SCN_SCAN_REQ_V2));

		prScanReqV2->rMsgHdr.eMsgId = MID_P2P_SCN_SCAN_REQ_V2;
		prScanReqV2->ucSeqNum = ++prScanReqInfo->ucSeqNumOfScnMsg;
		prScanReqV2->ucBssIndex = ucBssIndex;
		prScanReqV2->eScanType = prScanReqInfo->eScanType;
		prScanReqV2->eScanChannel = prScanReqInfo->eChannelSet;
		prScanReqV2->u2IELen = 0;

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
#if (CFG_SUPPORT_WIFI_6G == 1)
					/* Only scan 6G PSC channel */
					if (prDomainInfo->eBand == BAND_6G &&
						((prDomainInfo->
						ucChannelNum - 5) % 16) != 0)
						continue;
#endif
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

				prParamSsid = prScanReqV2->arSsid;

				for (prScanReqV2->ucSSIDNum = 0;
					prScanReqV2->ucSSIDNum <
					prScanReqInfo->ucSsidNum &&
					prScanReqV2->ucSSIDNum <
					CFG_SCAN_SSID_MAX_NUM;
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

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		ml = mldFindMlIE(prScanReqV2->aucIE,
			prScanReqV2->u2IELen, ML_CTRL_TYPE_PROBE_REQ);
		/* use ml prob req if scan req has ml ie, bssid, single chnl */
		if (ml && prScanReqV2->eScanChannel == SCAN_CHANNEL_SPECIFIED &&
		    prScanReqV2->ucChannelListNum == 1 &&
		    UNEQUAL_MAC_ADDR(prScanReqInfo->aucBSSID,
				     "\xff\xff\xff\xff\xff\xff")) {
			DBGLOG(P2P, INFO,
				"ML Probe Req to "MACSTR" chnl=%d band=%d\n",
				MAC2STR(prScanReqInfo->aucBSSID),
				prScanReqV2->arChnlInfoList[0].ucChannelNum,
				prScanReqV2->arChnlInfoList[0].eBand);

			COPY_MAC_ADDR(prScanReqV2->aucExtBssid[0],
				prScanReqInfo->aucBSSID);
			kalMemSet(prScanReqV2->ucBssidMatchSsidInd,
				CFG_SCAN_OOB_MAX_NUM,
				sizeof(prScanReqV2->ucBssidMatchSsidInd));
			prScanReqV2->ucScnFuncMask |=
				ENUM_SCN_USE_PADDING_AS_BSSID;
			prScanReqV2->u4ScnFuncMaskExtend |= ENUM_SCN_ML_PROBE;
			prScanReqV2->u2ChannelMinDwellTime =
				P2P_MLD_SCAN_DEFAULT_MIN_DWELL_TIME;
			prScanReqV2->u2ChannelDwellTime =
				P2P_MLD_SCAN_DEFAULT_DWELL_TIME;
		}
#endif
		prScanReqV2->fgOobRnrParseEn = FALSE;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prScanReqV2,
			MSG_SEND_METHOD_UNBUF);

	} while (FALSE);
}				/* p2pFuncRequestScan */

void p2pFuncCancelScan(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct P2P_SCAN_REQ_INFO *prScanInfo)
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
			MSG_SEND_METHOD_UNBUF);
	} while (FALSE);
}				/* p2pFuncCancelScan */

void p2pFuncGCJoin(struct ADAPTER *prAdapter,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_JOIN_INFO *prP2pJoinInfo)
{
	struct MSG_SAA_FSM_START *prJoinReqMsg =
		(struct MSG_SAA_FSM_START *) NULL;
	struct STA_RECORD *prMainStaRec = NULL;

	uint8_t i;

	if (!prAdapter || !prP2pRoleFsmInfo || !prP2pJoinInfo)
		return;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct STA_RECORD *prStaRec =
			(struct STA_RECORD *) NULL;
		struct BSS_INFO *prP2pBssInfo =
			p2pGetLinkBssInfo(prP2pRoleFsmInfo, i);
		struct BSS_DESC *prBssDesc =
			p2pGetLinkBssDesc(prP2pRoleFsmInfo, i);

		if (!prBssDesc || !prP2pBssInfo) {
			DBGLOG(P2P, ERROR,
				"[%d]: NO Target BSS Descriptor\n", i);
			continue;
		}

		/* Renew op trx nss */
		cnmOpModeGetTRxNss(prAdapter, prP2pBssInfo->ucBssIndex,
				   &prP2pBssInfo->ucOpRxNss,
				   &prP2pBssInfo->ucOpTxNss);

		if (prBssDesc->ucSSIDLen) {
			COPY_SSID(prP2pBssInfo->aucSSID,
				prP2pBssInfo->ucSSIDLen,
				prBssDesc->aucSSID,
				prBssDesc->ucSSIDLen);
		}

		/* 2 <1> We are goin to connect to this BSS */
		prBssDesc->fgIsConnecting |= BIT(prP2pBssInfo->ucBssIndex);

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
			DBGLOG(P2P, ERROR, "Create station record fail\n");
			continue;
		}

		if (prMainStaRec == NULL)
			prMainStaRec = prStaRec;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (mldSingleLink(prAdapter, prStaRec,
				  prP2pBssInfo->ucBssIndex)) {
			prP2pBssInfo->ucLinkIndex =
				prBssDesc->rMlInfo.ucLinkIndex;
			mldStarecJoin(prAdapter,
				prP2pRoleFsmInfo->prP2pMldBssInfo,
				prMainStaRec, prStaRec, prBssDesc);
		}
#endif

		p2pSetLinkStaRec(prP2pRoleFsmInfo, prStaRec, i);
		/* only setup link needs to do SAA */
		if (i == P2P_MAIN_LINK_INDEX) {
			prP2pJoinInfo->prTargetStaRec = prStaRec;
			prP2pJoinInfo->fgIsJoinSuccess = FALSE;
			prP2pJoinInfo->u4BufLength = 0;
		}

		/* 2 <2.1> Sync. to FW domain */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

		if (prP2pBssInfo->eConnectionState
			== MEDIA_STATE_DISCONNECTED) {
			prStaRec->fgIsReAssoc = FALSE;
			prStaRec->ucTxAuthAssocRetryLimit =
				TX_AUTH_ASSOCI_RETRY_LIMIT;
		} else {
			DBGLOG(P2P, ERROR,
				"JOIN INIT: Join Request when connected.\n");
			continue;
		}

		/* 2 <4> Use an appropriate Authentication Algorithm Number
		 * among the ucAvailableAuthTypes.
		 */
		if (prP2pJoinInfo->ucAvailableAuthTypes &
			(uint8_t) AUTH_TYPE_OPEN_SYSTEM) {

			DBGLOG(P2P, TRACE,
				"JOIN INIT: Try to do Authentication with AuthType == OPEN_SYSTEM.\n");

			prP2pJoinInfo->ucAvailableAuthTypes &=
				~(uint8_t) AUTH_TYPE_OPEN_SYSTEM;

			prStaRec->ucAuthAlgNum =
				(uint8_t) AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
		} else if (prP2pJoinInfo->ucAvailableAuthTypes &
			(uint8_t) AUTH_TYPE_SHARED_KEY) {

			DBGLOG(P2P, TRACE,
				"JOIN INIT: Try to do Authentication with AuthType == SHARED_KEY.\n");

			prP2pJoinInfo->ucAvailableAuthTypes &=
				~(uint8_t) AUTH_TYPE_SHARED_KEY;

			prStaRec->ucAuthAlgNum =
				(uint8_t) AUTH_ALGORITHM_NUM_SHARED_KEY;
		} else if (prP2pJoinInfo->ucAvailableAuthTypes &
			(uint8_t) AUTH_TYPE_SAE) {
			DBGLOG(P2P, TRACE,
				"JOIN INIT: Try to do Authentication with AuthType == SAE.\n");

			prP2pJoinInfo->ucAvailableAuthTypes &=
				~(uint8_t) AUTH_TYPE_SAE;

			prStaRec->ucAuthAlgNum =
				(uint8_t) AUTH_ALGORITHM_NUM_SAE;
		} else {
			DBGLOG(P2P, ERROR,
				"JOIN INIT: ucAvailableAuthTypes Error.\n");
			continue;
		}
	}

	do {
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
		prJoinReqMsg->prStaRec = prP2pJoinInfo->prTargetStaRec;

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
p2pFuncUpdateBssInfoForJOIN(struct ADAPTER *prAdapter,
		struct BSS_DESC *prBssDesc,
		struct STA_RECORD *prStaRec,
		struct BSS_INFO *prP2pBssInfo,
		struct SW_RFB *prAssocRspSwRfb)
{
	struct WLAN_ASSOC_RSP_FRAME *prAssocRspFrame =
		(struct WLAN_ASSOC_RSP_FRAME *) NULL;
	uint16_t u2IELength;
	const uint8_t *pucIE;

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

		/* Store limitation about 40Mhz bandwidth capability during
		 * association.
		 */
		prP2pBssInfo->fg40mBwAllowed =
			prP2pBssInfo->fgAssoc40mBwAllowed;
		prP2pBssInfo->fgAssoc40mBwAllowed = FALSE;

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

#if CFG_SUPPORT_TDLS_P2P_OFFCHANNEL
		prP2pBssInfo->fgTdlsIsProhibited =
			prStaRec->fgTdlsIsProhibited;
		prP2pBssInfo->fgTdlsIsChSwProhibited =
			prStaRec->fgTdlsIsChSwProhibited;
#endif /* CFG_SUPPORT_TDLS_P2P_OFFCHANNEL */

		/* 4 <2.3> Setup PHY Attributes and
		 * Basic Rate Set/Operational Rate Set
		 */
		prP2pBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;
		DBGLOG(P2P, INFO, "prP2pBssInfo->ucPhyTypeSet(%02x)\n",
			prP2pBssInfo->ucPhyTypeSet);

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

		prBssDesc->fgIsConnecting &= ~BIT(prP2pBssInfo->ucBssIndex);
		prBssDesc->fgIsConnected |= BIT(prP2pBssInfo->ucBssIndex);

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

void
p2pFuncAddPendingMgmtLinkEntry(struct ADAPTER *prAdapter,
	struct MSG_MGMT_TX_REQUEST *prMgmtTxMsg)
{
	struct GL_P2P_INFO *prGlueP2pInfo = NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo = NULL;
	struct P2P_PENDING_MGMT_INFO *prPendingMgmtInfo = NULL;

	prP2pRoleFsmInfo = p2pFuncGetRoleByBssIdx(prAdapter,
		prMgmtTxMsg->ucBssIdx);

	if (prP2pRoleFsmInfo)
		prGlueP2pInfo = prAdapter->prGlueInfo
			->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex];
	else
		prGlueP2pInfo = prAdapter->prGlueInfo->prP2PInfo[0];

	prPendingMgmtInfo = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(struct P2P_PENDING_MGMT_INFO));
	if (!prPendingMgmtInfo) {
		DBGLOG(P2P, WARN, "Allocate memory fail. cookie:0x%llx\n",
			prMgmtTxMsg->u8Cookie);
		return;
	}

	prPendingMgmtInfo->u8PendingMgmtCookie = prMgmtTxMsg->u8Cookie;
	prPendingMgmtInfo->eBand = prMgmtTxMsg->rChannelInfo.eBand;
	prPendingMgmtInfo->ucChannelNum =
		prMgmtTxMsg->rChannelInfo.ucChannelNum;
	prPendingMgmtInfo->fgIsOffChannel = prMgmtTxMsg->fgIsOffChannel;
	LINK_INSERT_TAIL(&prGlueP2pInfo->rWaitTxDoneLink,
		&prPendingMgmtInfo->rLinkEntry);

	DBGLOG(P2P, TRACE,
		"Add pending mgmt TX cookie:0x%llx eBand:%d ucChannelNum:%u\n",
		prPendingMgmtInfo->u8PendingMgmtCookie,
		prPendingMgmtInfo->eBand, prPendingMgmtInfo->ucChannelNum);
}

void
p2pFuncRemovePendingMgmtLinkEntry(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx, uint64_t u8Cookie)
{
	struct GL_P2P_INFO *prGlueP2pInfo = NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo = NULL;
	struct P2P_PENDING_MGMT_INFO *prPendingMgmtInfo = NULL;
	struct P2P_PENDING_MGMT_INFO *prPendingMgmtInfoNext = NULL;

	prP2pRoleFsmInfo = p2pFuncGetRoleByBssIdx(prAdapter,
		ucBssIdx);

	if (prP2pRoleFsmInfo)
		prGlueP2pInfo = prAdapter->prGlueInfo
			->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex];
	else
		prGlueP2pInfo = prAdapter->prGlueInfo->prP2PInfo[0];

	LINK_FOR_EACH_ENTRY_SAFE(prPendingMgmtInfo,
		prPendingMgmtInfoNext, &prGlueP2pInfo->rWaitTxDoneLink,
		rLinkEntry, struct P2P_PENDING_MGMT_INFO) {
		if (prPendingMgmtInfo->u8PendingMgmtCookie ==
			u8Cookie) {
			LINK_REMOVE_KNOWN_ENTRY(
				&prGlueP2pInfo->rWaitTxDoneLink,
				&prPendingMgmtInfo->rLinkEntry);
			cnmMemFree(prAdapter,
				prPendingMgmtInfo);
			DBGLOG(P2P, TRACE,
				"Remove pending mgmt TX cookie:0x%llx\n",
				u8Cookie);
			break;
		}
	}
}

uint32_t
p2pFuncIsPendingTxMgmtNeedWait(struct ADAPTER *prAdapter, uint8_t ucRoleIndex,
	enum ENUM_P2P_MGMT_TX_TYPE eP2pMgmtTxType)
{
	struct GL_P2P_INFO *prGlueP2pInfo = NULL;
	struct P2P_PENDING_MGMT_INFO *prPendingMgmtInfo = NULL;
	struct P2P_PENDING_MGMT_INFO *prPendingMgmtInfoNext = NULL;

	prGlueP2pInfo = prAdapter->prGlueInfo->prP2PInfo[ucRoleIndex];

	if (eP2pMgmtTxType == P2P_MGMT_REMAIN_ON_CH_TX) {
		struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo = NULL;
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo = NULL;
		enum ENUM_BAND eBand;
		uint8_t ucChannelNum;

		prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;
		if (prP2pDevFsmInfo == NULL)
			return FALSE;

		prChnlReqInfo = &(prP2pDevFsmInfo->rChnlReqInfo);

		if ((prChnlReqInfo->fgIsChannelRequested) &&
			prChnlReqInfo->eChnlReqType == CH_REQ_TYPE_ROC) {
			eBand = prChnlReqInfo->eBand;
			ucChannelNum = prChnlReqInfo->ucReqChnlNum;
		} else
			return FALSE;

		LINK_FOR_EACH_ENTRY_SAFE(prPendingMgmtInfo,
			prPendingMgmtInfoNext, &prGlueP2pInfo->rWaitTxDoneLink,
			rLinkEntry, struct P2P_PENDING_MGMT_INFO) {
			/* The non off ch TX frame is rely on RoC channel */
			if (prPendingMgmtInfo->fgIsOffChannel == FALSE)
				return TRUE;

			/* The off ch TX frame is not enqueue if RoC on the
			 * different channel. Only need wait the off ch TX
			 * on the same channel.
			 */
			if (prPendingMgmtInfo->fgIsOffChannel &&
				prPendingMgmtInfo->eBand == eBand &&
				prPendingMgmtInfo->ucChannelNum == ucChannelNum)
				return TRUE;
		}
	} else if (eP2pMgmtTxType == P2P_MGMT_OFF_CH_TX) {
		LINK_FOR_EACH_ENTRY_SAFE(prPendingMgmtInfo,
			prPendingMgmtInfoNext, &prGlueP2pInfo->rWaitTxDoneLink,
			rLinkEntry, struct P2P_PENDING_MGMT_INFO) {
			if (prPendingMgmtInfo->fgIsOffChannel)
				return TRUE;
		}
	}
	return FALSE;
}

uint32_t
p2pFunMgmtFrameTxDone(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		enum ENUM_TX_RESULT_CODE rTxDoneStatus)
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
p2pFuncTagActionActionP2PFrame(struct MSDU_INFO *prMgmtTxMsdu,
		struct WLAN_ACTION_FRAME *prActFrame,
		uint8_t ucP2pAction, uint64_t u8Cookie)
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
p2pFuncTagActionActionFrame(struct MSDU_INFO *prMgmtTxMsdu,
		struct WLAN_ACTION_FRAME *prActFrame,
		uint8_t ucAction, uint64_t u8Cookie)
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

	if (ucAction == WLAN_PA_GAS_INITIAL_REQ ||
		ucAction == WLAN_PA_GAS_INITIAL_RESP)
		nicTxConfigPktControlFlag(prMgmtTxMsdu,
			MSDU_CONTROL_FLAG_FORCE_TX, TRUE);

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
p2pFuncTagActionCategoryFrame(struct MSDU_INFO *prMgmtTxMsdu,
		struct WLAN_ACTION_FRAME *prActFrame,
		uint8_t ucCategory,
		uint64_t u8Cookie)
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

void p2pProcessActionResponse(struct ADAPTER *prAdapter,
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

	DBGLOG(P2P, INFO,
		"eConnState: %d, eType: %d, found P2P_%s\n",
		prAdapter->prP2pInfo->eConnState,
		eType,
		p2pActionFrameToString(eType));

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
p2pFuncTagMgmtFrame(struct MSDU_INFO *prMgmtTxMsdu,
		uint64_t u8Cookie)
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
		((uintptr_t) prMgmtTxMsdu->prPacket +
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

struct MSDU_INFO *p2pFuncProcessAuth(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucBssIdx,
	struct MSDU_INFO *prMgmtTxMsdu)
{
#if (CFG_SUPPORT_802_11BE_MLO == 0)
	return prMgmtTxMsdu;
#else
	struct MSDU_INFO *prRetMsduInfo = NULL;
	struct WLAN_AUTH_FRAME *prAuthFrame = prMgmtTxMsdu->prPacket;
	struct MLD_STA_RECORD *prMldSta;
	int32_t i4Offset;

	prMldSta = mldStarecGetByStarec(prAdapter, prStaRec);
	i4Offset = sortGetPayloadOffset(prAdapter, prMgmtTxMsdu->prPacket);
	if (i4Offset >= 0 && prMldSta) {
		prAuthFrame = prMgmtTxMsdu->prPacket;
		/*
		 * 1. For SAE auth, assume ML IE is appended in wpa_supplicant
		 * 2. For open auth, ML IE is only appended in NEW
		 *    wpa_supplicant. For backward compatible, check ML IE in
		 *    driver before append it.
		 */
		if ((prAuthFrame->u2AuthAlgNum == AUTH_ALGORITHM_NUM_SAE) ||
		    (prAuthFrame->u2AuthAlgNum ==
		     AUTH_ALGORITHM_NUM_OPEN_SYSTEM &&
		     mldFindMlIE(prMgmtTxMsdu->prPacket + i4Offset,
				 prMgmtTxMsdu->u2FrameLength - i4Offset,
				 ML_CTRL_TYPE_BASIC)))
			return prMgmtTxMsdu;
	}

	prRetMsduInfo = cnmMgtPktAlloc(prAdapter,
		(int32_t) (prMgmtTxMsdu->u2FrameLength +
		ELEM_HDR_LEN + MAX_LEN_OF_MLIE +
		MAC_TX_RESERVED_FIELD) +
		sizeof(uint64_t));
	if (!prRetMsduInfo) {
		DBGLOG(P2P, WARN, "alloc fail\n");
		return prMgmtTxMsdu;
	}

	kalMemCopy((uint8_t *)
		((uintptr_t) prRetMsduInfo->prPacket),
		prMgmtTxMsdu->prPacket,
		prMgmtTxMsdu->u2FrameLength);

	prRetMsduInfo->u2FrameLength = prMgmtTxMsdu->u2FrameLength;

	/* free after copy done */
	cnmMgtPktFree(prAdapter, prMgmtTxMsdu);

	/* update correct bssindex before generate ml ie */
	prRetMsduInfo->ucBssIndex = ucBssIdx;
	prRetMsduInfo->ucStaRecIndex = prStaRec->ucIndex;

	/* IEs from supplicant are sorted already, append ml ie */
	mldGenerateMlIE(prAdapter, prRetMsduInfo);

	/* no need to sort because mld is last element */

	return prRetMsduInfo;
#endif
}

struct MSDU_INFO *p2pFuncProcessP2pAssocResp(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucBssIdx,
	struct MSDU_INFO *prMgmtTxMsdu)
{
	struct WIFI_VAR *prWifiVar;
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo;
	struct MSDU_INFO *prMsduInfo;
	struct WLAN_ASSOC_RSP_FRAME *prAssocRspFrame;
	uint8_t *pucIEBuf;
	uint16_t u2Offset, u2IELength, u2RspHdrLen;

	prWifiVar = &prAdapter->rWifiVar;
	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (prP2pBssInfo == NULL ||
	    prP2pBssInfo->u4RsnSelectedAKMSuite != RSN_AKM_SUITE_OWE) {
		DBGLOG(P2P, TRACE, "[OWE] Incorrect akm\n");
		return prMgmtTxMsdu;
	}

	prP2pSpecBssInfo = prWifiVar->prP2pSpecificBssInfo[
		prP2pBssInfo->u4PrivateData];
	prAssocRspFrame = (struct WLAN_ASSOC_RSP_FRAME *)
		((uintptr_t) prMgmtTxMsdu->prPacket +
		MAC_TX_RESERVED_FIELD);

	u2RspHdrLen = (MAC_TX_RESERVED_FIELD +
		       WLAN_MAC_MGMT_HEADER_LEN +
		       CAP_INFO_FIELD_LEN +
		       STATUS_CODE_FIELD_LEN +
		       AID_FIELD_LEN);

	pucIEBuf = prAssocRspFrame->aucInfoElem;
	u2IELength = prMgmtTxMsdu->u2FrameLength - u2RspHdrLen;

	IE_FOR_EACH(pucIEBuf, u2IELength, u2Offset) {
		if (IE_ID(pucIEBuf) == ELEM_ID_RESERVED &&
		    IE_ID_EXT(pucIEBuf) == ELEM_EXT_ID_DIFFIE_HELLMAN_PARAM) {
			prP2pSpecBssInfo->pucDHIEBuf = pucIEBuf;
			prP2pSpecBssInfo->ucDHIELen = IE_SIZE(pucIEBuf);
			break;
		}
	}

	if (!prP2pSpecBssInfo->ucDHIELen) {
		DBGLOG(P2P, WARN, "[OWE] No DH IE\n");
		return prMgmtTxMsdu;
	}

	prMsduInfo = assocComposeReAssocRespFrame(prAdapter, prStaRec);
	if (!prMsduInfo) {
		DBGLOG(P2P, WARN, "[OWE] Compose fail\n");
		return prMgmtTxMsdu;
	}
	cnmMgtPktFree(prAdapter, prMgmtTxMsdu);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldGenerateAssocIE(prAdapter, prStaRec, prMsduInfo,
		assocComposeReAssocRespFrame);
#endif

	prP2pSpecBssInfo->pucDHIEBuf = NULL;
	prP2pSpecBssInfo->ucDHIELen = 0;

	return prMsduInfo;
}

static void p2pFuncMgmtSearchStarec(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t aucAddr[],
	struct BSS_INFO **pprBssInfo,
	struct STA_RECORD **pprStaRec)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_STA_RECORD *prMldSta = NULL;
	struct MLD_BSS_INFO *prMldBss = NULL;
#endif
	struct STA_RECORD *prSta = NULL;
	struct BSS_INFO *prBss = NULL;

	prBss = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBss) {
		DBGLOG(P2P, ERROR,
			"Null prBssInfo by idx(%u)\n",
			ucBssIndex);
		goto done;
	}

	/* no need to search for p2p dev */
	if (prBss->ucBssIndex == prAdapter->ucP2PDevBssIdx)
		goto done;

	prSta = cnmGetStaRecByAddress(prAdapter, ucBssIndex, aucAddr);
	if (prSta)
		goto done;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBss = mldBssGetByBss(prAdapter, prBss);
	prMldSta = mldStarecGetByMldAddr(prAdapter, prMldBss, aucAddr);

	if (!prMldBss) {
		DBGLOG(P2P, ERROR,
			"Null mld bss by bss(%u)\n",
			prBss->ucBssIndex);
		goto done;
	}

	if (prMldSta) {
		prSta = cnmGetStaRecByIndex(prAdapter,
			secGetStaIdxByWlanIdx(prAdapter,
				prMldSta->u2SetupWlanId));
	} else {
		struct LINK *prBssList;
		struct BSS_INFO *prTempBss;

		prBssList = &prMldBss->rBssList;
		LINK_FOR_EACH_ENTRY(prTempBss, prBssList, rLinkEntryMld,
				    struct BSS_INFO) {
			if (!cnmGetStaRecByAddress(prAdapter,
						   prTempBss->ucBssIndex,
						   aucAddr))
				continue;

			prSta = cnmGetStaRecByAddress(prAdapter,
						      prTempBss->ucBssIndex,
						      aucAddr);
			break;
		}
	}

	if (prSta) {
		prBss = GET_BSS_INFO_BY_INDEX(prAdapter, prSta->ucBssIndex);
		if (!prBss) {
			DBGLOG(P2P, ERROR,
				"Null prBssInfo by idx(%u)\n",
				prSta->ucBssIndex);
		}
	}
#endif


done:
	if (prBss)
		*pprBssInfo = prBss;

	if (prSta)
		*pprStaRec = prSta;
}

uint32_t
p2pFuncTxMgmtFrame(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct MSDU_INFO *prMgmtTxMsdu,
		u_int8_t fgNonCckRate)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	/* P_MSDU_INFO_T prTxMsduInfo = (P_MSDU_INFO_T)NULL; */
	struct WLAN_MAC_HEADER *prWlanHdr = (struct WLAN_MAC_HEADER *) NULL;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;
	uint8_t ucRetryLimit = 0;
	uint32_t u4TxLifeTimeInMs = 0;
	u_int8_t fgDrop = FALSE;
	struct BSS_INFO *prBssInfo = NULL;
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
			fgDrop = TRUE;
			break;
		}
		pu8GlCookie =
			(uint64_t *) ((uintptr_t) prMgmtTxMsdu->prPacket +
				(uintptr_t) prMgmtTxMsdu->u2FrameLength +
				MAC_TX_RESERVED_FIELD);

		u8GlCookie = *pu8GlCookie;

		prWlanHdr = (struct WLAN_MAC_HEADER *)
			((uintptr_t) prMgmtTxMsdu->prPacket +
			MAC_TX_RESERVED_FIELD);
		p2pFuncMgmtSearchStarec(prAdapter, ucBssIndex,
					prWlanHdr->aucAddr1,
					&prBssInfo, &prStaRec);
		if (!prBssInfo)
			goto drop;
		/* reassign bss idx again for mlo */
		ucBssIndex = prBssInfo->ucBssIndex;
		ucRetryLimit = prAdapter->rWifiVar.ucP2pMgmtTxRetryLimit;

		switch (prWlanHdr->u2FrameCtrl & MASK_FRAME_TYPE) {
		case MAC_FRAME_PROBE_RSP: {
			struct WLAN_BEACON_FRAME rProbeRspFrame;
			struct GL_P2P_INFO *prP2PInfo;
			struct MSDU_INFO *prNewMgmtTxMsdu;
#if defined(CFG_SUPPORT_PRE_WIFI7)
			uint8_t fgHide = TRUE;
#else
			uint8_t fgHide = FALSE;
#endif

			DBGLOG(P2P, TRACE, "TX Probe Resposne Frame\n");
			if (!nicTxIsMgmtResourceEnough(prAdapter) ||
			    isNetAbsent(prAdapter, prBssInfo)) {
				DBGLOG(P2P, INFO,
					"Drop Tx probe response due to resource issue\n");
				fgDrop = TRUE;
				break;
			} else if (CFG_MTK_P2P_DROP_PROBE_DURING_CSA &&
				   (prAdapter->rWifiVar.fgCsaInProgress ||
				    prBssInfo->fgIsSwitchingChnl)) {
				DBGLOG(P2P, INFO,
					"Drop Tx probe response due to CSA\n");
			}

			prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[
				prBssInfo->u4PrivateData];

			DBGLOG(P2P, TRACE,
				"Dump probe response content from supplicant.\n");
			DBGLOG_MEM8(P2P, TRACE, prMgmtTxMsdu->prPacket,
				prMgmtTxMsdu->u2FrameLength);

			p2pFuncProcessP2pProbeRspAction(prAdapter,
				prMgmtTxMsdu, ucBssIndex);

			/* backup header before free packet from supplicant */
			kalMemCopy(&rProbeRspFrame,
				(uint8_t *)((uintptr_t)prMgmtTxMsdu->prPacket +
				MAC_TX_RESERVED_FIELD),
				sizeof(rProbeRspFrame));

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (prP2PInfo->u2MlIELen != 0) {
				fgHide = FALSE;
			} else if (p2pNeedSkipProbeResp(prAdapter,
							prBssInfo)) {
				fgDrop = TRUE;
				break;
			}
#endif

			/* compose p2p probe rsp frame */
			prNewMgmtTxMsdu = p2pFuncProcessP2pProbeRsp(prAdapter,
				ucBssIndex, FALSE, fgHide,
				&rProbeRspFrame);

			if (prNewMgmtTxMsdu) {
				cnmMgtPktFree(prAdapter, prMgmtTxMsdu);
				prMgmtTxMsdu = prNewMgmtTxMsdu;
			}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (prNewMgmtTxMsdu && prP2PInfo->u2MlIELen != 0) {
				/* temp solution, supplicant only build ml
				 * common info for ml probe resp, so we have to
				 * fill complete per-sta profile when ml ie len
				 * is not 0.
				 */
				mldGenerateProbeRspIE(prAdapter, prMgmtTxMsdu,
					ucBssIndex, &rProbeRspFrame,
					p2pFuncProcessP2pProbeRsp);
			}
#endif

			/* Modifiy Lie time to 100 mS due
			 * to the STA only wait 30-50mS
			 */
			/* and AP do not need send it after STA left */
			/* nicTxSetPktLifeTime(prAdapter, prMgmtTxMsdu, 100); */

			/*
			 * Not check prMsduInfo sanity
			 * as p2pFuncProcessP2pProbeRsp will always
			 * return a MsduInfo
			 */
			pu8GlCookie =
				(uint64_t *) ((uintptr_t)
					prMgmtTxMsdu->prPacket +
					(uintptr_t)
					prMgmtTxMsdu->u2FrameLength +
					MAC_TX_RESERVED_FIELD);
			/* Restore cookie as it will be corrupted
			 * in p2pFuncProcessP2pProbeRsp
			 */
			*pu8GlCookie = u8GlCookie;
			if (ucRetryLimit == 0 ||
			    prAdapter->rWifiVar.ucProbeRspRetryLimit <
			    ucRetryLimit)
				ucRetryLimit =
				prAdapter->rWifiVar.ucProbeRspRetryLimit;
			u4TxLifeTimeInMs = DEFAULT_P2P_PROBERESP_LIFE_TIME;
			DBGLOG(P2P, TRACE,
				"Dump probe response content to FW.\n");
			DBGLOG_MEM8(P2P, TRACE, prMgmtTxMsdu->prPacket,
				prMgmtTxMsdu->u2FrameLength);
			break;
		}
		case MAC_FRAME_ASSOC_RSP:
			/* This case need to fall through */
		case MAC_FRAME_REASSOC_RSP:
			DBGLOG(P2P, TRACE, "[OWE] TX assoc resp Frame\n");
			if (!prStaRec) {
				DBGLOG(AAA, WARN,
					"get sta fail, bss=%d, A1=" MACSTR
					", A2=" MACSTR ", A3=" MACSTR "\n",
					ucBssIndex,
					MAC2STR(prWlanHdr->aucAddr1),
					MAC2STR(prWlanHdr->aucAddr2),
					MAC2STR(prWlanHdr->aucAddr3));
				fgDrop = TRUE;
				break;
			}
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			nicMgmtMAT_M2L(prAdapter, prMgmtTxMsdu,
				       prBssInfo->ucGroupMldId,
				       prStaRec->ucWlanIndex);
#endif
			prMgmtTxMsdu->ucStaRecIndex = prStaRec->ucIndex;
			prMgmtTxMsdu->ucBssIndex = ucBssIndex;
			DBGLOG(P2P, TRACE,
				"[OWE] Dump assoc resp from supplicant.\n");
			DBGLOG_MEM8(P2P, TRACE, prMgmtTxMsdu->prPacket,
					(uint32_t) prMgmtTxMsdu->u2FrameLength);
			prMgmtTxMsdu = p2pFuncProcessP2pAssocResp(prAdapter,
				prStaRec, ucBssIndex, prMgmtTxMsdu);
			pu8GlCookie =
				(uint64_t *) ((uintptr_t)
					prMgmtTxMsdu->prPacket +
					(uintptr_t)
					prMgmtTxMsdu->u2FrameLength +
					MAC_TX_RESERVED_FIELD);
			*pu8GlCookie = u8GlCookie;
			DBGLOG(P2P, TRACE, "[OWE] Dump assoc resp to FW.\n");
			DBGLOG_MEM8(P2P, TRACE, prMgmtTxMsdu->prPacket,
					(uint32_t) prMgmtTxMsdu->u2FrameLength);
			break;
		case MAC_FRAME_AUTH:
			DBGLOG(P2P, TRACE, "TX auth Frame\n");
			if (!prStaRec) {
				DBGLOG(AAA, WARN,
					"get sta fail, bss=%d, A1=" MACSTR
					", A2=" MACSTR ", A3=" MACSTR "\n",
					ucBssIndex,
					MAC2STR(prWlanHdr->aucAddr1),
					MAC2STR(prWlanHdr->aucAddr2),
					MAC2STR(prWlanHdr->aucAddr3));
				fgDrop = TRUE;
				break;
			}
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			nicMgmtMAT_M2L(prAdapter, prMgmtTxMsdu,
				       prBssInfo->ucGroupMldId,
				       prStaRec->ucWlanIndex);
#endif
			prMgmtTxMsdu->ucStaRecIndex = prStaRec->ucIndex;
			prMgmtTxMsdu->ucBssIndex = ucBssIndex;
			DBGLOG(P2P, TRACE,
				"Dump auth from supplicant.\n");
			DBGLOG_MEM8(P2P, TRACE, prMgmtTxMsdu->prPacket,
					(uint32_t) prMgmtTxMsdu->u2FrameLength);

			prMgmtTxMsdu = p2pFuncProcessAuth(prAdapter,
				prStaRec, ucBssIndex, prMgmtTxMsdu);
			pu8GlCookie =
				(uint64_t *) ((uintptr_t)
					prMgmtTxMsdu->prPacket +
					(uintptr_t)
					prMgmtTxMsdu->u2FrameLength +
					MAC_TX_RESERVED_FIELD);
			*pu8GlCookie = u8GlCookie;
			DBGLOG(P2P, TRACE, "Dump auth to FW.\n");
			DBGLOG_MEM8(P2P, TRACE, prMgmtTxMsdu->prPacket,
					(uint32_t) prMgmtTxMsdu->u2FrameLength);
			break;
		default:
			prMgmtTxMsdu->ucBssIndex = ucBssIndex;
			break;
		}

drop:
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

		if (ucRetryLimit)
			nicTxSetPktRetryLimit(prMgmtTxMsdu, ucRetryLimit);
		if (u4TxLifeTimeInMs)
			nicTxSetPktLifeTime(prAdapter,
				prMgmtTxMsdu, u4TxLifeTimeInMs);

		DBGLOG(P2P, LOUD, "ucRetryLimit = %u, u4TxLifeTimeInMs = %u\n",
			ucRetryLimit, u4TxLifeTimeInMs);

		eConnState = p2pFuncTagMgmtFrame(prMgmtTxMsdu, u8GlCookie);
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

void p2pFuncStopComplete(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo)
{
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo;
#if (CFG_SUPPORT_SUSPEND_NOTIFY_APGO_STOP == 1)
	struct GL_P2P_INFO *prP2pInfo;
	uint8_t ucSuspendStopAPGO = 0;
#endif

	if (prAdapter == NULL || prP2pBssInfo == NULL)
		return;

	DBGLOG(P2P, INFO, "bss=%d", prP2pBssInfo->ucBssIndex);

	prP2pSpecificBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo[
		prP2pBssInfo->u4PrivateData];

	/* GO: It would stop Beacon TX.
	 * GC: Stop all BSS related PS function.
	 */
	nicPmIndicateBssAbort(prAdapter, prP2pBssInfo->ucBssIndex);
	/* Reset RLM related field of BSSINFO. */
	rlmBssAborted(prAdapter, prP2pBssInfo);
	prP2pSpecificBssInfo->fgIsGcEapolDone = FALSE;

	if (prP2pBssInfo->ucBMCWlanIndex != WTBL_RESERVED_ENTRY &&
	    prP2pBssInfo->fgBcDefaultKeyExist) {
		struct PARAM_REMOVE_KEY  pvSetBuffer;
		uint32_t pu4SetInfoLen;

		kalMemZero(&pvSetBuffer, sizeof(struct PARAM_REMOVE_KEY));

		pvSetBuffer.u4KeyIndex = prP2pBssInfo->ucBcDefaultKeyIdx;
		pvSetBuffer.ucBssIdx = prP2pBssInfo->ucBssIndex;
		kalMemCopy(pvSetBuffer.arBSSID,
			   prP2pBssInfo->aucBSSID, MAC_ADDR_LEN);

		wlanSetRemoveKey(prAdapter,
				&pvSetBuffer,
				sizeof(struct PARAM_REMOVE_KEY),
				&pu4SetInfoLen, FALSE);
	}

	nicDeactivateNetwork(prAdapter,
		NETWORK_ID(prP2pBssInfo->ucBssIndex,
			   prP2pBssInfo->ucLinkIndex));
	/* Release CNM channel */
	nicUpdateBss(prAdapter, prP2pBssInfo->ucBssIndex);

#if (CFG_SUPPORT_SUSPEND_NOTIFY_APGO_STOP == 1)
	prP2pInfo =
		prAdapter->prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData];
	if (prP2pInfo) {
		ucSuspendStopAPGO =
			KAL_TEST_BIT(SUSPEND_STOP_APGO_WAITING_0,
			prP2pInfo->ulSuspendStopAp);

		if (!completion_done(&prP2pInfo->rSuspendStopApComp) &&
			ucSuspendStopAPGO == TRUE)
			complete(&prP2pInfo->rSuspendStopApComp);
	}
#endif

	if (prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT
#if (CFG_SUPPORT_SUSPEND_NOTIFY_APGO_STOP == 1)
		|| ucSuspendStopAPGO
#endif
		)
		kalP2pNotifyStopApComplete(prAdapter,
			prP2pBssInfo->u4PrivateData);
	else
		kalP2pNotifyDisconnComplete(prAdapter,
			prP2pBssInfo->u4PrivateData);

	if (IS_BSS_APGO(prP2pBssInfo))
		prP2pBssInfo->fgIsApGoStarted = FALSE;

	/* Reset current OPMode */
	prP2pBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;
	prP2pBssInfo->fgBcDefaultKeyExist = FALSE;
	prP2pBssInfo->u4RsnSelectedAKMSuite = 0;

	/* Point StaRecOfAP to NULL when GC role stop Complete */
	prP2pBssInfo->prStaRecOfAP = NULL;
}				/* p2pFuncStopComplete */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will start a P2P Group Owner and send Beacon Frames.
 *
 * @param (none)
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
static void p2pFuncStartGOBcnImpl(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	struct WIFI_VAR *prWifiVar;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo;
	struct P2P_FILS_DISCOVERY_INFO *prFilsInfo;
	struct P2P_UNSOL_PROBE_RESP_INFO *prUnsolProbeInfo;
	uint8_t ucRoleIdx;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucUnsolProbeResp = prAdapter->rWifiVar.ucUnsolProbeResp;
#endif
	ucRoleIdx = prBssInfo->u4PrivateData;
	prWifiVar = &prAdapter->rWifiVar;
	prP2pSpecificBssInfo = prWifiVar->prP2pSpecificBssInfo[ucRoleIdx];
	prFilsInfo = &prP2pSpecificBssInfo->rFilsInfo;
	prUnsolProbeInfo = &prP2pSpecificBssInfo->rUnsolProbeInfo;

	/* 4 <3.2> Reset HW TSF Update Mode and Beacon Mode */
	nicUpdateBss(prAdapter, prBssInfo->ucBssIndex);

	/* 4 <3.3> Update Beacon again
	 * for network phy type confirmed.
	 */
	bssUpdateBeaconContent(prAdapter,
		prBssInfo->ucBssIndex);

#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
	if (p2pFuncProbeRespUpdate(prAdapter,
		prBssInfo,
		prBssInfo->prBeacon->prPacket,
		prBssInfo->prBeacon->u2FrameLength,
		IE_UPD_METHOD_UPDATE_PROBE_RSP) ==
			WLAN_STATUS_FAILURE) {
		DBGLOG(P2P, ERROR,
			"Update probe resp IEs fail!\n");
	}
#endif

	if (prFilsInfo->fgValid) {
		nicUpdateFilsDiscIETemplate(prAdapter,
					    prBssInfo->ucBssIndex,
					    prFilsInfo->u4MaxInterval,
					    prFilsInfo->u4MinInterval,
					    prFilsInfo->aucIEBuf,
					    prFilsInfo->u4Length);
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prBssInfo->eBand == BAND_6G &&
		 (IS_FEATURE_FORCE_ENABLED(ucUnsolProbeResp) ||
		  prUnsolProbeInfo->fgValid)) {
		/* Update unsolicited probe response as beacon */
		bssUpdateBeaconContentEx(prAdapter,
					 prBssInfo->ucBssIndex,
					 IE_UPD_METHOD_UNSOL_PROBE_RSP);
	}
#endif
}

static void p2pFuncStartGOBcn(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo =
		mldBssGetByBss(prAdapter, prBssInfo);
	struct BSS_INFO *bss;

	if (prMldBssInfo) {
		LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList,
				    rLinkEntryMld, struct BSS_INFO) {
			if (bss->fgIsApGoGranted == FALSE)
				continue;

			p2pFuncStartGOBcnImpl(prAdapter, bss);
		}
	} else
#endif
	p2pFuncStartGOBcnImpl(prAdapter, prBssInfo);
}

void
p2pFuncStartGO(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo,
		struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo)
{
#if (CFG_SUPPORT_DFS_MASTER == 1)
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;
#endif
	uint8_t ucRoleIdx;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prBssInfo != NULL));

		if (prBssInfo->ucBssIndex >= prAdapter->ucSwBssIdNum) {
			DBGLOG(P2P, ERROR,
				"P2P BSS exceed the number of P2P interface number.");
			ASSERT(FALSE);
			break;
		}

		DBGLOG(P2P, TRACE, "p2pFuncStartGO:\n");
		ucRoleIdx = prBssInfo->u4PrivateData;
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
		if (prP2pChnlReqInfo->eBand != BAND_5G)
			goto SKIP_START_RDD;

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
		prCmdRddOnOffCtrl->ucRddIdx = rlmDomainGetDfsDbdcBand();
		prCmdRddOnOffCtrl->ucBssIdx = prBssInfo->ucBssIndex;

		DBGLOG(P2P, INFO,
			"Start TXQ - DFS ctrl: %d, RDD index: %d\n",
			prCmdRddOnOffCtrl->ucDfsCtrl,
			prCmdRddOnOffCtrl->ucRddIdx);

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

SKIP_START_RDD:

		/* Re-start AP mode.  */
		p2pFuncSwitchOPMode(prAdapter,
			prBssInfo, prBssInfo->eIntendOPMode, FALSE);

		prBssInfo->eIntendOPMode = OP_MODE_NUM;

		/* 4 <1.1> Assign SSID */
		COPY_SSID(prBssInfo->aucSSID,
			prBssInfo->ucSSIDLen,
			prP2pConnReqInfo->rSsidStruct.aucSsid,
			prP2pConnReqInfo->rSsidStruct.ucSsidLen);

		DBGLOG(P2P, TRACE, "GO SSID:%s\n", HIDE(prBssInfo->aucSSID));

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

		if (prBssInfo->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
			|| prBssInfo->eBand == BAND_6G
#endif
		) {
			/* Depend on eBand */
			prBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11A;
			/* Depend on eCurrentOPMode and ucPhyTypeSet */
			prBssInfo->ucConfigAdHocAPMode = AP_MODE_11A;
		} else if (prP2pConnReqInfo->eConnRequest
			== P2P_CONNECTION_TYPE_PURE_AP) {
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
		if (prP2pConnReqInfo->eConnRequest == P2P_CONNECTION_TYPE_GO) {
			/* Always enable protection at P2P GO */
			prBssInfo->fgIsProtection = TRUE;
		} else {
			ASSERT(prP2pConnReqInfo->eConnRequest
				== P2P_CONNECTION_TYPE_PURE_AP);
			if (kalP2PGetCipher(prAdapter->prGlueInfo, ucRoleIdx))
				prBssInfo->fgIsProtection = TRUE;
		}

		bssInitForAP(prAdapter, prBssInfo, TRUE);
		if (prBssInfo->fgEnableH2E) {
			prBssInfo->aucAllSupportedRates
				[prBssInfo->ucAllSupportedRatesLen]
				= RATE_H2E_ONLY_VAL;
			prBssInfo->ucAllSupportedRatesLen++;
		}

		DBGLOG(P2P, TRACE, "Phy type: 0x%x, %d, %d\n",
			prBssInfo->ucPhyTypeSet,
			prBssInfo->ucConfigAdHocAPMode,
			prBssInfo->ucNonHTBasicPhyType);

#if 0
		if (prBssInfo->ucBMCWlanIndex >= WTBL_SIZE) {
			prBssInfo->ucBMCWlanIndex =
			    secPrivacySeekForBcEntry(prAdapter,
					prBssInfo->ucBssIndex,
					prBssInfo->aucBSSID, 0xff,
					CIPHER_SUITE_NONE, 0xff);
		}
#endif
		nicQmUpdateWmmParms(prAdapter, prBssInfo->ucBssIndex);
#endif /* CFG_SUPPORT_AAA */

		/* 3 <3> Set MAC HW */
		/* 4 <3.1> Setup channel and bandwidth */
		rlmBssInitForAPandIbss(prAdapter, prBssInfo);

		p2pFuncStartGOBcn(prAdapter, prBssInfo);

		/* 4 <3.4> Setup BSSID */
		nicPmIndicateBssCreated(prAdapter, prBssInfo->ucBssIndex);
#if CFG_SUPPORT_IDC_RIL_BRIDGE_NOTIFY
		if (prP2pConnReqInfo->eConnRequest ==
			P2P_CONNECTION_TYPE_PURE_AP)
			kalIdcGetRilInfo();
#endif

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		if (prBssInfo->eBand == BAND_6G) {
			rlmDomain6GPwrModeUpdate(prAdapter,
				prBssInfo->ucBssIndex,
				PWR_MODE_6G_VLP);
		}
#endif

#ifdef CFG_AP_GO_DELAY_CARRIER_ON
		/* Wait for fw's setup done event and continue to
		 * notify carrier_on & start all tx queues to
		 * userspace
		 */
		cnmTimerStartTimer(prAdapter,
				   &(prBssInfo->rP2pApGoCarrierOnTimer),
				   AP_GO_DELAY_CARRIER_ON_TIMEOUT_MS);
#else
		kalP2PTxCarrierOn(prAdapter->prGlueInfo, prBssInfo);
#endif /* CFG_AP_GO_DELAY_CARRIER_ON */


#if (CFG_SUPPORT_DFS_MASTER == 1)
		if (prP2pChnlReqInfo->eBand == BAND_5G &&
		    p2pFuncGetDfsState() == DFS_STATE_DETECTED) {
			struct GL_P2P_INFO *prP2PInfo;
			struct GLUE_INFO *prGlueInfo;

			prGlueInfo = prAdapter->prGlueInfo;
			prP2PInfo = prGlueInfo->prP2PInfo[ucRoleIdx];
			prP2PInfo->fgChannelSwitchReq = TRUE;
			kalP2pIndicateChnlSwitch(prAdapter, prBssInfo);
		}
#endif

		if (prBssInfo &&
			IS_BSS_P2P(prBssInfo) &&
			IS_NET_PWR_STATE_ACTIVE(
				prAdapter,
				prBssInfo->ucBssIndex)) {
			if (p2pFuncIsAPMode(
				  prAdapter->rWifiVar.prP2PConnSettings
				  [ucRoleIdx])) {
				prAdapter->aprSapBssInfo[ucRoleIdx]
					  = prBssInfo;
			} else {
				p2pDevFsmNotifyGoState(prAdapter,
					prBssInfo->ucBssIndex,
					TRUE);
			}
		}

#if CFG_AP_80211KVR_INTERFACE
		/* 5. BSS status notification */
		p2pFunMulAPAgentBssStatusNotification(prAdapter,
			prBssInfo);
#endif /* CFG_AP_80211KVR_INTERFACE */
	} while (FALSE);
}				/* p2pFuncStartGO() */

void p2pFuncStopGO(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo)
{
	uint32_t u4ClientCount = 0;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo;

	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
		prAdapter, prP2pBssInfo->u4PrivateData);
	prP2pRoleFsmInfo->fgIsChannelSelectByAcs = FALSE;

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
		prAdapter->aprSapBssInfo[prP2pBssInfo->u4PrivateData]
			= NULL;

		if (!p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings
			  [prP2pBssInfo->u4PrivateData])) {
			p2pDevFsmNotifyGoState(prAdapter,
				prP2pBssInfo->ucBssIndex, FALSE);
		}

		if ((prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
		    && (prP2pBssInfo->eIntendOPMode == OP_MODE_NUM)) {
			/* AP is created, Beacon Updated. */
			p2pFuncDissolve(prAdapter,
				prP2pBssInfo, TRUE,
				REASON_CODE_DEAUTH_LEAVING_BSS,
				TRUE);
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

uint32_t p2pFuncRoleToBssIdx(struct ADAPTER *prAdapter,
		uint8_t ucRoleIdx, uint8_t *pucBssIdx)
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

struct P2P_ROLE_FSM_INFO *p2pFuncGetRoleByBssIdx(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex)
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
p2pFuncSwitchOPMode(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_OP_MODE eOpMode,
		u_int8_t fgSyncToFW)
{
	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prP2pBssInfo != NULL)
			&& (eOpMode < OP_MODE_NUM));

		if (prP2pBssInfo->eCurrentOPMode != eOpMode) {
			DBGLOG(P2P, TRACE,
				"BSS%d: Switch to from %d, to %d.\n",
				prP2pBssInfo->ucBssIndex,
				prP2pBssInfo->eCurrentOPMode, eOpMode);

			switch (prP2pBssInfo->eCurrentOPMode) {
			case OP_MODE_ACCESS_POINT:
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

#if KAL_P2P_NUM > 1
				/*avoid ap1 Bss  have diff A2 & A3, */
				/*ToDo :  fix for P2P case*/

#else
				COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr,
					prAdapter->rWifiVar
						.aucP2pInterfaceAddress[
						prP2pBssInfo->u4PrivateData]);
				COPY_MAC_ADDR(prP2pBssInfo->aucBSSID,
					prAdapter->rWifiVar
						.aucP2pInterfaceAddress[
						prP2pBssInfo->u4PrivateData]);
#endif
				break;
			case OP_MODE_P2P_DEVICE:
				/* Change device address. */
				DBGLOG(P2P, TRACE,
					"Switch back to P2P Device.\n");

				p2pChangeMediaState(prAdapter,
					prP2pBssInfo,
					MEDIA_STATE_DISCONNECTED);

				COPY_MAC_ADDR(
					prP2pBssInfo->aucOwnMacAddr,
					prAdapter->rWifiVar
						.aucP2pDeviceAddress[0]);
				COPY_MAC_ADDR(
					prP2pBssInfo->aucBSSID,
					prAdapter->rWifiVar
						.aucP2pDeviceAddress[0]);
				break;
			default:
				ASSERT(FALSE);
				break;
			}

			if (1) {
				struct P2P_DISCONNECT_INFO rP2PDisInfo = {0};
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

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			mldBssUpdateCap(prAdapter,
				mldBssGetByBss(prAdapter, prP2pBssInfo),
				NULL);
#endif

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
void p2pFuncReleaseCh(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	struct MSG_CH_ABORT *prMsgChRelease = (struct MSG_CH_ABORT *) NULL;

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
		prMsgChRelease->ucExtraChReqNum = prChnlReqInfo->ucChReqNum - 1;
#if CFG_SUPPORT_DBDC
		if (prMsgChRelease->ucExtraChReqNum >= 1)
			prMsgChRelease->eDBDCBand = ENUM_BAND_ALL;
		else
			prMsgChRelease->eDBDCBand = ENUM_BAND_AUTO;

		DBGLOG(P2P, INFO,
			"P2P abort channel on band %u. ucExtraChReqNum: %d\n",
			prMsgChRelease->eDBDCBand,
			prMsgChRelease->ucExtraChReqNum);
#endif /*CFG_SUPPORT_DBDC*/
		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgChRelease,
			MSG_SEND_METHOD_UNBUF);

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
void p2pFuncAcquireCh(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx, struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
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
		prChnlReqInfo->ucChReqNum = 1;
		prMsgChReq->ucExtraChReqNum = prChnlReqInfo->ucChReqNum - 1;

		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgChReq,
			MSG_SEND_METHOD_UNBUF);

		prChnlReqInfo->fgIsChannelRequested = TRUE;

	} while (FALSE);
}				/* p2pFuncAcquireCh */
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
void p2pFuncSetApNss(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		uint8_t ucOpTxNss,
		uint8_t ucOpRxNss)
{
	struct WIFI_EVENT *pEvent;
	struct EVENT_OPMODE_CHANGE *prEvtOpMode =
		(struct EVENT_OPMODE_CHANGE *) NULL;

	pEvent = (struct WIFI_EVENT *)
		kalMemAlloc(sizeof(struct WIFI_EVENT)+
		sizeof(struct EVENT_OPMODE_CHANGE),
		VIR_MEM_TYPE);
	if (!pEvent)
		return;

	prEvtOpMode = (struct EVENT_OPMODE_CHANGE *)
		&(pEvent->aucBuffer[0]);

	prEvtOpMode->ucBssBitmap = BIT(ucBssIdx);
	prEvtOpMode->ucEnable = TRUE;
	prEvtOpMode->ucOpTxNss = ucOpTxNss;
	prEvtOpMode->ucOpRxNss = ucOpRxNss;
	prEvtOpMode->ucReason =
		EVENT_OPMODE_CHANGE_REASON_USER_CONFIG;
#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
	if (prEvtOpMode->ucEnable) {
		prAdapter->fgANTCtrl = true;
		prAdapter->ucANTCtrlReason =
			EVENT_OPMODE_CHANGE_REASON_ANT_CTRL;
		prAdapter->ucANTCtrlPendingCount = 0;
	}
#endif
	cnmOpmodeEventHandler(
		prAdapter,
		(struct WIFI_EVENT *) pEvent);

	kalMemFree(pEvent, VIR_MEM_TYPE,
		sizeof(struct WIFI_EVENT)+
		sizeof(struct EVENT_OPMODE_CHANGE));

	DBGLOG(P2P, INFO,
			"P2p Set AP Nss tx:%d rx:%d\n",
			ucOpTxNss,
			ucOpRxNss);

}

void p2pFuncSetForceTrxConfig(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		uint8_t ucScenarioConfig)
{
	prAdapter->rWifiVar.ucForceTrxConfig =
		ucScenarioConfig;
	if (ucScenarioConfig == 1) {
		prAdapter->rWifiVar.ucAp2gBandwidth = 0;
		prAdapter->rWifiVar.ucAp5gBandwidth = 0;
		prAdapter->rWifiVar.ucAp6gBandwidth = 0;
	} else if (ucScenarioConfig == 2) {
		prAdapter->rWifiVar.ucAp2gBandwidth = 0;
		prAdapter->rWifiVar.ucAp5gBandwidth = 1;
		prAdapter->rWifiVar.ucAp6gBandwidth = 1;
	}
#if (CFG_SUPPORT_802_11AX == 1)
	if (ucScenarioConfig == 1) {
		prAdapter->rWifiVar.ucHeMaxMcsMap2g =
			HE_CAP_INFO_MCS_MAP_MCS7;
		prAdapter->rWifiVar.ucHeMaxMcsMap5g =
			HE_CAP_INFO_MCS_MAP_MCS7;
		prAdapter->rWifiVar.ucHeMaxMcsMap6g =
			HE_CAP_INFO_MCS_MAP_MCS7;
	} else if (ucScenarioConfig == 2) {
		prAdapter->rWifiVar.ucHeMaxMcsMap2g =
			HE_CAP_INFO_MCS_MAP_MCS9;
		prAdapter->rWifiVar.ucHeMaxMcsMap5g =
			HE_CAP_INFO_MCS_MAP_MCS9;
		prAdapter->rWifiVar.ucHeMaxMcsMap6g =
			HE_CAP_INFO_MCS_MAP_MCS9;
	} else {
		prAdapter->rWifiVar.ucHeMaxMcsMap2g =
			HE_CAP_INFO_MCS_MAP_MCS11;
		prAdapter->rWifiVar.ucHeMaxMcsMap5g =
			HE_CAP_INFO_MCS_MAP_MCS11;
		prAdapter->rWifiVar.ucHeMaxMcsMap6g =
			HE_CAP_INFO_MCS_MAP_MCS11;
	}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	if (ucScenarioConfig == 1) {
		prAdapter->rWifiVar.ucApEht =
			FEATURE_DISABLED;
	} else {
		prAdapter->rWifiVar.ucApEht =
			FEATURE_FORCE_ENABLED;
	}
#endif

}
#endif

uint8_t
p2pFuncGetForceTrxConfig(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_TRX_LIMITED_CONFIG == 1)
	return prAdapter->rWifiVar.ucForceTrxConfig;
#else
	return 0;
#endif
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
void p2pFuncSetDfsChannelAvailable(struct ADAPTER *prAdapter,
	uint8_t ucAvailable, uint8_t ucChannel,
	enum ENUM_MAX_BANDWIDTH_SETTING eBw)
{
	enum ENUM_CHNL_EXT eBssSCO;
	uint32_t u4CenterFreq;

	DBGLOG(P2P, INFO,
		"%s dfs channel, channel=%d, bw=%d\n",
		ucAvailable == 1 ? "Set" : "Unset",
		ucChannel,
		eBw);

#if CFG_SUPPORT_SAP_DFS_CHANNEL
	if (ucAvailable == 1) {
		eBssSCO = nicGetSco(prAdapter, BAND_5G, ucChannel);
		u4CenterFreq = nicGetS1Freq(prAdapter, BAND_5G, ucChannel,
			eBw);

		wlanDfsChannelsReqAdd(prAdapter,
			DFS_CHANNEL_CTRL_SOURCE_DBG,
			ucChannel,
			eBw, /* bandwidth */
			eBssSCO, /* sco */
			u4CenterFreq, /* center frequency */
			BAND_5G /* eBand */);
	} else {
		wlanDfsChannelsReqDel(prAdapter,
			DFS_CHANNEL_CTRL_SOURCE_DBG);
	}
#endif
}

void p2pFuncStartRdd(struct ADAPTER *prAdapter, uint8_t ucBssIdx)
{
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	uint8_t ucReqChnlNum;


	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
			prAdapter->aprBssInfo[ucBssIdx]->u4PrivateData);

	ucReqChnlNum = prP2pRoleFsmInfo->rChnlReqInfo[0].ucReqChnlNum;

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
	prCmdRddOnOffCtrl->ucRddIdx = rlmDomainGetDfsDbdcBand();
	prCmdRddOnOffCtrl->ucBssIdx = ucBssIdx;

#if (CFG_SUPPORT_SINGLE_SKU == 1)
	prCmdRddOnOffCtrl->ucSetVal = kalGetRdmVal(rlmDomainGetDfsRegion());
	if (rlmDomainIsSameCountryCode("KR", 2))
		prCmdRddOnOffCtrl->ucSetVal = ENUM_RDM_KR;
#endif

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.u4CC2Region))
		prCmdRddOnOffCtrl->ucSetVal =
			regCountryDfsMapping(prAdapter);

	if (prCmdRddOnOffCtrl->ucRddIdx)
		prCmdRddOnOffCtrl->ucRddRxSel = RDD_IN_SEL_1;
	else
		prCmdRddOnOffCtrl->ucRddRxSel = RDD_IN_SEL_0;

	DBGLOG(P2P, INFO,
		"Start Radar detection at %d - DFS ctrl: %d, RDD index: %d\n",
		prCmdRddOnOffCtrl->ucSetVal,
		prCmdRddOnOffCtrl->ucDfsCtrl,
		prCmdRddOnOffCtrl->ucRddIdx);

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

void p2pFuncStopRdd(struct ADAPTER *prAdapter, uint8_t ucBssIdx)
{
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;

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
	prCmdRddOnOffCtrl->ucRddIdx = rlmDomainGetDfsDbdcBand();
	prCmdRddOnOffCtrl->ucBssIdx = ucBssIdx;

	if (prCmdRddOnOffCtrl->ucRddIdx)
		prCmdRddOnOffCtrl->ucRddRxSel = RDD_IN_SEL_1;
	else
		prCmdRddOnOffCtrl->ucRddRxSel = RDD_IN_SEL_0;

	DBGLOG(P2P, INFO,
		"Stop Radar detection - DFS ctrl: %d, RDD index: %d\n",
		prCmdRddOnOffCtrl->ucDfsCtrl,
		prCmdRddOnOffCtrl->ucRddIdx);

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

void p2pFuncCsaUpdateGcStaRec(struct BSS_INFO *prBssInfo)
{
	struct STA_RECORD *prStaRec = prBssInfo->prStaRecOfAP;

	/* Update VHT op info of target AP */
	prStaRec->ucVhtOpChannelWidth =
		prBssInfo->ucVhtChannelWidth;
	prStaRec->ucVhtOpChannelFrequencyS1 =
		prBssInfo->ucVhtChannelFrequencyS1;
	prStaRec->ucVhtOpChannelFrequencyS2 =
		prBssInfo->ucVhtChannelFrequencyS2;

	/* Update HT, VHT PhyType of StaRec when GC switch channel granted */
	if (prBssInfo->ucPhyTypeSet & PHY_TYPE_SET_802_11N) {
		prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_HT;
		prStaRec->ucDesiredPhyTypeSet |= PHY_TYPE_BIT_HT;
	} else {
		prStaRec->ucPhyTypeSet &= ~(PHY_TYPE_BIT_HT);
		prStaRec->ucDesiredPhyTypeSet &= ~(PHY_TYPE_BIT_HT);
	}

	if (prBssInfo->ucPhyTypeSet & PHY_TYPE_SET_802_11AC) {
		prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;
		prStaRec->ucDesiredPhyTypeSet |= PHY_TYPE_BIT_VHT;
	} else {
		prStaRec->ucPhyTypeSet &= ~(PHY_TYPE_BIT_VHT);
		prStaRec->ucDesiredPhyTypeSet &= ~(PHY_TYPE_BIT_VHT);
	}
}

void p2pFuncDfsSwitchCh(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo)
{
	struct GLUE_INFO *prGlueInfo;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;
	struct WIFI_VAR *prWifiVar;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo;
#endif
	struct P2P_FILS_DISCOVERY_INFO *prFilsInfo;
	struct P2P_UNSOL_PROBE_RESP_INFO *prUnsolProbeInfo;
	uint8_t ucRoleIdx;
	u_int8_t fgIsCrossBand = FALSE;
	u_int8_t fgIsPureAp;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucUnsolProbeResp = prAdapter->rWifiVar.ucUnsolProbeResp;
#endif
	if (!prBssInfo) {
		DBGLOG(P2P, ERROR, "prBssInfo shouldn't be NULL!\n");
		return;
	}

	ucRoleIdx = prBssInfo->u4PrivateData;
	prWifiVar = &prAdapter->rWifiVar;
	prP2pSpecBssInfo = prWifiVar->prP2pSpecificBssInfo[ucRoleIdx];
	prFilsInfo = &prP2pSpecBssInfo->rFilsInfo;
	prUnsolProbeInfo = &prP2pSpecBssInfo->rUnsolProbeInfo;

	if (prBssInfo->eBand != prP2pChnlReqInfo->eBand)
		fgIsCrossBand = TRUE;

	/*  Setup Channel, Band */
	prBssInfo->ucPrimaryChannel = prP2pChnlReqInfo->ucReqChnlNum;
	prBssInfo->eBand = prP2pChnlReqInfo->eBand;
	prBssInfo->eBssSCO = prP2pChnlReqInfo->eChnlSco;

/* To Support Cross Band Channel Swtich */
#if CFG_SUPPORT_IDC_CH_SWITCH
	if (prBssInfo->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
		|| prBssInfo->eBand == BAND_6G
#endif
	) {
		/* Depend on eBand */
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11A;
		prBssInfo->ucPhyTypeSet &= ~(PHY_TYPE_SET_802_11BG);
		/* Depend on eCurrentOPMode and ucPhyTypeSet */
		prBssInfo->ucConfigAdHocAPMode = AP_MODE_11A;
	} else { /* Only SAP mode should enter this function */
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11BG;
		prBssInfo->ucPhyTypeSet &= ~(PHY_TYPE_SET_802_11A);
		/* Depend on eCurrentOPMode and ucPhyTypeSet */
		prBssInfo->ucConfigAdHocAPMode = AP_MODE_MIXED_11BG;
	}

	fgIsPureAp = p2pFuncIsAPMode(
			prAdapter->rWifiVar.prP2PConnSettings
			[prBssInfo->u4PrivateData]);

	/* Overwrite BSS PHY type set by Feature Options */
	bssDetermineApBssInfoPhyTypeSet(prAdapter,
		fgIsPureAp, prBssInfo);

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
	kalMemZero(prBssInfo->aucAllSupportedRates, RATE_NUM_SW);
	rateGetDataRatesFromRateSet(
		prBssInfo->u2OperationalRateSet,
		prBssInfo->u2BSSBasicRateSet,
		prBssInfo->aucAllSupportedRates,
		&prBssInfo->ucAllSupportedRatesLen);

	bssInitForAP(prAdapter, prBssInfo, TRUE);
	if (prBssInfo->fgEnableH2E) {
		prBssInfo->aucAllSupportedRates
			[prBssInfo->ucAllSupportedRatesLen]
			= RATE_H2E_ONLY_VAL;
		prBssInfo->ucAllSupportedRatesLen++;
	}

	nicQmUpdateWmmParms(prAdapter, prBssInfo->ucBssIndex);
#endif

	/* Setup channel and bandwidth */
	rlmBssInitForAPandIbss(prAdapter, prBssInfo);

	/* Update Beacon to FW. Note that we have to set Op mode Rx
	 * flag to TRUE in order to update VHT OP Notification IE.
	 * Otherwise, clients will not be able to process VHT OP
	 * Notification IE and assume wrong Rx NSS.
	 */
	prBssInfo->fgIsOpChangeRxNss = TRUE;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	if (prMldBssInfo) {
		struct LINK *prBssList;
		struct BSS_INFO *prTempBss;

		/* loop all links to re-generate beacons after csa */
		prBssList = &prMldBssInfo->rBssList;
		LINK_FOR_EACH_ENTRY(prTempBss, prBssList, rLinkEntryMld,
				    struct BSS_INFO) {
			bssUpdateBeaconContent(prAdapter,
					       prTempBss->ucBssIndex);
		}
	} else
#endif
	{
		bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);
	}

	if (prFilsInfo->fgValid) {
		nicUpdateFilsDiscIETemplate(prAdapter,
					    prBssInfo->ucBssIndex,
					    prFilsInfo->u4MaxInterval,
					    prFilsInfo->u4MinInterval,
					    prFilsInfo->aucIEBuf,
					    prFilsInfo->u4Length);
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prBssInfo->eBand == BAND_6G &&
		 (IS_FEATURE_FORCE_ENABLED(ucUnsolProbeResp) ||
		  prUnsolProbeInfo->fgValid)) {
		/* Update unsolicited probe response as beacon */
		bssUpdateBeaconContentEx(prAdapter,
					 prBssInfo->ucBssIndex,
					 IE_UPD_METHOD_UNSOL_PROBE_RSP);
	}
#endif

	prBssInfo->fgIsOpChangeRxNss = FALSE;

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
	prCmdRddOnOffCtrl->ucBssIdx = prBssInfo->ucBssIndex;

	DBGLOG(P2P, INFO,
		"p2pFuncDfsSwitchCh: Start TXQ - DFS ctrl: %d, RDD index: %d\n",
		prCmdRddOnOffCtrl->ucDfsCtrl,
		prCmdRddOnOffCtrl->ucRddIdx);

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

#if CFG_SUPPORT_SAP_DFS_CHANNEL
	if (fgIsPureAp)
		wlanDfsChannelsReqAdd(prAdapter,
			DFS_CHANNEL_CTRL_SOURCE_SAP,
			prBssInfo->ucPrimaryChannel,
			prBssInfo->ucVhtChannelWidth,
			prBssInfo->eBssSCO,
			nicChannelNum2Freq(
				prBssInfo->ucVhtChannelFrequencyS1,
				prBssInfo->eBand) / 1000,
			prBssInfo->eBand);
#endif

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	if (prBssInfo->eBand == BAND_6G) {
		rlmDomain6GPwrModeUpdate(prAdapter,
			prBssInfo->ucBssIndex,
			PWR_MODE_6G_VLP);
	} else if (prBssInfo->eBand == BAND_2G4 ||
		prBssInfo->eBand == BAND_5G) {
		rlmDomain6GPwrModeUpdate(prAdapter,
			prBssInfo->ucBssIndex,
			PWR_MODE_6G_LPI);
	}
#endif

	kalP2pIndicateChnlSwitch(prAdapter, prBssInfo);

	/* Down the flag */
	prAdapter->rWifiVar.ucChannelSwitchMode = 0;
#if CFG_SUPPORT_P2P_ECSA
	prP2pSpecBssInfo->fgEcsa = FALSE;
	prP2pSpecBssInfo->ucEcsaBw = 0;
#endif
#if CFG_SUPPORT_DBDC
	/* Check DBDC status */
	cnmDbdcRuntimeCheckDecision(prAdapter,
			prBssInfo->ucBssIndex,
			FALSE);

#endif
#if CFG_SUPPORT_IDC_CH_SWITCH
	cnmIdcSwitchSapChannel(prAdapter);
#endif
	if (prP2pSpecBssInfo->fgIsRddOpchng == TRUE) {
		cnmOpmodeEventHandler(prAdapter,
			prP2pSpecBssInfo->prRddPostOpchng);
		prP2pSpecBssInfo->fgIsRddOpchng = FALSE;
		kalMemFree(prP2pSpecBssInfo->prRddPostOpchng,
			VIR_MEM_TYPE,
			sizeof(struct WIFI_EVENT) +
			sizeof(struct EVENT_OPMODE_CHANGE));
	}
} /* p2pFuncDfsSwitchCh */

u_int8_t p2pFuncCheckWeatherRadarBand(
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
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
	if (kalIsETSIDfsRegin()) {
		if (eChannelWidth >= VHT_OP_CHANNEL_WIDTH_80) {
			if (ucCenterFreqS1 >= 114 && ucCenterFreqS1 <= 128)
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

int32_t p2pFuncSetDriverCacTime(uint32_t u4CacTime)
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

void p2pFuncDisableManualCac(void)
{
	g_fgManualCac = FALSE;
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

void p2pFuncGetRadarInfo(struct P2P_RADAR_INFO *prP2pRadarInfo)
{
	kalMemCopy(prP2pRadarInfo, &g_rP2pRadarInfo, sizeof(*prP2pRadarInfo));
}

void p2pFuncSetRadarDetectMode(uint8_t ucRadarDetectMode)
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

void p2pFuncAddRadarDetectCnt(void)
{
	g_ucRadarDetectCnt++;
}

void p2pFuncRadarDetectCntUevent(struct ADAPTER *prAdapter)
{
	char uEvent[300];

	kalSnprintf(uEvent, sizeof(uEvent),
		"radardetectcount=%u",
		p2pFuncGetRadarDetectCnt());

	DBGLOG(SCN, LOUD, "radar detect uevent:%s\n", uEvent);
	kalSendUevent(prAdapter, uEvent);
}

void p2pFuncRadarDetectDoneUevent(struct ADAPTER *prAdapter)
{
	char uEvent[300];

	kalSnprintf(uEvent, sizeof(uEvent),
		"radardetectdone=1");

	DBGLOG(SCN, LOUD, "radar detect done\n", uEvent);
	kalSendUevent(prAdapter, uEvent);
}

void p2pFuncResetRadarDetectCnt(void)
{
	g_ucRadarDetectCnt = 0;
}

uint8_t p2pFuncGetRadarDetectCnt(void)
{
	return g_ucRadarDetectCnt;
}

void p2pFuncSetDfsState(uint8_t ucDfsState)
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

const char *p2pFuncShowDfsState(void)
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

void p2pFuncChannelListFiltering(struct ADAPTER *prAdapter,
		uint16_t ucFilteredCh, uint8_t ucFilteredBw,
		uint8_t pucNumOfChannel,
		struct RF_CHANNEL_INFO *paucChannelList,
		uint8_t *pucOutNumOfChannel,
		struct RF_CHANNEL_INFO *paucOutChannelList)
{
	uint8_t i;
	uint8_t j;
	uint8_t rddS1;

	if (ucFilteredBw == VHT_OP_CHANNEL_WIDTH_20_40) {
		for (i = 0; i < pucNumOfChannel; i++)
			paucOutChannelList[i] = paucChannelList[i];
		return;
	}

	rddS1 = nicGetS1(prAdapter, BAND_5G, ucFilteredCh, ucFilteredBw);
	if (rddS1 == 0)
		return;

	j = 0;
	for (i = 0; i < pucNumOfChannel; i++) {
		if (nicGetS1(prAdapter, BAND_5G,
			paucChannelList[i].ucChannelNum,
			ucFilteredBw) != rddS1) {
			paucOutChannelList[j] = paucChannelList[i];
			DBGLOG(RLM, TRACE,
				"ch: %d, s1: %d, is_dfs: %d, rdds1: %d\n",
				paucOutChannelList[j].ucChannelNum,
				nicGetS1(prAdapter, BAND_5G,
				paucOutChannelList[j].ucChannelNum,
				ucFilteredBw),
				paucOutChannelList[j].fgDFS,
				rddS1);
			j++;
		}
	}
	*pucOutNumOfChannel = j;
}

#endif
uint8_t p2pFuncGetCsaBssIndex(void)
{
	return g_ucBssIdx;
}

void p2pFuncSetCsaBssIndex(uint8_t ucBssIdx)
{
	DBGLOG(P2P, TRACE,
		"ucBssIdx = %d\n", ucBssIdx);

	g_ucBssIdx = ucBssIdx;
}

void p2pFuncParseH2E(struct BSS_INFO *prP2pBssInfo)
{
	if (prP2pBssInfo) {
		uint32_t i;

		prP2pBssInfo->fgEnableH2E = FALSE;

		for (i = 0;
			i < RATE_NUM_SW &&
			i < prP2pBssInfo->ucAllSupportedRatesLen;
			i++) {
			DBGLOG(P2P, LOUD,
				"Rate [%d] = %d\n",
				i,
				prP2pBssInfo->aucAllSupportedRates[i]);
			if (prP2pBssInfo->aucAllSupportedRates[i] ==
				RATE_H2E_ONLY_VAL) {
				prP2pBssInfo->fgEnableH2E = TRUE;
				break;
			}
		}

		DBGLOG(P2P, TRACE,
			"fgEnableH2E = %d\n",
			prP2pBssInfo->fgEnableH2E);
	}
}

#if 0
uint32_t
p2pFuncBeaconUpdate(struct ADAPTER *prAdapter,
		uint8_t *pucBcnHdr,
		uint32_t u4HdrLen,
		uint8_t *pucBcnBody,
		uint32_t u4BodyLen,
		uint32_t u4DtimPeriod,
		uint32_t u4BcnInterval)
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
p2pFuncBeaconUpdate(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		struct P2P_BEACON_UPDATE_INFO *prBcnUpdateInfo,
		uint8_t *pucNewBcnHdr,
		uint32_t u4NewHdrLen,
		uint8_t *pucNewBcnBody,
		uint32_t u4NewBodyLen)
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
			ASSERT((uintptr_t) prBcnUpdateInfo->pucBcnHdr ==
				((uintptr_t) prBcnMsduInfo->prPacket +
				MAC_TX_RESERVED_FIELD));
		}

		if (prBcnUpdateInfo->pucBcnBody != NULL) {
			ASSERT((uintptr_t) prBcnUpdateInfo->pucBcnBody ==
				((uintptr_t) prBcnUpdateInfo->pucBcnHdr +
				(uint32_t) prBcnUpdateInfo->u4BcnHdrLen));
		}
#endif
		prBcnFrame = (struct WLAN_BEACON_FRAME *)
			((uintptr_t) prBcnMsduInfo->prPacket +
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
			((uintptr_t) prBcnUpdateInfo->pucBcnHdr +
			(uintptr_t) prBcnUpdateInfo->u4BcnHdrLen);
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
		prP2pBssInfo->u2CapInfo = prBcnFrame->u2CapInfo;

		p2pFuncParseBeaconContent(prAdapter,
			prP2pBssInfo,
			prBcnFrame->aucBSSID,
			(uint8_t *) prBcnFrame->aucInfoElem,
			(prBcnMsduInfo->u2FrameLength -
			OFFSET_OF(struct WLAN_BEACON_FRAME, aucInfoElem)));

		p2pFuncParseH2E(prP2pBssInfo);

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
p2pFuncAssocRespUpdate(struct ADAPTER *prAdapter,
		    struct BSS_INFO *prP2pBssInfo,
		    uint8_t *AssocRespIE, uint32_t u4AssocRespLen)
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
p2pFuncDeauth(struct ADAPTER *prAdapter,
		uint8_t *pucPeerMacAddr,
		uint16_t u2ReasonCode,
		uint8_t *pucIEBuf,
		uint16_t u2IELen,
		u_int8_t fgSendDeauth)
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
p2pFuncDisassoc(struct ADAPTER *prAdapter,
		uint8_t *pucPeerMacAddr,
		uint16_t u2ReasonCode,
		uint8_t *pucIEBuf,
		uint16_t u2IELen,
		u_int8_t fgSendDisassoc)
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
p2pFuncProbeRespUpdate(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t *ProbeRespIE, uint32_t u4ProbeRespLen,
		enum ENUM_IE_UPD_METHOD eMethod)

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

	prMsduInfo->u2FrameLength = 0;

	bssBuildBeaconProbeRespFrameCommonIEs(prMsduInfo,
		prP2pBssInfo, ProbeRespIE);

	u4IeArraySize = ARRAY_SIZE(txProbeRspIETable);

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
		"update probe response for bss index:%d, method:%d, IE len:%d\n",
		prP2pBssInfo->ucBssIndex,
		eMethod,
		prMsduInfo->u2FrameLength);
	/* dumpMemory8(prMsduInfo->prPacket, prMsduInfo->u2FrameLength); */

	DBGLOG(P2P, TRACE, "Dump probe response to FW\n");
	if (aucDebugModule[DBG_P2P_IDX] & DBG_CLASS_TRACE) {
		dumpMemory8((uint8_t *) prMsduInfo->prPacket,
					(uint32_t) prMsduInfo->u2FrameLength);
	}

	return nicUpdateBeaconIETemplate(prAdapter,
					eMethod,
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
p2pFuncDissolve(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		u_int8_t fgSendDeauth,
		uint16_t u2ReasonCode,
		u_int8_t fgIsLocallyGenerated)
{
	struct STA_RECORD *prCurrStaRec, *prStaRecNext;
	struct LINK *prClientList;

	do {

		ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL));

		switch (prP2pBssInfo->eCurrentOPMode) {
		case OP_MODE_INFRASTRUCTURE:
			/* Reset station record status. */
			if (prP2pBssInfo->prStaRecOfAP) {

				kalP2PGCIndicateConnectionStatus(
					prAdapter->prGlueInfo,
					(uint8_t) prP2pBssInfo->u4PrivateData,
					NULL, NULL, 0, u2ReasonCode,
					fgIsLocallyGenerated ?
					WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY :
					WLAN_STATUS_MEDIA_DISCONNECT);

				/* 2012/02/14 frog:
				 * After formation before join group,
				 * prStaRecOfAP is NULL.
				 */
				p2pFuncDisconnect(prAdapter,
					prP2pBssInfo,
					prP2pBssInfo->prStaRecOfAP,
					fgSendDeauth,
					u2ReasonCode,
					fgIsLocallyGenerated);
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
			prClientList = &prP2pBssInfo->rStaRecOfClientList;

			/* This case may let LINK_FOR_EACH_ENTRY_SAFE crash */
			if (prClientList == NULL ||
			    prClientList->u4NumElem == 0)
				break;
			if (fgSendDeauth) {
				/* Send bmc deauth. */
				authSendDeauthFrame(prAdapter, prP2pBssInfo,
					NULL, NULL, u2ReasonCode, NULL);
			}
			LINK_FOR_EACH_ENTRY_SAFE(prCurrStaRec, prStaRecNext,
				prClientList, rLinkEntry, struct STA_RECORD) {
				if (!prCurrStaRec)
					break;
				p2pFuncDisconnect(prAdapter,
					prP2pBssInfo, prCurrStaRec,
					fgSendDeauth, u2ReasonCode,
					fgIsLocallyGenerated);
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
p2pFuncDisconnect(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		struct STA_RECORD *prStaRec,
		u_int8_t fgSendDeauth, uint16_t u2ReasonCode,
		u_int8_t fgIsLocallyGenerated)
{
#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	struct _TWT_HOTSPOT_STA_NODE *prTWTHotspotStaNode = NULL;
#endif
	struct TIMER *prTimer;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (prStaRec != NULL) && (prP2pBssInfo != NULL));

		ASSERT_BREAK(prP2pBssInfo->eNetworkType == NETWORK_TYPE_P2P);

		ASSERT_BREAK(prP2pBssInfo->ucBssIndex
			< prAdapter->ucP2PDevBssIdx);
		prTimer = &(prStaRec->rDeauthTxDoneTimer);
		if (u2ReasonCode == REASON_CODE_DISASSOC_INACTIVITY ||
			u2ReasonCode == REASON_CODE_DISASSOC_LEAVING_BSS) {
			prAdapter->u4HifChkFlag |= HIF_TRIGGER_FW_DUMP;
			prAdapter->u4HifDbgMod = DBG_PLE_INT_MOD_TX;
			prAdapter->u4HifDbgBss = prP2pBssInfo->ucBssIndex;
			prAdapter->u4HifDbgReason = DBG_PLE_INT_REASON_MANUAL;
			kalSetHifDbgEvent(prAdapter->prGlueInfo);
		}

		/* Indicate disconnect. */
		if (prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
			struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
			    P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
					prP2pBssInfo->u4PrivateData);

#if CFG_TC10_FEATURE
			/* Store last sta deauth reason */
			prP2pBssInfo->u2DeauthReason = u2ReasonCode;
#endif
			if (!timerPendingTimer(prTimer))
				kalP2PGOStationUpdate(
					prAdapter->prGlueInfo,
					prP2pRoleFsmInfo->ucRoleIndex,
					prStaRec, FALSE);

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

#if (CFG_SUPPORT_DFS_MASTER == 1) && CFG_SUPPORT_SAP_DFS_CHANNEL
			if (!aisGetConnectedBssInfo(prAdapter)) {
				/* restore DFS channels table */
				wlanDfsChannelsReqDel(prAdapter,
					DFS_CHANNEL_CTRL_SOURCE_SAP);
			}
#endif
		} else {
			struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
			    P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
					prP2pBssInfo->u4PrivateData);

			prP2pRoleFsmInfo->rJoinInfo.prTargetBssDesc = NULL;
			prP2pRoleFsmInfo->rJoinInfo.prTargetStaRec = NULL;

			p2pClearAllLink(prP2pRoleFsmInfo);

			p2pRemoveAllBssDesc(prAdapter,
				prP2pBssInfo);
		}

		DBGLOG(P2P, INFO,
			"p2pFuncDisconnect(): BssMode: %d, reason: %d, SendDeauth %s, locally_generated: %d\n",
			prP2pBssInfo->eCurrentOPMode, u2ReasonCode,
			fgSendDeauth == TRUE ? "TRUE" : "FALSE",
			fgIsLocallyGenerated);

		if (fgSendDeauth) {
			prStaRec->u2ReasonCode = u2ReasonCode;
			prStaRec->fgIsLocallyGenerated = fgIsLocallyGenerated;
			p2pFunAbortOngoingScan(prAdapter);
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
			p2pLinkStaRecFree(prAdapter, prStaRec, prP2pBssInfo);
		}
	} while (FALSE);

	return;

}				/* p2pFuncDisconnect */

void p2pFuncSetChannel(struct ADAPTER *prAdapter,
		uint8_t ucRoleIdx,
		struct RF_CHANNEL_INFO *prRfChannelInfo)
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
				&u4BufLen);

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
u_int8_t p2pFuncRetryJOIN(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		struct P2P_JOIN_INFO *prJoinInfo)
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

		if (prStaRec->ucAuthAlgNum == AUTH_ALGORITHM_NUM_SAE) {
			DBGLOG(P2P, TRACE, "Do not retry join for SAE.");
			break;
		}

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

struct BSS_INFO *p2pFuncBSSIDFindBssInfo(struct ADAPTER *prAdapter,
		uint8_t *pucBSSID)
{
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	uint8_t ucBssIdx = 0;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (pucBSSID != NULL));

		for (ucBssIdx = 0;
			ucBssIdx < prAdapter->ucSwBssIdNum; ucBssIdx++) {
			if (!IS_NET_ACTIVE(prAdapter, ucBssIdx))
				continue;

			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
			if (!prBssInfo)
				break;
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
p2pFuncValidateAuth(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		struct SW_RFB *prSwRfb,
		struct STA_RECORD **pprStaRec,
		uint16_t *pu2StatusCode)
{
	u_int8_t fgPmfConn = FALSE;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;
	struct WLAN_AUTH_FRAME *prAuthFrame = (struct WLAN_AUTH_FRAME *) NULL;

	DBGLOG(P2P, TRACE, "p2pValidate Authentication Frame\n");


	/* P2P 3.2.8 */
	*pu2StatusCode = STATUS_CODE_REQ_DECLINED;

	if (!prSwRfb || !prSwRfb->pvHeader) {
		DBGLOG(P2P, ERROR,
			"prSwRfb or prSwRfb->pvHeader is NULL!\n");
		return FALSE;
	}
	prAuthFrame = (struct WLAN_AUTH_FRAME *) prSwRfb->pvHeader;

	if ((prP2pBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) ||
	    (prP2pBssInfo->eIntendOPMode != OP_MODE_NUM)) {
		/* We are not under AP Mode yet. */
		DBGLOG(P2P, WARN,
			"Current OP mode is not under AP mode. (%d)\n",
			prP2pBssInfo->eCurrentOPMode);
		return FALSE;
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
		*pu2StatusCode = STATUS_CODE_ASSOC_DENIED_AP_OVERLOAD;
		return FALSE;
	}
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
	/* Hotspot Blocklist */
	if (kalP2PCmpBlockList(prAdapter->prGlueInfo,
		prAuthFrame->aucSrcAddr,
		(uint8_t) prP2pBssInfo->u4PrivateData)
		|| !p2pRoleProcessACLInspection(prAdapter,
		prAuthFrame->aucSrcAddr, prP2pBssInfo->ucBssIndex)) {
		DBGLOG(P2P, WARN, "in block list.\n");
		*pu2StatusCode
			= STATUS_CODE_ASSOC_DENIED_OUTSIDE_STANDARD;
		return FALSE;
	}
#endif
	prStaRec = cnmGetStaRecByAddress(prAdapter,
		prP2pBssInfo->ucBssIndex, prAuthFrame->aucSrcAddr);

	if (prStaRec) {
#if CFG_SUPPORT_802_11W
		/* AP PMF. if PMF connection, do not reset state & FSM */
		fgPmfConn = rsnCheckBipKeyInstalled(prAdapter, prStaRec);
		if (prAdapter->rWifiVar.fgSapAuthPolicy ==
			P2P_AUTH_POLICY_RESET)
			DBGLOG(P2P, INFO, "Fall through PMF check\n");
		else if (fgPmfConn &&
			(prP2pBssInfo->u4RsnSelectedAKMSuite ==
			RSN_AKM_SUITE_OWE))
			DBGLOG(P2P, INFO, "[OWE] Fall through PMF check\n");
		else if (fgPmfConn &&
			(!rsnKeyMgmtSae(prP2pBssInfo->u4RsnSelectedAKMSuite) ||
			(prAdapter->rWifiVar.fgSapAuthPolicy ==
			P2P_AUTH_POLICY_IGNORE))) {
			DBGLOG(P2P, WARN, "PMF Connction, return false\n");
			return FALSE;
		}
#endif

		if (prStaRec->ucStaState > STA_STATE_1 &&
		    IS_STA_IN_P2P(prAdapter, prStaRec)) {
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			struct MLD_STA_RECORD *prMldSta;
			struct STA_RECORD *sta, *temp;
			struct BSS_INFO *bss;

			prMldSta = mldStarecGetByStarec(prAdapter, prStaRec);
			if (prMldSta) {
				struct LINK *prStarecList =
					&prMldSta->rStarecList;

				LINK_FOR_EACH_ENTRY_SAFE(sta, temp,
							 prStarecList,
							 rLinkEntryMld,
							 struct STA_RECORD) {
					bss = GET_BSS_INFO_BY_INDEX(prAdapter,
						sta->ucBssIndex);
					if (!bss)
						continue;

					bssRemoveClient(prAdapter, bss, sta);
					p2pFuncDisconnect(prAdapter, bss, sta,
						FALSE,
						REASON_CODE_DISASSOC_INACTIVITY,
						TRUE);
				}
			} else
#endif
			{
				bssRemoveClient(prAdapter, prP2pBssInfo,
						prStaRec);
				p2pFuncDisconnect(prAdapter, prP2pBssInfo,
					prStaRec, FALSE,
					REASON_CODE_DISASSOC_INACTIVITY,
					TRUE);
			}
			prStaRec = NULL;
		}
	}

	if (!prStaRec) {
		prStaRec = cnmStaRecAlloc(prAdapter, STA_TYPE_P2P_GC,
			prP2pBssInfo->ucBssIndex,
			prAuthFrame->aucSrcAddr);

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
	}

	/* prStaRec->eStaType = STA_TYPE_INFRA_CLIENT; */
	prStaRec->eStaType = STA_TYPE_P2P_GC;

	/* Update Station Record - Status/Reason Code */
	prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;

	prStaRec->ucJoinFailureCount = 0;

	prStaRec->ucAuthAlgNum = prAuthFrame->u2AuthAlgNum;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (p2pLinkProcessRxAuthReqFrame(prAdapter,
		prP2pBssInfo, prStaRec, prSwRfb) != WLAN_STATUS_SUCCESS) {
		cnmStaRecFree(prAdapter, prStaRec);
		*pu2StatusCode = STATUS_CODE_DENIED_EHT_NOT_SUPPORTED;
		return FALSE;
	}
#endif

	*pprStaRec = prStaRec;

	*pu2StatusCode = STATUS_CODE_SUCCESSFUL;


	return TRUE;

}				/* p2pFuncValidateAuth */

void p2pFuncResetStaRecStatus(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec)
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
p2pFuncInitConnectionSettings(struct ADAPTER *prAdapter,
		struct P2P_CONNECTION_SETTINGS *prP2PConnSettings,
		u_int8_t fgIsApMode)
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
u_int8_t p2pFuncValidateAssocReq(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		uint16_t *pu2StatusCode)
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
	 *  3. Check Block List here.
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
u_int8_t p2pFuncParseCheckForTKIPInfoElem(uint8_t *pucBuf)
{
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA;
	struct WPA_INFO_ELEM *prWpaIE = (struct WPA_INFO_ELEM *) NULL;
	uint32_t u4GroupKeyCipher = 0;
	int32_t i4RemainWpaIeLen;

	if (pucBuf == NULL)
		return FALSE;

	prWpaIE = (struct WPA_INFO_ELEM *) pucBuf;

	if (prWpaIE->ucLength <= ELEM_MIN_LEN_WFA_OUI_TYPE_SUBTYPE)
		return FALSE;

	if (kalMemCmp(prWpaIE->aucOui, aucWfaOui, sizeof(aucWfaOui)))
		return FALSE;

	i4RemainWpaIeLen = (int32_t) prWpaIE->ucLength -
				ELEM_MIN_LEN_WFA_OUI_TYPE_SUBTYPE;

	if (i4RemainWpaIeLen < 4)
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
 * @brief This function is used to check the MTK Oui IE from packets
 *        transmitted by the peer STA.
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void p2pFuncParseMTKOuiInfoElem(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		uint8_t *pucIE)
{
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
	uint8_t *aucCapa;
#if CFG_SUPPORT_BALANCE_MLR
	uint8_t *ie;
	uint16_t ie_len, ie_offset;
#endif

	if (pucIE == NULL)
		return;

	aucCapa = MTK_OUI_IE(pucIE)->aucCapability;

	/* only check tlv */
	if (IE_LEN(pucIE) < ELEM_MIN_LEN_MTK_OUI ||
	    kalMemCmp(pucIE + 2, aucMtkOui, sizeof(aucMtkOui)) ||
	    !(aucCapa[0] & MTK_SYNERGY_CAP_SUPPORT_TLV))
		return;

	prStaRec->fgIsSupportCsa = 1;
	DBGLOG(P2P, TRACE, "Peer support CSA\n");

#if CFG_SUPPORT_BALANCE_MLR
	ie = MTK_OUI_IE(pucIE)->aucInfoElem;
	ie_len = IE_LEN(pucIE) - 7;

	IE_FOR_EACH(ie, ie_len, ie_offset) {
		if (IE_ID(ie) == MTK_OUI_ID_MLR) {
			struct IE_MTK_MLR *prMLR = (struct IE_MTK_MLR *)ie;
			/* LR bitmap:
			 * BIT[0]-MLR_V1,
			 * BIT[1]->MLR_V2,
			 * BIT[2]MLR+,
			 * BIT[3]->ALR,
			 * BIT[4]->DUAL_CTS
			 */
			prStaRec->ucMlrSupportBitmap = prMLR->ucLRBitMap;

			MLR_DBGLOG(prAdapter, P2P, INFO,
				"MLR assoc req - Type|Len|B[0x%02x]\n",
				prStaRec->ucMlrSupportBitmap);
		}
	}
#endif /* CFG_SUPPORT_BALANCE_MLR */
}				/* p2pFuncParseMTKOuiInfoElem */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function is used to check the P2P IE
 *
 *
 * @return none
 */
/*---------------------------------------------------------------------------*/
u_int8_t p2pFuncParseCheckForP2PInfoElem(struct ADAPTER *prAdapter,
		uint8_t *pucBuf, uint8_t *pucOuiType)
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
p2pFuncValidateProbeReq(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		uint32_t *pu4ControlFlags,
		u_int8_t fgIsDevInterface,
		uint8_t ucRoleIdx)
{
	u_int8_t fgIsReplyProbeRsp = FALSE;
	u_int8_t fgApplyp2PDevFilter = FALSE;
	void *prRoleHandler = NULL;
	void *prDevHandler = NULL;

	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;

	do {

		ASSERT_BREAK((prAdapter != NULL) && (prSwRfb != NULL));

		prP2pRoleFsmInfo =
			prAdapter->rWifiVar.aprP2pRoleFsmInfo[ucRoleIdx];

		/* Process both cases that with amd without add p2p interface */
		if (fgIsDevInterface)
			fgApplyp2PDevFilter = TRUE;
		else {
			prRoleHandler = kalGetP2pNetHdl(
							prAdapter->prGlueInfo,
							ucRoleIdx, TRUE);
			prDevHandler = kalGetP2pNetHdl(
							prAdapter->prGlueInfo,
							0, FALSE);
			if (prDevHandler == prRoleHandler)
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
				prSwRfb, fgIsDevInterface, ucRoleIdx,
				MLD_LINK_ID_NONE);
		}

	} while (FALSE);

	return fgIsReplyProbeRsp;

}				/* end of p2pFuncValidateProbeReq() */

/*---------------------------------------------------------------------------*/
/*!
 * @brief Abort AIS and P2P scan.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 *
 * @retval void
 */
/*---------------------------------------------------------------------------*/
static void
p2pFunAbortOngoingScan(struct ADAPTER *prAdapter)
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
	else if (prScanInfo->rScanParam.ucBssIndex ==
			prAdapter->ucP2PDevBssIdx)
		p2pDevFsmRunEventScanAbort(prAdapter,
			prAdapter->ucP2PDevBssIdx);
}

static void p2pFunBufferP2pActionFrame(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		uint8_t ucRoleIdx)
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
		prSwRfb->ucChnlNum, prSwRfb->eRfBand) / 1000;
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

uint32_t
p2pFuncValidateP2pDevRxActionFrame(struct ADAPTER *prAdapter,
	u_int8_t fgIsDevInterface, uint8_t ucSwRfbChannel, uint8_t ucCategory)
{
	uint8_t i;
	uint32_t u4Ret = WLAN_STATUS_SUCCESS;
	struct BSS_INFO *prP2pBssInfo = NULL;
	struct P2P_DEV_FSM_INFO *prP2pDevFsmInfo = NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo = NULL;
	enum ENUM_P2P_DEV_STATE eCurrentState;

	prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;

	/* not P2P Device's frame, keep it valid */
	if (!fgIsDevInterface || !prP2pDevFsmInfo)
		goto exit;

	/* P2P Device should receive provision discovery on role channel */
	for (i = 0; i < KAL_P2P_NUM; i++) {
		prP2pRoleFsmInfo =
			P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, i);
		if (!prP2pRoleFsmInfo)
			continue;

		prP2pBssInfo = prAdapter->aprBssInfo[
			prP2pRoleFsmInfo->ucBssIndex];
		if (!prP2pBssInfo ||
		    p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[i]))
			continue;

		if (prP2pBssInfo->ucPrimaryChannel == ucSwRfbChannel) {
			DBGLOG(P2P, INFO,
				"rx action frame %d on state:%d, Bss:%d, Chnl:%d\n",
				ucCategory,
				prP2pDevFsmInfo->eCurrentState,
				prP2pRoleFsmInfo->ucBssIndex,
				prP2pBssInfo->ucPrimaryChannel);
			goto exit;
		}
	}

	/* Ignore frames received from wrong state */
	eCurrentState = prP2pDevFsmInfo->eCurrentState;
	if (eCurrentState != P2P_DEV_STATE_OFF_CHNL_TX &&
	    eCurrentState != P2P_DEV_STATE_CHNL_ON_HAND &&
	    eCurrentState != P2P_DEV_STATE_REQING_CHANNEL) {
		u4Ret = WLAN_STATUS_FAILURE;
		goto exit;
	}

	if (eCurrentState == P2P_DEV_STATE_REQING_CHANNEL &&
	    prP2pDevFsmInfo->rChnlReqInfo.eChnlReqType != CH_REQ_TYPE_ROC) {
		u4Ret = WLAN_STATUS_FAILURE;
		goto exit;
	}

	if (prP2pDevFsmInfo->rChnlReqInfo.ucReqChnlNum != ucSwRfbChannel) {
		u4Ret = WLAN_STATUS_FAILURE;
		goto exit;
	}

exit:
	if (u4Ret == WLAN_STATUS_FAILURE)
		DBGLOG(P2P, INFO,
			"ignore rx action frame %d on state:%d, ReqChnl:%d, RxChnl:%d\n",
			ucCategory,
			prP2pDevFsmInfo->eCurrentState,
			prP2pDevFsmInfo->rChnlReqInfo.ucReqChnlNum,
			ucSwRfbChannel);
	return u4Ret;
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
void p2pFuncValidateRxActionFrame(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb, u_int8_t fgIsDevInterface,
					uint8_t ucRoleIdx)
{
	struct WLAN_ACTION_FRAME *prActFrame;
	struct WLAN_PUBLIC_VENDOR_ACTION_FRAME *prActPubVenFrame;
	u_int32_t u4Oui;
	u_int8_t ucOuiType;
	u_int8_t fgBufferFrame = FALSE;

	if (prAdapter == NULL || prSwRfb == NULL) {
		DBGLOG(P2P, ERROR, "Invalid parameter.\n");
		return;
	}
	prActFrame = (struct WLAN_ACTION_FRAME *) prSwRfb->pvHeader;

	/* Check if P2P Device can receive frames or not.
	 * If indicate RX frames in wrong P2P Device state, the next response
	 * frame may TX fail.
	 */
	if (p2pFuncValidateP2pDevRxActionFrame(prAdapter, fgIsDevInterface,
	    prSwRfb->ucChnlNum, prActFrame->ucCategory) != WLAN_STATUS_SUCCESS)
		return;

	switch (prActFrame->ucCategory) {
	case CATEGORY_PUBLIC_ACTION:
		if (prActFrame->ucAction != 0x9 ||
			prSwRfb->u2PacketLen <
				sizeof(struct WLAN_PUBLIC_VENDOR_ACTION_FRAME))
			break;

		WLAN_GET_FIELD_BE24(prActFrame->ucActionDetails, &u4Oui);
		ucOuiType = prActFrame->ucActionDetails[3];
		DBGLOG(P2P, TRACE, "Action: oui: 0x%x, type: 0x%x\n",
			u4Oui, ucOuiType);

		if (u4Oui != OUI_WFA)
			break;

		if (ucOuiType == P2P_OUI_TYPE) {
			prActPubVenFrame =
				(struct WLAN_PUBLIC_VENDOR_ACTION_FRAME *)
				prActFrame;
			p2pProcessActionResponse(prAdapter,
				prActPubVenFrame->ucPubSubType);
			if ((prActPubVenFrame->ucPubSubType ==
				P2P_GO_NEG_REQ) ||
				(prActPubVenFrame->ucPubSubType ==
				P2P_INVITATION_REQ) ||
				(prActPubVenFrame->ucPubSubType ==
				P2P_INVITATION_RESP) ||
				(prActPubVenFrame->ucPubSubType ==
				P2P_PROV_DISC_REQ)) {
				p2pFunAbortOngoingScan(prAdapter);
			}

			if (fgIsDevInterface) {
				p2pDevFsmNotifyP2pRx(prAdapter,
					prActPubVenFrame->ucPubSubType,
					&fgBufferFrame);
			}
		} else if (ucOuiType == DPP_OUI_TYPE) {
			if (!p2pFuncIsAPMode(
				prAdapter->rWifiVar.
					prP2PConnSettings[ucRoleIdx])) {
				/* P2P doesn't support DPP */
				return;
			}
		}
		kal_fallthrough;
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
			prSwRfb, fgIsDevInterface, ucRoleIdx,
			MLD_LINK_ID_NONE);
	}

	return;
}				/* p2pFuncValidateRxMgmtFrame */

u_int8_t p2pFuncIsDualGOMode(struct ADAPTER *prAdapter)
{
	if (prAdapter)
		return (kalP2PGetRole(prAdapter->prGlueInfo, 0) == 2 &&
			kalP2PGetRole(prAdapter->prGlueInfo, 1) == 2);

	return FALSE;
}

u_int8_t p2pFuncIsDualAPMode(struct ADAPTER *prAdapter)
{
	if (!prAdapter)
		return FALSE;

	if (!p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[0]) ||
	    !p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[1]))
		return FALSE;

	/* use netdev to check whether mlo sap or dual sap */
	if (prAdapter->prGlueInfo->prP2PInfo[0]->aprRoleHandler ==
	    prAdapter->prGlueInfo->prP2PInfo[1]->aprRoleHandler)
		return FALSE;

	return TRUE;
}

u_int8_t p2pFuncIsDualAPActive(struct ADAPTER *prAdapter)
{
	uint8_t ucActiveSapNum = 0;
	uint8_t ucBssIndex = 0;
	struct BSS_INFO *prBssInfo = NULL;

	if (!prAdapter)
		return FALSE;

	if (!p2pFuncIsDualAPMode(prAdapter))
		return FALSE;

	for (ucBssIndex = 0;
	     ucBssIndex < prAdapter->ucSwBssIdNum;
	     ucBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
		if (prBssInfo &&
		    IS_BSS_APGO(prBssInfo) &&
		    IS_BSS_ACTIVE(prBssInfo)) {
			ucActiveSapNum += 1;
		}
	}

	return ucActiveSapNum > 1 ? TRUE : FALSE;
}

u_int8_t p2pFuncIsAPMode(struct P2P_CONNECTION_SETTINGS *prP2pConnSettings)
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
p2pFuncParseBeaconContent(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t aucBSSID[],
		uint8_t *pucIEInfo, uint32_t u4IELen)
{
	uint8_t *pucIE = (uint8_t *) NULL;
	uint32_t u4Offset = 0;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	uint8_t i = 0;
	struct RSN_INFO rRsnIe;
#if (CFG_SUPPORT_802_11AX == 1)
	uint8_t ucHe = 0;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	uint8_t ucEht = 0;
#endif
	u_int8_t fgIsApMode = FALSE;
	struct IE_RNR *rnr =  NULL;
	struct NEIGHBOR_AP_INFO_FIELD *info;

	kalMemZero(&rRsnIe, sizeof(struct RSN_INFO));

	ASSERT((prAdapter != NULL) && (prP2pBssInfo != NULL));

	if (u4IELen == 0) {
		DBGLOG(P2P, ERROR, "Zero beacon length.");
		return;
	}

	prP2pSpecificBssInfo =
		prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prP2pBssInfo->u4PrivateData];
	prP2pSpecificBssInfo->u2AttributeLen = 0;
	prP2pSpecificBssInfo->u2WpaIeLen = 0;
	prP2pSpecificBssInfo->u2RsnIeLen = 0;
	prP2pSpecificBssInfo->u2RsnxIeLen = 0;
	prP2pSpecificBssInfo->u2OweIeLen = 0;
	prP2pSpecificBssInfo->u2TpeIeLen = 0;
	prP2pSpecificBssInfo->fgMlIeExist = FALSE;
	fgIsApMode = p2pFuncIsAPMode(
		prAdapter->rWifiVar.prP2PConnSettings
		[prP2pBssInfo->u4PrivateData]);

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
#if (CFG_SUPPORT_802_11AX == 1)
	if (fgIsApMode)
		ucHe = prAdapter->rWifiVar.ucApHe;
	else
		ucHe = prAdapter->rWifiVar.ucP2pGoHe;

	if (!IS_FEATURE_FORCE_ENABLED(ucHe))
		prP2pBssInfo->ucPhyTypeSet &= ~PHY_TYPE_SET_802_11AX;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	if (fgIsApMode)
		ucEht = prAdapter->rWifiVar.ucApEht;
	else
		ucEht = prAdapter->rWifiVar.ucP2pGoEht;

	if (!IS_FEATURE_FORCE_ENABLED(ucEht))
		prP2pBssInfo->ucPhyTypeSet &= ~PHY_TYPE_SET_802_11BE;
#endif

	IE_FOR_EACH(pucIE, u4IELen, u4Offset) {
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
				"TIM IE, Len:%u, DTIM:%u\n",
				IE_LEN(pucIE),
				TIM_IE(pucIE)->ucDTIMPeriod);
			break;
#if CFG_SUPPORT_802_11D
		case ELEM_ID_COUNTRY_INFO: /* 7 */
			if (COUNTRY_IE(pucIE)->ucLength
				>= ELEM_MIN_LEN_COUNTRY_INFO &&
				COUNTRY_IE(pucIE)->ucLength
				<= ELEM_MAX_LEN_COUNTRY_INFO) {
				prP2pBssInfo->ucCountryIELen =
					COUNTRY_IE(pucIE)->ucLength;
				kalMemCopy(
				prP2pBssInfo->aucCountryStr,
				COUNTRY_IE(pucIE)->aucCountryStr, 3);
				kalMemCopy(
				prP2pBssInfo->aucSubbandTriplet,
				COUNTRY_IE(pucIE)->arCountryStr,
				COUNTRY_IE(pucIE)->ucLength - 3);
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
				DBGLOG(P2P, TRACE,
					"HT CAP IE would be overwritten by driver\n");

				DBGLOG(P2P, TRACE,
					"HT Cap Info:%x, AMPDU Param:%x\n",
					HT_CAP_IE(pucIE)->u2HtCapInfo,
					HT_CAP_IE(pucIE)->ucAmpduParam);

				DBGLOG(P2P, TRACE,
					"HT Extended Cap:%u, TX Beamforming Cap:%u, Ant Selection Cap:%u\n",
					HT_CAP_IE(pucIE)
						->u2HtExtendedCap,
					HT_CAP_IE(pucIE)
						->u4TxBeamformingCap,
					HT_CAP_IE(pucIE)->ucAselCap);

				prP2pBssInfo->ucPhyTypeSet |=
					PHY_TYPE_SET_802_11N;
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
				} else if (rsnKeyMgmtSae(
					rRsnIe.au4AuthKeyMgtSuite[i]))
					prP2pBssInfo
					->u4RsnSelectedAKMSuite
					= rRsnIe.au4AuthKeyMgtSuite[i];
				else if (rRsnIe.au4AuthKeyMgtSuite[i]
				== RSN_AKM_SUITE_OWE)
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
		{
#ifndef CFG_SUPPORT_P2P_GO_KEEP_RATE_SETTING
			uint8_t ucCurrLen =
				prP2pBssInfo->ucAllSupportedRatesLen;
			uint8_t ucIeLen =
				EXT_SUP_RATES_IE(pucIE)->ucLength;
			/* ELEM_ID_SUP_RATES should be placed
			 * before ELEM_ID_EXTENDED_SUP_RATES.
			 */
			DBGLOG(P2P, TRACE, "Ex Support Rate IE\n");
			if (ucIeLen < (RATE_NUM_SW - ucCurrLen)) {
				kalMemCopy(&
					(prP2pBssInfo
					->aucAllSupportedRates
					[ucCurrLen]),
					EXT_SUP_RATES_IE(pucIE)
					->aucExtSupportedRates,
					ucIeLen);

				DBGLOG_MEM8(P2P, TRACE,
					EXT_SUP_RATES_IE(pucIE)
					->aucExtSupportedRates,
					EXT_SUP_RATES_IE(pucIE)
					->ucLength);

				prP2pBssInfo->ucAllSupportedRatesLen +=
					ucIeLen;
			}
#endif
			break;
		}
		case ELEM_ID_HT_OP:
			/* 61 *//* V *//* TODO: */
			{
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
			DBGLOG(P2P, TRACE, "VHT CAP IE\n");
			prP2pBssInfo->ucPhyTypeSet |=
					PHY_TYPE_SET_802_11AC;
			break;
		case ELEM_ID_VHT_OP:
			DBGLOG(P2P, TRACE, "VHT OP IE\n");
			prP2pBssInfo->ucPhyTypeSet |=
					PHY_TYPE_SET_802_11AC;
			break;
		case ELEM_ID_RSNX:
			DBGLOG(P2P, TRACE, "RSNXIE\n");
			if (IE_LEN(pucIE) > ELEM_MAX_LEN_RSN) {
				DBGLOG(P2P, ERROR,
					"RSN IE length is unexpected !!\n");
				return;
			}
			kalMemCopy(
				prP2pSpecificBssInfo->aucRsnxIeBuffer,
				pucIE, IE_SIZE(pucIE));
			prP2pSpecificBssInfo->u2RsnxIeLen
				= IE_SIZE(pucIE);
			break;
		case ELEM_ID_RESERVED:
			DBGLOG(P2P, TRACE, "IE_ID_EXT=%d\n",
				IE_ID_EXT(pucIE));
#if (CFG_SUPPORT_802_11AX == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_CAP ||
			    IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_OP) {
				if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_OP) {
					struct _IE_HE_OP_T *prHeOp;

					prHeOp = (struct _IE_HE_OP_T *) pucIE;
					if (!prAdapter->rWifiVar.fgSapAddTPEIE)
						prP2pBssInfo->ucBssColorInfo =
							prHeOp->ucBssColorInfo;
				}

				prP2pBssInfo->ucPhyTypeSet |=
					PHY_TYPE_SET_802_11AX;
			}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_EHT_CAPS ||
			    IE_ID_EXT(pucIE) == ELEM_EXT_ID_EHT_OP)
				prP2pBssInfo->ucPhyTypeSet |=
					PHY_TYPE_SET_802_11BE;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_MLD) {
				struct MULTI_LINK_INFO rMlInfo;

				MLD_PARSE_BASIC_MLIE(&rMlInfo, pucIE,
					IE_SIZE(pucIE), /* no need fragment */
					aucBSSID,
					MAC_FRAME_BEACON);

				prP2pSpecificBssInfo->fgMlIeExist = TRUE;
			}
#endif
#endif
			break;

		case ELEM_ID_TX_PWR_ENVELOPE:
			if (prP2pSpecificBssInfo->u2TpeIeLen +
			    IE_SIZE(pucIE) > MAX_TPE_IE_LENGTH) {
				DBGLOG(P2P, ERROR,
				"TPE IE length %d %d exceeds %d\n",
				prP2pSpecificBssInfo->u2TpeIeLen,
				IE_SIZE(pucIE),
				MAX_TPE_IE_LENGTH);

				prP2pSpecificBssInfo->u2TpeIeLen = 0;
				return;
			}
			kalMemCopy(
				prP2pSpecificBssInfo->aucTpeIeBuffer +
				prP2pSpecificBssInfo->u2TpeIeLen,
				pucIE, IE_SIZE(pucIE));
			prP2pSpecificBssInfo->u2TpeIeLen
				+= IE_SIZE(pucIE);
			break;

		case ELEM_ID_RNR:
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			/* rnr is composed in driver for mlo case */
			if (p2pRoleFsmNeedMlo(prAdapter,
					      prP2pBssInfo->u4PrivateData))
				break;
#endif
			kalMemCopy(prP2pSpecificBssInfo->aucRnrIeBuffer,
				   pucIE, IE_SIZE(pucIE));
			prP2pSpecificBssInfo->u2RnrIeLen = IE_SIZE(pucIE);
			rnr = (struct IE_RNR *)pucIE;
			info = (struct NEIGHBOR_AP_INFO_FIELD *)
				rnr->aucInfoField;
			DBGLOG(P2P, INFO,
				"(RNR IE length,op,ch) = (%u,%u,%u)\n",
				IE_LEN(pucIE),
				info->ucOpClass,
				info->ucChannelNum);

			break;

		default:
			DBGLOG(P2P, TRACE,
				"Unprocessed element ID:%d LEN:%d\n",
				IE_ID(pucIE), IE_LEN(pucIE));
			break;
		}
	}

	bssDetermineApBssInfoPhyTypeSet(prAdapter, fgIsApMode, prP2pBssInfo);
}				/* p2pFuncParseBeaconContent */

/* Code refactoring for AOSP */
static void
p2pFuncParseBeaconVenderId(struct ADAPTER *prAdapter,
		uint8_t *pucIE,
		struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo,
		uint8_t ucRoleIndex)
{
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
	} else if (p2pFuncParseCheckForP2PInfoElem(
		prAdapter, pucIE, &ucOuiType)) {
		if (ucOuiType == VENDOR_OUI_TYPE_P2P ||
			ucOuiType == VENDOR_OUI_TYPE_WFD) {
			uint32_t u4EmptyLen;

			DBGLOG(P2P, TRACE, "%s IE in supplicant\n",
				ucOuiType == VENDOR_OUI_TYPE_P2P ?
				"P2P" : "WFD");

			u4EmptyLen = sizeof(prP2pSpecificBssInfo
				->aucAttributesCache) -
				prP2pSpecificBssInfo->u2AttributeLen;
			if (u4EmptyLen < IE_SIZE(pucIE)) {
				DBGLOG(P2P, ERROR,
					"Full, EmptyLen[%d] IE_SIZE[%d]\n",
					u4EmptyLen, IE_SIZE(pucIE));
				return;
			}

			kalMemCopy(&prP2pSpecificBssInfo
				->aucAttributesCache
				[prP2pSpecificBssInfo->u2AttributeLen],
				pucIE, IE_SIZE(pucIE));
			prP2pSpecificBssInfo->u2AttributeLen +=
				IE_SIZE(pucIE);
		} else if (ucOuiType == VENDOR_OUI_TYPE_OWE) {
			if (IE_LEN(pucIE) > ELEM_MAX_LEN_WPA) {
				DBGLOG(P2P, ERROR,
					"RSN IE length is unexpected !!\n");
				return;
			}
			kalMemCopy(
				prP2pSpecificBssInfo->aucOweIeBuffer,
				pucIE, IE_SIZE(pucIE));
			prP2pSpecificBssInfo->u2OweIeLen
				= IE_SIZE(pucIE);
			DBGLOG(P2P, INFO,
				"[OWE] Trans IE in supplicant\n");
		} else {
			DBGLOG(P2P, TRACE,
				"Unknown 50-6F-9A-%d IE.\n",
				ucOuiType);
		}
	} else {
		uint32_t u4EmptyLen;

		DBGLOG(P2P, TRACE,
			"Driver unprocessed Vender Specific IE\n");

		u4EmptyLen = sizeof(prP2pSpecificBssInfo
			->aucAttributesCache) -
			prP2pSpecificBssInfo->u2AttributeLen;
		if (u4EmptyLen < IE_SIZE(pucIE)) {
			DBGLOG(P2P, ERROR,
				"Full, EmptyLen[%d] IE_SIZE[%d]\n",
				u4EmptyLen, IE_SIZE(pucIE));
			return;
		}

		kalMemCopy(&prP2pSpecificBssInfo->aucAttributesCache
			[prP2pSpecificBssInfo->u2AttributeLen],
			pucIE, IE_SIZE(pucIE));

		prP2pSpecificBssInfo->u2AttributeLen +=
			IE_SIZE(pucIE);
	}
}

struct BSS_DESC *
p2pFuncKeepOnConnection(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
		struct P2P_SCAN_REQ_INFO *prScanReqInfo)
{
	struct BSS_DESC *prTargetBss = (struct BSS_DESC *) NULL;
	struct P2P_JOIN_INFO *prJoinInfo =
		(struct P2P_JOIN_INFO *) NULL;
	struct BSS_DESC_SET set;
	u_int8_t fgNeedMlScan = FALSE;

	prJoinInfo = &(prP2pRoleFsmInfo->rJoinInfo);

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prBssInfo != NULL) &&
			(prConnReqInfo != NULL) &&
			(prScanReqInfo != NULL));

		if (prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
			break;
		/* Update connection request information. */
		ASSERT(prConnReqInfo->eConnRequest == P2P_CONNECTION_TYPE_GC);

		/* Find BSS Descriptor first. */
		prTargetBss = scanP2pSearchDesc(prAdapter, prConnReqInfo, &set,
						&fgNeedMlScan);

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
			uint8_t i;

			p2pFillLinkBssDesc(prAdapter, prP2pRoleFsmInfo, &set);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (set.ucLinkNum > 1)
				p2pLinkInitGcOtherLinks(prAdapter,
							prP2pRoleFsmInfo,
							set.ucLinkNum);
#endif

			prJoinInfo->ucAvailableAuthTypes =
				(uint8_t) AUTH_TYPE_OPEN_SYSTEM;

			for (i = 0; i < MLD_LINK_MAX; i++) {
				struct BSS_INFO *prP2pBssInfo =
					p2pGetLinkBssInfo(prP2pRoleFsmInfo, i);
				struct BSS_DESC *prBssDesc =
					p2pGetLinkBssDesc(prP2pRoleFsmInfo, i);
				struct P2P_CHNL_REQ_INFO *prChnlReqInfo =
					p2pGetChnlReqInfo(prP2pRoleFsmInfo, i);

				if (!prP2pBssInfo || !prBssDesc)
					continue;

				prChnlReqInfo->u8Cookie = 0;
				prChnlReqInfo->ucReqChnlNum =
					prBssDesc->ucChannelNum;
				prChnlReqInfo->eBand =
					prBssDesc->eBand;
				prChnlReqInfo->eChnlSco =
					prBssDesc->eSco;
				prChnlReqInfo->u4MaxInterval =
					AIS_JOIN_CH_REQUEST_INTERVAL;
				prChnlReqInfo->eChnlReqType =
					CH_REQ_TYPE_JOIN;

				prChnlReqInfo->eChannelWidth =
					prBssDesc->eChannelWidth;
				prChnlReqInfo->ucCenterFreqS1 =
					prBssDesc->ucCenterFreqS1;
				prChnlReqInfo->ucCenterFreqS2 =
					prBssDesc->ucCenterFreqS2;
			}
		}

	} while (FALSE);

	return prTargetBss;
}				/* p2pFuncKeepOnConnection */

/* Currently Only for ASSOC Response Frame. */
void p2pFuncStoreAssocRspIEBuffer(struct ADAPTER *prAdapter,
		struct P2P_JOIN_INFO *prP2pJoinInfo,
		struct SW_RFB *prSwRfb)
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
p2pFuncMgmtFrameRegister(struct ADAPTER *prAdapter,
		uint16_t u2FrameType,
		u_int8_t fgIsRegistered,
		uint32_t *pu4P2pPacketFilter)
{
	uint32_t u4NewPacketFilter = 0;
	struct CMD_RX_PACKET_FILTER rSetRxPacketFilter;

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

void p2pFuncUpdateMgmtFrameRegister(struct ADAPTER *prAdapter,
		uint32_t u4OsFilter)
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

void p2pFuncGetStationInfo(struct ADAPTER *prAdapter,
		uint8_t *pucMacAddr,
		struct P2P_STATION_INFO *prStaInfo)
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
p2pFuncGetAttriList(struct ADAPTER *prAdapter,
		uint8_t ucOuiType,
		uint8_t *pucIE,
		uint16_t u2IELength,
		uint8_t **ppucAttriList,
		uint16_t *pu2AttriListLen)
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
p2pFuncGetAttriListAction(struct ADAPTER *prAdapter,
		struct IE_P2P *prIe,
		uint8_t ucOuiType,
		uint8_t **pucAttriListStart,
		uint16_t *u2AttriListLen,
		u_int8_t *fgIsAllocMem,
		u_int8_t *fgBackupAttributes,
		uint16_t *u2BufferSize)
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
					((uintptr_t) (*pucAttriListStart) +
					(uintptr_t) (*u2AttriListLen)),
					&prIe->aucP2PAttributes[0], u2CopyLen);
				*u2AttriListLen += u2CopyLen;
			}

		}
	} while (0);
}
#endif

uint32_t p2pCalculateWSCIELen(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prP2pBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prP2pBssInfo)
		return 0;
	return kalP2PCalWSC_IELen(prAdapter->prGlueInfo, 2,
					(uint8_t) prP2pBssInfo->u4PrivateData);
}

void p2pGenerateWSCIE(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prP2pBssInfo;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (!prP2pBssInfo)
		return;

	if (prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_NON_TX_LINK)
		return;

	kalP2PGenWSC_IE(prAdapter->prGlueInfo,
		2,
		(uint8_t *)
		((uintptr_t) prMsduInfo->prPacket +
		(uintptr_t) prMsduInfo->u2FrameLength),
		(uint8_t) prP2pBssInfo->u4PrivateData);
	prMsduInfo->u2FrameLength += (uint16_t)
		kalP2PCalWSC_IELen(prAdapter->prGlueInfo,
		2,
		(uint8_t) prP2pBssInfo->u4PrivateData);
}

#if CFG_SUPPORT_WFD
uint32_t p2pCalculateWFDIELen(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prP2pBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prP2pBssInfo)
		return 0;
	return prAdapter->prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData]
				->u2WFDIELen;
}

void p2pGenerateWFDIE(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prP2pBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (!prP2pBssInfo)
		return;

	if (prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_NON_TX_LINK)
		return;

	kalMemCopy((uint8_t *)
		((uintptr_t) prMsduInfo->prPacket +
		(uintptr_t) prMsduInfo->u2FrameLength),
		prAdapter->prGlueInfo->prP2PInfo
			[prP2pBssInfo->u4PrivateData]->aucWFDIE,
		prAdapter->prGlueInfo->prP2PInfo
			[prP2pBssInfo->u4PrivateData]
			->u2WFDIELen);
	prMsduInfo->u2FrameLength +=
	(uint16_t) prAdapter->prGlueInfo
		->prP2PInfo[prP2pBssInfo->u4PrivateData]
		->u2WFDIELen;
}
#endif


uint32_t p2pCalculateP2PIELen(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prP2pBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	uint16_t u2EstimateSize = 0;
	uint32_t u4Idx = 0;
	if (!prP2pBssInfo)
		return 0;

	for (u4Idx = 0; u4Idx < MAX_P2P_IE_SIZE; u4Idx++)
		u2EstimateSize +=
			kalP2PCalP2P_IELen(
				prAdapter->prGlueInfo,
				u4Idx,
				(uint8_t)
				prP2pBssInfo->u4PrivateData);

	if (u2EstimateSize > 0)
		u2EstimateSize +=
			p2pFuncCalculateP2P_IE_NoA(prAdapter,
				ucBssIndex, NULL);

	return u2EstimateSize;
}

void p2pGenerateP2PIE(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prP2pBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	uint32_t u4Idx = 0;
	uint16_t u2OriFrameLength = 0;

	if (!prP2pBssInfo)
		return;

	if (prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_NON_TX_LINK)
		return;

	u2OriFrameLength = prMsduInfo->u2FrameLength;

	for (u4Idx = 0; u4Idx < MAX_P2P_IE_SIZE; u4Idx++) {
		kalP2PGenP2P_IE(prAdapter->prGlueInfo,
			u4Idx,
			(uint8_t *)
			((uintptr_t)
			prMsduInfo->prPacket +
			(uintptr_t)
			prMsduInfo->u2FrameLength),
			(uint8_t) prP2pBssInfo->u4PrivateData);

		prMsduInfo->u2FrameLength +=
			(uint16_t)
			kalP2PCalP2P_IELen(
			prAdapter->prGlueInfo,
			u4Idx,
			(uint8_t) prP2pBssInfo->u4PrivateData);
	}

	/* Append NoA only when P2P IE already exists from supplicant */
	if (prMsduInfo->u2FrameLength > u2OriFrameLength)
		p2pFuncGenerateP2P_IE_NoA(prAdapter, prMsduInfo);
}

#if CFG_SUPPORT_CUSTOM_VENDOR_IE
uint32_t p2pCalculateVendorIELen(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prP2pBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prP2pBssInfo)
		return 0;
	return prAdapter->prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData]
			->u2VenderIELen;
}

void p2pGenerateVendorIE(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prP2pBssInfo;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);

	if (!prP2pBssInfo)
		return;

	if (prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_NON_TX_LINK)
		return;

	kalMemCopy((uint8_t *)
		((uintptr_t) prMsduInfo->prPacket +
		(uintptr_t) prMsduInfo->u2FrameLength),
		prAdapter->prGlueInfo->prP2PInfo
			[prP2pBssInfo->u4PrivateData]
			->aucVenderIE,
		prAdapter->prGlueInfo->prP2PInfo
			[prP2pBssInfo->u4PrivateData]
			->u2VenderIELen);
	prMsduInfo->u2FrameLength +=
	(uint16_t) prAdapter->prGlueInfo
		->prP2PInfo[prP2pBssInfo->u4PrivateData]
		->u2VenderIELen;
}
#endif

struct MSDU_INFO *p2pFuncProcessP2pProbeRsp(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx, uint8_t fgNonTxLink, uint8_t fgHide,
	struct WLAN_BEACON_FRAME *prProbeRspFrame)
{
	struct MSDU_INFO *prRetMsduInfo = NULL;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
	uint16_t u2EstimateSize = 0, u2EstimatedExtraIELen = 0;
	uint32_t u4Idx = 0;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

	/* 3 Check the total size & current frame. */
	u2EstimateSize = WLAN_MAC_MGMT_HEADER_LEN +
	    TIMESTAMP_FIELD_LEN +
	    BEACON_INTERVAL_FIELD_LEN +
	    CAP_INFO_FIELD_LEN +
	    (ELEM_HDR_LEN + ELEM_MAX_LEN_SSID) +
	    (ELEM_HDR_LEN + ELEM_MAX_LEN_SUP_RATES) +
	    (ELEM_HDR_LEN + ELEM_MAX_LEN_DS_PARAMETER_SET);

	u2EstimatedExtraIELen = 0;

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(txProbeRspIETable); u4Idx++) {
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

	u2EstimateSize += u2EstimatedExtraIELen;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	u2EstimateSize += ELEM_HDR_LEN + MAX_LEN_OF_MLIE +
			  ELEM_HDR_LEN + MAX_LEN_OF_FRAGMENT;
#endif

	prRetMsduInfo = nicAllocMgmtPktForDataQ(
		prAdapter, u2EstimateSize + sizeof(uint64_t));

	if (prRetMsduInfo == NULL) {
		DBGLOG(P2P, WARN,
			"No packet for sending new probe response, use original one\n");
		return NULL;
	}

	prRetMsduInfo->ucBssIndex = ucBssIdx;

	if (fgNonTxLink)
		prRetMsduInfo->ucControlFlag |= MSDU_CONTROL_FLAG_NON_TX_LINK;
	if (fgHide)
		prRetMsduInfo->ucControlFlag |= MSDU_CONTROL_FLAG_HIDE_INFO;

	if (prProbeRspFrame)
		/* 3 Compose / Re-compose probe response frame. */
		bssComposeBeaconProbeRespFrameHeaderAndFF((uint8_t *)
			((uintptr_t) (prRetMsduInfo->prPacket) +
			MAC_TX_RESERVED_FIELD),
			prProbeRspFrame->aucDestAddr,
			prProbeRspFrame->aucSrcAddr,
			prProbeRspFrame->aucBSSID,
			prProbeRspFrame->u2BeaconInterval,
			prProbeRspFrame->u2CapInfo);
	else
		bssComposeBeaconProbeRespFrameHeaderAndFF((uint8_t *)
			((uintptr_t)(prRetMsduInfo->prPacket)
			+ MAC_TX_RESERVED_FIELD),
			NULL,
			prP2pBssInfo->aucOwnMacAddr,
			prP2pBssInfo->aucBSSID,
			prP2pBssInfo->u2BeaconInterval,
			prP2pBssInfo->u2CapInfo);

	prRetMsduInfo->u2FrameLength =
	    (WLAN_MAC_MGMT_HEADER_LEN +
	    TIMESTAMP_FIELD_LEN +
	    BEACON_INTERVAL_FIELD_LEN + CAP_INFO_FIELD_LEN);

	bssBuildBeaconProbeRespFrameCommonIEs(prRetMsduInfo,
		prP2pBssInfo, prProbeRspFrame ?
		prProbeRspFrame->aucDestAddr : NULL);

	prRetMsduInfo->ucStaRecIndex = STA_REC_INDEX_NOT_FOUND;

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(txProbeRspIETable); u4Idx++) {
		if (txProbeRspIETable[u4Idx].pfnAppendIE)
			txProbeRspIETable[u4Idx]
				.pfnAppendIE(prAdapter, prRetMsduInfo);

	}

	sortMgmtFrameIE(prAdapter, prRetMsduInfo);

	return prRetMsduInfo;
}				/* p2pFuncProcessP2pProbeRsp */

static void
p2pFuncProcessP2pProbeRspSsid(struct ADAPTER *prAdapter,
		uint8_t *pucIEBuf,
		uint8_t ucBssIdx)
{
	struct BSS_INFO *prP2pBssInfo;

	if (SSID_IE(pucIEBuf)->ucLength <= 7) {
		if (ucBssIdx != prAdapter->ucP2PDevBssIdx) {
			DBGLOG(P2P, WARN,
				"Wrong SSID:%s with bssid=%d\n",
				pucIEBuf, ucBssIdx);
			return;
		}

		prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
		if (!prP2pBssInfo)
			return;
		COPY_SSID(
			prP2pBssInfo->aucSSID,
			prP2pBssInfo->ucSSIDLen,
			SSID_IE(pucIEBuf)->aucSSID,
			SSID_IE(pucIEBuf)->ucLength);
	}
}

static void
p2pFuncProcessP2pProbeRspVendor(struct ADAPTER *prAdapter,
		uint8_t *pucIEBuf,
		uint8_t ucBssIdx)
{
	uint8_t ucOuiType = 0;
	uint16_t u2SubTypeVersion = 0;
	struct BSS_INFO *prP2pBssInfo;
	struct GL_P2P_INFO *prP2PInfo;
	uint32_t u4Idx;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prP2pBssInfo)
		return;
	prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[
		prP2pBssInfo->u4PrivateData];

	if (rsnParseCheckForWFAInfoElem(prAdapter,
		pucIEBuf, &ucOuiType, &u2SubTypeVersion)) {
		if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
			kalP2PUpdateWSC_IE(prAdapter->prGlueInfo,
				2, pucIEBuf,
				IE_SIZE(pucIEBuf),
				(uint8_t)
				((struct BSS_INFO *)prP2pBssInfo)
				->u4PrivateData);
		}
	} else if (p2pFuncParseCheckForP2PInfoElem(prAdapter,
		pucIEBuf, &ucOuiType)) {
		if (ucOuiType == VENDOR_OUI_TYPE_P2P) {
			for (u4Idx = 0; u4Idx < MAX_P2P_IE_SIZE; u4Idx++) {
				if (prP2PInfo->u2P2PIELen[u4Idx] == 0) {
					kalP2PUpdateP2P_IE(
						prAdapter->prGlueInfo,
						u4Idx,
						pucIEBuf,
						IE_SIZE(pucIEBuf),
						(uint8_t)
						((struct BSS_INFO *)
						prP2pBssInfo)
						->u4PrivateData);
					break;
				}
			}
			if (u4Idx == MAX_P2P_IE_SIZE)
				DBGLOG(P2P, WARN,
					"Too much P2P IE for ProbeResp, skip update\n");
		}
#if CFG_SUPPORT_WFD
		else if (ucOuiType == VENDOR_OUI_TYPE_WFD) {
			DBGLOG(P2P, INFO,
			       "WFD IE is found in probe resp (supp). Len %u\n",
			       IE_SIZE(pucIEBuf));
			if ((sizeof(prAdapter->prGlueInfo->prP2PInfo
				[((struct BSS_INFO *)prP2pBssInfo)
				->u4PrivateData]->aucWFDIE)
				>= IE_SIZE(pucIEBuf))) {
				kalMemCopy(prAdapter->prGlueInfo
					->prP2PInfo
					[((struct BSS_INFO *)
					prP2pBssInfo)
					->u4PrivateData]->aucWFDIE,
					pucIEBuf, IE_SIZE(pucIEBuf));
				prAdapter->prGlueInfo
					->prP2PInfo
					[((struct BSS_INFO *)
					prP2pBssInfo)
					->u4PrivateData]->u2WFDIELen =
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
			[((struct BSS_INFO *)prP2pBssInfo)
			->u4PrivateData]->u2VenderIELen
			+ IE_SIZE(pucIEBuf)) < 1024) {
			kalMemCopy(prAdapter->prGlueInfo
				->prP2PInfo
				[((struct BSS_INFO *)
				prP2pBssInfo)
				->u4PrivateData]->aucVenderIE +
				prAdapter->prGlueInfo
				->prP2PInfo
				[((struct BSS_INFO *)
				prP2pBssInfo)
				->u4PrivateData]->u2VenderIELen,
				pucIEBuf, IE_SIZE(pucIEBuf));
			prAdapter->prGlueInfo
				->prP2PInfo
				[((struct BSS_INFO *)
				prP2pBssInfo)
				->u4PrivateData]->u2VenderIELen +=
				IE_SIZE(pucIEBuf);
		}
#endif
	}
}

/* Code refactoring for AOSP */
void
p2pFuncProcessP2pProbeRspAction(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMgmtTxMsdu,
		uint8_t ucBssIdx)
{
	struct BSS_INFO *prP2pBssInfo;
	struct GL_P2P_INFO *prP2PInfo;
	struct WLAN_BEACON_FRAME *prProbeRspFrame =
		(struct WLAN_BEACON_FRAME *) NULL;
	uint8_t *pucIEBuf = (uint8_t *) NULL;
	uint16_t u2Offset = 0, u2IELength = 0, u2ProbeRspHdrLen = 0;
	uint32_t u4Idx;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prP2pBssInfo)
		return;
	prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[
		prP2pBssInfo->u4PrivateData];

	/* 3 Make sure this is probe response frame. */
	prProbeRspFrame = (struct WLAN_BEACON_FRAME *)
		((uintptr_t) prMgmtTxMsdu->prPacket +
		MAC_TX_RESERVED_FIELD);

	if ((prProbeRspFrame->u2FrameCtrl & MASK_FRAME_TYPE) !=
			MAC_FRAME_PROBE_RSP) {
		DBGLOG(P2P, INFO, "fctrl=0x%lx is not probe resp",
			prProbeRspFrame->u2FrameCtrl & MASK_FRAME_TYPE);
		return;
	}

	/* 3 Get the importent P2P IE. */
	u2ProbeRspHdrLen =
	    (WLAN_MAC_MGMT_HEADER_LEN +
	    TIMESTAMP_FIELD_LEN +
	    BEACON_INTERVAL_FIELD_LEN +
	    CAP_INFO_FIELD_LEN);
	pucIEBuf = prProbeRspFrame->aucInfoElem;
	u2IELength = prMgmtTxMsdu->u2FrameLength - u2ProbeRspHdrLen;

	/* reset target ie length */
	prP2PInfo->u2WSCIELen[2] = 0;
	for (u4Idx = 0; u4Idx < MAX_P2P_IE_SIZE; u4Idx++)
		prP2PInfo->u2P2PIELen[u4Idx] = 0;
#if CFG_SUPPORT_WFD
	prP2PInfo->u2WFDIELen = 0;
#endif
#if CFG_SUPPORT_CUSTOM_VENDOR_IE
	prP2PInfo->u2VenderIELen = 0;
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prP2PInfo->u2MlIELen = 0;
#endif

	IE_FOR_EACH(pucIEBuf, u2IELength, u2Offset) {
		switch (IE_ID(pucIEBuf)) {
		case ELEM_ID_SSID:
		{
			p2pFuncProcessP2pProbeRspSsid(
				prAdapter,
				pucIEBuf,
				ucBssIdx);
		}
			break;
		case ELEM_ID_VENDOR:
		{
			p2pFuncProcessP2pProbeRspVendor(
				prAdapter,
				pucIEBuf,
				ucBssIdx);
		}
			break;
		case ELEM_ID_RESERVED:
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (IE_ID_EXT(pucIEBuf) == ELEM_EXT_ID_MLD) {
				kalMemCopy(prP2PInfo->aucMlIE,
					pucIEBuf, IE_SIZE(pucIEBuf));
				prP2PInfo->u2MlIELen = IE_SIZE(pucIEBuf);
			}
#endif
			break;
		default:
			break;
		}
	}
}

#if 0 /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0) */
uint32_t
p2pFuncCalculateExtra_IELenForBeacon(struct ADAPTER *prAdapter,
		ENUM_NETWORK_TYPE_INDEX_T eNetTypeIndex,
		struct STA_RECORD *prStaRec)
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

void p2pFuncGenerateExtra_IEForBeacon(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
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
uint32_t p2pFuncCalculateP2p_IELenForBeacon(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx, struct STA_RECORD *prStaRec)
{
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpeBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	uint32_t u4IELen = 0;
	struct BSS_INFO *prBssInfo;

	do {
		ASSERT_BREAK((prAdapter != NULL)
			&& (ucBssIdx < prAdapter->ucSwBssIdNum));

		prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

		if (!prAdapter->fgIsP2PRegistered)
			break;

		if (p2pFuncIsAPMode(
			prAdapter->rWifiVar.prP2PConnSettings
			[prBssInfo->u4PrivateData]))
			break;

		if (!p2pNeedAppendP2pIE(prAdapter,
			prBssInfo)) {
			DBGLOG(BSS, LOUD,
				"Skip p2p ie for role%d\n",
				prBssInfo->u4PrivateData);
			break;
		}

		prP2pSpeBssInfo =
			prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prBssInfo->u4PrivateData];

		u4IELen = prP2pSpeBssInfo->u2AttributeLen;

	} while (FALSE);

	return u4IELen;
}				/* p2pFuncCalculateP2p_IELenForBeacon */

void p2pFuncGenerateP2p_IEForBeacon(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
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
		if (!prBssInfo)
			break;

		prP2pSpeBssInfo =
			prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prBssInfo->u4PrivateData];
		if (!prP2pSpeBssInfo)
			break;

		if (p2pFuncIsAPMode(
			prAdapter->rWifiVar.prP2PConnSettings
			[prBssInfo->u4PrivateData]))
			break;

		if (!p2pNeedAppendP2pIE(prAdapter,
			prBssInfo)) {
			DBGLOG(BSS, LOUD,
				"Skip p2p ie for role%d\n",
				prBssInfo->u4PrivateData);
			break;
		}

		pucIEBuf = (uint8_t *) ((uintptr_t) prMsduInfo->prPacket +
			(uintptr_t) prMsduInfo->u2FrameLength);

		kalMemCopy(pucIEBuf,
			prP2pSpeBssInfo->aucAttributesCache,
			prP2pSpeBssInfo->u2AttributeLen);

		prMsduInfo->u2FrameLength += prP2pSpeBssInfo->u2AttributeLen;

	} while (FALSE);
}				/* p2pFuncGenerateP2p_IEForBeacon */

uint32_t p2pFuncCalculateWSC_IELenForBeacon(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx, struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

	if (!prP2pBssInfo ||
		prP2pBssInfo->eNetworkType != NETWORK_TYPE_P2P)
		return 0;

	return kalP2PCalWSC_IELen(prAdapter->prGlueInfo,
		0, (uint8_t) prP2pBssInfo->u4PrivateData);
}				/* p2pFuncCalculateP2p_IELenForBeacon */

void p2pFuncGenerateWSC_IEForBeacon(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucBuffer;
	uint16_t u2IELen = 0;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (!prP2pBssInfo)
		return;
	if (prP2pBssInfo->eNetworkType != NETWORK_TYPE_P2P)
		return;

	u2IELen = (uint16_t) kalP2PCalWSC_IELen(prAdapter->prGlueInfo,
		0, (uint8_t) prP2pBssInfo->u4PrivateData);

	pucBuffer = (uint8_t *) ((uintptr_t) prMsduInfo->prPacket +
		(uintptr_t) prMsduInfo->u2FrameLength);

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
uint32_t p2pFuncCalculateP2p_IELenForAssocRsp(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!prBssInfo ||
		prBssInfo->eNetworkType != NETWORK_TYPE_P2P)
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
void p2pFuncGenerateP2p_IEForAssocRsp(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;


	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prStaRec) {
		DBGLOG(P2P, ERROR, "prStaRec of ucStaRecIndex %d is NULL!\n",
			prMsduInfo->ucStaRecIndex);
		return;
	}

	if (IS_STA_IN_P2P(prAdapter, prStaRec)) {
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
p2pFuncCalculateP2P_IELen(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec,
		struct APPEND_VAR_ATTRI_ENTRY arAppendAttriTable[],
		uint32_t u4AttriTableSize)
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
p2pFuncGenerateP2P_IE(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		u_int8_t fgIsAssocFrame,
		uint16_t *pu2Offset,
		uint8_t *pucBuf,
		uint16_t u2BufSize,
		struct APPEND_VAR_ATTRI_ENTRY arAppendAttriTable[],
		uint32_t u4AttriTableSize)
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

		pucBuffer = (uint8_t *) ((uintptr_t) pucBuf + (*pu2Offset));

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
						((uintptr_t)
						prIeP2P +
						(VENDOR_OUI_TYPE_LEN +
						P2P_MAXIMUM_ATTRIBUTE_LEN));

					prIeP2P = (struct IE_P2P *)
						((uintptr_t) prIeP2P +
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
p2pFuncAppendAttriStatusForAssocRsp(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		u_int8_t fgIsAssocFrame,
		uint16_t *pu2Offset,
		uint8_t *pucBuf,
		uint16_t u2BufSize)
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
		((uintptr_t) pucBuf +
		(uintptr_t) (*pu2Offset));

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
p2pFuncAppendAttriExtListenTiming(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		u_int8_t fgIsAssocFrame,
		uint16_t *pu2Offset,
		uint8_t *pucBuf,
		uint16_t u2BufSize)
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
		((uintptr_t) pucBuf +
		(uintptr_t) (*pu2Offset));

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
p2pFuncGetSpecIE(struct ADAPTER *prAdapter,
		uint8_t *pucIEBuf,
		uint16_t u2BufferLen,
		uint8_t ucElemID,
		u_int8_t *pfgIsMore)
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
p2pFuncGetSpecAttri(struct ADAPTER *prAdapter,
		uint8_t ucOuiType,
		uint8_t *pucIEBuf,
		uint16_t u2BufferLen,
		uint8_t ucAttriID)
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
				ASSERT((uintptr_t) prP2pIE
					>= (uintptr_t) pucIE);
				u2BufferLenLeft = u2BufferLen -
					(uint16_t) (((uintptr_t) prP2pIE) -
					((uintptr_t) pucIEBuf));

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
					(((uintptr_t) prP2pIE) +
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
p2pFuncGetSpecAttriAction(struct IE_P2P *prP2pIE,
		uint8_t ucOuiType,
		uint8_t ucAttriID,
		struct P2P_ATTRIBUTE **prTargetAttri)
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
p2pFuncGenerateBeaconProbeRsp(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct MSDU_INFO *prMsduInfo,
		u_int8_t fgIsProbeRsp)
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
p2pFuncComposeBeaconProbeRspTemplate(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t *pucBcnBuffer,
		uint32_t u4BcnBufLen,
		u_int8_t fgIsProbeRsp,
		struct P2P_PROBE_RSP_UPDATE_INFO *prP2pProbeRspInfo,
		u_int8_t fgSynToFW)
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
			((uintptr_t) (prMsduInfo->prPacket) +
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
#if CFG_SUPPORT_WFD
uint32_t wfdFuncCalculateWfdIELenForAssocRsp(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		struct STA_RECORD *prStaRec)
{

#if CFG_SUPPORT_WFD_COMPOSE_IE
	uint16_t u2EstimatedExtraIELen = 0;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!prBssInfo ||
		prBssInfo->eNetworkType != NETWORK_TYPE_P2P)
		return 0;

	if (!IS_STA_P2P_TYPE(prStaRec))
		return 0;

	u2EstimatedExtraIELen = prAdapter->prGlueInfo->
		prP2PInfo[prBssInfo->u4PrivateData]->u2WFDIELen;

	if (u2EstimatedExtraIELen < VENDOR_SPECIFIC_IE_LENGTH)
		return u2EstimatedExtraIELen;
	else
		return 0;
#else
	return 0;
#endif
}				/* wfdFuncCalculateWfdIELenForAssocRsp */

void wfdFuncGenerateWfdIEForAssocRsp(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{

#if CFG_SUPPORT_WFD_COMPOSE_IE
	struct STA_RECORD *prStaRec;
	uint16_t u2EstimatedExtraIELen;
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
	struct GLUE_INFO *prGlueInfo;
	struct GL_P2P_INFO *prP2PInfo;

	if (!prAdapter || !prMsduInfo)
		return;

	prGlueInfo = prAdapter->prGlueInfo;
	if (!prGlueInfo)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	if (!prStaRec)
		return;

	if (!IS_STA_P2P_TYPE(prStaRec))
		return;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (!prP2pBssInfo)
		return;

	prP2PInfo = prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData];
	if (!prP2PInfo)
		return;

	u2EstimatedExtraIELen = prP2PInfo->u2WFDIELen;
	if (u2EstimatedExtraIELen > 0 &&
		u2EstimatedExtraIELen < VENDOR_SPECIFIC_IE_LENGTH) {
		kalMemCopy((uint8_t *)((uintptr_t)prMsduInfo->prPacket +
			prMsduInfo->u2FrameLength),
			prP2PInfo->aucWFDIE, u2EstimatedExtraIELen);
		prMsduInfo->u2FrameLength += u2EstimatedExtraIELen;
	}

	return;
#else

	return;
#endif
}				/* wfdFuncGenerateWfdIEForAssocRsp */
#endif

void
p2pFuncComposeNoaAttribute(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex,
		uint8_t *aucNoaAttrArray,
		uint32_t *pu4Len)
{
	struct BSS_INFO *prBssInfo = NULL;
	struct P2P_ATTRI_NOA *prNoaAttr = NULL;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo = NULL;
	struct NOA_DESCRIPTOR *prNoaDesc = NULL;
	uint32_t u4NumOfNoaDesc = 0;
	uint32_t i = 0;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return;
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

uint32_t p2pFuncCalculateP2P_IE_NoA(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct STA_RECORD *prStaRec)
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

void p2pFuncGenerateP2P_IE_NoA(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{
	struct IE_P2P *prIeP2P;
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;
	uint32_t u4AttributeLen = 0;
	struct BSS_INFO *prBssInfo;

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];

	if (p2pFuncIsAPMode(
		prAdapter->rWifiVar.prP2PConnSettings
		[prBssInfo->u4PrivateData]))
		return;

	prIeP2P = (struct IE_P2P *)
		((uintptr_t) prMsduInfo->prPacket +
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

void p2pFunCleanQueuedMgmtFrame(struct ADAPTER *prAdapter,
		struct P2P_QUEUED_ACTION_FRAME *prFrame)
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

#if CFG_TC1_FEATURE
static u_int8_t p2pFuncSwitchSapChannelToDbdc(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prP2pBssInfo,
	uint8_t ucStaChannelNum,
	uint8_t ucSapChannelNum,
	enum ENUM_BAND eStaBand,
	enum ENUM_BAND eSapBand)
{
#if CFG_SUPPORT_DBDC
	if (prAdapter->rWifiVar.eDbdcMode ==
		ENUM_DBDC_MODE_DISABLED)
		return FALSE;

	if ((eStaBand == eSapBand) &&
		((eSapBand == BAND_5G &&
		ucStaChannelNum != 149)
#if (CFG_SUPPORT_WIFI_6G == 1)
		|| (eSapBand == BAND_6G)
#endif
	)) {
		struct RF_CHANNEL_INFO rRfChnlInfo;
		uint8_t ucBssIdx = 0;

		DBGLOG(P2P, INFO,
			"[DBDC] StaCH(%d), SapCH(%d)\n",
			ucStaChannelNum,
			ucSapChannelNum);

		if (p2pFuncRoleToBssIdx(
			prAdapter, prP2pBssInfo->u4PrivateData,
			&ucBssIdx) != WLAN_STATUS_SUCCESS)
			return FALSE;

		rlmGetChnlInfoForCSA(prAdapter,
			BAND_2G4, 6,
			ucBssIdx, &rRfChnlInfo);

		cnmSapChannelSwitchReq(prAdapter,
			&rRfChnlInfo,
			prP2pBssInfo->u4PrivateData);

		return TRUE;

	}
#endif

	return FALSE;
}
#endif


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
	uint8_t ucMaxBw = 0;
#if CFG_SUPPORT_DBDC
	struct DBDC_DECISION_INFO rDbdcDecisionInfo = {0};
#endif
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

	prChnlReqInfo = &prP2pRoleFsmInfo->rChnlReqInfo[0];

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

	kalP2pClearCsaChan(prGlueP2pInfo);

	DBGLOG(P2P, INFO, "switch gc channel: %s band\n",
		prP2pBssInfo->eBand == prChnlReqInfo->eBand ? "same" : "cross");

	/* Update HT, VHT PhyType of BssInfo when GC channel switch.
	 * The PhyType of StaRec will be updated after channel granted.
	 */
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prP2pBssInfo->eBand == BAND_6G) {
		prP2pBssInfo->ucPhyTypeSet &= ~(PHY_TYPE_SET_802_11N);
		prP2pBssInfo->ucPhyTypeSet &= ~(PHY_TYPE_SET_802_11AC);
	} else {
		prP2pBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11N;
		prP2pBssInfo->ucPhyTypeSet |= PHY_TYPE_SET_802_11AC;
	}
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (!IS_MLD_BSSINFO_MULTI(mldBssGetByBss(prAdapter, prP2pBssInfo)))
#endif
	{
		/* Indicate PM abort to sync BSS state with FW */
		nicPmIndicateBssAbort(prAdapter, prP2pBssInfo->ucBssIndex);
		prP2pBssInfo->ucDTIMPeriod = 0;

		/* Update BSS with temp. disconnect state to FW */
		p2pDeactivateAllLink(prAdapter,
			prP2pRoleFsmInfo,
			FALSE);
		p2pChangeMediaState(prAdapter, prP2pBssInfo,
			MEDIA_STATE_DISCONNECTED);
		nicUpdateBssEx(prAdapter,
			prP2pBssInfo->ucBssIndex,
			FALSE);

#if CFG_SUPPORT_DBDC
		CNM_DBDC_ADD_DECISION_INFO(rDbdcDecisionInfo,
			prP2pBssInfo->ucBssIndex,
			prP2pBssInfo->eBand,
			prP2pBssInfo->ucPrimaryChannel,
			prP2pBssInfo->ucWmmQueSet);

		cnmDbdcPreConnectionEnableDecision(prAdapter,
			&rDbdcDecisionInfo);
#endif
	}

	/* Update channel parameters & channel request info */
	rRfChnlInfo.ucChannelNum = prP2pBssInfo->ucPrimaryChannel;
	rRfChnlInfo.eBand = prP2pBssInfo->eBand;
	ucMaxBw = cnmGetBssMaxBw(prAdapter, prP2pBssInfo->ucBssIndex);
	if (ucMaxBw < rlmGetBssOpBwByVhtAndHtOpInfo(prP2pBssInfo))
		rlmFillVhtOpInfoByBssOpBw(prAdapter, prP2pBssInfo, ucMaxBw);
	rRfChnlInfo.ucChnlBw = kal_min_t(uint8_t,
		rlmGetBssOpBwByVhtAndHtOpInfo(prP2pBssInfo),
		ucMaxBw);
	rRfChnlInfo.u2PriChnlFreq =
		nicChannelNum2Freq(rRfChnlInfo.ucChannelNum,
			rRfChnlInfo.eBand) / 1000;
	rRfChnlInfo.u4CenterFreq1 = nicGetS1Freq(prAdapter,
		rRfChnlInfo.eBand,
		rRfChnlInfo.ucChannelNum,
		rRfChnlInfo.ucChnlBw);
	rRfChnlInfo.u4CenterFreq2 = 0;

	p2pFuncSetChannel(prAdapter, role_idx, &rRfChnlInfo);

	rlmBssUpdateChannelParams(prAdapter, prP2pBssInfo);

	DBGLOG(P2P, INFO,
		"[%d] SCO=%d H1=%d H2=%d H3=%d BW=%d S1=%d S2=%d CH=%d Band=%d TxN=%d RxN=%d\n",
		prP2pBssInfo->ucBssIndex,
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
	prChnlReqInfo->u4MaxInterval = P2P_AP_CHNL_HOLD_TIME_CSA_MS;
	prChnlReqInfo->eChnlReqType = CH_REQ_TYPE_JOIN;

	p2pRoleFsmStateTransitionImpl(prAdapter,
				      prP2pRoleFsmInfo,
				      prP2pBssInfo->ucBssIndex,
				      P2P_ROLE_STATE_SWITCH_CHANNEL);
}

void p2pFuncRemoveOneSap(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prSapBssInfo;
	struct BSS_INFO *prSapNextBssInfo;

	if (!prAdapter)
		return;

	prSapBssInfo = cnmGetSapBssInfo(prAdapter);
	if (!prSapBssInfo) {
		DBGLOG(P2P, TRACE, "SAP is not active\n");
		return;
	}

	prSapNextBssInfo = cnmGetOtherSapBssInfo(prAdapter,
		prSapBssInfo);
	if (prSapNextBssInfo) {
		if (p2pGetMode() == RUNNING_P2P_DEV_MODE ||
			p2pGetMode() == RUNNING_P2P_NO_GROUP_MODE ||
			p2pGetMode() == RUNNING_P2P_MODE ||
			p2pGetMode() == RUNNING_DUAL_P2P_MODE) {
			DBGLOG(P2P, WARN,
				"Remove sap (role%d)\n",
				prSapNextBssInfo->u4PrivateData);
			p2pFuncStopGO(prAdapter, prSapNextBssInfo);
			SET_NET_PWR_STATE_IDLE(prAdapter,
				prSapNextBssInfo->ucBssIndex);
		} else {
			/* Remove first one */
			DBGLOG(P2P, WARN,
				"Remove sap (role%d)\n",
				prSapBssInfo->u4PrivateData);
			p2pFuncStopGO(prAdapter, prSapBssInfo);
			SET_NET_PWR_STATE_IDLE(prAdapter,
				prSapBssInfo->ucBssIndex);
		}
	}
}

struct BSS_INFO *p2pGetAisBssByBand(
	struct ADAPTER *ad,
	enum ENUM_BAND eBand)
{
	uint8_t i, j;

	for (j = 0; j < KAL_AIS_NUM; j++) {
		struct AIS_FSM_INFO *fsm =
			aisFsmGetInstance(ad, j);

		if (!fsm)
			continue;

		for (i = 0; i < MLD_LINK_MAX; i++) {
			struct BSS_INFO *bss =
				aisGetLinkBssInfo(fsm, i);

			if (bss &&
				IS_BSS_AIS(bss) &&
				(kalGetMediaStateIndicated(
				ad->prGlueInfo,
				bss->ucBssIndex) ==
				MEDIA_STATE_CONNECTED) &&
				bss->eBand == eBand) {
				DBGLOG(P2P, TRACE,
					"bss%d;c%d, ais%d;link%d\n",
					bss->ucBssIndex,
					bss->ucPrimaryChannel,
					j, i);

				return bss;
			}
		}
	}

	return NULL;
}

struct BSS_INFO *p2pGetAisBssRfBand(
	struct ADAPTER *ad,
	enum ENUM_BAND eBand)
{
	if (eBand == BAND_2G4)
		return p2pGetAisBssByBand(ad, eBand);
	else if (eBand == BAND_5G) {
		if (p2pGetAisBssByBand(ad, BAND_5G))
			return p2pGetAisBssByBand(ad, BAND_5G);
#if (CFG_SUPPORT_WIFI_6G == 1)
		else
			return p2pGetAisBssByBand(ad, BAND_6G);
	} else if (eBand == BAND_6G) {
		if (p2pGetAisBssByBand(ad, BAND_5G))
			return p2pGetAisBssByBand(ad, BAND_5G);
		else
			return p2pGetAisBssByBand(ad, BAND_6G);
#endif
	}
	return NULL;
}

struct BSS_INFO *p2pGetAisConnectedBss(
	struct ADAPTER *ad)
{
	struct BSS_INFO *bss = NULL;
	struct BSS_INFO *bssRet = NULL;

	if (!ad)
		return NULL;

	bss = cnmGetSapBssInfo(ad);
	if (!bss) {
		DBGLOG(P2P, TRACE, "SAP is not active\n");
		/* return AIS 5G channel to update channel if it is DFS */
		return p2pGetAisBssByBand(ad, BAND_5G);
	}

	if (p2pGetMode() != RUNNING_P2P_AP_MODE)
		bssRet = p2pGetAisBssRfBand(ad,
			bss->eBand);
	else {
		struct BSS_INFO *bssNext =
			cnmGetOtherSapBssInfo(ad, bss);

		if (bssNext)
			bssRet = p2pGetAisBssRfBand(ad,
				bssNext->eBand);
	}

	if (bssRet)
		return bssRet;
	else
		return aisGetConnectedBssInfo(ad);
}

void p2pFuncCrossBandChannelSwitchCheck(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prP2pBssInfo,
	uint8_t *ucStaChannelNum,
	uint8_t *ucSapChannelNum,
	enum ENUM_BAND *eStaBand,
	enum ENUM_BAND *eSapBand,
#if (CFG_SUPPORT_CONNAC3X == 1)
	enum ENUM_MBMC_BN eStaHwBand,
	enum ENUM_MBMC_BN eSapHwBand,
#endif
	u_int8_t *fgDbDcModeEn)
{

#if CFG_SUPPORT_DBDC
	*fgDbDcModeEn = (prAdapter->rWifiVar.eDbdcMode !=
		ENUM_DBDC_MODE_DISABLED);
#if (CFG_SUPPORT_WIFI_6G == 1)	/* Go SCC for 5G+6G */
#if (CFG_SUPPORT_CONNAC3X == 1)
	if (((*eStaBand == BAND_5G && *eSapBand == BAND_6G) ||
		(*eStaBand == BAND_6G && *eSapBand == BAND_5G)) &&
		(eStaHwBand == eSapHwBand))
		*fgDbDcModeEn = FALSE;
#else
	if ((*eStaBand == BAND_5G && *eSapBand == BAND_6G) ||
		(*eStaBand == BAND_6G && *eSapBand == BAND_5G))
		*fgDbDcModeEn = FALSE;
#endif
#if CFG_CH_SELECT_ENHANCEMENT
	if ((prAdapter->rWifiVar.eDbdcMode !=
		ENUM_DBDC_MODE_DISABLED) &&
		(*eStaBand == BAND_6G) &&
		(*eSapBand != BAND_2G4) &&
		!IS_6G_PSC_CHANNEL(*ucStaChannelNum)) {
		*ucStaChannelNum = AP_DEFAULT_CHANNEL_2G;
		*eStaBand = BAND_2G4;
		*fgDbDcModeEn = FALSE;
	}
#endif
#endif

#if CFG_CH_SELECT_ENHANCEMENT
	if ((prAdapter->rWifiVar.eDbdcMode !=
		ENUM_DBDC_MODE_DISABLED) &&
		rlmDomainIsLegalDfsChannel(prAdapter,
		*eStaBand, *ucStaChannelNum) &&
		(*eSapBand != BAND_2G4) &&
		(prAdapter->rWifiVar.eDbdcMode !=
		ENUM_DBDC_MODE_DISABLED)) {
		*ucStaChannelNum = AP_DEFAULT_CHANNEL_2G;
		*eStaBand = BAND_2G4;
		*fgDbDcModeEn = FALSE;
	} else if ((prAdapter->rWifiVar.eDbdcMode !=
		ENUM_DBDC_MODE_DISABLED) &&
		(rlmDomainIsIndoorChannel(prAdapter,
			*eStaBand, *ucStaChannelNum) &&
		!rlmDomainIsStaSapIndoorConn(prAdapter)) &&
		(*eSapBand != BAND_2G4) &&
		(prAdapter->rWifiVar.eDbdcMode !=
		ENUM_DBDC_MODE_DISABLED)) {
		*ucStaChannelNum = AP_DEFAULT_CHANNEL_2G;
		*eStaBand = BAND_2G4;
		*fgDbDcModeEn = FALSE;
	} else if ((prAdapter->rWifiVar.eDbdcMode !=
		ENUM_DBDC_MODE_DISABLED) &&
		*eStaBand == BAND_2G4 &&
		*eSapBand == BAND_5G) {
		/* Choose one 5G channel */
		*ucStaChannelNum = AP_NONINDOOR_CHANNEL_5G;
		*eStaBand = BAND_5G;
		*fgDbDcModeEn = FALSE;
	}

#endif
#endif

}

void p2pCrossBandStaSccFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario)
{
	struct BSS_INFO *aliveBss[MAX_BSSID_NUM] = { 0 };
	uint8_t ucNumAliveBss;

	ucNumAliveBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveBss);

	if (ucNumAliveBss == 0 ||
		!aliveBss[0]) {
		*ucChSwithCandNum = 0;
		return;
	}
	*ucChSwithCandNum = 1;
	prSapSwitchCand[0].eRfBand = aliveBss[0]->eBand;
	prSapSwitchCand[0].ucBssIndex = aliveBss[0]->ucBssIndex;
	prSapSwitchCand[0].ucChLowerBound = aliveBss[0]->ucPrimaryChannel;
	prSapSwitchCand[0].ucChUpperBound = aliveBss[0]->ucPrimaryChannel;

}

void p2pRemoveDfsChFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario)
{
	uint8_t i, j = 0;
	struct P2P_CH_SWITCH_CANDIDATE rSapSwitchCand[BAND_NUM];

	if (prAdapter->rWifiVar.fgSapChannelSwitchPolicy ==
		P2P_CHANNEL_SWITCH_POLICY_SCC)
		return;

	for (i = 0; i < *ucChSwithCandNum; ++i) {
		if ((prSapSwitchCand[i].ucChLowerBound ==
			prSapSwitchCand[i].ucChUpperBound) &&
			rlmDomainIsLegalDfsChannel(prAdapter,
				prSapSwitchCand[i].eRfBand,
				prSapSwitchCand[i].ucChLowerBound)) {
			DBGLOG(P2P, INFO,
					"[SKIP] Dfs StaCH(%d), Band(%d)\n",
					prSapSwitchCand[i].ucChLowerBound,
					prSapSwitchCand[i].eRfBand);
		} else {
			rSapSwitchCand[j] = prSapSwitchCand[i];
			j++;
		}
	}
	if (j > 0) {
		memcpy(prSapSwitchCand, &rSapSwitchCand[0],
			j*sizeof(struct P2P_CH_SWITCH_CANDIDATE));
		*ucChSwithCandNum = j;
	} else
		*ucChSwithCandNum = 0;

	for (i = 0; i < *ucChSwithCandNum; ++i)
		DBGLOG(P2P, INFO,
			"[CSA]CSA Cand %d Band(%d, %d), CH(%d, %d)\n",
			i,
			prSapSwitchCand[i].eRfBand,
			prSapSwitchCand[i].eHwBand,
			prSapSwitchCand[i].ucChLowerBound,
			prSapSwitchCand[i].ucChUpperBound);
}

void p2pBtDesenseChFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario)
{
#if (CFG_SUPPORT_AVOID_DESENSE == 1)
	u_int8_t fgIsSapDesense = FALSE;
	uint8_t ucNumAliveNonSapBss;
	struct BSS_INFO *aliveNonSapBss[MAX_BSSID_NUM];

	ucNumAliveNonSapBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveNonSapBss);

	fgIsSapDesense =
		IS_CHANNEL_IN_DESENSE_RANGE(prAdapter,
				prP2pBssInfo->ucPrimaryChannel,
				prP2pBssInfo->eBand);

	if (fgIsSapDesense &&
		ucNumAliveNonSapBss == 0) {
		*ucChSwithCandNum = 1;
		prSapSwitchCand[0].eRfBand = BAND_2G4;
		prSapSwitchCand[0].ucChLowerBound =
			AP_DEFAULT_CHANNEL_2G;
		prSapSwitchCand[0].ucChUpperBound =
			AP_DEFAULT_CHANNEL_2G;
		prSapSwitchCand[0].ucBssIndex =
			prP2pBssInfo->ucBssIndex;
	} else
		*ucChSwithCandNum = 0;
#endif
}

void p2pDualApChFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario)
{
#if (CFG_SUPPORT_AVOID_DESENSE == 1)
	u_int8_t fgIsSapDesense = FALSE;
	uint8_t ucNumAliveNonSapBss;
	struct BSS_INFO *aliveNonSapBss[MAX_BSSID_NUM];

	ucNumAliveNonSapBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveNonSapBss);

	fgIsSapDesense =
		IS_CHANNEL_IN_DESENSE_RANGE(prAdapter,
				prP2pBssInfo->ucPrimaryChannel,
				prP2pBssInfo->eBand);

	if (fgIsSapDesense &&
		ucNumAliveNonSapBss == 0) {
		*ucChSwithCandNum = 1;
		prSapSwitchCand[0].eRfBand = BAND_2G4;
		prSapSwitchCand[0].ucChLowerBound =
			AP_DEFAULT_CHANNEL_2G;
		prSapSwitchCand[0].ucChUpperBound =
			AP_DEFAULT_CHANNEL_2G;
		prSapSwitchCand[0].ucBssIndex =
			prP2pBssInfo->ucBssIndex;
	} else
		*ucChSwithCandNum = 0;
#endif
}

void p2pSapSwitchCandidateRemove(
		uint8_t *ucChSwitchCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		uint8_t ucRemoveIdx)
{
	uint8_t j;

	if (ucRemoveIdx <= *ucChSwitchCandNum) {
		(*ucChSwitchCandNum)--;
		for (j = ucRemoveIdx; j < *ucChSwitchCandNum; j++)
			prSapSwitchCand[j] = prSapSwitchCand[j+1];
	}
	DBGLOG(P2P, INFO, "[CSA] cand remove: %d\n",
		ucRemoveIdx);
}

void p2pUserPrefChFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwitchCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario)
{
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t ucNumAliveNonSapBss;
	struct BSS_INFO *aliveNonSapBss[MAX_BSSID_NUM];

	ucNumAliveNonSapBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveNonSapBss);
#if (CFG_SUPPORT_CONNAC1X || CFG_SUPPORT_CONNAC2X)
	/*same rf band scc*/
	for (i = *ucChSwitchCandNum; i > 0 ; i--) {
		for (j = 0; j < ucNumAliveNonSapBss ; j++) {
			if (prSapSwitchCand[i-1].eRfBand ==
				prP2pBssInfo->eBand &&
				prSapSwitchCand[i-1].eRfBand ==
				aliveNonSapBss[j]->eBand) {
				*ucChSwitchCandNum = 1;
				prSapSwitchCand[0] =
					prSapSwitchCand[i-1];
				break;
			}
		}
	}
#else
	/*same hw index scc*/
	for (i = *ucChSwitchCandNum; i > 0 ; i--) {
		for (j = 0; j < ucNumAliveNonSapBss ; j++) {
			if (prSapSwitchCand[i-1].eHwBand ==
				prP2pBssInfo->eHwBandIdx &&
				prSapSwitchCand[i-1].eHwBand ==
				aliveNonSapBss[j]->eHwBandIdx &&
				prSapSwitchCand[i-1].eRfBand ==
				aliveNonSapBss[j]->eBand) {
				*ucChSwitchCandNum = 1;
				prSapSwitchCand[0] =
					prSapSwitchCand[i-1];
				break;
			}
		}
	}
#endif
}

void p2pMccAliveBssSyncFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario)
{
	struct BSS_INFO *aliveNonSapBss[MAX_BSSID_NUM];
	uint8_t ucNumAliveNonSapBss;
	struct BSS_INFO *aliveSapBss[MAX_BSSID_NUM] = { 0 };
	uint8_t ucNumAliveSapBss;
	uint8_t i, j;

	ucNumAliveNonSapBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveNonSapBss);
	ucNumAliveSapBss = cnmGetAliveSapBssInfo(
						prAdapter, aliveSapBss);

	if (ucNumAliveNonSapBss == 0 ||
		!aliveSapBss[0] ||
		ucNumAliveSapBss != 1) {
		DBGLOG(P2P, INFO, "[CSA] no non-sap alive bss\n");
		*ucChSwithCandNum = 0;
		return;
	}

	for (i = *ucChSwithCandNum; i > 0; i--) {
		for (j = 0; j < ucNumAliveNonSapBss; j++) {
			if (prSapSwitchCand[i-1].eRfBand ==
				aliveNonSapBss[j]->eBand &&
				prSapSwitchCand[i-1].ucChLowerBound <=
					aliveNonSapBss[j]->ucPrimaryChannel &&
				prSapSwitchCand[i-1].ucChUpperBound >=
					aliveNonSapBss[j]->ucPrimaryChannel) {

				prSapSwitchCand[i-1].ucChLowerBound =
					aliveNonSapBss[j]->ucPrimaryChannel;
				prSapSwitchCand[i-1].ucChUpperBound =
					aliveNonSapBss[j]->ucPrimaryChannel;
				prSapSwitchCand[i-1].ucBssIndex =
					aliveNonSapBss[j]->ucBssIndex;
			}
		}
	}
}

void p2pSetDefaultFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwithCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario)
{
	uint8_t j;
#if (CFG_SUPPORT_CONNAC1X || CFG_SUPPORT_CONNAC2X)
	for (j = 0; j < *ucChSwithCandNum; j++) {
		if ((prSapSwitchCand[j].ucChLowerBound <=
			prP2pBssInfo->ucPrimaryChannel &&
			prSapSwitchCand[j].ucChUpperBound >=
			prP2pBssInfo->ucPrimaryChannel) &&
			prSapSwitchCand[j].eRfBand ==
			prP2pBssInfo->eBand) {
			prSapSwitchCand[j].ucChLowerBound =
				prP2pBssInfo->ucPrimaryChannel;
			prSapSwitchCand[j].ucChUpperBound =
				prP2pBssInfo->ucPrimaryChannel;
			*ucChSwithCandNum = 1;
			prSapSwitchCand[0] =
				prSapSwitchCand[j];
		}
	}
#else
	for (j = 0; j < *ucChSwithCandNum; j++) {
		if ((prSapSwitchCand[j].ucChLowerBound <=
			prP2pBssInfo->ucPrimaryChannel &&
			prSapSwitchCand[j].ucChUpperBound >=
			prP2pBssInfo->ucPrimaryChannel) &&
			prSapSwitchCand[j].eRfBand ==
			prP2pBssInfo->eBand &&
			prSapSwitchCand[j].eHwBand ==
			prP2pBssInfo->eHwBandIdx) {
			prSapSwitchCand[j].ucChLowerBound =
				prP2pBssInfo->ucPrimaryChannel;
			prSapSwitchCand[j].ucChUpperBound =
				prP2pBssInfo->ucPrimaryChannel;
			*ucChSwithCandNum = 1;
			prSapSwitchCand[0] =
				prSapSwitchCand[j];
		}
	}
#endif
	for (j = 0; j < *ucChSwithCandNum; j++) {
		if ((prSapSwitchCand[j].ucChLowerBound !=
			prSapSwitchCand[j].ucChUpperBound) &&
			prSapSwitchCand[j].eRfBand == BAND_2G4) {
			prSapSwitchCand[j].ucChLowerBound =
				AP_DEFAULT_CHANNEL_2G;
			prSapSwitchCand[j].ucChUpperBound =
				AP_DEFAULT_CHANNEL_2G;
		} else if ((prSapSwitchCand[j].ucChLowerBound !=
			prSapSwitchCand[j].ucChUpperBound) &&
			prSapSwitchCand[j].eRfBand == BAND_5G) {
			prSapSwitchCand[j].ucChLowerBound =
				AP_DEFAULT_CHANNEL_5G;
			prSapSwitchCand[j].ucChUpperBound =
				AP_DEFAULT_CHANNEL_5G;
#if (CFG_SUPPORT_WIFI_6G == 1)
		} else if ((prSapSwitchCand[j].ucChLowerBound !=
			prSapSwitchCand[j].ucChUpperBound) &&
			prSapSwitchCand[j].eRfBand == BAND_6G) {
			prSapSwitchCand[j].ucChLowerBound =
				AP_DEFAULT_CHANNEL_6G_2;
			prSapSwitchCand[j].ucChUpperBound =
				AP_DEFAULT_CHANNEL_6G_2;
#endif
		}
	}
}

void p2pFuncFillChInfo(struct ADAPTER *prAdapter,
		       struct P2P_CH_BW_RANGE *prChBwRange,
		       uint8_t ucBitIdx)
{
	uint32_t *u4CenterFreq = &prChBwRange->u4CenterFreq[ucBitIdx];
	uint32_t *u4LowerBound = &prChBwRange->u4LowerBound[ucBitIdx];
	uint32_t *u4UpperBound = &prChBwRange->u4UpperBound[ucBitIdx];
	uint8_t ucCh = prChBwRange->ucCh;
	enum ENUM_BAND eBand = prChBwRange->eRfBand;
	uint32_t u4Freq = nicChannelNum2Freq(ucCh, eBand);

	if (ucBitIdx == MAX_BW_20MHZ) {
		*u4CenterFreq = u4Freq / 1000;
		*u4LowerBound = *u4CenterFreq - 10;
		*u4UpperBound = *u4CenterFreq + 10;
	} else if (ucBitIdx == MAX_BW_40MHZ) {
		if (eBand == BAND_5G) {
			if ((ucCh <= 144 && (ucCh % 8) == 0) ||
			    (ucCh <= 177 && (ucCh % 8) == 1))
				*u4CenterFreq = u4Freq / 1000 - 10;
			else if ((ucCh <= 144 && (ucCh % 8) == 4) ||
				 (ucCh <= 177 && (ucCh % 8) == 5))
				*u4CenterFreq = u4Freq / 1000 + 10;
		}
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (eBand == BAND_6G) {
			if (ucCh <= 229 && (ucCh % 8) == 5)
				*u4CenterFreq = u4Freq / 1000 - 10;
			else if (ucCh <= 229 && (ucCh % 8) == 1)
				*u4CenterFreq = u4Freq / 1000 + 10;
		}
#endif
		*u4LowerBound = *u4CenterFreq - 20;
		*u4UpperBound = *u4CenterFreq + 20;
	} else if (ucBitIdx == MAX_BW_80MHZ) {
		if (eBand == BAND_5G) {
			if (ucCh <= 48 && ucCh >= 36)
				*u4CenterFreq = 5210;
			else if (ucCh <= 64 && ucCh >= 52)
				*u4CenterFreq = 5290;
			else if (ucCh <= 80 && ucCh >= 68)
				*u4CenterFreq = 5370;
			else if (ucCh <= 96 && ucCh >= 84)
				*u4CenterFreq = 5450;
			else if (ucCh <= 112 && ucCh >= 100)
				*u4CenterFreq = 5530;
			else if (ucCh <= 128 && ucCh >= 116)
				*u4CenterFreq = 5610;
			else if (ucCh <= 144 && ucCh >= 132)
				*u4CenterFreq = 5690;
			else if (ucCh <= 161)
				*u4CenterFreq = 5775;
#if (CFG_SUPPORT_WIFI_6G == 1)
		} else if (eBand == BAND_6G) {
			if ((ucCh % 16) == 1)
				*u4CenterFreq = u4Freq / 1000 + 30;
			else if ((ucCh % 16) == 5)
				*u4CenterFreq = u4Freq / 1000 + 10;
			else if ((ucCh % 16) == 9)
				*u4CenterFreq = u4Freq / 1000 - 10;
			else if ((ucCh % 16) == 13)
				*u4CenterFreq = u4Freq / 1000 - 30;
#endif
		}

		*u4LowerBound = *u4CenterFreq - 40;
		*u4UpperBound = *u4CenterFreq + 40;
	} else if (ucBitIdx == MAX_BW_160MHZ) {
		if (eBand == BAND_5G) {
			if (ucCh <= 64 && ucCh >= 36)
				*u4CenterFreq = 5250;
			else if (ucCh <= 96 && ucCh >= 68)
				*u4CenterFreq = 5410;
			else if (ucCh <= 128 && ucCh >= 100)
				*u4CenterFreq = 5570;
#if (CFG_SUPPORT_WIFI_6G == 1)
		} else if (eBand == BAND_6G) {
			if (ucCh >= 1 && ucCh <= 29)
				*u4CenterFreq = 6025;
			else if (ucCh >= 33 && ucCh <= 61)
				*u4CenterFreq = 6185;
			else if (ucCh >= 65 && ucCh <= 93)
				*u4CenterFreq = 6345;
			else if (ucCh >= 97 && ucCh <= 125)
				*u4CenterFreq = 6505;
			else if (ucCh >= 129 && ucCh <= 157)
				*u4CenterFreq = 6665;
			else if (ucCh >= 161 && ucCh <= 189)
				*u4CenterFreq = 6825;
			else if (ucCh >= 193 && ucCh <= 221)
				*u4CenterFreq = 6985;
#endif
		}

		*u4LowerBound = *u4CenterFreq - 80;
		*u4UpperBound = *u4CenterFreq + 80;
	} else if (ucBitIdx == MAX_BW_320_1MHZ) {
		if (ucCh >= 1 && ucCh <= 61)
			*u4CenterFreq = 6105;
		else if (ucCh >= 65 && ucCh <= 125)
			*u4CenterFreq = 6425;
		else if (ucCh >= 129 && ucCh <= 189)
			*u4CenterFreq = 6745;

		*u4LowerBound = *u4CenterFreq - 160;
		*u4UpperBound = *u4CenterFreq + 160;
	} else if (ucBitIdx == MAX_BW_320_2MHZ) {
		if (ucCh >= 33 && ucCh <= 93)
			*u4CenterFreq = 6265;
		else if (ucCh >= 97 && ucCh <= 157)
			*u4CenterFreq = 6585;
		else if (ucCh >= 161 && ucCh <= 221)
			*u4CenterFreq = 6905;

		*u4LowerBound = *u4CenterFreq - 160;
		*u4UpperBound = *u4CenterFreq + 160;
	}
}

void p2pFuncGetChBwBitmap(struct ADAPTER *prAdapter,
			  struct P2P_CH_BW_RANGE *prChBwRange)
{
	uint8_t i;

	if (prChBwRange->eRfBand == BAND_2G4) {
		prChBwRange->ucBwBitmap = BIT(MAX_BW_20MHZ);
	} else if (prChBwRange->eRfBand == BAND_5G) {
		if (prChBwRange->ucCh == 165)
			prChBwRange->ucBwBitmap = BIT(MAX_BW_20MHZ);
		else
			prChBwRange->ucBwBitmap = BIT(MAX_BW_20MHZ) |
						  BIT(MAX_BW_40MHZ) |
						  BIT(MAX_BW_80MHZ);
		if (prChBwRange->fgIsDfsSupport == TRUE &&
			(prChBwRange->ucCh >= 36 && prChBwRange->ucCh <= 128)) {
			prChBwRange->ucBwBitmap |= BIT(MAX_BW_160MHZ);
		}
#if (CFG_SUPPORT_WIFI_6G == 1)
	} else if (prChBwRange->eRfBand == BAND_6G) {
		prChBwRange->ucBwBitmap = BIT(MAX_BW_20MHZ);

		if (prChBwRange->ucCh != 2 && prChBwRange->ucCh <= 229)
			prChBwRange->ucBwBitmap |= (BIT(MAX_BW_40MHZ));
		if (prChBwRange->ucCh != 2 && prChBwRange->ucCh <= 221)
			prChBwRange->ucBwBitmap |= (BIT(MAX_BW_80MHZ) |
						    BIT(MAX_BW_160MHZ));
		if (prChBwRange->ucCh != 2 && prChBwRange->ucCh <= 189)
			prChBwRange->ucBwBitmap |= BIT(MAX_BW_320_1MHZ);
		if (prChBwRange->ucCh <= 221 && prChBwRange->ucCh >= 33)
			prChBwRange->ucBwBitmap |= BIT(MAX_BW_320_2MHZ);
#endif
	}

	for (i = 0 ; i < MAX_BW_NUM ; i++) {
		if ((BIT(i) & prChBwRange->ucBwBitmap)) {
			p2pFuncFillChInfo(prAdapter, prChBwRange, i);

			DBGLOG(CCM, TRACE,
				"supported fc:%u, upper/ lower bound:%u, %u\n",
				prChBwRange->u4CenterFreq[i],
				prChBwRange->u4UpperBound[i],
				prChBwRange->u4LowerBound[i]);
		}
	}
	DBGLOG(CCM, INFO, "band:%d ch:%d bw bitmap:%d\n",
	       prChBwRange->eRfBand,
	       prChBwRange->ucCh,
	       prChBwRange->ucBwBitmap);
}

#if CFG_SUPPORT_CCM
void p2pAAChCandModify(struct ADAPTER *prAdapter,
		uint8_t *ucChSwitchCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct P2P_HW_BAND_UNIT *prHwBandUnit)
{
	uint8_t i, j, rChListLen;
	struct RF_CHANNEL_INFO rRfChnlInfo;
	struct P2P_CH_BW_RANGE rP2pChBwRange = { 0 };
	uint16_t arTargetBw[2];
	uint8_t rForbiddenListLen;
	struct CCM_AA_FOBIDEN_REGION_UNIT aprRegionOutput[2];
	struct RF_CHANNEL_INFO
		arChnlList[MAX_PER_BAND_CHN_NUM] = { { 0 } };
	u_int8_t fgIsBandMatch = TRUE;
	struct BSS_INFO *aliveSapBss[MAX_BSSID_NUM] = { 0 };
	uint8_t ucNumAliveSapBss;

	ucNumAliveSapBss = cnmGetAliveSapBssInfo(
						prAdapter, aliveSapBss);
	rP2pChBwRange.eRfBand =
		prHwBandUnit->eRfBand;
	rP2pChBwRange.ucCh =
		prHwBandUnit->ucCh;

	p2pFuncGetChBwBitmap(prAdapter, &rP2pChBwRange);

	rRfChnlInfo.eBand =
		prHwBandUnit->eRfBand;

	for (i = 0; i < MAX_BW_NUM; i++) {
		if (rP2pChBwRange.ucBwBitmap & BIT(i)) {
			rRfChnlInfo.u4CenterFreq1 =
				rP2pChBwRange.u4CenterFreq[i];
			rRfChnlInfo.ucChnlBw = i;
		}
	}

	if (
#if (CFG_SUPPORT_WIFI_6G == 1)
		rRfChnlInfo.eBand == BAND_6G ||
#endif
		rRfChnlInfo.eBand == BAND_5G) {
		arTargetBw[0] = 80;
		ccmAAForbiddenRegionCal(prAdapter,
			&rRfChnlInfo,
			&rForbiddenListLen,
			arTargetBw,
			aprRegionOutput);
		arChnlList[0].eBand = BAND_5G;
		arChnlList[0].u4CenterFreq2 =
			rRfChnlInfo.u4CenterFreq1;
		DBGLOG(P2P, INFO,
			"[CSA] boundary :(%u, %u, %u, %u, %u)\n",
			aprRegionOutput[0].u4BoundForward1,
			aprRegionOutput[0].u4BoundForward2,
			aprRegionOutput[0].u4BoundInverse1,
			aprRegionOutput[0].u4BoundInverse2,
			aprRegionOutput[0].u4BoundIsolate);

		rChListLen =
			p2pFuncSapFilteredChListGen(prAdapter,
				arChnlList,
				&rForbiddenListLen,
				aprRegionOutput,
				arTargetBw);
		for (i = 0; i < rChListLen ; i++) {

			DBGLOG(P2P, INFO,
				"[CSA] ch list: ch %u\n",
				arChnlList[i].ucChannelNum);

		}

		for (i = 0; i < *ucChSwitchCandNum ; i++) {
			if (prSapSwitchCand[i].eRfBand ==
				BAND_5G)
				fgIsBandMatch = FALSE;
			else
				continue;
			for (j = 0; j < rChListLen ; j++) {
				DBGLOG(P2P, INFO,
					"[CSA] AA filter :(%u, %u, %u, %u, %u)\n",
					prSapSwitchCand[i].ucChLowerBound,
					prSapSwitchCand[i].ucChUpperBound,
					aliveSapBss[0]->ucPrimaryChannel,
					arChnlList[j].ucChannelNum,
					aliveSapBss[0]->eBand);

				if (prSapSwitchCand[i].eRfBand ==
					prHwBandUnit->eRfBand &&
					prSapSwitchCand[i].ucChUpperBound >=
					prHwBandUnit->ucCh &&
					prSapSwitchCand[i].ucChLowerBound <=
					prHwBandUnit->ucCh) {
					prSapSwitchCand[i].ucChLowerBound =
						prHwBandUnit->ucCh;
					prSapSwitchCand[i].ucChUpperBound =
						prHwBandUnit->ucCh;
					fgIsBandMatch = TRUE;
				} else if (prSapSwitchCand[i].eRfBand ==
					aliveSapBss[0]->eBand &&
					prSapSwitchCand[i].ucChLowerBound <=
					aliveSapBss[0]->ucPrimaryChannel &&
					prSapSwitchCand[i].ucChUpperBound >=
					aliveSapBss[0]->ucPrimaryChannel &&
					arChnlList[j].ucChannelNum ==
					aliveSapBss[0]->ucPrimaryChannel) {
					prSapSwitchCand[i].ucChLowerBound =
						aliveSapBss[0]
							->ucPrimaryChannel;
					prSapSwitchCand[i].ucChUpperBound =
						aliveSapBss[0]
							->ucPrimaryChannel;
					fgIsBandMatch = TRUE;
				}
			}
			for (j = 0; j < rChListLen ; j++) {
				if (prSapSwitchCand[i].eRfBand ==
					BAND_5G &&
					prSapSwitchCand[i].ucChLowerBound <=
					AP_DEFAULT_CHANNEL_5GL &&
					prSapSwitchCand[i].ucChUpperBound >=
					AP_DEFAULT_CHANNEL_5GL &&
					arChnlList[j].ucChannelNum ==
					AP_DEFAULT_CHANNEL_5GL) {
					prSapSwitchCand[i].ucChLowerBound =
						AP_DEFAULT_CHANNEL_5GL;
					prSapSwitchCand[i].ucChUpperBound =
						AP_DEFAULT_CHANNEL_5GL;
					fgIsBandMatch = TRUE;
				} else if (prSapSwitchCand[i].eRfBand ==
					BAND_5G &&
					prSapSwitchCand[i].ucChLowerBound <=
					AP_DEFAULT_CHANNEL_5GH &&
					prSapSwitchCand[i].ucChUpperBound >=
					AP_DEFAULT_CHANNEL_5GH &&
					arChnlList[j].ucChannelNum ==
					AP_DEFAULT_CHANNEL_5GH) {
					prSapSwitchCand[i].ucChLowerBound =
						AP_DEFAULT_CHANNEL_5GH;
					prSapSwitchCand[i].ucChUpperBound =
						AP_DEFAULT_CHANNEL_5GH;
					fgIsBandMatch = TRUE;
				}
			}
			if (fgIsBandMatch == FALSE) {
				prSapSwitchCand[i].ucChLowerBound = 0;
				prSapSwitchCand[i].ucChUpperBound = 0;
			}
		}
	}
	fgIsBandMatch = TRUE;
	if (rRfChnlInfo.eBand == BAND_5G) {
		arTargetBw[0] = 80;
		arTargetBw[1] = 320;

		ccmAAForbiddenRegionCal(prAdapter,
			&rRfChnlInfo,
			&rForbiddenListLen,
			arTargetBw,
			aprRegionOutput);
		for (i = 0; i < rForbiddenListLen ; i++) {

			DBGLOG(P2P, INFO,
				"[CSA] boundary :(%u, %u, %u, %u, %u)\n",
				aprRegionOutput[i].u4BoundForward1,
				aprRegionOutput[i].u4BoundForward2,
				aprRegionOutput[i].u4BoundInverse1,
				aprRegionOutput[i].u4BoundInverse2,
				aprRegionOutput[i].u4BoundIsolate);

		}
#if (CFG_SUPPORT_WIFI_6G == 1)
		arChnlList[0].eBand = BAND_6G;
		arChnlList[0].u4CenterFreq2 =
			rRfChnlInfo.u4CenterFreq1;

		rChListLen =
			p2pFuncSapFilteredChListGen(prAdapter,
				arChnlList,
				&rForbiddenListLen,
				aprRegionOutput,
				arTargetBw);
		for (i = 0; i < rChListLen ; i++) {

			DBGLOG(P2P, INFO,
				"[CSA] ch list: ch %u\n",
				arChnlList[i].ucChannelNum);

		}

		for (i = 0; i < *ucChSwitchCandNum ; i++) {
			if (prSapSwitchCand[i].eRfBand ==
				BAND_6G)
				fgIsBandMatch = FALSE;
			else
				continue;
			for (j = 0; j < rChListLen ; j++) {
				DBGLOG(P2P, INFO,
					"[CSA] AA filter :(%u, %u, %u, %u, %u)\n",
					prSapSwitchCand[i].ucChLowerBound,
					prSapSwitchCand[i].ucChUpperBound,
					aliveSapBss[0]->ucPrimaryChannel,
					arChnlList[j].ucChannelNum,
					aliveSapBss[0]->eBand);

				if (prSapSwitchCand[i].eRfBand ==
					prHwBandUnit->eRfBand &&
					prSapSwitchCand[i].ucChUpperBound >=
					prHwBandUnit->ucCh &&
					prSapSwitchCand[i].ucChLowerBound <=
					prHwBandUnit->ucCh) {
					prSapSwitchCand[i].ucChLowerBound =
						prHwBandUnit->ucCh;
					prSapSwitchCand[i].ucChUpperBound =
						prHwBandUnit->ucCh;
					fgIsBandMatch = TRUE;
				} else if (prSapSwitchCand[i].eRfBand ==
					aliveSapBss[0]->eBand &&
					prSapSwitchCand[i].ucChLowerBound <=
					aliveSapBss[0]->ucPrimaryChannel &&
					prSapSwitchCand[i].ucChUpperBound >=
					aliveSapBss[0]->ucPrimaryChannel &&
					arChnlList[j].ucChannelNum ==
					aliveSapBss[0]->ucPrimaryChannel) {
					prSapSwitchCand[i].ucChLowerBound =
						aliveSapBss[0]
							->ucPrimaryChannel;
					prSapSwitchCand[i].ucChUpperBound =
						aliveSapBss[0]
							->ucPrimaryChannel;
					fgIsBandMatch = TRUE;
				}
			}

			for (j = 0; j < rChListLen ; j++) {
				if (prSapSwitchCand[i].eRfBand ==
					BAND_6G &&
					prSapSwitchCand[i].ucChLowerBound <=
					AP_DEFAULT_CHANNEL_6G_1 &&
					prSapSwitchCand[i].ucChUpperBound >=
					AP_DEFAULT_CHANNEL_6G_1 &&
					arChnlList[j].ucChannelNum ==
					AP_DEFAULT_CHANNEL_6G_1) {
					prSapSwitchCand[i].ucChLowerBound =
						AP_DEFAULT_CHANNEL_6G_1;
					prSapSwitchCand[i].ucChUpperBound =
						AP_DEFAULT_CHANNEL_6G_1;
					fgIsBandMatch = TRUE;
				} else if (prSapSwitchCand[i].eRfBand ==
					BAND_6G &&
					prSapSwitchCand[i].ucChLowerBound <=
					AP_DEFAULT_CHANNEL_6G_2 &&
					prSapSwitchCand[i].ucChUpperBound >=
					AP_DEFAULT_CHANNEL_6G_2 &&
					arChnlList[j].ucChannelNum ==
					AP_DEFAULT_CHANNEL_6G_2) {
					prSapSwitchCand[i].ucChLowerBound =
						AP_DEFAULT_CHANNEL_6G_2;
					prSapSwitchCand[i].ucChUpperBound =
						AP_DEFAULT_CHANNEL_6G_2;
					fgIsBandMatch = TRUE;
				}
			}

			if (fgIsBandMatch == FALSE) {
				prSapSwitchCand[i].ucChLowerBound = 0;
				prSapSwitchCand[i].ucChUpperBound = 0;
			}
		}
#endif
	}

	for (i = *ucChSwitchCandNum; i > 0; i--) {
		if (prSapSwitchCand[i-1]
				.ucChLowerBound == 0 &&
			prSapSwitchCand[i-1]
				.ucChUpperBound == 0)
			p2pSapSwitchCandidateRemove(
				ucChSwitchCandNum,
				prSapSwitchCand, i-1);
	}

}
#endif /* CFG_SUPPORT_CCM */

void p2pForbiddenChRemove(struct ADAPTER *prAdapter,
		uint8_t *ucChSwitchCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct P2P_HW_BAND_GROUP *prHwBandGroup)
{
	struct BSS_INFO *aliveNonSapBss[MAX_BSSID_NUM];
	uint8_t ucNumAliveNonSapBss;
	struct BSS_INFO *aliveSapBss[MAX_BSSID_NUM] = { 0 };
	uint8_t ucNumAliveSapBss;
	uint8_t i;

	ucNumAliveNonSapBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveNonSapBss);
	ucNumAliveSapBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveSapBss);

	if (ENUM_BAND_NUM == (ENUM_BAND_1 + 1) &&
		ucNumAliveNonSapBss == 1 &&
		aliveNonSapBss[0]->eHwBandIdx ==
		ENUM_BAND_1) {
		for (i = *ucChSwitchCandNum; i > 0; i--) {
			if (prSapSwitchCand[i-1].eHwBand ==
				ENUM_BAND_NUM ||
				(prSapSwitchCand[i-1].eHwBand ==
				aliveNonSapBss[0]->eHwBandIdx &&
				prSapSwitchCand[i-1].eRfBand !=
				aliveNonSapBss[0]->eBand)) {
				p2pSapSwitchCandidateRemove(
					ucChSwitchCandNum,
					prSapSwitchCand, i-1);
			}
		}
#if (CFG_SUPPORT_CONNAC3X == 1 && CFG_SUPPORT_CCM)
	} else if (ENUM_BAND_NUM == (ENUM_BAND_2 + 1)) {

		if (prHwBandGroup[ENUM_BAND_1].ucUnitNum != 0 &&
			prHwBandGroup[ENUM_BAND_2].ucUnitNum == 0) {
			p2pAAChCandModify(prAdapter,
				ucChSwitchCandNum,
				prSapSwitchCand,
				prHwBandGroup[ENUM_BAND_1]
				.arP2pHwBandUnit);
		}
		if (prHwBandGroup[ENUM_BAND_1].ucUnitNum == 0 &&
			prHwBandGroup[ENUM_BAND_2].ucUnitNum != 0) {
			p2pAAChCandModify(prAdapter,
				ucChSwitchCandNum,
				prSapSwitchCand,
				prHwBandGroup[ENUM_BAND_2]
				.arP2pHwBandUnit);

		}
#endif
	}
}

void p2pHwBandMccRemove(struct ADAPTER *prAdapter,
		uint8_t *ucChSwitchCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand)
{
	struct P2P_HW_BAND_GROUP rHwBandGroup[ENUM_BAND_NUM] = { 0 };
	struct BSS_INFO *aliveNonSapBss[MAX_BSSID_NUM];
	uint8_t ucNumAliveNonSapBss;

	uint8_t j, i, k = 0;

	ucNumAliveNonSapBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveNonSapBss);

	for (i = 0; i < ENUM_BAND_NUM; i++) {
		rHwBandGroup[i].eHwBand =
			(enum ENUM_MBMC_BN) i;
	}

	for (i = 0; i < ENUM_BAND_NUM; i++) {
		k = 0;
		for (j = 0; j < ucNumAliveNonSapBss; j++) {
			if (aliveNonSapBss[j]->eHwBandIdx ==
				rHwBandGroup[i].eHwBand) {
				rHwBandGroup[i].arP2pHwBandUnit[k]
					.eRfBand = aliveNonSapBss[j]->eBand;
				rHwBandGroup[i].arP2pHwBandUnit[k]
					.ucCh =
					aliveNonSapBss[j]->ucPrimaryChannel;
				rHwBandGroup[i].arP2pHwBandUnit[k]
					.ucBssIndex =
						aliveNonSapBss[j]->ucBssIndex;
				rHwBandGroup[i].ucUnitNum++;
				if (rHwBandGroup[i].arP2pHwBandUnit[k].ucCh !=
					rHwBandGroup[i]
						.arP2pHwBandUnit[0].ucCh) {
					rHwBandGroup[i].fgIsMcc = TRUE;
					DBGLOG(P2P, INFO,
						"[CSA] mcc group: hw band %d\n",
						rHwBandGroup[i].eHwBand);
				}
				k++;
			}
		}
		DBGLOG(P2P, INFO,
			"[CSA] mcc group: hw band %d and num %d\n",
			i,
			rHwBandGroup[i].ucUnitNum);
	}

	for (i = *ucChSwitchCandNum; i > 0; i--) {
		for (j = 0; j < ENUM_BAND_NUM; j++) {
			if ((prSapSwitchCand[i-1].eHwBand ==
				rHwBandGroup[j].eHwBand) &&
				(rHwBandGroup[j].fgIsMcc == TRUE)) {
				p2pSapSwitchCandidateRemove(
					ucChSwitchCandNum,
					prSapSwitchCand,
					i-1);
			}
		}
	}
#if CFG_SUPPORT_CCM
	p2pForbiddenChRemove(prAdapter,
			ucChSwitchCandNum,
			prSapSwitchCand,
			rHwBandGroup);
#endif

}

void p2pDualABandFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwitchCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario)
{
	struct BSS_INFO *aliveNonSapBss[MAX_BSSID_NUM];
	uint8_t ucNumAliveNonSapBss;
	struct BSS_INFO *aliveSapBss[MAX_BSSID_NUM] = { 0 };
	uint8_t ucNumAliveSapBss;

	ucNumAliveNonSapBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveNonSapBss);
	ucNumAliveSapBss = cnmGetAliveSapBssInfo(
						prAdapter, aliveSapBss);

	if (ucNumAliveNonSapBss == 0 ||
		!aliveSapBss[0] ||
		ucNumAliveSapBss != 1) {
		DBGLOG(P2P, INFO, "[CSA] no non-sap alive bss\n");
		*ucChSwitchCandNum = 0;
		return;
	}

	p2pHwBandMccRemove(prAdapter,
			ucChSwitchCandNum,
			prSapSwitchCand);

}

void p2pRfBandCheckFilter(struct ADAPTER *prAdapter,
		uint8_t *ucChSwitchCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario)
{
	struct BSS_INFO *aliveNonSapBss[MAX_BSSID_NUM];
	uint8_t ucNumAliveNonSapBss;
	struct BSS_INFO *aliveSapBss[MAX_BSSID_NUM] = { 0 };
	uint8_t ucNumAliveSapBss;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t i;
#endif
	ucNumAliveNonSapBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveNonSapBss);
	ucNumAliveSapBss = cnmGetAliveSapBssInfo(
						prAdapter, aliveSapBss);

	if (ucNumAliveSapBss != 1) {
		DBGLOG(P2P, INFO, "[CSA] scenario error\n");
		return;
	}
	DBGLOG(P2P, INFO, "[CSA] is sap wpa3: %u and %u\n",
			aliveSapBss[0]->u4RsnSelectedAKMSuite &
			RSN_AKM_SUITE_SAE,
			(aliveSapBss[0]->u4RsnSelectedAKMSuite &
			RSN_AKM_SUITE_SAE) == RSN_AKM_SUITE_SAE);
#if (CFG_SUPPORT_WIFI_6G == 1)
	for (i = *ucChSwitchCandNum; i > 0; i--) {
		if (aliveSapBss[0]->eBand == BAND_5G &&
			prSapSwitchCand[i-1].eRfBand == BAND_6G &&
			(aliveSapBss[0]->u4RsnSelectedAKMSuite &
			RSN_AKM_SUITE_SAE) != RSN_AKM_SUITE_SAE) {
			p2pSapSwitchCandidateRemove(
				ucChSwitchCandNum,
				prSapSwitchCand,
				i-1);
		}
	}
#endif
}

void p2pFuncSapAvailibilityCheck(
		struct ADAPTER *prAdapter,
		struct BSS_INFO *prCsaBss)
{
	struct BSS_INFO *aliveBss[MAX_BSSID_NUM] = { 0 };
	uint8_t ucNumAliveNonSapBss;
#if CFG_SUPPORT_SAP_DFS_CHANNEL
	uint8_t i = 0;
#endif

	ucNumAliveNonSapBss =
		cnmGetAliveNonSapBssInfo(prAdapter, aliveBss);

	if (ucNumAliveNonSapBss != 0 &&
		prAdapter->rWifiVar.fgSapConcurrencyPolicy ==
			P2P_CONCURRENCY_POLICY_REMOVE) {

		/* Check other ap */
		p2pFuncRemoveOneSap(prAdapter);
	}
#if CFG_SUPPORT_SAP_DFS_CHANNEL
	for (i = 0; i < ucNumAliveNonSapBss; ++i) {
		/* restore DFS channels table */
		wlanDfsChannelsReqAdd(prAdapter,
			DFS_CHANNEL_CTRL_SOURCE_SAP,
			aliveBss[i]->ucPrimaryChannel,
			aliveBss[i]->ucVhtChannelWidth,
			aliveBss[i]->eBssSCO,
			nicChannelNum2Freq(
				aliveBss[i]->ucVhtChannelFrequencyS1,
				aliveBss[i]->eBand) / 1000,
			aliveBss[i]->eBand);
	}
#endif

	if (prAdapter->rWifiVar.fgSapConcurrencyPolicy ==
		P2P_CONCURRENCY_POLICY_KEEP &&
		ucNumAliveNonSapBss == 1) {
		struct BSS_INFO *prSapBssInfo =
			cnmGetOtherSapBssInfo(prAdapter,
			prCsaBss);
		if (prSapBssInfo &&
			(prCsaBss->eBand != aliveBss[0]->eBand))
			prCsaBss = prSapBssInfo;
	}
}


void p2pFuncSapFilterTrace(
		struct ADAPTER *prAdapter,
		uint8_t *ucChSwitchCandNum,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		uint8_t ucFilterIdx)
{
	uint8_t i;

	DBGLOG(P2P, TRACE,
			"[CSA]CSA filter index: %u\n",
			ucFilterIdx);

	for (i = 0; i < *ucChSwitchCandNum; ++i)
		DBGLOG(P2P, TRACE,
		"[CSA]CSA Cand %d Band(%d, %d), CH(%d, %d)\n",
		i,
		prSapSwitchCand[i].eRfBand,
		prSapSwitchCand[i].eHwBand,
		prSapSwitchCand[i].ucChLowerBound,
		prSapSwitchCand[i].ucChUpperBound);

}

void p2pFuncSapSwitchChCheck(
		struct ADAPTER *prAdapter,
		struct P2P_CH_SWITCH_INTERFACE *prSwitchInterface,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE *eFilterScnario)
{
	uint32_t u4FilterArraySize = 0;
	uint32_t u4Idx = 0;

	if (*eFilterScnario == P2P_STA_SCC_ONLY_SCENARIO) {
		u4FilterArraySize =
			sizeof(p2pSccOnlyChCandFilterTable) /
				sizeof(struct P2P_CH_CANDIDATE_FILETER_ENTRY);

		for (u4Idx = 0; u4Idx < u4FilterArraySize; u4Idx++) {
			if (p2pSccOnlyChCandFilterTable[u4Idx].pfnChCandFilter)
				p2pSccOnlyChCandFilterTable[u4Idx]
					.pfnChCandFilter(prAdapter,
					prSwitchInterface->ucInterfaceLen,
					prSwitchInterface->prP2pChInterface,
					prP2pBssInfo,
					*eFilterScnario);
			if (*prSwitchInterface->ucInterfaceLen == 1)
				break;
			else if (*prSwitchInterface->ucInterfaceLen == 0)
				return;
		}
	} else if (*eFilterScnario ==
		P2P_MCC_SINGLE_SAP_SCENARIO) {

		u4FilterArraySize =
			sizeof(p2pSingleApMccFilterTable) /
			sizeof(struct P2P_CH_CANDIDATE_FILETER_ENTRY);

		for (u4Idx = 0; u4Idx < u4FilterArraySize; u4Idx++) {
			if (p2pSingleApMccFilterTable[u4Idx].pfnChCandFilter)
				p2pSingleApMccFilterTable[u4Idx]
					.pfnChCandFilter(prAdapter,
					prSwitchInterface->ucInterfaceLen,
					prSwitchInterface->prP2pChInterface,
					prP2pBssInfo,
					*eFilterScnario);
			p2pFuncSapFilterTrace(prAdapter,
					prSwitchInterface->ucInterfaceLen,
					prSwitchInterface->prP2pChInterface,
					u4Idx);

			if (*prSwitchInterface
					->ucInterfaceLen == 1 &&
				prSwitchInterface
					->prP2pChInterface[0].ucChLowerBound ==
				prSwitchInterface
					->prP2pChInterface[0].ucChUpperBound) {
				if (prSwitchInterface
					->prP2pChInterface[0].ucChLowerBound ==
					prP2pBssInfo->ucPrimaryChannel &&
					prSwitchInterface
						->prP2pChInterface[0].eRfBand ==
					prP2pBssInfo->eBand)
					(*prSwitchInterface->ucInterfaceLen)--;
				break;
			} else if (*prSwitchInterface->ucInterfaceLen == 0)
				return;
		}
	} else if (*eFilterScnario ==
		P2P_BT_COEX_SCENARIO) {
		u4FilterArraySize =
			sizeof(p2pBtCoexChCandFilterTable) /
			sizeof(struct P2P_CH_CANDIDATE_FILETER_ENTRY);

		for (u4Idx = 0; u4Idx < u4FilterArraySize; u4Idx++) {
			if (p2pBtCoexChCandFilterTable[u4Idx].pfnChCandFilter)
				p2pBtCoexChCandFilterTable[u4Idx]
					.pfnChCandFilter(prAdapter,
					prSwitchInterface->ucInterfaceLen,
					prSwitchInterface->prP2pChInterface,
					prP2pBssInfo,
					*eFilterScnario);
			if (*prSwitchInterface->ucInterfaceLen == 1)
				break;
			else if (*prSwitchInterface->ucInterfaceLen == 0)
				return;
		}
	} else if (*eFilterScnario ==
		P2P_MCC_DUAL_SAP_SCENARIO) {
		*prSwitchInterface->ucInterfaceLen = 0;
	} else
		return;

}

#if CFG_SUPPORT_CCM
uint8_t p2pFuncSapFilteredChListGen(
		struct ADAPTER *prAdapter,
		struct RF_CHANNEL_INFO *prChnlList,
		uint8_t *prForbiddenListLen,
		struct CCM_AA_FOBIDEN_REGION_UNIT *prRegionOutput,
		uint16_t *prTargetBw)
{
	uint8_t ucChnlNum, j, k;
	uint8_t i = 0;
	struct P2P_CH_BW_RANGE rP2pChBwRange = { 0 };
	uint32_t u4CenterFreqInput;

	u4CenterFreqInput =
		prChnlList[0].u4CenterFreq2;

	if (prChnlList[0].eBand == BAND_5G) {
		rlmDomainGetChnlList(prAdapter,
			BAND_5G, TRUE, MAX_5G_BAND_CHN_NUM,
			&ucChnlNum, prChnlList);
		rP2pChBwRange.eRfBand = BAND_5G;
		for (k = 0; k < ucChnlNum; k++) {
			rP2pChBwRange.ucCh = prChnlList[k].ucChannelNum;
			p2pFuncGetChBwBitmap(prAdapter, &rP2pChBwRange);
			for (j = 0; j < MAX_BW_NUM; j++) {
				if (rP2pChBwRange.ucBwBitmap & BIT(j)) {
					prChnlList[k].u4CenterFreq1 =
						rP2pChBwRange.u4CenterFreq[j];
				}
			}
			DBGLOG(P2P, INFO,
				"ch:%d, center freq:%u\n",
				rP2pChBwRange.ucCh,
				prChnlList[k].u4CenterFreq1);

			if (prChnlList[k].u4CenterFreq1 <
				u4CenterFreqInput &&
				(prRegionOutput[0].u4BoundForward1 <=
				prChnlList[k].u4CenterFreq1 ||
				prRegionOutput[0].u4BoundForward2 >=
				prChnlList[k].u4CenterFreq1) &&
				(prRegionOutput[0].u4BoundInverse1 <=
				prChnlList[k].u4CenterFreq1 ||
				prRegionOutput[0].u4BoundInverse2 >=
				prChnlList[k].u4CenterFreq1) &&
				prRegionOutput[0].u4BoundIsolate >
				prChnlList[k].u4CenterFreq1) {
				prChnlList[i] = prChnlList[k];
				i++;
			} else if (prChnlList[k].u4CenterFreq1 >
				u4CenterFreqInput &&
				(prRegionOutput[0].u4BoundForward1 <=
				prChnlList[k].u4CenterFreq1 ||
				prRegionOutput[0].u4BoundForward2 >=
				prChnlList[k].u4CenterFreq1) &&
				(prRegionOutput[0].u4BoundInverse1 <=
				prChnlList[k].u4CenterFreq1 ||
				prRegionOutput[0].u4BoundInverse2 >=
				prChnlList[k].u4CenterFreq1) &&
				prRegionOutput[0].u4BoundIsolate <
				prChnlList[k].u4CenterFreq1) {
				prChnlList[i] = prChnlList[k];
				i++;
			}
		}
#if (CFG_SUPPORT_WIFI_6G == 1)
	} else if (prChnlList[0].eBand == BAND_6G) {
		rlmDomainGetChnlList(prAdapter,
			BAND_6G, TRUE, MAX_6G_BAND_CHN_NUM,
			&ucChnlNum, prChnlList);
		rP2pChBwRange.eRfBand = BAND_6G;
		for (k = 0; k < ucChnlNum; k++) {
			rP2pChBwRange.ucCh = prChnlList[k].ucChannelNum;
			p2pFuncGetChBwBitmap(prAdapter, &rP2pChBwRange);
			for (j = 0; j < MAX_BW_NUM; j++) {
				if (rP2pChBwRange.ucBwBitmap & BIT(j)) {
					prChnlList[k].u4CenterFreq1 =
						rP2pChBwRange.u4CenterFreq[j];
				}
			}
			DBGLOG(P2P, INFO,
				"ch:%d, center freq:%u\n",
				rP2pChBwRange.ucCh,
				prChnlList[k].u4CenterFreq1);
			if (prChnlList[k].u4CenterFreq1 <
				u4CenterFreqInput &&
				(prRegionOutput[1].u4BoundForward1 <=
				prChnlList[k].u4CenterFreq1 ||
				prRegionOutput[1].u4BoundForward2 >=
				prChnlList[k].u4CenterFreq1) &&
				(prRegionOutput[1].u4BoundInverse1 <=
				prChnlList[k].u4CenterFreq1 ||
				prRegionOutput[1].u4BoundInverse2 >=
				prChnlList[k].u4CenterFreq1) &&
				prRegionOutput[1].u4BoundIsolate >
				prChnlList[k].u4CenterFreq1) {
				prChnlList[i] = prChnlList[k];
				i++;
			} else if (prChnlList[k].u4CenterFreq1 >
				u4CenterFreqInput &&
				(prRegionOutput[1].u4BoundForward1 <=
				prChnlList[k].u4CenterFreq1 ||
				prRegionOutput[1].u4BoundForward2 >=
				prChnlList[k].u4CenterFreq1) &&
				(prRegionOutput[1].u4BoundInverse1 <=
				prChnlList[k].u4CenterFreq1 ||
				prRegionOutput[1].u4BoundInverse2 >=
				prChnlList[k].u4CenterFreq1) &&
				prRegionOutput[1].u4BoundIsolate <
				prChnlList[k].u4CenterFreq1) {
				prChnlList[i] = prChnlList[k];
				i++;
			}
		}
#endif
	}
	return i;
}
#endif /* CFG_SUPPORT_CCM */

uint8_t p2pFuncSapSwichCandidatGen(
		struct ADAPTER *prAdapter,
		struct P2P_CH_SWITCH_CANDIDATE *prSapSwitchCand,
		struct BSS_INFO *prP2pBssInfo,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE *eFilterScnario)
{
	struct RF_CHANNEL_INFO arChnlList[MAX_PER_BAND_CHN_NUM] = { { 0 } };
	uint8_t ucChnlNum, j, k, i = 0;
	struct BSS_INFO *prBssInfo;

	if (*eFilterScnario ==
		P2P_BT_COEX_SCENARIO)
		return 0;

	rlmDomainGetChnlList(prAdapter, BAND_2G4, TRUE, MAX_2G_BAND_CHN_NUM,
		&ucChnlNum, arChnlList);
	if (ucChnlNum != 0) {
		prSapSwitchCand[i].eRfBand = arChnlList[0].eBand;
		prSapSwitchCand[i].ucChLowerBound = arChnlList[0].ucChannelNum;
		prSapSwitchCand[i].ucChUpperBound =
				arChnlList[ucChnlNum-1].ucChannelNum;
		prSapSwitchCand[i].ucBssIndex =
			prP2pBssInfo->ucBssIndex;
		prSapSwitchCand[i].eHwBand =
			ENUM_BAND_0;

		i++;
	}
	rlmDomainGetChnlList(prAdapter, BAND_5G, TRUE, MAX_5G_BAND_CHN_NUM,
		&ucChnlNum, arChnlList);
	if (ucChnlNum != 0 &&
		ENUM_BAND_NUM == (ENUM_BAND_1+1)) {

		prSapSwitchCand[i].eRfBand = arChnlList[0].eBand;
		prSapSwitchCand[i].ucChLowerBound = arChnlList[0].ucChannelNum;
		prSapSwitchCand[i].ucChUpperBound =
				arChnlList[ucChnlNum-1].ucChannelNum;
		prSapSwitchCand[i].ucBssIndex =
			prP2pBssInfo->ucBssIndex;
		prSapSwitchCand[i].eHwBand =
			ENUM_BAND_NUM;
		for (j = 0; j < MAX_BSSID_NUM; j++) {
			prBssInfo =
				GET_BSS_INFO_BY_INDEX(prAdapter, j);
			if (IS_BSS_ALIVE(prAdapter, prBssInfo) &&
				prSapSwitchCand[i].eRfBand ==
				prBssInfo->eBand &&
				prBssInfo->ucPrimaryChannel <
				prSapSwitchCand[i].ucChLowerBound)
				prSapSwitchCand[i].ucChLowerBound =
					prBssInfo->ucPrimaryChannel;
		}
		i++;
#if (CFG_SUPPORT_CONNAC3X == 1)
#if (CONFIG_BAND_NUM == 3)
	} else if (ucChnlNum != 0 &&
		ENUM_BAND_NUM == (ENUM_BAND_2+1)) {
		prSapSwitchCand[i].eRfBand = arChnlList[0].eBand;
		prSapSwitchCand[i].ucChLowerBound = arChnlList[0].ucChannelNum;
		prSapSwitchCand[i].ucChUpperBound =
			UNII2A_UPPER_BOUND;

		/* subband boundary definition is decribed with freq/ ch edge */
		/* may be larger than primary channel */
		prSapSwitchCand[i].ucBssIndex =
			prP2pBssInfo->ucBssIndex;
		prSapSwitchCand[i].eHwBand =
			ENUM_BAND_NUM;

		i++;
		if (arChnlList[ucChnlNum-1].ucChannelNum >=
			UNII2C_LOWER_BOUND) {
			prSapSwitchCand[i].eRfBand = arChnlList[0].eBand;
			prSapSwitchCand[i].ucChLowerBound =
				UNII2C_LOWER_BOUND;
			prSapSwitchCand[i].ucChUpperBound =
				arChnlList[ucChnlNum-1].ucChannelNum;
			prSapSwitchCand[i].ucBssIndex =
				prP2pBssInfo->ucBssIndex;
			prSapSwitchCand[i].eHwBand =
				ENUM_BAND_NUM;

			i++;
		}
#endif
#endif
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	rlmDomainGetChnlList(prAdapter, BAND_6G, TRUE, MAX_6G_BAND_CHN_NUM,
		&ucChnlNum, arChnlList);
	if (ucChnlNum != 0) {

		prSapSwitchCand[i].eRfBand = arChnlList[0].eBand;
		if (arChnlList[ucChnlNum-1].ucChannelNum == 2 &&
			ucChnlNum >= 2) {
			prSapSwitchCand[i].ucChUpperBound =
				arChnlList[ucChnlNum-2].ucChannelNum;
			prSapSwitchCand[i].ucChLowerBound =
				arChnlList[0].ucChannelNum;
			prSapSwitchCand[i].ucBssIndex =
				prP2pBssInfo->ucBssIndex;
			prSapSwitchCand[i].eHwBand =
				ENUM_BAND_NUM;

			i++;
			prSapSwitchCand[i].eRfBand = arChnlList[0].eBand;
			prSapSwitchCand[i].ucChLowerBound =
				arChnlList[ucChnlNum-1].ucChannelNum;
			prSapSwitchCand[i].ucChUpperBound =
				arChnlList[ucChnlNum-1].ucChannelNum;
			prSapSwitchCand[i].ucBssIndex =
				prP2pBssInfo->ucBssIndex;
			prSapSwitchCand[i].eHwBand =
				ENUM_BAND_NUM;

			i++;
		} else {
			prSapSwitchCand[i].eRfBand = arChnlList[0].eBand;
			prSapSwitchCand[i].ucChLowerBound =
				arChnlList[0].ucChannelNum;
			prSapSwitchCand[i].ucChUpperBound =
				arChnlList[ucChnlNum-1].ucChannelNum;
			prSapSwitchCand[i].ucBssIndex =
				prP2pBssInfo->ucBssIndex;
			prSapSwitchCand[i].eHwBand =
				ENUM_BAND_NUM;

			i++;
		}
	}
#endif
	for (k = 0; k < i; k++) {

		for (j = 0; j < MAX_BSSID_NUM; j++) {
			prBssInfo =
				GET_BSS_INFO_BY_INDEX(prAdapter, j);
			if (IS_BSS_ALIVE(prAdapter, prBssInfo) &&
				(prBssInfo->ucPrimaryChannel <=
				prSapSwitchCand[k].ucChUpperBound &&
				prBssInfo->ucPrimaryChannel >=
				prSapSwitchCand[k].ucChLowerBound) &&
				prSapSwitchCand[k].eRfBand ==
				prBssInfo->eBand) {
				prSapSwitchCand[k].eHwBand =
					prBssInfo->eHwBandIdx;
				DBGLOG(P2P, INFO,
					"[cand gen]alive bssindex:%d, hw band:%d\n",
					j,
					prBssInfo->eHwBandIdx);

			}
		}
		DBGLOG(P2P, INFO,
			"[cand gen]hw band:%d, rf band:%d, low_ch:%d, up_ch:%d\n",
			prSapSwitchCand[k].eHwBand,
			prSapSwitchCand[k].eRfBand,
			prSapSwitchCand[k].ucChLowerBound,
			prSapSwitchCand[k].ucChUpperBound);
	}

	if (prAdapter->rWifiVar.eDbdcMode == ENUM_DBDC_MODE_DISABLED)
		*eFilterScnario = P2P_STA_SCC_ONLY_SCENARIO;
	else if (!cnmGetOtherSapBssInfo(prAdapter,
			prP2pBssInfo))
		*eFilterScnario = P2P_MCC_SINGLE_SAP_SCENARIO;
	else
		*eFilterScnario = P2P_MCC_DUAL_SAP_SCENARIO;

	return i;
}


bool p2pFuncSwitchSapChannel(
		struct ADAPTER *prAdapter,
		enum ENUM_P2P_FILTER_SCENARIO_TYPE eFilterScnario)
{
	u_int8_t fgEnable = FALSE;
	struct BSS_INFO *prP2pBssInfo =
		(struct BSS_INFO *) NULL;
	struct BSS_INFO *aliveNonSapBss[MAX_BSSID_NUM];
	uint8_t ucNumAliveNonSapBss;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	uint8_t ucSapChCandNum = 0;
	struct P2P_CH_SWITCH_CANDIDATE rSapSwitchCand[ENUM_BAND_NUM + 2];
	struct P2P_CH_SWITCH_INTERFACE rSapSwitchInterface;
	uint8_t i;
	struct RF_CHANNEL_INFO rRfChnlInfo;
	uint8_t ucBssIdx = 0;
	uint32_t u4Idx = 0;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *mld_bssinfo;
#endif
#if CFG_SUPPORT_DFS_MASTER && CFG_SUPPORT_IDC_CH_SWITCH
	fgEnable = TRUE;
#endif

	if (!prAdapter
		|| !cnmSapIsConcurrent(prAdapter)
		|| !fgEnable) {
		DBGLOG(P2P, TRACE, "Not support concurrent STA + SAP\n");
		goto exit;
	}

	ucNumAliveNonSapBss = cnmGetAliveNonSapBssInfo(
						prAdapter, aliveNonSapBss);

	for (u4Idx = 0; u4Idx < ucNumAliveNonSapBss; u4Idx++) {
		if (aliveNonSapBss[u4Idx]->eBand <= BAND_NULL ||
			aliveNonSapBss[u4Idx]->eBand >= BAND_NUM) {
			DBGLOG(P2P, WARN, "STA has invalid band\n");
			goto exit;
		}
	}

	prP2pBssInfo = cnmGetSapBssInfo(prAdapter);

	if (!prP2pBssInfo) {
		DBGLOG(P2P, TRACE, "SAP is not active or MLD\n");
		goto exit;
	}
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mld_bssinfo = mldBssGetByBss(
			prAdapter, prP2pBssInfo);
	if (IS_MLD_BSSINFO_MULTI(mld_bssinfo)) {
		DBGLOG(P2P, TRACE, "SAP is MLD\n");
		goto exit;
	}
#endif
	if (prP2pBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) {
		DBGLOG(P2P, TRACE, "SAP is during initialization\n");
		goto exit;
	} else if (kalP2pIsStoppingAp(prAdapter,
		prP2pBssInfo)) {
		DBGLOG(P2P, INFO, "SAP is during uninitialization\n");
		goto exit;
	}

	prP2pRoleFsmInfo =
		P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
			prP2pBssInfo->u4PrivateData);
	if (!prP2pRoleFsmInfo) {
		DBGLOG(P2P, WARN, "SAP is not active\n");
		goto exit;
	}

	if (prP2pBssInfo->eBand <= BAND_NULL ||
		prP2pBssInfo->eBand >= BAND_NUM) {
		DBGLOG(P2P, WARN, "SAP has invalid band\n");
		goto exit;
	}
	/* Check other ap */
	p2pFuncSapAvailibilityCheck(prAdapter,
			prP2pBssInfo);

	ucSapChCandNum = p2pFuncSapSwichCandidatGen(prAdapter,
						rSapSwitchCand,
						prP2pBssInfo,
						&eFilterScnario);
	DBGLOG(P2P, WARN,
		"SAP CSA Scenario:%d and cand num:%d\n",
		eFilterScnario, ucSapChCandNum);

	rSapSwitchInterface.prP2pChInterface =
		rSapSwitchCand;
	rSapSwitchInterface.ucInterfaceLen =
		&ucSapChCandNum;


	p2pFuncSapSwitchChCheck(prAdapter,
			&rSapSwitchInterface,
			prP2pBssInfo,
			&eFilterScnario);


	/* Use sta ch info to do sap ch switch */
	if (ucSapChCandNum == 0 ||
		p2pFuncRoleToBssIdx(
		prAdapter, prP2pBssInfo->u4PrivateData,
		&ucBssIdx) != WLAN_STATUS_SUCCESS)
		goto exit;

	DBGLOG(P2P, WARN,
		"CSA band:%d, up_ch:%d, low_ch:%d\n",
		rSapSwitchCand[0].eRfBand,
		rSapSwitchCand[0].ucChUpperBound,
		rSapSwitchCand[0].ucChLowerBound);

	for (i = 0; i < ucSapChCandNum; i++) {
		DBGLOG(P2P, WARN,
			"CSA band:%d, up_ch:%d, low_ch:%d\n",
			rSapSwitchCand[i].eRfBand,
			rSapSwitchCand[i].ucChUpperBound,
			rSapSwitchCand[i].ucChLowerBound);
	}

	rlmGetChnlInfoForCSA(prAdapter,
		rSapSwitchCand[0].eRfBand,
		rSapSwitchCand[0].ucChUpperBound,
		ucBssIdx, &rRfChnlInfo);

#if CFG_SUPPORT_CCM
	if (prAdapter->fgIsCcmPending) {
		DBGLOG(CCM, INFO,
		       "SAP wants to switch channel, cancel CCM pending");
		cnmTimerStopTimer(prAdapter, &prAdapter->rCcmPendingTimer);
		prAdapter->fgIsCcmPending = FALSE;
	}
#endif

	cnmSapChannelSwitchReq(prAdapter,
		&rRfChnlInfo,
		prP2pBssInfo->u4PrivateData);

	return TRUE;

exit:

	DBGLOG(P2P, TRACE, "Check done\n");
	return FALSE;
}

void p2pFuncNotifySapStarted(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx)
{
#if CFG_HOTSPOT_SUPPORT_ADJUST_SCC
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct BSS_INFO *prBssInfo;
	struct GL_P2P_INFO *prP2PInfo;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo;
	struct P2P_CHNL_REQ_INFO *prP2pChnlReqInfo;
	uint8_t ucRoleIdx;
	u_int8_t fgIsSap = FALSE, fgIsMloSap = FALSE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(P2P, ERROR, "Null bss by idx(%u)\n",
			ucBssIdx);
		return;
	}

	ucRoleIdx = (uint8_t)prBssInfo->u4PrivateData;
	prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[ucRoleIdx];
	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
		ucRoleIdx);
	prP2pChnlReqInfo = &(prP2pRoleFsmInfo->rChnlReqInfo[0]);
	fgIsSap = p2pFuncIsAPMode(prWifiVar->prP2PConnSettings[ucRoleIdx]);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	fgIsMloSap = IS_MLD_BSSINFO_MULTI(mldBssGetByBss(prAdapter,
							 prBssInfo));
#endif

	if (!fgIsSap || fgIsMloSap)
		return;

	prP2PInfo->eChnlSwitchPolicy = CHNL_SWITCH_POLICY_NONE;
	p2pFuncSwitchSapChannel(prAdapter, P2P_DEFAULT_SCENARIO);
	if (prP2PInfo->eChnlSwitchPolicy != CHNL_SWITCH_POLICY_NONE) {
		if (prP2pChnlReqInfo->fgIsChannelRequested)
			p2pFuncReleaseCh(prAdapter, ucBssIdx,
					 prP2pChnlReqInfo);

		cnmTimerStopTimer(prAdapter,
			&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer));
	}
#endif
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief Get the pref freq list with maximum number assigned.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] eBandPrefer The preferred band.
 * \param[in] eMaxBW The maximum freq bandwidth.
 * \param[in] u4TopPreferNum The top N number of preferred freq.
 * \param[out] pu4Freq The freq list.
 *
 * \retval The number of the preferred freq obtained.
 */
/*---------------------------------------------------------------------------*/
uint8_t
p2pFunGetTopPreferFreqByBand(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBandPrefer,
		uint8_t ucTopPreferNum, uint32_t *pu4Freq)
{
	uint8_t ucMaxChnNum = MAX_PER_BAND_CHN_NUM;
	uint8_t ucNumOfChannel = 0;
	uint8_t i;
	struct RF_CHANNEL_INFO *aucChannelList = NULL;
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	uint8_t isVlpSupport = 0;
#endif

	aucChannelList = (struct RF_CHANNEL_INFO *) kalMemAlloc(
			sizeof(struct RF_CHANNEL_INFO) * ucMaxChnNum,
			VIR_MEM_TYPE);
	if (!aucChannelList) {
		DBGLOG(P2P, ERROR,
			"Allocate buffer for channel list fail\n");
		return 0;
	}
	kalMemZero(aucChannelList,
			sizeof(struct RF_CHANNEL_INFO) * ucMaxChnNum);

#if (CFG_SUPPORT_P2PGO_ACS == 1)
	p2pFunGetAcsBestChList(prAdapter,
			BIT(eBandPrefer),
			BITS(0, 31), BITS(0, 31),
			BITS(0, 31), BITS(0, 31),
			&ucNumOfChannel, aucChannelList);
#else
	rlmDomainGetChnlList(prAdapter, eBandPrefer, TRUE,
		ucMaxChnNum, &ucNumOfChannel, aucChannelList);
#endif

	for (i = 0; i < ucNumOfChannel && i < ucTopPreferNum; i++) {
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		if (aucChannelList[i].eBand == BAND_6G &&
		    rlmDomain6GPwrModeCountrySupportChk(
		    aucChannelList[i].eBand,
		    aucChannelList[i].ucChannelNum,
		    COUNTRY_CODE_TW, PWR_MODE_6G_VLP,
		    &isVlpSupport) == WLAN_STATUS_SUCCESS &&
		    isVlpSupport == FALSE) {
			ucTopPreferNum++;
			continue;
		}
#endif
		*(pu4Freq + i) = nicChannelNum2Freq(
			aucChannelList[i].ucChannelNum,
			aucChannelList[i].eBand) / 1000;
	}

	kalMemFree(aucChannelList, VIR_MEM_TYPE,
			sizeof(struct RF_CHANNEL_INFO) * ucMaxChnNum);

	return i;
}

uint8_t p2pFuncGetFreqAllowList(struct ADAPTER *prAdapter,
			      uint32_t *pau4AllowFreqList)
{
	struct RF_CHANNEL_INFO *prChnlList = NULL;
	uint8_t i, ucChnlNum = 0, ucCandidateChnlNum;
	struct PARAM_GET_CHN_INFO *prLteSafeChn = NULL;
	uint32_t u4BufLen;
	uint32_t *pau4SafeChnl;
	uint32_t u4SafeChnlInfo_2g;
	uint32_t u4SafeChnlInfo_5g_0;
	uint32_t u4SafeChnlInfo_5g_1;
	uint32_t u4SafeChnlInfo_6g;
	uint32_t rStatus;

	/* Get Lte Safe Chnl */
	prLteSafeChn = kalMemZAlloc(sizeof(struct PARAM_GET_CHN_INFO),
			VIR_MEM_TYPE);
	if (!prLteSafeChn)
		goto exit;

	rStatus = kalIoctl(prAdapter->prGlueInfo, wlanoidQueryLteSafeChannel,
		   prLteSafeChn, sizeof(struct PARAM_GET_CHN_INFO), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(P2P, ERROR, "get safe chnl failed");

	pau4SafeChnl = prLteSafeChn->rLteSafeChnList.au4SafeChannelBitmask;
	u4SafeChnlInfo_2g = pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_2G4];
	u4SafeChnlInfo_5g_0 = pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_5G_0];
	u4SafeChnlInfo_5g_1 = pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_5G_1];
	u4SafeChnlInfo_6g = pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_6G];

	if (!u4SafeChnlInfo_2g && !u4SafeChnlInfo_5g_0 && !u4SafeChnlInfo_5g_1
	    && !u4SafeChnlInfo_6g) {
		DBGLOG(P2P, WARN, "No safe chnl, reset safe chnl bitmap");
		u4SafeChnlInfo_2g = BITS(0, 31);
		u4SafeChnlInfo_5g_0 = BITS(0, 31);
		u4SafeChnlInfo_5g_1 = BITS(0, 31);
		u4SafeChnlInfo_6g = BITS(0, 31);
	}

	DBGLOG(P2P, INFO,
	       "safe chnl bitmask: 2G=0x%x, 5G_0=0x%x, 5G_1=0x%x, 6G=0x%x",
	       u4SafeChnlInfo_2g, u4SafeChnlInfo_5g_0, u4SafeChnlInfo_5g_1,
	       u4SafeChnlInfo_6g);

	/* Get channel allow list */
	prChnlList = kalMemZAlloc(sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM,
			VIR_MEM_TYPE);
	if (!prChnlList)
		goto exit;

	rlmDomainGetChnlList(prAdapter, BAND_NULL, TRUE, MAX_CHN_NUM,
		&ucCandidateChnlNum, prChnlList);

	for (i = 0; i < ucCandidateChnlNum; ++i) {
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (prChnlList[i].eBand == BAND_6G) {
			/* Keep only PSC channels */
			if (!(prChnlList[i].ucChannelNum >= 5 &&
			      prChnlList[i].ucChannelNum <= 225 &&
			      ((prChnlList[i].ucChannelNum - 5) % 16 == 0)))
				continue;
		}
#endif

		if (!p2pFuncIsLteSafeChnl(prChnlList[i].eBand,
					  prChnlList[i].ucChannelNum,
					  pau4SafeChnl))
			continue;

		DBGLOG(P2P, LOUD, "safe chnl: %u", prChnlList[i].ucChannelNum);
		pau4AllowFreqList[ucChnlNum++] = nicChannelNum2Freq(
			prChnlList[i].ucChannelNum,
			prChnlList[i].eBand) / 1000;
	}
exit:
	if (prChnlList)
		kalMemFree(prChnlList, VIR_MEM_TYPE,
				sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM);
	if (prLteSafeChn)
		kalMemFree(prLteSafeChn, VIR_MEM_TYPE,
				sizeof(struct PARAM_GET_CHN_INFO));

	return ucChnlNum;
}

uint8_t p2pFuncGetAllAcsFreqList(struct ADAPTER *prAdapter,
					uint32_t *ucChnlNum,
					struct RF_CHANNEL_INFO *arChnlList,
					uint32_t *pau4SafeFreqList)
{
	uint8_t i;
	uint32_t freq_list[MAX_CHN_NUM] = {};

	uint32_t num_freq_list = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	for (i = 0; i < *ucChnlNum; ++i) {
		pau4SafeFreqList[i] = nicChannelNum2Freq(
			arChnlList[i].ucChannelNum,
			arChnlList[i].eBand) / 1000;
	}

	rStatus = p2pFunGetPreferredFreqList(prAdapter, IFTYPE_AP,
			freq_list, &num_freq_list, pau4SafeFreqList,
			*ucChnlNum, FALSE);

	for (i = 0; i < num_freq_list; i++)
		pau4SafeFreqList[i] = freq_list[i];

	DBGLOG(P2P, ERROR,
			"acs freq list input/ output len: %d/ %d\n",
			*ucChnlNum,
			num_freq_list);
	return (uint8_t)num_freq_list;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief Append a list of freq prBssList into pau4FreqList.
 *
 * \param[in] prBssList Pointer to alive bss info.
 * \param[in] ucNumOfAliveBss Number of alive bss.
 * \param[in] pau4FreqList Pointer to the output freq list.
 *
 * \retval Number of appended freq.
 */
/*---------------------------------------------------------------------------*/
uint8_t p2pFuncAppendPrefFreq(struct BSS_INFO **prBssList,
	uint8_t ucNumOfAliveBss, uint32_t *pau4FreqList)
{
	uint32_t freq;
	uint8_t i, j, fgIsExist, ucFreqNum = 0;

	for (i = 0; i < ucNumOfAliveBss; ++i) {
		fgIsExist = FALSE;
		freq = nicChannelNum2Freq(
			prBssList[i]->ucPrimaryChannel,
			prBssList[i]->eBand) / 1000;

		/* prevent append duplicate SCC channels */
		for (j = 0; j < ucFreqNum; ++j) {
			if (freq == pau4FreqList[j]) {
				fgIsExist = TRUE;
				DBGLOG(P2P, TRACE,
				       "BSS[%u] freq %u duplicate with BSS[%u]",
				       prBssList[i]->ucBssIndex, freq,
				       prBssList[j]->ucBssIndex);
			}
		}

		if (!fgIsExist)
			pau4FreqList[ucFreqNum++] = freq;
	}
	return ucFreqNum;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief Get the prefer safe freq by intersection of prefer freq and safe freq.
 *
 * \param[in] targetFreqList Pointer to prefer freq.
 * \param[in] targetFreqListNum Number of prefer freq.
 * \param[in] prSafeChnlList Pointer to safe chnl list.
 * \param[in] ucSafeChnlNum Number of safe chnl.
 *
 * \retval void.
 */
/*---------------------------------------------------------------------------*/
void p2pFuncGetSafeFreq(enum ENUM_IFTYPE eIftype,
			uint32_t *targetFreqList, uint32_t *targetFreqListNum,
			uint32_t *pau4FreqAllowList, uint8_t ucAllowFreqNum)
{
	uint32_t rIntersectionFreq[MAX_CHN_NUM] = { 0 };
	uint8_t i, j, ucFreqNum = 0;

	/* intersection of targetFreqList and rIntersectionFreq */
	for (i = 0; i < *targetFreqListNum; ++i) {
		for (j = 0; j < ucAllowFreqNum; ++j)
			if (targetFreqList[i] == pau4FreqAllowList[j])
				break;
		if (j < ucAllowFreqNum) {
			DBGLOG(P2P, LOUD, "[%u] insert freq: %u, ch: %u\n",
			       i, targetFreqList[i],
			       nicFreq2ChannelNum(targetFreqList[i] * 1000));
			rIntersectionFreq[ucFreqNum++] = targetFreqList[i];
		} else {
			if(eIftype == IFTYPE_P2P_GO || eIftype == IFTYPE_P2P_CLIENT){
				DBGLOG(P2P, TRACE, "[%u] p2p dfs scc: insert freq: %u, ch: %u\n",
				       i, targetFreqList[i],
				       nicFreq2ChannelNum(targetFreqList[i] * 1000));
				rIntersectionFreq[ucFreqNum++] = targetFreqList[i];
			} else {
				DBGLOG(P2P, TRACE, "[%u] ignore unsafe freq: %u, ch: %u\n",
				       i, targetFreqList[i],
				       nicFreq2ChannelNum(targetFreqList[i] * 1000));
			}
		}
	}

	if (ucFreqNum > 0) {
		kalMemCopy(targetFreqList, rIntersectionFreq,
			   ucFreqNum * sizeof(uint32_t));
		*targetFreqListNum = ucFreqNum;
	} else if (eIftype == IFTYPE_AP) {
		/* SAP must obey allow list from upper layer */
		kalMemCopy(targetFreqList, pau4FreqAllowList,
			   ucAllowFreqNum * sizeof(uint32_t));
		*targetFreqListNum = ucAllowFreqNum;
	}
	/* P2P return original targetFreqList */
}

#if CFG_SUPPORT_CCM
uint32_t p2pFuncAppendAaFreq(struct ADAPTER *prAdapter,
			     struct BSS_INFO *prBssInfo, uint32_t *apu4FreqList)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct RF_CHANNEL_INFO *prRfChnlInfo1;
	struct RF_CHANNEL_INFO rRfChnlInfo2;
	struct RF_CHANNEL_INFO arChnlList[
		MAX_5G_BAND_CHN_NUM + MAX_6G_BAND_CHN_NUM] = { 0 };
	struct BSS_INFO *bss;
	uint8_t bssIdx;
	uint8_t ch;
	uint8_t ucChNum, ucCandidateChnlNum = 0;

#if (CFG_SUPPORT_P2PGO_ACS == 1)
#if (CFG_SUPPORT_WIFI_6G == 1)
	p2pFunGetAcsBestChList(prAdapter, BIT(BAND_5G) | BIT(BAND_6G),
			       BITS(0, 31), BITS(0, 31),
			       BITS(0, 31), BITS(0, 31),
			       &ucCandidateChnlNum, arChnlList);
#endif /* CFG_SUPPORT_WIFI_6G == 1 */
#else /* CFG_SUPPORT_P2PGO_ACS == 1 */
#if (CFG_SUPPORT_WIFI_6G == 1)
	rlmDomainGetChnlList(prAdapter, BAND_6G, TRUE, MAX_6G_BAND_CHN_NUM,
		&ucChNum, &arChnlList[ucCandidateChnlNum]);
	ucCandidateChnlNum += ucChNum;
#endif /* CFG_SUPPORT_WIFI_6G == 1 */

	rlmDomainGetChnlList(prAdapter, BAND_5G, TRUE, MAX_5G_BAND_CHN_NUM,
		&ucChNum, arChnlList);
	ucCandidateChnlNum += ucChNum;
#endif

	ucChNum = 0;
	for (ch = 0; ch < ucCandidateChnlNum; ++ch) {
		prRfChnlInfo1 = &arChnlList[ch];

		if (prRfChnlInfo1->eBand == BAND_5G)
			prRfChnlInfo1->ucChnlBw =  prWifiVar->ucP2p5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (prRfChnlInfo1->eBand == BAND_6G)
			prRfChnlInfo1->ucChnlBw =  prWifiVar->ucP2p6gBandwidth;
#endif /* CFG_SUPPORT_WIFI_6G == 1 */

		prRfChnlInfo1->u4CenterFreq1 = nicGetS1Freq(prAdapter,
			prRfChnlInfo1->eBand, prRfChnlInfo1->ucChannelNum,
			prRfChnlInfo1->ucChnlBw);

		DBGLOG(P2P, LOUD, "chnlInfo1 b:%u, ch:%u, bw:%u, cf:%u\n",
		       prRfChnlInfo1->eBand,
		       prRfChnlInfo1->ucChannelNum,
		       prRfChnlInfo1->ucChnlBw,
		       prRfChnlInfo1->u4CenterFreq1);

		for (bssIdx = 0; bssIdx < MAX_BSSID_NUM; ++bssIdx) {
			bss = GET_BSS_INFO_BY_INDEX(prAdapter, bssIdx);

			if (!bss || !IS_BSS_ALIVE(prAdapter, bss) ||
			    bss->eBand == BAND_2G4)
				continue;

			/* skip BSS itself for CCM */
			if (prBssInfo && bss == prBssInfo)
				continue;

			rRfChnlInfo2.eBand = bss->eBand;
			rRfChnlInfo2.ucChannelNum = bss->ucPrimaryChannel;
			rRfChnlInfo2.ucChnlBw = rlmGetBssOpBwByChannelWidth(
					bss->eBssSCO, bss->ucVhtChannelWidth);
			rRfChnlInfo2.u4CenterFreq1 = nicChannelNum2Freq(
			      bss->ucVhtChannelFrequencyS1, bss->eBand) / 1000;
			DBGLOG(P2P, LOUD,
			       "chnlInfo2 b:%u, ch:%u, bw:%u, cf:%u\n",
			       rRfChnlInfo2.eBand,
			       rRfChnlInfo2.ucChannelNum,
			       rRfChnlInfo2.ucChnlBw,
			       rRfChnlInfo2.u4CenterFreq1);

			if (ccmAAAvailableCheck(prAdapter, prRfChnlInfo1,
						&rRfChnlInfo2)) {
				*(apu4FreqList + ucChNum++) =
					nicChannelNum2Freq(
						prRfChnlInfo1->ucChannelNum,
						prRfChnlInfo1->eBand) / 1000;
				DBGLOG(P2P, INFO, "valid [%u]ch:%u\n",
				       ucChNum, prRfChnlInfo1->ucChannelNum);
			}
		}
	}

	return ucChNum;
}
#endif /* CFG_SUPPORT_CCM */

uint32_t p2pFuncGetPreferAliveBssByBand(struct ADAPTER *prAdapter,
					enum ENUM_BAND eBand,
					struct BSS_INFO **prBssList,
					u_int8_t fgIsSkipDfs)
{
	struct BSS_INFO *bss;
	uint8_t i, ucNumAliveBss = 0, fgIsStaGcExist = FALSE;

	for (i = 0; i < MAX_BSSID_NUM && !fgIsStaGcExist; ++i) {
		bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		if (!bss || !IS_BSS_ALIVE(prAdapter, bss) ||
		    bss->eBand != eBand)
			continue;

		if (IS_BSS_AIS(bss) || IS_BSS_GC(bss))
			fgIsStaGcExist = TRUE;
	}

	for (i = 0; i < MAX_BSSID_NUM; ++i) {
		bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		if (!bss || !IS_BSS_ALIVE(prAdapter, bss))
			continue;
		else if (bss->eBand != eBand)
			continue;
		else if (fgIsSkipDfs && rlmDomainIsDfsChnls(prAdapter,
						bss->ucPrimaryChannel)) {
			DBGLOG(P2P, WARN, "skip Bss%u in dfs ch:%u\n",
			       i, bss->ucPrimaryChannel);
			continue;
		}

		/* only get STA/GC if STA/GC exist for pref freq selection,
		 * then prevent MCC by CCM.
		 */
		if (!fgIsStaGcExist || IS_BSS_AIS(bss) || IS_BSS_GC(bss)) {
			prBssList[ucNumAliveBss++] = bss;
			DBGLOG(BSS, INFO,
			       "bss%u(%s), b:%u, ch:%u, bw:%s, hw_band:%u\n",
			       i, bssGetRoleTypeString(prAdapter, bss),
			       bss->eBand, bss->ucPrimaryChannel,
			       bssOpBw2Str(bss), bss->eHwBandIdx);
		} else {
			DBGLOG(BSS, TRACE,
			       "ignore and prevent MCC by CCM, bss%u(%s), b:%u, ch:%u, bw:%s, hw_band:%u\n",
			       i, bssGetRoleTypeString(prAdapter, bss),
			       bss->eBand, bss->ucPrimaryChannel,
			       bssOpBw2Str(bss), bss->eHwBandIdx);
		}
	}

	return ucNumAliveBss;
}


/*---------------------------------------------------------------------------*/
/*!
 * \brief Get the pref freq list with BSS based algo.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] eIftype Type of iface.
 * \param[in] freq_list Pointer to store output prefer list.
 * \param[in] num_freq_list Number of freq in freq_list.
 *
 * \retval Status.
 */
/*---------------------------------------------------------------------------*/
uint32_t p2pFunGetPreferredFreqList(struct ADAPTER *prAdapter,
		enum ENUM_IFTYPE eIftype, uint32_t *pau4FreqList,
		uint32_t *pu4FreqListNum, uint32_t *pau4FreqAllowList,
		uint8_t ucAllowFreqNum, u_int8_t fgIsSkipDfs)
{
	struct BSS_INFO *aliveBss2g[MAX_BSSID_NUM] = { 0 };
	struct BSS_INFO *aliveBss5g[MAX_BSSID_NUM] = { 0 };
	struct BSS_INFO *aliveBss6g[MAX_BSSID_NUM] = { 0 };
#if (CFG_SUPPORT_WIFI_6G == 1)
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#endif
	uint8_t ucNumAliveBss2g, ucNumAliveBss5g, ucNumAliveBss6g = 0;

	/* prepare alive bss info for SCC */
	ucNumAliveBss2g = p2pFuncGetPreferAliveBssByBand(
			prAdapter, BAND_2G4, aliveBss2g, fgIsSkipDfs);
	ucNumAliveBss5g = p2pFuncGetPreferAliveBssByBand(
			prAdapter, BAND_5G, aliveBss5g, FALSE);
#if (CFG_SUPPORT_WIFI_6G == 1)
	ucNumAliveBss6g = p2pFuncGetPreferAliveBssByBand(
			prAdapter, BAND_6G, aliveBss6g, fgIsSkipDfs);
#endif
	DBGLOG(P2P, INFO,
	       "alive Bss num [bn0:bn1:bn2]=[%u:%u:%u]\n",
	       ucNumAliveBss2g, ucNumAliveBss5g, ucNumAliveBss6g);

#if (CFG_SUPPORT_CCM)
	/* Prefer A+A for SP Skyhawk Sku1 2G+2A+1A */
	if (p2pFuncIsPreferWfdAa(prAdapter, NULL))
		*pu4FreqListNum += p2pFuncAppendAaFreq(prAdapter, NULL,
			&pau4FreqList[*pu4FreqListNum]);
#endif

	/* Append 5G/6G channels first */
	if (ucNumAliveBss5g + ucNumAliveBss6g > 0) {
		*pu4FreqListNum += p2pFuncAppendPrefFreq(aliveBss6g,
		    ucNumAliveBss6g, &pau4FreqList[*pu4FreqListNum]);
		*pu4FreqListNum += p2pFuncAppendPrefFreq(aliveBss5g,
		    ucNumAliveBss5g, &pau4FreqList[*pu4FreqListNum]);
	} else {
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (IS_FEATURE_DISABLED(prWifiVar->ucDisallowAcs6G))
			*pu4FreqListNum += p2pFunGetTopPreferFreqByBand(
				prAdapter, BAND_6G,
				MAX_6G_BAND_CHN_NUM,
				&pau4FreqList[*pu4FreqListNum]);
#endif
		*pu4FreqListNum += p2pFunGetTopPreferFreqByBand(
			prAdapter, BAND_5G,
			MAX_5G_BAND_CHN_NUM,
			&pau4FreqList[*pu4FreqListNum]);
	}

	/* Append 2G channels */
	if (ucNumAliveBss2g > 0) {
		/* Prefer SCC */
		*pu4FreqListNum += p2pFuncAppendPrefFreq(aliveBss2g,
			ucNumAliveBss2g, &pau4FreqList[*pu4FreqListNum]);
	} else {
		/* Band is idle, append whole 2G */
		*pu4FreqListNum += p2pFunGetTopPreferFreqByBand(
			prAdapter,
			BAND_2G4,
			MAX_2G_BAND_CHN_NUM,
			&pau4FreqList[*pu4FreqListNum]);
	}

	p2pFuncGetSafeFreq(eIftype, pau4FreqList, pu4FreqListNum,
			   pau4FreqAllowList, ucAllowFreqNum);

	return WLAN_STATUS_SUCCESS;
}


enum ENUM_P2P_CONNECT_STATE
p2pFuncGetP2pActionFrameType(struct MSDU_INFO *prMgmtMsdu)
{
	struct WLAN_MAC_HEADER *prWlanHdr = (struct WLAN_MAC_HEADER *) NULL;
	struct WLAN_ACTION_FRAME *prActFrame;
	uint8_t *pucVendor = NULL;

	prWlanHdr = (struct WLAN_MAC_HEADER *)
			((uintptr_t) prMgmtMsdu->prPacket +
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
p2pFuncCheckOnRocChnl(struct RF_CHANNEL_INFO *prTxChnl,
		struct P2P_CHNL_REQ_INFO *prChnlReqInfo)
{
	if (prTxChnl == NULL || prChnlReqInfo == NULL)
		return FALSE;

	if (prTxChnl->ucChannelNum == prChnlReqInfo->ucReqChnlNum &&
			prChnlReqInfo->fgIsChannelRequested)
		return TRUE;

	return FALSE;
}

u_int8_t
p2pFuncNeedWaitRsp(struct ADAPTER *prAdapter,
		enum ENUM_P2P_CONNECT_STATE eConnState)
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

u_int8_t
p2pFuncNeedForceSleep(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *bss;
	uint8_t ucApForceSleep;

	if (!prAdapter ||
		!prAdapter->rWifiVar.ucApForceSleep ||
		(prAdapter->rPerMonitor.u4CurrPerfLevel > 1))
		return FALSE;

	if (!prAdapter->fgIsP2PRegistered ||
		(prAdapter->rP2PNetRegState !=
		ENUM_NET_REG_STATE_REGISTERED))
		return FALSE;

	bss = cnmGetSapBssInfo(prAdapter);
	ucApForceSleep = prAdapter->rWifiVar.ucApForceSleep;
	if (!bss)
		return FALSE;
	else if ((ucApForceSleep == 1) &&
		(bss->eConnectionState == MEDIA_STATE_CONNECTED)) {
#if CFG_SAP_RPS_SUPPORT
		if ((!prAdapter->rWifiVar.fgSapRpsSwitch &&
			prAdapter->rWifiVar.fgSapRpsEnable) ||
			!prAdapter->rWifiVar.fgSapRpsEnable)
#endif
			return FALSE;
	}
#if 0
	else if ((ucApForceSleep == 2) &&
		prAdapter->u4StaInPSBitmap)
		return FALSE;
#endif

	return TRUE;
}

void
p2pFunClearAllTxReq(struct ADAPTER *prAdapter,
		struct P2P_MGMT_TX_REQ_INFO *prP2pMgmtTxInfo)
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

uint8_t p2pFunGetAcsBestCh(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand,
		enum ENUM_MAX_BANDWIDTH_SETTING eChnlBw,
		uint32_t u4LteSafeChnMask_2G,
		uint32_t u4LteSafeChnMask_5G_1,
		uint32_t u4LteSafeChnMask_5G_2,
		uint32_t u4LteSafeChnMask_6G)
{
	struct RF_CHANNEL_INFO aucChannelList[MAX_PER_BAND_CHN_NUM];
	uint8_t ucNumOfChannel;
	struct PARAM_GET_CHN_INFO *prGetChnLoad;
	uint8_t i;
	struct PARAM_PREFER_CHN_INFO rPreferChannel;

	/* reset */
	rPreferChannel.ucChannel = 0;
	rPreferChannel.u4Dirtiness = 0xFFFFFFFF;

	kalMemZero(aucChannelList, sizeof(aucChannelList));

	rlmDomainGetChnlList(prAdapter, eBand, TRUE, MAX_PER_BAND_CHN_NUM,
			&ucNumOfChannel, aucChannelList);

	/*
	 * 2. Calculate each channel's dirty score
	 */
	prGetChnLoad = &(prAdapter->rWifiVar.rChnLoadInfo);

	DBGLOG(P2P, INFO, "acs chnl mask=[0x%08x][0x%08x][0x%08x][0x%08x]\n",
			u4LteSafeChnMask_2G,
			u4LteSafeChnMask_5G_1,
			u4LteSafeChnMask_5G_2,
			u4LteSafeChnMask_6G);

	for (i = 0; i < ucNumOfChannel; i++) {
		uint8_t ucIdx;

		ucIdx = wlanGetChannelIndex(aucChannelList[i].eBand,
				aucChannelList[i].ucChannelNum);

		if (ucIdx >= MAX_CHN_NUM)
			continue;

		DBGLOG(P2P, TRACE, "idx: %u, ch: %u, d: %d\n",
				ucIdx,
				aucChannelList[i].ucChannelNum,
				prGetChnLoad->rEachChnLoad[ucIdx].u4Dirtiness);

		if (aucChannelList[i].eBand == BAND_2G4) {
			if (!(u4LteSafeChnMask_2G & BIT(
					aucChannelList[i].ucChannelNum)))
				continue;
		} else if (aucChannelList[i].eBand == BAND_5G &&
				(aucChannelList[i].ucChannelNum >= 36) &&
				(aucChannelList[i].ucChannelNum <= 144)) {
			if (!(u4LteSafeChnMask_5G_1 & BIT(
				(aucChannelList[i].ucChannelNum - 36) / 4)))
				continue;
		} else if (aucChannelList[i].eBand == BAND_5G &&
				(aucChannelList[i].ucChannelNum >= 149) &&
				(aucChannelList[i].ucChannelNum <= 181)) {
			if (!(u4LteSafeChnMask_5G_2 & BIT(
				(aucChannelList[i].ucChannelNum - 149) / 4)))
				continue;
		}

#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (aucChannelList[i].eBand == BAND_6G) {
			/* Keep only PSC channels */
			if (!(aucChannelList[i].ucChannelNum >= 5 &&
			      aucChannelList[i].ucChannelNum <= 225 &&
			      ((aucChannelList[i].ucChannelNum - 5) % 16 == 0)))
				continue;

			/* Skip unsafe BW 80 channels */
			if ((eChnlBw >= MAX_BW_80MHZ) &&
				(aucChannelList[i].ucChannelNum >= 5) &&
				(aucChannelList[i].ucChannelNum <= 225) &&
				!(u4LteSafeChnMask_6G & BIT(
				  (aucChannelList[i].ucChannelNum - 5) / 16)))
				continue;
		}
#endif

		if (eBand == BAND_2G4 && eChnlBw >= MAX_BW_40MHZ &&
		    nicGetSco(prAdapter, eBand,
			      aucChannelList[i].ucChannelNum) == CHNL_EXT_SCN)
			continue;

		if (eBand == BAND_5G && eChnlBw >= MAX_BW_160MHZ &&
		    nicGetVhtS1(prAdapter, eBand,
				aucChannelList[i].ucChannelNum,
				VHT_OP_CHANNEL_WIDTH_160) == 0)
			continue;
		else if (eBand == BAND_5G && eChnlBw >= MAX_BW_80MHZ &&
			 nicGetVhtS1(prAdapter, eBand,
				     aucChannelList[i].ucChannelNum,
				     VHT_OP_CHANNEL_WIDTH_80) == 0)
			continue;
		else if (eBand == BAND_5G && eChnlBw >= MAX_BW_40MHZ &&
			 nicGetSco(prAdapter, eBand,
				   aucChannelList[i].ucChannelNum) ==
			 CHNL_EXT_SCN)
			continue;

#if (CFG_SUPPORT_WIFI_6G == 1)
		if (eBand == BAND_6G && eChnlBw >= MAX_BW_320_2MHZ &&
		    nicGetHe6gS1(prAdapter, aucChannelList[i].ucChannelNum,
				 CW_320_2MHZ) == 0)
			continue;
		else if (eBand == BAND_6G && eChnlBw >= MAX_BW_320_1MHZ &&
			 nicGetHe6gS1(prAdapter, aucChannelList[i].ucChannelNum,
				      CW_320_1MHZ) == 0)
			continue;
		else if (eBand == BAND_6G && eChnlBw >= MAX_BW_160MHZ &&
			 nicGetHe6gS1(prAdapter, aucChannelList[i].ucChannelNum,
				      CW_160MHZ) == 0)
			continue;
		else if (eBand == BAND_6G && eChnlBw >= MAX_BW_80MHZ &&
			 nicGetHe6gS1(prAdapter, aucChannelList[i].ucChannelNum,
				      CW_80MHZ) == 0)
			continue;
		else if (eBand == BAND_6G && eChnlBw >= MAX_BW_40MHZ &&
			 nicGetSco(prAdapter, eBand,
				   aucChannelList[i].ucChannelNum) ==
			 CHNL_EXT_SCN)
			continue;
#endif

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

#if (CFG_SUPPORT_P2PGO_ACS == 1)
void p2pFunGetAcsBestChList(struct ADAPTER *prAdapter,
		uint8_t eBandSel,
		uint32_t u4LteSafeChnMask_2G,
		uint32_t u4LteSafeChnMask_5G_1,
		uint32_t u4LteSafeChnMask_5G_2,
		uint32_t u4LteSafeChnMask_6G,
		uint8_t *pucSortChannelNumber,
		struct RF_CHANNEL_INFO *paucSortChannelList)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct PARAM_GET_CHN_INFO *prChnLoad;
	struct PARAM_CHN_RANK_INFO *prChnRank;
	uint8_t i, ucInUsedCHNumber = 0;
	uint32_t au4LteSafeChnMask[ENUM_SAFE_CH_MASK_MAX_NUM];

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
	au4LteSafeChnMask[ENUM_SAFE_CH_MASK_BAND_2G4] = u4LteSafeChnMask_2G;
	au4LteSafeChnMask[ENUM_SAFE_CH_MASK_BAND_5G_0] = u4LteSafeChnMask_5G_1;
	au4LteSafeChnMask[ENUM_SAFE_CH_MASK_BAND_5G_1] = u4LteSafeChnMask_5G_2;
	au4LteSafeChnMask[ENUM_SAFE_CH_MASK_BAND_6G] = u4LteSafeChnMask_6G;

	for (i = 0; i < MAX_CHN_NUM; i++) {
		prChnRank = &(prChnLoad->rChnRankList[i]);

		DBGLOG(P2P, TRACE, "idx:%d band:%d ch:%u d:0x%x\n",
			i,
			prChnRank->eBand,
			prChnRank->ucChannel,
			prChnRank->u4Dirtiness);

		if (!(eBandSel & BIT(prChnRank->eBand)))
			continue;

		if (!p2pFuncIsLteSafeChnl(prChnRank->eBand,
					  prChnRank->ucChannel,
					  au4LteSafeChnMask))
			continue;

#if (CFG_SUPPORT_WIFI_6G == 1)
		/* Keep only PSC channels */
		if (prChnRank->eBand == BAND_6G &&
		    !(prChnRank->ucChannel >= 5 &&
		      prChnRank->ucChannel <= 225 &&
		      IS_6G_PSC_CHANNEL(prChnRank->ucChannel)))
			continue;
#endif

		if (prChnRank->eBand == BAND_5G &&
		    prWifiVar->ucP2p5gBandwidth >= MAX_BW_80MHZ &&
			nicGetVhtS1(prAdapter, BAND_5G, prChnRank->ucChannel,
				rlmGetVhtOpBwByBssOpBw(
					prWifiVar->ucP2p5gBandwidth)) == 0)
			continue;

#if (CFG_SUPPORT_WIFI_6G == 1)
		/* If eChnlBw == MAX_BW_320MHZ, it will skip 160BW channel */
		if (prChnRank->eBand == BAND_6G &&
		    prWifiVar->ucP2p6gBandwidth >= MAX_BW_80MHZ &&
			nicGetHe6gS1(prAdapter, prChnRank->ucChannel,
				rlmGetVhtOpBwByBssOpBw(
					prWifiVar->ucP2p6gBandwidth)) == 0)
			continue;
#endif

		(paucSortChannelList+ucInUsedCHNumber)->ucChannelNum =
			prChnRank->ucChannel;
		(paucSortChannelList+ucInUsedCHNumber)->eBand =
			prChnRank->eBand;

		ucInUsedCHNumber++;
	}

	/*
	 * 4. Dump the Result
	*/
	*pucSortChannelNumber = ucInUsedCHNumber;
	for (i = 0; i < ucInUsedCHNumber; i++) {
		DBGLOG(P2P, TRACE, "ACS idx=%d, band[%d] ch[%d]\n", i,
			(paucSortChannelList+i)->eBand,
			(paucSortChannelList+i)->ucChannelNum);
	}
}
#endif

void p2pFunProcessAcsReport(struct ADAPTER *prAdapter,
		uint8_t ucRoleIndex,
		struct P2P_ACS_REQ_INFO *prAcsReqInfo)
{
	if (!prAdapter || !prAcsReqInfo)
		return;

	prAcsReqInfo->ucPrimaryCh =
		p2pFunGetAcsBestCh(prAdapter,
			prAcsReqInfo->eBand,
			prAcsReqInfo->eChnlBw,
			prAcsReqInfo
			->au4SafeChnl[ENUM_SAFE_CH_MASK_BAND_2G4],
			prAcsReqInfo
			->au4SafeChnl[ENUM_SAFE_CH_MASK_BAND_5G_0],
			prAcsReqInfo
			->au4SafeChnl[ENUM_SAFE_CH_MASK_BAND_5G_1],
			prAcsReqInfo
			->au4SafeChnl[ENUM_SAFE_CH_MASK_BAND_6G]);

	p2pFunIndicateAcsResult(prAdapter->prGlueInfo, prAcsReqInfo);
}

void p2pFunIndicateAcsResult(struct GLUE_INFO *prGlueInfo,
		struct P2P_ACS_REQ_INFO *prAcsReqInfo)
{
	struct WIFI_VAR *prWifiVar;
	uint8_t ucMaxBandwidth = MAX_BW_20MHZ;

	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

	if (prAcsReqInfo->ucPrimaryCh == 0) {
		if (prAcsReqInfo->eHwMode == P2P_VENDOR_ACS_HW_MODE_11B ||
		    prAcsReqInfo->eHwMode == P2P_VENDOR_ACS_HW_MODE_11G)
			prAcsReqInfo->ucPrimaryCh = AP_DEFAULT_CHANNEL_2G;
		else
			prAcsReqInfo->ucPrimaryCh = AP_DEFAULT_CHANNEL_5G;

		DBGLOG(P2P, WARN,
			"No chosed channel, use default channel %d\n",
			prAcsReqInfo->ucPrimaryCh);
	}

	if (!prGlueInfo->prAdapter->rWifiVar.fgSapOverwriteAcsChnlBw ||
	    prAcsReqInfo->fgIsAis)
		goto skip_bw_overwrite;

	if (prAcsReqInfo->eBand == BAND_2G4)
		ucMaxBandwidth = prWifiVar->ucAp2gBandwidth;
	else if (prAcsReqInfo->eBand == BAND_5G)
		ucMaxBandwidth = prWifiVar->ucAp5gBandwidth;
#ifdef CFG_SUPPORT_6G_OVERWRITE_ACS_BW
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prAcsReqInfo->eBand == BAND_6G)
		ucMaxBandwidth = prWifiVar->ucAp6gBandwidth;
#endif /* CFG_SUPPORT_WIFI_6G */
#endif /* CFG_SUPPORT_6G_OVERWRITE_ACS_BW */

	if (ucMaxBandwidth > prWifiVar->ucApBandwidth)
		ucMaxBandwidth = prWifiVar->ucApBandwidth;

	if (prAcsReqInfo->eBand != BAND_2G4 &&
	    ucMaxBandwidth != prAcsReqInfo->eChnlBw) {
		uint8_t ucS1;

		ucS1 = nicGetS1(prGlueInfo->prAdapter,
				prAcsReqInfo->eBand,
				prAcsReqInfo->ucPrimaryCh,
				rlmGetVhtOpBwByBssOpBw(ucMaxBandwidth));

		if ((ucMaxBandwidth >= MAX_BW_80MHZ && ucS1) ||
		    ucMaxBandwidth < MAX_BW_80MHZ) {
			DBGLOG(P2P, WARN,
				"Adjust bw from %d to %d\n",
				prAcsReqInfo->eChnlBw,
				ucMaxBandwidth);
			prAcsReqInfo->eChnlBw = ucMaxBandwidth;
		}
	}

skip_bw_overwrite:
	if (prAcsReqInfo->eChnlBw > MAX_BW_20MHZ) {
		enum ENUM_CHNL_EXT eSCO;

		eSCO = nicGetSco(prGlueInfo->prAdapter,
				 prAcsReqInfo->eBand,
				 prAcsReqInfo->ucPrimaryCh);

		prAcsReqInfo->ucSecondCh =
			nicGetSecCh(prGlueInfo->prAdapter,
				    prAcsReqInfo->eBand,
				    eSCO,
				    prAcsReqInfo->ucPrimaryCh);
	}

	prAcsReqInfo->ucVhtSeg0 = 0;
	prAcsReqInfo->ucVhtSeg1 = 0;

	switch (prAcsReqInfo->eChnlBw) {
	case MAX_BW_40MHZ:
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (prAcsReqInfo->eBand == BAND_6G)
			prAcsReqInfo->ucVhtSeg0 =
				nicGetS1(prGlueInfo->prAdapter,
					 prAcsReqInfo->eBand,
					 prAcsReqInfo->ucPrimaryCh,
					 VHT_OP_CHANNEL_WIDTH_20_40);
#endif /* CFG_SUPPORT_WIFI_6G */
		break;
	case MAX_BW_80MHZ:
		prAcsReqInfo->ucVhtSeg0 =
			nicGetS1(prGlueInfo->prAdapter,
				 prAcsReqInfo->eBand,
				 prAcsReqInfo->ucPrimaryCh,
				 VHT_OP_CHANNEL_WIDTH_80);
		break;
	case MAX_BW_160MHZ:
		prAcsReqInfo->ucVhtSeg1 =
			nicGetS1(prGlueInfo->prAdapter,
				 prAcsReqInfo->eBand,
				 prAcsReqInfo->ucPrimaryCh,
				 VHT_OP_CHANNEL_WIDTH_160);
		break;
	case MAX_BW_80_80_MHZ:
		prAcsReqInfo->ucVhtSeg0 =
			nicGetS1(prGlueInfo->prAdapter,
				 prAcsReqInfo->eBand,
				 prAcsReqInfo->ucPrimaryCh,
				 VHT_OP_CHANNEL_WIDTH_80);
		prAcsReqInfo->ucVhtSeg1 =
			nicGetS1(prGlueInfo->prAdapter,
				 prAcsReqInfo->eBand,
				 prAcsReqInfo->ucPrimaryCh,
				 VHT_OP_CHANNEL_WIDTH_80P80);
		break;
#if (CFG_SUPPORT_WIFI_6G == 1)
	case MAX_BW_320_1MHZ:
		prAcsReqInfo->ucVhtSeg1 =
			nicGetS1(prGlueInfo->prAdapter,
				 prAcsReqInfo->eBand,
				 prAcsReqInfo->ucPrimaryCh,
				 VHT_OP_CHANNEL_WIDTH_320_1);
		break;
	case MAX_BW_320_2MHZ:
		prAcsReqInfo->ucVhtSeg1 =
			nicGetS1(prGlueInfo->prAdapter,
				 prAcsReqInfo->eBand,
				 prAcsReqInfo->ucPrimaryCh,
				 VHT_OP_CHANNEL_WIDTH_320_2);
		break;
#endif /* CFG_SUPPORT_WIFI_6G */
	default:
		break;
	}

	prAcsReqInfo->fgIsProcessing = FALSE;

	kalP2pIndicateAcsResult(prGlueInfo,
				prAcsReqInfo->ucRoleIdx,
				prAcsReqInfo->eBand,
				prAcsReqInfo->ucPrimaryCh,
				prAcsReqInfo->ucSecondCh,
				prAcsReqInfo->ucVhtSeg0,
				prAcsReqInfo->ucVhtSeg1,
				prAcsReqInfo->eChnlBw,
				prAcsReqInfo->eHwMode);
}

void p2pFunCalAcsChnScores(struct ADAPTER *prAdapter)
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

#if CFG_ENABLE_CSA_BLOCK_SCAN
uint8_t p2pFuncIsCsaBlockScan(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prP2pBssInfo;
	uint8_t ucBssIndex;

	ucBssIndex = p2pFuncGetCsaBssIndex();

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prP2pBssInfo)
		return FALSE;

	return timerPendingTimer(&prP2pBssInfo->rP2pCsaDoneTimer);
}
#endif

enum ENUM_CHNL_SWITCH_POLICY
p2pFunDetermineChnlSwitchPolicy(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		struct RF_CHANNEL_INFO *prNewChannelInfo)
{
	enum ENUM_CHNL_SWITCH_POLICY ePolicy = CHNL_SWITCH_POLICY_CSA;
	struct BSS_INFO *prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	struct LINK *prClientList;

	if (!prBssInfo || !IS_BSS_APGO(prBssInfo))
		return ePolicy;

	prClientList = &prBssInfo->rStaRecOfClientList;
	if (!prClientList || prClientList->u4NumElem <= 0)
		return CHNL_SWITCH_POLICY_NO_CLIENT;

	if (!p2pFuncIsAPMode(prAdapter->rWifiVar.
			prP2PConnSettings[prBssInfo->u4PrivateData]))
		return ePolicy;

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucCsaDeauthClient))
		return ePolicy;

#if (CFG_SUPPORT_APGO_CROSS_BAND_CSA == 1)
	DBGLOG(P2P, INFO, "cross band csa enable\n");
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prNewChannelInfo->eBand == BAND_6G &&
		(prBssInfo->eBand == BAND_2G4 ||
		prBssInfo->eBand == BAND_5G))
		ePolicy = CHNL_SWITCH_POLICY_DEAUTH;
	else if (prBssInfo->eBand == BAND_6G &&
		(prNewChannelInfo->eBand == BAND_2G4 ||
		prNewChannelInfo->eBand == BAND_5G))
		ePolicy = CHNL_SWITCH_POLICY_DEAUTH;
#endif
#else
#if CFG_SEND_DEAUTH_DURING_CHNL_SWITCH
	/* Send deauth frame to clients:
	 * 1. Cross band
	 * 2. BW > 20MHz
	*/
	if (prNewChannelInfo->eBand == BAND_5G ||
#if (CFG_SUPPORT_WIFI_6G == 1)
		prNewChannelInfo->eBand == BAND_6G ||
#endif
		(prBssInfo && prBssInfo->eBand != prNewChannelInfo->eBand))
		ePolicy = CHNL_SWITCH_POLICY_DEAUTH;
#endif /* CFG_SEND_DEAUTH_DURING_CHNL_SWITCH */
#endif /* CFG_SUPPORT_APGO_CROSS_BAND_CSA */

	return ePolicy;
}

void
p2pFunNotifyChnlSwitch(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx,
		enum ENUM_CHNL_SWITCH_POLICY ePolicy,
		struct RF_CHANNEL_INFO *prNewChannelInfo)
{
	struct BSS_INFO *prBssInfo;
	struct LINK *prClientList;
	struct STA_RECORD *prCurrStaRec;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo = NULL;
	uint32_t u4TimeoutMs;

	DBGLOG(P2P, INFO, "bss index: %d, policy: %d\n", ucBssIdx, ePolicy);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo)
		return;
	prClientList = &prBssInfo->rStaRecOfClientList;
	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
		prAdapter, prBssInfo->u4PrivateData);
	switch (ePolicy) {
	case CHNL_SWITCH_POLICY_DEAUTH:
		if (prClientList && prClientList->u4NumElem > 0) {
			LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList,
					rLinkEntry, struct STA_RECORD) {
				struct TIMER *prTimer;

				if (!prCurrStaRec || !prCurrStaRec->fgIsInUse)
					break;

				prTimer = &(prCurrStaRec->rDeauthTxDoneTimer);

				if (!timerPendingTimer(prTimer)) {
					cnmTimerInitTimer(prAdapter,
						prTimer,
						(PFN_MGMT_TIMEOUT_FUNC)
						p2pRoleFsmDeauthTimeout,
						(uintptr_t) prCurrStaRec);
					cnmTimerStartTimer(prAdapter,
						prTimer,
						P2P_DEAUTH_TIMEOUT_TIME_MS);
				}
				p2pFuncDisconnect(prAdapter, prBssInfo,
						prCurrStaRec, TRUE,
						REASON_CODE_DEAUTH_LEAVING_BSS,
						TRUE);
			}
			/* wait for deauth TX done & switch channel */
		} else {
			p2pFunChnlSwitchNotifyDone(prAdapter);
		}
		break;
	case CHNL_SWITCH_POLICY_CSA:
		/* Set CSA IE */
		prAdapter->rWifiVar.ucChannelSwitchMode = 1;
		prAdapter->rWifiVar.ucNewOperatingClass =
			nicChannelInfo2OpClass(prNewChannelInfo);
		prAdapter->rWifiVar.ucNewChannelNumber =
			prNewChannelInfo->ucChannelNum;
		prAdapter->rWifiVar.ucChannelSwitchCount = 5;
		prAdapter->rWifiVar.ucSecondaryOffset =
			rlmGetScoByChnInfo(prAdapter, prNewChannelInfo);
		prAdapter->rWifiVar.ucNewChannelWidth =
			rlmGetVhtOpBwByBssOpBw(prNewChannelInfo->ucChnlBw);
		prAdapter->rWifiVar.ucNewChannelS1 =
			nicFreq2ChannelNum(
				prNewChannelInfo->u4CenterFreq1 * 1000);
		prAdapter->rWifiVar.ucNewChannelS2 = 0;
		p2pFunAbortOngoingScan(prAdapter);

		/* Send Action Frames */
		rlmSendChannelSwitchFrame(prAdapter, prBssInfo);

		/* To prevent race condition, we have to set CSA flags
		 * after all CSA parameters are updated. In this way,
		 * we can guarantee that CSA IE will be and only be
		 * reported once in the beacon.
		 */
		prAdapter->rWifiVar.fgCsaInProgress = TRUE;
		kalP2pIndicateChnlSwitchStarted(prAdapter,
			prBssInfo,
			prNewChannelInfo,
			prAdapter->rWifiVar.ucChannelSwitchCount,
			(prAdapter->rWifiVar.ucChannelSwitchMode == 1),
			FALSE);
#if CFG_ENABLE_CSA_BLOCK_SCAN
		u4TimeoutMs = DEFAULT_P2P_CSA_TIMEOUT_MS;
		u4TimeoutMs += TU_TO_MSEC(prBssInfo->u2BeaconInterval) *
			prAdapter->rWifiVar.ucChannelSwitchCount;
		cnmTimerStopTimer(prAdapter,
				  &(prBssInfo->rP2pCsaDoneTimer));
		cnmTimerStartTimer(prAdapter,
				   &(prBssInfo->rP2pCsaDoneTimer),
				   u4TimeoutMs);
#endif
		/* Update Beacon */
		bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);
		break;
	case CHNL_SWITCH_POLICY_NO_CLIENT:
		p2pFunChnlSwitchNotifyDone(prAdapter);
		break;
	default:
		DBGLOG(P2P, WARN, "invalid policy for channel switch: %d\n",
			ePolicy);
		break;
	}
}

void
p2pFunChnlSwitchNotifyDone(struct ADAPTER *prAdapter)
{
	struct GL_P2P_INFO *prP2PInfo;
	struct BSS_INFO *prBssInfo;
	struct MSG_P2P_CSA_DONE *prP2pCsaDoneMsg;
	uint8_t ucBssIndex;

	if (!prAdapter || !prAdapter->prGlueInfo)
		return;

	/* Check SAP interface */
	ucBssIndex = p2pFuncGetCsaBssIndex();
	if (!IS_BSS_INDEX_VALID(ucBssIndex)) {
		log_dbg(CNM, ERROR, "Csa bss is invalid!\n");
		return;
	}
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		ucBssIndex);
	if (!prBssInfo || !IS_BSS_P2P(prBssInfo)) {
		log_dbg(CNM, ERROR, "No SAP/P2P bss is active when CSA done!\n");
		return;
	}

	prP2pCsaDoneMsg = (struct MSG_P2P_CSA_DONE *) cnmMemAlloc(prAdapter,
			RAM_TYPE_MSG,
			sizeof(*prP2pCsaDoneMsg));

	if (!prP2pCsaDoneMsg) {
		log_dbg(CNM, ERROR, "allocate for prP2pCsaDoneMsg failed!\n");
		return;
	}

	DBGLOG(CNM, INFO, "p2pFuncSwitch Done, ucBssIndex = %d\n",
		prBssInfo->ucBssIndex);

	prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[prBssInfo->u4PrivateData];
	if (!prP2PInfo->fgChannelSwitchReq) {
		DBGLOG(CNM, ERROR, "Drop invalid csa done event!\n");
		return; /* Drop invalid csa done event */
	}

	prP2pCsaDoneMsg->rMsgHdr.eMsgId = MID_CNM_P2P_CSA_DONE;
	prP2pCsaDoneMsg->ucBssIndex = prBssInfo->ucBssIndex;
	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prP2pCsaDoneMsg,
			MSG_SEND_METHOD_BUF);
}

uint8_t p2pFuncIsBufferableMMPDU(struct ADAPTER *prAdapter,
		enum ENUM_P2P_CONNECT_STATE eConnState,
		struct MSDU_INFO *prMgmtTxMsdu)
{
	struct WLAN_MAC_HEADER *prWlanHdr = (struct WLAN_MAC_HEADER *) NULL;
	uint16_t u2TxFrameCtrl;
	uint8_t fgIsBufferableMMPDU = FALSE;

	prWlanHdr = (struct WLAN_MAC_HEADER *)
		((uintptr_t) prMgmtTxMsdu->prPacket +
		MAC_TX_RESERVED_FIELD);

	if (!prWlanHdr) {
		DBGLOG(P2P, ERROR, "prWlanHdr is NULL\n");
		return FALSE;
	}
	u2TxFrameCtrl = prWlanHdr->u2FrameCtrl & MASK_FRAME_TYPE;

	switch (u2TxFrameCtrl) {
	case MAC_FRAME_ACTION:
		/* p2p_device default use band0 to TX,
		 * and band0 may only support 2.4G for some platform.
		 * Set FORCE_TX in case p2p_device TX 5G/6G fail.
		 */
		if (prMgmtTxMsdu->ucBssIndex == prAdapter->ucP2PDevBssIdx) {
			fgIsBufferableMMPDU = FALSE;
			break;
		}

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

void p2pFuncSetAclPolicy(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	enum ENUM_PARAM_CUSTOM_ACL_POLICY ePolicy,
	uint8_t aucAddr[])
{
	struct CMD_SET_ACL_POLICY *prCmdAclPolicy;
	struct BSS_INFO *prBssInfo =
		(struct BSS_INFO *) NULL;

	if (!prAdapter)
		return;

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgSapOffload))
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

	if (!prBssInfo) {
		DBGLOG(P2P, ERROR,
			"prBssInfo is NULL!\n");
		return;
	}

	prCmdAclPolicy = (struct CMD_SET_ACL_POLICY *)
		cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
		sizeof(*prCmdAclPolicy));

	if (!prCmdAclPolicy) {
		DBGLOG(P2P, ERROR,
			"cnmMemAlloc for prCmdAclPolicy failed!\n");
		return;
	}

	prCmdAclPolicy->ucBssIdx = ucBssIdx;
	prCmdAclPolicy->ucPolicy = (uint8_t) ePolicy;

	if (ePolicy == PARAM_CUSTOM_ACL_POLICY_ADD ||
		ePolicy == PARAM_CUSTOM_ACL_POLICY_REMOVE) {
		COPY_MAC_ADDR(prCmdAclPolicy->aucAddr,
			aucAddr);
		DBGLOG(P2P, INFO,
			"[Role%d][%d] Policy:%d, MAC [" MACSTR "]\n",
			prBssInfo->u4PrivateData,
			prCmdAclPolicy->ucBssIdx,
			prCmdAclPolicy->ucPolicy,
			MAC2STR(prCmdAclPolicy->aucAddr));
	} else
		DBGLOG(P2P, INFO,
			"[Role%d][%d] Policy:%d\n",
			prBssInfo->u4PrivateData,
			prCmdAclPolicy->ucBssIdx,
			prCmdAclPolicy->ucPolicy);

	wlanSendSetQueryCmd(prAdapter,
		CMD_ID_SET_ACL_POLICY,
		TRUE,
		FALSE,
		FALSE,
		NULL,
		NULL,
		sizeof(*prCmdAclPolicy),
		(uint8_t *) prCmdAclPolicy, NULL, 0);

	cnmMemFree(prAdapter, prCmdAclPolicy);
}

#if CFG_AP_80211KVR_INTERFACE
void p2pFunMulAPAgentBssStatusNotification(
		struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
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
	i4Ret = kalGetMulAPIfIdx(prAdapter->prGlueInfo, 1,
				&prBssReport->uIfIndex);
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

uint32_t p2pFuncStoreFilsInfo(struct ADAPTER *prAdapter,
	struct MSG_P2P_FILS_DISCOVERY_UPDATE *prMsg,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prSpecBssInfo;
	struct P2P_FILS_DISCOVERY_INFO *prFilsInfo;

	if (!prMsg || !prMsg->u4Length || !prMsg->prBuffer)
		return WLAN_STATUS_INVALID_DATA;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prP2pBssInfo)
		return WLAN_STATUS_INVALID_DATA;

	prSpecBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prP2pBssInfo->u4PrivateData];
	prFilsInfo = &prSpecBssInfo->rFilsInfo;

	DBGLOG(P2P, INFO, "[FILS_DISC] bss[%d]max[%d]min[%d]\n",
		prP2pBssInfo->ucBssIndex,
		prMsg->u4MaxInterval,
		prMsg->u4MinInterval);
	DBGLOG_MEM8(P2P, TRACE,
		prMsg->prBuffer,
		prMsg->u4Length);
	prFilsInfo->u4MaxInterval = prMsg->u4MaxInterval;
	prFilsInfo->u4MinInterval = prMsg->u4MinInterval;
	prFilsInfo->u4Length = prMsg->u4Length;
	kalMemCopy(prFilsInfo->aucIEBuf,
		   prMsg->prBuffer,
		   prMsg->u4Length);

	prFilsInfo->fgValid = TRUE;

	return WLAN_STATUS_SUCCESS;
}

void p2pFuncClearFilsInfo(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prSpecBssInfo;
	struct P2P_FILS_DISCOVERY_INFO *prFilsInfo;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prP2pBssInfo)
		return;

	DBGLOG(P2P, INFO, "ucBssIndex: %d\n",
		ucBssIndex);

	prSpecBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prP2pBssInfo->u4PrivateData];
	prFilsInfo = &prSpecBssInfo->rFilsInfo;

	prFilsInfo->fgValid = FALSE;
	prFilsInfo->u4Length = 0;
}

uint32_t p2pFuncStoreUnsolProbeInfo(struct ADAPTER *prAdapter,
	struct MSG_P2P_UNSOL_PROBE_UPDATE *prMsg,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prSpecBssInfo;
	struct P2P_UNSOL_PROBE_RESP_INFO *prUnsolProbeInfo;

	if (!prMsg || !prMsg->u4Length || !prMsg->prBuffer)
		return WLAN_STATUS_INVALID_DATA;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prP2pBssInfo)
		return WLAN_STATUS_INVALID_DATA;

	prSpecBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prP2pBssInfo->u4PrivateData];
	prUnsolProbeInfo = &prSpecBssInfo->rUnsolProbeInfo;

	DBGLOG(P2P, INFO, "[UNSOL_PROBE] bss[%d]interval[%d]\n",
		prP2pBssInfo->ucBssIndex,
		prMsg->u4Interval);
	DBGLOG_MEM8(P2P, TRACE,
		prMsg->prBuffer,
		prMsg->u4Length);
	prUnsolProbeInfo->u4Interval = prMsg->u4Interval;
	prUnsolProbeInfo->u4Length = prMsg->u4Length;
	kalMemCopy(prUnsolProbeInfo->aucIEBuf,
		   prMsg->prBuffer,
		   prMsg->u4Length);

	prUnsolProbeInfo->fgValid = TRUE;

	return WLAN_STATUS_SUCCESS;
}

void p2pFuncClearUnsolProbeInfo(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prSpecBssInfo;
	struct P2P_UNSOL_PROBE_RESP_INFO *prUnsolProbeInfo;

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prP2pBssInfo)
		return;

	DBGLOG(P2P, INFO, "ucBssIndex: %d\n",
		ucBssIndex);

	prSpecBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo
			[prP2pBssInfo->u4PrivateData];
	prUnsolProbeInfo = &prSpecBssInfo->rUnsolProbeInfo;

	prUnsolProbeInfo->fgValid = FALSE;
	prUnsolProbeInfo->u4Length = 0;
}

uint32_t p2pFuncCalculateP2p_IELenForOwe(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, struct STA_RECORD *prStaRec)
{
	struct WIFI_VAR *prWifiVar;
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo;

	if (!prAdapter)
		return 0;

	prWifiVar = &prAdapter->rWifiVar;
	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prP2pBssInfo == NULL ||
	    prP2pBssInfo->u4RsnSelectedAKMSuite != RSN_AKM_SUITE_OWE) {
		return 0;
	}

	prP2pSpecBssInfo = prWifiVar->prP2pSpecificBssInfo[
		prP2pBssInfo->u4PrivateData];
	if (prP2pSpecBssInfo == NULL ||
	    prP2pSpecBssInfo->ucDHIELen == 0 ||
	    prP2pSpecBssInfo->pucDHIEBuf == NULL) {
		return 0;
	}

	return prP2pSpecBssInfo->ucDHIELen;
}

void p2pFuncGenerateP2p_IEForOwe(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct WIFI_VAR *prWifiVar;
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo;
	uint8_t *pucBuf;

	if (!prAdapter || !prMsduInfo)
		return;

	prWifiVar = &prAdapter->rWifiVar;
	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (prP2pBssInfo == NULL ||
	    prP2pBssInfo->u4RsnSelectedAKMSuite != RSN_AKM_SUITE_OWE) {
		return;
	}

	prP2pSpecBssInfo = prWifiVar->prP2pSpecificBssInfo[
		prP2pBssInfo->u4PrivateData];
	if (prP2pSpecBssInfo == NULL ||
	    prP2pSpecBssInfo->ucDHIELen == 0 ||
	    prP2pSpecBssInfo->pucDHIEBuf == NULL) {
		return;
	}

	pucBuf = (uint8_t *)((uintptr_t) prMsduInfo->prPacket +
			     (uint32_t) prMsduInfo->u2FrameLength);

	kalMemCopy(pucBuf,
		   prP2pSpecBssInfo->pucDHIEBuf,
		   prP2pSpecBssInfo->ucDHIELen);
	prMsduInfo->u2FrameLength += prP2pSpecBssInfo->ucDHIELen;
}

u_int8_t p2pFuncIsLteSafeChnl(enum ENUM_BAND eBand, uint8_t ucChnlNum,
				 uint32_t *pau4SafeChnl)
{
	uint32_t u4SafeChInfo_2g = BITS(0, 31);
	uint32_t u4SafeChInfo_5g_0 = BITS(0, 31);
	uint32_t u4SafeChInfo_5g_1 = BITS(0, 31);
	uint32_t u4SafeChInfo_6g = BITS(0, 31);

	if (pau4SafeChnl) {
		u4SafeChInfo_2g = pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_2G4];
		u4SafeChInfo_5g_0 = pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_5G_0];
		u4SafeChInfo_5g_1 = pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_5G_1];
		u4SafeChInfo_6g = pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_6G];
	}

	if (eBand == BAND_2G4 && ucChnlNum <= 14) {
		if (u4SafeChInfo_2g & BIT(ucChnlNum))
			return TRUE;
	} else if (eBand == BAND_5G && ucChnlNum >= 36 && ucChnlNum <= 144) {
		if (u4SafeChInfo_5g_0 & BIT((ucChnlNum - 36) / 4))
			return TRUE;
	} else if (eBand == BAND_5G && ucChnlNum >= 149 && ucChnlNum <= 181) {
		if (u4SafeChInfo_5g_1 & BIT((ucChnlNum - 149) / 4))
			return TRUE;
#if (CFG_SUPPORT_WIFI_6G == 1)
	} else if (eBand == BAND_6G) {
		if (u4SafeChInfo_6g & BIT((ucChnlNum - 5) / 16))
			return TRUE;
#endif
	}

	return FALSE;
}

#if CFG_SUPPORT_CCM
u_int8_t p2pFuncIsPreferWfdAa(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prCsaBss)
{
	struct WFD_CFG_SETTINGS *prWfdCfgSettings =
		&(prAdapter->rWifiVar.rWfdConfigureSettings);

	return (ccmIsPreferAA(prAdapter, prCsaBss) &&
		prWfdCfgSettings->ucWfdEnable == 1);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check if all client support CSA (Spec mgmt & MTK Oui).
 */
/*----------------------------------------------------------------------------*/
static u_int8_t
p2pFuncIsAllClientSupportCsa(struct ADAPTER *prAdapter,
			     struct BSS_INFO *prBssInfo)
{
	struct LINK *prClientList;
	struct STA_RECORD *prStaRec;

	prClientList = &prBssInfo->rStaRecOfClientList;
	LINK_FOR_EACH_ENTRY(prStaRec, prClientList,
			    rLinkEntry, struct STA_RECORD) {
		if (!prStaRec)
			break;

		if (!prStaRec->fgIsInUse)
			continue;

		/* assume only MTK device support CSA now */
		if (!prStaRec->fgIsSupportCsa) {
			DBGLOG(P2P, TRACE,
			       "peer not support CSA, starec idx:%u, mac:"
			       MACSTR "\n",
			       prStaRec->ucIndex,
			       MAC2STR(prStaRec->aucMacAddr));
			return FALSE;
		}

		if (!(prStaRec->u2CapInfo & CAP_INFO_SPEC_MGT)) {
			DBGLOG(P2P, TRACE,
			       "peer not support Spec Mgmt, starec idx:%u, mac:"
			       MACSTR ", cap:0x%x\n",
			       prStaRec->ucIndex,
			       MAC2STR(prStaRec->aucMacAddr),
			       prStaRec->u2CapInfo);
			return FALSE;
		}
	}

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check if all client support target op class.
 *        For 6G CSA should have this IE.
 */
/*----------------------------------------------------------------------------*/
static u_int8_t
p2pFuncIsAllClientSupportOpClass(struct ADAPTER *prAdapter,
				 struct BSS_INFO *prBssInfo,
				 uint8_t ucOpClass)
{
	struct LINK *prClientList;
	struct STA_RECORD *prStaRec;
	uint32_t u4OpClassBits;

	prClientList = &prBssInfo->rStaRecOfClientList;
	LINK_FOR_EACH_ENTRY(prStaRec, prClientList,
			    rLinkEntry, struct STA_RECORD) {
		if (!prStaRec)
			break;

		if (!prStaRec->fgIsInUse)
			continue;

		u4OpClassBits = prStaRec->u4SupportedOpClassBits;

		if (ucOpClass >= 81 && ucOpClass <= 84 &&
		    (u4OpClassBits & BIT(ucOpClass - 81)))
			continue;
		if (ucOpClass >= 115 && ucOpClass <= 136 &&
		    (u4OpClassBits & BIT(ucOpClass - 115 + 4)))
			continue;
		if (ucOpClass >= 180 && ucOpClass <= 183 &&
		    (u4OpClassBits & BIT(ucOpClass - 180 + 4 + 22)))
			continue;

		DBGLOG(P2P, TRACE,
		       "peer not support op class %u, starec idx:%u, mac:"
		       MACSTR ", opClassBit:0x%x\n",
		       ucOpClass, prStaRec->ucIndex,
		       MAC2STR(prStaRec->aucMacAddr), u4OpClassBits);
		return FALSE;
	}

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check if all client support target channel.
 *        For 6G channel should use supported op class IE instead of this.
 */
/*----------------------------------------------------------------------------*/
static u_int8_t
p2pFuncIsAllClientSupportCh(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo,
			    enum ENUM_BAND eBand, uint32_t u4Ch)
{
	struct LINK *prClientList;
	struct STA_RECORD *prStaRec;
	uint16_t u2SupCh_2g = 0;
	uint32_t u4SupCh_5g_0 = 0;
	uint16_t u2SupCh_5g_1 = 0;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G) {
		DBGLOG(P2P, WARN,
		       "6G should use sup op class instead of sup ch\n");
		return FALSE;
	}
#endif

	prClientList = &prBssInfo->rStaRecOfClientList;
	LINK_FOR_EACH_ENTRY(prStaRec, prClientList,
			    rLinkEntry, struct STA_RECORD) {
		if (!prStaRec)
			break;

		if (!prStaRec->fgIsInUse)
			continue;

		u2SupCh_2g = prStaRec->u2SupportedChnlBits_2g;
		u4SupCh_5g_0 = prStaRec->u4SupportedChnlBits_5g_0;
		u2SupCh_5g_1 = prStaRec->u2SupportedChnlBits_5g_1;

		if (eBand == BAND_2G4 && (u2SupCh_2g & BIT(u4Ch)))
			continue;
		if (eBand == BAND_5G) {
			if (u4SupCh_5g_0 & BIT((u4Ch - 36) / 4))
				continue;
			if (u2SupCh_5g_1 & BIT((u4Ch - 149) / 4))
				continue;
		}

		DBGLOG(P2P, TRACE,
		       "peer not support ch:%u, starec idx:%u, mac:" MACSTR
		       ", 2gChBit:0x%x, 5gChBit_0:0x%x, 5gChBit_1:0x%x\n",
		       u4Ch, prStaRec->ucIndex,
		       MAC2STR(prStaRec->aucMacAddr),
		       u2SupCh_2g, u4SupCh_5g_0, u2SupCh_5g_1);
		return FALSE;
	}

	return TRUE;
}

enum ENUM_CSA_STATUS p2pFuncIsCsaAllowed(struct ADAPTER *prAdapter,
			       struct BSS_INFO *prBssInfo,
			       uint32_t u4TargetCh,
			       enum ENUM_BAND eTargetBand)
{
	struct RF_CHANNEL_INFO rRfChnlInfo;
	uint8_t ucTargetOpClass;
	enum ENUM_CSA_STATUS rStatus = CSA_STATUS_SUCCESS;
	u_int8_t fgDfsChannel = rlmDomainIsDfsChnls(prAdapter, u4TargetCh);
	u_int8_t fgDfsDisAllowed = TRUE;

	/*
	 * Allow dfs channel for p2p GO:
	 *     1. sta connected on dfs channel
	 *     2. p2p GO connected with clients
	 */
	if (fgDfsChannel && IS_BSS_APGO(prBssInfo) &&
	    !p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[
			     prBssInfo->u4PrivateData]) &&
	    prBssInfo->rStaRecOfClientList.u4NumElem > 0) {
		kalMemZero(&rRfChnlInfo, sizeof(rRfChnlInfo));
		rRfChnlInfo.ucChannelNum = u4TargetCh;
		rRfChnlInfo.eBand = eTargetBand;
		rRfChnlInfo.ucChnlBw = prAdapter->rWifiVar.ucP2p5gBandwidth;
		rRfChnlInfo.u2PriChnlFreq =
			nicChannelNum2Freq(u4TargetCh,
					   eTargetBand) / 1000;
		rRfChnlInfo.u4CenterFreq1 =
			nicGetS1Freq(prAdapter,
				     eTargetBand,
				     u4TargetCh,
				     rRfChnlInfo.ucChnlBw);
		rRfChnlInfo.u4CenterFreq2 = 0;

		if (wlanDfsChannelsAllowdBySta(prAdapter,
					       &rRfChnlInfo))
			fgDfsDisAllowed = FALSE;
	}

	if (fgDfsChannel && fgDfsDisAllowed)
		rStatus = CSA_STATUS_DFS_NOT_SUP;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (eTargetBand == BAND_6G && !IS_6G_PSC_CHANNEL(u4TargetCh))
		rStatus = CSA_STATUS_NON_PSC_NOT_SUP;
	else if (eTargetBand == BAND_6G &&
	    !rsnKeyMgmtSae(prBssInfo->u4RsnSelectedAKMSuite)) {
		DBGLOG(CCM, WARN, "Skip CSA to 6G if auth type not SAE\n");
		rStatus = CSA_STATUS_NON_SAE_NOT_SUP;
	}
#endif
	/* SAP should not consider the capability of peers */
	else if (IS_BSS_APGO(prBssInfo) &&
	    !p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[
				    prBssInfo->u4PrivateData])) {
		/* prepare chnl info
		 * TODO: re-design nicGetS1Freq
		 */
		rRfChnlInfo.ucChannelNum = u4TargetCh;
		rRfChnlInfo.eBand = eTargetBand;
		/* always use BW20 to check the minimum capability of peer */
		rRfChnlInfo.ucChnlBw = MAX_BW_20MHZ;
		rRfChnlInfo.u2PriChnlFreq =
			nicChannelNum2Freq(u4TargetCh, eTargetBand) / 1000;
		rRfChnlInfo.u4CenterFreq1 = rRfChnlInfo.u2PriChnlFreq;
		rRfChnlInfo.u4CenterFreq2 = 0;
		rRfChnlInfo.fgDFS = FALSE;

		ucTargetOpClass = nicChannelInfo2OpClass(&rRfChnlInfo);
		if (!p2pFuncIsAllClientSupportCsa(prAdapter, prBssInfo))
			rStatus = CSA_STATUS_PEER_NOT_SUP_CSA;
		else if (!p2pFuncIsAllClientSupportOpClass(prAdapter,
					prBssInfo, ucTargetOpClass) &&
			 !p2pFuncIsAllClientSupportCh(prAdapter, prBssInfo,
					eTargetBand, u4TargetCh))
			rStatus = CSA_STATUS_PEER_NOT_SUP_CH;
	}

	DBGLOG(P2P, INFO, "Csa %s, status=%u",
	       (rStatus == CSA_STATUS_SUCCESS) ? "allowed" : "not allowed",
	       rStatus);
	return rStatus;
}
#endif /* CFG_ENABLE_WIFI_DIRECT */
