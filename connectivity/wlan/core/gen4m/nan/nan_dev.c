// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#if (CFG_SUPPORT_NAN == 1)

#include "precomp.h"
#include "nan/nan_sec.h"
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
#include "rlm_txpwr_limit_emi.h"
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
#include "nanRescheduler.h"
#endif

#define NAN_SKIP_BOOT_REQ_CH (1)

void nanResetMemory(void)
{
	nanResetWpaSm();
}

uint8_t
nanDevInit(struct ADAPTER *prAdapter, uint8_t ucIdx) {
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;
	struct BSS_INFO *prnanBssInfo = (struct BSS_INFO *)NULL;
	struct _GL_NAN_INFO_T *prNANInfo = (struct _GL_NAN_INFO_T *)NULL;
	enum ENUM_PARAM_NAN_MODE_T eNanMode;
	const struct NON_HT_PHY_ATTRIBUTE *prLegacyPhyAttr;
	const struct NON_HT_ADHOC_MODE_ATTRIBUTE *prLegacyModeAttr;
	uint8_t ucLegacyPhyTp;
	struct WIFI_VAR *prWifiVar;
#if CFG_SUPPORT_NAN_DBDC
	struct DBDC_DECISION_INFO rDbdcDecisionInfo = {0};
#endif
	uint8_t ucOmacIdx = INVALID_OMAC_IDX;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR,
			"[%s] prAdapter is NULL\n", __func__);
		return MAX_BSSID_NUM;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (ucIdx != NAN_DEFAULT_INDEX) {
		struct BSS_INFO *bss = (struct BSS_INFO *) NULL;

		bss = nanGetDefaultLinkBssInfo(prAdapter, NULL);
		if (bss)
			ucOmacIdx = bss->ucOwnMacIndex;
	}
#endif

	prnanBssInfo = cnmGetBssInfoAndInit(prAdapter,
		NETWORK_TYPE_NAN, FALSE, ucOmacIdx);
	if (prnanBssInfo == NULL) {
		DBGLOG(NAN, DEBUG, "No enough BSS INDEX\n");
		return MAX_BSSID_NUM;
	}

	prNANSpecInfo =
		prAdapter->rWifiVar.aprNanSpecificBssInfo[ucIdx];

	prNANInfo = prAdapter->prGlueInfo->aprNANDevInfo[NAN_BSS_INDEX_BAND0];
	if (prnanBssInfo != NULL) {
		DBGLOG(NAN, DEBUG, "NAN DEV BSSIFO INDEX %d %p\n",
		       prnanBssInfo->ucBssIndex, prnanBssInfo);
		COPY_MAC_ADDR(prnanBssInfo->aucOwnMacAddr,
			      prNANInfo->prDevHandler->dev_addr);
		prNANSpecInfo->ucBssIndex = prnanBssInfo->ucBssIndex;
		prNANSpecInfo->u4ModuleUsed = 0;
		prnanBssInfo->eCurrentOPMode = OP_MODE_NAN;

#if (CFG_SUPPORT_NAN_DBDC == 1)
		if (ucIdx == NAN_BSS_INDEX_BAND1)
			eNanMode = NAN_MODE_11A;
		else
			eNanMode = NAN_MODE_MIXED_11BG;
#else
		eNanMode = NAN_MODE_11A;
#endif
		prLegacyModeAttr = &rNonHTNanModeAttr[eNanMode];
		ucLegacyPhyTp =
			(uint8_t)prLegacyModeAttr->ePhyTypeIndex;
		prLegacyPhyAttr = &rNonHTPhyAttributes[ucLegacyPhyTp];
		prWifiVar = &prAdapter->rWifiVar;

#if (CFG_SUPPORT_NAN_DBDC == 1)
		if (ucIdx == NAN_BSS_INDEX_BAND1)
			prnanBssInfo->eBand = BAND_5G;
		else
			prnanBssInfo->eBand = BAND_2G4;
#else
		prnanBssInfo->eBand = BAND_5G;
#endif


		prnanBssInfo->u4PrivateData = 0;
		prnanBssInfo->ucSSIDLen = 0;
		prnanBssInfo->fgIsQBSS = 1;
		prnanBssInfo->eConnectionState = MEDIA_STATE_DISCONNECTED;
		prnanBssInfo->ucOpRxNss = wlanGetSupportNss(
			prAdapter, prnanBssInfo->ucBssIndex);

		/* let secIsProtectedFrame to decide protection or
		 * not by STA record
		 */
		prnanBssInfo->fgIsProtection = FALSE;

		nanGetLinkWmmQueSet(prAdapter, prnanBssInfo);

#if (CFG_SUPPORT_NAN_DBDC == 1)
		if (ucIdx == NAN_BSS_INDEX_BAND1) {
#if (CFG_SUPPORT_802_11AX == 1)
			prnanBssInfo->ucPhyTypeSet =
				prWifiVar->ucAvailablePhyTypeSet &
				PHY_TYPE_SET_802_11ABGNACAX;
#else
			prnanBssInfo->ucPhyTypeSet =
				prWifiVar->ucAvailablePhyTypeSet &
				PHY_TYPE_SET_802_11ANAC;
#endif
		} else
#endif
		{
#if (CFG_SUPPORT_802_11AX == 1)
			prnanBssInfo->ucPhyTypeSet =
				prWifiVar->ucAvailablePhyTypeSet &
				PHY_TYPE_SET_802_11ABGNACAX;
#else
#if (CFG_SUPPORT_NAN_DBDC == 1)
			prnanBssInfo->ucPhyTypeSet =
				prWifiVar->ucAvailablePhyTypeSet &
				PHY_TYPE_SET_802_11BGN;
#else
			prnanBssInfo->ucPhyTypeSet =
				prWifiVar->ucAvailablePhyTypeSet &
				PHY_TYPE_SET_802_11ANAC;
#endif
#endif
		}
#if (CFG_SUPPORT_NAN_11BE == 1)
		if (nanIsEhtSupport(prAdapter))
			prnanBssInfo->ucPhyTypeSet =
				prWifiVar->ucAvailablePhyTypeSet &
				PHY_TYPE_SET_802_11ABGNACAXBE;
#endif

		prnanBssInfo->ucNonHTBasicPhyType = ucLegacyPhyTp;
		if (prLegacyPhyAttr
			    ->fgIsShortPreambleOptionImplemented &&
		    (prWifiVar->ePreambleType == PREAMBLE_TYPE_SHORT ||
		     prWifiVar->ePreambleType == PREAMBLE_TYPE_AUTO))
			prnanBssInfo->fgUseShortPreamble = TRUE;
		else
			prnanBssInfo->fgUseShortPreamble = FALSE;
		prnanBssInfo->fgUseShortSlotTime =
			prLegacyPhyAttr
				->fgIsShortSlotTimeOptionImplemented;

		prnanBssInfo->u2OperationalRateSet =
			prLegacyPhyAttr->u2SupportedRateSet;
		prnanBssInfo->u2BSSBasicRateSet =
			prLegacyModeAttr->u2BSSBasicRateSet;
		/* Mask CCK 1M For Sco scenario except FDD mode */
		if (prAdapter->u4FddMode == FALSE)
			prnanBssInfo->u2BSSBasicRateSet &=
				~RATE_SET_BIT_1M;
		prnanBssInfo->u2VhtBasicMcsSet = 0;

		prnanBssInfo->fgErpProtectMode = FALSE;
		prnanBssInfo->eHtProtectMode = HT_PROTECT_MODE_NONE;
		prnanBssInfo->eGfOperationMode = GF_MODE_DISALLOWED;
		prnanBssInfo->eRifsOperationMode = RIFS_MODE_DISALLOWED;

#if (CFG_SUPPORT_NAN_DBDC == 1)
		if (ucIdx == NAN_BSS_INDEX_BAND1)
			prnanBssInfo->ucPrimaryChannel = 149;
		else
			prnanBssInfo->ucPrimaryChannel = 6;
#else
		prnanBssInfo->ucPrimaryChannel = 149;
#endif


		prnanBssInfo->eBssSCO = CHNL_EXT_SCN;
		prnanBssInfo->ucHtOpInfo1 = 0;
		prnanBssInfo->u2HtOpInfo2 = 0;
		prnanBssInfo->u2HtOpInfo3 = 0;

#if (CFG_SUPPORT_NAN_DBDC == 1)
		prnanBssInfo->ucVhtChannelWidth =
			nanGetChannelBwByBand(prAdapter,
			prnanBssInfo->eBand);
#else
		prnanBssInfo->ucVhtChannelWidth = CW_80MHZ;
#endif

		prnanBssInfo->ucVhtChannelFrequencyS1 = 0;
		prnanBssInfo->ucVhtChannelFrequencyS2 = 0;

		/* NAN En/Dis BW40 in Assoc IE */
		if ((prnanBssInfo->eBand == BAND_5G
				&& nanSchedGet5gNanBw(prAdapter)
				>= NAN_CHNL_BW_40) ||
			(prnanBssInfo->eBand == BAND_2G4
				&& nanSchedGet2gNanBw(prAdapter)
				>= NAN_CHNL_BW_40)) {
			prnanBssInfo->eBssSCO = CHNL_EXT_SCA;
			prnanBssInfo->ucHtOpInfo1 |=
				HT_OP_INFO1_STA_CHNL_WIDTH;
			prnanBssInfo->fgAssoc40mBwAllowed = TRUE;
		}

		prnanBssInfo->u2HwDefaultFixedRateCode = RATE_OFDM_6M;
		rateGetDataRatesFromRateSet(
			prnanBssInfo->u2OperationalRateSet,
			prnanBssInfo->u2BSSBasicRateSet,
			prnanBssInfo->aucAllSupportedRates,
			&prnanBssInfo->ucAllSupportedRatesLen);

#if (CFG_SUPPORT_802_11AX == 1)
		/* Set DBRTS to 0x3FF as defalt */
		prnanBssInfo->ucHeOpParams[0] |=
			HE_OP_PARAM0_TXOP_DUR_RTS_THRESHOLD_MASK;
		prnanBssInfo->ucHeOpParams[1] |=
			HE_OP_PARAM1_TXOP_DUR_RTS_THRESHOLD_MASK;
#endif

#if (CFG_SUPPORT_NAN_11BE_MLO == 1)
		nanMldBssRegister(prAdapter,
			prnanBssInfo);
#endif

		/* Activate NAN BSS */
		if (!IS_BSS_ACTIVE(
			    prAdapter->aprBssInfo
				    [prnanBssInfo->ucBssIndex])) {
			nicUpdateBss(prAdapter, prnanBssInfo->ucBssIndex);

#if (CFG_SUPPORT_NAN_DBDC == 1)
			/* Check if DBDC is required to be enabled first */
			CNM_DBDC_ADD_DECISION_INFO(rDbdcDecisionInfo,
				prnanBssInfo->ucBssIndex,
				prnanBssInfo->eBand,
				prnanBssInfo->ucPrimaryChannel,
				prnanBssInfo->ucWmmQueSet);

			cnmDbdcPreConnectionEnableDecision(
				prAdapter,
				&rDbdcDecisionInfo);
#endif

			/* DBDC decsion may change OpNss */
			cnmOpModeGetTRxNss(prAdapter,
				prnanBssInfo->ucBssIndex,
				&prnanBssInfo->ucOpRxNss,
				&prnanBssInfo->ucOpTxNss);

			nicActivateNetwork(prAdapter, prnanBssInfo->ucBssIndex);

			prnanBssInfo->eConnectionState
				= MEDIA_STATE_CONNECTED;

		}
	}

	return prnanBssInfo->ucBssIndex;

} /* p2pDevFsmInit */

void nanDevFsmUninit(struct ADAPTER *prAdapter, uint8_t ucIdx)
{
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;
	struct BSS_INFO *prnanBssInfo = (struct BSS_INFO *)NULL;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR,
			"[%s] prAdapter is NULL\n", __func__);
		return;
	}

	for (ucIdx = 0; ucIdx < NAN_BSS_INDEX_NUM; ucIdx++) {
		prNANSpecInfo =
			prAdapter->rWifiVar.aprNanSpecificBssInfo[ucIdx];

		if (prNANSpecInfo == NULL) {
			DBGLOG(NAN, ERROR,
				"[%s] prNANSpecInfo is NULL\n", __func__);
			return;
		}

		prnanBssInfo = prAdapter->aprBssInfo[prNANSpecInfo->ucBssIndex];
		DBGLOG(NAN, DEBUG, "UNINIT NAN DEV BSSIFO INDEX %d\n",
		       prnanBssInfo->ucBssIndex);

		/* Clear CmdQue */
		kalClearMgmtFramesByBssIdx(prAdapter->prGlueInfo,
					   prnanBssInfo->ucBssIndex);
		/* Clear PendingCmdQue */
		wlanReleasePendingCMDbyBssIdx(prAdapter,
					      prnanBssInfo->ucBssIndex);
		/* Clear PendingTxMsdu */
		nicFreePendingTxMsduInfo(prAdapter, prnanBssInfo->ucBssIndex,
					 MSDU_REMOVE_BY_BSS_INDEX);

		nicPmIndicateBssAbort(prAdapter, prnanBssInfo->ucBssIndex);

		/* Deactivate BSS. */
		prnanBssInfo->eConnectionState = MEDIA_STATE_DISCONNECTED;
		nicDeactivateNetwork(prAdapter, prnanBssInfo->ucBssIndex);
		nicUpdateBss(prAdapter, prnanBssInfo->ucBssIndex);

		cnmFreeBssInfo(prAdapter, prnanBssInfo);

#if (CFG_SUPPORT_NAN_11BE_MLO == 1)
		nanMldBssUnregister(prAdapter,
			prnanBssInfo);
#endif
	}
}

struct _NAN_SPECIFIC_BSS_INFO_T *
nanGetSpecificBssInfo(
	struct ADAPTER *prAdapter,
	uint8_t eIndex)
{
	return prAdapter->rWifiVar.aprNanSpecificBssInfo[eIndex];
}

uint8_t
nanGetBssIdxbyBand(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand)
{
	uint8_t ucIdx = 0;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo;
	struct BSS_INFO *prBssInfo;

	/* Use default BSS if can't find correct peerSchRec or no band info */
	if (eBand == BAND_NULL) {
		DBGLOG(NAN, WARN, "no band info\n");
		prNANSpecInfo = nanGetSpecificBssInfo(
				prAdapter, NAN_BSS_INDEX_BAND0);
		return prNANSpecInfo->ucBssIndex;
	}

	for (ucIdx = 0; ucIdx < NAN_BSS_INDEX_NUM; ucIdx++) {
		prNANSpecInfo = nanGetSpecificBssInfo(prAdapter, ucIdx);
		prBssInfo = GET_BSS_INFO_BY_INDEX(
			prAdapter,
			prNANSpecInfo->ucBssIndex);

		if ((prBssInfo != NULL) && (prBssInfo->eBand == eBand))
			break;
	}

	return prNANSpecInfo->ucBssIndex;
}

void
nanDevCommonSetCb(
	struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf)
{

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return;
	}

	if (prCmdInfo == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prCmdInfo is NULL\n", __func__);
		return;
	}
}

void
nanDevSetMasterPreference(
	struct ADAPTER *prAdapter,
	uint8_t ucMasterPreference)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_CMD_MASTER_PREFERENCE_T *prCmdNanMasterPreference = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_CMD_MASTER_PREFERENCE_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_MASTER_PREFERENCE,
				    sizeof(struct _NAN_CMD_MASTER_PREFERENCE_T),
				    u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
					      prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prCmdNanMasterPreference =
		(struct _NAN_CMD_MASTER_PREFERENCE_T *)prTlvElement->aucbody;
	prCmdNanMasterPreference->ucMasterPreference = ucMasterPreference;

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, nanDevCommonSetCb,
				      nicCmdTimeoutCommon, u4CmdBufferLen,
				      prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);
}

enum NanStatusType
nanDevSetDiscBcn(
	struct ADAPTER *prAdapter,
	struct _NAN_CMD_EVENT_SET_DISC_BCN_T *prNanSetDiscBcn)
{
	uint32_t rStatus = 0;
	void *prCmdBuffer = NULL;
	uint32_t u4CmdBufferLen = 0;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_CMD_EVENT_SET_DISC_BCN_T *prCmd = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
		sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
		sizeof(struct _NAN_CMD_EVENT_SET_DISC_BCN_T);

	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_INTERNAL_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus =
		nicNanAddNewTlvElement(NAN_CMD_SET_DISC_BCN,
		sizeof(struct _NAN_CMD_EVENT_SET_DISC_BCN_T),
		u4CmdBufferLen,
		prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_INTERNAL_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(1, prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_INTERNAL_FAILURE;
	}

	prCmd =
		(struct _NAN_CMD_EVENT_SET_DISC_BCN_T *)
		prTlvElement->aucbody;
	prCmd->ucDiscBcnType =
		prNanSetDiscBcn->ucDiscBcnType;
	prCmd->ucDiscBcnPeriod =
		prNanSetDiscBcn->ucDiscBcnPeriod;

	kalMemCopy(&prCmd->rDiscBcnTimeline[0],
		&prNanSetDiscBcn->rDiscBcnTimeline[0],
		sizeof(prNanSetDiscBcn->rDiscBcnTimeline));

	rStatus = wlanSendSetQueryCmd(
		prAdapter, CMD_ID_NAN_EXT_CMD,
		TRUE, FALSE, FALSE, nanDevCommonSetCb,
		nicCmdTimeoutCommon, u4CmdBufferLen,
		(uint8_t *)prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	if (rStatus == WLAN_STATUS_SUCCESS || rStatus == WLAN_STATUS_PENDING)
		return NAN_STATUS_SUCCESS;
	else
		return NAN_STATUS_INTERNAL_FAILURE;
}

void nanDevDiscBcnPeriodEvtHandler(
	struct ADAPTER *prAdapter,
	uint8_t *pcuEvtBuf)
{
	struct _NAN_CMD_EVENT_SET_DISC_BCN_T *prDiscBcnPeriodEvt = NULL;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "prAdapter is NULL\n");
		return;
	}

	prDiscBcnPeriodEvt = (struct _NAN_CMD_EVENT_SET_DISC_BCN_T *)
		pcuEvtBuf;

	DBGLOG(NAN, INFO,
		"Update Disc Bcn Period(%d): %d->%d\n",
		prDiscBcnPeriodEvt->ucDiscBcnType,
		prAdapter->rWifiVar.ucDiscBcnPeriod,
		prDiscBcnPeriodEvt->ucDiscBcnPeriod);

	prAdapter->rWifiVar.ucDiscBcnPeriod =
		prDiscBcnPeriodEvt->ucDiscBcnPeriod;
}

uint32_t nanUpdateDfspConfig(
	struct ADAPTER *prAdapter,
	struct _NAN_CMD_DFSP_CONFIG *ptrDfspCfg)
{

	uint32_t rStatus = WLAN_STATUS_FAILURE;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_CMD_DFSP_CONFIG *rCmdDfspConfig = NULL;

	if (prAdapter == NULL || ptrDfspCfg == NULL)
		return rStatus;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_CMD_DFSP_CONFIG);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return rStatus;
	}

	prTlvCommon =
		(struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(
		NAN_CMD_DFSP_CONFIG,
		sizeof(struct _NAN_CMD_DFSP_CONFIG),
		u4CmdBufferLen, prCmdBuffer);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	prTlvElement = nicNanGetTargetTlvElement(1, prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	rCmdDfspConfig =
		(struct _NAN_CMD_DFSP_CONFIG *)
		prTlvElement->aucbody;
	kalMemCopy(
		rCmdDfspConfig, ptrDfspCfg,
		sizeof(struct _NAN_CMD_DFSP_CONFIG));

	/* Default 1s for DFS AP BTO */
	if (rCmdDfspConfig->max_bcn_miss_duration == 0)
		rCmdDfspConfig->max_bcn_miss_duration = 1000;

	rStatus = wlanSendSetQueryCmd(prAdapter,
		CMD_ID_NAN_EXT_CMD,
		TRUE,
		FALSE,
		FALSE, NULL, nicCmdTimeoutCommon,
		u4CmdBufferLen,
		(uint8_t *)prCmdBuffer, NULL, 0);

	return rStatus;
}

enum NanStatusType
nanDevEnableRequest(struct ADAPTER *prAdapter,
		    struct NanEnableRequest *prEnableReq) {
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct NanEnableRequest *prCmdNanEnableReq = NULL;

#if (CFG_SUPPORT_NAN_DBDC == 1)
	/* Before NAN enable stage, host might configure new
	 * avail map then update the multiple map flag.
	 * But if DBDC is still off, we will override the flag as signle map.
	 */
	if (prAdapter->rWifiVar.ucNanMapMask < NAN_TIMELINE_MGMT_SIZE) {
		prAdapter->fgNanMultipleMapTimeline = FALSE;
	} else {
		if (!prAdapter->rWifiVar.fgDbDcModeEn)
			prAdapter->fgNanMultipleMapTimeline = FALSE;
	}
#endif

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct NanEnableRequest);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_ENABLE_REQUEST,
					 sizeof(struct NanEnableRequest),
					 u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
					      prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	prCmdNanEnableReq = (struct NanEnableRequest *)prTlvElement->aucbody;
	kalMemCopy(prCmdNanEnableReq, prEnableReq,
		   sizeof(struct NanEnableRequest));

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, nanDevCommonSetCb,
				      nicCmdTimeoutCommon, u4CmdBufferLen,
				      prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	rlmDomainConnectionNotifiey(prAdapter, NAN_INIT);
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */

	if (rStatus == WLAN_STATUS_SUCCESS)
		return NAN_STATUS_SUCCESS;
	else
		return NAN_STATUS_INTERNAL_FAILURE;
}

enum NanStatusType
nanDevDisableRequest(struct ADAPTER *prAdapter) {
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_DISABLE_REQUEST, 0,
					 u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
					      prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, nanDevCommonSetCb,
				      nicCmdTimeoutCommon, u4CmdBufferLen,
				      prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

#if (CFG_ENABLE_WIFI_DIRECT == 1)
	nanRestoreSapChannel(prAdapter);
#endif /* (CFG_ENABLE_WIFI_DIRECT == 1) */

	if (rStatus == WLAN_STATUS_SUCCESS)
		return NAN_STATUS_SUCCESS;
	else
		return NAN_STATUS_INTERNAL_FAILURE;
}

void
nanDevMasterIndEvtHandler(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf) {
	struct _NAN_ATTR_MASTER_INDICATION_T *prMasterIndEvt;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;

	prMasterIndEvt = (struct _NAN_ATTR_MASTER_INDICATION_T *)pcuEvtBuf;
	/* dumpMemory8((PUINT_8) pcuEvtBuf, 32); */

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return;
	}

	prNANSpecInfo = nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);

	kalMemCopy(&prNANSpecInfo->rMasterIndAttr, prMasterIndEvt,
		   sizeof(struct _NAN_ATTR_MASTER_INDICATION_T));
}

uint32_t
nanDevGetMasterIndAttr(struct ADAPTER *prAdapter,
		       uint8_t *pucMasterIndAttrBuf,
		       uint32_t *pu4MasterIndAttrLength) {
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return WLAN_STATUS_FAILURE;
	}

	prNANSpecInfo = nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);

	if (prNANSpecInfo == NULL)
		return WLAN_STATUS_FAILURE;

	kalMemCopy(pucMasterIndAttrBuf, &prNANSpecInfo->rMasterIndAttr,
		   sizeof(struct _NAN_ATTR_MASTER_INDICATION_T));
	*pu4MasterIndAttrLength = sizeof(struct _NAN_ATTR_MASTER_INDICATION_T);

	return WLAN_STATUS_SUCCESS;
}

void nanDevClusterIdEvtHandler(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf)
{
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return;
	}

	prNANSpecInfo = nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);

	COPY_MAC_ADDR(prNANSpecInfo->aucClusterId, pcuEvtBuf);

	DBGLOG(NAN, DEBUG, "ClusterId=%02x%02x%02x%02x%02x%02x\n",
	       prNANSpecInfo->aucClusterId[0], prNANSpecInfo->aucClusterId[1],
	       prNANSpecInfo->aucClusterId[2], prNANSpecInfo->aucClusterId[3],
	       prNANSpecInfo->aucClusterId[4], prNANSpecInfo->aucClusterId[5]);
}

uint32_t nanDevGetClusterId(struct ADAPTER *prAdapter, uint8_t *pucClusterId)
{
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return WLAN_STATUS_FAILURE;
	}

	prNANSpecInfo = nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);

	if (prNANSpecInfo == NULL)
		return WLAN_STATUS_FAILURE;

	COPY_MAC_ADDR(pucClusterId, prNANSpecInfo->aucClusterId);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanDevSendEnableRequestToCnm(struct ADAPTER *prAdapter)
{
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
	(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;
	struct BSS_INFO *prnanBssInfo = (struct BSS_INFO *)NULL;
	uint8_t ucIdx;

#if NAN_SKIP_BOOT_REQ_CH
	/** Set BSS to active */
	for (ucIdx = 0; ucIdx < NAN_BSS_INDEX_NUM; ucIdx++) {
		prNANSpecInfo = prAdapter
			->rWifiVar.aprNanSpecificBssInfo[ucIdx];
		prnanBssInfo = GET_BSS_INFO_BY_INDEX(
					prAdapter, prNANSpecInfo->ucBssIndex);
		if (prnanBssInfo == NULL) {
			DBGLOG(NAN, ERROR,
				"[%s] prnanBssInfo [%d] is NULL\n",
				__func__, prNANSpecInfo->ucBssIndex);
			return WLAN_STATUS_FAILURE;
		}

		nicDeactivateNetworkEx(prAdapter,
				       NETWORK_ID(prnanBssInfo->ucBssIndex,
						  prnanBssInfo->ucLinkId),
				       TRUE);
	}
	nanDevSendEnableRequest(prAdapter, NULL);
#else
	struct MSG_CH_REQ *prMsgChReq = NULL;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return WLAN_STATUS_FAILURE;
	}

	prNANSpecInfo = prAdapter
		->rWifiVar.aprNanSpecificBssInfo[NAN_BSS_INDEX_BAND0];
	if (prNANSpecInfo == NULL) {
		DBGLOG(NAN, ERROR,
			"[%s] prNANSpecInfo is NULL\n", __func__);
		return WLAN_STATUS_FAILURE;
	}

	prnanBssInfo =
		GET_BSS_INFO_BY_INDEX(prAdapter, prNANSpecInfo->ucBssIndex);

	if (prnanBssInfo == NULL) {
		DBGLOG(NAN, ERROR,
			"[%s] prnanBssInfo [%d] is NULL\n",
			__func__, prNANSpecInfo->ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	prMsgChReq =
		(struct MSG_CH_REQ *)cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG,
		sizeof(struct MSG_CH_REQ));
	if (!prMsgChReq) {
		DBGLOG(NAN, ERROR, "Can't indicate CNM\n");
		return WLAN_STATUS_FAILURE;
	}

	prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
	prMsgChReq->ucBssIndex = prNANSpecInfo->ucBssIndex;
	prMsgChReq->ucTokenID = prAdapter->ucNanReqTokenId = cnmIncreaseTokenId(prAdapter);
	prMsgChReq->ucPrimaryChannel = prnanBssInfo->ucPrimaryChannel;
	prMsgChReq->eRfSco = prnanBssInfo->eBssSCO;
	prMsgChReq->eRfBand = prnanBssInfo->eBand;
	prMsgChReq->eRfChannelWidth = prnanBssInfo->ucVhtChannelWidth;
	prMsgChReq->ucRfCenterFreqSeg1 = prnanBssInfo->ucVhtChannelFrequencyS1;
	prMsgChReq->ucRfCenterFreqSeg2 = prnanBssInfo->ucVhtChannelFrequencyS1;
	prMsgChReq->eReqType = CH_REQ_TYPE_NAN_ON;
	prMsgChReq->u4MaxInterval = 20;
	prMsgChReq->eDBDCBand = ENUM_BAND_AUTO;

	DBGLOG(NAN, DEBUG,
		"NAN req CH for N:%d,Tkn,%d\n",
		prMsgChReq->ucBssIndex,
		prMsgChReq->ucTokenID);

	/** Set BSS to active */
	for (ucIdx = 0; ucIdx < NAN_BSS_INDEX_NUM; ucIdx++) {
		prNANSpecInfo = prAdapter
			->rWifiVar.aprNanSpecificBssInfo[ucIdx];
		prnanBssInfo = GET_BSS_INFO_BY_INDEX(
					prAdapter, prNANSpecInfo->ucBssIndex);
		if (prnanBssInfo == NULL) {
			DBGLOG(NAN, ERROR,
				"[%s] prnanBssInfo [%d] is NULL\n",
				__func__, prNANSpecInfo->ucBssIndex);
			return WLAN_STATUS_FAILURE;
		}

		nicDeactivateNetworkEx(prAdapter,
				       NETWORK_ID(prnanBssInfo->ucBssIndex,
						  prnanBssInfo->ucLinkId),
				       TRUE);
	}

	mboxSendMsg(prAdapter, MBOX_ID_0,
		(struct MSG_HDR *)prMsgChReq,
		MSG_SEND_METHOD_BUF);

	prAdapter->fgIsNanSendRequestToCnm = TRUE;
#endif

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanDevSendAbortRequestToCnm(struct ADAPTER *prAdapter)
{
	struct MSG_CH_ABORT *prMsgChAbort = NULL;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return WLAN_STATUS_FAILURE;
	}

	prNANSpecInfo = prAdapter
		->rWifiVar.aprNanSpecificBssInfo[NAN_BSS_INDEX_BAND0];
	if (prNANSpecInfo == NULL) {
		DBGLOG(NAN, ERROR,
			"[%s] prNANSpecInfo is NULL\n", __func__);
		return WLAN_STATUS_FAILURE;
	}

	prMsgChAbort =
	(struct MSG_CH_ABORT *)cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG,
		sizeof(struct MSG_CH_ABORT));
	if (!prMsgChAbort) {
		DBGLOG(NAN, ERROR, "Can't indicate CNM\n");
		return WLAN_STATUS_FAILURE;
	}

	prMsgChAbort->rMsgHdr.eMsgId = MID_MNY_CNM_CH_ABORT;
	prMsgChAbort->ucBssIndex = prNANSpecInfo->ucBssIndex;
	prMsgChAbort->ucTokenID = prAdapter->ucNanReqTokenId;
	prMsgChAbort->eDBDCBand = ENUM_BAND_AUTO;

	DBGLOG(NAN, DEBUG,
		"NAN abort CH for N:%d,Tkn,%d\n",
		prMsgChAbort->ucBssIndex,
		prMsgChAbort->ucTokenID);

	mboxSendMsg(prAdapter, MBOX_ID_0,
		(struct MSG_HDR *)prMsgChAbort,
		MSG_SEND_METHOD_BUF);

	prAdapter->fgIsNanSendRequestToCnm = FALSE;

	return WLAN_STATUS_SUCCESS;
}

void
nanDevGenEnableRequest(struct ADAPTER *prAdapter)
{
	struct NanEnableRequest rEnableReq;

	/** Send NAN enable request to FW */
	kalMemZero(&rEnableReq, sizeof(struct NanEnableRequest));
	rEnableReq.master_pref = prAdapter->rWifiVar.ucMasterPref;
	rEnableReq.config_random_factor_force = 0;
	rEnableReq.random_factor_force_val = 0;
	rEnableReq.config_hop_count_force = 0;
	rEnableReq.hop_count_force_val = 0;
	rEnableReq.config_5g_channel =
		prAdapter->rWifiVar.ucConfig5gChannel;
	rEnableReq.channel_5g_val =
		prAdapter->rWifiVar.ucChannel5gVal;
	if (rlmDomainIsLegalChannel(prAdapter,
					BAND_5G,
					NAN_5G_LOW_DISC_CHANNEL))
		rEnableReq.channel_5g_val |= BIT(0);
	if (rlmDomainIsLegalChannel(prAdapter,
					BAND_5G,
					NAN_5G_HIGH_DISC_CHANNEL))
		rEnableReq.channel_5g_val |= BIT(1);
	rEnableReq.enable_log_slot_statistics =
		prAdapter->rWifiVar.ucNanLogSlotStatistics;
	nanDevEnableRequest(prAdapter, &rEnableReq);
}

enum NanStatusType
nanDevEnableUnsync(
	struct ADAPTER *prAdapter,
	struct NanEnableUnsync *prEnableUnsync)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct NanEnableUnsync *prCmdNanEnableUnsync = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct NanEnableUnsync);

	prCmdBuffer = cnmMemAlloc(prAdapter,
		RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_ENABLE_UNSYNC,
					 sizeof(struct NanEnableUnsync),
					 u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
						prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_NO_RESOURCE_AVAILABLE;
	}

	prCmdNanEnableUnsync = (struct NanEnableUnsync *)prTlvElement->aucbody;
	kalMemCopy(prCmdNanEnableUnsync, prEnableUnsync,
		   sizeof(struct NanEnableUnsync));

	rStatus = wlanSendSetQueryCmd(prAdapter,
		CMD_ID_NAN_EXT_CMD, TRUE,
		FALSE, FALSE, nanDevCommonSetCb,
		nicCmdTimeoutCommon, u4CmdBufferLen,
		prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	if (rStatus == WLAN_STATUS_SUCCESS)
		return NAN_STATUS_SUCCESS;
	else
		return NAN_STATUS_INTERNAL_FAILURE;
}

void
nanDevGenEnableUnsync(struct ADAPTER *prAdapter)
{
	struct NanEnableUnsync rEnableUnsync;

	/** Send NAN enable Unsync to FW */
	kalMemZero(&rEnableUnsync, sizeof(struct NanEnableUnsync));
	rEnableUnsync.default_publish_channel = 6;
	rEnableUnsync.minDwellMultiplier = 5;
	rEnableUnsync.maxDwellMultiplier = 10;
	rEnableUnsync.publish_channel_list[0] = 6;
	rEnableUnsync.publish_channel_list[1] = 149;
	rEnableUnsync.ucChannelListNum = 2;

	nanDevEnableUnsync(prAdapter, &rEnableUnsync);
}

void
nanDevSendEnableRequest(
	struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;
	struct BSS_INFO *prnanBssInfo = (struct BSS_INFO *)NULL;
	uint8_t ucIdx;
	struct AC_QUE_PARMS *prACQueParms;
	enum ENUM_WMM_ACI eAci;
	uint8_t auCWmin[WMM_AC_INDEX_NUM] = { 4, 4, 3, 2 };
	uint8_t auCWmax[WMM_AC_INDEX_NUM] = { 10, 10, 4, 3 };
	uint8_t auAifs[WMM_AC_INDEX_NUM] = { 3, 7, 2, 2 };
	uint8_t auTxop[WMM_AC_INDEX_NUM] = { 0, 0, 94, 47 };

	/** Update BSS Info and set BSS to connected state */
	for (ucIdx = 0; ucIdx < NAN_BSS_INDEX_NUM; ucIdx++) {

		prNANSpecInfo = prAdapter->rWifiVar
				.aprNanSpecificBssInfo[ucIdx];
		prnanBssInfo =
			prAdapter->aprBssInfo[prNANSpecInfo->ucBssIndex];

		if (!IS_BSS_ACTIVE(prnanBssInfo))
			nicActivateNetworkEx(prAdapter,
				prnanBssInfo->ucBssIndex, FALSE);

		prnanBssInfo->eConnectionState = MEDIA_STATE_CONNECTED;

		nicUpdateBss(prAdapter, prnanBssInfo->ucBssIndex);

		/* Update AC WMM Parm with correct BN info in BSSInfo */
		if (nanGetFeatureIsSigma(prAdapter))
			continue;

		prACQueParms = prnanBssInfo->arACQueParms;

		for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {

			prACQueParms[eAci].ucIsACMSet = FALSE;
			prACQueParms[eAci].u2Aifsn = auAifs[eAci];
			prACQueParms[eAci].u2CWmin = BIT(auCWmin[eAci]) - 1;
			prACQueParms[eAci].u2CWmax = BIT(auCWmax[eAci]) - 1;
			prACQueParms[eAci].u2TxopLimit = auTxop[eAci];
		}
		nicQmUpdateWmmParms(prAdapter,
			prnanBssInfo->ucBssIndex);
	}

	prAdapter->fgNanRestoreCh = FALSE;

	if (nanGetFeatureIsSigma(prAdapter)) {
		if (prAdapter->rNanDiscType == NAN_UNSYNC_DISC)
			nanDevGenEnableUnsync(prAdapter);
		else
			nanDevGenEnableRequest(prAdapter);
	} else {
		nanTrySwitchSapChannel(prAdapter);

		/** Set complete for mtk_cfg80211_vendor_nan send nan enable */
		if (!p2pFuncIsSapGoCsa(prAdapter))
			complete(&prAdapter->prGlueInfo->rNanHaltComp);
		else
			DBGLOG(NAN, DEBUG,
				"Concurrency: Wait CSA\n");
	}

	if (prMsgHdr) {
		nanDevSendAbortRequestToCnm(prAdapter);

		cnmMemFree(prAdapter, prMsgHdr);
	}
}

void nanDevSetDWInterval(struct ADAPTER *prAdapter, uint8_t ucDWInterval)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_CMD_DW_INTERVAL_T *prCmdNanDWInterval = NULL;


	DBGLOG(NAN, DEBUG, "Set DW interval=%u\n", ucDWInterval);

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_CMD_DW_INTERVAL_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_SET_DW_INTERVAL,
					 sizeof(struct _NAN_CMD_DW_INTERVAL_T),
					 u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
					      prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prCmdNanDWInterval =
		(struct _NAN_CMD_DW_INTERVAL_T *)prTlvElement->aucbody;
	prCmdNanDWInterval->ucDWInterval = ucDWInterval;

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, nanDevCommonSetCb,
				      nicCmdTimeoutCommon, u4CmdBufferLen,
				      prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);
}

uint32_t
nanDevGetDeviceInfo(struct ADAPTER *prAdapter,
		void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_CMD_GET_DEVICE_INFO *prCmdNanDeviceInfo = NULL;

	DBGLOG(NAN, DEBUG, "Enter\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct _NAN_EVENT_DEVICE_INFO);

	if (u4QueryBufferLen < sizeof(struct _NAN_EVENT_DEVICE_INFO))
		return WLAN_STATUS_INVALID_LENGTH;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_CMD_GET_DEVICE_INFO);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;
	rStatus =
		nicNanAddNewTlvElement(NAN_CMD_GET_DEVICE_INFO,
				    sizeof(struct _NAN_CMD_GET_DEVICE_INFO),
				    u4CmdBufferLen, prCmdBuffer);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
						prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prCmdNanDeviceInfo =
		(struct _NAN_CMD_GET_DEVICE_INFO *)prTlvElement->aucbody;
	prCmdNanDeviceInfo->ucVersion = 1;

	rStatus = wlanSendSetQueryCmd(
		prAdapter, CMD_ID_NAN_EXT_CMD,
		FALSE, TRUE, TRUE,
		nanDevEventQueryDeviceInfo,
		nicCmdTimeoutCommon,
		u4CmdBufferLen,
		(uint8_t *)prCmdBuffer,
		pvQueryBuffer,
		u4QueryBufferLen);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

#if (CFG_SUPPORT_CONNAC3X == 1)
void nanDevEventQueryDeviceInfo(struct ADAPTER *prAdapter,
				struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf)
{
	struct _NAN_EVENT_DEVICE_INFO *prEventDeviceInfo;
	uint32_t u4QueryInfoLen = 0;
	struct UNI_CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint32_t u4SubEvent;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prTlvElement = (struct UNI_CMD_EVENT_TLV_ELEMENT_T *)pucEventBuf;

	u4SubEvent = prTlvElement->u2Tag;
	DBGLOG(NAN, DEBUG, "event:%u\n", u4SubEvent);

	switch (u4SubEvent) {
	case UNI_EVENT_NAN_DEVICE_INFO:
		prEventDeviceInfo =
			(struct _NAN_EVENT_DEVICE_INFO *) prTlvElement->aucbody;
		kalMemCopy(
			prCmdInfo->pvInformationBuffer,
			prEventDeviceInfo,
			sizeof(struct _NAN_EVENT_DEVICE_INFO));
		u4QueryInfoLen = sizeof(struct _NAN_EVENT_DEVICE_INFO);
		break;
	default:
		break;
	}

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
}
#else
void nanDevEventQueryDeviceInfo(struct ADAPTER *prAdapter,
				struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf)
{
	struct _NAN_EVENT_DEVICE_INFO *prEventDeviceInfo;
	uint32_t u4QueryInfoLen = 0;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint32_t u4SubEvent;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)pucEventBuf;
	prTlvElement =
		(struct _CMD_EVENT_TLV_ELEMENT_T *)prTlvCommon->aucBuffer;

	u4SubEvent = prTlvElement->tag_type;
	DBGLOG(NAN, DEBUG, "event:%u\n", u4SubEvent);

	switch (u4SubEvent) {
	case NAN_EVENT_DEVICE_INFO:
		prEventDeviceInfo =
			(struct _NAN_EVENT_DEVICE_INFO *) prTlvElement->aucbody;
		kalMemCopy(
			prCmdInfo->pvInformationBuffer,
			prEventDeviceInfo,
			sizeof(struct _NAN_EVENT_DEVICE_INFO));
		u4QueryInfoLen = sizeof(struct _NAN_EVENT_DEVICE_INFO);
		break;
	default:
		break;
	}

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
}
#endif

u_int8_t nanIsOn(struct ADAPTER *ad)
{
	if (!ad)
		return FALSE;

	return ad->rNanDiscType != NAN_UNINIT_DISC;
}

uint8_t nanIsEhtSupport(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_NAN_11BE == 1)
	return prAdapter->rWifiVar.ucStaEht &&
		prAdapter->rWifiVar.ucNanEht;
#else
	return 0;
#endif
}

uint8_t nanIsEhtEnable(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_NAN_11BE == 1)
	return prAdapter->rWifiVar.ucStaEht &&
		(prAdapter->rWifiVar.ucNanEht ==
		FEATURE_FORCE_ENABLED);
#else
	return 0;
#endif
}

void nanBackToNormal(struct ADAPTER *prAdapter)
{
	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter is NULL\n");
		return;
	}

	nanExtBackToNormal(prAdapter);
}

static u_int8_t nanIsMockedChannelSet(struct ADAPTER *prAdapter,
				      enum ENUM_NETWORK_TYPE eNetworkType)
{
	struct WIFI_VAR *prWifiVar;
	enum ENUM_BAND eBand;

	if (eNetworkType != NETWORK_TYPE_AIS &&
	    eNetworkType != NETWORK_TYPE_P2P)
		return FALSE;

	prWifiVar = &prAdapter->rWifiVar;
	for (eBand = BAND_2G4; eBand < BAND_NUM; eBand++) {
		if (prWifiVar->aucNanMockedChannel[eNetworkType][eBand])
			return TRUE;
	}

	return FALSE;
}

u_int8_t nanIsAisActive(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;

	if (!prAdapter)
		return FALSE;

	if (nanIsMockedChannelSet(prAdapter, NETWORK_TYPE_AIS))
		return TRUE;

	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];
		if (prBssInfo && IS_BSS_AIS(prBssInfo) &&
		    prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED &&
		    prBssInfo->fgIsInUse && prBssInfo->fgIsNetActive)
			return TRUE;
	}

	return FALSE;
}

u_int8_t nanIsSapOrP2pActive(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	uint8_t i;

	if (!prAdapter)
		return FALSE;

	if (nanIsMockedChannelSet(prAdapter, NETWORK_TYPE_P2P))
		return TRUE;

	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];
		if (prBssInfo &&
		    IS_BSS_P2P(prBssInfo) &&
		    IS_BSS_ALIVE(prAdapter, prBssInfo))
			return TRUE;
	}

	return FALSE;
}

#if (CFG_SUPPORT_MLO_STA_NAN_FALLBACK == 1)
u_int8_t nanNeedFallback(struct ADAPTER *prAdapter)
{
	if (!prAdapter)
		return FALSE;

	return nanIsSapOrP2pActive(prAdapter) &&
		(nanIsOn(prAdapter) ||
		prAdapter->fgIsNANStartWaiting);
}

u_int8_t nanNeedComplete(struct ADAPTER *prAdapter)
{
	if (!prAdapter)
		return FALSE;

	return nanIsSapOrP2pActive(prAdapter) &&
		prAdapter->fgIsNANStartWaiting;
}
#endif

u_int8_t nanIsConcurrency(struct ADAPTER *prAdapter)
{
	if (!prAdapter)
		return FALSE;

#if CFG_SUPPORT_NAN && CFG_ENABLE_WIFI_DIRECT
	return (nanIsSapOrP2pActive(prAdapter) ||
		nanIsAisActive(prAdapter)) &&
		nanIsOn(prAdapter);
#else
	return FALSE;
#endif
}

void nanCcmStableCallback(struct ADAPTER *prAdapter)
{
#if CFG_SUPPORT_CCM
	/* Set complete for nan init */
	if (!prAdapter->rWifiVar.fgCsaInProgress
		&& !p2pFuncIsSapGoCsa(prAdapter)
		&& LINK_IS_EMPTY(&prAdapter->rCcmCheckCsList)
		&& !kal_completion_done(
		&prAdapter->prGlueInfo->rNanHaltComp)) {
		DBGLOG(NAN, DEBUG,
			"Concurrency: Complete NAN\n");
		complete(&prAdapter->prGlueInfo->rNanHaltComp);
	} else
#endif
		DBGLOG(NAN, DEBUG,
			"Concurrency: Don't Complete NAN\n");
}

u_int8_t nanTrySwitchSapChannel(
	struct ADAPTER *prAdapter)
{
#if CFG_ENABLE_WIFI_DIRECT && CFG_NAN_CONCURRENCY
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;
	struct BSS_INFO *nan =
		(struct BSS_INFO *)NULL;
	struct BSS_INFO *sta2g =
		(struct BSS_INFO *)NULL;
	struct BSS_INFO *sta5g =
		(struct BSS_INFO *)NULL;
	struct BSS_INFO *sap =
		(struct BSS_INFO *)NULL;
	uint8_t ucIdx = 0;
	uint8_t i = 0;
	uint8_t sapnum = 0;
	u_int8_t fgIsSingleSap = TRUE;

	if (!prAdapter)
		return FALSE;

	if (!prAdapter->rWifiVar.fgNanConcurrency)
		return FALSE;

	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		struct BSS_INFO *b =
			(struct BSS_INFO *)NULL;
		b = prAdapter->aprBssInfo[i];

		if (!IS_BSS_ALIVE(prAdapter, b))
			continue;

		b->ucBackupCh = 0;

		if (IS_BSS_AIS(b) &&
			(b->eBand == BAND_2G4))
			sta2g = b;
		if (IS_BSS_AIS(b) &&
			(b->eBand != BAND_2G4))
			sta5g = b;
		if (IS_BSS_AP(prAdapter, b)) {
			sap = b;
			sapnum++;
		}
	}

	if (!sap)
		return FALSE;

	fgIsSingleSap = (sapnum == 1);

	for (ucIdx = 0; ucIdx < NAN_BSS_INDEX_NUM; ucIdx++) {
		prNANSpecInfo = prAdapter->rWifiVar
			.aprNanSpecificBssInfo[ucIdx];

		nan = prAdapter->aprBssInfo[
			prNANSpecInfo->ucBssIndex];

		nanUpdateMbmcIdx(prAdapter,
			prNANSpecInfo->ucBssIndex,
			ucIdx);

		/* Single SAP or MLO 2G SAP */
		if (sta2g && !ucIdx)
			nan = sta2g;
		/* MLO 5/6G SAP */
		if (!fgIsSingleSap && ucIdx && sta5g)
			nan = sta5g;

		if (fgIsSingleSap &&
			(nan->ucPrimaryChannel ==
			sap->ucPrimaryChannel)) {
			DBGLOG(NAN, DEBUG,
				"Skip ch = %d\n",
				nan->ucPrimaryChannel);
			break;
		}

		ccmChannelSwitchProducer(
			prAdapter,
			nan,
			__func__);

		if (fgIsSingleSap)
			break;
	}

	ccmRegisterStableCb(
		prAdapter,
		nanCcmStableCallback);

	return TRUE;
#endif /* CFG_ENABLE_WIFI_DIRECT && CFG_NAN_CONCURRENCY */
}

uint8_t nanGetSapCsaChannel(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prP2pBssInfo,
	enum ENUM_BAND *eRfBand,
	uint8_t *ucCh)
{
	uint8_t i = 0;
	uint8_t ucSta2gCh = 0;
	uint8_t ucNan2gCh = 0;

	if (!prAdapter->rWifiVar.fgNanConcurrency)
		return FALSE;

	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		struct BSS_INFO *b =
			(struct BSS_INFO *)NULL;
		b = prAdapter->aprBssInfo[i];

		if (!IS_BSS_ALIVE(prAdapter, b))
			continue;

		if (IS_BSS_AIS(b) &&
			(b->eBand == BAND_2G4))
			ucSta2gCh = b->ucPrimaryChannel;
		if (IS_BSS_NAN(b) &&
			(b->eBand == BAND_2G4))
			ucNan2gCh = b->ucPrimaryChannel;
	}

	/* Restore Trigger */
	if (prAdapter->fgNanRestoreCh &&
		prAdapter->fgIsNANRegistered &&
		prP2pBssInfo &&
		prP2pBssInfo->ucBackupCh) {
		*eRfBand = prP2pBssInfo->eBackupBand;
		*ucCh = prP2pBssInfo->ucBackupCh;
		/* Reset */
		prP2pBssInfo->ucBackupCh = 0;
		DBGLOG(NAN, DEBUG,
			"Restore ch = %d\n", *ucCh);
		return TRUE;
	}

	/* Normal Single SAP */
	if (/* !prAdapter->fgIsNANRegistered && */
		nanIsOn(prAdapter)) {
		*eRfBand = BAND_2G4;
		if (ucSta2gCh)
			*ucCh = ucSta2gCh;
		else
			*ucCh = ucNan2gCh;
		DBGLOG(NAN, DEBUG,
			"Nan on ch = %d\n", *ucCh);
		return TRUE;
	}

	return FALSE;
}

void nanBackupSapChannel(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
#if (CFG_NAN_CONCURRENCY == 1)
	if (!prAdapter || !prBssInfo)
		return;

	if (!nanIsOn(prAdapter))
		return;

	if (!prAdapter->rWifiVar.fgNanConcurrency)
		return;

	prBssInfo->eBackupBand =
		prBssInfo->eBand;
	prBssInfo->ucBackupCh =
		prBssInfo->ucPrimaryChannel;
#endif /* (CFG_NAN_CONCURRENCY == 1) */
}

void nanRestoreSapChannel(
	struct ADAPTER *prAdapter)
{
#if (CFG_NAN_CONCURRENCY == 1)
#if CFG_SUPPORT_CCM
	struct _NAN_SPECIFIC_BSS_INFO_T *prNANSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *)NULL;
	struct BSS_INFO *prBssInfo =
		(struct BSS_INFO *)NULL;
	uint8_t i = 0;

	if (!prAdapter)
		return;

	if (!prAdapter->rWifiVar.fgNanConcurrency)
		return;

	if (!prAdapter->fgIsNANRegistered)
		return;

	ccmUnregisterStableCb(
		prAdapter,
		nanCcmStableCallback);

	prNANSpecInfo = prAdapter->rWifiVar
			.aprNanSpecificBssInfo[NAN_BSS_INDEX_BAND0];
	if (!prNANSpecInfo)
		return;
	prBssInfo = prAdapter->aprBssInfo[
		prNANSpecInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	prAdapter->fgNanRestoreCh = TRUE;

	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		struct BSS_INFO *sap =
			(struct BSS_INFO *)NULL;
		sap = prAdapter->aprBssInfo[i];

		if (!IS_BSS_ALIVE(prAdapter, sap))
			continue;

		if (sap->ucBackupCh && IS_BSS_P2P(sap)) {
			prBssInfo->eBand =
				sap->eBackupBand;
			prBssInfo->ucPrimaryChannel =
				sap->ucBackupCh;
			/* TODO */
			if (prBssInfo->eBand == BAND_2G4)
				prBssInfo->eHwBandIdx =
					ENUM_BAND_0;
			else
				prBssInfo->eHwBandIdx =
					ENUM_BAND_1;
			ccmChannelSwitchProducer(
				prAdapter,
				prBssInfo,
				__func__);
		}
	}
#endif /* CFG_SUPPORT_CCM */
#endif /* (CFG_NAN_CONCURRENCY == 1) */
}

void nanConcurrencyHandler(struct ADAPTER *prAdapter)
{
	nanBackToNormal(prAdapter);

#if (CFG_NAN_CONCURRENCY == 1)
	if (!nanIsConcurrency(prAdapter))
		return;

	DBGLOG(NAN, STATE, "NAN handle P2P status changed\n");
	nanSchedUpdateP2pAisMcc(prAdapter);
	nanRescheduleNdlIfNeeded(prAdapter, P2P_CONNECTED, NULL);
#endif
}

#endif /* CFG_SUPPORT_NAN */
