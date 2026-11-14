// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   connac.c
 *    \brief  Internal driver stack will export the required procedures
 *            here for GLUE Layer.
 *
 *    This file contains all routines which are exported from MediaTek 802.11
 *    Wireless LAN driver stack to GLUE Layer.
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


/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */


/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

#if defined(_HIF_USB)
/*
 * USB Endpoint OUT/DMA Scheduler Group Mapping (HW Define)
 * EP#4 / Group0  (DATA)
 * EP#5 / Group1  (DATA)
 * EP#6 / Group2  (DATA)
 * EP#7 / Group3  (DATA)
 * EP#9 / Group4  (DATA)
 * EP#8 / Group15 (CMD)
 */
uint8_t arAcQIdx2GroupId[MAC_TXQ_NUM] = {
	GROUP0_INDEX,    /* MAC_TXQ_AC0_INDEX */
	GROUP1_INDEX,    /* MAC_TXQ_AC1_INDEX */
	GROUP2_INDEX,    /* MAC_TXQ_AC2_INDEX */
	GROUP3_INDEX,    /* MAC_TXQ_AC3_INDEX */

	GROUP4_INDEX,    /* MAC_TXQ_AC10_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC11_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC12_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC13_INDEX */

	GROUP4_INDEX,    /* MAC_TXQ_AC20_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC21_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC22_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC23_INDEX */

	GROUP4_INDEX,    /* MAC_TXQ_AC30_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC31_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC32_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC33_INDEX */

	GROUP4_INDEX,    /* MAC_TXQ_ALTX_0_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_BMC_0_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_BCN_0_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_PSMP_0_INDEX */

	GROUP5_INDEX,    /* Reserved */
	GROUP5_INDEX,    /* Reserved */
	GROUP5_INDEX,    /* Reserved */
	GROUP5_INDEX,    /* Reserved */

	GROUP5_INDEX,    /* Reserved */
	GROUP5_INDEX,    /* Reserved */
};
#endif /* _HIF_USB */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
#if defined(_HIF_USB)
#define USB_DMA_SHDL_GROUP_DEF_SEQUENCE_ORDER 0xFFFEFFFF
#define USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA 0x3
#define USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA 0x1FF
#define USB_ACCESS_RETRY_LIMIT           1
#endif /* _HIF_USB */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void asicCapInit(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo = NULL;

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	prChipInfo->u2HifTxdSize = 0;
	prChipInfo->u2TxInitCmdPort = 0;
	prChipInfo->u2TxFwDlPort = 0;
	prChipInfo->fillHifTxDesc = NULL;
	prChipInfo->u4ExtraTxByteCount = 0;
	prChipInfo->asicFillInitCmdTxd = asicFillInitCmdTxd;
	prChipInfo->asicFillCmdTxd = asicFillCmdTxd;
	prChipInfo->u2CmdTxHdrSize = sizeof(struct WIFI_CMD);
	prChipInfo->u2RxSwPktBitMap = RXM_RXD_PKT_TYPE_SW_BITMAP;
	prChipInfo->u2RxSwPktEvent = RXM_RXD_PKT_TYPE_SW_EVENT;
	prChipInfo->u2RxSwPktFrame = RXM_RXD_PKT_TYPE_SW_FRAME;
	asicInitTxdHook(prChipInfo->prTxDescOps);
	asicInitRxdHook(prChipInfo->prRxDescOps);
#if (CFG_SUPPORT_MSP == 1)
	prChipInfo->asicRxGetRxv = asicRxGetRxv;
	prChipInfo->asicRxProcessRxvforMSP = asicRxProcessRxvforMSP;
#endif /* CFG_SUPPORT_MSP == 1 */
	prChipInfo->asicRxGetRcpiValueFromRxv =	asicRxGetRcpiValueFromRxv;
#if (CFG_SUPPORT_PERF_IND == 1)
	prChipInfo->asicRxPerfIndProcessRXV = asicRxPerfIndProcessRXV;
#endif
#if (CFG_CHIP_RESET_SUPPORT == 1) && (CFG_WMT_RESET_API_SUPPORT == 0)
	prChipInfo->rst_L0_notify_step2 = conn1_rst_L0_notify_step2;
#endif
#if CFG_SUPPORT_WIFI_SYSDVT
	prAdapter->u2TxTest = TX_TEST_UNLIMITIED;
	prAdapter->u2TxTestCount = 0;
	prAdapter->ucTxTestUP = TX_TEST_UP_UNDEF;
#endif /* CFG_SUPPORT_WIFI_SYSDVT */

	switch (prGlueInfo->u4InfType) {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	case MT_DEV_INF_PCIE:
	case MT_DEV_INF_AXI:
		prChipInfo->u2TxInitCmdPort = TX_RING_FWDL;
		prChipInfo->u2TxFwDlPort = TX_RING_FWDL;
		prChipInfo->ucPacketFormat = TXD_PKT_FORMAT_TXD;
		prChipInfo->u4HifDmaShdlBaseAddr = PCIE_HIF_DMASHDL_BASE;

		HAL_MCR_WR(prAdapter, CONN_HIF_ON_IRQ_ENA, BIT(0));
		break;
#endif /* _HIF_PCIE */
#if defined(_HIF_USB)
	case MT_DEV_INF_USB:
		prChipInfo->u2HifTxdSize = USB_HIF_TXD_LEN;
		prChipInfo->fillHifTxDesc = fillUsbHifTxDesc;
		prChipInfo->u2TxInitCmdPort = USB_DATA_BULK_OUT_EP8;
		prChipInfo->u2TxFwDlPort = USB_DATA_BULK_OUT_EP4;
		prChipInfo->ucPacketFormat = TXD_PKT_FORMAT_TXD_PAYLOAD;
		prChipInfo->u4ExtraTxByteCount =
			EXTRA_TXD_SIZE_FOR_TX_BYTE_COUNT;
		prChipInfo->u4HifDmaShdlBaseAddr = USB_HIF_DMASHDL_BASE;
		asicUdmaTxTimeoutEnable(prAdapter);
		asicUdmaRxFlush(prAdapter, FALSE);
		asicPdmaHifReset(prAdapter, TRUE);
		break;
#endif /* _HIF_USB */
#if defined(_HIF_SDIO)
	case MT_DEV_INF_SDIO:
		prChipInfo->ucPacketFormat = TXD_PKT_FORMAT_TXD_PAYLOAD;
		prChipInfo->u4ExtraTxByteCount =
			EXTRA_TXD_SIZE_FOR_TX_BYTE_COUNT;
		break;
#endif /* _HIF_SDIO */
	default:
		break;
	}
}

void asicEnableFWDownload(struct ADAPTER *prAdapter,
			  u_int8_t fgEnable)
{
	struct GLUE_INFO *prGlueInfo;

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;

	switch (prGlueInfo->u4InfType) {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	case MT_DEV_INF_PCIE:
	case MT_DEV_INF_AXI:
	{
		union WPDMA_GLO_CFG_STRUCT GloCfg;

		HAL_MCR_RD(prAdapter, WPDMA_GLO_CFG, &GloCfg.word);

		GloCfg.field_conn.bypass_dmashdl_txring3 = fgEnable;

		kalDevRegWrite(prGlueInfo, WPDMA_GLO_CFG, GloCfg.word);
	}
	break;
#endif /* _HIF_PCIE */

#if defined(_HIF_USB)
	case MT_DEV_INF_USB:
	{
		uint32_t u4Value = 0;

		ASSERT(prAdapter);

		HAL_MCR_RD(prAdapter, CONNAC_UDMA_TX_QSEL, &u4Value);

		if (fgEnable)
			u4Value |= FW_DL_EN;
		else
			u4Value &= ~FW_DL_EN;

		HAL_MCR_WR(prAdapter, CONNAC_UDMA_TX_QSEL, u4Value);
	}
	break;
#endif /* _HIF_USB */

	default:
		break;
	}
}

void fillNicTxDescAppendWithCR4(struct ADAPTER
				*prAdapter, struct MSDU_INFO *prMsduInfo,
				uint8_t *prTxDescBuffer)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	union HW_MAC_TX_DESC_APPEND *prHwTxDescAppend;

	/* Fill TxD append */
	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *)
			   prTxDescBuffer;
	kalMemZero(prHwTxDescAppend, prChipInfo->txd_append_size);
	prHwTxDescAppend->CR4_APPEND.u2PktFlags =
		HIF_PKT_FLAGS_CT_INFO_APPLY_TXD;
	prHwTxDescAppend->CR4_APPEND.ucBssIndex =
		prMsduInfo->ucBssIndex;
}

void fillTxDescAppendByHost(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, uint16_t u4MsduId,
	phys_addr_t rDmaAddr, uint32_t u4Idx,
	u_int8_t fgIsLast,
	uint8_t *pucBuffer)
{
	union HW_MAC_TX_DESC_APPEND *prHwTxDescAppend;
	struct TXD_PTR_LEN *prPtrLen;

	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *) (
				   pucBuffer + NIC_TX_DESC_LONG_FORMAT_LENGTH);
	prHwTxDescAppend->CONNAC_APPEND.au2MsduId[u4Idx] =
		u4MsduId | TXD_MSDU_ID_VLD;
	prPtrLen = &prHwTxDescAppend->CONNAC_APPEND.arPtrLen[u4Idx >> 1];
	if ((u4Idx & 1) == 0) {
		prPtrLen->u4Ptr0 = rDmaAddr;
		prPtrLen->u2Len0 = prMsduInfo->u2FrameLength | TXD_LEN_ML;
		if (fgIsLast)
			prPtrLen->u2Len0 |= TXD_LEN_AL;
	} else {
		prPtrLen->u4Ptr1 = rDmaAddr;
		prPtrLen->u2Len1 = prMsduInfo->u2FrameLength | TXD_LEN_ML;
		if (fgIsLast)
			prPtrLen->u2Len1 |= TXD_LEN_AL;
	}
}

void fillTxDescAppendByCR4(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, uint16_t u4MsduId,
	phys_addr_t rDmaAddr, uint32_t u4Idx,
	u_int8_t fgIsLast,
	uint8_t *pucBuffer)
{
	union HW_MAC_TX_DESC_APPEND *prHwTxDescAppend;

	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *)
		(pucBuffer + NIC_TX_DESC_LONG_FORMAT_LENGTH);
	prHwTxDescAppend->CR4_APPEND.u2MsduToken = u4MsduId;
	prHwTxDescAppend->CR4_APPEND.ucBufNum = u4Idx + 1;
	prHwTxDescAppend->CR4_APPEND.au4BufPtr[u4Idx] = rDmaAddr;
	prHwTxDescAppend->CR4_APPEND.au2BufLen[u4Idx] =
		prMsduInfo->u2FrameLength;
}

void fillTxDescTxByteCount(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo,
			   void *prTxDesc)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4TxByteCount = NIC_TX_DESC_LONG_FORMAT_LENGTH;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);
	ASSERT(prTxDesc);

	prChipInfo = prAdapter->chip_info;
	u4TxByteCount += prMsduInfo->u2FrameLength;

	if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_DATA)
		u4TxByteCount += prChipInfo->u4ExtraTxByteCount;

	/* Calculate Tx byte count */
	HAL_MAC_TX_DESC_SET_TX_BYTE_COUNT(
		(struct HW_MAC_TX_DESC *)prTxDesc, u4TxByteCount);
}

void fillTxDescTxByteCountWithCR4(struct ADAPTER
				  *prAdapter, struct MSDU_INFO *prMsduInfo,
				  void *prTxDesc)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4TxByteCount = NIC_TX_DESC_LONG_FORMAT_LENGTH;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);
	ASSERT(prTxDesc);

	prChipInfo = prAdapter->chip_info;
	u4TxByteCount += prMsduInfo->u2FrameLength;

	if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_DATA)
		u4TxByteCount += prChipInfo->txd_append_size;

	/* Calculate Tx byte count */
	HAL_MAC_TX_DESC_SET_TX_BYTE_COUNT(
		(struct HW_MAC_TX_DESC *)prTxDesc, u4TxByteCount);
}

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
void asicPcieDmaShdlInit(struct ADAPTER *prAdapter)
{
	uint32_t u4BaseAddr, u4MacVal = 0;
	uint32_t u4GroupCtrl0 = 0, u4GroupCtrl1 = 0, u4GroupCtrl2 = 0,
		u4DmashdlQMap0 = 0;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	uint32_t u4FreePageCnt = 0;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	u4BaseAddr = prChipInfo->u4HifDmaShdlBaseAddr;

	HAL_MCR_RD(prAdapter,
		   CONN_HIF_DMASHDL_PACKET_MAX_SIZE(u4BaseAddr), &u4MacVal);
	u4MacVal &= ~(PLE_PKT_MAX_SIZE_MASK | PSE_PKT_MAX_SIZE_MASK);
	u4MacVal |= PLE_PKT_MAX_SIZE_NUM(0x1);
	u4MacVal |= PSE_PKT_MAX_SIZE_NUM(0x18); /* 0x18 * 128 = 3K */
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_PACKET_MAX_SIZE(u4BaseAddr), u4MacVal);

	u4MacVal =
	(CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP1_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP2_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP3_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP4_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP5_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP6_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP7_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP8_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP9_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP10_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP11_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP12_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP13_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP14_REFILL_DISABLE_MASK |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP15_REFILL_DISABLE_MASK);
	/* Always use group 1 if we support 2 Data TxRing */
	if (prBusInfo->tx_ring0_data_idx != prBusInfo->tx_ring1_data_idx) {
		u4MacVal &=
		~CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP1_REFILL_DISABLE_MASK;
	}
	if (prBusInfo->tx_prio_data_idx) {
		u4MacVal &=
		~CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP2_REFILL_DISABLE_MASK;
	}
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_REFILL_CONTROL(u4BaseAddr), u4MacVal);

	HAL_MCR_RD(prAdapter,
		CONN_HIF_DMASHDL_STATUS_RD(u4BaseAddr), &u4FreePageCnt);
	u4FreePageCnt = (u4FreePageCnt & DMASHDL_FREE_PG_CNT_MASK)
		>> DMASHDL_FREE_PG_CNT_OFFSET;
	u4GroupCtrl0 = DMASHDL_MIN_QUOTA_NUM(0x3);
	u4GroupCtrl0 |= DMASHDL_MAX_QUOTA_NUM(0xFFF);
	if (prBusInfo->tx_ring0_data_idx != prBusInfo->tx_ring1_data_idx) {
		/* HW has no gruantee to switch Quota at runtime */
		/* Just separate equally. */
		u4GroupCtrl1 = DMASHDL_MIN_QUOTA_NUM(0x3);
		u4GroupCtrl1 |= DMASHDL_MAX_QUOTA_NUM(0xFFF);
		/* Wmm1: group 1, others group 0 */
		u4DmashdlQMap0 |= 0x11110000;
	}
	if (prBusInfo->tx_prio_data_idx) {
		u4GroupCtrl2 = DMASHDL_MIN_QUOTA_NUM(0x3);
		u4GroupCtrl2 |= DMASHDL_MAX_QUOTA_NUM(0xFFF);
		u4DmashdlQMap0 &= 0x0FFF0FFF;
		u4DmashdlQMap0 |= 0x20002000;
	}
	HAL_MCR_WR(prAdapter,
		CONN_HIF_DMASHDL_GROUP0_CTRL(u4BaseAddr), u4GroupCtrl0);
	HAL_MCR_WR(prAdapter,
		CONN_HIF_DMASHDL_GROUP1_CTRL(u4BaseAddr), u4GroupCtrl1);
	HAL_MCR_WR(prAdapter,
		CONN_HIF_DMASHDL_GROUP2_CTRL(u4BaseAddr), u4GroupCtrl2);
	HAL_MCR_WR(prAdapter,
		CONN_HIF_DMASHDL_Q_MAP0(u4BaseAddr), u4DmashdlQMap0);

	u4MacVal = 0;
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP3_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP4_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP5_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP6_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP7_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP8_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP9_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP10_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP11_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP12_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP13_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP14_CTRL(u4BaseAddr), u4MacVal);
}

void asicPdmaLoopBackConfig(struct GLUE_INFO *prGlueInfo, u_int8_t fgEnable)
{
	union WPDMA_GLO_CFG_STRUCT GloCfg;
	uint32_t word = 1;

	HAL_MCR_RD(prGlueInfo->prAdapter, WPDMA_GLO_CFG, &GloCfg.word);

	GloCfg.field_conn.bypass_dmashdl_txring3 = 1;
	GloCfg.field_conn.pdma_addr_ext_en = 0;
	GloCfg.field_conn.omit_rx_info = 1;
	GloCfg.field_conn.omit_tx_info = 1;
	GloCfg.field_conn.multi_dma_en = 0;
	GloCfg.field_conn.pdma_addr_ext_en = 0;
	GloCfg.field_conn.tx_dma_en = 1;
	GloCfg.field_conn.rx_dma_en = 1;
	GloCfg.field_conn.multi_dma_en = 0;

	kalDevRegWrite(prGlueInfo, WPDMA_FIFO_TEST_MOD, word);
	kalDevRegWrite(prGlueInfo, WPDMA_GLO_CFG, GloCfg.word);
}

static void configPdmaRxRingThreshold(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4OldVal = 0, u4NewVal = 0;

	if (!prGlueInfo)
		return;

	/* Config RX ring0 & ring1 */
	HAL_MCR_RD(prGlueInfo->prAdapter, WPDMA_PAUSE_RX_Q_TH10, &u4OldVal);
	u4NewVal += (WPDMA_PAUSE_RX_Q_TH0 << WPDMA_PAUSE_RX_Q_TH0_SHFT);
	u4NewVal += (WPDMA_PAUSE_RX_Q_TH1 << WPDMA_PAUSE_RX_Q_TH1_SHFT);
	kalDevRegWrite(prGlueInfo, WPDMA_PAUSE_RX_Q_TH10, u4NewVal);
	DBGLOG(HAL, TRACE, "RX_RING[0, 1] TH(0x%x) from 0x%x to 0x%x\n",
			WPDMA_PAUSE_RX_Q_TH10, u4OldVal, u4NewVal);

	/* Config RX ring2 & ring3 */
	u4OldVal = u4NewVal = 0;
	HAL_MCR_RD(prGlueInfo->prAdapter, WPDMA_PAUSE_RX_Q_TH32, &u4OldVal);
	u4NewVal += (WPDMA_PAUSE_RX_Q_TH2 << WPDMA_PAUSE_RX_Q_TH2_SHFT);
	u4NewVal += (WPDMA_PAUSE_RX_Q_TH3 << WPDMA_PAUSE_RX_Q_TH3_SHFT);
	kalDevRegWrite(prGlueInfo, WPDMA_PAUSE_RX_Q_TH32, u4NewVal);
	DBGLOG(HAL, TRACE, "RX_RING[2, 3] TH(0x%x) from 0x%x to 0x%x\n",
			WPDMA_PAUSE_RX_Q_TH32, u4OldVal, u4NewVal);
}

void asicPdmaIntMaskConfig(struct GLUE_INFO *prGlueInfo,
	uint8_t ucType,
	u_int8_t fgEnable)
{
	struct BUS_INFO *prBusInfo =
			prGlueInfo->prAdapter->chip_info->bus_info;
	union WPDMA_INT_MASK IntMask = {0};

	HAL_MCR_RD(prGlueInfo->prAdapter, WPDMA_INT_MSK, &IntMask.word);

	if (fgEnable == TRUE) {
		if (ucType & BIT(DMA_INT_TYPE_MCU2HOST))
			IntMask.field_conn.mcu2host_sw_int_ena = 1;
		if (ucType & BIT(DMA_INT_TYPE_TRX)) {
			IntMask.field.rx_done_0 = 1;
			IntMask.field.rx_done_1 = 1;
			IntMask.field.tx_done =
				BIT(prBusInfo->tx_ring_fwdl_idx) |
				BIT(prBusInfo->tx_ring_cmd_idx) |
				BIT(prBusInfo->tx_ring0_data_idx) |
				BIT(prBusInfo->tx_ring1_data_idx) |
				BIT(prBusInfo->tx_prio_data_idx);
			IntMask.field_conn.tx_coherent = 0;
			IntMask.field_conn.rx_coherent = 0;
			IntMask.field_conn.tx_dly_int = 0;
			IntMask.field_conn.rx_dly_int = 0;
		}
	} else {
		if (ucType & BIT(DMA_INT_TYPE_MCU2HOST))
			IntMask.field_conn.mcu2host_sw_int_ena = 0;
		if (ucType & BIT(DMA_INT_TYPE_TRX)) {
			IntMask.field_conn.rx_done_0 = 0;
			IntMask.field_conn.rx_done_1 = 0;
			IntMask.field_conn.tx_done = 0;
			IntMask.field_conn.tx_coherent = 0;
			IntMask.field_conn.rx_coherent = 0;
			IntMask.field_conn.tx_dly_int = 0;
			IntMask.field_conn.rx_dly_int = 0;
		}
	}

	DBGLOG(HAL, DEBUG, "type:0x%x, enable:%u, mask:0x%08x\n",
		ucType, fgEnable, IntMask.word);

	kalDevRegWrite(prGlueInfo, WPDMA_INT_MSK, IntMask.word);
}

static void asicEnableSlpProt(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4Val = 0;
	uint32_t u4WaitDelay = 20000;

	HAL_MCR_RD(prGlueInfo->prAdapter,
		   CONN_HIF_PDMA_CSR_PDMA_SLP_PROT_ADDR, &u4Val);
	u4Val |= CONN_HIF_PDMA_CSR_PDMA_SLP_PROT_PDMA_AXI_SLPPROT_ENABLE_MASK;
	kalDevRegWrite(prGlueInfo, CONN_HIF_PDMA_CSR_PDMA_SLP_PROT_ADDR, u4Val);
	while (TRUE) {
		u4WaitDelay--;
		HAL_MCR_RD(prGlueInfo->prAdapter,
			   CONN_HIF_PDMA_CSR_PDMA_SLP_PROT_ADDR,
			   &u4Val);
		if (CONN_HIF_PDMA_CSR_PDMA_SLP_PROT_PDMA_AXI_SLPPROT_RDY_MASK &
				u4Val)
			break;
		if (u4WaitDelay == 0) {
			DBGLOG(HAL, ERROR, "wait for sleep protect timeout.\n");
			GL_DEFAULT_RESET_TRIGGER(prGlueInfo->prAdapter,
						 RST_SLP_PROT_TIMEOUT);
			break;
		}
		kalUdelay(1);
	}
}

static void asicDisableSlpProt(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4Val = 0;

	HAL_MCR_RD(prGlueInfo->prAdapter,
		   CONN_HIF_PDMA_CSR_PDMA_SLP_PROT_ADDR, &u4Val);
	u4Val &= ~CONN_HIF_PDMA_CSR_PDMA_SLP_PROT_PDMA_AXI_SLPPROT_ENABLE_MASK;
	kalDevRegWrite(prGlueInfo, CONN_HIF_PDMA_CSR_PDMA_SLP_PROT_ADDR, u4Val);
}

u_int8_t asicWpdmaWaitIdle(struct GLUE_INFO *prGlueInfo,
	int32_t round, int32_t wait_us)
{
	int32_t i = 0;
	union WPDMA_GLO_CFG_STRUCT GloCfg = {0};

	do {
		HAL_MCR_RD(prGlueInfo->prAdapter,
			   WPDMA_GLO_CFG, &GloCfg.word);
		if ((GloCfg.field.TxDMABusy == 0) &&
		(GloCfg.field.RxDMABusy == 0)) {
			DBGLOG(HAL, TRACE,
				"==>  DMAIdle, GloCfg=0x%x\n", GloCfg.word);
			return TRUE;
		}
		kalUdelay(wait_us);
	} while ((i++) < round);

	DBGLOG(HAL, DEBUG, "==>  DMABusy, GloCfg=0x%x\n", GloCfg.word);

	return FALSE;
}

void asicPdmaConfig(struct GLUE_INFO *prGlueInfo, u_int8_t fgEnable,
		bool fgResetHif)
{
	struct BUS_INFO *prBusInfo =
			prGlueInfo->prAdapter->chip_info->bus_info;
	union WPDMA_GLO_CFG_STRUCT GloCfg;
	uint32_t u4Val = 0;

	asicPdmaIntMaskConfig(prGlueInfo,
		BIT(DMA_INT_TYPE_MCU2HOST) | BIT(DMA_INT_TYPE_TRX),
		fgEnable);
	HAL_MCR_RD(prGlueInfo->prAdapter, WPDMA_GLO_CFG, &GloCfg.word);

	if (fgEnable == TRUE) {
		GloCfg.field_conn.tx_dma_en = 1;
		GloCfg.field_conn.rx_dma_en = 1;
		GloCfg.field_conn.pdma_bt_size = 3;
		GloCfg.field_conn.pdma_addr_ext_en =
			(prBusInfo->u4DmaMask > 32) ? 1 : 0;
		GloCfg.field_conn.tx_wb_ddone = 1;
		GloCfg.field_conn.multi_dma_en = 2;
		GloCfg.field_conn.fifo_little_endian = 1;
		GloCfg.field_conn.clk_gate_dis = 1;
	} else {
		GloCfg.field_conn.tx_dma_en = 0;
		GloCfg.field_conn.rx_dma_en = 0;
	}

	kalDevRegWrite(prGlueInfo, WPDMA_GLO_CFG, GloCfg.word);
	kalDevRegWrite(prGlueInfo, WPDMA_PAUSE_TX_Q, 0);
	kalDevRegWrite(prGlueInfo, MCU2HOST_SW_INT_ENA,
		       ERROR_DETECT_MASK);

	/* Set PDMA APSRC_ACK CR */
	HAL_MCR_RD(prGlueInfo->prAdapter, WPDMA_APSRC_ACK_LOCK_SLPPROT, &u4Val);
	kalDevRegWrite(prGlueInfo, WPDMA_APSRC_ACK_LOCK_SLPPROT,
		u4Val | BIT(4));

	if (fgEnable) {
		kalDevRegWrite(prGlueInfo, WPDMA_PAUSE_TX_Q, 0);
		configPdmaRxRingThreshold(prGlueInfo);
	} else {
		asicWpdmaWaitIdle(prGlueInfo, 100, 1000);
		/* Reset DMA Index */
		kalDevRegWrite(prGlueInfo, WPDMA_RST_PTR, 0xFFFFFFFF);
		if (fgResetHif) {
			asicEnableSlpProt(prGlueInfo);
			halHifRst(prGlueInfo);
			asicDisableSlpProt(prGlueInfo);
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to update DmaSchdl max quota.
 *
 * @param prAdapter
 * @param u2Port, the TxRing number
 * @param u4MaxQuota, the desired max quota for the TxRing.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t asicUpdatTxRingMaxQuota(struct ADAPTER *prAdapter,
	uint8_t ucWmmIndex, uint32_t u4MaxQuota)
{
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4BaseAddr, u4GroupIdx;
	uint32_t u4MacVal = 0, u4SrcCnt, u4RsvCnt, u4TxRingBitmap = 0;
	uint16_t u2Port;

#define DMASHDL_MAX_QUOTA (DMASHDL_MAX_QUOTA_MASK >> DMASHDL_MAX_QUOTA_OFFSET)
	ASSERT(prAdapter);
	if (u4MaxQuota > DMASHDL_MAX_QUOTA)
		u4MaxQuota = DMASHDL_MAX_QUOTA;
#undef DMASHDL_MAX_QUOTA

	prGlueInfo = prAdapter->prGlueInfo;
	u4BaseAddr = prAdapter->chip_info->u4HifDmaShdlBaseAddr;
	u2Port = halRingDataSelectByWmmIndex(prAdapter, ucWmmIndex);

	/* The mapping must be equal to CONN_HIF_DMASHDL_Q_MAP0
	 * in asicPcieDmaShdlInit.
	 */
	switch (u2Port) {
	case TX_RING_DATA0:
		u4GroupIdx = 0;
		break;
	case TX_RING_DATA1:
		u4GroupIdx = 1;
		break;
	case TX_RING_DATA_PRIO:
		u4GroupIdx = 2;
		break;
	default:
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	/* Step 1. Pause the TxRing */
	HAL_MCR_RD(prAdapter, WPDMA_PAUSE_TX_Q, &u4TxRingBitmap);
	kalDevRegWrite(prGlueInfo, WPDMA_PAUSE_TX_Q,
		u4TxRingBitmap |
		(BIT(u2Port) << WPDMA_PAUSE_TX_Q_RINGIDX_OFFSET));

	/* Step 2. Check MaxQuota >= rsv_cnt + src_cnt */
	HAL_MCR_RD(prAdapter,
		CONN_HIF_DMASHDL_STATUS_RD_GP0(u4BaseAddr) + 4*u4GroupIdx,
		&u4MacVal);
	u4SrcCnt = (u4MacVal & DMASHDL_SRC_CNT_MASK) >> DMASHDL_SRC_CNT_OFFSET;
	u4RsvCnt = (u4MacVal & DMASHDL_RSV_CNT_MASK) >> DMASHDL_RSV_CNT_OFFSET;

	/* BE CAREFUL! Caller must call this function again until
	 * WLAN_STATUS_SUCCESS or unlock the TxRing by itself.
	 */
	if (u4MaxQuota < u4SrcCnt+u4RsvCnt) {
		DBGLOG(HAL, DEBUG,
			"WmmQuota,CannotUpdateNow,Port,%u,Grp,%u,reqMax,%u,src,%u,rsv,%u\n",
			u2Port, u4GroupIdx, u4MaxQuota, u4SrcCnt, u4RsvCnt);
		return WLAN_STATUS_PENDING;
	}

	/* Step 3. Update MaxQuota */
	HAL_MCR_RD(prAdapter,
			CONN_HIF_DMASHDL_GROUP0_CTRL(u4BaseAddr) + 4*u4GroupIdx,
			&u4MacVal);
	u4MacVal &= ~(DMASHDL_MAX_QUOTA_NUM(0xFFF));
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(u4MaxQuota);
	HAL_MCR_WR(prAdapter,
		CONN_HIF_DMASHDL_GROUP0_CTRL(u4BaseAddr) + 4*u4GroupIdx,
		u4MacVal);

	/* Step 4. Unlock the TxRing */
	kalDevRegWrite(prGlueInfo, WPDMA_PAUSE_TX_Q,
		u4TxRingBitmap &
		(~(BIT(u2Port) << WPDMA_PAUSE_TX_Q_RINGIDX_OFFSET)));

	return WLAN_STATUS_SUCCESS;
}


void asicEnableInterrupt(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = NULL;

	ASSERT(prAdapter);

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	enable_irq(prHifInfo->u4IrqId);
}

void asicDisableInterrupt(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = NULL;

	ASSERT(prAdapter);

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	disable_irq_nosync(prHifInfo->u4IrqId);
}

void asicLowPowerOwnRead(struct ADAPTER *prAdapter,
			 u_int8_t *pfgResult)
{
	uint32_t u4RegValue = 0;

	HAL_MCR_RD(prAdapter, CONN_HIF_ON_LPCTL, &u4RegValue);
	*pfgResult = (u4RegValue & PCIE_LPCR_HOST_SET_OWN) == 0 ?
		     TRUE : FALSE;
}

void asicLowPowerOwnSet(struct ADAPTER *prAdapter,
			u_int8_t *pfgResult)
{
	uint32_t u4RegValue = 0;

	HAL_MCR_WR(prAdapter, CONN_HIF_ON_LPCTL,
		   PCIE_LPCR_HOST_SET_OWN);
	HAL_MCR_RD(prAdapter, CONN_HIF_ON_LPCTL, &u4RegValue);
	*pfgResult = (u4RegValue & PCIE_LPCR_HOST_SET_OWN) == 1;
}

void asicLowPowerOwnClear(struct ADAPTER *prAdapter,
			  u_int8_t *pfgResult)
{
	uint32_t u4RegValue = 0;

	HAL_MCR_WR(prAdapter, CONN_HIF_ON_LPCTL,
		   PCIE_LPCR_HOST_CLR_OWN);
	HAL_MCR_RD(prAdapter, CONN_HIF_ON_LPCTL, &u4RegValue);
	*pfgResult = (u4RegValue & PCIE_LPCR_HOST_SET_OWN) == 0;
}

#if defined(_HIF_PCIE)
void asicLowPowerOwnClearPCIe(struct ADAPTER *prAdapter,
			  u_int8_t *pfgResult)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHif = NULL;

	prGlueInfo = prAdapter->prGlueInfo;
	prHif = &prGlueInfo->rHifInfo;

	pci_write_config_byte(prHif->pdev,
				PCIE_DOORBELL_PUSH,
				CR_PCIE_CFG_CLEAR_OWN);
}
#endif

bool asicIsValidRegAccess(struct ADAPTER *prAdapter, uint32_t u4Register)
{
	uint32_t au4ExcludeRegs[] = { CONN_HIF_ON_LPCTL };
	uint32_t u4Idx;

	if (wlanIsChipNoAck(prAdapter))
		return false;

	/* driver can access all consys registers on driver own */
	if (!prAdapter->fgIsFwOwn)
		return true;

	/* only own control register can be accessed on fw own */
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4ExcludeRegs); u4Idx++) {
		if (u4Register == au4ExcludeRegs[u4Idx])
			return true;
	}

	return false;
}

void asicGetMailboxStatus(struct ADAPTER *prAdapter,
			  uint32_t *pu4Val)
{
#define WF_MAILBOX_DBG (CONN_HIF_BASE + 0x11C)

	uint32_t u4RegValue = 0;

	HAL_MCR_RD(prAdapter,
		   WF_MAILBOX_DBG, &u4RegValue);
	*pu4Val = u4RegValue;
}

void asicSetDummyReg(struct GLUE_INFO *prGlueInfo)
{
	kalDevRegWrite(prGlueInfo, CONN_DUMMY_CR, PDMA_DUMMY_MAGIC_NUM);
}

void asicCheckDummyReg(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct ADAPTER *prAdapter;
	uint32_t u4Value = 0;
	uint32_t u4Idx;

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	HAL_MCR_RD(prAdapter, CONN_DUMMY_CR, &u4Value);
	DBGLOG(HAL, TRACE, "Check sleep mode DummyReg[0x%x]\n", u4Value);
	if (u4Value != PDMA_DUMMY_RESET_VALUE)
		return;

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		prHifInfo->TxRing[u4Idx].TxSwUsedIdx = 0;
		prHifInfo->TxRing[u4Idx].u4UsedCnt = 0;
		prHifInfo->TxRing[u4Idx].TxCpuIdx = 0;
	}
	DBGLOG(HAL, TRACE, "Weakup from sleep mode\n");

	if (halWpdmaGetRxDmaDoneCnt(prGlueInfo, RX_RING_EVT)) {
		DBGLOG(HAL, TRACE, "Force to read RX event\n");
		KAL_SET_BIT(RX_RING_EVT, prAdapter->ulNoMoreRfb);
	}
	if (halWpdmaGetRxDmaDoneCnt(prGlueInfo, RX_RING_DATA0)) {
		DBGLOG(HAL, TRACE, "Force to read RX data\n");
		KAL_SET_BIT(RX_RING_DATA0, prAdapter->ulNoMoreRfb);
	}
	/* Write sleep mode magic num to dummy reg */
	asicSetDummyReg(prGlueInfo);
}

void asicPdmaTxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *tx_ring,
	uint32_t index)
{
	struct BUS_INFO *prBusInfo;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t phy_addr_ext = 0, ext_offset = 0;
	struct RTMP_DMACB *prTxCell;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prTxCell = &tx_ring->Cell[0];

#ifdef CONFIG_PHYS_ADDR_T_64BIT
	phy_addr_ext = (((uint64_t)prTxCell->AllocPa >>
			DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK);
#endif
	ext_offset = index * MT_RINGREG_EXT_DIFF;

	tx_ring->hw_desc_base_ext =
		prBusInfo->host_tx_ring_ext_ctrl_base + ext_offset;

	HAL_MCR_WR(prAdapter, tx_ring->hw_desc_base_ext,
			phy_addr_ext);
}

void asicPdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	uint32_t index)
{
	struct BUS_INFO *prBusInfo;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t phy_addr_ext = 0, ext_offset = 0;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

#ifdef CONFIG_PHYS_ADDR_T_64BIT
	phy_addr_ext = (((uint64_t)rx_ring->Cell[0].AllocPa >>
			DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK);
#endif
	ext_offset = index * MT_RINGREG_EXT_DIFF;
	rx_ring->hw_desc_base_ext =
		prBusInfo->host_rx_ring_ext_ctrl_base + ext_offset;

	HAL_MCR_WR(prAdapter, rx_ring->hw_desc_base_ext,
			phy_addr_ext);
}
#endif /* _HIF_PCIE || _HIF_AXI */

#if defined(_HIF_USB)
/* DMS Scheduler Init */
void asicUsbDmaShdlGroupInit(struct ADAPTER *prAdapter,
			     uint32_t u4RefillGroup)
{
	uint32_t u4BaseAddr, u4MacVal = 0;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4CfgVal = 0;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;
	u4BaseAddr = prChipInfo->u4HifDmaShdlBaseAddr;

	HAL_MCR_RD(prAdapter,
		   CONN_HIF_DMASHDL_PACKET_MAX_SIZE(u4BaseAddr), &u4MacVal);
	u4MacVal &= ~(PLE_PKT_MAX_SIZE_MASK | PSE_PKT_MAX_SIZE_MASK);
	u4MacVal |= PLE_PKT_MAX_SIZE_NUM(0x1);
	u4MacVal |= PSE_PKT_MAX_SIZE_NUM(0x8);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_PACKET_MAX_SIZE(u4BaseAddr), u4MacVal);

	u4RefillGroup |=
	(CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP2_REFILL_PRIORITY_MASK
	|
	CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP3_REFILL_PRIORITY_MASK);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_REFILL_CONTROL(u4BaseAddr), u4RefillGroup);

	/* Use "User program group sequence order" by default.[16]1'b0	*/
	HAL_MCR_RD(prAdapter,
		   CONN_HIF_DMASHDL_PAGE_SETTING(u4BaseAddr), &u4MacVal);
	u4MacVal &= USB_DMA_SHDL_GROUP_DEF_SEQUENCE_ORDER;
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_PAGE_SETTING(u4BaseAddr), u4MacVal);

#if CFG_SUPPORT_CFG_FILE
	u4CfgVal = wlanCfgGetUint32(prAdapter,
				    "DmaShdlGroup1MinQuota",
				    USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA,
				    FEATURE_DEBUG_ONLY);
	u4MacVal = DMASHDL_MIN_QUOTA_NUM(u4CfgVal);
	u4CfgVal = wlanCfgGetUint32(prAdapter,
				    "DmaShdlGroup1MaxQuota",
				    USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA,
				    FEATURE_DEBUG_ONLY);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(u4CfgVal);
#else /* CFG_SUPPORT_CFG_FILE */
	u4MacVal = DMASHDL_MIN_QUOTA_NUM(
			   USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(
			    USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA);
#endif /* !CFG_SUPPORT_CFG_FILE */
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP1_CTRL(u4BaseAddr), u4MacVal);

#if CFG_SUPPORT_CFG_FILE
	u4CfgVal = wlanCfgGetUint32(prAdapter,
				    "DmaShdlGroup0MinQuota",
				    USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA,
				    FEATURE_DEBUG_ONLY);
	u4MacVal = DMASHDL_MIN_QUOTA_NUM(u4CfgVal);
	u4CfgVal = wlanCfgGetUint32(prAdapter,
				    "DmaShdlGroup0MaxQuota",
				    USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA,
				    FEATURE_DEBUG_ONLY);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(u4CfgVal);
#else /* CFG_SUPPORT_CFG_FILE */
	u4MacVal = DMASHDL_MIN_QUOTA_NUM(
			   USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(
			    USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA);
#endif /* !CFG_SUPPORT_CFG_FILE */
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP0_CTRL(u4BaseAddr), u4MacVal);

#if CFG_SUPPORT_CFG_FILE
	u4CfgVal = wlanCfgGetUint32(prAdapter,
				    "DmaShdlGroup2MinQuota",
				    USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA,
				    FEATURE_DEBUG_ONLY);
	u4MacVal = DMASHDL_MIN_QUOTA_NUM(u4CfgVal);
	u4CfgVal = wlanCfgGetUint32(prAdapter,
				    "DmaShdlGroup2MaxQuota",
				    USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA,
				    FEATURE_DEBUG_ONLY);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(u4CfgVal);
#else /* CFG_SUPPORT_CFG_FILE */
	u4MacVal = DMASHDL_MIN_QUOTA_NUM(
			   USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(
			    USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA);
#endif /* !CFG_SUPPORT_CFG_FILE */
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP2_CTRL(u4BaseAddr), u4MacVal);

#if CFG_SUPPORT_CFG_FILE
	u4CfgVal = wlanCfgGetUint32(prAdapter,
				    "DmaShdlGroup3MinQuota",
				    USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA,
				    FEATURE_DEBUG_ONLY);
	u4MacVal = DMASHDL_MIN_QUOTA_NUM(u4CfgVal);
	u4CfgVal = wlanCfgGetUint32(prAdapter,
				    "DmaShdlGroup3MaxQuota",
				    USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA,
				    FEATURE_DEBUG_ONLY);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(u4CfgVal);
#else /* CFG_SUPPORT_CFG_FILE */
	u4MacVal = DMASHDL_MIN_QUOTA_NUM(
			   USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(
			    USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA);
#endif /* !CFG_SUPPORT_CFG_FILE */
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP3_CTRL(u4BaseAddr), u4MacVal);

#if CFG_SUPPORT_CFG_FILE
	u4CfgVal = wlanCfgGetUint32(prAdapter,
				    "DmaShdlGroup4MinQuota",
				    USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA,
				    FEATURE_DEBUG_ONLY);
	u4MacVal = DMASHDL_MIN_QUOTA_NUM(u4CfgVal);
	u4CfgVal = wlanCfgGetUint32(prAdapter,
				    "DmaShdlGroup4MaxQuota",
				    USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA,
				    FEATURE_DEBUG_ONLY);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(u4CfgVal);
#else /* CFG_SUPPORT_CFG_FILE */
	u4MacVal = DMASHDL_MIN_QUOTA_NUM(
			   USB_DMA_SHDL_GROUP_DEF_MIN_QUOTA);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(
			    USB_DMA_SHDL_GROUP_DEF_MAX_QUOTA);
#endif /* !CFG_SUPPORT_CFG_FILE */
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_GROUP4_CTRL(u4BaseAddr), u4MacVal);

	u4MacVal = ((arAcQIdx2GroupId[MAC_TXQ_AC0_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE0_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC1_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE1_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC2_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE2_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC3_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE3_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC10_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE4_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC11_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE5_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC12_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE6_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC13_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE7_MAPPING));
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_Q_MAP0(u4BaseAddr),
		   u4MacVal);

	u4MacVal = ((arAcQIdx2GroupId[MAC_TXQ_AC20_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE8_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC21_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE9_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC22_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE10_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC23_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE11_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC30_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE12_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC31_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE13_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC32_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE14_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_AC33_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE15_MAPPING));
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_Q_MAP1(u4BaseAddr),
		   u4MacVal);

	u4MacVal = ((arAcQIdx2GroupId[MAC_TXQ_ALTX_0_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING2_QUEUE16_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_BMC_0_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING2_QUEUE17_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_BCN_0_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING2_QUEUE18_MAPPING) |
		    (arAcQIdx2GroupId[MAC_TXQ_PSMP_0_INDEX] <<
		     CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING2_QUEUE19_MAPPING));
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_Q_MAP2(u4BaseAddr),
		   u4MacVal);
}

void asicUsbDmaShdlInit(struct ADAPTER *prAdapter)
{
	uint32_t u4BaseAddr, u4MacVal;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;
	u4BaseAddr = prChipInfo->u4HifDmaShdlBaseAddr;

	/*
	 * Enable refill control group 0, 1, 2, 3, 4.
	 * Keep all group low refill priority to prevent low
	 * group starvation if we have high group.
	 */
	u4MacVal =
	(CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP5_REFILL_DISABLE_MASK
	 |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP6_REFILL_DISABLE_MASK
	 |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP7_REFILL_DISABLE_MASK
	 |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP8_REFILL_DISABLE_MASK
	 |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP9_REFILL_DISABLE_MASK
	 |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP10_REFILL_DISABLE_MASK
	 |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP11_REFILL_DISABLE_MASK
	 |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP12_REFILL_DISABLE_MASK
	 |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP13_REFILL_DISABLE_MASK
	 |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP14_REFILL_DISABLE_MASK
	 |
	 CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP15_REFILL_DISABLE_MASK);

	asicUsbDmaShdlGroupInit(prAdapter, u4MacVal);

	/*
	 * HIF Scheduler Setting
	 * Group15(CMD) is highest priority.
	 */
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_SHDL_SET0(u4BaseAddr), 0x6501234f);
	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_SHDL_SET1(u4BaseAddr), 0xedcba987);

	HAL_MCR_WR(prAdapter,
		   CONN_HIF_DMASHDL_OPTIONAL_CONTROL(u4BaseAddr), 0x7004801c);
}

u_int8_t asicUsbSuspend(struct ADAPTER *prAdapter,
			struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4Value;
	uint32_t count = 0;
	int32_t ret = 0;
	struct BUS_INFO *prBusInfo;

	TRACE_FUNC(HAL, DEBUG, "%s ---->\n");
	prBusInfo = prAdapter->chip_info->bus_info;

	/* Disable PDMA TX */
	HAL_MCR_RD(prAdapter, PDMA_IF_MISC, &u4Value);
	u4Value &= ~PDMA_IF_MISC_TX_ENABLE_MASK;
	HAL_MCR_WR(prAdapter, PDMA_IF_MISC, u4Value);

	/* Set PDMA Debug flag */
	u4Value = 0x00000116;
	HAL_MCR_WR(prAdapter, PDMA_DEBUG_EN, u4Value);
	/* Polling PDMA_dmashdl_request done  */
	while (count < PDMA_TX_IDLE_WAIT_COUNT) {
		HAL_MCR_RD(prAdapter, PDMA_DEBUG_STATUS, &u4Value);
		DBGLOG(HAL, DEBUG, "%s: 0x%08x = 0x%08x\n", __func__,
		       PDMA_DEBUG_STATUS, u4Value);
		if (!(u4Value & PDMA_DEBUG_DMASHDL_REQUEST_DONE_MASK)
		    && (count >= 3))
			break;
		mdelay(1);
		count++;
	}

	if (count >= PDMA_TX_IDLE_WAIT_COUNT) {
		DBGLOG(HAL, ERROR,
			"%s:: 2.2 suspend fail, enable PDMA TX again.\n",
			__func__);
		/* Enable PDMA TX again */
		HAL_MCR_RD(prAdapter, PDMA_IF_MISC, &u4Value);
		u4Value |= PDMA_IF_MISC_TX_ENABLE_MASK;
		HAL_MCR_WR(prAdapter, PDMA_IF_MISC, u4Value);
		return FALSE;
	}

	u4Value = 0x00000101;
	HAL_MCR_WR(prAdapter, PDMA_DEBUG_EN, u4Value);
	count = 0;
	while (count < PDMA_TX_IDLE_WAIT_COUNT) {
		HAL_MCR_RD(prAdapter, PDMA_DEBUG_STATUS, &u4Value);
		DBGLOG(HAL, DEBUG, "%s:: 0x%08x = 0x%08x\n",
		       __func__, PDMA_DEBUG_STATUS, u4Value);
		if ((u4Value == PDMA_DEBUG_TX_STATUS_MASK)
		    && (count >= 3)) {
			DBGLOG(HAL, ERROR, "%s:: PDMA Tx idle~\n", __func__);
			break;
		}
		DBGLOG(HAL, ERROR, "%s:: PDMA Tx busy.....\n", __func__);
		count++;
	}

	if (count >= PDMA_TX_IDLE_WAIT_COUNT) {
		DBGLOG(HAL, ERROR,
			"%s:: 2.4 suspend fail, enable PDMA TX again.\n",
			__func__);
		/* Enable PDMA TX again */
		HAL_MCR_RD(prAdapter, PDMA_IF_MISC, &u4Value);
		u4Value |= PDMA_IF_MISC_TX_ENABLE_MASK;
		HAL_MCR_WR(prAdapter, PDMA_IF_MISC, u4Value);
		return FALSE;
	}

	prGlueInfo->rHifInfo.state = USB_STATE_SUSPEND;
	halDisableInterrupt(prGlueInfo->prAdapter);
	halTxCancelAllSending(prGlueInfo->prAdapter);

	ret = usb_control_msg(prGlueInfo->rHifInfo.udev,
			      usb_sndctrlpipe(prGlueInfo->rHifInfo.udev, 0),
			      VND_REQ_FEATURE_SET,
			      prBusInfo->u4device_vender_request_out,
			      FEATURE_SET_WVALUE_SUSPEND, 0,
			      NULL, 0,
			      VENDOR_TIMEOUT_MS);
	if (ret) {
		DBGLOG(HAL, ERROR,
		"%s:: VendorRequest FeatureSetResume ERROR:", __func__);
		DBGLOG(HAL, ERROR,
		" %x, enable PDMA TX again.\n", (unsigned int)ret);
		/* Enable PDMA TX again */
		HAL_MCR_RD(prAdapter, PDMA_IF_MISC, &u4Value);
		u4Value |= PDMA_IF_MISC_TX_ENABLE_MASK;
		HAL_MCR_WR(prAdapter, PDMA_IF_MISC, u4Value);
		TRACE_FUNC(HAL, DEBUG, "%s <----\n");
		return FALSE;
	}
	TRACE_FUNC(HAL, DEBUG, "%s <----\n");
	return TRUE;
}

uint8_t asicUsbEventEpDetected(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	int32_t ret = 0;
	uint8_t ucRetryCount = 0;
	u_int8_t ucEp5Disable = FALSE;

	ASSERT(FALSE == 0);
	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	if (prHifInfo->fgEventEpDetected == FALSE) {
		prHifInfo->fgEventEpDetected = TRUE;
		do {
			ret = mtk_usb_vendor_request(prGlueInfo, 0,
					prBusInfo->u4device_vender_request_in,
						     VND_REQ_EP5_IN_INFO,
						     0, 0, &ucEp5Disable,
						     sizeof(ucEp5Disable));
			if (ret || ucRetryCount)
				DBGLOG(HAL, ERROR,
				       "usb_control_msg() status: %x retry: %u\n",
				       (unsigned int)ret, ucRetryCount);
			ucRetryCount++;
			if (ucRetryCount > USB_ACCESS_RETRY_LIMIT)
				break;
		} while (ret);

		if (ret) {
			kalSendAeeWarning(HIF_USB_ERR_TITLE_STR,
				HIF_USB_ERR_DESC_STR
				"USB() reports error: %x retry: %u",
				ret, ucRetryCount);
			DBGLOG(HAL, ERROR,
			  "usb_readl() reports error: %x retry: %u\n", ret,
			  ucRetryCount);
		} else {
			DBGLOG(HAL, DEBUG,
				"%s: Get ucEp5Disable = %d\n", __func__,
			  ucEp5Disable);
			if (ucEp5Disable)
				prHifInfo->eEventEpType = EVENT_EP_TYPE_DATA_EP;
		}
	}
	if (prHifInfo->eEventEpType == EVENT_EP_TYPE_DATA_EP)
		return USB_DATA_EP_IN;
	else
		return USB_EVENT_EP_IN;
}

void asicUdmaTxTimeoutEnable(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo;
	uint32_t u4Value;

	prBusInfo = prAdapter->chip_info->bus_info;
	HAL_MCR_RD(prAdapter, prBusInfo->u4UdmaWlCfg_1_Addr,
		   &u4Value);
	u4Value &= ~UDMA_WLCFG_1_TX_TIMEOUT_LIMIT_MASK;
	u4Value |= UDMA_WLCFG_1_TX_TIMEOUT_LIMIT(
			   prBusInfo->u4UdmaTxTimeout);
	HAL_MCR_WR(prAdapter, prBusInfo->u4UdmaWlCfg_1_Addr,
		   u4Value);

	HAL_MCR_RD(prAdapter, prBusInfo->u4UdmaWlCfg_0_Addr,
		   &u4Value);
	u4Value |= UDMA_WLCFG_0_TX_TIMEOUT_EN_MASK;
	HAL_MCR_WR(prAdapter, prBusInfo->u4UdmaWlCfg_0_Addr,
		   u4Value);
}

void asicUdmaRxFlush(struct ADAPTER *prAdapter,
		     u_int8_t bEnable)
{
	struct BUS_INFO *prBusInfo;
	uint32_t u4Value;

	prBusInfo = prAdapter->chip_info->bus_info;

	HAL_MCR_RD(prAdapter, prBusInfo->u4UdmaWlCfg_0_Addr,
		   &u4Value);
	if (bEnable)
		u4Value |= UDMA_WLCFG_0_RX_FLUSH_MASK;
	else
		u4Value &= ~UDMA_WLCFG_0_RX_FLUSH_MASK;
	HAL_MCR_WR(prAdapter, prBusInfo->u4UdmaWlCfg_0_Addr,
		   u4Value);
}

void asicPdmaHifReset(struct ADAPTER *prAdapter,
		      u_int8_t bRelease)
{
	uint32_t u4Value;

	HAL_MCR_RD(prAdapter, PDMA_HIF_RESET, &u4Value);
	if (bRelease)
		u4Value |= DPMA_HIF_LOGIC_RESET_MASK;
	else
		u4Value &= ~DPMA_HIF_LOGIC_RESET_MASK;
	HAL_MCR_WR(prAdapter, PDMA_HIF_RESET, u4Value);
}
#endif /* _HIF_USB */

static void asicFillInitCmdTxdInfo(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	u_int8_t *pucSeqNum)
{
	struct INIT_HIF_TX_HEADER *prInitHifTxHeader;
	struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES
			*prInitHifTxHeaderPending;
	uint32_t u4TxdLen =
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES);

	prInitHifTxHeaderPending =
		(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES *)
		(prCmdInfo->pucInfoBuffer);
	prInitHifTxHeader = (struct INIT_HIF_TX_HEADER *)
		(prCmdInfo->pucInfoBuffer + u4TxdLen);

	prInitHifTxHeaderPending->u2TxByteCount = prCmdInfo->u2InfoBufLen;
	if (!prCmdInfo->ucCID) {
		prInitHifTxHeaderPending->u2PQ_ID =
			INIT_CMD_PDA_PQ_ID;
		prInitHifTxHeaderPending->ucHeaderFormat =
			INIT_CMD_PDA_PACKET_TYPE_ID;
		prInitHifTxHeaderPending->ucPktFt =
			INIT_PKT_FT_PDA_FWDL;
	} else {
		prInitHifTxHeaderPending->u2PQ_ID =
			INIT_CMD_PQ_ID;
		prInitHifTxHeaderPending->ucHeaderFormat =
			INIT_CMD_PACKET_TYPE_ID;
		prInitHifTxHeaderPending->ucPktFt =
			INIT_PKT_FT_CMD;
	}

	prInitHifTxHeader->rInitWifiCmd.ucCID = prCmdInfo->ucCID;
	prInitHifTxHeader->rInitWifiCmd.ucPktTypeID = prCmdInfo->ucPktTypeID;
	prInitHifTxHeader->rInitWifiCmd.ucSeqNum =
		nicIncreaseCmdSeqNum(prAdapter);
	prInitHifTxHeader->u2TxByteCount =
		prInitHifTxHeaderPending->u2TxByteCount - u4TxdLen;

	if (pucSeqNum)
		*pucSeqNum = prInitHifTxHeader->rInitWifiCmd.ucSeqNum;

	DBGLOG_LIMITED(INIT, DEBUG, "TX CMD: ID[0x%02X] SEQ[%u] LEN[%u]\n",
			prInitHifTxHeader->rInitWifiCmd.ucCID,
			prInitHifTxHeader->rInitWifiCmd.ucSeqNum,
			prInitHifTxHeader->u2TxByteCount);
}

static void asicFillCmdTxdInfo(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	u_int8_t *pucSeqNum)
{
	struct WIFI_CMD *prWifiCmd;

	prWifiCmd = (struct WIFI_CMD *)prCmdInfo->pucInfoBuffer;

	prWifiCmd->u2TxByteCount = prCmdInfo->u2InfoBufLen;
	prWifiCmd->u2PQ_ID =
		CMD_PQ_ID;
	prWifiCmd->ucHeaderFormat =
		CMD_PACKET_TYPE_ID;
	prWifiCmd->ucPktFt =
		TXD_PKT_FT_CMD;

	prWifiCmd->ucCID = prCmdInfo->ucCID;
	prWifiCmd->ucExtenCID = prCmdInfo->ucExtCID;
	prWifiCmd->ucPktTypeID = prCmdInfo->ucPktTypeID;
	prWifiCmd->ucSetQuery = prCmdInfo->ucSetQuery;
	prWifiCmd->ucSeqNum = nicIncreaseCmdSeqNum(prAdapter);
	prWifiCmd->ucS2DIndex = S2D_INDEX_CMD_H2N_H2C;
	prWifiCmd->u2Length =
		prWifiCmd->u2TxByteCount
		- (uint16_t) OFFSET_OF(struct WIFI_CMD, u2Length);

	if (pucSeqNum)
		*pucSeqNum = prWifiCmd->ucSeqNum;

	DBGLOG_LIMITED(INIT, DEBUG,
			"TX CMD: ID[0x%02X] SEQ[%u] SET[%u] LEN[%u]\n",
			prWifiCmd->ucCID, prWifiCmd->ucSeqNum,
			prWifiCmd->ucSetQuery, prWifiCmd->u2Length);
}


void asicFillInitCmdTxd(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	uint16_t *pu2BufInfoLen,
	u_int8_t *pucSeqNum,
	void **pCmdBuf)
{
	struct INIT_HIF_TX_HEADER *prInitHifTxHeader;

	prInitHifTxHeader = (struct INIT_HIF_TX_HEADER *)
		(prCmdInfo->pucInfoBuffer +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES));

	if (!prCmdInfo->ucCID) {
		*pu2BufInfoLen += sizeof(struct INIT_HIF_TX_HEADER) +
		sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES);
		prCmdInfo->u2InfoBufLen = *pu2BufInfoLen;
	}
	asicFillInitCmdTxdInfo(prAdapter, prCmdInfo, pucSeqNum);

	if (pCmdBuf)
		*pCmdBuf = prInitHifTxHeader->rInitWifiCmd.aucBuffer;
}

void asicFillCmdTxd(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	u_int8_t *pucSeqNum,
	void **pCmdBuf)
{
	struct WIFI_CMD *prWifiCmd;

	prWifiCmd = (struct WIFI_CMD *)prCmdInfo->pucInfoBuffer;
	asicFillCmdTxdInfo(prAdapter, prCmdInfo, pucSeqNum);

	if (pCmdBuf)
		*pCmdBuf = &prWifiCmd->aucBuffer[0];
}

void asicInitTxdHook(
	struct TX_DESC_OPS_T *prTxDescOps)
{
	ASSERT(prTxDescOps);
	prTxDescOps->nic_txd_long_format_op = nic_txd_v1_long_format_op;
	prTxDescOps->nic_txd_tid_op = nic_txd_v1_tid_op;
	prTxDescOps->nic_txd_queue_idx_op = nic_txd_v1_queue_idx_op;
#if (CFG_TCP_IP_CHKSUM_OFFLOAD == 1)
	prTxDescOps->nic_txd_chksum_op = nic_txd_v1_chksum_op;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD == 1 */
	prTxDescOps->nic_txd_header_format_op = nic_txd_v1_header_format_op;
	prTxDescOps->nic_txd_fill_by_pkt_option =
		nic_txd_v1_fill_by_pkt_option;
	prTxDescOps->nic_txd_compose = nic_txd_v1_compose;
	prTxDescOps->nic_txd_set_pkt_fixed_rate_option_full =
		nic_txd_v1_set_pkt_fixed_rate_option_full;
	prTxDescOps->nic_txd_set_pkt_fixed_rate_option =
		nic_txd_v1_set_pkt_fixed_rate_option;
	prTxDescOps->nic_txd_set_hw_amsdu_template =
		nic_txd_v1_set_hw_amsdu_template;
	prTxDescOps->nic_txd_change_data_port_by_ac =
		nic_txd_v1_change_data_port_by_ac;
	prTxDescOps->u2TxdFrNstsMask = TX_DESC_NSTS_MASK;
	prTxDescOps->ucTxdFrNstsOffset = TX_DESC_NSTS_OFFSET;
	prTxDescOps->u2TxdFrStbcMask = TX_DESC_STBC;
}

void asicInitRxdHook(
	struct RX_DESC_OPS_T *prRxDescOps)
{
	ASSERT(prRxDescOps);
	prRxDescOps->nic_rxd_get_rx_byte_count = nic_rxd_v1_get_rx_byte_count;
	prRxDescOps->nic_rxd_get_pkt_type = nic_rxd_v1_get_packet_type;
	prRxDescOps->nic_rxd_get_wlan_idx = nic_rxd_v1_get_wlan_idx;
	prRxDescOps->nic_rxd_get_sec_mode = nic_rxd_v1_get_sec_mode;
	prRxDescOps->nic_rxd_get_sw_class_error_bit =
		nic_rxd_v1_get_sw_class_error_bit;
	prRxDescOps->nic_rxd_get_ch_num = nic_rxd_v1_get_ch_num;
	prRxDescOps->nic_rxd_get_rf_band = nic_rxd_v1_get_rf_band;
	prRxDescOps->nic_rxd_get_tcl = nic_rxd_v1_get_tcl;
	prRxDescOps->nic_rxd_get_ofld = nic_rxd_v1_get_ofld;
	prRxDescOps->nic_rxd_get_HdrTrans = nic_rxd_v1_get_HdrTrans;
	prRxDescOps->nic_rxd_fill_rfb = nic_rxd_v1_fill_rfb;
	prRxDescOps->nic_rxd_sanity_check = nic_rxd_v1_sanity_check;
#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
	prRxDescOps->nic_rxd_check_wakeup_reason =
		nic_rxd_v1_check_wakeup_reason;
#endif /* CFG_SUPPORT_WAKEUP_REASON_DEBUG */
}

#if (CFG_SUPPORT_MSP == 1)
void asicRxGetRxv(struct SW_RFB *prRetSwRfb, uint32_t *prRxV)
{
	struct HW_MAC_RX_STS_GROUP_3 *prGroup3;

	prGroup3 = prRetSwRfb->prRxStatusGroup3;
	if (!prGroup3)
		return;

	prRxV[0] = HAL_RX_VECTOR_GET_RX_VECTOR(prGroup3, 0);
	prRxV[1] = HAL_RX_VECTOR_GET_RX_VECTOR(prGroup3, 1);
	prRxV[2] = HAL_RX_VECTOR_GET_RX_VECTOR(prGroup3, 2);
	prRxV[3] = HAL_RX_VECTOR_GET_RX_VECTOR(prGroup3, 3);
	prRxV[4] = HAL_RX_VECTOR_GET_RX_VECTOR(prGroup3, 4);
}

void asicRxProcessRxvforMSP(struct ADAPTER *prAdapter,
	struct SW_RFB *prRetSwRfb)
{
	uint32_t *prRxV = NULL; /* pointer to destination buffer to store RxV */

	if (prRetSwRfb->ucStaRecIdx >= CFG_STA_REC_NUM) {
		DBGLOG(RX, LOUD,
		"prRetSwRfb->ucStaRecIdx(%d) >= CFG_STA_REC_NUM(%d)\n",
			prRetSwRfb->ucStaRecIdx, CFG_STA_REC_NUM);
		return;
	}

	if (prRetSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3)) {
		prRxV = prAdapter->arStaRec[prRetSwRfb->ucStaRecIdx].au4RxV;
		asicRxGetRxv(prRetSwRfb, prRxV);
	}
}
#endif /* CFG_SUPPORT_MSP */

uint8_t asicRxGetRcpiValueFromRxv(
	uint8_t ucRcpiMode,
	struct SW_RFB *prSwRfb)
{
	uint8_t ucRcpi0, ucRcpi1;
	uint8_t ucRcpiValue = 0;
	struct HW_MAC_RX_STS_GROUP_3 *prGroup3;

	ASSERT(prSwRfb);

	if (ucRcpiMode >= RCPI_MODE_NUM) {
		DBGLOG(RX, WARN,
		"Rcpi Mode=%d is invalid for getting uint8_t value from RXV\n",
			ucRcpiMode);
		return 0;
	}

	prGroup3 = (struct HW_MAC_RX_STS_GROUP_3 *)prSwRfb->prRxStatusGroup3;
	ucRcpi0 = HAL_RX_STATUS_GET_RCPI0(prGroup3);
	ucRcpi1 = HAL_RX_STATUS_GET_RCPI1(prGroup3);

	switch (ucRcpiMode) {
	case RCPI_MODE_WF0:
		ucRcpiValue = ucRcpi0;
		break;

	case RCPI_MODE_WF1:
		ucRcpiValue = ucRcpi1;
		break;

	case RCPI_MODE_WF2:
	case RCPI_MODE_WF3:
		DBGLOG(RX, WARN,
		"Rcpi Mode = %d is invalid for", ucRcpiMode);
		DBGLOG(RX, WARN,
		" device with only 2 antenna, use default rcpi0\n");
		ucRcpiValue = ucRcpi0;
		break;

	case RCPI_MODE_AVG: /*Not recommended for CBW80+80*/
		if (ucRcpi0 <= RCPI_HIGH_BOUND &&
			ucRcpi1 <= RCPI_HIGH_BOUND)
			ucRcpiValue = (ucRcpi0 + ucRcpi1) / 2;
		else
			ucRcpiValue = ucRcpi0 <= RCPI_HIGH_BOUND ?
				(ucRcpi0) : (ucRcpi1);
		break;

	case RCPI_MODE_MAX:
		if (ucRcpi0 <= RCPI_HIGH_BOUND &&
			ucRcpi1 <= RCPI_HIGH_BOUND)
			ucRcpiValue =
				(ucRcpi0 > ucRcpi1) ?
				(ucRcpi0) : (ucRcpi1);
		else
			ucRcpiValue = ucRcpi0 <= RCPI_HIGH_BOUND ?
				(ucRcpi0) : (ucRcpi1);
		break;

	case RCPI_MODE_MIN:
		ucRcpiValue =
			(ucRcpi0 < ucRcpi1) ? (ucRcpi0) : (ucRcpi1);
		break;

	default:
		break;
	}

	if (ucRcpiValue < RCPI_MEASUREMENT_NOT_AVAILABLE)
		return ucRcpiValue;

	DBGLOG(RX, ERROR,
	       "Invalid ucRcpiValue: %d\n", ucRcpiValue);
	return 0;
}

#if (CFG_SUPPORT_PERF_IND == 1)
void asicRxPerfIndProcessRXV(struct ADAPTER *prAdapter,
			       struct SW_RFB *prSwRfb,
			       uint8_t ucBssIndex)
{
	/* This Feature First MP on MT6779 */
	struct GLUE_INFO *prGlueInfo;
	struct HW_MAC_RX_STS_GROUP_3 *prRxStatusGroup3 = NULL;
	uint32_t u4PhyRate;
	uint8_t ucRCPI0 = 0, ucRCPI1 = 0;
	uint16_t u2Rate = 0; /* Unit 500 Kbps */
	struct RxRateInfo rRxRateInfo = {0};
	int status;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prGlueInfo = prAdapter->prGlueInfo;
	status = __wlanGetRxRateByBssid(prGlueInfo, ucBssIndex, &u4PhyRate,
			NULL, &rRxRateInfo, FALSE);
	/* ucRate(500kbs) = u4PhyRate(100kbps) */
	if (status < 0 || u4PhyRate == 0)
		return;

	if (prSwRfb->prRxStatusGroup3 == NULL)
		return;

	u2Rate = u4PhyRate / 5;

	/* RCPI */
	prRxStatusGroup3 = prSwRfb->prRxStatusGroup3;
	ucRCPI0 = HAL_RX_STATUS_GET_RCPI0(prRxStatusGroup3);
	ucRCPI1 = HAL_RX_STATUS_GET_RCPI1(prRxStatusGroup3);

	/* Record peak rate to Traffic Indicator*/
	if (u2Rate > prGlueInfo->PerfIndCache.u2CurRxRate[ucBssIndex]) {
		prGlueInfo->PerfIndCache.u2CurRxRate[ucBssIndex] = u2Rate;
		prGlueInfo->PerfIndCache.ucCurRxNss[ucBssIndex] =
							rRxRateInfo.u4Nss;
		prGlueInfo->PerfIndCache.ucCurRxRCPI0[ucBssIndex] = ucRCPI0;
		prGlueInfo->PerfIndCache.ucCurRxRCPI1[ucBssIndex] = ucRCPI1;
	}
}
#endif


#if (CFG_CHIP_RESET_SUPPORT == 1) && (CFG_WMT_RESET_API_SUPPORT == 0)
u_int8_t conn1_rst_L0_notify_step2(void)
{
	typedef int (*p_bt_fun_type) (void);
	p_bt_fun_type bt_func;
	char *bt_func_name = "WF_rst_L0_notify_BT_step2";
	void *pvAddr = NULL;

	DBGLOG(INIT, STATE, "[SER][L0] %s\n", bt_func_name);
	pvAddr = GLUE_SYMBOL_GET(bt_func_name);
	if (pvAddr) {
		bt_func = (p_bt_fun_type) pvAddr;
		bt_func();
		GLUE_SYMBOL_PUT(bt_func_name);
	} else {
		DBGLOG(INIT, WARN, "[SER][L0] %s does not exist\n",
							bt_func_name);
		return FALSE;
	}
	return TRUE;
}
#endif

int connsys_power_on(void)
{
	return 0;
}

int connsys_power_done(void)
{
	return 0;
}

void connsys_power_off(void)
{
}

#if CFG_MTK_ANDROID_WMT
static int wlanWmtCbGetBusCnt(void)
{
	struct wireless_dev *prWdev = gprWdev[0];
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(prWdev->wiphy, prGlueInfo);
	if (!prGlueInfo)
		return 0;

	return prGlueInfo->rHifInfo.u4HifCnt;
}

static int wlanWmtCbClrBusCnt(void)
{
	struct wireless_dev *prWdev = gprWdev[0];
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(prWdev->wiphy, prGlueInfo);
	if (prGlueInfo)
		prGlueInfo->rHifInfo.u4HifCnt = 0;

	return 0;
}

static int wlanWmtCbSetMpuProtect(bool enable)
{
	struct mt66xx_hif_driver_data *data = get_platform_driver_data();

#if CFG_MTK_ANDROID_EMI
	kalSetEmiMpuProtection(emi_mem_get_phy_base(data->chip_info),
		enable);
#endif
	return 0;
}

static int wlanWmtCbIsWifiDrvOwn(void)
{
	struct wireless_dev *prWdev = gprWdev[0];
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(prWdev->wiphy, prGlueInfo);
	if (!prGlueInfo || !prGlueInfo->prAdapter)
		return 0;

	return (prGlueInfo->prAdapter->fgIsFwOwn == FALSE) ? 1 : 0;
}

void unregister_plat_connsys_cbs(void)
{
	mtk_wcn_wmt_wlan_unreg();
}

void register_plat_connsys_cbs(void)
{
	struct _MTK_WCN_WMT_WLAN_CB_INFO rWmtCb;

	kalMemZero(&rWmtCb, sizeof(struct _MTK_WCN_WMT_WLAN_CB_INFO));
	rWmtCb.wlan_probe_cb = wlanFuncOn;
	rWmtCb.wlan_remove_cb = wlanFuncOff;
	rWmtCb.wlan_bus_cnt_get_cb = wlanWmtCbGetBusCnt;
	rWmtCb.wlan_bus_cnt_clr_cb = wlanWmtCbClrBusCnt;
	rWmtCb.wlan_emi_mpu_set_protection_cb = wlanWmtCbSetMpuProtect;
	rWmtCb.wlan_is_wifi_drv_own_cb = wlanWmtCbIsWifiDrvOwn;

	mtk_wcn_wmt_wlan_reg(&rWmtCb);
}
#endif

