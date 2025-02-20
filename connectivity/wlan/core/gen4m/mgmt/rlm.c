// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "rlm.c"
 *    \brief
 *
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#if (CFG_SUPPORT_BTWT == 1)
#include "twt_planner.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/* Retry limit of sending operation notification frame */
#define OPERATION_NOTICATION_TX_LIMIT	2
#define ENABLE_OMI BIT(0)
#define ENABLE_OMN BIT(1)
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
enum ENUM_OP_NOTIFY_STATE_T {
	OP_NOTIFY_STATE_KEEP = 0, /* Won't change OP mode */
	OP_NOTIFY_STATE_SENDING,  /* Sending OP notification frame */
	OP_NOTIFY_STATE_SUCCESS,  /* OP notification Tx success */
	OP_NOTIFY_STATE_FAIL,     /* OP notification Tx fail(over retry limit)*/
	OP_NOTIFY_STATE_ROLLBACK, /* OP notification rollback */
	OP_NOTIFY_STATE_NUM
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if (CFG_SUPPORT_802_11AX == 1)
uint8_t  g_fgHTSMPSEnabled = 0xFF;
#endif

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
#if CFG_SUPPORT_RXSMM_ALLOWLIST
uint8_t Rxsmm_Iot_Allowlist[]
	[VENDOR_OUI_RXSMM_OUI_IE_NUM] = {
	{0xF8, 0x32, 0xE4}
};
#endif

static const char * const apucOpBw[MAX_BW_UNKNOWN+1] = {
	[MAX_BW_20MHZ] = "MAX_BW_20MHZ",
	[MAX_BW_40MHZ] = "MAX_BW_40MHZ",
	[MAX_BW_80MHZ] = "MAX_BW_80MHZ",
	[MAX_BW_160MHZ] = "MAX_BW_160MHZ",
	[MAX_BW_80_80_MHZ] = "MAX_BW_80_80_MHZ",
	[MAX_BW_320_1MHZ] = "MAX_BW_320_1MHZ",
	[MAX_BW_320_2MHZ] = "MAX_BW_320_2MHZ",
	[MAX_BW_UNKNOWN] = "MAX_BW_UNKNOWN",
};

static const char * const apucVhtOpBw[CW_NUM+1] = {
	[CW_20_40MHZ] = "CW_20_40MHZ",
	[CW_80MHZ] = "CW_80MHZ",
	[CW_160MHZ] = "CW_160MHZ",
	[CW_80P80MHZ] = "CW_80P80MHZ",
	[CW_320_1MHZ] = "CW_320_1MHZ",
	[CW_320_2MHZ] = "CW_320_2MHZ",
};


/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void rlmFillHtCapIE(struct ADAPTER *prAdapter,
			   struct BSS_INFO *prBssInfo,
			   struct MSDU_INFO *prMsduInfo);

static void rlmFillExtCapIE(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo,
			    struct MSDU_INFO *prMsduInfo);

static void rlmFillHtOpIE(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo,
			  struct MSDU_INFO *prMsduInfo);

static uint8_t rlmRecIeInfoForClient(struct ADAPTER *prAdapter,
				     struct BSS_INFO *prBssInfo,
				     const uint8_t *pucIE, uint16_t u2IELength);

static u_int8_t rlmRecBcnFromNeighborForClient(struct ADAPTER *prAdapter,
					       struct BSS_INFO *prBssInfo,
					       struct SW_RFB *prSwRfb,
					       uint8_t *pucIE,
					       uint16_t u2IELength);

static u_int8_t rlmRecBcnInfoForClient(struct ADAPTER *prAdapter,
				       struct BSS_INFO *prBssInfo,
				       struct SW_RFB *prSwRfb, uint8_t *pucIE,
				       uint16_t u2IELength);

#if (CFG_SUPPORT_802_11AX == 1)
#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
u_int8_t updateHeBssColor(struct BSS_INFO *prBssInfo,
				       struct SW_RFB *prSwRfb,
					   u_int8_t ucBssColorInfo);
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */
#endif /* CFG_SUPPORT_802_11AX */

static void rlmBssReset(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo);

#if CFG_SUPPORT_802_11AC
static void rlmFillVhtCapIE(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo,
			    struct MSDU_INFO *prMsduInfo);
static void rlmFillVhtOpNotificationIE(struct ADAPTER *prAdapter,
				       struct BSS_INFO *prBssInfo,
				       struct MSDU_INFO *prMsduInfo,
				       u_int8_t fgIsMaxCap);

#endif

/* Operating BW/Nss change and notification */
static void rlmOpModeTxDoneHandler(struct ADAPTER *prAdapter,
				   struct MSDU_INFO *prMsduInfo,
				   uint8_t ucOpChangeType,
				   u_int8_t fgIsSuccess);
static void rlmApGoOmiOpModeDoneHandler(struct ADAPTER *prAdapter,
					struct MSDU_INFO *prMsduInfo);
static void rlmChangeOwnOpInfo(struct ADAPTER *prAdapter,
			       struct BSS_INFO *prBssInfo);
static void rlmCompleteOpModeChange(struct ADAPTER *prAdapter,
				    struct BSS_INFO *prBssInfo,
				    u_int8_t fgIsSuccess);
static void rlmRollbackOpChangeParam(struct BSS_INFO *prBssInfo,
				     u_int8_t fgIsRollbackBw,
				     u_int8_t fgIsRollbackNss);
static uint8_t rlmGetOpModeBwByVhtAndHtOpInfo(struct BSS_INFO *prBssInfo);
static u_int8_t rlmCheckOpChangeParamValid(struct ADAPTER *prAdapter,
					   struct BSS_INFO *prBssInfo,
					   uint8_t ucChannelWidth,
					   uint8_t ucOpRxNss,
					   uint8_t ucOpTxNss);

static void rlmUpdateParamsForCSA(struct ADAPTER *prAdapter,
			   struct BSS_INFO *prBssInfo);

static void rlmChangeOperationModeAfterCSA(
					struct ADAPTER *prAdapter,
					struct BSS_INFO *prBssInfo);

static void rlmRecOpModeBwForClient(uint8_t ucVhtOpModeChannelWidth,
				    struct BSS_INFO *prBssInfo);

static void rlmRecHtOpForClient(struct ADAPTER *prAdapter,
			struct IE_HT_OP *prHtOp,
			struct BSS_INFO *prBssInfo,
			uint8_t *pucPrimaryChannel);

#if CFG_SUPPORT_802_11K
static uint32_t rlmRegTxPwrLimitGet(struct ADAPTER *prAdapter,
					uint8_t ucBssIdx,
					int8_t *picPwrLmt);
#endif
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmFsmEventInit(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
	rlmDomainCheckCountryPowerLimitTable(prAdapter);
#endif

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
void rlmFsmEventUninit(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;

	ASSERT(prAdapter);

	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

		/* Note: all RLM timers will also be stopped.
		 *       Now only one OBSS scan timer.
		 */
		rlmBssReset(prAdapter, prBssInfo);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief For association request, power capability
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmReqGeneratePowerCapIE(struct ADAPTER *prAdapter,
			      struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucBuffer;
	int8_t icPwrMin = 0, icPwrMax = 0;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	pucBuffer =
		(uint8_t *)((uintptr_t)prMsduInfo->prPacket +
			prMsduInfo->u2FrameLength);

	if (prAdapter->rWifiVar.icRegPwrLmtMin < TX_PWR_MIN)
		icPwrMin = TX_PWR_MIN;
	else
		icPwrMin = prAdapter->rWifiVar.icRegPwrLmtMin;

	if (prAdapter->rWifiVar.icRegPwrLmtMax > TX_PWR_MAX)
		icPwrMax = TX_PWR_MAX;
	else
		icPwrMax = prAdapter->rWifiVar.icRegPwrLmtMax;

	POWER_CAP_IE(pucBuffer)->ucId = ELEM_ID_PWR_CAP;
	POWER_CAP_IE(pucBuffer)->ucLength = ELEM_MAX_LEN_POWER_CAP;
	POWER_CAP_IE(pucBuffer)->cMinTxPowerCap = icPwrMin;
	POWER_CAP_IE(pucBuffer)->cMaxTxPowerCap = icPwrMax;

	DBGLOG(RLM, INFO, "PwrCap Min[%d]Max[%d]\n"
			, POWER_CAP_IE(pucBuffer)->cMinTxPowerCap
			, POWER_CAP_IE(pucBuffer)->cMaxTxPowerCap);

	prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief For association request, supported channels
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmReqGenerateSupportedChIE(struct ADAPTER *prAdapter,
				 struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucBuffer;
	struct BSS_INFO *prBssInfo;
	struct RF_CHANNEL_INFO auc2gChannelList[MAX_2G_BAND_CHN_NUM];
	struct RF_CHANNEL_INFO auc5gChannelList[MAX_5G_BAND_CHN_NUM];
	uint8_t ucNumOf2gChannel = 0;
	uint8_t ucNumOf5gChannel = 0;
	uint8_t ucChIdx = 0;
	uint8_t ucIdx = 0;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	kalMemZero(&auc2gChannelList[0], sizeof(auc2gChannelList));
	kalMemZero(&auc5gChannelList[0], sizeof(auc5gChannelList));

	/* We should add supported channels IE in assoc/reassoc request
	 * if the spectrum management bit is set to 1 in Capability Info
	 * field, or the connection will be rejected by Marvell APs in
	 * some TGn items. (e.g. 5.2.3). Spectrum management related
	 * feature (802.11h) is for 5G band.
	 */
	if (!prBssInfo || prBssInfo->eBand != BAND_5G)
		return;

	pucBuffer =
		(uint8_t *)((uintptr_t)prMsduInfo->prPacket +
			prMsduInfo->u2FrameLength);

	rlmDomainGetChnlList(prAdapter, BAND_2G4, TRUE, MAX_2G_BAND_CHN_NUM,
			     &ucNumOf2gChannel, auc2gChannelList);
	rlmDomainGetChnlList(prAdapter, BAND_5G, TRUE, MAX_5G_BAND_CHN_NUM,
			     &ucNumOf5gChannel, auc5gChannelList);

	SUP_CH_IE(pucBuffer)->ucId = ELEM_ID_SUP_CHS;
	SUP_CH_IE(pucBuffer)->ucLength =
		(ucNumOf2gChannel + ucNumOf5gChannel) * 2;

	for (ucIdx = 0; ucIdx < ucNumOf2gChannel; ucIdx++, ucChIdx += 2) {
		SUP_CH_IE(pucBuffer)->ucChannelNum[ucChIdx] =
			auc2gChannelList[ucIdx].ucChannelNum;
		SUP_CH_IE(pucBuffer)->ucChannelNum[ucChIdx + 1] = 1;
	}

	for (ucIdx = 0; ucIdx < ucNumOf5gChannel; ucIdx++, ucChIdx += 2) {
		SUP_CH_IE(pucBuffer)->ucChannelNum[ucChIdx] =
			auc5gChannelList[ucIdx].ucChannelNum;
		SUP_CH_IE(pucBuffer)->ucChannelNum[ucChIdx + 1] = 1;
	}

	prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief For probe request, association request
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmReqGenerateHtCapIE(struct ADAPTER *prAdapter,
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

	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet &
	     PHY_TYPE_SET_802_11N) &&
	    (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)))
		rlmFillHtCapIE(prAdapter, prBssInfo, prMsduInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief For probe request, association request
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmReqGenerateExtCapIE(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
#if CFG_SUPPORT_PASSPOINT
	struct HS20_INFO *prHS20Info;
#endif

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

#if CFG_SUPPORT_PASSPOINT
	prHS20Info = aisGetHS20Info(prAdapter,
		prBssInfo->ucBssIndex);
	if (!prHS20Info)
		return;
#endif

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)
#if (CFG_SUPPORT_802_11AX == 1)
		|| (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11AX)
#endif
#if (CFG_SUPPORT_802_11BE == 1)
		|| (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11BE)
#endif
		)
		rlmFillExtCapIE(prAdapter, prBssInfo, prMsduInfo);
#if CFG_SUPPORT_PASSPOINT
	else if (prHS20Info->fgConnectHS20AP == TRUE)
		hs20FillExtCapIE(prAdapter, prBssInfo, prMsduInfo);
#endif /* CFG_SUPPORT_PASSPOINT */
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
void rlmRspGenerateHtCapIE(struct ADAPTER *prAdapter,
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

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (IS_BSS_APGO(prBssInfo) && prBssInfo->eBand == BAND_6G)
		return;
#endif

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	/* Decide PHY type set source */
	if (prStaRec) {
		/* Get PHY type set from target STA */
		ucPhyTypeSet = prStaRec->ucPhyTypeSet;
	} else {
		/* Get PHY type set from current BSS */
		ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
	}

	if (RLM_NET_IS_11N(prBssInfo) &&
	    (ucPhyTypeSet & PHY_TYPE_SET_802_11N) &&
		(!prBssInfo->fgIsWepCipherGroup))
		rlmFillHtCapIE(prAdapter, prBssInfo, prMsduInfo);
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
void rlmRspGenerateExtCapIE(struct ADAPTER *prAdapter,
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

	if (RLM_NET_IS_11N(prBssInfo) && ((ucPhyTypeSet & PHY_TYPE_SET_802_11N)
#if (CFG_SUPPORT_802_11AX == 1)
		|| (ucPhyTypeSet & PHY_TYPE_SET_802_11AX)
#endif
#if (CFG_SUPPORT_802_11BE == 1)
		|| (ucPhyTypeSet & PHY_TYPE_SET_802_11BE)
#endif
	))
		rlmFillExtCapIE(prAdapter, prBssInfo, prMsduInfo);
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
void rlmRspGenerateHtOpIE(struct ADAPTER *prAdapter,
			  struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucPhyTypeSet;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	if (!IS_BSS_ACTIVE(prBssInfo))
		return;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (IS_BSS_APGO(prBssInfo) && prBssInfo->eBand == BAND_6G)
		return;
#endif

	/* Decide PHY type set source */
	if (prStaRec) {
		/* Get PHY type set from target STA */
		ucPhyTypeSet = prStaRec->ucPhyTypeSet;
	} else {
		/* Get PHY type set from current BSS */
		ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
	}

	if (RLM_NET_IS_11N(prBssInfo) &&
	    (ucPhyTypeSet & PHY_TYPE_SET_802_11N) &&
		(!prBssInfo->fgIsWepCipherGroup))
		rlmFillHtOpIE(prAdapter, prBssInfo, prMsduInfo);
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
void rlmRspGenerateErpIE(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct IE_ERP *prErpIe;
	uint8_t ucPhyTypeSet;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	if (!IS_BSS_ACTIVE(prBssInfo))
		return;

	/* Decide PHY type set source */
	if (prStaRec) {
		/* Get PHY type set from target STA */
		ucPhyTypeSet = prStaRec->ucPhyTypeSet;
	} else {
		/* Get PHY type set from current BSS */
		ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
	}

	if (RLM_NET_IS_11GN(prBssInfo) && prBssInfo->eBand == BAND_2G4 &&
	    (ucPhyTypeSet & PHY_TYPE_SET_802_11GN)) {
		prErpIe = (struct IE_ERP *)(((uint8_t *)prMsduInfo->prPacket) +
					    prMsduInfo->u2FrameLength);

		/* Add ERP IE */
		prErpIe->ucId = ELEM_ID_ERP_INFO;
		prErpIe->ucLength = 1;

		prErpIe->ucERP = prBssInfo->fgObssErpProtectMode
					 ? ERP_INFO_USE_PROTECTION
					 : 0;

		if (prBssInfo->fgErpProtectMode)
			prErpIe->ucERP |= (ERP_INFO_NON_ERP_PRESENT |
					   ERP_INFO_USE_PROTECTION);

		/* Handle barker preamble */
		if (!prBssInfo->fgUseShortPreamble)
			prErpIe->ucERP |= ERP_INFO_BARKER_PREAMBLE_MODE;

		ASSERT(IE_SIZE(prErpIe) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_ERP));

		prMsduInfo->u2FrameLength += IE_SIZE(prErpIe);
	}
}

uint8_t rlmCheckMtkOuiChipCap(uint8_t *pucIe, uint64_t u8ChipCap)
{
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;

	if (IE_ID(pucIe) == ELEM_ID_VENDOR && IE_LEN(pucIe) > 7 &&
	    !kalMemCmp(MTK_OUI_IE(pucIe)->aucOui, aucMtkOui, 3) &&
	    (MTK_OUI_IE(pucIe)->aucCapability[0] &
			MTK_SYNERGY_CAP_SUPPORT_TLV)) {
		uint8_t *sub;
		uint16_t len, offset;

		sub = MTK_OUI_IE(pucIe)->aucInfoElem;
		len = IE_LEN(pucIe) - 7;

		IE_FOR_EACH(sub, len, offset) {
			if (IE_ID(sub) == MTK_OUI_ID_CHIP_CAP &&
			    IE_LEN(sub) == 8) {
				struct IE_MTK_CHIP_CAP *prCapIe =
					(struct IE_MTK_CHIP_CAP *)sub;

				DBGLOG(RLM, TRACE, "MTK_OUI_CHIP_CAP");
				DBGLOG_MEM8(RLM, TRACE, sub, IE_SIZE(sub));
				return prCapIe->u8ChipCap & u8ChipCap;
			}
		}
	}
	return 0;
}

#if CFG_SUPPORT_MTK_SYNERGY
uint32_t rlmCalculateMTKOuiIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec)
{
	uint8_t len = 0;

	len += ELEM_MIN_LEN_MTK_OUI;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	len += sizeof(struct IE_MTK_CHIP_CAP);
#if defined(CFG_SUPPORT_PRE_WIFI7)
	len += sizeof(struct IE_MTK_PRE_WIFI7);
	len += ehtRlmCalculateCapIELen(prAdapter, ucBssIndex, prStaRec);
	len += ehtRlmCalculateOpIELen(prAdapter, ucBssIndex, prStaRec);
	len += mldCalculateMlIELen(prAdapter, ucBssIndex, prStaRec);
#endif
#endif
#if ((CFG_SUPPORT_MLR_V2 == 1) || (CFG_SUPPORT_BALANCE_MLR == 1))
	len += sizeof(struct IE_MTK_MLR);
#endif
	return len;
}

uint16_t rlmGenerateMTKChipCapIE(uint8_t *pucBuf, uint16_t u2FrameLength,
	uint8_t fgNeedOui, uint64_t u8ChipCap)
{
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
	struct IE_MTK_CHIP_CAP *prChipCap;
	uint8_t *pos = pucBuf;
	uint16_t len = 0;

	if (fgNeedOui && ELEM_MIN_LEN_MTK_OUI + 2 < u2FrameLength) {
		kalMemSet(pucBuf, 0, ELEM_MIN_LEN_MTK_OUI + 2);
		MTK_OUI_IE(pucBuf)->ucId = ELEM_ID_VENDOR;
		MTK_OUI_IE(pucBuf)->ucLength =
			ELEM_MIN_LEN_MTK_OUI + sizeof(struct IE_MTK_CHIP_CAP);
		kalMemCopy(MTK_OUI_IE(pucBuf)->aucOui, aucMtkOui, 3);
		/* add icv sub ie for mlo device */
		MTK_OUI_IE(pucBuf)->aucCapability[0] |=
			MTK_SYNERGY_CAP_SUPPORT_TLV;
		pos = MTK_OUI_IE(pucBuf)->aucInfoElem;
		len += pos - pucBuf;
	}

	if (len + sizeof(struct IE_MTK_CHIP_CAP) < u2FrameLength) {
		kalMemSet(pos, 0, sizeof(struct IE_MTK_CHIP_CAP));
		prChipCap = (struct IE_MTK_CHIP_CAP *) pos;
		prChipCap->ucId = MTK_OUI_ID_CHIP_CAP;
		prChipCap->ucLength = sizeof(struct IE_MTK_CHIP_CAP) - 2;
		prChipCap->u8ChipCap = u8ChipCap;
		len += sizeof(struct IE_MTK_CHIP_CAP);
	}

	DBGLOG(RLM, TRACE, "MTK_OUI_CHIP_CAP");
	DBGLOG_MEM8(RLM, TRACE, pucBuf, len);

	return len;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is used to generate MTK Vendor Specific OUI
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmGenerateMTKOuiIE(struct ADAPTER *prAdapter,
	 struct MSDU_INFO *prMsduInfo)
{
	struct WLAN_MAC_MGMT_HEADER *mgmt;
	uint16_t frame_ctrl;
	struct BSS_INFO *prBssInfo;
#if (CFG_SUPPORT_MLR == 1)
	struct STA_RECORD *prStaRec;
	u_int8_t fgMlrBandCheck = FALSE;
	u_int8_t fgMlrCapCheck = FALSE;
#endif
	uint8_t *pucBuffer;
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
	uint16_t len;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	if (prAdapter->rWifiVar.ucMtkOui == FEATURE_DISABLED)
		return;

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	mgmt = (struct WLAN_MAC_MGMT_HEADER *)(prMsduInfo->prPacket);
	frame_ctrl = mgmt->u2FrameCtrl & MASK_FRAME_TYPE;
	pucBuffer = (uint8_t *)((uintptr_t)prMsduInfo->prPacket +
				(uintptr_t)prMsduInfo->u2FrameLength);

	MTK_OUI_IE(pucBuffer)->ucId = ELEM_ID_VENDOR;
	MTK_OUI_IE(pucBuffer)->ucLength = ELEM_MIN_LEN_MTK_OUI;
	MTK_OUI_IE(pucBuffer)->aucOui[0] = aucMtkOui[0];
	MTK_OUI_IE(pucBuffer)->aucOui[1] = aucMtkOui[1];
	MTK_OUI_IE(pucBuffer)->aucOui[2] = aucMtkOui[2];
	MTK_OUI_IE(pucBuffer)->aucCapability[0] =
		MTK_SYNERGY_CAP0 & (prAdapter->rWifiVar.aucMtkFeature[0]);
	MTK_OUI_IE(pucBuffer)->aucCapability[1] =
		MTK_SYNERGY_CAP1 & (prAdapter->rWifiVar.aucMtkFeature[1]);
	MTK_OUI_IE(pucBuffer)->aucCapability[2] =
		MTK_SYNERGY_CAP2 & (prAdapter->rWifiVar.aucMtkFeature[2]);
	MTK_OUI_IE(pucBuffer)->aucCapability[3] =
		MTK_SYNERGY_CAP3 & (prAdapter->rWifiVar.aucMtkFeature[3]);

	/* Disable the 2.4G 256QAM feature bit if chip doesn't support AC*/
	if (prAdapter->rWifiVar.ucHwNotSupportAC) {
		MTK_OUI_IE(pucBuffer)->aucCapability[0] &=
			~(MTK_SYNERGY_CAP_SUPPORT_24G_MCS89);
		DBGLOG(INIT, WARN,
		       "Disable 2.4G 256QAM support if N only chip\n");
	}

#if (CFG_SUPPORT_TWT_HOTSPOT_AC == 1)
	if (IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucTWTHotSpotSupport)) {
		MTK_OUI_IE(pucBuffer)->aucCapability[0] |=
			MTK_SYNERGY_CAP_SUPPORT_TWT_HOTSPOT_AC;

		DBGLOG(INIT, WARN, "TWT hotspot supported\n");
	}
#endif

#if CFG_ENABLE_WIFI_DIRECT
	if (IS_BSS_APGO(prBssInfo) &&
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgP2pGcCsa)) {
		MTK_OUI_IE(pucBuffer)->aucCapability[1] |=
			MTK_SYNERGY_CAP_SUPPORT_GC_CSA;
		DBGLOG(P2P, TRACE, "Add gc csa capa\n");
	}
#endif
	len = IE_SIZE(pucBuffer);
	prMsduInfo->u2FrameLength += len;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	/* AP/GO skip mtk cap and pre-wifi 7 IE if eht unsupported */
	if (!IS_BSS_APGO(prBssInfo) ||
	    (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_EHT)) {
		/* add icv sub ie for mlo device */
		MTK_OUI_IE(pucBuffer)->aucCapability[0] |=
			MTK_SYNERGY_CAP_SUPPORT_TLV;
		len = rlmGenerateMTKChipCapIE(
			MTK_OUI_IE(pucBuffer)->aucInfoElem,
			255, FALSE, MTK_OUI_CHIP_CAP);
		MTK_OUI_IE(pucBuffer)->ucLength += len;
		prMsduInfo->u2FrameLength += len;
	}

	/* hide eht & ml ie in vendor ie */
	if ((prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_EHT) &&
	    prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_HIDE_INFO &&
	   (frame_ctrl == MAC_FRAME_PROBE_RSP ||
	    frame_ctrl == MAC_FRAME_BEACON)) {
		struct IE_MTK_PRE_WIFI7 *prPreWifi7;

		prPreWifi7 = (struct IE_MTK_PRE_WIFI7 *)
			(pucBuffer + IE_SIZE(pucBuffer));
		len = prMsduInfo->u2FrameLength;

		prPreWifi7->ucId = MTK_OUI_ID_PRE_WIFI7;
		prPreWifi7->ucLength = 0;
		prPreWifi7->ucVersion0 = 0;
		prPreWifi7->ucVersion1 = 2;

		prMsduInfo->u2FrameLength += sizeof(struct IE_MTK_PRE_WIFI7);
		mldGenerateMlIEImpl(prAdapter, prMsduInfo);
		ehtRlmRspGenerateCapIEImpl(prAdapter, prMsduInfo);
		ehtRlmRspGenerateOpIEImpl(prAdapter, prMsduInfo);

		prPreWifi7->ucLength = (prMsduInfo->u2FrameLength - len - 2);
		MTK_OUI_IE(pucBuffer)->ucLength += IE_SIZE(prPreWifi7);

		DBGLOG(RLM, TRACE, "MTK_OUI_PRE_WIFI7");
		DBGLOG_MEM8(RLM, TRACE, pucBuffer, IE_SIZE(pucBuffer));
	}
#endif

#if (CFG_SUPPORT_MLR == 1)
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	if (prStaRec) {
		if (MLR_IS_V1_AFTER_INTERSECT(prAdapter, prStaRec)
			&& MLR_BAND_IS_SUPPORT(prBssInfo->eBand)) {
			/* MLRv1 doesn't need to gen MTK OUI - MLRIE */
			fgMlrCapCheck = FALSE;
			fgMlrBandCheck = TRUE;
		} else if ((MLR_IS_MLRP_AFTER_INTERSECT(prAdapter, prStaRec)
			|| MLR_IS_ALR_AFTER_INTERSECT(prAdapter, prStaRec))
			&& MLR_BAND_IS_SUPPORT(prBssInfo->eBand)) {
			fgMlrCapCheck = TRUE;
			fgMlrBandCheck = TRUE;
		} else if (MLR_IS_V2_AFTER_INTERSECT(prAdapter, prStaRec)
			|| MLR_IS_V1V2_AFTER_INTERSECT(prAdapter, prStaRec)) {
			fgMlrCapCheck = TRUE;
			fgMlrBandCheck = TRUE;
		}
	} else
		DBGLOG(RLM, WARN,
			"MLR assoc - gen MTK OUI - MLRIE but prStaRec is NULL");

	if (fgMlrCapCheck && fgMlrBandCheck &&
		(FALSE
#if (CFG_SUPPORT_BALANCE_MLR == 1)
		|| frame_ctrl == MAC_FRAME_BEACON
		|| frame_ctrl == MAC_FRAME_PROBE_RSP
		|| frame_ctrl == MAC_FRAME_ASSOC_RSP
		|| frame_ctrl == MAC_FRAME_REASSOC_RSP
#endif
#if ((CFG_SUPPORT_MLR_V2 == 1) || (CFG_SUPPORT_BALANCE_MLR == 1))
		|| frame_ctrl == MAC_FRAME_REASSOC_REQ
		|| frame_ctrl == MAC_FRAME_ASSOC_REQ
#endif
	)) {
		u_int8_t fgGenMlrIe = FALSE;
#if (CFG_SUPPORT_MLR_V2 == 1)
		if (IS_BSS_AIS(prBssInfo)
			&& prBssInfo->eCurrentOPMode
			== OP_MODE_INFRASTRUCTURE)
			fgGenMlrIe = TRUE;
#endif
#if (CFG_SUPPORT_BALANCE_MLR == 1)
		fgGenMlrIe = TRUE;
#endif
		if (fgGenMlrIe) {
			MTK_OUI_IE(pucBuffer)->aucCapability[0] |=
				MTK_SYNERGY_CAP_SUPPORT_TLV;

			len = mlrGenerateMlrIEforMTKOuiIE(prAdapter,
				prMsduInfo,
				pucBuffer + IE_SIZE(pucBuffer));
			MTK_OUI_IE(pucBuffer)->ucLength += len;
			prMsduInfo->u2FrameLength += len;
		}
	}
#endif

} /* rlmGenerateMTKOuiIE */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to check MTK Vendor Specific OUI
 *
 *
 * @return true:  correct MTK OUI
 *             false: incorrect MTK OUI
 */
/*----------------------------------------------------------------------------*/
u_int8_t rlmParseCheckMTKOuiIE(struct ADAPTER *prAdapter, const uint8_t *pucBuf,
			      struct STA_RECORD *prStaRec)
{
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
	struct IE_MTK_OUI *prMtkOuiIE = (struct IE_MTK_OUI *)NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (pucBuf != NULL));

		prMtkOuiIE = (struct IE_MTK_OUI *)pucBuf;

		if (prAdapter->rWifiVar.ucMtkOui == FEATURE_DISABLED)
			break;
		else if (IE_LEN(pucBuf) < ELEM_MIN_LEN_MTK_OUI)
			break;
		else if (prMtkOuiIE->aucOui[0] != aucMtkOui[0] ||
			 prMtkOuiIE->aucOui[1] != aucMtkOui[1] ||
			 prMtkOuiIE->aucOui[2] != aucMtkOui[2])
			break;
		/* apply NvRam setting */
		prMtkOuiIE->aucCapability[0] =
			prMtkOuiIE->aucCapability[0] &
			(prAdapter->rWifiVar.aucMtkFeature[0]);
		prMtkOuiIE->aucCapability[1] =
			prMtkOuiIE->aucCapability[1] &
			(prAdapter->rWifiVar.aucMtkFeature[1]);
		prMtkOuiIE->aucCapability[2] =
			prMtkOuiIE->aucCapability[2] &
			(prAdapter->rWifiVar.aucMtkFeature[2]);
		prMtkOuiIE->aucCapability[3] =
			prMtkOuiIE->aucCapability[3] &
			(prAdapter->rWifiVar.aucMtkFeature[3]);

		/* Disable the 2.4G 256QAM feature bit if chip doesn't support
		 * AC. Otherwise, FW would choose wrong max rate of auto rate.
		 */
		if (prAdapter->rWifiVar.ucHwNotSupportAC) {
			prMtkOuiIE->aucCapability[0] &=
				~(MTK_SYNERGY_CAP_SUPPORT_24G_MCS89);
			DBGLOG(INIT, WARN,
			       "Disable 2.4G 256QAM support if N only chip\n");
		}

		kalMemCopy(&prStaRec->u4Flags, prMtkOuiIE->aucCapability,
			sizeof(prStaRec->u4Flags));

		return TRUE;
	} while (FALSE);

	return FALSE;
} /* rlmParseCheckMTKOuiIE */

#endif

#if CFG_SUPPORT_RXSMM_ALLOWLIST
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to check MTK Vendor Specific OUI
 *
 *
 * @return true:  correct MTK OUI
 *             false: incorrect MTK OUI
 */
/*----------------------------------------------------------------------------*/
u_int8_t rlmParseCheckRxsmmOuiIE(struct ADAPTER *prAdapter,
		const uint8_t *pucBuf, u_int8_t *pfgRxsmmEnable)
{
	uint8_t u1RxsmmListIdx = 0;
	struct IE_MTK_OUI *prMtkOuiIE = (struct IE_MTK_OUI *)NULL;

	if ((prAdapter == NULL) || (pucBuf == NULL))
		return FALSE;

	*pfgRxsmmEnable = FALSE;

#if CFG_SUPPORT_MTK_SYNERGY
	for (u1RxsmmListIdx = 0;
		u1RxsmmListIdx < ARRAY_SIZE(Rxsmm_Iot_Allowlist);
		u1RxsmmListIdx++) {

		prMtkOuiIE = (struct IE_MTK_OUI *)pucBuf;

		if (prAdapter->rWifiVar.ucMtkOui == FEATURE_DISABLED)
			continue;
		else if (IE_LEN(pucBuf) < ELEM_MIN_LEN_MTK_OUI)
			continue;
		else if (prMtkOuiIE->aucOui[0] !=
			Rxsmm_Iot_Allowlist[u1RxsmmListIdx][0] ||
			 prMtkOuiIE->aucOui[1] !=
			 Rxsmm_Iot_Allowlist[u1RxsmmListIdx][1] ||
			 prMtkOuiIE->aucOui[2] !=
			 Rxsmm_Iot_Allowlist[u1RxsmmListIdx][2])
			continue;

		*pfgRxsmmEnable = TRUE;

	}
#endif

	DBGLOG(QM, TRACE, "RxSMM: OUI enable = %d\n", *pfgRxsmmEnable);

	return TRUE;
} /* rlmParseCheckRxsmmOuiIE */
#endif

#if CFG_ENABLE_WIFI_DIRECT
uint32_t rlmCalculateCsaIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec)
{
	uint32_t u4Length;

	u4Length = ELEM_HDR_LEN + sizeof(struct IE_CHANNEL_SWITCH);

	switch (prAdapter->rWifiVar.ucNewChannelWidth) {
	case VHT_OP_CHANNEL_WIDTH_20_40:
		if (prAdapter->rWifiVar.ucSecondaryOffset == CHNL_EXT_SCN ||
		    prAdapter->rWifiVar.ucSecondaryOffset == CHNL_EXT_RES)
			break;
		u4Length += sizeof(struct IE_SECONDARY_OFFSET);
		break;
	case VHT_OP_CHANNEL_WIDTH_80:
	case VHT_OP_CHANNEL_WIDTH_160:
	case VHT_OP_CHANNEL_WIDTH_80P80:
		u4Length += sizeof(struct IE_CHANNEL_SWITCH_WRAPPER);
		u4Length += sizeof(struct IE_WIDE_BAND_CHANNEL);
		break;
	default:
		break;
	}

	u4Length += sizeof(struct IE_EX_CHANNEL_SWITCH);

	return u4Length;
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
void rlmGenerateCsaIE(struct ADAPTER *prAdapter, struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucBuffer;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	if (!prAdapter->rWifiVar.fgCsaInProgress)
		return;

	if (prMsduInfo->ucBssIndex != p2pFuncGetCsaBssIndex())
		return;

	pucBuffer =
		(uint8_t *)((uintptr_t)prMsduInfo->prPacket +
			    (uintptr_t)prMsduInfo->u2FrameLength);

	/* Fill Channel Switch Announcement IE */
	CSA_IE(pucBuffer)->ucId = ELEM_ID_CH_SW_ANNOUNCEMENT;
	CSA_IE(pucBuffer)->ucLength = 3;
	CSA_IE(pucBuffer)->ucChannelSwitchMode =
		prAdapter->rWifiVar.ucChannelSwitchMode;
	CSA_IE(pucBuffer)->ucNewChannelNum =
		prAdapter->rWifiVar.ucNewChannelNumber;
	CSA_IE(pucBuffer)->ucChannelSwitchCount =
		prAdapter->rWifiVar.ucChannelSwitchCount;

	prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);

	switch (prAdapter->rWifiVar.ucNewChannelWidth) {
	case VHT_OP_CHANNEL_WIDTH_20_40:
		if (prAdapter->rWifiVar.ucSecondaryOffset == CHNL_EXT_SCN ||
		    prAdapter->rWifiVar.ucSecondaryOffset == CHNL_EXT_RES)
			break;
		pucBuffer += IE_SIZE(pucBuffer);

		SEC_OFFSET_IE(pucBuffer)->ucId = ELEM_ID_SCO;
		SEC_OFFSET_IE(pucBuffer)->ucLength = 1;
		SEC_OFFSET_IE(pucBuffer)->ucSecondaryOffset =
			prAdapter->rWifiVar.ucSecondaryOffset;

		prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
		break;
	case VHT_OP_CHANNEL_WIDTH_80:
	case VHT_OP_CHANNEL_WIDTH_160:
	case VHT_OP_CHANNEL_WIDTH_80P80:
		pucBuffer += IE_SIZE(pucBuffer);

		CSA_WRAPPER_IE(pucBuffer)->ucId = ELEM_ID_CH_SW_WRAPPER;
		CSA_WRAPPER_IE(pucBuffer)->ucLength = 5;

		prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);

		pucBuffer += 2;

		WIDE_BW_IE(pucBuffer)->ucId = ELEM_ID_WIDE_BAND_CHANNEL_SWITCH;
		WIDE_BW_IE(pucBuffer)->ucLength = 3;
		WIDE_BW_IE(pucBuffer)->ucNewChannelWidth =
			prAdapter->rWifiVar.ucNewChannelWidth;
		WIDE_BW_IE(pucBuffer)->ucChannelS1 =
			prAdapter->rWifiVar.ucNewChannelS1;
		WIDE_BW_IE(pucBuffer)->ucChannelS2 =
			prAdapter->rWifiVar.ucNewChannelS2;

		break;
	default:
		break;
	}

	/* Fill Extended Channel Switch Announcement IE */
	pucBuffer += IE_SIZE(pucBuffer);

	EX_CSA_IE(pucBuffer)->ucId = ELEM_ID_EX_CH_SW_ANNOUNCEMENT;
	EX_CSA_IE(pucBuffer)->ucLength = 4;
	EX_CSA_IE(pucBuffer)->ucChannelSwitchMode =
		prAdapter->rWifiVar.ucChannelSwitchMode;
	EX_CSA_IE(pucBuffer)->ucNewOperatingClass =
		prAdapter->rWifiVar.ucNewOperatingClass;
	EX_CSA_IE(pucBuffer)->ucNewChannelNum =
		prAdapter->rWifiVar.ucNewChannelNumber;
	EX_CSA_IE(pucBuffer)->ucChannelSwitchCount =
		prAdapter->rWifiVar.ucChannelSwitchCount;

	prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
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
void rlmProcessPublicAction2040Coexist(struct ADAPTER *prAdapter,
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

	if (!(prSwRfb->prStaRec))
		return;

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
static void rlmFillHtCapIE(struct ADAPTER *prAdapter,
			   struct BSS_INFO *prBssInfo,
			   struct MSDU_INFO *prMsduInfo)
{
	struct IE_HT_CAP *prHtCap;
	struct SUP_MCS_SET_FIELD *prSupMcsSet;
	u_int8_t fg40mAllowed;
	uint8_t ucIdx;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	if (IS_BSS_APGO(prBssInfo))
		fg40mAllowed = prBssInfo->fgAssoc40mBwAllowed;
	else
		fg40mAllowed = cnmGetBssMaxBw(prAdapter,
			prBssInfo->ucBssIndex) >= MAX_BW_40MHZ;

	prHtCap = (struct IE_HT_CAP *)(((uint8_t *)prMsduInfo->prPacket) +
				       prMsduInfo->u2FrameLength);

	/* Add HT capabilities IE */
	prHtCap->ucId = ELEM_ID_HT_CAP;
	prHtCap->ucLength = sizeof(struct IE_HT_CAP) - ELEM_HDR_LEN;

	prHtCap->u2HtCapInfo = HT_CAP_INFO_DEFAULT_VAL;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxShortGI))
		prHtCap->u2HtCapInfo |=
			(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M);

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxLdpc))
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_LDPC_CAP;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucTxStbc) &&
			prBssInfo->ucOpTxNss >= 2)
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_TX_STBC;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxStbc)) {

		uint8_t tempRxStbcNss;

		tempRxStbcNss = prAdapter->rWifiVar.ucRxStbcNss;
		tempRxStbcNss =
			(tempRxStbcNss >
			 wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex))
				? wlanGetSupportNss(prAdapter,
						    prBssInfo->ucBssIndex)
				: (tempRxStbcNss);
		if (tempRxStbcNss != prAdapter->rWifiVar.ucRxStbcNss) {
			DBGLOG(RLM, WARN, "Apply Nss:%d as RxStbcNss in HT Cap",
			       wlanGetSupportNss(prAdapter,
						 prBssInfo->ucBssIndex));
			DBGLOG(RLM, WARN,
			       " due to set RxStbcNss more than Nss is not appropriate.\n");
		}
		if (tempRxStbcNss == 1)
			prHtCap->u2HtCapInfo |= HT_CAP_INFO_RX_STBC_1_SS;
		else if (tempRxStbcNss == 2)
			prHtCap->u2HtCapInfo |= HT_CAP_INFO_RX_STBC_2_SS;
		else if (tempRxStbcNss >= 3)
			prHtCap->u2HtCapInfo |= HT_CAP_INFO_RX_STBC_3_SS;
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxGf))
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_HT_GF;

	if (prAdapter->rWifiVar.ucRxMaxMpduLen > VHT_CAP_INFO_MAX_MPDU_LEN_3K)
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_MAX_AMSDU_LEN;

	if (!fg40mAllowed)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
					  HT_CAP_INFO_SHORT_GI_40M |
					  HT_CAP_INFO_DSSS_CCK_IN_40M);

	/* SM power saving (IEEE 802.11, 2016, 10.2.4)
	 * A non-AP HT STA may also use SM Power Save bits in the HT
	 * Capabilities element of its Association Request to achieve
	 * the same purpose. The latter allows the STA to use only a
	 * single receive chain immediately after association.
	 */
	if (prBssInfo->ucOpRxNss <
		wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex))
		prHtCap->u2HtCapInfo &=
			~HT_CAP_INFO_SM_POWER_SAVE; /*Set as static power save*/

#if (CFG_SUPPORT_802_11AX == 1)
	if ((g_ucHtSMPSCapValue == 5) && (g_fgHTSMPSEnabled == 0xFF)) {
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_SM_POWER_SAVE;
	} else if ((g_ucHtSMPSCapValue == 5) && (g_fgHTSMPSEnabled == 1)) {
		prHtCap->u2HtCapInfo &=
			(~HT_CAP_INFO_SM_POWER_SAVE);
		(prHtCap->u2HtCapInfo) |=
				(1 << HT_CAP_INFO_SM_POWER_SAVE_OFFSET);
	} else if ((g_ucHtSMPSCapValue == 3) && (g_fgSigmaCMDHt == 1)) {
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_SM_POWER_SAVE;
	} else if ((g_ucHtSMPSCapValue == 1) && (g_fgSigmaCMDHt == 1)) {
		prHtCap->u2HtCapInfo &=
			(~HT_CAP_INFO_SM_POWER_SAVE);
		(prHtCap->u2HtCapInfo) |=
				(1 << HT_CAP_INFO_SM_POWER_SAVE_OFFSET);
	}
#endif

	if (prBssInfo->eBand == BAND_2G4 &&
	    prAdapter->rWifiVar.ucHtSmps2g4 == 1) {
		prHtCap->u2HtCapInfo &=
			(~HT_CAP_INFO_SM_POWER_SAVE);
		(prHtCap->u2HtCapInfo) |=
			(1 << HT_CAP_INFO_SM_POWER_SAVE_OFFSET);
	} else if (prBssInfo->eBand == BAND_2G4 &&
		   prAdapter->rWifiVar.ucHtSmps2g4 == 3) {
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_SM_POWER_SAVE;
	} else if (prBssInfo->eBand == BAND_5G &&
		   prAdapter->rWifiVar.ucHtSmps5g == 1) {
		prHtCap->u2HtCapInfo &=
			(~HT_CAP_INFO_SM_POWER_SAVE);
		(prHtCap->u2HtCapInfo) |=
			(1 << HT_CAP_INFO_SM_POWER_SAVE_OFFSET);
	} else if (prBssInfo->eBand == BAND_5G &&
		   prAdapter->rWifiVar.ucHtSmps5g == 3) {
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_SM_POWER_SAVE;
	}

	prHtCap->ucAmpduParam = AMPDU_PARAM_DEFAULT_VAL;

	prSupMcsSet = &prHtCap->rSupMcsSet;
	kalMemZero((void *)&prSupMcsSet->aucRxMcsBitmask[0],
		   SUP_MCS_RX_BITMASK_OCTET_NUM);

	for (ucIdx = 0;
	     ucIdx < wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);
	     ucIdx++) {
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
		if (p2pFuncGetForceTrxConfig(prAdapter) !=
			P2P_FORCE_TRX_CONFIG_NONE) {
			if (prBssInfo->ucOpChangeRxNss > ucIdx)
				prSupMcsSet->aucRxMcsBitmask[ucIdx] =
				BITS(0, 7);
		} else
#endif
#endif
			prSupMcsSet->aucRxMcsBitmask[ucIdx] = BITS(0, 7);

	}
	/* prSupMcsSet->aucRxMcsBitmask[0] = BITS(0, 7); */

	if (fg40mAllowed && IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucMCS32))
		prSupMcsSet->aucRxMcsBitmask[32 / 8] = BIT(0); /* MCS32 */
	prSupMcsSet->u2RxHighestSupportedRate = SUP_MCS_RX_DEFAULT_HIGHEST_RATE;
	prSupMcsSet->u4TxRateInfo = SUP_MCS_TX_DEFAULT_VAL;

	prHtCap->u2HtExtendedCap = HT_EXT_CAP_DEFAULT_VAL;
	if (!fg40mAllowed ||
	    prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
		prHtCap->u2HtExtendedCap &=
			~(HT_EXT_CAP_PCO | HT_EXT_CAP_PCO_TRANS_TIME_NONE);

	prHtCap->u4TxBeamformingCap = TX_BEAMFORMING_CAP_DEFAULT_VAL;
	if ((prAdapter->rWifiVar.eDbdcMode == ENUM_DBDC_MODE_DISABLED) ||
	    (prBssInfo->eBand == BAND_5G)
#if (CFG_SUPPORT_WIFI_6G == 1)
		|| (prBssInfo->eBand == BAND_6G)
#endif
	) {
		/* HT BFee has IOT issue
		 * only support HT BFee when force mode for testing
		 */
		if (IS_FEATURE_FORCE_ENABLED(prAdapter->rWifiVar.ucStaHtBfee))
			prHtCap->u4TxBeamformingCap = TX_BEAMFORMING_CAP_BFEE;
		if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaHtBfer))
			prHtCap->u4TxBeamformingCap |= TX_BEAMFORMING_CAP_BFER;
	}

	prHtCap->ucAselCap = ASEL_CAP_DEFAULT_VAL;

	ASSERT(IE_SIZE(prHtCap) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP));

	prMsduInfo->u2FrameLength += IE_SIZE(prHtCap);
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
static void rlmFillExtCapIE(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo,
			    struct MSDU_INFO *prMsduInfo)
{
	struct IE_EXT_CAP *prExtCap;
	u_int8_t fg40mAllowed, fgAppendVhtCap;
	struct STA_RECORD *prStaRec;
	struct CONNECTION_SETTINGS *prConnSettings = NULL;
	const uint8_t *extCapConn = NULL;
	uint32_t extCapIeLen = 0;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	if (prBssInfo == NULL) {
		DBGLOG(RLM, WARN, "prBssInfo is NULL\n");
		return;
	}

	if (IS_BSS_APGO(prBssInfo))
		fg40mAllowed = prBssInfo->fgAssoc40mBwAllowed;
	else
		fg40mAllowed = cnmGetBssMaxBw(prAdapter,
			prBssInfo->ucBssIndex) >= MAX_BW_40MHZ;

	if (IS_BSS_AIS(prBssInfo)) {
		prConnSettings =
			aisGetConnSettings(prAdapter, prBssInfo->ucBssIndex);
		extCapConn = kalFindIeMatchMask(ELEM_ID_EXTENDED_CAP,
					       prConnSettings->pucAssocIEs,
					       prConnSettings->assocIeLen,
					       NULL, 0, 0, NULL);
		if (extCapConn) {
			extCapIeLen =
				ELEM_HDR_LEN + RSN_IE(extCapConn)->ucLength;
			DBGLOG_MEM8(SAA, TRACE, extCapConn, extCapIeLen);
		}
	}

	/* Add Extended Capabilities IE */
	prExtCap = (struct IE_EXT_CAP *)(((uint8_t *)prMsduInfo->prPacket) +
					 prMsduInfo->u2FrameLength);

	prExtCap->ucId = ELEM_ID_EXTENDED_CAP;
	prExtCap->ucLength = ELEM_MAX_LEN_EXT_CAP;
	kalMemZero(prExtCap->aucCapabilities,
				sizeof(prExtCap->aucCapabilities));

	if (fg40mAllowed)
		SET_EXT_CAP(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
			ELEM_EXT_CAP_20_40_COEXIST_SUPPORT_BIT);

	SET_EXT_CAP(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
		ELEM_EXT_CAP_ECSA_CAP_BIT);

#if CFG_SUPPORT_802_11AC
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	fgAppendVhtCap = FALSE;

	/* Check append rule */
	if (prAdapter->rWifiVar.ucAvailablePhyTypeSet
		& PHY_TYPE_SET_802_11AC) {
		/* Note: For AIS connecting state,
		 * structure in BSS_INFO will not be inited
		 *       So, we check StaRec instead of BssInfo
		 */
		if (prStaRec) {
			if (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11AC)
				fgAppendVhtCap = TRUE;
		} else if (RLM_NET_IS_11AC(prBssInfo) &&
				((prBssInfo->eCurrentOPMode ==
				OP_MODE_INFRASTRUCTURE) ||
				(prBssInfo->eCurrentOPMode ==
				OP_MODE_ACCESS_POINT))) {
			fgAppendVhtCap = TRUE;
		}
	}

	if (fgAppendVhtCap)
		SET_EXT_CAP(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
			    ELEM_EXT_CAP_OP_MODE_NOTIFICATION_BIT);
#endif

#if (CFG_SUPPORT_TWT == 1)
#if (CFG_SUPPORT_802_11AX == 1)
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucTWTRequester) &&
		IS_BSS_AIS(prBssInfo))
		SET_EXT_CAP(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
			    ELEM_EXT_CAP_TWT_REQUESTER_BIT);
#endif
#endif

#if CFG_SUPPORT_PASSPOINT
	if (prAdapter->rWifiVar.u4SwTestMode == ENUM_SW_TEST_MODE_SIGMA_HS20_R2)
		SET_EXT_CAP(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
			    ELEM_EXT_CAP_QOSMAPSET_BIT);
#endif

	/* QoS R2 3.2.2:
	 * A Wi-Fi QoS Management STA shall support and enable by default
	 * QoS Map per 11.22.9 IEEE std 802.11-2020.
	 */
	SET_EXT_CAP(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
			ELEM_EXT_CAP_QOSMAPSET_BIT);

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
	SET_EXT_CAP(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
				ELEM_EXT_CAP_BSS_TRANSITION_BIT);
#endif

#if (CFG_SUPPORT_802_11V_MBSSID == 1)
	if (prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT)
		SET_EXT_CAP(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
				ELEM_EXT_CAP_MBSSID_BIT);
#endif

#if (CFG_SUPPORT_BCN_PROT == 1)
	if (IS_BSS_AIS(prBssInfo))
		SET_EXT_CAP(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
				ELEM_EXT_CAP_BCN_PROT_BIT);
#endif

#if CFG_FAST_PATH_SUPPORT
	if (mscsIsFpSupport(prAdapter) && IS_BSS_AIS(prBssInfo))
		SET_EXT_CAP(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
				ELEM_EXT_CAP_MSCS_BIT);
#endif

	if (extCapConn && IS_BSS_AIS(prBssInfo)) {
		if ((extCapIeLen - ELEM_HDR_LEN) > prExtCap->ucLength)
			prExtCap->ucLength = ELEM_MAX_LEN_EXT_CAP;
		rlmSyncExtCapIEwithSupplicant(prExtCap->aucCapabilities,
			extCapConn, extCapIeLen);
	} else
		DBGLOG(RLM, WARN, "extCapConn = NULL!");

	/* Disable BTM cap for WPA3 cert. */
	if (IS_BSS_AIS(prBssInfo)
		&& IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucBtmCap)) {
		CLEAR_EXT_CAP(prExtCap->aucCapabilities,
					ELEM_MAX_LEN_EXT_CAP,
					ELEM_EXT_CAP_BSS_TRANSITION_BIT);
		DBGLOG(RLM, INFO, "Disable BTM cap due to wifi.cfg");
	}

	while (prExtCap->ucLength > 0 &&
		prExtCap->aucCapabilities[prExtCap->ucLength - 1] == 0) {
		prExtCap->ucLength--;
	}

	DBGLOG_MEM8(SAA, INFO, prExtCap->aucCapabilities, prExtCap->ucLength);

	ASSERT(IE_SIZE(prExtCap) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP));

	prMsduInfo->u2FrameLength += IE_SIZE(prExtCap);
}

void rlmSyncExtCapIEwithSupplicant(uint8_t *aucCapabilities,
			const uint8_t *supExtCapIEs, size_t IElen) {
	uint32_t i;

	for (i = ELEM_HDR_LEN * 8; i < IElen * 8; i++) {
		if (supExtCapIEs[i / 8] & BIT(i % 8)) {
			SET_EXT_CAP(aucCapabilities, ELEM_MAX_LEN_EXT_CAP,
			    i-ELEM_HDR_LEN*8);
		}
	}
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
static void rlmFillHtOpIE(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo,
			  struct MSDU_INFO *prMsduInfo)
{
	struct IE_HT_OP *prHtOp;
	uint16_t i;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	prHtOp = (struct IE_HT_OP *)(((uint8_t *)prMsduInfo->prPacket) +
				     prMsduInfo->u2FrameLength);

	/* Add HT operation IE */
	prHtOp->ucId = ELEM_ID_HT_OP;
	prHtOp->ucLength = sizeof(struct IE_HT_OP) - ELEM_HDR_LEN;

	/* RIFS and 20/40 bandwidth operations are included */
	prHtOp->ucPrimaryChannel = prBssInfo->ucPrimaryChannel;
	prHtOp->ucInfo1 = prBssInfo->ucHtOpInfo1;

	/* Decide HT protection mode field */
	if (prBssInfo->eHtProtectMode == HT_PROTECT_MODE_NON_HT)
		prHtOp->u2Info2 = (uint8_t)HT_PROTECT_MODE_NON_HT;
	else if (prBssInfo->eObssHtProtectMode == HT_PROTECT_MODE_NON_MEMBER)
		prHtOp->u2Info2 = (uint8_t)HT_PROTECT_MODE_NON_MEMBER;
	else {
		/* It may be SYS_PROTECT_MODE_NONE or SYS_PROTECT_MODE_20M */
		prHtOp->u2Info2 = (uint8_t)prBssInfo->eHtProtectMode;
	}

	if (prBssInfo->eGfOperationMode != GF_MODE_NORMAL) {
		/* It may be GF_MODE_PROTECT or GF_MODE_DISALLOWED
		 * Note: it will also be set in ad-hoc network
		 */
		prHtOp->u2Info2 |= HT_OP_INFO2_NON_GF_HT_STA_PRESENT;
	}

	if (0 /* Regulatory class 16 */ &&
	    prBssInfo->eObssHtProtectMode == HT_PROTECT_MODE_NON_MEMBER) {
		/* (TBD) It is HT_PROTECT_MODE_NON_MEMBER, so require protection
		 * although it is possible to have no protection by spec.
		 */
		prHtOp->u2Info2 |= HT_OP_INFO2_OBSS_NON_HT_STA_PRESENT;
	}

	prHtOp->u2Info3 = prBssInfo->u2HtOpInfo3; /* To do: handle L-SIG TXOP */

	/* No basic MCSx are needed temporarily */
	for (i = 0; i < 16; i++)
		prHtOp->aucBasicMcsSet[i] = 0;

	ASSERT(IE_SIZE(prHtOp) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_OP));

	prMsduInfo->u2FrameLength += IE_SIZE(prHtOp);
}

void rlmGeneratePwrConstraintIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
#if CFG_ENABLE_WIFI_DIRECT
	struct BSS_INFO *prBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prSpecBssInfo;
	struct IE_HT_TPE *prPwrConstraint;

	if (!prAdapter || !prMsduInfo)
		return;

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo || !IS_BSS_APGO(prBssInfo))
		return;

	if (!IS_BSS_ACTIVE(prBssInfo))
		return;

	if (!(prBssInfo->ucPhyTypeSet & PHY_TYPE_SET_802_11A))
		return;

	prSpecBssInfo = prAdapter->rWifiVar.prP2pSpecificBssInfo[
		prBssInfo->u4PrivateData];
	if (prSpecBssInfo->fgAddPwrConstrIe == FALSE &&
	    IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgSapAddTPEIE))
		return;

	prPwrConstraint = (struct IE_HT_TPE *)
		(((uint8_t *)prMsduInfo->prPacket) +
		prMsduInfo->u2FrameLength);

	prPwrConstraint->ucId = ELEM_ID_PWR_CONSTRAINT;
	prPwrConstraint->ucLength =
		sizeof(struct IE_HT_TPE) - ELEM_HDR_LEN;
	prPwrConstraint->u8TxPowerInfo = 3;

	prMsduInfo->u2FrameLength += IE_SIZE(prPwrConstraint);
#endif
}

#if CFG_SUPPORT_802_11AC

/*----------------------------------------------------------------------------*/
/*!
 * \brief For probe request, association request
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmReqGenerateVhtCapIE(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct BSS_DESC *prBssDesc = NULL;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (!prBssInfo || !prStaRec) {
		DBGLOG(RLM, ERROR, "prBssInfo=%p, prStaRec=%p, return!!\n",
			prBssInfo, prStaRec);
		return;
	}

	if (IS_BSS_AIS(prBssInfo))
		prBssDesc =
			aisGetTargetBssDesc(prAdapter, prMsduInfo->ucBssIndex);
#if CFG_ENABLE_WIFI_DIRECT
	else if (IS_BSS_P2P(prBssInfo))
		prBssDesc =
			p2pGetTargetBssDesc(prAdapter, prMsduInfo->ucBssIndex);
#endif

	if (!prBssDesc) {
		DBGLOG(RLM, ERROR, "prBssDesc is NULL, return!\n");
		return;
	}

	DBGLOG(RLM, TRACE,
		"eBand=%d,PhyType=0x%02x, AvailPhyType=0x%02x, VHTPresent=%d\n",
		prBssDesc->eBand,
		prStaRec->ucPhyTypeSet,
		prAdapter->rWifiVar.ucAvailablePhyTypeSet,
		prBssDesc->fgIsVHTPresent);

	if ((prBssDesc->eBand == BAND_5G) &&
		(prAdapter->rWifiVar.ucAvailablePhyTypeSet &
		PHY_TYPE_SET_802_11AC)
		&& (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11AC))
		rlmFillVhtCapIE(prAdapter, prBssInfo, prMsduInfo);
#if CFG_SUPPORT_VHT_IE_IN_2G
	else if ((prBssDesc->eBand == BAND_2G4) &&
			(prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N) &&
			((prAdapter->rWifiVar.ucVhtIeIn2g
				==  FEATURE_FORCE_ENABLED) ||
			((prAdapter->rWifiVar.ucVhtIeIn2g == FEATURE_ENABLED) &&
				(prBssDesc->fgIsVHTPresent)))) {
		DBGLOG(RLM, TRACE, "Add VHT IE in 2.4G\n");
		rlmFillVhtCapIE(prAdapter, prBssInfo, prMsduInfo);
	}
#endif
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
void rlmRspGenerateVhtCapIE(struct ADAPTER *prAdapter,
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

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (IS_BSS_APGO(prBssInfo) && prBssInfo->eBand == BAND_6G)
		return;
#endif

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	/* Decide PHY type set source */
	if (prStaRec) {
		/* Get PHY type set from target STA */
		ucPhyTypeSet = prStaRec->ucPhyTypeSet;
	} else {
		/* Get PHY type set from current BSS */
		ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
	}

	if (RLM_NET_IS_11AC(prBssInfo) &&
	    (ucPhyTypeSet & PHY_TYPE_SET_802_11AC))
		rlmFillVhtCapIE(prAdapter, prBssInfo, prMsduInfo);
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
void rlmRspGenerateVhtOpIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucPhyTypeSet;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	if (!IS_BSS_ACTIVE(prBssInfo))
		return;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (IS_BSS_APGO(prBssInfo) && prBssInfo->eBand == BAND_6G)
		return;
#endif

	/* Decide PHY type set source */
	if (prStaRec) {
		/* Get PHY type set from target STA */
		ucPhyTypeSet = prStaRec->ucPhyTypeSet;
	} else {
		/* Get PHY type set from current BSS */
		ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
	}

	if (RLM_NET_IS_11AC(prBssInfo) &&
	    (ucPhyTypeSet & PHY_TYPE_SET_802_11AC))
		rlmFillVhtOpIE(prAdapter, prBssInfo, prMsduInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief For probe request, association request
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmReqGenerateVhtOpNotificationIE(struct ADAPTER *prAdapter,
				       struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;
	/* [TGac 5.2.46 STBC Receive Test with UCC 9.2.x]
	 * Operating Notification IE of Nss=2 will make Ralink testbed send data
	 * frames without STBC
	 * Enable the Operating Notification IE only for DBDC enable case.
	 */
#if (CFG_SUPPORT_DBDC_DOWNGRADE_BW == 1)
	if (!prAdapter->rWifiVar.fgDbDcModeEn)
		return;
#else
	if (prBssInfo->ucOpRxNss ==
		wlanGetSupportNss(prAdapter, prMsduInfo->ucBssIndex))
		return;
#endif

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet &
	     PHY_TYPE_SET_802_11AC) &&
	    (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11AC)
#if (CFG_SUPPORT_WIFI_6G == 1)
	    || (prBssInfo->eBand == BAND_6G &&
			prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11AX)
#endif
	)) {
		/* Fill own capability in channel width field in OP mode element
		 * since we haven't filled in channel width info in BssInfo at
		 * current state
		 */
		rlmFillVhtOpNotificationIE(prAdapter, prBssInfo, prMsduInfo,
					   TRUE);
	}
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
void rlmRspGenerateVhtOpNotificationIE(struct ADAPTER *prAdapter,
				       struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucPhyTypeSet;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	if (!IS_BSS_ACTIVE(prBssInfo))
		return;

#if (CFG_SUPPORT_DBDC_DOWNGRADE_BW == 1)
	if (!prAdapter->rWifiVar.fgDbDcModeEn)
		return;
#else
	if (prBssInfo->ucOpRxNss ==
		wlanGetSupportNss(prAdapter, prMsduInfo->ucBssIndex))
		return;
#endif

	/* Decide PHY type set source */
	if (prStaRec) {
		/* Get PHY type set from target STA */
		ucPhyTypeSet = prStaRec->ucPhyTypeSet;
	} else {
		/* Get PHY type set from current BSS */
		ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
	}

	if ((RLM_NET_IS_11AC(prBssInfo) &&
	    ucPhyTypeSet & PHY_TYPE_SET_802_11AC)
#if (CFG_SUPPORT_WIFI_6G == 1)
		|| (prBssInfo->eBand == BAND_6G &&
			ucPhyTypeSet & PHY_TYPE_SET_802_11AX)
#endif
	) {
		rlmFillVhtOpNotificationIE(prAdapter, prBssInfo, prMsduInfo,
					   FALSE);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 * add VHT operation notification IE for VHT-BW40 case specific
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static void rlmFillVhtOpNotificationIE(struct ADAPTER *prAdapter,
				       struct BSS_INFO *prBssInfo,
				       struct MSDU_INFO *prMsduInfo,
				       u_int8_t fgIsOwnCap)
{
	struct IE_VHT_OP_MODE_NOTIFICATION *prVhtOpMode;
	uint8_t ucOpModeBw = VHT_OP_MODE_CHANNEL_WIDTH_20;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	prVhtOpMode = (struct IE_VHT_OP_MODE_NOTIFICATION
			       *)(((uint8_t *)prMsduInfo->prPacket) +
				  prMsduInfo->u2FrameLength);

	kalMemZero((void *)prVhtOpMode,
		   sizeof(struct IE_VHT_OP_MODE_NOTIFICATION));

	prVhtOpMode->ucId = ELEM_ID_OP_MODE;
	prVhtOpMode->ucLength =
		sizeof(struct IE_VHT_OP_MODE_NOTIFICATION) - ELEM_HDR_LEN;

	DBGLOG(RLM, TRACE, "rlmFillVhtOpNotificationIE(%d) %u %u\n",
	       prBssInfo->ucBssIndex, fgIsOwnCap, prBssInfo->ucOpRxNss);

	if (fgIsOwnCap) {
		struct BSS_DESC *prBssDesc = NULL;
		uint8_t ucRfCenterFreqSeg1, ucPrimaryChannel;
		enum ENUM_CHANNEL_WIDTH eRfChannelWidth;
		enum ENUM_CHNL_EXT eRfSco;

		if (IS_BSS_AIS(prBssInfo))
			prBssDesc = aisGetTargetBssDesc(prAdapter,
				prMsduInfo->ucBssIndex);
#if CFG_ENABLE_WIFI_DIRECT
		else if (IS_BSS_P2P(prBssInfo))
			prBssDesc = p2pGetTargetBssDesc(prAdapter,
				prMsduInfo->ucBssIndex);
#endif
		if (prBssDesc) {
			ucPrimaryChannel = prBssDesc->ucChannelNum;
			eRfSco = prBssDesc->eSco;
			eRfChannelWidth = prBssDesc->eChannelWidth;
			ucRfCenterFreqSeg1 = nicGetS1(prAdapter,
				prBssDesc->eBand, ucPrimaryChannel,
				eRfChannelWidth);

			/* Fix the IOT issue of low DL t-put of VHT40 and HE40.
			 * The root cause is that the channel width of operation
			 * mode notification element in association request is
			 * wrong. According to 11ac 10.41, it shall be the
			 * maximum receiving bandwidth in operation rather than
			 * maximum chip capability. For example, it shall be
			 * 40MHz rather than 80MHz in VHT40 and HE40. Otherwise,
			 * some AP will try to transmit packets in 80MHz first
			 * even we can only receive packets with bandwidth up to
			 * 40MHz. So, we copy the bandwidth information in
			 * MID_MNY_CNM_CH_REQ to AIS BssInfo for later reference
			 * of the gereration of the operation mode notification
			 * element.
			 */
			rlmReviseMaxBw(prAdapter, prMsduInfo->ucBssIndex,
				&eRfSco, &eRfChannelWidth,
				&ucRfCenterFreqSeg1, &ucPrimaryChannel);
			ucOpModeBw = rlmGetBssOpBwByChannelWidth(eRfSco,
				eRfChannelWidth);
		} else {
			ucOpModeBw = cnmGetDbdcBwCapability(prAdapter,
				    prBssInfo->ucBssIndex);
			DBGLOG(RLM, ERROR,
				"Bss%d can't find bssdesc, fallback opmodebw=%d\n",
				prBssInfo->ucBssIndex, ucOpModeBw);
		}

		/*handle 80P80 case*/
		if (ucOpModeBw >= MAX_BW_160MHZ) {
			ucOpModeBw = VHT_OP_MODE_CHANNEL_WIDTH_80;
			prVhtOpMode->ucOperatingMode |=
				VHT_OP_MODE_CHANNEL_WIDTH_80P80_160;
		}

		prVhtOpMode->ucOperatingMode |= ucOpModeBw;
		prVhtOpMode->ucOperatingMode |=
			(((prBssInfo->ucOpRxNss - 1)
				<< VHT_OP_MODE_RX_NSS_OFFSET) &
			 VHT_OP_MODE_RX_NSS);
	} else {
		ucOpModeBw = rlmGetOpModeBwByVhtAndHtOpInfo(prBssInfo);
		if (ucOpModeBw == VHT_OP_MODE_CHANNEL_WIDTH_160_80P80) {
			ucOpModeBw = VHT_OP_MODE_CHANNEL_WIDTH_80;
			prVhtOpMode->ucOperatingMode |=
				VHT_OP_MODE_CHANNEL_WIDTH_80P80_160;
		}

		prVhtOpMode->ucOperatingMode |= ucOpModeBw;
		prVhtOpMode->ucOperatingMode |=
			(((prBssInfo->ucOpRxNss - 1)
				<< VHT_OP_MODE_RX_NSS_OFFSET) &
			 VHT_OP_MODE_RX_NSS);
	}

	prMsduInfo->u2FrameLength += IE_SIZE(prVhtOpMode);
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
static void rlmFillVhtCapIeMcs(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo,
				struct VHT_SUPPORTED_MCS_FIELD
					*prVhtSupportedMcsSet,
				uint8_t ucOffset,
				uint8_t ucAntIdx)
{
	uint8_t ucRxMcsMap, ucTxMcsMap;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (ucAntIdx < wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex)) {
		if (IS_BSS_AIS(prBssInfo) &&
		    prWifiVar->ucStaMaxMcsMap != 0xFF) {
			ucRxMcsMap = kal_min_t(uint8_t,
					prWifiVar->ucStaMaxMcsMap,
					VHT_CAP_INFO_MCS_MAP_MCS9);
			ucTxMcsMap = kal_min_t(uint8_t,
					prWifiVar->ucStaMaxMcsMap,
					VHT_CAP_INFO_MCS_MAP_MCS9);
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
		} else if (p2pFuncGetForceTrxConfig(prAdapter) !=
			P2P_FORCE_TRX_CONFIG_NONE) {
			if (prBssInfo->ucOpChangeTxNss > ucAntIdx)
				ucTxMcsMap = VHT_CAP_INFO_MCS_MAP_MCS7;
			else
				ucTxMcsMap =
					VHT_CAP_INFO_MCS_NOT_SUPPORTED;

			if (prBssInfo->ucOpChangeRxNss > ucAntIdx)
				ucRxMcsMap = VHT_CAP_INFO_MCS_MAP_MCS7;
			else
				ucRxMcsMap =
					VHT_CAP_INFO_MCS_NOT_SUPPORTED;
#endif
#endif
		} else {
			ucRxMcsMap = VHT_CAP_INFO_MCS_MAP_MCS9;
			ucTxMcsMap = VHT_CAP_INFO_MCS_MAP_MCS9;
		}
	} else {
		ucRxMcsMap = VHT_CAP_INFO_MCS_NOT_SUPPORTED;
		ucTxMcsMap = VHT_CAP_INFO_MCS_NOT_SUPPORTED;
	}

	prVhtSupportedMcsSet->u2RxMcsMap |= (ucRxMcsMap << ucOffset);
	prVhtSupportedMcsSet->u2TxMcsMap |= (ucTxMcsMap << ucOffset);
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
static void rlmFillVhtCapIE(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo,
			    struct MSDU_INFO *prMsduInfo)
{
	struct IE_VHT_CAP *prVhtCap;
	struct VHT_SUPPORTED_MCS_FIELD *prVhtSupportedMcsSet;
	uint8_t i;
	uint8_t ucMaxBw;
	uint8_t supportNss;
#if CFG_SUPPORT_BFEE
	struct STA_RECORD *prStaRec;
#endif

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	prVhtCap = (struct IE_VHT_CAP *)(((uint8_t *)prMsduInfo->prPacket) +
					 prMsduInfo->u2FrameLength);

	prVhtCap->ucId = ELEM_ID_VHT_CAP;
	prVhtCap->ucLength = sizeof(struct IE_VHT_CAP) - ELEM_HDR_LEN;
	prVhtCap->u4VhtCapInfo = VHT_CAP_INFO_DEFAULT_VAL;

	ucMaxBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);

	prVhtCap->u4VhtCapInfo |= (prAdapter->rWifiVar.ucRxMaxMpduLen &
				   VHT_CAP_INFO_MAX_MPDU_LEN_MASK);

	if (ucMaxBw == MAX_BW_160MHZ)
		prVhtCap->u4VhtCapInfo |=
			VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160;
	else if (ucMaxBw == MAX_BW_80_80_MHZ)
		prVhtCap->u4VhtCapInfo |=
			VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160_80P80;
	else
		prVhtCap->u4VhtCapInfo |=
			VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_NONE;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaVhtBfee)) {
		prVhtCap->u4VhtCapInfo |= FIELD_VHT_CAP_INFO_BFEE;
#if CFG_SUPPORT_BFEE
	prStaRec = cnmGetStaRecByIndex(prAdapter,
					prMsduInfo->ucStaRecIndex);

	if (prStaRec && (prStaRec->ucVhtCapNumSoundingDimensions == 0x2) &&
	    !prAdapter->rWifiVar.fgForceSTSNum) {
		/* For the compatibility with netgear R7000 AP */
		prVhtCap->u4VhtCapInfo |=
	(((uint32_t)prStaRec->ucVhtCapNumSoundingDimensions)
<< VHT_CAP_INFO_COMPRESSED_STEERING_NUMBER_OF_BEAMFORMER_ANTENNAS_SUP_OFF);
		DBGLOG(RLM, INFO, "Set VHT Cap BFEE STS CAP=%d\n",
		       prStaRec->ucVhtCapNumSoundingDimensions);
	} else {
		/* For 11ac cert. VHT-5.2.63C MU-BFee step3,
		* it requires STAUT to set its maximum STS capability here
		*/
		prVhtCap->u4VhtCapInfo |=
VHT_CAP_INFO_COMPRESSED_STEERING_NUMBER_OF_BEAMFORMER_ANTENNAS_4_SUP;
		DBGLOG(RLM, TRACE, "Set VHT Cap BFEE STS CAP=%d\n",
			VHT_CAP_INFO_BEAMFORMEE_STS_CAP_MAX);
	}
	supportNss = wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);
	if (supportNss == 2)
		prVhtCap->u4VhtCapInfo |=
		    VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS_2_SUPPORTED;
#endif
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaVhtMuBfee))
		prVhtCap->u4VhtCapInfo |=
			VHT_CAP_INFO_MU_BEAMFOMEE_CAPABLE;
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaVhtBfer))
		prVhtCap->u4VhtCapInfo |= FIELD_VHT_CAP_INFO_BFER;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxShortGI)) {
		if (ucMaxBw >= MAX_BW_80MHZ)
			prVhtCap->u4VhtCapInfo |= VHT_CAP_INFO_SHORT_GI_80;

		if (ucMaxBw >= MAX_BW_160MHZ)
			prVhtCap->u4VhtCapInfo |=
				VHT_CAP_INFO_SHORT_GI_160_80P80;
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxLdpc))
		prVhtCap->u4VhtCapInfo |= VHT_CAP_INFO_RX_LDPC;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxStbc)) {
		uint8_t tempRxStbcNss;

		if (prAdapter->rWifiVar.u4SwTestMode ==
		    ENUM_SW_TEST_MODE_SIGMA_AC) {
			tempRxStbcNss = 1;
			DBGLOG(RLM, INFO,
			       "Set RxStbcNss to 1 for 11ac certification.\n");
		} else {
			tempRxStbcNss = prAdapter->rWifiVar.ucRxStbcNss;
			tempRxStbcNss =
				(tempRxStbcNss >
				 wlanGetSupportNss(prAdapter,
						   prBssInfo->ucBssIndex))
					? wlanGetSupportNss(
						  prAdapter,
						  prBssInfo->ucBssIndex)
					: (tempRxStbcNss);
			if (tempRxStbcNss != prAdapter->rWifiVar.ucRxStbcNss) {
				DBGLOG(RLM, WARN,
				       "Apply Nss:%d as RxStbcNss in VHT Cap",
				       wlanGetSupportNss(
					       prAdapter,
					       prBssInfo->ucBssIndex));
				DBGLOG(RLM, WARN,
				       "due to set RxStbcNss more than Nss is not appropriate.\n");
			}
		}
		prVhtCap->u4VhtCapInfo |=
			((tempRxStbcNss << VHT_CAP_INFO_RX_STBC_OFFSET) &
			 VHT_CAP_INFO_RX_STBC_MASK);
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucTxStbc) &&
			prBssInfo->ucOpTxNss >= 2)
		prVhtCap->u4VhtCapInfo |= VHT_CAP_INFO_TX_STBC;

	prVhtCap->u4VhtCapInfo |=
		((VHT_CAP_INFO_EXT_NSS_BW_SUPPORT
			<< VHT_CAP_INFO_EXT_NSS_BW_SUPPORT_OFFSET) &
		 VHT_CAP_INFO_EXT_NSS_BW_SUPPORT_MASK);


	/*set MCS map */
	prVhtSupportedMcsSet = &prVhtCap->rVhtSupportedMcsSet;
	kalMemZero((void *)prVhtSupportedMcsSet,
		   sizeof(struct VHT_SUPPORTED_MCS_FIELD));

	for (i = 0; i < 8; i++) {
		uint8_t ucOffset = i * 2;

		rlmFillVhtCapIeMcs(prAdapter,
					prBssInfo,
					prVhtSupportedMcsSet,
					ucOffset,
					i);
	}
	DBGLOG(RLM, TRACE, "VHT MCS Map: %x %x",
		prVhtSupportedMcsSet->u2RxMcsMap,
		prVhtSupportedMcsSet->u2TxMcsMap);

#if 0
	for (i = 0; i < wlanGetSupportNss(prAdapter,
		prBssInfo->ucBssIndex); i++) {
		uint8_t ucOffset = i * 2;

		prVhtSupportedMcsSet->u2RxMcsMap &=
			((VHT_CAP_INFO_MCS_MAP_MCS9 << ucOffset) &
			BITS(ucOffset, ucOffset + 1));
		prVhtSupportedMcsSet->u2TxMcsMap &=
			((VHT_CAP_INFO_MCS_MAP_MCS9 << ucOffset) &
			BITS(ucOffset, ucOffset + 1));
	}
#endif

	prVhtSupportedMcsSet->u2RxHighestSupportedDataRate =
		VHT_CAP_INFO_DEFAULT_HIGHEST_DATA_RATE;
	prVhtSupportedMcsSet->u2TxHighestSupportedDataRate =
		VHT_CAP_INFO_DEFAULT_HIGHEST_DATA_RATE;

	prVhtSupportedMcsSet->u2TxHighestSupportedDataRate
		|= VHT_CAP_INFO_EXT_NSS_BW_CAP;

	ASSERT(IE_SIZE(prVhtCap) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP));

	prMsduInfo->u2FrameLength += IE_SIZE(prVhtCap);
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
void rlmFillVhtOpIE(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo,
		    struct MSDU_INFO *prMsduInfo)
{
	struct IE_VHT_OP *prVhtOp;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	prVhtOp = (struct IE_VHT_OP *)(((uint8_t *)prMsduInfo->prPacket) +
				       prMsduInfo->u2FrameLength);

	/* Add HT operation IE */
	prVhtOp->ucId = ELEM_ID_VHT_OP;
	prVhtOp->ucLength = sizeof(struct IE_VHT_OP) - ELEM_HDR_LEN;

	ASSERT(IE_SIZE(prVhtOp) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP));

	/* (UINT8)VHT_OP_CHANNEL_WIDTH_80; */
	prVhtOp->ucVhtOperation[0] = prBssInfo->ucVhtChannelWidth;
	prVhtOp->ucVhtOperation[1] = prBssInfo->ucVhtChannelFrequencyS1;
	prVhtOp->ucVhtOperation[2] = prBssInfo->ucVhtChannelFrequencyS2;

#if 0
	if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex) < MAX_BW_80MHZ) {
		prVhtOp->ucVhtOperation[0] = VHT_OP_CHANNEL_WIDTH_20_40;
		prVhtOp->ucVhtOperation[1] = 0;
		prVhtOp->ucVhtOperation[2] = 0;
	} else if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex) ==
			MAX_BW_80MHZ) {
		prVhtOp->ucVhtOperation[0] = VHT_OP_CHANNEL_WIDTH_80;
		prVhtOp->ucVhtOperation[1] =
			nicGetVhtS1(prBssInfo->ucPrimaryChannel);
		prVhtOp->ucVhtOperation[2] = 0;
	} else {
		/* TODO: BW80 + 80/160 support */
	}
#endif

	/* VHT-4.2.58 */
	if (IS_BSS_APGO(prBssInfo) &&
		(prBssInfo->ucVhtChannelWidth == VHT_OP_CHANNEL_WIDTH_160)) {
		/*
		 * Convert 160 MHz channel width to new style as interop
		 * workaround.
		 */
		prVhtOp->ucVhtOperation[0] = 1;
		prVhtOp->ucVhtOperation[2] = prVhtOp->ucVhtOperation[1];
		if (prBssInfo->ucPrimaryChannel <
			prBssInfo->ucVhtChannelFrequencyS1)
			prVhtOp->ucVhtOperation[1] -= 8;
		else
			prVhtOp->ucVhtOperation[1] += 8;
	}

	prVhtOp->u2VhtBasicMcsSet = prBssInfo->u2VhtBasicMcsSet;

	prMsduInfo->u2FrameLength += IE_SIZE(prVhtOp);
}

void rlmGenerateVhtTPEIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucPhyTypeSet;

	if (!prAdapter || !prMsduInfo)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo || !IS_BSS_APGO(prBssInfo))
		return;

	if (!IS_BSS_ACTIVE(prBssInfo))
		return;

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgSapAddTPEIE))
		return;

	/* Decide PHY type set source */
	if (prStaRec) {
		/* Get PHY type set from target STA */
		ucPhyTypeSet = prStaRec->ucPhyTypeSet;
	} else {
		/* Get PHY type set from current BSS */
		ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
	}

	if (RLM_NET_IS_11AC(prBssInfo) &&
		(ucPhyTypeSet & PHY_TYPE_SET_802_11AC)) {
		struct IE_VHT_TPE *prVhtTpe;
		uint8_t ucChannelWidth;
		uint8_t ucPower;
		uint8_t i = 0;

		ucChannelWidth = prBssInfo->ucVhtChannelWidth;

		prVhtTpe = (struct IE_VHT_TPE *)
			(((uint8_t *)prMsduInfo->prPacket) +
			prMsduInfo->u2FrameLength);

		prVhtTpe->ucId = ELEM_ID_TX_PWR_ENVELOPE;

		if (ucChannelWidth > VHT_OP_CHANNEL_WIDTH_80)
			ucPower = 3;
		else if (ucChannelWidth == VHT_OP_CHANNEL_WIDTH_80)
			ucPower = 2;
		else {
			if (prBssInfo->eBssSCO != CHNL_EXT_SCN)
				ucPower = 1;
			else
				ucPower = 0;
		}

		prVhtTpe->ucLength = (ucPower + 2);

		/* 23.5dB */
		for (i = 0; i <= ucPower; i++)
			prVhtTpe->u8TxPowerBw[i] = 47;

		prVhtTpe->u8TxPowerInfo = 0x10 | ucPower;

		prMsduInfo->u2FrameLength += IE_SIZE(prVhtTpe);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Get RxNss from VHT CAP IE.
 *
 * \param[in] prVhtCap
 *
 * \return ucRxNss
 */
/*----------------------------------------------------------------------------*/
uint8_t
rlmGetSupportRxNssInVhtCap(struct IE_VHT_CAP *prVhtCap)
{
	uint8_t ucRxNss = 1;

	if (!prVhtCap) {
		DBGLOG(RLM, TRACE, "null prVhtCap, assume RxNss=1\n");
		return ucRxNss;
	}

	if (((prVhtCap->rVhtSupportedMcsSet.u2RxMcsMap &
		VHT_CAP_INFO_MCS_2SS_MASK) >>
		VHT_CAP_INFO_MCS_2SS_OFFSET)
		!= VHT_CAP_INFO_MCS_NOT_SUPPORTED)
		ucRxNss = 2;
	if (((prVhtCap->rVhtSupportedMcsSet.u2RxMcsMap &
		VHT_CAP_INFO_MCS_3SS_MASK)
		>> VHT_CAP_INFO_MCS_3SS_OFFSET)
		!= VHT_CAP_INFO_MCS_NOT_SUPPORTED)
		ucRxNss = 3;
	if (((prVhtCap->rVhtSupportedMcsSet.u2RxMcsMap &
		VHT_CAP_INFO_MCS_4SS_MASK)
		>> VHT_CAP_INFO_MCS_4SS_OFFSET)
		!= VHT_CAP_INFO_MCS_NOT_SUPPORTED)
		ucRxNss = 4;
	if (((prVhtCap->rVhtSupportedMcsSet.u2RxMcsMap &
		VHT_CAP_INFO_MCS_5SS_MASK)
		>> VHT_CAP_INFO_MCS_5SS_OFFSET)
		!= VHT_CAP_INFO_MCS_NOT_SUPPORTED)
		ucRxNss = 5;
	if (((prVhtCap->rVhtSupportedMcsSet.u2RxMcsMap &
		VHT_CAP_INFO_MCS_6SS_MASK)
		>> VHT_CAP_INFO_MCS_6SS_OFFSET)
		!= VHT_CAP_INFO_MCS_NOT_SUPPORTED)
		ucRxNss = 6;
	if (((prVhtCap->rVhtSupportedMcsSet.u2RxMcsMap &
		VHT_CAP_INFO_MCS_7SS_MASK)
		>> VHT_CAP_INFO_MCS_7SS_OFFSET)
		!= VHT_CAP_INFO_MCS_NOT_SUPPORTED)
		ucRxNss = 7;
	if (((prVhtCap->rVhtSupportedMcsSet.u2RxMcsMap &
		VHT_CAP_INFO_MCS_8SS_MASK)
		>> VHT_CAP_INFO_MCS_8SS_OFFSET)
		!= VHT_CAP_INFO_MCS_NOT_SUPPORTED)
		ucRxNss = 8;

	return ucRxNss;
}

#endif

void rlmReqGenerateOMIIE(struct ADAPTER *prAdapter,
			struct BSS_INFO *prBssInfo)
{
	struct STA_RECORD *prStaRec;

	if (!prBssInfo || !prAdapter)
		return;

#if (CFG_SUPPORT_DBDC_DOWNGRADE_BW == 1)
	if (!prAdapter->rWifiVar.fgDbDcModeEn) {
		DBGLOG(RLM, WARN, "DBDC disable, return\n");
		return;
	}
#else
	if (prBssInfo->ucOpRxNss ==
		wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex)) {
		DBGLOG(RLM, WARN, "Op NSS == Max NSS %d\n",
			prBssInfo->ucOpRxNss);
		return;
	}
#endif

	prStaRec = prBssInfo->prStaRecOfAP;
	if (!prStaRec) {
		DBGLOG(RLM, WARN, "prStaRecOfAP null\n");
		return;
	}

	DBGLOG(RLM, TRACE, "try send OMI frame if possible\n");
#if (CFG_SUPPORT_802_11AX == 1)
	if (((RLM_NET_IS_11AX(prBssInfo) &&
		(prStaRec->ucDesiredPhyTypeSet &
		PHY_TYPE_SET_802_11AX))
#if (CFG_SUPPORT_802_11BE == 1)
		|| (RLM_NET_IS_11BE(prBssInfo) &&
		(prStaRec->ucDesiredPhyTypeSet &
		PHY_TYPE_SET_802_11BE))
#endif /* CFG_SUPPORT_802_11BE  */
		) && HE_IS_MAC_CAP_OM_CTRL(prStaRec->ucHeMacCapInfo)
		&& (prAdapter->rWifiVar.ucDbdcOMFrame & ENABLE_OMI)) {
		rlmSendOMIDataFrame(prAdapter, prStaRec,
				rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo),
				prBssInfo->ucOpRxNss,
				prBssInfo->ucOpTxNss,
				rlmNotifyOMIOpModeTxDone);
	}
#endif /* CFG_SUPPORT_802_11AX */
}

#if CFG_SUPPORT_802_11D
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmGenerateCountryIE(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo =
		prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	unsigned char *pucBuf =
		(((unsigned char *) prMsduInfo->prPacket) +
		prMsduInfo->u2FrameLength);

	if (prBssInfo->aucCountryStr[0] == 0)
		return;

	COUNTRY_IE(pucBuf)->ucId = ELEM_ID_COUNTRY_INFO;
	COUNTRY_IE(pucBuf)->ucLength = prBssInfo->ucCountryIELen;
	COUNTRY_IE(pucBuf)->aucCountryStr[0] = prBssInfo->aucCountryStr[0];
	COUNTRY_IE(pucBuf)->aucCountryStr[1] = prBssInfo->aucCountryStr[1];
	COUNTRY_IE(pucBuf)->aucCountryStr[2] = prBssInfo->aucCountryStr[2];
	kalMemCopy(COUNTRY_IE(pucBuf)->arCountryStr,
		prBssInfo->aucSubbandTriplet,
		prBssInfo->ucCountryIELen - 3);

	prMsduInfo->u2FrameLength += IE_SIZE(pucBuf);
}
#endif

void rlmModifyVhtBwPara(uint8_t *pucVhtChannelFrequencyS1,
			uint8_t *pucVhtChannelFrequencyS2,
			uint8_t ucHtChannelFrequencyS3,
			uint8_t *pucVhtChannelWidth)
{
	uint8_t i = 0, ucTempS = 0;
	uint8_t ucBW160Inteval = 8;
	uint8_t ucS1 = 0, ucS2 = 0;

	if ((*pucVhtChannelFrequencyS1 != 0) &&
	    (*pucVhtChannelFrequencyS2 != 0)) {
		/* if BW160's NSS >= max VHT NSS support,
		 * use VHT S1 & S2 directly
		 */
		ucS1 = *pucVhtChannelFrequencyS1;
		ucS2 = *pucVhtChannelFrequencyS2;
	} else if ((*pucVhtChannelFrequencyS1 != 0) &&
		ucHtChannelFrequencyS3 != 0) {
		/* if BW160's NSS < max VHT NSS support,
		 * then HT S3 stands for VHT S2
		 */
		ucS1 = *pucVhtChannelFrequencyS1;
		ucS2 = ucHtChannelFrequencyS3;
	}

	if (((ucS1 - ucS2) == ucBW160Inteval) ||
		((ucS2 - ucS1) == ucBW160Inteval)) {
		/*C160 case*/

		/* NEW spec should set central ch of bw80 at S1,
		 * set central ch of bw160 at S2
		 */
		for (i = 0; i < 2; i++) {

			if (i == 0)
				ucTempS = ucS1;
			else
				ucTempS = ucS2;

			if ((ucTempS == 50) || (ucTempS == 82)
			   || (ucTempS == 114) || (ucTempS == 163)
#if (CFG_SUPPORT_WIFI_6G == 1)
			   || (ucTempS == 15) || (ucTempS == 47)
			   || (ucTempS == 79) || (ucTempS == 111)
			   || (ucTempS == 143) || (ucTempS == 175)
			   || (ucTempS == 207)
#endif
			   )

				break;
		}

		if (ucTempS == 0) {
			DBGLOG(RLM, WARN,
			       "please check BW160 setting, find central freq fail\n");
			return;
		}

		*pucVhtChannelFrequencyS1 = ucTempS;
		*pucVhtChannelFrequencyS2 = 0;
		*pucVhtChannelWidth = CW_160MHZ;
	} else {
		/*real 80P80 case*/
	}
}

#if (CFG_SUPPORT_WIFI_6G == 1)
void rlmTransferHe6gOpInfor(struct ADAPTER *prAdapter,
	uint8_t ucChannelNum,
	uint8_t ucChannelWidth,
	uint8_t *pucChannelWidth,
	uint8_t *pucCenterFreqS1,
	uint8_t *pucCenterFreqS2,
	enum ENUM_CHNL_EXT *peSco)
{
	switch (ucChannelWidth) {
	case HE_OP_CHANNEL_WIDTH_20:
	case HE_OP_CHANNEL_WIDTH_40:
		*pucChannelWidth = (uint8_t)CW_20_40MHZ;
		break;
	case HE_OP_CHANNEL_WIDTH_80:
		*pucChannelWidth = (uint8_t)CW_80MHZ;
		break;
	case HE_OP_CHANNEL_WIDTH_80P80_160:
		*pucChannelWidth = (uint8_t)CW_160MHZ;
		break;
	default:
		*pucChannelWidth = (uint8_t)CW_20_40MHZ;
		break;
	}

	/* Covert S1, S2 to VHT format */
	rlmModifyHE6GBwPara(prAdapter,
		*pucChannelWidth,
		ucChannelNum,
		pucCenterFreqS1,
		pucCenterFreqS2);

	/* 6G BW40, need to modify Sco to proper value */
	if (ucChannelWidth == HE_OP_CHANNEL_WIDTH_40
		&& *pucCenterFreqS1 != 0) {
		if (*pucCenterFreqS1 > ucChannelNum)
			*peSco = CHNL_EXT_SCA;
		else if (*pucCenterFreqS1 < ucChannelNum)
			*peSco = CHNL_EXT_SCB;
		else
			*peSco = CHNL_EXT_SCN;
	}
}

void rlmModifyHE6GBwPara(struct ADAPTER *prAdapter,
	uint8_t ucHe6gChannelWidth,
	uint8_t ucHe6gPrimaryChannel,
	uint8_t *pucHe6gChannelFrequencyS1,
	uint8_t *pucHe6gChannelFrequencyS2)
{
	uint8_t i = 0, ucS1Modify = 0;
	uint8_t ucS1Origin = *pucHe6gChannelFrequencyS1;
	uint8_t ucS2Origin = *pucHe6gChannelFrequencyS2;

	if (ucHe6gChannelWidth == CW_160MHZ) {
		if ((ucS1Origin != 0 && ucS2Origin != 0) &&
			(((ucS2Origin - ucS1Origin) == 8) ||
			 ((ucS1Origin - ucS2Origin) == 8))) {
			/* HE operating element sets central ch of bw80 at S1,
			 * set central ch of bw160 at S2.
			 */
			for (i = 0; i < 2; i++) {
				if (i == 0)
					ucS1Modify = ucS1Origin;
				else
					ucS1Modify = ucS2Origin;

				if ((ucS1Modify == 15) ||
					(ucS1Modify == 47) ||
					(ucS1Modify == 79) ||
					(ucS1Modify == 111) ||
					(ucS1Modify == 143) ||
					(ucS1Modify == 175) ||
					(ucS1Modify == 207))
					break;
			}
		}

		if (ucS1Modify == 0) {
			ucS1Modify = nicGetHe6gS1(prAdapter,
				ucHe6gPrimaryChannel, ucHe6gChannelWidth);

			DBGLOG(RLM, WARN,
				"S1/S2 for 6G BW160 is out of spec, S1[%d->%d] S2[%d->0]\n",
				ucS1Origin, ucS1Modify, ucS2Origin);
		}

		*pucHe6gChannelFrequencyS1 = ucS1Modify;
		*pucHe6gChannelFrequencyS2 = 0;
	} else if (ucHe6gChannelWidth == CW_80MHZ) {
		ucS1Modify = nicGetHe6gS1(prAdapter, ucHe6gPrimaryChannel,
			ucHe6gChannelWidth);

		if (ucS1Modify != ucS1Origin) {
			DBGLOG(RLM, WARN,
				"S1/S2 for 6G BW80 is out of spec, S1[%d->%d] S2[%d->0]\n",
				ucS1Origin, ucS1Modify, ucS2Origin);

			*pucHe6gChannelFrequencyS1 = ucS1Modify;
			*pucHe6gChannelFrequencyS2 = 0;
		}
	} else if (ucHe6gChannelWidth == CW_20_40MHZ) {
		if (ucS2Origin != 0) {
			DBGLOG(RLM, WARN,
				"S1/S2 for 6G BW20/40 is out of spec, S1[%d->0] S2[%d->0]\n",
				ucS1Origin, ucS2Origin);

			*pucHe6gChannelFrequencyS1 = 0;
			*pucHe6gChannelFrequencyS2 = 0;
		}
	}
}
#endif /* CFG_SUPPORT_WIFI_6G */

void rlmRevisePreferBandwidthNss(struct ADAPTER *prAdapter,
					uint8_t ucBssIndex,
					struct STA_RECORD *prStaRec)
{
	enum ENUM_CHANNEL_WIDTH eChannelWidth = CW_20_40MHZ;
	struct BSS_INFO *prBssInfo;

#define VHT_MCS_TX_RX_MAX_2SS BITS(2, 3)
#define VHT_MCS_TX_RX_MAX_2SS_SHIFT 2

#define AR_STA_2AC_MCS(prStaRec)                                               \
	(((prStaRec)->u2VhtRxMcsMap & VHT_MCS_TX_RX_MAX_2SS) >>                \
	 VHT_MCS_TX_RX_MAX_2SS_SHIFT)

#define AR_IS_STA_2SS_AC(prStaRec) ((AR_STA_2AC_MCS(prStaRec) != BITS(0, 1)))

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "prBssInfo is null\n");
		return;
	}

	eChannelWidth = prBssInfo->ucVhtChannelWidth;

	/*
	 * Prefer setting modification
	 * 80+80 1x1 and 80 2x2 have the same phy rate, choose the 80 2x2
	 */

	if (AR_IS_STA_2SS_AC(prStaRec)) {
		/*
		 * DBGLOG(RLM, WARN, "support 2ss\n");
		 */

		if ((eChannelWidth == CW_80P80MHZ &&
		     prBssInfo->ucVhtChannelFrequencyS2 != 0)) {
			DBGLOG(RLM, WARN, "support (2Nss) and (80+80)\n");
			DBGLOG(RLM, WARN,
			       "choose (2Nss) and (80) for Bss_info\n");
			prBssInfo->ucVhtChannelWidth = CW_80MHZ;
			prBssInfo->ucVhtChannelFrequencyS2 = 0;
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Revise operating BW by own maximum bandwidth capability
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmReviseMaxBw(struct ADAPTER *prAdapter, uint8_t ucBssIndex,
		    enum ENUM_CHNL_EXT *peExtend,
		    enum ENUM_CHANNEL_WIDTH *peChannelWidth, uint8_t *pucS1,
		    uint8_t *pucPrimaryCh)
{
	uint8_t ucMaxBandwidth = MAX_BW_80MHZ;
	uint8_t ucCurrentBandwidth = MAX_BW_20MHZ;
	uint8_t ucOffset = (MAX_BW_80MHZ - CW_80MHZ);
	struct BSS_INFO *prBssInfo;
	uint8_t ucS1Origin = *pucS1;
	enum ENUM_CHANNEL_WIDTH eChBwOrigin = *peChannelWidth;
	enum ENUM_CHNL_EXT eScoOrigin = *peExtend;
	enum ENUM_CHNL_EXT eScoModify;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	ucMaxBandwidth = cnmGetDbdcBwCapability(prAdapter, ucBssIndex);

	if (*peChannelWidth > CW_20_40MHZ) {
		/*case BW > 80 , 160 80P80 */
		ucCurrentBandwidth = (uint8_t)*peChannelWidth + ucOffset;
	} else {
		/*case BW20 BW40 */
		if (eScoOrigin != CHNL_EXT_SCN) {
			/*case BW40 */
			ucCurrentBandwidth = MAX_BW_40MHZ;
		}
	}

	if (ucCurrentBandwidth > ucMaxBandwidth) {
		if (ucMaxBandwidth <= MAX_BW_40MHZ) { /* BW20, BW40 */
			*peChannelWidth = CW_20_40MHZ;
		} else { /* BW80, BW160, BW80P80, BW320 */
			*peChannelWidth = (ucMaxBandwidth - ucOffset);

			*pucS1 = nicGetS1(prAdapter, prBssInfo->eBand,
				*pucPrimaryCh,
				*peChannelWidth);
		}
	}

	/* Revise SCO */
	eScoModify = rlmReviseSco(*peChannelWidth,
		*pucPrimaryCh, *pucS1,
		eScoOrigin, ucMaxBandwidth);

	if (eScoOrigin != eScoModify) {
		*peExtend = eScoModify;

		DBGLOG(RLM, INFO, "Change SCO[%d->%d]\n",
			eScoOrigin, eScoModify);
	}

	/* Revise S1 */
	if (*peExtend == 0 &&
	    *peChannelWidth == CW_20_40MHZ)
		*pucS1 = *pucPrimaryCh;

	if (eChBwOrigin != *peChannelWidth ||
	    ucS1Origin != *pucS1) {
		DBGLOG(RLM, INFO, "Change BW[%d->%d], S1[%d->%d]\n",
			eChBwOrigin, *peChannelWidth,
			ucS1Origin, *pucS1);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Revise SCO
 *
 * \param[in]
 *
 * \return Revised SCO
 */
/*----------------------------------------------------------------------------*/
enum ENUM_CHNL_EXT rlmReviseSco(
	enum ENUM_CHANNEL_WIDTH eChannelWidth,
	uint8_t ucPrimaryCh,
	uint8_t ucS1,
	enum ENUM_CHNL_EXT eScoOrigin,
	uint8_t ucMaxBandwidth)
{
	enum ENUM_CHNL_EXT eSCO = eScoOrigin;

	if (eChannelWidth == CW_20_40MHZ) {
		if (ucMaxBandwidth == MAX_BW_20MHZ)
			eSCO = CHNL_EXT_SCN;
	}

	return eSCO;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Fill VHT Operation Information(VHT BW, S1, S2) by BSS operating
 *  channel width
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmFillVhtOpInfoByBssOpBw(struct ADAPTER *prAdapter,
			       struct BSS_INFO *prBssInfo, uint8_t ucBssOpBw)
{
	if (!prBssInfo) {
		DBGLOG(RLM, WARN, "no bssinfo\n");
		return;
	}

	prBssInfo->ucVhtChannelWidth =
		rlmGetVhtOpBwByBssOpBw(ucBssOpBw);
	prBssInfo->ucVhtChannelFrequencyS1 = nicGetS1(
		prAdapter,
		prBssInfo->eBand,
		prBssInfo->ucPrimaryChannel,
		prBssInfo->ucVhtChannelWidth);
	prBssInfo->ucVhtChannelFrequencyS2 = 0;
}

void rlmParseMtkOui(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	struct BSS_INFO *prBssInfo, const uint8_t *pucIE)
{
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
	uint8_t *aucCapa = MTK_OUI_IE(pucIE)->aucCapability;
	const uint8_t *ie, *sub;
	uint16_t ie_len, ie_offset, sub_len, sub_offset;

	if (kalMemCmp(MTK_OUI_IE(pucIE)->aucOui,
		aucMtkOui, sizeof(aucMtkOui)))
		return;
	else if (MTK_OUI_IE(pucIE)->ucLength <
		ELEM_MIN_LEN_MTK_OUI)
		return;

#if (CFG_SUPPORT_TWT_HOTSPOT_AC == 1)
	prStaRec->ucTWTHospotSupport =
		((aucCapa[0] &
		MTK_SYNERGY_CAP_SUPPORT_TWT_HOTSPOT_AC) ==
		MTK_SYNERGY_CAP_SUPPORT_TWT_HOTSPOT_AC);
#endif

	prStaRec->ucGcCsaSupported =
		((aucCapa[1] &
		MTK_SYNERGY_CAP_SUPPORT_GC_CSA) ==
		MTK_SYNERGY_CAP_SUPPORT_GC_CSA) &&
		prAdapter->rWifiVar.fgP2pGcCsa;

	DBGLOG(RLM, LOUD, "GcCsa: %d\n", prStaRec->ucGcCsaSupported);
	if (!(aucCapa[0] & MTK_SYNERGY_CAP_SUPPORT_TLV))
		return;

	ie = MTK_OUI_IE(pucIE)->aucInfoElem;
	ie_len = IE_LEN(pucIE) - 7;

	IE_FOR_EACH(ie, ie_len, ie_offset) {
		if (IE_ID(ie) == MTK_OUI_ID_PRE_WIFI7) {
			struct IE_MTK_PRE_WIFI7 *prPreWifi7 =
				(struct IE_MTK_PRE_WIFI7 *)ie;

			DBGLOG(RLM, TRACE, "MTK_OUI_PRE_WIFI7 %d.%d",
				prPreWifi7->ucVersion1, prPreWifi7->ucVersion0);
			DBGLOG_MEM8(RLM, TRACE, ie, IE_SIZE(ie));

			sub = prPreWifi7->aucInfoElem;
			sub_len = IE_LEN(prPreWifi7) - 2;

			IE_FOR_EACH(sub, sub_len, sub_offset) {
#if (CFG_SUPPORT_802_11BE == 1)
				if (IE_ID_EXT(sub) == ELEM_EXT_ID_EHT_CAPS)
					ehtRlmRecCapInfo(prAdapter,
						prStaRec, sub);

				if (IE_ID_EXT(sub) == ELEM_EXT_ID_EHT_OP)
					ehtRlmRecOperation(prAdapter, prStaRec,
						prBssInfo, sub);
#endif
			}
		}
#if ((CFG_SUPPORT_MLR_V2 == 1) || (CFG_SUPPORT_BALANCE_MLR == 1))
		if (IE_ID(ie) == MTK_OUI_ID_MLR) {
			struct IE_MTK_MLR *prMLR = (struct IE_MTK_MLR *)ie;

			MLR_DBGLOG(prAdapter, RLM, INFO,
				"MLR beacon or assoc resp - BSSID:" MACSTR
				" ,MLRIE[0x%02x, 0x%02x, 0x%02x] StaRec[0x%02x]",
					MAC2STR(prBssInfo->aucBSSID),
					prMLR->ucId,
					prMLR->ucLength,
					prMLR->ucLRBitMap,
					prStaRec->ucMlrSupportBitmap
				);
		}
#endif
	}
}

void rlmParseMtkOuiForAssocResp(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, struct BSS_INFO *prBssInfo,
	const uint8_t *pucIE)
{
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
	uint8_t *aucCapa = MTK_OUI_IE(pucIE)->aucCapability;
	const uint8_t *ie;
	uint16_t ie_len, ie_offset;

	if (kalMemCmp(MTK_OUI_IE(pucIE)->aucOui,
		aucMtkOui, sizeof(aucMtkOui)))
		return;
	else if (MTK_OUI_IE(pucIE)->ucLength <
		ELEM_MIN_LEN_MTK_OUI)
		return;

	if (!(aucCapa[0] & MTK_SYNERGY_CAP_SUPPORT_TLV))
		return;

	ie = MTK_OUI_IE(pucIE)->aucInfoElem;
	ie_len = IE_LEN(pucIE) - ELEM_MIN_LEN_MTK_OUI;

	IE_FOR_EACH(ie, ie_len, ie_offset) {
#if ((CFG_SUPPORT_MLR_V2 == 1) || (CFG_SUPPORT_BALANCE_MLR == 1))
		if (IE_ID(ie) == MTK_OUI_ID_MLR) {
			struct IE_MTK_MLR *prMLR = (struct IE_MTK_MLR *)ie;
			uint8_t ucBcnBitmap = prStaRec->ucMlrSupportBitmap;

			if (prAdapter->u4MlrSupportBitmap == MLR_MODE_MLR_V1) {
				DBGLOG(RLM, INFO,
					"MLR assoc resp - Skip because DUT Cap is MLR_V1\n");
				continue;
			}

			if (((prMLR->ucLRBitMap & prAdapter->u4MlrSupportBitmap)
				== MLR_MODE_MLR_V1)
				&& !MLR_BAND_IS_SUPPORT(prBssInfo->eBand))
				prStaRec->ucMlrSupportBitmap =
					MLR_MODE_NOT_SUPPORT;
			else
				prStaRec->ucMlrSupportBitmap =
					prMLR->ucLRBitMap;

		DBGLOG(RLM, INFO,
			"MLR assoc resp - BSSID:" MACSTR
			" MLRIE LRBitmap[0x%02x], Band[%d], Peer's Beacon[0x%02x], DUT Cap[0x%02x], Final Nego[0x%02x]",
			MAC2STR(prBssInfo->aucBSSID),
			prMLR->ucLRBitMap,
			prBssInfo->eBand,
			ucBcnBitmap,
			prAdapter->u4MlrSupportBitmap,
			prStaRec->ucMlrSupportBitmap);

		}
#endif
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function should be invoked to update parameters of associated AP.
 *        (Association response and Beacon)
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static uint8_t rlmRecIeInfoForClient(struct ADAPTER *prAdapter,
				     struct BSS_INFO *prBssInfo,
				     const uint8_t *pucIE, uint16_t u2IELength)
{
	uint16_t u2Offset;
	struct STA_RECORD *prStaRec;
	struct IE_HT_CAP *prHtCap;
	struct IE_HT_OP *prHtOp = NULL;
	struct IE_OBSS_SCAN_PARAM *prObssScnParam;
	uint8_t ucERP, ucPrimaryChannel;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#if CFG_SUPPORT_QUIET && 0
	u_int8_t fgHasQuietIE = FALSE;
#endif
	u_int8_t IsfgHtCapChange = FALSE;

#if CFG_SUPPORT_RXSMM_ALLOWLIST
	u_int8_t fgRxsmmEnable = FALSE;
#endif

#if CFG_SUPPORT_802_11AC
	struct IE_VHT_OP *prVhtOp;
	struct IE_VHT_CAP *prVhtCap = NULL;
	struct IE_OP_MODE_NOTIFICATION *prOPNotif;
	uint8_t fgHasOPModeIE = FALSE;
	uint8_t fgHasNewOPModeIE = FALSE;
	uint8_t fgUpdateToFWForDiffOPModeIE = TRUE;
	uint8_t ucVhtOpModeChannelWidth = 0;
	uint8_t ucVhtOpModeRxNss = 0;
	uint8_t ucMaxBwAllowed;
	uint8_t ucInitVhtOpMode = 0;
#endif

#if (CFG_SUPPORT_802_11AX == 1)
	u_int8_t fgIsRx1ss = FALSE;
	uint16_t u2HeRxMcsMapAssoc;
#if (CFG_SUPPORT_WIFI_6G == 1)
	struct _IE_HE_6G_BAND_CAP_T *prHe6gBandCap = NULL;
	u_int8_t IsfgHe6gBandCapChange = FALSE;
#endif
#endif /* CFG_SUPPORT_802_11AX == 1 */
#if CFG_SUPPORT_DFS
	struct IE_CHANNEL_SWITCH *prCSAIE;
	struct IE_EX_CHANNEL_SWITCH *prExCSAIE;
	struct IE_MAX_CHANNEL_SWITCH_TIME *prMaxCSATimeIE;
	struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams;
	uint8_t ucCurrentCsaCount;
	struct IE_SECONDARY_OFFSET *prSecondaryOffsetIE;
#endif
	const uint8_t *pucDumpIE;
	uint8_t fgDomainValid = FALSE;
	enum ENUM_CHANNEL_WIDTH eChannelWidth = CW_20_40MHZ;
	uint8_t ucHtOpChannelFrequencyS3 = 0;

#if (CFG_SUPPORT_TWT_HOTSPOT_AC == 1)
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
#endif

#if (CFG_SUPPORT_BTWT == 1)
	uint8_t fgBtwtIeFound = FALSE;
#endif
	const uint8_t *pucIEOpmode = NULL;

#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
	/* Record the old BssColorInfo before parsing beacon */
	uint8_t ucOldBssColorInfo = 0;
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(pucIE);

	DBGLOG(RLM, LOUD, "Dump beacon content from FW\n");
	DBGLOG_MEM8(RLM, LOUD, pucIE, u2IELength);

	prStaRec = prBssInfo->prStaRecOfAP;
	if (!prStaRec)
		return 0;

	prBssInfo->fgUseShortPreamble = prBssInfo->fgIsShortPreambleAllowed;
	ucPrimaryChannel = 0;
	prObssScnParam = NULL;
	ucMaxBwAllowed = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);
	pucDumpIE = pucIE;
#if CFG_SUPPORT_DFS
	prCSAParams = &prBssInfo->CSAParams;
	ucCurrentCsaCount = MAX_CSA_COUNT;
#endif

#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
	/* Reset if receive announcement */
	prBssInfo->ucColorAnnouncement = FALSE;
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */

	/* Note: HT-related members in staRec may not be zero before, so
	 *       if following IE does not exist, they are still not zero.
	 *       These HT-related parameters are valid only when the
	 * corresponding
	 *       BssInfo supports 802.11n, i.e., RLM_NET_IS_11N()
	 */
	IE_FOR_EACH(pucIE, u2IELength, u2Offset)
	{
		switch (IE_ID(pucIE)) {
		case ELEM_ID_HT_CAP:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (prBssInfo->eBand == BAND_6G) {
				DBGLOG(SCN, WARN, "Ignore HT CAP IE at 6G\n");
				break;
			}
#endif
			if (!RLM_NET_IS_11N(prBssInfo) ||
			    IE_LEN(pucIE) != (sizeof(struct IE_HT_CAP) - 2))
				break;
			prHtCap = (struct IE_HT_CAP *)pucIE;
			prStaRec->ucMcsSet =
				prHtCap->rSupMcsSet.aucRxMcsBitmask[0];
			prStaRec->fgSupMcs32 =
				(prHtCap->rSupMcsSet.aucRxMcsBitmask[32 / 8] &
				 BIT(0))
					? TRUE
					: FALSE;

			kalMemCopy(
				prStaRec->aucRxMcsBitmask,
				prHtCap->rSupMcsSet.aucRxMcsBitmask,
				/*SUP_MCS_RX_BITMASK_OCTET_NUM */
				sizeof(prStaRec->aucRxMcsBitmask));

			prStaRec->u2RxHighestSupportedRate =
				prHtCap->rSupMcsSet.u2RxHighestSupportedRate;
			prStaRec->u4TxRateInfo =
				prHtCap->rSupMcsSet.u4TxRateInfo;

			if ((prStaRec->u2HtCapInfo &
			     HT_CAP_INFO_SM_POWER_SAVE) !=
			    (prHtCap->u2HtCapInfo & HT_CAP_INFO_SM_POWER_SAVE))
				/* Purpose : To detect SMPS change */
				IsfgHtCapChange = TRUE;

			prStaRec->u2HtCapInfo = prHtCap->u2HtCapInfo;
			/* Set LDPC Tx capability */
			if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxLdpc))
				prStaRec->u2HtCapInfo |= HT_CAP_INFO_LDPC_CAP;
			else if (IS_FEATURE_DISABLED(prWifiVar->ucTxLdpc))
				prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_LDPC_CAP;

			/* Set STBC Tx capability */
			if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxStbc))
				prStaRec->u2HtCapInfo |= HT_CAP_INFO_RX_STBC;
			else if (IS_FEATURE_DISABLED(prWifiVar->ucTxStbc))
				prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_RX_STBC;

			/* Set Short GI Tx capability */
			if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxShortGI)) {
				prStaRec->u2HtCapInfo |=
					HT_CAP_INFO_SHORT_GI_20M;
				prStaRec->u2HtCapInfo |=
					HT_CAP_INFO_SHORT_GI_40M;
			} else if (IS_FEATURE_DISABLED(
					   prWifiVar->ucTxShortGI)) {
				prStaRec->u2HtCapInfo &=
					~HT_CAP_INFO_SHORT_GI_20M;
				prStaRec->u2HtCapInfo &=
					~HT_CAP_INFO_SHORT_GI_40M;
			}

			/* Set HT Greenfield Tx capability */
			if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxGf))
				prStaRec->u2HtCapInfo |= HT_CAP_INFO_HT_GF;
			else if (IS_FEATURE_DISABLED(prWifiVar->ucTxGf))
				prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_HT_GF;

			prStaRec->ucAmpduParam = prHtCap->ucAmpduParam;
			prStaRec->u2HtExtendedCap = prHtCap->u2HtExtendedCap;
			prStaRec->u4TxBeamformingCap =
				prHtCap->u4TxBeamformingCap;
			prStaRec->ucAselCap = prHtCap->ucAselCap;
			break;

		case ELEM_ID_HT_OP:
			if (IE_LEN(pucIE) != (sizeof(struct IE_HT_OP) - 2))
				break;

			prHtOp = (struct IE_HT_OP *)pucIE;
			rlmRecHtOpForClient(prAdapter, prHtOp,
				prBssInfo, &ucPrimaryChannel);

			break;

#if CFG_SUPPORT_802_11AC
		case ELEM_ID_VHT_CAP:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (prBssInfo->eBand == BAND_6G) {
				DBGLOG(SCN, WARN, "Ignore VHT CAP IE at 6G\n");
				break;
			}
#endif
			if (!RLM_NET_IS_11AC(prBssInfo) ||
			    IE_LEN(pucIE) != (sizeof(struct IE_VHT_CAP) - 2))
				break;

			prVhtCap = (struct IE_VHT_CAP *)pucIE;

			prStaRec->u4VhtCapInfo = prVhtCap->u4VhtCapInfo;
			/* Set Tx LDPC capability */
			if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxLdpc))
				prStaRec->u4VhtCapInfo |= VHT_CAP_INFO_RX_LDPC;
			else if (IS_FEATURE_DISABLED(prWifiVar->ucTxLdpc))
				prStaRec->u4VhtCapInfo &= ~VHT_CAP_INFO_RX_LDPC;

			/* Set Tx STBC capability */
			if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxStbc))
				prStaRec->u4VhtCapInfo |=
					VHT_CAP_INFO_RX_STBC_MASK;
			else if (IS_FEATURE_DISABLED(prWifiVar->ucTxStbc))
				prStaRec->u4VhtCapInfo &=
					~VHT_CAP_INFO_RX_STBC_MASK;

			/* Set Tx TXOP PS capability */
			if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxopPsTx))
				prStaRec->u4VhtCapInfo |=
					VHT_CAP_INFO_VHT_TXOP_PS;
			else if (IS_FEATURE_DISABLED(prWifiVar->ucTxopPsTx))
				prStaRec->u4VhtCapInfo &=
					~VHT_CAP_INFO_VHT_TXOP_PS;

			/* Set Tx Short GI capability */
			if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxShortGI)) {
				prStaRec->u4VhtCapInfo |=
					VHT_CAP_INFO_SHORT_GI_80;
				prStaRec->u4VhtCapInfo |=
					VHT_CAP_INFO_SHORT_GI_160_80P80;
			} else if (IS_FEATURE_DISABLED(
					   prWifiVar->ucTxShortGI)) {
				prStaRec->u4VhtCapInfo &=
					~VHT_CAP_INFO_SHORT_GI_80;
				prStaRec->u4VhtCapInfo &=
					~VHT_CAP_INFO_SHORT_GI_160_80P80;
			}

			prStaRec->u2VhtRxMcsMap =
				prVhtCap->rVhtSupportedMcsSet.u2RxMcsMap;
			prStaRec->u2VhtRxMcsMapAssoc =
				prStaRec->u2VhtRxMcsMap;

			prStaRec->u2VhtRxHighestSupportedDataRate =
				prVhtCap->rVhtSupportedMcsSet
					.u2RxHighestSupportedDataRate;
			prStaRec->u2VhtTxMcsMap =
				prVhtCap->rVhtSupportedMcsSet.u2TxMcsMap;
			prStaRec->u2VhtTxHighestSupportedDataRate =
				prVhtCap->rVhtSupportedMcsSet
					.u2TxHighestSupportedDataRate;

			break;

		case ELEM_ID_VHT_OP:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (prBssInfo->eBand == BAND_6G) {
				DBGLOG(SCN, WARN, "Ignore VHT OP IE at 6G\n");
				break;
			}
#endif
			if (!RLM_NET_IS_11AC(prBssInfo) ||
			    IE_LEN(pucIE) != (sizeof(struct IE_VHT_OP) - 2))
				break;

			prVhtOp = (struct IE_VHT_OP *)pucIE;

			/*Backup peer VHT OpInfo*/
			prStaRec->ucVhtOpChannelWidth =
				prVhtOp->ucVhtOperation[0];
			prStaRec->ucVhtOpChannelFrequencyS1 =
				prVhtOp->ucVhtOperation[1];
			prStaRec->ucVhtOpChannelFrequencyS2 =
				prVhtOp->ucVhtOperation[2];
			ucHtOpChannelFrequencyS3 =
				HT_GET_OP_INFO2_CH_CENTER_FREQ_SEG2(
					prStaRec->u2HtPeerOpInfo2);

			rlmModifyVhtBwPara(&prStaRec->ucVhtOpChannelFrequencyS1,
					   &prStaRec->ucVhtOpChannelFrequencyS2,
					   ucHtOpChannelFrequencyS3,
					   &prStaRec->ucVhtOpChannelWidth);

			prBssInfo->ucVhtChannelWidth =
				prVhtOp->ucVhtOperation[0];
			prBssInfo->ucVhtChannelFrequencyS1 =
				prVhtOp->ucVhtOperation[1];
			prBssInfo->ucVhtChannelFrequencyS2 =
				prVhtOp->ucVhtOperation[2];
			prBssInfo->u2VhtBasicMcsSet = prVhtOp->u2VhtBasicMcsSet;
			ucHtOpChannelFrequencyS3 =
				HT_GET_OP_INFO2_CH_CENTER_FREQ_SEG2(
					prBssInfo->u2HtOpInfo2);

			rlmModifyVhtBwPara(&prBssInfo->ucVhtChannelFrequencyS1,
					   &prBssInfo->ucVhtChannelFrequencyS2,
					   ucHtOpChannelFrequencyS3,
					   &prBssInfo->ucVhtChannelWidth);

			/* Revise by own OP BW if needed */
			if ((prBssInfo->fgIsOpChangeChannelWidth) &&
			    (rlmGetVhtOpBwByBssOpBw(
				     prBssInfo->ucOpChangeChannelWidth) <
			     prBssInfo->ucVhtChannelWidth)) {
				rlmFillVhtOpInfoByBssOpBw(
					prAdapter, prBssInfo,
					prBssInfo->ucOpChangeChannelWidth);
			}

			break;
		case ELEM_ID_OP_MODE:
			/* Check OP mode IE at last */
			pucIEOpmode = pucIE;

			break;
#endif
		case ELEM_ID_20_40_BSS_COEXISTENCE:
			if (!RLM_NET_IS_11N(prBssInfo))
				break;
			/* To do: store if scanning exemption grant to BssInfo
			 */
			break;

		case ELEM_ID_OBSS_SCAN_PARAMS:
			if (!RLM_NET_IS_11N(prBssInfo) ||
			    IE_LEN(pucIE) !=
				    (sizeof(struct IE_OBSS_SCAN_PARAM) - 2))
				break;
			/* Store OBSS parameters to BssInfo */
			prObssScnParam = (struct IE_OBSS_SCAN_PARAM *)pucIE;
			break;

		case ELEM_ID_EXTENDED_CAP:
			if (!RLM_NET_IS_11N(prBssInfo))
				break;
			/* To do: store extended capability (PSMP, coexist) to
			 * BssInfo
			 */
			break;

		case ELEM_ID_ERP_INFO:
			if (IE_LEN(pucIE) != (sizeof(struct IE_ERP) - 2) ||
			    prBssInfo->eBand != BAND_2G4)
				break;
			ucERP = ERP_INFO_IE(pucIE)->ucERP;
			prBssInfo->fgErpProtectMode =
				(ucERP & ERP_INFO_USE_PROTECTION) ? TRUE
								  : FALSE;

			if (ucERP & ERP_INFO_BARKER_PREAMBLE_MODE)
				prBssInfo->fgUseShortPreamble = FALSE;
			break;

		case ELEM_ID_DS_PARAM_SET:
			if (IE_LEN(pucIE) == ELEM_MAX_LEN_DS_PARAMETER_SET)
				ucPrimaryChannel =
					DS_PARAM_IE(pucIE)->ucCurrChnl;
			break;
#if CFG_SUPPORT_DFS
		case ELEM_ID_CH_SW_ANNOUNCEMENT:
			if (IE_LEN(pucIE) !=
			    (sizeof(struct IE_CHANNEL_SWITCH) - 2))
				break;

			prCSAIE = (struct IE_CHANNEL_SWITCH *)pucIE;

			if (prBssInfo->ucPrimaryChannel ==
					prCSAIE->ucNewChannelNum) {
				DBGLOG(RLM, WARN,
					"[CSA] BSS: " MACSTR
					" already at channel %u\n",
					MAC2STR(prBssInfo->aucBSSID),
					prCSAIE->ucNewChannelNum);
				break;
			}

#if CFG_SUPPORT_ROAMING
			if (IS_BSS_AIS(prBssInfo) &&
			    roamingFsmCheckIfRoaming(
				    prAdapter, prBssInfo->ucBssIndex)) {
				DBGLOG(RLM, INFO,
					"Ignore csa beacon frame when roaming\n");
				break;
			}
#endif

			/* Mode 1 implies that addressed AP is advised to
			 * transmit no further frames on current channel
			 * until the scheduled channel switch.
			 */
			DBGLOG(RLM, INFO,
			       "[CSA] Bss = %d Count = %d Mode = %d\n",
			       prBssInfo->ucBssIndex,
			       prCSAIE->ucChannelSwitchCount,
			       prCSAIE->ucChannelSwitchMode);
			prCSAParams->ucCsaNewCh = prCSAIE->ucNewChannelNum;
			ucCurrentCsaCount = prCSAIE->ucChannelSwitchCount;

			if (prCSAIE->ucChannelSwitchMode == 1) {
				/* Mode 1: Need to stop data
				 * transmission immediately
				 */
				if (!prCSAParams->fgHasStopTx) {
					prCSAParams->fgHasStopTx = TRUE;
					kalIndicateAllQueueTxAllowed(
						prAdapter->prGlueInfo,
						prBssInfo->ucBssIndex,
						FALSE);
					/* AP */
					qmSetStaRecTxAllowed(prAdapter,
						prStaRec,
						FALSE);
					DBGLOG(RLM, EVENT,
						"[CSA] TxAllowed = FALSE\n");
				}
			}

			prCSAParams->ucCsaMode = prCSAIE->ucChannelSwitchMode;
			if (prCSAParams->ucCsaMode > MODE_NUM) {
				DBGLOG(RLM, WARN,
					"[CSA] invalid ChannelSwitchMode = %d, follow mode 0\n",
					prCSAParams->ucCsaMode);
				prCSAParams->ucCsaMode = MODE_ALLOW_TX;
			}

#ifdef CFG_DFS_CHSW_FORCE_BW20
			if (RLM_NET_IS_11AC(prBssInfo)) {
				DBGLOG(RLM, INFO,
					"Send Operation Action Frame");
				rlmSendOpModeNotificationFrame(
					prAdapter, prStaRec,
					VHT_OP_MODE_CHANNEL_WIDTH_20,
					1);
			} else
				DBGLOG(RLM, INFO,
					"Skip Send Operation Action Frame");
#endif
			break;

		case ELEM_ID_EX_CH_SW_ANNOUNCEMENT:
			if (IE_LEN(pucIE) !=
			    (sizeof(struct IE_EX_CHANNEL_SWITCH) - 2))
				break;

			prExCSAIE = (struct IE_EX_CHANNEL_SWITCH *)pucIE;

			if (prBssInfo->ucPrimaryChannel ==
					prExCSAIE->ucNewChannelNum) {
				DBGLOG(RLM, WARN,
					"[ECSA] BSS: " MACSTR
					" already at channel %u\n",
					MAC2STR(prBssInfo->aucBSSID),
					prExCSAIE->ucNewChannelNum);
				break;
			}

#if CFG_SUPPORT_ROAMING
			if (IS_BSS_AIS(prBssInfo) &&
			    roamingFsmCheckIfRoaming(
				    prAdapter, prBssInfo->ucBssIndex)) {
				DBGLOG(RLM, INFO,
					"Ignore csa beacon frame when roaming\n");
				break;
			}
#endif

			prCSAParams->ucCsaNewCh = prExCSAIE->ucNewChannelNum;
			ucCurrentCsaCount = prExCSAIE->ucChannelSwitchCount;
			rlmProcessExCsaIE(prAdapter, prStaRec,
				prCSAParams,
				prExCSAIE->ucChannelSwitchMode,
				prExCSAIE->ucNewOperatingClass,
				prExCSAIE->ucNewChannelNum,
				prExCSAIE->ucChannelSwitchCount);
			break;

		case ELEM_ID_SCO:
			if (IE_LEN(pucIE) !=
			    (sizeof(struct IE_SECONDARY_OFFSET) - 2))
				break;

			prSecondaryOffsetIE =
				(struct IE_SECONDARY_OFFSET *)pucIE;
			DBGLOG(RLM, INFO, "[CSA] SCO [%d]->[%d]\n",
				prBssInfo->eBssSCO,
				prSecondaryOffsetIE->ucSecondaryOffset);
			prCSAParams->eSco = (enum ENUM_CHNL_EXT)
					prSecondaryOffsetIE->ucSecondaryOffset;
			break;
#endif

#if CFG_SUPPORT_QUIET && 0
		/* Note: RRM code should be moved to independent RRM function by
		 *       component design rule. But we attach it to RLM
		 * temporarily
		 */
		case ELEM_ID_QUIET:
			rrmQuietHandleQuietIE(prBssInfo,
					      (struct IE_QUIET *)pucIE);
			fgHasQuietIE = TRUE;
			break;
#endif

#if (CFG_SUPPORT_BTWT == 1)
		case ELEM_ID_TWT:
			heRlmRecBTWTparams(prAdapter, prStaRec, pucIE);
			fgBtwtIeFound = TRUE;
			break;
#endif

#if CFG_SUPPORT_DFS
		case ELEM_ID_CH_SW_WRAPPER: {
			const uint8_t *sub;
			uint16_t sub_len, sub_offset;

			DBGLOG(RLM, INFO, "[CSA] Channel switch wrapper\n");

			sub = &pucIE[2];
			sub_len = IE_LEN(pucIE);

			IE_FOR_EACH(sub, sub_len, sub_offset) {

				if (IE_ID(sub) ==
				    ELEM_ID_WIDE_BAND_CHANNEL_SWITCH) {
					struct IE_WIDE_BAND_CHANNEL *prWBC;

					DBGLOG(RLM, INFO,
					       "[CSA] ELEM_ID_WIDE_BAND_CHANNEL_SWITCH\n");
					prWBC =
					     (struct IE_WIDE_BAND_CHANNEL *)sub;
					prCSAParams->ucVhtBw =
						prWBC->ucNewChannelWidth;
					prCSAParams->ucVhtS1 =
						prWBC->ucChannelS1;
					prCSAParams->ucVhtS2 =
						prWBC->ucChannelS2;

					rlmModifyVhtBwPara(
						&prCSAParams->ucVhtS1,
						&prCSAParams->ucVhtS2,
						0,
						&prCSAParams->ucVhtBw);

					DBGLOG(RLM, INFO,
					       "[CSA] BW=%d, s1=%d, s2=%d\n",
					       prCSAParams->ucVhtBw,
					       prCSAParams->ucVhtS1,
					       prCSAParams->ucVhtS2);
				}
			}
		}
			break;
#endif

		case ELEM_ID_RESERVED:
#if CFG_SUPPORT_DFS
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_MAX_CH_SW_TIME) {
				uint32_t u4MaxSwitchTime = 0;

				if (IE_SIZE(pucIE) !=
				    sizeof(struct IE_MAX_CHANNEL_SWITCH_TIME))
					break;

				prMaxCSATimeIE =
				    (struct IE_MAX_CHANNEL_SWITCH_TIME *) pucIE;
				WLAN_GET_FIELD_24(
					&prMaxCSATimeIE->ucChannelSwitchTime[0],
					&u4MaxSwitchTime);
				prCSAParams->u4MaxSwitchTime =
					TU_TO_MSEC(u4MaxSwitchTime);
				DBGLOG(RLM, INFO,
					"[CSA] Max switch time %d in TU, %d in MSEC\n",
					u4MaxSwitchTime,
					prCSAParams->u4MaxSwitchTime);
			}
#endif
#if (CFG_SUPPORT_802_11AX == 1)
			if (fgEfuseCtrlAxOn != 1)
				break;
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_CAP)
				heRlmRecHeCapInfo(prAdapter,
					prStaRec, pucIE);
			else if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_OP) {
#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
				/*
				 * Record the old BssColorInfo value
				 * before parsing beacon.
				 */
				ucOldBssColorInfo = prBssInfo->ucBssColorInfo;
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */
				heRlmRecHeOperation(prAdapter,
					prBssInfo, pucIE);
#if (CFG_SUPPORT_WIFI_6G == 1)
				{
				uint32_t u4Offset = OFFSET_OF(
					struct _IE_HE_OP_T,
					aucVarInfo[0]);
				struct _6G_OPER_INFOR_T *pr6gOperInfor = NULL;

				if (prBssInfo->fgIsHE6GPresent) {
					if (prBssInfo->fgIsCoHostedBssPresent)
						u4Offset += sizeof(uint8_t);

					pr6gOperInfor =
						(struct _6G_OPER_INFOR_T *)
						(((uint8_t *) pucIE)+
								u4Offset);
					ucPrimaryChannel =
						pr6gOperInfor->
						ucPrimaryChannel;

					prBssInfo->ucVhtChannelFrequencyS1 =
						pr6gOperInfor->
						ucChannelCenterFreqSeg0;

					prBssInfo->ucVhtChannelFrequencyS2 =
						pr6gOperInfor->
						ucChannelCenterFreqSeg1;

					prBssInfo->eBssSCO = CHNL_EXT_SCN;

					rlmTransferHe6gOpInfor(prAdapter,
						ucPrimaryChannel,
						(uint8_t)pr6gOperInfor->
						rControl.bits.ChannelWidth,
						&prBssInfo->
							ucVhtChannelWidth,
						&prBssInfo->
							ucVhtChannelFrequencyS1,
						&prBssInfo->
							ucVhtChannelFrequencyS2,
						&prBssInfo->eBssSCO);
				}
				}
#endif /* CFG_SUPPORT_WIFI_6G */
			}
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (IE_ID_EXT(pucIE) ==
				ELEM_EXT_ID_HE_6G_BAND_CAP) {
				if (!RLM_NET_IS_11AX(prBssInfo) ||
					(prBssInfo->eBand != BAND_6G) ||
					IE_LEN(pucIE) != (sizeof
					(struct _IE_HE_6G_BAND_CAP_T)-2))
					break;

				prHe6gBandCap =
					(struct _IE_HE_6G_BAND_CAP_T *)pucIE;

				if ((prStaRec->u2He6gBandCapInfo &
					HE_6G_CAP_INFO_SM_POWER_SAVE) !=
					(prHe6gBandCap->u2CapInfo &
						HE_6G_CAP_INFO_SM_POWER_SAVE))
					/* Purpose : To detect SMPS change */
					IsfgHe6gBandCapChange = TRUE;

				prStaRec->u2He6gBandCapInfo =
					prHe6gBandCap->u2CapInfo;
			}
#endif /* CFG_SUPPORT_WIFI_6G */
#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
			else if (IE_ID_EXT(pucIE) ==
				ELEM_EXT_ID_BSS_COLOR_CHANGE) {
				heRlmRecBssColorChangeAnnouncement(prAdapter,
					prBssInfo, pucIE);
			}
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */
#if (CFG_SUPPORT_802_11BE == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_EHT_CAPS)
				ehtRlmRecCapInfo(prAdapter, prStaRec, pucIE);
			else if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_EHT_OP)
				ehtRlmRecOperation(prAdapter, prStaRec,
					prBssInfo, pucIE);
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_MLD &&
			    BE_IS_ML_CTRL_TYPE(pucIE, ML_CTRL_TYPE_RECONFIG))
				mldCheckApRemoval(prAdapter, prStaRec, pucIE);

#if (CFG_SUPPORT_802_11BE_T2LM == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_TID2LNK_MAP)
				t2lmParseT2LMIE(prAdapter, prStaRec, pucIE);
#endif /* CFG_SUPPORT_802_11BE_T2LM */
#endif /* CFG_SUPPORT_802_11BE_MLO */
#endif /* CFG_SUPPORT_802_11AX */
			break;

		case ELEM_ID_VENDOR:
			rlmParseMtkOui(prAdapter, prStaRec, prBssInfo, pucIE);
#if CFG_SUPPORT_RXSMM_ALLOWLIST
			if (rlmParseCheckRxsmmOuiIE(prAdapter,
				pucIE, &fgRxsmmEnable))
				prStaRec->fgRxsmmEnable =
					(fgRxsmmEnable) ? (fgRxsmmEnable) :
						(prStaRec->fgRxsmmEnable);

			DBGLOG(RLM, TRACE, "RxSMM: STAREC enable = %d\n",
				prStaRec->fgRxsmmEnable);
#endif
			break;
		default:
			break;
		} /* end of switch */
	}	 /* end of IE_FOR_EACH */

#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
	/*
	 * If AP NOT send BssColorChangeAnnouncement
	 * and NOT change "disabled" bit,
	 * DO NOT change BssColorInfo in (struct)BssInfo.
	 *
	 * When connect, BssColorInfo change from 0x00 to New value.
	 * So, filter this case.
	 */
	if ((prBssInfo->ucColorAnnouncement == FALSE) &&
		((prBssInfo->ucBssColorInfo &
		(HE_OP_BSSCOLOR_PARTIAL_BSS_COLOR |
			HE_OP_BSSCOLOR_BSS_COLOR_DISABLE))
		== (ucOldBssColorInfo &
		(HE_OP_BSSCOLOR_PARTIAL_BSS_COLOR |
			HE_OP_BSSCOLOR_BSS_COLOR_DISABLE))) &&
		((ucOldBssColorInfo & HE_OP_BSSCOLOR_BSS_COLOR_MASK) != 0))
		prBssInfo->ucBssColorInfo = ucOldBssColorInfo;
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */

	if (pucIEOpmode != NULL) {
		switch (IE_ID(pucIEOpmode)) {
		case ELEM_ID_OP_MODE:
			DBGLOG(RLM, TRACE, "Check OP mode IE at last\n");
			if (IE_LEN(pucIEOpmode) !=
				(sizeof(struct IE_OP_MODE_NOTIFICATION) -
					 2)) {
				DBGLOG(RLM, WARN,
					"OP mode IE len check fail\n");
				break;
			}

			if (!(RLM_NET_IS_11AC(prBssInfo)
#if (CFG_SUPPORT_WIFI_6G == 1)
				|| (prBssInfo->eBand == BAND_6G &&
					RLM_NET_IS_11AX(prBssInfo))
#endif
			))
				break;

			prOPNotif =
			(struct IE_OP_MODE_NOTIFICATION *) pucIEOpmode;

			/* NOTE: An AP always sets this field to 0,
			 * so break it if this bit is set.
			 */
			if ((prOPNotif->ucOpMode & VHT_OP_MODE_RX_NSS_TYPE)
				== VHT_OP_MODE_RX_NSS_TYPE)
				break;
			fgHasOPModeIE = TRUE;

			/* Check BIT(2) is BW80P80_160.
			 * Refactor ucOpMode with ucVhtOpMode format.
			 */
			if (prOPNotif->ucOpMode &
				VHT_OP_MODE_CHANNEL_WIDTH_80P80_160) {
				prOPNotif->ucOpMode &=
					~(VHT_OP_MODE_CHANNEL_WIDTH |
					VHT_OP_MODE_CHANNEL_WIDTH_80P80_160);
				prOPNotif->ucOpMode |=
					(VHT_OP_MODE_CHANNEL_WIDTH_160_80P80 &
					VHT_OP_MODE_CHANNEL_WIDTH);
			}

			/* Same OP mode, no need to update.
			 * Let the further flow not to update VhtOpMode.
			 */
			if (prStaRec->ucVhtOpMode == prOPNotif->ucOpMode) {
				ucInitVhtOpMode = prStaRec->ucVhtOpMode;
				DBGLOG(RLM, LOUD,
					"OP mode IE: same ucVhtOpMode\n");
			} else {
				prStaRec->ucVhtOpMode = prOPNotif->ucOpMode;
			}

			if (prStaRec->ucOpModeInOpNotificationIE
				== prOPNotif->ucOpMode) {
				DBGLOG(RLM, LOUD,
					"OP mode IE: same ucOpModeInOpNotificationIE 0x%x\n",
					prStaRec->ucOpModeInOpNotificationIE);
				fgUpdateToFWForDiffOPModeIE = FALSE;
			}

			prStaRec->ucOpModeInOpNotificationIE =
				prOPNotif->ucOpMode;

			fgHasNewOPModeIE = TRUE;

			ucVhtOpModeChannelWidth =
				(prOPNotif->ucOpMode &
				VHT_OP_MODE_CHANNEL_WIDTH);

			ucVhtOpModeRxNss =
				(prOPNotif->ucOpMode & VHT_OP_MODE_RX_NSS)
				>> VHT_OP_MODE_RX_NSS_OFFSET;

			if (ucVhtOpModeRxNss == VHT_OP_MODE_NSS_2) {
				prStaRec->u2VhtRxMcsMap = BITS(0, 15) &
					(~(VHT_CAP_INFO_MCS_1SS_MASK |
					VHT_CAP_INFO_MCS_2SS_MASK));

				prStaRec->u2VhtRxMcsMap |=
					(prStaRec->u2VhtRxMcsMapAssoc &
					(VHT_CAP_INFO_MCS_1SS_MASK |
					VHT_CAP_INFO_MCS_2SS_MASK));
			} else {
				prStaRec->u2VhtRxMcsMap = BITS(0, 15) &
					(~VHT_CAP_INFO_MCS_1SS_MASK);

				prStaRec->u2VhtRxMcsMap |=
					(prStaRec->u2VhtRxMcsMapAssoc &
					VHT_CAP_INFO_MCS_1SS_MASK);
			}

#if (CFG_SUPPORT_802_11AX == 1)
			u2HeRxMcsMapAssoc = prStaRec->u2HeRxMcsMapBW80Assoc;
			if (ucVhtOpModeRxNss == VHT_OP_MODE_NSS_2) {
				prStaRec->u2HeRxMcsMapBW80 = BITS(0, 15) &
					(~(HE_CAP_INFO_MCS_1SS_MASK |
					HE_CAP_INFO_MCS_2SS_MASK));

				prStaRec->u2HeRxMcsMapBW80 |=
					(u2HeRxMcsMapAssoc &
					(HE_CAP_INFO_MCS_1SS_MASK |
					HE_CAP_INFO_MCS_2SS_MASK));
			} else {
				fgIsRx1ss = TRUE;

				prStaRec->u2HeRxMcsMapBW80 = BITS(0, 15) &
					(~HE_CAP_INFO_MCS_1SS_MASK);

				prStaRec->u2HeRxMcsMapBW80 |=
					(u2HeRxMcsMapAssoc &
					HE_CAP_INFO_MCS_1SS_MASK);
			}

			if (ucMaxBwAllowed >= MAX_BW_160MHZ) {
				u2HeRxMcsMapAssoc =
					prStaRec->u2HeRxMcsMapBW160Assoc;
				if (ucVhtOpModeRxNss == VHT_OP_MODE_NSS_2) {
					prStaRec->u2HeRxMcsMapBW160 =
						BITS(0, 15) &
						(~(HE_CAP_INFO_MCS_1SS_MASK |
						HE_CAP_INFO_MCS_2SS_MASK));

					prStaRec->u2HeRxMcsMapBW160 |=
						(u2HeRxMcsMapAssoc &
						(HE_CAP_INFO_MCS_1SS_MASK |
						HE_CAP_INFO_MCS_2SS_MASK));
				} else {
					prStaRec->u2HeRxMcsMapBW160 =
						BITS(0, 15) &
						(~HE_CAP_INFO_MCS_1SS_MASK);

					prStaRec->u2HeRxMcsMapBW160 |=
						(u2HeRxMcsMapAssoc &
						HE_CAP_INFO_MCS_1SS_MASK);
				}
			}
			DBGLOG(RLM, INFO,
				"[OP Mode IE] NSS=%d,MaxBW=%d,(RxMcsMap,McsMapAssoc):(0x%x,0x%x)BW80(0x%x,0x%x)BW160(0x%x,0x%x)\n",
				ucVhtOpModeRxNss, ucMaxBwAllowed,
				prStaRec->u2VhtRxMcsMap,
				prStaRec->u2VhtRxMcsMapAssoc,
				prStaRec->u2HeRxMcsMapBW80,
				prStaRec->u2HeRxMcsMapBW80Assoc,
				prStaRec->u2HeRxMcsMapBW160,
				prStaRec->u2HeRxMcsMapBW160Assoc);
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (fgIsRx1ss)
				prStaRec->u2He6gBandCapInfo &=
					~HE_6G_CAP_INFO_SM_POWER_SAVE;
			else
				prStaRec->u2He6gBandCapInfo |=
					HE_6G_CAP_INFO_SM_POWER_SAVE;
#endif
#else
			DBGLOG(RLM, INFO,
				"[OP Mode IE] NSS=%x RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
				ucVhtOpModeRxNss, prStaRec->u2VhtRxMcsMap,
				prStaRec->u2VhtRxMcsMapAssoc);
#endif /* CFG_SUPPORT_802_11AX == 1 */
			break;
		default:
			break;

		}
	} else {
		/*
		 * beacon with OP mode IE -> beacon without OP mode IE
		 * for the first time, thus require update to FW
		 */
		if (fgHasOPModeIE == FALSE
			&& prStaRec->ucOpModeInOpNotificationIE != 0xff) {
			prStaRec->ucOpModeInOpNotificationIE = 0xff;
			DBGLOG(RLM, TRACE,
				"OP mode IE not present for the first time, need update to FW\n");
			cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
		} else if (fgHasOPModeIE == FALSE
			&& prStaRec->ucOpModeInOpNotificationIE == 0xff) {
			/* beacon w/o OP mode IE & no need to update to FW */
			fgUpdateToFWForDiffOPModeIE = FALSE;
		}
	}

	if (prStaRec->ucStaState == STA_STATE_3) {
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (IsfgHe6gBandCapChange) {
			if (fgHasNewOPModeIE == TRUE
				&& fgUpdateToFWForDiffOPModeIE == FALSE) {
			} else {
				DBGLOG(RLM, LOUD,
					"Update staRec due to IsfgHe6gBandCapChange");
				cnmStaSendUpdateCmd(prAdapter, prStaRec,
					NULL, FALSE);
			}
		}
#endif
		if (IsfgHtCapChange) {
			DBGLOG(RLM, LOUD,
				"Update staRec due to IsfgHtCapChange");
			cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
		}
	}

	/* Some AP will have wrong channel number (255) when running time.
	 * Check if correct channel number information. 20110501
	 */
	if ((prBssInfo->eBand == BAND_2G4 && ucPrimaryChannel > 14) ||
	    (prBssInfo->eBand == BAND_5G &&
	     (ucPrimaryChannel >= 180 || ucPrimaryChannel <= 14))
#if (CFG_SUPPORT_WIFI_6G == 1)
		|| (prBssInfo->eBand == BAND_6G &&
		 (ucPrimaryChannel > 233 || ucPrimaryChannel < 1))
#endif
	)
		ucPrimaryChannel = 0;
#if CFG_SUPPORT_802_11AC
	/* Check whether the Operation Mode IE is exist or not.
	 *  If exists, then the channel bandwidth of VHT operation field  is
	 * changed
	 *  with the channel bandwidth setting of Operation Mode field.
	 *  The channel bandwidth of OP Mode IE  is  0, represent as 20MHz.
	 *  The channel bandwidth of OP Mode IE  is  1, represent as 40MHz.
	 *  The channel bandwidth of OP Mode IE  is  2, represent as 80MHz.
	 *  The channel bandwidth of OP Mode IE  is  3, represent as
	 * 160/80+80MHz.
	 */
	if (fgHasNewOPModeIE == TRUE) {
		if (prStaRec->ucStaState == STA_STATE_3) {
			if (fgUpdateToFWForDiffOPModeIE == TRUE) {
				/* 1. Modify channel width parameters */
				rlmRecOpModeBwForClient(ucVhtOpModeChannelWidth,
						prBssInfo);

				/* 2. Update StaRec to FW (BssInfo will be
				* updated after return from this function)
				*/
				DBGLOG(RLM, INFO,
			       "Update VhtOpMode to 0x%x, to FW due to OpMode Notificaition",
			       prStaRec->ucVhtOpMode);
#if (CFG_SUPPORT_802_11AX == 1)
				DBGLOG(RLM, INFO,
					"HeBW80 RxMcsMap:0x%x, HeBW160 RxMcsMap:0x%x\n",
					prStaRec->u2HeRxMcsMapBW80,
					prStaRec->u2HeRxMcsMapBW160);
#endif
				cnmStaSendUpdateCmd(prAdapter, prStaRec,
					NULL, FALSE);
			}

			/* 3. Revise by own OP BW if needed */
			if ((prBssInfo->fgIsOpChangeChannelWidth)) {
				/* VHT */
				if (rlmGetVhtOpBwByBssOpBw(
					    prBssInfo->ucOpChangeChannelWidth) <
				    prBssInfo->ucVhtChannelWidth)
					rlmFillVhtOpInfoByBssOpBw(
					prAdapter,
					prBssInfo,
					prBssInfo
					->ucOpChangeChannelWidth);
				/* HT */
				if (prBssInfo->fgIsOpChangeChannelWidth &&
				    prBssInfo->ucOpChangeChannelWidth ==
					    MAX_BW_20MHZ) {
					prBssInfo->ucHtOpInfo1 &=
						~(HT_OP_INFO1_SCO |
						  HT_OP_INFO1_STA_CHNL_WIDTH);
					prBssInfo->eBssSCO = CHNL_EXT_SCN;
					DBGLOG(RLM, TRACE,
						"SCO updated by OP Mode IE\n");
				}
			}
		}
	} else { /* Set Default if the VHT OP mode field is not present */
		if (!fgHasOPModeIE) {
			ucInitVhtOpMode |=
				rlmGetOpModeBwByVhtAndHtOpInfo(prBssInfo);
			ucInitVhtOpMode |=
				((rlmGetSupportRxNssInVhtCap(prVhtCap) - 1)
				<< VHT_OP_MODE_RX_NSS_OFFSET) &
				VHT_OP_MODE_RX_NSS;
		}
		if ((prStaRec->ucVhtOpMode != ucInitVhtOpMode) &&
			(prStaRec->ucStaState == STA_STATE_3)) {
			prStaRec->ucVhtOpMode = ucInitVhtOpMode;
			DBGLOG(RLM, INFO, "Update OpMode to 0x%x",
			       prStaRec->ucVhtOpMode);
			DBGLOG(RLM, INFO,
			       "to FW due to NO OpMode Notificaition\n");
			cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
		} else
			prStaRec->ucVhtOpMode = ucInitVhtOpMode;
	}
#endif

#if CFG_SUPPORT_DFS
	if (SHOULD_CH_SWITCH(ucCurrentCsaCount, prCSAParams)) {
		uint16_t u2SwitchTime;
		cnmTimerStopTimer(prAdapter, &prBssInfo->rCsaTimer);
		if (prCSAParams->u4MaxSwitchTime != 0)
			u2SwitchTime =
			       prBssInfo->u2BeaconInterval * ucCurrentCsaCount +
			       prCSAParams->u4MaxSwitchTime;
		else
			u2SwitchTime =
				prBssInfo->u2BeaconInterval * ucCurrentCsaCount;

		cnmTimerStartTimer(prAdapter, &prBssInfo->rCsaTimer,
			u2SwitchTime);

		prCSAParams->ucCsaCount = ucCurrentCsaCount;
		DBGLOG(RLM, INFO, "[CSA] Channel switch Countdown: %d msecs\n",
		       u2SwitchTime);
	}
#endif

	/* Do not write prBssInfo->ucVhtChannelWidth directly
	 * in rlmReviseMaxBw, otherwise it will cause the following
	 * struct members being overwritten unexpectly.
	 */
	eChannelWidth = (enum ENUM_CHANNEL_WIDTH)
		prBssInfo->ucVhtChannelWidth;
	rlmReviseMaxBw(prAdapter, prBssInfo->ucBssIndex,
		&prBssInfo->eBssSCO,
		&eChannelWidth,
		&prBssInfo->ucVhtChannelFrequencyS1,
		&prBssInfo->ucPrimaryChannel);
	prBssInfo->ucVhtChannelWidth = (uint8_t)eChannelWidth;
	if (prBssInfo->ucVhtChannelWidth == CW_20_40MHZ
			&& prBssInfo->eBssSCO == CHNL_EXT_SCN)
		prBssInfo->ucHtOpInfo1 &=
			~(HT_OP_INFO1_SCO | HT_OP_INFO1_STA_CHNL_WIDTH);

	/* Receive new beacon after channel switch */
	if (!HAS_CH_SWITCH_PARAMS(prCSAParams) &&
	    prCSAParams->ucCsaMode < MODE_NUM &&
	    !IS_AIS_CH_SWITCH(prBssInfo)) {
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		struct BSS_DESC *prBssDesc = NULL;

		if (IS_BSS_INDEX_AIS(prAdapter,
			prBssInfo->ucBssIndex)) {
			prBssDesc = aisGetTargetBssDesc(prAdapter,
				prBssInfo->ucBssIndex);
		} else if (IS_BSS_INDEX_P2P(prAdapter,
			prBssInfo->ucBssIndex)) {
			prBssDesc = p2pGetTargetBssDesc(prAdapter,
				prBssInfo->ucBssIndex);
		}

		if (prBssDesc)
			rlmDomain6GPwrModeUpdate(prAdapter,
				prBssInfo->ucBssIndex,
				prBssDesc->e6GPwrMode);
#endif

		rlmUpdateParamsForCSA(prAdapter, prBssInfo);
		rlmChangeOperationModeAfterCSA(prAdapter, prBssInfo);

		if (IS_BSS_AIS(prBssInfo)) {
			cnmTimerStopTimer(prAdapter, &prBssInfo->rCsaDoneTimer);
			if (prCSAParams->fgIsCrossBand)
				aisFunFlushTxQueue(prAdapter, prStaRec);
		}

		if (prCSAParams->fgHasStopTx) {
			kalIndicateAllQueueTxAllowed(
				    prAdapter->prGlueInfo,
				    prBssInfo->ucBssIndex,
				    TRUE);
			qmSetStaRecTxAllowed(prAdapter, prStaRec, TRUE);
			DBGLOG(RLM, EVENT, "[CSA] TxAllowed = TRUE\n");
		}

		rlmResetCSAParams(prBssInfo, TRUE);
	}

	rlmRevisePreferBandwidthNss(prAdapter, prBssInfo->ucBssIndex, prStaRec);

	fgDomainValid = rlmDomainIsValidRfSetting(
		prAdapter,
		prBssInfo->eBand,
		prBssInfo->ucPrimaryChannel,
		prBssInfo->eBssSCO,
		prBssInfo->ucVhtChannelWidth,
		prBssInfo->ucVhtChannelFrequencyS1,
		prBssInfo->ucVhtChannelFrequencyS2);

	if (fgDomainValid == FALSE) {
		/*Dump IE Inforamtion */
		DBGLOG(RLM, WARN, "rlmRecIeInfoForClient IE Information\n");
		DBGLOG(RLM, WARN, "IE Length = %d\n", u2IELength);
		DBGLOG_MEM8(RLM, WARN, pucDumpIE, u2IELength);

		/*Error Handling for Non-predicted IE - Fixed to set 20MHz */
		prBssInfo->ucVhtChannelWidth = CW_20_40MHZ;
		prBssInfo->ucVhtChannelFrequencyS1 = 0;
		prBssInfo->ucVhtChannelFrequencyS2 = 0;
		prBssInfo->eBssSCO = CHNL_EXT_SCN;
		prBssInfo->ucHtOpInfo1 &=
			~(HT_OP_INFO1_SCO | HT_OP_INFO1_STA_CHNL_WIDTH);
	}

#if CFG_SUPPORT_QUIET && 0
	if (!fgHasQuietIE)
		rrmQuietIeNotExist(prAdapter, prBssInfo);
#endif

	/* Check if OBSS scan process will launch */
	if (!prAdapter->fgEnOnlineScan || !prObssScnParam ||
	    !(prStaRec->u2HtCapInfo & HT_CAP_INFO_SUP_CHNL_WIDTH) ||
	    prBssInfo->eBand != BAND_2G4 || !prBssInfo->fg40mBwAllowed) {

		/* Note: it is ok not to stop rObssScanTimer() here */
		prBssInfo->u2ObssScanInterval = 0;
	} else {
		if (prObssScnParam->u2TriggerScanInterval <
		    OBSS_SCAN_MIN_INTERVAL)
			prObssScnParam->u2TriggerScanInterval =
				OBSS_SCAN_MIN_INTERVAL;
		if (prBssInfo->u2ObssScanInterval !=
		    prObssScnParam->u2TriggerScanInterval) {

			prBssInfo->u2ObssScanInterval =
				prObssScnParam->u2TriggerScanInterval;

			/* Start timer to trigger OBSS scanning */
			cnmTimerStartTimer(
				prAdapter, &prBssInfo->rObssScanTimer,
				prBssInfo->u2ObssScanInterval * MSEC_PER_SEC);
		}
	}

#if (CFG_SUPPORT_BTWT == 1)
	if ((fgBtwtIeFound == FALSE) &&
		(prStaRec->arBTWTFlow[0].eBtwtState ==
			ENUM_BTWT_FLOW_STATE_ACTIVATED)) {
		prStaRec->arBTWTFlow[0].eBtwtState =
			ENUM_BTWT_FLOW_STATE_DEFAULT;
		prStaRec->arBTWTFlow[0].eTwtType = ENUM_TWT_TYPE_DEFAULT;

		btwtPlannerDelAgrtTbl(prAdapter, prBssInfo, prStaRec, 0);
	}
#endif

	return ucPrimaryChannel;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Update parameters from channel width field in OP Mode IE/action frame
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static void rlmRecOpModeBwForClient(uint8_t ucVhtOpModeChannelWidth,
				    struct BSS_INFO *prBssInfo)
{

	struct STA_RECORD *prStaRec = NULL;

	if (!prBssInfo)
		return;

	prStaRec = prBssInfo->prStaRecOfAP;
	if (!prStaRec)
		return;

	switch (ucVhtOpModeChannelWidth) {
	case VHT_OP_MODE_CHANNEL_WIDTH_20:
		prBssInfo->ucVhtChannelWidth = VHT_OP_CHANNEL_WIDTH_20_40;
		prBssInfo->ucHtOpInfo1 &= ~HT_OP_INFO1_STA_CHNL_WIDTH;
		prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_SUP_CHNL_WIDTH;

#if CFG_OPMODE_CONFLICT_OPINFO
		if (prBssInfo->eBssSCO != CHNL_EXT_SCN) {
			DBGLOG(RLM, WARN,
			       "HT_OP_Info != OPmode_Notifify, follow OPmode_Notify to BW20.\n");
			prBssInfo->eBssSCO = CHNL_EXT_SCN;
		}
#endif
		break;
	case VHT_OP_MODE_CHANNEL_WIDTH_40:
		prBssInfo->ucVhtChannelWidth = VHT_OP_CHANNEL_WIDTH_20_40;
		prBssInfo->ucHtOpInfo1 |= HT_OP_INFO1_STA_CHNL_WIDTH;
		prStaRec->u2HtCapInfo |= HT_CAP_INFO_SUP_CHNL_WIDTH;

#if CFG_OPMODE_CONFLICT_OPINFO
		if (prBssInfo->eBssSCO == CHNL_EXT_SCN) {
			prBssInfo->ucHtOpInfo1 &= ~HT_OP_INFO1_STA_CHNL_WIDTH;
			prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_SUP_CHNL_WIDTH;
			DBGLOG(RLM, WARN,
			       "HT_OP_Info != OPmode_Notifify, follow HT_OP_Info to BW20.\n");
		}
#endif
		break;
	case VHT_OP_MODE_CHANNEL_WIDTH_80:
#if CFG_OPMODE_CONFLICT_OPINFO
		if (prBssInfo->ucVhtChannelWidth !=
		    VHT_OP_CHANNEL_WIDTH_80) {
			DBGLOG(RLM, WARN,
			       "VHT_OP != OPmode:%d, follow VHT_OP to VHT_OP:%d HT_OP:%d\n",
			       ucVhtOpModeChannelWidth,
			       prBssInfo->ucVhtChannelWidth,
			       (uint8_t)(prBssInfo->ucHtOpInfo1 &
					 HT_OP_INFO1_STA_CHNL_WIDTH) >>
				       HT_OP_INFO1_STA_CHNL_WIDTH_OFFSET);
		} else
#endif
		{
			prBssInfo->ucVhtChannelWidth = VHT_OP_CHANNEL_WIDTH_80;
			prBssInfo->ucHtOpInfo1 |= HT_OP_INFO1_STA_CHNL_WIDTH;
			prStaRec->u2HtCapInfo |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		}
		break;
	case VHT_OP_MODE_CHANNEL_WIDTH_160_80P80:
/* Determine BW160 or BW80+BW80 by VHT OP Info */
#if CFG_OPMODE_CONFLICT_OPINFO
		if ((prBssInfo->ucVhtChannelWidth !=
		     VHT_OP_CHANNEL_WIDTH_160) &&
		    (prBssInfo->ucVhtChannelWidth !=
		     VHT_OP_CHANNEL_WIDTH_80P80)) {
			DBGLOG(RLM, WARN,
			       "VHT_OP != OPmode:%d, follow VHT_OP to VHT_OP:%d HT_OP:%d\n",
			       ucVhtOpModeChannelWidth,
			       prBssInfo->ucVhtChannelWidth,
			       (uint8_t)(prBssInfo->ucHtOpInfo1 &
					 HT_OP_INFO1_STA_CHNL_WIDTH) >>
				       HT_OP_INFO1_STA_CHNL_WIDTH_OFFSET);
		} else
#endif
		{
			prBssInfo->ucHtOpInfo1 |= HT_OP_INFO1_STA_CHNL_WIDTH;
			prStaRec->u2HtCapInfo |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		}
		break;
	default:
		break;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Update parameters from Association Response frame
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static void rlmRecAssocRespIeInfoForClient(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, const uint8_t *pucIE,
		uint16_t u2IELength)
{
	uint16_t u2Offset;
	struct STA_RECORD *prStaRec;
	u_int8_t fgIsHasHtCap = FALSE;
	u_int8_t fgIsHasVhtCap = FALSE;
#if (CFG_SUPPORT_802_11AX == 1)
	u_int8_t fgIsHasHeCap = FALSE;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	u_int8_t fgIsHasEhtCap = FALSE;
#endif
	struct BSS_DESC *prBssDesc;
	struct PARAM_SSID rSsid;
#if (CFG_SUPPORT_BSS_MAX_IDLE_PERIOD == 1)
	struct IE_BSS_MAX_IDLE_PERIOD *prBssMaxIdlePeriod;
#endif

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(pucIE);

	prStaRec = prBssInfo->prStaRecOfAP;
	kalMemZero(&rSsid, sizeof(rSsid));

	if (!prStaRec)
		return;
	COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen, prBssInfo->aucSSID,
		  prBssInfo->ucSSIDLen);
	prBssDesc = scanSearchBssDescByBssidAndSsid(
		prAdapter, prStaRec->aucMacAddr, TRUE, &rSsid);

	IE_FOR_EACH(pucIE, u2IELength, u2Offset)
	{
		switch (IE_ID(pucIE)) {
		case ELEM_ID_HT_CAP:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (prBssInfo->eBand == BAND_6G) {
				DBGLOG(SCN, WARN, "Ignore HT CAP IE at 6G\n");
				break;
			}
#endif
			if (!RLM_NET_IS_11N(prBssInfo) ||
			    IE_LEN(pucIE) != (sizeof(struct IE_HT_CAP) - 2))
				break;
			fgIsHasHtCap = TRUE;
			break;
#if CFG_SUPPORT_802_11AC
		case ELEM_ID_VHT_CAP:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (prBssInfo->eBand == BAND_6G) {
				DBGLOG(SCN, WARN, "Ignore VHT CAP IE at 6G\n");
				break;
			}
#endif
			if (!RLM_NET_IS_11AC(prBssInfo) ||
			    IE_LEN(pucIE) != (sizeof(struct IE_VHT_CAP) - 2))
				break;
			fgIsHasVhtCap = TRUE;
			break;
#endif
		case ELEM_ID_RESERVED:
#if (CFG_SUPPORT_802_11AX == 1)

			if (fgEfuseCtrlAxOn == 1) {
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_CAP &&
			    RLM_NET_IS_11AX(prBssInfo))
				fgIsHasHeCap = TRUE;
			}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_EHT_CAPS &&
				RLM_NET_IS_11BE(prBssInfo))
				fgIsHasEhtCap = TRUE;
#endif
			break;
#if (CFG_SUPPORT_BSS_MAX_IDLE_PERIOD == 1)
		case WLAN_EID_BSS_MAX_IDLE_PERIOD:
			if (IE_LEN(pucIE) !=
				(sizeof(struct IE_BSS_MAX_IDLE_PERIOD) - 2))
				break;

			prBssMaxIdlePeriod =
				(struct IE_BSS_MAX_IDLE_PERIOD *) pucIE;
			prBssInfo->u2MaxIdlePeriod =
				prBssMaxIdlePeriod->u2MaxIdlePeriod;
			prBssInfo->ucIdleOptions =
				prBssMaxIdlePeriod->ucIdleOptions;

			break;
#endif
		case ELEM_ID_EXTENDED_CAP:
			DBGLOG(P2P, TRACE, "Dump ext cap.\n");
			DBGLOG_MEM8(P2P, TRACE, pucIE, IE_SIZE(pucIE));
			if (EXT_CAP_IE(pucIE)->ucLength > sizeof(uint8_t) &&
			    (EXT_CAP_IE(pucIE)->aucCapabilities[0] &
			     ELEM_EXT_CAP_ECSA_CAP))
				prStaRec->fgEcsaCapable = TRUE;
			break;
		case ELEM_ID_VENDOR:
			rlmParseMtkOuiForAssocResp(prAdapter, prStaRec,
				prBssInfo, pucIE);
			break;
		default:
			break;
		} /* end of switch */
	}	 /* end of IE_FOR_EACH */

	if (!fgIsHasHtCap) {
		prStaRec->ucDesiredPhyTypeSet &= ~PHY_TYPE_BIT_HT;
		if (prBssDesc) {
			if (prBssDesc->ucPhyTypeSet & PHY_TYPE_BIT_HT) {
				DBGLOG(RLM, WARN,
				       "PhyTypeSet in Beacon and AssocResp are unsync. ");
				DBGLOG(RLM, WARN,
				       "Follow AssocResp to disable HT.\n");
			}
		}
	}
	if (!fgIsHasVhtCap) {
		prStaRec->ucDesiredPhyTypeSet &= ~PHY_TYPE_BIT_VHT;
		if (prBssDesc) {
			if (prBssDesc->ucPhyTypeSet & PHY_TYPE_BIT_VHT) {
				DBGLOG(RLM, WARN,
				       "PhyTypeSet in Beacon and AssocResp are unsync. ");
				DBGLOG(RLM, WARN,
				       "Follow AssocResp to disable VHT.\n");
			}
		}
	}
#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
	if (!fgIsHasHeCap) {
		prStaRec->ucDesiredPhyTypeSet &= ~PHY_TYPE_BIT_HE;
		if (prBssDesc) {
			if (prBssDesc->ucPhyTypeSet & PHY_TYPE_BIT_HE) {
				DBGLOG(RLM, WARN, "PhyTypeSet are unsync. ");
				DBGLOG(RLM, WARN, "Disable HE per assoc.\n");
			}
		}
	}
	}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	if (!fgIsHasEhtCap) {
		prStaRec->ucDesiredPhyTypeSet &= ~PHY_TYPE_BIT_EHT;
		if (prBssDesc) {
			if (prBssDesc->ucPhyTypeSet & PHY_TYPE_BIT_EHT) {
				DBGLOG(RLM, WARN, "PhyTypeSet are unsync. ");
				DBGLOG(RLM, WARN, "Disable EHT per assoc.\n");
			}
		}
	}
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief AIS or P2P GC.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static u_int8_t rlmRecBcnFromNeighborForClient(struct ADAPTER *prAdapter,
					       struct BSS_INFO *prBssInfo,
					       struct SW_RFB *prSwRfb,
					       uint8_t *pucIE,
					       uint16_t u2IELength)
{
	uint16_t u2Offset, i;
	uint8_t ucPriChannel, ucSecChannel;
	enum ENUM_CHNL_EXT eSCO;
	u_int8_t fgHtBss, fg20mReq;
	enum ENUM_BAND eBand = 0;
	struct RX_DESC_OPS_T *prRxDescOps;

	ASSERT(prAdapter);
	ASSERT(prBssInfo && prSwRfb);
	ASSERT(pucIE);
	prRxDescOps = prAdapter->chip_info->prRxDescOps;

	/* Record it to channel list to change 20/40 bandwidth */
	ucPriChannel = 0;
	eSCO = CHNL_EXT_SCN;

	fgHtBss = FALSE;
	fg20mReq = FALSE;

	IE_FOR_EACH(pucIE, u2IELength, u2Offset)
	{
		switch (IE_ID(pucIE)) {
		case ELEM_ID_HT_CAP: {
			struct IE_HT_CAP *prHtCap;

			if (IE_LEN(pucIE) != (sizeof(struct IE_HT_CAP) - 2))
				break;

			prHtCap = (struct IE_HT_CAP *)pucIE;
			if (prHtCap->u2HtCapInfo & HT_CAP_INFO_40M_INTOLERANT)
				fg20mReq = TRUE;
			fgHtBss = TRUE;
			break;
		}
		case ELEM_ID_HT_OP: {
			struct IE_HT_OP *prHtOp;

			if (IE_LEN(pucIE) != (sizeof(struct IE_HT_OP) - 2))
				break;

			prHtOp = (struct IE_HT_OP *)pucIE;
			/* Workaround that some APs fill primary channel field
			 * by its
			 * secondary channel, but its DS IE is correct 20110610
			 */
			if (ucPriChannel == 0)
				ucPriChannel = prHtOp->ucPrimaryChannel;

			if ((prHtOp->ucInfo1 & HT_OP_INFO1_SCO) != CHNL_EXT_RES)
				eSCO = (enum ENUM_CHNL_EXT)(prHtOp->ucInfo1 &
							    HT_OP_INFO1_SCO);
			break;
		}
		case ELEM_ID_20_40_BSS_COEXISTENCE: {
			struct IE_20_40_COEXIST *prCoexist;

			if (IE_LEN(pucIE) !=
			    (sizeof(struct IE_20_40_COEXIST) - 2))
				break;

			prCoexist = (struct IE_20_40_COEXIST *)pucIE;
			if (prCoexist->ucData & BSS_COEXIST_40M_INTOLERANT)
				fg20mReq = TRUE;
			break;
		}
		case ELEM_ID_DS_PARAM_SET:
			if (IE_LEN(pucIE) !=
			    (sizeof(struct IE_DS_PARAM_SET) - 2))
				break;
			ucPriChannel = DS_PARAM_IE(pucIE)->ucCurrChnl;
			break;

		default:
			break;
		}
	}

	/* To do: Update channel list and 5G band. All channel lists have the
	 * same
	 * update procedure. We should give it the entry pointer of desired
	 * channel list.
	 */
	eBand = prSwRfb->eRfBand;
	if (eBand != BAND_2G4)
		return FALSE;

	if (ucPriChannel == 0 || ucPriChannel > 14)
		ucPriChannel = prSwRfb->ucChnlNum;

	if (fgHtBss) {
		ASSERT(prBssInfo->auc2G_PriChnlList[0] <= CHNL_LIST_SZ_2G);
		for (i = 1; i <= prBssInfo->auc2G_PriChnlList[0] &&
			    i <= CHNL_LIST_SZ_2G;
		     i++) {
			if (prBssInfo->auc2G_PriChnlList[i] == ucPriChannel)
				break;
		}
		if ((i > prBssInfo->auc2G_PriChnlList[0]) &&
		    (i <= CHNL_LIST_SZ_2G)) {
			prBssInfo->auc2G_PriChnlList[i] = ucPriChannel;
			prBssInfo->auc2G_PriChnlList[0]++;
		}

		/* Update secondary channel */
		if (eSCO != CHNL_EXT_SCN) {
			ucSecChannel = (eSCO == CHNL_EXT_SCA)
					       ? (ucPriChannel + 4)
					       : (ucPriChannel - 4);

			ASSERT(prBssInfo->auc2G_SecChnlList[0] <=
			       CHNL_LIST_SZ_2G);
			for (i = 1; i <= prBssInfo->auc2G_SecChnlList[0] &&
				    i <= CHNL_LIST_SZ_2G;
			     i++) {
				if (prBssInfo->auc2G_SecChnlList[i] ==
				    ucSecChannel)
					break;
			}
			if ((i > prBssInfo->auc2G_SecChnlList[0]) &&
			    (i <= CHNL_LIST_SZ_2G)) {
				prBssInfo->auc2G_SecChnlList[i] = ucSecChannel;
				prBssInfo->auc2G_SecChnlList[0]++;
			}
		}

		/* Update 20M bandwidth request channels */
		if (fg20mReq) {
			ASSERT(prBssInfo->auc2G_20mReqChnlList[0] <=
			       CHNL_LIST_SZ_2G);
			for (i = 1; i <= prBssInfo->auc2G_20mReqChnlList[0] &&
				    i <= CHNL_LIST_SZ_2G;
			     i++) {
				if (prBssInfo->auc2G_20mReqChnlList[i] ==
				    ucPriChannel)
					break;
			}
			if ((i > prBssInfo->auc2G_20mReqChnlList[0]) &&
			    (i <= CHNL_LIST_SZ_2G)) {
				prBssInfo->auc2G_20mReqChnlList[i] =
					ucPriChannel;
				prBssInfo->auc2G_20mReqChnlList[0]++;
			}
		}
	} else {
		/* Update non-HT channel list */
		ASSERT(prBssInfo->auc2G_NonHtChnlList[0] <= CHNL_LIST_SZ_2G);
		for (i = 1; i <= prBssInfo->auc2G_NonHtChnlList[0] &&
			    i <= CHNL_LIST_SZ_2G;
		     i++) {
			if (prBssInfo->auc2G_NonHtChnlList[i] == ucPriChannel)
				break;
		}
		if ((i > prBssInfo->auc2G_NonHtChnlList[0]) &&
		    (i <= CHNL_LIST_SZ_2G)) {
			prBssInfo->auc2G_NonHtChnlList[i] = ucPriChannel;
			prBssInfo->auc2G_NonHtChnlList[0]++;
		}
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief AIS or P2P GC.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static u_int8_t rlmRecBcnInfoForClient(struct ADAPTER *prAdapter,
				       struct BSS_INFO *prBssInfo,
				       struct SW_RFB *prSwRfb, uint8_t *pucIE,
				       uint16_t u2IELength)
{
	/* For checking if syncing params are different from
	 * last syncing and need to sync again
	 */
	struct CMD_SET_BSS_RLM_PARAM rBssRlmParam;
	struct CMD_SET_BSS_INFO rBssInfo;
#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
	u_int8_t fgChangeBssColor = FALSE;
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */
	u_int8_t fgNewParameter = FALSE;
#if CFG_SUPPORT_802_PP_DSCB
	uint8_t  u1PreDscbPresent = 0;
	uint16_t u2PreDscBitmap = 0;
#endif

#if CFG_SUPPORT_BALANCE_MLR
	struct WLAN_BEACON_FRAME *prWlanBeacon = NULL;
	u_int8_t fgIsBeaconIntervalChange = FALSE;
#endif /* CFG_SUPPORT_BALANCE_MLR */

	ASSERT(prAdapter);
	ASSERT(prBssInfo && prSwRfb);
	ASSERT(pucIE);

#if 0 /* SW migration 2010/8/20 */
	/* Note: we shall not update parameters when scanning, otherwise
	 * channel and bandwidth will not be correct or asserted failure
	 * during scanning.
	 * Note: remove channel checking. All received Beacons should be
	 * processed if measurement or other actions are executed in adjacent
	 * channels and Beacon content checking mechanism is not disabled.
	 */
	if (IS_SCAN_ACTIVE()
	    /* || prBssInfo->ucPrimaryChannel != CHNL_NUM_BY_SWRFB(prSwRfb) */
	    ) {
		return FALSE;
	}
#endif

#if CFG_SUPPORT_BALANCE_MLR
	/* Handle change of Beacon Interval */
	prWlanBeacon = (struct WLAN_BEACON_FRAME *)(prSwRfb->pvHeader);
	if (prBssInfo->u2BeaconInterval != prWlanBeacon->u2BeaconInterval) {
		DBGLOG(RLM, TRACE, "Beacon interval change [%u]->[%u]\n",
					       prBssInfo->u2BeaconInterval,
					       prWlanBeacon->u2BeaconInterval);
		prBssInfo->u2BeaconInterval = prWlanBeacon->u2BeaconInterval;
		fgIsBeaconIntervalChange = TRUE;
	}
#endif /* CFG_SUPPORT_BALANCE_MLR */

	/* Handle change of slot time */
	prBssInfo->u2CapInfo =
		((struct WLAN_BEACON_FRAME *)(prSwRfb->pvHeader))->u2CapInfo;
	prBssInfo->fgUseShortSlotTime =
		((prBssInfo->u2CapInfo & CAP_INFO_SHORT_SLOT_TIME) ||
		 (prBssInfo->eBand != BAND_2G4))
			? TRUE
			: FALSE;

	/* Check if syncing params are different from last syncing and need to
	 * sync again
	 * If yes, return TRUE and sync with FW; Otherwise, return FALSE.
	 */
	rBssRlmParam.ucRfBand = (u_int8_t)prBssInfo->eBand;
	rBssRlmParam.ucPrimaryChannel = prBssInfo->ucPrimaryChannel;
	rBssRlmParam.ucRfSco = (u_int8_t)prBssInfo->eBssSCO;
	rBssRlmParam.ucErpProtectMode = (u_int8_t)prBssInfo->fgErpProtectMode;
	rBssRlmParam.ucHtProtectMode = (u_int8_t)prBssInfo->eHtProtectMode;
	rBssRlmParam.ucGfOperationMode = (u_int8_t)prBssInfo->eGfOperationMode;
	rBssRlmParam.ucTxRifsMode = (u_int8_t)prBssInfo->eRifsOperationMode;
	rBssRlmParam.u2HtOpInfo3 = prBssInfo->u2HtOpInfo3;
	rBssRlmParam.u2HtOpInfo2 = prBssInfo->u2HtOpInfo2;
	rBssRlmParam.ucHtOpInfo1 = prBssInfo->ucHtOpInfo1;
	rBssRlmParam.ucUseShortPreamble = prBssInfo->fgUseShortPreamble;
	rBssRlmParam.ucUseShortSlotTime = prBssInfo->fgUseShortSlotTime;
	rBssRlmParam.ucVhtChannelWidth = prBssInfo->ucVhtChannelWidth;
	rBssRlmParam.ucVhtChannelFrequencyS1 =
		prBssInfo->ucVhtChannelFrequencyS1;
	rBssRlmParam.ucVhtChannelFrequencyS2 =
		prBssInfo->ucVhtChannelFrequencyS2;
	rBssRlmParam.u2VhtBasicMcsSet = prBssInfo->u2VhtBasicMcsSet;
	rBssRlmParam.ucRxNss = prBssInfo->ucOpRxNss;
	rBssRlmParam.ucTxNss = prBssInfo->ucOpTxNss;
#if CFG_SUPPORT_802_PP_DSCB
	u1PreDscbPresent = prBssInfo->fgIsEhtDscbPresent;
	u2PreDscBitmap = prBssInfo->u2EhtDisSubChanBitmap;
#endif

	kalMemZero(&rBssInfo, sizeof(struct CMD_SET_BSS_INFO));
#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1)
		rBssInfo.ucBssColorInfo = prBssInfo->ucBssColorInfo;
#endif

	rlmRecIeInfoForClient(prAdapter, prBssInfo, pucIE, u2IELength);

	if (rBssRlmParam.ucRfBand != prBssInfo->eBand ||
		rBssRlmParam.ucPrimaryChannel != prBssInfo->ucPrimaryChannel ||
		rBssRlmParam.ucRfSco != prBssInfo->eBssSCO ||
		rBssRlmParam.ucErpProtectMode != prBssInfo->fgErpProtectMode ||
		rBssRlmParam.ucHtProtectMode != prBssInfo->eHtProtectMode ||
		rBssRlmParam.ucGfOperationMode != prBssInfo->eGfOperationMode ||
		rBssRlmParam.ucTxRifsMode != prBssInfo->eRifsOperationMode ||
		rBssRlmParam.u2HtOpInfo3 != prBssInfo->u2HtOpInfo3 ||
		rBssRlmParam.u2HtOpInfo2 != prBssInfo->u2HtOpInfo2 ||
		rBssRlmParam.ucHtOpInfo1 != prBssInfo->ucHtOpInfo1 ||
		rBssRlmParam.ucUseShortPreamble !=
			prBssInfo->fgUseShortPreamble ||
		rBssRlmParam.ucUseShortSlotTime !=
			prBssInfo->fgUseShortSlotTime ||
		rBssRlmParam.ucVhtChannelWidth !=
			prBssInfo->ucVhtChannelWidth ||
		rBssRlmParam.ucVhtChannelFrequencyS1 !=
			prBssInfo->ucVhtChannelFrequencyS1 ||
		rBssRlmParam.ucVhtChannelFrequencyS2 !=
			prBssInfo->ucVhtChannelFrequencyS2 ||
		rBssRlmParam.u2VhtBasicMcsSet != prBssInfo->u2VhtBasicMcsSet ||
		rBssRlmParam.ucRxNss != prBssInfo->ucOpRxNss ||
		rBssRlmParam.ucTxNss != prBssInfo->ucOpTxNss
#if CFG_SUPPORT_802_PP_DSCB
		|| u1PreDscbPresent != prBssInfo->fgIsEhtDscbPresent
		|| u2PreDscBitmap != prBssInfo->u2EhtDisSubChanBitmap
#endif
		)
		fgNewParameter = TRUE;
	else {
		DBGLOG(RLM, TRACE,
		       "prBssInfo's params are all the same! not to sync!\n");
		fgNewParameter = FALSE;
	}

#if (CFG_SUPPORT_802_11AX == 1)
		if (fgEfuseCtrlAxOn == 1) {
#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
			fgChangeBssColor =
				updateHeBssColor(prBssInfo,
					prSwRfb, rBssInfo.ucBssColorInfo);

			if (fgChangeBssColor)
				fgNewParameter = TRUE;
#else /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */
			if (rBssInfo.ucBssColorInfo
					!= prBssInfo->ucBssColorInfo) {
				fgNewParameter = TRUE;
				DBGLOG(RLM, INFO,
					"BssColorInfo is changed from %x to %x. Update BSSInfo to FW\n",
					rBssInfo.ucBssColorInfo,
					prBssInfo->ucBssColorInfo);
			}
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */
		}
#endif /* CFG_SUPPORT_802_11AX */

#if CFG_SUPPORT_BALANCE_MLR
	if (fgIsBeaconIntervalChange) {
		DBGLOG(RLM, TRACE,
			"Update Beacon info due to Beacon interval change\n");
		nicPmIndicateBssConnected(prAdapter, prBssInfo->ucBssIndex);
	}
#endif /* CFG_SUPPORT_BALANCE_MLR */

	return fgNewParameter;
}

static void rlmRecHtOpForClient(struct ADAPTER *prAdapter,
				struct IE_HT_OP *prHtOp,
				struct BSS_INFO *prBssInfo,
				uint8_t *pucPrimaryChannel)
{
	struct STA_RECORD *prStaRec;
	uint8_t ucBssIndex;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prBssInfo->eBand == BAND_6G) {
		DBGLOG(SCN, WARN, "Ignore HT OP IE at 6G\n");
		return;
	}
#endif
	if (!prHtOp || !prBssInfo || !RLM_NET_IS_11N(prBssInfo))
		return;

	prStaRec = prBssInfo->prStaRecOfAP;
	if (!prStaRec)
		return;
	ucBssIndex = prStaRec->ucBssIndex;

	/* Workaround that some APs fill primary channel field
	 * by its
	 * secondary channel, but its DS IE is correct 20110610
	 */
	if (*pucPrimaryChannel  == 0)
		*pucPrimaryChannel  = prHtOp->ucPrimaryChannel;
	prBssInfo->ucHtOpInfo1 = prHtOp->ucInfo1;
	prBssInfo->u2HtOpInfo2 = prHtOp->u2Info2;
	prBssInfo->u2HtOpInfo3 = prHtOp->u2Info3;

	/*Backup peer HT OP Info*/
	prStaRec->ucHtPeerOpInfo1 = prHtOp->ucInfo1;
	prStaRec->u2HtPeerOpInfo2 = prHtOp->u2Info2;

	if (!cnmBss40mBwPermitted(prAdapter, ucBssIndex)) {
		DBGLOG(RLM, TRACE, "ucHtOpInfo1 0x%x reset\n",
			prBssInfo->ucHtOpInfo1);
		prBssInfo->ucHtOpInfo1 &=
			~(HT_OP_INFO1_SCO |
			  HT_OP_INFO1_STA_CHNL_WIDTH);
	}

	if ((prBssInfo->ucHtOpInfo1 & HT_OP_INFO1_SCO) !=
	    CHNL_EXT_RES) {
		prBssInfo->eBssSCO = (enum ENUM_CHNL_EXT)(
			prBssInfo->ucHtOpInfo1 &
			HT_OP_INFO1_SCO);
		DBGLOG(RLM, TRACE, "SCO updated by HT OP: %d\n",
			prBssInfo->eBssSCO);
	}

	/* Revise by own OP BW */
	if (prBssInfo->fgIsOpChangeChannelWidth &&
	    prBssInfo->ucOpChangeChannelWidth == MAX_BW_20MHZ) {
		prBssInfo->ucHtOpInfo1 &=
			~(HT_OP_INFO1_SCO |
			  HT_OP_INFO1_STA_CHNL_WIDTH);
		prBssInfo->eBssSCO = CHNL_EXT_SCN;
		DBGLOG(RLM, TRACE,
			"SCO updated by own OP BW\n");
	}

	prBssInfo->eHtProtectMode = (enum ENUM_HT_PROTECT_MODE)(
		prBssInfo->u2HtOpInfo2 &
		HT_OP_INFO2_HT_PROTECTION);

	/* To do: process regulatory class 16 */
	if ((prBssInfo->u2HtOpInfo2 &
	     HT_OP_INFO2_OBSS_NON_HT_STA_PRESENT) &&
	    0 /* && regulatory class is 16 */)
		prBssInfo->eGfOperationMode =
			GF_MODE_DISALLOWED;
	else if (prBssInfo->u2HtOpInfo2 &
		 HT_OP_INFO2_NON_GF_HT_STA_PRESENT)
		prBssInfo->eGfOperationMode = GF_MODE_PROTECT;
	else
		prBssInfo->eGfOperationMode = GF_MODE_NORMAL;

	prBssInfo->eRifsOperationMode =
		(prBssInfo->ucHtOpInfo1 & HT_OP_INFO1_RIFS_MODE)
			? RIFS_MODE_NORMAL
			: RIFS_MODE_DISALLOWED;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Decide whether the bss color need update
 *
 * \param[in] prBssInfo       Pointer to the BssInfo
 * \param[in] prSwRfb         Pointer to the received frame
 * \param[in] ucBssColorInfo  The old Bss Color Info
 *
 * \return fgChangeBssColor   Need update or not
 */
/*----------------------------------------------------------------------------*/
#if (CFG_SUPPORT_802_11AX == 1)
#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
u_int8_t updateHeBssColor(struct BSS_INFO *prBssInfo,
				       struct SW_RFB *prSwRfb,
					   u_int8_t ucBssColorInfo)
{
	struct WLAN_BEACON_FRAME *prWlanBeaconFrame = NULL;
	u_int8_t fgChangeBssColor = FALSE;
	uint64_t u64BeaconTimestamp = 0; /* The timestamp of the beacon */

	/* Get beacon frame info */
	prWlanBeaconFrame = (struct WLAN_BEACON_FRAME *) prSwRfb->pvHeader;

	/* Get whole beacon timestamp */
	u64BeaconTimestamp =
		((uint64_t)prWlanBeaconFrame->au4Timestamp[1] << 32)
		| (prWlanBeaconFrame->au4Timestamp[0]);

	/* Timestamp >= expected, ready to change color */
	if ((u64BeaconTimestamp >= (prBssInfo->u64ExpectedTimestamp))
		&& (prBssInfo->u64ExpectedTimestamp != 0)) {

		fgChangeBssColor = TRUE;

		/*
		 * bit[7]   : disabled = 0
		 * bit[6]   : partial
		 * bit[5:0] : new color
		 */
		prBssInfo->ucBssColorInfo =
			(prBssInfo->ucBssColorInfo &
				HE_OP_BSSCOLOR_PARTIAL_BSS_COLOR) |
			(prBssInfo->ucNewBssColorInfo &
				HE_OP_BSSCOLOR_BSS_COLOR_MASK);

		DBGLOG(RLM, INFO,
			"End Countdown, BssColorInfo change from %x to %x. Update BSSInfo to FW\n",
			ucBssColorInfo, prBssInfo->ucBssColorInfo);

		/* Reset ExpectedTimestamp */
		prBssInfo->u64ExpectedTimestamp = 0;
		/* Reset ucColorSwitchCntdn */
		prBssInfo->ucColorSwitchCntdn = 0;
		/* Reset ucNewBssColorInfo */
		prBssInfo->ucNewBssColorInfo = 0;

		return fgChangeBssColor;
	}

	/* AP send announcement, will change bss color */
	if ((prBssInfo->ucColorAnnouncement) == TRUE) {

		DBGLOG(RLM, INFO,
			"Receive BssColorChangeAnnouncement\n");

		DBGLOG(RLM, INFO,
			"ucColorSwitchCntdn: 0x%x, ucNewBssColorInfo: 0x%x.\n",
				prBssInfo->ucColorSwitchCntdn,
				prBssInfo->ucNewBssColorInfo);

		/*
		 *  Retain original "disable" bit when receiving announcement.
		 *  In announcement phase, disable bit should be set to 1,
		 *  but we choose NOT to change the disabled bit.
		 *  Instead, retain to 0.
		 */
		prBssInfo->ucBssColorInfo = (prBssInfo->ucBssColorInfo)
			& (~HE_OP_BSSCOLOR_BSS_COLOR_DISABLE);

		/*
		 *  AP error case
		 *  Count not end but set NewColor = 0
		 */
		if ((prBssInfo->ucNewBssColorInfo == 0)
			&& (prBssInfo->ucColorSwitchCntdn > 0)) {

			/* Reset ExpectedTimestamp */
			prBssInfo->u64ExpectedTimestamp = 0;
			/* Reset ucColorSwitchCntdn */
			prBssInfo->ucColorSwitchCntdn = 0;

			return fgChangeBssColor;
		}

		/* Receive Cntdn = 0, update New color */
		if (prBssInfo->ucColorSwitchCntdn == 0) {
			fgChangeBssColor = TRUE;

			/*
			 * bit[7]   : disabled = 0
			 * bit[6]   : partial
			 * bit[5:0] : new color
			 */
			prBssInfo->ucBssColorInfo =
				(prBssInfo->ucBssColorInfo &
					HE_OP_BSSCOLOR_PARTIAL_BSS_COLOR) |
				(prBssInfo->ucNewBssColorInfo &
					HE_OP_BSSCOLOR_BSS_COLOR_MASK);

			DBGLOG(RLM, INFO,
				"Countdown = 0, BssColorInfo change from %x to %x. Update BSSInfo to FW\n",
				ucBssColorInfo, prBssInfo->ucBssColorInfo);

			/* Reset ExpectedTimestamp */
			prBssInfo->u64ExpectedTimestamp = 0;
			/* Reset ucNewBssColorInfo */
			prBssInfo->ucNewBssColorInfo = 0;

			return fgChangeBssColor;
		}

		/*
		 * General countdown case : countdown != 0
		 * Store expected timestamp to know when to update bss color
		 *
		 * Time unit
		 * u64BeaconTimestamp : us
		 * u2BeaconInterval : ms
		 */
		prBssInfo->u64ExpectedTimestamp = u64BeaconTimestamp +
			((uint64_t)((prWlanBeaconFrame->u2BeaconInterval)
			*(prBssInfo->ucColorSwitchCntdn)) * 1000);
			/* 1000 for ms -> us */

	} else if (((prBssInfo->ucBssColorInfo &
			HE_OP_BSSCOLOR_BSS_COLOR_DISABLE) !=
			(ucBssColorInfo & HE_OP_BSSCOLOR_BSS_COLOR_DISABLE)) &&
			(prBssInfo->ucNewBssColorInfo == 0)) {
		/*
		 * Only BssColorInfo "disable" bit changed,
		 * NO NEED to change color.
		 */
		fgChangeBssColor = TRUE;
		DBGLOG(RLM, INFO,
			"BssColorInfo is changed from %x to %x. Update BSSInfo to FW\n",
			ucBssColorInfo, prBssInfo->ucBssColorInfo);
	}

	return fgChangeBssColor;
}
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */
#endif /* CFG_SUPPORT_802_11AX */

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmProcessBcn(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
		   uint8_t *pucIE, uint16_t u2IELength)
{
	struct BSS_INFO *prBssInfo;
	struct WLAN_BEACON_FRAME *prWlanBeacon = NULL;
	u_int8_t fgNewParameter;
#if (CFG_SUPPORT_802_11AX == 1)
	u_int8_t fgNewSRParam = FALSE;
#endif
	uint8_t i;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	ASSERT(pucIE);

	fgNewParameter = FALSE;

	/* When concurrent networks exist, GO shall have the same handle as
	 * the other BSS, so the Beacon shall be processed for bandwidth and
	 * protection mechanism.
	 * Note1: we do not have 2 AP (GO) cases simultaneously now.
	 * Note2: If we are GO, concurrent AIS AP should detect it and reflect
	 *        action in its Beacon, so AIS STA just follows Beacon from AP.
	 */
	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

		if (prBssInfo == NULL)
			continue;

		if (IS_BSS_BOW(prBssInfo))
			continue;

		if (IS_BSS_ACTIVE(prBssInfo)) {
			if (prBssInfo->eCurrentOPMode ==
				    OP_MODE_INFRASTRUCTURE &&
			    prBssInfo->eConnectionState ==
				    MEDIA_STATE_CONNECTED) {
				/* P2P client or AIS infra STA */
				if (prBssInfo->eIftype == IFTYPE_P2P_CLIENT &&
					prBssInfo->fgIsSwitchingChnl) {
					DBGLOG(RLM, INFO,
						"Ignore rlm update when switching channel\n");
					continue;
				}

				if (IS_AIS_ROAMING(prAdapter,
					prBssInfo->ucBssIndex) ||
				    IS_AIS_OFF_CHNL(prAdapter,
					prBssInfo->ucBssIndex)) {
					DBGLOG(RLM, INFO,
						"Ignore rlm update when roaming/offchnl\n");
					continue;
				}

				if (EQUAL_MAC_ADDR(
					    prBssInfo->aucBSSID,
					    ((struct WLAN_MAC_MGMT_HEADER
						      *)(prSwRfb->pvHeader))
						    ->aucBSSID)) {

					prWlanBeacon =
						(struct WLAN_BEACON_FRAME *)
							(prSwRfb->pvHeader);

					if (prBssInfo->prStaRecOfAP) {
						(prBssInfo->prStaRecOfAP
							->au4Timestamp[0]) =
						prWlanBeacon->au4Timestamp[0];
						(prBssInfo->prStaRecOfAP
							->au4Timestamp[1]) =
						prWlanBeacon->au4Timestamp[1];
					}

					fgNewParameter = rlmRecBcnInfoForClient(
						prAdapter, prBssInfo, prSwRfb,
						pucIE, u2IELength);
#if (CFG_SUPPORT_802_11AX == 1)
					if (fgEfuseCtrlAxOn == 1) {
					fgNewSRParam = heRlmRecHeSRParams(
						prAdapter, prBssInfo,
						prSwRfb, pucIE, u2IELength);
					}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
					/* TODO */
#endif
				} else {
					fgNewParameter =
						rlmRecBcnFromNeighborForClient(
							prAdapter, prBssInfo,
							prSwRfb, pucIE,
							u2IELength);
				}
			}
#if CFG_ENABLE_WIFI_DIRECT
			else if (prAdapter->fgIsP2PRegistered &&
				 (prBssInfo->eCurrentOPMode ==
					  OP_MODE_ACCESS_POINT ||
				  prBssInfo->eCurrentOPMode ==
					  OP_MODE_P2P_DEVICE)) {
				/* AP scan to check if 20/40M bandwidth is
				 * permitted
				 */
				rlmRecBcnFromNeighborForClient(
					prAdapter, prBssInfo, prSwRfb, pucIE,
					u2IELength);
			}
#endif
			else if (prBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
				/* To do: Nothing */
				/* To do: Ad-hoc */
			}

			/* Appy new parameters if necessary */
			if (fgNewParameter) {
				nicUpdateBss(prAdapter, prBssInfo->ucBssIndex);
				fgNewParameter = FALSE;
			}
#if (CFG_SUPPORT_802_11AX == 1)
			if (fgEfuseCtrlAxOn == 1) {
				if (fgNewSRParam) {
					nicRlmUpdateSRParams(prAdapter,
						prBssInfo->ucBssIndex);
					fgNewSRParam = FALSE;
				}
			}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
			/* TODO */
#endif

		} /* end of IS_BSS_ACTIVE() */
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function should be invoked after judging successful association.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmProcessAssocRsp(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
			const uint8_t *pucIE, uint16_t u2IELength)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucPriChannel;
#if (CFG_SUPPORT_802_11AX == 1)
	uint8_t fgNewSRParam;
#endif

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	ASSERT(pucIE);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return;

	if (prStaRec != prBssInfo->prStaRecOfAP)
		return;

	/* To do: the invoked function is used to clear all members. It may be
	 *        done by center mechanism in invoker.
	 */
	rlmBssReset(prAdapter, prBssInfo);

	prBssInfo->fgUseShortSlotTime =
		((prBssInfo->u2CapInfo & CAP_INFO_SHORT_SLOT_TIME) ||
		 (prBssInfo->eBand != BAND_2G4))
			? TRUE
			: FALSE;
	ucPriChannel =
		rlmRecIeInfoForClient(prAdapter, prBssInfo, pucIE, u2IELength);

	/*Update the parameters from Association Response only,
	 *if the parameters need to be updated by both Beacon and Association
	 *Response,
	 *user should use another function, rlmRecIeInfoForClient()
	 */
	rlmRecAssocRespIeInfoForClient(prAdapter, prBssInfo, pucIE, u2IELength);

	if (prBssInfo->ucPrimaryChannel != ucPriChannel) {
		DBGLOG(RLM, INFO,
		       "Use RF pri channel[%u].Pri channel in HT OP IE is :[%u]\n",
		       prBssInfo->ucPrimaryChannel, ucPriChannel);
	}
	/* Avoid wrong primary channel info in HT operation
	 * IE info when accept association response
	 */
#if 0
	if (ucPriChannel > 0)
		prBssInfo->ucPrimaryChannel = ucPriChannel;
#endif

	if (!RLM_NET_IS_11N(prBssInfo) ||
	    !(prStaRec->u2HtCapInfo & HT_CAP_INFO_SUP_CHNL_WIDTH))
		prBssInfo->fg40mBwAllowed = FALSE;

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		fgNewSRParam = heRlmRecHeSRParams(prAdapter, prBssInfo,
						prSwRfb, pucIE, u2IELength);
		/* ASSERT(fgNewSRParam); */
		nicRlmUpdateSRParams(prAdapter, prBssInfo->ucBssIndex);
	}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
		/* TODO */
#endif

	/* Note: Update its capabilities to WTBL by cnmStaRecChangeState(),
	 * which
	 *       shall be invoked afterwards.
	 *       Update channel, bandwidth and protection mode by nicUpdateBss()
	 */
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
void rlmProcessHtAction(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct ACTION_NOTIFY_CHNL_WIDTH_FRAME *prRxFrame;
	struct ACTION_SM_POWER_SAVE_FRAME *prRxSmpsFrame;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint16_t u2HtCapInfoBitmask = 0;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint16_t u2HeCapInfoBitmask = 0;
#endif

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prRxFrame = (struct ACTION_NOTIFY_CHNL_WIDTH_FRAME *)prSwRfb->pvHeader;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);

	if (!prStaRec)
		return;

	switch (prRxFrame->ucAction) {
	case ACTION_HT_NOTIFY_CHANNEL_WIDTH:
		if (prStaRec->ucStaState != STA_STATE_3 ||
		    prSwRfb->u2PacketLen <
			    sizeof(struct ACTION_NOTIFY_CHNL_WIDTH_FRAME)) {
			return;
		}

		/* To do: depending regulation class 13 and 14 based on spec
		 * Note: (ucChannelWidth==1) shall restored back to original
		 * capability, not current setting to 40MHz BW here
		 */
		/* 1. Update StaRec for AP/STA mode */
		if (prRxFrame->ucChannelWidth == HT_NOTIFY_CHANNEL_WIDTH_20)
			prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_SUP_CHNL_WIDTH;
		else if (prRxFrame->ucChannelWidth ==
			 HT_NOTIFY_CHANNEL_WIDTH_ANY_SUPPORT_CAHNNAEL_WIDTH)
			prStaRec->u2HtCapInfo |= HT_CAP_INFO_SUP_CHNL_WIDTH;

		cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);

		/* 2. Update BssInfo for STA mode */
		prBssInfo = prAdapter->aprBssInfo[prStaRec->ucBssIndex];
		if (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
			if (prRxFrame->ucChannelWidth ==
			    HT_NOTIFY_CHANNEL_WIDTH_20) {
				prBssInfo->ucHtOpInfo1 &=
					~HT_OP_INFO1_STA_CHNL_WIDTH;
				prBssInfo->eBssSCO = CHNL_EXT_SCN;
			} else if (
				prRxFrame->ucChannelWidth ==
	HT_NOTIFY_CHANNEL_WIDTH_ANY_SUPPORT_CAHNNAEL_WIDTH)
				prBssInfo->ucHtOpInfo1 |=
					HT_OP_INFO1_STA_CHNL_WIDTH;

			/* Revise by own OP BW if needed */
			if (prBssInfo->fgIsOpChangeChannelWidth &&
			    prBssInfo->ucOpChangeChannelWidth == MAX_BW_20MHZ) {
				prBssInfo->ucHtOpInfo1 &=
					~(HT_OP_INFO1_SCO |
					  HT_OP_INFO1_STA_CHNL_WIDTH);
				prBssInfo->eBssSCO = CHNL_EXT_SCN;
			}

			/* 3. Update OP BW to FW */
			rlmSyncOperationParams(prAdapter, prBssInfo);
		}
		break;
		/* Support SM power save */ /* TH3_Huang */
	case ACTION_HT_SM_POWER_SAVE:
		prRxSmpsFrame =
			(struct ACTION_SM_POWER_SAVE_FRAME *)prSwRfb->pvHeader;
		if (prStaRec->ucStaState != STA_STATE_3 ||
		    prSwRfb->u2PacketLen <
			    sizeof(struct ACTION_SM_POWER_SAVE_FRAME)) {
			return;
		}

		/* The SM power enable bit is different definition in HtCap and
		 * SMpower IE field
		 */
		if (!(prRxSmpsFrame->ucSmPowerCtrl &
		      (HT_SM_POWER_SAVE_CONTROL_ENABLED |
		       HT_SM_POWER_SAVE_CONTROL_SM_MODE)))
			u2HtCapInfoBitmask |= HT_CAP_INFO_SM_POWER_SAVE;

		/* Support SMPS action frame, TH3_Huang */
		/* Update StaRec if SM power state changed */
		if ((prStaRec->u2HtCapInfo & HT_CAP_INFO_SM_POWER_SAVE) !=
		    u2HtCapInfoBitmask) {
			prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_SM_POWER_SAVE;
			prStaRec->u2HtCapInfo |= u2HtCapInfoBitmask;
			DBGLOG(RLM, INFO,
				"rlmProcessHtAction -- SMPS change u2HtCapInfo to (%x)\n",
				prStaRec->u2HtCapInfo);
			cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
		}

#if (CFG_SUPPORT_WIFI_6G == 1)
		if (!(prRxSmpsFrame->ucSmPowerCtrl &
		      (HT_SM_POWER_SAVE_CONTROL_ENABLED |
		       HT_SM_POWER_SAVE_CONTROL_SM_MODE)))
			u2HeCapInfoBitmask |= HE_6G_CAP_INFO_SM_POWER_SAVE;

		/* Update StaRec if SM power state changed */
		if ((prStaRec->u2He6gBandCapInfo &
				HE_6G_CAP_INFO_SM_POWER_SAVE) !=
				u2HeCapInfoBitmask) {
			prStaRec->u2He6gBandCapInfo &=
				~HE_6G_CAP_INFO_SM_POWER_SAVE;
			prStaRec->u2He6gBandCapInfo |= u2HeCapInfoBitmask;
			DBGLOG(RLM, INFO,
				"rlmProcessHtAction -- SMPS change u2HeCapInfoBitmask to (%x)\n",
				prStaRec->u2He6gBandCapInfo);
			cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
		}
#endif
		break;
	default:
		break;
	}
}

#if CFG_SUPPORT_802_11AC
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmUpdateVhtOpCapInfo(struct ADAPTER *prAdapter,
	uint8_t ucVhtOpModeChannelWidth,
	struct STA_RECORD *prStaRec)
{
	if (ucVhtOpModeChannelWidth ==
		VHT_OP_MODE_CHANNEL_WIDTH_80) {
		prStaRec->ucVhtOpMode &=
			~VHT_OP_MODE_CHANNEL_WIDTH_20;
		prStaRec->ucVhtOpMode |=
			VHT_OP_MODE_CHANNEL_WIDTH_80;
		prStaRec->u4VhtCapInfo |=
			VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160;
		prStaRec->u4VhtCapInfo |=
			VHT_CAP_INFO_SHORT_GI_80;
		prStaRec->u4VhtCapInfo |=
			VHT_CAP_INFO_SHORT_GI_160_80P80;
	} else if (ucVhtOpModeChannelWidth ==
		VHT_OP_MODE_CHANNEL_WIDTH_20) {
		prStaRec->ucVhtOpMode &=
			~VHT_OP_MODE_CHANNEL_WIDTH_80;
		prStaRec->ucVhtOpMode |=
			VHT_OP_MODE_CHANNEL_WIDTH_20;
		prStaRec->u4VhtCapInfo &=
			~VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160;
		prStaRec->u4VhtCapInfo &=
			~VHT_CAP_INFO_SHORT_GI_80;
		prStaRec->u4VhtCapInfo &=
			~VHT_CAP_INFO_SHORT_GI_160_80P80;
	}
}
void rlmProcessVhtAction(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct ACTION_OP_MODE_NOTIFICATION_FRAME *prRxFrame;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint8_t ucVhtOpModeChannelWidth = 0;
	uint8_t ucMaxBw;
	uint8_t ucOperatingMode;
#if (CFG_SUPPORT_802_11AX == 1)
	uint8_t fgIsRx1ss = false;
	uint16_t u2HeRxMcsMapAssoc;
#endif

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prRxFrame =
		(struct ACTION_OP_MODE_NOTIFICATION_FRAME *)prSwRfb->pvHeader;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);

	if (!prStaRec)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo)
		return;

	ucMaxBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);

	switch (prRxFrame->ucAction) {
	/* Support Operating mode notification action frame, TH3_Huang */
	case ACTION_OPERATING_MODE_NOTIFICATION:
		if (prStaRec->ucStaState != STA_STATE_3 ||
		    prSwRfb->u2PacketLen <
			    sizeof(struct ACTION_OP_MODE_NOTIFICATION_FRAME)) {
			return;
		}

		ucOperatingMode = prRxFrame->ucOperatingMode;

		if (((ucOperatingMode & VHT_OP_MODE_RX_NSS_TYPE) !=
		     VHT_OP_MODE_RX_NSS_TYPE) &&
		    (prStaRec->ucVhtOpMode != ucOperatingMode)) {
			/* 1. Fill OP mode notification info */
			prStaRec->ucVhtOpMode = ucOperatingMode;
			DBGLOG(RLM, INFO,
			       "rlmProcessVhtAction -- Update ucVhtOpMode to 0x%x\n",
			       prStaRec->ucVhtOpMode);

			/* 2. Modify channel width parameters */
			ucVhtOpModeChannelWidth = ucOperatingMode &
						  VHT_OP_MODE_CHANNEL_WIDTH;
			if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
				if (ucVhtOpModeChannelWidth ==
				    VHT_OP_MODE_CHANNEL_WIDTH_20)
					prStaRec->u2HtCapInfo &=
						~HT_CAP_INFO_SUP_CHNL_WIDTH;
				else /* for other 3 VHT cases: 40/80/160 */
					prStaRec->u2HtCapInfo |=
						HT_CAP_INFO_SUP_CHNL_WIDTH;

				rlmUpdateVhtOpCapInfo(prAdapter,
					ucVhtOpModeChannelWidth,
					prStaRec);

			} else if (prBssInfo->eCurrentOPMode ==
				   OP_MODE_INFRASTRUCTURE)
				rlmRecOpModeBwForClient(ucVhtOpModeChannelWidth,
							prBssInfo);

			/* 3. Update StaRec to FW */
			/* As defined in spec, 11 means not support this MCS */
			if (((ucOperatingMode & VHT_OP_MODE_RX_NSS)
				>> VHT_OP_MODE_RX_NSS_OFFSET) ==
				VHT_OP_MODE_NSS_2) {
				prStaRec->u2VhtRxMcsMap = BITS(0, 15) &
					(~(VHT_CAP_INFO_MCS_1SS_MASK |
					VHT_CAP_INFO_MCS_2SS_MASK));

				prStaRec->u2VhtRxMcsMap |=
					(prStaRec->u2VhtRxMcsMapAssoc &
					(VHT_CAP_INFO_MCS_1SS_MASK |
					VHT_CAP_INFO_MCS_2SS_MASK));

				DBGLOG(RLM, INFO,
				       "NSS=2 RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
				       prStaRec->u2VhtRxMcsMap,
				       prStaRec->u2VhtRxMcsMapAssoc);
			} else {
				/* NSS = 1 or others */
				prStaRec->u2VhtRxMcsMap = BITS(0, 15) &
					(~VHT_CAP_INFO_MCS_1SS_MASK);

				prStaRec->u2VhtRxMcsMap |=
					(prStaRec->u2VhtRxMcsMapAssoc &
					VHT_CAP_INFO_MCS_1SS_MASK);

				DBGLOG(RLM, INFO,
				       "NSS=1 RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
				       prStaRec->u2VhtRxMcsMap,
				       prStaRec->u2VhtRxMcsMapAssoc);
			}

#if (CFG_SUPPORT_802_11AX == 1)
			u2HeRxMcsMapAssoc = prStaRec->u2HeRxMcsMapBW80Assoc;
			if (((ucOperatingMode & VHT_OP_MODE_RX_NSS)
				>> VHT_OP_MODE_RX_NSS_OFFSET) ==
				VHT_OP_MODE_NSS_2) {
				prStaRec->u2HeRxMcsMapBW80 = BITS(0, 15) &
					(~(HE_CAP_INFO_MCS_1SS_MASK |
					HE_CAP_INFO_MCS_2SS_MASK));

				prStaRec->u2HeRxMcsMapBW80 |=
					(u2HeRxMcsMapAssoc &
					(HE_CAP_INFO_MCS_1SS_MASK |
					HE_CAP_INFO_MCS_2SS_MASK));
			} else {
				/* NSS = 1 or others */
				fgIsRx1ss = true;

				prStaRec->u2HeRxMcsMapBW80 = BITS(0, 15) &
					(~HE_CAP_INFO_MCS_1SS_MASK);

				prStaRec->u2HeRxMcsMapBW80 |=
					(u2HeRxMcsMapAssoc &
					HE_CAP_INFO_MCS_1SS_MASK);
			}

			DBGLOG(RLM, INFO,
				"HeBw80, NSS=%d, RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
				fgIsRx1ss ? 1 : 2,
				prStaRec->u2HeRxMcsMapBW80,
				prStaRec->u2HeRxMcsMapBW80Assoc);

			if (ucMaxBw >= MAX_BW_160MHZ) {
				u2HeRxMcsMapAssoc =
					prStaRec->u2HeRxMcsMapBW160Assoc;
				if (((ucOperatingMode & VHT_OP_MODE_RX_NSS)
					>> VHT_OP_MODE_RX_NSS_OFFSET) ==
					VHT_OP_MODE_NSS_2) {
					prStaRec->u2HeRxMcsMapBW160 =
						BITS(0, 15) &
						(~(HE_CAP_INFO_MCS_1SS_MASK |
						HE_CAP_INFO_MCS_2SS_MASK));

					prStaRec->u2HeRxMcsMapBW160 |=
						(u2HeRxMcsMapAssoc &
						(HE_CAP_INFO_MCS_1SS_MASK |
						HE_CAP_INFO_MCS_2SS_MASK));
				} else {
					/* NSS = 1 or others */
					prStaRec->u2HeRxMcsMapBW160 =
						BITS(0, 15) &
						(~HE_CAP_INFO_MCS_1SS_MASK);

					prStaRec->u2HeRxMcsMapBW160 |=
						(u2HeRxMcsMapAssoc &
						HE_CAP_INFO_MCS_1SS_MASK);
				}

				DBGLOG(RLM, INFO,
					"HeBw160, NSS=%d, RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
					fgIsRx1ss ? 1 : 2,
					prStaRec->u2HeRxMcsMapBW160,
					prStaRec->u2HeRxMcsMapBW160Assoc);
			}
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (fgIsRx1ss)
				prStaRec->u2He6gBandCapInfo &=
					~HE_6G_CAP_INFO_SM_POWER_SAVE;
			else
				prStaRec->u2He6gBandCapInfo |=
					HE_6G_CAP_INFO_SM_POWER_SAVE;
#endif
#endif /* CFG_SUPPORT_802_11AX == 1 */

			cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
			cnmDumpStaRec(prAdapter, prStaRec->ucIndex);
			/* 4. Update BW parameters in BssInfo for STA mode only
			 */
			if (prBssInfo->eCurrentOPMode ==
			    OP_MODE_INFRASTRUCTURE) {
				/* 4.1 Revise by own OP BW if needed for STA
				 * mode only
				 */
				if (prBssInfo->fgIsOpChangeChannelWidth) {
					/* VHT */
					if (rlmGetVhtOpBwByBssOpBw(
					prBssInfo
					->ucOpChangeChannelWidth) <
						prBssInfo->ucVhtChannelWidth)
						rlmFillVhtOpInfoByBssOpBw(
						prAdapter,
						prBssInfo,
						prBssInfo
						->ucOpChangeChannelWidth);
					/* HT */
					if (prBssInfo
					->fgIsOpChangeChannelWidth &&
					    prBssInfo->ucOpChangeChannelWidth ==
						    MAX_BW_20MHZ) {
						prBssInfo->ucHtOpInfo1 &= ~(
						HT_OP_INFO1_SCO |
						HT_OP_INFO1_STA_CHNL_WIDTH);
						prBssInfo->eBssSCO =
							CHNL_EXT_SCN;
					}
				}

				/* 4.2 Check if OP BW parameter valid */
				if (!rlmDomainIsValidRfSetting(
					    prAdapter, prBssInfo->eBand,
					    prBssInfo->ucPrimaryChannel,
					    prBssInfo->eBssSCO,
					    prBssInfo->ucVhtChannelWidth,
					    prBssInfo->ucVhtChannelFrequencyS1,
				prBssInfo->ucVhtChannelFrequencyS2)) {

					DBGLOG(RLM, WARN,
					       "rlmProcessVhtAction invalid RF settings\n");

					/* Error Handling for Non-predicted IE -
					 * Fixed to set 20MHz
					 */
					prBssInfo->ucVhtChannelWidth =
						CW_20_40MHZ;
					prBssInfo->ucVhtChannelFrequencyS1 = 0;
					prBssInfo->ucVhtChannelFrequencyS2 = 0;
					prBssInfo->eBssSCO = CHNL_EXT_SCN;
					prBssInfo->ucHtOpInfo1 &=
						~(HT_OP_INFO1_SCO |
						  HT_OP_INFO1_STA_CHNL_WIDTH);
				}

				/* 4.3 Update BSS OP BW to FW for STA mode only
				 */
				rlmSyncOperationParams(prAdapter, prBssInfo);
			}
		}
		break;
	default:
		break;
	}
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function should be invoked after judging successful association.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmFillSyncCmdParam(struct CMD_SET_BSS_RLM_PARAM *prCmdBody,
			 struct BSS_INFO *prBssInfo)
{
	if (!prCmdBody || !prBssInfo)
		return;

	prCmdBody->ucBssIndex = prBssInfo->ucBssIndex;
	prCmdBody->ucRfBand = (uint8_t)prBssInfo->eBand;
	prCmdBody->ucPrimaryChannel = prBssInfo->ucPrimaryChannel;
	prCmdBody->ucRfSco = (uint8_t)prBssInfo->eBssSCO;
	prCmdBody->ucErpProtectMode = (uint8_t)prBssInfo->fgErpProtectMode;
	prCmdBody->ucHtProtectMode = (uint8_t)prBssInfo->eHtProtectMode;
	prCmdBody->ucGfOperationMode = (uint8_t)prBssInfo->eGfOperationMode;
	prCmdBody->ucTxRifsMode = (uint8_t)prBssInfo->eRifsOperationMode;
	prCmdBody->u2HtOpInfo3 = prBssInfo->u2HtOpInfo3;
	prCmdBody->u2HtOpInfo2 = prBssInfo->u2HtOpInfo2;
	prCmdBody->ucHtOpInfo1 = prBssInfo->ucHtOpInfo1;
	prCmdBody->ucUseShortPreamble = prBssInfo->fgUseShortPreamble;
	prCmdBody->ucUseShortSlotTime = prBssInfo->fgUseShortSlotTime;
	prCmdBody->ucVhtChannelWidth = prBssInfo->ucVhtChannelWidth;
	prCmdBody->ucVhtChannelFrequencyS1 = prBssInfo->ucVhtChannelFrequencyS1;
	prCmdBody->ucVhtChannelFrequencyS2 = prBssInfo->ucVhtChannelFrequencyS2;
	prCmdBody->u2VhtBasicMcsSet = prBssInfo->u2BSSBasicRateSet;
	prCmdBody->ucTxNss = prBssInfo->ucOpTxNss;
	prCmdBody->ucRxNss = prBssInfo->ucOpRxNss;

	if (RLM_NET_PARAM_VALID(prBssInfo)) {
		DBGLOG(RLM, INFO,
		       "N=%d b=%d c=%d s=%d e=%d h=%d I=0x%02x l=%d p=%d w(vht)=%d %s s1=%d s2=%d RxN=%d, TxN=%d\n",
		       prCmdBody->ucBssIndex, prCmdBody->ucRfBand,
		       prCmdBody->ucPrimaryChannel, prCmdBody->ucRfSco,
		       prCmdBody->ucErpProtectMode, prCmdBody->ucHtProtectMode,
		       prCmdBody->ucHtOpInfo1, prCmdBody->ucUseShortSlotTime,
		       prCmdBody->ucUseShortPreamble,
		       prCmdBody->ucVhtChannelWidth,
		       apucVhtOpBw[prCmdBody->ucVhtChannelWidth],
		       prCmdBody->ucVhtChannelFrequencyS1,
		       prCmdBody->ucVhtChannelFrequencyS2,
		       prCmdBody->ucRxNss,
		       prCmdBody->ucTxNss);
	} else {
		DBGLOG(RLM, INFO, "N=%d closed\n", prCmdBody->ucBssIndex);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will operation parameters based on situations of
 *        concurrent networks. Channel, bandwidth, protection mode, supported
 *        rate will be modified.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmSyncOperationParams(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo)
{
	struct CMD_SET_BSS_RLM_PARAM *prCmdBody;
	uint32_t rStatus;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	prCmdBody = (struct CMD_SET_BSS_RLM_PARAM *)cnmMemAlloc(
		prAdapter, RAM_TYPE_BUF, sizeof(struct CMD_SET_BSS_RLM_PARAM));

	/* ASSERT(prCmdBody); */
	/* To do: exception handle */
	if (!prCmdBody) {
		DBGLOG(RLM, WARN, "No buf for sync RLM params (Net=%d)\n",
		       prBssInfo->ucBssIndex);
		return;
	}

	rlmFillSyncCmdParam(prCmdBody, prBssInfo);

	rStatus = wlanSendSetQueryCmd(
		prAdapter,			      /* prAdapter */
		CMD_ID_SET_BSS_RLM_PARAM,	     /* ucCID */
		TRUE,				      /* fgSetQuery */
		FALSE,				      /* fgNeedResp */
		FALSE,				      /* fgIsOid */
		NULL,				      /* pfCmdDoneHandler */
		NULL,				      /* pfCmdTimeoutHandler */
		sizeof(struct CMD_SET_BSS_RLM_PARAM), /* u4SetQueryInfoLen */
		(uint8_t *)prCmdBody,		      /* pucInfoBuffer */
		NULL,				      /* pvSetQueryBuffer */
		0				      /* u4SetQueryBufferLen */
		);

	/* ASSERT(rStatus == WLAN_STATUS_PENDING); */
	if (rStatus != WLAN_STATUS_PENDING)
		DBGLOG(RLM, WARN, "rlmSyncOperationParams set cmd fail\n");

	cnmMemFree(prAdapter, prCmdBody);
}

#if CFG_SUPPORT_AAA
/*----------------------------------------------------------------------------*/
/*!
 * \brief This function should be invoked after judging successful association.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmProcessAssocReq(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
			uint8_t *pucIE, uint16_t u2IELength)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint16_t u2Offset;
	struct IE_HT_CAP *prHtCap;
#if CFG_SUPPORT_802_11AC
	struct IE_VHT_CAP *prVhtCap = NULL;
	struct IE_OP_MODE_NOTIFICATION
		*prOPModeNotification = NULL; /* Operation Mode Notification */
	u_int8_t fgHasOPModeIE = FALSE;
#endif

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	ASSERT(pucIE);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return;
	if (prStaRec->ucBssIndex > prAdapter->ucSwBssIdNum)
		return;

	prBssInfo = prAdapter->aprBssInfo[prStaRec->ucBssIndex];

	IE_FOR_EACH(pucIE, u2IELength, u2Offset)
	{
		switch (IE_ID(pucIE)) {
		case ELEM_ID_HT_CAP:
			if (!RLM_NET_IS_11N(prBssInfo) ||
			    IE_LEN(pucIE) != (sizeof(struct IE_HT_CAP) - 2))
				break;
			prHtCap = (struct IE_HT_CAP *)pucIE;
			prStaRec->ucMcsSet =
				prHtCap->rSupMcsSet.aucRxMcsBitmask[0];
			prStaRec->fgSupMcs32 =
				(prHtCap->rSupMcsSet.aucRxMcsBitmask[32 / 8] &
				 BIT(0))
					? TRUE
					: FALSE;

			kalMemCopy(
				prStaRec->aucRxMcsBitmask,
				prHtCap->rSupMcsSet.aucRxMcsBitmask,
				/*SUP_MCS_RX_BITMASK_OCTET_NUM */
				sizeof(prStaRec->aucRxMcsBitmask));

			prStaRec->u2HtCapInfo = prHtCap->u2HtCapInfo;

			/* Set Short LDPC Tx capability */
			if (IS_FEATURE_FORCE_ENABLED(
				    prAdapter->rWifiVar.ucTxLdpc))
				prStaRec->u2HtCapInfo |= HT_CAP_INFO_LDPC_CAP;
			else if (IS_FEATURE_DISABLED(
					 prAdapter->rWifiVar.ucTxLdpc))
				prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_LDPC_CAP;

			/* Set STBC Tx capability */
			if (IS_FEATURE_FORCE_ENABLED(
				    prAdapter->rWifiVar.ucTxStbc))
				prStaRec->u2HtCapInfo |= HT_CAP_INFO_TX_STBC;
			else if (IS_FEATURE_DISABLED(
					 prAdapter->rWifiVar.ucTxStbc))
				prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_TX_STBC;
			/* Set Short GI Tx capability */
			if (IS_FEATURE_FORCE_ENABLED(
				    prAdapter->rWifiVar.ucTxShortGI)) {
				prStaRec->u2HtCapInfo |=
					HT_CAP_INFO_SHORT_GI_20M;
				prStaRec->u2HtCapInfo |=
					HT_CAP_INFO_SHORT_GI_40M;
			} else if (IS_FEATURE_DISABLED(
					   prAdapter->rWifiVar.ucTxShortGI)) {
				prStaRec->u2HtCapInfo &=
					~HT_CAP_INFO_SHORT_GI_20M;
				prStaRec->u2HtCapInfo &=
					~HT_CAP_INFO_SHORT_GI_40M;
			}

			/* Set HT Greenfield Tx capability */
			if (IS_FEATURE_FORCE_ENABLED(
				    prAdapter->rWifiVar.ucTxGf))
				prStaRec->u2HtCapInfo |= HT_CAP_INFO_HT_GF;
			else if (IS_FEATURE_DISABLED(
					 prAdapter->rWifiVar.ucTxGf))
				prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_HT_GF;

			prStaRec->ucAmpduParam = prHtCap->ucAmpduParam;
			prStaRec->u2HtExtendedCap = prHtCap->u2HtExtendedCap;
			prStaRec->u4TxBeamformingCap =
				prHtCap->u4TxBeamformingCap;
			prStaRec->ucAselCap = prHtCap->ucAselCap;
			break;

		case ELEM_ID_RESERVED:
#if (CFG_SUPPORT_802_11AX == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_CAP)
				heRlmRecHeCapInfo(prAdapter, prStaRec, pucIE);
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_6G_BAND_CAP)
				heRlmRecHe6GCapInfo(prAdapter, prStaRec, pucIE);
#endif
#endif
#if (CFG_SUPPORT_802_11BE == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_EHT_CAPS)
				ehtRlmRecCapInfo(prAdapter, prStaRec, pucIE);
#endif
			break;

#if CFG_SUPPORT_802_11AC
		case ELEM_ID_VHT_CAP:
			if (!RLM_NET_IS_11AC(prBssInfo) ||
			    IE_LEN(pucIE) != (sizeof(struct IE_VHT_CAP) - 2))
				break;

			prVhtCap = (struct IE_VHT_CAP *)pucIE;

			prStaRec->u4VhtCapInfo = prVhtCap->u4VhtCapInfo;

			/* Set Tx LDPC capability */
			if (IS_FEATURE_FORCE_ENABLED(
				    prAdapter->rWifiVar.ucTxLdpc))
				prStaRec->u4VhtCapInfo |= VHT_CAP_INFO_RX_LDPC;
			else if (IS_FEATURE_DISABLED(
					 prAdapter->rWifiVar.ucTxLdpc))
				prStaRec->u4VhtCapInfo &= ~VHT_CAP_INFO_RX_LDPC;

			/* Set Tx STBC capability */
			if (IS_FEATURE_FORCE_ENABLED(
				    prAdapter->rWifiVar.ucTxStbc))
				prStaRec->u4VhtCapInfo |= VHT_CAP_INFO_TX_STBC;
			else if (IS_FEATURE_DISABLED(
					 prAdapter->rWifiVar.ucTxStbc))
				prStaRec->u4VhtCapInfo &= ~VHT_CAP_INFO_TX_STBC;

			/* Set Tx TXOP PS capability */
			if (IS_FEATURE_FORCE_ENABLED(
				    prAdapter->rWifiVar.ucTxopPsTx))
				prStaRec->u4VhtCapInfo |=
					VHT_CAP_INFO_VHT_TXOP_PS;
			else if (IS_FEATURE_DISABLED(
					 prAdapter->rWifiVar.ucTxopPsTx))
				prStaRec->u4VhtCapInfo &=
					~VHT_CAP_INFO_VHT_TXOP_PS;

			/* Set Tx Short GI capability */
			if (IS_FEATURE_FORCE_ENABLED(
				    prAdapter->rWifiVar.ucTxShortGI)) {
				prStaRec->u4VhtCapInfo |=
					VHT_CAP_INFO_SHORT_GI_80;
				prStaRec->u4VhtCapInfo |=
					VHT_CAP_INFO_SHORT_GI_160_80P80;
			} else if (IS_FEATURE_DISABLED(
					   prAdapter->rWifiVar.ucTxShortGI)) {
				prStaRec->u4VhtCapInfo &=
					~VHT_CAP_INFO_SHORT_GI_80;
				prStaRec->u4VhtCapInfo &=
					~VHT_CAP_INFO_SHORT_GI_160_80P80;
			}

			prStaRec->u2VhtRxMcsMap =
				prVhtCap->rVhtSupportedMcsSet.u2RxMcsMap;

			prStaRec->u2VhtRxMcsMapAssoc =
				prStaRec->u2VhtRxMcsMap;

			prStaRec->u2VhtRxHighestSupportedDataRate =
				prVhtCap->rVhtSupportedMcsSet
					.u2RxHighestSupportedDataRate;
			prStaRec->u2VhtTxMcsMap =
				prVhtCap->rVhtSupportedMcsSet.u2TxMcsMap;
			prStaRec->u2VhtTxHighestSupportedDataRate =
				prVhtCap->rVhtSupportedMcsSet
					.u2TxHighestSupportedDataRate;

			/* Set initial value of VHT OP mode */
			prStaRec->ucVhtOpMode = 0;
			prStaRec->ucVhtOpMode |=
				rlmGetOpModeBwByVhtAndHtOpInfo(prBssInfo);
			prStaRec->ucVhtOpMode |=
				((rlmGetSupportRxNssInVhtCap(prVhtCap) - 1)
				 << VHT_OP_MODE_RX_NSS_OFFSET) &
				VHT_OP_MODE_RX_NSS;
			break;

		case ELEM_ID_OP_MODE:
			if (IE_LEN(pucIE) !=
				    (sizeof(struct IE_OP_MODE_NOTIFICATION) -
				     2))
				break;

			if (!(RLM_NET_IS_11AC(prBssInfo)
#if (CFG_SUPPORT_WIFI_6G == 1)
				|| (prBssInfo->eBand == BAND_6G &&
					RLM_NET_IS_11AX(prBssInfo))
#endif
			))
				break;

			prOPModeNotification =
				(struct IE_OP_MODE_NOTIFICATION *)pucIE;

			if ((prOPModeNotification->ucOpMode &
			     VHT_OP_MODE_RX_NSS_TYPE) !=
			    VHT_OP_MODE_RX_NSS_TYPE) {
				fgHasOPModeIE = TRUE;

				/* Check BIT(2) is BW80P80_160.
				 * Refactor ucOpMode with ucVhtOpMode format.
				 */
				if (prOPModeNotification->ucOpMode &
					VHT_OP_MODE_CHANNEL_WIDTH_80P80_160) {
					prOPModeNotification->ucOpMode &=
					~(VHT_OP_MODE_CHANNEL_WIDTH |
					VHT_OP_MODE_CHANNEL_WIDTH_80P80_160);

					prOPModeNotification->ucOpMode |=
					(VHT_OP_MODE_CHANNEL_WIDTH_160_80P80 &
					VHT_OP_MODE_CHANNEL_WIDTH);
				}
			}
			break;
#endif
		default:
			break;
		} /* end of switch */
	}	 /* end of IE_FOR_EACH */
#if CFG_SUPPORT_802_11AC
	/*Fill by OP Mode IE after completing parsing all IE to make sure it
	 * won't be overwrite
	 */
	if (fgHasOPModeIE == TRUE)
		prStaRec->ucVhtOpMode = prOPModeNotification->ucOpMode;

#if (CFG_SUPPORT_802_11AX == 1)
	else {
		if ((prBssInfo->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
			|| prBssInfo->eBand == BAND_6G
#endif
			) &&
			RLM_NET_IS_11AX(prBssInfo) &&
			!HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW40_BW80_5G(
			prStaRec->ucHePhyCapInfo))
			prStaRec->ucVhtOpMode = 0;
	}
#endif
#endif
}
#endif /* CFG_SUPPORT_AAA */

/*----------------------------------------------------------------------------*/
/*!
 * \brief It is for both STA and AP modes
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmBssInitForAPandIbss(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo)
{
	ASSERT(prAdapter);
	ASSERT(prBssInfo);

#if CFG_ENABLE_WIFI_DIRECT
	if (prAdapter->fgIsP2PRegistered &&
	    prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
		rlmBssInitForAP(prAdapter, prBssInfo);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief It is for both STA and AP modes
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmBssAborted(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo)
{
	PFN_OPMODE_NOTIFY_DONE_FUNC pfnCallback;
	u_int8_t fgIsSuccess = FALSE;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	rlmBssReset(prAdapter, prBssInfo);

	prBssInfo->fg40mBwAllowed = FALSE;
	prBssInfo->fgAssoc40mBwAllowed = FALSE;

	/* <4> Tell OpMode change caller the change result fail
	 */
	pfnCallback = prBssInfo->pfOpChangeHandler;
	prBssInfo->pfOpChangeHandler = NULL;
	if (pfnCallback)
		pfnCallback(prAdapter, prBssInfo->ucBssIndex, fgIsSuccess);

	/* Assume FW state is updated by CMD_ID_SET_BSS_INFO, so
	 * the sync CMD is not needed here.
	 */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief All RLM timers will also be stopped.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static void rlmBssReset(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo)
{
	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	/* HT related parameters */
	prBssInfo->ucHtOpInfo1 = 0; /* RIFS disabled. 20MHz */
	prBssInfo->u2HtOpInfo2 = 0;
	prBssInfo->u2HtOpInfo3 = 0;

#if CFG_SUPPORT_802_11AC
	prBssInfo->ucVhtChannelWidth = 0;       /* VHT_OP_CHANNEL_WIDTH_80; */
	prBssInfo->ucVhtChannelFrequencyS1 = 0; /* 42; */
	prBssInfo->ucVhtChannelFrequencyS2 = 0;
	prBssInfo->u2VhtBasicMcsSet = 0; /* 0xFFFF; */
#endif

	prBssInfo->eBssSCO = 0;
	prBssInfo->fgErpProtectMode = 0;
	prBssInfo->eHtProtectMode = 0;
	prBssInfo->eGfOperationMode = 0;
	prBssInfo->eRifsOperationMode = 0;

	/* OBSS related parameters */
	prBssInfo->auc2G_20mReqChnlList[0] = 0;
	prBssInfo->auc2G_NonHtChnlList[0] = 0;
	prBssInfo->auc2G_PriChnlList[0] = 0;
	prBssInfo->auc2G_SecChnlList[0] = 0;
	prBssInfo->auc5G_20mReqChnlList[0] = 0;
	prBssInfo->auc5G_NonHtChnlList[0] = 0;
	prBssInfo->auc5G_PriChnlList[0] = 0;
	prBssInfo->auc5G_SecChnlList[0] = 0;

	/* All RLM timers will also be stopped */
	cnmTimerStopTimer(prAdapter, &prBssInfo->rObssScanTimer);
	prBssInfo->u2ObssScanInterval = 0;

	prBssInfo->fgObssErpProtectMode = 0;    /* GO only */
	prBssInfo->eObssHtProtectMode = 0;      /* GO only */
	prBssInfo->eObssGfOperationMode = 0;    /* GO only */
	prBssInfo->fgObssRifsOperationMode = 0; /* GO only */
	prBssInfo->fgObssActionForcedTo20M = 0; /* GO only */
	prBssInfo->fgObssBeaconForcedTo20M = 0; /* GO only */

	/* OP mode change control parameters */
	prBssInfo->fgIsOpChangeChannelWidth = FALSE;
	prBssInfo->fgIsOpChangeRxNss = FALSE;
	prBssInfo->fgIsOpChangeTxNss = FALSE;

	/* STBC MRC */
	rlmUpdateStbcSetting(prAdapter, prBssInfo->ucBssIndex, 0, FALSE);
	prBssInfo->eForceStbc = STBC_MRC_STATE_DISABLED;
	prBssInfo->eForceMrc = STBC_MRC_STATE_DISABLED;

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		/* Spatial Reuse params */
		prBssInfo->ucSRControl = 0;
		prBssInfo->ucNonSRGObssPdMaxOffset = 0;
		prBssInfo->ucSRGObssPdMinOffset = 0;
		prBssInfo->ucSRGObssPdMaxOffset = 0;
		prBssInfo->u8SRGBSSColorBitmap = 0;
		prBssInfo->u8SRGPartialBSSIDBitmap = 0;

		/* HE Operation */
		memset(prBssInfo->ucHeOpParams, 0, HE_OP_BYTE_NUM);
		/* set TXOP to 0x3FF (Spec. define default value) */
		prBssInfo->ucHeOpParams[0]
			|= HE_OP_PARAM0_TXOP_DUR_RTS_THRESHOLD_MASK;
		prBssInfo->ucHeOpParams[1]
			|= HE_OP_PARAM1_TXOP_DUR_RTS_THRESHOLD_MASK;
		prBssInfo->ucBssColorInfo = 0;
		prBssInfo->u2HeBasicMcsSet = 0;
#if (CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON == 1)
		prBssInfo->ucColorAnnouncement = FALSE;
		prBssInfo->ucColorSwitchCntdn = 0;
		prBssInfo->ucNewBssColorInfo = 0;
		prBssInfo->u64ExpectedTimestamp = 0;
#endif /* CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON */
	}
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	prBssInfo->fgIsHE6GPresent = FALSE;
	prBssInfo->fgIsCoHostedBssPresent = FALSE;
#endif

#if (CFG_SUPPORT_802_11BE == 1)
	EHT_RESET_OP(prBssInfo->ucEhtOpParams);
	/*TODO */
#endif

#if CFG_SUPPORT_DFS
	rlmResetCSAParams(prBssInfo, TRUE);
#endif
}

#if CFG_SUPPORT_TDLS
#if CFG_SUPPORT_802_11AC
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmFillVhtCapIEByAdapter(struct ADAPTER *prAdapter,
				  struct BSS_INFO *prBssInfo, uint8_t *pOutBuf)
{
	struct IE_VHT_CAP *prVhtCap;
	struct VHT_SUPPORTED_MCS_FIELD *prVhtSupportedMcsSet;
	uint8_t i;
	uint8_t ucMaxBw;
	uint8_t supportNss;

	if (!prAdapter) {
		DBGLOG(TDLS, ERROR, "prAdapter error!\n");
		return 0;
	}
	if (!prBssInfo) {
		DBGLOG(TDLS, ERROR, "prBssInfo error!\n");
		return 0;
	}

	prVhtCap = (struct IE_VHT_CAP *)pOutBuf;

	prVhtCap->ucId = ELEM_ID_VHT_CAP;
	prVhtCap->ucLength = sizeof(struct IE_VHT_CAP) - ELEM_HDR_LEN;
	prVhtCap->u4VhtCapInfo = VHT_CAP_INFO_DEFAULT_VAL;

	ucMaxBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);
#if CFG_SUPPORT_TDLS_ADJUST_BW
	if (TdlsNeedAdjustBw(prAdapter, prBssInfo->ucBssIndex)) {
		uint8_t ucNewMaxBw =
			rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo);

		DBGLOG(TDLS, INFO,
			"Adjust bw %d to %d\n", ucMaxBw, ucNewMaxBw);
	}
#endif
	supportNss = wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);

	prVhtCap->u4VhtCapInfo |= (prAdapter->rWifiVar.ucRxMaxMpduLen &
			VHT_CAP_INFO_MAX_MPDU_LEN_MASK);

	if (ucMaxBw == MAX_BW_160MHZ)
		prVhtCap->u4VhtCapInfo |=
			VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160;
	else if (ucMaxBw == MAX_BW_80_80_MHZ)
		prVhtCap->u4VhtCapInfo |=
			VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160_80P80;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxShortGI)) {
		if (ucMaxBw >= MAX_BW_80MHZ)
			prVhtCap->u4VhtCapInfo |= VHT_CAP_INFO_SHORT_GI_80;

		if (ucMaxBw >= MAX_BW_160MHZ)
			prVhtCap->u4VhtCapInfo |=
				VHT_CAP_INFO_SHORT_GI_160_80P80;
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxLdpc))
		prVhtCap->u4VhtCapInfo |= VHT_CAP_INFO_RX_LDPC;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxStbc)) {
		uint8_t tempRxStbcNss = prAdapter->rWifiVar.ucRxStbcNss;

		if (tempRxStbcNss > supportNss) {
			DBGLOG(TDLS, WARN,
				"Apply Nss:%d -> %d as RxStbcNss in VHT Cap",
				tempRxStbcNss, supportNss);
			tempRxStbcNss = supportNss;
		}
		prVhtCap->u4VhtCapInfo |=
			((tempRxStbcNss << VHT_CAP_INFO_RX_STBC_OFFSET) &
			 VHT_CAP_INFO_RX_STBC_MASK);
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucTxStbc) &&
			prBssInfo->ucOpTxNss >= 2)
		prVhtCap->u4VhtCapInfo |= VHT_CAP_INFO_TX_STBC;

	/*set MCS map */
	prVhtSupportedMcsSet = &prVhtCap->rVhtSupportedMcsSet;
	kalMemZero((void *)prVhtSupportedMcsSet,
		   sizeof(struct VHT_SUPPORTED_MCS_FIELD));

	for (i = 0; i < 8; i++) {
		uint8_t ucOffset = i * 2;
		uint8_t ucMcsMap;

		if (i < supportNss)
			ucMcsMap = VHT_CAP_INFO_MCS_MAP_MCS9;
		else
			ucMcsMap = VHT_CAP_INFO_MCS_NOT_SUPPORTED;

		prVhtSupportedMcsSet->u2RxMcsMap |= (ucMcsMap << ucOffset);
		prVhtSupportedMcsSet->u2TxMcsMap |= (ucMcsMap << ucOffset);
	}

	prVhtSupportedMcsSet->u2RxHighestSupportedDataRate =
		VHT_CAP_INFO_DEFAULT_HIGHEST_DATA_RATE;
	prVhtSupportedMcsSet->u2TxHighestSupportedDataRate =
		VHT_CAP_INFO_DEFAULT_HIGHEST_DATA_RATE;

	if (IE_SIZE(prVhtCap) > (ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP)) {
		DBGLOG(TDLS, ERROR, "prVhtCap size error!\n");
		return 0;
	}

	return IE_SIZE(prVhtCap);
}
#endif
#endif

#if CFG_SUPPORT_NAN
uint32_t rlmFillNANHTCapIE(struct ADAPTER *prAdapter,
		   struct BSS_INFO *prBssInfo, uint8_t *pOutBuf)
{
	struct IE_HT_CAP *prHtCap;
	struct SUP_MCS_SET_FIELD *prSupMcsSet;
	unsigned char fg40mAllowed = FALSE;
	uint8_t ucIdx;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return 0;
	}
	if (!prBssInfo) {
		DBGLOG(NAN, ERROR, "prBssInfo error!\n");
		return 0;
	}

	fg40mAllowed = prBssInfo->fgAssoc40mBwAllowed;
	prHtCap = (struct IE_HT_CAP *)pOutBuf;

	/* Add HT capabilities IE */
	prHtCap->ucId = ELEM_ID_HT_CAP;
	prHtCap->ucLength = sizeof(struct IE_HT_CAP) - ELEM_HDR_LEN;

	prHtCap->u2HtCapInfo = HT_CAP_INFO_DEFAULT_VAL;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxShortGI))
		prHtCap->u2HtCapInfo |=
			(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M);

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxLdpc))
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_LDPC_CAP;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucTxStbc))
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_TX_STBC;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxStbc)) {
		uint8_t tempRxStbcNss;

		tempRxStbcNss = prAdapter->rWifiVar.ucRxStbcNss;
		tempRxStbcNss =
			(tempRxStbcNss >
			 wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex))
				? wlanGetSupportNss(prAdapter,
						    prBssInfo->ucBssIndex)
				: (tempRxStbcNss);
		if (tempRxStbcNss != prAdapter->rWifiVar.ucRxStbcNss) {
			DBGLOG(RLM, WARN, "Apply Nss:%d as RxStbcNss in HT Cap",
			       wlanGetSupportNss(prAdapter,
						 prBssInfo->ucBssIndex));
			DBGLOG(RLM, WARN,
			       " due to set RxStbcNss more than Nss is not appropriate.\n");
		}
		if (tempRxStbcNss == 1)
			prHtCap->u2HtCapInfo |= HT_CAP_INFO_RX_STBC_1_SS;
		else if (tempRxStbcNss == 2)
			prHtCap->u2HtCapInfo |= HT_CAP_INFO_RX_STBC_2_SS;
		else if (tempRxStbcNss >= 3)
			prHtCap->u2HtCapInfo |= HT_CAP_INFO_RX_STBC_3_SS;
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxGf))
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_HT_GF;

	if (prAdapter->rWifiVar.ucRxMaxMpduLen > VHT_CAP_INFO_MAX_MPDU_LEN_3K)
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_MAX_AMSDU_LEN;

	if (!fg40mAllowed)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
					  HT_CAP_INFO_SHORT_GI_40M |
					  HT_CAP_INFO_DSSS_CCK_IN_40M);

	if (prBssInfo->ucOpRxNss <
	    wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex))
		prHtCap->u2HtCapInfo &=
			~HT_CAP_INFO_SM_POWER_SAVE; /*Set as static power save*/

	prHtCap->ucAmpduParam = AMPDU_PARAM_DEFAULT_VAL;

	prSupMcsSet = &prHtCap->rSupMcsSet;
	kalMemZero((void *)&prSupMcsSet->aucRxMcsBitmask[0],
		   SUP_MCS_RX_BITMASK_OCTET_NUM);

	for (ucIdx = 0;
	     ucIdx < wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);
	     ucIdx++)
		prSupMcsSet->aucRxMcsBitmask[ucIdx] = BITS(0, 7);

	/* prSupMcsSet->aucRxMcsBitmask[0] = BITS(0, 7); */

	if (fg40mAllowed)
	/*whsu:: && IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucMCS32)*/
		prSupMcsSet->aucRxMcsBitmask[32 / 8] = BIT(0); /* MCS32 */
	prSupMcsSet->u2RxHighestSupportedRate = SUP_MCS_RX_DEFAULT_HIGHEST_RATE;
	prSupMcsSet->u4TxRateInfo = SUP_MCS_TX_DEFAULT_VAL;

	prHtCap->u2HtExtendedCap = HT_EXT_CAP_DEFAULT_VAL;
	if (!fg40mAllowed ||
	    prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
		prHtCap->u2HtExtendedCap &=
			~(HT_EXT_CAP_PCO | HT_EXT_CAP_PCO_TRANS_TIME_NONE);

	prHtCap->u4TxBeamformingCap = TX_BEAMFORMING_CAP_DEFAULT_VAL;

	if (prBssInfo->eBand == BAND_5G) {
		/* HT BFee has IOT issue
		 * only support HT BFee when force mode for testing
		 */
		if (IS_FEATURE_FORCE_ENABLED(prAdapter->rWifiVar.ucStaHtBfee))
			prHtCap->u4TxBeamformingCap = TX_BEAMFORMING_CAP_BFEE;
		if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaHtBfer))
			prHtCap->u4TxBeamformingCap |= TX_BEAMFORMING_CAP_BFER;
	}

	prHtCap->ucAselCap = ASEL_CAP_DEFAULT_VAL;

	if (IE_SIZE(prHtCap) > (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP)) {
		DBGLOG(NAN, ERROR, "prHtCap size error!\n");
		return 0;
	}

	return IE_SIZE(prHtCap);
}

uint32_t rlmFillNANVHTCapIE(struct ADAPTER *prAdapter,
		   struct BSS_INFO *prBssInfo, uint8_t *pOutBuf)
{
	struct IE_VHT_CAP *prVhtCap;
	struct VHT_SUPPORTED_MCS_FIELD *prVhtSupportedMcsSet;
	uint8_t i;
	uint8_t ucMaxBw;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return 0;
	}
	if (!prBssInfo) {
		DBGLOG(NAN, ERROR, "prBssInfo error!\n");
		return 0;
	}

	prVhtCap = (struct IE_VHT_CAP *)pOutBuf;

	prVhtCap->ucId = ELEM_ID_VHT_CAP;
	prVhtCap->ucLength = sizeof(struct IE_VHT_CAP) - ELEM_HDR_LEN;

	prVhtCap->u4VhtCapInfo = VHT_CAP_INFO_DEFAULT_VAL;

	ucMaxBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);

	prVhtCap->u4VhtCapInfo |= (prAdapter->rWifiVar.ucRxMaxMpduLen &
				   VHT_CAP_INFO_MAX_MPDU_LEN_MASK);

	if (ucMaxBw == MAX_BW_160MHZ)
		prVhtCap->u4VhtCapInfo |=
			VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160;
	else if (ucMaxBw == MAX_BW_80_80_MHZ)
		prVhtCap->u4VhtCapInfo |=
			VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160_80P80;

	/* Fixme: to support BF for NAN */

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxShortGI)) {
		prVhtCap->u4VhtCapInfo |= VHT_CAP_INFO_SHORT_GI_80;

		if (ucMaxBw >= MAX_BW_160MHZ)
			prVhtCap->u4VhtCapInfo |=
				VHT_CAP_INFO_SHORT_GI_160_80P80;
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxLdpc))
		prVhtCap->u4VhtCapInfo |= VHT_CAP_INFO_RX_LDPC;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxStbc)) {
		uint8_t tempRxStbcNss;

		if (prAdapter->rWifiVar.u4SwTestMode ==
		    ENUM_SW_TEST_MODE_SIGMA_AC) {
			tempRxStbcNss = 1;
			DBGLOG(RLM, INFO,
			       "Set RxStbcNss to 1 for 11ac certification.\n");
		} else {
			tempRxStbcNss = prAdapter->rWifiVar.ucRxStbcNss;
			tempRxStbcNss =
				(tempRxStbcNss >
				 wlanGetSupportNss(prAdapter,
						   prBssInfo->ucBssIndex))
					? wlanGetSupportNss(
						  prAdapter,
						  prBssInfo->ucBssIndex)
					: (tempRxStbcNss);
			if (tempRxStbcNss != prAdapter->rWifiVar.ucRxStbcNss) {
				DBGLOG(RLM, WARN,
				       "Apply Nss:%d as RxStbcNss in VHT Cap",
				       wlanGetSupportNss(
					       prAdapter,
					       prBssInfo->ucBssIndex));
				DBGLOG(RLM, WARN,
				       "due to set RxStbcNss more than Nss is not appropriate.\n");
			}
		}
		prVhtCap->u4VhtCapInfo |=
			((tempRxStbcNss << VHT_CAP_INFO_RX_STBC_OFFSET) &
			 VHT_CAP_INFO_RX_STBC_MASK);
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucTxStbc))
		prVhtCap->u4VhtCapInfo |= VHT_CAP_INFO_TX_STBC;

	/* set MCS map */
	prVhtSupportedMcsSet = &prVhtCap->rVhtSupportedMcsSet;
	kalMemZero((void *)prVhtSupportedMcsSet,
		   sizeof(struct VHT_SUPPORTED_MCS_FIELD));

	for (i = 0; i < 8; i++) {
		uint8_t ucOffset = i * 2;
		uint8_t ucMcsMap;

		if (i < wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex))
			ucMcsMap = VHT_CAP_INFO_MCS_MAP_MCS9;
		else
			ucMcsMap = VHT_CAP_INFO_MCS_NOT_SUPPORTED;

		prVhtSupportedMcsSet->u2RxMcsMap |= (ucMcsMap << ucOffset);
		prVhtSupportedMcsSet->u2TxMcsMap |= (ucMcsMap << ucOffset);
	}

	prVhtSupportedMcsSet->u2RxHighestSupportedDataRate =
		VHT_CAP_INFO_DEFAULT_HIGHEST_DATA_RATE;
	prVhtSupportedMcsSet->u2TxHighestSupportedDataRate =
		VHT_CAP_INFO_DEFAULT_HIGHEST_DATA_RATE;

	if (IE_SIZE(prVhtCap) > (ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP)) {
		DBGLOG(NAN, ERROR, "prVhtCap size error!\n");
		return 0;
	}

	return IE_SIZE(prVhtCap);
}
#endif

#if CFG_SUPPORT_TDLS
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmFillHtCapIEByParams(u_int8_t fg40mAllowed,
				u_int8_t fgShortGIDisabled,
				uint8_t u8SupportRxSgi20,
				uint8_t u8SupportRxSgi40, uint8_t u8SupportRxGf,
				enum ENUM_OP_MODE eCurrentOPMode,
				uint8_t *pOutBuf)
{
	struct IE_HT_CAP *prHtCap;
	struct SUP_MCS_SET_FIELD *prSupMcsSet;

	ASSERT(pOutBuf);

	prHtCap = (struct IE_HT_CAP *)pOutBuf;

	/* Add HT capabilities IE */
	prHtCap->ucId = ELEM_ID_HT_CAP;
	prHtCap->ucLength = sizeof(struct IE_HT_CAP) - ELEM_HDR_LEN;

	prHtCap->u2HtCapInfo = HT_CAP_INFO_DEFAULT_VAL;
	if (!fg40mAllowed) {
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
					  HT_CAP_INFO_SHORT_GI_40M |
					  HT_CAP_INFO_DSSS_CCK_IN_40M);
	}
	if (fgShortGIDisabled)
		prHtCap->u2HtCapInfo &=
			~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M);

	if (u8SupportRxSgi20 == 2)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SHORT_GI_20M);
	if (u8SupportRxSgi40 == 2)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SHORT_GI_40M);
	if (u8SupportRxGf == 2)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_HT_GF);

	prHtCap->ucAmpduParam = AMPDU_PARAM_DEFAULT_VAL;

	prSupMcsSet = &prHtCap->rSupMcsSet;
	kalMemZero((void *)&prSupMcsSet->aucRxMcsBitmask[0],
		   SUP_MCS_RX_BITMASK_OCTET_NUM);

	prSupMcsSet->aucRxMcsBitmask[0] = BITS(0, 7);

	if (fg40mAllowed)
		prSupMcsSet->aucRxMcsBitmask[32 / 8] = BIT(0); /* MCS32 */
	prSupMcsSet->u2RxHighestSupportedRate = SUP_MCS_RX_DEFAULT_HIGHEST_RATE;
	prSupMcsSet->u4TxRateInfo = SUP_MCS_TX_DEFAULT_VAL;

	prHtCap->u2HtExtendedCap = HT_EXT_CAP_DEFAULT_VAL;
	if (!fg40mAllowed || eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
		prHtCap->u2HtExtendedCap &=
			~(HT_EXT_CAP_PCO | HT_EXT_CAP_PCO_TRANS_TIME_NONE);

	prHtCap->u4TxBeamformingCap = TX_BEAMFORMING_CAP_DEFAULT_VAL;

	prHtCap->ucAselCap = ASEL_CAP_DEFAULT_VAL;

	ASSERT(IE_SIZE(prHtCap) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP));

	return IE_SIZE(prHtCap);
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
uint32_t rlmFillHtCapIEByAdapter(struct ADAPTER *prAdapter,
				 struct BSS_INFO *prBssInfo, uint8_t *pOutBuf)
{
	struct IE_HT_CAP *prHtCap;
	struct SUP_MCS_SET_FIELD *prSupMcsSet;
	uint8_t fg40mAllowed;
	uint8_t ucIdx;
	uint8_t supportNss = wlanGetSupportNss(
				prAdapter, prBssInfo->ucBssIndex);

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(pOutBuf);

	fg40mAllowed = prBssInfo->fgAssoc40mBwAllowed;

	prHtCap = (struct IE_HT_CAP *)pOutBuf;

	/* Add HT capabilities IE */
	prHtCap->ucId = ELEM_ID_HT_CAP;
	prHtCap->ucLength = sizeof(struct IE_HT_CAP) - ELEM_HDR_LEN;

	prHtCap->u2HtCapInfo = HT_CAP_INFO_DEFAULT_VAL;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxShortGI))
		prHtCap->u2HtCapInfo |=
			(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M);

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxLdpc))
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_LDPC_CAP;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucTxStbc) &&
			prBssInfo->ucOpTxNss >= 2)
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_TX_STBC;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxStbc)) {
		uint8_t tempRxStbcNss = prAdapter->rWifiVar.ucRxStbcNss;

		if (tempRxStbcNss > supportNss) {
			DBGLOG(TDLS, WARN,
				"Apply Nss:%d -> %d as RxStbcNss in HT Cap",
				tempRxStbcNss, supportNss);
			tempRxStbcNss = supportNss;
		}

		if (tempRxStbcNss == 1)
			prHtCap->u2HtCapInfo |= HT_CAP_INFO_RX_STBC_1_SS;
		else if (tempRxStbcNss == 2)
			prHtCap->u2HtCapInfo |= HT_CAP_INFO_RX_STBC_2_SS;
		else if (tempRxStbcNss >= 3)
			prHtCap->u2HtCapInfo |= HT_CAP_INFO_RX_STBC_3_SS;
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxGf))
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_HT_GF;

	if (prAdapter->rWifiVar.ucRxMaxMpduLen > VHT_CAP_INFO_MAX_MPDU_LEN_3K)
		prHtCap->u2HtCapInfo |= HT_CAP_INFO_MAX_AMSDU_LEN;

	if (!fg40mAllowed)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
				HT_CAP_INFO_SHORT_GI_40M |
				HT_CAP_INFO_DSSS_CCK_IN_40M);

	/* SM power saving (IEEE 802.11, 2016, 10.2.4)
	 * A non-AP HT STA may also use SM Power Save bits in the HT
	 * Capabilities element of its Association Request to achieve
	 * the same purpose. The latter allows the STA to use only a
	 * single receive chain immediately after association.
	 */
	/* Set as non static power save for Tx 2Nss*/
	prHtCap->u2HtCapInfo |= HT_CAP_INFO_SM_POWER_SAVE;

	prHtCap->ucAmpduParam = AMPDU_PARAM_DEFAULT_VAL;

	prSupMcsSet = &prHtCap->rSupMcsSet;
	kalMemZero((void *)&prSupMcsSet->aucRxMcsBitmask[0],
		   SUP_MCS_RX_BITMASK_OCTET_NUM);

	for (ucIdx = 0; ucIdx < supportNss; ucIdx++)
		prSupMcsSet->aucRxMcsBitmask[ucIdx] = BITS(0, 7);

	if (fg40mAllowed && IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucMCS32))
		prSupMcsSet->aucRxMcsBitmask[32 / 8] = BIT(0); /* MCS32 */
	prSupMcsSet->u2RxHighestSupportedRate = SUP_MCS_RX_DEFAULT_HIGHEST_RATE;
	prSupMcsSet->u4TxRateInfo = SUP_MCS_TX_DEFAULT_VAL;

	prHtCap->u2HtExtendedCap = HT_EXT_CAP_DEFAULT_VAL;
	if (!fg40mAllowed ||
	    prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
		prHtCap->u2HtExtendedCap &=
			~(HT_EXT_CAP_PCO | HT_EXT_CAP_PCO_TRANS_TIME_NONE);

	prHtCap->u4TxBeamformingCap = TX_BEAMFORMING_CAP_DEFAULT_VAL;

	prHtCap->ucAselCap = ASEL_CAP_DEFAULT_VAL;

	ASSERT(IE_SIZE(prHtCap) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP));

	return IE_SIZE(prHtCap);
}

#endif
static uint32_t tpcGetCurrentTxPwr(struct ADAPTER *prAdapter,
				uint8_t ucBssIdx,
				int8_t *picTxPwr)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct BSS_DESC *prBssDesc = NULL;
	int8_t icPwrLmt = RLM_INVALID_POWER_LIMIT;

	if (!prAdapter || (ucBssIdx >= MAX_BSSID_NUM))
		return WLAN_STATUS_INVALID_DATA;

	*picTxPwr = prAdapter->u4GetTxPower;

#if CFG_SUPPORT_802_11K
	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIdx);
	/* only consider regulatory power limit when connected */
	if (prBssDesc != NULL && prBssDesc->fgIsConnected) {
		u4Status = rlmRegTxPwrLimitGet(prAdapter, ucBssIdx, &icPwrLmt);
		if ((u4Status == WLAN_STATUS_SUCCESS) &&
			(icPwrLmt != RLM_INVALID_POWER_LIMIT) &&
			(*picTxPwr > icPwrLmt)) {
			/* Choose min value between current target power
			 * and regulatory power limit
			 */
			*picTxPwr = icPwrLmt;
		}
	}
#endif

	DBGLOG(RLM, INFO, "Get txpower curr[%d]Lmt[%d]Final[%d]\n",
	       prAdapter->u4GetTxPower,
	       icPwrLmt,
	       *picTxPwr);

	return WLAN_STATUS_SUCCESS;
}
#if CFG_SUPPORT_DFS
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the TPC Report frame.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
static void tpcComposeReportFrame(struct ADAPTER *prAdapter,
				  struct STA_RECORD *prStaRec,
				  PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct BSS_INFO *prBssInfo;
	struct ACTION_TPC_REPORT_FRAME *prTxFrame;
	uint16_t u2PayloadLen;
	int8_t icTxPwr = 0;

	ASSERT(prAdapter);
	ASSERT(prStaRec);

	if (prStaRec->ucBssIndex >= MAX_BSSID_NUM)
		return;
	prBssInfo = &prAdapter->rWifiVar.arBssInfoPool[prStaRec->ucBssIndex];
	ASSERT(prBssInfo);

	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(
		prAdapter, MAC_TX_RESERVED_FIELD + PUBLIC_ACTION_MAX_LEN);

	if (!prMsduInfo)
		return;

	prTxFrame = (struct ACTION_TPC_REPORT_FRAME
			     *)((uintptr_t)(prMsduInfo->prPacket) +
				MAC_TX_RESERVED_FIELD);

	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	prTxFrame->ucCategory = CATEGORY_SPEC_MGT;
	prTxFrame->ucAction = ACTION_TPC_REPORT;

	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = prStaRec->ucSmDialogToken;
	prTxFrame->ucElemId = ELEM_ID_TPC_REPORT;
	prTxFrame->ucLength =
		sizeof(prTxFrame->ucLinkMargin) + sizeof(prTxFrame->ucTransPwr);
	tpcGetCurrentTxPwr(prAdapter, prStaRec->ucBssIndex, &icTxPwr);
	prTxFrame->ucTransPwr = icTxPwr;
	prTxFrame->ucLinkMargin =
		prAdapter->rLinkQuality.rLq[prStaRec->ucBssIndex].
		cRssi - (0 - MIN_RCV_PWR);

	u2PayloadLen = ACTION_SM_TPC_REPORT_LEN;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     WLAN_MAC_MGMT_HEADER_LEN + u2PayloadLen, pfTxDoneHandler,
		     MSDU_RATE_MODE_AUTO);

	DBGLOG(RLM, INFO, "ucDialogToken %d ucTransPwr %d ucLinkMargin %d\n",
	       prTxFrame->ucDialogToken, prTxFrame->ucTransPwr,
	       prTxFrame->ucLinkMargin);

	/* 4 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return;

} /* end of tpcComposeReportFrame() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the Measurement Report frame.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
static void msmtComposeReportFrame(struct ADAPTER *prAdapter,
				   struct STA_RECORD *prStaRec,
				   PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct BSS_INFO *prBssInfo;
	struct ACTION_SM_REQ_FRAME *prTxFrame;
	struct IE_MEASUREMENT_REPORT *prMeasurementRepIE;
	uint8_t *pucIE;
	uint16_t u2PayloadLen;

	ASSERT(prAdapter);
	ASSERT(prStaRec);

	if (prStaRec->ucBssIndex >= MAX_BSSID_NUM)
		return;

	prBssInfo = &prAdapter->rWifiVar.arBssInfoPool[prStaRec->ucBssIndex];
	ASSERT(prBssInfo);

	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(
		prAdapter, MAC_TX_RESERVED_FIELD + PUBLIC_ACTION_MAX_LEN);

	if (!prMsduInfo)
		return;

	prTxFrame = (struct ACTION_SM_REQ_FRAME
			     *)((uintptr_t)(prMsduInfo->prPacket) +
				MAC_TX_RESERVED_FIELD);
	pucIE = prTxFrame->aucInfoElem;
	prMeasurementRepIE = SM_MEASUREMENT_REP_IE(pucIE);

	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	prTxFrame->ucCategory = CATEGORY_SPEC_MGT;
	prTxFrame->ucAction = ACTION_MEASUREMENT_REPORT;

	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = prStaRec->ucSmDialogToken;
	prMeasurementRepIE->ucId = ELEM_ID_MEASUREMENT_REPORT;

#if 0
	if (prStaRec->ucSmMsmtRequestMode == ELEM_RM_TYPE_BASIC_REQ) {
		prMeasurementRepIE->ucLength =
		     sizeof(struct SM_BASIC_REPORT) + 3;
		u2PayloadLen = ACTION_SM_MEASURE_REPORT_LEN+
		     ACTION_SM_BASIC_REPORT_LEN;
	} else if (prStaRec->ucSmMsmtRequestMode == ELEM_RM_TYPE_CCA_REQ) {
		prMeasurementRepIE->ucLength = sizeof(struct SM_CCA_REPORT) + 3;
		u2PayloadLen = ACTION_SM_MEASURE_REPORT_LEN+
		     ACTION_SM_CCA_REPORT_LEN;
	} else if (prStaRec->ucSmMsmtRequestMode ==
		     ELEM_RM_TYPE_RPI_HISTOGRAM_REQ) {
		prMeasurementRepIE->ucLength = sizeof(struct SM_RPI_REPORT) + 3;
		u2PayloadLen = ACTION_SM_MEASURE_REPORT_LEN+
		     ACTION_SM_PRI_REPORT_LEN;
	} else {
		prMeasurementRepIE->ucLength = 3;
		u2PayloadLen = ACTION_SM_MEASURE_REPORT_LEN;
	}
#else
	prMeasurementRepIE->ucLength = 3;
	u2PayloadLen = ACTION_SM_MEASURE_REPORT_LEN;
	prMeasurementRepIE->ucToken = prStaRec->ucSmMsmtToken;
	prMeasurementRepIE->ucReportMode = BIT(1);
	prMeasurementRepIE->ucMeasurementType = prStaRec->ucSmMsmtRequestMode;
#endif

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     WLAN_MAC_MGMT_HEADER_LEN + u2PayloadLen, pfTxDoneHandler,
		     MSDU_RATE_MODE_AUTO);

	DBGLOG(RLM, TRACE,
	       "ucDialogToken %d ucToken %d ucReportMode %d ucMeasurementType %d\n",
	       prTxFrame->ucDialogToken, prMeasurementRepIE->ucToken,
	       prMeasurementRepIE->ucReportMode,
	       prMeasurementRepIE->ucMeasurementType);

	/* 4 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return;

} /* end of msmtComposeReportFrame() */

void rlmProcessExCsaIE(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams,
	uint8_t ucChannelSwitchMode, uint8_t ucNewOperatingClass,
	uint8_t ucNewChannelNum, uint8_t ucChannelSwitchCount)
{
	DBGLOG(RLM, INFO,
		"[ECSA] bss[%d] mode[%d], op_class[%d], channel[%d], count[%d]\n",
		prStaRec->ucBssIndex,
		ucChannelSwitchMode,
		ucNewOperatingClass,
		ucNewChannelNum,
		ucChannelSwitchCount);

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (IS_6G_OP_CLASS(ucNewOperatingClass))
		prCSAParams->eCsaBand = BAND_6G;
	else
#endif
	if (ucNewChannelNum > 14)
		prCSAParams->eCsaBand = BAND_5G;
	else
		prCSAParams->eCsaBand = BAND_2G4;

	prCSAParams->ucCsaNewCh = ucNewChannelNum;
	switch (rlmOpClassToBandwidth(ucNewOperatingClass)) {
	case BW_20:
	case BW_40:
		prCSAParams->ucVhtBw = VHT_OP_CHANNEL_WIDTH_20_40;
		break;
	case BW_80:
		prCSAParams->ucVhtBw = VHT_OP_CHANNEL_WIDTH_80;
		break;
	case BW_160:
		prCSAParams->ucVhtBw = VHT_OP_CHANNEL_WIDTH_160;
		break;
	case BW_8080:
		prCSAParams->ucVhtBw = VHT_OP_CHANNEL_WIDTH_80P80;
		break;
	case BW_320:
		prCSAParams->ucVhtBw = VHT_OP_CHANNEL_WIDTH_320_1;
		break;
	default:
		break;
	}
	prCSAParams->ucVhtS1 = nicGetS1(prAdapter, prCSAParams->eCsaBand,
					ucNewChannelNum,
					prCSAParams->ucVhtBw);

	if (ucChannelSwitchMode == 1) {
		/* Need to stop data transmission immediately */
		if (!prCSAParams->fgHasStopTx) {
			prCSAParams->fgHasStopTx = TRUE;
			kalIndicateAllQueueTxAllowed(
				prAdapter->prGlueInfo,
				prStaRec->ucBssIndex,
				FALSE);
			/* AP */
			qmSetStaRecTxAllowed(prAdapter,
				   prStaRec,
				   FALSE);
			DBGLOG(RLM, EVENT,
				"[ECSA] TxAllowed = FALSE\n");
		}
	}

	prCSAParams->ucCsaMode = ucChannelSwitchMode;
	if (prCSAParams->ucCsaMode > MODE_NUM) {
		DBGLOG(RLM, WARN,
			"[ECSA] invalid ChannelSwitchMode = %d, follow mode 0\n",
			prCSAParams->ucCsaMode);
		prCSAParams->ucCsaMode = MODE_ALLOW_TX;
	}

	DBGLOG(RLM, INFO,
		"[ECSA] bw[%d], channel[%d], s1[%d]\n",
		prCSAParams->ucVhtBw,
		prCSAParams->ucCsaNewCh,
		prCSAParams->ucVhtS1);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function handle spectrum management action frame
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmProcessSpecMgtAction(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	uint8_t *pucIE;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint16_t u2IELength;
#if CFG_SUPPORT_DFS
	uint16_t u2Offset = 0;
	struct IE_CHANNEL_SWITCH *prChannelSwitchAnnounceIE;
	struct IE_SECONDARY_OFFSET *prSecondaryOffsetIE;
	struct IE_WIDE_BAND_CHANNEL *prWideBandChannelIE;
	struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams;
	uint8_t ucCurrentCsaCount;
#endif
	struct IE_TPC_REQ *prTpcReqIE;
	struct IE_TPC_REPORT *prTpcRepIE;
	struct IE_MEASUREMENT_REQ *prMeasurementReqIE;
	struct IE_MEASUREMENT_REPORT *prMeasurementRepIE;
	struct WLAN_ACTION_FRAME *prActFrame;
	u_int8_t ucAction;

	DBGLOG(RLM, INFO, "[Mgt Action]rlmProcessSpecMgtAction\n");
	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return;

	if (prStaRec->ucBssIndex > prAdapter->ucSwBssIdNum)
		return;

	prActFrame = (struct WLAN_ACTION_FRAME *) prSwRfb->pvHeader;
	if (prActFrame->ucAction == ACTION_CHNL_SWITCH) {
		struct ACTION_CHANNEL_SWITCH_FRAME *prRxFrame;

		u2IELength = prSwRfb->u2PacketLen -
			(uint16_t)OFFSET_OF(struct ACTION_CHANNEL_SWITCH_FRAME,
					aucInfoElem[0]);
		prRxFrame =
		    (struct ACTION_CHANNEL_SWITCH_FRAME *)prSwRfb->pvHeader;
		pucIE = prRxFrame->aucInfoElem;
		ucAction = prRxFrame->ucAction;
	} else {
		struct ACTION_SM_REQ_FRAME *prRxFrame;

		u2IELength = prSwRfb->u2PacketLen -
			(uint16_t)OFFSET_OF(struct ACTION_SM_REQ_FRAME,
					aucInfoElem[0]);
		prRxFrame =
		    (struct ACTION_SM_REQ_FRAME *)prSwRfb->pvHeader;
		pucIE = prRxFrame->aucInfoElem;
		ucAction = prRxFrame->ucAction;
		prStaRec->ucSmDialogToken = prRxFrame->ucDialogToken;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "prBssInfo is null\n");
		return;
	}

	DBGLOG_MEM8(RLM, INFO, pucIE, u2IELength);
	switch (ucAction) {
	case ACTION_MEASUREMENT_REQ:
		DBGLOG(RLM, INFO, "[Mgt Action] Measure Request\n");
		prMeasurementReqIE = SM_MEASUREMENT_REQ_IE(pucIE);
		if (prMeasurementReqIE->ucId == ELEM_ID_MEASUREMENT_REQ) {
			/* Check IE length is valid */
			if (prMeasurementReqIE->ucLength != 0 &&
				(prMeasurementReqIE->ucLength >=
				sizeof(struct IE_MEASUREMENT_REQ) - 2)) {
				prStaRec->ucSmMsmtRequestMode =
					prMeasurementReqIE->ucRequestMode;
				prStaRec->ucSmMsmtToken =
					prMeasurementReqIE->ucToken;
				msmtComposeReportFrame(prAdapter, prStaRec,
							NULL);
			}
		}

		break;
	case ACTION_MEASUREMENT_REPORT:
		DBGLOG(RLM, INFO, "[Mgt Action] Measure Report\n");
		prMeasurementRepIE = SM_MEASUREMENT_REP_IE(pucIE);
		if (prMeasurementRepIE->ucId == ELEM_ID_MEASUREMENT_REPORT)
			DBGLOG(RLM, TRACE,
			       "[Mgt Action] Correct Measurement report IE !!\n");
		break;
	case ACTION_TPC_REQ:
		DBGLOG(RLM, INFO, "[Mgt Action] TPC Request\n");
		prTpcReqIE = SM_TPC_REQ_IE(pucIE);

		if (prTpcReqIE->ucId == ELEM_ID_TPC_REQ)
			tpcComposeReportFrame(prAdapter, prStaRec, NULL);

		break;
	case ACTION_TPC_REPORT:
		DBGLOG(RLM, INFO, "[Mgt Action] TPC Report\n");
		prTpcRepIE = SM_TPC_REP_IE(pucIE);

		if (prTpcRepIE->ucId == ELEM_ID_TPC_REPORT)
			DBGLOG(RLM, TRACE,
			       "[Mgt Action] Correct TPC report IE !!\n");

		break;
#if CFG_SUPPORT_DFS
	case ACTION_CHNL_SWITCH:
		prCSAParams = &prBssInfo->CSAParams;
		ucCurrentCsaCount = MAX_CSA_COUNT;

		if (prBssInfo->fgIsSwitchingChnl) {
			DBGLOG(RLM, WARN,
			       "[CSA Mgt] Still waiting for CSA to be done, drop it");
			break;
		}

		if (IS_BSS_AIS(prBssInfo)) {
			if (aisFsmIsInProcessPostpone(prAdapter,
				prBssInfo->ucBssIndex))
				break;
#if CFG_SUPPORT_ROAMING
			if (roamingFsmCheckIfRoaming(prAdapter,
				prBssInfo->ucBssIndex)) {
				DBGLOG(RLM, INFO,
					"Ignore csa action frame when roaming\n");
				break;
			}
#endif
		}

		IE_FOR_EACH(pucIE, u2IELength, u2Offset)
		{
			switch (IE_ID(pucIE)) {

			case ELEM_ID_WIDE_BAND_CHANNEL_SWITCH:
				if (IE_LEN(pucIE) !=
					(sizeof(struct IE_WIDE_BAND_CHANNEL) -
						2)) {
					DBGLOG(RLM, INFO,
					       "[CSA Mgt] ELEM_ID_WIDE_BAND_CHANNEL_SWITCH, Length\n");
					break;
				}
				DBGLOG(RLM, INFO,
				       "[CSA Mgt] ELEM_ID_WIDE_BAND_CHANNEL_SWITCH, 11AC\n");

				prWideBandChannelIE =
					(struct IE_WIDE_BAND_CHANNEL *)pucIE;
				prCSAParams->ucVhtBw =
					prWideBandChannelIE->ucNewChannelWidth;
				prCSAParams->ucVhtS1 =
					prWideBandChannelIE->ucChannelS1;
				prCSAParams->ucVhtS2 =
					prWideBandChannelIE->ucChannelS2;
				break;

			case ELEM_ID_CH_SW_ANNOUNCEMENT:
				if (IE_LEN(pucIE) !=
				    (sizeof(struct IE_CHANNEL_SWITCH) - 2)) {
					DBGLOG(RLM, INFO,
					       "[CSA Mgt] ELEM_ID_CH_SW_ANNOUNCEMENT, Length\n");
					break;
				}

				prChannelSwitchAnnounceIE =
					(struct IE_CHANNEL_SWITCH *)pucIE;

				if (prBssInfo->ucPrimaryChannel ==
						prChannelSwitchAnnounceIE->
						ucNewChannelNum) {
					DBGLOG(RLM, WARN,
						"[CSA Mgt] BSS: " MACSTR
						" already at channel %u\n",
						MAC2STR(prBssInfo->aucBSSID),
						prChannelSwitchAnnounceIE->
							ucNewChannelNum);
					break;
				}

				DBGLOG(RLM, INFO,
					"[CSA Mgt] Count = %d Mode = %d\n",
				prChannelSwitchAnnounceIE->ucChannelSwitchCount,
				prChannelSwitchAnnounceIE->ucChannelSwitchMode);

				if (prChannelSwitchAnnounceIE
					    ->ucChannelSwitchMode == 1) {

					/* Need to stop data
					 * transmission immediately
					 */
					if (!prCSAParams->fgHasStopTx) {
						prCSAParams->fgHasStopTx = TRUE;
						kalIndicateAllQueueTxAllowed(
							prAdapter->prGlueInfo,
							prStaRec->ucBssIndex,
							FALSE);
						/* AP */
						qmSetStaRecTxAllowed(prAdapter,
							   prStaRec,
							   FALSE);
						DBGLOG(RLM, EVENT,
							"[CSA Mgt] TxAllowed = FALSE\n");
					}
				}

				prCSAParams->ucCsaNewCh =
				    prChannelSwitchAnnounceIE->ucNewChannelNum;
				prCSAParams->ucCsaMode =
				 prChannelSwitchAnnounceIE->ucChannelSwitchMode;

				if (prCSAParams->ucCsaMode > MODE_NUM) {
					DBGLOG(RLM, WARN,
						"[CSA] invalid ChannelSwitchMode = %d, follow mode 0\n",
						prCSAParams->ucCsaMode);
					prCSAParams->ucCsaMode = MODE_ALLOW_TX;
				}

				DBGLOG(RLM, INFO,
					"[CSA Mgt] switch channel [%d]->[%d]\n",
					prBssInfo->ucPrimaryChannel,
					prChannelSwitchAnnounceIE
						->ucNewChannelNum);

				ucCurrentCsaCount =
					prChannelSwitchAnnounceIE->
						ucChannelSwitchCount;

				break;

			case ELEM_ID_SCO:
				if (IE_LEN(pucIE) !=
				    (sizeof(struct IE_SECONDARY_OFFSET) - 2)) {
					DBGLOG(RLM, INFO,
					       "[CSA Mgt] ELEM_ID_SCO, Length\n");
					break;
				}
				prSecondaryOffsetIE =
					(struct IE_SECONDARY_OFFSET *)pucIE;

				DBGLOG(RLM, INFO,
				       "[CSA Mgt] SCO [%d]->[%d]\n",
				       prBssInfo->eBssSCO,
				       prSecondaryOffsetIE->ucSecondaryOffset);

				prCSAParams->eSco = (enum ENUM_CHNL_EXT)
					prSecondaryOffsetIE->ucSecondaryOffset;
				break;

			default:
				break;
			} /*end of switch IE_ID */
		}	 /*end of IE_FOR_EACH */

		if (SHOULD_CH_SWITCH(ucCurrentCsaCount, prCSAParams)) {
			cnmTimerStopTimer(prAdapter, &prBssInfo->rCsaTimer);
			cnmTimerStartTimer(prAdapter, &prBssInfo->rCsaTimer,
				prBssInfo->u2BeaconInterval *
					ucCurrentCsaCount);
			prCSAParams->ucCsaCount = ucCurrentCsaCount;
			DBGLOG(RLM, INFO,
				"[CSA Mgt] Channel switch Countdown: %d msecs, Mode: %d\n",
				prBssInfo->u2BeaconInterval *
					prCSAParams->ucCsaCount,
				prCSAParams->ucCsaMode);
		}

		break;
#endif
	default:
		break;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function handle public action frame
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmProcessPublicActionExCsa(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb)
{
	struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams;
	struct ACTION_EX_CHANNEL_SWITCH_FRAME *prEcsaActionFrame;
	struct IE_WIDE_BAND_CHANNEL *prWideBandIe;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucIE;
	uint16_t u2IELength, u2Offset;
	uint8_t ucCurrentCsaCount;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return;

	if (prStaRec->ucBssIndex > prAdapter->ucSwBssIdNum)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return;

	if (IS_BSS_AIS(prBssInfo)) {
		if (aisFsmIsInProcessPostpone(prAdapter,
			prBssInfo->ucBssIndex))
			return;
#if CFG_SUPPORT_ROAMING
		if (roamingFsmCheckIfRoaming(prAdapter,
			prBssInfo->ucBssIndex)) {
			DBGLOG(RLM, INFO,
				"Ignore csa action frame when roaming\n");
			return;
		}
#endif
	}

	prCSAParams = &prBssInfo->CSAParams;
	if (prBssInfo->fgIsSwitchingChnl) {
		DBGLOG(RLM, WARN,
		       "[CSA Mgt] Still waiting for CSA to be done, drop it");
		return;
	}

	u2IELength = prSwRfb->u2PacketLen -
		(uint16_t)OFFSET_OF(struct ACTION_EX_CHANNEL_SWITCH_FRAME,
				aucInfoElem[0]);
	prEcsaActionFrame =
		(struct ACTION_EX_CHANNEL_SWITCH_FRAME *)prSwRfb->pvHeader;
	pucIE = prEcsaActionFrame->aucInfoElem;

	if (prBssInfo->ucPrimaryChannel == prEcsaActionFrame->ucNewChannelNum) {
		DBGLOG(RLM, WARN,
			"[ECSA Public] BSS: " MACSTR " already at channel %u\n",
			MAC2STR(prBssInfo->aucBSSID),
			prEcsaActionFrame->ucNewChannelNum);
		return;
	}
	rlmProcessExCsaIE(prAdapter, prStaRec,
		prCSAParams,
		prEcsaActionFrame->ucChannelSwitchMode,
		prEcsaActionFrame->ucNewOperatingClass,
		prEcsaActionFrame->ucNewChannelNum,
		prEcsaActionFrame->ucChannelSwitchCount);

	prCSAParams->ucCsaNewCh = prEcsaActionFrame->ucNewChannelNum;
	ucCurrentCsaCount = prEcsaActionFrame->ucChannelSwitchCount;

	IE_FOR_EACH(pucIE, u2IELength, u2Offset)
	{
		switch (IE_ID(pucIE)) {
		case ELEM_ID_WIDE_BAND_CHANNEL_SWITCH:
			if (IE_LEN(pucIE) !=
				(sizeof(struct IE_WIDE_BAND_CHANNEL) - 2)) {
				break;
			}

			prWideBandIe = (struct IE_WIDE_BAND_CHANNEL *)pucIE;

			DBGLOG(RLM, INFO,
				"[Wide BW] bw[%d], s1[%d], s2[%d]\n",
				prWideBandIe->ucNewChannelWidth,
				prWideBandIe->ucChannelS1,
				prWideBandIe->ucChannelS2);
			break;

		default:
			break;
		}
	}

	if (SHOULD_CH_SWITCH(ucCurrentCsaCount, prCSAParams)) {
		cnmTimerStopTimer(prAdapter, &prBssInfo->rCsaTimer);
		cnmTimerStartTimer(prAdapter, &prBssInfo->rCsaTimer,
			prBssInfo->u2BeaconInterval * ucCurrentCsaCount);
		prCSAParams->ucCsaCount = ucCurrentCsaCount;
		DBGLOG(RLM, INFO,
			"[ECSA Public] Channel switch Countdown: %d msecs\n",
			prBssInfo->u2BeaconInterval * prCSAParams->ucCsaCount);
	}
}

void rlmResetCSAParams(struct BSS_INFO *prBssInfo, uint8_t fgClearAll)
{
	struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams;
	uint8_t fgHasStopTx;
	uint8_t ucCsaMode;
	uint8_t fgIsCrossBand;

	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "Reset CSA params failed, Bssinfo null!");
		return;
	}

	prCSAParams = &(prBssInfo->CSAParams);
	fgHasStopTx = prCSAParams->fgHasStopTx;
	ucCsaMode = prCSAParams->ucCsaMode;
	fgIsCrossBand = prCSAParams->fgIsCrossBand;
	kalMemZero(prCSAParams, sizeof(struct SWITCH_CH_AND_BAND_PARAMS));
	prCSAParams->ucCsaCount = MAX_CSA_COUNT;
	prCSAParams->ucCsaMode = MODE_NUM;
	if (!fgClearAll) {
		prCSAParams->fgHasStopTx = fgHasStopTx;
		prCSAParams->ucCsaMode = ucCsaMode;
		prCSAParams->fgIsCrossBand = fgIsCrossBand;
	}
	DBGLOG(RLM, TRACE, "Reset CSA count to %u for BSS%d fgHasStopTx=%d",
		prCSAParams->ucCsaCount, prBssInfo->ucBssIndex,
		prCSAParams->fgHasStopTx);
}

void rlmCsaTimeout(struct ADAPTER *prAdapter,
				   uintptr_t ulParamPtr)
{
	uint8_t ucBssIndex = (uint8_t) ulParamPtr;
	struct BSS_INFO *prBssInfo;
	struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams;
	struct PARAM_SSID rSsid;
	struct BSS_DESC *prBssDesc;
	struct STA_RECORD *prStaRec;
	enum ENUM_BAND eNewBand;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(AIS, INFO, "No prBssInfo\n");
		return;
	}

	prStaRec = prBssInfo->prStaRecOfAP;
	if (!prStaRec) {
		DBGLOG(AIS, INFO, "No prStaRec\n");
		return;
	}

	kalMemZero(&rSsid, sizeof(rSsid));
	prCSAParams = &prBssInfo->CSAParams;
	if (prBssInfo->ucPrimaryChannel == prCSAParams->ucCsaNewCh) {
		DBGLOG(RLM, WARN,
			"BSS: " MACSTR " already at channel %u\n",
			MAC2STR(prBssInfo->aucBSSID), prCSAParams->ucCsaNewCh);
		if (prCSAParams->fgHasStopTx) {
			kalIndicateAllQueueTxAllowed(
				prAdapter->prGlueInfo,
				prStaRec->ucBssIndex,
				TRUE);
			qmSetStaRecTxAllowed(prAdapter, prStaRec, TRUE);
			DBGLOG(RLM, EVENT, "[CSA] TxAllowed = TRUE\n");
		}
		rlmResetCSAParams(prBssInfo, TRUE);
		return;
	}
	prBssInfo->ucPrimaryChannel = prCSAParams->ucCsaNewCh;
	if (prCSAParams->eCsaBand != BAND_NULL)
		eNewBand = prCSAParams->eCsaBand;
	else
		eNewBand = (prCSAParams->ucCsaNewCh <= 14)
			? BAND_2G4 : BAND_5G;
	if (cnmGet80211Band(prBssInfo->eBand) != cnmGet80211Band(eNewBand))
		prCSAParams->fgIsCrossBand = TRUE;
	else
		prCSAParams->fgIsCrossBand = FALSE;
	prBssInfo->eBand = eNewBand;

	/* Store VHT Channel width for later op mode operation */
	prBssInfo->ucVhtChannelWidthBeforeCsa = prBssInfo->ucVhtChannelWidth;

	prBssInfo->ucVhtChannelWidth = prCSAParams->ucVhtBw;
	prBssInfo->ucVhtChannelFrequencyS1 = prCSAParams->ucVhtS1;
	prBssInfo->ucVhtChannelFrequencyS2 = prCSAParams->ucVhtS2;

	prBssInfo->ucOpRxNssBeforeCsa = prBssInfo->ucOpRxNss;
	prBssInfo->ucOpTxNssBeforeCsa = prBssInfo->ucOpTxNss;

	if (HAS_WIDE_BAND_PARAMS(prCSAParams)) {
		if (prBssInfo->fgIsOpChangeChannelWidth &&
		    rlmGetVhtOpBwByBssOpBw(prBssInfo->ucOpChangeChannelWidth) <
			     prBssInfo->ucVhtChannelWidth) {

			DBGLOG(RLM, LOUD,
			       "Change to w:%d s1:%d s2:%d since own changed BW < peer's WideBand BW",
			       prBssInfo->ucVhtChannelWidth,
			       prBssInfo->ucVhtChannelFrequencyS1,
			       prBssInfo->ucVhtChannelFrequencyS2);
			rlmFillVhtOpInfoByBssOpBw(prAdapter,
				prBssInfo, prBssInfo->ucOpChangeChannelWidth);
		}
	}

	prBssInfo->eBssScoBeforeCsa = prBssInfo->eBssSCO;
	if (HAS_SCO_PARAMS(prCSAParams))
		prBssInfo->eBssSCO = prCSAParams->eSco;

	COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
		  prBssInfo->aucSSID, prBssInfo->ucSSIDLen);
	prBssDesc = scanSearchBssDescByBssidAndSsid(
			prAdapter, prBssInfo->aucBSSID, TRUE, &rSsid);

	if (prBssDesc) {
		DBGLOG(RLM, INFO,
		       "[%d] DFS: BSS: " MACSTR
		       " Desc found, channel from %u to %u (band from %u to %u) with sco:%u\n ",
		       ucBssIndex,
		       MAC2STR(prBssInfo->aucBSSID),
		       prBssDesc->ucChannelNum, prCSAParams->ucCsaNewCh,
		       prBssDesc->eBand, prBssInfo->eBand, prBssInfo->eBssSCO);

		prBssDesc->eBand = prBssInfo->eBand;
		prBssDesc->ucChannelNum = prBssInfo->ucPrimaryChannel;
		prBssDesc->eChannelWidth = prBssInfo->ucVhtChannelWidth;
		prBssDesc->ucCenterFreqS1 = prBssInfo->ucVhtChannelFrequencyS1;
		prBssDesc->ucCenterFreqS2 = prBssInfo->ucVhtChannelFrequencyS2;

#if CFG_ENABLE_WIFI_DIRECT
		if (IS_BSS_P2P(prBssInfo))
			p2pFuncSwitchGcChannel(prAdapter, prBssInfo);
#endif
	} else {
		DBGLOG(RLM, INFO,
		       "DFS: BSS: " MACSTR " Desc is not found\n ",
		       MAC2STR(prBssInfo->aucBSSID));
	}

#ifdef CFG_DFS_CHSW_FORCE_BW20
	/*DFS Certification for Channel Bandwidth 20MHz */
	prBssInfo->eBssSCO = CHNL_EXT_SCN;
	prBssInfo->ucVhtChannelWidth = CW_20_40MHZ;
	prBssInfo->ucVhtChannelFrequencyS1 = 0;
	prBssInfo->ucVhtChannelFrequencyS2 = 255;
	prBssInfo->ucHtOpInfo1 &=
		~(HT_OP_INFO1_SCO | HT_OP_INFO1_STA_CHNL_WIDTH);
	DBGLOG(RLM, INFO, "Ch : DFS has Appeared\n");
#endif

	rlmReviseMaxBw(prAdapter, prBssInfo->ucBssIndex, &prBssInfo->eBssSCO,
		       (enum ENUM_CHANNEL_WIDTH *)&prBssInfo->ucVhtChannelWidth,
		       &prBssInfo->ucVhtChannelFrequencyS1,
		       &prBssInfo->ucPrimaryChannel);

	rlmRevisePreferBandwidthNss(prAdapter, prBssInfo->ucBssIndex, prStaRec);

	if (!rlmDomainIsValidRfSetting(
		    prAdapter, prBssInfo->eBand, prBssInfo->ucPrimaryChannel,
		    prBssInfo->eBssSCO, prBssInfo->ucVhtChannelWidth,
		    prBssInfo->ucVhtChannelFrequencyS1,
		    prBssInfo->ucVhtChannelFrequencyS2)) {
		prBssInfo->ucVhtChannelWidth = CW_20_40MHZ;
		prBssInfo->ucVhtChannelFrequencyS1 = 0;
		prBssInfo->ucVhtChannelFrequencyS2 = 0;
		prBssInfo->eBssSCO = CHNL_EXT_SCN;
		prBssInfo->ucHtOpInfo1 &=
			~(HT_OP_INFO1_SCO | HT_OP_INFO1_STA_CHNL_WIDTH);
#if CFG_ENABLE_WIFI_DIRECT
		/* Check SAP channel */
		p2pFuncSwitchSapChannel(prAdapter,
			P2P_DEFAULT_SCENARIO);
#endif
	}

	rlmResetCSAParams(prBssInfo, FALSE);

	if (IS_BSS_AIS(prBssInfo))
		aisFunSwitchChannel(prAdapter, prBssInfo);
}

void rlmCsaDoneTimeout(struct ADAPTER *prAdapter,
				   uintptr_t ulParamPtr)
{
	uint8_t ucBssIndex = (uint8_t) ulParamPtr;
	struct BSS_INFO *prBssInfo;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(AIS, INFO, "No prBssInfo\n");
		return;
	}

	DBGLOG(RLM, WARN, "[CSA] BSS=%d, AP isn't switch to new channel!\n",
		prBssInfo->ucBssIndex);

	prBssInfo->u2DeauthReason = REASON_CODE_BEACON_TIMEOUT * 100;
	aisBssBeaconTimeout(prAdapter, prBssInfo->ucBssIndex);
}

#endif /* CFG_SUPPORT_DFS */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Update STBC settings
 *
 * \param[in] enable 1: force STBC, 0: disable force STBC
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmUpdateStbcSetting(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t enable, uint8_t notify)
{
	struct BSS_INFO *prBssInfo = NULL;
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
	uint8_t cmd[30] = {0};
	uint8_t strLen = 0;
	uint32_t strOutLen = 0;

	if (prAdapter == NULL)
		return WLAN_STATUS_FAILURE;

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	if (prBssInfo == NULL)
		return WLAN_STATUS_FAILURE;

	/* skip non alive BSS */
	if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo)) {
		DBGLOG(RLM, ERROR, "Skip non alive BSS[%d]\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	/* skip non STA/GC */
	if ((prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE) ||
		    (!prBssInfo->prStaRecOfAP)) {
		DBGLOG(RLM, ERROR, "Skip invalid STA BSS[%d]\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	strLen = kalSnprintf(cmd, sizeof(cmd),
			"SET_STBC %d %d", ucBssIndex, enable);

	DBGLOG(RLM, INFO, "Notify FW %s, strlen=%d", cmd, strLen);

	rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
	rChipConfigInfo.u2MsgSize = strLen;
	kalStrnCpy(rChipConfigInfo.aucCmd, cmd, strLen);
	wlanSetChipConfig(prAdapter, &rChipConfigInfo,
		sizeof(rChipConfigInfo), &strOutLen, FALSE);

	if (enable) {
		prBssInfo->eForceStbc = STBC_MRC_STATE_ENABLED;
		if (notify)
			kalSendUevent(prAdapter,
				      "forcestbc=status:Success,enable=True");
	} else {
		prBssInfo->eForceStbc = STBC_MRC_STATE_DISABLED;
		if (notify)
			kalSendUevent(prAdapter,
				      "forcestbc=status:Success,enable=False");
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Update MRC settings
 *
 * \param[in] enable 1: notify AP to NSS 1x1, 0: notify AP to NSS 2x2
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmUpdateMrcSetting(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t enable)
{
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t ucOpRxNss;
	uint8_t ucChannelWidth;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	if (prAdapter == NULL)
		return WLAN_STATUS_FAILURE;

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	if (prBssInfo == NULL)
		return WLAN_STATUS_FAILURE;

	/* skip non alive BSS */
	if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo)) {
		DBGLOG(RLM, ERROR, "Skip non alive BSS[%d]\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	/* skip non STA/GC */
	if ((prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE) ||
		    (!prBssInfo->prStaRecOfAP)) {
		DBGLOG(RLM, ERROR, "Skip invalid STA BSS[%d]\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	/* skip if already 1x1 */
	if (prBssInfo->ucOpRxNss == 1) {
		DBGLOG(RLM, ERROR, "Skip BSS[%d] if already 1x1\n",
				ucBssIndex);
		kalSendUevent(prAdapter,
			      "forcemrc=status:Fail,reason=Already1x1");
		return WLAN_STATUS_FAILURE;
	}

	/* skip on going op mode change */
	if (prBssInfo->pfOpChangeHandler) {
		DBGLOG(RLM, ERROR, "Skip ongoing op mode change: BSS[%d]\n",
				ucBssIndex);
		kalSendUevent(prAdapter,
			      "forcemrc=status:Fail,reason=OngoingOpMode");
		return WLAN_STATUS_FAILURE;
	}

	ucOpRxNss = (enable) ? (1) :
		(wlanGetSupportNss(prAdapter, ucBssIndex));
	ucChannelWidth = rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo);

#if CFG_SUPPORT_802_11AC
	if (RLM_NET_IS_11AC(prBssInfo)
#if (CFG_SUPPORT_802_11AX == 1)
			|| RLM_NET_IS_11AX(prBssInfo)
#endif /* (CFG_SUPPORT_802_11AX == 1) */
	   ) {
		u4Status = rlmSendOpModeNotificationFrame(prAdapter,
				prBssInfo->prStaRecOfAP,
				ucChannelWidth, ucOpRxNss);

		DBGLOG(RLM, INFO,
			"Send VHT OP notification frame: BSS[%d] BW[%d] RxNss[%d] Status[%d]\n",
			ucBssIndex, ucChannelWidth,
			ucOpRxNss, u4Status);
	}
#endif /* CFG_SUPPORT_802_11AC */

	if (RLM_NET_IS_11N(prBssInfo)
#if (CFG_SUPPORT_802_11AX == 1)
			|| RLM_NET_IS_11AX(prBssInfo)
#endif
	   ) {
		u4Status = rlmSendSmPowerSaveFrame(prAdapter,
				prBssInfo->prStaRecOfAP, ucOpRxNss);
		DBGLOG(RLM, INFO,
			"Send HT SM Power Save frame: BSS[%d] RxNss[%d] Status[%d]\n",
			ucBssIndex, ucOpRxNss, u4Status);
	}

	if (u4Status == WLAN_STATUS_SUCCESS) {
		prBssInfo->eForceMrc = (enable) ?
			STBC_MRC_STATE_ENABLING : STBC_MRC_STATE_DISABLING;
		DBGLOG(RLM, INFO,
			"Updating BSS[%d] MRC setting to %d, state = %d.\n",
			ucBssIndex, enable, prBssInfo->eForceMrc);
	} else {
		kalSendUevent(prAdapter,
			      "forcemrc=status:Fail,reason=SendActionFail");
	}

	return u4Status;
}

static void rlmResetMrc(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo = NULL;

	if (prAdapter == NULL)
		return;

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	if (prBssInfo == NULL)
		return;

	if (prBssInfo->eForceMrc != STBC_MRC_STATE_DISABLED) {
		prBssInfo->eForceMrc = STBC_MRC_STATE_DISABLED;
		kalSendUevent(prAdapter,
			      "forcemrc=status:Fail,reason=MrcIsReset");
	}
}

static void rlmUpdateMrcTxDone(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t fgIsSuccess)
{
	struct BSS_INFO *prBssInfo = NULL;

	if (prAdapter == NULL)
		return;

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	if (prBssInfo == NULL)
		return;

	switch (prBssInfo->eForceMrc) {
	case STBC_MRC_STATE_ENABLING:
		prBssInfo->eForceMrc = (fgIsSuccess) ?
			STBC_MRC_STATE_ENABLED : STBC_MRC_STATE_DISABLED;
		DBGLOG(RLM, INFO,
			"TxDone [Status: %d] BSS[%d] MRC state = %d.\n",
			fgIsSuccess, ucBssIndex, prBssInfo->eForceMrc);
		if (fgIsSuccess)
			kalSendUevent(prAdapter,
				      "forcemrc=status:Success,enable=True");
		else
			kalSendUevent(prAdapter,
				      "forcemrc=status:Fail,reason=SendActionFail");
		break;

	case STBC_MRC_STATE_DISABLING:
		prBssInfo->eForceMrc = (fgIsSuccess) ?
			STBC_MRC_STATE_DISABLED : STBC_MRC_STATE_ENABLED;
		DBGLOG(RLM, INFO,
			"TxDone [Status: %d] BSS[%d] MRC state = %d.\n",
			fgIsSuccess, ucBssIndex, prBssInfo->eForceMrc);
		if (fgIsSuccess)
			kalSendUevent(prAdapter,
				      "forcemrc=status:Success,enable=False");
		else
			kalSendUevent(prAdapter,
				      "forcemrc=status:Fail,reason=SendActionFail");
		break;

	default:
		break;
	}
}

static void rlmOpModeDummyTxDoneHandler(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo, uint8_t ucOpChangeType,
		uint8_t fgIsSuccess)
{
	struct BSS_INFO *prBssInfo = NULL;

	if (prAdapter == NULL || prMsduInfo == NULL)
		return;

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (prBssInfo == NULL)
		return;

	DBGLOG(RLM, INFO,
		"OP notification Tx done: BSS[%d] Type[%d] IsSuccess[%d]\n",
		prBssInfo->ucBssIndex, ucOpChangeType, fgIsSuccess);

	rlmUpdateMrcTxDone(prAdapter, prMsduInfo->ucBssIndex, fgIsSuccess);
}

static uint32_t rlmDummyOmiOpModeTxDone(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	rlmOpModeDummyTxDoneHandler(prAdapter, prMsduInfo,
			OP_NOTIFY_TYPE_OMI_NSS_BW,
			(rTxDoneStatus == TX_RESULT_SUCCESS) ? TRUE : FALSE);
	return WLAN_STATUS_SUCCESS;
}

static uint32_t rlmDummyVhtOpModeTxDone(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	rlmOpModeDummyTxDoneHandler(prAdapter, prMsduInfo,
			OP_NOTIFY_TYPE_VHT_NSS_BW,
			(rTxDoneStatus == TX_RESULT_SUCCESS) ? TRUE : FALSE);
	return WLAN_STATUS_SUCCESS;
}

static uint32_t rlmDummySmPowerSaveTxDone(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	rlmOpModeDummyTxDoneHandler(prAdapter, prMsduInfo,
			OP_NOTIFY_TYPE_HT_NSS,
			(rTxDoneStatus == TX_RESULT_SUCCESS) ? TRUE : FALSE);
	return WLAN_STATUS_SUCCESS;
}

void rlmProcessPublicAction(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb)
{
	struct STA_RECORD *prStaRec = NULL;
	struct WLAN_ACTION_FRAME *prActFrame = NULL;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return;

	if (prStaRec->ucBssIndex > prAdapter->ucHwBssIdNum)
		return;

	prActFrame = (struct WLAN_ACTION_FRAME *) prSwRfb->pvHeader;

	switch (prActFrame->ucAction) {
#if CFG_ENABLE_WIFI_DIRECT
	case ACTION_PUBLIC_20_40_COEXIST:
		rlmProcessPublicAction2040Coexist(prAdapter, prSwRfb);
		break;
#endif
#if CFG_SUPPORT_DFS
	case ACTION_PUBLIC_EX_CH_SW_ANNOUNCEMENT:
		rlmProcessPublicActionExCsa(prAdapter, prSwRfb);
		break;
#endif
	case ACTION_PUBLIC_VENDOR_SPECIFIC:
	default:
		break;
	}
}

uint32_t
rlmSendOpModeFrameByType(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucOpChangeType,
	uint8_t ucChannelWidth,
	uint8_t ucRxNss, uint8_t ucTxNss)
{
	switch (ucOpChangeType) {
	case OP_NOTIFY_TYPE_HT_NSS:
		return rlmSendSmPowerSaveFrame(prAdapter,
			prStaRec, ucRxNss);
	case OP_NOTIFY_TYPE_HT_BW:
		return rlmSendNotifyChannelWidthFrame(prAdapter,
			prStaRec, ucChannelWidth);
	case OP_NOTIFY_TYPE_VHT_NSS_BW:
		return rlmSendOpModeNotificationFrame(prAdapter,
			prStaRec, ucChannelWidth, ucRxNss);
#if (CFG_SUPPORT_802_11AX == 1) || (CFG_SUPPORT_802_11BE == 1)
	case OP_NOTIFY_TYPE_OMI_NSS_BW:
		return rlmSendOMIDataFrame(prAdapter,
			prStaRec, ucChannelWidth,
			ucRxNss, ucTxNss,
			rlmNotifyOMIOpModeTxDone);
#endif /* CFG_SUPPORT_802_11AX || CFG_SUPPORT_802_11BE == 1 */
	default:
		DBGLOG(RLM, WARN,
			"Can't find op change type [%d]\n",
			ucOpChangeType);
		return WLAN_STATUS_FAILURE;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Send OpMode Norification frame (VHT action frame)
 *
 * \param[in] ucChannelWidth 0:20MHz, 1:40MHz, 2:80MHz, 3:160MHz or 80+80MHz
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmSendOpModeNotificationFrame(struct ADAPTER *prAdapter,
				    struct STA_RECORD *prStaRec,
				    uint8_t ucChannelWidth, uint8_t ucOpRxNss)
{

	struct MSDU_INFO *prMsduInfo;
	struct ACTION_OP_MODE_NOTIFICATION_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	PFN_TX_DONE_HANDLER pfTxDoneHandler =
		(PFN_TX_DONE_HANDLER)rlmDummyVhtOpModeTxDone;

	/* Sanity Check */
	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
			      sizeof(struct ACTION_OP_MODE_NOTIFICATION_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
							u2EstimatedFrameLen);

	if (!prMsduInfo)
		return WLAN_STATUS_FAILURE;

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	prTxFrame = prMsduInfo->prPacket;

	/* Fill frame ctrl */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	/* 3 Compose the frame body's frame */
	prTxFrame->ucCategory = CATEGORY_VHT_ACTION;
	prTxFrame->ucAction = ACTION_OPERATING_MODE_NOTIFICATION;

	/* 9.4.1.53 Operating Mode field */
	if (ucChannelWidth >= MAX_BW_160MHZ) {
		prTxFrame->ucOperatingMode &=
			~(VHT_OP_MODE_CHANNEL_WIDTH |
			VHT_OP_MODE_CHANNEL_WIDTH_80P80_160);
		prTxFrame->ucOperatingMode |=
			(VHT_OP_MODE_CHANNEL_WIDTH_80 &
			VHT_OP_MODE_CHANNEL_WIDTH) |
			VHT_OP_MODE_CHANNEL_WIDTH_80P80_160;
	} else {
		prTxFrame->ucOperatingMode |=
			(ucChannelWidth & VHT_OP_MODE_CHANNEL_WIDTH);
	}

	if (ucOpRxNss == 0)
		ucOpRxNss = 1;
	prTxFrame->ucOperatingMode |=
		(((ucOpRxNss - 1) << 4) & VHT_OP_MODE_RX_NSS);
	prTxFrame->ucOperatingMode &= ~VHT_OP_MODE_RX_NSS_TYPE;

	if (prBssInfo->pfOpChangeHandler)
		pfTxDoneHandler = rlmNotifyVhtOpModeTxDone;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prBssInfo->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     sizeof(struct ACTION_OP_MODE_NOTIFICATION_FRAME),
		     pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	/* 4 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

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
uint32_t rlmSendSmPowerSaveFrame(struct ADAPTER *prAdapter,
			     struct STA_RECORD *prStaRec, uint8_t ucOpRxNss)
{
	struct MSDU_INFO *prMsduInfo;
	struct ACTION_SM_POWER_SAVE_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	PFN_TX_DONE_HANDLER pfTxDoneHandler =
		(PFN_TX_DONE_HANDLER)rlmDummySmPowerSaveTxDone;

	/* Sanity Check */
	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
			      sizeof(struct ACTION_SM_POWER_SAVE_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
							u2EstimatedFrameLen);

	if (!prMsduInfo)
		return WLAN_STATUS_FAILURE;

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

	if (ucOpRxNss == 1)
		prTxFrame->ucSmPowerCtrl |= HT_SM_POWER_SAVE_CONTROL_ENABLED;
	else if (ucOpRxNss == 2)
		prTxFrame->ucSmPowerCtrl &= ~HT_SM_POWER_SAVE_CONTROL_ENABLED;
	else {
		DBGLOG(RLM, WARN,
		       "Can't switch to RxNss = %d since we don't support.\n",
		       ucOpRxNss);
		return WLAN_STATUS_FAILURE;
	}

	/* Static SM power save mode */
	prTxFrame->ucSmPowerCtrl &=
		(~HT_SM_POWER_SAVE_CONTROL_SM_MODE);

	if (prBssInfo->pfOpChangeHandler)
		pfTxDoneHandler = rlmSmPowerSaveTxDone;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prBssInfo->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     sizeof(struct ACTION_SM_POWER_SAVE_FRAME), pfTxDoneHandler,
		     MSDU_RATE_MODE_AUTO);

	/* 4 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Send Notify Channel Width or NSS frame (OMI Data frame)
 *
 * \param[in] ucChannelWidth 0:20MHz, 1:Any channel width
 *  in the STAs Supported Channel Width Set subfield
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
#if (CFG_SUPPORT_802_11AX == 1) || (CFG_SUPPORT_802_11BE == 1)
uint32_t
rlmSendOMIDataFrame(struct ADAPTER *prAdapter,
		    struct STA_RECORD *prStaRec,
		    uint8_t ucChannelWidth,
		    uint8_t ucOpRxNss,
		    uint8_t ucOpTxNss,
		    PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	uint32_t u4Status;
	struct BSS_INFO *prBssInfo;
	uint8_t ucMaxBw;
	uint8_t ucEht = FALSE;
	PFN_TX_DONE_HANDLER __pfTxDoneHandler =
		(PFN_TX_DONE_HANDLER)rlmDummyOmiOpModeTxDone;

	/* Sanity Check */
	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;

	if (prBssInfo->pfOpChangeHandler) {
		prBssInfo->aucOpModeChangeState
			[OP_NOTIFY_TYPE_OMI_NSS_BW] =
			OP_NOTIFY_STATE_SENDING;
		__pfTxDoneHandler = pfTxDoneHandler;
		DBGLOG(RLM, INFO,
			"OMI Fill pfTxDoneHandler\n");
	}

	ucMaxBw = cnmGetBssMaxBw(prAdapter, prStaRec->ucBssIndex);
	if (ucChannelWidth >= ucMaxBw)
		ucChannelWidth = ucMaxBw;

	if (ucOpRxNss == 0)
		ucOpRxNss = 1;
	if (ucOpTxNss == 0)
		ucOpTxNss = 1;

#if (CFG_SUPPORT_802_11BE == 1)
	if (RLM_NET_IS_11BE(prBssInfo) &&
		(prStaRec->ucDesiredPhyTypeSet &
		PHY_TYPE_SET_802_11BE)) {
		ucEht = TRUE;
		ehtRlmInitHtcACtrlOM(prAdapter);
		if (ucChannelWidth == MAX_BW_320_1MHZ ||
			ucChannelWidth == MAX_BW_320_2MHZ) {
			/* BW320: CH_WIDTH EXT 1 and CH_WIDTH 0 */
			/* For NSS 0-7, Ext Nss is 0 */
			/* EHT OM */
			EHT_SET_HTC_EHT_OM_RX_NSS_EXT(prAdapter->u4HeHtcOM,
				0);
			EHT_SET_HTC_EHT_OM_TX_NSTS_EXT(prAdapter->u4HeHtcOM,
				0);
			EHT_SET_HTC_EHT_OM_CH_WIDTH_EXT(prAdapter->u4HeHtcOM,
				EHT_OM_CH_WIDTH_EXT_BW320);
			/* HE OM */
			EHT_SET_HTC_HE_OM_RX_NSS(prAdapter->u4HeHtcOM,
				ucOpRxNss - 1);
			EHT_SET_HTC_HE_OM_TX_NSTS(prAdapter->u4HeHtcOM,
				ucOpRxNss - 1);
			EHT_SET_HTC_HE_OM_CH_WIDTH(prAdapter->u4HeHtcOM,
				EHT_OM_CH_WIDTH_BW320);
		} else {
			/* EHT OM */
			EHT_SET_HTC_EHT_OM_RX_NSS_EXT(prAdapter->u4HeHtcOM,
				0);
			EHT_SET_HTC_EHT_OM_TX_NSTS_EXT(prAdapter->u4HeHtcOM,
				0);
			EHT_SET_HTC_EHT_OM_CH_WIDTH_EXT(prAdapter->u4HeHtcOM,
				EHT_OM_CH_WIDTH_EXT_LT_BW320);
			/* HE OM */
			EHT_SET_HTC_HE_OM_RX_NSS(prAdapter->u4HeHtcOM,
				ucOpRxNss - 1);
			EHT_SET_HTC_HE_OM_TX_NSTS(prAdapter->u4HeHtcOM,
				ucOpRxNss - 1);
			EHT_SET_HTC_HE_OM_CH_WIDTH(prAdapter->u4HeHtcOM,
				ucChannelWidth)

		}
		DBGLOG(RLM, TRACE,
			"Ready to send Htc null frame with EHT OM\n");
	}
#endif /* CFG_SUPPORT_802_11BE  */

#if (CFG_SUPPORT_802_11AX == 1)
	if (!ucEht && RLM_NET_IS_11AX(prBssInfo) &&
		(prStaRec->ucDesiredPhyTypeSet &
		PHY_TYPE_SET_802_11AX)) {
		heRlmInitHeHtcACtrlOMAndUPH(prAdapter);
		HE_SET_HTC_HE_OM_CH_WIDTH(prAdapter->u4HeHtcOM,
			ucChannelWidth);
		HE_SET_HTC_HE_OM_RX_NSS(prAdapter->u4HeHtcOM, ucOpRxNss - 1);
		HE_SET_HTC_HE_OM_TX_NSTS(prAdapter->u4HeHtcOM, ucOpTxNss - 1);
		DBGLOG(RLM, TRACE,
			"Ready to send Htc null frame with HE OM\n");
	}
#endif /* CFG_SUPPORT_802_11AX  */


	u4Status =
		heRlmSendHtcNullFrame(
		prAdapter, prStaRec, 7,
		__pfTxDoneHandler);

	return u4Status;

}
#endif /* CFG_SUPPORT_802_11AX  or CFG_SUPPORT_802_11BE*/
/*----------------------------------------------------------------------------*/
/*!
 * \brief Send Notify Channel Width frame (HT action frame)
 *
 * \param[in] ucChannelWidth 0:20MHz, 1:Any channel width
 *  in the STAs Supported Channel Width Set subfield
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmSendNotifyChannelWidthFrame(struct ADAPTER *prAdapter,
				    struct STA_RECORD *prStaRec,
				    uint8_t ucChannelWidth)
{
	struct MSDU_INFO *prMsduInfo;
	struct ACTION_NOTIFY_CHANNEL_WIDTH_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	PFN_TX_DONE_HANDLER pfTxDoneHandler = (PFN_TX_DONE_HANDLER)NULL;

	/* Sanity Check */
	if (!prStaRec)
		return WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
			      sizeof(struct ACTION_NOTIFY_CHANNEL_WIDTH_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
							u2EstimatedFrameLen);

	if (!prMsduInfo)
		return WLAN_STATUS_FAILURE;

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	prTxFrame = prMsduInfo->prPacket;

	/* Fill frame ctrl */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	/* 3 Compose the frame body's frame */
	prTxFrame->ucCategory = CATEGORY_HT_ACTION;
	prTxFrame->ucAction = ACTION_HT_NOTIFY_CHANNEL_WIDTH;

	prTxFrame->ucChannelWidth = ucChannelWidth;

	if (prBssInfo->pfOpChangeHandler)
		pfTxDoneHandler = rlmNotifyChannelWidthtTxDone;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prBssInfo->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     sizeof(struct ACTION_NOTIFY_CHANNEL_WIDTH_FRAME),
		     pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	/* 4 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmNotifyVhtOpModeTxDone(struct ADAPTER *prAdapter,
				  struct MSDU_INFO *prMsduInfo,
				  enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	u_int8_t fgIsSuccess = FALSE;

	do {
		ASSERT((prAdapter != NULL) && (prMsduInfo != NULL));

		if (rTxDoneStatus == TX_RESULT_SUCCESS)
			fgIsSuccess = TRUE;

	} while (FALSE);

	rlmOpModeTxDoneHandler(prAdapter, prMsduInfo, OP_NOTIFY_TYPE_VHT_NSS_BW,
			       fgIsSuccess);

	return WLAN_STATUS_SUCCESS;
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
uint32_t rlmNotifyOMIOpModeTxDone(struct ADAPTER *prAdapter,
				  struct MSDU_INFO *prMsduInfo,
				  enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	u_int8_t fgIsSuccess = FALSE;

	do {
		ASSERT((prAdapter != NULL) && (prMsduInfo != NULL));

		if (rTxDoneStatus == TX_RESULT_SUCCESS)
			fgIsSuccess = TRUE;

	} while (FALSE);

	rlmOpModeTxDoneHandler(prAdapter, prMsduInfo, OP_NOTIFY_TYPE_OMI_NSS_BW,
		fgIsSuccess);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
rlmNotifyApGoOmiOpModeTxDone(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsduInfo,
			     enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct BSS_INFO *prBssInfo = NULL;

	do {
		ASSERT((prAdapter != NULL) && (prMsduInfo != NULL));

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
						  prMsduInfo->ucBssIndex);
		if (!prBssInfo)
			break;

		DBGLOG(RLM, INFO,
			"bss[%u] sta[%u] success=%d\n",
			prMsduInfo->ucBssIndex,
			prMsduInfo->ucStaRecIndex,
			rTxDoneStatus);

		rlmApGoOmiOpModeDoneHandler(prAdapter, prMsduInfo);
	} while (FALSE);

	return WLAN_STATUS_SUCCESS;
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
uint32_t rlmSmPowerSaveTxDone(struct ADAPTER *prAdapter,
			      struct MSDU_INFO *prMsduInfo,
			      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	u_int8_t fgIsSuccess = FALSE;

	do {
		ASSERT((prAdapter != NULL) && (prMsduInfo != NULL));

		if (rTxDoneStatus == TX_RESULT_SUCCESS)
			fgIsSuccess = TRUE;

	} while (FALSE);

	rlmOpModeTxDoneHandler(prAdapter, prMsduInfo, OP_NOTIFY_TYPE_HT_NSS,
			       fgIsSuccess);

	return WLAN_STATUS_SUCCESS;
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
uint32_t rlmNotifyChannelWidthtTxDone(struct ADAPTER *prAdapter,
				      struct MSDU_INFO *prMsduInfo,
				      enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	u_int8_t fgIsSuccess = FALSE;

	do {
		ASSERT((prAdapter != NULL) && (prMsduInfo != NULL));

		if (rTxDoneStatus == TX_RESULT_SUCCESS)
			fgIsSuccess = TRUE;

	} while (FALSE);

	rlmOpModeTxDoneHandler(prAdapter, prMsduInfo, OP_NOTIFY_TYPE_HT_BW,
			       fgIsSuccess);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle TX done for OP mode noritfication frame
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static void rlmOpModeTxDoneHandler(struct ADAPTER *prAdapter,
				   struct MSDU_INFO *prMsduInfo,
				   uint8_t ucOpChangeType,
				   u_int8_t fgIsSuccess)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct BSS_INFO *prBssInfo = NULL;
	struct STA_RECORD *prStaRec = NULL;
	u_int8_t fgIsOpModeChangeSuccess = FALSE; /* OP change result */
	uint8_t *pucCurrOpState = NULL;
	uint8_t ucFailCnt = 0, i = 0;

	/* Sanity check */
	ASSERT((prAdapter != NULL) && (prMsduInfo != NULL));

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	prStaRec = prBssInfo->prStaRecOfAP;
	if (!prStaRec)
		return;

	DBGLOG(RLM, INFO,
	       "OP notification Tx done: BSS[%d] Type[%d] Status[%d] IsSuccess[%d]\n",
	       prBssInfo->ucBssIndex, ucOpChangeType,
	       prBssInfo->aucOpModeChangeState[ucOpChangeType], fgIsSuccess);

	do {
		/* <1>handle abnormal case */
		if ((prBssInfo->aucOpModeChangeState[ucOpChangeType] !=
		     OP_NOTIFY_STATE_KEEP) &&
		    (prBssInfo->aucOpModeChangeState[ucOpChangeType] !=
		     OP_NOTIFY_STATE_SENDING) &&
		     (prBssInfo->aucOpModeChangeState[ucOpChangeType] !=
		     OP_NOTIFY_STATE_ROLLBACK)) {
			DBGLOG(RLM, WARN,
			       "Unexpected BSS[%d] OpModeChangeState[%d]\n",
			       prBssInfo->ucBssIndex,
			       prBssInfo->aucOpModeChangeState[ucOpChangeType]);
			rlmRollbackOpChangeParam(prBssInfo, TRUE, TRUE);
			fgIsOpModeChangeSuccess = FALSE;
			break;
		}

		if (ucOpChangeType >= OP_NOTIFY_TYPE_NUM) {
			DBGLOG(RLM, WARN,
			       "Uxexpected Bss[%d] OpChangeType[%d]\n",
			       prMsduInfo->ucBssIndex, ucOpChangeType);
			fgIsOpModeChangeSuccess = FALSE;
			break;
		}

		pucCurrOpState = &prBssInfo
				->aucOpModeChangeState[ucOpChangeType];


		/* <3.1>handle TX done - SUCCESS */
		if (fgIsSuccess == TRUE) {
			/* Clear retry count */
			prBssInfo->aucOpModeChangeRetryCnt[ucOpChangeType]
							= 0;
			*pucCurrOpState = OP_NOTIFY_STATE_SUCCESS;

			/* Record rollback frame status to fail only */
			if (prBssInfo->aucOpModeChangeState[ucOpChangeType]
				== OP_NOTIFY_STATE_ROLLBACK) {
				*pucCurrOpState = OP_NOTIFY_STATE_FAIL;
				DBGLOG(RLM, INFO,
					"type [%d] roll back success\n",
					ucOpChangeType);
				return;
			}
		} else {
			/* Record rollback frame status to fail only */
			if (prBssInfo->aucOpModeChangeState[ucOpChangeType]
				== OP_NOTIFY_STATE_ROLLBACK) {
				DBGLOG(RLM, WARN,
					"type [%d] roll back fail\n",
					ucOpChangeType);
				*pucCurrOpState = OP_NOTIFY_STATE_FAIL;
				return;
			}

			prBssInfo->aucOpModeChangeRetryCnt[ucOpChangeType]++;

			/* Re-send notification frame */
			if (prBssInfo
				    ->aucOpModeChangeRetryCnt[ucOpChangeType] <=
			    OPERATION_NOTICATION_TX_LIMIT) {
				u4Status = rlmSendOpModeFrameByType(prAdapter,
					prStaRec, ucOpChangeType,
					prBssInfo->ucOpChangeChannelWidth,
					prBssInfo->ucOpChangeRxNss,
					prBssInfo->ucOpChangeTxNss);

				if (u4Status == WLAN_STATUS_SUCCESS)
					return;
			} else {
				*pucCurrOpState = OP_NOTIFY_STATE_FAIL;
				/* Clear retry count when retry */
				/* count > TX limit */
				prBssInfo
				->aucOpModeChangeRetryCnt[ucOpChangeType] = 0;
			}
		}


		for (i = 0; i < OP_NOTIFY_TYPE_NUM; i++) {
			if (prBssInfo->aucOpModeChangeState[i]
					== OP_NOTIFY_STATE_SENDING) {
				/* Wait for All BW/Nss notification
				 * frames Tx done
				 */
				return;
			} else if (prBssInfo->aucOpModeChangeState[i]
					== OP_NOTIFY_STATE_FAIL) {
				ucFailCnt++;
				DBGLOG(RLM, WARN,
					"OpType[%d] Tx Failed, fail[%d]",
					i, ucFailCnt);
			}
		}

		if (ucFailCnt == 0) {
			/* All notification frame sent */
			fgIsOpModeChangeSuccess = TRUE;
			break;
		}
		/* If any tx fail occurs, rollback the successful one */
		for (i = 0; i < OP_NOTIFY_TYPE_NUM; i++) {
			if (prBssInfo->aucOpModeChangeState[i]
				== OP_NOTIFY_STATE_SUCCESS) {
				prBssInfo->aucOpModeChangeState[i]
					= OP_NOTIFY_STATE_ROLLBACK;
				rlmRollbackOpChangeParam(prBssInfo,
						TRUE, TRUE);
				DBGLOG(RLM, WARN,
				"Type[%d] roll back to BW[%d] RxNss[%d] TxNss[%d]\n",
				i,
				rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo),
				prBssInfo->ucOpRxNss,
				prBssInfo->ucOpTxNss);
				rlmSendOpModeFrameByType(prAdapter,
				prStaRec, i,
				rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo),
				prBssInfo->ucOpRxNss,
				prBssInfo->ucOpTxNss);
			}
		}
		fgIsOpModeChangeSuccess = FALSE;

	} while (FALSE);

	/* <4>Change own OP info */
	rlmCompleteOpModeChange(prAdapter, prBssInfo, fgIsOpModeChangeSuccess);

	/* notify FW if no active BSS or no pending action frame */
#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
	if (prAdapter->fgANTCtrl) {
		DBGLOG(RLM, INFO,
			"ANT Control [Enable:%d], Pending count = %d\n",
			prAdapter->fgANTCtrl, prAdapter->ucANTCtrlPendingCount);
		if (prAdapter->ucANTCtrlPendingCount > 0)
			prAdapter->ucANTCtrlPendingCount--;
		if (prAdapter->ucANTCtrlPendingCount == 0)
			rlmSyncAntCtrl(prAdapter,
				prBssInfo->ucOpTxNss, prBssInfo->ucOpRxNss);
	}
#endif
}

static void rlmApGoOmiOpModeDoneHandler(struct ADAPTER *prAdapter,
					struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t ucOpChangeType = (uint8_t)OP_NOTIFY_TYPE_OMI_NSS_BW;

	/* Sanity check */
	if (!prAdapter || !prMsduInfo) {
		DBGLOG(RLM, WARN,
		       "prAdapter=0x%p prMsduInfo=0x%p\n",
		       prAdapter, prMsduInfo);
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (!prBssInfo || prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) {
		DBGLOG(RLM, WARN,
		       "prBssInfo=0x%p mode=%d\n",
		       prBssInfo,
		       prBssInfo != NULL ? prBssInfo->eCurrentOPMode : -1);
		return;
	}

	DBGLOG(RLM, INFO,
	       "OP notification done: BSS[%d] State[%d] WaitCnt[%d]\n",
	       prBssInfo->ucBssIndex,
	       prBssInfo->aucOpModeChangeState[ucOpChangeType],
	       prBssInfo->ucOmiWaitingCount);

	if (prBssInfo->aucOpModeChangeState[ucOpChangeType] !=
	    OP_NOTIFY_STATE_SENDING)
		return;

	if (prBssInfo->ucOmiWaitingCount > 0)
		prBssInfo->ucOmiWaitingCount--;

	if (prBssInfo->ucOmiWaitingCount > 0)
		return;

	prBssInfo->aucOpModeChangeState[ucOpChangeType] =
		OP_NOTIFY_STATE_SUCCESS;

	rlmCompleteOpModeChange(prAdapter, prBssInfo, TRUE);

	/* notify FW if no active BSS or no pending action frame */
#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
	if (prAdapter->fgANTCtrl) {
		DBGLOG(RLM, INFO,
			"ANT Control [Enable:%d], Pending count = %d\n",
			prAdapter->fgANTCtrl,
			prAdapter->ucANTCtrlPendingCount);
		if (prAdapter->ucANTCtrlPendingCount > 0)
			prAdapter->ucANTCtrlPendingCount--;
		if (prAdapter->ucANTCtrlPendingCount == 0)
			rlmSyncAntCtrl(prAdapter, prBssInfo->ucOpTxNss,
				       prBssInfo->ucOpRxNss);
	}
#endif
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
static void rlmRollbackOpChangeParam(struct BSS_INFO *prBssInfo,
				     u_int8_t fgIsRollbackBw,
				     u_int8_t fgIsRollbackNss)
{

	ASSERT(prBssInfo);

	if (fgIsRollbackBw == TRUE) {
		prBssInfo->fgIsOpChangeChannelWidth = FALSE;
		prBssInfo->ucOpChangeChannelWidth =
			rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo);
	}

	if (fgIsRollbackNss == TRUE) {
		prBssInfo->fgIsOpChangeRxNss = FALSE;
		prBssInfo->fgIsOpChangeTxNss = FALSE;
		prBssInfo->ucOpChangeRxNss = prBssInfo->ucOpRxNss;
		prBssInfo->ucOpChangeTxNss = prBssInfo->ucOpTxNss;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Get BSS operating channel width by VHT and HT OP Info
 *
 * \param[in]
 *
 * \return ucBssOpBw 0:20MHz, 1:40MHz, 2:80MHz, 3:160MHz 4:80+80MHz
 *
 */
/*----------------------------------------------------------------------------*/
uint8_t rlmGetBssOpBwByVhtAndHtOpInfo(struct BSS_INFO *prBssInfo)
{

	uint8_t ucBssOpBw = MAX_BW_20MHZ;
	uint8_t ucChannelWidth = prBssInfo->ucVhtChannelWidth;

	ASSERT(prBssInfo);

	switch (ucChannelWidth) {
	case VHT_OP_CHANNEL_WIDTH_320_1:
		ucBssOpBw = MAX_BW_320_1MHZ;
		break;
	case VHT_OP_CHANNEL_WIDTH_320_2:
		ucBssOpBw = MAX_BW_320_2MHZ;
		break;

	case VHT_OP_CHANNEL_WIDTH_80P80:
		ucBssOpBw = MAX_BW_80_80_MHZ;
		break;

	case VHT_OP_CHANNEL_WIDTH_160:
		ucBssOpBw = MAX_BW_160MHZ;
		break;

	case VHT_OP_CHANNEL_WIDTH_80:
		ucBssOpBw = MAX_BW_80MHZ;
		break;

	case VHT_OP_CHANNEL_WIDTH_20_40:
		if (prBssInfo->eBssSCO != CHNL_EXT_SCN)
			ucBssOpBw = MAX_BW_40MHZ;
		break;
	default:
		DBGLOG(RLM, WARN, "%s: unexpected VHT channel width: %d\n",
		       __func__, ucChannelWidth);
#if CFG_SUPPORT_802_11AC
		if (RLM_NET_IS_11AC(prBssInfo))
			/*VHT default should support BW 80*/
			ucBssOpBw = MAX_BW_80MHZ;
#endif
		break;
	}

	return ucBssOpBw;
}

uint8_t rlmGetBssOpBwByOwnAndPeerCapability(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	uint8_t ucOpMaxBw;
	uint8_t ucBssOpBw = MAX_BW_20MHZ;
	struct STA_RECORD *prStaRec;

	ASSERT(prBssInfo);
	ASSERT(prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE);

	prStaRec = prBssInfo->prStaRecOfAP;

	if (prStaRec == NULL) {
		DBGLOG(RLM, WARN, "AP is gone? Use current BW setting\n");
		return rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo);
	}

	ucOpMaxBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);

#if CFG_SUPPORT_802_11AC
	/* VHT && EHT */
	if (RLM_NET_IS_11AC(prBssInfo)
#if (CFG_SUPPORT_802_11BE == 1)
	    || RLM_NET_IS_11BE(prBssInfo)
#endif
	    ) {
		switch (prStaRec->ucVhtOpChannelWidth) {
		case VHT_OP_CHANNEL_WIDTH_320_1:
			ucBssOpBw = MAX_BW_320_1MHZ;
			break;
		case VHT_OP_CHANNEL_WIDTH_320_2:
			ucBssOpBw = MAX_BW_320_2MHZ;
			break;
		case VHT_OP_CHANNEL_WIDTH_80P80:
			ucBssOpBw = MAX_BW_80_80_MHZ;
			break;
		case VHT_OP_CHANNEL_WIDTH_160:
			ucBssOpBw = MAX_BW_160MHZ;
			break;
		case VHT_OP_CHANNEL_WIDTH_80:
			ucBssOpBw = MAX_BW_80MHZ;
			break;
		case VHT_OP_CHANNEL_WIDTH_20_40:
			if (prBssInfo->eBssSCO != CHNL_EXT_SCN)
				ucBssOpBw = MAX_BW_40MHZ;
			break;
		default:
			ucBssOpBw = MAX_BW_80MHZ;
			break;
		}

		if (ucOpMaxBw > ucBssOpBw) {
			DBGLOG(RLM, WARN,
				"Reduce max op bw from %d to %d per peer's VHT capability\n",
				ucOpMaxBw, ucBssOpBw);
			ucOpMaxBw = ucBssOpBw;
		}
	} else
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	if (RLM_NET_IS_11AX(prBssInfo)) { /* HE */
		if (HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW80P80_5G(
			prStaRec->ucHePhyCapInfo))
			ucBssOpBw = MAX_BW_80_80_MHZ;
		else if (HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW160_5G(
			prStaRec->ucHePhyCapInfo))
			ucBssOpBw = MAX_BW_160MHZ;
		else
			ucBssOpBw = MAX_BW_80MHZ;

		if (ucOpMaxBw > ucBssOpBw) {
			DBGLOG(RLM, WARN,
				"Reduce max op bw from %d to %d per peer's HE capability\n",
				ucOpMaxBw, ucBssOpBw);
			ucOpMaxBw = ucBssOpBw;
		}
	} else
#endif
	if (RLM_NET_IS_11N(prBssInfo)) { /* HT */
		if ((prStaRec->ucHtPeerOpInfo1 & HT_OP_INFO1_STA_CHNL_WIDTH)
			&& prBssInfo->fg40mBwAllowed) {
			ucBssOpBw = MAX_BW_40MHZ;
		}

		if (ucOpMaxBw > ucBssOpBw) {
			DBGLOG(RLM, WARN,
				"Reduce max op bw from %d to %d per peer's HT capability\n",
				ucOpMaxBw, ucBssOpBw);
			ucOpMaxBw = ucBssOpBw;
		}
	}

	return ucOpMaxBw;
}

uint8_t rlmGetBssOpBwByChannelWidth(enum ENUM_CHNL_EXT eSco,
	enum ENUM_CHANNEL_WIDTH eChannelWidth)
{
	switch (eChannelWidth) {
	case CW_20_40MHZ:
		if (eSco != CHNL_EXT_SCN)
			return MAX_BW_40MHZ;
		else
			return MAX_BW_20MHZ;
	case CW_80MHZ:
		return MAX_BW_80MHZ;
	case CW_160MHZ:
		return MAX_BW_160MHZ;
	case CW_80P80MHZ:
		return MAX_BW_80_80_MHZ;
	case CW_320_1MHZ:
		return MAX_BW_320_1MHZ;
	case CW_320_2MHZ:
		return MAX_BW_320_2MHZ;
	default:
		DBGLOG(RLM, WARN, "unexpected channel width: %d\n",
			eChannelWidth);
		return 0;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return ucVhtOpBw 0:20M/40Hz, 1:80MHz, 2:160MHz, 3:80+80MHz
 *
 */
/*----------------------------------------------------------------------------*/
uint8_t rlmGetVhtOpBwByBssOpBw(uint8_t ucBssOpBw)
{
	uint8_t ucVhtOpBw =
		VHT_OP_CHANNEL_WIDTH_80; /*VHT default should support BW 80*/

	switch (ucBssOpBw) {
	case MAX_BW_20MHZ:
	case MAX_BW_40MHZ:
		ucVhtOpBw = VHT_OP_CHANNEL_WIDTH_20_40;
		break;

	case MAX_BW_80MHZ:
		ucVhtOpBw = VHT_OP_CHANNEL_WIDTH_80;
		break;

	case MAX_BW_160MHZ:
		ucVhtOpBw = VHT_OP_CHANNEL_WIDTH_160;
		break;

	case MAX_BW_80_80_MHZ:
		ucVhtOpBw = VHT_OP_CHANNEL_WIDTH_80P80;
		break;

	case MAX_BW_320_1MHZ:
		ucVhtOpBw = VHT_OP_CHANNEL_WIDTH_320_1;
		break;

	case MAX_BW_320_2MHZ:
		ucVhtOpBw = VHT_OP_CHANNEL_WIDTH_320_2;
		break;

	default:
		DBGLOG(RLM, WARN, "%s: unexpected Bss OP BW: %d\n", __func__,
		       ucBssOpBw);
		break;
	}

	return ucVhtOpBw;
}

uint8_t rlmGetVhtOpBw320ByS1(uint8_t ucS1)
{
	uint8_t ucBw320Pos = VHT_OP_CHANNEL_WIDTH_320_1; /* default */

	/* BW320-1 */
	if (ucS1 == 31 || ucS1 == 95 || ucS1 == 159)
		ucBw320Pos = VHT_OP_CHANNEL_WIDTH_320_1;
	/* BW320-2 */
	else if (ucS1 == 63 || ucS1 == 127 || ucS1 == 191)
		ucBw320Pos = VHT_OP_CHANNEL_WIDTH_320_2;
	else
		DBGLOG(NIC, WARN, "unexpected s1: %d\n", ucS1);

	return ucBw320Pos;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Get operating notification channel width by VHT and HT operating Info
 *
 * \param[in]
 *
 * \return ucOpModeBw 0:20MHz, 1:40MHz, 2:80MHz, 3:160MHz/80+80MHz
 *
 */
/*----------------------------------------------------------------------------*/
static uint8_t rlmGetOpModeBwByVhtAndHtOpInfo(struct BSS_INFO *prBssInfo)
{
	uint8_t ucOpModeBw = VHT_OP_MODE_CHANNEL_WIDTH_20;

	ASSERT(prBssInfo);

	switch (prBssInfo->ucVhtChannelWidth) {
	case VHT_OP_CHANNEL_WIDTH_20_40:
		if (prBssInfo->eBssSCO != CHNL_EXT_SCN)
			ucOpModeBw = VHT_OP_MODE_CHANNEL_WIDTH_40;
		break;
	case VHT_OP_CHANNEL_WIDTH_80:
		ucOpModeBw = VHT_OP_MODE_CHANNEL_WIDTH_80;
		break;
	case VHT_OP_CHANNEL_WIDTH_160:
	case VHT_OP_CHANNEL_WIDTH_80P80:
	case VHT_OP_CHANNEL_WIDTH_320_1:
	case VHT_OP_CHANNEL_WIDTH_320_2:
		ucOpModeBw = VHT_OP_MODE_CHANNEL_WIDTH_160_80P80;
		break;
	default:
		DBGLOG(RLM, WARN, "%s: unexpected VHT channel width: %d\n",
		       __func__, prBssInfo->ucVhtChannelWidth);
		/*VHT default IE should support BW 80*/
		ucOpModeBw = VHT_OP_MODE_CHANNEL_WIDTH_80;
		break;
	}

	return ucOpModeBw;
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
static void rlmChangeOwnOpInfo(struct ADAPTER *prAdapter,
			       struct BSS_INFO *prBssInfo)
{
	struct STA_RECORD *prStaRec;

	ASSERT((prAdapter != NULL) && (prBssInfo != NULL));

	/* Update own operating channel Width */
	if (prBssInfo->fgIsOpChangeChannelWidth) {
		if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_HT
#if (CFG_SUPPORT_802_11AX == 1)
			|| prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_HE
#endif
		) {
			/* Update HT OP Info*/
			if (prBssInfo->ucOpChangeChannelWidth == MAX_BW_20MHZ) {
				prBssInfo->ucHtOpInfo1 &=
					~HT_OP_INFO1_STA_CHNL_WIDTH;
				prBssInfo->eBssSCO = CHNL_EXT_SCN;
			} else {
				prBssInfo->ucHtOpInfo1 |=
					HT_OP_INFO1_STA_CHNL_WIDTH;

				if (prBssInfo->eCurrentOPMode ==
				    OP_MODE_INFRASTRUCTURE) {
					prStaRec = prBssInfo->prStaRecOfAP;
					if (!prStaRec)
						return;

					if ((prStaRec->ucHtPeerOpInfo1 &
					     HT_OP_INFO1_SCO) != CHNL_EXT_RES)
						prBssInfo->eBssSCO =
						(enum ENUM_CHNL_EXT)(
						prStaRec->ucHtPeerOpInfo1 &
						HT_OP_INFO1_SCO);
#if CFG_ENABLE_WIFI_DIRECT
				} else if (prBssInfo->eCurrentOPMode ==
					   OP_MODE_ACCESS_POINT) {
					prBssInfo->eBssSCO = rlmDecideScoForAP(
						prAdapter, prBssInfo);
#endif
				}
			}

			DBGLOG(RLM, INFO,
			       "Update BSS[%d] HT Channel Width Info to bw=%d sco=%d\n",
			       prBssInfo->ucBssIndex,
			       (uint8_t)((prBssInfo->ucHtOpInfo1 &
					  HT_OP_INFO1_STA_CHNL_WIDTH) >>
					 HT_OP_INFO1_STA_CHNL_WIDTH_OFFSET),
			       prBssInfo->eBssSCO);
		}

#if CFG_SUPPORT_802_11AC
		/* Update VHT OP Info*/
		if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_VHT
#if (CFG_SUPPORT_802_11AX == 1)
			|| prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_HE
#endif
		) {
			rlmFillVhtOpInfoByBssOpBw(
				prAdapter,
				prBssInfo,
				prBssInfo->ucOpChangeChannelWidth);

			DBGLOG(RLM, INFO,
				"Update BSS[%d] VHT Channel Width Info to w=%d s1=%d s2=%d\n",
				prBssInfo->ucBssIndex,
				prBssInfo->ucVhtChannelWidth,
				prBssInfo->ucVhtChannelFrequencyS1,
				prBssInfo->ucVhtChannelFrequencyS2);
		}
#endif

#if CFG_ENABLE_WIFI_DIRECT
#if (CFG_SUPPORT_802_11AX == 1)
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_HE) {
			/* Update 6G operating info */
			rlmUpdate6GOpInfo(prAdapter, prBssInfo);
		}
#endif
#endif
#endif
	}

	/* Update own operating RxNss */
	if (prBssInfo->fgIsOpChangeRxNss) {
		prBssInfo->ucOpRxNss = prBssInfo->ucOpChangeRxNss;
		DBGLOG(RLM, INFO, "Update OP RxNss[%d]\n",
			prBssInfo->ucOpRxNss);
	}

	/* Update own operating TxNss */
	if (prBssInfo->fgIsOpChangeTxNss) {
		prBssInfo->ucOpTxNss = prBssInfo->ucOpChangeTxNss;
		DBGLOG(RLM, INFO, "Update OP TxNss[%d]\n",
			prBssInfo->ucOpTxNss);
	}
}

void rlmSyncAntCtrl(struct ADAPTER *prAdapter, uint8_t txNss, uint8_t rxNss)
{
#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
	uint8_t cmd[30] = {0};
	uint8_t strLen = 0;
	uint32_t strOutLen = 0;

	strLen = kalSnprintf(cmd, sizeof(cmd),
			"AntControlConfig %d %d %d",
			prAdapter->ucANTCtrlReason,
			txNss, rxNss);
	DBGLOG(RLM, INFO, "Notify FW %s, strlen=%d", cmd, strLen);

	rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
	rChipConfigInfo.u2MsgSize = strLen;
	kalStrnCpy(rChipConfigInfo.aucCmd, cmd, strLen);
	wlanSetChipConfig(prAdapter, &rChipConfigInfo,
			sizeof(rChipConfigInfo), &strOutLen, FALSE);

	/* clean up */
	prAdapter->fgANTCtrl = false;
	prAdapter->ucANTCtrlPendingCount = 0;
#endif
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
static void rlmCompleteOpModeChange(struct ADAPTER *prAdapter,
				    struct BSS_INFO *prBssInfo,
				    u_int8_t fgIsSuccess)
{
	PFN_OPMODE_NOTIFY_DONE_FUNC pfnCallback;
	u_int8_t fgSkipRlmSync = FALSE;

	ASSERT((prAdapter != NULL) && (prBssInfo != NULL));

	if ((prBssInfo->fgIsOpChangeChannelWidth) ||
		(prBssInfo->fgIsOpChangeRxNss) ||
		(prBssInfo->fgIsOpChangeTxNss)) {

		if (IS_BSS_P2P(prBssInfo) && prBssInfo->fgIsSwitchingChnl) {
			DBGLOG(RLM, INFO,
				"Ignore rlm update when switch p2p channel\n");
			fgSkipRlmSync = TRUE;
		}

		if (IS_AIS_ROAMING(prAdapter, prBssInfo->ucBssIndex) ||
		    IS_AIS_OFF_CHNL(prAdapter, prBssInfo->ucBssIndex)) {
			DBGLOG(RLM, INFO,
				"Ignore rlm update when roaming/offchnl\n");
			fgSkipRlmSync = TRUE;
		}

		/* <1> Update own OP BW/Nss */
		rlmChangeOwnOpInfo(prAdapter, prBssInfo);

		/* <2> Update OP BW/Nss to FW */
		if (!fgSkipRlmSync)
			rlmSyncOperationParams(prAdapter, prBssInfo);

#if CFG_ENABLE_WIFI_DIRECT
		/* <3> Update BCN/Probe Resp IE to notify peers our OP info is
		 * changed (AP mode)
		 */
		if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT &&
			!fgSkipRlmSync)
			bssUpdateBeaconContent(prAdapter,
				prBssInfo->ucBssIndex);
#endif
		/* <4) Reset flags */
		prBssInfo->fgIsOpChangeChannelWidth = FALSE;
		prBssInfo->fgIsOpChangeRxNss = FALSE;
		prBssInfo->fgIsOpChangeTxNss = FALSE;
	}

	DBGLOG(RLM, INFO,
		"Complete BSS[%d] OP Mode change to BW[%d, %s] RxNss[%d] TxNss[%d]",
		prBssInfo->ucBssIndex,
		rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo),
		apucOpBw[rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo)],
		prBssInfo->ucOpRxNss,
		prBssInfo->ucOpTxNss);

	/* <4> Tell OpMode change caller the change result
	 * Allow callback function re-trigger OpModeChange immediately.
	 */
	pfnCallback = prBssInfo->pfOpChangeHandler;
	prBssInfo->pfOpChangeHandler = NULL;
	if (pfnCallback)
		pfnCallback(prAdapter, prBssInfo->ucBssIndex,
					     fgIsSuccess);
}

static enum ENUM_OP_CHANGE_STATUS_T
rlmChangeOperationModeApGo(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t ucChannelWidth,
	uint8_t ucOpRxNss,
	uint8_t ucOpTxNss,
	u_int8_t fgIsChangeBw,
	u_int8_t fgIsChangeRxNss,
	u_int8_t fgIsChangeTxNss)
{
	struct LINK *prClientList;
#if (CFG_SUPPORT_WIFI_6G == 1) && \
	((CFG_SUPPORT_802_11AX == 1) || (CFG_SUPPORT_802_11BE == 1))
	struct STA_RECORD *prCurrStaRec;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
#endif

	prClientList = &prBssInfo->rStaRecOfClientList;
	if (prClientList->u4NumElem == 0)
		goto op_mode_done;
	else if (!fgIsChangeBw && !fgIsChangeRxNss)
		goto op_mode_done;
#if (CFG_SUPPORT_WIFI_6G == 1) && \
	((CFG_SUPPORT_802_11AX == 1) || (CFG_SUPPORT_802_11BE == 1))
	else if (prBssInfo->eBand != BAND_6G)
		goto op_mode_done;
	else if (!RLM_NET_IS_11AX(prBssInfo)
#if (CFG_SUPPORT_802_11BE == 1)
		&& !RLM_NET_IS_11BE(prBssInfo)
#endif /* CFG_SUPPORT_802_11BE  */
	)
		goto op_mode_done;
	else if (!(prAdapter->rWifiVar.ucDbdcOMFrame & ENABLE_OMI))
		goto op_mode_done;

	/* reset status */
	prBssInfo->ucOmiWaitingCount = 0;

	LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList,
			    rLinkEntry,
			    struct STA_RECORD) {
		if (!prCurrStaRec)
			break;

		DBGLOG(RLM, INFO,
			"bss[%u] sta[%u] type=0x%x om_ctrl=%d\n",
			prBssInfo->ucBssIndex,
			prCurrStaRec->ucIndex,
			prCurrStaRec->ucDesiredPhyTypeSet,
			HE_IS_MAC_CAP_OM_CTRL(
				prCurrStaRec->ucHeMacCapInfo));

		if ((prCurrStaRec->ucDesiredPhyTypeSet &
			PHY_TYPE_SET_802_11AX) == 0 ||
#if (CFG_SUPPORT_802_11BE == 1)
		    (prCurrStaRec->ucDesiredPhyTypeSet &
			PHY_TYPE_SET_802_11BE) == 0 ||
#endif /* CFG_SUPPORT_802_11BE  */
		    !HE_IS_MAC_CAP_OM_CTRL(
			prCurrStaRec->ucHeMacCapInfo))
			continue;

		u4Status = rlmSendOMIDataFrame(prAdapter,
					       prCurrStaRec,
					       ucChannelWidth,
					       ucOpRxNss,
					       ucOpTxNss,
					       rlmNotifyApGoOmiOpModeTxDone);
		if (u4Status == WLAN_STATUS_SUCCESS)
			prBssInfo->ucOmiWaitingCount++;
	}

	DBGLOG(RLM, INFO,
		"ucOmiWaitingCount=%u\n",
		prBssInfo->ucOmiWaitingCount);

	if (prBssInfo->ucOmiWaitingCount > 0)
		return OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_WAIT;
#endif

op_mode_done:
	/* Complete OP Info change after notifying client by beacon */
	rlmCompleteOpModeChange(prAdapter, prBssInfo, TRUE);
	return OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_DONE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Change OpMode Nss/Channel Width
 *
 * \param[in] ucChannelWidth 0:20MHz, 1:40MHz, 2:80MHz, 3:160MHz 4:80+80MHz
 *
 * \return fgIsChangeOpMode
 *	TRUE: Can change/Don't need to change operation mode
 *	FALSE: Can't change operation mode
 */
/*----------------------------------------------------------------------------*/
enum ENUM_OP_CHANGE_STATUS_T
rlmChangeOperationMode(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex,
	uint8_t ucChannelWidth, uint8_t ucOpRxNss, uint8_t ucOpTxNss,
	enum ENUM_OP_CHANGE_SEND_ACT_T ucSendAct,
	PFN_OPMODE_NOTIFY_DONE_FUNC pfOpChangeHandler
	)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *)NULL;
	u_int8_t fgIsChangeBw = TRUE,
		 fgIsChangeRxNss = TRUE, /* Indicate if need to change */
		 fgIsChangeTxNss = TRUE;
	uint8_t i;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	/* Sanity check */
	if (ucBssIndex >= prAdapter->ucSwBssIdNum)
		return OP_CHANGE_STATUS_INVALID;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!prBssInfo)
		return OP_CHANGE_STATUS_INVALID;

	/* <1>Check if OP change parameter is valid */
	if (rlmCheckOpChangeParamValid(prAdapter, prBssInfo, ucChannelWidth,
				       ucOpRxNss, ucOpTxNss) == FALSE)
		return OP_CHANGE_STATUS_INVALID;

	/* <2>Check if OpMode notification is ongoing, if not, register the call
	 * back function
	 */
	if (prBssInfo->pfOpChangeHandler) {
		DBGLOG(RLM, INFO,
		       "BSS[%d] OpMode change notification is ongoing\n",
		       ucBssIndex);
		return OP_CHANGE_STATUS_INVALID;
	}

	prBssInfo->pfOpChangeHandler = pfOpChangeHandler;

	/* <3>Check if the current operating BW/Nss is the same as the target
	 * one
	 */
	if (ucSendAct == OP_CHANGE_SEND_ACT_DEFAULT &&
		ucChannelWidth == rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo)) {
		fgIsChangeBw = FALSE;
		prBssInfo->fgIsOpChangeChannelWidth = FALSE;
	}
	if (ucSendAct == OP_CHANGE_SEND_ACT_DEFAULT &&
		ucOpRxNss == prBssInfo->ucOpRxNss) {
		fgIsChangeRxNss = FALSE;
		prBssInfo->fgIsOpChangeRxNss = FALSE;
	}
	if (ucSendAct == OP_CHANGE_SEND_ACT_DEFAULT &&
		ucOpTxNss == prBssInfo->ucOpTxNss) {
		fgIsChangeTxNss = FALSE;
		prBssInfo->fgIsOpChangeTxNss = FALSE;
	}
	if ((!fgIsChangeBw) && (!fgIsChangeRxNss) && (!fgIsChangeTxNss)) {
		DBGLOG(RLM, INFO,
			"BSS[%d] target OpMode BW[%d] RxNss[%d] TxNss[%d] No change, return\n",
			ucBssIndex, ucChannelWidth, ucOpRxNss, ucOpTxNss);
		rlmCompleteOpModeChange(prAdapter, prBssInfo, TRUE);
		return OP_CHANGE_STATUS_VALID_NO_CHANGE;
	}

#if CFG_SUPPORT_DBDC
	/* Indicate operation mode changes */
	kalIndicateOpModeChange(prAdapter, ucBssIndex,
		ucChannelWidth, ucOpTxNss, ucOpRxNss);
#endif

	DBGLOG(RLM, INFO,
		"Intend to change BSS[%d] OP Mode to BW[%d,%s] RxNss[%d] TxNss[%d]\n",
		ucBssIndex, ucChannelWidth, apucOpBw[ucChannelWidth],
		ucOpRxNss, ucOpTxNss);

	/* <4> Fill OP Change Info into BssInfo*/

	/* When we resent OP Notification frame, we will use these params
	 * prBssInfo->ucOpChangeChannelWidth, prBssInfo->ucOpChangeRxNss,
	 * prBssInfo->ucOpChangeTxNss to resend, even if we don't need to
	 *  change BW or TRXNSS, we still need to assign value to those
	 * variables
	 */
	prBssInfo->ucOpChangeChannelWidth = ucChannelWidth;
	prBssInfo->ucOpChangeRxNss = ucOpRxNss;
	prBssInfo->ucOpChangeTxNss = ucOpTxNss;
	if (fgIsChangeBw) {
		prBssInfo->fgIsOpChangeChannelWidth = TRUE;
		DBGLOG(RLM, INFO, "Intend to change BSS[%d] to BW[%d]\n",
		       ucBssIndex, ucChannelWidth);
	}
	if (fgIsChangeRxNss) {
		prBssInfo->fgIsOpChangeRxNss = TRUE;
		DBGLOG(RLM, INFO, "Intend to change BSS[%d] to RxNss[%d]\n",
		       ucBssIndex, ucOpRxNss);
	}
	if (fgIsChangeTxNss) {
		prBssInfo->fgIsOpChangeTxNss = TRUE;
		DBGLOG(RLM, INFO, "Intend to change BSS[%d] to TxNss[%d]\n",
			ucBssIndex, ucOpTxNss);
	}

	/* <5>Handling OP Info change for STA/GC */
	if ((prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) &&
	    (prBssInfo->prStaRecOfAP)) {
		prStaRec = prBssInfo->prStaRecOfAP;
		/* <5.1>Initialize OP mode change parameters related to
		 * notification Tx done handler (STA mode)
		 */
		if (prBssInfo->pfOpChangeHandler) {
			for (i = 0; i < OP_NOTIFY_TYPE_NUM; i++) {
				prBssInfo->aucOpModeChangeState[i] =
					OP_NOTIFY_STATE_KEEP;
				prBssInfo->aucOpModeChangeRetryCnt[i] = 0;
			}
		}

		if (ucSendAct == OP_CHANGE_SEND_ACT_DISABLE) {
			/* no need to send action frame, just done */
			rlmCompleteOpModeChange(prAdapter, prBssInfo, TRUE);
			return OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_DONE;
		}

		if (fgIsChangeRxNss)
			rlmResetMrc(prAdapter, ucBssIndex);

#if (CFG_SUPPORT_802_11AX == 1)
		if (((RLM_NET_IS_11AX(prBssInfo) &&
			(prStaRec->ucDesiredPhyTypeSet &
			PHY_TYPE_SET_802_11AX))
#if (CFG_SUPPORT_802_11BE == 1)
			|| (RLM_NET_IS_11BE(prBssInfo) &&
			(prStaRec->ucDesiredPhyTypeSet &
			PHY_TYPE_SET_802_11BE))
#endif /* CFG_SUPPORT_802_11BE  */
		) && HE_IS_MAC_CAP_OM_CTRL(prStaRec->ucHeMacCapInfo)
		&& (prAdapter->rWifiVar.ucDbdcOMFrame & ENABLE_OMI)
		&& (fgIsChangeBw || fgIsChangeRxNss)) {
			DBGLOG(RLM, INFO,
				"Send OMI frame: BSS[%d] BW[%d, %s] RxNss[%d]\n",
				ucBssIndex, ucChannelWidth,
				apucOpBw[ucChannelWidth], ucOpRxNss);

			u4Status = rlmSendOMIDataFrame(prAdapter,
				prStaRec, ucChannelWidth,
				ucOpRxNss, ucOpTxNss,
				rlmNotifyOMIOpModeTxDone);

		}
#endif /* CFG_SUPPORT_802_11AX */
#if CFG_SUPPORT_802_11AC
		if (((RLM_NET_IS_11AC(prBssInfo) &&
			(prStaRec->ucDesiredPhyTypeSet &
			PHY_TYPE_SET_802_11AC))
			&& (prAdapter->rWifiVar.ucDbdcOMFrame & ENABLE_OMN))
			&& (fgIsChangeBw || fgIsChangeRxNss)) {
			if (prBssInfo->pfOpChangeHandler)
				prBssInfo->aucOpModeChangeState
					[OP_NOTIFY_TYPE_VHT_NSS_BW] =
					OP_NOTIFY_STATE_SENDING;
			DBGLOG(RLM, INFO,
				"Send VHT OP notification frame: BSS[%d] BW[%d, %s] RxNss[%d]\n",
				ucBssIndex, ucChannelWidth,
				apucOpBw[ucChannelWidth], ucOpRxNss);
			u4Status = rlmSendOpModeNotificationFrame(
				prAdapter, prStaRec,
				ucChannelWidth, ucOpRxNss);
		}
#endif
		if (RLM_NET_IS_11N(prBssInfo)
			&& (fgIsChangeBw || fgIsChangeRxNss)) {
			if (prBssInfo->pfOpChangeHandler) {
				if (fgIsChangeRxNss)
					prBssInfo->aucOpModeChangeState
						[OP_NOTIFY_TYPE_HT_NSS] =
						OP_NOTIFY_STATE_SENDING;
				if (fgIsChangeBw)
					prBssInfo->aucOpModeChangeState
						[OP_NOTIFY_TYPE_HT_BW] =
						OP_NOTIFY_STATE_SENDING;
			}
			if (fgIsChangeRxNss) {
				u4Status = rlmSendSmPowerSaveFrame(
					prAdapter, prStaRec, ucOpRxNss);
				DBGLOG(RLM, INFO,
					"Send HT SM Power Save frame: ");
				DBGLOG(RLM, INFO, "BSS[%d] RxNss[%d]\n",
					ucBssIndex, ucOpRxNss);
			}
			if (fgIsChangeBw) {
				u4Status = rlmSendNotifyChannelWidthFrame(
					prAdapter, prStaRec, ucChannelWidth);
				DBGLOG(RLM, INFO,
					"Send HT Notify Channel Width frame: ");
				DBGLOG(RLM, INFO, "BSS[%d] BW[%d, %s]\n",
					ucBssIndex, ucChannelWidth,
					apucOpBw[ucChannelWidth]);
			}
		}

		/* Error handling */
		if (u4Status != WLAN_STATUS_SUCCESS) {
			rlmCompleteOpModeChange(prAdapter, prBssInfo, FALSE);
			return OP_CHANGE_STATUS_INVALID;
		}

		/* <5.3> Change OP Info w/o waiting for notification Tx done */
		if (prBssInfo->pfOpChangeHandler == NULL ||
			(!fgIsChangeBw && !fgIsChangeRxNss)) {
			rlmCompleteOpModeChange(prAdapter, prBssInfo, TRUE);
			/* No callback */
			return OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_DONE;
		}

		return OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_WAIT;
	}
	/* <6>Handling OP Info change for AP/GO */
	else if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
		return rlmChangeOperationModeApGo(prAdapter, prBssInfo,
						  ucChannelWidth, ucOpRxNss,
						  ucOpTxNss, fgIsChangeBw,
						  fgIsChangeRxNss,
						  fgIsChangeTxNss);
	}

	/* Complete OP mode change if no sending action frames */
	rlmCompleteOpModeChange(prAdapter, prBssInfo, TRUE);
	return OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_DONE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will update the contain of BSS_INFO_T and STA_RECORD_T
 *        for AIS network once reciving new beacon after CSA.
 *
 * @param[in] prBssInfo              Pointer to AIS BSS_INFO_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void rlmUpdateParamsForCSA(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo)
{
	struct STA_RECORD *prStaRec;
	struct BSS_DESC *prBssDesc;

	prStaRec = prBssInfo->prStaRecOfAP;
	prBssDesc = scanSearchBssDescByBssid(prAdapter, prStaRec->aucMacAddr);

	if (!prBssDesc) {
		DBGLOG(AIS, ERROR,
			"Can't find " MACSTR "\n",
			MAC2STR(prStaRec->aucMacAddr));
		return;
	}

	/* <1> Update information from BSS_DESC to current P_STA_RECORD */
	bssUpdateStaRecFromBssDesc(prAdapter, prBssDesc, prStaRec);

	/* <2> Setup PHY Attributes and Basic Rate Set/Operational
	 * Rate Set
	 */
	prBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;
	prBssInfo->ucNonHTBasicPhyType = prStaRec->ucNonHTBasicPhyType;
	prBssInfo->u2OperationalRateSet = prStaRec->u2OperationalRateSet;
	prBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

	nicTxUpdateBssDefaultRate(prBssInfo);
	nicTxUpdateStaRecDefaultRate(prAdapter, prStaRec);
	cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);

	cnmDumpStaRec(prAdapter, prStaRec->ucIndex);
}				/* end of aisUpdateParamsForCSA() */

#if (CFG_SUPPORT_802_11AX == 1)
static uint8_t rlmGetPeerNssbyHeMcsMap(uint8_t ucVhtChannelWidth,
	struct STA_RECORD *prStaRec)
{
	uint8_t ucNss = 1;

	if (ucVhtChannelWidth == VHT_OP_CHANNEL_WIDTH_160) {
		if (((prStaRec->u2HeRxMcsMapBW160 &
			HE_CAP_INFO_MCS_1SS_MASK) >>
			HE_CAP_INFO_MCS_1SS_OFFSET) !=
			HE_CAP_INFO_MCS_NOT_SUPPORTED) {
			ucNss = 1;
		}
		if (((prStaRec->u2HeRxMcsMapBW160 &
			HE_CAP_INFO_MCS_2SS_MASK) >>
			HE_CAP_INFO_MCS_2SS_OFFSET) !=
			HE_CAP_INFO_MCS_NOT_SUPPORTED) {
			ucNss = 2;
		}
		if (((prStaRec->u2HeRxMcsMapBW160 &
			HE_CAP_INFO_MCS_3SS_MASK) >>
			HE_CAP_INFO_MCS_3SS_OFFSET) !=
			HE_CAP_INFO_MCS_NOT_SUPPORTED) {
			ucNss = 3;
		}
		if (((prStaRec->u2HeRxMcsMapBW160 &
			HE_CAP_INFO_MCS_4SS_MASK) >>
			HE_CAP_INFO_MCS_4SS_OFFSET) !=
			HE_CAP_INFO_MCS_NOT_SUPPORTED) {
			ucNss = 4;
		}
	} else { /* <= VHT_OP_CHANNEL_WIDTH_80 */
		if (((prStaRec->u2HeRxMcsMapBW80 &
			HE_CAP_INFO_MCS_1SS_MASK) >>
			HE_CAP_INFO_MCS_1SS_OFFSET) !=
			HE_CAP_INFO_MCS_NOT_SUPPORTED) {
			ucNss = 1;
		}
		if (((prStaRec->u2HeRxMcsMapBW80 &
			HE_CAP_INFO_MCS_2SS_MASK) >>
			HE_CAP_INFO_MCS_2SS_OFFSET) !=
			HE_CAP_INFO_MCS_NOT_SUPPORTED) {
			ucNss = 2;
		}
		if (((prStaRec->u2HeRxMcsMapBW80 &
			HE_CAP_INFO_MCS_3SS_MASK) >>
			HE_CAP_INFO_MCS_3SS_OFFSET) !=
			HE_CAP_INFO_MCS_NOT_SUPPORTED) {
			ucNss = 3;
		}
		if (((prStaRec->u2HeRxMcsMapBW80 &
			HE_CAP_INFO_MCS_4SS_MASK) >>
			HE_CAP_INFO_MCS_4SS_OFFSET) !=
			HE_CAP_INFO_MCS_NOT_SUPPORTED) {
			ucNss = 4;
		}
	}
	return ucNss;
}
#endif

void rlmChangeOperationModeAfterCSA(
	struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo)
{
	uint8_t ucVhtChannelWidthAfterCsa;
	enum ENUM_CHNL_EXT eBssScoAfterCsa;
	uint8_t ucOpRxNssAfterCsa;
	uint8_t ucOpTxNssAfterCsa;
	uint8_t ucGetOpRxNss = 0;
	uint8_t ucGetOpTxNss = 0;

	if (!prBssInfo)
		return;

	/* Keep new info and swap into old info */
	ucVhtChannelWidthAfterCsa = prBssInfo->ucVhtChannelWidth;
	eBssScoAfterCsa = prBssInfo->eBssSCO;
	ucOpRxNssAfterCsa = prBssInfo->ucOpRxNss;
	ucOpTxNssAfterCsa = prBssInfo->ucOpTxNss;

	prBssInfo->ucVhtChannelWidth = prBssInfo->ucVhtChannelWidthBeforeCsa;
	prBssInfo->eBssSCO = prBssInfo->eBssScoBeforeCsa;
	prBssInfo->ucOpRxNss = prBssInfo->ucOpRxNssBeforeCsa;
	prBssInfo->ucOpTxNss = prBssInfo->ucOpTxNssBeforeCsa;
	/* change ucOpRxNssBeforeCsa by peer's cap */
#if (CFG_SUPPORT_802_11AX == 1)
	if (RLM_NET_IS_11AX(prBssInfo)) { /* HE */
		ucOpRxNssAfterCsa =
			rlmGetPeerNssbyHeMcsMap(ucVhtChannelWidthAfterCsa,
				prBssInfo->prStaRecOfAP);
		ucOpTxNssAfterCsa =
			rlmGetPeerNssbyHeMcsMap(ucVhtChannelWidthAfterCsa,
				prBssInfo->prStaRecOfAP);
	}
#endif

	cnmOpModeGetTRxNss(prAdapter,
		prBssInfo->ucBssIndex,
		&ucGetOpRxNss,
		&ucGetOpTxNss);

	if (ucGetOpRxNss < ucOpRxNssAfterCsa)
		ucOpRxNssAfterCsa = ucGetOpRxNss;
	if (ucGetOpTxNss < ucOpTxNssAfterCsa)
		ucOpTxNssAfterCsa = ucGetOpTxNss;

	DBGLOG(RLM, INFO,
		"op mode change from BW(vht)[%d]-RxNss[%d]-TxNss[%d] to BW(vht)[%d]-RxNss[%d]-TxNss[%d]",
		prBssInfo->ucVhtChannelWidth,
		prBssInfo->ucOpRxNss,
		prBssInfo->ucOpTxNss,
		ucVhtChannelWidthAfterCsa,
		ucOpRxNssAfterCsa,
		ucOpTxNssAfterCsa);
	rlmChangeOperationMode(
		prAdapter, prBssInfo->ucBssIndex,
		rlmGetBssOpBwByChannelWidth(eBssScoAfterCsa,
					    ucVhtChannelWidthAfterCsa),
		ucOpRxNssAfterCsa,
		ucOpTxNssAfterCsa,
		OP_CHANGE_SEND_ACT_DEFAULT,
		rlmDummyChangeOpHandler);

	/* Restore info after op mode change */
	prBssInfo->ucVhtChannelWidth = ucVhtChannelWidthAfterCsa;
	prBssInfo->eBssSCO = eBssScoAfterCsa;
	prBssInfo->ucOpRxNss = ucOpRxNssAfterCsa;
	prBssInfo->ucOpTxNss = ucOpTxNssAfterCsa;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check our desired BW/RxNss is less or equal than peer's capability to
 *        prevent IOT issue.
 *
 * \param[in] prBssInfo
 * \param[in] ucChannelWidth
 * \param[in] ucOpTxNss
 *
 * \return
 *	TRUE : Can change to desired BW/RxNss
 *	FALSE: Should not change operation mode
 */
/*----------------------------------------------------------------------------*/
static u_int8_t rlmCheckOpChangeParamForClient(struct BSS_INFO *prBssInfo,
					       uint8_t ucChannelWidth,
					       uint8_t ucOpRxNss)
{
	struct STA_RECORD *prStaRec;

	prStaRec = prBssInfo->prStaRecOfAP;

	if (!prStaRec)
		return FALSE;

#if CFG_SUPPORT_802_11AC
	if (RLM_NET_IS_11AC(prBssInfo)) { /* VHT */
		/* Check peer OP Channel Width */
		switch (ucChannelWidth) {
		case MAX_BW_80_80_MHZ:
			if (prStaRec->ucVhtOpChannelWidth !=
			    VHT_OP_CHANNEL_WIDTH_80P80) {
				DBGLOG(RLM, INFO,
				       "Can't change BSS[%d] OP BW to:%d for peer VHT OP BW is:%d\n",
				       prBssInfo->ucBssIndex, ucChannelWidth,
				       prStaRec->ucVhtOpChannelWidth);
				return FALSE;
			}
			break;
		case MAX_BW_160MHZ:
			if (prStaRec->ucVhtOpChannelWidth !=
			    VHT_OP_CHANNEL_WIDTH_160) {
				DBGLOG(RLM, INFO,
				       "Can't change BSS[%d] OP BW to:%d for peer VHT OP BW is:%d\n",
				       prBssInfo->ucBssIndex, ucChannelWidth,
				       prStaRec->ucVhtOpChannelWidth);
				return FALSE;
			}
			break;
		case MAX_BW_80MHZ:
			if (prStaRec->ucVhtOpChannelWidth <
			    VHT_OP_CHANNEL_WIDTH_80) {
				DBGLOG(RLM, INFO,
				       "Can't change BSS[%d] OP BW to:%d for peer VHT OP BW is:%d\n",
				       prBssInfo->ucBssIndex, ucChannelWidth,
				       prStaRec->ucVhtOpChannelWidth);
				return FALSE;
			}
			break;
		case MAX_BW_40MHZ:
			if (!(prStaRec->ucHtPeerOpInfo1 &
			      HT_OP_INFO1_STA_CHNL_WIDTH) ||
			    (!prBssInfo->fg40mBwAllowed)) {
				DBGLOG(RLM, INFO,
				       "Can't change BSS[%d] OP BW to:%d for PeerOpBw:%d fg40mBwAllowed:%d\n",
				       prBssInfo->ucBssIndex, ucChannelWidth,
				       (uint8_t)(prStaRec->ucHtPeerOpInfo1 &
						 HT_OP_INFO1_STA_CHNL_WIDTH),
				       prBssInfo->fg40mBwAllowed);
				return FALSE;
			}
			break;
		case MAX_BW_20MHZ:
			break;
		default:
			DBGLOG(RLM, WARN,
			       "BSS[%d] target OP BW:%d is invalid for VHT OpMode change\n",
			       prBssInfo->ucBssIndex, ucChannelWidth);
			return FALSE;
		}

		/* Check peer Rx Nss Cap */
		if (ucOpRxNss == 2 &&
		    ((prStaRec->u2VhtRxMcsMap & VHT_CAP_INFO_MCS_2SS_MASK) >>
		     VHT_CAP_INFO_MCS_2SS_OFFSET) ==
			    VHT_CAP_INFO_MCS_NOT_SUPPORTED) {
			DBGLOG(RLM, INFO,
			       "Don't change Nss since VHT peer doesn't support 2ss\n");
			return FALSE;
		}

	} else
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	if (RLM_NET_IS_11AX(prBssInfo)) { /* HE */
		/* Check peer OP Channel Width */
		switch (ucChannelWidth) {
		/* Check peer OP Channel Width */
#if (CFG_SUPPORT_802_11BE == 1)
		case MAX_BW_320_1MHZ:
		case MAX_BW_320_2MHZ:
			if (!RLM_NET_IS_11BE(prBssInfo)) { /* BE */
				DBGLOG(RLM, WARN,
					"BSS[%d] target OP BW:%d is invalid for EHT OpMode change\n",
				prBssInfo->ucBssIndex, ucChannelWidth);
				return FALSE;
			}
			if (!(*prStaRec->ucEhtPhyCapInfo
				& DOT11BE_PHY_CAP_320M_6G)) {
				DBGLOG(RLM, WARN,
					"Can't change BSS[%d] OP BW to:%d for peer EHT doesn't support BW320\n",
				prBssInfo->ucBssIndex, ucChannelWidth);
				return FALSE;
			}
			break;
#endif /* #if (CFG_SUPPORT_802_11BE == 1) */
		case MAX_BW_80_80_MHZ:
			if (!HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW80P80_5G(
				prStaRec->ucHePhyCapInfo)) {
				DBGLOG(RLM, INFO,
					"Can't change BSS[%d] OP BW to:%d for peer HE doesn't support BW80P80\n",
					prBssInfo->ucBssIndex, ucChannelWidth);
				return FALSE;
			}
			break;
		case MAX_BW_160MHZ:
			if (!HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW160_5G(
				prStaRec->ucHePhyCapInfo)) {
				DBGLOG(RLM, INFO,
					"Can't change BSS[%d] OP BW to:%d for peer HE doesn't support BW160\n",
					prBssInfo->ucBssIndex, ucChannelWidth);
				return FALSE;
			}
			break;
		case MAX_BW_80MHZ:
		case MAX_BW_40MHZ:
			if (!HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW40_BW80_5G(
				prStaRec->ucHePhyCapInfo) &&
				(!HE_IS_PHY_CAP_CHAN_WIDTH_SET_BW40_2G(
				prStaRec->ucHePhyCapInfo))) {
				DBGLOG(RLM, INFO,
					"Can't change BSS[%d] OP BW to:%d for peer HE doens't support 5G BW80/BW40 or 2G BW 40\n",
					prBssInfo->ucBssIndex, ucChannelWidth);
				return FALSE;
			}
			break;
		case MAX_BW_20MHZ:
			break;
		default:
			DBGLOG(RLM, WARN,
				"BSS[%d] target OP BW:%d is invalid for HE OpMode change\n",
					prBssInfo->ucBssIndex, ucChannelWidth);
			return FALSE;
		}

		/* Check peer Rx Nss Cap */
		if (ucOpRxNss == 2) {
			switch (ucChannelWidth) {
			case MAX_BW_80_80_MHZ:
				if (((prStaRec->u2HeRxMcsMapBW80P80 &
					HE_CAP_INFO_MCS_2SS_MASK) >>
					HE_CAP_INFO_MCS_2SS_OFFSET) ==
						HE_CAP_INFO_MCS_NOT_SUPPORTED) {
					DBGLOG(RLM, INFO,
						"Don't change Nss since HE peer doesn't support BW80P80 2ss\n");
					return FALSE;
				}
				break;
			case MAX_BW_160MHZ:
				if (((prStaRec->u2HeRxMcsMapBW160 &
					HE_CAP_INFO_MCS_2SS_MASK) >>
					HE_CAP_INFO_MCS_2SS_OFFSET) ==
						HE_CAP_INFO_MCS_NOT_SUPPORTED) {
					DBGLOG(RLM, INFO,
						"Don't change Nss since HE peer doesn't support BW160 2ss\n");
					return FALSE;
				}
				break;
			case MAX_BW_80MHZ:
			default:
				if (((prStaRec->u2HeRxMcsMapBW80 &
					HE_CAP_INFO_MCS_2SS_MASK) >>
					HE_CAP_INFO_MCS_2SS_OFFSET) ==
						HE_CAP_INFO_MCS_NOT_SUPPORTED) {
					DBGLOG(RLM, INFO,
						"Don't change Nss since HE peer doesn't support BW80 2ss\n");
					return FALSE;
				}
				break;
		    }
		}
	} else
#endif
	if (RLM_NET_IS_11N(prBssInfo)) { /* HT */
		/* Check peer Channel Width */
		if (ucChannelWidth >= MAX_BW_80MHZ) {
			DBGLOG(RLM, WARN,
				"BSS[%d] target OP BW:%d is invalid for HT OpMode change\n",
				prBssInfo->ucBssIndex, ucChannelWidth);
			return FALSE;
		} else if (ucChannelWidth == MAX_BW_40MHZ) {
			if (!(prStaRec->ucHtPeerOpInfo1 &
					HT_OP_INFO1_STA_CHNL_WIDTH) ||
				(!prBssInfo->fg40mBwAllowed)) {
				DBGLOG(RLM, INFO,
					"Can't change BSS[%d] OP BW to:%d for PeerOpBw:%d fg40mBwAllowed:%d\n",
					prBssInfo->ucBssIndex,
					ucChannelWidth,
					(uint8_t)(prStaRec->ucHtPeerOpInfo1 &
						HT_OP_INFO1_STA_CHNL_WIDTH),
					prBssInfo->fg40mBwAllowed);
				return FALSE;
			}
		}

		/* Check peer Rx Nss Cap */
		if (ucOpRxNss == 2 &&
			(prStaRec->aucRxMcsBitmask[1] == 0)) {
			DBGLOG(RLM, INFO,
				"Don't change Nss since HT peer doesn't support 2ss\n");
			return FALSE;
		}
	}

	return TRUE;
}

static u_int8_t rlmCheckOpChangeParamValid(struct ADAPTER *prAdapter,
					   struct BSS_INFO *prBssInfo,
					   uint8_t ucChannelWidth,
					   uint8_t ucOpRxNss,
					   uint8_t ucOpTxNss)
{
	uint8_t ucCapNss;

	ASSERT(prBssInfo);

	/* <1>Check if BSS PHY type is legacy mode */
	if (!(RLM_NET_IS_11N(prBssInfo)
#if CFG_SUPPORT_802_11AC
		|| RLM_NET_IS_11AC(prBssInfo)
#endif
#if (CFG_SUPPORT_802_11AX == 1)
		|| RLM_NET_IS_11AX(prBssInfo)
#endif
	)) {
		DBGLOG(RLM, WARN,
		       "Can't change BSS[%d] OP info for legacy BSS\n",
		       prBssInfo->ucBssIndex);
		return FALSE;
	}

	/* <2>Check network type */
	if ((prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE) &&
	    (prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT)) {
		DBGLOG(RLM, WARN,
		       "Can't change BSS[%d] OP info for OpMode:%d\n",
		       prBssInfo->ucBssIndex, prBssInfo->eCurrentOPMode);
		return FALSE;
	}

	/* <3>Check if target OP BW/Nss <= Own Cap BW/Nss */
	ucCapNss = wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);
	if (ucOpRxNss > ucCapNss || ucOpRxNss == 0 ||
		ucOpTxNss > ucCapNss || ucOpTxNss == 0) {
		DBGLOG(RLM, WARN,
		       "Can't change BSS[%d] OP RxNss[%d]TxNss[%d] due to CapNss[%d]\n",
		       prBssInfo->ucBssIndex, ucOpRxNss, ucOpTxNss, ucCapNss);
		return FALSE;
	}

	if (ucChannelWidth > cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex)) {
		DBGLOG(RLM, WARN,
		       "Can't change BSS[%d] OP BW[%d] due to CapBW[%d]\n",
		       prBssInfo->ucBssIndex, ucChannelWidth,
		       cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex));
		return FALSE;
	}

	/* <4>Check if target OP BW is valid for band and primary channel of
	 * current BSS
	 */
	if (prBssInfo->eBand == BAND_2G4) {
		if ((ucChannelWidth != MAX_BW_20MHZ) &&
		    (ucChannelWidth != MAX_BW_40MHZ)) {
			DBGLOG(RLM, WARN,
			       "Can't change BSS[%d] OP BW to:%d for 2.4G\n",
			       prBssInfo->ucBssIndex, ucChannelWidth);
			return FALSE;
		}
	} else if (prBssInfo->eBand == BAND_5G) {
		/* It can only use BW20 for CH165 */
		if ((ucChannelWidth != MAX_BW_20MHZ) &&
			(prBssInfo->ucPrimaryChannel == 165)) {
			DBGLOG(RLM, WARN,
			       "Can't change BSS[%d] OP BW to:%d for CH165\n",
			       prBssInfo->ucBssIndex, ucChannelWidth);
			return FALSE;
		}

		if ((ucChannelWidth == MAX_BW_160MHZ) &&
		    ((prBssInfo->ucPrimaryChannel < 36) ||
		     ((prBssInfo->ucPrimaryChannel > 64) &&
		      (prBssInfo->ucPrimaryChannel < 100)) ||
		     (prBssInfo->ucPrimaryChannel > 128))) {
			DBGLOG(RLM, WARN,
			       "Can't change BSS[%d] to OP BW160 for primary CH%d\n",
			       prBssInfo->ucBssIndex,
			       prBssInfo->ucPrimaryChannel);
			return FALSE;
		}
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prBssInfo->eBand == BAND_6G) {
		if ((ucChannelWidth == MAX_BW_160MHZ) &&
			((prBssInfo->ucPrimaryChannel < 1) ||
			 (prBssInfo->ucPrimaryChannel > 221))) {
			DBGLOG(RLM, WARN,
				"Can't change BSS[%d] to OP BW160 for primary CH%d\n",
				prBssInfo->ucBssIndex,
				prBssInfo->ucPrimaryChannel);
			return FALSE;
		}
	}
#endif
	else {
		DBGLOG(RLM, WARN,
			"Unknown band %d for BSS[%d] with primary CH%d\n",
			prBssInfo->eBand,
			prBssInfo->ucBssIndex,
			prBssInfo->ucPrimaryChannel);
	}

	/* <5>Check if target OP BW/Nss <= peer's BW/Nss (STA mode) */
	if (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
		if (rlmCheckOpChangeParamForClient(prBssInfo, ucChannelWidth,
						   ucOpRxNss) == FALSE)
			return FALSE;
	}

	return TRUE;
}

void rlmDummyChangeOpHandler(struct ADAPTER *prAdapter, uint8_t ucBssIndex,
			     bool fgIsChangeSuccess)
{
	DBGLOG(RLM, INFO, "OP change done for BSS[%d] IsSuccess[%d]\n",
	       ucBssIndex, fgIsChangeSuccess);
}
#if CFG_SUPPORT_802_11K
static uint32_t rlmRegTxPwrLimitGet(struct ADAPTER *prAdapter,
					uint8_t ucBssIdx,
					int8_t *picPwrLmt)
{
	struct BSS_DESC *prBssDesc = NULL;
	int8_t icMaxPwrLmt = 0, icMinPwrLmt = 0;

	if (!prAdapter || (ucBssIdx >= MAX_BSSID_NUM))
		return WLAN_STATUS_INVALID_DATA;

	if (prAdapter->rWifiVar.icRegPwrLmtMax > TX_PWR_MAX)
		icMaxPwrLmt = TX_PWR_MAX;
	else
		icMaxPwrLmt = prAdapter->rWifiVar.icRegPwrLmtMax;

	if (prAdapter->rWifiVar.icRegPwrLmtMin < TX_PWR_MIN)
		icMinPwrLmt = TX_PWR_MIN;
	else
		icMinPwrLmt = prAdapter->rWifiVar.icRegPwrLmtMin;

	prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIdx);

	if (!prBssDesc)
		return WLAN_STATUS_INVALID_DATA;

	*picPwrLmt = prBssDesc->cPowerLimit;

	if (*picPwrLmt > icMaxPwrLmt)
		*picPwrLmt = icMaxPwrLmt;

	if (*picPwrLmt < icMinPwrLmt)
		*picPwrLmt = icMinPwrLmt;

	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to update Regulatory TxPower limit if need.
 *
 * \param[in] prAdapter : Pointer of adapter
 * \param[in] prBssDesc : Pointer ofBSS desription
 * \param[in] eHwBand : RF Band
 * \param[in] prCountryIE : Pointer of Country IE
 * \param[in] ucPowerConstraint : TxPower constraint value from Power
 *                                Constrait IE
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmRegTxPwrLimitUpdate(
	struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc,
	enum ENUM_BAND eHwBand,
	struct IE_COUNTRY *prCountryIE,
	uint8_t ucPowerConstraint
)
{
	uint8_t ucRemainLen = 0;
	uint8_t ucChnlOfst = 0;
	const uint8_t ucSubBandSize =
		(uint8_t)sizeof(struct COUNTRY_INFO_SUBBAND_TRIPLET);
	struct COUNTRY_INFO_SUBBAND_TRIPLET *prSubBand = NULL;
	int8_t icNewPwrLimit;
	uint8_t ucStartCh = 0;
	uint8_t ucEndCh = 0;
	uint8_t ucCurrChnl = 0;
	uint32_t u4SwTestMode = 0;

	/* Sanity check for null pointer & IE content */
	if (!prAdapter || !prBssDesc || !prCountryIE)
		return WLAN_STATUS_INVALID_DATA;

#if (CFG_SUPPORT_WIFI_6G == 1)
	/* Regulatory TxPower Limit only support on 2.4G/5G */
	if (eHwBand == BAND_6G)
		return WLAN_STATUS_NOT_SUPPORTED;
#endif

	ucRemainLen = prCountryIE->ucLength - 3;
	prSubBand = &prCountryIE->arCountryStr[0];
	ucChnlOfst = (eHwBand == BAND_2G4) ? CHNL_SPAN_5 : CHNL_SPAN_20;
	ucCurrChnl = prBssDesc->ucChannelNum;

	while (ucRemainLen >= ucSubBandSize) {
		ucStartCh = prSubBand->ucFirstChnlNum;
		ucEndCh = ucStartCh + (prSubBand->ucNumOfChnl - 1) * ucChnlOfst;
		DBGLOG(RLM, LOUD,
			"Country IE B[%d]ofst[%d]PriCh[%d]StartCh[%d]ChNum[%d]EndCh[%d]Lmt[%d]\n",
			eHwBand,
			ucChnlOfst,
			ucCurrChnl,
			ucStartCh,
			prSubBand->ucNumOfChnl,
			ucEndCh,
			prSubBand->cMaxTxPwrLv);

		if (ucStartCh < 201 &&
			ucCurrChnl >= ucStartCh &&
			ucCurrChnl <= ucEndCh) {
			/* Found */
			break;
		}
		ucRemainLen -= ucSubBandSize;
		prSubBand++;
	}

	/* Found a right country band */
	if (ucRemainLen >= ucSubBandSize) {
		u4SwTestMode = prAdapter->rWifiVar.u4SwTestMode;

		if ((u4SwTestMode == ENUM_SW_TEST_MODE_SIGMA_VOICE_ENT) ||
			(rlmDomainIsDfsChnls(prAdapter, ucCurrChnl))) {
			/* There is two case will consider power constrant
			 * 1. DFS channel
			 * 2. VOE test, due to the testcase is define
			 *    not DFS channel.
			 */
			icNewPwrLimit =
				prSubBand->cMaxTxPwrLv - ucPowerConstraint;
		} else {
			icNewPwrLimit = prSubBand->cMaxTxPwrLv;
		}

		/* Tx power changed Limit */
		if (prBssDesc->cPowerLimit != icNewPwrLimit) {

			DBGLOG(RLM, TRACE,
			"Update Regulatory PwrLmt(%c%c)SSID:%s BSSID["MACSTR
			"]Old PwrLmt[%d]New PwrLmt[%d]Constrant[%d]DFS[%d]\n",
			((prBssDesc->u2CurrCountryCode & 0xff00) >> 8),
			(prBssDesc->u2CurrCountryCode & 0x00ff),
			prBssDesc->aucSSID,
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->cPowerLimit,
			icNewPwrLimit,
			ucPowerConstraint,
			rlmDomainIsDfsChnls(prAdapter, ucCurrChnl));

			prBssDesc->cPowerLimit = icNewPwrLimit;

			/* Update TxPower limit to FW if BSS is connected */
			if (prBssDesc->fgIsConnected) {
				rlmSetMaxTxPwrLimit(prAdapter,
				prBssDesc->cPowerLimit, 1);
			}
		}
	} else if (prBssDesc->cPowerLimit != RLM_INVALID_POWER_LIMIT) {
		DBGLOG(RLM, LOUD,
			"The channel[%d] of BSSID["MACSTR
			"] SSID:%sdoesn't match with country IE, Pwrlmt[%d]\n",
			ucCurrChnl,
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->aucSSID,
			prBssDesc->cPowerLimit);

		prBssDesc->cPowerLimit = RLM_INVALID_POWER_LIMIT;
		/* Disable TxPower limit */
		rlmSetMaxTxPwrLimit(prAdapter, 0, 0);
	}
	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_802_11K */
void rlmSetMaxTxPwrLimit(struct ADAPTER *prAdapter,
			int8_t icLimit,
			 uint8_t ucEnable)
{
	struct CMD_SET_AP_CONSTRAINT_PWR_LIMIT rTxPwrLimit;
	int8_t icMinPwrLmt = 0, icMaxPwrLmt = 0;

	if (!prAdapter)
		return;

	kalMemZero(&rTxPwrLimit, sizeof(rTxPwrLimit));
	rTxPwrLimit.ucCmdVer =  0x1;
	rTxPwrLimit.ucPwrSetEnable =  ucEnable;

	/* Sanity check min power limit */
	if (prAdapter->rWifiVar.icRegPwrLmtMin < TX_PWR_MIN)
		icMinPwrLmt = TX_PWR_MIN;
	else
		icMinPwrLmt = prAdapter->rWifiVar.icRegPwrLmtMin;

	/* Sanity check max power limit */
	if (prAdapter->rWifiVar.icRegPwrLmtMax > TX_PWR_MAX)
		icMaxPwrLmt = TX_PWR_MAX;
	else
		icMaxPwrLmt = prAdapter->rWifiVar.icRegPwrLmtMax;

	if (ucEnable) {
		if (icLimit > icMaxPwrLmt) {
			DBGLOG(RLM, INFO,
				"LM: Target MaxPwr [%d] too big, use default[%d]\n"
				, icLimit,
				icMaxPwrLmt);
			icLimit = icMaxPwrLmt;
		}
		if (icLimit < icMinPwrLmt) {
			DBGLOG(RLM, INFO,
				"LM: Target MinPwr [%d] too low, use default[%d]\n"
				, icLimit
				, icMinPwrLmt);
			icLimit = icMinPwrLmt;
		}

		/* Convert to unit 0.5dBm */
		rTxPwrLimit.cMaxTxPwr = icLimit * 2;
		rTxPwrLimit.cMinTxPwr = icMinPwrLmt * 2;

		DBGLOG(RLM, INFO,
			"LM: Set Max Tx Power Limit %d, Min Limit %d\n",
			rTxPwrLimit.cMaxTxPwr,
			rTxPwrLimit.cMinTxPwr);
	} else {
		DBGLOG(RLM, TRACE, "LM: Disable Tx Power Limit\n");
	}

	wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_AP_CONSTRAINT_PWR_LIMIT, TRUE,
			    FALSE, FALSE, nicCmdEventSetCommon,
			    nicOidCmdTimeoutCommon,
			    sizeof(struct CMD_SET_AP_CONSTRAINT_PWR_LIMIT),
			    (uint8_t *)&rTxPwrLimit, NULL, 0);
}

#if (CFG_SUPPORT_802_11AX == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief Enable/Disable the SR function according to
      the wifi.cfg SREnable parameter
 *
 * @param prAdapter      a pointer to adapter private data structure.
 * @param fgIsEnableSr  a parameter to decide sr enable or disable.
 *
 * @return -
 */
/*----------------------------------------------------------------------------*/
void rlmSetSrControl(struct ADAPTER *prAdapter, bool fgIsEnableSr)
{
	struct _SR_CMD_SR_CAP_T *prCmdSrCap = NULL;

	ASSERT(prAdapter);

	prCmdSrCap = (struct _SR_CMD_SR_CAP_T *)
		kalMemAlloc(sizeof(struct _SR_CMD_SR_CAP_T),
			VIR_MEM_TYPE);
	if (prCmdSrCap == NULL) {
		DBGLOG(RLM, ERROR, "LM: Mem alloc fail for SR_CMD_SR_CAP\n");
		return;
	}

	prCmdSrCap->rSrCmd.u1CmdSubId = SR_CMD_SET_SR_CAP_SREN_CTRL;
	prCmdSrCap->rSrCmd.u1DbdcIdx = 0;
	prCmdSrCap->rSrCap.fgSrEn = fgIsEnableSr;

	wlanSendSetQueryExtCmd(prAdapter,
		CMD_ID_LAYER_0_EXT_MAGIC_NUM,
		EXT_CMD_ID_SR_CTRL,
		TRUE,
		FALSE,
		TRUE,
		NULL,
		nicOidCmdTimeoutCommon,
		sizeof(struct _SR_CMD_SR_CAP_T),
		(uint8_t *) (prCmdSrCap),
		NULL, 0);

	if (prCmdSrCap)
		kalMemFree(prCmdSrCap, VIR_MEM_TYPE,
			    sizeof(struct _SR_CMD_SR_CAP_T));
}
#endif

uint32_t rlmSendChannelSwitchTxDone(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

		DBGLOG(P2P, INFO,
			"CSA TX Done Status: %d, seqNo: %d, sta: %d\n",
			rTxDoneStatus,
			prMsduInfo->ucTxSeqNum,
			prMsduInfo->ucStaRecIndex);

	} while (FALSE);

	return WLAN_STATUS_SUCCESS;
}

static void __rlmSendChannelSwitchFrame(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStarec)
{
	struct MSDU_INFO *prMsduInfo;
	struct ACTION_CHANNEL_SWITCH_FRAME *prFrame;
	uint16_t u2EstimatedFrameLen;
	uint8_t *start, *pos;
#if CFG_ENABLE_WIFI_DIRECT
	uint32_t u4LifeTimeout, u4MaxLifeTimeout;
	uint32_t u4MarginTimeout = GO_CSA_ACTION_FRAME_LIFE_TIME_MARGIN_MS;
#endif

	if (!prBssInfo || !prStarec)
		return;

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
			      sizeof(struct ACTION_CHANNEL_SWITCH_FRAME) +
			      sizeof(struct IE_CHANNEL_SWITCH) +
			      sizeof(struct IE_SECONDARY_OFFSET) +
			      sizeof(struct IE_WIDE_BAND_CHANNEL);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
							u2EstimatedFrameLen);
	if (!prMsduInfo)
		return;

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);
	start = pos = (uint8_t *)prMsduInfo->prPacket;
	prFrame = (struct ACTION_CHANNEL_SWITCH_FRAME *)pos;

	prFrame->u2FrameCtrl = MAC_FRAME_ACTION;
	COPY_MAC_ADDR(prFrame->aucDestAddr, prStarec->aucMacAddr);
	COPY_MAC_ADDR(prFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prFrame->aucBSSID, prBssInfo->aucBSSID);
	prFrame->ucCategory = CATEGORY_SPEC_MGT;
	prFrame->ucAction = ACTION_CHNL_SWITCH;
	pos += sizeof(*prFrame);

	CSA_IE(pos)->ucId = ELEM_ID_CH_SW_ANNOUNCEMENT;
	CSA_IE(pos)->ucLength = 3;
	CSA_IE(pos)->ucChannelSwitchMode =
		prAdapter->rWifiVar.ucChannelSwitchMode;
	CSA_IE(pos)->ucNewChannelNum =
		prAdapter->rWifiVar.ucNewChannelNumber;
	CSA_IE(pos)->ucChannelSwitchCount =
		prAdapter->rWifiVar.ucChannelSwitchCount;
	pos += sizeof(struct IE_CHANNEL_SWITCH);

	switch (prAdapter->rWifiVar.ucNewChannelWidth) {
	case VHT_OP_CHANNEL_WIDTH_20_40:
		if (prAdapter->rWifiVar.ucSecondaryOffset == CHNL_EXT_SCN ||
		    prAdapter->rWifiVar.ucSecondaryOffset == CHNL_EXT_RES)
			break;

		SEC_OFFSET_IE(pos)->ucId = ELEM_ID_SCO;
		SEC_OFFSET_IE(pos)->ucLength = 1;
		SEC_OFFSET_IE(pos)->ucSecondaryOffset =
			prAdapter->rWifiVar.ucSecondaryOffset;

		pos += sizeof(struct IE_SECONDARY_OFFSET);
		break;
	case VHT_OP_CHANNEL_WIDTH_80:
	case VHT_OP_CHANNEL_WIDTH_160:
	case VHT_OP_CHANNEL_WIDTH_80P80:
		WIDE_BW_IE(pos)->ucId = ELEM_ID_WIDE_BAND_CHANNEL_SWITCH;
		WIDE_BW_IE(pos)->ucLength = 3;
		WIDE_BW_IE(pos)->ucNewChannelWidth =
			prAdapter->rWifiVar.ucNewChannelWidth;
		WIDE_BW_IE(pos)->ucChannelS1 =
			prAdapter->rWifiVar.ucNewChannelS1;
		WIDE_BW_IE(pos)->ucChannelS2 =
			prAdapter->rWifiVar.ucNewChannelS2;

		pos += sizeof(struct IE_WIDE_BAND_CHANNEL);
		break;
	default:
		break;
	}

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prBssInfo->ucBssIndex,
		     prStarec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     (uint16_t)(pos - start),
		     rlmSendChannelSwitchTxDone,
		     MSDU_RATE_MODE_AUTO);

#if CFG_ENABLE_WIFI_DIRECT
	/* GC not expects to receive after csa timeout */
	u4MaxLifeTimeout = prAdapter->rWifiVar.ucChannelSwitchCount *
		prBssInfo->u2BeaconInterval;

	if (u4MaxLifeTimeout > u4MarginTimeout)
		u4LifeTimeout = u4MaxLifeTimeout - u4MarginTimeout;
	else
		u4LifeTimeout = 0;

	if (u4LifeTimeout < GO_CSA_ACTION_FRAME_MINIMUM_LIFE_TIME_MS)
		u4LifeTimeout = GO_CSA_ACTION_FRAME_MINIMUM_LIFE_TIME_MS;

	nicTxSetPktLifeTime(prAdapter, prMsduInfo, u4LifeTimeout);
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
			MSDU_CONTROL_FLAG_FORCE_LINK |
			MSDU_CONTROL_FLAG_DIS_MAT,
			TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* 4 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}

uint32_t rlmSendExChannelSwitchTxDone(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

		DBGLOG(P2P, INFO,
			"Extended CSA TX Done Status: %d, seqNo: %d, sta: %d\n",
			rTxDoneStatus,
			prMsduInfo->ucTxSeqNum,
			prMsduInfo->ucStaRecIndex);

	} while (FALSE);

	return WLAN_STATUS_SUCCESS;
}

static void __rlmSendExChannelSwitchFrame(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct STA_RECORD *prStarec)
{
	struct MSDU_INFO *prMsduInfo;
	struct ACTION_EX_CHANNEL_SWITCH_FRAME *prFrame;
	uint16_t u2EstimatedFrameLen;
	uint8_t *start, *pos;
#if CFG_ENABLE_WIFI_DIRECT
	uint32_t u4LifeTimeout, u4MaxLifeTimeout;
	uint32_t u4MarginTimeout = GO_CSA_ACTION_FRAME_LIFE_TIME_MARGIN_MS;
#endif

	if (!prBssInfo || !prStarec)
		return;

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
			      sizeof(struct ACTION_EX_CHANNEL_SWITCH_FRAME) +
			      sizeof(struct IE_WIDE_BAND_CHANNEL);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
							u2EstimatedFrameLen);
	if (!prMsduInfo)
		return;

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);
	start = pos = (uint8_t *)prMsduInfo->prPacket;
	prFrame = (struct ACTION_EX_CHANNEL_SWITCH_FRAME *)pos;

	prFrame->u2FrameCtrl = MAC_FRAME_ACTION;
	COPY_MAC_ADDR(prFrame->aucDestAddr, prStarec->aucMacAddr);
	COPY_MAC_ADDR(prFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prFrame->aucBSSID, prBssInfo->aucBSSID);
	prFrame->ucCategory = CATEGORY_PUBLIC_ACTION;
	prFrame->ucAction = ACTION_PUBLIC_EX_CH_SW_ANNOUNCEMENT;
	prFrame->ucChannelSwitchMode =
		prAdapter->rWifiVar.ucChannelSwitchMode;
	prFrame->ucNewOperatingClass =
		prAdapter->rWifiVar.ucNewOperatingClass;
	prFrame->ucNewChannelNum =
		prAdapter->rWifiVar.ucNewChannelNumber;
	prFrame->ucChannelSwitchCount =
		prAdapter->rWifiVar.ucChannelSwitchCount;
	pos += sizeof(struct ACTION_EX_CHANNEL_SWITCH_FRAME);

	switch (prAdapter->rWifiVar.ucNewChannelWidth) {
	case VHT_OP_CHANNEL_WIDTH_80:
	case VHT_OP_CHANNEL_WIDTH_160:
	case VHT_OP_CHANNEL_WIDTH_80P80:
		WIDE_BW_IE(pos)->ucId = ELEM_ID_WIDE_BAND_CHANNEL_SWITCH;
		WIDE_BW_IE(pos)->ucLength = 3;
		WIDE_BW_IE(pos)->ucNewChannelWidth =
			prAdapter->rWifiVar.ucNewChannelWidth;
		WIDE_BW_IE(pos)->ucChannelS1 =
			prAdapter->rWifiVar.ucNewChannelS1;
		WIDE_BW_IE(pos)->ucChannelS2 =
			prAdapter->rWifiVar.ucNewChannelS2;

		pos += sizeof(struct IE_WIDE_BAND_CHANNEL);
		break;
	default:
		break;
	}

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prBssInfo->ucBssIndex,
		     prStarec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     (uint16_t)(pos - start),
		     rlmSendExChannelSwitchTxDone,
		     MSDU_RATE_MODE_AUTO);

#if CFG_ENABLE_WIFI_DIRECT
	/* GC not expects to receive after csa timeout */
	u4MaxLifeTimeout = prAdapter->rWifiVar.ucChannelSwitchCount *
		prBssInfo->u2BeaconInterval;

	if (u4MaxLifeTimeout > u4MarginTimeout)
		u4LifeTimeout = u4MaxLifeTimeout - u4MarginTimeout;
	else
		u4LifeTimeout = 0;

	if (u4LifeTimeout < GO_CSA_ACTION_FRAME_MINIMUM_LIFE_TIME_MS)
		u4LifeTimeout = GO_CSA_ACTION_FRAME_MINIMUM_LIFE_TIME_MS;

	nicTxSetPktLifeTime(prAdapter, prMsduInfo, u4LifeTimeout);
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicTxConfigPktControlFlag(prMsduInfo,
			MSDU_CONTROL_FLAG_FORCE_LINK |
			MSDU_CONTROL_FLAG_DIS_MAT,
			TRUE);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/* 4 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}

uint16_t rlmOpClassToBandwidth(uint8_t ucOpClass)
{
	switch (ucOpClass) {
		case 81:
		case 82:
			return BW_20;
		case 83: /* channels 1..9; 40 MHz */
		case 84: /* channels 5..13; 40 MHz */
			return BW_40;
		case 115: /* channels 36,40,44,48; indoor only */
			return BW_20;
		case 116: /* channels 36,44; 40 MHz; indoor only */
		case 117: /* channels 40,48; 40 MHz; indoor only */
			return BW_40;
		case 118: /* channels 52,56,60,64; dfs */
			return BW_20;
		case 119: /* channels 52,60; 40 MHz; dfs */
		case 120: /* channels 56,64; 40 MHz; dfs */
			return BW_40;
		case 121: /* channels 100-140 */
			return BW_20;
		case 122: /* channels 100-142; 40 MHz */
		case 123: /* channels 104-136; 40 MHz */
			return BW_40;
		case 124: /* channels 149,153,157,161 */
		case 125: /* channels 149,153,157,161,165,169,173,177 */
			return BW_20;
		case 126: /* channels 149,157,161,165,169,173; 40 MHz */
		case 127: /* channels 153..177; 40 MHz */
			return BW_40;
		case 128: /* center freqs 42, 58, 106, 122, 138, 155, 171; 80 MHz */
			return BW_80;
		case 129: /* center freqs 50, 114, 163; 160 MHz */
			return BW_160;
		case 130: /* center freqs 42, 58, 106, 122, 138, 155, 171; 80+80 MHz */
			return BW_8080;
		case 131: /* UHB channels, 20 MHz: 1, 5, 9.. */
		case 136:
			return BW_20;
		case 132: /* UHB channels, 40 MHz: 3, 11, 19.. */
			return BW_40;
		case 133: /* UHB channels, 80 MHz: 7, 23, 39.. */
			return BW_80;
		case 134: /* UHB channels, 160 MHz: 15, 47, 79.. */
			return BW_160;
		case 135: /* UHB channels, 80+80 MHz: 7, 23, 39.. */
			return BW_8080;
		case 137: /* UHB channels, 320 MHz */
			return BW_320;
	}
	return BW_20;
}

int32_t rlmGetOpClassForChannel(int32_t channel,
	enum ENUM_BAND band, enum ENUM_CHNL_EXT eSco,
	enum ENUM_CHANNEL_WIDTH eChBw, uint16_t u2Country)
{
	/* 2GHz Band */
	if ((band == BAND_2G4) != 0) {
		if (channel == 14)
			return 82;

		if (channel >= 1 && channel <= 13) {
			if (eSco == CHNL_EXT_SCN) {
				/* 20MHz channel */
				if (u2Country == COUNTRY_CODE_US)
					return 12;
				else
					return 81;
			}
			if (channel <= 9 && eSco == CHNL_EXT_SCA)
			/* HT40 with secondary channel
			 * above primary, priCH will be 1~9
			 */
				return 83;
			else if (channel >= 5 && eSco == CHNL_EXT_SCB)
			/* HT40 with secondary channel
			 * below primary, priCH will be 5~13
			 */
				return 84;
		}
		/* Error */
		return 0;
	}

	/* 5GHz Band */
	if ((band == BAND_5G) != 0) {
		/* BW20 */
		if (eChBw == CW_20_40MHZ && eSco == CHNL_EXT_SCN) {
			if (channel >= 36 && channel <= 48) {
				if (u2Country == COUNTRY_CODE_US)
					return 1;
				else
					return 115;
			}
			if (channel >= 52 && channel <= 64) {
				if (u2Country == COUNTRY_CODE_US)
					return 2;
				else
					return 118;
			}
			if (channel >= 100 && channel <= 144) {
				if (u2Country == COUNTRY_CODE_US)
					return 4;
				else
					return 121;
			}
			if (channel >= 149 && channel <= 161) {
				if (u2Country == COUNTRY_CODE_US)
					return 3;
				else
					return 124;
			}
			if (channel >= 165 && channel <= 169) {
				if (u2Country == COUNTRY_CODE_US)
					return 5;
				else
					return 125;
			}
		/* BW 40 */
		} else if (eChBw == CW_20_40MHZ &&
			(eSco == CHNL_EXT_SCA || eSco == CHNL_EXT_SCB)) {
			switch (channel) {
			case 36:
			case 44:
				/* HT40 with secondary channel
				 * above primary
				 */
				return 116;
			case 40:
			case 48:
				/* HT40 with secondary channel
				 * below primary
				 */
				return 117;
			case 52:
			case 60:
				/* HT40 with secondary channel
				 * above primary
				 */
				return  119;
			case 56:
			case 64:
				/* HT40 with secondary channel
				 * below primary
				 */
				return 120;
			case 100:
			case 108:
			case 116:
			case 124:
			case 132:
			case 140:
				/* HT40 with secondary channel
				 * above primary
				 */
				return 122;
			case 104:
			case 112:
			case 120:
			case 128:
			case 136:
			case 144:
				/* HT40 with secondary channel
				 * below primary
				 */
				return 123;
			case 149:
			case 157:
				/* HT40 with secondary channel
				 * above primary
				 */
				return 126;
			case 153:
			case 161:
				/* HT40 with secondary channel
				 * below primary
				 */
				return 127;
			}
		} else if (eChBw == CW_80MHZ) {
			return 128;
		} else if (eChBw == CW_160MHZ) {
			return 129;
		}
		/* Error */
		return 0;
	}

#if (CFG_SUPPORT_WIFI_6G == 1)
	/* 6GHz Band */
	if ((band == BAND_6G) != 0) {
		/* Channels 1, 5. 9, 13, ... */
		if ((channel & 0x03) == 0x01)
			/* 20MHz channel */
			return 131;

		/* Channels 3, 11, 19, 27, ... */
		if ((channel & 0x07) == 0x03)
			/* 40MHz channel */
			return 132;

		/* Channels 7, 23, 39, 55, ... */
		if ((channel & 0x0F) == 0x07)
			/* 80MHz channel */
			return 133;

		/* Channels 15, 47, 69, ... */
		if ((channel & 0x1F) == 0x0F)
			/* 160MHz channel */
			return 134;

		/* Channels 31, 63, 95, ... */
		if ((channel & 0x3F) == 0x1F)
			/* 320MHz channel */
			return 137;

		if (channel == 2)
			/* 20MHz channel */
			return 136;

		/* Error */
		return 0;
	}
#endif

	return 0;
}

#if CFG_SUPPORT_BFER
/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void rlmBfStaRecPfmuUpdate(struct ADAPTER *prAdapter,
				struct STA_RECORD *prStaRec)
{
	uint8_t ucBFerMaxNr, ucBFeeMaxNr, ucMode = 0;
	struct BSS_INFO *prBssInfo;
	struct CMD_STAREC_BF *prStaRecBF;
	struct CMD_STAREC_UPDATE *prStaRecUpdateInfo;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SetBufferLen = sizeof(struct CMD_STAREC_BF);
#if (CFG_SUPPORT_802_11BE == 1)
	uint32_t u4EhtPhyCap1 = 0;
#endif

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	ASSERT(prBssInfo);

	if (RLM_NET_IS_11AC(prBssInfo) &&
	    IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaVhtBfer))
		ucMode = MODE_VHT;
	else if (RLM_NET_IS_11N(prBssInfo) &&
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaHtBfer))
		ucMode = MODE_HT;
#if (CFG_SUPPORT_802_11AX == 1)
	if (RLM_NET_IS_11AX(prBssInfo) &&
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaHeSuBfer))
		ucMode = MODE_HE_SU;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	if (RLM_NET_IS_11BE(prBssInfo) &&
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaHeSuBfer))
		ucMode = MODE_EHT_SU;
#endif

	prStaRecBF =
	    (struct CMD_STAREC_BF *) cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG, u4SetBufferLen);

	if (!prStaRecBF) {
		DBGLOG(RLM, ERROR, "STA Rec memory alloc fail\n");
		return;
	}

	prStaRecUpdateInfo =
	    (struct CMD_STAREC_UPDATE *) cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG, (CMD_STAREC_UPDATE_HDR_SIZE + u4SetBufferLen));

	if (!prStaRecUpdateInfo) {
		cnmMemFree(prAdapter, prStaRecBF);
		DBGLOG(RLM, ERROR, "STA Rec Update Info memory alloc fail\n");
		return;
	}

	prStaRec->rTxBfPfmuStaInfo.u2PfmuId = 0xFFFF;

	switch (ucMode) {
#if (CFG_SUPPORT_802_11BE == 1)
	case MODE_EHT_SU:
		memcpy(&u4EhtPhyCap1, prStaRec->ucEhtPhyCapInfo,
			sizeof(u4EhtPhyCap1));
		prStaRec->rTxBfPfmuStaInfo.fgSU_MU = FALSE;
		prStaRec->rTxBfPfmuStaInfo.u1TxBfCap =
			((u4EhtPhyCap1 & DOT11BE_PHY_CAP_SU_BFEE) >> 6);

		if (prStaRec->rTxBfPfmuStaInfo.u1TxBfCap) {
			/* OFDM, NDPA/Report Poll/CTS2Self tx mode */
			prStaRec->rTxBfPfmuStaInfo.ucSoundingPhy =
						TX_RATE_MODE_OFDM;

			/* 9: OFDM 24M */
			prStaRec->rTxBfPfmuStaInfo.ucNdpaRate = PHY_RATE_24M;

			/* VHT mode, NDP tx mode */
			prStaRec->rTxBfPfmuStaInfo.ucTxMode =
				TX_RATE_MODE_EHT_MU;

			/* 0: MCS0 */
			prStaRec->rTxBfPfmuStaInfo.ucNdpRate = PHY_RATE_MCS0;

			/* 9: OFDM 24M */
			prStaRec->rTxBfPfmuStaInfo.ucReptPollRate =
				PHY_RATE_24M;

			switch (prBssInfo->ucVhtChannelWidth) {
			case VHT_OP_CHANNEL_WIDTH_320_1:
			case VHT_OP_CHANNEL_WIDTH_320_2:
				prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW320;
				ucBFeeMaxNr =
					GET_DOT11BE_PHY_CAP_BFEE_320M(
						u4EhtPhyCap1);
				break;
			case VHT_OP_CHANNEL_WIDTH_160:
				prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW160;
				ucBFeeMaxNr =
					GET_DOT11BE_PHY_CAP_BFEE_160M(
						u4EhtPhyCap1);
				break;
			case VHT_OP_CHANNEL_WIDTH_80:
				prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW80;
				ucBFeeMaxNr =
					GET_DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M(
						u4EhtPhyCap1);
				break;
			case VHT_OP_CHANNEL_WIDTH_20_40:
			default:
				prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW20;
				if (prBssInfo->eBssSCO != CHNL_EXT_SCN)
					prStaRec->rTxBfPfmuStaInfo.ucCBW =
								BF_CBW40;
				ucBFeeMaxNr =
					GET_DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M(
						u4EhtPhyCap1);
				break;
			}

			ucBFerMaxNr = 1;
			prStaRec->rTxBfPfmuStaInfo.ucNr =
				(ucBFerMaxNr < ucBFeeMaxNr) ?
					ucBFerMaxNr : ucBFeeMaxNr;

			if (RLM_NET_IS_11AC(prBssInfo)) {
				prStaRec->rTxBfPfmuStaInfo.ucNc =
					((prStaRec->u2VhtRxMcsMap &
					VHT_CAP_INFO_MCS_2SS_MASK) !=
						BITS(2, 3)) ? 1 : 0;
			} else if (RLM_NET_IS_11N(prBssInfo)) {
				prStaRec->rTxBfPfmuStaInfo.ucNc =
				     (prStaRec->aucRxMcsBitmask[1] > 0) ? 1 : 0;
			}
		}
		break;
#endif
#if (CFG_SUPPORT_802_11AX == 1)
case MODE_HE_SU:
	prStaRec->rTxBfPfmuStaInfo.fgSU_MU = FALSE;
	prStaRec->rTxBfPfmuStaInfo.u1TxBfCap =
			HE_GET_PHY_CAP_SU_BFMEE(prStaRec->ucHePhyCapInfo);

	if (prStaRec->rTxBfPfmuStaInfo.u1TxBfCap) {
		/* OFDM, NDPA/Report Poll/CTS2Self tx mode */
		prStaRec->rTxBfPfmuStaInfo.ucSoundingPhy =
						TX_RATE_MODE_OFDM;

		/* 9: OFDM 24M */
		prStaRec->rTxBfPfmuStaInfo.ucNdpaRate = PHY_RATE_24M;

		/* VHT mode, NDP tx mode */
		prStaRec->rTxBfPfmuStaInfo.ucTxMode = TX_RATE_MODE_HE_SU;

		/* 0: MCS0 */
		prStaRec->rTxBfPfmuStaInfo.ucNdpRate = PHY_RATE_MCS0;

		/* 9: OFDM 24M */
		prStaRec->rTxBfPfmuStaInfo.ucReptPollRate = PHY_RATE_24M;

		switch (prBssInfo->ucVhtChannelWidth) {
		case VHT_OP_CHANNEL_WIDTH_160:
			prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW160;
			break;
		case VHT_OP_CHANNEL_WIDTH_80:
			prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW80;
			break;

		case VHT_OP_CHANNEL_WIDTH_20_40:
		default:
			prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW20;
			if (prBssInfo->eBssSCO != CHNL_EXT_SCN)
				prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW40;
			break;
		}

		ucBFerMaxNr = 1;
		ucBFeeMaxNr = (prBssInfo->ucVhtChannelWidth <=
			VHT_OP_CHANNEL_WIDTH_80) ?
			HE_GET_PHY_CAP_BFMEE_STS_LT_OR_EQ_80M(
				prStaRec->ucHePhyCapInfo) :
			HE_GET_PHY_CAP_BFMEE_STS_GT_80M(
				prStaRec->ucHePhyCapInfo);
		prStaRec->rTxBfPfmuStaInfo.ucNr =
			(ucBFerMaxNr < ucBFeeMaxNr) ?
				ucBFerMaxNr : ucBFeeMaxNr;

		if (RLM_NET_IS_11AC(prBssInfo)) {
			prStaRec->rTxBfPfmuStaInfo.ucNc =
				((prStaRec->u2VhtRxMcsMap &
				VHT_CAP_INFO_MCS_2SS_MASK) !=
						BITS(2, 3)) ? 1 : 0;
		} else if (RLM_NET_IS_11N(prBssInfo)) {
			prStaRec->rTxBfPfmuStaInfo.ucNc =
				(prStaRec->aucRxMcsBitmask[1] > 0) ? 1 : 0;
		}
	}
	break;

#endif
	case MODE_VHT:
		prStaRec->rTxBfPfmuStaInfo.fgSU_MU = FALSE;
		prStaRec->rTxBfPfmuStaInfo.u1TxBfCap =
				rlmClientSupportsVhtETxBF(prStaRec);

		if (prStaRec->rTxBfPfmuStaInfo.u1TxBfCap) {
			/* OFDM, NDPA/Report Poll/CTS2Self tx mode */
			prStaRec->rTxBfPfmuStaInfo.ucSoundingPhy =
							TX_RATE_MODE_OFDM;

			/* 9: OFDM 24M */
			prStaRec->rTxBfPfmuStaInfo.ucNdpaRate = PHY_RATE_24M;

			/* VHT mode, NDP tx mode */
			prStaRec->rTxBfPfmuStaInfo.ucTxMode = TX_RATE_MODE_VHT;

			/* 0: MCS0 */
			prStaRec->rTxBfPfmuStaInfo.ucNdpRate = PHY_RATE_MCS0;

			/* 9: OFDM 24M */
			prStaRec->rTxBfPfmuStaInfo.ucReptPollRate = PHY_RATE_24M;

			switch (prBssInfo->ucVhtChannelWidth) {
			case VHT_OP_CHANNEL_WIDTH_160:
				prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW160;
				break;
			case VHT_OP_CHANNEL_WIDTH_80:
				prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW80;
				break;

			case VHT_OP_CHANNEL_WIDTH_20_40:
			default:
				prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW20;
				if (prBssInfo->eBssSCO != CHNL_EXT_SCN)
					prStaRec->rTxBfPfmuStaInfo.ucCBW =
								BF_CBW40;
				break;
			}

			ucBFerMaxNr = 1;
			ucBFeeMaxNr = rlmClientSupportsVhtBfeeStsCap(prStaRec);
			prStaRec->rTxBfPfmuStaInfo.ucNr =
				(ucBFerMaxNr < ucBFeeMaxNr) ?
					ucBFerMaxNr : ucBFeeMaxNr;
			prStaRec->rTxBfPfmuStaInfo.ucNc =
				((prStaRec->u2VhtRxMcsMap &
					VHT_CAP_INFO_MCS_2SS_MASK) !=
							BITS(2, 3)) ? 1 : 0;
		}
		break;

	case MODE_HT:
		prStaRec->rTxBfPfmuStaInfo.fgSU_MU = FALSE;
		prStaRec->rTxBfPfmuStaInfo.u1TxBfCap =
				rlmClientSupportsHtETxBF(prStaRec);

		if (prStaRec->rTxBfPfmuStaInfo.u1TxBfCap) {
			/* HT mode, NDPA/NDP tx mode */
			prStaRec->rTxBfPfmuStaInfo.ucTxMode =
						TX_RATE_MODE_HTMIX;

			/* 0: HT MCS0 */
			prStaRec->rTxBfPfmuStaInfo.ucNdpaRate = PHY_RATE_MCS0;

			prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW20;
			if (prBssInfo->eBssSCO != CHNL_EXT_SCN)
				prStaRec->rTxBfPfmuStaInfo.ucCBW = BF_CBW40;

			ucBFerMaxNr = 1;
			ucBFeeMaxNr =
				(prStaRec->u4TxBeamformingCap &
				TXBF_COMPRESSED_TX_ANTENNANUM_SUPPORTED) >>
				TXBF_COMPRESSED_TX_ANTENNANUM_SUPPORTED_OFFSET;
			prStaRec->rTxBfPfmuStaInfo.ucNr =
				(ucBFerMaxNr < ucBFeeMaxNr) ?
					ucBFerMaxNr : ucBFeeMaxNr;
			prStaRec->rTxBfPfmuStaInfo.ucNc =
				(prStaRec->aucRxMcsBitmask[1] > 0) ? 1 : 0;
			prStaRec->rTxBfPfmuStaInfo.ucNdpRate =
				prStaRec->rTxBfPfmuStaInfo.ucNr * 8;
		}
		break;
	default:
		cnmMemFree(prAdapter, prStaRecBF);
		cnmMemFree(prAdapter, prStaRecUpdateInfo);
		return;
	}

	DBGLOG(RLM, INFO, "ucMode=%d\n", ucMode);
	DBGLOG(RLM, INFO, "rlmClientSupportsVhtETxBF(prStaRec)=%d\n",
				rlmClientSupportsVhtETxBF(prStaRec));
	DBGLOG(RLM, INFO, "rlmClientSupportsVhtBfeeStsCap(prStaRec)=%d\n",
				rlmClientSupportsVhtBfeeStsCap(prStaRec));
	DBGLOG(RLM, INFO, "prStaRec->u2VhtRxMcsMap=%x\n",
				prStaRec->u2VhtRxMcsMap);

	DBGLOG(RLM, INFO,
	    "====================== BF StaRec Info =====================\n");
	DBGLOG(RLM, INFO, "u2PfmuId       =%d\n",
				prStaRec->rTxBfPfmuStaInfo.u2PfmuId);
	DBGLOG(RLM, INFO, "fgSU_MU        =%d\n",
				prStaRec->rTxBfPfmuStaInfo.fgSU_MU);
	DBGLOG(RLM, INFO, "u1TxBfCap     =%d\n",
				prStaRec->rTxBfPfmuStaInfo.u1TxBfCap);
	DBGLOG(RLM, INFO, "ucSoundingPhy  =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucSoundingPhy);
	DBGLOG(RLM, INFO, "ucNdpaRate     =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucNdpaRate);
	DBGLOG(RLM, INFO, "ucNdpRate      =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucNdpRate);
	DBGLOG(RLM, INFO, "ucReptPollRate =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucReptPollRate);
	DBGLOG(RLM, INFO, "ucTxMode       =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucTxMode);
	DBGLOG(RLM, INFO, "ucNc           =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucNc);
	DBGLOG(RLM, INFO, "ucNr           =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucNr);
	DBGLOG(RLM, INFO, "ucCBW          =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucCBW);
	DBGLOG(RLM, INFO, "ucTotMemRequire=%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucTotMemRequire);
	DBGLOG(RLM, INFO, "ucMemRequire20M=%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucMemRequire20M);
	DBGLOG(RLM, INFO, "ucMemRow0      =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucMemRow0);
	DBGLOG(RLM, INFO, "ucMemCol0      =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucMemCol0);
	DBGLOG(RLM, INFO, "ucMemRow1      =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucMemRow1);
	DBGLOG(RLM, INFO, "ucMemCol1      =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucMemCol1);
	DBGLOG(RLM, INFO, "ucMemRow2      =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucMemRow2);
	DBGLOG(RLM, INFO, "ucMemCol2      =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucMemCol2);
	DBGLOG(RLM, INFO, "ucMemRow3      =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucMemRow3);
	DBGLOG(RLM, INFO, "ucMemCol3      =%d\n",
				prStaRec->rTxBfPfmuStaInfo.ucMemCol3);
	DBGLOG(RLM, INFO,
	    "===========================================================\n");



	prStaRecBF->u2Tag = STA_REC_BF;
	prStaRecBF->u2Length = u4SetBufferLen;
	kalMemCopy(&prStaRecBF->rTxBfPfmuInfo,
		&prStaRec->rTxBfPfmuStaInfo, sizeof(struct TXBF_PFMU_STA_INFO));


	prStaRecUpdateInfo->ucBssIndex = prStaRec->ucBssIndex;
	prStaRecUpdateInfo->ucWlanIdx = prStaRec->ucWlanIndex;
	prStaRecUpdateInfo->u2TotalElementNum = 1;
	kalMemCopy(prStaRecUpdateInfo->aucBuffer, prStaRecBF, u4SetBufferLen);


	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			     EXT_CMD_ID_STAREC_UPDATE,
			     TRUE,
			     FALSE,
			     FALSE,
			     nicCmdEventSetCommon,
			     nicOidCmdTimeoutCommon,
			     (CMD_STAREC_UPDATE_HDR_SIZE + u4SetBufferLen),
			     (uint8_t *) prStaRecUpdateInfo, NULL, 0);

	if (rWlanStatus == WLAN_STATUS_FAILURE)
		DBGLOG(RLM, ERROR, "Send BF sounding cmd fail\n");

	cnmMemFree(prAdapter, prStaRecBF);
	cnmMemFree(prAdapter, prStaRecUpdateInfo);

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
void rlmETxBfTriggerPeriodicSounding(struct ADAPTER *prAdapter)
{
	uint32_t u4SetBufferLen = sizeof(union PARAM_CUSTOM_TXBF_ACTION_STRUCT);
	union PARAM_CUSTOM_TXBF_ACTION_STRUCT rTxBfActionInfo;
	union CMD_TXBF_ACTION rCmdTxBfActionInfo;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(RLM, INFO, "rlmETxBfTriggerPeriodicSounding\n");

	rTxBfActionInfo.rTxBfSoundingStart.ucTxBfCategory =
								BF_SOUNDING_ON;

	rTxBfActionInfo.rTxBfSoundingStart.ucSuMuSndMode =
						    AUTO_SU_PERIODIC_SOUNDING;

	rTxBfActionInfo.rTxBfSoundingStart.u4SoundingInterval = 0;

	rTxBfActionInfo.rTxBfSoundingStart.ucStaNum = 0;

	kalMemCopy(&rCmdTxBfActionInfo, &rTxBfActionInfo,
					sizeof(union CMD_TXBF_ACTION));

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
					     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
					     EXT_CMD_ID_BF_ACTION,
					     TRUE,
					     FALSE,
					     FALSE,
					     nicCmdEventSetCommon,
					     nicOidCmdTimeoutCommon,
					     sizeof(union CMD_TXBF_ACTION),
					     (uint8_t *) &rCmdTxBfActionInfo,
					     &rTxBfActionInfo, u4SetBufferLen);

	if (rWlanStatus == WLAN_STATUS_FAILURE)
		DBGLOG(RLM, ERROR, "Send BF sounding cmd fail\n");

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
bool
rlmClientSupportsVhtETxBF(struct STA_RECORD *prStaRec)
{
	uint8_t ucVhtCapSuBfeeCap;

	ucVhtCapSuBfeeCap =
		(prStaRec->u4VhtCapInfo & VHT_CAP_INFO_SU_BEAMFORMEE_CAPABLE)
		>> VHT_CAP_INFO_SU_BEAMFORMEE_CAPABLE_OFFSET;

	return (ucVhtCapSuBfeeCap) ? TRUE : FALSE;
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
uint8_t
rlmClientSupportsVhtBfeeStsCap(struct STA_RECORD *prStaRec)
{
	uint8_t ucVhtCapBfeeStsCap;

	ucVhtCapBfeeStsCap =
	    (prStaRec->u4VhtCapInfo &
VHT_CAP_INFO_COMPRESSED_STEERING_NUMBER_OF_BEAMFORMER_ANTENNAS_SUP) >>
VHT_CAP_INFO_COMPRESSED_STEERING_NUMBER_OF_BEAMFORMER_ANTENNAS_SUP_OFF;

	return ucVhtCapBfeeStsCap;
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
bool
rlmClientSupportsHtETxBF(struct STA_RECORD *prStaRec)
{
	uint32_t u4RxNDPCap, u4ComBfFbkCap;

	u4RxNDPCap = (prStaRec->u4TxBeamformingCap & TXBF_RX_NDP_CAPABLE)
						>> TXBF_RX_NDP_CAPABLE_OFFSET;

	/* Support compress feedback */
	u4ComBfFbkCap = (prStaRec->u4TxBeamformingCap &
			TXBF_EXPLICIT_COMPRESSED_FEEDBACK_IMMEDIATE_CAPABLE)
			>> TXBF_EXPLICIT_COMPRESSED_FEEDBACK_CAPABLE_OFFSET;

	return (u4RxNDPCap == 1) && (u4ComBfFbkCap > 0);
}

#endif

#if CFG_AP_80211K_SUPPORT
static void rlmMulAPAgentFillApRrmCapa(uint8_t *pucCapa)
{
	uint8_t ucIndex = 0;
	uint8_t aucEnabledBits[] = {RRM_CAP_INFO_BEACON_PASSIVE_MEASURE_BIT,
				    RRM_CAP_INFO_BEACON_ACTIVE_MEASURE_BIT,
				    RRM_CAP_INFO_BEACON_TABLE_BIT,
				    RRM_CAP_INFO_CHANNEL_LOAD_MEASURE_BIT,
				    RRM_CAP_INFO_NOISE_HISTOGRAM_MEASURE_BIT,
				    RRM_CAP_INFO_RRM_BIT};

	for (; ucIndex < sizeof(aucEnabledBits); ucIndex++)
		SET_EXT_CAP(pucCapa, ELEM_MAX_LEN_RRM_CAP,
			    aucEnabledBits[ucIndex]);
}

void rlmMulAPAgentGenerateApRRMEnabledCapIE(
				struct ADAPTER *prAdapter,
				struct MSDU_INFO *prMsduInfo)
{
	struct IE_RRM_ENABLED_CAP *prRrmEnabledCap = NULL;

	if (!prAdapter) {
		DBGLOG(RLM, ERROR, "prAdapter is NULL!\n");
		return;
	}

	if (!prMsduInfo) {
		DBGLOG(RLM, ERROR, "prMsduInfo is NULL!\n");
		return;
	}

	prRrmEnabledCap = (struct IE_RRM_ENABLED_CAP *)
	    (((uint8_t *) prMsduInfo->prPacket) + prMsduInfo->u2FrameLength);
	prRrmEnabledCap->ucId = ELEM_ID_RRM_ENABLED_CAP;
	prRrmEnabledCap->ucLength = ELEM_MAX_LEN_RRM_CAP;
	kalMemZero(&prRrmEnabledCap->aucCap[0], ELEM_MAX_LEN_RRM_CAP);
	rlmMulAPAgentFillApRrmCapa(&prRrmEnabledCap->aucCap[0]);
	prMsduInfo->u2FrameLength += IE_SIZE(prRrmEnabledCap);
}

void rlmMulAPAgentTxMeasurementRequest(
				struct ADAPTER *prAdapter,
				struct STA_RECORD *prStaRec,
				struct SUB_ELEMENT_LIST *prSubIEs)
{
	static uint8_t ucDialogToken = 1;
	struct MSDU_INFO *prMsduInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t *pucPayload = NULL;
	struct ACTION_RM_REQ_FRAME *prTxFrame = NULL;
	uint16_t u2TxFrameLen = 500;
	uint16_t u2FrameLen = 0;

	if (!prStaRec)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "prBssInfo is NULL!\n");
		return;
	}
	/* 1 Allocate MSDU Info */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(
		prAdapter, MAC_TX_RESERVED_FIELD + u2TxFrameLen);
	if (!prMsduInfo)
		return;
	prTxFrame = (struct ACTION_RM_REQ_FRAME
			     *)((uintptr_t)(prMsduInfo->prPacket) +
				MAC_TX_RESERVED_FIELD);

	/* 2 Compose The Mac Header. */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;
	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);
	prTxFrame->ucCategory = CATEGORY_RM_ACTION;
	prTxFrame->ucAction = RM_ACTION_RM_REQUEST;
	prTxFrame->u2Repetitions = HTONS(prStaRec->u2BcnReqRepetition);
	u2FrameLen = OFFSET_OF(struct ACTION_RM_REQ_FRAME, aucInfoElem);
	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = ucDialogToken++;
	u2TxFrameLen -= (sizeof(*prTxFrame) - 1);
	pucPayload = &prTxFrame->aucInfoElem[0];
	while (prSubIEs && u2TxFrameLen >= (prSubIEs->rSubIE.ucLength + 2)) {
		kalMemCopy(pucPayload, &prSubIEs->rSubIE,
			   prSubIEs->rSubIE.ucLength + 2);
		pucPayload += prSubIEs->rSubIE.ucLength + 2;
		u2FrameLen += prSubIEs->rSubIE.ucLength + 2;
		prSubIEs = prSubIEs->prNext;
		if (prSubIEs)
			prTxFrame->u2Repetitions++;
	}
	nicTxSetMngPacket(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
			  prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
			  u2FrameLen, NULL, MSDU_RATE_MODE_AUTO);

	/* 5 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}

static u_int8_t rlmMulAPAgentRmReportFrameIsValid(
					struct SW_RFB *prSwRfb)
{
	uint16_t u2ElemLen = 0;
	uint16_t u2Offset =
		(uint16_t)OFFSET_OF(struct ACTION_RM_REPORT_FRAME, aucInfoElem);
	uint8_t *pucIE = (uint8_t *)prSwRfb->pvHeader;
	struct IE_MEASUREMENT_REPORT *prCurrMeasElem = NULL;
	uint16_t u2CalcIELen = 0;
	uint16_t u2IELen = 0;

	if (prSwRfb->u2PacketLen <= u2Offset) {
		DBGLOG(RLM, ERROR, "RRM: Rm Packet length %d is too short\n",
		       prSwRfb->u2PacketLen);
		return FALSE;
	}
	pucIE += u2Offset;
	u2ElemLen = prSwRfb->u2PacketLen - u2Offset;
	IE_FOR_EACH(pucIE, u2ElemLen, u2Offset)
	{
		u2IELen = IE_LEN(pucIE);

		/* The minimum value of the Length field is 3 (based on a
		 ** minimum length for the Measurement Request field
		 ** of 0 octets
		 */
		if (u2IELen <= 3) {
			DBGLOG(RLM, ERROR, "RRM: Abnormal RM IE length is %d\n",
			       u2IELen);
			return FALSE;
		}

		/* Check whether the length of each measurment request element
		 ** is reasonable
		 */
		prCurrMeasElem = (struct IE_MEASUREMENT_REPORT *)pucIE;
		switch (prCurrMeasElem->ucMeasurementType) {
		case ELEM_RM_TYPE_BEACON_REPORT:
			if (u2IELen < (3 + OFFSET_OF(struct RM_BCN_REPORT,
						     aucOptElem))) {
				DBGLOG(RLM, ERROR,
				       "RRM: Abnormal Becaon Req IE length is %d\n",
				       u2IELen);
				return FALSE;
			}
			break;
		default:
			DBGLOG(RLM, ERROR,
			       "RRM: Not support: MeasurementType is %d, IE length is %d\n",
				prCurrMeasElem->ucMeasurementType, u2IELen);
			return FALSE;
		}

		u2CalcIELen += IE_SIZE(pucIE);
	}
	if (u2CalcIELen != u2ElemLen) {
		DBGLOG(RLM, ERROR,
		       "RRM: Calculated Total IE len is not equal to received length\n");
		return FALSE;
	}
	return TRUE;
}

void rlmMulAPAgentProcessRadioMeasurementResponse(
		struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct ACTION_RM_REPORT_FRAME *prRxFrame = NULL;
	uint8_t *pucOptInfo = NULL;
	uint8_t *pucReportElem = NULL;
	uint16_t u2TmpLen = 0;
	struct IE_MEASUREMENT_REPORT *prMeasureReportIE = NULL;
	struct RM_BCN_REPORT *prBeaconReportIE = NULL;
	struct T_MULTI_AP_BEACON_METRICS_RESP *prBcnMeasureReport = NULL;
	int32_t i4Ret = 0;

	if (!prAdapter) {
		DBGLOG(RLM, ERROR, "prAdapter is NULL!\n");
		return;
	}

	if (!prSwRfb) {
		DBGLOG(RLM, ERROR, "prSwRfb is NULL!\n");
		return;
	}

	/*TODO, check it's soft ap mode or not ?*/

	prRxFrame = (struct ACTION_RM_REPORT_FRAME *)prSwRfb->pvHeader;
	if (!rlmMulAPAgentRmReportFrameIsValid(prSwRfb))
		return;

	prBcnMeasureReport = (struct T_MULTI_AP_BEACON_METRICS_RESP *)
			kalMemAlloc(sizeof(
				struct T_MULTI_AP_BEACON_METRICS_RESP),
			VIR_MEM_TYPE);
	if (prBcnMeasureReport == NULL) {
		DBGLOG(INIT, ERROR, "alloc memory fail\n");
		return;
	}

	kalMemZero(prBcnMeasureReport,
		sizeof(struct T_MULTI_AP_BEACON_METRICS_RESP));
	COPY_MAC_ADDR(prBcnMeasureReport->mStaMac, prRxFrame->aucSrcAddr);

	pucOptInfo = &prRxFrame->aucInfoElem[0];
	u2TmpLen = OFFSET_OF(struct ACTION_RM_REPORT_FRAME, aucInfoElem);
	pucReportElem = prBcnMeasureReport->uElem;

	DBGLOG(RLM, INFO,
		"[SAP_Test] u2FrameCtrl = 0x%x\n", prRxFrame->u2FrameCtrl);
	DBGLOG(RLM, INFO,
		"[SAP_Test] u2Duration = %u\n", prRxFrame->u2Duration);
	DBGLOG(RLM, INFO,
		"[SAP_Test] aucDestAddr = " MACSTR "\n",
		MAC2STR(prRxFrame->aucDestAddr));
	DBGLOG(RLM, INFO,
		"[SAP_Test] aucSrcAddr = " MACSTR "\n",
		MAC2STR(prRxFrame->aucSrcAddr));
	DBGLOG(RLM, INFO,
		"[SAP_Test] aucBSSID = " MACSTR "\n",
		MAC2STR(prRxFrame->aucBSSID));
	DBGLOG(RLM, INFO,
		"[SAP_Test] u2SeqCtrl = %u\n", prRxFrame->u2SeqCtrl);
	DBGLOG(RLM, INFO,
		"[SAP_Test] ucCategory = %u\n", prRxFrame->ucCategory);
	DBGLOG(RLM, INFO,
		"[SAP_Test] ucAction = %u\n", prRxFrame->ucAction);
	DBGLOG(RLM, INFO,
		"[SAP_Test] ucDialogToken = %u\n", prRxFrame->ucDialogToken);
	/* Measurement Report Elements */
	while (prSwRfb->u2PacketLen > u2TmpLen) {
		switch (pucOptInfo[0]) {
		case ELEM_ID_MEASUREMENT_REPORT:
			prMeasureReportIE =
				(struct IE_MEASUREMENT_REPORT *) &pucOptInfo[0];
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucId = %u\n",
				prMeasureReportIE->ucId);
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucLength = %u\n",
				prMeasureReportIE->ucLength);
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucToken = %u\n",
				prMeasureReportIE->ucToken);
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucReportMode = 0x%x\n",
				prMeasureReportIE->ucReportMode);
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucMeasurementType = 0x%x\n",
				prMeasureReportIE->ucMeasurementType);
			prBeaconReportIE =
				(struct RM_BCN_REPORT *)
				&prMeasureReportIE->aucReportFields[0];
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucRegulatoryClass = %d\n",
				prBeaconReportIE->ucRegulatoryClass);
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucChannel = %d\n",
				prBeaconReportIE->ucChannel);
			DBGLOG_MEM8(RLM, INFO,
				prBeaconReportIE->aucStartTime, 8);
			DBGLOG(RLM, INFO,
				"[SAP_Test] u2Duration = %d\n",
				prBeaconReportIE->u2Duration);
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucReportInfo = 0x%x\n",
				prBeaconReportIE->ucReportInfo);
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucRCPI = 0x%x\n",
				prBeaconReportIE->ucRCPI);
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucRSNI = 0x%x\n",
				prBeaconReportIE->ucRSNI);
			DBGLOG(RLM, INFO,
				"[SAP_Test] aucBSSID = " MACSTR "\n",
				MAC2STR(prBeaconReportIE->aucBSSID));
			DBGLOG(RLM, INFO,
				"[SAP_Test] ucAntennaID = %d\n",
				prBeaconReportIE->ucAntennaID);
			DBGLOG_MEM8(RLM, INFO,
				prBeaconReportIE->aucParentTSF, 4);
			DBGLOG_MEM8(RLM, INFO,
				prBeaconReportIE->aucOptElem,
				prMeasureReportIE->ucLength - 3 - 26);

			prBcnMeasureReport->u8ElemNum++;
			prBcnMeasureReport->uElemLen +=
				(prMeasureReportIE->ucLength + 2);
			kalMemCopy(pucReportElem,
				&prMeasureReportIE->ucId,
				prMeasureReportIE->ucLength + 2);
			break;
		default:
			DBGLOG(RLM, INFO, "[SAP_Test] not know\n");
			DBGLOG_MEM8(RLM, WARN,
				pucOptInfo, pucOptInfo[1] + 2);
			break;
		}
		pucOptInfo += (pucOptInfo[1] + 2);
		u2TmpLen += (pucOptInfo[1] + 2);
		pucReportElem += prMeasureReportIE->ucLength;
	}

	DBGLOG(RLM, INFO,
		"[SAP_Test] mStaMac = " MACSTR "\n",
		MAC2STR(prRxFrame->aucSrcAddr));
	DBGLOG(RLM, INFO,
		"[SAP_Test] u8ElemNum = %u\n",
		prBcnMeasureReport->u8ElemNum);
	DBGLOG(RLM, INFO,
		"[SAP_Test] uElemLen = %u\n",
		prBcnMeasureReport->uElemLen);
	DBGLOG_MEM8(RLM, INFO,
		prBcnMeasureReport->uElem,
		prBcnMeasureReport->uElemLen);

	i4Ret = MulAPAgentMontorSendMsg(
		EV_WLAN_MULTIAP_BEACON_METRICS_RESPONSE,
		prBcnMeasureReport, sizeof(*prBcnMeasureReport));
	if (i4Ret < 0)
		DBGLOG(AAA, ERROR,
			"EV_WLAN_MULTIAP_BEACON_METRICS_RESPONSE nl send msg failed!\n");

	kalMemFree(prBcnMeasureReport, VIR_MEM_TYPE,
		sizeof(struct T_MULTI_AP_BEACON_METRICS_RESP));
}
#endif /* CFG_AP_80211K_SUPPORT */

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief Init Transmit Power Envelope max TxPower limit to max TxPower
 *
 * \param[in] picMaxTxPwr : Pointer of max TxPower limit
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void rlmTxPwrEnvMaxPwrInit(int8_t *picMaxTxPwr)
{
	enum TX_PWR_ENV_MAX_TXPWR_BW_TYPE eBwType;

	for (eBwType = TX_PWR_ENV_MAX_TXPWR_BW20;
		eBwType < TX_PWR_ENV_MAX_TXPWR_BW_NUM; eBwType++) {
		picMaxTxPwr[eBwType] = MAX_TX_POWER;
	}
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to get BW shift by channel BW
 *
 * \param[in] eChannelWidth : Channel BW
 * \param[in] eSco : Channel extent parameter
 * \param[in] pucShift : Pinter of shift
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_NOT_SUPPORTED
 */
/*----------------------------------------------------------------------------*/
static uint32_t rlmTxPwrEnvGetBwShift(
	enum ENUM_CHANNEL_WIDTH eChannelWidth,
	enum ENUM_CHNL_EXT eSco,
	uint8_t *pucShift)
{
	if (pucShift == NULL)
		return WLAN_STATUS_INVALID_DATA;

	switch (eChannelWidth) {
	case CW_20_40MHZ:
		if (eSco == CHNL_EXT_SCA || eSco == CHNL_EXT_SCB) {
			/* BW40 */
			*pucShift = TX_PWR_ENV_BW_SHIFT_BW40;
		} else {
			/* BW20 */
			*pucShift = TX_PWR_ENV_BW_SHIFT_BW20;
		}
		break;
	case CW_80MHZ:
		*pucShift = TX_PWR_ENV_BW_SHIFT_BW80;
		break;
	case CW_160MHZ:
	case CW_80P80MHZ:
		*pucShift = TX_PWR_ENV_BW_SHIFT_BW160;
		break;
	default:
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief 1. This func is use to get Transmit Power Envelope max TxPower PSD
 *        2. Due to max TxPower PSD field in transmit Power Envelope is indicate
 *           the Nth primary channel. It will need CenterCh /PriCh /BW info to
 *           get the corresponding PSD TxPower limit
 *           - Nth primary channel = abs(CenterCh - BW Shift - PriCh) / 4
 *
 * \param[in] eChannelWidth : Channel BW
 * \param[in] eSco : Channel extent parameter
 * \param[in] ucSize : Indicate the quantity to compare
 * \param[in] ucCenterCh : Center channel
 * \param[in] ucPriCh : Primary channel
 * \param[in] prTxPwrEnvIE : Pointer of Tranmit Power Envelope IE content
 * \param[in] picMaxTxPwrPsd : Pointer of Max TxPower PSD
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
static uint32_t rlmTxPwrEnvGetMaxTxPwrPsd(
	enum ENUM_CHANNEL_WIDTH eChannelWidth,
	enum ENUM_CHNL_EXT eSco,
	uint8_t ucCenterCh,
	uint8_t ucPriCh,
	struct IE_TX_PWR_ENV_FRAME *prTxPwrEnvIE,
	int8_t *picMaxTxPwrPsd)
{
	uint8_t ucPriChIdx = 0;
	uint8_t ucBwShift = 0;
	uint8_t ucTxPwrEnvCnt = 0;
	uint8_t ucNumMaxTxPwr = 0;

	if (!picMaxTxPwrPsd)
		return WLAN_STATUS_INVALID_DATA;

	if (rlmTxPwrEnvGetBwShift(eChannelWidth, eSco, &ucBwShift)
		!= WLAN_STATUS_SUCCESS) {
		DBGLOG(RLM, ERROR,
			"Get PSD BW shift fail,ChBw[%d]Sco[%d]\n",
			eChannelWidth,
			eSco);
		return WLAN_STATUS_FAILURE;
	}

	ucTxPwrEnvCnt =
		TX_PWR_ENV_INFO_GET_TXPWR_COUNT(prTxPwrEnvIE->ucTxPwrInfo);

	if (ucTxPwrEnvCnt == 0) {
		*picMaxTxPwrPsd = prTxPwrEnvIE->aicMaxTxPwr[0];
	} else {
		/* Calculate Transmit Power Envelope max TxPower PSD index
		 * for target primary channel :
		 *     - abs(CenterCh - BwShift - PriCh) / 4
		 * Example :
		 *    - BW80  6G CenterCH=23 PriCH=21, idx = abs(23- 6-21)/4 = 1
		 *    - BW160 6G CenterCH=79 PriCH=89, idx = abs(79-14-89)/4 = 6
		 *    - BW80  5G CenterCH=58 PriCH=52, idx = abs(58- 6-52)/4 = 1
		 */
		if (ucCenterCh > (ucBwShift + ucPriCh))
			ucPriChIdx = (ucCenterCh - (ucBwShift + ucPriCh)) / 4;
		else
			ucPriChIdx = ((ucBwShift + ucPriCh) - (ucCenterCh)) / 4;

		/* In TxPwr PSD case , the value of Transmit Power Envelope
		 * Info Count represent the number of Max TxPwr field by
		 * following transfer func : (1 << (ucTxPwrEnvCount - 1))
		 *  ie.
		 *      |ucTxPwrEnvCount|Number of Max TxPwr field|
		 *      |      0        |             0           |
		 *      |      1        |             1           |
		 *      |      2        |             2           |
		 *      |      3        |             4           |
		 *      |      4        |             8           |
		 */
		ucNumMaxTxPwr = (1 << (ucTxPwrEnvCnt - 1));

		/* Sanity check for TxPwrEnv count */
		if (ucNumMaxTxPwr > TX_PWR_ENV_INFO_TXPWR_COUNT_MAX ||
			ucPriChIdx >= ucNumMaxTxPwr) {
			DBGLOG(RLM, ERROR,
			"Get max TxPwr PSD idx fail,MaxNum[%d]PriCh_Idx[%d]\n",
			ucNumMaxTxPwr,
			ucPriChIdx);
			return WLAN_STATUS_FAILURE;
		}

		*picMaxTxPwrPsd = prTxPwrEnvIE->aicMaxTxPwr[ucPriChIdx];
	}
	DBGLOG(RLM, TRACE,
		"TPE PSD,BW[%d]Sco[%d]BwShif[%d]PriCh[%d]CenCh[%d]Idx[%d]Cnt[%d]PSD[%d]\n",
		eChannelWidth,
		eSco,
		ucBwShift,
		ucPriCh,
		ucCenterCh,
		ucPriChIdx,
		ucTxPwrEnvCnt,
		*picMaxTxPwrPsd);

	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to get TxPower delta to transfer PSD(dBm/Hz) to
 *        TxPower(dBm)
 *
 * \param[in] eBwType : Transmit Power Envelope max TxPower limt BW type
 * \param[in] pucTxPwrDelta : Pointer of TxPower delta
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
static uint32_t rlmTxPwrEnvGetPwrDelta(
	enum TX_PWR_ENV_MAX_TXPWR_BW_TYPE eBwType,
	uint8_t *pucTxPwrDelta)
{
	switch (eBwType) {
	case TX_PWR_ENV_MAX_TXPWR_BW20:
		*pucTxPwrDelta = TX_PWR_ENV_PSD_TRANS_DBM_BW20;
		break;
	case TX_PWR_ENV_MAX_TXPWR_BW40:
		*pucTxPwrDelta = TX_PWR_ENV_PSD_TRANS_DBM_BW40;
		break;
	case TX_PWR_ENV_MAX_TXPWR_BW80:
		*pucTxPwrDelta = TX_PWR_ENV_PSD_TRANS_DBM_BW80;
		break;
	case TX_PWR_ENV_MAX_TXPWR_BW160:
		*pucTxPwrDelta = TX_PWR_ENV_PSD_TRANS_DBM_BW160;
		break;
	default:
		return WLAN_STATUS_NOT_SUPPORTED;
	}
	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief 1. This func is use to get Transmit Power Envelope max TxPower PSD and
 *        calculate TxPower limit dBm from PSD
 *        2. Due to max TxPower PSD field in transmit Power Envelope is indicate
 *           the Nth primary channel. It will need CenterCh /PriCh /BW info to
 *           get the corresponding PSD TxPower limit
 *           - Nth primary channel = abs(CenterCh - BW Shift - PriCh) / 4
 *        3. The PSD to TxPower dBm transfer func is as below :
 *           - TxPower(dBm) = PSD(dBm/Hz) + 10*log(BW)
 *
 * \param[in] eChannelWidth : Channel BW
 * \param[in] eSco : Channel extent parameter
 * \param[in] ucSize : Indicate the quantity to compare
 * \param[in] ucCenterCh : Center channel
 * \param[in] ucPriCh : Primary channel
 * \param[in] prTxPwrEnvIE : Pointer of Tranmit Power Envelope IE content
 * \param[in] picMaxTxPwr : Pointer of Max TxPower
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmTxPwrEnvMaxTxPwrCalcByPsd(
	enum ENUM_CHANNEL_WIDTH eChannelWidth,
	enum ENUM_CHNL_EXT eSco,
	uint8_t ucCenterCh,
	uint8_t ucPriCh,
	struct IE_TX_PWR_ENV_FRAME *prTxPwrEnvIE,
	int8_t *picMaxTxPwr)
{
	enum TX_PWR_ENV_MAX_TXPWR_BW_TYPE eBwType;
	int8_t icMaxTxPwrPsd = 0;
	uint8_t ucTxPwrDelta = 0;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	if (!prTxPwrEnvIE)
		return WLAN_STATUS_INVALID_DATA;

	/* 1. Get correct TxPower limit */
	u4Status = rlmTxPwrEnvGetMaxTxPwrPsd(eChannelWidth,
			eSco,
			ucCenterCh,
			ucPriCh,
			prTxPwrEnvIE,
			&icMaxTxPwrPsd);

	/* 2. Convert TxPower limit PSD to BW Power limit
	 *    - Max TxPwr(dBm) = PSD(dBm/Hz) + 10*log(BW)
	 *    We will also convert power LSB = 0.5dBm
	 */
	if (u4Status == WLAN_STATUS_SUCCESS) {
		for (eBwType = TX_PWR_ENV_MAX_TXPWR_BW20;
			eBwType < TX_PWR_ENV_MAX_TXPWR_BW_NUM; eBwType++) {
			rlmTxPwrEnvGetPwrDelta(eBwType, &ucTxPwrDelta);

			/* Note the icMaxTxPwrPsd is LSB = 0.5dBm and
			 * ucTxPwrDelta is already convert to LSB = 0.5dBm
			 */
			picMaxTxPwr[eBwType] = icMaxTxPwrPsd + ucTxPwrDelta;
		}
	}
	return u4Status;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Update new TxPower limit, the update criteria is only when the
 *        TxPower limit  have been change
 *        It will set a flag var(pfgIsChange) to record whether the TxPower
 *        limit have been change
 *
 * \param[in] prAdapter : Pointer of adapter
 * \param[in] picTarget : Pointer of new TxPower limit
 * \param[in] picCompare : Pointer of old TxPower limit
 * \param[in] ucNum : Indicate the quantity to update
 * \param[in] pfgIsChange : The flag to recored whether the TxPower limit have
 *                          been change
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void rlmTxPwrEnvMaxPwrUpdateArbi(
	struct ADAPTER *prAdapter,
	int8_t *picTarget,
	int8_t *picCompare,
	uint8_t ucNum,
	uint8_t *pfgIsChange)
{
	enum TX_PWR_ENV_MAX_TXPWR_BW_TYPE eBwType;
	int8_t icMinPwrLmt = 0;

	/* Sanity check for null pointer */
	if (!prAdapter || !picTarget || !picCompare || !pfgIsChange)
		return;

	/* Align IEEE spec, to avoid AP send power limit info too low */
	icMinPwrLmt = prAdapter->rWifiVar.icTxPwrEnvLmtMin;

	/* Sanity check for quantity to update */
	if (ucNum > TX_PWR_ENV_MAX_TXPWR_BW_NUM)
		ucNum = TX_PWR_ENV_MAX_TXPWR_BW_NUM;

	for (eBwType = TX_PWR_ENV_MAX_TXPWR_BW20; eBwType < ucNum; eBwType++) {
		if (picTarget[eBwType] > picCompare[eBwType]) {
			picTarget[eBwType] = picCompare[eBwType];
			*pfgIsChange = TRUE;
		}

		/* Sanity check TxPower boundary */
		if (picTarget[eBwType] > MAX_TX_POWER) {
			DBGLOG(RLM, INFO,
			"TPE PwrLmt too big, use default[%d]Lmt[%d]BW[%d]\n",
			MAX_TX_POWER,
			picTarget[eBwType],
			eBwType);

			picTarget[eBwType] = MAX_TX_POWER;
		}

		else if (picTarget[eBwType] < icMinPwrLmt) {
			DBGLOG(RLM, INFO,
			"TPE PwrLmt too low, use default[%d]Lmt[%d]BW[%d]\n",
			icMinPwrLmt,
			picTarget[eBwType],
			eBwType);

			picTarget[eBwType] = icMinPwrLmt;
		}

		DBGLOG(RLM, TRACE, "Final TPE BW[%d]Lmt[%d]Change[%d]\n",
				eBwType,
				picTarget[eBwType],
				*pfgIsChange);
	}
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use send Tranmit Power Envelope max TxPower limit to FW
 *
 * \param[in] prAdapter : Pointer to adapter
 * \param[in] eBand : RF Band index
 * \param[in] ucPriCh : Primary Channel
 * \param[in] picTxPwrEnvMaxPwr : Pointer to Tranmit Power Envelope max TxPower
 *                                limit
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void rlmTxPwrEnvMaxPwrSend(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucPriCh,
	uint8_t ucPwrLmtNum,
	int8_t *picTxPwrEnvMaxPwr,
	uint8_t fgPwrLmtEnable)
{
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd = NULL;
	struct CMD_CHANNEL_POWER_LIMIT_TX_PWR_ENV *prTxPwrEnvPwrLmt = NULL;
	enum TX_PWR_ENV_MAX_TXPWR_BW_TYPE eBwType;
	uint32_t u4CmdSize = sizeof(struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT);
	uint32_t rStatus;

	/* Sanity check for null pointer */
	if (picTxPwrEnvMaxPwr == NULL)
		return;

	prCmd = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdSize);
	if (!prCmd) {
		DBGLOG(RLM, ERROR, "TxPwr Envelope: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmd, u4CmdSize);

	/* Fill in CMD content */
	prCmd->ucCategoryId = POWER_LIMIT_TX_PWR_ENV_CTRL;
	prTxPwrEnvPwrLmt = &prCmd->u.rTxPwrEnvPwrLmt;
	prTxPwrEnvPwrLmt->fgPwrLmtEnable = fgPwrLmtEnable;
	prTxPwrEnvPwrLmt->ucBand = (uint8_t)eBand;
	prTxPwrEnvPwrLmt->ucPriCh = ucPriCh;
	prTxPwrEnvPwrLmt->ucPwrLmtNum = ucPwrLmtNum;
	for (eBwType = TX_PWR_ENV_MAX_TXPWR_BW20;
			eBwType < ucPwrLmtNum; eBwType++) {
		prTxPwrEnvPwrLmt->aicMaxTxPwrLmt[eBwType]
			= picTxPwrEnvMaxPwr[eBwType];
	}

	DBGLOG(RLM, INFO,
		"TPE Send:En[%d]B[%d]PriCh[%d]Num[%d]PwrLmtBW20[%d]BW40[%d]BW80[%d]BW160[%d]\n",
		prTxPwrEnvPwrLmt->fgPwrLmtEnable,
		prTxPwrEnvPwrLmt->ucBand,
		prTxPwrEnvPwrLmt->ucPriCh,
		prTxPwrEnvPwrLmt->ucPwrLmtNum,
		prTxPwrEnvPwrLmt->aicMaxTxPwrLmt[TX_PWR_ENV_MAX_TXPWR_BW20],
		prTxPwrEnvPwrLmt->aicMaxTxPwrLmt[TX_PWR_ENV_MAX_TXPWR_BW40],
		prTxPwrEnvPwrLmt->aicMaxTxPwrLmt[TX_PWR_ENV_MAX_TXPWR_BW80],
		prTxPwrEnvPwrLmt->aicMaxTxPwrLmt[TX_PWR_ENV_MAX_TXPWR_BW160]);

	rStatus = wlanSendSetQueryCmd(prAdapter, /* prAdapter */
		CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
		TRUE,	/* fgSetQuery */
		FALSE,	/* fgNeedResp */
		FALSE,	/* fgIsOid */
		NULL,	/* pfCmdDoneHandler */
		NULL,	/* pfCmdTimeoutHandler */
		u4CmdSize, /* u4SetQueryInfoLen */
		(uint8_t *) prCmd, /* pucInfoBuffer */
		NULL,	/* pvSetQueryBuffer */
		0	/* u4SetQueryBufferLen */
		);

	if (rStatus != WLAN_STATUS_PENDING)
		DBGLOG(RLM, ERROR, "Send TxPwrEnv error 0x%08x\n", rStatus);
	else
		DBGLOG(RLM, INFO, "Send TxPwrEnv success 0x%08x\n", rStatus);
err:
	cnmMemFree(prAdapter, prCmd);
}
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to handle 6G power mode for TPE TxPower limit.
 *        EX : Station should set power limit 6dBm below the standard power AP
 *             transmit power authorized by AFC system.
 *
 * \param[in] prAdapter : Pointer of adapter.
 * \param[in] e6GPwrMode : Enum of 6G power mode
 * \param[in] ucNum : Indicate the quantity of TPE TxPower limit is valid.
 * \param[in] picTpeLmt : Pointer of TPE Max TxPower limit.
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmTxPwrEnv6GPwrModeHdler(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrMode,
	uint8_t ucNum,
	int8_t *picTpeLmt)
{
	enum TX_PWR_ENV_MAX_TXPWR_BW_TYPE eBw;

	if (e6GPwrMode == PWR_MODE_6G_SP) {
		if (prAdapter->rWifiVar.u2CountryCode != COUNTRY_CODE_US) {
			/* So far only FCC need handle TPE limit for SP AP */
			return WLAN_STATUS_SUCCESS;
		}

		if (prAdapter->rWifiVar.fgSpPwrLmtBackoff != FEATURE_ENABLED) {
			/* When AP handle 6dbm backoff, client no need to
			 * base on TPE power limit - 6dBm
			 */
			return WLAN_STATUS_SUCCESS;
		}

		/* 1. For Standard power mode AP, AFC system will send
		 * the power limit to AP and AP will send this power
		 * limit to station by Transmit Power Envelope.
		 *
		 * 2. Follow FCC regulation, Station should set power limit
		 * 6dBm below the standard power AP transmit power authorized
		 * by AFC system.
		 */
		for (eBw = TX_PWR_ENV_MAX_TXPWR_BW20; eBw < ucNum; eBw++) {

			if (picTpeLmt[eBw] < TX_PWR_ENV_INT8_MIN + 12) {
				/* To avoid over flow */
				picTpeLmt[eBw] = TX_PWR_ENV_INT8_MIN;
			} else {
				picTpeLmt[eBw] =  picTpeLmt[eBw] - 12;
			}

			DBGLOG(RLM, INFO, "TPE handle for SP,BW[%d]Lmt[%d]\n",
					eBw,
					picTpeLmt[eBw]);
		}
	}

	return WLAN_STATUS_SUCCESS;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to update TxPwr Envelope if need.
 *        1. It will translate IE content if the content is represent as PSD
 *        2. Update the TxPwr Envelope TxPower limit to BSS_DESC if the IE
 *           content is smaller than the current exit setting.
 *        3. If the TxPower limit have update, it will send cmd to FW to reset
 *           TxPower limit
 *
 * \param[in] prAdapter : Pointer of adapter
 * \param[in] prBssDesc : Pointer ofBSS desription
 * \param[in] eHwBand : RF Band
 * \param[in] prTxPwrEnvIE : Pointer of TxPwer Envelope IE
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmTxPwrEnvMaxPwrUpdate(
	struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc,
	enum ENUM_BAND eHwBand,
	struct IE_TX_PWR_ENV_FRAME *prTxPwrEnvIE)
{
	uint8_t ucTxPwrEnvIntrpt = 0;
	uint8_t ucPwrLmtNum = 0;
	enum TX_PWR_ENV_MAX_TXPWR_BW_TYPE eBwType;
	int8_t aicTxPwrEnvMaxTxPwr[TX_PWR_ENV_MAX_TXPWR_BW_NUM] = {0};
	uint8_t fgIsTxPwrEnvChange = FALSE;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	/* sanity check null pointer */
	if (!prAdapter || !prBssDesc || !prTxPwrEnvIE) {
		DBGLOG(RLM, WARN, "Update TxPwrEnv fail: null pointer\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	/* If AP is not operation in 6GHz and not extended spectrum management
	 * capable, STA no need to refer to TxPwr Env IE
	 */
	if ((prBssDesc->fgExtSpecMgmtCap == FALSE)
#if (CFG_SUPPORT_WIFI_6G == 1)
		 && (eHwBand != BAND_6G)
#endif
	) {
		DBGLOG(RLM, TRACE,
			"Skip TxPwrEnv update,band[%d]SpecMgmt[%d]\n",
			eHwBand,
			prBssDesc->fgExtSpecMgmtCap);
		return WLAN_STATUS_SUCCESS;
	}

	rlmTxPwrEnvMaxPwrInit(aicTxPwrEnvMaxTxPwr);

	ucTxPwrEnvIntrpt
		= TX_PWR_ENV_INFO_GET_TXPWR_INTRPT(prTxPwrEnvIE->ucTxPwrInfo);

	if (ucTxPwrEnvIntrpt == TX_PWR_ENV_LOCAL_EIRP
		|| ucTxPwrEnvIntrpt == TX_PWR_ENV_REG_CLIENT_EIRP) {

		ucPwrLmtNum = TX_PWR_ENV_INFO_GET_TXPWR_COUNT(
					prTxPwrEnvIE->ucTxPwrInfo) + 1;

		if (ucPwrLmtNum > TX_PWR_ENV_MAX_TXPWR_BW_NUM)
			ucPwrLmtNum = TX_PWR_ENV_MAX_TXPWR_BW_NUM;

		/* Direct copy Transmit Power Envelope content */
		for (eBwType = TX_PWR_ENV_MAX_TXPWR_BW20;
			eBwType < ucPwrLmtNum; eBwType++) {
			aicTxPwrEnvMaxTxPwr[eBwType]
				= prTxPwrEnvIE->aicMaxTxPwr[eBwType];
		}

	} else if (ucTxPwrEnvIntrpt == TX_PWR_ENV_LOCAL_EIRP_PSD
		|| ucTxPwrEnvIntrpt == TX_PWR_ENV_REG_CLIENT_EIRP_PSD) {

		/* Convert TxPower limit PSD to BW TxPower limit first
		 * and store in the aicTxPwrEnvMaxTxPwr
		 */
		u4Status = rlmTxPwrEnvMaxTxPwrCalcByPsd(
				prBssDesc->eChannelWidth,
				prBssDesc->eSco,
				prBssDesc->ucCenterFreqS1,
				prBssDesc->ucChannelNum,
				prTxPwrEnvIE,
				aicTxPwrEnvMaxTxPwr);

		ucPwrLmtNum = TX_PWR_ENV_MAX_TXPWR_BW_NUM;

		if (u4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(RLM, WARN,
				"Update TxPwrEnv fail 0x%08x\n", u4Status);
		}

	} else {
		DBGLOG(RLM, WARN, "TxPwrEnv Itrp[%d] not support\n",
			ucTxPwrEnvIntrpt);
		u4Status = WLAN_STATUS_NOT_SUPPORTED;
	}

	if (u4Status == WLAN_STATUS_SUCCESS) {

		for (eBwType = TX_PWR_ENV_MAX_TXPWR_BW20;
			eBwType < ucPwrLmtNum; eBwType++) {
			DBGLOG(RLM, TRACE,
				"Parse TPE,Band[%d]Itpt[%d]BW[%d]Lmt\n",
				eHwBand,
				ucTxPwrEnvIntrpt,
				eBwType,
				aicTxPwrEnvMaxTxPwr[eBwType]);
		}

		prBssDesc->fgIsTxPwrEnvPresent = TRUE;
		/* Since there will receive the multiple Transmit Power Envelope
		 * IE, different IE may content different size of TxPower Limit
		 * number, we should set the biggest one.
		 */
		if (ucPwrLmtNum > prBssDesc->ucTxPwrEnvPwrLmtNum)
			prBssDesc->ucTxPwrEnvPwrLmtNum = ucPwrLmtNum;

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		if (eHwBand == BAND_6G) {
			rlmTxPwrEnv6GPwrModeHdler(
				prAdapter,
				prBssDesc->e6GPwrMode,
				ucPwrLmtNum,
				aicTxPwrEnvMaxTxPwr);
		}
#endif
		/* Set minimum TxPower limit */
		rlmTxPwrEnvMaxPwrUpdateArbi(
			prAdapter,
			prBssDesc->aicTxPwrEnvMaxTxPwr,
			aicTxPwrEnvMaxTxPwr,
			prBssDesc->ucTxPwrEnvPwrLmtNum,
			&fgIsTxPwrEnvChange);
	} else {
		return u4Status;
	}

	if (prBssDesc->fgIsConnected && fgIsTxPwrEnvChange == TRUE) {
		rlmTxPwrEnvMaxPwrSend(
			prAdapter,
			eHwBand,
			prBssDesc->ucChannelNum,
			prBssDesc->ucTxPwrEnvPwrLmtNum,
			prBssDesc->aicTxPwrEnvMaxTxPwr,
			TRUE);
	}

	return u4Status;
}
#endif /* CFG_SUPPORT_TX_PWR_ENV */

void
rlmSendChannelSwitchFrame(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	struct LINK *prClientList;
	struct STA_RECORD *prCurrStaRec;

	if (!prBssInfo || !IS_BSS_APGO(prBssInfo))
		return;

	prClientList = &prBssInfo->rStaRecOfClientList;

	LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
			    struct STA_RECORD) {
		if (!prCurrStaRec)
			break;

		DBGLOG(P2P, INFO,
			"bss[%d] " MACSTR ", sta[%d][%d] " MACSTR
			", ecsa: %d\n",
			prBssInfo->ucBssIndex,
			MAC2STR(prBssInfo->aucOwnMacAddr),
			prCurrStaRec->ucIndex,
			prCurrStaRec->ucWlanIndex,
			MAC2STR(prCurrStaRec->aucMacAddr),
			prCurrStaRec->fgEcsaCapable);

		__rlmSendChannelSwitchFrame(prAdapter, prBssInfo,
					    prCurrStaRec);

		__rlmSendExChannelSwitchFrame(prAdapter, prBssInfo,
					    prCurrStaRec);
	}
}

uint32_t rlmCalculateTpeIELen(struct ADAPTER *prAdapter,
			      uint8_t ucBssIndex, struct STA_RECORD *prStaRec)
{
#if CFG_ENABLE_WIFI_DIRECT
	struct BSS_INFO *prBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo;

	if (!prAdapter)
		return 0;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgSapAddTPEIE))
		return 0;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo || !IS_BSS_APGO(prBssInfo))
		return 0;

	prP2pSpecificBssInfo =
		prAdapter->rWifiVar
			.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

	if (!prP2pSpecificBssInfo)
		return 0;

	return prP2pSpecificBssInfo->u2TpeIeLen;
#else
	return 0;
#endif
}

void rlmGenerateTpeIE(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo)
{
#if CFG_ENABLE_WIFI_DIRECT
	struct BSS_INFO *prBssInfo;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo;
	uint8_t *pucBuffer;

	if (!prAdapter || !prMsduInfo)
		return;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgSapAddTPEIE))
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (!prBssInfo || !IS_BSS_APGO(prBssInfo))
		return;

	prP2pSpecificBssInfo =
		prAdapter->rWifiVar
			.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

	if (!prP2pSpecificBssInfo || !prP2pSpecificBssInfo->u2TpeIeLen)
		return;

	pucBuffer = (uint8_t *) ((uintptr_t)prMsduInfo->prPacket +
		(uintptr_t)prMsduInfo->u2FrameLength);

	kalMemCopy(pucBuffer,
		prP2pSpecificBssInfo->aucTpeIeBuffer,
		prP2pSpecificBssInfo->u2TpeIeLen);
	prMsduInfo->u2FrameLength +=
		prP2pSpecificBssInfo->u2TpeIeLen;
#endif
}

enum ENUM_MAX_BANDWIDTH_SETTING
rlmVhtBw2Bw(uint8_t ucVhtBw, enum ENUM_CHNL_EXT eSco)
{
	enum ENUM_MAX_BANDWIDTH_SETTING eBw;

	switch (ucVhtBw) {
	case VHT_OP_CHANNEL_WIDTH_320_1:
		eBw = MAX_BW_320_1MHZ;
		break;
	case VHT_OP_CHANNEL_WIDTH_320_2:
		eBw = MAX_BW_320_2MHZ;
		break;
	case VHT_OP_CHANNEL_WIDTH_80P80:
		eBw = MAX_BW_80_80_MHZ;
		break;
	case VHT_OP_CHANNEL_WIDTH_160:
		eBw = MAX_BW_160MHZ;
		break;
	case VHT_OP_CHANNEL_WIDTH_80:
		eBw = MAX_BW_80MHZ;
		break;
	case VHT_OP_CHANNEL_WIDTH_20_40:
		if (eSco == CHNL_EXT_SCN)
			eBw = MAX_BW_20MHZ;
		else
			eBw = MAX_BW_40MHZ;
		break;
	default:
		eBw = MAX_BW_20MHZ;
		break;
	}

	return eBw;
}
