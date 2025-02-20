// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "precomp.h"

/*******************************************************************************
 *                                 CCM Related
 *******************************************************************************
 */
#if CFG_SUPPORT_CCM
struct P2P_CCM_CSA_ENTRY {
	struct LINK_ENTRY rLinkEntry;
	struct BSS_INFO *prBssInfo;
	uint32_t u4TargetCh;
	enum ENUM_MBMC_BN eTargetHwBandIdx;
	enum ENUM_BAND eTargetBand;
};

static void __ccmChannelSwitchProducer(struct ADAPTER *prAdapter,
				       struct BSS_INFO *prTargetBss,
				       const char *pucSrcFunc);

static u_int8_t ccmCheckAndPrepareChannelSwitch(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo,
			    uint32_t *u4TargetCh,
			    enum ENUM_MBMC_BN eTargetHwBandIdx,
			    enum ENUM_BAND *eTargetBand);

void ccmInit(struct ADAPTER *prAdapter)
{
	LINK_INITIALIZE(&prAdapter->rCcmCheckCsList);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Prevent to access the old connection info.
 */
/*----------------------------------------------------------------------------*/
void ccmRemoveBssPendingEntry(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prBssInfo)
{
	struct LINK *prCcmCheckCsList = &prAdapter->rCcmCheckCsList;
	struct P2P_CCM_CSA_ENTRY *prCcmCsaEntry;
	struct P2P_CCM_CSA_ENTRY *prCcmCsaEntryNext;

	LINK_FOR_EACH_ENTRY_SAFE(prCcmCsaEntry, prCcmCsaEntryNext,
				 prCcmCheckCsList, rLinkEntry,
				 struct P2P_CCM_CSA_ENTRY) {
		if (prCcmCsaEntry->prBssInfo != prBssInfo)
			continue;
		DBGLOG(CCM, INFO, "bss=%u free, remove pending entry",
		       prBssInfo->ucBssIndex);
		LINK_REMOVE_KNOWN_ENTRY(prCcmCheckCsList, prCcmCsaEntry);
		cnmMemFree(prAdapter, prCcmCsaEntry);
	}
}

static void ccmPendingEventTimeout(struct ADAPTER *prAdapter,
				   uintptr_t ulParamPtr)
{
	struct BSS_INFO *targetBss = (struct BSS_INFO *) ulParamPtr;

	prAdapter->fgIsCcmPending = FALSE;
	ccmChannelSwitchProducer(prAdapter, targetBss, __func__);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check if CCM need to pending until ch grant timeout.
 *        Only pending when a GO want to CSA.
 *
 * \param[in] pvAdapter Pointer to the adapter descriptor.
 *            u4TargetCh Target BSS channel.
 *            eTargetHwBandIdx Target BSS HW band.
 *            eTargetBand Target BSS RF band.
 *
 * \return status
 */
/*----------------------------------------------------------------------------*/
void ccmPendingCheck(struct ADAPTER *prAdapter,
		     struct BSS_INFO *prTargetBss,
		     uint32_t u4GrantInterval)
{
	uint8_t i;
	struct BSS_INFO *bss;
	uint32_t u4TargetCh;
	enum ENUM_BAND eTargetBand;
	u_int8_t fgIsTargetMlo = FALSE;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBss = mldBssGetByBss(prAdapter, prTargetBss);

	fgIsTargetMlo = IS_MLD_BSSINFO_MULTI(prMldBss);
#endif

	if (prAdapter->fgIsCcmPending) {
		DBGLOG(CCM, WARN,
		       "skip pending check due to previous check not done\n");
		return;
	}
	prAdapter->fgIsCcmPending = FALSE;

	for (i = 0; i < MAX_BSSID_NUM && !prAdapter->fgIsCcmPending; ++i) {
		bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		/* skip target bss itself */
		if (!fgIsTargetMlo && bss == prTargetBss)
			continue;
#if (CFG_SUPPORT_802_11BE_MLO == 1) || defined(CFG_SUPPORT_UNIFIED_COMMAND)
		else if (bss->ucGroupMldId == prTargetBss->ucGroupMldId)
			continue;
#endif

		/* Only check GO, because only GO has NoA */
		if (!IS_BSS_APGO(bss) || !IS_BSS_ALIVE(prAdapter, bss) ||
		    p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[
				    bss->u4PrivateData]))
			continue;

		/* Must copy, because
		 * ccmCheckAndPrepareChannelSwitch may modify it.
		 */
		u4TargetCh = prTargetBss->ucPrimaryChannel;
		eTargetBand = prTargetBss->eBand;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		/* MLO GC/STA only ch req once */
		if (IS_BSS_GC(prTargetBss) || IS_BSS_AIS(prTargetBss)) {
			struct BSS_INFO *mldTargetbss;

			if (prMldBss) {
				LINK_FOR_EACH_ENTRY(mldTargetbss,
						    &prMldBss->rBssList,
						    rLinkEntryMld,
						    struct BSS_INFO) {
					u4TargetCh =
						mldTargetbss->ucPrimaryChannel;
					eTargetBand = mldTargetbss->eBand;

					DBGLOG(CCM, INFO,
					       "checking [%s]bss=%u ch=%u, hwBand=%u, rfBand=%u [Target]ch=%u, hwBand=%u, rfBand=%u\n",
					       bssGetRoleTypeString(prAdapter,
								    bss),
					       bss->ucBssIndex,
					       bss->ucPrimaryChannel,
					       bss->eHwBandIdx, bss->eBand,
					       u4TargetCh,
					       mldTargetbss->eHwBandIdx,
					       eTargetBand);

					if (!ccmCheckAndPrepareChannelSwitch(
						prAdapter, bss, &u4TargetCh,
						mldTargetbss->eHwBandIdx,
						&eTargetBand))
						continue;

					prAdapter->fgIsCcmPending = TRUE;
					break;
				}
			} else if (ccmCheckAndPrepareChannelSwitch(prAdapter,
					bss, &u4TargetCh,
					prTargetBss->eHwBandIdx, &eTargetBand))
				prAdapter->fgIsCcmPending = TRUE;
		} else if (IS_BSS_APGO(prTargetBss))
#endif /* CFG_SUPPORT_802_11BE_MLO == 1 */
			if (ccmCheckAndPrepareChannelSwitch(prAdapter, bss,
					 &u4TargetCh, prTargetBss->eHwBandIdx,
					 &eTargetBand))
				prAdapter->fgIsCcmPending = TRUE;
	}

	if (prAdapter->fgIsCcmPending) {
		/* New connect ch req will make GO NoA for 4~5s,
		 * which may cause GC absent and not receive beacon.
		 */
		DBGLOG(CCM, INFO,
		       "pending CCM for %ums because a GO wants to CSA\n",
		       u4GrantInterval);
		cnmTimerInitTimer(prAdapter, &prAdapter->rCcmPendingTimer,
			(PFN_MGMT_TIMEOUT_FUNC) ccmPendingEventTimeout,
			(uintptr_t) prTargetBss);
		cnmTimerStartTimer(prAdapter, &prAdapter->rCcmPendingTimer,
				   u4GrantInterval);
	}
}

static void ccmGetOtherAliveBssHwBitmap(struct ADAPTER *prAdapter,
				 uint32_t *pau4Bitmap,
				 struct BSS_INFO *prBssInfo)
{
	struct BSS_INFO *bss;
	uint8_t i;

	for (i = 0; i < MAX_BSSID_NUM; ++i) {
		bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		if (!IS_BSS_ALIVE(prAdapter, bss) || bss == prBssInfo ||
		    bss->eHwBandIdx >= AA_HW_BAND_NUM)
			continue;

		pau4Bitmap[bss->eHwBandIdx] |= BIT(i);
	}
	DBGLOG(P2P, TRACE,
	       "alive BSS hw bitmap [bn0:bn1:bn2]=[0x%x:0x%x:0x%x]\n",
	       pau4Bitmap[AA_HW_BAND_0], pau4Bitmap[AA_HW_BAND_1],
	       pau4Bitmap[AA_HW_BAND_2]);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Entry Point of CCM, other modules should call this.
 */
/*----------------------------------------------------------------------------*/
void ccmChannelSwitchProducer(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prTargetBss,
			      const char *pucSrcFunc)
{
	if (!prAdapter->fgIsP2PRegistered)
		return;

	if (!prTargetBss) {
		DBGLOG(CCM, INFO, "null target Bss\n");
		return;
	}

	if (prAdapter->fgIsCcmPending) {
		DBGLOG(CCM, INFO, "skip CCM due to pending");
		return;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (IS_BSS_GC(prTargetBss) || IS_BSS_AIS(prTargetBss)) {
		struct BSS_INFO *bss;
		struct MLD_BSS_INFO *prMldBss = mldBssGetByBss(prAdapter,
							       prTargetBss);

		if (prMldBss) {
			/* MLO GC/STA only ch abort once */
			LINK_FOR_EACH_ENTRY(bss, &prMldBss->rBssList,
					    rLinkEntryMld, struct BSS_INFO)
				__ccmChannelSwitchProducer(prAdapter, bss,
							   pucSrcFunc);
		} else
			__ccmChannelSwitchProducer(prAdapter, prTargetBss,
						   pucSrcFunc);
	} else if (IS_BSS_APGO(prTargetBss))
#endif /* CFG_SUPPORT_802_11BE_MLO == 1 */
		__ccmChannelSwitchProducer(prAdapter, prTargetBss, pucSrcFunc);
}

void ccmChannelSwitchProducerDfs(struct ADAPTER *prAdapter,
				 struct BSS_INFO *prTargetBss)
{
	struct LINK *list = &prAdapter->rCcmCheckCsList;
	struct BSS_INFO *bss;
	struct P2P_CCM_CSA_ENTRY *entry;
	uint8_t i;
	u_int8_t fgNewReqs = FALSE;

	if (prAdapter->prGlueInfo->u4ReadyFlag == 0)
		return;

	if (IS_STA_DFS_CHANNEL_ENABLED(prAdapter) == FALSE &&
	    IS_STA_INDOOR_CHANNEL_ENABLED(prAdapter) == FALSE)
		return;

	for (i = 0; i < MAX_BSSID_NUM; ++i) {
		struct RF_CHANNEL_INFO rRfChnlInfo;
		uint32_t au4FreqList[MAX_5G_BAND_CHN_NUM];
		uint32_t u4FreqListNum;

		bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		/* sanity check for bss */
		if (bss == NULL || !IS_BSS_ALIVE(prAdapter, bss) ||
		    !IS_BSS_APGO(bss))
			continue;

		/* Only GO use STA's DFS channel */
		if (p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[
				    bss->u4PrivateData]))
			continue;

		kalMemZero(&rRfChnlInfo, sizeof(rRfChnlInfo));
		rRfChnlInfo.eBand = bss->eBand;
		rRfChnlInfo.u4CenterFreq1 = bss->ucVhtChannelFrequencyS1;
		rRfChnlInfo.u4CenterFreq2 = bss->ucVhtChannelFrequencyS2;
		rRfChnlInfo.u2PriChnlFreq =
			nicChannelNum2Freq(bss->ucPrimaryChannel,
					   bss->eBand) / 1000;
		rRfChnlInfo.ucChnlBw =
			rlmVhtBw2Bw(bss->ucVhtChannelWidth,
				    bss->eBssSCO);
		rRfChnlInfo.ucChannelNum = bss->ucPrimaryChannel;

		/* check bss is using DFS channel, and DFS is allowed by STA */
		if (bss->eBand != BAND_5G ||
		    rlmDomainIsDfsChnls(prAdapter,
					bss->ucPrimaryChannel) == FALSE ||
		    wlanDfsChannelsAllowdBySta(prAdapter, &rRfChnlInfo))
			continue;

		u4FreqListNum =
			p2pFunGetTopPreferFreqByBand(prAdapter,
						     BAND_5G,
						     MAX_5G_BAND_CHN_NUM,
						     au4FreqList);
		if (u4FreqListNum == 0)
			continue;

		entry = cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(*entry));
		if (!entry) {
			DBGLOG(CCM, ERROR, "Alloc mem fail\n");
			return;
		}

		entry->prBssInfo = bss;
		entry->u4TargetCh = nicFreq2ChannelNum(au4FreqList[0] * 1000);
		entry->eTargetHwBandIdx = bss->eHwBandIdx;
		entry->eTargetBand = bss->eBand;

		LINK_INSERT_TAIL(list, &entry->rLinkEntry);

		fgNewReqs = TRUE;

		DBGLOG(CCM, INFO,
			"insert GO bss=%u waiting to check\n",
			bss->ucBssIndex);
	}

	if (fgNewReqs)
		ccmChannelSwitchConsumer(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check if channel switch allowed and prepare to switchh to A+A channel
 *        or original input target channel.
 *
 * \param[in] u4TargetCh  Point to target channel we want to switch to,
 *                        may be modified for A+A.
 *            eTargetBand Point to target band we want to switch to,
 *                        may be modified for A+A.
 *
 * \return TRUE, ready to channel switch.
 *         FALSE, not alloed to channel switch.
 */
/*----------------------------------------------------------------------------*/
static u_int8_t ccmCheckAndPrepareChannelSwitch(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo,
			    uint32_t *u4TargetCh,
			    enum ENUM_MBMC_BN eTargetHwBandIdx,
			    enum ENUM_BAND *eTargetBand)
{
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint32_t freqList[MAX_5G_BAND_CHN_NUM + MAX_6G_BAND_CHN_NUM] = {};
#else
	uint32_t freqList[MAX_5G_BAND_CHN_NUM] = {};
#endif
	uint32_t u4FreqListNum;
	uint32_t u4Idx;
	uint32_t u4ChForAa;
	enum ENUM_BAND eBandForAa;
	uint32_t u4Freq;

	/* pass for MCC only */
	if (prBssInfo->eHwBandIdx != eTargetHwBandIdx ||
	    prBssInfo->ucPrimaryChannel == *u4TargetCh) {
		DBGLOG(CCM, INFO,
		       "do not trigger CSA, [%s]Bss%u, ch=%u, hwBand=%u, rfBand=%u [Target]ch=%u, hwBand=%u, rfBand=%u\n",
		       bssGetRoleTypeString(prAdapter, prBssInfo),
		       prBssInfo->ucBssIndex, prBssInfo->ucPrimaryChannel,
		       prBssInfo->eHwBandIdx, prBssInfo->eBand,
		       *u4TargetCh, eTargetHwBandIdx, *eTargetBand);
		return FALSE;
	}

	if ((prBssInfo->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
	     || prBssInfo->eBand == BAND_6G
#endif
	     ) &&
	    p2pFuncIsPreferWfdAa(prAdapter, prBssInfo)) {
		u4FreqListNum = p2pFuncAppendAaFreq(prAdapter, prBssInfo,
						     freqList);
		for (u4Idx = 0; u4Idx < u4FreqListNum; u4Idx++) {
			u4Freq = freqList[u4Idx];
			u4ChForAa = nicFreq2ChannelNum(u4Freq * 1000);
			if (u4Freq >= 2412 && u4Freq <= 2484)
				eBandForAa = BAND_2G4;
			else if (u4Freq >= 5180 && u4Freq <= 5900)
				eBandForAa = BAND_5G;
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (u4Freq >= 5955 && u4Freq <= 7115)
				eBandForAa = BAND_6G;
#endif
			if (p2pFuncIsCsaAllowed(prAdapter, prBssInfo,
				u4ChForAa, eBandForAa) == CSA_STATUS_SUCCESS) {
				/* modify the input ch and band! */
				*u4TargetCh = u4ChForAa;
				*eTargetBand = eBandForAa;
				return TRUE;
			}
		}
	} else if (p2pFuncIsCsaAllowed(prAdapter, prBssInfo, *u4TargetCh,
				 *eTargetBand) == CSA_STATUS_SUCCESS)
		return TRUE;

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Each time consume one entry in queue to check whether it needs to CS.
 *        Only trigger by __ccmChannelSwitchProducer,
 *        p2pRoleStateAbort_SWITCH_CHANNEL and itself.
 *
 * \param[in] pvAdapter Pointer to the adapter descriptor.
 *
 * \return status
 */
/*----------------------------------------------------------------------------*/
void ccmChannelSwitchConsumer(struct ADAPTER *prAdapter)
{
	struct LINK *prCcmCheckCsList = &prAdapter->rCcmCheckCsList;
	struct P2P_CCM_CSA_ENTRY *prCcmCsaEntry;
	struct BSS_INFO *bss;
	uint32_t u4TargetCh;
	enum ENUM_MBMC_BN eTargetHwBandIdx;
	enum ENUM_BAND eTargetBand;
	u_int8_t fgIsSwitching = FALSE;
	u_int8_t fgIsMlo = FALSE;
	int8_t i;

	if (prAdapter->rWifiVar.fgCsaInProgress) {
		/* csa start countdown till csadone */
		DBGLOG(CCM, INFO, "skip due to CSA still in progress\n");
		return;
	} else {
		/* start switching channel till ch granted */
		for (i = 0; i < MAX_BSSID_NUM; ++i) {
			bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

			if (IS_BSS_P2P(bss) && bss->fgIsSwitchingChnl)
				break;
			else if (IS_BSS_AIS(bss) && bss->fgIsAisSwitchingChnl)
				break;
		}

		if (i < MAX_BSSID_NUM) {
			DBGLOG(CCM, INFO,
			       "skip due to Bss%u channel switch still in progress\n",
			       i);
			return;
		}
	}

	if (!LINK_IS_EMPTY(prCcmCheckCsList)) {
		LINK_REMOVE_HEAD(prCcmCheckCsList, prCcmCsaEntry,
			struct P2P_CCM_CSA_ENTRY *);
		bss = prCcmCsaEntry->prBssInfo;
		u4TargetCh = prCcmCsaEntry->u4TargetCh;
		eTargetHwBandIdx = prCcmCsaEntry->eTargetHwBandIdx;
		eTargetBand = prCcmCsaEntry->eTargetBand;

		if (!IS_BSS_APGO(bss))
			return;

		cnmMemFree(prAdapter, prCcmCsaEntry);
	} else {
		DBGLOG(CCM, INFO,
		       "all BSS align new connection or CSA chnl done\n");
		return;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	fgIsMlo = IS_MLD_BSSINFO_MULTI(mldBssGetByBss(prAdapter, bss));
#endif

	DBGLOG(CCM, INFO,
	       "checking [%s]bss=%u ch=%u, hwBand=%u, rfBand=%u, is_mlo=%u, [Target]ch=%u, hwBand=%u, rfBand=%u\n",
	       bssGetRoleTypeString(prAdapter, bss),
	       bss->ucBssIndex, bss->ucPrimaryChannel,
	       bss->eHwBandIdx, bss->eBand, fgIsMlo,
	       u4TargetCh, eTargetHwBandIdx, eTargetBand);

	if (p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[
		bss->u4PrivateData]) && !fgIsMlo)
		fgIsSwitching = p2pFuncSwitchSapChannel(prAdapter,
					P2P_DEFAULT_SCENARIO);
	else if (ccmCheckAndPrepareChannelSwitch(prAdapter, bss, &u4TargetCh,
				      eTargetHwBandIdx, &eTargetBand)) {
		cnmIdcCsaReq(prAdapter, eTargetBand, u4TargetCh,
				     bss->u4PrivateData);
		fgIsSwitching = TRUE;
	}

	if (!fgIsSwitching)
		ccmChannelSwitchConsumer(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Produce the BSS CS check event, and consumer will check whether it
 *        needs to CS to SCC with prTargetBss.
 *
 * \param[in] pvAdapter Pointer to the adapter descriptor.
 * \param[in] prTargetBss The target BSS, MLO should call twice.
 * \param[in] pucSrcFunc Source function that trigger the CSA.
 *
 * \return status
 */
/*----------------------------------------------------------------------------*/
static void __ccmChannelSwitchProducer(struct ADAPTER *prAdapter,
				struct BSS_INFO *prTargetBss,
				const char *pucSrcFunc)
{
	struct BSS_INFO *bss;
	uint8_t i;
	struct LINK *prCcmCheckCsList = &prAdapter->rCcmCheckCsList;
	u_int8_t fgIsTargetMlo = FALSE;
	struct P2P_CCM_CSA_ENTRY *prCcmCsaEntry;
	struct P2P_CCM_CSA_ENTRY *prFirstSap = NULL;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	fgIsTargetMlo = IS_MLD_BSSINFO_MULTI(
				mldBssGetByBss(prAdapter, prTargetBss));
#endif

	DBGLOG(CCM, INFO,
	       "%s: bss=%u, role=%s, is_mlo=%u notify other GO/SAP to CSA\n",
	       pucSrcFunc, prTargetBss->ucBssIndex,
	       bssGetRoleTypeString(prAdapter, prTargetBss), fgIsTargetMlo);

	/* find the first SAP entry in list */
	LINK_FOR_EACH_ENTRY(prCcmCsaEntry, prCcmCheckCsList, rLinkEntry,
			    struct P2P_CCM_CSA_ENTRY) {
		if (p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[
				prCcmCsaEntry->prBssInfo->u4PrivateData])) {
			prFirstSap = prCcmCsaEntry;
			DBGLOG(CCM, TRACE, "first SAP in list is bss=%u\n",
			       prFirstSap->prBssInfo->ucBssIndex);
			break;
		}
	}

	/* enqueue GO before the first SAP first */
	if (prWifiVar->eP2pCcmMode != P2P_CCM_MODE_DISABLE) {
		for (i = 0; i < MAX_BSSID_NUM; ++i) {
			bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

			if (!IS_BSS_APGO(bss) || !IS_BSS_ALIVE(prAdapter, bss)
			    || p2pFuncIsAPMode(
					prAdapter->rWifiVar.prP2PConnSettings[
							bss->u4PrivateData]))
				continue;

			/* skip target bss itself */
			if (!fgIsTargetMlo && bss == prTargetBss)
				continue;
#if (CFG_SUPPORT_802_11BE_MLO == 1) || defined(CFG_SUPPORT_UNIFIED_COMMAND)
			else if (bss->ucGroupMldId == prTargetBss->ucGroupMldId)
				continue;
#endif

			prCcmCsaEntry = cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, sizeof(struct P2P_CCM_CSA_ENTRY));
			if (!prCcmCsaEntry) {
				DBGLOG(CCM, ERROR, "Alloc mem fail\n");
				return;
			}

			prCcmCsaEntry->prBssInfo = bss;
			prCcmCsaEntry->u4TargetCh =
				prTargetBss->ucPrimaryChannel;
			prCcmCsaEntry->eTargetHwBandIdx =
				prTargetBss->eHwBandIdx;
			prCcmCsaEntry->eTargetBand = prTargetBss->eBand;

			if (prFirstSap) {
				LINK_INSERT_BEFORE(prCcmCheckCsList,
						   &prFirstSap->rLinkEntry,
						   &prCcmCsaEntry->rLinkEntry);
			} else {
				LINK_INSERT_TAIL(prCcmCheckCsList,
						 &prCcmCsaEntry->rLinkEntry);
			}

			DBGLOG(CCM, INFO, "insert GO bss=%u waiting to check\n",
			       bss->ucBssIndex);
		}
	} else {
		/* we should still notify SAP even if P2P CCM is disabled */
		DBGLOG(CCM, WARN, "P2P CCM is disabled, skip to ckeck GO\n");
	}

	/* SAP must do CSA last, enqueue SAP at the end */
	for (i = 0; i < MAX_BSSID_NUM; ++i) {
		bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		if (!IS_BSS_APGO(bss) || !IS_BSS_ALIVE(prAdapter, bss)
		    || !p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[
					bss->u4PrivateData]))
			continue;

		/* skip target bss itself */
		if (!fgIsTargetMlo && bss == prTargetBss)
			continue;
#if (CFG_SUPPORT_802_11BE_MLO == 1) || defined(CFG_SUPPORT_UNIFIED_COMMAND)
		else if (bss->ucGroupMldId == prTargetBss->ucGroupMldId)
			continue;
#endif

		prCcmCsaEntry = cnmMemAlloc(prAdapter,
			RAM_TYPE_MSG, sizeof(struct P2P_CCM_CSA_ENTRY));
		if (!prCcmCsaEntry) {
			DBGLOG(CCM, ERROR, "Alloc mem fail\n");
			return;
		}

		prCcmCsaEntry->prBssInfo = bss;
		prCcmCsaEntry->u4TargetCh = prTargetBss->ucPrimaryChannel;
		prCcmCsaEntry->eTargetHwBandIdx = prTargetBss->eHwBandIdx;
		prCcmCsaEntry->eTargetBand = prTargetBss->eBand;

		LINK_INSERT_TAIL(prCcmCheckCsList,
				 &prCcmCsaEntry->rLinkEntry);

		DBGLOG(CCM, INFO, "insert SAP bss=%u waiting to check\n",
		       bss->ucBssIndex);
	}

	ccmChannelSwitchConsumer(prAdapter);
}


/*******************************************************************************
 *                                A + A Related
 *******************************************************************************
 */
uint32_t ccmAABwEnumToValue(uint8_t ucInputChnlBw)
{
	uint32_t u4OutputBw;

	if (ucInputChnlBw > MAX_BW_320_2MHZ)
		return 0;

	if (ucInputChnlBw == MAX_BW_320_1MHZ ||
		ucInputChnlBw == MAX_BW_320_2MHZ)
		u4OutputBw = 20 * BIT(4);
	else
		u4OutputBw = 20 * BIT(ucInputChnlBw);

	return u4OutputBw;
}

void ccmAAForbiddenRegionCal(struct ADAPTER *prAdapter,
			     struct RF_CHANNEL_INFO *rChnlInfo,
			     uint8_t *prForbiddenListLen,
			     uint16_t *prTargetBw,
			     struct CCM_AA_FOBIDEN_REGION_UNIT *arRegionOutput)
{
	uint32_t u4InputBw, u4TargetBw, u4RegionBw;
	uint32_t u4Freq = rChnlInfo->u4CenterFreq1;
	uint8_t  i;

	if (rChnlInfo->ucChnlBw == MAX_BW_320_1MHZ ||
	    rChnlInfo->ucChnlBw == MAX_BW_320_2MHZ)
		u4InputBw = 20 * BIT(4);
	else
		u4InputBw = 20 * BIT(rChnlInfo->ucChnlBw);

	DBGLOG(CCM, TRACE, "input: fc %u, bw %u, target: bw1 %u bw2 %u\n",
	       u4Freq, u4InputBw, prTargetBw[0], prTargetBw[1]);

	*prForbiddenListLen = 1;

	if (u4Freq <= P2P_5G_L_UPPER_BOUND) {
		u4TargetBw = prTargetBw[0];
		u4RegionBw = (u4InputBw + u4TargetBw) / 2;

		arRegionOutput[0].u4BoundForward1 =
			u4Freq + (4 * u4InputBw) + u4RegionBw;
		arRegionOutput[0].u4BoundIsolate =
			u4Freq + P2P_5G_L_H_ISOLATION_WIDTH + u4RegionBw;
		arRegionOutput[0].u4BoundForward2 =
			u4Freq + (4 * u4InputBw) - u4RegionBw;
		arRegionOutput[0].u4BoundInverse1 =
			u4Freq + (4 * u4TargetBw) + u4RegionBw;
		arRegionOutput[0].u4BoundInverse2 =
			u4Freq + (4 * u4TargetBw) - u4RegionBw;

		u4TargetBw = prTargetBw[1];
		if (u4TargetBw) {
			u4RegionBw = (u4InputBw + u4TargetBw) / 2;

			arRegionOutput[1].u4BoundForward1 =
				u4Freq + (4 * u4InputBw) + u4RegionBw;
			arRegionOutput[1].u4BoundIsolate =
				u4Freq + P2P_5G_6G_ISOLATION_WIDTH + u4RegionBw;
			arRegionOutput[1].u4BoundForward2 =
				u4Freq + (4 * u4InputBw) - u4RegionBw;
			arRegionOutput[1].u4BoundInverse1 =
				u4Freq + (4 * u4TargetBw) + u4RegionBw;
			arRegionOutput[1].u4BoundInverse2 =
				u4Freq + (4 * u4TargetBw) - u4RegionBw;

			(*prForbiddenListLen)++;
		}
	} else if (u4Freq <= P2P_5G_H_UPPER_BOUND) {
		u4TargetBw = prTargetBw[0];
		u4RegionBw = (u4InputBw + u4TargetBw) / 2;

		arRegionOutput[0].u4BoundForward1 =
			u4Freq - (4 * u4InputBw) + u4RegionBw;
		arRegionOutput[0].u4BoundForward2 =
			u4Freq - (4 * u4InputBw) - u4RegionBw;
		arRegionOutput[0].u4BoundIsolate =
			u4Freq - P2P_5G_L_H_ISOLATION_WIDTH - u4RegionBw;
		arRegionOutput[0].u4BoundInverse1 =
			u4Freq - (4 * u4TargetBw) + u4RegionBw;
		arRegionOutput[0].u4BoundInverse2 =
			u4Freq - (4 * u4TargetBw) - u4RegionBw;

		u4TargetBw = prTargetBw[1];
		if (u4TargetBw) {
			u4RegionBw = (u4InputBw + u4TargetBw) / 2;

			arRegionOutput[1].u4BoundForward1 =
				u4Freq + (4 * u4InputBw) + u4RegionBw;
			arRegionOutput[1].u4BoundIsolate =
				u4Freq + P2P_5G_6G_ISOLATION_WIDTH + u4RegionBw;
			arRegionOutput[1].u4BoundForward2 =
				u4Freq + (4 * u4InputBw) - u4RegionBw;
			arRegionOutput[1].u4BoundInverse1 =
				u4Freq + (4 * u4TargetBw) + u4RegionBw;
			arRegionOutput[1].u4BoundInverse2 =
				u4Freq + (4 * u4TargetBw) - u4RegionBw;

			(*prForbiddenListLen)++;
		}
	} else {
		u4TargetBw = prTargetBw[0];
		u4RegionBw = (u4InputBw + u4TargetBw) / 2;

		arRegionOutput[0].u4BoundForward1 =
			u4Freq - (4 * u4InputBw) + u4RegionBw;
		arRegionOutput[0].u4BoundForward2 =
			u4Freq - (4 * u4InputBw) - u4RegionBw;
		arRegionOutput[0].u4BoundIsolate =
			u4Freq - P2P_5G_6G_ISOLATION_WIDTH - u4RegionBw;
		arRegionOutput[0].u4BoundInverse1 =
			u4Freq - (4 * u4TargetBw) + u4RegionBw;
		arRegionOutput[0].u4BoundInverse2 =
			u4Freq - (4 * u4TargetBw) - u4RegionBw;
	}

	for (i = 0; i < *prForbiddenListLen; ++i) {
		DBGLOG(CCM, TRACE, "fw1:%u, fw2:%u, inv1:%u, inv2:%u, iso:%u\n",
			arRegionOutput[i].u4BoundForward1,
			arRegionOutput[i].u4BoundForward2,
			arRegionOutput[i].u4BoundInverse1,
			arRegionOutput[i].u4BoundInverse2,
			arRegionOutput[i].u4BoundIsolate);
	}
}

u_int8_t ccmIsPreferAA(struct ADAPTER *prAdapter,
		       struct BSS_INFO *prCsaBss)
{
#if (CONFIG_BAND_NUM > 2)
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	u_int8_t fgIsAaDbdcEnable = prChipInfo->isAaDbdcEnable;
	uint32_t au4AliveBssBitmap[AA_HW_BAND_NUM] = { 0 };

	DBGLOG(CCM, LOUD, "AaDbdcEnable=%u\n", fgIsAaDbdcEnable);

	if (prCsaBss)
		ccmGetOtherAliveBssHwBitmap(prAdapter,
					    au4AliveBssBitmap, prCsaBss);
	else
		bssGetAliveBssHwBitmap(prAdapter, au4AliveBssBitmap);

	if (ENUM_BAND_NUM > 2 &&
	    /* bn2 is available */
	    au4AliveBssBitmap[AA_HW_BAND_1] != 0 &&
	    au4AliveBssBitmap[AA_HW_BAND_2] == 0 &&
	    /* skyhawk sku1 */
	    fgIsAaDbdcEnable)
		return TRUE;
#endif
	return FALSE;
}

bool ccmAAAvailableCheck(struct ADAPTER *prAdapter,
			 struct RF_CHANNEL_INFO *prChnlInfo1,
			 struct RF_CHANNEL_INFO *prChnlInfo2)
{
	uint16_t arTargetBw[2];
	uint8_t prForbiddenListLen;
	struct CCM_AA_FOBIDEN_REGION_UNIT arRegionOutput[2];
	struct RF_CHANNEL_INFO *prChnlInfo_h;
	struct RF_CHANNEL_INFO *prChnlInfo_l;

	if (!ccmIsPreferAA(prAdapter, NULL))
		return FALSE;

	if (prChnlInfo1->eBand == BAND_5G && prChnlInfo2->eBand == BAND_5G) {
		if (prChnlInfo1->u4CenterFreq1 > prChnlInfo2->u4CenterFreq1) {
			prChnlInfo_h = prChnlInfo1;
			prChnlInfo_l = prChnlInfo2;
		} else {
			prChnlInfo_h = prChnlInfo2;
			prChnlInfo_l = prChnlInfo1;
		}
#if (CFG_SUPPORT_WIFI_6G == 1)
	} else if (prChnlInfo1->eBand == BAND_5G &&
		   prChnlInfo2->eBand == BAND_6G) {
		prChnlInfo_h = prChnlInfo2;
		prChnlInfo_l = prChnlInfo1;
	} else if (prChnlInfo1->eBand == BAND_6G &&
		   prChnlInfo2->eBand == BAND_5G) {
		prChnlInfo_h = prChnlInfo1;
		prChnlInfo_l = prChnlInfo2;
#endif /* CFG_SUPPORT_WIFI_6G == 1 */
	} else
		return FALSE;

	arTargetBw[0] = ccmAABwEnumToValue(prChnlInfo_l->ucChnlBw);

	ccmAAForbiddenRegionCal(prAdapter, prChnlInfo_h, &prForbiddenListLen,
				arTargetBw, arRegionOutput);

	if (/* ADC Aliasing Forbidden Spacing */
	    (arRegionOutput[0].u4BoundForward1 <= prChnlInfo_l->u4CenterFreq1 ||
	    arRegionOutput[0].u4BoundForward2 >= prChnlInfo_l->u4CenterFreq1)
	    &&
	    (arRegionOutput[0].u4BoundInverse1 <= prChnlInfo_l->u4CenterFreq1 ||
	    arRegionOutput[0].u4BoundInverse2 >= prChnlInfo_l->u4CenterFreq1)
	    /* Isolation Channel Spacing */
	    &&
	    arRegionOutput[0].u4BoundIsolate > prChnlInfo_l->u4CenterFreq1 &&
	    prChnlInfo_l->u4CenterFreq1 != prChnlInfo_h->u4CenterFreq1)
		return TRUE;
	else
		return FALSE;
}

#endif /* CFG_SUPPORT_CCM */
