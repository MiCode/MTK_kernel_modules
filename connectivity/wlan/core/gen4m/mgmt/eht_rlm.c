// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
** Id: @(#) eht_rlm.c@@
*/

/*! \file   "eht_rlm.c"
*    \brief This file contains EHT Phy processing routines.
*
*/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include "precomp.h"

#if (CFG_SUPPORT_802_11BE == 1)
#include "eht_rlm.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

#define EHT_CAP_INFO_MCS_MAP_MCS13	3

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

uint32_t ehtRlmCalculateCapIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucMaxBw;
	uint32_t u4OverallLen;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	u4OverallLen = OFFSET_OF(struct IE_EHT_CAP, aucVarInfo[0]);
	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "prBssInfo is null\n");
		return u4OverallLen;
	}

	ucMaxBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);

	if (ucMaxBw == MAX_BW_20MHZ) {
		/* 20 MHz-Only Non-AP STA */
		u4OverallLen += !IS_BSS_APGO(prBssInfo) ?
		    sizeof(struct EHT_SUPPORTED_MCS_BW20_FIELD) :
		    sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);
        } else {
		if (ucMaxBw >= MAX_BW_40MHZ) {
			u4OverallLen += sizeof(
				struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);
		}
		if (ucMaxBw >= MAX_BW_160MHZ) {
			u4OverallLen += sizeof(
				struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);
		}
		if (ucMaxBw >= MAX_BW_320_1MHZ) {
			u4OverallLen += sizeof(
				struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);
		}
        }

	// TODO: add variable IE length for EHT PPE threshold

	return u4OverallLen;
}

uint32_t ehtRlmCalculateOpIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec)
{
	/* struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL; */
	uint32_t u4OverallLen = OFFSET_OF(struct IE_EHT_OP, aucVarInfo[0]);

	u4OverallLen += sizeof(struct EHT_OP_INFO);

	return u4OverallLen;
}

static void ehtRlmFillBW80MCSMap(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t *prEhtSupportedMcsSet)
{
	uint8_t ucMcsMap, ucSupportedNss;
	struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD *_prEhtSupportedMcsSet
			= (struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD *)
				prEhtSupportedMcsSet;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	kalMemZero((void *) prEhtSupportedMcsSet,
		sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD));
	ucSupportedNss = wlanGetSupportNss(prAdapter,
		prBssInfo->ucBssIndex);

	DBGLOG(RLM, INFO,
		"eht uc80MNss: %d, op tx: %d, op rx: %d\n",
		ucSupportedNss,
		prBssInfo->ucOpChangeTxNss,
		prBssInfo->ucOpChangeRxNss);
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
	if (p2pFuncGetForceTrxConfig(prAdapter) ==
		P2P_FORCE_TRX_CONFIG_MCS9)
		ucMcsMap = prBssInfo->ucOpChangeRxNss +
			(prBssInfo->ucOpChangeTxNss << 4);
	else
#endif
#endif
		ucMcsMap = ucSupportedNss + (ucSupportedNss << 4);

	if (prAdapter->fgMcsMapBeenSet & SET_EHT_BW80_MCS_MAP) {
		WLAN_SET_FIELD_24(_prEhtSupportedMcsSet,
			prAdapter->u4EhtMcsMap80MHzSetFromSigma);
	} else if (prAdapter->fgMcsMapBeenSet & SET_HE_MCS_MAP) {
		uint8_t map = prAdapter->ucMcsMapSetFromSigma;

		if (map >= HE_CAP_INFO_MCS_MAP_MCS9)
			_prEhtSupportedMcsSet->eht_mcs_0_9 = ucMcsMap;
		if (map >= HE_CAP_INFO_MCS_MAP_MCS11)
			_prEhtSupportedMcsSet->eht_mcs_10_11 = ucMcsMap;
		if (map >= EHT_CAP_INFO_MCS_MAP_MCS13)
			_prEhtSupportedMcsSet->eht_mcs_12_13 = ucMcsMap;
	} else if (IS_BSS_AIS(prBssInfo) && prWifiVar->ucStaMaxMcsMap != 0xFF) {
		if (prWifiVar->ucStaMaxMcsMap >= HE_CAP_INFO_MCS_MAP_MCS9)
			_prEhtSupportedMcsSet->eht_mcs_0_9 = ucMcsMap;
		else
			_prEhtSupportedMcsSet->eht_mcs_0_9 = 0;
		if (prWifiVar->ucStaMaxMcsMap >= HE_CAP_INFO_MCS_MAP_MCS11)
			_prEhtSupportedMcsSet->eht_mcs_10_11 = ucMcsMap;
		else
			_prEhtSupportedMcsSet->eht_mcs_10_11 = 0;
		if (prWifiVar->ucStaMaxMcsMap >= EHT_CAP_INFO_MCS_MAP_MCS13)
			_prEhtSupportedMcsSet->eht_mcs_12_13 = ucMcsMap;
		else
			_prEhtSupportedMcsSet->eht_mcs_12_13 = 0;
	} else {
		_prEhtSupportedMcsSet->eht_mcs_0_9 = ucMcsMap;
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
		if (p2pFuncGetForceTrxConfig(prAdapter) ==
			P2P_FORCE_TRX_CONFIG_MCS9) {
			_prEhtSupportedMcsSet->eht_mcs_10_11 = 0;
			_prEhtSupportedMcsSet->eht_mcs_12_13 = 0;
		} else {
			_prEhtSupportedMcsSet->eht_mcs_10_11 =
				ucMcsMap;
			_prEhtSupportedMcsSet->eht_mcs_12_13 =
				ucMcsMap;
		}
#else
	_prEhtSupportedMcsSet->eht_mcs_10_11 = ucMcsMap;
	_prEhtSupportedMcsSet->eht_mcs_12_13 = ucMcsMap;
#endif
#else
	_prEhtSupportedMcsSet->eht_mcs_10_11 = ucMcsMap;
	_prEhtSupportedMcsSet->eht_mcs_12_13 = ucMcsMap;
#endif
	}

	DBGLOG(RLM, TRACE, "EHT BW80 MCS Map: %x %x %x",
		_prEhtSupportedMcsSet->eht_mcs_12_13,
		_prEhtSupportedMcsSet->eht_mcs_10_11,
		_prEhtSupportedMcsSet->eht_mcs_0_9);
}

static void ehtRlmFillBW20MCSMap(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t *prEhtSupportedMcsSet)
{
	uint8_t ucMcsMap, ucSupportedNss;
	struct EHT_SUPPORTED_MCS_BW20_FIELD *_prEhtSupportedMcsSet =
		(struct EHT_SUPPORTED_MCS_BW20_FIELD *) prEhtSupportedMcsSet;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	kalMemZero((void *) prEhtSupportedMcsSet,
		sizeof(struct EHT_SUPPORTED_MCS_BW20_FIELD));
	ucSupportedNss = wlanGetSupportNss(prAdapter,
		prBssInfo->ucBssIndex);
	ucMcsMap = ucSupportedNss + (ucSupportedNss << 4);

	DBGLOG(RLM, INFO,
		"eht uc20MNss: %d, op tx: %d, op rx: %d\n",
		ucSupportedNss,
		prBssInfo->ucOpChangeTxNss,
		prBssInfo->ucOpChangeRxNss);
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
	if (p2pFuncGetForceTrxConfig(prAdapter) ==
		P2P_FORCE_TRX_CONFIG_MCS9)
		ucMcsMap = prBssInfo->ucOpChangeRxNss +
			(prBssInfo->ucOpChangeTxNss << 4);
	else
#endif
#endif
		ucMcsMap = ucSupportedNss + (ucSupportedNss << 4);

	if (prAdapter->fgMcsMapBeenSet & SET_EHT_BW20_MCS_MAP) {
		WLAN_SET_FIELD_32(_prEhtSupportedMcsSet,
			prAdapter->u4EhtMcsMap20MHzSetFromSigma);
	} else if (prAdapter->fgMcsMapBeenSet & SET_HE_MCS_MAP) {
		uint8_t map = prAdapter->ucMcsMapSetFromSigma;

		_prEhtSupportedMcsSet->eht_bw20_mcs_0_7 = ucMcsMap;
		if (map >= HE_CAP_INFO_MCS_MAP_MCS9)
			_prEhtSupportedMcsSet->eht_bw20_mcs_8_9 = ucMcsMap;
		else
			_prEhtSupportedMcsSet->eht_bw20_mcs_8_9 = 0;
		if (map >= HE_CAP_INFO_MCS_MAP_MCS11)
			_prEhtSupportedMcsSet->eht_bw20_mcs_10_11 = ucMcsMap;
		else
			_prEhtSupportedMcsSet->eht_bw20_mcs_10_11 = 0;
		if (map >= EHT_CAP_INFO_MCS_MAP_MCS13)
			_prEhtSupportedMcsSet->eht_bw20_mcs_12_13 = ucMcsMap;
		else
			_prEhtSupportedMcsSet->eht_bw20_mcs_12_13 = 0;
	} else if (IS_BSS_AIS(prBssInfo) && prWifiVar->ucStaMaxMcsMap != 0xFF) {
		_prEhtSupportedMcsSet->eht_bw20_mcs_0_7 = ucMcsMap;
		if (prWifiVar->ucStaMaxMcsMap >= HE_CAP_INFO_MCS_MAP_MCS9)
			_prEhtSupportedMcsSet->eht_bw20_mcs_8_9 = ucMcsMap;
		if (prWifiVar->ucStaMaxMcsMap >= HE_CAP_INFO_MCS_MAP_MCS11)
			_prEhtSupportedMcsSet->eht_bw20_mcs_10_11 = ucMcsMap;
		if (prWifiVar->ucStaMaxMcsMap >= EHT_CAP_INFO_MCS_MAP_MCS13)
			_prEhtSupportedMcsSet->eht_bw20_mcs_12_13 = ucMcsMap;
	} else {
		_prEhtSupportedMcsSet->eht_bw20_mcs_0_7 = ucMcsMap;
		_prEhtSupportedMcsSet->eht_bw20_mcs_8_9 = ucMcsMap;
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
		if (p2pFuncGetForceTrxConfig(prAdapter) ==
			P2P_FORCE_TRX_CONFIG_MCS9) {
			_prEhtSupportedMcsSet->eht_bw20_mcs_10_11 = 0;
			_prEhtSupportedMcsSet->eht_bw20_mcs_12_13 = 0;
		} else {
			_prEhtSupportedMcsSet->eht_bw20_mcs_10_11 =
				ucMcsMap;
			_prEhtSupportedMcsSet->eht_bw20_mcs_12_13 =
				ucMcsMap;
		}
#else
		_prEhtSupportedMcsSet->eht_bw20_mcs_10_11 = ucMcsMap;
		_prEhtSupportedMcsSet->eht_bw20_mcs_12_13 = ucMcsMap;
#endif
#else
		_prEhtSupportedMcsSet->eht_bw20_mcs_10_11 = ucMcsMap;
		_prEhtSupportedMcsSet->eht_bw20_mcs_12_13 = ucMcsMap;
#endif
	}

	DBGLOG(RLM, TRACE, "EHT BW20 MCS Map: %x %x %x %x",
		_prEhtSupportedMcsSet->eht_bw20_mcs_12_13,
		_prEhtSupportedMcsSet->eht_bw20_mcs_10_11,
		_prEhtSupportedMcsSet->eht_bw20_mcs_8_9,
		_prEhtSupportedMcsSet->eht_bw20_mcs_0_7);
}

void ehtRlmFillCapIE(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct MSDU_INFO *prMsduInfo)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct IE_EHT_CAP *prEhtCap;
	struct EHT_PHY_CAP_INFO eht_phy_cap;
	uint32_t phy_cap_1 = 0;
	uint32_t phy_cap_2 = 0;
	uint32_t u4OverallLen = OFFSET_OF(struct IE_EHT_CAP, aucVarInfo[0]);
	uint8_t eht_mcs15_mru = EHT_MCS15_MRU_106_or_52_w_26_tone;
	uint8_t ucSupportedNss = 0;
	int8_t eht_bw = 0;
	u_int8_t fgBfEn = TRUE;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	prEhtCap = (struct IE_EHT_CAP *)
		(((uint8_t *)prMsduInfo->prPacket)+prMsduInfo->u2FrameLength);

	prEhtCap->ucId = ELEM_ID_RESERVED;
	prEhtCap->ucExtId = ELEM_EXT_ID_EHT_CAPS;

	ucSupportedNss = wlanGetSupportNss(prAdapter,
		prBssInfo->ucBssIndex) - 1;

	/* MAC capabilities */
	EHT_RESET_MAC_CAP(prEhtCap->ucEhtMacCap);

#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	/* EPCS: (previously called NSEP) supported for STA */
	if (IS_BSS_AIS(prBssInfo) && IS_FEATURE_ENABLED(prWifiVar->fgEnEpcs))
		SET_EHT_MAC_CAP_EPCS_PRI_ACCESS(prEhtCap->ucEhtMacCap);
#endif
	/* OM_CTRL: default support for both STA and AP; */
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtOMCtrl))
		SET_EHT_MAC_CAP_OM_CTRL(prEhtCap->ucEhtMacCap);

	/* TXOP_SHARING: not support */
	/* SET_EHT_MAC_CAP_TXOP_SHARING(prEhtCap->ucEhtMacCap); */

	/* SCS: default support for STA */
	if (IS_BSS_AIS(prBssInfo) && IS_FEATURE_ENABLED(prWifiVar->fgEnTuao))
		SET_EHT_MAC_CAP_SCS(prEhtCap->ucEhtMacCap);

	/*
	 * RTWT support bit
	 */
#if (CFG_SUPPORT_RTWT == 1)
	if (IS_FEATURE_ENABLED(prWifiVar->ucRTWTSupport))
		SET_EHT_MAC_CAP_RESTRICTED_TWT(prEhtCap->ucEhtMacCap);
#endif

	/* PHY capabilities */
	EHT_RESET_PHY_CAP(prEhtCap->ucEhtPhyCap);

	eht_bw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prBssInfo->eBand == BAND_6G) {
		if (eht_bw >= MAX_BW_320_1MHZ)
			phy_cap_1 |= DOT11BE_PHY_CAP_320M_6G;
	}
#endif

	if (!IS_BSS_APGO(prBssInfo)) {
		if (IS_FEATURE_ENABLED(prWifiVar->ucEhtPartialBwDLMUMIMO))
			phy_cap_2 |= DOT11BE_PHY_CAP_PARTIAL_BW_DL_MU_MIMO;

		if (IS_FEATURE_ENABLED(prWifiVar->ucStaEht242ToneRUWt20M)) {
			if (eht_bw == MAX_BW_20MHZ)
				phy_cap_1 &=
					~DOT11BE_PHY_CAP_242_TONE_RU_WT_20M;
			else
				phy_cap_1 |= DOT11BE_PHY_CAP_242_TONE_RU_WT_20M;
		}
	} else {
		/* phy_cap_2 |= DOT11BE_PHY_CAP_PPE_THRLD_PRESENT; */
	}

	if (eht_bw >= MAX_BW_20MHZ) {
		/* set 3 to support AP NSS 4 */
		SET_DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M(phy_cap_1,
			prWifiVar->ucEhtBfeeSSLeEq80m);
		if (IS_FEATURE_ENABLED(prWifiVar->ucEhtSUBfer))
			/* set 1 to support 2 TX NSS */
			SET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_LE_EQ_80M(
				phy_cap_1, (uint32_t)ucSupportedNss);
	}

	if (eht_bw >= MAX_BW_80MHZ)
		eht_mcs15_mru |= EHT_MCS15_MRU_484_w_242_tone_80M;

	if (eht_bw >= MAX_BW_160MHZ) {
		eht_mcs15_mru |= EHT_MCS15_MRU_996_to_242_tone_160M;
		/* set 3 to support AP NSS 4 */
		SET_DOT11BE_PHY_CAP_BFEE_160M(phy_cap_1,
			prWifiVar->ucEhtBfee160m);
		if (IS_FEATURE_ENABLED(prWifiVar->ucEhtSUBfer))
			/* set 1 to support TX NSS 2 */
			SET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_160M(
				phy_cap_1, (uint32_t)ucSupportedNss);
	}
	if (eht_bw >= MAX_BW_320_1MHZ) {
		eht_mcs15_mru |= EHT_MCS15_MRU_3x996_tone_320M;
		/* set 3 to support AP NSS 4 */
		SET_DOT11BE_PHY_CAP_BFEE_320M(phy_cap_1,
			prWifiVar->ucEhtBfee320m);
		if (IS_FEATURE_ENABLED(prWifiVar->ucEhtSUBfer))
			/* set 1 to support TX NSS 2 */
			SET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_320M(
				phy_cap_1, (uint32_t)ucSupportedNss);
	}

	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtNDP4xLTF3dot2usGI))
		phy_cap_1 |= DOT11BE_PHY_CAP_NDP_4X_EHT_LTF_3DOT2US_GI;

	/* phy_cap_1 &= ~DOT11BE_PHY_CAP_PARTIAL_BW_UL_MU_MIMO; */
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtSUBfer))
		phy_cap_1 |= DOT11BE_PHY_CAP_SU_BFER;

#if (CFG_SUPPORT_CONDITIONAL_BFEE == 1)
	if (prAdapter->rWifiVar.u4SwTestMode != ENUM_SW_TEST_MODE_SIGMA_AX &&
	    prAdapter->rWifiVar.u4SwTestMode != ENUM_SW_TEST_MODE_SIGMA_BE &&
	    IS_BSS_AIS(prBssInfo)) {
		struct BSS_DESC *prBssDesc = aisGetTargetBssDesc(prAdapter,
			prBssInfo->ucBssIndex);

		if (prBssDesc != NULL) {
			uint32_t soundingDim = 0;
			uint32_t cap1 = *(uint32_t *)prBssDesc->ucEhtPhyCapInfo;

			if (eht_bw >= MAX_BW_320_1MHZ)
				soundingDim =
				   GET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_320M(cap1);
			else if (eht_bw >= MAX_BW_160MHZ)
				soundingDim =
				   GET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_160M(cap1);
			else if (eht_bw >= MAX_BW_20MHZ)
				soundingDim =
			      GET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_LE_EQ_80M(cap1);

			DBGLOG(RLM, INFO,
				"eht ucSupportedNss: %d, soundingDim: %d\n",
				ucSupportedNss, soundingDim);
			if (ucSupportedNss == soundingDim) {
				fgBfEn = FALSE;
				DBGLOG(SW4, ERROR,
					"Disable Bfee due to same Nss between STA and AP\n");
			} else {
				fgBfEn = TRUE;
			}
		}
	}
#endif

	if (fgBfEn == TRUE && IS_FEATURE_ENABLED(prWifiVar->ucEhtSUBfee))
		phy_cap_1 |= DOT11BE_PHY_CAP_SU_BFEE;
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtNG16SUFeedback))
		phy_cap_1 |= DOT11BE_PHY_CAP_NG16_SU_FEEDBACK;
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtNG16MUFeedback))
		phy_cap_1 |= DOT11BE_PHY_CAP_NG16_MU_FEEDBACK;
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtCodebook75MuFeedback))
		phy_cap_1 |= DOT11BE_PHY_CAP_CODEBOOK_7_5_MU_FEEDBACK;
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtTrigedSUBFFeedback))
		phy_cap_1 |= DOT11BE_PHY_CAP_TRIGED_SU_BF_FEEDBACK;
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtTrigedMUBFPartialBWFeedback))
		phy_cap_1 |= DOT11BE_PHY_CAP_TRIGED_MU_BF_PARTIAL_BW_FEEDBACK;
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtTrigedCQIFeedback))
		phy_cap_1 |= DOT11BE_PHY_CAP_TRIGED_CQI_FEEDBACK;
	/* phy_cap_2 &= ~DOT11BE_PHY_CAP_PSR_BASED_SR; */
	/* phy_cap_2 &= ~DOT11BE_PHY_CAP_POWER_BOOST_FACTOR; */
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtMUPPDU4xEHTLTFdot8usGI))
		phy_cap_2 |= DOT11BE_PHY_CAP_EHT_MU_PPDU_4X_EHT_LTF_DOT8US_GI;
	SET_DOT11BE_PHY_CAP_MAX_NC(phy_cap_2, (uint32_t)ucSupportedNss);

	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtNonTrigedCQIFeedback))
		phy_cap_2 |= DOT11BE_PHY_CAP_NON_TRIGED_CQI_FEEDBACK;
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtTx1024QAM4096QAMLe242ToneRU))
		phy_cap_2 |= DOT11BE_PHY_CAP_TX_1024QAM_4096QAM_LE_242_TONE_RU;
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtRx1024QAM4096QAMLe242ToneRU))
		phy_cap_2 |= DOT11BE_PHY_CAP_RX_1024QAM_4096QAM_LE_242_TONE_RU;
	/* phy_cap_2 |= DOT11BE_PHY_CAP_PPE_THRLD_PRESENT; */
	SET_DOT11BE_PHY_CAP_COMMON_NOMINAL_PKT_PAD(phy_cap_2,
		prWifiVar->ucEhtCommonNominalPktPadding);
	SET_DOT11BE_PHY_CAP_MAX_EHT_LTF_NUM(phy_cap_2,
		prWifiVar->ucEhtMaxLTFNum);
	SET_DOT11BE_PHY_CAP_MCS_15(phy_cap_2,
		prWifiVar->ucEhtMCS15 != 0xFF ?
		prWifiVar->ucEhtMCS15 : eht_mcs15_mru);
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prBssInfo->eBand == BAND_6G &&
	    IS_FEATURE_ENABLED(prWifiVar->ucEhtDup6G))
		phy_cap_2 |= DOT11BE_PHY_CAP_EHT_DUP_6G;
#endif
	if (IS_FEATURE_ENABLED(prWifiVar->ucEht20MRxNDPWiderBW))
		phy_cap_2 |= DOT11BE_PHY_CAP_20M_RX_NDP_W_WIDER_BW;
	if (IS_FEATURE_ENABLED(prWifiVar->ucEhtTbSndFBRateLimit))
		phy_cap_2 |= DOT11BE_PHY_CAP_TB_SND_FB_RATE_LIMIT;

	/* phy_cap_2 &= ~DOT11BE_PHY_CAP_NON_OFDMA_UL_MU_MIMO_80M; */
	/* phy_cap_2 &= ~DOT11BE_PHY_CAP_NON_OFDMA_UL_MU_MIMO_160M; */
	/* phy_cap_2 &= ~DOT11BE_PHY_CAP_NON_OFDMA_UL_MU_MIMO_320M; */
	/* phy_cap_2 &= ~DOT11BE_PHY_CAP_MU_BFER_80M; */
	/* phy_cap_2 &= ~DOT11BE_PHY_CAP_MU_BFER_160M; */
	/* phy_cap_2 &= ~DOT11BE_PHY_CAP_MU_BFER_320M; */

	DBGLOG(RLM, INFO,
		"[%d] eht_bw=%d, phy_cap_1=0x%x, phy_cap_2=0x%x\n",
		prBssInfo->ucBssIndex,
		eht_bw, phy_cap_1, phy_cap_2);

	eht_phy_cap.phy_capinfo_1 = (phy_cap_1);
	eht_phy_cap.phy_capinfo_2 = (phy_cap_2);

	memcpy(prEhtCap->ucEhtPhyCap, &phy_cap_1,
		sizeof(phy_cap_1));
	memcpy(prEhtCap->ucEhtPhyCap + 4, &phy_cap_2,
		sizeof(phy_cap_2));

	/* Set EHT MCS MAP & NSS */
	if (eht_bw == MAX_BW_20MHZ) {
		uint8_t *mcs = NULL;

		mcs = (((uint8_t *) prEhtCap) + u4OverallLen);

		/* 20 MHz-Only Non-AP STA */
		if (!IS_BSS_APGO(prBssInfo)) {
			ehtRlmFillBW20MCSMap(prAdapter, prBssInfo, mcs);
			u4OverallLen +=
				sizeof(struct EHT_SUPPORTED_MCS_BW20_FIELD);
		} else {
			ehtRlmFillBW80MCSMap(prAdapter, prBssInfo, mcs);
			u4OverallLen += sizeof(
				struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);
		}
	} else {
		uint8_t *prEhtSupportedBw80McsSet = NULL;

		if (eht_bw >= MAX_BW_40MHZ) {
			prEhtSupportedBw80McsSet =
				(((uint8_t *) prEhtCap) + u4OverallLen);
			ehtRlmFillBW80MCSMap(
				prAdapter, prBssInfo, prEhtSupportedBw80McsSet);
			u4OverallLen += sizeof(
				struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);
		}
		if (eht_bw >= MAX_BW_160MHZ) {
			prEhtSupportedBw80McsSet =
				(((uint8_t *) prEhtCap) + u4OverallLen);
			ehtRlmFillBW80MCSMap(
				prAdapter, prBssInfo, prEhtSupportedBw80McsSet);
			u4OverallLen += sizeof(
				struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);
		}
		if (eht_bw >= MAX_BW_320_1MHZ) {
			prEhtSupportedBw80McsSet =
				(((uint8_t *) prEhtCap) + u4OverallLen);
			ehtRlmFillBW80MCSMap(
				prAdapter, prBssInfo, prEhtSupportedBw80McsSet);
			u4OverallLen += sizeof(
				struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);
		}
	}
	/* Set EHT PPE Thresholds */
	if (phy_cap_2 & DOT11BE_PHY_CAP_PPE_THRLD_PRESENT) {
		// TODO: add EHT PPE threshold
	}

	prEhtCap->ucLength = u4OverallLen - ELEM_HDR_LEN;
	prMsduInfo->u2FrameLength += IE_SIZE(prEhtCap);

	DBGLOG_MEM8(RLM, INFO, prEhtCap, IE_SIZE(prEhtCap));
}

void ehtRlmReqGenerateCapIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11BE)
	    && (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11BE)))
		ehtRlmFillCapIE(prAdapter, prBssInfo, prMsduInfo);
}

void ehtRlmRspGenerateCapIEImpl(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucPhyTypeSet;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	if (!IS_BSS_ACTIVE(prBssInfo))
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	/* Decide PHY type set source */
	if (prStaRec) {
		/* Get PHY type set from target STA */
		ucPhyTypeSet = prStaRec->ucPhyTypeSet;
	} else {
		/* Get PHY type set from current BSS */
		ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
	}

	if (RLM_NET_IS_11BE(prBssInfo) &&
	    (ucPhyTypeSet & PHY_TYPE_SET_802_11BE))
		ehtRlmFillCapIE(prAdapter, prBssInfo, prMsduInfo);
}

void ehtRlmRspGenerateCapIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	/* pre-wifi7 device, build in vendor id */
	if (prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_HIDE_INFO)
		return;

	ehtRlmRspGenerateCapIEImpl(prAdapter, prMsduInfo);
}

uint8_t ehtRlmGetEhtOpBwByBssOpBw(uint8_t ucBssOpBw)
{
	uint8_t ucEhtOpBw = EHT_MAX_BW_20;

	switch (ucBssOpBw) {
	case MAX_BW_20MHZ:
		ucEhtOpBw = EHT_MAX_BW_20;
		break;

	case MAX_BW_40MHZ:
		ucEhtOpBw = EHT_MAX_BW_40;
		break;

	case MAX_BW_80MHZ:
		ucEhtOpBw = EHT_MAX_BW_80;
		break;

	case MAX_BW_160MHZ:
		ucEhtOpBw = EHT_MAX_BW_160;
		break;

	case MAX_BW_320_1MHZ:
	case MAX_BW_320_2MHZ:
		ucEhtOpBw = EHT_MAX_BW_320;
		break;

	default:
		DBGLOG(RLM, WARN, "unexpected Bss OP BW: %d\n",
		       ucBssOpBw);

		ucEhtOpBw = EHT_MAX_BW_20;
		break;
	}

	return ucEhtOpBw;
}

static void ehtRlmFillOpIE(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct MSDU_INFO *prMsduInfo)
{
	struct IE_EHT_OP *prEhtOp;
	/* struct IE_EHT_OP is packed,
	 * save to use sizeof instead of
	 * using OFFSET_OF with ZERO array at end
	 */
	uint32_t u4OverallLen = sizeof(struct IE_EHT_OP);
	uint8_t eht_bw = 0;
	struct EHT_OP_INFO *prEhtOpInfo;
	struct EHT_SUPPORTED_MCS_BW20_FIELD *prEhtMcsSet;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	prEhtOp = (struct IE_EHT_OP *)
		(((uint8_t *)prMsduInfo->prPacket)+prMsduInfo->u2FrameLength);
	prEhtMcsSet = (struct EHT_SUPPORTED_MCS_BW20_FIELD *)
		&prEhtOp->u4BasicEhtMcsNssSet;

	prEhtOp->ucId = ELEM_ID_RESERVED;
	prEhtOp->ucExtId = ELEM_EXT_ID_EHT_OP;

	/* MAC capabilities */
	EHT_RESET_OP(prEhtOp->ucEhtOpParams);

	EHT_SET_OP_PARAM_OP_INFO_PRESENT(prEhtOp->ucEhtOpParams);

	eht_bw = cnmOpModeGetMaxBw(prAdapter, prBssInfo);

	/* Basic EHT-MCS And Nss Set */
	kalMemZero(prEhtMcsSet, sizeof(*prEhtMcsSet));
	prEhtMcsSet->eht_bw20_mcs_0_7 = 1 + (1 << 4);

	/* filling operation info field */
	prEhtOpInfo = (struct EHT_OP_INFO *) prEhtOp->aucVarInfo;

	/* fixed field in operation info */
	prEhtOpInfo->ucControl = ehtRlmGetEhtOpBwByBssOpBw(eht_bw);
	prEhtOpInfo->ucCCFS0 = nicGetEhtS1(prAdapter, prBssInfo->eBand,
		prBssInfo->ucPrimaryChannel, rlmGetVhtOpBwByBssOpBw(eht_bw));
	prEhtOpInfo->ucCCFS1 = nicGetEhtS2(prAdapter, prBssInfo->eBand,
		prBssInfo->ucPrimaryChannel, rlmGetVhtOpBwByBssOpBw(eht_bw));
	u4OverallLen += 3;

	DBGLOG(RLM, INFO, "EHT channel width: %d\n",
		prEhtOpInfo->ucControl);

#if CFG_SUPPORT_802_PP_DSCB
	if (IS_BSS_APGO(prBssInfo) &&
		(EHT_IS_OP_PARAM_OP_INFO_PRESENT(prEhtOp->ucEhtOpParams))) {

		if (prBssInfo->fgIsEhtDscbPresent) {
			EHT_SET_OP_PARAM_DIS_SUBCHANNEL_PRESENT(
				prEhtOp->ucEhtOpParams);
			prEhtOpInfo->u2EhtDisSubChanBitmap =
				prBssInfo->u2EhtDisSubChanBitmap;
			u4OverallLen += 2;
		} else {
			EHT_RESET_OP_PARAM_DIS_SUBCHANNEL_PRESENT(
				prEhtOp->ucEhtOpParams);
			prEhtOpInfo->u2EhtDisSubChanBitmap = 0;
		}
		u4OverallLen += 2;
	}
#endif

	prEhtOp->ucLength = u4OverallLen - ELEM_HDR_LEN;

	prMsduInfo->u2FrameLength += IE_SIZE(prEhtOp);
}

void ehtRlmRspGenerateOpIEImpl(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucPhyTypeSet;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	if (!IS_BSS_ACTIVE(prBssInfo))
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	/* Decide PHY type set source */
	if (prStaRec) {
		/* Get PHY type set from target STA */
		ucPhyTypeSet = prStaRec->ucPhyTypeSet;
	} else {
		/* Get PHY type set from current BSS */
		ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
	}

	if (RLM_NET_IS_11BE(prBssInfo) &&
	    (ucPhyTypeSet & PHY_TYPE_SET_802_11BE))
		ehtRlmFillOpIE(prAdapter, prBssInfo, prMsduInfo);
}

void ehtRlmRspGenerateOpIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	if (prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_HIDE_INFO)
		return;

	ehtRlmRspGenerateOpIEImpl(prAdapter, prMsduInfo);
}


static void ehtRlmRecMcsMap(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct IE_EHT_CAP *prEhtCap)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucMaxBw;
	uint8_t *pos;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "prBssInfo is null\n");
		return;
	}
	pos = prEhtCap->aucVarInfo;
	ucMaxBw = heRlmPeerMaxBwCap(prStaRec->ucHePhyCapInfo);

	kalMemZero((void *) prStaRec->aucMcsMap20MHzSta,
		sizeof(struct EHT_SUPPORTED_MCS_BW20_FIELD));
	kalMemZero((void *) prStaRec->aucMcsMap80MHz,
		sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD));
	kalMemZero((void *) prStaRec->aucMcsMap160MHz,
		sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD));
	kalMemZero((void *) prStaRec->aucMcsMap320MHz,
		sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD));

	if (ucMaxBw == MAX_BW_20MHZ) {
		/* 20 MHz-Only Non-AP STA */
		if (IS_BSS_APGO(prBssInfo))
			kalMemCopy(prStaRec->aucMcsMap20MHzSta, pos,
				sizeof(struct EHT_SUPPORTED_MCS_BW20_FIELD));
		else
			kalMemCopy(prStaRec->aucMcsMap80MHz, pos,
			   sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD));
		return;
        }

	/* BW <= 80 MHz, Except 20 MHz-Only Non-AP STA */
	kalMemCopy(prStaRec->aucMcsMap80MHz, pos,
		sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD));
	pos += sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);

	if (ucMaxBw >= MAX_BW_160MHZ) {
		kalMemCopy(prStaRec->aucMcsMap160MHz, pos,
			sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD));
		pos += sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);
	}
	if ((*prEhtCap->ucEhtPhyCap) & DOT11BE_PHY_CAP_320M_6G) {
		kalMemCopy(prStaRec->aucMcsMap320MHz, pos,
			sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD));
		pos += sizeof(struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD);
	}
}

void ehtRlmRecCapInfo(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	const uint8_t *pucIE)
{
	struct IE_EHT_CAP *prEhtCap = (struct IE_EHT_CAP *) pucIE;

	if (!(prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_BIT_EHT))
		return;

	/* if payload not contain any aucVarInfo,
	 * IE size = sizeof(struct IE_EHT_CAP)
	 */
	if (IE_SIZE(prEhtCap) < (sizeof(struct IE_EHT_CAP))) {
		DBGLOG(SCN, WARN,
			"EHT_CAP IE_LEN err(%d)!\n", IE_LEN(prEhtCap));
		return;
	}

	DBGLOG(RLM, TRACE, "\n");

	memcpy(prStaRec->ucEhtMacCapInfo, prEhtCap->ucEhtMacCap,
		EHT_MAC_CAP_BYTE_NUM);
	memcpy(prStaRec->ucEhtPhyCapInfo, prEhtCap->ucEhtPhyCap,
		EHT_PHY_CAP_BYTE_NUM);
	ehtRlmRecMcsMap(prAdapter, prStaRec, prEhtCap);
}

void ehtRlmRecOperation(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	struct BSS_INFO *prBssInfo, const uint8_t *pucIE)
{
	struct IE_EHT_OP *prEhtOp = (struct IE_EHT_OP *) pucIE;
	struct EHT_OP_INFO *prEhtOpInfo;
	uint8_t ucVhtOpBw = 0;

	if (!(prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_BIT_EHT))
		return;

	/* if payload not contain any aucVarInfo,
	 * IE size = sizeof(struct IE_EHT_OP)
	 */
	if (IE_SIZE(prEhtOp) < (sizeof(struct IE_EHT_OP))) {
		DBGLOG(SCN, WARN,
			"HE_OP IE_LEN err(%d)!\n", IE_LEN(prEhtOp));
		return;
	}

	prBssInfo->ucEhtOpParams = prEhtOp->ucEhtOpParams;
	prBssInfo->u4BasicEhtMcsNssSet = prEhtOp->u4BasicEhtMcsNssSet;

	if (EHT_IS_OP_PARAM_OP_INFO_PRESENT(prEhtOp->ucEhtOpParams)) {
		prBssInfo->fgIsEhtOpPresent = TRUE;
		prEhtOpInfo = (struct EHT_OP_INFO *) prEhtOp->aucVarInfo;
		ucVhtOpBw = ehtRlmGetVhtOpBwByEhtOpBw(prEhtOpInfo);
		if (ucVhtOpBw == VHT_MAX_BW_INVALID) {
			DBGLOG(RLM, WARN, "invalid Bss OP BW, control: %d\n",
				prEhtOpInfo->ucControl);
			return;
		}

		prBssInfo->ucVhtChannelWidth = ucVhtOpBw;
		prBssInfo->ucVhtChannelFrequencyS1 = nicGetS1(prAdapter,
			prBssInfo->eBand, prBssInfo->ucPrimaryChannel,
			prBssInfo->ucVhtChannelWidth);
		prBssInfo->ucVhtChannelFrequencyS2 = 0;
		prBssInfo->ucEhtCtrl = prEhtOpInfo->ucControl;
		prBssInfo->ucEhtCcfs0 = prEhtOpInfo->ucCCFS0;
		prBssInfo->ucEhtCcfs1 = prEhtOpInfo->ucCCFS1;

		DBGLOG(RLM, TRACE,
			"EHT channel width: %d, s1 %d and s2 %d in IE -> s1 %d and s2 %d in driver\n",
			prBssInfo->ucVhtChannelWidth,
			prEhtOpInfo->ucCCFS0,
			prEhtOpInfo->ucCCFS1,
			prBssInfo->ucVhtChannelFrequencyS1,
			prBssInfo->ucVhtChannelFrequencyS2);
	} else {
		prBssInfo->fgIsEhtOpPresent = FALSE;
	}

	/*Backup peer VHT OpInfo*/
	prStaRec->ucVhtOpChannelWidth = prBssInfo->ucVhtChannelWidth;

#if CFG_SUPPORT_802_PP_DSCB

	if (EHT_IS_OP_PARAM_DIS_SUBCHANNEL_PRESENT(prEhtOp->ucEhtOpParams))
		prBssInfo->fgIsEhtDscbPresent = TRUE;
	else
		prBssInfo->fgIsEhtDscbPresent = FALSE;

	DBGLOG(RLM, LOUD, "RlmEHTOpInfo-0x%x\n",
		prBssInfo->ucEhtOpParams);

	if (prBssInfo->fgIsEhtOpPresent &&
	    prBssInfo->fgIsEhtDscbPresent) {
		struct EHT_DSCB_INFO *prEhtDscbInfo = NULL;
		uint32_t u4EhtOffset;

		/* struct IE_EHT_OP is packed,
		 * save to use sizeof instead of
		 * using OFFSET_OF with ZERO array
		 * at end
		 */
		u4EhtOffset = OFFSET_OF(struct IE_EHT_OP, aucVarInfo[0]) +
			OFFSET_OF(struct EHT_OP_INFO, u2EhtDisSubChanBitmap);
		prEhtDscbInfo = (struct EHT_DSCB_INFO *)
			(((uint8_t *) pucIE) + u4EhtOffset);
		prBssInfo->u2EhtDisSubChanBitmap =
			prEhtDscbInfo->u2DisSubChannelBitmap;
	} else if (prBssInfo->fgIsEhtDscbPresent) {
		struct EHT_DSCB_INFO *prEhtDscbInfo = NULL;
		uint32_t u4EhtOffset;

		/* struct IE_EHT_OP is packed,
		 * save to use sizeof instead of
		 * using OFFSET_OF with ZERO array
		 * at end
		 */
		u4EhtOffset = OFFSET_OF(struct IE_EHT_OP, aucVarInfo[0]);
		prEhtDscbInfo = (struct EHT_DSCB_INFO *)
			(((uint8_t *) pucIE) + u4EhtOffset);
		prBssInfo->u2EhtDisSubChanBitmap =
			prEhtDscbInfo->u2DisSubChannelBitmap;
	} else {
		/* prBssInfo->fgIsEhtDscbPresent == 0
		 * No dscb IE in beacon IE, so init DSCB as 0
		*/
		prBssInfo->u2EhtDisSubChanBitmap = 0;
	}

	DBGLOG(RLM, INFO, "DscbBitmap: 0x%x\n",
		prBssInfo->u2EhtDisSubChanBitmap);

#endif
}

void ehtRlmInit(
	struct ADAPTER *prAdapter)
{
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return ucVhtOpBw 0:20M/40Hz, 1:80MHz, 2:160MHz, 4:320MHz
 * Note: 80+80MHz is not used for EHT op BW
 *
 */
/*----------------------------------------------------------------------------*/

uint8_t ehtRlmGetVhtOpBwByEhtOpBw(struct EHT_OP_INFO *op)
{
	uint8_t ucVhtOpBw;
	uint8_t ucBssOpBw = op->ucControl & BITS(0, 2);
	uint8_t ucS1 = op->ucCCFS1;

	switch (ucBssOpBw) {
	case EHT_MAX_BW_20:
	case EHT_MAX_BW_40:
		ucVhtOpBw = VHT_OP_CHANNEL_WIDTH_20_40;
		break;

	case EHT_MAX_BW_80:
		ucVhtOpBw = VHT_OP_CHANNEL_WIDTH_80;
		break;

	case EHT_MAX_BW_160:
		ucVhtOpBw = VHT_OP_CHANNEL_WIDTH_160;
		break;

	case EHT_MAX_BW_320:
		ucVhtOpBw = rlmGetVhtOpBw320ByS1(ucS1);
		break;
	default:
		DBGLOG(RLM, WARN, "unexpected Bss OP BW: %d\n", ucBssOpBw);
		ucVhtOpBw = VHT_MAX_BW_INVALID;
		break;
	}

	return ucVhtOpBw;
}

void ehtRlmInitHtcACtrlOM(struct ADAPTER *prAdapter)
{
	prAdapter->fgEhtHtcOM = TRUE;
	prAdapter->u4HeHtcOM = 0;
	EHT_SET_HTC_HE_VARIANT(prAdapter->u4HeHtcOM);
	EHT_SET_HTC_1ST_A_CTRL_ID(prAdapter->u4HeHtcOM, HTC_EHT_A_CTRL_OM);
	EHT_SET_HTC_EHT_OM_RX_NSS_EXT(prAdapter->u4HeHtcOM, 0);
	EHT_SET_HTC_EHT_OM_TX_NSTS_EXT(prAdapter->u4HeHtcOM, 0);
	EHT_SET_HTC_EHT_OM_CH_WIDTH_EXT(prAdapter->u4HeHtcOM, 0);
	EHT_SET_HTC_2ND_A_CTRL_ID(prAdapter->u4HeHtcOM, HTC_HE_A_CTRL_OM);
	EHT_SET_HTC_HE_OM_RX_NSS(prAdapter->u4HeHtcOM, 1);
	EHT_SET_HTC_HE_OM_TX_NSTS(prAdapter->u4HeHtcOM, 1);
	EHT_SET_HTC_HE_OM_CH_WIDTH(prAdapter->u4HeHtcOM, CH_BW_80);
	EHT_SET_HTC_HE_OM_UL_MU_DISABLE(prAdapter->u4HeHtcOM, 0);
	EHT_SET_HTC_HE_OM_UL_MU_DATA_DISABLE(prAdapter->u4HeHtcOM, 0);
}

#endif /* CFG_SUPPORT_802_11BE == 1 */
