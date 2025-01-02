/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/nic/nic_tx.c#2
 */

/*! \file   nic_tx.c
 *    \brief  Functions that provide TX operation in NIC Layer.
 *
 *    This file provides TX functions which are responsible for both Hardware
 *    and Software Resource Management and keep their Synchronization.
 */


#if (CFG_SUPPORT_CONNAC2X == 1)
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#if (CFG_SUPPORT_SNIFFER_RADIOTAP == 1)
#include "radiotap.h"
#endif

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

uint16_t nic_rxd_v2_get_rx_byte_count(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC2X_RX_STATUS_GET_RX_BYTE_CNT(
		(struct HW_MAC_CONNAC2X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v2_get_packet_type(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC2X_RX_STATUS_GET_PKT_TYPE(
		(struct HW_MAC_CONNAC2X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v2_get_wlan_idx(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC2X_RX_STATUS_GET_WLAN_IDX(
		(struct HW_MAC_CONNAC2X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v2_get_sec_mode(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC2X_RX_STATUS_GET_SEC_MODE(
		(struct HW_MAC_CONNAC2X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v2_get_sw_class_error_bit(
	void *prRxStatus)
{
	struct HW_MAC_CONNAC2X_RX_DESC *prRxD;

	prRxD = (struct HW_MAC_CONNAC2X_RX_DESC *)prRxStatus;
	if (HAL_MAC_CONNAC2X_RX_STATUS_IS_SW_DEFINE_RX_CLASSERR(prRxD)) {
		DBGLOG(RSN, ERROR,
		       "RX_CLASSERR: RXD.DW2=0x%x\n",
		       prRxD->u4DW2);
		return TRUE;
	} else
		return FALSE;
}

uint8_t nic_rxd_v2_get_ch_num(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC2X_RX_STATUS_GET_CHNL_NUM(
		(struct HW_MAC_CONNAC2X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v2_get_rf_band(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC2X_RX_STATUS_GET_RF_BAND(
		(struct HW_MAC_CONNAC2X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v2_get_tcl(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC2X_RX_STATUS_GET_TCL(
		(struct HW_MAC_CONNAC2X_RX_DESC *)prRxStatus);
}

uint8_t nic_rxd_v2_get_ofld(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC2X_RX_STATUS_GET_OFLD(
		(struct HW_MAC_CONNAC2X_RX_DESC *)prRxStatus);
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
void nic_rxd_v2_fill_rfb(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct mt66xx_chip_info *prChipInfo;
	struct HW_MAC_CONNAC2X_RX_DESC *prRxStatus;

	uint32_t u4PktLen = 0;
	/* UINT_32 u4MacHeaderLen; */
	uint32_t u4HeaderOffset;
	uint16_t u2RxStatusOffset;

	DEBUGFUNC("nicRxFillRFB");

	prChipInfo = prAdapter->chip_info;
	prRxStatus = prSwRfb->prRxStatus;

	u4PktLen = (uint32_t) HAL_MAC_CONNAC2X_RX_STATUS_GET_RX_BYTE_CNT(
				prRxStatus);
	u4HeaderOffset = (uint32_t) (
		HAL_MAC_CONNAC2X_RX_STATUS_GET_HEADER_OFFSET(prRxStatus));

	u2RxStatusOffset = prChipInfo->rxd_size;
	prSwRfb->ucGroupVLD =
		(uint8_t) HAL_MAC_CONNAC2X_RX_STATUS_GET_GROUP_VLD(prRxStatus);
	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4)) {
		prSwRfb->prRxStatusGroup4 = (struct HW_MAC_RX_STS_GROUP_4 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_4);

	}
	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_1)) {
		prSwRfb->prRxStatusGroup1 = (struct HW_MAC_RX_STS_GROUP_1 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_1);

	}
	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_2)) {
		prSwRfb->prRxStatusGroup2 = (struct HW_MAC_RX_STS_GROUP_2 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_2);

	}
	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3)) {
		prSwRfb->prRxStatusGroup3 = (void *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_3_V2);
	}

	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_5)) {
		prSwRfb->prRxStatusGroup5 = (struct HW_MAC_RX_STS_GROUP_5 *)
			((uint8_t *) prRxStatus + u2RxStatusOffset);
		u2RxStatusOffset += prChipInfo->group5_size;
	}


	prSwRfb->u2RxStatusOffst = u2RxStatusOffset;
	prSwRfb->pvHeader = (uint8_t *) prRxStatus +
		u2RxStatusOffset + u4HeaderOffset;
	prSwRfb->u2RxByteCount = u4PktLen;
	prSwRfb->u2PacketLen = (uint16_t) (u4PktLen -
		(u2RxStatusOffset + u4HeaderOffset));
	prSwRfb->u2HeaderLen = (uint16_t)
		HAL_MAC_CONNAC2X_RX_STATUS_GET_HEADER_LEN(prRxStatus);
	prSwRfb->ucWlanIdx =
		(uint8_t) HAL_MAC_CONNAC2X_RX_STATUS_GET_WLAN_IDX(prRxStatus);
	prSwRfb->ucStaRecIdx = secGetStaIdxByWlanIdx(prAdapter,
		(uint8_t) HAL_MAC_CONNAC2X_RX_STATUS_GET_WLAN_IDX(prRxStatus));
	prSwRfb->prStaRec = cnmGetStaRecByIndex(prAdapter,
		prSwRfb->ucStaRecIdx);
	prSwRfb->ucTid =
		(uint8_t) HAL_MAC_CONNAC2X_RX_STATUS_GET_TID(prRxStatus);
	prSwRfb->fgHdrTran =
		HAL_MAC_CONNAC2X_RX_STATUS_IS_HEADER_TRAN(prRxStatus);
	prSwRfb->ucPayloadFormat =
		HAL_MAC_CONNAC2X_RX_STATUS_GET_PAYLOAD_FORMAT(prRxStatus);
	prSwRfb->fgIcvErr =
		HAL_MAC_CONNAC2X_RX_STATUS_IS_ICV_ERROR(prRxStatus);
	prSwRfb->ucSecMode =
		HAL_MAC_CONNAC2X_RX_STATUS_GET_SEC_MODE(prRxStatus);
	prSwRfb->ucOFLD = HAL_MAC_CONNAC2X_RX_STATUS_GET_OFLD(prRxStatus);
	prSwRfb->fgIsBC = HAL_MAC_CONNAC2X_RX_STATUS_IS_BC(prRxStatus);
	prSwRfb->fgIsMC = HAL_MAC_CONNAC2X_RX_STATUS_IS_MC(prRxStatus);
	prSwRfb->fgIsCipherMS =
		HAL_MAC_CONNAC2X_RX_STATUS_IS_CIPHER_MISMATCH(prRxStatus);
	prSwRfb->fgIsCipherLenMS =
		HAL_MAC_CONNAC2X_RX_STATUS_IS_CLM_ERROR(prRxStatus);
	prSwRfb->fgIsFrag = HAL_MAC_CONNAC2X_RX_STATUS_IS_FRAG(prRxStatus);
	prSwRfb->fgIsFCS = HAL_MAC_CONNAC2X_RX_STATUS_IS_FCS_ERROR(prRxStatus);
	prSwRfb->fgIsSecDone =
		HAL_MAC_CONNAC2X_RX_STATUS_IS_SEC_DONE(prRxStatus);
	prSwRfb->fgIsAmpdu = HAL_MAC_CONNAC2X_RX_STATUS_IS_NAMP(prRxStatus);
	prSwRfb->ucRxvSeqNo =
		HAL_MAC_CONNAC2X_RX_STATUS_GET_RXV_SEQ_NO(prRxStatus);
	prSwRfb->ucChnlNum =
		HAL_MAC_CONNAC2X_RX_STATUS_GET_CHNL_NUM(prRxStatus);
	prSwRfb->ucKeyID = HAL_MAC_CONNAC2X_RX_STATUS_GET_KID(prRxStatus);
	prSwRfb->ucHeaderOffset =
		HAL_MAC_CONNAC2X_RX_STATUS_GET_HEADER_OFFSET(prRxStatus);

#if 0
	if (prHifRxHdr->ucReorder &
	    HIF_RX_HDR_80211_HEADER_FORMAT) {
		prSwRfb->u4HifRxHdrFlag |= HIF_RX_HDR_FLAG_802_11_FORMAT;
		DBGLOG(RX, TRACE, "HIF_RX_HDR_FLAG_802_11_FORMAT\n");
	}

	if (prHifRxHdr->ucReorder & HIF_RX_HDR_DO_REORDER) {
		prSwRfb->u4HifRxHdrFlag |= HIF_RX_HDR_FLAG_DO_REORDERING;
		DBGLOG(RX, TRACE, "HIF_RX_HDR_FLAG_DO_REORDERING\n");

		/* Get Seq. No and TID, Wlan Index info */
		if (prHifRxHdr->u2SeqNoTid & HIF_RX_HDR_BAR_FRAME) {
			prSwRfb->u4HifRxHdrFlag |= HIF_RX_HDR_FLAG_BAR_FRAME;
			DBGLOG(RX, TRACE, "HIF_RX_HDR_FLAG_BAR_FRAME\n");
		}

		prSwRfb->u2SSN = prHifRxHdr->u2SeqNoTid &
				 HIF_RX_HDR_SEQ_NO_MASK;
		prSwRfb->ucTid = (uint8_t) ((prHifRxHdr->u2SeqNoTid &
					     HIF_RX_HDR_TID_MASK)
					    >> HIF_RX_HDR_TID_OFFSET);
		DBGLOG(RX, TRACE, "u2SSN = %d, ucTid = %d\n",
		       prSwRfb->u2SSN, prSwRfb->ucTid);
	}

	if (prHifRxHdr->ucReorder & HIF_RX_HDR_WDS) {
		prSwRfb->u4HifRxHdrFlag |= HIF_RX_HDR_FLAG_AMP_WDS;
		DBGLOG(RX, TRACE, "HIF_RX_HDR_FLAG_AMP_WDS\n");
	}
#endif
}

u_int8_t nic_rxd_v2_sanity_check(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct mt66xx_chip_info *prChipInfo;
	struct HW_MAC_CONNAC2X_RX_DESC *prRxStatus;
	u_int8_t fgDrop = FALSE;

	prChipInfo = prAdapter->chip_info;
	prRxStatus = (struct HW_MAC_CONNAC2X_RX_DESC *)prSwRfb->prRxStatus;

	if (!HAL_MAC_CONNAC2X_RX_STATUS_IS_FCS_ERROR(prRxStatus)
#if CFG_SUPPORT_TKIP_MICERROR_DETECTION
		&& !HAL_MAC_CONNAC2X_RX_STATUS_IS_TKIP_MIC_ERROR(prRxStatus)
#endif /* CFG_SUPPORT_TKIP_MICERROR_DETECTION */
	) {
		if (!HAL_MAC_CONNAC2X_RX_STATUS_IS_NAMP(prRxStatus)
			&& !HAL_MAC_CONNAC2X_RX_STATUS_IS_DAF(prRxStatus))
			prSwRfb->fgReorderBuffer = TRUE;
		else if (HAL_MAC_CONNAC2X_RX_STATUS_IS_NDATA(prRxStatus))
			prSwRfb->fgDataFrame = FALSE;
		else if (HAL_MAC_CONNAC2X_RX_STATUS_IS_FRAG(prRxStatus))
			prSwRfb->fgFragFrame = TRUE;
	} else {
		uint8_t ucBssIndex =
			secGetBssIdxByWlanIdx(prAdapter,
			HAL_MAC_CONNAC2X_RX_STATUS_GET_WLAN_IDX(prRxStatus));

		fgDrop = TRUE;
		if (!HAL_MAC_CONNAC2X_RX_STATUS_IS_ICV_ERROR(prRxStatus)
		    && HAL_MAC_CONNAC2X_RX_STATUS_IS_TKIP_MIC_ERROR(
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
				DBGLOG(RSN, EVENT, "MIC_ERR_PKT\n");
				rsnTkipHandleMICFailure(prAdapter, prStaRec, 0);
			}
		} else if (HAL_MAC_CONNAC2X_RX_STATUS_IS_LLC_MIS(prRxStatus)
			 && !HAL_MAC_CONNAC2X_RX_STATUS_IS_ERROR(prRxStatus)
			 && !FEAT_SUP_LLC_VLAN_RX(prChipInfo)) {
			uint16_t *pu2EtherType;

			pu2EtherType = (uint16_t *)
				((uint8_t *)prSwRfb->pvHeader +
				2 * MAC_ADDR_LEN);

			/* If ethernet type is VLAN, do not drop it.
			 * Pass up to driver process
			 */
			if (prSwRfb->u2HeaderLen >= ETH_HLEN
			    && *pu2EtherType == NTOHS(ETH_P_VLAN))
				fgDrop = FALSE;

#if CFG_SUPPORT_AMSDU_ATTACK_DETECTION
			/*
			 * let qmAmsduAttackDetection check this subframe
			 * before drop it
			 */
			if (prSwRfb->ucPayloadFormat
				== RX_PAYLOAD_FORMAT_FIRST_SUB_AMSDU) {
				DBGLOG(RX, INFO, "LLC_MIS:%d\n",
					HAL_MAC_CONNAC2X_RX_STATUS_IS_LLC_MIS(
					prRxStatus));
				fgDrop = FALSE;
			}
#endif /* CFG_SUPPORT_AMSDU_ATTACK_DETECTION */

		}
	}

	/* Drop plain text during security connection */
	if (HAL_MAC_CONNAC2X_RX_STATUS_IS_CIPHER_MISMATCH(prRxStatus)
		&& (prSwRfb->fgDataFrame == TRUE)) {
		uint16_t *pu2EtherType;

		DBGLOG(RSN, INFO,
			"HAL_MAC_CONNAC2X_RX_STATUS_IS_CIPHER_MISMATCH\n");

		nicRxFillRFB(prAdapter, prSwRfb);
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
		} else {
			fgDrop = TRUE;
			DBGLOG(RSN, INFO,
				"Drop plain text during security connection\n");
		}

	}

#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
	/* Drop fragmented broadcast and multicast frame */
	if ((prSwRfb->fgIsBC | prSwRfb->fgIsMC) &&
			(prSwRfb->fgFragFrame == TRUE)) {
		fgDrop = TRUE;
		DBGLOG(RSN, INFO,
			"Drop fragmented broadcast and multicast\n");
	}
#endif /* CFG_SUPPORT_FRAG_ATTACK_DETECTION */

	if (HAL_MAC_CONNAC2X_RX_STATUS_IS_DAF(prRxStatus)) {
		fgDrop = TRUE;
		DBGLOG(RSN, INFO, "de-amsdu fail, Drop:%d\n", fgDrop);
	}

	if (HAL_MAC_CONNAC2X_RX_STATUS_IS_ICV_ERROR(prRxStatus)) {
		fgDrop = TRUE;
		DBGLOG(RSN, INFO, "Drop icv error\n");
	}

	return fgDrop;
}

uint8_t nic_rxd_v2_get_HdrTrans(
	void *prRxStatus)
{
	return HAL_MAC_CONNAC2X_RX_STATUS_IS_HEADER_TRAN(
		(struct HW_MAC_CONNAC2X_RX_DESC *)prRxStatus);
}

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
void nic_rxd_v2_check_wakeup_reason(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_EVENT *prEvent;
	uint8_t *pvHeader = NULL;
	struct HW_MAC_CONNAC2X_RX_DESC *prRxStatus;
	uint16_t u2PktLen = 0;
	uint32_t u4HeaderOffset;
	u_int8_t fgDrop = FALSE;

	prChipInfo = prAdapter->chip_info;

	prRxStatus = (struct HW_MAC_CONNAC2X_RX_DESC *) prSwRfb->prRxStatus;
	if (!prRxStatus)
		return;

	fgDrop = nic_rxd_v2_sanity_check(prAdapter, prSwRfb);
	if (fgDrop) {
		DBGLOG(RX, WARN,
			"%s: sanity check failed. drop!\n", __func__);
		return;
	}

	prSwRfb->ucGroupVLD =
		(uint8_t) HAL_MAC_CONNAC2X_RX_STATUS_GET_GROUP_VLD(prRxStatus);

	switch (prSwRfb->ucPacketType) {
	case RX_PKT_TYPE_RX_DATA:
	{
		uint16_t u2Temp = 0;

/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
		nicUpdateWakeupStatistics(prAdapter, RX_DATA_INT);
#endif /* fos_change end */

		u2PktLen =
			HAL_MAC_CONNAC2X_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus);
		u4HeaderOffset = (uint32_t)
			HAL_MAC_CONNAC2X_RX_STATUS_GET_HEADER_OFFSET(
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
		if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_5))
			u2Temp += sizeof(struct HW_MAC_RX_STS_GROUP_5);
		pvHeader = (uint8_t *)prRxStatus + u2Temp + u4HeaderOffset;
		u2PktLen -= u2Temp + u4HeaderOffset;
		if (!pvHeader) {
			DBGLOG(RX, ERROR,
				"data packet but pvHeader is NULL!\n");
			break;
		}
		if (HAL_MAC_CONNAC2X_RX_STATUS_IS_NDATA(prRxStatus)) {
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
			break;
		case ETH_P_1X:
		case ETH_P_PRE_1X:
#if CFG_SUPPORT_WAPI
		case ETH_WPI_1X:
#endif
		case ETH_P_AARP:
		case ETH_P_IPV6:
		case ETH_P_IPX:
		case 0x8100: /* VLAN */
		case 0x890d: /* TDLS */
			DBGLOG(RX, INFO,
				"Data Packet, EthType 0x%04x wakeup host\n",
				u2Temp);
			break;
		default:
			DBGLOG(RX, WARN,
				"abnormal packet, EthType 0x%04x wakeup host\n",
				u2Temp);
			DBGLOG_MEM8(RX, INFO,
				pvHeader, u2PktLen > 50 ? 50:u2PktLen);
			break;
		}
		break;
	}
	case RX_PKT_TYPE_SW_DEFINED:
		/* HIF_RX_PKT_TYPE_EVENT */
		if ((NIC_RX_GET_U2_SW_PKT_TYPE(prSwRfb->prRxStatus) &
			CONNAC2X_RX_STATUS_PKT_TYPE_SW_BITMAP) ==
			CONNAC2X_RX_STATUS_PKT_TYPE_SW_EVENT) {

			prEvent = (struct WIFI_EVENT *)
				(prSwRfb->pucRecvBuff + prChipInfo->rxd_size);

/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
			nicUpdateWakeupStatistics(prAdapter, RX_EVENT_INT);
			g_wake_event_count[prEvent->ucEID]++;
#endif/* fos_change end */
			DBGLOG(RX, INFO, "Event 0x%02x wakeup host\n",
				prEvent->ucEID);
			break;

		} else if ((NIC_RX_GET_U2_SW_PKT_TYPE(prSwRfb->prRxStatus) &
			CONNAC2X_RX_STATUS_PKT_TYPE_SW_BITMAP) ==
			CONNAC2X_RX_STATUS_PKT_TYPE_SW_FRAME) {
			/* case HIF_RX_PKT_TYPE_MANAGEMENT: */
			uint8_t ucSubtype;
			struct WLAN_MAC_MGMT_HEADER *prWlanMgmtHeader;
			uint16_t u2Temp = prChipInfo->rxd_size;
			u2PktLen =
			HAL_MAC_CONNAC2X_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus);

/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
			nicUpdateWakeupStatistics(prAdapter, RX_MGMT_INT);
#endif /* fos_change end */

			u4HeaderOffset = (uint32_t)
				HAL_MAC_CONNAC2X_RX_STATUS_GET_HEADER_OFFSET(
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
			if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_5))
				u2Temp += sizeof(struct HW_MAC_RX_STS_GROUP_5);
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

#if (CFG_SUPPORT_SNIFFER_RADIOTAP == 1)
uint8_t nic_rxd_v2_fill_radiotap(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct HW_MAC_CONNAC2X_RX_DESC *prRxStatus;
	struct HW_MAC_RX_STS_GROUP_2 *prRxStatusGroup2 = NULL;
	struct HW_MAC_RX_STS_GROUP_3_V2 *prRxStatusGroup3 = NULL;
	struct HW_MAC_RX_STS_GROUP_5 *prRxStatusGroup5 = NULL;
	struct IEEE80211_RADIOTAP_INFO *prRadiotapInfo;
	uint16_t u2RxStatusOffset;
	uint32_t u4HeaderOffset;

	prChipInfo = prAdapter->chip_info;
	prRxStatus = (struct HW_MAC_CONNAC2X_RX_DESC *)prSwRfb->prRxStatus;
	u2RxStatusOffset = prChipInfo->rxd_size;
	prSwRfb->ucGroupVLD =
		(uint8_t) HAL_MAC_CONNAC2X_RX_STATUS_GET_GROUP_VLD(prRxStatus);

	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4))
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_4);

	if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_1))
		u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_1);

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
	    (uint16_t) HAL_MAC_CONNAC2X_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus);

	if (HAL_MAC_CONNAC2X_RX_STATUS_GET_RXV_SEQ_NO(prRxStatus) != 0)
		prGlueInfo->u4AmpduRefNum += 1;

	u4HeaderOffset = (uint32_t) (
		HAL_MAC_CONNAC2X_RX_STATUS_GET_HEADER_OFFSET(prRxStatus));
	u2RxStatusOffset += u4HeaderOffset;

	prRadiotapInfo = prSwRfb->prRadiotapInfo;
	prRadiotapInfo->u2VendorLen = u2RxStatusOffset;
	prRadiotapInfo->ucSubNamespace = 2;
	prRadiotapInfo->u4AmpduRefNum = prGlueInfo->u4AmpduRefNum;
	prRadiotapInfo->u4Timestamp = prRxStatusGroup2->u4Timestamp;
	prRadiotapInfo->ucFcsErr =
		HAL_MAC_CONNAC2X_RX_STATUS_IS_FCS_ERROR(prRxStatus);
	prRadiotapInfo->ucFrag =
		HAL_MAC_CONNAC2X_RX_STATUS_IS_FRAG(prRxStatus);
	prRadiotapInfo->ucChanNum =
		HAL_MAC_CONNAC2X_RX_STATUS_GET_CHNL_NUM(prRxStatus);
	prRadiotapInfo->ucRfBand =
		HAL_MAC_CONNAC2X_RX_STATUS_GET_RF_BAND(prRxStatus);
	prRadiotapInfo->ucTxMode =
		HAL_MAC_CONNAC2X_RX_VT_GET_RX_MODE(prRxStatusGroup5);
	prRadiotapInfo->ucFrMode =
		HAL_MAC_CONNAC2X_RX_VT_GET_FR_MODE(prRxStatusGroup5);
	prRadiotapInfo->ucShortGI =
		HAL_MAC_CONNAC2X_RX_VT_GET_SHORT_GI(prRxStatusGroup5);
	prRadiotapInfo->ucSTBC =
		HAL_MAC_CONNAC2X_RX_VT_GET_STBC(prRxStatusGroup5);
	prRadiotapInfo->ucNess =
		HAL_MAC_CONNAC2X_RX_VT_GET_NESS(prRxStatusGroup5);
	prRadiotapInfo->ucLDPC =
		HAL_MAC_CONNAC2X_RX_VT_GET_LDPC(prRxStatusGroup3);
	prRadiotapInfo->ucMcs =
		HAL_MAC_CONNAC2X_RX_VT_GET_RX_RATE(prRxStatusGroup3);
	prRadiotapInfo->ucRcpi0 =
		HAL_MAC_CONNAC2X_RX_VT_GET_RCPI0(prRxStatusGroup5);
	prRadiotapInfo->ucTxopPsNotAllow =
		HAL_MAC_CONNAC2X_RX_VT_TXOP_PS_NOT_ALLOWED(prRxStatusGroup5);
	prRadiotapInfo->ucLdpcExtraOfdmSym =
		HAL_MAC_CONNAC2X_RX_VT_LDPC_EXTRA_OFDM_SYM(prRxStatusGroup5);
	prRadiotapInfo->ucVhtGroupId =
		HAL_MAC_CONNAC2X_RX_VT_GET_GROUP_ID(prRxStatusGroup5);
	prRadiotapInfo->ucNsts =
		HAL_MAC_CONNAC2X_RX_VT_GET_NSTS(prRxStatusGroup3) + 1;
	prRadiotapInfo->ucBeamFormed =
		HAL_MAC_CONNAC2X_RX_VT_GET_BEAMFORMED(prRxStatusGroup3);

	if (prRadiotapInfo->ucTxMode & TX_RATE_MODE_HE_SU) {
		prRadiotapInfo->ucPeDisamb =
			HAL_MAC_CONNAC2X_RX_VT_GET_PE_DIS_AMB(prRxStatusGroup5);
		prRadiotapInfo->ucNumUser =
			HAL_MAC_CONNAC2X_RX_VT_GET_NUM_USER(prRxStatusGroup5);
		prRadiotapInfo->ucSigBRU0 =
			HAL_MAC_CONNAC2X_RX_VT_GET_SIGB_RU0(prRxStatusGroup5);
		prRadiotapInfo->ucSigBRU1 =
			HAL_MAC_CONNAC2X_RX_VT_GET_SIGB_RU1(prRxStatusGroup5);
		prRadiotapInfo->ucSigBRU2 =
			HAL_MAC_CONNAC2X_RX_VT_GET_SIGB_RU2(prRxStatusGroup5);
		prRadiotapInfo->ucSigBRU3 =
			HAL_MAC_CONNAC2X_RX_VT_GET_SIGB_RU3(prRxStatusGroup5);
		prRadiotapInfo->u2VhtPartialAid =
			HAL_MAC_CONNAC2X_RX_VT_GET_PART_AID(prRxStatusGroup5);
		prRadiotapInfo->u2RuAllocation =
		((HAL_MAC_CONNAC2X_RX_VT_GET_RU_ALLOC1(prRxStatusGroup3) >> 1) |
		(HAL_MAC_CONNAC2X_RX_VT_GET_RU_ALLOC2(prRxStatusGroup3) << 3));
		prRadiotapInfo->u2BssClr =
			HAL_MAC_CONNAC2X_RX_VT_GET_BSS_COLOR(prRxStatusGroup5);
		prRadiotapInfo->u2BeamChange =
		    HAL_MAC_CONNAC2X_RX_VT_GET_BEAM_CHANGE(prRxStatusGroup5);
		prRadiotapInfo->u2UlDl =
			HAL_MAC_CONNAC2X_RX_VT_GET_UL_DL(prRxStatusGroup5);
		prRadiotapInfo->u2DataDcm =
			HAL_MAC_CONNAC2X_RX_VT_GET_DCM(prRxStatusGroup5);
		prRadiotapInfo->u2SpatialReuse1 =
		    HAL_MAC_CONNAC2X_RX_VT_GET_SPATIAL_REUSE1(prRxStatusGroup5);
		prRadiotapInfo->u2SpatialReuse2 =
		    HAL_MAC_CONNAC2X_RX_VT_GET_SPATIAL_REUSE2(prRxStatusGroup5);
		prRadiotapInfo->u2SpatialReuse3 =
		    HAL_MAC_CONNAC2X_RX_VT_GET_SPATIAL_REUSE3(prRxStatusGroup5);
		prRadiotapInfo->u2SpatialReuse4 =
		    HAL_MAC_CONNAC2X_RX_VT_GET_SPATIAL_REUSE4(prRxStatusGroup5);
		prRadiotapInfo->u2Ltf =
			HAL_MAC_CONNAC2X_RX_VT_GET_LTF(prRxStatusGroup5) + 1;
		prRadiotapInfo->u2Doppler =
			HAL_MAC_CONNAC2X_RX_VT_GET_DOPPLER(prRxStatusGroup5);
		prRadiotapInfo->u2Txop =
			HAL_MAC_CONNAC2X_RX_VT_GET_TXOP(prRxStatusGroup5);
	}

	return TRUE;
}
#endif
#endif /* CFG_SUPPORT_CONNAC2X == 1 */
