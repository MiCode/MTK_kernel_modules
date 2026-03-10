/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*
 * Id: @(#) mlr.c@@
 */

/*! \file   mlr.c
 *    \brief  Functions for processing MLR related elements and frames.
 *
 *    Functions for processing MLR related elements and frames.
 */


/*******************************************************************************
 *                        C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                   E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

#if CFG_SUPPORT_MLR

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                           P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static u_int8_t mlrCompose80211Header(
	struct ADAPTER *prAdapter,
	uint8_t	*pucBuffer,
	uint8_t	*pucEthernetFrame,
	struct STA_RECORD *prStaRec,
	u_int8_t fgIsFragment,
	uint8_t ucFragNo,
	uint8_t ucSplitTotal)
{
	const uint8_t aucBridgeTunnelEncap[6] = {
		0xAA, 0xAA, 0x03, 0x00, 0x00, 0xF8
	};
	const uint8_t aucRfc1042Encap[6] = {
		0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00
	};
	struct WLAN_MAC_HEADER_QOS *prWlanHeader = NULL;
	uint16_t u2FrameCtrl = 0, u2EtherType = 0;
	uint8_t	*pucLlc = NULL;
	struct ETH_FRAME_HEADER *prEthernetFrame =
		(struct ETH_FRAME_HEADER *)pucEthernetFrame;
	uint8_t *pucCurrPos = NULL;
	u_int8_t fgHasLlc = FALSE;

	if ((pucBuffer == NULL) || (prStaRec == NULL)
		|| (pucEthernetFrame == NULL)) {
		DBGLOG(INIT, WARN,
			"MLR header - return due to parameter null\n");
		goto err;
	}

	u2FrameCtrl = (prStaRec->fgIsQoS) ?
		MAC_FRAME_QOS_DATA : MAC_FRAME_DATA;
	if (IS_AP_STA(prStaRec)) {
		u2FrameCtrl |= MASK_FC_TO_DS;
		if (qmIsStaInPS(prAdapter, prStaRec))
			u2FrameCtrl |= MASK_FC_PWR_MGT;
		if (fgIsFragment &&	ucFragNo < ucSplitTotal)
			u2FrameCtrl |= MASK_FC_MORE_FRAG;
	} else if (IS_CLIENT_STA(prStaRec)) {
		u2FrameCtrl |= MASK_FC_FROM_DS;
		if (qmIsStaInPS(prAdapter, prStaRec))
			u2FrameCtrl |= MASK_FC_PWR_MGT;
		if (fgIsFragment && ucFragNo < ucSplitTotal)
			u2FrameCtrl |= MASK_FC_MORE_FRAG;
	} else {
		DBGLOG(INIT, WARN, "MLR header - break due to else\n");
		goto err;
	}

	prWlanHeader = (struct WLAN_MAC_HEADER_QOS *) pucBuffer;
	if (prStaRec->fgIsQoS) {
		prWlanHeader->u2QosCtrl = 0;
		prWlanHeader->u2QosCtrl |=
			(ACK_POLICY_NORMAL_ACK_IMPLICIT_BA_REQ <<
			MASK_QC_ACK_POLICY_OFFSET);
		pucCurrPos = (uint8_t *)(pucBuffer +
			WLAN_MAC_HEADER_QOS_LEN);
	} else
		pucCurrPos = (uint8_t *)(pucBuffer +
			WLAN_MAC_HEADER_LEN);

	/* Fill in Frame Control */
	prWlanHeader->u2FrameCtrl = u2FrameCtrl;

	/* Fill in Sequence Control: SeqNo(12bits)
	 * + Fragment number(Lower 4bits)
	 */
	prWlanHeader->u2SeqCtrl = ucFragNo;

	/* Fill Address: BSSID, SA, DA */
	COPY_MAC_ADDR(prWlanHeader->aucAddr1, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prWlanHeader->aucAddr2,
		prEthernetFrame->aucSrcAddr);
	COPY_MAC_ADDR(prWlanHeader->aucAddr3,
		prEthernetFrame->aucDestAddr);

	DBGLOG(INIT, INFO,
		"MLR header - BSSID:" MACSTR
		" SA:" MACSTR " DA:" MACSTR
		" QoS=%d FC=0x%02x SC=0x%02x Etype=%02x(%d)\n",
		MAC2STR(prWlanHeader->aucAddr1),
		MAC2STR(prWlanHeader->aucAddr2),
		MAC2STR(prWlanHeader->aucAddr3),
		prStaRec->fgIsQoS,
		prWlanHeader->u2FrameCtrl,
		prWlanHeader->u2SeqCtrl,
		prEthernetFrame->u2TypeLen,
		NTOHS(prEthernetFrame->u2TypeLen));

	u2EtherType = prEthernetFrame->u2TypeLen;
	if (NTOHS(u2EtherType) > ETH_802_3_MAX_LEN
		&& ucFragNo == 0) {
		/* Only 1st fragment need to add a LLC */
		pucLlc = pucCurrPos;
		/* Move to EtypeType */
		pucCurrPos = (uint8_t *)(pucCurrPos +
			(LLC_LEN - ETHER_TYPE_LEN));
		fgHasLlc = TRUE;
	} else
		pucLlc = NULL;

	/* Insert the LLC/SNAP header */
	if (pucLlc) {
		/* Composing LLC/SNAP header shall be performed
		* after fields of Ethernet header has been
		* copied to the 802.11 header
		*/
		switch (NTOHS(u2EtherType)) {
		case ETH_P_IPX:
		case ETH_P_AARP:
			/* 802.1h format */
			kalMemCopy(pucLlc, aucBridgeTunnelEncap, 6);
			break;
		default:
			/* RFC 1042 format */
			kalMemCopy(pucLlc, aucRfc1042Encap, 6);
			break;
		}
	}

	if ((fgIsFragment && ucFragNo == 0) || !fgIsFragment)
		/* Fill EtherType */
		kalMemCopy(pucCurrPos, (uint8_t *)(pucEthernetFrame +
			(ETHER_HEADER_LEN - ETHER_TYPE_LEN)),
			ETHER_TYPE_LEN);

err:
	return fgHasLlc;
}

static uint16_t mlrCalcTcpUdpChecksum(void *pvData, uint16_t u2Len)
{
	uint32_t u4Accumulation;
	uint16_t u2Src;
	uint8_t *pucCurr;

	if (unlikely(!pvData)) {
		DBGLOG(TX, WARN, "MLR frag - pvData is NULL");
		return FALSE;
	}

	u4Accumulation = 0;
	pucCurr = (uint8_t *)pvData;
	while (u2Len > 1) {
		u2Src = (*pucCurr) << 8;
		pucCurr++;
		u2Src |= (*pucCurr);
		pucCurr++;
		u4Accumulation += u2Src;
		u2Len -= 2;
	}

	if (u2Len > 0) {
		u2Src = (*pucCurr) << 8;
		u4Accumulation += u2Src;
	}

	u4Accumulation = (u4Accumulation >> 16) +
		(u4Accumulation & 0x0000ffffUL);
	if ((u4Accumulation & 0xffff0000UL) != 0)
		u4Accumulation = (u4Accumulation >> 16)
			+ (u4Accumulation & 0x0000ffffUL);

	u2Src = (uint16_t)u4Accumulation;
	return ~u2Src;
}

static u_int8_t mlrFillTcpUdpChecksum(void *pvPacket)
{
	uint8_t *aucLookAheadBuf = NULL;
	uint16_t u2EtherTypeLen = 0;
	uint8_t ucChksumFlag = 0;

	if (unlikely(!pvPacket)) {
		DBGLOG(TX, INFO, "MLR frag - pvPacket is NULL");
		return FALSE;
	}

	kalGetPacketBuf(pvPacket, &aucLookAheadBuf);
	WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ETHER_TYPE_LEN_OFFSET],
				&u2EtherTypeLen);

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	kalQueryTxChksumOffloadParam(pvPacket, &ucChksumFlag);
#endif

	if (ucChksumFlag & TX_CS_TCP_UDP_GEN) {
		uint8_t ucIpProto = 0;
		uint8_t *pucIpHdr = NULL;

		pucIpHdr = &aucLookAheadBuf[ETHER_HEADER_LEN];
		switch (u2EtherTypeLen) {
		case ETH_P_IPV4:
			ucIpProto = pucIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET];
			break;
		case ETH_P_IPV6:
			ucIpProto = pucIpHdr[IPV6_HDR_IP_PROTOCOL_OFFSET];
			break;
		default:
			break;
		}
		if (ucIpProto == IP_PRO_UDP || ucIpProto == IP_PRO_TCP) {
			if (unlikely(kal_skb_checksum_help(pvPacket))) {
				uint8_t ucIpVersion = 0;
				uint16_t u2IpId = 0;
				uint8_t *pucUdpTcpHdr = NULL;
				uint16_t u2Checksum = 0;

				u2Checksum = mlrCalcTcpUdpChecksum(
					&pucIpHdr[0],
					kalQueryPacketLength(pvPacket)
					- ETHER_HEADER_LEN);
				switch (u2EtherTypeLen) {
				case ETH_P_IPV4:
					ucIpVersion = (pucIpHdr[0] &
						IP_VERSION_MASK) >>
						IP_VERSION_OFFSET;
					if (ucIpVersion != IP_VERSION_4) {
						DBGLOG(TX, WARN,
							"MLR frag - Invalid IPv4 packet version: %u\n",
							ucIpVersion);
					}
					pucUdpTcpHdr = &pucIpHdr[IPV4_HDR_LEN];
					u2IpId = (pucIpHdr[
					IPV4_HDR_IP_IDENTIFICATION_OFFSET]
					<< 8) | pucIpHdr[
					IPV4_HDR_IP_IDENTIFICATION_OFFSET
					+ 1];
					break;
				case ETH_P_IPV6:
					ucIpVersion = (pucIpHdr[0] &
						IP_VERSION_MASK)
						>> IP_VERSION_OFFSET;
					if (ucIpVersion != IP_VERSION_6) {
						DBGLOG(TX, WARN,
							"MLR frag - Invalid IPv6 packet version: %u\n",
							ucIpVersion);
					}
					pucUdpTcpHdr = &pucIpHdr[IPV6_HDR_LEN];
					break;
				default:
					break;
				}

				if (ucIpProto == IP_PRO_UDP) {
					DBGLOG(TX, INFO,
						"MLR frag - %s UDP checksum value(ipid:0x%04x): 0x%04x",
						(ucIpVersion == IP_VERSION_4) ?
						"IPv4" : "IPv6",
						(ucIpVersion == IP_VERSION_4) ?
						u2IpId : 0, u2Checksum);
					pucUdpTcpHdr[UDP_HDR_UDP_CSUM_OFFSET] =
						u2Checksum >> 8;
					pucUdpTcpHdr[UDP_HDR_UDP_CSUM_OFFSET
						+ 1] = u2Checksum & 0xff;
				} else if (ucIpProto == IP_PRO_TCP) {
					DBGLOG(TX, INFO,
						"MLR frag - %s TCP checksum value(ipid:0x%04x): 0x%04x",
						(ucIpVersion == IP_VERSION_4) ?
						"IPv4" : "IPv6",
						(ucIpVersion == IP_VERSION_4) ?
						u2IpId : 0,	u2Checksum);
					pucUdpTcpHdr[TCP_HDR_TCP_CSUM_OFFSET] =
						u2Checksum >> 8;
					pucUdpTcpHdr[TCP_HDR_TCP_CSUM_OFFSET
						+ 1] = u2Checksum & 0xff;
				}
			}
		}
	}

	return TRUE;
}

static u_int8_t mlrProccessFragMsduInfo(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		uint8_t ucMacHeaderLength,
		uint8_t ucSplitTotal,
		void *prNativePacket,
		void **pprFragmentSkb,
		struct QUE *prFragmentedQue)
{
	struct MSDU_INFO *prMsduInfoFrag;
	void *prSkbNative, *prSkbSplit;
	uint8_t *aucLookAheadBuf = NULL;
	uint16_t u2EtherTypeLen = 0;
	uint8_t ucSeqNo;
	uint8_t ucIndex = 0;

	if (unlikely(!prAdapter)) {
		DBGLOG(TX, WARN, "MLR frag - prAdapter is NULL");
		return FALSE;
	}

	if (unlikely(!prMsduInfo)) {
		DBGLOG(TX, WARN, "MLR frag - prMsduInfo is NULL");
		return FALSE;
	}
	if (unlikely(!prNativePacket)) {
		DBGLOG(TX, WARN, "MLR frag - prNativePacket is NULL");
		return FALSE;
	}

	prSkbNative = prNativePacket;
	kalGetPacketBuf(prSkbNative, &aucLookAheadBuf);
	WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ETHER_TYPE_LEN_OFFSET],
				&u2EtherTypeLen);

	/* Update First MsduInfo
	 * 802.11 have to provide Mac Header length and Llc Length
	 * in order to set Ether-type offset to TXD
	 */
	prMsduInfo->ucMacHeaderLength = ucMacHeaderLength;
	prMsduInfo->prPacket = prNativePacket;
	/* 802.11header + LLC + EtherType + Payload Len(1st frag) */
	prMsduInfo->u2FrameLength = kalQueryPacketLength(prSkbNative);
	if (u2EtherTypeLen > ETH_802_3_MAX_LEN)
		prMsduInfo->ucLlcLength = LLC_LEN - ETHER_TYPE_LEN;
	else
		prMsduInfo->ucLlcLength = 0;

	/* Consider whether the fragment has txdone */
	if (prMsduInfo->ucPktType == 0
		&& IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgEnTxFragTxDone)) {
		ucSeqNo = nicIncreaseTxSeqNum(prAdapter);
		GLUE_SET_PKT_SEQ_NO(prNativePacket, ucSeqNo);
		prMsduInfo->ucTxSeqNum = GLUE_GET_PKT_SEQ_NO(prSkbNative);
		prMsduInfo->pfTxDoneHandler = wlanPktTxDone;
	}

	/* The format of the fragment must be filled in 802.11 */
	prMsduInfo->fgIs802_11 = TRUE;
	prMsduInfo->eFragPos = MSDU_FRAG_POS_FIRST;
	/* Set lowest rate, using MLR rate is decided
	 * by nicTxSetPktLowestFixedRate
	 */
	prMsduInfo->ucRateMode = MSDU_RATE_MODE_LOWEST_RATE;

	if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
		DBGLOG(TX, INFO, "MLR frag - MSDU_INFO: Frag[0] dump...\n");
		dumpMemory8((uint8_t *)prMsduInfo, sizeof(struct MSDU_INFO));
	}
	/* Enqueue to prFragmentedQue */
	QUEUE_INSERT_TAIL(prFragmentedQue, &prMsduInfo->rQueEntry);

	for (ucIndex = 0; ucIndex < ucSplitTotal; ucIndex++) {
		prMsduInfoFrag = cnmPktAlloc(prAdapter, 0);

		/* for testing error handling case#4 */
		if (prAdapter->rWifiVar.ucErrPos == 4) {
			if (prMsduInfoFrag) {
				cnmPktFree(prAdapter, prMsduInfoFrag);
				prMsduInfoFrag = NULL;
			}
		}

		if (!prMsduInfoFrag) {
			DBGLOG(TX, WARN,
				"MLR frag - prMsduInfoFrag alloc fail\n");
			goto err;
		}

		/* Copy to prMsduInfoFrag from MsduInfo
		 * (with ucMacHeaderLength/ucLlcLength)
		 */
		kalMemCopy(prMsduInfoFrag, prMsduInfo,
			sizeof(struct MSDU_INFO));

		prSkbSplit = (void *)pprFragmentSkb[ucIndex];
		/* Copy the skb control buffer from
		 * native to split
		 */
		kalSkbCopyCbData(prSkbSplit, prSkbNative);

		/* Reset packet length after split,
		 * It should not need to be set.
		 */
		GLUE_SET_PKT_FRAME_LEN(prSkbSplit,
			kalQueryPacketLength(prSkbSplit));

		prMsduInfoFrag->prPacket = (void *)prSkbSplit;
		prMsduInfoFrag->u2FrameLength =
			kalQueryPacketLength(prSkbSplit);

		/* Don't need to set due to Msdu copy
		 * prMsduInfoFrag->ucTxSeqNum =
		 *  GLUE_GET_PKT_SEQ_NO(prSkbNative);
		 * prMsduInfoFrag->pfTxDoneHandler =
		 *  wlanPktTxDone;
		 * prMsduInfoFrag->ucRateMode =
		 *  MSDU_RATE_MODE_LOWEST_RATE;
		 * prMsduInfoFrag->fgIs802_11 = TRUE;
		 */

		/* Set the position of the fragment position */
		if (ucSplitTotal - ucIndex == 1)
			prMsduInfoFrag->eFragPos = MSDU_FRAG_POS_LAST;
		else
			prMsduInfoFrag->eFragPos = MSDU_FRAG_POS_MIDDLE;

		/* Set LLC length, only first fragment needs to have a LLC */
		prMsduInfoFrag->ucLlcLength = 0;

		if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
			DBGLOG(TX, INFO,
				"MLR frag - dump MSDU_INFO: Frag[%d]\n",
				ucIndex + 1);
			dumpMemory8((uint8_t *)prMsduInfoFrag,
				sizeof(struct MSDU_INFO));
		}
		/* Enqueue to prFragmentedQue */
		QUEUE_INSERT_TAIL(prFragmentedQue, &prMsduInfoFrag->rQueEntry);
	}
	DBGLOG(TX, INFO,
		"MLR frag - prFragmentedQue u4NumElem[%d]\n",
		prFragmentedQue->u4NumElem);
	return TRUE;

err:
	DBGLOG(TX, INFO, "MLR frag - mlrProccessFragMsduInfo ERR\n");
	return FALSE;
}

u_int8_t mlrDoFragPacket(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		uint16_t u2SplitSize,
		uint16_t u2SplitThreshold,
		void *prNativePacket,
		struct QUE *prFragmentedQue)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	void *prSkb = prNativePacket;
	void *prSkbSplit = NULL;
	void *prSkbTemp = NULL;
	void *prSkbCpy = NULL;
	/* Fragment number is 0~15 */
	void *prSplittedSkbList[16] = {NULL};
	void *pvAllocPacket = NULL;
	uint8_t *pucRecvBuff;
	uint16_t u2AllocSize = 0;
	uint8_t ucMacHeaderLength = 0;
	uint8_t aucWlanHeaderLlcEthType[WLAN_MAC_HEADER_QOS_LEN
		+ LLC_LEN] = {0};
	uint16_t u2SplitExpandOffset = 0;
	u_int8_t fgHasLlc = FALSE;
	uint8_t *pucBuff = NULL;
	uint8_t *pucOutputBuf = NULL;
	uint8_t ucFillBuffLength = 0;
	uint8_t ucSplitTotal = 0;
	uint16_t u2FinalSplitSize;
	uint8_t ucIndex = 0;
	uint16_t u2SkbLength = 0;
	uint16_t u2SkbDupLen = 0;
	uint8_t *pucTempData = NULL;
	uint8_t *pucTempData2 = NULL;
	uint16_t u2EtherTypeLen = 0;
	/* For tolerance & recovery */
	struct MSDU_INFO rMsduInfoDup;
	struct MSDU_INFO *prCurrMsduInfo;
	struct MSDU_INFO *prNextMsduInfo;
	void *prSkbDup = NULL;
	struct QUE rNeedToFreeQue;
	struct QUE *prNeedToFreeQue = &rNeedToFreeQue;
	uint8_t ucErrCode = 0;

	/* KAL_TIME_INTERVAL_DECLARATION(); */

	if (unlikely(!prAdapter)) {
		DBGLOG(TX, WARN, "MLR frag - prAdapter is NULL");
		return FALSE;
	}

	if (unlikely(!prMsduInfo)) {
		DBGLOG(TX, WARN, "MLR frag - prMsduInfo is NULL");
		return FALSE;
	}
	if (unlikely(!prNativePacket)) {
		DBGLOG(TX, WARN, "MLR frag - prNativePacket is NULL");
		return FALSE;
	}

	if (kalQueryPacketLength(prSkb) <= u2SplitThreshold)
		return FALSE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (unlikely(!prBssInfo)) {
		DBGLOG(TX, WARN, "MLR frag - prBssInfo is NULL");
		return FALSE;
	}
	prStaRec = prBssInfo->prStaRecOfAP;
	if (unlikely(!prStaRec)) {
		DBGLOG(TX, WARN, "MLR frag - prStaRec is NULL");
		return FALSE;
	}
	if (unlikely(!prStaRec->fgIsInUse)) {
		DBGLOG(TX, INFO, "MLR check - prStaRec->fgIsInUse is FALSE");
		return FALSE;
	}

	/* Get EtherType */
	u2EtherTypeLen = kalQueryPacketEtherType(prSkb);

	/* For tolerance & recovery */
	kalMemCopy(&rMsduInfoDup, prMsduInfo, sizeof(struct MSDU_INFO));
	prSkbDup = kal_skb_copy(prSkb);
	if (unlikely(!prSkbDup)) {
		DBGLOG(TX, WARN, "MLR frag - prSkbDup is NULL");
		return FALSE;
	}

	DBGLOG(TX, INFO, "MLR frag - Start\n");
	/* KAL_REC_TIME_START(); */

	/* Step 1: Calculate and deal with split size and split number */
	/* The fragment size must meet 802.11 spec requirement */
	u2SkbDupLen = kalQueryPacketLength(prSkbDup);
	if (u2SplitSize == 0) {
		u2FinalSplitSize = (u2SkbDupLen % 2 == 0) ?
			u2SkbDupLen / 2 : u2SkbDupLen / 2 + 1;
	} else
		u2FinalSplitSize = u2SplitSize;

	u2FinalSplitSize = (u2FinalSplitSize % 2 == 0) ?
		u2FinalSplitSize : u2FinalSplitSize + 1;

	/* Check whether the final split size is reasonable */
	if (u2FinalSplitSize <= ETHER_HEADER_LEN) {
		DBGLOG(TX, WARN,
			"MLR frag - Split size[%u,%u] is less than or equal to ETHER_HEADER_LEN(14)",
			u2SplitSize, u2FinalSplitSize);
		return FALSE;
	}

	if (u2SkbDupLen % u2FinalSplitSize == 0)
		ucSplitTotal = u2SkbDupLen / u2FinalSplitSize - 1;
	else
		ucSplitTotal = u2SkbDupLen / u2FinalSplitSize;

	/* Check whether there are enough MsduInfos */
	if (prAdapter->rTxCtrl.rFreeMsduInfoList.u4NumElem < ucSplitTotal) {
		DBGLOG(TX, WARN,
			"MLR frag - FreeMsduInfoList.u4NumElem[%d] is less than ucSplitTotal[%d]\n",
			prAdapter->rTxCtrl.rFreeMsduInfoList.u4NumElem,
			ucSplitTotal);
		return FALSE;
	}

	/* Step 2: SW needs to calculate TCP/UDP checksum due to MAC behavior */
	mlrFillTcpUdpChecksum(prSkbDup);

	/* Step 3: Allocate and split skb buffers
	 * In order to fix the 1st to N-1 fragment
	 * are all split into the same size
	 */
	if (u2EtherTypeLen > ETH_802_3_MAX_LEN)
		/* It should be 6 */
		u2SplitExpandOffset = ETHER_HEADER_LEN - LLC_LEN;
	else
		/* It should be 12 */
		u2SplitExpandOffset = ETHER_HEADER_LEN - ETHER_TYPE_LEN;

	ucMacHeaderLength = (prStaRec->fgIsQoS ?
		WLAN_MAC_HEADER_QOS_LEN : WLAN_MAC_HEADER_LEN);

	/* For tolerance & recovery */
	prSkbTemp = prSkbDup;
	/* prSkbTemp = prSkb; */
	u2SkbLength = kalQueryPacketLength(prSkbTemp);
	DBGLOG(TX, INFO,
		"MLR frag - prSkb->len=%u u2EtherTypeLen=%u ucSplitTotal=%u[%u/%u]\n",
		u2SkbLength, u2EtherTypeLen,
		ucSplitTotal, u2SkbLength,
		u2FinalSplitSize);
	if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
		kalGetPacketBuf(prSkbTemp, &pucTempData);
		DBGLOG(TX, INFO,
			"MLR frag - Original prSkb->len=%u dump...\n",
			u2SkbLength);
		dumpMemory8(pucTempData, u2SkbLength);
	}

	for (ucIndex = 0; ucIndex < ucSplitTotal; ucIndex++) {
		u2AllocSize = u2SkbLength - (u2FinalSplitSize +
			u2SplitExpandOffset) + MAC_TX_RESERVED_FIELD +
			sizeof(struct WLAN_MAC_HEADER_QOS) + LLC_LEN;
#if 0
		if (ucSplitTotal - ucIndex == 1)
			/* last position sbk buffer */
			u2AllocSize = u2SkbLength - (u2FinalSplitSize +
			u2SplitExpandOffset) + MAC_TX_RESERVED_FIELD +
			sizeof(struct WLAN_MAC_HEADER_QOS) + LLC_LEN;
		else
			/* middle position sbk buffer */
			u2AllocSize = u2SplitSize + MAC_TX_RESERVED_FIELD +
			sizeof(struct WLAN_MAC_HEADER_QOS) + LLC_LEN;
#endif
		pvAllocPacket = kalPacketAlloc(prAdapter->prGlueInfo,
		  u2AllocSize, TRUE, &pucRecvBuff);
		/* for testing error handling case#1 */
		if (prAdapter->rWifiVar.ucErrPos == 1) {
			if (pvAllocPacket)
				kalPacketFree(prAdapter->prGlueInfo,
				pvAllocPacket);
			ucErrCode = 1;
			goto err_handler_1;
		}

		if (!pvAllocPacket) {
			DBGLOG(TX, ERROR,
				"MLR frag - alloc skb buffer failed, Don't do fragment");
			ucErrCode = 1;
			goto err_handler_1;
		}

		prSkbSplit = pvAllocPacket;
		prSplittedSkbList[ucIndex] = pvAllocPacket;
		kal_skb_reserve(prSkbSplit, ucMacHeaderLength);

		MLR_DBGLOG(prAdapter, TX, INFO,
			"MLR frag - ucIndex[%d] u2AllocSize=%d u2SplitSize=%d u2FinalSplitSize=%d u2SplitExpandOffset=%d",
			ucIndex, u2AllocSize, u2SplitSize,
			u2FinalSplitSize, u2SplitExpandOffset);

		/* Split sk_buff into 2 sk_buffs */
		kal_skb_split(prSkbTemp, prSkbSplit,
			(u2FinalSplitSize + u2SplitExpandOffset));

		/* for testing error handling case#2 */
		if (prAdapter->rWifiVar.ucErrPos == 2) {
			ucErrCode = 2;
			goto err_handler_1;
		}

		if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
			kalGetPacketBuf(prSkbSplit, &pucTempData);
			kalGetPacketBuf(prSkbTemp, &pucTempData2);
			DBGLOG(TX, INFO,
				"MLR frag - after-split ucIndex[%d] prSkbTemp[%p,%d] prSkbSplit[%p,%d]\n",
				 ucIndex, pucTempData2,
				 kalQueryPacketLength(prSkbTemp),
				 pucTempData, kalQueryPacketLength(prSkbSplit));
			dumpMemory8(pucTempData,
				kalQueryPacketLength(prSkbSplit));
		}

		if (ucSplitTotal - ucIndex > 1)
			/* Continue to split */
			prSkbTemp = prSkbSplit;
		u2SkbLength -= (u2FinalSplitSize + u2SplitExpandOffset);

		/* In order to fix the 1st to N-1 fragment are all split
		 * into the same size, and only 1st frag needs to expand
		 */
		if (ucIndex == 0)
			u2SplitExpandOffset = 0;
	}

    /* Step 4: Compose 802.11 header buffer and put to split skb buffers */
	kalGetPacketBuf(prSkbDup, &pucTempData);
	for (ucIndex = 0; ucIndex < ucSplitTotal; ucIndex++) {
		/* pucBuff for composing 802.11 header buffer */
		pucBuff = &aucWlanHeaderLlcEthType[0];

		/* Compose 802.11 header(802.11 header + LLC + EtherType) */
		mlrCompose80211Header(prAdapter, pucBuff,
					pucTempData, prStaRec,
					TRUE, ucIndex + 1, ucSplitTotal);
		/* Because Frag number is larger than 0,
		 * only need the 802.11 header
		 */
		ucFillBuffLength = ucMacHeaderLength;

		if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
			dumpMemory8(pucBuff, ucFillBuffLength);
			DBGLOG(TX, INFO,
				"MLR frag - pucBuff-after filled 802.11 header + LLC + EthType(FillLen=%d) dump...\n",
				ucFillBuffLength);
		}

		/* Add the buffer of 802.11 header to skb split */
		prSkbSplit = prSplittedSkbList[ucIndex];
		pucOutputBuf = kal_skb_push(prSkbSplit,
					ucFillBuffLength);
		/* Fill in skb split with the composed buffer */
		kalMemCopy(pucOutputBuf, pucBuff, ucFillBuffLength);

		if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
			kalGetPacketBuf(prSkbSplit, &pucTempData);
			DBGLOG(TX, INFO,
				"MLR frag - prSkbSplit-put 802.11 header + LLC + EthType into prSkbSplit dump...\n");
			dumpMemory8(pucTempData, ucFillBuffLength);
		}
	}

	/* Step 5: Deal with original skb */
	/* Compose 802.11 header buffer */
	kalGetPacketBuf(prSkbDup, &pucTempData);
	fgHasLlc = mlrCompose80211Header(prAdapter, pucBuff,
				pucTempData, prStaRec,
				TRUE, 0, ucSplitTotal);

	if (fgHasLlc) {
		ucFillBuffLength = ucMacHeaderLength + LLC_LEN;
		/* 802.11 have to provide 802.11 header length and LLC Length
		 * in order to set ether-type offset to TXD
		 */
		prMsduInfo->ucLlcLength = LLC_LEN - ETHER_TYPE_LEN;
	} else {
		ucFillBuffLength = ucMacHeaderLength + ETHER_TYPE_LEN;
		prMsduInfo->ucLlcLength = 0;
	}

	kalGetPacketBuf(prSkbSplit, &pucTempData2);
	if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
		DBGLOG(TX, INFO, "MLR frag - prSkb-after split dump...\n");
		dumpMemory8(pucTempData, kalQueryPacketLength(prSkbDup));
	}

	DBGLOG(TX, INFO,
		"MLR frag - SPLIT XAFTER prSkbDup[%p, %p, %d] prSkbSplit[%p, %p, %d]\n",
		prSkbDup, pucTempData, kalQueryPacketLength(prSkbDup),
		prSkbSplit, pucTempData2, kalQueryPacketLength(prSkbSplit));

	prSkbCpy = kal_skb_copy(prSkbDup);

	/* for testing error handling case#3 */
	if (prAdapter->rWifiVar.ucErrPos == 3) {
		if (prSkbCpy)
			kalPacketFree(prAdapter->prGlueInfo, prSkbCpy);
		ucErrCode = 3;
		goto err_handler_1;
	}

	if (unlikely(!prSkbCpy)) {
		DBGLOG(TX, WARN, "MLR frag - prSkbCpy is NULL");
		goto err_handler_1;
	}
	/* Remove all data from the skb duplicated buffer */
	kal_skb_pull(prSkbDup, kalQueryPacketLength(prSkbDup));
	kalGetPacketBuf(prSkbDup, &pucTempData);
	MLR_DBGLOG(prAdapter, TX, INFO,
		"MLR frag - Remove all data from prSkbDup: data=%p len=%u prSkbCpy: len=%u\n",
		pucTempData, kalQueryPacketLength(prSkbDup),
		kalQueryPacketLength(prSkbCpy));
	/* Extend headroom for wlan header in skb duplicated buffer */
	kal_skb_reserve(prSkbDup, ucFillBuffLength - ETHER_HEADER_LEN);
	kalGetPacketBuf(prSkbDup, &pucTempData);
	MLR_DBGLOG(prAdapter, TX, INFO,
		"MLR frag - Extend headroom(%d) in prSkbDup: data=%p len=%u\n",
		ucFillBuffLength - ETHER_HEADER_LEN,
		pucTempData, kalQueryPacketLength(prSkbDup));
	/* Remove the ether header from skb copy buffer */
	kal_skb_pull(prSkbCpy, ETHER_HEADER_LEN);
	MLR_DBGLOG(prAdapter, TX, INFO,
		"MLR frag - Remove the ether header from prSkbCpy: len=%d\n",
		kalQueryPacketLength(prSkbCpy));

	/* Add the data to skb duplicated buffer */
	pucOutputBuf = kal_skb_push(prSkbDup,
				kalQueryPacketLength(prSkbCpy));

	/* Fill in skb duplicate with skb copy buffer */
	kalGetPacketBuf(prSkbCpy, &pucTempData);
	kalMemCopy(pucOutputBuf, pucTempData, kalQueryPacketLength(prSkbCpy));
	if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
		kalGetPacketBuf(prSkbDup, &pucTempData);
		DBGLOG(TX, INFO,
			"MLR frag - Add the data from prSkbCpy to prSkbDup: prSkb->len=%d prSkbCpy->len=%d dump...\n",
			kalQueryPacketLength(prSkbDup),
			kalQueryPacketLength(prSkbCpy));
		dumpMemory8(pucTempData, kalQueryPacketLength(prSkbDup));
	}

	/* Add Wlan header + LLC + EtheType(ucFillBuffLength)
	 * to skb duplicated buffer.
	 * It will result exceeding headroom while
	 * skb_push(TXD+TXDAPPEND), So prSkbDup should add more
	 * reserve "ucFillBuffLength-Ether Header"
	 */
	pucOutputBuf = kal_skb_push(prSkbDup,
				ucFillBuffLength);
	/* Fill in skb duplicate with the composed buffer */
	kalMemCopy(pucOutputBuf, pucBuff, ucFillBuffLength);
	if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) {
		kalGetPacketBuf(prSkbDup, &pucTempData);
		DBGLOG(TX, INFO,
			"MLR frag - Add Wlan header + LLC + EtheType to prSkbDup: len=%d dump...\n",
			kalQueryPacketLength(prSkbDup));
		dumpMemory8(pucTempData, kalQueryPacketLength(prSkbDup));
	}
	/* Free the skb copy buffer */
	if (prSkbCpy)
		kalPacketFree(prAdapter->prGlueInfo, prSkbCpy);

	/* Step 6: Deal with MsduInfo */
	if (!mlrProccessFragMsduInfo(prAdapter,
		prMsduInfo, ucMacHeaderLength,
		ucSplitTotal, (void *)prSkbDup,
		&prSplittedSkbList[0], prFragmentedQue))
		goto err_handler_2;

	/* Step 7: All operations of skb buffer work fine,
	 * free the original skb buffer
	 */
	kalPacketFree(prAdapter->prGlueInfo, prSkb);

	/* KAL_REC_TIME_END();
	 * DBGLOG(TX, INFO, "MLR frag - End[%lu us]\n",
	 * KAL_GET_TIME_INTERVAL());
	 */
	DBGLOG(TX, INFO, "MLR frag - End\n");

	return TRUE;

err_handler_1: /* handling that processes skb buffers happen an exception */
	DBGLOG(TX, ERROR, "MLR frag - err1 handler:%d\n",
		prAdapter->rWifiVar.ucErrPos);
	/* Free the duplicated skb buffer */
	if (prSkbDup)
		kalPacketFree(prAdapter->prGlueInfo, prSkbDup);
	/* Free all split skb buffers */
	for (ucIndex = 0; ucIndex < ucSplitTotal; ucIndex++) {
		if (prSplittedSkbList[ucIndex] != NULL)
			kalPacketFree(prAdapter->prGlueInfo,
				prSplittedSkbList[ucIndex]);
	}
	return FALSE;
err_handler_2: /* handling that processing MsduInfo happens an exception */
	DBGLOG(TX, ERROR, "MLR frag - err2 handler:%d\n",
		prAdapter->rWifiVar.ucErrPos);
	/* Free the duplicated skb buffer */
	if (prSkbDup)
		kalPacketFree(prAdapter->prGlueInfo, prSkbDup);
	/* Free all split skb buffers */
	for (ucIndex = 0; ucIndex < ucSplitTotal; ucIndex++) {
		if (prSplittedSkbList[ucIndex] != NULL)
			kalPacketFree(prAdapter->prGlueInfo,
				prSplittedSkbList[ucIndex]);
	}

	if (prFragmentedQue->u4NumElem == 0) {
		DBGLOG(TX, ERROR, "MLR frag - handle err2-1\n");
		/* Rollback MsduInfo */
		kalMemCopy(prMsduInfo, &rMsduInfoDup, sizeof(struct MSDU_INFO));
		/* Insert original MsduInfo to return queue */
		QUEUE_INSERT_TAIL(prFragmentedQue, &prMsduInfo->rQueEntry);
	} else if (prFragmentedQue->u4NumElem == 1) {
		DBGLOG(TX, ERROR, "MLR frag - handle err2-2\n");
		/* Rollback MsduInfo */
		kalMemCopy(prMsduInfo, &rMsduInfoDup, sizeof(struct MSDU_INFO));
	} else if (prFragmentedQue->u4NumElem > 1) {
		DBGLOG(TX, ERROR, "MLR frag - handle err2-3\n");
		QUEUE_MOVE_ALL(prNeedToFreeQue, prFragmentedQue);
		prCurrMsduInfo = QUEUE_GET_HEAD(prNeedToFreeQue);
		while (prCurrMsduInfo) {
			prNextMsduInfo = QUEUE_GET_NEXT_ENTRY(
					&prCurrMsduInfo->rQueEntry);
			if (prCurrMsduInfo == prMsduInfo) {
				DBGLOG(TX, ERROR,
					"MLR frag - Rollback MsduInfo %p\n",
					prCurrMsduInfo);
				/* Rollback MsduInfo */
				kalMemCopy(prCurrMsduInfo, &rMsduInfoDup,
					sizeof(struct MSDU_INFO));
			} else {
				DBGLOG(TX, ERROR,
					"MLR frag - Free fragmented MsduInfo %p\n",
					prCurrMsduInfo);
				cnmPktFree(prAdapter, prCurrMsduInfo);
			}
			prCurrMsduInfo = prNextMsduInfo;
		}
	}

	return FALSE;
}

static u_int8_t mlrMlrCapVerCheck(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct STA_RECORD *prStaRec)
{
	u_int8_t fgMlrCapVerCheck = FALSE;

	if (unlikely(!prAdapter)) {
		DBGLOG(TX, INFO, "MLR check - prAdapter is NULL");
		return FALSE;
	}
	if (unlikely(!prBssInfo)) {
		DBGLOG(TX, INFO, "MLR check - prBssInfo is NULL");
		return FALSE;
	}
	if (unlikely(!prStaRec)) {
		DBGLOG(TX, INFO, "MLR check - prStaRec is NULL");
		return FALSE;
	}

	/* check band */
	fgMlrCapVerCheck = MLR_CHECK_IF_BAND_IS_SUPPORT(prBssInfo->eBand);

	/* check capability */
	fgMlrCapVerCheck &= MLR_IS_SUPPORT(prAdapter) ? TRUE : FALSE;
	fgMlrCapVerCheck &= MLR_IS_PEER_SUPPORT(prStaRec) ? TRUE : FALSE;
	if (!fgMlrCapVerCheck) {
		MLR_DBGLOG(prAdapter, TX, INFO,
			"MLR check - MLR cap doesn't support(band=%d)",
			prBssInfo->eBand);
		return fgMlrCapVerCheck;
	}

	switch (prAdapter->ucMlrVersion) {
	case 1: /* MLR version 1 is only limited to when Peer is legacy AP */
		fgMlrCapVerCheck &= (prStaRec->eStaType ==
			STA_TYPE_LEGACY_AP) ? TRUE : FALSE;
		fgMlrCapVerCheck &= (IS_BSS_AIS(prBssInfo)) ? TRUE : FALSE;
		fgMlrCapVerCheck &= (prBssInfo->eCurrentOPMode ==
			OP_MODE_INFRASTRUCTURE) ? TRUE : FALSE;
		fgMlrCapVerCheck &= (prBssInfo->prStaRecOfAP != NULL)
			? TRUE : FALSE;
		fgMlrCapVerCheck &= MLR_BIT_V1_SUPPORT(
			prStaRec->ucMlrSupportBitmap);
		fgMlrCapVerCheck &= MLR_STATE_IN_START(prStaRec->ucMlrState);
		break;
	case 2:
		/* TODO(@phase2): do while MLR version is MLRv2 */
		fgMlrCapVerCheck = (prStaRec->eStaType == STA_TYPE_LEGACY_AP
			|| prStaRec->eStaType ==
			STA_TYPE_LEGACY_CLIENT) ? TRUE : FALSE;
		fgMlrCapVerCheck &= MLR_BIT_V1_V2_SUPPORT(
			prStaRec->ucMlrSupportBitmap);
		break;
	default:
		fgMlrCapVerCheck = FALSE;
		DBGLOG(TX, INFO, "MLR check - MLR ver %d, Ver doesn't support");
		break;
	}

	return fgMlrCapVerCheck;
}

u_int8_t mlrCheckIfDoFrag(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		void *prNativePacket)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	u_int8_t fgMlrCapVerCheck = FALSE;
	u_int8_t fgForceTxFrag = FALSE;

	if (unlikely(!prAdapter)) {
		DBGLOG(TX, INFO, "MLR check - prAdapter is NULL");
		return FALSE;
	}
	if (unlikely(!prMsduInfo)) {
		DBGLOG(TX, INFO, "MLR check - prMsduInfo is NULL");
		return FALSE;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	if (unlikely(!prBssInfo)) {
		DBGLOG(TX, INFO, "MLR check - prBssInfo is NULL");
		return FALSE;
	}

	if (!IS_BSS_AIS(prBssInfo)
		|| !(kalGetMediaStateIndicated(
		prAdapter->prGlueInfo,
		prBssInfo->ucBssIndex) ==
		MEDIA_STATE_CONNECTED)) {
		return FALSE;
	}

	prStaRec = prBssInfo->prStaRecOfAP;
	if (unlikely(!prStaRec)) {
		DBGLOG(TX, INFO, "MLR check - prStaRec is NULL");
		return FALSE;
	}
	if (unlikely(!prStaRec->fgIsInUse)) {
		DBGLOG(TX, INFO, "MLR check - prStaRec->fgIsInUse is FALSE");
		return FALSE;
	}

	if (unlikely(!prNativePacket)) {
		DBGLOG(TX, INFO, "MLR check - prNativePacket is NULL");
		return FALSE;
	}

	fgForceTxFrag = prAdapter->rWifiVar.fgEnForceTxFrag;
	if (!MLR_CHECK_IF_ENABLE_TX_FRAG(prStaRec)
		&& !fgForceTxFrag)
		return FALSE;

	fgMlrCapVerCheck = mlrMlrCapVerCheck(prAdapter, prBssInfo, prStaRec);
	fgMlrCapVerCheck |= fgForceTxFrag;
	if (fgForceTxFrag)
		DBGLOG(TX, INFO, "MLR check - Force Tx Frag\n");

	if (fgMlrCapVerCheck
		/* check whether the condition of TX fragment is meet */
		&& !IS_BMCAST_MAC_ADDR(prMsduInfo->aucEthDestAddr)
		&& MLR_CHECK_IF_PKT_LEN_DO_FRAG(prAdapter, prNativePacket))
		return TRUE;
	else
		return FALSE;
}

u_int8_t mlrDecideIfUseMlrRate(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct STA_RECORD *prStaRec,
		struct MSDU_INFO *prMsduInfo,
		uint16_t *pu2RateCode)
{
	u_int8_t fgIsUseMlrRate = FALSE;

	if (unlikely(!prAdapter || !prBssInfo || !prStaRec || !prMsduInfo))
		return fgIsUseMlrRate;

	MLR_DBGLOG(prAdapter, NIC, INFO,
		   "MLR rate - prStaRec->u2HwDefaultFixedRateCode=0x%x, u2OperationalRateSet=0x%x\n",
		   prStaRec->u2HwDefaultFixedRateCode,
		   prStaRec->u2OperationalRateSet);

	/* STA MLR capability && Peer-AP MLR capability */
	if (MLR_IS_BOTH_SUPPORT(prAdapter, prStaRec)
		/* MLR_V1-Unbalance */
		&& prAdapter->ucMlrVersion == 1) {
		if (MLR_CHECK_IF_BAND_IS_SUPPORT(prBssInfo->eBand)
			&& MLR_BIT_V1_SUPPORT(
			MLR_BIT_INTERSECTION(
			prAdapter->u4MlrSupportBitmap,
			prStaRec->ucMlrSupportBitmap))
			/* MLRv1 */
			&& prStaRec->eStaType ==
			STA_TYPE_LEGACY_AP) {

			/* In case of MGMT frame: Auth and (re)Assoc */
			/* && RSSI < -93(RCPI:34)~-90(RCPI:40) */
			if (prMsduInfo->eSrc == TX_PACKET_MGMT
				&& prMsduInfo->prPacket) {
				uint16_t u2FrameCtrl =
					((struct WLAN_MAC_HEADER *)prMsduInfo
					->prPacket)->u2FrameCtrl;

				u2FrameCtrl &= MASK_FRAME_TYPE;
				if (MLR_CHECK_IF_MGMT_USE_MLR_RATE(
					u2FrameCtrl)) {
					if (MLR_CHECK_IF_RCPI_IS_LOW(prAdapter,
						prStaRec->ucRCPI)) {
						*pu2RateCode = RATE_MLR_1_5M;
						fgIsUseMlrRate = TRUE;
						DBGLOG(NIC, INFO,
							"MLR rate - TX MGMT frame(FC=0x%02x) to Peer(eStaType=0x%02x) with MLR RateCode=0x%x\n",
							u2FrameCtrl,
							prStaRec->eStaType,
							*pu2RateCode);
					} else {
						DBGLOG(NIC, INFO,
							"MLR rate - RCPI condition doesn't meet(Peer RCPI:%d > Threshold:%d)\n",
							prStaRec->ucRCPI,
							prAdapter->rWifiVar
							.ucTxMlrRateRcpiThr);
					}
				}
			/* In case of Data frame and it is fragmented */
			} else if (MLR_CHECK_IF_ENABLE_TX_FRAG(prStaRec)
				&& prMsduInfo->eSrc == TX_PACKET_OS
				&& (MLR_CHECK_IF_MSDU_IS_FRAG(prMsduInfo)
				/* Consider fixed rate (ARP, DHCP, EAPOL) */
				|| prMsduInfo->ucRateMode ==
				MSDU_RATE_MODE_MANUAL_DESC)) {
				*pu2RateCode = RATE_MLR_1_5M;
				fgIsUseMlrRate = TRUE;
				DBGLOG(NIC, INFO,
					"MLR rate - TX Data frame to Peer(eStaType=0x%02x) with MLR RateCode=0x%x\n",
					prStaRec->eStaType, *pu2RateCode);
			}
		}
	}

	MLR_DBGLOG(prAdapter, NIC, INFO,
		   "MLR rate - u2RateCode=0x%x fgIsUseMlrRate=%d MsduInfo[eSrc=%d ucTxSeqNum=%d eFragPos=%d] Intersection[0x%x] rBssInfo->eBand=%d prStaRec->eStaType=0x%02x prStaRec->ucRCPI=%d(RSSI=%d) prAdapter->u4MlrSupportBitmap=%d(0x%04x) prStaRec->ucMlrIsSupport=%d(0x%x) BSSIDX(%d,%d)\n",
		   *pu2RateCode,
		   fgIsUseMlrRate,
		   prMsduInfo->eSrc,
		   prMsduInfo->ucTxSeqNum,
		   prMsduInfo->eFragPos,
		   MLR_BIT_INTERSECTION(prAdapter->u4MlrSupportBitmap,
				prStaRec->ucMlrSupportBitmap),
		   prBssInfo->eBand,
		   prStaRec->eStaType,
		   prStaRec->ucRCPI,
		   RCPI_TO_dBm(prStaRec->ucRCPI),
		   prAdapter->ucMlrIsSupport, prAdapter->u4MlrSupportBitmap,
		   prStaRec->fgIsMlrSupported, prStaRec->ucMlrSupportBitmap,
		   prStaRec->ucBssIndex, prMsduInfo->ucBssIndex);

	return fgIsUseMlrRate;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is used to generate MTK Vendor Specific OUI for MLR
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mlrGenerateMTKOuiIEforMlr(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucBuffer;
	uint8_t *pucVsDataBuffer;
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;

	if (unlikely(!prAdapter)) {
		DBGLOG(TX, WARN, "MLR assoc - prAdapter is NULL");
		return;
	}

	if (unlikely(!prMsduInfo)) {
		DBGLOG(TX, WARN, "MLR assoc - prMsduInfo is NULL");
		return;
	}

	if (prAdapter->rWifiVar.ucMtkOui == FEATURE_DISABLED)
		return;

	prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];
	if (!prBssInfo)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	if (!prStaRec)
		return;

	if (!MLR_IS_SUPPORT(prAdapter)) {
		MLR_DBGLOG(prAdapter, TX, INFO,
			"MLR assoc - DUT doesn't support MLR\n");
		return;
	}

	if (!MLR_IS_PEER_SUPPORT(prStaRec)) {
		MLR_DBGLOG(prAdapter, TX, INFO,
			"MLR assoc - Peer doesn't support MLR\n");
		return;
	}

	pucBuffer = (uint8_t *)((unsigned long)prMsduInfo->prPacket +
				(unsigned long)prMsduInfo->u2FrameLength);

	MTK_OUI_IE(pucBuffer)->ucId = ELEM_ID_VENDOR;
	MTK_OUI_IE(pucBuffer)->ucLength = ELEM_MLR_MTK_OUI_LEN;
	MTK_OUI_IE(pucBuffer)->aucOui[0] = aucMtkOui[0];
	MTK_OUI_IE(pucBuffer)->aucOui[1] = aucMtkOui[1];
	MTK_OUI_IE(pucBuffer)->aucOui[2] = aucMtkOui[2];

	MTK_OUI_IE(pucBuffer)->aucCapability[0] = 0x01; /* indicates MTK TLV */
	MTK_OUI_IE(pucBuffer)->aucCapability[1] = 0x00;
	MTK_OUI_IE(pucBuffer)->aucCapability[2] = 0x00;
	MTK_OUI_IE(pucBuffer)->aucCapability[3] = 0x00;

	pucVsDataBuffer = (uint8_t *) MTK_OUI_IE(pucBuffer)->aucInfoElem;
	pucVsDataBuffer[0] = 0x01; /* MLR type */
	pucVsDataBuffer[1] = 0x01; /* MLR length */
	/* Pay attention to trimming bitmap data if u4MlrSupportBitmap > 255 */
	pucVsDataBuffer[2] = (uint8_t) (prAdapter->u4MlrSupportBitmap &
		prStaRec->ucMlrSupportBitmap);

	DBGLOG(TX, INFO,
		"MLR assoc - MlrB:0x%02x (DUT:0x%04x bitwise-and Peer:0x%02x)\n",
		pucVsDataBuffer[2], prAdapter->u4MlrSupportBitmap,
		prStaRec->ucMlrSupportBitmap);

	prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
	pucBuffer += IE_SIZE(pucBuffer);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is in response of EVENT_ID_MLR_FSM_UPDATE form FW
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param prEvent	Pointer to the event structure.
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void mlrEventMlrFsmUpdateHandler(struct ADAPTER *prAdapter,
		  struct WIFI_EVENT *prEvent)
{
	struct EVENT_MLR_FSM_UPDATE *prEvtMlrFsmUpdate;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint8_t ucStaIdx;

	if (unlikely(!prAdapter)) {
		DBGLOG(TX, WARN, "MLR event - prAdapter is NULL");
		return;
	}

	prEvtMlrFsmUpdate =
		(struct EVENT_MLR_FSM_UPDATE *)(prEvent->aucBuffer);

	if (wlanGetStaIdxByWlanIdx(prAdapter,
		prEvtMlrFsmUpdate->u2WlanIdx, &ucStaIdx) !=
		WLAN_STATUS_SUCCESS) {
		DBGLOG(NIC, WARN,
			"MLR event - wlanGetStaIdxByWlanIdx failed.\n");
		return;
	}

	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaIdx);
	if (prStaRec == NULL) {
		DBGLOG(NIC, WARN, "MLR event - prStaRec is NULL.\n");
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  prStaRec->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(NIC, WARN, "MLR event - prBssInfo is NULL.\n");
		return;
	}

	if (IS_BSS_AIS(prBssInfo) &&
		kalGetMediaStateIndicated(
		prAdapter->prGlueInfo,
		prBssInfo->ucBssIndex) ==
		MEDIA_STATE_CONNECTED) {

		prStaRec->ucMlrMode = prEvtMlrFsmUpdate->ucMlrMode;
		prStaRec->ucMlrState = prEvtMlrFsmUpdate->ucMlrState;
		if (MLR_STATE_IN_START(prStaRec->ucMlrState)
			&& MLR_BIT_SUPPORT(prStaRec->ucMlrMode))
			MLR_ENABLE_TX_FRAG(prStaRec);
		else
			MLR_DISABLE_TX_FRAG(prStaRec);

		DBGLOG(NIC, INFO,
		       "MLR event - BSSIDX[%d]WIDX[%d]STARECIDX[%d] prStaRec->fgEnableTxFrag=%d\n",
		       prBssInfo->ucBssIndex,
		       prEvtMlrFsmUpdate->u2WlanIdx,
		       ucStaIdx, prStaRec->fgEnableTxFrag);
	}

	MLR_DBGLOG(prAdapter, NIC, INFO,
		"MLR event - BSSIDX[%d]WIDX[%d]STARECIDX[%d] CONNECTED[%d] ucMlrMode=%d, ucMlrState=%d\n",
		prBssInfo->ucBssIndex,
		prEvtMlrFsmUpdate->u2WlanIdx,
		ucStaIdx,
		kalGetMediaStateIndicated(prAdapter->prGlueInfo,
		prBssInfo->ucBssIndex) == MEDIA_STATE_CONNECTED,
	    prEvtMlrFsmUpdate->ucMlrMode,
	    prEvtMlrFsmUpdate->ucMlrState);
}
#endif

