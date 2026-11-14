// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "p2p_link.c"
 *  \brief
 */

#include "precomp.h"

#if CFG_ENABLE_WIFI_DIRECT
#if (CFG_SUPPORT_802_11BE_MLO == 1)

struct MLD_BSS_INFO *gprP2pMldBssInfo;
uint8_t gucRemainMldBssLinkNum;

struct MLD_BSS_INFO *p2pMldBssInit(struct ADAPTER *prAdapter,
	const uint8_t aucIntfMac[],
	u_int8_t fgIsApMode)
{
	struct MLD_BSS_INFO *prMldbss = NULL;

#if (CFG_SUPPORT_P2P_SET_ML_BSS_NUM == 1)
	if (gucRemainMldBssLinkNum > 0) {
		if (gprP2pMldBssInfo == NULL) {
			DBGLOG(INIT, ERROR, "MldBss not allocated yet.\n");
			return NULL;
		}

		prMldbss = gprP2pMldBssInfo;
		gucRemainMldBssLinkNum--;
		DBGLOG(INIT, TRACE, "Use global MldBss[%u]\n",
			prMldbss->ucGroupMldId);
	} else {
		prMldbss = mldBssAlloc(prAdapter, aucIntfMac);
		if (prMldbss == NULL) {
			DBGLOG(INIT, WARN, "Allocate MldBss failed\n");
			return NULL;
		}
		if (fgIsApMode == FALSE)
			gprP2pMldBssInfo = prMldbss;
		DBGLOG(INIT, TRACE, "Allocate MldBss[%u] for %s\n",
			prMldbss->ucGroupMldId,
			fgIsApMode ? "SAP" : "P2P");
	}
#else
	if (fgIsApMode == FALSE &&
	    mldIsMultiLinkEnabled(prAdapter, NETWORK_TYPE_P2P, fgIsApMode)) {
		if (gprP2pMldBssInfo == NULL) {
			DBGLOG(INIT, TRACE, "\n");
			gprP2pMldBssInfo = mldBssAlloc(prAdapter,
				aucIntfMac);
		}

		prMldbss = gprP2pMldBssInfo;
	} else {
		prMldbss = mldBssAlloc(prAdapter, aucIntfMac);
	}
#endif

	return prMldbss;
}

void p2pMldBssUninit(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldbss)
{
	if (prMldbss != NULL &&
	    prMldbss->rBssList.u4NumElem == 0) {
		if (gprP2pMldBssInfo == prMldbss)
			gprP2pMldBssInfo = NULL;

		mldBssFree(prAdapter, prMldbss);
	}
}

void p2pLinkInitGcOtherLinks(struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkNum)
{
	struct MLD_BSS_INFO *prMldBssInfo;
	uint8_t i;

	if (!prAdapter || p2pGetMode() != RUNNING_P2P_DEV_MODE)
		return;

	if (ucLinkNum > prAdapter->rWifiVar.ucP2pMldLinkMax) {
		DBGLOG(P2P, INFO,
			"Reduce connection link num from %u to %u\n",
			ucLinkNum,
			prAdapter->rWifiVar.ucP2pMldLinkMax);
		ucLinkNum = prAdapter->rWifiVar.ucP2pMldLinkMax;
	}

	prMldBssInfo = prP2pRoleFsmInfo->prP2pMldBssInfo;
	for (i = 0; i < ucLinkNum; i++) {
		uint8_t aucLinkAddr[MAC_ADDR_LEN];

		if (p2pGetLinkBssInfo(prP2pRoleFsmInfo, i))
			continue;

		nicApplyLinkAddress(prAdapter,
				    prMldBssInfo->aucOwnMldAddr,
				    aucLinkAddr,
				    i);
		p2pRoleFsmInitLink(prAdapter, prP2pRoleFsmInfo,
				   aucLinkAddr, prMldBssInfo->ucGroupMldId,
				   i);
	}
}

void p2pLinkUninitGcOtherLinks(struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo)
{
	struct BSS_INFO *prP2pBssInfo;
	uint8_t i;

	if (!prAdapter || p2pGetMode() != RUNNING_P2P_DEV_MODE)
		return;

	for (i = 0;
	     i < prAdapter->rWifiVar.ucP2pMldLinkMax;
	     i++) {
		prP2pBssInfo = p2pGetLinkBssInfo(prP2pRoleFsmInfo, i);
		if (!prP2pBssInfo)
			continue;

		p2pRoleFsmUninitLink(prAdapter, prP2pRoleFsmInfo, prP2pBssInfo);
	}
}

static uint8_t *p2pLinkGetAuthSaeCommitIes(struct WLAN_AUTH_FRAME *prAuthFrame,
	uint16_t u2IELength, u_int8_t fgIsApMode)
{
	uint8_t *pucIE = prAuthFrame->aucInfoElem;
	uint16_t u2Group, u2Offset = 0;

	if (fgIsApMode &&
	    prAuthFrame->u2StatusCode != WLAN_STATUS_SAE_HASH_TO_ELEMENT)
		return NULL;

	if (u2IELength < 2)
		return NULL;

	/* SAE H2E commit message (group, scalar, FFE) */
	WLAN_GET_FIELD_16(&prAuthFrame->aucInfoElem[0], &u2Group);
	switch (u2Group) {
	case 19:
		u2Offset = 2 + 32 + 32 * 2;
		break;
	case 20:
		u2Offset = 2 + 48 + 48 * 2;
		break;
	case 21:
		u2Offset = 2 + 66 + 66 * 2;
		break;
	default:
		DBGLOG(AAA, WARN,
			"Unsupported group(%u)\n",
			u2Group);
		return NULL;
	}

	if (u2Offset > u2IELength) {
		DBGLOG(AAA, WARN, "Invalid offset %u, group %u\n",
			u2Offset, u2Group);
		return NULL;
	}

	return pucIE + u2Offset;
}

uint32_t p2pLinkProcessRxAuthReqFrame(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct SW_RFB *prSwRfb)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct WLAN_AUTH_FRAME *prAuthFrame = NULL;
	struct MULTI_LINK_INFO rMlInfo;
	struct MULTI_LINK_INFO *prMlInfo = &rMlInfo;
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct MLD_STA_RECORD *prMldStarec = NULL;
	uint8_t *pucIE = NULL;
	uint16_t u2IELength;
	const uint8_t *ml = NULL;
	uint16_t u2RxFrameCtrl;
	u_int8_t fgMldType;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	u_int8_t fgIsApMode;

	if (IS_FEATURE_DISABLED(prWifiVar->ucEnableMlo))
		goto exit;

	if (!prSwRfb || !prBssInfo || !prStaRec || !prStaRec->fgIsInUse) {
		DBGLOG(AAA, WARN,
			"Invalid parameters swrfb=%p, bss=%p, starec=%p, used=%d, skip!\n",
			prSwRfb, prBssInfo, prStaRec,
			prStaRec ? prStaRec->fgIsInUse : -1);
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	if (prSwRfb->u2PacketLen <
			(uint16_t) OFFSET_OF(struct WLAN_AUTH_FRAME,
			aucInfoElem[0])) {
		DBGLOG(AAA, WARN,
			"Invalid packet length (%u)\n",
			prSwRfb->u2PacketLen);
		DBGLOG_MEM8(AAA, WARN, prSwRfb->pvHeader,
			prSwRfb->u2PacketLen);
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	prAuthFrame = (struct WLAN_AUTH_FRAME *)prSwRfb->pvHeader;

	u2RxFrameCtrl = prAuthFrame->u2FrameCtrl;

	u2RxFrameCtrl &= MASK_FRAME_TYPE;

	if (u2RxFrameCtrl != MAC_FRAME_AUTH) {
		DBGLOG(AAA, WARN, "Incorrect frame type, failure!\n");
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	fgIsApMode = IS_BSS_AP(prAdapter, prBssInfo);
	u2IELength = prSwRfb->u2PacketLen -
		(uint16_t) OFFSET_OF(struct WLAN_AUTH_FRAME,
		aucInfoElem[0]);
	if (u2IELength > 0) {
		pucIE = prAuthFrame->aucInfoElem;
		if (prAuthFrame->u2AuthAlgNum == AUTH_ALGORITHM_NUM_SAE) {
			DBGLOG_MEM8(AAA, TRACE, prSwRfb->pvHeader,
				prSwRfb->u2PacketLen);
			if (prAuthFrame->u2AuthTransSeqNo == 1)
				pucIE = p2pLinkGetAuthSaeCommitIes(prAuthFrame,
					u2IELength, fgIsApMode);
			else
				goto exit;
		}
	}
	if (!pucIE)
		goto exit;

	ml = mldFindMlIE(pucIE, u2IELength, ML_CTRL_TYPE_BASIC);
	if (!ml)
		goto exit;

	mldParseBasicMlIE(prMlInfo, ml,
			IE_SIZE(ml), /* no need fragment */
			prAuthFrame->aucBSSID,
			u2RxFrameCtrl,
			__func__);
	if (!prMlInfo->ucValid) {
		DBGLOG(AAA, ERROR, "Invalid mld_info, reject!\n");
		u4Status = WLAN_STATUS_NOT_SUPPORTED;
		goto exit;
	}

	fgMldType = mldCheckMldType(prAdapter, pucIE, u2IELength);
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	prMldStarec = mldStarecGetByMldAddr(prAdapter,
		prMldBssInfo, prMlInfo->aucMldAddr);
	if (!prMldStarec) {
		struct ML_INFO rMlInfo = {0};

		COPY_MAC_ADDR(rMlInfo.aucMldAddr, prMlInfo->aucMldAddr);
		rMlInfo.u2EmlCap = prMlInfo->u2EmlCap;
		rMlInfo.u2MldCap = prMlInfo->u2MldCap;
		rMlInfo.fgMldType = fgMldType;

		prMldStarec = mldStarecAlloc(prAdapter, prMldBssInfo, &rMlInfo);
		if (!prMldStarec) {
			DBGLOG(AAA, ERROR, "Can't alloc mldstarec!\n");
			u4Status = WLAN_STATUS_FAILURE;
			goto exit;
		}
		mldStarecRegister(prAdapter, prMldStarec, prStaRec,
			prBssInfo->ucLinkId);
		mldStarecSetSetupIdx(prAdapter, prStaRec);
	}

exit:
	return u4Status;
}

uint32_t p2pLinkProcessRxAssocReqFrame(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStaRec,
	struct SW_RFB *prSwRfb,
	uint16_t *pu2StatusCode)
{
	struct WLAN_ASSOC_REQ_FRAME *prFrame = NULL;
	struct MLD_STA_RECORD *prMldStarec = NULL;
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct STA_RECORD *prCurr;
	struct LINK *prStarecList = NULL;
	struct MULTI_LINK_INFO rMlInfo;
	struct MULTI_LINK_INFO *prMlInfo = &rMlInfo;
	uint8_t *pucIE;
	uint16_t u2IELength;
	const uint8_t *ml;
	uint16_t u2RxFrameCtrl;
	uint8_t i;

	if (!prAdapter || !prSwRfb || !prBssInfo) {
		DBGLOG(AAA, WARN, "Invalid parameters, ignore pkt!\n");
		return WLAN_STATUS_FAILURE;
	}

	prFrame = (struct WLAN_ASSOC_REQ_FRAME *)prSwRfb->pvHeader;

	u2RxFrameCtrl = prFrame->u2FrameCtrl;

	u2RxFrameCtrl &= MASK_FRAME_TYPE;

	if (u2RxFrameCtrl != MAC_FRAME_ASSOC_REQ &&
		u2RxFrameCtrl != MAC_FRAME_REASSOC_REQ) {
		DBGLOG(AAA, WARN, "Incorrect frame type, ignore pkt!\n");
		return WLAN_STATUS_FAILURE;
	}

	if (u2RxFrameCtrl == MAC_FRAME_REASSOC_REQ) {
		u2IELength = prSwRfb->u2PacketLen -
		    (uint16_t)
		    OFFSET_OF(struct WLAN_REASSOC_REQ_FRAME,
		    aucInfoElem[0]);
		pucIE = ((struct WLAN_REASSOC_REQ_FRAME *)prFrame)->aucInfoElem;
	} else {
		u2IELength = prSwRfb->u2PacketLen -
			(uint16_t)
			OFFSET_OF(struct WLAN_ASSOC_REQ_FRAME,
			aucInfoElem[0]);
		pucIE = prFrame->aucInfoElem;
	}

	ml = mldFindMlIE(pucIE, u2IELength, ML_CTRL_TYPE_BASIC);
	if (ml) {
		mldParseBasicMlIE(prMlInfo, ml, pucIE + u2IELength - ml,
			prFrame->aucBSSID, u2RxFrameCtrl, __func__);
	} else {
		DBGLOG(AAA, INFO, "no ml ie\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (!prMlInfo->ucValid) {
		DBGLOG(AAA, ERROR, "Invalid mld_info, reject!\n");
		*pu2StatusCode = STATUS_CODE_DENIED_EXISTING_MLD_ASSOC;
		return WLAN_STATUS_SUCCESS;
	}

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	prMldStarec = mldStarecGetByMldAddr(prAdapter,
		prMldBssInfo, prMlInfo->aucMldAddr);
	if (!prMldStarec) {
		DBGLOG(AAA, ERROR, "mld sta search failed, addr="MACSTR"\n",
			MAC2STR(prMlInfo->aucMldAddr));
		*pu2StatusCode = STATUS_CODE_DENIED_EXISTING_MLD_ASSOC;
		return WLAN_STATUS_SUCCESS;
	}

	if (prMlInfo->ucProfNum == 0) {
		DBGLOG(AAA, INFO, "ml ie ["MACSTR"] without links\n",
			MAC2STR(prMlInfo->aucMldAddr));
		return WLAN_STATUS_SUCCESS;
	}

	prStarecList = &prMldStarec->rStarecList;
	for (i = 0; i < prMlInfo->ucProfNum; i++) {
		struct STA_PROFILE *prProfiles =
			&prMlInfo->rStaProfiles[i];
		struct BSS_INFO *bss = mldGetBssInfoByLinkID(prAdapter,
			prMldBssInfo, prProfiles->ucLinkId, FALSE);
		uint8_t found = FALSE;

		DBGLOG(AAA, INFO,
			"%d/%d profile: %d, mac: " MACSTR "\n",
			i + 1, prMlInfo->ucProfNum, prProfiles->ucLinkId,
			MAC2STR(prProfiles->aucLinkAddr));

		if (bss == NULL) {
			DBGLOG(AAA, WARN, "wrong link id(%d)\n",
				prProfiles->ucLinkId);
			continue;
		}

		DBGLOG(AAA, INFO,
			"[link%d] bss: %d, mac: " MACSTR
			", bssid: " MACSTR "\n",
			prProfiles->ucLinkId,
			bss->ucBssIndex,
			MAC2STR(bss->aucOwnMacAddr),
			MAC2STR(bss->aucBSSID));

		LINK_FOR_EACH_ENTRY(prCurr, prStarecList,
				rLinkEntryMld, struct STA_RECORD) {
			if (EQUAL_MAC_ADDR(prCurr->aucMacAddr,
				prProfiles->aucLinkAddr)) {
				found = TRUE;
				break;
			}
		}
		/* starec not found */
		if (!found) {
			prCurr = cnmStaRecAlloc(prAdapter,
				STA_TYPE_P2P_GC,
				bss->ucBssIndex,
				prProfiles->aucLinkAddr);
			if (!prCurr) {
				DBGLOG(AAA, WARN,
					"StaRec Full. (%d)\n", CFG_STA_REC_NUM);
				continue;
			}

			prCurr->u2BSSBasicRateSet = bss->u2BSSBasicRateSet;
			prCurr->u2DesiredNonHTRateSet = RATE_SET_ERP_P2P;
			prCurr->u2OperationalRateSet = RATE_SET_ERP_P2P;
			prCurr->ucPhyTypeSet = PHY_TYPE_SET_802_11GN;
			nicTxUpdateStaRecDefaultRate(prAdapter, prCurr);
			cnmStaRecChangeState(prAdapter, prCurr, STA_STATE_1);
			prCurr->eStaType = STA_TYPE_P2P_GC;
			prCurr->u2StatusCode = STATUS_CODE_SUCCESSFUL;
			prCurr->ucJoinFailureCount = 0;

			/* ml link info */
			mldStarecRegister(prAdapter, prMldStarec, prCurr,
				prProfiles->ucLinkId);
		}
	}

	/* make sure all links are assigned link id */
	LINK_FOR_EACH_ENTRY(prCurr, prStarecList,
			rLinkEntryMld, struct STA_RECORD) {
		if (prCurr->ucLinkId == MLD_LINK_ID_NONE) {
			DBGLOG(AAA, WARN, "sta%d " MACSTR " no link id\n",
				prCurr->ucWlanIndex,
				MAC2STR(prCurr->aucMacAddr));
			return WLAN_STATUS_FAILURE;
		}
	}

	return WLAN_STATUS_SUCCESS;
}

static uint32_t p2pLinkGet2ndLinkFreqByOwnPref(struct ADAPTER *prAdapter,
	enum ENUM_BAND eMainLinkBand,
	uint32_t u4MainLinkFreq,
	uint32_t *u4PreferFreq)
{
	struct BSS_INFO *prBssList[MAX_BSSID_NUM] = { 0 };
	struct RF_CHANNEL_INFO arChnlList[MAX_PER_BAND_CHN_NUM] = { { 0 } };
	uint32_t *pu4FreqList = NULL, *pu4FreqWhiteList = NULL;
	uint32_t u4FreqListNum = 0;
	uint8_t ucFreqWhiteListNum = 0, ucNum2gBss, ucNum5gBss, ucNum6gBss = 0;
	uint8_t i, ucChnlNum = 0;
	uint32_t u4Status;

	*u4PreferFreq = 0;

	pu4FreqWhiteList = (uint32_t *)kalMemZAlloc(
		MAX_CHN_NUM * sizeof(uint32_t),
		VIR_MEM_TYPE);
	if (!pu4FreqWhiteList) {
		DBGLOG(P2P, ERROR, "alloc white freq list failed\n");
		goto exit;
	}

	ucNum2gBss = bssGetAliveBssByBand(prAdapter, BAND_2G4, prBssList);
	ucNum5gBss = bssGetAliveBssByBand(prAdapter, BAND_5G, prBssList);
#if (CFG_SUPPORT_WIFI_6G == 1)
	ucNum6gBss = bssGetAliveBssByBand(prAdapter, BAND_6G, prBssList);
#endif

	if (eMainLinkBand == BAND_2G4 &&
	    (ucNum5gBss > 0 || ucNum6gBss > 0)) {
		rlmDomainGetChnlList(prAdapter, BAND_5G, TRUE,
				     MAX_5G_BAND_CHN_NUM,
				     &ucChnlNum, arChnlList);
		for (i = 0; i < ucChnlNum; i++) {
			pu4FreqWhiteList[ucFreqWhiteListNum++] =
				nicChannelNum2Freq(
					arChnlList[i].ucChannelNum,
					arChnlList[i].eBand) / 1000;
		}

#if (CFG_SUPPORT_WIFI_6G == 1)
		rlmDomainGetChnlList(prAdapter, BAND_6G, TRUE,
				     MAX_6G_BAND_CHN_NUM,
				     &ucChnlNum, arChnlList);
		for (i = 0; i < ucChnlNum; i++) {
			pu4FreqWhiteList[ucFreqWhiteListNum++] =
				nicChannelNum2Freq(
					arChnlList[i].ucChannelNum,
					arChnlList[i].eBand) / 1000;
		}
#endif
	} else if ((eMainLinkBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
	|| eMainLinkBand == BAND_6G
#endif
	) && ucNum2gBss > 0) {
		rlmDomainGetChnlList(prAdapter, BAND_2G4, TRUE,
				     MAX_2G_BAND_CHN_NUM,
				     &ucChnlNum, arChnlList);
		for (i = 0; i < ucChnlNum; ++i) {
			pu4FreqWhiteList[ucFreqWhiteListNum++] =
				nicChannelNum2Freq(
					arChnlList[i].ucChannelNum,
					arChnlList[i].eBand) / 1000;
		}
	}

	if (ucFreqWhiteListNum == 0)
		goto exit;

	pu4FreqList = (uint32_t *)kalMemZAlloc(MAX_CHN_NUM * sizeof(uint32_t),
		VIR_MEM_TYPE);
	if (!pu4FreqList) {
		DBGLOG(P2P, ERROR, "alloc freq list failed\n");
		goto exit;
	}

	u4Status = p2pFunGetPreferredFreqList(prAdapter,
					      IFTYPE_NUM, /* don't care */
					      pu4FreqList,
					      &u4FreqListNum,
					      pu4FreqWhiteList,
					      ucFreqWhiteListNum,
					      TRUE);
	if (u4Status == WLAN_STATUS_SUCCESS && u4FreqListNum > 0)
		*u4PreferFreq = pu4FreqList[0];

exit:
	if (pu4FreqWhiteList)
		kalMemFree(pu4FreqWhiteList, VIR_MEM_TYPE,
			MAX_CHN_NUM * sizeof(uint32_t));

	if (pu4FreqList)
		kalMemFree(pu4FreqList, VIR_MEM_TYPE,
			MAX_CHN_NUM * sizeof(uint32_t));

	return *u4PreferFreq > 0 ?
		WLAN_STATUS_SUCCESS : WLAN_STATUS_NOT_ACCEPTED;
}

static uint32_t p2pLinkGet2ndLinkFreqByPeerPref(struct ADAPTER *prAdapter,
	enum ENUM_BAND eMainLinkBand, uint32_t u4MainLinkFreq,
	uint32_t u4PeerFreq, uint32_t *u4PreferFreq)
{
	enum ENUM_BAND eBand = cnmGetBandByFreq(u4PeerFreq);

	*u4PreferFreq = 0;

	if (u4PeerFreq <= 0)
		return WLAN_STATUS_NOT_SUPPORTED;

	if (eMainLinkBand == BAND_2G4) {
		if (eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
		    || eBand == BAND_6G
#endif
		    )
			*u4PreferFreq = u4PeerFreq;
	} else if (eMainLinkBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
		   || eMainLinkBand == BAND_6G
#endif
		   ) {
		if (eBand == BAND_2G4)
			*u4PreferFreq = u4PeerFreq;
	}

	return *u4PreferFreq > 0 ?
		WLAN_STATUS_SUCCESS : WLAN_STATUS_NOT_ACCEPTED;
}

uint32_t p2pLinkGet2ndLinkFreqByCfg(struct ADAPTER *prAdapter,
	u_int8_t fgIsApMode,
	enum ENUM_BAND eMainLinkBand, uint32_t u4MainLinkFreq,
	uint32_t *u4PreferFreq)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t *pu4Mlo2ndLinkFreqs = NULL;
	uint8_t ucArrSize, i;

	*u4PreferFreq = 0;

	if (fgIsApMode) {
		pu4Mlo2ndLinkFreqs = prWifiVar->au4MloSap2ndLinkFreqs;
		ucArrSize = ARRAY_SIZE(prWifiVar->au4MloSap2ndLinkFreqs);
	} else {
		pu4Mlo2ndLinkFreqs = prWifiVar->au4MloP2p2ndLinkFreqs;
		ucArrSize = ARRAY_SIZE(prWifiVar->au4MloP2p2ndLinkFreqs);
	}

	for (i = 0; i < ucArrSize && pu4Mlo2ndLinkFreqs[i]; i++) {
		uint32_t u4Freq = pu4Mlo2ndLinkFreqs[i];
		uint8_t ucChannelNum = nicFreq2ChannelNum(u4Freq * 1000);
		enum ENUM_BAND eBand = cnmGetBandByFreq(u4Freq);
		u_int8_t fgIsValid = rlmDomainIsLegalChannel(prAdapter,
			eBand, ucChannelNum);

		if (fgIsValid == FALSE)
			continue;

		switch (eMainLinkBand) {
		case BAND_2G4:
			if (eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
			    || eBand == BAND_6G
#endif
			    ) {
				*u4PreferFreq = u4Freq;
			}
			break;

		case BAND_5G:
#if (CFG_SUPPORT_WIFI_6G == 1)
		case BAND_6G:
#endif
			if (eBand == BAND_2G4) {
				*u4PreferFreq = u4Freq;
			} else if (prAdapter->rWifiFemCfg.u2WifiDBDCAwithA &&
				   (eBand >= BAND_5G)) {
				*u4PreferFreq = u4Freq;
			}
			break;

		default:
			break;
		}

		if (*u4PreferFreq)
			break;
	}

	return *u4PreferFreq > 0 ?
		WLAN_STATUS_SUCCESS : WLAN_STATUS_NOT_ACCEPTED;
}

void p2pLinkGet2ndLinkFreq(struct ADAPTER *prAdapter,
	u_int8_t fgIsApMode,
	enum ENUM_BAND eMainLinkBand, uint32_t u4MainLinkFreq,
	uint32_t u4PeerFreq, uint32_t *u4PreferFreq)
{
	*u4PreferFreq = 0;

	DBGLOG(P2P, INFO, "ap=%d, main band=%d freq=%u, peer freq=%u\n",
		fgIsApMode, eMainLinkBand, u4MainLinkFreq, u4PeerFreq);

	/* <1> by wifi cfg */
	if (p2pLinkGet2ndLinkFreqByCfg(prAdapter, fgIsApMode, eMainLinkBand,
				       u4MainLinkFreq, u4PreferFreq) ==
	    WLAN_STATUS_SUCCESS) {
		DBGLOG(P2P, INFO,
			"freq[%u] by p2pLinkGet2ndLinkFreqByCfg\n",
			*u4PreferFreq);
		return;
	}

	/* <2> by own preference */
	if (p2pLinkGet2ndLinkFreqByOwnPref(prAdapter,
					   eMainLinkBand,
					   u4MainLinkFreq,
					   u4PreferFreq) ==
	    WLAN_STATUS_SUCCESS) {
		DBGLOG(P2P, INFO,
			"freq[%u] by p2pLinkGet2ndLinkFreqByOwnPref\n",
			*u4PreferFreq);
		return;
	}

	/* <3> by peer's preference */
	if (p2pLinkGet2ndLinkFreqByPeerPref(prAdapter,
					    eMainLinkBand,
					    u4MainLinkFreq,
					    u4PeerFreq,
					    u4PreferFreq) ==
	    WLAN_STATUS_SUCCESS) {
		DBGLOG(P2P, INFO,
			"freq[%u] by p2pLinkGet2ndLinkFreqByPeerPref\n",
			*u4PreferFreq);
		return;
	}

	/* <4> by default */
	if (*u4PreferFreq == 0) {
		if (eMainLinkBand != BAND_2G4) {
			*u4PreferFreq = nicChannelNum2Freq(
				AP_DEFAULT_CHANNEL_2G,
				BAND_2G4) / 1000;
		} else {
#if (CFG_SUPPORT_WIFI_6G == 1)
			u_int8_t fgIsValid;

			fgIsValid = rlmDomainIsLegalChannel(prAdapter,
				BAND_6G, AP_DEFAULT_CHANNEL_6G);
			if (fgIsValid)
				*u4PreferFreq = nicChannelNum2Freq(
					AP_DEFAULT_CHANNEL_6G,
					BAND_6G) / 1000;
			else
#endif
				*u4PreferFreq = nicChannelNum2Freq(
					AP_DEFAULT_CHANNEL_5G,
					BAND_5G) / 1000;
		}

		DBGLOG(P2P, INFO,
			"freq[%u] by default\n",
			*u4PreferFreq);
	}
}
#endif

void p2pTargetBssDescResetConnecting(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *fsm)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *prBss =
			p2pGetLinkBssInfo(fsm, i);
		struct BSS_DESC *prBssDesc =
			p2pGetLinkBssDesc(fsm, i);

		if (prBss && prBssDesc) {
			prBssDesc->fgIsConnecting &=
				~BIT(prBss->ucBssIndex);
		}
	}
}

void p2pRemoveAllBssDesc(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo =
		mldBssGetByBss(prAdapter, prBssInfo);
	struct BSS_INFO *bss;

	if (prMldBssInfo) {
		LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList,
			rLinkEntryMld, struct BSS_INFO) {
			scanRemoveConnFlagOfBssDescByBssid(
				prAdapter,
				bss->aucBSSID,
				bss->ucBssIndex);
		}
	} else
#endif
	scanRemoveConnFlagOfBssDescByBssid(prAdapter,
		prBssInfo->aucBSSID,
		prBssInfo->ucBssIndex);
}

void p2pClearAllLink(struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		prP2pRoleFsmInfo->aprP2pLinkInfo[i]
			.prP2pTargetStaRec = NULL;
	}
}

void p2pDeactivateAllLink(
	struct ADAPTER *prAdapter,
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t fgClearStaRec)
{
	uint8_t i;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		struct BSS_INFO *bss =
			p2pGetLinkBssInfo(prP2pRoleFsmInfo, i);

		if (bss && IS_NET_ACTIVE(prAdapter, bss->ucBssIndex))
			nicDeactivateNetworkEx(prAdapter,
				NETWORK_ID(bss->ucBssIndex,
					   bss->ucLinkId),
				fgClearStaRec);
	}
}

struct P2P_ROLE_FSM_INFO *p2pGetDefaultRoleFsmInfo(
	struct ADAPTER *prAdapter,
	enum ENUM_IFTYPE eIftype)
{
	return P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
		P2P_MAIN_ROLE_INDEX);
}

struct BSS_INFO *p2pGetDefaultLinkBssInfo(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo =
		mldBssGetByBss(prAdapter, prBssInfo);

	if (prMldBssInfo) {
		struct BSS_INFO *bss =
			LINK_PEEK_HEAD(&(prMldBssInfo->rBssList),
			struct BSS_INFO,
			rLinkEntryMld);

		if (bss)
			return bss;
	}
#endif

	return prBssInfo;
}

void p2pSetLinkBssInfo(struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx,
	struct BSS_INFO *prBssInfo)
{
	prP2pRoleFsmInfo->aprP2pLinkInfo[ucLinkIdx].prP2pBss = prBssInfo;
}

struct BSS_INFO *p2pGetLinkBssInfo(struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx)
{
	return prP2pRoleFsmInfo->aprP2pLinkInfo[ucLinkIdx].prP2pBss;
}

void p2pGetLinkWmmQueSet(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	struct BSS_INFO *bss;

	/* main bss must assign wmm first */
	bss = p2pGetDefaultLinkBssInfo(prAdapter, prBssInfo);
	cnmWmmIndexDecision(prAdapter, bss);

#if (CFG_SUPPORT_802_11BE_MLO == 1) && (CFG_WLAN_CONNAC3_DEV == 0)
	/* connac3 MLO all bss use the same wmm index as main bss use */
	if (p2pRoleFsmNeedMlo(prAdapter, prBssInfo->u4PrivateData)) {
		prBssInfo->fgIsWmmInited = TRUE;
		prBssInfo->ucWmmQueSet = bss->ucWmmQueSet;
	} else
#endif
	{
		/* connac2 always assign different wmm index to bssinfo */
		cnmWmmIndexDecision(prAdapter, prBssInfo);
	}

	DBGLOG(P2P, TRACE, "bss[%d] = %d\n",
		prBssInfo->ucBssIndex, prBssInfo->ucWmmQueSet);
}

struct BSS_DESC *p2pGetLinkBssDesc(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx)
{
	struct P2P_JOIN_INFO *prJoinInfo;

	if (ucLinkIdx >= MLD_LINK_MAX)
		return NULL;

	prJoinInfo = &(prP2pRoleFsmInfo->rJoinInfo);

	return prJoinInfo->rBssDescSet.aprBssDescW[ucLinkIdx] ?
	       prJoinInfo->rBssDescSet.aprBssDescW[ucLinkIdx]->prBssDesc :
	       NULL;
}

uint8_t p2pGetLinkNum(struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo)
{
	uint8_t i, num = 0;

	for (i = 0; i < MLD_LINK_MAX; i++) {
		if (p2pGetLinkBssDesc(prP2pRoleFsmInfo, i))
			num++;
	}

	return num;
}

void p2pSetLinkStaRec(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	struct STA_RECORD *prStaRec,
	uint8_t ucLinkIdx)
{
	if (ucLinkIdx >= MLD_LINK_MAX)
		return;
	prP2pRoleFsmInfo->aprP2pLinkInfo[ucLinkIdx]
		.prP2pTargetStaRec = prStaRec;
}

struct STA_RECORD *p2pGetLinkStaRec(
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
	uint8_t ucLinkIdx)
{
	if (ucLinkIdx >= MLD_LINK_MAX)
		return NULL;

	return prP2pRoleFsmInfo->aprP2pLinkInfo[ucLinkIdx]
		.prP2pTargetStaRec;
}

void p2pLinkStaRecFree(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct BSS_INFO *prP2pBssInfo)
{
	enum ENUM_PARAM_MEDIA_STATE eOriMediaStatus =
		prP2pBssInfo->eConnectionState;

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

void p2pLinkAcquireChJoin(struct ADAPTER *prAdapter,
			  struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo,
			  struct P2P_CHNL_REQ_INFO *prChnlReq,
			  struct P2P_JOIN_INFO *prJoinInfo)
{
	struct MSG_CH_REQ *prMsgChReq = NULL;
	struct MSG_CH_REQ *prSubReq = NULL;
	uint8_t ucReqChNum = 0;
	uint32_t u4MsgSz;
	uint8_t i = 0;
	uint8_t ucSeqNumOfChReq;

	if (!prAdapter || !prChnlReq || !prJoinInfo)
		return;

	ucReqChNum = prJoinInfo->rBssDescSet.ucLinkNum;

	p2pFuncReleaseCh(prAdapter, prP2pRoleFsmInfo->ucBssIndex, prChnlReq);

	/* send message to CNM for acquiring channel */
	u4MsgSz = sizeof(struct MSG_CH_REQ) * ucReqChNum;
	prMsgChReq = (struct MSG_CH_REQ *)cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG,
		u4MsgSz);
	if (!prMsgChReq) {
		DBGLOG(P2P, ERROR, "Alloc CH req msg failed.\n");
		return;
	}
	kalMemZero(prMsgChReq, u4MsgSz);

	prChnlReq->ucChReqNum = ucReqChNum;

	prMsgChReq->ucExtraChReqNum = prChnlReq->ucChReqNum - 1;

	ucSeqNumOfChReq = ++prChnlReq->ucSeqNumOfChReq;

	for (i = 0; i < ucReqChNum; i++) {
		struct BSS_INFO *prBss =
			p2pGetLinkBssInfo(prP2pRoleFsmInfo, i);
		struct BSS_DESC *prBssDesc =
			p2pGetLinkBssDesc(prP2pRoleFsmInfo, i);

		if (!prBss || !prBssDesc)
			continue;

		/* for secondary link */
		if (!IS_NET_ACTIVE(prAdapter, prBss->ucBssIndex)) {
			/* sync with firmware */
			nicActivateNetwork(prAdapter,
				NETWORK_ID(prBss->ucBssIndex,
					   prBss->ucLinkId));
			SET_NET_PWR_STATE_ACTIVE(prAdapter,
						 prBss->ucBssIndex);
		}

		prSubReq = (struct MSG_CH_REQ *)&prMsgChReq[i];

		prSubReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
		prSubReq->ucBssIndex = prBss->ucBssIndex;
		prSubReq->ucTokenID = ucSeqNumOfChReq;
		prSubReq->eReqType = CH_REQ_TYPE_JOIN;
		prSubReq->u4MaxInterval = P2P_GC_JOIN_CH_REQUEST_INTERVAL;
		prSubReq->ucPrimaryChannel = prBssDesc->ucChannelNum;
		prSubReq->eRfSco = prBssDesc->eSco;
		prSubReq->eRfBand = prBssDesc->eBand;
		prSubReq->eRfChannelWidth = prBssDesc->eChannelWidth;
		prSubReq->ucRfCenterFreqSeg1 =
			nicGetS1(prBssDesc->eBand,
				 prBssDesc->ucChannelNum,
				 prBssDesc->eSco,
				 rlmVhtBw2OpBw(prBssDesc->eChannelWidth,
					       prBssDesc->eSco));
		prSubReq->ucRfCenterFreqSeg2 = 0;
#if CFG_SUPPORT_DBDC
		if (ucReqChNum >= 2)
			prSubReq->eDBDCBand = ENUM_BAND_ALL;
		else
			prSubReq->eDBDCBand = ENUM_BAND_AUTO;
#endif /*CFG_SUPPORT_DBDC*/

		rlmReviseMaxBw(prAdapter,
			       prBss->ucBssIndex,
			       &prSubReq->eRfSco,
			       &prSubReq->eRfChannelWidth,
			       &prSubReq->ucRfCenterFreqSeg1,
			       &prSubReq->ucPrimaryChannel);
	}

	DBGLOG(P2P, INFO, "dbdc=%d token=%u num=%d\n",
		prMsgChReq->eDBDCBand,
		prMsgChReq->ucTokenID,
		prMsgChReq->ucExtraChReqNum);

	mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *)prMsgChReq,
			MSG_SEND_METHOD_UNBUF);

	prChnlReq->fgIsChannelRequested = TRUE;
}

uint32_t
p2pRoleFsmRunEventAAAComplete(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct BSS_INFO *prP2pBssInfo)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_STA_RECORD *mld_starec;
#endif

	bssAssignAssocID(prAdapter, prStaRec);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mld_starec = mldStarecGetByStarec(prAdapter, prStaRec);
	if (mld_starec) {
		struct LINK *links;
		struct STA_RECORD *starec;
		uint32_t rStatus = WLAN_STATUS_SUCCESS;

		links =  &mld_starec->rStarecList;
		LINK_FOR_EACH_ENTRY(starec,
			links, rLinkEntryMld,
			struct STA_RECORD) {
			struct BSS_INFO *bss =
				GET_BSS_INFO_BY_INDEX(
				prAdapter,
				starec->ucBssIndex);

			if (!bss) {
				DBGLOG(P2P, WARN,
					"\tNull bss by idx(%u)\n",
					starec->ucBssIndex);
				continue;
			}

			DBGLOG(INIT, DEBUG,
				"\tsta: %d, wid: %d, bss: %d, " MACSTR "\n",
				starec->ucIndex,
				starec->ucWlanIndex,
				starec->ucBssIndex,
				MAC2STR(bss->aucOwnMacAddr));

			rStatus =
			p2pRoleFsmRunEventAAACompleteImpl(
				prAdapter,
				starec,
				bss);
			if (rStatus != WLAN_STATUS_SUCCESS)
				return rStatus;
		}

		return rStatus;
	}
#endif

	return p2pRoleFsmRunEventAAACompleteImpl(
		prAdapter,
		prStaRec,
		prP2pBssInfo);
}

uint32_t
p2pRoleFsmRunEventAAASuccess(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct BSS_INFO *prP2pBssInfo,
	struct MSDU_INFO *prMsduInfo)
{
	int32_t offset;
	uint32_t status;

	offset = sortMsduPayloadOffset(prAdapter, prMsduInfo);

	if (offset != -1) {
		prStaRec->pucAssocRespIe =
			prMsduInfo->prPacket + offset;
		prStaRec->u2AssocRespIeLen =
			prMsduInfo->u2FrameLength - offset;
	} else {
		DBGLOG(P2P, ERROR, "Invalid packet format.");
	}

	status = p2pRoleFsmRunEventAAASuccessImpl(prAdapter, prStaRec,
		prP2pBssInfo);

	prStaRec->pucAssocRespIe = NULL;
	prStaRec->u2AssocRespIeLen = 0;

	return status;
}

void p2pRoleFsmRunEventAAATxFail(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct BSS_INFO *prP2pBssInfo)
{
	p2pRoleFsmRunEventAAATxFailImpl(
		prAdapter,
		prStaRec,
		prP2pBssInfo);
}

uint16_t bssAssignAssocID(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec)
{
	uint8_t u2AssocId = 0;
	struct BSS_INFO *prBssInfo = NULL;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct BSS_INFO *prBssInfoNext = NULL;
	struct MLD_BSS_INFO *prBssInfoMld = NULL;
	struct LINK *prBssList;
#endif

	prBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter,
			prStaRec->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(P2P, ERROR, "Invalid BssInfo.");
		goto exit;
	}
	prBssInfo->u2P2pAssocIdCounter++;
	if (prBssInfo->u2P2pAssocIdCounter == 0 ||
		prBssInfo->u2P2pAssocIdCounter > P2P_MAX_AID_VALUE ||
		prBssInfo->u2P2pAssocIdCounter > CFG_STA_REC_NUM)
		prBssInfo->u2P2pAssocIdCounter = 1;

	u2AssocId =
		prBssInfo->u2P2pAssocIdCounter;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prBssInfoMld = mldBssGetByBss(prAdapter, prBssInfo);
	if (prBssInfoMld) {
		prBssList = &prBssInfoMld->rBssList;
		LINK_FOR_EACH_ENTRY(prBssInfoNext, prBssList,
			rLinkEntryMld, struct BSS_INFO) {
				prBssInfoNext->u2P2pAssocIdCounter =
					prBssInfo->u2P2pAssocIdCounter;
		}
	}
#endif

exit:
	return u2AssocId;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
void p2pScanFillSecondaryLink(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prBssInfo,
			      struct BSS_DESC_SET *prBssDescSet)
{
	struct MLD_BSS_INFO *prMlsBss = mldBssGetByBss(prAdapter, prBssInfo);
	struct LINK *prBSSDescList =
		&prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	struct BSS_DESC_W *prBssDescW;
	struct BSS_DESC *prBssDesc;
	struct BSS_DESC *prMainBssDesc = prBssDescSet->prMainBssDesc;
	uint8_t i, j, ucMaxLinkNum;

	if (!prMainBssDesc || !prMainBssDesc->rMlInfo.fgValid ||
	    prMainBssDesc->rMlInfo.fgMldType == MLD_TYPE_ICV_METHOD_V1) {
		DBGLOG(P2P, INFO, "no need secondary link");
		return;
	}

	if (!mldIsMultiLinkEnabled(prAdapter, NETWORK_TYPE_P2P, FALSE) ||
	    !mldBssAllowReconfig(prAdapter, prMlsBss))
		return;

	ucMaxLinkNum = prAdapter->rWifiVar.ucP2pMldLinkMax;

	/* setup secondary link */
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry,
		struct BSS_DESC) {

		/* break if reach the limit num of links */
		if (prBssDescSet->ucLinkNum >= ucMaxLinkNum)
			break;

		if (!prBssDesc->rMlInfo.fgValid ||
		    EQUAL_MAC_ADDR(prMainBssDesc->aucBSSID,
				 prBssDesc->aucBSSID) ||
		    !EQUAL_MAC_ADDR(prMainBssDesc->rMlInfo.aucMldAddr,
				 prBssDesc->rMlInfo.aucMldAddr)) {
			DBGLOG(P2P, LOUD,
				"Skip " MACSTR " valid=%d mld_addr="MACSTR"\n",
				MAC2STR(prBssDesc->aucBSSID),
				prBssDesc->rMlInfo.fgValid,
				MAC2STR(prBssDesc->rMlInfo.aucMldAddr));
			continue;
		}

		DBGLOG(P2P, INFO,
			"Add " MACSTR " mld_addr=" MACSTR " link=%d\n",
			MAC2STR(prBssDesc->aucBSSID),
			MAC2STR(prBssDesc->rMlInfo.aucMldAddr),
			prBssDesc->rMlInfo.ucLinkId);

		prBssDescSet->aprBssDescW[prBssDescSet->ucLinkNum] =
			&prBssDescSet->arBssDescWPool[prBssDescSet->ucLinkNum];
		/* Record same Mld list */
		scanFillBssDescW(
			prBssDescSet->aprBssDescW[prBssDescSet->ucLinkNum],
			prBssDesc);
		prBssDescSet->ucLinkNum++;
	}

	for (i = 0; i < prBssDescSet->ucLinkNum - 1; i++) {
		for (j = i + 1; j < prBssDescSet->ucLinkNum; j++) {
			if (prBssDescSet->aprBssDescW[j]->ucLinkId <
				prBssDescSet->aprBssDescW[i]->ucLinkId) {
				prBssDescW = prBssDescSet->aprBssDescW[j];
				prBssDescSet->aprBssDescW[j] =
					prBssDescSet->aprBssDescW[i];
				prBssDescSet->aprBssDescW[i] =
					prBssDescW;
			}
		}
	}

	/* first bss desc is main bss */
	prBssDescSet->prMainBssDesc = prBssDescSet->aprBssDescW[0]->prBssDesc;
	prMainBssDesc = prBssDescSet->prMainBssDesc;
	DBGLOG(P2P, INFO,
		"Total %d link(s), Main=" MACSTR
		" mld_addr=" MACSTR " link=%d\n",
		prBssDescSet->ucLinkNum,
		MAC2STR(prMainBssDesc->aucBSSID),
		MAC2STR(prMainBssDesc->rMlInfo.aucMldAddr),
		prMainBssDesc->rMlInfo.ucLinkId);
}
#endif

u_int8_t
p2pNeedAppendP2pIE(
	struct ADAPTER *ad,
	struct BSS_INFO *bss)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (ad && ad->rWifiVar.fgSkipP2pIe &&
	    bss && IS_BSS_APGO(bss)) {
		struct MLD_BSS_INFO *mld =
			mldBssGetByBss(ad, bss);

		if (IS_MLD_BSSINFO_MULTI(mld) &&
		    bss->ucLinkId != P2P_MAIN_LINK_INDEX) {
			DBGLOG(BSS, LOUD,
				"Skip p2p ie for role%d\n",
				bss->u4PrivateData);
			return FALSE;
		}
	}
#endif

	return TRUE;
}

u_int8_t
p2pNeedSkipProbeResp(
	struct ADAPTER *ad,
	struct BSS_INFO *bss)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (ad && ad->rWifiVar.fgSkipP2pProbeResp &&
	    bss && IS_BSS_APGO(bss)) {
		struct MLD_BSS_INFO *mld =
			mldBssGetByBss(ad, bss);

		if (IS_MLD_BSSINFO_MULTI(mld) &&
		    bss->ucLinkId != P2P_MAIN_LINK_INDEX) {
			DBGLOG(BSS, LOUD,
				"Skip p2p ie for role%d\n",
				bss->u4PrivateData);
			return TRUE;
		}
	}
#endif

	return FALSE;
}

#endif /* CFG_ENABLE_WIFI_DIRECT */
