// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "p2p_rlm.c"
 *    \brief
 *
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
#include "rlm.h"

#if CFG_ENABLE_WIFI_DIRECT
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
static enum ENUM_CHNL_EXT rlmGetSco(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);
/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */
#if (CFG_SUPPORT_WIFI_6G == 1)
void rlmUpdate6GOpInfo(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	struct _6G_OPER_INFOR_T *pr6gOperInfo;
	uint8_t ucMaxBandwidth;
	uint8_t ucSeg0 = 0, ucSeg1 = 0;

	if (IS_BSS_APGO(prBssInfo) == FALSE || prBssInfo->eBand != BAND_6G)
		return;

	pr6gOperInfo = &prBssInfo->r6gOperInfor;

	HE_SET_6G_OP_INFOR_PRESENT(prBssInfo->ucHeOpParams);

	ucMaxBandwidth = rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo);
#if (CFG_SUPPORT_SAP_PUNCTURE == 1)
	if (prBssInfo->fgIsEhtDscbPresent) {
		/* get seg0 & seg1 with original bandwidth for puncturing */
		ucSeg0 = nicGetS1(prBssInfo->eBand,
				  prBssInfo->ucPrimaryChannel,
				  prBssInfo->eBssSCO,
				  ucMaxBandwidth);
		ucSeg1 = nicGetS2(prBssInfo->eBand,
				  prBssInfo->ucPrimaryChannel,
				  ucMaxBandwidth);

		rlmPunctUpdateLegacyBw(prBssInfo->eBand,
				       prBssInfo->u2EhtDisSubChanBitmap,
				       prBssInfo->ucPrimaryChannel,
				       &ucMaxBandwidth,
				       &ucSeg0,
				       &ucSeg1,
				       NULL);
	}
#endif /* CFG_SUPPORT_SAP_PUNCTURE */

	ucMaxBandwidth = kal_min_t(uint8_t,
				   ucMaxBandwidth,
				   MAX_BW_160MHZ);
	/* re-sync seg0 & seg1 channel in case puncture takes effect. */
	ucSeg0 = nicGetS1(prBssInfo->eBand,
			  prBssInfo->ucPrimaryChannel,
			  prBssInfo->eBssSCO,
			  ucMaxBandwidth);
	ucSeg1 = nicGetS2(prBssInfo->eBand,
			  prBssInfo->ucPrimaryChannel,
			  ucMaxBandwidth);

	pr6gOperInfo->rControl.bits.ChannelWidth =
		heRlmMaxBwToHeBw(ucMaxBandwidth);
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	pr6gOperInfo->rControl.bits.RegulatoryInfo =
		prAdapter->rWifiVar.ucHe6gRegPwrType;
#endif
	pr6gOperInfo->ucPrimaryChannel =
		prBssInfo->ucPrimaryChannel;
	pr6gOperInfo->ucChannelCenterFreqSeg0 = ucSeg0;
	pr6gOperInfo->ucChannelCenterFreqSeg1 = ucSeg1;
	pr6gOperInfo->ucMinimumRate = 6;

	DBGLOG(RLM, TRACE,
		"Set 6G operating info: BW[%d] REG[%d] CH[%d] Seg0[%d] Seg1[%d]\n",
		pr6gOperInfo->rControl.bits.ChannelWidth,
		pr6gOperInfo->rControl.bits.RegulatoryInfo,
		pr6gOperInfo->ucPrimaryChannel,
		pr6gOperInfo->ucChannelCenterFreqSeg0,
		pr6gOperInfo->ucChannelCenterFreqSeg1);
}
#endif

void rlmBssUpdateChannelParams(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	uint8_t i;
	uint8_t ucMaxBw = 0;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	/* Operation band, channel shall be ready before invoking this function.
	 * Bandwidth may be ready if other network is connected
	 */
	prBssInfo->fg40mBwAllowed = FALSE;
	prBssInfo->fgAssoc40mBwAllowed = FALSE;
	prBssInfo->ucHtOpInfo1 = 0;

	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
				prBssInfo->u4PrivateData);

	/* Check if AP can set its bw to 40MHz
	 * But if any of BSS is setup in 40MHz,
	 * the second BSS would prefer to use 20MHz
	 * in order to remain in SCC case.
	 */
	if (cnmBss40mBwPermitted(prAdapter, prBssInfo->ucBssIndex)) {
		/* GO/SAP decides bss params by itself, including SCO.
		 * GC follows GO's SCO and assuming SCO under BssInfo
		 * has been updated elsewhere.
		 * CSA skip this because CSA has it's own sco setting flow.
		 */
		if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT &&
		    prP2pRoleFsmInfo->eCurrentState !=
				P2P_ROLE_STATE_SWITCH_CHANNEL)
			prBssInfo->eBssSCO =
				rlmGetScoForAP(prAdapter, prBssInfo);

		if (prBssInfo->eBssSCO != CHNL_EXT_SCN) {
			prBssInfo->fg40mBwAllowed = TRUE;
			prBssInfo->fgAssoc40mBwAllowed = TRUE;

			prBssInfo->ucHtOpInfo1 = (uint8_t)
				(((uint32_t) prBssInfo->eBssSCO)
				| HT_OP_INFO1_STA_CHNL_WIDTH);
		}
	} else {
		prBssInfo->eBssSCO = CHNL_EXT_SCN;
	}

	/* Filled the VHT BW/S1/S2 and MCS rate set */
	if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_VHT) {
		for (i = 0; i < 8; i++)
			prBssInfo->u2VhtBasicMcsSet |= BITS(2 * i, (2 * i + 1));

		prBssInfo->u2VhtBasicMcsSet &=
			(VHT_CAP_INFO_MCS_MAP_MCS9
			<< VHT_CAP_INFO_MCS_1SS_OFFSET);

		ucMaxBw = cnmOpModeGetMaxBw(prAdapter,
			prBssInfo);

		rlmFillVhtOpInfoByBssOpBw(prBssInfo, ucMaxBw);

		/* If the S1 is invalid, force to change bandwidth */
		if (prBssInfo->ucVhtChannelFrequencyS1 == 0) {
			/* Give GO/AP another chance to use BW80
			 * if failed to get S1 for BW160.
			 */
			if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT &&
				ucMaxBw == MAX_BW_160MHZ) {
				rlmFillVhtOpInfoByBssOpBw(prBssInfo,
					MAX_BW_80MHZ);
			}

			/* fallback to BW20/40 */
			if (prBssInfo->ucVhtChannelFrequencyS1 == 0) {
				prBssInfo->ucVhtChannelWidth =
					VHT_OP_CHANNEL_WIDTH_20_40;
			}
		}
	} else if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_HE) {
		ucMaxBw = cnmOpModeGetMaxBw(prAdapter,
			prBssInfo);

		rlmFillVhtOpInfoByBssOpBw(prBssInfo, ucMaxBw);
	}

#if (CFG_SUPPORT_802_11AX == 1)
	/* Filled the HE Operation IE */
	if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_HE) {
		memset(prBssInfo->ucHeOpParams, 0, HE_OP_BYTE_NUM);

		prBssInfo->ucHeOpParams[0]
			|= HE_OP_PARAM0_TXOP_DUR_RTS_THRESHOLD_MASK;
		prBssInfo->ucHeOpParams[1]
			|= HE_OP_PARAM1_TXOP_DUR_RTS_THRESHOLD_MASK;

		prBssInfo->u2HeBasicMcsSet |= (HE_CAP_INFO_MCS_MAP_MCS7 << 0);
		for (i = 1; i < 8; i++)
			prBssInfo->u2HeBasicMcsSet |=
				(HE_CAP_INFO_MCS_NOT_SUPPORTED << 2 * i);

		if (IS_FEATURE_ENABLED(
			prAdapter->rWifiVar.fgBssMaxIdle)) {
			prBssInfo->u2MaxIdlePeriod =
				prAdapter->rWifiVar.u2BssMaxIdlePeriod;
			prBssInfo->ucIdleOptions =
				secIsProtectedBss(prAdapter, prBssInfo);
		}

#if (CFG_SUPPORT_WIFI_6G == 1)
		ucMaxBw = cnmOpModeGetMaxBw(prAdapter, prBssInfo);
		rlmFillVhtOpInfoByBssOpBw(prBssInfo, ucMaxBw);
		rlmUpdate6GOpInfo(prAdapter, prBssInfo);
#endif
	} else {
		memset(prBssInfo->ucHeOpParams, 0, HE_OP_BYTE_NUM);
		prBssInfo->ucBssColorInfo = 0;
		prBssInfo->u2HeBasicMcsSet = 0;
		prBssInfo->u2MaxIdlePeriod = 0;
		prBssInfo->ucIdleOptions = 0;
	}
#endif

#if (CFG_SUPPORT_802_11BE == 1)
	if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_EHT) {
		EHT_RESET_OP(prBssInfo->ucEhtOpParams);
		/* TODO */
	} else {
		EHT_RESET_OP(prBssInfo->ucEhtOpParams);
	}
#endif

	/*ERROR HANDLE*/
	if ((prBssInfo->ucVhtChannelWidth >= VHT_OP_CHANNEL_WIDTH_80) &&
	    (prBssInfo->ucVhtChannelFrequencyS1 == 0)) {
		DBGLOG(RLM, INFO,
			"Wrong AP S1 parameter setting, back to BW20!!!\n");

		prBssInfo->ucVhtChannelWidth = VHT_OP_CHANNEL_WIDTH_20_40;
		prBssInfo->eBssSCO = 0;
		prBssInfo->ucVhtChannelFrequencyS1 = 0;
		prBssInfo->ucVhtChannelFrequencyS2 = 0;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Init AP Bss
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmBssInitForAP(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo)
{
	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	if (prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT)
		return;

	/* Check if AP can set its bw to 40MHz
	 * But if any of BSS is setup in 40MHz,
	 * the second BSS would prefer to use 20MHz
	 * in order to remain in SCC case
	 */
	rlmBssUpdateChannelParams(prAdapter, prBssInfo);

	if (cnmBss40mBwPermitted(prAdapter, prBssInfo->ucBssIndex)) {
		if (prBssInfo->eBssSCO != CHNL_EXT_SCN)
			rlmUpdateBwByChListForAP(prAdapter, prBssInfo);
	}

	/* We may limit AP/GO Nss by RfBand in some case, ex CoAnt.
	 * Recalculte Nss when channel is selected.
	 */
	cnmOpModeGetTRxNss(prAdapter,
		prBssInfo->ucBssIndex,
		&prBssInfo->ucOpRxNss,
		&prBssInfo->ucOpTxNss);
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
	if (p2pFuncGetForceTrxConfig(prAdapter,
			prBssInfo->ucBssIndex) ==
		P2P_FORCE_TRX_CONFIG_1NSS_LOW_POWER) {
		prBssInfo->ucOpRxNss = 1;
		prBssInfo->ucOpTxNss = 1;
	}
#endif
	DBGLOG(RLM, INFO,
		"WLAN AP SCO=%d BW=%d S1=%d S2=%d CH=%d Band=%d TxN=%d RxN=%d\n",
		prBssInfo->eBssSCO,
		prBssInfo->ucVhtChannelWidth,
		prBssInfo->ucVhtChannelFrequencyS1,
		prBssInfo->ucVhtChannelFrequencyS2,
		prBssInfo->ucPrimaryChannel,
		prBssInfo->eBand,
		prBssInfo->ucOpTxNss,
		prBssInfo->ucOpRxNss);

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief For probe response (GO, IBSS) and association response
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmRspGenerateObssScanIE(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct IE_OBSS_SCAN_PARAM *prObssScanIe;
	struct STA_RECORD *prStaRec =
		(struct STA_RECORD *) NULL;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter,
		prMsduInfo->ucStaRecIndex);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prMsduInfo->ucBssIndex);

	if (!prBssInfo)
		return;

	if (!IS_BSS_ACTIVE(prBssInfo))
		return;

	if (RLM_NET_IS_11N(prBssInfo) &&
	    prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT &&
	    (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)) &&
	    prBssInfo->eBand == BAND_2G4 &&
	    prBssInfo->eBssSCO != CHNL_EXT_SCN) {

		prObssScanIe = (struct IE_OBSS_SCAN_PARAM *)
		    (((uint8_t *) prMsduInfo->prPacket)
				+ prMsduInfo->u2FrameLength);

		/* Add 20/40 BSS coexistence IE */
		prObssScanIe->ucId = ELEM_ID_OBSS_SCAN_PARAMS;
		prObssScanIe->ucLength =
			sizeof(struct IE_OBSS_SCAN_PARAM) - ELEM_HDR_LEN;

		prObssScanIe->u2ScanPassiveDwell =
			dot11OBSSScanPassiveDwell;
		prObssScanIe->u2ScanActiveDwell =
			dot11OBSSScanActiveDwell;
		prObssScanIe->u2TriggerScanInterval =
			dot11BSSWidthTriggerScanInterval;
		prObssScanIe->u2ScanPassiveTotalPerChnl =
			dot11OBSSScanPassiveTotalPerChannel;
		prObssScanIe->u2ScanActiveTotalPerChnl =
			dot11OBSSScanActiveTotalPerChannel;
		prObssScanIe->u2WidthTransDelayFactor =
			dot11BSSWidthChannelTransitionDelayFactor;
		prObssScanIe->u2ScanActivityThres =
			dot11OBSSScanActivityThreshold;

		ASSERT(
			IE_SIZE(prObssScanIe)
			<= (ELEM_HDR_LEN + ELEM_MAX_LEN_OBSS_SCAN));

		prMsduInfo->u2FrameLength += IE_SIZE(prObssScanIe);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief P2P GO.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
u_int8_t rlmUpdateBwByChListForAP(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	uint8_t ucLevel;
	u_int8_t fgBwChange;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	fgBwChange = FALSE;

	if (prBssInfo->eBssSCO == CHNL_EXT_SCN)
		return fgBwChange;

	ucLevel = rlmObssChnlLevel(prBssInfo,
		prBssInfo->eBand,
		prBssInfo->ucPrimaryChannel,
		prBssInfo->eBssSCO);

	if ((ucLevel == CHNL_LEVEL0) &&
		!prAdapter->rWifiVar.fgSapGoSkipObss) {
		/* Forced to 20MHz,
		 * so extended channel is SCN and STA width is zero
		 */
		prBssInfo->fgObssActionForcedTo20M = TRUE;

		if (prBssInfo->ucHtOpInfo1 != (uint8_t) CHNL_EXT_SCN) {
			prBssInfo->ucHtOpInfo1 = (uint8_t) CHNL_EXT_SCN;
			fgBwChange = TRUE;
			DBGLOG(RLM, INFO,
				"BW40: Set fgObssActionForcedTo20M\n");
		}

		cnmTimerStartTimer(prAdapter,
			&prBssInfo->rObssScanTimer,
			OBSS_20_40M_TIMEOUT * MSEC_PER_SEC);
	}

	/* Clear up all channel lists */
	prBssInfo->auc2G_20mReqChnlList[0] = 0;
	prBssInfo->auc2G_NonHtChnlList[0] = 0;
	prBssInfo->auc2G_PriChnlList[0] = 0;
	prBssInfo->auc2G_SecChnlList[0] = 0;
	prBssInfo->auc5G_20mReqChnlList[0] = 0;
	prBssInfo->auc5G_NonHtChnlList[0] = 0;
	prBssInfo->auc5G_PriChnlList[0] = 0;
	prBssInfo->auc5G_SecChnlList[0] = 0;

	return fgBwChange;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmHandleObssStatusEventPkt(struct ADAPTER *prAdapter,
		struct EVENT_AP_OBSS_STATUS *prObssStatus)
{
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);
	ASSERT(prObssStatus);
	ASSERT(prObssStatus->ucBssIndex
		< prAdapter->ucSwBssIdNum);

	prBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, prObssStatus->ucBssIndex);

	if (!prBssInfo || prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT)
		return;

	DBGLOG(RLM, TRACE,
		"erp_prot=%u ht_prot=%u gf=%u rifs=%u force_20m=%u\n",
		prObssStatus->ucObssErpProtectMode,
		prObssStatus->ucObssHtProtectMode,
		prObssStatus->ucObssGfOperationMode,
		prObssStatus->ucObssRifsOperationMode,
		prObssStatus->ucObssBeaconForcedTo20M);

	prBssInfo->fgObssErpProtectMode =
		(u_int8_t) prObssStatus->ucObssErpProtectMode;
	prBssInfo->eObssHtProtectMode =
		(enum ENUM_HT_PROTECT_MODE) prObssStatus->ucObssHtProtectMode;
	prBssInfo->eObssGfOperationMode =
		(enum ENUM_GF_MODE) prObssStatus->ucObssGfOperationMode;
	prBssInfo->fgObssRifsOperationMode =
		(u_int8_t) prObssStatus->ucObssRifsOperationMode;
	prBssInfo->fgObssBeaconForcedTo20M =
		(u_int8_t) prObssStatus->ucObssBeaconForcedTo20M;

	if (prAdapter->rWifiVar.fgSapGoSkipObss)
		prBssInfo->fgObssBeaconForcedTo20M = FALSE;

	/* Check if Beacon content need to be updated */
	rlmUpdateParamsForAP(prAdapter, prBssInfo, TRUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief It is only for AP mode in NETWORK_TYPE_P2P_INDEX.
 *
 * \param[in]
 *
 * \return if beacon was updated
 */
/*----------------------------------------------------------------------------*/
u_int8_t rlmUpdateParamsForAP(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		u_int8_t fgUpdateBeacon)
{
	struct LINK *prStaList;
	struct STA_RECORD *prStaRec;
	u_int8_t fgErpProtectMode, fgSta40mIntolerant;
	u_int8_t fgUseShortPreamble, fgUseShortSlotTime;
	enum ENUM_HT_PROTECT_MODE eHtProtectMode;
	enum ENUM_GF_MODE eGfOperationMode;
	uint8_t ucHtOpInfo1;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	if (!IS_BSS_ACTIVE(prBssInfo)
		|| prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT)
		return FALSE;

	fgErpProtectMode = FALSE;
	eHtProtectMode = HT_PROTECT_MODE_NONE;
	eGfOperationMode = GF_MODE_NORMAL;
	fgSta40mIntolerant = FALSE;
	fgUseShortPreamble = prBssInfo->fgIsShortPreambleAllowed;
	fgUseShortSlotTime = TRUE;
	ucHtOpInfo1 = (uint8_t) CHNL_EXT_SCN;

	prStaList = &prBssInfo->rStaRecOfClientList;

	LINK_FOR_EACH_ENTRY(prStaRec,
		prStaList, rLinkEntry, struct STA_RECORD) {
		if (!prStaRec) {
			DBGLOG(P2P, WARN,
				"NULL STA_REC ptr in BSS client list\n");
			bssDumpClientList(prAdapter, prBssInfo);
			break;
		}

		if (prStaRec->fgIsInUse
			&& prStaRec->ucStaState == STA_STATE_3
			&& prStaRec->ucBssIndex == prBssInfo->ucBssIndex) {
			if (!(prStaRec->ucPhyTypeSet
				& (PHY_TYPE_SET_802_11GN
				| PHY_TYPE_SET_802_11A))) {
				/* B-only mode, so mode 1 (ERP protection) */
				fgErpProtectMode = TRUE;
			}

			if (!(prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)) {
				/* BG-only or A-only */
				eHtProtectMode = HT_PROTECT_MODE_NON_HT;
			} else if (prBssInfo->fg40mBwAllowed &&
				!(prStaRec->u2HtCapInfo
				& HT_CAP_INFO_SUP_CHNL_WIDTH)) {
				/* 20MHz-only */
				if (eHtProtectMode == HT_PROTECT_MODE_NONE)
					eHtProtectMode = HT_PROTECT_MODE_20M;
			}

			if (!(prStaRec->u2HtCapInfo & HT_CAP_INFO_HT_GF))
				eGfOperationMode = GF_MODE_PROTECT;

			if (!(prStaRec->u2CapInfo & CAP_INFO_SHORT_PREAMBLE))
				fgUseShortPreamble = FALSE;
#if 1
			/* ap mode throughput enhancement
			 * only 2.4G with B mode client connecion
			 * use long slot time
			 */
			if ((!(prStaRec->u2CapInfo & CAP_INFO_SHORT_SLOT_TIME))
					&& fgErpProtectMode
					&& prBssInfo->eBand == BAND_2G4)
				fgUseShortSlotTime = FALSE;
#else
			if (!(prStaRec->u2CapInfo & CAP_INFO_SHORT_SLOT_TIME))
				fgUseShortSlotTime = FALSE;
#endif
			if (prStaRec->u2HtCapInfo & HT_CAP_INFO_40M_INTOLERANT)
				fgSta40mIntolerant = TRUE;
		}
	}			/* end of LINK_FOR_EACH_ENTRY */

	/* Check if HT operation IE
	 * about 20/40M bandwidth shall be updated
	 */
	if (prBssInfo->eBssSCO != CHNL_EXT_SCN) {
		if (/*!LINK_IS_EMPTY(prStaList) && */ !fgSta40mIntolerant &&
		    !prBssInfo->fgObssActionForcedTo20M
		    && !prBssInfo->fgObssBeaconForcedTo20M) {

			ucHtOpInfo1 = (uint8_t)
				(((uint32_t) prBssInfo->eBssSCO)
					| HT_OP_INFO1_STA_CHNL_WIDTH);
		}
	}

	/* Check if any new parameter may be updated */
	if (prBssInfo->fgErpProtectMode != fgErpProtectMode ||
	    prBssInfo->eHtProtectMode != eHtProtectMode ||
	    prBssInfo->eGfOperationMode != eGfOperationMode ||
	    prBssInfo->ucHtOpInfo1 != ucHtOpInfo1 ||
	    prBssInfo->fgUseShortPreamble != fgUseShortPreamble ||
	    prBssInfo->fgUseShortSlotTime != fgUseShortSlotTime) {

		prBssInfo->fgErpProtectMode = fgErpProtectMode;
		prBssInfo->eHtProtectMode = eHtProtectMode;
		prBssInfo->eGfOperationMode = eGfOperationMode;
		prBssInfo->ucHtOpInfo1 = ucHtOpInfo1;
		prBssInfo->fgUseShortPreamble = fgUseShortPreamble;
		prBssInfo->fgUseShortSlotTime = fgUseShortSlotTime;

		if (fgUseShortSlotTime)
			prBssInfo->u2CapInfo |= CAP_INFO_SHORT_SLOT_TIME;
		else
			prBssInfo->u2CapInfo &= ~CAP_INFO_SHORT_SLOT_TIME;

		rlmSyncOperationParams(prAdapter, prBssInfo);
		fgUpdateBeacon = TRUE;
	}

	/* Update Beacon content if related IE content is changed */
	if (fgUpdateBeacon)
		bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);

	return fgUpdateBeacon;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
enum ENUM_CHNL_EXT rlmDecideScoForAP(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	struct DOMAIN_SUBBAND_INFO *prSubband;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;
	uint8_t ucSecondChannel, i, j;
	enum ENUM_CHNL_EXT eSCO;
	enum ENUM_CHNL_EXT eTempSCO;
	/*chip capability*/
	uint8_t ucMaxBandwidth = MAX_BW_80_80_MHZ;

	eSCO = CHNL_EXT_SCN;
	eTempSCO = CHNL_EXT_SCN;

	if (prBssInfo->eBand == BAND_2G4) {
		if (prBssInfo->ucPrimaryChannel != 14)
			eSCO = (prBssInfo->ucPrimaryChannel > 7)
				? CHNL_EXT_SCB : CHNL_EXT_SCA;
	} else {
		if (regd_is_single_sku_en(prAdapter)) {
			if (rlmDomainIsLegalChannel(prAdapter,
					prBssInfo->eBand,
					prBssInfo->ucPrimaryChannel))
				eSCO = rlmSelectSecondaryChannelType(prAdapter,
					prBssInfo->eBand,
					prBssInfo->ucPrimaryChannel);
		} else {
		prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
		ASSERT(prDomainInfo);

		for (i = 0; i < MAX_SUBBAND_NUM; i++) {
			prSubband = &prDomainInfo->rSubBand[i];
			if (prSubband->ucBand == prBssInfo->eBand) {
				for (j = 0; j < prSubband->ucNumChannels; j++) {
					if ((prSubband->ucFirstChannelNum
						+ j * prSubband->ucChannelSpan)
					    == prBssInfo->ucPrimaryChannel) {
						eSCO = (j & 1)
							? CHNL_EXT_SCB
							: CHNL_EXT_SCA;
						break;
					}
				}

				if (j < prSubband->ucNumChannels)
					break;	/* Found */
			}
		}
	}
	}

	/* Check if it is boundary channel
	 * and 40MHz BW is permitted
	 */
	if (eSCO != CHNL_EXT_SCN) {
		ucSecondChannel = (eSCO == CHNL_EXT_SCA)
			? (prBssInfo->ucPrimaryChannel + CHNL_SPAN_20)
			: (prBssInfo->ucPrimaryChannel - CHNL_SPAN_20);

		if (!rlmDomainIsLegalChannel(prAdapter,
			prBssInfo->eBand,
			ucSecondChannel))
			eSCO = CHNL_EXT_SCN;
	}

	/* Overwrite SCO settings by wifi cfg */
	if (IS_BSS_P2P(prBssInfo)) {
		/* AP mode */
		if (IS_BSS_AP(prAdapter, prBssInfo)) {
			if (prAdapter->rWifiVar.ucApSco == CHNL_EXT_SCA
				|| prAdapter->rWifiVar.ucApSco == CHNL_EXT_SCB)
				eTempSCO =
					(enum ENUM_CHNL_EXT)
						prAdapter->rWifiVar.ucApSco;
#if ((CFG_SUPPORT_TWT == 1) && (CFG_SUPPORT_TWT_HOTSPOT == 1))
				prBssInfo->twt_flow_id_bitmap = 0;
				prBssInfo->aeTWTRespState = 0;
				LINK_INITIALIZE(&prBssInfo->twt_sch_link);

				for (i = 0; i < TWT_MAX_FLOW_NUM; i++)
					prBssInfo->arTWTSta[i].agrt_tbl_idx = i;

				DBGLOG(RLM, INFO,
					"WLAN AP BSS_INFO[%d] TWT flow id bitmap=%d\n",
					prBssInfo->ucBssIndex,
					prBssInfo->twt_flow_id_bitmap);
#endif
		}
		/* P2P mode */
		else {
			if (prAdapter->rWifiVar.ucP2pGoSco == CHNL_EXT_SCA ||
			    prAdapter->rWifiVar.ucP2pGoSco == CHNL_EXT_SCB) {
				eTempSCO =
					(enum ENUM_CHNL_EXT)
						prAdapter->rWifiVar.ucP2pGoSco;
			}
		}

		/* Check again if it is boundary channel
		 * and 40MHz BW is permitted
		 */
		if (eTempSCO != CHNL_EXT_SCN) {
			ucSecondChannel = (eTempSCO == CHNL_EXT_SCA)
				? (prBssInfo->ucPrimaryChannel + 4)
				: (prBssInfo->ucPrimaryChannel - 4);
			if (rlmDomainIsLegalChannel(prAdapter,
				prBssInfo->eBand,
				ucSecondChannel))
				eSCO = eTempSCO;
		}
	}

	/* Overwrite SCO settings by wifi cfg bandwidth setting */
	if (IS_BSS_P2P(prBssInfo)) {
		/* AP mode */
		if (IS_BSS_AP(prAdapter, prBssInfo)) {
			ucMaxBandwidth = p2pFuncGetMaxBw(prAdapter,
							 prBssInfo->eBand,
							 TRUE);
		}
		/* P2P mode */
		else {
			ucMaxBandwidth = p2pFuncGetMaxBw(prAdapter,
							 prBssInfo->eBand,
							 FALSE);
		}

		if (ucMaxBandwidth < MAX_BW_40MHZ)
			eSCO = CHNL_EXT_SCN;
	}

	return eSCO;
}

enum ENUM_CHNL_EXT rlmGetScoByChnInfo(struct ADAPTER *prAdapter,
		struct RF_CHANNEL_INFO *prChannelInfo)
{
	enum ENUM_CHNL_EXT eSCO = CHNL_EXT_SCN;
	int32_t i4DeltaBw;
	uint32_t u4AndOneSCO;

	if (prChannelInfo->ucChnlBw == MAX_BW_40MHZ) {
		/* If BW 40, compare S0 and primary channel freq */
		if (prChannelInfo->u4CenterFreq1
			> prChannelInfo->u2PriChnlFreq)
			eSCO = CHNL_EXT_SCA;
		else
			eSCO = CHNL_EXT_SCB;
	} else if (prChannelInfo->ucChnlBw > MAX_BW_40MHZ) {
		/* P: PriChnlFreq,
		 * A: CHNL_EXT_SCA,
		 * B: CHNL_EXT_SCB, -:BW SPAN 5M
		 */
		/* --|----|--CenterFreq1--|----|-- */
		/* --|----|--CenterFreq1--B----P-- */
		/* --|----|--CenterFreq1--P----A-- */
		i4DeltaBw = prChannelInfo->u2PriChnlFreq
			- prChannelInfo->u4CenterFreq1;
		u4AndOneSCO = CHNL_EXT_SCB;
		eSCO = CHNL_EXT_SCA;
		if (i4DeltaBw < 0) {
			/* --|----|--CenterFreq1--|----|-- */
			/* --P----A--CenterFreq1--|----|-- */
			/* --B----P--CenterFreq1--|----|-- */
			u4AndOneSCO = CHNL_EXT_SCA;
			eSCO = CHNL_EXT_SCB;
			i4DeltaBw = -i4DeltaBw;
		}
		i4DeltaBw = i4DeltaBw - (CHANNEL_SPAN_20 >> 1);
		if ((i4DeltaBw/CHANNEL_SPAN_20) & 1)
			eSCO = u4AndOneSCO;
	}

	return eSCO;
}

static enum ENUM_CHNL_EXT rlmGetSco(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	enum ENUM_CHNL_EXT eSCO = CHNL_EXT_SCN;
	int32_t i4DeltaBw;
	uint32_t u4AndOneSCO;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo =
		(struct P2P_CONNECTION_REQ_INFO *) NULL;

	prP2pRoleFsmInfo = p2pFuncGetRoleByBssIdx(prAdapter,
		prBssInfo->ucBssIndex);

	if (prP2pRoleFsmInfo) {

		prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
		eSCO = CHNL_EXT_SCN;

		if (cnmGetBssMaxBw(prAdapter,
			prBssInfo->ucBssIndex) == MAX_BW_40MHZ) {
			/* If BW 40, compare S0 and primary channel freq */
			if (prP2pConnReqInfo->u4CenterFreq1
				> prP2pConnReqInfo->u2PriChnlFreq)
				eSCO = CHNL_EXT_SCA;
			else
				eSCO = CHNL_EXT_SCB;
		} else if (cnmGetBssMaxBw(prAdapter,
			prBssInfo->ucBssIndex) > MAX_BW_40MHZ) {
			/* P: PriChnlFreq,
			 * A: CHNL_EXT_SCA,
			 * B: CHNL_EXT_SCB, -:BW SPAN 5M
			 */
			/* --|----|--CenterFreq1--|----|-- */
			/* --|----|--CenterFreq1--B----P-- */
			/* --|----|--CenterFreq1--P----A-- */
			i4DeltaBw = prP2pConnReqInfo->u2PriChnlFreq
				- prP2pConnReqInfo->u4CenterFreq1;
			u4AndOneSCO = CHNL_EXT_SCB;
			eSCO = CHNL_EXT_SCA;
			if (i4DeltaBw < 0) {
				/* --|----|--CenterFreq1--|----|-- */
				/* --P----A--CenterFreq1--|----|-- */
				/* --B----P--CenterFreq1--|----|-- */
				u4AndOneSCO = CHNL_EXT_SCA;
				eSCO = CHNL_EXT_SCB;
				i4DeltaBw = -i4DeltaBw;
			}
			i4DeltaBw = i4DeltaBw - (CHANNEL_SPAN_20 >> 1);
			if ((i4DeltaBw/CHANNEL_SPAN_20) & 1)
				eSCO = u4AndOneSCO;
		}
	}

	return eSCO;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief: Get AP secondary channel offset from cfg80211 or wifi.cfg
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T, prBssInfo Pointer of BSS_INFO_T,
 *
 * \return ENUM_CHNL_EXT_T AP secondary channel offset
 */
/*----------------------------------------------------------------------------*/
enum ENUM_CHNL_EXT rlmGetScoForAP(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	enum ENUM_BAND eBand;
	uint8_t ucChannel;
	enum ENUM_CHNL_EXT eSCO;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;

	prP2pRoleFsmInfo = p2pFuncGetRoleByBssIdx(prAdapter,
		prBssInfo->ucBssIndex);

	if (!prAdapter->rWifiVar.ucApChnlDefFromCfg
		&& prP2pRoleFsmInfo) {
		eSCO = rlmGetSco(prAdapter, prBssInfo);
	} else {
		/* In this case, the first BSS's SCO is 40MHz
		 * and known, so AP can apply 40MHz bandwidth,
		 * but the first BSS's SCO may be changed
		 * later if its Beacon lost timeout occurs
		 */
		if (!(cnmPreferredChannel(prAdapter,
			&eBand, &ucChannel, &eSCO)
			&& eSCO != CHNL_EXT_SCN
			&& ucChannel == prBssInfo->ucPrimaryChannel
			&& eBand == prBssInfo->eBand))
			eSCO = rlmDecideScoForAP(prAdapter, prBssInfo);
	}
	return eSCO;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief: Get AP channel number of Channel Center Frequency Segment 0
 *           from cfg80211 or wifi.cfg
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T, prBssInfo Pointer of BSS_INFO_T,
 *
 * \return UINT_8 AP channel number of Channel Center Frequency Segment 0
 */
/*----------------------------------------------------------------------------*/
uint8_t rlmGetVhtS1ForAP(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	uint32_t ucFreq1Channel;

	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo =
		(struct P2P_CONNECTION_REQ_INFO *) NULL;

	prP2pRoleFsmInfo =
		p2pFuncGetRoleByBssIdx(prAdapter, prBssInfo->ucBssIndex);

	if (prBssInfo->ucVhtChannelWidth == VHT_OP_CHANNEL_WIDTH_20_40)
		return 0;

	if (!prAdapter->rWifiVar.ucApChnlDefFromCfg && prP2pRoleFsmInfo) {
		prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
		ucFreq1Channel =
			nicFreq2ChannelNum(
				prP2pConnReqInfo->u4CenterFreq1 * 1000);
	} else {
		ucFreq1Channel = nicGetS1(
			prBssInfo->eBand,
			prBssInfo->ucPrimaryChannel,
			prBssInfo->eBssSCO,
			rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo));
	}

	return ucFreq1Channel;
}

void rlmGetChnlInfoForCSA(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucCh,
	uint8_t ucBw,
	uint8_t ucBssIdx,
	struct RF_CHANNEL_INFO *prRfChnlInfo)
{
	struct BSS_INFO *prBssInfo = NULL;
	enum ENUM_BAND eBandOrig, eBandCsa;
	enum ENUM_CHNL_EXT eScoCsa;

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	prRfChnlInfo->ucChannelNum = ucCh;
	eScoCsa = nicGetSco(prAdapter, eBand, ucCh);

	eBandCsa = eBand;
	prRfChnlInfo->eBand = eBandCsa;

	/* temp replace BSS eBand to get BW of CSA band */
	eBandOrig = prBssInfo->eBand;
	prBssInfo->eBand = eBandCsa;
	prRfChnlInfo->ucChnlBw = cnmOpModeGetMaxBw(prAdapter, prBssInfo);
	prRfChnlInfo->eSco = eScoCsa;
	if (ucBw < MAX_BW_NUM && ucBw < prRfChnlInfo->ucChnlBw)
		prRfChnlInfo->ucChnlBw = ucBw;
#if (CFG_SUPPORT_802_11BE == 1)
	if (!(prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_EHT) &&
	    prRfChnlInfo->ucChnlBw >= MAX_BW_320_1MHZ)
		prRfChnlInfo->ucChnlBw = MAX_BW_160MHZ;
#endif
	nicReviseBwByCh(prAdapter, eBand, ucCh, eScoCsa,
			&prRfChnlInfo->ucChnlBw);
	prBssInfo->eBand = eBandOrig; /* Restore BSS eBand */

	prRfChnlInfo->u2PriChnlFreq = nicChannelNum2Freq(ucCh, eBandCsa) / 1000;
	prRfChnlInfo->u4CenterFreq1 = nicGetS1Freq(eBandCsa, ucCh, eScoCsa,
		prRfChnlInfo->ucChnlBw);
	prRfChnlInfo->u4CenterFreq2 = nicGetS2Freq(eBandCsa, ucCh,
		prRfChnlInfo->ucChnlBw);

	if (eBand == BAND_5G && ucCh >= 52 && ucCh <= 144)
		prRfChnlInfo->fgDFS = TRUE;
	else
		prRfChnlInfo->fgDFS = FALSE;
}

#if (CFG_SUPPORT_SAP_PUNCTURE == 1)
static void rlmPunctUpdateLegacyBw80(uint16_t u2Bitmap, uint8_t ucPriChannel,
				     uint8_t *pucSeg0)
{
	uint8_t ucFirstChan = *pucSeg0 - 6, ucSecChan;

	switch (u2Bitmap) {
	case 0x6:
		*pucSeg0 = 0;
		return;
	case 0x8:
	case 0x4:
	case 0x2:
	case 0x1:
	case 0xC:
	case 0x3:
		if (ucPriChannel < *pucSeg0)
			*pucSeg0 -= 4;
		else
			*pucSeg0 += 4;
		break;
	}

	if (ucPriChannel < *pucSeg0)
		ucSecChan = ucPriChannel + 4;
	else
		ucSecChan = ucPriChannel - 4;

	if (u2Bitmap & BIT((ucSecChan - ucFirstChan) / 4))
		*pucSeg0 = 0;
}

static void rlmPunctUpdateLegacyBw160(uint16_t u2Bitmap, uint8_t ucPriChannel,
				      uint8_t *pucBw, uint8_t *pucSeg0)
{
	if (ucPriChannel < *pucSeg0) {
		*pucSeg0 -= 8;
		if (u2Bitmap & 0x0F) {
			*pucBw = MAX_BW_40MHZ;
			rlmPunctUpdateLegacyBw80(u2Bitmap & 0xF, ucPriChannel,
						 pucSeg0);
		}
	} else {
		*pucSeg0 += 8;
		if (u2Bitmap & 0xF0) {
			*pucBw = MAX_BW_40MHZ;
			rlmPunctUpdateLegacyBw80((u2Bitmap & 0xF0) >> 4,
						 ucPriChannel, pucSeg0);
		}
	}
}

static void rlmPunctUpdateLegacyBw320(uint16_t u2Bitmap, uint8_t ucPriChannel,
				      uint8_t *pucBw, uint8_t *pucSeg0)
{
	if (ucPriChannel < *pucSeg0) {
		*pucSeg0 -= 16;
		if (u2Bitmap & 0x00FF) {
			*pucBw = MAX_BW_80MHZ;
			rlmPunctUpdateLegacyBw160(u2Bitmap & 0xFF,
						  ucPriChannel, pucBw,
						  pucSeg0);
		}
	} else {
		*pucSeg0 += 16;
		if (u2Bitmap & 0xFF00) {
			*pucBw = MAX_BW_80MHZ;
			rlmPunctUpdateLegacyBw160((u2Bitmap & 0xFF00) >> 8,
						  ucPriChannel, pucBw,
						  pucSeg0);
		}
	}
}

void rlmPunctUpdateLegacyBw(enum ENUM_BAND eBand, uint16_t u2Bitmap,
			    uint8_t ucPriChannel, uint8_t *pucBw,
			    uint8_t *pucSeg0, uint8_t *pucSeg1,
			    uint8_t *pucOpClass)
{
	uint8_t ucCenterCh;

	switch (*pucBw) {
	case MAX_BW_80MHZ:
	case MAX_BW_80_80_MHZ:
		ucCenterCh = *pucSeg0;
		break;
	case MAX_BW_160MHZ:
	case MAX_BW_320_1MHZ:
	case MAX_BW_320_2MHZ:
		ucCenterCh = *pucSeg1;
		break;
	default:
		return;
	}

	if ((*pucBw == MAX_BW_80MHZ || *pucBw == MAX_BW_80_80_MHZ) &&
	    (u2Bitmap & 0xF)) {
		*pucBw = MAX_BW_40MHZ;
		rlmPunctUpdateLegacyBw80(u2Bitmap & 0xF, ucPriChannel,
					 &ucCenterCh);
	}

	if (*pucBw == MAX_BW_160MHZ && (u2Bitmap & 0xFF)) {
		*pucBw = MAX_BW_80MHZ;
		rlmPunctUpdateLegacyBw160(u2Bitmap & 0xFF, ucPriChannel,
					  pucBw, &ucCenterCh);
	}

	if ((*pucBw == MAX_BW_320_1MHZ || *pucBw == MAX_BW_320_2MHZ) &&
	    (u2Bitmap & 0xFFFF)) {
		*pucBw = MAX_BW_160MHZ;
		rlmPunctUpdateLegacyBw320(u2Bitmap & 0xFFFF, ucPriChannel,
					  pucBw, &ucCenterCh);
	}

	*pucSeg0 = nicGetS1(eBand, ucPriChannel, CHNL_EXT_RES, *pucBw);
	*pucSeg1 = nicGetS2(eBand, ucPriChannel, *pucBw);

	if (pucOpClass) {
		struct RF_CHANNEL_INFO rChannelInfo;

		rChannelInfo.eBand = eBand;
		rChannelInfo.ucChnlBw = *pucBw;
		/* Sco no matter for BW > 40 MHz */
		rChannelInfo.eSco = CHNL_EXT_RES;
		rChannelInfo.ucChannelNum = ucPriChannel;
		rChannelInfo.u2PriChnlFreq =
			nicChannelNum2Freq(ucPriChannel,
					   eBand) / 1000;
		rChannelInfo.u4CenterFreq1 =
			nicGetS1Freq(eBand, ucPriChannel,
				     rChannelInfo.eSco, *pucBw);
		rChannelInfo.u4CenterFreq2 =
			nicGetS2Freq(eBand, ucPriChannel, *pucBw);

		*pucOpClass = nicChannelInfo2OpClass(&rChannelInfo);
	}
}

u_int8_t rlmValidatePunctBitmap(struct ADAPTER *prAdapter,
				enum ENUM_BAND eBand,
				enum ENUM_MAX_BANDWIDTH_SETTING eBw,
				uint8_t ucPriCh, uint16_t u2PunctBitmap)
{
	uint8_t ucIdx, ucCount, ucStartCh, ucCenterCh;
	uint16_t u2Bitmap, u2PriChBit;
	const uint16_t *prValidBitmaps;

	if (!u2PunctBitmap)
		return TRUE;

	u2Bitmap = ~u2PunctBitmap;
	if (!u2Bitmap)
		return FALSE;

	ucCenterCh = nicGetCenterCh(eBand, ucPriCh, CHNL_EXT_SCN, eBw);

	switch (eBw) {
	case MAX_BW_80MHZ:
		if (u2PunctBitmap > 0xF || ucCenterCh < 7)
			return FALSE;
		u2Bitmap &= 0xF;
		prValidBitmaps = PUNCT_VALID_BITMAP_80;
		ucCount = ARRAY_SIZE(PUNCT_VALID_BITMAP_80);
		ucStartCh = ucCenterCh - 6;
		break;

	case MAX_BW_160MHZ:
		if (u2PunctBitmap > 0xFF || ucCenterCh < 15)
			return FALSE;
		u2Bitmap &= 0xFF;
		prValidBitmaps = PUNCT_VALID_BITMAP_160;
		ucCount = ARRAY_SIZE(PUNCT_VALID_BITMAP_160);
		ucStartCh = ucCenterCh - 14;
		break;

	case MAX_BW_320_1MHZ:
	case MAX_BW_320_2MHZ:
		if (ucCenterCh < 31)
			return FALSE;
		u2Bitmap &= 0xFFFF;
		prValidBitmaps = PUNCT_VALID_BITMAP_320;
		ucCount = ARRAY_SIZE(PUNCT_VALID_BITMAP_320);
		ucStartCh = ucCenterCh - 30;
		break;

	default:
		return FALSE;
	}

	u2PriChBit = (uint16_t)((ucPriCh - ucStartCh) / 4);

	/* Primary channel cannot be punctured */
	if (!(u2Bitmap & BIT(u2PriChBit)))
		return FALSE;

	for (ucIdx = 0; ucIdx < ucCount; ucIdx++) {
		if (prValidBitmaps[ucIdx] == u2Bitmap)
			return TRUE;
	}

	return FALSE;
}
#endif /* CFG_SUPPORT_SAP_PUNCTURE */

#if (CFG_P2P2_SUPPORT_GC_REQ_CSA == 1)
void p2pRlmTriggerP2pGcCsa(struct ADAPTER *prAdapter,
			   struct BSS_INFO *prBssInfo,
			   struct STA_RECORD *prStarec,
			   struct RF_CHANNEL_INFO *prRfChnlInfo,
			   const uint8_t *prSuppOpClassIe,
			   const uint8_t ucSuppOpClassIeLen)
{
	struct CHANNEL_USAGE_REQ_PARAM *prParam = NULL;

	if (!prStarec->fgCapGcCsaSupp)
		return;

	prParam = (struct CHANNEL_USAGE_REQ_PARAM *)
		kalMemZAlloc(sizeof(*prParam), VIR_MEM_TYPE);
	if (!prParam) {
		DBGLOG(RLM, ERROR,
			"Alloc prParam failed.\n");
		goto exit;
	}

	if (prBssInfo->ucWnmDialogToken == 0)
		prBssInfo->ucWnmDialogToken++;
	prParam->ucDialogToken = prBssInfo->ucWnmDialogToken;
	prParam->ucMode = CHANNEL_USAGE_MODE_CHAN_SWITCH_REQ;
	prParam->ucTargetOpClass = nicChannelInfo2OpClass(prRfChnlInfo);
	prParam->ucTargetOpChannel = prRfChnlInfo->ucChannelNum;
	prParam->prSuppOpClassIe = prSuppOpClassIe;
	prParam->ucSuppOpClassIeLen = ucSuppOpClassIeLen;

	rlmSendChanUsageReqFrame(prAdapter, prBssInfo, prStarec, prParam);

exit:
	if (prParam)
		kalMemFree(prParam, VIR_MEM_TYPE, sizeof(*prParam));
}
#endif /* CFG_P2P2_SUPPORT_GC_REQ_CSA */

#if (CFG_P2P2_SUPPORT_CAP_NOTIFICATION == 1)
void p2pRlmReSyncCapAfterCsa(struct ADAPTER *prAdapter,
			     struct BSS_INFO *prBssInfo,
			     struct STA_RECORD *prStarec)
{
	struct MSDU_INFO *prMsduInfo;
	struct RF_CHANNEL_INFO rRfChnlInfo;
	struct CHANNEL_USAGE_REQ_PARAM rParam = {0};
	uint32_t u4CapIeLen = 0;
	uint16_t u2Offset = 0;
	uint8_t *pucIE = NULL;

	u4CapIeLen += (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP);
#if CFG_SUPPORT_802_11AC
	u4CapIeLen += (ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP);
#endif /* CFG_SUPPORT_802_11AC */
#if CFG_SUPPORT_802_11AX
	u4CapIeLen += heRlmCalculateHeCapIELen(prAdapter,
					       prBssInfo->ucBssIndex,
					       prStarec);
#if (CFG_SUPPORT_WIFI_6G == 1)
	u4CapIeLen += (ELEM_HDR_LEN + ELEM_MAX_LEN_HE_6G_CAP);
#endif /* CFG_SUPPORT_WIFI_6G */
#endif /* CFG_SUPPORT_802_11AX */
#if (CFG_SUPPORT_802_11BE == 1)
	u4CapIeLen += ehtRlmCalculateCapIELen(prAdapter,
					      prBssInfo->ucBssIndex,
					      prStarec);
#endif /* CFG_SUPPORT_802_11BE */

	prMsduInfo = cnmMgtPktAlloc(prAdapter, u4CapIeLen);
	if (!prMsduInfo) {
		DBGLOG(RLM, ERROR, "cnmMgtPktAlloc failed.\n");
		return;
	}

	prMsduInfo->ucBssIndex = prBssInfo->ucBssIndex;
	prMsduInfo->ucStaRecIndex = prStarec->ucIndex;

	rlmReqGenerateHtCapIE(prAdapter, prMsduInfo);
#if CFG_SUPPORT_802_11AC
	rlmReqGenerateVhtCapIE(prAdapter, prMsduInfo);
#endif /* CFG_SUPPORT_802_11AC */
#if CFG_SUPPORT_802_11AX
	heRlmReqGenerateHeCapIE(prAdapter, prMsduInfo);
#if (CFG_SUPPORT_WIFI_6G == 1)
	heRlmReqGenerateHe6gBandCapIE(prAdapter, prMsduInfo);
#endif /* CFG_SUPPORT_WIFI_6G */
#endif /* CFG_SUPPORT_802_11AX */
#if (CFG_SUPPORT_802_11BE == 1)
	ehtRlmReqGenerateCapIE(prAdapter, prMsduInfo);
#endif /* CFG_SUPPORT_802_11BE */

	pucIE = (uint8_t *)prMsduInfo->prPacket;

	kalMemZero(&rRfChnlInfo, sizeof(rRfChnlInfo));
	rRfChnlInfo.eBand = prBssInfo->eBand;
	rRfChnlInfo.ucChnlBw =
		rlmVhtBw2OpBw(prBssInfo->ucVhtChannelWidth,
			      prBssInfo->eBssSCO);
	rRfChnlInfo.eSco = prBssInfo->eBssSCO;
	rRfChnlInfo.u4CenterFreq1 =
		nicGetS1Freq(prBssInfo->eBand,
			     prBssInfo->ucPrimaryChannel,
			     prBssInfo->eBssSCO,
			     rRfChnlInfo.ucChnlBw);
	rRfChnlInfo.u4CenterFreq2 =
		nicGetS2Freq(prBssInfo->eBand,
			     prBssInfo->ucPrimaryChannel,
			     rRfChnlInfo.ucChnlBw);
	rRfChnlInfo.u2PriChnlFreq =
		nicChannelNum2Freq(prBssInfo->ucPrimaryChannel,
				   prBssInfo->eBand) / 1000;
	rRfChnlInfo.ucChannelNum = prBssInfo->ucPrimaryChannel;
	rRfChnlInfo.fgDFS = prBssInfo->eBand == BAND_5G ?
		rlmDomainIsDfsChnls(prAdapter,
				    prBssInfo->ucPrimaryChannel) :
		FALSE;

	if (prBssInfo->ucWnmDialogToken == 0)
		prBssInfo->ucWnmDialogToken++;
	rParam.ucDialogToken = prBssInfo->ucWnmDialogToken;
	rParam.ucMode = CHANNEL_USAGE_MODE_CAP_NOTIF;
	rParam.ucTargetOpClass = nicChannelInfo2OpClass(&rRfChnlInfo);
	rParam.ucTargetOpChannel = prBssInfo->ucPrimaryChannel;
	IE_FOR_EACH(pucIE, prMsduInfo->u2FrameLength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_HT_CAP:
			rParam.prIeHtCap = pucIE;
			rParam.ucIeHtCapSize = IE_SIZE(pucIE);
			break;
#if CFG_SUPPORT_802_11AC
		case ELEM_ID_VHT_CAP:
			rParam.prIeVhtCap = pucIE;
			rParam.ucIeVhtCapSize = IE_SIZE(pucIE);
			break;
#endif /* CFG_SUPPORT_802_11AC */
		case ELEM_ID_RESERVED:
		{
			switch (IE_ID_EXT(pucIE)) {
#if (CFG_SUPPORT_802_11AX == 1)
			case ELEM_EXT_ID_HE_CAP:
				rParam.prIeHeCap = pucIE;
				rParam.ucIeHeCapSize = IE_SIZE(pucIE);
				break;
#if (CFG_SUPPORT_WIFI_6G == 1)
			case ELEM_EXT_ID_HE_6G_BAND_CAP:
				rParam.prIeHe6gCap = pucIE;
				rParam.ucIeHe6gCapSize = IE_SIZE(pucIE);
				break;
#endif /* CFG_SUPPORT_WIFI_6G */
#endif /* CFG_SUPPORT_802_11AX */
#if (CFG_SUPPORT_802_11BE == 1)
			case ELEM_EXT_ID_EHT_CAPS:
				rParam.prIeEhtCap = pucIE;
				rParam.ucIeEhtCapSize = IE_SIZE(pucIE);
				break;
#endif /* CFG_SUPPORT_802_11BE */
			default:
				break;
			}
		}
			break;
		default:
			break;
		}
	}

	rlmSendChanUsageReqFrame(prAdapter, prBssInfo, prStarec, &rParam);

	cnmMgtPktFree(prAdapter, prMsduInfo);
}
#endif /* CFG_P2P2_SUPPORT_CAP_NOTIFICATION */

static uint32_t rlmChanUsageReqTxDone(struct ADAPTER *prAdapter,
				      struct MSDU_INFO *prMsduInfo,
				      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	DBGLOG(RLM, INFO,
		"bss=%u, sta=%u, seq=%u, widx=%u, pid=%u, status=%d\n",
		prMsduInfo->ucBssIndex,
		prMsduInfo->ucStaRecIndex,
		prMsduInfo->ucTxSeqNum,
		prMsduInfo->ucWlanIndex,
		prMsduInfo->ucPID,
		rTxDoneStatus);

	return WLAN_STATUS_SUCCESS;
}

void rlmSendChanUsageReqFrame(struct ADAPTER *prAdapter,
			      struct BSS_INFO *prBssInfo,
			      struct STA_RECORD *prStarec,
			      struct CHANNEL_USAGE_REQ_PARAM *prParam)
{
#define MAX_LEN_OF_CHANNEL_USAGE_REQ_FRAME		(1024)

	struct MSDU_INFO *prMsduInfo;
	struct ACTION_CHANNEL_USAGE_FRAME *prFrame;
	struct IE_CHANNEL_USAGE *prIeChanUsage;
	struct IE_CHANNEL_USAGE_ENTRY *prIeChanUsageEntry;
	uint16_t u2EstimatedFrameLen = MAX_LEN_OF_CHANNEL_USAGE_REQ_FRAME;
	uint8_t *start, *pos;

	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (!prMsduInfo) {
		DBGLOG(RLM, ERROR,
			"cnmMgtPktAlloc failed, size=%u\n",
			u2EstimatedFrameLen);
		return;
	}

	DBGLOG(RLM, INFO,
		"bss=%u sta=%u token=0x%x mode=%u class/channel=%u/%u %u/%u/%u/%u/%u/%u\n",
		prBssInfo->ucBssIndex,
		prStarec->ucIndex,
		prParam->ucDialogToken,
		prParam->ucMode,
		prParam->ucTargetOpClass,
		prParam->ucTargetOpChannel,
		prParam->ucSuppOpClassIeLen,
		prParam->ucIeHtCapSize,
		prParam->ucIeVhtCapSize,
		prParam->ucIeHeCapSize,
		prParam->ucIeHe6gCapSize,
		prParam->ucIeEhtCapSize);

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);
	start = pos = (uint8_t *)prMsduInfo->prPacket;
	prFrame = (struct ACTION_CHANNEL_USAGE_FRAME *)pos;

	prFrame->u2FrameCtrl = MAC_FRAME_ACTION;
	COPY_MAC_ADDR(prFrame->aucDestAddr, prStarec->aucMacAddr);
	COPY_MAC_ADDR(prFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prFrame->aucBSSID, prBssInfo->aucBSSID);
	prFrame->ucCategory = CATEGORY_WNM_ACTION;
	prFrame->ucAction = ACTION_WNM_CHANNEL_USAGE_REQ;
	prFrame->ucDialogToken = prParam->ucDialogToken;
	pos += sizeof(*prFrame);

	prIeChanUsage = (struct IE_CHANNEL_USAGE *)pos;
	prIeChanUsage->ucId = ELEM_ID_CHNNEL_USAGE;
	prIeChanUsage->ucLength = 1 + 2;
	prIeChanUsage->ucMode = prParam->ucMode;
	pos += sizeof(*prIeChanUsage);

	prIeChanUsageEntry = (struct IE_CHANNEL_USAGE_ENTRY *)pos;
	prIeChanUsageEntry->ucOpClass = prParam->ucTargetOpClass;
	prIeChanUsageEntry->ucChannel = prParam->ucTargetOpChannel;
	pos += sizeof(*prIeChanUsageEntry);

	if (prParam->ucMode != CHANNEL_USAGE_MODE_CAP_NOTIF &&
	    prParam->prSuppOpClassIe && prParam->ucSuppOpClassIeLen) {
		kalMemCopy(pos,
			   prParam->prSuppOpClassIe,
			   prParam->ucSuppOpClassIeLen);
		pos += prParam->ucSuppOpClassIeLen;
	}

	/* TWT element */
	/* Timeout interval element */

	if (prParam->ucMode == CHANNEL_USAGE_MODE_CHAN_SWITCH_REQ ||
	    prParam->ucMode == CHANNEL_USAGE_MODE_CAP_NOTIF) {
		if (prParam->prIeHtCap && prParam->ucIeHtCapSize) {
			kalMemCopy(pos,
				   prParam->prIeHtCap,
				   prParam->ucIeHtCapSize);
			pos += prParam->ucIeHtCapSize;
		}
		if (prParam->prIeVhtCap && prParam->ucIeVhtCapSize) {
			kalMemCopy(pos,
				   prParam->prIeVhtCap,
				   prParam->ucIeVhtCapSize);
			pos += prParam->ucIeVhtCapSize;
		}
		if (prParam->prIeHeCap && prParam->ucIeHeCapSize) {
			kalMemCopy(pos,
				   prParam->prIeHeCap,
				   prParam->ucIeHeCapSize);
			pos += prParam->ucIeHeCapSize;
		}
		if (prParam->prIeHe6gCap && prParam->ucIeHe6gCapSize) {
			kalMemCopy(pos,
				   prParam->prIeHe6gCap,
				   prParam->ucIeHe6gCapSize);
			pos += prParam->ucIeHe6gCapSize;
		}
		if (prParam->prIeEhtCap && prParam->ucIeEhtCapSize) {
			kalMemCopy(pos,
				   prParam->prIeEhtCap,
				   prParam->ucIeEhtCapSize);
			pos += prParam->ucIeEhtCapSize;
		}
	}

	TX_SET_MMPDU(prAdapter, prMsduInfo, prBssInfo->ucBssIndex,
		     prStarec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     (uint16_t)(pos - start),
		     rlmChanUsageReqTxDone,
		     MSDU_RATE_MODE_AUTO);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
				  MSDU_CONTROL_FLAG_FORCE_LINK,
				  TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}

static uint32_t rlmChanUsageRespTxDone(struct ADAPTER *prAdapter,
				       struct MSDU_INFO *prMsduInfo,
				       enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	DBGLOG(RLM, INFO,
		"bss=%u, sta=%u, seq=%u, widx=%u, pid=%u, status=%d\n",
		prMsduInfo->ucBssIndex,
		prMsduInfo->ucStaRecIndex,
		prMsduInfo->ucTxSeqNum,
		prMsduInfo->ucWlanIndex,
		prMsduInfo->ucPID,
		rTxDoneStatus);

	return WLAN_STATUS_SUCCESS;
}

void rlmSendChanUsageRespFrame(struct ADAPTER *prAdapter,
			       struct BSS_INFO *prBssInfo,
			       struct STA_RECORD *prStarec,
			       struct CHANNEL_USAGE_RESP_PARAM *prParam)
{
#define MAX_LEN_OF_CHANNEL_USAGE_RESP_FRAME		(1024)

	struct MSDU_INFO *prMsduInfo;
	struct ACTION_CHANNEL_USAGE_FRAME *prFrame;
	struct IE_CHANNEL_USAGE *prIeChanUsage;
	struct IE_CHANNEL_USAGE_ENTRY *prIeChanUsageEntry;
	uint16_t u2EstimatedFrameLen = MAX_LEN_OF_CHANNEL_USAGE_RESP_FRAME;
	uint8_t *start, *pos;

	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (!prMsduInfo) {
		DBGLOG(RLM, ERROR,
			"cnmMgtPktAlloc failed, size=%u\n",
			u2EstimatedFrameLen);
		return;
	}

	DBGLOG(RLM, INFO,
		"bss=%u sta=%u token=0x%x mode=%u class/channel=%u/%u country=%s\n",
		prBssInfo->ucBssIndex,
		prStarec->ucIndex,
		prParam->ucDialogToken,
		prParam->ucMode,
		prParam->ucTargetOpClass,
		prParam->ucTargetOpChannel,
		prParam->aucCountry);

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);
	start = pos = (uint8_t *)prMsduInfo->prPacket;
	prFrame = (struct ACTION_CHANNEL_USAGE_FRAME *)pos;

	prFrame->u2FrameCtrl = MAC_FRAME_ACTION;
	COPY_MAC_ADDR(prFrame->aucDestAddr, prStarec->aucMacAddr);
	COPY_MAC_ADDR(prFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prFrame->aucBSSID, prBssInfo->aucBSSID);
	prFrame->ucCategory = CATEGORY_WNM_ACTION;
	prFrame->ucAction = ACTION_WNM_CHANNEL_USAGE_RESP;
	prFrame->ucDialogToken = prParam->ucDialogToken;
	pos += sizeof(*prFrame);

	prIeChanUsage = (struct IE_CHANNEL_USAGE *)pos;
	prIeChanUsage->ucId = ELEM_ID_CHNNEL_USAGE;
	prIeChanUsage->ucLength = 1;
	prIeChanUsage->ucMode = prParam->ucMode;
	pos += sizeof(*prIeChanUsage);

	if (prParam->ucTargetOpClass && prParam->ucTargetOpChannel) {
		prIeChanUsageEntry = (struct IE_CHANNEL_USAGE_ENTRY *)pos;
		prIeChanUsageEntry->ucOpClass = prParam->ucTargetOpClass;
		prIeChanUsageEntry->ucChannel = prParam->ucTargetOpChannel;
		pos += sizeof(*prIeChanUsageEntry);

		prIeChanUsage->ucLength += 2;
	}

	kalMemCopy(pos, prParam->aucCountry, sizeof(prParam->aucCountry));
	pos += sizeof(prParam->aucCountry);

	/* Power constraint element */
	/* EDCA parameter set element */
	/* Transmit power envelope element */
	/* TWT element */
	/* Timeout interval element */

	TX_SET_MMPDU(prAdapter, prMsduInfo, prBssInfo->ucBssIndex,
		     prStarec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     (uint16_t)(pos - start),
		     rlmChanUsageRespTxDone,
		     MSDU_RATE_MODE_AUTO);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
				  MSDU_CONTROL_FLAG_FORCE_LINK,
				  TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}

static void p2pRlmHandleChanUsageReqFrame(struct ADAPTER *prAdapter,
					  struct SW_RFB *prSwRfb,
					  struct BSS_INFO *prBssInfo,
					  struct STA_RECORD *prStaRec)
{
	struct ACTION_CHANNEL_USAGE_FRAME *prFrame;
	struct IE_CHANNEL_USAGE *prIeChanUsage;
	struct IE_CHANNEL_USAGE_ENTRY *prIeChanUsageEntry;
	uint32_t u4RemainLen = prSwRfb->u2PacketLen;

	if (u4RemainLen < (sizeof(*prFrame) + sizeof(*prIeChanUsage) +
			   sizeof(*prIeChanUsageEntry))) {
		DBGLOG(RLM, ERROR,
			"Invalid length (%u) for channel usage request frame.\n",
			u4RemainLen);
		return;
	}

	u4RemainLen -= (sizeof(*prFrame) + sizeof(*prIeChanUsage) +
			sizeof(*prIeChanUsageEntry));
	prFrame = (struct ACTION_CHANNEL_USAGE_FRAME *)prSwRfb->pvHeader;
	if (prFrame->ucDialogToken == 0) {
		DBGLOG(RLM, ERROR,
			"Invalid token\n");
		return;
	}

	prIeChanUsage = (struct IE_CHANNEL_USAGE *)prFrame->aucInfoElem;
	prIeChanUsageEntry = (struct IE_CHANNEL_USAGE_ENTRY *)
		prIeChanUsage->aucEntries;

	switch (prIeChanUsage->ucMode) {
#if (CFG_P2P2_SUPPORT_GC_REQ_CSA == 1)
	case CHANNEL_USAGE_MODE_CHAN_SWITCH_REQ:
	{
		struct CHANNEL_USAGE_RESP_PARAM rParam = {0};
		struct RF_CHANNEL_INFO rChnlInfo = {0};
		uint16_t u2Bw, u2CountryCode;
		u_int8_t fgReqAcceptable;

		if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgP2pGcCsaReq))
			break;

		u2CountryCode = prAdapter->rWifiVar.u2CountryCode;

		DBGLOG(RLM, INFO,
			"MODE_CHAN_SWITCH_REQ token=%u op_class/op_channel=%u/%u\n",
			prFrame->ucDialogToken,
			prIeChanUsageEntry->ucOpClass,
			prIeChanUsageEntry->ucChannel);

		u2Bw = rlmOpClassToBandwidth(prIeChanUsageEntry->ucOpClass);

		rChnlInfo.eBand =
			scanOpClassToBand(prIeChanUsageEntry->ucOpClass);
		rChnlInfo.ucChannelNum = prIeChanUsageEntry->ucChannel;
		rChnlInfo.u2PriChnlFreq =
			nicChannelNum2Freq(rChnlInfo.ucChannelNum,
					   rChnlInfo.eBand) / 1000;
		switch (u2Bw) {
		case BW_20:
			rChnlInfo.ucChnlBw = MAX_BW_20MHZ;
			break;
		case BW_40:
			rChnlInfo.ucChnlBw = MAX_BW_40MHZ;
			break;
		case BW_80:
			rChnlInfo.ucChnlBw = MAX_BW_80MHZ;
			break;
		case BW_160:
			rChnlInfo.ucChnlBw = MAX_BW_160MHZ;
			break;
		case BW_8080:
			rChnlInfo.ucChnlBw = MAX_BW_80_80_MHZ;
			break;
		case BW_320:
			rChnlInfo.ucChnlBw = MAX_BW_320_1MHZ;
			break;
		default:
			DBGLOG(RLM, ERROR,
				"Unknown bandwidth=%u\n",
				u2Bw);
			rChnlInfo.ucChnlBw = MAX_BW_20MHZ;
			break;
		}
		if (u2Bw > BW_20)
			rChnlInfo.eSco = nicGetSco(prAdapter, rChnlInfo.eBand,
						   rChnlInfo.ucChannelNum);
		rChnlInfo.u4CenterFreq1 =
			nicGetS1Freq(rChnlInfo.eBand,
				     rChnlInfo.ucChannelNum,
				     rChnlInfo.eSco,
				     rChnlInfo.ucChnlBw);
		rChnlInfo.u4CenterFreq2 =
			nicGetS2Freq(rChnlInfo.eBand,
				     rChnlInfo.ucChannelNum,
				     rChnlInfo.ucChnlBw);

		fgReqAcceptable =
			ccmIsGcCsaReqChanAcceptable(prAdapter, prBssInfo,
						    &rChnlInfo);

		rParam.ucDialogToken = prFrame->ucDialogToken;
		rParam.ucMode = prIeChanUsage->ucMode;
		if (fgReqAcceptable) {
			rParam.ucTargetOpClass =
				prIeChanUsageEntry->ucOpClass;
			rParam.ucTargetOpChannel =
				prIeChanUsageEntry->ucChannel;
		}
		kalSnprintf(rParam.aucCountry,
			    sizeof(rParam.aucCountry),
			    "%c%c",
			    (u2CountryCode & 0xff00) >> 8,
			    (u2CountryCode & 0x00ff));

		rlmSendChanUsageRespFrame(prAdapter, prBssInfo, prStaRec,
					  &rParam);

		if (fgReqAcceptable == TRUE)
			cnmIdcCsaReq(prAdapter, rChnlInfo.eBand,
			     prIeChanUsageEntry->ucChannel,
			     MODE_DISALLOW_TX,
			     rlmOpClassToBandwidth(rParam.ucTargetOpClass),
			     prBssInfo->u4PrivateData);
	}
		break;
#endif /* CFG_P2P2_SUPPORT_GC_REQ_CSA */

#if (CFG_P2P2_SUPPORT_CAP_NOTIFICATION == 1)
	case CHANNEL_USAGE_MODE_CAP_NOTIF:
	{
		uint8_t *pucIE;
		uint16_t u2IELength;

		if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgP2pCapNotif))
			break;

		pucIE = (uint8_t *)(prSwRfb->pvHeader +
			(prSwRfb->u2PacketLen - u4RemainLen));
		u2IELength = u4RemainLen;

		DBGLOG(RLM, INFO, "MODE_CAP_NOTIF\n");
		DBGLOG_MEM8(RLM, TRACE, pucIE, u2IELength);

		/* re-use assoc req flow to update ht/vht/he/he_6g/eht cap */
		rlmProcessAssocReq(prAdapter, prStaRec, pucIE, u2IELength);
		cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
		cnmDumpStaRec(prAdapter, prStaRec->ucIndex);
	}
		break;
#endif /* CFG_P2P2_SUPPORT_CAP_NOTIFICATION */

	default:
		break;
	}
}

static void p2pRlmHandleChanUsageRespFrame(struct ADAPTER *prAdapter,
					   struct SW_RFB *prSwRfb,
					   struct BSS_INFO *prBssInfo,
					   struct STA_RECORD *prStaRec)
{
	struct ACTION_CHANNEL_USAGE_FRAME *prFrame;
	struct IE_CHANNEL_USAGE *prIeChanUsage;
	uint32_t u4RemainLen = prSwRfb->u2PacketLen;

	if (u4RemainLen < sizeof(*prFrame)) {
		DBGLOG(RLM, ERROR,
			"Invalid length (%u) for channel usage response frame.\n",
			u4RemainLen);
		return;
	}

	u4RemainLen -= sizeof(*prFrame);
	prFrame = (struct ACTION_CHANNEL_USAGE_FRAME *)prSwRfb->pvHeader;
	if (prFrame->ucDialogToken == 0) {
		DBGLOG(RLM, ERROR,
			"Invalid token\n");
		return;
	} else if (prBssInfo->ucWnmDialogToken != prFrame->ucDialogToken) {
		DBGLOG(RLM, WARN,
			"WNM token mismatch, expected %u but %u\n",
			prBssInfo->ucWnmDialogToken,
			prFrame->ucDialogToken);
		return;
	}

	if (u4RemainLen < sizeof(*prIeChanUsage)) {
		DBGLOG(RLM, ERROR,
			"Invalid length (%u) for IE_CHANNEL_USAGE.\n",
			u4RemainLen);
		return;
	}

	u4RemainLen -= sizeof(*prIeChanUsage);
	prIeChanUsage = (struct IE_CHANNEL_USAGE *)prFrame->aucInfoElem;
	if (prIeChanUsage->ucId != ELEM_ID_CHNNEL_USAGE) {
		DBGLOG(RLM, WARN,
			"Invalid channel usage id %u\n",
			prIeChanUsage->ucId);
		return;
	}

	switch (prIeChanUsage->ucMode) {
	case CHANNEL_USAGE_MODE_CHAN_SWITCH_REQ:
	{
		struct IE_CHANNEL_USAGE_ENTRY *prIeChanUsageEntry;

		if (prIeChanUsage->ucLength >= (sizeof(*prIeChanUsageEntry) +
		    sizeof(prIeChanUsage->ucMode))) {
			u4RemainLen -= sizeof(*prIeChanUsageEntry);
			prIeChanUsageEntry =
				(struct IE_CHANNEL_USAGE_ENTRY *)
				prIeChanUsage->aucEntries;
			DBGLOG(RLM, INFO,
				"op_class/op_channel=%u/%u is acceptable by peer\n",
				prIeChanUsageEntry->ucOpClass,
				prIeChanUsageEntry->ucChannel);
		} else {
			DBGLOG(RLM, INFO,
				"Channel is NOT acceptable by peer\n");
		}
	}
		break;

	default:
		break;
	}
}

void p2pRlmProcessWnmActionFrame(struct ADAPTER *prAdapter,
				 struct SW_RFB *prSwRfb)
{
	struct WLAN_ACTION_FRAME *prRxFrame;
	struct BSS_INFO *prBssInfo = NULL;

	prRxFrame = (struct WLAN_ACTION_FRAME *)prSwRfb->pvHeader;
	if (prSwRfb->prStaRec)
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prSwRfb->prStaRec->ucBssIndex);

	DBGLOG(RX, TRACE, "action=%u\n",
		prRxFrame->ucAction);

	switch (prRxFrame->ucAction) {
	case ACTION_WNM_CHANNEL_USAGE_REQ:
		if (prBssInfo)
			p2pRlmHandleChanUsageReqFrame(prAdapter, prSwRfb,
						      prBssInfo,
						      prSwRfb->prStaRec);
		break;

	case ACTION_WNM_CHANNEL_USAGE_RESP:
		if (prBssInfo)
			p2pRlmHandleChanUsageRespFrame(prAdapter, prSwRfb,
						       prBssInfo,
						       prSwRfb->prStaRec);
		break;

	default:
		DBGLOG(RX, INFO,
			"Unhandled wnm action frame %u from " MACSTR "\n",
			prRxFrame->ucAction,
			MAC2STR(prRxFrame->aucSrcAddr));
		break;
	}
}

void p2pRlmParseP2p2Ie(struct ADAPTER *prAdapter,
		       struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
		       const uint8_t *pucBuffer)
{
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;
	struct IE_P2P2 *prIeP2p2;
	uint32_t u4RemainIeLen = IE_SIZE(pucBuffer);
	uint8_t *pucPos;

	if (u4RemainIeLen < sizeof(*prIeP2p2))
		return;

	prIeP2p2 = (struct IE_P2P2 *)pucBuffer;
	u4RemainIeLen -= sizeof(*prIeP2p2);
	if (prIeP2p2->ucId != ELEM_ID_P2P ||
	    kalMemCmp(prIeP2p2->aucOui, aucWfaOui, sizeof(prIeP2p2->aucOui)))
		return;

	pucPos = (uint8_t *)prIeP2p2->aucAttrs;
	do {
		struct IE_P2P_ATTR *prP2pAttr;

		if (u4RemainIeLen < sizeof(struct IE_P2P_ATTR))
			break;

		prP2pAttr = (struct IE_P2P_ATTR *)pucPos;
		u4RemainIeLen -= sizeof(struct IE_P2P_ATTR);

		switch (prP2pAttr->ucId) {
		case P2P_ATTR_CAPABILITY_EXTENSION:
		{
			uint16_t u2CapInfo = 0;

			if (u4RemainIeLen < sizeof(u2CapInfo))
				break;

			kalMemCopy(&u2CapInfo, prP2pAttr->aucBody,
				   sizeof(u2CapInfo));

#if (CFG_P2P2_SUPPORT_GC_REQ_CSA == 1)
			prStaRec->fgCapGcCsaSupp =
				(u2CapInfo & P2P_PCEA_CLI_REQ_CS) ?
				TRUE : FALSE;
#endif /* CFG_P2P2_SUPPORT_GC_REQ_CSA */
		}
			break;
		default:
			break;
		}

		if (u4RemainIeLen < prP2pAttr->u2Length)
			break;
		u4RemainIeLen -= prP2pAttr->u2Length;
	} while (u4RemainIeLen);
}

uint32_t p2pRlmCalcP2p2IeLen(struct ADAPTER *prAdapter,
			     uint8_t ucBssIndex,
			     struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
							   ucBssIndex);

	if (!prBssInfo || !IS_BSS_GO(prAdapter, prBssInfo))
		return 0;

#if (CFG_P2P2_SUPPORT_GC_REQ_CSA == 1)
	return sizeof(struct IE_P2P2) +
		sizeof(struct IE_P2P_ATTR) + sizeof(uint16_t);
#else
	return 0;
#endif /* CFG_P2P2_SUPPORT_GC_REQ_CSA */
}

uint16_t p2pRlmGenP2p2Ie(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo)
{
#if (CFG_P2P2_SUPPORT_GC_REQ_CSA == 1)
	struct BSS_INFO *prBssInfo;
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;
	struct IE_P2P2 *prIeP2p2;
	struct IE_P2P_ATTR *prP2pAttr;
	uint16_t u2CapInfo = 0;
	uint8_t *pucBuffer, *pucPos;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  prMsduInfo->ucBssIndex);
	if (!prBssInfo || !IS_BSS_GO(prAdapter, prBssInfo))
		return 0;
	else if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgP2pGcCsaReq))
		return 0;

	u2CapInfo |= P2P_PCEA_CLI_REQ_CS;

	pucPos = pucBuffer =
		(uint8_t *)((uintptr_t)prMsduInfo->prPacket +
			    (uintptr_t)prMsduInfo->u2FrameLength);
	prIeP2p2 = (struct IE_P2P2 *)pucPos;

	prIeP2p2->ucId = ELEM_ID_P2P;
	/* let length to be filled later */
	kalMemCopy(prIeP2p2->aucOui, aucWfaOui, sizeof(prIeP2p2->aucOui));
	prIeP2p2->ucType = VENDOR_OUI_TYPE_P2P2;
	pucPos += sizeof(*prIeP2p2);

	u2CapInfo |= (sizeof(u2CapInfo) - 1) & P2P_PCEA_LEN_MASK;

	prP2pAttr = (struct IE_P2P_ATTR *)prIeP2p2->aucAttrs;
	prP2pAttr->ucId = P2P_ATTR_CAPABILITY_EXTENSION;
	/* let length to be filled later */
	kalMemCopy(prP2pAttr->aucBody, &u2CapInfo, sizeof(u2CapInfo));
	pucPos += sizeof(struct IE_P2P_ATTR) + sizeof(u2CapInfo);

	prP2pAttr->u2Length = (pucPos - (uint8_t *)prP2pAttr - 1 - 2);
	prIeP2p2->ucLength = (pucPos - pucBuffer - 1 - 1);

	DBGLOG(P2P, TRACE, "IE_P2P2");
	DBGLOG_MEM8(P2P, TRACE, prIeP2p2, IE_SIZE(prIeP2p2));

	return IE_SIZE(prIeP2p2);
#else
	return 0;
#endif /* CFG_P2P2_SUPPORT_GC_REQ_CSA */
}

uint32_t p2pRlmCalculateOweIeLen(struct ADAPTER *prAdapter,
				 uint8_t ucBssIndex,
				 struct STA_RECORD *prStaRec)
{
	struct WIFI_VAR *prWifiVar;
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo;
	const uint8_t *pucDHIEBuf;
	uint8_t ucRoleIdx;

	if (!prAdapter)
		return 0;

	prWifiVar = &prAdapter->rWifiVar;
	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prP2pBssInfo == NULL ||
	    prP2pBssInfo->u4RsnSelectedAKMSuite != RSN_AKM_SUITE_OWE)
		return 0;

	ucRoleIdx = prP2pBssInfo->u4PrivateData;
	if (ucRoleIdx >= BSS_P2P_NUM)
		return 0;

	prP2pSpecBssInfo = prWifiVar->prP2pSpecificBssInfo[ucRoleIdx];
	if (!prP2pSpecBssInfo->u2AssocIEBufLen)
		return 0;

	pucDHIEBuf =
		kalFindIeExtIE(ELEM_ID_RESERVED,
			       ELEM_EXT_ID_DIFFIE_HELLMAN_PARAM,
			       prP2pSpecBssInfo->pucAssocIEBuf,
			       prP2pSpecBssInfo->u2AssocIEBufLen);
	if (!pucDHIEBuf)
		return 0;

	return IE_SIZE(pucDHIEBuf);
}

void p2pRlmGenerateOweIe(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo)
{
	struct WIFI_VAR *prWifiVar;
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo;
	const uint8_t *pucDHIEBuf;
	uint8_t *pucBuf;
	uint8_t ucRoleIdx;

	if (!prAdapter || !prMsduInfo)
		return;

	prWifiVar = &prAdapter->rWifiVar;
	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					     prMsduInfo->ucBssIndex);
	if (prP2pBssInfo == NULL ||
	    prP2pBssInfo->u4RsnSelectedAKMSuite != RSN_AKM_SUITE_OWE)
		return;

	ucRoleIdx = prP2pBssInfo->u4PrivateData;
	if (ucRoleIdx >= BSS_P2P_NUM)
		return;

	prP2pSpecBssInfo = prWifiVar->prP2pSpecificBssInfo[ucRoleIdx];
	if (!prP2pSpecBssInfo->u2AssocIEBufLen)
		return;

	pucDHIEBuf =
		kalFindIeExtIE(ELEM_ID_RESERVED,
			       ELEM_EXT_ID_DIFFIE_HELLMAN_PARAM,
			       prP2pSpecBssInfo->pucAssocIEBuf,
			       prP2pSpecBssInfo->u2AssocIEBufLen);
	if (!pucDHIEBuf)
		return;

	pucBuf = (uint8_t *)((uintptr_t) prMsduInfo->prPacket +
			     (uint32_t) prMsduInfo->u2FrameLength);

	kalMemCopy(pucBuf, pucDHIEBuf, IE_SIZE(pucDHIEBuf));
	prMsduInfo->u2FrameLength += IE_SIZE(pucDHIEBuf);
}

uint32_t p2pRlmCalculateRsnIeLen(struct ADAPTER *prAdapter,
				 uint8_t ucBssIndex,
				 struct STA_RECORD *prStaRec)
{
	struct WIFI_VAR *prWifiVar;
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo;
	const uint8_t *pucRsnIE;
	uint8_t ucRoleIdx;

	if (!prAdapter)
		return 0;

	prWifiVar = &prAdapter->rWifiVar;
	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prP2pBssInfo == NULL ||
	    prP2pBssInfo->u4RsnSelectedAKMSuite != RSN_AKM_SUITE_OWE)
		return 0;

	ucRoleIdx = prP2pBssInfo->u4PrivateData;
	if (ucRoleIdx >= BSS_P2P_NUM)
		return 0;

	prP2pSpecBssInfo = prWifiVar->prP2pSpecificBssInfo[ucRoleIdx];
	if (!prP2pSpecBssInfo->u2AssocIEBufLen)
		return 0;

	pucRsnIE =
		kalFindIeMatchMask(ELEM_ID_RSN,
				   prP2pSpecBssInfo->pucAssocIEBuf,
				   prP2pSpecBssInfo->u2AssocIEBufLen,
				   NULL, 0, 0, NULL);
	if (!pucRsnIE)
		return 0;

	return IE_SIZE(pucRsnIE);
}

void p2pRlmGenerateRsnIe(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo)
{
	struct WIFI_VAR *prWifiVar;
	struct BSS_INFO *prP2pBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo;
	const uint8_t *pucRsnIE;
	uint8_t *pucBuf;
	uint8_t ucRoleIdx;

	if (!prAdapter || !prMsduInfo)
		return;

	prWifiVar = &prAdapter->rWifiVar;
	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					     prMsduInfo->ucBssIndex);
	if (prP2pBssInfo == NULL ||
	    prP2pBssInfo->u4RsnSelectedAKMSuite != RSN_AKM_SUITE_OWE)
		return;

	ucRoleIdx = prP2pBssInfo->u4PrivateData;
	if (ucRoleIdx >= BSS_P2P_NUM)
		return;

	prP2pSpecBssInfo = prWifiVar->prP2pSpecificBssInfo[ucRoleIdx];
	if (!prP2pSpecBssInfo->u2AssocIEBufLen)
		return;

	pucRsnIE =
		kalFindIeMatchMask(ELEM_ID_RSN,
				   prP2pSpecBssInfo->pucAssocIEBuf,
				   prP2pSpecBssInfo->u2AssocIEBufLen,
				   NULL, 0, 0, NULL);
	if (!pucRsnIE)
		return;

	pucBuf = (uint8_t *)((uintptr_t) prMsduInfo->prPacket +
			     (uint32_t) prMsduInfo->u2FrameLength);

	kalMemCopy(pucBuf, pucRsnIE, IE_SIZE(pucRsnIE));
	prMsduInfo->u2FrameLength += IE_SIZE(pucRsnIE);
}
#endif /* CFG_ENABLE_WIFI_DIRECT */
