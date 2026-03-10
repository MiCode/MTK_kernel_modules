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
/*
 ** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/rlm.c#3
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

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
enum ENUM_OP_NOTIFY_STATE_T {
	OP_NOTIFY_STATE_KEEP = 0, /* Won't change OP mode */
	OP_NOTIFY_STATE_SENDING,  /* Sending OP notification frame */
	OP_NOTIFY_STATE_SUCCESS,  /* OP notification Tx success */
	OP_NOTIFY_STATE_FAIL,     /* OP notification Tx fail(over retry limit)*/
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
#if CFG_SUPPORT_RXSMM_WHITELIST
uint8_t Rxsmm_Iot_Whitelist[VENDOR_OUI_RXSMM_LIST_NUM]
	[VENDOR_OUI_RXSMM_OUI_IE_NUM] = {
/*	{0x00, 0x0C, 0xE7}, */
};
#endif

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
				     struct BSS_INFO *prBssInfo, uint8_t *pucIE,
				     uint16_t u2IELength);

static u_int8_t rlmRecBcnFromNeighborForClient(struct ADAPTER *prAdapter,
					       struct BSS_INFO *prBssInfo,
					       struct SW_RFB *prSwRfb,
					       uint8_t *pucIE,
					       uint16_t u2IELength);

static u_int8_t rlmRecBcnInfoForClient(struct ADAPTER *prAdapter,
				       struct BSS_INFO *prBssInfo,
				       struct SW_RFB *prSwRfb, uint8_t *pucIE,
				       uint16_t u2IELength);

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
static void rlmRecOpModeBwForClient(uint8_t ucVhtOpModeChannelWidth,
				    struct BSS_INFO *prBssInfo);

static void rlmRecHtOpForClient(struct IE_HT_OP *prHtOp,
			struct BSS_INFO *prBssInfo,
			uint8_t *pucPrimaryChannel);

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

	/* Note: assume struct TIMER structures are reset to zero or stopped
	 * before invoking this function.
	 */

	/* Initialize OBSS FSM */
	rlmObssInit(prAdapter);

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

	for (i = 0; i < prAdapter->ucHwBssIdNum; i++) {
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

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	pucBuffer =
		(uint8_t *)((uintptr_t)prMsduInfo->prPacket +
			prMsduInfo->u2FrameLength);

	POWER_CAP_IE(pucBuffer)->ucId = ELEM_ID_PWR_CAP;
	POWER_CAP_IE(pucBuffer)->ucLength = ELEM_MAX_LEN_POWER_CAP;
	POWER_CAP_IE(pucBuffer)->cMinTxPowerCap = RLM_MIN_TX_PWR;
	POWER_CAP_IE(pucBuffer)->cMaxTxPowerCap = RLM_MAX_TX_PWR;

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

	if (RLM_NET_IS_11N(prBssInfo) && (ucPhyTypeSet & PHY_TYPE_SET_802_11N))
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

	if (IS_BSS_APGO(prBssInfo) &&
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgP2pGcCsa)) {
		MTK_OUI_IE(pucBuffer)->aucCapability[1] |=
			MTK_SYNERGY_CAP_SUPPORT_GC_CSA;
		DBGLOG(P2P, TRACE, "Add gc csa capa\n");
	}

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
u_int8_t rlmParseCheckMTKOuiIE(struct ADAPTER *prAdapter, uint8_t *pucBuf,
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

#if CFG_SUPPORT_RXSMM_WHITELIST
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to check MTK Vendor Specific OUI
 *
 *
 * @return true:  correct MTK OUI
 *             false: incorrect MTK OUI
 */
/*----------------------------------------------------------------------------*/
u_int8_t rlmParseCheckRxsmmOuiIE(struct ADAPTER *prAdapter, uint8_t *pucBuf,
			       u_int8_t *pfgRxsmmEnable)
{
	uint8_t u1RxsmmListIdx = 0;
	struct IE_MTK_OUI *prMtkOuiIE = (struct IE_MTK_OUI *)NULL;

	if ((prAdapter == NULL) || (pucBuf == NULL))
		return FALSE;

	*pfgRxsmmEnable = FALSE;

	for (u1RxsmmListIdx = 0;
		u1RxsmmListIdx < VENDOR_OUI_RXSMM_LIST_NUM;
		u1RxsmmListIdx++) {

		prMtkOuiIE = (struct IE_MTK_OUI *)pucBuf;

		if (prAdapter->rWifiVar.ucMtkOui == FEATURE_DISABLED)
			continue;
		else if (IE_LEN(pucBuf) < ELEM_MIN_LEN_MTK_OUI)
			continue;
		else if (prMtkOuiIE->aucOui[0] !=
			Rxsmm_Iot_Whitelist[u1RxsmmListIdx][0] ||
			 prMtkOuiIE->aucOui[1] !=
			 Rxsmm_Iot_Whitelist[u1RxsmmListIdx][1] ||
			 prMtkOuiIE->aucOui[2] !=
			 Rxsmm_Iot_Whitelist[u1RxsmmListIdx][2])
			continue;

		*pfgRxsmmEnable = TRUE;

	}

	DBGLOG(QM, INFO, "RxSMM: RxSMM enable = %d\n", *pfgRxsmmEnable);

	return TRUE;
} /* rlmParseCheckRxsmmOuiIE */
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
void rlmGenerateCsaIE(struct ADAPTER *prAdapter, struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucBuffer;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	if (prAdapter->rWifiVar.fgCsaInProgress) {
		if (prMsduInfo->ucBssIndex !=
			p2pFuncGetCsaBssIndex())
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

		/* Fill Secondary channel offset IE */
		pucBuffer += IE_SIZE(pucBuffer);

		SEC_OFFSET_IE(pucBuffer)->ucId = ELEM_ID_SCO;
		SEC_OFFSET_IE(pucBuffer)->ucLength = 1;
		SEC_OFFSET_IE(pucBuffer)->ucSecondaryOffset =
			prAdapter->rWifiVar.ucSecondaryOffset;

		prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);

		/* Fill Wide Bandwidth Channel Switch IE */
		pucBuffer += IE_SIZE(pucBuffer);

		WIDE_BW_IE(pucBuffer)->ucId = ELEM_ID_WIDE_BAND_CHANNEL_SWITCH;
		WIDE_BW_IE(pucBuffer)->ucLength = 3;
		WIDE_BW_IE(pucBuffer)->ucNewChannelWidth =
			prAdapter->rWifiVar.ucNewChannelWidth;
		WIDE_BW_IE(pucBuffer)->ucChannelS1 =
			prAdapter->rWifiVar.ucNewChannelS1;
		WIDE_BW_IE(pucBuffer)->ucChannelS2 =
			prAdapter->rWifiVar.ucNewChannelS2;

		prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);

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

	fg40mAllowed = prBssInfo->fgAssoc40mBwAllowed;

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

	prHtCap->ucAmpduParam = AMPDU_PARAM_DEFAULT_VAL;

	prSupMcsSet = &prHtCap->rSupMcsSet;
	kalMemZero((void *)&prSupMcsSet->aucRxMcsBitmask[0],
		   SUP_MCS_RX_BITMASK_OCTET_NUM);

	for (ucIdx = 0;
	     ucIdx < wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);
	     ucIdx++)
		prSupMcsSet->aucRxMcsBitmask[ucIdx] = BITS(0, 7);

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

	fg40mAllowed = prBssInfo->fgAssoc40mBwAllowed;
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
			DBGLOG_MEM8(SAA, INFO, extCapConn, extCapIeLen);
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

	DBGLOG(RLM, TRACE, "prExtCap->ucLength = %d!", prExtCap->ucLength);

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


void rlmGenerateHtTPEIE(
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

	if (RLM_NET_IS_11N(prBssInfo) &&
		(ucPhyTypeSet & PHY_TYPE_SET_802_11N)) {
		struct IE_HT_TPE *prHtTpe;

		prHtTpe = (struct IE_HT_TPE *)
			(((uint8_t *)prMsduInfo->prPacket) +
			prMsduInfo->u2FrameLength);

		prHtTpe->ucId = ELEM_ID_PWR_CONSTRAINT;
		prHtTpe->ucLength =
			sizeof(struct IE_HT_TPE) - ELEM_HDR_LEN;
		prHtTpe->u8TxPowerInfo = 3;

		prMsduInfo->u2FrameLength += IE_SIZE(prHtTpe);
	}
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
					   FALSE);
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
#if CFG_SUPPORT_DBDC
		ucOpModeBw = cnmGetDbdcBwCapability(prAdapter,
						    prBssInfo->ucBssIndex);
#else
		ucOpModeBw = cnmGetBssMaxBw(prAdapter,
						    prBssInfo->ucBssIndex);
#endif
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
		uint8_t ucMcsMap;

		if (i < wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex))
			ucMcsMap = VHT_CAP_INFO_MCS_MAP_MCS9;
		else
			ucMcsMap = VHT_CAP_INFO_MCS_NOT_SUPPORTED;

		prVhtSupportedMcsSet->u2RxMcsMap |= (ucMcsMap << ucOffset);
		prVhtSupportedMcsSet->u2TxMcsMap |= (ucMcsMap << ucOffset);
	}

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
void rlmTransferHe6gOpInfor(uint8_t ucChannelNum,
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
	rlmModifyHE6GBwPara(*pucChannelWidth,
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

void rlmModifyHE6GBwPara(uint8_t ucHe6gChannelWidth,
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
			ucS1Modify = nicGetHe6gS1(ucHe6gPrimaryChannel,
				ucHe6gChannelWidth);

			DBGLOG(RLM, WARN,
				"S1/S2 for 6G BW160 is out of spec, S1[%d->%d] S2[%d->0]\n",
				ucS1Origin, ucS1Modify, ucS2Origin);
		}

		*pucHe6gChannelFrequencyS1 = ucS1Modify;
		*pucHe6gChannelFrequencyS2 = 0;
	} else if (ucHe6gChannelWidth == CW_80MHZ) {
		ucS1Modify = nicGetHe6gS1(ucHe6gPrimaryChannel,
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

#if CFG_SUPPORT_DBDC
	ucMaxBandwidth = cnmGetDbdcBwCapability(prAdapter, ucBssIndex);
#else
	ucMaxBandwidth = cnmGetBssMaxBw(prAdapter, ucBssIndex);
#endif
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
		} else { /* BW80, BW160, BW80P80 */
			/*
			 * Note that we should make sure S1 is non-zeno
			 * before changing BW / S1. Clients may want
			 * to stick bandwidth on CW_20_40MHZ even we
			 * have more bandwidth capability.
			 */
			if (ucS1Origin) {
				*peChannelWidth = (ucMaxBandwidth - ucOffset);

				/* modify S1 for Bandwidth 160 -> 80 or
				 * 320 -> 160
				 */
				if ((ucCurrentBandwidth == MAX_BW_160MHZ &&
					ucMaxBandwidth == MAX_BW_80MHZ) ||
				    ((ucCurrentBandwidth == MAX_BW_320_1MHZ ||
				      ucCurrentBandwidth == MAX_BW_320_2MHZ) &&
					ucMaxBandwidth == MAX_BW_160MHZ)) {
					*pucS1 = nicGetS1(prBssInfo->eBand,
						*pucPrimaryCh,
						*peChannelWidth);
				}
			}
		}

		if (eChBwOrigin != *peChannelWidth) {
			DBGLOG(RLM, INFO,
				"Change BW(VHT)[%d->%d], S1[%d->%d]\n",
				eChBwOrigin, *peChannelWidth,
				ucS1Origin, *pucS1);
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
void rlmFillVhtOpInfoByBssOpBw(struct BSS_INFO *prBssInfo, uint8_t ucBssOpBw)
{
	if (!prBssInfo) {
		DBGLOG(RLM, WARN, "no bssinfo\n");
		return;
	}

	prBssInfo->ucVhtChannelWidth =
		rlmGetVhtOpBwByBssOpBw(ucBssOpBw);
	prBssInfo->ucVhtChannelFrequencyS1 = nicGetS1(
		prBssInfo->eBand,
		prBssInfo->ucPrimaryChannel,
		prBssInfo->ucVhtChannelWidth);
	prBssInfo->ucVhtChannelFrequencyS2 = 0;
}

void rlmParseMtkOui(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct BSS_INFO *prBssInfo,
	uint8_t *pucIE)
{
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
	uint8_t *aucCapa = MTK_OUI_IE(pucIE)->aucCapability;
	uint8_t *ie, *sub;
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

			DBGLOG_MEM8(RLM, TRACE, ie, IE_SIZE(ie));
			if (IE_SIZE(prPreWifi7) <
			    sizeof(struct IE_MTK_PRE_WIFI7))
				return;

			DBGLOG(RLM, TRACE, "MTK_OUI_PRE_WIFI7 %d.%d",
				prPreWifi7->ucVersion1, prPreWifi7->ucVersion0);

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
				     struct BSS_INFO *prBssInfo, uint8_t *pucIE,
				     uint16_t u2IELength)
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
	struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams;
	uint8_t ucCurrentCsaCount;
	struct IE_SECONDARY_OFFSET *prSecondaryOffsetIE;
	struct IE_WIDE_BAND_CHANNEL *prWideBandChannelIE;
#endif
	uint8_t *pucDumpIE;
	uint8_t fgDomainValid = FALSE;
	enum ENUM_CHANNEL_WIDTH eChannelWidth = CW_20_40MHZ;
	uint8_t ucHtOpChannelFrequencyS3 = 0;

#if (CFG_SUPPORT_TWT_HOTSPOT_AC == 1)
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
#endif

#if (CFG_SUPPORT_BTWT == 1)
	uint8_t fgBtwtIeFound = FALSE;
#endif
	uint8_t *pucIEOpmode = NULL;

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
			rlmRecHtOpForClient(prHtOp,
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
					prBssInfo,
					prBssInfo->ucOpChangeChannelWidth);
			}

			break;
		case ELEM_ID_OP_MODE:
			/* Check OP mode IE at last */
			pucIEOpmode = pucIE;

			break;
#if CFG_SUPPORT_DFS
		case ELEM_ID_WIDE_BAND_CHANNEL_SWITCH:
			if (IE_LEN(pucIE) !=
				(sizeof(struct IE_WIDE_BAND_CHANNEL) - 2))
				break;
			DBGLOG(RLM, INFO,
			       "[CSA] ELEM_ID_WIDE_BAND_CHANNEL_SWITCH, 11AC\n");
			prWideBandChannelIE =
				(struct IE_WIDE_BAND_CHANNEL *)pucIE;
			prCSAParams->ucVhtBw =
				prWideBandChannelIE->ucNewChannelWidth;
			prCSAParams->ucVhtS1 = prWideBandChannelIE->ucChannelS1;
			prCSAParams->ucVhtS2 = prWideBandChannelIE->ucChannelS2;
			DBGLOG(RLM, INFO, "[CSA] BW=%d, s1=%d, s2=%d\n",
			       prCSAParams->ucVhtBw,
			       prCSAParams->ucVhtS1, prCSAParams->ucVhtS2);
			break;
#endif

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

			/* Mode 1 implies that addressed AP is advised to
			 * transmit no further frames on current channel
			 * until the scheduled channel switch.
			 */
			if (prCSAIE->ucChannelSwitchMode != 1)
				continue;

			DBGLOG(RLM, INFO, "[CSA] Count = %d\n",
			       prCSAIE->ucChannelSwitchCount);
			prCSAParams->ucCsaNewCh = prCSAIE->ucNewChannelNum;
			ucCurrentCsaCount = prCSAIE->ucChannelSwitchCount;

			/* Stop tx */
			if (!prBssInfo->fgHasStopTx) {
				prBssInfo->fgHasStopTx = TRUE;
				/* AP */
				qmSetStaRecTxAllowed(prAdapter,
					prStaRec, FALSE);
				DBGLOG(RLM, EVENT, "[CSA] TxAllowed = FALSE\n");
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

			/* Mode 1 implies that addressed AP is advised to
			 * transmit no further frames on current channel
			 * until the scheduled channel switch.
			 */
			if (prExCSAIE->ucChannelSwitchMode != 1)
				continue;

#if (CFG_SUPPORT_WIFI_6G == 1)
			if (prExCSAIE->ucNewOperatingClass >= 131 &&
				prExCSAIE->ucNewOperatingClass <= 135)
				prCSAParams->eCsaBand = BAND_6G;
			else
#endif
			if (prExCSAIE->ucNewChannelNum <= 14)
				prCSAParams->eCsaBand = BAND_2G4;
			else
				prCSAParams->eCsaBand = BAND_5G;

			DBGLOG(RLM, INFO,
				"[CSA] Op class[%d], Band[%d], CH[%d]\n",
				prExCSAIE->ucNewOperatingClass,
				prCSAParams->eCsaBand,
				prExCSAIE->ucNewChannelNum);
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

#if (CFG_SUPPORT_802_11AX == 1)
		case ELEM_ID_RESERVED:
			if (fgEfuseCtrlAxOn != 1)
				break;
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_CAP)
				heRlmRecHeCapInfo(prAdapter,
					prStaRec, pucIE);
			else if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_OP) {
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

					rlmTransferHe6gOpInfor(ucPrimaryChannel,
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
#if (CFG_SUPPORT_802_11BE == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_EHT_CAPS)
				ehtRlmRecCapInfo(prAdapter, prStaRec, pucIE);
			else if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_EHT_OP)
				ehtRlmRecOperation(prAdapter, prStaRec,
					prBssInfo, pucIE);
#endif
			break;
#endif /* CFG_SUPPORT_802_11AX */
		case ELEM_ID_VENDOR:
			rlmParseMtkOui(prAdapter, prStaRec, prBssInfo, pucIE);
			break;
		default:
			break;
		} /* end of switch */
	}	 /* end of IE_FOR_EACH */

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
			DBGLOG(RLM, INFO,
				"[OP Mode IE] NSS=%x RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
				ucVhtOpModeRxNss, prStaRec->u2VhtRxMcsMap,
				prStaRec->u2VhtRxMcsMapAssoc);

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

			DBGLOG(RLM, INFO,
				"[OP Mode IE] HeBW80, NSS=%d, RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
				ucVhtOpModeRxNss,
				prStaRec->u2HeRxMcsMapBW80,
				prStaRec->u2HeRxMcsMapBW80Assoc);

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

				DBGLOG(RLM, INFO,
					"[OP Mode IE] HeBW160, NSS=%d, RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
					ucVhtOpModeRxNss,
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
		cnmTimerStopTimer(prAdapter, &prBssInfo->rCsaTimer);
		cnmTimerStartTimer(prAdapter, &prBssInfo->rCsaTimer,
			prBssInfo->u2BeaconInterval * ucCurrentCsaCount);
		prCSAParams->ucCsaCount = ucCurrentCsaCount;
		DBGLOG(RLM, INFO, "[CSA] Channel switch Countdown: %d msecs\n",
		       prBssInfo->u2BeaconInterval * prCSAParams->ucCsaCount);
	}
#endif

	if (!HAS_CH_SWITCH_PARAMS(prCSAParams) && prBssInfo->fgHasStopTx) {
		if (IS_BSS_AIS(prBssInfo)) {
			/* ucHtOpInfo1 depends on fg40mBwAllowed, which may be
			 * updated after CSA. Should update HtOp again here.
			 */
			aisUpdateParamsForCSA(prAdapter, prBssInfo);
			rlmRecHtOpForClient(prHtOp,
					prBssInfo, &ucPrimaryChannel);
		}

		/* AP */
		qmSetStaRecTxAllowed(prAdapter, prStaRec, TRUE);

		DBGLOG(RLM, EVENT, "[CSA] TxAllowed = TRUE\n");
		prBssInfo->fgHasStopTx = FALSE;
	}

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
		(prStaRec->arTWTFlow[0].eBtwtState ==
			ENUM_BTWT_FLOW_STATE_ACTIVATED)) {
		prStaRec->arTWTFlow[0].eBtwtState =
			ENUM_BTWT_FLOW_STATE_DEFAULT;
		prStaRec->arTWTFlow[0].fgIsBTWT = FALSE;

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
					   struct BSS_INFO *prBssInfo,
					   uint8_t *pucIE, uint16_t u2IELength)
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
	u_int8_t fgNewParameter = FALSE;

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
		rBssRlmParam.ucTxNss != prBssInfo->ucOpTxNss)
		fgNewParameter = TRUE;
	else {
		DBGLOG(RLM, TRACE,
		       "prBssInfo's params are all the same! not to sync!\n");
		fgNewParameter = FALSE;
	}

#if (CFG_SUPPORT_802_11AX == 1)
		if (fgEfuseCtrlAxOn == 1)
			if (rBssInfo.ucBssColorInfo
					!= prBssInfo->ucBssColorInfo) {
				fgNewParameter = TRUE;
				DBGLOG(RLM, INFO,
					"BssColorInfo is changed from %x to %x. Update BSSInfo to FW\n",
					rBssInfo.ucBssColorInfo,
					prBssInfo->ucBssColorInfo);
			}
#endif

	return fgNewParameter;
}

static void rlmRecHtOpForClient(struct IE_HT_OP *prHtOp,
				struct BSS_INFO *prBssInfo,
				uint8_t *pucPrimaryChannel)
{
	struct STA_RECORD *prStaRec;

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

	/* Workaround that some APs fill primary channel field
	 * by its
	 * secondary channel, but its DS IE is correct 20110610
	 */
	if (*pucPrimaryChannel == 0)
		*pucPrimaryChannel = prHtOp->ucPrimaryChannel;
	prBssInfo->ucHtOpInfo1 = prHtOp->ucInfo1;
	prBssInfo->u2HtOpInfo2 = prHtOp->u2Info2;
	prBssInfo->u2HtOpInfo3 = prHtOp->u2Info3;

	/*Backup peer HT OP Info*/
	prStaRec->ucHtPeerOpInfo1 = prHtOp->ucInfo1;
	prStaRec->u2HtPeerOpInfo2 = prHtOp->u2Info2;

	if (!prBssInfo->fg40mBwAllowed) {
		DBGLOG(RLM, TRACE, "ucHtOpInfo1 reset\n");
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
	u_int8_t fgNewParameter;
#if (CFG_SUPPORT_802_11AX == 1)
	u_int8_t fgNewSRParam = FALSE;
#endif
	uint8_t i;
#if (CFG_SUPPORT_BTWT == 1)
	struct WLAN_BEACON_FRAME *prWlanBeacon = NULL;
#endif

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
	for (i = 0; i < prAdapter->ucHwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];

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

				if (EQUAL_MAC_ADDR(
					    prBssInfo->aucBSSID,
					    ((struct WLAN_MAC_MGMT_HEADER
						      *)(prSwRfb->pvHeader))
						    ->aucBSSID)) {
#if (CFG_SUPPORT_BTWT == 1)
				prWlanBeacon = (struct WLAN_BEACON_FRAME *)
								(prSwRfb->pvHeader);

				if (prBssInfo->prStaRecOfAP) {
					(prBssInfo->prStaRecOfAP
						->au4Timestamp[0]) =
					prWlanBeacon->au4Timestamp[0];
					(prBssInfo->prStaRecOfAP
						->au4Timestamp[1]) =
					prWlanBeacon->au4Timestamp[1];
				}
#endif


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
				if (IS_BSS_AIS(prBssInfo) &&
					IS_AIS_ROAMING(prAdapter,
						prBssInfo->ucBssIndex))
					roamingFsmSetRecoverBitmap(prAdapter,
						prBssInfo->ucBssIndex,
						ROAMING_RECOVER_BSS_UPDATE);
				else
					nicUpdateBss(prAdapter,
						     prBssInfo->ucBssIndex);
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
			uint8_t *pucIE, uint16_t u2IELength)
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
		       "N=%d b=%d c=%d s=%d e=%d h=%d I=0x%02x l=%d p=%d w(vht)=%d s1=%d s2=%d RxN=%d, TxN=%d\n",
		       prCmdBody->ucBssIndex, prCmdBody->ucRfBand,
		       prCmdBody->ucPrimaryChannel, prCmdBody->ucRfSco,
		       prCmdBody->ucErpProtectMode, prCmdBody->ucHtProtectMode,
		       prCmdBody->ucHtOpInfo1, prCmdBody->ucUseShortSlotTime,
		       prCmdBody->ucUseShortPreamble,
		       prCmdBody->ucVhtChannelWidth,
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
	ASSERT(prStaRec->ucBssIndex <= prAdapter->ucHwBssIdNum);

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
	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	rlmBssReset(prAdapter, prBssInfo);

	prBssInfo->fg40mBwAllowed = FALSE;
	prBssInfo->fgAssoc40mBwAllowed = FALSE;

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

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		/* MU EDCA params */
		prBssInfo->ucMUEdcaUpdateCnt = 0;
		kalMemSet(&prBssInfo->arMUEdcaParams[0], 0,
		sizeof(struct _CMD_MU_EDCA_PARAMS_T) * WMM_AC_INDEX_NUM);

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
	unsigned char fg40mAllowed = prBssInfo->fgAssoc40mBwAllowed;
	uint8_t ucIdx;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return 0;
	}
	if (!prBssInfo) {
		DBGLOG(NAN, ERROR, "prBssInfo error!\n");
		return 0;
	}

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
	if (prBssInfo->ucOpRxNss < supportNss)
		prHtCap->u2HtCapInfo &=
			~HT_CAP_INFO_SM_POWER_SAVE; /*Set as static power save*/

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
	prTxFrame->ucTransPwr = prAdapter->u4GetTxPower;
	prTxFrame->ucLinkMargin =
		prAdapter->rLinkQuality.rLq[prStaRec->ucBssIndex].
		cRssi - (0 - MIN_RCV_PWR);

	u2PayloadLen = ACTION_SM_TPC_REPORT_LEN;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
		     prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
		     WLAN_MAC_MGMT_HEADER_LEN + u2PayloadLen, pfTxDoneHandler,
		     MSDU_RATE_MODE_AUTO);

	DBGLOG(RLM, TRACE, "ucDialogToken %d ucTransPwr %d ucLinkMargin %d\n",
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

	if (prStaRec->ucBssIndex > prAdapter->ucHwBssIdNum)
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

				if (prChannelSwitchAnnounceIE
					    ->ucChannelSwitchMode == 1) {

					/* Need to stop data
					 * transmission immediately
					 */
					if (!prBssInfo->fgHasStopTx) {
						prBssInfo->fgHasStopTx = TRUE;
						/* AP */
						qmSetStaRecTxAllowed(prAdapter,
							   prStaRec,
							   FALSE);
						DBGLOG(RLM, EVENT,
							"[CSA Mgt] TxAllowed = FALSE\n");
					}

					DBGLOG(RLM, INFO,
						"[CSA Mgt] switch channel [%d]->[%d]\n",
						prBssInfo->ucPrimaryChannel,
						prChannelSwitchAnnounceIE
						    ->ucNewChannelNum);

					prCSAParams->ucCsaNewCh =
						prChannelSwitchAnnounceIE->
							ucNewChannelNum;
					ucCurrentCsaCount =
						prChannelSwitchAnnounceIE->
							ucChannelSwitchCount;

				} else {
					DBGLOG(RLM, INFO,
					       "[CSA Mgt] ucChannelSwitchMode = 0\n");
				}
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
				"[CSA Mgt] Channel switch Countdown: %d msecs\n",
				prBssInfo->u2BeaconInterval *
					prCSAParams->ucCsaCount);
		}

		break;
#endif
	default:
		break;
	}
}

void rlmResetCSAParams(struct BSS_INFO *prBssInfo)
{
	struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams;

	if (!prBssInfo) {
		DBGLOG(RLM, ERROR, "Reset CSA params failed, Bssinfo null!");
		return;
	}

	prCSAParams = &(prBssInfo->CSAParams);
	kalMemZero(prCSAParams, sizeof(struct SWITCH_CH_AND_BAND_PARAMS));
	prCSAParams->ucCsaCount = MAX_CSA_COUNT;
	DBGLOG(RLM, INFO, "Reset CSA count to %u for BSS%d",
	       prCSAParams->ucCsaCount, prBssInfo->ucBssIndex);
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
	uint8_t fgIsSameChnl = FALSE;

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
	prBssInfo->ucPrimaryChannel = prCSAParams->ucCsaNewCh;
	if (prCSAParams->eCsaBand != BAND_NULL)
		prBssInfo->eBand = prCSAParams->eCsaBand;
	else
		prBssInfo->eBand = (prCSAParams->ucCsaNewCh <= 14)
			? BAND_2G4 : BAND_5G;

	/* Store VHT Channel width for later op mode operation */
	prBssInfo->ucVhtChannelWidthBeforeCsa =
		prBssInfo->ucVhtChannelWidth;

	prBssInfo->ucVhtChannelWidth = prCSAParams->ucVhtBw;
	prBssInfo->ucVhtChannelFrequencyS1 = prCSAParams->ucVhtS1;
	prBssInfo->ucVhtChannelFrequencyS2 = prCSAParams->ucVhtS2;

	if (HAS_WIDE_BAND_PARAMS(prCSAParams)) {
		if (prBssInfo->fgIsOpChangeChannelWidth &&
		    rlmGetVhtOpBwByBssOpBw(prBssInfo->ucOpChangeChannelWidth) <
			     prBssInfo->ucVhtChannelWidth) {

			DBGLOG(RLM, LOUD,
			       "Change to w:%d s1:%d s2:%d since own changed BW < peer's WideBand BW",
			       prBssInfo->ucVhtChannelWidth,
			       prBssInfo->ucVhtChannelFrequencyS1,
			       prBssInfo->ucVhtChannelFrequencyS2);
			rlmFillVhtOpInfoByBssOpBw(
				prBssInfo, prBssInfo->ucOpChangeChannelWidth);
		}
	}

	if (HAS_SCO_PARAMS(prCSAParams))
		prBssInfo->eBssSCO = prCSAParams->eSco;

	COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
		  prBssInfo->aucSSID, prBssInfo->ucSSIDLen);
	prBssDesc = scanSearchBssDescByBssidAndSsid(
			prAdapter, prBssInfo->aucBSSID, TRUE, &rSsid);

	if (prBssDesc) {
		DBGLOG(RLM, INFO,
		       "DFS: BSS: " MACSTR
		       " Desc found, channel from %u to %u (band:%u) with sco:%u\n ",
		       MAC2STR(prBssInfo->aucBSSID),
		       prBssDesc->ucChannelNum, prCSAParams->ucCsaNewCh,
		       prBssInfo->eBand, prBssInfo->eBssSCO);
		if (prBssDesc->ucChannelNum == prCSAParams->ucCsaNewCh)
			fgIsSameChnl = TRUE;

		prBssDesc->ucChannelNum = prBssInfo->ucPrimaryChannel;
		prBssDesc->eChannelWidth = prBssInfo->ucVhtChannelWidth;
		prBssDesc->ucCenterFreqS1 = prBssInfo->ucVhtChannelFrequencyS1;
		prBssDesc->ucCenterFreqS2 = prBssInfo->ucVhtChannelFrequencyS2;

		if (IS_BSS_P2P(prBssInfo))
			p2pFuncSwitchGcChannel(prAdapter, prBssInfo);
		else
			kalIndicateChannelSwitch(
				prAdapter->prGlueInfo,
				prBssInfo->eBssSCO,
				prBssDesc->ucChannelNum,
				prBssInfo->eBand,
				prBssInfo->ucBssIndex);
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

		/* Check SAP channel */
		p2pFuncSwitchSapChannel(prAdapter);

	}

	if (IS_BSS_AIS(prBssInfo) &&
		!prBssInfo->fgIsAisSwitchingChnl &&
		!fgIsSameChnl) {
		struct AIS_FSM_INFO *prAisFsmInfo;

		prAisFsmInfo = aisGetAisFsmInfo(
			prAdapter, prBssInfo->ucBssIndex);

		/* Indicate PM abort to sync BSS state with FW */
		nicPmIndicateBssAbort(prAdapter, prBssInfo->ucBssIndex);
		/* Defer ucDTIMPeriod updating to when beacon is received */
		prBssInfo->ucDTIMPeriod = 0;
		/* Release channel if CSA immediately before set authorized */
		aisFsmReleaseCh(prAdapter, prBssInfo->ucBssIndex);

		prBssInfo->fgIsAisSwitchingChnl = TRUE;
		aisReqJoinChPrivilegeForCSA(prAdapter, prAisFsmInfo,
			prBssInfo, &prAisFsmInfo->ucSeqNumOfChReq);
	}

	rlmResetCSAParams(prBssInfo);
}
#endif /* CFG_SUPPORT_DFS */

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
	PFN_TX_DONE_HANDLER pfTxDoneHandler = (PFN_TX_DONE_HANDLER)NULL;

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
	PFN_TX_DONE_HANDLER pfTxDoneHandler = (PFN_TX_DONE_HANDLER)NULL;

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
	uint8_t ucRelatedFrameType =
		OP_NOTIFY_TYPE_NUM; /* Used for HT notification frame */
		/* Used for HT notification frame */
	uint8_t *pucCurrOpState = NULL;
	uint8_t *pucRelatedOpState = NULL;

	/* Sanity check */
	ASSERT((prAdapter != NULL) && (prMsduInfo != NULL));

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];

	ASSERT(prBssInfo);

	prStaRec = prBssInfo->prStaRecOfAP;

	DBGLOG(RLM, INFO,
	       "OP notification Tx done: BSS[%d] Type[%d] Status[%d] IsSuccess[%d]\n",
	       prBssInfo->ucBssIndex, ucOpChangeType,
	       prBssInfo->aucOpModeChangeState[ucOpChangeType], fgIsSuccess);

	do {
		/* <1>handle abnormal case */
		if ((prBssInfo->aucOpModeChangeState[ucOpChangeType] !=
		     OP_NOTIFY_STATE_KEEP) &&
		    (prBssInfo->aucOpModeChangeState[ucOpChangeType] !=
		     OP_NOTIFY_STATE_SENDING)) {
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
			rlmRollbackOpChangeParam(prBssInfo, TRUE, TRUE);
			fgIsOpModeChangeSuccess = FALSE;
			break;
		}

		pucCurrOpState = &prBssInfo
				->aucOpModeChangeState[ucOpChangeType];

		/* <2>Assign Op notification Type/State for HT notification
		 * frame
		 */
		if ((ucOpChangeType == OP_NOTIFY_TYPE_HT_BW) ||
		    (ucOpChangeType == OP_NOTIFY_TYPE_HT_NSS)) {

			ucRelatedFrameType =
				(ucOpChangeType == OP_NOTIFY_TYPE_HT_BW)
					? OP_NOTIFY_TYPE_HT_NSS
					: OP_NOTIFY_TYPE_HT_BW;

			pucRelatedOpState = &prBssInfo
				->aucOpModeChangeState[ucRelatedFrameType];
		}

		/* <3.1>handle TX done - SUCCESS */
		if (fgIsSuccess == TRUE) {

			/* Clear retry count */
			prBssInfo->aucOpModeChangeRetryCnt[ucOpChangeType] = 0;

			if (ucOpChangeType == OP_NOTIFY_TYPE_VHT_NSS_BW) {
				*pucCurrOpState = OP_NOTIFY_STATE_SUCCESS;

				if (prBssInfo->aucOpModeChangeState
					[OP_NOTIFY_TYPE_HT_BW] ==
						OP_NOTIFY_STATE_SENDING ||
					prBssInfo->aucOpModeChangeState
					[OP_NOTIFY_TYPE_HT_NSS] ==
						OP_NOTIFY_STATE_SENDING) {
					/* Wait for HT BW/Nss notification
					 * frames Tx done
					 */
					return;
				}

				/* VHT notification frame sent */
				fgIsOpModeChangeSuccess = TRUE;
				break;
			}

			/* HT notification frame sent */
			if (*pucCurrOpState ==
			    OP_NOTIFY_STATE_SENDING) { /* Change OpMode */
				*pucCurrOpState = OP_NOTIFY_STATE_SUCCESS;

				/* Case1: Wait for VHT notification frame &
				 * HT BW/Nss notification frame TX done
				 */
				if (*pucRelatedOpState ==
					OP_NOTIFY_STATE_SENDING ||
					prBssInfo->aucOpModeChangeState
					[OP_NOTIFY_TYPE_VHT_NSS_BW] ==
						OP_NOTIFY_STATE_SENDING)
					return;

				/* Case2: Both BW and Nss notification TX done
				 * or only change either BW or Nss
				 */
				if ((*pucRelatedOpState ==
				     OP_NOTIFY_STATE_KEEP) ||
				    (*pucRelatedOpState ==
				     OP_NOTIFY_STATE_SUCCESS)) {
					fgIsOpModeChangeSuccess = TRUE;

					/* Case3: One of the notification TX
					 * failed,
					 * re-send a notification frame to
					 * rollback the successful one
					 */
				} else if (*pucRelatedOpState ==
					   OP_NOTIFY_STATE_FAIL) {
					/*Rollback to keep the original BW/Nss
					 */
					*pucCurrOpState = OP_NOTIFY_STATE_KEEP;
					if (ucOpChangeType ==
					    OP_NOTIFY_TYPE_HT_BW)
						u4Status =
						rlmSendNotifyChannelWidthFrame(
						prAdapter, prStaRec,
						rlmGetBssOpBwByVhtAndHtOpInfo(
						prBssInfo));
					else if (ucOpChangeType ==
						 OP_NOTIFY_TYPE_HT_NSS)
						u4Status =
						rlmSendSmPowerSaveFrame(
							prAdapter, prStaRec,
							prBssInfo->ucOpRxNss);

					DBGLOG(RLM, INFO,
						"Bss[%d] OpType[%d] Tx Failed, send OpType",
						prMsduInfo->ucBssIndex,
						ucRelatedFrameType);
					DBGLOG(RLM, INFO,
						"[%d] for roll back to BW[%d] RxNss[%d]\n",
						ucOpChangeType,
						rlmGetBssOpBwByVhtAndHtOpInfo
							(prBssInfo),
						prBssInfo->ucOpRxNss);

					if (u4Status == WLAN_STATUS_SUCCESS)
						return;
				}
			} else if (*pucCurrOpState ==
				   OP_NOTIFY_STATE_KEEP) { /* Rollback OpMode */

				/* Case4: Rollback success, keep original OP
				 * BW/Nss
				 */
				if (ucOpChangeType == OP_NOTIFY_TYPE_HT_BW)
					rlmRollbackOpChangeParam(prBssInfo,
								 TRUE, FALSE);
				else if (ucOpChangeType ==
					 OP_NOTIFY_TYPE_HT_NSS)
					rlmRollbackOpChangeParam(prBssInfo,
								 FALSE, TRUE);

				fgIsOpModeChangeSuccess = FALSE;
			}
		} /* End of processing TX success */
		/* <3.2>handle TX done - FAIL */
		else {
			prBssInfo->aucOpModeChangeRetryCnt[ucOpChangeType]++;

			/* Re-send notification frame */
			if (prBssInfo
				    ->aucOpModeChangeRetryCnt[ucOpChangeType] <=
			    OPERATION_NOTICATION_TX_LIMIT) {
				if (ucOpChangeType == OP_NOTIFY_TYPE_VHT_NSS_BW)
					u4Status =
					rlmSendOpModeNotificationFrame(
					prAdapter, prStaRec,
					prBssInfo->ucOpChangeChannelWidth,
					prBssInfo->ucOpChangeRxNss);
				else if (ucOpChangeType ==
					 OP_NOTIFY_TYPE_HT_NSS)
					u4Status = rlmSendSmPowerSaveFrame(
						prAdapter, prStaRec,
						prBssInfo->ucOpChangeRxNss);
				else if (ucOpChangeType == OP_NOTIFY_TYPE_HT_BW)
					u4Status =
					rlmSendNotifyChannelWidthFrame(
						prAdapter, prStaRec,
						prBssInfo
						->ucOpChangeChannelWidth);

				if (u4Status == WLAN_STATUS_SUCCESS)
					return;
			}

			/* Clear retry count when retry count > TX limit */
			prBssInfo->aucOpModeChangeRetryCnt[ucOpChangeType] = 0;

#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
			/* for power control error handling, */
			/* retry fail but before rollback parameter */
			if (prAdapter->fgPowerNeedDisconnect) {
				cnmPowerControlErrorHandling(prAdapter,
					prBssInfo);
			}
#endif

			/* VHT notification frame sent */
			if (ucOpChangeType ==
			    OP_NOTIFY_TYPE_VHT_NSS_BW) {
				*pucCurrOpState = OP_NOTIFY_STATE_FAIL;

				/* Change failed, keep original OP BW/Nss */
				rlmRollbackOpChangeParam(prBssInfo, TRUE, TRUE);
				fgIsOpModeChangeSuccess = FALSE;
				break;
			}

			/* HT notification frame sent */
			if (*pucCurrOpState ==
			    OP_NOTIFY_STATE_SENDING) { /* Change OpMode */
				*pucCurrOpState = OP_NOTIFY_STATE_FAIL;

				/* Change failed, keep original OP BW/Nss */
				if (ucOpChangeType == OP_NOTIFY_TYPE_HT_BW)
					rlmRollbackOpChangeParam(prBssInfo,
								 TRUE, FALSE);
				else if (ucOpChangeType ==
					 OP_NOTIFY_TYPE_HT_NSS)
					rlmRollbackOpChangeParam(prBssInfo,
								 FALSE, TRUE);

				/* Case1: Wait for both HT BW/Nss notification
				 * frame TX done
				 */
				if (*pucRelatedOpState ==
				    OP_NOTIFY_STATE_SENDING) {
					return;

					/* Case2: Both BW and Nss notification
					 * TX done
					 * or only change either BW or Nss
					 */
				} else if ((*pucRelatedOpState ==
					    OP_NOTIFY_STATE_KEEP) ||
					   (*pucRelatedOpState ==
					    OP_NOTIFY_STATE_FAIL)) {
					fgIsOpModeChangeSuccess = FALSE;

					/* Case3: One of the notification TX
					 * failed,
					 * re-send a notification frame to
					 * rollback the successful one
					 */
				} else if (*pucRelatedOpState ==
					   OP_NOTIFY_STATE_SUCCESS) {
					/*Rollback to keep the original BW/Nss
					 */
					*pucRelatedOpState =
						OP_NOTIFY_STATE_KEEP;

					if (ucRelatedFrameType ==
					    OP_NOTIFY_TYPE_HT_BW) {
						u4Status =
						rlmSendNotifyChannelWidthFrame(
						prAdapter, prStaRec,
						rlmGetBssOpBwByVhtAndHtOpInfo(
						prBssInfo));
					} else if (ucRelatedFrameType ==
						   OP_NOTIFY_TYPE_HT_NSS)
						u4Status =
						rlmSendSmPowerSaveFrame(
							prAdapter, prStaRec,
							prBssInfo->ucOpRxNss);

					DBGLOG(RLM, INFO,
					       "Bss[%d] OpType[%d] Tx Failed, send a OpType[%d] for roll back to BW[%d] RxNss[%d]\n",
					       prMsduInfo->ucBssIndex,
					       ucOpChangeType,
					       ucRelatedFrameType,
					       rlmGetBssOpBwByVhtAndHtOpInfo(
						       prBssInfo),
					       prBssInfo->ucOpRxNss);

					if (u4Status == WLAN_STATUS_SUCCESS)
						return;
				}
			} else if (*pucCurrOpState ==
				   OP_NOTIFY_STATE_KEEP) /* Rollback OpMode */
				/* Case4: Rollback failed, keep changing OP
				 * BW/Nss
				 */
				fgIsOpModeChangeSuccess = FALSE;
		} /* End of processing TX failed */

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
	uint8_t ucOpMaxBw = MAX_BW_UNKNOWN;
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
	if (RLM_NET_IS_11AC(prBssInfo)) { /* VHT */
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
				} else if (prBssInfo->eCurrentOPMode ==
					   OP_MODE_ACCESS_POINT) {
					prBssInfo->eBssSCO = rlmDecideScoForAP(
						prAdapter, prBssInfo);
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

#if (CFG_SUPPORT_802_11AX == 1)
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_HE) {
			/* Update 6G operating info */
			rlmUpdate6GOpInfo(prAdapter, prBssInfo);
		}
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
	u_int8_t fgIsSwitchingP2pChnl = FALSE;

	ASSERT((prAdapter != NULL) && (prBssInfo != NULL));

	if ((prBssInfo->fgIsOpChangeChannelWidth) ||
		(prBssInfo->fgIsOpChangeRxNss) ||
		(prBssInfo->fgIsOpChangeTxNss)) {

		if (IS_BSS_P2P(prBssInfo) && prBssInfo->fgIsSwitchingChnl) {
			DBGLOG(RLM, INFO,
				"Ignore rlm update when switch p2p channel\n");
			fgIsSwitchingP2pChnl = TRUE;
		}

		/* <1> Update own OP BW/Nss */
		rlmChangeOwnOpInfo(prAdapter, prBssInfo);

		/* <2> Update OP BW/Nss to FW */
		if (!fgIsSwitchingP2pChnl) {
			if (IS_BSS_AIS(prBssInfo) &&
			    IS_AIS_ROAMING(prAdapter, prBssInfo->ucBssIndex))
				roamingFsmSetRecoverBitmap(prAdapter,
					prBssInfo->ucBssIndex,
					ROAMING_RECOVER_RLM_SYNC);
			else
				rlmSyncOperationParams(prAdapter, prBssInfo);
		}

		/* <3> Update BCN/Probe Resp IE to notify peers our OP info is
		 * changed (AP mode)
		 */
		if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT &&
			!fgIsSwitchingP2pChnl)
			bssUpdateBeaconContent(prAdapter,
				prBssInfo->ucBssIndex);

		/* <4) Reset flags */
		prBssInfo->fgIsOpChangeChannelWidth = FALSE;
		prBssInfo->fgIsOpChangeRxNss = FALSE;
		prBssInfo->fgIsOpChangeTxNss = FALSE;
	}

	DBGLOG(RLM, INFO,
		"Complete BSS[%d] OP Mode change to BW[%d] RxNss[%d] TxNss[%d]",
		prBssInfo->ucBssIndex,
		rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo),
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
	uint8_t ucSendAct,
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
	if (ucBssIndex >= prAdapter->ucHwBssIdNum)
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
	if (ucChannelWidth == rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo)) {
		fgIsChangeBw = FALSE;
		prBssInfo->fgIsOpChangeChannelWidth = FALSE;
	}
	if (ucOpRxNss == prBssInfo->ucOpRxNss) {
		fgIsChangeRxNss = FALSE;
		prBssInfo->fgIsOpChangeRxNss = FALSE;
	}
	if (ucOpTxNss == prBssInfo->ucOpTxNss) {
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
		"Intend to change BSS[%d] OP Mode to BW[%d] RxNss[%d] TxNss[%d]\n",
		ucBssIndex, ucChannelWidth, ucOpRxNss, ucOpTxNss);

	/* <4> Fill OP Change Info into BssInfo*/
	if (fgIsChangeBw) {
		prBssInfo->ucOpChangeChannelWidth = ucChannelWidth;
		prBssInfo->fgIsOpChangeChannelWidth = TRUE;
		DBGLOG(RLM, INFO, "Intend to change BSS[%d] to BW[%d]\n",
		       ucBssIndex, ucChannelWidth);
	}
	if (fgIsChangeRxNss) {
		prBssInfo->ucOpChangeRxNss = ucOpRxNss;
		prBssInfo->fgIsOpChangeRxNss = TRUE;
		DBGLOG(RLM, INFO, "Intend to change BSS[%d] to RxNss[%d]\n",
		       ucBssIndex, ucOpRxNss);
	}
	if (fgIsChangeTxNss) {
		prBssInfo->ucOpChangeTxNss = ucOpTxNss;
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

		if (!ucSendAct) {
			/* no need to send action frame, just done */
			rlmCompleteOpModeChange(prAdapter, prBssInfo, TRUE);
			return OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_DONE;
		}

#if CFG_SUPPORT_802_11AC
		if (((RLM_NET_IS_11AC(prBssInfo) &&
			(prStaRec->ucDesiredPhyTypeSet &
			PHY_TYPE_SET_802_11AC))
#if (CFG_SUPPORT_802_11AX == 1)
			|| (RLM_NET_IS_11AX(prBssInfo) &&
			(prStaRec->ucDesiredPhyTypeSet &
			PHY_TYPE_SET_802_11AX))
#endif
			)
			&& (fgIsChangeBw || fgIsChangeRxNss)) {
			if (prBssInfo->pfOpChangeHandler)
				prBssInfo->aucOpModeChangeState
					[OP_NOTIFY_TYPE_VHT_NSS_BW] =
					OP_NOTIFY_STATE_SENDING;
			DBGLOG(RLM, INFO,
				"Send VHT OP notification frame: BSS[%d] BW[%d] RxNss[%d]\n",
				ucBssIndex, ucChannelWidth, ucOpRxNss);
			u4Status = rlmSendOpModeNotificationFrame(
				prAdapter, prStaRec,
				ucChannelWidth, ucOpRxNss);
		}
#endif
		if (RLM_NET_IS_11N(prBssInfo)
#if (CFG_SUPPORT_802_11AX == 1)
			|| RLM_NET_IS_11AX(prBssInfo)
#endif
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
				DBGLOG(RLM, INFO, "BSS[%d] BW[%d]\n",
					ucBssIndex, ucChannelWidth);
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
		/* Complete OP Info change after notifying client by beacon */
		rlmCompleteOpModeChange(prAdapter, prBssInfo, TRUE);
		return OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_DONE;
	}

	/* Complete OP mode change if no sending action frames */
	rlmCompleteOpModeChange(prAdapter, prBssInfo, TRUE);
	return OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_DONE;
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
				prStaRec->ucHePhyCapInfo)) {
				DBGLOG(RLM, INFO,
					"Can't change BSS[%d] OP BW to:%d for peer HE doens't support BW80/BW40\n",
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

void rlmSetMaxTxPwrLimit(struct ADAPTER *prAdapter, int8_t cLimit,
			 uint8_t ucEnable)
{
	struct CMD_SET_AP_CONSTRAINT_PWR_LIMIT rTxPwrLimit;

	kalMemZero(&rTxPwrLimit, sizeof(rTxPwrLimit));
	rTxPwrLimit.ucCmdVer =  0x1;
	rTxPwrLimit.ucPwrSetEnable =  ucEnable;
	if (ucEnable) {
		if (cLimit > RLM_MAX_TX_PWR) {
			DBGLOG(RLM, INFO,
			       "LM: Target MaxPwr %d Higher than Capability, reset to capability\n",
			       cLimit);
			cLimit = RLM_MAX_TX_PWR;
		}
		if (cLimit < RLM_MIN_TX_PWR) {
			DBGLOG(RLM, INFO,
			       "LM: Target MinPwr %d Lower than Capability, reset to capability\n",
			       cLimit);
			cLimit = RLM_MIN_TX_PWR;
		}
		DBGLOG(RLM, INFO,
		       "LM: Set Max Tx Power Limit %d, Min Limit %d\n", cLimit,
		       RLM_MIN_TX_PWR);
		rTxPwrLimit.cMaxTxPwr =
			cLimit * 2; /* unit of cMaxTxPwr is 0.5 dBm */
		rTxPwrLimit.cMinTxPwr = RLM_MIN_TX_PWR * 2;
	} else
		DBGLOG(RLM, TRACE, "LM: Disable Tx Power Limit\n");
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

	kalMemFree(prCmdSrCap, VIR_MEM_TYPE,
		   sizeof(struct _SR_CMD_SR_CAP_T));
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Send channel switch frame
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmSendChannelSwitchTxDone(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

		DBGLOG(P2P, INFO,
			"CSA TX Done Status: %d, seqNo: %d\n",
			rTxDoneStatus,
			prMsduInfo->ucTxSeqNum);

	} while (FALSE);

	return WLAN_STATUS_SUCCESS;
}

void rlmSendChannelSwitchFrame(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct MSDU_INFO *prMsduInfo;
	struct ACTION_CHANNEL_SWITCH_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	PFN_TX_DONE_HANDLER pfTxDoneHandler = (PFN_TX_DONE_HANDLER)NULL;
	uint8_t aucBMC[] = BC_MAC_ADDR;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return;

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
			      sizeof(struct ACTION_CHANNEL_SWITCH_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
							u2EstimatedFrameLen);
	if (!prMsduInfo)
		return;

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	prTxFrame = prMsduInfo->prPacket;

	/* Fill frame ctrl */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, aucBMC);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	/* 3 Compose the frame body's frame */
	prTxFrame->ucCategory = CATEGORY_SPEC_MGT;
	prTxFrame->ucAction = ACTION_CHNL_SWITCH;

	/* 3.1 - Channel Switch Announcement element */
	prTxFrame->aucInfoElem[0] = ELEM_ID_CH_SW_ANNOUNCEMENT;
	prTxFrame->aucInfoElem[1] = 3;
	prTxFrame->aucInfoElem[2]
		= prAdapter->rWifiVar.ucChannelSwitchMode;
	prTxFrame->aucInfoElem[3]
		= prAdapter->rWifiVar.ucNewChannelNumber;
	prTxFrame->aucInfoElem[4]
		= prAdapter->rWifiVar.ucChannelSwitchCount;

	/* 3.2 - Secondary Channel Offset element */
	prTxFrame->aucInfoElem[5] = ELEM_ID_SCO;
	prTxFrame->aucInfoElem[6] = 1;
	prTxFrame->aucInfoElem[7]
		= prAdapter->rWifiVar.ucSecondaryOffset;

	/* 3.3 - Wide Bandwidth Channel Switch element */
	prTxFrame->aucInfoElem[8] = ELEM_ID_WIDE_BAND_CHANNEL_SWITCH;
	prTxFrame->aucInfoElem[9] = 3;
	prTxFrame->aucInfoElem[10]
		= prAdapter->rWifiVar.ucNewChannelWidth;
	prTxFrame->aucInfoElem[11]
		= prAdapter->rWifiVar.ucNewChannelS1;
	prTxFrame->aucInfoElem[12]
		= prAdapter->rWifiVar.ucNewChannelS2;

	pfTxDoneHandler = rlmSendChannelSwitchTxDone;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prBssInfo->ucBssIndex,
		     STA_REC_INDEX_BMCAST, WLAN_MAC_MGMT_HEADER_LEN,
		     sizeof(struct ACTION_CHANNEL_SWITCH_FRAME),
		     pfTxDoneHandler,
		     MSDU_RATE_MODE_AUTO);

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
			"Extended CSA TX Done Status: %d, seqNo: %d\n",
			rTxDoneStatus,
			prMsduInfo->ucTxSeqNum);

	} while (FALSE);

	return WLAN_STATUS_SUCCESS;
}

void rlmSendExChannelSwitchFrame(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct MSDU_INFO *prMsduInfo;
	struct ACTION_EX_CHANNEL_SWITCH_FRAME *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	PFN_TX_DONE_HANDLER pfTxDoneHandler = (PFN_TX_DONE_HANDLER)NULL;
	uint8_t aucBMC[] = BC_MAC_ADDR;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return;

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
			      sizeof(struct ACTION_EX_CHANNEL_SWITCH_FRAME);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
							u2EstimatedFrameLen);
	if (!prMsduInfo)
		return;

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	prTxFrame = prMsduInfo->prPacket;

	/* Fill frame ctrl */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, aucBMC);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);

	/* 3 Compose the frame body's frame */
	prTxFrame->ucCategory = CATEGORY_PUBLIC_ACTION;
	prTxFrame->ucAction = ACTION_PUBLIC_EX_CH_SW_ANNOUNCEMENT;

	/* Extended Channel Switch Announcement element */
	prTxFrame->aucInfoElem[0] = ELEM_ID_EX_CH_SW_ANNOUNCEMENT;
	prTxFrame->aucInfoElem[1] = 4;
	prTxFrame->aucInfoElem[2]
		= prAdapter->rWifiVar.ucChannelSwitchMode;
	prTxFrame->aucInfoElem[3]
		= prAdapter->rWifiVar.ucNewOperatingClass;
	prTxFrame->aucInfoElem[4]
		= prAdapter->rWifiVar.ucNewChannelNumber;
	prTxFrame->aucInfoElem[5]
		= prAdapter->rWifiVar.ucChannelSwitchCount;

	pfTxDoneHandler = rlmSendExChannelSwitchTxDone;

	/* 4 Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter, prMsduInfo, prBssInfo->ucBssIndex,
		     STA_REC_INDEX_BMCAST, WLAN_MAC_MGMT_HEADER_LEN,
		     sizeof(struct ACTION_EX_CHANNEL_SWITCH_FRAME),
		     pfTxDoneHandler,
		     MSDU_RATE_MODE_AUTO);

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

int32_t rlmGetOpClassForChannel(
	int32_t channel,
	enum ENUM_BAND band)
{
	const u_int8_t support11n = TRUE;
	const u_int8_t support11ac = TRUE;

	/* 2GHz Band */
	if ((band == BAND_2G4) != 0) {
		if (channel == 14)
			return 82;

		if (channel >= 1 && channel <= 13) {
			if (!support11n)
				/* 20MHz channel */
				return 81;

			if (channel <= 9)
				/* HT40 with secondary channel
				 * above primary
				 */
				return 83;

			/* HT40 with secondary channel
			 * below primary
			 */
			return 84;
		}
		/* Error */
		return 0;
	}

	/* 5GHz Band */
	if ((band == BAND_5G) != 0) {
		if (support11ac) {
			switch (channel) {
			case 42:
			case 58:
			case 106:
			case 122:
			case 138:
			case 155:
				/* 80MHz channel */
				return 128;
			case 50:
			case 114:
				/* 160MHz channel */
				return 129;
			}
		}

		if (!support11n) {
			if (channel >= 36 && channel <= 48)
				return 115;

			if (channel >= 52 && channel <= 64)
				return 118;

			if (channel >= 100 && channel <= 144)
				return 121;

			if (channel >= 149 && channel <= 161)
				return 124;

			if (channel >= 165 && channel <= 169)
				return 125;

		} else {
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
		prRxFrame->aucDestAddr);
	DBGLOG(RLM, INFO,
		"[SAP_Test] aucSrcAddr = " MACSTR "\n", prRxFrame->aucSrcAddr);
	DBGLOG(RLM, INFO,
		"[SAP_Test] aucBSSID = " MACSTR "\n", prRxFrame->aucBSSID);
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
				prBeaconReportIE->aucBSSID);
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
		prRxFrame->aucSrcAddr);
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
