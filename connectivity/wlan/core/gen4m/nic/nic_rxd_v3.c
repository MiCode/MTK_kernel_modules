// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   nic_tx.c
 *    \brief  Functions that provide TX operation in NIC Layer.
 *
 *    This file provides TX functions which are responsible for both Hardware
 *    and Software Resource Management and keep their Synchronization.
 */


#if (CFG_SUPPORT_CONNAC3X == 1)
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "radiotap.h"
#include "coda/WF_TX_FREE_DONE_EVENT_BESRA_c_header.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

uint16_t nic_rxd_v3_get_rx_byte_count(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC3X_RX_STATUS_GET_RX_BYTE_CNT(
		(struct HW_MAC_CONNAC3X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v3_get_packet_type(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC3X_RX_STATUS_GET_PKT_TYPE(
		(struct HW_MAC_CONNAC3X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v3_get_wlan_idx(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC3X_RX_STATUS_GET_MLD_ID(
		(struct HW_MAC_CONNAC3X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v3_get_sec_mode(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC3X_RX_STATUS_GET_SEC_MODE(
		(struct HW_MAC_CONNAC3X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v3_get_sw_class_error_bit(
	void *prRxStatus)
{
	struct HW_MAC_CONNAC3X_RX_DESC *prRxD;

	prRxD = (struct HW_MAC_CONNAC3X_RX_DESC *)prRxStatus;
	if (HAL_MAC_CONNAC3X_RX_STATUS_IS_SW_DEFINE_RX_CLASSERR(prRxD)) {
		DBGLOG(RSN, ERROR,
		       "RX_CLASSERR: RXD.DW2=0x%x\n",
		       prRxD->u4DW2);
		return TRUE;
	} else
		return FALSE;
}

uint8_t nic_rxd_v3_get_ch_num(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC3X_RX_STATUS_GET_CHNL_NUM(
		(struct HW_MAC_CONNAC3X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v3_get_rf_band(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC3X_RX_STATUS_GET_RF_BAND(
		(struct HW_MAC_CONNAC3X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v3_get_tcl(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC3X_RX_STATUS_GET_TCL(
		(struct HW_MAC_CONNAC3X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v3_get_ofld(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC3X_RX_STATUS_GET_OFLD(
		(struct HW_MAC_CONNAC3X_RX_DESC *)prRxStatus);
}

static uint8_t getUnicastWlanIndex(struct ADAPTER *prAdapter,
				   struct SW_RFB *prSwRfb)
{
	uint16_t u2FrameCtrl;
	struct HW_MAC_CONNAC3X_RX_DESC *prRxStatus = prSwRfb->prRxStatus;

	u2FrameCtrl = nicRxGetFrameControl(prAdapter, prSwRfb);

	if (RXM_IS_MGMT_FRAME(u2FrameCtrl))
		return getWlanIdxByBand(prAdapter, prSwRfb->ucHwBandIdx,
			(uint8_t)
			HAL_MAC_CONNAC3X_RX_STATUS_GET_MLD_ID(prRxStatus));

	else
		return getPrimaryWlanIdx(prAdapter, prSwRfb->ucTid, (uint8_t)
			HAL_MAC_CONNAC3X_RX_STATUS_GET_MLD_ID(prRxStatus));
}

static uint8_t getWlanIndex(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct HW_MAC_CONNAC3X_RX_DESC *prRxStatus = prSwRfb->prRxStatus;

	if (prSwRfb->fgIsBC || prSwRfb->fgIsMC)
		return HAL_MAC_CONNAC3X_RX_STATUS_GET_MLD_ID(prRxStatus);
	else
		return getUnicastWlanIndex(prAdapter, prSwRfb);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Fill RFB
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb   specify the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nic_rxd_v3_fill_rfb(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_VAR *prWifiVar;
	struct HW_MAC_CONNAC3X_RX_DESC *prRxStatus;

	uint32_t u4PktLen = 0;
	/* UINT_32 u4MacHeaderLen; */
	uint32_t u4HeaderOffset;
	uint16_t u2RxStatusOffset;

	uint8_t ucHwChnlNum;

	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;
	prRxStatus = prSwRfb->prRxStatus;

	u4PktLen = (uint32_t) HAL_MAC_CONNAC3X_RX_STATUS_GET_RX_BYTE_CNT(
				prRxStatus);
	u4HeaderOffset = (uint32_t) (
		HAL_MAC_CONNAC3X_RX_STATUS_GET_HEADER_OFFSET(prRxStatus));

	u2RxStatusOffset = prChipInfo->rxd_size;
	prSwRfb->ucGroupVLD =
		(uint8_t) HAL_MAC_CONNAC3X_RX_STATUS_GET_GROUP_VLD(prRxStatus);
	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4)) {
		prSwRfb->prRxStatusGroup4 = (struct HW_MAC_RX_STS_GROUP_4 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_4);

#if CFG_SUPPORT_STA_INFO
#define RETRY_BIT 3
		if (prSwRfb->ucPacketType == RX_PKT_TYPE_RX_DATA && (
			prSwRfb->prRxStatusGroup4->u2FrameCtl &
			BIT(RETRY_BIT)) >> RETRY_BIT) {
			if (prSwRfb->prStaRec)
				prSwRfb->prStaRec->u4RxRetryCnt += 1;
		}
#endif

		NIC_DUMP_RXD_HEADER(prAdapter, "****** RXD GROUP 4 ******\n");
		NIC_DUMP_RXD(prAdapter, (uint32_t *) prSwRfb->prRxStatusGroup4,
			sizeof(struct HW_MAC_RX_STS_GROUP_4));
	}
	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_1)) {
		prSwRfb->prRxStatusGroup1 = (struct HW_MAC_RX_STS_GROUP_1 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_1);

		NIC_DUMP_RXD_HEADER(prAdapter, "****** RXD GROUP 1 ******\n");
		NIC_DUMP_RXD(prAdapter, (uint32_t *) prSwRfb->prRxStatusGroup1,
				sizeof(struct HW_MAC_RX_STS_GROUP_1));
	}
	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_2)) {
		prSwRfb->prRxStatusGroup2 = (struct HW_MAC_RX_STS_GROUP_2 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_2);

		NIC_DUMP_RXD_HEADER(prAdapter, "****** RXD GROUP 2 ******\n");
		NIC_DUMP_RXD(prAdapter, (uint32_t *) prSwRfb->prRxStatusGroup2,
				sizeof(struct HW_MAC_RX_STS_GROUP_2));
	}
	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3)) {
		prSwRfb->prRxStatusGroup3 = (void *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_3_V2);

		NIC_DUMP_RXD_HEADER(prAdapter, "****** RXD GROUP 3 ******\n");
		NIC_DUMP_RXD(prAdapter, (uint32_t *) prSwRfb->prRxStatusGroup3,
				sizeof(struct HW_MAC_RX_STS_GROUP_3_V2));
	}

	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_5)) {
		prSwRfb->prRxStatusGroup5 = (struct HW_MAC_RX_STS_GROUP_5 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += prChipInfo->group5_size;

		NIC_DUMP_RXD_HEADER(prAdapter, "****** RXD GROUP 5 ******\n");
		NIC_DUMP_RXD(prAdapter, (uint32_t *) prSwRfb->prRxStatusGroup5,
				prChipInfo->group5_size);
	}


	prSwRfb->u2RxStatusOffst = u2RxStatusOffset;
	prSwRfb->pvHeader = (uint8_t *) prRxStatus +
		u2RxStatusOffset + u4HeaderOffset;
	prSwRfb->u2RxByteCount = u4PktLen;
	prSwRfb->u2PacketLen = (uint16_t) (u4PktLen -
		(u2RxStatusOffset + u4HeaderOffset));
	prSwRfb->u2HeaderLen = (uint16_t)
		HAL_MAC_CONNAC3X_RX_STATUS_GET_HEADER_LEN(prRxStatus);

	prSwRfb->ucHwBandIdx =
		HAL_MAC_CONNAC3X_RX_STATUS_GET_BAND_IDX(prRxStatus);
	prSwRfb->fgIsBC = HAL_MAC_CONNAC3X_RX_STATUS_IS_BC(prRxStatus);
	prSwRfb->fgIsMC = HAL_MAC_CONNAC3X_RX_STATUS_IS_MC(prRxStatus);
	/* For MLO, MLD_ID need to be corrected according to TID */
	prSwRfb->ucTid =
		(uint8_t) HAL_MAC_CONNAC3X_RX_STATUS_GET_TID(prRxStatus);

	/* prerequisite: ucTid, fgIsBC, fgIsMC, ucHwBandIdx */
	prSwRfb->ucWlanIdx = getWlanIndex(prAdapter, prSwRfb);

	prSwRfb->ucStaRecIdx = secGetStaIdxByWlanIdx(prAdapter,
			prSwRfb->ucWlanIdx);
	prSwRfb->prStaRec = cnmGetStaRecByIndex(prAdapter,
			prSwRfb->ucStaRecIdx);
	prSwRfb->fgHdrTran =
		HAL_MAC_CONNAC3X_RX_STATUS_IS_HEADER_TRAN(prRxStatus);
	prSwRfb->ucPayloadFormat =
		HAL_MAC_CONNAC3X_RX_STATUS_GET_PAYLOAD_FORMAT(prRxStatus);
	prSwRfb->ucRxClassify =
		HAL_MAC_CONNAC3X_RX_STATUS_GET_CLASSIFY(prRxStatus);

	prSwRfb->fgIcvErr =
		HAL_MAC_CONNAC3X_RX_STATUS_IS_ICV_ERROR(prRxStatus);
	prSwRfb->ucSecMode =
		HAL_MAC_CONNAC3X_RX_STATUS_GET_SEC_MODE(prRxStatus);
	prSwRfb->ucOFLD = HAL_MAC_CONNAC3X_RX_STATUS_GET_OFLD(prRxStatus);
	prSwRfb->fgIsCipherMS =
		HAL_MAC_CONNAC3X_RX_STATUS_IS_CIPHER_MISMATCH(prRxStatus);
	prSwRfb->fgIsCipherLenMS =
		HAL_MAC_CONNAC3X_RX_STATUS_IS_CLM_ERROR(prRxStatus);
	prSwRfb->fgIsFrag = HAL_MAC_CONNAC3X_RX_STATUS_IS_FRAG(prRxStatus);
	prSwRfb->fgIsFCS = HAL_MAC_CONNAC3X_RX_STATUS_IS_FCS_ERROR(prRxStatus);
	prSwRfb->fgIsAmpdu = HAL_MAC_CONNAC3X_RX_STATUS_IS_NAMP(prRxStatus);
	prSwRfb->ucRxvSeqNo =
		HAL_MAC_CONNAC3X_RX_STATUS_GET_RXV_SEQ_NO(prRxStatus);
	ucHwChnlNum = HAL_MAC_CONNAC3X_RX_STATUS_GET_CHNL_NUM(prRxStatus);
	prSwRfb->eRfBand =
		HAL_MAC_CONNAC3X_RX_STATUS_GET_RF_BAND(prRxStatus);
	prSwRfb->ucChnlNum =
		nicRxdChNumTranslate(prSwRfb->eRfBand, ucHwChnlNum);
	prSwRfb->ucTcl =
		HAL_MAC_CONNAC3X_RX_STATUS_GET_TCL(prRxStatus);

	if (prChipInfo->fgCheckRxDropThreshold &&
	    prChipInfo->prRxDescOps->getRxModeMcs)
		prChipInfo->prRxDescOps->getRxModeMcs(prSwRfb);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldStarecLogRxData(prAdapter, prSwRfb->prStaRec, prSwRfb->ucHwBandIdx);
#endif

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
#if CFG_TCP_IP_CHKSUM_OFFLOAD
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro)) {
		prSwRfb->u4TcpUdpIpCksStatus =
			HAL_MAC_CONNAC3X_RX_STATUS_GET_CHECKSUM(prRxStatus);
		DBGLOG(RX, LOUD, "u4TcpUdpIpCksStatus[0x%x]\n",
		       prSwRfb->u4TcpUdpIpCksStatus);
	}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}

u_int8_t nic_rxd_v3_sanity_check(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct mt66xx_chip_info *prChipInfo;
	struct HW_MAC_CONNAC3X_RX_DESC *prRxStatus;
	u_int8_t fgDrop = FALSE;
	uint8_t ucBssIndex;
	struct RX_CTRL *prRxCtrl;

	prRxCtrl = &prAdapter->rRxCtrl;
	prChipInfo = prAdapter->chip_info;
	prRxStatus = (struct HW_MAC_CONNAC3X_RX_DESC *)prSwRfb->prRxStatus;

	ucBssIndex = secGetBssIdxByWlanIdx(prAdapter,
			HAL_MAC_CONNAC3X_RX_STATUS_GET_MLD_ID(prRxStatus));

	if (prSwRfb->pvPacket == NULL) {
		fgDrop = TRUE;
		goto end;
	}

	if (!HAL_MAC_CONNAC3X_RX_STATUS_IS_FCS_ERROR(prRxStatus)
	    && !HAL_MAC_CONNAC3X_RX_STATUS_IS_DAF(prRxStatus)
	    && !HAL_MAC_CONNAC3X_RX_STATUS_IS_ICV_ERROR(prRxStatus)
#if CFG_SUPPORT_FRAG_AGG_VALIDATION
	    && !HAL_MAC_CONNAC3X_RX_STATUS_IS_TKIP_MIC_ERROR(prRxStatus)
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */
	) {
		if (!HAL_MAC_CONNAC3X_RX_STATUS_IS_NAMP(prRxStatus))
			prSwRfb->fgReorderBuffer = TRUE;
		else if (HAL_MAC_CONNAC3X_RX_STATUS_IS_NDATA(prRxStatus))
			prSwRfb->fgDataFrame = FALSE;
		else if (HAL_MAC_CONNAC3X_RX_STATUS_IS_FRAG(prRxStatus))
			prSwRfb->fgFragFrame = TRUE;
	} else {
		fgDrop = TRUE;
		if (!HAL_MAC_CONNAC3X_RX_STATUS_IS_ICV_ERROR(prRxStatus)
		    && HAL_MAC_CONNAC3X_RX_STATUS_IS_TKIP_MIC_ERROR(
			prRxStatus)) {
			struct STA_RECORD *prStaRec = NULL;
			struct PARAM_BSSID_EX *prCurrBssid =
				aisGetCurrBssId(prAdapter,
				ucBssIndex);

			if (prCurrBssid)
				prStaRec = cnmGetStaRecByAddress(prAdapter,
					ucBssIndex,
					prCurrBssid->arMacAddress);
			if (prStaRec) {
				struct mt66xx_chip_info *prChipInfo;

				rsnTkipHandleMICFailure(prAdapter, prStaRec, 0);
				nicRxParseDropPkt(prSwRfb);
				DBGLOG(RSN, EVENT,
					"MIC_ERR_PKT, dump RXD and RXP\n");
				/* dump RXD */
				prChipInfo = prAdapter->chip_info;
				DBGLOG_MEM8(NIC, INFO, prRxStatus,
					prChipInfo->rxd_size);
				if (prSwRfb->u2PacketLen > 0) {
					/* dump RXP */
					DBGLOG_MEM8(NIC, INFO,
						prSwRfb->pvHeader,
						prSwRfb->u2PacketLen);
				}
			}
		} else if (HAL_MAC_CONNAC3X_RX_STATUS_IS_LLC_MIS(prRxStatus)
			 && !HAL_MAC_CONNAC3X_RX_STATUS_IS_ERROR(prRxStatus)
			 && !HAL_MAC_CONNAC3X_RX_STATUS_IS_FCS_ERROR(prRxStatus)
			 && !FEAT_SUP_LLC_VLAN_RX(prChipInfo)) {
			uint16_t *pu2EtherType;

			nicRxFillRFB(prAdapter, prSwRfb);

			pu2EtherType = (uint16_t *)
				((uint8_t *)prSwRfb->pvHeader +
				2 * MAC_ADDR_LEN);

			/* If ethernet type is VLAN, do not drop it.
			 * Pass up to driver process
			 */
			if (prSwRfb->u2HeaderLen >= ETH_HLEN
			    && *pu2EtherType == NTOHS(ETH_P_VLAN))
				fgDrop = FALSE;

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
			/*
			 * let qmAmsduValidation check this subframe
			 * before drop it
			 */
			if (prSwRfb->ucPayloadFormat
				== RX_PAYLOAD_FORMAT_FIRST_SUB_AMSDU) {
				fgDrop = FALSE;
				prSwRfb->fgIsFirstSubAMSDULLCMS = TRUE;
			}
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */
		}

		DBGLOG(RSN, TEMP, "Sanity check to drop:%d\n", fgDrop);
	}

	/* Drop plain text during security connection */
	if (prSwRfb->fgIsCipherMS && prSwRfb->fgDataFrame == TRUE) {
		uint16_t *pu2EtherType;

		pu2EtherType = (uint16_t *)
				((uint8_t *)prSwRfb->pvHeader +
				2 * MAC_ADDR_LEN);
		if (prSwRfb->u2HeaderLen >= ETH_HLEN
			&& (*pu2EtherType == NTOHS(ETH_P_1X)
#if CFG_SUPPORT_WAPI
			|| (*pu2EtherType == NTOHS(ETH_WPI_1X))
#endif
		)) {
			fgDrop = FALSE;
			DBGLOG(RSN, INFO,
				"Don't drop eapol or wpi packet\n");
#if CFG_SUPPORT_NAN
		} else if ((ucBssIndex < MAX_BSSID_NUM)
			&& (GET_BSS_INFO_BY_INDEX(prAdapter,
			ucBssIndex)->eNetworkType == NETWORK_TYPE_NAN)
			&& (prSwRfb->fgIsBC | prSwRfb->fgIsMC)) {
			fgDrop = FALSE;
			DBGLOG(RSN, INFO,
				"Don't drop NAN MC pkt for sec\n");
#endif
		} else {
			nicRxParseDropPkt(prSwRfb);
			RX_INC_CNT(prRxCtrl, RX_CIPHER_MISMATCH_DROP_COUNT);
			fgDrop = TRUE;
			DBGLOG(RSN, TEMP,
				"Drop plain text during security connection\n");
		}
	}

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
	/* Drop fragmented broadcast and multicast frame */
	if ((prSwRfb->fgIsBC | prSwRfb->fgIsMC)
		&& (prSwRfb->fgFragFrame == TRUE)) {
		RX_INC_CNT(prRxCtrl, RX_FRAGMENT_BMC_DROP_COUNT);
		fgDrop = TRUE;
		DBGLOG(RSN, TEMP,
			"Drop fragmented broadcast and multicast\n");
	}

end:
	if (fgDrop) {
		if (prSwRfb->pvPacket == NULL)
			RX_INC_CNT(prRxCtrl, RX_NULL_PACKET_COUNT);

		if (HAL_MAC_CONNAC3X_RX_STATUS_IS_FCS_ERROR(prRxStatus))
			RX_INC_CNT(prRxCtrl, RX_FCS_ERR_DROP_COUNT);

		if (HAL_MAC_CONNAC3X_RX_STATUS_IS_DAF(prRxStatus))
			RX_INC_CNT(prRxCtrl, RX_DAF_ERR_DROP_COUNT);

		if (HAL_MAC_CONNAC3X_RX_STATUS_IS_ICV_ERROR(prRxStatus))
			RX_INC_CNT(prRxCtrl, RX_ICV_ERR_DROP_COUNT);

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
		if (HAL_MAC_CONNAC3X_RX_STATUS_IS_TKIP_MIC_ERROR(prRxStatus))
			RX_INC_CNT(prRxCtrl, RX_TKIP_MIC_ERROR_DROP_COUNT);
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

		if (HAL_MAC_CONNAC3X_RX_STATUS_IS_ICV_ERROR(prRxStatus)
#if CFG_SUPPORT_FRAG_AGG_VALIDATION
			|| HAL_MAC_CONNAC3X_RX_STATUS_IS_TKIP_MIC_ERROR(
				prRxStatus)
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */
			) {
			rsnReqDumpWTBL(prAdapter, FW_DUMP_WTBL, ucBssIndex);
			NIC_DUMP_ICV_RXD(prAdapter, prRxStatus);
			if (IS_FEATURE_ENABLED(
				prAdapter->rWifiVar.fgRxIcvErrDbg)
				&& prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4)) {
				DBGLOG(RX, INFO, "****** RXD GROUP 4 ******\n");
				DBGLOG_MEM8(RX, INFO,
					(uint32_t *) prSwRfb->prRxStatusGroup4,
					sizeof(struct HW_MAC_RX_STS_GROUP_4));
			}
			NIC_DUMP_ICV_RXP(prSwRfb->pvHeader,
				prSwRfb->u2PacketLen);
		}
	}

	if (HAL_MAC_CONNAC3X_RX_STATUS_IS_DAF(prRxStatus))
		DBGLOG(RSN, TEMP, "De-amsdu fail, drop:%d\n", fgDrop);
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

	return fgDrop;
}

uint8_t nic_rxd_v3_get_HdrTrans(void *prRxStatus)
{
	return HAL_MAC_CONNAC3X_RX_STATUS_IS_HEADER_TRAN(
		(struct HW_MAC_CONNAC3X_RX_DESC *)prRxStatus);
}

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
void nic_rxd_v3_check_wakeup_reason(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_EVENT *prEvent;
	uint8_t *pvHeader = NULL;
	struct HW_MAC_CONNAC3X_RX_DESC *prRxStatus;
	uint16_t u2PktLen = 0;
	uint32_t u4HeaderOffset;

	prChipInfo = prAdapter->chip_info;

	prRxStatus = (struct HW_MAC_CONNAC3X_RX_DESC *) prSwRfb->prRxStatus;
	if (!prRxStatus)
		return;

	prSwRfb->ucGroupVLD =
		(uint8_t) HAL_MAC_CONNAC3X_RX_STATUS_GET_GROUP_VLD(prRxStatus);

	switch (prSwRfb->ucPacketType) {
	case RX_PKT_TYPE_SW_DEFINED:

	prSwRfb->fgHdrTran = nic_rxd_v3_get_HdrTrans(prRxStatus);
	prSwRfb->ucOFLD = nic_rxd_v3_get_ofld(prRxStatus);

	if (prSwRfb->ucOFLD || prSwRfb->fgHdrTran) {
		DBGLOG(RX, INFO, "Need to treat as data frame.\n");
		/*
		 * In order to jump to case RX_PKT_TYPE_RX_DATA,
		 * DO NOT ADD break here!!!
		 */
	} else {
	/* HIF_RX_PKT_TYPE_EVENT */
		if ((NIC_RX_GET_U2_SW_PKT_TYPE(prSwRfb->prRxStatus) &
			CONNAC3X_RX_STATUS_PKT_TYPE_SW_BITMAP) ==
			CONNAC3X_RX_STATUS_PKT_TYPE_SW_EVENT) {
			prEvent = (struct WIFI_EVENT *)
				(prSwRfb->pucRecvBuff + prChipInfo->rxd_size);
			/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
			nicUpdateWakeupStatistics(prAdapter, RX_EVENT_INT);
			prAdapter->wake_event_count[prEvent->ucEID]++;
#endif
			DBGLOG(RX, INFO, "Event 0x%02x wakeup host\n",
				prEvent->ucEID);
			break;
		} else if ((NIC_RX_GET_U2_SW_PKT_TYPE(prSwRfb->prRxStatus) &
			CONNAC3X_RX_STATUS_PKT_TYPE_SW_BITMAP) ==
			CONNAC3X_RX_STATUS_PKT_TYPE_SW_FRAME) {
			/* case HIF_RX_PKT_TYPE_MANAGEMENT: */
			uint8_t ucSubtype;
			struct WLAN_MAC_MGMT_HEADER *prWlanMgmtHeader;
			uint16_t u2Temp = prChipInfo->rxd_size;
/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
			nicUpdateWakeupStatistics(prAdapter, RX_MGMT_INT);
#endif /* fos_change end */


			u2PktLen =
				HAL_MAC_CONNAC3X_RX_STATUS_GET_RX_BYTE_CNT(
				prRxStatus);

			u4HeaderOffset = (uint32_t)
				HAL_MAC_CONNAC3X_RX_STATUS_GET_HEADER_OFFSET(
				prRxStatus);
			if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4))
				u2Temp += sizeof(struct HW_MAC_RX_STS_GROUP_4);
			if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_1))
				u2Temp += sizeof(struct HW_MAC_RX_STS_GROUP_1);
			if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_2))
				u2Temp += sizeof(struct HW_MAC_RX_STS_GROUP_2);
			if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3))
				u2Temp +=
			    sizeof(struct HW_MAC_RX_STS_GROUP_3_V2);
			pvHeader = (uint8_t *)
				prRxStatus + u2Temp + u4HeaderOffset;
				u2PktLen -= u2Temp + u4HeaderOffset;
			if (!pvHeader) {
				DBGLOG(RX, ERROR,
				"Frame but pvHeader is NULL!\n");
				break;
			}
			prWlanMgmtHeader =
			(struct WLAN_MAC_MGMT_HEADER *)pvHeader;
			ucSubtype = (prWlanMgmtHeader->u2FrameCtrl &
				MASK_FC_SUBTYPE) >> OFFSET_OF_FC_SUBTYPE;
			DBGLOG(RX, INFO,
				"frame subtype: %d",
				ucSubtype);
				DBGLOG(RX, INFO,
				" SeqCtrl %d wakeup host\n",
				prWlanMgmtHeader->u2SeqCtrl);
			DBGLOG_MEM8(RX, INFO,
					pvHeader, u2PktLen > 50 ? 50:u2PktLen);
		} else {
			DBGLOG(RX, ERROR,
			"[%s]: u2PktTYpe(0x%04X) is OUT OF DEF.!!!\n",
			__func__,
			NIC_RX_GET_U2_SW_PKT_TYPE(prSwRfb->prRxStatus));
			ASSERT(0);
		}
		break;
	}
	kal_fallthrough;
	case RX_PKT_TYPE_RX_DATA:
	{
		uint16_t u2Temp = 0;
/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
		nicUpdateWakeupStatistics(prAdapter, RX_DATA_INT);
#endif /* fos_change end */

		u2PktLen =
			HAL_MAC_CONNAC3X_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus);
		u4HeaderOffset = (uint32_t)
			HAL_MAC_CONNAC3X_RX_STATUS_GET_HEADER_OFFSET(
				prRxStatus);
		u2Temp = prChipInfo->rxd_size;
		if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4))
			u2Temp += sizeof(struct HW_MAC_RX_STS_GROUP_4);
		if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_1))
			u2Temp += sizeof(struct HW_MAC_RX_STS_GROUP_1);
		if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_2))
			u2Temp += sizeof(struct HW_MAC_RX_STS_GROUP_2);
		if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3))
			u2Temp += sizeof(struct HW_MAC_RX_STS_GROUP_3_V2);
		pvHeader = (uint8_t *)prRxStatus + u2Temp + u4HeaderOffset;
		u2PktLen -= u2Temp + u4HeaderOffset;
		if (!pvHeader) {
			DBGLOG(RX, ERROR,
				"data packet but pvHeader is NULL!\n");
			break;
		}
		if (HAL_MAC_CONNAC3X_RX_STATUS_IS_NDATA(prRxStatus)) {
			struct WLAN_MAC_HEADER *prWlanMacHeader =
				(struct WLAN_MAC_HEADER *)pvHeader;

			if ((prWlanMacHeader->u2FrameCtrl & MASK_FRAME_TYPE) ==
				MAC_FRAME_BLOCK_ACK_REQ) {
				DBGLOG(RX, INFO,
					"BAR frame[SSN:%d,TID:%d] wakeup host\n"
					, prSwRfb->u2SSN, prSwRfb->ucTid);
				break;
			}
		}
		u2Temp = (pvHeader[ETH_TYPE_LEN_OFFSET] << 8) |
			 (pvHeader[ETH_TYPE_LEN_OFFSET + 1]);

		switch (u2Temp) {
		case ETH_P_IPV4:
			u2Temp = *(uint16_t *) &pvHeader[ETH_HLEN + 4];
			DBGLOG(RX, INFO,
				"IP Packet from:%d.%d.%d.%d,\n",
				pvHeader[ETH_HLEN + 12],
				pvHeader[ETH_HLEN + 13],
				pvHeader[ETH_HLEN + 14],
				pvHeader[ETH_HLEN + 15]);
			DBGLOG(RX, INFO,
				" IP ID 0x%04x wakeup host\n",
				u2Temp);
			break;
		case ETH_P_ARP:
		case ETH_P_1X:
		case ETH_P_PRE_1X:
#if CFG_SUPPORT_WAPI
		case ETH_WPI_1X:
#endif
		case ETH_P_AARP:
		case ETH_P_IPV6:
		case ETH_P_IPX:
		case ETH_P_VLAN:
		case ETH_PRO_TDLS:
			DBGLOG(RX, INFO,
				"Data Packet, EthType 0x%04x wakeup host\n",
				u2Temp);
			break;
		default:
			{

			if (HAL_MAC_CONNAC3X_RX_STATUS_IS_HEADER_TRAN(
						prRxStatus)) {
				if (HAL_MAC_CONNAC3X_RX_STATUS_IS_LLC_MIS(
						prRxStatus)) {
					DBGLOG(RX, INFO,
						"Header translate fail\n");
				} else {
					uint8_t ucPfSts = 0;

					ucPfSts =
					HAL_MAC_CONNAC3X_RX_STATUS_GET_PF_STS(
						prRxStatus);
					DBGLOG(RX, INFO,
						"Wakeup by Eth[0x%x] pf[%d]\n",
						u2Temp, ucPfSts);
				}
			} else {
				struct WLAN_MAC_HEADER *prHeader;

				prHeader = (struct WLAN_MAC_HEADER *)pvHeader;
				if (RXM_IS_FROM_DS_TO_DS(
						prHeader->u2FrameCtrl)) {
					DBGLOG(RX, INFO,
						"Wakeup by TDLS packet\n");
				} else {
					DBGLOG(RX, INFO,
						"Wakeup by frame type[0x%lx]\n",
						prHeader->u2FrameCtrl &
							MASK_FRAME_TYPE);
				}
			}
			DBGLOG_MEM8(RX, INFO,
				(uint8_t *)prSwRfb->prRxStatus,
				prChipInfo->rxd_size);
			DBGLOG_MEM8(RX, INFO,
				pvHeader, u2PktLen > 50 ? 50:u2PktLen);
			}
			break;
		}
		break;
	}

	default:
/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
		nicUpdateWakeupStatistics(prAdapter, RX_OTHERS_INT);
#endif
		DBGLOG(RX, WARN, "Unknown Packet %d wakeup host\n",
			prSwRfb->ucPacketType);
		break;
	}
}
#endif /* CFG_SUPPORT_WAKEUP_REASON_DEBUG */

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
uint8_t nic_rxd_v3_fill_radiotap(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct HW_MAC_CONNAC3X_RX_DESC *prRxStatus;
	struct HW_MAC_RX_STS_GROUP_2 *prRxStatusGroup2 = NULL;
	struct HW_MAC_RX_STS_GROUP_3_V2 *prRxStatusGroup3 = NULL;
	struct HW_MAC_RX_STS_GROUP_5 *prRxStatusGroup5 = NULL;
	struct IEEE80211_RADIOTAP_INFO *prRadiotapInfo;
	uint16_t u2RxStatusOffset;
	uint32_t u4HeaderOffset;
	uint8_t ucHwChnlNum;

	prChipInfo = prAdapter->chip_info;
	prRxStatus = (struct HW_MAC_CONNAC3X_RX_DESC *)prSwRfb->prRxStatus;
	u2RxStatusOffset = prChipInfo->rxd_size;
	prSwRfb->ucGroupVLD =
		(uint8_t) HAL_MAC_CONNAC3X_RX_STATUS_GET_GROUP_VLD(prRxStatus);

	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4)) {
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_4);
	}

	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_1)) {
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_1);
	}

	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_2)) {
		prRxStatusGroup2 = (struct HW_MAC_RX_STS_GROUP_2 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_2);

	}

	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3)) {
		prRxStatusGroup3 = (struct HW_MAC_RX_STS_GROUP_3_V2 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_3_V2);
	}

	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_5)) {
		prRxStatusGroup5 = (struct HW_MAC_RX_STS_GROUP_5 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += prChipInfo->group5_size;
	}

	if (prRxStatusGroup2 == NULL ||
		prRxStatusGroup3 == NULL ||
		prRxStatusGroup5 == NULL)
		return FALSE;

	prSwRfb->u2RxByteCount =
		(uint16_t) HAL_MAC_CONNAC3X_RX_STATUS_GET_RX_BYTE_CNT(
			prRxStatus);

	prSwRfb->ucHwBandIdx =
		HAL_MAC_CONNAC3X_RX_STATUS_GET_BAND_IDX(prRxStatus);

	if (HAL_MAC_CONNAC3X_RX_STATUS_GET_RXV_SEQ_NO(prRxStatus) != 0)
		prGlueInfo->u4AmpduRefNum[prSwRfb->ucHwBandIdx] += 1;

	u4HeaderOffset = (uint32_t) (
		HAL_MAC_CONNAC3X_RX_STATUS_GET_HEADER_OFFSET(prRxStatus));
	u2RxStatusOffset += u4HeaderOffset;

	prRadiotapInfo = prSwRfb->prRadiotapInfo;
	prRadiotapInfo->u2VendorLen = u2RxStatusOffset;
	prRadiotapInfo->ucSubNamespace = 3;
	prRadiotapInfo->u4AmpduRefNum =
		prGlueInfo->u4AmpduRefNum[prSwRfb->ucHwBandIdx];
	prRadiotapInfo->u4Timestamp = prRxStatusGroup2->u4Timestamp;
	prRadiotapInfo->ucFcsErr =
		HAL_MAC_CONNAC3X_RX_STATUS_IS_FCS_ERROR(prRxStatus);
	prRadiotapInfo->ucFrag = HAL_MAC_CONNAC3X_RX_STATUS_IS_FRAG(prRxStatus);
	ucHwChnlNum = HAL_MAC_CONNAC3X_RX_STATUS_GET_CHNL_NUM(prRxStatus);
	prRadiotapInfo->ucRfBand =
		HAL_MAC_CONNAC3X_RX_STATUS_GET_RF_BAND(prRxStatus);
	prRadiotapInfo->ucChanNum =
		nicRxdChNumTranslate(prRadiotapInfo->ucRfBand, ucHwChnlNum);
	prRadiotapInfo->ucTxMode =
		HAL_MAC_CONNAC3X_RX_VT_GET_RX_MODE(prRxStatusGroup5);
	prRadiotapInfo->ucFrMode =
		HAL_MAC_CONNAC3X_RX_VT_GET_FR_MODE(prRxStatusGroup5);
	prRadiotapInfo->ucShortGI =
		HAL_MAC_CONNAC3X_RX_VT_GET_SHORT_GI(prRxStatusGroup5);
	prRadiotapInfo->ucSTBC =
		HAL_MAC_CONNAC3X_RX_VT_GET_STBC(prRxStatusGroup5);
	prRadiotapInfo->ucNess =
		HAL_MAC_CONNAC3X_RX_VT_GET_NESS(prRxStatusGroup5);
	prRadiotapInfo->ucLDPC =
		HAL_MAC_CONNAC3X_RX_VT_GET_LDPC(prRxStatusGroup3);
	prRadiotapInfo->ucMcs =
		HAL_MAC_CONNAC3X_RX_VT_GET_RX_RATE(prRxStatusGroup3);
	prRadiotapInfo->ucRcpi0 =
		HAL_MAC_CONNAC3X_RX_VT_GET_RCPI0(prRxStatusGroup5);
	prRadiotapInfo->ucRcpi1 =
		HAL_MAC_CONNAC3X_RX_VT_GET_RCPI1(prRxStatusGroup5);
	prRadiotapInfo->ucTxopPsNotAllow =
		HAL_MAC_CONNAC3X_RX_VT_TXOP_PS_NOT_ALLOWED(prRxStatusGroup5);
	prRadiotapInfo->ucLdpcExtraOfdmSym =
		HAL_MAC_CONNAC3X_RX_VT_LDPC_EXTRA_OFDM_SYM(prRxStatusGroup5);
	prRadiotapInfo->ucVhtGroupId =
		HAL_MAC_CONNAC3X_RX_VT_GET_GROUP_ID(prRxStatusGroup5);
	prRadiotapInfo->ucNsts =
		HAL_MAC_CONNAC3X_RX_VT_GET_NSTS(prRxStatusGroup3) + 1;
	prRadiotapInfo->ucBeamFormed =
		HAL_MAC_CONNAC3X_RX_VT_GET_BEAMFORMED(prRxStatusGroup3);

	if (prRadiotapInfo->ucTxMode >= TX_RATE_MODE_HE_SU) {
		prRadiotapInfo->ucPeDisamb =
			HAL_MAC_CONNAC3X_RX_VT_GET_PE_DIS_AMB(prRxStatusGroup5);
		prRadiotapInfo->ucNumUser =
			HAL_MAC_CONNAC3X_RX_VT_GET_NUM_USER(prRxStatusGroup5);
		prRadiotapInfo->ucSigBRU0 =
			HAL_MAC_CONNAC3X_RX_VT_GET_SIGB_RU0(prRxStatusGroup5);
		prRadiotapInfo->ucSigBRU1 =
			HAL_MAC_CONNAC3X_RX_VT_GET_SIGB_RU1(prRxStatusGroup5);
		prRadiotapInfo->ucSigBRU2 =
			HAL_MAC_CONNAC3X_RX_VT_GET_SIGB_RU2(prRxStatusGroup5);
		prRadiotapInfo->ucSigBRU3 =
			HAL_MAC_CONNAC3X_RX_VT_GET_SIGB_RU3(prRxStatusGroup5) |
			HAL_MAC_CONNAC3X_RX_VT_GET_SIGB_RU3_1(prRxStatusGroup5);
		prRadiotapInfo->u2VhtPartialAid =
			HAL_MAC_CONNAC3X_RX_VT_GET_PART_AID(prRxStatusGroup5);
		prRadiotapInfo->u2RuAllocation =
			HAL_MAC_CONNAC3X_RX_VT_GET_RU_ALLOC(prRxStatusGroup3);
		prRadiotapInfo->u2BssClr =
			HAL_MAC_CONNAC3X_RX_VT_GET_BSS_COLOR(prRxStatusGroup5);
		prRadiotapInfo->u2BeamChange =
			HAL_MAC_CONNAC3X_RX_VT_GET_BEAM_CHANGE(
				prRxStatusGroup5);
		prRadiotapInfo->u2UlDl =
			HAL_MAC_CONNAC3X_RX_VT_GET_UL_DL(prRxStatusGroup5);
		prRadiotapInfo->u2DataDcm =
			HAL_MAC_CONNAC3X_RX_VT_GET_DCM(prRxStatusGroup5);
		prRadiotapInfo->u2SpatialReuse1 =
			HAL_MAC_CONNAC3X_RX_VT_GET_SPATIAL_REUSE1(
				prRxStatusGroup5);
		prRadiotapInfo->u2SpatialReuse2 =
			HAL_MAC_CONNAC3X_RX_VT_GET_SPATIAL_REUSE2(
				prRxStatusGroup5);
		prRadiotapInfo->u2SpatialReuse3 =
			HAL_MAC_CONNAC3X_RX_VT_GET_SPATIAL_REUSE3(
				prRxStatusGroup5);
		prRadiotapInfo->u2SpatialReuse4 =
			HAL_MAC_CONNAC3X_RX_VT_GET_SPATIAL_REUSE4(
				prRxStatusGroup5);
		prRadiotapInfo->u2Ltf =
			HAL_MAC_CONNAC3X_RX_VT_GET_LTF(prRxStatusGroup5) + 1;
		prRadiotapInfo->u2Doppler =
			HAL_MAC_CONNAC3X_RX_VT_GET_DOPPLER(prRxStatusGroup5);
		prRadiotapInfo->u2Txop =
			HAL_MAC_CONNAC3X_RX_VT_GET_TXOP(prRxStatusGroup5);
		prRadiotapInfo->ucPpduTypeComp =
			HAL_MAC_CONNAC3X_RX_VT_GET_PPDU_TYPE_COMP(
				prRxStatusGroup5);
		prRadiotapInfo->ucEhtSigMcs =
			HAL_MAC_CONNAC3X_RX_VT_GET_EHT_SIG_MCS(
				prRxStatusGroup5);
		prRadiotapInfo->ucEhtLtfSymNum =
			HAL_MAC_CONNAC3X_RX_VT_GET_EHT_LTF_SYM_NUM(
				prRxStatusGroup5);
		prRadiotapInfo->ucEhtSigSymNum =
			HAL_MAC_CONNAC3X_RX_VT_GET_EHT_SIG_SYM_NUM(
				prRxStatusGroup5);
	}

	return TRUE;
}
#endif

static void handle_host_rpt_v5_v6(struct ADAPTER *prAdapter,
	struct tx_free_done_rpt *rpt,
	struct QUE *prFreeQueue)
{
#if (defined(_HIF_PCIE) || defined(_HIF_AXI)) && (CFG_SUPPORT_WED_PROXY == 0)
	struct MSDU_TOKEN_INFO *prTokenInfo =
		&prAdapter->prGlueInfo->rHifInfo.rTokenInfo;
#endif
	uint16_t len = HAL_TX_FREE_DONE_GET_RX_BYTE_COUNT(rpt->dw0);
	uint16_t msdu_cnt = HAL_TX_FREE_DONE_GET_MSDU_ID_COUNT(rpt->dw0);
	uint16_t txd_cnt = HAL_TX_FREE_DONE_GET_TXD_COUNT(rpt->dw1);
	uint16_t serial = HAL_TX_FREE_DONE_GET_SERIAL_ID(rpt->dw1);
	uint8_t msdu_cnt_handled = 0, txd_cnt_handled = 0;
	uint16_t wlan_idx = 0, qid = 0;
	uint16_t rts_tx_cnt = 0, tid = 0, rls_period = 0, tx_bn = 0;
	uint16_t tx_delay = 0, air_delay = 0, tx_cnt = 0;
	uint8_t stat = 0;
	uint8_t pair = 0, header = 0, pair_handled_cnt = 0;
	uint32_t msdu0 = WF_TX_FREE_DONE_EVENT_MSDU_ID0_MASK;
	uint32_t msdu1 = WF_TX_FREE_DONE_EVENT_MSDU_ID0_MASK;
	uint32_t *pos = (uint32_t *)rpt;
	uint32_t *end = (uint32_t *)rpt + len / 4;

	if (len > CFG_RX_MAX_MPDU_SIZE) {
		DBGLOG(HAL, ERROR, "Invalid MSDU Report len:%d\n", len);
		DBGLOG_MEM8(HAL, ERROR, rpt, CFG_RX_MAX_MPDU_SIZE);
		return;
	}

	DBGLOG_MEM8(HAL, TEMP, rpt, len);
	DBGLOG(HAL, TEMP, "len: %d, msdu_cnt: %d, txd_cnt: %d, serial: %d\n",
		len, msdu_cnt, txd_cnt, serial);

	pos += 2;
	do {
		pair = HAL_TX_FREE_DONE_GET_P3(*pos);
		header = HAL_TX_FREE_DONE_GET_H3(*pos);

		if (pair == 1) {
			if (pair_handled_cnt == 0) {
				wlan_idx = HAL_TX_FREE_DONE_GET_WLAN_ID(*pos);
				qid = HAL_TX_FREE_DONE_GET_QID(*pos);
			} else if (pair_handled_cnt == 1) {
				rts_tx_cnt =
					HAL_TX_FREE_DONE_GET_RTS_TX_CNT(*pos);
				tid = HAL_TX_FREE_DONE_GET_TID(*pos);
				rls_period =
					HAL_TX_FREE_DONE_GET_RLS_PERIOD(*pos);
				tx_bn = HAL_TX_FREE_DONE_GET_TX_BN(*pos);
			}
			pair_handled_cnt++;
		} else if (header == 1) {
			tx_delay = HAL_TX_FREE_DONE_GET_TRANSMIT_DELAY(*pos);
			air_delay = HAL_TX_FREE_DONE_GET_AIR_DELAY(*pos);
			tx_cnt = HAL_TX_FREE_DONE_GET_TX_COUNT(*pos);
			stat = HAL_TX_FREE_DONE_GET_STAT(*pos);
			txd_cnt_handled++;
		} else {
			msdu0 = HAL_TX_FREE_DONE_GET_MSDU_ID0(*pos);
			msdu1 = HAL_TX_FREE_DONE_GET_MSDU_ID1(*pos);

			if (msdu0 != WF_TX_FREE_DONE_EVENT_MSDU_ID0_MASK) {
#if (defined(_HIF_PCIE) || defined(_HIF_AXI)) && (CFG_SUPPORT_WED_PROXY == 0)
				if (msdu0 >= prTokenInfo->u4TokenNum) {
					DBGLOG(HAL, ERROR,
						"Invalid MSDU0 [%u]\n",
						msdu0);
					break;
				}
#endif
				halMsduReportStats(prAdapter, msdu0,
						tx_delay, air_delay, stat);
				halProcessToken(prAdapter, msdu0, prFreeQueue);
				msdu_cnt_handled++;
			}

			if (msdu1 != WF_TX_FREE_DONE_EVENT_MSDU_ID0_MASK) {
#if (defined(_HIF_PCIE) || defined(_HIF_AXI)) && (CFG_SUPPORT_WED_PROXY == 0)
				if (msdu1 >= prTokenInfo->u4TokenNum) {
					DBGLOG(HAL, ERROR,
						"Invalid MSDU1 [%u]\n",
						msdu1);
					break;
				}
#endif
				halMsduReportStats(prAdapter, msdu1,
						tx_delay, air_delay, stat);
				halProcessToken(prAdapter, msdu1, prFreeQueue);
				msdu_cnt_handled++;
			}
		}

#define TEMP_LOG_TEMPLATE "wlan_idx: %d, qid: %d, tx_delay: %d, " \
		"rts_tx_cnt: %u, tid: %u, rls_period: %u, tx_bn: %u, "\
		"air_delay: %d, tx_cnt: %d, stat: %d, "	\
		"msdu0: %d, msdu1: %d\n"
		DBGLOG(HAL, TEMP, TEMP_LOG_TEMPLATE,
		       wlan_idx, qid, tx_delay,
		       rts_tx_cnt, tid, rls_period, tx_bn,
		       air_delay, tx_cnt, stat,
		       msdu0, msdu1);
#undef TEMP_LOG_TEMPLATE
		pos += 1;
	} while (pos < end);

	if (msdu_cnt_handled != msdu_cnt || txd_cnt_handled != txd_cnt) {
		DBGLOG(HAL, WARN,
			"Unexpected msdu_cnt[%d/%d] txd_cnt[%d/%d]\n",
			msdu_cnt_handled, msdu_cnt, txd_cnt_handled, txd_cnt);
		DBGLOG_MEM8(HAL, WARN, rpt, len);
	}
}

void nic_rxd_v3_handle_host_rpt(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct QUE *prFreeQueue)
{
	struct tx_free_done_rpt *rpt = (struct tx_free_done_rpt *)prSwRfb->pucRecvBuff;
	uint8_t ver = HAL_TX_FREE_DONE_GET_VER(rpt->dw1);

	switch (ver) {
	case TFD_EVT_VER_5:
	case TFD_EVT_VER_6:
		handle_host_rpt_v5_v6(prAdapter, rpt, prFreeQueue);
		break;
	default:
		DBGLOG(RX, ERROR, "Unsupported ver: %d\n",
			ver);
		break;
	}
}

#endif /* CFG_SUPPORT_CONNAC3X == 1 */
