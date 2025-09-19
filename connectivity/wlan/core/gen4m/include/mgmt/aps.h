/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _APS_H
#define _APS_H

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define APS_LINK_MAX		3

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct CU_INFO {
	uint32_t ucTotalCount;
	uint32_t ucTotalCu;
	enum ENUM_BAND eBand;
};

struct APS_INFO {
#if (CFG_SUPPORT_WIFI_6G == 1)
	/*
	 * Record 6G info from index 256
	 * | <-- 0-255 --> | <-- 256-511 --> |
	 * |	2.4G,5G    |	   6G	     |
	 */
	struct CU_INFO arCuInfo[512];
#else
	struct CU_INFO arCuInfo[256];
#endif
	uint8_t ucConsiderEsp;
	uint8_t fgIsGBandCoex;
	uint16_t u2EssApNum;
};

struct AP_COLLECTION {
	struct LINK_ENTRY rLinkEntry;
	struct AP_COLLECTION *hnext; /* next entry in hash table list */
	uint32_t u4Index;
	struct BSS_DESC *aprTarget[APS_LINK_MAX];
	struct LINK arLinks[BAND_NUM]; /* categorize AP by band */
	uint8_t ucLinkNum;
	uint8_t ucTotalCount; /* total BssDesc count */
	uint8_t fgIsMatchBssid;
	uint8_t fgIsMatchBssidHint;
	uint8_t fgIsAllLinkInBlockList;
	uint8_t fgIsAllLinkConnected;
	enum ENUM_MLO_MODE eMloMode;
	uint8_t ucMaxSimuLinks;
	uint8_t fgIsLastDeauth;
	uint32_t u4TotalTput;
	uint32_t u4TotalScore;
	uint8_t aucAddr[MAC_ADDR_LEN]; /* mld addr or bssid */
};

#if (CFG_SUPPORT_AVOID_DESENSE == 1)
struct WFA_DESENSE_CHANNEL_LIST {
	int8_t ucChLowerBound;
	int8_t ucChUpperBound;
};

extern const struct WFA_DESENSE_CHANNEL_LIST desenseChList[BAND_NUM];

#define IS_CHANNEL_IN_DESENSE_RANGE(_prAdapter, _ch, _band) \
	(!!(_prAdapter->fgIsNeedAvoidDesenseFreq && \
	(_band != BAND_2G4) && (_band < BAND_NUM) && \
	(_ch >= desenseChList[_band].ucChLowerBound) && \
	(_ch <= desenseChList[_band].ucChUpperBound)))
#endif

static const char * const apucBandStr[BAND_NUM] = {
	"NULL",
	"2.4G",
	"5G",
#if (CFG_SUPPORT_WIFI_6G == 1)
	"6G",
#endif
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

struct BSS_DESC *apsSearchBssDescByScore(struct ADAPTER *prAdapter,
	enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex, struct BSS_DESC_SET *prBssDescSet);

enum ENUM_MLO_LINK_PLAN apsSearchLinkPlan(struct ADAPTER *prAdapter,
	uint8_t ucRfBandBmap, uint8_t ucLinkNum);

#endif

