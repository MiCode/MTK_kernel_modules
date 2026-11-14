// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "he_rlm.c"
*    \brief This file contains HE Phy processing routines.
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

#if (CFG_SUPPORT_802_11AX == 1)
#include "he_rlm.h"

#if (CFG_SUPPORT_BTWT == 1)
#include "twt_planner.h"
#endif
/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*
 * Ignore PPE Threshold (Optional) currently
 * PPE threshold should be filled based on HW RX capability
 */
#define CFG_RX_PPE_THRESHOLD              1

/*
 * NSS: 3 bits
 * RU Index Bitmask: 4 bits
 * PPET8 & PPET16 info for 2NSS and 4 RU index size: 6 * 2 * 4 = 48 bits
 * PPE pad: 1 bit
 * Total = 56 bits = 7 bytes
 */
#define MAX_PPE_THRESHOLD_LEN_2NSS  7
#define MAX_PPE_THRESHOLD_LEN_1NSS  4

#if (CFG_SUPPORT_HE_ER == 1)
#define MAX_SUPPORT_CONSTELLATION_DCM_QPSK 2
#endif

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
/* TBD: where/what is the proper place/way to store this config */
uint8_t g_au8RlmHeCfgContellIdx[4][4][2] = {
	/* NSS1 */
	{{CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 0 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 1 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 2 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE} },	/* RU idx 3 */
	/* NSS2 */
	{{CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 0 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 1 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 2 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE} },	/* RU idx 3 */
	/* NSS3 */
	{{CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 0 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 1 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 2 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE} },	/* RU idx 3 */
	/* NSS4 */
	{{CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 0 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 1 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE},	/* RU idx 2 */
	 {CONSTELL_IDX_BPSK, CONSTELL_IDX_NONE} }	/* RU idx 3 */
};

#if (CFG_SUPPORT_802_11AX == 1)
#define TESTMODE_FORCE_DISABLE 0xFE
uint8_t  g_ucHtSMPSCapValue = TESTMODE_FORCE_DISABLE;
uint8_t  g_ucHeSMPSCapValue = TESTMODE_FORCE_DISABLE;
uint8_t  g_ucHtSMPS6GCapValue = TESTMODE_FORCE_DISABLE;
uint8_t  g_fgSigmaCMDHt = TESTMODE_FORCE_DISABLE;
uint8_t  g_fgSigmaCMDHe = TESTMODE_FORCE_DISABLE;
uint8_t  g_fgSigmaCMD6g = TESTMODE_FORCE_DISABLE;
#endif

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define PPET_SUBFIELD_MASK 0x7
#define BIT64(n)                ((u_int64_t) 1UL << (n))
#define BITS64(m, n)            (~(BIT64(m)-1) & ((BIT64(n) - 1) | BIT64(n)))

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
#if (CFG_SUPPORT_WIFI_6G == 1)
static void heRlmFillHe6gBandCapIE(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo, struct MSDU_INFO *prMsduInfo);
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
uint8_t heRlmMaxBwToHeBw(uint8_t ucMaxBw)
{
	uint8_t ucHeBw = HE_OP_CHANNEL_WIDTH_20;

	switch (ucMaxBw) {
	case MAX_BW_20MHZ:
		ucHeBw = HE_OP_CHANNEL_WIDTH_20;
		break;
	case MAX_BW_40MHZ:
		ucHeBw = HE_OP_CHANNEL_WIDTH_40;
		break;
	case MAX_BW_80MHZ:
		ucHeBw = HE_OP_CHANNEL_WIDTH_80;
		break;
	case MAX_BW_160MHZ:
	case MAX_BW_80_80_MHZ:
		ucHeBw = HE_OP_CHANNEL_WIDTH_80P80_160;
		break;
	default:
		break;
	}

	return ucHeBw;
}

uint32_t heRlmCalculateHeCapIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec)
{
	uint8_t ucMaxBw;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	uint32_t u4OverallLen = OFFSET_OF(struct _IE_HE_CAP_T, aucVarInfo[0]);

	if (fgEfuseCtrlAxOn == 1) {

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "prBssInfo is null\n");
		return u4OverallLen;
	}
	ucMaxBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);

	u4OverallLen += 4;

	if (ucMaxBw >= MAX_BW_160MHZ)
		u4OverallLen += 4;

	if (ucMaxBw >= MAX_BW_80_80_MHZ)
		u4OverallLen += 4;

	//TODO: check 320MHZ

#if (CFG_RX_PPE_THRESHOLD == 1)
	u4OverallLen += sizeof(struct _PPE_THRESHOLD_FIELD);
#endif
	} else {
		u4OverallLen = 0;
	}

	return u4OverallLen;
}

uint32_t heRlmCalculateHeOpIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec)
{
	uint32_t u4OverallLen = OFFSET_OF(struct _IE_HE_OP_T, aucVarInfo[0]);

	return u4OverallLen;
}

static uint8_t heRlmGetHeSupportedMcs(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	switch (prBssInfo->eBand) {
	case BAND_2G4:
		return  prAdapter->rWifiVar.ucHeMaxMcsMap2g;
	case BAND_5G:
		return prAdapter->rWifiVar.ucHeMaxMcsMap5g;
#if (CFG_SUPPORT_WIFI_6G == 1)
	case BAND_6G:
		return  prAdapter->rWifiVar.ucHeMaxMcsMap6g;
#endif
	default:
		return HE_CAP_INFO_MCS_NOT_SUPPORTED;
	}
}

static void heRlmFillMCSMap(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct _HE_SUPPORTED_MCS_FIELD *prHeSupportedMcsSet)
{
	uint8_t i, ucSupportedNss;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	kalMemZero((void *) prHeSupportedMcsSet,
		sizeof(struct _HE_SUPPORTED_MCS_FIELD));
	ucSupportedNss = wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);

	for (i = 0; i < 8; i++) {
		uint8_t ucOffset = i * 2;
		uint8_t ucMcsMap;

		if (i < ucSupportedNss) {
			if (prAdapter->fgMcsMapBeenSet & SET_HE_MCS_MAP) {
				ucMcsMap = kal_min_t(uint8_t,
					prAdapter->ucMcsMapSetFromSigma,
					HE_CAP_INFO_MCS_MAP_MCS11);
			} else if (IS_BSS_AIS(prBssInfo) &&
				 prWifiVar->ucStaMaxMcsMap != 0xFF) {
				ucMcsMap = kal_min_t(uint8_t,
					prWifiVar->ucStaMaxMcsMap,
					heRlmGetHeSupportedMcs(prAdapter,
								prBssInfo));
			} else
				ucMcsMap = heRlmGetHeSupportedMcs(prAdapter,
								prBssInfo);

			DBGLOG(RLM, TRACE, "Supported MCS Map: %u", ucMcsMap);
		} else {
			ucMcsMap = HE_CAP_INFO_MCS_NOT_SUPPORTED;
		}
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
		if (p2pFuncGetForceTrxConfig(prAdapter,
				prBssInfo->ucBssIndex) !=
			P2P_FORCE_TRX_CONFIG_NONE &&
			prBssInfo->ucOpTxNss <= i)
			ucMcsMap = VHT_CAP_INFO_MCS_NOT_SUPPORTED;
#endif
#endif
		prHeSupportedMcsSet->u2TxMcsMap |= (ucMcsMap << ucOffset);
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
		if (p2pFuncGetForceTrxConfig(prAdapter,
				prBssInfo->ucBssIndex) !=
			P2P_FORCE_TRX_CONFIG_NONE &&
			prBssInfo->ucOpRxNss <= i)
			ucMcsMap = VHT_CAP_INFO_MCS_NOT_SUPPORTED;
#endif
#endif
		prHeSupportedMcsSet->u2RxMcsMap |= (ucMcsMap << ucOffset);
	}

	prHeSupportedMcsSet->u2RxMcsMap =
		CPU_TO_LE32(prHeSupportedMcsSet->u2RxMcsMap);
	prHeSupportedMcsSet->u2TxMcsMap =
		CPU_TO_LE32(prHeSupportedMcsSet->u2TxMcsMap);
}

/*
 * ucBitPos: bit position (starting from 0) for setting 3-bit PPE value
 * ucVal has to be 3-bit value (i.e. no greater than 7)
 */
static void heRlmSet3BitPPE(uint8_t *pMem, uint8_t ucBitPos, uint8_t ucVal)
{
	uint8_t ucValidBits, ucPosInCurByte;
	uint8_t *pucCurByte = pMem + (ucBitPos >> 3);

	ucPosInCurByte = (ucBitPos & 0x7);
	ucValidBits = 8 - ucPosInCurByte;
	if (ucValidBits >= 3) {
		(*pucCurByte) |= (ucVal  << ucPosInCurByte);
	} else {
		(*pucCurByte) |= (((uint8_t)(ucVal << ucPosInCurByte)) &
			BITS(ucPosInCurByte, 7));
		(*(pucCurByte + 1)) |= (ucVal >> ucValidBits);
	}
}

/* ucBitPos: bit position (starting from 0) for getting 3-bit PPE value */
static uint8_t heRlmGet3BitPPE(uint8_t *pMem, uint8_t ucBitPos)
{
	/* ucValidBits: valid bit counts in current byte */
	uint8_t ucValidBits, ucPosInCurByte, ucPPE;
	uint8_t *pucCurByte;

	/* Get the pointer of the byte that contains the ucBitPos */
	pucCurByte = pMem + (ucBitPos >> 3);  /* divided by 8 */
	ucPosInCurByte = (ucBitPos & 0x7);
	/* get the remaining bits in this byte */
	ucValidBits = 8 - ucPosInCurByte;
	if (ucValidBits >= 3) {	/* enough bits for getting 3-bit PPE */
		ucPPE = ((*pucCurByte) &
			BITS(ucPosInCurByte, ucPosInCurByte + 2)) >>
			ucPosInCurByte;
	} else {
		uint8_t ucLSb, ucMSb;

		ucLSb = ((*pucCurByte) & BITS(ucPosInCurByte, 7)) >>
			ucPosInCurByte;
		ucMSb = (*(pucCurByte + 1)) & BITS(0, 2 - ucValidBits);
		ucPPE = ucLSb | (ucMSb << ucValidBits);
	}

	return ucPPE;
}

/* Note: ucSupportedNss is (actual NSS - 1) */
static uint8_t heRlmFillPPEThresholdInfo(
	uint8_t ucSupportedNss,
	uint8_t ucRUIdxSize,
	uint8_t *pMem
)
{
	uint8_t ucNssIdx, ucRUIdx;
	uint8_t ucT16Pos = HE_CAP_PPE_PPET16_NSS1_RU0_SHFT;
	uint8_t ucT8Pos = HE_CAP_PPE_PPET8_NSS1_RU0_SHFT;
	uint8_t ucMaxSupportNss = (sizeof(g_au8RlmHeCfgContellIdx) /
				sizeof(g_au8RlmHeCfgContellIdx[0]));

	/* Coverity:
	 * due to ucSupportedNss is Supported Nss - 1
	 * if it > first dimention of g_au8RlmHeCfgContellIdx - 1
	 * assign it to first dimention of g_au8RlmHeCfgContellIdx - 1
	 */
	ucSupportedNss = (ucSupportedNss > (ucMaxSupportNss - 1)) ?
				(ucMaxSupportNss - 1) : ucSupportedNss;

	for (ucNssIdx = 0; ucNssIdx <= ucSupportedNss; ucNssIdx++) {
		for (ucRUIdx = 0; ucRUIdx < ucRUIdxSize; ucRUIdx++) {
			heRlmSet3BitPPE(pMem, ucT16Pos,
				g_au8RlmHeCfgContellIdx[ucNssIdx][ucRUIdx][0]);
			heRlmSet3BitPPE(pMem, ucT8Pos,
				g_au8RlmHeCfgContellIdx[ucNssIdx][ucRUIdx][1]);
			ucT16Pos += PPE_SUBFIELD_BITS_NUM;
			ucT8Pos += PPE_SUBFIELD_BITS_NUM;
		}
	}

	return ucT16Pos;
}

static uint8_t heRlmFillPPEThreshold(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t *pPPEThreshold)
{
	uint8_t ucRUIdxSize = 0, ucLen;
	uint8_t ucMaxBw =
		cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);
	uint8_t ucSupportedNss =
		wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);

	/* Fix coverity issue :
	 * if wlanGetSupportNss() returns 0, -1 will overflow.
	 * Because ucSupportedNss is unsigned int.
	 */
	if (ucSupportedNss > 0)
		ucSupportedNss -= 1;
	else
		ucSupportedNss = 0;

	kalMemZero((void *) pPPEThreshold, sizeof(struct _PPE_THRESHOLD_FIELD));

	/* Note: this field in spec is (actual NSS - 1) */
	(*pPPEThreshold) = ucSupportedNss;

	switch (ucMaxBw) {
	case MAX_BW_20MHZ:
	case MAX_BW_40MHZ:
		(*pPPEThreshold) |= HE_CAP_PPE_484_RU_IDX;
		ucRUIdxSize = 2;
		break;
	case MAX_BW_80MHZ:
		(*pPPEThreshold) |= HE_CAP_PPE_996_RU_IDX;
		ucRUIdxSize = 3;
		break;
	case MAX_BW_160MHZ:
	case MAX_BW_80_80_MHZ:
		(*pPPEThreshold) |= HE_CAP_PPE_996X2_RU_IDX;
		ucRUIdxSize = 4;
		break;
	case MAX_BW_320_1MHZ:
	case MAX_BW_320_2MHZ:
		//TODO: check spec
		(*pPPEThreshold) |= HE_CAP_PPE_996X2_RU_IDX;
		ucRUIdxSize = 4;
		break;
	default:
		/* should never come here */
		ASSERT(0);
	}

	ucLen = heRlmFillPPEThresholdInfo(ucSupportedNss,
		ucRUIdxSize, pPPEThreshold);

	ucLen = (ucLen % 8) ? (ucLen / 8 + 1) : (ucLen / 8);

	return ucLen;
}

void heRlmFillHeCapIE(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct MSDU_INFO *prMsduInfo)
{
	struct _IE_HE_CAP_T *prHeCap;
	struct _HE_SUPPORTED_MCS_FIELD *prHeSupportedMcsSet;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4OverallLen = OFFSET_OF(struct _IE_HE_CAP_T, aucVarInfo[0]);
#if (CFG_SUPPORT_BFEE == 1)
	u_int8_t fgBfEn = TRUE;
#if (CFG_SUPPORT_CONDITIONAL_BFEE == 1)
	uint32_t soundingDim = 0;
#endif
#endif
	uint8_t ucMaxBw;

	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#if (CFG_RX_PPE_THRESHOLD == 1)
	uint8_t *pPPEThreshold;
#endif
#if ((CFG_SUPPORT_BFEE == 1) || (CFG_SUPPORT_HE_ER == 1))
	uint8_t ucSupportedNss = 0;
#endif
	u_int8_t fgTxStbcEn = TRUE;

	struct AIS_FSM_INFO *prAisFsmInfo = NULL;
	struct BSS_DESC *prBssDesc = NULL;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

#if ((CFG_SUPPORT_BFEE == 1) || (CFG_SUPPORT_HE_ER == 1))
	ucSupportedNss =
		wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);
	/* Fix coverity issue :
	 * if wlanGetSupportNss() returns 0, -1 will overflow.
	 * Because ucSupportedNss is unsigned int.
	 */
	if (ucSupportedNss > 0)
		ucSupportedNss -= 1;
	else
		ucSupportedNss = 0;
#endif

	prChipInfo = prAdapter->chip_info;

	prHeCap = (struct _IE_HE_CAP_T *)
		(((uint8_t *)prMsduInfo->prPacket)+prMsduInfo->u2FrameLength);

	prHeCap->ucId = ELEM_ID_RESERVED;
	prHeCap->ucExtId = ELEM_EXT_ID_HE_CAP;

	ucMaxBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);
#if CFG_SUPPORT_TDLS_ADJUST_BW
	ucMaxBw = TdlsAdjustBw(prAdapter,
			cnmGetStaRecByIndex(prAdapter,
			prMsduInfo->ucStaRecIndex),
			prBssInfo->ucBssIndex,
			ucMaxBw);
#endif

	/* MAC capabilities */
	HE_RESET_MAC_CAP(prHeCap->ucHeMacCap);

	HE_SET_MAC_CAP_TRIGGER_PAD_DURATION(prHeCap->ucHeMacCap,
		prWifiVar->ucTrigMacPadDur);

	if (IS_FEATURE_ENABLED(prWifiVar->ucHeHTC)) {
		/* Check HTC blocklist */
		if (IS_BSS_AIS(prBssInfo)) {
			prAisFsmInfo = aisGetAisFsmInfo(prAdapter,
				prBssInfo->ucBssIndex);
			prBssDesc = aisGetTargetBssDesc(prAdapter,
					prBssInfo->ucBssIndex);
			if (prAisFsmInfo && prBssDesc != NULL &&
				queryAxBlocklist(prAdapter, prBssDesc->aucBSSID,
				   prBssInfo->ucBssIndex, BLOCKLIST_DIS_HE_HTC))
				DBGLOG(BSS, INFO,
				    "BSSID " MACSTR " is in HTC blocklist!\n",
				    MAC2STR(prBssDesc->aucBSSID));
			else
				HE_SET_MAC_CAP_HTC_HE(prHeCap->ucHeMacCap);
		} else {
			HE_SET_MAC_CAP_HTC_HE(prHeCap->ucHeMacCap);
		}
	}

	if (IS_FEATURE_ENABLED(prWifiVar->ucHeOMCtrl))
		HE_SET_MAC_CAP_OM_CTRL(prHeCap->ucHeMacCap);

	if (IS_FEATURE_ENABLED(prWifiVar->ucRxCtrlToMutiBss))
		HE_SET_MAC_CAP_RX_CTRL_TO_MUTI_BSS(prHeCap->ucHeMacCap);

	HE_SET_MAC_CAP_MAX_AMPDU_LEN_EXP(prHeCap->ucHeMacCap,
		prWifiVar->ucMaxAmpduLenExp);

	if (prBssInfo->eBand == BAND_2G4 &&
	    prChipInfo->is_specify_he_cap_max_ampdu_len_exp) {
		HE_SET_MAC_CAP_MAX_AMPDU_LEN_EXP(prHeCap->ucHeMacCap,
					  prChipInfo->uc2G4HeCapMaxAmpduLenExp);
	} else if (prBssInfo->eBand == BAND_5G &&
		   prChipInfo->is_specify_he_cap_max_ampdu_len_exp) {
		HE_SET_MAC_CAP_MAX_AMPDU_LEN_EXP(prHeCap->ucHeMacCap,
					   prChipInfo->uc5GHeCapMaxAmpduLenExp);
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prBssInfo->eBand == BAND_6G &&
		 prChipInfo->is_specify_he_cap_max_ampdu_len_exp) {
		HE_SET_MAC_CAP_MAX_AMPDU_LEN_EXP(prHeCap->ucHeMacCap,
					   prChipInfo->uc5GHeCapMaxAmpduLenExp);
	}
#endif

#if (CFG_SUPPORT_TWT == 1)
	if (IS_BSS_AIS(prBssInfo) &&
		IS_FEATURE_ENABLED(prWifiVar->ucTWTRequester))
		HE_SET_MAC_CAP_TWT_REQ(prHeCap->ucHeMacCap);

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	if (IS_FEATURE_ENABLED(prWifiVar->ucTWTResponder))
		HE_SET_MAC_CAP_TWT_RSP(prHeCap->ucHeMacCap);
#endif

#endif

#if (CFG_SUPPORT_BTWT == 1)
	if (IS_FEATURE_ENABLED(prWifiVar->ucBTWTSupport))
		HE_SET_MAC_CAP_BTWT_SUPT(prHeCap->ucHeMacCap);
#endif

#if (CFG_SUPPORT_802_11AX == 1)
	if ((g_ucHeSMPSCapValue == 5) &&
		(IS_FEATURE_ENABLED(prWifiVar->ucHeDynamicSMPS)))
		HE_SET_MAC_CAP_DYNAMIC_SMPS(prHeCap->ucHeMacCap);
	else if ((g_ucHeSMPSCapValue == 1) && (g_fgSigmaCMDHe == 1))
		HE_SET_MAC_CAP_DYNAMIC_SMPS(prHeCap->ucHeMacCap);
	else if ((g_ucHeSMPSCapValue == 0) && (g_fgSigmaCMDHe == 1))
		HE_UNSET_MAC_CAP_DYNAMIC_SMPS(prHeCap->ucHeMacCap);
#endif

	/* PHY capabilities */
	HE_RESET_PHY_CAP(prHeCap->ucHePhyCap);

	if (prBssInfo->eBand == BAND_2G4 && ucMaxBw >= MAX_BW_40MHZ) {
		if (IS_BSS_APGO(prBssInfo)) {
			if (prBssInfo->fgAssoc40mBwAllowed)
				HE_SET_PHY_CAP_CHAN_WIDTH_SET_BW40_2G(
					prHeCap->ucHePhyCap);
		} else {
			HE_SET_PHY_CAP_CHAN_WIDTH_SET_BW40_2G(
				prHeCap->ucHePhyCap);
		}
	} else if ((prBssInfo->eBand == BAND_5G)
#if (CFG_SUPPORT_WIFI_6G == 1)
		|| (prBssInfo->eBand == BAND_6G)
#endif
		) {
		if (ucMaxBw >= MAX_BW_40MHZ)
			HE_SET_PHY_CAP_CHAN_WIDTH_SET_BW40_BW80_5G(
				prHeCap->ucHePhyCap);

		if (ucMaxBw >= MAX_BW_160MHZ)
			HE_SET_PHY_CAP_CHAN_WIDTH_SET_BW160_5G(
				prHeCap->ucHePhyCap);

		/* 80+80 not supported, skip setting 80+80 for bw320 */
		if (ucMaxBw == MAX_BW_80_80_MHZ)
			HE_SET_PHY_CAP_CHAN_WIDTH_SET_BW80P80_5G(
				prHeCap->ucHePhyCap);
	}

	/* Set PUNCTURED_PREAMBLE_RX:
	 * BW80_sec20(bit[0]) and BW80_sec40(bit[1]) are supported
	 */
	if (IS_FEATURE_ENABLED(prWifiVar->ucStaHePpRx) &&
		(prWifiVar->ucSta5gBandwidth >= MAX_BW_80MHZ)) {
		HE_SET_PHY_CAP_PUNCTURED_PREAMBLE_RX(prHeCap->ucHePhyCap, 3);
	} else
		HE_SET_PHY_CAP_PUNCTURED_PREAMBLE_RX(prHeCap->ucHePhyCap, 0);

	if (IS_FEATURE_ENABLED(prWifiVar->ucRxLdpc) &&
		IS_FEATURE_ENABLED(prWifiVar->ucTxLdpc))
		HE_SET_PHY_CAP_LDPC_CODING_IN_PAYLOAD(prHeCap->ucHePhyCap);

	if (prBssInfo->ucOpTxNss < 2)
		fgTxStbcEn = FALSE;

	if (rlmCheckTxStbc(prAdapter, prBssInfo->ucBssIndex,
		FEATURE_ENABLED) &&
		fgTxStbcEn) {
		HE_SET_PHY_CAP_STBC_TX_LT_OR_EQ_80M(prHeCap->ucHePhyCap);
		if (ucMaxBw >= MAX_BW_160MHZ)
			HE_SET_PHY_CAP_STBC_TX_GT_80M(prHeCap->ucHePhyCap);
	}

	if (rlmCheckRxStbc(prAdapter,
		prBssInfo->ucBssIndex, FEATURE_ENABLED)) {
		HE_SET_PHY_CAP_STBC_RX_LT_OR_EQ_80M(prHeCap->ucHePhyCap);
		if (ucMaxBw >= MAX_BW_160MHZ)
			HE_SET_PHY_CAP_STBC_RX_GT_80M(prHeCap->ucHePhyCap);
	}

#if (CFG_SUPPORT_BFEE == 1)
#if (CFG_SUPPORT_CONDITIONAL_BFEE == 1)
	if (prAdapter->rWifiVar.u4SwTestMode != ENUM_SW_TEST_MODE_SIGMA_AX &&
	    prAdapter->rWifiVar.u4SwTestMode != ENUM_SW_TEST_MODE_SIGMA_BE &&
	    IS_BSS_AIS(prBssInfo)) {
		prBssDesc = aisGetTargetBssDesc(prAdapter,
			prBssInfo->ucBssIndex);
		if (prBssDesc != NULL) {
			soundingDim =
				HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW160_5G(
						prHeCap->ucHePhyCap) ?
				HE_GET_PHY_CAP5_NUM_OF_SND_DIM_GT_80M(
						prBssDesc->ucHePhyCapInfo) :
				HE_GET_PHY_CAP_NUM_OF_SND_DIM_LT_OR_EQ_80M(
						prBssDesc->ucHePhyCapInfo);

			DBGLOG(RLM, TRACE,
				"he ucSupportedNss: %d, soundingDim: %d\n",
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
	if ((fgBfEn == TRUE) && (IS_FEATURE_ENABLED(prWifiVar->ucStaHeBfee))) {
		HE_SET_PHY_CAP_NDP_4X_HE_LTF(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_SU_BFMEE(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_BFMEE_STS_LT_OR_EQ_80M(prHeCap->ucHePhyCap, 3);
		if (ucMaxBw >= MAX_BW_160MHZ) {
			HE_SET_PHY_CAP_BFMEE_STS_GT_80M(
				prHeCap->ucHePhyCap, 3);
		}
		HE_SET_PHY_CAP_CODE_BOOK_4_2_SU_FB(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_CODE_BOOK_7_5_MU_FB(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_TRIG_SU_BF_FB(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_TRIG_MU_BF_PARTIAL_BW_FB(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_TRIG_CQI_FB(prHeCap->ucHePhyCap);
		if (IS_FEATURE_ENABLED(prWifiVar->ucHeSUMU4xHeLTF))
			HE_SET_PHY_CAP_SU_MU_4X_HE_LTF(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_MAX_NC(prHeCap->ucHePhyCap, ucSupportedNss);
	}
#endif

#if (CFG_SUPPORT_BFER == 1)
	if (IS_FEATURE_ENABLED(prWifiVar->ucStaHeSuBfer)) {
		HE_SET_PHY_CAP_SU_BFMER(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_NUM_OF_SND_DIM_LT_OR_EQ_80M(prHeCap->ucHePhyCap, 1);

		if (ucMaxBw >= MAX_BW_160MHZ)
			HE_SET_PHY_CAP_NUM_OF_SND_DIM_GT_80M(
				prHeCap->ucHePhyCap, 1);
	}
#endif

	if (IS_BSS_APGO(prBssInfo)) {
		/* SAP not support UL MU MIMO */
		HE_UNSET_PHY_CAP_FULL_BW_UL_MU_MIMO(prHeCap->ucHePhyCap);
		HE_UNSET_PHY_CAP_PARTIAL_BW_UL_MU_MIMO(prHeCap->ucHePhyCap);
	}

#if (CFG_SUPPORT_HE_ER == 1)
	if (IS_FEATURE_ENABLED(prWifiVar->u4ExtendedRange)) {
		if (IS_FEATURE_ENABLED(prWifiVar->fgErTx)) {
			HE_SET_PHY_CAP_DCM_MAX_CONSTELLATION_TX(
				prHeCap->ucHePhyCap,
				MAX_SUPPORT_CONSTELLATION_DCM_QPSK);
			if (ucSupportedNss > 0)
				HE_SET_PHY_CAP_DCM_MAX_NSS_TX(
					prHeCap->ucHePhyCap);
		}

		if (IS_FEATURE_ENABLED(prWifiVar->fgErRx)) {
			HE_SET_PHY_CAP_DCM_MAX_CONSTELLATION_RX(
				prHeCap->ucHePhyCap,
				MAX_SUPPORT_CONSTELLATION_DCM_QPSK);
			if (ucSupportedNss > 0)
				HE_SET_PHY_CAP_DCM_MAX_NSS_RX(
					prHeCap->ucHePhyCap);
			if (IS_FEATURE_ENABLED(prWifiVar->fgErSuRx)) {
				HE_SET_PHY_CAP_ER_SU_4X_HE_LTF(
					prHeCap->ucHePhyCap);
				HE_SET_PHY_CAP_ER_SU_1X_HE_LTF(
					prHeCap->ucHePhyCap);
			}
		}

		if (IS_FEATURE_ENABLED(prWifiVar->fgErTx) ||
			IS_FEATURE_ENABLED(prWifiVar->fgErRx)) {
			HE_SET_PHY_CAP_PARTIAL_BW_EXTENDED_RANGE(
				prHeCap->ucHePhyCap);
			HE_SET_PHY_CAP_DCM_MAX_RU(prHeCap->ucHePhyCap,
				(ucMaxBw >= MAX_BW_160MHZ) ?
				MAX_BW_160MHZ : ucMaxBw);
		}

		DBGLOG(RLM, TRACE, "ER: Set ER Phy capabilities\n");
	}
#endif

	HE_SET_PHY_CAP_TX_1KQAM_242_TONE_RU(prHeCap->ucHePhyCap);
	HE_SET_PHY_CAP_RX_1KQAM_242_TONE_RU(prHeCap->ucHePhyCap);

	/* Set MCS map */
	prHeSupportedMcsSet = (struct _HE_SUPPORTED_MCS_FIELD *)
		(((uint8_t *) prHeCap) + u4OverallLen);
	heRlmFillMCSMap(prAdapter, prBssInfo, prHeSupportedMcsSet);
	u4OverallLen += sizeof(struct _HE_SUPPORTED_MCS_FIELD);

	if (ucMaxBw >= MAX_BW_160MHZ) {
		prHeSupportedMcsSet = (struct _HE_SUPPORTED_MCS_FIELD *)
			(((uint8_t *) prHeCap) + u4OverallLen);
		heRlmFillMCSMap(prAdapter, prBssInfo, prHeSupportedMcsSet);
		u4OverallLen += sizeof(struct _HE_SUPPORTED_MCS_FIELD);
	}

	/* 80+80 not supported, skip setting 80+80 for bw320 */
	if (ucMaxBw == MAX_BW_80_80_MHZ) {
		prHeSupportedMcsSet = (struct _HE_SUPPORTED_MCS_FIELD *)
			(((uint8_t *) prHeCap) + u4OverallLen);
		heRlmFillMCSMap(prAdapter, prBssInfo, prHeSupportedMcsSet);
		u4OverallLen += sizeof(struct _HE_SUPPORTED_MCS_FIELD);
	}

#if (CFG_RX_PPE_THRESHOLD == 1)
	pPPEThreshold = ((uint8_t *) prHeCap) + u4OverallLen;
	u4OverallLen +=
		heRlmFillPPEThreshold(prAdapter, prBssInfo, pPPEThreshold);
#endif
	/* The Element ID Extension byte is included in length calculation */
	prHeCap->ucLength = u4OverallLen - ELEM_HDR_LEN;

	prMsduInfo->u2FrameLength += IE_SIZE(prHeCap);
}

void heRlmReqGenerateHeCapIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;

	if (fgEfuseCtrlAxOn == 1) {

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11AX)
	    && (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11AX)))
		heRlmFillHeCapIE(prAdapter, prBssInfo, prMsduInfo);
	}
}

void heRlmRspGenerateHeCapIE(
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

	if (RLM_NET_IS_11AX(prBssInfo) &&
	    (ucPhyTypeSet & PHY_TYPE_SET_802_11AX))
		heRlmFillHeCapIE(prAdapter, prBssInfo, prMsduInfo);
}

void heRlmRspGenerateHeRnrIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
#if CFG_ENABLE_WIFI_DIRECT
	struct BSS_INFO *prBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo;
	uint8_t *pucBuffer;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (!prBssInfo || !IS_BSS_APGO(prBssInfo))
		return;

	prP2pSpecBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo[
		prBssInfo->u4PrivateData];
	if (!prP2pSpecBssInfo || prP2pSpecBssInfo->u2RnrIeLen == 0)
		return;

	pucBuffer = (uint8_t *) ((uintptr_t)prMsduInfo->prPacket +
		(uintptr_t)prMsduInfo->u2FrameLength);
	kalMemCopy(pucBuffer,
		   prP2pSpecBssInfo->aucRnrIeBuffer,
		   prP2pSpecBssInfo->u2RnrIeLen);
	prMsduInfo->u2FrameLength += prP2pSpecBssInfo->u2RnrIeLen;

	DBGLOG(RSN, INFO, "[%d] Keep supplicant RNR IE content w/o update\n",
		prBssInfo->ucBssIndex);
#endif
}

static void heRlmFillHeOpIE(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct MSDU_INFO *prMsduInfo)
{
	struct _IE_HE_OP_T *prHeOp;
	uint32_t u4Offset = OFFSET_OF(struct _IE_HE_OP_T, aucVarInfo[0]);
#if (CFG_SUPPORT_WIFI_6G == 1)
	struct _6G_OPER_INFOR_T *pr6gOperInfor = NULL;
#endif

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	prHeOp = (struct _IE_HE_OP_T *)
		(((uint8_t *)prMsduInfo->prPacket)+prMsduInfo->u2FrameLength);

	prHeOp->ucId = ELEM_ID_RESERVED;
	prHeOp->ucExtId = ELEM_EXT_ID_HE_OP;

	memcpy(prHeOp->ucHeOpParams, prBssInfo->ucHeOpParams, HE_OP_BYTE_NUM);
	prHeOp->ucBssColorInfo = prBssInfo->ucBssColorInfo;
	prHeOp->u2HeBasicMcsSet = CPU_TO_LE16(prBssInfo->u2HeBasicMcsSet);

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (IS_BSS_APGO(prBssInfo) &&
		HE_IS_6G_OP_INFOR_PRESENT(prBssInfo->ucHeOpParams)) {

		if (HE_IS_CO_HOSTED_BSS(prBssInfo->ucHeOpParams))
			u4Offset += sizeof(uint8_t);

		pr6gOperInfor = (struct _6G_OPER_INFOR_T *)
			(((uint8_t *) prHeOp) + u4Offset);

		memcpy(pr6gOperInfor, &prBssInfo->r6gOperInfor,
			sizeof(struct _6G_OPER_INFOR_T));

		u4Offset += sizeof(struct _6G_OPER_INFOR_T);
	}
#endif

	prHeOp->ucLength = u4Offset - ELEM_HDR_LEN;
	prMsduInfo->u2FrameLength += IE_SIZE(prHeOp);
}

void heRlmRspGenerateHeOpIE(
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

	if (RLM_NET_IS_11AX(prBssInfo) &&
	    (ucPhyTypeSet & PHY_TYPE_SET_802_11AX))
		heRlmFillHeOpIE(prAdapter, prBssInfo, prMsduInfo);
}

#if (CFG_SUPPORT_WIFI_6G == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static void heRlmFillHe6gBandCapIE(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo,
			    struct MSDU_INFO *prMsduInfo)
{
	struct _IE_HE_6G_BAND_CAP_T *prHe6gBandCap;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	prHe6gBandCap = (struct _IE_HE_6G_BAND_CAP_T *)
		(((uint8_t *)prMsduInfo->prPacket) +
			prMsduInfo->u2FrameLength);

	prHe6gBandCap->ucId = ELEM_ID_RESERVED;
	prHe6gBandCap->ucLength =
		sizeof(struct _IE_HE_6G_BAND_CAP_T) - ELEM_HDR_LEN;
	prHe6gBandCap->ucExtId = ELEM_EXT_ID_HE_6G_BAND_CAP;

	prHe6gBandCap->u2CapInfo = HE_6G_CAP_INFO_DEFAULT_VAL;

	prHe6gBandCap->u2CapInfo |= ((prAdapter->rWifiVar.ucRxMaxMpduLen
			<< HE_6G_CAP_INFO_MAX_MPDU_LEN_OFFSET) &
			   HE_6G_CAP_INFO_MAX_MPDU_LEN_MASK);

	if (prBssInfo->ucOpRxNss <
		wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex)) {
		/*Set as static power save*/
		prHe6gBandCap->u2CapInfo &=
			~HE_6G_CAP_INFO_SM_POWER_SAVE;
	}

#if (CFG_SUPPORT_802_11AX == 1)
	if ((g_ucHtSMPS6GCapValue == 5) && (g_fgHTSMPSEnabled == 0xFF)) {
		prHe6gBandCap->u2CapInfo |= HE_6G_CAP_INFO_SM_POWER_SAVE;
	} else if ((g_ucHtSMPS6GCapValue == 5) && (g_fgHTSMPSEnabled == 1)) {
		prHe6gBandCap->u2CapInfo &=
			(~HE_6G_CAP_INFO_SM_POWER_SAVE);
		prHe6gBandCap->u2CapInfo |=
		(1 << HE_6G_CAP_INFO_SM_POWER_SAVE_OFFSET);
	} else if (g_ucHtSMPS6GCapValue == 3 && g_fgSigmaCMD6g == 1)
		prHe6gBandCap->u2CapInfo |= HE_6G_CAP_INFO_SM_POWER_SAVE;
	else if (g_ucHtSMPS6GCapValue == 1 && g_fgSigmaCMD6g == 1) {
		prHe6gBandCap->u2CapInfo &=
			(~HE_6G_CAP_INFO_SM_POWER_SAVE);
		(prHe6gBandCap->u2CapInfo) |=
			(1 << HE_6G_CAP_INFO_SM_POWER_SAVE_OFFSET);
	}
#endif

	if (prBssInfo->eBand == BAND_6G &&
	    prAdapter->rWifiVar.ucHtSmps6g == 1) {
		prHe6gBandCap->u2CapInfo &=
			(~HE_6G_CAP_INFO_SM_POWER_SAVE);
		(prHe6gBandCap->u2CapInfo) |=
			(1 << HE_6G_CAP_INFO_SM_POWER_SAVE_OFFSET);
	} else if (prBssInfo->eBand == BAND_6G &&
		   prAdapter->rWifiVar.ucHtSmps6g == 3) {
		prHe6gBandCap->u2CapInfo |=
			HE_6G_CAP_INFO_SM_POWER_SAVE;
	}

	ASSERT(IE_SIZE(prHe6gBandCap) <=
		(ELEM_HDR_LEN + ELEM_MAX_LEN_HE_6G_CAP));

	prMsduInfo->u2FrameLength += IE_SIZE(prHe6gBandCap);
}

void heRlmReqGenerateHe6gBandCapIE(
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

	if (((prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11AX)
	    && (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11AX)))
	    && (prBssInfo->eBand == BAND_6G))
		heRlmFillHe6gBandCapIE(prAdapter, prBssInfo, prMsduInfo);
}
#endif /* CFG_SUPPORT_WIFI_6G == 1 */

#if (CFG_SUPPORT_NAN == 1)
uint32_t heRlmFillNANHECapIE(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t *pOutBuf)
{
	enum ENUM_BAND eHePhyCapBand = BAND_5G;
	struct _IE_HE_CAP_T *prHeCap;
	struct _HE_SUPPORTED_MCS_FIELD *prHeSupportedMcsSet;
	uint32_t u4OverallLen = OFFSET_OF(struct _IE_HE_CAP_T, aucVarInfo[0]);
	u_int8_t fgBfEn = TRUE;
	uint32_t soundingDim = 0;

	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#if (CFG_RX_PPE_THRESHOLD == 1)
	uint8_t *pPPEThreshold;
#endif
	uint8_t ucSupportedNss, ucDutNss = 0;
	u_int8_t fgTxStbcEn = TRUE;

	struct AIS_FSM_INFO *prAisFsmInfo = NULL;
	struct BSS_DESC *prBssDesc = NULL;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return 0;
	}
	if (!prBssInfo) {
		DBGLOG(NAN, ERROR, "prBssInfo error!\n");
		return 0;
	}

	ucDutNss = wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);
	if (ucDutNss > 0)
		ucSupportedNss = ucDutNss - 1;
	else
		ucSupportedNss = 0;
	prHeCap = (struct _IE_HE_CAP_T *)pOutBuf;

	prHeCap->ucId = ELEM_ID_RESERVED;
	prHeCap->ucExtId = ELEM_EXT_ID_HE_CAP;

	/* MAC capabilities */
	HE_RESET_MAC_CAP(prHeCap->ucHeMacCap);

	HE_SET_MAC_CAP_TRIGGER_PAD_DURATION(prHeCap->ucHeMacCap,
		prWifiVar->ucTrigMacPadDur);

	/* Check HTC blocklist */
	if (IS_BSS_AIS(prBssInfo)) {
		prAisFsmInfo = aisGetAisFsmInfo(prAdapter,
			prBssInfo->ucBssIndex);
		prBssDesc = aisGetTargetBssDesc(prAdapter,
				prBssInfo->ucBssIndex);
		if (prBssDesc != NULL &&
			queryAxBlocklist(prAdapter, prBssDesc->aucBSSID,
			    prBssInfo->ucBssIndex, BLOCKLIST_DIS_HE_HTC)) {
			DBGLOG(BSS, INFO,
			    "BSSID " MACSTR " is in HTC blocklist!\n",
			    MAC2STR(prBssDesc->aucBSSID));
		} else {
			HE_SET_MAC_CAP_HTC_HE(prHeCap->ucHeMacCap);
		}
	} else {
		HE_SET_MAC_CAP_HTC_HE(prHeCap->ucHeMacCap);
	}

	HE_SET_MAC_CAP_OM_CTRL(prHeCap->ucHeMacCap);

	HE_SET_MAC_CAP_MAX_AMPDU_LEN_EXP(prHeCap->ucHeMacCap,
		prWifiVar->ucMaxAmpduLenExp);

	/* PHY capabilities */
	HE_RESET_PHY_CAP(prHeCap->ucHePhyCap);

	if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex)
		>= MAX_BW_40MHZ
		&& prBssInfo->fgAssoc40mBwAllowed)
		HE_SET_PHY_CAP_CHAN_WIDTH_SET_BW40_2G(prHeCap->ucHePhyCap);

	if (prBssInfo->eBand == BAND_2G4)
		eHePhyCapBand = BAND_2G4;
	else if (prBssInfo->eBand == BAND_5G)
		eHePhyCapBand = BAND_5G;
	/* If current Bss band is 6G, use 6G BW to set PHY CAP, otherwise
	 * use 5G BW to set it. Prevent that ucSta5gBandwidth is differet with
	 * ucSta6gBandwidth
	 */
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prBssInfo->eBand == BAND_6G)
		eHePhyCapBand = BAND_6G;
#endif

	if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex)
		>= MAX_BW_40MHZ)
		HE_SET_PHY_CAP_CHAN_WIDTH_SET_BW40_BW80_5G(prHeCap->ucHePhyCap);

	if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex)
		>= MAX_BW_160MHZ)
		HE_SET_PHY_CAP_CHAN_WIDTH_SET_BW160_5G(prHeCap->ucHePhyCap);

	if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex)
		>= MAX_BW_80_80_MHZ)
		HE_SET_PHY_CAP_CHAN_WIDTH_SET_BW80P80_5G(prHeCap->ucHePhyCap);

	if (IS_FEATURE_ENABLED(prWifiVar->ucRxLdpc) &&
		IS_FEATURE_ENABLED(prWifiVar->ucTxLdpc))
		HE_SET_PHY_CAP_LDPC_CODING_IN_PAYLOAD(prHeCap->ucHePhyCap);

	if (prBssInfo->ucOpTxNss < 2)
		fgTxStbcEn = FALSE;

	if (rlmCheckTxStbc(prAdapter, prBssInfo->ucBssIndex,
		FEATURE_ENABLED) && fgTxStbcEn) {
		HE_SET_PHY_CAP_STBC_TX_LT_OR_EQ_80M(prHeCap->ucHePhyCap);
		if (IS_BSS_AIS(prBssInfo))
			HE_SET_PHY_CAP_STBC_TX_GT_80M(prHeCap->ucHePhyCap);
	}

	if (rlmCheckRxStbc(prAdapter,
		prBssInfo->ucBssIndex, FEATURE_ENABLED)) {
		HE_SET_PHY_CAP_STBC_RX_LT_OR_EQ_80M(prHeCap->ucHePhyCap);
		if (IS_BSS_AIS(prBssInfo))
			HE_SET_PHY_CAP_STBC_RX_GT_80M(prHeCap->ucHePhyCap);
	}

#if (CFG_SUPPORT_BFEE == 1)
#if (CFG_SUPPORT_CONDITIONAL_BFEE == 1)
	if ((prAdapter->rWifiVar.u4SwTestMode != ENUM_SW_TEST_MODE_SIGMA_AX) &&
		(IS_BSS_AIS(prBssInfo) && prAisFsmInfo != NULL)) {
		prBssDesc = aisGetTargetBssDesc(prAdapter,
			prBssInfo->ucBssIndex);
		if (prBssDesc != NULL) {
			soundingDim =
				HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW160_5G(
						prHeCap->ucHePhyCap) ?
				HE_GET_PHY_CAP5_NUM_OF_SND_DIM_GT_80M(
						prBssDesc->ucHePhyCapInfo) :
				HE_GET_PHY_CAP_NUM_OF_SND_DIM_LT_OR_EQ_80M(
						prBssDesc->ucHePhyCapInfo);

			DBGLOG(RLM, INFO,
				"bssGetRxNss: %d, soundingDim: %d\n",
				bssGetHeRxNss(prBssDesc), soundingDim);
			if ((bssGetHeRxNss(prBssDesc) ==
				wlanGetSupportNss(prAdapter,
					prBssInfo->ucBssIndex))
				&& (bssGetHeRxNss(prBssDesc) ==
					soundingDim + 1)) {
				fgBfEn = FALSE;
				DBGLOG(SW4, ERROR,
					"Disable Bfee due to same Nss between STA and AP\n");
			} else {
				fgBfEn = TRUE;
			}
		}
	}
#endif
	if ((fgBfEn == TRUE) && (IS_FEATURE_ENABLED(prWifiVar->ucStaHeBfee))) {
		HE_SET_PHY_CAP_SU_BFMEE(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_BFMEE_STS_LT_OR_EQ_80M(prHeCap->ucHePhyCap, 3);
		if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex)
				>= MAX_BW_160MHZ) {
			HE_SET_PHY_CAP_BFMEE_STS_GT_80M(
				prHeCap->ucHePhyCap, 3);
		}
		HE_SET_PHY_CAP_CODE_BOOK_4_2_SU_FB(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_CODE_BOOK_7_5_MU_FB(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_TRIG_SU_BF_FB(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_TRIG_MU_BF_PARTIAL_BW_FB(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_SU_MU_4X_HE_LTF(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_MAX_NC(prHeCap->ucHePhyCap, ucSupportedNss);
	}
#endif

#if (CFG_SUPPORT_HE_ER == 1)
	if (IS_FEATURE_ENABLED(prWifiVar->u4ExtendedRange)) {
		HE_SET_PHY_CAP_DCM_MAX_CONSTELLATION_TX(prHeCap->ucHePhyCap,
			MAX_SUPPORT_CONSTELLATION_DCM_QPSK);
		HE_SET_PHY_CAP_DCM_MAX_CONSTELLATION_RX(prHeCap->ucHePhyCap,
			MAX_SUPPORT_CONSTELLATION_DCM_QPSK);
		HE_SET_PHY_CAP_PARTIAL_BW_EXTENDED_RANGE(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_ER_SU_4X_HE_LTF(prHeCap->ucHePhyCap);
		HE_SET_PHY_CAP_ER_SU_PPDU_1X_HE_LTF(prHeCap->ucHePhyCap);
	}
#endif

	/* Set MCS map */
	prHeSupportedMcsSet = (struct _HE_SUPPORTED_MCS_FIELD *)
		(((uint8_t *) prHeCap) + u4OverallLen);
	heRlmFillMCSMap(prAdapter, prBssInfo, prHeSupportedMcsSet);
	u4OverallLen += sizeof(struct _HE_SUPPORTED_MCS_FIELD);

	if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex)
		>= MAX_BW_160MHZ) {
		prHeSupportedMcsSet = (struct _HE_SUPPORTED_MCS_FIELD *)
			(((uint8_t *) prHeCap) + u4OverallLen);
		heRlmFillMCSMap(prAdapter, prBssInfo, prHeSupportedMcsSet);
		u4OverallLen += sizeof(struct _HE_SUPPORTED_MCS_FIELD);
	}

	if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex)
		>= MAX_BW_80_80_MHZ) {
		prHeSupportedMcsSet = (struct _HE_SUPPORTED_MCS_FIELD *)
			(((uint8_t *) prHeCap) + u4OverallLen);
		heRlmFillMCSMap(prAdapter, prBssInfo, prHeSupportedMcsSet);
		u4OverallLen += sizeof(struct _HE_SUPPORTED_MCS_FIELD);
	}

#if (CFG_RX_PPE_THRESHOLD == 1)
	pPPEThreshold = ((uint8_t *) prHeCap) + u4OverallLen;
	u4OverallLen +=
		heRlmFillPPEThreshold(prAdapter, prBssInfo, pPPEThreshold);
#endif
	/* The Element ID Extension byte is included in length calculation */
	prHeCap->ucLength = u4OverallLen - ELEM_HDR_LEN;

	return IE_SIZE(prHeCap);
}

uint32_t heRlmFillNANHeOpIE(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t *pOutBuf)
{
	struct _IE_HE_OP_T *prHeOp;
	uint32_t u4Offset = OFFSET_OF(struct _IE_HE_OP_T, aucVarInfo[0]);
#if (CFG_SUPPORT_WIFI_6G == 1)
	struct _6G_OPER_INFOR_T *pr6gOperInfor = NULL;
#endif

	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	prHeOp = (struct _IE_HE_OP_T *)pOutBuf;

	prHeOp->ucId = ELEM_ID_RESERVED;
	prHeOp->ucExtId = ELEM_EXT_ID_HE_OP;

	memcpy(prHeOp->ucHeOpParams, prBssInfo->ucHeOpParams, HE_OP_BYTE_NUM);
	prHeOp->ucBssColorInfo = prBssInfo->ucBssColorInfo;
	prHeOp->u2HeBasicMcsSet = CPU_TO_LE16(prBssInfo->u2HeBasicMcsSet);

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (IS_BSS_APGO(prBssInfo) &&
		HE_IS_6G_OP_INFOR_PRESENT(prBssInfo->ucHeOpParams)) {

		if (HE_IS_CO_HOSTED_BSS(prBssInfo->ucHeOpParams))
			u4Offset += sizeof(uint8_t);

		pr6gOperInfor = (struct _6G_OPER_INFOR_T *)
			(((uint8_t *) prHeOp) + u4Offset);

		memcpy(pr6gOperInfor, &prBssInfo->r6gOperInfor,
			sizeof(struct _6G_OPER_INFOR_T));

		u4Offset += sizeof(struct _6G_OPER_INFOR_T);
	}
#endif

	prHeOp->ucLength = u4Offset - ELEM_HDR_LEN;
	return IE_SIZE(prHeOp);
}

#endif

uint8_t heRlmPeerMaxBwCap(
	uint8_t *pucChannelWidthSet)
{
	uint8_t width = MAX_BW_20MHZ;

	if (HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW40_2G(pucChannelWidthSet))
		width = MAX_BW_40MHZ;
	if (HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW40_BW80_5G(pucChannelWidthSet)) {
		width = MAX_BW_80MHZ;
		if (HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW80P80_5G(pucChannelWidthSet))
			width = MAX_BW_160MHZ;
		if (HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW160_5G(pucChannelWidthSet))
			width = MAX_BW_160MHZ;
	}

	return width;
}

static uint16_t heRlmGetHeMcsMap(uint8_t *pSrc)
{
	uint16_t u2McsMap;

	kalMemCopy(&u2McsMap, pSrc, sizeof(u2McsMap));
	u2McsMap = LE16_TO_CPU(u2McsMap);

	return u2McsMap;
}

static uint32_t heRlmRecHeMcsMap(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct _IE_HE_CAP_T *prHeCap)
{
	uint32_t u4McsMapOffset;
	uint16_t u2McsMap;
	struct BSS_INFO *prBssInfo;
	uint8_t ucHeCapMcsOwnNotSupportOffset = 0, ucMaxBw;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "prBssInfo is null\n");
		return 0;
	}
	ucMaxBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);

	/* BW 80Mhz */
	u4McsMapOffset = OFFSET_OF(struct _IE_HE_CAP_T, aucVarInfo[0]);

	u2McsMap = heRlmGetHeMcsMap(((uint8_t *)prHeCap) + u4McsMapOffset);
	prStaRec->u2HeRxMcsMapBW80 = u2McsMap;

	if (wlanGetSupportNss(prAdapter, prStaRec->ucBssIndex) < 8) {
		ucHeCapMcsOwnNotSupportOffset =
			wlanGetSupportNss(prAdapter, prStaRec->ucBssIndex) * 2;
		/* Mark Rx Mcs Map which we don't support */
		prStaRec->u2HeRxMcsMapBW80 |=
			BITS(ucHeCapMcsOwnNotSupportOffset, 15);
	}
	prStaRec->u2HeRxMcsMapBW80Assoc = prStaRec->u2HeRxMcsMapBW80;

	if (prStaRec->u2HeRxMcsMapBW80 != u2McsMap)
		DBGLOG(RLM, TRACE, "Change HeRxMcsMapBW80 from 0x%x to 0x%x\n",
			u2McsMap, prStaRec->u2HeRxMcsMapBW80);

	u4McsMapOffset += sizeof(uint16_t);

	u2McsMap = heRlmGetHeMcsMap(((uint8_t *)prHeCap) + u4McsMapOffset);
	prStaRec->u2HeTxMcsMapBW80 = u2McsMap;

	u4McsMapOffset += sizeof(uint16_t);

	/* BW 160Mhz */
	if (HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW160_5G(prStaRec->ucHePhyCapInfo)) {
		u2McsMap = heRlmGetHeMcsMap(((uint8_t *)prHeCap) +
			u4McsMapOffset);
		prStaRec->u2HeRxMcsMapBW160 = u2McsMap;

		if (ucMaxBw >= MAX_BW_160MHZ) {
			/* Mark Rx Mcs Map which we don't support */
			prStaRec->u2HeRxMcsMapBW160 |=
				BITS(ucHeCapMcsOwnNotSupportOffset, 15);

			if (prStaRec->u2HeRxMcsMapBW160 != u2McsMap)
				DBGLOG(RLM, TRACE,
				    "Change McsMapBW160 from 0x%x to 0x%x\n",
				    u2McsMap, prStaRec->u2HeRxMcsMapBW160);
		} else {
			/* BW160 is not supported locally */
			prStaRec->u2HeRxMcsMapBW160 = BITS(0, 15);
		}

		u4McsMapOffset += sizeof(uint16_t);
		u2McsMap = heRlmGetHeMcsMap(((uint8_t *)prHeCap) +
			u4McsMapOffset);
		prStaRec->u2HeTxMcsMapBW160 = u2McsMap;
		u4McsMapOffset += sizeof(uint16_t);
	} else {
		prStaRec->u2HeRxMcsMapBW160 = BITS(0, 15);
		prStaRec->u2HeTxMcsMapBW160 = BITS(0, 15);
	}
	prStaRec->u2HeRxMcsMapBW160Assoc = prStaRec->u2HeRxMcsMapBW160;

	/* BW 80+80 */
	if (HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW80P80_5G(prStaRec->ucHePhyCapInfo)) {
		u2McsMap = heRlmGetHeMcsMap(((uint8_t *)prHeCap) +
			u4McsMapOffset);
		prStaRec->u2HeRxMcsMapBW80P80 = u2McsMap;

		if (ucMaxBw >= MAX_BW_80_80_MHZ) {
			/* Mark Rx Mcs Map which we don't support */
			prStaRec->u2HeRxMcsMapBW80P80 |=
				BITS(ucHeCapMcsOwnNotSupportOffset, 15);

			if (prStaRec->u2HeRxMcsMapBW80P80 != u2McsMap)
				DBGLOG(RLM, TRACE,
				    "Change McsMapBW80P80 from 0x%x to 0x%x\n",
				    u2McsMap, prStaRec->u2HeRxMcsMapBW80P80);
		} else {
			/* BW 80+80 is not supported locally */
			prStaRec->u2HeRxMcsMapBW80P80 = BITS(0, 15);
		}

		u4McsMapOffset += sizeof(uint16_t);
		u2McsMap = heRlmGetHeMcsMap(((uint8_t *)prHeCap) +
			u4McsMapOffset);
		prStaRec->u2HeTxMcsMapBW80P80 = u2McsMap;
		u4McsMapOffset += sizeof(uint16_t);
	} else {
		prStaRec->u2HeRxMcsMapBW80P80 = BITS(0, 15);
		prStaRec->u2HeTxMcsMapBW80P80 = BITS(0, 15);
	}
	prStaRec->u2HeRxMcsMapBW80P80Assoc = prStaRec->u2HeRxMcsMapBW80P80;

	log_dbg(RLM, LOUD, "PhyCap:1st:%x,..heRlmRecHeMcsMap-80:%x,%x,160:%x,%x,80+80:%x,%x\n",
		prStaRec->ucHePhyCapInfo[0],
		prStaRec->u2HeRxMcsMapBW80,
		prStaRec->u2HeTxMcsMapBW80,
		prStaRec->u2HeRxMcsMapBW160,
		prStaRec->u2HeTxMcsMapBW160,
		prStaRec->u2HeRxMcsMapBW80P80,
		prStaRec->u2HeTxMcsMapBW80P80
		);

	return u4McsMapOffset;
}

static void heRlmRecHePPEThresholds(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct _IE_HE_CAP_T *prHeCap,
	uint32_t u4Offset)
{
	/* To store subfiled values in peer's PPE threshold */
	uint8_t i, j, k, ucNSS, ucRUIdxBMP, ucRUIdxBMPTmp;
	uint8_t ucPPEInfo[128], ucPos;
	uint8_t *pucMem = ((uint8_t *)prHeCap) + u4Offset;

	ucNSS = (*pucMem) & HE_CAP_PPE_NSS;
	ucRUIdxBMP = ((*pucMem) & HE_CAP_PPE_RU_IDX_BMP) >>
		HE_CAP_PPE_RU_IDX_BMP_SHFT;

	k = 0;
	ucPos = HE_CAP_PPE_PPET16_NSS1_RU0_SHFT;
	for (i = 0; i <= ucNSS; i++) {
		ucRUIdxBMPTmp = ucRUIdxBMP;
		for (j = 0; j < PPE_RU_IDX_SIZE * 2; j++) {
			if (ucRUIdxBMPTmp & 0x1) {
				ucPPEInfo[k] = heRlmGet3BitPPE(pucMem, ucPos);
				ucPos += 3;
			} else {
				ucPPEInfo[k] = 0;
			}
			k++;
			ucRUIdxBMPTmp = (ucRUIdxBMPTmp >> 1);
		}
	}
}

#if (CFG_SUPPORT_WIFI_6G == 1)
void heRlmRecHe6GCapInfo(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t *pucIE)
{
	struct _IE_HE_6G_BAND_CAP_T *prHe6GCap =
		(struct _IE_HE_6G_BAND_CAP_T *) pucIE;

	/* if payload not contain any aucVarInfo,
	 * IE size = sizeof(struct _IE_HE_6G_BAND_CAP_T)
	 */
	if (IE_SIZE(prHe6GCap) < (sizeof(struct _IE_HE_6G_BAND_CAP_T))) {
		DBGLOG(SCN, WARN,
			"HE_6G_CAP IE_LEN err(%d)!\n", IE_LEN(prHe6GCap));
		return;
	}

	prStaRec->u2He6gBandCapInfo = prHe6GCap->u2CapInfo;
}
#endif

void heRlmRecHeCapInfo(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	const uint8_t *pucIE)
{
	uint32_t u4Offset;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct _IE_HE_CAP_T *prHeCap = (struct _IE_HE_CAP_T *) pucIE;

	/* if payload not contain any aucVarInfo,
	 * IE size = sizeof(struct _IE_HE_CAP_T)
	 */
	if (IE_SIZE(prHeCap) < (sizeof(struct _IE_HE_CAP_T))) {
		DBGLOG(SCN, WARN, "HE_CAP IE_LEN err(%d)!\n", IE_LEN(prHeCap));
		return;
	}

	memcpy(prStaRec->ucHeMacCapInfo, prHeCap->ucHeMacCap,
		HE_MAC_CAP_BYTE_NUM);
	memcpy(prStaRec->ucHePhyCapInfo, prHeCap->ucHePhyCap,
		HE_PHY_CAP_BYTE_NUM);

#if (CFG_SUPPORT_HE_ER == 1)
	DBGLOG(RLM, TRACE,
		"ER: TX:%lu, RX:%lu, bw:%lu, 4x LTF:%lu, 1X LTF:%lu\n",
		HE_GET_PHY_CAP_DCM_MAX_CONSTELLATION_TX(
			prStaRec->ucHePhyCapInfo),
		HE_GET_PHY_CAP_DCM_MAX_CONSTELLATION_RX(
			prStaRec->ucHePhyCapInfo),
		HE_GET_PHY_CAP_PARTIAL_BW_EXTENDED_RANGE(
			prStaRec->ucHePhyCapInfo),
		HE_GET_PHY_CAP_ER_SU_4X_HE_LTF(
			prStaRec->ucHePhyCapInfo),
		HE_GET_PHY_CAP_ER_SU_PPDU_1X_HE_LTF(
			prStaRec->ucHePhyCapInfo));
#endif

	log_dbg(RLM, LOUD, "ucHePhyCapInfo-0-%x,%x,%x,%x,%x,%x\n",
			prStaRec->ucHePhyCapInfo[0],
			prStaRec->ucHePhyCapInfo[1],
			prStaRec->ucHePhyCapInfo[2],
			prStaRec->ucHePhyCapInfo[3],
			prStaRec->ucHePhyCapInfo[4],
			prStaRec->ucHePhyCapInfo[5]
			);

	log_dbg(RLM, LOUD, "ucHePhyCapInfo-1-%x,%x,%x,%x,%x\n",
			prStaRec->ucHePhyCapInfo[6],
			prStaRec->ucHePhyCapInfo[7],
			prStaRec->ucHePhyCapInfo[8],
			prStaRec->ucHePhyCapInfo[9],
			prStaRec->ucHePhyCapInfo[10]
			);

	/* Disable peer bfer cap indication to FW
	 * if our bfee feature is not on
	 */
	if (IS_FEATURE_DISABLED(prWifiVar->ucStaHeBfee)) {
		HE_UNSET_PHY_CAP_SU_BFMER(prStaRec->ucHePhyCapInfo);
		HE_UNSET_PHY_CAP_MU_BFMER(prStaRec->ucHePhyCapInfo);
	}

	/* Check LDPC setting */
	if (IS_FEATURE_DISABLED(prWifiVar->ucTxLdpc)) {
		/* Only check TxLdpc due to this setting only affect the RA */
		HE_UNSET_PHY_CAP_LDPC_CODING_IN_PAYLOAD(
			prStaRec->ucHePhyCapInfo);
	}

	/* Set HE Rx Mcs Map upon peer's capability and our capability */
	u4Offset = heRlmRecHeMcsMap(prAdapter, prStaRec, prHeCap);

	/* Set HE PPE Thresholds if it exists */
	if (HE_IS_PHY_CAP_PPE_THRESHOLD(prStaRec->ucHePhyCapInfo))
		heRlmRecHePPEThresholds(prAdapter, prStaRec, prHeCap, u4Offset);

	if (rlmCheckTxStbc(prAdapter, prStaRec->ucBssIndex,
		FEATURE_DISABLED))
		HE_UNSET_PHY_CAP_STBC_TX_LT_OR_EQ_80M(prStaRec->ucHePhyCapInfo);
	if (rlmCheckRxStbc(prAdapter,
		prStaRec->ucBssIndex, FEATURE_DISABLED))
		HE_UNSET_PHY_CAP_STBC_RX_LT_OR_EQ_80M(prStaRec->ucHePhyCapInfo);

	log_dbg(RLM, LOUD, "ucSapTxStbc: %d,ucSapRxStbc: %d\n",
				prWifiVar->ucSapTxStbc,
				prWifiVar->ucSapRxStbc);
}

void heRlmRecHeOperation(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo,
	const uint8_t *pucIE)
{
	struct _IE_HE_OP_T *prHeOp = (struct _IE_HE_OP_T *) pucIE;
	uint8_t ucBssHeOpParams[2];
	uint32_t u4IeTxopDurationRtsThreshold = 0;
#if (CFG_SUPPORT_HE_ER == 1)
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	/* if payload not contain any aucVarInfo,
	 * IE size = sizeof(struct _IE_HE_OP_T)
	 */
	if (IE_SIZE(prHeOp) < (sizeof(struct _IE_HE_OP_T))) {
		DBGLOG(SCN, WARN, "HE_OP IE_LEN err(%d)!\n", IE_LEN(prHeOp));
		return;
	}

	if (IS_FEATURE_DISABLED(prWifiVar->u4ExtendedRange)) {
		HE_SET_OP_PARAM_ER_SU_DISABLE(prHeOp->ucHeOpParams);

		DBGLOG(RLM, TRACE, "ER: is ER SU: %lu\n",
			HE_IS_ER_SU_DISABLE(prHeOp->ucHeOpParams));
	}
#endif

	ucBssHeOpParams[0] = prBssInfo->ucHeOpParams[0];
	ucBssHeOpParams[1] = prBssInfo->ucHeOpParams[1];
	u4IeTxopDurationRtsThreshold =
		((prHeOp->ucHeOpParams[0] &
			HE_OP_PARAM0_TXOP_DUR_RTS_THRESHOLD_MASK) >>
			HE_OP_PARAM0_TXOP_DUR_RTS_THRESHOLD_SHFT) |
		(((prHeOp->ucHeOpParams[1] &
			HE_OP_PARAM1_TXOP_DUR_RTS_THRESHOLD_MASK) >>
			HE_OP_PARAM1_TXOP_DUR_RTS_THRESHOLD_SHFT) << 4);

	memcpy(prBssInfo->ucHeOpParams, prHeOp->ucHeOpParams, HE_OP_BYTE_NUM);

	if (u4IeTxopDurationRtsThreshold == 0) {
		prBssInfo->ucHeOpParams[0] |= ucBssHeOpParams[0] &
			HE_OP_PARAM0_TXOP_DUR_RTS_THRESHOLD_MASK;
		prBssInfo->ucHeOpParams[1] |= ucBssHeOpParams[1] &
			HE_OP_PARAM1_TXOP_DUR_RTS_THRESHOLD_MASK;
	}

	prBssInfo->ucBssColorInfo = prHeOp->ucBssColorInfo;
	prBssInfo->u2HeBasicMcsSet = prHeOp->u2HeBasicMcsSet;

	/* BssColor Disabled bit == False && Color == 0 -> AP ERROR*/
	if ((((prBssInfo->ucBssColorInfo) & (HE_OP_BSSCOLOR_BSS_COLOR_DISABLE))
		>> HE_OP_BSSCOLOR_BSS_COLOR_DISABLE_SHFT == 0) &&
		((prBssInfo->ucBssColorInfo)
			& (HE_OP_BSSCOLOR_BSS_COLOR_MASK) == 0))
		log_dbg(RLM, ERROR, "BssColor Enable but Color = 0\n");

	log_dbg(RLM, LOUD, "RlmHeOpInfo-0x:%x,%x,%x,%x,%x\n",
		prBssInfo->ucHeOpParams[0],
		prBssInfo->ucHeOpParams[1],
		prBssInfo->ucHeOpParams[2],
		prBssInfo->ucBssColorInfo,
		prBssInfo->u2HeBasicMcsSet
		);

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (HE_IS_CO_HOSTED_BSS(prHeOp->ucHeOpParams))
		prBssInfo->fgIsCoHostedBssPresent = TRUE;
	else
		prBssInfo->fgIsCoHostedBssPresent = FALSE;

	if (prBssInfo->eBand == BAND_6G) {
		if (HE_IS_VHT_OP_INFO_PRESENT(prHeOp->ucHeOpParams))
			prBssInfo->fgIsHE6GPresent = FALSE;
		else if (HE_IS_6G_OP_INFOR_PRESENT(prHeOp->ucHeOpParams))
			prBssInfo->fgIsHE6GPresent = TRUE;
		else
			prBssInfo->fgIsHE6GPresent = FALSE;
	} else {
		prBssInfo->fgIsHE6GPresent = FALSE;
	}
#endif
}

#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
void heRlmRecBssColorChangeAnnouncement(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	const uint8_t *pucIE)
{
	struct _IE_COLOR_CHANGE_ANNOUNCEMENT_T *prColorChangeAnnouncement =
		(struct _IE_COLOR_CHANGE_ANNOUNCEMENT_T *) pucIE;

	if (IE_SIZE(prColorChangeAnnouncement)
			< (sizeof(struct _IE_COLOR_CHANGE_ANNOUNCEMENT_T))) {
		DBGLOG(SCN, WARN, "COLOR_CHANGE_ANNOUNCEMENT IE_LEN err(%d)!\n",
			IE_LEN(prColorChangeAnnouncement));
		return;
	}

	prBssInfo->ucColorAnnouncement = TRUE;

	prBssInfo->ucColorSwitchCntdn =
		prColorChangeAnnouncement->ucColorSwitchCntdn;
	prBssInfo->ucNewBssColorInfo =
		prColorChangeAnnouncement->ucNewBssColorInfo;


	DBGLOG(RLM, LOUD,
		"RlmBssColorChangeAnnouncement-ColorSwitchCntdn:0x%x,NewBssColorInfo:0x%x\n",
		prBssInfo->ucColorSwitchCntdn,
		prBssInfo->ucNewBssColorInfo
	);

}   /* end of heRlmRecBssColorChangeAnnouncement */
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */

uint8_t heRlmUpdateSRParams(
	struct BSS_INFO *prBssInfo,
	const uint8_t *pucIE)
{
	struct _IE_SR_PARAM_T *prSRParam = (struct _IE_SR_PARAM_T *) pucIE;
	uint32_t u4IEOffset;
	uint8_t fgIsNew = FALSE;

	if (prBssInfo->ucSRControl != prSRParam->ucSRControl) {
		fgIsNew = TRUE;
		prBssInfo->ucSRControl = prSRParam->ucSRControl;
	}

	u4IEOffset = OFFSET_OF(struct _IE_SR_PARAM_T, aucVarInfo[0]);

	if (prBssInfo->ucSRControl & SR_PARAM_NON_SRG_OFFSET_PRESENT) {
		struct _NON_SRG_SR_INFO_T *prNonSRGInfo =
			(struct _NON_SRG_SR_INFO_T *)
			(((uint8_t *) pucIE) + u4IEOffset);

		if (prBssInfo->ucNonSRGObssPdMaxOffset !=
			prNonSRGInfo->ucObssPdMaxOffset) {

			fgIsNew = TRUE;
			prBssInfo->ucNonSRGObssPdMaxOffset =
				prNonSRGInfo->ucObssPdMaxOffset;
		}

		u4IEOffset += sizeof(struct _NON_SRG_SR_INFO_T);
	} else
		prBssInfo->ucNonSRGObssPdMaxOffset = 0;

	if (prBssInfo->ucSRControl & SR_PARAM_SRG_INFO_PRESENT) {
		struct _SRG_SR_INFO_T *prSRGInfo = (struct _SRG_SR_INFO_T *)
			(((uint8_t *) pucIE) + u4IEOffset);

		if (prBssInfo->ucSRGObssPdMinOffset !=
			prSRGInfo->ucObssPdMinOffset) {
			fgIsNew = TRUE;
			prBssInfo->ucSRGObssPdMinOffset =
				prSRGInfo->ucObssPdMinOffset;
		}
		if (prBssInfo->ucSRGObssPdMaxOffset !=
			prSRGInfo->ucObssPdMaxOffset) {
			fgIsNew = TRUE;
			prBssInfo->ucSRGObssPdMaxOffset =
				prSRGInfo->ucObssPdMaxOffset;
		}
		if (prBssInfo->u8SRGBSSColorBitmap !=
			LE64_TO_CPU(prSRGInfo->u8BSSColorBitmap)) {
			fgIsNew = TRUE;
			prBssInfo->u8SRGBSSColorBitmap =
				LE64_TO_CPU(prSRGInfo->u8BSSColorBitmap);
		}
		if (prBssInfo->u8SRGPartialBSSIDBitmap !=
			LE64_TO_CPU(prSRGInfo->u8PartialBSSIDBitmap)) {
			fgIsNew = TRUE;
			prBssInfo->u8SRGPartialBSSIDBitmap =
				LE64_TO_CPU(prSRGInfo->u8PartialBSSIDBitmap);
		}
	} else {
		prBssInfo->ucSRGObssPdMinOffset = 0;
		prBssInfo->ucSRGObssPdMaxOffset = 0;
		prBssInfo->u8SRGBSSColorBitmap = 0;
		prBssInfo->u8SRGPartialBSSIDBitmap = 0;
	}

	return fgIsNew;
}

uint8_t heRlmRecHeSRParams(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct SW_RFB *prSwRfb,
	const uint8_t *pucIE,
	uint16_t u2IELength)
{
	uint16_t u2Offset;
	uint8_t fgNewParameter = FALSE;

	if (!pucIE)
		return FALSE;

	/* Obtain the SR parameters */
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_RESERVED:
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_SR_PARAM) {
				fgNewParameter =
					heRlmUpdateSRParams(prBssInfo, pucIE);
			}
			break;
		default:
			break;
		}
	}

	return fgNewParameter;
}

void heRlmInitHeHtcACtrlOMAndUPH(
	struct ADAPTER *prAdapter)
{
	prAdapter->u4HeHtcOM = 0;
	HE_SET_HTC_HE_VARIANT(prAdapter->u4HeHtcOM);
	HE_SET_HTC_1ST_A_CTRL_ID(prAdapter->u4HeHtcOM, HTC_HE_A_CTRL_OM);
	HE_SET_HTC_HE_OM_RX_NSS(prAdapter->u4HeHtcOM, 1);
	HE_SET_HTC_HE_OM_TX_NSTS(prAdapter->u4HeHtcOM, 1);
	HE_SET_HTC_HE_OM_CH_WIDTH(prAdapter->u4HeHtcOM, CH_BW_80);
	HE_SET_HTC_HE_OM_UL_MU_DISABLE(prAdapter->u4HeHtcOM, 0);
	HE_SET_HTC_HE_OM_UL_MU_DATA_DISABLE(prAdapter->u4HeHtcOM, 0);
	HE_SET_HTC_2ND_A_CTRL_ID(prAdapter->u4HeHtcOM, HTC_HE_A_CTRL_UPH);
	HE_SET_HTC_HE_UPH(prAdapter->u4HeHtcOM, 10);
	HE_SET_HTC_HE_UPH_MIN_TX_PWR_FLAG(prAdapter->u4HeHtcOM, 0);
}

void heRlmReqGenerateHeHtcACtrlOM(
	struct HE_A_CTRL_OM_T *prHeActrlOM,
	uint32_t *pHtc)
{
	uint32_t u4HTC = 0;

	if (!pHtc || !prHeActrlOM) {
		DBGLOG(RLM, WARN,
		 "heRlmReqGenerateHeHtcACtrlOM:: Please check function parameters.\n");
		return;
	}

	HE_SET_HTC_1ST_A_CTRL_ID(u4HTC, HTC_HE_A_CTRL_OM);
	HE_SET_HTC_HE_OM_RX_NSS(u4HTC, prHeActrlOM->ucRxNss);
	HE_SET_HTC_HE_OM_TX_NSTS(u4HTC, prHeActrlOM->ucTxNsts);
	HE_SET_HTC_HE_OM_CH_WIDTH(u4HTC, prHeActrlOM->ucBW);
	HE_SET_HTC_HE_OM_UL_MU_DISABLE(u4HTC, prHeActrlOM->fgDisMuUL);
	HE_SET_HTC_HE_OM_UL_MU_DATA_DISABLE(u4HTC, prHeActrlOM->fgDisMuULData);
	*pHtc = u4HTC;
	DBGLOG(RLM, STATE,
		"heRlmReqGenerateHeHtcACtrlOM:: u4HTC = 0x%08x\n", u4HTC);
}

void heRlmParseHeHtcACtrlOM(
	uint32_t u4Htc,
	struct HE_A_CTRL_OM_T *prHeActrlOM)
{
	DBGLOG(RLM, STATE,
		"heRlmParseHeHtcACtrlOM:: HTC Control Field = 0x%08x\n",
		u4Htc);
	if (HE_GET_HTC_1ST_A_CTRL_ID(u4Htc) == HTC_HE_A_CTRL_OM) {
		prHeActrlOM->ucRxNss = HE_GET_HTC_HE_OM_RX_NSS(u4Htc);
		prHeActrlOM->ucTxNsts = HE_GET_HTC_HE_OM_TX_NSTS(u4Htc);
		prHeActrlOM->ucBW = HE_GET_HTC_HE_OM_CH_WIDTH(u4Htc);
		prHeActrlOM->fgDisMuUL = HE_GET_HTC_HE_OM_UL_MU_DISABLE(u4Htc);
		prHeActrlOM->fgDisMuULData =
			HE_GET_HTC_HE_OM_UL_MU_DATA_DISABLE(u4Htc);
	} else
		DBGLOG(RLM, WARN, "This is not Operating mode (OM).\n");
}


/*----------------------------------------------------------------------------*/
/*!
* @brief This function will compose the QoS Null + HTC Data frame.
*
* @param[in] prAdapter              Pointer to the Adapter structure.
* @param[in] pucBuffer              Pointer to the frame buffer.
* @param[in] prStaRec               Pointer to the STA_RECORD_T.
* @param[in] ucUP                   User Priority.
* @param[in] fgSetEOSP              Set the EOSP bit.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void
heRlmComposeHtcNullFrame(
	struct ADAPTER *prAdapter,
	uint8_t *pucBuffer,
	struct STA_RECORD *prStaRec,
	uint8_t ucUP,
	u_int8_t fgSetEOSP)
{
	struct WLAN_MAC_HEADER_HT *prQoSNullFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2FrameCtrl;
	uint16_t u2QosControl;
	uint8_t ucBssIndex;

	ASSERT(prStaRec);
	ucBssIndex = prStaRec->ucBssIndex;

	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	ASSERT(pucBuffer);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "prBssInfo is null\n");
		return;
	}

	prQoSNullFrame = (struct WLAN_MAC_HEADER_HT *) pucBuffer;

	/* 4 <1> Decide the Frame Control Field */
#if 0
	u2FrameCtrl = MAC_FRAME_QOS_DATA | MASK_FC_ORDER;
#else
	u2FrameCtrl = MAC_FRAME_QOS_NULL | MASK_FC_ORDER;
#endif

	if (IS_AP_STA(prStaRec)) {
		u2FrameCtrl |= MASK_FC_TO_DS;
	} else if (IS_CLIENT_STA(prStaRec)) {
		u2FrameCtrl |= MASK_FC_FROM_DS;
	} else if (IS_DLS_STA(prStaRec)) {
		/* TODO(Kevin) */
	} else {
		/* NOTE(Kevin): We won't send QoS Null frame for IBSS */
		ASSERT(0);
		return;
	}

	/* 4 <2> Compose the QoS Null frame */
	/* Fill the Frame Control field. */
	/* WLAN_SET_FIELD_16(&prQoSNullFrame->u2FrameCtrl, u2FrameCtrl); */
	prQoSNullFrame->u2FrameCtrl = u2FrameCtrl;

	/* Fill the Address 1 field with Target Peer Address. */
	COPY_MAC_ADDR(prQoSNullFrame->aucAddr1, prStaRec->aucMacAddr);

	/* Fill the Address 2 field with our MAC Address. */
	COPY_MAC_ADDR(prQoSNullFrame->aucAddr2, prBssInfo->aucOwnMacAddr);

	/* Fill the Address 3 field with Target BSSID. */
	COPY_MAC_ADDR(prQoSNullFrame->aucAddr3, prBssInfo->aucBSSID);

	/* Clear the SEQ/FRAG_NO field */
	/* (HW won't overide the FRAG_NO, so we need to clear it). */
	prQoSNullFrame->u2SeqCtrl = 0;

	u2QosControl = (uint16_t) (ucUP & WMM_QC_UP_MASK);

	if (fgSetEOSP)
		u2QosControl |= WMM_QC_EOSP;

	prQoSNullFrame->u2QosCtrl = u2QosControl;

#ifdef	CFG_AX_CERT_WKR
	HE_SET_HTC_2ND_A_CTRL_ID(prAdapter->u4HeHtcOM, HTC_HE_A_CTRL_UPH);
	HE_SET_HTC_HE_UPH(prAdapter->u4HeHtcOM, 10);
	HE_SET_HTC_HE_UPH_MIN_TX_PWR_FLAG(prAdapter->u4HeHtcOM, 0);
#endif

	prQoSNullFrame->u4HtCtrl = prAdapter->u4HeHtcOM;

	return;

}				/* end of bssComposeQoSNullFrameHeader() */

uint32_t heRlmSendHtcNullFrame(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucUP,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	uint16_t u2EstimatedFrameLen;

	/* 4 <1> Allocate a PKT_INFO_T for Null Frame */
	/* Init with MGMT Header Length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD
		+ WLAN_MAC_HEADER_QOS_HTC_LEN;

	/* Allocate a MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(RLM, WARN, "No PKT_INFO_T for sending Null Frame.\n");
		return WLAN_STATUS_FAILURE;
	}
	/* 4 <2> Compose Null frame in MSDU_INfO_T. */
	heRlmComposeHtcNullFrame(prAdapter,
		(uint8_t *)((uintptr_t)(prMsduInfo->prPacket)
			+ MAC_TX_RESERVED_FIELD),
		prStaRec, ucUP, FALSE);

	TX_SET_MMPDU(prAdapter,
		     prMsduInfo,
		     prStaRec->ucBssIndex,
		     prStaRec->ucIndex,
		     WLAN_MAC_HEADER_QOS_HTC_LEN,
		     WLAN_MAC_HEADER_QOS_HTC_LEN,
		     pfTxDoneHandler,
		     MSDU_RATE_MODE_AUTO);

	/* prMsduInfo->ucUserPriority = 0;*/
	prMsduInfo->u4Option |= MSDU_OPT_NO_AGGREGATE;
	prMsduInfo->u4Option |= MSDU_OPT_SW_HTC;
	prMsduInfo->fgMgmtForceAutoRate = TRUE;
	/* 4 <4> Inform TXM  to send this Null frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;

}				/* end of bssSendQoSNullFrame() */

void heRlmInit(
	struct ADAPTER *prAdapter)
{
	struct __HE_CFG_INFO_T *prHeCfg;

	prHeCfg = &prAdapter->rHeCfg;
	prHeCfg->fgHeSupport = TRUE;
	/* It can be disabled by wifi.cfg or iwpriv command */
	prHeCfg->fgHeEnable = TRUE;
	/* It can be disabled by wifi.cfg or iwpriv command */
	prHeCfg->fgTwtRequesterEnable = TRUE;
}


#if (CFG_SUPPORT_802_11AX == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint32_t heRlmSMPSTxDone(struct ADAPTER *prAdapter,
			      struct MSDU_INFO *prMsduInfo,
			      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{

	return WLAN_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief Send SM Power Save frame (HT action frame)
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void heRlmSendSMPSActionFrame(struct ADAPTER *prAdapter,
			     struct STA_RECORD *prStaRec)
{
	struct MSDU_INFO *prMsduInfo;
	struct ACTION_SM_POWER_SAVE_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	PFN_TX_DONE_HANDLER pfTxDoneHandler = (PFN_TX_DONE_HANDLER)NULL;

	/* Sanity Check*/
	if (!prStaRec)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return;

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
			      sizeof(struct ACTION_SM_POWER_SAVE_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
							u2EstimatedFrameLen);

	if (!prMsduInfo)
		return;

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	prTxFrame = prMsduInfo->prPacket;

	/* Fill frame ctrl */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	/* 3 Compose the frame body's frame */
	prTxFrame->ucCategory = CATEGORY_HT_ACTION;
	prTxFrame->ucAction = ACTION_HT_SM_POWER_SAVE;

	/* Set SM power save enabled*/
	prTxFrame->ucSmPowerCtrl |= HT_SM_POWER_SAVE_CONTROL_ENABLED;

	/* Static SM power save mode */
	prTxFrame->ucSmPowerCtrl |= HT_SM_POWER_SAVE_CONTROL_SM_MODE;

	if (prBssInfo->pfOpChangeHandler)
		pfTxDoneHandler = heRlmSMPSTxDone;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prBssInfo->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     sizeof(struct ACTION_SM_POWER_SAVE_FRAME), pfTxDoneHandler,
		     MSDU_RATE_MODE_AUTO);

	/* 5 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}


void heRlmProcessSMPSAction(
	struct ADAPTER *prAdapter, struct MSG_HDR *prMsgHdr)
{
	struct _MSG_SMPS_PARAMS_SET_T *prSMPSParamSetMsg;
	struct _SMPS_CTRL_T rSMPSCtrl, *prSMPSCtrl = &rSMPSCtrl;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucBssIdx;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (!prAdapter) {
		DBGLOG(RLM, ERROR, "prAdapter is NULL\n");
		return;
	}

	if (!prMsgHdr) {
		DBGLOG(RLM, ERROR, "prMsgHdr is NULL\n");
		return;
	}

	prSMPSParamSetMsg = (struct _MSG_SMPS_PARAMS_SET_T *) prMsgHdr;
	kalMemCopy(prSMPSCtrl, &prSMPSParamSetMsg->rSMPSCtrl,
			sizeof(*prSMPSCtrl));

	cnmMemFree(prAdapter, prMsgHdr);

	/* Find the BSS info */
	ucBssIdx = prSMPSCtrl->ucBssIdx;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "prBssInfo is null\n");
		return;
	}

	switch (prSMPSCtrl->ucCtrlAction) {

	case SMPS_ACTION_UPDATE_HT_CAP:
		g_ucHtSMPSCapValue = prSMPSCtrl->rSMPSParams.ucHtHeCap;
		g_ucHtSMPS6GCapValue = prSMPSCtrl->rSMPSParams.ucHtHeCap;
		g_fgSigmaCMDHt = 1;
		g_fgSigmaCMD6g = 1;
		break;

	case SMPS_ACTION_UPDATE_HE_CAP:
		/* Update HE SMPS Capability*/
		prWifiVar->ucHeDynamicSMPS = prSMPSCtrl->rSMPSParams.ucHtHeCap;
		g_ucHeSMPSCapValue = prSMPSCtrl->rSMPSParams.ucHtHeCap;
		g_fgSigmaCMDHe = 1;
		break;

	case SMPS_ACTION_SEND_ACTION_FRAME:
		/* Get the STA Record */
		if (prBssInfo) {
			prStaRec = prBssInfo->prStaRecOfAP;
			if (!prStaRec) {
				DBGLOG(RLM, INFO, "No AP STA Record\n");
				return;
			}
		} else {
			DBGLOG(RLM, WARN, "BssInfo is NULL!\n");
			return;
		}

		DBGLOG(RLM, INFO, "SMPS_ACTION_SEND_ACTION_FRAME\n");
		heRlmSendSMPSActionFrame(prAdapter, prStaRec);
		break;

	default:
		DBGLOG(RLM, INFO,
			"Action %u not supported\n", prSMPSCtrl->ucCtrlAction);
		break;
	}
}
#endif

#if (CFG_SUPPORT_BTWT == 1)
void heRlmRecBTWTparams(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	const uint8_t *pucIE)
{
	uint32_t u4Offset;
	struct _IE_BTWT_T *prBTWTIE = NULL;
	const uint8_t *pucBTWT_PARAMS_HEAD = NULL;
	const uint8_t *pucBTWT_PARAMS = NULL;
	struct _IE_BTWT_PARAMS_T *p_IE_BTWT_PARAMS = NULL;
	struct _TWT_PARAMS_T  *prTWT_PARAMS = NULL;
	uint8_t ucBtwtId = 0;
	uint64_t u8TargetWakeTime = 0;
	uint64_t u8Temp = 0;
	uint64_t u8twt_interval = 0;
	uint64_t u8Mod = 0;
	struct BSS_INFO *prBssInfo;
	uint8_t ucRecommendation = 0;
	struct _TWT_FLOW_T *prTWTFlow = NULL;
#if (CFG_SUPPORT_RTWT == 1)
	/* The RTWT IE handling must be backward compatible with BTWT IE */
	struct _IE_RTWT_PARAMS_T *p_IE_RTWT_PARAMS = NULL;
	uint8_t ucRtwtTfInfoLen = 0;
	struct _TWT_PARAMS_T  rRTWT_PARAMS = {0};
#endif

	if (prAdapter == NULL)
		return;

	if (prStaRec == NULL)
		return;

	if (pucIE == NULL)
		return;

	prBTWTIE = (struct _IE_BTWT_T *) pucIE;

	if (GET_BTWT_CTRL_NEGO(prBTWTIE->ucCtrl) != 0x2)
		return;

	u4Offset = OFFSET_OF(struct _IE_BTWT_T,	u2ReqType);

	DBGLOG(RLM, WARN, "(struct _IE_BTWT_T, u2ReqType)=%d\n", u4Offset);

	pucBTWT_PARAMS_HEAD = pucIE + u4Offset;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (prBssInfo == NULL)
		return;

	/*
	 * The RTWT IE handling must be backward compatible with BTWT IE
	 * [1]parse BTWT IE and insert BTWT param into
	 *    prStaRec->arBTWTFlow[ID==0].rTWTPeerParams and
	 *    prStaRec->arBTWTFlow[Otherwise].rTWTParams
	 * [2]parse RTWT IE and insert RTWT param into
	 *    prStaRec->arRTWTFlow[ID!=0].rTWTParams,
	 *    RTWT not operate on ID=0!!!!!!
	 */
	for (pucBTWT_PARAMS = pucBTWT_PARAMS_HEAD, u4Offset = 0;
		(pucBTWT_PARAMS != NULL) &&
		(u4Offset < (prBTWTIE->ucLength - 1));
#if (CFG_SUPPORT_RTWT == 1)
		pucBTWT_PARAMS += sizeof(struct _IE_BTWT_PARAMS_T)
					+ ucRtwtTfInfoLen,
		u4Offset += sizeof(struct _IE_BTWT_PARAMS_T)
				+ ucRtwtTfInfoLen
#else
		pucBTWT_PARAMS += sizeof(struct _IE_BTWT_PARAMS_T),
		u4Offset += sizeof(struct _IE_BTWT_PARAMS_T)
#endif
		) {
		p_IE_BTWT_PARAMS = (struct _IE_BTWT_PARAMS_T *)pucBTWT_PARAMS;

		DBGLOG_MEM8(RLM, WARN,
			pucBTWT_PARAMS, sizeof(struct _IE_BTWT_PARAMS_T));

		DBGLOG(RLM, WARN,
			"BTWT ReqType=%x TWT=%x MinWakeDur=%x\n",
			p_IE_BTWT_PARAMS->u2ReqType,
			p_IE_BTWT_PARAMS->u2TWT,
			p_IE_BTWT_PARAMS->ucMinWakeDur);

		DBGLOG(RLM, WARN,
			"BTWT WakeIntMantissa=%x BTWTInfo=%x\n",
			p_IE_BTWT_PARAMS->u2WakeIntvalMantiss,
			p_IE_BTWT_PARAMS->u2BTWTInfo);

		ucBtwtId = GET_BTWT_ID(p_IE_BTWT_PARAMS->u2BTWTInfo);

		ucRecommendation = GET_BTWT_RECOMMENDATION(
					p_IE_BTWT_PARAMS->u2ReqType);

		DBGLOG(RLM, WARN, "BTWT[%d] Ofset=%d recomd=%d\n",
			ucBtwtId, u4Offset, ucRecommendation);

		if (ucBtwtId >= RTWT_MAX_FLOW_NUM) {
			DBGLOG(RLM, ERROR,
				"BTWT[%d] Ofset=%d recomd=%d out of range\n",
				ucBtwtId, u4Offset, ucRecommendation);

			break;
		}

		/*
		 * We need to consider the case AP supports both BTWT and RTWT
		 * , where BTWT[0|1|2..] and RTWT[1|2|3...], only BTWT support
		 * BTWT_ID = 0!!!
		 */
#if (CFG_SUPPORT_RTWT == 1)
		if (ucRecommendation == 0)
#endif
			prTWTFlow = &prStaRec->arBTWTFlow[ucBtwtId];
#if (CFG_SUPPORT_RTWT == 1)
		else
			prTWTFlow = &prStaRec->arRTWTFlow[ucBtwtId];
#endif

		/*
		 * Native join BTWT_ID[0], otherwise keep whatever TWT
		 * parameters in prStaRec->arBTWTFlow[ucBtwtId].rTWTParams,
		 * this holds for both BTWT and RTWT.
		 */
		if (ucBtwtId == 0)
			prTWT_PARAMS =
				&prTWTFlow->rTWTPeerParams;
		else
			prTWT_PARAMS =
				&prTWTFlow->rTWTParams;

		if (prTWT_PARAMS == NULL)
			break;

#if (CFG_SUPPORT_RTWT == 1)
		if (ucRecommendation == 0) {
#endif

			prTWTFlow->eTwtType = ENUM_TWT_TYPE_BTWT;

			prTWT_PARAMS->ucSetupCmd = 0x0; /* Request TWT by WFA */

#if (CFG_SUPPORT_RTWT == 1)

			ucRtwtTfInfoLen = 0;
		} else {
			prTWTFlow->eTwtType = ENUM_TWT_TYPE_RTWT;

			prTWT_PARAMS->ucSetupCmd = 0x2; /* Demand TWT by WFA */

			if (GET_BTWT_RTWT_TRAFFIC_INFO_PRESENT(
						((struct _IE_BTWT_PARAMS_T *)
						pucBTWT_PARAMS)->u2BTWTInfo)) {
				ucRtwtTfInfoLen =
					RTWT_TRAFFIC_INFO_BYTE_LENGTH;

				/*
				 * Cast _IE_BTWT_PARAMS_T to _IE_RTWT_PARAMS_T,
				 * RTWT = BTWT + RTWT traffic info(3 bytes)
				 */
				p_IE_RTWT_PARAMS =
				(struct _IE_RTWT_PARAMS_T *)pucBTWT_PARAMS;

				prTWT_PARAMS->ucTrafficInfoPresent = 1;
				prTWT_PARAMS->ucDlUlBmpValid =
				p_IE_RTWT_PARAMS->uc_arRTWTTrafficInfo[0];
				prTWT_PARAMS->ucDlBmp =
				p_IE_RTWT_PARAMS->uc_arRTWTTrafficInfo[1];
				prTWT_PARAMS->ucUlBmp =
				p_IE_RTWT_PARAMS->uc_arRTWTTrafficInfo[2];
			} else {
				ucRtwtTfInfoLen = 0;
				prTWT_PARAMS->ucTrafficInfoPresent = 0;
				prTWT_PARAMS->ucDlUlBmpValid = 0;
				prTWT_PARAMS->ucDlBmp = 0;
				prTWT_PARAMS->ucUlBmp = 0;
			}
		}

#endif

		prTWT_PARAMS->ucMinWakeDur = p_IE_BTWT_PARAMS->ucMinWakeDur;
		prTWT_PARAMS->u2WakeIntvalMantiss =
			p_IE_BTWT_PARAMS->u2WakeIntvalMantiss;
		prTWT_PARAMS->u8TWT = 0;
		prTWT_PARAMS->fgReq = 0x1;
		prTWT_PARAMS->fgTrigger = GET_TWT_RT_TRIGGER(
						p_IE_BTWT_PARAMS->u2ReqType);
		prTWT_PARAMS->fgUnannounced = GET_TWT_RT_FLOW_TYPE(
						p_IE_BTWT_PARAMS->u2ReqType);
		prTWT_PARAMS->ucWakeIntvalExponent =
					GET_TWT_RT_WAKE_INTVAL_EXP(
						p_IE_BTWT_PARAMS->u2ReqType);
		prTWT_PARAMS->fgProtect = 0x0;

		if ((prTWTFlow->eTwtType == ENUM_TWT_TYPE_BTWT) &&
			(prTWTFlow->eBtwtState
				== ENUM_BTWT_FLOW_STATE_DEFAULT) &&
			(ucBtwtId == 0)) {
			u8TargetWakeTime = (prStaRec->au4Timestamp[0] |
				(((uint64_t)(prStaRec->au4Timestamp[1]))
				<< 32));

			u8twt_interval = (((u_int64_t)
					prTWT_PARAMS->u2WakeIntvalMantiss)
					<< prTWT_PARAMS->ucWakeIntvalExponent);

			u8Temp = u8TargetWakeTime + u8twt_interval;

			u8Mod = kal_mod64(u8Temp, u8twt_interval);

			prTWT_PARAMS->u8TWT = (u8TargetWakeTime +
						(u8twt_interval - u8Mod));

			/* BTWT[0] by default join */
			btwtPlannerAddAgrtTbl(prAdapter, prBssInfo,
				prStaRec, prTWT_PARAMS, ucBtwtId, FALSE,
				NULL, NULL);
		}

		/*
		 * By WFA RTWT test plan, the RTWT 5.52.1 STAUT stops TXOP
		 * before each active RTWT service period start, because
		 * STAUT's RTXT has not yet activated, the uplink traffic
		 * has been asked not to overlap with active RTWT service
		 * period start.
		 *
		 * We need to setup a local bypass TWT to skip the active
		 * RTWT service period start and with a shrinkaged interval.
		 *
		 * We only do this in WFA logo test!!!
		 */

#if (CFG_SUPPORT_RTWT == 1)
		if ((g_IsTwtLogo == 1) && (g_IsWfaTestBed == 0) &&
			(prTWTFlow->eTwtType == ENUM_TWT_TYPE_RTWT) &&
			(IS_FEATURE_ENABLED(
				prAdapter->rWifiVar.ucRTWTStautProtect))) {
			/*
			 * To etimate current active RTWT target time
			 */
			u8TargetWakeTime = (prStaRec->au4Timestamp[0] |
				(((uint64_t)(prStaRec->au4Timestamp[1]))
				<< 32));

			u8twt_interval = (((u_int64_t)
					prTWT_PARAMS->u2WakeIntvalMantiss)
					<< prTWT_PARAMS->ucWakeIntvalExponent);

			u8Temp = u8TargetWakeTime + u8twt_interval;

			u8Mod = kal_mod64(u8Temp, u8twt_interval);

			if (twtPlannerDrvAgrtFindWithTwtType(
				prAdapter,
				prBssInfo->ucBssIndex,
				ucBtwtId,
				/*
				 * We'd like to know if bypass nego has
				 * been setup previously!!
				 */
				1,
				ENUM_TWT_TYPE_RTWT) >= TWT_AGRT_MAX_NUM) {
				/*
				 * If we reach here, we are safe to add
				 * local bypass nego of ucBtwtId.
				 */
				kalMemCopy(&rRTWT_PARAMS, prTWT_PARAMS,
					sizeof(struct _TWT_PARAMS_T));

				/*
				 * For WFA RTWT 5.52.1, alive RTWT schedule:
				 *     32 * 255 = 8.160ms SP in 32ms interval
				 *
				 * We setup local bypass nego:
				 *     112 * 255 = 28.560ms SP in 32ms interval
				 *
				 * We'd likt to shift 2ms away!!!
				 */
				rRTWT_PARAMS.ucMinWakeDur = 112;

				rRTWT_PARAMS.u8TWT = (u8TargetWakeTime +
						(u8twt_interval - u8Mod));

				rRTWT_PARAMS.u8TWT += 2048;

				rRTWT_PARAMS.fgByPassNego = 1;

				rtwtPlannerAddAgrtTbl(
					prAdapter, prBssInfo,
					prStaRec, &rRTWT_PARAMS,
					ucBtwtId, FALSE,
					NULL, NULL);
			}
		}
#endif

		DBGLOG(RLM, WARN,
			"%sTWT[%d] %x, %x, %x, %x, %x, %x %x, %x, %x, %x\n",
			(prTWTFlow->eTwtType == ENUM_TWT_TYPE_BTWT) ? "B":"R",
			ucBtwtId,
			prTWT_PARAMS->ucMinWakeDur,
			prTWT_PARAMS->u2WakeIntvalMantiss,
			prTWT_PARAMS->fgTrigger,
			prTWT_PARAMS->fgUnannounced,
			prTWT_PARAMS->ucWakeIntvalExponent,
			(prTWT_PARAMS->u8TWT & 0x00000000FFFFFFFF),
			((prTWT_PARAMS->u8TWT &
			0xFFFFFFFF00000000) >> 32),
			prStaRec->au4Timestamp[0],
			prStaRec->au4Timestamp[1],
			p_IE_BTWT_PARAMS->u2TWT);

		if (GET_BTWT_LAST_BCAST(p_IE_BTWT_PARAMS->u2ReqType) == 0x1)
			break;
	}
}

#endif /* CFG_SUPPORT_BTWT == 1 */

void heRlmRspGenerateBssMaxIdleIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucPhyTypeSet;

	if (!prAdapter || !prMsduInfo ||
		IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgBssMaxIdle))
		return;

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo || !IS_BSS_ACTIVE(prBssInfo))
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

	if (RLM_NET_IS_11AX(prBssInfo) &&
	    (ucPhyTypeSet & PHY_TYPE_SET_802_11AX))
		heRlmFillBssMaxIdleIE(prAdapter, prBssInfo, prMsduInfo);
}

void heRlmReqGenerateBssMaxIdleIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;

	if (fgEfuseCtrlAxOn == 1) {

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11AX)
	    && (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11AX)))
		heRlmFillBssMaxIdleIE(prAdapter, prBssInfo, prMsduInfo);
	}
}

static void heRlmFillBssMaxIdleIE(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct MSDU_INFO *prMsduInfo)
{
	struct IE_BSS_MAX_IDLE_PERIOD  *prBssMaxIdleIE;
	uint8_t ucIdleOptions = 0;
	struct STA_RECORD *prStaRec;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	prBssMaxIdleIE = (struct IE_BSS_MAX_IDLE_PERIOD *)
		(((uint8_t *)prMsduInfo->prPacket)+prMsduInfo->u2FrameLength);

	prBssMaxIdleIE->ucId = ELEM_ID_BSS_MAX_IDLE_PERIOD;
	prBssMaxIdleIE->ucLength = ELEM_MAX_LEN_BSS_MAX_IDLE;
	WLAN_SET_FIELD_16(&prBssMaxIdleIE->u2MaxIdlePeriod,
		prAdapter->rWifiVar.u2BssMaxIdlePeriod);

	/* The Protected Keep-Alive Required subfield is set to 1
	 * to indicate that only a protected frame indicates activity.
	 */
	if (prStaRec && (prStaRec->u2CapInfo & CAP_INFO_PRIVACY) &&
		secEnabledInAis(prAdapter, prMsduInfo->ucBssIndex))
		ucIdleOptions = 1;
	else if (IS_BSS_APGO(prBssInfo) &&
		rsnCheckBipKeyInstalled(prAdapter, prStaRec))
		ucIdleOptions = 1;
	prBssMaxIdleIE->ucIdleOptions = ucIdleOptions;

	prMsduInfo->u2FrameLength += IE_SIZE(prBssMaxIdleIE);
}

#endif /* CFG_SUPPORT_802_11AX == 1 */
