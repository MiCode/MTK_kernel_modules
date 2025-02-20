// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "precomp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*
 * definition for AP selection algrithm
 */
#define BSS_FULL_SCORE                          (100)
#define CHNL_BSS_NUM_THRESOLD                   100
#define BSS_STA_CNT_THRESOLD                    30
#define SCORE_PER_AP                            1
#define ROAMING_NO_SWING_SCORE_STEP             100
/* MCS9 at BW 160 requires rssi at least -48dbm */
#define BEST_RSSI                               -48
/* MCS7 at 20BW, MCS5 at 40BW, MCS4 at 80BW, MCS3 at 160BW */
#define GOOD_RSSI_FOR_HT_VHT                    -64
/* Link speed 1Mbps need at least rssi -94dbm for 2.4G */
#define MINIMUM_RSSI_2G4                        -94
/* Link speed 6Mbps need at least rssi -86dbm for 5G */
#define MINIMUM_RSSI_5G                         -86
#if (CFG_SUPPORT_WIFI_6G == 1)
/* Link speed 6Mbps need at least rssi -86dbm for 6G */
#define MINIMUM_RSSI_6G                         -86
#endif

/* level of rssi range on StatusBar */
#define RSSI_MAX_LEVEL                          -55
#define RSSI_SECOND_LEVEL                       -66

/* Real Rssi of a Bss may range in current_rssi - 5 dbm
 *to current_rssi + 5 dbm
 */
#define RSSI_DIFF_BETWEEN_BAND                  15 /*dbm */
#define RSSI_DIFF_BETWEEN_BSS                   10 /* dbm */
#define LOW_RSSI_FOR_5G_BAND                    -70 /* dbm */
#define HIGH_RSSI_FOR_5G_BAND                   -60 /* dbm */

#define CHNL_DWELL_TIME_DEFAULT  100
#define CHNL_DWELL_TIME_ONLINE   50

#define WEIGHT_IDX_CHNL_UTIL                    0
#define WEIGHT_IDX_RSSI                         2
#define WEIGHT_IDX_SCN_MISS_CNT                 2
#define WEIGHT_IDX_PROBE_RSP                    1
#define WEIGHT_IDX_CLIENT_CNT                   0
#define WEIGHT_IDX_AP_NUM                       0
#define WEIGHT_IDX_5G_BAND                      2
#define WEIGHT_IDX_BAND_WIDTH                   1
#define WEIGHT_IDX_STBC                         1
#define WEIGHT_IDX_DEAUTH_LAST                  1
#define WEIGHT_IDX_BLOCK_LIST                   2
#define WEIGHT_IDX_SAA                          0
#define WEIGHT_IDX_CHNL_IDLE                    1
#define WEIGHT_IDX_OPCHNL                       0
#define WEIGHT_IDX_TPUT                         1
#define WEIGHT_IDX_PREFERENCE                   2

#define WEIGHT_IDX_CHNL_UTIL_PER                0
#define WEIGHT_IDX_RSSI_PER                     4
#define WEIGHT_IDX_SCN_MISS_CNT_PER             4
#define WEIGHT_IDX_PROBE_RSP_PER                1
#define WEIGHT_IDX_CLIENT_CNT_PER               1
#define WEIGHT_IDX_AP_NUM_PER                   6
#define WEIGHT_IDX_5G_BAND_PER                  4
#define WEIGHT_IDX_BAND_WIDTH_PER               1
#define WEIGHT_IDX_STBC_PER                     1
#define WEIGHT_IDX_DEAUTH_LAST_PER              1
#define WEIGHT_IDX_BLOCK_LIST_PER               4
#define WEIGHT_IDX_SAA_PER                      1
#define WEIGHT_IDX_CHNL_IDLE_PER                6
#define WEIGHT_IDX_OPCHNL_PER                   6
#define WEIGHT_IDX_TPUT_PER                     2
#define WEIGHT_IDX_PREFERENCE_PER               2

#define WEIGHT_MCC_DOWNGRADE			70 /* 0~100 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct WEIGHT_CONFIG {
	uint8_t ucChnlUtilWeight;
	uint8_t ucSnrWeight;
	uint8_t ucRssiWeight;
	uint8_t ucScanMissCntWeight;
	uint8_t ucProbeRespWeight;
	uint8_t ucClientCntWeight;
	uint8_t ucApNumWeight;
	uint8_t ucBandWeight;
	uint8_t ucBandWidthWeight;
	uint8_t ucStbcWeight;
	uint8_t ucLastDeauthWeight;
	uint8_t ucBlockListWeight;
	uint8_t ucSaaWeight;
	uint8_t ucChnlIdleWeight;
	uint8_t ucOpchnlWeight;
	uint8_t ucTputWeight;
	uint8_t ucPreferenceWeight;
};

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static uint8_t scanNetworkReplaceHandler2G4(enum ENUM_BAND eCurrentBand,
	int8_t cCandidateRssi, int8_t cCurrentRssi);
static uint8_t scanNetworkReplaceHandler5G(enum ENUM_BAND eCurrentBand,
	int8_t cCandidateRssi, int8_t cCurrentRssi);
#if (CFG_SUPPORT_WIFI_6G == 1)
static uint8_t scanNetworkReplaceHandler6G(enum ENUM_BAND eCurrentBand,
	int8_t cCandidateRssi, int8_t cCurrentRssi);
#endif

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */


struct WEIGHT_CONFIG gasMtkWeightConfig[ROAM_TYPE_NUM] = {
	[ROAM_TYPE_RCPI] = {
		.ucChnlUtilWeight = WEIGHT_IDX_CHNL_UTIL,
		.ucRssiWeight = WEIGHT_IDX_RSSI,
		.ucScanMissCntWeight = WEIGHT_IDX_SCN_MISS_CNT,
		.ucProbeRespWeight = WEIGHT_IDX_PROBE_RSP,
		.ucClientCntWeight = WEIGHT_IDX_CLIENT_CNT,
		.ucApNumWeight = WEIGHT_IDX_AP_NUM,
		.ucBandWeight = WEIGHT_IDX_5G_BAND,
		.ucBandWidthWeight = WEIGHT_IDX_BAND_WIDTH,
		.ucStbcWeight = WEIGHT_IDX_STBC,
		.ucLastDeauthWeight = WEIGHT_IDX_DEAUTH_LAST,
		.ucBlockListWeight = WEIGHT_IDX_BLOCK_LIST,
		.ucSaaWeight = WEIGHT_IDX_SAA,
		.ucChnlIdleWeight = WEIGHT_IDX_CHNL_IDLE,
		.ucOpchnlWeight = WEIGHT_IDX_OPCHNL,
		.ucTputWeight = WEIGHT_IDX_TPUT,
		.ucPreferenceWeight = WEIGHT_IDX_PREFERENCE
	}
#if CFG_SUPPORT_ROAMING
	, [ROAM_TYPE_PER] = {
		.ucChnlUtilWeight = WEIGHT_IDX_CHNL_UTIL_PER,
		.ucRssiWeight = WEIGHT_IDX_RSSI_PER,
		.ucScanMissCntWeight = WEIGHT_IDX_SCN_MISS_CNT_PER,
		.ucProbeRespWeight = WEIGHT_IDX_PROBE_RSP_PER,
		.ucClientCntWeight = WEIGHT_IDX_CLIENT_CNT_PER,
		.ucApNumWeight = WEIGHT_IDX_AP_NUM_PER,
		.ucBandWeight = WEIGHT_IDX_5G_BAND_PER,
		.ucBandWidthWeight = WEIGHT_IDX_BAND_WIDTH_PER,
		.ucStbcWeight = WEIGHT_IDX_STBC_PER,
		.ucLastDeauthWeight = WEIGHT_IDX_DEAUTH_LAST_PER,
		.ucBlockListWeight = WEIGHT_IDX_BLOCK_LIST_PER,
		.ucSaaWeight = WEIGHT_IDX_SAA_PER,
		.ucChnlIdleWeight = WEIGHT_IDX_CHNL_IDLE_PER,
		.ucOpchnlWeight = WEIGHT_IDX_OPCHNL_PER,
		.ucTputWeight = WEIGHT_IDX_TPUT_PER,
		.ucPreferenceWeight = WEIGHT_IDX_PREFERENCE_PER
	}
#endif
};

struct NETWORK_SELECTION_POLICY_BY_BAND networkReplaceHandler[BAND_NUM] = {
	[BAND_2G4] = {BAND_2G4, scanNetworkReplaceHandler2G4},
	[BAND_5G]  = {BAND_5G,  scanNetworkReplaceHandler5G},
#if (CFG_SUPPORT_WIFI_6G == 1)
	[BAND_6G]  = {BAND_6G,  scanNetworkReplaceHandler6G},
#endif
};

#if (CFG_SUPPORT_AVOID_DESENSE == 1)
const struct WFA_DESENSE_CHANNEL_LIST desenseChList[BAND_NUM] = {
	[BAND_5G]  = {120, 157},
#if (CFG_SUPPORT_WIFI_6G == 1)
	[BAND_6G]  = {13,  53},
#endif
};
#endif

static enum ROAM_TYPE roamReasonToType(enum ENUM_ROAMING_REASON type)
{
	enum ROAM_TYPE ret = ROAM_TYPE_RCPI;

	if (type >= ROAMING_REASON_NUM)
		return ret;
#if CFG_SUPPORT_ROAMING
	if (type == ROAMING_REASON_TX_ERR)
		ret = ROAM_TYPE_PER;
#endif
	return ret;
}
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define CALCULATE_SCORE_BY_PROBE_RSP(prBssDesc, eRoamType) \
	(gasMtkWeightConfig[eRoamType].ucProbeRespWeight * \
	(prBssDesc->fgSeenProbeResp ? BSS_FULL_SCORE : 0))

#define CALCULATE_SCORE_BY_MISS_CNT(prAdapter, prBssDesc, eRoamType) \
	(gasMtkWeightConfig[eRoamType].ucScanMissCntWeight * \
	(prAdapter->rWifiVar.rScanInfo.u4ScanUpdateIdx - \
	prBssDesc->u4UpdateIdx > 3 ? 0 : \
	(BSS_FULL_SCORE - (prAdapter->rWifiVar.rScanInfo.u4ScanUpdateIdx - \
	prBssDesc->u4UpdateIdx) * 25)))

#if (CFG_SUPPORT_WIFI_6G == 1)
#define CALCULATE_SCORE_BY_BAND(prAdapter, prBssDesc, cRssi, eRoamType) \
	(gasMtkWeightConfig[eRoamType].ucBandWeight * \
	((((prBssDesc->eBand == BAND_5G && prAdapter->fgEnable5GBand) || \
	   (prBssDesc->eBand == BAND_6G && prAdapter->fgIsHwSupport6G)) && \
	cRssi > -70) ? BSS_FULL_SCORE : 0))
#else
#define CALCULATE_SCORE_BY_BAND(prAdapter, prBssDesc, cRssi, eRoamType) \
	(gasMtkWeightConfig[eRoamType].ucBandWeight * \
	((prBssDesc->eBand == BAND_5G && prAdapter->fgEnable5GBand && \
	cRssi > -70) ? BSS_FULL_SCORE : 0))
#endif

#define CALCULATE_SCORE_BY_STBC(prAdapter, prBssDesc, eRoamType) \
	(gasMtkWeightConfig[eRoamType].ucStbcWeight * \
	(prBssDesc->fgMultiAnttenaAndSTBC ? BSS_FULL_SCORE:0))

#define CALCULATE_SCORE_BY_DEAUTH(prBssDesc, eRoamType) \
	(gasMtkWeightConfig[eRoamType].ucLastDeauthWeight * \
	(prBssDesc->prBlock && prBssDesc->prBlock->fgDeauthLastTime ? 0 : \
	BSS_FULL_SCORE))

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#if 0
static uint16_t scanCaculateScoreBySTBC(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;
	uint8_t ucSpatial = 0;

	ucSpatial = prBssDesc->ucSpatial;

	if (prBssDesc->fgMultiAnttenaAndSTBC) {
		ucSpatial = (ucSpatial >= 4)?4:ucSpatial;
		u2Score = (BSS_FULL_SCORE-50)*ucSpatial;
	} else {
		u2Score = 0;
	}
	return u2Score*mtk_weight_config[eRoamType].ucStbcWeight;
}
#endif

void scanCheckConcurrent(struct ADAPTER *ad, struct BSS_DESC *bss,
	uint8_t bidx)
{
	struct BSS_INFO *bssinfo;
	uint32_t bmap = aisGetBssIndexBmap(aisGetAisFsmInfo(ad, bidx));
	uint8_t i;

	/* SCC */
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		/* Is connected BssInfo */
		if (BIT(i) & bmap)
			continue;

		bssinfo = GET_BSS_INFO_BY_INDEX(ad, i);
		if (!bssinfo || !IS_BSS_ALIVE(ad, bssinfo))
			continue;

		if (bss->eBand == bssinfo->eBand &&
		    bss->ucChannelNum == bssinfo->ucPrimaryChannel) {
			bss->fgIsMCC = FALSE;
			return;
		}
	}

	/* MCC */
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		/* Is connected BssInfo */
		if (BIT(i) & bmap)
			continue;

		bssinfo = GET_BSS_INFO_BY_INDEX(ad, i);
		if (!bssinfo || !IS_BSS_ALIVE(ad, bssinfo))
			continue;

		/* G band */
		if (bss->eBand == BAND_2G4 && bssinfo->eBand == BAND_2G4) {
			if (bss->ucChannelNum != bssinfo->ucPrimaryChannel) {
				bss->fgIsMCC = TRUE;
				return;
			}
		}

		/* A band */
		if (bss->eBand != BAND_2G4 && bssinfo->eBand != BAND_2G4) {
			if (bss->eBand != bssinfo->eBand ||
			    bss->ucChannelNum != bssinfo->ucPrimaryChannel) {
				bss->fgIsMCC = TRUE;
				return;
			}
		}
	}

	bss->fgIsMCC = FALSE;
}

/* Channel Utilization: weight index will be */
static uint16_t scanCalculateScoreByChnlInfo(
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo, uint8_t ucChannel,
	enum ROAM_TYPE eRoamType)
{
	struct ESS_CHNL_INFO *prEssChnlInfo = &prAisSpecificBssInfo->
		arCurEssChnlInfo[0];
	uint8_t i = 0;
	uint16_t u2Score = 0;
	uint8_t weight = 0;

	if (eRoamType < 0 || eRoamType >= ROAM_TYPE_NUM) {
		log_dbg(SCN, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

	weight = gasMtkWeightConfig[eRoamType].ucApNumWeight;

	for (; i < prAisSpecificBssInfo->ucCurEssChnlInfoNum; i++) {
		if (ucChannel == prEssChnlInfo[i].ucChannel) {
#if 0	/* currently, we don't take channel utilization into account */
			/* the channel utilization max value is 255.
			 *great utilization means little weight value.
			 * the step of weight value is 2.6
			 */
			u2Score = mtk_weight_config[eRoamType].
				ucChnlUtilWeight * (BSS_FULL_SCORE -
				(prEssChnlInfo[i].ucUtilization * 10 / 26));
#endif
			/* if AP num on this channel is greater than 100,
			 * the weight will be 0.
			 * otherwise, the weight value decrease 1
			 * if AP number increase 1
			 */
			if (prEssChnlInfo[i].ucApNum <= CHNL_BSS_NUM_THRESOLD)
				u2Score += weight *
				(BSS_FULL_SCORE - prEssChnlInfo[i].ucApNum *
					SCORE_PER_AP);
			log_dbg(SCN, TRACE, "channel %d, AP num %d\n",
				ucChannel, prEssChnlInfo[i].ucApNum);
			break;
		}
	}
	return u2Score;
}

static uint16_t scanCalculateScoreByBandwidth(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;
	enum ENUM_CHANNEL_WIDTH eChannelWidth = prBssDesc->eChannelWidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucSta6GBW = prAdapter->rWifiVar.ucSta6gBandwidth;
#endif
	uint8_t ucSta5GBW = prAdapter->rWifiVar.ucSta5gBandwidth;
	uint8_t ucSta2GBW = prAdapter->rWifiVar.ucSta2gBandwidth;
	uint8_t ucStaBW = prAdapter->rWifiVar.ucStaBandwidth;

	if (eRoamType < 0 || eRoamType >= ROAM_TYPE_NUM) {
		log_dbg(SCN, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

	if (prBssDesc->fgIsVHTPresent && prAdapter->fgEnable5GBand) {
		if (ucSta5GBW > ucStaBW)
			ucSta5GBW = ucStaBW;
		switch (ucSta5GBW) {
		case MAX_BW_20MHZ:
		case MAX_BW_40MHZ:
			eChannelWidth = CW_20_40MHZ;
			break;
		case MAX_BW_80MHZ:
			eChannelWidth = CW_80MHZ;
			break;
		}
		switch (eChannelWidth) {
		case CW_20_40MHZ:
			u2Score = 60;
			break;
		case CW_80MHZ:
			u2Score = 80;
			break;
		case CW_160MHZ:
		case CW_80P80MHZ:
		case CW_320_1MHZ:
		case CW_320_2MHZ:
			u2Score = BSS_FULL_SCORE;
			break;
		default:
			break;
		}
	} else if (prBssDesc->fgIsHTPresent) {
		if (prBssDesc->eBand == BAND_2G4) {
			if (ucSta2GBW > ucStaBW)
				ucSta2GBW = ucStaBW;
			u2Score = (prBssDesc->eSco == 0 ||
					ucSta2GBW == MAX_BW_20MHZ) ? 40:60;
		} else if (prBssDesc->eBand == BAND_5G) {
			if (ucSta5GBW > ucStaBW)
				ucSta5GBW = ucStaBW;
			u2Score = (prBssDesc->eSco == 0 ||
					ucSta5GBW == MAX_BW_20MHZ) ? 40:60;
		}
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (prBssDesc->eBand == BAND_6G) {
			if (ucSta6GBW > ucStaBW)
				ucSta6GBW = ucStaBW;
			u2Score = (prBssDesc->eSco == 0 ||
				ucSta6GBW == MAX_BW_20MHZ) ? 40:60;
		}
#endif
	} else if (prBssDesc->u2BSSBasicRateSet & RATE_SET_OFDM)
		u2Score = 20;
	else
		u2Score = 10;

	return u2Score * gasMtkWeightConfig[eRoamType].ucBandWidthWeight;
}

static uint16_t scanCalculateScoreByClientCnt(struct BSS_DESC *prBssDesc,
			enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;
	uint16_t u2StaCnt = 0;
#define BSS_STA_CNT_NORMAL_SCORE 50
#define BSS_STA_CNT_GOOD_THRESOLD 10

	log_dbg(SCN, TRACE, "Exist bss load %d, sta cnt %d\n",
			prBssDesc->fgExistBssLoadIE, prBssDesc->u2StaCnt);

	if (eRoamType < 0 || eRoamType >= ROAM_TYPE_NUM) {
		log_dbg(SCN, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

	if (!prBssDesc->fgExistBssLoadIE) {
		u2Score = BSS_STA_CNT_NORMAL_SCORE;
		return u2Score *
		gasMtkWeightConfig[eRoamType].ucClientCntWeight;
	}

	u2StaCnt = prBssDesc->u2StaCnt;
	if (u2StaCnt > BSS_STA_CNT_THRESOLD)
		u2Score = 0;
	else if (u2StaCnt < BSS_STA_CNT_GOOD_THRESOLD)
		u2Score = BSS_FULL_SCORE - u2StaCnt;
	else
		u2Score = BSS_STA_CNT_NORMAL_SCORE;

	return u2Score * gasMtkWeightConfig[eRoamType].ucClientCntWeight;
}

#if CFG_SUPPORT_802_11K
struct NEIGHBOR_AP *scanGetNeighborAPEntry(
	struct ADAPTER *prAdapter, uint8_t *pucBssid, uint8_t ucBssIndex)
{
	struct LINK *prNeighborAPLink =
		&aisGetAisSpecBssInfo(prAdapter, ucBssIndex)
		->rNeighborApList.rUsingLink;
	struct NEIGHBOR_AP *prNeighborAP = NULL;

	LINK_FOR_EACH_ENTRY(prNeighborAP, prNeighborAPLink, rLinkEntry,
			    struct NEIGHBOR_AP)
	{
		if (EQUAL_MAC_ADDR(prNeighborAP->aucBssid, pucBssid))
			return prNeighborAP;
	}
	return NULL;
}
#endif

static uint8_t scanNetworkReplaceHandler2G4(enum ENUM_BAND eCurrentBand,
	int8_t cCandidateRssi, int8_t cCurrentRssi)
{
	/* Current AP is 2.4G, replace candidate AP if target AP is good */
	if (eCurrentBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
		|| eCurrentBand == BAND_6G
#endif
		) {
		if (cCurrentRssi >= GOOD_RSSI_FOR_HT_VHT)
			return TRUE;

		if (cCurrentRssi < LOW_RSSI_FOR_5G_BAND &&
			(cCandidateRssi > cCurrentRssi + 5))
			return FALSE;
	}
	return FALSE;
}

static uint8_t scanNetworkReplaceHandler5G(enum ENUM_BAND eCurrentBand,
	int8_t cCandidateRssi, int8_t cCurrentRssi)
{
	/* Candidate AP is 5G, don't replace it if it's good enough. */
	if (eCurrentBand == BAND_2G4) {
		if (cCandidateRssi >= GOOD_RSSI_FOR_HT_VHT)
			return FALSE;

		if (cCandidateRssi < LOW_RSSI_FOR_5G_BAND &&
			(cCurrentRssi > cCandidateRssi + 5))
			return TRUE;
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (eCurrentBand == BAND_6G) {
		/* Target AP is 6G, replace candidate AP if target AP is good */
		if (cCurrentRssi >= GOOD_RSSI_FOR_HT_VHT)
			return TRUE;

		if (cCurrentRssi < LOW_RSSI_FOR_5G_BAND &&
			(cCandidateRssi > cCurrentRssi + 5))
			return FALSE;
	}
#endif
	return FALSE;
}

#if (CFG_SUPPORT_WIFI_6G == 1)
static uint8_t scanNetworkReplaceHandler6G(enum ENUM_BAND eCurrentBand,
	int8_t cCandidateRssi, int8_t cCurrentRssi)
{
	if (eCurrentBand < BAND_2G4 || eCurrentBand > BAND_6G)
		return FALSE;

	/* Candidate AP is 6G, don't replace it if it's good enough. */
	if (cCandidateRssi >= GOOD_RSSI_FOR_HT_VHT)
		return FALSE;

	if (cCandidateRssi < LOW_RSSI_FOR_5G_BAND &&
		(cCurrentRssi > cCandidateRssi + 7))
		return TRUE;

	return FALSE;
}
#endif

static u_int8_t scanNeedReplaceCandidate(struct ADAPTER *prAdapter,
	struct BSS_DESC *prCandBss, struct BSS_DESC *prCurrBss,
	uint16_t u2CandScore, uint16_t u2CurrScore,
	enum ENUM_ROAMING_REASON eRoamReason, uint8_t ucBssIndex)
{
	int8_t cCandRssi = RCPI_TO_dBm(prCandBss->ucRCPI);
	int8_t cCurrRssi = RCPI_TO_dBm(prCurrBss->ucRCPI);
	uint32_t u4UpdateIdx = prAdapter->rWifiVar.rScanInfo.u4ScanUpdateIdx;
	uint16_t u2CandMiss = u4UpdateIdx - prCandBss->u4UpdateIdx;
	uint16_t u2CurrMiss = u4UpdateIdx - prCurrBss->u4UpdateIdx;
	struct BSS_DESC *prBssDesc = NULL;
	int8_t ucOpChannelNum = 0;
	enum ROAM_TYPE eRoamType = roamReasonToType(eRoamReason);

	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
	if (prBssDesc)
		ucOpChannelNum = prBssDesc->ucChannelNum;

#if CFG_SUPPORT_NCHO
	if (prAdapter->rNchoInfo.fgNCHOEnabled)
		return cCurrRssi >= cCandRssi ? TRUE : FALSE;
#endif

	/* 1. No need check score case
	 * 1.1 Scan missing count of CurrBss is too more,
	 * but Candidate is suitable, don't replace
	 */
	if (u2CurrMiss > 2 && u2CurrMiss > u2CandMiss) {
		log_dbg(SCN, INFO, "Scan Miss count of CurrBss > 2, and Candidate <= 2\n");
		return FALSE;
	}
	/* 1.2 Scan missing count of Candidate is too more,
	 * but CurrBss is suitable, replace
	 */
	if (u2CandMiss > 2 && u2CandMiss > u2CurrMiss) {
		log_dbg(SCN, INFO, "Scan Miss count of Candidate > 2, and CurrBss <= 2\n");
		return TRUE;
	}
	/* 1.3 Hard connecting RSSI check */
	if ((prCurrBss->eBand == BAND_5G && cCurrRssi < MINIMUM_RSSI_5G) ||
#if (CFG_SUPPORT_WIFI_6G == 1)
		(prCurrBss->eBand == BAND_6G && cCurrRssi < MINIMUM_RSSI_6G) ||
#endif
		(prCurrBss->eBand == BAND_2G4 && cCurrRssi < MINIMUM_RSSI_2G4))
		return FALSE;
	else if ((prCandBss->eBand == BAND_5G && cCandRssi < MINIMUM_RSSI_5G) ||
#if (CFG_SUPPORT_WIFI_6G == 1)
		(prCandBss->eBand == BAND_6G && cCandRssi < MINIMUM_RSSI_6G) ||
#endif
		(prCandBss->eBand == BAND_2G4 && cCandRssi < MINIMUM_RSSI_2G4))
		return TRUE;

	/* 1.4 prefer to select 5G Bss if Rssi of a 5G band BSS is good */
	if (eRoamType == ROAM_TYPE_RCPI &&
		prCandBss->eBand != prCurrBss->eBand) {
		if (prCandBss->eBand >= BAND_2G4 &&
#if (CFG_SUPPORT_WIFI_6G == 1)
			prCandBss->eBand <= BAND_6G &&
#else
			prCandBss->eBand <= BAND_5G &&
#endif
			networkReplaceHandler[prCandBss->eBand].
			pfnNetworkSelection(
			prCurrBss->eBand, cCandRssi, cCurrRssi))
			return TRUE;
	}

	/* 1.5 RSSI of Current Bss is lower than Candidate, don't replace
	 * If the lower Rssi is greater than -59dbm,
	 * then no need check the difference
	 * Otherwise, if difference is greater than 10dbm, select the good RSSI
	 */
	 do {
#if CFG_SUPPORT_ROAMING
		if ((eRoamType == ROAM_TYPE_PER) &&
			cCandRssi >= RSSI_SECOND_LEVEL &&
			cCurrRssi >= RSSI_SECOND_LEVEL)
			break;
#endif

		/*customer request prCandBss and prCurrBss not same band
		*to check rssi delta
		*/
		if (prCandBss->eBand != prCurrBss->eBand &&
			((cCurrRssi - cCandRssi) <= RSSI_DIFF_BETWEEN_BAND ||
			(cCandRssi - cCurrRssi) <= RSSI_DIFF_BETWEEN_BAND)) {
			if ((prCurrBss->eBand == BAND_5G &&
				cCurrRssi >= LOW_RSSI_FOR_5G_BAND) ||
				(prCandBss->eBand == BAND_5G &&
				cCandRssi >= LOW_RSSI_FOR_5G_BAND)) {
				log_dbg(SCN, INFO,
				"curBand:%d, candBand:%d, curRssi:%d, candRssi:%d",
				prCurrBss->eBand, prCandBss->eBand,
				cCurrRssi, cCandRssi);
				break;
			}
		}

		if (cCandRssi - cCurrRssi >= RSSI_DIFF_BETWEEN_BSS)
			return FALSE;
		if (cCurrRssi - cCandRssi >= RSSI_DIFF_BETWEEN_BSS)
			return TRUE;
	} while (FALSE);
#if CFG_SUPPORT_ROAMING
	if (eRoamType == ROAM_TYPE_PER) {
		if (prCandBss->ucChannelNum == ucOpChannelNum) {
			log_dbg(SCN, INFO, "CandBss in opchnl,add CurBss Score\n");
			u2CurrScore += BSS_FULL_SCORE *
				gasMtkWeightConfig[eRoamType].ucOpchnlWeight;
		}
		if (prCurrBss->ucChannelNum == ucOpChannelNum) {
			log_dbg(SCN, INFO, "CurrBss in opchnl,add CandBss Score\n");
			u2CandScore += BSS_FULL_SCORE *
				gasMtkWeightConfig[eRoamType].ucOpchnlWeight;
		}
	}
#endif

	/* 2. Check Score */
	/* 2.1 Cases that no need to replace candidate */
#if (CFG_EXT_ROAMING == 1)
	if (u2CandScore >= u2CurrScore)
		return FALSE;
#else
	if (prCandBss->fgIsConnected) {
		if ((u2CandScore + ROAMING_NO_SWING_SCORE_STEP) >= u2CurrScore)
			return FALSE;
	} else if (prCurrBss->fgIsConnected) {
		if (u2CandScore >= (u2CurrScore + ROAMING_NO_SWING_SCORE_STEP))
			return FALSE;
	} else if (u2CandScore >= u2CurrScore)
		return FALSE;
#endif
	/* 2.2 other cases, replace candidate */
	return TRUE;
}

static u_int8_t scanSanityCheckBssDesc(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ENUM_BAND eBand, uint8_t ucChannel,
	u_int8_t fgIsFixedChannel, enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prAisBssInfo;
	struct BSS_DESC *target;
	struct APS_INFO *prApsInfo = aisGetApsInfo(prAdapter, ucBssIndex);
#if CFG_SUPPORT_MBO
	struct PARAM_BSS_DISALLOWED_LIST *disallow;
	uint32_t i = 0;

	disallow = &prAdapter->rWifiVar.rBssDisallowedList;
	for (i = 0; i < disallow->u4NumBssDisallowed; ++i) {
		uint32_t index = i * MAC_ADDR_LEN;

		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
				&disallow->aucList[index])) {
			log_dbg(SCN, WARN, MACSTR" disallowed list\n",
				MAC2STR(prBssDesc->aucBSSID));
#if (CFG_SUPPORT_CONN_LOG == 1)
			connLogDisallowedList(prAdapter,
				ucBssIndex,
				prBssDesc);
#endif
			return FALSE;
		}
	}

	if (aisQueryCusBlocklist(prAdapter, ucBssIndex, prBssDesc))
		return FALSE;

	if (prBssDesc->fgIsDisallowed) {
		log_dbg(SCN, WARN, MACSTR" disallowed\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	if (prBssDesc->prBlock && prBssDesc->prBlock->fgDisallowed &&
	    !(prBssDesc->prBlock->i4RssiThreshold > 0 &&
	      RCPI_TO_dBm(prBssDesc->ucRCPI) >
			prBssDesc->prBlock->i4RssiThreshold)) {
		log_dbg(SCN, WARN, MACSTR" disallowed delay, rssi %d(%d)\n",
			MAC2STR(prBssDesc->aucBSSID),
			RCPI_TO_dBm(prBssDesc->ucRCPI),
			prBssDesc->prBlock->i4RssiThreshold);
		return FALSE;
	}

	if (prBssDesc->prBlock && prBssDesc->prBlock->fgDisallowed) {
		log_dbg(SCN, WARN, MACSTR" disallowed delay\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}
#endif

	if (!prBssDesc->fgIsInUse) {
		log_dbg(SCN, WARN, MACSTR" is not in use\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	if (prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE) {
		log_dbg(SCN, WARN, MACSTR" is not infrastructure\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	target = aisGetTargetBssDesc(prAdapter, ucBssIndex);
	if (prBssDesc->prBlock) {
		if (prBssDesc->prBlock->fgIsInFWKBlocklist) {
			log_dbg(SCN, WARN, MACSTR" in FWK blocklist\n",
				MAC2STR(prBssDesc->aucBSSID));
#if (CFG_SUPPORT_CONN_LOG == 1)
			connLogBlockList(prAdapter,
				ucBssIndex,
				prBssDesc);
#endif
			return FALSE;
		}

		if (prBssDesc->prBlock->fgDeauthLastTime &&
		    (prApsInfo->u2EssApNum <= 1 ||
		     prBssDesc->prBlock->ucDeauthCount >= 2)) {
			log_dbg(SCN, WARN, MACSTR " is sending deauth. [%d]\n",
				MAC2STR(prBssDesc->aucBSSID),
				prBssDesc->prBlock->ucDeauthCount);
			return FALSE;
		}

		if (prBssDesc->prBlock->fgArpNoResponse &&
		    prBssDesc->prBlock->ucCount >= AIS_ARP_NO_RSP_THRESHOLD) {
			log_dbg(SCN, WARN, MACSTR " is arp no response many times.\n",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}

		if (prBssDesc->prBlock->ucCount >= 10)  {
			log_dbg(SCN, WARN,
				MACSTR
				" Skip AP that add toblocklist count >= 10\n",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}
	}

#if CFG_EXT_SCAN
	/* BTO case */
	if (prBssDesc->fgIsInBTO) {
		log_dbg(SCN, WARN, MACSTR " is in BTO.\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}
#endif

	/* roaming case */
	if (target &&
	   (prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED ||
	    aisFsmIsInProcessPostpone(prAdapter, ucBssIndex))) {
		int32_t r1, r2;

		r1 = RCPI_TO_dBm(target->ucRCPI);
		r2 = RCPI_TO_dBm(prBssDesc->ucRCPI);

		if (eRoamReason == ROAMING_REASON_BTM &&
		    r2 < prAdapter->rWifiVar.iBTMMinRssi) {
			log_dbg(SCN, INFO, MACSTR " low rssi %d for BTM\n",
				MAC2STR(prBssDesc->aucBSSID), r2);
			return FALSE;
		} else if (prBssDesc->ucRCPI < RCPI_FOR_DONT_ROAM
#if CFG_SUPPORT_NCHO
		|| (prAdapter->rNchoInfo.fgNCHOEnabled &&
		    (eRoamReason == ROAMING_REASON_POOR_RCPI ||
		     eRoamReason == ROAMING_REASON_RETRY) &&
		    r1 - r2 <= prAdapter->rNchoInfo.i4RoamDelta)
#endif
		) {
			log_dbg(SCN, INFO, MACSTR " low rssi %d (cur=%d)\n",
				MAC2STR(prBssDesc->aucBSSID), r2, r1);
			return FALSE;
		}
	}

	if (ucBssIndex != aisGetDefaultLinkBssIndex(prAdapter)) {
		struct BSS_DESC *target =
			aisGetDefaultLink(prAdapter)->prTargetBssDesc;

		if (target && prBssDesc->eBand == target->eBand) {
			log_dbg(SCN, WARN,
				MACSTR" used the same band with main\n",
				MAC2STR(prBssDesc->aucBSSID));
			if (!prAdapter->rWifiVar.fgAllowSameBandDualSta)
				return FALSE;
		}
	}

#if CFG_SUPPORT_NCHO
	if (prAdapter->rNchoInfo.fgNCHOEnabled) {
		if (!(BIT(prBssDesc->eBand) &
			prAdapter->rNchoInfo.ucRoamBand)) {
			log_dbg(SCN, WARN,
			MACSTR" band(%s) is not in NCHO roam band\n",
			MAC2STR(prBssDesc->aucBSSID),
			apucBandStr[prBssDesc->eBand]);
			return FALSE;
		}
	}
#endif

	if (!(prBssDesc->ucPhyTypeSet &
		(prAdapter->rWifiVar.ucAvailablePhyTypeSet))) {
		log_dbg(SCN, WARN,
			MACSTR" ignore unsupported ucPhyTypeSet = %x\n",
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->ucPhyTypeSet);
		return FALSE;
	}
	if (prBssDesc->fgIsUnknownBssBasicRate) {
		log_dbg(SCN, WARN, MACSTR" unknown bss basic rate\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}
	if (fgIsFixedChannel &&	(eBand != prBssDesc->eBand || ucChannel !=
		prBssDesc->ucChannelNum)) {
		log_dbg(SCN, WARN,
			MACSTR" fix channel required band %d, channel %d\n",
			MAC2STR(prBssDesc->aucBSSID), eBand, ucChannel);
		return FALSE;
	}

#if CFG_SUPPORT_WIFI_SYSDVT
	if (!IS_SKIP_CH_CHECK(prAdapter))
#endif
	if (!rlmDomainIsLegalChannel(prAdapter, prBssDesc->eBand,
		prBssDesc->ucChannelNum)) {
		log_dbg(SCN, WARN, MACSTR" band %d channel %d is not legal\n",
			MAC2STR(prBssDesc->aucBSSID), prBssDesc->eBand,
			prBssDesc->ucChannelNum);
		return FALSE;
	}
	if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), prBssDesc->rUpdateTime,
		SEC_TO_SYSTIME(wlanWfdEnabled(prAdapter) ?
		SCN_BSS_DESC_STALE_SEC_WFD : SCN_BSS_DESC_STALE_SEC))) {
		log_dbg(SCN, WARN, MACSTR " description is too old.\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	if (!rsnPerformPolicySelection(prAdapter, prBssDesc,
		ucBssIndex)) {
		log_dbg(SCN, WARN, MACSTR " rsn policy select fail.\n",
			MAC2STR(prBssDesc->aucBSSID));
#if (CFG_SUPPORT_CONN_LOG == 1)
		connLogRsnMismatch(prAdapter,
			ucBssIndex,
			prBssDesc);
#endif
		return FALSE;
	}
	if (aisGetAisSpecBssInfo(prAdapter,
		ucBssIndex)->fgCounterMeasure) {
		log_dbg(SCN, WARN, MACSTR " Skip in counter measure period.\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

#if CFG_SUPPORT_ROAMING
#if CFG_SUPPORT_802_11K
	if (eRoamReason == ROAMING_REASON_BTM) {
		struct BSS_TRANSITION_MGT_PARAM *prBtmParam;
		uint8_t ucRequestMode = 0;

		prBtmParam = aisGetBTMParam(prAdapter, ucBssIndex);
		ucRequestMode = prBtmParam->ucRequestMode;
		if (aisCheckNeighborApValidity(prAdapter, ucBssIndex)) {
			if (prBssDesc->prNeighbor &&
			    prBssDesc->prNeighbor->fgPrefPresence &&
			    !prBssDesc->prNeighbor->ucPreference) {
				log_dbg(SCN, INFO,
				     MACSTR " preference is 0, skip it\n",
				     MAC2STR(prBssDesc->aucBSSID));
				return FALSE;
			}

			if ((ucRequestMode & WNM_BSS_TM_REQ_ABRIDGED) &&
			    !prBssDesc->prNeighbor &&
			    prBtmParam->ucDisImmiState !=
				    AIS_BTM_DIS_IMMI_STATE_3) {
				log_dbg(SCN, INFO,
				     MACSTR " not in candidate list, skip it\n",
				     MAC2STR(prBssDesc->aucBSSID));
				return FALSE;
			}

#if CFG_EXT_ROAMING_WTC
		return scanWtcCheckBssDesc(
			prAdapter,
			ucBssIndex,
			prBssDesc);
#endif
		}
	}
#endif
#endif
	return TRUE;
}

#if (CFG_TC10_FEATURE == 1)
static int32_t scanCalculateScoreByCu(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex)
{
	struct SCAN_INFO *info;
	struct SCAN_PARAM *param;
	struct BSS_INFO *bss;
	int32_t score, rssi, cu = 0, cuRatio, dwell;
	uint32_t rssiFactor, cuFactor, rssiWeight, cuWeight;
	uint32_t slot = 0, idle;
	uint8_t i;

	if (eRoamReason == ROAMING_REASON_BEACON_TIMEOUT || !prBssDesc ||
	    (prBssDesc->prBlock && prBssDesc->prBlock->fgDeauthLastTime))
		return -1;

#if CFG_SUPPORT_NCHO
	if (prAdapter->rNchoInfo.fgNCHOEnabled)
		return -1;
#endif
	rssi = RCPI_TO_dBm(prBssDesc->ucRCPI);
	rssiWeight = 65;
	cuWeight = 35;
	if (rssi >= -55)
		rssiFactor = 100;
	else if (rssi < -55 && rssi >= -60)
		rssiFactor = 90 + 2 * (60 + rssi);
	else if (rssi < -60 && rssi >= -70)
		rssiFactor = 60 + 3 * (70 + rssi);
	else if (rssi < -70 && rssi >= -80)
		rssiFactor = 20 + 4 * (80 + rssi);
	else if (rssi < -80 && rssi >= -90)
		rssiFactor = 2 * (90 + rssi);
	else
		rssiFactor = 0;
	if (prBssDesc->fgExistBssLoadIE) {
		cu = prBssDesc->ucChnlUtilization;
	} else {
		bss = aisGetAisBssInfo(prAdapter, ucBssIndex);
		info = &(prAdapter->rWifiVar.rScanInfo);
		param = &(info->rScanParam);
		if (param->u2ChannelDwellTime > 0)
			dwell = param->u2ChannelDwellTime;
		else if (bss->eConnectionState == MEDIA_STATE_CONNECTED)
			dwell = CHNL_DWELL_TIME_ONLINE;
		else
			dwell = CHNL_DWELL_TIME_DEFAULT;
		for (i = 0; i < info->ucSparseChannelArrayValidNum; i++) {
			if (prBssDesc->ucChannelNum == info->aucChannelNum[i]) {
				slot = info->au2ChannelIdleTime[i];
				idle = (slot * 9 * 100) / (dwell * 1000);
				cu = 255 - idle * 255 / 100;
				break;
			}
		}
	}
	cuRatio = cu * 100 / 255;
	if (prBssDesc->eBand == BAND_2G4) {
		if (cuRatio < 10)
			cuFactor = 100;
		else if (cuRatio < 70 && cuRatio >= 10)
			cuFactor = 111 - (13 * cuRatio / 10);
		else
			cuFactor = 20;
	} else {
		if (cuRatio < 30)
			cuFactor = 100;
		else if (cuRatio < 80 && cuRatio >= 30)
			cuFactor = 148 - (16 * cuRatio / 10);
		else
			cuFactor = 20;
	}
	score = rssiFactor * rssiWeight + cuFactor * cuWeight;
	log_dbg(SCN, INFO,
		MACSTR
		" 5G[%d],chl[%d],slt[%d],ld[%d] Basic Score %d,rssi[%d],cu[%d],cuR[%d],rf[%d],rw[%d],cf[%d],cw[%d]\n",
		MAC2STR(prBssDesc->aucBSSID),
		(prBssDesc->eBand == BAND_5G ? 1 : 0),
		prBssDesc->ucChannelNum, slot,
		prBssDesc->fgExistBssLoadIE, score, rssi, cu, cuRatio,
		rssiFactor, rssiWeight,	cuFactor, cuWeight);
	return score;
}
#endif

static uint16_t scanCalculateScoreByRssi(struct BSS_DESC *prBssDesc,
	enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;
	int8_t cRssi = RCPI_TO_dBm(prBssDesc->ucRCPI);

	if (eRoamType < 0 || eRoamType >= ROAM_TYPE_NUM) {
		log_dbg(SCN, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}
	if (cRssi >= BEST_RSSI)
		u2Score = 100;
	else if (cRssi <= -98)
		u2Score = 0;
	else
		u2Score = (cRssi + 98) * 2;
	u2Score *= gasMtkWeightConfig[eRoamType].ucRssiWeight;

	return u2Score;
}

static uint16_t scanCalculateScoreBySaa(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;

	if (eRoamType < 0 || eRoamType >= ROAM_TYPE_NUM) {
		log_dbg(SCN, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

	prStaRec = cnmGetStaRecByAddress(prAdapter, NETWORK_TYPE_AIS,
		prBssDesc->aucSrcAddr);
	if (prStaRec)
		u2Score = gasMtkWeightConfig[eRoamType].ucSaaWeight *
		(prStaRec->ucTxAuthAssocRetryCount ? 0 : BSS_FULL_SCORE);
	else
		u2Score = gasMtkWeightConfig[eRoamType].ucSaaWeight *
		BSS_FULL_SCORE;

	return u2Score;
}

static uint16_t scanCalculateScoreByIdleTime(struct ADAPTER *prAdapter,
	uint8_t ucChannel, enum ROAM_TYPE eRoamType,
	struct BSS_DESC *prBssDesc, uint8_t ucBssIndex,
	enum ENUM_BAND eBand)
{
	struct SCAN_INFO *info;
	struct SCAN_PARAM *param;
	struct BSS_INFO *bss;
	int32_t score, rssi, cu = 0, cuRatio, dwell;
	uint32_t rssiFactor, cuFactor, rssiWeight, cuWeight;
	uint32_t slot = 0, idle;
	uint8_t i;

	rssi = RCPI_TO_dBm(prBssDesc->ucRCPI);
	rssiWeight = 65;
	cuWeight = 35;
	if (rssi >= -55)
		rssiFactor = 100;
	else if (rssi < -55 && rssi >= -60)
		rssiFactor = 90 + 2 * (60 + rssi);
	else if (rssi < -60 && rssi >= -70)
		rssiFactor = 60 + 3 * (70 + rssi);
	else if (rssi < -70 && rssi >= -80)
		rssiFactor = 20 + 4 * (80 + rssi);
	else if (rssi < -80 && rssi >= -90)
		rssiFactor = 2 * (90 + rssi);
	else
		rssiFactor = 0;
	if (eRoamType < 0 || eRoamType >= ROAM_TYPE_NUM) {
		log_dbg(SCN, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}
	if (prBssDesc->fgExistBssLoadIE) {
		cu = prBssDesc->ucChnlUtilization;
	} else {
		bss = aisGetAisBssInfo(prAdapter, ucBssIndex);
		info = &(prAdapter->rWifiVar.rScanInfo);
		param = &(info->rScanParam);

		if (param->u2ChannelDwellTime > 0)
			dwell = param->u2ChannelDwellTime;
		else if (bss->eConnectionState == MEDIA_STATE_CONNECTED)
			dwell = CHNL_DWELL_TIME_ONLINE;
		else
			dwell = CHNL_DWELL_TIME_DEFAULT;

		for (i = 0; i < info->ucSparseChannelArrayValidNum; i++) {
			if (prBssDesc->ucChannelNum == info->aucChannelNum[i] &&
					eBand == info->aeChannelBand[i]) {
				slot = info->au2ChannelIdleTime[i];
				idle = (slot * 9 * 100) / (dwell * 1000);
#if CFG_SUPPORT_ROAMING
				if (eRoamType == ROAM_TYPE_PER) {
					score = idle > BSS_FULL_SCORE ?
						BSS_FULL_SCORE : idle;
					goto done;
				}
#endif
				cu = 255 - idle * 255 / 100;
				break;
			}
		}
	}

	cuRatio = cu * 100 / 255;
	if (prBssDesc->eBand == BAND_2G4) {
		if (cuRatio < 10)
			cuFactor = 100;
		else if (cuRatio < 70 && cuRatio >= 10)
			cuFactor = 111 - (13 * cuRatio / 10);
		else
			cuFactor = 20;
	} else {
		if (cuRatio < 30)
			cuFactor = 100;
		else if (cuRatio < 80 && cuRatio >= 30)
			cuFactor = 148 - (16 * cuRatio / 10);
		else
			cuFactor = 20;
	}

	score = (rssiFactor * rssiWeight + cuFactor * cuWeight) >> 6;

	log_dbg(SCN, TRACE,
		MACSTR
		" Band[%s],chl[%d],slt[%d],ld[%d] idle Score %d,rssi[%d],cu[%d],cuR[%d],rf[%d],rw[%d],cf[%d],cw[%d]\n",
		MAC2STR(prBssDesc->aucBSSID),
		apucBandStr[prBssDesc->eBand],
		prBssDesc->ucChannelNum, slot,
		prBssDesc->fgExistBssLoadIE, score, rssi, cu, cuRatio,
		rssiFactor, rssiWeight, cuFactor, cuWeight);
#if CFG_SUPPORT_ROAMING
done:
#endif
	return score * gasMtkWeightConfig[eRoamType].ucChnlIdleWeight;
}

uint16_t scanCalculateScoreByBlockList(struct ADAPTER *prAdapter,
	    struct BSS_DESC *prBssDesc, enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;
	uint8_t ucRoamType = (uint8_t) eRoamType;

	if (ucRoamType >= ROAM_TYPE_NUM) {
		log_dbg(SCN, WARN, "Invalid roam type %d!\n", ucRoamType);
		return 0;
	}
	if (!prBssDesc->prBlock)
		u2Score = 100;
	else if (rsnApOverload(prBssDesc->prBlock->u2AuthStatus,
		prBssDesc->prBlock->u2DeauthReason) ||
		 prBssDesc->prBlock->ucCount >= 10)
		u2Score = 0;
	else
		u2Score = 100 - prBssDesc->prBlock->ucCount * 10;

	return u2Score * gasMtkWeightConfig[ucRoamType].ucBlockListWeight;
}

uint16_t scanCalculateScoreByTput(struct ADAPTER *prAdapter,
	    struct BSS_DESC *prBssDesc, enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;

#if CFG_SUPPORT_MBO
	if (prBssDesc->fgExistEspIE)
		u2Score = (prBssDesc->u4EspInfo[ESP_AC_BE] >> 8) & 0xff;
#endif
	return u2Score * gasMtkWeightConfig[eRoamType].ucTputWeight;
}

uint16_t scanCalculateScoreByPreference(struct ADAPTER *prAdapter,
	    struct BSS_DESC *prBssDesc, enum ENUM_ROAMING_REASON eRoamReason)
{
#if CFG_SUPPORT_ROAMING
#if CFG_SUPPORT_802_11K
	if (eRoamReason == ROAMING_REASON_BTM) {
		if (prBssDesc->prNeighbor) {
			enum ROAM_TYPE eRoamType =
				roamReasonToType(eRoamReason);

			return prBssDesc->prNeighbor->ucPreference *
			       gasMtkWeightConfig[eRoamType].ucPreferenceWeight;
		}
	}
#endif
#endif
	return 0;
}

uint16_t scanCalculateTotalScore(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex)
{
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo = NULL;
	uint16_t u2ScoreStaCnt = 0;
	uint16_t u2ScoreBandwidth = 0;
	uint16_t u2ScoreSTBC = 0;
	uint16_t u2ScoreChnlInfo = 0;
	uint16_t u2ScoreSnrRssi = 0;
	uint16_t u2ScoreDeauth = 0;
	uint16_t u2ScoreProbeRsp = 0;
	uint16_t u2ScoreScanMiss = 0;
	uint16_t u2ScoreBand = 0;
	uint16_t u2ScoreSaa = 0;
	uint16_t u2ScoreIdleTime = 0;
	uint16_t u2ScoreTotal = 0;
	uint16_t u2BlockListScore = 0;
	uint16_t u2PreferenceScore = 0;
	uint16_t u2TputScore = 0;
#if (CFG_SUPPORT_AVOID_DESENSE == 1)
	uint8_t fgBssInDenseRange =
		IS_CHANNEL_IN_DESENSE_RANGE(prAdapter,
		prBssDesc->ucChannelNum,
		prBssDesc->eBand);
	char extra[16] = {0};
#else
	char *extra = "";
#endif
	int8_t cRssi = -128;
	enum ROAM_TYPE eRoamType = roamReasonToType(eRoamReason);

	prAisSpecificBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	cRssi = RCPI_TO_dBm(prBssDesc->ucRCPI);

	if (eRoamType < 0 || eRoamType >= ROAM_TYPE_NUM) {
		log_dbg(SCN, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}
	u2ScoreBandwidth =
		scanCalculateScoreByBandwidth(prAdapter, prBssDesc, eRoamType);
	u2ScoreStaCnt = scanCalculateScoreByClientCnt(prBssDesc, eRoamType);
	u2ScoreSTBC = CALCULATE_SCORE_BY_STBC(prAdapter, prBssDesc, eRoamType);
	u2ScoreChnlInfo = scanCalculateScoreByChnlInfo(prAisSpecificBssInfo,
				prBssDesc->ucChannelNum, eRoamType);
	u2ScoreSnrRssi = scanCalculateScoreByRssi(prBssDesc, eRoamType);
	u2ScoreDeauth = CALCULATE_SCORE_BY_DEAUTH(prBssDesc, eRoamType);
	u2ScoreProbeRsp = CALCULATE_SCORE_BY_PROBE_RSP(prBssDesc, eRoamType);
	u2ScoreScanMiss = CALCULATE_SCORE_BY_MISS_CNT(prAdapter,
		prBssDesc, eRoamType);
	u2ScoreBand = CALCULATE_SCORE_BY_BAND(prAdapter, prBssDesc,
		cRssi, eRoamType);
	u2ScoreSaa = scanCalculateScoreBySaa(prAdapter, prBssDesc, eRoamType);
	u2ScoreIdleTime = scanCalculateScoreByIdleTime(prAdapter,
		prBssDesc->ucChannelNum, eRoamType, prBssDesc, ucBssIndex,
		prBssDesc->eBand);
	u2BlockListScore =
	       scanCalculateScoreByBlockList(prAdapter, prBssDesc, eRoamType);
	u2PreferenceScore =
	      scanCalculateScoreByPreference(prAdapter, prBssDesc, eRoamReason);

	u2TputScore = scanCalculateScoreByTput(prAdapter, prBssDesc, eRoamType);

	u2ScoreTotal = u2ScoreBandwidth + u2ScoreChnlInfo +
		u2ScoreDeauth + u2ScoreProbeRsp + u2ScoreScanMiss +
		u2ScoreSnrRssi + u2ScoreStaCnt + u2ScoreSTBC +
		u2ScoreBand + u2BlockListScore + u2ScoreSaa +
		u2ScoreIdleTime + u2TputScore;

#if (CFG_SUPPORT_AVOID_DESENSE == 1)
	if (fgBssInDenseRange)
		u2ScoreTotal /= 4;
	kalSnprintf(extra, sizeof(extra), ", DESENSE[%d]", fgBssInDenseRange);
#endif

	if (prBssDesc->fgIsMCC)
		u2ScoreTotal = (u2ScoreTotal * WEIGHT_MCC_DOWNGRADE / 100);

#define TEMP_LOG_TEMPLATE\
		MACSTR" cRSSI[%d] Band[%s] Score,Total %d,DE[%d]"\
		", PR[%d], SM[%d], RSSI[%d],BD[%d],BL[%d],SAA[%d]"\
		", BW[%d], SC[%d],ST[%d],CI[%d],IT[%d],CU[%d,%d],PF[%d]"\
		", MCC[%d], TPUT[%d]%s\n"

	log_dbg(SCN, INFO,
		TEMP_LOG_TEMPLATE,
		MAC2STR(prBssDesc->aucBSSID), cRssi,
		apucBandStr[prBssDesc->eBand], u2ScoreTotal,
		u2ScoreDeauth, u2ScoreProbeRsp, u2ScoreScanMiss,
		u2ScoreSnrRssi, u2ScoreBand, u2BlockListScore,
		u2ScoreSaa, u2ScoreBandwidth, u2ScoreStaCnt,
		u2ScoreSTBC, u2ScoreChnlInfo, u2ScoreIdleTime,
		prBssDesc->fgExistBssLoadIE,
		prBssDesc->ucChnlUtilization,
		u2PreferenceScore, prBssDesc->fgIsMCC,
		u2TputScore, extra);

#undef TEMP_LOG_TEMPLATE

	return u2ScoreTotal;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint8_t scanSanityCheckSecondary(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prBssDescSet, struct BSS_DESC *prBssDesc,
	enum ENUM_ROAMING_REASON eRoamReason, uint8_t ucBssIndex)
{
	uint8_t i;

	if (!scanSanityCheckBssDesc(prAdapter, prBssDesc, 0, 0, FALSE,
					eRoamReason, ucBssIndex))
		return FALSE;

	for (i = 0; i < prBssDescSet->ucLinkNum; i++) {
		if (prBssDesc->eBand == prBssDescSet->aprBssDesc[i]->eBand)
			return FALSE;
	}

	return TRUE;
}

void scanFillSecondaryLink(struct ADAPTER *prAdapter,
	struct BSS_DESC_SET *prBssDescSet,
	enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex)
{
	struct CONNECTION_SETTINGS *prConnSettings = NULL;
	enum ENUM_PARAM_CONNECTION_POLICY policy;
	struct BSS_DESC *prBssDesc = NULL;
	struct BSS_DESC *prMainBssDesc = prBssDescSet->prMainBssDesc;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo = NULL;
	struct LINK *prEssLink = NULL;
	uint8_t ucAisIdx = AIS_INDEX(prAdapter, ucBssIndex);
	uint8_t i;

	if (!prMainBssDesc || !prMainBssDesc->rMlInfo.fgValid)
		return;

	if (!mldIsMultiLinkEnabled(prAdapter, NETWORK_TYPE_AIS, ucBssIndex))
		return;

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	policy = prConnSettings->eConnectionPolicy;
	prAisSpecificBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	prEssLink = &prAisSpecificBssInfo->rCurEssLink;

	for (i = 1; i < prAdapter->rWifiVar.ucStaMldLinkMax; ++i) {
		uint16_t u2ScoreTotal = 0;
		uint16_t u2CandBssScore = 0;
		struct BSS_DESC *prCandBssDesc = NULL;

		/* setup secondary link */
		LINK_FOR_EACH_ENTRY(prBssDesc, prEssLink,
				rLinkEntryEss[ucAisIdx], struct BSS_DESC) {
			if (!prBssDesc->rMlInfo.fgValid ||
			    EQUAL_MAC_ADDR(prMainBssDesc->aucBSSID,
					prBssDesc->aucBSSID) ||
			    !EQUAL_MAC_ADDR(prMainBssDesc->rMlInfo.aucMldAddr,
					prBssDesc->rMlInfo.aucMldAddr) ||
			    !scanSanityCheckSecondary(prAdapter, prBssDescSet,
					prBssDesc, eRoamReason, ucBssIndex)) {
				log_dbg(SCN, INFO,
					MACSTR " valid=%d mld_addr="MACSTR"\n",
					MAC2STR(prBssDesc->aucBSSID),
					prBssDesc->rMlInfo.fgValid,
					MAC2STR(prBssDesc->rMlInfo.aucMldAddr));
				continue;
			}

			u2ScoreTotal = scanCalculateTotalScore(prAdapter,
				prBssDesc, eRoamReason, ucBssIndex);
			if (u2ScoreTotal > u2CandBssScore) {
				u2CandBssScore = u2ScoreTotal;
				prCandBssDesc = prBssDesc;
			}
		}

		if (prCandBssDesc) {
			prBssDescSet->aprBssDesc[i] = prCandBssDesc;
			prBssDescSet->ucLinkNum++;
		}
	}

	/* prefer bssid mld addr */
	if (policy != CONNECT_BY_BSSID) {
		for (i = 0; i < prBssDescSet->ucLinkNum; i++) {
			prBssDesc = prBssDescSet->aprBssDesc[i];
			if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
				prMainBssDesc->rMlInfo.aucMldAddr)) {
				prBssDescSet->aprBssDesc[i] =
					prBssDescSet->aprBssDesc[0];
				prBssDescSet->aprBssDesc[0] = prBssDesc;
				break;
			}
		}
	}

	/* first bss desc is main bss */
	prBssDescSet->prMainBssDesc = prBssDescSet->aprBssDesc[0];
	log_dbg(SCN, INFO, " Total %d link(s)\n", prBssDescSet->ucLinkNum);
}
#endif

void apsResetEssApList(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct LINK *prCurEssLink;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo;

	prAisSpecBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	if (!prAisSpecBssInfo) {
		log_dbg(SCN, INFO, "No prAisSpecBssInfo\n");
		return;
	}

	prCurEssLink = &prAisSpecBssInfo->rCurEssLink;
	if (!prCurEssLink) {
		log_dbg(SCN, INFO, "No prCurEssLink\n");
		return;
	}

	LINK_INITIALIZE(prCurEssLink);
	log_dbg(SCN, INFO, "BssIndex:%d reset prCurEssLin done\n", ucBssIndex);
}

void apsUpdateEssApList(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct BSS_DESC *prBssDesc = NULL;
	struct LINK *prBSSDescList =
		&prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	struct CONNECTION_SETTINGS *prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);
	struct LINK *prCurEssLink;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo;
	uint8_t ucAisIdx = AIS_INDEX(prAdapter, ucBssIndex);

	prAisSpecBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	if (!prAisSpecBssInfo) {
		log_dbg(SCN, INFO, "No prAisSpecBssInfo\n");
		return;
	}

	prCurEssLink = &prAisSpecBssInfo->rCurEssLink;
	if (!prCurEssLink) {
		log_dbg(SCN, INFO, "No prCurEssLink\n");
		return;
	}

	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry,
		struct BSS_DESC) {
		if (prBssDesc->ucChannelNum > 233)
			continue;
		if (!EQUAL_SSID(prConnSettings->aucSSID,
			prConnSettings->ucSSIDLen,
			prBssDesc->aucSSID, prBssDesc->ucSSIDLen) ||
			prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE)
			continue;
		/* Record same BSS list */
		LINK_ENTRY_INITIALIZE(&prBssDesc->rLinkEntryEss[ucAisIdx]);
		LINK_INSERT_HEAD(prCurEssLink,
			&prBssDesc->rLinkEntryEss[ucAisIdx]);

		scanCheckConcurrent(prAdapter, prBssDesc, ucBssIndex);
	}

	log_dbg(SCN, INFO, "Find %s in %d BSSes, result %d\n",
		prConnSettings->aucSSID, prBSSDescList->u4NumElem,
		prCurEssLink->u4NumElem);
}

/*
 * Bss Characteristics to be taken into account when calculate Score:
 * Channel Loading Group:
 * 1. Client Count (in BSS Load IE).
 * 2. AP number on the Channel.
 *
 * RF Group:
 * 1. Channel utilization.
 * 2. SNR.
 * 3. RSSI.
 *
 * Misc Group:
 * 1. Deauth Last time.
 * 2. Scan Missing Count.
 * 3. Has probe response in scan result.
 *
 * Capability Group:
 * 1. Prefer 5G band.
 * 2. Bandwidth.
 * 3. STBC and Multi Anttena.
 */
struct BSS_DESC *apsSearchBssDescByScore(struct ADAPTER *prAdapter,
	enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex, struct BSS_DESC_SET *prBssDescSet)
{
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo = NULL;
	struct BSS_INFO *prAisBssInfo = NULL;
	struct LINK *prEssLink = NULL;
	struct CONNECTION_SETTINGS *prConnSettings = NULL;
	struct BSS_DESC *prBssDesc = NULL;
	struct BSS_DESC *prCandBssDesc = NULL;
	struct BSS_DESC *prLastDeauthBssDesc = NULL;
	uint16_t u2ScoreTotal = 0;
	uint16_t u2CandBssScore = 0;
	u_int8_t fgSearchBlockList = FALSE;
	u_int8_t fgIsFixedChnl = FALSE;
	enum ENUM_BAND eBand = BAND_2G4;
	uint8_t ucChannel = 0;
	enum ENUM_PARAM_CONNECTION_POLICY policy;
	enum ROAM_TYPE eRoamType;
#if (CFG_TC10_FEATURE == 1)
	int32_t base, goal;
#endif
	uint8_t ucAisIdx;

	if (!prAdapter || eRoamReason >= ROAMING_REASON_NUM) {
		log_dbg(SCN, ERROR,
			"prAdapter %p, reason %d!\n", prAdapter, eRoamReason);
		return NULL;
	}

	prAisSpecificBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prEssLink = &prAisSpecificBssInfo->rCurEssLink;
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	eRoamType = roamReasonToType(eRoamReason);
#if CFG_SUPPORT_CHNL_CONFLICT_REVISE
	fgIsFixedChnl =	cnmAisDetectP2PChannel(prAdapter, &eBand, &ucChannel);
#else
	fgIsFixedChnl =	cnmAisInfraChannelFixed(prAdapter, &eBand, &ucChannel);
#endif
	ucAisIdx = AIS_INDEX(prAdapter, ucBssIndex);

	aisRemoveArpNRBlocklist(prAdapter, AIS_BLOCKLIST_TIMEOUT_ARP_NO_RSP);
	aisRemoveTimeoutBlocklist(prAdapter, AIS_BLOCKLIST_TIMEOUT);
	aisClearCusBlocklist(prAdapter, ucBssIndex, FALSE);
	apsUpdateEssApList(prAdapter, ucBssIndex);

#if CFG_SUPPORT_802_11K
	/* check before using neighbor report */
	aisCheckNeighborApValidity(prAdapter, prAisBssInfo->ucBssIndex);
#endif
	log_dbg(SCN, INFO, "ConnectionPolicy = %d, reason = %d\n",
		prConnSettings->eConnectionPolicy, eRoamReason);
	policy = prConnSettings->eConnectionPolicy;
#if (CFG_TC10_FEATURE == 1)
	goal = base = scanCalculateScoreByCu(prAdapter,
		aisGetTargetBssDesc(prAdapter, ucBssIndex),
		eRoamReason, ucBssIndex);
	switch (eRoamReason) {
	case ROAMING_REASON_POOR_RCPI:
	case ROAMING_REASON_RETRY:
		goal += base * 20 / 100;
		break;
	case ROAMING_REASON_IDLE:
		goal += base * 1 / 100;
		break;
	case ROAMING_REASON_BTM:
	{
		struct BSS_TRANSITION_MGT_PARAM *prBtmParam;
		uint8_t ucRequestMode = 0;

		goal += base * prAdapter->rWifiVar.u4BtmDelta / 100;
		prBtmParam = aisGetBTMParam(prAdapter, ucBssIndex);
		ucRequestMode = prBtmParam->ucRequestMode;
		if (ucRequestMode &
			WNM_BSS_TM_REQ_DISASSOC_IMMINENT) {
			if (prBtmParam->ucDisImmiState ==
					AIS_BTM_DIS_IMMI_STATE_2)
				goal = 6000;
			else if (prBtmParam->ucDisImmiState ==
					AIS_BTM_DIS_IMMI_STATE_3)
				goal = 0;
		}
		break;
	}
	default:
		break;
	}
#endif

try_again:
	LINK_FOR_EACH_ENTRY(prBssDesc, prEssLink, rLinkEntryEss[ucAisIdx],
		struct BSS_DESC) {
		if (!fgSearchBlockList) {
			/* update blocklist info */
			prBssDesc->prBlock =
				aisQueryBlockList(prAdapter, prBssDesc);
#if CFG_SUPPORT_802_11K
			/* update neighbor report entry */
			prBssDesc->prNeighbor = scanGetNeighborAPEntry(
				prAdapter, prBssDesc->aucBSSID, ucBssIndex);
#endif
		}
		/*
		 * Skip if
		 * 1. sanity check fail or
		 * 2. bssid is in driver's blocklist in the first round
		 */
		if (!scanSanityCheckBssDesc(prAdapter, prBssDesc, eBand,
			ucChannel, fgIsFixedChnl, eRoamReason, ucBssIndex) ||
		    (!fgSearchBlockList && prBssDesc->prBlock))
			continue;

		/* pick by bssid first */
		if (policy == CONNECT_BY_BSSID) {
			if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
					prConnSettings->aucBSSID)) {
				log_dbg(SCN, INFO, MACSTR" match bssid\n",
					MAC2STR(prBssDesc->aucBSSID));
				prCandBssDesc = prBssDesc;
				break;
			}
			/* skip when bssid unmatched if conn by bssid */
			log_dbg(SCN, INFO, MACSTR" unmatch bssid\n",
				MAC2STR(prBssDesc->aucBSSID));
			continue;
		} else if (policy == CONNECT_BY_BSSID_HINT) {
			uint8_t oce = FALSE;
#if (CFG_EXT_ROAMING == 1)
			uint8_t chnl = nicFreq2ChannelNum(
					prConnSettings->u4FreqInKHz * 1000);
#endif

#if CFG_SUPPORT_MBO
			oce = prAdapter->rWifiVar.u4SwTestMode ==
				ENUM_SW_TEST_MODE_SIGMA_OCE;
#endif
			if (!oce && EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
					prConnSettings->aucBSSIDHint)
#if (CFG_EXT_ROAMING == 1)
					&&
					(chnl == 0 ||
					 chnl == prBssDesc->ucChannelNum)
#endif
					) {
#if (CFG_SUPPORT_AVOID_DESENSE == 1)
				if (IS_CHANNEL_IN_DESENSE_RANGE(prAdapter,
					prBssDesc->ucChannelNum,
					prBssDesc->eBand)) {
					log_dbg(SCN, INFO,
						"Do network selection even match bssid_hint\n");
				} else
#endif
				{
					log_dbg(SCN, INFO,
						MACSTR" match bssid_hint\n",
						MAC2STR(prBssDesc->aucBSSID));
					prCandBssDesc = prBssDesc;
					break;
				}
			}
		}

#if (CFG_TC10_FEATURE == 1)
		if (base > 0 && UNEQUAL_MAC_ADDR(prBssDesc->aucBSSID,
				prAisBssInfo->aucBSSID)) {
			u2ScoreTotal = scanCalculateScoreByCu(
				prAdapter, prBssDesc, eRoamReason, ucBssIndex);
			if (u2ScoreTotal < goal) {
				log_dbg(SCN, WARN,
					MACSTR " reason %d, score %d < %d\n",
					MAC2STR(prBssDesc->aucBSSID),
					eRoamReason, u2ScoreTotal, goal);
				continue;
			}
		}
#endif

		if (prBssDesc->prBlock &&
		    prBssDesc->prBlock->fgDeauthLastTime) {
			prLastDeauthBssDesc = prBssDesc;
			continue;
		}

		u2ScoreTotal = scanCalculateTotalScore(prAdapter, prBssDesc,
			eRoamReason, ucBssIndex);
		if (!prCandBssDesc ||
			scanNeedReplaceCandidate(prAdapter, prCandBssDesc,
			prBssDesc, u2CandBssScore, u2ScoreTotal,
			eRoamReason, ucBssIndex)) {
			prCandBssDesc = prBssDesc;
			u2CandBssScore = u2ScoreTotal;
		}
	}

	if (!prCandBssDesc &&
	    prAdapter->rWifiVar.u4SwTestMode == ENUM_SW_TEST_MODE_NONE)
		prCandBssDesc = prLastDeauthBssDesc;

	if (prCandBssDesc) {
		if ((prCandBssDesc->fgIsConnected & BIT(ucBssIndex)) &&
		    !fgSearchBlockList && prEssLink->u4NumElem > 0) {
			fgSearchBlockList = TRUE;
			log_dbg(SCN, INFO, "Can't roam out, try blocklist\n");
			goto try_again;
		}

		if (prConnSettings->eConnectionPolicy == CONNECT_BY_BSSID)
			log_dbg(SCN, INFO, "Selected "
				MACSTR
				" %d base on ssid,when find %s, "
				MACSTR
				" in %d bssid,fix channel %d.\n",
				MAC2STR(prCandBssDesc->aucBSSID),
				RCPI_TO_dBm(prCandBssDesc->ucRCPI),
				HIDE(prConnSettings->aucSSID),
				MAC2STR(prConnSettings->aucBSSID),
				prEssLink->u4NumElem, ucChannel);
		else
			log_dbg(SCN, INFO,
				"Selected "
				MACSTR
				", cRSSI[%d] Band[%s] Score %d when find %s, "
				MACSTR
				" in %d BSSes, fix channel %d.\n",
				MAC2STR(prCandBssDesc->aucBSSID),
				RCPI_TO_dBm(prCandBssDesc->ucRCPI),
				apucBandStr[prCandBssDesc->eBand],
				u2CandBssScore, prConnSettings->aucSSID,
				MAC2STR(prConnSettings->aucBSSID),
				prEssLink->u4NumElem, ucChannel);

		goto done;
	}

	/* if No Candidate BSS is found, try BSSes which are in blocklist */
	if (!fgSearchBlockList && prEssLink->u4NumElem > 0) {
		fgSearchBlockList = TRUE;
		log_dbg(SCN, INFO, "No Bss is found, Try blocklist\n");
		goto try_again;
	}
	log_dbg(SCN, INFO, "Selected None when find %s, " MACSTR
		" in %d BSSes, fix channel %d.\n",
		prConnSettings->aucSSID, MAC2STR(prConnSettings->aucBSSID),
		prEssLink->u4NumElem, ucChannel);

done:
	if (prBssDescSet) {
		if (prCandBssDesc) {
			/* setup primary link */
			prBssDescSet->ucLinkNum = 1;
			prBssDescSet->aprBssDesc[0] = prCandBssDesc;
			prBssDescSet->prMainBssDesc = prCandBssDesc;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			scanFillSecondaryLink(prAdapter,
				prBssDescSet, eRoamReason, ucBssIndex);
#endif
		} else {
			prBssDescSet->ucLinkNum = 0;
			prBssDescSet->prMainBssDesc = NULL;
		}
	}

	apsResetEssApList(prAdapter, ucBssIndex);

	return prCandBssDesc;
}

