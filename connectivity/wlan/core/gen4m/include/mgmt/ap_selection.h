/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _AP_SELECTION_H
#define _AP_SELECTION_H

typedef uint8_t(*PFN_SELECTION_POLICY_FUNC) (
	enum ENUM_BAND eCurrentBand,
	int8_t cCandidateRssi,
	int8_t cCurrentRssi
);

struct NETWORK_SELECTION_POLICY_BY_BAND {
	enum ENUM_BAND eCandidateBand;
	PFN_SELECTION_POLICY_FUNC pfnNetworkSelection;
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

struct BSS_DESC *apsSearchBssDescByScore(struct ADAPTER *prAdapter,
	enum ENUM_ROAMING_REASON eRoamReason,
	uint8_t ucBssIndex, struct BSS_DESC_SET *prBssDescSet);

#endif

static const char * const apucBandStr[BAND_NUM] = {
	"NULL",
	"2.4G",
	"5G",
#if (CFG_SUPPORT_WIFI_6G == 1)
	"6G",
#endif
};

