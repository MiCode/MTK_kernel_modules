// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "nan_link.c"
 *  \brief
 */

#include "precomp.h"
#include "nan/nan_link.h"
#include "nan_sec.h"

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
struct BSS_INFO *nanGetDefaultLinkBssInfo(
	struct ADAPTER *ad,
	struct BSS_INFO *bss)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;

	if (!ad)
		return bss;

	for (i = 0; i < ad->ucSwBssIdNum; i++) {
		prBssInfo = ad->aprBssInfo[i];

		if (prBssInfo &&
			IS_BSS_NAN(prBssInfo))
			return prBssInfo;
	}

	return bss;
}

u_int8_t nanLinkNeedMlo(
	struct ADAPTER *prAdapter)
{
	if (!prAdapter)
		return FALSE;

	if (prAdapter->rWifiVar.ucNanMldLinkMax <= 1)
		return FALSE;

#if (CFG_SUPPORT_NAN_11BE_MLO == 1)
	return mldIsMultiLinkEnabled(prAdapter,
		NETWORK_TYPE_NAN, FALSE);
#else
	return FALSE;
#endif
}

uint8_t
nanGetBssIdxbyLink(
	struct ADAPTER *prAdapter,
	uint8_t ucLinkIdx)
{
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo;

	if (ucLinkIdx >= NAN_LINK_NUM)
		ucLinkIdx = NAN_MAIN_LINK_INDEX;

	prNANSpecInfo = nanGetSpecificBssInfo(prAdapter,
		ucLinkIdx);

	if (prNANSpecInfo)
		return prNANSpecInfo->ucBssIndex;

	return 0;
}

enum NAN_BSS_ROLE_INDEX
nanGetRoleIndexbyLink(
	uint8_t ucLinkIdx)
{
#if (CFG_SUPPORT_NAN_DBDC == 1)
	if (ucLinkIdx > NAN_MAIN_LINK_INDEX)
		return NAN_BSS_INDEX_BAND1;
#endif

	return NAN_BSS_INDEX_BAND0;
}

uint8_t nanGetLinkIndexbyRole(enum NAN_BSS_ROLE_INDEX eRole)
{
#if (CFG_SUPPORT_NAN_11BE_MLO == 1)
	/* TBD */
	if (eRole > NAN_BSS_INDEX_BAND0)
		return NAN_HIGH_LINK_INDEX;
#endif

	return NAN_MAIN_LINK_INDEX;
}

uint8_t
nanGetLinkIndexbyBand(
	struct ADAPTER *ad,
	enum ENUM_BAND eBand)
{
	uint8_t ucIdx = 0;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo;
	struct BSS_INFO *prBssInfo;

	/* Use default BSS if can't find correct peerSchRec or no band info */
	if (eBand == BAND_NULL) {
		DBGLOG(NAN, WARN, "no band info\n");
		prNANSpecInfo = nanGetSpecificBssInfo(
			ad, NAN_BSS_INDEX_BAND0);
		return NAN_MAIN_LINK_INDEX;
	}

	for (ucIdx = 0; ucIdx < NAN_BSS_INDEX_NUM; ucIdx++) {
		prNANSpecInfo = nanGetSpecificBssInfo(ad, ucIdx);
		prBssInfo = GET_BSS_INFO_BY_INDEX(
			ad,
			prNANSpecInfo->ucBssIndex);

		if ((prBssInfo != NULL) &&
			(prBssInfo->eBand == eBand) &&
			(ucIdx < ad->rWifiVar.ucNanMldLinkMax)) {
			DBGLOG(NAN, INFO,
				"Band%d, Idx%d\n",
				eBand, ucIdx);
			return ucIdx;
		}
	}

	return NAN_MAIN_LINK_INDEX;
}

uint8_t nanGetLinkIndexbyOpClass(
	struct ADAPTER *prAdapter,
	uint32_t op)
{
#if (CFG_SUPPORT_NAN_11BE_MLO == 1)
	/* TBD */
	if (!IS_2G_OP_CLASS(op) &&
		nanLinkNeedMlo(prAdapter))
		return NAN_HIGH_LINK_INDEX;
#endif

	return NAN_MAIN_LINK_INDEX;
}

void nanGetLinkWmmQueSet(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	struct BSS_INFO *bss;

	/* main bss must assign wmm first */
	bss = nanGetDefaultLinkBssInfo(prAdapter, prBssInfo);

	cnmWmmIndexDecision(prAdapter, bss);

#if (CFG_SUPPORT_802_11BE_MLO == 1) && (CFG_SUPPORT_CONNAC3X == 1)
	/* connac3 MLO all bss use the same wmm index as main bss use */
	if (TRUE /*nanLinkNeedMlo(prAdapter)*/) {
		prBssInfo->fgIsWmmInited = TRUE;
		prBssInfo->ucWmmQueSet = bss->ucWmmQueSet;

		/* if (bss != prBssInfo)
		 *	prBssInfo->ucOwnMacIndex = bss->ucOwnMacIndex;
		 */
	} else
#endif
	{
		/* connac2 always assign different wmm index to bssinfo */
		cnmWmmIndexDecision(prAdapter, prBssInfo);
	}

	DBGLOG(NAN, DEBUG, "bss%d, wmm=%d, omac=%d\n",
		prBssInfo->ucBssIndex,
		prBssInfo->ucWmmQueSet,
		prBssInfo->ucOwnMacIndex);
}

u_int8_t nanGetStaRecExist(
	struct _NAN_NDP_CONTEXT_T *cxt,
	struct STA_RECORD *sta)
{
	uint32_t i = 0;

	if (!cxt) {
		DBGLOG(NAN, ERROR,
			"cxt error\n");
		return FALSE;
	}

	for (i = 0; i < NAN_LINK_NUM; i++) {
		if (cxt->prNanStaRec[i] == sta)
			return TRUE;
	}

	return FALSE;
}

void nanResetStaRec(
	struct _NAN_NDP_CONTEXT_T *cxt)
{
	uint32_t i = 0;

	if (!cxt) {
		DBGLOG(NAN, ERROR,
			"cxt error\n");
		return;
	}

	for (i = 0; i < NAN_LINK_NUM; i++)
		cxt->prNanStaRec[i] = NULL;
}

void nanDumpStaRec(
	struct _NAN_NDP_CONTEXT_T *cxt)
{
	uint32_t i = 0;

	if (!cxt) {
		DBGLOG(NAN, ERROR,
			"cxt error\n");
		return;
	}

	for (i = 0; i < NAN_LINK_NUM; i++) {
		uint8_t ucIndex = STA_REC_INDEX_NOT_FOUND;
		uint8_t ucBssIndex = MAX_BSSID_NUM;
		struct STA_RECORD *s = NULL;

		s = cxt->prNanStaRec[i];

		if (s) {
			ucIndex = s->ucIndex;
			ucBssIndex = s->ucBssIndex;
		}

		DBGLOG(NAN, DEBUG,
			"CxtId:%d, Sta:%d, Bss:%d, Enrollee:%d\n",
			cxt->ucId,
			ucIndex,
			ucBssIndex,
			cxt->ucNumEnrollee);
	}
}

struct STA_RECORD *nanGetLinkStaRec(
	struct _NAN_NDP_CONTEXT_T *cxt,
	uint8_t ucLinkIdx)
{
	if (!cxt ||
		(ucLinkIdx >= NAN_LINK_NUM))
		return NULL;

	return cxt->prNanStaRec[ucLinkIdx];
}

void nanSetLinkStaRec(
	struct _NAN_NDP_CONTEXT_T *cxt,
	struct STA_RECORD *sta,
	uint8_t ucLinkIdx)
{
	if (ucLinkIdx >= NAN_LINK_NUM)
		return;

	cxt->prNanStaRec[ucLinkIdx] = sta;
}

uint32_t nanSetPreferLinkStaRec(
	struct ADAPTER *ad,
	struct _NAN_NDP_CONTEXT_T *cxt,
	uint8_t idx)
{
	uint32_t i = 0;
	struct STA_RECORD *sta = NULL;

	if (!cxt)
		return WLAN_STATUS_FAILURE;

	/* Reset */
	cxt->prNanPreferStaRec = NULL;

	for (i = 0;
		i < ad->rWifiVar.ucNanMldLinkMax;
		i++) {
		sta = nanGetLinkStaRec(cxt, i);
		if (!sta) {
			DBGLOG(NAN, ERROR,
				"prNanStaRec error\n");
			return WLAN_STATUS_FAILURE;
		}

		DBGLOG(NAN, DEBUG,
			"Check sta%d, bss%d\n",
			sta->ucWlanIndex,
			sta->ucBssIndex);

		if (sta->ucBssIndex == idx) {
			cxt->prNanPreferStaRec = sta;
			DBGLOG(NAN, INFO,
				"Prefer sta %d\n",
				sta->ucWlanIndex);
			return WLAN_STATUS_SUCCESS;
		}
	}

	return WLAN_STATUS_FAILURE;
}

struct STA_RECORD *nanGetPreferLinkStaRec(
	struct ADAPTER *ad,
	struct _NAN_NDP_CONTEXT_T *cxt)
{
	if (!cxt)
		return NULL;

	if (cxt->prNanPreferStaRec)
		return cxt->prNanPreferStaRec;

	DBGLOG(NAN, WARN,
		"Prefer sta is null\n");
	return nanGetLinkStaRec(cxt,
		NAN_MAIN_LINK_INDEX);
}

u_int8_t
nanIsTxKeyReady(
	struct _NAN_NDP_CONTEXT_T *cxt)
{
	struct STA_RECORD *s = NULL;

	if (!cxt)
		return FALSE;

	s = cxt->prNanStaRec[NAN_MAIN_LINK_INDEX];

	if (s)
		return s->fgIsTxKeyReady;

	return FALSE;
}

uint32_t nanLinkResetTk(
	struct ADAPTER *ad,
	struct _NAN_NDP_CONTEXT_T *cxt)
{
	uint32_t i = 0;
	struct STA_RECORD *prNanStaRec = NULL;
	struct _NAN_NDP_INSTANCE_T *prTargetNdpSA;

	if (!cxt)
		return WLAN_STATUS_FAILURE;

	prTargetNdpSA = cxt->aprEnrollNdp[1];

	for (i = 0;
		i < ad->rWifiVar.ucNanMldLinkMax;
		i++) {
		prNanStaRec = nanGetLinkStaRec(cxt, i);
		if (!prNanStaRec) {
			DBGLOG(NAN, ERROR,
				"prNanStaRec error\n");
			return WLAN_STATUS_FAILURE;
		}

		if (!prTargetNdpSA->fgSecurityRequired)
			nanSecResetTk(prNanStaRec);
		else if (ad->rWifiVar.fgNanUnrollInstallTk)
			nanSecInstallTk(prTargetNdpSA,
					prNanStaRec);
	}

	return WLAN_STATUS_SUCCESS;
}

#if (CFG_SUPPORT_NAN_11BE_MLO == 1)
struct MLD_BSS_INFO *gprNanMldBssInfo;

void nanMldBssInit(struct ADAPTER *prAdapter)
{
	if (nanLinkNeedMlo(prAdapter)) {
		if (gprNanMldBssInfo == NULL) {
			struct BSS_INFO *bss;

			/* main bss must assign wmm first */
			bss = nanGetDefaultLinkBssInfo(
				prAdapter,
				NULL);
			if (bss) {
				DBGLOG(INIT, TRACE, "\n");
				gprNanMldBssInfo =
					mldBssAlloc(prAdapter,
					bss->aucOwnMacAddr);
			}
		}
	}
}

void nanMldBssUninit(struct ADAPTER *prAdapter)
{
	if (gprNanMldBssInfo != NULL &&
	    gprNanMldBssInfo->rBssList.u4NumElem == 0) {
		mldBssFree(prAdapter, gprNanMldBssInfo);
		gprNanMldBssInfo = NULL;
		DBGLOG(INIT, TRACE, "\n");
	}
}

void nanMldBssRegister(struct ADAPTER *prAdapter,
	struct BSS_INFO *prNanBssInfo)
{
	struct MLD_BSS_INFO *prMldBssInfo;

	do {
		nanMldBssInit(prAdapter);

		prMldBssInfo = gprNanMldBssInfo;
		if (!prMldBssInfo) {
			DBGLOG(NAN, ERROR,
				"Error allocating mld bss\n");
			break;
		}

		if (nanLinkNeedMlo(prAdapter)) {
			prNanBssInfo->ucLinkId =
				prMldBssInfo->rBssList.u4NumElem;
		}

		mldBssRegister(prAdapter,
			prMldBssInfo,
			prNanBssInfo);
	} while (FALSE);
}

void nanMldBssUnregister(struct ADAPTER *prAdapter,
	struct BSS_INFO *prNanBssInfo)
{
	struct MLD_BSS_INFO *prMldBssInfo;

	do {
		prMldBssInfo = gprNanMldBssInfo;
		if (!prMldBssInfo) {
			DBGLOG(NAN, ERROR,
				"Error allocating mld bss\n");
			break;
		}

		mldBssUnregister(prAdapter, prMldBssInfo, prNanBssInfo);

		nanMldBssUninit(prAdapter);
	} while (FALSE);
}

void nanMldStaRecRegister(struct ADAPTER *prAdapter,
			  struct STA_RECORD *prStaRec,
			  uint8_t ucLinkIndex)
{
	struct MLD_STA_RECORD *prMldStarec = NULL;
	struct MLD_BSS_INFO *prMldBssInfo = gprNanMldBssInfo;

	if (!prMldBssInfo || !prStaRec) {
		DBGLOG(NAN, ERROR, "Mld bss is null\n");
		return;
	}

	prMldStarec = mldStarecGetByMldAddr(prAdapter, prMldBssInfo,
					    prStaRec->aucMacAddr);
	if (!prMldStarec) {
		struct ML_INFO rMlInfo = {0};

		COPY_MAC_ADDR(rMlInfo.aucMldAddr, prStaRec->aucMacAddr);
		rMlInfo.u2EmlCap = 0;
		rMlInfo.u2MldCap = 0;
		rMlInfo.fgMldType = MLD_TYPE_ICV_METHOD_V2;

		prMldStarec = mldStarecAlloc(prAdapter, prMldBssInfo, &rMlInfo);
		if (!prMldStarec) {
			DBGLOG(NAN, ERROR, "Can't alloc mldstarec!\n");
			return;
		}
	}

	mldStarecRegister(prAdapter, prMldStarec, prStaRec, ucLinkIndex);

	mldStarecSetSetupIdx(prAdapter, prStaRec);

	nanUpdateMbmcIdx(prAdapter, prStaRec->ucBssIndex, ucLinkIndex);
}
#endif

