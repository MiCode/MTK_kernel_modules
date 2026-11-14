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


#if (CFG_SUPPORT_CONNAC5X == 1)
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

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

uint8_t nic_txd_v5_long_format_op(
	void *prTxDesc,
	uint8_t fgSet)
{
	return TRUE;
}

uint8_t nic_txd_v5_tid_op(
	void *prTxDesc,
	uint8_t ucTid,
	uint8_t fgSet)
{
	if (fgSet)
		HAL_MAC_CONNAC5X_TXD_SET_TID_MGMT_TYPE(
			(struct HW_MAC_CONNAC5X_TX_DESC *)prTxDesc, ucTid);
	return HAL_MAC_CONNAC5X_TXD_GET_TID_MGMT_TYPE(
			(struct HW_MAC_CONNAC5X_TX_DESC *)prTxDesc);
}

uint8_t nic_txd_v5_pkt_format_op(
	void *prTxDesc,
	uint8_t ucFormat,
	uint8_t fgSet)
{
	if (fgSet)
		HAL_MAC_CONNAC5X_TXD_SET_PKT_FORMAT(
			(struct HW_MAC_CONNAC5X_TX_DESC *)prTxDesc, ucFormat);
	return HAL_MAC_CONNAC5X_TXD_GET_PKT_FORMAT(
			(struct HW_MAC_CONNAC5X_TX_DESC *)prTxDesc);
}

uint8_t nic_txd_v5_queue_idx_op(
	void *prTxDesc,
	uint8_t ucQueIdx,
	uint8_t fgSet)
{
	if (fgSet)
		HAL_MAC_CONNAC5X_TXD_SET_QUEUE_INDEX(
			(struct HW_MAC_CONNAC5X_TX_DESC *)prTxDesc, ucQueIdx);
	return HAL_MAC_CONNAC5X_TXD_GET_QUEUE_INDEX(
			(struct HW_MAC_CONNAC5X_TX_DESC *)prTxDesc);
}

#if (CFG_TCP_IP_CHKSUM_OFFLOAD == 1)
void nic_txd_v5_chksum_op(void *prTxDesc, uint8_t ucChksumFlag,
			struct MSDU_INFO *prMsduInfo)
{
	if ((ucChksumFlag & TX_CS_IP_GEN) ||
	    (ucChksumFlag & TX_CS_TCP_UDP_GEN))
		HAL_MAC_CONNAC5X_TXD_SET_CHKSUM_OFFLOAD(
			(struct HW_MAC_CONNAC5X_TX_DESC *)prTxDesc);
	/*
	 * If kernel do not expect HW checksum for this frame, set ~AMSDU.
	 * The ICMP frame check were done by checking pfTxDoneHandler
	 * in nic_txd_*_compose().
	 * In that case ICMP do not need HW checksum would cause following
	 * frames need checksum but skipped, but only happened if IcmpTxs
	 * were disabled for special test case.
	 */
	if (!(ucChksumFlag & (TX_CS_IP_GEN | TX_CS_TCP_UDP_GEN)) &&
	    prMsduInfo->ucPktType != ENUM_PKT_ICMP)
		HAL_MAC_CONNAC5X_TXD_UNSET_HW_AMSDU(
			(struct HW_MAC_CONNAC5X_TX_DESC *)prTxDesc);
}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD == 1 */

void nic_txd_v5_header_format_op(
	void *prTxDesc,
	struct MSDU_INFO *prMsduInfo)
{
	if (!prMsduInfo->fgIs802_11) {
		if (prMsduInfo->fgIs802_3)
			HAL_MAC_CONNAC5X_TXD_UNSET_ETHERNET_II(
				(struct HW_MAC_CONNAC5X_TX_DESC *)prTxDesc);
		if (prMsduInfo->fgIsVlanExists)
			HAL_MAC_CONNAC5X_TXD_SET_VLAN(
				(struct HW_MAC_CONNAC5X_TX_DESC *)prTxDesc);
	}
}

void nic_txd_v5_fill_by_pkt_option(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	void *prTxD)
{
	struct HW_MAC_CONNAC5X_TX_DESC *prTxDesc =
				(struct HW_MAC_CONNAC5X_TX_DESC *)prTxD;
	uint32_t u4PktOption = prMsduInfo->u4Option;
	u_int8_t fgProtected = FALSE;

	/* Skip this function if no options is set */
	if (!u4PktOption)
		return;

	/* Fields in DW0 and DW1 (Short Format) */
	if (u4PktOption & MSDU_OPT_NO_ACK)
		HAL_MAC_CONNAC5X_TXD_SET_NO_ACK(prTxDesc);

	if (u4PktOption & MSDU_OPT_PROTECTED_FRAME) {
		DBGLOG_LIMITED(RSN, TRACE, "MSDU_OPT_PROTECTED_FRAME\n");
		HAL_MAC_CONNAC5X_TXD_SET_PROTECTION(prTxDesc);
		fgProtected = TRUE;
	}

	switch (HAL_MAC_CONNAC5X_TXD_GET_HEADER_FORMAT(prTxDesc)) {
	case HEADER_FORMAT_802_11_ENHANCE_MODE:
		if (u4PktOption & MSDU_OPT_AMSDU)
			HAL_MAC_CONNAC5X_TXD_SET_AMSDU(prTxDesc);
		break;

	case HEADER_FORMAT_NON_802_11:
		if (u4PktOption & MSDU_OPT_REMOVE_VLAN)
			HAL_MAC_CONNAC5X_TXD_SET_REMOVE_VLAN(prTxDesc);
		break;

	case HEADER_FORMAT_802_11_NORMAL_MODE:
		if (fgProtected && prMsduInfo->prPacket) {
			struct WLAN_MAC_HEADER *prWlanHeader = NULL;
#if CFG_SUPPORT_MLR
			if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_MGMT) {
				prWlanHeader = (struct WLAN_MAC_HEADER *)
					((uintptr_t) (prMsduInfo->prPacket)
					+ MAC_TX_RESERVED_FIELD);
				if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
					DBGLOG(RSN, INFO,
						"MLR txdf - 802.11mgmt FC=0x%04x dump...\n",
						prWlanHeader->u2FrameCtrl);
					dumpMemory8((uint8_t *)prWlanHeader,
						WLAN_MAC_HEADER_LEN);
				}
			} else if (prMsduInfo->ucPacketType ==
				TX_PACKET_TYPE_DATA) {
				uint8_t *pucData = NULL;

				kalGetPacketBuf(prMsduInfo->prPacket, &pucData);
				prWlanHeader = (struct WLAN_MAC_HEADER *)
					(pucData
					+ MAC_TX_RESERVED_FIELD
					+ wlanGetTxNeededHeadRoom(prAdapter));
				if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
					DBGLOG(RSN, INFO,
						"MLR txdf - 802.11data FC=0x%04x dump...\n",
						prWlanHeader->u2FrameCtrl);
					dumpMemory8((uint8_t *)prWlanHeader,
						WLAN_MAC_HEADER_QOS_LEN);
				}
			} else {
				prWlanHeader = (struct WLAN_MAC_HEADER *)
					((uintptr_t) (prMsduInfo->prPacket)
					+ MAC_TX_RESERVED_FIELD);
				DBGLOG(RSN, ERROR,
					"MLR txdf - ERROR ucPacketType = %d\n",
					prMsduInfo->ucPacketType);
				dumpMemory8((uint8_t *)prWlanHeader,
					WLAN_MAC_HEADER_LEN);
			}
#else
		    prWlanHeader = (struct WLAN_MAC_HEADER *)
			    ((uintptr_t) (prMsduInfo->prPacket)
				+ MAC_TX_RESERVED_FIELD);
#endif
			prWlanHeader->u2FrameCtrl |= MASK_FC_PROTECTED_FRAME;
		}
		break;

	default:
		break;
	}

	/* Fields in DW2~6 (Long Format) */
	if (u4PktOption & MSDU_OPT_NO_AGGREGATE)
		HAL_MAC_CONNAC5X_TXD_SET_BA_DISABLE(prTxDesc);

	if (u4PktOption & MSDU_OPT_FORCE_RTS)
		HAL_MAC_CONNAC5X_TXD_SET_PROTECT_FRAME_OPT(prTxDesc,
			PTF_OPT_RTS_CTS_PROCEDURE);

	/* SW field */
	if (u4PktOption & MSDU_OPT_SW_DURATION)
		HAL_MAC_CONNAC5X_TXD_SET_DURATION_CONTROL_BY_SW(prTxDesc);

	if (u4PktOption & MSDU_OPT_SW_PS_BIT)
		HAL_MAC_CONNAC5X_TXD_SET_SW_PM_CONTROL(prTxDesc);

	if (u4PktOption & MSDU_OPT_SW_HTC)
		HAL_MAC_CONNAC5X_TXD_SET_HTC_EXIST(prTxDesc);

	if (u4PktOption & MSDU_OPT_MANUAL_SN) {
		HAL_MAC_CONNAC5X_TXD_SET_TXD_SN_VALID(prTxDesc);
		HAL_MAC_CONNAC5X_TXD_SET_SEQUENCE_NUMBER
			(prTxDesc, prMsduInfo->u2SwSN);
	}

}

static u_int8_t isMgmtFrameByDataQueue(struct MSDU_INFO *prMsduInfo)
{
#if (CFG_TX_MGMT_BY_DATA_Q == 1)
	return (prMsduInfo->fgMgmtUseDataQ &&
		prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_FORCE_TX);
#else
	return FALSE;
#endif /* CFG_TX_MGMT_BY_DATA_Q == 1 */
}

static uint8_t nicConnac3TxGetTxDestQueue(struct ADAPTER *prAdapter,
				  struct MSDU_INFO *prMsduInfo,
				  struct BSS_INFO *prBssInfo)
{
	uint8_t ucTarPort;
	uint8_t ucTarQueueMcu;
	uint8_t ucTarQueueLmac;
	uint8_t ucWmmQueSet = 0;
	uint8_t ucControlFlag = prMsduInfo->ucControlFlag;

	if (likely(prBssInfo))
		ucWmmQueSet = prBssInfo->ucWmmQueSet;
	else
		DBGLOG(TX, ERROR, "prBssInfo is NULL\n");

	ucTarPort = nicTxGetTxDestPortIdxByTc(prMsduInfo->ucTC);

	if (ucTarPort == PORT_INDEX_MCU) {
		if (ucControlFlag & MSDU_CONTROL_FLAG_MGNT_2_CMD_QUE)
			ucTarQueueMcu = MCU_Q0_INDEX;
		else
			ucTarQueueMcu = MCU_Q1_INDEX;
#if (CFG_SUPPORT_FORCE_ALTX == 1)
		/* All Connac3 projects have enabled this option, makes FW
		 * accept value 17 (MCU_Q1_INDEX | MAC_TXQ_ALTX_0_INDEX)
		 * for always TX.
		 * Connac2 only handle 16 (MAC_TXQ_ALTX_0_INDEX) for always TX.
		 * Keep this option just in case if some projects need to
		 * disable always TX in the future.
		 */
		if (ucControlFlag & MSDU_CONTROL_FLAG_FORCE_TX)
			ucTarQueueMcu |= (uint8_t)MAC_TXQ_ALTX_0_INDEX;
#endif /* CFG_SUPPORT_FORCE_ALTX == 1 */

		return ucTarQueueMcu;
	} else { /* ucTarPort == PORT_INDEX_LMAC */
		if (isMgmtFrameByDataQueue(prMsduInfo)) {
			ucTarQueueLmac = MAC_TXQ_ALTX_0_INDEX;
		} else {
			ucTarQueueLmac =
				nicTxGetTxDestQIdxByTc(prMsduInfo->ucTC);
			ucTarQueueLmac += ucWmmQueSet * WMM_AC_INDEX_NUM;
		}

		return ucTarQueueLmac;
	}

}

/*----------------------------------------------------------------------------*/
/*!
* @brief In this function, we'll compose the Tx descriptor of the MSDU.
*
* @param prAdapter              Pointer to the Adapter structure.
* @param prMsduInfo             Pointer to the Msdu info
* @param prTxDesc               Pointer to the Tx descriptor buffer
*
* @retval VOID
*/
/*----------------------------------------------------------------------------*/
void nic_txd_v5_compose(struct ADAPTER *prAdapter, struct MSDU_INFO *prMsduInfo,
			u_int32_t u4TxDescLength, u_int8_t fgIsTemplate,
			u_int8_t *prTxDescBuffer)
{
	struct HW_MAC_CONNAC5X_TX_DESC *prTxDesc;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
#if CFG_TX_CUSTOMIZE_LTO
	struct WIFI_VAR *prWifiVar;
#endif /* CFG_TX_CUSTOMIZE_LTO */
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_STA_RECORD *prMldSta;
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
#endif
	u_int32_t u4TxDescAndPaddingLength;
	u_int8_t ucTarQueue;
	uint8_t ucEtherTypeOffsetInWord;
	uint8_t ucControlFlag;

#if CFG_TX_CUSTOMIZE_LTO
	prWifiVar = &prAdapter->rWifiVar;
#endif /* CFG_TX_CUSTOMIZE_LTO */

	prTxDesc = (struct HW_MAC_CONNAC5X_TX_DESC *) prTxDescBuffer;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldSta = mldStarecGetByStarec(prAdapter, prStaRec);
#if (CFG_SINGLE_BAND_MLSR_56 == 1)
	if (prMldSta && prMldSta->fgIsSbMlsr)
		prMldSta = NULL;
#endif /* CFG_SINGLE_BAND_MLSR_56*/
#endif

	u4TxDescAndPaddingLength = u4TxDescLength + NIC_TX_DESC_PADDING_LENGTH;

	kalMemZero(prTxDesc, u4TxDescAndPaddingLength);

#if (CFG_SUPPORT_802_11BE == 1)
	if (prAdapter->rWifiVar.ucEhtAmsduInAmpduTx)
		HAL_MAC_CONNAC5X_TXD_SET_HW_AMSDU(prTxDesc);
#endif

	/** DW0 **/
	/* Packet Format */
	ucTarQueue = nicConnac3TxGetTxDestQueue(prAdapter, prMsduInfo,
						prBssInfo);
	HAL_MAC_CONNAC5X_TXD_SET_QUEUE_INDEX(prTxDesc, ucTarQueue);

	/* Packet Format */
	HAL_MAC_CONNAC5X_TXD_SET_PKT_FORMAT(
		prTxDesc, prMsduInfo->ucPacketFormat);

	/* Ether-type offset */
	if (!prMsduInfo->fgIs802_11) {
		ucEtherTypeOffsetInWord = ((ETHER_HEADER_LEN - ETHER_TYPE_LEN)
				+ prAdapter->chip_info->pse_header_length) >> 1;

		HAL_MAC_CONNAC5X_TXD_SET_ETHER_TYPE_OFFSET(
			prTxDesc, ucEtherTypeOffsetInWord);
	} else {
		ucEtherTypeOffsetInWord =
			(prAdapter->chip_info->pse_header_length
			+ prMsduInfo->ucMacHeaderLength
			+ prMsduInfo->ucLlcLength) >> 1;

		HAL_MAC_CONNAC5X_TXD_SET_ETHER_TYPE_OFFSET(
			prTxDesc, ucEtherTypeOffsetInWord);
#if CFG_SUPPORT_MLR
		if (MLR_CHECK_IF_MSDU_IS_FRAG(prMsduInfo))
			MLR_DBGLOG(prAdapter, RSN, WARN,
				"MLR txdc - 802.11 Ether-type offset[%u] [PseHeader:%u, MacHeader:%u, HeaderPading:2 LLC:%u]\n",
				ucEtherTypeOffsetInWord,
				prAdapter->chip_info->pse_header_length,
				prMsduInfo->ucMacHeaderLength,
				prMsduInfo->ucLlcLength);
#endif
	}

	/** DW1 **/
	/* MLDID-legacy */
	prMsduInfo->ucWlanIndex = nicTxGetWlanIdx(prAdapter,
		prMsduInfo->ucBssIndex, prMsduInfo->ucStaRecIndex);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (prMldSta) {
		HAL_MAC_CONNAC5X_TXD_SET_MLD_ID(
			prTxDesc,
			(prMsduInfo->ucUserPriority & 0xf) % 2 == 0 ?
				prMldSta->u2PrimaryMldId : prMldSta->u2SecondMldId);
	} else
#endif
	{
		HAL_MAC_CONNAC5X_TXD_SET_MLD_ID(prTxDesc,
			prMsduInfo->ucWlanIndex);
	}

	/* Header format */
	if (prMsduInfo->fgIs802_11) {
		HAL_MAC_CONNAC5X_TXD_SET_HEADER_FORMAT(
			prTxDesc, HEADER_FORMAT_802_11_NORMAL_MODE);
		HAL_MAC_CONNAC5X_TXD_SET_802_11_HEADER_LENGTH(
			prTxDesc, (prMsduInfo->ucMacHeaderLength >> 1));
	} else {
		HAL_MAC_CONNAC5X_TXD_SET_HEADER_FORMAT(
			prTxDesc, HEADER_FORMAT_NON_802_11);
		HAL_MAC_CONNAC5X_TXD_SET_ETHERNET_II(prTxDesc);
	}
	HAL_MAC_CONNAC5X_TXD_SET_HEADER_PADDING(
		prTxDesc, NIC_TX_DESC_HEADER_PADDING_LENGTH);

	/* TID mgmt type */
	if (prMsduInfo->fgIs802_11) {
		HAL_MAC_CONNAC5X_TXD_SET_TID_MGMT_TYPE(prTxDesc,
			TYPE_NORMAL_MANAGEMENT);
	} else {
		HAL_MAC_CONNAC5X_TXD_SET_TID_MGMT_TYPE(prTxDesc,
			prMsduInfo->ucUserPriority);
	}

	if (prBssInfo) {
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
		if (prMldBssInfo &&
		    prMldBssInfo->ucOmRemapIdx != OM_REMAP_IDX_NONE) {
			HAL_MAC_CONNAC5X_TXD_SET_OM_MAP(prTxDesc);
			HAL_MAC_CONNAC5X_TXD_SET_OWN_MAC_INDEX(
				prTxDesc, prMldBssInfo->ucOmRemapIdx);
		} else
#endif
			/* Own MAC */
			HAL_MAC_CONNAC5X_TXD_SET_OWN_MAC_INDEX(
				prTxDesc, prBssInfo->ucOwnMacIndex);

		/* TGID should align HW band idx */
		HAL_MAC_CONNAC5X_TXD_SET_TGID(prTxDesc, prBssInfo->eHwBandIdx);
	}

	/** DW2 **/
	/* Type */
	if (prMsduInfo->fgIs802_11) {
		struct WLAN_MAC_HEADER *prWlanHeader = NULL;
#if CFG_SUPPORT_MLR
		if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_MGMT) {
			prWlanHeader = (struct WLAN_MAC_HEADER *)(
				(uintptr_t) (prMsduInfo->prPacket) +
				MAC_TX_RESERVED_FIELD);
			if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
				DBGLOG(RSN, INFO,
					"MLR txdc - 802.11mgmt FC=0x%04x SC=0x%04x dump...\n",
					prWlanHeader->u2FrameCtrl,
					prWlanHeader->u2SeqCtrl);
				dumpMemory8((uint8_t *)prWlanHeader,
					WLAN_MAC_HEADER_LEN);
			}
		} else if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_DATA) {
			uint8_t *pucData = NULL;

			kalGetPacketBuf(prMsduInfo->prPacket, &pucData);
			prWlanHeader = (struct WLAN_MAC_HEADER *)
				(pucData
				+ MAC_TX_RESERVED_FIELD
				+ wlanGetTxNeededHeadRoom(prAdapter));
			if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
				DBGLOG(RSN, INFO,
					"MLR txdc - 802.11data FC=0x%04x SC=0x%04x dump...\n",
					prWlanHeader->u2FrameCtrl,
					prWlanHeader->u2SeqCtrl);
				dumpMemory8((uint8_t *)prWlanHeader,
					WLAN_MAC_HEADER_QOS_LEN);
			}
		} else {
			prWlanHeader = (struct WLAN_MAC_HEADER *)(
				(uintptr_t) (prMsduInfo->prPacket) +
				MAC_TX_RESERVED_FIELD);
			DBGLOG(RSN, ERROR,
				"MLR txdc - ERROR ucPacketType = %d\n",
				prMsduInfo->ucPacketType);
			dumpMemory8((uint8_t *)prWlanHeader,
				WLAN_MAC_HEADER_LEN);
		}
#else
		prWlanHeader = (struct WLAN_MAC_HEADER *)(
			(uintptr_t) (prMsduInfo->prPacket) +
			MAC_TX_RESERVED_FIELD);
#endif

		HAL_MAC_CONNAC5X_TXD_SET_TYPE(
			prTxDesc,
			(prWlanHeader->u2FrameCtrl & MASK_FC_TYPE) >> 2);

		HAL_MAC_CONNAC5X_TXD_SET_SUB_TYPE(
			prTxDesc,
			(prWlanHeader->u2FrameCtrl & MASK_FC_SUBTYPE)
				>> OFFSET_OF_FC_SUBTYPE);
	} else {
		HAL_MAC_CONNAC5X_TXD_SET_TYPE(
			prTxDesc,
			MAC_FRAME_TYPE_DATA >> 2);

		HAL_MAC_CONNAC5X_TXD_SET_SUB_TYPE(
			prTxDesc,
			(prStaRec && prStaRec->fgIsQoS) ?
				MAC_FRAME_QOS_DATA >> 4 : MAC_FRAME_DATA >> 4);
	}

	/* Remaining TX time */
	if (!(prMsduInfo->u4Option & MSDU_OPT_MANUAL_LIFE_TIME)) {
		uint32_t u4RemainingLifetime = 0;

		if (prMsduInfo->ucStaRecIndex == STA_REC_INDEX_BMCAST)
			/* BMC packet would be enqueued to BMC_TC_INDEX,
			 * which currently is TC1_INDEX.
			 * Force BMC packets set remaining tx time to
			 * NIC_TX_BMC_REMAINING_TX_TIME to avoid packets cannot
			 * be released when MAC is in abnormal status.
			 */
			u4RemainingLifetime = NIC_TX_BMC_REMAINING_TX_TIME;
		else
			u4RemainingLifetime =
				nicTxGetRemainingTxTimeByTc(prMsduInfo->ucTC);

#if CFG_TX_CUSTOMIZE_LTO
		if (IS_FEATURE_ENABLED(prWifiVar->ucEnableConfigLTO) &&
			nicTxEnableLTO(prAdapter, prMsduInfo, prBssInfo))
			u4RemainingLifetime = prWifiVar->u4LTOValue;
#endif /* CFG_TX_CUSTOMIZE_LTO */

		prMsduInfo->u4RemainingLifetime = u4RemainingLifetime;

	}

	HAL_MAC_CONNAC5X_TXD_SET_REMAINING_LIFE_TIME_IN_MS(
		prTxDesc, prMsduInfo->u4RemainingLifetime);

	/* Power Offset */
	HAL_MAC_CONNAC5X_TXD_SET_POWER_OFFSET(
		prTxDesc, prMsduInfo->cPowerOffset);

#if CFG_SUPPORT_MLR
	if (MLR_CHECK_IF_MSDU_IS_FRAG(prMsduInfo)) {
		uint8_t *pucData = NULL;

		HAL_MAC_CONNAC5X_TXD_SET_FRAG_PACKET_POS(prTxDesc,
			prMsduInfo->eFragPos);

		kalGetPacketBuf(prMsduInfo->prPacket, &pucData);
		MLR_DBGLOG(prAdapter, REQ, DEBUG,
			"MLR txdc - PID=%d SeqNo=%d prPacket=%p prPacket->data=%p u2FrameLength=%d eFragPos=%d\n",
			prMsduInfo->ucPID,
			prMsduInfo->ucTxSeqNum,
			prMsduInfo->prPacket,
			pucData,
			prMsduInfo->u2FrameLength,
			prMsduInfo->eFragPos);
	}
#endif

	/** DW3 **/
	/* Tx count limit */
	if (!(prMsduInfo->u4Option & MSDU_OPT_MANUAL_RETRY_LIMIT)) {
		/* Note: BMC packet retry limit is set to unlimited */
		prMsduInfo->ucRetryLimit =
			nicTxGetTxCountLimitByTc(prMsduInfo->ucTC);
	}
#if (CFG_SUPPORT_LOWLATENCY_MODE == 1)
	if (prAdapter->rWifiVar.u2LlmDataRtyCnt != 0 &&
	    nicTxIsLowLatencyCriticalData(prAdapter, prMsduInfo))
		prMsduInfo->ucRetryLimit = prAdapter->rWifiVar.u2LlmDataRtyCnt;
#endif
	HAL_MAC_CONNAC5X_TXD_SET_REMAINING_TX_COUNT(
		prTxDesc, prMsduInfo->ucRetryLimit);

	/* BMC packet */
	if (prMsduInfo->ucStaRecIndex == STA_REC_INDEX_BMCAST) {
		HAL_MAC_CONNAC5X_TXD_SET_BMC(prTxDesc);
		/* Must set No ACK to mask retry bit in FC */
		HAL_MAC_CONNAC5X_TXD_SET_NO_ACK(prTxDesc);
	}

	/* HW AMSDU CAP */
	if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_MGMT)
		HAL_MAC_CONNAC5X_TXD_UNSET_HW_AMSDU(prTxDesc);

	/* Protection */
	if (secIsProtectedFrame(prAdapter, prMsduInfo, prStaRec)) {
		/* Update Packet option, */
		/* PF bit will be set in nicTxFillDescByPktOption() */
		if ((prStaRec
			&& prStaRec->fgTransmitKeyExist) || fgIsTemplate) {
			DBGLOG_LIMITED(RSN, TRACE,
				"Set MSDU_OPT_PROTECTED_FRAME\n");
			nicTxConfigPktOption(
				prMsduInfo, MSDU_OPT_PROTECTED_FRAME, TRUE);

			if (prMsduInfo->fgIs802_1x &&
			    prMsduInfo->fgIs802_1x_NonProtected) {
				nicTxConfigPktOption(
					prMsduInfo,
					MSDU_OPT_PROTECTED_FRAME, FALSE);
				DBGLOG(RSN, LOUD,
					"Pairwise EAPoL not protect!\n");
			}
		} else if (prMsduInfo->ucStaRecIndex
				== STA_REC_INDEX_BMCAST) {/* BMC packet */
			nicTxConfigPktOption(
				prMsduInfo, MSDU_OPT_PROTECTED_FRAME, TRUE);
			DBGLOG(RSN, LOUD, "Protect BMC frame!\n");
		}
	}

	/* Update Packet option */
	nic_txd_v5_fill_by_pkt_option(prAdapter, prMsduInfo, prTxDesc);

	/** DW5 **/
	/* PID */
	if (prMsduInfo->pfTxDoneHandler) {
		prMsduInfo->ucPID = nicTxAssignPID(prAdapter,
				prMsduInfo->ucWlanIndex,
				prMsduInfo->ucPacketType); /* 0/1: data/mgmt */
		HAL_MAC_CONNAC5X_TXD_SET_PID(prTxDesc, prMsduInfo->ucPID);
		HAL_MAC_CONNAC5X_TXD_SET_TXS_TO_MCU(prTxDesc);
		/* TXS is MPDU based, AMSDU will cause TX skb leak in driver */
		HAL_MAC_CONNAC5X_TXD_UNSET_HW_AMSDU(prTxDesc);
		/* Save for matching TXS */
		prMsduInfo->ucTxdWlanIdx =
			HAL_MAC_CONNAC5X_TXD_GET_MLD_ID(prTxDesc);

		DBGLOG(TX, TEMP,
			"TXS MSDU: w/w'/p/t/up=%u/%u/%u/%u/%u\n",
			prMsduInfo->ucWlanIndex,
			prMsduInfo->ucTxdWlanIdx,
			prMsduInfo->ucPID,
			prMsduInfo->ucTC,
			prMsduInfo->ucUserPriority);
	} else if (prAdapter->rWifiVar.ucDataTxDone == 2) {
		/* Log mode: only TxS to FW, no event to driver */
		HAL_MAC_CONNAC5X_TXD_SET_PID(
			prTxDesc, NIC_TX_DESC_PID_RESERVED);
		HAL_MAC_CONNAC5X_TXD_SET_TXS_TO_MCU(prTxDesc);
	}

	ucControlFlag = prMsduInfo->ucControlFlag;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	/* altx set TGID and force link */
	if (prMldSta && ucControlFlag & MSDU_CONTROL_FLAG_FORCE_LINK)
		HAL_MAC_CONNAC5X_TXD_SET_FORCE_ASSIGN_LINK(prTxDesc);
#endif /* CFG_SUPPORT_802_11BE_MLO */

	/** DW6 **/
	/* Disable MLD to link address translation */
	if (ucControlFlag &
	    (MSDU_CONTROL_FLAG_FORCE_TX | MSDU_CONTROL_FLAG_DIS_MAT))
		HAL_MAC_CONNAC5X_TXD_SET_DIS_MAT(prTxDesc);

#if CFG_BMC_DISABLE_RETRY_RTS
	/* BMC packet disable retry RTS*/
	if (prMsduInfo->ucStaRecIndex == STA_REC_INDEX_BMCAST
		|| prMsduInfo->ucStaRecIndex == STA_REC_INDEX_NOT_FOUND)
		HAL_MAC_CONNAC5X_TXD_SET_PROTECT_FRAME_OPT(prTxDesc,
			PTF_OPT_DISABLE_RTS);
#endif /* CFG_TXD_DISABLE_RTS */

	/* Msdu count */
	HAL_MAC_CONNAC5X_TXD_SET_MSDU_COUNT(prTxDesc, 1);

	if (prMsduInfo->ucStaRecIndex != STA_REC_INDEX_BMCAST)
		HAL_MAC_CONNAC5X_TXD_SET_DA_SRC_SELECTION(prTxDesc);

	/** DW7 **/
	HAL_MAC_CONNAC5X_TXD_SET_TXD_LENGTH(prTxDesc, TXD_LEN_1_PAGE);

	/* Fix rate */
	switch (prMsduInfo->ucRateMode) {
	case MSDU_RATE_MODE_MANUAL_DESC:
		HAL_MAC_CONNAC5X_TXD_SET_FIXED_RATE_ENABLE(prTxDesc);
		HAL_MAC_CONNAC5X_TXD_SET_FIXED_RATE_IDX(prTxDesc,
						prMsduInfo->u4FixedRateOption);
		HAL_MAC_CONNAC5X_TXD_SET_FR_BW(prTxDesc, 0x8);
		/* PP would regard TXD invalid if FR=1 && HW_AMSDU_CAP==1 */
		HAL_MAC_CONNAC5X_TXD_UNSET_HW_AMSDU(prTxDesc);

		if (prMsduInfo->ucPacketType != TX_PACKET_TYPE_MGMT)
			HAL_MAC_CONNAC5X_TXD_SET_PROTECT_FRAME_OPT(prTxDesc,
				PTF_OPT_RTS_CTS_PROCEDURE);
		break;
	case MSDU_RATE_MODE_MANUAL_CR:
	case MSDU_RATE_MODE_AUTO:
	default:
		break;
	}

	if (prMsduInfo->pfTxDoneHandler) {
		DBGLOG(TX, DEBUG,
			"TX[%s] WIDX[%u] PID[%u] Rate mode[%d], RateIdx=%u\n",
			TXS_PACKET_TYPE[prMsduInfo->ucPktType],
			prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
			prMsduInfo->ucRateMode, prMsduInfo->u4FixedRateOption);
	}
}

#if CFG_SUPPORT_MLR
static uint8_t setMlrFixedRate(struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucRateIdx;

	if (prMsduInfo->eSrc == TX_PACKET_OS) {
		ucRateIdx = FIXED_RATE_INDEX_MLR_MCS0_SPE_IDX_FAVOR_WTBL;
	} else if (prMsduInfo->eSrc == TX_PACKET_MGMT) {
		ucRateIdx = FIXED_RATE_INDEX_MLR_MCS0_SPE_IDX_FAVOR_TXD;
	} else {
		ucRateIdx = FIXED_RATE_INDEX_OFDM_6M;
		DBGLOG(TX, WARN, "MLR rate - Don't use MLR rate\n");
	}
	return ucRateIdx;
}

static uint8_t setMlrpFixedRate(struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucRateIdx;

	if (prMsduInfo->eSrc == TX_PACKET_OS) {
		ucRateIdx = FIXED_RATE_INDEX_MLRP_MCS3_SPE_IDX_FAVOR_WTBL;
	} else if (prMsduInfo->eSrc == TX_PACKET_MGMT) {
		ucRateIdx = FIXED_RATE_INDEX_MLRP_MCS3_SPE_IDX_FAVOR_TXD;
	} else {
		ucRateIdx = FIXED_RATE_INDEX_OFDM_6M;
		DBGLOG(TX, WARN, "MLRP rate - Don't use MLRP rate\n");
	}
	return ucRateIdx;
}

static uint8_t setAlrFixedRate(struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucRateIdx;

	if (prMsduInfo->eSrc == TX_PACKET_OS) {
		ucRateIdx = FIXED_RATE_INDEX_ALR_MCS2_SPE_IDX_FAVOR_WTBL;
	} else if (prMsduInfo->eSrc == TX_PACKET_MGMT) {
		ucRateIdx = FIXED_RATE_INDEX_ALR_MCS2_SPE_IDX_FAVOR_TXD;
	} else {
		ucRateIdx = FIXED_RATE_INDEX_OFDM_6M;
		DBGLOG(TX, WARN, "ALR rate - Don't use ALR rate\n");
	}
	return ucRateIdx;
}
#endif /* CFG_SUPPORT_MLR */

static uint8_t getSpeIdx(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucSpeIdx = 0;
#if ((CFG_SISO_SW_DEVELOP == 1) || (CFG_SUPPORT_SPE_IDX_CONTROL == 1))
	struct BSS_INFO *prBssInfo;
	enum ENUM_WF_PATH_FAVOR_T eWfPathFavor;

	/* Update spatial extension index setting */
	eWfPathFavor = wlanGetAntPathType(prAdapter, ENUM_WF_NON_FAVOR);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);

	if (prBssInfo) {
		ucSpeIdx = wlanGetSpeIdx(prAdapter, prBssInfo->ucBssIndex,
				eWfPathFavor);
	}
#endif
	return ucSpeIdx;
}

void nic_txd_v5_set_pkt_fixed_rate_option(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint16_t u2RateCode,
	uint8_t ucBandwidth,
	u_int8_t fgShortGI,
	u_int8_t fgDynamicBwRts)
{
	uint8_t ucRateIdx;
	uint8_t ucSpeIdx = 0;

	ucSpeIdx = getSpeIdx(prAdapter, prMsduInfo);

	/* Convert rate code to pre-defined rate index */
	if (ucSpeIdx == 0x18) {
		switch (u2RateCode) {
		case RATE_CCK_1M_LONG: /* 0x0000 */
			ucRateIdx = FIXED_RATE_INDEX_CCK_1M_SPE_IDX_FAVOR_TXD;
			break;

		case RATE_OFDM_6M: /* 0x004B */
			ucRateIdx = FIXED_RATE_INDEX_OFDM_6M_SPE_IDX_FAVOR_TXD;
			break;

		case RATE_MLR_1_5M: /* 0x0140 */
			ucRateIdx = FIXED_RATE_INDEX_MLR_MCS0_SPE_IDX_FAVOR_TXD;
			break;

		case RATE_MLRP_0_375M: /* 0x0183 */
			ucRateIdx =
				FIXED_RATE_INDEX_MLRP_MCS3_SPE_IDX_FAVOR_TXD;
			break;

		case RATE_ALR_0_75M: /* 0x01C2 */
			ucRateIdx = FIXED_RATE_INDEX_ALR_MCS2_SPE_IDX_FAVOR_TXD;
			break;

#if (CFG_SUPPORT_HE_ER == 1)
		case RATE_HE_ER_DCM_MCS_0: /* 0x0250 */
			ucRateIdx = FIXED_RATE_INDEX_HE_ER_DMC_MCS0;
			break;
#endif
		default:
			ucRateIdx = FIXED_RATE_INDEX_OFDM_6M_SPE_IDX_FAVOR_TXD;
			DBGLOG(TX, WARN,
				"No mapped rate speidx=0x%02x, 0x%04x\n",
				ucSpeIdx, u2RateCode);
			break;
		}
	} else {
		switch (u2RateCode) {
		case RATE_CCK_1M_LONG: /* 0x0000 */
			ucRateIdx = FIXED_RATE_INDEX_CCK_1M;
			break;

		case RATE_OFDM_6M: /* 0x004B */
			ucRateIdx = FIXED_RATE_INDEX_OFDM_6M;
			break;

		case RATE_OFDM_24M: /* 0x0049 */
			ucRateIdx = FIXED_RATE_INDEX_OFDM_24M;
			break;

		case RATE_MM_MCS_0: /* 0x0080 */
			ucRateIdx = FIXED_RATE_INDEX_HT_MCS0;
			break;

		case RATE_VHT_MCS_0: /* 0x0100 */
			ucRateIdx = FIXED_RATE_INDEX_VHT_MCS0;
			break;

#if CFG_SUPPORT_MLR
		case RATE_MLR_1_5M: /* 0x0140 */
		case RATE_MLR_3M: /* 0x0141 */
			ucRateIdx = setMlrFixedRate(prMsduInfo);
			break;

		case RATE_MLRP_0_375M: /* 0x0183 */
			ucRateIdx = setMlrpFixedRate(prMsduInfo);
			break;
		case RATE_ALR_0_75M: /* 0x01C2 */
			ucRateIdx = setAlrFixedRate(prMsduInfo);
			break;
#endif
#if (CFG_SUPPORT_HE_ER == 1)
		case RATE_HE_ER_DCM_MCS_0: /* 0x0250 */
			ucRateIdx = FIXED_RATE_INDEX_HE_ER_DMC_MCS0;
			break;
#endif
		default:
			ucRateIdx = FIXED_RATE_INDEX_OFDM_6M;
			DBGLOG(TX, WARN,
				"No mapped rate speidx=0x%02x, 0x%04x\n",
				ucSpeIdx, u2RateCode);
			break;
		}
	}

	DBGLOG(TX, TRACE, "SpeIdx=0x%02x, RateCode=0x%04x -> RateIdx=%u\n",
		ucSpeIdx, u2RateCode, ucRateIdx);

	prMsduInfo->u4FixedRateOption = ucRateIdx;
	prMsduInfo->ucRateMode = MSDU_RATE_MODE_MANUAL_DESC;
}

void nic_txd_v5_set_hw_amsdu_template(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	u_int8_t ucTid,
	u_int8_t fgSet)
{
	struct HW_MAC_CONNAC5X_TX_DESC *prTxDesc;

	DBGLOG(QM, DEBUG,
		"Update HW Amsdu field of TXD template for STA[%u] Tid[%u]\n",
		prStaRec->ucIndex, ucTid);

	if (prStaRec->aprTxDescTemplate[ucTid]) {
		prTxDesc =
			(struct HW_MAC_CONNAC5X_TX_DESC *)
			prStaRec->aprTxDescTemplate[ucTid];
		if (fgSet)
			HAL_MAC_CONNAC5X_TXD_SET_HW_AMSDU(prTxDesc);
		else
			HAL_MAC_CONNAC5X_TXD_UNSET_HW_AMSDU(prTxDesc);
	}
}
#endif /* CFG_SUPPORT_CONNAC5X == 1 */
