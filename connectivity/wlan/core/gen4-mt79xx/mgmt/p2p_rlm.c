/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: @(#) p2p_rlm.c@@
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

void rlmBssUpdateChannelParams(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	uint8_t i;
	uint8_t ucMaxBw = 0;

	if (!prAdapter)
		return;
	if (!prBssInfo)
		return;

	/* Operation band, channel shall be ready before invoking this function.
	 * Bandwidth may be ready if other network is connected
	 */
	prBssInfo->fg40mBwAllowed = FALSE;
	prBssInfo->fgAssoc40mBwAllowed = FALSE;
	prBssInfo->eBssSCO = CHNL_EXT_SCN;
	prBssInfo->ucHtOpInfo1 = 0;

	/* Check if AP can set its bw to 40MHz
	 * But if any of BSS is setup in 40MHz,
	 * the second BSS would prefer to use 20MHz
	 * in order to remain in SCC case
	 */
	if (cnmBss40mBwPermitted(prAdapter, prBssInfo->ucBssIndex)) {

		if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
			prBssInfo->eBssSCO =
				rlmGetScoForAP(prAdapter, prBssInfo);
		else
			prBssInfo->eBssSCO =
				rlmGetSco(prAdapter, prBssInfo);

		if (prBssInfo->eBssSCO != CHNL_EXT_SCN) {
			prBssInfo->fg40mBwAllowed = TRUE;
			prBssInfo->fgAssoc40mBwAllowed = TRUE;

			prBssInfo->ucHtOpInfo1 = (uint8_t)
				(((uint32_t) prBssInfo->eBssSCO)
				| HT_OP_INFO1_STA_CHNL_WIDTH);
		}
	}

	/* Filled the VHT BW/S1/S2 and MCS rate set */
	if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_VHT) {
		for (i = 0; i < 8; i++)
			prBssInfo->u2VhtBasicMcsSet |= BITS(2 * i, (2 * i + 1));
		prBssInfo->u2VhtBasicMcsSet &=
			(VHT_CAP_INFO_MCS_MAP_MCS9
				<< VHT_CAP_INFO_MCS_1SS_OFFSET);
#if CFG_SUPPORT_DBDC
		ucMaxBw = cnmGetDbdcBwCapability(prAdapter,
			prBssInfo->ucBssIndex);
#else
		ucMaxBw = cnmGetBssMaxBw(prAdapter,
			prBssInfo->ucBssIndex);
#endif
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
	}

#if (CFG_SUPPORT_802_11AX == 1)
	/* Filled the HE Operation IE */
	if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_HE) {
		kalMemZero(prBssInfo->ucHeOpParams,
			HE_OP_BYTE_NUM * sizeof(uint8_t));

		/* Disable BSS color support*/
		prBssInfo->ucBssColorInfo |=
			BIT(HE_OP_BSSCOLOR_BSS_COLOR_DISABLE_SHFT);

		prBssInfo->ucBssColorInfo |=
			BIT(HE_OP_BSSCOLOR_BSS_COLOR_SHFT);

		prBssInfo->u2HeBasicMcsSet |= (HE_CAP_INFO_MCS_MAP_MCS7 << 0);
		for (i = 1; i < 8; i++)
			prBssInfo->u2HeBasicMcsSet |=
				(HE_CAP_INFO_MCS_NOT_SUPPORTED << 2 * i);
	} else {
		kalMemZero(prBssInfo->ucHeOpParams,
			HE_OP_BYTE_NUM * sizeof(uint8_t));
		prBssInfo->ucBssColorInfo = 0;
		prBssInfo->u2HeBasicMcsSet = 0;
	}
#endif

	/*ERROR HANDLE*/
	if ((prBssInfo->ucVhtChannelWidth >= VHT_OP_CHANNEL_WIDTH_80) &&
	    (prBssInfo->ucVhtChannelFrequencyS1 == 0)) {
		DBGLOG(RLM, INFO,
			"Wrong AP S1 parameter setting, back to BW20!!!\n");

		prBssInfo->ucVhtChannelWidth = VHT_OP_CHANNEL_WIDTH_20_40;
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
	if (!prAdapter)
		return;
	if (!prBssInfo)
		return;

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

	/* !RLM_NET_IS_BOW(prBssInfo) &&   FIXME. */
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

	if (ucLevel == CHNL_LEVEL0) {
		/* Forced to 20MHz,
		 * so extended channel is SCN and STA width is zero
		 */
		prBssInfo->fgObssActionForcedTo20M = TRUE;

		if (prBssInfo->ucHtOpInfo1 != (uint8_t) CHNL_EXT_SCN) {
			prBssInfo->ucHtOpInfo1 = (uint8_t) CHNL_EXT_SCN;
			fgBwChange = TRUE;
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
void rlmProcessPublicAction(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb)
{
	struct ACTION_20_40_COEXIST_FRAME *prRxFrame;
	struct IE_20_40_COEXIST *prCoexist;
	struct IE_INTOLERANT_CHNL_REPORT *prChnlReport;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucIE;
	uint16_t u2IELength, u2Offset;
	uint8_t i, j;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prRxFrame = (struct ACTION_20_40_COEXIST_FRAME *) prSwRfb->pvHeader;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);

	if (!(prSwRfb->prStaRec)) {
		return;
	}

	if (prRxFrame->ucAction != ACTION_PUBLIC_20_40_COEXIST
		|| !prStaRec
		|| prStaRec->ucStaState != STA_STATE_3
		|| prSwRfb->u2PacketLen < (WLAN_MAC_MGMT_HEADER_LEN + 5)
		|| prSwRfb->prStaRec->ucBssIndex !=
	    /* HIF_RX_HDR_GET_NETWORK_IDX(prSwRfb->prHifRxHdr) != */
	    prStaRec->ucBssIndex) {
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prStaRec->ucBssIndex);
	ASSERT(prBssInfo);

	if (!IS_BSS_ACTIVE(prBssInfo) ||
	    prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT
	    || prBssInfo->eBssSCO == CHNL_EXT_SCN) {
		return;
	}

	prCoexist = &prRxFrame->rBssCoexist;
	if (prCoexist->ucData
		& (BSS_COEXIST_40M_INTOLERANT
		| BSS_COEXIST_20M_REQ)) {

		ASSERT(prBssInfo->auc2G_20mReqChnlList[0]
			<= CHNL_LIST_SZ_2G);

		for (i = 1; i <= prBssInfo->auc2G_20mReqChnlList[0]
			&& i <= CHNL_LIST_SZ_2G; i++) {

			if (prBssInfo->auc2G_20mReqChnlList[i]
				== prBssInfo->ucPrimaryChannel)
				break;
		}
		if ((i > prBssInfo->auc2G_20mReqChnlList[0])
			&& (i <= CHNL_LIST_SZ_2G)) {
			prBssInfo->auc2G_20mReqChnlList[i] =
				prBssInfo->ucPrimaryChannel;
			prBssInfo->auc2G_20mReqChnlList[0]++;
		}
	}

	/* Process intolerant channel report IE */
	pucIE = (uint8_t *) &prRxFrame->rChnlReport;
	u2IELength = prSwRfb->u2PacketLen - (WLAN_MAC_MGMT_HEADER_LEN + 5);

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_20_40_INTOLERANT_CHNL_REPORT:
			prChnlReport =
				(struct IE_INTOLERANT_CHNL_REPORT *) pucIE;

			if (prChnlReport->ucLength <= 1)
				break;

			/* To do: process regulatory class.
			 * Now we assume 2.4G band
			 */

			for (j = 0; j < prChnlReport->ucLength - 1; j++) {
				/* Update non-HT channel list */
				ASSERT(prBssInfo->auc2G_NonHtChnlList[0]
					<= CHNL_LIST_SZ_2G);
				for (i = 1;
					i <= prBssInfo->auc2G_NonHtChnlList[0]
					&& i <= CHNL_LIST_SZ_2G; i++) {

					if (prBssInfo->auc2G_NonHtChnlList[i]
						==
						prChnlReport->aucChannelList[j])
						break;
				}
				if ((i > prBssInfo->auc2G_NonHtChnlList[0])
					&& (i <= CHNL_LIST_SZ_2G)) {
					prBssInfo->auc2G_NonHtChnlList[i] =
						prChnlReport->aucChannelList[j];
					prBssInfo->auc2G_NonHtChnlList[0]++;
				}
			}
			break;

		default:
			break;
		}
	}			/* end of IE_FOR_EACH */

	if (rlmUpdateBwByChListForAP(prAdapter, prBssInfo)) {
		bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);
		rlmSyncOperationParams(prAdapter, prBssInfo);
	}

	/* Check if OBSS scan exemption response should be sent */
	if (prCoexist->ucData & BSS_COEXIST_OBSS_SCAN_EXEMPTION_REQ)
		rlmObssScanExemptionRsp(prAdapter, prBssInfo, prSwRfb);
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
		< prAdapter->ucHwBssIdNum);

	prBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, prObssStatus->ucBssIndex);

	if (prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT)
		return;

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

	/* Check if Beacon content need to be updated */
	rlmUpdateParamsForAP(prAdapter, prBssInfo, TRUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief It is only for AP mode in NETWORK_TYPE_P2P_INDEX.
 *
 * \param[in]
 *
 * \return none
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
#if 0
/*----------------------------------------------------------------------------*/
/*!
 * \brief    Initial the channel list from the domain information.
 *           This function is called after P2P initial
 *           and Domain information changed.
 *           Make sure the device is disconnected
 *           while changing domain information.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return boolean value if probe response frame is
 */
/*----------------------------------------------------------------------------*/
void rlmFuncInitialChannelList(IN struct ADAPTER *prAdapter)
{
	struct P2P_CONNECTION_SETTINGS *prP2pConnSetting =
		(struct P2P_CONNECTION_SETTINGS *) NULL;
	struct DOMAIN_INFO_ENTRY *prDomainInfoEntry =
		(struct DOMAIN_INFO_ENTRY *) NULL;
	struct DOMAIN_SUBBAND_INFO *prDomainSubBand =
		(struct DOMAIN_SUBBAND_INFO *) NULL;
	struct CHANNEL_ENTRY_FIELD *prChannelEntryField =
		(struct CHANNEL_ENTRY_FIELD *) NULL;
	uint32_t u4Idx = 0, u4IdxII = 0;
	uint8_t ucBufferSize = P2P_MAX_SUPPORTED_CHANNEL_LIST_SIZE;
#if 0
	uint8_t ucSocialChnlSupport = 0, ucAutoChnl = 0;
#endif

	do {
		ASSERT_BREAK(prAdapter != NULL);

		prP2pConnSetting = prAdapter->rWifiVar.prP2PConnSettings;
#if 0
		ucAutoChnl = prP2pConnSetting->ucOperatingChnl;
#endif

		prDomainInfoEntry = rlmDomainGetDomainInfo(prAdapter);

		ASSERT_BREAK((prDomainInfoEntry != NULL)
			&& (prP2pConnSetting != NULL));

		prChannelEntryField =
			(struct CHANNEL_ENTRY_FIELD *)
				prP2pConnSetting->aucChannelEntriesField;

		for (u4Idx = 0; u4Idx < MAX_SUBBAND_NUM; u4Idx++) {
			prDomainSubBand = &prDomainInfoEntry->rSubBand[u4Idx];

			if (((prDomainSubBand->ucBand == BAND_5G)
				&& (!prAdapter->fgEnable5GBand))
			    || (prDomainSubBand->ucBand == BAND_NULL)) {
				continue;
			}

			if (ucBufferSize <
				(P2P_ATTRI_LEN_CHANNEL_ENTRY
				+ prDomainSubBand->ucNumChannels)) {
				/* Buffer is not enough
				 * to include all supported channels.
				 */
				break;	/* for */
			}

			prChannelEntryField->ucRegulatoryClass =
				prDomainSubBand->ucRegClass;
			prChannelEntryField->ucNumberOfChannels =
				prDomainSubBand->ucNumChannels;

			for (u4IdxII = 0;
				u4IdxII < prDomainSubBand->ucNumChannels;
				u4IdxII++) {
				prChannelEntryField
					->aucChannelList[u4IdxII] =
				    prDomainSubBand->ucFirstChannelNum
				    + (u4IdxII
				    * prDomainSubBand->ucChannelSpan);

#if 0
				switch (prChannelEntryField
					->aucChannelList[u4IdxII]) {
				case 1:
					ucSocialChnlSupport = 1;
					break;
				case 6:
					ucSocialChnlSupport = 6;
					break;
				case 11:
					ucSocialChnlSupport = 11;
					break;
				default:
					break;
				}

#endif
			}

			if (ucBufferSize >= (P2P_ATTRI_LEN_CHANNEL_ENTRY
				+ prChannelEntryField->ucNumberOfChannels))
				ucBufferSize -= (P2P_ATTRI_LEN_CHANNEL_ENTRY
				+ prChannelEntryField->ucNumberOfChannels);
			else
				break;

			prChannelEntryField =
				(struct CHANNEL_ENTRY_FIELD *)
				((unsigned long) prChannelEntryField +
				P2P_ATTRI_LEN_CHANNEL_ENTRY +
				(unsigned long)
				prChannelEntryField->ucNumberOfChannels);

		}

#if 0
		if (prP2pConnSetting->ucListenChnl == 0) {
			prP2pConnSetting->ucListenChnl =
				P2P_DEFAULT_LISTEN_CHANNEL;

			if (ucSocialChnlSupport != 0) {
				/* 1. User Not Set LISTEN channel.
				 * 2. Social channel is not empty.
				 */
				prP2pConnSetting->ucListenChnl =
					ucSocialChnlSupport;
			}
		}
#endif

		/* TODO: 20110921 frog - */
		/* If LISTEN channel is not set,
		 * a random supported channel would be set.
		 * If no social channel is supported,
		 * DEFAULT channel would be set.
		 */

		prP2pConnSetting->ucRfChannelListSize =
			P2P_MAX_SUPPORTED_CHANNEL_LIST_SIZE - ucBufferSize;

#if 0
		/* User not set OPERATE channel. */
		if (prP2pConnSetting->ucOperatingChnl == 0) {

			if (scnQuerySparseChannel(prAdapter, NULL, &ucAutoChnl))
				break;	/* while */

			ucBufferSize = prP2pConnSetting->ucRfChannelListSize;

			prChannelEntryField =
				(struct CHANNEL_ENTRY_FIELD *)
				prP2pConnSetting->aucChannelEntriesField;

			while (ucBufferSize != 0) {
				if (prChannelEntryField
					->ucNumberOfChannels != 0) {
					ucAutoChnl =
					prChannelEntryField->aucChannelList[0];
					break;	/* while */
				}

				else {
					prChannelEntryField =
						(struct CHANNEL_ENTRY_FIELD *)
						((uint32_t) prChannelEntryField
						+ P2P_ATTRI_LEN_CHANNEL_ENTRY
						+ (uint32_t)
						prChannelEntryField
						->ucNumberOfChannels);

					ucBufferSize -=
						(P2P_ATTRI_LEN_CHANNEL_ENTRY
						+
						prChannelEntryField
						->ucNumberOfChannels);
				}

			}

		}
#endif
		/* We assume user would not set a channel
		 * not in the channel list.
		 * If so, the operating channel still depends
		 * on target device supporting capability.
		 */

		/* TODO: 20110921 frog - */
		/* If the Operating channel is not set,
		 * a channel from supported channel list is set automatically.
		 * If there is no supported channel in channel list,
		 * a DEFAULT channel is set.
		 */

	} while (FALSE);

#if 0
	prP2pConnSetting->ucOperatingChnl = ucAutoChnl;
#endif
}				/* rlmFuncInitialChannelList */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    Find a common channel list from the local channel list info
 *           & target channel list info.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return boolean value if probe response frame is
 */
/*----------------------------------------------------------------------------*/
void
rlmFuncCommonChannelList(IN struct ADAPTER *prAdapter,
		IN struct CHANNEL_ENTRY_FIELD *prChannelEntryII,
		IN uint8_t ucChannelListSize)
{
	struct P2P_CONNECTION_SETTINGS *prP2pConnSetting =
		(struct P2P_CONNECTION_SETTINGS *) NULL;
	struct CHANNEL_ENTRY_FIELD *prChannelEntryI =
	    (struct CHANNEL_ENTRY_FIELD *) NULL,
	    prChannelEntryIII = (struct CHANNEL_ENTRY_FIELD *) NULL;
	uint8_t aucCommonChannelList[P2P_MAX_SUPPORTED_CHANNEL_LIST_SIZE];
	uint8_t ucOriChnlSize = 0, ucNewChnlSize = 0;

	do {

		ASSERT_BREAK(prAdapter != NULL);

		prP2pConnSetting = prAdapter->rWifiVar.prP2PConnSettings;

		prChannelEntryIII =
			(struct CHANNEL_ENTRY_FIELD *) aucCommonChannelList;

		while (ucChannelListSize > 0) {

			prChannelEntryI =
				(struct CHANNEL_ENTRY_FIELD *)
				prP2pConnSetting->aucChannelEntriesField;
			ucOriChnlSize = prP2pConnSetting->ucRfChannelListSize;

			while (ucOriChnlSize > 0) {
				if (prChannelEntryI->ucRegulatoryClass ==
					prChannelEntryII->ucRegulatoryClass) {

					prChannelEntryIII->ucRegulatoryClass =
					prChannelEntryI->ucRegulatoryClass;

					/* TODO: Currently we assume
					 * that the regulatory class the same,
					 * the channels are the same.
					 */
					kalMemCopy(
					prChannelEntryIII->aucChannelList,
					prChannelEntryII->aucChannelList,
					prChannelEntryII->ucNumberOfChannels);

					prChannelEntryIII->ucNumberOfChannels =
					prChannelEntryII->ucNumberOfChannels;

					ucNewChnlSize +=
				    P2P_ATTRI_LEN_CHANNEL_ENTRY +
				    prChannelEntryIII->ucNumberOfChannels;

					prChannelEntryIII =
						(struct CHANNEL_ENTRY_FIELD *)
						((unsigned long)
						prChannelEntryIII +
						P2P_ATTRI_LEN_CHANNEL_ENTRY +
						(unsigned long)
						prChannelEntryIII
						->ucNumberOfChannels);
				}

				ucOriChnlSize -= (P2P_ATTRI_LEN_CHANNEL_ENTRY
					+ prChannelEntryI->ucNumberOfChannels);

				prChannelEntryI =
					(struct CHANNEL_ENTRY_FIELD *)
					((unsigned long) prChannelEntryI +
					P2P_ATTRI_LEN_CHANNEL_ENTRY +
					(unsigned long)
					prChannelEntryI->ucNumberOfChannels);

			}

			ucChannelListSize -=
				(P2P_ATTRI_LEN_CHANNEL_ENTRY
				+ prChannelEntryII->ucNumberOfChannels);

			prChannelEntryII = (struct CHANNEL_ENTRY_FIELD *)
				((unsigned long) prChannelEntryII +
				P2P_ATTRI_LEN_CHANNEL_ENTRY +
				(unsigned long)
				prChannelEntryII->ucNumberOfChannels);

		}

		kalMemCopy(prP2pConnSetting->aucChannelEntriesField,
			aucCommonChannelList,
			ucNewChnlSize);
		prP2pConnSetting->ucRfChannelListSize = ucNewChnlSize;

	} while (FALSE);
}				/* rlmFuncCommonChannelList */

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint8_t rlmFuncFindOperatingClass(IN struct ADAPTER *prAdapter,
	IN uint8_t ucChannelNum)
{
	uint8_t ucRegulatoryClass = 0, ucBufferSize = 0;
	struct P2P_CONNECTION_SETTINGS *prP2pConnSetting =
		(struct P2P_CONNECTION_SETTINGS *) NULL;
	struct CHANNEL_ENTRY_FIELD *prChannelEntryField =
		(struct CHANNEL_ENTRY_FIELD *) NULL;
	uint32_t u4Idx = 0;

	do {
		ASSERT_BREAK(prAdapter != NULL);

		prP2pConnSetting = prAdapter->rWifiVar.prP2PConnSettings;
		ucBufferSize = prP2pConnSetting->ucRfChannelListSize;
		prChannelEntryField =
			(struct CHANNEL_ENTRY_FIELD *)
			prP2pConnSetting->aucChannelEntriesField;

		while (ucBufferSize != 0) {

			for (u4Idx = 0;
				u4Idx < prChannelEntryField->ucNumberOfChannels;
				u4Idx++) {
				if (prChannelEntryField->aucChannelList[u4Idx]
					== ucChannelNum) {
					ucRegulatoryClass =
						prChannelEntryField
						->ucRegulatoryClass;
					break;
				}

			}

			if (ucRegulatoryClass != 0)
				break;	/* while */

			prChannelEntryField =
				(struct CHANNEL_ENTRY_FIELD *)
				((unsigned long) prChannelEntryField +
				P2P_ATTRI_LEN_CHANNEL_ENTRY +
				(unsigned long)
				prChannelEntryField->ucNumberOfChannels);

			ucBufferSize -=
				(P2P_ATTRI_LEN_CHANNEL_ENTRY
				+ prChannelEntryField->ucNumberOfChannels);

		}

	} while (FALSE);

	return ucRegulatoryClass;
}				/* rlmFuncFindOperatingClass */

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
u_int8_t
rlmFuncFindAvailableChannel(IN struct ADAPTER *prAdapter,
		IN uint8_t ucCheckChnl,
		IN uint8_t *pucSuggestChannel,
		IN u_int8_t fgIsSocialChannel,
		IN u_int8_t fgIsDefaultChannel)
{
	u_int8_t fgIsResultAvailable = FALSE;
	struct CHANNEL_ENTRY_FIELD *prChannelEntry =
		(struct CHANNEL_ENTRY_FIELD *) NULL;
	struct P2P_CONNECTION_SETTINGS *prP2pConnSetting =
		(struct P2P_CONNECTION_SETTINGS *) NULL;
	uint8_t ucBufferSize = 0, ucIdx = 0, ucChannelSelected = 0;

	do {
		ASSERT_BREAK(prAdapter != NULL);

		if (fgIsDefaultChannel)
			ucChannelSelected = P2P_DEFAULT_LISTEN_CHANNEL;

		prP2pConnSetting = prAdapter->rWifiVar.prP2PConnSettings;
		ucBufferSize = prP2pConnSetting->ucRfChannelListSize;
		prChannelEntry =
			(struct CHANNEL_ENTRY_FIELD *)
			prP2pConnSetting->aucChannelEntriesField;

		while ((ucBufferSize != 0) && (!fgIsResultAvailable)) {

			for (ucIdx = 0;
				ucIdx < prChannelEntry->ucNumberOfChannels;
				ucIdx++) {

				if ((!fgIsSocialChannel) ||
				    (prChannelEntry->aucChannelList[ucIdx]
						== 1) ||
				    (prChannelEntry->aucChannelList[ucIdx]
						== 6) ||
				    (prChannelEntry->aucChannelList[ucIdx]
						== 11)) {

					if (prChannelEntry
						->aucChannelList[ucIdx] <= 11) {
						/* 2.4G. */
						ucChannelSelected =
							prChannelEntry
							->aucChannelList[ucIdx];
					} else if ((
						prChannelEntry
						->aucChannelList[ucIdx] < 52)
						&&
						(prChannelEntry
						->aucChannelList[ucIdx] > 14)) {

						/* 2.4G + 5G. */
						ucChannelSelected =
							prChannelEntry
							->aucChannelList[ucIdx];
					}

					if (ucChannelSelected == ucCheckChnl) {
						fgIsResultAvailable = TRUE;
						break;
					}
				}

			}

			ucBufferSize -=
				(P2P_ATTRI_LEN_CHANNEL_ENTRY
				+ prChannelEntry->ucNumberOfChannels);

			prChannelEntry =
				(struct CHANNEL_ENTRY_FIELD *)
				((unsigned long) prChannelEntry +
				P2P_ATTRI_LEN_CHANNEL_ENTRY +
				(unsigned long)
				prChannelEntry->ucNumberOfChannels);

		}

		if ((!fgIsResultAvailable)
			&& (pucSuggestChannel != NULL)) {
			log_dbg(P2P, TRACE,
			       "The request channel %d is not available, sugguested channel:%d\n",
			       ucCheckChnl, ucChannelSelected);

			/* Given a suggested channel. */
			*pucSuggestChannel = ucChannelSelected;
		}

	} while (FALSE);

	return fgIsResultAvailable;
}
#endif

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
		if (regd_is_single_sku_en()) {
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
		if (p2pFuncIsAPMode(prAdapter->rWifiVar
			.prP2PConnSettings[prBssInfo->u4PrivateData])) {
			if (prAdapter->rWifiVar.ucApSco == CHNL_EXT_SCA
				|| prAdapter->rWifiVar.ucApSco == CHNL_EXT_SCB)
				eTempSCO =
					(enum ENUM_CHNL_EXT)
						prAdapter->rWifiVar.ucApSco;
#if ((CFG_SUPPORT_TWT == 1) && (CFG_SUPPORT_TWT_HOTSPOT == 1))
			prBssInfo->twt_flow_id_bitmap = 0;
			prBssInfo->aeTWTRespState = 0;
			DlListInit(&prBssInfo->twt_sch_link);

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
		if (p2pFuncIsAPMode(prAdapter->rWifiVar
			.prP2PConnSettings[prBssInfo->u4PrivateData])) {
			if (prBssInfo->eBand == BAND_2G4)
				ucMaxBandwidth =
					prAdapter->rWifiVar.ucAp2gBandwidth;
			else if (prBssInfo->eBand == BAND_5G)
				ucMaxBandwidth =
					prAdapter->rWifiVar.ucAp5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (prBssInfo->eBand == BAND_6G)
				ucMaxBandwidth =
					prAdapter->rWifiVar.ucAp6gBandwidth;
#endif
		}
		/* P2P mode */
		else {
			if (prBssInfo->eBand == BAND_2G4)
				ucMaxBandwidth =
					prAdapter->rWifiVar.ucP2p2gBandwidth;
			else if (prBssInfo->eBand == BAND_5G)
				ucMaxBandwidth =
					prAdapter->rWifiVar.ucP2p5gBandwidth;
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (prBssInfo->eBand == BAND_6G)
				ucMaxBandwidth =
					prAdapter->rWifiVar.ucP2p6gBandwidth;
#endif
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
			 * A:CHNL_EXT_SCA,
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
	uint8_t ucPrimaryChannel = prBssInfo->ucPrimaryChannel;
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
	} else
		ucFreq1Channel =
			nicGetVhtS1(ucPrimaryChannel,
				prBssInfo->ucVhtChannelWidth);

	return ucFreq1Channel;
}

void rlmGetChnlInfoForCSA(struct ADAPTER *prAdapter,
	IN enum ENUM_BAND eBandCsa,
	IN uint8_t ucCh,
	IN uint8_t ucBssIdx,
	OUT struct RF_CHANNEL_INFO *prRfChnlInfo)
{
	struct BSS_INFO *prBssInfo = NULL;
	enum ENUM_BAND eBandOrig;
	uint8_t ucPrimaryChnlOrig;
	uint8_t ucCsaChnlS1 = 0;
	uint8_t fgDomainValid;

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	if (!prBssInfo)
		return;

	eBandOrig = prBssInfo->eBand;
	ucPrimaryChnlOrig = prBssInfo->ucPrimaryChannel;

	prRfChnlInfo->ucChannelNum = ucCh;
	prRfChnlInfo->eBand = eBandCsa;

	/* temp replace BSS eBand/channel
	* to get BW of CSA band
	*/
	prBssInfo->eBand = eBandCsa;
	prBssInfo->ucPrimaryChannel = ucCh;
	prRfChnlInfo->ucChannelNum = ucCh;
	prRfChnlInfo->ucChnlBw =
		cnmGetBssMaxBw(prAdapter, ucBssIdx);

	/* restore */
	prBssInfo->eBand = eBandOrig;
	prBssInfo->ucPrimaryChannel = ucPrimaryChnlOrig;

	prRfChnlInfo->u2PriChnlFreq =
		nicChannelNum2Freq(
				ucCh, eBandCsa) / 1000;
	ucCsaChnlS1 = nicGetVhtS1(ucCh,
			rlmGetVhtOpBwByBssOpBw(prRfChnlInfo->ucChnlBw));
	prRfChnlInfo->u4CenterFreq1 = (ucCsaChnlS1 != 0)
		? (nicChannelNum2Freq(
				ucCsaChnlS1, eBandCsa) / 1000) :
				prRfChnlInfo->u2PriChnlFreq;
	prRfChnlInfo->u4CenterFreq2 = 0;

	/* check domain info valid */
	fgDomainValid = rlmDomainIsValidRfSetting(
		prAdapter,
		eBandCsa,
		ucCh,
		rlmGetScoByChnInfo(prAdapter, prRfChnlInfo),
		(prRfChnlInfo->ucChnlBw == CW_20_40MHZ) ?
			CW_20_40MHZ :
			(prRfChnlInfo->ucChnlBw - 1),
		nicFreq2ChannelNum(
				prRfChnlInfo->u4CenterFreq1 * 1000),
		nicFreq2ChannelNum(
				prRfChnlInfo->u4CenterFreq2 * 1000));

	if (fgDomainValid == FALSE) {
		DBGLOG(P2P, WARN,
			"fgDomainValid: FALSE, set to default\n");
		/*Error Handling - Fixed to set 20MHz */
		prRfChnlInfo->ucChnlBw = CW_20_40MHZ;
		prRfChnlInfo->u4CenterFreq1 = 0;
		prRfChnlInfo->u4CenterFreq2 = 0;
	}
}
