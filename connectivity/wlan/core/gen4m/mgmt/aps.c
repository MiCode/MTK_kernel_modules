/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

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

#if (CFG_TC10_FEATURE == 1)
#define RCPI_FOR_DONT_ROAM                      80 /*-70dbm*/
#else
#define RCPI_FOR_DONT_ROAM                      60 /*-80dbm*/
#endif

/* Real Rssi of a Bss may range in current_rssi - 5 dbm
 *to current_rssi + 5 dbm
 */
#define RSSI_DIFF_BIG_STEP			15 /* dbm */
#define RSSI_DIFF_MED_STEP			10 /* dbm */
#define RSSI_DIFF_SML_STEP			5 /* dbm */
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
#define WEIGHT_IDX_BLACK_LIST                   2
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
#define WEIGHT_IDX_BLACK_LIST_PER               4
#define WEIGHT_IDX_SAA_PER                      1
#define WEIGHT_IDX_CHNL_IDLE_PER                6
#define WEIGHT_IDX_OPCHNL_PER                   6
#define WEIGHT_IDX_TPUT_PER                     2
#define WEIGHT_IDX_PREFERENCE_PER               2

#define ROAM_SCORE_DELTA			5

#define APS_AMSDU_HT_3K				(3839)
#define APS_AMSDU_HT_8K				(7935)
#define APS_AMSDU_VHT_HE_3K			(3895)
#define APS_AMSDU_VHT_HE_8K			(7991)
#define APS_AMSDU_VHT_HE_11K			(11454)

#define PPDU_DURATION 5 /* ms */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct WEIGHT_CONFIG {
	uint8_t ucChnlUtilWeight;
	uint8_t ucSnrWeight;
	uint8_t ucRssiWeight;
	uint8_t ucProbeRespWeight;
	uint8_t ucClientCntWeight;
	uint8_t ucApNumWeight;
	uint8_t ucBandWeight;
	uint8_t ucBandWidthWeight;
	uint8_t ucStbcWeight;
	uint8_t ucLastDeauthWeight;
	uint8_t ucBlackListWeight;
	uint8_t ucSaaWeight;
	uint8_t ucChnlIdleWeight;
	uint8_t ucOpchnlWeight;
	uint8_t ucTputWeight;
	uint8_t ucPreferenceWeight;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

enum ENUM_APS_REPLACE_REASON {
	/* reason to not replace */
	APS_LOW_SCORE = 0,
	APS_UNMATCH_BSSID,
	APS_UNMATCH_BSSID_HINT,
	APS_WORSE_RSSI,

	/* reason to replace */
	APS_FIRST_CANDIDATE,
	APS_HIGH_SCORE,
	APS_MATCH_BSSID,
	APS_MATCH_BSSID_HINT,
	APS_BETTER_RSSI,

	/* don't add after this */
	APS_REPLACE_REASON_NUM,
	APS_NEED_REPLACE = APS_FIRST_CANDIDATE,
};

/* mapping with ENUM_APS_REPLACE_REASON */
static const uint8_t *apucReplaceReasonStr[APS_REPLACE_REASON_NUM] = {
	(uint8_t *) DISP_STRING("LOW SCORE"),
	(uint8_t *) DISP_STRING("UNMATCH BSSID"),
	(uint8_t *) DISP_STRING("UNMATCH BSSID_HINT"),
	(uint8_t *) DISP_STRING("WORSE RSSI"),
	(uint8_t *) DISP_STRING("FIRST CANDIDATE"),
	(uint8_t *) DISP_STRING("HIGH SCORE"),
	(uint8_t *) DISP_STRING("MATCH BSSID"),
	(uint8_t *) DISP_STRING("MATCH BSSID_HINT"),
	(uint8_t *) DISP_STRING("BETTER RSSI"),
};

struct WEIGHT_CONFIG gasMtkWeightConfig[ROAM_TYPE_NUM] = {
	[ROAM_TYPE_RCPI] = {
		.ucChnlUtilWeight = WEIGHT_IDX_CHNL_UTIL,
		.ucRssiWeight = WEIGHT_IDX_RSSI,
		.ucProbeRespWeight = WEIGHT_IDX_PROBE_RSP,
		.ucClientCntWeight = WEIGHT_IDX_CLIENT_CNT,
		.ucApNumWeight = WEIGHT_IDX_AP_NUM,
		.ucBandWeight = WEIGHT_IDX_5G_BAND,
		.ucBandWidthWeight = WEIGHT_IDX_BAND_WIDTH,
		.ucStbcWeight = WEIGHT_IDX_STBC,
		.ucLastDeauthWeight = WEIGHT_IDX_DEAUTH_LAST,
		.ucBlackListWeight = WEIGHT_IDX_BLACK_LIST,
		.ucSaaWeight = WEIGHT_IDX_SAA,
		.ucChnlIdleWeight = WEIGHT_IDX_CHNL_IDLE,
		.ucOpchnlWeight = WEIGHT_IDX_OPCHNL,
		.ucTputWeight = WEIGHT_IDX_TPUT,
		.ucPreferenceWeight = WEIGHT_IDX_PREFERENCE
	},
	[ROAM_TYPE_PER] = {
		.ucChnlUtilWeight = WEIGHT_IDX_CHNL_UTIL_PER,
		.ucRssiWeight = WEIGHT_IDX_RSSI_PER,
		.ucProbeRespWeight = WEIGHT_IDX_PROBE_RSP_PER,
		.ucClientCntWeight = WEIGHT_IDX_CLIENT_CNT_PER,
		.ucApNumWeight = WEIGHT_IDX_AP_NUM_PER,
		.ucBandWeight = WEIGHT_IDX_5G_BAND_PER,
		.ucBandWidthWeight = WEIGHT_IDX_BAND_WIDTH_PER,
		.ucStbcWeight = WEIGHT_IDX_STBC_PER,
		.ucLastDeauthWeight = WEIGHT_IDX_DEAUTH_LAST_PER,
		.ucBlackListWeight = WEIGHT_IDX_BLACK_LIST_PER,
		.ucSaaWeight = WEIGHT_IDX_SAA_PER,
		.ucChnlIdleWeight = WEIGHT_IDX_CHNL_IDLE_PER,
		.ucOpchnlWeight = WEIGHT_IDX_OPCHNL_PER,
		.ucTputWeight = WEIGHT_IDX_TPUT_PER,
		.ucPreferenceWeight = WEIGHT_IDX_PREFERENCE_PER
	}
};

static uint8_t *apucBandStr[BAND_NUM] = {
	(uint8_t *) DISP_STRING("NULL"),
	(uint8_t *) DISP_STRING("2.4G"),
	(uint8_t *) DISP_STRING("5G"),
#if (CFG_SUPPORT_WIFI_6G == 1)
	(uint8_t *) DISP_STRING("6G")
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

const uint16_t mpduLen[CW_320_2MHZ + 1] = {
	[CW_20_40MHZ]  = 40,
	[CW_80MHZ] = 80,
	[CW_160MHZ] = 160,
	[CW_80P80MHZ] = 160,
	[CW_320_1MHZ]  = 320,
	[CW_320_2MHZ]  = 320
};

#if (CFG_MLO_LINK_PLAN_MODE == 0)
const uint8_t aucLinkPlan[] = {
	BIT(BAND_2G4),
	BIT(BAND_5G)
#if (CFG_SUPPORT_WIFI_6G == 1)
	 | BIT(BAND_6G)
#endif
};
#endif

#if (CFG_MLO_LINK_PLAN_MODE == 1)
const uint8_t aucLinkPlan[] = {
	BIT(BAND_2G4),
	BIT(BAND_5G),
#if (CFG_SUPPORT_WIFI_6G == 1)
	BIT(BAND_6G)
#endif
};
#endif

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define MAC_ADDR_HASH(_addr) \
	(_addr[0] ^ _addr[1] ^ _addr[2] ^ _addr[3] ^ _addr[4] ^ _addr[5])
#define AP_HASH(_addr) \
	((uint8_t) (MAC_ADDR_HASH(_addr) & (AP_HASH_SIZE - 1)))

#define CALCULATE_SCORE_BY_DEAUTH(prBssDesc, eRoamType) \
	(gasMtkWeightConfig[eRoamType].ucLastDeauthWeight * \
	(prBssDesc->prBlack && prBssDesc->prBlack->fgDeauthLastTime ? 0 : \
	BSS_FULL_SCORE))

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

struct AP_COLLECTION *apsHashGet(struct ADAPTER *ad,
	uint8_t *addr, uint8_t bidx, uint8_t is_mlo)
{
	struct AIS_SPECIFIC_BSS_INFO *s = aisGetAisSpecBssInfo(ad, bidx);
	struct AP_COLLECTION *a = NULL;

	a = s->arApHash[AP_HASH(addr)];

	while (a != NULL &&
	       (UNEQUAL_MAC_ADDR(a->aucAddr, addr) ||
	       a->fgIsMultiLink != is_mlo))
		a = a->hnext;
	return a;
}

void apsHashAdd(struct ADAPTER *ad, struct AP_COLLECTION *ap, uint8_t bidx)
{
	struct AIS_SPECIFIC_BSS_INFO *s = aisGetAisSpecBssInfo(ad, bidx);

	ap->hnext = s->arApHash[AP_HASH(ap->aucAddr)];
	s->arApHash[AP_HASH(ap->aucAddr)] = ap;
}

void apsHashDel(struct ADAPTER *ad, struct AP_COLLECTION *ap, uint8_t bidx)
{
	struct AIS_SPECIFIC_BSS_INFO *s = aisGetAisSpecBssInfo(ad, bidx);
	struct AP_COLLECTION *a = NULL;

	a = s->arApHash[AP_HASH(ap->aucAddr)];

	if (a == NULL)
		return;

	if (EQUAL_MAC_ADDR(a->aucAddr, ap->aucAddr) &&
		a->fgIsMultiLink == ap->fgIsMultiLink) {
		s->arApHash[AP_HASH(ap->aucAddr)] = a->hnext;
		return;
	}

	while (a->hnext != NULL &&
	       (UNEQUAL_MAC_ADDR(a->hnext->aucAddr, ap->aucAddr) ||
	       a->hnext->fgIsMultiLink != ap->fgIsMultiLink)) {
		a = a->hnext;
	}
	if (a->hnext != NULL)
		a->hnext = a->hnext->hnext;
	else
		DBGLOG(APS, INFO, "Could not remove AP " MACSTR
			   " from hash table\n", MAC2STR(ap->aucAddr));
}

uint8_t apsCanFormMld(struct ADAPTER *ad, struct BSS_DESC *bss, uint8_t bidx)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct WIFI_VAR *prWifiVar = &ad->rWifiVar;
	uint8_t ucEhtOption = FEATURE_ENABLED;

	if (AIS_INDEX(ad, bidx) < ad->rWifiVar.u4AisEHTNumber)
		ucEhtOption = prWifiVar->ucStaEht;
	else
		ucEhtOption = FEATURE_DISABLED;

	if (IS_FEATURE_DISABLED(ucEhtOption))
		return FALSE;

	if (!mldIsMloFeatureEnabled(ad, NETWORK_TYPE_AIS, FALSE) ||
	    !aisSecondLinkAvailable(ad, bidx))
		return FALSE;

	if (!bss->rMlInfo.fgValid)
		return FALSE;

	bss->rMlInfo.prBlock = aisQueryMldBlockList(ad, bss);

	if (!bss->rMlInfo.prBlock ||
	    bss->rMlInfo.prBlock->ucCount < ad->rWifiVar.ucMldRetryCount)
		return TRUE;

	DBGLOG(APS, LOUD, "Mld[" MACSTR "] is in mld blocklist\n",
	       MAC2STR(bss->rMlInfo.aucMldAddr));
#endif
	return FALSE;
}

uint8_t apsBssDescToLink(struct ADAPTER *ad,
	struct AP_COLLECTION *ap, struct BSS_DESC *bss, uint8_t bidx)
{
	uint8_t i = 0, j = 0;

	for (i = 0; i < ap->ucLinkNum; i++) {
		if (ap->aucMask[i] & BIT(bss->eBand))
			return i;
	}

	if (i == ap->ucLinkNum && i < MAX_LINK_PLAN_NUM) {
		for (j = 0; j < ARRAY_SIZE(aucLinkPlan); j++) {
			if (aucLinkPlan[j] & BIT(bss->eBand))
				break;
		}

		if (j < ARRAY_SIZE(aucLinkPlan)) {
			ap->aucMask[i] = aucLinkPlan[j];
			ap->ucLinkNum++;
			return i;
		}
	}

	return 0;
}

uint32_t apsAddBssDescToList(struct ADAPTER *ad, struct AP_COLLECTION *ap,
			struct BSS_DESC *bss, uint8_t bidx)
{
	uint8_t aidx = AIS_INDEX(ad, bidx);
	uint8_t l = apsBssDescToLink(ad, ap, bss, bidx);

	if (l >= MLD_LINK_MAX)
		return WLAN_STATUS_FAILURE;

	LINK_ENTRY_INITIALIZE(&bss->rLinkEntryEss[aidx]);
	LINK_INSERT_TAIL(&ap->arLinks[l], &bss->rLinkEntryEss[aidx]);
	ap->ucTotalCount++;

	return WLAN_STATUS_SUCCESS;
}

struct AP_COLLECTION *apsAddAp(struct ADAPTER *ad,
	struct BSS_DESC *bss, uint8_t bidx)
{
	struct AIS_SPECIFIC_BSS_INFO *s = aisGetAisSpecBssInfo(ad, bidx);
	struct LINK *ess = &s->rCurEssLink;
	struct AP_COLLECTION *ap;
	uint8_t i;

	ap = kalMemZAlloc(sizeof(*ap), VIR_MEM_TYPE);
	if (!ap) {
		DBGLOG(APS, WARN, "no resource for " MACSTR "\n",
			MAC2STR(bss->aucBSSID));
		return NULL;
	}

	COPY_MAC_ADDR(ap->aucAddr, bss->aucBSSID);
	ap->fgIsMultiLink = apsCanFormMld(ad, bss, bidx);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (ap->fgIsMultiLink)
		COPY_MAC_ADDR(ap->aucAddr, bss->rMlInfo.aucMldAddr);
#endif

	DBGLOG(APS, TRACE, "Add APC[" MACSTR "][MLO=%d]\n",
		MAC2STR(ap->aucAddr), ap->fgIsMultiLink);

	for (i = 0; i < MLD_LINK_MAX; i++)
		LINK_INITIALIZE(&ap->arLinks[i]);

	if (apsAddBssDescToList(ad, ap, bss, bidx)) {
		kalMemFree(ap, VIR_MEM_TYPE, sizeof(*ap));
		return NULL;
	}

	LINK_INSERT_TAIL(ess, &ap->rLinkEntry);
	apsHashAdd(ad, ap, bidx);

	return ap;
}

struct AP_COLLECTION *apsGetAp(struct ADAPTER *ad,
	struct BSS_DESC *bss, uint8_t bidx)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (apsCanFormMld(ad, bss, bidx))
		return apsHashGet(ad, bss->rMlInfo.aucMldAddr, bidx, TRUE);
#endif

	return apsHashGet(ad, bss->aucBSSID, bidx, FALSE);
}

void apsRemoveAp(struct ADAPTER *ad, struct AP_COLLECTION *ap, uint8_t bidx)
{
	DBGLOG(APS, TRACE,
		"Remove APC[" MACSTR "][MLO=%d] LinkNum=%d, TotalCount=%d\n",
		MAC2STR(ap->aucAddr), ap->fgIsMultiLink,
		ap->ucLinkNum, ap->ucTotalCount);

	apsHashDel(ad, ap, bidx);
	kalMemFree(ap, VIR_MEM_TYPE, sizeof(*ap));
}

void apsResetEssApList(struct ADAPTER *ad, uint8_t bidx)
{
	struct AIS_SPECIFIC_BSS_INFO *s = aisGetAisSpecBssInfo(ad, bidx);
	struct LINK *ess = &s->rCurEssLink;
	struct AP_COLLECTION *ap;

	while (!LINK_IS_EMPTY(ess)) {
		LINK_REMOVE_HEAD(ess, ap, struct AP_COLLECTION *);
		apsRemoveAp(ad, ap, bidx);
		/* mem is freed, don't use ap after this point */
	}

	kalMemZero(&s->arApHash[0], sizeof(s->arApHash));
	DBGLOG(APS, INFO, "BssIndex:%d reset prCurEssLink done\n", bidx);
}

uint16_t apsUpdateEssApList(struct ADAPTER *ad, uint8_t bidx)
{
	struct AP_COLLECTION *ap;
	struct BSS_DESC *bss = NULL;
	struct LINK *scan_result = &ad->rWifiVar.rScanInfo.rBSSDescList;
	struct CONNECTION_SETTINGS *conn = aisGetConnSettings(ad, bidx);
	uint16_t count = 0;

	LINK_FOR_EACH_ENTRY(bss, scan_result, rLinkEntry,
		struct BSS_DESC) {
		if (bss->ucChannelNum > 233)
			continue;
		if (!EQUAL_SSID(conn->aucSSID,
			conn->ucSSIDLen,
			bss->aucSSID, bss->ucSSIDLen) ||
			bss->eBSSType != BSS_TYPE_INFRASTRUCTURE)
			continue;

		ap = apsGetAp(ad, bss, bidx);
		if (ap) {
			if (!apsAddBssDescToList(ad, ap, bss, bidx))
				count++;
		} else {
			ap = apsAddAp(ad, bss, bidx);
			if (ap)
				count++;
		}
	}

	DBGLOG(APS, INFO, "Find %s in %d BSSes, result %d\n",
		conn->aucSSID, scan_result->u4NumElem, count);
	return count;
}

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

/* Channel Utilization: weight index will be */
static uint16_t apsCalculateScoreByChnlInfo(
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo, uint8_t ucChannel,
	enum ROAM_TYPE eRoamType)
{
	struct ESS_CHNL_INFO *prEssChnlInfo = &prAisSpecificBssInfo->
		arCurEssChnlInfo[0];
	uint8_t i = 0;
	uint16_t u2Score = 0;
	uint8_t weight = 0;

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
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
			DBGLOG(APS, TRACE, "channel %d, AP num %d\n",
				ucChannel, prEssChnlInfo[i].ucApNum);
			break;
		}
	}
	return u2Score;
}

static uint16_t apsCalculateScoreByBW(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ROAM_TYPE eRoamType,
	uint8_t ucBssIndex)
{
	uint16_t u2Score = 0;

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

	switch (prBssDesc->eChannelWidth) {
	case CW_20_40MHZ:
		u2Score = 40;
		break;
	case CW_80MHZ:
		u2Score = 60;
		break;
	case CW_160MHZ:
	case CW_80P80MHZ:
		u2Score = 80;
		break;
	case CW_320_1MHZ:
	case CW_320_2MHZ:
		u2Score = 100;
		break;
	}

	return u2Score * gasMtkWeightConfig[eRoamType].ucBandWidthWeight;
}

static uint16_t apsCalculateScoreByBand(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, int8_t cRssi, enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

	switch (prBssDesc->eBand) {
	case BAND_2G4:
		u2Score = 0;
		break;
	case BAND_5G:
		if (prAdapter->fgEnable5GBand && cRssi > LOW_RSSI_FOR_5G_BAND)
			u2Score = 80;
		break;
#if (CFG_SUPPORT_WIFI_6G == 1)
	case BAND_6G:
		if (prAdapter->fgIsHwSupport6G && cRssi > LOW_RSSI_FOR_5G_BAND)
			u2Score = BSS_FULL_SCORE;
		break;
#endif
	default:
		break;
	}

	return u2Score * gasMtkWeightConfig[eRoamType].ucBandWeight;
}

static uint16_t apsCalculateScoreByClientCnt(struct BSS_DESC *prBssDesc,
			enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;
	uint16_t u2StaCnt = 0;
#define BSS_STA_CNT_NORMAL_SCORE 50
#define BSS_STA_CNT_GOOD_THRESOLD 10

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

	DBGLOG(APS, TRACE, "Exist bss load %d, sta cnt %d\n",
			prBssDesc->fgExsitBssLoadIE, prBssDesc->u2StaCnt);

	if (!prBssDesc->fgExsitBssLoadIE) {
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

static uint16_t apsCalculateScoreByStbc(struct BSS_DESC *prBssDesc,
	enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

	if (prBssDesc->fgMultiAnttenaAndSTBC)
		u2Score = BSS_FULL_SCORE;

#if (CFG_SUPPORT_WIFI_6G == 1)
	/* assume stbc is supported because 6g AP doesn't carry ht cap */
	if (prBssDesc->eBand == BAND_6G)
		u2Score = BSS_FULL_SCORE;
#endif

	u2Score *= gasMtkWeightConfig[eRoamType].ucStbcWeight;

	return u2Score;
}

static uint16_t apsCalculateScoreByRssi(struct BSS_DESC *prBssDesc,
	enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;
	int8_t cRssi = RCPI_TO_dBm(prBssDesc->ucRCPI);

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

	if (cRssi >= BEST_RSSI)
		u2Score = 100;
	else if (prBssDesc->eBand == BAND_5G && cRssi >= GOOD_RSSI_FOR_HT_VHT)
		u2Score = 100;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prBssDesc->eBand == BAND_6G && cRssi >= GOOD_RSSI_FOR_HT_VHT)
		u2Score = 100;
#endif
	else if (prBssDesc->eBand == BAND_2G4 && cRssi < MINIMUM_RSSI_2G4)
		u2Score = 0;
	else if (prBssDesc->eBand == BAND_5G && cRssi < MINIMUM_RSSI_5G)
		u2Score = 0;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prBssDesc->eBand == BAND_6G && cRssi < MINIMUM_RSSI_6G)
		u2Score = 0;
#endif
	else if (cRssi <= -98)
		u2Score = 0;
	else
		u2Score = (cRssi + 98) * 2;

	u2Score *= gasMtkWeightConfig[eRoamType].ucRssiWeight;

	return u2Score;
}

static uint16_t apsCalculateScoreBySaa(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
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

static uint16_t apsCalculateScoreByIdleTime(struct ADAPTER *prAdapter,
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
	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}
	if (prBssDesc->eBand >= BAND_NUM) {
		DBGLOG(APS, WARN, "Invalid Band %d\n", prBssDesc->eBand);
		return 0;
	}
	if (prBssDesc->fgExsitBssLoadIE) {
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
				if (eRoamType == ROAM_TYPE_PER) {
					score = idle > BSS_FULL_SCORE ?
						BSS_FULL_SCORE : idle;
					goto done;
				}
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

	DBGLOG(APS, TRACE,
		MACSTR
		" Band[%s],chl[%d],slt[%d],ld[%d] idle Score %d,rssi[%d],cu[%d],cuR[%d],rf[%d],rw[%d],cf[%d],cw[%d]\n",
		MAC2STR(prBssDesc->aucBSSID),
		apucBandStr[prBssDesc->eBand],
		prBssDesc->ucChannelNum, slot,
		prBssDesc->fgExsitBssLoadIE, score, rssi, cu, cuRatio,
		rssiFactor, rssiWeight, cuFactor, cuWeight);
done:
	return score * gasMtkWeightConfig[eRoamType].ucChnlIdleWeight;
}

uint16_t apsCalculateScoreByBlackList(struct ADAPTER *prAdapter,
	    struct BSS_DESC *prBssDesc, enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

	if (!prBssDesc->prBlack)
		u2Score = 100;
	else if (rsnApOverload(prBssDesc->prBlack->u2AuthStatus,
		prBssDesc->prBlack->u2DeauthReason) ||
		 prBssDesc->prBlack->ucCount >= 10)
		u2Score = 0;
	else
		u2Score = 100 - prBssDesc->prBlack->ucCount * 10;

	return u2Score * gasMtkWeightConfig[eRoamType].ucBlackListWeight;
}

uint16_t apsCalculateScoreByTput(struct ADAPTER *prAdapter,
	    struct BSS_DESC *prBssDesc, enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

#if CFG_SUPPORT_MBO
	if (prBssDesc->fgExistEspIE)
		u2Score = (prBssDesc->u4EspInfo[ESP_AC_BE] >> 8) & 0xff;
#endif

	return u2Score * gasMtkWeightConfig[eRoamType].ucTputWeight;
}

uint16_t apsCalculateScoreByPreference(struct ADAPTER *prAdapter,
	    struct BSS_DESC *prBssDesc, enum ENUM_ROAMING_REASON eRoamReason)
{
	enum ROAM_TYPE eRoamType = roamReasonToType(eRoamReason);

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}

#if CFG_SUPPORT_802_11K
	if (prBssDesc->prNeighbor)
		return (prBssDesc->prNeighbor->ucPreference + 100) *
		       gasMtkWeightConfig[eRoamType].ucPreferenceWeight;
#endif

	return 100 * gasMtkWeightConfig[eRoamType].ucPreferenceWeight;
}

uint16_t apsCalculateTotalScore(struct ADAPTER *prAdapter,
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
	uint16_t u2ScoreBand = 0;
	uint16_t u2ScoreSaa = 0;
	uint16_t u2ScoreIdleTime = 0;
	uint16_t u2ScoreTotal = 0;
	uint16_t u2BlackListScore = 0;
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

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
		return 0;
	}
	if (prBssDesc->eBand >= BAND_NUM) {
		DBGLOG(APS, WARN, "Invalid Band %d\n", prBssDesc->eBand);
		return 0;
	}
	u2ScoreBandwidth = apsCalculateScoreByBW(prAdapter,
		prBssDesc, eRoamType, ucBssIndex);
	u2ScoreStaCnt = apsCalculateScoreByClientCnt(prBssDesc, eRoamType);
	u2ScoreSTBC = apsCalculateScoreByStbc(prBssDesc, eRoamType);
	u2ScoreChnlInfo = apsCalculateScoreByChnlInfo(prAisSpecificBssInfo,
				prBssDesc->ucChannelNum, eRoamType);
	u2ScoreSnrRssi = apsCalculateScoreByRssi(prBssDesc, eRoamType);
	u2ScoreDeauth = CALCULATE_SCORE_BY_DEAUTH(prBssDesc, eRoamType);
	u2ScoreBand = apsCalculateScoreByBand(prAdapter, prBssDesc,
		cRssi, eRoamType);
	u2ScoreSaa = apsCalculateScoreBySaa(prAdapter, prBssDesc, eRoamType);
	u2ScoreIdleTime = apsCalculateScoreByIdleTime(prAdapter,
		prBssDesc->ucChannelNum, eRoamType, prBssDesc, ucBssIndex,
		prBssDesc->eBand);
	u2BlackListScore =
	       apsCalculateScoreByBlackList(prAdapter, prBssDesc, eRoamType);
	u2PreferenceScore =
	      apsCalculateScoreByPreference(prAdapter, prBssDesc, eRoamReason);

	u2TputScore = apsCalculateScoreByTput(prAdapter, prBssDesc, eRoamType);

	u2ScoreTotal = u2ScoreBandwidth + u2ScoreChnlInfo +
		u2ScoreDeauth + u2ScoreSnrRssi + u2ScoreStaCnt + u2ScoreSTBC +
		u2ScoreBand + u2BlackListScore + u2ScoreSaa +
		u2ScoreIdleTime + u2TputScore;

#if (CFG_SUPPORT_AVOID_DESENSE == 1)
	if (fgBssInDenseRange)
		u2ScoreTotal /= 4;
	kalSnprintf(extra, sizeof(extra), ", DESENSE[%d]", fgBssInDenseRange);
#endif

#define TEMP_LOG_TEMPLATE\
		"BSS["MACSTR"] Band[%s],Score[%d],cRSSI[%d],DE[%d]"\
		",RSSI[%d],BD[%d],BL[%d],SAA[%d]"\
		",BW[%d],SC[%d],ST[%d],CI[%d],IT[%d],CU[%d,%d],PF[%d]"\
		",TPUT[%d]%s\n"

	DBGLOG(APS, INFO,
		TEMP_LOG_TEMPLATE,
		MAC2STR(prBssDesc->aucBSSID), apucBandStr[prBssDesc->eBand],
		u2ScoreTotal, cRssi,
		u2ScoreDeauth,
		u2ScoreSnrRssi, u2ScoreBand, u2BlackListScore,
		u2ScoreSaa, u2ScoreBandwidth, u2ScoreStaCnt,
		u2ScoreSTBC, u2ScoreChnlInfo, u2ScoreIdleTime,
		prBssDesc->fgExsitBssLoadIE,
		prBssDesc->ucChnlUtilization,
		u2PreferenceScore,
		u2TputScore, extra);

#undef TEMP_LOG_TEMPLATE

	return u2ScoreTotal;
}

static uint8_t apsSanityCheckBssDesc(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	uint32_t bmap = aisGetBssIndexBmap(ais);
	uint8_t connected = prBssDesc->fgIsConnected & bmap;
	struct BSS_INFO *prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

#if CFG_SUPPORT_MBO
	struct PARAM_BSS_DISALLOWED_LIST *disallow;
	uint32_t i = 0;

	disallow = &prAdapter->rWifiVar.rBssDisallowedList;
	for (i = 0; i < disallow->u4NumBssDisallowed; ++i) {
		uint32_t index = i * MAC_ADDR_LEN;

		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
				&disallow->aucList[index])) {
			DBGLOG(APS, WARN, MACSTR" disallowed list\n",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}
	}

	if (prBssDesc->fgIsDisallowed) {
		DBGLOG(APS, WARN, MACSTR" disallowed\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	if (prBssDesc->prBlack && prBssDesc->prBlack->fgDisallowed &&
	    !(prBssDesc->prBlack->i4RssiThreshold > 0 &&
	      RCPI_TO_dBm(prBssDesc->ucRCPI) >
			prBssDesc->prBlack->i4RssiThreshold)) {
		DBGLOG(APS, WARN, MACSTR" disallowed delay, rssi %d(%d)\n",
			MAC2STR(prBssDesc->aucBSSID),
			RCPI_TO_dBm(prBssDesc->ucRCPI),
			prBssDesc->prBlack->i4RssiThreshold);
		return FALSE;
	}

	if (prBssDesc->prBlack && prBssDesc->prBlack->fgDisallowed) {
		DBGLOG(APS, WARN, MACSTR" disallowed delay\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}
#endif

	if (!prBssDesc->fgIsInUse) {
		DBGLOG(APS, WARN, MACSTR" is not in use\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	if ((prBssDesc->eBand == BAND_2G4 &&
		prAdapter->rWifiVar.ucDisallowBand2G) ||
	    (prBssDesc->eBand == BAND_5G &&
		prAdapter->rWifiVar.ucDisallowBand5G)
#if (CFG_SUPPORT_WIFI_6G == 1)
	 || (prBssDesc->eBand == BAND_6G &&
		prAdapter->rWifiVar.ucDisallowBand6G)
#endif
	) {
		DBGLOG(APS, WARN, MACSTR" Band[%s] is not allowed\n",
			MAC2STR(prBssDesc->aucBSSID),
			apucBandStr[prBssDesc->eBand]);
		return FALSE;
	}

	if (prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE) {
		DBGLOG(APS, WARN, MACSTR" is not infrastructure\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	if (prBssDesc->prBlack) {
		if (prBssDesc->prBlack->fgIsInFWKBlacklist) {
			DBGLOG(APS, WARN, MACSTR" in FWK blacklist\n",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}

		if (prBssDesc->prBlack->fgDeauthLastTime) {
			DBGLOG(APS, WARN, MACSTR " is sending deauth.\n",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}

		if (prBssDesc->prBlack->fgArpNoResponse &&
		    prBssDesc->prBlack->ucCount >= 2) {
			log_dbg(SCN, WARN, MACSTR " is arp no response many times.\n",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}

		if (prBssDesc->prBlack->ucCount >= 10)  {
			DBGLOG(APS, WARN,
				MACSTR
				" Skip AP that add toblacklist count >= 10\n",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}
	}

	/* roaming case */
	if ((prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED ||
	    aisFsmIsInProcessPostpone(prAdapter, ucBssIndex))) {
		if (!connected && prBssDesc->ucRCPI < RCPI_FOR_DONT_ROAM) {
			DBGLOG(APS, INFO, MACSTR " low rssi %d\n",
				MAC2STR(prBssDesc->aucBSSID),
				RCPI_TO_dBm(prBssDesc->ucRCPI));
			return FALSE;
		}
	}

	if (!(prBssDesc->ucPhyTypeSet &
		(prAdapter->rWifiVar.ucAvailablePhyTypeSet))) {
		DBGLOG(APS, WARN,
			MACSTR" ignore unsupported ucPhyTypeSet = %x\n",
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->ucPhyTypeSet);
		return FALSE;
	}

	if (prBssDesc->fgIsUnknownBssBasicRate) {
		DBGLOG(APS, WARN, MACSTR" unknown bss basic rate\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	if (!rlmDomainIsLegalChannel(prAdapter, prBssDesc->eBand,
		prBssDesc->ucChannelNum)) {
		DBGLOG(APS, WARN, MACSTR" band %d channel %d is not legal\n",
			MAC2STR(prBssDesc->aucBSSID), prBssDesc->eBand,
			prBssDesc->ucChannelNum);
		return FALSE;
	}

	if (!connected &&
	    CHECK_FOR_TIMEOUT(kalGetTimeTick(), prBssDesc->rUpdateTime,
		SEC_TO_SYSTIME(wlanWfdEnabled(prAdapter) ?
		SCN_BSS_DESC_STALE_SEC_WFD : SCN_BSS_DESC_STALE_SEC))) {
		DBGLOG(APS, WARN, MACSTR " description is too old.\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	if (!rsnPerformPolicySelection(prAdapter, prBssDesc,
		ucBssIndex)) {
		DBGLOG(APS, WARN, MACSTR " rsn policy select fail.\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}
	if (aisGetAisSpecBssInfo(prAdapter,
		ucBssIndex)->fgCounterMeasure) {
		DBGLOG(APS, WARN, MACSTR " Skip in counter measure period.\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}


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
				DBGLOG(APS, INFO,
				     MACSTR " preference is 0, skip it\n",
				     MAC2STR(prBssDesc->aucBSSID));
				return FALSE;
			}

			if ((ucRequestMode & WNM_BSS_TM_REQ_ABRIDGED) &&
			    !prBssDesc->prNeighbor &&
			    prBtmParam->ucDisImmiState !=
				    AIS_BTM_DIS_IMMI_STATE_3) {
				DBGLOG(APS, INFO,
				     MACSTR " not in candidate list, skip it\n",
				     MAC2STR(prBssDesc->aucBSSID));
				return FALSE;
			}

		}
	}
#endif
	return TRUE;
}

#if CFG_SUPPORT_802_11K
struct NEIGHBOR_AP *apsGetNeighborAPEntry(
	struct ADAPTER *prAdapter, struct BSS_DESC *bss, uint8_t ucBssIndex)
{
	struct LINK *prNeighborAPLink =
		&aisGetAisSpecBssInfo(prAdapter, ucBssIndex)
		->rNeighborApList.rUsingLink;
	struct NEIGHBOR_AP *prNeighborAP = NULL;

	LINK_FOR_EACH_ENTRY(prNeighborAP, prNeighborAPLink, rLinkEntry,
			    struct NEIGHBOR_AP)
	{
		if (EQUAL_MAC_ADDR(prNeighborAP->aucBssid, bss->aucBSSID))
			return prNeighborAP;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (bss->rMlInfo.fgValid && prNeighborAP->fgIsMld &&
		   EQUAL_MAC_ADDR(prNeighborAP->aucMldAddr,
				  bss->rMlInfo.aucMldAddr) &&
		   (prNeighborAP->u2ValidLinks & BIT(bss->rMlInfo.ucLinkIndex)))
			return prNeighborAP;
#endif
	}
	return NULL;
}
#endif

uint8_t apsIntraNeedReplace(struct ADAPTER *ad,
	struct BSS_DESC *cand, struct BSS_DESC *curr,
	uint16_t cand_score, uint16_t curr_score,
	enum ENUM_ROAMING_REASON reason, uint8_t bidx)
{
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(ad, bidx);
	uint32_t bmap = aisGetBssIndexBmap(ais);

	if (!cand && curr && (curr->fgIsConnected & bmap))
		return TRUE;

	if (curr_score > cand_score)
		return TRUE;

	return FALSE;
}

uint8_t apsIntraUpdateTargetAp(struct ADAPTER *ad, struct AP_COLLECTION *ap,
	uint8_t link_idx, uint16_t goal_score,
	enum ENUM_ROAMING_REASON reason, uint8_t bidx)
{
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(ad, bidx);
	uint32_t bmap = aisGetBssIndexBmap(ais);
	struct CONNECTION_SETTINGS *conn = aisGetConnSettings(ad, bidx);
	enum ENUM_PARAM_CONNECTION_POLICY policy = conn->eConnectionPolicy;
	struct LINK *link = &ap->arLinks[link_idx];
	uint8_t aidx = AIS_INDEX(ad, bidx);
	struct BSS_DESC *bss, *cand = NULL;
	uint16_t score;
	uint8_t search_blk = FALSE;

try_again:
	LINK_FOR_EACH_ENTRY(bss, link, rLinkEntryEss[aidx], struct BSS_DESC) {
		if (!search_blk) {
			/* update blacklist info */
			bss->prBlack = aisQueryBlackList(ad, bss);
#if CFG_SUPPORT_802_11K
			/* update neighbor report entry */
			bss->prNeighbor = apsGetNeighborAPEntry(
				ad, bss, bidx);
#endif
		}

		/*
		 * Skip if
		 * 1. sanity check fail or
		 * 2. bssid is in driver's blacklist in 1st try
		 */
		if (!apsSanityCheckBssDesc(ad, bss, reason, bidx))
			continue;
		if (!search_blk && bss->prBlack) {
			DBGLOG(APS, INFO, MACSTR" in blacklist\n",
				MAC2STR(bss->aucBSSID));
			continue;
		}

		/* pick by bssid first */
		if (policy == CONNECT_BY_BSSID) {
			if (EQUAL_MAC_ADDR(bss->aucBSSID, conn->aucBSSID)) {
				ap->fgIsMatchBssid = TRUE;
				cand = bss;
				goal_score = 30000;
				break;
			}
		} else if (policy == CONNECT_BY_BSSID_HINT) {
			uint8_t oce = FALSE;

#if CFG_SUPPORT_MBO
			oce = ad->rWifiVar.u4SwTestMode ==
				ENUM_SW_TEST_MODE_SIGMA_OCE;
#endif
			if (!oce && EQUAL_MAC_ADDR(bss->aucBSSID,
					conn->aucBSSIDHint)) {
#if (CFG_SUPPORT_AVOID_DESENSE == 1)
				if (IS_CHANNEL_IN_DESENSE_RANGE(
					ad,
					bss->ucChannelNum,
					bss->eBand)) {
					DBGLOG(APS, INFO,
						"Do network selection even match bssid_hint\n");
				} else
#endif
				{
					ap->fgIsMatchBssidHint = TRUE;
					cand = bss;
					goal_score = 20000;
					break;
				}
			}
		}

		score = apsCalculateTotalScore(ad, bss, reason, bidx);
		if (apsIntraNeedReplace(ad, cand, bss,
			goal_score, score, reason, bidx)) {
			cand = bss;
			goal_score = score;
		}
	}

	if (cand) {
		if ((cand->fgIsConnected & bmap) &&
		    !search_blk && link->u4NumElem > 0) {
			search_blk = TRUE;
			DBGLOG(APS, INFO, "Can't roam out, try blacklist\n");
			goto try_again;
		}

		ap->aprTarget[link_idx] = cand;
		ap->au2TargetScore[link_idx] = goal_score;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (cand->rMlInfo.fgValid)
			DBGLOG(APS, INFO,
				"[%d/%d] Select BSS["
				MACSTR " %s mld=" MACSTR
				"] score[%d] conn[%d] policy[%d] bssid[%d] bssid_hint[%d]\n",
				link_idx, ap->ucLinkNum,
				MAC2STR(cand->aucBSSID),
				apucBandStr[cand->eBand],
				MAC2STR(cand->rMlInfo.aucMldAddr), goal_score,
				cand->fgIsConnected & bmap, policy,
				ap->fgIsMatchBssid, ap->fgIsMatchBssidHint);
		else
#endif
			DBGLOG(APS, INFO,
				"[%d/%d] Select BSS[" MACSTR
				" %s] score[%d] conn[%d] policy[%d] bssid[%d] bssid_hint[%d]\n",
				link_idx, ap->ucLinkNum,
				MAC2STR(cand->aucBSSID),
				apucBandStr[cand->eBand], goal_score,
				cand->fgIsConnected & bmap, policy,
				ap->fgIsMatchBssid, ap->fgIsMatchBssidHint);
		goto done;
	}

	/* if No Candidate BSS is found, try BSSes which are in blacklist */
	if (!search_blk && link->u4NumElem > 0) {
		search_blk = TRUE;
		DBGLOG(APS, INFO, "No Bss is found, Try blacklist\n");
		goto try_again;
	}

	DBGLOG(APS, INFO,
		"[%d/%d] Select NONE policy[%d] bssid[%d] bssid_hint[%d]\n",
		link_idx, ap->ucLinkNum,
		policy, ap->fgIsMatchBssid, ap->fgIsMatchBssidHint);
done:
	return cand != NULL;
}

uint8_t apsIsValidBssDesc(struct ADAPTER *ad, struct BSS_DESC *bss,
	enum ENUM_ROAMING_REASON reason, uint8_t bidx)
{
	if (!bss)
		return FALSE;
	if (aisQueryBlackList(ad, bss))
		return FALSE;
	if (reason == ROAMING_REASON_TEMP_REJECT)
		return FALSE;
	if (reason == ROAMING_REASON_BTM) {
		struct NEIGHBOR_AP *nei = apsGetNeighborAPEntry(ad, bss, bidx);

		if (nei && nei->fgPrefPresence && !nei->ucPreference)
			return FALSE;
	}

	return TRUE;
}

uint16_t apsGetAmsduByte(struct BSS_DESC *bss)
{
	uint16_t bssAmsduLen = 0, amsduLen = 0;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (bss->eBand == BAND_6G) {
		bssAmsduLen = (bss->u2MaximumMpdu &
			HE_6G_CAP_INFO_MAX_MPDU_LEN_MASK) & 0xffff;

		if (bssAmsduLen == HE_6G_CAP_INFO_MAX_MPDU_LEN_8K)
			amsduLen = APS_AMSDU_VHT_HE_8K;
		else if (bssAmsduLen == HE_6G_CAP_INFO_MAX_MPDU_LEN_11K)
			amsduLen = APS_AMSDU_VHT_HE_11K;
		else if (bssAmsduLen == VHT_CAP_INFO_MAX_MPDU_LEN_3K)
			amsduLen = APS_AMSDU_VHT_HE_3K;
		else {
			DBGLOG(APS, INFO,
				"Unexpected HE maximum mpdu length\n");
			amsduLen = APS_AMSDU_VHT_HE_3K;
		}
		return amsduLen;
	}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	if (bss->fgIsEHTPresent == TRUE) {
		bssAmsduLen = (bss->u2MaximumMpdu &
			EHT_MAC_CAP_MAX_MPDU_LEN_MASK) & 0xffff;

		if (bssAmsduLen == EHT_MAC_CAP_MAX_MPDU_LEN_8K)
			amsduLen = APS_AMSDU_VHT_HE_8K;
		else if (bssAmsduLen == EHT_MAC_CAP_MAX_MPDU_LEN_11K)
			amsduLen = APS_AMSDU_VHT_HE_11K;
		else if (bssAmsduLen == EHT_MAC_CAP_MAX_MPDU_LEN_3K)
			amsduLen = APS_AMSDU_VHT_HE_3K;
		else {
			DBGLOG(APS, INFO,
				"Unexpected HE maximum mpdu length\n");
			amsduLen = APS_AMSDU_VHT_HE_3K;
		}
		return amsduLen;
	}
#endif
	if (bss->u2MaximumMpdu) {
		bssAmsduLen = (bss->u2MaximumMpdu &
			VHT_CAP_INFO_MAX_MPDU_LEN_MASK) & 0xffff;
		if (bss->fgIsVHTPresent) {
			if (bssAmsduLen == VHT_CAP_INFO_MAX_MPDU_LEN_8K)
				amsduLen = APS_AMSDU_VHT_HE_8K;
			else if (bssAmsduLen ==
				VHT_CAP_INFO_MAX_MPDU_LEN_11K)
				amsduLen = APS_AMSDU_VHT_HE_11K;
			else if (bssAmsduLen ==
				VHT_CAP_INFO_MAX_MPDU_LEN_3K)
				amsduLen = APS_AMSDU_VHT_HE_3K;
			else {
				DBGLOG(APS, INFO,
					"Unexpected VHT maximum mpdu length\n");
				amsduLen = APS_AMSDU_VHT_HE_3K;
			}
		} else
			amsduLen = APS_AMSDU_HT_8K;
	} else {
		if (bss->fgIsVHTPresent)
			amsduLen = APS_AMSDU_VHT_HE_3K;
		else
			amsduLen = APS_AMSDU_HT_3K;
	}

	return amsduLen;
}

uint32_t apsGetEstimatedTput(struct ADAPTER *ad, struct BSS_DESC *bss)
{
	uint8_t rcpi = 0;
	uint16_t amsduByte = apsGetAmsduByte(bss);
	uint16_t baSize = mpduLen[bss->eChannelWidth];
	uint16_t slot = 0;
	uint32_t airTime = 0, idle = 0, ideal = 0, slope = 0, tput = 0, est = 0;

	/* TODO: get CU from other AP's cu if AP doesn't support BSS load */
	if (bss->fgExsitBssLoadIE) {
		airTime = (255 - bss->ucChnlUtilization) * 100 / 255;
	} else {
#if CFG_SUPPORT_ROAMING
		slot = roamingGetChIdleSlot(ad, bss->eBand, bss->ucChannelNum);
#endif
		/* 90000 ms = 90ms dwell time to micro sec */
		idle = (slot * 9 * 100) / (90000);
		airTime  = idle > 100 ? 100 : idle;
		/* Give a default value of air time */
		if (airTime == 0)
			airTime = 45;
	}

	/* Unit: mbps */
	ideal = baSize * amsduByte * 8 / PPDU_DURATION;
	/* slope: from peak to zero -> RCPI from 100 to 50 */
	slope = ideal / 50;
	rcpi = bss->ucRCPI < 50 ? 50 : (bss->ucRCPI > 100 ? 100 : bss->ucRCPI);
	tput = slope * (rcpi - 50);
	est = airTime * tput / 100;

	DBGLOG(APS, INFO, "BSS["MACSTR
		"] EST: %d ideal[%d] ba[%d] amsdu[%d] slope[%d] rcpi[%d] est[%d] airTime[%d] slot[%d]\n",
		MAC2STR(bss->aucBSSID), est, ideal, baSize, amsduByte,
		slope, rcpi, tput, airTime, slot);

	return est;
}

struct AP_COLLECTION *apsIntraApSelection(struct ADAPTER *ad,
	enum ENUM_ROAMING_REASON reason, uint8_t bidx)
{
	struct AIS_SPECIFIC_BSS_INFO *s = aisGetAisSpecBssInfo(ad, bidx);
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(ad, bidx);
	uint32_t bmap = aisGetBssIndexBmap(ais);
	struct LINK *ess = &s->rCurEssLink;
	struct AP_COLLECTION *ap, *nap, *current_ap = NULL;
	struct BSS_DESC *bss;
	uint16_t delta = 0, base = 0, goal = 0, score = 0;
	int i, j, k;

	if (reason == ROAMING_REASON_POOR_RCPI ||
	    reason == ROAMING_REASON_INACTIVE)
		delta += ROAM_SCORE_DELTA;

	/* minium requirement */
	for (i = 0; i < MLD_LINK_MAX; i++) {
		bss = aisGetLinkBssDesc(ais, i);

		if (!apsIsValidBssDesc(ad, bss, reason, bidx))
			continue;

		score = apsCalculateTotalScore(ad, bss, reason, bidx);
		if (base == 0 || score < base)
			base = score;
	}

	goal = base * (100 + delta) / 100;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	DBGLOG(APS, INFO,
		"GOAL SCORE=%d (EnableMlo=%d,StaMldLinkMax=%d,StaPreferMldAddr=%d)\n",
		goal,
		ad->rWifiVar.ucEnableMlo,
		ad->rWifiVar.ucStaMldLinkMax,
		ad->rWifiVar.ucStaPreferMldAddr);
#else
	DBGLOG(APS, INFO, "GOAL SCORE=%d\n", goal);
#endif

	LINK_FOR_EACH_ENTRY_SAFE(ap, nap,
			ess, rLinkEntry, struct AP_COLLECTION) {
		uint8_t found = FALSE;
		uint32_t akm = 0;
		uint16_t best = 0;

		for (i = 0; i < ap->ucLinkNum; i++) {
			found |= apsIntraUpdateTargetAp(ad, ap,
				i, goal, reason, bidx);
			score = ap->au2TargetScore[i];
			bss = ap->aprTarget[i];

			/* use akm of best ap as target akm */
			if (bss && (best == 0 || score > best)) {
				best = score;
				akm = bss->u4RsnSelectedAKMSuite;
			}
		}

		if (!found) {
			ap->ucLinkNum = 0;
			continue;
		}

		for (i = 0; i < ap->ucLinkNum; i++) {
			bss = ap->aprTarget[i];

			/* use lower score to find links if already found one */
			if (!bss) {
				DBGLOG(APS, INFO, "GOAL SCORE[%d/%d]=0\n",
					i, ap->ucLinkNum);
				apsIntraUpdateTargetAp(ad, ap,
					i, 0, reason, bidx);
			}

			/* check common akm */
			bss = ap->aprTarget[i];
			if (bss && bss->u4RsnSelectedAKMSuite != akm) {
				DBGLOG(APS, INFO,
					"Remove target akm 0x%x!=0x%x\n",
					bss->u4RsnSelectedAKMSuite, akm);
				ap->aprTarget[i] = NULL;
				ap->au2TargetScore[i] = 0;
			}
		}

		/* insertion sort by score */
		for (i = 1; i < ap->ucLinkNum; i++) {
			score = ap->au2TargetScore[i];
			bss = ap->aprTarget[i];

			for (j = i - 1;
			     j >= 0 && ap->au2TargetScore[j] < score; j--) {
				ap->au2TargetScore[j + 1] =
					ap->au2TargetScore[j];
				ap->aprTarget[j + 1] =
					ap->aprTarget[j];
			}

			ap->au2TargetScore[j + 1] = score;
			ap->aprTarget[j + 1] = bss;
		}

		/* ensure no null target */
		ap->ucLinkNum = 0;
		for (i = 0; i < MAX_LINK_PLAN_NUM; i++) {
			if (ap->aprTarget[i])
				ap->ucLinkNum++;
		}

		/* trim ap */
		if (ap->ucLinkNum > ad->rWifiVar.ucStaMldLinkMax) {
			DBGLOG(APS, INFO, "trim links %d => %d",
				ap->ucLinkNum, ad->rWifiVar.ucStaMldLinkMax);
			ap->ucLinkNum = ad->rWifiVar.ucStaMldLinkMax;
		}

		for (i = 0, j = 0, k = 0; i < ap->ucLinkNum; i++) {
			ap->u4Tput += apsGetEstimatedTput(ad, ap->aprTarget[i]);

			if (ap->aprTarget[i]->prBlack)
				j++;

			if (ap->aprTarget[i]->fgIsConnected & bmap)
				k++;
		}

		if (j == ap->ucLinkNum)
			ap->fgIsAllLinkInBlackList = TRUE;

		if (k == ap->ucLinkNum) {
			ap->fgIsAllLinkConnected = TRUE;
			current_ap = ap;
		}

		DBGLOG(APS, INFO,
			"-- APC[" MACSTR "], Total(%d) EST: %d %s%s\n",
			MAC2STR(ap->aucAddr), ap->ucLinkNum, ap->u4Tput,
			ap->fgIsAllLinkConnected ? "(connected)" : "",
			ap->fgIsAllLinkInBlackList ? "(in blocklist)" : "");
	}

	return current_ap;
}

uint32_t apsCalculateApScore(struct ADAPTER *ad,
	struct AP_COLLECTION *ap, enum ENUM_ROAMING_REASON reason,
	uint8_t bidx)
{
	uint32_t score = 0;

	switch (reason) {
#if CFG_SUPPORT_ROAMING
	case ROAMING_REASON_BTM: {
		uint8_t i;

		if (!aisCheckNeighborApValidity(ad, bidx)) {
			score = ap->u4Tput;
		} else {
			for (i = 0; i < ap->ucLinkNum; i++) {
				uint32_t pref;

				if (!ap->aprTarget[i])
					continue;

				pref = apsCalculateScoreByPreference(
					ad, ap->aprTarget[i], reason);

				/* select highest pref as tput score */
				if (score == 0 || pref > score)
					score = pref;
			}
		}
	}
		break;
#endif
	default:
		score = ap->u4Tput;
		break;
	}

	return score;
}

static uint8_t apsNeedReplaceCandidateByRssi(struct ADAPTER *prAdapter,
	struct BSS_DESC *prCandBss, struct BSS_DESC *prCurrBss,
	enum ENUM_ROAMING_REASON eRoamReason)
{
	int8_t cCandRssi = RCPI_TO_dBm(prCandBss->ucRCPI);
	int8_t cCurrRssi = RCPI_TO_dBm(prCurrBss->ucRCPI);
	enum ENUM_BAND eCurrBand = prCurrBss->eBand;
	enum ENUM_BAND eCandBand = prCandBss->eBand;

#if CFG_SUPPORT_NCHO
	if (prAdapter->rNchoInfo.fgNCHOEnabled)
		return cCurrRssi >= cCandRssi ? TRUE : FALSE;
#endif

	/* 1.3 Hard connecting RSSI check */
	if ((eCurrBand == BAND_5G && cCurrRssi < MINIMUM_RSSI_5G) ||
#if (CFG_SUPPORT_WIFI_6G == 1)
	   (eCurrBand == BAND_6G && cCurrRssi < MINIMUM_RSSI_6G) ||
#endif
	   (eCurrBand == BAND_2G4 && cCurrRssi < MINIMUM_RSSI_2G4))
		return FALSE;
	else if ((eCandBand == BAND_5G && cCandRssi < MINIMUM_RSSI_5G) ||
#if (CFG_SUPPORT_WIFI_6G == 1)
	   (eCandBand == BAND_6G && cCandRssi < MINIMUM_RSSI_6G) ||
#endif
	   (eCandBand == BAND_2G4 && cCandRssi < MINIMUM_RSSI_2G4))
		return TRUE;

	/* 1.4 prefer to select 5G Bss if Rssi of a 5G band BSS is good */
	if (eCandBand != eCurrBand) {
		switch (eCandBand) {
		case BAND_2G4:
			/* Current AP is 2.4G, replace candidate AP if target
			 * AP is good
			 */
			if (eCurrBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
			 || eCurrBand == BAND_6G
#endif
			) {
				if (cCurrRssi >= GOOD_RSSI_FOR_HT_VHT)
					return TRUE;

				if (cCurrRssi < LOW_RSSI_FOR_5G_BAND &&
				   (cCandRssi > cCurrRssi + RSSI_DIFF_BIG_STEP))
					return FALSE;

				if (cCandRssi - cCurrRssi >= RSSI_DIFF_BIG_STEP)
					return FALSE;

				if (cCurrRssi - cCandRssi >= RSSI_DIFF_SML_STEP)
					return TRUE;
			}
			break;
		case BAND_5G:
			/* Candidate AP is 5G, don't replace it if it's
			 * good enough.
			 */
			if (eCurrBand == BAND_2G4) {
				if (cCandRssi >= GOOD_RSSI_FOR_HT_VHT)
					return FALSE;

				if (cCandRssi < LOW_RSSI_FOR_5G_BAND &&
				   (cCurrRssi > cCandRssi + RSSI_DIFF_BIG_STEP))
					return TRUE;

				if (cCandRssi - cCurrRssi >= RSSI_DIFF_SML_STEP)
					return FALSE;

				if (cCurrRssi - cCandRssi >= RSSI_DIFF_BIG_STEP)
					return TRUE;
			}
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (eCurrBand == BAND_6G) {
				/* Target AP is 6G, replace candidate AP
				 * if target AP is good
				 */
				if (cCurrRssi >= GOOD_RSSI_FOR_HT_VHT)
					return TRUE;

				if (cCurrRssi < LOW_RSSI_FOR_5G_BAND &&
				   (cCandRssi > cCurrRssi + RSSI_DIFF_MED_STEP))
					return FALSE;

				if (cCandRssi - cCurrRssi >= RSSI_DIFF_MED_STEP)
					return FALSE;

				if (cCurrRssi - cCandRssi >= RSSI_DIFF_SML_STEP)
					return TRUE;
			}
#endif
			break;
#if (CFG_SUPPORT_WIFI_6G == 1)
		case BAND_6G:
			/* Candidate AP is 6G, don't replace it if
			 * it's good enough.
			 */
			if (eCurrBand == BAND_2G4) {
				if (cCandRssi >= GOOD_RSSI_FOR_HT_VHT)
					return FALSE;

				if (cCandRssi < LOW_RSSI_FOR_5G_BAND &&
				   (cCurrRssi > cCandRssi + RSSI_DIFF_BIG_STEP))
					return TRUE;

				if (cCandRssi - cCurrRssi >= RSSI_DIFF_SML_STEP)
					return FALSE;

				if (cCurrRssi - cCandRssi >= RSSI_DIFF_BIG_STEP)
					return TRUE;
			} else if (eCurrBand == BAND_5G) {
				if (cCandRssi >= GOOD_RSSI_FOR_HT_VHT)
					return FALSE;

				if (cCandRssi < LOW_RSSI_FOR_5G_BAND &&
				   (cCurrRssi > cCandRssi + RSSI_DIFF_MED_STEP))
					return TRUE;

				if (cCandRssi - cCurrRssi >= RSSI_DIFF_SML_STEP)
					return FALSE;

				if (cCurrRssi - cCandRssi >= RSSI_DIFF_MED_STEP)
					return TRUE;
			}
			break;
#endif
		default:
			break;
		}
	} else {
		if (cCandRssi - cCurrRssi >= RSSI_DIFF_MED_STEP)
			return FALSE;
		if (cCurrRssi - cCandRssi >= RSSI_DIFF_MED_STEP)
			return TRUE;
	}

	return FALSE;
}

static uint8_t apsNeedReplaceByRssi(struct ADAPTER *ad,
	struct AP_COLLECTION *cand, struct AP_COLLECTION *curr,
	enum ENUM_ROAMING_REASON reason)
{
	uint8_t i, j;

	for (i = 0; i < curr->ucLinkNum; i++) {
		uint8_t better_rssi = TRUE;

		if (!curr->aprTarget[i])
			continue;

		for (j = 0; j < cand->ucLinkNum; j++) {
			if (!cand->aprTarget[j])
				continue;

			if (!apsNeedReplaceCandidateByRssi(ad,
				cand->aprTarget[j],
				curr->aprTarget[i],
				reason)) {
				better_rssi = FALSE;
				break;
			}
		}

		if (better_rssi)
			return TRUE;
	}

	return FALSE;
}

enum ENUM_APS_REPLACE_REASON apsInterNeedReplace(struct ADAPTER *ad,
	enum ENUM_PARAM_CONNECTION_POLICY policy,
	struct AP_COLLECTION *cand, struct AP_COLLECTION *curr,
	uint32_t cand_score, uint32_t curr_score,
	enum ENUM_ROAMING_REASON reason, uint8_t bidx)
{
	if (policy == CONNECT_BY_BSSID) {
		if (curr->fgIsMatchBssid)
			return APS_MATCH_BSSID;
		else
			return APS_UNMATCH_BSSID;
	} else if (policy == CONNECT_BY_BSSID_HINT) {
		if (curr->fgIsMatchBssidHint)
			return APS_MATCH_BSSID_HINT;
		if (cand && cand->fgIsMatchBssidHint)
			return APS_UNMATCH_BSSID_HINT;
	}

	if (!cand)
		return APS_FIRST_CANDIDATE;

	if (reason == ROAMING_REASON_POOR_RCPI ||
	    reason == ROAMING_REASON_INACTIVE) {
		if (apsNeedReplaceByRssi(ad, cand, curr, reason))
			return APS_BETTER_RSSI;
		if (apsNeedReplaceByRssi(ad, curr, cand, reason))
			return APS_WORSE_RSSI;
	}

	return curr_score > cand_score ? APS_HIGH_SCORE : APS_LOW_SCORE;
}

struct BSS_DESC *apsFillBssDescSet(struct ADAPTER *ad,
	struct AP_COLLECTION *ap, struct BSS_DESC_SET *set, uint8_t bidx)
{
	struct CONNECTION_SETTINGS *conn = aisGetConnSettings(ad, bidx);
	enum ENUM_PARAM_CONNECTION_POLICY policy = conn->eConnectionPolicy;
	uint8_t i;

	if (!set)
		return ap ? ap->aprTarget[0] : NULL;

	kalMemSet(set, 0, sizeof(*set));
	if (!ap)
		goto done;

	for (i = 0; i < ap->ucLinkNum && set->ucLinkNum < MLD_LINK_MAX; i++) {
		if (!ap->aprTarget[i])
			continue;

		set->aprBssDesc[set->ucLinkNum++] = ap->aprTarget[i];
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	/* pick by bssid or bssid by upper layer */
	for (i = 1; i < set->ucLinkNum; i++) {
		uint8_t *found = NULL;
		struct BSS_DESC *bss;

		bss = set->aprBssDesc[i];
		if (policy == CONNECT_BY_BSSID &&
		     EQUAL_MAC_ADDR(bss->aucBSSID, conn->aucBSSID)) {
			set->aprBssDesc[i] = set->aprBssDesc[0];
			set->aprBssDesc[0] = bss;
			found = "bssid";
		} else if (IS_FEATURE_ENABLED(ad->rWifiVar.ucStaPreferMldAddr)
			   && EQUAL_MAC_ADDR(bss->aucBSSID, ap->aucAddr)) {
			set->aprBssDesc[i] = set->aprBssDesc[0];
			set->aprBssDesc[0] = bss;
			found = "mld_addr";
		} else if (bss->rMlInfo.ucLinkIndex ==
			   ad->rWifiVar.ucStaMldMainLinkIdx) {
			set->aprBssDesc[i] = set->aprBssDesc[0];
			set->aprBssDesc[0] = bss;
			found = "link_id";
		} else if (policy == CONNECT_BY_BSSID_HINT &&
			   EQUAL_MAC_ADDR(bss->aucBSSID, conn->aucBSSIDHint)) {
			set->aprBssDesc[i] = set->aprBssDesc[0];
			set->aprBssDesc[0] = bss;
			found = "bssid_hint";
		}
		if (found != NULL) {
			DBGLOG(APS, INFO, MACSTR
				" link_id=%d max_links=%d Setup for %s\n",
				MAC2STR(bss->aucBSSID),
				bss->rMlInfo.ucLinkIndex,
				found);
			goto done;
		}
	}
#endif

done:
	/* first bss desc is main bss */
	set->prMainBssDesc = set->aprBssDesc[0];
	if (ap) {
		set->fgIsMatchBssid = ap->fgIsMatchBssid;
		set->fgIsMatchBssidHint = ap->fgIsMatchBssidHint;
		set->fgIsAllLinkInBlackList = ap->fgIsAllLinkInBlackList;
		set->fgIsAllLinkConnected = ap->fgIsAllLinkConnected;
	}
	DBGLOG(APS, INFO, "Total %d link(s)\n", set->ucLinkNum);
	return set->prMainBssDesc;
}

struct BSS_DESC *apsInterApSelection(struct ADAPTER *ad,
	struct BSS_DESC_SET *set,
	enum ENUM_ROAMING_REASON reason, uint8_t bidx,
	struct AP_COLLECTION *current_ap)
{
	struct CONNECTION_SETTINGS *conn = aisGetConnSettings(ad, bidx);
	enum ENUM_PARAM_CONNECTION_POLICY policy = conn->eConnectionPolicy;
	struct AIS_SPECIFIC_BSS_INFO *s = aisGetAisSpecBssInfo(ad, bidx);
	struct LINK *ess = &s->rCurEssLink;
	struct AP_COLLECTION *ap, *cand = NULL;
	uint32_t best = 0, score = 0;
	uint8_t try_blockList = FALSE;
	enum ENUM_APS_REPLACE_REASON replace_reason;

try_again:
	LINK_FOR_EACH_ENTRY(ap, ess, rLinkEntry, struct AP_COLLECTION) {
		if (ap->ucLinkNum == 0)
			continue;

		if (!try_blockList && ap->fgIsAllLinkInBlackList)
			continue;

		score = apsCalculateApScore(ad, ap, reason, bidx);
		replace_reason = apsInterNeedReplace(ad, policy,
			cand, ap, best, score, reason, bidx);
		if (replace_reason >= APS_NEED_REPLACE) {
			best = score;
			cand = ap;
		}

		DBGLOG(APS, INFO,
			"%s APC[" MACSTR
			"] Total(%d) Score: %d (%s)",
			replace_reason >= APS_FIRST_CANDIDATE ? "--->" : "<---",
			MAC2STR(ap->aucAddr), ap->ucLinkNum, score,
			replace_reason < APS_REPLACE_REASON_NUM ?
			apucReplaceReasonStr[replace_reason] :
			(const uint8_t *)"UNKNOWN");

		/* early leave for specific reasons */
		if (replace_reason == APS_MATCH_BSSID ||
		    replace_reason == APS_MATCH_BSSID_HINT)
			goto done;
	}

	if (!try_blockList && (!cand || cand->fgIsAllLinkConnected)) {
		try_blockList = TRUE;
		DBGLOG(APS, INFO, "No ap collection found, try blacklist\n");
		goto try_again;
	}

done:
	return apsFillBssDescSet(ad, cand, set, bidx);
}

struct BSS_DESC *apsSearchBssDescByScore(struct ADAPTER *ad,
	enum ENUM_ROAMING_REASON reason,
	uint8_t bidx, struct BSS_DESC_SET *set)
{
	struct AIS_SPECIFIC_BSS_INFO *s = aisGetAisSpecBssInfo(ad, bidx);
	struct LINK *ess = &s->rCurEssLink;
	struct CONNECTION_SETTINGS *conn = aisGetConnSettings(ad, bidx);
	struct BSS_DESC *cand = NULL;
	struct AP_COLLECTION *current_ap = NULL;
	uint16_t count = 0;

	if (reason >= ROAMING_REASON_NUM) {
		DBGLOG(APS, ERROR, "reason %d!\n", reason);
		return NULL;
	}

	DBGLOG(APS, INFO, "ConnectionPolicy = %d, reason = %d\n",
		conn->eConnectionPolicy, reason);

	aisRemoveTimeoutBlacklist(ad);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	aisRemoveTimeoutMldBlocklist(ad);
#endif

	count = apsUpdateEssApList(ad, bidx);

#if CFG_SUPPORT_802_11K
	/* check before using neighbor report */
	aisCheckNeighborApValidity(ad, bidx);
#endif

	current_ap = apsIntraApSelection(ad, reason, bidx);
	cand = apsInterApSelection(ad, set, reason, bidx, current_ap);
	if (cand) {
		DBGLOG(APS, INFO,
			"Selected " MACSTR ", RSSI[%d] Band[%s] when find %s, "
			MACSTR " policy=%d in %d(%d) BSSes.\n",
			MAC2STR(cand->aucBSSID),
			RCPI_TO_dBm(cand->ucRCPI),
			apucBandStr[cand->eBand], HIDE(conn->aucSSID),
			conn->eConnectionPolicy == CONNECT_BY_BSSID ?
			MAC2STR(conn->aucBSSID) :
			MAC2STR(conn->aucBSSIDHint),
			conn->eConnectionPolicy,
			count,
			ess->u4NumElem);
		goto done;
	}

	DBGLOG(APS, INFO, "Selected None when find %s, " MACSTR
		" in %d(%d) BSSes.\n",
		conn->aucSSID, MAC2STR(conn->aucBSSID),
		count,
		ess->u4NumElem);
done:
	apsResetEssApList(ad, bidx);
	return cand;
}

