// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "bss.c"
 *    \brief  This file contains the functions for creating BSS(AP)/IBSS(AdHoc)
 *
 *   This file contains the functions for BSS(AP)/IBSS(AdHoc).
 *   We may create a BSS/IBSS network, or merge with exist IBSS network
 *   and sending Beacon Frame or reply the Probe Response Frame
 *   for received Probe Request Frame.
 */

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "precomp.h"

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */
/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

const uint8_t *apucNetworkType[NETWORK_TYPE_NUM] = {
	(uint8_t *) "AIS",
	(uint8_t *) "P2P",
	(uint8_t *) "BOW",
	(uint8_t *) "MBSS"
};

const uint8_t *apucNetworkOpMode[OP_MODE_NUM] = {
	(uint8_t *) "INFRASTRUCTURE",
	(uint8_t *) "IBSS",
	(uint8_t *) "ACCESS_POINT",
	(uint8_t *) "P2P_DEVICE",
	(uint8_t *) "BOW",
	(uint8_t *) "NAN"
};

#if (CFG_SUPPORT_ADHOC) || (CFG_SUPPORT_AAA)
struct APPEND_VAR_IE_ENTRY txBcnIETable[] = {
	{(ELEM_HDR_LEN + (RATE_NUM_SW - ELEM_MAX_LEN_SUP_RATES)), NULL,
	 bssGenerateExtSuppRate_IE}	/* 50 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_ERP), NULL,
	   rlmRspGenerateErpIE}	/* 42 */
#if CFG_AP_80211K_SUPPORT
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_RRM_CAP), NULL,
	   rlmMulAPAgentGenerateApRRMEnabledCapIE} /* 70 */
#endif /* CFG_AP_80211K_SUPPORT */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP), NULL,
	   rlmRspGenerateHtCapIE}	/* 45 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_HT_OP), NULL,
	   rlmRspGenerateHtOpIE}	/* 61 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_TPE), NULL,
	   rlmGeneratePwrConstraintIE}		/* 32 */
#if CFG_ENABLE_WIFI_DIRECT
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_OBSS_SCAN), NULL,
	   rlmRspGenerateObssScanIE}	/* 74 */
#endif
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP), NULL,
	   rlmRspGenerateExtCapIE}	/* 127 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
	   rsnGenerateWpaNoneIE}	/* 221 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WMM_PARAM), NULL,
	   mqmGenerateWmmParamIE}	/* 221 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
	   rsnGenerateWPAIE}	/* 221 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_RSN), NULL,
	   rsnGenerateRSNIE}	/* 48 */
#if CFG_ENABLE_WIFI_DIRECT
	, {0, p2pFuncCalculateP2p_IELenForBeacon,
	   p2pFuncGenerateP2p_IEForBeacon}	/* 221 */
	, {0, p2pFuncCalculateWSC_IELenForBeacon,
	   p2pFuncGenerateWSC_IEForBeacon}	/* 221 */
	, {0, p2pFuncCalculateP2P_IE_NoA,
	   p2pFuncGenerateP2P_IE_NoA}	/* 221 */
#endif /* CFG_ENABLE_WIFI_DIRECT */
#if CFG_SUPPORT_802_11AC
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP), NULL,
	   rlmRspGenerateVhtCapIE}	/*191 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP), NULL,
	   rlmRspGenerateVhtOpIE}	/*192 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_TPE), NULL,
	   rlmGenerateVhtTPEIE}	/* 195 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP_MODE_NOTIFICATION), NULL,
	   rlmRspGenerateVhtOpNotificationIE}	/*199 */
#endif
#if CFG_SUPPORT_802_11AX
	, {0, heRlmCalculateHeCapIELen,
	   heRlmRspGenerateHeCapIE}    /* 255, EXT 35 */
	, {0, heRlmCalculateHeOpIELen,
	   heRlmRspGenerateHeOpIE}      /* 255, EXT 36 */
#if (CFG_SUPPORT_WIFI_6G == 1)
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_HE_6G_CAP), NULL,
	   heRlmReqGenerateHe6gBandCapIE} /* 255, EXT 59 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_RNR), NULL,
	   heRlmRspGenerateHeRnrIE} /* 201 */
#endif
#endif
#if CFG_SUPPORT_802_11BE
	, {0, ehtRlmCalculateCapIELen,
	   ehtRlmRspGenerateCapIE}
	, {0, ehtRlmCalculateOpIELen,
	   ehtRlmRspGenerateOpIE}
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	, {0, mldCalculateRnrIELen, mldGenerateRnrIE}
	, {0, mldCalculateMlIELen, mldGenerateMlIE}
#endif
#endif
#if CFG_SUPPORT_MTK_SYNERGY
	, {0, rlmCalculateMTKOuiIELen, rlmGenerateMTKOuiIE}	/* 221 */
#endif
	, {0, rlmCalculateCsaIELen,
	   rlmGenerateCsaIE}	/* 37 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_RSN), NULL,
	   rsnGenerateRSNXIE}	/* 244 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
	   rsnGenerateOWEIE}
	, {0, rlmCalculateTpeIELen, rlmGenerateTpeIE}
};

struct APPEND_VAR_IE_ENTRY txProbRspIETable[] = {
	{(ELEM_HDR_LEN + (RATE_NUM_SW - ELEM_MAX_LEN_SUP_RATES)), NULL,
	 bssGenerateExtSuppRate_IE}	/* 50 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_ERP), NULL,
	   rlmRspGenerateErpIE}	/* 42 */
#if CFG_AP_80211K_SUPPORT
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_RRM_CAP), NULL,
	   rlmMulAPAgentGenerateApRRMEnabledCapIE} /* 70 */
#endif /* CFG_AP_80211K_SUPPORT */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP), NULL,
	   rlmRspGenerateHtCapIE}	/* 45 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_HT_OP), NULL,
	   rlmRspGenerateHtOpIE}	/* 61 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_TPE), NULL,
	   rlmGeneratePwrConstraintIE}		/* 32 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
	   rsnGenerateWPAIE}	/* 221 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_RSN), NULL,
	   rsnGenerateRSNIE}	/* 48 */
#if CFG_ENABLE_WIFI_DIRECT
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_OBSS_SCAN), NULL,
	   rlmRspGenerateObssScanIE}	/* 74 */
#endif
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP), NULL,
	   rlmRspGenerateExtCapIE}	/* 127 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
	   rsnGenerateWpaNoneIE}	/* 221 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WMM_PARAM), NULL,
	   mqmGenerateWmmParamIE}	/* 221 */
#if CFG_SUPPORT_802_11AC
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP), NULL,
	   rlmRspGenerateVhtCapIE}	/*191 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP), NULL,
	   rlmRspGenerateVhtOpIE}	/*192 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_TPE), NULL,
	   rlmGenerateVhtTPEIE}	/* 195 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP_MODE_NOTIFICATION), NULL,
	   rlmRspGenerateVhtOpNotificationIE}	/*199 */
#endif
#if CFG_SUPPORT_802_11AX
	, {0, heRlmCalculateHeCapIELen,
	   heRlmRspGenerateHeCapIE}    /* 255, EXT 35 */
	, {0, heRlmCalculateHeOpIELen,
	   heRlmRspGenerateHeOpIE}      /* 255, EXT 36 */
#endif
#if CFG_SUPPORT_802_11BE
	, {0, ehtRlmCalculateCapIELen,
	   ehtRlmRspGenerateCapIE}
	, {0, ehtRlmCalculateOpIELen,
	   ehtRlmRspGenerateOpIE}
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	, {0, mldCalculateRnrIELen, mldGenerateRnrIE}
	, {0, mldCalculateMlIELen, mldGenerateMlIE}
#endif
#endif
#if CFG_SUPPORT_MTK_SYNERGY
	, {0, rlmCalculateMTKOuiIELen, rlmGenerateMTKOuiIE}
#endif
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_RSN), NULL,
	   rsnGenerateRSNXIE}	/* 244 */
	, {(ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL,
	   rsnGenerateOWEIE}
};

#endif /* CFG_SUPPORT_ADHOC || CFG_SUPPORT_AAA */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */
/*---------------------------------------------------------------------------*/
/* Routines for all Operation Modes                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will decide PHY type set of STA_RECORD_T by given
 *        BSS_DESC_T for Infrastructure or AdHoc Mode.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prBssDesc              Received Beacon/ProbeResp from this STA
 * @param[out] prStaRec              StaRec to be decided PHY type set
 *
 * @retval   VOID
 */
/*---------------------------------------------------------------------------*/
void bssDetermineStaRecPhyTypeSet(struct ADAPTER *prAdapter,
				  struct BSS_DESC *prBssDesc,
				  struct STA_RECORD *prStaRec)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint8_t ucHtOption = FEATURE_ENABLED;
	uint8_t ucVhtOption = FEATURE_ENABLED;
	struct BSS_INFO *prBssInfo;
#if (CFG_SUPPORT_802_11BE == 1)
	uint8_t ucEhtOption = FEATURE_ENABLED;
#endif
	uint32_t u4PTKCipher;
	uint32_t u4GTKCipher;
#if (CFG_SUPPORT_802_11AX == 1)
	uint8_t ucHeOption = FEATURE_ENABLED;

	/* 802.11 AX blocklist */
	if (queryAxBlocklist(prAdapter, prBssDesc->aucBSSID,
			     prStaRec->ucBssIndex, BLOCKLIST_AX_TO_AC)) {
		DBGLOG(BSS, INFO,
		    "BSSID " MACSTR " is in AX blocklist!\n",
		    MAC2STR(prBssDesc->aucBSSID));
		prStaRec->ucPhyTypeSet =
			prBssDesc->ucPhyTypeSet &= ~PHY_TYPE_BIT_HE;
	} else
#endif
		prStaRec->ucPhyTypeSet = prBssDesc->ucPhyTypeSet;

#if CFG_SUPPORT_BFEE
	prStaRec->ucVhtCapNumSoundingDimensions =
	    prBssDesc->ucVhtCapNumSoundingDimensions;
#endif
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prStaRec->ucBssIndex);

	/* Decide AIS PHY type set */
	if (prStaRec->eStaType == STA_TYPE_LEGACY_AP) {
		if (prBssInfo == NULL)
			goto BYPASS_SEC_CHECK;
		u4PTKCipher = prBssInfo->u4RsnSelectedPairwiseCipher;
		u4GTKCipher = prBssInfo->u4RsnSelectedGroupCipher;

		if (GET_SELECTOR_TYPE(u4PTKCipher) == CIPHER_SUITE_TKIP ||
		    GET_SELECTOR_TYPE(u4PTKCipher) == CIPHER_SUITE_WEP40 ||
		    GET_SELECTOR_TYPE(u4PTKCipher) == CIPHER_SUITE_WEP104 ||
		    (!prWifiVar->fgDisGTKCipherCheck &&
		    (GET_SELECTOR_TYPE(u4GTKCipher) == CIPHER_SUITE_TKIP ||
		     GET_SELECTOR_TYPE(u4GTKCipher) == CIPHER_SUITE_WEP40 ||
		     GET_SELECTOR_TYPE(u4GTKCipher) == CIPHER_SUITE_WEP104))) {
			DBGLOG(BSS, STATE,
			       "Ignore the HT/VHT or higher Bit for pairwise/group cipher (0x%08x/0x%08x) as key cipher configed!\n",
			       u4PTKCipher, u4GTKCipher);

			prStaRec->ucPhyTypeSet &=
			    ~(PHY_TYPE_BIT_HT | PHY_TYPE_BIT_VHT);
#if (CFG_SUPPORT_802_11AX == 1)
			prStaRec->ucPhyTypeSet &= ~(PHY_TYPE_BIT_HE);
#endif
#if (CFG_SUPPORT_802_11BE == 1)
			prStaRec->ucPhyTypeSet &= ~(PHY_TYPE_BIT_EHT);
#endif
		}

#if (CFG_SUPPORT_802_11BE == 1)
		if ((prStaRec->ucPhyTypeSet & PHY_TYPE_BIT_EHT) &&
		    !prWifiVar->fgDisSecurityCheck &&
		    !rsnIsKeyMgmtForEht(prAdapter, prBssDesc,
					prStaRec->ucBssIndex)) {
			DBGLOG(BSS, STATE,
			       "Ignore the EHT Bit for AKM suite (0x%x) configed!\n",
			       SWAP32(prBssDesc->u4RsnSelectedAKMSuite));
			prStaRec->ucPhyTypeSet &= ~(PHY_TYPE_BIT_EHT);
		}
#endif

BYPASS_SEC_CHECK:
		ucHtOption = prWifiVar->ucStaHt;
		ucVhtOption = prWifiVar->ucStaVht;
#if (CFG_SUPPORT_802_11AX == 1)
		if (fgEfuseCtrlAxOn == 1)
			ucHeOption = prWifiVar->ucStaHe;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
		if (AIS_INDEX(prAdapter, prStaRec->ucBssIndex) <
			      prAdapter->rWifiVar.u4AisEHTNumber)
			ucEhtOption = prWifiVar->ucStaEht;
		else
			ucEhtOption = FEATURE_DISABLED;
#endif

		if (IS_FEATURE_ENABLED(prWifiVar->ucStaUapsd))
			prStaRec->fgIsUapsdSupported =
				prBssDesc->fgIsUapsdSupported;
	}
#if CFG_ENABLE_WIFI_DIRECT
	/* Decide P2P GC PHY type set */
	else if (prStaRec->eStaType == STA_TYPE_P2P_GO) {
		ucHtOption = prWifiVar->ucP2pGcHt;
		ucVhtOption = prWifiVar->ucP2pGcVht;
#if (CFG_SUPPORT_802_11AX == 1)
		ucHeOption = prWifiVar->ucP2pGcHe;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
		ucEhtOption = prWifiVar->ucP2pGcEht;
#endif
	}
#endif /* CFG_ENABLE_WIFI_DIRECT */
	/* Set HT/VHT capability from Feature Option */
	if (IS_FEATURE_DISABLED(ucHtOption))
		prStaRec->ucPhyTypeSet &= ~PHY_TYPE_BIT_HT;
	else if (IS_FEATURE_FORCE_ENABLED(ucHtOption))
		prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_HT;

	if (IS_FEATURE_DISABLED(ucVhtOption))
		prStaRec->ucPhyTypeSet &= ~PHY_TYPE_BIT_VHT;
	else if (IS_FEATURE_FORCE_ENABLED(ucVhtOption))
		prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;
	else if (prBssDesc && prBssDesc->eBand == BAND_2G4 &&
		IS_FEATURE_DISABLED(prWifiVar->ucVhtIeIn2g)) {
		prStaRec->ucPhyTypeSet &= ~PHY_TYPE_BIT_VHT;
	}

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
	if (IS_FEATURE_DISABLED(ucHeOption))
		prStaRec->ucPhyTypeSet &= ~PHY_TYPE_BIT_HE;
	else if (IS_FEATURE_FORCE_ENABLED(ucHeOption))
		prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_HE;
	}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	if (IS_FEATURE_DISABLED(ucEhtOption))
		prStaRec->ucPhyTypeSet &= ~PHY_TYPE_BIT_EHT;
	else if (IS_FEATURE_FORCE_ENABLED(ucEhtOption))
		prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_EHT;
#endif

	prStaRec->ucDesiredPhyTypeSet =
	    prStaRec->ucPhyTypeSet & prAdapter->rWifiVar.ucAvailablePhyTypeSet;

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will decide PHY type set of BSS_INFO for
 *        AP Mode.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] fgIsApMode             Legacy AP mode or P2P GO
 * @param[out] prBssInfo             BssInfo to be decided PHY type set
 *
 * @retval   VOID
 */
/*----------------------------------------------------------------------------*/
void bssDetermineApBssInfoPhyTypeSet(struct ADAPTER *prAdapter,
				     u_int8_t fgIsPureAp,
				     struct BSS_INFO *prBssInfo)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint8_t ucHtOption;
	uint8_t ucVhtOption;
#if (CFG_SUPPORT_802_11AX == 1)
	uint8_t ucHeOption;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	uint8_t ucEhtOption;
#endif

	/* Decide AP mode PHY type set */
	if (fgIsPureAp) {
		ucHtOption = prWifiVar->ucApHt;
		ucVhtOption = prWifiVar->ucApVht;
#if (CFG_SUPPORT_802_11AX == 1)
		ucHeOption = prWifiVar->ucApHe;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
		ucEhtOption = prWifiVar->ucApEht;
#endif
	}
	/* Decide P2P GO PHY type set */
	else {
		ucHtOption = prWifiVar->ucP2pGoHt;
		ucVhtOption = prWifiVar->ucP2pGoVht;
#if (CFG_SUPPORT_802_11AX == 1)
		ucHeOption = prWifiVar->ucP2pGoHe;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
		ucEhtOption = prWifiVar->ucP2pGoEht;
#endif
	}

	/* Set HT/VHT capability from Feature Option */
	if (IS_FEATURE_DISABLED(ucHtOption))
		prBssInfo->ucPhyTypeSet &= ~PHY_TYPE_BIT_HT;
	else if (IS_FEATURE_FORCE_ENABLED(ucHtOption))
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_HT;
	else if (!fgIsPureAp && IS_FEATURE_ENABLED(ucHtOption))
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_HT;

	if (IS_FEATURE_DISABLED(ucVhtOption)) {
		prBssInfo->ucPhyTypeSet &= ~PHY_TYPE_BIT_VHT;
	} else if (IS_FEATURE_FORCE_ENABLED(ucVhtOption)) {
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;
	} else if (IS_FEATURE_ENABLED(ucVhtOption) &&
			prBssInfo->eBand == BAND_2G4 &&
			prWifiVar->ucVhtIeIn2g &&
			(prBssInfo->ucPhyTypeSet & PHY_TYPE_SET_802_11N)) {
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;
	} else if (!fgIsPureAp &&
			IS_FEATURE_ENABLED(ucVhtOption) &&
			((prBssInfo->eBand == BAND_5G)
#if (CFG_SUPPORT_WIFI_6G == 1)
			|| (prBssInfo->eBand == BAND_6G)
#endif
			)) {
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;
	}

#if (CFG_SUPPORT_802_11AX == 1)
	if (IS_FEATURE_DISABLED(ucHeOption))
		prBssInfo->ucPhyTypeSet &= ~PHY_TYPE_BIT_HE;
	else if (IS_FEATURE_FORCE_ENABLED(ucHeOption))
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_HE;
	else if (!fgIsPureAp && IS_FEATURE_ENABLED(ucHeOption))
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_HE;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	if (IS_FEATURE_DISABLED(ucEhtOption))
		prBssInfo->ucPhyTypeSet &= ~PHY_TYPE_BIT_EHT;
	else if (IS_FEATURE_FORCE_ENABLED(ucEhtOption))
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_EHT;
	else if (!fgIsPureAp && IS_FEATURE_ENABLED(ucEhtOption))
		prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_EHT;
#endif

	prBssInfo->ucPhyTypeSet &= prAdapter->rWifiVar.ucAvailablePhyTypeSet;
#if (CFG_SUPPORT_802_11BE == 1)
	if (!(prBssInfo->ucPhyTypeSet &
		PHY_TYPE_BIT_HE))
		prBssInfo->ucPhyTypeSet &=
			~(PHY_TYPE_BIT_EHT);
#endif

}

uint32_t bssInfoConnType(struct ADAPTER *ad, struct BSS_INFO *bssinfo)
{
	if (bssinfo->eNetworkType == NETWORK_TYPE_AIS) {
		return CONNECTION_INFRA_STA;
	} else if (bssinfo->eNetworkType == NETWORK_TYPE_P2P) {
		if (bssinfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
			return CONNECTION_P2P_GC;
		} else if (bssinfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
#if CFG_ENABLE_WIFI_DIRECT
			if (ad->fgIsP2PRegistered &&
			    !p2pFuncIsAPMode(ad->rWifiVar.prP2PConnSettings[
					bssinfo->u4PrivateData])) {
				return CONNECTION_P2P_GO;
			}
#endif
			return CONNECTION_INFRA_AP;
		} else if (bssinfo->eCurrentOPMode == OP_MODE_P2P_DEVICE) {
			return CONNECTION_P2P_DEVICE;
		}
	} else if (bssinfo->eNetworkType == NETWORK_TYPE_NAN) {
		return CONNECTION_NAN;
	}

	return 0;

}

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will create or reset a STA_RECORD_T by given BSS_DESC_T
 *        for Infrastructure or AdHoc Mode.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] eStaType               Assign STA Type for this STA_RECORD_T
 * @param[in] eNetTypeIndex          Assign Net Type Index for this
 *                                   STA_RECORD_T
 * @param[in] prBssDesc              Received Beacon/ProbeResp from this STA
 *
 * @retval   Pointer to STA_RECORD_T
 */
/*---------------------------------------------------------------------------*/
struct STA_RECORD *bssCreateStaRecFromBssDesc(struct ADAPTER *prAdapter,
					      enum ENUM_STA_TYPE eStaType,
					      uint8_t ucBssIndex,
					      struct BSS_DESC *prBssDesc)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucNonHTPhyTypeSet;
	struct CONNECTION_SETTINGS *prConnSettings;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(BSS, ERROR, "prBssInfo is null\n");
		return NULL;
	}

	/* 4 <1> Get a valid STA_RECORD_T */
	prStaRec =
	    cnmGetStaRecByAddress(prAdapter, ucBssIndex, prBssDesc->aucSrcAddr);
	if (prStaRec) {
		DBGLOG(BSS, WARN,
		       "STA_REC entry exist, cannot acquire new entry for ["
		       MACSTR "]!!\n", MAC2STR(prBssDesc->aucSrcAddr));
		return NULL;
	}

	prStaRec = cnmStaRecAlloc(prAdapter, eStaType, ucBssIndex,
		prBssDesc->aucSrcAddr);
	if (!prStaRec) {
		DBGLOG(BSS, WARN,
		       "STA_REC entry is full, cannot acquire new entry for ["
		       MACSTR "]!!\n", MAC2STR(prBssDesc->aucSrcAddr));
		return NULL;
	}

	prStaRec->ucStaState = STA_STATE_1;
	prStaRec->ucJoinFailureCount = 0;
	prStaRec->ucAuthAlgNum = AUTH_ALGORITHM_NUM;

	/* 4 <2,3> Update information from BSS_DESC to current P_STA_RECORD */
	bssUpdateStaRecFromBssDesc(prAdapter, prBssDesc, prStaRec);

	/* Check for Target BSS's non HT Phy Types */
	ucNonHTPhyTypeSet =
	    prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11ABG;
	if (ucNonHTPhyTypeSet) {

		if (ucNonHTPhyTypeSet & PHY_TYPE_BIT_ERP) {
			prStaRec->ucNonHTBasicPhyType = PHY_TYPE_ERP_INDEX;
		} else if (ucNonHTPhyTypeSet & PHY_TYPE_BIT_OFDM) {
			prStaRec->ucNonHTBasicPhyType = PHY_TYPE_OFDM_INDEX;
		} else {/* if (ucNonHTPhyTypeSet & PHY_TYPE_HR_DSSS_INDEX) */

			prStaRec->ucNonHTBasicPhyType = PHY_TYPE_HR_DSSS_INDEX;
		}

		prStaRec->fgHasBasicPhyType = TRUE;
	} else {
		/* Use mandatory for 11N only BSS */
		{
			/* TODO(Kevin): which value should we set
			 *    for 11n ? ERP ?
			 */
			prStaRec->ucNonHTBasicPhyType = PHY_TYPE_HR_DSSS_INDEX;
		}

		prStaRec->fgHasBasicPhyType = FALSE;
	}

	/* Update non HT Desired Rate Set */
	if (IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
		prStaRec->u2DesiredNonHTRateSet =
			(prStaRec->u2OperationalRateSet &
			prConnSettings->u2DesiredNonHTRateSet);
	} else {
		prStaRec->u2DesiredNonHTRateSet =
			(prStaRec->u2OperationalRateSet & RATE_SET_ALL_ABG);
	}

	/* 4 <4> Update default value */
	prStaRec->fgDiagnoseConnection = FALSE;

	/* 4 <5> Update default value for other Modules */
	/* Determine WMM related parameters for STA_REC */
	mqmProcessScanResult(prAdapter, prBssDesc, prStaRec);

	/* 4 <6> Update Tx Rate */
	/* Update default Tx rate */
	nicTxUpdateStaRecDefaultRate(prAdapter, prStaRec);

	/* Update default op mode to a impossible value */
	prStaRec->ucOpModeInOpNotificationIE = 0xff;

	/* 4 <6> Decide if this BSS 20/40M bandwidth is allowed */
	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet &
	     PHY_TYPE_SET_802_11N) &&
	    (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)) {
		prBssInfo->fgAssoc40mBwAllowed =
			cnmBss40mBwPermitted(prAdapter, ucBssIndex);
	} else {
		prBssInfo->fgAssoc40mBwAllowed = FALSE;
	}
	DBGLOG(RLM, TRACE, "STA 40mAllowed=%d\n",
	       prBssInfo->fgAssoc40mBwAllowed);

	return prStaRec;
}				/* end of bssCreateStaRecFromBssDesc() */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the Null Data frame.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] pucBuffer              Pointer to the frame buffer.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T.
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void bssComposeNullFrame(struct ADAPTER *prAdapter, uint8_t *pucBuffer,
			 struct STA_RECORD *prStaRec)
{
	struct WLAN_MAC_HEADER *prNullFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2FrameCtrl;
	uint8_t ucBssIndex;

	ucBssIndex = prStaRec->ucBssIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(BSS, ERROR, "prBssInfo is null\n");
		return;
	}
	prNullFrame = (struct WLAN_MAC_HEADER *)pucBuffer;

	/* 4 <1> Decide the Frame Control Field */
	u2FrameCtrl = MAC_FRAME_NULL;

	if (IS_AP_STA(prStaRec)) {
		u2FrameCtrl |= MASK_FC_TO_DS;

		if (prStaRec->fgSetPwrMgtBit)
			u2FrameCtrl |= MASK_FC_PWR_MGT;

	} else if (IS_CLIENT_STA(prStaRec)) {
		u2FrameCtrl |= MASK_FC_FROM_DS;
	} else if (IS_DLS_STA(prStaRec)) {
		/* TODO(Kevin) */
	} else {
		/* NOTE(Kevin): We won't send Null frame for IBSS */
		ASSERT(0);
		return;
	}

	/* 4 <2> Compose the Null frame */
	/* Fill the Frame Control field. */
	/* WLAN_SET_FIELD_16(&prNullFrame->u2FrameCtrl, u2FrameCtrl); */
	prNullFrame->u2FrameCtrl = u2FrameCtrl;
	/* NOTE(Kevin): Optimized for ARM */

	/* Fill the Address 1 field with Target Peer Address. */
	COPY_MAC_ADDR(prNullFrame->aucAddr1, prStaRec->aucMacAddr);

	/* Fill the Address 2 field with our MAC Address. */
	COPY_MAC_ADDR(prNullFrame->aucAddr2, prBssInfo->aucOwnMacAddr);

	/* Fill the Address 3 field with Target BSSID. */
	COPY_MAC_ADDR(prNullFrame->aucAddr3, prBssInfo->aucBSSID);

	/* Clear the SEQ/FRAG_NO field(HW won't overide the FRAG_NO,
	 * so we need to clear it).
	 */
	prNullFrame->u2SeqCtrl = 0;

	return;

}				/* end of bssComposeNullFrameHeader() */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the QoS Null Data frame.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] pucBuffer              Pointer to the frame buffer.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T.
 * @param[in] ucUP                   User Priority.
 * @param[in] fgSetEOSP              Set the EOSP bit.
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void
bssComposeQoSNullFrame(struct ADAPTER *prAdapter,
		       uint8_t *pucBuffer, struct STA_RECORD *prStaRec,
		       uint8_t ucUP, u_int8_t fgSetEOSP)
{
	struct WLAN_MAC_HEADER_QOS *prQoSNullFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2FrameCtrl;
	uint16_t u2QosControl;
	uint8_t ucBssIndex;

	ucBssIndex = prStaRec->ucBssIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (prBssInfo == NULL) {
		DBGLOG(BSS, ERROR, "Can't find the valid bssinfo (idx=%d)\n",
			ucBssIndex);
		return;
	}

	prQoSNullFrame = (struct WLAN_MAC_HEADER_QOS *)pucBuffer;

	/* 4 <1> Decide the Frame Control Field */
	u2FrameCtrl = MAC_FRAME_QOS_NULL;

	if (IS_AP_STA(prStaRec)) {
		u2FrameCtrl |= MASK_FC_TO_DS;

		if (prStaRec->fgSetPwrMgtBit)
			u2FrameCtrl |= MASK_FC_PWR_MGT;

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
	/* NOTE(Kevin): Optimized for ARM */

	/* Fill the Address 1 field with Target Peer Address. */
	COPY_MAC_ADDR(prQoSNullFrame->aucAddr1, prStaRec->aucMacAddr);

	/* Fill the Address 2 field with our MAC Address. */
	COPY_MAC_ADDR(prQoSNullFrame->aucAddr2, prBssInfo->aucOwnMacAddr);

	/* Fill the Address 3 field with Target BSSID. */
	COPY_MAC_ADDR(prQoSNullFrame->aucAddr3, prBssInfo->aucBSSID);

	/* Clear the SEQ/FRAG_NO field(HW won't overide the FRAG_NO,
	 * so we need to clear it).
	 */
	prQoSNullFrame->u2SeqCtrl = 0;

	u2QosControl = (uint16_t) (ucUP & WMM_QC_UP_MASK);

	if (fgSetEOSP)
		u2QosControl |= WMM_QC_EOSP;

	/* WLAN_SET_FIELD_16(&prQoSNullFrame->u2QosCtrl, u2QosControl); */
	prQoSNullFrame->u2QosCtrl = u2QosControl;
	/* NOTE(Kevin): Optimized for ARM */

	return;

}				/* end of bssComposeQoSNullFrameHeader() */

/*---------------------------------------------------------------------------*/
/*!
 * @brief Send the Null Frame
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] pfTxDoneHandler    TX Done call back function
 *
 * @retval WLAN_STATUS_RESOURCE  No available resources to send frame.
 * @retval WLAN_STATUS_SUCCESS   Succe]ss.
 */
/*---------------------------------------------------------------------------*/
uint32_t
bssSendNullFrame(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
		 PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	uint16_t u2EstimatedFrameLen;

	/* 4 <1> Allocate a PKT_INFO_T for Null Frame */
	/* Init with MGMT Header Length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD + WLAN_MAC_HEADER_LEN;

	/* Allocate a MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(BSS, WARN, "No PKT_INFO_T for sending Null Frame.\n");
		return WLAN_STATUS_RESOURCES;
	}
	/* 4 <2> Compose Null frame in MSDU_INfO_T. */
	bssComposeNullFrame(prAdapter,
			    (uint8_t *) ((uintptr_t)prMsduInfo->prPacket +
					 MAC_TX_RESERVED_FIELD), prStaRec);
	TX_SET_MMPDU(prAdapter,
		     prMsduInfo,
		     prStaRec->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_HEADER_LEN,
		     WLAN_MAC_HEADER_LEN, pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	/* 4 <4> Inform TXM  to send this Null frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;

}				/* end of bssSendNullFrame() */

/*---------------------------------------------------------------------------*/
/*!
 * @brief Send the QoS Null Frame
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] pfTxDoneHandler    TX Done call back function
 *
 * @retval WLAN_STATUS_RESOURCE  No available resources to send frame.
 * @retval WLAN_STATUS_SUCCESS   Success.
 */
/*---------------------------------------------------------------------------*/
uint32_t
bssSendQoSNullFrame(struct ADAPTER *prAdapter,
		    struct STA_RECORD *prStaRec, uint8_t ucUP,
		    PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	uint16_t u2EstimatedFrameLen;

	/* 4 <1> Allocate a PKT_INFO_T for Null Frame */
	/* Init with MGMT Header Length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD + WLAN_MAC_HEADER_QOS_LEN;

	/* Allocate a MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(BSS, WARN, "No PKT_INFO_T for sending Null Frame.\n");
		return WLAN_STATUS_RESOURCES;
	}
	/* 4 <2> Compose Null frame in MSDU_INfO_T. */
	bssComposeQoSNullFrame(prAdapter,
			       (uint8_t
				*) ((uintptr_t)(prMsduInfo->prPacket) +
				    MAC_TX_RESERVED_FIELD), prStaRec, ucUP,
			       FALSE);

	TX_SET_MMPDU(prAdapter,
		     prMsduInfo,
		     prStaRec->ucBssIndex,
		     prStaRec->ucIndex,
		     WLAN_MAC_HEADER_QOS_LEN, WLAN_MAC_HEADER_QOS_LEN,
		     pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	/* 4 <4> Inform TXM  to send this Null frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;

}				/* end of bssSendQoSNullFrame() */

const char *bssOpBw2Str(struct BSS_INFO *prBssInfo)
{
	static const char * const apucDebug[] = {
		"20",
		"40",
		"80",
		"160",
		"80+80",
		"320-1",
		"320-2",
		"UNKNOWN",
	};
	uint8_t ucBssOpBw =
		rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo);

	if (ucBssOpBw < MAX_BW_UNKNOWN)
		return apucDebug[ucBssOpBw];

	return "UNKNOWN";
}

uint32_t bssGetAliveBssByBand(struct ADAPTER *prAdapter, enum ENUM_BAND eBand,
			      struct BSS_INFO **prBssList)
{
	struct BSS_INFO *bss;
	uint8_t i, ucNumAliveBss = 0;

	for (i = 0; i < MAX_BSSID_NUM; ++i) {
		bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		if (!bss || !IS_BSS_ALIVE(prAdapter, bss))
			continue;
		else if (eBand != BAND_NULL && eBand != BAND_NUM &&
			 bss->eBand != eBand)
			continue;

		prBssList[ucNumAliveBss++] = bss;
		DBGLOG(BSS, INFO,
		       "bss%u (%s), band:%u, ch:%u, bw:%s, hw_band:%u\n",
		       i, bssGetRoleTypeString(prAdapter, bss),
		       bss->eBand, bss->ucPrimaryChannel,
		       bssOpBw2Str(bss), bss->eHwBandIdx);
	}

	return ucNumAliveBss;
}

const char *bssGetRoleTypeString(struct ADAPTER *prAdapter,
				 struct BSS_INFO *bss)
{
	if (IS_BSS_AIS(bss))
		return "STA";
#if CFG_ENABLE_WIFI_DIRECT
	else if (IS_BSS_P2P(bss)) {
		if (IS_BSS_GC(bss))
			return "GC";
		else if (IS_BSS_APGO(bss)) {
			if (p2pFuncIsAPMode(
				prAdapter->rWifiVar.prP2PConnSettings[
					bss->u4PrivateData]))
				return "SAP";
			else
				return "GO";
		}
	}
#endif
	else if (IS_BSS_BOW(bss))
		return "BOW";

	return "Unknown";
}

#if (CFG_SUPPORT_ADHOC) || (CFG_ENABLE_WIFI_DIRECT)
/*---------------------------------------------------------------------------*/
/* Routines for both IBSS(AdHoc) and BSS(AP)                                 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * @brief This function is used to generate Information Elements of Extended
 *        Support Rate
 *
 * @param[in] prAdapter      Pointer to the Adapter structure.
 * @param[in] prMsduInfo     Pointer to the composed MSDU_INFO_T.
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void bssGenerateExtSuppRate_IE(struct ADAPTER *prAdapter,
			       struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	uint8_t *pucBuffer;
	uint8_t ucExtSupRatesLen;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(BSS, ERROR, "prBssInfo is null\n");
		return;
	}
	pucBuffer =
	    (uint8_t *) ((uintptr_t)prMsduInfo->prPacket +
			 (uintptr_t)prMsduInfo->u2FrameLength);

	if (prBssInfo->ucAllSupportedRatesLen > ELEM_MAX_LEN_SUP_RATES)

		ucExtSupRatesLen =
		    prBssInfo->ucAllSupportedRatesLen - ELEM_MAX_LEN_SUP_RATES;
	else
		ucExtSupRatesLen = 0;

	/* Fill the Extended Supported Rates element. */
	if (ucExtSupRatesLen) {

		EXT_SUP_RATES_IE(pucBuffer)->ucId = ELEM_ID_EXTENDED_SUP_RATES;
		EXT_SUP_RATES_IE(pucBuffer)->ucLength = ucExtSupRatesLen;

		kalMemCopy(EXT_SUP_RATES_IE(pucBuffer)->aucExtSupportedRates,
			   &prBssInfo->aucAllSupportedRates
			   [ELEM_MAX_LEN_SUP_RATES], ucExtSupRatesLen);

		prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
	}
}				/* end of bssGenerateExtSuppRate_IE() */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function is used to compose Common Information Elements for
 *        Beacon or Probe Response Frame.
 *
 * @param[in] prMsduInfo     Pointer to the composed MSDU_INFO_T.
 * @param[in] prBssInfo      Pointer to the BSS_INFO_T.
 * @param[in] pucDestAddr    Pointer to the Destination Address,
 *                           if NULL, means Beacon.
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void
bssBuildBeaconProbeRespFrameCommonIEs(struct MSDU_INFO *prMsduInfo,
				      struct BSS_INFO *prBssInfo,
				      uint8_t *pucDestAddr)
{
	uint8_t *pucBuffer;
	uint8_t ucSupRatesLen;

	pucBuffer =
	    (uint8_t *) ((uintptr_t)prMsduInfo->prPacket +
			 (uintptr_t)prMsduInfo->u2FrameLength);
	/* 4 <1> Fill the SSID element. */
	SSID_IE(pucBuffer)->ucId = ELEM_ID_SSID;

	if ((!pucDestAddr)
	    && (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)) {
		/* For Beacon */
		if (prBssInfo->eHiddenSsidType ==
			ENUM_HIDDEN_SSID_ZERO_CONTENT) {
			/* clear the data, but keep the correct
			 * length of the SSID
			 */
			SSID_IE(pucBuffer)->ucLength = prBssInfo->ucSSIDLen;
			kalMemZero(SSID_IE(pucBuffer)->aucSSID,
				   prBssInfo->ucSSIDLen);
		} else if (prBssInfo->eHiddenSsidType ==
			   ENUM_HIDDEN_SSID_ZERO_LEN) {
			/* empty SSID */
			SSID_IE(pucBuffer)->ucLength = 0;
		} else {
			SSID_IE(pucBuffer)->ucLength = prBssInfo->ucSSIDLen;
			if (prBssInfo->ucSSIDLen)
				kalMemCopy(SSID_IE(pucBuffer)->aucSSID,
					   prBssInfo->aucSSID,
					   prBssInfo->ucSSIDLen);
		}
	} else {		/* Probe response */
		SSID_IE(pucBuffer)->ucLength = prBssInfo->ucSSIDLen;
		if (prBssInfo->ucSSIDLen)
			kalMemCopy(SSID_IE(pucBuffer)->aucSSID,
				   prBssInfo->aucSSID, prBssInfo->ucSSIDLen);
	}

	prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
	pucBuffer += IE_SIZE(pucBuffer);

	/* 4 <2> Fill the Supported Rates element. */
	if (prBssInfo->ucAllSupportedRatesLen > ELEM_MAX_LEN_SUP_RATES)

		ucSupRatesLen = ELEM_MAX_LEN_SUP_RATES;
	else
		ucSupRatesLen = prBssInfo->ucAllSupportedRatesLen;

	if (ucSupRatesLen) {
		SUP_RATES_IE(pucBuffer)->ucId = ELEM_ID_SUP_RATES;
		SUP_RATES_IE(pucBuffer)->ucLength = ucSupRatesLen;
		kalMemCopy(SUP_RATES_IE(pucBuffer)->aucSupportedRates,
			   prBssInfo->aucAllSupportedRates, ucSupRatesLen);

		prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
		pucBuffer += IE_SIZE(pucBuffer);
	}

	/* 4 <3> Fill the DS Parameter Set element. */
	if (prBssInfo->eBand == BAND_2G4) {
		DS_PARAM_IE(pucBuffer)->ucId = ELEM_ID_DS_PARAM_SET;
		DS_PARAM_IE(pucBuffer)->ucLength =
		    ELEM_MAX_LEN_DS_PARAMETER_SET;
		DS_PARAM_IE(pucBuffer)->ucCurrChnl =
		    prBssInfo->ucPrimaryChannel;

		prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
		pucBuffer += IE_SIZE(pucBuffer);
	}

	/* 4 <4> IBSS Parameter Set element, ID: 6 */
	if (prBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
		IBSS_PARAM_IE(pucBuffer)->ucId = ELEM_ID_IBSS_PARAM_SET;
		IBSS_PARAM_IE(pucBuffer)->ucLength =
		    ELEM_MAX_LEN_IBSS_PARAMETER_SET;
		WLAN_SET_FIELD_16(&(IBSS_PARAM_IE(pucBuffer)->u2ATIMWindow),
				  prBssInfo->u2ATIMWindow);

		prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
		pucBuffer += IE_SIZE(pucBuffer);
	}

	/* 4 <5> TIM element, ID: 5 */
	if ((!pucDestAddr) &&	/* For Beacon only. */
	    (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)) {
#if CFG_ENABLE_WIFI_DIRECT
		/*no fgIsP2PRegistered protect */
		if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
			/* IEEE 802.11 2007 - 7.3.2.6 */
			TIM_IE(pucBuffer)->ucId = ELEM_ID_TIM;
			/* NOTE: fixed PVB length
			 * (AID is allocated from 8 ~ 15 only)
			 */
			TIM_IE(pucBuffer)->ucLength =
			    (3 +
			     MAX_LEN_TIM_PARTIAL_BMP) /*((u4N2 - u4N1) + 4) */;
			TIM_IE(pucBuffer)->ucDTIMCount =
			    0 /*prBssInfo->ucDTIMCount */;
			/* will be overwritten by FW */
			TIM_IE(pucBuffer)->ucDTIMPeriod =
			    prBssInfo->ucDTIMPeriod;
			/* will be overwritten by FW */
			TIM_IE(pucBuffer)->ucBitmapControl =
			    0 /*ucBitmapControl | (uint8_t)u4N1 */;

			prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
			pucBuffer += IE_SIZE(pucBuffer);
		} else
#endif /* CFG_ENABLE_WIFI_DIRECT */
		{
			/* NOTE(Kevin): 1. AIS - Didn't Support AP Mode.
			 *              2. BOW - Didn't Support BCAST and PS.
			 */
		}
	}
#if CFG_SUPPORT_802_11D
	/* 4 <6> Fill the DS Parameter Set element. */
	if (prBssInfo->ucCountryIELen != 0) {
		COUNTRY_IE(pucBuffer)->ucId = ELEM_ID_COUNTRY_INFO;
		COUNTRY_IE(pucBuffer)->ucLength = prBssInfo->ucCountryIELen;
		COUNTRY_IE(pucBuffer)->aucCountryStr[0] =
			prBssInfo->aucCountryStr[0];
		COUNTRY_IE(pucBuffer)->aucCountryStr[1] =
			prBssInfo->aucCountryStr[1];
		COUNTRY_IE(pucBuffer)->aucCountryStr[2] =
			prBssInfo->aucCountryStr[2];
		kalMemCopy(COUNTRY_IE(pucBuffer)->arCountryStr,
			prBssInfo->aucSubbandTriplet,
			prBssInfo->ucCountryIELen - 3);

		prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
	}
#endif
}			/* end of bssBuildBeaconProbeRespFrameCommonIEs() */

/*---------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the Beacon/Probe Response frame header
 *        and its fixed fields.
 *
 * @param[in] pucBuffer              Pointer to the frame buffer.
 * @param[in] pucDestAddr            Pointer to the Destination Address,
 *                                   if NULL, means Beacon.
 * @param[in] pucOwnMACAddress       Given Our MAC Address.
 * @param[in] pucBSSID               Given BSSID of the BSS.
 * @param[in] u2BeaconInterval       Given Beacon Interval.
 * @param[in] u2CapInfo              Given Capability Info.
 *
 * @return (none)
 */
/*---------------------------------------------------------------------------*/
void
bssComposeBeaconProbeRespFrameHeaderAndFF(uint8_t *pucBuffer,
					  uint8_t *pucDestAddr,
					  uint8_t *pucOwnMACAddress,
					  uint8_t *pucBSSID,
					  uint16_t u2BeaconInterval,
					  uint16_t u2CapInfo)
{
	struct WLAN_BEACON_FRAME *prBcnProbRspFrame;
	uint8_t aucBCAddr[] = BC_MAC_ADDR;
	uint16_t u2FrameCtrl;

	/* DBGLOG(INIT, LOUD, ("\n")); */

	prBcnProbRspFrame = (struct WLAN_BEACON_FRAME *)pucBuffer;

	/* 4 <1> Compose the frame header of the Beacon /ProbeResp frame. */
	/* Fill the Frame Control field. */
	if (pucDestAddr) {
		u2FrameCtrl = MAC_FRAME_PROBE_RSP;
	} else {
		u2FrameCtrl = MAC_FRAME_BEACON;
		pucDestAddr = aucBCAddr;
	}
	/* WLAN_SET_FIELD_16(&prBcnProbRspFrame->u2FrameCtrl, u2FrameCtrl); */
	prBcnProbRspFrame->u2FrameCtrl = u2FrameCtrl;
	/* NOTE(Kevin): Optimized for ARM */

	/* Fill the DA field with BCAST MAC ADDR or TA of ProbeReq. */
	COPY_MAC_ADDR(prBcnProbRspFrame->aucDestAddr, pucDestAddr);

	/* Fill the SA field with our MAC Address. */
	COPY_MAC_ADDR(prBcnProbRspFrame->aucSrcAddr, pucOwnMACAddress);

	/* Fill the BSSID field with current BSSID. */
	COPY_MAC_ADDR(prBcnProbRspFrame->aucBSSID, pucBSSID);

	/* Clear the SEQ/FRAG_NO field(HW won't overide the FRAG_NO,
	 * so we need to clear it).
	 */
	prBcnProbRspFrame->u2SeqCtrl = 0;

	/* 4 <2> Compose the frame body's common fixed field part of the Beacon
	 *       / ProbeResp frame.
	 */
	/* MAC will update TimeStamp field */

	/* Fill the Beacon Interval field. */
	/* WLAN_SET_FIELD_16(&prBcnProbRspFrame->u2BeaconInterval,
	 *      u2BeaconInterval);
	 */
	prBcnProbRspFrame->u2BeaconInterval = u2BeaconInterval;
	/* NOTE(Kevin): Optimized for ARM */

	/* Fill the Capability Information field. */
	/* WLAN_SET_FIELD_16(&prBcnProbRspFrame->u2CapInfo, u2CapInfo); */
	prBcnProbRspFrame->u2CapInfo = u2CapInfo;
	/* NOTE(Kevin): Optimized for ARM */
}		/* end of bssComposeBeaconProbeRespFrameHeaderAndFF() */

uint32_t bssUpdateBeaconContent(struct ADAPTER
				*prAdapter, uint8_t uBssIndex)
{
	return bssUpdateBeaconContentEx(prAdapter,
		uBssIndex,
		IE_UPD_METHOD_UPDATE_ALL);
}

struct MSDU_INFO *bssComposeBeaconContent(struct ADAPTER *prAdapter,
				uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	struct MSDU_INFO *prMsduInfo;
	uint32_t i;

	DBGLOG(INIT, LOUD, "\n");

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(BSS, ERROR, "prBssInfo is null\n");
		return NULL;
	}

	/* 4 <1> Allocate a PKT_INFO_T for Beacon Frame */
	/* Allocate a MSDU_INFO_T */
	/* For Beacon */
	prMsduInfo = prBssInfo->prBeacon;

	/* beacon prMsduInfo will be NULLify once BSS deactivated,
	 * so skip if it is
	 */
	if (prMsduInfo == NULL)
		return NULL;

	/* 4 <2> Compose header */
	bssComposeBeaconProbeRespFrameHeaderAndFF((uint8_t *)
		((uintptr_t)(prMsduInfo->prPacket)
			+ MAC_TX_RESERVED_FIELD),
			NULL,
			prBssInfo->aucOwnMacAddr,
			prBssInfo->aucBSSID,
			prBssInfo->u2BeaconInterval,
			prBssInfo->u2CapInfo);

	prMsduInfo->u2FrameLength = (WLAN_MAC_MGMT_HEADER_LEN +
				     (TIMESTAMP_FIELD_LEN +
				      BEACON_INTERVAL_FIELD_LEN +
				      CAP_INFO_FIELD_LEN));

	prMsduInfo->ucBssIndex = ucBssIndex;

	/* 4 <3> Compose the frame body's Common IEs of the Beacon frame. */
	bssBuildBeaconProbeRespFrameCommonIEs(prMsduInfo, prBssInfo, NULL);

	/* 4 <4> Compose IEs in MSDU_INFO_T */

#if defined(CFG_SUPPORT_PRE_WIFI7)
	nicTxConfigPktControlFlag(prMsduInfo,
				  MSDU_CONTROL_FLAG_HIDE_INFO, TRUE);
#endif

	/* Append IE for Beacon */
	for (i = 0; i < ARRAY_SIZE(txBcnIETable); i++) {
		if (txBcnIETable[i].pfnAppendIE)
			txBcnIETable[i].pfnAppendIE(prAdapter, prMsduInfo);

	}

	sortMgmtFrameIE(prAdapter, prMsduInfo);

	return prMsduInfo;
}

/*---------------------------------------------------------------------------*/
/*!
 * @brief Update the Beacon Frame Template to FW for AIS AdHoc and P2P GO.
 *
 * @param[in] prAdapter         Pointer to the Adapter structure.
 * @param[in] ucBssIndex        Specify which network reply the Probe Response.
 *
 * @retval WLAN_STATUS_SUCCESS   Success.
 */
/*---------------------------------------------------------------------------*/
uint32_t bssUpdateBeaconContentEx(struct ADAPTER *prAdapter,
				uint8_t ucBssIndex,
				enum ENUM_IE_UPD_METHOD eMethod)
{

	struct MSDU_INFO *prMsduInfo;
	struct BSS_INFO *prBssInfo;
	struct WLAN_BEACON_FRAME *prBcnFrame;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!prBssInfo || !prBssInfo->fgIsInUse) {
		DBGLOG(P2P, WARN,
			"bss%d is not in used\n",
			ucBssIndex);
		return 0;
	}

	/* if FW receive beacon update before ch grant, beacon will always
	 * in bn0 with Skayhawk.
	 */
	if (!IS_BSS_ACTIVE(prBssInfo) || prBssInfo->fgIsSwitchingChnl) {
		DBGLOG(P2P, TRACE,
		       "skip update beacon to FW, active=%u, chnlSwitching=%u",
		       IS_BSS_ACTIVE(prBssInfo), prBssInfo->fgIsSwitchingChnl);
		return 0;
	}

	prMsduInfo = bssComposeBeaconContent(prAdapter, ucBssIndex);
	if (!prMsduInfo)
		return WLAN_STATUS_SUCCESS;

	DBGLOG(P2P, TRACE, "Dump beacon content %d to FW, method:%d\n",
		ucBssIndex, eMethod);
	if (aucDebugModule[DBG_P2P_IDX] & DBG_CLASS_TRACE) {
		dumpMemory8((uint8_t *) prMsduInfo->prPacket,
			(uint32_t) prMsduInfo->u2FrameLength);
	}

	prBcnFrame = (struct WLAN_BEACON_FRAME *)prMsduInfo->prPacket;
	return nicUpdateBeaconIETemplate(prAdapter,
				 eMethod,
				 ucBssIndex,
				 prBssInfo->u2CapInfo,
				 (uint8_t *) prBcnFrame->aucInfoElem,
				 prMsduInfo->u2FrameLength -
				 OFFSET_OF(struct WLAN_BEACON_FRAME,
					   aucInfoElem));
}				/* end of bssUpdateBeaconContent() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Send the Beacon Frame(for BOW) or Probe Response Frame according to
 *        the given Destination Address.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] ucBssIndex         Specify which network reply the Probe Response.
 * @param[in] pucDestAddr        Pointer to the Destination Address to reply
 * @param[in] u4ControlFlags     Control flags for information on
 *                               Probe Response.
 *
 * @retval WLAN_STATUS_RESOURCE  No available resources to send frame.
 * @retval WLAN_STATUS_SUCCESS   Success.
 */
/*----------------------------------------------------------------------------*/
uint32_t
bssSendBeaconProbeResponse(struct ADAPTER *prAdapter,
			   uint8_t ucBssIndex, uint8_t *pucDestAddr,
			   uint32_t u4ControlFlags)
{
	struct BSS_INFO *prBssInfo;
	struct MSDU_INFO *prMsduInfo;
	uint16_t u2EstimatedFrameLen;
	uint16_t u2EstimatedFixedIELen;
	uint16_t u2EstimatedExtraIELen;
	struct APPEND_VAR_IE_ENTRY *prIeArray = NULL;
	uint32_t u4IeArraySize = 0;
	uint32_t i;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!pucDestAddr) {	/* For Beacon */
		prIeArray = &txBcnIETable[0];
		u4IeArraySize = ARRAY_SIZE(txBcnIETable);
	} else {
		prIeArray = &txProbRspIETable[0];
		u4IeArraySize = ARRAY_SIZE(txProbRspIETable);
	}

	/* 4 <1> Allocate a PKT_INFO_T for Beacon /Probe Response Frame */
	/* Allocate a MSDU_INFO_T */

	/* Init with MGMT Header Length + Length of Fixed Fields
	 *     + Common IE Fields
	 */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
	    WLAN_MAC_MGMT_HEADER_LEN +
	    TIMESTAMP_FIELD_LEN +
	    BEACON_INTERVAL_FIELD_LEN +
	    CAP_INFO_FIELD_LEN +
	    (ELEM_HDR_LEN + ELEM_MAX_LEN_SSID) +
	    (ELEM_HDR_LEN + ELEM_MAX_LEN_SUP_RATES) +
	    (ELEM_HDR_LEN + ELEM_MAX_LEN_DS_PARAMETER_SET) +
	    (ELEM_HDR_LEN + ELEM_MAX_LEN_IBSS_PARAMETER_SET) +
	    (ELEM_HDR_LEN + (3 + MAX_LEN_TIM_PARTIAL_BMP));

	/* + Extra IE Length */
	u2EstimatedExtraIELen = 0;

	for (i = 0; i < u4IeArraySize; i++) {
		u2EstimatedFixedIELen = prIeArray[i].u2EstimatedFixedIELen;

		if (u2EstimatedFixedIELen) {
			u2EstimatedExtraIELen += u2EstimatedFixedIELen;
		} else {
			ASSERT(prIeArray[i].pfnCalculateVariableIELen);

			u2EstimatedExtraIELen += (uint16_t)
			    prIeArray[i].pfnCalculateVariableIELen(prAdapter,
								   ucBssIndex,
								   NULL);
		}
	}

	u2EstimatedFrameLen += u2EstimatedExtraIELen;
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(BSS, WARN, "No PKT_INFO_T for sending %s.\n",
		       ((!pucDestAddr) ? "Beacon" : "Probe Response"));
		return WLAN_STATUS_RESOURCES;
	}

	/* 4 <2> Compose Beacon/Probe Response frame header
	 * and fixed fields in MSDU_INfO_T.
	 */
	/* Compose Header and Fixed Field */
#if CFG_ENABLE_WIFI_DIRECT
	if (u4ControlFlags & BSS_PROBE_RESP_USE_P2P_DEV_ADDR) {
		if (prAdapter->fgIsP2PRegistered) {
			bssComposeBeaconProbeRespFrameHeaderAndFF((uint8_t *)
				((uintptr_t)
				(prMsduInfo->prPacket) + MAC_TX_RESERVED_FIELD),
				pucDestAddr,
				prAdapter->rWifiVar.aucP2pDeviceAddress[0],
				prAdapter->rWifiVar.aucP2pDeviceAddress[0],
				DOT11_BEACON_PERIOD_DEFAULT,
				(prBssInfo->u2CapInfo & ~(CAP_INFO_ESS
							   | CAP_INFO_IBSS)));
		}
	} else
#endif /* CFG_ENABLE_WIFI_DIRECT */
	{
		bssComposeBeaconProbeRespFrameHeaderAndFF((uint8_t *)
			  ((uintptr_t)
			   (prMsduInfo->prPacket) + MAC_TX_RESERVED_FIELD),
			   pucDestAddr, prBssInfo->aucOwnMacAddr,
			   prBssInfo->aucBSSID,
			   prBssInfo->u2BeaconInterval, prBssInfo->u2CapInfo);
	}

	/* 4 <3> Update information of MSDU_INFO_T */

	TX_SET_MMPDU(prAdapter,
		     prMsduInfo,
		     ucBssIndex,
		     STA_REC_INDEX_NOT_FOUND,
		     WLAN_MAC_MGMT_HEADER_LEN,
		     (WLAN_MAC_MGMT_HEADER_LEN + TIMESTAMP_FIELD_LEN +
		      BEACON_INTERVAL_FIELD_LEN + CAP_INFO_FIELD_LEN), NULL,
		     MSDU_RATE_MODE_AUTO);

	/* 4 <4> Compose the frame body's Common IEs of
	 *       the Beacon/ProbeResp frame.
	 */
	bssBuildBeaconProbeRespFrameCommonIEs(prMsduInfo, prBssInfo,
					      pucDestAddr);

#if defined(CFG_SUPPORT_PRE_WIFI7)
	nicTxConfigPktControlFlag(prMsduInfo,
				  MSDU_CONTROL_FLAG_HIDE_INFO, TRUE);
#endif

	/* 4 <5> Compose IEs in MSDU_INFO_T */

	/* Append IE */
	for (i = 0; i < u4IeArraySize; i++) {
		if (prIeArray[i].pfnAppendIE)
			prIeArray[i].pfnAppendIE(prAdapter, prMsduInfo);

	}

	/* Set limited retry count and lifetime for Probe Resp is reasonable */
	nicTxSetPktLifeTime(prAdapter, prMsduInfo, 100);
	nicTxSetPktRetryLimit(prMsduInfo, 2);

	/* TODO(Kevin):
	 *    Also release the unused tail room of the composed MMPDU
	 */

	/* 4 <6> Inform TXM  to send this Beacon /Probe Response frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;

}				/* end of bssSendBeaconProbeResponse() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will process the Rx Probe Request Frame and then send
 *        back the corresponding Probe Response Frame if the specified
 *        conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Always return success
 */
/*----------------------------------------------------------------------------*/
uint32_t bssProcessProbeRequest(struct ADAPTER *prAdapter,
				struct SW_RFB *prSwRfb)
{
	struct WLAN_MAC_MGMT_HEADER *prMgtHdr;
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex;
	uint8_t aucBCBSSID[] = BC_BSSID;
	u_int8_t fgIsBcBssid;
	u_int8_t fgReplyProbeResp;
	uint32_t u4CtrlFlagsForProbeResp = 0;
	enum ENUM_BAND eBand = 0;
	uint8_t ucHwChannelNum = 0;
	struct RX_DESC_OPS_T *prRxDescOps;
#if CFG_AP_80211KVR_INTERFACE
	uint8_t ucIndex;
	struct T_MULTI_AP_STA_UNASSOC_METRICS *prUnAssocSTA;
	uint8_t aucNullAddr[] = NULL_MAC_ADDR;
	struct HW_MAC_RX_STS_GROUP_3 *prRxStatusGroup3;
#endif

	prRxDescOps = prAdapter->chip_info->prRxDescOps;
	/* 4 <1> Parse Probe Req and Get BSSID */
	prMgtHdr = (struct WLAN_MAC_MGMT_HEADER *)prSwRfb->pvHeader;

	if (EQUAL_MAC_ADDR(aucBCBSSID, prMgtHdr->aucBSSID))
		fgIsBcBssid = TRUE;
	else
		fgIsBcBssid = FALSE;

	/* 4 <2> Check network conditions before reply Probe Response Frame
	 *         (Consider Concurrent)
	 */
	for (ucBssIndex = 0; ucBssIndex <= MAX_BSSID_NUM;
	     ucBssIndex++) {

		if (!IS_NET_ACTIVE(prAdapter, ucBssIndex))
			continue;

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

		if ((!prBssInfo) || ((!fgIsBcBssid)
		    && UNEQUAL_MAC_ADDR(prBssInfo->aucBSSID,
					prMgtHdr->aucBSSID)))
			continue;

		eBand = prSwRfb->eRfBand;
		ucHwChannelNum = prSwRfb->ucChnlNum;

		if (prBssInfo->eBand != eBand)
			continue;

		if (prBssInfo->ucPrimaryChannel != ucHwChannelNum)
			continue;

		fgReplyProbeResp = FALSE;

		if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
#if CFG_SUPPORT_ADHOC
			fgReplyProbeResp =
			    aisValidateProbeReq(prAdapter, prSwRfb,
						ucBssIndex,
						&u4CtrlFlagsForProbeResp);
#endif
		}
#if CFG_ENABLE_WIFI_DIRECT
		else if ((prAdapter->fgIsP2PRegistered)
			 && (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)) {

			fgReplyProbeResp =
			    p2pFuncValidateProbeReq(prAdapter, prSwRfb,
						    &u4CtrlFlagsForProbeResp,
						    (prBssInfo->ucBssIndex ==
						     prAdapter->ucP2PDevBssIdx),
						    (uint8_t)
						    prBssInfo->u4PrivateData);
#if CFG_AP_80211KVR_INTERFACE
			for (ucIndex = 0;
				ucIndex < SAP_UNASSOC_METRICS_STA_MAX;
				ucIndex++) {
				prUnAssocSTA =
					&prBssInfo->arUnAssocSTA[ucIndex];

				if (EQUAL_MAC_ADDR(prUnAssocSTA->mStaMac,
					prMgtHdr->aucSrcAddr)
					&& UNEQUAL_MAC_ADDR(aucNullAddr,
					prMgtHdr->aucSrcAddr)) {
					prRxStatusGroup3 =
					prSwRfb->prRxStatusGroup3;
					prUnAssocSTA->iRssi =
						RCPI_TO_dBm(
						((prRxStatusGroup3)
						->u4RxVector[3]
						& RX_VT_RCPI0_MASK)
						>> RX_VT_RCPI0_OFFSET);
					prUnAssocSTA->u8Channel =
						prBssInfo->ucPrimaryChannel;
					prUnAssocSTA->uTime = kalGetTimeTick();
				}
			}
#endif
		}
#endif
#if CFG_ENABLE_BT_OVER_WIFI
		else if (prBssInfo->eNetworkType == NETWORK_TYPE_BOW)
			fgReplyProbeResp =
			    bowValidateProbeReq(prAdapter, prSwRfb,
						&u4CtrlFlagsForProbeResp);
#endif

		if (fgReplyProbeResp) {
			if (nicTxGetFreeCmdCount(prAdapter) >
			    (CFG_TX_MAX_CMD_PKT_NUM / 2)) {
				/* Resource margin is enough */
				bssSendBeaconProbeResponse(prAdapter,
						   ucBssIndex,
						   prMgtHdr->aucSrcAddr,
						   u4CtrlFlagsForProbeResp);
			}
		}
	}

	return WLAN_STATUS_SUCCESS;

}				/* end of bssProcessProbeRequest() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to initialize the client list for
 *        AdHoc or AP Mode
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prBssInfo              Given related BSS_INFO_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssInitializeClientList(struct ADAPTER *prAdapter,
			     struct BSS_INFO *prBssInfo)
{
	struct LINK *prStaRecOfClientList;

	prStaRecOfClientList = &prBssInfo->rStaRecOfClientList;

	if (!LINK_IS_EMPTY(prStaRecOfClientList))
		LINK_INITIALIZE(prStaRecOfClientList);

	DBGLOG(BSS, INFO, "Init BSS[%u] Client List\n", prBssInfo->ucBssIndex);

	bssCheckClientList(prAdapter, prBssInfo);
}				/* end of bssClearClientList() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to Add a STA_RECORD_T to the client list
 *        for AdHoc or AP Mode
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prBssInfo              Given related BSS_INFO_T.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssAddClient(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo,
		  struct STA_RECORD *prStaRec)
{
	struct LINK *prClientList;
	struct STA_RECORD *prCurrStaRec;

	prClientList = &prBssInfo->rStaRecOfClientList;

	LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
			    struct STA_RECORD) {

		if (!prCurrStaRec)
			break;

		if (prCurrStaRec == prStaRec) {
			DBGLOG(BSS, WARN,
			       "Current Client List already contains that struct STA_RECORD["
			       MACSTR "]\n", MAC2STR(prStaRec->aucMacAddr));
			return;
		}
	}

	LINK_INSERT_TAIL(prClientList, &prStaRec->rLinkEntry);

	bssCheckClientList(prAdapter, prBssInfo);
}				/* end of bssAddStaRecToClientList() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to Remove a STA_RECORD_T from the client list
 *        for AdHoc or AP Mode
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
u_int8_t bssRemoveClient(struct ADAPTER *prAdapter,
			 struct BSS_INFO *prBssInfo,
			 struct STA_RECORD *prStaRec)
{
	struct LINK *prClientList;
	struct STA_RECORD *prCurrStaRec;

	prClientList = &prBssInfo->rStaRecOfClientList;

	LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
			    struct STA_RECORD) {

		/* Break to check client list */
		if (!prCurrStaRec)
			break;

		if (prCurrStaRec == prStaRec) {

			LINK_REMOVE_KNOWN_ENTRY(prClientList,
						&prStaRec->rLinkEntry);

			return TRUE;
		}
	}

	DBGLOG(BSS, INFO,
	       "Current Client List didn't contain that struct STA_RECORD["
	       MACSTR "] before removing.\n", MAC2STR(prStaRec->aucMacAddr));

	bssCheckClientList(prAdapter, prBssInfo);

	return FALSE;
}				/* end of bssRemoveStaRecFromClientList() */

struct STA_RECORD *bssRemoveClientByMac(struct ADAPTER *prAdapter,
					struct BSS_INFO *prBssInfo,
					uint8_t *pucMac)
{
	struct LINK *prClientList;
	struct STA_RECORD *prCurrStaRec;

	prClientList = &prBssInfo->rStaRecOfClientList;

	LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
			    struct STA_RECORD) {

		if (!prCurrStaRec)
			break;

		if (EQUAL_MAC_ADDR(prCurrStaRec->aucMacAddr, pucMac)) {

			LINK_REMOVE_KNOWN_ENTRY(prClientList,
						&prCurrStaRec->rLinkEntry);

			return prCurrStaRec;
		}
	}

	DBGLOG(BSS, INFO,
	       "Current Client List didn't contain that struct STA_RECORD["
	       MACSTR "] before removing.\n", MAC2STR(pucMac));

	bssCheckClientList(prAdapter, prBssInfo);

	return NULL;
}

struct STA_RECORD *bssGetClientByMac(struct ADAPTER *prAdapter,
				     struct BSS_INFO *prBssInfo,
				     uint8_t *pucMac)
{
	struct LINK *prClientList;
	struct STA_RECORD *prCurrStaRec;

	prClientList = &prBssInfo->rStaRecOfClientList;

	LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
			    struct STA_RECORD) {

		if (!prCurrStaRec)
			break;

		if (EQUAL_MAC_ADDR(prCurrStaRec->aucMacAddr, pucMac))
			return prCurrStaRec;
	}

	DBGLOG(BSS, INFO,
	       "Current Client List didn't contain that struct STA_RECORD["
	       MACSTR "] before removing.\n", MAC2STR(pucMac));

	bssCheckClientList(prAdapter, prBssInfo);

	return NULL;
}

struct STA_RECORD *bssRemoveHeadClient(struct ADAPTER *prAdapter,
				       struct BSS_INFO *prBssInfo)
{
	struct LINK *prStaRecOfClientList;
	struct STA_RECORD *prStaRec = NULL;

	prStaRecOfClientList = &prBssInfo->rStaRecOfClientList;

	if (!LINK_IS_EMPTY(prStaRecOfClientList))
		LINK_REMOVE_HEAD(prStaRecOfClientList, prStaRec,
				 struct STA_RECORD *);

	bssCheckClientList(prAdapter, prBssInfo);

	return prStaRec;
}

uint32_t bssGetClientCount(struct ADAPTER *prAdapter,
			   struct BSS_INFO *prBssInfo)
{
	return prBssInfo->rStaRecOfClientList.u4NumElem;
}

void bssDumpClientList(struct ADAPTER *prAdapter,
		       struct BSS_INFO *prBssInfo)
{
	struct LINK *prClientList;
	struct STA_RECORD *prCurrStaRec;
	uint8_t ucCount = 0;

	prClientList = &prBssInfo->rStaRecOfClientList;

	DBGLOG(SW4, INFO, "Dump BSS[%u] Client List NUM[%u]\n",
	       prBssInfo->ucBssIndex, prClientList->u4NumElem);

	LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
			    struct STA_RECORD) {

		if (!prCurrStaRec) {
			DBGLOG(SW4, INFO, "[%2u] is NULL STA_REC\n", ucCount);
			break;
		}
		DBGLOG(SW4, INFO, "[%2u] STA[%u] [" MACSTR "]\n", ucCount,
		       prCurrStaRec->ucIndex,
		       MAC2STR(prCurrStaRec->aucMacAddr));

		ucCount++;
	}
}

void bssCheckClientList(struct ADAPTER *prAdapter,
			struct BSS_INFO *prBssInfo)
{
	struct LINK *prClientList;
	struct STA_RECORD *prCurrStaRec;
	uint8_t ucCount = 0;
	u_int8_t fgError = FALSE;

	prClientList = &prBssInfo->rStaRecOfClientList;

	/* Check MAX number */
	if (prClientList->u4NumElem > P2P_MAXIMUM_CLIENT_COUNT) {
		DBGLOG(SW4, INFO, "BSS[%u] Client List NUM[%u] ERR\n",
		       prBssInfo->ucBssIndex, prClientList->u4NumElem);

		fgError = TRUE;
	}

	/* Check default list status */
	if (prClientList->u4NumElem == 0) {
		if ((void *)prClientList->prNext != (void *)prClientList)
			fgError = TRUE;
		if ((void *)prClientList->prPrev != (void *)prClientList)
			fgError = TRUE;

		if (fgError) {
			DBGLOG(SW4, INFO,
			       "BSS[%u] Client List PTR next/prev[%p/%p] ERR\n",
			       prBssInfo->ucBssIndex, prClientList->prNext,
			       prClientList->prPrev);
		}
	}

	/* Traverse list */
	LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
			    struct STA_RECORD) {
		if (!prCurrStaRec) {
			fgError = TRUE;
			DBGLOG(SW4, INFO, "BSS[%u] Client List NULL PTR ERR\n",
			       prBssInfo->ucBssIndex);

			break;
		}

		ucCount++;
	}

	/* Check real count and list number */
	if (ucCount != prClientList->u4NumElem) {
		DBGLOG(SW4, INFO,
		       "BSS[%u] Client List NUM[%u] REAL CNT[%u] ERR\n",
		       prBssInfo->ucBssIndex, prClientList->u4NumElem, ucCount);

		fgError = TRUE;
	}

	if (fgError)
		bssDumpClientList(prAdapter, prBssInfo);

}

#endif /* CFG_SUPPORT_ADHOC || CFG_SUPPORT_AAA */

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/* Routines for IBSS(AdHoc) only                                              */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to process Beacons from current Ad-Hoc network
 *        peers. We also process Beacons from other Ad-Hoc network during SCAN.
 *        If it has the same SSID and we'll decide to merge into it
 *        if it has a larger TSF.
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] prBssInfo  Pointer to the BSS_INFO_T.
 * @param[in] prBSSDesc  Pointer to the BSS Descriptor.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void
ibssProcessMatchedBeacon(struct ADAPTER *prAdapter,
			 struct BSS_INFO *prBssInfo,
			 struct BSS_DESC *prBssDesc, uint8_t ucRCPI)
{
	struct STA_RECORD *prStaRec = NULL;

	u_int8_t fgIsCheckCapability = FALSE;
	u_int8_t fgIsCheckTSF = FALSE;
	u_int8_t fgIsGoingMerging = FALSE;
	u_int8_t fgIsSameBSSID;

	/* 4 <1> Process IBSS Beacon only after we create or merge
	 *         with other IBSS.
	 */
	if (!prBssInfo->fgIsBeaconActivated)
		return;

	/* 4 <2> Get the STA_RECORD_T of TA. */
	prStaRec =
	    cnmGetStaRecByAddress(prAdapter,
				  prBssInfo->ucBssIndex,
				  prBssDesc->aucSrcAddr);

	fgIsSameBSSID =
	    UNEQUAL_MAC_ADDR(prBssInfo->aucBSSID,
			     prBssDesc->aucBSSID) ? FALSE : TRUE;

	/* 4 <3> IBSS Merge Decision Flow for Processing Beacon. */
	if (fgIsSameBSSID) {

		/* Same BSSID:
		 * Case I.
		 *      This is a new TA and it has decide to merged with us.
		 *      a) If fgIsMerging == FALSE
		 *            - we will send msg to notify AIS.
		 *      b) If fgIsMerging == TRUE
		 *            - already notify AIS.
		 * Case II.
		 *      This is an old TA and we've already merged together.
		 */
		if (!prStaRec) {
			/* For Case I -
			 *     Check this IBSS's capability first before
			 *     adding this Sta Record.
			 */
			fgIsCheckCapability = TRUE;

			/* If check is passed, then we perform merging
			 *    with this new IBSS
			 */
			fgIsGoingMerging = TRUE;

		} else {
			if (prStaRec->ucStaState != STA_STATE_3) {

				if (!prStaRec->fgIsMerging) {

					/* For Case I -
					 * Check this IBSS's capability first
					 *  before adding this Sta Record.
					 */
					fgIsCheckCapability = TRUE;

					/* If check is passed, then we perform
					 * merging with this new IBSS
					 */
					fgIsGoingMerging = TRUE;
				} else {
					/* For Case II - Update rExpirationTime
					 * of Sta Record
					 */
					GET_CURRENT_SYSTIME
					    (&prStaRec->rUpdateTime);
				}
			} else {
				/* For Case II
				 * - Update rExpirationTime of Sta Record
				 */
				GET_CURRENT_SYSTIME(&prStaRec->rUpdateTime);
			}

		}
	} else {

		/* Unequal BSSID:
		 * Case III. This is a new TA and we need to compare
		 *               the TSF and get the winner.
		 * Case IV.  This is an old TA and it merge into
		 *               a new IBSS before we do the same thing.
		 *               We need to compare the TSF to get the winner.
		 * Case V.   This is an old TA and it restart a new IBSS.
		 *               We also need to compare the TSF to
		 *               get the winner.
		 */

		/* For Case III, IV & V - We'll always check this new IBSS's
		 *     capability first before merging into new IBSS.
		 */
		fgIsCheckCapability = TRUE;

		/* If check is passed, we need to perform TSF check to
		 *    decide the major BSSID
		 */
		fgIsCheckTSF = TRUE;

		/* For Case IV & V - We won't update rExpirationTime
		 *    of Sta Record
		 */
	}

	/* 4 <7> Check this BSS_DESC_T's capability. */
	if (fgIsCheckCapability) {
		u_int8_t fgIsCapabilityMatched = FALSE;

		do {
			if (!
			    (prBssDesc->ucPhyTypeSet &
			     (prAdapter->rWifiVar.ucAvailablePhyTypeSet))) {
				DBGLOG(BSS, LOUD,
				       "IBSS MERGE: Ignore Peer MAC: " MACSTR
				       " - Unsupported Phy.\n",
				       MAC2STR(prBssDesc->aucSrcAddr));

				break;
			}

			if (prBssDesc->fgIsUnknownBssBasicRate) {
				DBGLOG(BSS, LOUD,
				       "IBSS MERGE: Ignore Peer MAC: " MACSTR
				       " - Unknown Basic Rate.\n",
				       MAC2STR(prBssDesc->aucSrcAddr));

				break;
			}

			if (ibssCheckCapabilityForAdHocMode
			    (prAdapter, prBssDesc,
			    prBssInfo->ucBssIndex)
			    == WLAN_STATUS_FAILURE) {
				DBGLOG(BSS, LOUD,
				       "IBSS MERGE: Ignore Peer MAC: " MACSTR
				       " - Capability is not matched.\n",
				       MAC2STR(prBssDesc->aucSrcAddr));

				break;
			}

			fgIsCapabilityMatched = TRUE;
		} while (FALSE);

		if (!fgIsCapabilityMatched) {

			if (prStaRec) {
				/* Case II -
				 *  We merge this STA_RECORD in RX Path.
				 * Case IV & V -
				 *  They change their BSSID after
				 *  we merge with them.
				 */

				DBGLOG(BSS, LOUD,
				       "IBSS MERGE: Ignore Peer MAC: " MACSTR
				       " - Capability is not matched.\n",
				       MAC2STR(prBssDesc->aucSrcAddr));
			}

			return;
		}

		DBGLOG(BSS, LOUD,
		       "IBSS MERGE: Peer MAC: " MACSTR
		       " - Check capability was passed.\n",
		       MAC2STR(prBssDesc->aucSrcAddr));
	}

	if (fgIsCheckTSF) {
#if CFG_SLT_SUPPORT
		fgIsGoingMerging = TRUE;
#else
		if (prBssDesc->fgIsLargerTSF)
			fgIsGoingMerging = TRUE;
		else
			return;

#endif
	}

	if (fgIsGoingMerging) {
		struct MSG_AIS_IBSS_PEER_FOUND *prAisIbssPeerFoundMsg;

		/* 4 <1> We will merge with to this BSS immediately. */
		prBssDesc->fgIsConnecting |= BIT(prBssInfo->ucBssIndex);
		prBssDesc->fgIsConnected &= ~BIT(prBssInfo->ucBssIndex);

		/* 4 <2> Setup corresponding STA_RECORD_T */
		prStaRec = bssCreateStaRecFromBssDesc(prAdapter,
						      STA_TYPE_ADHOC_PEER,
						      prBssInfo->ucBssIndex,
						      prBssDesc);

		if (!prStaRec) {
			/* no memory ? */
			return;
		}

		prStaRec->fgIsMerging = TRUE;

		/* update RCPI */
		prStaRec->ucRCPI = ucRCPI;

		/* 4 <3> Send Merge Msg to CNM to obtain
		 *          the channel privilege.
		 */
		prAisIbssPeerFoundMsg = (struct MSG_AIS_IBSS_PEER_FOUND *)
		    cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_AIS_IBSS_PEER_FOUND));

		if (!prAisIbssPeerFoundMsg) {
			DBGLOG(AIS, ERROR, "Can't send Merge Msg\n");
			return;
		}

		prAisIbssPeerFoundMsg->rMsgHdr.eMsgId = MID_SCN_AIS_FOUND_IBSS;
		prAisIbssPeerFoundMsg->ucBssIndex =
		    prBssInfo->ucBssIndex;
		prAisIbssPeerFoundMsg->prStaRec = prStaRec;

		/* Inform AIS to do STATE TRANSITION
		 * For Case I - If AIS in IBSS_ALONE, let it jump to
		 *     NORMAL_TR after we know the new member.
		 * For Case III, IV - Now this new BSSID wins the TSF,
		 *     follow it.
		 */
		if (fgIsSameBSSID) {
			prAisIbssPeerFoundMsg->fgIsMergeIn = TRUE;
		} else {
#if CFG_SLT_SUPPORT
			prAisIbssPeerFoundMsg->fgIsMergeIn = TRUE;
#else
			prAisIbssPeerFoundMsg->fgIsMergeIn =
			    (prBssDesc->fgIsLargerTSF) ? FALSE : TRUE;
#endif
		}

		mboxSendMsg(prAdapter, MBOX_ID_0,
			    (struct MSG_HDR *)prAisIbssPeerFoundMsg,
			    MSG_SEND_METHOD_BUF);

	}
}				/* end of ibssProcessMatchedBeacon() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check the Capability for Ad-Hoc to decide
 *        if we are able to merge with(same capability).
 *
 * @param[in] prBSSDesc  Pointer to the BSS Descriptor.
 *
 * @retval WLAN_STATUS_FAILURE   Can't pass the check of Capability.
 * @retval WLAN_STATUS_SUCCESS   Pass the check of Capability.
 */
/*----------------------------------------------------------------------------*/
uint32_t ibssCheckCapabilityForAdHocMode(struct ADAPTER *prAdapter,
					 struct BSS_DESC *prBssDesc,
					 uint8_t ucBssIndex)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	do {
		/* 4 <1> Check the BSS Basic Rate Set for current AdHoc Mode */
		if ((prConnSettings->eAdHocMode == AD_HOC_MODE_11B) &&
		    (prBssDesc->u2BSSBasicRateSet & ~RATE_SET_HR_DSSS)) {
			break;
		} else if ((prConnSettings->eAdHocMode == AD_HOC_MODE_11A) &&
			   (prBssDesc->u2BSSBasicRateSet & ~RATE_SET_OFDM)) {
			break;
		}
		/* 4 <2> Check the Short Slot Time. */
#if 0
/* Do not check ShortSlotTime until Wi-Fi define such policy */
		if (prConnSettings->eAdHocMode == AD_HOC_MODE_11G) {
			if (((prConnSettings->fgIsShortSlotTimeOptionEnable) &&
			     !(prBssDesc->u2CapInfo & CAP_INFO_SHORT_SLOT_TIME))
			    || (!(prConnSettings->fgIsShortSlotTimeOptionEnable)
				&& (prBssDesc->u2CapInfo &
				    CAP_INFO_SHORT_SLOT_TIME))) {
				break;
			}
		}
#endif

		/* 4 <3> Check the ATIM window setting. */
		if (prBssDesc->u2ATIMWindow) {
			DBGLOG(BSS, INFO,
			       "AdHoc PS was not supported(ATIM Window: %d)\n",
			       prBssDesc->u2ATIMWindow);
			break;
		}
		/* 4 <4> Check the Security setting. */
		if (!rsnPerformPolicySelection(prAdapter, prBssDesc,
			ucBssIndex))
			break;

		rStatus = WLAN_STATUS_SUCCESS;
	} while (FALSE);

	return rStatus;

}				/* end of ibssCheckCapabilityForAdHocMode() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will initial the BSS_INFO_T for IBSS Mode.
 *
 * @param[in] prBssInfo      Pointer to the BSS_INFO_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void ibssInitForAdHoc(struct ADAPTER *prAdapter,
		      struct BSS_INFO *prBssInfo)
{
	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint16_t *pu2BSSID = (uint16_t *) &aucBSSID[0];
	uint32_t i;

	/* 4 <1> Setup PHY Attributes and Basic Rate Set/Operational Rate Set */
	prBssInfo->ucNonHTBasicPhyType = (uint8_t)
	    rNonHTAdHocModeAttributes[prBssInfo->
				      ucConfigAdHocAPMode].ePhyTypeIndex;
	prBssInfo->u2BSSBasicRateSet =
	    rNonHTAdHocModeAttributes[prBssInfo->
				      ucConfigAdHocAPMode].u2BSSBasicRateSet;

	prBssInfo->u2OperationalRateSet =
	    rNonHTPhyAttributes[prBssInfo->
				ucNonHTBasicPhyType].u2SupportedRateSet;

	rateGetDataRatesFromRateSet(prBssInfo->u2OperationalRateSet,
				    prBssInfo->u2BSSBasicRateSet,
				    prBssInfo->aucAllSupportedRates,
				    &prBssInfo->ucAllSupportedRatesLen);

	/* 4 <2> Setup BSSID */
	if (!prBssInfo->fgHoldSameBssidForIBSS) {

		for (i = 0; i < sizeof(aucBSSID) / sizeof(uint16_t); i++)
			pu2BSSID[i] = (uint16_t) (kalRandomNumber() & 0xFFFF);

		/* 7.1.3.3.3 -
		 * The individual/group bit of the address is set to 0.
		 */
		aucBSSID[0] &= ~0x01;
		/* 7.1.3.3.3 -
		 * The universal/local bit of the address is set to 1.
		 */
		aucBSSID[0] |= 0x02;

		COPY_MAC_ADDR(prBssInfo->aucBSSID, aucBSSID);
	}

	/* 4 <3> Setup Capability - Short Preamble */
	if (rNonHTPhyAttributes
	    [prBssInfo->ucNonHTBasicPhyType].fgIsShortPreambleOptionImplemented
	    &&
	    /* Short Preamble Option Enable is TRUE */
	    ((prAdapter->rWifiVar.ePreambleType == PREAMBLE_TYPE_SHORT) ||
	     (prAdapter->rWifiVar.ePreambleType == PREAMBLE_TYPE_AUTO))) {

		prBssInfo->fgIsShortPreambleAllowed = TRUE;
		prBssInfo->fgUseShortPreamble = TRUE;
	} else {
		prBssInfo->fgIsShortPreambleAllowed = FALSE;
		prBssInfo->fgUseShortPreamble = FALSE;
	}

	/* 4 <4> Setup Capability - Short Slot Time */
	/* 7.3.1.4 For IBSS, the Short Slot Time subfield shall be set to 0. */
	prBssInfo->fgUseShortSlotTime = FALSE;	/* Set to FALSE for AdHoc */

	/* 4 <5> Compoase Capability */
	prBssInfo->u2CapInfo = CAP_INFO_IBSS;

	if (prBssInfo->fgIsProtection)
		prBssInfo->u2CapInfo |= CAP_INFO_PRIVACY;

	if (prBssInfo->fgIsShortPreambleAllowed)
		prBssInfo->u2CapInfo |= CAP_INFO_SHORT_PREAMBLE;

	if (prBssInfo->fgUseShortSlotTime)
		prBssInfo->u2CapInfo |= CAP_INFO_SHORT_SLOT_TIME;

	/* 4 <6> Find Lowest Basic Rate Index for default TX Rate of MMPDU */
	nicTxUpdateBssDefaultRate(prBssInfo);
}				/* end of ibssInitForAdHoc() */

#endif /* CFG_SUPPORT_ADHOC */

#if CFG_SUPPORT_AAA

/*----------------------------------------------------------------------------*/
/* Routines for BSS(AP) only                                                  */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will initial the BSS_INFO_T for AP Mode.
 *
 * @param[in] prBssInfo              Given related BSS_INFO_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssInitForAP(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo,
		  u_int8_t fgIsRateUpdate)
{
	struct AC_QUE_PARMS *prACQueParms;

	enum ENUM_WMM_ACI eAci;

	uint8_t auCWminLog2ForBcast[WMM_AC_INDEX_NUM] = { 4, 4, 3, 2 };
	uint8_t auCWmaxLog2ForBcast[WMM_AC_INDEX_NUM] = { 10, 10, 4, 3 };
	uint8_t auAifsForBcast[WMM_AC_INDEX_NUM] = { 3, 7, 2, 2 };
	/* If the AP is OFDM */
	uint8_t auTxopForBcast[WMM_AC_INDEX_NUM] = { 0, 0, 94, 47 };

	uint8_t auCWminLog2[WMM_AC_INDEX_NUM] = { 4, 4, 3, 2 };
	uint8_t auCWmaxLog2[WMM_AC_INDEX_NUM] = { 6, 10, 4, 3 };
	uint8_t auAifs[WMM_AC_INDEX_NUM] = { 3, 7, 1, 1 };
	/* If the AP is OFDM */
	uint8_t auTxop[WMM_AC_INDEX_NUM] = { 0, 0, 94, 47 };

#if 0
	prAdapter->rWifiVar.rConnSettings.fgRxShortGIDisabled = TRUE;
	prAdapter->rWifiVar.rConnSettings.uc2G4BandwidthMode = CONFIG_BW_20M;
	prAdapter->rWifiVar.rConnSettings.uc5GBandwidthMode = CONFIG_BW_20M;
#endif

	/* 4 <1> Setup PHY Attributes and Basic Rate Set/Operational Rate Set */
	prBssInfo->ucNonHTBasicPhyType = (uint8_t)
	    rNonHTApModeAttributes[prBssInfo->
				   ucConfigAdHocAPMode].ePhyTypeIndex;
	prBssInfo->u2BSSBasicRateSet =
	    rNonHTApModeAttributes[prBssInfo->
				   ucConfigAdHocAPMode].u2BSSBasicRateSet;

	prBssInfo->u2OperationalRateSet =
	    rNonHTPhyAttributes[prBssInfo->
				ucNonHTBasicPhyType].u2SupportedRateSet;

	if (fgIsRateUpdate) {
		rateGetDataRatesFromRateSet(prBssInfo->u2OperationalRateSet,
					    prBssInfo->u2BSSBasicRateSet,
					    prBssInfo->aucAllSupportedRates,
					    &prBssInfo->ucAllSupportedRatesLen);
	}
	/* 4 <2> Setup BSSID */
	COPY_MAC_ADDR(prBssInfo->aucBSSID, prBssInfo->aucOwnMacAddr);

	/* 4 <3> Setup Capability - Short Preamble */
	if (rNonHTPhyAttributes
	    [prBssInfo->ucNonHTBasicPhyType].fgIsShortPreambleOptionImplemented
	    &&
	    /* Short Preamble Option Enable is TRUE */
	    ((prAdapter->rWifiVar.ePreambleType == PREAMBLE_TYPE_SHORT) ||
	     (prAdapter->rWifiVar.ePreambleType == PREAMBLE_TYPE_AUTO))) {

		prBssInfo->fgIsShortPreambleAllowed = TRUE;
		prBssInfo->fgUseShortPreamble = TRUE;
	} else {
		prBssInfo->fgIsShortPreambleAllowed = FALSE;
		prBssInfo->fgUseShortPreamble = FALSE;
	}

	/* 4 <4> Setup Capability - Short Slot Time */
	prBssInfo->fgUseShortSlotTime = TRUE;
#ifdef CFG_SET_BCN_CAPINFO_BY_DRIVER
	/* 4 <5> Compoase Capability */
	prBssInfo->u2CapInfo = CAP_INFO_ESS;

	if (prBssInfo->fgIsProtection)
		prBssInfo->u2CapInfo |= CAP_INFO_PRIVACY;

	if (prBssInfo->fgIsShortPreambleAllowed)
		prBssInfo->u2CapInfo |= CAP_INFO_SHORT_PREAMBLE;

	if (prBssInfo->fgUseShortSlotTime)
		prBssInfo->u2CapInfo |= CAP_INFO_SHORT_SLOT_TIME;
#if CFG_AP_80211K_SUPPORT
	prBssInfo->u2CapInfo |= CAP_INFO_RADIO_MEASUREMENT;
#endif /* CFG_AP_80211K_SUPPORT */
#endif
	/* 4 <6> Find Lowest Basic Rate Index for default TX Rate of MMPDU */
	nicTxUpdateBssDefaultRate(prBssInfo);

	/* 4 <7> Fill the EDCA */

	prACQueParms = prBssInfo->arACQueParmsForBcast;

	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {

		prACQueParms[eAci].ucIsACMSet = FALSE;
		prACQueParms[eAci].u2Aifsn = auAifsForBcast[eAci];
		prACQueParms[eAci].u2CWmin = BIT(auCWminLog2ForBcast[eAci]) - 1;
		prACQueParms[eAci].u2CWmax = BIT(auCWmaxLog2ForBcast[eAci]) - 1;
		prACQueParms[eAci].u2TxopLimit = auTxopForBcast[eAci];

		/* used to send WMM IE */
		prBssInfo->aucCWminLog2ForBcast[eAci] =
		    auCWminLog2ForBcast[eAci];
		prBssInfo->aucCWmaxLog2ForBcast[eAci] =
		    auCWmaxLog2ForBcast[eAci];
	}

	DBGLOG(BSS, INFO,
	       "Bcast: ACM[%d,%d,%d,%d] Aifsn[%d,%d,%d,%d] CWmin/max[%d/%d,%d/%d,%d/%d,%d/%d] TxopLimit[%d,%d,%d,%d]\n",
	       prACQueParms[0].ucIsACMSet, prACQueParms[1].ucIsACMSet,
	       prACQueParms[2].ucIsACMSet, prACQueParms[3].ucIsACMSet,
	       prACQueParms[0].u2Aifsn, prACQueParms[1].u2Aifsn,
	       prACQueParms[2].u2Aifsn, prACQueParms[3].u2Aifsn,
	       prACQueParms[0].u2CWmin, prACQueParms[0].u2CWmax,
	       prACQueParms[1].u2CWmin, prACQueParms[1].u2CWmax,
	       prACQueParms[2].u2CWmin, prACQueParms[2].u2CWmax,
	       prACQueParms[3].u2CWmin, prACQueParms[3].u2CWmax,
	       prACQueParms[0].u2TxopLimit, prACQueParms[1].u2TxopLimit,
	       prACQueParms[2].u2TxopLimit, prACQueParms[3].u2TxopLimit);

	prACQueParms = prBssInfo->arACQueParms;

	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {

		prACQueParms[eAci].ucIsACMSet = FALSE;
		prACQueParms[eAci].u2Aifsn = auAifs[eAci];
		prACQueParms[eAci].u2CWmin = BIT(auCWminLog2[eAci]) - 1;
		prACQueParms[eAci].u2CWmax = BIT(auCWmaxLog2[eAci]) - 1;
		prACQueParms[eAci].u2TxopLimit = auTxop[eAci];
	}

	DBGLOG(BSS, INFO,
	       "ACM[%d,%d,%d,%d] Aifsn[%d,%d,%d,%d] CWmin/max[%d/%d,%d/%d,%d/%d,%d/%d] TxopLimit[%d,%d,%d,%d]\n",
	       prACQueParms[0].ucIsACMSet, prACQueParms[1].ucIsACMSet,
	       prACQueParms[2].ucIsACMSet, prACQueParms[3].ucIsACMSet,
	       prACQueParms[0].u2Aifsn, prACQueParms[1].u2Aifsn,
	       prACQueParms[2].u2Aifsn, prACQueParms[3].u2Aifsn,
	       prACQueParms[0].u2CWmin, prACQueParms[0].u2CWmax,
	       prACQueParms[1].u2CWmin, prACQueParms[1].u2CWmax,
	       prACQueParms[2].u2CWmin, prACQueParms[2].u2CWmax,
	       prACQueParms[3].u2CWmin, prACQueParms[3].u2CWmax,
	       prACQueParms[0].u2TxopLimit, prACQueParms[1].u2TxopLimit,
	       prACQueParms[2].u2TxopLimit, prACQueParms[3].u2TxopLimit);

	/* Note: Caller should update the EDCA setting to HW by
	 *         nicQmUpdateWmmParms() it there is no AIS network
	 * Note: In E2, only 4 HW queues.
	 *         The the Edca parameters should be folow by AIS network
	 * Note: In E3, 8 HW queues.
	 *         the Wmm parameters should be updated to right queues
	 *         according to BSS
	 */
}				/* end of bssInitForAP() */

#endif /* CFG_SUPPORT_AAA */

void bssCreateStaRecFromAuth(struct ADAPTER *prAdapter)
{

}

void bssUpdateStaRecFromAssocReq(struct ADAPTER *prAdapter)
{

}

void bssUpdateStaRecFromBssDesc(struct ADAPTER *prAdapter,
				struct BSS_DESC *prBssDesc,
				struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex;

	if (!prBssDesc || !prStaRec)
		return;

	ucBssIndex = prStaRec->ucBssIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(BSS, ERROR, "prBssInfo is null\n");
		return;
	}

	/* 4 <2> Update information from BSS_DESC_T to current P_STA_RECORD_T */
	prStaRec->u2CapInfo = prBssDesc->u2CapInfo;

	prStaRec->u2OperationalRateSet = prBssDesc->u2OperationalRateSet;
	prStaRec->u2BSSBasicRateSet = prBssDesc->u2BSSBasicRateSet;

	bssDetermineStaRecPhyTypeSet(prAdapter, prBssDesc, prStaRec);

	/* 4 <3> Update information from BSS_DESC_T to current P_STA_RECORD_T */
	if (IS_AP_STA(prStaRec)) {
		/* do not need to parse IE for DTIM,
		 * which have been parsed before inserting into struct BSS_DESC
		 */
		if (prBssDesc->ucDTIMPeriod)
			prStaRec->ucDTIMPeriod = prBssDesc->ucDTIMPeriod;
		else
			prStaRec->ucDTIMPeriod = 0;
		/* Means that TIM was not parsed. */
	}
}

void bssDumpBssInfo(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	uint32_t u4NetworkType, u4CurrentOPMode;
	/* P_LINK_T prStaRecOfClientList = (P_LINK_T) NULL; */
	/* P_STA_RECORD_T prCurrStaRec = (P_STA_RECORD_T) NULL; */

	if (ucBssIndex > prAdapter->ucSwBssIdNum) {
		DBGLOG(SW4, INFO, "Invalid BssInfo index[%u], skip dump!\n",
		       ucBssIndex);
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(SW4, INFO, "Invalid BssInfo index[%u], skip dump!\n",
		       ucBssIndex);
		return;
	}

	DBGLOG(SW4, INFO, "OWNMAC[" MACSTR "] BSSID[" MACSTR "] SSID[%s]\n",
	       MAC2STR(prBssInfo->aucOwnMacAddr), MAC2STR(prBssInfo->aucBSSID),
	       HIDE(prBssInfo->aucSSID));

	u4NetworkType = prBssInfo->eNetworkType;
	u4CurrentOPMode = prBssInfo->eCurrentOPMode;
	if (u4NetworkType < NETWORK_TYPE_NUM
			&& u4CurrentOPMode < OP_MODE_NUM) {
		DBGLOG(SW4, INFO,
			"BSS IDX[%u] Type[%s] OPMode[%s] ConnState[%u] Absent[%u]\n",
			prBssInfo->ucBssIndex,
			apucNetworkType[u4NetworkType],
			apucNetworkOpMode[u4CurrentOPMode],
			prBssInfo->eConnectionState, prBssInfo->fgIsNetAbsent);
	}

	DBGLOG(SW4, INFO,
	       "Channel[%u] Band[%u] SCO[%u] Assoc40mBwAllowed[%u] 40mBwAllowed[%u]\n",
	       prBssInfo->ucPrimaryChannel, prBssInfo->eBand,
	       prBssInfo->eBssSCO, prBssInfo->fgAssoc40mBwAllowed,
	       prBssInfo->fg40mBwAllowed);

	DBGLOG(SW4, INFO, "MaxBw[%u] OpRxNss[%u] OpTxNss[%u]\n",
	       cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex),
	       prBssInfo->ucOpRxNss, prBssInfo->ucOpTxNss);

	DBGLOG(SW4, INFO, "QBSS[%u] CapInfo[0x%04x] AID[%u]\n",
	       prBssInfo->fgIsQBSS, prBssInfo->u2CapInfo, prBssInfo->u2AssocId);

	DBGLOG(SW4, INFO,
	       "ShortPreamble Allowed[%u] EN[%u], ShortSlotTime[%u]\n",
	       prBssInfo->fgIsShortPreambleAllowed,
	       prBssInfo->fgUseShortPreamble, prBssInfo->fgUseShortSlotTime);

	DBGLOG(SW4, INFO, "PhyTypeSet: Basic[0x%02x] NonHtBasic[0x%02x]\n",
	       prBssInfo->ucPhyTypeSet, prBssInfo->ucNonHTBasicPhyType);

	DBGLOG(SW4, INFO, "RateSet: BssBasic[0x%04x] Operational[0x%04x]\n",
	       prBssInfo->u2BSSBasicRateSet, prBssInfo->u2OperationalRateSet);

	DBGLOG(SW4, INFO, "ATIMWindow[%u] DTIM Period[%u] Count[%u]\n",
	       prBssInfo->u2ATIMWindow, prBssInfo->ucDTIMPeriod,
	       prBssInfo->ucDTIMCount);

	DBGLOG(SW4, INFO,
	       "HT Operation Info1[0x%02x] Info2[0x%04x] Info3[0x%04x]\n",
	       prBssInfo->ucHtOpInfo1, prBssInfo->u2HtOpInfo2,
	       prBssInfo->u2HtOpInfo3);

	DBGLOG(SW4, INFO,
	       "ProtectMode HT[%u] ERP[%u], OperationMode GF[%u] RIFS[%u]\n",
	       prBssInfo->eHtProtectMode, prBssInfo->fgErpProtectMode,
	       prBssInfo->eGfOperationMode, prBssInfo->eRifsOperationMode);

	DBGLOG(SW4, INFO,
	       "(OBSS) ProtectMode HT[%u] ERP[%u], OperationMode GF[%u] RIFS[%u]\n",
	       prBssInfo->eObssHtProtectMode, prBssInfo->fgObssErpProtectMode,
	       prBssInfo->eObssGfOperationMode,
	       prBssInfo->fgObssRifsOperationMode);

	DBGLOG(SW4, INFO,
	       "VhtChannelWidth[%u] OpChangeChannelWidth[%u], IsOpChangeChannelWidth[%u]\n",
	       prBssInfo->ucVhtChannelWidth, prBssInfo->ucOpChangeChannelWidth,
	       prBssInfo->fgIsOpChangeChannelWidth);

	DBGLOG(SW4, INFO, "======== Dump Connected Client ========\n");

#if 0
	DBGLOG(SW4, INFO, "NumOfClient[%u]\n",
	       bssGetClientCount(prAdapter, prBssInfo));

	prStaRecOfClientList = &prBssInfo->rStaRecOfClientList;

	LINK_FOR_EACH_ENTRY(prCurrStaRec, prStaRecOfClientList, rLinkEntry,
			    struct STA_RECORD) {
		DBGLOG(SW4, INFO, "STA[%u] [" MACSTR "]\n",
		       prCurrStaRec->ucIndex,
		       MAC2STR(prCurrStaRec->aucMacAddr));
	}
#else
#if CFG_ENABLE_WIFI_DIRECT
	bssDumpClientList(prAdapter, prBssInfo);
#endif
#endif

	DBGLOG(SW4, INFO, "============== Dump Done ==============\n");
}

int8_t bssGetHtRxNss(struct BSS_DESC *prBssDesc)
{
	uint8_t nss = 0;
	const uint8_t *pucIe;
	uint8_t *mcs_mask;

	pucIe = kalFindIeMatchMask(ELEM_ID_HT_CAP, prBssDesc->pucIeBuf,
				   prBssDesc->u2IELength, NULL, 0, 0, NULL);
	if (!pucIe)
		return 0;
	mcs_mask = &((struct IE_HT_CAP *)pucIe)->rSupMcsSet.aucRxMcsBitmask[0];
	/*HT_NSS_MAX = 4 */
	while (mcs_mask[nss] && nss < 4)
		nss++;
	return nss;
}

int8_t bssGetVhtRxNss(struct BSS_DESC *prBssDesc)
{
	uint8_t nss = 0;
	const uint8_t *pucIe;
	uint16_t mcs_map;

	pucIe = kalFindIeMatchMask(ELEM_ID_VHT_CAP, prBssDesc->pucIeBuf,
				   prBssDesc->u2IELength, NULL, 0, 0, NULL);
	if (!pucIe)
		return 0;
	mcs_map = ((struct IE_VHT_CAP *)pucIe)->rVhtSupportedMcsSet.u2RxMcsMap;
	/*VHT_NSS_MAX = 8 */
	while (((mcs_map & BITS(0, 1)) != VHT_CAP_INFO_MCS_NOT_SUPPORTED) &&
		(nss < 8)) {
		nss++;
		mcs_map >>= 2;
	}
	return nss;
}

#if (CFG_SUPPORT_802_11AX == 1)
int8_t bssGetHeRxNss(struct BSS_DESC *prBssDesc)
{
	uint8_t nss = 0;
	const uint8_t *pucIe;
	uint16_t mcs_map;

	pucIe = kalFindIeExtIE(ELEM_ID_RESERVED, ELEM_EXT_ID_HE_CAP,
			       prBssDesc->pucIeBuf, prBssDesc->u2IELength);
	if (!pucIe)
		return 0;
	WLAN_GET_FIELD_16(&((struct _IE_HE_CAP_T *)pucIe)->aucVarInfo[0],
			  &mcs_map)
	/*VHT_NSS_MAX = 8 */
	while (((mcs_map & BITS(0, 1)) != HE_CAP_INFO_MCS_NOT_SUPPORTED) &&
		(nss < 8)) {
		nss++;
		mcs_map >>= 2;
	}
	return nss;
}
#endif

#if (CFG_SUPPORT_802_11BE == 1)
int8_t bssGetEhtRxNss(struct BSS_DESC *prBssDesc)
{
	uint8_t nss = 0;
	const uint8_t *pucIe;
	uint8_t eht_mcs_mask;

	pucIe = kalFindIeExtIE(ELEM_ID_RESERVED, ELEM_EXT_ID_EHT_CAPS,
			       prBssDesc->pucIeBuf, prBssDesc->u2IELength);
	if (!pucIe)
		return 0;
	eht_mcs_mask = ((struct IE_EHT_CAP *)pucIe)->aucVarInfo[0];
	nss = eht_mcs_mask & BITS(0, 3);
	return nss;
}
#endif

int8_t bssGetRxNss(struct BSS_DESC *prBssDesc)
{
	uint8_t nss = 0;

#if (CFG_SUPPORT_802_11BE == 1)
	nss = bssGetEhtRxNss(prBssDesc);
	if (nss)
		/* No need to check lower protocol? */
		return nss;
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	nss = bssGetHeRxNss(prBssDesc);
	if (nss)
		return nss;
#endif
	nss = bssGetVhtRxNss(prBssDesc);
	if (nss)
		return nss;
	nss = bssGetHtRxNss(prBssDesc);
	if (nss)
		return nss;
	return 1;
}


#if (CFG_SUPPORT_HE_ER == 1)
void bssProcessErTxModeEvent(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct BSS_INFO *prBssInfo;
	struct EVENT_ER_TX_MODE *prErTxMode;

	prErTxMode = (struct EVENT_ER_TX_MODE *) (prEvent->aucBuffer);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prErTxMode->ucBssInfoIdx);
	if (!prBssInfo) {
		DBGLOG(BSS, ERROR, "prBssInfo is null\n");
		return;
	}

	prBssInfo->ucErMode = prErTxMode->ucErMode;

	DBGLOG_LIMITED(BSS, WARN,
		"Receive ER Tx mode event,BSS[%d],Mode[0x%x]\n",
		prErTxMode->ucBssInfoIdx, prErTxMode->ucErMode);
}
#endif

#if CFG_SUPPORT_IOT_AP_BLOCKLIST
/*----------------------------------------------------------------------------*/
/*!
 * @brief get IOT AP handle action.
 *
 * @param[in] prBssDesc
 *
 * @return ENUM_WLAN_IOT_ACTION
 */
/*----------------------------------------------------------------------------*/
uint32_t bssGetIotApAction(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc)
{
	uint8_t  ucCnt = 0;
	int8_t   ucBssNss;
	uint8_t  *pucMask;
	uint16_t u2MatchFlag;
	const  uint8_t *pucIes;
	const  uint8_t *pucIe;
	struct WLAN_IOT_AP_RULE_T *prIotApRule;

	if (!prAdapter || !prBssDesc) {
		DBGLOG(BSS, INFO, "GetIotApAction Param Error!\n");
		return -EINVAL;
	}
	/*To make sure one Bss only parse once*/
	if (prBssDesc->fgIotApActionValid)
		return prBssDesc->u8IotApAct;


	prBssDesc->fgIotApActionValid = TRUE;
	prBssDesc->u8IotApAct = 0;
	prBssDesc->ucIotVer = 0;

	pucIes = prBssDesc->pucIeBuf;
	for (ucCnt = 0; ucCnt < CFG_IOT_AP_RULE_MAX_CNT; ucCnt++) {
		prIotApRule = &prAdapter->rIotApRule[ucCnt];
		u2MatchFlag = prIotApRule->u2MatchFlag;

		/*No need to match empty rule*/
		if (prIotApRule->u2MatchFlag == 0)
			continue;

		/*Match Vendor OUI*/
		if (u2MatchFlag & BIT(WLAN_IOT_AP_FG_OUI)) {
			pucIe = kalFindIeMatchMask(
				WLAN_EID_VENDOR_SPECIFIC,
				pucIes, prBssDesc->u2IELength,
				prIotApRule->aVendorOui,
				MAC_OUI_LEN, 2, NULL);
			if (!pucIe)
				continue;
			/*Match!, Fall through*/
		}

		/*Match Vendor Data rule*/
		if (u2MatchFlag & BIT(WLAN_IOT_AP_FG_DATA)) {
			pucMask =
				u2MatchFlag & BIT(WLAN_IOT_AP_FG_DATA_MASK) ?
				&prIotApRule->aVendorDataMask[0] : NULL;
			pucIe = kalFindIeMatchMask(
				WLAN_EID_VENDOR_SPECIFIC,
				pucIes, prBssDesc->u2IELength,
				prIotApRule->aVendorData,
				prIotApRule->ucDataLen, 5, pucMask);
			if (!pucIe)
				continue;
			/*Match!, Fall through*/
		}

		/*Match BSSID rule*/
		if (u2MatchFlag & BIT(WLAN_IOT_AP_FG_BSSID)) {
			pucMask =
				u2MatchFlag & BIT(WLAN_IOT_AP_FG_BSSID_MASK) ?
				&prIotApRule->aBssidMask[0] : NULL;
			if (kalMaskMemCmp(&prBssDesc->aucBSSID,
				&prIotApRule->aBssid,
				pucMask,
				MAC_ADDR_LEN))
				continue;
			/*Match!, Fall through*/
		}

		/*Match Rx NSS rule*/
		if (u2MatchFlag & BIT(WLAN_IOT_AP_FG_NSS)) {
			ucBssNss = bssGetRxNss(prBssDesc);
			if (ucBssNss < 0)
				DBGLOG(BSS, TRACE,
					"IOTAP Nss=%d invalid", ucBssNss);
			if (ucBssNss != prIotApRule->ucNss)
				continue;
			/*Match!, Fall through*/
		}

		/*Match HT type rule*/
		if (u2MatchFlag & BIT(WLAN_IOT_AP_FG_HT)) {
#if (CFG_SUPPORT_802_11BE == 1)
			if (prBssDesc->fgIsEHTPresent) {
				if (prIotApRule->ucHtType != 7)
					continue;
			} else
#endif
#if (CFG_SUPPORT_802_11AX == 1)
			if (prBssDesc->fgIsHEPresent) {
				if (prIotApRule->ucHtType != 6)
					continue;
			} else
#endif
			if (prBssDesc->fgIsVHTPresent) {
				if (prIotApRule->ucHtType != 2)
					continue;
			} else if (prBssDesc->fgIsHTPresent) {
				if (prIotApRule->ucHtType != 1)
					continue;
			} else {
				if (prIotApRule->ucHtType != 0)
					continue;
			}
			/*Matched, Fall through*/
		}

		/*Match Band Rule*/
		if (u2MatchFlag & BIT(WLAN_IOT_AP_FG_BAND)) {
			if (prBssDesc->eBand != prIotApRule->ucBand)
				continue;
			/*Matched, Fall through*/
		}
		/*All match, set the actions and version*/
		prBssDesc->u8IotApAct = prIotApRule->u8Action;
		prBssDesc->ucIotVer = prIotApRule->ucVersion;
		break;
	}
	return prBssDesc->u8IotApAct;
}

bool bssIsIotAp(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ENUM_WLAN_IOT_ACTION eAction)
{
	uint64_t u8IotApAct;

	u8IotApAct = bssGetIotApAction(prAdapter, prBssDesc);
	if (prBssDesc && prBssDesc->fgIotApActionValid) {
		if (prBssDesc->ucIotVer == 1)
			return u8IotApAct & BIT(eAction);
		else
			return u8IotApAct == eAction;
	}
	return FALSE;
}

#endif

#if CFG_ENABLE_WIFI_DIRECT
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get the alive bss HW band bitmap. Each bit indicate the bss
 *        of that index number is alive or not.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pau4Bitmap The output bitmap.
 * \param[in] ucBandNum Number of HW band of the chip.
 */
/*---------------------------------------------------------------------------*/
void bssGetAliveBssHwBitmap(struct ADAPTER *prAdapter, uint32_t *pau4Bitmap)
{
	struct BSS_INFO *bss;
	uint8_t i;

	for (i = 0; i < MAX_BSSID_NUM; ++i) {
		bss = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		if (!IS_BSS_ALIVE(prAdapter, bss) ||
		    bss->eHwBandIdx >= AA_HW_BAND_NUM)
			continue;

		pau4Bitmap[bss->eHwBandIdx] |= BIT(i);
	}
	DBGLOG(BSS, TRACE,
	       "alive Bss hw bitmap [bn0:bn1:bn2]=[0x%x:0x%x:0x%x]\n",
	       pau4Bitmap[AA_HW_BAND_0], pau4Bitmap[AA_HW_BAND_1],
	       pau4Bitmap[AA_HW_BAND_2]);
}
#endif
