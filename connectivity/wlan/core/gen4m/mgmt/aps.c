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

#define ROAM_SCORE_DELTA                        5
#define BSS_MATCH_BSSID_SCORE			(30000)
#define BSS_MATCH_BSSID_HINT_SCORE		(20000)

#define APS_AMSDU_HT_3K                         (3839)
#define APS_AMSDU_HT_8K                         (7935)
#define APS_AMSDU_VHT_HE_3K                     (3895)
#define APS_AMSDU_VHT_HE_8K                     (7991)
#define APS_AMSDU_VHT_HE_11K                    (11454)

#define WEIGHT_GBAND_COEX_DOWNGRADE		70 /* 0~100 */
#define CU_6G_INDEX_OFFSET			256
#define WEIGHT_MCC_DOWNGRADE			70 /* 0~100 */

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
	uint8_t ucBlockListWeight;
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
	APS_LAST_DEAUTH,

	/* reason to replace */
	APS_FIRST_CANDIDATE,
	APS_HIGH_SCORE,
	APS_MATCH_BSSID,
	APS_MATCH_BSSID_HINT,
	APS_BETTER_RSSI,
	APS_NOT_LAST_DEAUTH,

	/* don't add after this */
	APS_REPLACE_REASON_NUM,
	APS_NEED_REPLACE = APS_FIRST_CANDIDATE,
};

/* mapping with ENUM_APS_REPLACE_REASON */
static const char * const apucReplaceReasonStr[APS_REPLACE_REASON_NUM] = {
	"LOW SCORE",
	"UNMATCH BSSID",
	"UNMATCH BSSID_HINT",
	"WORSE RSSI",
	"LAST DEAUTH",
	"FIRST CANDIDATE",
	"HIGH SCORE",
	"MATCH BSSID",
	"MATCH BSSID_HINT",
	"BETTER RSSI",
	"NOT LAST DEAUTH",
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

static uint8_t aucBaSizeTranslate[8] = {
	[0] = 0,
	[1] = 2,
	[2] = 4,
	[3] = 6,
	[4] = 8,
	[5] = 16,
	[6] = 32,
	[7] = 64
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

#define PERCENTAGE(_val, _base) (_val * 100 / _base)

enum ENUM_BAND g_aeLinkPlan[MLO_LINK_PLAN_NUM][APS_LINK_MAX] = {
	{BAND_2G4, BAND_NULL, BAND_NULL},
	{BAND_5G, BAND_NULL, BAND_NULL},
	{BAND_2G4, BAND_5G, BAND_NULL},
	{BAND_5G, BAND_5G, BAND_NULL},
	{BAND_2G4, BAND_5G, BAND_5G},
#if (CFG_SUPPORT_WIFI_6G == 1)
	{BAND_6G, BAND_NULL, BAND_NULL},
	{BAND_2G4, BAND_6G, BAND_NULL},
	{BAND_5G, BAND_6G, BAND_NULL},
	{BAND_6G, BAND_6G, BAND_NULL},
	{BAND_2G4, BAND_5G, BAND_6G},
	{BAND_2G4, BAND_6G, BAND_6G},
#endif
};

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
	(prBssDesc->prBlock && prBssDesc->prBlock->fgDeauthLastTime ? 0 : \
	BSS_FULL_SCORE))

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static uint8_t apsSanityCheckBssDesc(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex);

static uint8_t apsIsValidBssDesc(struct ADAPTER *ad, struct BSS_DESC *bss,
	enum ENUM_ROAMING_REASON reason, uint8_t bidx);

static uint32_t apsGetEstimatedTput(struct ADAPTER *ad, struct BSS_DESC *bss,
	uint8_t bidx);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

struct AP_COLLECTION *apsHashGet(struct ADAPTER *ad,
	uint8_t *addr, uint8_t bidx)
{
	struct AIS_SPECIFIC_BSS_INFO *s = aisGetAisSpecBssInfo(ad, bidx);
	struct AP_COLLECTION *a = NULL;

	a = s->arApHash[AP_HASH(addr)];

	while (a != NULL &&
	       UNEQUAL_MAC_ADDR(a->aucAddr, addr))
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

	if (EQUAL_MAC_ADDR(a->aucAddr, ap->aucAddr)) {
		s->arApHash[AP_HASH(ap->aucAddr)] = a->hnext;
		return;
	}

	while (a->hnext != NULL &&
	       UNEQUAL_MAC_ADDR(a->hnext->aucAddr, ap->aucAddr)) {
		a = a->hnext;
	}
	if (a->hnext != NULL)
		a->hnext = a->hnext->hnext;
	else
		DBGLOG(APS, INFO, "Could not remove AP " MACSTR
			   " from hash table\n", MAC2STR(ap->aucAddr));
}

uint8_t apsCanFormMultiLink(struct ADAPTER *ad,
	struct BSS_DESC *bss, uint8_t bidx)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (!mldIsMultiLinkEnabled(ad, NETWORK_TYPE_AIS, bidx) ||
	    !aisSecondLinkAvailable(ad, bidx))
		return FALSE;

	if (!bss->rMlInfo.fgValid)
		return FALSE;

	bss->rMlInfo.prBlock = aisQueryMldBlockList(ad, bss);

	if (!bss->rMlInfo.prBlock ||
	    bss->rMlInfo.prBlock->ucCount < ad->rWifiVar.ucMldRetryCount)
		return TRUE;
#endif
	return FALSE;
}

uint8_t apsBssDescToLink(struct ADAPTER *ad,
	struct AP_COLLECTION *ap, struct BSS_DESC *bss, uint8_t bidx)
{
	return bss->eBand;
}

uint32_t apsAddBssDescToList(struct ADAPTER *ad, struct AP_COLLECTION *ap,
			struct BSS_DESC *bss, uint8_t bidx)
{
	uint8_t aidx = AIS_INDEX(ad, bidx);
	uint8_t l = apsBssDescToLink(ad, ap, bss, bidx);

	if (l >= BAND_NUM)
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

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (apsCanFormMultiLink(ad, bss, bidx))
		COPY_MAC_ADDR(ap->aucAddr, bss->rMlInfo.aucMldAddr);

	if (bss->rMlInfo.prBlock &&
	    bss->rMlInfo.prBlock->ucCount >= ad->rWifiVar.ucMldRetryCount)
		DBGLOG(APS, WARN, "Mld[" MACSTR
			"] is in mld blocklist, retry count %d >= %d\n",
			MAC2STR(bss->rMlInfo.aucMldAddr),
			bss->rMlInfo.prBlock->ucCount,
			ad->rWifiVar.ucMldRetryCount);
#endif

	DBGLOG(APS, TRACE, "Add APC[" MACSTR "]\n", MAC2STR(ap->aucAddr));

	for (i = 0; i < BAND_NUM; i++)
		LINK_INITIALIZE(&ap->arLinks[i]);

	if (apsAddBssDescToList(ad, ap, bss, bidx)) {
		kalMemFree(ap, VIR_MEM_TYPE, sizeof(*ap));
		return NULL;
	}

	LINK_INSERT_TAIL(ess, &ap->rLinkEntry);
	apsHashAdd(ad, ap, bidx);

	ap->u4Index = ess->u4NumElem - 1;

	return ap;
}

struct AP_COLLECTION *apsGetAp(struct ADAPTER *ad,
	struct BSS_DESC *bss, uint8_t bidx)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (apsCanFormMultiLink(ad, bss, bidx))
		return apsHashGet(ad, bss->rMlInfo.aucMldAddr, bidx);
#endif

	return apsHashGet(ad, bss->aucBSSID, bidx);
}

void apsRemoveAp(struct ADAPTER *ad, struct AP_COLLECTION *ap, uint8_t bidx)
{
	DBGLOG(APS, TRACE,
		"Remove APC[" MACSTR "] LinkNum=%d, TotalCount=%d\n",
		MAC2STR(ap->aucAddr), ap->ucLinkNum, ap->ucTotalCount);

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

#if (CFG_EXT_ROAMING == 0) /* Common */
uint8_t apsIsBssQualify(struct ADAPTER *ad, struct BSS_DESC *bss,
	enum ENUM_ROAMING_REASON eRoamReason, uint32_t u4ConnectedApScore,
	uint32_t u4CandidateApScore, uint8_t bidx)
{
	uint16_t delta = 0;

	switch (eRoamReason) {
	case ROAMING_REASON_POOR_RCPI:
	case ROAMING_REASON_INACTIVE:
	case ROAMING_REASON_RETRY:
	{
		delta += ROAM_SCORE_DELTA;

		/* Minimum Roam Delta
		 * Absolute score value comparing to current AP
		 */
		if (u4CandidateApScore <=
		    u4ConnectedApScore * (100 + delta) / 100) {
			DBGLOG(APS, TRACE, "BSS[" MACSTR
				"] (%d <= %d*%d%%) reason=%d\n",
				MAC2STR(bss->aucBSSID),
				u4CandidateApScore, u4ConnectedApScore,
				100 + delta, eRoamReason);
			return FALSE;
		}

		break;
	}
	case ROAMING_REASON_BEACON_TIMEOUT:
	{
		/* DON'T compare score if roam with BTO */
		break;
	}
	case ROAMING_REASON_SAA_FAIL:
	{
		/* DON'T compare score if roam with emergency */
		break;
	}
	default:
	{
		if (u4CandidateApScore < u4ConnectedApScore) {
			DBGLOG(APS, TRACE, "BSS[" MACSTR
				"] (%d < %d) reason=%d\n",
				MAC2STR(bss->aucBSSID),
				u4CandidateApScore, u4ConnectedApScore,
				eRoamReason);
			return FALSE;
		}
		break;
	}
	}
	return TRUE;
}
#endif

uint16_t apsGetAmsduByte(struct ADAPTER *ad,
	struct BSS_DESC *bss, uint8_t bidx)
{
	uint16_t bssAmsduLen = 0, amsduLen = 0;
	struct WIFI_VAR *prWifiVar = &ad->rWifiVar;

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
	if (bss->fgIsEHTPresent == TRUE &&
	    (prWifiVar->fgDisSecurityCheck ||
	     rsnIsKeyMgmtForEht(ad, bss, bidx))) {
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

void apsRecordCuInfo(struct ADAPTER *ad, struct BSS_DESC *bss,
	uint8_t bidx)
{
	struct APS_INFO *aps = aisGetApsInfo(ad, bidx);
	uint16_t u2CuOffset = 0;

	if (bss->eBand == BAND_2G4 || bss->eBand == BAND_5G)
		u2CuOffset = bss->ucChannelNum;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (bss->eBand == BAND_6G)
		u2CuOffset = CU_6G_INDEX_OFFSET + bss->ucChannelNum;
#endif

	aps->arCuInfo[u2CuOffset].eBand = bss->eBand;
	aps->arCuInfo[u2CuOffset].ucTotalCount++;
	aps->arCuInfo[u2CuOffset].ucTotalCu += bss->ucChnlUtilization;
}

struct RF_CHANNEL_INFO apsGetRfChannelInfo(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, struct BSS_INFO *prBssInfo)
{
	struct RF_CHANNEL_INFO rRfChnlInfo = {0};

	if (!prBssDesc && !prBssInfo)
		return rRfChnlInfo;

	if (prBssDesc) {
		rRfChnlInfo.eBand = prBssDesc->eBand;
		rRfChnlInfo.ucChannelNum = prBssDesc->ucChannelNum;
		rRfChnlInfo.ucChnlBw = rlmGetBssOpBwByChannelWidth(
						prBssDesc->eSco,
						prBssDesc->eChannelWidth);
	} else {
		rRfChnlInfo.eBand = prBssInfo->eBand;
		rRfChnlInfo.ucChannelNum = prBssInfo->ucPrimaryChannel;
		rRfChnlInfo.ucChnlBw = rlmGetBssOpBwByChannelWidth(
						prBssInfo->eBssSCO,
						prBssInfo->ucVhtChannelWidth);
	}

	rRfChnlInfo.u4CenterFreq1 = nicGetS1Freq(prAdapter,
						 rRfChnlInfo.eBand,
						 rRfChnlInfo.ucChannelNum,
						 rRfChnlInfo.ucChnlBw);

	return rRfChnlInfo;
}

void apsCheckConcurrent(struct ADAPTER *ad, struct BSS_DESC *bss,
	struct BSS_DESC *hint, uint8_t bidx)
{
	struct BSS_INFO *bssinfo;
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(ad, bidx);
	uint32_t bmap = aisGetBssIndexBmap(aisGetAisFsmInfo(ad, bidx));
	struct CONNECTION_SETTINGS *conn = aisGetConnSettings(ad, bidx);
	uint8_t i;
#if CFG_SUPPORT_CCM
	struct RF_CHANNEL_INFO rExist, rCurr;

	rCurr = apsGetRfChannelInfo(ad, bss, NULL);
#endif

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
			/* prefer higher band scc */
			if (hint && bss->eBand >= hint->eBand &&
			    !ad->rWifiVar.fgDisForceSCC &&
			    ais->ucReasonOfDisconnect !=
			    DISCONNECT_REASON_CODE_TEST_MODE)
				conn->eConnectionPolicy =
					CONNECT_BY_SSID_BEST_RSSI;
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
#if CFG_SUPPORT_CCM
			rExist = apsGetRfChannelInfo(ad, NULL, bssinfo);
			if (!ccmAAAvailableCheck(ad, &rCurr, &rExist)) {
				bss->fgIsMCC = TRUE;
				return;
			}
#else
			if (bss->eBand != bssinfo->eBand ||
			    bss->ucChannelNum != bssinfo->ucPrimaryChannel) {
				bss->fgIsMCC = TRUE;
				return;
			}
#endif
		}
	}

	bss->fgIsMCC = FALSE;
}

uint8_t apsGetCuInfo(struct ADAPTER *ad, struct BSS_DESC *bss, uint8_t bidx)
{
	struct APS_INFO *aps = aisGetApsInfo(ad, bidx);
	uint16_t u2CuOffset = 0;

	if (bss->eBand == BAND_2G4 || bss->eBand == BAND_5G)
		u2CuOffset = bss->ucChannelNum;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (bss->eBand == BAND_6G)
		u2CuOffset = CU_6G_INDEX_OFFSET + bss->ucChannelNum;
#endif

	return aps->arCuInfo[u2CuOffset].ucTotalCount == 0 ? 0 :
	       aps->arCuInfo[u2CuOffset].ucTotalCu /
	       aps->arCuInfo[u2CuOffset].ucTotalCount;
}

static uint32_t apsGetEstimatedTput(struct ADAPTER *ad, struct BSS_DESC *bss,
	uint8_t bidx)
{
	struct APS_INFO *aps = aisGetApsInfo(ad, bidx);
	uint8_t fgIsGBandCoex = aps->fgIsGBandCoex;
	uint8_t rcpi = 0, ppduDuration = 5, ucChannelCuInfo = 0;
	uint16_t amsduByte = apsGetAmsduByte(ad, bss, bidx);
	uint16_t baSize = mpduLen[bss->eChannelWidth];
	uint16_t slot = 0;
	uint32_t airTime = 0, idle = 0, ideal = 0, tput = 0, est = 0;
	int32_t a = 0, b = 0, delta = 5;
	uint8_t *pucIEs = NULL;

	if (aps->ucConsiderEsp) {
		pucIEs = (uint8_t *) &bss->u4EspInfo[ESP_AC_BE];
		baSize = aucBaSizeTranslate[(uint8_t)((pucIEs[0] & 0xE0) >> 5)];
		airTime = pucIEs[1];
		if (bss->fgExistEspOutIE)
			airTime += bss->ucEspOutInfo[ESP_AC_BE];
		else
			airTime += pucIEs[1];
		airTime = airTime >> 1;
		if (pucIEs[2])
			ppduDuration = pucIEs[2];
	} else {
		if (bss->fgExistBssLoadIE) {
			airTime = 255 - bss->ucChnlUtilization;
		} else {
			ucChannelCuInfo = apsGetCuInfo(ad, bss, bidx);
			if (ucChannelCuInfo) {
				airTime = 255 - ucChannelCuInfo;
			} else {
				slot = scanGetChnlIdleSlot(ad,
					bss->eBand, bss->ucChannelNum);

				/* 90000 ms = 90ms dwell time to micro sec */
				idle = (slot * 9 * 100) / (90000);
				airTime  = idle > 100 ? 100 : idle;
				/* Give a default value of air time */
				if (airTime == 0)
					airTime = 45;

				/* nomalized to 0~255 */
				airTime = airTime * 255 / 100;
			}

#if (CFG_EXT_ROAMING == 1)
			/* Cannot find any CU info in the same channel */
			if (ucChannelCuInfo == 0) {
				/* Apply default CU(50%) */
				ucChannelCuInfo = 128;
			}
#endif
		}
	}

	/* Unit: mbps */
	ideal = baSize * amsduByte * 8 / ppduDuration;
	rcpi = bss->ucRCPI;
	/* Consider the TxPwr only when the RCPI is sufficiently good */
	if (bss->fgExistTxPwr && bss->cTransmitPwr < 0 && rcpi > 100)
		rcpi = rcpi -
			(bss->cTransmitPwr > -10 ? bss->cTransmitPwr : -10);

	rcpi = rcpi > 220 ? 220 : rcpi;
	/* Adjust RCPI based on simultaneous equation */
	if (rcpi > 100) {
		/* RCPI from 220 to 100, peak(1) to breakpoint(1 - delta/100)
		 * y = ax + b, through 2 points (220, 1) (100, 1 - delta /100)
		 */
		a = (delta * 60000 / 100) / 120;
		b = 60000 - ((delta * 60000 / 100) * (220 / 120));
	} else if (rcpi >= 50) {
		/* RCPI from 100 to 50, breakpoint(1 - delta /100) to zero(0)
		 * y = ax + b, through 2 points (100, 1 - delta /100) (50, 0)
		 */
		a = (60000 - (delta * 60000 / 100)) / 50;
		b = (delta * 60000 / 100) - 60000;
	} else {
		/* RCPI less than 50, estimated tput will be zero */
		a = 0;
		b = 0;
	}
	tput = (uint32_t)((uint64_t)ideal * (uint64_t)(a * rcpi + b) / 60000);
	est = PERCENTAGE(airTime, 255) * tput / 100;

	if (aps->ucConsiderEsp) {
		if (bss->fgIsRWMValid && bss->u2ReducedWanMetrics < est)
			est = bss->u2ReducedWanMetrics;
	}

#if (CFG_EXT_ROAMING == 1)
	if (fgIsGBandCoex && bss->eBand == BAND_2G4)
		est = (est * ad->rWifiVar.ucRBTCETPW / 100);
#else
	if (fgIsGBandCoex && bss->eBand == BAND_2G4)
		est = (est * WEIGHT_GBAND_COEX_DOWNGRADE / 100);
#endif

	if (bss->fgIsMCC)
		est = (est * WEIGHT_MCC_DOWNGRADE / 100);

	DBGLOG(APS, TRACE, "BSS["MACSTR
		"] EST:%d ideal[%d] ba[%d] amsdu[%d] a[%d] b[%d] rcpi[%d] tput[%d] airTime[%d] slot[%d] coex[%d] MCC[%d] TxPwr[%d]\n",
		MAC2STR(bss->aucBSSID), est, ideal, baSize, amsduByte,
		a, b, rcpi, tput, airTime, slot,
		fgIsGBandCoex, bss->fgIsMCC, bss->cTransmitPwr);

	return est;
}

uint16_t apsUpdateEssApList(struct ADAPTER *ad,
	enum ENUM_ROAMING_REASON reason, uint8_t bidx)
{
	struct APS_INFO *aps = aisGetApsInfo(ad, bidx);
	struct AP_COLLECTION *ap;
	struct BSS_DESC *bss = NULL, *hint = NULL;
	struct LINK *scan_result = &ad->rWifiVar.rScanInfo.rBSSDescList;
	struct CONNECTION_SETTINGS *conn = aisGetConnSettings(ad, bidx);
	uint16_t count = 0;

	kalMemZero(aps->arCuInfo, sizeof(aps->arCuInfo));
	aps->ucConsiderEsp = TRUE;

	if (conn->eConnectionPolicy == CONNECT_BY_BSSID_HINT) {
		struct PARAM_SSID ssid = {0};

		COPY_SSID(ssid.aucSsid, ssid.u4SsidLen,
			  conn->aucSSID, conn->ucSSIDLen);
		hint = scanSearchBssDescByBssidAndSsid(ad,
			conn->aucBSSIDHint, TRUE, &ssid);
	}

	LINK_FOR_EACH_ENTRY(bss, scan_result, rLinkEntry,
		struct BSS_DESC) {
		if (bss->ucChannelNum > 233)
			continue;
		if (!EQUAL_SSID(conn->aucSSID,
			conn->ucSSIDLen,
			bss->aucSSID, bss->ucSSIDLen) ||
			bss->eBSSType != BSS_TYPE_INFRASTRUCTURE)
			continue;

		bss->prBlock = aisQueryBlockList(ad, bss);
#if CFG_SUPPORT_802_11K
		/* update neighbor report entry */
		bss->prNeighbor = aisGetNeighborAPEntry(
			ad, bss, bidx);
#endif

		/* sanity check */
		if (!apsSanityCheckBssDesc(ad, bss, reason, bidx))
			continue;

		/* add bss to ap collection */
		ap = apsGetAp(ad, bss, bidx);
		if (ap) {
			if (!apsAddBssDescToList(ad, ap, bss, bidx))
				count++;
		} else {
			ap = apsAddAp(ad, bss, bidx);
			if (ap)
				count++;
		}

		/* handle cu/esp */
		if (bss->fgExistBssLoadIE)
			apsRecordCuInfo(ad, bss, bidx);
		if (!bss->fgExistEspIE)
			aps->ucConsiderEsp = FALSE;

		apsCheckConcurrent(ad, bss, hint, bidx);
	}

	DBGLOG(APS, INFO,
		"Find %s in %d BSSes, result %d, Using %s estimated tput, policy %d\n",
		conn->aucSSID, scan_result->u4NumElem, count,
		aps->ucConsiderEsp ? "ESP" : "LEGACY",
		conn->eConnectionPolicy);
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

#if (CFG_EXT_ROAMING == 0) /* Common part */
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
	default:
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
			prBssDesc->fgExistBssLoadIE, prBssDesc->u2StaCnt);

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

	DBGLOG(APS, TRACE,
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

uint16_t apsCalculateScoreByBlockList(struct ADAPTER *prAdapter,
	    struct BSS_DESC *prBssDesc, enum ROAM_TYPE eRoamType)
{
	uint16_t u2Score = 0;

	if (eRoamType >= ROAM_TYPE_NUM) {
		DBGLOG(APS, WARN, "Invalid roam type %d!\n", eRoamType);
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

	return u2Score * gasMtkWeightConfig[eRoamType].ucBlockListWeight;
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

#if CFG_SUPPORT_ROAMING
#if CFG_SUPPORT_802_11K
	if (prBssDesc->prNeighbor)
		return (prBssDesc->prNeighbor->ucPreference + 100) *
		       gasMtkWeightConfig[eRoamType].ucPreferenceWeight;
#endif
#endif
	return 100 * gasMtkWeightConfig[eRoamType].ucPreferenceWeight;
}

uint16_t apsCalculateApScore(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex)
{
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo = NULL;
	struct APS_INFO *aps = aisGetApsInfo(prAdapter, ucBssIndex);
	uint8_t fgIsGBandCoex = aps->fgIsGBandCoex;
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
	u2BlockListScore =
	       apsCalculateScoreByBlockList(prAdapter, prBssDesc, eRoamType);
	u2PreferenceScore =
	      apsCalculateScoreByPreference(prAdapter, prBssDesc, eRoamReason);

	u2TputScore = apsCalculateScoreByTput(prAdapter, prBssDesc, eRoamType);

	u2ScoreTotal = u2ScoreBandwidth + u2ScoreChnlInfo +
		u2ScoreDeauth + u2ScoreSnrRssi + u2ScoreStaCnt + u2ScoreSTBC +
		u2ScoreBand + u2BlockListScore + u2ScoreSaa +
		u2ScoreIdleTime + u2TputScore;

	/* Adjust 2.4G AP's score if BT coex */
	if (prBssDesc->eBand == BAND_2G4 && fgIsGBandCoex)
		u2ScoreTotal = u2ScoreTotal * WEIGHT_GBAND_COEX_DOWNGRADE / 100;

	if (prBssDesc->fgIsMCC)
		u2ScoreTotal = (u2ScoreTotal * WEIGHT_MCC_DOWNGRADE / 100);

#if (CFG_SUPPORT_AVOID_DESENSE == 1)
	if (fgBssInDenseRange)
		u2ScoreTotal /= 4;
	kalSnprintf(extra, sizeof(extra), ", DESENSE[%d]", fgBssInDenseRange);
#endif

#define TEMP_LOG_TEMPLATE\
		"BSS["MACSTR"] Score:%d Band[%s],cRSSI[%d],GBandCoex[%d]"\
		",MCC[%d],DE[%d],RSSI[%d],BD[%d],BL[%d],SAA[%d]"\
		",BW[%d],SC[%d],ST[%d],CI[%d],IT[%d],CU[%d,%d],PF[%d]"\
		",TPUT[%d]%s\n"

	DBGLOG(APS, TRACE,
		TEMP_LOG_TEMPLATE,
		MAC2STR(prBssDesc->aucBSSID),
		u2ScoreTotal, apucBandStr[prBssDesc->eBand],
		cRssi, fgIsGBandCoex, prBssDesc->fgIsMCC, u2ScoreDeauth,
		u2ScoreSnrRssi, u2ScoreBand, u2BlockListScore,
		u2ScoreSaa, u2ScoreBandwidth, u2ScoreStaCnt,
		u2ScoreSTBC, u2ScoreChnlInfo, u2ScoreIdleTime,
		prBssDesc->fgExistBssLoadIE,
		prBssDesc->ucChnlUtilization,
		u2PreferenceScore,
		u2TputScore, extra);

#undef TEMP_LOG_TEMPLATE

	return u2ScoreTotal;
}
#endif /* CFG_EXT_ROAMING  */

uint8_t apsSanityCheckBssDesc(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex)
{
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	struct BSS_INFO *prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct CONNECTION_SETTINGS *conn =
				aisGetConnSettings(prAdapter, ucBssIndex);
#endif
	struct APS_INFO *prApsInfo = aisGetApsInfo(prAdapter, ucBssIndex);
#if CFG_SUPPORT_MBO
	struct PARAM_BSS_DISALLOWED_LIST *disallow;
	uint32_t i = 0;
#endif

	if (ais == NULL) {
		DBGLOG(APS, WARN, "ais is NULL\n");
		return FALSE;
	}

	if (aisQueryCusBlocklist(prAdapter, ucBssIndex, prBssDesc))
		return FALSE;

#if CFG_SUPPORT_MBO
	disallow = &prAdapter->rWifiVar.rBssDisallowedList;
	for (i = 0; i < disallow->u4NumBssDisallowed; ++i) {
		uint32_t index = i * MAC_ADDR_LEN;

		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
				&disallow->aucList[index])) {
			DBGLOG(APS, WARN, MACSTR" disallowed list\n",
				MAC2STR(prBssDesc->aucBSSID));
#if (CFG_SUPPORT_CONN_LOG == 1)
			connLogDisallowedList(prAdapter,
				ucBssIndex,
				prBssDesc);
#endif
			return FALSE;
		}
	}

	if (prBssDesc->fgIsDisallowed) {
		DBGLOG(APS, WARN, MACSTR" disallowed\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	if (prBssDesc->prBlock && prBssDesc->prBlock->fgDisallowed &&
	    !(prBssDesc->prBlock->i4RssiThreshold > 0 &&
	      RCPI_TO_dBm(prBssDesc->ucRCPI) >
			prBssDesc->prBlock->i4RssiThreshold)) {
		DBGLOG(APS, WARN, MACSTR" disallowed delay, rssi %d(%d)\n",
			MAC2STR(prBssDesc->aucBSSID),
			RCPI_TO_dBm(prBssDesc->ucRCPI),
			prBssDesc->prBlock->i4RssiThreshold);
		return FALSE;
	}

	if (prBssDesc->prBlock && prBssDesc->prBlock->fgDisallowed) {
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

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (prBssDesc->rMlInfo.fgValid &&
		!(BIT(prBssDesc->rMlInfo.ucLinkIndex) & conn->u2LinkIdBitmap)) {
		DBGLOG(APS, WARN, MACSTR" LinkID[%d] is not allowed [%d]\n",
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->rMlInfo.ucLinkIndex,
			conn->u2LinkIdBitmap);
		return FALSE;
	}
#endif

	if (prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE) {
		DBGLOG(APS, WARN, MACSTR" is not infrastructure\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}

	if (prBssDesc->prBlock) {
		if (prBssDesc->prBlock->fgIsInFWKBlocklist) {
			DBGLOG(APS, WARN, MACSTR" in FWK blocklist\n",
				MAC2STR(prBssDesc->aucBSSID));
#if (CFG_SUPPORT_CONN_LOG == 1)
			connLogBlockList(prAdapter,
				ucBssIndex,
				prBssDesc);
#endif
			return FALSE;
		}

		if (prBssDesc->prBlock->fgDeauthLastTime &&
		    (apsCanFormMultiLink(prAdapter, prBssDesc, ucBssIndex) ||
		     prApsInfo->u2EssApNum <= 1 ||
		     prBssDesc->prBlock->ucDeauthCount >= 2)) {
			DBGLOG(APS, WARN, MACSTR " is sending deauth [%d].\n",
				MAC2STR(prBssDesc->aucBSSID),
				prBssDesc->prBlock->ucDeauthCount);
			return FALSE;
		}

		if (prBssDesc->prBlock->ucDeauthCount >= AIS_DEAUTH_THRESHOLD) {
			DBGLOG(APS, WARN,
				MACSTR " is sending deauth multiple times.\n",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}

		if (prBssDesc->prBlock->fgArpNoResponse &&
		    prBssDesc->prBlock->ucCount >= AIS_ARP_NO_RSP_THRESHOLD) {
			DBGLOG(APS, WARN,
				MACSTR " is arp no response many times.\n",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}

		if (prBssDesc->prBlock->ucCount >= 10)  {
			DBGLOG(APS, WARN,
				MACSTR
				" Skip AP that add toblocklist count >= 10\n",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}
	}

	/* roaming case */
	if ((prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED ||
	    aisFsmIsInProcessPostpone(prAdapter, ucBssIndex))) {
#if (CFG_EXT_ROAMING == 1)
		int32_t r1, r2;
		struct BSS_DESC *target = NULL;
#endif

		if (prBssDesc->fgDriverGen)
			goto skip_rcpi_check;

#if (CFG_EXT_ROAMING == 1)
		target = aisGetTargetBssDesc(prAdapter, ucBssIndex);
		r1 = RCPI_TO_dBm(target ? target->ucRCPI : RCPI_LOW_BOUND);
		r2 = RCPI_TO_dBm(prBssDesc->ucRCPI);
		switch (eRoamReason) {
		case ROAMING_REASON_BEACON_TIMEOUT:
		case ROAMING_REASON_SAA_FAIL:
		{
			if (r2 < prAdapter->rWifiVar.cRBMinRssi) {
				DBGLOG(APS, WARN, MACSTR " low rssi %d < %d\n",
					MAC2STR(prBssDesc->aucBSSID),
					r2, prAdapter->rWifiVar.cRBMinRssi);
				return FALSE;
			}
			break;
		}
		case ROAMING_REASON_BT_COEX:
		{
			if (r2 < prAdapter->rWifiVar.cRBTCRssi) {
				log_dbg(SCN, INFO,
					MACSTR " BTCoex low rssi %d < %d\n",
					MAC2STR(prBssDesc->aucBSSID),
					r2, prAdapter->rWifiVar.ucRBTCDelta);
				return FALSE;
			}
			break;
		}
		case ROAMING_REASON_INACTIVE_TIMER:
		case ROAMING_REASON_SCAN_TIMER:
		case ROAMING_REASON_POOR_RCPI:
		case ROAMING_REASON_RETRY:
		{
			if (prAdapter->rNchoInfo.fgNCHOEnabled &&
			    r2 - r1 <= prAdapter->rNchoInfo.i4RoamDelta) {
				DBGLOG(APS, WARN,
					MACSTR " low rssi %d - %d <= %d\n",
					MAC2STR(prBssDesc->aucBSSID), r2, r1,
					prAdapter->rNchoInfo.i4RoamDelta);
				return FALSE;
			}
			break;
		}
		default:
			break;
		}
#else
		if (eRoamReason == ROAMING_REASON_BTM &&
		    RCPI_TO_dBm(prBssDesc->ucRCPI) <
				prAdapter->rWifiVar.iBTMMinRssi) {
			DBGLOG(APS, INFO, MACSTR " low rssi %d for BTM\n",
				MAC2STR(prBssDesc->aucBSSID),
				RCPI_TO_dBm(prBssDesc->ucRCPI));
			return FALSE;
		} else if (prBssDesc->ucRCPI < RCPI_FOR_DONT_ROAM) {
			DBGLOG(APS, INFO, MACSTR " low rssi %d\n",
				MAC2STR(prBssDesc->aucBSSID),
				RCPI_TO_dBm(prBssDesc->ucRCPI));
			return FALSE;
		}
#endif
	}

skip_rcpi_check:
	/* Restrict STAs other than wlan0 */
	if (ais->ucAisIndex != AIS_DEFAULT_INDEX) {
		struct AIS_FSM_INFO *tempAis;
		struct BSS_DESC *tempBssDesc;
		uint32_t bmap;
		uint8_t i, j;
		uint8_t ucExistALinks = 0, ucExistGLinks = 0;
		struct RF_CHANNEL_INFO rCurrRfChnlInfo, rExistRfChnlInfo;

		bmap = aisGetBssIndexBmap(ais);

		/* Disallow to pick a bss that is connecting */
		if (prBssDesc->fgIsConnecting) {
			DBGLOG(APS, INFO,
				MACSTR " is connecting by other WLAN",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}

		/* Disallow to pick a bss that already connected */
		if (prBssDesc->fgIsConnected & ~bmap) {
			DBGLOG(APS, INFO,
				MACSTR " already connected by wlan0",
				MAC2STR(prBssDesc->aucBSSID));
			return FALSE;
		}

		for (i = 0; i < KAL_AIS_NUM; i++) {
			if (i == ais->ucAisIndex)
				continue;

			tempAis = aisFsmGetInstance(prAdapter, i);
			for (j = 0; j < MLD_LINK_MAX; j++) {
				tempBssDesc = aisGetLinkBssDesc(tempAis, j);
				if (tempBssDesc) {
					if (tempBssDesc->eBand == BAND_2G4) {
						ucExistGLinks++;
					} else {
						ucExistALinks++;
						rExistRfChnlInfo =
							apsGetRfChannelInfo(
								prAdapter,
								tempBssDesc,
								NULL);
					}
				}
			}
		}

		rCurrRfChnlInfo =
			apsGetRfChannelInfo(prAdapter, prBssDesc, NULL);

		/* If band not fully used */
		if (ucExistGLinks < 1 || ucExistALinks < ENUM_BAND_NUM - 1) {
			/* G band used, skip 2.4G */
			if (ucExistGLinks >= 1 &&
			    prBssDesc->eBand == BAND_2G4) {
				DBGLOG(APS, INFO,
					MACSTR " can't use G band",
					MAC2STR(prBssDesc->aucBSSID));
				return FALSE;
			}

			/* A band is fully used, only allow 2.4G */
			if (ucExistALinks >= ENUM_BAND_NUM - 1 &&
			    prBssDesc->eBand != BAND_2G4) {
				DBGLOG(APS, INFO,
					MACSTR " can't use A band",
					MAC2STR(prBssDesc->aucBSSID));
				return FALSE;
			}

#if CFG_SUPPORT_CCM
			/* A+A */
			if (prBssDesc->eBand != BAND_2G4 &&
			    ucExistALinks == 1 &&
			    !ccmAAAvailableCheck(prAdapter,
						 &rCurrRfChnlInfo,
						 &rExistRfChnlInfo)) {
				DBGLOG(APS, INFO, MACSTR
					" Band[%d] Chnl[%d] ChnlBw[%d] Freq[%d] can't form A+A DBDC with Band[%d] Chnl[%d] ChnlBw[%d] Freq[%d]",
					MAC2STR(prBssDesc->aucBSSID),
					rCurrRfChnlInfo.eBand,
					rCurrRfChnlInfo.ucChannelNum,
					rCurrRfChnlInfo.ucChnlBw,
					rCurrRfChnlInfo.u4CenterFreq1,
					rExistRfChnlInfo.eBand,
					rExistRfChnlInfo.ucChannelNum,
					rExistRfChnlInfo.ucChnlBw,
					rExistRfChnlInfo.u4CenterFreq1);
				return FALSE;
			}
#endif
		}
	}


#if CFG_SUPPORT_NCHO
	if (prAdapter->rNchoInfo.fgNCHOEnabled) {
		if (!(BIT(prBssDesc->eBand) &
			prAdapter->rNchoInfo.ucRoamBand)) {
			DBGLOG(APS, WARN,
				MACSTR" band(%s) is not in NCHO roam band\n",
				MAC2STR(prBssDesc->aucBSSID),
				apucBandStr[prBssDesc->eBand]);
			return FALSE;
		}
	}
#endif

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

	if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), prBssDesc->rUpdateTime,
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
#if (CFG_SUPPORT_CONN_LOG == 1)
		connLogRsnMismatch(prAdapter,
			ucBssIndex,
			prBssDesc);
#endif
		return FALSE;
	}

	if (aisGetAisSpecBssInfo(prAdapter,
		ucBssIndex)->fgCounterMeasure) {
		DBGLOG(APS, WARN, MACSTR " Skip in counter measure period.\n",
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
#endif

#if CFG_SUPPORT_802_11BE_MLO
	if (mldIsMultiLinkEnabled(prAdapter, NETWORK_TYPE_AIS, ucBssIndex) &&
	    prBssDesc->rMlInfo.u2ApRemovalTimer) {
		DBGLOG(APS, WARN, MACSTR " is being removed.\n",
			MAC2STR(prBssDesc->aucBSSID));
		return FALSE;
	}
#endif

	return TRUE;
}

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

uint8_t apsLinkPlanDecision(struct ADAPTER *prAdapter,
	struct AP_COLLECTION *prAp, enum ENUM_MLO_LINK_PLAN eLinkPlan,
	uint8_t ucBssIndex)
{
	uint32_t u4LinkPlanBmap = BIT(MLO_LINK_PLAN_2_5);

#if (CFG_SUPPORT_WIFI_6G == 1)
#if (CFG_MLO_LINK_PLAN_MODE == 0)
	u4LinkPlanBmap |= BIT(MLO_LINK_PLAN_2_6); /* 2+5/6 */
#else
	u4LinkPlanBmap |= BIT(MLO_LINK_PLAN_2_5_6); /* 2+5+6 */
#endif
#endif

	return !!(u4LinkPlanBmap & BIT(eLinkPlan));
}

struct BSS_DESC *apsIntraUpdateCandi(struct ADAPTER *ad,
	struct AP_COLLECTION *ap, enum ENUM_BAND eBand, uint16_t min_score,
	enum ENUM_ROAMING_REASON reason, uint8_t search_blk, uint8_t bidx)
{
	struct AIS_FSM_INFO *ais = aisGetAisFsmInfo(ad, bidx);
	uint32_t bmap = aisGetBssIndexBmap(ais);
	struct LINK *link = &ap->arLinks[eBand];
	uint8_t aidx = AIS_INDEX(ad, bidx);
	struct BSS_DESC *bss, *cand = NULL;
	uint16_t score, goal_score = 0;

try_again:
	LINK_FOR_EACH_ENTRY(bss, link, rLinkEntryEss[aidx], struct BSS_DESC) {
		if (bss->fgPicked)
			continue;

		if (!search_blk) {
			/* Skip connected AP */
			uint8_t connected = !!(bss->fgIsConnected & bmap);

			if (reason != ROAMING_REASON_UPPER_LAYER_TRIGGER &&
			    reason != ROAMING_REASON_BTM &&
			    connected) {
				DBGLOG(APS, WARN, MACSTR" connected\n",
					MAC2STR(bss->aucBSSID));
				continue;
			}
		}

		if (!search_blk && link->u4NumElem > 1 && bss->prBlock)
			continue;

		if (!apsIsBssQualify(ad, bss, reason, min_score,
			bss->u2Score, bidx))
			continue;

		score = bss->u2Score;
		if (apsIntraNeedReplace(ad, cand, bss,
			goal_score, score, reason, bidx)) {
			cand = bss;
			goal_score = score;
		}
	}

	if (cand) {
		if ((cand->fgIsConnected & bmap) &&
		    !search_blk && link->u4NumElem > 1) {
			search_blk = TRUE;
			goto try_again;
		}

		cand->fgPicked = TRUE;
		goto done;
	}

	/* if No Candidate BSS is found, try BSSes which are in blocklist */
	if (!search_blk && link->u4NumElem > 1) {
		search_blk = TRUE;
		goto try_again;
	}
done:
	return cand;
}

static uint8_t apsIsValidBssDesc(struct ADAPTER *ad, struct BSS_DESC *bss,
	enum ENUM_ROAMING_REASON reason, uint8_t bidx)
{
	uint8_t valid = TRUE;
	struct STA_RECORD *sta = aisGetTargetStaRec(ad, bidx);

	if (!bss || !sta)
		return FALSE;

	if (bss->prBlock && bss->prBlock->fgDisallowed)
		valid = FALSE;

#if CFG_SUPPORT_ROAMING
	if (reason == ROAMING_REASON_TEMP_REJECT)
		valid = FALSE;

	if (reason == ROAMING_REASON_BTM) {
		struct NEIGHBOR_AP *nei = aisGetNeighborAPEntry(ad, bss, bidx);

		/* AP suggests to leave */
		if (!nei || (nei->fgPrefPresence && !nei->ucPreference))
			valid = FALSE;
	}
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (sta->fgApRemoval)
		valid = FALSE;
#endif

	if (!valid)
		DBGLOG(APS, INFO,
			"CURR[" MACSTR "] not valid, reason[%d]\n",
			MAC2STR(bss->aucBSSID), reason);

	return valid;
}

void apsUpdateTotalScore(struct ADAPTER *ad,
	struct BSS_DESC *links[], uint8_t link_num,
	enum ENUM_MLO_LINK_PLAN curr_plan,
	struct AP_COLLECTION *ap, uint8_t bidx)
{
	uint32_t total_score = 0;
	uint32_t total_tput = 0;
	uint8_t i;

	for (i = 0; i < link_num; i++) {
		total_score += links[i]->u2Score;
		total_tput += links[i]->u4Tput;
	}

	if (total_score > ap->u4TotalScore) {
		kalMemCopy(ap->aprTarget, links, sizeof(ap->aprTarget));
		ap->ucLinkNum = link_num;
		ap->u4TotalScore = total_score;
		ap->u4TotalTput = total_tput;
		ap->eMloMode = MLO_MODE_STR;
		ap->ucMaxSimuLinks = link_num - 1;
	}
}

uint8_t apsSortTrimCandiByScore(struct ADAPTER *ad, struct BSS_DESC *candi[],
	enum ENUM_MLO_LINK_PLAN *curr_plan)
{
	struct BSS_DESC *bss;
	struct BSS_DESC *bssTemp[APS_LINK_MAX] = {0};
	int i, j;
	uint8_t link_num = 0, band_bmap = 0;

	/* insertion sort by score */
	for (i = 1; i < APS_LINK_MAX; i++) {
		uint16_t score = 0;

		bss = candi[i];
		score = (bss && !bss->fgDriverGen) ? bss->u2Score : 0;

		for (j = i - 1; j >= 0 && (candi[j] ?
			candi[j]->u2Score : 0) < score; j--)
			candi[j + 1] = candi[j];

		candi[j + 1] = bss;
	}

	/* ensure no null target */
	for (i = 0, j = 0; i < APS_LINK_MAX; i++) {
		if (candi[i]) {
			link_num++;
			/* bssTemp will not be null */
			bssTemp[j] = candi[i];
			j++;
		}
	}
	/* Make sure candi[0] ~ candi[link_num - 1] will not be null */
	for (i = 0; i < link_num; i++)
		candi[i] = bssTemp[i];

#if (CFG_SUPPORT_802_11BE == 1)
	/* trim ap */
	if (link_num > ad->rWifiVar.ucStaMldLinkMax) {
		DBGLOG(APS, INFO, "trim links %d => %d",
			link_num, ad->rWifiVar.ucStaMldLinkMax);
		link_num = ad->rWifiVar.ucStaMldLinkMax;
	}
#endif

	/* find matched link plan by final link combination */
	for (i = 0; i < link_num; i++)
		band_bmap |= BIT(candi[i]->eBand);
	*curr_plan = apsSearchLinkPlan(ad, band_bmap, link_num);

	return link_num;
}

enum ENUM_MLO_LINK_PLAN apsSearchLinkPlan(struct ADAPTER *prAdapter,
	uint8_t ucRfBandBmap, uint8_t ucLinkNum)
{
	switch (ucRfBandBmap) {
	case BIT(BAND_2G4):
		if (ucLinkNum == 1)
			return MLO_LINK_PLAN_2;
		break;
	case BIT(BAND_5G): /* 5 or 5+5 */
		if (ucLinkNum == 1)
			return MLO_LINK_PLAN_5;
		else if (ucLinkNum == 2)
			return MLO_LINK_PLAN_5_5;
		break;
	case BIT(BAND_2G4) | BIT(BAND_5G): /* 2+5 or 2+5+5 */
		if (ucLinkNum == 2)
			return MLO_LINK_PLAN_2_5;
		else if (ucLinkNum == 3)
			return MLO_LINK_PLAN_2_5_5;
		break;
#if (CFG_SUPPORT_WIFI_6G == 1)
	case BIT(BAND_6G): /* 6 or 6+6 */
		if (ucLinkNum == 1)
			return MLO_LINK_PLAN_6;
		else if (ucLinkNum == 2)
			return MLO_LINK_PLAN_6_6;
		break;
	case BIT(BAND_2G4) | BIT(BAND_6G): /* 2+6 or 2+6+6 */
		if (ucLinkNum == 2)
			return MLO_LINK_PLAN_2_6;
		else if (ucLinkNum == 3)
			return MLO_LINK_PLAN_2_6_6;
		break;
	case BIT(BAND_5G) | BIT(BAND_6G):
		if (ucLinkNum == 2)
			return MLO_LINK_PLAN_5_6;
		break;
	case BIT(BAND_2G4) | BIT(BAND_5G) | BIT(BAND_6G):
		if (ucLinkNum == 3)
			return MLO_LINK_PLAN_2_5_6;
		break;
#endif
	default:
		break;
	}

	return MLO_LINK_PLAN_NUM;
}

void apsIntraSelectLinkPlan(struct ADAPTER *ad, struct AP_COLLECTION *ap,
	uint16_t min_score, enum ENUM_ROAMING_REASON reason, uint8_t bidx)
{
	struct mt66xx_chip_info *prChipInfo = ad->chip_info;
	struct CONNECTION_SETTINGS *conn = aisGetConnSettings(ad, bidx);
	enum ENUM_PARAM_CONNECTION_POLICY policy = conn->eConnectionPolicy;
	uint8_t aidx = AIS_INDEX(ad, bidx);
	enum ENUM_MLO_LINK_PLAN curr_plan;
	struct BSS_DESC *bss;
	int i, j;

	/* candi bss scoring */
	for (i = BAND_2G4; i < BAND_NUM; i++) {
		struct LINK *link = &ap->arLinks[i];

		LINK_FOR_EACH_ENTRY(bss, link,
			rLinkEntryEss[aidx], struct BSS_DESC) {

			bss->u2Score =
				apsCalculateApScore(ad, bss, reason, bidx);
			bss->u4Tput =
				apsGetEstimatedTput(ad, bss, bidx);

#if (CFG_SUPPORT_ROAMING_LOG == 1)
			if (roamingFsmIsDiscovering(ad, bidx)) {
				char log[32] = {0};

				kalSprintf(log, "SCORE_CANDI[%d]", ap->u4Index);
				roamingFsmLogSocre(ad, log, bidx, bss,
					bss->u2Score, bss->u4Tput);
			}
#endif

			if (policy == CONNECT_BY_BSSID) {
				if (EQUAL_MAC_ADDR(bss->aucBSSID,
						   conn->aucBSSID))
					bss->u2Score = BSS_MATCH_BSSID_SCORE;
			} else if (policy == CONNECT_BY_BSSID_HINT) {
				uint8_t oce = FALSE;
				uint8_t chnl = nicFreq2ChannelNum(
						conn->u4FreqInMHz * 1000);

#if CFG_SUPPORT_MBO
				oce = ad->rWifiVar.u4SwTestMode ==
					ENUM_SW_TEST_MODE_SIGMA_OCE;
#endif
				if (!oce && EQUAL_MAC_ADDR(bss->aucBSSID,
					conn->aucBSSIDHint) &&
				    (chnl == 0 || chnl == bss->ucChannelNum)) {
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
						bss->u2Score =
						     BSS_MATCH_BSSID_HINT_SCORE;
					}
				}
			}
		}
	}

	/* select highest score link plan */
	for (i = 0; i < MLO_LINK_PLAN_NUM; i++) {
		struct BSS_DESC *candi[APS_LINK_MAX] = {0};
		uint8_t found = FALSE;
		uint32_t akm = 0;
		uint16_t best = 0;
		uint8_t allow, link_num;
		enum ENUM_BAND *link_plan = g_aeLinkPlan[i];

		/* reset picked flag */
		for (j = BAND_2G4; j < BAND_NUM; j++) {
			struct LINK *link = &ap->arLinks[j];

			LINK_FOR_EACH_ENTRY(bss, link,
				rLinkEntryEss[aidx], struct BSS_DESC)
				bss->fgPicked = FALSE;
		}

		if (prChipInfo->apsLinkPlanDecision)
			allow = prChipInfo->apsLinkPlanDecision(ad, ap,
				i, bidx);
		else
			allow = apsLinkPlanDecision(ad, ap,
				i, bidx);

#if (CFG_SINGLE_BAND_MLSR_56 == 1)
		if (mldNeedSingleBandMlsr56(ad, i))
			allow = TRUE;
#endif

		/* skip if not found matched link plan */
		if (!allow)
			continue;

		for (j = 0; j < APS_LINK_MAX; j++) {
			if (link_plan[j] == BAND_NULL)
				continue;

			candi[j] = bss = apsIntraUpdateCandi(ad, ap,
			       link_plan[j], min_score, reason, FALSE, bidx);

			if (!bss)
				continue;

			found = TRUE;

			/* use akm of best ap as target akm */
			if (bss->u2Score > best) {
				best = bss->u2Score;
				akm = bss->u4RsnSelectedAKMSuite;
			}
		}

		/* skip if no candidate */
		if (!found)
			continue;

		/* use lower score to find links if already found one */
		for (j = 0; j < APS_LINK_MAX; j++) {
			if (link_plan[j] == BAND_NULL)
				continue;

			bss = candi[j];

			if (!bss)
				candi[j] = bss = apsIntraUpdateCandi(ad, ap,
				       link_plan[j], 0, reason, TRUE, bidx);

			if (bss && bss->u4RsnSelectedAKMSuite != akm) {
				DBGLOG(APS, INFO,
				       "Remove target akm 0x%x!=0x%x\n",
				       bss->u4RsnSelectedAKMSuite, akm);
				candi[j] = NULL;
			}
		}

		link_num = apsSortTrimCandiByScore(ad, candi, &curr_plan);

		if (prChipInfo->apsUpdateTotalScore)
			prChipInfo->apsUpdateTotalScore(ad,
				candi, link_num, curr_plan, ap, bidx);
		else
			apsUpdateTotalScore(ad,
				candi, link_num, curr_plan, ap, bidx);

#if (CFG_SINGLE_BAND_MLSR_56 == 1)
		if (mldNeedSingleBandMlsr56(ad, curr_plan) && link_num == 2) {
			kalMemCopy(ap->aprTarget, candi, sizeof(ap->aprTarget));
			ap->ucLinkNum = 2;
			ap->eMloMode = MLO_MODE_SB_MLSR;
			ap->ucMaxSimuLinks = 0;
		}
#endif
	}
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
	uint16_t min_score = 0;
	int i, j, k;

	/* minimum requirement */
	for (i = 0; i < MLD_LINK_MAX; i++) {
		bss = aisGetLinkBssDesc(ais, i);

		if (!apsIsValidBssDesc(ad, bss, reason, bidx))
			continue;

		bss->u2Score = apsCalculateApScore(ad, bss, reason, bidx);
		bss->u4Tput = apsGetEstimatedTput(ad, bss, bidx);

		if (min_score == 0 || bss->u2Score < min_score)
			min_score = bss->u2Score;

		DBGLOG(APS, INFO,
			"CURR[" MACSTR "] score[%d] tput[%d]\n",
			MAC2STR(bss->aucBSSID), bss->u2Score, bss->u4Tput);
#if (CFG_SUPPORT_ROAMING_LOG == 1)
		if (roamingFsmIsDiscovering(ad, bidx))
			roamingFsmLogSocre(ad, "SCORE_CUR_AP", bidx,
				bss, bss->u2Score, bss->u4Tput);
#endif
	}

	LINK_FOR_EACH_ENTRY_SAFE(ap, nap,
			ess, rLinkEntry, struct AP_COLLECTION) {

		/* select best link plan */
		apsIntraSelectLinkPlan(ad, ap, min_score, reason, bidx);

		if (ap->ucLinkNum == 0)
			continue;

		for (i = 0, j = 0, k = 0; i < ap->ucLinkNum; i++) {
			struct BSS_DESC *cand = ap->aprTarget[i];
			uint8_t addr[MAC_ADDR_LEN] = {0};
			uint8_t *mld_addr = addr;


			if (cand->prBlock) {
				j++;

				if (cand->prBlock->fgDeauthLastTime)
					ap->fgIsLastDeauth = TRUE;
			}

			if (cand->fgIsConnected & bmap)
				k++;

			if (cand->u2Score == BSS_MATCH_BSSID_SCORE)
				ap->fgIsMatchBssid = TRUE;

			if (cand->u2Score == BSS_MATCH_BSSID_HINT_SCORE)
				ap->fgIsMatchBssidHint = TRUE;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			mld_addr = cand->rMlInfo.aucMldAddr;
#endif

			DBGLOG(APS, INFO,
				"CAND[%d] BSS[" MACSTR " %s mld=" MACSTR
				"] score[%d] tput[%d] conn[%d] bssid[%d] bssid_hint[%d] blk[%d] deauth[%d] mode[%d] simu[%d]\n",
				ap->u4Index,
				MAC2STR(cand->aucBSSID),
				apucBandStr[cand->eBand],
				MAC2STR(mld_addr),
				cand->u2Score, cand->u4Tput,
				ap->aprTarget[i]->fgIsConnected,
				cand->u2Score == BSS_MATCH_BSSID_SCORE,
				cand->u2Score == BSS_MATCH_BSSID_HINT_SCORE,
				cand->prBlock != NULL,
				ap->fgIsLastDeauth,
				ap->eMloMode, ap->ucMaxSimuLinks);
		}

		if (j == ap->ucLinkNum)
			ap->fgIsAllLinkInBlockList = TRUE;

		if (k == ap->ucLinkNum) {
			ap->fgIsAllLinkConnected = TRUE;
			current_ap = ap;
		}
	}

	return current_ap;
}

uint32_t apsCalculateFinalScore(struct ADAPTER *ad,
	struct AP_COLLECTION *ap, enum ENUM_ROAMING_REASON reason,
	uint8_t bidx)
{
	uint32_t score = 0;

#if (CFG_EXT_ROAMING == 1)
	/* Customization */
	switch (reason) {
#if CFG_SUPPORT_ROAMING
	case ROAMING_REASON_POOR_RCPI:
	case ROAMING_REASON_INACTIVE:
	case ROAMING_REASON_RETRY:
	case ROAMING_REASON_HIGH_CU:
	case ROAMING_REASON_BTM:
		score = ap->u4TotalTput;
		break;
#endif
	default:
		score = ap->u4TotalScore;
		break;
	}

#else
	/* Common */
	switch (reason) {
	default:
		score = ap->u4TotalTput;
		break;
	}

#endif

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

	/* Only select the last deauth AP in non-certified situations. */
	if (!cand)
		if (curr->fgIsLastDeauth &&
		    ad->rWifiVar.u4SwTestMode != ENUM_SW_TEST_MODE_NONE)
			return APS_LAST_DEAUTH;
		else
			return APS_FIRST_CANDIDATE;

	/*
	 *  Only allow selecting the AP of last deauth,
	 *  if there are no other suitable AP in ESS.
	 */
	if (curr->fgIsLastDeauth)
		return APS_LAST_DEAUTH;
	else if (cand->fgIsLastDeauth)
		return APS_NOT_LAST_DEAUTH;

#if (CFG_TC10_FEATURE == 0)
	if (apsNeedReplaceByRssi(ad, cand, curr, reason))
		return APS_BETTER_RSSI;
	if (apsNeedReplaceByRssi(ad, curr, cand, reason))
		return APS_WORSE_RSSI;
#endif

	return curr_score > cand_score ? APS_HIGH_SCORE : APS_LOW_SCORE;
}

struct BSS_DESC *apsFillBssDescSet(struct ADAPTER *ad,
	struct AP_COLLECTION *ap, struct BSS_DESC_SET *set, uint8_t bidx)
{
	struct mt66xx_chip_info *prChipInfo = ad->chip_info;
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
		set->ucRfBandBmap |= BIT(ap->aprTarget[i]->eBand);
	}

	if (policy == CONNECT_BY_BSSID)
		goto done;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (prChipInfo->apsFillBssDescSet)
		prChipInfo->apsFillBssDescSet(ad, set, bidx);

	/* pick by special requirement
	 * and avoid picking the poor-quality link.
	 */
	for (i = 1; i < set->ucLinkNum; i++) {
		uint8_t *found = NULL;
		struct BSS_DESC *bss;

		bss = set->aprBssDesc[i];
		if (bss->fgDriverGen)
			continue;

		if (set->aprBssDesc[0]->ucJoinFailureCount >=
		    AIS_ROAMING_CONNECTION_TRIAL_LIMIT &&
		    bss->ucJoinFailureCount <
		    set->aprBssDesc[0]->ucJoinFailureCount) {
			set->aprBssDesc[i] = set->aprBssDesc[0];
			set->aprBssDesc[0] = bss;
			found = "bad_main_link";
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
		}

		if (found != NULL) {
			DBGLOG(APS, INFO, MACSTR
				" link_id=%d max_links=%d Setup for %s\n",
				MAC2STR(bss->aucBSSID),
				bss->rMlInfo.ucLinkIndex,
				bss->rMlInfo.ucMaxSimuLinks,
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
		set->fgIsAllLinkInBlockList = ap->fgIsAllLinkInBlockList;
		set->fgIsAllLinkConnected = ap->fgIsAllLinkConnected;
		set->eMloMode = ap->eMloMode;
		set->ucMaxSimuLinks = ap->ucMaxSimuLinks;
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

		if (!try_blockList && ap->fgIsAllLinkInBlockList)
			continue;

		score = apsCalculateFinalScore(ad, ap, reason, bidx);
		replace_reason = apsInterNeedReplace(ad, policy,
			cand, ap, best, score, reason, bidx);
		if (replace_reason >= APS_NEED_REPLACE) {
			best = score;
			cand = ap;
		}

		DBGLOG(APS, INFO,
			"%s CAND[%d] %s[" MACSTR
			"] num[%d] score[%d] (%s)",
			replace_reason >= APS_FIRST_CANDIDATE ? "--->" : "<---",
			ap->u4Index, ap->ucLinkNum > 1 ? "MLD" : "BSS",
			MAC2STR(ap->aucAddr), ap->ucLinkNum, score,
			replace_reason < APS_REPLACE_REASON_NUM ?
			apucReplaceReasonStr[replace_reason] :
			"UNKNOWN");

		/* early leave for specific reasons */
		if (replace_reason == APS_MATCH_BSSID ||
		    replace_reason == APS_MATCH_BSSID_HINT)
			goto done;
	}

	if (!try_blockList && (!cand || cand->fgIsAllLinkConnected)) {
		try_blockList = TRUE;
		DBGLOG(APS, INFO, "No ap collection found, try blocklist\n");
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

	aisRemoveArpNRBlocklist(ad, AIS_BLOCKLIST_TIMEOUT_ARP_NO_RSP);
	aisRemoveTimeoutBlocklist(ad, AIS_BLOCKLIST_TIMEOUT);
	aisClearCusBlocklist(ad, bidx, FALSE);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	aisRemoveTimeoutMldBlocklist(ad);
#endif

#if CFG_SUPPORT_802_11K
	/* check before using neighbor report */
	aisCheckNeighborApValidity(ad, bidx);
#endif

	count = apsUpdateEssApList(ad, reason, bidx);
	current_ap = apsIntraApSelection(ad, reason, bidx);
	cand = apsInterApSelection(ad, set, reason, bidx, current_ap);
	if (cand) {
		if (cand->eBand < 0 || cand->eBand >= BAND_NUM) {
			DBGLOG(APS, WARN, "Invalid Band %d\n", cand->eBand);
		} else {
			DBGLOG(APS, INFO,
				"Selected "
				MACSTR ", RSSI[%d] Band[%s] when find %s, "
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
	}

	DBGLOG(APS, INFO, "Selected None when find %s, " MACSTR
		" in %d(%d) BSSes.\n",
		conn->aucSSID, MAC2STR(conn->aucBSSID),
		count,
		ess->u4NumElem);
done:
#if (CFG_SUPPORT_ROAMING_LOG == 1)
	roamingFsmLogResult(ad, bidx, cand);
#endif
	apsResetEssApList(ad, bidx);
	return cand;
}

